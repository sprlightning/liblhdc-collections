#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "lhdcv5BT_dec.h"

#define LOG_NDEBUG 0
#define LOG_TAG "lhdcv5BT_dec"
#include "common/bt_trace.h"

// static uint8_t serial_no = 0xff;
static uint8_t serial_no = 0xfe; // 0xfe 作为“未初始化”标记，0~255 为有效序列号
static uint8_t last_seqno = 0xfe; // 记录上一次接收的 seqno，用于判断连续丢包

// description
//   a function to log in LHDC decoder library
// Parameter
//   msg: char string to print
static void print_log_cb(char *msg)
{
  if (msg == NULL) {
    return;
  }

  LOG_INFO("[V5Dec_lib] %s", msg);
}


// description
//   check number of frames in one packet and return pointer to first byte of 1st frame in current packet
// Parameter
//   input: pointer to input buffer
//   input_len: length (bytes) of input buffer pointed by input
//   pLout: pointer to pointer to output buffer
//   pLlen: length (bytes) of encoded stream in output buffer
//   upd_seq_no: sequence number type
// return:
//   > 0: number of frames in current packet
//   == 0: No frames in current packet
//   < 0: error
static int32_t assemble_lhdcv5_packet(uint32_t *frame_num, uint8_t *input, uint32_t input_len,
    uint8_t **pLout, uint32_t *pLlen, int upd_seq_no)
{
  uint8_t hdr = 0, seqno = 0xff;
  uint32_t status = 0;
  uint32_t lhdc_total_frame_nb = 0;

  if ((input == NULL) ||
      (pLout == NULL) ||
      (pLlen == NULL)) {
    LOG_ERROR("%s: null ptr", __func__);
    return -1;
  }

  if (input_len < 2) {
    LOG_ERROR("%s: input len too small", __func__);
    return -1;
  }

  // 提取 LHDC V5 头部（第1字节hdr，第2字节seqno）
  hdr = (*input);
  input++;
  seqno = (*input);
  input++;
  input_len -= 2;

  // Check latency and update value when changed.
  status = hdr & A2DP_LHDC_HDR_LATENCY_MASK;

  // 解析帧数量（从hdr的bit0~1提取）
  status = (hdr & A2DP_LHDC_HDR_FRAME_NO_MASK) >> 2;

  if (status <= 0) {
    LOG_WARN("%s: no any frame in packet.", __func__);
    *frame_num = 0;
    return 0;
  }

  lhdc_total_frame_nb = status;

  // 序列号同步逻辑
  // if (seqno != serial_no) {
  //   // LOG_INFO("%s: packet lost! now(%d), expect(%d)", __func__, seqno, serial_no);
  //   LOG_WARN("%s: packet lost! reset serial_no from %d to %d", __func__, serial_no, seqno);
  //   serial_no = seqno; // 仅同步，不递增
  //   // 仅在连续丢包时返回错误，单次不返回
  //   //return -1;
  // }
  if (serial_no == 0xfe) {
    // 首次同步：无警告，直接初始化序列号
    serial_no = seqno;
    last_seqno = seqno;
    LOG_DEBUG("%s: first packet sync, serial_no initialized to %d", __func__, seqno);
  } else {
    // 计算差值（处理8位溢出）
    uint8_t diff = (seqno - serial_no + 256) % 256;
    if (diff == 0) {
      // 序列号一致：正常
      last_seqno = seqno;
    } else if (diff == 1 || diff == 255) {
      // 正常递增或溢出循环：静默同步
      serial_no = seqno;
      last_seqno = seqno;
      LOG_DEBUG("%s: seqno sync (normal), serial_no updated to %d", __func__, seqno);
    } else {
      // 真丢包：打印警告（仅连续跳变时）
      uint8_t lost_count = diff;
      LOG_WARN("%s: real packet lost! expected serial_no=%d, received seqno=%d, lost %d packets", 
               __func__, serial_no, seqno, lost_count);
      serial_no = seqno;
      last_seqno = seqno;
    }
  }

  // 序列号递增（处理8位溢出，循环到0）
  if (upd_seq_no == LHDCBT_DEC_UPD_SEQ_NO) {
    // serial_no = seqno + 1;
    serial_no = (seqno + 1) % 256; // 溢出后从0重新开始
  }

  // 输出缓冲区配置
  *pLlen = input_len;
  *pLout = input;

  *frame_num = (int) lhdc_total_frame_nb;

  LOG_DEBUG("%s: total frame number (%d)", __func__, *frame_num);
  return 0;
}


// description
//   init. LHDC V5 decoder
// Parameter
//   handle: codec handle(ptr for heap) from bt stack
//   config: configuration for LHDC V5 decoder
// return:
//   == 0: succeed
//   != 0: error code
int32_t lhdcv5BT_dec_init_decoder(HANDLE_LHDCV5_BT *handle, tLHDCV5_DEC_CONFIG *config)
{
  int32_t func_ret = LHDCV5_UTIL_DEC_SUCCESS;
  uint32_t mem_req_bytes = 0;
  HANDLE_LHDCV5_BT hLhdcBT = NULL;

  LOG_INFO("%s: decoder lib version = %s", __func__, lhdcv5_util_dec_get_version());

  if (handle == NULL || config == NULL) {
    LOG_INFO("%s: null ptr handle %p config %p", __func__, handle, config);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  LOG_INFO("%s: bits_depth:%u sample_rate=%u bit_rate=%u version=%d lossless_enable=%d", __func__,
      config->bits_depth, config->sample_rate, config->bit_rate, config->version, config->lossless_enable);

  if ((config->bits_depth != LHDCV5BT_BIT_DEPTH_16) &&
      (config->bits_depth != LHDCV5BT_BIT_DEPTH_24) &&
      (config->bits_depth != LHDCV5BT_BIT_DEPTH_32)) {
    LOG_INFO("%s: bits_depth %d not supported", __func__, config->bits_depth);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  if ((config->sample_rate != LHDCV5BT_SAMPLE_RATE_44K) &&
      (config->sample_rate != LHDCV5BT_SAMPLE_RATE_48K) &&
      (config->sample_rate != LHDCV5BT_SAMPLE_RATE_96K) &&
      (config->sample_rate != LHDCV5BT_SAMPLE_RATE_192K)) {
    LOG_INFO("%s: sample_rate %d not supported", __func__, config->sample_rate);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  if ((0 > config->bit_rate) || (config->bit_rate > LHDCV5BT_BIT_RATE_1000K)) {
    LOG_INFO("%s: bit_rate %d not supported", __func__, config->bit_rate);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  if ((config->version != VERSION_5)) {
    LOG_INFO("%s: version %d not supported", __func__, config->version);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  if ((config->lossless_enable != 0) &&
      (config->lossless_enable != 1)) {
    LOG_INFO("%s: lossless enable invalid value %d ", __func__, config->lossless_enable);
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  lhdcv5_util_dec_register_log_cb(&print_log_cb);

  func_ret = lhdcv5_util_dec_get_mem_req(config->version, &mem_req_bytes);
  if (func_ret != LHDCV5_UTIL_DEC_SUCCESS || mem_req_bytes <= 0) {
    LOG_WARN("%s: Fail to get required memory size (%d)!", __func__, func_ret);
    return LHDCV5BT_DEC_API_ALLOC_MEM_FAIL;
  }

  hLhdcBT = (HANDLE_LHDCV5_BT)malloc(mem_req_bytes);
  if (hLhdcBT == NULL) {
    LOG_WARN("%s: Fail to allocate memory!", __func__);
    return LHDCV5BT_DEC_API_ALLOC_MEM_FAIL;
  }

  LOG_INFO("%s: init lhdcv5 decoder...", __func__);
  //TODO: send mem_req_bytes for size check
  func_ret = lhdcv5_util_init_decoder(hLhdcBT, config->bits_depth,
      config->sample_rate, config->bit_rate, config->lossless_enable, config->version);
  if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
    LOG_WARN("%s: failed to init decoder (%d)!", __func__, func_ret);
    free(hLhdcBT);
    return LHDCV5BT_DEC_API_INIT_DECODER_FAIL;
  }

  *handle = hLhdcBT;
  if ((*handle) == NULL) {
    LOG_WARN("%s: handle return NULL!", __func__);
    return LHDCV5BT_DEC_API_INIT_DECODER_FAIL;
  }

  func_ret = lhdcv5_util_dec_channel_selsect(LHDC_OUTPUT_STEREO);
  if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
    LOG_WARN("%s: failed to configure channel (%d)!", __func__, func_ret);
    return LHDCV5BT_DEC_API_CHANNEL_SETUP_FAIL;
  }

  // serial_no = 0xff; // 禁用，会导致重复初始化

  LOG_INFO("%s: init lhdcv5 decoder success", __func__);
  return LHDCV5BT_DEC_API_SUCCEED;
}


// description
//   check whether all frames of one packet are in buffer?
// Parameter
//   frameData: pointer to input buffer
//   frameBytes: length (bytes) of input buffer pointed by frameData
//   packetBytes: return the final number of queued data in decoder lib (for validation)
// return:
//   == 0: succeed
//   < 0: error
int32_t lhdcv5BT_dec_check_frame_data_enough(const uint8_t *frameData,
    uint32_t frameBytes, uint32_t *packetBytes)
{
  uint8_t *frameDataStart = (uint8_t *)frameData;
  uint8_t *in_buf = NULL;
  uint32_t in_len = 0;
  uint32_t frame_num = 0;
  lhdc_frame_Info_t lhdc_frame_Info;
  uint32_t ptr_offset = 0;
  int32_t func_ret = LHDCV5_UTIL_DEC_SUCCESS;

  if ((frameData == NULL) || (packetBytes == NULL)) {
    return LHDCV5_UTIL_DEC_ERROR_PARAM;
  }

  *packetBytes = 0;

  func_ret = assemble_lhdcv5_packet(&frame_num, frameDataStart, frameBytes, &in_buf, &in_len,
      LHDCBT_DEC_NOT_UPD_SEQ_NO);
  if (func_ret < 0 || in_buf == NULL) {
    LOG_ERROR("%s: failed setup input buffer", __func__);
    return LHDCV5BT_DEC_API_FAIL;
  }

  if (frame_num == 0) {
    return LHDCV5BT_DEC_API_SUCCEED;
  }

  LOG_INFO("%s: incoming frame size(%d), decoding size(%d), total frame num(%d)", __func__,
      frameBytes, in_len, frame_num);

  ptr_offset = 0;

  while ((frame_num > 0) && (ptr_offset < in_len))
  {
    func_ret = lhdcv5_util_dec_fetch_frame_info(in_buf + ptr_offset, in_len, &lhdc_frame_Info);
    if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
      LOG_INFO("%s: fetch frame info fail (%d)", __func__, func_ret);
      return LHDCV5BT_DEC_API_FRAME_INFO_FAIL;
    }

    LOG_INFO("%s: frame_num[%d]: ptr_offset (%d), frame_len (%d)", __func__,
        (int)frame_num, (int)ptr_offset, (int)lhdc_frame_Info.frame_len);

    if ((ptr_offset + lhdc_frame_Info.frame_len) > in_len) {
      LOG_INFO(" %s: frame_num[%d]: Not Enough... ptr_offset(%d), frame_len(%d)",
          __func__, (int)frame_num, (int)ptr_offset, (int)lhdc_frame_Info.frame_len);
      return LHDCV5BT_DEC_API_INPUT_NOT_ENOUGH;
    }

    ptr_offset += lhdc_frame_Info.frame_len;

    frame_num--;
  }

  *packetBytes = ptr_offset;

  return LHDCV5BT_DEC_API_SUCCEED;
}


// description
//   decode all frames in one packet
// Parameter
//   frameData: pointer to input buffer from bt stack
//   frameBytes: length (bytes) of input buffer pointed by frameData
//   pcmData: pointer to output buffer to bt stack
//   pcmBytes: length (bytes) of pcm samples in output buffer
//   bits_depth: bit per sample
// return:
//   == 0: succeed
//   < 0: error
int32_t lhdcv5BT_dec_decode(const uint8_t *frameData, uint32_t frameBytes,
    uint8_t *pcmData, uint32_t *pcmBytes, uint32_t bits_depth)
{
  uint8_t *frameDataStart = (uint8_t *)frameData;
  uint32_t dec_sum = 0;
  uint32_t lhdc_out_len = 0;
  uint8_t *in_buf = NULL;   //buffer position to input to decode process
  uint32_t in_len = 0;
  uint32_t frame_num = 0;
  lhdc_frame_Info_t lhdc_frame_Info;
  uint32_t ptr_offset = 0;
  uint32_t frame_samples;
  uint32_t frame_bytes;
  uint32_t pcmSpaceBytes;
  int32_t func_ret = LHDCV5_UTIL_DEC_SUCCESS;

  LOG_DEBUG("%s: enter frameBytes %d", __func__, (int)frameBytes);

  if ((frameData == NULL) ||
      (pcmData == NULL) ||
      (pcmBytes == NULL)) {
    return LHDCV5BT_DEC_API_INVALID_INPUT;
  }

  pcmSpaceBytes = *pcmBytes;
  *pcmBytes = 0;

  /*
  if(frameBytes >= 16) {
    for(int i=0; i<16; i++) {
      LOG_INFO(" %s: dumpFrame[%d]= 0x%02X", __func__, i, (int)*(frameDataStart+i));
    }
  } else {
    for(int i=0; i<(int)frameBytes; i++) {
      LOG_INFO(" %s: dumpFrame[%d]= 0x%02X", __func__, i, (int)*(frameDataStart+i));
    }
  }
   */

  func_ret = assemble_lhdcv5_packet(&frame_num, frameDataStart, frameBytes, &in_buf, &in_len,
      LHDCBT_DEC_UPD_SEQ_NO);
  if (func_ret < 0 || in_buf == NULL) {
    LOG_ERROR("%s: failed setup input buffer", __func__);
    return LHDCV5BT_DEC_API_FAIL;
  }

  if (frame_num == 0) {
    return LHDCV5BT_DEC_API_SUCCEED;
  }

  func_ret = lhdcv5_util_dec_get_sample_size(&frame_samples);
  if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
    LOG_WARN("%s: fetch frame samples failed (%d)", __func__, func_ret);
    return LHDCV5BT_DEC_API_FRAME_INFO_FAIL;
  }
  LOG_DEBUG("%s: output frame samples %d", __func__, (int)frame_samples);

  if (bits_depth == LHDCV5BT_BIT_DEPTH_16) {
    frame_bytes = frame_samples * 2 * 2; // 16位，2声道
  } else if (bits_depth == LHDCV5BT_BIT_DEPTH_24) {
    frame_bytes = frame_samples * 3 * 2; // 24位，2声道
  } else {
    frame_bytes = frame_samples * 4 * 2; // 32位，2声道
  }

  ptr_offset = 0;
  dec_sum = 0;

  while ((frame_num > 0) && (ptr_offset < in_len))
  {
    func_ret = lhdcv5_util_dec_fetch_frame_info(in_buf + ptr_offset, in_len, &lhdc_frame_Info);
    if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
      LOG_INFO("%s: fetch frame info fail (%d)", __func__, func_ret);
      return LHDCV5BT_DEC_API_FRAME_INFO_FAIL;
    }

    if ((ptr_offset + lhdc_frame_Info.frame_len) > in_len) {
      return LHDCV5BT_DEC_API_INPUT_NOT_ENOUGH;
    }

    if ((dec_sum + frame_bytes) > pcmSpaceBytes) {
      return LHDCV5BT_DEC_API_OUTPUT_NOT_ENOUGH;
    }

    LOG_DEBUG("%s: get ptr_offset=%d, dec_sum=%d", __func__, ptr_offset, dec_sum);
    func_ret = lhdcv5_util_dec_process(
        ((uint8_t *)pcmData) + dec_sum,
        in_buf + ptr_offset,  // 从帧起始位置开始解析
        lhdc_frame_Info.frame_len,
        &lhdc_out_len);
    if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
      LOG_WARN("%s: decode fail (%d)", __func__, func_ret);
      return LHDCV5BT_DEC_API_DECODE_FAIL;
    }

    LOG_DEBUG("%s: frame_num[%d]: input_frame_len %d output_len %d", __func__,
        (int)frame_num, (int)lhdc_frame_Info.frame_len, (int)lhdc_out_len);

    ptr_offset += lhdc_frame_Info.frame_len;
    dec_sum += lhdc_out_len;

    frame_num--;
  }

  *pcmBytes = (uint32_t) dec_sum;

  return LHDCV5BT_DEC_API_SUCCEED;
}


// description
//   de-initialize (free) all resources allocated by LHDC V5 decoder
// Parameter
//   none
// return:
//   == 0: success
int32_t lhdcv5BT_dec_deinit_decoder(HANDLE_LHDCV5_BT handle)
{
  int32_t func_ret = 0;

  if(handle == NULL) {
    LOG_INFO("%s: empty handle", __func__);
    return LHDCV5BT_DEC_API_SUCCEED;
  }

  func_ret = lhdcv5_util_dec_destroy();
  if (func_ret != LHDCV5_UTIL_DEC_SUCCESS) {
    LOG_INFO("%s: deinit decoder error (%d)", __func__, func_ret);
    return LHDCV5BT_DEC_API_FAIL;
  }

  if(handle) {
    LOG_INFO("%s: free handle %p!", __func__, handle);
    free(handle);
  }

  return LHDCV5BT_DEC_API_SUCCEED;
}


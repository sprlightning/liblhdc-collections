/**
 * SPDX-FileCopyrightText: 2025 The Android Open Source Project
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * a2dp_vendor_lhdcv5_decoder.c
 * 
 * 本文件依赖外部库liblhdcv5dec中的lhdcv5BT_dec.h
 * 库liblhdcv5dec中的lhdcv5_util_dec.c无解码功能，其解码部分使用正弦波发生器替代
 * 需要实现lhdcv5_util_dec.c中所有内容，或者使用动态库lhdcv5_util_dec.so、lhdcv5BT_dec.so
 */

#include "common/bt_trace.h"
#include "stack/a2dp_vendor_lhdcv5.h"
#include "stack/a2dp_vendor_lhdc_constants.h"
#include "stack/a2dp_vendor_lhdcv5_constants.h"
#include "stack/a2dp_vendor_lhdcv5_decoder.h"

#if (defined(LHDCV5_DEC_INCLUDED) && LHDCV5_DEC_INCLUDED == TRUE)

typedef struct {
    bool initialized;
    HANDLE_LHDCV5_BT lhdc_handle;
    uint32_t sample_rate;
    uint8_t channel_count;
    uint8_t bits_per_sample;
    decoded_data_callback_t decode_callback;
} tA2DP_LHDCV5_DECODER_CB;

static tA2DP_LHDCV5_DECODER_CB a2dp_lhdcv5_decoder_cb;
static const tA2DP_DECODER_INTERFACE lhdcv5_decoder_interface;

// 初始化LHDC V5解码器
bool a2dp_lhdcv5_decoder_init(decoded_data_callback_t decode_callback) {
    LOG_INFO("%s: Initializing LHDC V5 decoder", __func__);
    memset(&a2dp_lhdcv5_decoder_cb, 0, sizeof(tA2DP_LHDCV5_DECODER_CB));

    a2dp_lhdcv5_decoder_cb.decode_callback = decode_callback;
    a2dp_lhdcv5_decoder_cb.initialized = true;
    
    LOG_INFO("%s: LHDC V5 decoder initialized", __func__);
    return true;
}

void a2dp_lhdcv5_decoder_cleanup() {
    LOG_INFO("%s: Cleaning up LHDC V5 decoder", __func__);
    
    if (a2dp_lhdcv5_decoder_cb.initialized) {
        if (a2dp_lhdcv5_decoder_cb.lhdc_handle != NULL) {
            lhdcv5BT_dec_deinit_decoder(a2dp_lhdcv5_decoder_cb.lhdc_handle);
            a2dp_lhdcv5_decoder_cb.lhdc_handle = NULL;
        }
        a2dp_lhdcv5_decoder_cb.initialized = false;
        a2dp_lhdcv5_decoder_cb.decode_callback = NULL;
    }

    LOG_INFO("%s: LHDC V5 decoder cleaned up", __func__);
}

ssize_t a2dp_lhdcv5_decoder_decode_packet_header(BT_HDR* p_buf) {
    // 跳过A2DP头部
    size_t header_len = sizeof(struct media_packet_header) + 
                        A2DP_LHDC_MPL_HDR_LEN;
    p_buf->offset += header_len;
    p_buf->len -= header_len;
    return 0;
}

// 错误码转描述字符串
static const char* lhdcv5_get_error_desc(int32_t error_code) {
    switch (error_code) {
        case LHDCV5BT_DEC_API_SUCCEED:
            return "success";
        case LHDCV5BT_DEC_API_FAIL:
            return "fail";
        case LHDCV5BT_DEC_API_INVALID_INPUT:
            return "invalid input";
        case LHDCV5BT_DEC_API_INVALID_OUTPUT:
            return "invalid output";
        case LHDCV5BT_DEC_API_INVALID_SEQ_NO:
            return "invalid seq no";
        case LHDCV5BT_DEC_API_INIT_DECODER_FAIL:
            return "init decoder fail";
        case LHDCV5BT_DEC_API_CHANNEL_SETUP_FAIL:
            return "channel setup fail";
        case LHDCV5BT_DEC_API_FRAME_INFO_FAIL:
            return "frame info fail";
        case LHDCV5BT_DEC_API_INPUT_NOT_ENOUGH:
            return "input not enough";
        case LHDCV5BT_DEC_API_OUTPUT_NOT_ENOUGH:
            return "output not enough";
        case LHDCV5BT_DEC_API_DECODE_FAIL:
            return "decode fail";
        case LHDCV5BT_DEC_API_ALLOC_MEM_FAIL:
            return "alloc mem fail";
        default:
            return "unknown error";
    }
}

// LHDC V5解码函数
bool a2dp_lhdcv5_decoder_decode_packet(BT_HDR* p_buf, unsigned char* buf, size_t buf_len) {
    if (!a2dp_lhdcv5_decoder_cb.initialized || p_buf == NULL || buf == NULL) {
        return false;
    }

    // 获取payload数据
    uint8_t* payload = (uint8_t*)(p_buf + 1) + p_buf->offset;
    uint16_t payload_len = p_buf->len;
    
    uint32_t decoded_bytes = buf_len;
    int32_t ret = lhdcv5BT_dec_decode(
        payload,
        payload_len,
        buf,
        &decoded_bytes,
        a2dp_lhdcv5_decoder_cb.bits_per_sample
    );

    if (ret != LHDCV5BT_DEC_API_SUCCEED) {
        const char* err_desc = lhdcv5_get_error_desc(ret);
        LOG_ERROR("%s: decode error. result = %d(%s)", __func__, ret, err_desc);
        return false;
    }
    
    // 调用回调函数传递解码后的数据
    if (a2dp_lhdcv5_decoder_cb.decode_callback) {
        a2dp_lhdcv5_decoder_cb.decode_callback(buf, decoded_bytes);
    }
    
    return true;
}

void a2dp_lhdcv5_decoder_start() {
    LOG_INFO("%s: Starting LHDC V5 decoder", __func__);
}

void a2dp_lhdcv5_decoder_suspend() {
    LOG_INFO("%s: Suspending LHDC V5 decoder", __func__);
}

// 配置LHDCV5解码器
void a2dp_lhdcv5_decoder_configure(const uint8_t* p_codec_info) {
    if (!a2dp_lhdcv5_decoder_cb.initialized || p_codec_info == NULL) {
        return;
    }

    LOG_INFO( "Enter: %s", __func__);

    tA2DP_LHDCV5_CIE cie;
    tA2D_STATUS status;
    status = A2DP_ParseInfoLhdcV5(&cie, (uint8_t*)p_codec_info, false, IS_SNK);
    if (status != A2D_SUCCESS) {
        LOG_ERROR("%s: failed parsing codec info. %d", __func__, status);
        return;
    }

    // 映射LHDC采样率
    switch (cie.sampleRate & A2DP_LHDCV5_SAMPLING_FREQ_MASK) {
        case A2DP_LHDCV5_SAMPLING_FREQ_44100:
            a2dp_lhdcv5_decoder_cb.sample_rate = 44100;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_48000:
            a2dp_lhdcv5_decoder_cb.sample_rate = 48000;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_96000:
            a2dp_lhdcv5_decoder_cb.sample_rate = 96000;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_192000:
            a2dp_lhdcv5_decoder_cb.sample_rate = 192000;
            break;
        default:
            LOG_ERROR("%s: Unsupported sample rate: 0x%02x", __func__, cie.sampleRate);
            return;
    }
    LOG_INFO("%s: LHDC V5 Sampling frequency = %lu", __func__, a2dp_lhdcv5_decoder_cb.sample_rate);

    // 映射通道模式
    if (cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_STEREO) {
        a2dp_lhdcv5_decoder_cb.channel_count = 2;
        LOG_INFO("%s: LHDC V5 Channel mode: Stereo", __func__);
    } else if (cie.channelMode & A2DP_LHDCV5_CHANNEL_MODE_MONO) {
        a2dp_lhdcv5_decoder_cb.channel_count = 1;
        LOG_INFO("%s: LHDC V5 Channel mode: Mono", __func__);
    } else {
        LOG_ERROR("%s: Unsupported channel mode: 0x%02x", __func__, cie.channelMode);
        return;
    }

    // 映射位深
    switch (cie.bitsPerSample & A2DP_LHDCV5_BIT_FMT_MASK) {
        case A2DP_LHDCV5_BIT_FMT_32:
            a2dp_lhdcv5_decoder_cb.bits_per_sample = 32;
            break;
        case A2DP_LHDCV5_BIT_FMT_24:
            a2dp_lhdcv5_decoder_cb.bits_per_sample = 24;
            break;
        case A2DP_LHDCV5_BIT_FMT_16:
            a2dp_lhdcv5_decoder_cb.bits_per_sample = 16;
            break;
        default:
            LOG_ERROR("%s: Unsupported bits per sample: 0x%02x", __func__, cie.bitsPerSample);
            return;
    }
    LOG_INFO("%s: LHDC V5 Bit depth = %d", __func__, a2dp_lhdcv5_decoder_cb.bits_per_sample);

    // 如果已存在解码器句柄，先释放
    if (a2dp_lhdcv5_decoder_cb.lhdc_handle != NULL) {
        lhdcv5BT_dec_deinit_decoder(a2dp_lhdcv5_decoder_cb.lhdc_handle);
        a2dp_lhdcv5_decoder_cb.lhdc_handle = NULL;
    }

    // 初始化解码器配置
    tLHDCV5_DEC_CONFIG config = {0};
    config.version = VERSION_5;
    config.sample_rate = a2dp_lhdcv5_decoder_cb.sample_rate;
    config.bits_depth = a2dp_lhdcv5_decoder_cb.bits_per_sample;
    config.lossless_enable = (cie.hasFeatureLL && (cie.hasFeatureLLESS24Bit || cie.hasFeatureLLESS48K || cie.hasFeatureLLESS96K)) ? 1 : 0;

    // 初始化解码器
    int32_t ret = lhdcv5BT_dec_init_decoder(&a2dp_lhdcv5_decoder_cb.lhdc_handle, &config);
    if (ret != LHDCV5BT_DEC_API_SUCCEED || a2dp_lhdcv5_decoder_cb.lhdc_handle == NULL) {
        APPL_TRACE_ERROR("%s: Failed to initialize LHDC V5 decoder: %d", __func__, ret);
        return;
    }

    LOG_INFO("%s: LHDC V5 decoder configured - Sample rate: %d, Channels: %d, Bits: %d",
             __func__, a2dp_lhdcv5_decoder_cb.sample_rate, a2dp_lhdcv5_decoder_cb.channel_count, 
             a2dp_lhdcv5_decoder_cb.bits_per_sample);
}

// LHDCV5 decoder interface，已转移到a2dp_vendor_lhdcv5.c
// static const tA2DP_DECODER_INTERFACE lhdcv5_decoder_interface = {
//     a2dp_lhdcv5_decoder_init,
//     a2dp_lhdcv5_decoder_cleanup,
//     NULL,
//     a2dp_lhdcv5_decoder_decode_pocket_header,
//     a2dp_lhdcv5_decoder_decode_packet,
//     a2dp_lhdcv5_decoder_start,
//     a2dp_lhdcv5_decoder_suspend,
//     a2dp_lhdcv5_decoder_configure,
// };

// const tA2DP_DECODER_INTERFACE* A2DP_LHDCV5_DecoderInterface(void) {
//     return &lhdcv5_decoder_interface;
// }

#endif /* LHDCV5_DEC_INCLUDED */
/*
 * lhdcv5_util_dec.c
 * 
 * 适用于ESP-IDF的LHDC V5 util解码器实现；
 * 生成正弦波用于测试解码器流程
 * 由于无法获得LHDC帧解码方式，目前核心解码功能是用正弦波发生器替代，正常情况下，须链接Savitech LHDC V5官方库；
 * 如获得静态库(lhdcv5_util_dec.a)，那就不需要本文件了，直接在CMakeLists.txt中链接即可；
 * 一种思路是集成FLAC或ALAC等开源无损音频编码的解码库，如libFLAC，可是测试失败了，flac解码器无法识别LHDC V5数据；
 * 此外还有一种思路是实现一个简单的PCM直通模式，尝试将输入数据直接转发到输出，也失败了，因为LHDC V5的数据不是PCM格式；
 */

#include "lhdcv5_util_dec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common/bt_trace.h"

// ------------------------ Internal State ------------------------
static int decoder_initialized = 0;
static uint32_t g_bitPerSample = 16;
static uint32_t g_output_bit_per_sample = 32; // 存储输出位深
static uint32_t g_sampleRate = 44100;
static uint32_t g_scaleTo16Bits = 1;
static uint32_t g_is_lossless_enable = 0;
static lhdc_ver_t g_version = VERSION_5;
static print_log_fp g_log_cb = NULL;
static lhdc_channel_t g_channel_type = LHDC_OUTPUT_STEREO;

static uint8_t *g_mem_ptr = NULL;
static uint32_t g_mem_size = 0;

lhdc_ver_t lhdc_ver = VERSION_5;

// 正弦波生成相关变量
static float g_phase = 0.0f;           // 相位累加器
static float g_frequency = 440.0f;     // 生成250Hz的正弦波（440.0为A4标准音）

// ------------------------ Utility Functions ------------------------

static void log_info(const char *msg)
{
    if (g_log_cb) {
        g_log_cb((char *)msg);
    } else {
        LOG_INFO("%s", msg);
    }
}

// 生成正弦波样本
static int32_t generate_sine_sample(uint32_t sample_rate, uint32_t bit_depth) {
    // 计算相位增量
    float phase_inc = 2 * M_PI * g_frequency / sample_rate;
    
    // 计算正弦值并缩放到位深范围
    const float VOLUME_ATTENUATION = 0.005f; // 衰减系数，0.1=10%幅度，0.5=50%幅度，实测1%就挺响了
    float sample = sinf(g_phase) * VOLUME_ATTENUATION;
    
    // 更新相位，保持在0到2π之间
    g_phase += phase_inc;
    while (g_phase >= 2 * M_PI) {
        g_phase -= 2 * M_PI;
    }
    
    // 根据位深计算最大值
    int32_t max_val;
    switch (bit_depth) {
        case 16:
            max_val = 32767;  // 2^15 - 1
            break;
        case 24:
            max_val = 8388607; // 2^23 - 1
            break;
        case 32:
            max_val = 2147483647; // 2^31 - 1
            break;
        default:
            max_val = 32767; // 默认16位
            break;
    }
    
    return (int32_t)(sample * max_val);
}

// ------------------------ API Implementation ------------------------

int32_t lhdcv5_util_init_decoder(uint32_t *ptr, uint32_t bitPerSample, uint32_t sampleRate, uint32_t scaleTo16Bits, uint32_t is_lossless_enable, lhdc_ver_t version)
{
    if (decoder_initialized) {
        lhdcv5_util_dec_destroy();
    }

    if (ptr == NULL) {
        LOG_ERROR("%s: Decoder init error (null memory ptr)", __func__);
        return LHDCV5_UTIL_DEC_ERROR_PARAM;
    }

    decoder_initialized = 1;
    g_bitPerSample = bitPerSample; // 存储原始输入位深
    g_output_bit_per_sample = 32; // 设置输出位深为32位以统一处理
    g_sampleRate = sampleRate;
    g_scaleTo16Bits = scaleTo16Bits;
    g_is_lossless_enable = is_lossless_enable;
    g_version = version;
    g_mem_ptr = (uint8_t *)ptr;
    
    // 初始化相位
    g_phase = 0.0f;
    
    // 根据采样率调整频率，确保不同采样率下音调一致
    if (sampleRate == 44100) {
        g_frequency = 440.0f;
    }else if (sampleRate == 48000) {
        g_frequency = 440.0f * 48000.0f / 44100.0f;
    } else if (sampleRate == 96000) {
        g_frequency = 440.0f * 96000.0f / 44100.0f;
    } else if (sampleRate == 192000) {
        g_frequency = 440.0f * 192000.0f / 44100.0f;
    } else {
        g_frequency = 440.0f; // 默认44100Hz
    }

    LOG_INFO("%s: Decoder initialized with sine wave - Sample rate: %ldHz, Bit depth: %ldbit, Frequency: %.1fHz", 
             __func__, sampleRate, bitPerSample, g_frequency);
    return LHDCV5_UTIL_DEC_SUCCESS;
}

int32_t lhdcv5_util_dec_process(uint8_t * pOutBuf, uint8_t * pInput, uint32_t InLen, uint32_t *OutLen)
{
    if (!decoder_initialized)
        return LHDCV5_UTIL_DEC_ERROR_NO_INIT;
    if (!pInput || !pOutBuf || !OutLen)
        return LHDCV5_UTIL_DEC_ERROR_PARAM;

    // 计算样本大小和帧样本数 - 使用输出位深
    uint32_t sample_bytes = (g_output_bit_per_sample + 7) / 8;
    uint32_t frame_samples = 256;  // LHDC V5典型每声道样本数
    uint32_t channels = (g_channel_type == LHDC_OUTPUT_STEREO) ? 2 : 1;
    *OutLen = frame_samples * sample_bytes * channels;

    // 生成32位正弦波PCM数据，然后根据需要转换
    int32_t *out_32 = (int32_t *)pOutBuf;
    int16_t *out_16 = (int16_t *)pOutBuf;
    uint8_t *out_24 = pOutBuf;  // 24位特殊处理，存储为3字节

    for (uint32_t i = 0; i < frame_samples; i++) {
        // 生成左声道样本（始终生成32位样本）
        int32_t sample_32 = generate_sine_sample(g_sampleRate, 32);
        
        // 根据输出位深转换
        switch (g_output_bit_per_sample) {
            case 16:
                // 从32位转换到16位
                int16_t sample_16 = (int16_t)(sample_32 >> 16);
                out_16[i * 2] = sample_16;  // 左声道
                if (channels == 2) {
                    // 右声道使用稍作修改的样本，制造立体声效果
                    out_16[i * 2 + 1] = (int16_t)(sample_16 * 0.9f); // 右声道
                }
                break;
                
            case 24:
                // 从32位转换到24位
                int32_t sample_24 = sample_32 >> 8; // 右移8位得到24位

                // 正确处理有符号24位样本
                int32_t clamped_sample = sample_24;
                // 确保样本在24位范围内
                if (clamped_sample > 8388607) clamped_sample = 8388607;
                if (clamped_sample < -8388608) clamped_sample = -8388608;
                
                // 转换为无符号偏移量（+8388608）以便正确存储符号位
                uint32_t u_sample = clamped_sample + 8388608;
                
                out_24[i * 6] = u_sample & 0xFF;         // 低8位
                out_24[i * 6 + 1] = (u_sample >> 8) & 0xFF;  // 中8位
                out_24[i * 6 + 2] = (u_sample >> 16) & 0xFF; // 高8位
                
                if (channels == 2) {
                    int32_t right_sample = (int32_t)(sample_24 * 0.9f);
                    int32_t clamped_right = right_sample;
                    if (clamped_right > 8388607) clamped_right = 8388607;
                    if (clamped_right < -8388608) clamped_right = -8388608;
                    
                    uint32_t u_right = clamped_right + 8388608;
                    out_24[i * 6 + 3] = u_right & 0xFF;
                    out_24[i * 6 + 4] = (u_right >> 8) & 0xFF;
                    out_24[i * 6 + 5] = (u_right >> 16) & 0xFF;
                }
                break;
                
            case 32:
                out_32[i * 2] = sample_32;  // 左声道
                if (channels == 2) {
                    out_32[i * 2 + 1] = (int32_t)(sample_32 * 0.9f);  // 右声道
                }
                break;
        }
    }

    LOG_DEBUG("%s: Generated sine wave frame - %u samples, %u bits, %u channels, %u bytes", 
             __func__, frame_samples, g_bitPerSample, channels, *OutLen);
    return LHDCV5_UTIL_DEC_SUCCESS;
}

char *lhdcv5_util_dec_get_version()
{
    if (lhdc_ver == VERSION_5)
        return (char *)"LHDC V5";
    else
        return (char *)"LHDC";
}

int32_t lhdcv5_util_dec_destroy()
{
    if (!decoder_initialized)
        return LHDCV5_UTIL_DEC_ERROR_NO_INIT;
    decoder_initialized = 0;
    g_mem_ptr = NULL;
    g_mem_size = 0;
    LOG_INFO("%s: Decoder destroyed", __func__);
    return LHDCV5_UTIL_DEC_SUCCESS;
}

void lhdcv5_util_dec_register_log_cb(print_log_fp cb)
{
    g_log_cb = cb;
    LOG_INFO("%s: Log callback registered", __func__);
}

int32_t lhdcv5_util_dec_get_sample_size(uint32_t *frame_samples)
{
    if (!decoder_initialized)
        return LHDCV5_UTIL_DEC_ERROR_NO_INIT;
    if (!frame_samples)
        return LHDCV5_UTIL_DEC_ERROR_PARAM;

    // LHDC V5: 256 samples per frame per channel
    *frame_samples = 256;
    return LHDCV5_UTIL_DEC_SUCCESS;
}

int32_t lhdcv5_util_dec_fetch_frame_info(uint8_t *frameData, uint32_t frameDataLen, lhdc_frame_Info_t *frameInfo)
{
    if (!decoder_initialized)
        return LHDCV5_UTIL_DEC_ERROR_NO_INIT;
    if (!frameData || !frameInfo || frameDataLen == 0)
        return LHDCV5_UTIL_DEC_ERROR_PARAM;
    
    // 计算实际需要的帧长度，避免输入不足错误
    uint32_t sample_bytes = (g_bitPerSample + 7) / 8;
    frameInfo->frame_len = 256 * sample_bytes * 2;  // 立体声
    frameInfo->isSplit = (frameDataLen > frameInfo->frame_len) ? 1 : 0;
    frameInfo->isLeft = 0;
    
    // 确保返回的帧长度不会导致输入不足错误
    if (frameInfo->frame_len > frameDataLen) {
        frameInfo->frame_len = frameDataLen;
    }
    
    return LHDCV5_UTIL_DEC_SUCCESS;
}

int32_t lhdcv5_util_dec_channel_selsect(lhdc_channel_t channel_type)
{
    if (!decoder_initialized)
        return LHDCV5_UTIL_DEC_ERROR_NO_INIT;
    g_channel_type = channel_type;
    switch(channel_type){
        case LHDC_OUTPUT_STEREO:
            LOG_INFO("%s: Channel selected (stereo)", __func__);
            break;
        case LHDC_OUTPUT_LEFT_CAHNNEL:
            LOG_INFO("%s: Channel selected (left)", __func__);
            break;
        case LHDC_OUTPUT_RIGHT_CAHNNEL:
            LOG_INFO("%s: Channel selected (right)", __func__);
            break;
        default:
            LOG_INFO("%s: Channel select unknown", __func__);
            break;
    }
    return LHDCV5_UTIL_DEC_SUCCESS;
}

int32_t lhdcv5_util_dec_get_mem_req(lhdc_ver_t version, uint32_t *mem_req_bytes)
{
    if (!mem_req_bytes)
        return LHDCV5_UTIL_DEC_ERROR_PARAM;
    switch (version) {
        case VERSION_5:
            *mem_req_bytes = 4096; // 典型V5解码器所需内存
            break;
        default:
            *mem_req_bytes = 0;
            return LHDCV5_UTIL_DEC_ERROR_PARAM;
    }
    return LHDCV5_UTIL_DEC_SUCCESS;
}

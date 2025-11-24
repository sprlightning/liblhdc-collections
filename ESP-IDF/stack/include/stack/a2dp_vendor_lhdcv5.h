/*
 * SPDX-FileCopyrightText: 2025 The Android Open Source Project
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * a2dp_vendor_lhdcv5.h
 */

//
// A2DP Codec API for LHDCV5
//

#ifndef A2DP_VENDOR_LHDCV5_H
#define A2DP_VENDOR_LHDCV5_H

#include "a2d_api.h"
#include "a2dp_codec_api.h"
#include "stack/a2dp_vendor_lhdc_constants.h"
#include "stack/a2dp_vendor_lhdcv5_constants.h"
#include "a2dp_shim.h"
#include "avdt_api.h"
#include "bt_av.h"

/* lossless raw mode support */
//#define LHDC_LOSSLESS_RAW_SUPPORT

#define IS_SRC  (true)
#define IS_SNK  (false)

/*****************************************************************************
**  Type Definitions - 严格按照LHDC的CIE顺序排列，这里直接使用Android的即可
*****************************************************************************/
// data type for the LHDC Codec Information Element
typedef struct {
  uint32_t vendorId;                                    /* Vendor ID */
  uint16_t codecId;                                     /* Codec ID */
  uint8_t sampleRate;                                   /* Sampling Frequency Type */
  uint8_t bitsPerSample;                                /* Bits Per Sample Type */
  uint8_t channelMode;                                  /* Channel Mode */
  uint8_t version;                                      /* Codec SubVersion Number */
  uint8_t frameLenType;                                 /* Frame Length Type */
  uint8_t maxTargetBitrate;                             /* Max Target Bit Rate Type */
  uint8_t minTargetBitrate;                             /* Min Target Bit Rate Type */
  bool hasFeatureAR;                                    /* FeatureSupported: AR */
  bool hasFeatureJAS;                                   /* FeatureSupported: JAS */
  bool hasFeatureMETA;                                  /* FeatureSupported: META */
  bool hasFeatureLL;                                    /* FeatureSupported: Low Latency */
  bool hasFeatureLLESS48K;                              /* FeatureSupported: Lossless enable/disable (standard 48 KHz) */
  bool hasFeatureLLESS24Bit;                            /* Lossless extended configurable: 24 bit-per-sample */
  bool hasFeatureLLESS96K;                              /* Lossless extended configurable: 96 KHz */
#ifdef LHDC_LOSSLESS_RAW_SUPPORT
  bool hasFeatureLLESSRaw;                              /* FeatureSupported: Lossless Raw mode (standard 48 KHz) */
#endif
} tA2DP_LHDCV5_CIE;

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
**
** Function         A2DP_ParseInfoLhdcV5
**
** Description      This function is called by an application to parse
**                  the Media Codec Capabilities byte sequence
**                  beginning from the LOSC octet.
**                  Input Parameters:
**                      p_codec_info:  the byte sequence to parse.
**
**                      is_capability:  TRUE, if the byte sequence is for get capabilities response.
**
**                  Output Parameters:
**                      p_ie:  The Codec Information Element information.
**
** Returns          A2D_SUCCESS if function execution succeeded.
**                  Error status code, otherwise.
******************************************************************************/
tA2D_STATUS A2DP_ParseInfoLhdcV5(tA2DP_LHDCV5_CIE* p_ie,const uint8_t* p_codec_info,bool is_capability, bool is_source);

/******************************************************************************
**
** Function         A2DP_BuildInfoLhdcv5
**
** Description      Builds the LHDCV5 Media Codec Capabilities byte sequence beginning from the
**                  LOSC octet.
**                  
**                      media_type:  the media type |AVDT_MEDIA_TYPE_*|.
**
**                      p_ie:  is a pointer to the LHDCV5 Codec Information Element information.
**
**                  Output Parameters:
**                      p_result:  Codec capabilities.
**
** Returns          A2D_SUCCESS on success, otherwise the corresponding A2DP error status code.
**
******************************************************************************/
static tA2D_STATUS A2DP_BuildInfoLhdcV5(uint8_t media_type, const tA2DP_LHDCV5_CIE* p_ie, uint8_t* p_result);

/******************************************************************************
**
** Function         A2DP_IsVendorPeerSourceCodecValidLhdcV5
**
** Description      Checks whether the codec capabilities contain a valid peer A2DP LHDCV5 Source
**                  codec.
**                  NOTE: only codecs that are implemented are considered valid.
**
**                      p_codec_info:  contains information about the codec capabilities of the
**                                     peer device.
**
** Returns          true if |p_codec_info| contains information about a valid LHDCV5 codec,
**                  otherwise false.
**
******************************************************************************/
tA2D_STATUS A2DP_IsVendorPeerSourceCodecValidLhdcV5(const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_IsVendorPeerSinkCodecValidLhdcV5
**
** Description      Checks whether the codec capabilities contain a valid peer A2DP LHDCV5 Sink
**                  codec.
**                  NOTE: only codecs that are implemented are considered valid.
**
**                      p_codec_info:  contains information about the codec capabilities of the
**                                     peer device.
**
** Returns          true if |p_codec_info| contains information about a valid LHDCV5 codec,
**                  otherwise false.
**
******************************************************************************/
bool A2DP_IsVendorPeerSinkCodecValidLhdcV5(const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_IsVendorSinkCodecSupportedLhdcV5
**
** Description      Checks whether A2DP LHDCV5 codec configuration matches with a device's codec
**                  capabilities. |p_cap| is the LHDCV5 codec configuration. |p_codec_info| is
**                  the device's codec capabilities.
**
**                      p_cap:  is the LHDCV5 codec configuration.
**                  
**                      p_codec_info:  the device's codec capabilities
**
**                      is_peer_codec_info:  true, the byte sequence is codec capabilities,
**                                           otherwise is codec configuration.
**
**                  Output Parameters:
**                      p_result:  
**
** Returns          A2D_SUCCESS if the codec configuration matches with capabilities,
**                  otherwise the corresponding A2DP error status code.
**
******************************************************************************/
static tA2DP_STATUS A2DP_IsVendorSinkCodecSupportedLhdcV5(const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_IsPeerSourceCodecSupportedLhdcV5
**
** Description      Checks whether A2DP LHDCV5 codec configuration matches with a device's codec
**                  capabilities. |p_cap| is the LHDCV5 codec configuration. |p_codec_info| is
**                  the device's codec capabilities.
**
**                      p_cap:  is the LHDCV5 codec configuration.
**                  
**                      p_codec_info:  the device's codec capabilities
**
**                      is_peer_codec_info:  true, the byte sequence is codec capabilities,
**                                           otherwise is codec configuration.
**
**                  Output Parameters:
**                      p_result:  
**
** Returns          A2D_SUCCESS if the codec configuration matches with capabilities,
**                  otherwise the corresponding A2DP error status code.
**
******************************************************************************/
static bool A2DP_IsPeerSourceCodecSupportedLhdcV5(const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_CodecInfoMatchesCapabilityLhdcV5
**
** Description      Checks whether A2DP LHDCV5 codec configuration matches with a device's codec
**                  capabilities. |p_cap| is the LHDCV5 codec configuration. |p_codec_info| is
**                  the device's codec capabilities.
**
**                      p_cap:  is the LHDCV5 codec configuration.
**                  
**                      p_codec_info:  the device's codec capabilities
**
**                      is_peer_codec_info:  true, the byte sequence is codec capabilities,
**                                           otherwise is codec configuration.
**
**                  Output Parameters:
**                      p_result:  
**
** Returns          A2D_SUCCESS if the codec configuration matches with capabilities,
**                  otherwise the corresponding A2DP error status code.
**
******************************************************************************/
static tA2D_STATUS A2DP_CodecInfoMatchesCapabilityLhdcV5(
    const tA2DP_LHDCV5_CIE* p_cap, const uint8_t* p_codec_info,
    bool is_peer_codec_info);

/******************************************************************************
**
** Function         A2DP_VendorSinkCodecIndexLhdcV5
**
** Description      Gets the A2DP LHDCV5 Sink codec index for a given |p_codec_info|.
**
**                      p_codec_info:  Contains information about the codec capabilities.
**
** Returns          the corresponding |btav_a2dp_codec_index_t| on success,
**                  otherwise |BTAV_A2DP_CODEC_INDEX_MAX|.
**
******************************************************************************/
btav_a2dp_codec_index_t A2DP_VendorSinkCodecIndexLhdcV5(
    const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_VendorSourceCodecIndexLhdcV5
**
** Description      Gets the A2DP LHDCV5 Source codec index for a given |p_codec_info|.
**
**                      p_codec_info:  Contains information about the codec capabilities.
**
** Returns          the corresponding |btav_a2dp_codec_index_t| on success,
**                  otherwise |BTAV_A2DP_CODEC_INDEX_MAX|.
**
******************************************************************************/
btav_a2dp_codec_index_t A2DP_VendorSourceCodecIndexLhdcV5(
    const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_VendorCodecNameLhdcv5
**
** Description      Gets the A2DP LHDCV5 codec name for a given |p_codec_info|.
**
**                      p_codec_info:  Contains information about the codec capabilities.
**
** Returns          Codec name.
**
******************************************************************************/
const char* A2DP_VendorCodecNameLhdcV5(const uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_VendorCodecTypeEqualsLhdcv5
**
** Description      Checks whether two A2DP LHDCV5 codecs |p_codec_info_a| and |p_codec_info_b|
**                  have the same type.
**
**                      p_codec_info_a:  Contains information about the codec capabilities.
**
**                      p_codec_info_b:  Contains information about the codec capabilities.
**
** Returns          true if the two codecs have the same type, otherwise false.
**
******************************************************************************/
bool A2DP_VendorCodecTypeEqualsLhdcV5(const uint8_t* p_codec_info_a,
                                    const uint8_t* p_codec_info_b);

/******************************************************************************
**
** Function         A2DP_VendorInitCodecConfigLhdcV5
**
** Description      Initializes A2DP codec-specific information into |p_result|.
**                  The selected codec is defined by |codec_index|.
**
**                      codec_index:  Selected codec index.
**
**                  Output Parameters:
**                      p_result:  The resulting codec information element.
**
** Returns          true on success, otherwise false.
**
******************************************************************************/
bool A2DP_VendorInitCodecConfigLhdcV5(btav_a2dp_codec_index_t codec_index, UINT8 *p_result);

/******************************************************************************
**
** Function         A2DP_VendorInitCodecConfigLhdcv5Sink
**
** Description      Initializes A2DP codec-specific information into |p_result|.
**                  The selected codec is defined by |codec_index|.
**
**                      p_codec_info:  Contains information about the codec capabilities.
**
** Returns          true on success, otherwise false.
**
******************************************************************************/
static bool A2DP_VendorInitCodecConfigLhdcV5Sink(uint8_t* p_codec_info);

/******************************************************************************
**
** Function         A2DP_VendorBuildCodecConfigLhdcV5
**
** Description      Initializes A2DP codec-specific information into |p_result|.
**                  The selected codec is defined by |codec_index|.
**
**                      p_codec_info:  Contains information about the codec capabilities.
**
** Returns          true on success, otherwise false.
**
******************************************************************************/
bool A2DP_VendorBuildCodecConfigLhdcV5(UINT8 *p_src_cap, UINT8 *p_result);

/******************************************************************************
**
** Function         A2DP_GetVendorDecoderInterfaceLhdcV5
**
** Description      Gets the A2DP LHDCV5 decoder interface that can be used to decode received A2DP
**                  packets
**
**                      p_codec_info:  contains the codec information.
**
** Returns          the A2DP LHDCV5 decoder interface if the |p_codec_info| is valid and
**                  supported, otherwise NULL.
**
******************************************************************************/
const tA2DP_DECODER_INTERFACE* A2DP_GetVendorDecoderInterfaceLhdcV5(
    const uint8_t* p_codec_info);

#ifdef __cplusplus
}
#endif

#endif /* A2DP_VENDOR_LHDCV5_H */

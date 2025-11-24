/**
 * SPDX-FileCopyrightText: 2025 The Android Open Source Project
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * a2dp_vendor_lhdcv5_decoder.h
 */

//
// Interface to the A2DP LHDCV5 Decoder
//

#ifndef A2DP_VENDOR_LHDCV5_DECODER_H
#define A2DP_VENDOR_LHDCV5_DECODER_H

#include "a2dp_codec_api.h"
#include "stack/a2dp_decoder.h"
#include "stack/bt_types.h"
#include "lhdcv5BT_dec.h"

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_init
**
** Description      Initialize the A2DP LHDCV5 decoder
**
**                      decode_callback:  Callback function after decode completion
**
** Returns          true on success, false otherwise
**
******************************************************************************/
bool a2dp_lhdcv5_decoder_init(decoded_data_callback_t decode_callback);

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_cleanup
**
** Description      Cleanup the A2DP LHDCV5 decoder |need lhdcv5BT_dec.h
**
** Returns          None
**
******************************************************************************/
void a2dp_lhdcv5_decoder_cleanup();

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_decode_packet_header
**
** Description      Decode the LHDCV5 packet header.
**
** Returns          
**
******************************************************************************/
ssize_t a2dp_lhdcv5_decoder_decode_packet_header(BT_HDR* p_data);

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_decode_packet
**
** Description      Decodes |p_buf|. Calls |decode_callback| passed into |a2dp_lhdcv5_decoder_init|
**                  if decoded frames are available.
**
**                      p_buf:  Packet data
**
** Returns          true on success, false otherwise
**
******************************************************************************/
bool a2dp_lhdcv5_decoder_decode_packet(BT_HDR* p_buf, unsigned char* buf, size_t buf_len);

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_start
**
** Description      Start the A2DP LHDCV5 decoder.
**
******************************************************************************/
void a2dp_lhdcv5_decoder_start();

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_suspend
**
** Description      Suspend the A2DP LHDCV5 decoder.
**
******************************************************************************/
void a2dp_lhdcv5_decoder_suspend();

/******************************************************************************
**
** Function         a2dp_lhdcv5_decoder_configure
**
** Description      Configure the A2DP LHDCV5 decoder.
**
******************************************************************************/
void a2dp_lhdcv5_decoder_configure(const uint8_t* p_codec_info);

// const tA2DP_DECODER_INTERFACE* A2DP_LHDCV5_DecoderInterface();

#ifdef __cplusplus
}
#endif

#endif /* A2DP_VENDOR_LHDCV5_DECODER_H */
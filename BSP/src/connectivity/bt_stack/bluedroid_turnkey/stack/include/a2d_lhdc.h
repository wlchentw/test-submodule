/******************************************************************************
 *
 *  Copyright (C) 2010 MediaTek Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This is the interface to utility functions for dealing with LHDC data
 *  frames and codec capabilities.
 *
 ******************************************************************************/
#ifndef A2D_LHDC_H
#define A2D_LHDC_H
#include "utl.h"
#include "mdroid_buildcfg.h"
#include "a2d_api.h"
#include "bt_types.h"
#include "avdt_api.h"

/*****************************************************************************
**  macros
*****************************************************************************/



/*****************************************************************************
**  constants
*****************************************************************************/

/* the LOSC of LHDC media codec capabilitiy */
#define BTA_AV_CO_LHDC_CODEC_LEN (11) // media_type (1 byte) + codec_type (1 byte)  + element (9 byte)

// [Octet 0-3] Vendor ID
#define A2DP_LHDC_VENDOR_ID 0x0000053A
// [Octet 4-5] Vendor Specific Codec ID
#define A2DP_LHDC_CODEC_ID 0x4C32
#define A2DP_LHDC_LL_CODEC_ID 0x4C4C


#define LHDC_SAMPLE_FREQ_MSK (0x0F)  /* sampling frequency */
#define LHDC_SAMPLE_FREQ_96k (0x01)
#define LHDC_SAMPLE_FREQ_88k (0x02)
#define LHDC_SAMPLE_FREQ_48k (0x04)
#define LHDC_SAMPLE_FREQ_44k (0x08)

#define LHDC_CHANNEL_MSK     (0xF0)    /* channels */
#define LHDC_CHANNEL_DEFAULT (0x00)
#define LHDC_CHANNEL_MONO    (0x10)
#define LHDC_CHANNEL_STEREO  (0x20)
#define LHDC_CHANNEL_CH5_1   (0x40)
#define LHDC_CHANNEL_REV     (0x80)

#define LHDC_BITDEPTH_MASK (0x30)
#define LHDC_BITDEPTH_16 (0x20)
#define LHDC_BITDEPTH_24 (0x10)

#define LHDC_MAX_RATE_MASK (0xF0)
#define LHDC_MAX_RATE_900  (0x00)
#define LHDC_MAX_RATE_560  (0x10)
#define LHDC_MAX_RATE_400  (0x20)
#define LHDC_MAX_RATE_LOW  (0x30)
#define LHDC_MAX_RATE_TEST (0x40)

#define LHDC_OBJ_TYPE_MASK (0x0F)
#define LHDC_OBJ_TYPE_1_0  (0x00)
#define LHDC_OBJ_TYPE_2_0  (0x01)

#define LHDC_COMPRESSOR_OUTPUT_FMT_MASK      (0x0F)
#define LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE   (0x01)
#define LHDC_COMPRESSOR_OUTPUT_FMT_SPLIT     (0x02)
#define LHDC_COMPRESSOR_OUTPUT_FMT_PRE_SPLIT (0x04)

#define LHDC_LATENCY_LOW  (0)
#define LHDC_LATENCY_MID  (1)
#define LHDC_LATENCY_HIGH (2)

typedef struct
{
    UINT32 vendorId;
    UINT16 codecId;    /* Codec ID for LDAC */
    UINT8 samp_freq;
    UINT8 bit_depth;
    UINT8 obj_type;
    UINT8 max_rate;
    UINT8 compressor_output_fmt;
    UINT8 channels;
} tA2D_LHDC_CIE;


BOOLEAN A2D_VendorInitCodecConfigLhdc(UINT8 *p_codec_info);
UINT8 A2D_BldLHDCInfo(UINT8 media_type, tA2D_LHDC_CIE *p_ie, UINT8 *p_result);
UINT8 A2D_ParsLHDCInfo(tA2D_LHDC_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps);
UINT8 bta_av_lhdc_cfg_matches_cap(UINT8 *p_cfg);
UINT8 bta_av_lhdc_cfg_in_cap(UINT8 *p_cfg);
BOOLEAN A2D_VendorGetCodecConfigLhdc(UINT8 *p_cfg_info, UINT8 *p_codec_cfg);
BOOLEAN A2D_VendorDecoderInitLhdc(UINT32 bitdpth, UINT32 scal_to_16b, decoded_data_callback_t decode_callback);
BOOLEAN A2D_VendorDecoderDestroyLhdc(void);
tA2D_STATUS A2D_BuildSrc2SinkConfigLhdc(UINT8 *p_pref_cfg, UINT8 *p_src_cap);
BOOLEAN A2D_VendorDecoderProcessLhdc(BT_HDR* p_buf);
void A2D_VendorUnloadEncoderLhdc(void);

#endif /* A2D_LHDC_H */


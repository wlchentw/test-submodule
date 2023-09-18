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

#include <math.h>
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "a2d_api.h"
#include "a2d_lhdc.h"
#include "bta_av_api.h"
#include <utils/Log.h>
#include "utl.h"

#define A2D_LHDC_DIFF_TM_MS(a, b) \
    (((a).tv_sec-(b).tv_sec) * 1000L+((a).tv_usec-(b).tv_usec) /1000)

#define A2D_LHDC_DIFF_TM_US(a, b) \
    (((a).tv_sec-(b).tv_sec) * 1000000L+((a).tv_usec-(b).tv_usec))

#define A2D_LHDC_DBG_STAT

typedef struct bt_get_local_info_t{
    uint8_t bt_addr[BD_ADDR_LEN];
    char *bt_name;
    uint8_t bt_len;
}bt_get_local_info;


typedef int (*tLHDC_GET_BT_INFO)(bt_get_local_info * bt_info);

static const char* LHDC_DECODER_LIB_NAME = "liblhdc-dec.so";
static void* lhdc_decoder_lib_handle = NULL;

static const char* LHDC_DECODER_INIT_NAME = "lhdcInit";
typedef int (*tLHDC_DECODER_INIT)(UINT32 bitPerSample, UINT32 scaleTo16Bits);

static const char* LHDC_DECODER_SET_LICENSE_KEY_TABLE_NAME = "lhdcSetLicenseKeyTable";
typedef int (*tLHDC_DECODER_SET_LICENSE_KEY_TABLE)(const UINT8 *licTable, tLHDC_GET_BT_INFO pFunc);

static const char* LHDC_DECODER_PUT_FRAME_NAME = "lhdcPutFrame";
typedef int (*tLHDC_DECODER_PUT_FRAME)(UINT8 *pIn, INT32 len);

static const char* LHDC_DECODER_DECODE_PROCESS_NAME = "lhdcDecodeProcess";
typedef int (*tLHDC_DECODER_DECODE_PROCESS)(UINT8 *pOutBuf);

static const char* LHDC_DECODER_GET_VERSION_CODE_NAME = "getVersionCode";
typedef int (*tLHDC_DECODER_GET_VERSION_CODE)(void);

static const char* LHDC_DECODER_DESTROY_NAME = "lhdcDestroy";
typedef int (*tLHDC_DECODER_DESTROY)(void);

static const char* LHDC_DECODER_TEST_KEY_BIN_NAME = "testkey_bin";

static tLHDC_DECODER_INIT lhdc_decoder_init_func = NULL;
static tLHDC_DECODER_SET_LICENSE_KEY_TABLE lhdc_decoder_set_license_key_table_func = NULL;
static tLHDC_DECODER_PUT_FRAME lhdc_decoder_put_frame_func = NULL;
static tLHDC_DECODER_DECODE_PROCESS lhdc_decoder_decode_process_func = NULL;
static tLHDC_DECODER_GET_VERSION_CODE lhdc_decoder_get_ver_code_func = NULL;
static tLHDC_DECODER_DESTROY lhdc_decoder_destroy_func = NULL;
static UINT8 *lhdc_decoder_test_key_bin = NULL;

static decoded_data_callback_t lhdc_decode_data_callback = NULL;

static INT32 lhdc_decode_inited = 0;

/* LHDC Source codec capabilities */
static tA2D_LHDC_CIE a2dp_lhdc_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDC_CODEC_ID,   // codecId

    (LHDC_SAMPLE_FREQ_44k | LHDC_SAMPLE_FREQ_48k | LHDC_SAMPLE_FREQ_96k),

    (LHDC_BITDEPTH_16 | LHDC_BITDEPTH_24),

    LHDC_OBJ_TYPE_2_0, /* LHDC 2.0 */

    LHDC_MAX_RATE_900,

    LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE,

    LHDC_CHANNEL_DEFAULT,
};

/* Default LHDC codec configuration */
static tA2D_LHDC_CIE a2dp_lhdc_default_config = {
    A2DP_LHDC_VENDOR_ID,                // vendorId
    A2DP_LHDC_CODEC_ID,                 // codecId
    LHDC_SAMPLE_FREQ_96k,               // sampleRate
    LHDC_BITDEPTH_24,
    LHDC_OBJ_TYPE_2_0,                  //obj_type
    LHDC_MAX_RATE_900,
    LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE,
    LHDC_CHANNEL_STEREO,
};


/* LHDC Source codec capabilities */
static tA2D_LHDC_CIE a2dp_lhdc_ll_caps = {
    A2DP_LHDC_VENDOR_ID,  // vendorId
    A2DP_LHDC_LL_CODEC_ID,   // codecId

    (LHDC_SAMPLE_FREQ_44k | LHDC_SAMPLE_FREQ_48k),

    (LHDC_BITDEPTH_16 | LHDC_BITDEPTH_24),

    LHDC_OBJ_TYPE_2_0, /* LHDC 2.0 */

    LHDC_MAX_RATE_400,

    LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE,

    LHDC_CHANNEL_DEFAULT,
};

/* Default LHDC codec configuration */
static tA2D_LHDC_CIE a2dp_lhdc_ll_default_config = {
    A2DP_LHDC_VENDOR_ID,                // vendorId
    A2DP_LHDC_LL_CODEC_ID,                 // codecId
    LHDC_SAMPLE_FREQ_48k,               // sampleRate
    LHDC_BITDEPTH_24,
    LHDC_OBJ_TYPE_2_0,                  //obj_type
    LHDC_MAX_RATE_400,
    LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE,
    LHDC_CHANNEL_STEREO,
};


/******************************************************************************
**
** Function         A2D_Bld_LHDCInfo
**
******************************************************************************/
UINT8 A2D_BldLHDCInfo(UINT8 media_type, tA2D_LHDC_CIE *p_ie, UINT8 *p_result)
{
  if (p_ie == NULL || p_result == NULL) {
    return A2D_INVALID_PARAMS;
  }

  *p_result++ = BTA_AV_CO_LHDC_CODEC_LEN;
  *p_result++ = (media_type << 4);
  *p_result++ = A2D_MEDIA_CT_NON;

  // Vendor ID and Codec ID
  *p_result++ = (uint8_t)(p_ie->vendorId & 0x000000FF);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x0000FF00) >> 8);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0x00FF0000) >> 16);
  *p_result++ = (uint8_t)((p_ie->vendorId & 0xFF000000) >> 24);
  *p_result++ = (uint8_t)(p_ie->codecId & 0x00FF);
  *p_result++ = (uint8_t)((p_ie->codecId & 0xFF00) >> 8);

  // Sampling Frequency
  *p_result = (p_ie->samp_freq & LHDC_SAMPLE_FREQ_MSK)
                | (p_ie->bit_depth & LHDC_BITDEPTH_MASK);
  p_result++;

  *p_result = (p_ie->max_rate & LHDC_MAX_RATE_MASK)
                | (p_ie->obj_type & LHDC_OBJ_TYPE_MASK);
  p_result++;

  *p_result = (p_ie->channels & LHDC_CHANNEL_MSK)
                | (p_ie->compressor_output_fmt & LHDC_COMPRESSOR_OUTPUT_FMT_MASK);


  return A2D_SUCCESS;
}

/******************************************************************************
**
** Function         A2D_ParsLHDCInfo
**
******************************************************************************/

tA2D_STATUS A2D_ParsLHDCInfo(tA2D_LHDC_CIE *p_ie, UINT8 *p_info, BOOLEAN for_caps)
{
    tA2D_STATUS status;
    UINT8   losc;
    UINT8   mt;

    if (p_ie == NULL || p_info == NULL)
    {
        status = A2D_INVALID_PARAMS;
    }
    else
    {
        losc    = *p_info++;
        mt      = *p_info++;
        APPL_TRACE_DEBUG("losc %d, mt %02x", losc, mt);
        /* If the function is called for the wrong Media Type or Media Codec Type */
        if (losc != BTA_AV_CO_LHDC_CODEC_LEN || *p_info != A2D_MEDIA_CT_NON)
        {
            APPL_TRACE_ERROR("A2D_ParsAptxInfo: wrong media type %02x", *p_info);
            status = A2D_WRONG_CODEC;
        }
        else
        {
            p_info++;
            p_ie->vendorId = (*p_info & 0x000000FF) |
                             (*(p_info + 1) << 8    & 0x0000FF00) |
                             (*(p_info + 2) << 16  & 0x00FF0000) |
                             (*(p_info + 3) << 24  & 0xFF000000);
            p_info = p_info + 4;
            p_ie->codecId = (*p_info & 0x00FF) | (*(p_info + 1) << 8 & 0xFF00);
            p_info = p_info + 2;
            p_ie->samp_freq = *p_info & 0x0F;
            p_ie->bit_depth = *p_info & 0x30;
            p_info = p_info + 1;
            p_ie->obj_type = *p_info & 0x0F;
            p_ie->max_rate = *p_info & 0x30;
            p_info = p_info + 1;
            p_ie->channels = *p_info & 0xF0;
            p_ie->compressor_output_fmt = *p_info & 0x0F;

            status = A2D_SUCCESS;

            if (for_caps == FALSE)
            {
                if (A2D_BitsSet(p_ie->samp_freq) != A2D_SET_ONE_BIT)
                    status = A2D_BAD_SAMP_FREQ;
            }
        }
    }
    return status;
}

UINT8 bta_av_lhdc_cfg_matches_cap(UINT8 *p_cfg)
{
    UINT8           status = 0;
    tA2D_LHDC_CIE    cfg_cie;
    tA2D_LHDC_CIE *p_cap = NULL;
    /* parse configuration */
    if ((status = A2D_ParsLHDCInfo(&cfg_cie, p_cfg, FALSE)) != 0)
    {
        APPL_TRACE_DEBUG("<LHDC> %s status=%d", __FUNCTION__, status);
        return status;
    }
    p_cap = &a2dp_lhdc_caps;

    APPL_TRACE_DEBUG(" object_type peer: 0%x, capability  0%x", cfg_cie.obj_type, p_cap->obj_type);
    APPL_TRACE_DEBUG(" samp_freq peer: 0%x, capability  0%x", cfg_cie.samp_freq, p_cap->samp_freq);
    APPL_TRACE_DEBUG(" channels peer: 0%x, capability  0%x", cfg_cie.channels, p_cap->channels);
    APPL_TRACE_DEBUG(" bit_depth peer: 0%x, capability  0%x", cfg_cie.bit_depth, p_cap->bit_depth);
    APPL_TRACE_DEBUG(" max_rate peer: 0%x, capability  0%x", cfg_cie.max_rate, p_cap->max_rate);
    APPL_TRACE_DEBUG(" compressor_output_fmt peer: 0%x, capability  0%x", cfg_cie.compressor_output_fmt, p_cap->compressor_output_fmt);

    if ((cfg_cie.samp_freq & p_cap->samp_freq) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    if (cfg_cie.channels != 0 && ((cfg_cie.channels & p_cap->channels) == 0))
    {
        status = A2D_INVALID_PARAMS;
    }
    if ((cfg_cie.bit_depth & p_cap->bit_depth) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    if ((cfg_cie.compressor_output_fmt & p_cap->compressor_output_fmt) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    return status;
}

/*******************************************************************************
**
** Function         bta_av_LHDC_cfg_in_cap
**
** Description      This function checks whether an LHDC codec configuration
**                  is allowable for the given codec capabilities.
**
** Returns          0 if ok, nonzero if error.
**
*******************************************************************************/
UINT8 bta_av_lhdc_cfg_in_cap(UINT8 *p_cfg)
{
    UINT8           status = A2D_SUCCESS;
    tA2D_LHDC_CIE    cfg_cie;
    tA2D_LHDC_CIE *p_cap = NULL;

    /* parse configuration */
    if ((status = A2D_ParsLHDCInfo(&cfg_cie, p_cfg, FALSE)) != 0)
    {
        APPL_TRACE_DEBUG("%s parse fail status=%d", __FUNCTION__, status);
        return status;
    }
    p_cap = &a2dp_lhdc_caps;
    if ((cfg_cie.samp_freq & p_cap->samp_freq) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    if ((cfg_cie.channels != 0) && ((cfg_cie.channels & p_cap->channels) == 0))
    {
        status = A2D_INVALID_PARAMS;
    }
    if ((cfg_cie.bit_depth & p_cap->bit_depth) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    if ((cfg_cie.compressor_output_fmt & p_cap->compressor_output_fmt) == 0)
    {
        status = A2D_INVALID_PARAMS;
    }
    return status;
}

BOOLEAN A2D_VendorLoadDecoderLhdc(void) {
  if (lhdc_decoder_lib_handle != NULL) return true;  // Already loaded

  // Open the decoder library
  lhdc_decoder_lib_handle = dlopen(LHDC_DECODER_LIB_NAME, RTLD_NOW);
  if (lhdc_decoder_lib_handle == NULL) {
    APPL_TRACE_ERROR("%s: cannot open lhdc decoder library %s: %s", __func__,
              LHDC_DECODER_LIB_NAME, dlerror());
    return false;
  }

  lhdc_decoder_init_func = (tLHDC_DECODER_INIT)dlsym(lhdc_decoder_lib_handle,
                                                     LHDC_DECODER_INIT_NAME);
  if (lhdc_decoder_init_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_INIT_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  lhdc_decoder_set_license_key_table_func = (tLHDC_DECODER_SET_LICENSE_KEY_TABLE)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_SET_LICENSE_KEY_TABLE_NAME);
  if (lhdc_decoder_set_license_key_table_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_SET_LICENSE_KEY_TABLE_NAME, dlerror());
    //A2D_VendorUnloadEncoderLhdc();
    //return false;
  }

  lhdc_decoder_put_frame_func = (tLHDC_DECODER_PUT_FRAME)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_PUT_FRAME_NAME);
  if (lhdc_decoder_put_frame_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_PUT_FRAME_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  lhdc_decoder_decode_process_func = (tLHDC_DECODER_DECODE_PROCESS)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_DECODE_PROCESS_NAME);
  if (lhdc_decoder_decode_process_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_DECODE_PROCESS_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  lhdc_decoder_get_ver_code_func = (tLHDC_DECODER_GET_VERSION_CODE)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_GET_VERSION_CODE_NAME);
  if (lhdc_decoder_get_ver_code_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_GET_VERSION_CODE_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  lhdc_decoder_destroy_func = (tLHDC_DECODER_DESTROY)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_DESTROY_NAME);
  if (lhdc_decoder_destroy_func == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find function '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_DESTROY_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  lhdc_decoder_test_key_bin = (UINT8*)dlsym(
      lhdc_decoder_lib_handle, LHDC_DECODER_TEST_KEY_BIN_NAME);
  if (lhdc_decoder_test_key_bin == NULL) {
    APPL_TRACE_ERROR(
              "%s: cannot find var '%s' in the decoder library: %s",
              __func__, LHDC_DECODER_TEST_KEY_BIN_NAME, dlerror());
    A2D_VendorUnloadEncoderLhdc();
    return false;
  }

  APPL_TRACE_DEBUG("%s: load lhdc decoder library ok", __func__);

  return true;
}

void A2D_VendorUnloadEncoderLhdc(void) {
  lhdc_decoder_init_func = NULL;
  lhdc_decoder_set_license_key_table_func = NULL;
  lhdc_decoder_put_frame_func = NULL;
  lhdc_decoder_decode_process_func = NULL;
  lhdc_decoder_get_ver_code_func = NULL;
  lhdc_decoder_destroy_func = NULL;
  lhdc_decoder_test_key_bin = NULL;

  if (lhdc_decoder_lib_handle != NULL) {
    dlclose(lhdc_decoder_lib_handle);
    lhdc_decoder_lib_handle = NULL;
  }
}

BOOLEAN A2D_VendorInitCodecConfigLhdc(UINT8 *p_codec_info) {
  if (A2D_VendorLoadDecoderLhdc() != TRUE)
  {
    return FALSE;
  }

  if (A2D_BldLHDCInfo(AVDT_MEDIA_AUDIO, &a2dp_lhdc_caps,
                         p_codec_info) != A2D_SUCCESS) {
    return false;
  }

  return true;
}

tA2D_STATUS A2D_BuildSrc2SinkConfigLhdc(UINT8 *p_pref_cfg, UINT8 *p_src_cap)
{
  UINT8 status = A2D_SUCCESS;
  tA2D_LHDC_CIE src_cap;
  tA2D_LHDC_CIE pref_cap;

  /* initialize it to default SBC configuration */
  A2D_BldLHDCInfo(AVDT_MEDIA_AUDIO, &a2dp_lhdc_default_config,
                    p_pref_cfg);

  /* now try to build a preferred one */
  /* parse configuration */
  status = A2D_ParsLHDCInfo(&src_cap, p_src_cap, true);
  if (status != A2D_SUCCESS) {
    APPL_TRACE_ERROR("%s: can't parse src cap ret = %d", __func__, status);
    return A2D_FAIL;
  }

  memset(&pref_cap, 0, sizeof(pref_cap));
  pref_cap.vendorId = a2dp_lhdc_caps.vendorId;
  pref_cap.codecId = a2dp_lhdc_caps.codecId;

  src_cap.samp_freq &= a2dp_lhdc_caps.samp_freq;
  if (src_cap.samp_freq & LHDC_SAMPLE_FREQ_96k)
    pref_cap.samp_freq = LHDC_SAMPLE_FREQ_96k;
  else if (src_cap.samp_freq & LHDC_SAMPLE_FREQ_48k)
    pref_cap.samp_freq = LHDC_SAMPLE_FREQ_48k;
  else if (src_cap.samp_freq & LHDC_SAMPLE_FREQ_44k)
    pref_cap.samp_freq = LHDC_SAMPLE_FREQ_44k;


  src_cap.bit_depth &= a2dp_lhdc_caps.bit_depth;
  if (src_cap.bit_depth & LHDC_BITDEPTH_24)
    pref_cap.bit_depth = LHDC_BITDEPTH_24;
  else if (src_cap.bit_depth & LHDC_BITDEPTH_16)
    pref_cap.bit_depth = LHDC_BITDEPTH_16;

  if (src_cap.obj_type & LHDC_OBJ_TYPE_2_0)
    pref_cap.obj_type = LHDC_OBJ_TYPE_2_0;

  pref_cap.max_rate = src_cap.max_rate;

  src_cap.channels &= a2dp_lhdc_caps.channels;
  if (src_cap.channels & LHDC_CHANNEL_STEREO)
    pref_cap.channels = LHDC_CHANNEL_STEREO;
  else if (src_cap.channels & LHDC_CHANNEL_CH5_1)
    pref_cap.channels = LHDC_CHANNEL_CH5_1;
  else if (src_cap.channels & LHDC_CHANNEL_MONO)
    pref_cap.channels = LHDC_CHANNEL_MONO;

  src_cap.compressor_output_fmt &= a2dp_lhdc_caps.compressor_output_fmt;
  if (src_cap.compressor_output_fmt & LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE)
    pref_cap.compressor_output_fmt = LHDC_COMPRESSOR_OUTPUT_FMT_DISABLE;
  else if (src_cap.compressor_output_fmt & LHDC_COMPRESSOR_OUTPUT_FMT_SPLIT)
    pref_cap.compressor_output_fmt = LHDC_COMPRESSOR_OUTPUT_FMT_SPLIT;
  else if (src_cap.compressor_output_fmt & LHDC_COMPRESSOR_OUTPUT_FMT_PRE_SPLIT)
    pref_cap.compressor_output_fmt = LHDC_COMPRESSOR_OUTPUT_FMT_PRE_SPLIT;

  status = A2D_BldLHDCInfo(AVDT_MEDIA_AUDIO, &pref_cap, p_pref_cfg);

  return status;
}

BOOLEAN A2D_VendorGetCodecConfigLhdc(UINT8 *p_cfg_info, UINT8 *p_codec_cfg)
{
    tA2D_LHDC_CIE *lhdc_config = (tA2D_LHDC_CIE *)p_codec_cfg;
    if(A2D_ParsLHDCInfo(lhdc_config, p_cfg_info, FALSE) == A2D_SUCCESS)
    {
        return TRUE;
    }
    else
    {
        memcpy(lhdc_config, &a2dp_lhdc_default_config, sizeof(tA2D_LHDC_CIE));
    }
    return TRUE;
}
/* --------------------------------------------------------------------------*/
typedef enum {
    ASM_PKT_WAT_STR,
    ASM_PKT_WAT_LST,
}ASM_PKT_STATUS;


#define A2DP_LHDC_HDR_F_MSK 0x80
#define A2DP_LHDC_HDR_S_MSK 0x40
#define A2DP_LHDC_HDR_L_MSK 0x20
#define A2DP_LHDC_HDR_FLAG_MSK ( A2DP_LHDC_HDR_F_MSK | A2DP_LHDC_HDR_S_MSK | A2DP_LHDC_HDR_L_MSK)

#define A2DP_LHDC_HDR_LATENCY_LOW   0x00
#define A2DP_LHDC_HDR_LATENCY_MID   0x01
#define A2DP_LHDC_HDR_LATENCY_HIGH  0x02
#define A2DP_LHDC_HDR_LATENCY_MASK  (A2DP_LHDC_HDR_LATENCY_MID | A2DP_LHDC_HDR_LATENCY_HIGH)

#define PACKET_BUFFER_LENGTH   4 * 1024

#define A2DP_LHDC_PCM_BUFFER_LEN (4096)

typedef struct {
    uint8_t prev_sn;
    uint8_t frames;
    uint8_t latency;
    ASM_PKT_STATUS status;
    uint32_t written_len;
    uint8_t * dat;
}ASM_PKT_ST;

typedef struct {
    uint16_t frame_len;
    BOOLEAN isSplit;
    BOOLEAN isLeft;
} LHDC_FRAME_HDR;

ASM_PKT_ST asm_st;
static UINT32 a2d_lhdc_sample_bytes = 4;
static UINT8 a2d_lhdc_latency = 0;

#ifdef A2D_LHDC_DBG_STAT
typedef struct
{
    uint32_t buf_cnt;
    uint32_t pkt_cnt;
    uint32_t frm_cnt;

    uint32_t max_assam_us; /* max assam interval: us*/
    uint32_t max_dec_us;   /* max decode interval: us*/
    uint32_t max_play_us;         /* max decode time */

    struct timeval last_dec_tm;   /* first start dec tm */
    struct timeval last_assam_tm; /* first start assam in tm */
    struct timeval last_play_tm; /* first start assam in tm */
} LHDC_DECODE_STAT;

LHDC_DECODE_STAT a2d_lhdc_dec_stat;
#endif

void reset_lhdc_assmeble_packet()
{
    if (asm_st.dat) {
        memset(asm_st.dat, 0, PACKET_BUFFER_LENGTH);
    }
    asm_st.status = ASM_PKT_WAT_STR;
    asm_st.written_len = 0;
}

void initial_lhdc_assemble_packet(){
    asm_st.dat = realloc(asm_st.dat, PACKET_BUFFER_LENGTH);

    reset_lhdc_assmeble_packet();
    asm_st.prev_sn = 0xff;
}


/**
 * Assemble LHDC packet...
 *
 */
int assemble_lhdc_packet(uint8_t * input, uint32_t input_len, uint8_t** pOut, uint32_t * pLen){
    uint8_t hdr = 0, seqno = 0xff;
    int ret = -1;
    //uint32_t status = 0;

    hdr = (*input);
    input++;
    seqno = (*input);
    input++;
    input_len -= 2;

    if( seqno != ((asm_st.prev_sn + 1) & 0xFF) ) {
        //Packet lose...
        reset_lhdc_assmeble_packet();
        if ((hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0 ||
             (hdr & A2DP_LHDC_HDR_S_MSK) != 0 )
        {
            goto lhdc_start;
        }
        else
        {
            APPL_TRACE_ERROR( "[LHDC] drop packet No. %u, last_no:%u\n",
                seqno, asm_st.prev_sn);
        }
        return -2;
    }

lhdc_start:
    switch (asm_st.status) {
        case ASM_PKT_WAT_STR:{
                if( (hdr & A2DP_LHDC_HDR_FLAG_MSK) == 0 )
                {
                    asm_st.frames = (hdr>>2) & 0x7;
                    asm_st.latency = hdr & 0x3;
                    memcpy(&(asm_st.dat[0]), input, input_len);
                    if (pLen && pOut) {
                        *pLen = input_len;
                        *pOut = asm_st.dat;
                    }
                    //Status not changed!!
                    //asm_st.status = ASM_PKT_WAT_STR;
                    asm_st.written_len = 0;
                    ret = 1;
                }
                else if( hdr & A2DP_LHDC_HDR_S_MSK )
                {
                    asm_st.frames = (hdr>>2) & 0x7;
                    asm_st.latency = hdr & 0x3;

                    if (asm_st.written_len + input_len >= PACKET_BUFFER_LENGTH) {
                        asm_st.written_len = 0;
                        asm_st.status = ASM_PKT_WAT_STR;
                        APPL_TRACE_ERROR("[LHDC] ASM_PKT_WAT_STR:Frame buffer overflow!(%d)\n", asm_st.written_len);
                        break;
                    }
                    memcpy(&(asm_st.dat[0]), input, input_len);
                    asm_st.written_len = input_len;
                    asm_st.status = ASM_PKT_WAT_LST;
                    ret = 0;
                }
                else
                    ret = -1;

                if( ret >= 0 )
                {
                    asm_st.prev_sn = seqno;
                }
            break;
        }
        case ASM_PKT_WAT_LST:{
            if (asm_st.written_len + input_len >= PACKET_BUFFER_LENGTH) {
                asm_st.written_len = 0;
                asm_st.status = ASM_PKT_WAT_STR;
                APPL_TRACE_ERROR("[LHDC] ASM_PKT_WAT_LST:Frame buffer overflow(%d)\n", asm_st.written_len);
                break;
            }
            memcpy(&(asm_st.dat[asm_st.written_len]), input, input_len);
            asm_st.written_len += input_len;
            ret = 0;

            if( hdr & A2DP_LHDC_HDR_L_MSK )
            {
                if (pLen && pOut) {
                    *pLen = asm_st.written_len;
                    *pOut = asm_st.dat;
                }
                asm_st.written_len = 0;
                asm_st.status = ASM_PKT_WAT_STR;
                ret = 1;
            }
            asm_st.prev_sn = seqno;
            break;
        }
        default:
            ret = -1;
            APPL_TRACE_ERROR("[LHDC] Status error(%d)\n", asm_st.status);
            break;

    }
    return ret;
}


/**
 * get lhdc frame header
 */
BOOLEAN get_lhdc_header(uint8_t * in, LHDC_FRAME_HDR * h) {
#define LHDC_HDR_LEN 4
    uint32_t hdr = 0;
    BOOLEAN ret = false;
    memcpy(&hdr, in , LHDC_HDR_LEN);
    h->frame_len = ( int)( ( hdr >> 8) & 0x1fff);
    h->isSplit = ( ( hdr & 0x00600000) == 0x00600000);
    h->isLeft = ((hdr & 0xf) == 0);

    if ( ( hdr & 0xff000000) != 0x4c000000){
        APPL_TRACE_ERROR( "[LHDC] lhdc hdr err!\n");
        ret = false;
    } else {
        ret = true;
    }
    return ret;
}


#define BITS_PER_SAMPLE 32
#define CHANNELS    2
#define SAPMLES_PER_FRAME   512
#define BYTES_PER_SAMPLE     (BITS_PER_SAMPLE / 8) * CHANNELS
#define BYTES_PER_FRAME     (BITS_PER_SAMPLE / 8) * CHANNELS * SAPMLES_PER_FRAME

static UINT8 user_license_key[128] = {0};
static char user_license_name[26] = {0};
extern bt_bdaddr_t btif_local_bd_addr;


int A2D_SetLHDCLicenseKey(char *name, uint8_t *data, int data_len)
{
    if (name == NULL || data == NULL || data_len <= 0)
    {
        APPL_TRACE_ERROR("[LHDC] name=%p, data=%p, data_len=%d", name, data, data_len);
        return -1;
    }

    strncpy(user_license_name, name, 25);
    user_license_name[25] = 0;

    memset(user_license_key, 0, sizeof(user_license_key));
    memcpy(user_license_key, data, data_len>128?128:data_len);

    APPL_TRACE_ERROR("[LHDC] license name=%s, key_len=%d", user_license_name, data_len);

#if 0
    {
        char buf[257]={0};
        int i=0;
        for(i=0;i<128;i++)
        {
            sprintf(buf+i*2, "%02x", user_license_key[i]);
        }
        buf[256] = 0;

        APPL_TRACE_ERROR("[LHDC] key=%s(buf[0]=0x%02x), (key[0]=0x%02x)",
            buf, buf[0], user_license_key[0]);
    }
#endif

    return 0;
}

int A2D_VendorBuildBtUserInfoCb(bt_get_local_info *bt_info)
{
    if (strlen(user_license_name) > 0)
    {
        uint8_t *ptr_addr = NULL;
        strncpy(bt_info->bt_name, user_license_name, 26);
        bt_info->bt_len = strlen(user_license_name) + 1;
        if (bt_info->bt_len > 26)
        {
            bt_info->bt_len = 26;
        }
        bt_info->bt_name[bt_info->bt_len-1] = 0;

        ptr_addr = (uint8_t *)&btif_local_bd_addr;

        bt_info->bt_addr[0] = ptr_addr[5];
        bt_info->bt_addr[1] = ptr_addr[4];
        bt_info->bt_addr[2] = ptr_addr[3];
        bt_info->bt_addr[3] = ptr_addr[2];
        bt_info->bt_addr[4] = ptr_addr[1];
        bt_info->bt_addr[5] = ptr_addr[0];
    }

    return 0;
}

int A2D_VendorBuildBtInfoCb(bt_get_local_info *bt_info)
{
    /**
     *Name : BES2000iZ
     *Start address: 111122333333
     *End address: 111122333396
     */
    char name[] = "BES2000iZ";
    char addr[] = {0x33, 0x33, 0x33, 0x22, 0x11, 0x11};

    memcpy(bt_info->bt_name, name, strlen(name) + 1);
    bt_info->bt_len = strlen(name) + 1;
    memcpy(bt_info->bt_addr, addr, sizeof(addr));

    return 0;
}


static UINT8 test_key_bin[128];

BOOLEAN A2D_VendorDecoderInitLhdc(UINT32 bitdepth, UINT32 scal_to_16b,
    decoded_data_callback_t decode_callback)
{
    APPL_TRACE_ERROR("[LHDC] init bitdepth=%d, scal_to_16b=%d, inited=%d",
        bitdepth, scal_to_16b, lhdc_decode_inited);

    if (lhdc_decode_inited == 0)
    {
        BOOLEAN license_key_ok = FALSE;
        initial_lhdc_assemble_packet();
        if (lhdc_decoder_set_license_key_table_func != NULL)
        {
            if (strlen(user_license_name) > 0)
            {
                if (lhdc_decoder_set_license_key_table_func(user_license_key, A2D_VendorBuildBtUserInfoCb) == TRUE)
                {
                    APPL_TRACE_WARNING("[LHDC] %s set user license key fail", __func__);
                    license_key_ok = TRUE;
                }
                else
                {
                    APPL_TRACE_ERROR("[LHDC] %s set user license key fail", __func__);
                }
            }

            if (license_key_ok == FALSE && lhdc_decoder_test_key_bin != NULL)
            {
                //APPL_TRACE_ERROR("[LHDC] %s default key[0]=0x%02x, key[127]=0x%02x", __func__,
                //    lhdc_decoder_test_key_bin[0], lhdc_decoder_test_key_bin[127]);
                memcpy(&test_key_bin, lhdc_decoder_test_key_bin, 128);

                if (lhdc_decoder_set_license_key_table_func(test_key_bin, A2D_VendorBuildBtInfoCb) != TRUE)
                {
                    APPL_TRACE_ERROR("[LHDC] %s set default license key fail", __func__);
                }
                else
                {
                    APPL_TRACE_WARNING("[LHDC] %s set default license key OK", __func__);
                }
            }
        }

        lhdc_decoder_init_func(bitdepth, scal_to_16b);

        if (24 == bitdepth)
        {
            a2d_lhdc_sample_bytes = 8;
        }
        else
        {
            a2d_lhdc_sample_bytes = 4;
        }
#ifdef A2D_LHDC_DBG_STAT
        memset(&a2d_lhdc_dec_stat, 0, sizeof(a2d_lhdc_dec_stat));
#endif
        lhdc_decode_data_callback = decode_callback;
        lhdc_decode_inited = 1;
        return TRUE;
    }
    return FALSE;
}

BOOLEAN A2D_VendorDecoderDestroyLhdc(void)
{
    APPL_TRACE_ERROR("[LHDC] deinit, inited=%d", lhdc_decode_inited);
    if (lhdc_decode_inited == 1)
    {
        reset_lhdc_assmeble_packet();
        if (asm_st.dat)
        {
            free(asm_st.dat);
            asm_st.dat = NULL;
        }
        lhdc_decoder_destroy_func();
        lhdc_decode_inited = 0;
    }
    return TRUE;
}

#define LHDC_DEBUG(str, ...) //APPL_TRACE_ERROR(str, ##__VA_ARGS__)
#define LHDC_ERROR(str, ...) APPL_TRACE_ERROR(str, ##__VA_ARGS__)

UINT8 a2d_lhdc_output_pcm_buf[A2DP_LHDC_PCM_BUFFER_LEN];
BOOLEAN A2D_VendorDecoderProcessLhdc(BT_HDR* p_buf)
{
    uint8_t *output_pkt;
    uint32_t output_pkt_size;
    int assmeble_ret = 0;
    LHDC_FRAME_HDR hdr_info;

    uint8_t* pBuffer = (uint8_t* )(p_buf->data + p_buf->offset);
    UINT32 bufferSize = p_buf->len;

#ifdef A2D_LHDC_DBG_STAT
    if (a2d_lhdc_dec_stat.buf_cnt == 0)
    {
        gettimeofday(&a2d_lhdc_dec_stat.last_assam_tm, NULL);
    }
    else
    {
        struct timeval last_assam_tm = a2d_lhdc_dec_stat.last_assam_tm;
        uint32_t diff_us = 0;
        gettimeofday(&a2d_lhdc_dec_stat.last_assam_tm, NULL);
        diff_us = A2D_LHDC_DIFF_TM_US(a2d_lhdc_dec_stat.last_assam_tm, last_assam_tm);

        if (diff_us > a2d_lhdc_dec_stat.max_assam_us)
        {
            a2d_lhdc_dec_stat.max_assam_us = diff_us;
        }

        if (diff_us > 80000)
        {
            LHDC_ERROR("[LHDC] assam diff_us time %u us, seq_no=%d",
                diff_us, p_buf->layer_specific);
        }
    }
    a2d_lhdc_dec_stat.buf_cnt++;
#endif

    if ((assmeble_ret = assemble_lhdc_packet(pBuffer, bufferSize, &output_pkt, &output_pkt_size)) > 0)
    {
        if (output_pkt != NULL && output_pkt_size != 0)
        {
            int offset = 0;

            struct timeval prev1;
            struct timeval now1;
            UINT32 diff1 = 0;
            UINT32 frames = 0;
            gettimeofday(&prev1, NULL);

            LHDC_DEBUG("[LHDC] start decode packet(frame=%d), time=%lld.%06d, output_pkt_size=%d",
                        asm_st.frames, (UINT64)prev1.tv_sec, (UINT32)prev1.tv_usec,
                            output_pkt_size);
            if (a2d_lhdc_latency != asm_st.latency)
            {
                LHDC_ERROR("[LHDC] latency change %d => %d",
                        a2d_lhdc_latency, asm_st.latency);
                a2d_lhdc_latency = asm_st.latency;
            }

#ifdef A2D_LHDC_DBG_STAT
            if (a2d_lhdc_dec_stat.pkt_cnt == 0)
            {
                gettimeofday(&a2d_lhdc_dec_stat.last_dec_tm, NULL);
            }
            else
            {
                struct timeval last_dec_tm = a2d_lhdc_dec_stat.last_dec_tm;
                uint32_t diff_us = 0;
                gettimeofday(&a2d_lhdc_dec_stat.last_dec_tm, NULL);
                diff_us = A2D_LHDC_DIFF_TM_US(a2d_lhdc_dec_stat.last_dec_tm, last_dec_tm);

                if (diff_us > a2d_lhdc_dec_stat.max_dec_us)
                {
                    a2d_lhdc_dec_stat.max_dec_us = diff_us;
                }
                if (diff_us > 80000)
                {
                    LHDC_ERROR("[LHDC] dec diff_us time %u us, seq_no=%d",
                        diff_us, p_buf->layer_specific);
                }
            }
            a2d_lhdc_dec_stat.pkt_cnt++;
#endif
            while (output_pkt_size) {

                //gettimeofday(&now1, NULL);
                //LHDC_DEBUG("%s [LHDC] start get header, time=%lld.%06d",
                //    __func__, (UINT64)now1.tv_sec, (UINT32)now1.tv_usec);

                if (get_lhdc_header(output_pkt + offset, &hdr_info)) {
                    struct timeval prev;
                    struct timeval now;
                    struct timeval now1;
                    UINT32 diff = 0;
                    UINT32 diff1 = 0;
                    UINT32 diff2 = 0;
                    int put_len = 0;
                    gettimeofday(&prev, NULL);
                    if ((hdr_info.isSplit && !hdr_info.isLeft) || !hdr_info.isSplit)
                    {
                        LHDC_DEBUG("[LHDC] start decode frame, time=%lld.%06d, frame_len=%d, isLeft=%d, isSplit=%d",
                            (UINT64)prev.tv_sec, (UINT32)prev.tv_usec,
                            hdr_info.frame_len, hdr_info.isLeft, hdr_info.isSplit);

                        //To putting 1 frame data to internal queue of the decoder.
                        put_len = lhdc_decoder_put_frame_func(output_pkt + offset, hdr_info.frame_len);

                        uint32_t offset2 = 0;
                        uint32_t frame_samples = 0;

                        //In normally, this loop should be executed 8 times.
                        while (true) {
                            INT32 outSamples = lhdc_decoder_decode_process_func(a2d_lhdc_output_pcm_buf + offset2);
                            if (outSamples < 64) {
                                // output samples should be 64.
                                // If outSamples is 0 that means is need more data.
                                // If outSamples is small than 0 that means is decode error.
                                break;
                            }
                            offset2 += outSamples * a2d_lhdc_sample_bytes;
                            frame_samples += outSamples;

                            LHDC_DEBUG("[LHDC] seq_no=%d, frame_samples=%d",
                                        p_buf->layer_specific, frame_samples);

                            if (frame_samples >= 512)
                            {
#ifdef A2D_LHDC_DBG_STAT
                                if (a2d_lhdc_dec_stat.frm_cnt == 0)
                                {
                                    gettimeofday(&a2d_lhdc_dec_stat.last_play_tm, NULL);
                                }
                                else
                                {
                                    struct timeval last_play_tm = a2d_lhdc_dec_stat.last_play_tm;
                                    uint32_t diff_us = 0;
                                    gettimeofday(&a2d_lhdc_dec_stat.last_play_tm, NULL);
                                    diff_us = A2D_LHDC_DIFF_TM_US(a2d_lhdc_dec_stat.last_play_tm, last_play_tm);

                                    if (diff_us > a2d_lhdc_dec_stat.max_play_us)
                                    {
                                        a2d_lhdc_dec_stat.max_play_us = diff_us;
                                    }
                                    if (diff_us > 80000)
                                    {
                                        LHDC_ERROR("[LHDC] play diff_us time %u us, seq_no=%d",
                                            diff_us, p_buf->layer_specific);
                                    }
                                }
                                a2d_lhdc_dec_stat.frm_cnt += frame_samples;
#endif

                                lhdc_decode_data_callback(a2d_lhdc_output_pcm_buf, offset2);
                                offset2 = 0;
                                frame_samples = 0;
                            }
                        }

                        gettimeofday(&now, NULL);
                        diff2 = A2D_LHDC_DIFF_TM_US(now, prev);
                        frames++;

                        //APPL_TRACE_ERROR("[LHDC] time=%lld.%06d, offset2=%d, dec-diff=%d, frame_len=%d",
                        //    (UINT64)now.tv_sec, (UINT32)now.tv_usec, offset2, diff, hdr_info.frame_len);
                        if (offset2 > 0)
                        {
                            LHDC_DEBUG("[LHDC] decode over length, seq_no=%d, offset2=%d",
                                        p_buf->layer_specific, offset2);
                            lhdc_decode_data_callback(a2d_lhdc_output_pcm_buf, offset2);
                        }

                        gettimeofday(&now1, NULL);
                        diff = A2D_LHDC_DIFF_TM_US(now1, prev);
                        diff1 = A2D_LHDC_DIFF_TM_US(now1, now);

                        LHDC_DEBUG("[LHDC] end decode frame time=%lld.%06d, offset2=%d, total-diff=%d,"
                            "send-diff=%d, dec-diff=%d, frame_len=%d, output_pkt_size=%d, put_len=%d",
                            (UINT64)now1.tv_sec, (UINT32)now1.tv_usec, offset2,
                            diff, diff1, diff2, hdr_info.frame_len, output_pkt_size, put_len);
                    }
                    offset += hdr_info.frame_len;
                    output_pkt_size -= hdr_info.frame_len;
                }else{
                    LHDC_ERROR("[LHDC] parse frame head fail, seq_no=%d",
                                p_buf->layer_specific);
                    break;
                }
            }

            gettimeofday(&now1, NULL);
            diff1 = A2D_LHDC_DIFF_TM_US(now1, prev1);

            LHDC_DEBUG("[LHDC] end decode packet, time=%lld.%06d, frames=%d, diff=%d, output_pkt_size=%d",
                (UINT64)now1.tv_sec, (UINT32)now1.tv_usec, frames, diff1, output_pkt_size);
        }
        else
        {
            LHDC_ERROR("[LHDC] assemble_lhdc_packet fail, seq_no=%d, output_pkt=%p",
                        p_buf->layer_specific, output_pkt_size, output_pkt);
        }
    }
    else
    {
        if (assmeble_ret < 0)
        {
            LHDC_ERROR("[LHDC] assemble_lhdc_packet fail, seq_no=%d",
                        p_buf->layer_specific);
        }
    }

    return TRUE;
}



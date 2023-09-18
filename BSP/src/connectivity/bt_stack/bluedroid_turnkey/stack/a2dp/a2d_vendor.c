/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Vendor Specific A2DP Codecs Support
 */

#define LOG_TAG "a2dp_vendor"

#include "bt_target.h"
#include "a2d_vendor.h"
#include "bta_av_api.h"
#include "a2d_api.h"
#include "a2d_lhdc.h"
#include "a2d_aptx.h"
#include "osi/include/log.h"
#include "osi/include/osi.h"

extern const tA2D_APTX_CIE btif_av_aptx_default_config;

bool A2D_IsVendorPeerSourceCodecValid(
    const uint8_t* p_codec_info) {
  uint32_t vendor_id = A2D_VendorCodecGetVendorId(p_codec_info);
  uint16_t codec_id = A2D_VendorCodecGetCodecId(p_codec_info);

  switch(vendor_id)
  {
    case A2DP_LHDC_VENDOR_ID:
        if (codec_id == A2DP_LHDC_CODEC_ID)
        {
            return TRUE;
        }
        break;
    default:
        break;
  }

  return FALSE;
}


tA2D_STATUS A2D_VendorBuildSrc2SinkConfig(UINT8 *p_pref_cfg, UINT8 *p_src_cap)
{
  uint32_t vendor_id = A2D_VendorCodecGetVendorId(p_src_cap);
  uint16_t codec_id = A2D_VendorCodecGetCodecId(p_src_cap);
  APPL_TRACE_DEBUG("%s: vendor_id:0x%x, codec_id:0x%x", __func__, vendor_id, codec_id);

  switch(vendor_id)
  {
    case A2DP_LHDC_VENDOR_ID:
        if (codec_id == A2DP_LHDC_CODEC_ID)
        {
            return A2D_BuildSrc2SinkConfigLhdc(p_pref_cfg, p_src_cap);
        }
        break;
    default:
        break;
  }

  return A2D_NS_CODEC_TYPE;
}

uint32_t A2D_VendorCodecGetVendorId(const uint8_t* p_codec_info) {
  const uint8_t* p = &p_codec_info[A2D_VENDOR_CODEC_VENDOR_ID_START_IDX];

  uint32_t vendor_id = (p[0] & 0x000000ff) | ((p[1] << 8) & 0x0000ff00) |
                       ((p[2] << 16) & 0x00ff0000) |
                       ((p[3] << 24) & 0xff000000);

  return vendor_id;
}

uint16_t A2D_VendorCodecGetCodecId(const uint8_t* p_codec_info) {
  const uint8_t* p = &p_codec_info[A2D_VENDOR_CODEC_CODEC_ID_START_IDX];

  uint16_t codec_id = (p[0] & 0x00ff) | ((p[1] << 8) & 0xff00);

  return codec_id;
}

UINT8 A2D_VendorCfgInCap(UINT8 *p_cfg)
{
  UINT8 status = A2D_FAIL;
  uint32_t vendor_id = A2D_VendorCodecGetVendorId(p_cfg);
  uint16_t codec_id = A2D_VendorCodecGetCodecId(p_cfg);
  APPL_TRACE_DEBUG("%s: vendor_id:0x%x, codec_id:0x%x", __func__, vendor_id, codec_id);

  switch(vendor_id)
  {
    case A2DP_LHDC_VENDOR_ID:
        if (codec_id == A2DP_LHDC_CODEC_ID)
        {
            status = bta_av_lhdc_cfg_in_cap(p_cfg);
        }
        break;
    default:
        break;
  }
  return status;
}

BOOLEAN A2D_VendorGetCodecConfig(UINT8 *p_codec_cfg, UINT8 *p_cfg_info)
{
  BOOLEAN status = FALSE;
  uint32_t vendor_id = A2D_VendorCodecGetVendorId(p_cfg_info);
  uint16_t codec_id = A2D_VendorCodecGetCodecId(p_cfg_info);
  APPL_TRACE_DEBUG("%s: vendor_id:0x%x, codec_id:0x%x", __func__, vendor_id, codec_id);

  switch(vendor_id)
  {
    case A2DP_LHDC_VENDOR_ID:
        if (codec_id == A2DP_LHDC_CODEC_ID)
        {
            status = A2D_VendorGetCodecConfigLhdc(p_cfg_info, p_codec_cfg);
        }
        break;
#if defined(MTK_A2DP_SRC_APTX_CODEC) && (MTK_A2DP_SRC_APTX_CODEC == TRUE)
    case CSR_APTX_VENDOR_ID:
        if (codec_id == CSR_APTX_CODEC_ID_BLUETOOTH)
        {
            tA2D_APTX_CIE *aptx_config = (tA2D_APTX_CIE *)p_cfg_info;
            if(A2D_ParsAptxInfo(aptx_config, p_codec_cfg, FALSE) == A2D_SUCCESS)
            {
                status = TRUE;
            }
            else
            {
                memcpy((tA2D_APTX_CIE *) p_cfg_info, &btif_av_aptx_default_config, sizeof(tA2D_APTX_CIE));
            }
        }
#endif
    default:
        break;
  }
  return status;
}

tBTA_AV_CODEC A2D_GetCodecType(const uint8_t* p_codec_info)
{
  return (tBTA_AV_CODEC)(p_codec_info[AVDT_CODEC_TYPE_INDEX]);
}

bool A2D_VendorCodecTypeEquals(const uint8_t* p_codec_info_a,
                                const uint8_t* p_codec_info_b)
{
  tBTA_AV_CODEC codec_type_a = A2D_GetCodecType(p_codec_info_a);
  tBTA_AV_CODEC codec_type_b = A2D_GetCodecType(p_codec_info_b);

  if ((codec_type_a != codec_type_b) ||
      (codec_type_a != A2D_MEDIA_CT_NON)) {
    return false;
  }

  uint32_t vendor_id_a = A2D_VendorCodecGetVendorId(p_codec_info_a);
  uint16_t codec_id_a = A2D_VendorCodecGetCodecId(p_codec_info_a);
  uint32_t vendor_id_b = A2D_VendorCodecGetVendorId(p_codec_info_b);
  uint16_t codec_id_b = A2D_VendorCodecGetCodecId(p_codec_info_b);

  if (vendor_id_a != vendor_id_b || codec_id_a != codec_id_b) return false;

  return true;
}



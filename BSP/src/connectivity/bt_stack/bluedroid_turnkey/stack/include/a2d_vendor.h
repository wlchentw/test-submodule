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

//
// Vendor Specific A2DP Codecs Support
//

#ifndef A2D_VENDOR_H
#define A2D_VENDOR_H

#include <stdbool.h>
#include "a2d_api.h"
/* Offset for A2DP vendor codec */
#define A2D_VENDOR_CODEC_START_IDX 3

/* Offset for Vendor ID for A2DP vendor codec */
#define A2D_VENDOR_CODEC_VENDOR_ID_START_IDX A2D_VENDOR_CODEC_START_IDX

/* Offset for Codec ID for A2DP vendor codec */
#define A2D_VENDOR_CODEC_CODEC_ID_START_IDX \
  (A2D_VENDOR_CODEC_VENDOR_ID_START_IDX + sizeof(uint32_t))

// Checks whether the codec capabilities contain a valid A2DP vendor-specific
// Source codec.
// NOTE: only codecs that are implemented are considered valid.
// Returns true if |p_codec_info| contains information about a valid
// vendor-specific codec, otherwise false.
bool A2DP_IsVendorSourceCodecValid(const uint8_t* p_codec_info);


// Builds a vendor-specific A2DP preferred Sink capability from a vendor
// Source capability.
// |p_src_cap| is the Source capability to use.
// |p_pref_cfg| is the result Sink capability to store.
// Returns |A2DP_SUCCESS| on success, otherwise the corresponding A2DP error
// status code.
tA2D_STATUS A2D_VendorBuildSrc2SinkConfig(UINT8 *p_pref_cfg, UINT8 *p_src_cap);

// Gets the Vendor ID for the vendor-specific A2DP codec.
// |p_codec_info| contains information about the codec capabilities.
// Returns the Vendor ID for the vendor-specific A2DP codec.
uint32_t A2D_VendorCodecGetVendorId(const uint8_t* p_codec_info);

// Gets the Codec ID for the vendor-specific A2DP codec.
// |p_codec_info| contains information about the codec capabilities.
// Returns the Codec ID for the vendor-specific A2DP codec.
uint16_t A2D_VendorCodecGetCodecId(const uint8_t* p_codec_info);

UINT8 A2D_VendorCfgInCap(UINT8 *p_cfg);
BOOLEAN A2D_VendorGetCodecConfig(UINT8 *p_cfg, UINT8 *p_codec_cfg);

bool A2D_VendorCodecTypeEquals(const uint8_t* p_codec_info_a,
                                const uint8_t* p_codec_info_b);


#endif  // A2DP_VENDOR_H

/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2017. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef TZ_TBASE_H
#define TZ_TBASE_H

#include "typedefs.h"

/* Tbase Magic For Interface */
#define TBASE_BOOTCFG_MAGIC (0x434d4254U) // String TBMC in little-endian

/* TEE version */
#define TEE_ARGUMENT_VERSION (0x00010000U)

typedef struct {
    u32 magic;        // magic value from information
    u32 length;       // size of struct in bytes.
    u64 version;      // Version of structure
    u64 dRamBase;     // NonSecure DRAM start address
    u64 dRamSize;     // NonSecure DRAM size
    u64 secDRamBase;  // Secure DRAM start address
    u64 secDRamSize;  // Secure DRAM size
    u64 secIRamBase;  // Secure IRAM base
    u64 secIRamSize;  // Secure IRam size
    u64 conf_mair_el3;// MAIR_EL3 for memory attributes sharing
    u32 RFU1;
    u32 MSMPteCount;  // Number of MMU entries for MSM
    u64 MSMBase;      // MMU entries for MSM
    u64 gic_distributor_base;
    u64 gic_cpuinterface_base;
    u32 gic_version;
    u32 RFU2;
    u64 flags;
    u32 total_number_spi;
    u32 ssiq_number;
}tee_arg_t, *tee_arg_t_ptr;

/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/
void tbase_secmem_param_prepare(u32 param_addr, u32 tee_entry, u32 tbase_sec_dram_size, u32 tee_smem_size);
void tbase_boot_param_prepare(u32 param_addr, u32 tee_entry, u64 tbase_sec_dram_size, u64 dram_base, u64 dram_size);

#endif /* TZ_TBASE_H */

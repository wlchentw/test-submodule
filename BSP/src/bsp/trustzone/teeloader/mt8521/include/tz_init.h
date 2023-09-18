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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef TRUSTZONE_H
#define TRUSTZONE_H

#include "tz_keys.h"
#include "typedefs.h"

#define ATF_BOOTCFG_MAGIC (0x4D415446) // String MATF in little-endian
#define DEVINFO_SIZE 4

#define MCUSYS_CFGREG_BASE  (0x10000000 + 0x00200000)
#define RVBADDRESS_CPU0     (MCUSYS_CFGREG_BASE + 0x38)

/* 
    RSA2048 public key for verifying mtee image
    It should be the same as AUTH_PARAM_N in alps\mediatek\custom\mt6752_evb\trustzone\TRUSTZONE_IMG_PROTECT_CFG.ini
*/
#define MTEE_IMG_VFY_PUBK_SZ 256

typedef struct {
	u32 atf_magic;
	u32 tee_support;
	u32 tee_entry;
	u32 tee_boot_arg_addr;
	u32 hwuid[4];     // HW Unique id for t-base used
	u32 HRID[2];      // HW random id for t-base used
	u32 atf_log_port;
	u32 atf_log_baudrate;
	u32 atf_log_buf_start;
	u32 atf_log_buf_size;
	u32 atf_irq_num;
	u32 devinfo[DEVINFO_SIZE];
	u32 atf_aee_debug_buf_start;
	u32 atf_aee_debug_buf_size;
#if CFG_TEE_SUPPORT
	u32 tee_rpmb_size;
#endif
} atf_arg_t, *atf_arg_t_ptr;

/**************************************************************************
 * EXPORTED FUNCTIONS
 **************************************************************************/
void tee_get_secmem_start(u32 *addr);
void tee_get_secmem_size(u32 *size);
void tee_set_entry(u32 addr);
void tee_set_hwuid(u8 *id, u32 size);
int  tee_verify_image(u32 *addr, u32 size);
u32 tee_get_load_addr(u32 maddr);
void trustzone_pre_init(void);
void trustzone_post_init(void);
void trustzone_jump(u32 addr, u32 arg1, u32 arg2);

#endif /* TRUSTZONE_H */

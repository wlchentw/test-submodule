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

//#include "device_apc.h"
#include "print.h"
#include "typedefs.h"
#include "tz_init.h"
#include "tz_mem.h"
#include "tz_tbase.h"
#include "platform.h"

static u64 trustzone_get_atf_boot_param_addr(void)
{
    return ATF_BOOT_ARG_ADDR;
}

static void set_atf_parameters(mtk_bl_param_t *atf_arg, unsigned long boot_reason)
{
    atf_arg->bootarg_loc = 0;
    atf_arg->bootarg_size = 0;
    atf_arg->bl33_start_addr = BL33;
    atf_arg->tee_info_addr = ATF_INIT_ARG_ADDR;
    atf_arg->boot_reason = boot_reason;
}

int teeloader_main(unsigned long bl33_addr, unsigned long boot_reason)
{
    u32 tee_addr = 0;
    u32 hwuid[4] = { 0x55C09893, 0x2B404DDF, 0x3ACE08B, 0x1092600D };
    mtk_bl_param_t *atf_arg = (mtk_bl_param_t *)trustzone_get_atf_boot_param_addr();

    set_atf_parameters(atf_arg, boot_reason);

    /* marked because no device APC support */
	//device_APC_dom_setup();
    trustzone_pre_init();

#if CFG_TEE_SUPPORT
    tee_addr = TRUSTEDOS_ENTRYPOINT;
#endif
    /* set tee entry address */
    tee_set_entry(tee_addr);
    tee_set_hwuid((u8*)&hwuid[0], sizeof(hwuid));

    trustzone_post_init();
    trustzone_jump(BL31, BL33, tee_addr);

    return 0;
}

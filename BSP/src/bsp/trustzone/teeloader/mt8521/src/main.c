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

#include "stdlib.h"
#include "typedefs.h"
#include "tz_init.h"

typedef void (*jump_func_type)(u32 addr ,u32 arg1, u32 arg2) __attribute__ ((__noreturn__));

extern unsigned int heap_start_addr;
extern unsigned int heap_max_size;
extern unsigned int heap_current_alloc;
extern int mtee_verify_decrypt(u32 *pt_addr, u32 mtee_total_memory_size, const u8 au8_pubk_n[MTEE_IMG_VFY_PUBK_SZ]);
extern int mtee_decrypt(u32* pt_addr);

void sec_malloc_buf_reset(void)
{
	heap_start_addr = BASE_ADDR - 0x200000;
	heap_max_size = 0x4000;
	heap_current_alloc = 0;
}

void bldr_jump(u32 addr, u32 arg1, u32 arg2)
{
	jump_func_type jump_func;
	jump_func = (jump_func_type)addr;
	(*jump_func)(arg1, 0, 0);
}

int teeloader_prepare_tee(void)
{
	int ret = 0;
	u32 load_addr = BASE_ADDR+TL_ALIGN_SIZE;
	u32 real_addr = load_addr;
	init_dram_buffer();
	sec_malloc_buf_reset();
#if TL_VERIFY_ENABLE != 0
	ret = mtee_decrypt(&load_addr);
#endif

	return ret;
}

int teeloader_main(unsigned long next_stage_entry)
{
	int ret = 0;
	ret = teeloader_prepare_tee();
	if (ret != 0) {
		tl_printf("[teeloader] tee img verify fail, stop!\n");
		while (1) {
		}
	}
	tl_printf("[teeloader] jump to tee 0X%x\n", (BASE_ADDR + TL_ALIGN_SIZE));
	bldr_jump(BASE_ADDR + TL_ALIGN_SIZE, next_stage_entry, sizeof(next_stage_entry));
	return 0;
}

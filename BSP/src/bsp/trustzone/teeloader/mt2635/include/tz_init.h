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

#ifndef TZ_INIT_H
#define TZ_INIT_H


#define ATF_BOOTCFG_MAGIC (0x4D415446) // String MATF in little-endian
#define DEVINFO_SIZE 4

#define TEE_BOOT_ARG_ADDR (ATF_BOOT_ARG_ADDR + 0x100)

#define TEE_PARAMETER_BASE (ATF_BOOT_ARG_ADDR)
#define TEE_PARAMETER_ADDR (TEE_BOOT_ARG_ADDR + 0x100)

#if CFG_ATF_LOG_SUPPORT
#define ATF_LOG_BUFFER_SIZE (0x40000)//256KB
#define ATF_AEE_BUFFER_SIZE (0x4000)//16KB
#else
#define ATF_LOG_BUFFER_SIZE (0x0)//don't support ATF log
#define ATF_AEE_BUFFER_SIZE (0x0)//don't support ATF log
#endif

typedef struct {
	unsigned int atf_magic;
	unsigned int tee_support;
	unsigned int tee_entry;
	unsigned int tee_boot_arg_addr;
	unsigned int hwuid[4];     // HW Unique id for t-base used
	unsigned int HRID[2];      // HW random id for t-base used
	unsigned int atf_log_port;
	unsigned int atf_log_baudrate;
	unsigned int atf_log_buf_start;
	unsigned int atf_log_buf_size;
	unsigned int atf_irq_num;
	unsigned int devinfo[DEVINFO_SIZE];
	unsigned int atf_aee_debug_buf_start;
	unsigned int atf_aee_debug_buf_size;
#if CFG_TEE_SUPPORT
	unsigned int tee_rpmb_size;
#endif
} atf_arg_t, *atf_arg_t_ptr;

extern void tee_set_entry(unsigned int addr);
extern void tee_set_hwuid(unsigned char *id, unsigned int size);
void trustzone_pre_init(void);
void trustzone_post_init(void);
void trustzone_jump(unsigned int addr, unsigned int arg1, unsigned int arg2);

#endif /* TZ_INIT_H */

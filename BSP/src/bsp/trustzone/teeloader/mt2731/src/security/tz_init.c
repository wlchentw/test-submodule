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

#include "platform.h"
#include "print.h"
#include "sec_devinfo.h"
#include "string.h"
#include "typedefs.h"
#include "tz_emi_mpu.h"
#include "tz_init.h"
#include "device_apc.h"
#include "tz_mem.h"
#if CFG_TRUSTONIC_TEE_SUPPORT
#include "tz_tbase.h"
#endif

/**************************************************************************
 *  DEBUG FUNCTIONS
 **************************************************************************/
#define MOD "[TZ_INIT]"

#define TEE_DEBUG
#ifdef TEE_DEBUG
#define DBG_MSG(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#else
#define DBG_MSG(str, ...) do {} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#endif

/**************************************************************************
 *  MACROS
 **************************************************************************/
#define TEE_MEM_ALIGNMENT (0x1000U)  //4K Alignment
#define TEE_ENABLE_VERIFY (1U)

/**************************************************************************
 *  EXTERNAL FUNCTIONS
 **************************************************************************/
extern void tz_sec_mem_init(u32 start, u32 end, u32 mpu_region);
extern void tz_dapc_sec_init(void);
extern void tz_dapc_sec_postinit(void);

/**************************************************************************
 *  INTERNAL VARIABLES
 **************************************************************************/
static u32 tee_entry_addr = 0;
static u8 g_hwuid[16];
static u8 g_hwuid_initialized = 0;

/**************************************************************************
 *  INTERNAL FUNCTIONS
 **************************************************************************/

static u64 trustzone_get_atf_init_param_addr(void)
{
    return ATF_INIT_ARG_ADDR;
}

static u32 tee_secmem_size = 0;
static u32 tee_secmem_start = 0;
static u32 atf_log_buf_start = 0;

void tee_set_entry(u32 addr)
{
    tee_entry_addr = addr;

    DBG_MSG("%s TEE start entry : 0x%x\n", MOD, tee_entry_addr);
}

void tee_set_hwuid(u8 *id, u32 size)
{
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)(void *)trustzone_get_atf_init_param_addr();

    memcpy(teearg->hwuid, id, size);
    memcpy(g_hwuid, id, size);
    g_hwuid_initialized = 1;

    DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[0], id[1], id[2], id[3]);
    DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[4], id[5], id[6], id[7]);
    DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[8], id[9], id[10], id[11]);
    DBG_MSG("%s MEID : 0x%x, 0x%x, 0x%x, 0x%x\n", MOD, id[12], id[13], id[14], id[15]);
}

int tee_get_hwuid(u8 *id, u32 size)
{
    int ret = 0;

    if (!g_hwuid_initialized)
        return -1;

    memcpy(id, g_hwuid, size);

    return ret;
}

static void tee_sec_config(void)
{
    u32 atf_entry_addr = BL31;

#if CFG_TEE_SUPPORT
#if CFG_TEE_SECURE_MEM_PROTECTED
    /* memory protection for TEE */
    u32 secmem_end_addr = tee_entry_addr + tee_secmem_size - 1;

    tz_sec_mem_init(tee_entry_addr, secmem_end_addr, SECURE_OS_MPU_REGION_ID);
    DBG_MSG("%s set secure memory protection : 0x%x, 0x%x (%d)\n", MOD, tee_entry_addr,
        secmem_end_addr, SECURE_OS_MPU_REGION_ID);
#endif
#endif

    /* memory protection for ATF */
    atf_entry_addr = atf_entry_addr & ~(EMI_MPU_ALIGNMENT - 1);
    u32 atf_end_addr = atf_entry_addr + BL31_SIZE - 1;

    DBG_MSG("%s ATF entry addr, aligned addr : 0x%x, 0x%x\n", MOD, BL31, atf_entry_addr);

    tz_sec_mem_init(atf_entry_addr, atf_end_addr, ATF_MPU_REGION_ID);
    DBG_MSG("%s set ATF memory protection : 0x%x, 0x%x (%d)\n", MOD, atf_entry_addr,
        atf_end_addr, ATF_MPU_REGION_ID);
}

void trustzone_pre_init(void)
{
#if CFG_ATF_LOG_SUPPORT
    atf_log_buf_start = CFG_ATF_LOG_BUFFER_ADDR;
#endif

#if CFG_TEE_SUPPORT
    tee_secmem_size = CFG_TEE_SECMEM_SIZE;
#endif
    tz_apc_common_init();
}

void trustzone_post_init(void)
{
    atf_arg_t_ptr atf_init_arg = (atf_arg_t_ptr)(void *)trustzone_get_atf_init_param_addr();

    atf_init_arg->atf_magic = ATF_BOOTCFG_MAGIC;
    atf_init_arg->tee_entry = tee_entry_addr;
    atf_init_arg->tee_boot_arg_addr = TEE_BOOT_ARG_ADDR;
    atf_init_arg->HRID[0] = seclib_get_devinfo_with_index(E_AREA12);
    atf_init_arg->HRID[1] = seclib_get_devinfo_with_index(E_AREA13);
    atf_init_arg->atf_log_port = 0x11002000;
    atf_init_arg->atf_log_baudrate = 0xE1000;
    atf_init_arg->atf_irq_num = 267; /* reserve SPI ID for ATF log */
    atf_init_arg->devinfo[0] = 0;
    atf_init_arg->devinfo[1] = 0;
    atf_init_arg->devinfo[2] = 0xFFFFFFFF;
    atf_init_arg->devinfo[3] = 0xFFFFFFFF;

    DBG_MSG("%s HRID[0] : 0x%x\n", MOD, atf_init_arg->HRID[0]);
    DBG_MSG("%s HRID[1] : 0x%x\n", MOD, atf_init_arg->HRID[1]);
    DBG_MSG("%s atf_log_port : 0x%x\n", MOD, atf_init_arg->atf_log_port);
    DBG_MSG("%s atf_log_baudrate : 0x%x\n", MOD, atf_init_arg->atf_log_baudrate);
    DBG_MSG("%s atf_irq_num : %d\n", MOD, atf_init_arg->atf_irq_num);

#if CFG_TRUSTONIC_TEE_SUPPORT
    tbase_secmem_param_prepare(TEE_PARAMETER_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE,
        tee_secmem_size);
    tbase_boot_param_prepare(TEE_BOOT_ARG_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE,
        CFG_DRAM_ADDR, CFG_PLATFORM_DRAM_SIZE);
    atf_init_arg->tee_support = 1;
#else
    atf_init_arg->tee_support = 0;
#endif

#if CFG_ATF_LOG_SUPPORT
    atf_init_arg->atf_log_buf_start = atf_log_buf_start;
    atf_init_arg->atf_log_buf_size = ATF_LOG_BUFFER_SIZE;
    atf_init_arg->atf_aee_debug_buf_start = (atf_log_buf_start + ATF_LOG_BUFFER_SIZE - ATF_AEE_BUFFER_SIZE);
    atf_init_arg->atf_aee_debug_buf_size = ATF_AEE_BUFFER_SIZE;
#else
    atf_init_arg->atf_log_buf_start = 0;
    atf_init_arg->atf_log_buf_size = 0;
    atf_init_arg->atf_aee_debug_buf_start = 0;
    atf_init_arg->atf_aee_debug_buf_size = 0;
#endif
    DBG_MSG("%s ATF log buffer start : 0x%x\n", MOD, atf_init_arg->atf_log_buf_start);
    DBG_MSG("%s ATF log buffer size : 0x%x\n", MOD, atf_init_arg->atf_log_buf_size);
    DBG_MSG("%s ATF aee buffer start : 0x%x\n", MOD, atf_init_arg->atf_aee_debug_buf_start);
    DBG_MSG("%s ATF aee buffer size : 0x%x\n", MOD, atf_init_arg->atf_aee_debug_buf_size);
}

typedef void (*jump_atf)(u64 addr ,u64 arg1) __attribute__ ((__noreturn__));

void trustzone_jump(u32 addr, u32 arg1, u32 arg2)
{
    u32 bl31_reserve = 0;
    jump_atf atf_entry = (void *)addr;

    /* EMI MPU support */
    tee_sec_config();

#if CFG_TEE_SUPPORT
    DBG_MSG("%s Jump to ATF, then 0x%x and 0x%x\n", MOD, arg1, arg2);
#else
    DBG_MSG("%s Jump to ATF, then jump to bl33 0x%x\n", MOD, arg1);
#endif

    atf_entry = (jump_atf)BL31;
    DBG_MSG("[teeloader] teeloader jump to atf!\n");
    (*atf_entry)(ATF_BOOT_ARG_ADDR, bl31_reserve);
}

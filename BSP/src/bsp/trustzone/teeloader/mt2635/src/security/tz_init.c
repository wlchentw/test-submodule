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

#include "platform.h"
#include "print.h"
#include "sec_devinfo.h"
#include "string.h"
#include "typedefs.h"
#include "tz_emi_mpu.h"
#include "tz_init.h"
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
#define TEE_MEM_ALIGNMENT (0x1000)  //4K Alignment
#define TEE_ENABLE_VERIFY (1)

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
static u32 trustzone_get_atf_boot_param_addr(void)
{
    return ATF_BOOT_ARG_ADDR;
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
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)(void *)trustzone_get_atf_boot_param_addr();

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
    u32 secmem_end_addr = tee_entry_addr + tee_secmem_size - 1;

    tz_sec_mem_init(tee_entry_addr, secmem_end_addr, SECURE_OS_MPU_REGION_ID);
    DBG_MSG("%s set secure memory protection : 0x%x, 0x%x (%d)\n", MOD, tee_entry_addr,
        secmem_end_addr, SECURE_OS_MPU_REGION_ID);
#endif
#endif
    atf_entry_addr = atf_entry_addr & ~(EMI_MPU_ALIGNMENT - 1);

    DBG_MSG("%s ATF entry addr, aligned addr : 0x%x, 0x%x\n", MOD, BL31, atf_entry_addr);

    tz_sec_mem_init(atf_entry_addr, atf_entry_addr + BL31_SIZE - 1, ATF_MPU_REGION_ID);
    DBG_MSG("%s set secure memory protection : 0x%x, 0x%x (%d)\n", MOD, atf_entry_addr,
        atf_entry_addr + BL31_SIZE - 1, ATF_MPU_REGION_ID);
}

void trustzone_pre_init(void)
{
    tz_dapc_sec_init();
#if CFG_ATF_LOG_SUPPORT
    atf_log_buf_start = CFG_ATF_LOG_BUFFER_ADDR;
#if CFG_TEE_SUPPORT
    tee_secmem_size = CFG_TEE_SECMEM_SIZE;
#endif
#endif
}

void trustzone_post_init(void)
{
    atf_arg_t_ptr teearg = (atf_arg_t_ptr)(void *)trustzone_get_atf_boot_param_addr();

    teearg->atf_magic = ATF_BOOTCFG_MAGIC;
    teearg->tee_entry = tee_entry_addr;
    teearg->tee_boot_arg_addr = TEE_BOOT_ARG_ADDR;
    teearg->HRID[0] = seclib_get_devinfo_with_index(E_AREA12);
    teearg->HRID[1] = seclib_get_devinfo_with_index(E_AREA13);
    teearg->atf_log_port = 0x11002000;
    teearg->atf_log_baudrate = 0xE1000;
    teearg->atf_irq_num = (32 + 249); /* reserve SPI ID 249 for ATF log, which is ID 281 */

    //DBG_MSG("%s hwuid[0] : 0x%x\n", MOD, teearg->hwuid[0]);
    //DBG_MSG("%s hwuid[1] : 0x%x\n", MOD, teearg->hwuid[1]);
    //DBG_MSG("%s hwuid[2] : 0x%x\n", MOD, teearg->hwuid[2]);
    //DBG_MSG("%s hwuid[3] : 0x%x\n", MOD, teearg->hwuid[3]);
    DBG_MSG("%s HRID[0] : 0x%x\n", MOD, teearg->HRID[0]);
    DBG_MSG("%s HRID[1] : 0x%x\n", MOD, teearg->HRID[1]);
    DBG_MSG("%s atf_log_port : 0x%x\n", MOD, teearg->atf_log_port);
    DBG_MSG("%s atf_log_baudrate : 0x%x\n", MOD, teearg->atf_log_baudrate);
    DBG_MSG("%s atf_irq_num : %d\n", MOD, teearg->atf_irq_num);


#if CFG_TRUSTONIC_TEE_SUPPORT
    tbase_secmem_param_prepare(TEE_PARAMETER_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE,
        tee_secmem_size);
    tbase_boot_param_prepare(TEE_BOOT_ARG_ADDR, tee_entry_addr, CFG_TEE_CORE_SIZE,
        CFG_DRAM_ADDR, CFG_PLATFORM_DRAM_SIZE);
    teearg->tee_support = 1;
#else
    teearg->tee_support = 0;
#endif
    tz_dapc_sec_postinit();

#if CFG_ATF_LOG_SUPPORT
    teearg->atf_log_buf_start = atf_log_buf_start;
    teearg->atf_log_buf_size = ATF_LOG_BUFFER_SIZE;
    teearg->atf_aee_debug_buf_start = (atf_log_buf_start + ATF_LOG_BUFFER_SIZE - ATF_AEE_BUFFER_SIZE);
    teearg->atf_aee_debug_buf_size = ATF_AEE_BUFFER_SIZE;
#else
    teearg->atf_log_buf_start = 0;
    teearg->atf_log_buf_size = 0;
    teearg->atf_aee_debug_buf_start = 0;
    teearg->atf_aee_debug_buf_size = 0;
#endif
    DBG_MSG("%s ATF log buffer start : 0x%x\n", MOD, teearg->atf_log_buf_start);
    DBG_MSG("%s ATF log buffer size : 0x%x\n", MOD, teearg->atf_log_buf_size);
    DBG_MSG("%s ATF aee buffer start : 0x%x\n", MOD, teearg->atf_aee_debug_buf_start);
    DBG_MSG("%s ATF aee buffer size : 0x%x\n", MOD, teearg->atf_aee_debug_buf_size);
}

typedef void (*jump_atf)(u32 addr ,u32 arg1, u32 arg2, u32 arg3,
                         u32 arg4, u32 arg5, u32 arg6, u32 arg7) __attribute__ ((__noreturn__));

void trustzone_jump(u32 addr, u32 arg1, u32 arg2)
{
    jump_atf atf_entry = (void *)addr;

    tee_sec_config();

#if CFG_TEE_SUPPORT
    DBG_MSG("%s Jump to ATF, then 0x%x and 0x%x\n", MOD, arg1, arg2);
#else
    DBG_MSG("%s Jump to ATF, then jump 0x%x\n", MOD, arg1);
#endif

    DBG_MSG("[teeloader] tl jump to atf!\n");
    (*atf_entry)(0xC0000000, 0x10200038, 0x170,
        trustzone_get_atf_boot_param_addr(),
        0x4219C480, 0x170, arg1,
        trustzone_get_atf_boot_param_addr());
}

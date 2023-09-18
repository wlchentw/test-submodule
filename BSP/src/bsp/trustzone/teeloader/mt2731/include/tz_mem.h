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

#ifndef TZ_MEM_H
#define TZ_MEM_H

#include "tz_init.h"

#define SRAM_BASE_ADDRESS   (0x00100000UL)
#define SRAM_START_ADDR     (0x00102140UL)
#define VECTOR_START        (SRAM_START_ADDR + 0xBAC0UL)

typedef struct tz_memory_t {
    short next, previous;
} tz_memory_t;

#define FREE            ((short)(0x0001U))
#define IS_FREE(x)      ((x)->next & FREE)
#define CLEAR_FREE(x)   ((x)->next &= ~FREE)
#define SET_FREE(x)     ((x)->next |= FREE)
#define FROM_ADDR(x)    ((short)(ptrdiff_t)(x))
#define TO_ADDR(x)      ((tz_memory_t *)(SRAM_BASE_ADDRESS + ((x) & ~FREE)))

/* SEC MEM magic */
#define SEC_MEM_MAGIC                   (0x3C562817U)
/* SEC MEM version */
#define SEC_MEM_VERSION                 (0x00010000U)
/* Tplay Table Size */
#define SEC_MEM_TPLAY_TABLE_SIZE        (0x1000UL) //4KB by default
#define SEC_MEM_TPLAY_MEMORY_SIZE       (0x200000UL) //2MB by default

#define BL31                            (0x43001000UL)
#define BL31_SIZE                       (0x40000UL) //change to 256KB (192KB by default)
#define BL33                            (0x42110000UL)


#define ATF_BOOT_ARG_ADDR				(0x40000000UL)
#define ATF_INIT_ARG_ADDR				(0x40000100UL)
#define TEE_BOOT_ARG_ADDR				(0x43000100UL)

#define TEE_PARAMETER_BASE (TEE_BOOT_ARG_ADDR)
#define TEE_PARAMETER_ADDR (TEE_BOOT_ARG_ADDR + 0x100UL)

#define TEE_SECURE_ISRAM_ADDR           (0x0UL)
#define TEE_SECURE_ISRAM_SIZE           (0x0UL)

#if CFG_ATF_LOG_SUPPORT
#define ATF_LOG_BUFFER_SIZE (0x40000UL) //256KB
#define ATF_AEE_BUFFER_SIZE (0x4000UL) //16KB
#else
#define ATF_LOG_BUFFER_SIZE (0x0UL) //don't support ATF log
#define ATF_AEE_BUFFER_SIZE (0x0UL) //don't support ATF log
#endif

typedef struct {
    unsigned int magic;              // Magic number
    unsigned int version;            // version
    unsigned int svp_mem_start;      // MM sec mem pool start addr.
    unsigned int svp_mem_end;        // MM sec mem pool end addr.
    unsigned int tplay_table_start;  // tplay handle-to-physical table start
    unsigned int tplay_table_size;   // tplay handle-to-physical table size
    unsigned int tplay_mem_start;    // tplay physcial memory start address for crypto operation
    unsigned int tplay_mem_size;     // tplay phsycial memory size for crypto operation
    unsigned int secmem_obfuscation; // MM sec mem obfuscation or not
    unsigned int msg_auth_key[8];    /* size of message auth key is 32bytes(256 bits) */
    unsigned int rpmb_size;          /* size of rpmb partition */
    unsigned int emmc_rel_wr_sec_c;  // emmc ext_csd[222]
} sec_mem_arg_t;
#endif /* TZ_MEM_H */

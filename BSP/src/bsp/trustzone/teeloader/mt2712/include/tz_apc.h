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
 * MediaTek Inc. (C) 2018 All rights reserved.
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

#ifndef TZ_APC_H
#define TZ_APC_H

#include "typedefs.h"

/*****************************************************************************
 * Register base address definition
 *****************************************************************************/

#define SRAMROM_SEC_CTRL                ((volatile unsigned int*)(0x10001804U))
#define SRAMROM_SEC_ADDR                ((volatile unsigned int*)(0x10001808U))

/* APDMA */
#define APDMA_GLOBAL_GSEC_CTRL          ((volatile unsigned int*)(0x11000014U))
#define APDMA_UART_TX0_SEC_CTRL         ((volatile unsigned int*)(0x11000038U))
#define APDMA_UART_RX0_SEC_CTRL         ((volatile unsigned int*)(0x1100003CU))
#define APDMA_UART_TX1_SEC_CTRL         ((volatile unsigned int*)(0x11000040U))
#define APDMA_UART_RX1_SEC_CTRL         ((volatile unsigned int*)(0x11000044U))
#define APDMA_UART_TX2_SEC_CTRL         ((volatile unsigned int*)(0x11000048U))
#define APDMA_UART_RX2_SEC_CTRL         ((volatile unsigned int*)(0x1100004CU))
#define APDMA_UART_TX3_SEC_CTRL         ((volatile unsigned int*)(0x11000050U))
#define APDMA_UART_RX3_SEC_CTRL         ((volatile unsigned int*)(0x11000054U))
#define APDMA_UART_TX4_SEC_CTRL         ((volatile unsigned int*)(0x11000058U))
#define APDMA_UART_RX4_SEC_CTRL         ((volatile unsigned int*)(0x1100005CU))
#define APDMA_UART_TX5_SEC_CTRL         ((volatile unsigned int*)(0x11000060U))
#define APDMA_UART_RX5_SEC_CTRL         ((volatile unsigned int*)(0x11000064U))
#define APDMA_GLOBAL_GSEC_EN            0x1

/* CQDMA */
#define CQDMA_SEC_CTRL                  ((volatile unsigned int*)(0x10212C58U))

/* SMI BDPSYS (larb8 and larb9) */
#define SMI_BDPSYS_LARB8_BASE           ((volatile unsigned int*)(0x1501a000U))
#define SMI_BDPSYS_LARB9_BASE           ((volatile unsigned int*)(0x1501a008U))
#define SMI_BDPSYS_DOMAIN_MASK          (0xf0000)
#define SMI_BDPSYS_AR_DOMAIN(dom)       (((dom) & 0x3) << 16)
#define SMI_BDPSYS_AW_DOMAIN(dom)       (((dom) & 0x3) << 18)

/*****************************************************************************
 * Enum
 *****************************************************************************/
typedef enum
{
    TZ_APC_SEC_RW_NSEC_RW = 0,  /* read and write for both secure and non-secure access */
    TZ_APC_SEC_RW_NSEC_DENY,    /* read and write for secure access */
    TZ_APC_SEC_DENY_NSEC_RW,    /* read and write for non-secure access */
    TZ_APC_SEC_DENY_NSEC_DENY   /* Any access is prohibited */
} tz_apc_permission;

typedef enum
{
    TZ_APC_DOMAIN_IVI = 0,      /* The domain is for in-vehicle infotainment system (normally Linux OS).  */
    TZ_APC_DOMAIN_CLUSTER = 1,  /* The domain is for cluster system. */
    TZ_APC_DOMAIN_DSP = 2,      /* The domain is for Audio DSP system. */
    TZ_APC_DOMAIN_MCU = 3,      /* AP MCU will access the bus throgh the domain ID. The MCU used by any of the sub-system,
                                   including IVI, cluster, and DSP will access the bus with this domain.
                                   This domain can access almost all the slave devices in secure and non-secure mode and
                                   hence we must apply the MMU and MPU to protect the device access and memory access in
                                   the system. */
} tz_apc_domain_partition;

typedef enum
{
    TZ_SRAMROM_SEC_RW_NSEC_RW = 0,      /* read and write for both secure and non-secure access */
    TZ_SRAMROM_SEC_RW_NSEC_DENY = 1,    /* read and write for secure access */
    TZ_SRAMROM_SEC_RW_NSEC_RO = 2,      /* read and write for secure access and read only for non-secure access */
    TZ_SRAMROM_SEC_RW_NSEC_WO = 3,      /* read and write for secure access and write only for non-secure access */
    TZ_SRAMROM_SEC_RO_NSEC_RO = 4,      /* read only for both secure access and non-secure access */
    TZ_SRAMROM_SEC_DENY_NSEC_DENY = 7  /* Any access is prohibited */
} tz_sramrom_permission;

typedef enum
{
    TZ_UART_APDMA_NSEC = 0,    /* Read and write with non-secure sideband AXI signal. */
    TZ_UART_APDMA_SEC = 1,    /* Read and write with secure sideband AXI signal. */
} tz_uart_apdma_permission;

typedef enum
{
    TZ_CQDMA_NSEC = 0,    /* Read and write with non-secure sideband AXI signal. */
    TZ_CQDMA_SEC = 1,    /* Read and write with secure sideband AXI signal. */
} tz_cpdma_permission;

typedef enum
{
    TZ_SRAMROM_REGION_0 = 0,        /* Region 0 set by SRAMROM_SEC_ADD. Refer to TZ_SRAMROM_SET_REGION_SIZE for more info */
    TZ_SRAMROM_REGION_1 = 1         /* Region 1 set by SRAMROM_SEC_ADD. Refer to TZ_SRAMROM_SET_REGION_SIZE for more info */
} tz_sramrom_region;

/*****************************************************************************
 * Functions
 *****************************************************************************/

#define reg_read16(reg)        __raw_readw(reg)
#define reg_read32(reg)        __raw_readl(reg)
#define reg_write16(reg,val)   __raw_writew(val,reg)
#define reg_write32(reg,val)   __raw_writel(val,reg)

static inline u32 tz_sramrom_set_bitwise_domain_permision(tz_sramrom_region region,
    tz_apc_domain_partition domain, tz_sramrom_permission permission)
{
    return (permission & 0x7) << ((domain * 3) + (region == TZ_SRAMROM_REGION_1 ? 16: 0));
}

/* Enabling this bit to protect all multimedia secure related registers, including SMI,
   accessing in non-secure world. */
#define TZ_SRAMROM_ENABLE_MULTIMEDIA_SECURE_ACCESS (u32)(0x1 << 30)

/* Enabling this bit to protect sramrom region 1 by region 1's security setting */
#define TZ_SRAMROM_ENABLE_REGION_1_PROTECTION (u32)(0x1 << 28)

/* Set the region 0 size of the on-chip SRAM and the region 1 size will be (192KB - size_of_region_0). */
#define TZ_SRAMROM_SET_REGION_0_SIZE_KB(size) (reg_write32(SRAMROM_SEC_ADDR, ((size & 0xff) << 10)))

extern void tz_apc_common_init();
extern void tz_apc_common_postinit();

#endif /* TZ_APC_H */

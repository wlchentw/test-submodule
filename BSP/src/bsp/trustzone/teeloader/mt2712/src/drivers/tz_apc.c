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
 * MediaTek Inc. (C) 2018. All rights reserved.
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

/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/
#include "tz_apc.h"
#include "device_apc.h"
#include "print.h"

#define _DEBUG_
#define DBG_DEVAPC

/* Debug message event */
#define DBG_EVT_NONE       (0x00000000U)      /* No event */
#define DBG_EVT_ERR        (0x00000001U)      /* ERR related event */
#define DBG_EVT_DOM        (0x00000002U)      /* DOM related event */

#define DBG_EVT_ALL        (0xffffffffU)

#define DBG_EVT_MASK       (DBG_EVT_DOM)

#ifdef _DEBUG_
#define MSG(evt, fmt, args...) \
    do {    \
        if ((DBG_EVT_##evt) & DBG_EVT_MASK) { \
            print(fmt, ##args); \
        } \
    } while(0)

#define MSG_FUNC_ENTRY(f)   MSG(FUC, "<FUN_ENT>: %s\n", __FUNCTION__)
#else
#define MSG(evt, fmt, args...) do{}while(0)
#define MSG_FUNC_ENTRY(f)      do{}while(0)
#endif

/* MTK SMI bdpsys larb domain setting (larb8 and larb9) */
static void tz_apc_smi_bdpsys_sec_init()
{
    u32 reg, domain = 0;/* always be domain 0.*/

    /* Larb8 */
    reg = reg_read32(SMI_BDPSYS_LARB8_BASE);
    reg &= ~SMI_BDPSYS_DOMAIN_MASK;
    reg |= SMI_BDPSYS_AR_DOMAIN(domain) | SMI_BDPSYS_AW_DOMAIN(domain);
    reg_write32(SMI_BDPSYS_LARB8_BASE, reg);

    /* Larb9 */
    reg = reg_read32(SMI_BDPSYS_LARB9_BASE);
    reg &= ~SMI_BDPSYS_DOMAIN_MASK;
    reg |= SMI_BDPSYS_AR_DOMAIN(domain) | SMI_BDPSYS_AW_DOMAIN(domain);
    reg_write32(SMI_BDPSYS_LARB9_BASE, reg);
}

void tz_apc_common_init()
{
#ifdef CFG_TZ_SRAMROM_SUPPORT
    u32 domain_permission = 0U;
#endif /* CFG_TZ_SRAMROM_SUPPORT */

#ifdef CFG_TZ_SRAMROM_SUPPORT
    /* Enabling this bit to protect all multimedia secure related registers,
       including SMI, accessing in non-secure world. */
    domain_permission |= TZ_SRAMROM_ENABLE_MULTIMEDIA_SECURE_ACCESS;
    /* Enable this bit to protect sramrom region 1 by its security setting */
    domain_permission |= TZ_SRAMROM_ENABLE_REGION_1_PROTECTION;
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_0,
        TZ_APC_DOMAIN_IVI, TZ_SRAMROM_SEC_RW_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_0,
        TZ_APC_DOMAIN_CLUSTER, TZ_SRAMROM_SEC_DENY_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_0,
        TZ_APC_DOMAIN_DSP, TZ_SRAMROM_SEC_DENY_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_0,
        TZ_APC_DOMAIN_MCU, TZ_SRAMROM_SEC_RW_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_1,
        TZ_APC_DOMAIN_IVI, TZ_SRAMROM_SEC_DENY_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_1,
        TZ_APC_DOMAIN_CLUSTER, TZ_SRAMROM_SEC_DENY_NSEC_DENY);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_1,
        TZ_APC_DOMAIN_DSP, TZ_SRAMROM_SEC_RW_NSEC_RW);
    domain_permission |= tz_sramrom_set_bitwise_domain_permision(TZ_SRAMROM_REGION_1,
        TZ_APC_DOMAIN_MCU, TZ_SRAMROM_SEC_RW_NSEC_RW);
    reg_write32(SRAMROM_SEC_CTRL, domain_permission);
    TZ_SRAMROM_SET_REGION_0_SIZE_KB(96);
#endif /* CFG_TZ_SRAMROM_SUPPORT */

#ifdef CFG_TZ_UART_APDMA_SUPPORT

    /* GLOBAL GSEC_EN */
    reg_write32(APDMA_GLOBAL_GSEC_CTRL, reg_read32(APDMA_GLOBAL_GSEC_CTRL) | APDMA_GLOBAL_GSEC_EN);

#if 0
    /* Setting TX0/RX0 */
    domain_permission = reg_read32(APDMA_UART_TX0_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX0_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX0_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX0_SEC_CTRL, domain_permission);

    /* Setting TX1/RX1 */
    domain_permission = reg_read32(APDMA_UART_TX1_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX1_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX1_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX1_SEC_CTRL, domain_permission);

    /* Setting TX2/RX2 */
    domain_permission = reg_read32(APDMA_UART_TX2_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX2_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX2_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX2_SEC_CTRL, domain_permission);

    /* Setting TX3/RX3 */
    domain_permission = reg_read32(APDMA_UART_TX3_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX3_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX3_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX3_SEC_CTRL, domain_permission);
#endif

    /* Setting TX4/RX4 */
    domain_permission = reg_read32(APDMA_UART_TX4_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_IVI<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX4_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX4_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_IVI<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX4_SEC_CTRL, domain_permission);

#if 0
    /* Setting TX5/RX5 */
    domain_permission = reg_read32(APDMA_UART_TX5_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_TX5_SEC_CTRL, domain_permission);

    domain_permission = reg_read32(APDMA_UART_RX5_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_UART_APDMA_NSEC;
    reg_write32(APDMA_UART_RX5_SEC_CTRL, domain_permission);

    /* Setting CQDMA */
    domain_permission = reg_read32(CQDMA_SEC_CTRL);
    domain_permission |= TZ_APC_DOMAIN_CLUSTER<<1;
    domain_permission |= TZ_CQDMA_NSEC;
    reg_write32(CQDMA_SEC_CTRL, domain_permission);
#endif
#endif

    /* Set domain of masters and their transaction type.
       Set access permission of slaves in each domain. */
    tz_dapc_sec_setting();
}

void tz_apc_common_postinit()
{
    tz_apc_smi_bdpsys_sec_init();
}

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

/*=======================================================================*/
/* HEADER FILES                                                          */
/*=======================================================================*/
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
#define MSG(evt, fmt, args...) do{} while(0)
#define MSG_FUNC_ENTRY(f)      do{} while(0)
#endif

void tz_dapc_set_master_transaction(unsigned int  master_index , E_TRANSACTION permisssion_control)
{
    reg_set_field(DEVAPC0_MAS_SEC_GROUP_0 , (0x1 << master_index), permisssion_control);
}

void tz_dapc_sec_init(void)
{
    /* tz_dapc_set_master_transaction(MASTER_MSDC0 , SECURE_TRAN); */
}

void tz_dapc_sec_postinit(void)
{
    /* tz_dapc_set_master_transaction(MASTER_MSDC0 , NON_SECURE_TRAN); */
}

void device_APC_dom_setup(void)
{
    MSG(DOM, "\nDevice APC domain init setup:\n\n");
    reg_write32(DEVAPC0_APC_CON, 0x0);

#ifdef DBG_DEVAPC
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_0));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_1));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_2));
#endif
    /*Set masters to DOMAINX here if needed*/
#ifdef DBG_DEVAPC
    MSG(DOM, "Device APC domain after setup:\n");
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_0));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_1));
    MSG(DOM, "Domain Setup (0x%x)\n", reg_read32(DEVAPC0_MAS_DOM_GROUP_2));
#endif
}

void tz_dapc_sec_setting(void)
{
    /* unmask debug bit for setting other DAPC registers */
    reg_write32(DEVAPC0_APC_CON, 0x0);

    /* Set domain of masters */
    DAPC_SET_MASTER_DOMAIN(
        DEVAPC0_MAS_DOM_GROUP_0,
        MODULE_DOMAIN(MASTER_NFI,          DOMAIN_0) |
        MODULE_DOMAIN(MASTER_PWM,          DOMAIN_0) |
        MODULE_DOMAIN(MASTER_THERMAL_CTRL, DOMAIN_0) |
        MODULE_DOMAIN(MASTER_MSDC0,        DOMAIN_0) |
        MODULE_DOMAIN(MASTER_MSDC1,        DOMAIN_0) |
        MODULE_DOMAIN(MASTER_MSDC2,        DOMAIN_0) |
        MODULE_DOMAIN(MASTER_MSDC3,        DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI0,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPM,          DOMAIN_0) |
        MODULE_DOMAIN(MASTER_DEBUG_SYSTEM, DOMAIN_0) |
        MODULE_DOMAIN(MASTER_AUDIO_AFE,    DOMAIN_2) |
        MODULE_DOMAIN(MASTER_APMCU,        DOMAIN_3)
    );

    DAPC_SET_MASTER_DOMAIN(
        DEVAPC0_MAS_DOM_GROUP_1,
        MODULE_DOMAIN(MASTER_MFG_M0,       DOMAIN_1) |
        MODULE_DOMAIN(MASTER_USB30,        DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI1,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI2,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI3,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI4,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SPI5,         DOMAIN_0) |
#ifdef CFG_TINYSYS_SCP_SUPPORT
        MODULE_DOMAIN(MASTER_SCP,          DOMAIN_3) |
#else
        MODULE_DOMAIN(MASTER_SCP,          DOMAIN_1) |
#endif
        MODULE_DOMAIN(MASTER_USB30_2,      DOMAIN_0) |
        MODULE_DOMAIN(MASTER_SFLASH,       DOMAIN_0) |
        MODULE_DOMAIN(MASTER_GMAC,         DOMAIN_0) |
        MODULE_DOMAIN(MASTER_PCIE0,        DOMAIN_0)
    );

    DAPC_SET_MASTER_DOMAIN(
        DEVAPC0_MAS_DOM_GROUP_2,
        MODULE_DOMAIN(MASTER_PCIE1,       DOMAIN_0)
    );

    /* Set the transaction type of masters*/
    DAPC_SET_MASTER_TRANSACTION(
        DEVAPC0_MAS_SEC_GROUP_0,
        MODULE_TRANSACTION(MASTER_NFI,          DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_PWM,          DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_THERMAL_CTRL, DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_MSDC0,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_MSDC1,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_MSDC2,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_MSDC3,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI0,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPM,          DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_DEBUG_SYSTEM, DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_AUDIO_AFE,    DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_APMCU,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_MFG_M0,       DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_USB30,        DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI1,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI2,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI3,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI4,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SPI5,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SCP,           DAPC_S_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_USB30_2,      DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_SFLASH,       DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_GMAC,         DAPC_NS_TRANSACTION) |
        MODULE_TRANSACTION(MASTER_PCIE0,        DAPC_NS_TRANSACTION)
    );

    DAPC_SET_MASTER_TRANSACTION(
        DEVAPC0_MAS_SEC_GROUP_1,
        MODULE_TRANSACTION(MASTER_PCIE1,        DAPC_NS_TRANSACTION)
    );

   /* Set the access permission of slaves in domain 0*/
    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_0,
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_INFRASYS_CONFIG_REGS       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_PERISYS_CONFIG_REGS        ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_GPIO_CONTROLLER            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_SLP_MANAGER      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_RESET_GENERATOR  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_GPT                        ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_SEJ                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_APMCU_EINT_CONTROLLER      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SYS_TIMER_CONTROL_REG               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IRRX_CONTROL_REG                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO              ,DAPC_SEC_RW) |
        MODULE_PERMISSION(UART5_REG                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_1,
        MODULE_PERMISSION(INFRA_AO_KPAD_CONTROL_REG           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(TOP_RTC_REG                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI4_REG                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI1_REG                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_GPT2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH0_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH1_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH2_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH3_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_MCUSYS_CONFIG_REG          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_CONTROL_REG                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_BOOTROM_SRAM               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EMI_BUS_INTERFACE          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_SYSTEM_CIRQ                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_M4U_CONFIGURATION          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EFUSEC                     ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_2,
        MODULE_PERMISSION(INFRASYS_DEVICE_APC_MONITOR         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BUS_DEBUG_TRAKER                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_AP_MIXED_CONTROL_REG       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_M4U_2_CONFIGURATION        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI3                       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRASYS_MBIST_CONTROL_REG          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EMI_MPU_CONTROL_REG        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_TRNG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU_NS                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_CQ_DMA                     ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU_M4U                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI1                       ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_3,
        MODULE_PERMISSION(ANA_MIPI_CSI0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_CSI1                       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DEGBUG_CORESIGHT                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DMA                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(AUXADC                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART1                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART2                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART3                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(PWM                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C1                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(THERM_CTRL                          ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_4,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SPI_NOR                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(NFI                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(NFI_ECC                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C3                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C4                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(I2C5                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SPI2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI3                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(UART4                               ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(GMAC                                ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_5,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(AUDIO                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC1                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC2                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC3                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0_2                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF_2                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF2_2                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SCPSYS_SRAM                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PCIe0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(PCIe1                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_6,
        MODULE_PERMISSION(G3D_CONFIG                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MMSYS_CONFIG                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ0                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ2                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WDMA                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_TDSHP0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_TDSHP1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_OVL0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OVL1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_RDMA0                          ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_7,
        MODULE_PERMISSION(DISP_RDMA1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_RDMA2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_AAL                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_GAMMA                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_SPLIT0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_UFOE                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DPI                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_PWM0                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_8,
        MODULE_PERMISSION(DISP_PWM1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MM_MUTEX                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OD                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DPI1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(LVDS                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB4                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR2                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_AAL1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OD1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OVL2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(LVDS1                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_9,
        MODULE_PERMISSION(MDP_TDSHP2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB5                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB7                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA3                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_MONITOR0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR1                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR3                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_PWM2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IMGSYS_CONFIG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB2                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_10,
        MODULE_PERMISSION(SENINF_TOP0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SENINF_TOP1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP3                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP4                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP5                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(BDP_DISPSYS_CONFIG                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_DISPFMT                         ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_11,
        MODULE_PERMISSION(BDP_VDO                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_NR                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_NR2                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_TVD                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_DI                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_VDI                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_LARB                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_LARB_RT                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_DRAM2AXI_BRIDGE                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDECSYS_CONFIGURATION               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDECSYS_SMI_LARB1                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDEC_FULL_TOP                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IMGRZ                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDEC_MBIST                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(JPGDEC_CONFIGURATION                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(JPDEC                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(
        DEVAPC0_D0_APC_12,
        MODULE_PERMISSION(JPDGDEC1                            ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(VENC_CONFIGURATION                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC_SMI_LARB3                      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC_SMI_LARB6                      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON_2                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SFLASH                              ,DAPC_SEC_RW_NSEC_RW)
    );

    /* Set the access permission of slaves in domain 1*/
    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_0,
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_INFRASYS_CONFIG_REGS       ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_PERISYS_CONFIG_REGS        ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_GPIO_CONTROLLER            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_SLP_MANAGER      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_RESET_GENERATOR  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_GPT                        ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_SEJ                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_APMCU_EINT_CONTROLLER      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SYS_TIMER_CONTROL_REG               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IRRX_CONTROL_REG                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART5_REG                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_1,
        MODULE_PERMISSION(INFRA_AO_KPAD_CONTROL_REG           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(TOP_RTC_REG                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI4_REG                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI1_REG                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_GPT2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH0_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH1_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH2_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH3_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_MCUSYS_CONFIG_REG          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_CONTROL_REG                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_BOOTROM_SRAM               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EMI_BUS_INTERFACE          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_SYSTEM_CIRQ                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_M4U_CONFIGURATION          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EFUSEC                     ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_2,
        MODULE_PERMISSION(INFRASYS_DEVICE_APC_MONITOR         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BUS_DEBUG_TRAKER                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_AP_MIXED_CONTROL_REG       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_M4U_2_CONFIGURATION        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI3                       ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRASYS_MBIST_CONTROL_REG          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EMI_MPU_CONTROL_REG        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_TRNG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU_NS                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_CQ_DMA                     ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU_M4U                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI1                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_CSI0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_CSI1                       ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_3,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DEGBUG_CORESIGHT                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DMA                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(AUXADC                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART1                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART2                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART3                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PWM                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(THERM_CTRL                          ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_4,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SPI_NOR                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(NFI                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(NFI_ECC                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C4                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(I2C5                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SPI2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(UART4                               ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(GMAC                                ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_5,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(AUDIO                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC1                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC2                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC3                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0_2                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF_2                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF2_2                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SCPSYS_SRAM                         ,DAPC_SEC_RW) |
        MODULE_PERMISSION(PCIe0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PCIe1                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_6,
        MODULE_PERMISSION(G3D_CONFIG                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MMSYS_CONFIG                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ0                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ1                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ2                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WDMA                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_TDSHP0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_TDSHP1                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_OVL0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OVL1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_RDMA0                          ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_7,
        MODULE_PERMISSION(DISP_RDMA1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_RDMA2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR0                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_AAL                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_GAMMA                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_SPLIT0                         ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_UFOE                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI1                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DPI                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_PWM0                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_8,
        MODULE_PERMISSION(DISP_PWM1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MM_MUTEX                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OD                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DPI1                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(LVDS                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB4                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR2                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_AAL1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OD1                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OVL2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(LVDS1                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_9,
        MODULE_PERMISSION(MDP_TDSHP2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB5                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB7                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA3                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI3                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_MONITOR0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR1                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR3                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_PWM2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IMGSYS_CONFIG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB2                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_10,
        MODULE_PERMISSION(SENINF_TOP0                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SENINF_TOP1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP3                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP4                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP5                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(BDP_DISPSYS_CONFIG                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_DISPFMT                         ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_11,
        MODULE_PERMISSION(BDP_VDO                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_NR                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_NR2                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_TVD                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_DI                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_VDI                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_LARB                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_LARB_RT                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_DRAM2AXI_BRIDGE                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDECSYS_CONFIGURATION               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDECSYS_SMI_LARB1                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDEC_FULL_TOP                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IMGRZ                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDEC_MBIST                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(JPGDEC_CONFIGURATION                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(JPDEC                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(
        DEVAPC0_D1_APC_12,
        MODULE_PERMISSION(JPDGDEC1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(VENC_CONFIGURATION                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC_SMI_LARB3                      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC_SMI_LARB6                      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON_2                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SFLASH                              ,DAPC_SEC_DENY_NSEC_DENY)
    );

    /* Set the access permission of slaves in domain 2*/
    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_0,
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_INFRASYS_CONFIG_REGS       ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_PERISYS_CONFIG_REGS        ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_GPIO_CONTROLLER            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_SLP_MANAGER      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_RESET_GENERATOR  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_GPT                        ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRA_AO_SEJ                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_APMCU_EINT_CONTROLLER      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SYS_TIMER_CONTROL_REG               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IRRX_CONTROL_REG                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART5_REG                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_1,
        MODULE_PERMISSION(INFRA_AO_KPAD_CONTROL_REG           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(TOP_RTC_REG                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI4_REG                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI1_REG                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRA_AO_GPT2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH0_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH1_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH2_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DRAMC_CH3_REG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_MCUSYS_CONFIG_REG          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_CONTROL_REG                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_BOOTROM_SRAM               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EMI_BUS_INTERFACE          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_SYSTEM_CIRQ                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_M4U_CONFIGURATION          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EFUSEC                     ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_2,
        MODULE_PERMISSION(INFRASYS_DEVICE_APC_MONITOR         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BUS_DEBUG_TRAKER                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_AP_MIXED_CONTROL_REG       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_M4U_2_CONFIGURATION        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI3                       ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(INFRASYS_MBIST_CONTROL_REG          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_EMI_MPU_CONTROL_REG        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_TRNG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU_NS                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_CQ_DMA                     ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(INFRASYS_GCPU_M4U                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_DSI1                       ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_3,
        MODULE_PERMISSION(ANA_MIPI_CSI0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(ANA_MIPI_CSI1                       ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DEGBUG_CORESIGHT                    ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DMA                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(AUXADC                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART1                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART2                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(UART3                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PWM                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(THERM_CTRL                          ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_4,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SPI_NOR                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(NFI                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(NFI_ECC                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(I2C4                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(I2C5                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SPI2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SPI3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(UART4                               ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(GMAC                                ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_5,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(AUDIO                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC1                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC2                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MSDC3                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0_2                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF_2                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(USB3_0SIF2_2                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SCPSYS_SRAM                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PCIe0                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PCIe1                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_6,
        MODULE_PERMISSION(G3D_CONFIG                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MMSYS_CONFIG                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ0                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RSZ2                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WDMA                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_TDSHP0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_TDSHP1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_OVL0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OVL1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_RDMA0                          ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_7,
        MODULE_PERMISSION(DISP_RDMA1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_RDMA2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_AAL                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_GAMMA                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_SPLIT0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_UFOE                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI0                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DPI                                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_PWM0                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_8,
        MODULE_PERMISSION(DISP_PWM1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MM_MUTEX                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB0                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OD                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DPI1                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(LVDS                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB4                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_COLOR2                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_AAL1                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OD1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_OVL2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_WDMA2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(LVDS1                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_9,
        MODULE_PERMISSION(MDP_TDSHP2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB5                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB7                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_RDMA3                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(MDP_WROT2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI2                                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DSI3                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(DISP_MONITOR0                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR1                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR2                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_MONITOR3                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(DISP_PWM2                           ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IMGSYS_CONFIG                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_LARB2                           ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_10,
        MODULE_PERMISSION(SENINF_TOP0                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SENINF_TOP1                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP0                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP1                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP2                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP3                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP4                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(CAMSV_TOP5                          ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(BDP_DISPSYS_CONFIG                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_DISPFMT                         ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_11,
        MODULE_PERMISSION(BDP_VDO                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_NR                              ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_NR2                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_TVD                             ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_DI                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_VDI                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_LARB                            ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_LARB_RT                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(BDP_DRAM2AXI_BRIDGE                 ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDECSYS_CONFIGURATION               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDECSYS_SMI_LARB1                   ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDEC_FULL_TOP                       ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(IMGRZ                               ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VDEC_MBIST                          ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(JPGDEC_CONFIGURATION                ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(JPDEC                               ,DAPC_SEC_DENY_NSEC_DENY)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(
        DEVAPC0_D2_APC_12,
        MODULE_PERMISSION(JPDGDEC1                            ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(VENC_CONFIGURATION                  ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC_SMI_LARB3                      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC_SMI_LARB6                      ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(SMI_COMMON_2                        ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(VENC                                ,DAPC_SEC_DENY_NSEC_DENY) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_DENY_NSEC_DENY) | */
        MODULE_PERMISSION(SFLASH                              ,DAPC_SEC_DENY_NSEC_DENY)
    );

    /* Set the access permission of slaves in domain 3*/
    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_0,
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_INFRASYS_CONFIG_REGS       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_PERISYS_CONFIG_REGS        ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_GPIO_CONTROLLER            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_SLP_MANAGER      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_TOP_LEVEL_RESET_GENERATOR  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_GPT                        ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRA_AO_SEJ                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_APMCU_EINT_CONTROLLER      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SYS_TIMER_CONTROL_REG               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IRRX_CONTROL_REG                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_DEVICE_APC_AO              ,DAPC_SEC_RW) |
        MODULE_PERMISSION(UART5_REG                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_1,
        MODULE_PERMISSION(INFRA_AO_KPAD_CONTROL_REG           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(TOP_RTC_REG                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI4_REG                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI1_REG                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRA_AO_GPT2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH0_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH1_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH2_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DRAMC_CH3_REG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_MCUSYS_CONFIG_REG          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_CONTROL_REG                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_BOOTROM_SRAM               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EMI_BUS_INTERFACE          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_SYSTEM_CIRQ                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_M4U_CONFIGURATION          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EFUSEC                     ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_2,
        MODULE_PERMISSION(INFRASYS_DEVICE_APC_MONITOR         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BUS_DEBUG_TRAKER                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_AP_MIXED_CONTROL_REG       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_M4U_2_CONFIGURATION        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI3                       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(INFRASYS_MBIST_CONTROL_REG          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_EMI_MPU_CONTROL_REG        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_TRNG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU_NS                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_CQ_DMA                     ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(INFRASYS_GCPU_M4U                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_DSI1                       ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_3,
        MODULE_PERMISSION(ANA_MIPI_CSI0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(ANA_MIPI_CSI1                       ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DEGBUG_CORESIGHT                    ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DMA                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(AUXADC                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART1                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART2                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(UART3                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(PWM                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C1                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(THERM_CTRL                          ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_4,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SPI_NOR                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(NFI                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(NFI_ECC                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C3                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(I2C4                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(I2C5                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SPI2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SPI3                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(UART4                               ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(GMAC                                ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_5,
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(AUDIO                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC1                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC2                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MSDC3                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0_2                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF_2                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(USB3_0SIF2_2                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SCPSYS_SRAM                         ,DAPC_SEC_DENY_NSEC_DENY) |
        MODULE_PERMISSION(PCIe0                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(PCIe1                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_6,
        MODULE_PERMISSION(G3D_CONFIG                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MMSYS_CONFIG                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ0                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ1                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RSZ2                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WDMA                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_TDSHP0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_TDSHP1                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_OVL0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OVL1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_RDMA0                          ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_7,
        MODULE_PERMISSION(DISP_RDMA1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_RDMA2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR0                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_AAL                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_GAMMA                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_SPLIT0                         ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_UFOE                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI0                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI1                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DPI                                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_PWM0                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_8,
        MODULE_PERMISSION(DISP_PWM1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MM_MUTEX                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB0                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OD                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DPI1                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(LVDS                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB4                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_COLOR2                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_AAL1                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OD1                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_OVL2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_WDMA2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(LVDS1                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_9,
        MODULE_PERMISSION(MDP_TDSHP2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB5                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB7                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_RDMA3                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(MDP_WROT2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI2                                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DSI3                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(DISP_MONITOR0                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR1                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR2                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_MONITOR3                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(DISP_PWM2                           ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IMGSYS_CONFIG                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_LARB2                           ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_10,
        MODULE_PERMISSION(SENINF_TOP0                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SENINF_TOP1                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP0                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP1                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP2                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP3                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP4                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(CAMSV_TOP5                          ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(BDP_DISPSYS_CONFIG                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_DISPFMT                         ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_11,
        MODULE_PERMISSION(BDP_VDO                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_NR                              ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_NR2                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_TVD                             ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_DI                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_WR_CHANNEL_VDI                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_LARB                            ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_LARB_RT                         ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(BDP_DRAM2AXI_BRIDGE                 ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDECSYS_CONFIGURATION               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDECSYS_SMI_LARB1                   ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDEC_FULL_TOP                       ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(IMGRZ                               ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VDEC_MBIST                          ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(JPGDEC_CONFIGURATION                ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(JPDEC                               ,DAPC_SEC_RW_NSEC_RW)
    );

    DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(
        DEVAPC0_D3_APC_12,
        MODULE_PERMISSION(JPDGDEC1                            ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(VENC_CONFIGURATION                  ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC_SMI_LARB3                      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC_SMI_LARB6                      ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(SMI_COMMON_2                        ,DAPC_SEC_RW_NSEC_RW) |
        MODULE_PERMISSION(VENC                                ,DAPC_SEC_RW_NSEC_RW) |
        /* MODULE_PERMISSION(Reserved                            ,DAPC_SEC_RW_NSEC_RW) | */
        MODULE_PERMISSION(SFLASH                              ,DAPC_SEC_RW_NSEC_RW)
    );
}

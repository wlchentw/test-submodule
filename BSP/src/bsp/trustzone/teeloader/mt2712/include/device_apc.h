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

#ifndef DEVICE_APC_H
#define DEVICE_APC_H

#include "typedefs.h"

#define DEVAPC0_AO_BASE         (0x1000E000U)
#define DEVAPC0_PD_BASE         (0x10207000U)

/*******************************************************************************
 * REGISTER ADDRESS DEFINATION
 ******************************************************************************/
#define DEVAPC0_D0_APC_0        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0000))
#define DEVAPC0_D0_APC_1        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0004))
#define DEVAPC0_D0_APC_2        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0008))
#define DEVAPC0_D0_APC_3        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x000C))
#define DEVAPC0_D0_APC_4        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0010))
#define DEVAPC0_D0_APC_5        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0014))
#define DEVAPC0_D0_APC_6        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0018))
#define DEVAPC0_D0_APC_7        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x001C))
#define DEVAPC0_D0_APC_8        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0020))
#define DEVAPC0_D0_APC_9        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0020))
#define DEVAPC0_D0_APC_10       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0024))
#define DEVAPC0_D0_APC_11       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0028))
#define DEVAPC0_D0_APC_12       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0030))
#define DEVAPC0_D1_APC_0        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0100))
#define DEVAPC0_D1_APC_1        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0104))
#define DEVAPC0_D1_APC_2        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0108))
#define DEVAPC0_D1_APC_3        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x010C))
#define DEVAPC0_D1_APC_4        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0110))
#define DEVAPC0_D1_APC_5        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0114))
#define DEVAPC0_D1_APC_6        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0118))
#define DEVAPC0_D1_APC_7        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x011C))
#define DEVAPC0_D1_APC_8        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0120))
#define DEVAPC0_D1_APC_9        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0120))
#define DEVAPC0_D1_APC_10       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0124))
#define DEVAPC0_D1_APC_11       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0128))
#define DEVAPC0_D1_APC_12       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0130))
#define DEVAPC0_D2_APC_0        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0200))
#define DEVAPC0_D2_APC_1        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0204))
#define DEVAPC0_D2_APC_2        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0208))
#define DEVAPC0_D2_APC_3        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x020C))
#define DEVAPC0_D2_APC_4        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0210))
#define DEVAPC0_D2_APC_5        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0214))
#define DEVAPC0_D2_APC_6        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0218))
#define DEVAPC0_D2_APC_7        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x021C))
#define DEVAPC0_D2_APC_8        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0220))
#define DEVAPC0_D2_APC_9        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0220))
#define DEVAPC0_D2_APC_10       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0224))
#define DEVAPC0_D2_APC_11       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0228))
#define DEVAPC0_D2_APC_12       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0230))
#define DEVAPC0_D3_APC_0        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0300))
#define DEVAPC0_D3_APC_1        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0304))
#define DEVAPC0_D3_APC_2        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0308))
#define DEVAPC0_D3_APC_3        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x030C))
#define DEVAPC0_D3_APC_4        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0310))
#define DEVAPC0_D3_APC_5        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0314))
#define DEVAPC0_D3_APC_6        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0318))
#define DEVAPC0_D3_APC_7        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x031C))
#define DEVAPC0_D3_APC_8        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0320))
#define DEVAPC0_D3_APC_9        ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0320))
#define DEVAPC0_D3_APC_10       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0324))
#define DEVAPC0_D3_APC_11       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0328))
#define DEVAPC0_D3_APC_12       ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0330))
#define DEVAPC0_MAS_DOM_GROUP_0 ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0400))
#define DEVAPC0_MAS_DOM_GROUP_1 ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0404))
#define DEVAPC0_MAS_DOM_GROUP_2 ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0408))
#define DEVAPC0_MAS_SEC_GROUP_0 ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0500))
#define DEVAPC0_MAS_SEC_GROUP_1 ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0504))
#define DEVAPC0_APC_CON         ((volatile unsigned int*)(DEVAPC0_AO_BASE+0x0F00))
#define DEVAPC0_PD_APC_CON      ((volatile unsigned int*)(DEVAPC0_PD_BASE+0x0F00))
#define DEVAPC_APC_CON_CTRL     (0x1U)
#define DEVAPC_APC_CON_EN       (0x1U)
#define MASTER_MSDC0            4U

typedef enum {
    NON_SECURE_TRAN = 0,
    SECURE_TRAN,
} E_TRANSACTION;


///* DOMAIN_SETUP */
#define DOMAIN_0  0U
#define DOMAIN_1  1U
#define DOMAIN_2  2U
#define DOMAIN_3  3U
#define CONN2AP  (0xf << 16)//index12   DEVAPC0_MAS_DOM_1
#define GPU      (0xf << 20)//index21   DEVAPC0_MAS_DOM_2

static inline unsigned int uffs(unsigned int x)
{
    unsigned int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

#define reg_read16(reg)        __raw_readw(reg)
#define reg_read32(reg)        __raw_readl(reg)
#define reg_write16(reg,val)   __raw_writew(val,reg)
#define reg_write32(reg,val)   __raw_writel(val,reg)

#define reg_set_bits(reg,bs)   ((*(volatile u32*)(reg)) |= (u32)(bs))
#define reg_clr_bits(reg,bs)   ((*(volatile u32*)(reg)) &= ~((u32)(bs)))

#define reg_set_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = reg_read32(reg); \
        tv &= ~(field); \
        tv |= ((val) << (uffs((unsigned int)field) - 1)); \
        reg_write32(reg,tv); \
    } while(0)

#define reg_get_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = reg_read32(reg); \
        val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
    } while(0)

#define DAPC_SEC_RW_NSEC_RW     0U /* read and write for both secure and non-secure access */
#define DAPC_SEC_RW             1U /* read and write for secure access */
#define DAPC_NSEC_RW            2U /* read and write for non-secure access */
#define DAPC_SEC_DENY_NSEC_DENY 3U /* Any access is prohibited */

#define DAPC_NS_TRANSACTION 0U /* Emit non-secure signal sideband */
#define DAPC_S_TRANSACTION  1U /* Emit secure signal sideband */

#define MASTER_NFI           0U
#define MASTER_PWM           2U
#define MASTER_THERMAL_CTRL  3U
#define MASTER_MSDC0         4U
#define MASTER_MSDC1         5U
#define MASTER_MSDC2         6U
#define MASTER_MSDC3         7U
#define MASTER_SPI0          8U
#define MASTER_SPM           9U
#define MASTER_DEBUG_SYSTEM 11U
#define MASTER_AUDIO_AFE    12U
#define MASTER_APMCU        13U
#define MASTER_MFG_M0       19U
#define MASTER_USB30        20U
#define MASTER_SPI1         22U
#define MASTER_SPI2         23U
#define MASTER_SPI3         24U
#define MASTER_SPI4         25U
#define MASTER_SPI5         26U
#define MASTER_SCP          27U
#define MASTER_USB30_2      28U
#define MASTER_SFLASH       29U
#define MASTER_GMAC         30U
#define MASTER_PCIE0        31U
#define MASTER_PCIE1        32U

#define MODULE_TRANSACTION(index, is_secure) (is_secure << index)
#define DAPC_SET_MASTER_TRANSACTION(devapc_register, is_secure) reg_write32(devapc_register, is_secure)

#define MODULE_DOMAIN(index, domain) (domain << (2 * (index % 16)))
#define DAPC_SET_MASTER_DOMAIN(devapc_register, domain) reg_write32(devapc_register, domain)

#define MODULE_PERMISSION(index, permission) (permission << (2 * (index % 16)))
#define DAPC_SET_SLAVE_PERMISSION_DOMAIN_0(devapc_register, permission) reg_write32(devapc_register, permission)
#define DAPC_SET_SLAVE_PERMISSION_DOMAIN_1(devapc_register, permission) reg_write32(devapc_register, permission)
#define DAPC_SET_SLAVE_PERMISSION_DOMAIN_2(devapc_register, permission) reg_write32(devapc_register, permission)
#define DAPC_SET_SLAVE_PERMISSION_DOMAIN_3(devapc_register, permission) reg_write32(devapc_register, permission)

#define INFRA_AO_TOP_LEVEL_CLOCK_GENERATOR         0U
#define INFRA_AO_INFRASYS_CONFIG_REGS              1U
/* #define Reserved                                   2U */
#define INFRA_AO_PERISYS_CONFIG_REGS               3U
/* #define Reserved                                   4U */
#define INFRA_AO_GPIO_CONTROLLER                   5U
#define INFRA_AO_TOP_LEVEL_SLP_MANAGER             6U
#define INFRA_AO_TOP_LEVEL_RESET_GENERATOR         7U
#define INFRA_AO_GPT                               8U
/* #define Reserved                                   9U */
#define INFRA_AO_SEJ                               10U
#define INFRA_AO_APMCU_EINT_CONTROLLER             11U
#define SYS_TIMER_CONTROL_REG                      12U
#define IRRX_CONTROL_REG                           13U
#define INFRA_AO_DEVICE_APC_AO                     14U
#define UART5_REG                                  15U
#define INFRA_AO_KPAD_CONTROL_REG                  16U
#define TOP_RTC_REG                                17U
#define SPI4_REG                                   18U
#define SPI1_REG                                   19U
#define INFRA_AO_GPT2                              20U
#define DRAMC_CH0_REG                              21U
#define DRAMC_CH1_REG                              22U
#define DRAMC_CH2_REG                              23U
#define DRAMC_CH3_REG                              24U
#define INFRASYS_MCUSYS_CONFIG_REG                 25U
#define INFRASYS_CONTROL_REG                       26U
#define INFRASYS_BOOTROM_SRAM                      27U
#define INFRASYS_EMI_BUS_INTERFACE                 28U
#define INFRASYS_SYSTEM_CIRQ                       29U
#define INFRASYS_M4U_CONFIGURATION                 30U
#define INFRASYS_EFUSEC                            31U
#define INFRASYS_DEVICE_APC_MONITOR                32U
#define BUS_DEBUG_TRAKER                           33U
#define INFRASYS_AP_MIXED_CONTROL_REG              34U
#define INFRASYS_M4U_2_CONFIGURATION               35U
#define ANA_MIPI_DSI3                              36U
/* #define Reserved                                   37U */
#define INFRASYS_MBIST_CONTROL_REG                 38U
#define INFRASYS_EMI_MPU_CONTROL_REG               39U
#define INFRASYS_TRNG                              40U
#define INFRASYS_GCPU                              41U
#define INFRASYS_GCPU_NS                           42U
#define INFRASYS_CQ_DMA                            43U
#define INFRASYS_GCPU_M4U                          44U
#define ANA_MIPI_DSI2                              45U
#define ANA_MIPI_DSI0                              46U
#define ANA_MIPI_DSI1                              47U
#define ANA_MIPI_CSI0                              48U
#define ANA_MIPI_CSI1                              49U
/* #define Reserved                                   50U */
#define DEGBUG_CORESIGHT                           51U
#define DMA                                        52U
#define AUXADC                                     53U
#define UART0                                      54U
#define UART1                                      55U
#define UART2                                      56U
#define UART3                                      57U
#define PWM                                        58U
#define I2C0                                       59U
#define I2C1                                       60U
#define I2C2                                       61U
#define SPI0                                       62U
#define THERM_CTRL                                 63U
/* #define Reserved                                   64U */
#define SPI_NOR                                    65U
#define NFI                                        66U
#define NFI_ECC                                    67U
#define I2C3                                       68U
#define I2C4                                       69U
/* #define Reserved                                   70U */
#define I2C5                                       71U
/* #define Reserved                                   72U */
#define SPI2                                       73U
#define SPI3                                       74U
/* #define Reserved                                   75U */
/* #define Reserved                                   76U */
#define UART4                                      77U
/* #define Reserved                                   78U */
#define GMAC                                       79U
/* #define Reserved                                   80U */
/* #define Reserved                                   81U */
#define AUDIO                                      82U
#define MSDC0                                      83U
#define MSDC1                                      84U
#define MSDC2                                      85U
#define MSDC3                                      86U
#define USB3_0                                     87U
#define USB3_0SIF                                  88U
#define USB3_0SIF2                                 89U
#define USB3_0_2                                   90U
#define USB3_0SIF_2                                91U
#define USB3_0SIF2_2                               92U
#define SCPSYS_SRAM                                93U
#define PCIe0                                      94U
#define PCIe1                                      95U
#define G3D_CONFIG                                 96U
#define MMSYS_CONFIG                               97U
#define MDP_RDMA0                                  98U
#define MDP_RDMA1                                  99U
#define MDP_RSZ0                                   100U
#define MDP_RSZ1                                   101U
#define MDP_RSZ2                                   102U
#define MDP_WDMA                                   103U
#define MDP_WROT0                                  104U
#define MDP_WROT1                                  105U
#define MDP_TDSHP0                                 106U
#define MDP_TDSHP1                                 107U
/* #define Reserved                                   108U */
#define DISP_OVL0                                  109U
#define DISP_OVL1                                  110U
#define DISP_RDMA0                                 111U
#define DISP_RDMA1                                 112U
#define DISP_RDMA2                                 113U
#define DISP_WDMA0                                 114U
#define DISP_WDMA1                                 115U
#define DISP_COLOR0                                116U
#define DISP_COLOR1                                117U
#define DISP_AAL                                   118U
#define DISP_GAMMA                                 119U
/* #define Reserved                                   120U */
#define DISP_SPLIT0                                121U
/* #define Reserved                                   122U */
#define DISP_UFOE                                  123U
#define DSI0                                       124U
#define DSI1                                       125U
#define DPI                                        126U
#define DISP_PWM0                                  127U
#define DISP_PWM1                                  128U
#define MM_MUTEX                                   129U
#define SMI_LARB0                                  130U
#define SMI_COMMON                                 131U
#define DISP_OD                                    132U
#define DPI1                                       133U
/* #define Reserved                                   134U */
#define LVDS                                       135U
#define SMI_LARB4                                  136U
#define MDP_RDMA2                                  137U
#define DISP_COLOR2                                138U
#define DISP_AAL1                                  139U
#define DISP_OD1                                   140U
#define DISP_OVL2                                  141U
#define DISP_WDMA2                                 142U
#define LVDS1                                      143U
#define MDP_TDSHP2                                 144U
#define SMI_LARB5                                  145U
#define SMI_COMMON1                                146U
#define SMI_LARB7                                  147U
#define MDP_RDMA3                                  148U
#define MDP_WROT2                                  149U
#define DSI2                                       150U
#define DSI3                                       151U
/* #define Reserved                                   152U */
#define DISP_MONITOR0                              153U
#define DISP_MONITOR1                              154U
#define DISP_MONITOR2                              155U
#define DISP_MONITOR3                              156U
#define DISP_PWM2                                  157U
#define IMGSYS_CONFIG                              158U
#define SMI_LARB2                                  159U
#define SENINF_TOP0                                160U
#define SENINF_TOP1                                161U
#define CAMSV_TOP0                                 162U
#define CAMSV_TOP1                                 163U
#define CAMSV_TOP2                                 164U
#define CAMSV_TOP3                                 165U
#define CAMSV_TOP4                                 166U
#define CAMSV_TOP5                                 167U
/* #define Reserved                                   168U */
/* #define Reserved                                   169U */
/* #define Reserved                                   170U */
/* #define Reserved                                   171U */
/* #define Reserved                                   172U */
/* #define Reserved                                   173U */
#define BDP_DISPSYS_CONFIG                         174U
#define BDP_DISPFMT                                175U
#define BDP_VDO                                    176U
#define BDP_NR                                     177U
#define BDP_NR2                                    178U
#define BDP_TVD                                    179U
#define BDP_WR_CHANNEL_DI                          180U
#define BDP_WR_CHANNEL_VDI                         181U
#define BDP_LARB                                   182U
#define BDP_LARB_RT                                183U
#define BDP_DRAM2AXI_BRIDGE                        184U
#define VDECSYS_CONFIGURATION                      185U
#define VDECSYS_SMI_LARB1                          186U
#define VDEC_FULL_TOP                              187U
#define IMGRZ                                      188U
#define VDEC_MBIST                                 189U
#define JPGDEC_CONFIGURATION                       190U
#define JPDEC                                      191U
#define JPDGDEC1                                   192U
/* #define Reserved                                   193U */
#define VENC_CONFIGURATION                         194U
#define VENC_SMI_LARB3                             195U
#define VENC_SMI_LARB6                             196U
#define SMI_COMMON_2                               197U
#define VENC                                       198U
/*  #define Reserved                                   199U */
#define SFLASH                                     200U

extern void device_APC_dom_setup(void);
extern void tz_dapc_sec_setting(void);
#endif

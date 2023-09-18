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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#ifndef PLL_H
#define PLL_H
#include <platform/mt8518.h>
#include <platform/mt_reg_base.h>

#define CKSYS_BASE        	(IO_PHYS + 0x00000000)
#define INFRACFG_AO_BASE	(IO_PHYS + 0x00001000)
#define APMIXED_BASE      	(IO_PHYS + 0x00018000)
#define MCUSYS_CONFIG_BASE	(IO_PHYS + 0x00200000)
#define MMSYS_CONFIG_BASE 	(IO_PHYS + 0x04000000)

/***********************/
/* APMIXEDSYS Register */
/***********************/
#define AP_PLL_CON0          (APMIXED_BASE + 0x0000)
#define AP_PLL_CON1          (APMIXED_BASE + 0x0004)
#define AP_PLL_CON2          (APMIXED_BASE + 0x0008)
#define AP_PLL_CON3          (APMIXED_BASE + 0x000C)
#define AP_PLL_CON4          (APMIXED_BASE + 0x0010)
#define PLL_HP_CON0          (APMIXED_BASE + 0x0014)
#define PLL_HP_CON1          (APMIXED_BASE + 0x0018)
#define CLKSQ_STB_CON0       (APMIXED_BASE + 0x001C)
#define PLL_PWR_CON0         (APMIXED_BASE + 0x0020)
#define PLL_PWR_CON1         (APMIXED_BASE + 0x0024)
#define PLL_PWR_CON2         (APMIXED_BASE + 0x0028)
#define PLL_PWR_CON3         (APMIXED_BASE + 0x002C)
#define PLL_PWR_CON4         (APMIXED_BASE + 0x0070)
#define PLL_PWR_CON5         (APMIXED_BASE + 0x0074)
#define PLL_PWR_CON6         (APMIXED_BASE + 0x0078)
#define PLL_PWR_CON7         (APMIXED_BASE + 0x007C)
#define PLL_ISO_CON0         (APMIXED_BASE + 0x0030)
#define PLL_ISO_CON1         (APMIXED_BASE + 0x0034)
#define PLL_ISO_CON2         (APMIXED_BASE + 0x0038)
#define PLL_ISO_CON3         (APMIXED_BASE + 0x003C)
#define PLL_ISO_CON4         (APMIXED_BASE + 0x0080)
#define PLL_ISO_CON5         (APMIXED_BASE + 0x0084)
#define PLL_ISO_CON6         (APMIXED_BASE + 0x0088)
#define PLL_ISO_CON7         (APMIXED_BASE + 0x008C)
#define PLL_EN_CON0          (APMIXED_BASE + 0x0040)
#define PLL_EN_CON1          (APMIXED_BASE + 0x0044)
#define PLL_EN_CON2          (APMIXED_BASE + 0x0090)
#define PLL_EN_CON3          (APMIXED_BASE + 0x0094)
#define PLL_STB_CON0         (APMIXED_BASE + 0x0048)
#define PLL_STB_CON1         (APMIXED_BASE + 0x004C)
#define PLL_STB_CON2         (APMIXED_BASE + 0x0050)
#define PLL_STB_CON3         (APMIXED_BASE + 0x0098)
#define PLL_STB_CON4         (APMIXED_BASE + 0x009C)
#define PLL_STB_CON5         (APMIXED_BASE + 0x00A0)
#define DIV_STB_CON0         (APMIXED_BASE + 0x0054)
#define PLL_CHG_CON0         (APMIXED_BASE + 0x0058)
#define PLL_CHG_CON1         (APMIXED_BASE + 0x005C)
#define PLL_CHG_CON2         (APMIXED_BASE + 0x00A4)
#define PLL_CHG_CON3         (APMIXED_BASE + 0x00A8)
#define PLL_TEST_CON0        (APMIXED_BASE + 0x0060)
#define PLL_TEST_CON1        (APMIXED_BASE + 0x0064)
#define PLL_INT_CON0         (APMIXED_BASE + 0x0068)
#define PLL_INT_CON1         (APMIXED_BASE + 0x006C)
#define XTAL_CON0            (APMIXED_BASE + 0x00D0)
#define ARMPLL_CON0          (APMIXED_BASE + 0x0100)
#define ARMPLL_CON1          (APMIXED_BASE + 0x0104)
#define ARMPLL_CON2          (APMIXED_BASE + 0x0108)
#define ARMPLL_CON3          (APMIXED_BASE + 0x010C)
#define ARMPLL_PWR_CON0      (APMIXED_BASE + 0x0110)
#define MAINPLL_CON0         (APMIXED_BASE + 0x0120)
#define MAINPLL_CON1         (APMIXED_BASE + 0x0124)
#define MAINPLL_CON2         (APMIXED_BASE + 0x0128)
#define MAINPLL_CON3         (APMIXED_BASE + 0x012C)
#define MAINPLL_PWR_CON0     (APMIXED_BASE + 0x0130)
#define UNIVPLL_CON0         (APMIXED_BASE + 0x0140)
#define UNIVPLL_CON1         (APMIXED_BASE + 0x0144)
#define UNIVPLL_CON2         (APMIXED_BASE + 0x0148)
#define UNIVPLL_CON3         (APMIXED_BASE + 0x014C)
#define UNIVPLL_PWR_CON0     (APMIXED_BASE + 0x0150)
#define MMPLL_CON0           (APMIXED_BASE + 0x0160)
#define MMPLL_CON1           (APMIXED_BASE + 0x0164)
#define MMPLL_CON2           (APMIXED_BASE + 0x0168)
#define MMPLL_CON3           (APMIXED_BASE + 0x016C)
#define MMPLL_PWR_CON0       (APMIXED_BASE + 0x0170)
#define APLL1_CON0           (APMIXED_BASE + 0x0180)
#define APLL1_CON1           (APMIXED_BASE + 0x0184)
#define APLL1_CON2           (APMIXED_BASE + 0x0188)
#define APLL1_CON3           (APMIXED_BASE + 0x018C)
#define APLL1_PWR_CON0       (APMIXED_BASE + 0x0190)
#define APLL1_CON_TUNER      (APMIXED_BASE + 0x0194)
#define APLL2_CON0           (APMIXED_BASE + 0x01A0)
#define APLL2_CON1           (APMIXED_BASE + 0x01A4)
#define APLL2_CON2           (APMIXED_BASE + 0x01A8)
#define APLL2_CON3           (APMIXED_BASE + 0x01AC)
#define APLL2_PWR_CON0       (APMIXED_BASE + 0x01B0)
#define APLL2_CON_TUNER      (APMIXED_BASE + 0x01B4)
#define TVDPLL_CON0          (APMIXED_BASE + 0x01C0)
#define TVDPLL_CON1          (APMIXED_BASE + 0x01C4)
#define TVDPLL_CON2          (APMIXED_BASE + 0x01C8)
#define TVDPLL_CON3          (APMIXED_BASE + 0x01CC)
#define TVDPLL_PWR_CON0      (APMIXED_BASE + 0x01D0)
#define AP_AUXADC_CON0       (APMIXED_BASE + 0x0400)
#define TS_CON0              (APMIXED_BASE + 0x0600)
#define TS_CON1              (APMIXED_BASE + 0x0604)
#define AP_ABIST_MON_CON0    (APMIXED_BASE + 0x0E00)
#define AP_ABIST_MON_CON1    (APMIXED_BASE + 0x0E04)
#define AP_ABIST_MON_CON2    (APMIXED_BASE + 0x0E08)
#define AP_ABIST_MON_CON3    (APMIXED_BASE + 0x0E0C)
#define OCCSCAN_CON0         (APMIXED_BASE + 0x0E1C)
#define CLKDIV_CON0          (APMIXED_BASE + 0x0E20)
#define RSV_RW0_CON0         (APMIXED_BASE + 0x0B00)
#define RSV_RW0_CON1         (APMIXED_BASE + 0x0B04)
#define RSV_RW1_CON0         (APMIXED_BASE + 0x0B08)
#define RSV_RW1_CON1         (APMIXED_BASE + 0x0B0C)
#define RSV_RO_CON0          (APMIXED_BASE + 0x0B10)
#define RSV_RO_CON1          (APMIXED_BASE + 0x0B14)
#define RSV_ATPG_CON0        (APMIXED_BASE + 0x0B18)
#define ZCD_RO_CON0          (APMIXED_BASE + 0x0C00)
#define ANA_FIFO_RO_CON0     (APMIXED_BASE + 0x0C10)

/***********************/
/* TOPCKGEN Register   */
/***********************/
#define CLK_MUX_SEL0           (CKSYS_BASE + 0x000)
#define CLK_MUX_SEL1           (CKSYS_BASE + 0x004)
#define TOPBUS_DCMCTL          (CKSYS_BASE + 0x008)
#define TOPEMI_DCMCTL          (CKSYS_BASE + 0x00C)
#define FREQ_MTR_CTRL          (CKSYS_BASE + 0x010)
#define FREQ_MTR_DAT           (CKSYS_BASE + 0x014)
#define CLK_GATING_CTRL0       (CKSYS_BASE + 0x020)
#define CLK_GATING_CTRL1       (CKSYS_BASE + 0x024)
#define INFRABUS_DCMCTL0       (CKSYS_BASE + 0x028)
#define INFRABUS_DCMCTL1       (CKSYS_BASE + 0x02C)
#define MPLL_FREDIV_EN         (CKSYS_BASE + 0x030)
#define UPLL_FREDIV_EN         (CKSYS_BASE + 0x034)
#define TEST_DBG_CTRL          (CKSYS_BASE + 0x038)
#define CLK_GATING_CTRL7       (CKSYS_BASE + 0x03C)
#define CLK_MUX_SEL8           (CKSYS_BASE + 0x040)
#define CLK_SEL_9              (CKSYS_BASE + 0x044)
#define CLK_SEL_10             (CKSYS_BASE + 0x048)
#define CLK_SEL_11             (CKSYS_BASE + 0x04C)
#define CLK_GATING_CTRL0_SET   (CKSYS_BASE + 0x050)
#define CLK_GATING_CTRL1_SET   (CKSYS_BASE + 0x054)
#define INFRABUS_DCMCTL0_SET   (CKSYS_BASE + 0x058)
#define INFRABUS_DCMCTL1_SET   (CKSYS_BASE + 0x05C)
#define SET_MPLL_FREDIV_EN     (CKSYS_BASE + 0x060)
#define SET_UPLL_FREDIV_EN     (CKSYS_BASE + 0x064)
#define TEST_DBG_CTRL_SET      (CKSYS_BASE + 0x068)
#define CLK_GATING_CTRL7_SET   (CKSYS_BASE + 0x06C)
#define CLK_GATING_CTRL8       (CKSYS_BASE + 0x070)
#define CLK_GATING_CTRL9       (CKSYS_BASE + 0x074)
#define CLK_SEL_12             (CKSYS_BASE + 0x078)
#define CLK_MUX_SEL13          (CKSYS_BASE + 0x07c)
#define CLK_GATING_CTRL0_CLR   (CKSYS_BASE + 0x080)
#define CLK_GATING_CTRL1_CLR   (CKSYS_BASE + 0x084)
#define INFRABUS_DCMCTL0_CLR   (CKSYS_BASE + 0x088)
#define INFRABUS_DCMCTL1_CLR   (CKSYS_BASE + 0x08C)
#define CLR_MPLL_FREDIV_EN     (CKSYS_BASE + 0x090)
#define CLR_UPLL_FREDIV_EN     (CKSYS_BASE + 0x094)
#define TEST_DBG_CTRL_CLR      (CKSYS_BASE + 0x098)
#define CLK_GATING_CTRL7_CLR   (CKSYS_BASE + 0x09C)
#define CLK_GATING_CTRL8_SET   (CKSYS_BASE + 0x0A0)
#define SET_CLK_GATING_CTRL9   (CKSYS_BASE + 0x0A4)
#define CLK_GATING_CTRL8_CLR   (CKSYS_BASE + 0x0B0)
#define CLR_CLK_GATING_CTRL9   (CKSYS_BASE + 0x0B4)
#define CLK_MUX_SEL14          (CKSYS_BASE + 0x0c0)
#define CLK_MUX_SEL15          (CKSYS_BASE + 0x0c4)
#define CLK_MUX_SEL16          (CKSYS_BASE + 0x0c8)
#define CLK_MUX_SEL17          (CKSYS_BASE + 0x0cc)
#define CLK_MUX_SEL19          (CKSYS_BASE + 0x0d4)
#define CLK_MUX_SEL20          (CKSYS_BASE + 0x0d8)
#define CLK_MUX_SEL21          (CKSYS_BASE + 0x0dc)
#define CLK_GATING_CTRL10      (CKSYS_BASE + 0x0e0)
#define CLK_GATING_CTRL11      (CKSYS_BASE + 0x0e4)
#define CLK_GATING_CTRL12      (CKSYS_BASE + 0x0e8)
#define CLK_GATING_CTRL13      (CKSYS_BASE + 0x0ec)
#define CLK_MUX_SEL22          (CKSYS_BASE + 0x0f4)
#define CLK_MUX_SEL23          (CKSYS_BASE + 0x0f8)
#define LPM_CTRL               (CKSYS_BASE + 0x100)
#define LPM_TOTAL_TIME         (CKSYS_BASE + 0x104)
#define LPM_LOW2HIGH_COUNT     (CKSYS_BASE + 0x108)
#define LPM_HIGH_DUR_TIME      (CKSYS_BASE + 0x10C)
#define LPM_LONGEST_HIGHTIME   (CKSYS_BASE + 0x110)
#define LPM_GOODDUR_COUNT      (CKSYS_BASE + 0x114)
#define CLK_GATING_CTRL10_SET  (CKSYS_BASE + 0x120)
#define CLK_GATING_CTRL12_SET  (CKSYS_BASE + 0x128)
#define CLK_GATING_CTRL13_SET  (CKSYS_BASE + 0x12c)
#define CLK_GATING_CTRL10_CLR  (CKSYS_BASE + 0x140)
#define CLK_GATING_CTRL12_CLR  (CKSYS_BASE + 0x148)
#define CLK_GATING_CTRL13_CLR  (CKSYS_BASE + 0x14c)

/***********************/
/* INFRASYS Register   */
/***********************/
#define INFRA_CLKSEL            (INFRACFG_AO_BASE + 0x80)
#define TOPAXI_PROT_EN          (INFRACFG_AO_BASE + 0x220)
#define TOPAXI_PROT_STA0        (INFRACFG_AO_BASE + 0x224)
#define TOPAXI_PROT_STA1        (INFRACFG_AO_BASE + 0x228)

/***********************/
/* MMSYS Register      */
/***********************/
#define MMSYS_CG_CON0		(MMSYS_CONFIG_BASE + 0x100)
#define MMSYS_HW_DCM_DIS0		(MMSYS_CONFIG_BASE + 0x120)

/***********************/
/* MCUSYS Register      */
/***********************/
#define ACLKEN_DIV          	(MCUSYS_CONFIG_BASE + 0x640)
#define PCLKEN_DIV          	(MCUSYS_CONFIG_BASE + 0x644)
#define MCU_BUS_MUX             (MCUSYS_CONFIG_BASE + 0x7C0)

void mt_pll_init(void);
void mt_pll_post_init(void);
//====================================================

#endif

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
#include <platform/mt8512.h>
#include <platform/mt_reg_base.h>

#define CKSYS_BASE        		(IO_PHYS + 0x00000000)
#define INFRACFG_AO_BASE		(IO_PHYS + 0x00001000)
#define APMIXED_BASE      		(IO_PHYS + 0x0000C000)
#define MCUSYS_CONFIG_BASE		(IO_PHYS + 0x00200000)
#define MMSYS_CONFIG_BASE 		(IO_PHYS + 0x04000000)
#define IMGSYS_CONFIG_BASE 		(IO_PHYS + 0x05000000)
#define IPSYS_BASE			(IO_PHYS + 0x03002000)
#define SPM_BASE			(IO_PHYS + 0x00006000)

/***********************/
/* APMIXEDSYS Register */
/***********************/
#define AP_PLL_CON0           (APMIXED_BASE + 0x0000)
#define AP_PLL_CON1           (APMIXED_BASE + 0x0004)
#define AP_PLL_CON2           (APMIXED_BASE + 0x0008)
#define AP_PLL_CON3           (APMIXED_BASE + 0x000C)
#define AP_PLL_CON4           (APMIXED_BASE + 0x0010)
#define AP_PLL_CON5           (APMIXED_BASE + 0x0014)
#define AP_PLL_CON5_SET       (APMIXED_BASE + 0x0050)
#define AP_PLL_CON5_CLR       (APMIXED_BASE + 0x0054)
#define CLKSQ_STB_CON0        (APMIXED_BASE + 0x0018)
#define PLL_PWR_CON0          (APMIXED_BASE + 0x001C)
#define PLL_PWR_CON1          (APMIXED_BASE + 0x0020)
#define PLL_ISO_CON0          (APMIXED_BASE + 0x0024)
#define PLL_ISO_CON1          (APMIXED_BASE + 0x0028)
#define PLL_STB_CON0          (APMIXED_BASE + 0x002C)
#define DIV_STB_CON0          (APMIXED_BASE + 0x0030)
#define PLL_CHG_CON0          (APMIXED_BASE + 0x0034)
#define PLL_TEST_CON0         (APMIXED_BASE + 0x0038)
#define PLL_TEST_CON1         (APMIXED_BASE + 0x003C)
#define APLL1_TUNER_CON0      (APMIXED_BASE + 0x0040)
#define PLLON_CON0            (APMIXED_BASE + 0x0044)
#define PLLON_CON1            (APMIXED_BASE + 0x0048)
#define APLL2_TUNER_CON0      (APMIXED_BASE + 0x004C)
#define AP_ABIST_MON_CON0     (APMIXED_BASE + 0x0800)
#define AP_ABIST_MON_CON1     (APMIXED_BASE + 0x0804)
#define AP_ABIST_MON_CON2     (APMIXED_BASE + 0x0808)
#define AP_ABIST_MON_CON3     (APMIXED_BASE + 0x080C)
#define OCCSCAN_CON0          (APMIXED_BASE + 0x0810)
#define CLKDIV_CON0           (APMIXED_BASE + 0x0814)
#define OCCSCAN_CON1          (APMIXED_BASE + 0x0818)
#define OCCSCAN_CON2          (APMIXED_BASE + 0x081C)
#define OCCSCAN_CON3          (APMIXED_BASE + 0x0820)
#define MCU_OCCSCAN_CON0      (APMIXED_BASE + 0x0824)
#define AP_TSENSE_CON0        (APMIXED_BASE + 0x0600)
#define AP_TSENSE_CON1        (APMIXED_BASE + 0x0604)
#define AP_TSENSE_CON2        (APMIXED_BASE + 0x0608)
#define RSV_RW0_CON0          (APMIXED_BASE + 0x0900)
#define RSV_RW1_CON0          (APMIXED_BASE + 0x0904)
#define RSV_RO_CON0           (APMIXED_BASE + 0x0908)
#define AP_PLLGP1_CON0        (APMIXED_BASE + 0x0200)
#define AP_PLLGP1_CON1        (APMIXED_BASE + 0x0204)
#define UNIVPLL_CON0          (APMIXED_BASE + 0x0208)
#define UNIVPLL_CON1          (APMIXED_BASE + 0x020C)
#define UNIVPLL_CON2          (APMIXED_BASE + 0x0210)
#define UNIVPLL_CON3          (APMIXED_BASE + 0x0214)
#define MAINPLL_CON0          (APMIXED_BASE + 0x0228)
#define MAINPLL_CON1          (APMIXED_BASE + 0x022C)
#define MAINPLL_CON2          (APMIXED_BASE + 0x0230)
#define MAINPLL_CON3          (APMIXED_BASE + 0x0234)
#define AP_PLLGP2_CON0        (APMIXED_BASE + 0x0300)
#define AP_PLLGP2_CON1        (APMIXED_BASE + 0x0304)
#define AP_PLLGP2_CON2        (APMIXED_BASE + 0x0308)
#define ARMPLL_CON0           (APMIXED_BASE + 0x030C)
#define ARMPLL_CON1           (APMIXED_BASE + 0x0310)
#define ARMPLL_CON2           (APMIXED_BASE + 0x0314)
#define ARMPLL_CON3           (APMIXED_BASE + 0x0318)
#define APLL1_CON0            (APMIXED_BASE + 0x031C)
#define APLL1_CON1            (APMIXED_BASE + 0x0320)
#define APLL1_CON2            (APMIXED_BASE + 0x0324)
#define APLL1_CON3            (APMIXED_BASE + 0x0328)
#define APLL1_CON4            (APMIXED_BASE + 0x032C)
#define MPLL_CON0             (APMIXED_BASE + 0x0340)
#define MPLL_CON1             (APMIXED_BASE + 0x0344)
#define MPLL_CON2             (APMIXED_BASE + 0x0348)
#define MPLL_CON3             (APMIXED_BASE + 0x034C)
#define MSDCPLL_CON0          (APMIXED_BASE + 0x0350)
#define MSDCPLL_CON1          (APMIXED_BASE + 0x0354)
#define MSDCPLL_CON2          (APMIXED_BASE + 0x0358)
#define MSDCPLL_CON3          (APMIXED_BASE + 0x035C)
#define APLL2_CON0            (APMIXED_BASE + 0x0360)
#define APLL2_CON1            (APMIXED_BASE + 0x0364)
#define APLL2_CON2            (APMIXED_BASE + 0x0368)
#define APLL2_CON3            (APMIXED_BASE + 0x036C)
#define APLL2_CON4            (APMIXED_BASE + 0x0370)
#define IPPLL_CON0            (APMIXED_BASE + 0x0374)
#define IPPLL_CON1            (APMIXED_BASE + 0x0378)
#define IPPLL_CON2            (APMIXED_BASE + 0x037C)
#define IPPLL_CON3            (APMIXED_BASE + 0x0380)
#define DSPPLL_CON0           (APMIXED_BASE + 0x0390)
#define DSPPLL_CON1           (APMIXED_BASE + 0x0394)
#define DSPPLL_CON2           (APMIXED_BASE + 0x0398)
#define DSPPLL_CON3           (APMIXED_BASE + 0x039C)
#define TCONPLL_CON0          (APMIXED_BASE + 0x03A0)
#define TCONPLL_CON1          (APMIXED_BASE + 0x03A4)
#define TCONPLL_CON2          (APMIXED_BASE + 0x03A8)
#define TCONPLL_CON3          (APMIXED_BASE + 0x03AC)
#define IOSEL		      (APMIXED_BASE + 0x033C)

/***********************/
/* TOPCKGEN Register   */
/***********************/
#define CLK_MODE              (CKSYS_BASE + 0x000)
#define CLK_CFG_UPDATE        (CKSYS_BASE + 0x004)
#define CLK_CFG_UPDATE1       (CKSYS_BASE + 0x008)
#define CLK_CFG_0             (CKSYS_BASE + 0x040)
#define CLK_CFG_0_SET         (CKSYS_BASE + 0x044)
#define CLK_CFG_0_CLR         (CKSYS_BASE + 0x048)
#define CLK_CFG_1             (CKSYS_BASE + 0x050)
#define CLK_CFG_1_SET         (CKSYS_BASE + 0x054)
#define CLK_CFG_1_CLR         (CKSYS_BASE + 0x058)
#define CLK_CFG_2             (CKSYS_BASE + 0x060)
#define CLK_CFG_2_SET         (CKSYS_BASE + 0x064)
#define CLK_CFG_2_CLR         (CKSYS_BASE + 0x068)
#define CLK_CFG_3             (CKSYS_BASE + 0x070)
#define CLK_CFG_3_SET         (CKSYS_BASE + 0x074)
#define CLK_CFG_3_CLR         (CKSYS_BASE + 0x078)
#define CLK_CFG_4             (CKSYS_BASE + 0x080)
#define CLK_CFG_4_SET         (CKSYS_BASE + 0x084)
#define CLK_CFG_4_CLR         (CKSYS_BASE + 0x088)
#define CLK_CFG_5             (CKSYS_BASE + 0x090)
#define CLK_CFG_5_SET         (CKSYS_BASE + 0x094)
#define CLK_CFG_5_CLR         (CKSYS_BASE + 0x098)
#define CLK_CFG_6             (CKSYS_BASE + 0x0A0)
#define CLK_CFG_6_SET         (CKSYS_BASE + 0x0A4)
#define CLK_CFG_6_CLR         (CKSYS_BASE + 0x0A8)
#define CLK_CFG_7             (CKSYS_BASE + 0x0B0)
#define CLK_CFG_7_SET         (CKSYS_BASE + 0x0B4)
#define CLK_CFG_7_CLR         (CKSYS_BASE + 0x0B8)
#define CLK_CFG_8             (CKSYS_BASE + 0x0C0)
#define CLK_CFG_8_SET         (CKSYS_BASE + 0x0C4)
#define CLK_CFG_8_CLR         (CKSYS_BASE + 0x0C8)
#define CLK_CFG_9             (CKSYS_BASE + 0x0D0)
#define CLK_CFG_9_SET         (CKSYS_BASE + 0x0D4)
#define CLK_CFG_9_CLR         (CKSYS_BASE + 0x0D8)
#define CLK_CFG_10            (CKSYS_BASE + 0x0E0)
#define CLK_CFG_10_SET        (CKSYS_BASE + 0x0E4)
#define CLK_CFG_10_CLR        (CKSYS_BASE + 0x0E8)
#define CLK_CFG_11            (CKSYS_BASE + 0x0EC)
#define CLK_CFG_11_SET        (CKSYS_BASE + 0x0F0)
#define CLK_CFG_11_CLR        (CKSYS_BASE + 0x0F4)
#define CLK_MISC_CFG_0        (CKSYS_BASE + 0x104)
#define CLK_MISC_CFG_1        (CKSYS_BASE + 0x108)
#define CLK_DBG_CFG           (CKSYS_BASE + 0x10C)
#define CLK_SCP_CFG_0         (CKSYS_BASE + 0x200)
#define CLK_SCP_CFG_1         (CKSYS_BASE + 0x204)
#define CLK26CALI_0           (CKSYS_BASE + 0x220)
#define CLK26CALI_1           (CKSYS_BASE + 0x224)
#define CLK26CALI_2           (CKSYS_BASE + 0x228)
#define CKSTA_REG             (CKSYS_BASE + 0x230)
#define CKSTA1_REG            (CKSYS_BASE + 0x234)
#define CLKMON_CLK_SEL_REG    (CKSYS_BASE + 0x300)
#define CLKMON_K1_REG         (CKSYS_BASE + 0x304)
#define CLK_AUDDIV_0          (CKSYS_BASE + 0x320)
#define CLK_AUDDIV_1          (CKSYS_BASE + 0x324)
#define CLK_AUDDIV_2          (CKSYS_BASE + 0x328)
#define CLK_AUDDIV_3          (CKSYS_BASE + 0x32C)
#define AUD_TOP_CFG           (CKSYS_BASE + 0x330)
#define AUD_TOP_MON           (CKSYS_BASE + 0x334)
#define CLK_APLL_CFG          (CKSYS_BASE + 0x338)
#define CLK_IPPLL_CFG         (CKSYS_BASE + 0x33C)
#define CLK_AUDDIV_4          (CKSYS_BASE + 0x340)
#define CLK_AUDDIV_5          (CKSYS_BASE + 0x344)
#define CLK_AUDDIV_6          (CKSYS_BASE + 0x348)
#define CLK_AUDDIV_7          (CKSYS_BASE + 0x34C)
#define CLK_DUMMY_0           (CKSYS_BASE + 0x350)
#define CLK_DUMMY_1           (CKSYS_BASE + 0x354)
#define CLK_DUMMY_2           (CKSYS_BASE + 0x358)
#define CLK_DUMMY_3           (CKSYS_BASE + 0x35C)
#define CLK_PDN_REG           (CKSYS_BASE + 0x400)
#define CLK_EXTCK_REG         (CKSYS_BASE + 0x500)

/***********************/
/* INFRASYS Register   */
/***********************/
#define MODULE_SW_CG_0_SET    (INFRACFG_AO_BASE + 0x080)
#define MODULE_SW_CG_0_CLR    (INFRACFG_AO_BASE + 0x084)
#define MODULE_SW_CG_1_SET    (INFRACFG_AO_BASE + 0x088)
#define MODULE_SW_CG_1_CLR    (INFRACFG_AO_BASE + 0x08C)
#define MODULE_SW_CG_0_STA    (INFRACFG_AO_BASE + 0x090)
#define MODULE_SW_CG_1_STA    (INFRACFG_AO_BASE + 0x094)
#define MODULE_SW_CG_2_SET    (INFRACFG_AO_BASE + 0x0A4)
#define MODULE_SW_CG_2_CLR    (INFRACFG_AO_BASE + 0x0A8)
#define MODULE_SW_CG_2_STA    (INFRACFG_AO_BASE + 0x0AC)
#define MODULE_SW_CG_3_SET    (INFRACFG_AO_BASE + 0x0C0)
#define MODULE_SW_CG_3_CLR    (INFRACFG_AO_BASE + 0x0C4)
#define MODULE_SW_CG_3_STA    (INFRACFG_AO_BASE + 0x0C8)
#define MODULE_SW_CG_4_SET    (INFRACFG_AO_BASE + 0x0D0)
#define MODULE_SW_CG_4_CLR    (INFRACFG_AO_BASE + 0x0D4)
#define MODULE_SW_CG_4_STA    (INFRACFG_AO_BASE + 0x0D8)

#define INFRA_MFG_MASTER_M0_GALS_CTRL    (INFRACFG_AO_BASE + 0x294)
/***********************/
/* MMSYS Register      */
/***********************/
#define MMSYS_CG_CON0   (MMSYS_CONFIG_BASE + 0x100)
#define MMSYS_CG_SET0   (MMSYS_CONFIG_BASE + 0x104)
#define MMSYS_CG_CLR0   (MMSYS_CONFIG_BASE + 0x108)

/***********************/
/* MMSYS Register      */
/***********************/
#define IMGSYS_CG_CON0       (IMGSYS_CONFIG_BASE + 0x100)
#define IMGSYS_CG_SET0       (IMGSYS_CONFIG_BASE + 0x104)
#define IMGSYS_CG_CLR0       (IMGSYS_CONFIG_BASE + 0x108)
#define IMGSYS_CG_CON1       (IMGSYS_CONFIG_BASE + 0x110)
#define IMGSYS_CG_SET1       (IMGSYS_CONFIG_BASE + 0x114)
#define IMGSYS_CG_CLR1       (IMGSYS_CONFIG_BASE + 0x118)

/***********************/
/* MCUSYS Register      */
/***********************/
#define ACLKEN_DIV          (MCUSYS_CONFIG_BASE + 0x640)
#define PCLKEN_DIV          (MCUSYS_CONFIG_BASE + 0x644)
#define MCU_BUS_MUX         (MCUSYS_CONFIG_BASE + 0x7C0)

/***********************/
/* IPSYS Register      */
/***********************/
#define IPSYS_NNA0_PWR_ON	(IPSYS_BASE + 0x110)
#define IPSYS_NNA1_PWR_ON	(IPSYS_BASE + 0x114)
#define IPSYS_WFST_PWR_ON	(IPSYS_BASE + 0x118)
#define IPSYS_AXI_CK_CG		(IPSYS_BASE + 0x108)
#define IPSYS_26M_CK_CG		(IPSYS_BASE + 0xfc)
#define IPSYS_EMI_CK_CG		(IPSYS_BASE + 0x100)
#define IPSYS_SRAM_CK_CG	(IPSYS_BASE + 0x104)
#define IPSYS_NNAO_CK_CG	(IPSYS_BASE + 0x98)
#define IPSYS_NNA1_CK_CG	(IPSYS_BASE + 0x9c)
#define IPSYS_WFST_CK_CG	(IPSYS_BASE + 0xa0)

#define WDT_SWSYSRST		(RGU_BASE + 0x018)

#define DA_XTAL_CTRL2         (SPM_BASE + 0xAB8)

void mt_pll_init(void);
void mt_pll_post_init(void);
//====================================================

#endif

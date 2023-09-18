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

#include <stdbool.h>
#include "device_apc.h"
#include "print.h"

#define MTAG	"[DEVAPC]"
#define DAPC_DEBUG

#ifdef DAPC_DEBUG
#define DBG(str, ...) do {print(MTAG str, ##__VA_ARGS__);} while(0)
#else
#define DBG(str, ...) do {} while(0)
#endif

#define INFO(str, ...) do {print(MTAG str, ##__VA_ARGS__);} while(0)
#define ERROR(str, ...) do {print(MTAG "[ERROR]" str, ##__VA_ARGS__);} while(0)


static const struct INFRA_PERI_DEVICE_INFO D_APC_INFRA_Devices[] = {
	/*		module,					AP permission,   MD permission,   SPM permission,
	 */

	/* 0 */
	DAPC_INFRA_ATTR("SPM_APB_S",                            E_NO_PROTECTION, E_SEC_RW_ONLY,   E_NO_PROTECTION),
	DAPC_INFRA_ATTR("SPM_APB_S-1",                          E_SEC_RW_ONLY,   E_SEC_RW_ONLY,   E_NO_PROTECTION),
	DAPC_INFRA_ATTR("SPM_APB_S-2",                          E_SEC_RW_ONLY,   E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("TOPCKGEN_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("INFRACFG_AO_APB_S",                    E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("IOCFG_APB_S",                          E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("PERICFG_AO_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("EFUSE_DEBUG_AO_APB_S",                 E_SEC_RW_NS_R,   E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("GPIO_APB_S",                           E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("TOPRGU_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),

	/* 10 */
	DAPC_INFRA_ATTR("APXGPT_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("INFRAAO_RSV0_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("SEJ_APB_S",                            E_SEC_RW_ONLY,   E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("AP_CIRQ_EINT_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("APMIXEDSYS_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("PMIC_WRAP_APB_S",                      E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("INFRAAO_RSV2_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("KP_APB_S",                             E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("TOP_MISC_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DVFSRC_APB_S",                         E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),

	/* 20 */
	DAPC_INFRA_ATTR("MBIST_AO_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("CLDMA_AO_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("INFRAAO_BCRM_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("AES_TOP0_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("SYS_TIMER_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("MODEM_TEMP_SHARE_APB_S",               E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DEBUG_CTRL_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("SECURITY_AO_APB_S",                    E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("TOPCKGEN_INFRA_CFG_APB_S",             E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DEVICE_APC_AO_APB_S",                  E_SEC_RW_ONLY,   E_FORBIDDEN,     E_FORBIDDEN),

	/* 30 */
	DAPC_INFRA_ATTR("PWM_APB_S",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("HSM_AXI_S",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("PCIE_BR_S",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("PCIE_PCI0_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SSUSB_S",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SSUSB_S-1",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SSUSB_S-2",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("USB_S",                                E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("USB_S-1",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("USB_S-2",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 40 */
	DAPC_INFRA_ATTR("MCUPM_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MCUPM_APB_S-1",                        E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MCUPM_APB_S-2",                        E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("AUDIO_S",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MSDC0_S",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MSDC1_S",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MSDC2_S",                              E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("EAST_APB0_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("EAST_APB1_S",                          E_NO_PROTECTION, E_SEC_RW_NS_R,   E_FORBIDDEN),
	DAPC_INFRA_ATTR("EAST_APB2_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 50 */
	DAPC_INFRA_ATTR("EAST_APB3_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SOUTH_APB0_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SOUTH_APB1_S",                         E_NO_PROTECTION, E_SEC_RW_NS_R,   E_FORBIDDEN),
	DAPC_INFRA_ATTR("SOUTH_APB2_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SOUTH_APB3_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("WEST_APB0_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("WEST_APB1_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("WEST_APB2_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("WEST_APB3_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("NORTH_APB0_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 60 */
	DAPC_INFRA_ATTR("NORTH_APB1_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("NORTH_APB2_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("NORTH_APB3_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("MCUCFG_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SYS_CIRQ_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("TRNG_APB_S",                           E_SEC_RW_ONLY,   E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("DEVICE_APC_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("DEBUG_TRACKER_APB_S",                  E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("CCIF0_AP_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("CCIF0_MD_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),

	/* 70 */
	DAPC_INFRA_ATTR("CCIF1_AP_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("CCIF1_MD_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("MBIST_PDN_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("INFRACFG_PDN_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_NS_APB_S",                        E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_MMU_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("CQ_DMA_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("INFRA_RSV0_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("SRAMROM_APB_S",                        E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),

	/* 80 */
	DAPC_INFRA_ATTR("INFRA_BCRM_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("EMI_APB_S",                            E_NO_PROTECTION, E_NO_PROTECTION, E_NO_PROTECTION),
	DAPC_INFRA_ATTR("INFRA_RSV2_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("CLDMA_APB_S",                          E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("CLDMA_MD_APB_S",                       E_NO_PROTECTION, E_NO_PROTECTION, E_FORBIDDEN),
	DAPC_INFRA_ATTR("EMI_MPU_APB_S",                        E_SEC_RW_NS_R,   E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("INFRA_RSV3_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH0_TOP0_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH0_TOP1_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH0_TOP2_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),

	/* 90 */
	DAPC_INFRA_ATTR("DRAMC_CH0_TOP3_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH0_TOP4_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH1_TOP0_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH1_TOP1_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH1_TOP2_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH1_TOP3_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("DRAMC_CH1_TOP4_APB_S",                 E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("GCE_APB_S",                            E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("CCIF2_AP_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("CCIF2_MD_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 100 */
	DAPC_INFRA_ATTR("CCIF3_AP_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("CCIF3_MD_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC2_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC3_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC4_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC5_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC6_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC7_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC8_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 110 */
	DAPC_INFRA_ATTR("GCPU_ECC9_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC10_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC11_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC12_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC13_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC14_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC15_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("GCPU_ECC16_APB_S",                     E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("FAKE_ENG_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("TRFG_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 120 */
	DAPC_INFRA_ATTR("DEBUG_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("APDMA_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("AUXADC_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("UART0_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("UART1_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("UART2_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("UART3_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_NO_PROTECTION),
	DAPC_INFRA_ATTR("I2C0_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("I2C1_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("I2C2_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 130 */
	DAPC_INFRA_ATTR("SPI0_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("PTP_THERM_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("BTIF_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("PERI_RSV0_APB_S",                      E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("DISP_PWM_APB_S",                       E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("I2C3_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SPI1_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("I2C4_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SPI2_APB_S",                           E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("SPI_SLV_APB_S",                        E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

	/* 140 */
	DAPC_INFRA_ATTR("UART4_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("UART5_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("UART6_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("IMP_IIC_WRAP_APB_S",                   E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("NFI_APB_S",                            E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("NFIECC_APB_S",                         E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),
	DAPC_INFRA_ATTR("ETHER_APB_S",                          E_NO_PROTECTION, E_FORBIDDEN,     E_FORBIDDEN),

};

static const struct MD_DEVICE_INFO D_APC_MD_Devices[] = {
	/*              module,                     AP permission */

	 /* 0 */
	DAPC_MD_ATTR("MDPERISYS_1",                 E_NO_PROTECTION),
	DAPC_MD_ATTR("MDPERISYS_2/MDTOP",           E_NO_PROTECTION),
	DAPC_MD_ATTR("MDMCUAPB",                    E_NO_PROTECTION),
	DAPC_MD_ATTR("MDCORESYS",                   E_NO_PROTECTION),
	DAPC_MD_ATTR("MDINFRA_APB_1",               E_NO_PROTECTION),
	DAPC_MD_ATTR("MDINFRA_APB_2",               E_NO_PROTECTION),
	DAPC_MD_ATTR("MML2",                        E_NO_PROTECTION),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),

	 /* 10 */
	DAPC_MD_ATTR("MD_INFRA",                    E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("uSIP Peripheral",             E_FORBIDDEN),
	DAPC_MD_ATTR("modeml1_ao_top_pwr_wrap",     E_NO_PROTECTION),
	DAPC_MD_ATTR("md2gsys_pwr_wrap",            E_FORBIDDEN),
	DAPC_MD_ATTR("rxdfesys_pwr_wrap",           E_FORBIDDEN),

	 /* 20 */
	DAPC_MD_ATTR("cssys_pwr_wrap",              E_FORBIDDEN),
	DAPC_MD_ATTR("txsys_pwr_wrap",              E_FORBIDDEN),
	DAPC_MD_ATTR("bigramsys (mem)",             E_FORBIDDEN),
	DAPC_MD_ATTR("md32scq share (mem)",         E_FORBIDDEN),
	DAPC_MD_ATTR("md32scq_vu01 (mem)",          E_FORBIDDEN),
	DAPC_MD_ATTR("peripheral (reg)",            E_FORBIDDEN),
	DAPC_MD_ATTR("rakesys_pwr_wrap",            E_FORBIDDEN),
	DAPC_MD_ATTR("rakesys_pwr_wrap",            E_FORBIDDEN),
	DAPC_MD_ATTR("brpsys_pwr_wrap",             E_FORBIDDEN),
	DAPC_MD_ATTR("brpsys_pwr_wrap",             E_FORBIDDEN),

	 /* 30 */
	DAPC_MD_ATTR("dmcsys_pwr_wrap",             E_FORBIDDEN),
	DAPC_MD_ATTR("dmcsys_pwr_wrap",             E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),
	DAPC_MD_ATTR("-",                           E_FORBIDDEN),

};

static uint32_t set_module_apc(enum DAPC_SLAVE_TYPE slave_type, uint32_t module,
	enum E_MASK_DOM domain_num, enum APC_ATTR permission)
{
	uint32_t *base = NULL;
	uint32_t apc_index;
	uint32_t apc_set_index;
	uint32_t clr_bit;
	uint32_t set_bit;

	if (permission != E_NO_PROTECTION &&
		permission != E_SEC_RW_ONLY &&
		permission != E_SEC_RW_NS_R &&
		permission != E_FORBIDDEN) {

		ERROR("permission=0x%x is not supported!\n",
			permission);
		return DEVAPC_ERR_PERMISSION_NOT_SUPPORTED;
	}

	apc_index = module / MOD_NO_IN_1_DEVAPC;
	apc_set_index = module % MOD_NO_IN_1_DEVAPC;
	clr_bit = 0xFFFFFFFF ^ (0x3 << (apc_set_index * 2));
	set_bit = permission << (apc_set_index * 2);

	/* Do boundary check */
	if (slave_type == E_DAPC_INFRA_SLAVE &&
		module <= SLAVE_INFRA_MAX_INDEX &&
		domain_num <= E_DOMAIN_15)
		base = (uint32_t *)((size_t)DEVAPC_SYS0_D0_APC_0 +
				domain_num * 0x40 + apc_index * 4);

	else if (slave_type == E_DAPC_SRAMROM_SLAVE &&
		module <= SLAVE_SRAMROM_MAX_INDEX &&
		domain_num <= E_DOMAIN_8)
		base = (uint32_t *)((size_t)DEVAPC_SYS1_D0_APC_0 +
				domain_num * 0x40 + apc_index * 4);

	else if (slave_type == E_DAPC_MD_SLAVE &&
		module <= SLAVE_MD_MAX_INDEX &&
		domain_num <= E_DOMAIN_3)
		base = (uint32_t *)((size_t)DEVAPC_SYS2_D0_APC_0 +
				domain_num * 0x40 + apc_index * 4);
	else {
		ERROR("out of boundary, %s=0x%x, %s=0x%x, %s=0x%x\n",
			"slave_type", slave_type,
			"module", module,
			"domain_num", domain_num);

		return DEVAPC_ERR_OUT_OF_BOUNDARY;
	}

	if (base != NULL) {
		devapc_writel((devapc_readl(base) & clr_bit), base);
		devapc_writel((devapc_readl(base) | set_bit), base);

		return DEVAPC_OK;
	}

	return DEVAPC_ERR_GENERIC;
}

static uint32_t set_master_transaction(enum DAPC_MASTER_TYPE master_type,
		uint32_t master_index, enum E_TRANSACTION transaction_type)
{
	uint32_t *base = NULL;
	uint32_t master_set_index;

	master_set_index = master_index % (MOD_NO_IN_1_DEVAPC * 2);

	if (master_type == E_DAPC_MASTER &&
		master_index <= MASTER_INFRA_MAX_INDEX)
		base = (uint32_t *)DEVAPC_INFRA_MAS_SEC_0;

	else if (master_type == E_DAPC_INFRACFG_AO_MASTER &&
		master_index <= MASTER_INFRACFG_AO_MAX_INDEX)
		base = (uint32_t *)INFRACFG_AO_DEVAPC_MAS_SEC;

	else {
		ERROR("out of boundary, %s=0x%x, %s=0x%x\n",
			"master_type", master_type,
			"master_index", master_index);

		return DEVAPC_ERR_OUT_OF_BOUNDARY;
	}

	if (base != NULL) {
		if (transaction_type == NON_SECURE_TRANSACTION)
			devapc_writel(devapc_readl(base) &
				(0xFFFFFFFF ^ (0x1 << master_set_index)), base);
		else if (transaction_type == SECURE_TRANSACTION)
			devapc_writel(devapc_readl(base) |
				(0x1 << master_set_index), base);
		else {
			ERROR("transaction=0x%x is not supported!\n",
					transaction_type);
			return DEVAPC_ERR_PERMISSION_NOT_SUPPORTED;
		}

		return DEVAPC_OK;
	}

	return DEVAPC_ERR_GENERIC;
}

static uint32_t set_master_domain(enum DAPC_MASTER_TYPE master_type,
		uint32_t master_index, enum E_MASK_DOM dom_num)
{
	uint32_t *base = NULL;
	uint32_t master_reg_index;
	uint32_t master_set_index;
	uint32_t set_bit;

	if (master_type == E_DAPC_MASTER &&
		master_index <= MASTER_INFRA_MAX_INDEX &&
		dom_num <= E_DOMAIN_15) {
		master_reg_index = master_index / MAS_DOM_NO_IN_1_DEVAPC;
		master_set_index = master_index % MAS_DOM_NO_IN_1_DEVAPC;
		set_bit = dom_num << (master_set_index * 8);

		base = (uint32_t *)((size_t)DEVAPC_INFRA_MAS_DOM_0 + master_reg_index * 4);

	} else if (master_type == E_DAPC_INFRACFG_AO_MASTER &&
		master_index <= MASTER_INFRACFG_AO_MAX_INDEX &&
		dom_num <= E_DOMAIN_15) {
		set_bit = dom_num << (master_index * 4);
		base = (uint32_t *)INFRACFG_AO_DEVAPC_MAS_DOM;

	} else {
		ERROR("out of boundary, %s=0x%x, %s=0x%x, %s=0x%x\n",
			"master_type", master_type,
			"master_index", master_index,
			"dom_num", dom_num);

		return DEVAPC_ERR_OUT_OF_BOUNDARY;
	}

	if (base != NULL) {
		devapc_writel(devapc_readl(base) | set_bit, base);
		return DEVAPC_OK;
	}

	return DEVAPC_ERR_GENERIC;
}

static void dump_infra_apc(void)
{
	/* d: domain, i: register number */
	int d, i;

	for (d = 0; d < DEVAPC_INFRA_DOM_MAX; d++) {
		if (d != E_DOMAIN_0 && d != E_DOMAIN_1 && d != E_DOMAIN_9)
			continue;

		for (i = 0; i < DEVAPC_INFRA_APC_NUM; i++) {
			INFO("(INFRA)SYS0_D%d_APC_%d(0x%x) = 0x%x\n", d, i,
				((size_t)DEVAPC_SYS0_D0_APC_0 + 0x40 * d + i * 4),
				devapc_readl((size_t)DEVAPC_SYS0_D0_APC_0 +
					0x40 * d + i * 4)
			);
		}
	}

	INFO("(INFRA)MAS_SEC_0 = 0x%x\n", devapc_readl(DEVAPC_INFRA_MAS_SEC_0));
}

static void dump_sramrom_apc(void)
{
	/* d: domain, i: register number */
	int d, i;

	for (d = 0 ; d < DEVAPC_SRAMROM_DOM_MAX ; d++)
		for (i = 0; i < DEVAPC_SRAMROM_APC_NUM; i++) {
			INFO("(MD)SYS1_D%d_APC_%d(0x%x) = 0x%x\n", d, i,
				((size_t)DEVAPC_SYS1_D0_APC_0 + 0x40 * d + i * 4),
				devapc_readl((size_t)DEVAPC_SYS1_D0_APC_0 +
					0x40 * d + i * 4)
			);
		}
}

static void dump_md_apc(void)
{
	/* d: domain, i: register number */
	int d, i;

	for (d = 0 ; d < DEVAPC_MD_DOM_MAX ; d++) {
		if (d != E_DOMAIN_0)
			continue;

		for (i = 0; i < DEVAPC_MD_APC_NUM; i++) {
			INFO("(MD)SYS2_D%d_APC_%d(0x%x) = 0x%x\n", d, i,
				((size_t)DEVAPC_SYS2_D0_APC_0 + 0x40 * d + i * 4),
				devapc_readl((size_t)DEVAPC_SYS2_D0_APC_0 +
					0x40 * d + i * 4)
			);
		}
	}
}

/*
static void dump_pms_info(void)
{
	INFO("[PMS]AP2MD1_PMS_CTRL_EN = 0x%x\n", devapc_readl(AP2MD1_PMS_CTRL_EN));
	INFO("[PMS]AP2MD1_PMS_CTRL_EN_LOCK = 0x%x\n", devapc_readl(AP2MD1_PMS_CTRL_EN_LOCK));
}
*/

static void print_vio_mask_sta(void)
{
	int i;

	for (i = 0; i < VIO_MASK_STA_NUM; i++) {
		INFO("%s: (%d:0x%x) %s: (%d:0x%x)\n",
			"INFRA VIO_MASK", i,
			devapc_readl(DEVAPC_PD_INFRA_VIO_MASK(i)),
			"INFRA VIO_STA", i,
			devapc_readl(DEVAPC_PD_INFRA_VIO_STA(i))
		);
	}
}

static void unmask_infra_module(uint32_t module)
{
	uint32_t apc_index = 0;
	uint32_t apc_bit_index = 0;

	if (module > VIOLATION_MAX_INDEX) {
		ERROR("%s: module overflow!\n", __func__);
		return;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	devapc_writel(devapc_readl(DEVAPC_PD_INFRA_VIO_MASK(apc_index)) &
		(0xFFFFFFFF ^ (1 << apc_bit_index)),
		DEVAPC_PD_INFRA_VIO_MASK(apc_index));
}

static uint32_t clear_infra_vio_status(uint32_t module)
{
	uint32_t apc_index = 0;
	uint32_t apc_bit_index = 0;

	if (module > VIOLATION_MAX_INDEX) {
		ERROR("%s: module overflow!\n", __func__);
		return DEVAPC_ERR_OUT_OF_BOUNDARY;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	devapc_writel(0x1 << apc_bit_index,
		DEVAPC_PD_INFRA_VIO_STA(apc_index));

	return 0;
}

static int check_infra_vio_status(uint32_t module)
{
	uint32_t apc_index = 0;
	uint32_t apc_bit_index = 0;

	if (module > VIOLATION_MAX_INDEX) {
		ERROR("%s: module overflow!\n", __func__);
		return DEVAPC_ERR_OUT_OF_BOUNDARY;
	}

	apc_index = module / (MOD_NO_IN_1_DEVAPC * 2);
	apc_bit_index = module % (MOD_NO_IN_1_DEVAPC * 2);

	if (devapc_readl(DEVAPC_PD_INFRA_VIO_STA(apc_index)) &
			(0x1 << apc_bit_index))
		return VIOLATION_TRIGGERED;

	return 0;
}

static bool vio_shift_sta_handler(void)
{
	uint32_t vio_shift_sta = 0;

	vio_shift_sta = devapc_readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA);
	INFO("(Pre)VIO_SHIFT_STA = 0x%x\n", vio_shift_sta);

	if (vio_shift_sta) {
		devapc_writel(vio_shift_sta, DEVAPC_PD_INFRA_VIO_SHIFT_STA);
		INFO("(Post)clear VIO_SHIFT_STA = 0x%x\n",
			devapc_readl(DEVAPC_PD_INFRA_VIO_SHIFT_STA));
	}

	return vio_shift_sta ? true : false;
}

static void devapc_irq_handler(void)
{
	int i;
	uint64_t vio_sta;
	uint64_t vio_addr;

	INFO("enter %s...\n", __func__);
	print_vio_mask_sta();
	if (!vio_shift_sta_handler()) {
		INFO("violation is not triggered or is clean before\n");
		return;
	}

	for (i = 0; i <= VIOLATION_MAX_INDEX; i++) {
		if (check_infra_vio_status(i) == VIOLATION_TRIGGERED) {
			INFO("violation is triggered, vio_idx=%d\n", i);
			if (i == SRAMROM_VIO_INDEX)
				handle_sramrom_vio(&vio_sta, &vio_addr);

			clear_infra_vio_status(i);
		}
	}

	print_vio_mask_sta();
}

#ifdef DEVAPC_UT
static void devapc_ut(void)
{
	INFO("test violation...\n");

	INFO("read blocked reg = 0x%x\n",
		devapc_readl((unsigned int *)BLOCKED_REG_BASE));

	devapc_writel(0xdead, (unsigned int *)BLOCKED_REG_BASE);
	INFO("read blocked reg = 0x%x\n",
		devapc_readl((unsigned int *)BLOCKED_REG_BASE));

	devapc_irq_handler();

	mt_irq_dump_status(DEVAPC_IRQ_BIT_ID);
}
#endif

static void devapc_set_dom(void)
{

	/* For EMI workaround, need set SPM as secure master to rw EMI self
	 * test start/end address
	 */
	set_master_transaction(E_DAPC_MASTER, MASTER_SPM_SEC_INDEX, SECURE_TRANSACTION);

	INFO("Setup master secure: %s = (0x%x), %s = (0x%x)\n",
			"DEVAPC_INFRA_MAS_SEC_0",
			devapc_readl(DEVAPC_INFRA_MAS_SEC_0),
			"INFRACFG_AO_DEVAPC_MAS_SEC",
			devapc_readl(INFRACFG_AO_DEVAPC_MAS_SEC)
	);

/******************************************************************************/
/* Infra Master Domain Setting */

	/* Set MD1 to DOMAIN1 */
	set_master_domain(E_DAPC_INFRACFG_AO_MASTER, MASTER_MD_INDEX, E_DOMAIN_1);

	/* Set SPM to DOMAIN9 */
	set_master_domain(E_DAPC_MASTER, MASTER_SPM_DOM_INDEX, E_DOMAIN_9);

	INFO("Setup master domain MAS_DOM_x: (0x%x), (0x%x), (0x%x), (0x%x), (0x%x)\n",
			devapc_readl(DEVAPC_INFRA_MAS_DOM_0),
			devapc_readl(DEVAPC_INFRA_MAS_DOM_1),
			devapc_readl(DEVAPC_INFRA_MAS_DOM_2),
			devapc_readl(DEVAPC_INFRA_MAS_DOM_3),
			devapc_readl(DEVAPC_INFRA_MAS_DOM_4)
	);

	INFO("Setup master domain INFRACFG_AO_DEVAPC_MAS_DOM: (0x%x)\n",
			devapc_readl(INFRACFG_AO_DEVAPC_MAS_DOM)
	);

/******************************************************************************/
/*Infra Domain Remap Setting */
/* Infra: no domain remap */

/* SRAMROM Domain Remap Setting */
	devapc_writel(MASTER_DOM_RMP_INIT, DEVAPC_SRAMROM_DOM_REMAP_0_0);
	devapc_writel(MASTER_DOM_RMP_INIT, DEVAPC_SRAMROM_DOM_REMAP_0_1);

	reg_set_field(DEVAPC_SRAMROM_DOM_REMAP_0_0, SRAMROM_RMP_AP, E_DOMAIN_0); // remap Infra domain 0 to SRAMROM domain 0

	/* HW BUG: DOM_REMAP reg cannot read */
/*	INFO("Setup SRAMROM domain remap: (0x%x), (0x%x)\n",
			devapc_readl(DEVAPC_SRAMROM_DOM_REMAP_0_0),
			devapc_readl(DEVAPC_SRAMROM_DOM_REMAP_0_1)
	);
*/
/* MD Domain Remap Setting */
	devapc_writel(MASTER_DOM_RMP_INIT, DEVAPC_SRAMROM_DOM_REMAP_1_0);
	reg_set_field(DEVAPC_SRAMROM_DOM_REMAP_1_0, MD_RMP_AP, E_DOMAIN_0); // remap Infra domain 0 to MD domain 0

	/* HW BUG: DOM_REMAP reg cannot read */
/*	INFO("Setup MD domain remap: (0x%x)\n",
			devapc_readl(DEVAPC_SRAMROM_DOM_REMAP_1_0)
	);
*/
}

static void devapc_set_apc(void)
{
	uint32_t module_index, dom_index;
	uint32_t infra_size = sizeof(D_APC_INFRA_Devices)/
		sizeof(struct INFRA_PERI_DEVICE_INFO);
	uint32_t md_size = sizeof(D_APC_MD_Devices)/
		sizeof(struct MD_DEVICE_INFO);
	enum E_MASK_DOM dom_id;

	/* Initial Permission */
	INFO("Walk initial permission setting - Infra_peri\n");
	for (module_index = 0; module_index < infra_size; module_index++) {
		set_module_apc(E_DAPC_INFRA_SLAVE, module_index, E_DOMAIN_0,
				D_APC_INFRA_Devices[module_index].d0_permission);	/* APMCU */
		set_module_apc(E_DAPC_INFRA_SLAVE, module_index, E_DOMAIN_1,
				D_APC_INFRA_Devices[module_index].d1_permission);	/* MD1 */
		set_module_apc(E_DAPC_INFRA_SLAVE, module_index, E_DOMAIN_9,
				D_APC_INFRA_Devices[module_index].d9_permission);	/* SPM */

		/* block all reserved domain */
		for (dom_id = E_DOMAIN_0; dom_id <= E_DOMAIN_15; dom_id++) {
			if (dom_id != E_DOMAIN_0 && dom_id != E_DOMAIN_1 &&
				dom_id != E_DOMAIN_9)
				set_module_apc(E_DAPC_INFRA_SLAVE, module_index,
						dom_id, E_FORBIDDEN);
		}
	}

	INFO("Walk initial permission setting - SRAMROM\n");
	for (dom_index = 0; dom_index <= E_DOMAIN_8; dom_index++) {
		if (dom_index == E_DOMAIN_0)
			set_module_apc(E_DAPC_SRAMROM_SLAVE,
					DEVAPC_CTRL_SRAMROM_INDEX,
					dom_index, E_NO_PROTECTION);
		else
			set_module_apc(E_DAPC_SRAMROM_SLAVE,
					DEVAPC_CTRL_SRAMROM_INDEX,
					dom_index, E_FORBIDDEN);
	}

	/* MD 2nd level protection */
	INFO("Walk initial permission setting - MD\n");
	for (module_index = 0; module_index < md_size; module_index++) {
		set_module_apc(E_DAPC_MD_SLAVE, module_index, E_DOMAIN_0,
				D_APC_MD_Devices[module_index].d0_permission);
		/* block all reserved domain */
		set_module_apc(E_DAPC_MD_SLAVE, module_index, E_DOMAIN_1,
				E_FORBIDDEN);
		set_module_apc(E_DAPC_MD_SLAVE, module_index, E_DOMAIN_2,
				E_FORBIDDEN);
		set_module_apc(E_DAPC_MD_SLAVE, module_index, E_DOMAIN_3,
				E_FORBIDDEN);
	}

	/* Dump Permission */
	dump_infra_apc();
	dump_sramrom_apc();
	dump_md_apc();

	/* Set CG to Secure (INFRACFG_AO) */
	devapc_writel(devapc_readl(INFRA_AO_SEC_CG_CON0) | SEJ_CG_PROTECT_BIT,
			INFRA_AO_SEC_CG_CON0);
	devapc_writel(devapc_readl(INFRA_AO_SEC_CG_CON1) | TRNG_CG_PROTECT_BIT,
			INFRA_AO_SEC_CG_CON1);
	devapc_writel(devapc_readl(INFRA_AO_SEC_CG_CON1) | DEVAPC_CG_PROTECT_BIT,
			INFRA_AO_SEC_CG_CON1);


	INFO("INFRA_APC_CON = 0x%x\n", devapc_readl(DEVAPC_INFRA_APC_CON));

	/* Set PMS(MD devapc) enable */
//	devapc_writel(devapc_readl(AP2MD1_PMS_CTRL_EN) | 0x1, AP2MD1_PMS_CTRL_EN);
//	devapc_writel(devapc_readl(AP2MD1_PMS_CTRL_EN_LOCK) | 0x1, AP2MD1_PMS_CTRL_EN_LOCK);
//	dump_pms_info();

	if (vio_shift_sta_handler()) {
		INFO("violation happened after %s\n", __func__);
		print_vio_mask_sta();
	}

#ifdef DEVAPC_UT
	devapc_ut();
#endif

}

static void sramrom_set_apc(void)
{
	INFO("[Pre] SRAMROM SEC_ADDR:0x%x, SEC_ADDR1:0x%x, SEC_ADDR2:0x%x\n",
		devapc_readl(SRAMROM_SEC_ADDR),
		devapc_readl(SRAMROM_SEC_ADDR1),
		devapc_readl(SRAMROM_SEC_ADDR2)
	);
	INFO("[Pre] SRAMROM SEC_CTRL:0x%x, SEC_CTRL2:0x%x, SEC_CTRL5:0x%x, SEC_CTRL6:0x%x\n",
		devapc_readl(SRAMROM_SEC_CTRL), devapc_readl(SRAMROM_SEC_CTRL2),
		devapc_readl(SRAMROM_SEC_CTRL5), devapc_readl(SRAMROM_SEC_CTRL6)
	);

	/* Split 2 regions: 96KB(SEC), 96KB(NON_SEC) */
	TZ_SRAMROM_SET_REGION_0_SIZE_KB(96);

	/* Set APC to region 0 */
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC0_DOM0_MASK, PERMIT_S_RW_NS_BLOCK    << SRAMROM_SEC_CTRL_SEC0_DOM0_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC0_DOM1_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC0_DOM1_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC0_DOM2_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC0_DOM2_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC0_DOM3_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC0_DOM3_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC0_DOM4_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC0_DOM4_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC0_DOM5_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC0_DOM5_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC0_DOM6_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC0_DOM6_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC0_DOM7_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC0_DOM7_SHIFT);

	/* Set APC to region 1 */
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC1_DOM0_MASK, PERMIT_S_RW_NS_RW       << SRAMROM_SEC_CTRL_SEC1_DOM0_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC1_DOM1_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC1_DOM1_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC1_DOM2_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC1_DOM2_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL, SRAMROM_SEC_CTRL_SEC1_DOM3_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL_SEC1_DOM3_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC1_DOM4_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC1_DOM4_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC1_DOM5_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC1_DOM5_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC1_DOM6_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC1_DOM6_SHIFT);
	reg_set_field(SRAMROM_SEC_CTRL2, SRAMROM_SEC_CTRL2_SEC1_DOM7_MASK, PERMIT_S_BLOCK_NS_BLOCK << SRAMROM_SEC_CTRL2_SEC1_DOM7_SHIFT);

	/* Enable region 0 & region 1 protection */
	DRV_SetReg32(SRAMROM_SEC_ADDR, (0x1<<SRAMROM_SEC_ADDR_SEC0_SEC_EN));
	DRV_SetReg32(SRAMROM_SEC_ADDR, (0x1<<SRAMROM_SEC_ADDR_SEC1_SEC_EN));

	INFO("[Post] SRAMROM SEC_ADDR:0x%x, SEC_ADDR1:0x%x, SEC_ADDR2:0x%x\n",
		devapc_readl(SRAMROM_SEC_ADDR),
		devapc_readl(SRAMROM_SEC_ADDR1),
		devapc_readl(SRAMROM_SEC_ADDR2)
	);
	INFO("[Post] SRAMROM SEC_CTRL:0x%x, SEC_CTRL2:0x%x, SEC_CTRL5:0x%x, SEC_CTRL6:0x%x\n",
		devapc_readl(SRAMROM_SEC_CTRL), devapc_readl(SRAMROM_SEC_CTRL2),
		devapc_readl(SRAMROM_SEC_CTRL5), devapc_readl(SRAMROM_SEC_CTRL6)
	);

}

void devapc_init(void)
{
	uint64_t sramrom_vio_sta;
	uint64_t sramrom_vio_addr;
	int i;

	INFO("%s...\n", __func__);
	/* Enable Devapc */
	/* Lock DEVAPC_AO/DEVAPC_CON to secure access only */
	devapc_writel(0x80000001, DEVAPC_INFRA_APC_CON);
	devapc_writel(0x1, INFRACFG_AO_DEVAPC_CON);
	devapc_writel(0x80000000, DEVAPC_PD_INFRA_APC_CON);

	INFO("(Post) DEVAPC_INFRA_APC_CON:0x%x\n",
			devapc_readl(DEVAPC_INFRA_APC_CON));
	INFO("(Post) INFRACFG_AO_DEVAPC_CON:0x%x\n",
			devapc_readl(INFRACFG_AO_DEVAPC_CON));
	INFO("(Post) DEVAPC_PD_INFRA_APC_CON:0x%x\n",
			devapc_readl(DEVAPC_PD_INFRA_APC_CON));

	/* clr sramrom vio if any */
	handle_sramrom_vio(&sramrom_vio_sta, &sramrom_vio_addr);

//	print_vio_mask_sta();
	if (!vio_shift_sta_handler())
		INFO("no violation happened before init\n");

	/* clear vio_status & unmask vio_mask */
	INFO("clear vio_staus & unmask modules\n");
	for (i = 0; i <= VIOLATION_MAX_INDEX; i++) {
		clear_infra_vio_status(i);
		unmask_infra_module(i);
	}

//	print_vio_mask_sta();

	devapc_set_dom();
#if CFG_DEVAPC_SET_PROTECT
	devapc_set_apc();
#else
	INFO("Skip to set APC\n");
#endif
	sramrom_set_apc();

	INFO("%s done\n", __func__);
}

/* It should clear SRAMROM vio before clear DEVAPC vio */
int handle_sramrom_vio(uint64_t *vio_sta, uint64_t *vio_addr)
{
	int sramrom = -1;

	if (devapc_readl(SRAMROM_SEC_VIO_STA) & SRAM_SEC_VIO_BIT) {		/* SRAM part */
		INFO("SRAM violation is triggered\n");

		sramrom = 1;
		*vio_sta = devapc_readl(SRAMROM_SEC_VIO_STA);
		*vio_addr = devapc_readl(SRAMROM_SEC_VIO_ADDR);

		INFO("(Pre) SRAMROM_SEC_VIO_STA: 0x%x, VIO_ADDR: 0x%x\n",
			(uint32_t)*vio_sta, (uint32_t)*vio_addr);

		devapc_writel(0x1, SRAMROM_SEC_VIO_CLR);

		INFO("(Post) SRAMROM_SEC_VIO_STA: 0x%x, VIO_ADDR: 0x%x\n",
			devapc_readl(SRAMROM_SEC_VIO_STA),
			devapc_readl(SRAMROM_SEC_VIO_ADDR));

	} else if (devapc_readl(SRAMROM_ROM_SEC_VIO_STA) & ROM_SEC_VIO_BIT) {	/* ROM part */
		INFO("ROM violation is triggered\n");

		sramrom = 0;
		*vio_sta = devapc_readl(SRAMROM_ROM_SEC_VIO_STA);
		*vio_addr = devapc_readl(SRAMROM_ROM_SEC_VIO_ADDR);

		INFO("(Pre) SRAMROM_ROM_SEC_VIO_STA: 0x%x, VIO_ADDR: 0x%x\n",
			(uint32_t)*vio_sta, (uint32_t)*vio_addr);

		devapc_writel(0x1, SRAMROM_ROM_SEC_VIO_CLR);

		INFO("(Post) SRAMROM_ROM_SEC_VIO_STA: 0x%x, VIO_ADDR: 0x%x\n",
			devapc_readl(SRAMROM_ROM_SEC_VIO_STA),
			devapc_readl(SRAMROM_ROM_SEC_VIO_ADDR));
	} else {
		INFO("SRAMROM violation is not triggered\n");
	}

	return sramrom;
}

unsigned int devapc_perm_get(int type, int domain, int index)
{
	int d = domain; /* domain id */
	int reg = index / MOD_NO_IN_1_DEVAPC; /* register num */
	unsigned int value = 0xdeadbeaf;

	INFO("[INFRA] default value is 0x%x\n", value);
	if (type == E_DAPC_INFRA_SLAVE) {
		if (index > SLAVE_INFRA_MAX_INDEX)
			INFO("[INFRA] slave index out of range\n");
		else if (d > E_DOMAIN_15)
			INFO("[INFRA] domain id out of range\n");
		else {
			value = devapc_readl(DEVAPC_SYS0_D0_APC_0 + 0x40 * d + reg * 4);
			INFO("[INFRA] SYS0_D%d_APC_%d = 0x%x\n", d, reg, value);
		}
	} else if (type == E_DAPC_SRAMROM_SLAVE) {
		if (index > SLAVE_SRAMROM_MAX_INDEX)
			INFO("[SRAMROM] slave index out of range\n");
		else if (d > E_DOMAIN_8)
			INFO("[SRAMROM] domain id out of range\n");
		else {
			value = devapc_readl(DEVAPC_SYS1_D0_APC_0 + 0x40 * d + reg * 4);
			INFO("[SRAMROM] SYS1_D%d_APC_%d = 0x%x\n", d, reg, value);
		}
	} else if (type == E_DAPC_MD_SLAVE) {
		if (index > SLAVE_MD_MAX_INDEX)
			INFO("[MD] slave index out of range\n");
		else if (d > E_DOMAIN_3)
			INFO("[MD] domain id out of range\n");
		else {
			value = devapc_readl(DEVAPC_SYS2_D0_APC_0 + 0x40 * d + reg * 4);
			INFO("[MD] SYS2_D%d_APC_%d = 0x%x\n", d, reg, value);
		}
	} else {
		dump_infra_apc();
		dump_sramrom_apc();
		dump_md_apc();

//		INFO("%s: dump PMS(DEVAPC) reg:\n", __func__);
//		dump_pms_info();
	}

	return value;
}

void tz_dapc_sec_init(void)
{
	//nothing
}

void tz_dapc_sec_postinit(void)
{
	//nothing
}

void tz_apc_common_init(void)
{
	devapc_init();
}

void tz_apc_common_postinit(void)
{
	//nothing
}

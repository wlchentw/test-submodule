/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef __DTS_MT8512_PINFUNC_H
#define __DTS_MT8512_PINFUNC_H

#include <dt-bindings/pinctrl/mt65xx.h>

#define MT8512_PIN_0_GPIO0__FUNC_GPIO0 (MTK_PIN_NO(0) | 0)
#define MT8512_PIN_0_GPIO0__FUNC_DMIC_CLK0 (MTK_PIN_NO(0) | 1)
#define MT8512_PIN_0_GPIO0__FUNC_CONN_MCU_AICE_TMSC (MTK_PIN_NO(0) | 3)
#define MT8512_PIN_0_GPIO0__FUNC_UDI_NTRST_XI (MTK_PIN_NO(0) | 5)
#define MT8512_PIN_0_GPIO0__FUNC_DBG_MON_A_0 (MTK_PIN_NO(0) | 7)

#define MT8512_PIN_1_GPIO1__FUNC_GPIO1 (MTK_PIN_NO(1) | 0)
#define MT8512_PIN_1_GPIO1__FUNC_DMIC_DAT0 (MTK_PIN_NO(1) | 1)
#define MT8512_PIN_1_GPIO1__FUNC_CONN_MCU_AICE_TCKC (MTK_PIN_NO(1) | 3)
#define MT8512_PIN_1_GPIO1__FUNC_UDI_TMS_XI (MTK_PIN_NO(1) | 5)
#define MT8512_PIN_1_GPIO1__FUNC_DBG_MON_A_1 (MTK_PIN_NO(1) | 7)

#define MT8512_PIN_2_GPIO2__FUNC_GPIO2 (MTK_PIN_NO(2) | 0)
#define MT8512_PIN_2_GPIO2__FUNC_EPDC_D11 (MTK_PIN_NO(2) | 1)
#define MT8512_PIN_2_GPIO2__FUNC_UDI_TCK_XI (MTK_PIN_NO(2) | 5)
#define MT8512_PIN_2_GPIO2__FUNC_DBG_MON_A_2 (MTK_PIN_NO(2) | 7)

#define MT8512_PIN_3_GPIO3__FUNC_GPIO3 (MTK_PIN_NO(3) | 0)
#define MT8512_PIN_3_GPIO3__FUNC_EPDC_D13 (MTK_PIN_NO(3) | 1)
#define MT8512_PIN_3_GPIO3__FUNC_UDI_TDI_XI (MTK_PIN_NO(3) | 5)
#define MT8512_PIN_3_GPIO3__FUNC_DBG_MON_A_3 (MTK_PIN_NO(3) | 7)

#define MT8512_PIN_4_GPIO4__FUNC_GPIO4 (MTK_PIN_NO(4) | 0)
#define MT8512_PIN_4_GPIO4__FUNC_EPDC_D0 (MTK_PIN_NO(4) | 1)
#define MT8512_PIN_4_GPIO4__FUNC_UDI_TDO (MTK_PIN_NO(4) | 5)
#define MT8512_PIN_4_GPIO4__FUNC_DBG_MON_A_4 (MTK_PIN_NO(4) | 7)

#define MT8512_PIN_5_GPIO5__FUNC_GPIO5 (MTK_PIN_NO(5) | 0)
#define MT8512_PIN_5_GPIO5__FUNC_EPDC_D2 (MTK_PIN_NO(5) | 1)
#define MT8512_PIN_5_GPIO5__FUNC_DBG_MON_A_5 (MTK_PIN_NO(5) | 7)

#define MT8512_PIN_6_GPIO6__FUNC_GPIO6 (MTK_PIN_NO(6) | 0)
#define MT8512_PIN_6_GPIO6__FUNC_EPDC_D4 (MTK_PIN_NO(6) | 1)
#define MT8512_PIN_6_GPIO6__FUNC_SDA0_0 (MTK_PIN_NO(6) | 2)
#define MT8512_PIN_6_GPIO6__FUNC_DMIC_DAT4 (MTK_PIN_NO(6) | 3)
#define MT8512_PIN_6_GPIO6__FUNC_DBG_MON_A_6 (MTK_PIN_NO(6) | 7)

#define MT8512_PIN_7_GPIO7__FUNC_GPIO7 (MTK_PIN_NO(7) | 0)
#define MT8512_PIN_7_GPIO7__FUNC_EPDC_SDOE (MTK_PIN_NO(7) | 1)
#define MT8512_PIN_7_GPIO7__FUNC_SCL0_0 (MTK_PIN_NO(7) | 2)
#define MT8512_PIN_7_GPIO7__FUNC_DMIC_DAT5 (MTK_PIN_NO(7) | 3)
#define MT8512_PIN_7_GPIO7__FUNC_WIFI_TXD (MTK_PIN_NO(7) | 4)
#define MT8512_PIN_7_GPIO7__FUNC_IRRX (MTK_PIN_NO(7) | 5)
#define MT8512_PIN_7_GPIO7__FUNC_DBG_MON_A_7 (MTK_PIN_NO(7) | 7)

#define MT8512_PIN_8_GPIO8__FUNC_GPIO8 (MTK_PIN_NO(8) | 0)
#define MT8512_PIN_8_GPIO8__FUNC_KPCOL0 (MTK_PIN_NO(8) | 1)
#define MT8512_PIN_8_GPIO8__FUNC_SPDIF_IN0 (MTK_PIN_NO(8) | 2)
#define MT8512_PIN_8_GPIO8__FUNC_DMIC_DAT6 (MTK_PIN_NO(8) | 3)
#define MT8512_PIN_8_GPIO8__FUNC_CONN_UART0_RXD (MTK_PIN_NO(8) | 4)
#define MT8512_PIN_8_GPIO8__FUNC_IRRX (MTK_PIN_NO(8) | 5)

#define MT8512_PIN_9_GPIO9__FUNC_GPIO9 (MTK_PIN_NO(9) | 0)
#define MT8512_PIN_9_GPIO9__FUNC_KPCOL1 (MTK_PIN_NO(9) | 1)
#define MT8512_PIN_9_GPIO9__FUNC_SPDIF_IN1 (MTK_PIN_NO(9) | 2)
#define MT8512_PIN_9_GPIO9__FUNC_DMIC_DAT7 (MTK_PIN_NO(9) | 3)
#define MT8512_PIN_9_GPIO9__FUNC_CONN_UART0_TXD (MTK_PIN_NO(9) | 4)
#define MT8512_PIN_9_GPIO9__FUNC_WIFI_TXD (MTK_PIN_NO(9) | 5)
#define MT8512_PIN_9_GPIO9__FUNC_SRCLKENA0 (MTK_PIN_NO(9) | 6)

#define MT8512_PIN_10_GPIO10__FUNC_GPIO10 (MTK_PIN_NO(10) | 0)
#define MT8512_PIN_10_GPIO10__FUNC_KPCOL2 (MTK_PIN_NO(10) | 1)
#define MT8512_PIN_10_GPIO10__FUNC_SPDIF_IN2 (MTK_PIN_NO(10) | 2)
#define MT8512_PIN_10_GPIO10__FUNC_DMIC_CLK2 (MTK_PIN_NO(10) | 3)

#define MT8512_PIN_11_GPIO11__FUNC_GPIO11 (MTK_PIN_NO(11) | 0)
#define MT8512_PIN_11_GPIO11__FUNC_KPCOL3 (MTK_PIN_NO(11) | 1)
#define MT8512_PIN_11_GPIO11__FUNC_DMIC_CLK3 (MTK_PIN_NO(11) | 3)

#define MT8512_PIN_12_GPIO12__FUNC_GPIO12 (MTK_PIN_NO(12) | 0)
#define MT8512_PIN_12_GPIO12__FUNC_PWM_4 (MTK_PIN_NO(12) | 3)
#define MT8512_PIN_12_GPIO12__FUNC_CONN_MCU_DBGACK_N (MTK_PIN_NO(12) | 5)
#define MT8512_PIN_12_GPIO12__FUNC_ANT_SEL4 (MTK_PIN_NO(12) | 6)

#define MT8512_PIN_13_GPIO13__FUNC_GPIO13 (MTK_PIN_NO(13) | 0)
#define MT8512_PIN_13_GPIO13__FUNC_PWM_5 (MTK_PIN_NO(13) | 3)
#define MT8512_PIN_13_GPIO13__FUNC_CONN_MCU_DBGI_N (MTK_PIN_NO(13) | 5)
#define MT8512_PIN_13_GPIO13__FUNC_ANT_SEL5 (MTK_PIN_NO(13) | 6)

#define MT8512_PIN_14_GPIO14__FUNC_GPIO14 (MTK_PIN_NO(14) | 0)
#define MT8512_PIN_14_GPIO14__FUNC_PWM_6 (MTK_PIN_NO(14) | 3)
#define MT8512_PIN_14_GPIO14__FUNC_I2SIN_DAT3 (MTK_PIN_NO(14) | 4)
#define MT8512_PIN_14_GPIO14__FUNC_CONN_MCU_TCK (MTK_PIN_NO(14) | 5)
#define MT8512_PIN_14_GPIO14__FUNC_ANT_SEL6 (MTK_PIN_NO(14) | 6)

#define MT8512_PIN_15_GPIO15__FUNC_GPIO15 (MTK_PIN_NO(15) | 0)
#define MT8512_PIN_15_GPIO15__FUNC_SPI_DAT0_NOR (MTK_PIN_NO(15) | 1)
#define MT8512_PIN_15_GPIO15__FUNC_SPI_DAT0_NAND (MTK_PIN_NO(15) | 2)
#define MT8512_PIN_15_GPIO15__FUNC_I2SIN_DAT2 (MTK_PIN_NO(15) | 4)
#define MT8512_PIN_15_GPIO15__FUNC_CONN_MCU_TDI (MTK_PIN_NO(15) | 5)
#define MT8512_PIN_15_GPIO15__FUNC_ANT_SEL7 (MTK_PIN_NO(15) | 6)

#define MT8512_PIN_16_GPIO16__FUNC_GPIO16 (MTK_PIN_NO(16) | 0)
#define MT8512_PIN_16_GPIO16__FUNC_SPI_DAT1_NOR (MTK_PIN_NO(16) | 1)
#define MT8512_PIN_16_GPIO16__FUNC_SPI_DAT1_NAND (MTK_PIN_NO(16) | 2)
#define MT8512_PIN_16_GPIO16__FUNC_I2SIN_DAT1 (MTK_PIN_NO(16) | 4)
#define MT8512_PIN_16_GPIO16__FUNC_CONN_MCU_TRST_B (MTK_PIN_NO(16) | 5)
#define MT8512_PIN_16_GPIO16__FUNC_ANT_SEL0 (MTK_PIN_NO(16) | 6)

#define MT8512_PIN_17_GPIO17__FUNC_GPIO17 (MTK_PIN_NO(17) | 0)
#define MT8512_PIN_17_GPIO17__FUNC_SPI_DAT2_NOR (MTK_PIN_NO(17) | 1)
#define MT8512_PIN_17_GPIO17__FUNC_SPI_DAT2_NAND (MTK_PIN_NO(17) | 2)
#define MT8512_PIN_17_GPIO17__FUNC_I2SIN_DAT0 (MTK_PIN_NO(17) | 4)
#define MT8512_PIN_17_GPIO17__FUNC_CONN_MCU_TMS (MTK_PIN_NO(17) | 5)
#define MT8512_PIN_17_GPIO17__FUNC_ANT_SEL1 (MTK_PIN_NO(17) | 6)

#define MT8512_PIN_18_GPIO18__FUNC_GPIO18 (MTK_PIN_NO(18) | 0)
#define MT8512_PIN_18_GPIO18__FUNC_SPI_DAT3_NOR (MTK_PIN_NO(18) | 1)
#define MT8512_PIN_18_GPIO18__FUNC_SPI_DAT3_NAND (MTK_PIN_NO(18) | 2)
#define MT8512_PIN_18_GPIO18__FUNC_I2SIN_MCK (MTK_PIN_NO(18) | 4)
#define MT8512_PIN_18_GPIO18__FUNC_CONN_MCU_TDO (MTK_PIN_NO(18) | 5)
#define MT8512_PIN_18_GPIO18__FUNC_ANT_SEL2 (MTK_PIN_NO(18) | 6)

#define MT8512_PIN_19_GPIO19__FUNC_GPIO19 (MTK_PIN_NO(19) | 0)
#define MT8512_PIN_19_GPIO19__FUNC_SPI_CSB_NOR (MTK_PIN_NO(19) | 1)
#define MT8512_PIN_19_GPIO19__FUNC_SPI_CSB_NAND (MTK_PIN_NO(19) | 2)
#define MT8512_PIN_19_GPIO19__FUNC_I2SIN_LRCK (MTK_PIN_NO(19) | 4)
#define MT8512_PIN_19_GPIO19__FUNC_ANT_SEL3 (MTK_PIN_NO(19) | 6)

#define MT8512_PIN_20_GPIO20__FUNC_GPIO20 (MTK_PIN_NO(20) | 0)
#define MT8512_PIN_20_GPIO20__FUNC_SPI_CLK_NOR (MTK_PIN_NO(20) | 1)
#define MT8512_PIN_20_GPIO20__FUNC_SPI_CLK_NAND (MTK_PIN_NO(20) | 2)
#define MT8512_PIN_20_GPIO20__FUNC_I2SIN_BCK (MTK_PIN_NO(20) | 4)

#define MT8512_PIN_21_AUDIO_SYNC__FUNC_GPIO21 (MTK_PIN_NO(21) | 0)
#define MT8512_PIN_21_AUDIO_SYNC__FUNC_CONN_WF_CTRL2 (MTK_PIN_NO(21) | 1)
#define MT8512_PIN_21_AUDIO_SYNC__FUNC_TSF_IN (MTK_PIN_NO(21) | 6)
#define MT8512_PIN_21_AUDIO_SYNC__FUNC_DBG_MON_B_16 (MTK_PIN_NO(21) | 7)

#define MT8512_PIN_22_WIFI_INTB__FUNC_GPIO22 (MTK_PIN_NO(22) | 0)
#define MT8512_PIN_22_WIFI_INTB__FUNC_CONN_WF_CTRL0 (MTK_PIN_NO(22) | 1)
#define MT8512_PIN_22_WIFI_INTB__FUNC_DBG_MON_B_17 (MTK_PIN_NO(22) | 7)

#define MT8512_PIN_23_BT_INTB__FUNC_GPIO23 (MTK_PIN_NO(23) | 0)
#define MT8512_PIN_23_BT_INTB__FUNC_CONN_BT_CLK (MTK_PIN_NO(23) | 1)
#define MT8512_PIN_23_BT_INTB__FUNC_DVFSRC_EXT_REQ (MTK_PIN_NO(23) | 6)
#define MT8512_PIN_23_BT_INTB__FUNC_DBG_MON_B_18 (MTK_PIN_NO(23) | 7)

#define MT8512_PIN_24_BT_STEREO__FUNC_GPIO24 (MTK_PIN_NO(24) | 0)
#define MT8512_PIN_24_BT_STEREO__FUNC_CONN_WF_CTRL1 (MTK_PIN_NO(24) | 1)
#define MT8512_PIN_24_BT_STEREO__FUNC_DBG_MON_B_19 (MTK_PIN_NO(24) | 7)

#define MT8512_PIN_25_RSTNB__FUNC_GPIO25 (MTK_PIN_NO(25) | 0)
#define MT8512_PIN_25_RSTNB__FUNC_CONN_BT_DATA (MTK_PIN_NO(25) | 1)
#define MT8512_PIN_25_RSTNB__FUNC_DBG_MON_B_20 (MTK_PIN_NO(25) | 7)

#define MT8512_PIN_26_USB_ID__FUNC_GPIO26 (MTK_PIN_NO(26) | 0)
#define MT8512_PIN_26_USB_ID__FUNC_USB_IDDIG (MTK_PIN_NO(26) | 1)
#define MT8512_PIN_26_USB_ID__FUNC_DVFSRC_EXT_REQ (MTK_PIN_NO(26) | 6)
#define MT8512_PIN_26_USB_ID__FUNC_DBG_MON_B_21 (MTK_PIN_NO(26) | 7)

#define MT8512_PIN_27_USB_DRV__FUNC_GPIO27 (MTK_PIN_NO(27) | 0)
#define MT8512_PIN_27_USB_DRV__FUNC_USB_DRVVBUS (MTK_PIN_NO(27) | 1)
#define MT8512_PIN_27_USB_DRV__FUNC_ADSP_JTAG_TMS (MTK_PIN_NO(27) | 5)
#define MT8512_PIN_27_USB_DRV__FUNC_DBG_MON_B_22 (MTK_PIN_NO(27) | 7)

#define MT8512_PIN_28_EINT_GAUGEING__FUNC_GPIO28 (MTK_PIN_NO(28) | 0)
#define MT8512_PIN_28_EINT_GAUGEING__FUNC_URXD2 (MTK_PIN_NO(28) | 1)
#define MT8512_PIN_28_EINT_GAUGEING__FUNC_PWM_0 (MTK_PIN_NO(28) | 2)
#define MT8512_PIN_28_EINT_GAUGEING__FUNC_KPCOL0 (MTK_PIN_NO(28) | 3)
#define MT8512_PIN_28_EINT_GAUGEING__FUNC_ADSP_JTAG_TCK (MTK_PIN_NO(28) | 5)
#define MT8512_PIN_28_EINT_GAUGEING__FUNC_DBG_MON_B_23 (MTK_PIN_NO(28) | 7)

#define MT8512_PIN_29_CHG_IRQ__FUNC_GPIO29 (MTK_PIN_NO(29) | 0)
#define MT8512_PIN_29_CHG_IRQ__FUNC_UTXD2 (MTK_PIN_NO(29) | 1)
#define MT8512_PIN_29_CHG_IRQ__FUNC_PWM_1 (MTK_PIN_NO(29) | 2)
#define MT8512_PIN_29_CHG_IRQ__FUNC_KPCOL1 (MTK_PIN_NO(29) | 3)
#define MT8512_PIN_29_CHG_IRQ__FUNC_ADSP_JTAG_TDI (MTK_PIN_NO(29) | 5)
#define MT8512_PIN_29_CHG_IRQ__FUNC_DBG_MON_B_24 (MTK_PIN_NO(29) | 7)

#define MT8512_PIN_30_CHG_OTG__FUNC_GPIO30 (MTK_PIN_NO(30) | 0)
#define MT8512_PIN_30_CHG_OTG__FUNC_UTXD0 (MTK_PIN_NO(30) | 1)
#define MT8512_PIN_30_CHG_OTG__FUNC_PWM_2 (MTK_PIN_NO(30) | 2)
#define MT8512_PIN_30_CHG_OTG__FUNC_KPCOL2 (MTK_PIN_NO(30) | 3)
#define MT8512_PIN_30_CHG_OTG__FUNC_SDA1_0 (MTK_PIN_NO(30) | 4)
#define MT8512_PIN_30_CHG_OTG__FUNC_ADSP_JTAG_TDO (MTK_PIN_NO(30) | 5)
#define MT8512_PIN_30_CHG_OTG__FUNC_DBG_MON_B_12 (MTK_PIN_NO(30) | 7)

#define MT8512_PIN_31_CHG_CEB__FUNC_GPIO31 (MTK_PIN_NO(31) | 0)
#define MT8512_PIN_31_CHG_CEB__FUNC_URXD0 (MTK_PIN_NO(31) | 1)
#define MT8512_PIN_31_CHG_CEB__FUNC_PWM_3 (MTK_PIN_NO(31) | 2)
#define MT8512_PIN_31_CHG_CEB__FUNC_KPCOL3 (MTK_PIN_NO(31) | 3)
#define MT8512_PIN_31_CHG_CEB__FUNC_SCL1_0 (MTK_PIN_NO(31) | 4)
#define MT8512_PIN_31_CHG_CEB__FUNC_ADSP_JTAG_TRST (MTK_PIN_NO(31) | 5)
#define MT8512_PIN_31_CHG_CEB__FUNC_DBG_MON_B_6 (MTK_PIN_NO(31) | 7)

#define MT8512_PIN_32_FL_EN__FUNC_GPIO32 (MTK_PIN_NO(32) | 0)
#define MT8512_PIN_32_FL_EN__FUNC_SPDIF_IN0 (MTK_PIN_NO(32) | 2)
#define MT8512_PIN_32_FL_EN__FUNC_SPDIF_IN1 (MTK_PIN_NO(32) | 3)
#define MT8512_PIN_32_FL_EN__FUNC_SPDIF_IN2 (MTK_PIN_NO(32) | 4)
#define MT8512_PIN_32_FL_EN__FUNC_IRRX (MTK_PIN_NO(32) | 5)

#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_GPIO33 (MTK_PIN_NO(33) | 0)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_KPROW0 (MTK_PIN_NO(33) | 1)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_PWM_4 (MTK_PIN_NO(33) | 2)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_PCM_CLK (MTK_PIN_NO(33) | 4)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_WATCHDOG (MTK_PIN_NO(33) | 5)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_SPI_CSB (MTK_PIN_NO(33) | 6)
#define MT8512_PIN_33_WAN_SMS_RDY__FUNC_DBG_MON_B_3 (MTK_PIN_NO(33) | 7)

#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_GPIO34 (MTK_PIN_NO(34) | 0)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_KPROW1 (MTK_PIN_NO(34) | 1)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_PWM_5 (MTK_PIN_NO(34) | 2)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_PCM_SYNC (MTK_PIN_NO(34) | 4)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_SRCLKENA0 (MTK_PIN_NO(34) | 5)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_SPI_CLK (MTK_PIN_NO(34) | 6)
#define MT8512_PIN_34_SOC2WAN_RESET__FUNC_DBG_MON_B_2 (MTK_PIN_NO(34) | 7)

#define MT8512_PIN_35_WAN_FM_RDY__FUNC_GPIO35 (MTK_PIN_NO(35) | 0)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_KPCOL0 (MTK_PIN_NO(35) | 1)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_PWM_6 (MTK_PIN_NO(35) | 2)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_PCM_RX (MTK_PIN_NO(35) | 4)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_CONN_UART0_RXD (MTK_PIN_NO(35) | 5)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_SPI_MI (MTK_PIN_NO(35) | 6)
#define MT8512_PIN_35_WAN_FM_RDY__FUNC_DBG_MON_B_14 (MTK_PIN_NO(35) | 7)

#define MT8512_PIN_36_WAN_DIS__FUNC_GPIO36 (MTK_PIN_NO(36) | 0)
#define MT8512_PIN_36_WAN_DIS__FUNC_KPCOL1 (MTK_PIN_NO(36) | 1)
#define MT8512_PIN_36_WAN_DIS__FUNC_PCM_TX (MTK_PIN_NO(36) | 4)
#define MT8512_PIN_36_WAN_DIS__FUNC_CONN_UART0_TXD (MTK_PIN_NO(36) | 5)
#define MT8512_PIN_36_WAN_DIS__FUNC_SPI_MO (MTK_PIN_NO(36) | 6)
#define MT8512_PIN_36_WAN_DIS__FUNC_DBG_MON_B_15 (MTK_PIN_NO(36) | 7)

#define MT8512_PIN_37_WAN_VBUS_EN__FUNC_GPIO37 (MTK_PIN_NO(37) | 0)

#define MT8512_PIN_38_WAN_VBAT_EN__FUNC_GPIO38 (MTK_PIN_NO(38) | 0)

#define MT8512_PIN_39_WAN_PWR_EN__FUNC_GPIO39 (MTK_PIN_NO(39) | 0)

#define MT8512_PIN_40_KPROW0__FUNC_GPIO40 (MTK_PIN_NO(40) | 0)
#define MT8512_PIN_40_KPROW0__FUNC_KPROW0 (MTK_PIN_NO(40) | 1)
#define MT8512_PIN_40_KPROW0__FUNC_KPCOL2 (MTK_PIN_NO(40) | 2)

#define MT8512_PIN_41_KPROW1__FUNC_GPIO41 (MTK_PIN_NO(41) | 0)
#define MT8512_PIN_41_KPROW1__FUNC_KPROW1 (MTK_PIN_NO(41) | 1)
#define MT8512_PIN_41_KPROW1__FUNC_KPCOL3 (MTK_PIN_NO(41) | 2)

#define MT8512_PIN_42_KPCOL0__FUNC_GPIO42 (MTK_PIN_NO(42) | 0)
#define MT8512_PIN_42_KPCOL0__FUNC_KPCOL0 (MTK_PIN_NO(42) | 1)
#define MT8512_PIN_42_KPCOL0__FUNC_DBG_MON_A_31 (MTK_PIN_NO(42) | 7)

#define MT8512_PIN_43_KPCOL1__FUNC_GPIO43 (MTK_PIN_NO(43) | 0)
#define MT8512_PIN_43_KPCOL1__FUNC_KPCOL1 (MTK_PIN_NO(43) | 1)
#define MT8512_PIN_43_KPCOL1__FUNC_DBG_MON_A_32 (MTK_PIN_NO(43) | 7)

#define MT8512_PIN_44_PWM0__FUNC_GPIO44 (MTK_PIN_NO(44) | 0)
#define MT8512_PIN_44_PWM0__FUNC_PWM_0 (MTK_PIN_NO(44) | 1)
#define MT8512_PIN_44_PWM0__FUNC_SPDIF_IN0 (MTK_PIN_NO(44) | 5)
#define MT8512_PIN_44_PWM0__FUNC_DBG_MON_A_25 (MTK_PIN_NO(44) | 7)

#define MT8512_PIN_45_PWM1__FUNC_GPIO45 (MTK_PIN_NO(45) | 0)
#define MT8512_PIN_45_PWM1__FUNC_PWM_1 (MTK_PIN_NO(45) | 1)
#define MT8512_PIN_45_PWM1__FUNC_SPDIF_IN1 (MTK_PIN_NO(45) | 5)
#define MT8512_PIN_45_PWM1__FUNC_DBG_MON_A_26 (MTK_PIN_NO(45) | 7)

#define MT8512_PIN_46_PWM2__FUNC_GPIO46 (MTK_PIN_NO(46) | 0)
#define MT8512_PIN_46_PWM2__FUNC_PWM_2 (MTK_PIN_NO(46) | 1)
#define MT8512_PIN_46_PWM2__FUNC_SPDIF_IN2 (MTK_PIN_NO(46) | 5)
#define MT8512_PIN_46_PWM2__FUNC_DSP_TEST_CK (MTK_PIN_NO(46) | 6)
#define MT8512_PIN_46_PWM2__FUNC_DBG_MON_A_27 (MTK_PIN_NO(46) | 7)

#define MT8512_PIN_47_PWM3__FUNC_GPIO47 (MTK_PIN_NO(47) | 0)
#define MT8512_PIN_47_PWM3__FUNC_PWM_3 (MTK_PIN_NO(47) | 1)
#define MT8512_PIN_47_PWM3__FUNC_IRRX (MTK_PIN_NO(47) | 5)
#define MT8512_PIN_47_PWM3__FUNC_DVFSRC_EXT_REQ (MTK_PIN_NO(47) | 6)
#define MT8512_PIN_47_PWM3__FUNC_DBG_MON_A_28 (MTK_PIN_NO(47) | 7)

#define MT8512_PIN_48_JTMS__FUNC_GPIO48 (MTK_PIN_NO(48) | 0)
#define MT8512_PIN_48_JTMS__FUNC_JTMS (MTK_PIN_NO(48) | 1)
#define MT8512_PIN_48_JTMS__FUNC_DFD_TMS_XI (MTK_PIN_NO(48) | 2)
#define MT8512_PIN_48_JTMS__FUNC_SPIS_CSB (MTK_PIN_NO(48) | 4)
#define MT8512_PIN_48_JTMS__FUNC_CONN_MCU_TMS (MTK_PIN_NO(48) | 5)

#define MT8512_PIN_49_JTCK__FUNC_GPIO49 (MTK_PIN_NO(49) | 0)
#define MT8512_PIN_49_JTCK__FUNC_JTCK (MTK_PIN_NO(49) | 1)
#define MT8512_PIN_49_JTCK__FUNC_DFD_TCK_XI (MTK_PIN_NO(49) | 2)
#define MT8512_PIN_49_JTCK__FUNC_SPIS_CLK (MTK_PIN_NO(49) | 4)
#define MT8512_PIN_49_JTCK__FUNC_CONN_MCU_TCK (MTK_PIN_NO(49) | 5)

#define MT8512_PIN_50_JTDI__FUNC_GPIO50 (MTK_PIN_NO(50) | 0)
#define MT8512_PIN_50_JTDI__FUNC_JTDI (MTK_PIN_NO(50) | 1)
#define MT8512_PIN_50_JTDI__FUNC_DFD_TDI_XI (MTK_PIN_NO(50) | 2)
#define MT8512_PIN_50_JTDI__FUNC_SPIS_SO (MTK_PIN_NO(50) | 4)
#define MT8512_PIN_50_JTDI__FUNC_CONN_MCU_TDI (MTK_PIN_NO(50) | 5)

#define MT8512_PIN_51_JTDO__FUNC_GPIO51 (MTK_PIN_NO(51) | 0)
#define MT8512_PIN_51_JTDO__FUNC_JTDO (MTK_PIN_NO(51) | 1)
#define MT8512_PIN_51_JTDO__FUNC_DFD_TDO (MTK_PIN_NO(51) | 2)
#define MT8512_PIN_51_JTDO__FUNC_SPIS_SI (MTK_PIN_NO(51) | 4)
#define MT8512_PIN_51_JTDO__FUNC_CONN_MCU_TDO (MTK_PIN_NO(51) | 5)

#define MT8512_PIN_52_URXD0__FUNC_GPIO52 (MTK_PIN_NO(52) | 0)
#define MT8512_PIN_52_URXD0__FUNC_URXD0 (MTK_PIN_NO(52) | 1)
#define MT8512_PIN_52_URXD0__FUNC_UTXD0 (MTK_PIN_NO(52) | 2)
#define MT8512_PIN_52_URXD0__FUNC_DSP_URXD (MTK_PIN_NO(52) | 5)

#define MT8512_PIN_53_UTXD0__FUNC_GPIO53 (MTK_PIN_NO(53) | 0)
#define MT8512_PIN_53_UTXD0__FUNC_UTXD0 (MTK_PIN_NO(53) | 1)
#define MT8512_PIN_53_UTXD0__FUNC_URXD0 (MTK_PIN_NO(53) | 2)
#define MT8512_PIN_53_UTXD0__FUNC_DSP_UTXD (MTK_PIN_NO(53) | 5)
#define MT8512_PIN_53_UTXD0__FUNC_DVFSRC_EXT_REQ (MTK_PIN_NO(53) | 6)
#define MT8512_PIN_53_UTXD0__FUNC_DBG_MON_B_7 (MTK_PIN_NO(53) | 7)

#define MT8512_PIN_54_URXD1__FUNC_GPIO54 (MTK_PIN_NO(54) | 0)
#define MT8512_PIN_54_URXD1__FUNC_URXD1 (MTK_PIN_NO(54) | 1)
#define MT8512_PIN_54_URXD1__FUNC_KPCOL0 (MTK_PIN_NO(54) | 2)
#define MT8512_PIN_54_URXD1__FUNC_PWM_0 (MTK_PIN_NO(54) | 3)
#define MT8512_PIN_54_URXD1__FUNC_CONN_TCXO_REQ (MTK_PIN_NO(54) | 4)
#define MT8512_PIN_54_URXD1__FUNC_ANT_SEL0 (MTK_PIN_NO(54) | 6)
#define MT8512_PIN_54_URXD1__FUNC_DBG_MON_B_8 (MTK_PIN_NO(54) | 7)

#define MT8512_PIN_55_UTXD1__FUNC_GPIO55 (MTK_PIN_NO(55) | 0)
#define MT8512_PIN_55_UTXD1__FUNC_UTXD1 (MTK_PIN_NO(55) | 1)
#define MT8512_PIN_55_UTXD1__FUNC_KPCOL1 (MTK_PIN_NO(55) | 2)
#define MT8512_PIN_55_UTXD1__FUNC_PWM_1 (MTK_PIN_NO(55) | 3)
#define MT8512_PIN_55_UTXD1__FUNC_CONN_MCU_TRST_B (MTK_PIN_NO(55) | 5)
#define MT8512_PIN_55_UTXD1__FUNC_ANT_SEL1 (MTK_PIN_NO(55) | 6)
#define MT8512_PIN_55_UTXD1__FUNC_DBG_MON_B_9 (MTK_PIN_NO(55) | 7)

#define MT8512_PIN_56_URTS1__FUNC_GPIO56 (MTK_PIN_NO(56) | 0)
#define MT8512_PIN_56_URTS1__FUNC_URTS1 (MTK_PIN_NO(56) | 1)
#define MT8512_PIN_56_URTS1__FUNC_KPCOL2 (MTK_PIN_NO(56) | 2)
#define MT8512_PIN_56_URTS1__FUNC_PWM_2 (MTK_PIN_NO(56) | 3)
#define MT8512_PIN_56_URTS1__FUNC_DSP_URXD (MTK_PIN_NO(56) | 4)
#define MT8512_PIN_56_URTS1__FUNC_CONN_MCU_DBGACK_N (MTK_PIN_NO(56) | 5)
#define MT8512_PIN_56_URTS1__FUNC_ANT_SEL2 (MTK_PIN_NO(56) | 6)
#define MT8512_PIN_56_URTS1__FUNC_DBG_MON_B_10 (MTK_PIN_NO(56) | 7)

#define MT8512_PIN_57_UCTS1__FUNC_GPIO57 (MTK_PIN_NO(57) | 0)
#define MT8512_PIN_57_UCTS1__FUNC_UCTS1 (MTK_PIN_NO(57) | 1)
#define MT8512_PIN_57_UCTS1__FUNC_KPCOL3 (MTK_PIN_NO(57) | 2)
#define MT8512_PIN_57_UCTS1__FUNC_PWM_3 (MTK_PIN_NO(57) | 3)
#define MT8512_PIN_57_UCTS1__FUNC_DSP_UTXD (MTK_PIN_NO(57) | 4)
#define MT8512_PIN_57_UCTS1__FUNC_CONN_MCU_DBGI_N (MTK_PIN_NO(57) | 5)
#define MT8512_PIN_57_UCTS1__FUNC_ANT_SEL3 (MTK_PIN_NO(57) | 6)
#define MT8512_PIN_57_UCTS1__FUNC_DBG_MON_B_11 (MTK_PIN_NO(57) | 7)

#define MT8512_PIN_58_RTC32K_CK__FUNC_GPIO58 (MTK_PIN_NO(58) | 0)
#define MT8512_PIN_58_RTC32K_CK__FUNC_RTC32K_CK (MTK_PIN_NO(58) | 1)
#define MT8512_PIN_58_RTC32K_CK__FUNC_PWM_4 (MTK_PIN_NO(58) | 3)

#define MT8512_PIN_59_PMIC_DVS_REQ0__FUNC_GPIO59 (MTK_PIN_NO(59) | 0)
#define MT8512_PIN_59_PMIC_DVS_REQ0__FUNC_PWM_5 (MTK_PIN_NO(59) | 3)
#define MT8512_PIN_59_PMIC_DVS_REQ0__FUNC_DBG_MON_B_4 (MTK_PIN_NO(59) | 7)

#define MT8512_PIN_60_PMIC_DVS_REQ1__FUNC_GPIO60 (MTK_PIN_NO(60) | 0)
#define MT8512_PIN_60_PMIC_DVS_REQ1__FUNC_PWM_6 (MTK_PIN_NO(60) | 3)
#define MT8512_PIN_60_PMIC_DVS_REQ1__FUNC_CONN_TCXO_REQ (MTK_PIN_NO(60) | 4)
#define MT8512_PIN_60_PMIC_DVS_REQ1__FUNC_DBG_MON_B_5 (MTK_PIN_NO(60) | 7)

#define MT8512_PIN_61_WATCHDOG__FUNC_GPIO61 (MTK_PIN_NO(61) | 0)
#define MT8512_PIN_61_WATCHDOG__FUNC_WATCHDOG (MTK_PIN_NO(61) | 1)

#define MT8512_PIN_62_PMIC_INT__FUNC_GPIO62 (MTK_PIN_NO(62) | 0)
#define MT8512_PIN_62_PMIC_INT__FUNC_SRCLKENA1 (MTK_PIN_NO(62) | 1)

#define MT8512_PIN_63_SUSPEND__FUNC_GPIO63 (MTK_PIN_NO(63) | 0)
#define MT8512_PIN_63_SUSPEND__FUNC_SRCLKENA0 (MTK_PIN_NO(63) | 1)

#define MT8512_PIN_64_SDA0__FUNC_GPIO64 (MTK_PIN_NO(64) | 0)
#define MT8512_PIN_64_SDA0__FUNC_SDA0_0 (MTK_PIN_NO(64) | 1)
#define MT8512_PIN_64_SDA0__FUNC_SDA1_0 (MTK_PIN_NO(64) | 2)
#define MT8512_PIN_64_SDA0__FUNC_DBG_MON_A_29 (MTK_PIN_NO(64) | 7)

#define MT8512_PIN_65_SCL0__FUNC_GPIO65 (MTK_PIN_NO(65) | 0)
#define MT8512_PIN_65_SCL0__FUNC_SCL0_0 (MTK_PIN_NO(65) | 1)
#define MT8512_PIN_65_SCL0__FUNC_SCL1_0 (MTK_PIN_NO(65) | 2)
#define MT8512_PIN_65_SCL0__FUNC_DBG_MON_A_30 (MTK_PIN_NO(65) | 7)

#define MT8512_PIN_66_SDA1__FUNC_GPIO66 (MTK_PIN_NO(66) | 0)
#define MT8512_PIN_66_SDA1__FUNC_SDA1_0 (MTK_PIN_NO(66) | 1)
#define MT8512_PIN_66_SDA1__FUNC_SDA0_0 (MTK_PIN_NO(66) | 2)
#define MT8512_PIN_66_SDA1__FUNC_DBG_SDA (MTK_PIN_NO(66) | 6)

#define MT8512_PIN_67_SCL1__FUNC_GPIO67 (MTK_PIN_NO(67) | 0)
#define MT8512_PIN_67_SCL1__FUNC_SCL1_0 (MTK_PIN_NO(67) | 1)
#define MT8512_PIN_67_SCL1__FUNC_SCL0_0 (MTK_PIN_NO(67) | 2)
#define MT8512_PIN_67_SCL1__FUNC_DBG_SCL (MTK_PIN_NO(67) | 6)
#define MT8512_PIN_67_SCL1__FUNC_DBG_MON_B_0 (MTK_PIN_NO(67) | 7)

#define MT8512_PIN_68_SDA2__FUNC_GPIO68 (MTK_PIN_NO(68) | 0)
#define MT8512_PIN_68_SDA2__FUNC_SDA2_0 (MTK_PIN_NO(68) | 1)
#define MT8512_PIN_68_SDA2__FUNC_SDA1_0 (MTK_PIN_NO(68) | 2)
#define MT8512_PIN_68_SDA2__FUNC_DBG_MON_B_25 (MTK_PIN_NO(68) | 7)

#define MT8512_PIN_69_SCL2__FUNC_GPIO69 (MTK_PIN_NO(69) | 0)
#define MT8512_PIN_69_SCL2__FUNC_SCL2_0 (MTK_PIN_NO(69) | 1)
#define MT8512_PIN_69_SCL2__FUNC_SCL1_0 (MTK_PIN_NO(69) | 2)
#define MT8512_PIN_69_SCL2__FUNC_DBG_MON_B_26 (MTK_PIN_NO(69) | 7)

#define MT8512_PIN_70_MSDC1_CMD__FUNC_GPIO70 (MTK_PIN_NO(70) | 0)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_MSDC1_CMD (MTK_PIN_NO(70) | 1)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_UDI_TMS_XI (MTK_PIN_NO(70) | 3)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_I2SO_BCK (MTK_PIN_NO(70) | 4)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_DMIC_CLK0 (MTK_PIN_NO(70) | 5)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_DFD_TMS_XI (MTK_PIN_NO(70) | 6)
#define MT8512_PIN_70_MSDC1_CMD__FUNC_ADSP_JTAG_TMS (MTK_PIN_NO(70) | 7)

#define MT8512_PIN_71_MSDC1_CLK__FUNC_GPIO71 (MTK_PIN_NO(71) | 0)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_MSDC1_CLK (MTK_PIN_NO(71) | 1)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_UDI_TCK_XI (MTK_PIN_NO(71) | 3)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_I2SO_LRCK (MTK_PIN_NO(71) | 4)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_DMIC_DAT0 (MTK_PIN_NO(71) | 5)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_DFD_TCK_XI (MTK_PIN_NO(71) | 6)
#define MT8512_PIN_71_MSDC1_CLK__FUNC_ADSP_JTAG_TCK (MTK_PIN_NO(71) | 7)

#define MT8512_PIN_72_MSDC1_DAT0__FUNC_GPIO72 (MTK_PIN_NO(72) | 0)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_MSDC1_DAT0 (MTK_PIN_NO(72) | 1)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_UDI_TDI_XI (MTK_PIN_NO(72) | 3)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_I2SO_MCK (MTK_PIN_NO(72) | 4)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_DMIC_DAT1 (MTK_PIN_NO(72) | 5)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_DFD_TDI_XI (MTK_PIN_NO(72) | 6)
#define MT8512_PIN_72_MSDC1_DAT0__FUNC_ADSP_JTAG_TDI (MTK_PIN_NO(72) | 7)

#define MT8512_PIN_73_MSDC1_DAT1__FUNC_GPIO73 (MTK_PIN_NO(73) | 0)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_MSDC1_DAT1 (MTK_PIN_NO(73) | 1)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_UDI_TDO (MTK_PIN_NO(73) | 3)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_I2SO_DAT0 (MTK_PIN_NO(73) | 4)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_DMIC_CLK1 (MTK_PIN_NO(73) | 5)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_DFD_TDO (MTK_PIN_NO(73) | 6)
#define MT8512_PIN_73_MSDC1_DAT1__FUNC_ADSP_JTAG_TDO (MTK_PIN_NO(73) | 7)

#define MT8512_PIN_74_MSDC1_DAT2__FUNC_GPIO74 (MTK_PIN_NO(74) | 0)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_MSDC1_DAT2 (MTK_PIN_NO(74) | 1)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_UDI_NTRST_XI (MTK_PIN_NO(74) | 3)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_I2SO_DAT1 (MTK_PIN_NO(74) | 4)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_DMIC_DAT2 (MTK_PIN_NO(74) | 5)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_DFD_NTRST_XI (MTK_PIN_NO(74) | 6)
#define MT8512_PIN_74_MSDC1_DAT2__FUNC_ADSP_JTAG_TRST (MTK_PIN_NO(74) | 7)

#define MT8512_PIN_75_MSDC1_DAT3__FUNC_GPIO75 (MTK_PIN_NO(75) | 0)
#define MT8512_PIN_75_MSDC1_DAT3__FUNC_MSDC1_DAT3 (MTK_PIN_NO(75) | 1)
#define MT8512_PIN_75_MSDC1_DAT3__FUNC_I2SO_DAT2 (MTK_PIN_NO(75) | 4)
#define MT8512_PIN_75_MSDC1_DAT3__FUNC_DMIC_DAT3 (MTK_PIN_NO(75) | 5)

#define MT8512_PIN_76_MSDC0_DAT7__FUNC_GPIO76 (MTK_PIN_NO(76) | 0)
#define MT8512_PIN_76_MSDC0_DAT7__FUNC_MSDC0_DAT7 (MTK_PIN_NO(76) | 1)
#define MT8512_PIN_76_MSDC0_DAT7__FUNC_SPI_DAT0_NOR (MTK_PIN_NO(76) | 2)
#define MT8512_PIN_76_MSDC0_DAT7__FUNC_SPI_DAT0_NAND (MTK_PIN_NO(76) | 3)
#define MT8512_PIN_76_MSDC0_DAT7__FUNC_CONN_MCU_AICE_TMSC (MTK_PIN_NO(76) | 5)

#define MT8512_PIN_77_MSDC0_DAT6__FUNC_GPIO77 (MTK_PIN_NO(77) | 0)
#define MT8512_PIN_77_MSDC0_DAT6__FUNC_MSDC0_DAT6 (MTK_PIN_NO(77) | 1)
#define MT8512_PIN_77_MSDC0_DAT6__FUNC_SPI_DAT1_NOR (MTK_PIN_NO(77) | 2)
#define MT8512_PIN_77_MSDC0_DAT6__FUNC_SPI_DAT1_NAND (MTK_PIN_NO(77) | 3)
#define MT8512_PIN_77_MSDC0_DAT6__FUNC_CONN_MCU_AICE_TCKC (MTK_PIN_NO(77) | 5)
#define MT8512_PIN_77_MSDC0_DAT6__FUNC_DBG_MON_B_1 (MTK_PIN_NO(77) | 7)

#define MT8512_PIN_78_MSDC0_DAT5__FUNC_GPIO78 (MTK_PIN_NO(78) | 0)
#define MT8512_PIN_78_MSDC0_DAT5__FUNC_MSDC0_DAT5 (MTK_PIN_NO(78) | 1)
#define MT8512_PIN_78_MSDC0_DAT5__FUNC_SPI_DAT2_NOR (MTK_PIN_NO(78) | 2)
#define MT8512_PIN_78_MSDC0_DAT5__FUNC_SPI_DAT2_NAND (MTK_PIN_NO(78) | 3)
#define MT8512_PIN_78_MSDC0_DAT5__FUNC_DBG_MON_B_27 (MTK_PIN_NO(78) | 7)

#define MT8512_PIN_79_MSDC0_DAT4__FUNC_GPIO79 (MTK_PIN_NO(79) | 0)
#define MT8512_PIN_79_MSDC0_DAT4__FUNC_MSDC0_DAT4 (MTK_PIN_NO(79) | 1)
#define MT8512_PIN_79_MSDC0_DAT4__FUNC_SPI_DAT3_NOR (MTK_PIN_NO(79) | 2)
#define MT8512_PIN_79_MSDC0_DAT4__FUNC_SPI_DAT3_NAND (MTK_PIN_NO(79) | 3)
#define MT8512_PIN_79_MSDC0_DAT4__FUNC_DBG_MON_B_28 (MTK_PIN_NO(79) | 7)

#define MT8512_PIN_80_MSDC0_RSTB__FUNC_GPIO80 (MTK_PIN_NO(80) | 0)
#define MT8512_PIN_80_MSDC0_RSTB__FUNC_MSDC0_RSTB (MTK_PIN_NO(80) | 1)
#define MT8512_PIN_80_MSDC0_RSTB__FUNC_SPI_CLK_NOR (MTK_PIN_NO(80) | 2)
#define MT8512_PIN_80_MSDC0_RSTB__FUNC_SPI_CSB_NAND (MTK_PIN_NO(80) | 3)

#define MT8512_PIN_81_MSDC0_CMD__FUNC_GPIO81 (MTK_PIN_NO(81) | 0)
#define MT8512_PIN_81_MSDC0_CMD__FUNC_MSDC0_CMD (MTK_PIN_NO(81) | 1)
#define MT8512_PIN_81_MSDC0_CMD__FUNC_IRRX (MTK_PIN_NO(81) | 5)
#define MT8512_PIN_81_MSDC0_CMD__FUNC_DBG_MON_B_29 (MTK_PIN_NO(81) | 7)

#define MT8512_PIN_82_MSDC0_CLK__FUNC_GPIO82 (MTK_PIN_NO(82) | 0)
#define MT8512_PIN_82_MSDC0_CLK__FUNC_MSDC0_CLK (MTK_PIN_NO(82) | 1)
#define MT8512_PIN_82_MSDC0_CLK__FUNC_SPI_CSB_NOR (MTK_PIN_NO(82) | 2)
#define MT8512_PIN_82_MSDC0_CLK__FUNC_SPI_CLK_NAND (MTK_PIN_NO(82) | 3)

#define MT8512_PIN_83_MSDC0_DAT3__FUNC_GPIO83 (MTK_PIN_NO(83) | 0)
#define MT8512_PIN_83_MSDC0_DAT3__FUNC_MSDC0_DAT3 (MTK_PIN_NO(83) | 1)
#define MT8512_PIN_83_MSDC0_DAT3__FUNC_KPROW0 (MTK_PIN_NO(83) | 2)
#define MT8512_PIN_83_MSDC0_DAT3__FUNC_PWM_3 (MTK_PIN_NO(83) | 3)
#define MT8512_PIN_83_MSDC0_DAT3__FUNC_DBG_MON_B_30 (MTK_PIN_NO(83) | 7)

#define MT8512_PIN_84_MSDC0_DAT2__FUNC_GPIO84 (MTK_PIN_NO(84) | 0)
#define MT8512_PIN_84_MSDC0_DAT2__FUNC_MSDC0_DAT2 (MTK_PIN_NO(84) | 1)
#define MT8512_PIN_84_MSDC0_DAT2__FUNC_KPROW1 (MTK_PIN_NO(84) | 2)
#define MT8512_PIN_84_MSDC0_DAT2__FUNC_PWM_4 (MTK_PIN_NO(84) | 3)
#define MT8512_PIN_84_MSDC0_DAT2__FUNC_DBG_MON_B_13 (MTK_PIN_NO(84) | 7)

#define MT8512_PIN_85_MSDC0_DAT1__FUNC_GPIO85 (MTK_PIN_NO(85) | 0)
#define MT8512_PIN_85_MSDC0_DAT1__FUNC_MSDC0_DAT1 (MTK_PIN_NO(85) | 1)
#define MT8512_PIN_85_MSDC0_DAT1__FUNC_KPCOL0 (MTK_PIN_NO(85) | 2)
#define MT8512_PIN_85_MSDC0_DAT1__FUNC_PWM_5 (MTK_PIN_NO(85) | 3)
#define MT8512_PIN_85_MSDC0_DAT1__FUNC_DBG_MON_B_31 (MTK_PIN_NO(85) | 7)

#define MT8512_PIN_86_MSDC0_DAT0__FUNC_GPIO86 (MTK_PIN_NO(86) | 0)
#define MT8512_PIN_86_MSDC0_DAT0__FUNC_MSDC0_DAT0 (MTK_PIN_NO(86) | 1)
#define MT8512_PIN_86_MSDC0_DAT0__FUNC_KPCOL1 (MTK_PIN_NO(86) | 2)
#define MT8512_PIN_86_MSDC0_DAT0__FUNC_PWM_6 (MTK_PIN_NO(86) | 3)
#define MT8512_PIN_86_MSDC0_DAT0__FUNC_DBG_MON_B_32 (MTK_PIN_NO(86) | 7)

#define MT8512_PIN_87_SPDIF__FUNC_GPIO87 (MTK_PIN_NO(87) | 0)
#define MT8512_PIN_87_SPDIF__FUNC_SPDIF_IN0 (MTK_PIN_NO(87) | 1)
#define MT8512_PIN_87_SPDIF__FUNC_SPDIF_IN1 (MTK_PIN_NO(87) | 2)
#define MT8512_PIN_87_SPDIF__FUNC_EPDC_D9 (MTK_PIN_NO(87) | 3)
#define MT8512_PIN_87_SPDIF__FUNC_SPDIF_IN2 (MTK_PIN_NO(87) | 4)
#define MT8512_PIN_87_SPDIF__FUNC_IRRX (MTK_PIN_NO(87) | 5)
#define MT8512_PIN_87_SPDIF__FUNC_DBG_MON_A_24 (MTK_PIN_NO(87) | 7)

#define MT8512_PIN_88_PCM_CLK__FUNC_GPIO88 (MTK_PIN_NO(88) | 0)
#define MT8512_PIN_88_PCM_CLK__FUNC_PCM_CLK (MTK_PIN_NO(88) | 2)
#define MT8512_PIN_88_PCM_CLK__FUNC_CONN_TOP_CLK (MTK_PIN_NO(88) | 3)

#define MT8512_PIN_89_PCM_SYNC__FUNC_GPIO89 (MTK_PIN_NO(89) | 0)
#define MT8512_PIN_89_PCM_SYNC__FUNC_PCM_SYNC (MTK_PIN_NO(89) | 2)
#define MT8512_PIN_89_PCM_SYNC__FUNC_CONN_WB_PTA (MTK_PIN_NO(89) | 3)

#define MT8512_PIN_90_PCM_RX__FUNC_GPIO90 (MTK_PIN_NO(90) | 0)
#define MT8512_PIN_90_PCM_RX__FUNC_PCM_RX (MTK_PIN_NO(90) | 2)
#define MT8512_PIN_90_PCM_RX__FUNC_CONN_TOP_DATA (MTK_PIN_NO(90) | 3)

#define MT8512_PIN_91_PCM_TX__FUNC_GPIO91 (MTK_PIN_NO(91) | 0)
#define MT8512_PIN_91_PCM_TX__FUNC_IOSEL_DBGOUT (MTK_PIN_NO(91) | 1)
#define MT8512_PIN_91_PCM_TX__FUNC_PCM_TX (MTK_PIN_NO(91) | 2)
#define MT8512_PIN_91_PCM_TX__FUNC_CONN_HRST_B (MTK_PIN_NO(91) | 3)

#define MT8512_PIN_92_I2SIN_MCLK__FUNC_GPIO92 (MTK_PIN_NO(92) | 0)
#define MT8512_PIN_92_I2SIN_MCLK__FUNC_I2SIN_MCK (MTK_PIN_NO(92) | 1)
#define MT8512_PIN_92_I2SIN_MCLK__FUNC_TDMIN_MCLK (MTK_PIN_NO(92) | 2)
#define MT8512_PIN_92_I2SIN_MCLK__FUNC_EPDC_D15 (MTK_PIN_NO(92) | 3)

#define MT8512_PIN_93_I2SIN_LRCK__FUNC_GPIO93 (MTK_PIN_NO(93) | 0)
#define MT8512_PIN_93_I2SIN_LRCK__FUNC_I2SIN_LRCK (MTK_PIN_NO(93) | 1)
#define MT8512_PIN_93_I2SIN_LRCK__FUNC_EPDC_GDOE (MTK_PIN_NO(93) | 3)
#define MT8512_PIN_93_I2SIN_LRCK__FUNC_SPLIN_LRCK (MTK_PIN_NO(93) | 4)

#define MT8512_PIN_94_I2SIN_BCK__FUNC_GPIO94 (MTK_PIN_NO(94) | 0)
#define MT8512_PIN_94_I2SIN_BCK__FUNC_I2SIN_BCK (MTK_PIN_NO(94) | 1)
#define MT8512_PIN_94_I2SIN_BCK__FUNC_EPDC_GDSP (MTK_PIN_NO(94) | 3)
#define MT8512_PIN_94_I2SIN_BCK__FUNC_SPLIN_BCK (MTK_PIN_NO(94) | 4)

#define MT8512_PIN_95_I2SIN_DAT0__FUNC_GPIO95 (MTK_PIN_NO(95) | 0)
#define MT8512_PIN_95_I2SIN_DAT0__FUNC_I2SIN_DAT0 (MTK_PIN_NO(95) | 1)
#define MT8512_PIN_95_I2SIN_DAT0__FUNC_EPDC_D12 (MTK_PIN_NO(95) | 3)
#define MT8512_PIN_95_I2SIN_DAT0__FUNC_SPLIN_D0 (MTK_PIN_NO(95) | 4)

#define MT8512_PIN_96_I2SIN_DAT1__FUNC_GPIO96 (MTK_PIN_NO(96) | 0)
#define MT8512_PIN_96_I2SIN_DAT1__FUNC_I2SIN_DAT1 (MTK_PIN_NO(96) | 1)
#define MT8512_PIN_96_I2SIN_DAT1__FUNC_TDMIN_BCK (MTK_PIN_NO(96) | 2)
#define MT8512_PIN_96_I2SIN_DAT1__FUNC_EPDC_D10 (MTK_PIN_NO(96) | 3)
#define MT8512_PIN_96_I2SIN_DAT1__FUNC_SPLIN_D1 (MTK_PIN_NO(96) | 4)
#define MT8512_PIN_96_I2SIN_DAT1__FUNC_DBG_MON_A_15 (MTK_PIN_NO(96) | 7)

#define MT8512_PIN_97_I2SIN_DAT2__FUNC_GPIO97 (MTK_PIN_NO(97) | 0)
#define MT8512_PIN_97_I2SIN_DAT2__FUNC_I2SIN_DAT2 (MTK_PIN_NO(97) | 1)
#define MT8512_PIN_97_I2SIN_DAT2__FUNC_TDMIN_LRCK (MTK_PIN_NO(97) | 2)
#define MT8512_PIN_97_I2SIN_DAT2__FUNC_EPDC_D8 (MTK_PIN_NO(97) | 3)
#define MT8512_PIN_97_I2SIN_DAT2__FUNC_SPLIN_D2 (MTK_PIN_NO(97) | 4)
#define MT8512_PIN_97_I2SIN_DAT2__FUNC_DBG_MON_A_16 (MTK_PIN_NO(97) | 7)

#define MT8512_PIN_98_I2SIN_DAT3__FUNC_GPIO98 (MTK_PIN_NO(98) | 0)
#define MT8512_PIN_98_I2SIN_DAT3__FUNC_I2SIN_DAT3 (MTK_PIN_NO(98) | 1)
#define MT8512_PIN_98_I2SIN_DAT3__FUNC_TDMIN_DI (MTK_PIN_NO(98) | 2)
#define MT8512_PIN_98_I2SIN_DAT3__FUNC_EPDC_D6 (MTK_PIN_NO(98) | 3)
#define MT8512_PIN_98_I2SIN_DAT3__FUNC_SPLIN_D3 (MTK_PIN_NO(98) | 4)
#define MT8512_PIN_98_I2SIN_DAT3__FUNC_DBG_MON_A_17 (MTK_PIN_NO(98) | 7)

#define MT8512_PIN_99_DMIC0_CLK__FUNC_GPIO99 (MTK_PIN_NO(99) | 0)
#define MT8512_PIN_99_DMIC0_CLK__FUNC_DMIC_CLK0 (MTK_PIN_NO(99) | 1)
#define MT8512_PIN_99_DMIC0_CLK__FUNC_EPDC_GDCLK (MTK_PIN_NO(99) | 3)
#define MT8512_PIN_99_DMIC0_CLK__FUNC_DBG_MON_A_18 (MTK_PIN_NO(99) | 7)

#define MT8512_PIN_100_DMIC0_DAT0__FUNC_GPIO100 (MTK_PIN_NO(100) | 0)
#define MT8512_PIN_100_DMIC0_DAT0__FUNC_DMIC_DAT0 (MTK_PIN_NO(100) | 1)
#define MT8512_PIN_100_DMIC0_DAT0__FUNC_DBG_MON_A_19 (MTK_PIN_NO(100) | 7)

#define MT8512_PIN_101_DMIC0_DAT1__FUNC_GPIO101 (MTK_PIN_NO(101) | 0)
#define MT8512_PIN_101_DMIC0_DAT1__FUNC_DMIC_DAT1 (MTK_PIN_NO(101) | 1)
#define MT8512_PIN_101_DMIC0_DAT1__FUNC_TDMIN_DI (MTK_PIN_NO(101) | 2)
#define MT8512_PIN_101_DMIC0_DAT1__FUNC_EPDC_D14 (MTK_PIN_NO(101) | 3)
#define MT8512_PIN_101_DMIC0_DAT1__FUNC_DBG_MON_A_20 (MTK_PIN_NO(101) | 7)

#define MT8512_PIN_102_DMIC1_CLK__FUNC_GPIO102 (MTK_PIN_NO(102) | 0)
#define MT8512_PIN_102_DMIC1_CLK__FUNC_DMIC_CLK1 (MTK_PIN_NO(102) | 1)
#define MT8512_PIN_102_DMIC1_CLK__FUNC_TDMIN_MCLK (MTK_PIN_NO(102) | 2)
#define MT8512_PIN_102_DMIC1_CLK__FUNC_EPDC_SDCE1 (MTK_PIN_NO(102) | 3)
#define MT8512_PIN_102_DMIC1_CLK__FUNC_DBG_MON_A_21 (MTK_PIN_NO(102) | 7)

#define MT8512_PIN_103_DMIC1_DAT0__FUNC_GPIO103 (MTK_PIN_NO(103) | 0)
#define MT8512_PIN_103_DMIC1_DAT0__FUNC_DMIC_DAT2 (MTK_PIN_NO(103) | 1)
#define MT8512_PIN_103_DMIC1_DAT0__FUNC_TDMIN_BCK (MTK_PIN_NO(103) | 2)
#define MT8512_PIN_103_DMIC1_DAT0__FUNC_EPDC_SDCE2 (MTK_PIN_NO(103) | 3)
#define MT8512_PIN_103_DMIC1_DAT0__FUNC_DBG_MON_A_22 (MTK_PIN_NO(103) | 7)

#define MT8512_PIN_104_DMIC1_DAT1__FUNC_GPIO104 (MTK_PIN_NO(104) | 0)
#define MT8512_PIN_104_DMIC1_DAT1__FUNC_DMIC_DAT3 (MTK_PIN_NO(104) | 1)
#define MT8512_PIN_104_DMIC1_DAT1__FUNC_TDMIN_LRCK (MTK_PIN_NO(104) | 2)
#define MT8512_PIN_104_DMIC1_DAT1__FUNC_EPDC_SDCE3 (MTK_PIN_NO(104) | 3)
#define MT8512_PIN_104_DMIC1_DAT1__FUNC_DBG_MON_A_23 (MTK_PIN_NO(104) | 7)

#define MT8512_PIN_105_I2SO_BCK__FUNC_GPIO105 (MTK_PIN_NO(105) | 0)
#define MT8512_PIN_105_I2SO_BCK__FUNC_I2SO_BCK (MTK_PIN_NO(105) | 1)
#define MT8512_PIN_105_I2SO_BCK__FUNC_EPDC_D5 (MTK_PIN_NO(105) | 3)
#define MT8512_PIN_105_I2SO_BCK__FUNC_DBG_MON_A_8 (MTK_PIN_NO(105) | 7)

#define MT8512_PIN_106_I2SO_LRCK__FUNC_GPIO106 (MTK_PIN_NO(106) | 0)
#define MT8512_PIN_106_I2SO_LRCK__FUNC_I2SO_LRCK (MTK_PIN_NO(106) | 1)
#define MT8512_PIN_106_I2SO_LRCK__FUNC_EPDC_D7 (MTK_PIN_NO(106) | 3)
#define MT8512_PIN_106_I2SO_LRCK__FUNC_DBG_MON_A_9 (MTK_PIN_NO(106) | 7)

#define MT8512_PIN_107_I2SO_MCLK__FUNC_GPIO107 (MTK_PIN_NO(107) | 0)
#define MT8512_PIN_107_I2SO_MCLK__FUNC_I2SO_MCK (MTK_PIN_NO(107) | 1)
#define MT8512_PIN_107_I2SO_MCLK__FUNC_EPDC_SDCLK (MTK_PIN_NO(107) | 3)
#define MT8512_PIN_107_I2SO_MCLK__FUNC_DBG_MON_A_10 (MTK_PIN_NO(107) | 7)

#define MT8512_PIN_108_I2SO_DAT0__FUNC_GPIO108 (MTK_PIN_NO(108) | 0)
#define MT8512_PIN_108_I2SO_DAT0__FUNC_I2SO_DAT0 (MTK_PIN_NO(108) | 1)
#define MT8512_PIN_108_I2SO_DAT0__FUNC_EPDC_D3 (MTK_PIN_NO(108) | 3)
#define MT8512_PIN_108_I2SO_DAT0__FUNC_DBG_MON_A_11 (MTK_PIN_NO(108) | 7)

#define MT8512_PIN_109_I2SO_DAT1__FUNC_GPIO109 (MTK_PIN_NO(109) | 0)
#define MT8512_PIN_109_I2SO_DAT1__FUNC_I2SO_DAT1 (MTK_PIN_NO(109) | 1)
#define MT8512_PIN_109_I2SO_DAT1__FUNC_EPDC_D1 (MTK_PIN_NO(109) | 3)
#define MT8512_PIN_109_I2SO_DAT1__FUNC_DBG_MON_A_12 (MTK_PIN_NO(109) | 7)

#define MT8512_PIN_110_I2SO_DAT2__FUNC_GPIO110 (MTK_PIN_NO(110) | 0)
#define MT8512_PIN_110_I2SO_DAT2__FUNC_I2SO_DAT2 (MTK_PIN_NO(110) | 1)
#define MT8512_PIN_110_I2SO_DAT2__FUNC_SDA0_0 (MTK_PIN_NO(110) | 2)
#define MT8512_PIN_110_I2SO_DAT2__FUNC_EPDC_SDCE0 (MTK_PIN_NO(110) | 3)
#define MT8512_PIN_110_I2SO_DAT2__FUNC_URXD1 (MTK_PIN_NO(110) | 4)
#define MT8512_PIN_110_I2SO_DAT2__FUNC_DBG_MON_A_13 (MTK_PIN_NO(110) | 7)

#define MT8512_PIN_111_I2SO_DAT3__FUNC_GPIO111 (MTK_PIN_NO(111) | 0)
#define MT8512_PIN_111_I2SO_DAT3__FUNC_I2SO_DAT3 (MTK_PIN_NO(111) | 1)
#define MT8512_PIN_111_I2SO_DAT3__FUNC_SCL0_0 (MTK_PIN_NO(111) | 2)
#define MT8512_PIN_111_I2SO_DAT3__FUNC_EPDC_SDLE (MTK_PIN_NO(111) | 3)
#define MT8512_PIN_111_I2SO_DAT3__FUNC_UTXD1 (MTK_PIN_NO(111) | 4)
#define MT8512_PIN_111_I2SO_DAT3__FUNC_DBG_MON_A_14 (MTK_PIN_NO(111) | 7)

#define MT8512_PIN_112_SPI_CSB__FUNC_GPIO112 (MTK_PIN_NO(112) | 0)
#define MT8512_PIN_112_SPI_CSB__FUNC_SPI_CSB (MTK_PIN_NO(112) | 1)
#define MT8512_PIN_112_SPI_CSB__FUNC_SPI_CSB_NOR (MTK_PIN_NO(112) | 3)
#define MT8512_PIN_112_SPI_CSB__FUNC_JTMS (MTK_PIN_NO(112) | 4)
#define MT8512_PIN_112_SPI_CSB__FUNC_I2SO_BCK (MTK_PIN_NO(112) | 5)

#define MT8512_PIN_113_SPI_CLK__FUNC_GPIO113 (MTK_PIN_NO(113) | 0)
#define MT8512_PIN_113_SPI_CLK__FUNC_SPI_CLK (MTK_PIN_NO(113) | 1)
#define MT8512_PIN_113_SPI_CLK__FUNC_SPI_CLK_NOR (MTK_PIN_NO(113) | 3)
#define MT8512_PIN_113_SPI_CLK__FUNC_JTCK (MTK_PIN_NO(113) | 4)
#define MT8512_PIN_113_SPI_CLK__FUNC_I2SO_LRCK (MTK_PIN_NO(113) | 5)

#define MT8512_PIN_114_SPI_MISO__FUNC_GPIO114 (MTK_PIN_NO(114) | 0)
#define MT8512_PIN_114_SPI_MISO__FUNC_SPI_MI (MTK_PIN_NO(114) | 1)
#define MT8512_PIN_114_SPI_MISO__FUNC_SPI_DAT0_NOR (MTK_PIN_NO(114) | 3)
#define MT8512_PIN_114_SPI_MISO__FUNC_JTDI (MTK_PIN_NO(114) | 4)
#define MT8512_PIN_114_SPI_MISO__FUNC_I2SO_MCK (MTK_PIN_NO(114) | 5)

#define MT8512_PIN_115_SPI_MOSI__FUNC_GPIO115 (MTK_PIN_NO(115) | 0)
#define MT8512_PIN_115_SPI_MOSI__FUNC_SPI_MO (MTK_PIN_NO(115) | 1)
#define MT8512_PIN_115_SPI_MOSI__FUNC_SPI_DAT1_NOR (MTK_PIN_NO(115) | 3)
#define MT8512_PIN_115_SPI_MOSI__FUNC_JTDO (MTK_PIN_NO(115) | 4)
#define MT8512_PIN_115_SPI_MOSI__FUNC_I2SO_DAT0 (MTK_PIN_NO(115) | 5)

#endif /* __DTS_MT8512_PINFUNC_H */

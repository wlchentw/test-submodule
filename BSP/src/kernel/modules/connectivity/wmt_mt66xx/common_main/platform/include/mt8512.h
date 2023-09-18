/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

/*! \file
 * \brief  Declaration of library functions
 *
 * Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#ifndef _MTK_MT8512_H_
#define _MTK_MT8512_H_

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

#define CONSYS_BT_WIFI_SHARE_V33	0
#define CONSYS_PMIC_CTRL_ENABLE		1
#define CONSYS_AHB_CLK_MAGEMENT		1
#define CONSYS_USE_PLATFORM_WRITE	1
#define CONSYS_PWR_ON_OFF_API_AVAILABLE	0
#define CONSYS_AFE_REG_SETTING		1
/* need zhao confirm */
#define CONSYS_CLOCK_BUF_CTRL		0
/* TODO ZQ confirm */
#define CONFIG_RESET_CONTROL        0

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*tag start:new platform need to make sure these define */
#define PLATFORM_SOC_CHIP 0x8512
/*tag end*/

#define TOPCKGEN_BASE		0x10000000

/*device tree mode*/
/* A-Die interface pinmux base */
#define CONSYS_IF_PINMUX_REG_BASE	0x10005000
#define CONSYS_IF_PINMUX_01_OFFSET	0x00000260
#define CONSYS_IF_PINMUX_01_MASK	0xC0FFFFFF
#define CONSYS_IF_PINMUX_01_VALUE	0x1B000000
#define CONSYS_IF_PINMUX_02_OFFSET	0x00000270
#define CONSYS_IF_PINMUX_02_MASK	0xFFFFFFC0
#define CONSYS_IF_PINMUX_02_VALUE	0x0000001B
#define CONSYS_IF_PINMUX_03_OFFSET	0x00000200
#define CONSYS_IF_PINMUX_03_MASK	0xFFFC0007
#define CONSYS_IF_PINMUX_03_VALUE	0x00009248
#define CONSYS_IF_DRV_PINMUX_01_OFFSET	0x00000750
#define CONSYS_IF_DRV_PINMUX_01_MASK	0x0FFFFFFF
#define CONSYS_IF_DRV_PINMUX_01_VALUE	0x00000000
#define CONSYS_IF_DRV_PINMUX_02_OFFSET	0x00000710
#define CONSYS_IF_DRV_PINMUX_02_MASK	0x0FFFFFFF
#define CONSYS_IF_DRV_PINMUX_02_VALUE	0x10000000
#define CONSYS_IF_PINMUX_TCXO_OFFSET    0x00000230
#define CONSYS_IF_PINMUX_TCXO_MASK      0xFFFF8FFF
#define CONSYS_IF_PINMUX_TCXO_VALUE     0x00004000

/*TOPCKGEN_BASE*/
#define CONSYS_AP2CONN_OSC_EN_OFFSET	0x00000f00
#define CONSYS_EMI_MAPPING_OFFSET	0x00000380
#define CONSYS_EMI_PERI_MAPPING_OFFSET	0x00000388
/*AP_RGU_BASE*/
#define CONSYS_CPU_SW_RST_OFFSET	0x00000018
/*SPM_BASE*/
#define CONSYS_PWRON_CONFG_EN_OFFSET	0x00000000
#define CONSYS_TOP1_PWR_CTRL_OFFSET	0x0000032C
#define CONSYS_PWR_CONN_ACK_OFFSET	0x00000180
#define CONSYS_PWR_CONN_ACK_S_OFFSET	0x00000184
/*CONN_MCU_CONFIG_BASE*/
#define CONSYS_IP_VER_OFFSET		0x00000010
#define CONSYS_CONF_ID_OFFSET		0x0000001c
#define CONSYS_HW_ID_OFFSET		0x00000000
#define CONSYS_FW_ID_OFFSET		0x00000004
#define CONSYS_IP_VER_ID		0x10060000
#define CONSYS_MCU_CFG_MISC_OFFSET	0x00000140
#define CONSYS_CPUPCR_OFFSET		0x00000104
#define CONSYS_SW_IRQ_OFFSET		0x00000150
#define CONSYS_CLOCK_CONTROL            0x00000100
#define CONSYS_BUS_CONTROL              0x00000110
#define CONSYS_DEBUG_SELECT             0x00000400
#define CONSYS_DEBUG_STATUS             0x0000040c
#define CONSYS_EMI_CTRL_VALUE           (1 << 21)
#define CONSYS_BUS_TIMEOUT_CONFIG_OFFSET 0x00000440
#define CONSYS_ENABLE_AHB_BUS_TIMEOUT    0x80000101

/*CONN_HIF_TOP*/
#define CONSYS_HIF_TOP_MISC             0x00000104
#define CONSYS_HIF_DBG_IDX              0x0000012C
#define CONSYS_HIF_DBG_PROBE            0x00000130
#define CONSYS_HIF_BUSY_STATUS          0x00000138
#define CONSYS_HIF_PDMA_AXI_RREADY      0x00000154
#define CONSYS_HIF_PDMA_BUSY_STATUS     0x00000168

/*CONN_HIF_ON_BASE*/
#define CONSYS_BUSY_OFFSET              0x110
#define CONSYS_BUSY_BIT                 (0x1 << 27)
#define CONSYS_CLOCK_CHECK_VALUE        0x30000
#define CONSYS_HCLK_CHECK_BIT           (0x1 << 16)
#define CONSYS_OSCCLK_CHECK_BIT         (0x1 << 17)
#define CONSYS_SLEEP_CHECK_BIT          (0x1 << 18)

/*CONN_TOP_MISC_ON_BASE*/
#define CONN_ON_ADIE_IC_OFFSET      0x130
#define CONN_ON_ADIE_IC_MASK        (0x1 << 8)
#define CONN_ON_ADIE_6631_VALUE     0x100
#define CONN_ON_HOST_CSR_MISC		0x14c
#define CONN_ON_IRQ_CTL			0x170
#define CONN_ON_IRQ_STATUS		0x174
#define CONN_ON_SLPPROTECT_OFFSET	0x168
#define CONN_ON_SLPPROTECT_BIT		0x1

/*AXI bus*/
#define CONSYS_AHBAXI_PROT_EN_SET_OFFSET	0x2A0
#define CONSYS_AHBAXI_PROT_EN_CLR_OFFSET	0x2A4
#define CONSYS_AHBAXI_PROT_EN_1_SET_OFFSET	0x2A8
#define CONSYS_AHBAXI_PROT_EN_1_CLR_OFFSET	0x2AC
#define CONSYS_AHBAXI_PROT_STA1_OFFSET	    0x228
#define CONSYS_AHBAXI_PROT_STA1_1_OFFSET	0x258

#define CONSYS_AHB_RX_PROT_MASK		(0x1 << 21) /* bit 21 */
#define CONSYS_AHB_TX_PROT_MASK		(0x1 << 13) /* bit 13 */
#define CONSYS_AXI_RX_PROT_MASK		(0x1 << 14) /* bit 14 */
#define CONSYS_AXI_TX_PROT_MASK		(0x1 << 18) /* bit 18 */
#define CONSYS_PDMA_AXI_RREADY_MASK	(0x1 << 1)	/* bit 1 */

/*SPM clock gating control register */
#define CONSYS_PWRON_CONFG_EN_VALUE	(0x0b160001)
#define CONSYS_PWRON_CONFG_DIS_VALUE	(0x0b160000)

#if CONSYS_AFE_REG_SETTING
#define CONSYS_AFE_REG_BASE                 (0x180B3000)
#define CONSYS_AFE_RG_WBG_AFE_01_OFFSET     (0x00000010)
#define CONSYS_AFE_RG_WBG_AFE_01_VALUE      (0x00000004)
#define CONSYS_AFE_RG_WBG_PLL_01_OFFSET     (0x00000024)
#define CONSYS_AFE_RG_WBG_PLL_01_VALUE      (0x00000010)
#define CONSYS_AFE_RG_WBG_BT_RX_01_OFFSET   (0x00000048)
#define CONSYS_AFE_RG_WBG_BT_RX_01_VALUE    (0xEFC82200)
#define CONSYS_AFE_RG_WBG_WF0_RX_01_OFFSET  (0x00000068)
#define CONSYS_AFE_RG_WBG_WF0_RX_01_VALUE   (0xBBF21108)
#endif

#define CONSYS_COCLOCK_STABLE_TIME_BASE		(0x180C1200)
#define CONSYS_COCLOCK_ACK_ENABLE_OFFSET	(0x4)
#define CONSYS_COCLOCK_ACK_ENABLE_BIT		(1 << 0)
#define CONSYS_COCLOCK_STABLE_TIME		(0x708)
#define CONSYS_COCLOCK_STABLE_TIME_MASK		(0xffff0000)

/*CONSYS_CPU_SW_RST_REG*/
#define CONSYS_CPU_SW_RST_BIT		(0x1 << 12)
#define CONSYS_CPU_SW_RST_CTRL_KEY	(0x88 << 24)
#define CONSYS_SW_RST_BIT		(0x1 << 9)

/*CONSYS_TOP1_PWR_CTRL_REG*/
#define CONSYS_SPM_PWR_RST_BIT		(0x1 << 0)
#define CONSYS_SPM_PWR_ISO_S_BIT	(0x1 << 1)
#define CONSYS_SPM_PWR_ON_BIT		(0x1 << 2)
#define CONSYS_SPM_PWR_ON_S_BIT		(0x1 << 3)
#define CONSYS_CLK_CTRL_BIT		(0x1 << 4)
#define CONSYS_SRAM_CONN_PD_BIT		(0x1 << 8)

/*CONSYS_PWR_CONN_ACK_REG*/
#define CONSYS_PWR_ON_ACK_BIT		(0x1 << 1)

/*CONSYS_PWR_CONN_ACK_S_REG*/
#define CONSYS_PWR_ON_ACK_S_BIT		(0x1 << 1)

/*CONSYS_PWR_CONN_TOP2_ACK_REG*/
#define CONSYS_TOP2_PWR_ON_ACK_BIT	(0x1 << 30)

/*CONSYS_PWR_CONN_TOP2_ACK_S_REG*/
#define CONSYS_TOP2_PWR_ON_ACK_S_BIT	(0x1 << 30)

/*CONSYS_WD_SYS_RST_REG*/
#define CONSYS_WD_SYS_RST_CTRL_KEY	(0x88 << 24)
#define CONSYS_WD_SYS_RST_BIT		(0x1 << 9)

/*CONSYS_MCU_CFG_ACR_REG*/
#define CONSYS_MCU_CFG_ACR_MBIST_BIT	(0x1 << 0 | 0x1 << 1)

/*control app2cnn_osc_en*/
#define CONSYS_AP2CONN_OSC_EN_BIT	(0x1 << 10)
#define CONSYS_AP2CONN_WAKEUP_BIT	(0x1 << 9)

/* EMI part mapping & ctrl*/
#define CONSYS_EMI_COREDUMP_OFFSET	(0x68000)
#define CONSYS_EMI_AP_PHY_OFFSET	(0x00000)
#define CONSYS_EMI_AP_PHY_BASE		(0x80068000)
#define CONSYS_EMI_FW_PHY_BASE		(0xf0068000)
#define CONSYS_EMI_PAGED_TRACE_OFFSET	(0x400)
#define CONSYS_EMI_PAGED_DUMP_OFFSET	(0x8400)
#define CONSYS_EMI_FULL_DUMP_OFFSET	(0x10400)
#define CONSYS_EMI_MET_DATA_OFFSET	(0x0)

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

#ifdef CONSYS_BT_WIFI_SHARE_V33
struct bt_wifi_v33_status {
	UINT32 counter;
	UINT32 flags;
	spinlock_t lock;
};

extern struct bt_wifi_v33_status gBtWifiV33;
#endif

enum conn_clk {
	CONN_CLK_32K,
	CONN_CLK_26M,
	CONN_CLK_MAX
};

/*******************************************************************************
*                            P U B L I C   D A T A
*******************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

#endif /* _MTK_MT8512_H_ */

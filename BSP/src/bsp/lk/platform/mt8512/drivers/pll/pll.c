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

#include <debug.h>
#include <reg.h>
#include <platform/mt8512.h>
#include <platform/pll.h>
#include <platform/spm_mtcmos.h>
#include <platform/mtk_timer.h>

#define ALL_CLK_ON      0
#define DEBUG_FQMTR     0

#define  RGU_KEY_CODE    (0x88 << 24)
#define  CONNSYS_CPU_SW_RST    (0x1 << 12)

#define DRV_WriteReg32(addr, data)  writel(data, addr)
#define DRV_Reg32(addr) readl(addr)

enum rg_fqmtr_abist_clk {
	FM_ABIST_CLK_NULL			=  0,

	FM_AD_ARMPLL_CK			=  1,
	FM_AD_MAINPLL_CK		=  3,
	FM_AD_XO_LP_CLK_26M		=  4,
	FM_AD_MAINPLL_H546M_CK		= 10,
	FM_AD_MAINPLL_H364M_CK		= 11,
	FM_AD_MAINPLL_H218P4M_CK	= 12,
	FM_AD_MAINPLL_H156M_CK		= 13,
	FM_AD_UNIVPLL_1248M_CK		= 14,
	FM_AD_USB20_192M_CK		= 15,
	FM_AD_UNIVPLL_624M_CK		= 16,
	FM_AD_UNIVPLL_416M_CK		= 17,
	FM_AD_UNIVPLL_249P6M_CK		= 18,
	FM_AD_UNIVPLL_178P3M_CK		= 19,
	FM_AD_MDPLL1_FS26M_CK		= 20,
	FM_CLKRTC			= 25,
	FM_DA_PLLGP1_26M_CK		= 26,
	FM_DA_PLLGP2_26M_CK		= 27,
	FM_AD_MSDCPLL_CK		= 28,
	FM_AD_APLL1_CK			= 32,
	FM_AD_APLL2_CK			= 33,
	FM_DA_USB20_48M_DIV_CK		= 41,
	FM_DA_UNIV_48M_DIV_CK		= 42,
	FM_AD_TCONPLL_CK		= 51,
	FM_AD_DSPPLL_CK			= 52,
	FM_AD_XO_HP_CLK_26M		= 53,
	FM_AD_IPPLL_CK			= 54,
	FM_AD_SYS_26M_CK		= 56,
	FM_AD_CLKSQ_26M_CK		= 57,
	FM_AD_26M_CLKMON		= 58,

	FM_ABIST_CLK_END			= 59,
};

enum rg_fqmtr_ckgen_clk {
	FM_CKGEN_CLK_NULL			=  0,

	FM_AXI_CK			=  1,
	FM_MEM_CK			=  2,
	FM_UART_CK			=  3,
	FM_SPI_CK			=  4,
	FM_SPIS_CK			=  5,
	FM_MSDC50_0_HCLK_CK		=  6,
	FM_MSDC2_2_HCLK_CK		=  7,
	FM_MSDC50_0_CK			=  8,
	FM_MSDC50_2_CK			=  9,
	FM_MSDC30_1_CK			= 10,
	FM_AUDIO_CK			= 11,
	FM_AUD_INTBUS_CK		= 12,
	FM_HAPLL1_CK			= 13,
	FM_HAPLL2_CK			= 14,
	FM_A2SYS_CK			= 15,
	FM_A1SYS_CK			= 16,
	FM_ASM_L_CK			= 17,
	FM_ASM_M_CK			= 18,
	FM_ASM_H_CK			= 19,
	FM_AUD_SPDIF_IN_CK		= 20,
	FM_AUD_1_CK			= 21,
	FM_AUD_2_CK			= 22,
	FM_SSUSB_SYS_CK			= 23,
	FM_SSUSB_XHCI_CK		= 24,
	FM_SPM_CK			= 25,
	FM_I2C_CK			= 26,
	FM_PWM_CK			= 27,
	FM_DSP_CK			= 28,
	FM_NFI2X_CK			= 29,
	FM_SPINFI_CK			= 30,
	FM_ECC_CK			= 31,
	FM_GCPU_CK			= 32,
	FM_GCPUM_CK			= 33,
	FM_MBIST_DIAG_CK		= 34,
	FM_IP0_NNA_CK			= 35,
	FM_IP1_NNA_CK			= 36,
	FM_IP2_WFST_CK			= 37,
	FM_SFLASH_CK			= 38,
	FM_SRAM_CK			= 39,
	FM_MM_CK			= 40,
	FM_DPI0_CK			= 41,
	FM_DBG_ATCLK_CK			= 42,
	FM_OCC_104M_CK			= 43,
	FM_OCC_68M_CK			= 44,
	FM_OCC_182M_CK			= 45,
	FM_F_UFS_MP_SAP_CFG_CK		= 48,
	FM_F_UFS_TICK1US_CK		= 49,
	FM_HD_FAXI_EAST_CK		= 50,
	FM_HD_FAXI_WEST_CK		= 51,
	FM_HD_FAXI_NORTH_CK		= 52,
	FM_HD_FAXI_SOUTH_CK		= 53,
	FM_HG_FMIPICFG_TX_CK		= 54,

	FM_CKGEN_CLK_END			= 55,
};

static const char* abist_clk[] = {
	[FM_AD_ARMPLL_CK]		= "AD_ARMPLL_CK",
	[FM_AD_MAINPLL_CK]		= "AD_MAINPLL_CK",
	[FM_AD_XO_LP_CLK_26M]		= "AD_XO_LP_CLK_26M",
	[FM_AD_MAINPLL_H546M_CK]	= "AD_MAINPLL_H546M_CK",
	[FM_AD_MAINPLL_H364M_CK]	= "AD_MAINPLL_H364M_CK",
	[FM_AD_MAINPLL_H218P4M_CK]	= "AD_MAINPLL_H218P4M_CK",
	[FM_AD_MAINPLL_H156M_CK]	= "AD_MAINPLL_H156M_CK",
	[FM_AD_UNIVPLL_1248M_CK]	= "AD_UNIVPLL_1248M_CK",
	[FM_AD_USB20_192M_CK]		= "AD_USB20_192M_CK",
	[FM_AD_UNIVPLL_624M_CK]		= "AD_UNIVPLL_624M_CK",
	[FM_AD_UNIVPLL_416M_CK]		= "AD_UNIVPLL_416M_CK",
	[FM_AD_UNIVPLL_249P6M_CK]	= "AD_UNIVPLL_249P6M_CK",
	[FM_AD_UNIVPLL_178P3M_CK]	= "AD_UNIVPLL_178P3M_CK",
	[FM_AD_MDPLL1_FS26M_CK]		= "AD_MDPLL1_FS26M_CK",
	[FM_CLKRTC]			= "clkrtc",
	[FM_DA_PLLGP1_26M_CK]		= "DA_PLLGP1_26M_CK",
	[FM_DA_PLLGP2_26M_CK]		= "DA_PLLGP2_26M_CK",
	[FM_AD_MSDCPLL_CK]		= "AD_MSDCPLL_CK",
	[FM_AD_APLL1_CK]		= "AD_APLL1_CK",
	[FM_AD_APLL2_CK]		= "AD_APLL2_CK",
	[FM_DA_USB20_48M_DIV_CK]	= "DA_USB20_48M_DIV_CK",
	[FM_DA_UNIV_48M_DIV_CK]		= "DA_UNIV_48M_DIV_CK",
	[FM_AD_TCONPLL_CK]		= "AD_TCONPLL_CK",
	[FM_AD_DSPPLL_CK]		= "AD_DSPPLL_CK",
	[FM_AD_XO_HP_CLK_26M]		= "AD_XO_HP_CLK_26M",
	[FM_AD_IPPLL_CK]		= "AD_IPPLL_CK",
	[FM_AD_SYS_26M_CK]		= "AD_SYS_26M_CK",
	[FM_AD_CLKSQ_26M_CK]		= "AD_CLKSQ_26M_CK",
	[FM_AD_26M_CLKMON]		= "AD_26M_CLKMON",
};

static const char* ckgen_clk[] = {
	[FM_AXI_CK]			= "axi_ck",
	[FM_MEM_CK]			= "mem_ck",
	[FM_UART_CK]			= "uart_ck",
	[FM_SPI_CK]			= "spi_ck",
	[FM_SPIS_CK]			= "spis_ck",
	[FM_MSDC50_0_HCLK_CK]		= "msdc50_0_hclk_ck",
	[FM_MSDC2_2_HCLK_CK]		= "msdc2_2_hclk_ck",
	[FM_MSDC50_0_CK]		= "msdc50_0_ck",
	[FM_MSDC50_2_CK]		= "msdc50_2_ck",
	[FM_MSDC30_1_CK]		= "msdc30_1_ck",
	[FM_AUDIO_CK]			= "audio_ck",
	[FM_AUD_INTBUS_CK]		= "aud_intbus_ck",
	[FM_HAPLL1_CK]			= "hapll1_ck",
	[FM_HAPLL2_CK]			= "hapll2_ck",
	[FM_A2SYS_CK]			= "a2sys_ck",
	[FM_A1SYS_CK]			= "a1sys_ck",
	[FM_ASM_L_CK]			= "asm_l_ck",
	[FM_ASM_M_CK]			= "asm_m_ck",
	[FM_ASM_H_CK]			= "asm_h_ck",
	[FM_AUD_SPDIF_IN_CK]		= "aud_spdif_in_ck",
	[FM_AUD_1_CK]			= "aud_1_ck",
	[FM_AUD_2_CK]			= "aud_2_ck",
	[FM_SSUSB_SYS_CK]		= "ssusb_sys_ck",
	[FM_SSUSB_XHCI_CK]		= "ssusb_xhci_ck",
	[FM_SPM_CK]			= "spm_ck",
	[FM_I2C_CK]			= "i2c_ck",
	[FM_PWM_CK]			= "pwm_ck",
	[FM_DSP_CK]			= "dsp_ck",
	[FM_NFI2X_CK]			= "nfi2x_ck",
	[FM_SPINFI_CK]			= "spinfi_ck",
	[FM_ECC_CK]			= "ecc_ck",
	[FM_GCPU_CK]			= "gcpu_ck",
	[FM_GCPUM_CK]			= "gcpum_ck",
	[FM_MBIST_DIAG_CK]		= "mbist_diag_ck",
	[FM_IP0_NNA_CK]			= "ip0_nna_ck",
	[FM_IP1_NNA_CK]			= "ip1_nna_ck",
	[FM_IP2_WFST_CK]		= "ip2_wfst_ck",
	[FM_SFLASH_CK]			= "sflash_ck",
	[FM_SRAM_CK]			= "sram_ck",
	[FM_MM_CK]			= "mm_ck",
	[FM_DPI0_CK]			= "dpi0_ck",
	[FM_DBG_ATCLK_CK]		= "dbg_atclk_ck",
	[FM_OCC_104M_CK]		= "occ_104m_ck",
	[FM_OCC_68M_CK]			= "occ_68m_ck",
	[FM_OCC_182M_CK]		= "occ_182m_ck",
	[FM_F_UFS_MP_SAP_CFG_CK]	= "f_ufs_mp_sap_cfg_ck",
	[FM_F_UFS_TICK1US_CK]		= "f_ufs_tick1us_ck",
	[FM_HD_FAXI_EAST_CK]		= "hd_faxi_east_ck",
	[FM_HD_FAXI_WEST_CK]		= "hd_faxi_west_ck",
	[FM_HD_FAXI_NORTH_CK]		= "hd_faxi_north_ck",
	[FM_HD_FAXI_SOUTH_CK]		= "hd_faxi_south_ck",
	[FM_HG_FMIPICFG_TX_CK]		= "hg_fmipicfg_tx_ck",
};

unsigned int mt_get_abist_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = DRV_Reg32(CLK_DBG_CFG);
	DRV_WriteReg32(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC)|(ID << 16)); //sel abist_cksw and enable freq meter sel abist

	clk_misc_cfg_0 = DRV_Reg32(CLK_MISC_CFG_0);
	DRV_WriteReg32(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF) | (0x3 << 24)); // select divider, WAIT CONFIRM

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	clk26cali_1 = DRV_Reg32(CLK26CALI_1);
	DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x1000) | 0x1000); // bit[12] = 1, enable fmeter
	DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x10) | 0x10); // bit[4] = 1, start fmeter

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x10)
	{
		mdelay(10);
		i++;
		if(i > 10)
			break;
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;
	output = ((temp * 26000) ) / 1024; // Khz

	DRV_WriteReg32(CLK_DBG_CFG, clk_dbg_cfg);
	DRV_WriteReg32(CLK_MISC_CFG_0, clk_misc_cfg_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);
	DRV_WriteReg32(CLK26CALI_1, clk26cali_1);

	return output * 4;
}
static unsigned int mt_get_ckgen_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int temp, clk26cali_0, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1;

	clk_dbg_cfg = DRV_Reg32(CLK_DBG_CFG);
	DRV_WriteReg32(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFFC0FC)|(ID << 8)|(0x1)); //sel ckgen_cksw[22] and enable freq meter sel ckgen[21:16], 01:hd_faxi_ck

	clk_misc_cfg_0 = DRV_Reg32(CLK_MISC_CFG_0);
	DRV_WriteReg32(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF)); // select divider?dvt set zero

	clk26cali_0 = DRV_Reg32(CLK26CALI_0);
	clk26cali_1 = DRV_Reg32(CLK26CALI_1);
	DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x1000) | 0x1000); // bit[12] = 1, enable fmeter
	DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x10) | 0x10); // bit[4] = 1, start fmeter

	/* wait frequency meter finish */
	while (DRV_Reg32(CLK26CALI_0) & 0x10)
	{
		mdelay(10);
		i++;
		if(i > 10)
			break;
	}

	temp = DRV_Reg32(CLK26CALI_1) & 0xFFFF;
	output = ((temp * 26000) ) / 1024; // Khz

	DRV_WriteReg32(CLK_DBG_CFG, clk_dbg_cfg);
	DRV_WriteReg32(CLK_MISC_CFG_0, clk_misc_cfg_0);
	DRV_WriteReg32(CLK26CALI_0, clk26cali_0);
	DRV_WriteReg32(CLK26CALI_1, clk26cali_1);

	return output;
}

void dump_fqmtr(void)
{
    unsigned int temp;

    dprintf(CRITICAL, "abist:\n");
    for (temp = 0; temp < FM_ABIST_CLK_END; temp++) {
        if (!abist_clk[temp])
            continue;
		dprintf(CRITICAL, "%d: %s: %d KHz\n", temp, abist_clk[temp],
			mt_get_abist_freq(temp));
    }

	dprintf(CRITICAL, "ckegen:\n");
    for (temp = 0; temp < FM_CKGEN_CLK_END; temp++) {
        if (!ckgen_clk[temp])
            continue;
        dprintf(CRITICAL, "%d: %s: %d KHz\n", temp, ckgen_clk[temp],
            mt_get_ckgen_freq(temp));
    }
}

unsigned int mt_get_cpu_freq(void)
{
#if FPGA_PLATFORM
    return 0;
#else
    return mt_get_abist_freq(FM_AD_ARMPLL_CK);
#endif
}

unsigned int mt_get_bus_freq(void)
{
#if FPGA_PLATFORM
    return 0;
#else
    return mt_get_ckgen_freq(FM_AXI_CK);
#endif
}

/* mt_pll_post_init() should be invoked after pmic_init */
void mt_pll_post_init(void)
{
#if DEBUG_FQMTR
    dump_fqmtr();

    dprintf(CRITICAL, "AP_PLL_CON1= 0x%x\n", DRV_Reg32(AP_PLL_CON1));
    dprintf(CRITICAL, "AP_PLL_CON2= 0x%x\n", DRV_Reg32(AP_PLL_CON2));
    dprintf(CRITICAL, "CLKSQ_STB_CON0= 0x%x\n", DRV_Reg32(CLKSQ_STB_CON0));
    dprintf(CRITICAL, "PLL_ISO_CON0= 0x%x\n", DRV_Reg32(PLL_ISO_CON0));
    dprintf(CRITICAL, "ARMPLL_CON0= 0x%x\n", DRV_Reg32(ARMPLL_CON0));
    dprintf(CRITICAL, "ARMPLL_CON1= 0x%x\n", DRV_Reg32(ARMPLL_CON1));
    dprintf(CRITICAL, "ARMPLL_CON3= 0x%x\n", DRV_Reg32(ARMPLL_CON3));
    dprintf(CRITICAL, "MAINPLL_CON0 = 0x%x\n", DRV_Reg32(MAINPLL_CON0));
    dprintf(CRITICAL, "MAINPLL_CON1 = 0x%x\n", DRV_Reg32(MAINPLL_CON1));
    dprintf(CRITICAL, "MAINPLL_CON3 = 0x%x\n", DRV_Reg32(MAINPLL_CON3));
    dprintf(CRITICAL, "UPLL_CON0= 0x%x\n", DRV_Reg32(UNIVPLL_CON0));
    dprintf(CRITICAL, "UPLL_CON1= 0x%x\n", DRV_Reg32(UNIVPLL_CON1));
    dprintf(CRITICAL, "UPLL_CON3= 0x%x", DRV_Reg32(UNIVPLL_CON3));
    dprintf(CRITICAL, "MMSYS_CG_CON0= 0x%x, \n", DRV_Reg32(MMSYS_CG_CON0));
#endif /* DEBUG_FQMTR */
    dprintf(CRITICAL, "cpu_freq = %d KHz\n", mt_get_cpu_freq());
    dprintf(CRITICAL, "bus_freq = %d KHz\n", mt_get_bus_freq());
}

void mt_pll_init(void)
{
  dprintf(CRITICAL, "mt_pll_init +\n");
  unsigned int temp;
  unsigned int iosel;
/*************
 * CLKSQ
 * ***********/
    DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) | 0x1)); // [0] CLKSQ_EN = 1
    udelay(100);  // wait 100us
    DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) | 0x2)); // [1] CLKSQ_LPF_EN =1

/*************
 * xPLL PWR ON
 **************/
    DRV_WriteReg32(ARMPLL_CON3, (DRV_Reg32(ARMPLL_CON3) | 0x1));    // [0]ARMPLL_PWR_ON = 1
    DRV_WriteReg32(MAINPLL_CON3, (DRV_Reg32(MAINPLL_CON3) | 0x1));  // [0]MAINPLL_PWR_ON = 1
    DRV_WriteReg32(UNIVPLL_CON3, (DRV_Reg32(UNIVPLL_CON3) | 0x1));  // [0]UNIVPLL_PWR_ON = 1
    DRV_WriteReg32(APLL1_CON4, (DRV_Reg32(APLL1_CON4) | 0x1));      // [0]APLL1_PWR_ON = 1
    DRV_WriteReg32(APLL2_CON4, (DRV_Reg32(APLL2_CON4) | 0x1));      // [0]APLL2_PWR_ON = 1
    DRV_WriteReg32(IPPLL_CON3, (DRV_Reg32(IPPLL_CON3) | 0x1));    // [0]IPPLL_PWR_ON = 1
    DRV_WriteReg32(TCONPLL_CON3, (DRV_Reg32(TCONPLL_CON3) | 0x1));    // [0]TCONPLL_PWR_ON = 1
    DRV_WriteReg32(DSPPLL_CON3, (DRV_Reg32(DSPPLL_CON3) | 0x1));    // [0]DSPPLL_PWR_ON = 1
    DRV_WriteReg32(MSDCPLL_CON3, (DRV_Reg32(MSDCPLL_CON3) | 0x1));    // [0]MSDCPLL_PWR_ON = 1

/*************
 * Wait PWR ready(30ns)
 **************/
    udelay(30);

/******************
* xPLL ISO Disable
*******************/
    DRV_WriteReg32(ARMPLL_CON3, (DRV_Reg32(ARMPLL_CON3) & 0xFFFFFFFD));   // [2]ARMPLL_ISO_EN = 0
    DRV_WriteReg32(MAINPLL_CON3, (DRV_Reg32(MAINPLL_CON3) & 0xFFFFFFFD)); // [2]MAINPLL_ISO_EN = 0
    DRV_WriteReg32(UNIVPLL_CON3, (DRV_Reg32(UNIVPLL_CON3) & 0xFFFFFFFD)); // [2]UNIVPLL_ISO_EN = 0
    DRV_WriteReg32(APLL1_CON4, (DRV_Reg32(APLL1_CON4) & 0xFFFFFFFD));     // [2]APLL1_ISO_EN = 0
    DRV_WriteReg32(APLL2_CON4, (DRV_Reg32(APLL2_CON4) & 0xFFFFFFFD));     // [2]APLL2_ISO_EN = 0
    DRV_WriteReg32(IPPLL_CON3, (DRV_Reg32(IPPLL_CON3) & 0xFFFFFFFD));   // [2]IPPLL_ISO_EN = 0
    DRV_WriteReg32(TCONPLL_CON3, (DRV_Reg32(TCONPLL_CON3) & 0xFFFFFFFD));   // [2]TCONPLL_ISO_EN = 0
    DRV_WriteReg32(DSPPLL_CON3, (DRV_Reg32(DSPPLL_CON3) & 0xFFFFFFFD));   // [2]DSPPLL_ISO_EN = 0
    DRV_WriteReg32(MSDCPLL_CON3, (DRV_Reg32(MSDCPLL_CON3) & 0xFFFFFFFD));   // [2]MSDCPLL_ISO_EN = 0

/********************
 * xPLL Frequency Set
 *********************/
 #if BOOT_FREQ_1000
    DRV_WriteReg32(ARMPLL_CON1, 0x81133B13);  // 1000 MHz
 #else
    DRV_WriteReg32(ARMPLL_CON1, 0x811AEC4E);  // 1400 MHz
 #endif
    DRV_WriteReg32(MAINPLL_CON1, 0x81150000); // 1092 MHz
    DRV_WriteReg32(UNIVPLL_CON1, 0x80180000); // 1248 MHz

    DRV_WriteReg32(APLL1_CON2, 0x6F28BD4C);   // 180.6 MHz
    DRV_WriteReg32(APLL1_CON1, 0x84000000);

    DRV_WriteReg32(APLL2_CON2, 0x78FD5265);   // 196.6 MHz
    DRV_WriteReg32(APLL2_CON1, 0x84000000);

    DRV_WriteReg32(IPPLL_CON1, 0x821713B1);  // 600 MHz
    DRV_WriteReg32(TCONPLL_CON1, 0x83189D89);  // 320 MHz
    DRV_WriteReg32(DSPPLL_CON1, 0x841E0000);  // 195 MHz
    DRV_WriteReg32(MSDCPLL_CON1, 0x831EC4EC);  // 400 MHz

/***********************
 * xPLL Frequency Enable
 ************************/
    DRV_WriteReg32(ARMPLL_CON0, (DRV_Reg32(ARMPLL_CON0) | 0x1));   // [0]ARMPLL_EN = 1
    DRV_WriteReg32(MAINPLL_CON0, (DRV_Reg32(MAINPLL_CON0) | 0x1)); // [0]MAINPLL_EN = 1
    DRV_WriteReg32(UNIVPLL_CON0, (DRV_Reg32(UNIVPLL_CON0) | 0x1)); // [0]UNIVPLL_EN = 1
    DRV_WriteReg32(APLL1_CON0, (DRV_Reg32(APLL1_CON0) | 0x1));     // [0]APLL1_EN = 1
    DRV_WriteReg32(APLL2_CON0, (DRV_Reg32(APLL2_CON0) | 0x1));     // [0]APLL2_EN = 1
    DRV_WriteReg32(IPPLL_CON0, (DRV_Reg32(IPPLL_CON0) | 0x1));     // [0]IPPLL_EN = 1
    DRV_WriteReg32(TCONPLL_CON0, (DRV_Reg32(TCONPLL_CON0) | 0x1));     // [0]TCONPLL_EN = 1
    DRV_WriteReg32(DSPPLL_CON0, (DRV_Reg32(DSPPLL_CON0) | 0x1));     // [0]DSPPLL_EN = 1
    DRV_WriteReg32(MSDCPLL_CON0, (DRV_Reg32(MSDCPLL_CON0) | 0x1));     // [0]MSDCPLL_EN = 1

/*************
 * Wait PWR ready(20ns)
 **************/
    udelay(20); // wait for PLL stable (min delay is 20us)

/***************
 * xPLL DIV RSTB
 ****************/
    DRV_WriteReg32(MAINPLL_CON0, (DRV_Reg32(MAINPLL_CON0) | 0x00800000)); // [23]MAINPLL_DIV_RSTB = 1
    DRV_WriteReg32(UNIVPLL_CON0, (DRV_Reg32(UNIVPLL_CON0) | 0x00800000)); // [23]UNIVPLL_DIV_RSTB = 1

    DRV_WriteReg32(PLLON_CON0, 0x1111F0F0); // armpll/mainpll/mpll sleep control

    DRV_WriteReg32(ACLKEN_DIV, 0x12); // CPU BUS clock freq is divided by 2

/*****************
 * switch CPU clock to ARMPLL
 ******************/
    DRV_WriteReg32(CLK_MISC_CFG_0, DRV_Reg32(CLK_MISC_CFG_0) | 0x30);

    temp = DRV_Reg32(MCU_BUS_MUX) & ~0x600;
    DRV_WriteReg32(MCU_BUS_MUX, temp | 0x200);

/*****************
 * AXI BUS DCM Setting
 ******************/

/*****************
 * 32k setting
 ******************/
#if WITH_EXT_32K
    DRV_WriteReg32(CLK26CALI_2, (DRV_Reg32(CLK26CALI_2) & ~0x3000));
#else
    DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x7FFF0000 | 0x1D3F0000));
    DRV_WriteReg32(CLK26CALI_2, (DRV_Reg32(CLK26CALI_2) & ~0x3000 | 0x3000));
    DRV_WriteReg32(CLK26CALI_0, (DRV_Reg32(CLK26CALI_0) & ~0x7FFF0000 | 0x5D3F0000));
#endif

/************
 * TOP CLKMUX
 *************/
    DRV_WriteReg32(CLK_CFG_0_CLR, 0x07010307);
    DRV_WriteReg32(CLK_CFG_1_CLR, 0x07030307);
    DRV_WriteReg32(CLK_CFG_2_CLR, 0x07030707);
    DRV_WriteReg32(CLK_CFG_3_CLR, 0x07070707);
    DRV_WriteReg32(CLK_CFG_4_CLR, 0x03030303);
    DRV_WriteReg32(CLK_CFG_5_CLR, 0x03030101);
    DRV_WriteReg32(CLK_CFG_6_CLR, 0x07070701);
    DRV_WriteReg32(CLK_CFG_7_CLR, 0x07030707);
    DRV_WriteReg32(CLK_CFG_8_CLR, 0x07070103);
    DRV_WriteReg32(CLK_CFG_9_CLR, 0x07070707);
    DRV_WriteReg32(CLK_CFG_10_CLR, 0x01010307);
    DRV_WriteReg32(CLK_CFG_11_CLR, 0x00000003);

    DRV_WriteReg32(CLK_CFG_0_SET, 0x01000001);
    DRV_WriteReg32(CLK_CFG_1_SET, 0x01010301);
    DRV_WriteReg32(CLK_CFG_2_SET, 0x01000202);
    DRV_WriteReg32(CLK_CFG_3_SET, 0x04040101);
    DRV_WriteReg32(CLK_CFG_4_SET, 0x01020202);
    DRV_WriteReg32(CLK_CFG_5_SET, 0x03030101);
    DRV_WriteReg32(CLK_CFG_6_SET, 0x01000301);
    DRV_WriteReg32(CLK_CFG_7_SET, 0x02030000);
    DRV_WriteReg32(CLK_CFG_8_SET, 0x04040001);
    DRV_WriteReg32(CLK_CFG_9_SET, 0x06010701); /* 312M mm_sel clock rate */
    DRV_WriteReg32(CLK_CFG_10_SET, 0x00000102);
    DRV_WriteReg32(CLK_CFG_11_SET, 0x00000000);

    DRV_WriteReg32(CLK_CFG_UPDATE, 0xffffffff);
    DRV_WriteReg32(CLK_CFG_UPDATE1, 0x00001fff);

    /* CONNSYS MCU reset */
    temp = DRV_Reg32(WDT_SWSYSRST);
    dprintf(CRITICAL, "before: WDT_SWSYSRST = 0x%x\n", DRV_Reg32(WDT_SWSYSRST));
    DRV_WriteReg32(WDT_SWSYSRST, (temp | CONNSYS_CPU_SW_RST | RGU_KEY_CODE));
    dprintf(CRITICAL, "after: WDT_SWSYSRST = 0x%x\n", DRV_Reg32(WDT_SWSYSRST));

    /* CLKSQ setting */
    iosel = (DRV_Reg32(IOSEL) & 0x80000000) >> 31;

    if (iosel) {
        /* BB clk source from 26M_CLKSQ */
        dprintf(CRITICAL, "BB clk source from 26M_CLKSQ\n");
        DRV_WriteReg32(DA_XTAL_CTRL2, (DRV_Reg32(DA_XTAL_CTRL2) & ~0x800000) | 0x800000); // FPM->PDN
        DRV_WriteReg32(AP_PLL_CON3, (DRV_Reg32(AP_PLL_CON3) & ~0x6)); // CLKSQ sleep control
    } else {
        /* BB clk source from XTAL, turn off clksq */
        dprintf(CRITICAL, "BB clk source from XTAL\n");
        DRV_WriteReg32(AP_PLL_CON3, (DRV_Reg32(AP_PLL_CON3) & ~0x6) | 0x6);
        DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) & ~0x1));

        #if WITH_CLKSQ_MONCK_OFF
        DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) & ~0x80) | 0x80);
        #endif
    }

#if ALL_CLK_ON
/************
 * TOP CG
 *************/
    DRV_WriteReg32(CLK_AUDDIV_4, (DRV_Reg32(CLK_AUDDIV_4) & ~0x00000007));
    DRV_WriteReg32(CLK_MISC_CFG_0, (DRV_Reg32(CLK_MISC_CFG_0) & ~0x00c00300) | 0x00c00300);
    DRV_WriteReg32(CLK_MODE, (DRV_Reg32(CLK_MODE) & ~0x00030c00));

/************
 * INFRA_AO CG
 *************/
    DRV_WriteReg32(MODULE_SW_CG_0_CLR, 0x9dff8740);
    DRV_WriteReg32(MODULE_SW_CG_1_CLR, 0x23044796);
    DRV_WriteReg32(MODULE_SW_CG_2_CLR, 0x0800005b);
    DRV_WriteReg32(MODULE_SW_CG_3_CLR, 0x07c00780);
    DRV_WriteReg32(MODULE_SW_CG_4_CLR, 0x00000a8e);
    DRV_WriteReg32(INFRA_MFG_MASTER_M0_GALS_CTRL, (DRV_Reg32(INFRA_MFG_MASTER_M0_GALS_CTRL) & ~0x00000100) | 0x00000100);

    DRV_WriteReg32(IPSYS_EMI_CK_CG, (DRV_Reg32(IPSYS_EMI_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_SRAM_CK_CG, (DRV_Reg32(IPSYS_SRAM_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_AXI_CK_CG, (DRV_Reg32(IPSYS_AXI_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_NNA0_PWR_ON, (DRV_Reg32(IPSYS_NNA0_PWR_ON) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_NNA1_PWR_ON, (DRV_Reg32(IPSYS_NNA1_PWR_ON) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_WFST_PWR_ON, (DRV_Reg32(IPSYS_WFST_PWR_ON) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_NNAO_CK_CG, (DRV_Reg32(IPSYS_NNAO_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_NNA1_CK_CG, (DRV_Reg32(IPSYS_NNA1_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_WFST_CK_CG, (DRV_Reg32(IPSYS_WFST_CK_CG) & ~0x1) | 0x1);
    DRV_WriteReg32(IPSYS_26M_CK_CG, (DRV_Reg32(IPSYS_26M_CK_CG) & ~0x1) | 0x1);

/*************
 * for MTCMOS
 *************/
    spm_mtcmos_ctrl_conn(STA_POWER_ON);
    spm_mtcmos_ctrl_ip0(STA_POWER_ON);
    spm_mtcmos_ctrl_ip1(STA_POWER_ON);
    spm_mtcmos_ctrl_ip2(STA_POWER_ON);
    spm_mtcmos_ctrl_usb_mac_p1(STA_POWER_ON);
    spm_mtcmos_ctrl_dsp(STA_POWER_ON);
    /*spm_mtcmos_ctrl_audio(STA_POWER_ON);*/
    spm_mtcmos_ctrl_asrc(STA_POWER_ON);


#endif /* ALL_CLK_ON */

/*************
 * enable mmsys/imgsys power domain/clocks
 *************/
    spm_mtcmos_ctrl_mm(STA_POWER_ON);
    spm_mtcmos_ctrl_img(STA_POWER_ON);

    DRV_WriteReg32(MMSYS_CG_CLR0, 0x03f800bf);
    DRV_WriteReg32(IMGSYS_CG_CLR0, 0x00a18935);
    DRV_WriteReg32(IMGSYS_CG_CLR1, 0x0000000e);

	dprintf(CRITICAL, "mt_pll_init done\n");
}

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
#include <platform/mt8518.h>
#include <platform/pll.h>
#include <platform/spm.h>
#include <platform/spm_mtcmos.h>
#include <platform/sec_devinfo.h>

typedef volatile unsigned int *V_UINT32P;

#define ALL_CLK_ON      1
#define DEBUG_FQMTR     0

#define udelay(x)       spin(x)
#define mdelay(x)       udelay((x) * 1000)

#define DRV_WriteReg32(addr, data)  writel(data, addr)
#define DRV_Reg32(addr) readl(addr)

#define FREQ_MTR_CTRL_REG       (CKSYS_BASE + 0x10)
#define FREQ_MTR_CTRL_RDATA     (CKSYS_BASE + 0x14)

#define RG_FQMTR_CKDIV_GET(x)           (((x) >> 28) & 0x3)
#define RG_FQMTR_CKDIV_SET(x)           (((x)& 0x3) << 28)
#define RG_FQMTR_FIXCLK_SEL_GET(x)      (((x) >> 24) & 0x3)
#define RG_FQMTR_FIXCLK_SEL_SET(x)      (((x)& 0x3) << 24)
#define RG_FQMTR_MONCLK_SEL_GET(x)      (((x) >> 16) & 0x7f)
#define RG_FQMTR_MONCLK_SEL_SET(x)      (((x)& 0x7f) << 16)
#define RG_FQMTR_MONCLK_EN_GET(x)       (((x) >> 15) & 0x1)
#define RG_FQMTR_MONCLK_EN_SET(x)       (((x)& 0x1) << 15)
#define RG_FQMTR_MONCLK_RST_GET(x)      (((x) >> 14) & 0x1)
#define RG_FQMTR_MONCLK_RST_SET(x)      (((x)& 0x1) << 14)
#define RG_FQMTR_MONCLK_WINDOW_GET(x)   (((x) >> 0) & 0xfff)
#define RG_FQMTR_MONCLK_WINDOW_SET(x)   (((x)& 0xfff) << 0)

#define RG_FQMTR_CKDIV_DIV_2    0
#define RG_FQMTR_CKDIV_DIV_4    1
#define RG_FQMTR_CKDIV_DIV_8    2
#define RG_FQMTR_CKDIV_DIV_16   3

#define RG_FQMTR_FIXCLK_26MHZ   0
#define RG_FQMTR_FIXCLK_32KHZ   2

enum rg_fqmtr_monclk {
	FM_CLK_NULL			=  0,

	FM_G_MAINPLL_D8_188M_CK		=  1,
	FM_G_MAINPLL_D11_137M_CK	=  2,
	FM_G_MAINPLL_D12_126M_CK	=  3,
	FM_G_MAINPLL_D20_76M_CK		=  4,
	FM_G_MAINPLL_D7_215M_CK		=  5,
	FM_G_UNIVPLL_D16_78M_CK		=  6,
	FM_G_UNIVPLL_D24_52M_CK		=  7,
	FM_CSW_MUX_NFI2X_CK		=  8,
	FM_AD_SYS_26M_CK		= 11,
	FM_AD_USB_F48M_CK		= 12,
	FM_CSW_GFMUX_EMI1X_CK		= 13,
	FM_CSW_GFMUX_AXIBUS_CK		= 14,
	FM_CSW_MUX_SMI_CK		= 15,
	FM_CSW_GFMUX_UART0_CK		= 16,
	FM_CSW_GFMUX_UART1_CK		= 17,
	FM_HF_FAUD_INTBUS_CK		= 18,
	FM_CSW_MUX_MSDC0_CK		= 19,
	FM_CSW_MUX_MSDC1_CK		= 20,
	FM_AD_MPPLL_TST_CK		= 22,
	FM_AD_PLLGP_TST_CK		= 23,
	FM_CSW_52M_CK			= 24,
	FM_MCUSYS_DEBUG_MON0		= 25,
	FM_CSW_32K_CK			= 26,
	FM_AD_MEMPLL_MONCLK		= 27,
	FM_AD_MEMPLL2_MONCLK		= 28,
	FM_AD_MEMPLL3_MONCLK		= 29,
	FM_AD_MEMPLL4_MONCLK		= 30,
	FM_HF_SPIS_CK			= 32,
	FM_HF_GCPU_CK			= 33,
	FM_CSW_MUX_PWM_MM_CK		= 34,
	FM_CSW_MUX_DDRPHYCFG_CK		= 35,
	FM_CSW_MUX_PMICSPI_CK		= 36,
	FM_CSW_MUX_SPI_CK1		= 37,
	FM_CSW_104M_CK			= 38,
	FM_CSW_78M_CK			= 39,
	FM_CSW_MUX_SPINOR_CK		= 40,
	FM_CSW_MUX_ETH_CK		= 41,
	FM_CSW_MUX_AUDIO_CK		= 42,
	FM_CSW_MUX_IMGRZ_SYS_CK		= 43,
	FM_CSW_MUX_PNG_SYS_CK		= 44,
	FM_CSW_MUX_GRAPH_ECLK		= 45,
	FM_CSW_MUX_FDBI_CK		= 46,
	FM_CSW_MUX_AUD1_CK		= 47,
	FM_CSW_MUX_AUD2_CK		= 48,
	FM_CSW_MUX_FA2SYS_CK		= 49,
	FM_CSW_MUX_FA1SYS_CK		= 50,
	FM_CSW_MUX_SPINFI_BCLK_CK	= 51,
	FM_CSW_MUX_PWM_INFRA_CK		= 52,
	FM_CSW_MUX_AUD_SPDIF_IN_CK	= 53,
	FM_CSW_GFMUX_UART2_CK		= 54,
	FM_CSW_MUX_DBG_ATCLK_CK		= 56,
	FM_CSW_MUX_FASM_M_CK		= 57,
	FM_CSW_MUX_FECC_CK		= 58,
	FM_FQ_TRNG_FREQ_DEBUG_OUT0	= 59,
	FM_FQ_TRNG_FREQ_DEBUG_OUT1	= 60,
	FM_FQ_USB20_CLK480M		= 61,
	FM_CSW_MUX_HF_PE2_MAC_CK	= 62,
	FM_CSW_MUX_HF_CMSYS_CK		= 63,
	FM_AD_ARMPLL_650M_CK		= 64,
	FM_AD_MAINPLL_1501P5M_CK	= 65,
	FM_AD_UNIVPLL_1248M_CK		= 66,
	FM_AD_MMPLL_380M_CK		= 67,
	FM_AD_TVDPLL_594M_CK		= 68,
	FM_AD_AUD1PLL_180P6336M_CK	= 69,
	FM_AD_AUD2PLL_196P608M_CK	= 70,
	FM_AD_ARMPLL_650M_VPROC_CK	= 71,
	FM_AD_USB20_48M_CK		= 72,
	FM_AD_UNIV_48M_CK		= 73,
	FM_AD_TPHY_26M_CK		= 74,
	FM_FQ_HF_FASM_L_CK		= 75,
	FM_AD_MEM_26M_CK		= 77,
	FM_AD_MEMPLL5_MONCLK		= 78,
	FM_AD_MEMPLL6_MONCLK		= 79,
	FM_CSW_MUX_MSDC2_CK		= 80,
	FM_CSW_MUX_MSDC0_HCLK50_CK	= 81,
	FM_CSW_MUX_MSDC2_HCLK50_CK	= 82,
	FM_CSW_MUX_DISP_DPI_CK		= 83,
	FM_CSW_MUX_SPI_CK2		= 84,
	FM_CSW_MUX_SPI_CK3		= 85,
	FM_CSW_MUX_HAPLL1_CK		= 86,
	FM_CSW_MUX_HAPLL2_CK		= 87,
	FM_CSW_MUX_I2C_CK		= 88,
	FM_CSW_SEJ_13M_CK		= 89,

	FM_CLK_END			= 90,
};

const char* rg_fqmtr_monclk_name[] = {
	[FM_G_MAINPLL_D8_188M_CK]	= "g_mainpll_d8_188m_ck",
	[FM_G_MAINPLL_D11_137M_CK]	= "g_mainpll_d11_137m_ck",
	[FM_G_MAINPLL_D12_126M_CK]	= "g_mainpll_d12_126m_ck",
	[FM_G_MAINPLL_D20_76M_CK]	= "g_mainpll_d20_76m_ck",
	[FM_G_MAINPLL_D7_215M_CK]	= "g_mainpll_d7_215m_ck",
	[FM_G_UNIVPLL_D16_78M_CK]	= "g_univpll_d16_78m_ck",
	[FM_G_UNIVPLL_D24_52M_CK]	= "g_univpll_d24_52m_ck",
	[FM_CSW_MUX_NFI2X_CK]		= "csw_mux_nfi2x_ck",
	[FM_AD_SYS_26M_CK]		= "AD_SYS_26M_CK",
	[FM_AD_USB_F48M_CK]		= "AD_USB_F48M_CK",
	[FM_CSW_GFMUX_EMI1X_CK]		= "csw_gfmux_emi1x_ck",
	[FM_CSW_GFMUX_AXIBUS_CK]	= "csw_gfmux_axibus_ck",
	[FM_CSW_MUX_SMI_CK]		= "csw_mux_smi_ck",
	[FM_CSW_GFMUX_UART0_CK]		= "csw_gfmux_uart0_ck",
	[FM_CSW_GFMUX_UART1_CK]		= "csw_gfmux_uart1_ck",
	[FM_HF_FAUD_INTBUS_CK]		= "hf_faud_intbus_ck",
	[FM_CSW_MUX_MSDC0_CK]		= "csw_mux_msdc0_ck",
	[FM_CSW_MUX_MSDC1_CK]		= "csw_mux_msdc1_ck",
	[FM_AD_MPPLL_TST_CK]		= "AD_MPPLL_TST_CK",
	[FM_AD_PLLGP_TST_CK]		= "AD_PLLGP_TST_CK",
	[FM_CSW_52M_CK]			= "csw_52m_ck",
	[FM_MCUSYS_DEBUG_MON0]		= "mcusys_debug_mon0",
	[FM_CSW_32K_CK]			= "csw_32k_ck",
	[FM_AD_MEMPLL_MONCLK]		= "AD_MEMPLL_MONCLK",
	[FM_AD_MEMPLL2_MONCLK]		= "AD_MEMPLL2_MONCLK",
	[FM_AD_MEMPLL3_MONCLK]		= "AD_MEMPLL3_MONCLK",
	[FM_AD_MEMPLL4_MONCLK]		= "AD_MEMPLL4_MONCLK",
	[FM_HF_SPIS_CK]			= "hf_spis_ck",
	[FM_HF_GCPU_CK]			= "hf_gcpu_ck",
	[FM_CSW_MUX_PWM_MM_CK]		= "csw_mux_pwm_mm_ck",
	[FM_CSW_MUX_DDRPHYCFG_CK]	= "csw_mux_ddrphycfg_ck",
	[FM_CSW_MUX_PMICSPI_CK]		= "csw_mux_pmicspi_ck",
	[FM_CSW_MUX_SPI_CK1]		= "csw_mux_spi_ck1",
	[FM_CSW_104M_CK]		= "csw_104m_ck",
	[FM_CSW_78M_CK]			= "csw_78m_ck",
	[FM_CSW_MUX_SPINOR_CK]		= "csw_mux_spinor_ck",
	[FM_CSW_MUX_ETH_CK]		= "csw_mux_eth_ck",
	[FM_CSW_MUX_AUDIO_CK]		= "csw_mux_audio_ck",
	[FM_CSW_MUX_IMGRZ_SYS_CK]	= "csw_mux_imgrz_sys_ck",
	[FM_CSW_MUX_PNG_SYS_CK]		= "csw_mux_png_sys_ck",
	[FM_CSW_MUX_GRAPH_ECLK]		= "csw_mux_graph_eclk",
	[FM_CSW_MUX_FDBI_CK]		= "csw_mux_fdbi_ck",
	[FM_CSW_MUX_AUD1_CK]		= "csw_mux_aud1_ck",
	[FM_CSW_MUX_AUD2_CK]		= "csw_mux_aud2_ck",
	[FM_CSW_MUX_FA2SYS_CK]		= "csw_mux_fa2sys_ck",
	[FM_CSW_MUX_FA1SYS_CK]		= "csw_mux_fa1sys_ck",
	[FM_CSW_MUX_SPINFI_BCLK_CK]	= "csw_mux_spinfi_bclk_ck",
	[FM_CSW_MUX_PWM_INFRA_CK]	= "csw_mux_pwm_infra_ck",
	[FM_CSW_MUX_AUD_SPDIF_IN_CK]	= "csw_mux_aud_spdif_in_ck",
	[FM_CSW_GFMUX_UART2_CK]		= "csw_gfmux_uart2_ck",
	[FM_CSW_MUX_DBG_ATCLK_CK]	= "csw_mux_dbg_atclk_ck",
	[FM_CSW_MUX_FASM_M_CK]		= "csw_mux_fasm_m_ck",
	[FM_CSW_MUX_FECC_CK]		= "csw_mux_fecc_ck",
	[FM_FQ_TRNG_FREQ_DEBUG_OUT0]	= "fq_trng_freq_debug_out0",
	[FM_FQ_TRNG_FREQ_DEBUG_OUT1]	= "fq_trng_freq_debug_out1",
	[FM_FQ_USB20_CLK480M]		= "fq_usb20_clk480m",
	[FM_CSW_MUX_HF_PE2_MAC_CK]	= "csw_mux_hf_pe2_mac_ck",
	[FM_CSW_MUX_HF_CMSYS_CK]	= "csw_mux_hf_cmsys_ck",
	[FM_AD_ARMPLL_650M_CK]		= "AD_ARMPLL_650M_CK",
	[FM_AD_MAINPLL_1501P5M_CK]	= "AD_MAINPLL_1501P5M_CK",
	[FM_AD_UNIVPLL_1248M_CK]	= "AD_UNIVPLL_1248M_CK",
	[FM_AD_MMPLL_380M_CK]		= "AD_MMPLL_380M_CK",
	[FM_AD_TVDPLL_594M_CK]		= "AD_TVDPLL_594M_CK",
	[FM_AD_AUD1PLL_180P6336M_CK]	= "AD_AUD1PLL_180P6336M_CK",
	[FM_AD_AUD2PLL_196P608M_CK]	= "AD_AUD2PLL_196P608M_CK",
	[FM_AD_ARMPLL_650M_VPROC_CK]	= "AD_ARMPLL_650M_VPROC_CK",
	[FM_AD_USB20_48M_CK]		= "AD_USB20_48M_CK",
	[FM_AD_UNIV_48M_CK]		= "AD_UNIV_48M_CK",
	[FM_AD_TPHY_26M_CK]		= "AD_TPHY_26M_CK",
	[FM_FQ_HF_FASM_L_CK]		= "fq_hf_fasm_l_ck",
	[FM_AD_MEM_26M_CK]		= "AD_MEM_26M_CK",
	[FM_AD_MEMPLL5_MONCLK]		= "AD_MEMPLL5_MONCLK",
	[FM_AD_MEMPLL6_MONCLK]		= "AD_MEMPLL6_MONCLK",
	[FM_CSW_MUX_MSDC2_CK]		= "csw_mux_msdc2_ck",
	[FM_CSW_MUX_MSDC0_HCLK50_CK]	= "csw_mux_msdc0_hclk50_ck",
	[FM_CSW_MUX_MSDC2_HCLK50_CK]	= "csw_mux_msdc2_hclk50_ck",
	[FM_CSW_MUX_DISP_DPI_CK]	= "csw_mux_disp_dpi_ck",
	[FM_CSW_MUX_SPI_CK2]		= "csw_mux_spi_ck2",
	[FM_CSW_MUX_SPI_CK3]		= "csw_mux_spi_ck3",
	[FM_CSW_MUX_HAPLL1_CK]		= "csw_mux_hapll1_ck",
	[FM_CSW_MUX_HAPLL2_CK]		= "csw_mux_hapll2_ck",
	[FM_CSW_MUX_I2C_CK]		= "csw_mux_i2c_ck",
	[FM_CSW_SEJ_13M_CK]		= "csw_sej_13m_ck",
};

#define RG_FQMTR_EN     1
#define RG_FQMTR_RST    1

#define RG_FRMTR_WINDOW     1023

unsigned int do_fqmtr_ctrl(int fixclk, int monclk_sel)
{
    u32 value = 0;

    if(!((fixclk == RG_FQMTR_FIXCLK_26MHZ) | (fixclk == RG_FQMTR_FIXCLK_32KHZ))) {
		dprintf(CRITICAL, "do_fqmtr_ctrl: clock source wrong!\n");
		return 0;
    }
    // reset
    DRV_WriteReg32(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(RG_FQMTR_RST));
    // reset deassert
    DRV_WriteReg32(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(!RG_FQMTR_RST));
    // set window and target
    DRV_WriteReg32(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_WINDOW_SET(RG_FRMTR_WINDOW) |
                RG_FQMTR_MONCLK_SEL_SET(monclk_sel) |
                RG_FQMTR_FIXCLK_SEL_SET(fixclk) |
                RG_FQMTR_MONCLK_EN_SET(RG_FQMTR_EN));
    udelay(100);
    value = DRV_Reg32(FREQ_MTR_CTRL_RDATA);
    // reset
    DRV_WriteReg32(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(RG_FQMTR_RST));
    // reset deassert
    DRV_WriteReg32(FREQ_MTR_CTRL_REG, RG_FQMTR_MONCLK_RST_SET(!RG_FQMTR_RST));
    if (fixclk == RG_FQMTR_FIXCLK_26MHZ)
        return ((26000 * value) / (RG_FRMTR_WINDOW + 1));
    else
        return ((32 * value) / (RG_FRMTR_WINDOW + 1));

}

void dump_fqmtr(void)
{
    int i = 0;
    unsigned int ret;

    // fixclk = RG_FQMTR_FIXCLK_26MHZ
    for (i = 0; i < FM_CLK_END; i++)
    {
		if (!rg_fqmtr_monclk_name[i])
            continue;

        ret = do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, i);
        dprintf(CRITICAL, "%s - %d KHz\n", rg_fqmtr_monclk_name[i], ret);
    }
}

unsigned int mt_get_cpu_freq(void)
{
#if FPGA_PLATFORM
    return 0;
#else
    return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, FM_AD_ARMPLL_650M_CK);
#endif
}

unsigned int mt_get_mem_freq(void)
{
#if FPGA_PLATFORM
    return 0;
#else
    return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, FM_AD_MEMPLL6_MONCLK);
#endif
}

unsigned int mt_get_bus_freq(void)
{
#if FPGA_PLATFORM
    return 0;
#else
    return do_fqmtr_ctrl(RG_FQMTR_FIXCLK_26MHZ, FM_CSW_GFMUX_AXIBUS_CK);
#endif
}

/* mt_pll_post_init() should be invoked after pmic_init */
void mt_pll_post_init(void)
{
    unsigned int temp;

/*****************
 * xPLL HW Control
 ******************/
    // TBD (mem init)
    DRV_WriteReg32(AP_PLL_CON1, (DRV_Reg32(AP_PLL_CON1) & 0xFCFCEFCC)); // Main, ARM PLL HW Control
    DRV_WriteReg32(AP_PLL_CON2, (DRV_Reg32(AP_PLL_CON2) & 0xFFFFFFFC)); // Main, ARM PLL HW Control

/*****************
 * switch CPU clock to ARMPLL
 ******************/
    temp = DRV_Reg32(MCU_BUS_MUX) & ~0x600;
    DRV_WriteReg32(MCU_BUS_MUX, temp | 0x200);

#if DEBUG_FQMTR
    dump_fqmtr();

    dprintf(CRITICAL, "AP_PLL_CON1= 0x%x\n", DRV_Reg32(AP_PLL_CON1));
    dprintf(CRITICAL, "AP_PLL_CON2= 0x%x\n", DRV_Reg32(AP_PLL_CON2));
    dprintf(CRITICAL, "CLKSQ_STB_CON0= 0x%x\n", DRV_Reg32(CLKSQ_STB_CON0));
    dprintf(CRITICAL, "PLL_ISO_CON0= 0x%x\n", DRV_Reg32(PLL_ISO_CON0));
    dprintf(CRITICAL, "ARMPLL_CON0= 0x%x\n", DRV_Reg32(ARMPLL_CON0));
    dprintf(CRITICAL, "ARMPLL_CON1= 0x%x\n", DRV_Reg32(ARMPLL_CON1));
    dprintf(CRITICAL, "ARMPLL_PWR_CON0= 0x%x\n", DRV_Reg32(ARMPLL_PWR_CON0));
    dprintf(CRITICAL, "MPLL_CON0= 0x%x\n", DRV_Reg32(MAINPLL_CON0));
    dprintf(CRITICAL, "MPLL_CON1= 0x%x\n", DRV_Reg32(MAINPLL_CON1));
    dprintf(CRITICAL, "MPLL_PWR_CON0= 0x%x\n", DRV_Reg32(MAINPLL_PWR_CON0));
    dprintf(CRITICAL, "UPLL_CON0= 0x%x\n", DRV_Reg32(UNIVPLL_CON0));
    dprintf(CRITICAL, "UPLL_CON1= 0x%x\n", DRV_Reg32(UNIVPLL_CON1));
    dprintf(CRITICAL, "UPLL_PWR_CON0= 0x%x", DRV_Reg32(UNIVPLL_PWR_CON0));
    dprintf(CRITICAL, "MMSYS_CG_CON0= 0x%x, \n", DRV_Reg32(MMSYS_CG_CON0));
#endif /* DEBUG_FQMTR */
    dprintf(CRITICAL, "cpu_freq = %d KHz\n", mt_get_cpu_freq());
    dprintf(CRITICAL, "bus_freq = %d KHz\n", mt_get_bus_freq());
}

void mt_pll_init(void)
{
  dprintf(CRITICAL, "mt_pll_init +\n");
  unsigned int devinfo;
/*************
 * CLKSQ
 * ***********/
    DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) | 0x1)); // [0] CLKSQ_EN = 1
    udelay(100);  // wait 100us
    DRV_WriteReg32(AP_PLL_CON0, (DRV_Reg32(AP_PLL_CON0) | 0x2)); // [1] CLKSQ_LPF_EN =1

/*************
 * xPLL PWR ON
 **************/
    DRV_WriteReg32(ARMPLL_PWR_CON0, (DRV_Reg32(ARMPLL_PWR_CON0) | 0x1));    // [0]ARMPLL_PWR_ON = 1
    DRV_WriteReg32(MAINPLL_PWR_CON0, (DRV_Reg32(MAINPLL_PWR_CON0) | 0x1));  // [0]MAINPLL_PWR_ON = 1
    DRV_WriteReg32(UNIVPLL_PWR_CON0, (DRV_Reg32(UNIVPLL_PWR_CON0) | 0x1));  // [0]UNIVPLL_PWR_ON = 1
    DRV_WriteReg32(MMPLL_PWR_CON0, (DRV_Reg32(MMPLL_PWR_CON0) | 0x1));      // [0]MMPLL_PWR_ON = 1
    DRV_WriteReg32(APLL1_PWR_CON0, (DRV_Reg32(APLL1_PWR_CON0) | 0x1));      // [0]APLL1_PWR_ON = 1
    DRV_WriteReg32(APLL2_PWR_CON0, (DRV_Reg32(APLL2_PWR_CON0) | 0x1));      // [0]APLL2_PWR_ON = 1
    DRV_WriteReg32(TVDPLL_PWR_CON0, (DRV_Reg32(TVDPLL_PWR_CON0) | 0x1));    // [0]TVDPLL_PWR_ON = 1

/*************
 * Wait PWR ready(30ns)
 **************/
    udelay(30);

/******************
* xPLL ISO Disable
*******************/
    DRV_WriteReg32(ARMPLL_PWR_CON0, (DRV_Reg32(ARMPLL_PWR_CON0) & 0xFFFFFFFD));   // [2]ARMPLL_ISO_EN = 0
    DRV_WriteReg32(MAINPLL_PWR_CON0, (DRV_Reg32(MAINPLL_PWR_CON0) & 0xFFFFFFFD)); // [2]MAINPLL_ISO_EN = 0
    DRV_WriteReg32(UNIVPLL_PWR_CON0, (DRV_Reg32(UNIVPLL_PWR_CON0) & 0xFFFFFFFD)); // [2]UNIVPLL_ISO_EN = 0
    DRV_WriteReg32(MMPLL_PWR_CON0, (DRV_Reg32(MMPLL_PWR_CON0) & 0xFFFFFFFD));     // [2]MMPLL_ISO_EN = 0
    DRV_WriteReg32(APLL1_PWR_CON0, (DRV_Reg32(APLL1_PWR_CON0) & 0xFFFFFFFD));     // [2]APLL1_ISO_EN = 0
    DRV_WriteReg32(APLL2_PWR_CON0, (DRV_Reg32(APLL2_PWR_CON0) & 0xFFFFFFFD));     // [2]APLL2_ISO_EN = 0
    DRV_WriteReg32(TVDPLL_PWR_CON0, (DRV_Reg32(TVDPLL_PWR_CON0) & 0xFFFFFFFD));   // [2]TVDPLL_ISO_EN = 0

/********************
 * xPLL Frequency Set
 *********************/
    DRV_WriteReg32(ARMPLL_CON1, 0x810d7627);  // 700 MHz

    DRV_WriteReg32(MAINPLL_CON1, 0x800e7000); // 1501 MHz

    DRV_WriteReg32(UNIVPLL_CON1, 0x81000060); // 1248 MHz

	DRV_WriteReg32(MMPLL_CON1, 0x820F0000);   // 390 MHz

    DRV_WriteReg32(APLL1_CON1, 0xb7945ea6);   // 180.6 MHz
    DRV_WriteReg32(APLL1_CON0, 0x16);
    DRV_WriteReg32(APLL1_CON_TUNER, 0x37945ea7); // 180.6MHz + 1

    DRV_WriteReg32(APLL2_CON1, 0xbc7ea932);   // 196.6 MHz
    DRV_WriteReg32(APLL2_CON0, 0x16);
    DRV_WriteReg32(APLL2_CON_TUNER, 0x3c7ea933); // 196.6 MHz + 1

    DRV_WriteReg32(TVDPLL_CON1, 0x8216d89e);  // 594 MHz

/***********************
 * xPLL Frequency Enable
 ************************/
    DRV_WriteReg32(ARMPLL_CON0, (DRV_Reg32(ARMPLL_CON0) | 0x1));   // [0]ARMPLL_EN = 1
    DRV_WriteReg32(MAINPLL_CON0, (DRV_Reg32(MAINPLL_CON0) | 0x1)); // [0]MAINPLL_EN = 1
    DRV_WriteReg32(UNIVPLL_CON0, (DRV_Reg32(UNIVPLL_CON0) | 0x1)); // [0]UNIVPLL_EN = 1
    DRV_WriteReg32(MMPLL_CON0, (DRV_Reg32(MMPLL_CON0) | 0x1));     // [0]MMPLL_EN = 1
    DRV_WriteReg32(APLL1_CON0, (DRV_Reg32(APLL1_CON0) | 0x1));     // [0]APLL1_EN = 1
    DRV_WriteReg32(APLL2_CON0, (DRV_Reg32(APLL2_CON0) | 0x1));     // [0]APLL2_EN = 1
    DRV_WriteReg32(TVDPLL_CON0, (DRV_Reg32(TVDPLL_CON0) | 0x1D));  // [0]TVDPLL_EN = 1

/*************
 * Wait PWR ready(20ns)
 **************/
    udelay(20); // wait for PLL stable (min delay is 20us)

/***************
 * xPLL DIV RSTB
 ****************/
    DRV_WriteReg32(MAINPLL_CON0, (DRV_Reg32(MAINPLL_CON0) | 0x08000000)); // [27]MAINPLL_DIV_RSTB = 1
    DRV_WriteReg32(UNIVPLL_CON0, (DRV_Reg32(UNIVPLL_CON0) | 0x08000000)); // ]27]UNIVPLL_DIV_RSTB = 1


/**************
 * INFRA CLKMUX
 ***************/
    DRV_WriteReg32(ACLKEN_DIV, 0x12); // CPU BUS clock freq is divided by 2
    DRV_WriteReg32(INFRA_CLKSEL, (DRV_Reg32(INFRA_CLKSEL) | 0x1E)); // i2c_bclk_sel bit[4:1] = 1

/*****************
 * AXI BUS DCM Setting
 ******************/
    DRV_WriteReg32(INFRABUS_DCMCTL1, 0x80000000);
    if (readl(VERSION_BASE_VIRT+0x08) == 0xCB00)
        DRV_WriteReg32(INFRABUS_DCMCTL0, (DRV_Reg32(INFRABUS_DCMCTL0) & ~0xC000C00) | 0x4000400);

/************
 * TOP CLKMUX
 *************/
	DRV_WriteReg32(CLK_MUX_SEL15, (DRV_Reg32(CLK_MUX_SEL15) & ~0x77fc000f) | 0x3380000a);
	DRV_WriteReg32(CLK_MUX_SEL16, (DRV_Reg32(CLK_MUX_SEL16) & ~0x00000ff7) | 0x00000081);
#if WITH_32K_OSC
	DRV_WriteReg32(CLK_MUX_SEL17, (DRV_Reg32(CLK_MUX_SEL17) & ~0x0001ffc0) | 0x00000200);
#else
	DRV_WriteReg32(CLK_MUX_SEL17, (DRV_Reg32(CLK_MUX_SEL17) & ~0x0001ffc0) | 0x00001200);
#endif
	DRV_WriteReg32(CLK_MUX_SEL19, (DRV_Reg32(CLK_MUX_SEL19) & ~0xff00ff00) | 0x02000200);
	DRV_WriteReg32(CLK_MUX_SEL21, (DRV_Reg32(CLK_MUX_SEL21) & ~0x00000fff) | 0x00000133);
	DRV_WriteReg32(CLK_MUX_SEL22, (DRV_Reg32(CLK_MUX_SEL22) & ~0x1fffbfff) | 0x06400c40);
	DRV_WriteReg32(CLK_MUX_SEL23, (DRV_Reg32(CLK_MUX_SEL23) & ~0x3fffffff) | 0x02020209);
	DRV_WriteReg32(CLK_MUX_SEL0, (DRV_Reg32(CLK_MUX_SEL0) & ~0x03cc0ff7) | 0x01c00206);
	DRV_WriteReg32(CLK_MUX_SEL1, (DRV_Reg32(CLK_MUX_SEL1) & ~0x007f8007) | 0x00298000);
	DRV_WriteReg32(CLK_MUX_SEL8, (DRV_Reg32(CLK_MUX_SEL8) & ~0x70c001c7) | 0x40c00001);
	DRV_WriteReg32(CLK_SEL_9, (DRV_Reg32(CLK_SEL_9) & ~0x00059000) | 0x00000000);
	DRV_WriteReg32(CLK_MUX_SEL13, (DRV_Reg32(CLK_MUX_SEL13) & ~0x0047039d) | 0x00450104);
	DRV_WriteReg32(CLK_MUX_SEL14, (DRV_Reg32(CLK_MUX_SEL14) & ~0xff03ff07) | 0x9b000104);

#if ALL_CLK_ON
/************
 * TOP CG
 *************/
	DRV_WriteReg32(CLK_GATING_CTRL0_CLR, 0x000f1e09);
	DRV_WriteReg32(CLK_GATING_CTRL1_CLR, 0xfdcfeffe);
	DRV_WriteReg32(CLK_GATING_CTRL7_CLR, 0x8b9abf36);
	DRV_WriteReg32(CLK_GATING_CTRL7_SET, 0x70000000);
	DRV_WriteReg32(CLK_SEL_9, (DRV_Reg32(CLK_SEL_9) & ~0x00000119));
	DRV_WriteReg32(CLK_GATING_CTRL8_CLR, 0x0002f307);
	DRV_WriteReg32(CLK_GATING_CTRL10_SET, 0x07801fff);
	DRV_WriteReg32(CLK_GATING_CTRL10_CLR, 0x08000000);
	DRV_WriteReg32(CLK_GATING_CTRL12_CLR, 0x000000ff);
	DRV_WriteReg32(CLK_GATING_CTRL13_SET, 0x00000001);

/*************
 * for MTCMOS
 *************/
    spm_write(SPM_POWERON_CONFIG_SET, (SPM_PROJECT_CODE << 16) | (1U << 0));

    spm_mtcmos_ctrl_disp(STA_POWER_ON);
    spm_mtcmos_ctrl_audafe(STA_POWER_ON);

    devinfo = seclib_get_devinfo_with_index(3);
    if (!(devinfo & 0x1000))
        spm_mtcmos_ctrl_cm4(STA_POWER_ON);

/*************
 * for Subsys CG
 *************/
    DRV_WriteReg32(MMSYS_CG_CON0, (DRV_Reg32(MMSYS_CG_CON0) & ~0x000c8007));
#endif /* ALL_CLK_ON */

	dprintf(CRITICAL, "mt_pll_init done\n");
}

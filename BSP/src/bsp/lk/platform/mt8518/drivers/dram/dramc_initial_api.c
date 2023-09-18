/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek
 * Software if you have agreed to and been bound by the applicable license
 * agreement with MediaTek ("License Agreement") and been granted explicit
 * permission to do so within the License Agreement ("Permitted User").
 * If you are not a Permitted User, please cease any access or use of MediaTek
 * Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
 * DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE.
 * MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A
 * PARTICULAR STANDARD OR OPEN FORUM.
 * RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 * LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/** @file dramc_initial_api.c
 *  Basic DRAMC API implementation
 */

 /* Include files */
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>

extern unsigned short final_rx_vref_dq[CHANNEL_NUM][RANK_MAX];

void set_dram_type(DRAMC_CTX_T *p)
{
	unsigned char type;

	switch ((unsigned int)p->dram_type) {
	case TYPE_LPDDR4:
		type = 0x4;
		break;
	case TYPE_PCDDR4:
		type = 0x3;
		break;
	case TYPE_LPDDR3:
		type = 0x2;
		break;
	default:
	case TYPE_PCDDR3:
		type = (p->data_width == DATA_WIDTH_16BIT) ? 0x1 : 0x0;
		break;
	}

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DMPINMUX_CTRL),
		type, DMPINMUX_CTRL_DMPINMUX);

	if ((p->dram_type == TYPE_LPDDR4) ||
		((p->dram_type == TYPE_PCDDR3) &&
		(p->data_width == DATA_WIDTH_16BIT))) {
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SREFCTRL),
			0x3, SREFCTRL_DQPINMUX_OPTION_EN);
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_PLL1),
			p_fld(SET_FLD, PLL1_DDR3_2X8_OPEN) |
			p_fld(SET_FLD, PLL1_DDR3_2X8_OPEN_H));
	}
}

/* ++++
 * cc notes,of the following init funtions, LP4 part is
 * from Azalea and others come from 8167.
 * ----
 */

DRAM_STATUS_T ssc_enable(DRAMC_CTX_T *p)
	{
		if (p->ssc_en == ENABLE) {
			show_msg2((INFO, "Enable SSC\n"));

			io_32_write_fld_align_all(DDRPHY_PLL7, 1,
				PLL7_RG_RCLRPLL_SDM_SSC_EN);
		}
		return DRAM_OK;
	}

static void apply_ssc_setting(DRAMC_CTX_T *p,
	unsigned char percent, unsigned char slope, unsigned char dir)
{
	if (p->ssc_en == ENABLE) {
		unsigned int prd, delta, delta1;

		prd = 13000 / slope;
		delta = ((((p->frequency)*2 / 26) *percent) << 18) / (prd *100);
		delta1 = delta;

		io_32_write_fld_align_all(DDRPHY_PLL7,
			prd, PLL7_RG_RCLRPLL_SDM_SSC_PRD);
		io_32_write_fld_align_all(DDRPHY_PLL7,
			dir, PLL7_RG_RCLRPLL_SDM_SSC_PH_INIT); /* ssc direction*/
		io_32_write_fld_align_all(DDRPHY_PLL8,
			delta1, PLL8_RG_RCLRPLL_SDM_SSC_DELTA1);
		io_32_write_fld_align_all(DDRPHY_PLL8,
			delta, PLL8_RG_RCLRPLL_SDM_SSC_DELTA);

		show_msg2((INFO, "[apply_ssc_setting]\n"
			"Percent %d, Slope %d kHz, Dir %d (Down)\n"
			"PRD = (%d), Delta (%d), Delta1 (%d)\n",
			percent, slope, dir,
			prd, delta, delta1));
	}
}

#if SUPPORT_TYPE_LPDDR4
static DRAM_STATUS_T update_initial_setting_lp4(DRAMC_CTX_T *p)
{
	unsigned short rxvrefdefault = 0;
	unsigned int backup_broadcast;

	if (p->odt_onoff == ODT_ON) {
		io_32_write_fld_align_all(DRAMC_REG_SHU_ODTCTRL,
			1, SHU_ODTCTRL_ROEN);
	} else {
		io_32_write_fld_align_all(DRAMC_REG_SHU_ODTCTRL,
			0, SHU_ODTCTRL_ROEN);
	}

	io_32_write_fld_align_all(DDRPHY_CA_CMD3, SET_FLD,
		CA_CMD3_RG_RX_ARCMD_STBENCMP_EN);

	io_32_write_fld_align_all(DDRPHY_CA_CMD3, SET_FLD,
		CA_CMD3_RG_RX_ARCLK_DQSIENMODE);

	io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B0) | /* cc add */
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN) |
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B0) | /* cc add */
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM1_IN_BUFF_EN) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_IN_BUFF_EN));
	io_32_write_fld_multi_all(DDRPHY_CA_CMD3,
		p_fld(SET_FLD, CA_CMD3_RG_RX_ARCMD_IN_BUFF_EN) |
		p_fld(SET_FLD, CA_CMD3_RG_RX_ARCLK_IN_BUFF_EN));

	io_32_write_fld_align_all(DDRPHY_B0_DQ3, CLEAR_FLD,
		B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ3, CLEAR_FLD,
		B1_DQ3_RG_RX_ARDQ_SMT_EN_B1);

	io_32_write_fld_align_all(DDRPHY_B0_DQ5, SET_FLD,
		B0_DQ5_RG_RX_ARDQS0_DVS_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, SET_FLD,
		B1_DQ5_RG_RX_ARDQS0_DVS_EN_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD5, SET_FLD,
		CA_CMD5_RG_RX_ARCLK_DVS_EN);

	/*
	 * LP4 no need, follow LP3 first.
	 * io_32_write_fld_align_all(DDRPHY_MISC_VREF_CTRL,
	 * p_fld(SET_FLD, MISC_VREF_CTRL_RG_RVREF_DDR3_SEL) |
	 * | p_fld(CLEAR_FLD, MISC_VREF_CTRL_RG_RVREF_DDR4_SEL));
	 */

	/*
	 * io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
	 * p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_DDR3_SEL) |
	 * | p_fld(SET_FLD, CA_CMD6_RG_TX_ARCMD_DDR4_SEL)
	 * | p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL)
	 * | p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR4_SEL));
	 */

	io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
		p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_DDR4_SEL));


	io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
		p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0));

	io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
		p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1) |
		p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1));

	io_32_write_fld_multi_all(DDRPHY_MISC_IMP_CTRL0,
		p_fld(SET_FLD, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, SET_FLD,
		B0_DQ6_RG_RX_ARDQ_O1_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, SET_FLD,
		B1_DQ6_RG_RX_ARDQ_O1_SEL_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_O1_SEL);

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, SET_FLD,
		B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, SET_FLD,
		B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_BIAS_PS);

	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN);

	/* cc add */
	io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
		p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_SER_MODE));

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, CLEAR_FLD,
		B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, CLEAR_FLD,
		B1_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, CLEAR_FLD,
		CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS);
	io_32_write_fld_align_all(DDRPHY_B0_DQ2, CLEAR_FLD,
		B0_DQ2_RG_TX_ARDQS0_ODTEN_EXT_DIS);
	io_32_write_fld_align_all(DDRPHY_B1_DQ2, CLEAR_FLD,
		B1_DQ2_RG_TX_ARDQS0_ODTEN_EXT_DIS);

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, SET_FLD,
		B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, SET_FLD,
		B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN);

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, CLEAR_FLD,
		B0_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, CLEAR_FLD,
		B1_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, CLEAR_FLD,
		CA_CMD6_RG_TX_ARCMD_OE_EXT_DIS);

	if (p->dram_type == TYPE_LPDDR4) {
		if (p->odt_onoff == ODT_ON) {
			rxvrefdefault = 0x9;
		} else {
			rxvrefdefault = 0x32;
		}
	}

	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5,
		rxvrefdefault, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5,
		rxvrefdefault, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);

	io_32_write_fld_align_all(DDRPHY_B0_DQ5,
		rxvrefdefault, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5,
		rxvrefdefault, B1_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B1);

	backup_broadcast = get_dramc_broadcast();
	dramc_broadcast_on_off(DRAMC_BROADCAST_OFF);

	io_32_write_fld_multi_all(DDRPHY_B0_DQ8,
		p_fld(CLEAR_FLD, B0_DQ8_RG_TX_ARDQ_EN_LP4P_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ8,
		p_fld(CLEAR_FLD, B1_DQ8_RG_TX_ARDQ_EN_LP4P_B1));

	io_32_write_fld_multi_all(DDRPHY_B0_DQ7,
		p_fld(SET_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RB_EXT_EN_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RST_EXT_EN_B01) |
		p_fld(SET_FLD, B0_DQ7_RG_RX_ARDQS0_LP4_BURSTMODE_SEL_B01) | /* cc add */
		p_fld(0x2, B0_DQ7_RG_RX_ARDQS0_BURST_EN_B01) | /* cc add */
		p_fld(CLEAR_FLD, B0_DQ7_RG_ARDQ_READ_BASE_EN_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_GATE_EN_MODE_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_DQSIEN_RB_DLY_B01));

	dramc_broadcast_on_off(backup_broadcast);

	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ7,
		SET_FLD, SHU1_B0_DQ7_RG_ARDQ_REV_B0_2);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ7,
		SET_FLD, SHU1_B1_DQ7_RG_ARDQ_REV_B1_2);
	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD7,
		0x0, SHU1_CA_CMD7_RG_ARCMD_REV);

	dramc_broadcast_on_off(DRAMC_BROADCAST_ON);
	io_32_write_fld_align_all(DDRPHY_CA_CMD8,
		SET_FLD, CA_CMD8_RG_TX_RRESETB_DDR3_SEL);
	io_32_write_fld_align_all(DDRPHY_CA_CMD8,
		CLEAR_FLD, CA_CMD8_RG_TX_RRESETB_DDR4_SEL);

	dramc_broadcast_on_off(DRAMC_BROADCAST_ON);

	io_32_write_fld_align_all(DRAMC_REG_SHU_MISC,
		0x2, SHU_MISC_REQQUE_MAXCNT);


	io_32_write_fld_align_all(DRAMC_REG_SHU_SCINTV,
		0x1f, SHU_SCINTV_SCINTV);

	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5,
		CLEAR_FLD, SHU1_B0_DQ5_RG_ARPI_FB_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5,
		CLEAR_FLD, SHU1_B1_DQ5_RG_ARPI_FB_B1);

	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD5,
		CLEAR_FLD, SHU1_CA_CMD5_RG_ARPI_FB_CA);

#if 0
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI1,
		CLEAR_FLD,
		B0_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI1,
		CLEAR_FLD,
		B1_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B1);
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI1,
		CLEAR_FLD,
		CA_DLL_ARPI1_RG_ARPI_OFFSET_CLKIEN);
#endif


	/*
	 * IMP Tracking Init Settings
	 * Write (DRAMC _BASE+ 0x219) [31:0] = 32'h80080020//DDR3200 default
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_IMPCAL1,
		p_fld(8, SHU_IMPCAL1_IMPCAL_CALICNT) |
		p_fld(0x10, SHU_IMPCAL1_IMPCALCNT) |
		p_fld(4, SHU_IMPCAL1_IMPCAL_CALEN_CYCLE) |
		p_fld(4, SHU_IMPCAL1_IMPCAL_CHKCYCLE));

#if SUPPORT_HYNIX_RX_DQS_WEAK_PULL
#if 1
	if (p->vendor_id == VENDOR_HYNIX) {
		io_32_write_fld_multi_all(DDRPHY_B0_DQ7,
			p_fld(CLEAR_FLD, B0_DQ7_RG_TX_ARDQS0_PULL_UP_B0) |
			p_fld(SET_FLD, B0_DQ7_RG_TX_ARDQS0_PULL_DN_B0) |
			p_fld(SET_FLD, B0_DQ7_RG_TX_ARDQS0B_PULL_UP_B0) |
			p_fld(CLEAR_FLD, B0_DQ7_RG_TX_ARDQS0B_PULL_DN_B0));

		io_32_write_fld_multi_all(DDRPHY_B1_DQ7,
			p_fld(CLEAR_FLD, B1_DQ7_RG_TX_ARDQS0_PULL_UP_B1) |
			p_fld(SET_FLD, B1_DQ7_RG_TX_ARDQS0_PULL_DN_B1) |
			p_fld(SET_FLD, B1_DQ7_RG_TX_ARDQS0B_PULL_UP_B1) |
			p_fld(CLEAR_FLD, B1_DQ7_RG_TX_ARDQS0B_PULL_DN_B1));
	}
#endif
#endif


	io_32_write_fld_align_all(DRAMC_REG_PRE_TDQSCK1,
		SET_FLD, PRE_TDQSCK1_TXUIPI_CAL_CGAR);

	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ7_R_DMRXDVS_DQM_FLAGSEL_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ7_R_DMRXDVS_DQM_FLAGSEL_B1));

	io_32_write_fld_align_all(DRAMC_REG_DUMMY_RD,
		SET_FLD, DUMMY_RD_DUMMY_RD_PA_OPT);

	/* cc add */
	io_32_write_fld_align_all(DRAMC_REG_STBCAL1,
		SET_FLD, STBCAL1_STB_PIDLYCG_IG);

	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_EYESCAN_DQS_SYNC_EN) |
		p_fld(SET_FLD, EYESCAN_EYESCAN_NEW_DQ_SYNC_EN) |
		p_fld(SET_FLD, EYESCAN_EYESCAN_DQ_SYNC_EN));

	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_REORDEREN) |
		p_fld(CLEAR_FLD, PERFCTL0_RWSPLIT));

	io_32_write_fld_align_all(DRAMC_REG_SREFCTRL,
		CLEAR_FLD, SREFCTRL_SREF2_OPTION);
	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL1,
		0x4d, SHUCTRL1_FC_PRDCNT);
	io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
		p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
		p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B1) |
		p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_EN_B1));
	io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
		p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_OP_BIAS_SW_EN) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_BIAS_EN));

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DQSG), \
		p_fld(9, SHU1_DQSG_STB_UPDMASKCYC) |
		p_fld(SET_FLD, SHU1_DQSG_STB_UPDMASK_EN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSCAL,
		p_fld(CLEAR_FLD, SHURK0_DQSCAL_R0DQSIENLLMTEN) |
		p_fld(CLEAR_FLD, SHURK0_DQSCAL_R0DQSIENHLMTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_DQSCAL,
		p_fld(CLEAR_FLD, SHURK1_DQSCAL_R1DQSIENLLMTEN) |
		p_fld(CLEAR_FLD, SHURK1_DQSCAL_R1DQSIENHLMTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(SET_FLD, SHU_RANKCTL_DQSG_MODE) |
		p_fld(SET_FLD, SHU_RANKCTL_PICGLAT));


	io_32_write_fld_align_all(DDRPHY_MISC_RXDVS1,
		0x7,
		MISC_RXDVS1_R_IN_GATE_EN_LOW_OPT);

	return DRAM_OK;
}

static void dramc_setting_ddr2667(DRAMC_CTX_T *p)
{
	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF2, 0x54,
		SHU_CONF2_FSPCHG_PRDCNT);
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x4, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(0x2, SHU_RANKCTL_RANKINCTL_ROOT1) |
		p_fld(0x2, SHU_RANKCTL_RANKINCTL));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CKECTRL,
		p_fld(0x3, SHU_CKECTRL_TCKESRX) |
		p_fld(0x3, SHU_CKECTRL_CKEPRD));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE2) |
		p_fld(0x4, SHU_ODTCTRL_RODT) |
		p_fld(SET_FLD, SHU_ODTCTRL_ROEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DQSOSC_PRD,
		p_fld(0xc, SHU1_DQSOSC_PRD_DQSOSCTHRD_DEC) |
		p_fld(0xc, SHU1_DQSOSC_PRD_DQSOSCTHRD_INC) |
		p_fld(0x10, SHU1_DQSOSC_PRD_DQSOSC_PRDCNT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS0));

	/*
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_MR2, 0x24, SHU_HWSET_MR2_HWSET_MR2_OP);
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_MR13, 0xc8, SHU_HWSET_MR13_HWSET_MR13_OP);
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_VRCG, 0xc0, SHU_HWSET_VRCG_HWSET_VRCG_OP);
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
		p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN1DLY) |
		p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN0DLY));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_PI,
		p_fld(0x14, SHU1RK0_PI_RK0_ARPI_DQM_B1) |
		p_fld(0x14, SHU1RK0_PI_RK0_ARPI_DQM_B0) |
		p_fld(0x14, SHU1RK0_PI_RK0_ARPI_DQ_B1) |
		p_fld(0x14, SHU1RK0_PI_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_DQSOSC,
		p_fld(0x1ae, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0_B1) |
		p_fld(0x1ae, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0));

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM0));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
			p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
			p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
			p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
			p_fld(SET_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
			p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM2));
	}
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_DQM0));

	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5, 0x3,
		SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
	/*
	 * io_32_write_fld_multi(DDRPHY_SHU1_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0) |
	 * | p_fld(SET_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0));
	 */
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1)
	{
	#ifdef WHITNEY_USE
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
			p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
	#else
		/*
		 * TODO, double confirm,zj
		 * io_32_write_fld_multi(DDRPHY_SHU1_B0_DQ7,
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
		 * | p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
		 */
	#endif
	}
	else
	{
	#ifdef WHITNEY_USE
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(SET_FLD, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0) |
			p_fld(SET_FLD, SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0) |
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
			p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
	#else
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(SET_FLD, SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0));
	#endif
	}
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5, 0x3,
		SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);
	/*
	 * io_32_write_fld_multi(DDRPHY_SHU1_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1) |
	 * | p_fld(SET_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1));
	 */
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1)
	{
	}
	else
	{
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ7,
			p_fld(SET_FLD, SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1));
	}
	/*
	 * dramc_broadcast_on_off(DRAMC_BROADCAST_OFF);
	 * io_32_write_fld_multi(DDRPHY_SHU1_CA_CMD6,
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA) |
	 * | p_fld(SET_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA));
	 * io_32_write_fld_multi(DDRPHY_SHU1_CA_CMD6+(1<<POS_BANK_NUM),
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA) |
	 * | p_fld(SET_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA));
	 * dramc_broadcast_on_off(DRAMC_BROADCAST_ON);
	 * io_32_write_fld_align(DDRPHY_SHU1_PLL5, 0x3300, SHU1_PLL5_RG_RPHYPLL_SDM_PCW);
	 * io_32_write_fld_align(DDRPHY_SHU1_PLL7, 0x3300, SHU1_PLL7_RG_RCLRPLL_SDM_PCW);
	 */
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(0x12, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(0x12, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(0x12, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(0x12, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(0x1d, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(0x15, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(0x14, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0x14, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(0x12, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(0x12, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(0x12, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(0x12, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(0x1d, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(0x15, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(0x14, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0x14, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
}

static void dramc_setting_ddr1600(DRAMC_CTX_T *p)
{
	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF2, 0x32,
		SHU_CONF2_FSPCHG_PRDCNT);
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x2, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_RANKINCTL_ROOT1) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_RANKINCTL));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CKECTRL,
		p_fld(CLEAR_FLD, SHU_CKECTRL_TCKESRX) |
		p_fld(0x2, SHU_CKECTRL_CKEPRD));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE2) |
		p_fld(0x2, SHU_ODTCTRL_RODT) |
#ifdef LOOPBACK_TEST
		p_fld(SET_FLD, SHU_ODTCTRL_ROEN));
#else
		p_fld(SET_FLD, SHU_ODTCTRL_ROEN));
#endif

	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DQSOSC_PRD,
		p_fld(0x14, SHU1_DQSOSC_PRD_DQSOSCTHRD_DEC) |
		p_fld(0x14, SHU1_DQSOSC_PRD_DQSOSCTHRD_INC) |
		p_fld(0xf, SHU1_DQSOSC_PRD_DQSOSC_PRDCNT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(SET_FLD, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(SET_FLD, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(SET_FLD, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(SET_FLD, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(0x6, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(SET_FLD, SHU_SELPH_DQS1_DLY_DQS0));

	/*
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_MR2, 0x12, SHU_HWSET_MR2_HWSET_MR2_OP);
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_MR13, 0x8, SHU_HWSET_MR13_HWSET_MR13_OP);
	 * io_32_write_fld_align(DRAMC_REG_SHU_HWSET_VRCG, CLEAR_FLD, SHU_HWSET_VRCG_HWSET_VRCG_OP);
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
		p_fld(CLEAR_FLD, SHURK0_DQSIEN_R0DQSIEN1DLY) |
		p_fld(CLEAR_FLD, SHURK0_DQSIEN_R0DQSIEN0DLY));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_PI,
		p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQM_B1) |
		p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQM_B0) |
		p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQ_B1) |
		p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_DQSOSC,
		p_fld(0x2d0, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0_B1) |
		p_fld(0x2d0, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
		p_fld(0x5, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld(0x5, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld(0x5, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM0));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
			p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
			p_fld(SET_FLD, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM0));
	}
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(0x7, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(0x7, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(0x7, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(0x7, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ2_DLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(0x7, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(0x7, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(0x7, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(0x7, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQ3_DLY_DQM0));

	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5, 0x4,
		SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
	/*
	 * io_32_write_fld_multi(DDRPHY_SHU1_B0_DQ6,
		p_fld(SET_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0) |
	 * | p_fld(CLEAR_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0));
	 */
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1)
	{
	#ifdef WHITNEY_USE
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
	#else
		/*
		 * TODO, double confirm,zj
		 * io_32_write_fld_multi(DDRPHY_SHU1_B0_DQ7,
			p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
		 * | p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
		 */
	#endif
	}
	else
	{
	#ifdef WHITNEY_USE
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0) |
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0) |
			p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_MIDPI_ENABLE));
	#else

		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
			p_fld(CLEAR_FLD, SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0));
	#endif
	}
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5, 0x4,
		SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);
	/*
	 * io_32_write_fld_multi(DDRPHY_SHU1_B1_DQ6,
		p_fld(SET_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1) |
	 * | p_fld(CLEAR_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1));
	 */
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1)
	{}
	else
	{
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ7,
			p_fld(CLEAR_FLD, SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1));
	}
	/*
	 * dramc_broadcast_on_off(DRAMC_BROADCAST_OFF);
	 * io_32_write_fld_multi(DDRPHY_SHU1_CA_CMD6,
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA) |
	 * | p_fld(SET_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA));
	 * io_32_write_fld_multi(DDRPHY_SHU1_CA_CMD6+(1<<POS_BANK_NUM),
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA) |
	 * | p_fld(SET_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA));
	 * dramc_broadcast_on_off(DRAMC_BROADCAST_ON);
	 * io_32_write_fld_align(DDRPHY_SHU1_PLL5, 0x3d00, SHU1_PLL5_RG_RPHYPLL_SDM_PCW);
	 * io_32_write_fld_align(DDRPHY_SHU1_PLL7, 0x3d00, SHU1_PLL7_RG_RCLRPLL_SDM_PCW);
	 */
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(0xc, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(0xc, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(0xc, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(0xc, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(0xc, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(0xc, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(0xc, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(0xc, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(0x1d, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(0x15, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(0xc, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(0x4, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(0xc, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(0xc, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(0xc, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(0xc, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(0xc, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(0xc, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(0xc, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(0xc, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(0x1d, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(0x15, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(0xc, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(0x4, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
}

#if DVFS_EN
void ddrphy_pll_switch(DRAMC_CTX_T *p)
{
	unsigned int sdm_pcw;

	if (p->frequency >= DDR1600_FREQ) {
			io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL0,
				p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_B01) |
				p_fld(0x7, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_B01) |
				p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_B01) |
				p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_DELAY_CODE_B01) |
				p_fld(0x8, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_B01));
			io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL0,
				p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_CA) |
				p_fld(0x7, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_CA) |
				p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_CA) |
				p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_DELAY_CODE_CA) |
				p_fld(0x8, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_CA));

			/* cc add */
			io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
				p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
				p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
				p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
				p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
				p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
				p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
			io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
				p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
				p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
				p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
				p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
				p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
				p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
		} else {
			/* DIV4 mode fore low freq (i.e, 1066M data rate) */
			io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL0,
				p_fld(0x20, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_B01) |
				p_fld(0x3, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_B01) |
				p_fld(0x8, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_B01) |
				p_fld(0x7, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_B01));
			io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
				p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
				p_fld(0xf, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
				p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
				p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
				p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
				p_fld(SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_B01) |
				p_fld(CLEAR_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
			io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL0,
				p_fld(0x20, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_CA) |
				p_fld(0x3, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_CA) |
				p_fld(0x8, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_CA) |
				p_fld(0x7, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_CA));
			io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
				p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
				p_fld(0xf, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
				p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
				p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
				p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
				p_fld(SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_CA) |
				p_fld(CLEAR_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
		}


	switch (p->freq_sel) {
	default:
	case DDR_DDR1066:
		sdm_pcw = 0x51000000;
		break;
	case DDR_DDR1600:
		sdm_pcw = 0x3d000000;
		break;
	case DDR_DDR1866:
		sdm_pcw = 0x47000000;
		break;
	case DDR_DDR2400:
		sdm_pcw = 0x5c000000;
		break;
	}

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_PLL7),
		sdm_pcw, SHU1_PLL7_RG_RCLRPLL_SDM_PCW);

	/* 1. PLL enable */
	io_32_write_fld_align_all(DDRPHY_PLL2, CLEAR_FLD, PLL2_RG_RCLRPLL_EN); //re-set
	io_32_write_fld_align_all(DDRPHY_PLL2, SET_FLD, PLL2_RG_RCLRPLL_EN);

	delay_us(31);

	if (p->frequency >= DDR1600_FREQ) {
		/* 4. DCDL enable */
		io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
			SET_FLD, CA_CKGEN_DLL1_RG_CKGEN_DCDL_EN_CA);
		io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
			SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DCDL_EN_B01);

		/*5. DLL enable. AUTO mode */
		io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
			SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_B01);
		io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
			SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_CA);

		delay_us(1);
	}

}

void dramc_dump_cal_settings(DRAMC_CTX_T *p)
{
	unsigned int reg_idx;

	for (reg_idx = 0; reg_idx < DVFS_LP4_ADDR_NUM; reg_idx++)
		show_msg2((INFO, "ch%d, reg_idx %d :0x%x\n", p->channel, reg_idx, io32_read_4b(dvfs_lp4_cal_setting_addr[reg_idx])));

}

void dramc_save_cal_settings(DRAMC_CTX_T *p)
{
	unsigned int reg_idx;

	if (p->freq_sel == DVFS_HIGH_FREQ) {
		for (reg_idx = 0; reg_idx < DVFS_LP4_ADDR_NUM; reg_idx++) {
			dvfs_lp4_high_freq_cal_setting[reg_idx] = io32_read_4b(dvfs_lp4_cal_setting_addr[reg_idx]);
			//show_msg2((INFO, "save ch%d H setting: reg_idx %d :0x%x\n", p->channel, reg_idx, dvfs_lp4_high_freq_cal_setting[reg_idx]));
		}
	} else if (p->freq_sel == DVFS_LOW_FREQ) {
		for (reg_idx = 0; reg_idx < DVFS_LP4_ADDR_NUM; reg_idx++) {
			dvfs_lp4_low_freq_cal_setting[reg_idx] = io32_read_4b(dvfs_lp4_cal_setting_addr[reg_idx]);
			//show_msg2((INFO, "save ch%d L setting: reg_idx %d :0x%x\n", p->channel, reg_idx, dvfs_lp4_low_freq_cal_setting[reg_idx]));
		}
	} else
		show_msg2((INFO, "%s: Wrong dvfs freq_sel!!\n", __func__));
}

void dramc_restore_cal_settings(DRAMC_CTX_T *p)
{
	unsigned int reg_idx;

	if (p->freq_sel == DVFS_HIGH_FREQ) {
		for (reg_idx = 0; reg_idx < DVFS_LP4_ADDR_NUM; reg_idx++)
			io32_write_4b(dvfs_lp4_cal_setting_addr[reg_idx], dvfs_lp4_high_freq_cal_setting[reg_idx]);
	} else if (p->freq_sel == DVFS_LOW_FREQ) {
		for (reg_idx = 0; reg_idx < DVFS_LP4_ADDR_NUM; reg_idx++)
			io32_write_4b(dvfs_lp4_cal_setting_addr[reg_idx], dvfs_lp4_low_freq_cal_setting[reg_idx]);
	} else
		show_msg2((INFO, "%s: Wrong dvfs freq_sel!!\n", __func__));

	//dramc_dump_cal_settings(p);
}

void dramc_dvfs_switch(DRAMC_CTX_T *p, unsigned int freq_sel)
{
	ddr_phy_freq_sel(p, freq_sel);
	ddrphy_pll_switch(p);
	dramc_restore_cal_settings(p);
	dram_phy_reset(p);

	delay_us(2);

	if (p->frequency == DDR2400_FREQ) {
		dramc_mode_reg_write(p, MR13, MR13_FSP1_INIT);
	} else if (p->frequency <= DDR1600_FREQ) {
		dramc_mode_reg_write(p, MR13, MR13_FSP0_INIT);
	}

	delay_us(2);
}


void dramc_dvfs_init(DRAMC_CTX_T *p)
{
	ddrphy_pll_switch(p);

	if (p->frequency == DDR2400_FREQ) {
		dramc_setting_ddr2667(p);
	} else if (p->frequency <= DDR1600_FREQ) {
		dramc_setting_ddr1600(p);
	}

	ddr_update_ac_timing(p);

	if (p->frequency == DDR2400_FREQ) {
		dramc_mode_reg_write(p, MR13, MR13_FSP1_INIT);
	} else if (p->frequency <= DDR1600_FREQ) {
		dramc_mode_reg_write(p, MR13, MR13_FSP0_INIT);
	}

}
#endif

void dramc_setting_lp4(DRAMC_CTX_T *p)
{
	unsigned int sdm_pcw, prediv, posdiv;

	delay_us(1);

	dramc_broadcast_on_off(DRAMC_BROADCAST_ON);
	io_set_sw_broadcast(TRUE);

	if (p->frequency >= DDR1600_FREQ) {
	io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL0,
		p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_B01) |
		p_fld(0x7, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_B01) |
		p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_B01) |
		p_fld(0x0, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_DELAY_CODE_B01) |
		p_fld(0x8, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_B01));
	show_msg2((INFO, "DDRPHY_B0_CKGEN_DLL0 = 0x%x\n",
		io32_read_4b(DDRPHY_B0_CKGEN_DLL0)));
	io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL0,
		p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_CA) |
		p_fld(0x7, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_CA) |
		p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_CA) |
		p_fld(0x0, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_DELAY_CODE_CA) |
		p_fld(0x8, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_CA));

	/* cc add */
	io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
		p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
		p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
		p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
		p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
		p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
		p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
	io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
		p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
		p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
		p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
		p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
		p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
		p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
	} else {
		/* DIV4 mode fore low freq (i.e, 1066M data rate) */
		io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL0,
			p_fld(0x20, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_B01) |
			p_fld(0x3, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_B01) |
			p_fld(0x8, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_B01) |
			p_fld(0x7, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_B01));
		io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
			p_fld(0xf, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
			p_fld(SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_B01) |
			p_fld(CLEAR_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
		io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL0,
			p_fld(0x20, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_CA) |
			p_fld(0x3, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_CA) |
			p_fld(0x8, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_CA) |
			p_fld(0x7, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_CA));
		io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
			p_fld(0xf, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
			p_fld(SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_CA) |
			p_fld(CLEAR_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
	}

	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL1, CLEAR_FLD,
			MISC_SPM_CTRL1_PHY_SPM_CTL1);
	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL0, 0xffffffff,
		MISC_SPM_CTRL0_PHY_SPM_CTL0);
	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL2, 0xffffffff,
		MISC_SPM_CTRL2_PHY_SPM_CTL2);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL2, 0x6003bf,
		MISC_CG_CTRL2_RG_MEM_DCM_CTL);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL4, 0x13300000,
		MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL1, CLEAR_FLD,
		SHU1_PLL1_RG_RPHYPLLGP_CK_SEL);
	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD7, SET_FLD,
		SHU1_CA_CMD7_R_DMRANKRXDVS_CA);
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
		p_fld(SET_FLD, SHU1_B0_DQ7_R_DMRANKRXDVS_B0));
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ7, SET_FLD,
		SHU1_B1_DQ7_R_DMRANKRXDVS_B1);
	io_32_write_fld_align_all(DDRPHY_MISC_CTRL1, SET_FLD,
		MISC_CTRL1_R_DMDQSIENCG_EN);

	io_32_write_fld_multi_all(DDRPHY_CA_CMD2,
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCMD_OE_DIS) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCMD_ODTEN_DIS) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_OE_DIS));

	io_32_write_fld_multi_all(DDRPHY_B0_DQ2,
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_OE_DIS_B0) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_ODTEN_DIS_B0) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_OE_DIS) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_ODTEN_DIS));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ2,
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQ_OE_DIS_B1) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQ_ODTEN_DIS_B1) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS1_OE_DIS) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS1_ODTEN_DIS));
	io_32_write_fld_align_all(DDRPHY_MISC_RXDVS1,
		0x7, MISC_RXDVS1_R_IN_GATE_EN_LOW_OPT);
	io_32_write_fld_multi_all(DDRPHY_MISC_VREF_CTRL,
		p_fld(CLEAR_FLD, MISC_VREF_CTRL_RG_RVREF_VREF_EN) |
		p_fld(CLEAR_FLD, MISC_VREF_CTRL_RG_RVREF_SEL_CMD) |
		p_fld(SET_FLD, MISC_VREF_CTRL_RG_RVREF_DDR4_SEL) |
		p_fld(CLEAR_FLD, MISC_VREF_CTRL_RG_RVREF_SEL_DQ));
	io_32_write_fld_align_all(DDRPHY_MISC_IMP_CTRL0, SET_FLD,
		MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL);
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5, 0x5,
		SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5, 0x5,
		SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5, 0x0,
		SHU1_B0_DQ5_RG_RX_ARDQS0_DQSIEN_DLY_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5, 0x0,
		SHU1_B1_DQ5_RG_RX_ARDQS0_DQSIEN_DLY_B1);

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(0x3f, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(0x3f, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(0x3f, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(0x3f, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(0x3f, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(0x3f, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(0x8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(0x8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(0x8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(0x8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(0x8, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ2,
		p_fld(0x9, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_R_DLY_B0) |
		p_fld(0x9, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ3,
		p_fld(0x9, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_R_DLY_B0) |
		p_fld(0x9, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ4,
		p_fld(0x9, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_R_DLY_B0) |
		p_fld(0x9, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ5,
		p_fld(0x9, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_R_DLY_B0) |
		p_fld(0x9, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ6,
		p_fld(0x9, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_R_DLY_B0) |
		p_fld(0x9, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_F_DLY_B0) |
		p_fld(SET_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(0x8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(0x8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(0x8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(0x8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(0x8, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ2,
		p_fld(0x9, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_R_DLY_B1) |
		p_fld(0x9, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ3,
		p_fld(0x9, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_R_DLY_B1) |
		p_fld(0x9, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ4,
		p_fld(0x9, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_R_DLY_B1) |
		p_fld(0x9, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ5,
		p_fld(0x9, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_R_DLY_B1) |
		p_fld(0x9, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ6,
		p_fld(0x9, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_R_DLY_B1) |
		p_fld(0x9, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_F_DLY_B1) |
		p_fld(SET_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_align_all(DDRPHY_B0_DQ3, SET_FLD,
		B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ3, SET_FLD,
		B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL1, 0x3f600,
		MISC_CG_CTRL1_R_DVS_DIV4_CG_CTRL);

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ7,
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ7,
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ4,
		p_fld(0x00, B0_DQ4_RG_RX_ARDQS_EYE_R_DLY_B0) |
		p_fld(0x00, B0_DQ4_RG_RX_ARDQS_EYE_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ5,
		p_fld(CLEAR_FLD, B0_DQ5_RG_RX_ARDQ_EYE_EN_B0) |
		p_fld(SET_FLD, B0_DQ5_RG_RX_ARDQ_EYE_SEL_B0) |
		p_fld(0x9, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_DDR4_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_DDR3_SEL_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0));
	io_32_write_fld_align_all(DDRPHY_B0_DQ5, CLEAR_FLD,
		B0_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B0);
	io_32_write_fld_multi_all(DDRPHY_B1_DQ4,
		p_fld(CLEAR_FLD, B1_DQ4_RG_RX_ARDQS_EYE_R_DLY_B1) |
		p_fld(CLEAR_FLD, B1_DQ4_RG_RX_ARDQS_EYE_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ5,
		p_fld(CLEAR_FLD, B1_DQ5_RG_RX_ARDQ_EYE_EN_B1) |
		p_fld(SET_FLD, B1_DQ5_RG_RX_ARDQ_EYE_SEL_B1) |
		p_fld(0x9, B1_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B1));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B1) |
		p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_DDR4_SEL_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1) |
		p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_SER_MODE_B1));

	io_32_write_fld_align_all(DDRPHY_B0_DQ9, 0x4,
		B0_DQ9_RG_RX_ARDQ_RESERVE_B01);

	io_32_write_fld_align_all(DDRPHY_B1_DQ5, CLEAR_FLD,
		B1_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B1);
	io_32_write_fld_multi_all(DDRPHY_CA_CMD3,
		p_fld(SET_FLD, CA_CMD3_RG_RX_ARCMD_IN_BUFF_EN) |
		p_fld(SET_FLD, CA_CMD3_RG_RX_ARCLK_IN_BUFF_EN) |
		p_fld(SET_FLD, CA_CMD3_RG_ARCMD_RESETB) |
		p_fld(SET_FLD, CA_CMD3_RG_TX_ARCMD_EN));

	io_32_write_fld_align_all(DDRPHY_PLL3, CLEAR_FLD,
		PLL3_RG_RPHYPLL_TSTOP_EN);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL0, SET_FLD,
		SHU1_PLL0_RG_RPHYPLL_TOP_REV);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL8, CLEAR_FLD,
		SHU1_PLL8_RG_RPHYPLL_POSDIV);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL4, CLEAR_FLD,
		SHU1_PLL4_RG_RPHYPLL_SDM_FRA_EN);

	if (p->ssc_en == ENABLE) {
		io_32_write_fld_align_all(DDRPHY_SHU1_PLL6,
			SET_FLD, SHU1_PLL6_RG_RCLRPLL_SDM_FRA_EN);
	} else {
		io_32_write_fld_align_all(DDRPHY_SHU1_PLL6,
			CLEAR_FLD, SHU1_PLL6_RG_RCLRPLL_SDM_FRA_EN);
	}

	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI2,
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI2,
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCK_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCTL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_FB_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQS_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQM_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQ_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_MPDIV_CG_B1));
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI2,
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCK_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCTL_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARRPI_CG_FB_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CS) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CLK) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CMD) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN) |
		p_fld(SET_FLD, CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA));
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI1,
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA));

	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI1, 0x5,
		CA_DLL_ARPI1_RG_ARPI_OFFSET_CLKIEN);

	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI1,
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0) |
		p_fld(0x0, B0_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI1,
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPISM_MCK_SEL_B1) |
		p_fld(0x0, B1_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B1));

	io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_ARDQ_RESETB_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_TX_ARDQ_EN_B0) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBENCMP_EN_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_ARDQ_RESETB_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_TX_ARDQ_EN_B1) |
		p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1));

	if (p->channel == CHANNEL_B) {
		/* B0 RX PINMUX*/
		io_32_write_fld_multi(DDRPHY_B0_DQ3,
			p_fld(0x00, B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
			p_fld(0x0, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0));

		/* B1 */
		io_32_write_fld_multi(DDRPHY_B1_DQ3,
			p_fld(0xff, B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1));

		/* B23 */
		io_32_write_fld_multi(DDRPHY_B0_DQ3 + (1 << POS_BANK_NUM),
			p_fld(0x00, B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
			p_fld(0x0, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0));
		io_32_write_fld_multi(DDRPHY_B1_DQ3 + (1 << POS_BANK_NUM),
			p_fld(0xff, B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1));

		/* B0 TX PINMUX */
		io_32_write_fld_multi(DDRPHY_B0_DQ2,
			p_fld(0x00, B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0) |
			p_fld(0x0, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0));
		/* B1 */
		io_32_write_fld_multi(DDRPHY_B1_DQ2,
			p_fld(0xff, B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1) |
			p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1));

		/* B23 */
		io_32_write_fld_multi(DDRPHY_B0_DQ2 + (1 << POS_BANK_NUM),
			p_fld(0x00, B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0) |
			p_fld(0x0, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0));
		io_32_write_fld_multi(DDRPHY_B1_DQ2 + (1 << POS_BANK_NUM),
			p_fld(0xff, B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1) |
			p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1));
	}

	io_32_write_fld_multi_all(DDRPHY_CA_CMD8,
		p_fld(SET_FLD, CA_CMD8_RG_TX_RRESETB_DDR4_SEL) |
		p_fld(SET_FLD, CA_CMD8_RG_RRESETB_DRVN) |
		p_fld(SET_FLD, CA_CMD8_RG_RRESETB_DRVP));
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL4,
		0x11400000, MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);
	io_32_write_fld_multi_all(DDRPHY_MISC_SHU_OPT,
		p_fld(CLEAR_FLD, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN) |
		p_fld(SET_FLD, MISC_SHU_OPT_R_CA_SHU_PHY_GATING_RESETB_SPM_EN) |
		p_fld(CLEAR_FLD, MISC_SHU_OPT_R_DQB1_SHU_PHDET_SPM_EN) |
		p_fld(SET_FLD, MISC_SHU_OPT_R_DQB1_SHU_PHY_GATING_RESETB_SPM_EN) |
		p_fld(CLEAR_FLD, MISC_SHU_OPT_R_DQB0_SHU_PHDET_SPM_EN) |
		p_fld(SET_FLD, MISC_SHU_OPT_R_DQB0_SHU_PHY_GATING_RESETB_SPM_EN));

	io_32_write_fld_align_all(DDRPHY_PLL4, CLEAR_FLD,
		PLL4_RG_RPHYPLL_MCK8X_EN);
	prediv = 0x0;
	posdiv = 0x0;
	//extfbdiv = 0x1; /* cc notes: not used??? */

	switch (p->freq_sel) {
	default:
	case DDR_DDR1066:
		sdm_pcw = 0x51000000;
		break;
	case DDR_DDR1600:
		sdm_pcw = 0x3d000000;
		break;
	case DDR_DDR1866:
		sdm_pcw = 0x47000000;
		break;
	case DDR_DDR2400:
		sdm_pcw = 0x5c000000;
		break;
	}

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_PLL10),
		p_fld(prediv, SHU1_PLL10_RG_RCLRPLL_PREDIV) |
		p_fld(posdiv, SHU1_PLL10_RG_RCLRPLL_POSDIV));
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_PLL7),
		sdm_pcw, SHU1_PLL7_RG_RCLRPLL_SDM_PCW);

	delay_us(1);

	/* cc add PI general setting */
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI1,
		p_fld(CLEAR_FLD, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CS_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CLK_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CMD_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CLKIEN_JUMP_EN));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI1,
		p_fld(CLEAR_FLD, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQS_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQM_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQ_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQSIEN_JUMP_EN_B0) |
		p_fld(0x0, B0_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI1,
		p_fld(CLEAR_FLD, B1_DLL_ARPI1_RG_ARPISM_MCK_SEL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQS_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQM_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQ_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQSIEN_JUMP_EN_B1) |
		p_fld(0x0, B1_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B1));
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI0,
		p_fld(SET_FLD, CA_DLL_ARPI0_RG_ARPI_BYPASS_SR_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_8PHASE_XLATCH_FORCE_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_BUFGP_XLATCH_FORCE_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_BUFGP_XLATCH_FORCE_CMD) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_PSMUX_XLATCH_FORCE_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_PSMUX_XLATCH_FORCE_CMD) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_SMT_XLATCH_FORCE_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_SMT_XLATCH_FORCE_CMD) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_PSMUX_DRV_SEL_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI0_RG_ARPI_BYPASS_SR_CMD));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI0,
		p_fld(SET_FLD, B0_DLL_ARPI0_RG_ARPI_BYPASS_SR_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_8PHASE_XLATCH_FORCE_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_BUFGP_XLATCH_FORCE_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_BUFGP_XLATCH_FORCE_DQS_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_PSMUX_XLATCH_FORCE_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_PSMUX_XLATCH_FORCE_DQS_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_SMT_XLATCH_FORCE_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_PSMUX_DRV_SEL_B01) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_SMT_XLATCH_FORCE_DQS_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI0_RG_ARPI_BYPASS_SR_DQS_B0));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI0,
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARPI_BUFGP_XLATCH_FORCE_DQS_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARPI_PSMUX_XLATCH_FORCE_DQS_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARPI_SMT_XLATCH_FORCE_DQS_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI0_RG_ARPI_BYPASS_SR_DQS_B1));
	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD6, SET_FLD,
		SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA);
	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD6, 0x0,
		SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
	io_32_write_fld_align_all(DDRPHY_SHU1_CA_CMD6, 0x48,
		SHU1_CA_CMD6_RG_ARPI_CAP_SEL_CA);
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ6, SET_FLD,
		SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ6, SET_FLD,
		SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1);
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ6, 0x48,
		SHU1_B0_DQ6_RG_ARPI_CAP_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ6, 0x48,
		SHU1_B1_DQ6_RG_ARPI_CAP_SEL_B1);

	/* cc move CKGEN DLL/DLL general setting here */
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI5,
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_CA) |
		p_fld(0x6, CA_DLL_ARPI5_RG_ARDLL_GAIN_CA) |
		p_fld(0x8, CA_DLL_ARPI5_RG_ARDLL_P_GAIN_CA) |
		p_fld(0x9, CA_DLL_ARPI5_RG_ARDLL_IDLECNT_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDIV_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_DIV_DEC_CA) |
		p_fld(0x2, CA_DLL_ARPI5_RG_ARDLL_DIV_MCTL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_MON_SEL_CA));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI5,
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_B0) |
		p_fld(0x7, B0_DLL_ARPI5_RG_ARDLL_GAIN_B0) |
		p_fld(0x8, B0_DLL_ARPI5_RG_ARDLL_P_GAIN_B0) |
		p_fld(0x7, B0_DLL_ARPI5_RG_ARDLL_IDLECNT_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDIV_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_DIV_DEC_B0) |
		p_fld(0x2, B0_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_MON_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI5,
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_B1) |
		p_fld(0x7, B1_DLL_ARPI5_RG_ARDLL_GAIN_B1) |
		p_fld(0x8, B1_DLL_ARPI5_RG_ARDLL_P_GAIN_B1) |
		p_fld(0x7, B1_DLL_ARPI5_RG_ARDLL_IDLECNT_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDIV_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_DIV_DEC_B1) |
		p_fld(0x2, B1_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_MON_SEL_B1));
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI0, SET_FLD,
		CA_DLL_ARPI0_RG_ARDLL_PD_CK_SEL_CA);
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI0, SET_FLD,
		B0_DLL_ARPI0_RG_ARDLL_FASTPJ_CK_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI0, SET_FLD,
		B1_DLL_ARPI0_RG_ARDLL_FASTPJ_CK_SEL_B1);
	/*
	 * zj mark io_32_write_fld_multi_all(DDRPHY_SHU1_PLL9,
	 * p_fld(SET_FLD, SHU1_PLL9_RG_RPHYPLL_LVROD_EN) |
	 * zj mark			| p_fld(0x2, SHU1_PLL9_RG_RPHYPLL_RST_DLY));
	 * zj mark io_32_write_fld_multi_all(DDRPHY_SHU1_PLL11,
	 * p_fld(SET_FLD, SHU1_PLL11_RG_RCLRPLL_LVROD_EN) |
	 * zj mark			| p_fld(0x2, SHU1_PLL11_RG_RCLRPLL_RST_DLY));
	 */
	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL9,
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_LVROD_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_RST_DLY) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_MONCK_EN)); /* cc change */
	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL11,
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_LVROD_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_RST_DLY) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_MONCK_EN)); /* cc change */

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_PLL4, CLEAR_FLD,
		PLL4_RG_RPHYPLL_RESETB);

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_PLL4, SET_FLD,
		PLL4_RG_RPHYPLL_RESETB); /* cc move here */
	/* 1. PLL enable */
	io_32_write_fld_align_all(DDRPHY_PLL2, CLEAR_FLD, PLL2_RG_RCLRPLL_EN);

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_PLL2, SET_FLD, PLL2_RG_RCLRPLL_EN);

	delay_us(31);

	/* 2. SSC enable */
	ssc_enable(p);

	io_32_write_fld_align_all(DDRPHY_B0_DQ5, SET_FLD,
		B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, SET_FLD,
		B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ5, 0x9,
		SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ5, 0x9,
		SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);
	io_32_write_fld_align_all(DDRPHY_B0_DQ5, 0,
		B0_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, 0,
		B1_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B1);

	io_32_write_fld_multi_all(DDRPHY_PLL4,
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_MCK8X_EN) |
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_MCK8X_SEL));

	/* 3. PI RESETB release */
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI0,
		SET_FLD, CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI0,
		SET_FLD, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI0,
		SET_FLD, B1_DLL_ARPI0_RG_ARPI_RESETB_B1);

	delay_us(1);

	if (p->frequency >= DDR1600_FREQ) {
	/* 4. DCDL enable */
	io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
		SET_FLD, CA_CKGEN_DLL1_RG_CKGEN_DCDL_EN_CA);
	io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
		SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DCDL_EN_B01);

	/*5. DLL enable. AUTO mode */
	io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
		SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_B01);
	io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
		SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_CA);

	delay_us(1);
	}

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI2,
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCK_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_MCTL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARRPI_CG_FB_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CS) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CMD) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI2,
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI2,
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCK_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_MCTL_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_FB_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQS_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQM_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQ_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_MPDIV_CG_B1));

	/* 6. PI enable */
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI3,
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_MCTL_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_FB_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CS_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CLK_EN) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI3_RG_ARPI_CLKIEN_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CMD_EN));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI3,
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_FB_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQS_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQM_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQ_EN_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI3,
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_FB_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQS_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQM_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQ_EN_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B1));


	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL0, 0x21f,
		MISC_CG_CTRL0_CLK_MEM_DFS_CFG);

	delay_us(1);

	/* 7. Master PHDET_EN */
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI5,
		SET_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDET_EN_CA);

	delay_us(1);

	/* 8. Slave PHDET_EN */
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI5,
		SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDET_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI5,
		SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDET_EN_B1);
	/* PLL sequence Complete */

	delay_us(1);

	set_dram_type(p);

	io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB));
	io_32_write_fld_align_all(DDRPHY_B1_DQ3,
		SET_FLD, B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB);

	io_32_write_fld_align_all(DDRPHY_MISC_CTRL3,
		CLEAR_FLD, MISC_CTRL3_ARPI_CG_CLK_OPT);
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI2,
		SET_FLD, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN);

	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ1,
		p_fld(0xA, SHU1_B0_DQ1_RG_TX_ARDQ_ODTN_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ1_RG_TX_ARDQ_ODTP_B0) |
		p_fld(0xD, SHU1_B0_DQ1_RG_TX_ARDQ_DRVN_B_B0) |
		p_fld(0xE, SHU1_B0_DQ1_RG_TX_ARDQ_DRVP_B_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ2,
		p_fld(0xA, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTN_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTP_B0) |
		p_fld(0xD, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVN_B0) |
		p_fld(0xE, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVP_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ1,
		p_fld(0xA, SHU1_B1_DQ1_RG_TX_ARDQ_ODTN_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ1_RG_TX_ARDQ_ODTP_B1) |
		p_fld(0xD, SHU1_B1_DQ1_RG_TX_ARDQ_DRVN_B1) |
		p_fld(0xE, SHU1_B1_DQ1_RG_TX_ARDQ_DRVP_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ2,
		p_fld(0xA, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTN_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTP_B1) |
		p_fld(0xD, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVN_B1) |
		p_fld(0xE, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVP_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD1,
		p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN_B) |
		p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP_B));
	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
		p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
		p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
		p_fld(0xf, SHURK0_DQSIEN_R0DQSIEN3DLY) |
		p_fld(0xf, SHURK0_DQSIEN_R0DQSIEN2DLY) |
		p_fld(0xf, SHURK0_DQSIEN_R0DQSIEN1DLY) |
		p_fld(0xf, SHURK0_DQSIEN_R0DQSIEN0DLY));
	io_32_write_fld_multi_all(DRAMC_REG_STBCAL1,
		p_fld(SET_FLD, STBCAL1_DLLFRZ_MON_PBREF_OPT) |
		p_fld(SET_FLD, STBCAL1_STB_FLAGCLR) |
		p_fld(SET_FLD, STBCAL1_STBCNT_MODESEL));

	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DRVING1,
		p_fld(0x14a, SHU1_DRVING1_DQSDRV2) |
		p_fld(0x14a, SHU1_DRVING1_DQSDRV1) |
		p_fld(0x14a, SHU1_DRVING1_DQDRV2));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DRVING2,
		p_fld(0x14a, SHU1_DRVING2_DQDRV1) |
		p_fld(0x14a, SHU1_DRVING2_CMDDRV2) |
		p_fld(0x14a, SHU1_DRVING2_CMDDRV1));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DRVING3,
		p_fld(0x14a, SHU1_DRVING3_DQSODT2) |
		p_fld(0x14a, SHU1_DRVING3_DQSODT1) |
		p_fld(0x14a, SHU1_DRVING3_DQODT2));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DRVING4,
		p_fld(0x14a, SHU1_DRVING4_DQODT1) |
		p_fld(0x14a, SHU1_DRVING4_CMDODT2) |
		p_fld(0x14a, SHU1_DRVING4_CMDODT1));
	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(CLEAR_FLD, SHUCTRL2_HWSET_WLRL) |
		p_fld(SET_FLD, SHUCTRL2_SHU_PERIOD_GO_ZERO_CNT) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_OPTION) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_PARK_N) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_DLL_CHA) |
		p_fld(0xa, SHUCTRL2_R_DLL_IDLE));
	io_32_write_fld_multi_all(DRAMC_REG_DVFSDLL,
		p_fld(0x18, DVFSDLL_DLL_IDLE_SHU4) |
		p_fld(0x18, DVFSDLL_DLL_IDLE_SHU3) |
		p_fld(0x18, DVFSDLL_DLL_IDLE_SHU2) |
		p_fld(SET_FLD, DVFSDLL_DLL_LOCK_SHU_EN));
	io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
		p_fld(SET_FLD, DDRCONF0_LPDDR4EN) |
		p_fld(SET_FLD, DDRCONF0_DM64BITEN) |
		p_fld(SET_FLD, DDRCONF0_BC4OTF) |
		p_fld(SET_FLD, DDRCONF0_DM8BKEN));

	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_STB_GERR_B01) |
		p_fld(SET_FLD, EYESCAN_STB_GERRSTOP) |
		p_fld(SET_FLD, EYESCAN_EYESCAN_RD_SEL_OPT));
	io_32_write_fld_align_all(DRAMC_REG_EYESCAN, SET_FLD,
		EYESCAN_STB_GERR_RST);
	io_32_write_fld_align_all(DRAMC_REG_EYESCAN, CLEAR_FLD,
		EYESCAN_STB_GERR_RST);
	io_32_write_fld_align_all(DRAMC_REG_CLKAR, SET_FLD, CLKAR_PSELAR);
	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DDRPHY_MISC_CTRL0,
			p_fld(CLEAR_FLD, MISC_CTRL0_R_STBENCMP_DIV4CK_EN) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_FIFO_EN) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_RDSEL_LAT) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMSTBEN_OUTSEL) |
			p_fld(0xf, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));
	} else {
		io_32_write_fld_multi_all(DDRPHY_MISC_CTRL0,
			p_fld(CLEAR_FLD, MISC_CTRL0_R_STBENCMP_DIV4CK_EN) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_FIFO_EN) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_RDSEL_LAT) |
			p_fld(SET_FLD, MISC_CTRL0_R_DMSTBEN_OUTSEL) |
			p_fld(0xf, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));
	}
	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL1,
		p_fld(SET_FLD, MISC_CTRL1_R_DMDA_RRESETB_E) |
		p_fld(SET_FLD, MISC_CTRL1_R_HWSAVE_MODE_ENA) |
		p_fld(SET_FLD, MISC_CTRL1_R_DMDQSIENCG_EN) |
		//p_fld(SET_FLD, MISC_CTRL1_R_DMPINMUX) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCMD_OE) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCLK_OE));

	io_32_write_fld_multi_all(DDRPHY_CA_CMD7,
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARRESETB_PULL_DN) |
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARCMD_D0_PULL_UP) |
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARCMD_D0_PULL_DN) |
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARCS_PULL_DN));

	io_32_write_fld_multi_all(DDRPHY_B0_DQ7,
		p_fld(CLEAR_FLD, B0_DQ7_RG_TX_ARDQ_PULL_DN_B0) |
		p_fld(SET_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RB_EXT_EN_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RST_EXT_EN_B01));
	io_32_write_fld_align_all(DDRPHY_B1_DQ7, CLEAR_FLD,
		B1_DQ7_RG_TX_ARDQ_PULL_DN_B1);

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF0,
		p_fld(0x2, SHU_CONF0_MATYPE) |
		p_fld(SET_FLD, SHU_CONF0_BL4) |
		p_fld(SET_FLD, SHU_CONF0_FREQDIV4) |
		p_fld(SET_FLD, SHU_CONF0_REFTHD) |
		p_fld(SET_FLD, SHU_CONF0_ADVPREEN) |
		p_fld(0x3f, SHU_CONF0_DMPGTIM));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE2) |
		p_fld(SET_FLD, SHU_ODTCTRL_TWODT) |
		p_fld(0x5, SHU_ODTCTRL_RODT) |
		p_fld(SET_FLD, SHU_ODTCTRL_WOEN) |
		p_fld(SET_FLD, SHU_ODTCTRL_ROEN));
	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, 0x5,
		REFCTRL0_REF_PREGATE_CNT);
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA1,
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_WE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RESET) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_ODT) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CKE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA2,
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CKE1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CMD) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA3,
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA7) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA6) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA5) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA4) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA3) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA4,
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA15) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA14) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA13) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA12) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA11) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA10) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA9) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA8));
	io_32_write_fld_align_all(DRAMC_REG_SHU_SELPH_CA5, CLEAR_FLD,
		SHU_SELPH_CA5_DLY_ODT);
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(0x5, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(0x5, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(0x5, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(0x5, SHU_SELPH_DQS1_DLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
		p_fld(0x3, SHURK0_SELPH_DQ0_TXDLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
		p_fld(0x3, SHURK0_SELPH_DQ1_TXDLY_DQM0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(0x2, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(0x6, SHURK0_SELPH_DQ2_DLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(0x2, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(0x6, SHURK0_SELPH_DQ3_DLY_DQM0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ0,
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ3) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ2) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ1) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ0) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1DQ3) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1DQ2) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1DQ1) |
		p_fld(0x3, SHURK1_SELPH_DQ0_TX_DLY_R1DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ1,
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM3) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM2) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM1) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM0) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1DQM3) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1DQM2) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1DQM1) |
		p_fld(0x3, SHURK1_SELPH_DQ1_TX_DLY_R1DQM0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ2,
		p_fld(0x2, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ3) |
		p_fld(0x2, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ2) |
		p_fld(0x2, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ1) |
		p_fld(0x2, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ0) |
		p_fld(0x6, SHURK1_SELPH_DQ2_DLY_R1DQ3) |
		p_fld(0x6, SHURK1_SELPH_DQ2_DLY_R1DQ2) |
		p_fld(0x6, SHURK1_SELPH_DQ2_DLY_R1DQ1) |
		p_fld(0x6, SHURK1_SELPH_DQ2_DLY_R1DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ3,
		p_fld(0x2, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM3) |
		p_fld(0x2, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM2) |
		p_fld(0x2, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM1) |
		p_fld(0x2, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM0) |
		p_fld(0x6, SHURK1_SELPH_DQ3_DLY_R1DQM3) |
		p_fld(0x6, SHURK1_SELPH_DQ3_DLY_R1DQM2) |
		p_fld(0x6, SHURK1_SELPH_DQ3_DLY_R1DQM1) |
		p_fld(0x6, SHURK1_SELPH_DQ3_DLY_R1DQM0));
#if 0
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ7,
		p_fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1) |
		p_fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ7,
		p_fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0) |
		p_fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
#endif

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
			p_fld(0x6, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
			p_fld(0x6, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
			p_fld(0x6, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
			p_fld(0x2, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
			p_fld(0x4, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
			p_fld(0x7, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
			p_fld(0x7, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
			p_fld(0x7, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
			p_fld(0x7, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));
	}

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_B1_DQ3, SET_FLD,
		B1_DQ3_RG_RX_ARDQS1_DQSIENMODE);
	io_32_write_fld_align_all(DDRPHY_B0_DQ3, SET_FLD,
		B0_DQ3_RG_RX_ARDQS0_DQSIENMODE);
	io_32_write_fld_align_all(DRAMC_REG_STBCAL, SET_FLD, STBCAL_DQSIENMODE);
	io_32_write_fld_multi_all(DRAMC_REG_SREFCTRL,
		p_fld(CLEAR_FLD, SREFCTRL_SREF_HW_EN) |
		p_fld(0x8, SREFCTRL_SREFDLY));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CKECTRL,
		p_fld(0x3, SHU_CKECTRL_SREF_CK_DLY) |
		p_fld(0x3, SHU_CKECTRL_TCKESRX) |
		p_fld(0x3, SHU_CKECTRL_CKEPRD));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_PIPE,
		p_fld(SET_FLD, SHU_PIPE_READ_START_EXTEND1) |
		p_fld(SET_FLD, SHU_PIPE_DLE_LAST_EXTEND1));
	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(SET_FLD, CKECTRL_CKEON) |
		p_fld(SET_FLD, CKECTRL_CKETIMER_SEL));
	io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD,
		RKCFG_CKE2RANK_OPT2);

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF2,
			p_fld(SET_FLD, SHU_CONF2_WPRE2T) |
			p_fld(0x7, SHU_CONF2_DCMDLYREF) |
			p_fld(0x64, SHU_CONF2_FSPCHG_PRDCNT));
		io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
			SPCMDCTRL_CLR_EN);
		io_32_write_fld_align_all(DRAMC_REG_SHU_SCINTV, 0xf,
			SHU_SCINTV_MRW_INTV);
		io_32_write_fld_align_all(DRAMC_REG_SHUCTRL1, 0x40,
			SHUCTRL1_FC_PRDCNT);
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF2,
			p_fld(SET_FLD, SHU_CONF2_WPRE2T) |
			p_fld(0x7, SHU_CONF2_DCMDLYREF) |
			p_fld(0x64, SHU_CONF2_FSPCHG_PRDCNT));
		io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
			SPCMDCTRL_CLR_EN);
		io_32_write_fld_align_all(DRAMC_REG_SHU_SCINTV, 0xf,
			SHU_SCINTV_MRW_INTV);
		io_32_write_fld_align_all(DRAMC_REG_SHUCTRL1, 0x40,
			SHUCTRL1_FC_PRDCNT);
	}
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(CLEAR_FLD, REFCTRL1_SREF_PRD_OPT));

	/*
	 * zj mark io_32_write_fld_align_all(DDRPHY_SHU1_PLL4,
	 * 0xfe,
	 * SHU1_PLL4_RG_RPHYPLL_RESERVED);
	 * zj mark io_32_write_fld_align_all(DDRPHY_SHU1_PLL6,
	 * 0xfe,
	 * SHU1_PLL6_RG_RCLRPLL_RESERVED);
	 */
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL4, CLEAR_FLD,
		SHU1_PLL4_RG_RPHYPLL_RESERVED);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL6, CLEAR_FLD,
		SHU1_PLL6_RG_RCLRPLL_RESERVED);
	io_32_write_fld_multi_all(DRAMC_REG_REFRATRE_FILTER,
		p_fld(SET_FLD, REFRATRE_FILTER_PB2AB_OPT));

#if !APPLY_LP4_POWER_INIT_SEQUENCE
	io_32_write_fld_align_all(DRAMC_REG_DDRCONF0, SET_FLD, DDRCONF0_GDDR3RST);
#endif
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
		DRAMCTRL_CLKWITRFC);
	io_32_write_fld_multi_all(DRAMC_REG_MISCTL0,
		p_fld(SET_FLD, MISCTL0_REFP_ARB_EN2) |
		p_fld(SET_FLD, MISCTL0_PBC_ARB_EN) |
		p_fld(SET_FLD, MISCTL0_REFA_ARB_EN2));
	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_MWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWSPLIT) |
		p_fld(SET_FLD, PERFCTL0_WFLUSHEN) |
		p_fld(SET_FLD, PERFCTL0_EMILLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWAGEEN) |
		p_fld(SET_FLD, PERFCTL0_RWLLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWOFOEN) |
		p_fld(SET_FLD, PERFCTL0_DISRDPHASE1) |
		p_fld(SET_FLD, PERFCTL0_DUALSCHEN));
	io_32_write_fld_align_all(DRAMC_REG_ARBCTL, 0x80,
		ARBCTL_MAXPENDCNT);
	io_32_write_fld_multi_all(DRAMC_REG_PADCTRL,
		p_fld(SET_FLD, PADCTRL_DQIENLATEBEGIN) |
		p_fld(SET_FLD, PADCTRL_DQIENQKEND));
	io_32_write_fld_align_all(DRAMC_REG_DRAMC_PD_CTRL, SET_FLD,
		DRAMC_PD_CTRL_DCMREF_OPT);
	io_32_write_fld_align_all(DRAMC_REG_CLKCTRL, SET_FLD, CLKCTRL_REG_CLK_1);
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(0x4, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_DLLFRZ));
	io_32_write_fld_multi_all(DRAMC_REG_CATRAINING1,
		p_fld(0xff, CATRAINING1_CATRAIN_INTV) |
		p_fld(CLEAR_FLD, CATRAINING1_CATRAINLAT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x6, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(0x4, SHU_RANKCTL_RANKINCTL_ROOT1) |
		p_fld(0x4, SHU_RANKCTL_RANKINCTL) |
		p_fld(SET_FLD, SHU_RANKCTL_DMSTBLAT));
	io_32_write_fld_align_all(DRAMC_REG_SHURK1_DQSCTL, 0x5,
		SHURK1_DQSCTL_R1DQSINCTL);

	delay_us(2);

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x4, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
			p_fld(0x3, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
			p_fld(0x3, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
			p_fld(0x7, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
			p_fld(0x5, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
			p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));
	} else  {
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
			p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
			p_fld(0x2, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
			p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN0DLY) |
			p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN1DLY));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
			p_fld(0x3, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
			p_fld(0x3, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
			p_fld(0x3, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
			p_fld(0x3, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
			p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
			p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1));
	}

	/*
	 * 41536 === over_write_setting_begin ===
	 * 41536 === over_write_setting_end ===
	 */
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_PREALL_OPTION);
	io_32_write_fld_align_all(DRAMC_REG_ZQCS, 0x56, ZQCS_ZQCSOP);

	delay_us(1);

	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF3, 0xff,
		SHU_CONF3_REFRCNT);
	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, SET_FLD,
		REFCTRL0_REFFRERUN);
	io_32_write_fld_align_all(DRAMC_REG_SREFCTRL, SET_FLD,
		SREFCTRL_SREF_HW_EN);
	io_32_write_fld_align_all(DRAMC_REG_MPC_OPTION, SET_FLD,
		MPC_OPTION_MPCRKEN);
	io_32_write_fld_align_all(DRAMC_REG_DRAMC_PD_CTRL, CLEAR_FLD,
		DRAMC_PD_CTRL_PHYCLKDYNGEN);
	io_32_write_fld_align_all(DRAMC_REG_DRAMC_PD_CTRL, SET_FLD,
		DRAMC_PD_CTRL_DCMEN);
	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_RX_DQ_EYE_SEL) |
		p_fld(SET_FLD, EYESCAN_RG_EX_EYE_SCAN_EN));
	io_32_write_fld_multi_all(DRAMC_REG_STBCAL1,
		p_fld(SET_FLD, STBCAL1_STBCNT_LATCH_EN) |
		p_fld(SET_FLD, STBCAL1_STBENCMPEN));
	io_32_write_fld_align_all(DRAMC_REG_TEST2_1,
		0x10000, TEST2_1_TEST2_BASE_ADR);
#if (FOR_DV_SIMULATION_USED == 1)
	/*
	 * because cmd_len=1 has bug with byte mode,
	 * so need to set cmd_len=0,
	 * then it will cost more time to do a pattern test
	 * workaround: reduce TEST2_OFF to make less test agent cmd.
	 * make lpddr4-1600 can finish in 60us
	 */
	io_32_write_fld_align_all(DRAMC_REG_TEST2_2, 0x20,
		TEST2_2_TEST2_OFFSET_ADR);
#else
	io_32_write_fld_align_all(DRAMC_REG_TEST2_2, 0x400,
		TEST2_2_TEST2_OFFSET_ADR);
#endif
	io_32_write_fld_multi_all(DRAMC_REG_TEST2_3,
		p_fld(SET_FLD, TEST2_3_TESTWREN2_HW_EN) |
		p_fld(0x4, TEST2_3_DQSICALSTP) |
		p_fld(SET_FLD, TEST2_3_TESTAUDPAT));
	io_32_write_fld_align_all(DRAMC_REG_RSTMASK, CLEAR_FLD,
		RSTMASK_DAT_SYNC_MASK);
	io_32_write_fld_align_all(DRAMC_REG_RSTMASK, CLEAR_FLD,
		RSTMASK_PHY_SYNC_MASK);

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_HW_MRR_FUN,
		p_fld(CLEAR_FLD, HW_MRR_FUN_TRPMRR_EN) |
		p_fld(CLEAR_FLD, HW_MRR_FUN_TRCDMRR_EN) |
		p_fld(CLEAR_FLD, HW_MRR_FUN_TMRR_ENA));

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
			p_fld(SET_FLD, PERFCTL0_WRFIO_MODE2) |
			p_fld(CLEAR_FLD, PERFCTL0_RWSPLIT));
		io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
			p_fld(SET_FLD, PERFCTL0_REORDEREN) |
			p_fld(CLEAR_FLD, PERFCTL0_REORDER_MODE));
		io_32_write_fld_align_all(DRAMC_REG_RSTMASK, CLEAR_FLD,
			RSTMASK_GT_SYNC_MASK);
		io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD, RKCFG_DQSOSC2RK);
		io_32_write_fld_align_all(DRAMC_REG_MRS, CLEAR_FLD, MRS_MPCRK);
		io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
			SPCMDCTRL_REFR_BLOCKEN);
		io_32_write_fld_align_all(DRAMC_REG_EYESCAN, SET_FLD,
			EYESCAN_RG_RX_MIOCK_JIT_EN);
	} else {
		io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
			DRAMCTRL_CTOREQ_HPRI_OPT);
		io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
			p_fld(SET_FLD, PERFCTL0_REORDEREN) |
			p_fld(CLEAR_FLD, PERFCTL0_REORDER_MODE));
		io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
			SPCMDCTRL_REFR_BLOCKEN);
		io_32_write_fld_align_all(DRAMC_REG_RSTMASK, CLEAR_FLD,
			RSTMASK_GT_SYNC_MASK);
		io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD, RKCFG_DQSOSC2RK);

		io_32_write_fld_align_all(DRAMC_REG_MRS, 0, MRS_MPCRK);
		io_32_write_fld_align_all(DRAMC_REG_MPC_OPTION, 1,
			MPC_OPTION_MPCRKEN);
		io_32_write_fld_align_all(DRAMC_REG_EYESCAN, SET_FLD,
			EYESCAN_RG_RX_MIOCK_JIT_EN);
		io_32_write_fld_align_all(DRAMC_REG_SHU1_WODT, SET_FLD,
			SHU1_WODT_DBIWR);
		io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ7, SET_FLD,
			SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0);
		io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ7, SET_FLD,
			SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
			p_fld(0x3, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
			p_fld(0x4, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));
		io_32_write_fld_align_all(DRAMC_REG_SHU_RANKCTL, 0x4,
			SHU_RANKCTL_RANKINCTL);
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
			p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
			p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN) |
			p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
			p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ0,
			p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ1) |
			p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ1,
			p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM1) |
			p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM0));
	}
	delay_us(5);

	io_32_write_fld_align_all(DRAMC_REG_STBCAL1, 0x3,
		STBCAL1_STBCAL_FILTER);
	io_32_write_fld_align_all(DRAMC_REG_STBCAL1, SET_FLD,
		STBCAL1_STBCAL_FILTER);
	io_32_write_fld_multi_all(DRAMC_REG_STBCAL,
		p_fld(SET_FLD, STBCAL_STB_DQIEN_IG) |
		p_fld(SET_FLD, STBCAL_PICHGBLOCK_NORD) |
		p_fld(CLEAR_FLD, STBCAL_STBCALEN) |
		p_fld(CLEAR_FLD, STBCAL_STB_SELPHYCALEN) |
		p_fld(SET_FLD, STBCAL_PIMASK_RKCHG_OPT));
	io_32_write_fld_align_all(DRAMC_REG_STBCAL1, SET_FLD,
		STBCAL1_STB_SHIFT_DTCOUT_IG);
	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DQSG,
		p_fld(0x9, SHU1_DQSG_STB_UPDMASKCYC) |
		p_fld(SET_FLD, SHU1_DQSG_STB_UPDMASK_EN));
	io_32_write_fld_align_all(DDRPHY_MISC_CTRL0, CLEAR_FLD,
		MISC_CTRL0_R_DMDQSIEN_SYNCOPT);
	io_32_write_fld_align_all(DRAMC_REG_SHU_RANKCTL, SET_FLD,
		SHU_RANKCTL_DQSG_MODE);
	io_32_write_fld_align_all(DRAMC_REG_STBCAL, SET_FLD,
		STBCAL_SREF_DQSGUPD);
	io_32_write_fld_align_all(DDRPHY_MISC_CTRL1, CLEAR_FLD,
		MISC_CTRL1_R_DMDQMDBI);
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(SET_FLD, SHU_RANKCTL_PICGLAT) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_DMSTBLAT));
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(SET_FLD, REFCTRL1_REF_QUE_AUTOSAVE_EN) |
		p_fld(SET_FLD, REFCTRL1_SLEFREF_AUTOSAVE_EN));
	io_32_write_fld_multi_all(DRAMC_REG_DQSOSCR,
		p_fld(SET_FLD, DQSOSCR_SREF_TXPI_RELOAD_OPT) |
		p_fld(SET_FLD, DQSOSCR_SREF_TXUI_RELOAD_OPT));
	io_32_write_fld_multi_all(DRAMC_REG_RSTMASK,
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_GT_SYNC_MASK_FOR_PHY) |
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK_FOR_PHY));
	io_32_write_fld_align_all(DRAMC_REG_RKCFG, CLEAR_FLD, RKCFG_RKMODE);

#if !APPLY_LP4_POWER_INIT_SEQUENCE
	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(SET_FLD, CKECTRL_CKEFIXON) |
		p_fld(SET_FLD, CKECTRL_CKE1FIXON));
#endif

	delay_us(12);

	/*
	 * TODO:DDR3200
	 * if(p->frequency==1600)
	 */
	{

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1)
		{
			io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
				p_fld(0x5, SHU_RANKCTL_RANKINCTL_PHY) |
				p_fld(0x3, SHU_RANKCTL_RANKINCTL_ROOT1) |
				p_fld(0x3, SHU_RANKCTL_RANKINCTL));
		}
		else
		{
			io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
				p_fld(0x6, SHU_RANKCTL_RANKINCTL_PHY) |
				p_fld(0x4, SHU_RANKCTL_RANKINCTL_ROOT1));
		}
		io_32_write_fld_multi_all(DRAMC_REG_SHU1_DQSOSC_PRD,
			p_fld(0xa, SHU1_DQSOSC_PRD_DQSOSCTHRD_DEC) |
			p_fld(0xa, SHU1_DQSOSC_PRD_DQSOSCTHRD_INC) |
			p_fld(0x10, SHU1_DQSOSC_PRD_DQSOSC_PRDCNT));
		io_32_write_fld_align_all(DRAMC_REG_SHU_DQSOSCR, 0x10,
			SHU_DQSOSCR_DQSOSCRCNT);
		io_32_write_fld_align_all(DRAMC_REG_SHU1_WODT, CLEAR_FLD,
			SHU1_WODT_WPST2T);

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
				p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN1DLY) |
				p_fld(0x19, SHURK0_DQSIEN_R0DQSIEN0DLY));
		}

		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_PI,
			p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQM_B1) |
			p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQM_B0) |
			p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQ_B1) |
			p_fld(0x1a, SHU1RK0_PI_RK0_ARPI_DQ_B0));
		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK0_DQSOSC,
			p_fld(0x168, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0_B1) |
			p_fld(0x168, SHU1RK0_DQSOSC_DQSOSC_BASE_RK0));

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
				p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
				p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
				p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
			io_32_write_fld_align_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
				0x4,
				SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1);
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
				p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
				p_fld(0x5, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
				p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));
		} else {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
				p_fld(0x2, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1));
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
				p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
			io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
				p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
				p_fld(0x3, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
				p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
				p_fld(0x3, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));

		}
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
			p_fld(0x4, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
			p_fld(0x4, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
			p_fld(0x4, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
			p_fld(0x4, SHURK0_SELPH_DQ2_DLY_OEN_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
			p_fld(0x4, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
			p_fld(0x4, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
			p_fld(0x4, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
			p_fld(0x4, SHURK0_SELPH_DQ3_DLY_OEN_DQM0));

		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_PI,
			p_fld(0x14, SHU1RK1_PI_RK1_ARPI_DQM_B1) |
			p_fld(0x14, SHU1RK1_PI_RK1_ARPI_DQM_B0) |
			p_fld(0x14, SHU1RK1_PI_RK1_ARPI_DQ_B1) |
			p_fld(0x14, SHU1RK1_PI_RK1_ARPI_DQ_B0));
		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQSOSC,
			p_fld(0x127, SHU1RK1_DQSOSC_DQSOSC_BASE_RK1_B1) |
			p_fld(0x127, SHU1RK1_DQSOSC_DQSOSC_BASE_RK1));

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN));
		} else {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN) |
				p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN_P1) |
				p_fld(0x2, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN_P1));
		}

		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
			p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_align_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
				0x4,
				SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED);
			io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
				p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
				p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
				p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
				p_fld(0x2, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));
		} else {
			io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
				p_fld(0x5, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
				p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
				p_fld(0x5, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
				p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));
		}
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ2,
			p_fld(0x4, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ3) |
			p_fld(0x4, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ2) |
			p_fld(0x4, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ1) |
			p_fld(0x4, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ3,
			p_fld(0x4, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM3) |
			p_fld(0x4, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM2) |
			p_fld(0x4, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM1) |
			p_fld(0x4, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM0));

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL1,
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL1_BOOT_ORIG_UI_RK1_DQ1) |
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL1_BOOT_ORIG_UI_RK1_DQ0));
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL2,
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL2_BOOT_TARG_UI_RK1_DQ1) |
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL2_BOOT_TARG_UI_RK1_DQ0));
		} else {
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL1,
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL1_BOOT_ORIG_UI_RK1_DQ1) |
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL1_BOOT_ORIG_UI_RK1_DQ0));
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL2,
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL2_BOOT_TARG_UI_RK1_DQ1) |
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL2_BOOT_TARG_UI_RK1_DQ0));
		}
		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL3,
			p_fld(0x14, SHU1RK1_DQS2DQ_CAL3_BOOT_TARG_UI_RK1_OEN_DQ1_B4TO0) |
			p_fld(0x14, SHU1RK1_DQS2DQ_CAL3_BOOT_TARG_UI_RK1_OEN_DQ0_B4TO0) |
			p_fld(0x1c, SHU1RK1_DQS2DQ_CAL3_BOOT_TARG_UI_RK1_OEN_DQ1) |
			p_fld(0x1c, SHU1RK1_DQS2DQ_CAL3_BOOT_TARG_UI_RK1_OEN_DQ0));
		io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL4,
			p_fld(0x14, SHU1RK1_DQS2DQ_CAL4_BOOT_TARG_UI_RK1_OEN_DQM1_B4TO0) |
			p_fld(0x14, SHU1RK1_DQS2DQ_CAL4_BOOT_TARG_UI_RK1_OEN_DQM0_B4TO0) |
			p_fld(0x1c, SHU1RK1_DQS2DQ_CAL4_BOOT_TARG_UI_RK1_OEN_DQM1) |
			p_fld(0x1c, SHU1RK1_DQS2DQ_CAL4_BOOT_TARG_UI_RK1_OEN_DQM0));

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL5,
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL5_BOOT_TARG_UI_RK1_DQM1) |
				p_fld(0x3d4, SHU1RK1_DQS2DQ_CAL5_BOOT_TARG_UI_RK1_DQM0));
		} else {
			io_32_write_fld_multi_all(DRAMC_REG_SHU1RK1_DQS2DQ_CAL5,
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL5_BOOT_TARG_UI_RK1_DQM1) |
				p_fld(0x2d4, SHU1RK1_DQS2DQ_CAL5_BOOT_TARG_UI_RK1_DQM0));
		}

		if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
			io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ7, SET_FLD,
				SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0);
			io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ7, SET_FLD,
				SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1);
		} else {
			io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
				p_fld(SET_FLD, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0));
			io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ7,
				p_fld(SET_FLD, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1));
		}
	}

#if 1
	if (p->frequency == DDR2400_FREQ) {
		dramc_setting_ddr2667(p);
	} else if (p->frequency == DDR1600_FREQ) {
		dramc_setting_ddr1600(p);
	}
#endif

	if (p->frequency == DDR2400_FREQ) {
		io_32_write_fld_multi_all(DRAMC_REG_SHU1_WODT,
		p_fld(CLEAR_FLD, SHU1_WODT_WPST2T) |
		p_fld(SET_FLD, SHU1_WODT_DBIWR));
	} else if (p->frequency <= DDR1600_FREQ) {
		io_32_write_fld_multi_all(DRAMC_REG_SHU1_WODT,
		p_fld(SET_FLD, SHU1_WODT_WPST2T) |
		p_fld(CLEAR_FLD, SHU1_WODT_DBIWR));
	}

	update_initial_setting_lp4(p);

#if SIMULATION_SW_IMPED
	if (p->dram_type == TYPE_LPDDR4)
		dramc_sw_impedance_save_register(p, p->odt_onoff,
			p->odt_onoff, DRAM_DFS_SHUFFLE_1);
#endif

#if (FOR_DV_SIMULATION_USED == 0)
	//ddrphy_freq_meter();
#endif

	delay_us(1);
#if 0
	io_32_write_fld_multi_all(DRAMC_REG_MRS,
		p_fld(CLEAR_FLD, MRS_MRSRK) |
		p_fld(0x4, MRS_MRSMA) |
		p_fld(CLEAR_FLD, MRS_MRSOP));
	delay_us(1);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, SET_FLD, SPCMD_MRREN);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, CLEAR_FLD, SPCMD_MRREN);
#endif

	io_32_write_fld_align_all(DRAMC_REG_TEST2_4, CLEAR_FLD,
		TEST2_4_TEST_REQ_LEN1);

	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF3, 0x5,
		SHU_CONF3_ZQCSCNT);

	delay_us(1);

#if !APPLY_LP4_POWER_INIT_SEQUENCE
	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(CLEAR_FLD, CKECTRL_CKEFIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKE1FIXON));
#endif
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(SET_FLD, REFCTRL0_PBREFEN) |
		p_fld(SET_FLD, REFCTRL0_PBREF_DISBYRATE) |
		p_fld(SET_FLD, REFCTRL0_REFNA_OPT));

	if (p->dram_cbt_mode[p->rank] == CBT_BYTE_MODE1) {
		io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, SET_FLD,
			DQSOSCR_RK0_BYTE_MODE);
		io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, SET_FLD,
			DQSOSCR_RK1_BYTE_MODE);
	}
	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(SET_FLD, SHUCTRL2_MR13_SHU_EN) |
		p_fld(SET_FLD, SHUCTRL2_HWSET_WLRL));
	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, CLEAR_FLD, REFCTRL0_REFDIS);

	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, CLEAR_FLD,
		SPCMDCTRL_REFRDIS);
	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_REQQUE_THD_EN) |
		p_fld(SET_FLD, DRAMCTRL_DPDRK_OPT));
	io_32_write_fld_multi_all(DRAMC_REG_DUMMY_RD,
		p_fld(SET_FLD, DUMMY_RD_DQSG_DMYRD_EN) |
		p_fld(0x2, DUMMY_RD_RANK_NUM) |
		p_fld(SET_FLD, DUMMY_RD_DUMMY_RD_CNT6) |
		p_fld(SET_FLD, DUMMY_RD_DUMMY_RD_CNT5) |
		p_fld(SET_FLD, DUMMY_RD_DUMMY_RD_CNT3) |
		p_fld(SET_FLD, DUMMY_RD_DUMMY_RD_SW));
	io_32_write_fld_align_all(DRAMC_REG_TEST2_4, 0x4,
		TEST2_4_TESTAGENTRKSEL);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL,
		CLEAR_FLD,
		DRAMCTRL_CTOREQ_HPRI_OPT);

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL,
		p_fld(CLEAR_FLD, SHUCTRL_R_DRAMC_CHA) |
		p_fld(CLEAR_FLD, SHUCTRL_SHU_PHYRST_SEL));
	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_DLL_CHA));

	io_set_sw_broadcast(FALSE);

	dramc_mr_init_lp4(p);
	dramc_broadcast_on_off(DRAMC_BROADCAST_OFF);
}
#endif /* SUPPORT_TYPE_LPDDR4 */

/* cc notes: PHY settings are common for these three types */
#if (SUPPORT_TYPE_LPDDR3 || SUPPORT_TYPE_PCDDR3 || SUPPORT_TYPE_PCDDR4)
void ddrphy_pll_setting_common(DRAMC_CTX_T *p)
{
	unsigned char cap_sel;
	unsigned char midpicap_sel;
	unsigned int sdm_pcw, prediv, posdiv; //extfbdiv;

	show_msg2((INFO, "SSC %s\n", p->ssc_en ? "ON" : "OFF"));

	cap_sel = 0x2;
	midpicap_sel = 0x0;

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD6,
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA) |
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1));

	io_32_write_fld_multi_all(DDRPHY_PLL4,
		p_fld(CLEAR_FLD, PLL4_RG_RPHYPLL_MCK8X_EN) |
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_MCK8X_SEL) |
		p_fld(CLEAR_FLD, PLL4_RG_RPHYPLL_RESETB));

	io_32_write_fld_align_all(DDRPHY_PLL1, CLEAR_FLD, PLL1_RG_RPHYPLL_EN);
	io_32_write_fld_align_all(DDRPHY_PLL2, CLEAR_FLD, PLL2_RG_RCLRPLL_EN);

	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI5, CLEAR_FLD,
		CA_DLL_ARPI5_RG_ARDLL_PHDET_EN_CA);
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI5, CLEAR_FLD,
		B0_DLL_ARPI5_RG_ARDLL_PHDET_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI5, CLEAR_FLD,
		B1_DLL_ARPI5_RG_ARDLL_PHDET_EN_B1);

	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI0, CLEAR_FLD,
		CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI0, CLEAR_FLD,
		B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI0, CLEAR_FLD,
		B1_DLL_ARPI0_RG_ARPI_RESETB_B1);

	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI2, SET_FLD,
		CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA);
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI2,
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI2,
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_CG_FB_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI2_RG_ARPI_MPDIV_CG_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL9,
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_MONCK_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_MONVC_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_LVROD_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_MONREF_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL9_RG_RPHYPLL_RST_DLY));
	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL11,
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_MONCK_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_MONVC_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_LVROD_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_MONREF_EN) |
		p_fld(CLEAR_FLD, SHU1_PLL11_RG_RCLRPLL_RST_DLY));

	io_32_write_fld_align_all(DDRPHY_SHU1_PLL0, SET_FLD,
		SHU1_PLL0_RG_RPHYPLL_TOP_REV);

	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL8,
		p_fld(CLEAR_FLD, SHU1_PLL8_RG_RPHYPLL_POSDIV) |
		p_fld(CLEAR_FLD, SHU1_PLL8_RG_RPHYPLL_PREDIV));
	io_32_write_fld_multi_all(DDRPHY_SHU1_PLL10,
		p_fld(2, SHU1_PLL10_RG_RCLRPLL_POSDIV) |
		p_fld(CLEAR_FLD, SHU1_PLL10_RG_RCLRPLL_PREDIV));
	if (p->ssc_en == ENABLE) {
		io_32_write_fld_multi_all(DDRPHY_SHU1_PLL4,
			p_fld(SET_FLD, SHU1_PLL4_RG_RPHYPLL_SDM_FRA_EN) |
			p_fld(0xfe, SHU1_PLL4_RG_RPHYPLL_RESERVED));
		io_32_write_fld_multi_all(DDRPHY_SHU1_PLL6,
			p_fld(SET_FLD, SHU1_PLL6_RG_RCLRPLL_SDM_FRA_EN) |
			p_fld(0xfe, SHU1_PLL6_RG_RCLRPLL_RESERVED));
	} else {
		io_32_write_fld_multi_all(DDRPHY_SHU1_PLL4,
			p_fld(CLEAR_FLD, SHU1_PLL4_RG_RPHYPLL_SDM_FRA_EN) |
			p_fld(0xfe, SHU1_PLL4_RG_RPHYPLL_RESERVED));
		io_32_write_fld_multi_all(DDRPHY_SHU1_PLL6,
			p_fld(CLEAR_FLD, SHU1_PLL6_RG_RCLRPLL_SDM_FRA_EN) |
			p_fld(0xfe, SHU1_PLL6_RG_RCLRPLL_RESERVED));
	}

	prediv = 0x0;
	posdiv = 0x0;

	switch (p->freq_sel) {
	case DDR_DDR1600:
		sdm_pcw = 0x3d000000;
		break;
	case DDR_DDR1866:
		sdm_pcw = 0x47000000;
		break;
	case DDR_DDR2400:
		sdm_pcw = 0x5c000000;
		break;
	case DDR_DDR2667:
		sdm_pcw = 0x66000000;
		break;
	default:
	case DDR_DDR1066:
		sdm_pcw = 0x51000000;
		break;
	}

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_PLL10),
		p_fld(prediv, SHU1_PLL10_RG_RCLRPLL_PREDIV) |
		p_fld(posdiv, SHU1_PLL10_RG_RCLRPLL_POSDIV));
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_PLL7),
		sdm_pcw, SHU1_PLL7_RG_RCLRPLL_SDM_PCW);

	apply_ssc_setting(p, 2, 30, 1);

	/*
	 * io_32_write_fld_multi_all(DDRPHY_PLL6,
	 * p_fld(CLEAR_FLD, PLL6_RG_RPHYPLL_SDM_SSC_DELTA1) |
	 * | p_fld(CLEAR_FLD, PLL6_RG_RPHYPLL_SDM_SSC_DELTA));
	 */
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL8, 1,
		SHU1_PLL8_RG_RPHYPLL_FBDIV_SE);
	io_32_write_fld_align_all(DDRPHY_SHU1_PLL10, SET_FLD,
		SHU1_PLL10_RG_RCLRPLL_FBDIV_SE);

	io_32_write_fld_align_all(DDRPHY_SHU1_PLL1, CLEAR_FLD,
		SHU1_PLL1_RG_RPHYPLLGP_CK_SEL);

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI0,
		p_fld(SET_FLD, CA_DLL_ARPI0_RG_ARPI_BYPASS_SR_CLK) |
		p_fld(SET_FLD, CA_DLL_ARPI0_RG_ARPI_BYPASS_SR_CMD) |
		p_fld(SET_FLD, CA_DLL_ARPI0_RG_ARDLL_PD_CK_SEL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARDLL_FASTPJ_CK_SEL_CA));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI0,
		p_fld(SET_FLD, B0_DLL_ARPI0_RG_ARPI_BYPASS_SR_B01)|
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARDLL_PD_CK_SEL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI0_RG_ARDLL_FASTPJ_CK_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI0,
		p_fld(SET_FLD, B1_DLL_ARPI0_RG_ARPI_BYPASS_SR_DQS_B1)|
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARDLL_PD_CK_SEL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI0_RG_ARDLL_FASTPJ_CK_SEL_B1));

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI5,
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_CA) |
		p_fld(0x6, CA_DLL_ARPI5_RG_ARDLL_GAIN_CA) |
		p_fld(0x9, CA_DLL_ARPI5_RG_ARDLL_IDLECNT_CA) |
		p_fld(0x8, CA_DLL_ARPI5_RG_ARDLL_P_GAIN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI5_RG_ARDLL_PHDIV_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI5_RG_ARDLL_DIV_DEC_CA) |
		p_fld(0x2, CA_DLL_ARPI5_RG_ARDLL_DIV_MCTL_CA));

	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI5,
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_B0) |
		p_fld(0x7, B0_DLL_ARPI5_RG_ARDLL_GAIN_B0) |
		p_fld(0x7, B0_DLL_ARPI5_RG_ARDLL_IDLECNT_B0) |
		p_fld(0x8, B0_DLL_ARPI5_RG_ARDLL_P_GAIN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI5_RG_ARDLL_PHDIV_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI5_RG_ARDLL_DIV_DEC_B0) |
		p_fld(0x2, B0_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B0));

	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI5,
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDET_OUT_SEL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDET_IN_SWAP_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_FJ_OUT_MODE_SEL_B1) |
		p_fld(0x7, B1_DLL_ARPI5_RG_ARDLL_GAIN_B1) |
		p_fld(0x7, B1_DLL_ARPI5_RG_ARDLL_IDLECNT_B1) |
		p_fld(0x8, B1_DLL_ARPI5_RG_ARDLL_P_GAIN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHJUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI5_RG_ARDLL_PHDIV_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI5_RG_ARDLL_DIV_DEC_B1) |
		p_fld(0x2, B1_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B1));

	if (p->dram_type != TYPE_PCDDR4) {
		io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI5, CLEAR_FLD,
			CA_DLL_ARPI5_RG_ARDLL_DIV_MCTL_CA);
		io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI5, CLEAR_FLD,
			B0_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B0);
		io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI5, CLEAR_FLD,
			B1_DLL_ARPI5_RG_ARDLL_DIV_MCTL_B1);
	}

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI4,
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_MCTL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_FB_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_CS) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_CLK) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_CMD) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI4_RG_ARPI_BYPASS_CLKIEN));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI4,
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_MCTL_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_FB_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_DQS_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_DQM_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_DQ_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI4_RG_ARPI_BYPASS_DQSIEN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI4,
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_MCTL_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_FB_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_DQS_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_DQM_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_DQ_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI4_RG_ARPI_BYPASS_DQSIEN_B1));

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI1,
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CS_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CLK_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CMD_JUMP_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI1_RG_ARPI_CLKIEN_JUMP_EN));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI1,
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQS_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQM_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQ_JUMP_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI1_RG_ARPI_DQSIEN_JUMP_EN_B0) |
		p_fld(0x0, B0_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI1,
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPISM_MCK_SEL_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_FB_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQS_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQM_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQ_JUMP_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI1_RG_ARPI_DQSIEN_JUMP_EN_B1) |
		p_fld(0x0, B1_DLL_ARPI1_RG_ARPI_OFFSET_DQSIEN_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD4,
		p_fld(CLEAR_FLD, SHU1_CA_CMD4_RG_ARPI_DA_MCK_FB_DL_CA) |
		p_fld(CLEAR_FLD, SHU1_CA_CMD4_RG_ARPI_AA_MCK_FB_DL_CA) |
		p_fld(CLEAR_FLD, SHU1_CA_CMD4_RG_ARPI_AA_MCK_DL_CA));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ4,
		p_fld(CLEAR_FLD, SHU1_B0_DQ4_RG_ARPI_DA_MCK_FB_DL_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ4_RG_ARPI_AA_MCK_FB_DL_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ4_RG_ARPI_AA_MCK_DL_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ4,
		p_fld(CLEAR_FLD, SHU1_B1_DQ4_RG_ARPI_DA_MCK_FB_DL_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ4_RG_ARPI_AA_MCK_FB_DL_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ4_RG_ARPI_AA_MCK_DL_B1));

	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI0,
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_LS_SEL_CA) |
		p_fld(CLEAR_FLD, CA_DLL_ARPI0_RG_ARPI_LS_EN_CA));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI0,
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_LS_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI0_RG_ARPI_LS_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI0,
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARPI_LS_SEL_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI0_RG_ARPI_LS_EN_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD6,
		p_fld(0x2, SHU1_CA_CMD6_RG_ARPI_MIDPI_VTH_SEL_CA) |
		p_fld(cap_sel, SHU1_CA_CMD6_RG_ARPI_CAP_SEL_CA) |
		p_fld(midpicap_sel, SHU1_CA_CMD6_RG_ARPI_MIDPI_CAP_SEL_CA) |
		p_fld(0x0, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ6,
		p_fld(0x0, SHU1_B0_DQ6_RG_ARPI_RESERVE_B0) |
		p_fld(0x2, SHU1_B0_DQ6_RG_ARPI_MIDPI_VTH_SEL_B0) |
		p_fld(cap_sel, SHU1_B0_DQ6_RG_ARPI_CAP_SEL_B0) |
		p_fld(midpicap_sel, SHU1_B0_DQ6_RG_ARPI_MIDPI_CAP_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ6,
		p_fld(0x0, SHU1_B1_DQ6_RG_ARPI_RESERVE_B1) |
		p_fld(0x2, SHU1_B1_DQ6_RG_ARPI_MIDPI_VTH_SEL_B1) |
		p_fld(cap_sel, SHU1_B1_DQ6_RG_ARPI_CAP_SEL_B1) |
		p_fld(midpicap_sel, SHU1_B1_DQ6_RG_ARPI_MIDPI_CAP_SEL_B1));

	io_32_write_fld_multi_all(DDRPHY_PLL1,
		p_fld(SET_FLD, PLL1_RG_RPHYPLL_EN) |
		p_fld(SET_FLD, PLL1_R_DMPHYCLKP0ENB));

	/* 1. PLL enable */
	io_32_write_fld_align_all(DDRPHY_PLL2, SET_FLD, PLL2_RG_RCLRPLL_EN);

	delay_us(30);

	/* 2. SSC enable */
	if (p->ssc_en == ENABLE) {
		io_32_write_fld_align_all(DDRPHY_PLL5, SET_FLD,
			PLL5_RG_RPHYPLL_SDM_SSC_EN);
		io_32_write_fld_align_all(DDRPHY_PLL7, SET_FLD,
			PLL7_RG_RCLRPLL_SDM_SSC_EN);
	}

	/* 3. PI RESET release */
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI0, SET_FLD,
		CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI0, SET_FLD,
		B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI0, SET_FLD,
		B1_DLL_ARPI0_RG_ARPI_RESETB_B1);

	delay_us(1);

	if (p->frequency >= DDR1600_FREQ) {
		/* 4. DCDL enable */
		io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
			SET_FLD, CA_CKGEN_DLL1_RG_CKGEN_DCDL_EN_CA);
		io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
			SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DCDL_EN_B01);

		/*5. CKGENDLL enable. AUTO mode */
		io_32_write_fld_align_all(DDRPHY_B0_CKGEN_DLL1,
			SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_B01);
		io_32_write_fld_align_all(DDRPHY_CA_CKGEN_DLL1,
			SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_EN_CA);
	}

	/*
	 * MCK8X_EN>0us, RESETB>10ns
	 * io_32_write_fld_multi_all(DDRPHY_PLL4,
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_ADA_MCK8X_EN) |
	 * | p_fld(SET_FLD, PLL4_RG_RPHYPLL_RESETB));
	 */
	io_32_write_fld_multi_all(DDRPHY_PLL4,
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_MCK8X_EN) |
		p_fld(SET_FLD, PLL4_RG_RPHYPLL_RESETB));

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_MISC_CTRL1, SET_FLD,
		MISC_CTRL1_R_DMPHYRST);
	io_32_write_fld_align_all(DRAMC_REG_DDRCONF0, SET_FLD,
		DDRCONF0_DMSW_RST);

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD6,
		p_fld(CLEAR_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA) |
		p_fld(SET_FLD, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0) |
		p_fld(SET_FLD, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1) |
		p_fld(SET_FLD, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1));

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI2, CLEAR_FLD,
		CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA);
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI2,
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0) |
		p_fld(CLEAR_FLD, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI2,
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_CG_FB_B1) |
		p_fld(CLEAR_FLD, B1_DLL_ARPI2_RG_ARPI_MPDIV_CG_B1));

	/* 6. PI enable */
	io_32_write_fld_multi_all(DDRPHY_CA_DLL_ARPI3,
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_MCTL_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_FB_EN_CA) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CS_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CLK_EN) |
		p_fld(SET_FLD, CA_DLL_ARPI3_RG_ARPI_CMD_EN));
	io_32_write_fld_multi_all(DDRPHY_B0_DLL_ARPI3,
		p_fld(CLEAR_FLD, B0_DLL_ARPI3_RG_ARPI_MCTL_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_FB_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQS_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQM_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQ_EN_B0) |
		p_fld(SET_FLD, B0_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DLL_ARPI3,
		p_fld(CLEAR_FLD, B1_DLL_ARPI3_RG_ARPI_MCTL_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_FB_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQS_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQM_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQ_EN_B1) |
		p_fld(SET_FLD, B1_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B1));

	io_32_write_fld_align_all(DDRPHY_MISC_CTRL1, 0,
		MISC_CTRL1_R_DMPHYRST);
	io_32_write_fld_align_all(DRAMC_REG_DDRCONF0, 0, DDRCONF0_DMSW_RST);
	io_32_write_fld_align_all(DDRPHY_PLL1, 0, PLL1_R_DMPHYCLKP0ENB);

	delay_us(1);

	/* 7. Master DLL enable */
	io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI5, SET_FLD,
		CA_DLL_ARPI5_RG_ARDLL_PHDET_EN_CA);
	delay_us(1);

	/* 8. Slave DLL enable */
	io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI5, SET_FLD,
		B0_DLL_ARPI5_RG_ARDLL_PHDET_EN_B0);
	delay_us(1);
	io_32_write_fld_align_all(DDRPHY_B1_DLL_ARPI5, SET_FLD,
		B1_DLL_ARPI5_RG_ARDLL_PHDET_EN_B1);

	delay_us(1);

	set_dram_type(p);
}

static void update_initial_setting_common(DRAMC_CTX_T *p)
{
	unsigned char vref_sel;

	if (p->dram_type == TYPE_PCDDR4) {
		io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
			p_fld(SET_FLD, B0_DQ3_RG_BURST_TRACK_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_DQSIENMODE) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
			p_fld(SET_FLD, B1_DQ3_RG_BURST_TRACK_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBENCMP_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_DQSIENMODE) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM1_IN_BUFF_EN) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_IN_BUFF_EN) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1));

		io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_O1_SEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_O1_SEL_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_SER_MODE_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1));
		io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DMRANK_OUTSEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DDR4_SEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_OE_EXT_DIS) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_BIAS_PS));
	} else if (p->dram_type == TYPE_PCDDR3) {
		io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
			p_fld(CLEAR_FLD, B0_DQ3_RG_BURST_TRACK_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQS0_DQSIENMODE) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
			p_fld(CLEAR_FLD, B1_DQ3_RG_BURST_TRACK_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBENCMP_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQS1_DQSIENMODE) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM1_IN_BUFF_EN) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_IN_BUFF_EN) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1));
		io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_O1_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_O1_SEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_TX_ARDQ_SER_MODE_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1));
		io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DMRANK_OUTSEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DDR4_SEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_O1_SEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_OE_EXT_DIS) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_BIAS_PS));
	} else {
		/* LPDDR3 */
		io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
			p_fld(CLEAR_FLD, B0_DQ3_RG_BURST_TRACK_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQS0_DQSIENMODE) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN) |
			p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
			p_fld(CLEAR_FLD, B1_DQ3_RG_BURST_TRACK_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQS0_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBENCMP_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQS1_DQSIENMODE) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM1_IN_BUFF_EN) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_IN_BUFF_EN) |
			p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1));
		io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_O1_SEL_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0) |
			p_fld(CLEAR_FLD, B0_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B0) |
			p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DMRANK_OUTSEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_O1_SEL_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_TX_ARDQ_SER_MODE_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B1) |
			p_fld(CLEAR_FLD, B1_DQ6_RG_TX_ARDQ_OE_EXT_DIS_B1) |
			p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1));
		io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DMRANK_OUTSEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DDR4_SEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_O1_SEL) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS) |
			p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_OE_EXT_DIS) |
			p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_BIAS_PS));
	}

	io_32_write_fld_multi_all(DDRPHY_B0_DQ2,
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_ODTEN_DIS_B0) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQM0_ODTEN_DIS) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_ODTEN_DIS) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_OE_EXT_DIS) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_ODTEN_EXT_DIS));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ2,
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQ_ODTEN_DIS_B1) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQM1_ODTEN_DIS) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS1_ODTEN_DIS) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS0_OE_EXT_DIS) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS0_ODTEN_EXT_DIS));

	io_32_write_fld_align_all(DDRPHY_B0_DQ8, 0, B0_DQ8_RG_T2RLPBK_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ8, 0, B1_DQ8_RG_T2RLPBK_B1);

	io_32_write_fld_align_all(DDRPHY_B0_DQ5, 1,
		B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, 1,
		B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);


	if (p->dram_type == TYPE_PCDDR3 || p->dram_type == TYPE_LPDDR3)
		vref_sel = 0x12;
	else
		vref_sel = 0xf; /* DDR4 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ5,
		p_fld(CLEAR_FLD, SHU1_B0_DQ5_RG_RX_ARDQS0_DQSIEN_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_BYPASS_B0) |
		p_fld(vref_sel, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ5,
		p_fld(CLEAR_FLD, SHU1_B1_DQ5_RG_RX_ARDQS0_DQSIEN_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_BYPASS_B1) |
		p_fld(vref_sel, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1));

	io_32_write_fld_align_all(DRAMC_REG_SHU_SCINTV, 0x1f,
		SHU_SCINTV_SCINTV);
	io_32_write_fld_multi_all(DRAMC_REG_CLKAR,
		p_fld(SET_FLD, CLKAR_SELPH_CMD_CG_DIS) |
		p_fld(0x7FFF, CLKAR_REQQUE_PACG_DIS));
	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
		SPCMDCTRL_REFR_BLOCKEN);
	io_32_write_fld_align_all(DRAMC_REG_PERFCTL0, CLEAR_FLD,
		PERFCTL0_WRFFIFO_OPT);
	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_REORDEREN) |
		p_fld(CLEAR_FLD, PERFCTL0_RWHPRICTL));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ7,
		p_fld(SET_FLD, B0_DQ7_RG_RX_ARDQS0_LP4_BURSTMODE_SEL_B01) |
		p_fld(SET_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RB_EXT_EN_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_SER_RST_EXT_EN_B01) |
		p_fld(CLEAR_FLD, B0_DQ7_RG_RX_ARDQS0_DQSIEN_RB_DLY_B01));
}

static DRAM_STATUS_T ddrphy_setting_common(DRAMC_CTX_T *p)
{
	unsigned char pinmux[4] = {0};

	io_32_write_fld_multi_all(DRAMC_REG_CLKCTRL,
		p_fld(SET_FLD, CLKCTRL_REG_CLK_1) |
		p_fld(SET_FLD, CLKCTRL_REG_CLK_0) |
		p_fld(0xc, CLKCTRL_PSEL_CNT));

	io_32_write_fld_multi_all(DDRPHY_CA_CMD5,
		p_fld(0xe, CA_CMD5_RG_RX_ARCMD_EYE_VREF_SEL));
	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD5,
		p_fld(CLEAR_FLD, SHU1_CA_CMD5_RG_ARPI_FB_CA) |
		p_fld(0xe, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL));

	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL1, CLEAR_FLD,
		MISC_SPM_CTRL1_PHY_SPM_CTL1);
	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL0, 0xffffffff,
		MISC_SPM_CTRL0_PHY_SPM_CTL0);
	io_32_write_fld_align_all(DDRPHY_MISC_SPM_CTRL2, 0xffffffff,
		MISC_SPM_CTRL2_PHY_SPM_CTL2);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL2, 0x6003bf,
		MISC_CG_CTRL2_RG_MEM_DCM_CTL);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL4, 0x13300000,
		MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);

	/* cc add */
	if (p->frequency >= DDR1600_FREQ) {
		io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
		io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
	} else {
		/* DIV4 mode fore low freq (i.e, 1066M data rate) */
		io_32_write_fld_align_all(DDRPHY_CA_DLL_ARPI0,
			SET_FLD, CA_DLL_ARPI0_RG_ARPI_SMT_EN_CA);

		io_32_write_fld_align_all(DDRPHY_B0_DLL_ARPI0,
			SET_FLD, B0_DLL_ARPI0_RG_ARPI_SMT_EN_B01);

		io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL0,
			p_fld(0x20, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_B01) |
			p_fld(0x3, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_B01) |
			p_fld(0x8, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_B01) |
			p_fld(0x7, B0_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_B01));
		io_32_write_fld_multi_all(DDRPHY_B0_CKGEN_DLL1,
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_FD_EN_B01) |
			p_fld(0xf, B0_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_B01) |
			p_fld(0x1, B0_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_B01) |
			p_fld(0x0, B0_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_B01) |
			p_fld(SET_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_B01) |
			p_fld(CLEAR_FLD, B0_CKGEN_DLL1_RG_ARCKGEN_PD_EN_B01));
		io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL0,
			p_fld(0x20, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_RESERVE_CA) |
			p_fld(0x3, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_OUT_SEL_CA) |
			p_fld(0x8, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_P_GAIN_CA) |
			p_fld(0x7, CA_CKGEN_DLL0_RG_ARCKGEN_DLL_IDLECNT_CA));
		io_32_write_fld_multi_all(DDRPHY_CA_CKGEN_DLL1,
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_FD_EN_CA) |
			p_fld(0xf, CA_CKGEN_DLL1_RG_ARCKGEN_PI_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_ONLINE_CAL_CA) |
			p_fld(0x1, CA_CKGEN_DLL1_RG_ARCKGEN_PI_EXT_CAP_SEL_CA) |
			p_fld(0x0, CA_CKGEN_DLL1_RG_ARCKGEN_DLL_FAST_PSPJ_CA) |
			p_fld(SET_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_CKDIV4_EN_CA) |
			p_fld(CLEAR_FLD, CA_CKGEN_DLL1_RG_ARCKGEN_PD_EN_CA));
	}

	if (p->dram_type == TYPE_PCDDR4) {
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD7,
			p_fld(CLEAR_FLD, SHU1_CA_CMD7_RG_ARCMD_REV) |
			p_fld(SET_FLD, SHU1_CA_CMD7_R_DMRANKRXDVS_CA));
	} else if (p->dram_type == TYPE_PCDDR3) {
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD7,
			p_fld(0x1, SHU1_CA_CMD7_RG_ARCMD_REV) |
			p_fld(SET_FLD, SHU1_CA_CMD7_R_DMRANKRXDVS_CA));
		io_32_write_fld_align_all(DDRPHY_B0_DQ9, 0x1,
			B0_DQ9_RG_TX_ARDQ_RESERVE_B01);
	} else {
		/* lp3 */
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD7,
			p_fld(0x30C0, SHU1_CA_CMD7_RG_ARCMD_REV) |
			p_fld(SET_FLD, SHU1_CA_CMD7_R_DMRANKRXDVS_CA));
	}

	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ7,
		p_fld(SET_FLD, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE) |
		p_fld(SET_FLD, SHU1_B0_DQ7_R_DMRANKRXDVS_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ7,
		p_fld(SET_FLD, SHU1_B1_DQ7_R_DMRANKRXDVS_B1));

	io_32_write_fld_multi_all(DDRPHY_CA_CMD2,
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_MCK4XB_DLY_EN) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_MCK4X_DLY_EN) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCMD_OE_DIS) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCMD_ODTEN_DIS) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_OE_DIS));
	io_32_write_fld_align_all(DDRPHY_SEL_MUX0,
		0xfff, SEL_MUX0_SEL_MUX0);
	io_32_write_fld_multi_all(DDRPHY_B0_DQ2,
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_OE_DIS_B0) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_ODTEN_DIS_B0) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_OE_DIS) |
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQS0_ODTEN_DIS));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ2,
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQ_OE_DIS_B1) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQ_ODTEN_DIS_B1) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS1_OE_DIS) |
		p_fld(CLEAR_FLD, B1_DQ2_RG_TX_ARDQS1_ODTEN_DIS));
	io_32_write_fld_align_all(DDRPHY_MISC_RXDVS1, 0x7,
		MISC_RXDVS1_R_IN_GATE_EN_LOW_OPT);
	io_32_write_fld_align_all(DDRPHY_PLL3, CLEAR_FLD,
		PLL3_RG_RPHYPLL_TSTOP_EN);
	io_32_write_fld_multi_all(DDRPHY_MISC_VREF_CTRL,
		p_fld(SET_FLD, MISC_VREF_CTRL_RG_RVREF_VREF_EN) |
		p_fld(0xe, MISC_VREF_CTRL_RG_RVREF_SEL_CMD) |
		p_fld(SET_FLD, MISC_VREF_CTRL_RG_RVREF_DDR3_SEL) |
		p_fld(CLEAR_FLD, MISC_VREF_CTRL_RG_RVREF_DDR4_SEL) |
		p_fld(0xe, MISC_VREF_CTRL_RG_RVREF_SEL_DQ));
	io_32_write_fld_multi_all(DDRPHY_MISC_IMP_CTRL0,
		p_fld(0x2e, MISC_IMP_CTRL0_RG_RIMP_VREF_SEL) |
		p_fld(SET_FLD, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL) |
		p_fld(CLEAR_FLD, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ5,
		p_fld(0x5, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_B0_DQ5_RG_ARPI_FB_B0) |
		p_fld(0x12, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ5,
		p_fld(0x5, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_B1_DQ5_RG_ARPI_FB_B1) |
		p_fld(0x12, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1));

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_B0_DQ3, SET_FLD,
		B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ3, SET_FLD,
		B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL1, 0x3f600,
		MISC_CG_CTRL1_R_DVS_DIV4_CG_CTRL);

	io_32_write_fld_multi_all(DDRPHY_B0_DQ4,
		p_fld(0x10, B0_DQ4_RG_RX_ARDQS_EYE_R_DLY_B0) |
		p_fld(0x10, B0_DQ4_RG_RX_ARDQS_EYE_F_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ5,
		p_fld(CLEAR_FLD, B0_DQ5_RG_RX_ARDQ_EYE_EN_B0) |
		p_fld(SET_FLD, B0_DQ5_RG_RX_ARDQ_EYE_SEL_B0) |
		p_fld(SET_FLD, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0) |
		p_fld(0xe, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ6,
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0) |
		p_fld(CLEAR_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0));
	io_32_write_fld_align_all(DDRPHY_B0_DQ5, SET_FLD,
		B0_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B0);
	io_32_write_fld_multi_all(DDRPHY_B1_DQ4,
		p_fld(0x10, B1_DQ4_RG_RX_ARDQS_EYE_R_DLY_B1) |
		p_fld(0x10, B1_DQ4_RG_RX_ARDQS_EYE_F_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ5,
		p_fld(CLEAR_FLD, B1_DQ5_RG_RX_ARDQ_EYE_EN_B1) |
		p_fld(SET_FLD, B1_DQ5_RG_RX_ARDQ_EYE_SEL_B1) |
		p_fld(SET_FLD, B1_DQ5_RG_RX_ARDQ_VREF_EN_B1) |
		p_fld(0xe, B1_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B1));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ6,
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B1) |
		p_fld(CLEAR_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_EN_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B1));
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, SET_FLD,
		B1_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B1);
	io_32_write_fld_multi_all(DDRPHY_CA_CMD3,
		p_fld(0x7fffff, SEL_MUX1) |
		p_fld(SET_FLD, CA_CMD3_RG_ARCMD_RESETB) |
		p_fld(SET_FLD, CA_CMD3_RG_TX_ARCMD_EN) |
		p_fld(CLEAR_FLD, CA_CMD3_RG_RX_ARCMD_SMT_EN) |
		p_fld(CLEAR_FLD, CA_CMD3_RG_ARCMD_ATPG_EN));
	io_32_write_fld_multi_all(DDRPHY_CA_CMD6,
		p_fld(SET_FLD, CA_CMD6_RG_TX_ARCMD_DDR3_SEL) |
		p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_DDR4_SEL) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_DDR4_SEL) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_BIAS_VREF_SEL) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN) |
		p_fld(CLEAR_FLD, CA_CMD6_RG_TX_ARCMD_SER_MODE));
	if (p->dram_type != TYPE_PCDDR4) {
		io_32_write_fld_align_all(DDRPHY_CA_CMD6,
			SET_FLD, CA_CMD6_RG_TX_ARCMD_SER_MODE);
	}

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
		p_fld(SET_FLD, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
		p_fld(SET_FLD, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));
	io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_ARDQ_RESETB_B0) |
		p_fld(SET_FLD, B0_DQ3_RG_TX_ARDQ_EN_B0) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));
	io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBENCMP_EN_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_ARDQ_RESETB_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_TX_ARDQ_EN_B1) |
		p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1));
	io_32_write_fld_multi_all(DDRPHY_CA_CMD8,
		p_fld(SET_FLD, CA_CMD8_RG_TX_RRESETB_DDR3_SEL) |
		p_fld(SET_FLD, CA_CMD8_RG_RRESETB_DRVN) |
		p_fld(SET_FLD, CA_CMD8_RG_RRESETB_DRVP));
	io_32_write_fld_multi_all(DDRPHY_MISC_IMP_CTRL1,
		p_fld(CLEAR_FLD, MISC_IMP_CTRL1_RG_RIMP_REV) |
		p_fld(CLEAR_FLD, MISC_IMP_CTRL1_RG_RIMP_PRE_EN));
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL4, 0x11400000,
		MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);
	io32_write_4b_all(DDRPHY_MISC_SHU_OPT, 0xfff0f0f0);
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL0, 0x1f,
		MISC_CG_CTRL0_CLK_MEM_DFS_CFG);

	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD1,
		p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN_B) |
		p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP_B));
	io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
		p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
		p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));

	if (p->dram_type == TYPE_PCDDR3) {
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ1,
			p_fld(0x4, SHU1_B0_DQ1_RG_TX_ARDQ_ODTN_B0) |
			p_fld(0x4, SHU1_B0_DQ1_RG_TX_ARDQ_ODTP_B0) |
			p_fld(0xc, SHU1_B0_DQ1_RG_TX_ARDQ_DRVN_B_B0) |
			p_fld(0xc, SHU1_B0_DQ1_RG_TX_ARDQ_DRVP_B_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ2,
			p_fld(0x4, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTN_B0) |
			p_fld(0x4, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTP_B0) |
			p_fld(0xc, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVN_B0) |
			p_fld(0xc, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVP_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ1,
			p_fld(0x4, SHU1_B1_DQ1_RG_TX_ARDQ_ODTN_B1) |
			p_fld(0x4, SHU1_B1_DQ1_RG_TX_ARDQ_ODTP_B1) |
			p_fld(0xc, SHU1_B1_DQ1_RG_TX_ARDQ_DRVN_B1) |
			p_fld(0xc, SHU1_B1_DQ1_RG_TX_ARDQ_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ2,
			p_fld(0x4, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTN_B1) |
			p_fld(0x4, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTP_B1) |
			p_fld(0xc, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVN_B1) |
			p_fld(0xc, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD1,
			p_fld(0xC, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN_B) |
			p_fld(0xC, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP_B));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
			p_fld(0xC, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
			p_fld(0xC, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));
	} else if (p->dram_type == TYPE_PCDDR4) {
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ1,
			p_fld(0x5, SHU1_B0_DQ1_RG_TX_ARDQ_ODTN_B0) |
			p_fld(0x5, SHU1_B0_DQ1_RG_TX_ARDQ_ODTP_B0) |
			p_fld(0xe, SHU1_B0_DQ1_RG_TX_ARDQ_DRVN_B_B0) |
			p_fld(0xd, SHU1_B0_DQ1_RG_TX_ARDQ_DRVP_B_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ2,
			p_fld(0x5, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTN_B0) |
			p_fld(0x5, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTP_B0) |
			p_fld(0xe, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVN_B0) |
			p_fld(0xd, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVP_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ1,
			p_fld(0x5, SHU1_B1_DQ1_RG_TX_ARDQ_ODTN_B1) |
			p_fld(0x5, SHU1_B1_DQ1_RG_TX_ARDQ_ODTP_B1) |
			p_fld(0xe, SHU1_B1_DQ1_RG_TX_ARDQ_DRVN_B1) |
			p_fld(0xd, SHU1_B1_DQ1_RG_TX_ARDQ_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ2,
			p_fld(0x5, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTN_B1) |
			p_fld(0x5, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTP_B1) |
			p_fld(0xe, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVN_B1) |
			p_fld(0xd, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD1,
			p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN_B) |
			p_fld(0xE, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP_B));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
			p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
			p_fld(0xE, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));
	} else {
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ1,
			p_fld(0x5, SHU1_B0_DQ1_RG_TX_ARDQ_ODTN_B0) |
			p_fld(0x5, SHU1_B0_DQ1_RG_TX_ARDQ_ODTP_B0) |
			p_fld(0xe, SHU1_B0_DQ1_RG_TX_ARDQ_DRVN_B_B0) |
			p_fld(0xd, SHU1_B0_DQ1_RG_TX_ARDQ_DRVP_B_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B0_DQ2,
			p_fld(0x5, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTN_B0) |
			p_fld(0x5, SHU1_B0_DQ2_RG_TX_ARDQS0_ODTP_B0) |
			p_fld(0xe, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVN_B0) |
			p_fld(0xd, SHU1_B0_DQ2_RG_TX_ARDQS0_DRVP_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ1,
			p_fld(0x5, SHU1_B1_DQ1_RG_TX_ARDQ_ODTN_B1) |
			p_fld(0x5, SHU1_B1_DQ1_RG_TX_ARDQ_ODTP_B1) |
			p_fld(0xe, SHU1_B1_DQ1_RG_TX_ARDQ_DRVN_B1) |
			p_fld(0xd, SHU1_B1_DQ1_RG_TX_ARDQ_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_B1_DQ2,
			p_fld(0x5, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTN_B1) |
			p_fld(0x5, SHU1_B1_DQ2_RG_TX_ARDQS0_ODTP_B1) |
			p_fld(0xe, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVN_B1) |
			p_fld(0xd, SHU1_B1_DQ2_RG_TX_ARDQS0_DRVP_B1));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD1,
			p_fld(0xC, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN_B) |
			p_fld(0xC, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP_B));
		io_32_write_fld_multi_all(DDRPHY_SHU1_CA_CMD2,
			p_fld(0xC, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN) |
			p_fld(0xC, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));
	}

	ddrphy_pll_setting_common(p);

	io_32_write_fld_align_all(DDRPHY_B0_DQ3, SET_FLD,
		B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB);
	io_32_write_fld_align_all(DDRPHY_B1_DQ3, SET_FLD,
		B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB);

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL1,
		p_fld(SET_FLD, MISC_CTRL1_R_DMDA_RRESETB_E) |
		p_fld(SET_FLD, MISC_CTRL1_R_DMDQSIENCG_EN) |
		p_fld(SET_FLD, MISC_CTRL1_R_DMARPIDQ_SW) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCMD_OE) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCLK_OE));

	io_32_write_fld_align_all(DDRPHY_CA_CMD8, CLEAR_FLD,
		CA_CMD8_RG_TX_RRESETB_PULL_DN);
	io_32_write_fld_multi_all(DDRPHY_CA_CMD7,
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARRESETB_PULL_DN) |
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARCMD_D0_PULL_UP) |
		p_fld(CLEAR_FLD, CA_CMD7_RG_TX_ARCMD_D0_PULL_DN));
	io_32_write_fld_align_all(DDRPHY_B0_DQ7, CLEAR_FLD,
		B0_DQ7_RG_TX_ARDQ_PULL_DN_B0);

	/* cc add for DDR4 gating for 8UI mode */
	if (p->dram_type == TYPE_PCDDR4) {
		io_32_write_fld_multi_all(DDRPHY_B0_DQ7,
			p_fld(SET_FLD,
				B0_DQ7_RG_RX_ARDQS0_LP4_BURSTMODE_SEL_B01) |
			p_fld(0x1, B0_DQ7_RG_RX_ARDQS0_BURST_EN_B01));
	}

	io_32_write_fld_align_all(DDRPHY_B1_DQ7, CLEAR_FLD,
		B1_DQ7_RG_TX_ARDQ_PULL_DN_B1);

#if 0
	if (p->en_4bit_mux == ENABLE) {
		io_32_write_fld_align_all(DRAMC_REG_DDRCONF0, SET_FLD,
			DDRCONF0_DQ4BMUX);
		io_32_write_fld_align_all(DDRPHY_B0_DQ3, 0x00,
			B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0);
		io_32_write_fld_align_all(DDRPHY_B1_DQ3, 0xff,
			B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1);

		io_32_write_fld_multi_all(DDRPHY_B0_DQ2,
			p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0) |
			p_fld(0x00, B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ2,
			p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1) |
			p_fld(0xff, B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1));
	} else {
		io_32_write_fld_multi_all(DDRPHY_B0_DQ3,
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ3,
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
			p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1));

		io_32_write_fld_multi_all(DDRPHY_B0_DQ2,
			p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0) |
			p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0));
		io_32_write_fld_multi_all(DDRPHY_B1_DQ2,
			p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1) |
			p_fld(0xff, B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1));
	}
#endif

	switch (p->dram_type) {
	case TYPE_PCDDR3:
	case TYPE_PCDDR4:
		pinmux[0] = 0x0;
		pinmux[1] = 0xff;
		pinmux[2] = 0x0;
		pinmux[3] = 0xff;
		break;

	case TYPE_LPDDR3:
		pinmux[0] = 0x0f;
		pinmux[1] = 0xf0;
		pinmux[2] = 0x0f;
		pinmux[3] = 0xf0;

	default:
		break;
	}

	/* B0 RX PINMUX*/
	io_32_write_fld_multi(DDRPHY_B0_DQ3,
		p_fld(pinmux[0], B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0));

	/* B1 */
	io_32_write_fld_multi(DDRPHY_B1_DQ3,
		p_fld(pinmux[1],B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1));

	/* B23 */
	io_32_write_fld_multi(DDRPHY_B0_DQ3 + (1 << POS_BANK_NUM),
		p_fld(pinmux[2], B0_DQ3_RG_RX_ARDQ_DQSI_SEL_B0) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQM0_DQSI_SEL_B0));
	io_32_write_fld_multi(DDRPHY_B1_DQ3 + (1 << POS_BANK_NUM),
		p_fld(pinmux[3], B1_DQ3_RG_RX_ARDQ_DQSI_SEL_B1) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQM0_DQSI_SEL_B1));

	/* B0 TX PINMUX */
	io_32_write_fld_multi(DDRPHY_B0_DQ2,
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0) |
		p_fld(pinmux[0], B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0));
	io_32_write_fld_multi(DDRPHY_B1_DQ2,
		p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1) |
		p_fld(pinmux[1], B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1));

	/* B23 */
	io_32_write_fld_multi(DDRPHY_B0_DQ2 + (1 << POS_BANK_NUM),
		p_fld(CLEAR_FLD, B0_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B0) |
		p_fld(pinmux[2], B0_DQ2_RG_TX_ARDQ_MCK4X_SEL_B0));
	io_32_write_fld_multi(DDRPHY_B1_DQ2 + (1 << POS_BANK_NUM),
		p_fld(SET_FLD, B1_DQ2_RG_TX_ARDQM0_MCK4X_SEL_B1) |
		p_fld(pinmux[3], B1_DQ2_RG_TX_ARDQ_MCK4X_SEL_B1));

	if ((p->dram_type == TYPE_LPDDR3) &&
		(p->en_4bit_mux == ENABLE)) {
		/* LP3 uses 4BIT mux */
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1),
			0x1, MISC_CTRL1_R_DMDQ4BITMUX_NEW);
	}

	update_initial_setting_common(p);

	dramc_sw_impedance_save_register(p, p->odt_onoff,
		p->odt_onoff, DRAM_DFS_SHUFFLE_1);

    return DRAM_OK;
}

#endif /* SUPPORT_TYPE_LPDDR3 || PCDDR3 || PCDDR4 */


#if SUPPORT_TYPE_LPDDR3
static DRAM_STATUS_T dramc_setting_default_lp3(DRAMC_CTX_T *p)
{
	unsigned char reg_txdly_dqs = 0, reg_txdly_dqs_oen = 0;
	unsigned char reg_txdly_dqdqm = 0, reg_txdly_dqdqm_oen = 0;
	unsigned char reg_dly_dqs = 0, reg_dly_dqs_oen = 0;
	unsigned char reg_dly_dqdqm = 0, reg_dly_dqdqm_oen = 0;

	if (p->freq_sel == DDR_DDR1600) {
		reg_txdly_dqs = 0x2;
		reg_txdly_dqs_oen = 0x2;
		reg_dly_dqs = 0x3;
		reg_dly_dqs_oen = 0x1;

		reg_txdly_dqdqm = 0x2;
		reg_txdly_dqdqm_oen = 0x2;
		reg_dly_dqdqm = 0x3;
		reg_dly_dqdqm_oen = 0x1;
	} else if (p->freq_sel == DDR_DDR1866) {
		reg_txdly_dqs = 0x3;
		reg_txdly_dqs_oen = 0x3;
		reg_dly_dqs = 0x3;
		reg_dly_dqs_oen = 0x1;

		reg_txdly_dqdqm = 0x3;
		reg_txdly_dqdqm_oen = 0x3;
		reg_dly_dqdqm = 0x3;
		reg_dly_dqdqm_oen = 0x1;
	} else { //1066
		reg_txdly_dqs = 0x1;
		reg_txdly_dqs_oen = 0x1;
		reg_dly_dqs = 0x3;
		reg_dly_dqs_oen = 0x1;

		reg_txdly_dqdqm = 0x1;
		reg_txdly_dqdqm_oen = 0x1;
		reg_dly_dqdqm = 0x3;
		reg_dly_dqdqm_oen = 0x1;
	}

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA1,
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_WE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RESET) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_ODT) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CKE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA2,
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CKE1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CMD) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA3,
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA7) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA6) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA5) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA4) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA3) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA4,
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA15) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA14) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA13) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA12) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA11) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA10) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA9) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA8));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA5,
		p_fld(CLEAR_FLD, SHU_SELPH_CA5_DLY_ODT) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CKE) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CS));
	io_32_write_fld_align_all(DRAMC_REG_SHU_SELPH_CA6, CLEAR_FLD,
		SHU_SELPH_CA6_DLY_CMD);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA7,
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA7) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA6) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA5) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA4) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA3) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA2) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA1) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA8,
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA15) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA14) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA13) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA12) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA11) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA10) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA9) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA8));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
		p_fld(0xf, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(0x0, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CLK) |
		p_fld(0xf, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CMD) |
		p_fld(0x2, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CS));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ7_RK1_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ7_RK1_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TX_DLY_RANK_MCK) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(16 - 4, SHU_CONF1_DATLAT_DSEL_PHY) |
		p_fld(16 - 4, SHU_CONF1_DATLAT_DSEL) |
		p_fld(16, SHU_CONF1_DATLAT));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
		p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld(0x2, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1) |
		p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1) |
		p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
		p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
		p_fld(0x7, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
		p_fld(0x2, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED) |
		p_fld(0x2, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED) |
		p_fld(0x2, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
		p_fld(0x2, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
		p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED_P1) |
		p_fld(0x7, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED_P1) |
		p_fld(0x7, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
		p_fld(0x7, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
		p_fld(0x7, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(10, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(10, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(10, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(10, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(10, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ2,
		p_fld(10, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_R_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ3,
		p_fld(10, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_R_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ4,
		p_fld(10, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_R_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ5,
		p_fld(10, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_R_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_F_DLY_B0) |
		p_fld(10, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(10, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(10, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(10, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(10, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(10, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ2,
		p_fld(10, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_R_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ3,
		p_fld(10, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_R_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ4,
		p_fld(10, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_R_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ5,
		p_fld(10, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_R_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_F_DLY_B1) |
		p_fld(10, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_R_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_R_DLY_B1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x3, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(SET_FLD, SHU_RANKCTL_RANKINCTL_ROOT1) |
		p_fld(SET_FLD, SHU_RANKCTL_RANKINCTL) |
		p_fld(SET_FLD, SHU_RANKCTL_DMSTBLAT));

	io_32_write_fld_align_all(DRAMC_REG_SHURK0_DQSCTL, 0x3,
		SHURK0_DQSCTL_DQSINCTL);
	io_32_write_fld_align_all(DRAMC_REG_SHURK1_DQSCTL, 0x3,
		SHURK1_DQSCTL_R1DQSINCTL);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM0,
		p_fld(0x9, SHU_ACTIM0_TRCD) |
		p_fld(0x3, SHU_ACTIM0_TRRD) |
		p_fld(0xb, SHU_ACTIM0_TWR) |
		p_fld(0x6, SHU_ACTIM0_TWTR));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM1,
		p_fld(0x14, SHU_ACTIM1_TRC) |
		p_fld(0x9, SHU_ACTIM1_TRAS) |
		p_fld(0x9, SHU_ACTIM1_TRP) |
		p_fld(SET_FLD, SHU_ACTIM1_TRPAB));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM2,
		p_fld(0xc, SHU_ACTIM2_TFAW) |
		p_fld(0x5, SHU_ACTIM2_TR2W) |
		p_fld(0x2, SHU_ACTIM2_TRTP));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM3,
		p_fld(0x49, SHU_ACTIM3_TRFC) |
		p_fld(0x19, SHU_ACTIM3_TRFCPB));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM4,
		p_fld(0x23, SHU_ACTIM4_TZQCS) |
		p_fld(0x65, SHU_ACTIM4_REFCNT_FR_CLK) |
		p_fld(0x60, SHU_ACTIM4_TXREFCNT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM5,
		p_fld(CLEAR_FLD, SHU_ACTIM5_TMRR2W) |
		p_fld(0x9, SHU_ACTIM5_TWTPD) |
		p_fld(0x8, SHU_ACTIM5_TR2PD));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM_XRT,
		p_fld(0x8, SHU_ACTIM_XRT_XRTW2W) |
		p_fld(SET_FLD, SHU_ACTIM_XRT_XRTW2R) |
		p_fld(0x4, SHU_ACTIM_XRT_XRTR2W) |
		p_fld(0xc, SHU_ACTIM_XRT_XRTR2R));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_AC_TIME_05T,
		p_fld(SET_FLD, SHU_AC_TIME_05T_TWTR_M05T) |
		p_fld(SET_FLD, SHU_AC_TIME_05T_TR2W_05T) |
		p_fld(SET_FLD, SHU_AC_TIME_05T_TWR_M05T));

    return DRAM_OK;
}

static DRAM_STATUS_T ddrphy_setting_lp3(DRAMC_CTX_T *p)
{
	return ddrphy_setting_common(p);
}

static DRAM_STATUS_T dramc_setting_lp3(DRAMC_CTX_T *p)
{
	ddrphy_setting_lp3(p);

	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(CLEAR_FLD, SHUCTRL2_HWSET_WLRL) |
		p_fld(SET_FLD, SHUCTRL2_SHU_PERIOD_GO_ZERO_CNT) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_OPTION) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_PARK_N) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_DLL_CHA) |
		p_fld(0x7, SHUCTRL2_R_DLL_IDLE));
	io_32_write_fld_multi_all(DRAMC_REG_DVFSDLL,
		p_fld(0xe, DVFSDLL_DLL_IDLE_SHU4) |
		p_fld(0x9, DVFSDLL_DLL_IDLE_SHU3) |
		p_fld(0x6, DVFSDLL_DLL_IDLE_SHU2) |
		p_fld(SET_FLD, DVFSDLL_DLL_LOCK_SHU_EN));
	if (p->data_width == DATA_WIDTH_16BIT) {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_LPDDR3EN) |
			p_fld(CLEAR_FLD, DDRCONF0_DM64BITEN) |
			p_fld(SET_FLD, DDRCONF0_DM8BKEN));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_LPDDR3EN) |
			p_fld(SET_FLD, DDRCONF0_DM64BITEN) |
			p_fld(SET_FLD, DDRCONF0_DM8BKEN));
	}
	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_STB_GERR_B01) |
		p_fld(SET_FLD, EYESCAN_STB_GERRSTOP));
	io_32_write_fld_align_all(DRAMC_REG_EYESCAN, SET_FLD,
		EYESCAN_STB_GERR_RST);
	io_32_write_fld_align_all(DRAMC_REG_EYESCAN, CLEAR_FLD,
		EYESCAN_STB_GERR_RST);
	io_32_write_fld_align_all(DRAMC_REG_SHU1_WODT, CLEAR_FLD,
		SHU1_WODT_WPST2T);
	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL0,
		p_fld(SET_FLD, MISC_CTRL0_R_DQS0IEN_DIV4_CK_CG_CTRL) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_FIFO_EN) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMDQSIEN_DEPTH_HALF) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMSTBEN_OUTSEL) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMRDSEL_DIV2_OPT) |
		p_fld(0xf, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF0,
		p_fld(0x2, SHU_CONF0_MATYPE) |
		p_fld(SET_FLD, SHU_CONF0_BL4) |
		p_fld(SET_FLD, SHU_CONF0_FDIV2) |
		p_fld(SET_FLD, SHU_CONF0_REFTHD) |
		p_fld(SET_FLD, SHU_CONF0_ADVPREEN) |
		p_fld(0x3f, SHU_CONF0_DMPGTIM));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(0x3, SHU_ODTCTRL_TWODT) |
		p_fld(0x5, SHU_ODTCTRL_RODT) |
		p_fld(CLEAR_FLD, SHU_ODTCTRL_WOEN));
	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, 0x5,
		REFCTRL0_REF_PREGATE_CNT);

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_STBCAL,
		p_fld(CLEAR_FLD, STBCAL_DQSIENMODE_SELPH) |
		p_fld(SET_FLD, STBCAL_STB_DQIEN_IG) |
		p_fld(SET_FLD, STBCAL_PICHGBLOCK_NORD) |
		p_fld(SET_FLD, STBCAL_PIMASK_RKCHG_OPT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU1_DQSG,
		p_fld(0x9, SHU1_DQSG_STB_UPDMASKCYC) |
		p_fld(SET_FLD, SHU1_DQSG_STB_UPDMASK_EN));
	io_32_write_fld_align_all(DRAMC_REG_STBCAL, SET_FLD, STBCAL_DQSIENMODE);
	io_32_write_fld_multi_all(DRAMC_REG_SREFCTRL,
		p_fld(CLEAR_FLD, SREFCTRL_SREF_HW_EN) |
		p_fld(0x8, SREFCTRL_SREFDLY) |
		p_fld(CLEAR_FLD, SREFCTRL_SREF2_OPTION));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_PIPE,
		p_fld(SET_FLD, SHU_PIPE_READ_START_EXTEND1) |
		p_fld(SET_FLD, SHU_PIPE_DLE_LAST_EXTEND1));
	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(SET_FLD, CKECTRL_CKEON) |
		p_fld(SET_FLD, CKECTRL_CKETIMER_SEL));

	io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD,
		RKCFG_CKE2RANK_OPT2);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF2,
		p_fld(0x7, SHU_CONF2_DCMDLYREF) |
		p_fld(0x32, SHU_CONF2_FSPCHG_PRDCNT) |
		p_fld(0xf, SHU_CONF2_TCMDO1LAT));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SCINTV,
		p_fld(CLEAR_FLD, SHU_SCINTV_DQS2DQ_SHU_PITHRD) |
		p_fld(0x15, SHU_SCINTV_MRW_INTV) |
		p_fld(CLEAR_FLD, SHU_SCINTV_RDDQC_INTV) |
		p_fld(CLEAR_FLD, SHU_SCINTV_TZQLAT));

	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL, CLEAR_FLD,
		SHUCTRL_R_SHUFFLE_BLOCK_OPT);
	io_32_write_fld_align_all(DRAMC_REG_REFCTRL1, CLEAR_FLD,
		REFCTRL1_SREF_PRD_OPT);
	io_32_write_fld_align_all(DRAMC_REG_REFRATRE_FILTER, SET_FLD,
		REFRATRE_FILTER_PB2AB_OPT);
	io_32_write_fld_align_all(DRAMC_REG_DDRCONF0, SET_FLD,
		DDRCONF0_GDDR3RST);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
		DRAMCTRL_CLKWITRFC);
	io_32_write_fld_multi_all(DRAMC_REG_MISCTL0,
		p_fld(SET_FLD, MISCTL0_REFP_ARB_EN2) |
		p_fld(SET_FLD, MISCTL0_PBC_ARB_EN) |
		p_fld(SET_FLD, MISCTL0_REFA_ARB_EN2));
	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_MWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWSPLIT) |
		p_fld(SET_FLD, PERFCTL0_WFLUSHEN) |
		p_fld(SET_FLD, PERFCTL0_EMILLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWAGEEN) |
		p_fld(SET_FLD, PERFCTL0_RWLLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWOFOEN) |
		p_fld(SET_FLD, PERFCTL0_DISRDPHASE1) |
		p_fld(SET_FLD, PERFCTL0_DUALSCHEN));
	io_32_write_fld_align_all(DRAMC_REG_ARBCTL, 0x80, ARBCTL_MAXPENDCNT);
	io_32_write_fld_multi_all(DRAMC_REG_PADCTRL,
		p_fld(SET_FLD, PADCTRL_DQIENLATEBEGIN) |
		p_fld(SET_FLD, PADCTRL_DQIENQKEND));
	io_32_write_fld_align_all(DRAMC_REG_DRAMC_PD_CTRL, SET_FLD,
		DRAMC_PD_CTRL_DCMREF_OPT);
	io_32_write_fld_align_all(DRAMC_REG_CLKCTRL, SET_FLD,
		CLKCTRL_REG_CLK_1);
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(0x5, REFCTRL0_REF_PREGATE_CNT) |
		p_fld(0x4, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_DLLFRZ));
	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
		SPCMDCTRL_CLR_EN);
	io_32_write_fld_multi_all(DRAMC_REG_CATRAINING1,
		p_fld(0x4, CATRAINING1_CATRAIN_INTV) |
		p_fld(0x3, CATRAINING1_CATRAINLAT));
	io_32_write_fld_align_all(DRAMC_REG_STBCAL, SET_FLD, STBCAL_REFUICHG);

	delay_us(2);

	io_32_write_fld_align_all(DRAMC_REG_RKCFG, CLEAR_FLD, RKCFG_RKMODE);
	/*
	 * 59818 === over_write_setting_begin ===
	 * 59818 === over_write_setting_end ===
	 */
	dramc_setting_default_lp3(p);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF3,
		p_fld(0x5, SHU_CONF3_ZQCSCNT));
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(SET_FLD, REFCTRL0_PBREFEN) |
		p_fld(SET_FLD, REFCTRL0_PBREF_DISBYRATE));
	io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD, RKCFG_DQSOSC2RK);
	io_32_write_fld_align_all(DRAMC_REG_RKCFG, CLEAR_FLD, RKCFG_CKE2RANK);
	io_32_write_fld_align_all(DRAMC_REG_HW_MRR_FUN, SET_FLD,
		HW_MRR_FUN_TMRR_ENA);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_PREALL_OPTION);
	io_32_write_fld_align_all(DRAMC_REG_ZQCS, 0x56, ZQCS_ZQCSOP);

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF3,
		p_fld(0xff, SHU_CONF3_REFRCNT));
	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, CLEAR_FLD,
		SPCMDCTRL_REFRDIS);
	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF1, 0xb0,
		SHU_CONF1_REFBW_FR);
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(SET_FLD, REFCTRL0_REFFRERUN) |
		p_fld(CLEAR_FLD, REFCTRL0_REFDIS));
	io_32_write_fld_align_all(DRAMC_REG_SREFCTRL, SET_FLD,
		SREFCTRL_SREF_HW_EN);
	io_32_write_fld_multi_all(DRAMC_REG_DRAMC_PD_CTRL,
		p_fld(SET_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMEN));
	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_RX_DQ_EYE_SEL) |
		p_fld(SET_FLD, EYESCAN_RG_EX_EYE_SCAN_EN));
	io_32_write_fld_multi_all(DRAMC_REG_STBCAL1,
		p_fld(SET_FLD, STBCAL1_STBCNT_LATCH_EN) |
		p_fld(SET_FLD, STBCAL1_STBENCMPEN));
	io_32_write_fld_align_all(DRAMC_REG_TEST2_1, 0x10000,
		TEST2_1_TEST2_BASE_ADR);
	io_32_write_fld_align_all(DRAMC_REG_TEST2_2, 0x400,
		TEST2_2_TEST2_OFFSET_ADR);
	io_32_write_fld_multi_all(DRAMC_REG_TEST2_3,
		p_fld(SET_FLD, TEST2_3_TESTWREN2_HW_EN) |
		p_fld(0x4, TEST2_3_DQSICALSTP) |
		p_fld(SET_FLD, TEST2_3_TESTAUDPAT));
	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL2, SET_FLD,
		SHUCTRL2_MR13_SHU_EN);
	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_REQQUE_THD_EN) |
		p_fld(SET_FLD, DRAMCTRL_DPDRK_OPT));
	io_32_write_fld_align_all(DRAMC_REG_SHU_CKECTRL, 0x3,
		SHU_CKECTRL_SREF_CK_DLY);
	io_32_write_fld_align_all(DRAMC_REG_DUMMY_RD, 0x2, DUMMY_RD_RANK_NUM);
	io_32_write_fld_align_all(DRAMC_REG_TEST2_4, 0x4,
		TEST2_4_TESTAGENTRKSEL);
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(SET_FLD, REFCTRL1_REF_QUE_AUTOSAVE_EN) |
		p_fld(SET_FLD, REFCTRL1_SLEFREF_AUTOSAVE_EN));
	io_32_write_fld_multi_all(DRAMC_REG_RSTMASK,
		p_fld(CLEAR_FLD, RSTMASK_PHY_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_DAT_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_GT_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_GT_SYNC_MASK_FOR_PHY) |
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK_FOR_PHY));
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
		DRAMCTRL_CTOREQ_HPRI_OPT);

	dramc_mr_init_lp3(p);

	return DRAM_OK;
}
#endif /* SUPPORT_TYPE_LPDDR3 */

#if SUPPORT_TYPE_PCDDR3
static DRAM_STATUS_T dramc_setting_default_ddr3(DRAMC_CTX_T *p)
{
	unsigned char reg_txdly_dqs=0, reg_txdly_dqs_oen=0;
	unsigned char reg_txdly_dqdqm=0, reg_txdly_dqdqm_oen=0;
	unsigned char reg_dly_dqs=0, reg_dly_dqs_oen=0;
	unsigned char reg_dly_dqdqm=0, reg_dly_dqdqm_oen=0;

	if (p->freq_sel == DDR_DDR1600) {
		reg_txdly_dqs = 0x2;
		reg_txdly_dqs_oen = 0x1;
		reg_dly_dqs = 0x1;
		reg_dly_dqs_oen = 0x3;

		reg_txdly_dqdqm = 0x2;
		reg_txdly_dqdqm_oen = 0x1;
		reg_dly_dqdqm = 0x1;
		reg_dly_dqdqm_oen = 0x2;
	} else if (p->freq_sel == DDR_DDR1866) {
		reg_txdly_dqs = 0x2;
		reg_txdly_dqs_oen = 0x2;
		reg_dly_dqs = 0x3;
		reg_dly_dqs_oen = 0x1;

		reg_txdly_dqdqm = 0x2;
		reg_txdly_dqdqm_oen = 0x2;
		reg_dly_dqdqm = 0x3;
		reg_dly_dqdqm_oen = 0x1;
	} else if (p->freq_sel == DDR_DDR1066) {
		reg_txdly_dqs = 0x1;
		reg_txdly_dqs_oen = 0x0;
		reg_dly_dqs = 0x1;
		reg_dly_dqs_oen = 0x0;

		reg_txdly_dqdqm = 0x1;
		reg_txdly_dqdqm_oen = 0x0;
		reg_dly_dqdqm = 0x1;
		reg_dly_dqdqm_oen = 0x0;
	}
	show_msg2((INFO, "[Dramc] PCDDR3 default setting update \n"));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA1,
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_WE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RESET) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_ODT) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CKE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA2,
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CKE1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CMD) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA3,
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA7) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA6) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA5) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA4) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA3) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA4,
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA15) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA14) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA13) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA12) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA11) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA10) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA9) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA8));

#if 0
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA5, 0x11111111);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA6, 0x01101111);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA7, 0x11111111);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA8, 0x11111111);
#else

	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA5, 0x10000111);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA6, 0x01000000);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA7, 0x00000000);
	io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA8, 0x00000000);
#endif

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CLK) |
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CMD) |
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CS));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ0,
		p_fld(SET_FLD, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ3) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ2) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ0_TX_DLY_R1OEN_DQ0) |
		p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ3) |
		p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ2) |
		p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ1) |
		p_fld(0x2, SHURK1_SELPH_DQ0_TX_DLY_R1DQ0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ1,
		p_fld(SET_FLD, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM3) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM2) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ1_TX_DLY_R1OEN_DQM0) |
		p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM3) |
		p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM2) |
		p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM1) |
		p_fld(0x2, SHURK1_SELPH_DQ1_TX_DLY_R1DQM0));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ2,
		p_fld(0x3, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ3) |
		p_fld(0x3, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ2) |
		p_fld(0x3, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ1) |
		p_fld(0x3, SHURK1_SELPH_DQ2_DLY_R1OEN_DQ0) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ2_DLY_R1DQ3) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ2_DLY_R1DQ2) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ2_DLY_R1DQ1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ2_DLY_R1DQ0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQ3,
		p_fld(0x3, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM3) |
		p_fld(0x3, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM2) |
		p_fld(0x3, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM1) |
		p_fld(0x3, SHURK1_SELPH_DQ3_DLY_R1OEN_DQM0) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ3_DLY_R1DQM3) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ3_DLY_R1DQM2) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ3_DLY_R1DQM1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQ3_DLY_R1DQM0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ7_RK1_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ7_RK1_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TX_DLY_RANK_MCK) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(15 - 4, SHU_CONF1_DATLAT_DSEL_PHY) |
		p_fld(15 - 4, SHU_CONF1_DATLAT_DSEL) |
		p_fld(15, SHU_CONF1_DATLAT));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1) |
		p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1) |
		p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
		p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
		p_fld(0x4, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED_P1) |
		p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED_P1) |
		p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
		p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
		p_fld(0x4, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));


	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(28, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(20, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(8, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ2,
		p_fld(8, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_R_DLY_B0) |
		p_fld(8, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ3,
		p_fld(8, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_R_DLY_B0) |
		p_fld(8, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ4,
		p_fld(8, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_R_DLY_B0) |
		p_fld(8, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ5,
		p_fld(8, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_R_DLY_B0) |
		p_fld(8, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ6,
		p_fld(28, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_F_DLY_B0) |
		p_fld(20, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_R_DLY_B0) |
		p_fld(8, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_F_DLY_B0) |
		p_fld(0, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(28, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(20, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(8, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(0, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ2,
		p_fld(8, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_R_DLY_B1) |
		p_fld(8, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ3,
		p_fld(8, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_R_DLY_B1) |
		p_fld(8, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ4,
		p_fld(8, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_R_DLY_B1) |
		p_fld(8, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ5,
		p_fld(8, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_R_DLY_B1) |
		p_fld(8, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ6,
		p_fld(28, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_F_DLY_B1) |
		p_fld(20, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_R_DLY_B1) |
		p_fld(8, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_F_DLY_B1) |
		p_fld(0, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_R_DLY_B1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x2, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(SET_FLD, SHU_RANKCTL_DMSTBLAT));

	io_32_write_fld_align_all(DRAMC_REG_SHURK0_DQSCTL, 0x2,
		SHURK0_DQSCTL_DQSINCTL);
	io_32_write_fld_align_all(DRAMC_REG_SHURK1_DQSCTL, 0x2,
		SHURK1_DQSCTL_R1DQSINCTL);

    return DRAM_OK;
}


static DRAM_STATUS_T ddrphy_setting_ddr3(DRAMC_CTX_T *p)
{
	return ddrphy_setting_common(p);
}

static DRAM_STATUS_T dramc_setting_ddr3(DRAMC_CTX_T *p)
{
	ddrphy_setting_ddr3(p);
	show_msg((INFO, "\n[dramc_init] Meter MEMPLL_MONCLK: %d MHz\n", DDRPhyFreqMeter(27)));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(SET_FLD, REFCTRL0_REFFRERUN) |
		p_fld(SET_FLD, REFCTRL0_REFDIS) |
		p_fld(0x5, REFCTRL0_REF_PREGATE_CNT) |
		p_fld(0x7, REFCTRL0_ADVREF_CNT) |
		p_fld(SET_FLD, REFCTRL0_PBREF_DISBYREFNUM) |
		p_fld(0x4, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_DLLFRZ));

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL3,
		p_fld(CLEAR_FLD, MISC_CTRL3_ARPI_CG_CLK_OPT));

	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(SET_FLD, SHUCTRL2_SHU_PERIOD_GO_ZERO_CNT) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_OPTION) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_PARK_N) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_DLL_CHA) |
		p_fld(0x7, SHUCTRL2_R_DLL_IDLE));

	io_32_write_fld_multi_all(DRAMC_REG_DVFSDLL,
		p_fld(0xe, DVFSDLL_DLL_IDLE_SHU4) |
		p_fld(0x9, DVFSDLL_DLL_IDLE_SHU3) |
		p_fld(0x6, DVFSDLL_DLL_IDLE_SHU2) |
		p_fld(SET_FLD, DVFSDLL_DLL_LOCK_SHU_EN));

	if (p->data_width == DATA_WIDTH_16BIT) {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_DDR3EN) |
			p_fld(SET_FLD, DDRCONF0_BC4OTF_OPT) |
			p_fld(SET_FLD, DDRCONF0_DM8BKEN));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_DDR3EN) |
			p_fld(SET_FLD, DDRCONF0_DM64BITEN) |
			p_fld(SET_FLD, DDRCONF0_BC4OTF_OPT) |
			p_fld(SET_FLD, DDRCONF0_DM8BKEN));
	}

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL0,
		p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_FIFO_EN) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMDQSIEN_DEPTH_HALF) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMSTBEN_OUTSEL) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMRDSEL_DIV2_OPT) |
		p_fld(0xf, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF0,
		p_fld(0x2, SHU_CONF0_MATYPE) |
		p_fld(SET_FLD, SHU_CONF0_BL4) |
		p_fld(SET_FLD, SHU_CONF0_FDIV2) |
		p_fld(SET_FLD, SHU_CONF0_REFTHD) |
		p_fld(SET_FLD, SHU_CONF0_ADVPREEN) |
		p_fld(0x3f, SHU_CONF0_DMPGTIM));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE2) |
		p_fld(0x3, SHU_ODTCTRL_TWODT) |
		p_fld(0x2, SHU_ODTCTRL_RODT) |
		p_fld(SET_FLD, SHU_ODTCTRL_ROEN) |
		p_fld(CLEAR_FLD, SHU_ODTCTRL_WOEN));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(CLEAR_FLD, REFCTRL0_REFFRERUN) |
		p_fld(0x2, REFCTRL0_DISBYREFNUM) |
		p_fld(CLEAR_FLD, REFCTRL0_DLLFRZ));

	io_32_write_fld_multi_all(DRAMC_REG_STBCAL,
		p_fld(SET_FLD, STBCAL_DQSIENMODE) |
		p_fld(CLEAR_FLD, STBCAL_DQSIENMODE_SELPH));

	io_32_write_fld_multi_all(DRAMC_REG_SREFCTRL,
		p_fld(CLEAR_FLD, SREFCTRL_SREF_HW_EN) |
		p_fld(0x8, SREFCTRL_SREFDLY) |
		p_fld(SET_FLD, SREFCTRL_CAPINMUX_OPTION_EN) | /* Switch as ODT pin */
		p_fld(CLEAR_FLD, SREFCTRL_SREF2_OPTION));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_PIPE,
		p_fld(SET_FLD, SHU_PIPE_READ_START_EXTEND1) |
		p_fld(SET_FLD, SHU_PIPE_DLE_LAST_EXTEND1));

	io_32_write_fld_align_all(DRAMC_REG_RKCFG, SET_FLD,
		RKCFG_CKE2RANK_OPT2);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF2,
		p_fld(0x7, SHU_CONF2_DCMDLYREF) |
		p_fld(0x32, SHU_CONF2_FSPCHG_PRDCNT) |
		p_fld(0xf, SHU_CONF2_TCMDO1LAT));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SCINTV,
		p_fld(CLEAR_FLD, SHU_SCINTV_DQS2DQ_SHU_PITHRD) |
		p_fld(0x15, SHU_SCINTV_MRW_INTV) |
		p_fld(CLEAR_FLD, SHU_SCINTV_RDDQC_INTV) |
		p_fld(CLEAR_FLD, SHU_SCINTV_TZQLAT));

	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL, CLEAR_FLD,
		SHUCTRL_R_SHUFFLE_BLOCK_OPT);

	io_32_write_fld_align_all(DRAMC_REG_REFCTRL1, CLEAR_FLD,
		REFCTRL1_SREF_PRD_OPT);

	io_32_write_fld_align_all(DRAMC_REG_REFRATRE_FILTER, SET_FLD,
		REFRATRE_FILTER_PB2AB_OPT);

	dramc_setting_default_ddr3(p);

	io_32_write_fld_multi_all(DDRPHY_PLL2,
		p_fld(SET_FLD, PLL2_GDDR3RST));
	delay_us(500);

	io_32_write_fld_multi_all(DRAMC_REG_MISCTL0,
		p_fld(SET_FLD, MISCTL0_REFP_ARB_EN2) |
		p_fld(SET_FLD, MISCTL0_PBC_ARB_EN) |
		p_fld(SET_FLD, MISCTL0_REFA_ARB_EN2));
	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_MWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWSPLIT) |
		p_fld(SET_FLD, PERFCTL0_WFLUSHEN) |
		p_fld(SET_FLD, PERFCTL0_EMILLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWAGEEN) |
		p_fld(SET_FLD, PERFCTL0_RWLLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWOFOEN) |
		p_fld(SET_FLD, PERFCTL0_DISRDPHASE1) |
		p_fld(SET_FLD, PERFCTL0_DUALSCHEN));
	io_32_write_fld_align_all(DRAMC_REG_PERFCTL0, CLEAR_FLD,
		PERFCTL0_DUALSCHEN);

	io_32_write_fld_align_all(DRAMC_REG_ARBCTL, 0x80, ARBCTL_MAXPENDCNT);

	io_32_write_fld_multi_all(DRAMC_REG_PADCTRL,
		p_fld(SET_FLD, PADCTRL_DQIENLATEBEGIN) |
		p_fld(SET_FLD, PADCTRL_DQIENQKEND));

	io_32_write_fld_multi_all(DRAMC_REG_DRAMC_PD_CTRL,
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMREF_OPT) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMEN));

	io_32_write_fld_align_all(DRAMC_REG_CLKCTRL, SET_FLD,
		CLKCTRL_REG_CLK_1);

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(0x5, REFCTRL0_REF_PREGATE_CNT) |
		p_fld(0x4, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_DLLFRZ));

	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
		SPCMDCTRL_CLR_EN);

	io_32_write_fld_multi_all(DRAMC_REG_CATRAINING1,
		p_fld(0x4, CATRAINING1_CATRAIN_INTV) |
		p_fld(0x3, CATRAINING1_CATRAINLAT));

	io_32_write_fld_multi_all(DRAMC_REG_STBCAL,
		p_fld(SET_FLD, STBCAL_DQSIENMODE) |
		p_fld(CLEAR_FLD, STBCAL_DQSIENMODE_SELPH) |
		p_fld(SET_FLD, STBCAL_REFUICHG));

	io_32_write_fld_multi_all(DRAMC_REG_RKCFG,
		p_fld(SET_FLD, RKCFG_MRS2RK) |
		p_fld(CLEAR_FLD, RKCFG_RKMODE) |
		p_fld(SET_FLD, RKCFG_CKE2RANK_OPT2));

	io_32_write_fld_align_all(DRAMC_REG_RKCFG,
		CLEAR_FLD, RKCFG_CS2RANK);

	delay_us(20);

	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(SET_FLD, CKECTRL_CKEON) |
		p_fld(SET_FLD, CKECTRL_CKETIMER_SEL) |
		p_fld(SET_FLD, CKECTRL_CKEFIXON) |
		p_fld(SET_FLD, CKECTRL_CKE1FIXON));
	delay_us(100);

	dramc_mr_init_ddr3(p);

	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(0x7, DRAMCTRL_TCMD));

	io_32_write_fld_align_all(DRAMC_REG_MRS,0x400, MRS_MRSMA);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, 1, SPCMD_ZQCEN);
	delay_us(1);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, 0, SPCMD_ZQCEN);

	io_32_write_fld_align_all(DRAMC_REG_SPCMD, 1, SPCMD_TCMDEN);
	delay_us(1);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, 0, SPCMD_TCMDEN);

	io_32_write_fld_align_all(DRAMC_REG_HW_MRR_FUN, SET_FLD,
		HW_MRR_FUN_TMRR_ENA);

	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_WDATRGO);

	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_CLKWITRFC);

	io_32_write_fld_align_all(DRAMC_REG_ZQCS, 0x56, ZQCS_ZQCSOP);

	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF3, 0xff, SHU_CONF3_REFRCNT);

	io_32_write_fld_multi_all(DRAMC_REG_SPCMDCTRL,
		p_fld(CLEAR_FLD, SPCMDCTRL_REFRDIS) |
		p_fld(SET_FLD, SPCMDCTRL_CLR_EN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(0xb0, SHU_CONF1_REFBW_FR));

	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, SET_FLD,
		REFCTRL0_REFFRERUN);

	io_32_write_fld_align_all(DRAMC_REG_SREFCTRL, SET_FLD,
		SREFCTRL_SREF_HW_EN);

	io_32_write_fld_multi_all(DRAMC_REG_DRAMC_PD_CTRL,
		p_fld(SET_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMEN));

	io_32_write_fld_multi_all(DRAMC_REG_EYESCAN,
		p_fld(SET_FLD, EYESCAN_RX_DQ_EYE_SEL) |
		p_fld(SET_FLD, EYESCAN_RG_EX_EYE_SCAN_EN));

	io_32_write_fld_multi_all(DRAMC_REG_STBCAL1,
		p_fld(SET_FLD, STBCAL1_STBCNT_LATCH_EN) |
		p_fld(SET_FLD, STBCAL1_STBENCMPEN));

	io_32_write_fld_align_all(DRAMC_REG_TEST2_1, 0x10000,
		TEST2_1_TEST2_BASE_ADR);

	io_32_write_fld_align_all(DRAMC_REG_TEST2_2, 0x400, TEST2_2_TEST2_OFFSET_ADR);

	io_32_write_fld_multi_all(DRAMC_REG_TEST2_3,
		p_fld(SET_FLD, TEST2_3_TESTWREN2_HW_EN) |
		p_fld(0x4, TEST2_3_DQSICALSTP) |
		p_fld(SET_FLD, TEST2_3_TESTAUDPAT));

	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL2, SET_FLD,
		SHUCTRL2_MR13_SHU_EN);

	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_REQQUE_THD_EN) |
		p_fld(SET_FLD, DRAMCTRL_PREALL_OPTION) |
		p_fld(SET_FLD, DRAMCTRL_DPDRK_OPT));

	io_32_write_fld_align_all(DRAMC_REG_SHU_CKECTRL, 0x3,
		SHU_CKECTRL_SREF_CK_DLY);

	io_32_write_fld_align_all(DRAMC_REG_DUMMY_RD, 0x2, DUMMY_RD_RANK_NUM);

	io_32_write_fld_align_all(DRAMC_REG_TEST2_4, 0x4,
		TEST2_4_TESTAGENTRKSEL);

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(SET_FLD, REFCTRL1_REF_QUE_AUTOSAVE_EN) |
		p_fld(SET_FLD, REFCTRL1_SLEFREF_AUTOSAVE_EN));

	io_32_write_fld_multi_all(DRAMC_REG_RSTMASK,
		p_fld(CLEAR_FLD, RSTMASK_PHY_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_DAT_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_GT_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK) |
		p_fld(CLEAR_FLD, RSTMASK_GT_SYNC_MASK_FOR_PHY) |
		p_fld(CLEAR_FLD, RSTMASK_DVFS_SYNC_MASK_FOR_PHY));

	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
		DRAMCTRL_CTOREQ_HPRI_OPT);

#if DRAM_8BIT_X2
	io_32_write_fld_align_all(DDRPHY_PLL1, SET_FLD, PLL1_DDR3_2X8_OPEN);
#endif

	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(CLEAR_FLD, CKECTRL_CKEFIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKE1FIXON));

	io_32_write_fld_align_all(DRAMC_REG_REFCTRL0, CLEAR_FLD,
		REFCTRL0_REFDIS);

	return DRAM_OK;
}
#endif /* SUPPORT_TYPE_PCDDR3 */

#if SUPPORT_TYPE_PCDDR4
static DRAM_STATUS_T dramc_setting_default_ddr4(DRAMC_CTX_T *p)
{
	unsigned char reg_txdly_dqs = 0, reg_txdly_dqs_oen = 0;
	unsigned char reg_txdly_dqdqm = 0 , reg_txdly_dqdqm_oen = 0;
	unsigned char reg_dly_dqs = 0, reg_dly_dqs_oen = 0;
	unsigned char reg_dly_dqdqm = 0, reg_dly_dqdqm_oen = 0;

	if (p->freq_sel == DDR_DDR2667) {
		reg_txdly_dqs = 0x2;
		reg_txdly_dqs_oen = 0x2;
		reg_dly_dqs = 0x7;//0x5;
		reg_dly_dqs_oen = 0x4;//0x2;

		reg_txdly_dqdqm = 0x3;
		reg_txdly_dqdqm_oen = 0x2;
		reg_dly_dqdqm = 0x0;//0x6;
		reg_dly_dqdqm_oen = 0x5;//0x3;
	} else if (p->freq_sel == DDR_DDR2400) {
		reg_txdly_dqs = 0x2;
		reg_txdly_dqs_oen = 0x2;//0x1;
		reg_dly_dqs = 0x3;//0x1;
		reg_dly_dqs_oen = 0x0;//0x6;

		reg_txdly_dqdqm = 0x2;
		reg_txdly_dqdqm_oen = 0x2;//0x1;
		reg_dly_dqdqm = 0x4;//0x2;
		reg_dly_dqdqm_oen = 0x1;//0x7;
	} else if (p->freq_sel == DDR_DDR1866) {
		reg_txdly_dqs = 0x0;
		reg_txdly_dqs_oen = 0x0;
		reg_dly_dqs = 0x5;
		reg_dly_dqs_oen = 0x2;

		reg_txdly_dqdqm = 0x0;
		reg_txdly_dqdqm_oen = 0x0;
		reg_dly_dqdqm = 0x6;
		reg_dly_dqdqm_oen = 0x3;
	}

	show_msg2((INFO, "[Dramc] PCDDR4 default setting update \n"));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA1,
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_WE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RESET) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_ODT) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CKE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA2,
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CKE1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CMD) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA3,
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA7) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA6) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA5) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA4) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA3) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA4,
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA15) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA14) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA13) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA12) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA11) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA10) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA9) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA8));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA5,
		p_fld(0x3, SHU_SELPH_CA5_DLY_CS) |
		p_fld(0x3, SHU_SELPH_CA5_DLY_CKE) |
		p_fld(0x3, SHU_SELPH_CA5_DLY_ODT) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_RESET) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_WE) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CAS) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_RAS) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CS1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA6,
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA0) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA1) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA6_DLY_CMD) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_CKE1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA7,
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA0) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA1) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA2) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA3) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA4) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA5) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA6) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA7));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA8,
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA8) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA9) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA10) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA11) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA12) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA13) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA14) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA15));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_CA_CMD9,
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CLK) |
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CMD) |
		p_fld(CLEAR_FLD, SHU1_R1_CA_CMD9_RG_RK1_ARPI_CS));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS3) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(reg_txdly_dqs_oen, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(reg_txdly_dqs, SHU_SELPH_DQS0_TXDLY_DQS0));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS3) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(reg_dly_dqs_oen, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(reg_dly_dqs, SHU_SELPH_DQS1_DLY_DQS0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ0_TXDLY_DQ0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(reg_txdly_dqdqm_oen, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
		p_fld(reg_txdly_dqdqm, SHURK0_SELPH_DQ1_TXDLY_DQM0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ2_DLY_DQ0));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM3) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(reg_dly_dqdqm_oen, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(reg_dly_dqdqm, SHURK0_SELPH_DQ3_DLY_DQM0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ7_RK1_ARPI_PBYTE_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0) |
		p_fld(0xf, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ7,
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ7_RK1_ARPI_PBYTE_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1) |
		p_fld(0xf, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TX_DLY_RANK_MCK) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN0,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN0_TXDLY_B0_R1RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_ODTEN1,
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B3_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B2_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B1_R1RODTEN) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_ODTEN1_DLY_B0_R1RODTEN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(0xa, SHU_CONF1_DATLAT_DSEL_PHY) |
		p_fld(0xa, SHU_CONF1_DATLAT_DSEL) |
		p_fld(0xf, SHU_CONF1_DATLAT));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
		p_fld(0x6, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG0,
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS3_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS2_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS1_GATED) |
		p_fld(SET_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG0_TX_DLY_R1DQS0_GATED));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK1_SELPH_DQSG1,
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED_P1) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS3_GATED) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED_P1) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS2_GATED) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED_P1) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS1_GATED) |
		p_fld(CLEAR_FLD, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED_P1) |
		p_fld(0x6, SHURK1_SELPH_DQSG1_REG_DLY_R1DQS0_GATED));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(15, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(15, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(15, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(15, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(15, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ2,
		p_fld(15, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ2_RK1_RX_ARDQ1_R_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ2_RK1_RX_ARDQ0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ3,
		p_fld(15, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ3_RK1_RX_ARDQ3_R_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ3_RK1_RX_ARDQ2_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ4,
		p_fld(15, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ4_RK1_RX_ARDQ5_R_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ4_RK1_RX_ARDQ4_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ5,
		p_fld(15, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ5_RK1_RX_ARDQ7_R_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_F_DLY_B0) |
		p_fld(15, SHU1_R1_B0_DQ5_RK1_RX_ARDQ6_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQS0_R_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R1_B0_DQ6_RK1_RX_ARDQM0_R_DLY_B0));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(15, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(15, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(15, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(15, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(15, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ2,
		p_fld(15, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ2_RK1_RX_ARDQ1_R_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ2_RK1_RX_ARDQ0_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ3,
		p_fld(15, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ3_RK1_RX_ARDQ3_R_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ3_RK1_RX_ARDQ2_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ4,
		p_fld(15, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ4_RK1_RX_ARDQ5_R_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ4_RK1_RX_ARDQ4_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ5,
		p_fld(15, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ5_RK1_RX_ARDQ7_R_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_F_DLY_B1) |
		p_fld(15, SHU1_R1_B1_DQ5_RK1_RX_ARDQ6_R_DLY_B1));
	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQS0_R_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R1_B1_DQ6_RK1_RX_ARDQM0_R_DLY_B1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(0x3, SHU_RANKCTL_RANKINCTL_PHY) |
		p_fld(SET_FLD, SHU_RANKCTL_DMSTBLAT));

	io_32_write_fld_align_all(DRAMC_REG_SHURK0_DQSCTL, 0x3,
		SHURK0_DQSCTL_DQSINCTL);
	io_32_write_fld_align_all(DRAMC_REG_SHURK1_DQSCTL, 0x3,
		SHURK1_DQSCTL_R1DQSINCTL);

    return DRAM_OK;
}

static DRAM_STATUS_T ddrphy_setting_ddr4(DRAMC_CTX_T *p)
{
	return ddrphy_setting_common(p);
}

static DRAM_STATUS_T dramc_setting_ddr4(DRAMC_CTX_T *p)
{
	show_msg2((INFO, "Init DRAMC for DDR4...\n"));

	/*
	 * cc mark. seems that these registers are all set during PHY init
	 * and are strongly related with PLL. we'd better not touch it???
	 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ2,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ3,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ4,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ5,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0) |
		p_fld(0x8, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0) |
		p_fld(0xa, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
		p_fld(0x12, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ2,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ3,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ2_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ3_RK0_RX_ARDQ3_F_DLY_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ4,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ4_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ4_RK0_RX_ARDQ5_F_DLY_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ5,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ6_F_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ5_RK0_RX_ARDQ7_F_DLY_B1));

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1) |
		p_fld(0x8, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1) |
		p_fld(0xa, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
		p_fld(0x12, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1));

	/*
	 * TX delay line 4bits
	 * cc mark io32_write_4b_all(DDRPHY_SHU1_R0_B0_DQ0, 0x00000000);
	 */
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ0,
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0) |
		p_fld(0x2, SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0));
	/*
	 * cc mark for RANK1 io32_write_4b_all(DDRPHY_SHU1_R1_B0_DQ0, 0x22222222);
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x1000), 0xbbbbbbbb);
	 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ1,
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQM0_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLYB_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLYB_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLY_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLY_B0));

	/*
	 * cc mark for RANK1 io32_write_4b_all(DDRPHY_SHU1_R1_B0_DQ1, 0x00000002);
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x1004), 0x0000000b);
	 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_R1_B1_DQ0,
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1) |
		p_fld(0x2, SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1));

	/*
	 * cc mark for RANK1 io32_write_4b_all(DDRPHY_SHU1_R1_B1_DQ0, 0x22222222);
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x1050), 0xbbbbbbbb);
	 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ1,
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQM0_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0_DLYB_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0B_DLYB_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0_DLY_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0B_DLY_B1));

	/*
	 * PI
	 * cc mark io32_write_4b_all(DDRPHY_SHU1_R0_B0_DQ7, 0x000f0f00);
	 */
	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0) |
		p_fld(0xf, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(CLEAR_FLD, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0));

	/*
	 * cc mark for RANK1 io32_write_4b_all(DDRPHY_SHU1_R1_B0_DQ7, 0x000f0f00);
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x101c), 0x000f0f00);
	 */

	io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1) |
		p_fld(0xf, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(CLEAR_FLD, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1));

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL1,
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMDQMDBI) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMPHYRST) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCMD_OE) |
		p_fld(SET_FLD, MISC_CTRL1_R_DM_TX_ARCLK_OE) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXBYTE0) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXBYTE1) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXCA) |
		p_fld(SET_FLD, MISC_CTRL1_R_DMARPIDQ_SW) |

		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMARPICA_SW_UPDX) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXBYTE0_SEC) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXBYTE1_SEC) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMMUXCA_SEC) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMASYNNETRIC_DRAM_EN) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMDRAMCLKEN0) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMDRAMCLKEN1) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMSTBENCMP_RK_OPT) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_WL_DOWNSP) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMODTDISOE_A) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMODTDISOE_B) |
		p_fld(CLEAR_FLD, MISC_CTRL1_R_DMODTDISOE_C) |
		p_fld(SET_FLD, MISC_CTRL1_R_DMDA_RRESETB_E));

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL3,
		p_fld(SET_FLD, MISC_CTRL3_ARPI_CG_CMD_OPT) |
		p_fld(CLEAR_FLD, MISC_CTRL3_ARPI_CG_CLK_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_MPDIV_CG_CA_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_CG_MCK_CA_OPT) |
		p_fld(CLEAR_FLD, MISC_CTRL3_ARPI_CG_MCTL_CA_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_DDRPHY_MCK_MPDIV_CG_CA_SEL) |
		p_fld(SET_FLD, MISC_CTRL3_DRAM_CLK_NEW_CA_EN_SEL) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_CG_DQ_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_CG_DQS_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_MPDIV_CG_DQ_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_ARPI_CG_MCK_DQ_OPT) |
		p_fld(CLEAR_FLD, MISC_CTRL3_ARPI_CG_MCTL_DQ_OPT) |
		p_fld(SET_FLD, MISC_CTRL3_DDRPHY_MCK_MPDIV_CG_DQ_SEL) |
		p_fld(SET_FLD, MISC_CTRL3_DRAM_CLK_NEW_DQ_EN_SEL));

	/*
	 * cc mark io32_write_4b_all(DDRPHY_MISC_CTRL3+(1<<POS_BANK_NUM), 0x11351131);
	 * cc mark io32_write_4b_all(DRAMC_REG_SHUCTRL2, 0x0001d007);
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL2,
		p_fld(CLEAR_FLD, SHUCTRL2_SHU_CLK_MASK) |
		p_fld(CLEAR_FLD, SHUCTRL2_R_SHU_RESTORE) |
		p_fld(CLEAR_FLD, SHUCTRL2_MR13_SHU_EN) |
		p_fld(CLEAR_FLD, SHUCTRL2_HWSET_WLRL) |
		p_fld(SET_FLD, SHUCTRL2_SHU_PERIOD_GO_ZERO_CNT) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_OPTION) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_PARK_N) |
		p_fld(CLEAR_FLD, SHUCTRL2_R_GATE_SHU_AHEAD) |
		p_fld(SET_FLD, SHUCTRL2_R_DVFS_DLL_CHA) |
		p_fld(CLEAR_FLD, SHUCTRL2_R_DVFS_SREF_OPT) |
		p_fld(CLEAR_FLD, SHUCTRL2_R_DVFS_FSM_CLR) |
		p_fld(0x7, SHUCTRL2_R_DLL_IDLE));

	io_32_write_fld_multi_all(DRAMC_REG_DVFSDLL,
		p_fld(SET_FLD, DVFSDLL_DLL_LOCK_SHU_EN) |
		p_fld(0x6, DVFSDLL_DLL_IDLE_SHU2) |
		p_fld(0x9, DVFSDLL_DLL_IDLE_SHU3) |
		p_fld(0xe, DVFSDLL_DLL_IDLE_SHU4));

	io_32_write_fld_multi_all(DDRPHY_MISC_CTRL0,
		p_fld(0xf, MISC_CTRL0_R_DMDQSIEN_SYNCOPT) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMDQSIEN_OUTSEL) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMSTBEN_SYNCOPT) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMSTBEN_OUTSEL) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMVALID_DLY_OPT) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMVALID_NARROW_IG) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMVALID_DLY) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_DEPTH_HALF) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMDQSIEN_RDSEL_LAT) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMDQSIEN_VALID_LAT) |
		p_fld(SET_FLD, MISC_CTRL0_R_DMDQSIEN_FIFO_EN) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMSTBENCMP_FIFO_EN) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_DMSTBENCMP_RK_FIFO_EN) |
		p_fld(SET_FLD, MISC_CTRL0_R_DQS0IEN_DIV4_CK_CG_CTRL) |
		p_fld(SET_FLD, MISC_CTRL0_R_DQS1IEN_DIV4_CK_CG_CTRL) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_CLKIEN_DIV4_CK_CG_CTRL) |
		p_fld(CLEAR_FLD, MISC_CTRL0_R_STBENCMP_DIV4CK_EN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF0,
		p_fld(0x3f, SHU_CONF0_DMPGTIM) |
		p_fld(CLEAR_FLD, SHU_CONF0_ADVREFEN) |
		p_fld(SET_FLD, SHU_CONF0_ADVPREEN) |
		p_fld(CLEAR_FLD, SHU_CONF0_TRFCPBIG) |
		p_fld(SET_FLD, SHU_CONF0_REFTHD) |
		p_fld(0x8, SHU_CONF0_REQQUE_DEPTH) |
		p_fld(SET_FLD, SHU_CONF0_FREQDIV4) |
		p_fld(CLEAR_FLD, SHU_CONF0_FDIV2) |
		p_fld(CLEAR_FLD, SHU_CONF0_CL2) |
		p_fld(CLEAR_FLD, SHU_CONF0_BL2) |
		p_fld(SET_FLD, SHU_CONF0_BL4) |
		p_fld(0x2, SHU_CONF0_MATYPE));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ODTCTRL,
		p_fld(p->odt_onoff, SHU_ODTCTRL_ROEN) |
		p_fld(CLEAR_FLD, SHU_ODTCTRL_WOEN) | /* Enable after mrr_init */
		p_fld(0x5, SHU_ODTCTRL_RODT) |
		p_fld(0x7, SHU_ODTCTRL_TWODT) |
		p_fld(CLEAR_FLD, SHU_ODTCTRL_FIXRODT) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE2) |
		p_fld(SET_FLD, SHU_ODTCTRL_RODTE));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(CLEAR_FLD, REFCTRL0_DLLFRZ) |
		p_fld(CLEAR_FLD, REFCTRL0_UPDBYWR) |
		p_fld(CLEAR_FLD, REFCTRL0_DRVCGWREF) |
		p_fld(CLEAR_FLD, REFCTRL0_RFRINTCTL) |
		p_fld(CLEAR_FLD, REFCTRL0_RFRINTEN) |
		p_fld(CLEAR_FLD, REFCTRL0_REFOVERCNT_RST) |
		p_fld(CLEAR_FLD, REFCTRL0_DMPGVLD_IG) |
		p_fld(CLEAR_FLD, REFCTRL0_REFMODE_MANUAL) |
		p_fld(CLEAR_FLD, REFCTRL0_REFMODE_MANUAL_TRIG) |
		p_fld(0x2, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_PBREF_DISBYREFNUM) |
		p_fld(CLEAR_FLD, REFCTRL0_PBREF_DISBYRATE) |
		p_fld(CLEAR_FLD, REFCTRL0_PBREFEN) |
		p_fld(0x7, REFCTRL0_ADVREF_CNT) |
		p_fld(0x5, REFCTRL0_REF_PREGATE_CNT) |
		p_fld(CLEAR_FLD, REFCTRL0_REFNA_OPT) |
		p_fld(SET_FLD, REFCTRL0_REFDIS) |
		p_fld(CLEAR_FLD, REFCTRL0_REFFRERUN) |
		p_fld(CLEAR_FLD, REFCTRL0_REFBW_FREN));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA1,
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CKE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_ODT) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RESET) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_WE) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_RAS) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA1_TXDLY_CS1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA2,
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA0) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_BA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CMD) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA2_TXDLY_CKE1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA3,
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA0) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA1) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA2) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA3) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA4) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA5) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA6) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA3_TXDLY_RA7));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA4,
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA8) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA9) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA10) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA11) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA12) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA13) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA14) |
		p_fld(CLEAR_FLD, SHU_SELPH_CA4_TXDLY_RA15));

	/*
	 * 2T mode (PERFCTL0_DUALSCHEN = 0x1)
	 * cc mark io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA5, 0x10000001);
	 * cc mark io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA6, 0x00000000);
	 * cc mark io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA7, 0x00000000);
	 * cc mark io32_write_4b_all(DRAMC_REG_SHU_SELPH_CA8, 0x00000000);
	 */

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA5,
		p_fld(0x3, SHU_SELPH_CA5_DLY_CS) |
		p_fld(0x3, SHU_SELPH_CA5_DLY_CKE) |
		p_fld(0x3, SHU_SELPH_CA5_DLY_ODT) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_RESET) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_WE) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CAS) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_RAS) |
		p_fld(SET_FLD, SHU_SELPH_CA5_DLY_CS1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA6,
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA0) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA1) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_BA2) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_CMD) |
		p_fld(SET_FLD, SHU_SELPH_CA6_DLY_CKE1));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA7,
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA0) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA1) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA2) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA3) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA4) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA5) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA6) |
		p_fld(SET_FLD, SHU_SELPH_CA7_DLY_RA7));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_CA8,
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA8) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA9) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA10) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA11) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA12) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA13) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA14) |
		p_fld(SET_FLD, SHU_SELPH_CA8_DLY_RA15));

	/*
	 * UI
	 * cc mark io32_write_4b_all(DRAMC_REG_SHU_SELPH_DQS0, 0x22222222);
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS0,
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS0) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_DQS3) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS0) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS3));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_SELPH_DQS1,
		p_fld(0x4, SHU_SELPH_DQS1_DLY_DQS0) |
		p_fld(0x4, SHU_SELPH_DQS1_DLY_DQS1) |
		p_fld(0x4, SHU_SELPH_DQS1_DLY_DQS2) |
		p_fld(0x4, SHU_SELPH_DQS1_DLY_DQS3) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS0) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS1) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS2) |
		p_fld(0x2, SHU_SELPH_DQS1_DLY_OEN_DQS3));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ0,
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ0) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ1) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ2) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_DQ3) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(0x2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ1,
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM0) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM1) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM2) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_DQM3) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(0x2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ2,
		p_fld(0x5, SHURK0_SELPH_DQ2_DLY_DQ0) |
		p_fld(0x5, SHURK0_SELPH_DQ2_DLY_DQ1) |
		p_fld(0x5, SHURK0_SELPH_DQ2_DLY_DQ2) |
		p_fld(0x5, SHURK0_SELPH_DQ2_DLY_DQ3) |
		p_fld(0x3, SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(0x3, SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(0x3, SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(0x3, SHURK0_SELPH_DQ2_DLY_OEN_DQ3));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQ3,
		p_fld(0x5, SHURK0_SELPH_DQ3_DLY_DQM0) |
		p_fld(0x5, SHURK0_SELPH_DQ3_DLY_DQM1) |
		p_fld(0x5, SHURK0_SELPH_DQ3_DLY_DQM2) |
		p_fld(0x5, SHURK0_SELPH_DQ3_DLY_DQM3) |
		p_fld(0x3, SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(0x3, SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
		p_fld(0x3, SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
		p_fld(0x3, SHURK0_SELPH_DQ3_DLY_OEN_DQM3));

	io_32_write_fld_multi_all(DRAMC_REG_STBCAL,
		p_fld(CLEAR_FLD, STBCAL_PIMASK_RKCHG_OPT) |
		p_fld(CLEAR_FLD, STBCAL_PIMASK_RKCHG_EXT) |
		p_fld(CLEAR_FLD, STBCAL_STBDLELAST_OPT) |
		p_fld(CLEAR_FLD, STBCAL_DLLFRZIDLE4XUPD) |
		p_fld(CLEAR_FLD, STBCAL_FASTDQSG2X) |
		p_fld(CLEAR_FLD, STBCAL_FASTDQSGUPD) |
		p_fld(CLEAR_FLD, STBCAL_STBDLELAST_PULSE) |
		p_fld(CLEAR_FLD, STBCAL_STBDLELAST_FILTER) |
		p_fld(CLEAR_FLD, STBCAL_STBUPDSTP) |
		p_fld(CLEAR_FLD, STBCAL_CG_RKEN) |
		p_fld(CLEAR_FLD, STBCAL_STBSTATE_OPT) |
		p_fld(CLEAR_FLD, STBCAL_PHYVALID_IG) |
		p_fld(CLEAR_FLD, STBCAL_SREF_DQSGUPD) |
		p_fld(CLEAR_FLD, STBCAL_STBCNTRST) |
		p_fld(CLEAR_FLD, STBCAL_RKCHGMASKDIS) |
		p_fld(SET_FLD, STBCAL_PICGEN) |
		p_fld(CLEAR_FLD, STBCAL_REFUICHG) |
		p_fld(CLEAR_FLD, STBCAL_STB_SELPHYCALEN) |
		p_fld(CLEAR_FLD, STBCAL_STBCAL2R) |
		p_fld(CLEAR_FLD, STBCAL_STBCALEN) |
		p_fld(CLEAR_FLD, STBCAL_STBDLYOUT_OPT) |
		p_fld(CLEAR_FLD, STBCAL_PICHGBLOCK_NORD) |
		p_fld(CLEAR_FLD, STBCAL_STB_DQIEN_IG) |
		p_fld(SET_FLD, STBCAL_DQSIENCG_CHG_EN) |
		p_fld(SET_FLD, STBCAL_DQSIENCG_NORMAL_EN) |
		p_fld(SET_FLD, STBCAL_DQSIENMODE_SELPH) |
		p_fld(SET_FLD, STBCAL_DQSIENMODE));

	io_32_write_fld_multi_all(DRAMC_REG_SREFCTRL,
		p_fld(CLEAR_FLD, SREFCTRL_SRFPD_DIS) |
		p_fld(CLEAR_FLD, SREFCTRL_SREF3_OPTION) |
		p_fld(CLEAR_FLD, SREFCTRL_SREF3_OPTION1) |
		p_fld(CLEAR_FLD, SREFCTRL_SREF2_OPTION) |
		p_fld(0x8, SREFCTRL_SREFDLY) |
		p_fld(SET_FLD, SREFCTRL_CAPINMUX_OPTION_EN) | /* Switch as ODT pin */
		p_fld(CLEAR_FLD, SREFCTRL_SREF_HW_EN) |
		p_fld(CLEAR_FLD, SREFCTRL_SELFREF));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_PIPE,
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXPIPE1) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXPIPE2) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXPIPE3) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXRDSLPIPE1) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXRDSLPIPE2) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYRXRDSLPIPE3) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYPIPE3EN) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYPIPE2EN) |
		p_fld(CLEAR_FLD, SHU_PIPE_PHYPIPE1EN) |
		p_fld(CLEAR_FLD, SHU_PIPE_DLE_LAST_EXTEND3) |
		p_fld(CLEAR_FLD, SHU_PIPE_READ_START_EXTEND3) |
		p_fld(CLEAR_FLD, SHU_PIPE_DLE_LAST_EXTEND2) |
		p_fld(CLEAR_FLD, SHU_PIPE_READ_START_EXTEND2) |
		p_fld(SET_FLD, SHU_PIPE_DLE_LAST_EXTEND1) |
		p_fld(SET_FLD, SHU_PIPE_READ_START_EXTEND1));

	/*
	 * cc mark io32_write_4b_all(DRAMC_REG_SHUCTRL, 0x00010110);
	 * cc notes: bit[16] is undefined for 7580 & 8167, but is set as 1'b1???
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHUCTRL,
		p_fld(CLEAR_FLD, SHUCTRL_R_SHUFFLE_BLOCK_OPT) |
		p_fld(SET_FLD, SHUCTRL_VRCG_EN) |
		p_fld(CLEAR_FLD, SHUCTRL_SHU_PHYRST_SEL) |
		p_fld(SET_FLD, SHUCTRL_DMSHU_CNT) |
		p_fld(CLEAR_FLD, SHUCTRL_R_DRAMC_CHA) |
		p_fld(CLEAR_FLD, SHUCTRL_R_NEW_SHU_MUX_SPM) |
		p_fld(CLEAR_FLD, SHUCTRL_R_MPDIV_SHU_GP) |
		p_fld(CLEAR_FLD, SHUCTRL_R_OTHER_SHU_GP));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(CLEAR_FLD, REFCTRL1_SLEFREF_AUTOSAVE_EN) |
		p_fld(CLEAR_FLD, REFCTRL1_SREF_PRD_OPT) |
		p_fld(CLEAR_FLD, REFCTRL1_PRE8REF) |
		p_fld(CLEAR_FLD, REFCTRL1_REF_QUE_AUTOSAVE_EN) |
		p_fld(0x7, REFCTRL1_MPENDREF_CNT) |
		p_fld(0x3, REFCTRL1_REFRATE_MANUAL) |
		p_fld(CLEAR_FLD, REFCTRL1_REFRATE_MANUAL_RATE_TRIG));

	/*
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x0d90), 0xe57900fe);
	 * cc mark io32_write_4b_all((DDRPHY_BASE_ADDR + 0x0d98), 0xe57800fe);
	 */

	io_32_write_fld_multi_all(DRAMC_REG_REFRATRE_FILTER,
		p_fld(SET_FLD, REFRATRE_FILTER_REFRATE_FIL0) |
		p_fld(0x2, REFRATRE_FILTER_REFRATE_FIL1) |
		p_fld(0x3, REFRATRE_FILTER_REFRATE_FIL2) |
		p_fld(0x3, REFRATRE_FILTER_REFRATE_FIL3) |
		p_fld(SET_FLD, REFRATRE_FILTER_PB2AB_OPT) |
		p_fld(0x3, REFRATRE_FILTER_REFRATE_FIL4) |
		p_fld(0x4, REFRATRE_FILTER_REFRATE_FIL5) |
		p_fld(0x5, REFRATRE_FILTER_REFRATE_FIL6) |
		p_fld(0x6, REFRATRE_FILTER_REFRATE_FIL7) |
		p_fld(CLEAR_FLD, REFRATRE_FILTER_REFRATE_FILEN));

	io32_write_4b_all(DRAMC_REG_DDRCONF0, 0x00000000);

	if (p->data_width == DATA_WIDTH_16BIT) {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_BC4OTF_OPT) |
			p_fld(SET_FLD, DDRCONF0_GDDR3RST) |
			p_fld(SET_FLD, DDRCONF0_DDR4EN));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_DDRCONF0,
			p_fld(SET_FLD, DDRCONF0_BC4OTF_OPT) |
			p_fld(SET_FLD, DDRCONF0_GDDR3RST) |
			p_fld(SET_FLD, DDRCONF0_DM64BITEN) |
			p_fld(SET_FLD, DDRCONF0_DDR4EN));
	}

	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_CTOREQ_HPRI_OPT) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRDECEN_TARKMODE) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRDECEN) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRBIT3DEC) |
		p_fld(CLEAR_FLD, DRAMCTRL_TMRR2WDIS) |
		p_fld(CLEAR_FLD, DRAMCTRL_DPDRK_OPT) |
		p_fld(CLEAR_FLD, DRAMCTRL_DATMOD) |
		p_fld(CLEAR_FLD, DRAMCTRL_WDATRGO) |
		p_fld(CLEAR_FLD, DRAMCTRL_RANK_ASYM) |
		p_fld(CLEAR_FLD, DRAMCTRL_CLKWITRFC) |
		p_fld(CLEAR_FLD, DRAMCTRL_CHKFORPRE) |
		p_fld(CLEAR_FLD, DRAMCTRL_ASYNCEN) |
		p_fld(SET_FLD, DRAMCTRL_DYNMWREN) |
		p_fld(CLEAR_FLD, DRAMCTRL_ALEBLOCK) |
		p_fld(CLEAR_FLD, DRAMCTRL_TMRRICHKDIS) |
		p_fld(CLEAR_FLD, DRAMCTRL_DMRCDRSV) |
		p_fld(CLEAR_FLD, DRAMCTRL_ZQCALL) |
		p_fld(CLEAR_FLD, DRAMCTRL_PREALL_OPTION) |
		p_fld(CLEAR_FLD, DRAMCTRL_TCMD) |
		p_fld(CLEAR_FLD, DRAMCTRL_FASTW2R) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_DEPTH_UPD) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_THD_EN) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_MAXCNT_CHG));

	io_32_write_fld_multi_all(DRAMC_REG_MISCTL0,
		p_fld(CLEAR_FLD, MISCTL0_CA_IDLE_EN) |
		p_fld(CLEAR_FLD, MISCTL0_IDLE_CNT_OPT) |
		p_fld(CLEAR_FLD, MISCTL0_PAGDIS) |
		p_fld(SET_FLD, MISCTL0_REFA_ARB_EN2) |
		p_fld(CLEAR_FLD, MISCTL0_REFA_ARB_EN_OPT) |
		p_fld(CLEAR_FLD, MISCTL0_REORDER_MASK_E1T) |
		p_fld(CLEAR_FLD, MISCTL0_PBC_ARB_E1T) |
		p_fld(SET_FLD, MISCTL0_PBC_ARB_EN) |
		p_fld(CLEAR_FLD, MISCTL0_REFA_ARB_EN) |
		p_fld(CLEAR_FLD, MISCTL0_REFP_ARB_EN) |
		p_fld(CLEAR_FLD, MISCTL0_EMIPREEN) |
		p_fld(SET_FLD, MISCTL0_REFP_ARB_EN2));

	io_32_write_fld_multi_all(DRAMC_REG_PERFCTL0,
		p_fld(SET_FLD, PERFCTL0_DUALSCHEN) |
		p_fld(SET_FLD, PERFCTL0_DISRDPHASE1) |
		p_fld(CLEAR_FLD, PERFCTL0_AIDCHKEN) |
		p_fld(CLEAR_FLD, PERFCTL0_RWOFOEN) |
		p_fld(CLEAR_FLD, PERFCTL0_RWOFOWNUM) |
		p_fld(CLEAR_FLD, PERFCTL0_RWHPRIEN) |
		p_fld(SET_FLD, PERFCTL0_RWLLATEN) |
		p_fld(SET_FLD, PERFCTL0_RWAGEEN) |
		p_fld(SET_FLD, PERFCTL0_EMILLATEN) |
		p_fld(CLEAR_FLD, PERFCTL0_LASTCMDOPT) |
		p_fld(SET_FLD, PERFCTL0_RWHPRICTL) |
		p_fld(SET_FLD, PERFCTL0_WFLUSHEN) |
		p_fld(CLEAR_FLD, PERFCTL0_RWSPLIT) |
		p_fld(SET_FLD, PERFCTL0_MWHPRIEN) |
		p_fld(CLEAR_FLD, PERFCTL0_REORDER_MODE) |
		p_fld(CLEAR_FLD, PERFCTL0_REORDEREN) |
		p_fld(CLEAR_FLD, PERFCTL0_SBR_MASK_OPT) |
		p_fld(CLEAR_FLD, PERFCTL0_SBR_MASK_OPT2) |
		p_fld(CLEAR_FLD, PERFCTL0_MAFIXHIGH) |
		p_fld(CLEAR_FLD, PERFCTL0_TESTWRHIGH) |
		p_fld(CLEAR_FLD, PERFCTL0_RECORDER_MARSK_OPT) |
		p_fld(CLEAR_FLD, PERFCTL0_WRFFIFO_OPT) |
		p_fld(CLEAR_FLD, PERFCTL0_WRFIO_MODE2) |
		p_fld(CLEAR_FLD, PERFCTL0_RDFIFOEN) |
		p_fld(CLEAR_FLD, PERFCTL0_WRFIFOEN));

	io_32_write_fld_multi_all(DRAMC_REG_ARBCTL,
		p_fld(0x80, ARBCTL_MAXPENDCNT) |
		p_fld(CLEAR_FLD, ARBCTL_RDATACNTDIS) |
		p_fld(CLEAR_FLD, ARBCTL_WDATACNTDIS));

	io_32_write_fld_multi_all(DRAMC_REG_PADCTRL,
		p_fld(SET_FLD, PADCTRL_DQIENQKEND) |
		p_fld(SET_FLD, PADCTRL_DQIENLATEBEGIN) |
		p_fld(CLEAR_FLD, PADCTRL_DISDMOEDIS) |
		p_fld(CLEAR_FLD, PADCTRL_DRAMOEN) |
		p_fld(CLEAR_FLD, PADCTRL_FIXDQIEN) |
		p_fld(CLEAR_FLD, PADCTRL_DISDQIEN) |
		p_fld(CLEAR_FLD, PADCTRL_PINMUX));

	io_32_write_fld_multi_all(DRAMC_REG_DRAMC_PD_CTRL,
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DCMEN) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMEN2) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMENNOTRFC) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_PHYCLK_REFWKEN) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_COMBPHY_CLKENSAME) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DIV4CKE_OPT) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DIV4CLK_OPT) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMREF_OPT) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_COMB_DCM) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_RDPERIODON) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DQIEN_BUFFEN_OPT) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DISSTOP26M) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_COMBCLKCTRL));

	io_32_write_fld_multi_all(DRAMC_REG_CLKCTRL,
		p_fld(0xc, CLKCTRL_PSEL_CNT) |
		p_fld(SET_FLD, CLKCTRL_REG_CLK_0) |
		p_fld(SET_FLD, CLKCTRL_REG_CLK_1));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(SET_FLD, REFCTRL0_DLLFRZ) |
		p_fld(CLEAR_FLD, REFCTRL0_UPDBYWR) |
		p_fld(CLEAR_FLD, REFCTRL0_DRVCGWREF) |
		p_fld(CLEAR_FLD, REFCTRL0_RFRINTCTL) |
		p_fld(CLEAR_FLD, REFCTRL0_RFRINTEN) |
		p_fld(CLEAR_FLD, REFCTRL0_REFOVERCNT_RST) |
		p_fld(CLEAR_FLD, REFCTRL0_DMPGVLD_IG) |
		p_fld(CLEAR_FLD, REFCTRL0_REFMODE_MANUAL) |
		p_fld(CLEAR_FLD, REFCTRL0_REFMODE_MANUAL_TRIG) |
		p_fld(0x4, REFCTRL0_DISBYREFNUM) |
		p_fld(SET_FLD, REFCTRL0_PBREF_DISBYREFNUM) |
		p_fld(CLEAR_FLD, REFCTRL0_PBREF_DISBYRATE) |
		p_fld(CLEAR_FLD, REFCTRL0_PBREFEN) |
		p_fld(0x7, REFCTRL0_ADVREF_CNT) |
		p_fld(0x5, REFCTRL0_REF_PREGATE_CNT) |
		p_fld(CLEAR_FLD, REFCTRL0_REFNA_OPT) |
		p_fld(SET_FLD, REFCTRL0_REFDIS) |
		p_fld(CLEAR_FLD, REFCTRL0_REFFRERUN) |
		p_fld(CLEAR_FLD, REFCTRL0_REFBW_FREN));

	io_32_write_fld_multi_all(DRAMC_REG_SPCMDCTRL,
		p_fld(CLEAR_FLD, SPCMDCTRL_MRRSWUPD) |
		p_fld(CLEAR_FLD, SPCMDCTRL_R_DMDVFSMRW_EN) |
		p_fld(SET_FLD, SPCMDCTRL_DPDWOSC) |
		p_fld(CLEAR_FLD, SPCMDCTRL_RDDQCDIS) |
		p_fld(CLEAR_FLD, SPCMDCTRL_SCPRE) |
		p_fld(CLEAR_FLD, SPCMDCTRL_MRWWOPRA) |
		p_fld(SET_FLD, SPCMDCTRL_CLR_EN) |
		p_fld(CLEAR_FLD, SPCMDCTRL_MRRREFUPD_B) |
		p_fld(CLEAR_FLD, SPCMDCTRL_MRRREFUPD_B) |
		p_fld(CLEAR_FLD, SPCMDCTRL_REFR_BLOCKEN) |
		p_fld(SET_FLD, SPCMDCTRL_REFRDIS) |
		p_fld(CLEAR_FLD, SPCMDCTRL_ZQCALDISB) |
		p_fld(CLEAR_FLD, SPCMDCTRL_ZQCSDISB));

	io_32_write_fld_multi_all(DRAMC_REG_CATRAINING1,
		p_fld(CLEAR_FLD, CATRAINING1_CATRAINEN) |
		p_fld(CLEAR_FLD, CATRAINING1_CATRAINMRS) |
		p_fld(CLEAR_FLD, CATRAINING1_TESTCATRAIN) |
		p_fld(CLEAR_FLD, CATRAINING1_CSTRAIN_OPTION) |
		p_fld(CLEAR_FLD, CATRAINING1_CATRAINCSEXT) |
		p_fld(0x3, CATRAINING1_CATRAINLAT) |
		p_fld(0x4, CATRAINING1_CATRAIN_INTV));

	/*
	 * cc mark io32_write_4b_all(DRAMC_REG_STBCAL, 0xf0300000);
	 * cc notes: this register has already been set above. and Only 1 bit is
	 * different. So only set that bit here.
	 */
	io_32_write_fld_align_all(DRAMC_REG_STBCAL, SET_FLD, STBCAL_REFUICHG);

	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(SET_FLD, SHU_RANKCTL_DMSTBLAT) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_PICGLAT) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_DQSG_MODE) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_TXRANKINCTL) |
		p_fld(CLEAR_FLD, SHU_RANKCTL_TXRANKINCTL_ROOT) |
		p_fld(SET_FLD, SHU_RANKCTL_RANKINCTL) |
		p_fld(SET_FLD, SHU_RANKCTL_RANKINCTL_ROOT1) |
		p_fld(SET_FLD, SHU_RANKCTL_RANKINCTL_PHY));

	/*
	 * cc mark for rank1 io32_write_4b_all(DRAMC_REG_SHURK1_DQSCTL, 0x00000003);
	 * cc mark already set during phy init. io32_write_4b_all(DRAMC_REG_PINMUX_TYPE, 0x00000001);
	 */

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_DQSIEN,
		p_fld(0x10, SHURK0_DQSIEN_R0DQSIEN0DLY) |
		p_fld(0x10, SHURK0_DQSIEN_R0DQSIEN1DLY) |
		p_fld(0x10, SHURK0_DQSIEN_R0DQSIEN2DLY) |
		p_fld(0x10, SHURK0_DQSIEN_R0DQSIEN3DLY));

	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG0,
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1));

	io32_write_4b_all(DRAMC_REG_SHURK0_SELPH_DQSG1, 0x06060606);
	io_32_write_fld_multi_all(DRAMC_REG_SHURK0_SELPH_DQSG1,
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1) |
		p_fld(CLEAR_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED) |
		p_fld(SET_FLD, SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1));

	dramc_setting_default_ddr4(p);
	ddrphy_setting_ddr4(p);
	ssc_enable(p);

	io_32_write_fld_align_all(DDRPHY_PLL2, SET_FLD, PLL2_GDDR3RST);

	delay_us(500);

	io_32_write_fld_multi_all(DRAMC_REG_RKCFG,
		p_fld(CLEAR_FLD, RKCFG_TXRANK) |
		p_fld(SET_FLD, RKCFG_CKE2RANK_OPT2) |
		p_fld(CLEAR_FLD, RKCFG_TXRANKFIX) |
		p_fld(CLEAR_FLD, RKCFG_RKMODE) |
		p_fld(CLEAR_FLD, RKCFG_DM3RANK) |
		p_fld(CLEAR_FLD, RKCFG_RANKRDY_OPT) |
		p_fld(SET_FLD, RKCFG_MRS2RK) |
		p_fld(CLEAR_FLD, RKCFG_DQSOSC2RK) |
		p_fld(SET_FLD, RKCFG_CKE2RANK) |
		p_fld(CLEAR_FLD, RKCFG_CS2RANK) |
		p_fld(CLEAR_FLD, RKCFG_SHU2RKOPT) |
		p_fld(CLEAR_FLD, RKCFG_CKE2RANK_OPT) |
		p_fld(0x3, RKCFG_RKSIZE) |
		p_fld(CLEAR_FLD, RKCFG_DMCKEWAKE) |
		p_fld(SET_FLD, RKCFG_RK0SRF) |
		p_fld(SET_FLD, RKCFG_RK1SRF) |
		p_fld(SET_FLD, RKCFG_RK2SRF) |
		p_fld(CLEAR_FLD, RKCFG_SREF_ENTER_MASK_OPT) |
		p_fld(CLEAR_FLD, RKCFG_RK0DPD) |
		p_fld(CLEAR_FLD, RKCFG_RK1DPD) |
		p_fld(CLEAR_FLD, RKCFG_RK2DPD) |
		p_fld(CLEAR_FLD, RKCFG_RK0DPDX) |
		p_fld(CLEAR_FLD, RKCFG_RK1DPDX) |
		p_fld(CLEAR_FLD, RKCFG_RK2DPDX));

	delay_us(20);

	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(CLEAR_FLD, CKECTRL_CKEBYCTL) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2RANK_OPT3) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2FIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2FIXOFF) |
		p_fld(SET_FLD, CKECTRL_CKE1FIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKE1FIXOFF) |
		p_fld(SET_FLD, CKECTRL_CKEFIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKEFIXOFF) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2RANK_OPT5) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2RANK_OPT6) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2RANK_OPT7) |
		p_fld(CLEAR_FLD, CKECTRL_CKE2RANK_OPT8) |
		p_fld(CLEAR_FLD, CKECTRL_CKEEXTEND) |
		p_fld(SET_FLD, CKECTRL_CKETIMER_SEL) |
		p_fld(CLEAR_FLD, CKECTRL_FASTWAKE_SEL) |
		p_fld(CLEAR_FLD, CKECTRL_CKEWAKE_SEL) |
		p_fld(CLEAR_FLD, CKECTRL_CKE_H2L_OPT) |
		p_fld(SET_FLD, CKECTRL_CKEON));

	delay_us(100);

	dramc_mr_init_ddr4(p);

	io_32_write_fld_multi_all(DRAMC_REG_MRS,
		p_fld(CLEAR_FLD, MRS_MRSOP) |
		p_fld(0x400, MRS_MRSMA) |
		p_fld(CLEAR_FLD, MRS_MRSBA) |
		p_fld(CLEAR_FLD, MRS_MRSRK) |
		p_fld(CLEAR_FLD, MRS_MRRRK) |
		p_fld(CLEAR_FLD, MRS_MPCRK) |
		p_fld(CLEAR_FLD, MRS_MRSBG));
	io32_write_4b_all(DRAMC_REG_SPCMD, 0x00000000);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, SET_FLD, SPCMD_ZQCEN);
	delay_us(10);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, CLEAR_FLD, SPCMD_ZQCEN);

	delay_us(100);

	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_CTOREQ_HPRI_OPT) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRDECEN_TARKMODE) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRDECEN) |
		p_fld(CLEAR_FLD, DRAMCTRL_ADRBIT3DEC) |
		p_fld(CLEAR_FLD, DRAMCTRL_TMRR2WDIS) |
		p_fld(CLEAR_FLD, DRAMCTRL_DPDRK_OPT) |
		p_fld(CLEAR_FLD, DRAMCTRL_DATMOD) |
		p_fld(CLEAR_FLD, DRAMCTRL_WDATRGO) |
		p_fld(CLEAR_FLD, DRAMCTRL_RANK_ASYM) |
		p_fld(CLEAR_FLD, DRAMCTRL_CLKWITRFC) |
		p_fld(CLEAR_FLD, DRAMCTRL_CHKFORPRE) |
		p_fld(CLEAR_FLD, DRAMCTRL_ASYNCEN) |
		p_fld(SET_FLD, DRAMCTRL_DYNMWREN) |
		p_fld(CLEAR_FLD, DRAMCTRL_ALEBLOCK) |
		p_fld(CLEAR_FLD, DRAMCTRL_TMRRICHKDIS) |
		p_fld(CLEAR_FLD, DRAMCTRL_DMRCDRSV) |
		p_fld(CLEAR_FLD, DRAMCTRL_ZQCALL) |
		p_fld(CLEAR_FLD, DRAMCTRL_PREALL_OPTION) |
		p_fld(0x7, DRAMCTRL_TCMD) |
		p_fld(CLEAR_FLD, DRAMCTRL_FASTW2R) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_DEPTH_UPD) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_THD_EN) |
		p_fld(CLEAR_FLD, DRAMCTRL_REQQUE_MAXCNT_CHG));

	io_32_write_fld_align_all(DRAMC_REG_SPCMD, SET_FLD, SPCMD_TCMDEN);
	delay_us(10);
	io_32_write_fld_align_all(DRAMC_REG_SPCMD, CLEAR_FLD, SPCMD_TCMDEN);
	io32_write_4b_all(DRAMC_REG_HW_MRR_FUN, 0x00000000);
	io_32_write_fld_align_all(DRAMC_REG_HW_MRR_FUN, SET_FLD,
		HW_MRR_FUN_TMRR_ENA);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_WDATRGO);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, SET_FLD,
		DRAMCTRL_DMRCDRSV);
	io_32_write_fld_align_all(DRAMC_REG_SHU_ODTCTRL, p->odt_onoff,
		SHU_ODTCTRL_WOEN);
	io_32_write_fld_multi_all(DRAMC_REG_ZQCS,
		p_fld(0x56, ZQCS_ZQCSOP) |
		p_fld(0xa, ZQCS_ZQCSAD) |
		p_fld(CLEAR_FLD, ZQCS_ZQCSMASK) |
		p_fld(CLEAR_FLD, ZQCS_ZQCSDUAL));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF3,
		p_fld(CLEAR_FLD, SHU_CONF3_ZQCSCNT) |
		p_fld(0xff, SHU_CONF3_REFRCNT));

	io_32_write_fld_multi_all(DRAMC_REG_SPCMDCTRL,
		p_fld(CLEAR_FLD, SPCMDCTRL_MRRSWUPD) |
		p_fld(SET_FLD, SPCMDCTRL_R_DMDVFSMRW_EN) |
		p_fld(SET_FLD, SPCMDCTRL_DPDWOSC) |
		p_fld(CLEAR_FLD, SPCMDCTRL_RDDQCDIS) |
		p_fld(CLEAR_FLD, SPCMDCTRL_SCPRE) |
		p_fld(CLEAR_FLD, SPCMDCTRL_MRWWOPRA) |
		p_fld(SET_FLD, SPCMDCTRL_CLR_EN) |
		p_fld(CLEAR_FLD, SPCMDCTRL_MRRREFUPD_B) |
		p_fld(CLEAR_FLD, SPCMDCTRL_REFR_BLOCKEN) |
		p_fld(CLEAR_FLD, SPCMDCTRL_REFRDIS) |
		p_fld(CLEAR_FLD, SPCMDCTRL_ZQCALDISB) |
		p_fld(CLEAR_FLD, SPCMDCTRL_ZQCSDISB));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(0xb0, SHU_CONF1_REFBW_FR));
	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL0,
		p_fld(CLEAR_FLD, REFCTRL0_REFDIS) |
		p_fld(SET_FLD, REFCTRL0_REFFRERUN));
	io_32_write_fld_align_all(DRAMC_REG_SREFCTRL, SET_FLD,
		SREFCTRL_SREF_HW_EN);
	io_32_write_fld_multi_all(DRAMC_REG_MPC_OPTION,
		p_fld(CLEAR_FLD, MPC_OPTION_MPCOP) |
		p_fld(CLEAR_FLD, MPC_OPTION_MPCMANEN) |
		p_fld(CLEAR_FLD, MPC_OPTION_MPCMAN_CAS2EN) |
		p_fld(SET_FLD, MPC_OPTION_MPCRKEN));
	io_32_write_fld_multi_all(DRAMC_REG_DRAMC_PD_CTRL,
		p_fld(SET_FLD, DRAMC_PD_CTRL_DCMEN) |
		p_fld(SET_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN));
	io_32_write_fld_multi_all(DRAMC_REG_STBCAL1,
		p_fld(CLEAR_FLD, STBCAL1_IORGTIM) |
		p_fld(CLEAR_FLD, STBCAL1_STBCNT_MODESEL) |
		p_fld(CLEAR_FLD, STBCAL1_STB_PIDLYCG_IG) |
		p_fld(CLEAR_FLD, STBCAL1_STB_SHIFT_DTCOUT_IG) |
		p_fld(CLEAR_FLD, STBCAL1_INPUTRXTRACK_BLOCK) |
		p_fld(CLEAR_FLD, STBCAL1_STB_FLAGCLR) |
		p_fld(SET_FLD, STBCAL1_STB_DLLFRZ_IG) |
		p_fld(SET_FLD, STBCAL1_STBENCMPEN) |
		p_fld(SET_FLD, STBCAL1_STBCNT_LATCH_EN) |
		p_fld(SET_FLD, STBCAL1_DLLFRZ_MON_PBREF_OPT) |
		p_fld(CLEAR_FLD, STBCAL1_DLLFRZ_BLOCKLONG) |
		p_fld(SET_FLD, STBCAL1_DQSERRCNT_DIS) |
		p_fld(CLEAR_FLD, STBCAL1_STBCNT_SW_RST) |
		p_fld(SET_FLD, STBCAL1_STBCAL_FILTER));

	io_32_write_fld_align_all(DRAMC_REG_TEST2_1, 0x10000,
		TEST2_1_TEST2_BASE_ADR);
	io_32_write_fld_align_all(DRAMC_REG_TEST2_2, 0x400,
		TEST2_2_TEST2_OFFSET_ADR);

	io_32_write_fld_multi_all(DRAMC_REG_TEST2_3,
		p_fld(CLEAR_FLD, TEST2_3_TESTCNT) |
		p_fld(CLEAR_FLD, TEST2_3_DQSICALENX) |
		p_fld(CLEAR_FLD, TEST2_3_DQSICALUPD) |
		p_fld(CLEAR_FLD, TEST2_3_PSTWR2) |
		p_fld(SET_FLD, TEST2_3_TESTAUDPAT) |
		p_fld(0x4, TEST2_3_DQSICALSTP) |
		p_fld(CLEAR_FLD, TEST2_3_DQDLYAUTO) |
		p_fld(CLEAR_FLD, TEST2_3_MANUDLLFRZ) |
		p_fld(CLEAR_FLD, TEST2_3_MANUDQSUPD) |
		p_fld(CLEAR_FLD, TEST2_3_DQSUPDMODE) |
		p_fld(CLEAR_FLD, TEST2_3_DM2RDELSWEN) |
		p_fld(CLEAR_FLD, TEST2_3_DM2RDELSWSEL) |
		p_fld(CLEAR_FLD, TEST2_3_MANUDQS) |
		p_fld(CLEAR_FLD, TEST2_3_DMPAT32) |
		p_fld(SET_FLD, TEST2_3_TESTADR_SHIFT) |
		p_fld(CLEAR_FLD, TEST2_3_TAHPRI_B) |
		p_fld(CLEAR_FLD, TEST2_3_TESTLOOP) |
		p_fld(SET_FLD, TEST2_3_TESTWREN2_HW_EN) |
		p_fld(CLEAR_FLD, TEST2_3_TESTEN1) |
		p_fld(CLEAR_FLD, TEST2_3_TESTRDEN2) |
		p_fld(CLEAR_FLD, TEST2_3_TESTWREN2));

	io_32_write_fld_align_all(DRAMC_REG_SHUCTRL2, SET_FLD,
		SHUCTRL2_MR13_SHU_EN);
	io_32_write_fld_multi_all(DRAMC_REG_DRAMCTRL,
		p_fld(SET_FLD, DRAMCTRL_DPDRK_OPT) |
		p_fld(SET_FLD, DRAMCTRL_REQQUE_DEPTH_UPD));
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CKECTRL,
		p_fld(SET_FLD, SHU_CKECTRL_CMDCKE) |
		p_fld(0x2, SHU_CKECTRL_CKEPRD) |
		p_fld(CLEAR_FLD, SHU_CKECTRL_TCKESRX) |
		p_fld(0x3, SHU_CKECTRL_SREF_CK_DLY));


	io_32_write_fld_multi_all(DRAMC_REG_SHU_CKECTRL,
		p_fld(CLEAR_FLD, DUMMY_RD_SREF_DMYRD_MASK) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMYRDOFOEN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_SW) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMYWR_LPRI_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMY_WR_DBG) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMY_RD_DBG) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT0) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT1) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT2) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT3) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT4) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT5) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT6) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_CNT7) |
		p_fld(0x2, DUMMY_RD_RANK_NUM) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_SREF_DMYRD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DQSG_DMYRD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DQSG_DMYWR_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_PA_OPT) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMY_RD_RX_TRACK));

	io32_write_4b_all(DRAMC_REG_TEST2_4, 0x4080110d);
	io_32_write_fld_multi_all(DRAMC_REG_TEST2_4,
		p_fld(0xd, TEST2_4_TESTAUDINC) |
		p_fld(CLEAR_FLD, TEST2_4_TEST2DISSCRAM) |
		p_fld(CLEAR_FLD, TEST2_4_TESTSSOPAT) |
		p_fld(CLEAR_FLD, TEST2_4_TESTSSOXTALKPAT) |
		p_fld(0x11, TEST2_4_TESTAUDINIT) |
		p_fld(CLEAR_FLD, TEST2_4_TESTAUDBITINV) |
		p_fld(CLEAR_FLD, TEST2_4_TESTAUDMODE) |
		p_fld(CLEAR_FLD, TEST2_4_TESTXTALKPAT) |
		p_fld(CLEAR_FLD, TEST2_4_TEST_REQ_LEN1) |
		p_fld(CLEAR_FLD, TEST2_4_DISMASK) |
		p_fld(CLEAR_FLD, TEST2_4_DQCALDIS) |
		p_fld(SET_FLD, TEST2_4_NEGDQS) |
		p_fld(CLEAR_FLD, TEST2_4_TESTAGENTRK) |
		p_fld(0x4, TEST2_4_TESTAGENTRKSEL) |
		p_fld(CLEAR_FLD, TEST2_4_TESTAGENT_DMYRD_OPT));

	io_32_write_fld_multi_all(DRAMC_REG_REFCTRL1,
		p_fld(SET_FLD, REFCTRL1_SLEFREF_AUTOSAVE_EN) |
		p_fld(SET_FLD, REFCTRL1_REF_QUE_AUTOSAVE_EN));
	io32_write_4b_all(DRAMC_REG_RSTMASK, 0x00000000);
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL, CLEAR_FLD,
		DRAMCTRL_CTOREQ_HPRI_OPT);
	io_32_write_fld_multi_all(DRAMC_REG_CKECTRL,
		p_fld(CLEAR_FLD, CKECTRL_CKE1FIXON) |
		p_fld(CLEAR_FLD, CKECTRL_CKEFIXON));

	return DRAM_OK;
}
#endif /* SUPPORT_TYPE_PCDDR4 */

void dramc_setting_init(DRAMC_CTX_T *p)
{
	switch (p->dram_type) {
	#if SUPPORT_TYPE_LPDDR4
	case TYPE_LPDDR4:
		dramc_setting_lp4(p);
		break;
	#endif

	#if SUPPORT_TYPE_LPDDR3
	case TYPE_LPDDR3:
		dramc_setting_lp3(p);
		break;
	#endif

	#if SUPPORT_TYPE_PCDDR4
	case TYPE_PCDDR4:
		dramc_setting_ddr4(p);
		break;
	#endif

	#if SUPPORT_TYPE_PCDDR3
	case TYPE_PCDDR3:
		dramc_setting_ddr3(p);
		break;
	#endif

	default:
		show_err("Unrecognized Dram type.\n");
		break;
	}
}

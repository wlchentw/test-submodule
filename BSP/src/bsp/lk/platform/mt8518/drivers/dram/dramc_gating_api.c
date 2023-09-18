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

/*
 * @file dramc_calibration_api.c
 *  Basic DRAMC calibration API implementation
 */

/* -----------------------------------------------------------------------------
 *  Include files
 * -----------------------------------------------------------------------------
 */
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>
#include <platform/dramc_api.h>

/* -----------------------------------------------------------------------------
 *  Global variables
 * -----------------------------------------------------------------------------
 */
#if GATING_ADJUST_TXDLY_FOR_TRACKING
unsigned char tx_dly_cal_min[CHANNEL_NUM] = { BYTE_MAX };
unsigned char tx_dly_cal_max[CHANNEL_NUM] = {0};
unsigned char best_coarse_tune2t_backup[CHANNEL_NUM][RANK_MAX][DQS_NUMBER];
unsigned char best_coarse_tune0p5t_backup[CHANNEL_NUM][RANK_MAX][DQS_NUMBER];
unsigned char best_coarse_tune2t_p1_backup[CHANNEL_NUM][RANK_MAX][DQS_NUMBER];
unsigned char best_coarse_tune0p5t_p1_backup
	[CHANNEL_NUM][RANK_MAX][DQS_NUMBER];
#endif

/* This table provides the Start point for Gating Calibration */
static const unsigned char gw_corse_start[TYPE_MAX][DDR_FREQ_MAX] =
{
	/* DDR3 (2667-2400-1866-1600-1066) */
	{0, 0, 0 , 13, 0},

	/* DDR4 */
	{9, 9, 0, 0, 0},

	/* LP3 */
	{0, 0, 19 ,14, 0},

	/* LP4 */
	{0, 29, 0, 0, 0},
};

#if SIMULATION_GATING
/*
 * LP4 RODT range is very large(5ns),  no need to adjust with gating position
 * 0x860 SHU_ODTCTRL     32      ODT CONTROL REGISTER
 *	31	31	RODTE	RW      PUBLIC  1'b1
 *	30	30	RODTE2	RW      PUBLIC  1'b1
 *	7	4	RODT	RW      PUBLIC  4'bx= DQSINCTL or -1
 */
#define GATING_RODT_LATANCY_EN 1 /* Need to enable when RODT enable */
#define GATING_PATTERN_NUM_LP4 0x23
#define GATING_PATTERN_NUM_LP3 0x46
#define GATING_PATTERN_NUM_DDR4 0x46
#define GATING_GOLDEND_DQSCNT_LP4 0x4646 /* Shared by all types */

/*
* Use gating old burst mode to find gating window boundary
*  Set the beginning of window as new burst mode gating window center.
* Current function is for LP4 only
* u1Mode decides old or new length modes (7UI, 8UI) should be used
* 0: OLD 8UI mode (not extend 2T RD preamble)
* 1: NEW 7UI mode (extend 2T RD preamble) (DQS_GW_7UI defined)
*    NEW 8UI mode (extend 2T RD preamble) (DQS_GW_7UI not defined)
*/
void dramc_gating_mode(DRAMC_CTX_T *p, GW_MODE_TYPE_T mode)
{
	/*
	 * mode 0: old burst mode
	 * mode 1:7UI or 8UI gating window length mode
	 * (depends on if DQS_GW_7UI is defined)
	 */
	unsigned char vref_sel = 0;
	unsigned char burste2 = 0;
	unsigned char burst_en = 0;

	if (p->dram_type == TYPE_LPDDR4) {
		if (mode == GW_MODE_NORMAL) {
			vref_sel = 0;
			burste2 = SET_FLD;
			burst_en = 0;
		} else if (mode == GW_MODE_7UI) {
			vref_sel = 2;
			burste2 = CLEAR_FLD;
			burst_en = 2;
		} else { /* 8UI */
			vref_sel = 1;
			burste2 = SET_FLD;
			burst_en = 0;
		}

		/* BIAS_VREF_SEL is used as switch for old, new burst modes */
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ6),
			vref_sel, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ6),
			vref_sel, B1_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B1);

		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
			SET_FLD, B0_DQ3_RG_RX_ARDQS0_DQSIENMODE);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
			SET_FLD, B1_DQ3_RG_RX_ARDQS1_DQSIENMODE);

		/* Switch between 7UI and 8UI mode */
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ7),
			burste2, B0_DQ7_RG_RX_ARDQS0_LP4_BURSTMODE_SEL_B01);
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1),
			!burste2, STBCAL1_STB_PIDLYCG_IG);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ7),
			burst_en, B0_DQ7_RG_RX_ARDQS0_BURST_EN_B01);

		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL),
			SET_FLD, STBCAL_DQSIENMODE_SELPH);
	}

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB) |
		p_fld(CLEAR_FLD, B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
		p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB) |
		p_fld(CLEAR_FLD, B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1));

	delay_us(1);

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB) |
		p_fld(SET_FLD, B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB) |
		p_fld(SET_FLD, B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0));
}

static void rx_dqs_gating_set(DRAMC_CTX_T *p, unsigned char coarse_tune,
	unsigned char rx_dqs_ctl_loop, unsigned char freq_div,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr)
{
	unsigned int value;
	unsigned char dly_coarse_large_p1, dly_coarse_0p5t_p1;
	unsigned char dly_coarse_0p5t, dly_coarse_large;
#if GATING_RODT_LATANCY_EN
	unsigned char dly_coarse_large_rodt, dly_coarse_0p5t_rodt;
	unsigned char dly_coarse_large_rodt_p1 = 0,
		dly_coarse_0p5t_rodt_p1 = 0;
#endif

#if GATING_RODT_LATANCY_EN
	dly_coarse_large_rodt = 0;
	dly_coarse_0p5t_rodt = 0;

	dly_coarse_large_rodt_p1 = 4;
	dly_coarse_0p5t_rodt_p1 = 4;

	/* 1.   DQSG latency =
	 * (1)   R_DMR*DQSINCTL[3:0] (MCK) +
	 * (2)   selph_TX_DLY[2:0] (MCK) +
	 * (3)   selph_dly[2:0] (UI)
	 *
	 * 2.   RODT latency =
	 * (1)   R_DMTRODT[3:0] (MCK) +
	 * (2)   selph_TX_DLY[2:0] (MCK) +
	 * (3)   selph_dly[2:0] (UI)
	 */
#endif
	rxdqs_cal_ptr->dly_coarse_large = coarse_tune / rx_dqs_ctl_loop;
	rxdqs_cal_ptr->dly_coarse_0p5t = coarse_tune % rx_dqs_ctl_loop;
	rxdqs_cal_ptr->dly_coarse_large_p1 =
		(coarse_tune + freq_div) / rx_dqs_ctl_loop;
	rxdqs_cal_ptr->dly_coarse_0p5t_p1 =
		(coarse_tune + freq_div) % rx_dqs_ctl_loop;

	dly_coarse_large = rxdqs_cal_ptr->dly_coarse_large;
	dly_coarse_0p5t = rxdqs_cal_ptr->dly_coarse_0p5t;
	dly_coarse_large_p1 = rxdqs_cal_ptr->dly_coarse_large_p1;
	dly_coarse_0p5t_p1 = rxdqs_cal_ptr->dly_coarse_0p5t_p1;

#if GATING_RODT_LATANCY_EN
	value = (dly_coarse_large << 3) + dly_coarse_0p5t;

	if (value >= 11) {
		value -= 11;
		dly_coarse_large_rodt = value >> 3;
		dly_coarse_0p5t_rodt = value - (dly_coarse_large_rodt << 3);

		value = (dly_coarse_large << 3) + dly_coarse_0p5t - 11;
		dly_coarse_large_rodt_p1 = value >> 3;
		dly_coarse_0p5t_rodt_p1 =
			value - (dly_coarse_large_rodt_p1 << 3);
	} else {
		dly_coarse_large_rodt = 0;
		dly_coarse_0p5t_rodt = 0;
		dly_coarse_large_rodt_p1 = 4;
		dly_coarse_0p5t_rodt_p1 = 4;

		show_msg((INFO, "[RxdqsGatingCal] Error: "));
		show_msg((INFO, "dly_coarse_large_rodt[%d] is already ", 0));
		show_msg((INFO, "0. RODT cannot be -11 UI\n"));
	}
#endif

	/*  4T or 2T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQSG0),
		p_fld((unsigned int) dly_coarse_large,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED) |
		p_fld((unsigned int) dly_coarse_large,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld((unsigned int) dly_coarse_large,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld((unsigned int) dly_coarse_large,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld((unsigned int) dly_coarse_large_p1,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1)
		| p_fld((unsigned int) dly_coarse_large_p1,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1)
		| p_fld((unsigned int) dly_coarse_large_p1,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1)
		| p_fld((unsigned int) dly_coarse_large_p1,
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1));

	/*  0.5T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQSG1),
		p_fld((unsigned int) dly_coarse_0p5t,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED)
		| p_fld((unsigned int) dly_coarse_0p5t,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED)
		| p_fld((unsigned int) dly_coarse_0p5t,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED)
		| p_fld((unsigned int) dly_coarse_0p5t,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED)
		| p_fld((unsigned int) dly_coarse_0p5t_p1,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1)
		| p_fld((unsigned int) dly_coarse_0p5t_p1,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1)
		| p_fld((unsigned int) dly_coarse_0p5t_p1,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1)
		| p_fld((unsigned int) dly_coarse_0p5t_p1,
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1));

#if GATING_RODT_LATANCY_EN
	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_ODTEN0),
		p_fld((unsigned int) dly_coarse_large_rodt,
		SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN) |
		p_fld((unsigned int) dly_coarse_large_rodt,
		SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN)|
		p_fld((unsigned int)dly_coarse_large_rodt,
		SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
		p_fld((unsigned int)dly_coarse_large_rodt,
		SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
		p_fld((unsigned int)dly_coarse_large_rodt_p1,
		SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld((unsigned int)dly_coarse_large_rodt_p1,
		SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
		p_fld((unsigned int)dly_coarse_large_rodt_p1,
		SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1));

	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_ODTEN1),
		p_fld((unsigned int) dly_coarse_0p5t_rodt,
		SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN) |
		p_fld((unsigned int) dly_coarse_0p5t_rodt,
		SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld((unsigned int) dly_coarse_0p5t_rodt,
		SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
		p_fld((unsigned int) dly_coarse_0p5t_rodt,
		SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
		p_fld((unsigned int)dly_coarse_0p5t_rodt_p1,
		SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld((unsigned int)dly_coarse_0p5t_rodt_p1,
		SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
		p_fld((unsigned int)dly_coarse_0p5t_rodt_p1,
		SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1));
#endif
}

static void get_dqs_lead_lag_value(DRAMC_CTX_T *p, unsigned char dqs_i,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr)
{
	if (dqs_i == 0) {
		rxdqs_trans_ptr->dqs_lead[0] = io_32_read_fld_align
			(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS2),
			MISC_PHY_RGS2_AD_RX_ARDQS0_STBEN_LEAD_B01);
		rxdqs_trans_ptr->dqs_lag[0] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS2),
			MISC_PHY_RGS2_AD_RX_ARDQS0_STBEN_LAG_B01);
	} else {	/* dqs1 */
		rxdqs_trans_ptr->dqs_lead[1] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS3),
			MISC_PHY_RGS3_AD_RX_ARDQS1_STBEN_LEAD_B01);
		rxdqs_trans_ptr->dqs_lag[1] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS3),
			MISC_PHY_RGS3_AD_RX_ARDQS1_STBEN_LAG_B01);
	}
}

static void dramc_rx_dqs_cal_init(DRAMC_CTX_T *p,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr)
{
	unsigned int value;

	if (p->dram_type == TYPE_LPDDR4 || p->dram_type == TYPE_PCDDR4)
		dramc_gating_mode(p, GW_MODE_NORMAL);

	/* ok we set a coarse/fine tune value already */
	value = rxdqs_cal_ptr->dly_fine_xt | (rxdqs_cal_ptr->dly_fine_xt << 8)
		| (rxdqs_cal_ptr->dly_fine_xt << 16)
		| (rxdqs_cal_ptr->dly_fine_xt << 24);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSIEN), value);

	/* reset phy, reset read data counter */
	dram_phy_reset(p);

	/* reset DQS counter */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		SET_FLD, SPCMD_DQSGCNTRST);
	delay_us(1);	/* delay 2T */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		CLEAR_FLD, SPCMD_DQSGCNTRST);

	/*  enable TE2, audio pattern */
	dramc_engine2_run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
}


static void init_dqs_gating_nonused_bytes(DRAMC_CTX_T *p,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr, unsigned char dqs_i_start)
{
	unsigned char dqs_i;

	/* LP4, DQ byte 2 and byte 3 are useless, set gating result as 0. */
	for (dqs_i = dqs_i_start; dqs_i < 4; dqs_i++) {
		rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i]
			= rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i]
			= rxdqs_best_wins_ptr->best_fine_tune[dqs_i] = 0;
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i]
			= rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i]
			= rxdqs_best_wins_ptr->best_fine_tune_p1[dqs_i] = 0;

#if GATING_ADJUST_TXDLY_FOR_TRACKING
		best_coarse_tune2t_backup[p->channel][p->rank][dqs_i] = 0;
		best_coarse_tune0p5t_backup[p->channel][p->rank][dqs_i] = 0;
		best_coarse_tune2t_p1_backup[p->channel][p->rank][dqs_i] = 0;
		best_coarse_tune0p5t_p1_backup[p->channel][p->rank][dqs_i] = 0;
#endif
	}
}

static void set_dqs_gating(DRAMC_CTX_T *p,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr, unsigned char dqs_i)
{
#if GATING_ADJUST_TXDLY_FOR_TRACKING
	unsigned char tx_dly_dqsgated = 0;
#endif
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		show_msg((INFO,
			"best DQS%d dly(2T, 0.5T, PI) = (%d, %d, %d)\n",
			dqs_i, rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i],
			rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i],
			rxdqs_best_wins_ptr->best_fine_tune[dqs_i]));
#if GATING_ADJUST_TXDLY_FOR_TRACKING
		/*  find min gating TXDLY (should be in P0) */
		tx_dly_dqsgated =
			rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i];

		if (tx_dly_dqsgated < tx_dly_cal_min[p->channel])
			tx_dly_cal_min[p->channel] = tx_dly_dqsgated;

		best_coarse_tune0p5t_backup[p->channel][p->rank][dqs_i] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i];
		best_coarse_tune2t_backup[p->channel][p->rank][dqs_i] =
			rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i];
#endif
	}
	show_msg2((INFO, "\n"));

	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		show_msg((INFO,
			"best DQS%d P1 dly(2T, 0.5T, PI) = (%d, %d, %d)\n",
			dqs_i,
			rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i],
			rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i],
			rxdqs_best_wins_ptr->best_fine_tune[dqs_i]));

#if GATING_ADJUST_TXDLY_FOR_TRACKING
		/* find max gating TXDLY (should be in P1) */
		tx_dly_dqsgated =
			rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i];

		if (tx_dly_dqsgated > tx_dly_cal_max[p->channel])
			tx_dly_cal_max[p->channel] = tx_dly_dqsgated;

		best_coarse_tune0p5t_p1_backup[p->channel][p->rank][dqs_i] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i];
		best_coarse_tune2t_p1_backup[p->channel][p->rank][dqs_i] =
			rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i];
#endif
	}

	show_msg2((INFO, "\n"));
}

static void set_coarse_tune(DRAMC_CTX_T *p,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr)
{
	/* Yes, for Gating, the delay values are not swapped
	 * to B01. So SW shall set B23's value here...
	 */
	if (p->channel == CHANNEL_B) {
		rxdqs_best_wins_ptr->best_coarse_tune2t[2] =
			rxdqs_best_wins_ptr->best_coarse_tune2t[0];
		rxdqs_best_wins_ptr->best_coarse_tune2t[3] =
			rxdqs_best_wins_ptr->best_coarse_tune2t[1];
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[2] =
			rxdqs_best_wins_ptr->best_coarse_tune2t_p1[0];
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[3] =
			rxdqs_best_wins_ptr->best_coarse_tune2t_p1[1];
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[2] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t[0];
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[3] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t[1];
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[2] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[0];
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[3] =
			rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[1];
	}

	/*  4T or 2T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQSG0),
		p_fld((unsigned int)rxdqs_best_wins_ptr->best_coarse_tune2t[0],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED) |
		p_fld((unsigned int)rxdqs_best_wins_ptr->best_coarse_tune2t[1],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED) |
		p_fld((unsigned int)rxdqs_best_wins_ptr->best_coarse_tune2t[2],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED) |
		p_fld((unsigned int)rxdqs_best_wins_ptr->best_coarse_tune2t[3],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[0],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[1],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[2],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune2t_p1[3],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1));

	/*  0.5T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQSG1),
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[0],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[1],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[2],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[3],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[0],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[1],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[2],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1) |
		p_fld((unsigned int)
		rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[3],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1));
}

#if GATING_RODT_LATANCY_EN
static void set_gating_rodt(DRAMC_CTX_T *p,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr)
{
	unsigned char dqs_i = 0;
	unsigned int v = 0, v1 = 0;
	unsigned char best_coarse_large_rodt[DQS_NUMBER] = { 0 },
		best_coarse_0p5t_rodt[DQS_NUMBER] = {0};
	unsigned char best_coarse_large_rodt_p1[DQS_NUMBER] = { 0 },
		best_coarse_0p5t_rodt_p1[DQS_NUMBER] = {0};

	memset(best_coarse_large_rodt, 0, sizeof(best_coarse_large_rodt));
	memset(best_coarse_0p5t_rodt, 0, sizeof(best_coarse_0p5t_rodt));
	memset(best_coarse_large_rodt_p1, 0, sizeof(best_coarse_large_rodt_p1));
	memset(best_coarse_0p5t_rodt_p1, 0, sizeof(best_coarse_0p5t_rodt_p1));

	/*  RODT = Gating - 11UI, */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {

		v = (rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] << 3) +
			rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i];
		v1 = rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i];
		if (v >= 11) {
			/* P0 */
			v -= 11;
			best_coarse_large_rodt[dqs_i] = v >> 3;
			best_coarse_0p5t_rodt[dqs_i] = v -
				(best_coarse_large_rodt[dqs_i] << 3);

			/* P1 */
			v = rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i];
			v = (v << 3) + v1 - 11;
			best_coarse_large_rodt_p1[dqs_i] = v >> 3;
			best_coarse_0p5t_rodt_p1[dqs_i] = v -
				(best_coarse_large_rodt_p1[dqs_i] << 3);

			show_msg((INFO,
				"best RODT dly(2T, 0.5T) = (%d, %d)\n",
				best_coarse_large_rodt[dqs_i],
				best_coarse_0p5t_rodt[dqs_i]));
		} else {/* just only protect */
			/* P0 */
			best_coarse_large_rodt[dqs_i] = 0;
			best_coarse_0p5t_rodt[dqs_i] = 0;
			/* P1 */
			best_coarse_large_rodt_p1[dqs_i] = 4;
			best_coarse_0p5t_rodt_p1[dqs_i] = 4;

			show_err("[RxdqsGatingCal] Error: ");
			show_err2("best_coarse_tune2t[%d] is already 0.",
				dqs_i);
			show_err("RODT cannot be -1 UI\n");
		}
	}

	/* Two cases.
	 * 1. B01 is used, then B23 is unused, set it to the value of
	 *    B01 is OK.
	 * 2. B23 is used, then the value stored in best_coarse_xxx[0]
	 *    and [1] shall actually be set to B23. Thus copy value to B23
	 *    is neccessary.
	 */
	if (p->channel == CHANNEL_B) {
		best_coarse_large_rodt[2] =
			best_coarse_large_rodt[0];
		best_coarse_large_rodt[3] =
			best_coarse_large_rodt[1];
		best_coarse_large_rodt_p1[2] =
			best_coarse_large_rodt_p1[0];
		best_coarse_large_rodt_p1[3] =
			best_coarse_large_rodt_p1[1];
		best_coarse_0p5t_rodt[2] =
			best_coarse_0p5t_rodt[0];
		best_coarse_0p5t_rodt[3] =
			best_coarse_0p5t_rodt[1];
		best_coarse_0p5t_rodt_p1[2] =
			best_coarse_0p5t_rodt_p1[0];
		best_coarse_0p5t_rodt_p1[3] =
			best_coarse_0p5t_rodt_p1[1];
	}

	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_ODTEN0),
		p_fld((unsigned int) best_coarse_large_rodt[0],
		SHURK0_SELPH_ODTEN0_TXDLY_B0_RODTEN) |
		p_fld((unsigned int) best_coarse_large_rodt[1],
		SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN) |
		p_fld((unsigned int) best_coarse_large_rodt[2],
		SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN) |
		p_fld((unsigned int) best_coarse_large_rodt[3],
		SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN) |
		p_fld((unsigned int) best_coarse_large_rodt_p1[1],
		SHURK0_SELPH_ODTEN0_TXDLY_B1_RODTEN_P1) |
		p_fld((unsigned int) best_coarse_large_rodt_p1[2],
		SHURK0_SELPH_ODTEN0_TXDLY_B2_RODTEN_P1) |
		p_fld((unsigned int) best_coarse_large_rodt_p1[3],
		SHURK0_SELPH_ODTEN0_TXDLY_B3_RODTEN_P1));

	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_ODTEN1),
		p_fld((unsigned int) best_coarse_0p5t_rodt[0],
		SHURK0_SELPH_ODTEN1_DLY_B0_RODTEN) |
		p_fld((unsigned int) best_coarse_0p5t_rodt[1],
		SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN) |
		p_fld((unsigned int) best_coarse_0p5t_rodt[2],
		SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN) |
		p_fld((unsigned int) best_coarse_0p5t_rodt[3],
		SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN) |
		p_fld((unsigned int) best_coarse_0p5t_rodt_p1[1],
		SHURK0_SELPH_ODTEN1_DLY_B1_RODTEN_P1) |
		p_fld((unsigned int) best_coarse_0p5t_rodt_p1[2],
		SHURK0_SELPH_ODTEN1_DLY_B2_RODTEN_P1) |
		p_fld((unsigned int) best_coarse_0p5t_rodt_p1[3],
		SHURK0_SELPH_ODTEN1_DLY_B3_RODTEN_P1));
}
#endif

#if SUPPORT_TYPE_LPDDR4
static void rx_dqs_gating_init_lp4(DRAMC_CTX_T *p)
{
	/*
	* DQ_REV_B*[5] =1, select RX gating mode
	* to prevent 0.5T fake gating window behind real window.
	* LP4: Disable(set to 0) "RX DQS ISI pulse CG function"
	*	during gating window calibration (must set to 1 when done)
	*/
	rx_dqs_isi_pulse_cg(p, DISABLE);

	dram_mr.mr01_value[p->dram_fsp] |= 0x80;
	dramc_mode_reg_write_by_rank(p, p->rank,
		MR01, dram_mr.mr01_value[p->dram_fsp]);

	/* Disable perbank refresh, use all bank refresh */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0), CLEAR_FLD,
		 REFCTRL0_PBREFEN);

	/*
	* Disable HW gating first, 0x1c0[31], need
	*	to disable both UI and PI tracking or
	*	the gating delay reg won't be valid.
	*/
	dramc_hw_gating_on_off(p, 0);

	/*
	 * If DQS ring counter is different as our expectation,
	 * error flag is asserted and the status is in ddrphycfg 0xFC0 ~ 0xFCC
	 * Enable this function by R_DMSTBENCMPEN=1 (0x348[18])
	 * Set R_DMSTBCNT_LATCH_EN=1, 0x348[11]
	 * Set R_DM4TO1MODE=0, 0x54[11]
	 * Clear error flag by ddrphycfg 0x5c0[1] R_DMPHYRST
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBENCMPEN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBCNT_LATCH_EN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0), CLEAR_FLD,
		DDRCONF0_DM4TO1MODE);

	/* enable &reset DQS counter */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTEN);
	delay_us(4); /* wait 1 auto refresh after DQS Counter enable */

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTRST);
	delay_us(1); /* delay 2T */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), CLEAR_FLD,
		SPCMD_DQSGCNTRST);

#if NEED_REVIEW
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL0),
		SET_FLD, MISC_CTRL0_R_STBENCMP_DIV4CK_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1),
		SET_FLD, PLL1_R_DMSTBENCMPEN_CHA);
#endif
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1),
		get_rank(p), MISC_CTRL1_R_DMSTBENCMP_RK_OPT);
	dramc_engine2_init(p, PATTERN1, PATTERN2 | GATING_PATTERN_NUM_LP4,
		TEST_AUDIO_PATTERN, 0);

	show_msg3((INFO, "[Gating]\n"));
	print_calibration_basic_info(p);
}

static void dqs_trans_lp4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr)
{
	unsigned char dqs_i = 0;

	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {

		get_dqs_lead_lag_value(p, dqs_i, trans_ptr);

		if ((trans_ptr->dqs_lead[dqs_i] == 1)
			&& (trans_ptr->dqs_lag[dqs_i] == 1)) {
			trans_ptr->dqs_high[dqs_i]++;
		}

		/* >16 PI prevent glitch */
		if (trans_ptr->dqs_high[dqs_i] *
			rxdqs_cal_ptr->dqs_gw_fine_step > 16) {
			if ((trans_ptr->dqs_lead[dqs_i] == 1)
				&& (trans_ptr->dqs_lag[dqs_i] == 1)) {
				trans_ptr->dly_coarse_large_leadLag[dqs_i]
					= rxdqs_cal_ptr->dly_coarse_large;
				trans_ptr->dly_coarse_0p5t_leadLag[dqs_i]
					= rxdqs_cal_ptr->dly_coarse_0p5t;
				trans_ptr->dly_fine_tune_leadLag[dqs_i]
					= rxdqs_cal_ptr->dly_fine_xt;
				trans_ptr->dqs_transition[dqs_i] = 1;
			} else if (((trans_ptr->dqs_lead[dqs_i] == 1)
				 && (trans_ptr->dqs_lag[dqs_i] == 0))
				|| ((trans_ptr->dqs_lead[dqs_i] == 0)
				 && (trans_ptr->dqs_lag[dqs_i] == 1))) {
				if (trans_ptr->dqs_transition[dqs_i] == 1)
					show_msg3((INFO,
					"[Byte %d] %s (%d, %d, %d)\n", dqs_i,
					"Lead/lag falling Transition",
					trans_ptr->
					dly_coarse_large_leadLag[dqs_i],
					trans_ptr->
					dly_coarse_0p5t_leadLag[dqs_i],
					trans_ptr->
					dly_fine_tune_leadLag[dqs_i]));

				trans_ptr->dqs_transition[dqs_i]++;
			} else if ((trans_ptr->dqs_lead[dqs_i] == 0)
				&& (trans_ptr->dqs_lag[dqs_i] == 0)) {
				show_msg3((INFO, "[Byte %d] %s (%d)\n", dqs_i,
					"Lead/lag Transition tap number",
					trans_ptr->dqs_transition[dqs_i]));
				trans_ptr->dqs_high[dqs_i] = 0;
			}
		}
	}
}

static unsigned char dramc_rx_dqs_tap_check_pass_lp4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char coarse_tune,
	unsigned char coarse_end)
{
	/* if current tap is pass */
	if (rxdqs_win_ptr->pass_begin[dqs_i] == 0) {
		/*
		* no pass tap before ,
		* so it is the beginning of pass range
		*/
		rxdqs_win_ptr->pass_begin[dqs_i] = 1;
		rxdqs_win_ptr->pass_count_1[dqs_i] = 0;
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_large;
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_0p5t;
		rxdqs_win_ptr->min_fine_tune_1[dqs_i] =
			rxdqs_cal_ptr->dly_fine_xt;

		show_msg2((INFO, "[Byte %d]First pass (%d, %d, %d)\n",
			 dqs_i, rxdqs_cal_ptr->dly_coarse_large,
			 rxdqs_cal_ptr->dly_coarse_0p5t,
			 rxdqs_cal_ptr->dly_fine_xt));
	}

	/* incr pass tap number */
	if (rxdqs_win_ptr->pass_begin[dqs_i] == 1)
		rxdqs_win_ptr->pass_count_1[dqs_i]++;

	if ((rxdqs_win_ptr->pass_begin[dqs_i] == 1)
		&& (rxdqs_win_ptr->pass_count_1[dqs_i] *
		rxdqs_cal_ptr->dqs_gw_fine_step > DQS_GW_FINE_END))
		rxdqs_trans_ptr->dqs_high[dqs_i] = 0; /* no count lead/lag */

	if ((rxdqs_win_ptr->pass_count_1[0] *
		rxdqs_cal_ptr->dqs_gw_fine_step > DQS_GW_FINE_END)
		&& (rxdqs_win_ptr->pass_count_1[1] *
		rxdqs_cal_ptr->dqs_gw_fine_step > DQS_GW_FINE_END)) {
		show_log("All bytes gating window > 1UI, Early break!\n");
		rxdqs_cal_ptr->dly_fine_xt = DQS_GW_FINE_END;
		coarse_tune = coarse_end;
	}

	return coarse_tune;
}


static unsigned char dramc_rx_dqs_max_range_lp4(DRAMC_CTX_T *p,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char pass_byte_count)
{

	rxdqs_win_ptr->min_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i];
	rxdqs_win_ptr->min_fine_tune[dqs_i] =
		rxdqs_win_ptr->min_fine_tune_1[dqs_i];
	rxdqs_win_ptr->pass_count[dqs_i] =
		rxdqs_win_ptr->pass_count_1[dqs_i];

	show_msg((INFO,
		"[Byte %d]Bigger pass win(%d, %d, %d)  Pass tap=%d\n",
		dqs_i,
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i],
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i],
		rxdqs_win_ptr->min_fine_tune_1[dqs_i],
		rxdqs_win_ptr->pass_count_1[dqs_i]));

	/*  LP4 pass window around 6 UI(burst mode), set 1~3 UI is pass */
	if ((rxdqs_win_ptr->pass_count_1[dqs_i] *
	     rxdqs_cal_ptr->dqs_gw_fine_step
	     > DQS_GW_FINE_END)
	    && (rxdqs_win_ptr->pass_count_1[dqs_i] *
	     rxdqs_cal_ptr->dqs_gw_fine_step < 96)) {
		pass_byte_count |= (1 << dqs_i);
	}
	return pass_byte_count;
}

static unsigned char dramc_rx_dqs_cal_lp4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char coarse_tune, unsigned char coarse_end)
{
	unsigned int all_result_r = 0, all_result_f = 0;
	unsigned int debug_cnt[DQS_NUMBER] = { 0 };
	unsigned int dqs_counter0, dqs_counter1;
	unsigned char dqs_i = 0;
	unsigned char dqs_result_r, dqs_result_f;
	unsigned short debug_cnt_per_byte;
	unsigned char current_pass;
	static unsigned char pass_byte_count = 0;

	memset(debug_cnt, CLEAR_FLD, sizeof(debug_cnt));

	dramc_rx_dqs_cal_init(p, rxdqs_cal_ptr);

	/* Note that Weber has some limit for STBERR: since
	 * LP4 uses B23, but some internal signal is tied to 0,
	 * which causes STBERR in B23 is always 0, that means
	 * STBERR always indicates PASS...
	 * But B01 is OK.
	 */
	all_result_r = io_32_read_fld_align(DRAMC_REG_ADDR
		(DDRPHY_MISC_STBERR_RK0_R), MISC_STBERR_RK0_R_STBENERR_RK0_R);
	all_result_f = io_32_read_fld_align(DRAMC_REG_ADDR
		(DDRPHY_MISC_STBERR_RK0_F), MISC_STBERR_RK0_F_STBENERR_RK1_R);

	dqs_counter0 = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT0));
	dqs_counter1 = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT1));

	/* read DQS counter */
	if (p->channel == CHANNEL_A) {
		debug_cnt[0] = dqs_counter0;
		debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
		debug_cnt[0] &= 0xffff;
	} else {
		debug_cnt[0] = dqs_counter1;
		debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
		debug_cnt[0] &= 0xffff;
	}

	dramc_gating_mode(p, GW_MODE_7UI);
	dramc_engine2_run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);

	dqs_trans_lp4(p, rxdqs_trans_ptr, rxdqs_cal_ptr);

	show_msg3((INFO, "%2d  %2d  %2d |(B3->B0) 0x%4x, 0x%4x, 0x%4x, 0x%4x |",
		rxdqs_cal_ptr->dly_coarse_large, rxdqs_cal_ptr->dly_coarse_0p5t,
		rxdqs_cal_ptr->dly_fine_xt, debug_cnt[3], debug_cnt[2],
		debug_cnt[1], debug_cnt[0]));
	show_msg3((INFO, " %2x %2x  %2x %2x  %2x %2x  %2x %2x |",
		(all_result_f >> 24) & 0xff, (all_result_r >> 24) & 0xff,
		(all_result_f >> 16) & 0xff, (all_result_r >> 16) & 0xff,
		(all_result_f >> 8) & 0xff, (all_result_r >> 8) & 0xff,
		(all_result_f) & 0xff, (all_result_r) & 0xff));
	show_msg3((INFO, " (%2d, %2d) (%2d %2d) (%2d %2d) (%2d %2d)\n",
		rxdqs_trans_ptr->dqs_lead[3], rxdqs_trans_ptr->dqs_lag[3],
		rxdqs_trans_ptr->dqs_lead[2], rxdqs_trans_ptr->dqs_lag[2],
		rxdqs_trans_ptr->dqs_lead[1], rxdqs_trans_ptr->dqs_lag[1],
		rxdqs_trans_ptr->dqs_lead[0], rxdqs_trans_ptr->dqs_lag[0]));

	/* find gating window pass range per DQS separately */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		if (pass_byte_count & (1 << dqs_i)) /* real window found */
			continue;
		/* get dqs error result */
		dqs_result_r =
			(unsigned char) ((all_result_r >> (8 * dqs_i)) & 0xff);
		dqs_result_f =
			(unsigned char) ((all_result_f >> (8 * dqs_i)) & 0xff);
		debug_cnt_per_byte = (unsigned short) debug_cnt[dqs_i];

		/* check if current tap is pass */
		current_pass = 0;
		if ((dqs_result_r == 0) && (dqs_result_f == 0) &&
			(debug_cnt_per_byte == GATING_GOLDEND_DQSCNT_LP4))
			current_pass = 1;

		/* if current tap is pass */
		if (current_pass) {
			coarse_tune = dramc_rx_dqs_tap_check_pass_lp4(
				p, rxdqs_trans_ptr, rxdqs_cal_ptr,
				rxdqs_win_ptr, dqs_i,
				coarse_tune,  coarse_end);
		} else {	/*  current tap is fail */
			if (rxdqs_win_ptr->pass_begin[dqs_i] == 1) {
				/* at the end of pass range */
				rxdqs_win_ptr->pass_begin[dqs_i] = 0;

				/* save the max range settings, to avoid glitch */
				if (rxdqs_win_ptr->pass_count_1[dqs_i] >
					rxdqs_win_ptr->pass_count[dqs_i]) {

					pass_byte_count =
						dramc_rx_dqs_max_range_lp4(p,
						rxdqs_cal_ptr, rxdqs_win_ptr,
						dqs_i, pass_byte_count);

					if (pass_byte_count == 0x3) {
						show_log("All bytes gating ");
						show_log("window pass Done, ");
						show_log("Early break!\n");
						rxdqs_cal_ptr->dly_fine_xt =
							DQS_GW_FINE_END;
						coarse_tune = coarse_end;
					}
				}
			}
		}
	}
	return coarse_tune;
}

/* find center of each byte */
static void find_dqs_center_lp4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr,
	RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr, unsigned char dqs_i,
	unsigned char rx_dly_dqsienstb_loop, unsigned char rx_dqs_ctl_loop,
	unsigned char freq_div)
{
	unsigned char tmp_offset, tmp_value;

	rxdqs_win_ptr->pass_count[dqs_i] =
		rxdqs_trans_ptr->dqs_transition[dqs_i];
	rxdqs_win_ptr->min_fine_tune[dqs_i] =
		rxdqs_trans_ptr->dly_fine_tune_leadLag[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_0p5t_leadLag[dqs_i];
	rxdqs_win_ptr->min_coarse_tune2t[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_large_leadLag[dqs_i];

	/*  -- PI for Phase0 & Phase1 -- */
	tmp_offset = rxdqs_win_ptr->pass_count[dqs_i] *
		rxdqs_cal_ptr->dqs_gw_fine_step / 2;

	tmp_value = rxdqs_win_ptr->min_fine_tune[dqs_i] + tmp_offset;
	rxdqs_best_wins_ptr->best_fine_tune[dqs_i] =
		tmp_value % rx_dly_dqsienstb_loop;
	rxdqs_best_wins_ptr->best_fine_tune_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_fine_tune[dqs_i];

	/*  coarse tune 0.5T for Phase 0 */
	tmp_offset = tmp_value / rx_dly_dqsienstb_loop;
	tmp_value = rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] + tmp_offset;
	rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i] =
		tmp_value % rx_dqs_ctl_loop;

	/*  coarse tune 2T for Phase 0 */
	tmp_offset = tmp_value / rx_dqs_ctl_loop;
	rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t[dqs_i] + tmp_offset;

	/*  coarse tune 0.5T for Phase 1 */
	tmp_value =
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i] + freq_div;
	rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i] =
		tmp_value % rx_dqs_ctl_loop;

	/*  coarse tune 2T for Phase 1 */
	tmp_offset = tmp_value / rx_dqs_ctl_loop;
	rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] + tmp_offset;
}

static DRAM_STATUS_T dramc_rx_dqs_gating_lp4(DRAMC_CTX_T *p)
{
	unsigned char rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div;
	unsigned int value;
	unsigned char dqs_i = 0;

	show_msg((INFO, "\n[DramcRxdqsGatingCal]\n"));

	unsigned char coarse_tune = 0, coarse_start = 0, coarse_end = BYTE_MAX;

	RXDQS_GATING_CAL_T rxdqs_cal;
	RXDQS_GATING_TRANS_T rxdqs_trans;
	RXDQS_GATING_WIN_T rxdqs_wins;
	RXDQS_GATING_BEST_WIN_T rxdqs_best_wins;

	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}

	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1)),
		(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMD)),
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ6)),
		(DRAMC_REG_ADDR(DDRPHY_B1_DQ6)),
	};

	memset(&rxdqs_cal, 0x0, sizeof(RXDQS_GATING_CAL_T));
	memset(&rxdqs_trans, 0x0, sizeof(RXDQS_GATING_TRANS_T));
	memset(&rxdqs_wins, 0x0, sizeof(RXDQS_GATING_WIN_T));
	memset(&rxdqs_best_wins, 0x0, sizeof(RXDQS_GATING_BEST_WIN_T));

	/* Register backup */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	rx_dqs_gating_init_lp4(p);

	rx_dly_dqsienstb_loop = 32;	/* PI fine tune 0->31 */
	rx_dqs_ctl_loop = 8;	/* rx_dqs_ctl_loop is 8. */
	freq_div = 4;
	rxdqs_cal.dqs_gw_fine_step = DQS_GW_FINE_STEP;

	coarse_start = gw_corse_start[p->dram_type][p->freq_sel];
	coarse_end = coarse_start + 48;

	for (coarse_tune = coarse_start; coarse_tune < coarse_end;
		coarse_tune += DQS_GW_COARSE_STEP) {

		rx_dqs_gating_set(p, coarse_tune, rx_dqs_ctl_loop,
			freq_div, &rxdqs_cal);

		for (rxdqs_cal.dly_fine_xt = DQS_GW_FINE_START;
			rxdqs_cal.dly_fine_xt < DQS_GW_FINE_END;
			rxdqs_cal.dly_fine_xt += rxdqs_cal.dqs_gw_fine_step) {
			coarse_tune = dramc_rx_dqs_cal_lp4(p, &rxdqs_trans,
				&rxdqs_cal, &rxdqs_wins, coarse_tune,
				coarse_end);
		}
	}

	dramc_engine2_end(p);
	set_calibration_result(p, DRAM_CALIBRATION_GATING, DRAM_OK);

	/* Set the gating window end-2UI as gating position */
	rxdqs_wins.pass_count[0] <<= 1;
	rxdqs_wins.pass_count[1] <<= 1;

	/* find center of each byte */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		find_dqs_center_lp4(p, &rxdqs_trans, &rxdqs_cal,
			&rxdqs_wins, &rxdqs_best_wins, dqs_i,
			rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div);
	}

	show_msg2((INFO, "\tdqs input gating window, final dly value\n"));

	init_dqs_gating_nonused_bytes(p, &rxdqs_best_wins, dqs_i);
	set_dqs_gating(p, &rxdqs_best_wins, dqs_i);

	/* Restore registers */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/* MR1 OP[7]=0; */
	dram_mr.mr01_value[p->dram_fsp] &= UNCHAR_MAX;
	dramc_mode_reg_write_by_rank(p, p->rank, MR01,
		dram_mr.mr01_value[p->dram_fsp]);

	/*
	 *LP4: Set ARDQ_RPRE_TOG_EN must be 1
	 *	after gating window calibration
	*/
	rx_dqs_isi_pulse_cg(p, ENABLE);

	/*  4T or 2T coarse tune */
	set_coarse_tune(p, &rxdqs_best_wins);

#if GATING_RODT_LATANCY_EN
	set_gating_rodt(p, &rxdqs_best_wins);
#endif
	if (p->channel == CHANNEL_B) {
		show_msg((INFO, "set CHANNEL_B best win\n"));
		rxdqs_best_wins.best_fine_tune[2] = rxdqs_best_wins.best_fine_tune[0];
		rxdqs_best_wins.best_fine_tune[3] = rxdqs_best_wins.best_fine_tune[1];
	}

	/*  Set Fine Tune Value to registers */
	value = rxdqs_best_wins.best_fine_tune[0] |
		(rxdqs_best_wins.best_fine_tune[1] << 8) |
		(rxdqs_best_wins.best_fine_tune[2] << 16) |
		(rxdqs_best_wins.best_fine_tune[3] << 24);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSIEN), value);

	dram_phy_reset(p);
	show_msg((INFO, "[DramcRxdqsGatingCal] Done\n"));
	return DRAM_OK;
}
#endif

#if SUPPORT_TYPE_PCDDR4
static void rx_dqs_gating_init_ddr4(DRAMC_CTX_T *p)
{
	/*
	* DQ_REV_B*[5] =1, select RX gating mode
	* to prevent 0.5T fake gating window behind real window.
	* LP4: Disable(set to 0) "RX DQS ISI pulse CG function"
	*	during gating window calibration (must set to 1 when done)
	*/
	rx_dqs_isi_pulse_cg(p, DISABLE);

	/* Disable perbank refresh, use all bank refresh */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0), CLEAR_FLD,
		 REFCTRL0_PBREFEN);

	/*
	* Disable HW gating first, 0x1c0[31], need
	*	to disable both UI and PI tracking or
	*	the gating delay reg won't be valid.
	*/
	dramc_hw_gating_on_off(p, 0);

	/*
	 * If DQS ring counter is different as our expectation,
	 * error flag is asserted and the status is in ddrphycfg 0xFC0 ~ 0xFCC
	 * Enable this function by R_DMSTBENCMPEN=1 (0x348[18])
	 * Set R_DMSTBCNT_LATCH_EN=1, 0x348[11]
	 * Set R_DM4TO1MODE=0, 0x54[11]
	 * Clear error flag by ddrphycfg 0x5c0[1] R_DMPHYRST
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBENCMPEN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBCNT_LATCH_EN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0), CLEAR_FLD,
		DDRCONF0_DM4TO1MODE);

	/* enable &reset DQS counter */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTEN);
	delay_us(4); /* wait 1 auto refresh after DQS Counter enable */

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTRST);
	delay_us(1); /* delay 2T */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), CLEAR_FLD,
		SPCMD_DQSGCNTRST);

#if NEED_REVIEW
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL0),
		CLEAR_FLD, MISC_CTRL0_R_STBENCMP_DIV4CK_EN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_PLL1),
		SET_FLD, PLL1_R_DMSTBENCMPEN_CHA);
#endif

	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), get_rank(p),
		MISC_CTRL1_R_DMSTBENCMP_RK_OPT);
	dramc_engine2_init(p, PATTERN1, PATTERN2 | GATING_PATTERN_NUM_DDR4,
		TEST_AUDIO_PATTERN, 0);

	show_msg3((INFO, "[Gating]\n"));
	print_calibration_basic_info(p);
}

static void dqs_trans_ddr4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr)
{
	unsigned char dqs_i = 0;

	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {

		get_dqs_lead_lag_value(p, dqs_i, trans_ptr);

		if ((trans_ptr->dqs_lead[dqs_i] == 1)
			&& (trans_ptr->dqs_lag[dqs_i] == 1)) {
			trans_ptr->dqs_high[dqs_i]++;

			trans_ptr->dly_coarse_large_leadLag[dqs_i]
				= rxdqs_cal_ptr->dly_coarse_large;
			trans_ptr->dly_coarse_0p5t_leadLag[dqs_i]
				= rxdqs_cal_ptr->dly_coarse_0p5t;
			trans_ptr->dly_fine_tune_leadLag[dqs_i]
				= rxdqs_cal_ptr->dly_fine_xt;
		} else if ((trans_ptr->dqs_high[dqs_i] *
			rxdqs_cal_ptr->dqs_gw_fine_step) > 32) {
			trans_ptr->dqs_transition[dqs_i] = 1;

			if ((trans_ptr->dqs_lead[dqs_i] == 1) &&
				(trans_ptr->dqs_lag[dqs_i] = 0))
				trans_ptr->dqs_transition[dqs_i]++;
		}
	}
}

static unsigned char dramc_rx_dqs_tap_check_pass_ddr4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char coarse_tune,
	unsigned char coarse_end)
{
	/* if current tap is pass */
	if (rxdqs_win_ptr->pass_begin[dqs_i] == 0) {
		/*
		* no pass tap before ,
		* so it is the beginning of pass range
		*/
		rxdqs_win_ptr->pass_begin[dqs_i] = 1;
		rxdqs_win_ptr->pass_count_1[dqs_i] = 0;

		show_msg2((INFO, "[Byte %d]First pass (%d, %d, %d)\n",
			dqs_i, rxdqs_trans_ptr->dly_coarse_large_leadLag[dqs_i],
			rxdqs_trans_ptr->dly_coarse_0p5t_leadLag[dqs_i],
			rxdqs_trans_ptr->dly_fine_tune_leadLag[dqs_i]));
	}

	/* Record the latest pass setting */
	rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_large_leadLag[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_0p5t_leadLag[dqs_i];
	rxdqs_win_ptr->min_fine_tune_1[dqs_i] =
		rxdqs_trans_ptr->dly_fine_tune_leadLag[dqs_i];

	rxdqs_win_ptr->pass_count_1[dqs_i]++;

	return coarse_tune;
}

/* find center of each byte */
static void find_dqs_center_ddr4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr,
	RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr, unsigned char dqs_i,
	unsigned char rx_dly_dqsienstb_loop, unsigned char rx_dqs_ctl_loop,
	unsigned char freq_div)
{
	unsigned char tmp_offset, tmp_value;

	rxdqs_best_wins_ptr->best_fine_tune[dqs_i] =
		rxdqs_win_ptr->min_fine_tune[dqs_i];
	rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i];
	rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t[dqs_i];

	/* Update P1 */
	rxdqs_best_wins_ptr->best_fine_tune_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_fine_tune[dqs_i];

	tmp_value = rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i]
		+ freq_div;
	rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i] =
		tmp_value % rx_dqs_ctl_loop;

	tmp_offset = tmp_value / rx_dqs_ctl_loop;
	rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] + tmp_offset;
}

static unsigned char dramc_rx_dqs_max_range_ddr4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char pass_byte_count)
{
	rxdqs_win_ptr->min_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i];
	rxdqs_win_ptr->min_fine_tune[dqs_i] =
		rxdqs_win_ptr->min_fine_tune_1[dqs_i];
	rxdqs_win_ptr->pass_count[dqs_i] =
		rxdqs_win_ptr->pass_count_1[dqs_i];

	show_msg((INFO,
		"[Byte %d]Bigger pass win(%d, %d, %d)  Pass tap=%d\n",
		dqs_i,
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i],
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i],
		rxdqs_win_ptr->min_fine_tune_1[dqs_i],
		rxdqs_win_ptr->pass_count_1[dqs_i]));

	/* DDR4 pass condition is to find
	 * lead/lag flag change from (1,1)->!(1,1)
	 */
	if (rxdqs_trans_ptr->dqs_transition[dqs_i] != 0) {
		pass_byte_count |= (1 << dqs_i);
	}
	return pass_byte_count;
}

static unsigned char dramc_rx_dqs_cal_ddr4(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char coarse_tune, unsigned char coarse_end)
{
	unsigned int all_result_r = 0, all_result_f = 0;
	unsigned int debug_cnt[DQS_NUMBER] = { 0 };
	unsigned char dqs_i = 0;
	unsigned char dqs_result_r, dqs_result_f;
	unsigned short debug_cnt_per_byte;
	unsigned char current_pass;
	static unsigned char pass_byte_count = 0;

	memset(debug_cnt, CLEAR_FLD, sizeof(debug_cnt));

	dramc_rx_dqs_cal_init(p, rxdqs_cal_ptr);

	all_result_r = io_32_read_fld_align(DRAMC_REG_ADDR
		(DDRPHY_MISC_STBERR_RK0_R), MISC_STBERR_RK0_R_STBENERR_RK0_R);
	all_result_f = io_32_read_fld_align(DRAMC_REG_ADDR
		(DDRPHY_MISC_STBERR_RK0_F), MISC_STBERR_RK0_F_STBENERR_RK1_R);

	/* read DQS counter */
	debug_cnt[0] = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT0));
	debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
	debug_cnt[0] &= 0xffff;

#if 0 /* cc mark to follow Azalea flow */
	dramc_gating_mode(p, 1);
	dramc_engine2_run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
#endif

	dqs_trans_ddr4(p, rxdqs_trans_ptr, rxdqs_cal_ptr);

	show_msg2((INFO, "%2d  %2d  %2d |(B3->B0) 0x%4x, 0x%4x, 0x%4x, 0x%4x |",
		rxdqs_cal_ptr->dly_coarse_large, rxdqs_cal_ptr->dly_coarse_0p5t,
		rxdqs_cal_ptr->dly_fine_xt, debug_cnt[3], debug_cnt[2],
		debug_cnt[1], debug_cnt[0]));
	show_msg2((INFO, " %2x %2x  %2x %2x  %2x %2x  %2x %2x |",
		(all_result_f >> 24) & 0xff, (all_result_r >> 24) & 0xff,
		(all_result_f >> 16) & 0xff, (all_result_r >> 16) & 0xff,
		(all_result_f >> 8) & 0xff, (all_result_r >> 8) & 0xff,
		(all_result_f) & 0xff, (all_result_r) & 0xff));
	show_msg2((INFO, " (%2d, %2d) (%2d %2d) (%2d %2d) (%2d %2d)\n",
		rxdqs_trans_ptr->dqs_lead[3], rxdqs_trans_ptr->dqs_lag[3],
		rxdqs_trans_ptr->dqs_lead[2], rxdqs_trans_ptr->dqs_lag[2],
		rxdqs_trans_ptr->dqs_lead[1], rxdqs_trans_ptr->dqs_lag[1],
		rxdqs_trans_ptr->dqs_lead[0], rxdqs_trans_ptr->dqs_lag[0]));

	/* find gating window pass range per DQS separately */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		if (pass_byte_count & (1 << dqs_i)) /* real window found */
			continue;
		/* get dqs error result */
		dqs_result_r =
			(unsigned char) ((all_result_r >> (8 * dqs_i)) & 0xff);
		dqs_result_f =
			(unsigned char) ((all_result_f >> (8 * dqs_i)) & 0xff);
		debug_cnt_per_byte = (unsigned short) debug_cnt[dqs_i];

		/* check if current tap is pass */
		current_pass = 0;
		if ((dqs_result_r == 0) && (dqs_result_f == 0) &&
			(debug_cnt_per_byte == GATING_GOLDEND_DQSCNT_LP4) &&
			(rxdqs_trans_ptr->dqs_lead[dqs_i] == 1) &&
			(rxdqs_trans_ptr->dqs_lag[dqs_i] == 1))
			current_pass = 1;

		/* if current tap is pass */
		if (current_pass) {
			coarse_tune = dramc_rx_dqs_tap_check_pass_ddr4(
				p, rxdqs_trans_ptr, rxdqs_cal_ptr,
				rxdqs_win_ptr, dqs_i,
				coarse_tune,  coarse_end);
		} else {	/*  current tap is fail */
			if (rxdqs_win_ptr->pass_begin[dqs_i] == 1) {
				/* at the end of pass range */
				rxdqs_win_ptr->pass_begin[dqs_i] = 0;

			/* save the max range settings, to avoid glitch */
				if ((rxdqs_win_ptr->pass_count_1[dqs_i] *
					rxdqs_cal_ptr->dqs_gw_fine_step) >
					DQS_GW_FINE_END) {

					pass_byte_count =
						dramc_rx_dqs_max_range_ddr4(p,
						rxdqs_trans_ptr, rxdqs_win_ptr,
						dqs_i, pass_byte_count);

					if (pass_byte_count == 0x3) {
						show_log("All bytes gating ");
						show_log("window pass Done, ");
						show_log("Early break!\n");
						rxdqs_cal_ptr->dly_fine_xt =
							DQS_GW_FINE_END;
						coarse_tune = coarse_end;
					}
				}
			}
		}
	}
	return coarse_tune;
}

static DRAM_STATUS_T dramc_rx_dqs_gating_ddr4(DRAMC_CTX_T *p)
{
	unsigned char rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div;
	unsigned int value;
	unsigned char dqs_i = 0;

	show_msg3((INFO, "start DQS Gating cal\n"));

	unsigned char coarse_tune = 0, coarse_start = 0, coarse_end = BYTE_MAX;

	RXDQS_GATING_CAL_T rxdqs_cal;
	RXDQS_GATING_TRANS_T rxdqs_trans;
	RXDQS_GATING_WIN_T rxdqs_wins;
	RXDQS_GATING_BEST_WIN_T rxdqs_best_wins;

	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}

	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1)),
		(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMD)),
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ6)),
		(DRAMC_REG_ADDR(DDRPHY_B1_DQ6)),
	};

	memset(&rxdqs_cal, 0x0, sizeof(RXDQS_GATING_CAL_T));
	memset(&rxdqs_trans, 0x0, sizeof(RXDQS_GATING_TRANS_T));
	memset(&rxdqs_wins, 0x0, sizeof(RXDQS_GATING_WIN_T));
	memset(&rxdqs_best_wins, 0x0, sizeof(RXDQS_GATING_BEST_WIN_T));

	/* Register backup */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	rx_dqs_gating_init_ddr4(p);

	rx_dly_dqsienstb_loop = 32; /* PI fine tune 0->31 */
	rx_dqs_ctl_loop = 8;	/* rx_dqs_ctl_loop is 8. */
	freq_div = 4;
	rxdqs_cal.dqs_gw_fine_step = DQS_GW_FINE_STEP;

	/*
	 * coarse_start
	 * 1. Depends on current freq's DQSINCTL setting
	 * 2. Preserves ~4UI before actual DQS delay value
	 */
	coarse_start = gw_corse_start[p->dram_type][p->freq_sel];
	coarse_end = coarse_start + RX_DQS_RANGE;

	for (coarse_tune = coarse_start; coarse_tune < coarse_end;
		coarse_tune += DQS_GW_COARSE_STEP) {

		rx_dqs_gating_set(p, coarse_tune, rx_dqs_ctl_loop,
			freq_div, &rxdqs_cal);

		for (rxdqs_cal.dly_fine_xt = DQS_GW_FINE_START;
			rxdqs_cal.dly_fine_xt < DQS_GW_FINE_END;
			rxdqs_cal.dly_fine_xt += rxdqs_cal.dqs_gw_fine_step) {
			coarse_tune = dramc_rx_dqs_cal_ddr4(p, &rxdqs_trans,
				&rxdqs_cal, &rxdqs_wins, coarse_tune,
				coarse_end);
		}
	}

	dramc_engine2_end(p);
	set_calibration_result(p, DRAM_CALIBRATION_GATING, DRAM_OK);

	/* Set the gating window end-2UI as gating position */
	rxdqs_wins.pass_count[0] <<= 1;
	rxdqs_wins.pass_count[1] <<= 1;

	/* find center of each byte */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		find_dqs_center_ddr4(p, &rxdqs_trans, &rxdqs_cal,
			&rxdqs_wins, &rxdqs_best_wins, dqs_i,
			rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div);
	}

	show_msg2((INFO, "\tdqs input gating window, final dly value\n"));

	init_dqs_gating_nonused_bytes(p, &rxdqs_best_wins, dqs_i);
	set_dqs_gating(p, &rxdqs_best_wins, dqs_i);

	/* Restore registers */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/*
	 *LP4: Set ARDQ_RPRE_TOG_EN must be 1
	 *	after gating window calibration
	*/
	rx_dqs_isi_pulse_cg(p, ENABLE);

	/*	4T or 2T coarse tune */
	set_coarse_tune(p, &rxdqs_best_wins);

#if GATING_RODT_LATANCY_EN
	set_gating_rodt(p, &rxdqs_best_wins);
#endif

	/*	Set Fine Tune Value to registers */
	value = rxdqs_best_wins.best_fine_tune[0] |
		(rxdqs_best_wins.best_fine_tune[1] << 8) |
		(rxdqs_best_wins.best_fine_tune[2] << 16) |
		(rxdqs_best_wins.best_fine_tune[3] << 24);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSIEN), value);

	dram_phy_reset(p);
	show_msg3((INFO, "[DramcRxdqsGatingCal] Done\n\n"));
	return DRAM_OK;
}
#endif

#if (SUPPORT_TYPE_LPDDR3 || SUPPORT_TYPE_PCDDR3)
/* lp3 and ddr3 (3rd Generation, 3G DRAM) uses
 * DQS counter to find center
 */

#if SUPPORT_TYPE_PCDDR3
#define ENABLE_LEAD_LAG_FOR_DDR3	1
#else
#define ENABLE_LEAD_LAG_FOR_DDR3	0
#endif

static void rx_dqs_gating_init_3g(DRAMC_CTX_T *p)
{
	/*
	 * DQ_REV_B*[5] =1, select RX gating mode
	 * to prevent 0.5T fake gating window behind real window.
	 * LP4: Disable(set to 0) "RX DQS ISI pulse CG function"
	 * during gating window calibration (must set to 1 when done)
	 */
	rx_dqs_isi_pulse_cg(p, DISABLE);

	/* Disable perbank refresh, use all bank refresh */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0), CLEAR_FLD,
		 REFCTRL0_PBREFEN);

	/*
	 * Disable HW gating first, 0x1c0[31], need
	 * to disable both UI and PI tracking or
	 * the gating delay reg won't be valid.
	 */
	dramc_hw_gating_on_off(p, 0);

	/*
	 * If DQS ring counter is different as our expectation,
	 * error flag is asserted and the status is in ddrphycfg 0xFC0 ~ 0xFCC
	 * Enable this function by R_DMSTBENCMPEN=1 (0x348[18])
	 * Set R_DMSTBCNT_LATCH_EN=1, 0x348[11]
	 * Set R_DM4TO1MODE=0, 0x54[11]
	 * Clear error flag by ddrphycfg 0x5c0[1] R_DMPHYRST
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBENCMPEN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), SET_FLD,
		STBCAL1_STBCNT_LATCH_EN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0), CLEAR_FLD,
		DDRCONF0_DM4TO1MODE);

	/* enable &reset DQS counter */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTEN);
	delay_us(4); /* wait 1 auto refresh after DQS Counter enable */

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSGCNTRST);
	delay_us(1); /* delay 2T */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), CLEAR_FLD,
		SPCMD_DQSGCNTRST);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), get_rank(p),
		MISC_CTRL1_R_DMSTBENCMP_RK_OPT);

	/* cc notes: For 32BIT data width, the DQS number will be
	 * half of 16BIT width, thus shall double pattern length
	 */
	if (p->data_width == DATA_WIDTH_32BIT) {
		dramc_engine2_init(p, PATTERN1,
			PATTERN2 | (GATING_PATTERN_NUM_LP3 << 1),
			TEST_AUDIO_PATTERN, 0);
	} else {
		dramc_engine2_init(p, PATTERN1,
			PATTERN2 | GATING_PATTERN_NUM_LP3,
			TEST_AUDIO_PATTERN, 0);
	}

	show_msg3((INFO, "[Gating]\n"));
	print_calibration_basic_info(p);
}

static void dqs_trans_3g(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr,
	unsigned char dqs_i)
{
#if (ENABLE_LEAD_LAG_FOR_DDR3 == 1)
	unsigned char channel_bkp;

	channel_bkp = p->channel;

	if (dqs_i > 1)
		p->channel = CHANNEL_B;

	if ((dqs_i % 2) == 0) {
		trans_ptr->dqs_lead[dqs_i] = io_32_read_fld_align
			(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS2),
			MISC_PHY_RGS2_AD_RX_ARDQS0_STBEN_LEAD_B01);
		trans_ptr->dqs_lag[dqs_i] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS2),
			MISC_PHY_RGS2_AD_RX_ARDQS0_STBEN_LAG_B01);
	} else { /* dqs1 */
		trans_ptr->dqs_lead[dqs_i] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS3),
			MISC_PHY_RGS3_AD_RX_ARDQS1_STBEN_LEAD_B01);
		trans_ptr->dqs_lag[dqs_i] = io_32_read_fld_align
			(DRAMC_REG_ADDR
			(DDRPHY_MISC_PHY_RGS3),
			MISC_PHY_RGS3_AD_RX_ARDQS1_STBEN_LAG_B01);
	}

	if (trans_ptr->dqs_lead[dqs_i] == 1 &&
		trans_ptr->dqs_lag[dqs_i] == 1) {
		trans_ptr->dqs_high[dqs_i]++;
		trans_ptr->dqs_transition[dqs_i] = 1;

		trans_ptr->dly_coarse_large_leadLag[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_large;
		trans_ptr->dly_coarse_0p5t_leadLag[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_0p5t;
		trans_ptr->dly_fine_tune_leadLag[dqs_i] =
			rxdqs_cal_ptr->dly_fine_xt;
	} else if (trans_ptr->dqs_lead[dqs_i] == 1 &&
		trans_ptr->dqs_lag[dqs_i] == 0) {
		trans_ptr->dqs_transition[dqs_i]++;
	}

	p->channel = channel_bkp;
#endif
}

static void dramc_rx_dqs_tap_check_pass_3g(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char coarse_end)
{
	/* if current tap is pass */
	if (rxdqs_win_ptr->pass_begin[dqs_i] == 0) {
		/*
		* no pass tap before ,
		* so it is the beginning of pass range
		*/
		rxdqs_win_ptr->pass_begin[dqs_i] = 1;
		rxdqs_win_ptr->pass_count_1[dqs_i] = 0;
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_large;
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i] =
			rxdqs_cal_ptr->dly_coarse_0p5t;
		rxdqs_win_ptr->min_fine_tune_1[dqs_i] =
			rxdqs_cal_ptr->dly_fine_xt;

		show_msg2((INFO, "[Byte %d]First pass (%d, %d, %d)\n",
			 dqs_i, rxdqs_cal_ptr->dly_coarse_large,
			 rxdqs_cal_ptr->dly_coarse_0p5t,
			 rxdqs_cal_ptr->dly_fine_xt));
	}

	/* incr pass tap number */
	if (rxdqs_win_ptr->pass_begin[dqs_i] == 1)
		rxdqs_win_ptr->pass_count_1[dqs_i]++;

#if (ENABLE_LEAD_LAG_FOR_DDR3 == 1)
	/* If
	 * 1. dqs counter pass window is larger than 1UI
	 * 2. lead/lag transition position is found
	 * will stop update lead lag info in case new value
	 * re-write the corrent ones.
	 */
	if ((rxdqs_win_ptr->pass_count_1[dqs_i] *
		rxdqs_cal_ptr->dqs_gw_fine_step) > DQS_GW_FINE_END) {
		if ((rxdqs_trans_ptr->dqs_high[dqs_i] != 0) &&
			(rxdqs_trans_ptr->dqs_lead[dqs_i] == 0) &&
			(rxdqs_trans_ptr->dqs_lag[dqs_i] == 0)) {
			show_msg2((INFO, "Byte %d lead/lag %s",
				dqs_i, "pass condition triggerred!\n"));

			return;
		}
	}

	dqs_trans_3g(p, rxdqs_trans_ptr, rxdqs_cal_ptr, dqs_i);

	show_msg2((INFO, " Byte %d lead/lag: (%2d, %2d)\n", dqs_i,
		rxdqs_trans_ptr->dqs_lead[dqs_i],
		rxdqs_trans_ptr->dqs_lag[dqs_i]));
#endif
}

static unsigned char dramc_rx_dqs_max_range_3g(DRAMC_CTX_T *p,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char dqs_i, unsigned char pass_byte_count)
{
	rxdqs_win_ptr->min_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i];
	rxdqs_win_ptr->min_fine_tune[dqs_i] =
		rxdqs_win_ptr->min_fine_tune_1[dqs_i];
	rxdqs_win_ptr->pass_count[dqs_i] =
		rxdqs_win_ptr->pass_count_1[dqs_i];

	show_msg((INFO,
		"[Byte %d]Bigger pass win(%d, %d, %d)  Pass tap=%d\n",
		dqs_i,
		rxdqs_win_ptr->min_coarse_tune2t_1[dqs_i],
		rxdqs_win_ptr->min_coarse_tune0p5t_1[dqs_i],
		rxdqs_win_ptr->min_fine_tune_1[dqs_i],
		rxdqs_win_ptr->pass_count_1[dqs_i]));

	/*	LP4 pass window around 6 UI(burst mode), set 1~3 UI is pass */
	if ((rxdqs_win_ptr->pass_count_1[dqs_i] *
		 rxdqs_cal_ptr->dqs_gw_fine_step
		 > DQS_GW_FINE_END)
		&& (rxdqs_win_ptr->pass_count_1[dqs_i] *
		 rxdqs_cal_ptr->dqs_gw_fine_step < 96)) {
		pass_byte_count |= (1 << dqs_i);
	}
	return pass_byte_count;
}

static unsigned char dramc_rx_dqs_cal_3g(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr, RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	unsigned char coarse_tune, unsigned char coarse_end)
{
	unsigned int debug_cnt[DQS_NUMBER] = { 0 };
	unsigned int dqs_counter0, dqs_counter1;
	unsigned char dqs_i = 0;
	unsigned short debug_cnt_per_byte;
	unsigned char current_pass;
	unsigned char pass_byte_expected;
	static unsigned char pass_byte_count = 0;

	memset(debug_cnt, CLEAR_FLD, sizeof(debug_cnt));
	if (p->data_width == DATA_WIDTH_16BIT)
		pass_byte_expected = 0x3;
	else
		pass_byte_expected = 0xf;

	dramc_rx_dqs_cal_init(p, rxdqs_cal_ptr);


	/* read DQS counter */
	debug_cnt[0] = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT0));
	debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
	debug_cnt[0] &= 0xffff;

	dqs_counter0 = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT0));
	dqs_counter1 = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DQSGNWCNT1));

	if (p->channel == CHANNEL_A) {
		debug_cnt[0] = dqs_counter0;
		debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
		debug_cnt[0] &= 0xffff;

		if (p->data_width == DATA_WIDTH_32BIT) {
			debug_cnt[2] = dqs_counter1;
			debug_cnt[3] = (debug_cnt[2] >> 16) & 0xffff;
			debug_cnt[2] &= 0xffff;
		}
	} else {
		/* If CHB is used, the data width must be 16BIT */
		debug_cnt[0] = dqs_counter1;
		debug_cnt[1] = (debug_cnt[0] >> 16) & 0xffff;
		debug_cnt[0] &= 0xffff;
	}


	show_msg2((INFO, "%2d  %2d	%2d |(B3->B0) 0x%4x, 0x%4x, 0x%4x, 0x%4x\n",
		rxdqs_cal_ptr->dly_coarse_large, rxdqs_cal_ptr->dly_coarse_0p5t,
		rxdqs_cal_ptr->dly_fine_xt, debug_cnt[3], debug_cnt[2],
		debug_cnt[1], debug_cnt[0]));

	/* find gating window pass range per DQS separately */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		if (pass_byte_count & (1 << dqs_i)) /* real window found */
			continue;

		debug_cnt_per_byte = (unsigned short) debug_cnt[dqs_i];

		/* check if current tap is pass */
		current_pass = 0;

		if (debug_cnt_per_byte == GATING_GOLDEND_DQSCNT_LP4)
			current_pass = 1;

		/* if current tap is pass */
		if (current_pass) {
			dramc_rx_dqs_tap_check_pass_3g(
				p, rxdqs_trans_ptr, rxdqs_cal_ptr,
				rxdqs_win_ptr, dqs_i, coarse_end);
		} else {	/*	current tap is fail */
			if (rxdqs_win_ptr->pass_begin[dqs_i] == 1) {
				/* at the end of pass range */
				rxdqs_win_ptr->pass_begin[dqs_i] = 0;

			/* save the max range settings, to avoid glitch */
				if (rxdqs_win_ptr->pass_count_1[dqs_i] >
					rxdqs_win_ptr->pass_count[dqs_i]) {

					pass_byte_count =
						dramc_rx_dqs_max_range_3g(p,
						rxdqs_cal_ptr, rxdqs_win_ptr,
						dqs_i, pass_byte_count);

					if (pass_byte_count == pass_byte_expected) {
						show_log("All bytes gating ");
						show_log("window pass Done, ");
						show_log("Early break!\n");
						rxdqs_cal_ptr->dly_fine_xt =
							DQS_GW_FINE_END;
						coarse_tune = coarse_end;
					}
				}
			}
		}
	}
	return coarse_tune;
}

/* find center of each byte */
static void find_dqs_center_3g(DRAMC_CTX_T *p,
	RXDQS_GATING_TRANS_T *rxdqs_trans_ptr,
	RXDQS_GATING_CAL_T *rxdqs_cal_ptr,
	RXDQS_GATING_WIN_T *rxdqs_win_ptr,
	RXDQS_GATING_BEST_WIN_T *rxdqs_best_wins_ptr, unsigned char dqs_i,
	unsigned char rx_dly_dqsienstb_loop, unsigned char rx_dqs_ctl_loop,
	unsigned char freq_div)
{
	unsigned char tmp_offset, tmp_value;

#if (ENABLE_LEAD_LAG_FOR_DDR3 == 1)
	rxdqs_win_ptr->pass_count[dqs_i] =
		rxdqs_trans_ptr->dqs_transition[dqs_i];
	rxdqs_win_ptr->min_fine_tune[dqs_i] =
		rxdqs_trans_ptr->dly_fine_tune_leadLag[dqs_i];
	rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_0p5t_leadLag[dqs_i];
	rxdqs_win_ptr->min_coarse_tune2t[dqs_i] =
		rxdqs_trans_ptr->dly_coarse_large_leadLag[dqs_i];

	show_msg2((INFO, "FindCenter: %d, (%2d, %2d, %2d) %d\n",
		dqs_i, rxdqs_win_ptr->min_coarse_tune2t[dqs_i],
		rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i],
		rxdqs_win_ptr->min_fine_tune[dqs_i],
		rxdqs_win_ptr->pass_count[dqs_i]));
#endif

	/*	-- PI for Phase0 & Phase1 -- */
	tmp_offset = rxdqs_win_ptr->pass_count[dqs_i] *
		rxdqs_cal_ptr->dqs_gw_fine_step / 2;

	tmp_value = rxdqs_win_ptr->min_fine_tune[dqs_i] + tmp_offset;
	rxdqs_best_wins_ptr->best_fine_tune[dqs_i] =
		tmp_value % rx_dly_dqsienstb_loop;
	rxdqs_best_wins_ptr->best_fine_tune_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_fine_tune[dqs_i];

	/*	coarse tune 0.5T for Phase 0 */
	tmp_offset = tmp_value / rx_dly_dqsienstb_loop;
	tmp_value = rxdqs_win_ptr->min_coarse_tune0p5t[dqs_i] + tmp_offset;
	rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i] =
		tmp_value % rx_dqs_ctl_loop;

	/*	coarse tune 2T for Phase 0 */
	tmp_offset = tmp_value / rx_dqs_ctl_loop;
	rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] =
		rxdqs_win_ptr->min_coarse_tune2t[dqs_i] + tmp_offset;

	/*	coarse tune 0.5T for Phase 1 */
	tmp_value =
		rxdqs_best_wins_ptr->best_coarse_tune0p5t[dqs_i] + freq_div;
	rxdqs_best_wins_ptr->best_coarse_tune0p5t_p1[dqs_i] =
		tmp_value % rx_dqs_ctl_loop;

	/*	coarse tune 2T for Phase 1 */
	tmp_offset = tmp_value / rx_dqs_ctl_loop;
	rxdqs_best_wins_ptr->best_coarse_tune2t_p1[dqs_i] =
		rxdqs_best_wins_ptr->best_coarse_tune2t[dqs_i] + tmp_offset;
}

static DRAM_STATUS_T dramc_rx_dqs_gating_3g(DRAMC_CTX_T *p)
{
	unsigned char rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div;
	unsigned int value;
	unsigned char dqs_i = 0;

	show_msg3((INFO, "start DQS Gating cal\n"));

	unsigned char coarse_tune = 0, coarse_start = 0, coarse_end = BYTE_MAX;

	RXDQS_GATING_CAL_T rxdqs_cal;
	RXDQS_GATING_TRANS_T rxdqs_trans;
	RXDQS_GATING_WIN_T rxdqs_wins;
	RXDQS_GATING_BEST_WIN_T rxdqs_best_wins;

	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}

	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1)),
		(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMD)),
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DDRPHY_B0_DQ6),
		(DDRPHY_B1_DQ6),

		/* PHY B23 */
		((DDRPHY_B0_DQ6) +
			(CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ6) +
			(CHANNEL_B << POS_BANK_NUM)),
	};

	memset(&rxdqs_cal, 0x0, sizeof(RXDQS_GATING_CAL_T));
	memset(&rxdqs_trans, 0x0, sizeof(RXDQS_GATING_TRANS_T));
	memset(&rxdqs_wins, 0x0, sizeof(RXDQS_GATING_WIN_T));
	memset(&rxdqs_best_wins, 0x0, sizeof(RXDQS_GATING_BEST_WIN_T));

	/* Register backup */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	rx_dqs_gating_init_3g(p);

	rx_dly_dqsienstb_loop = 32; /* PI fine tune 0->31 */
	rx_dqs_ctl_loop = 8;	/* rx_dqs_ctl_loop is 8. */
	freq_div = 2;
	rxdqs_cal.dqs_gw_fine_step = DQS_GW_FINE_STEP;

	/*
	 * coarse_start
	 * 1. Depends on current freq's DQSINCTL setting
	 * 2. Preserves ~4UI before actual DQS delay value
	 */
#if 0
	if (p->frequency <= DDR1866_FREQ)
		coarse_start = RX_DQS_BEGIN_1600;
	else if (p->frequency <= DDR3200_FREQ)
		coarse_start = RX_DQS_BEGIN_3200;
	else if (p->frequency <= DDR3733_FREQ)
		coarse_start = RX_DQS_BEGIN_3733;
	else	/* 4266 */
		coarse_start = RX_DQS_BEGIN_4266;
#endif
	coarse_start = gw_corse_start[p->dram_type][p->freq_sel];
	coarse_end = coarse_start + RX_DQS_RANGE;

	for (coarse_tune = coarse_start; coarse_tune < coarse_end;
		coarse_tune += DQS_GW_COARSE_STEP) {

		rx_dqs_gating_set(p, coarse_tune, rx_dqs_ctl_loop,
			freq_div, &rxdqs_cal);

		for (rxdqs_cal.dly_fine_xt = DQS_GW_FINE_START;
			rxdqs_cal.dly_fine_xt < DQS_GW_FINE_END;
			rxdqs_cal.dly_fine_xt += rxdqs_cal.dqs_gw_fine_step) {
			coarse_tune = dramc_rx_dqs_cal_3g(p, &rxdqs_trans,
				&rxdqs_cal, &rxdqs_wins, coarse_tune,
				coarse_end);
		}
	}

	dramc_engine2_end(p);
	set_calibration_result(p, DRAM_CALIBRATION_GATING, DRAM_OK);

	/* find center of each byte */
	for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM); dqs_i++) {
		find_dqs_center_3g(p, &rxdqs_trans, &rxdqs_cal,
			&rxdqs_wins, &rxdqs_best_wins, dqs_i,
			rx_dly_dqsienstb_loop, rx_dqs_ctl_loop, freq_div);
	}

	show_msg2((INFO, "\tdqs input gating window, final dly value\n"));

	init_dqs_gating_nonused_bytes(p, &rxdqs_best_wins, dqs_i);
	set_dqs_gating(p, &rxdqs_best_wins, dqs_i);

	/* Restore registers */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/*
	 *LP4: Set ARDQ_RPRE_TOG_EN must be 1
	 *	after gating window calibration
	*/
	rx_dqs_isi_pulse_cg(p, ENABLE);

	/*	4T or 2T coarse tune */
	set_coarse_tune(p, &rxdqs_best_wins);

#if GATING_RODT_LATANCY_EN
	set_gating_rodt(p, &rxdqs_best_wins);
#endif

	if (p->channel == CHANNEL_B) {
		show_msg((INFO, "set CHANNEL_B best win\n"));
		rxdqs_best_wins.best_fine_tune[2] = rxdqs_best_wins.best_fine_tune[0];
		rxdqs_best_wins.best_fine_tune[3] = rxdqs_best_wins.best_fine_tune[1];
	}

	/*	Set Fine Tune Value to registers */
	value = rxdqs_best_wins.best_fine_tune[0] |
		(rxdqs_best_wins.best_fine_tune[1] << 8) |
		(rxdqs_best_wins.best_fine_tune[2] << 16) |
		(rxdqs_best_wins.best_fine_tune[3] << 24);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSIEN), value);

	dram_phy_reset(p);
	show_msg3((INFO, "[DramcRxdqsGatingCal] Done\n\n"));
	return DRAM_OK;
}

static DRAM_STATUS_T dramc_rx_dqs_gating_lp3(DRAMC_CTX_T *p)
{
	return dramc_rx_dqs_gating_3g(p);
}

static DRAM_STATUS_T dramc_rx_dqs_gating_ddr3(DRAMC_CTX_T *p)
{
	return dramc_rx_dqs_gating_3g(p);
}
#endif

#if GATING_ADJUST_TXDLY_FOR_TRACKING
static signed char dramc_txdly_for_tracking_init(DRAMC_CTX_T *p)
{
	unsigned int reg_tx_dly_dqsgated_min = 0;
	signed char change_dqs_inctl;

#ifdef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	/* DQSgated_min should be 2 when freq >= 1333, 1 when freq < 1333 */
	if (p->frequency >= DDR2666_FREQ)
		reg_tx_dly_dqsgated_min = 2;
	else
		reg_tx_dly_dqsgated_min = 1;
#else
	/* DQSgated_min should be 3 when freq >= 1333, 2 when freq < 1333 */
	if (p->frequency >= DDR2666_FREQ)
		reg_tx_dly_dqsgated_min = 3;
	else
		reg_tx_dly_dqsgated_min = 2;
#endif
	change_dqs_inctl =
		reg_tx_dly_dqsgated_min - tx_dly_cal_min[p->channel];

	show_msg((INFO, "%s%d\n %s %d, %s%d, tx_dly_cal_min %d\n",
		"[RxdqsGatingPostProcess] freq ", p->frequency,
		"ChangeDQSINCTL", change_dqs_inctl, "reg_tx_dly_dqsgated_min ",
		reg_tx_dly_dqsgated_min, tx_dly_cal_min[p->channel]));

	return change_dqs_inctl;
}

static void dramc_rxdqs_corsa_tune_set(DRAMC_CTX_T *p, unsigned char rank_idx)
{
	/* 4T or 2T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_DQSG0),
		p_fld((unsigned int)
		best_coarse_tune2t_backup
		[p->channel][rank_idx][0],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED)
		| p_fld((unsigned int)
		best_coarse_tune2t_backup
		[p->channel][rank_idx][1],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED)
		| p_fld((unsigned int)
		best_coarse_tune2t_backup
		[p->channel][rank_idx][2],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED)
		| p_fld((unsigned int)
		best_coarse_tune2t_backup
		[p->channel][rank_idx][3],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED)
		| p_fld((unsigned int)
		best_coarse_tune2t_p1_backup
		[p->channel][rank_idx][0],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS0_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune2t_p1_backup
		[p->channel][rank_idx][1],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS1_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune2t_p1_backup
		[p->channel][rank_idx][2],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS2_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune2t_p1_backup
		[p->channel][rank_idx][3],
		SHURK0_SELPH_DQSG0_TX_DLY_DQS3_GATED_P1));

	/*  0.5T coarse tune */
	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DRAMC_REG_SHURK0_SELPH_DQSG1),
		p_fld((unsigned int)
		best_coarse_tune0p5t_backup
		[p->channel][rank_idx][0],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_backup
		[p->channel][rank_idx][1],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_backup
		[p->channel][rank_idx][2],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_backup
		[p->channel][rank_idx][3],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_p1_backup
		[p->channel][rank_idx][0],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS0_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_p1_backup
		[p->channel][rank_idx][1],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS1_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_p1_backup
		[p->channel][rank_idx][2],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS2_GATED_P1)
		| p_fld((unsigned int)
		best_coarse_tune0p5t_p1_backup
		[p->channel][rank_idx][3],
		SHURK0_SELPH_DQSG1_REG_DLY_DQS3_GATED_P1));
}


static void dramc_rxdqs_gating_set(DRAMC_CTX_T *p,
	signed char change_dqs_inctl)
{
	unsigned int read_dqs_inctl, rank_inctl_root, xrtr2r;

	read_dqs_inctl =
		io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSCTL),
		SHURK0_DQSCTL_DQSINCTL);
	read_dqs_inctl -= change_dqs_inctl;

#ifdef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	/*
	* RANKINCTL_RXDLY = RANKINCTL = RankINCTL_ROOT = read_dqs_inctl-2,
	* if XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY enable
	*/

	/*  New algorithm : read_dqs_inctl-2 >= 0 */
	if (read_dqs_inctl >= 2) {
		rank_inctl_root = read_dqs_inctl - 2;
	} else {
		rank_inctl_root = 0;
		show_err("rank_inctl_root <2, Please check\n");
	}
#else
	/* Modify for corner IC failed at HQA test XTLV */
	if (read_dqs_inctl >= 3) {
		rank_inctl_root = read_dqs_inctl - 3;
	} else {
		rank_inctl_root = 0;
		show_err("rank_inctl_root <3, Risk for supporting 1066/RL8\n");
	}
#endif

	/* DQSINCTL */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_DQSCTL),
		read_dqs_inctl, SHURK0_DQSCTL_DQSINCTL);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHURK1_DQSCTL),
		read_dqs_inctl, SHURK1_DQSCTL_R1DQSINCTL);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_RANKCTL),
		read_dqs_inctl, SHU_RANKCTL_RANKINCTL_PHY);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_RANKCTL),
		rank_inctl_root, SHU_RANKCTL_RANKINCTL);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_RANKCTL),
		rank_inctl_root, SHU_RANKCTL_RANKINCTL_ROOT1);

#ifdef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_RANKCTL),
		rank_inctl_root, SHU_RANKCTL_RANKINCTL_RXDLY);

	xrtr2r =
	    io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM_XRT),
		SHU_ACTIM_XRT_XRTR2R);

	show_msg((INFO,
		"TX_dly_DQSgated check: min %d  max %d, ChangeDQSINCTL=%d\n",
		tx_dly_cal_min[p->channel],
		tx_dly_cal_max[p->channel], change_dqs_inctl));
	show_msg((INFO, "DQSINCTL=%d, RANKINCTL=%d, xrtr2r=%d\n",
		read_dqs_inctl, rank_inctl_root, xrtr2r));

#else
	/*
	* XRTR2R=A-phy forbidden margin(6T) + reg_TX_dly_DQSgated (max) +
	* Roundup(tDQSCKdiff/MCK+0.25MCK)+1(05T sel_ph margin) -
	* 1(forbidden margin overlap part)
	* Roundup(tDQSCKdiff/MCK+1UI) =1~2 all LP3 and LP4 timing
	* xrtr2r= 8 + tx_dly_cal_max;  6+ tx_dly_cal_max +2
	*/

	/* Modify for corner IC failed at HQA test XTLV @ 3200MHz */
	xrtr2r = 8 + tx_dly_cal_max[p->channel] + 1; /* 6+ tx_dly_cal_max +2 */
	if (xrtr2r > 12) {
		xrtr2r = 12;
		show_err("XRTR2R > 12, Max value is 12\n");
	}
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM_XRT), xrtr2r,
		SHU_ACTIM_XRT_XRTR2R);

	show_msg((INFO, "%smin %d  max %d, ChangeDQSINCTL=%d\n",
		"TX_dly_DQSgated check: ", tx_dly_cal_min[p->channel],
		tx_dly_cal_max[p->channel], change_dqs_inctl));
	show_msg((INFO, "DQSINCTL=%d, RANKINCTL=%d, xrtr2r=%d\n",
		read_dqs_inctl, rank_inctl_root, xrtr2r));

#endif
}

void dramc_rxdqs_gating_post_process(DRAMC_CTX_T *p)
{
	unsigned char dqs_i;
	unsigned char rank, rank_max;
	signed char change_dqs_inctl;
	unsigned int backup_rank;
	unsigned int read_tx_dly[RANK_MAX][DQS_NUMBER],
		read_tx_dly_p1[RANK_MAX][DQS_NUMBER];

	backup_rank = get_rank(p);

	change_dqs_inctl = dramc_txdly_for_tracking_init(p);

	/* need to change DQSINCTL and TXDLY of each byte */
	if (change_dqs_inctl != 0) {
		tx_dly_cal_min[p->channel] += change_dqs_inctl;
		tx_dly_cal_max[p->channel] += change_dqs_inctl;

		if (p->support_rank_num == RANK_DUAL)
			rank_max = RANK_MAX;
		else
			rank_max = RANK_1;

		for (rank = 0; rank < rank_max; rank++) {
			show_msg2((INFO, "Rank: %d\n", rank));
			for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM);
				dqs_i++) {
				read_tx_dly[rank][dqs_i] =
					best_coarse_tune2t_backup[p->channel]
					[rank][dqs_i];
				read_tx_dly_p1[rank][dqs_i] =
					best_coarse_tune2t_p1_backup
					[p->channel][rank][dqs_i];

				read_tx_dly[rank][dqs_i] += change_dqs_inctl;
				read_tx_dly_p1[rank][dqs_i] +=
					change_dqs_inctl;

				best_coarse_tune2t_backup[p->channel]
					[rank][dqs_i] =
					read_tx_dly[rank][dqs_i];
				best_coarse_tune2t_p1_backup[p->channel]
					[rank][dqs_i] =
					read_tx_dly_p1[rank][dqs_i];
				show_msg((INFO,
					"%s%d dly(2T, 0.5T) = (%d, %d)\n",
					"best DQS", dqs_i,
					best_coarse_tune2t_backup[p->channel]
					[rank][dqs_i],
					best_coarse_tune0p5t_backup
					[p->channel][rank][dqs_i]));
			}

			for (dqs_i = 0; dqs_i < (p->data_width / DQS_BIT_NUM);
			     dqs_i++) {
				show_msg((INFO,
					"%s%d P1 dly(2T, 0.5T) = (%d, %d)\n",
					"best DQS", dqs_i,
					best_coarse_tune2t_p1_backup
					[p->channel][rank][dqs_i],
					best_coarse_tune0p5t_p1_backup
					[p->channel][rank][dqs_i]));
			}
		}

		for (rank = 0; rank < rank_max; rank++) {
			set_rank(p, rank);
			dramc_rxdqs_corsa_tune_set(p, rank);
		}
	}
	set_rank(p, backup_rank);

	dramc_rxdqs_gating_set(p, change_dqs_inctl);

#if ENABLE_RODT_TRACKING
	/*
	 * The following 2 items are indepentent
	 * 1. if TX_WDQS on(by vendor_id) or p->odt_onoff = 1,
	 *	ROEN/RODTE/RODTE2 = 1
	 * 2. if ENABLE_RODT_TRACKING on, apply new setting and
	 *	RODTEN_MCK_MODESEL = ROEN
	 *  LP4 support only
	 */
	unsigned char u1ReadROEN;

	u1ReadROEN =
		io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_ODTCTRL),
		SHU_ODTCTRL_ROEN);
	io_32_write_fld_multi(
		DRAMC_REG_ADDR(DRAMC_REG_SHU_RODTENSTB),
		p_fld(0xff, SHU_RODTENSTB_RODTENSTB_EXT) |
		p_fld(9,	SHU_RODTENSTB_RODTENSTB_OFFSET) |
		p_fld(u1ReadROEN, SHU_RODTENSTB_RODTEN_MCK_MODESEL));
#endif
	set_rank(p, backup_rank);

}
#endif

#if GATING_ADJUST_TXDLY_FOR_TRACKING
void dramc_rxdqs_gating_pre_process(DRAMC_CTX_T *p)
{
	tx_dly_cal_min[p->channel] = BYTE_MAX;
	tx_dly_cal_max[p->channel] = 0;
}
#endif

/* Gating calibration is special for different types
 * need different algorithm.
 */
DRAM_STATUS_T dramc_rx_dqs_gating_cal(DRAMC_CTX_T *p)
{
	switch (p->dram_type) {
	#if SUPPORT_TYPE_LPDDR4
	case TYPE_LPDDR4:
		dramc_rx_dqs_gating_lp4(p);
		break;
	#endif

	#if SUPPORT_TYPE_LPDDR3
	case TYPE_LPDDR3:
		dramc_rx_dqs_gating_lp3(p);
		break;
	#endif

	#if SUPPORT_TYPE_PCDDR4
	case TYPE_PCDDR4:
		dramc_rx_dqs_gating_ddr4(p);
		break;
	#endif

	#if SUPPORT_TYPE_PCDDR3
	case TYPE_PCDDR3:
		dramc_rx_dqs_gating_ddr3(p);
		break;
	#endif

	default:
		show_err("[Error] Gating: Unrecognized DRAM type\n");
		break;
	}

	return DRAM_OK;
}

#endif /* SIMULATION_GATING */


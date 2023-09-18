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

#define OSC_CNT_NUM	2
#define TERM_OPTION_NUM	2

#define DQSOSCTHRD_INC 0x06
#define DQSOSCTHRD_DEC 0x04

#define PRD_INT	0x3ff
#define DQS_OSC_INT	0x1ff

#define IMPEDANCE_MAX	31

/* Definitions to make IMPCAL_VREF_SEL function more readable */
#define IMPCAL_STAGE_DRVP     1
#define IMPCAL_STAGE_DRVN     2
#define IMPCAL_STAGE_TRACKING 3

/* LP4 IMP_VREF_SEL ============================== */
#define IMP_LP4_VREF_SEL               0x1b

/* LP4X IMP_VREF_SEL ============================== */
#define IMP_LP4X_TERM_VREF_SEL     0x1b

/* LP4X IMP_VREF_SEL w/o term ==== */
#define IMP_DRVP_LP4X_UNTERM_VREF_SEL   0x1a
#define IMP_DRVN_LP4X_UNTERM_VREF_SEL   0x16
#define IMP_TRACK_LP4X_UNTERM_VREF_SEL  0x1a

/* LP4P IMP_VREF_SEL ============================== */
#define IMP_DRVP_LP4P_VREF_SEL        0x13
#define IMP_DRVN_LP4P_VREF_SEL        0xf
#define IMP_TRACK_LP4P_VREF_SEL       0x13

/* -----------------------------------------------------------------------------
 *  Global variables
 * -----------------------------------------------------------------------------
 */
MR_SET_VALUE_T dram_mr;

/* DQSOSCTHRD_INC & _DEC are 12 bits */
unsigned short dqsosc_thrd_inc[CHANNEL_NUM][RANK_MAX];
unsigned short dqsosc_thrd_dec[CHANNEL_NUM][RANK_MAX];
unsigned short dqsosc[CHANNEL_NUM][RANK_MAX];
unsigned short dqs_osc_cnt_val[CHANNEL_NUM][RANK_MAX][OSC_CNT_NUM];

signed int wl_final_delay[CHANNEL_NUM][DQS_NUMBER];
#ifdef ENABLE_MIOCK_JMETER
unsigned char num_dlycell_per_t[CHANNEL_NUM] = { 49 };
#endif
unsigned char num_dlycell_per_t_all[DRAM_DFS_SHUFFLE_MAX][CHANNEL_NUM];
unsigned short delay_cell_ps[CHANNEL_NUM];
unsigned short delay_cell_ps_all[DRAM_DFS_SHUFFLE_MAX][CHANNEL_NUM];
unsigned int vcore_value[DRAM_DFS_SHUFFLE_MAX];

enum {
	TERM_TYPE_DRVP = 0,
	TERM_TYPE_DRVN,
	TERM_TYPE_ODTP,
	TERM_TYPE_ODTN,
	TERM_TYPE_NUM,
};

static unsigned int dramc_imp_result[TERM_OPTION_NUM][TERM_TYPE_NUM] = {
	{0, 0, 0, 0}, {0, 0, 0, 0} };

signed char final_k_dqs_clk_delay_cell[DQS_NUMBER];

/* dramc_new_duty_calibration backup register value */
void init_global_variables_by_condition(void)
{
	unsigned char ch_idx, rank_idx, fsp_idx;

	dram_mr.mr01_value[FSP_0] = MR01_FSP0_INIT;
	dram_mr.mr01_value[FSP_1] = MR01_FSP1_INIT;

	/* LP4 default 0x4d, LP4X 0x5d */
	//memset(dram_mr.mr12_value, MR12_INIT, sizeof(dram_mr.mr12_value)); /* ca vref */
	for (ch_idx = 0; ch_idx < CHANNEL_NUM; ch_idx++)
		for (rank_idx = 0; rank_idx < RANK_MAX; rank_idx++)
			for (fsp_idx = 0; fsp_idx < FSP_MAX; fsp_idx++) {
				dram_mr.mr12_value[ch_idx][rank_idx][fsp_idx] =  /* tx vref */
					(fsp_idx == FSP_0) ?
					MR12_INIT : MR12_INIT;
			}



	dram_mr.mr13_value[FSP_0] = MR13_FSP0_INIT;
	dram_mr.mr13_value[FSP_1] = MR13_FSP1_INIT;

	for (fsp_idx = 0; fsp_idx < FSP_MAX; fsp_idx++) {
		dram_mr.mr02_value[fsp_idx] = MR02_INIT;
		dram_mr.mr03_value[fsp_idx] = MR03_INIT;
	}

	for (ch_idx = 0; ch_idx < CHANNEL_NUM; ch_idx++)
		for (rank_idx = 0; rank_idx < RANK_MAX; rank_idx++)
			for (fsp_idx = 0; fsp_idx < FSP_MAX; fsp_idx++) {
				/*
				* MR14 default value, LP4 default 0x4d,
				* LP4X 0x5d
				*/
				dram_mr.mr14_value[ch_idx][rank_idx][fsp_idx] =  /* tx vref */
					(fsp_idx == FSP_0) ?
					MR14_FSP0_INIT : MR14_FSP1_INIT;
			}

	memset(dram_mr.mr23_value, MR23_INIT, sizeof(dram_mr.mr23_value));
	memset(dqsosc_thrd_inc, DQSOSCTHRD_INC, sizeof(dqsosc_thrd_inc));
	memset(dqsosc_thrd_dec, DQSOSCTHRD_DEC, sizeof(dqsosc_thrd_dec));
	memset(rx_dqs_duty_offset, CLEAR_FLD, sizeof(rx_dqs_duty_offset));

	for (ch_idx = 0; ch_idx < CHANNEL_NUM; ch_idx++) {
	#if SIMULATION_WRITE_LEVELING
		wrlevel_done[ch_idx] = 0;
	#endif

	#ifdef ENABLE_MIOCK_JMETER
		num_dlycell_per_t[ch_idx] = 49;
	#endif

	#if GATING_ADJUST_TXDLY_FOR_TRACKING
		tx_dly_cal_min[ch_idx] = BYTE_MAX;
		tx_dly_cal_max[ch_idx] = 0;
	#endif
	}
}

void set_channel_number(DRAMC_CTX_T *p)
{
#if 0  /* Fix to single Physic channel number */
	#if (CHANNEL_NUM == 4)
	p->support_channel_num = CHANNEL_NUM;	/*4 channel*/
	#elif (CHANNEL_NUM == 2)
	p->support_channel_num = CHANNEL_DUAL;
	#else
	p->support_channel_num = CHANNEL_SINGLE;
	#endif
#endif
	p->support_channel_num = CHANNEL_SINGLE;
}

void set_rank_number(DRAMC_CTX_T *p)
{
#if 0
	if ((io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RSTMASK),
		RSTMASK_RSV_DRAM_SUPPORT_RANK_NUM)) == 0)
		p->support_rank_num = RANK_DUAL;
	else
		p->support_rank_num = RANK_SINGLE;
#endif
	/* cc change. Weber rank number is fixed as Single */
	p->support_rank_num = RANK_SINGLE;
}

void set_phy_2_channel_mapping(DRAMC_CTX_T *p, unsigned char channel)
{
	/* For Weber, Fix the mapping since only 1 channel is supported */
	if ((p->dram_type == TYPE_LPDDR4) ||
		((p->dram_type == TYPE_PCDDR3) &&
		(p->data_width == DATA_WIDTH_16BIT))) {
		p->channel = CHANNEL_B;
	} else {
		p->channel = CHANNEL_A;
	}

#if 0
	p->channel = channel;
#endif
}

unsigned char get_phy_2_channel_mapping(DRAMC_CTX_T *p)
{
	return p->channel;
}

void set_rank(DRAMC_CTX_T *p, unsigned char rank)
{
	p->rank = rank;
}

unsigned char get_rank(DRAMC_CTX_T *p)
{
	return p->rank;
}

/*
 * get_dram_cbt_mode
 * Due to current HW design (both ranks share the same set of ACTiming regs),
 * mixed mode LP4 now uses byte mode ACTiming settings. This means most
 * calibration steps should use byte mode code flow.
 * Note: The below items must have per-rank settings (Don't use this function)
 * 1. CBT training 2. TX tracking
 */
DRAM_CBT_MODE_T get_dram_cbt_mode(DRAMC_CTX_T *p)
{
	if (p->dram_cbt_mode[RANK_0] == CBT_NORMAL_MODE
		&& p->dram_cbt_mode[RANK_1] == CBT_NORMAL_MODE)
		return CBT_NORMAL_MODE;
	else	/* For Mixed mode & Byte mode LP4 */
		return CBT_BYTE_MODE1;
}

#if PRINT_CALIBRATION_SUMMARY
void set_calibration_result(DRAMC_CTX_T *p, unsigned char cal_type,
	unsigned char result)
{
	p->cal_execute_flag[p->channel][p->rank] |= (1 << cal_type);
	if (result == DRAM_OK)
		p->cal_result_flag[p->channel][p->rank] &= (~(1 << cal_type));
	else
		p->cal_result_flag[p->channel][p->rank] |= (1 << cal_type);
}

const char *calib_status_name[DRAM_CALIBRATION_MAX] = {
	"ZQ Calibration",
	"SW Impedance",
	"CA Training",
	"Write leveling",
	"RX DQS gating",
	"RX DATLAT",
	"RX DQ/DQS(RDDQC)",
	"RX DQ/DQS(Engine)",
	"TX DQ/DQS",
};
void print_calibration_result2(DRAMC_CTX_T *p,
	unsigned char ch_idx, unsigned char rank_idx)
{
	unsigned char cal_idx;
	unsigned int cal_result_all, cal_execute_all;
	unsigned char cal_result, cal_execute;
	unsigned char calibration_fail = 0;

	cal_execute_all =
		p->cal_execute_flag[ch_idx][rank_idx];
	cal_result_all = p->cal_result_flag[ch_idx][rank_idx];
	show_msg((INFO, "CH %d, Rank %d\n", ch_idx,
		rank_idx));

	for (cal_idx = 0; cal_idx < DRAM_CALIBRATION_MAX; cal_idx++) {
		cal_execute = (unsigned char)
			((cal_execute_all >> cal_idx) & 0x1);
		cal_result = (unsigned char)
			((cal_result_all >> cal_idx) & 0x1);

		/* excuted and fail */
		if (cal_execute == 1 && cal_result == 1) {
			calibration_fail = 1;
			show_msg((INFO, "%s: %s\n",
				calib_status_name[cal_idx],
				((cal_result == 0) ? "OK" : "Fail")));
		}
	}

	if (calibration_fail == 0)
		show_log("All Pass.\n");
	show_log("\n");
}

void print_calibration_result(DRAMC_CTX_T *p)
{
	unsigned char ch_idx, rank_idx;

	show_msg((INFO, "\n\n[Calibration Summary] Freqency %d\n",
		p->frequency));

	for (ch_idx = 0; ch_idx < p->support_channel_num; ch_idx++)
		for (rank_idx = 0; rank_idx < p->support_rank_num; rank_idx++)
			print_calibration_result2(p, ch_idx, rank_idx);

	memset(p->cal_result_flag, CLEAR_FLD, sizeof(p->cal_result_flag));
	memset(p->cal_execute_flag, CLEAR_FLD, sizeof(p->cal_execute_flag));
}
#endif

void print_calibration_basic_info(DRAMC_CTX_T *p)
{
#ifndef PARALLEL_CH_CAL
	show_msg((INFO,
		"==========================================================\n"
		"Dram Type= %d, Freq= %u, FreqGroup= %u, CH_%d, rank %d\n"
		"fsp= %d, odt_onoff= %d, Byte mode= %d\n"
		"==========================================================\n",
		p->dram_type, p->frequency, p->freqGroup, p->channel,
		p->rank, p->dram_fsp, p->odt_onoff,
		p->dram_cbt_mode[p->rank]));
#endif
}

void apply_config_after_calibration(DRAMC_CTX_T *p)
{
	/* PHY RX Settings */
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL4, 0x11400000,
		MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);

	/* Burst mode settings are removed from here due to
	*  1. Set in update_initial_settings_lp4
	*  2. DQS Gating ensures new burst mode is switched when to done
	*  (or doesn't switch gatingMode at all, depending on
	*  "LP4_GATING_OLD_BURST_MODE")
	*/

	io_32_write_fld_align_all(DDRPHY_CA_CMD6, CLEAR_FLD,
		CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN);
	/* DA mode */
	io_32_write_fld_align_all(DDRPHY_B0_DQ6, CLEAR_FLD,
		B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, CLEAR_FLD,
		B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, CLEAR_FLD,
		CA_CMD6_RG_RX_ARCMD_BIAS_PS);

	io_32_write_fld_align_all(DDRPHY_B0_DQ6, SET_FLD,
		B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, SET_FLD,
		B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN);

	/* IMPCAL Settings */
	/* RG_RIMP_BIAS_EN and RG_RIMP_VREF_EN move to IMPPDP and IMPPDN */
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL),
		p_fld(CLEAR_FLD, IMPCAL_IMPCAL_IMPPDP) |
		p_fld(CLEAR_FLD, IMPCAL_IMPCAL_IMPPDN));

	/*
	* MR1 op[7]=0 already be setted at end of gating calibration,
	* no need to set here again
	* Prevent M_CK OFF because of hardware auto-sync
	*/
	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL0, CLEAR_FLD,
		ffld(4, CLEAR_FLD, AC_MSKB0));

	/*DFS- fix Gating Tracking settings*/
	io_32_write_fld_align_all(DDRPHY_MISC_CTRL0, CLEAR_FLD,
		MISC_CTRL0_R_STBENCMP_DIV4CK_EN);
	io_32_write_fld_align_all(DDRPHY_MISC_CTRL1, CLEAR_FLD,
		MISC_CTRL1_R_DMSTBENCMP_RK_OPT);

	/* TODO: Disable MR4 MR18/MR19, TxHWTracking, Dummy RD */
	/* MR4 Disable */
	io_32_write_fld_align_all(DRAMC_REG_SPCMDCTRL, SET_FLD,
		SPCMDCTRL_REFRDIS);
	/* MR18, MR19 Disable */
	io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, SET_FLD,
		DQSOSCR_DQSOSCRDIS);
	io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, SET_FLD,
		DQSOSCR_DQSOSCENDIS);

	io_32_write_fld_multi_all(DRAMC_REG_DUMMY_RD,
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_EN)
		| p_fld(CLEAR_FLD, DUMMY_RD_SREF_DMYRD_EN)
		| p_fld(CLEAR_FLD, DUMMY_RD_DQSG_DMYRD_EN)
		| p_fld(CLEAR_FLD, DUMMY_RD_DMY_RD_DBG));

	/* CKE dynamic */
	cke_fix_on_off(p, CKE_DYNAMIC, CKE_WRITE_TO_ALL_CHANNEL);
	/* Enable  HW MIOCK control to make CLK dynamic */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		CLEAR_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF);

	/* close eyescan to save power */
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN),
		p_fld(CLEAR_FLD, EYESCAN_EYESCAN_DQS_SYNC_EN)
		| p_fld(CLEAR_FLD, EYESCAN_EYESCAN_NEW_DQ_SYNC_EN)
		| p_fld(CLEAR_FLD, EYESCAN_EYESCAN_DQ_SYNC_EN));

	/* TESTAGENT2 */
	/* Rank selection is controlled by Test Agent */
	io_32_write_fld_align_all(DRAMC_REG_TEST2_4, 4,
		TEST2_4_TESTAGENTRKSEL);
}

void reset_delay_chain_before_calibration(DRAMC_CTX_T *p)
{
	unsigned char rank_idx, rank_idx_bak;

	rank_idx_bak = get_rank(p);

	/* Set LP4 Rank0/1 CA/TX delay chain to 0 */
	/*
	 * CA0~9 per bit delay line -> CHA_CA0 CHA_CA3 CHA_B0_DQ6
	 * CHA_B0_DQ7 CHA_B0_DQ2 CHA_B0_DQ5 CHA_B0_DQ4 CHA_B0_DQ1
	 * CHA_B0_DQ0 CHA_B0_DQ3
	 */
	for (rank_idx = RANK_0; rank_idx < p->support_rank_num; rank_idx++) {
		set_rank(p, rank_idx);

		io_32_write_fld_multi_all(DDRPHY_SHU1_R0_CA_CMD0,
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA5_DLY) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA4_DLY) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA3_DLY) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA2_DLY) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA1_DLY) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD0_RK0_TX_ARCA0_DLY));
		io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ0,
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0) |
			p_fld(CLEAR_FLD, SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ0,
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1) |
			p_fld(CLEAR_FLD, SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1));
		io_32_write_fld_align_all(DDRPHY_SHU1_R0_B0_DQ1, CLEAR_FLD,
			SHU1_R0_B0_DQ1_RK0_TX_ARDQM0_DLY_B0);
		io_32_write_fld_align_all(DDRPHY_SHU1_R0_B1_DQ1, CLEAR_FLD,
			SHU1_R0_B1_DQ1_RK0_TX_ARDQM0_DLY_B1);
	}

	set_rank(p, rank_idx_bak);
}

static void apply_config_before_calibration1(DRAMC_CTX_T *p)
{
	unsigned char shu_index;

	/* Clk free run */
	enable_dramc_phy_dcm(p, 0);

	reset_delay_chain_before_calibration(p);

	/* MR4 refresh cnt set to 0x1ff (2ms update) */
	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF3, 0x1ff,
		SHU_CONF3_REFRCNT);

	/*
	* The counter for Read MR4 cannot be reset after SREF
	* if DRAMC no power down.
	*/

	/* ---- ZQ CS init -------- */
	/*
	* ZQ Calibration Time, unit: 38.46ns, tZQCAL min is 1 us.
	* need to set larger than 0x1b
	*/
	io_32_write_fld_align_all(DRAMC_REG_SHU_SCINTV,
		0x1f, SHU_SCINTV_TZQLAT);
	for (shu_index = DRAM_DFS_SHUFFLE_1; shu_index < DRAM_DFS_SHUFFLE_MAX;
		shu_index++) {
		/* Every refresh number to issue ZQCS commands */
		io_32_write_fld_align_all(
			DRAMC_REG_SHU_CONF3 + SHU_GRP_DRAMC_OFFSET * shu_index,
			0x1ff, SHU_CONF3_ZQCSCNT);
	}
	/*
	* HW send ZQ command for both rank,
	* disable it due to some dram only have 1 ZQpin for two rank.
	*/
	io_32_write_fld_align_all(DRAMC_REG_DRAMCTRL,
		CLEAR_FLD, DRAMCTRL_ZQCALL);

	/* Dual channel ZQCS interlace,  0: disable, 1: enable */
	if (p->support_channel_num == CHANNEL_SINGLE) {
		/* single channel, ZQCSDUAL=0, ZQCSMASK=0 */
		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_ZQCS),
			p_fld(CLEAR_FLD, ZQCS_ZQCSDUAL) |
			p_fld(CLEAR_FLD,	ZQCS_ZQCSMASK));
	}

	/* Disable LP3 HW ZQ, LP3 ZQCSDISB=0 */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL),
		CLEAR_FLD, SPCMDCTRL_ZQCSDISB);
	/* Disable LP4 HW ZQ,LP4 ZQCALDISB=0 */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL),
		CLEAR_FLD, SPCMDCTRL_ZQCALDISB);
	/* ---- End of ZQ CS init ----- */

}

void apply_config_before_calibration(DRAMC_CTX_T *p)
{
	apply_config_before_calibration1(p);

#if ENABLE_TX_TRACKING
	io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, p->dram_cbt_mode[RANK_0],
		DQSOSCR_RK0_BYTE_MODE);
	io_32_write_fld_align_all(DRAMC_REG_DQSOSCR, p->dram_cbt_mode[RANK_1],
		DQSOSCR_RK1_BYTE_MODE);
#endif
	/* Disable write-DBI of DRAMC */
	dramc_write_dbi_on_off(p, DBI_OFF);
	/* Disable read-DBI of DRAMC */
	dramc_read_dbi_on_off(p, DBI_OFF);
	/* disable MR4 read, REFRDIS=1 */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL),
		SET_FLD, SPCMDCTRL_REFRDIS);
	/* MR18, MR19 Disable */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR),
		SET_FLD, DQSOSCR_DQSOSCRDIS);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR),
		SET_FLD, DQSOSCR_DQSOSCENDIS);

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DRAMC_REG_DUMMY_RD),
		p_fld(CLEAR_FLD, DUMMY_RD_DUMMY_RD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_SREF_DMYRD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DQSG_DMYRD_EN) |
		p_fld(CLEAR_FLD, DUMMY_RD_DMY_RD_DBG));

	/*
	 * Disable HW gating tracking first, 0x1c0[31], need to disable
	 * both UI and PI tracking or the gating delay reg won't be valid.
	 */
	dramc_hw_gating_on_off(p, 0);

	/* ARPI_DQ SW mode mux, TX DQ use 1: PHY Reg 0: DRAMC Reg */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1),
		SET_FLD, MISC_CTRL1_R_DMARPIDQ_SW);

	/* Set to all-bank refresh */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0),
		CLEAR_FLD, REFCTRL0_PBREFEN);

	/* set MRSRK to 0, MPCRKEN always set 1 (Derping) */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_MRS),
		CLEAR_FLD, MRS_MRSRK);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION),
		SET_FLD, MPC_OPTION_MPCRKEN);

	/* RG mode */
	io_32_write_fld_align_all(DDRPHY_B0_DQ6, SET_FLD,
		B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ6, SET_FLD,
		B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
	io_32_write_fld_align_all(DDRPHY_CA_CMD6, SET_FLD,
		CA_CMD6_RG_RX_ARCMD_BIAS_PS);

#if ENABLE_RX_TRACKING_LP4
	dramc_rx_input_delay_tracking_init_by_freq(p);
#endif

#ifdef DUMMY_READ_FOR_TRACKING
	io_32_write_fld_align_all(DRAMC_REG_DUMMY_RD, SET_FLD,
		DUMMY_RD_DMY_RD_RX_TRACK);
#endif

}

/* cc notes: from Azalea */
void dram_phy_reset(DRAMC_CTX_T *p)
{
	/* cc mark if (is_lp4_family(p->dram_type)) { */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0), 1,
		DDRCONF0_RDATRST);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 1,
		MISC_CTRL1_R_DMPHYRST);

	/*
	 * RG_ARCMD_RESETB & RG_ARDQ_RESETB_B0/1 only reset once at init,
	 * Justin Chan.
	 * TODO:needtoconfirmRG_ARCMD_RESETB&RG_ARDQ_RESETB_B0/1isresetatmem.c
	 */
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
		p_fld(0, B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB) |
		p_fld(0, B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
		p_fld(0, B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB) |
		p_fld(0, B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1));
#ifdef LOOPBACK_TEST
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_CA_CMD3),
		p_fld(0, CA_CMD3_RG_RX_ARCLK_STBEN_RESETB) |
		p_fld(0, CA_CMD3_RG_RX_ARCMD_STBEN_RESETB));
#endif
	delay_us(1);
#ifdef LOOPBACK_TEST
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_CA_CMD3),
		p_fld(1, CA_CMD3_RG_RX_ARCLK_STBEN_RESETB) |
		p_fld(1, CA_CMD3_RG_RX_ARCMD_STBEN_RESETB));
#endif
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
		p_fld(1, B1_DQ3_RG_RX_ARDQS1_STBEN_RESETB) |
		p_fld(1, B1_DQ3_RG_RX_ARDQ_STBEN_RESETB_B1));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
		p_fld(1, B0_DQ3_RG_RX_ARDQS0_STBEN_RESETB) |
		p_fld(1, B0_DQ3_RG_RX_ARDQ_STBEN_RESETB_B0));

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 0,
		MISC_CTRL1_R_DMPHYRST);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DDRCONF0), 0,
		DDRCONF0_RDATRST);
}


DRAM_STATUS_T dramc_rank_swap(DRAMC_CTX_T *p, unsigned char rank)
{
#if 0 /* cc mark for only single rank supported */
	unsigned char multi;
	if (p->support_rank_num > 1)
		multi = 1;
	else
		multi = 0;

	show_msg((INFO, "[RankSwap] Rank num %d, (Multi %d), Rank %d\n",
		p->support_rank_num, multi, rank));

	/* Set to non-zero for multi-rank */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), multi,
		RKCFG_RKMODE);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), rank,
		RKCFG_RKSWAP);

	/* use other rank's setting TXRANK should be set before TXRANKFIX*/
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), rank,
		RKCFG_TXRANK);

	if (rank == 0)
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			CLEAR_FLD, RKCFG_TXRANKFIX);
	else
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			SET_FLD, RKCFG_TXRANKFIX);
#endif

	return DRAM_OK;
}

DRAM_STATUS_T dramc_start_dqsosc(DRAMC_CTX_T *p)
{
	unsigned int response;
	unsigned int time_cnt;

	time_cnt = TIME_OUT_CNT;
	show_log("[DQSOSC]\n");

	/*
	 * R_DMDQSOSCENEN, 0x1E4[10]=1 for DQSOSC Start
	 * Wait dqsoscen_response=1 (dramc_conf_nao, 0x3b8[29])
	 * R_DMDQSOSCENEN, 0x1E4[10]=0
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_DQSOSCENEN);
	do {
		response =
			io_32_read_fld_align(
			DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP),
			SPCMDRESP_DQSOSCEN_RESPONSE);
		time_cnt--;
		delay_us(1);
	} while ((response == 0) && (time_cnt > 0));

	if (time_cnt == 0)	{
		show_log("Start fail (time out)\n");
		return DRAM_FAIL;
	}
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		CLEAR_FLD, SPCMD_DQSOSCENEN);
	return DRAM_OK;
}

DRAM_STATUS_T dramc_dqsosc_auto(DRAMC_CTX_T *p)
{
	unsigned char mr23 = dram_mr.mr23_value[p->channel][p->rank];
	unsigned short mr18, mr19;
	unsigned short dqs_cnt;
	unsigned short dqs_osc[2];
	unsigned int reg_bak[3];

	reg_bak[0] = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_MRS));
	reg_bak[1] = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL));
	reg_bak[2] = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL));

	/* !!R_DMMRSRK(R_DMMPCRKEN=1) specify rank0 or rank1 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), CLEAR_FLD,
		RKCFG_DQSOSC2RK);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), get_rank(p),
		MRS_MRSRK);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION), SET_FLD,
		MPC_OPTION_MPCRKEN);

	/*
	 * LPDDR4-3200,     PI resolution = tCK/64 =9.76ps
	 * Only if MR23>=16, then error < PI resolution.
	 * Set MR23 == 0x3f, stop after 63*16 clock
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), get_rank(p),
		MRS_MRSRK);
	dramc_mode_reg_write(p, MR23, mr23);

	/* SW mode */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR),
		SET_FLD, DQSOSCR_DQSOSCENDIS);

	/* MIOCKCTRLOFF=1 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		SET_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF);

	cke_fix_on_off(p, CKE_FIXON, CKE_WRITE_TO_ONE_CHANNEL);

	dramc_start_dqsosc(p);
	delay_us(1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), get_rank(p),
		MRS_MRRRK);
	dramc_mode_reg_read(p, MR18, &mr18);
	dramc_mode_reg_read(p, MR19, &mr19);

	/* B0 */
	dqs_cnt = (mr18 & 0x00ff) | ((mr19 & 0x00ff) << 8);
	if (dqs_cnt != 0) /* tDQSOSC = 16*MR23*tCK/2*count */
		dqs_osc[0] = mr23 * 16 * 1000000 / (2 * dqs_cnt * p->frequency);
	else
		dqs_osc[0] = 0;

	/* B1 */
	dqs_cnt = (mr18 >> 8) | ((mr19 & 0xff00));
	if (dqs_cnt != 0) /* tDQSOSC = 16*MR23*tCK/2*count */
		dqs_osc[1] = mr23 * 16 * 1000000 / (2 * dqs_cnt * p->frequency);
	else
		dqs_osc[1] = 0;
	show_msg2((INFO,
		"%s%d%s0x%x, (MSB)MR19=0x%x,%s%dps tDQSOscB1=%dps\n",
		"[DQSOSCAuto] RK", get_rank(p), ", (LSB)MR18=", mr18,
		mr19, " tDQSOscB0=", dqs_osc[0], dqs_osc[1]));

	dram_mr.mr18_value[p->channel][p->rank] = mr18;
	dram_mr.mr19_value[p->channel][p->rank] = mr19;
	dqsosc[p->channel][p->rank] = dqs_osc[0];

	if (dqs_osc[1] != 0 && dqs_osc[1] < dqs_osc[0])
		dqsosc[p->channel][p->rank] = dqs_osc[1];

	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_MRS), reg_bak[0]);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), reg_bak[1]);
	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL), reg_bak[2]);

	/* MR23 should be 0x3F for all case ("HW tracking modify" DVT)
	 * -> Value is already set in during mr23_value array init
	 * (Adjusts MR23 according to freq, since back then _INC _DEC bit's
	 * weren't sufficient)
	 */

	return DRAM_OK;
}

/* Using dqsosc results calculated from dramc_dqsosc_auto
 * -> calculate DQSOSCTHRD_INC, DQSOSCTHRD_DEC
 */
DRAM_STATUS_T dramc_dqsosc_mr23(DRAMC_CTX_T *p)
{
	unsigned char mr23 = dram_mr.mr23_value[p->channel][p->rank];
	unsigned short dqs_osc = dqsosc[p->channel][p->rank];
	unsigned int tck = 1000000 / p->frequency;

	dqsosc_thrd_inc[p->channel][p->rank] =
		(mr23 * tck * tck) / (dqs_osc * dqs_osc * 10);
	dqsosc_thrd_dec[p->channel][p->rank] =
		(3 * mr23 * tck * tck) / (dqs_osc * dqs_osc * 20);
	show_msg((INFO,
		"CH%d_RK%d: MR19=0x%X, MR18=0x%X%s%d, MR23=%u%s%u, DEC=%u\n",
		p->channel, p->rank, dram_mr.mr19_value[p->channel][p->rank],
		dram_mr.mr18_value[p->channel][p->rank], ", DQSOSC=",
		dqsosc[p->channel][p->rank], mr23, ", INC=",
		dqsosc_thrd_inc[p->channel][p->rank],
		dqsosc_thrd_dec[p->channel][p->rank]));
	return DRAM_OK;
}

/* Sets DQSOSC_BASE for specified rank/byte */
DRAM_STATUS_T dramc_dqsosc_set_mr18_mr19(DRAMC_CTX_T *p)
{
	unsigned short dqs_osc_cnt[2];

	dramc_dqsosc_auto(p);

	/* B0 */
	dqs_osc_cnt_val[p->channel][p->rank][0] = dqs_osc_cnt[0] =
		(dram_mr.mr18_value[p->channel][p->rank] & 0x00ff) |
		((dram_mr.mr19_value[p->channel][p->rank] & 0x00ff) << 8);
	/* B1 */
	dqs_osc_cnt_val[p->channel][p->rank][1] = dqs_osc_cnt[1] =
		(dram_mr.mr18_value[p->channel][p->rank] >> 8) |
		((dram_mr.mr19_value[p->channel][p->rank] & 0xff00));

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1RK0_DQSOSC),
		p_fld(dqs_osc_cnt[0], SHU1RK0_DQSOSC_DQSOSC_BASE_RK0) |
		p_fld(dqs_osc_cnt[1], SHU1RK0_DQSOSC_DQSOSC_BASE_RK0_B1));

	show_msg((INFO, "CH%d RK%d: MR19=%X, MR18=%X\n", p->channel,
			p->rank, dram_mr.mr19_value[p->channel][p->rank],
			dram_mr.mr18_value[p->channel][p->rank]));
	return DRAM_OK;
}

/* cc notes: from Azalea */
DRAM_STATUS_T dramc_dqsosc_shu_settings(DRAMC_CTX_T *p)
{
	unsigned short prdcnt = 0x3FF;
	unsigned short dqsoscencnt = 0x1FF;
	unsigned char thrd_inc, thrd_dec;

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_SCINTV), CLEAR_FLD,
		SHU_SCINTV_DQS2DQ_SHU_PITHRD);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RK0_DQSOSC), CLEAR_FLD,
		RK0_DQSOSC_DQS2DQ_FILT_OPT);

	if (p->frequency <= DDR1600_FREQ) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_SCINTV), 0x5,
			SHU_SCINTV_DQS2DQ_FILT_PITHRD);
		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
			p_fld(0x5, SHU1_WODT_TXUPD_W2R_SEL) |
			p_fld(CLEAR_FLD, SHU1_WODT_TXUPD_SEL));
	} else if (p->frequency <= DDR2666_FREQ) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_SCINTV), 0x8,
			SHU_SCINTV_DQS2DQ_FILT_PITHRD);
		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
			p_fld(0x2, SHU1_WODT_TXUPD_W2R_SEL) |
			p_fld(CLEAR_FLD, SHU1_WODT_TXUPD_SEL));
	} else if (p->frequency <= DDR3200_FREQ) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_SCINTV), 0xA,
			SHU_SCINTV_DQS2DQ_FILT_PITHRD);
		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
			p_fld(0x2, SHU1_WODT_TXUPD_W2R_SEL) |
			p_fld(CLEAR_FLD, SHU1_WODT_TXUPD_SEL));
	} else if (p->frequency <= DDR3733_FREQ) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_SCINTV), 0xB,
			SHU_SCINTV_DQS2DQ_FILT_PITHRD);
		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
			p_fld(0x2, SHU1_WODT_TXUPD_W2R_SEL) |
			p_fld(CLEAR_FLD, SHU1_WODT_TXUPD_SEL));
	}

	prdcnt = (dram_mr.mr23_value[p->channel][RANK_0] / 4) + 3;

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DQSOSC_PRD),
		prdcnt, SHU1_DQSOSC_PRD_DQSOSC_PRDCNT);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_DQSOSCR), 0x40,
		SHU_DQSOSCR_DQSOSCRCNT);


	thrd_inc = dqsosc_thrd_inc[p->channel][0];
	thrd_dec = dqsosc_thrd_dec[p->channel][0];

	if (dqsosc_thrd_inc[p->channel][1] > dqsosc_thrd_inc[p->channel][0])
	{
		thrd_inc = dqsosc_thrd_inc[p->channel][1];
		thrd_dec = dqsosc_thrd_dec[p->channel][1];
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DQSOSC_PRD),
		thrd_inc, SHU1_DQSOSC_PRD_DQSOSCTHRD_INC);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DQSOSC_PRD),
		thrd_dec, SHU1_DQSOSC_PRD_DQSOSCTHRD_DEC);

	/*
	 * set interval to do MPC(start DQSOSC) command,
	 * and dramc send DQSOSC start to rank0/1/2 at the same time
	 * TX tracking period unit: 3.9us
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_DQSOSCR2),
		dqsoscencnt, SHU_DQSOSCR2_DQSOSCENCNT);

	return DRAM_OK;
}

void dramc_hwdqsosc_set_freq_ratio(DRAMC_CTX_T *p)
{
	show_err("[Error] Review dqsosc_ratio settings\n");
	while (1);

#if NON_EXIST_RG
	/* for SHUFFLE_1 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_RK2_DQSOSC),
		p_fld((unsigned short)
		(freq_tbl[1].frequency * 8 / freq_tbl[0].frequency),
		RK2_DQSOSC_FREQ_RATIO_TX_0) |
		p_fld((unsigned short)
		(freq_tbl[2].frequency * 8 / freq_tbl[0].frequency),
		RK2_DQSOSC_FREQ_RATIO_TX_1));
	/* for SHUFFLE_2 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_RK2_DQSOSC),
		p_fld((unsigned short)
		(freq_tbl[0].frequency * 8 / freq_tbl[1].frequency),
		RK2_DQSOSC_FREQ_RATIO_TX_3) |
		p_fld((unsigned short)
		(freq_tbl[2].frequency * 8 / freq_tbl[1].frequency),
		RK2_DQSOSC_FREQ_RATIO_TX_4));
	/* for SHUFFLE_3 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_RK2_DUMMY_RD_BK),
		p_fld((unsigned short)
		(freq_tbl[0].frequency * 8 / freq_tbl[2].frequency),
		RK2_DUMMY_RD_BK_FREQ_RATIO_TX_6) |
		p_fld((unsigned short)
		(freq_tbl[1].frequency * 8 / freq_tbl[2].frequency),
		RK2_DUMMY_RD_BK_FREQ_RATIO_TX_7));

	/* for SHUFFLE_4 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_PRE_TDQSCK1),
		 p_fld(SET_FLD, PRE_TDQSCK1_SHU_PRELOAD_TX_HW) |
		 p_fld(CLEAR_FLD, PRE_TDQSCK1_SHU_PRELOAD_TX_START) |
		 p_fld(CLEAR_FLD, PRE_TDQSCK1_SW_UP_TX_NOW_CASE));

	show_msg2((INFO, "TX_FREQ_RATIO_0=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_0)));
	show_msg2((INFO, "TX_FREQ_RATIO_1=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_1)));
	show_msg2((INFO, "TX_FREQ_RATIO_2=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_2)));
	show_msg2((INFO, "TX_FREQ_RATIO_3=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_3)));
	show_msg2((INFO, "TX_FREQ_RATIO_4=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_4)));
	show_msg2((INFO, "TX_FREQ_RATIO_5=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC), RK2_DQSOSC_FREQ_RATIO_TX_5)));
	show_msg2((INFO, "TX_FREQ_RATIO_6=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DUMMY_RD_BK),
		RK2_DUMMY_RD_BK_FREQ_RATIO_TX_6)));
	show_msg2((INFO, "TX_FREQ_RATIO_7=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DUMMY_RD_BK),
		RK2_DUMMY_RD_BK_FREQ_RATIO_TX_7)));
	show_msg2((INFO, "TX_FREQ_RATIO_8=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DUMMY_RD_BK),
		RK2_DUMMY_RD_BK_FREQ_RATIO_TX_8)));
	show_msg2((INFO, "TX_FREQ_RATIO_9=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_RK2_DQSOSC),
		RK2_DQSOSC_FREQ_RATIO_TX_0)));
	show_msg2((INFO, "TX_FREQ_RATIO_9=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_PRE_TDQSCK1),
		PRE_TDQSCK1_FREQ_RATIO_TX_9)));
	show_msg2((INFO, "TX_FREQ_RATIO_10=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_PRE_TDQSCK1),
		PRE_TDQSCK1_FREQ_RATIO_TX_10)));
	show_msg2((INFO, "TX_FREQ_RATIO_11=%d\n",
		io_32_read_fld_align(DRAMC_REG_ADDR
		(DRAMC_REG_PRE_TDQSCK1),
		PRE_TDQSCK1_FREQ_RATIO_TX_11)));
#endif
}


static void tx_tracking_mode_setting(DRAMC_CTX_T *p, unsigned char mode)
{
	/* enable DQSOSC HW mode */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR),
		SET_FLD, DQSOSCR_DQSOSCENDIS);
}

void dramc_hw_dqsosc(DRAMC_CTX_T *p)
{
	DRAM_RANK_T rank_bak = get_rank(p);
	DRAM_CHANNEL_T ch_bak = p->channel;

	dramc_hwdqsosc_set_freq_ratio(p);

#if NON_EXIST_RG
	/* DQSOSC MPC command violation */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION),
		CLEAR_FLD, MPC_OPTION_MPC_BLOCKALE_OPT);
#endif

	/* DQS2DQ UI/PI setting controlled by HW */
#if ENABLE_SW_TX_TRACKING
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), SET_FLD,
		MISC_CTRL1_R_DMARPIDQ_SW);
#else
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), CLEAR_FLD,
		MISC_CTRL1_R_DMARPIDQ_SW);
#endif
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), SET_FLD,
		DQSOSCR_ARUIDQ_SW);

	/*
	 * Set dqsosc oscillator run time by MRW
	 * write RK0 MR23
	 * Enable HW read MR18/MR19 for each rank
	 */
#if ENABLE_SW_TX_TRACKING
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), SET_FLD,
		DQSOSCR_DQSOSCRDIS);
#else
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), CLEAR_FLD,
		DQSOSCR_DQSOSCRDIS);
#endif

	set_rank(p, RANK_0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RK0_DQSOSC),
		SET_FLD, RK0_DQSOSC_DQSOSCR_RK0EN);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), SET_FLD,
			DQSOSCR_DQSOSC_CALEN);

#if ENABLE_SW_TX_TRACKING
	tx_tracking_mode_setting(p, SET_FLD);
#else
	tx_tracking_mode_setting(p, CLEAR_FLD);
#endif

	set_rank(p, rank_bak);
	set_phy_2_channel_mapping(p, ch_bak);
}

#if ENABLE_RX_TRACKING_LP4
void dramc_rx_input_delay_tracking_init_common(DRAMC_CTX_T *p)
{
	/* Porting from Azelea */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CG_CTRL1),
		0xFFFFFFFF, MISC_CG_CTRL1_R_DVS_DIV4_CG_CTRL);
}
#endif

void dramc_rx_input_delay_tracking_init_by_freq(DRAMC_CTX_T *p)
{
	unsigned char dvs_delay;
	/* Monitor window size setting */
	/* DDRPHY.SHU*_B*_DQ5.RG_RX_ARDQS0_DVS_DLY_B*
	 * ¡E4266@0.8v
	 * ¡E3733@0.8v
	 * ¡E3200@0.7v
	 * ¡E1600@0.6v
	 */
	if (p->freqGroup == DDR4266_FREQ)
		dvs_delay = 3;
	else if (p->freqGroup == DDR3733_FREQ)
		dvs_delay = 3;
	else if (p->freqGroup == DDR3200_FREQ)
		dvs_delay = 4;
	else
		dvs_delay = 5;

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5),
		dvs_delay, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5),
		dvs_delay, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7),
		p_fld(CLEAR_FLD,
		SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0));
#if NON_EXIST_RG
		p_fld(CLEAR_FLD,
		SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0));
#endif
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ7),
		p_fld(CLEAR_FLD,
		SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1));
#if NON_EXIST_RG
		p_fld(CLEAR_FLD,
		SHU1_B1_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B1));
#endif
}

#if ENABLE_RX_TRACKING_LP4
void dramc_rx_input_delay_tracking_hw(DRAMC_CTX_T *p)
{
	/* Porting from Azalea */

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_MISC_RXDVS2),
		p_fld(SET_FLD, MISC_RXDVS2_R_RXDVS_RDSEL_TOG_LAT) |
		p_fld(0x2, MISC_RXDVS2_R_RXDVS_RDSEL_BUS_LAT));
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1),
		CLEAR_FLD, MISC_CTRL1_R_DMDQMDBI);

	/* Enable A-PHY DVS LEAD/LAG */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), SET_FLD,
		B0_DQ5_RG_RX_ARDQS0_DVS_EN_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), SET_FLD,
		B1_DQ5_RG_RX_ARDQS0_DVS_EN_B1);
}
#endif

#if SIMULATION_LP4_ZQ
/*
 * dramc_zq_calibration
 *  start Dram ZQ calibration.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
DRAM_STATUS_T dramc_zq_calibration(DRAMC_CTX_T *p)
{
	unsigned int response;
	unsigned int time_cnt = TIME_OUT_CNT;
	unsigned int reg_backup_address[] = { DRAMC_REG_ADDR(DRAMC_REG_MRS),
		DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		DRAMC_REG_ADDR(DRAMC_REG_CKECTRL) };

	/* Backup rank, CKE fix on/off, HW MIOCK control settings */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	show_msg3((INFO, "[ZQCalibration]\n"));

	/* Disable HW MIOCK control to make CLK always on */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		SET_FLD,	DRAMC_PD_CTRL_MIOCKCTRLOFF);
	delay_us(1);

	/* it will apply to both rank. */
	cke_fix_on_off(p, CKE_FIXON, CKE_WRITE_TO_ONE_CHANNEL);

	/* !!R_DMMRSRK(R_DMMPCRKEN=1) specify rank0 or rank1 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), get_rank(p),
		MRS_MRSRK);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION), SET_FLD,
		MPC_OPTION_MPCRKEN);

	/* ZQCAL Start
	 * R_DMZQCEN, 0x1E4[4]=1 for ZQCal Start
	 * Wait zqc_response=1 (dramc_conf_nao, 0x3b8[4])
	 * R_DMZQCEN, 0x1E4[4]=0
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		SET_FLD, SPCMD_ZQCEN);
	do {
		response =
			io_32_read_fld_align(
			DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP),
			SPCMDRESP_ZQC_RESPONSE);
		time_cnt--;
		delay_us(1);	/* Wait tZQCAL(min) 1us or wait next polling */

		show_msg2((INFO, "%d- ", time_cnt));
	} while ((response == 0) && (time_cnt > 0));

	if (time_cnt == 0) {	/* time out */
		set_calibration_result(p, DRAM_CALIBRATION_ZQ, DRAM_FAIL);
		show_err("ZQCAL Start fail (time out)\n");
		return DRAM_FAIL;
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		CLEAR_FLD, SPCMD_ZQCEN);

	/* delay tZQCAL */
	delay_us(1);
	time_cnt = TIME_OUT_CNT;

	/*
	 * ZQCAL Latch
	 * R_DMZQLATEN, 0x1E4[6]=1 for ZQCal latch
	 * Wait zqlat_response=1 (dramc_conf_nao, 0x3b8[28])
	 * R_DMZQLATEN, 0x1E4[6]=0
	 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		SET_FLD, SPCMD_ZQLATEN);
	do {
		response =
			io_32_read_fld_align(DRAMC_REG_ADDR
				(DRAMC_REG_SPCMDRESP),
				SPCMDRESP_ZQLAT_RESPONSE);
		time_cnt--;
		delay_us(1);	/*  Wait tZQLAT 30ns or wait next polling */

		show_msg2((INFO, "%d=", time_cnt));
	} while ((response == 0) && (time_cnt > 0));

	if (time_cnt == 0) {	/* time out */
		set_calibration_result(p, DRAM_CALIBRATION_ZQ, DRAM_FAIL);
		show_err("ZQCAL Latch fail (time out)\n");
		return DRAM_FAIL;
	}
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		CLEAR_FLD, SPCMD_ZQLATEN);

	/* delay tZQLAT */
	delay_us(1);

	/* Restore rank, CKE fix on, HW MIOCK control settings */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	set_calibration_result(p, DRAM_CALIBRATION_ZQ, DRAM_OK);
	show_log("[DramcZQCalibration] Done\n");

	return DRAM_OK;
}
#endif

/*
 *	dramc_sw_impedance_cal
 *	start TX OCD impedance calibration.
 *	@param p	Pointer of context created by DramcCtxCreate.
 *	@param  apply	(unsigned char): 0 don't apply the register
 *	we set  1 apply the register we set ,default don't apply.
 *	@retval status	(DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */

/* Impedance have a total of 19 steps,
* but the HW value mapping to hardware is 0~15, 29~31
* This function adjusts passed value u1ImpVal
* by adjust step count "u1AdjStepCnt"
* After adjustment, if value is
*	1. Too large (val > 31) -> set to max 31
*	2. Too small (val < 0) -> set to min 0
*	3. Value is between 15 & 29, adjust accordingly ( 15 < value < 29 )
* returns: Impedance value after adjustment
*/
#if SIMULATION_SW_IMPED
void dramc_sw_impedance_save_register(DRAMC_CTX_T *p,
	unsigned char ca_term_option, unsigned char dq_term_option,
	unsigned char save_to_where)
{
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING2),
		(dramc_imp_result[dq_term_option][TERM_TYPE_DRVP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_DRVN],
		SHU1_DRVING2_DQDRV1);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING1),
		(dramc_imp_result[dq_term_option][TERM_TYPE_DRVP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_DRVN],
		SHU1_DRVING1_DQSDRV1);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING2),
		(dramc_imp_result[dq_term_option][TERM_TYPE_DRVP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_DRVN],
		SHU1_DRVING2_CMDDRV1);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING2),
		(dramc_imp_result[dq_term_option][TERM_TYPE_DRVP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_DRVN],
		SHU1_DRVING2_CMDDRV2);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING4),
		(dramc_imp_result[dq_term_option][TERM_TYPE_ODTP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_ODTN],
		SHU1_DRVING4_DQODT1);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU1_DRVING3),
		(dramc_imp_result[dq_term_option][TERM_TYPE_ODTP] << 5) |
		dramc_imp_result[dq_term_option][TERM_TYPE_ODTP],
		SHU1_DRVING3_DQSODT1);
}

/*
 * imp_cal_vref_sel
 *  Set IMP_VREF_SEL for DRVP, DRVN, Run-time/Tracking
 *  (Refer to "IMPCAL Settings" document register "RG_RIMP_VREF_SEL" settings)
 *  @param p	Pointer of context created by DramcCtxCreate.
 *  @param  term_option	(unsigned char): pass term_option (odt_on/off) for LP4X
 *  @param  imp_cal_stage	(unsigned char): During DRVP, DRVN,
 *	un-time/tracking stages
 *	some vref_sel values are different
 */
/* Refer to "IMPCAL Settings" document register "RG_RIMP_VREF_SEL" settings */
static void dramc_sw_impedance_cal_init(DRAMC_CTX_T *p)
{
    unsigned int rimp_vref = 0;

	if (p->dram_type == TYPE_PCDDR3 || p->dram_type == TYPE_LPDDR3)
		rimp_vref = 0x30;
	else if (p->dram_type == TYPE_PCDDR4)
		rimp_vref = 0x37;
	else if (p->dram_type == TYPE_LPDDR4)
		rimp_vref = 0x2a;

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_SPM_CTRL1),
		CLEAR_FLD, MISC_SPM_CTRL1_PHY_SPM_CTL1);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_SPM_CTRL2),
		CLEAR_FLD, MISC_SPM_CTRL2_PHY_SPM_CTL2);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_SPM_CTRL0),
		CLEAR_FLD, MISC_SPM_CTRL0_PHY_SPM_CTL0);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL),
		0, IMPCAL_IMPCAL_HW);

	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1),
		p_fld(0, MISC_IMP_CTRL1_RG_RIMP_PRE_EN));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL),
		p_fld(0, IMPCAL_IMPCAL_CALI_ENN) |
		p_fld(1, IMPCAL_IMPCAL_IMPPDP) |
		p_fld(1, IMPCAL_IMPCAL_IMPPDN));

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0),
		rimp_vref, MISC_IMP_CTRL0_RG_RIMP_VREF_SEL);
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_PLL3),
		p_fld(0, PLL3_RG_RPHYPLL_TSTOD_EN) |
		p_fld(0, PLL3_RG_RPHYPLL_TSTCK_EN));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0),
		p_fld(0, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL) |
		p_fld(0, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));

	show_msg2((INFO, "MISC_IMP_CTRL1: 0x%X = 0x%X\n",
		DDRPHY_MISC_IMP_CTRL1,
		io32_read_4b(DDRPHY_MISC_IMP_CTRL1)));
	show_msg2((INFO, "MISC_IMP_CTR0: 0x%X = 0x%X\n",
		DDRPHY_MISC_IMP_CTRL0,
		io32_read_4b(DDRPHY_MISC_IMP_CTRL0)));

	delay_us(1);
}

static unsigned int dramc_sw_impedance_drvp(DRAMC_CTX_T *p)
{
	unsigned int impx_drv, imp_cal_result;
	unsigned int drvp_result = BYTE_MAX;

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL),
		p_fld(1, IMPCAL_IMPCAL_CALI_EN) |
		p_fld(1, IMPCAL_IMPCAL_CALI_ENP) |
		p_fld(0, IMPCAL_IMPCAL_CALI_ENN));

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1),
		p_fld(0, SHU_IMPCAL1_IMPDRVN) |
		p_fld(0, SHU_IMPCAL1_IMPDRVP));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1),
		p_fld(0, MISC_IMP_CTRL1_RG_RIMP_REV));

	for (impx_drv = 0; impx_drv < 32; impx_drv++) {
		if (impx_drv == 16)
			impx_drv = 27;

		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1),
			impx_drv, SHU_IMPCAL1_IMPDRVP);
		delay_us(1);
		imp_cal_result = io_32_read_fld_align(
			DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS1),
			MISC_PHY_RGS1_RGS_RIMPCALOUT);
		show_msg3((INFO, "1. OCD DRVP = %d CALOUT = %d\n",
			impx_drv, imp_cal_result));

		if ((imp_cal_result == 1) && (drvp_result == 0xff)) {
			drvp_result = impx_drv;
			show_msg2((INFO, "1. OCD DRVP calibration OK! DRVP = %d\n\n",
				drvp_result));
			break;
		}
	}

	return drvp_result;
}

static unsigned int dramc_sw_impedance_drvn(DRAMC_CTX_T *p)
{
	unsigned int impx_drv, imp_cal_result;
	unsigned int drvn_result = BYTE_MAX;


	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL),
		0, IMPCAL_IMPCAL_CALI_ENP);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1),
		0, SHU_IMPCAL1_IMPDRVN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1),
		0, MISC_IMP_CTRL1_RG_RIMP_REV);

	for (impx_drv = 0; impx_drv < 32; impx_drv++) {
		if (impx_drv == 16)
			impx_drv = 27;

		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1),
			impx_drv, SHU_IMPCAL1_IMPDRVN);
		delay_us(1);
		imp_cal_result = io_32_read_fld_align(
			DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS1),
			MISC_PHY_RGS1_RGS_RIMPCALOUT);
		show_msg3((INFO, "3. OCD DRVN = %d, CALOUT = %d\n",
			impx_drv, imp_cal_result));

		if ((imp_cal_result == 0) && (drvn_result == 0xff)) {
			drvn_result = impx_drv;
			show_msg2((INFO, "3. OCD DRVN calibration OK! DRVN = %d\n\n",
				drvn_result));
			break;
		}
	}

	return drvn_result;
}

static unsigned short round_operation(unsigned short A,
	unsigned short B)
{
	unsigned short temp;

	if (B == 0) {
		return 0xffff;
	}

	temp = A / B;

	if ((A - temp*B) >= ((temp + 1)*B - A)) {
		return (temp + 1);
	} else {
		return temp;
	}
}

static void dramc_sw_impedance_calc(DRAMC_CTX_T *p,
	unsigned char ocdp, unsigned char ocdn, unsigned char term_option)
{
	unsigned char term_type;

	if (ocdp == BYTE_MAX || ocdn == BYTE_MAX) {
		dramc_imp_result[term_option][TERM_TYPE_DRVP] = OCDP_DEFAULT;
		dramc_imp_result[term_option][TERM_TYPE_DRVN] = OCDN_DEFAULT;
		dramc_imp_result[term_option][TERM_TYPE_ODTP] = ODTP_DEFAULT;
		dramc_imp_result[term_option][TERM_TYPE_ODTN] = ODTN_DEFAULT;
	} else {
		/* Covert to slice number */
		ocdp = ((ocdp >> 4) & 0x1) * 5 + (ocdp & 0xf);
		ocdn = ((ocdn >> 4) & 0x1) * 5 + (ocdn & 0xf);

		/* OCDP_final */
		dramc_imp_result[term_option][TERM_TYPE_DRVP] =
			round_operation(fcR_PU * ((unsigned short)ocdp),
			fcR_OCD);

		/* OCDN_final */
		dramc_imp_result[term_option][TERM_TYPE_DRVN] =
			round_operation(fcR_PD * ((unsigned short)ocdn),
			fcR_OCD);

		/* ODTP_final */
		dramc_imp_result[term_option][TERM_TYPE_ODTP] =
			round_operation(fcR_EXT * ((unsigned short)ocdp),
			fcR_ODT);

		/* ODTN_final */
		dramc_imp_result[term_option][TERM_TYPE_ODTN] =
			round_operation(fcR_EXT * ((unsigned short)ocdn),
			fcR_ODT);

		if (p->dram_type == TYPE_LPDDR4) {
			dramc_imp_result[term_option][TERM_TYPE_ODTP] = 0;
		} else if (p->dram_type == TYPE_PCDDR4) {
			dramc_imp_result[term_option][TERM_TYPE_ODTN] = 0;
		} else if (p->dram_type == TYPE_LPDDR3) {
			dramc_imp_result[term_option][TERM_TYPE_ODTP] = 0;
			dramc_imp_result[term_option][TERM_TYPE_ODTN] = 0;
		}

		/* Covert back to Register format */
		for (term_type = TERM_TYPE_DRVP; term_type < TERM_TYPE_NUM;
			term_type++) {
			unsigned char term_result;

			term_result = dramc_imp_result[term_option][term_type];

			/* Maximum slice number is 20 */
			if (term_result > 20) {
				term_result = 20;
				show_msg((INFO, "Warning: Calculated result for"
					"Impedance type %d exceed limit. Use"
					"Upper limit %d instead\n",
					term_type, term_result));
			}

			if (term_result > 0xf) {
				/* Bit[4] has a weight of 5 */
				term_result -= 5;
				term_result += (1 << 4);

			}

			dramc_imp_result[term_option][term_type] = term_result;
		}
	}
}

DRAM_STATUS_T dramc_sw_impedance_cal(DRAMC_CTX_T *p,
	unsigned char term_option)
{
	unsigned int drvp_result, drvn_result;

	unsigned int reg_backup_address[] = {
		((DDRPHY_MISC_IMP_CTRL0)), /* Only in B01 */
		((DDRPHY_MISC_IMP_CTRL1)),
		(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL)),
	};

	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	dramc_sw_impedance_cal_init(p);

	show_msg2((INFO, "======= K DRVP=====================\n"));
	drvp_result = dramc_sw_impedance_drvp(p);

	if (drvp_result == BYTE_MAX)
		show_msg2((INFO, "OCD DRVP calibration FAIL\n"));

	show_msg2((INFO, "======= K DRVN=====================\n"));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1),
		p_fld(drvp_result, SHU_IMPCAL1_IMPDRVP));
	drvn_result = dramc_sw_impedance_drvn(p);

	if (drvn_result == BYTE_MAX)
		show_msg2((INFO, "OCD DRVN calibration FAIL\n"));

	dramc_sw_impedance_calc(p, drvp_result, drvn_result,
		term_option);

	show_msg2((INFO, "%s: OCDP %x, OCDN %x, ODTP %x, ODTN %x\n",
		"Final Impdance Cal Result: ",
		dramc_imp_result[term_option][TERM_TYPE_DRVP],
		dramc_imp_result[term_option][TERM_TYPE_DRVN],
		dramc_imp_result[term_option][TERM_TYPE_ODTP],
		dramc_imp_result[term_option][TERM_TYPE_ODTN]));
#ifdef ETT
	ett_drv[0] = dramc_imp_result[term_option][TERM_TYPE_DRVP];
	ett_drv[1] = dramc_imp_result[term_option][TERM_TYPE_DRVN];
	ett_drv[2] = dramc_imp_result[term_option][TERM_TYPE_ODTP];
	ett_drv[3] = dramc_imp_result[term_option][TERM_TYPE_ODTN];
#endif


	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	return DRAM_OK;
}

#endif /* SIMULATION_SW_IMPED */

void dramc_update_impedance_term_2un_term(DRAMC_CTX_T *p)
{
	dramc_imp_result[ODT_OFF][TERM_TYPE_ODTP] =
		dramc_imp_result[ODT_ON][TERM_TYPE_ODTP];
	dramc_imp_result[ODT_OFF][TERM_TYPE_ODTN] =
		dramc_imp_result[ODT_ON][TERM_TYPE_ODTN];
}

void o1_path_on_off(DRAMC_CTX_T *p, unsigned char on_off)
{
	unsigned char fix_dqien = 0;

	fix_dqien = (on_off == ENABLE) ? 3 : 0;
	io_32_write_fld_align_all(DRAMC_REG_PADCTRL, fix_dqien,
		PADCTRL_FIXDQIEN);

	io_32_write_fld_align_all(DDRPHY_B0_DQ5, on_off,
		B0_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ5, on_off,
		B1_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B1);
	io_32_write_fld_align_all(DDRPHY_B0_DQ3, on_off,
		B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
	io_32_write_fld_align_all(DDRPHY_B1_DQ3, on_off,
		B1_DQ3_RG_RX_ARDQ_SMT_EN_B1);
	delay_us(1);
}

#if SUPPORT_TYPE_LPDDR4
/*  LPDDR DQ -> PHY DQ mapping
 *  Use LPDDR DQ bit as index to find PHY DQ bit position
 */
unsigned char lpddr4_phy_mapping_pop[CHANNEL_NUM][16] = {
#if 0 /* PINMUX not applied */
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15
	},
#else /* PINUMX applied */
	{
		0, 2, 7, 4, 6, 5, 3, 1,
		10, 11, 9, 8, 12, 14, 13, 15,
	},
	{
		0, 2, 7, 4, 6, 5, 3, 1,
		10, 11, 9, 8, 12, 14, 13, 15,
	},
#endif
};

/* LP4 DQ mux if 4BIT mux enabled. DRAMC DQ (as index) -> PHY DQ */
/*
unsigned char lpddr4_4bitmux_byte_mapping[DATA_WIDTH_16BIT] = {
	0, 1, 2, 3, 8, 9, 10, 11,
	4, 5, 6, 7, 12, 13, 14, 15,
};
*/
unsigned char lpddr4_4bitmux_byte_mapping[DATA_WIDTH_16BIT] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
};

#endif

#if SUPPORT_TYPE_LPDDR3
/* LPDDR3 DQ (as index) -> PHY pad */
unsigned char lpddr3_phy_mapping_pop[CHANNEL_NUM][32] = {
#if 0 /* PINMUX not applied */
	/* CH-A */
	{
		0, 1, 2, 3, 4, 5, 6, 7,
		8, 9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23,
		24, 25, 26, 27, 28, 29, 30, 31,
	},
#else /* PINMUX applied */
	/* CH-A */
	{
		25, 24, 21, 23, 27, 26, 20, 22,
		8, 10, 4, 6, 11, 9, 7, 5,
		17, 16, 18, 31, 19, 29, 30, 28,
		1, 0, 13, 14, 3, 2, 15, 12,
	},
#endif
};

unsigned char lpddr3_4bitmux_byte_mapping[DATA_WIDTH_32BIT] = {
	8, 9, 10, 11, 4, 5, 6, 7,
	0, 1, 2, 3, 12, 13, 14, 15,
	24, 25, 26, 27, 20, 21, 22, 23,
	16, 17, 18, 19, 28, 29, 30, 31,
};
#endif

unsigned char *dramc_get_4bitmux_byte_mapping(DRAMC_CTX_T *p)
{
	unsigned char *ret;

	ret = NULL;

#if (SUPPORT_TYPE_LPDDR4 == 1)
	ret = (p->dram_type == TYPE_LPDDR4) ?
		lpddr4_4bitmux_byte_mapping : NULL;
#elif (SUPPORT_TYPE_LPDDR3== 1)
	ret = (p->dram_type == TYPE_LPDDR3) ?
		lpddr3_4bitmux_byte_mapping : NULL;
#endif

	return ret;
}

/* dramc_miock_jmeter
 *  start MIOCK jitter meter.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @param block_no         (unsigned char): block 0 or 1.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */

#ifdef ENABLE_MIOCK_JMETER
static void set_enable_miock_jmeter_rg(DRAMC_CTX_T *p)
{
	/* MCK4X CG */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), CLEAR_FLD,
		MISC_CTRL1_R_DMDQSIENCG_EN);

	/*  Bypass DQS glitch-free mode */

	/*  RG_RX_*RDQ_EYE_DLY_DQS_BYPASS_B** */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), SET_FLD,
		B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), SET_FLD,
		B1_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B1);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		CLEAR_FLD, DRAMC_PD_CTRL_DCMEN);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ7),
		0x3, B0_DQ7_RG_RX_ARDQS0_BURST_EN_B01);
	/*
	* Enable DQ eye scan
	* RG_??_RX_EYE_SCAN_EN
	* RG_??_RX_VREF_EN
	* RG_??_RX_SMT_EN
	*/
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), SET_FLD,
		PLL1_RG_RX_EYE_SCAN_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL2), SET_FLD,
		PLL2_R_DMEYESCAN_DQS_SYNC_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), SET_FLD,
		B0_DQ5_RG_RX_ARDQ_EYE_EN_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), SET_FLD,
		B1_DQ5_RG_RX_ARDQ_EYE_EN_B1);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), SET_FLD,
		B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), SET_FLD,
		B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ3), CLEAR_FLD,
		B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ3), CLEAR_FLD,
		B1_DQ3_RG_RX_ARDQ_SMT_EN_B1);

	/* JM_SEL */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), SET_FLD,
		B0_DQ6_RG_RX_ARDQ_JM_SEL_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), SET_FLD,
		B1_DQ6_RG_RX_ARDQ_JM_SEL_B1);
	/* Enable MIOCK jitter meter mode ( RG_RX_MIOCK_JIT_EN=1) */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), SET_FLD,
		PLL1_RG_RX_MIOCK_JIT_EN);

	/* Disable DQ eye scan (b'1), for counter clear */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), CLEAR_FLD,
		PLL1_RG_RX_EYE_SCAN_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), CLEAR_FLD,
		PLL1_R_DMDQSERRCNT_DIS);
}

static unsigned char check_counter_jmeter(DRAMC_CTX_T *p,
	unsigned char delay, unsigned char sel_clk)
{
	unsigned char fgcurrent_value;
	unsigned int sample_cnt, ones_cnt[DQS_NUMBER];

	/* Set DQS delay (RG_??_RX_DQS_EYE_DLY) */
	if (sel_clk) {
		io_32_write_fld_align_all(DDRPHY_SHU1_R0_CA_CMD10,
			delay, SHU1_R0_CA_CMD10_RG_RK0_RX_ARCLK_DLY);
	} else {
		io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B0_DQ6,
			p_fld(delay, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
			p_fld(delay, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0));
		io_32_write_fld_multi_all(DDRPHY_SHU1_R0_B1_DQ6,
			p_fld(delay, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1) |
			p_fld(delay, SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1));
	}

	dram_phy_reset(p);

	/* Reset eye scan counters (reg_sw_rst): 1 to 0 */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL4),
		SET_FLD, PLL4_RG_EYESCAN_SW_RST);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL4),
		CLEAR_FLD, PLL4_RG_EYESCAN_SW_RST);

	/* Enable DQ eye scan (b'1) */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1),
		SET_FLD, PLL1_RG_RX_EYE_SCAN_EN);

	/*  2ns/sample, here we delay 1ms about 500 samples */
	delay_us(1000);

	/* Disable DQ eye scan (b'1), for counter latch */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1),
		CLEAR_FLD, PLL1_RG_RX_EYE_SCAN_EN);

	/* Read the counter values from registers
	 * Note that these registers are located in CMD
	 * region, that is B01 base shall be selected.
	 */
	sample_cnt = io_32_read_fld_align(DDRPHY_TOGGLE_CNT,
		TOGGLE_CNT_TOGGLE_CNT);
	ones_cnt[0] = io_32_read_fld_align(DDRPHY_DQS0_ERR_CNT,
		DQS0_ERR_CNT_DQS0_ERR_CNT);
	ones_cnt[1] = io_32_read_fld_align(DDRPHY_DQS1_ERR_CNT,
		DQS1_ERR_CNT_DQS1_ERR_CNT);
	ones_cnt[2] = io_32_read_fld_align(DDRPHY_DQS2_ERR_CNT,
		DQS2_ERR_CNT_DQS2_ERR_CNT);
	ones_cnt[3] = io_32_read_fld_align(DDRPHY_DQS3_ERR_CNT,
		DQS3_ERR_CNT_DQS3_ERR_CNT);
	show_msg3((INFO, "%d : %d, %d, %d, %d, %d\n", delay,
		sample_cnt, ones_cnt[0], ones_cnt[1],
		ones_cnt[2], ones_cnt[3]));

	/* change to boolean value */
	if (ones_cnt[0] < (sample_cnt / 2))
		fgcurrent_value = 0;
	else
		fgcurrent_value = 1;

	return fgcurrent_value;
}

unsigned short ca_delay_cell_time_lp4(DRAMC_CTX_T *p)
{
	unsigned short real_freq;
	unsigned int sdm_pcw;
	unsigned int prediv;
	unsigned int posdiv;
	unsigned int vco_freq;
	unsigned int data_rate;

	sdm_pcw = io_32_read_fld_align(DDRPHY_SHU1_PLL7,
		SHU1_PLL7_RG_RCLRPLL_SDM_PCW);
	prediv = io_32_read_fld_align(DDRPHY_SHU1_PLL10,
		SHU1_PLL10_RG_RCLRPLL_PREDIV);
	posdiv = io_32_read_fld_align(DDRPHY_SHU1_PLL10,
		SHU1_PLL10_RG_RCLRPLL_POSDIV);

	vco_freq = ((26 >> prediv) * (sdm_pcw >> 24)) >> posdiv;

	if (p->frequency >= DDR1600_FREQ)
		data_rate = vco_freq;
	else
		data_rate = vco_freq >> 1;

	real_freq = data_rate >> 1;

	show_msg2((INFO, "prediv %d, posdiv %d, vco_freq %d, "
		"data_rate %d, real_freq %d\n",
		prediv, posdiv, vco_freq, data_rate, real_freq));

	return real_freq;
}

DRAM_STATUS_T dramc_miock_jmeter(DRAMC_CTX_T *p)
{
	unsigned char search_state, dqs_dly, fgcurrent_value, fginitial_value=0,
		start_period = 0, middle_period = 0, end_period = 0;
	unsigned short real_freq, real_period;
	unsigned char step;
	unsigned int reg_backup_address[] = {
		((DDRPHY_PLL1)),
		((DDRPHY_PLL2)),
		((DDRPHY_PLL4)),
		((DDRPHY_B0_DQ7)),
		((DDRPHY_B1_DQ7)),
		((DDRPHY_B0_DQ6)),
		((DDRPHY_B1_DQ6)),
		((DDRPHY_B0_DQ5)),
		((DDRPHY_B1_DQ5)),
		((DDRPHY_B0_DQ3)),
		((DDRPHY_B1_DQ3)),
		((DDRPHY_SHU1_B0_DQ7)),
		((DDRPHY_SHU1_B1_DQ7)),
		((DDRPHY_B0_DQ4)),
		((DDRPHY_B1_DQ4)),

		((DDRPHY_B0_DQ7) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ7) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B0_DQ6) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ6) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B0_DQ5) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ5) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B0_DQ3) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ3) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_SHU1_B0_DQ7) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_SHU1_B1_DQ7) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B0_DQ4) + (CHANNEL_B << POS_BANK_NUM)),
		((DDRPHY_B1_DQ4) + (CHANNEL_B << POS_BANK_NUM)),

		((DDRPHY_MISC_CTRL1)),
		((DDRPHY_CA_DLL_ARPI2)), /* Only in B01 */
		((DDRPHY_B0_DLL_ARPI2)),
		((DDRPHY_B1_DLL_ARPI2)),
	};

	delay_cell_ps[p->channel] = 0;

#if (FOR_DV_SIMULATION_USED == 0)
	step = 1;
#else
	step = 4;
#endif

	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/*
	 * DLL off to fix middle transion from high to low or low to high
	 * at high vcore
	 */
	/*
	 * io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI2),CLEAR_FLD,
	 * CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
	 * io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2),CLEAR_FLD,
	 * B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);
	 * io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DLL_ARPI2),CLEAR_FLD,
	 * B1_DLL_ARPI2_RG_ARDLL_PHDET_EN_B1);
	 */
	set_enable_miock_jmeter_rg(p);
	search_state = 0;

	/* to see 1T(H,L) or 1T(L,H) from delaycell=0 to 127 */
	for (dqs_dly = 0; dqs_dly < 128; dqs_dly += step) {

		fgcurrent_value = check_counter_jmeter(p, dqs_dly, FALSE);

		/* more than 1T data */
		if (search_state == 0) {
			/* record initial value at the beginning */
			fginitial_value = fgcurrent_value;
			search_state = 1;
		} else if (search_state == 1) {
			/*  check if change value */
			if (fgcurrent_value != fginitial_value) {
				/* start of the period */
				fginitial_value = fgcurrent_value;
				start_period = dqs_dly;
				search_state = 2;
			}
		} else if (search_state == 2) {
			/*  check if change value */
			if (fgcurrent_value != fginitial_value) {
				fginitial_value = fgcurrent_value;
				middle_period = dqs_dly;
				search_state = 3;
			}
		} else if (search_state == 3) {
			/*  check if change value */
			if (fgcurrent_value != fginitial_value) {
				/* end of the period, break the loop */
				end_period = dqs_dly;
				search_state = 4;
				break;
			}
		} else { /* nothing */
		}
	}

	/* restore to orignal value */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	if (search_state != 4) {
		if (search_state != 3) {
			show_msg((INFO, "\n\tMIOCK jitter meter - ch=%d\n",
				p->channel));
			show_msg((INFO,
				"\tLess than 0.5T data."));
			show_msg((INFO,
				"Cannot calculate delay cell time\n\n"));
			return DRAM_FAIL;
		}

		/* Calculate 1 delay cell = ? ps */
		num_dlycell_per_t[p->channel] =
			(middle_period - start_period) * 2;
	} else {
		/* Calculate 1 delay cell = ? ps */
		num_dlycell_per_t[p->channel] = (end_period - start_period);
	}

	show_msg((INFO, "\n\tMIOCK jitter meter\tch=%d\n\n ",
		p->channel));
	if (search_state == 4) {
		show_msg((INFO, " 1T = (%d-%d) = %d dly cells\n",
			end_period, start_period,
			num_dlycell_per_t[p->channel]));
	} else {
		show_msg((INFO, " 1T = (%d-%d)*2 = %d dly cells\n",
			middle_period, start_period,
			num_dlycell_per_t[p->channel]));
	}

	real_freq = ca_delay_cell_time_lp4(p);
	real_period = (unsigned short) (1000000 / real_freq);
	/* calculate delay cell time */

	if (num_dlycell_per_t[p->channel] == 0) {
		delay_cell_ps[p->channel] = 0;
	} else {
		delay_cell_ps[p->channel] =
			real_period * 100 / num_dlycell_per_t[p->channel];
	}

	show_msg((INFO, "Clock freq = %d MHz, ", real_freq));
	show_msg((INFO, "period = %d ps, 1 dly cell = %d/100 ps\n",
		real_period, delay_cell_ps[p->channel]));

	return DRAM_OK;
}
#endif

#if ENABLE_DUTY_SCAN_V2

#define DUTY_OFFSET_START -8
#define DUTY_OFFSET_END 8

#define CLOCK_PI_START 0
#define CLOCK_PI_END 63
#define CLOCK_PI_STEP 2

#define ClockDutyFailLowerBound 4500	/*  45% */
#define ClockDutyFailUpperBound 5500	/*  55% */
#define ClockDutyMiddleBound    5000	/*  50% */

void dqs_duty_scan_set_dqs_delay_cell(DRAMC_CTX_T *p,
	unsigned char channel_idx, signed char *duty_delay)
{
	unsigned char shuffle_idx = 0, dqsIdx, rank_idx = 0;
	unsigned int save_offset;
	unsigned char delay[2], delay_b[2];
	unsigned char rev_bit0[2], rev_bit1[2];

	for (dqsIdx = 0; dqsIdx < 2; dqsIdx++) {
		if (duty_delay[dqsIdx] < 0) {
			delay[dqsIdx]  = -(duty_delay[dqsIdx]);
			delay_b[dqsIdx]  = 0;

			rev_bit0[dqsIdx] = 1;
			rev_bit1[dqsIdx] = 0;
		} else if (duty_delay[dqsIdx] > 0) {
			delay[dqsIdx]  = 0;
			delay_b[dqsIdx]  = duty_delay[dqsIdx];

			rev_bit0[dqsIdx] = 0;
			rev_bit1[dqsIdx] = 1;
		} else {
			delay[dqsIdx]  = 0;
			delay_b[dqsIdx]  = 0;

			rev_bit0[dqsIdx] = 0;
			rev_bit1[dqsIdx] = 0;
		}
	}

	for (shuffle_idx = 0; shuffle_idx < DRAM_DFS_SHUFFLE_MAX;
		shuffle_idx++) {
		for (rank_idx = 0; rank_idx < p->support_rank_num;
			rank_idx++) {
			for (dqsIdx = 0; dqsIdx < 2; dqsIdx++) {
				save_offset = shuffle_idx *
					SHU_GRP_DDRPHY_OFFSET +
					rank_idx*0x100 + dqsIdx*0x50;
				io_32_write_fld_multi(DRAMC_REG_ADDR
					(DDRPHY_SHU1_R0_B0_DQ1) +
					save_offset, p_fld(delay[dqsIdx],
					SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLY_B0) |
					p_fld(delay[dqsIdx],
					SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLY_B0));
				io_32_write_fld_multi(DRAMC_REG_ADDR
					(DDRPHY_SHU1_R0_B0_DQ1) +
					save_offset, p_fld(delay_b[dqsIdx],
					SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLYB_B0) |
					p_fld(delay_b[dqsIdx],
					SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLYB_B0));

				save_offset = shuffle_idx *
					SHU_GRP_DDRPHY_OFFSET + dqsIdx*0x80;
				io_32_write_fld_multi(DRAMC_REG_ADDR
					(DDRPHY_SHU1_B0_DQ3) + save_offset,
					p_fld(rev_bit0[dqsIdx],
					RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN) |
					p_fld(rev_bit1[dqsIdx],
					RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN));
			}
		}
	}
}

/*
 *  offset is not related to DQ/DQM/DQS
 *  we have a circuit to measure duty, But this circuit is not very accurate
 *  so we need to K offset of this circuit first
 *  After we got this offset, then we can use it to measure duty
 *  this offset can measure DQ/DQS/DQM, and every byte has this circuit, too.
 *  B0/B1/CA all have one circuit.
 *  CA's circuit can measure CLK duty
 *  B0/B1's can measure DQ/DQM/DQS duty
 */
signed char duty_scan_offset_convert(unsigned char val)
{
	unsigned char cal_seq[NIBBLE_MAX] = {
		0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x0, 0x1, 0x2, 0x3, 0x4,
		0x5, 0x6, 0x7
	};

	return ((signed char)(cal_seq[val] > 8 ? 0-(cal_seq[val] & 0x7) :
		cal_seq[val]));
}

static void duty_scan_offset_cal_init(DRAMC_CTX_T *p)
{
	/* B0 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B0_DQ6),
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_LPBK_EN_B0) |
		p_fld(CLEAR_FLD,	B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ3),
		p_fld(SET_FLD, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN) |
		p_fld(CLEAR_FLD, RG_ARDQ_REV_BIT_21_DATA_SWAP) |
		p_fld(SET_FLD, RG_ARDQ_REV_BIT_22_DATA_SWAP));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5),
		p_fld(CLEAR_FLD,	SHU1_B0_DQ5_RG_RX_ARDQ_VREF_BYPASS_B0) |
		p_fld(0xb, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0));
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), SET_FLD,
		SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DQ0), CLEAR_FLD,
		B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B0_DQ5),
		p_fld(SET_FLD, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0) |
		p_fld(SET_FLD, B0_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B0));
	delay_us(1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), SET_FLD,
		SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT2);

	/* B1 */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B1_DQ6),
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_LPBK_EN_B1) |
		p_fld(CLEAR_FLD,	B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ3),
		p_fld(SET_FLD, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN) |
		p_fld(CLEAR_FLD, RG_ARDQ_REV_BIT_21_DATA_SWAP) |
		p_fld(SET_FLD, RG_ARDQ_REV_BIT_22_DATA_SWAP));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5),
		p_fld(CLEAR_FLD, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_BYPASS_B1) |
		p_fld(0xb, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1));
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), SET_FLD,
		SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ0), CLEAR_FLD,
		B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B1_DQ5),
		p_fld(SET_FLD, B1_DQ5_RG_RX_ARDQ_VREF_EN_B1) |
		p_fld(SET_FLD, B1_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B1));
	delay_us(1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), SET_FLD,
		SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT2);

	/* CA */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_CA_CMD6),
		p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_LPBK_EN) |
		p_fld(CLEAR_FLD,	CA_CMD6_RG_RX_ARCMD_DDR4_SEL) |
		p_fld(SET_FLD, CA_CMD6_RG_RX_ARCMD_DDR3_SEL));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD3),
		p_fld(SET_FLD, RG_ARCMD_REV_BIT_20_DATA_SWAP_EN) |
		p_fld(SET_FLD, RG_ARCMD_REV_BIT_22_DATA_SWAP) |
		p_fld(CLEAR_FLD, RG_ARCMD_REV_BIT_21_DATA_SWAP));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD5),
		p_fld(CLEAR_FLD,	SHU1_CA_CMD5_RG_RX_ARCMD_VREF_BYPASS) |
		p_fld(0xb, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL));
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), SET_FLD,
		SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_CA_CMD0), CLEAR_FLD,
		CA_CMD0_RG_RX_ARCA2_OFFC);
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_CA_CMD5),
		p_fld(SET_FLD, CA_CMD5_RG_RX_ARCMD_VREF_EN) |
		p_fld(SET_FLD, CA_CMD5_RG_RX_ARCMD_EYE_VREF_EN));
	delay_us(1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), SET_FLD,
		SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT2);

	delay_us(1);

	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), SET_FLD,
		SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), SET_FLD,
		SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), SET_FLD,
		SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT0);
}

void duty_scan_offset_calibration(DRAMC_CTX_T *p)
{
	unsigned char cal_seq[NIBBLE_MAX] = {
		0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x0, 0x1, 0x2, 0x3,
		0x4, 0x5, 0x6, 0x7 };
	unsigned char i, read_val_b0, read_val_b1, read_val_ca;
	unsigned char cal_i_b0 = BYTE_MAX, cal_i_b1 = BYTE_MAX,
		cal_i_ca = BYTE_MAX;

	print_calibration_basic_info(p);

	show_msg((INFO, "[Duty_Offset_Calibration]\n\n"));

	duty_scan_offset_cal_init(p);

	show_msg((INFO, "\tB0\tB1\tCA\n"));
	show_msg((INFO, "===========================\n"));

	for (i = 0; i < NIBBLE_MAX; i++) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DQ0),
			cal_seq[i], B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ0),
			cal_seq[i], B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_CA_CMD0),
			cal_seq[i], CA_CMD0_RG_RX_ARCA2_OFFC);

		delay_us(1);

		read_val_b0 = io_32_read_fld_align(
			DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1),
			MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B0_BIT2);
		read_val_b1 = io_32_read_fld_align(
			DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1),
			MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B1_BIT2);
		read_val_ca = io_32_read_fld_align(
			DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_CMD_O1),
			MISC_AD_RX_CMD_O1_AD_RX_ARCA2_O1);

		show_msg((INFO, "%d\t%d\t%d\t%d\n",
			duty_scan_offset_convert(i), read_val_b0,
			read_val_b1, read_val_ca));

		if (read_val_b0 == 0 && cal_i_b0 == BYTE_MAX)
			cal_i_b0 = i;

		if (read_val_b1 == 0 && cal_i_b1 == BYTE_MAX)
			cal_i_b1 = i;

		if (read_val_ca == 0 && cal_i_ca == BYTE_MAX)
			cal_i_ca = i;
	}

	if (cal_i_b0 == 0 || cal_i_b1 == 0 || cal_i_ca == 0) {
		show_err("offset calibration i=-7 and AD_RX_*RDQ_O1_B*<2>/");
		show_err("AD_RX_*RCA2_O1 ==0 !!\n");
	} else if ((read_val_b0 == 1 && cal_i_b0 == BYTE_MAX) ||
		(read_val_b1 == 1 && cal_i_b1 == BYTE_MAX) ||
		(read_val_ca == 1 && cal_i_ca == BYTE_MAX)) {
		show_err("offset calibration i=-7 and AD_RX_*RDQ_O1_B*<2>/");
		show_err("AD_RX_*RCA2_O1 ==0 !!\n");

	} else {
		show_msg((INFO, "===========================\n"));
		show_msg((INFO, "\tB0:%d\tB1:%d\tCA:%d\n",
			duty_scan_offset_convert(cal_i_b0),
			duty_scan_offset_convert(cal_i_b1),
			duty_scan_offset_convert(cal_i_ca)));
	}
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), CLEAR_FLD,
		SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), CLEAR_FLD,
		SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), CLEAR_FLD,
		SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT0);

	if (cal_i_b0 != BYTE_MAX)
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DQ0),
			cal_seq[cal_i_b0], B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
	if (cal_i_b1 != BYTE_MAX)
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ0),
			cal_seq[cal_i_b1], B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
	if (cal_i_ca != BYTE_MAX)
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_CA_CMD0),
			cal_seq[cal_i_ca], CA_CMD0_RG_RX_ARCA2_OFFC);
}

void dramc_new_duty_calibration(DRAMC_CTX_T *p)
{
	unsigned char backup_channel;
#if 0
	static unsigned int duty_reg_backup_address[] = {
		(DDRPHY_B0_DQ6),
		(DDRPHY_SHU1_B0_DQ3),
		(DDRPHY_SHU1_B0_DQ5),
		(DDRPHY_SHU1_B0_DQ0),
		(DDRPHY_B0_DQ0),
		(DDRPHY_B0_DQ5),
		(DDRPHY_B0_DQ8),
		(DDRPHY_SHU1_R0_B0_DQ7),
		(DDRPHY_B0_DLL_ARPI0),
		(DDRPHY_B0_DLL_ARPI2),

		(DDRPHY_B1_DQ6),
		(DDRPHY_SHU1_B1_DQ3),
		(DDRPHY_SHU1_B1_DQ5),
		(DDRPHY_SHU1_B1_DQ0),
		(DDRPHY_B1_DQ0),
		(DDRPHY_B1_DQ5),
		(DDRPHY_B1_DQ8),
		(DDRPHY_SHU1_R0_B1_DQ7),
		(DDRPHY_B1_DLL_ARPI0),
		(DDRPHY_B1_DLL_ARPI2),

		(DDRPHY_CA_CMD6),
		(DDRPHY_SHU1_CA_CMD3),
		(DDRPHY_SHU1_CA_CMD5),
		(DDRPHY_SHU1_CA_CMD0),
		(DDRPHY_CA_CMD0),
		(DDRPHY_CA_CMD5),
		(DDRPHY_CA_CMD9),
		(DDRPHY_SHU1_R0_CA_CMD9),
		(DDRPHY_CA_DLL_ARPI0),
		(DDRPHY_CA_DLL_ARPI2),
	};
#else
	static unsigned int duty_reg_backup_address[] = {
		(DDRPHY_B0_DQ6),
		(DDRPHY_SHU1_B0_DQ3),
		(DDRPHY_SHU1_B0_DQ5),
		(DDRPHY_SHU1_B0_DQ0),
		(DDRPHY_B0_DQ0),
		(DDRPHY_B0_DQ5),
		(DDRPHY_B0_DQ8),
		(DDRPHY_SHU1_R0_B0_DQ7),
		(DDRPHY_B0_DLL_ARPI0),
		(DDRPHY_B0_DLL_ARPI2),

		(DDRPHY_B1_DQ6),
		(DDRPHY_SHU1_B1_DQ3),
		(DDRPHY_SHU1_B1_DQ5),
		(DDRPHY_SHU1_B1_DQ0),
		(DDRPHY_B1_DQ0),
		(DDRPHY_B1_DQ5),
		(DDRPHY_B1_DQ8),
		(DDRPHY_SHU1_R0_B1_DQ7),
		(DDRPHY_B1_DLL_ARPI0),
		(DDRPHY_B1_DLL_ARPI2),

		(DDRPHY_CA_CMD6), /* Only in B01 */
		(DDRPHY_SHU1_CA_CMD3),
		(DDRPHY_SHU1_CA_CMD5),
		(DDRPHY_SHU1_CA_CMD0),
		(DDRPHY_CA_CMD0),
		(DDRPHY_CA_CMD5),
		(DDRPHY_CA_CMD9),
		(DDRPHY_SHU1_R0_CA_CMD9),
		(DDRPHY_CA_DLL_ARPI0),
		(DDRPHY_CA_DLL_ARPI2),
	};
#endif

	if ((p->frequency == dfs_get_highest_freq(p))
		 && (get_pre_miock_jmeter_hqa_used_flag() == 0)) {
		unsigned char channel_idx;

		backup_channel = get_phy_2_channel_mapping(p);

		enable_dramc_phy_dcm(p, 0); /* Clk free run */

		for (channel_idx = p->channel;
			channel_idx < p->support_channel_num;
			channel_idx++) {
			set_phy_2_channel_mapping(p, channel_idx);

			/* backup register value */
			dramc_backup_registers(p, duty_reg_backup_address,
				sizeof(duty_reg_backup_address)
				/ sizeof(unsigned int));

			duty_scan_offset_calibration(p);
			/* restore to orignal value */
			dramc_restore_registers(p, duty_reg_backup_address,
				sizeof(duty_reg_backup_address)
				/ sizeof(unsigned int));
			/*
			* Set K DQS MCK4X_DLY_EN and MCK4XB_DLY_EN again,
			* this is especially for K DQS because other bit fields
			* need to be restored.
			*/
			dqs_duty_scan_set_dqs_delay_cell(p, p->channel,
				final_k_dqs_clk_delay_cell);
		}
		set_phy_2_channel_mapping(p, backup_channel);
	}
}
#endif

#ifdef ENABLE_MIOCK_JMETER
/*
 * "picoseconds per delay cell" depends on Vcore only
  * (frequency doesn't matter)
 * 1. Retrieve current freq's vcore voltage using pmic API
 * 2. Perform delay cell time calculation
 * (Bypass if shuffle vcore value is the same as before)
 */
static void get_vcore_delay_cell_time(DRAMC_CTX_T *p, unsigned char shuffleIdx)
{
	unsigned int channel_i;

	dramc_miock_jmeter(p);

	for (channel_i = CHANNEL_A; channel_i < CHANNEL_NUM;
		channel_i++) {
		num_dlycell_per_t_all[shuffleIdx][channel_i] =
			num_dlycell_per_t[p->channel];
		delay_cell_ps_all[shuffleIdx][channel_i] =
			delay_cell_ps[p->channel];
	}
}

unsigned char get_hqa_shuffle_idx(DRAMC_CTX_T *p)
{
#if 0 /* cc mark since only SHUFFLE-0 is supported */
	unsigned char shuffle_idx = 0;
	DRAM_DFS_FREQUENCY_TABLE_T *pfreq_tbl;

	pfreq_tbl = freq_tbl;

	/* Retrieve shuffle number from gFreqTbl */
	for (shuffle_idx = 0; shuffle_idx < DRAM_DFS_SHUFFLE_MAX;
		shuffle_idx++) {
		if (pfreq_tbl[shuffle_idx].frequency == p->frequency)
			break;
	}

	if (shuffle_idx == DRAM_DFS_SHUFFLE_MAX)
		show_log("shuffle num err!\n");
	else
		show_msg3((INFO, "shuffleIdx %d\n", shuffle_idx));

	return shuffle_idx;
#endif

	return DRAM_DFS_SHUFFLE_1;
}

void dramc_miock_jmeter_hqa(DRAMC_CTX_T *p)
{
	/* do MiockJitterMeter@DDR2667 */
	unsigned char shuffle_idx;

	show_msg((INFO, "[MiockJmeterHQA]\n"));

	shuffle_idx = get_hqa_shuffle_idx(p);
	get_vcore_delay_cell_time(p, shuffle_idx);

	/* Use highest freq's delay cell time measure results as reference */
	p->ucnum_dlycell_perT = num_dlycell_per_t_all[shuffle_idx][p->channel];
	p->delay_cell_timex100 = delay_cell_ps_all[shuffle_idx][p->channel];

	show_msg3((INFO, "DelayCellTimex100 CH_%d, (VCORE=%d, cell=%d)\n",
		p->channel, vcore_value[shuffle_idx], p->delay_cell_timex100));
}

#endif /* #ifdef ENABLE_MIOCK_JMETER */

#if (ENABLE_DUTY_SCAN && ENABLE_MIOCK_JMETER)
static void set_enable_duty_scan_rg(DRAMC_CTX_T *p)
{
	/* MCK4X CG */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), CLEAR_FLD,
		MISC_CTRL1_R_DMDQSIENCG_EN);

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		CLEAR_FLD, DRAMC_PD_CTRL_DCMEN);

	/*
	* Enable DQ eye scan
	* RG_??_RX_EYE_SCAN_EN
	* RG_??_RX_VREF_EN
	* RG_??_RX_SMT_EN
	*/
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), SET_FLD,
		PLL1_RG_RX_EYE_SCAN_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_CA_CMD3), SET_FLD,
		CA_CMD3_RG_RX_ARCMD_SMT_EN);
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_CA_CMD2),
		p_fld(SET_FLD, CA_CMD2_RG_TX_ARCLK_JM_EN) |
		p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_JM_SEL)); /* LPBK_CLK */

	/* Enable MIOCK jitter meter mode ( RG_RX_MIOCK_JIT_EN=1) */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), SET_FLD,
		PLL1_RG_RX_MIOCK_JIT_EN);

	/* Disable DQ eye scan (b'1), for counter clear */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), CLEAR_FLD,
		PLL1_RG_RX_EYE_SCAN_EN);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_PLL1), CLEAR_FLD,
		PLL1_R_DMDQSERRCNT_DIS);
}

static void set_clk_duty_code(DRAMC_CTX_T *p, signed char duty_code)
{
	if (duty_code < 0) {
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD2),
			p_fld(SET_FLD, CA_CMD2_RG_TX_ARCLK_MCK4X_DLY_EN) |
			p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_MCK4XB_DLY_EN));
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD10),
			p_fld(-duty_code, SHU1_R0_CA_CMD10_RG_RK0_TX_ARCLK_DLY) |
			p_fld(0x0, SHU1_R0_CA_CMD10_RG_RK0_TX_ARCLKB_DLY));
	} else {
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD2),
			p_fld(CLEAR_FLD, CA_CMD2_RG_TX_ARCLK_MCK4X_DLY_EN) |
			p_fld(SET_FLD, CA_CMD2_RG_TX_ARCLK_MCK4XB_DLY_EN));
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD10),
			p_fld(0x0, SHU1_R0_CA_CMD10_RG_RK0_TX_ARCLK_DLY) |
			p_fld(duty_code, SHU1_R0_CA_CMD10_RG_RK0_TX_ARCLKB_DLY));
	}
}
DRAM_STATUS_T dramc_clk_duty_scan(DRAMC_CTX_T *p)
{
	unsigned char search_state, clk_dly, fgcurrent_value, fginitial_value=0,
		start_period = 0, middle_period = 0, end_period = 0;
	unsigned short real_freq, real_period;
	unsigned short high_pulse_width, period_width;
	unsigned int duty_cycle;
	unsigned char rx_gating_pi = 0;
	signed char duty_code, best_code;
	unsigned short diff, min_diff;
	unsigned char step;
	unsigned int reg_backup_address[] = {
		(DRAMC_REG_DRAMC_PD_CTRL),
		((DDRPHY_PLL1)),
		((DDRPHY_PLL2)),
		((DDRPHY_PLL4)),

		(DDRPHY_CA_CMD2),
		(DDRPHY_CA_CMD3),
		(DDRPHY_MISC_CTRL1),
	};

	rx_gating_pi = 0x10;

#if (FOR_DV_SIMULATION_USED == 0)
	step = 1;
#else
	step = 4;
#endif

	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	set_enable_duty_scan_rg(p);

	min_diff = 1000;
	for (duty_code = -7; duty_code < 8; duty_code++) {
		/* Adjust clock duty */
		set_clk_duty_code(p, duty_code);

		search_state = 0;

		/* to see 1T(H,L) or 1T(L,H) from delaycell=0 to 127 */
		for (clk_dly = 0; clk_dly < 128; clk_dly += step) {

			fgcurrent_value = check_counter_jmeter(p, clk_dly, TRUE);

			/* more than 1T data */
			if (search_state == 0) {
				/* record initial value at the beginning */
				fginitial_value = fgcurrent_value;
				search_state = 1;
			} else if (search_state == 1) {
				/*  check if change value */
				if (fgcurrent_value != fginitial_value) {
					/* start of the period */
					fginitial_value = fgcurrent_value;
					start_period = clk_dly;
					search_state = 2;
				}
			} else if (search_state == 2) {
				/*  check if change value */
				if (fgcurrent_value != fginitial_value) {
					fginitial_value = fgcurrent_value;
					middle_period = clk_dly;
					search_state = 3;
				}
			} else if (search_state == 3) {
				/*  check if change value */
				if (fgcurrent_value != fginitial_value) {
					/* end of the period, break the loop */
					end_period = clk_dly;
					search_state = 4;
					break;
				}
			} else { /* nothing */
			}
		}

		if (search_state != 4) {
			show_msg2((INFO, "Cannot find 1T to calculate duty\n"));
			break;
		} else {
			if (fgcurrent_value == 1) {
				/* The end is a rising edge */
				high_pulse_width = middle_period - start_period;
			} else {
				/* Falling edge ending */
				high_pulse_width = end_period - middle_period;
			}

			period_width = end_period - start_period;
			duty_cycle = (high_pulse_width * 1000) / period_width;
			show_msg2((INFO, "The duty cycle with duty_code %d is %d.%d\n",
				duty_code, duty_cycle/10,
				duty_cycle - (duty_cycle/10)*10));
		}

		if (duty_cycle > 500) {
			diff = duty_cycle - 500;
		} else {
			diff = 500 - duty_cycle;
		}

		if (diff < min_diff) {
			min_diff = diff;
			best_code = duty_code;
			show_msg2((INFO, "Current best code = %d\n", best_code));
		}
	}

	/* Set best code to RG */
	set_clk_duty_code(p, best_code);

	/* restore to orignal value */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	return DRAM_OK;
}

#endif /* #ifdef ENABLE_MIOCK_JMETER */

void dramc_write_dbi_on_off(DRAMC_CTX_T *p, unsigned char onoff)
{
	/*  DRAMC Write-DBI On/Off */
	io_32_write_fld_align_all(DRAMC_REG_SHU1_WODT, onoff,
		SHU1_WODT_DBIWR);
	show_msg((INFO, "DramC Write-DBI %s\n",
		((onoff == DBI_ON) ? "on" : "off")));
}

void dramc_read_dbi_on_off(DRAMC_CTX_T *p, unsigned char onoff)
{
	/*  DRAMC Read-DBI On/Off */
	io_32_write_fld_align_all(DDRPHY_SHU1_B0_DQ7, onoff,
		SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0);
	io_32_write_fld_align_all(DDRPHY_SHU1_B1_DQ7, onoff,
		SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
	show_msg((INFO, "DramC Read-DBI %s\n",
		((onoff == DBI_ON) ? "on" : "off")));
}

#if ENABLE_WRITE_DBI
void dramc_write_minus_1mck_for_write_dbi(DRAMC_CTX_T *p,
	signed char shift_ui)
{
	REG_TRANSFER_T transfer_reg[2];
	if (p->dbi_w_onoff[p->dram_fsp]) {
		/* DQ0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);

		/* DQ1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);

		/* DQM0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);

		/* DQM1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
	}
}

void set_dram_mode_reg_for_write_dbi_on_off(DRAMC_CTX_T *p,
	unsigned char onoff)
{
	/* DRAM MR3[7] write-DBI On/Off */
	dram_mr.mr03_value[p->dram_fsp] =
		((dram_mr.mr03_value[p->dram_fsp] & 0x7F) | (onoff << 7));
	dramc_mode_reg_write_by_rank(p, p->rank, MR03,
		dram_mr.mr03_value[p->dram_fsp]);
}
#endif

#if ENABLE_WRITE_DBI
void apply_write_dbi_power_improve(DRAMC_CTX_T *p, unsigned char onoff)
{
}
#endif


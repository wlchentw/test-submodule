
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
 * @file dramc_cbt_api.c
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
unsigned char cbt_eye_scan_flag = FALSE;
unsigned char cbt_eye_scan_only_higheset_freq_flag = 1;

unsigned char cbt_vref_range_sel;
unsigned char cbt_vref_range_begin;
unsigned char cbt_vref_range_end;
unsigned char cbt_vref_range_step;

signed int ca_train_cmd_delay[CHANNEL_NUM][RANK_MAX] ={0};
unsigned int ca_train_cs_delay[CHANNEL_NUM][RANK_MAX] = {0};
signed int ca_train_clk_delay[CHANNEL_NUM][RANK_MAX] = {0};
#define CBT_WORKAROUND_B0_B1_SWAP		1

/* CA perbit delaycell calculate */
static void cbt_cal_delay_cell_perbit(DRAMC_CTX_T *p,
	signed short center_min, unsigned char delay_cell_ofst[],
	PASS_WIN_DATA_T final_win_per_ca[])
{
	unsigned char ca_idx;
	unsigned char ca_max;

	if (p->dram_type == TYPE_LPDDR4)
		ca_max = CATRAINING_NUM_LP4;
	else
		ca_max = CATRAINING_NUM_LP3;

	/* calculate delay cell perbit */
	for (ca_idx = 0; ca_idx < ca_max; ca_idx++) {
		unsigned char u1PIDiff =
			final_win_per_ca[ca_idx].win_center -
			center_min;
		if (p->delay_cell_timex100 != 0) {
			delay_cell_ofst[ca_idx] = (u1PIDiff * 100000000 /
				(p->frequency * 64)) / p->delay_cell_timex100;
			show_msg((INFO,
				"delay_cell_ofst[%d]=%d cells (%d PI)\n",
				ca_idx, delay_cell_ofst[ca_idx],
				u1PIDiff));
		} else {
			show_msg((INFO, "Error: Cell time %s is 0\n",
				"(p->delay_cell_timex100)"));
			memset((void *)delay_cell_ofst, 0, ca_max);
			break;
		}
	}
}

static unsigned char cbt_enable_perbit_adjust(DRAMC_CTX_T *p)
{
	unsigned int ret = 0;

	/* If high freq && delay_cell is calibrated */
	if ((p->frequency >= PERBIT_THRESHOLD_FREQ) &&
		(p->delay_cell_timex100 != 0))
		ret = 0;  /* disable cbt perbit tune */

	return ret;
}

#if (SUPPORT_TYPE_LPDDR4 && SIMULATION_CBT)
unsigned char get_cbt_vref_pin_mux_revert_value(DRAMC_CTX_T *p,
	unsigned char vref_level);

/* cbt_lp4: cmd_bus_training_lp4
 *  start the calibrate the skew between
 *	(1) Clk pin and CAx pins.
 *	(2) Clk and CS pin
 *	(3) Vref(ca) driving
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
void cbt_switch_freq(DRAMC_CTX_T *p, unsigned char freq)
{
#if DUAL_FREQ_K
	if (freq == CBT_LOW_FREQ)
		cbt_dramc_dfs_direct_jump(p, DRAM_DFS_SHUFFLE_2);
	else
		cbt_dramc_dfs_direct_jump(p, DRAM_DFS_SHUFFLE_1);

#endif
}

static void set_dram_mrcbt_on_off(DRAMC_CTX_T *p, unsigned char on_off,
	unsigned char op_fsp)
{
	if (on_off) {
		/* MR13 OP[0]=1, enable CBT */
		dram_mr.mr13_value[op_fsp] |= 0x1;

		/*
		* op[7] = !(p->dram_fsp),
		* dram will switch to another FSP_OP automatically
		*/
		if (op_fsp)
			dram_mr.mr13_value[op_fsp] &= 0x7f; /* OP[7] =0; */
		else
			dram_mr.mr13_value[op_fsp] |= 0x80; /* OP[7] =1; */
	} else {
		dram_mr.mr13_value[op_fsp] &= 0xfe; /* disable CBT */

		if (op_fsp)
			dram_mr.mr13_value[op_fsp] |= 0x80; /* OP[7] =1; */
		else
			dram_mr.mr13_value[op_fsp] &= 0x7f; /* OP[7] =0; */
	}

#if (CBT_K_RANK1_USE_METHOD == 1)
	dramc_mode_reg_write_by_rank(p, p->rank, MR13,
		dram_mr.mr13_value[op_fsp]);
#else
	dramc_mode_reg_write_by_rank(p, RANK_0, MR13,
		dram_mr.mr13_value[op_fsp]);
#endif
}

void CBTEntry(DRAMC_CTX_T *p, unsigned char operating_fsp,
	unsigned short operation_frequency)
{
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN) |
		p_fld(CLEAR_FLD, DRAMC_PD_CTRL_DCMEN) |
		p_fld(0x3, DRAMC_PD_CTRL_DQIEN_BUFFEN_OPT)); /* cc add to enable DQ-RX */

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_STBCAL), CLEAR_FLD,
		STBCAL_DQSIENCG_NORMAL_EN);

	/* Step 0.0 CKE go high (Release R_DMCKEFIXOFF, R_DMCKEFIXON=1) */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		SET_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF);	/* MIOCKCTRLOFF=1 */

	cke_fix_on_off(p, CKE_FIXON, CKE_WRITE_TO_ONE_CHANNEL);

	/* Step 0: MRW MR13 OP[0]=1 to enable CBT */
	set_dram_mrcbt_on_off(p, ENABLE, operating_fsp);

	/*
	* Step 0.1: before CKE low, Let DQS=0 by R_DMwrite_level_en=1,
	* spec: DQS_t has to retain a low level during tDQSCKE period
	*/
	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			SET_FLD, WRITE_LEV_WRITE_LEVEL_EN);
	}

	delay_us(1);

	/* Step 1.0: let CKE go low */
	cke_fix_on_off(p, CKE_FIXOFF, CKE_WRITE_TO_ONE_CHANNEL);

	/* Step 1.1 : let IO to O1 path valid */
	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		/*
		* Let R_DMFIXDQIEN1=1 (byte1), 0xd8[13]
		* ==> Note: Do not enable again.
		* Currently set in o1_path_on_off
		*/

		/* Let DDRPHY RG_RX_ARDQ_SMT_EN_B1=1 (byte1) */
		o1_path_on_off(p, 1);
	}
}

void cbt_exit(DRAMC_CTX_T *p, unsigned char operating_fsp,
	unsigned char operation_frequency)
{

	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		/*
		* Step 1: CKE go high
		* (Release R_DMCKEFIXOFF, R_DMCKEFIXON=1)
		*/
		cke_fix_on_off(p, CKE_FIXON, CKE_WRITE_TO_ONE_CHANNEL);

		/* Step 2:wait tCATX, wait tFC */
		delay_us(1);

		/*
		* Step 3: MRW to command bus training exit
		* (MR13 OP[0]=0 to disable CBT)
		*/
		set_dram_mrcbt_on_off(p, DISABLE, operating_fsp);
	}
	/*
	* Step 4:
	* Disable O1 path output
	*/
	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		/* Let DDRPHY RG_RX_ARDQ_SMT_EN_B1=0 */
		o1_path_on_off(p, DISABLE);

		/*
		* Let FIXDQIEN1=0 ==> Note: Do not enable again.
		* Moved into o1_path_on_off
		*/
	}
}

static void cbt_set_fsp(DRAMC_CTX_T *p, unsigned char operating_fsp,
	unsigned char final_set_flag)
{
	if (operating_fsp == FSP_0) {/* OP[6], fsp_wr=0, OP[7] =0; */
		dram_mr.mr13_value[operating_fsp] &= ~(SET_FLD << 6);
		dram_mr.mr13_value[operating_fsp] &= 0x7f;
	} else {/* OP[6], fsp_wr=1, OP[7] =0; */
#if DUAL_FREQ_K
		 if (final_set_flag == 0) {
			dram_mr.mr13_value[operating_fsp] |= (SET_FLD << 6);
			dram_mr.mr13_value[operating_fsp] &= 0x7f;
		} else {
			dram_mr.mr13_value[operating_fsp] |= (SET_FLD << 6);
			dram_mr.mr13_value[operating_fsp] |= 0x80;
		}
#else
		dram_mr.mr13_value[operating_fsp] |= (SET_FLD << 6);
		dram_mr.mr13_value[operating_fsp] |= 0x80;
#endif
	}

	dramc_mode_reg_write_by_rank(p, p->rank, MR13,
		dram_mr.mr13_value[operating_fsp]);
}

static void cbt_set_vref(DRAMC_CTX_T *p, unsigned int vref_level,
	unsigned char operating_fsp, unsigned char final_set_flag)
{
	unsigned int dbg_value;
	unsigned char vref_value_pinmux;

	vref_value_pinmux =
		(get_cbt_vref_pin_mux_revert_value(p, vref_level) & 0x3f);

	if ((p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) &&
		(final_set_flag == DISABLE)) {
		dram_mr.mr12_value[p->channel][p->rank][operating_fsp] =
		    ((cbt_vref_range_sel & 0x1) << 6) | vref_value_pinmux;

		/* MR12, bit[25:20]=OP[5:0]  bit 26=OP[6] */
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			vref_level, WRITE_LEV_DMVREFCA);

		/* If only B23 used, Select DQS1 (which is mapped to
		 * PHY PAD_DQS2) to send VREFca. Else, shall select
		 * DQS0.
		 */
	#if CBT_WORKAROUND_B0_B1_SWAP
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_PADCTRL),
			SET_FLD, PADCTRL_DRAMOEN);
	#endif
		if (p->channel == CHANNEL_B) {
			io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
				0x2, WRITE_LEV_DQS_SEL);
			io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
				SET_FLD, WRITE_LEV_CBTMASKDQSOE);
		} else {
			io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
				SET_FLD, WRITE_LEV_DQS_SEL);
		}

		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			p_fld(SET_FLD, WRITE_LEV_DQS_B3_G) |
			p_fld(CLEAR_FLD, WRITE_LEV_DQS_B2_G) |
			p_fld(SET_FLD, WRITE_LEV_DQS_B1_G) |
			p_fld(CLEAR_FLD, WRITE_LEV_DQS_B0_G));

		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			SET_FLD, WRITE_LEV_DQS_WLEV);
		delay_us(1);
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			CLEAR_FLD, WRITE_LEV_DQS_WLEV);

	#if CBT_WORKAROUND_B0_B1_SWAP
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_PADCTRL),
			CLEAR_FLD, PADCTRL_DRAMOEN);
	#endif

	} else {
		dbg_value = (((cbt_vref_range_sel & 0x1) << 6) |
			(vref_level & 0x3f));
		dram_mr.mr12_value[p->channel][p->rank][operating_fsp] =
			dbg_value;
		show_msg2((INFO, "dbg_value = 0x%x\n", dbg_value));

		dramc_mode_reg_write_by_rank(p, p->rank, MR12, dbg_value);
	}

	/* wait tVREF_LONG */
	delay_us(1);
}

static unsigned int cbt_delay_compare(DRAMC_CTX_T *p, signed int delay)
{
	unsigned int result = 0, result0 = 0, ready;
	unsigned int time_cnt;

	time_cnt = TIME_OUT_CNT;

	if (delay < 0) {	/* Set CLK delay */
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(CLEAR_FLD,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(-delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(-delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	} else {		/* Set CA output delay */
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(CLEAR_FLD,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));

	}

	/* Let R_DMTESTCATRAIN=1 to enable HW CAPAT Generator */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), SET_FLD,
		CATRAINING1_TESTCATRAIN);

	/* Check CA training compare ready */
	do {
		ready =
		    io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_TCMDO1LAT),
			TCMDO1LAT_CATRAIN_CMP_CPT);
		time_cnt--;
		delay_us(1);
	} while ((ready == 0) && (time_cnt > 0));

	if (time_cnt == 0)
		show_err("[cbt_delay_compare] Resp fail (time out)\n");

	/* Get CA training compare result */
	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		result = io_32_read_fld_align(
			DRAMC_REG_ADDR(DRAMC_REG_TCMDO1LAT),
			TCMDO1LAT_CATRAIN_CMP_ERR);
	}

	/* Let R_DMTESTCATRAIN=0 to disable HW CAPAT Generator */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1),
		CLEAR_FLD, CATRAINING1_TESTCATRAIN);

	return (result | result0);	/* return pattern compre result */
}

static unsigned int cbt_delay_cs_compare(DRAMC_CTX_T *p, unsigned int delay)
{
	unsigned char *lpddr_phy_mapping = NULL;
	unsigned int result, ready;
	unsigned int time_cnt;
	unsigned int dq_o1;
	unsigned int byte_index;

	time_cnt = TIME_OUT_CNT;

	lpddr_phy_mapping =
		(unsigned char *) lpddr4_phy_mapping_pop[p->channel];

	/* Set CS output delay */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9), delay,
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS);

	/* Step 5: toggle CS/CA for CS training by R_DMTCMDEN */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), SET_FLD,
		SPCMD_TCMDEN);
	do {
		ready = io_32_read_fld_align(
			DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP),
			SPCMDRESP_TCMD_RESPONSE);
		time_cnt--;
		delay_us(1);
	} while ((ready == 0) && (time_cnt > 0));

	if (time_cnt == 0)
		show_err("[cbt_delay_cs_compare] Resp fail (time out)\n");

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), CLEAR_FLD,
		SPCMD_TCMDEN);

	/*
	* Step 6: check CS training result on DQ[13:8] by O1, DDRPHYCFG 0xF80
	* Expected CA value is h2a (CA pulse width is 6UI, CS pulse is 1UI)
	*/
	dq_o1 = io_32_read_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_DQO1),
		MISC_DQO1_DQO1_RO);

	if (p->channel == CHANNEL_B)
		dq_o1 >>= 16; /* LP4 uses B23 */

	result = 0;
	for (byte_index = 8; byte_index <= 13; byte_index++) {
		result |=
		    (((dq_o1 & (1 << lpddr_phy_mapping[byte_index])) >>
		      (lpddr_phy_mapping[byte_index])) << (byte_index - 8));
	}
	show_msg3((INFO, "CS Dly = %d, Result=0x%x\n", delay, result));

	return result;	/* return pattern compre result */
}

void dramc_cmd_bus_training_post_process(DRAMC_CTX_T *p)
{
	signed int cs_Final_clk_elay, cs_Final_cmd_delay, cs_finalcs_delay;
	unsigned char backup_rank, irank;

	/* CBT Rank0/1 must set Clk/CA/CS the same */

	show_msg((INFO,
		"[dramc_cmd_bus_training_post_process] p->frequency=%d\n",
		p->frequency));

	backup_rank = get_rank(p);

	cs_Final_clk_elay =
		(ca_train_clk_delay[p->channel][RANK_0] +
		ca_train_clk_delay[p->channel][RANK_1]) / 2;
		ca_train_clk_delay[p->channel][RANK_0] = cs_Final_clk_elay;
		ca_train_clk_delay[p->channel][RANK_1] = cs_Final_clk_elay;

	cs_Final_cmd_delay =
		(ca_train_cmd_delay[p->channel][RANK_0] +
		ca_train_cmd_delay[p->channel][RANK_1]) / 2;
		ca_train_cmd_delay[p->channel][RANK_0] = cs_Final_cmd_delay;
		ca_train_cmd_delay[p->channel][RANK_1] = cs_Final_cmd_delay;

	cs_finalcs_delay =
		(ca_train_cs_delay[p->channel][RANK_0] +
		ca_train_cs_delay[p->channel][RANK_1]) / 2;
		ca_train_cs_delay[p->channel][RANK_0] = cs_finalcs_delay;
		ca_train_cs_delay[p->channel][RANK_1] = cs_finalcs_delay;

	for (irank = RANK_0; irank <= RANK_1; irank++) {
		set_rank(p, irank);
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(cs_Final_clk_elay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(cs_Final_cmd_delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(cs_finalcs_delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	}

	show_msg((INFO, "Clk Dly = %d\nCmd Dly = %d\nCS Dly = %d\n",
		cs_Final_clk_elay, cs_Final_cmd_delay, cs_finalcs_delay));

	set_rank(p, backup_rank);

}

static void cbt_adjust_cs(DRAMC_CTX_T *p)
{
	signed int first_cs_pass, last_cs_pass, cs_final_delay;
	unsigned int delay, value_read_back, cs_win_size;
	unsigned short step;

#if (FOR_DV_SIMULATION_USED == 0)
	step = 1;
#else
	step = 8;
#endif

	first_cs_pass = -1;
	last_cs_pass = -1;

	for (delay = 0; delay <= MAX_CS_PI_DELAY; delay += step) {
		value_read_back = cbt_delay_cs_compare(p, delay);

		if (first_cs_pass == -1) {
			if (value_read_back == 0x2a) { /* compare pass */
				first_cs_pass = delay;
			}
		} else if (last_cs_pass == -1) {
			if (value_read_back != 0x2a) { /* compare fail */
				last_cs_pass = delay - 1;
				break; /* Early break */
			} else if (delay == MAX_CS_PI_DELAY) {
				last_cs_pass = delay;
				break; /* Early break */
			}
		}

		/*  Wait time before output CS pattern to DDR again..  */
		delay_us(1);
	}

	cs_win_size = last_cs_pass - first_cs_pass +
		(last_cs_pass == first_cs_pass ? 0 : 1);

	/*  if winSize >32, CS delay= winSize -32. */
	if (cs_win_size > ((MAX_CS_PI_DELAY + 1) >> 1))
		cs_final_delay = cs_win_size - ((MAX_CS_PI_DELAY + 1) >> 1);
	else
		cs_final_delay = 0;

	/*
	* Set CS output delay after training
	* p->rank = RANK_0, save to Reg Rank0 and Rank1, p->rank = RANK_1,
	* save to Reg Rank1
	*/
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
		cs_final_delay, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS);

	show_msg((INFO,
		"[CBT_LP4] CS Dly: %d, CSPass First: %d, Last: %d\n",
		cs_final_delay, first_cs_pass, last_cs_pass));

	ca_train_cs_delay[p->channel][p->rank] = cs_final_delay;

}

static void cbt_set_ca_clk_result(DRAMC_CTX_T *p)
{
	/*
	* Set CA_PI_Delay after trainging
	* p->rank = RANK_0, save to Reg Rank0 and Rank1,
	* p->rank = RANK_1, save to Reg Rank1
	*/
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
		p_fld(ca_train_cmd_delay[p->channel][p->rank],
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(ca_train_clk_delay[p->channel][p->rank],
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK));

	show_msg((INFO,
		"[CBTSetCACLKResult] CA Dly = %d, CLK dly =%d\n",
		ca_train_cmd_delay[p->channel][p->rank],
		ca_train_clk_delay[p->channel][p->rank]));
}

unsigned char get_cbt_vref_pin_mux(DRAMC_CTX_T *p,
	unsigned char vref_level)
{
	//unsigned char vref_bit, vref_new, vref_org;
	unsigned char vref_bit, vref_org;
	unsigned char vref_new;

	vref_new = 0;
	vref_org = ((cbt_vref_range_sel & 0x1) << 6) | (vref_level & 0x3f);
	for (vref_bit = 0; vref_bit < 8; vref_bit++) {
		if (vref_org & (1 << vref_bit)) {
			vref_new |= (1 <<
				lpddr4_phy_mapping_pop[p->channel][vref_bit]);
		}
	}

	show_msg2((INFO, "=== vref_new: 0x%x --> 0x%x\n", vref_org, vref_new));

	return vref_new;
}

unsigned char get_cbt_vref_pin_mux_revert_value(DRAMC_CTX_T *p,
	unsigned char vref_level)
{
	unsigned char vref_bit, vref_new, vref_org;

	vref_new = 0;
	vref_org = vref_level;
	for (vref_bit = 0; vref_bit < 8; vref_bit++) {
		vref_new |= ((vref_org >>
		lpddr4_phy_mapping_pop[p->channel][vref_bit]) & 1) << vref_bit;
	}

	show_msg2((INFO, "=== Revert vref_new: 0x%x --> 0x%x\n",
		vref_org, vref_new));

	return vref_new;
}

static void cbt_enter_lp4(DRAMC_CTX_T *p)
{
	unsigned char operating_fsp;
	unsigned short operation_frequency;

	operating_fsp = p->dram_fsp;
	operation_frequency = p->frequency;

	if (operating_fsp == FSP_1)
		cbt_switch_freq(p, CBT_LOW_FREQ);
	/*  Step1: Enter Command Bus Training Mode */
	CBTEntry(p, operating_fsp, operation_frequency);
	/* Step 2: wait tCAENT */
	delay_us(1);
	if (operating_fsp == FSP_1)
		cbt_switch_freq(p, CBT_HIGH_FREQ);

	print_calibration_basic_info(p);

	/* Step 3: set CBT range, verify range and setp */
	cbt_vref_range_sel = 1;	/* MR12,OP[6] */
	cbt_vref_range_step = 2;

	if (p->enable_cbt_scan_vref == DISABLE) {
		cbt_vref_range_begin =
			(dram_mr.mr12_value[p->channel][p->rank][p->dram_fsp]
			& 0x3f);
		cbt_vref_range_end = cbt_vref_range_begin;
	} else {
		if (p->dram_type == TYPE_LPDDR4) {
			/* range 1, 300/1100(VDDQ) = 27.2% */
			cbt_vref_range_begin = CBT_VREF_BEGIN;
			cbt_vref_range_end = CBT_VREF_END;
		} else {
			/* range 1, 290/600(VDDQ)=48.3% */
			cbt_vref_range_begin = CBT_VREF_BEGIN_X;
			cbt_vref_range_end = CBT_VREF_END_X;
		}
	}
	show_msg((INFO, "=== vref scan from %d to %d\n",
		cbt_vref_range_begin, cbt_vref_range_end));
}

static void cbt_leave_lp4(DRAMC_CTX_T *p, unsigned short operation_frequency,
	unsigned char operating_fsp, unsigned char final_vref)
{
	/* wait tVREF_LONG */
	delay_us(1);

	/* -------------  CS and CLK ---------- */
	cbt_adjust_cs(p);

	/* -------  Going to exit Command bus training(CBT) mode.----------- */
	if (operating_fsp == FSP_1)
		cbt_switch_freq(p, CBT_LOW_FREQ);

	cbt_exit(p, operating_fsp, operation_frequency);

	/*
	* normal mode go MR12 set vref again,
	* set final_set_flag to force to MR12 flow
	*/
	if (p->dram_cbt_mode[p->rank] == CBT_NORMAL_MODE) {
		cbt_set_fsp(p, operating_fsp, 1);
		cbt_set_vref(p, final_vref, operating_fsp, 1);
	}

	if (operating_fsp == FSP_1)
		cbt_switch_freq(p, CBT_HIGH_FREQ);

	show_msg((INFO, "[CmdBusTrainingLP4] Done\n"));

	/* restore MRR pin mux */
	set_mrr_pinmux_mapping(p, PINMUX_MRR);

	if (p->rank == RANK_1) {
#if (CBT_K_RANK1_USE_METHOD == 1)
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS),
			CLEAR_FLD, MRS_MRSRK);
		/* use other rank's setting */
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			CLEAR_FLD, RKCFG_TXRANK);
		/*
		* TXRANKFIX should be write after TXRANK
		* or the rank will be fix at rank 1
		*/
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			CLEAR_FLD, RKCFG_TXRANKFIX);
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION),
			SET_FLD,	MPC_OPTION_MPCRKEN);
#else
		dramc_rank_swap(p, RANK_0);
#endif
	}
}

unsigned char cbt_irange_check_lp4(DRAMC_CTX_T *p)
{

	if ((cbt_eye_scan_flag == 1) &&
	    (((cbt_eye_scan_only_higheset_freq_flag == 1)
	      && (p->frequency == dfs_get_highest_freq(p)))
	     || (cbt_eye_scan_only_higheset_freq_flag == 0)))
		return 1;
	else
		return 0;
}

static void _cbt_delayscan_lp4(DRAMC_CTX_T *p, SCAN_WIN_DATA_T *sacn_ptr,
	signed short delay, unsigned int compare_result)
{
	unsigned int ca, temp;

	for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
		if ((sacn_ptr->first_ca_pass[ca] != PASS_RANGE_NA)
			&& (sacn_ptr->last_ca_pass[ca] != PASS_RANGE_NA)) {
			continue;
		}

		/* Get Each bit of CA result */
		temp = (compare_result >> ca) & 0x1;

		if (sacn_ptr->first_ca_pass[ca] == PASS_RANGE_NA) {
			if (temp == 0)
				sacn_ptr->first_ca_pass[ca] = delay;
		} else if (sacn_ptr->last_ca_pass[ca] == PASS_RANGE_NA) {
			if (temp == 1) {
				if ((delay - sacn_ptr->first_ca_pass[ca])
					< 8) {
					sacn_ptr->first_ca_pass[ca] =
						PASS_RANGE_NA;
					continue;
				}
				sacn_ptr->last_ca_pass[ca] = (delay - 1);
			} else if (delay == MAX_CA_PI_DELAY) {
				sacn_ptr->last_ca_pass[ca] = delay;
			}

			if (sacn_ptr->last_ca_pass[ca] != PASS_RANGE_NA) {
				sacn_ptr->finish_count++;
				sacn_ptr->ca_win_sum +=
					(sacn_ptr->last_ca_pass[ca] -
					sacn_ptr->first_ca_pass[ca]);
				sacn_ptr->ca_center[ca] =
					(sacn_ptr->last_ca_pass[ca] +
					sacn_ptr->first_ca_pass[ca]) >> 1;
				sacn_ptr->ca_center_sum +=
					sacn_ptr->ca_center[ca];
			}
		}
	}
}

static void cbt_delayscan_lp4(DRAMC_CTX_T *p, SCAN_WIN_DATA_T *sacn_ptr)
{
	signed short delay;
	unsigned int compare_result;
	unsigned short step;

#if (FOR_DV_SIMULATION_USED == 1)
	step = 8;
#else
	step = 1;
#endif

	for (delay = (-MAX_CLK_PI_DELAY);
			delay <= MAX_CA_PI_DELAY; delay += step) {
		compare_result = cbt_delay_compare(p, delay);

		/* wait 1us between each CA pattern */
		delay_us(1);

		show_msg3((INFO, "CBTDelayCACLK Delay= %d, ",
			 delay));
		show_msg3((INFO, "CompareResult 0x%x\n", compare_result));

		_cbt_delayscan_lp4(p, sacn_ptr, delay, compare_result);

		/*
		* Wait tCACD(22clk)
		* before output CA pattern to DDR again
		*/
		delay_us(1);

		if (sacn_ptr->finish_count == CATRAINING_NUM_LP4)
			break;
	}
}

static void _cbt_scan_lp4(DRAMC_CTX_T *p, FINAL_WIN_DATA_T *final_cbt_set_ptr,
	PASS_WIN_DATA_T final_win_per_ca[], SCAN_WIN_DATA_T scan_result)
{
	signed int ca_final_center[CATRAINING_NUM_LP4] = { 0 };
	signed int ck_min = 1000;
	unsigned int ca;

	memset(ca_final_center, 0, sizeof(ca_final_center));

	if (cbt_enable_perbit_adjust(p)) {
		ck_min = 1000;
		for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
			ca_final_center[ca] = scan_result.ca_center[ca];
			if (ca_final_center[ca] < ck_min)
				ck_min = ca_final_center[ca];
		}

		if (ck_min < 0) {
			show_msg((INFO,
				"Warning!! smallest%s%d < 0, %s !!\n",
				"CA min center = ", ck_min,
				"then adjust to 0"));
			for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
				if (ca_final_center[ca] < 0)
					ca_final_center[ca] = 0;
			}
			ck_min = 0;
		}

		final_cbt_set_ptr->final_ca_clk = ck_min;
	} else {
		final_cbt_set_ptr->final_ca_clk = scan_result.ca_center_sum
			/ CATRAINING_NUM_LP4;
	}

	for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
		final_win_per_ca[ca].first_pass =
			scan_result.first_ca_pass[ca];
		final_win_per_ca[ca].last_pass =
			scan_result.last_ca_pass[ca];
		final_win_per_ca[ca].win_center =
			scan_result.ca_center[ca];
		final_win_per_ca[ca].win_size = (scan_result.last_ca_pass[ca] -
			scan_result.first_ca_pass[ca]) +
			(scan_result.last_ca_pass[ca] ==
			scan_result.first_ca_pass[ca] ? 0 : 1);
	}
}

static void cbt_scan_lp4(DRAMC_CTX_T *p, unsigned char operating_fsp,
	unsigned char irange, unsigned char irange_start,
	unsigned char irange_end,	FINAL_WIN_DATA_T *final_cbt_set_ptr,
	PASS_WIN_DATA_T final_win_per_ca[])
{
	unsigned char vref_level;
	unsigned int ca;
	unsigned int ca_win_sum_max;
	SCAN_WIN_DATA_T scan_result;

	memset(&scan_result, 0x0, sizeof(SCAN_WIN_DATA_T));

	/*  SW variable initialization */
	ca_win_sum_max = 0;

	for (irange = irange_start; irange <= irange_end; irange++) {
		if (cbt_irange_check_lp4(p)) {
			cbt_vref_range_sel = irange;
			cbt_vref_range_begin = 0;
			cbt_vref_range_end = CBT_VREF_MAX;

		#if (FOR_DV_SIMULATION_USED == 0)
			cbt_vref_range_step = 1;
		#else
			cbt_vref_range_step = cbt_vref_range_end;
		#endif

			if (cbt_vref_range_sel == 1)
				cbt_vref_range_begin = CBT_VREF_RANGE1_BEGIN;
		}
		for (vref_level = cbt_vref_range_begin;
			vref_level <= cbt_vref_range_end;
			vref_level += cbt_vref_range_step) {
			cbt_set_vref(p, get_cbt_vref_pin_mux(p, vref_level),
				operating_fsp, 0);

			/*
			 * Delay CA output delay to do CA training
			 * in order to get the pass window.
			 * moving CA relative to CK and repeating
			 * until CA is centered on the latching edge of CK
			 *
			 * Note  !!!!!!!!!!!!!!!!!!!!!!!
			 * Assume : Leave clk as the init value
			 * and adjust CA delay only can find out
			 * each CA window including of the left boundary.
			 * If NOT, we may need to off-line
			 * adjust SELPH2_TXDLY_CMD
			 */

			/*  SW variable initialization */
			scan_result.finish_count = 0;
			scan_result.ca_win_sum = 0;
			scan_result.ca_center_sum = 0;
			for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
				scan_result.last_ca_pass[ca] = PASS_RANGE_NA;
				scan_result.first_ca_pass[ca] = PASS_RANGE_NA;
				scan_result.ca_center[ca] = 0;
			}

			cbt_delayscan_lp4(p, &scan_result);

			/* set CA/CK/CS pi delay to 0 */
			io_32_write_fld_multi_all(DRAMC_REG_ADDR
				(DDRPHY_SHU1_R0_CA_CMD9),
				p_fld(CLEAR_FLD,
				SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
				p_fld(CLEAR_FLD,
				SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
				p_fld(CLEAR_FLD,
				SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));

			if (scan_result.ca_win_sum > ca_win_sum_max) {
				ca_win_sum_max = scan_result.ca_win_sum;
				final_cbt_set_ptr->final_vref = vref_level;
				if (cbt_irange_check_lp4(p))
					final_cbt_set_ptr->final_range =
						cbt_vref_range_sel;

				_cbt_scan_lp4(p, final_cbt_set_ptr,
					final_win_per_ca,  scan_result);
			}

			if (cbt_eye_scan_flag == 0) {
				/* max vref found (95%) , early break; */
				if (scan_result.ca_win_sum <
				    (ca_win_sum_max * 95 / 100)) {
					show_msg2((INFO,
						"\nCBT Vref found, "));
					show_msg2((INFO,
						"early break!\n"));
					break;
				}
			}
		}
	}
}

static void cbt_perbit_adjust_lp4(DRAMC_CTX_T *p,
	unsigned char delay_cell_ofst[])
{
	/* Set Perbit result */
	if (cbt_enable_perbit_adjust(p)) {
		/* CA pinmux:
		 * CA5->ARCA10
		 * CA4->ARCA4
		 * CA3->ARCA2
		 * CA2->ARCA11
		 * CA1->ARCA17
		 * CA0->ARCA18
		 */
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD0
				+ (1 << POS_BANK_NUM),
			p_fld(delay_cell_ofst[5],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA2_DLY) |
			p_fld(delay_cell_ofst[2],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA3_DLY));
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD0,
			p_fld(delay_cell_ofst[4],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA4_DLY) |
			p_fld(delay_cell_ofst[3],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA2_DLY));
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD1,
			p_fld(delay_cell_ofst[1],
				SHU1_R0_CA_CMD1_RK0_TX_ARCMD1_DLY) |
			p_fld(delay_cell_ofst[0],
				SHU1_R0_CA_CMD1_RK0_TX_ARCMD2_DLY));
	}
}
DRAM_STATUS_T cbt_lp4(DRAMC_CTX_T *p)
{
	PASS_WIN_DATA_T final_win_per_ca[CATRAINING_NUM_LP4];
	FINAL_WIN_DATA_T final_cbt_set;
	unsigned int ca;
	unsigned char operating_fsp;
	unsigned short operation_frequency;
	unsigned char irange, irange_start, irange_end;
	unsigned char delay_cell_ofst[CATRAINING_NUM_LP4];
	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL)),
		(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV)),
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL)),
		(DRAMC_REG_ADDR(DDRPHY_B1_DQ2)),
	};

	show_msg((INFO, "\n[CmdBusTrainingLP4]\n"));

	memset(&final_win_per_ca, 0x0, sizeof(PASS_WIN_DATA_T));
	memset(&final_cbt_set, 0x0, sizeof(FINAL_WIN_DATA_T));

	/* Back up dramC register */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));
#if CBT_WORKAROUND_B0_B1_SWAP
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ2),
			SET_FLD, B1_DQ2_RG_TX_ARDQ_OE_DIS_B1);
#endif

	/* When doing CA training, should make sure that
	 * auto refresh is disable.
	 */
	auto_refresh_switch(p, DISABLE);

#if (CBT_K_RANK1_USE_METHOD == 1)
	if (p->rank == RANK_1) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), p->rank,
			MRS_MRSRK);
		/* use other rank's setting */
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			p->rank, RKCFG_TXRANK);
		/*
		* TXRANKFIX should be write
		* after TXRANK or the rank will be fix at rank 1
		*/
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_RKCFG),
			SET_FLD, RKCFG_TXRANKFIX);
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION),
			CLEAR_FLD, MPC_OPTION_MPCRKEN);
	}
#else
	if (p->rank == RANK_1) {
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), RANK_0,
			MRS_MRSRK);
		dramc_rank_swap(p, RANK_1);
	}
#endif

	/*  SW variable initialization */
	final_cbt_set.final_vref =
		dram_mr.mr12_value[p->channel][p->rank][p->dram_fsp] & 0x3f;
	irange_start = irange_end = 1;
	operating_fsp = p->dram_fsp;
	operation_frequency = p->frequency;

	cbt_enter_lp4(p);

	if (cbt_irange_check_lp4(p)) {
		irange_start = 0;
		irange_end = 1;
	}

	cbt_scan_lp4(p, operating_fsp, irange, irange_start, irange_end,
		&final_cbt_set, final_win_per_ca);
	if (cbt_enable_perbit_adjust(p)) {
		cbt_cal_delay_cell_perbit(p, final_cbt_set.final_ca_clk,
			delay_cell_ofst, final_win_per_ca);
	}

	if (cbt_irange_check_lp4(p))
		cbt_vref_range_sel = final_cbt_set.final_range;

	for (ca = 0; ca < CATRAINING_NUM_LP4; ca++) {
		show_msg((INFO, "[CA %d] Center %d (%d~%d) winsize %d\n",
			ca, final_win_per_ca[ca].win_center,
			final_win_per_ca[ca].first_pass,
			final_win_per_ca[ca].last_pass,
			final_win_per_ca[ca].win_size));
#ifdef ETT
		ett_cbt_win[ca] = final_win_per_ca[ca].win_size;
#endif
	}

	/*
	 * Set Vref after trainging, normol mode go DQ pin set vref,
	 * don't set final_set_flag here
	 */
	cbt_set_vref(p, get_cbt_vref_pin_mux(p, final_cbt_set.final_vref),
		operating_fsp, 0);

	show_msg((INFO, "[CBT_LP4] Vref(ca) range %d: %d\n",
		cbt_vref_range_sel, final_cbt_set.final_vref));

	/* Set CA_PI_Delay after trainging */
	if (final_cbt_set.final_ca_clk < 0) { /* Set CLK delay */
		ca_train_clk_delay[p->channel][p->rank] = 0;
		show_err2("Warning!! Clk Dly = %d, adjust to 0 !!\n",
			-final_cbt_set.final_ca_clk);
		ca_train_cmd_delay[p->channel][p->rank] = 0;
	} else { /*  Set CA output delay */
		ca_train_clk_delay[p->channel][p->rank] = 0;
		ca_train_cmd_delay[p->channel][p->rank] =
			final_cbt_set.final_ca_clk;
	}

	cbt_set_ca_clk_result(p);
	cbt_perbit_adjust_lp4(p, delay_cell_ofst);

	show_msg2((INFO, "Average CA Dly: %d\n", final_cbt_set.final_ca_clk));

	cbt_leave_lp4(p, operation_frequency, operating_fsp,
		final_cbt_set.final_vref);

	/* Restore setting registers */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	return DRAM_OK;
}
#endif /* SIMUILATION_LP4_CBT */

#if (SUPPORT_TYPE_LPDDR3 && SIMULATION_CBT)
static unsigned int cbt_delay_compare_lp3(DRAMC_CTX_T *p, signed int delay)
{
	unsigned int result = 0;

	if (delay < 0) { /* Set CLK delay */
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(CLEAR_FLD,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(-delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(-delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	} else { /* Set CA output delay */
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(CLEAR_FLD,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(CLEAR_FLD,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), SET_FLD,
		CATRAINING1_CATRAINEN);

	delay_us(1);

	/* Get CA training compare result */
	result = io32_read_4b(DDRPHY_MISC_DQO1) & 0xffff;
	result |=
		io32_read_4b(DDRPHY_MISC_DQO1 + (1 << POS_BANK_NUM)) &
		0xffff0000;

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1),
		CLEAR_FLD, CATRAINING1_CATRAINEN);

	return result; /* return pattern compare result */
}

void dramc_cmd_bus_training_post_process(DRAMC_CTX_T *p)
{
	signed int cs_Final_clk_elay, cs_Final_cmd_delay, cs_finalcs_delay;
	unsigned char backup_rank, irank;

	/* CBT Rank0/1 must set Clk/CA/CS the same */

	show_msg((INFO,
		"[dramc_cmd_bus_training_post_process] p->frequency=%d\n",
		p->frequency));

	backup_rank = get_rank(p);

	cs_Final_clk_elay =
		(ca_train_clk_delay[p->channel][RANK_0] +
		ca_train_clk_delay[p->channel][RANK_1]) / 2;
		ca_train_clk_delay[p->channel][RANK_0] = cs_Final_clk_elay;
		ca_train_clk_delay[p->channel][RANK_1] = cs_Final_clk_elay;

	cs_Final_cmd_delay =
		(ca_train_cmd_delay[p->channel][RANK_0] +
		ca_train_cmd_delay[p->channel][RANK_1]) / 2;
		ca_train_cmd_delay[p->channel][RANK_0] = cs_Final_cmd_delay;
		ca_train_cmd_delay[p->channel][RANK_1] = cs_Final_cmd_delay;

	cs_finalcs_delay =
		(ca_train_cs_delay[p->channel][RANK_0] +
		ca_train_cs_delay[p->channel][RANK_1]) / 2;
		ca_train_cs_delay[p->channel][RANK_0] = cs_finalcs_delay;
		ca_train_cs_delay[p->channel][RANK_1] = cs_finalcs_delay;

	for (irank = RANK_0; irank <= RANK_1; irank++) {
		set_rank(p, irank);
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
			p_fld(cs_Final_clk_elay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
			p_fld(cs_Final_cmd_delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
			p_fld(cs_finalcs_delay,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));
	}

	show_msg((INFO, "Clk Dly = %d\nCmd Dly = %d\nCS Dly = %d\n",
		cs_Final_clk_elay, cs_Final_cmd_delay, cs_finalcs_delay));

	set_rank(p, backup_rank);

}

static void cbt_set_cbt_result_lp3(DRAMC_CTX_T *p)
{
	/*
	* Set CA_PI_Delay after trainging
	* p->rank = RANK_0, save to Reg Rank0 and Rank1,
	* p->rank = RANK_1, save to Reg Rank1
	*/
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
		p_fld(ca_train_cmd_delay[p->channel][p->rank],
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(ca_train_clk_delay[p->channel][p->rank],
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
		p_fld(ca_train_cs_delay[p->channel][p->rank],
		SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));

	show_msg((INFO,
		"[CBTSetCACLKResult] CA Dly = %d, CLK dly =%d, CS dly = %d\n",
		ca_train_cmd_delay[p->channel][p->rank],
		ca_train_clk_delay[p->channel][p->rank],
		ca_train_cs_delay[p->channel][p->rank]));
}

static void cbt_enter_lp3(DRAMC_CTX_T *p, unsigned char cal_bit4_9,
	unsigned int ca_pattern)
{
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), 1,
		CATRAINING1_CATRAINCSEXT);

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL),
		p_fld(0, CKECTRL_CKEFIXOFF) |
		p_fld(1, CKECTRL_CKEFIXON));

	/* Let CA be stable prior to issue CS */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1),
		1, CATRAINING1_CATRAINMRS);

	if (cal_bit4_9) {
		show_msg2((INFO, "[CBT] Calibration BIT4, BIT9\n"));

		/* Write to MR48 */
		dramc_mode_reg_write(p, 48, 0xc0);
	} else {
		show_msg2((INFO, "[CBT] Calibration BIT1~3, BIT5~8\n"));

		/* Write to MR41 */
		dramc_mode_reg_write(p, 41, 0xa4);
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1),
		0, CATRAINING1_CATRAINMRS);

	delay_us(1);

	/* CKE low */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL),
		p_fld(1, CKECTRL_CKEFIXOFF) |
		p_fld(0,CKECTRL_CKEFIXON));

	io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING2), ca_pattern);

	delay_us(1);
}


static void cbt_leave_lp3(DRAMC_CTX_T *p)
{
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL),
		p_fld(0, CKECTRL_CKEFIXOFF) |
		p_fld(1, CKECTRL_CKEFIXON));

	delay_us(1);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), 1,
		CATRAINING1_CATRAINCSEXT);

	/* Hold the CA bus stable for at least one cycle. */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), 1,
		CATRAINING1_CATRAINMRS);

	dramc_mode_reg_write(p, 42, 0xa8);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), 0,
		CATRAINING1_CATRAINMRS);

	/* CS extent disable */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_CATRAINING1), 0,
		CATRAINING1_CATRAINCSEXT);
}

static void cbt_delayscan_lp3(DRAMC_CTX_T *p, SCAN_WIN_DATA_T *sacn_ptr,
	signed short delay, unsigned int compare_result,
	unsigned char cal_bit4_9, unsigned int golden_pattern)
{
	unsigned char *lpddr_phy_mapping;
	unsigned short rst_rising, rst_falling;
	unsigned short pat_rising, pat_falling;
	unsigned char ca;
	unsigned char ca_idx;
	unsigned char fail;

	lpddr_phy_mapping = (unsigned char *)lpddr3_phy_mapping_pop;

	for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
		if (cal_bit4_9 && (ca != 4) && (ca != 9)) {
		#if CALIBRATION_LOG
			show_msg2((INFO, "-"));
		#endif
			continue;
		}

		if (!cal_bit4_9 && ((ca == 4) || (ca == 9))) {
		#if CALIBRATION_LOG
			show_msg2((INFO, "-"));
		#endif
			continue;
		}

		/* Get the result of each CA bit */
		switch (ca) {
		case 4:
			rst_rising = !!(compare_result &
				(1 << lpddr_phy_mapping[0]));
			rst_falling = !!(compare_result &
				(1 << lpddr_phy_mapping[1]));
			break;
		case 9:
			rst_rising = !!(compare_result &
				(1 << lpddr_phy_mapping[8]));
			rst_falling = !!(compare_result &
				(1 << lpddr_phy_mapping[9]));
			break;
		default:
			ca_idx = (ca < 4) ? (ca << 1) : ((ca - 1) << 1);

			rst_rising = !!(compare_result &
				(1 << lpddr_phy_mapping[ca_idx]));
			rst_falling = !!(compare_result
				& (1 << lpddr_phy_mapping[ca_idx+1]));
			break;
		}

		/* Value of golden pattern */
		pat_rising = !!(golden_pattern & (1 << (ca << 1)));
		pat_falling = !!(golden_pattern & (1 << ((ca << 1) + 1)));

		fail = 1;
		if ((pat_rising == rst_rising) &&
			(pat_falling == rst_falling)) {
			fail = 0;
		}

	#if CALIBRATION_LOG
		if (fail == 0)
			show_msg2((INFO, "o"));
		else
			show_msg2((INFO, "x"));
	#endif
		if (sacn_ptr->first_ca_pass[ca] == PASS_RANGE_NA) {
			if (fail == 0)
				sacn_ptr->first_ca_pass[ca] = delay;
		} else if (sacn_ptr->last_ca_pass[ca] == PASS_RANGE_NA) {
			if (fail == 1) {
				if ((delay - sacn_ptr->first_ca_pass[ca])
					< 8) {
					sacn_ptr->first_ca_pass[ca] =
						PASS_RANGE_NA;
					continue;
				}
				sacn_ptr->last_ca_pass[ca] = (delay - 1);
			} else if (delay == MAX_CA_PI_DELAY) {
				sacn_ptr->last_ca_pass[ca] = delay;
			}

			if (sacn_ptr->last_ca_pass[ca] != PASS_RANGE_NA) {
				sacn_ptr->finish_count++;
				sacn_ptr->ca_win_sum +=
					(sacn_ptr->last_ca_pass[ca] -
					sacn_ptr->first_ca_pass[ca]);
				sacn_ptr->ca_center[ca] =
					(sacn_ptr->last_ca_pass[ca] +
					sacn_ptr->first_ca_pass[ca]) >> 1;
				sacn_ptr->ca_center_sum +=
					sacn_ptr->ca_center[ca];
			}
		}
	}

#if CALIBRATION_LOG
	show_msg2((INFO, " [MSB]\n"));
#endif
}

static void _cbt_scan_lp3(DRAMC_CTX_T *p, FINAL_WIN_DATA_T *final_cbt_set_ptr,
	PASS_WIN_DATA_T final_win_per_ca[], SCAN_WIN_DATA_T *scan_result)
{
	signed int ca_final_center[CATRAINING_NUM_LP3] = { 0 };
	signed int ck_min = 1000;
	unsigned int ca;

	memset(ca_final_center, 0, sizeof(ca_final_center));

	/* cc notes: For High Freq, may need per-bit ajust. Currently
	 * not implemented since the highest freqency is 1866.
	 * Instead, using the average value.
	 */
	if (cbt_enable_perbit_adjust(p)) {
		ck_min = 1000;
		for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
			ca_final_center[ca] = scan_result->ca_center[ca];
			if (ca_final_center[ca] < ck_min)
				ck_min = ca_final_center[ca];
		}

		if (ck_min < 0) {
			show_msg((INFO,
				"Warning!! smallest%s%d < 0, %s !!\n",
				"CA min center = ", ck_min,
				"then adjust to 0"));
			for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
				if (ca_final_center[ca] < 0)
					ca_final_center[ca] = 0;
			}
			ck_min = 0;
		}

		final_cbt_set_ptr->final_ca_clk = ck_min;
	} else {
		final_cbt_set_ptr->final_ca_clk = scan_result->ca_center_sum
			/ CATRAINING_NUM_LP3;
	}

	for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
		final_win_per_ca[ca].first_pass =
			scan_result->first_ca_pass[ca];
		final_win_per_ca[ca].last_pass =
			scan_result->last_ca_pass[ca];
		final_win_per_ca[ca].win_center =
			scan_result->ca_center[ca];
		final_win_per_ca[ca].win_size =
			(scan_result->last_ca_pass[ca] -
			scan_result->first_ca_pass[ca]) +
			(scan_result->last_ca_pass[ca] ==
			scan_result->first_ca_pass[ca] ? 0 : 1);
	}
}

static void cbt_perbit_adjust_lp3(DRAMC_CTX_T *p,
	unsigned char delay_cell_ofst[])
{
	/* Set per-bit adjust result */
	if (cbt_enable_perbit_adjust(p)) {
		/* CA Pinmux:
		 * CA9->ARCA14
		 * CA8->ARCA6
		 * CA7->ARCA11
		 * CA6->ARCA4
		 * CA5->ARCA16
		 * CA4->ARCA19
		 * CA3->ARCA0
		 * CA2->ARCA15
		 * CA1->ARCA5
		 * CA0->ARCA17
		 */
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD0,
			p_fld(delay_cell_ofst[3],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA0_DLY) |
			p_fld(delay_cell_ofst[6],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA4_DLY) |
			p_fld(delay_cell_ofst[1],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA5_DLY) |
			p_fld(delay_cell_ofst[8],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA6_DLY));
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD1,
			p_fld(delay_cell_ofst[5],
				SHU1_R0_CA_CMD1_RK0_TX_ARCMD0_DLY) |
			p_fld(delay_cell_ofst[0],
				SHU1_R0_CA_CMD1_RK0_TX_ARCMD1_DLY));
		io_32_write_fld_multi(DDRPHY_SHU1_R0_CA_CMD0 +
				(1 << POS_BANK_NUM),
			p_fld(delay_cell_ofst[7],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA3_DLY) |
			p_fld(delay_cell_ofst[9],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA6_DLY) |
			p_fld(delay_cell_ofst[2],
				SHU1_R0_CA_CMD0_RK0_TX_ARCA7_DLY));
		io_32_write_fld_align(DDRPHY_SHU1_R0_CA_CMD1 +
				(1 << POS_BANK_NUM),
			delay_cell_ofst[4], SHU1_R0_CA_CMD1_RK0_TX_ARCMD0_DLY);
	}
}
static void cbt_scan_lp3(DRAMC_CTX_T *p,SCAN_WIN_DATA_T *scan_ptr,
	unsigned char cal_bit4_9, unsigned int ca_golden_pattern)
{
	signed int delay;
	unsigned int delay_step;
	unsigned int result;
	unsigned int finish_count;

#if FOR_DV_SIMULATION_USED
	delay_step = 4;
#else
	delay_step = 1;
#endif

	/*  SW variable initialization */
	finish_count = scan_ptr->finish_count;
	for (delay = -(MAX_CLK_PI_DELAY/2); delay < MAX_CA_PI_DELAY;
		delay += delay_step) {

		result = cbt_delay_compare_lp3(p, delay);

		show_msg2((INFO, "%2d, 0x%08x | ", delay, result));

		cbt_delayscan_lp3(p, scan_ptr, delay, result,
			cal_bit4_9, ca_golden_pattern);

		if ((cal_bit4_9 &&
			(scan_ptr->finish_count - finish_count == 2)) ||
			(!cal_bit4_9 &&
			(scan_ptr->finish_count - finish_count == 8))) {
			show_msg((INFO, "[CBT] Finish %d bits. Early break\n",
				scan_ptr->finish_count - finish_count));
			break;
		}
		delay_us(1); /* for tCACD consern */
	}
}

DRAM_STATUS_T cbt_lp3(DRAMC_CTX_T *p)
{
	PASS_WIN_DATA_T final_win_per_ca[CATRAINING_NUM_LP3];
	FINAL_WIN_DATA_T final_cbt_set;
	SCAN_WIN_DATA_T scan_cbt_win;
	unsigned int ca;
	unsigned char cal_bit4_9;
	unsigned int ca_golden_pattern;
	unsigned char delay_cell_ofst[CATRAINING_NUM_LP3];

	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL)),
		(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV)),
		(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM3)),
		(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM4)),
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL)),
	};

	show_msg3((INFO, "start CBT\n"));

/* need_check
	bakup_arpi_delay =
		io32_read_4b(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9));
*/

	memset(&final_win_per_ca, 0x0,
		sizeof(PASS_WIN_DATA_T) * CATRAINING_NUM_LP3);
	memset(&final_cbt_set, 0x0, sizeof(FINAL_WIN_DATA_T));
	memset(&scan_cbt_win, 0x0, sizeof(SCAN_WIN_DATA_T));
	ca_golden_pattern = CA_GOLDEN_PATTERN;

	for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
		scan_cbt_win.last_ca_pass[ca] = PASS_RANGE_NA;
		scan_cbt_win.first_ca_pass[ca] = PASS_RANGE_NA;
	}

	/* Back up dramC register */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0), 1,
		REFCTRL0_REFDIS);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM3), 0,
		SHU_ACTIM3_REFCNT);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM4), 0,
		SHU_ACTIM4_REFCNT_FR_CLK);

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), 1,
		DRAMC_PD_CTRL_MIOCKCTRLOFF);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), 0,
		DRAMC_PD_CTRL_PHYCLKDYNGEN);

	o1_path_on_off(p, 1);

	/*
	 * When doing CA training, should make sure that auto refresh is disable
	 */
	auto_refresh_switch(p, DISABLE);

	/* According to Spec., Shall first cal bits excluding bit4 and bit9,
	 * then change DQ mapping and cal bit4 and bit9.
	 */
	cal_bit4_9 = FALSE;
	cbt_enter_lp3(p, cal_bit4_9, ca_golden_pattern);
	cbt_scan_lp3(p, &scan_cbt_win, cal_bit4_9, ca_golden_pattern);

	cal_bit4_9 = TRUE;
	cbt_enter_lp3(p, cal_bit4_9, ca_golden_pattern);
	cbt_scan_lp3(p, &scan_cbt_win, cal_bit4_9, ca_golden_pattern);

	/* set CA/CK/CS pi delay to 0 */
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK) |
		p_fld(CLEAR_FLD, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS));

	_cbt_scan_lp3(p, &final_cbt_set,
		final_win_per_ca, &scan_cbt_win);

	for (ca = 0; ca < CATRAINING_NUM_LP3; ca++) {
		show_msg((INFO, "[CA %d] Center %d (%d~%d) winsize %d\n",
			ca, final_win_per_ca[ca].win_center,
			final_win_per_ca[ca].first_pass,
			final_win_per_ca[ca].last_pass,
			final_win_per_ca[ca].win_size));
#ifdef ETT
		ett_cbt_win[ca] = final_win_per_ca[ca].win_size;
#endif
	}

	if (cbt_enable_perbit_adjust(p)) {
		cbt_cal_delay_cell_perbit(p, final_cbt_set.final_ca_clk,
			delay_cell_ofst, final_win_per_ca);
	}
	/* Set CA_PI_Delay after trainging */
	if (final_cbt_set.final_ca_clk < 0) { /* Set CLK delay */
		ca_train_clk_delay[p->channel][p->rank] = 0;
		ca_train_cs_delay[p->channel][p->rank] = 0;
		show_err2("Warning!! Clk Dly = %d, adjust to 0 !!\n",
			-final_cbt_set.final_ca_clk);
		ca_train_cmd_delay[p->channel][p->rank] = 0;
	} else { /*  Set CA output delay */
		ca_train_clk_delay[p->channel][p->rank] = 0;
		ca_train_cs_delay[p->channel][p->rank] = 0;
		ca_train_cmd_delay[p->channel][p->rank] =
			final_cbt_set.final_ca_clk;
	}

	cbt_set_cbt_result_lp3(p);
	cbt_perbit_adjust_lp3(p, delay_cell_ofst);

	show_msg2((INFO, "Average CA Dly: %d\n", final_cbt_set.final_ca_clk));

	cbt_leave_lp3(p);
	o1_path_on_off(p, 0);

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL),
		p_fld(0, CKECTRL_CKEFIXOFF) |
		p_fld(0, CKECTRL_CKEFIXON));

	/* Restore setting registers */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	return DRAM_OK;
}
#endif /* SIMUILATION_LP3_CBT */

DRAM_STATUS_T cmd_bus_training(DRAMC_CTX_T *p)
{
	if (p->dram_type == TYPE_LPDDR4) {
	#if SUPPORT_TYPE_LPDDR4
		return cbt_lp4(p);
	#endif
	} else if (p->dram_type == TYPE_LPDDR3) {
	#if SUPPORT_TYPE_LPDDR3
		return cbt_lp3(p);
	#endif
	}

	return DRAM_OK;
}

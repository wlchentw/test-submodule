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
 * @file dramc_txcal_api.c
 *  Basic DRAMC calibration API implementation
 */

/* -----------------------------------------------------------------------------
 *  Include files
 * -----------------------------------------------------------------------------
 */
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>
#include <platform/dramc_api.h>

#define VALUE_2		2
#define VALUE_3		3
#define VALUE_64	64

#define WRITE_LEVELING_MOVD_DQS 1	/* UI */

/* -----------------------------------------------------------------------------
 *  Global variables
 * -----------------------------------------------------------------------------
 */
#if SIMULATION_TX_PERBIT
unsigned short tx_dq_pre_cal_lp4[DQS_NUMBER];
#endif
#if SIMULATION_WRITE_LEVELING
unsigned char wrlevel_done[CHANNEL_NUM] = { 0 };
#endif

#if SIMULATION_WRITE_LEVELING
/* dramc_write_leveling
 *  start Write Leveling Calibration.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @param  apply           (unsigned char): 0 don't apply the register
 *		we set  1 apply the register we set ,default don't apply.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
DRAM_STATUS_T execute_move_dramc_delay(DRAMC_CTX_T *p,
	REG_TRANSFER_T regs[], signed char shift_ui)
{
	signed int high_level_delay, delay_sum;
	unsigned int tmp0p5t, tmp2t;
	unsigned char data_rate_div_shift = 0;
	DRAM_STATUS_T move_result;

	if ((p->dram_type == TYPE_PCDDR3) || (p->dram_type == TYPE_LPDDR3))
		data_rate_div_shift = 2;
	else
	data_rate_div_shift = 3;

	tmp0p5t = io_32_read_fld_align(DRAMC_REG_ADDR(regs[0].addr),
		regs[0].fld) & (~(1 << data_rate_div_shift));
	tmp2t = io_32_read_fld_align(DRAMC_REG_ADDR(regs[1].addr),
		regs[1].fld);

	high_level_delay = (tmp2t << data_rate_div_shift) + tmp0p5t;
	delay_sum = (high_level_delay + shift_ui);

	if (delay_sum < 0) {
		tmp0p5t = 0;
		tmp2t = 0;
		move_result = DRAM_FAIL;
	} else {
		tmp2t = delay_sum >> data_rate_div_shift;
		tmp0p5t = delay_sum - (tmp2t << data_rate_div_shift);
		move_result = DRAM_OK;
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(regs[0].addr), tmp0p5t,
		regs[0].fld);
	io_32_write_fld_align(DRAMC_REG_ADDR(regs[1].addr), tmp2t,
		regs[1].fld);

	return move_result;
}

void move_dramc_tx_dqs(DRAMC_CTX_T *p, unsigned char byte_idx,
	signed char shift_ui)
{
	REG_TRANSFER_T transfer_reg[2];

	switch (byte_idx) {
	case 0:
		/*  DQS0 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_DQS0;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_DQS0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 1:
		/*  DQS1 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_DQS1;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_DQS1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 2:
		/*  DQS2 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_DQS2;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_DQS2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 3:
		/*  DQS3 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_DQS3;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_DQS3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	default:
		break;
	}
}

void move_dramc_tx_dqs_oen(DRAMC_CTX_T *p, unsigned char byte_idx,
	signed char shift_ui)
{
	REG_TRANSFER_T transfer_reg[2];

	switch (byte_idx) {
	case 0:
		/* DQS_OEN_0 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_OEN_DQS0;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_OEN_DQS0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 1:
		/* DQS_OEN_1 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_OEN_DQS1;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_OEN_DQS1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 2:
		/* DQS_OEN_2 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_OEN_DQS2;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_OEN_DQS2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 3:
		/* DQS_OEN_3 */
		transfer_reg[0].addr = DRAMC_REG_SHU_SELPH_DQS1;
		transfer_reg[0].fld = SHU_SELPH_DQS1_DLY_OEN_DQS3;
		transfer_reg[1].addr = DRAMC_REG_SHU_SELPH_DQS0;
		transfer_reg[1].fld = SHU_SELPH_DQS0_TXDLY_OEN_DQS3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	default:
		break;
	}
}

void move_dramc_tx_dq(DRAMC_CTX_T *p, unsigned char byte_idx,
	signed char shift_ui)
{
	REG_TRANSFER_T transfer_reg[2];

	switch (byte_idx) {
	case 0:
		/* DQM0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);

		/* DQ0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 1:
		/* DQM1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/* DQ1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 2:
		/* DQM2 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM2;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/* DQ2 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ2;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 3:
		/* DQM3 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_DQM3;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_DQM3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/* DQ3 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_DQ3;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_DQ3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;
	}
}

void move_dramc_tx_dq_oen(DRAMC_CTX_T *p, unsigned char byte_idx,
	signed char shift_ui)
{
	REG_TRANSFER_T transfer_reg[2];

	switch (byte_idx) {
	case 0:
		/*  DQM_OEN_0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_OEN_DQM0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/*  DQ_OEN_0 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_OEN_DQ0;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 1:
		/*  DQM_OEN_1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_OEN_DQM1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/*  DQ_OEN_1 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_OEN_DQ1;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 2:
		/*  DQM_OEN_2 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_OEN_DQM2;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/*  DQ_OEN_2 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_OEN_DQ2;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;

	case 3:
		/*  DQM_OEN_3 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ3;
		transfer_reg[0].fld = SHURK0_SELPH_DQ3_DLY_OEN_DQM3;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ1;
		transfer_reg[1].fld = SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		/*  DQ_OEN_3 */
		transfer_reg[0].addr = DRAMC_REG_SHURK0_SELPH_DQ2;
		transfer_reg[0].fld = SHURK0_SELPH_DQ2_DLY_OEN_DQ3;
		transfer_reg[1].addr = DRAMC_REG_SHURK0_SELPH_DQ0;
		transfer_reg[1].fld = SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3;
		execute_move_dramc_delay(p, transfer_reg, shift_ui);
		break;
	}
}

void write_leveling_move_dqs_instead_of_clk(DRAMC_CTX_T *p)
{
	unsigned char byte_idx;
	unsigned char backup_rank, ii;

	backup_rank = get_rank(p);

	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
	     byte_idx++) {
		move_dramc_tx_dqs(p, byte_idx, -WRITE_LEVELING_MOVD_DQS);
		move_dramc_tx_dqs_oen(p, byte_idx, -WRITE_LEVELING_MOVD_DQS);

		for (ii = RANK_0; ii < p->support_rank_num; ii++) {
			set_rank(p, ii);
			move_dramc_tx_dq(p, byte_idx,
				-WRITE_LEVELING_MOVD_DQS);
			move_dramc_tx_dq_oen(p, byte_idx,
				-WRITE_LEVELING_MOVD_DQS);
		}
		set_rank(p, backup_rank);
	}
}

void set_dram_mr_write_leveling_on_off(DRAMC_CTX_T *p, unsigned char on_off)
{
	unsigned int mr_val = 0;
	unsigned int mr_idx = 0;

	if (p->dram_type == TYPE_LPDDR4 ||
		p->dram_type == TYPE_LPDDR3) {
		mr_idx = 0x2;
		mr_val = dram_mr.mr02_value[p->dram_fsp];
	} else if (p->dram_type == TYPE_PCDDR4 ||
		p->dram_type == TYPE_PCDDR3) {
		mr_idx = 0x1;
		mr_val = dram_mr.mr01_value[p->dram_fsp];
	}

	if (on_off)
		mr_val |= (1 << 7);
	else
		mr_val &= ~(1 << 7);

	dramc_mode_reg_write(p, mr_idx, mr_val);
}

#define WL_CNT_16	0xfc
#define WL_CNT		0xf0

static void dramc_wl_lp4_init(DRAMC_CTX_T *p)
{
	unsigned int value;

	auto_refresh_switch(p, DISABLE);

	if (p->arfg_write_leveling_init_shif[p->channel][p->rank] == FALSE) {
		write_leveling_move_dqs_instead_of_clk(p);
		p->arfg_write_leveling_init_shif[p->channel][RANK_0] = TRUE;
		p->arfg_write_leveling_init_shif[p->channel][RANK_1] = TRUE;
		p->fg_tx_perbif_init[p->channel][RANK_0] = FALSE;
		p->fg_tx_perbif_init[p->channel][RANK_1] = FALSE;

		show_msg2((INFO, "WriteLevelingMoveDQSInsteadOfCLK\n"));
	}

	/* write leveling mode initialization */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		SET_FLD, DRAMC_PD_CTRL_MIOCKCTRLOFF);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL),
		CLEAR_FLD, DRAMC_PD_CTRL_PHYCLKDYNGEN);

	/*
	* Make CKE fixed at 1
	* (Don't enter power down, Put this before issuing MRS)
	*/
	cke_fix_on_off(p, CKE_FIXON, CKE_WRITE_TO_ONE_CHANNEL);

	/* PHY RX Setting for Write Leveling */
	o1_path_on_off(p, ENABLE);

	/* Disable WODT in case it's asserted while issuing MRW */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
		SET_FLD, SHU1_WODT_WODTFIXOFF);

	/* issue MR2[7] to enable write leveling */
	set_dram_mr_write_leveling_on_off(p, ENABLE);

	/* wait tWLDQSEN (25 nCK / 25ns) after enabling write leveling mode */
	delay_us(1);

	/* Enable Write leveling */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
		p_fld(SET_FLD, WRITE_LEV_DQS_B3_G) |
		p_fld(CLEAR_FLD, WRITE_LEV_DQS_B2_G) |
		p_fld(SET_FLD, WRITE_LEV_DQS_B1_G) |
		p_fld(CLEAR_FLD, WRITE_LEV_DQS_B0_G));

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV), SET_FLD,
		WRITE_LEV_WRITE_LEVEL_EN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV), SET_FLD,
		WRITE_LEV_CBTMASKDQSOE);

	/* select DQS */
	if (p->data_width == DATA_WIDTH_32BIT) {
		value = 0xf;
	} else {
		value = 0x3;
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV), value,
		WRITE_LEV_DQS_SEL);

	/* wait tWLMRD (40 nCL / 40 ns) before DQS pulse */
	delay_us(1);

	/* Set DQS output delay to 0 */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7),
		CLEAR_FLD, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0);
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7),
		CLEAR_FLD, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1);
}


static void adjust_clk_dqs_output_delay(DRAMC_CTX_T *p, signed int ii)
{
	if (ii <= 0) {
		/* Adjust Clk output delay. */
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_CA_CMD9), -ii,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK);
	} else {
		/* Adjust DQS output delay. */
		/* PI (TX DQ/DQS adjust at the same time) */
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B0_DQ7), ii,
			SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B1_DQ7), ii,
			SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1);
	}
}

static unsigned char wl_check_lp4(DRAMC_CTX_T *p, signed int begin,
	signed int end, unsigned char step, unsigned char sample_count)
{
	signed int ii;
	unsigned char i;
	unsigned int dq_o1 = 0;
	unsigned char sample_status[DQS_NUMBER], dq_o1_perbyte[DQS_NUMBER],
		dq_o1_index[DQS_NUMBER];

	/* DQ mapping */
	for (i = 0; i < (p->data_width / DQS_BIT_NUM); i++)
		dq_o1_index[i] = i * 8;

	/* Initialize sw parameters */
	for (ii = 0; ii < (signed int) (p->data_width / DQS_BIT_NUM); ii++) {
		sample_status[ii] = 0;
		wl_final_delay[p->channel][ii] = 0;
	}

	for (ii = begin; ii < end; ii += step) {
		adjust_clk_dqs_output_delay(p, ii);

		/* Trigger DQS pulse */
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			SET_FLD, WRITE_LEV_DQS_WLEV);
		io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV),
			CLEAR_FLD, WRITE_LEV_DQS_WLEV);

		/* wait tWLO (7.5ns / 20ns) before output (DDR3 / LPDDR3) */
		delay_us(1);

		/* Read DQ_O1 from register */
		dq_o1 = io32_read_4b(DRAMC_REG_ADDR(DDRPHY_MISC_DQO1));

		if (p->channel == CHANNEL_B) {
			/* If B23 is used?? */
			dq_o1 >>= 16;
		} else if (p->data_width == DATA_WIDTH_32BIT) {
			unsigned int tmp;

			tmp = io32_read_4b(DRAMC_REG_ADDR(DDRPHY_MISC_DQO1) +
				(1 << POS_BANK_NUM));
			dq_o1 = (dq_o1 & 0xffff) | (tmp & 0xffff0000);
		}

		/* Since the O1 result is located in DDRPHY side,
		 * if Swap between bytes, shall make the bits that driven
		 * by the same DQS appear in 1byte.
		 * PINMUX. REVIEW
		 */
		if (p->en_4bit_mux) {
			unsigned char * lpddr_4bitmux_byte_mapping;
			unsigned char bit_idx;
			unsigned int dq_o1_tmp = 0;

			show_msg3((INFO,
				"WL 4bit pinmux. Read dq_o1 = 0x%x\n", dq_o1));

			lpddr_4bitmux_byte_mapping =
				dramc_get_4bitmux_byte_mapping(p);
			if (lpddr_4bitmux_byte_mapping != NULL) {
				for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
					dq_o1_tmp |= ((dq_o1 >> bit_idx) & 0x1) <<
						lpddr_4bitmux_byte_mapping[bit_idx];
				}

				dq_o1 = dq_o1_tmp;
			}
			show_msg3((INFO,
				"WL 4bit pinmux. Processed dq_o1 = 0x%x\n", dq_o1));
		}
		show_msg3((INFO, "%d    ", ii));

		for (i = 0; i < (p->data_width / DQS_BIT_NUM); i++) {
			dq_o1_perbyte[i] = (unsigned char)
				((dq_o1 >> dq_o1_index[i]) & 0xff);

			show_msg3((INFO, "%x   ", dq_o1_perbyte[i]));

			if ((sample_status[i] == 0) && (dq_o1_perbyte[i] == 0))
				sample_status[i] = 1;
			else if ((sample_status[i] >= 1) &&
				(dq_o1_perbyte[i] == 0))
				sample_status[i] = 1;
			else if ((sample_status[i] >= 1) &&
				(dq_o1_perbyte[i] != 0))
				sample_status[i]++;

			/* result not found of byte yet */
			if (((sample_count & (0x01 << i)) == 0)
				&& ((sample_status[i] == 8) ||
				((ii == end - 1) && (sample_status[i] > 1)))) {
				wl_final_delay[p->channel][i] =
					ii - (sample_status[i] - 2)*step;
				sample_count |= (0x01 << i);
			}

		}

		show_msg3((INFO, "\n"));

#if !DQS_DUTY_MEASURE_WITH_WRITE_LEVELING
		if (sample_count == BYTE_MAX)
			break;	/*  all byte found, early break. */
#endif
	}
	show_msg2((INFO, "pass bytecount = 0x%x (0xff: all bytes pass)\n",
		sample_count));
	return sample_count;
}

static void wl_adjust_clk_ca_lp4(DRAMC_CTX_T *p)
{
	signed int clock_delay_max;
	unsigned int value;
	signed int diff;
	unsigned char i;

	/* Initialize sw parameters */
	clock_delay_max = MAX_TX_DQSDLY_TAPS;

	for (i = 0; i < (p->data_width / DQS_BIT_NUM); i++) {
		if (clock_delay_max >
			wl_final_delay[p->channel][i]) {
			clock_delay_max = wl_final_delay[p->channel][i];
		}
	}

	if (clock_delay_max > 0)
		clock_delay_max = 0;
	else
		clock_delay_max = -clock_delay_max;

	show_msg((INFO, "WL Clk dly = %d, CA clk dly = %d\n",
		clock_delay_max, ca_train_clk_delay[p->channel][p->rank]));

	/* Adjust Clk & CA if needed */
	/* cc notes: since SW will only adjust DQS delay for WL,
	 * clock_delay_max will always be 0. So even for non-lp4 type,
	 * the following code will execute 'else' branch. Seems OK.
	 */
	if (ca_train_clk_delay[p->channel][p->rank] < clock_delay_max) {
		diff = clock_delay_max -
			ca_train_clk_delay[p->channel][p->rank];
		show_msg((INFO, "CA adjust %d taps\n", diff));

		/* Write shift value into CA output delay. */
		value = ca_train_cmd_delay[p->channel][p->rank];
		value += diff;
		io_32_write_fld_align(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_CA_CMD9), value,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD);

		show_msg((INFO,
			"Update CA PI Dly Macro0 = %dn", value));

		/* Write shift value into CS output delay. */
		value = ca_train_cs_delay[p->channel][p->rank];
		value += diff;
		io_32_write_fld_align(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_CA_CMD9), value,
			SHU1_R0_CA_CMD9_RG_RK0_ARPI_CS);
		show_msg((INFO, "Update CS Dly = %d\n", value));
	} else {
		show_msg((INFO, "No need to update CA/CS dly %s",
			"(CLK dly smaller than CA training)\n"));
		clock_delay_max = ca_train_clk_delay[p->channel][p->rank];
	}

	/* Write max center value into Clk output delay. */
	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9),
		clock_delay_max, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK);

	show_msg((INFO, "Final Clk output dly = %d\n", clock_delay_max));

	for (i = 0; i < (p->data_width / DQS_BIT_NUM); i++) {
		wl_final_delay[p->channel][i] += clock_delay_max;
		show_msg((INFO, "[Write Leveling]DQS%d dly: %d\n", i,
			wl_final_delay[p->channel][i]));
	}
}

static void wl_set_values_lp4(DRAMC_CTX_T *p)
{
	signed int wrlevel_dq_delay[DQS_NUMBER] = {0, 0, 0, 0};
	unsigned char i;

	/* Process the result to better compatible with
	 * 32Bit and 16Bit data width
	 */
	if (p->channel == CHANNEL_B) {
		/* If B23 is used, the result is stored in
		 * [0] and [1] while the result shall be
		 * written to B23, so set the result accordingly.
		 * Be noticed that although PHY B2->DRAMC B1,
		 * but WL result is obtained via O1 path,
		 * thus no need to switch PI result.
		 */
		wl_final_delay[p->channel][2] =
			wl_final_delay[p->channel][0];
		wl_final_delay[p->channel][3] =
			wl_final_delay[p->channel][1];
	}

	/* set to best values for  DQS */
	io_32_write_fld_align(DDRPHY_SHU1_R0_B0_DQ7,
		wl_final_delay[p->channel][0],
		SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0);
	io_32_write_fld_align(DDRPHY_SHU1_R0_B1_DQ7,
		wl_final_delay[p->channel][1],
		SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1);
	io_32_write_fld_align(DDRPHY_SHU1_R0_B0_DQ7 +
		(CHANNEL_B << POS_BANK_NUM),
		wl_final_delay[p->channel][2],
		SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0);
	io_32_write_fld_align(DDRPHY_SHU1_R0_B1_DQ7 +
		(CHANNEL_B << POS_BANK_NUM),
		wl_final_delay[p->channel][3],
		SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1);

	for (i = 0; i < (p->data_width / DQS_BIT_NUM); i++) {
		unsigned char dqs_map;

		if (i==2)
			wrlevel_dq_delay[i] = wl_final_delay[p->channel][i] + 0x16;
		else
			wrlevel_dq_delay[i] = wl_final_delay[p->channel][i] + 0x12;

		/* ARPI_DQ_B* is 6 bits, max 0x40 */
		if (wrlevel_dq_delay[i] >= 0x40) {
			dqs_map = i;

			/* If CHB is used, only process B23 */
			if (p->channel == CHANNEL_B) {

				/* PHY B2 is mapped to DRAMC B1, so
				 * dramc delay (2T, 0.5T) shall be set
				 * to B1. Same for PHY B3.
				 */
				dqs_map = (i == 0) ? 1 : 0;
			}

			wrlevel_dq_delay[i] -= 0x40;
			move_dramc_tx_dq(p, dqs_map, 2);
			move_dramc_tx_dq_oen(p, dqs_map, 2);
		}
	}

	/* Same as DQS PI, DQ PI shall also be set to PHY B23
	 * if neccessary
	 */
	if (p->channel == CHANNEL_B) {
		wrlevel_dq_delay[2] = wrlevel_dq_delay[0];
		wrlevel_dq_delay[3] = wrlevel_dq_delay[1];
	}

	/* set to best values for  DQM, DQ */
	io_32_write_fld_multi(DDRPHY_SHU1_R0_B0_DQ7,
		p_fld(wrlevel_dq_delay[0], SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(wrlevel_dq_delay[0], SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));

	io_32_write_fld_multi(DDRPHY_SHU1_R0_B1_DQ7,
		p_fld(wrlevel_dq_delay[1], SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(wrlevel_dq_delay[1], SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));

	io_32_write_fld_multi(DDRPHY_SHU1_R0_B0_DQ7 +
		(CHANNEL_B << POS_BANK_NUM),
		p_fld(wrlevel_dq_delay[2], SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0) |
		p_fld(wrlevel_dq_delay[2], SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));

	io_32_write_fld_multi(DDRPHY_SHU1_R0_B1_DQ7+
		(CHANNEL_B << POS_BANK_NUM),
		p_fld(wrlevel_dq_delay[3], SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1) |
		p_fld(wrlevel_dq_delay[3], SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
}

DRAM_STATUS_T dramc_write_leveling(DRAMC_CTX_T *p)
{
	/* Note that below procedure is based on "ODT off" */
	DRAM_STATUS_T result = DRAM_FAIL;
	unsigned char sample_count;
	signed int begin, end;
	unsigned char step;

	show_msg((INFO, "\n[DramcWriteLeveling]\n"));
	print_calibration_basic_info(p);

	/* error handling */
	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}

	unsigned int reg_backup_address[] = {
		(DRAMC_REG_ADDR(DRAMC_REG_REFCTRL0)),
		(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL)),
		(DRAMC_REG_ADDR(DRAMC_REG_WRITE_LEV)),
		(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL)),
	};

	wrlevel_done[p->channel] = 0;

	dramc_rank_swap(p, p->rank);

	/*  backup mode settings */
	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/* Proceed write leveling... */
	dramc_wl_lp4_init(p);

	/*
	* used for WL done status
	* each bit of sample_cnt represents one-byte WL status
	* 1: done or N/A. 0: NOK
	*/
	if (p->data_width == DATA_WIDTH_16BIT)
		sample_count = WL_CNT_16;
	else
		sample_count = WL_CNT;

	show_msg3((INFO, "\n[Write Leveling]\n"));
	show_msg3((INFO, "delay  byte0  byte1\n"));

	begin = WRITE_LEVELING_MOVD_DQS * 32 - MAX_CLK_PI_DELAY - 1;
	begin = begin + WL_OFFSET;
	end = begin + WL_RANGE;
#if (FOR_DV_SIMULATION_USED == 1)
	step = 2;
#else
	step = WL_STEP;
#endif

	sample_count = wl_check_lp4(p, begin, end, step, sample_count);

	if (sample_count == BYTE_MAX) { /* all bytes are done */
		wrlevel_done[p->channel] = 1;
		result = DRAM_OK;
	} else {
		result = DRAM_FAIL;
	}
	set_calibration_result(p, DRAM_CALIBRATION_WRITE_LEVEL, result);

	wl_adjust_clk_ca_lp4(p);

	/*
	* write leveling done, mode settings recovery if necessary
	* recover mode registers : issue MR2[7] to disable write leveling
	*/
	set_dram_mr_write_leveling_on_off(p, DISABLE);

	/*  restore registers. */
	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	/*  Release WODTFIXOFF, re-enable WODT */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SHU1_WODT),
		CLEAR_FLD, SHU1_WODT_WODTFIXOFF);

	/* Disable DQ_O1, SELO1ASO=0 for power saving */
	o1_path_on_off(p, 0);

	wl_set_values_lp4(p);

	dramc_rank_swap(p, RANK_0);

	show_msg((INFO, "[DramcWriteLeveling] Done\n\n"));

	return result;
}
#endif /* SIMULATION_WRITE_LEVELING */

#if SIMULATION_TX_PERBIT
/* dramc_tx_window_perbit_cal (v2)
 *  TX DQS per bit SW calibration.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @param  apply           (unsigned char): 0 don't apply the register we set
 *			1 apply the register we set ,default don't apply.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
#define ENABLE_64_PI_TO_UI 1
#if ENABLE_64_PI_TO_UI /* 1 PI = tCK/64, total 128 PI, 1UI = 64 PI */
#define TX_DQ_UI_TO_PI_TAP         64
#else /* 1 PI = tCK/64, total 128 PI, 1UI = 32 PI */
#define TX_DQ_UI_TO_PI_TAP         32
#endif

#ifdef ETT
unsigned char tx_eye_scan_flag = TRUE;
#else
unsigned char tx_eye_scan_flag = FALSE;
#endif

static void tx_win_transfer_delay_to_uipi(DRAMC_CTX_T *p, unsigned short delay,
	unsigned char adjust_pi_to_center, unsigned char *ui_large_dq,
	unsigned char *ui_small_dq, unsigned char *u_pi,
	unsigned char *pu1_u1_large_dqoe, unsigned char *pu1_u1_small_dqoe)
{
	unsigned char small_ui_to_large, pi;
	unsigned short tmp_value;

	/* in LP4, 8 small UI =  1 large UI */
	if ((p->dram_type == TYPE_PCDDR3) || (p->dram_type == TYPE_LPDDR3))
		small_ui_to_large = 2;
	else
	small_ui_to_large = 3;

	if (u_pi != NULL) {
		pi = delay & (TX_DQ_UI_TO_PI_TAP - 1);
		*u_pi = pi;
	}

	tmp_value = (delay / TX_DQ_UI_TO_PI_TAP) << 1;

	if (adjust_pi_to_center && (u_pi != NULL)) {
		if (pi < 10) {
			pi += (TX_DQ_UI_TO_PI_TAP) >> 1;
			tmp_value--;
		} else if (pi > TX_DQ_UI_TO_PI_TAP - 10) {
			pi -= (TX_DQ_UI_TO_PI_TAP) >> 1;
			tmp_value++;
		}

		*u_pi = pi;
	}

	*ui_small_dq = tmp_value -
		((tmp_value >> small_ui_to_large) << small_ui_to_large);
	*ui_large_dq = (tmp_value >> small_ui_to_large);

	/* calculate DQ OE according to DQ UI */
	tmp_value -= TX_DQ_OE_SHIFT_LP4;

	*pu1_u1_small_dqoe = tmp_value -
		((tmp_value >> small_ui_to_large) << small_ui_to_large);
	*pu1_u1_large_dqoe = (tmp_value >> small_ui_to_large);
}

void dramc_tx_set_vref(DRAMC_CTX_T *p, unsigned char vref_range,
	unsigned char vref_value)
{
	unsigned char temp_op_value = ((vref_value & 0x3f) | (vref_range << 6));

#if SUPPORT_TYPE_PCDDR4
	dram_mr.mr06_value[p->dram_fsp] |= temp_op_value;
	dramc_mode_reg_write(p, MR06, dram_mr.mr06_value[p->dram_fsp]);
	dramc_mode_reg_write(p, MR06, dram_mr.mr06_value[p->dram_fsp] | 0x80);
	dramc_mode_reg_write(p, MR06, dram_mr.mr06_value[p->dram_fsp] | 0x00);
#else
	dram_mr.mr14_value[p->channel][p->rank][p->dram_fsp] = temp_op_value;
	dramc_mode_reg_write_by_rank(p, p->rank, MR14, temp_op_value);
#endif
}

static unsigned short find_smallest_dq_byte_delay(DRAMC_CTX_T *p)
{
	unsigned char byte_idx;
	unsigned int reg_value_txdly, reg_value_dly;
	unsigned short temp_virtual_delay, smallest_virtual_delay = WORD_MAX;
	unsigned char dq_ui_large_bak[DQS_NUMBER], dq_ui_small_bak[DQS_NUMBER];

	reg_value_txdly =
		io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ0));
	reg_value_dly =
		io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ2));

	/*  find smallest DQ byte delay */
	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
	     byte_idx++) {
		dq_ui_large_bak[byte_idx] =
			(reg_value_txdly >> (byte_idx * 4)) & 0x7;
		dq_ui_small_bak[byte_idx] =
			(reg_value_dly >> (byte_idx * 4)) & 0x7;

		temp_virtual_delay = (dq_ui_large_bak[byte_idx] << 3)
			+ dq_ui_small_bak[byte_idx];
		if (temp_virtual_delay < smallest_virtual_delay)
			smallest_virtual_delay = temp_virtual_delay;

		show_msg((INFO, "Original DQ_B%d (%d %d) =%d, OEN = %d\n",
			byte_idx, dq_ui_large_bak[byte_idx],
			dq_ui_small_bak[byte_idx],
			temp_virtual_delay,
			temp_virtual_delay - TX_DQ_OE_SHIFT_LP4));
	}
	     return smallest_virtual_delay;
}

static void set_dq_oen_rg(DRAMC_CTX_T *p, unsigned char dq_oen_large[],
	unsigned char dq_oen_small[])
{
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ0),
		p_fld(dq_oen_large[0], SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
		p_fld(dq_oen_large[1], SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
		p_fld(dq_oen_large[2], SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
		p_fld(dq_oen_large[3], SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3));

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ1),
		p_fld(dq_oen_large[0], SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
		p_fld(dq_oen_large[1], SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
		p_fld(dq_oen_large[2], SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
		p_fld(dq_oen_large[3], SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3));

	/* DLY_DQ[2:0] */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ2),
		p_fld(dq_oen_small[0], SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
		p_fld(dq_oen_small[1], SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
		p_fld(dq_oen_small[2], SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
		p_fld(dq_oen_small[3], SHURK0_SELPH_DQ2_DLY_OEN_DQ3));

	/* DLY_DQM[2:0] */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ3),
		p_fld(dq_oen_small[0], SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(dq_oen_small[1], SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(dq_oen_small[2], SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
		p_fld(dq_oen_small[3], SHURK0_SELPH_DQ3_DLY_OEN_DQM0));
}

void dramc_tx_oe_calibration(DRAMC_CTX_T *p)
{
	unsigned char byte_idx, begin[DQS_NUMBER] = { 0 },
		end[DQS_NUMBER] = {BYTE_MAX, BYTE_MAX, BYTE_MAX, BYTE_MAX},
		best_step[DQS_NUMBER];
	unsigned int err_value;
	unsigned short delay, smallest_virtual_delay = WORD_MAX;
	unsigned short dqoen_delay_begin, dqoen_delay_end;
	unsigned char dq_oen_ui_large[DQS_NUMBER], dq_oen_ui_small[DQS_NUMBER];

	show_msg3((INFO, "\n[DramC_TX_OE_Calibration] TA2\n"));

	smallest_virtual_delay = find_smallest_dq_byte_delay(p);

	dramc_engine2_init(p, p->test2_1, p->test2_2, TEST_AUDIO_PATTERN, 0);

	if (smallest_virtual_delay >= 7)
		dqoen_delay_begin = smallest_virtual_delay - 7;
	else
		dqoen_delay_begin = 0;

	dqoen_delay_end = dqoen_delay_begin + 10;

	for (delay = dqoen_delay_begin; delay <= dqoen_delay_end; delay++) {
		for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
			byte_idx++) {
			unsigned char mck2ui, msk;

			if ((p->dram_type == TYPE_PCDDR3) ||
				(p->dram_type == TYPE_LPDDR3)) {
				mck2ui = 2;
				msk = 3;
			} else {
				mck2ui = 3;
				msk = 7;
			}

			dq_oen_ui_large[byte_idx] = (delay >> mck2ui);
			dq_oen_ui_small[byte_idx] = delay & msk;
		}

		set_dq_oen_rg(p, dq_oen_ui_large, dq_oen_ui_small);

		err_value = dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK,
			TEST_AUDIO_PATTERN);

		/* 3 */
		for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
			byte_idx++) {
			if (((err_value >> (byte_idx << 3)) & 0xff) == 0) {
				if (begin[byte_idx] == 0)
					begin[byte_idx] = 1;
				end[byte_idx] = delay;
			}
		}

		show_msg2((INFO,
			"TAP=%2d, err_value=0x%8x, End_B0=%d End_B1=%d\n",
			delay, err_value, end[0], end[1]));

		if (((err_value & 0xffff) != 0) && (begin[0] == 1) &&
			(begin[1] == 1))
			break;	/* early break; */
	}

	dramc_engine2_end(p);

	/* 4 */
	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
		byte_idx++) {
		if (end[byte_idx] == BYTE_MAX) {
			best_step[byte_idx] =
				smallest_virtual_delay - TX_DQ_OE_SHIFT_LP4;
			show_err2("Byte %d no TX OE taps pass", byte_idx);
			show_err(", calibration fail!\n");
		} else { /* window is larger htan 3 */
			best_step[byte_idx] = end[byte_idx] - 3;
		}
		show_msg((INFO, "Byte%d end_step=%d  best_step=%d ",
			byte_idx, end[byte_idx],
			best_step[byte_idx]));

		dq_oen_ui_large[byte_idx] = (best_step[byte_idx] >> 3);
		dq_oen_ui_small[byte_idx] = best_step[byte_idx] & 0x7;
		show_msg((INFO, "Final TX OE(2T, 0.5T) = (%d, %d)\n",
			dq_oen_ui_large[byte_idx],
			dq_oen_ui_small[byte_idx]));
	}

	show_msg3((INFO, "\n"));

#if 0
	if (p->channel == CHANNEL_B) {
		unsigned char dqs_i;

		for (dqs_i = 2; dqs_i < DQS_NUMBER; dqs_i++) {
			unsigned char dqs_map;

			dqs_map = (dqs_i == 2) ? 1 : 0;

			dq_oen_ui_large[dqs_i] =
				dq_oen_ui_large[dqs_map];
			dq_oen_ui_small[dqs_i] =
				dq_oen_ui_small[dqs_map];
		}
	}
#endif

	set_dq_oen_rg(p, dq_oen_ui_large, dq_oen_ui_small);
}

static unsigned short find_smallest_dqs_delay(DRAMC_CTX_T *p)
{
	unsigned int reg_value_txdly, reg_value_dly;
	unsigned char byte_idx;
	unsigned char dq_ui_large_bak[DQS_NUMBER], dq_ui_small_bak[DQS_NUMBER];
	unsigned short temp_virtual_delay, smallest_virtual_delay = WORD_MAX;
	unsigned char mck2ui;

	reg_value_txdly =
		io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_SHU_SELPH_DQS0));
	reg_value_dly =
		io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_SHU_SELPH_DQS1));

	if ((p->dram_type == TYPE_PCDDR3) || (p->dram_type == TYPE_LPDDR3))
		mck2ui = VALUE_2;
	else
	mck2ui = VALUE_3;

	/*  find smallest DQS delay */
	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
	     byte_idx++) {
		dq_ui_large_bak[byte_idx] =
			(reg_value_txdly >> (byte_idx << 2)) & 0x7;
		dq_ui_small_bak[byte_idx] =
			(reg_value_dly >> (byte_idx << 2)) & 0x7;

		/* LP4 : Virtual Delay = 256 * MCK + 32*UI + PI; */
		temp_virtual_delay =
		    (((dq_ui_large_bak[byte_idx] << mck2ui) +
		      dq_ui_small_bak[byte_idx]) << 5) +
		    wl_final_delay[p->channel][byte_idx];

		if (temp_virtual_delay < smallest_virtual_delay)
			smallest_virtual_delay = temp_virtual_delay;
	}
	return smallest_virtual_delay;
}

static void set_txdq_vref_lp4(DRAMC_CTX_T *p, unsigned short vref_range,
	unsigned short vref_level)
{
		/*  SET tx Vref (DQ) here */
#if SUPPORT_TYPE_PCDDR4
		dram_mr.mr06_value[p->dram_fsp] |=
			(vref_level | (vref_range << 6)) ;
		dramc_mode_reg_write(p, MR06,
			dram_mr.mr06_value[p->dram_fsp]);
		dramc_mode_reg_write(p, MR06,
			dram_mr.mr06_value[p->dram_fsp] | 0x80);
		dramc_mode_reg_write(p, MR06,
			dram_mr.mr06_value[p->dram_fsp] | 0x00);
		show_msg((INFO,"MR6 =0x%x \n",dram_mr.mr06_value[p->dram_fsp]));
#else
		dram_mr.mr14_value[p->channel][p->rank][p->dram_fsp] =
			(vref_level | (vref_range << 6));
		dramc_mode_reg_write_by_rank(p, p->rank, MR14,
			vref_level | (vref_range << 6));
#endif
}

static void set_txdq_delay_lp4(DRAMC_CTX_T *p, unsigned char delay1,
	unsigned char delay2, unsigned char delay3, unsigned char delay4,
	unsigned char type)
{
	if (type == 0) { /* TXDLY_DQ , TXDLY_OEN_DQ */
		io_32_write_fld_multi(
			DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ0),
			p_fld(delay1, SHURK0_SELPH_DQ0_TXDLY_DQ0)
			| p_fld(delay1, SHURK0_SELPH_DQ0_TXDLY_DQ1)
			| p_fld(delay1, SHURK0_SELPH_DQ0_TXDLY_DQ2)
			| p_fld(delay1, SHURK0_SELPH_DQ0_TXDLY_DQ3)
			| p_fld(delay2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0)
			| p_fld(delay2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1)
			| p_fld(delay2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2)
			| p_fld(delay2, SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3));

		/* DLY_DQ[2:0] */
		io_32_write_fld_multi(
		DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ2),
			p_fld(delay3, SHURK0_SELPH_DQ2_DLY_DQ0)
			| p_fld(delay3, SHURK0_SELPH_DQ2_DLY_DQ1)
			| p_fld(delay3, SHURK0_SELPH_DQ2_DLY_DQ2)
			| p_fld(delay3, SHURK0_SELPH_DQ2_DLY_DQ3)
			| p_fld(delay4, SHURK0_SELPH_DQ2_DLY_OEN_DQ0)
			| p_fld(delay4, SHURK0_SELPH_DQ2_DLY_OEN_DQ1)
			| p_fld(delay4, SHURK0_SELPH_DQ2_DLY_OEN_DQ2)
			| p_fld(delay4, SHURK0_SELPH_DQ2_DLY_OEN_DQ3));
	} else if (type == 1) { /* TXDLY_DQM , TXDLY_OEN_DQM */
		io_32_write_fld_multi(
			DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ1),
			p_fld(delay1, SHURK0_SELPH_DQ1_TXDLY_DQM0)
			| p_fld(delay1, SHURK0_SELPH_DQ1_TXDLY_DQM1)
			| p_fld(delay1, SHURK0_SELPH_DQ1_TXDLY_DQM2)
			| p_fld(delay1, SHURK0_SELPH_DQ1_TXDLY_DQM3)
			| p_fld(delay2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0)
			| p_fld(delay2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1)
			| p_fld(delay2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2)
			| p_fld(delay2, SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3));

		/* DLY_DQM[2:0] */
		io_32_write_fld_multi(
			DRAMC_REG_ADDR(DRAMC_REG_SHURK0_SELPH_DQ3),
			p_fld(delay3, SHURK0_SELPH_DQ3_DLY_DQM0)
			| p_fld(delay3, SHURK0_SELPH_DQ3_DLY_DQM1)
			| p_fld(delay3, SHURK0_SELPH_DQ3_DLY_DQM2)
			| p_fld(delay3, SHURK0_SELPH_DQ3_DLY_DQM3)
			| p_fld(delay4, SHURK0_SELPH_DQ3_DLY_OEN_DQM0)
			| p_fld(delay4, SHURK0_SELPH_DQ3_DLY_OEN_DQM1)
			| p_fld(delay4, SHURK0_SELPH_DQ3_DLY_OEN_DQM2)
			| p_fld(delay4, SHURK0_SELPH_DQ3_DLY_OEN_DQM3));
	}

}

static unsigned char move_dq_delay_lp4(DRAMC_CTX_T *p, unsigned short delay,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	unsigned char vref_scan_enable, unsigned char dq_ui_small_reg_value)
{
	unsigned char dq_pi, dq_ui_small, dq_ui_large,
		dq_oen_ui_small, dq_oen_ui_large;

	tx_win_transfer_delay_to_uipi(p, delay, 0,
		&dq_ui_large, &dq_ui_small,
		&dq_pi, &dq_oen_ui_large, &dq_oen_ui_small);

	if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
	    || cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
		/* TXDLY_DQ , TXDLY_OEN_DQ */
		if (dq_ui_small_reg_value != dq_ui_small) {
			set_txdq_delay_lp4(p, dq_ui_large,
				dq_oen_ui_large, dq_ui_small,
				dq_oen_ui_small, 0);
		}
	}

	if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY
	    || cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
		/* TXDLY_DQM , TXDLY_OEN_DQM */
		if (dq_ui_small_reg_value != dq_ui_small) {
			set_txdq_delay_lp4(p, dq_ui_large,
				dq_oen_ui_large, dq_ui_small,
				dq_oen_ui_small, 1);
		}
	}

	dq_ui_small_reg_value = dq_ui_small;

	/* set to registers, PI DQ (per byte) */

	/*  update TX DQ PI delay, for rank 0 need to take care rank 1 and 2 */
	if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
	    || cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B0_DQ7),
			dq_pi, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B1_DQ7),
			dq_pi, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1);
	}

	if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY
	    || cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B0_DQ7),
			dq_pi, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B1_DQ7),
			dq_pi, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1);
	}
	if (vref_scan_enable == 0) {
	show_msg3((INFO,
		"Delay=%3d |%2d %2d %3d| ",
		delay, dq_ui_large, dq_ui_small, dq_pi));
	}
	return dq_ui_small_reg_value;
}

static void txdly_dq_set(DRAMC_CTX_T *p,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	TX_DLY_T *dq_ptr)
{
	/* TXDLY_DQ , TXDLY_OEN_DQ */
	if ((cal_type == TX_DQ_DQS_MOVE_DQ_ONLY)
		|| (cal_type == TX_DQ_DQS_MOVE_DQ_DQM)) {
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHURK0_SELPH_DQ0),
			p_fld(dq_ptr->dq_final_ui_large[0],
			SHURK0_SELPH_DQ0_TXDLY_DQ0) |
			p_fld(dq_ptr->dq_final_ui_large[1],
			SHURK0_SELPH_DQ0_TXDLY_DQ1) |
			p_fld(dq_ptr->dq_final_ui_large[2],
			SHURK0_SELPH_DQ0_TXDLY_DQ2) |
			p_fld(dq_ptr->dq_final_ui_large[3],
			SHURK0_SELPH_DQ0_TXDLY_DQ3) |
			p_fld(dq_ptr->dq_final_oen_ui_large[0],
			SHURK0_SELPH_DQ0_TXDLY_OEN_DQ0) |
			p_fld(dq_ptr->dq_final_oen_ui_large[1],
			SHURK0_SELPH_DQ0_TXDLY_OEN_DQ1) |
			p_fld(dq_ptr->dq_final_oen_ui_large[2],
			SHURK0_SELPH_DQ0_TXDLY_OEN_DQ2) |
			p_fld(dq_ptr->dq_final_oen_ui_large[3],
			SHURK0_SELPH_DQ0_TXDLY_OEN_DQ3));

		/*  DLY_DQ[2:0] */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHURK0_SELPH_DQ2),
			p_fld(dq_ptr->dq_final_ui_small[0],
			SHURK0_SELPH_DQ2_DLY_DQ0) |
			p_fld(dq_ptr->dq_final_ui_small[1],
			SHURK0_SELPH_DQ2_DLY_DQ1) |
			p_fld(dq_ptr->dq_final_ui_small[2],
			SHURK0_SELPH_DQ2_DLY_DQ2) |
			p_fld(dq_ptr->dq_final_ui_small[3],
			SHURK0_SELPH_DQ2_DLY_DQ3) |
			p_fld(dq_ptr->dq_final_oen_ui_small[0],
			SHURK0_SELPH_DQ2_DLY_OEN_DQ0) |
			p_fld(dq_ptr->dq_final_oen_ui_small[1],
			SHURK0_SELPH_DQ2_DLY_OEN_DQ1) |
			p_fld(dq_ptr->dq_final_oen_ui_small[2],
			SHURK0_SELPH_DQ2_DLY_OEN_DQ2) |
			p_fld(dq_ptr->dq_final_oen_ui_small[3],
			SHURK0_SELPH_DQ2_DLY_OEN_DQ3));
	}
}

static void txdly_dqm_set(DRAMC_CTX_T *p,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	TX_FINAL_DLY_T *dqm_ptr, unsigned char vref_scan_enable)
{
#if TX_K_DQM_WITH_WDBI
	if ((cal_type == TX_DQ_DQS_MOVE_DQM_ONLY)
		|| (cal_type == TX_DQ_DQS_MOVE_DQ_DQM)
		|| ((cal_type == TX_DQ_DQS_MOVE_DQ_ONLY)
		&& vref_scan_enable))
#endif
	{
		if ((p->dram_type != TYPE_PCDDR4) ||
			(cal_type != TX_DQ_DQS_MOVE_DQ_ONLY)) {
		/* TXDLY_DQM , TXDLY_OEN_DQM */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHURK0_SELPH_DQ1),
			p_fld(dqm_ptr->dq_final_dqm_ui_large[0],
			SHURK0_SELPH_DQ1_TXDLY_DQM0) |
			p_fld(dqm_ptr->dq_final_dqm_ui_large[1],
			SHURK0_SELPH_DQ1_TXDLY_DQM1) |
			p_fld(dqm_ptr->dq_final_dqm_ui_large[2],
			SHURK0_SELPH_DQ1_TXDLY_DQM2) |
			p_fld(dqm_ptr->dq_final_dqm_ui_large[3],
			SHURK0_SELPH_DQ1_TXDLY_DQM3) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_large[0],
			SHURK0_SELPH_DQ1_TXDLY_OEN_DQM0) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_large[1],
			SHURK0_SELPH_DQ1_TXDLY_OEN_DQM1) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_large[2],
			SHURK0_SELPH_DQ1_TXDLY_OEN_DQM2) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_large[3],
			SHURK0_SELPH_DQ1_TXDLY_OEN_DQM3));

		/*  DLY_DQM[2:0] */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHURK0_SELPH_DQ3),
			p_fld(dqm_ptr->dq_final_dqm_ui_small[0],
			SHURK0_SELPH_DQ3_DLY_DQM0) |
			p_fld(dqm_ptr->dq_final_dqm_ui_small[1],
			SHURK0_SELPH_DQ3_DLY_DQM1) |
			p_fld(dqm_ptr->dq_final_dqm_ui_small[2],
			SHURK0_SELPH_DQ3_DLY_DQM2) |
			p_fld(dqm_ptr->dq_final_dqm_ui_small[3],
			SHURK0_SELPH_DQ3_DLY_DQM3) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_small[0],
			SHURK0_SELPH_DQ3_DLY_OEN_DQM0) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_small[1],
			SHURK0_SELPH_DQ3_DLY_OEN_DQM1) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_small[2],
			SHURK0_SELPH_DQ3_DLY_OEN_DQM2) |
			p_fld(dqm_ptr->dq_final_dqm_oen_ui_small[3],
			SHURK0_SELPH_DQ3_DLY_OEN_DQM3));
		}
	}
}


static void txdly_dqpi_set(DRAMC_CTX_T *p, unsigned char vref_scan_enable,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	TX_FINAL_DLY_T *dqm_ptr, TX_DLY_T *dq_ptr)
{
	unsigned char byte_idx;
	unsigned int bak_channel;

	bak_channel = p->channel;

	for (byte_idx = 0; byte_idx < p->data_width/DQS_BIT_NUM;
			byte_idx += 2) {
		if ((p->data_width == DATA_WIDTH_32BIT) &&
			(byte_idx > 1)){
			p->channel = CHANNEL_B; /* Write to B23 reg */
		}

#if TX_K_DQM_WITH_WDBI
		if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
			|| cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
			io_32_write_fld_align(DRAMC_REG_ADDR
				(DDRPHY_SHU1_R0_B0_DQ7),
				dq_ptr->dq_final_pi[byte_idx],
				SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
			io_32_write_fld_align(DRAMC_REG_ADDR
				(DDRPHY_SHU1_R0_B1_DQ7),
				dq_ptr->dq_final_pi[byte_idx+1],
				SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1);
		}

		if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY
			|| cal_type == TX_DQ_DQS_MOVE_DQ_DQM
			|| (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
			&& vref_scan_enable)) {
			io_32_write_fld_align(DRAMC_REG_ADDR
				(DDRPHY_SHU1_R0_B0_DQ7),
				dqm_ptr->dq_final_dqm_pi[byte_idx],
				SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0);
			io_32_write_fld_align(DRAMC_REG_ADDR
				(DDRPHY_SHU1_R0_B1_DQ7),
				dqm_ptr->dq_final_dqm_pi[byte_idx+1],
				SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1);
		}
#else /* !TX_K_DQM_WITH_WDBI */
		if ((p->dram_type == TYPE_PCDDR4)
			&& (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY)) {
			io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7),
				dq_ptr->dq_final_pi[byte_idx],
				SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);

			io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7),
				dq_ptr->dq_final_pi[byte_idx+1],
				SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1);
		} else {
			io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7),
				p_fld(dq_ptr->dq_final_pi[byte_idx],
				SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0) |
				p_fld(dqm_ptr->dq_final_dqm_pi[byte_idx],
				SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0));

			io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7),
				p_fld(dq_ptr->dq_final_pi[byte_idx+1],
				SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1) |
				p_fld(dqm_ptr->dq_final_dqm_pi[byte_idx+1],
				SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1));
		}
#endif
	}

	p->channel = bak_channel;
}

#if ENABLE_TX_TRACKING
static void txdly_dqpi_txtracking_set(DRAMC_CTX_T *p,
	unsigned char vref_scan_enable,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	TX_FINAL_DLY_T *dqm_ptr, TX_DLY_T *dq_ptr)
{
#if TX_K_DQM_WITH_WDBI
	if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
		|| cal_type == TX_DQ_DQS_MOVE_DQM_ONLY)
#else
	if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY)
#endif
	{
		/* make a copy to dramc reg for TX DQ tracking used */
#if TX_K_DQM_WITH_WDBI
		if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY) {
			io_32_write_fld_multi(DRAMC_REG_ADDR
				(DRAMC_REG_SHU1RK0_PI),
				p_fld(dq_ptr->dq_final_pi[0],
				SHU1RK0_PI_RK0_ARPI_DQ_B0)
				| p_fld(dq_ptr->dq_final_pi[1],
				SHU1RK0_PI_RK0_ARPI_DQ_B1));

			/*  Source DQ */
			io_32_write_fld_multi(DRAMC_REG_ADDR
				(DRAMC_REG_SHU1RK0_DQS2DQ_CAL1),
				p_fld(dq_ptr->dq_final_pi[1],
				SHU1RK0_DQS2DQ_CAL1_BOOT_ORIG_UI_RK0_DQ1)
				| p_fld(dq_ptr->dq_final_pi[0],
				SHU1RK0_DQS2DQ_CAL1_BOOT_ORIG_UI_RK0_DQ0));
			/*  Target DQ */
			io_32_write_fld_multi(DRAMC_REG_ADDR
				(DRAMC_REG_SHU1RK0_DQS2DQ_CAL2),
				p_fld(dq_ptr->dq_final_pi[1],
				SHU1RK0_DQS2DQ_CAL2_BOOT_TARG_UI_RK0_DQ1)
				| p_fld(dq_ptr->dq_final_pi[0],
				SHU1RK0_DQS2DQ_CAL2_BOOT_TARG_UI_RK0_DQ0));
		}

		if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY
			|| (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
			&& vref_scan_enable)) {
			io_32_write_fld_multi(DRAMC_REG_ADDR
				(DRAMC_REG_SHU1RK0_PI),
				p_fld(dqm_ptr->dq_final_dqm_pi[0],
				SHU1RK0_PI_RK0_ARPI_DQM_B0)
				| p_fld(dqm_ptr->dq_final_dqm_pi[1],
				SHU1RK0_PI_RK0_ARPI_DQM_B1));

			/*  Target DQM */
			io_32_write_fld_multi(DRAMC_REG_ADDR
				(DRAMC_REG_SHU1RK0_DQS2DQ_CAL5),
				p_fld(dqm_ptr->dq_final_dqm_pi[1],
				SHU1RK0_DQS2DQ_CAL5_BOOT_TARG_UI_RK0_DQM1)
				| p_fld(dqm_ptr->dq_final_dqm_pi[0],
				SHU1RK0_DQS2DQ_CAL5_BOOT_TARG_UI_RK0_DQM0));
		}
#else /* !TX_K_DQM_WITH_WDBI */

		io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU1RK0_PI),
			p_fld(dq_ptr->dq_final_pi[0],
			SHU1RK0_PI_RK0_ARPI_DQ_B0)
			| p_fld(dq_ptr->dq_final_pi[1],
			SHU1RK0_PI_RK0_ARPI_DQ_B1)
			| p_fld(dqm_ptr->dq_final_dqm_pi[0],
			SHU1RK0_PI_RK0_ARPI_DQM_B0)
			| p_fld(dqm_ptr->dq_final_dqm_pi[1],
			SHU1RK0_PI_RK0_ARPI_DQM_B1));

		/*  Source DQ */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHU1RK0_DQS2DQ_CAL1),
			p_fld(dq_ptr->dq_final_pi[1],
			SHU1RK0_DQS2DQ_CAL1_BOOT_ORIG_UI_RK0_DQ1)
			| p_fld(dq_ptr->dq_final_pi[0],
			SHU1RK0_DQS2DQ_CAL1_BOOT_ORIG_UI_RK0_DQ0));
		/*  Target DQ */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHU1RK0_DQS2DQ_CAL2),
			p_fld(dq_ptr->dq_final_pi[1],
			SHU1RK0_DQS2DQ_CAL2_BOOT_TARG_UI_RK0_DQ1)
			| p_fld(dq_ptr->dq_final_pi[0],
			SHU1RK0_DQS2DQ_CAL2_BOOT_TARG_UI_RK0_DQ0));

		/*  Target DQM */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DRAMC_REG_SHU1RK0_DQS2DQ_CAL5),
			p_fld(dqm_ptr->dq_final_dqm_pi[1],
			SHU1RK0_DQS2DQ_CAL5_BOOT_TARG_UI_RK0_DQM1)
			| p_fld(dqm_ptr->dq_final_dqm_pi[0],
			SHU1RK0_DQS2DQ_CAL5_BOOT_TARG_UI_RK0_DQM0));

#endif
	}
}
#endif /* End ENABLE_TX_TRACKING */

static void tx_cal_delay_cell_perbit(DRAMC_CTX_T *p, unsigned char byte,
	unsigned short center_min[], unsigned char delay_cell_ofst[],
	PASS_WIN_DATA_T final_win_per_bit[])
{
	unsigned char bit_idx;

	/* calculate delay cell perbit */
	for (bit_idx = 0; bit_idx < DQS_BIT_NUM; bit_idx++) {
		unsigned char u1BitTemp = byte * DQS_BIT_NUM + bit_idx;
		unsigned char u1PIDiff =
			final_win_per_bit[u1BitTemp].win_center -
			center_min[byte];
		if (p->delay_cell_timex100 != 0) {
			delay_cell_ofst[u1BitTemp] = (u1PIDiff * 100000000 /
				(p->frequency * 64)) / p->delay_cell_timex100;
			show_msg((INFO,
				"delay_cell_ofst[%d]=%d cells (%d PI)\n",
				u1BitTemp, delay_cell_ofst[u1BitTemp],
				u1PIDiff));
		} else {
			show_msg((INFO, "Error: Cell time %s is 0\n",
				"(p->delay_cell_timex100)"));
			break;
		}
	}
}

static unsigned int tx_win_per_bit_cal(DRAMC_CTX_T *p,
	unsigned int finish_count, unsigned char bit_idx,
	unsigned int fail_bit, unsigned short delay,
	unsigned short dq_delay_end, PASS_WIN_DATA_T win_per_bit[],
	PASS_WIN_DATA_T vrefwin_per_bit[])
{
	if (win_per_bit[bit_idx].first_pass ==
		PASS_RANGE_NA) {
		if (fail_bit == 0)
			win_per_bit[bit_idx].first_pass = delay;
	} else if (win_per_bit[bit_idx].last_pass ==
		PASS_RANGE_NA) {
		if (fail_bit != 0)
			win_per_bit[bit_idx].last_pass = (delay - 1);
		else if (delay == dq_delay_end)
			win_per_bit[bit_idx].last_pass = delay;

		if (win_per_bit[bit_idx].last_pass != PASS_RANGE_NA) {
			if ((win_per_bit[bit_idx].last_pass -
				win_per_bit[bit_idx].first_pass) >=
				(vrefwin_per_bit[bit_idx].last_pass -
				vrefwin_per_bit[bit_idx].first_pass)) {
				if ((vrefwin_per_bit[bit_idx].last_pass !=
					PASS_RANGE_NA) && (vrefwin_per_bit
					[bit_idx].last_pass - vrefwin_per_bit
					[bit_idx].first_pass) > 0) {
					/*
					show_msg2((INFO, "Bit[%d%s%d > %d%s",
						bit_idx,
						"] Bigger window update ",
						(win_per_bit[bit_idx].last_pass
						- win_per_bit[bit_idx].
						first_pass), (vrefwin_per_bit
						[bit_idx].last_pass -
						vrefwin_per_bit[bit_idx].
						first_pass),
						", window broken?\n"));*/

				}
				/* if window size bigger than 7,
				* consider as real pass window.
				* If not, don't update finish counte
				* and won't do early break;
				*/
				if ((win_per_bit[bit_idx].last_pass -
					win_per_bit[bit_idx].first_pass) > 7)
					finish_count |= (1 << bit_idx);

				/* update bigger window size */
				vrefwin_per_bit[bit_idx].first_pass =
					win_per_bit[bit_idx].first_pass;
				vrefwin_per_bit[bit_idx].last_pass =
					win_per_bit[bit_idx].last_pass;
			}
			/* reset tmp window */
			win_per_bit[bit_idx].first_pass = PASS_RANGE_NA;
			win_per_bit[bit_idx].last_pass = PASS_RANGE_NA;
		}
	}
return finish_count;
}

static void tx_move_dq(DRAMC_CTX_T *p, unsigned char vref_scan_enable,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	unsigned short dq_delay_begin, unsigned short dq_delay_end,
	PASS_WIN_DATA_T win_per_bit[], PASS_WIN_DATA_T vrefwin_per_bit[])
{
	unsigned char bit_idx;
	unsigned short delay;
	unsigned char dq_ui_small_reg_value;
	unsigned int finish_count;
	unsigned int err_value, fail_bit;
	unsigned short step;

	/*  initialize parameters */
	finish_count = 0;
	dq_ui_small_reg_value = BYTE_MAX;
#if (FOR_DV_SIMULATION_USED == 0)
	step = 1;
#else
	step = 4;
#endif

	for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
		win_per_bit[bit_idx].first_pass =
			(signed short) PASS_RANGE_NA;
		win_per_bit[bit_idx].last_pass =
			(signed short) PASS_RANGE_NA;
		vrefwin_per_bit[bit_idx].first_pass =
			(signed short) PASS_RANGE_NA;
		vrefwin_per_bit[bit_idx].last_pass =
			(signed short) PASS_RANGE_NA;
	}

	/*
	 * Move DQ delay ,  1 PI = tCK/64, total 128 PI, 1UI = 32 PI
	 * For data rate 3200, max tDQS2DQ is 2.56UI (82 PI)
	 * For data rate 4266, max tDQS2DQ is 3.41UI (109 PI)
	 */
	for (delay = dq_delay_begin; delay < dq_delay_end; delay += step) {
		dq_ui_small_reg_value = move_dq_delay_lp4(p, delay, cal_type,
			vref_scan_enable, dq_ui_small_reg_value);

		/* audio +xtalk pattern */
		err_value = 0;
		/* cc notes: for Simulation, too time costy...
		 * So only use XTALK.
		 */
	#if (FOR_DV_SIMULATION_USED == 0)
		dramc_engine2_set_pat(p, TEST_AUDIO_PATTERN, 0, 0);
		err_value = dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK,
			TEST_AUDIO_PATTERN);
	#endif
		dramc_engine2_set_pat(p, TEST_XTALK_PATTERN, 0, 0);
		err_value |= dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK,
			TEST_XTALK_PATTERN);
#if CALIBRATION_LOG
		show_msg3((INFO, "0x%8x [0]", err_value));
#endif
		/* check fail bit ,0 ok ,others fail */
		for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
			fail_bit = err_value & ((unsigned int) 1 << bit_idx);
#if CALIBRATION_LOG
			if (bit_idx % DQS_BIT_NUM == 0)
				show_msg3((INFO, " "));
			if (fail_bit == 0)
				show_msg3((INFO, "o"));
			else
				show_msg3((INFO, "x"));
#endif
			finish_count = tx_win_per_bit_cal(p, finish_count,
				bit_idx, fail_bit, delay, dq_delay_end,
				win_per_bit, vrefwin_per_bit);
		}
#if CALIBRATION_LOG
		show_msg3((INFO, " [MSB]\n"));
#endif
		/*
		 * if all bits widnow found and all bits turns to fail again,
		 * early break;
		*/
		if (((p->data_width == DATA_WIDTH_16BIT) &&
			 (finish_count == WORD_MAX)) ||
			((p->data_width == DATA_WIDTH_32BIT) &&
			 (finish_count == LWORD_MAX))) {
			set_calibration_result(p, DRAM_CALIBRATION_TX_PERBIT,
				DRAM_OK);
				break;
		}
	}
}

static void tx_set_delay_cell(DRAMC_CTX_T *p,
	unsigned char delay_cell_ofst[])
{
	unsigned char *lpddr_4bitmux_byte_mapping;
	unsigned int b0_reg, b1_reg, b2_reg = 0, b3_reg = 0;

	/* Process 4BIT pinmux. PINMUX, REVIEW */
	if (p->en_4bit_mux) {
		unsigned char swap;
		unsigned char bit_idx;
		unsigned char bit_mapping;

		lpddr_4bitmux_byte_mapping =
			dramc_get_4bitmux_byte_mapping(p);
		if (lpddr_4bitmux_byte_mapping == NULL)
			return;

		for (bit_idx = 0; bit_idx < 8; bit_idx++) {
			bit_mapping = lpddr_4bitmux_byte_mapping[bit_idx];

			swap = delay_cell_ofst[bit_idx];
			delay_cell_ofst[bit_idx] =
				delay_cell_ofst[bit_mapping];
			delay_cell_ofst[bit_mapping] = swap;

			if (p->data_width == DATA_WIDTH_32BIT) {
				bit_mapping =
					lpddr_4bitmux_byte_mapping[bit_idx+16];

				swap = delay_cell_ofst[bit_idx+16];
				delay_cell_ofst[bit_idx+16] =
					delay_cell_ofst[bit_mapping];
				delay_cell_ofst[bit_mapping] = swap;
			}
		}
	}

	if (p->channel == CHANNEL_B) {
		/* Swap PHY B2 and B3 */
		b0_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ0);
		b1_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0);
	} else {
		b0_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0);
		b1_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ0);

		if (p->data_width == DATA_WIDTH_32BIT) {
			b2_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0 +
			(CHANNEL_B << POS_BANK_NUM));
			b3_reg = DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ0 +
				(CHANNEL_B << POS_BANK_NUM));
		}
	}

	io_32_write_fld_multi(b0_reg,
		p_fld(delay_cell_ofst[7],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0) |
		p_fld(delay_cell_ofst[6],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0) |
		p_fld(delay_cell_ofst[5],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0) |
		p_fld(delay_cell_ofst[4],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0) |
		p_fld(delay_cell_ofst[3],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0) |
		p_fld(delay_cell_ofst[2],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0) |
		p_fld(delay_cell_ofst[1],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0) |
		p_fld(delay_cell_ofst[0],
		SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
	io_32_write_fld_multi(b1_reg,
		p_fld(delay_cell_ofst[15],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1) |
		p_fld(delay_cell_ofst[14],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1) |
		p_fld(delay_cell_ofst[13],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1) |
		p_fld(delay_cell_ofst[12],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1) |
		p_fld(delay_cell_ofst[11],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1) |
		p_fld(delay_cell_ofst[10],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1) |
		p_fld(delay_cell_ofst[9],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1) |
		p_fld(delay_cell_ofst[8],
		SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1));

	/* Set PHY B23 */
	if (p->data_width == DATA_WIDTH_32BIT) {
		io_32_write_fld_multi(b2_reg,
			p_fld(delay_cell_ofst[23],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0) |
			p_fld(delay_cell_ofst[22],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0) |
			p_fld(delay_cell_ofst[21],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0) |
			p_fld(delay_cell_ofst[20],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0) |
			p_fld(delay_cell_ofst[19],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0) |
			p_fld(delay_cell_ofst[18],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0) |
			p_fld(delay_cell_ofst[17],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0) |
			p_fld(delay_cell_ofst[16],
			SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
		io_32_write_fld_multi(b3_reg,
			p_fld(delay_cell_ofst[31],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1) |
			p_fld(delay_cell_ofst[30],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1) |
			p_fld(delay_cell_ofst[29],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1) |
			p_fld(delay_cell_ofst[28],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1) |
			p_fld(delay_cell_ofst[27],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1) |
			p_fld(delay_cell_ofst[26],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1) |
			p_fld(delay_cell_ofst[25],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1) |
			p_fld(delay_cell_ofst[24],
			SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1));
	}
}

DRAM_STATUS_T dramc_tx_window_perbit_cal(DRAMC_CTX_T *p,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	unsigned char vref_scan_enable)
{
	unsigned char ucindex, bit_idx, byte;
	unsigned char ii, backup_rank;
	PASS_WIN_DATA_T win_per_bit[DQ_DATA_WIDTH],
		vrefwin_per_bit[DQ_DATA_WIDTH],
		final_win_per_bit[DQ_DATA_WIDTH];
#if TX_K_DQM_WITH_WDBI
	unsigned char mck2ui;
#endif
	unsigned short smallest_virtual_delay = WORD_MAX;
	unsigned short delay, dq_delay_begin = 0, dq_delay_end = 0,
		tx_dq_pre_cal_lp4_samll;
	TX_DLY_T final_dq_dqm;
	TX_FINAL_DLY_T dq_final_dqm;

	show_msg((INFO, "\n[TxWindowPerbitCal]\n"));

	/* TX_DQM_CALC_MAX_MIN_CENTER */
	unsigned short center_min[DQS_NUMBER], center_max[DQS_NUMBER];
	unsigned char enable_delay_cell = 0;
	unsigned char delay_cell_ofst[DQ_DATA_WIDTH];
	unsigned short vref_range, vref, final_vref = 0xd;
	unsigned short vref_begin, final_range = 0, vref_end, vref_step;
	unsigned short temp_win_sum, tx_window_sum;
	unsigned short tx_perbit_win_min_max = 0;
	unsigned int min_bit, min_winsize;
	unsigned char enable_full_eye_scan = 0;

	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}
	print_calibration_basic_info(p);
	memset(&final_dq_dqm, 0x0, sizeof(TX_DLY_T));

	backup_rank = get_rank(p);

	/* Set TX delay chain to 0 */
#if TX_K_DQM_WITH_WDBI
	if (cal_type != TX_DQ_DQS_MOVE_DQM_ONLY)
#endif
	{
		io32_write_4b_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0), 0);
		io32_write_4b_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ0), 0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ1),
			CLEAR_FLD, SHU1_R0_B0_DQ1_RK0_TX_ARDQM0_DLY_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ1),
			CLEAR_FLD, SHU1_R0_B1_DQ1_RK0_TX_ARDQM0_DLY_B1);
	}

#if TX_K_DQM_WITH_WDBI
	mck2ui = VALUE_3;
#endif
	smallest_virtual_delay = find_smallest_dqs_delay(p);

	/*
	* (1)LP4 will calibration DQM at the first time, K full range,
	* and then rember the TX position.
	* (2)The sencod time will calibrate DQ+Vref, reference TX postion of (1)
	*/
	if (cal_type == TX_DQ_DQS_MOVE_DQ_DQM) {
		dq_delay_begin = smallest_virtual_delay;
		dq_delay_end = dq_delay_begin + BYTE_MAX;
	} else {	/* (cal_type==TX_DQ_DQS_MOVE_DQ_ONLY) */
		if (tx_dq_pre_cal_lp4[0] < tx_dq_pre_cal_lp4[1])
			tx_dq_pre_cal_lp4_samll = tx_dq_pre_cal_lp4[0];
		else
			tx_dq_pre_cal_lp4_samll = tx_dq_pre_cal_lp4[1];

		if (tx_dq_pre_cal_lp4_samll > 24)
			dq_delay_begin = tx_dq_pre_cal_lp4_samll - 24;
		else
			dq_delay_begin = 0;

#if TX_K_DQM_WITH_WDBI
		if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY) {
			/* DBI on, calibration range -1MCK */
			dq_delay_begin -= (1 << (mck2ui + 5));
		}
#endif
		dq_delay_end = dq_delay_begin + VALUE_64;
	}

	if (vref_scan_enable) {
		if (p->odt_onoff == ODT_OFF) {
			if (p->dram_type == TYPE_LPDDR4) {
				/* range 1 */
				vref_begin = TX_VREF_RANGE_BEGIN1;
				vref_end = TX_VREF_RANGE_END1;
			} else {
				/* range 1 */
				vref_begin = TX_VREF_RANGE_BEGIN2;
				vref_end = TX_VREF_RANGE_END2;
			}
			vref_step = 1;
		} else {
			/* range 0 */
			vref_begin = TX_VREF_RANGE_BEGIN;
			vref_end = TX_VREF_RANGE_END;
			vref_step = TX_VREF_RANGE_STEP;
		}
	} else {
		vref_begin = 0;
		vref_end = 0;
		vref_step = 1;
	}

	final_range = vref_range = (!(p->odt_onoff));

	tx_window_sum = 0;
	set_calibration_result(p, DRAM_CALIBRATION_TX_PERBIT, DRAM_FAIL);
	show_msg2((INFO,
		"[TxWindowPerbitCal] cal_type=%d, VrefScanEnable %d range %d(%d~%d)\n"
		"Begin, DQ Scan Range %d~%d\n", cal_type, vref_scan_enable, final_range,
		vref_begin, vref_end, dq_delay_begin, dq_delay_end));

	io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), SET_FLD,
		MISC_CTRL1_R_DMARPIDQ_SW);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), SET_FLD,
		DQSOSCR_ARUIDQ_SW);

	dramc_engine2_init(p, p->test2_1, p->test2_2, p->test_pattern, 0);

	for (vref = vref_begin; vref <= vref_end; vref += vref_step) {
		/*  SET tx Vref (DQ) here */
		if (vref_scan_enable)
			set_txdq_vref_lp4(p, vref_range, vref);
		else
			show_msg2((INFO, "\n\tTX Vref Scan disable\n"));

		/*  initialize parameters */
		temp_win_sum = 0;

		tx_move_dq(p, vref_scan_enable, cal_type, dq_delay_begin,
			dq_delay_end, win_per_bit, vrefwin_per_bit);

		min_winsize = WORD_MAX;
		min_bit = BYTE_MAX;
		for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
			vrefwin_per_bit[bit_idx].win_size =
				vrefwin_per_bit[bit_idx].last_pass -
				vrefwin_per_bit[bit_idx].first_pass +
				(vrefwin_per_bit[bit_idx].last_pass ==
				vrefwin_per_bit[bit_idx].first_pass ? 0 : 1);

			if (vrefwin_per_bit[bit_idx].win_size < min_winsize) {
				min_bit = bit_idx;
				min_winsize =
					vrefwin_per_bit[bit_idx].win_size;
			}
			/* Sum of CA Windows for vref selection */
			temp_win_sum += vrefwin_per_bit[bit_idx].win_size;
		}
		show_msg((INFO, "TXVref=%d, MinBit=%d, winsize=%d, temp_win_sum=%d\n", vref,
			min_bit, min_winsize, temp_win_sum));

		if ((min_winsize > tx_perbit_win_min_max)
			|| ((min_winsize == tx_perbit_win_min_max)
			&& (temp_win_sum > tx_window_sum))) {
			if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY) {
				show_msg((INFO, "%s%d, Window Sum %d > %d\n\n",
					 "Better TX Vref found ", vref,
					 temp_win_sum, tx_window_sum));
			}

			tx_perbit_win_min_max = min_winsize;
			tx_window_sum = temp_win_sum;
			final_range = vref_range;
			final_vref = vref;
#ifdef ETT
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)
			ett_tx_vref = vref;
#endif
			ett_tx_min = 0xff;
			for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
				ett_tx_win[bit_idx] = vrefwin_per_bit[bit_idx].win_size;
				ett_tx_first[bit_idx] = vrefwin_per_bit[bit_idx].first_pass;
				ett_tx_last[bit_idx] = vrefwin_per_bit[bit_idx].last_pass;
				if (ett_tx_win[bit_idx] < ett_tx_min)
					ett_tx_min = ett_tx_win[bit_idx];
			}
#endif
			/* Calculate the center of DQ pass window */
			/* Record center sum of each byte */
			for (byte = 0; byte < (p->data_width / DQS_BIT_NUM);
				byte++) {
				/* TX_DQM_CALC_MAX_MIN_CENTER */
				center_min[byte] = WORD_MAX;
				center_max[byte] = 0;

				for (bit_idx = 0; bit_idx < DQS_BIT_NUM;
					bit_idx++) {
					ucindex = byte * DQS_BIT_NUM + bit_idx;
					final_win_per_bit[ucindex].first_pass =
						vrefwin_per_bit[ucindex].
						first_pass;
					final_win_per_bit[ucindex].last_pass =
						vrefwin_per_bit[ucindex].
						last_pass;
					final_win_per_bit[ucindex].win_size =
						vrefwin_per_bit[ucindex].
						win_size;
					final_win_per_bit[ucindex].win_center =
						(final_win_per_bit[ucindex].
						first_pass + final_win_per_bit
						[ucindex].last_pass) >> 1;

					if (final_win_per_bit[ucindex].
						win_center < center_min[byte])
						center_min[byte] =
							final_win_per_bit
							[ucindex].win_center;

					if (final_win_per_bit[ucindex].
						win_center > center_max[byte])
						center_max[byte] =
							final_win_per_bit
							[ucindex].win_center;

				}
			}
		}

		if ((temp_win_sum < (tx_window_sum * 95 / 100))
			&& tx_eye_scan_flag == 0) {
			show_msg3((INFO,
				"\nTX Vref found, early break! %d< %d\n",
				temp_win_sum, (tx_window_sum * 95 / 100)));
			break;	/* max vref found (95%) , early break; */
		}
	}

	dramc_engine2_end(p);

	/*
	* first freq 800(LP4-1600) doesn't support jitter meter(data < 1T),
	* therefore, don't use delay cell
	*/
	if ((cal_type == TX_DQ_DQS_MOVE_DQ_ONLY)
		&& (p->frequency >= PERBIT_THRESHOLD_FREQ)
		&& (p->delay_cell_timex100 != 0)) {
		enable_delay_cell = 1;
		show_msg((INFO, "%sDelayCellTimex100 =%d/100 ps\n",
			"[TX_PER_BIT_DELAY_CELL] ", p->delay_cell_timex100));
	}
	/* Calculate the center of DQ pass window */
	/* average the center delay */
	for (byte = 0; byte < (p->data_width / DQS_BIT_NUM); byte++) {
		if (enable_delay_cell == 0) {
			delay = ((center_min[byte] + center_max[byte]) >> 1);
			tx_dq_pre_cal_lp4[byte] = delay;
		} else {	/*  if(cal_type == TX_DQ_DQS_MOVE_DQ_ONLY) */
			delay = center_min[byte];
			tx_dq_pre_cal_lp4[byte] = ((center_min[byte] +
				center_max[byte]) >> 1);

			/* calculate delay cell perbit */
			tx_cal_delay_cell_perbit(p, byte, center_min,
				delay_cell_ofst, final_win_per_bit);
		}

		tx_win_transfer_delay_to_uipi(p, delay, 1,
			&final_dq_dqm.dq_final_ui_large[byte],
			&final_dq_dqm.dq_final_ui_small[byte],
			&final_dq_dqm.dq_final_pi[byte],
			&final_dq_dqm.dq_final_oen_ui_large[byte],
			&final_dq_dqm.dq_final_oen_ui_small[byte]);

		tx_win_transfer_delay_to_uipi(p, tx_dq_pre_cal_lp4[byte], 1,
			&dq_final_dqm.dq_final_dqm_ui_large[byte],
			&dq_final_dqm.dq_final_dqm_ui_small[byte],
			&dq_final_dqm.dq_final_dqm_pi[byte],
			&dq_final_dqm.dq_final_dqm_oen_ui_large[byte],
			&dq_final_dqm.dq_final_dqm_oen_ui_small[byte]);

		show_msg((INFO, "Byte%d, DQ PI dly=%d, DQM PI dly= %d\n",
			byte, delay, tx_dq_pre_cal_lp4[byte]));
		show_msg((INFO, "%s=(%d ,%d, %d)\n",
			"Final DQ PI dly(LargeUI, SmallUI, PI) ",
			final_dq_dqm.dq_final_ui_large[byte],
			final_dq_dqm.dq_final_ui_small[byte],
			final_dq_dqm.dq_final_pi[byte]));
		show_msg((INFO,	"%s =(%d ,%d, %d)\n",
			"OEN DQ PI dly(LargeUI, SmallUI, PI)",
			final_dq_dqm.dq_final_oen_ui_large[byte],
			final_dq_dqm.dq_final_oen_ui_small[byte],
			final_dq_dqm.dq_final_pi[byte]));
	}

	/* DRAMC delay is set to B01. But PI delay is in PHY and
	 * need to process the mapping relationship.
	 */
	if (p->channel == CHANNEL_B) {
		unsigned char dq_pi;

		/* Swap PI for pinmux */
		dq_pi = final_dq_dqm.dq_final_pi[0];
		final_dq_dqm.dq_final_pi[0] = final_dq_dqm.dq_final_pi[1];
		final_dq_dqm.dq_final_pi[1] = dq_pi;

		dq_pi = dq_final_dqm.dq_final_dqm_pi[0];
		dq_final_dqm.dq_final_dqm_pi[0] = dq_final_dqm.dq_final_dqm_pi[1];
		dq_final_dqm.dq_final_dqm_pi[1] = dq_pi;
	}

	for (ii = p->rank; ii < p->support_rank_num; ii++) {
		set_rank(p, ii);

		txdly_dq_set(p, cal_type, &final_dq_dqm);
		txdly_dqm_set(p, cal_type, &dq_final_dqm, vref_scan_enable);
		txdly_dqpi_set(p, vref_scan_enable, cal_type, &dq_final_dqm,
			&final_dq_dqm);
#if TX_K_DQM_WITH_WDBI
		if (cal_type == TX_DQ_DQS_MOVE_DQ_ONLY
			|| cal_type == TX_DQ_DQS_MOVE_DQ_DQM)
#endif
		{
			if (enable_delay_cell)
				tx_set_delay_cell(p, delay_cell_ofst);
		}

#if ENABLE_TX_TRACKING
		txdly_dqpi_txtracking_set(p,  vref_scan_enable, cal_type,
			&dq_final_dqm, &final_dq_dqm);
#endif
	}

	set_rank(p, backup_rank);

	if (vref_scan_enable) {
		set_txdq_vref_lp4(p, final_range, final_vref);
		show_msg((INFO, "Final TX Range %d Vref %d\n",
			final_range, final_vref));
	}

	show_msg((INFO, "[TxWindowPerbitCal] Done\n"));

	if (cal_type == TX_DQ_DQS_MOVE_DQM_ONLY && vref_scan_enable == 0)
		return TX_DQM_WINDOW_SPEC_IN;

	return DRAM_OK;
}
#endif /* SIMULATION_TX_PERBIT */


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
 * @file dramc_crxcal_api.c
 *  Basic DRAMC calibration API implementation
 */

/* -----------------------------------------------------------------------------
 *  Include files
 * -----------------------------------------------------------------------------
 */
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>
#include <platform/dramc_api.h>

#define VALUE_5555	0x5555

/* -----------------------------------------------------------------------------
 *  Global variables
 * -----------------------------------------------------------------------------
 */
#if SIMULATION_RX_PERBIT
#ifdef ETT
unsigned char rx_eye_scan_flag = TRUE;
#else
unsigned char rx_eye_scan_flag = FALSE;
#endif
unsigned char rx_eye_scan_only_higheset_freq_flag = 1;
static unsigned short rx_window_sum[CHANNEL_NUM];
short rx_dqs_duty_offset[CHANNEL_NUM][DQS_NUMBER][EDGE_NUMBER];
#endif
unsigned char final_rx_vref_dq[CHANNEL_NUM][RANK_MAX];
#if SIMULATION_DATLAT
static unsigned char rx_datlat_result[CHANNEL_NUM][RANK_MAX];
#endif
#if SIMULATION_RX_OFFSET
static unsigned char rx_input_buffer_delay_exchange(signed char offset)
{
	unsigned char ret;

	if (offset < 0) {
		ret = 0x8 | (-offset);
	} else {
		ret = offset;
	}

	return ret;
}

static void rx_set_dq_dqm_offset(DRAMC_CTX_T *p, unsigned char dqm_offset[],
	unsigned char dq_offset[])
{
	unsigned char bit_idx, byte_idx;
	unsigned int dq_offset_set[DQS_NUMBER] = {0};

	for (byte_idx = 0; byte_idx < p->data_width/DQS_BIT_NUM;
		byte_idx += 2) {
		unsigned char bit_start;
		DRAM_CHANNEL_T backup_channel;

		backup_channel = p->channel;
		if (p->data_width == DATA_WIDTH_32BIT)
			p->channel = CHANNEL_B;

		bit_start = byte_idx * DQS_BIT_NUM;
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DQ1),
			dqm_offset[byte_idx], B0_DQ1_RG_RX_ARDQM0_OFFC_B0);
		io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ1),
			dqm_offset[byte_idx + 1], B1_DQ1_RG_RX_ARDQM0_OFFC_B1);

		for (bit_idx = bit_start; bit_idx < bit_start + DQS_BIT_NUM;
			bit_idx++) {
			unsigned char shift;

			shift = bit_idx - bit_start;

			dq_offset_set[byte_idx] |=
				dq_offset[bit_idx] << (shift * 4);
			dq_offset_set[byte_idx+1] |=
				dq_offset[bit_idx + 8] << (shift * 4);
		}

		io32_write_4b(DRAMC_REG_ADDR(DDRPHY_B0_DQ0),
			dq_offset_set[byte_idx]);
		io32_write_4b(DRAMC_REG_ADDR(DDRPHY_B1_DQ0),
			dq_offset_set[byte_idx + 1]);

		p->channel = backup_channel;
	}
}

DRAM_STATUS_T dramc_rx_input_buffer_offset_cal(DRAMC_CTX_T *p)
{
	signed char offset;
	unsigned char dqm_offset[DQS_NUMBER];
	unsigned char dq_offset[DQ_DATA_WIDTH];
	unsigned char dq_flag_change[DQ_DATA_WIDTH];
	unsigned char dqm_flag_change[DQS_NUMBER];
	unsigned int dq_result;
	unsigned int dqm_result;
	unsigned char bit_idx, byte_idx, finish_count;

	unsigned int reg_backup_address[] = {
		DDRPHY_B0_DQ3,
		DDRPHY_B1_DQ3,
		DDRPHY_B0_DQ5,
		DDRPHY_B1_DQ5,
		DDRPHY_B0_DQ6,
		DDRPHY_B1_DQ6,

		DDRPHY_B0_DQ3 + (1 << POS_BANK_NUM),
		DDRPHY_B1_DQ3 + (1 << POS_BANK_NUM),
		DDRPHY_B0_DQ5 + (1 << POS_BANK_NUM),
		DDRPHY_B1_DQ5 + (1 << POS_BANK_NUM),
		DDRPHY_B0_DQ6 + (1 << POS_BANK_NUM),
		DDRPHY_B1_DQ6 + (1 << POS_BANK_NUM),
	};

	show_msg_with_timestamp((INFO, "start Rx InputBuffer calibration\n"));

	dramc_backup_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B0_DQ6),
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0) |
		p_fld(SET_FLD, B0_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B0));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B1_DQ6),
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1) |
		p_fld(SET_FLD, B1_DQ6_RG_RX_ARDQ_RES_BIAS_EN_B1));

	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), SET_FLD,
		B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), SET_FLD,
		B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);

	delay_us(1);

	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B0_DQ3),
		p_fld(1, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0) |
		p_fld(1, B0_DQ3_RG_RX_ARDQ_OFFC_EN_B0));
	io_32_write_fld_multi(DRAMC_REG_ADDR(DDRPHY_B1_DQ3),
		p_fld(1, B1_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B1) |
		p_fld(1, B1_DQ3_RG_RX_ARDQ_OFFC_EN_B1));

	finish_count = 0;
	for (bit_idx = 0; bit_idx < DQ_DATA_WIDTH; bit_idx++) {
		dq_flag_change[bit_idx] = 0x7f;
	}

	for (byte_idx = 0; byte_idx < DQS_NUMBER; byte_idx++) {
		dqm_flag_change[byte_idx] = 0x7f;
	}

	for (offset = -7; offset < 8; offset++)
	{
		unsigned char convert_offset;

		convert_offset = rx_input_buffer_delay_exchange(offset);

		show_msg2((INFO, "offset = %2d, convert_offset = %2d,",
			offset, convert_offset));

		/* Adjust dq & dqm offset value */
		memset(dqm_offset, convert_offset, DQS_NUMBER);
		memset(dq_offset, convert_offset, DQ_DATA_WIDTH);
		rx_set_dq_dqm_offset(p, dqm_offset, dq_offset);

		delay_us(1);

		/* Check Offset Flag */
		dq_result = io_32_read_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS0),
			MISC_PHY_RGS0_RGS_ARDQ_OFFSET_FLAG);
		dqm_result = io_32_read_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS0),
			MISC_PHY_RGS0_RGS_ARDQM0_OFFSET_FLAG);
		dqm_result |= io_32_read_fld_align(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS0),
			MISC_PHY_RGS0_RGS_ARDQM1_OFFSET_FLAG) << 1;

		if (p->data_width == DATA_WIDTH_32BIT) {
			unsigned int b23_result;

			b23_result = io_32_read_fld_align(DDRPHY_MISC_PHY_RGS0 +
				(CHANNEL_B << POS_BANK_NUM),
				MISC_PHY_RGS0_RGS_ARDQ_OFFSET_FLAG);
			dqm_result |= io_32_read_fld_align(DDRPHY_MISC_PHY_RGS0 +
				(CHANNEL_B << POS_BANK_NUM),
				MISC_PHY_RGS0_RGS_ARDQM0_OFFSET_FLAG) << 2;
			dqm_result |= io_32_read_fld_align(DDRPHY_MISC_PHY_RGS0 +
				(CHANNEL_B << POS_BANK_NUM),
				MISC_PHY_RGS0_RGS_ARDQM1_OFFSET_FLAG) << 3;

			dq_result |= (b23_result << 16);
		}
		show_msg2((INFO, "dq_result = 0x%x\n", dq_result));
		show_msg2((INFO, "dqm_result = 0x%x\n", dqm_result));

		for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
			if (dq_flag_change[bit_idx] == 0x7f) {
				unsigned char result;

				result = (dq_result >> bit_idx) & 0x1;

				if (result == 0) {
					dq_flag_change[bit_idx] = convert_offset;
					finish_count++;
				}
			}
		}

		for (byte_idx = 0; byte_idx < (p->data_width/DQS_BIT_NUM);
			byte_idx++) {
			if (dqm_flag_change[byte_idx] == 0x7f) {
				if ((dqm_result & (1 << byte_idx)) == 0) {
					dqm_flag_change[byte_idx]= convert_offset;
					finish_count++;
				}
			}
		}

		if (finish_count == (p->data_width + p->data_width/DQS_BIT_NUM)) {
			show_msg2((INFO, "%d bits finished. Early break\n",
				finish_count));
			break;
		}
	}

	/* log the result */
	show_msg2((INFO, "Final offset result:\n"));
	for (byte_idx = 0; byte_idx < (p->data_width/DQS_BIT_NUM); byte_idx++) {
		unsigned char bit_start;

		show_msg2((INFO, "DQM %d: %d\n", byte_idx,
			dqm_flag_change[byte_idx]));

		bit_start = byte_idx * DQS_BIT_NUM;
		for (bit_idx = bit_start; bit_idx < (bit_start + DQS_BIT_NUM);
			bit_idx++) {
			show_msg2((INFO, "DQ %d: %d; ", bit_idx,
				dq_flag_change[bit_idx]));

			if ((bit_idx % 4) == 3)
				show_msg2((INFO, "\n"));
		}
		show_msg2((INFO, "\n\n"));
	}

	dramc_restore_registers(p, reg_backup_address,
		sizeof(reg_backup_address) / sizeof(unsigned int));

	rx_set_dq_dqm_offset(p, dqm_flag_change, dq_flag_change);

	show_msg2((INFO, "[RxInputBuffer calibration] Done\n"));
	return DRAM_OK;
}
#endif

#if SIMULATION_RX_PERBIT
/* dramc_rx_window_perbit_cal (v2 version)
 *  start the rx dqs perbit sw calibration.
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
/* default RX vref is 0xe=14 */
#if SUPPORT_TYPE_LPDDR4
static void dramc_rx_win_rddqc_init_lp4(DRAMC_CTX_T *p)
{
	unsigned char *lpddr_phy_mapping;
	unsigned short temp_value = 0;
	unsigned char mr_golden_mr15_golden_value = 0;
	unsigned char mr_golden_mr20_golden_value = 0;
	int i;

	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7), CLEAR_FLD,
		SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ7), CLEAR_FLD,
		SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), get_rank(p),
		MRS_MRSRK);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MPC_OPTION), SET_FLD,
		MPC_OPTION_MPCRKEN);

	lpddr_phy_mapping =
		(unsigned char *)lpddr4_phy_mapping_pop[p->channel];

	for (i = 0; i < 16; i++) {
		temp_value |= ((VALUE_5555 >> i) & 0x1) << lpddr_phy_mapping[i];
	}

	if (p->channel == CHANNEL_A) {
		mr_golden_mr15_golden_value =
			(unsigned char) temp_value & BYTE_MAX;
		mr_golden_mr20_golden_value =
			(unsigned char) (temp_value >> 8) & BYTE_MAX;
	} else {
		/* If B23 is used, PHY B3 will be send to DRAMC B0,
		 * and PHY B2 will be send to DRAMC B1.
		 */
		mr_golden_mr15_golden_value =
			(unsigned char) (temp_value >> 8) & BYTE_MAX;
		mr_golden_mr20_golden_value =
			(unsigned char) temp_value & BYTE_MAX;
	}

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_MR_GOLDEN),
		p_fld(mr_golden_mr15_golden_value,
		MR_GOLDEN_MR15_GOLDEN) |
		p_fld(mr_golden_mr20_golden_value,
		MR_GOLDEN_MR20_GOLDEN));
}

/* Issue "RD DQ Calibration"
 * 1. RDDQCEN = 1 for RDDQC
 * 2. RDDQCDIS = 1 to stop RDDQC burst
 * 3. Wait rddqc_response = 1
 * 4. Read compare result
 * 5. RDDQCEN = 0
 * 6. RDDQCDIS = 0 (Stops RDDQC request)
 */
static unsigned int dramc_rx_win_rddqc_run_lp4(DRAMC_CTX_T *p)
{
	unsigned int result, response;
	unsigned int time_cnt = TIME_OUT_CNT;

	/*
	* Issue RD DQ calibration
	*  to stop RDDQC burst
	* Wait rddqc_response=1
	*/
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		SET_FLD, SPCMD_RDDQCEN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL), SET_FLD,
		SPCMDCTRL_RDDQCDIS);
	do {
		response = io_32_read_fld_align(DRAMC_REG_ADDR
			(DRAMC_REG_SPCMDRESP), SPCMDRESP_RDDQC_RESPONSE);
		time_cnt--;
		delay_us(1);
	} while ((response == 0) && (time_cnt > 0));

	if (time_cnt == 0)
		show_msg((INFO, "[RxWinRDDQC] Resp fail (time out)\n"));

	result = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_RDQC_CMP));

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), CLEAR_FLD,
		SPCMD_RDDQCEN);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL), CLEAR_FLD,
		SPCMDCTRL_RDDQCDIS);

	return result;
}

static void dramc_rx_win_rddqc_end_lp4(DRAMC_CTX_T *p)
{
	/*  Recover MPC Rank */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), CLEAR_FLD,
		MRS_MRSRK);
}
#endif

#if SUPPORT_TYPE_PCDDR4
#define RDDQC_DQ_GOLDEN			0xaa

void  dramc_rx_win_rddqc_init_ddr4(DRAMC_CTX_T *p)
{
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_DRAMC_PD_CTRL), 0,
		DRAMC_PD_CTRL_PHYCLKDYNGEN);
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_CKECTRL),
		p_fld(SET_FLD, CKECTRL_CKEFIXON) |
		p_fld(SET_FLD, CKECTRL_CKE1FIXON));
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7), 0,
		SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0);
	io_32_write_fld_align(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ7), 0,
		SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMDCTRL),
		1, SPCMDCTRL_RDDQCDIS);

	/* Enable MPR. Serial mode, from MPR0 (default: 01010101B) */
	dramc_mode_reg_write(p, 3, 0x4);
}

#define GET_BIT(_v, _n)		(((_v) >> (_n)) & 0x1)
static unsigned int dramc_rx_win_rddqc_run_ddr4(DRAMC_CTX_T *p)
{
	unsigned int mrrdata0, mrrdata1, mrrdata2, mrrdata3;
	unsigned int  result, response;
	unsigned int bit_idx;
	unsigned int timecnt = TIME_OUT_CNT;

	/* Read MPR0 */
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS),
		0, MRS_MRSBA);
	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD),
		1, SPCMD_MRREN);

	do {
		response = io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMDRESP),
			SPCMDRESP_MRR_RESPONSE);
		timecnt --;
		delay_us(1);
	} while ((response == 0) && (timecnt > 0));

	if (timecnt == 0) {
		show_msg2((INFO, "[DramcRxWinRDDQC] response fail (time out)\n"));
		return WORD_MAX;
	}

	mrrdata0 = io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRRDATA0),
		MRRDATA0_MRR_DATA0);
	mrrdata1 = io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRRDATA1),
		MRRDATA1_MRR_DATA1);
	mrrdata2 = io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRRDATA2),
		MRRDATA2_MRR_DATA2);
	mrrdata3 = io_32_read_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRRDATA3),
		MRRDATA3_MRR_DATA3);

	result = 0;

	/* Check per-bit result */
	for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
		unsigned char dq_data;

		dq_data = GET_BIT(mrrdata0, bit_idx) << 0 |
			GET_BIT(mrrdata0, bit_idx+16) << 1 |
			GET_BIT(mrrdata1, bit_idx) << 2 |
			GET_BIT(mrrdata1, bit_idx+16) << 3 |
			GET_BIT(mrrdata2, bit_idx) << 4 |
			GET_BIT(mrrdata2, bit_idx+16) << 5 |
			GET_BIT(mrrdata3, bit_idx) << 6 |
			GET_BIT(mrrdata3, bit_idx+16) << 7;

		if (dq_data != RDDQC_DQ_GOLDEN) {
			result |= (1 << bit_idx);
		}
	}

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_SPCMD), 0, SPCMD_MRREN);

	return result;
}

void  dramc_rx_win_rddqc_end_ddr4(DRAMC_CTX_T *p)
{
	/*
	 * Recover Read DBI
	 * io_32_write_fld_align((DDRPHY_SHU1_B0_DQ7),  p->DBI_R_onoff, SHU1_B0_DQ7_R_DMDQMDBI_SHU_B0);
	 * io_32_write_fld_align((DDRPHY_SHU1_B1_DQ7),  p->DBI_R_onoff, SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
	 */

	io_32_write_fld_align(DRAMC_REG_ADDR(DRAMC_REG_MRS), 0, MRS_MPCRK);
	dramc_mode_reg_write(p, 3, 0x0);
}
#endif

static inline void dramc_rx_win_rddqc_init(DRAMC_CTX_T *p)
{
	if (p->dram_type == TYPE_LPDDR4) {
	#if SUPPORT_TYPE_LPDDR4
		dramc_rx_win_rddqc_init_lp4(p);
	#endif
	} else if (p->dram_type == TYPE_PCDDR4) {
	#if SUPPORT_TYPE_PCDDR4
		dramc_rx_win_rddqc_init_ddr4(p);
	#endif
	}
}

/* Issue "RD DQ Calibration"
 * 1. RDDQCEN = 1 for RDDQC
 * 2. RDDQCDIS = 1 to stop RDDQC burst
 * 3. Wait rddqc_response = 1
 * 4. Read compare result
 * 5. RDDQCEN = 0
 * 6. RDDQCDIS = 0 (Stops RDDQC request)
 */
static inline unsigned int dramc_rx_win_rddqc_run(DRAMC_CTX_T *p)
{
	if (p->dram_type == TYPE_LPDDR4) {
	#if SUPPORT_TYPE_LPDDR4
		return dramc_rx_win_rddqc_run_lp4(p);
	#endif
	} else if (p->dram_type == TYPE_PCDDR4) {
	#if SUPPORT_TYPE_PCDDR4
		return dramc_rx_win_rddqc_run_ddr4(p);
	#endif
	}
    return 0;
}


static inline void dramc_rx_win_rddqc_end(DRAMC_CTX_T *p)
{
	if (p->dram_type == TYPE_LPDDR4) {
	#if SUPPORT_TYPE_LPDDR4
		dramc_rx_win_rddqc_end_lp4(p);
	#endif
	} else if (p->dram_type == TYPE_PCDDR4) {
	#if SUPPORT_TYPE_PCDDR4
		dramc_rx_win_rddqc_end_ddr4(p);
	#endif
	}
}


static void set_dq_output_delay_lp4(DRAMC_CTX_T *p, unsigned int value)
{
	unsigned char ii;
	/* every 2bit dq have the same delay register address */
	for (ii = 0; ii < 4; ii++) {
		io32_write_4b_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2
			+ ii * 4), value);	/* DQ0~DQ7 */
		io32_write_4b_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ2
			+ ii * 4), value);	/* DQ8~DQ15 */
	}
}

static void set_dqm_output_delay_lp4(DRAMC_CTX_T *p, signed short delay)
{
	/*  Adjust DQM output delay */
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6),
		p_fld(delay, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0)
		| p_fld(delay, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));
	io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ6),
		p_fld(delay, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1)
		| p_fld(delay, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1));
}

static void set_rx_dq_dqs_delay(DRAMC_CTX_T *p, signed short delay)
{
	unsigned int value;

	if (delay <= 0) {
		/*  Set DQS delay */
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6),
			p_fld((-delay + rx_dqs_duty_offset[p->channel][0][0]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
			p_fld((-delay + rx_dqs_duty_offset[p->channel][0][1]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0));
		io_32_write_fld_multi_all(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ6),
			p_fld((-delay + rx_dqs_duty_offset[p->channel][1][0]),
			SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_R_DLY_B1) |
			p_fld((-delay + rx_dqs_duty_offset[p->channel][1][1]),
			SHU1_R0_B1_DQ6_RK0_RX_ARDQS0_F_DLY_B1));

		dram_phy_reset(p);
	} else {
		/*  Adjust DQM output delay. */
		set_dqm_output_delay_lp4(p, delay);

		dram_phy_reset(p);

		/*  Adjust DQ output delay. */
		value = ((unsigned int) delay) | (((unsigned int) delay) << 8)
			| (((unsigned int) delay) << 16)
			| (((unsigned int) delay) << 24);

		set_dq_output_delay_lp4(p, value);
	}
}

unsigned char rx_eye_scan_flag_lp4(DRAMC_CTX_T *p,
	unsigned char vref_scan_enable)
{
		return 1;
}

static void set_dq_delay_lp4(DRAMC_CTX_T *p, unsigned char bit_idx,
	signed short delay1, signed short delay2,
	signed short delay3, signed short delay4)
{
	unsigned int bak_channel;

	bak_channel = p->channel;

	/* For data width = 32, shall write the the delay
	 * value to B2, B3, that is virtual CHANNEL_B.
	 */
	if ((p->data_width == DATA_WIDTH_32BIT) &&
		(bit_idx > 15)) {
		p->channel = CHANNEL_B;

		bit_idx -= 16;
	}

	/*  set dq delay */
	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DDRPHY_SHU1_R0_B0_DQ2 + bit_idx * 2),
		p_fld(((unsigned int)delay1),
		SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0) |
		p_fld(((unsigned int)delay1),
		SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0) |
		p_fld(((unsigned int)delay2),
		SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
		p_fld(((unsigned int)delay2),
		SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0));

	io_32_write_fld_multi(DRAMC_REG_ADDR
		(DDRPHY_SHU1_R0_B1_DQ2 + bit_idx * 2),
		p_fld(((unsigned int)delay3),
		SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1) |
		p_fld(((unsigned int)delay3),
		SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1) |
		p_fld(((unsigned int)delay4),
		SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |
		p_fld(((unsigned int)delay4),
		SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1));

	p->channel = bak_channel;
}

static unsigned char dramc_rx_window_perbit_ini(DRAMC_CTX_T *p,
	unsigned char use_test_engine)
{
	unsigned char vref_scan_enable;

	print_calibration_basic_info(p);
	show_msg3((INFO, "Start DQ dly to find pass range UseTestEngine =%d\n",
		use_test_engine));

	/*  for loop, different Vref, */
	rx_window_sum[p->channel] = 0;
	/* only apply RX Vref Scan for Rank 0 */

	/*(Rank 0 and 1 use the same Vref reg) */
	if ((use_test_engine == 1) && (p->enable_rx_scan_vref == ENABLE) &&
		(((p->rank == RANK_0)) ||
		(rx_eye_scan_flag == 1 &&
		((rx_eye_scan_only_higheset_freq_flag == 1 &&
		p->frequency == dfs_get_highest_freq(p)) ||
		rx_eye_scan_only_higheset_freq_flag == 0))))
		vref_scan_enable = 1;
	else
		vref_scan_enable = 0;

	return vref_scan_enable;
}

static unsigned int dramc_rx_win_check(DRAMC_CTX_T *p,
	PASS_WIN_DATA_T final_bitwin[], PASS_WIN_DATA_T win_per_bit[],
	signed short delay, unsigned int err_value, unsigned int finish_count,
	unsigned char vref_scan_enable, signed short delay_end)
{
	unsigned char bit_idx;
	unsigned int fail_bit = BYTE_MAX;

	/*  check fail bit ,0 ok ,others fail */
	for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
		fail_bit = err_value & ((unsigned int)1 << bit_idx);

		if (win_per_bit[bit_idx].first_pass == PASS_RANGE_NA) {
			if (fail_bit == 0)
				win_per_bit[bit_idx].first_pass = delay;
		} else if (win_per_bit[bit_idx].last_pass == PASS_RANGE_NA) {
		#if (FOR_DV_SIMULATION_USED == 1)
			/* cc notes, during Simulation, the compare result
			 * maybe 'X' in waveform, but recognized as '0' by SW,
			 * which causes fake window found. Workaround method...
			 */
			if (delay - win_per_bit[bit_idx].first_pass > 32) {
				fail_bit = 1;
			}
		#endif

			if (fail_bit != 0)
				win_per_bit[bit_idx].last_pass = (delay - 1);
			else if (delay == delay_end)
				win_per_bit[bit_idx].last_pass = delay;

			if (win_per_bit[bit_idx].last_pass != PASS_RANGE_NA) {
				if ((win_per_bit[bit_idx].last_pass -
					win_per_bit[bit_idx].first_pass)
					>= (final_bitwin[bit_idx].last_pass -
					final_bitwin[bit_idx].first_pass)) {
					if ((win_per_bit[bit_idx].last_pass -
					     win_per_bit[bit_idx].first_pass)
					    > 7)
						finish_count |= (1 << bit_idx);

					/* update bigger window size */
					final_bitwin[bit_idx].first_pass =
					    win_per_bit[bit_idx].first_pass;
					final_bitwin[bit_idx].last_pass =
					    win_per_bit[bit_idx].last_pass;
				}
				/* reset tmp window */
				win_per_bit[bit_idx].first_pass =
					PASS_RANGE_NA;
				win_per_bit[bit_idx].last_pass =
					PASS_RANGE_NA;
			}
		}

#if CALIBRATION_LOG
		{
			if (bit_idx % DQS_BIT_NUM == 0)
				show_msg3((INFO, " "));
			if (fail_bit == 0)
				show_msg3((INFO, "o"));
			else
				show_msg3((INFO, "x"));
		}
#endif
	}

#if CALIBRATION_LOG
	{
		show_msg3((INFO, " [MSB]\n"));
	}
#endif
	return finish_count;
}

static unsigned short dramc_rx_window_perbit_cal_process(DRAMC_CTX_T *p,
	unsigned char vref_scan_enable, PASS_WIN_DATA_T final_bitwin[],
	unsigned char use_test_engine, unsigned short vref_begin,
	unsigned short vref_end, unsigned short vref_step,
	signed short delay_begin, signed short delay_end,
	signed short delay_step)
{
	unsigned char bit_idx;
	unsigned int finish_count;
	unsigned short temp_win_sum;
	PASS_WIN_DATA_T win_per_bit[DQ_DATA_WIDTH];
	signed short delay = 0;
	unsigned int err_value;
	unsigned short vref, final_vref = 0xe;

	for (vref = vref_begin; vref <= vref_end; vref += vref_step) {
		if (vref_scan_enable == 1) {
			io_32_write_fld_align_all(
				DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), vref,
				SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
			io_32_write_fld_align_all(
				DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), vref,
				SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);
		} else {
			vref = io_32_read_fld_align(DRAMC_REG_ADDR
				(DDRPHY_SHU1_B0_DQ5),
				SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
		}

		temp_win_sum = 0;
		finish_count = 0;

		for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
			win_per_bit[bit_idx].first_pass =
				(signed short) PASS_RANGE_NA;
			win_per_bit[bit_idx].last_pass =
				(signed short) PASS_RANGE_NA;
			final_bitwin[bit_idx].first_pass =
				(signed short) PASS_RANGE_NA;
			final_bitwin[bit_idx].last_pass =
				(signed short) PASS_RANGE_NA;
		}

		/*  Adjust DQM output delay to 0 */
		set_dqm_output_delay_lp4(p, 0);

		/*  Adjust DQ output delay to 0 */
		set_dq_output_delay_lp4(p, 0);

		show_msg2((INFO, "RX vref=%d\n", vref));
		for (delay = delay_begin; delay <= delay_end;
			delay += delay_step) {
			set_rx_dq_dqs_delay(p, delay);

			if (use_test_engine) {
				err_value = dramc_engine2_run(p,
					TE_OP_WRITE_READ_CHECK,
					p->test_pattern);
			} else {
				err_value = dramc_rx_win_rddqc_run(p);
			}
#if CALIBRATION_LOG
			if (rx_eye_scan_flag_lp4(p, vref_scan_enable))
			{
				show_msg3((INFO, "%d, [0]", delay));
			}
#endif
			/*  check fail bit ,0 ok ,others fail */
			finish_count = dramc_rx_win_check(p, final_bitwin,
				win_per_bit, delay, err_value, finish_count,
				vref_scan_enable, delay_end);

			if (((p->data_width == DATA_WIDTH_16BIT) &&
				(finish_count == WORD_MAX)) ||
				((p->data_width == DATA_WIDTH_32BIT) &&
				(finish_count == LWORD_MAX))) {
				if (use_test_engine)
					set_calibration_result(p,
						DRAM_CALIBRATION_RX_PERBIT,
						DRAM_OK);
				else
					set_calibration_result(p,
						DRAM_CALIBRATION_RX_RDDQC,
						DRAM_OK);

				if (!rx_eye_scan_flag) {
					break; /* early break */
				}
			}
		}

		for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
			final_bitwin[bit_idx].win_size =
				final_bitwin[bit_idx].last_pass -
				final_bitwin[bit_idx].first_pass +
				(final_bitwin[bit_idx].last_pass
				== final_bitwin[bit_idx].first_pass ? 0 : 1);
			temp_win_sum += final_bitwin[bit_idx].win_size;
			/* Sum of CA Windows for vref selection */
		}

		if (temp_win_sum > rx_window_sum[p->channel]) {
			show_msg2((INFO, "%s%d, Window Sum %d > %d\n\n",
				"Better RX Vref found ", vref, temp_win_sum,
				rx_window_sum[p->channel]));

			rx_window_sum[p->channel] = temp_win_sum;
			final_vref = vref;
#ifdef ETT
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)
			ett_rx_vref = vref;
#endif
			ett_rx_min = 0xff;
#endif
			for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
				final_bitwin[bit_idx].win_center =
					(final_bitwin[bit_idx].last_pass +
					final_bitwin[bit_idx].first_pass) >> 1;
				/* window center of each DQ bit */
#ifdef ETT
				ett_rx_first[bit_idx] = final_bitwin[bit_idx].first_pass;
				ett_rx_last[bit_idx] = final_bitwin[bit_idx].last_pass;
				ett_rx_win[bit_idx] =
				final_bitwin[bit_idx].last_pass - final_bitwin[bit_idx].first_pass;
				if (ett_rx_win[bit_idx] < ett_rx_min) {
					ett_rx_min = ett_rx_win[bit_idx];
				}
#endif
				if (rx_eye_scan_flag_lp4(p, vref_scan_enable))
				{
					show_msg3((INFO,
						"Bit %d%s%d (%d ~ %d) %d\n",
						bit_idx, ", Center ",
						final_bitwin[bit_idx].
						win_center,
						final_bitwin[bit_idx].
						first_pass,
						final_bitwin[bit_idx].
						last_pass,
						final_bitwin[bit_idx].
						win_size));
				}

			}
		}

		if ((temp_win_sum < (rx_window_sum[p->channel] * 95 / 100))
		    && rx_eye_scan_flag == 0) {
			show_msg2((INFO, "\nRX Vref found %d, early break!\n", final_vref));
			vref = vref_end;
			break;	/* max vref found (95%), early break; */
		}
	}
	return final_vref;
}

static void rx_check_delay(DRAMC_CTX_T *p,
	PASS_WIN_DATA_T final_bitwin[], RX_DELAY_SET_PERBIT_T *rx_delay)
{
	unsigned char bit_idx, byte_idx;
	unsigned char bit_first, bit_last;
	unsigned short tmp_dqm_sum;
	/*
	* 3
	* As per byte, check max DQS delay in 8-bit.
	* Except for the bit of max DQS delay,
	* delay DQ to fulfill setup time = hold time
	*/
	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
	     byte_idx++) {
		tmp_dqm_sum = 0;

		bit_first = DQS_BIT_NUM * byte_idx;
		bit_last = DQS_BIT_NUM * byte_idx + DQS_BIT_NUM - 1;
		rx_delay->dqs_dly_perbyte[byte_idx] = MAX_RX_DQSDLY_TAPS;

		for (bit_idx = bit_first; bit_idx <= bit_last; bit_idx++) {
			/* find out max Center value */
			if (final_bitwin[bit_idx].win_center <
				rx_delay->dqs_dly_perbyte[byte_idx]) {
				rx_delay->dqs_dly_perbyte[byte_idx] =
					final_bitwin[bit_idx].win_center;
			}
		}
		/* Delay DQS=0, Delay DQ only */
		if (rx_delay->dqs_dly_perbyte[byte_idx] > 0) {
			rx_delay->dqs_dly_perbyte[byte_idx] = 0;
		} else {	/* Need to delay DQS */
			rx_delay->dqs_dly_perbyte[byte_idx] = -
				rx_delay->dqs_dly_perbyte[byte_idx];
		}

		/*
		* we delay DQ or DQS to let DQS sample the middle of
		* rx pass window for all the 8 bits,
		*/
		for (bit_idx = bit_first; bit_idx <= bit_last; bit_idx++) {
			final_bitwin[bit_idx].best_dqdly =
				rx_delay->dqs_dly_perbyte[byte_idx] +
				final_bitwin[bit_idx].win_center;
			tmp_dqm_sum += final_bitwin[bit_idx].best_dqdly;
		}

		/*  calculate DQM as average of 8 DQ delay */
		rx_delay->dqm_dly_perbyte[byte_idx] =
			tmp_dqm_sum / DQS_BIT_NUM;
	}

	show_msg((INFO, "%s%d%s%d\nDQM Delay:\nDQM0 = %d, DQM1 = %d\n",
			"DQS Delay:\nDQS0 = ", rx_delay->dqs_dly_perbyte[0],
			", DQS1 = ", rx_delay->dqs_dly_perbyte[1],
			rx_delay->dqm_dly_perbyte[0],
			rx_delay->dqm_dly_perbyte[1]));
	show_msg((INFO, "DQ Delay:\n"));
}

/* Process 4bit pinmux. PINMUX. REVIEW.
 * Note that currently only LPDDR3 uses 4bit mux.
 */
static void dramc_rx_win_process_pinmux(DRAMC_CTX_T *p,
	PASS_WIN_DATA_T win_per_bit[], RX_DELAY_SET_PERBIT_T *rx_delay)
{
	unsigned char *lpddr_4bitmux_byte_mapping;
	unsigned char idx;
	unsigned char idx_mapping;
	unsigned short dq_swap;

	if (p->en_4bit_mux == DISABLE)
		return;

	lpddr_4bitmux_byte_mapping = dramc_get_4bitmux_byte_mapping(p);
	if (lpddr_4bitmux_byte_mapping == NULL)
		return; /* do nothing */

	for (idx = 0; idx < 8; idx++) {
		idx_mapping = lpddr_4bitmux_byte_mapping[idx];

		/* swap */
		dq_swap = win_per_bit[idx].best_dqdly;
		win_per_bit[idx].best_dqdly =
			win_per_bit[idx_mapping].best_dqdly;
		win_per_bit[idx_mapping].best_dqdly = dq_swap;

		if (p->data_width == DATA_WIDTH_32BIT) {
			unsigned char idx_b23;

			idx_b23 = idx + 16;
			idx_mapping = lpddr_4bitmux_byte_mapping[idx_b23];

			dq_swap = win_per_bit[idx_b23].best_dqdly;
			win_per_bit[idx_b23].best_dqdly =
				win_per_bit[idx_mapping].best_dqdly;
			win_per_bit[idx_mapping].best_dqdly = dq_swap;
		}
	}
}

#define RX_ADDR_OFST	0x50
DRAM_STATUS_T dramc_rx_window_perbit_cal(DRAMC_CTX_T *p,
	unsigned char use_test_engine)
{
	unsigned char bit_idx, byte_idx;
	signed short delay_begin = 0, delay_end, delay_step = 1;
	PASS_WIN_DATA_T final_win_per_bit[DQ_DATA_WIDTH];
	RX_DELAY_SET_PERBIT_T rx_delay;
	unsigned char vref_scan_enable;
	unsigned short final_vref = 0xe;
	unsigned short vref_begin, vref_end, vref_step;

	show_msg((INFO, "\n[DramcRxWindowPerbitCal]\n"));

	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}
	memset(&rx_delay, 0x0, sizeof(RX_DELAY_SET_PERBIT_T));
	/*
	 * 1.delay DQ ,find the pass widnow (left boundary).
	 *  2.delay DQS find the pass window (right boundary).
	 *  3.Find the best DQ / DQS to satify the middle value of
	 *	the overall pass window per bit
	 *  4.Set DQS delay to the max per byte, delay DQ to de-skew
	 */
	vref_scan_enable = dramc_rx_window_perbit_ini(p, use_test_engine);

	if (vref_scan_enable) { /* 3200, 3733, 4266 */
		if (p->odt_onoff) {
			vref_begin = RX_VREF_RANGE_BEGIN;
		} else { /* 1600 */
			if (rx_eye_scan_flag == 0)
				vref_begin = RX_VREF_RANGE_BEGIN_ODTOFF;
			else
				vref_begin = 0;
		}
		vref_end = RX_VREF_RANGE_END;
		vref_step = RX_VREF_RANGE_STEP;
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), SET_FLD,
			B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), SET_FLD,
			B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);
	} else {
		vref_begin = 0;
		vref_end = 0;
		vref_step = 1;
	}
	show_msg2((INFO, "Rx Vref Scan = %d (%d~%d)\n", vref_scan_enable,
		vref_begin, vref_end));

	if (p->frequency >= DDR2666_FREQ)
		delay_begin = -MAX_RX_DQSDLY_TAPS_2666;
	else if (p->frequency >= DDR1600_FREQ)
		delay_begin = -MAX_RX_DQSDLY_TAPS_1600;
	else
		delay_begin = -MAX_RX_DQSDLY_TAPS;

	delay_end = MAX_RX_DQDLY_TAPS;
	show_msg3((INFO, "x-axis: bit #, y-axis: DQ dly (%d~%d)\n",
		delay_begin, delay_end));

#if (FOR_DV_SIMULATION_USED == 0)
		delay_step = 1;
#else
	if (p->frequency <= DDR1600_FREQ)
		delay_step = 4;
	else
		delay_step = 2;
#endif

	/*
	 * default set result fail.
	 * When window found, update the result as oK
	*/
	if (use_test_engine) {
		set_calibration_result(p, DRAM_CALIBRATION_RX_PERBIT,
			DRAM_FAIL);
		dramc_engine2_init(p, p->test2_1, p->test2_2,
			p->test_pattern, 0);
	} else {
		set_calibration_result(p, DRAM_CALIBRATION_RX_RDDQC,
			DRAM_FAIL);
		dramc_rx_win_rddqc_init(p);
	}

	final_vref = dramc_rx_window_perbit_cal_process(p, vref_scan_enable,
		final_win_per_bit, use_test_engine, vref_begin,
		vref_end, vref_step, delay_begin, delay_end, delay_step);

	if (use_test_engine)
		dramc_engine2_end(p);
	else
		dramc_rx_win_rddqc_end(p);

	/* Set RX Final Vref Here */
	if (vref_scan_enable == 1) {
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5),
			final_vref, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5),
			final_vref, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);

		show_msg3((INFO,
			"Final RX Vref %d, apply to both rank0 and 1\n\n",
			final_vref));

		final_rx_vref_dq[p->channel][p->rank] =
			(unsigned char) final_vref;
	}

	rx_check_delay(p, final_win_per_bit, &rx_delay);
	dramc_rx_win_process_pinmux(p, final_win_per_bit, &rx_delay);

	for (bit_idx = 0; bit_idx < p->data_width; bit_idx = bit_idx + 4) {
		show_msg((INFO,
			"DQ%d =%d, DQ%d =%d, DQ%d =%d, DQ%d =%d\n",
			bit_idx, final_win_per_bit[bit_idx].best_dqdly,
			bit_idx + 1, final_win_per_bit[bit_idx + 1].best_dqdly,
			bit_idx + 2, final_win_per_bit[bit_idx + 2].best_dqdly,
			bit_idx + 3, final_win_per_bit[bit_idx + 3].best_dqdly));
	}

	rx_dqs_duty_offset[p->channel][0][0] =
		rx_dqs_duty_offset[p->channel][0][1] =
		rx_dqs_duty_offset[p->channel][1][0] =
		rx_dqs_duty_offset[p->channel][1][1] = 0;

	/*  set dqs delay, (dqm delay) */
	for (byte_idx = 0; byte_idx < (p->data_width / DQS_BIT_NUM);
	     byte_idx++) {
		unsigned int bak_channel;
		unsigned char byte_sel;

		bak_channel = p->channel;
		byte_sel = byte_idx;

		if ((p->data_width == DATA_WIDTH_32BIT) &&
			(byte_idx > 1)) {
			/* For 32Bit data width, switch to
			 * Virtual CHANNEL-B to write PHY B23
			 */
			p->channel = CHANNEL_B;
			byte_sel -= 2;
		} else if (p->channel == CHANNEL_B) {
			/* The delay is in PHY part, shall be
			 * processed.
			 */
			byte_sel = (byte_idx == 0) ? 3 : 2;

			/* If it's EMI_B0, the PHY byte is PHY B2,
			 * and DRAMC byte is DRAMC B1. As the compare
			 * result is in DRAMC view, so when byte_idx = 1 (
			 * DRAMC VIEW), the delay shall be set to PHY B2 (
			 * byte 0 of B23). Same for byte_idx = 0, its delay
			 * value shall be set to PHY B3.
			 */
			byte_sel -= 2;
		}

		/* Set DQS & DQM delay */
		io_32_write_fld_multi(DRAMC_REG_ADDR
			(DDRPHY_SHU1_R0_B0_DQ6 + RX_ADDR_OFST * byte_sel),
			p_fld(((unsigned int)rx_delay.dqs_dly_perbyte[byte_idx]
			+ rx_dqs_duty_offset[p->channel][byte_idx][0]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |
			p_fld(((unsigned int)rx_delay.dqs_dly_perbyte[byte_idx]
			+ rx_dqs_duty_offset[p->channel][byte_idx][1]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
			p_fld(((unsigned int)rx_delay.dqm_dly_perbyte[byte_idx]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0) |
			p_fld(((unsigned int)rx_delay.dqm_dly_perbyte[byte_idx]),
			SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));

		p->channel = bak_channel;
	}

	dram_phy_reset(p);

	/*  set dq delay */
	for (bit_idx = 0; bit_idx < DQS_BIT_NUM; bit_idx += 2) {
		unsigned char phy_bit0_start, phy_bit1_start;

		phy_bit0_start = 0;
		phy_bit1_start = 8;

		/* For Rx perbit, since the result is read from
		 * TEST2 register, that is the view of DRAMC, thus
		 * B0 result is related to PHY B3 and B1 result is
		 * is related to PHY B2.
		 */
		if (p->channel == CHANNEL_B) {
			phy_bit0_start = 8;
			phy_bit1_start = 0;
		}

		phy_bit0_start += bit_idx;
		phy_bit1_start += bit_idx;

		set_dq_delay_lp4(p, bit_idx,
		final_win_per_bit[phy_bit0_start].best_dqdly,
		final_win_per_bit[phy_bit0_start + 1].best_dqdly,
		final_win_per_bit[phy_bit1_start].best_dqdly,
		final_win_per_bit[phy_bit1_start + 1].best_dqdly);

		/* Set B2, B3 */
		if (p->data_width == DATA_WIDTH_32BIT) {
			unsigned int bit_idx_tmp;
			bit_idx_tmp = bit_idx + 16;

			set_dq_delay_lp4(p, bit_idx_tmp,
			final_win_per_bit[bit_idx_tmp].best_dqdly,
			final_win_per_bit[bit_idx_tmp + 1].best_dqdly,
			final_win_per_bit[bit_idx_tmp + 8].best_dqdly,
			final_win_per_bit[bit_idx_tmp + 9].best_dqdly);
		}
	}

	show_msg((INFO, "[DramcRxWindowPerbitCal] Done\n"));

	return DRAM_OK;
}
#endif /* SIMULATION_RX_PERBIT */

#if SIMULATION_DATLAT
static void dle_factor_handler(DRAMC_CTX_T *p, unsigned char curr_val,
	unsigned char pip_num)
{
	unsigned char dlecg_option_start_ext2 = 0, dlecg_option_start_ext3 = 0;
	unsigned char dlecg_option_last_ext2 = 0, dlecg_option_last_ext3 = 0;

	if (curr_val < 2)
		curr_val = 2;

	/*  Datlat_dsel = datlat -1, only 1 TX pipe */
	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF1),
		p_fld(curr_val, SHU_CONF1_DATLAT) |
		p_fld(curr_val - 3, SHU_CONF1_DATLAT_DSEL) |
		p_fld(curr_val - 4, SHU_CONF1_DATLAT_DSEL_PHY));

	if (p->frequency >= DDR3200_FREQ)
		dlecg_option_start_ext2 = 1;

	if (curr_val >= 24)
		dlecg_option_last_ext2 = dlecg_option_last_ext3 = 1;
	else if (curr_val >= 18)
		dlecg_option_last_ext2 = 1;

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_SHU_PIPE),
		p_fld(SET_FLD, SHU_PIPE_READ_START_EXTEND1)
		| p_fld(SET_FLD, SHU_PIPE_DLE_LAST_EXTEND1)
		| p_fld(dlecg_option_start_ext2,
		SHU_PIPE_READ_START_EXTEND2)
		| p_fld(dlecg_option_last_ext2,
		SHU_PIPE_DLE_LAST_EXTEND2)
		| p_fld(dlecg_option_start_ext3,
		SHU_PIPE_READ_START_EXTEND3)
		| p_fld(dlecg_option_last_ext3,
		SHU_PIPE_DLE_LAST_EXTEND3));

	dram_phy_reset(p);
}

unsigned char dramc_rxdatlat_scan(DRAMC_CTX_T *p,
	DRAM_DATLAT_CALIBRATION_TYTE_T use_rxtx_scan)
{
	unsigned char ii;
	unsigned int prv_register_080;
	unsigned int err_value = LWORD_MAX;
	unsigned char first, begin, sum, best_step, pipe_num = 0;
	unsigned short datlat_begin;

	/* error handling */
	if (!p) {
		show_err("context NULL\n");
		return DRAM_FAIL;
	}

	show_msg((INFO, "\n[DATLAT]\n"
		"Freq=%d, CH%d RK%d, use_rxtx_scan=%d\n\n",
		p->frequency, p->channel, p->rank, use_rxtx_scan));

	/*  pre-save */
	/*  0x07c[6:4]   DATLAT bit2-bit0 */
	prv_register_080 = io32_read_4b(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF1));

	/*  init best_step to default */
	best_step = (unsigned char) io_32_read_fld_align
		(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF1), SHU_CONF1_DATLAT);
	show_msg2((INFO, "DATLAT Default: 0x%x\n", best_step));

	/*
	 *  1.set DATLAT 0-15
	 *  2.enable engine1 or engine2
	 *  3.check result  ,3~4 taps pass
	 *  4.set DATLAT 2nd value for optimal
	 */

	/* Initialize */
	first = BYTE_MAX;
	begin = 0;
	sum = 0;

	dramc_engine2_init(p, p->test2_1, p->test2_2, p->test_pattern, 0);
	datlat_begin = best_step - 4;

	for (ii = datlat_begin; ii < DATLAT_TAP_NUMBER; ii++) {
		/* 1 */
		dle_factor_handler(p, ii, pipe_num);

		/* 2 */
		if (use_rxtx_scan == fcDATLAT_USE_DEFAULT) {
			err_value = dramc_engine2_run(p,
				TE_OP_WRITE_READ_CHECK, p->test_pattern);
		}
		/* 3 */
		if (err_value == 0) {
			if (begin == 0) { /* first tap which is pass */
				first = ii;
				begin = 1;
			}
			if (begin == 1) {
				sum++;
				if (sum > 4) /* early break. */
					break;
			}
		} else if (err_value != 0 && sum <= 1) {
			first = BYTE_MAX;
			begin = 0;
			sum = 0;
		} else {
			if (begin == 1) {
				/* pass range end */
				begin = BYTE_MAX;
			}
		}

		show_msg2((INFO, "%d, 0x%X, sum=%d\n", ii, err_value, sum));
	}

	dramc_engine2_end(p);

	/* 4 */
	if (sum == 0)
		show_err("no DATLAT taps pass, DATLAT calibration fail!\n");
	else if (sum <= 3)
		best_step = first + (sum >> 1);
	else /* window is larger htan 3 */
#if SUPPORT_TYPE_LPDDR3
		best_step = first + 1;
#else
		best_step = first + 2;
#endif
	rx_datlat_result[p->channel][p->rank] = best_step;

	show_msg((INFO,
		"pattern=%d first_step=%d total pass=%d best_step=%d\n",
		p->test_pattern, first, sum, best_step));

	if (sum < 4) {
		show_msg2((INFO, "[NOTICE] CH%d, DatlatSum %d\n",
			 p->channel, sum));
	}

	if (sum == 0) {
		show_err("DATLAT calibration fail, write back to default values!\n");
		io32_write_4b(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF1),
			prv_register_080);
		set_calibration_result(p, DRAM_CALIBRATION_DATLAT, DRAM_FAIL);
	} else {
		dle_factor_handler(p, best_step, pipe_num);
		set_calibration_result(p, DRAM_CALIBRATION_DATLAT, DRAM_OK);
	}

	io_32_write_fld_multi(DRAMC_REG_ADDR(DRAMC_REG_PADCTRL),
		p_fld(SET_FLD, PADCTRL_DQIENQKEND) | p_fld(SET_FLD,
		PADCTRL_DQIENLATEBEGIN));

	show_msg3((INFO, "[DramcRxdatlatCal] Done\n"));

	return sum;
}

void dramc_rxdatlat_cal(DRAMC_CTX_T *p)
{
	show_msg3((INFO, "start Rx DatLat cal\n"));
	dramc_rxdatlat_scan(p, fcDATLAT_USE_DEFAULT);
}

DRAM_STATUS_T dramc_dual_rank_rxdatlat_cal(DRAMC_CTX_T *p)
{
	unsigned char final_datlat, datlat0, datlat1;

	datlat0 = rx_datlat_result[p->channel][0];
	datlat1 = rx_datlat_result[p->channel][1];

	if (datlat0 > datlat1)
		final_datlat = datlat0;
	else
		final_datlat = datlat1;

	dle_factor_handler(p, final_datlat, 3);
	show_msg((INFO,
		"[DualRankRxdatlatCal] RK0: %d, RK1: %d, Final_Datlat %d\n",
		datlat0, datlat1, final_datlat));

	return DRAM_OK;

}
#endif /* SIMULATION_DATLAT */


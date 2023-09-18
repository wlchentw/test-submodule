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
 * ==========================================================================
 *   Include Files
 * ==========================================================================
 */
#include <platform/dramc_common.h>
#include <platform/dramc_api.h>
#include <platform/x_hal_io.h>
#include <platform/mtk_wdt.h>
#include <platform/reg_emi_reg.h>
#include <platform/mt_reg_base.h>

#if (FOR_DV_SIMULATION_USED == 0)
#include <platform/emi.h>
#endif

#if (FOR_DV_SIMULATION_USED == 0)
#if USE_PMIC_CHIP_MT6355
#include <platform/pmic.h>
#endif
#endif

#define sys_print(...)

#ifdef ETT
unsigned int ett_drv[4];
unsigned int ett_rx_min, ett_tx_min;

#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)
unsigned int ett_rx_vref, ett_tx_vref;
#endif

#if SUPPORT_TYPE_LPDDR3
unsigned int ett_cbt_win[CATRAINING_NUM_LP3];
#elif SUPPORT_TYPE_LPDDR4
unsigned int ett_cbt_win[CATRAINING_NUM_LP4];
#endif

#if (SUPPORT_PCDDR3_32BIT || SUPPORT_TYPE_LPDDR3)
unsigned int ett_rx_win[DATA_WIDTH_32BIT], ett_rx_first[DATA_WIDTH_32BIT], ett_rx_last[DATA_WIDTH_32BIT];
unsigned int ett_tx_win[DATA_WIDTH_32BIT], ett_tx_first[DATA_WIDTH_32BIT], ett_tx_last[DATA_WIDTH_32BIT];
#else
unsigned int ett_rx_win[DATA_WIDTH_16BIT], ett_rx_first[DATA_WIDTH_16BIT], ett_rx_last[DATA_WIDTH_16BIT];
unsigned int ett_tx_win[DATA_WIDTH_16BIT], ett_tx_first[DATA_WIDTH_16BIT], ett_tx_last[DATA_WIDTH_16BIT];
#endif
#endif

#if DVFS_EN
unsigned int dvfs_lp4_cal_setting_addr[DVFS_LP4_ADDR_NUM]= { //caodan
		((DDRPHY_SHU1_R0_CA_CMD9)), //clk/cs/cmd. -phy all 1

		((DRAMC_REG_SHURK0_DQSIEN)),//gating fine 5
		((DRAMC_REG_SHURK0_SELPH_DQSG0)), //gating large ui
		((DRAMC_REG_SHURK0_SELPH_DQSG1)), //gating small ui
		((DRAMC_REG_SHURK0_SELPH_ODTEN0)), //gating rodt large ui
		((DRAMC_REG_SHURK0_SELPH_ODTEN1)), //gating rodt small ui

		((DRAMC_REG_SHU_CONF1)), //dle 2
		((DRAMC_REG_SHU_PIPE)), //dle

		((DDRPHY_SHU1_R0_B0_DQ6)), //rx dqs, dqm. -phy all 10
		((DDRPHY_SHU1_R0_B1_DQ6)), //rx dqs, dqm. -phy all
		((DDRPHY_SHU1_R0_B0_DQ2)), //rx dq start. -phy all
		((DDRPHY_SHU1_R0_B0_DQ3)),
		((DDRPHY_SHU1_R0_B0_DQ4)),
		((DDRPHY_SHU1_R0_B0_DQ5)),
		((DDRPHY_SHU1_R0_B1_DQ2)),
		((DDRPHY_SHU1_R0_B1_DQ3)),
		((DDRPHY_SHU1_R0_B1_DQ4)),
		((DDRPHY_SHU1_R0_B1_DQ5)), //rx dq end

		((DRAMC_REG_SHU_SELPH_DQS0)), //tx dqs large ui 6
		((DRAMC_REG_SHU_SELPH_DQS1)), //tx dqs small ui
		((DRAMC_REG_SHURK0_SELPH_DQ0)), //tx dq large ui
		((DRAMC_REG_SHURK0_SELPH_DQ2)), //tx dq small ui
		((DRAMC_REG_SHURK0_SELPH_DQ1)), //tx dqm large ui
		((DRAMC_REG_SHURK0_SELPH_DQ3)), //tx dqm small ui

		((DDRPHY_SHU1_R0_B0_DQ7)), //tx dq,dqm,dqs pi. -phy all 4
		((DDRPHY_SHU1_R0_B1_DQ7)), //tx dq,dqm,dqs pi. -phy all
		((DDRPHY_SHU1_R0_B1_DQ0)), //tx dq delay cell. -phy all
		((DDRPHY_SHU1_R0_B0_DQ0)), //tx dq delay cell. -phy all

		((DRAMC_REG_SHU_CONF2)), //different initial setting 10
		((DRAMC_REG_SHU_RANKCTL)),
		((DRAMC_REG_SHU_CKECTRL)),
		((DRAMC_REG_SHU1_DQSOSC_PRD)),
		((DRAMC_REG_SHU1RK0_PI)),
		((DRAMC_REG_SHU1RK0_DQSOSC)),
		((DDRPHY_SHU1_B0_DQ5)),
		((DDRPHY_SHU1_B0_DQ7)),
		((DDRPHY_SHU1_B1_DQ5)),
		((DDRPHY_SHU1_B1_DQ7)),

		((DDRPHY_B0_CKGEN_DLL0)), // pll setting 4
		((DDRPHY_CA_CKGEN_DLL0)),
		((DDRPHY_B0_CKGEN_DLL1)),
		((DDRPHY_CA_CKGEN_DLL1)),

		((DRAMC_REG_SHU_ACTIM0)), // AC timing 14
		((DRAMC_REG_SHU_ACTIM1)),
		((DRAMC_REG_SHU_ACTIM2)),
		((DRAMC_REG_SHU_ACTIM3)),
		((DRAMC_REG_SHU_ACTIM4)),
		((DRAMC_REG_SHU_ACTIM5)),
		((DRAMC_REG_SHU_ACTIM6)),
		((DRAMC_REG_SHU_ACTIM_XRT)),
		((DRAMC_REG_SHU_AC_TIME_05T)),
		((DRAMC_REG_CATRAINING1)),
		((DRAMC_REG_SHURK0_DQSCTL)),
		((DRAMC_REG_SHURK1_DQSCTL)),
		((DRAMC_REG_SHU_ODTCTRL)),
		((DRAMC_REG_SHU_SCINTV)),
	};

unsigned int dvfs_lp4_high_freq_cal_setting[DVFS_LP4_ADDR_NUM];
unsigned int dvfs_lp4_low_freq_cal_setting[DVFS_LP4_ADDR_NUM];
#endif


#define PATTERN_5A5A		0x5a5a
#define TX_SHIFT_OFFSET	8
void assertion_failed(const char *function, const char *file, int line)
{
	sys_print("!!! DEBUG ASSERTION FAILED !!!\n"
		"[Function] %s\n [File] %s\n [Line] %d\n",
		function, file, line);
	while
		(1);
}

#define ASSERT(expr)							\
	do {								\
		if (!(expr))						\
		assertion_failed(__func__, __FILE__, __LINE__);	\
	} while (0)							\

/*
 * ==========================================================================
 *   Global Variables
 * ==========================================================================
 */
DRAM_DFS_FREQUENCY_TABLE_T freq_tbl[DRAM_DFS_SHUFFLE_MAX] = {
	{DDR_DDR2400, DDR2400_FREQ, DRAM_DFS_SHUFFLE_1},
#if 0 /* cc mark since Magnolia only support one shuffle */
	{LP4_DDR1600, LP4_DDR1600_FREQ, DRAM_DFS_SHUFFLE_2},
	{LP4_DDR3733, LP4_DDR3733_FREQ, DRAM_DFS_SHUFFLE_3},
	{LP4_DDR4266, LP4_DDR4266_FREQ, DRAM_DFS_SHUFFLE_4},
#endif
};

DRAMC_CTX_T *ps_curr_dram_ctx;

#if (FOR_DV_SIMULATION_USED == 0)
/* Real chip LP4 */
DRAMC_CTX_T DramCtx_LPDDR4 = {
	CHANNEL_SINGLE,
	CHANNEL_B,
	RANK_SINGLE,
	RANK_0,
#if DVFS_EN
	DVFS_LOW_FREQ,
#else
	DDR_DDR2400,
#endif
	DRAM_DFS_SHUFFLE_1,
	TYPE_LPDDR4,
	FSP_0,
	ODT_OFF,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_16BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR1600_FREQ,
	DDR1600_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE, /* CBT VREF K*/
#ifdef ETT
	ENABLE,
	ENABLE,
#else
	ENABLE, /* RX VREF K*/
	ENABLE, /* TX VREF K*/
#endif
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

/* Real chip LP3 */
DRAMC_CTX_T DramCtx_LPDDR3 = {
	CHANNEL_SINGLE,
	CHANNEL_A,
	RANK_SINGLE,
	RANK_0,
	DDR_DDR1866,
	DRAM_DFS_SHUFFLE_1,
	TYPE_LPDDR3,
	FSP_0,
	ODT_OFF,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_32BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR1866_FREQ,
	DDR1866_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE, /* CBT VREF K*/
	DISABLE, /* RX VREF K*/
	DISABLE, /* TX VREF K*/
	DISABLE, /* SSC */
	ENABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

/* Real chip PC4 */
DRAMC_CTX_T DramCtx_PCDDR4 = {
	CHANNEL_SINGLE,
	CHANNEL_A,
	RANK_SINGLE,
	RANK_0,
	DDR_DDR2400,
	DRAM_DFS_SHUFFLE_1,
	TYPE_PCDDR4,
	FSP_0,
	ODT_OFF,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_16BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR1600_FREQ,
	DDR1600_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE, /* CBT VREF K*/
#ifdef ETT
	ENABLE,
	ENABLE,
#else
	DISABLE, /* RX VREF K*/
	DISABLE, /* TX VREF K*/
#endif
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

/* Real chip PC3 */
DRAMC_CTX_T DramCtx_PCDDR3 = {
	CHANNEL_SINGLE,
#if SUPPORT_PCDDR3_32BIT
	CHANNEL_A,
#else
	CHANNEL_B,
#endif
	RANK_SINGLE,
	RANK_0,
#if SUPPORT_PCDDR3_32BIT
	DDR_DDR1866,
#else
	DDR_DDR1066,
#endif
	DRAM_DFS_SHUFFLE_1,
	TYPE_PCDDR3,
	FSP_0,
	ODT_ON,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
	{DBI_OFF, DBI_OFF},
#if SUPPORT_PCDDR3_32BIT
	DATA_WIDTH_32BIT,
#else
	DATA_WIDTH_16BIT,
#endif
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR1600_FREQ,
	DDR1600_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE, /* CBT VREF K*/
	DISABLE, /* RX VREF K*/
	DISABLE, /* TX VREF K*/
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

#else /* For DV simulation used context */
DRAMC_CTX_T DramCtx_LPDDR4 = {
	CHANNEL_SINGLE,
	CHANNEL_B,
	RANK_SINGLE,
	RANK_0,
	DDR_DDR2400,
	DRAM_DFS_SHUFFLE_1,
	TYPE_LPDDR4,
	FSP_0,
	ODT_ON,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_16BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR2400_FREQ,
	DDR2400_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE, /* CBT VREF K*/
	DISABLE, /* RX VREF K*/
	DISABLE, /* TX VREF K*/
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

DRAMC_CTX_T DramCtx_PCDDR3 = {
	CHANNEL_SINGLE,
#if SUPPORT_PCDDR3_32BIT
	CHANNEL_A,
#else
	CHANNEL_B,
#endif
	RANK_SINGLE,
	RANK_0,
	DDR_DDR1600,
	DRAM_DFS_SHUFFLE_1,
	TYPE_PCDDR3,
	FSP_0,
	ODT_ON,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
#if SUPPORT_PCDDR3_32BIT
	DATA_WIDTH_32BIT,
#else
	DATA_WIDTH_16BIT,
#endif
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR2400_FREQ,
	DDR2400_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE,
	DISABLE,
	DISABLE,
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

DRAMC_CTX_T DramCtx_LPDDR3 = {
	CHANNEL_SINGLE,
	CHANNEL_A,
	RANK_SINGLE,
	RANK_0,
	DDR_DDR1600,
	DRAM_DFS_SHUFFLE_1,
	TYPE_LPDDR3,
	FSP_0,
	ODT_OFF,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_32BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR2400_FREQ,
	DDR2400_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE,
	DISABLE,
	DISABLE,
	DISABLE, /* SSC */
	ENABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0} },
	{ {0, 0}, {0, 0} },
#endif
	{ {0, 0}, {0, 0} },
	{ {FALSE, FALSE}, {FALSE, FALSE} },
};

DRAMC_CTX_T DramCtx_PCDDR4 = {
	CHANNEL_SINGLE,
	CHANNEL_A,
	RANK_SINGLE,
	RANK_0,
	DDR_DDR2667,
	DRAM_DFS_SHUFFLE_1,
	TYPE_PCDDR4,
	FSP_0,
	ODT_ON,
	{CBT_NORMAL_MODE, CBT_NORMAL_MODE},
	{DBI_OFF, DBI_OFF},
#if ENABLE_WRITE_DBI
	{DBI_OFF, DBI_ON},
#else
	{DBI_OFF, DBI_OFF},
#endif
	DATA_WIDTH_16BIT,
	DEFAULT_TEST2_1_CAL,
	DEFAULT_TEST2_2_CAL,
	TEST_XTALK_PATTERN,
	DDR2400_FREQ,
	DDR2400_FREQ,
	{0},
	0x88,
	0,
	0,
	{0, 0},
	0,
	0,
	DISABLE,
	DISABLE,
	DISABLE,
	DISABLE, /* SSC */
	DISABLE, /* 4BIT MUX */
#if PRINT_CALIBRATION_SUMMARY
	{ {0, 0}, {0, 0}},
	{ {0, 0}, {0, 0}},
#endif
	{ {0, 0}, {0, 0}},
	{ {FALSE, FALSE}, {FALSE, FALSE}},
};

#endif

DRAMC_CTX_T *dramc_ctx_lp4 = &DramCtx_LPDDR4;
DRAMC_CTX_T *dramc_ctx_lp3 = &DramCtx_LPDDR3;
DRAMC_CTX_T *dramc_ctx_ddr4 = &DramCtx_PCDDR4;
DRAMC_CTX_T *dramc_ctx_ddr3 = &DramCtx_PCDDR3;

#ifdef DDR_RESERVE_MODE
unsigned int g_ddr_reserve_enable = 0;
unsigned int g_ddr_reserve_success = 0;
#endif

unsigned char gfirst_init_flag = FALSE;
unsigned char get_mdl_used_flag_value = FALSE;
#ifdef ENABLE_MIOCK_JMETER
unsigned char pre_miock_jmeter_hqa_used_flag = FALSE;
#endif

/*
 * LP4 table
 * LPDDR4(X) SDRAM 16-bit 4266Mbps @ 0.8V SRAM 0.9 voltage
 * LPDDR4(X) SDRAM 16-bit 3733Mbps @ 0.8V SRAM 0.9 voltage
 * LPDDR4(X) SDRAM 16-bit 3200Mbps @ 0.7V SRAM 0.85 voltage
 * LPDDR4(X) SDRAM 16-bit 1600Mbps @ 0.6V SRAM 0.85 voltage
 * Init ->1600 -> 3733 -> 4266 -> 3200
 * 0.8 -> 0.65 ->  0.8   ->  0.8  -> 0.75
 */
void set_vcore_by_freq(DRAMC_CTX_T *p)
{
#if (FOR_DV_SIMULATION_USED == 0) /* cc notes: confirm for REAL CHIP */
#if USE_PMIC_CHIP_MT6355
	if (p->frequency >= DDR4266_FREQ) {
		show_msg((INFO, "Set VCORE2 to 0.8V\n"));
		/* VCORE_SRAM=0.9v, VCORE2=0.8v, unit: uV */
		set_buck_voltage(BUCK_VCORE, 900000);
		set_buck_voltage(BUCK_VGPU, 800000);
	} else if (p->frequency >= DDR3733_FREQ) {
		show_msg((INFO, "Set VCORE2 to 0.8V\n"));
		/* VCORE_SRAM=0.95v, VCORE2=0.8v, unit: uV */
		set_buck_voltage(BUCK_VCORE, 900000);
		set_buck_voltage(BUCK_VGPU, 800000);
	} else if (p->frequency >= DDR3200_FREQ) {
		show_msg((INFO, "Set VCORE2 to 0.75V\n"));
		/* VCORE_SRAM=0.85v, VCORE2=0.75v, unit: uV */
		set_buck_voltage(BUCK_VGPU, 750000);
		set_buck_voltage(BUCK_VCORE, 850000);
	} else {
		show_msg((INFO, "Set VCORE2 to 0.65V\n"));
		/* VCORE_SRAM=0.85v, VCORE2=0.65v, unit: uV */
		set_buck_voltage(BUCK_VGPU, 650000);
		set_buck_voltage(BUCK_VCORE, 850000);
	}
#endif
#endif
}

#if CPU_RW_TEST_AFTER_K
void mem_test_address_calculation(DRAMC_CTX_T *p, unsigned int src_addr,
	unsigned int *pu4_dest)
{
	*pu4_dest = src_addr + p->ranksize[RANK_0];
}

#define new_addr(cnt, addr)	*(volatile unsigned int *)(cnt + addr)
void dram_cpu_read_write_test_after_calibration(DRAMC_CTX_T *p)
{
	unsigned char dump_info = 0, rank_idx;
	unsigned int len, count, fixed_addr, rank_addr[RANK_MAX];
	unsigned int pass_count, err_count;

	len = 0x8000000;
	rank_addr[0] = DRAM_BASE_VIRT;

	mem_test_address_calculation(p, DDR_BASE, &rank_addr[1]);

	for (rank_idx = 0; rank_idx < p->support_rank_num; rank_idx++) {
		dump_info = 0;
		err_count = 0;
		pass_count = 0;

		if (rank_idx >= 1)
			continue;
		fixed_addr = rank_addr[rank_idx];

		for (count = 0; count < len; count += 4)
			new_addr(count, fixed_addr) =
				count + (PATTERN_5A5A << 16);

		for (count = 0; count < len; count += 4) {
			if (new_addr(count, fixed_addr) !=
				count + (PATTERN_5A5A << 16))
				err_count++;
			else
				pass_count++;
		}

		if (err_count) {
			show_err2("[MEM_TEST] Rank %d Fail.",
				rank_idx);
			dump_info = 1;
		} else {
			show_msg3((INFO, "[MEM_TEST] Rank %d OK.",
				rank_idx));
		}
		show_msg3((INFO,
			"(FixedAddr 0x%X, Pass count =%d, Fail count =%d)\n",
			fixed_addr, pass_count, err_count));
	}

	//if (dump_info)
	//	dramc_dump_debug_info(p);
}
#endif

void set_mdl_used_flag(unsigned char value)
{
	get_mdl_used_flag_value = value;
}

unsigned char get_mdl_used_flag(void)
{
	return get_mdl_used_flag_value;
}

#if TX_K_DQM_WITH_WDBI
void switch_write_dbi_settings(DRAMC_CTX_T *p, unsigned char on_off)
{
	signed char tx_shift_ui;

	tx_shift_ui = (on_off) ? -TX_SHIFT_OFFSET : TX_SHIFT_OFFSET;
	/* Tx DQ/DQM -1 MCK for write DBI ON */
	dramc_write_minus_1mck_for_write_dbi(p, tx_shift_ui);

	set_dram_mode_reg_for_write_dbi_on_off(p, on_off);
	dramc_write_dbi_on_off(p, on_off);
}
#endif

static void __do_dramc_calibration(DRAMC_CTX_T *p)
{
	unsigned char rank_max;
	signed char rank_idx;

#if TX_K_DQM_WITH_WDBI
	unsigned char tx_vref_dq;
#endif
#if TX_K_DQM_WITH_WDBI
	unsigned char tx_vref_dqm, tx_final_vref = BYTE_MAX, dqm_spec_result;
#endif
	rank_max = (p->support_rank_num == RANK_DUAL) ? (RANK_MAX) : RANK_1;

	for (rank_idx = RANK_0; rank_idx < rank_max; rank_idx++) {
		set_rank(p, rank_idx);
	#if SIMULATION_WRITE_LEVELING
		dramc_write_leveling((DRAMC_CTX_T *) p);
	#endif
		/*
		 * when doing gating, RX and TX calibration,
		 * auto refresh should be enable
		 */
		auto_refresh_switch(p, ENABLE);

	#if SIMULATION_GATING
		dramc_rx_dqs_gating_cal(p);
	#endif

	#if SIMULATION_RX_PERBIT
		if (p->dram_type == TYPE_LPDDR4 || p->dram_type == TYPE_PCDDR4)
			dramc_rx_window_perbit_cal((DRAMC_CTX_T *) p, DISABLE);
		else
			dramc_rx_window_perbit_cal((DRAMC_CTX_T *) p, ENABLE);
	#endif

#if SIMULATION_TX_PERBIT
#if TX_K_DQM_WITH_WDBI
		dramc_write_dbi_on_off(p, DBI_OFF);
#endif
		dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
			TX_DQ_DQS_MOVE_DQ_DQM, FALSE);

#if TX_K_DQM_WITH_WDBI
		tx_vref_dq = dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
			TX_DQ_DQS_MOVE_DQ_ONLY, p->enable_tx_scan_vref);
#else
		dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
			TX_DQ_DQS_MOVE_DQ_ONLY, p->enable_tx_scan_vref);
#endif

#if TX_K_DQM_WITH_WDBI
		if ((p->dbi_w_onoff[p->dram_fsp]) == DBI_ON) {
			show_log("[TX_K_DQM_WITH_WDBI] K DQM with DBI_ON ");
			show_log("and check DQM window spec.\n\n");
			switch_write_dbi_settings(p, DBI_ON);
			dqm_spec_result =
				dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
				TX_DQ_DQS_MOVE_DQM_ONLY, FALSE);

			if (dqm_spec_result == TX_DQM_WINDOW_SPEC_IN) {
				switch_write_dbi_settings(p, DBI_OFF);
			} else { /* Scan DQM + Vref */
				show_log("K DQM and Vref with DBI_ON\n\n");
				tx_vref_dqm = dramc_tx_window_perbit_cal(
					(DRAMC_CTX_T *) p,
					TX_DQ_DQS_MOVE_DQM_ONLY, TRUE);

				tx_final_vref = (tx_vref_dq + tx_vref_dqm) >> 1;

				dramc_tx_set_vref(p, 0, tx_final_vref);
				show_msg((INFO, "tx_vref_dq=%d,", tx_vref_dq));
				show_msg((INFO,
					" tx_vref_dqm %d, Set MR14=%d\n\n",
					 tx_vref_dqm, tx_final_vref));

				show_log("Scan DQM\n\n");
				dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
					TX_DQ_DQS_MOVE_DQM_ONLY, FALSE);

				/* Write DBI off + Scan DQ */
				show_log("Write DBI off + Scan DQ\n\n");
				switch_write_dbi_settings(p, DBI_OFF);
				dramc_tx_window_perbit_cal((DRAMC_CTX_T *) p,
					TX_DQ_DQS_MOVE_DQ_ONLY, FALSE);
			}
		}
#endif
#endif /* SIMULATION_TX_PERBIT */

	#if SIMULATION_DATLAT
	dramc_rxdatlat_cal((DRAMC_CTX_T *) p);
	#endif

	#if SIMULATION_RX_PERBIT
		if (p->dram_type == TYPE_LPDDR4 || p->dram_type == TYPE_PCDDR4)
			dramc_rx_window_perbit_cal((DRAMC_CTX_T *) p, ENABLE);
	#endif

		/* cc notes, for non-lp4 type, calibrate TX OE may cause
		 * model enter Error state which will lead to simulation
		 * FAIL.
		 * For chip, this issue may not exist, that is SW can try
		 * to calibrate TX OE for all types.
		 */
		//if (p->dram_type == TYPE_LPDDR4)
		//	dramc_tx_oe_calibration(p);

		/*
		* After gating, Rx and Tx calibration,
		* auto refresh should be disable
		*/
		auto_refresh_switch(p, DISABLE);
#if ENABLE_TX_TRACKING
		if (p->dram_type == TYPE_LPDDR4) {
			dramc_dqsosc_auto(p);
			dramc_dqsosc_mr23(p);
			dramc_dqsosc_set_mr18_mr19(p);
		}
#endif
	}
}

static void do_dramc_calibration(DRAMC_CTX_T *p)
{
	unsigned char rank_max;
	signed char rank_idx;

#if GATING_ADJUST_TXDLY_FOR_TRACKING
	dramc_rxdqs_gating_pre_process(p);
#endif

	rank_max = (p->support_rank_num == RANK_DUAL) ? (RANK_MAX) : RANK_1;

#if SIMULATION_CBT
	if ((p->dram_type == TYPE_LPDDR4) ||
		(p->dram_type == TYPE_LPDDR3)) {
		for (rank_idx = RANK_0; rank_idx < rank_max; rank_idx++) {
			set_rank(p, rank_idx);
			cmd_bus_training(p);
		}
	}
	set_rank(p, RANK_0);
#endif

#if DUAL_FREQ_K
	no_parking_on_clrpll(p);
#endif

	__do_dramc_calibration(p);

	set_rank(p, RANK_0);

#if ENABLE_TX_TRACKING
	if (p->dram_type == TYPE_LPDDR4)
		dramc_dqsosc_shu_settings(p);
#endif

#if GATING_ADJUST_TXDLY_FOR_TRACKING
	dramc_rxdqs_gating_post_process(p);
#endif

	if (p->support_rank_num == RANK_DUAL)
		dramc_dual_rank_rxdatlat_cal(p);

}

static void dram_calibration_single_channel(DRAMC_CTX_T *p)
{
	do_dramc_calibration(p);
}

static void dram_calibration_all_channel(DRAMC_CTX_T *p)
{
#if (ENABLE_WRITE_DBI)
	unsigned char rank_idx;
#endif

	unsigned char channel_idx;

	for (channel_idx = CHANNEL_A; channel_idx < p->support_channel_num;
		channel_idx++) {
		/*
		 * when switching channel,
		 * must update PHY to Channel Mapping
		 */
		show_msg3((INFO, "cal_all_channel: cal CH%d\n", channel_idx));

		set_phy_2_channel_mapping(p, channel_idx);
		dram_calibration_single_channel(p);
	}

	set_phy_2_channel_mapping(p, CHANNEL_A);

#if PRINT_CALIBRATION_SUMMARY
	print_calibration_result(p);
#endif

#if ENABLE_WRITE_DBI
	for (channel_idx = CHANNEL_A; channel_idx < p->support_channel_num;
		channel_idx++) {
		set_phy_2_channel_mapping(p, channel_idx);

		for (rank_idx = RANK_0;
			rank_idx < p->support_rank_num; rank_idx++) {
			set_rank(p, rank_idx);

			dramc_write_minus_1mck_for_write_dbi(p, -8);

			set_dram_mode_reg_for_write_dbi_on_off(p,
				p->dbi_w_onoff[p->dram_fsp]);
		}
		set_rank(p, RANK_0);
	}
	set_phy_2_channel_mapping(p, CHANNEL_A);

	dramc_write_dbi_on_off(p, p->dbi_w_onoff[p->dram_fsp]);

	apply_write_dbi_power_improve(p, ENABLE);
#endif
}

int get_dram_info_after_cal_by_mrr(DRAMC_CTX_T *p,
	DRAM_INFO_BY_MRR_T *dram_info)
{
	unsigned char ch_idx, rank_idx;
	unsigned short density;
	unsigned long long size = 0;
	unsigned long long channel_size = 0;

	show_log("[get_dram_info_after_cal_by_mrr]\n");
	set_phy_2_channel_mapping(p, CHANNEL_A);

	dramc_mrr_by_rank(p, RANK_0, MR05, &(p->vendor_id));
	show_msg((INFO, "Vendor %x.\n", p->vendor_id));

	dramc_mrr_by_rank(p, RANK_0, MR06, &(p->revision_id));
	show_msg((INFO, "Revision %x.\n", p->revision_id));

	if (dram_info != NULL) {
		dram_info->mr5_vendor_id = p->vendor_id;
		dram_info->mr6_vevision_id = p->revision_id;

		for (ch_idx = 0; ch_idx < (p->support_channel_num); ch_idx++)
			for (rank_idx = 0; rank_idx < RANK_MAX; rank_idx++)
				dram_info->mr8_density[ch_idx][rank_idx] = 0;
	}
	for (rank_idx = 0; rank_idx < (p->support_rank_num); rank_idx++) {
		size = 0;
		for (ch_idx = 0; ch_idx < (p->support_channel_num); ch_idx++) {
			set_phy_2_channel_mapping(p, ch_idx);
			dramc_mrr_by_rank(p, rank_idx, 8, &density);
			show_msg((INFO, "MR8 %x\n", density));

			density = (density >> 2) & 0xf;
			if (is_lp4_family(p->dram_type)) {
				switch (density) {
				case channel_density_2Gb:
					channel_size = 0x10000000;
					break;
				case channel_density_3Gb:
					channel_size = 0x18000000;
					break;
				case channel_density_4Gb:
					channel_size = 0x20000000;
					break;
				case channel_density_6Gb:
					channel_size = 0x30000000;
					break;
				case channel_density_8Gb:
					channel_size = 0x40000000;
					break;
				case channel_density_12Gb:
					channel_size = 0x60000000;
					break;
				case channel_density_16Gb:
					channel_size = 0x80000000;
					break;
				default:
					channel_size = 0;
					break;
				}
			}
			size += channel_size;
				p->density = density;
			show_msg((INFO,
				"CH%d, RK%d, Density %llx, RKsize %llx.\n",
				ch_idx, rank_idx, size,
				p->ranksize[rank_idx]));
			}
		p->ranksize[rank_idx] = size;
			if (dram_info != NULL)
			dram_info->mr8_density[ch_idx][rank_idx] = size;

		show_msg((INFO, "CH%d, RK%d, Density %llx, RKsize %llx.\n",
			ch_idx, rank_idx, size, p->ranksize[rank_idx]));
		}

	set_phy_2_channel_mapping(p, CHANNEL_A);

	return DRAM_OK;
}

#ifdef ENABLE_MIOCK_JMETER
void set_pre_miock_jmeter_hqa_used_flag(unsigned char value)
{
	pre_miock_jmeter_hqa_used_flag = value;
}

unsigned char get_pre_miock_jmeter_hqa_used_flag(void)
{
	return pre_miock_jmeter_hqa_used_flag;
}

void pre_miock_jmeter_hqa_used(DRAMC_CTX_T *p)
{
	unsigned int backup_freq_sel, backup_channel;

	backup_freq_sel = p->freq_sel;
	backup_channel = p->channel;

	show_msg3((INFO, "[JMETER_HQA]\n"));
	set_pre_miock_jmeter_hqa_used_flag(1);

	set_phy_2_channel_mapping(p, CHANNEL_A);
	dramc_miock_jmeter_hqa(p);
	set_phy_2_channel_mapping(p, backup_channel);

	set_pre_miock_jmeter_hqa_used_flag(0);

	p->freq_sel = backup_freq_sel;
}
#endif

#ifdef ETT
static void ett_summary(DRAMC_CTX_T *p)
{
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_LPDDR3)
	unsigned int cbt_num=0, rx_num=16, tx_num=16;
	unsigned uiCA;
#endif
	unsigned u1BitIdx, PI_meter;

#if SUPPORT_TYPE_LPDDR3
	cbt_num = CATRAINING_NUM_LP3;
	rx_num = 32;
	tx_num = 32;
#elif SUPPORT_TYPE_LPDDR4
	cbt_num = CATRAINING_NUM_LP4;
#endif

	PI_meter = 100*1000000/(64 * p->frequency);
	show_msg2((INFO, "\n\n[DDR Margin Summary] Dram frequency=%dMHz, 1PI=%d/100 ps\n", p->frequency, PI_meter));
	show_msg2((INFO, "==========================================================\n"));
	show_msg2((INFO, "For ETT table\n"));
	show_msg2((INFO, "==========================================================\n"));
	show_msg2((INFO, "DRVP=0x%x DRVN=0x%x ODTP=0x%x ODTN=0x%x\n", ett_drv[0], ett_drv[1], ett_drv[2], ett_drv[3]));
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)
	show_msg2((INFO, "RX VREF=%d, TX VREF=%d\n", ett_rx_vref, ett_tx_vref));
#endif
	show_msg2((INFO, "Delay cell(x100)=%d, RX min win=%d, TX min win=%d.\n", p->delay_cell_timex100, ett_rx_min, ett_tx_min));

#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_LPDDR3)
	if (cbt_num) {
		show_msg2((INFO, "==========================================================\n"));
		show_msg2((INFO, "CA pass criteria: window > 20PI(0.6UI)\n"));
		show_msg2((INFO, "==========================================================\n"));

		for (uiCA = 0; uiCA < CATRAINING_NUM_LP4; uiCA++)
		{
			show_msg2((INFO, "CA%d Window: %dPI, ", uiCA, ett_cbt_win[uiCA]));
			if (ett_cbt_win[uiCA] > 20)
				show_msg2((INFO, "PASS. Margin: %dPI = %d/100 ps.\n", ett_cbt_win[uiCA] - 20,
				(ett_cbt_win[uiCA] - 20) * PI_meter));
			else
				show_msg2((INFO, "FAIL\n"));
		}
    }
#endif

	show_msg2((INFO, "==========================================================\n"));
	show_msg2((INFO, "RX pass criteria: window > 13PI(0.4UI)\n"));
	show_msg2((INFO, "==========================================================\n"));

	for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
	{
		show_msg2((INFO, "Bit%d (%d - %d) Window: %d delay cell, ", u1BitIdx, ett_rx_first[u1BitIdx],
			ett_rx_last[u1BitIdx], ett_rx_win[u1BitIdx]));
		if ((ett_rx_win[u1BitIdx] * p->delay_cell_timex100) >= (13 * PI_meter))
			show_msg2((INFO, "PASS. Margin: %d/100 ps.\n",
			((ett_rx_win[u1BitIdx] * p->delay_cell_timex100) - (13 * PI_meter))));
		else
			show_msg2((INFO, "FAIL\n"));
	}

	show_msg2((INFO, "==========================================================\n"));
	show_msg2((INFO, "TX pass criteria (LP4): window > 15PI(0.45UI)\n"));
	show_msg2((INFO, "==========================================================\n"));

	for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
	{
		show_msg2((INFO, "Bit%d (%d - %d) Window: %dPI, ", u1BitIdx, ett_tx_first[u1BitIdx],
			ett_tx_last[u1BitIdx], ett_tx_win[u1BitIdx]));
		if (ett_tx_win[u1BitIdx] > 15)
			show_msg2((INFO, "PASS. Margin: %d/100 ps.\n",
			(ett_tx_win[u1BitIdx] - 15) * PI_meter));
		else
			show_msg2((INFO, "FAIL\n"));

	}

    show_msg2((INFO, "[DDR Margin Summary Done]\n\n\n"));
    show_msg2((INFO, "ETT while here!\n"));
	while(1) ;

}

#endif

static int dram_calibration_main(DRAMC_CTX_T *p,
	DRAM_INFO_BY_MRR_T *dram_info)
{
	if ((get_mdl_used_flag()) == GET_MDL_USED) {
		set_phy_2_channel_mapping(p, CHANNEL_A);
		dram_calibration_single_channel(p);
		get_dram_info_after_cal_by_mrr(p, dram_info);
		return DRAM_OK;
	}
	dram_calibration_all_channel(p);

	if (p->dram_type == TYPE_LPDDR4) {
		get_dram_info_after_cal_by_mrr(p, NULL);
		dramc_ac_timing_optimize(p);
	}

#if DVFS_EN
	/* save 1st cal setting */
	dramc_save_cal_settings(p);
	show_msg((CRITICAL, "\n[DVFS] 2nd setting begin\n"));

	/* do 2nd Calibration & save 2nd cal settings */
	ddr_phy_freq_sel(p, DVFS_HIGH_FREQ);
	dramc_dvfs_init(p);
	show_msg((INFO, "\n freq meter CLK: %d MHz\n", DDRPhyFreqMeter(27)));
	dram_calibration_all_channel(p);
	if (p->dram_type == TYPE_LPDDR4) {
		get_dram_info_after_cal_by_mrr(p, NULL);
		dramc_ac_timing_optimize(p);
	}
	dramc_save_cal_settings(p);
	show_msg((CRITICAL, "\n[DVFS] 2nd setting done\n"));

#if DVFS_STRESS_TEST //dram dfs stress test
	int i, err1, err2;
	auto_refresh_switch(p, ENABLE);

	while(1)
	{
		show_msg((CRITICAL, "\n[DVFS]==== H to L start======\n"));
		dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
		dramc_dvfs_switch(p, DVFS_LOW_FREQ);
		show_msg((INFO, "freq meter: %d MHz\n", DDRPhyFreqMeter(27)));
		err1 = dramc_engine2_run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
		err2 = dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
		if (err1 || err2) {
			show_msg((CRITICAL, "[DVFS]==== H to L fail, err1=0x%x, err2=0x%x ======\n", err1, err2));
			while(1);
		}
		show_msg((CRITICAL, "[DVFS]==== H to L success ======\n\n"));


		show_msg((CRITICAL, "\n[DVFS]==== L to H ======\n"));
		dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
		dramc_dvfs_switch(p, DVFS_HIGH_FREQ);
		show_msg((INFO, "freq meter: %d MHz\n", DDRPhyFreqMeter(27)));

		err1 = dramc_engine2_run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
		err2 = dramc_engine2_run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);

		if (err1 || err2) {
			show_msg((CRITICAL, "[DVFS]==== L to H fail, err1=0x%x, err2=0x%x ======\n", err1, err2));
			while(1);
		}
		show_msg((CRITICAL, "[DVFS]==== L to H success======\n\n"));
	}
#endif
#endif /* end of DVFS_EN*/


#if DUAL_FREQ_K
	dramc_save_to_shuffle_reg(p, DRAM_DFS_SHUFFLE_1, DRAM_DFS_SHUFFLE_2);
#endif

#if DUAL_FREQ_K
	/* do LP4_DDR3733 Calibration */
	ddr_phy_freq_sel(p, DRAM_CALIBRATION_FREQ_3);
	set_vcore_by_freq(p);
	dfs_init_for_calibration(p);
	dram_calibration_all_channel(p);
	dramc_ac_timing_optimize(p);
	dramc_save_to_shuffle_reg(p, DRAM_DFS_SHUFFLE_1, DRAM_DFS_SHUFFLE_3);

	/* do LP4_DDR4266 Calibration */
	ddr_phy_freq_sel(p, LP4_HIGHEST_FREQSEL);
	set_vcore_by_freq(p);
	dfs_init_for_calibration(p);
	dram_calibration_all_channel(p);
	dramc_ac_timing_optimize(p);
	dramc_save_to_shuffle_reg(p, DRAM_DFS_SHUFFLE_1, DRAM_DFS_SHUFFLE_4);
#ifndef LK_LEAVE_FREQ_LP4_DDR4266
	/* do LP4_DDR3200 Calibration */
	ddr_phy_freq_sel(p, DRAM_CALIBRATION_FREQ_2);
	set_vcore_by_freq(p);
	dfs_init_for_calibration(p);
	dram_calibration_all_channel(p);
	dramc_ac_timing_optimize(p);
#endif
#endif
	apply_config_after_calibration(p);

#ifdef ETT
	ett_summary(p);
#endif

	return DRAM_OK;
}

static void dram_check(DRAMC_CTX_T *p)
{
#if (FOR_DV_SIMULATION_USED == 0)
#if DRAMC_MODEREG_CHECK
#if SUPPORT_TYPE_LPDDR4
	//set_mrr_pinmux_mapping(p, PINMUX_MRR);
	dramc_mode_reg_check(p);
#endif
#endif

#if CPU_RW_TEST_AFTER_K
	show_msg2((INFO,
	"\n[MEM_TEST] 02: After DFS, before run time config\n"));
	dram_cpu_read_write_test_after_calibration(p);
#endif

#if TA2_RW_TEST_AFTER_K
	show_msg((INFO, "\n[TA2_TEST] freq %d\n", p->frequency));
	ta2_test_run_time_hw(p);
#endif

	show_msg3((INFO, "\n\nSettings after calibration\n\n"));
	dramc_run_time_config(p);

#if 0
	unsigned int *MEM32_BASE = (unsigned int *) DRAM_BASE_VIRT;
	unsigned int err_count = 0;
	unsigned int i = 0;
	unsigned int len = 0x10000000;
	len = len >> 2;

	show_msg((INFO, "test start addr:%p, test len:%d\n", MEM32_BASE, len));
	for (i = len-1; i>0; i--)
		MEM32_BASE[i] = i*4;

	show_msg((INFO, " MEM32_BASE set finish. start test\n"));
	for (int loop = 0; loop <= 100; loop++) {
		err_count = 0;
		for (i = len-1; i>0; i--) {
			if (MEM32_BASE[i] != i*4) {
				show_msg((INFO, "mem test fail, 0x%x != 0x%x(expect)\n",
					MEM32_BASE[i], i*4));
				err_count++;
			} else {
				MEM32_BASE[i] = 0xa5a5a5a5;
			}
			if (i % 600000 == 0)
				show_msg((INFO, "compare count %d finish\n", i));
		}
		for (i = len-1; i>0; i--) {
			if (MEM32_BASE[i] != 0xa5a5a5a5) {
				show_msg((INFO, "mem test fail, 0x%x != 0xa5a5a5a5, addr:0x%x\n",
					MEM32_BASE[i], i*4));
				err_count++;
			} else {
				MEM32_BASE[i] = i*4;
			}
			if (i % 500000 == 0)
				show_msg((INFO, "compare count %d finish\n", i));
		}
		show_msg((INFO, "loop:%d mem test finish, test result %s\n",
			loop, err_count == 0? "pass":"fail"));
		show_msg((INFO, "total error count:%d\n", err_count));
		if (err_count != 0) {
			show_msg((INFO, "\n[TA2_TEST] freq %d\n", p->frequency));
			ta2_test_run_time_hw(p);
		}
	}

	show_msg((INFO, "complex mem test finish, test result %s\n",
		err_count == 0? "pass":"fail"));
	show_msg((INFO, "\n total error count:%d\n", err_count));
	while(1);
#endif

#if CPU_RW_TEST_AFTER_K
	show_msg((INFO, "\n[MEM_TEST] 03: After run time config\n"));
	dram_cpu_read_write_test_after_calibration(p);
#endif
#if 0
	show_msg((INFO, "\n\n VOREE2=0.8V, freq=4266\n"));
	set_buck_voltage(BUCK_VCORE, 900000); /* VCORE_SRAM=0.9v, unit: uV */
	set_buck_voltage(BUCK_VGPU, 800000);  /* VCORE2=0.8v, unit: uV */
	ddr_phy_freq_sel(p, DRAM_CALIBRATION_FREQ_4);
	cbt_dramc_dfs_direct_jump(p, DRAM_DFS_SHUFFLE_4);
#endif
#if TA2_RW_TEST_AFTER_K
	show_msg((INFO, "\n[TA2_TEST] freq %d\n", p->frequency));
	ta2_test_run_time_hw(p);
#endif
#endif
}

static void dump_dramc_ctx_t(DRAMC_CTX_T *p)
{
	show_msg3((INFO, "---------- DRAMC_CTX_T ------------\n"));;
	show_msg3((INFO, "support_channel_num: %d\n", p->support_channel_num));
	show_msg3((INFO, "channel: %d\n", p->channel));
	show_msg3((INFO, "support_rank_num: %d\n", p->support_rank_num));
	show_msg3((INFO, "rank: %d\n", p->rank));
	show_msg3((INFO, "freq_sel: %d\n", p->freq_sel));
	show_msg3((INFO, "shu_type: %d\n", p->shu_type));
	show_msg3((INFO, "dram_type: %d\n", p->dram_type));
	show_msg3((INFO, "dram_fsp: %d\n", p->dram_fsp));
	show_msg3((INFO, "odt_onoff: %d\n", p->odt_onoff));
	show_msg3((INFO, "cbt_mode: %d\n", p->dram_cbt_mode[p->rank]));
	show_msg3((INFO, "dbi_r_onoff: %d\n", p->dbi_r_onoff[p->dram_fsp]));
	show_msg3((INFO, "dbi_w_onoff: %d\n", p->dbi_w_onoff[p->dram_fsp]));
	show_msg3((INFO, "data_width: %d\n", p->data_width));
	show_msg3((INFO, "frequency: %d\n", p->frequency));
	show_msg3((INFO, "freqGroup: %d\n", p->freqGroup));
	show_msg3((INFO, "-----------------------------------\n"));
}

int init_dram(DRAM_DRAM_TYPE_T dram_type,
	DRAM_CBT_MODE_EXTERN_T dram_cbt_mode_extern,
	DRAM_INFO_BY_MRR_T *dram_info, unsigned char get_mdl_used)
{
	DRAMC_CTX_T *p;
	unsigned char backup_broadcast;

	show_msg((INFO, "%s:%d: init_dram Starting\n", __func__, __LINE__));

	switch (dram_type) {
	case TYPE_LPDDR4:
		ps_curr_dram_ctx = dramc_ctx_lp4;
		break;
	case TYPE_LPDDR3:
		ps_curr_dram_ctx = dramc_ctx_lp3;
		break;
	case TYPE_PCDDR4:
		ps_curr_dram_ctx = dramc_ctx_ddr4;
		break;
	case TYPE_PCDDR3:
		ps_curr_dram_ctx = dramc_ctx_ddr3;
		break;
	default:
		show_err("[Error] Unrecognized type\n");
		break;
	}

	p = ps_curr_dram_ctx;
	dump_dramc_ctx_t(p);

#ifdef DDR_RESERVE_MODE
		if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1) {
			show_msg((INFO, "DDR reserve enabled, bypass dram init\n"));
			ddr_phy_freq_sel(p, p->freq_sel);
			goto NO_DRAMK;
		}
#endif

	set_mdl_used_flag(get_mdl_used);

	/* Convert DRAM_CBT_MODE_EXTERN_T to DRAM_CBT_MODE_T */
	switch ((int)dram_cbt_mode_extern) {
	case CBT_R0_R1_NORMAL:
		p->dram_cbt_mode[RANK_0] = CBT_NORMAL_MODE;
		p->dram_cbt_mode[RANK_1] = CBT_NORMAL_MODE;
		break;
	case CBT_R0_R1_BYTE:
	case CBT_R0_NORMAL_R1_BYTE:
	case CBT_R0_BYTE_R1_NORMAL:
	default:
		show_err("Error!Not Support");
		break;
	}

	dramc_broadcast_on_off(DRAMC_BROADCAST_ON);

	if (gfirst_init_flag == 0) {
		mpll_init();
		global_option_init(p);
		gfirst_init_flag = 1;
	}

	show_msg((INFO, "\n%s %s\n", chip_name, version));
	show_msg((INFO, "dram_type %d, R0/R1 cbt_mode %d/%d\n\n",
		p->dram_type, p->dram_cbt_mode[RANK_0],
		p->dram_cbt_mode[RANK_1]));

	dramc_init_pre_settings(p);

#if DUAL_FREQ_K
	spm_pinmux_setting(p);
#endif
	ddr_phy_freq_sel(p, p->freq_sel);

	//set_vcore_by_freq(p);  //yifei: remove for bring up

	dump_dramc_ctx_t(p);

#if SIMULATION_SW_IMPED
#if 0
	if (p->dram_type == TYPE_LPDDR4) {
		dramc_sw_impedance_cal(p, TERM);
	} else if (p->dram_type == TYPE_LPDDR4X) {
		dramc_sw_impedance_cal(p, UNTERM);
		dramc_sw_impedance_cal(p, TERM);
	} else { /* TYPE_LPDDR4P */
		dramc_sw_impedance_cal(p, UNTERM);
	}
#endif
	dramc_sw_impedance_cal(p, p->odt_onoff);

	dramc_update_impedance_term_2un_term(p);
#endif

	dfs_init_for_calibration(p);

#if DVFS_EN
	unsigned int i, high_addr, low_addr, rg_addr;
	for (i = 0; i < DVFS_LP4_ADDR_NUM; i++) {
		dvfs_lp4_cal_setting_addr[i] = DRAMC_REG_ADDR(dvfs_lp4_cal_setting_addr[i]);
	}
	/* use dummy read pattern RG to save the addr of DVFS high/low/RG*/
	high_addr = (unsigned int)(dvfs_lp4_high_freq_cal_setting);
	low_addr = (unsigned int)(dvfs_lp4_low_freq_cal_setting);
	rg_addr = (unsigned int)dvfs_lp4_cal_setting_addr;

	io32_write_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA0, high_addr);
	io32_write_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA1, low_addr);
	io32_write_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA2, rg_addr);

#if 1
	show_msg((INFO, "DVFS addr check: H setting addr:0x%x\n", dvfs_lp4_high_freq_cal_setting));
	show_msg((INFO, "DVFS addr check: L setting addr:0x%x\n", dvfs_lp4_low_freq_cal_setting));
	show_msg((INFO, "DVFS addr check: RG addr:0x%x\n", dvfs_lp4_cal_setting_addr));
	show_msg((INFO, "DVFS addr check: WDATA0:0x%x\n", io32_read_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA0)));
	show_msg((INFO, "DVFS addr check: WDATA1:0x%x\n", io32_read_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA1)));
	show_msg((INFO, "DVFS addr check: WDATA2:0x%x\n", io32_read_4b(DRAMC_REG_RK0_DUMMY_RD_WDATA2)));
#endif
#endif

#ifdef ENABLE_MIOCK_JMETER
	if (p->frequency >= PERBIT_THRESHOLD_FREQ)
	pre_miock_jmeter_hqa_used(p);
#endif

	backup_broadcast = get_dramc_broadcast();
#if (FOR_DV_SIMULATION_USED == 0)
	emi_init(p);
	emi_init2();
#endif
	dramc_broadcast_on_off(backup_broadcast);

	dram_calibration_main(p, dram_info);

#if SCRAMBLE_ENABLE
	dram_scramble (p);
#endif

	dram_check(p);

#ifdef DDR_RESERVE_MODE
NO_DRAMK:
#endif

#ifdef DDR_RESERVE_MODE
	rgu_dram_reserved(1);
#endif

	return DRAM_OK;
}


int dram_in_self_refresh(void)
{
	DRAMC_CTX_T * p;
	p = &DramCtx_LPDDR4;

	if (io_32_read_fld_align(DRAMC_REG_MISC_STATUSA, MISC_STATUSA_SREF_STATE))
		return 1;
	else
		return 0;
}

unsigned int is_dramc_exit_slf(void)
{
	return !dram_in_self_refresh();
}

void Dramc_DDR_Reserved_Mode_setting(void)
{
#ifdef DDR_RESERVE_MODE
	DRAMC_CTX_T * p;
	p = &DramCtx_LPDDR4;

	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL0, 1, MISC_CG_CTRL0_W_CHG_MEM);

	delay_us(1);

	io_32_write_fld_align_all(DDRPHY_MISC_CG_CTRL0, 0, MISC_CG_CTRL0_W_CHG_MEM);
#endif
}

void release_dram(void)
{
#ifdef DDR_RESERVE_MODE
	int i;
	int counter = 3;

	// scy: restore pmic setting (VCORE, VDRAM, VSRAM, VDDQ)
	//restore_pmic_setting();
	rgu_release_rg_dramc_conf_iso();//Release DRAMC/PHY conf ISO
	Dramc_DDR_Reserved_Mode_setting();
	rgu_release_rg_dramc_iso();//Release PHY IO ISO
	rgu_release_rg_dramc_sref();//Let DRAM Leave SR

	// setup for EMI: touch center EMI and channel EMI to enable CLK
	show_msg((INFO, "[DDR reserve] EMI CONA: %x\n", *(volatile unsigned int*)EMI_BASE));

	for (i=0;i<10;i++);

	while(counter)
	{
		if(is_dramc_exit_slf() == 1) /* expect to exit dram-self-refresh */
			break;
		counter--;
	}

	if(counter == 0)
	{
		if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1)
		{
			show_msg((INFO, "[DDR Reserve] release dram from self-refresh FAIL!\n"));
			g_ddr_reserve_success = 0;
		}
	}
	else
	{
		show_msg((INFO, "[DDR Reserve] release dram from self-refresh PASS!\n"));
	}
//Dramc_DDR_Reserved_Mode_AfterSR();
//Expect to Use LPDDR3200 and PHYPLL as output, so no need to handle
//shuffle status since the status will be reset by system reset
//There is an PLLL_SHU_GP in SPM which will reset by system reset

//EMI_Restore_Setting();

	return;
#endif
}


void check_ddr_reserve_status(void)
{
#ifdef DDR_RESERVE_MODE
	int counter = 3;

	if(rgu_is_reserve_ddr_enabled()) {
		show_msg((INFO, "\n DDR reserve mode enabled.\n"));
		g_ddr_reserve_enable = 1;
		g_ddr_reserve_success = 1; //8695 ddr reserve status bit tie 1
		while(counter) {
			if(dram_in_self_refresh()) {
				g_ddr_reserve_success = 1;
				break;
			}
			counter--;
		}

		if(counter == 0) {
			show_msg((INFO, "[DDR Reserve] ddr reserve mode enabled but DRAM not in self-refresh!\n"));
			g_ddr_reserve_success = 0;
		}

		release_dram();
	} else {
		show_msg((INFO, "[DDR Reserve] ddr reserve mode not be enabled yet\n"));
		g_ddr_reserve_enable = 0;
	}
#endif
}


#if (FOR_DV_SIMULATION_USED == 1)
/*
 * This function is used to run bring up simulation.
 * It shall not be used in normal flow, but use init_dram() instead.
 * @ps_dramc_ctx: NOT used. Just to meet DPI interface requirement.
 * Spec.:
 * LPDDR4: 2400
 * LPDDR3: 1866
 * PCDDR4: 2667
 * PCDDR3: 1866
 */
void dpi_simulation_dramc(DRAMC_CTX_T *ps_dramc_ctx_unused)
{
	DRAM_DRAM_TYPE_T type;

#if SUPPORT_TYPE_LPDDR4 /* Highest Priority */
	type = TYPE_LPDDR4;
#elif SUPPORT_TYPE_LPDDR3
	type = TYPE_LPDDR3;
#elif SUPPORT_TYPE_PCDDR4
	type = TYPE_PCDDR4;
#else
	type = TYPE_PCDDR3; /* Lowest */
#endif

	show_msg_with_timestamp((CRITICAL, "start dramc calibration\n"));
	init_dram(type, CBT_R0_R1_NORMAL, NULL, NORMAL_USED);
}

#endif

#ifndef __DRAMC_REGISTER_H
#define __DRAMC_REGISTER_H

#include <platform/dramc_ao_reg.h>
#include <platform/dramc_nao_reg.h>
#include <platform/ddrphy_b01_reg.h>
#include <platform/mt_reg_base.h>

/* Note that since PHY has B01 and B23, which seems like
 * two channels, SW will treat that as two virtual channel.
 * Thus LP4 and DDR3X16 shall be set to use virtual channel B.
 * Doing this is mainly for IO processing convinience.
 */
#define DRAMC_NAO_BASE_VIRTUAL			0x40000
#define DRAMC_NAO_BASE_VIRTUAL_CHB		0x50000
#define DRAMC_AO_BASE_VIRTUAL			0x60000
#define DRAMC_AO_BASE_VIRTUAL_CHB		0x70000
#define DDRPHY_BASE_ADDR_VIRTUAL		0x80000
#define DDRPHY_B23_BASE_VIRTUAL			0x90000
#define MAX_BASE_VIRTUAL			0xa0000

#define DRAMC_NAO_BASE_PHYS			(IO_PHYS + 0x00206000)
#define DRAMC_AO_BASE_PHYS			(IO_PHYS + 0x00207000)
#define DDRPHY_BASE_PHYS			(IO_PHYS + 0x00211000)
#define DDRPHY_B23_BASE_PHYS		(IO_PHYS + 0x00212000)

#define DRAMC_BROADCAST_ON		0x1
#define DRAMC_BROADCAST_OFF		0x0

#define POS_BANK_NUM			16
/*===================== DRAMC NAO ====================*/
#define DRAMC_NAO_BASE		DRAMC_NAO_BASE_VIRTUAL

#define DRAMC_REG_TESTMODE (DRAMC_NAO_BASE + TESTMODE)
#define DRAMC_REG_LBWDAT0 (DRAMC_NAO_BASE + LBWDAT0)
#define DRAMC_REG_LBWDAT1 (DRAMC_NAO_BASE + LBWDAT1)
#define DRAMC_REG_LBWDAT2 (DRAMC_NAO_BASE + LBWDAT2)
#define DRAMC_REG_LBWDAT3 (DRAMC_NAO_BASE + LBWDAT3)
#define DRAMC_REG_CKPHCHK (DRAMC_NAO_BASE + CKPHCHK)
#define DRAMC_REG_DMMONITOR (DRAMC_NAO_BASE + DMMONITOR)
#define DRAMC_REG_TESTCHIP_DMA1 (DRAMC_NAO_BASE + TESTCHIP_DMA1)
#define DRAMC_REG_MISC_STATUSA (DRAMC_NAO_BASE + MISC_STATUSA)
#define DRAMC_REG_SPECIAL_STATUS (DRAMC_NAO_BASE + SPECIAL_STATUS)
#define DRAMC_REG_SPCMDRESP (DRAMC_NAO_BASE + SPCMDRESP)
#define DRAMC_REG_MRR_STATUS (DRAMC_NAO_BASE + MRR_STATUS)
#define DRAMC_REG_MRR_STATUS2 (DRAMC_NAO_BASE + MRR_STATUS2)
#define DRAMC_REG_MRRDATA0 (DRAMC_NAO_BASE + MRRDATA0)
#define DRAMC_REG_MRRDATA1 (DRAMC_NAO_BASE + MRRDATA1)
#define DRAMC_REG_MRRDATA2 (DRAMC_NAO_BASE + MRRDATA2)
#define DRAMC_REG_MRRDATA3 (DRAMC_NAO_BASE + MRRDATA3)
#define DRAMC_REG_JMETER_ST (DRAMC_NAO_BASE + JMETER_ST)
#define DRAMC_REG_TCMDO1LAT (DRAMC_NAO_BASE + TCMDO1LAT)
#define DRAMC_REG_RDQC_CMP (DRAMC_NAO_BASE + RDQC_CMP)
#define DRAMC_REG_CKPHCHK_STATUS (DRAMC_NAO_BASE + CKPHCHK_STATUS)
#define DRAMC_REG_HWMRR_PUSH2POP_CNT (DRAMC_NAO_BASE + HWMRR_PUSH2POP_CNT)
#define DRAMC_REG_HWMRR_STATUS (DRAMC_NAO_BASE + HWMRR_STATUS)
#define DRAMC_REG_TESTRPT (DRAMC_NAO_BASE + TESTRPT)
#define DRAMC_REG_CMP_ERR (DRAMC_NAO_BASE + CMP_ERR)
#define DRAMC_REG_TEST_ABIT_STATUS1 (DRAMC_NAO_BASE + TEST_ABIT_STATUS1)
#define DRAMC_REG_TEST_ABIT_STATUS2 (DRAMC_NAO_BASE + TEST_ABIT_STATUS2)
#define DRAMC_REG_TEST_ABIT_STATUS3 (DRAMC_NAO_BASE + TEST_ABIT_STATUS3)
#define DRAMC_REG_TEST_ABIT_STATUS4 (DRAMC_NAO_BASE + TEST_ABIT_STATUS4)
#define DRAMC_REG_DQSDLY0 (DRAMC_NAO_BASE + DQSDLY0)
#define DRAMC_REG_DQ_CAL_MAX_0 (DRAMC_NAO_BASE + DQ_CAL_MAX_0)
#define DRAMC_REG_DQ_CAL_MAX_1 (DRAMC_NAO_BASE + DQ_CAL_MAX_1)
#define DRAMC_REG_DQ_CAL_MAX_2 (DRAMC_NAO_BASE + DQ_CAL_MAX_2)
#define DRAMC_REG_DQ_CAL_MAX_3 (DRAMC_NAO_BASE + DQ_CAL_MAX_3)
#define DRAMC_REG_DQ_CAL_MAX_4 (DRAMC_NAO_BASE + DQ_CAL_MAX_4)
#define DRAMC_REG_DQ_CAL_MAX_5 (DRAMC_NAO_BASE + DQ_CAL_MAX_5)
#define DRAMC_REG_DQ_CAL_MAX_6 (DRAMC_NAO_BASE + DQ_CAL_MAX_6)
#define DRAMC_REG_DQ_CAL_MAX_7 (DRAMC_NAO_BASE + DQ_CAL_MAX_7)
#define DRAMC_REG_DQS_CAL_MIN_0 (DRAMC_NAO_BASE + DQS_CAL_MIN_0)
#define DRAMC_REG_DQS_CAL_MIN_1 (DRAMC_NAO_BASE + DQS_CAL_MIN_1)
#define DRAMC_REG_DQS_CAL_MIN_2 (DRAMC_NAO_BASE + DQS_CAL_MIN_2)
#define DRAMC_REG_DQS_CAL_MIN_3 (DRAMC_NAO_BASE + DQS_CAL_MIN_3)
#define DRAMC_REG_DQS_CAL_MIN_4 (DRAMC_NAO_BASE + DQS_CAL_MIN_4)
#define DRAMC_REG_DQS_CAL_MIN_5 (DRAMC_NAO_BASE + DQS_CAL_MIN_5)
#define DRAMC_REG_DQS_CAL_MIN_6 (DRAMC_NAO_BASE + DQS_CAL_MIN_6)
#define DRAMC_REG_DQS_CAL_MIN_7 (DRAMC_NAO_BASE + DQS_CAL_MIN_7)
#define DRAMC_REG_DQS_CAL_MAX_0 (DRAMC_NAO_BASE + DQS_CAL_MAX_0)
#define DRAMC_REG_DQS_CAL_MAX_1 (DRAMC_NAO_BASE + DQS_CAL_MAX_1)
#define DRAMC_REG_DQS_CAL_MAX_2 (DRAMC_NAO_BASE + DQS_CAL_MAX_2)
#define DRAMC_REG_DQS_CAL_MAX_3 (DRAMC_NAO_BASE + DQS_CAL_MAX_3)
#define DRAMC_REG_DQS_CAL_MAX_4 (DRAMC_NAO_BASE + DQS_CAL_MAX_4)
#define DRAMC_REG_DQS_CAL_MAX_5 (DRAMC_NAO_BASE + DQS_CAL_MAX_5)
#define DRAMC_REG_DQS_CAL_MAX_6 (DRAMC_NAO_BASE + DQS_CAL_MAX_6)
#define DRAMC_REG_DQS_CAL_MAX_7 (DRAMC_NAO_BASE + DQS_CAL_MAX_7)
#define DRAMC_REG_DQICAL0 (DRAMC_NAO_BASE + DQICAL0)
#define DRAMC_REG_DQICAL1 (DRAMC_NAO_BASE + DQICAL1)
#define DRAMC_REG_DQICAL2 (DRAMC_NAO_BASE + DQICAL2)
#define DRAMC_REG_DQICAL3 (DRAMC_NAO_BASE + DQICAL3)
#define DRAMC_REG_TESTCHIP_DMA_STATUS1 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS1)
#define DRAMC_REG_TESTCHIP_DMA_STATUS2 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS2)
#define DRAMC_REG_TESTCHIP_DMA_STATUS3 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS3)
#define DRAMC_REG_TESTCHIP_DMA_STATUS4 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS4)
#define DRAMC_REG_TESTCHIP_DMA_STATUS5 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS5)
#define DRAMC_REG_TESTCHIP_DMA_STATUS6 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS6)
#define DRAMC_REG_TESTCHIP_DMA_STATUS7 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS7)
#define DRAMC_REG_TESTCHIP_DMA_STATUS8 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS8)
#define DRAMC_REG_TESTCHIP_DMA_STATUS9 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS9)
#define DRAMC_REG_TESTCHIP_DMA_STATUS10 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS10)
#define DRAMC_REG_TESTCHIP_DMA_STATUS11 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS11)
#define DRAMC_REG_TESTCHIP_DMA_STATUS12 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS12)
#define DRAMC_REG_TESTCHIP_DMA_STATUS13 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS13)
#define DRAMC_REG_TESTCHIP_DMA_STATUS14 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS14)
#define DRAMC_REG_TESTCHIP_DMA_STATUS15 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS15)
#define DRAMC_REG_TESTCHIP_DMA_STATUS16 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS16)
#define DRAMC_REG_TESTCHIP_DMA_STATUS17 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS17)
#define DRAMC_REG_TESTCHIP_DMA_STATUS18 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS18)
#define DRAMC_REG_TESTCHIP_DMA_STATUS19 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS19)
#define DRAMC_REG_TESTCHIP_DMA_STATUS20 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS20)
#define DRAMC_REG_TESTCHIP_DMA_STATUS21 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS21)
#define DRAMC_REG_TESTCHIP_DMA_STATUS22 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS22)
#define DRAMC_REG_TESTCHIP_DMA_STATUS23 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS23)
#define DRAMC_REG_TESTCHIP_DMA_STATUS24 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS24)
#define DRAMC_REG_TESTCHIP_DMA_STATUS25 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS25)
#define DRAMC_REG_TESTCHIP_DMA_STATUS26 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS26)
#define DRAMC_REG_TESTCHIP_DMA_STATUS27 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS27)
#define DRAMC_REG_TESTCHIP_DMA_STATUS28 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS28)
#define DRAMC_REG_TESTCHIP_DMA_STATUS29 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS29)
#define DRAMC_REG_TESTCHIP_DMA_STATUS30 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS30)
#define DRAMC_REG_TESTCHIP_DMA_STATUS31 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS31)
#define DRAMC_REG_TESTCHIP_DMA_STATUS32 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS32)
#define DRAMC_REG_TESTCHIP_DMA_STATUS33 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS33)
#define DRAMC_REG_TESTCHIP_DMA_STATUS34 (DRAMC_NAO_BASE + TESTCHIP_DMA_STATUS34)
#define DRAMC_REG_REFRESH_POP_COUNTER (DRAMC_NAO_BASE + REFRESH_POP_COUNTER)
#define DRAMC_REG_FREERUN_26M_COUNTER (DRAMC_NAO_BASE + FREERUN_26M_COUNTER)
#define DRAMC_REG_DRAMC_IDLE_COUNTER (DRAMC_NAO_BASE + DRAMC_IDLE_COUNTER)
#define DRAMC_REG_R2R_PAGE_HIT_COUNTER (DRAMC_NAO_BASE + R2R_PAGE_HIT_COUNTER)
#define DRAMC_REG_R2R_PAGE_MISS_COUNTER (DRAMC_NAO_BASE + R2R_PAGE_MISS_COUNTER)
#define DRAMC_REG_R2R_INTERBANK_COUNTER (DRAMC_NAO_BASE + R2R_INTERBANK_COUNTER)
#define DRAMC_REG_R2W_PAGE_HIT_COUNTER (DRAMC_NAO_BASE + R2W_PAGE_HIT_COUNTER)
#define DRAMC_REG_R2W_PAGE_MISS_COUNTER (DRAMC_NAO_BASE + R2W_PAGE_MISS_COUNTER)
#define DRAMC_REG_R2W_INTERBANK_COUNTER (DRAMC_NAO_BASE + R2W_INTERBANK_COUNTER)
#define DRAMC_REG_W2R_PAGE_HIT_COUNTER (DRAMC_NAO_BASE + W2R_PAGE_HIT_COUNTER)
#define DRAMC_REG_W2R_PAGE_MISS_COUNTER (DRAMC_NAO_BASE + W2R_PAGE_MISS_COUNTER)
#define DRAMC_REG_W2R_INTERBANK_COUNTER (DRAMC_NAO_BASE + W2R_INTERBANK_COUNTER)
#define DRAMC_REG_W2W_PAGE_HIT_COUNTER (DRAMC_NAO_BASE + W2W_PAGE_HIT_COUNTER)
#define DRAMC_REG_W2W_PAGE_MISS_COUNTER (DRAMC_NAO_BASE + W2W_PAGE_MISS_COUNTER)
#define DRAMC_REG_W2W_INTERBANK_COUNTER (DRAMC_NAO_BASE + W2W_INTERBANK_COUNTER)
#define DRAMC_REG_RK0_PRE_STANDBY_COUNTER (DRAMC_NAO_BASE + RK0_PRE_STANDBY_COUNTER)
#define DRAMC_REG_RK0_PRE_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK0_PRE_POWERDOWN_COUNTER)
#define DRAMC_REG_RK0_ACT_STANDBY_COUNTER (DRAMC_NAO_BASE + RK0_ACT_STANDBY_COUNTER)
#define DRAMC_REG_RK0_ACT_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK0_ACT_POWERDOWN_COUNTER)
#define DRAMC_REG_RK1_PRE_STANDBY_COUNTER (DRAMC_NAO_BASE + RK1_PRE_STANDBY_COUNTER)
#define DRAMC_REG_RK1_PRE_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK1_PRE_POWERDOWN_COUNTER)
#define DRAMC_REG_RK1_ACT_STANDBY_COUNTER (DRAMC_NAO_BASE + RK1_ACT_STANDBY_COUNTER)
#define DRAMC_REG_RK1_ACT_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK1_ACT_POWERDOWN_COUNTER)
#define DRAMC_REG_RK2_PRE_STANDBY_COUNTER (DRAMC_NAO_BASE + RK2_PRE_STANDBY_COUNTER)
#define DRAMC_REG_RK2_PRE_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK2_PRE_POWERDOWN_COUNTER)
#define DRAMC_REG_RK2_ACT_STANDBY_COUNTER (DRAMC_NAO_BASE + RK2_ACT_STANDBY_COUNTER)
#define DRAMC_REG_RK2_ACT_POWERDOWN_COUNTER (DRAMC_NAO_BASE + RK2_ACT_POWERDOWN_COUNTER)
#define DRAMC_REG_DQ0_TOGGLE_COUNTER (DRAMC_NAO_BASE + DQ0_TOGGLE_COUNTER)
#define DRAMC_REG_DQ1_TOGGLE_COUNTER (DRAMC_NAO_BASE + DQ1_TOGGLE_COUNTER)
#define DRAMC_REG_DQ2_TOGGLE_COUNTER (DRAMC_NAO_BASE + DQ2_TOGGLE_COUNTER)
#define DRAMC_REG_DQ3_TOGGLE_COUNTER (DRAMC_NAO_BASE + DQ3_TOGGLE_COUNTER)
#define DRAMC_REG_DQ0_TOGGLE_COUNTER_R (DRAMC_NAO_BASE + DQ0_TOGGLE_COUNTER_R)
#define DRAMC_REG_DQ1_TOGGLE_COUNTER_R (DRAMC_NAO_BASE + DQ1_TOGGLE_COUNTER_R)
#define DRAMC_REG_DQ2_TOGGLE_COUNTER_R (DRAMC_NAO_BASE + DQ2_TOGGLE_COUNTER_R)
#define DRAMC_REG_DQ3_TOGGLE_COUNTER_R (DRAMC_NAO_BASE + DQ3_TOGGLE_COUNTER_R)
#define DRAMC_REG_READ_BYTES_COUNTER (DRAMC_NAO_BASE + READ_BYTES_COUNTER)
#define DRAMC_REG_WRITE_BYTES_COUNTER (DRAMC_NAO_BASE + WRITE_BYTES_COUNTER)
#define DRAMC_REG_DQSSAMPLEV (DRAMC_NAO_BASE + DQSSAMPLEV)
#define DRAMC_REG_DQSGNWCNT0 (DRAMC_NAO_BASE + DQSGNWCNT0)
#define DRAMC_REG_DQSGNWCNT1 (DRAMC_NAO_BASE + DQSGNWCNT1)
#define DRAMC_REG_DQSGNWCNT2 (DRAMC_NAO_BASE + DQSGNWCNT2)
#define DRAMC_REG_DQSGNWCNT3 (DRAMC_NAO_BASE + DQSGNWCNT3)
#define DRAMC_REG_DQSGNWCNT4 (DRAMC_NAO_BASE + DQSGNWCNT4)
#define DRAMC_REG_DQSGNWCNT5 (DRAMC_NAO_BASE + DQSGNWCNT5)
#define DRAMC_REG_IORGCNT (DRAMC_NAO_BASE + IORGCNT)
#define DRAMC_REG_DQSG_RETRY_STATE (DRAMC_NAO_BASE + DQSG_RETRY_STATE)
#define DRAMC_REG_DQSG_RETRY_STATE1 (DRAMC_NAO_BASE + DQSG_RETRY_STATE1)
#define DRAMC_REG_IMPCAL_STATUS1 (DRAMC_NAO_BASE + IMPCAL_STATUS1)
#define DRAMC_REG_IMPCAL_STATUS2 (DRAMC_NAO_BASE + IMPCAL_STATUS2)
#define DRAMC_REG_DQDRV_STATUS (DRAMC_NAO_BASE + DQDRV_STATUS)
#define DRAMC_REG_CMDDRV_STATUS (DRAMC_NAO_BASE + CMDDRV_STATUS)
#define DRAMC_REG_CMDDRV1 (DRAMC_NAO_BASE + CMDDRV1)
#define DRAMC_REG_CMDDRV2 (DRAMC_NAO_BASE + CMDDRV2)
#define DRAMC_REG_RK0_DQSOSC_STATUS (DRAMC_NAO_BASE + RK0_DQSOSC_STATUS)
#define DRAMC_REG_RK0_DQSOSC_DELTA (DRAMC_NAO_BASE + RK0_DQSOSC_DELTA)
#define DRAMC_REG_RK0_DQSOSC_DELTA2 (DRAMC_NAO_BASE + RK0_DQSOSC_DELTA2)
#define DRAMC_REG_RK0_CURRENT_TX_SETTING1 (DRAMC_NAO_BASE + RK0_CURRENT_TX_SETTING1)
#define DRAMC_REG_RK0_CURRENT_TX_SETTING2 (DRAMC_NAO_BASE + RK0_CURRENT_TX_SETTING2)
#define DRAMC_REG_RK0_CURRENT_TX_SETTING3 (DRAMC_NAO_BASE + RK0_CURRENT_TX_SETTING3)
#define DRAMC_REG_RK0_CURRENT_TX_SETTING4 (DRAMC_NAO_BASE + RK0_CURRENT_TX_SETTING4)
#define DRAMC_REG_RK0_DUMMY_RD_DATA0 (DRAMC_NAO_BASE + RK0_DUMMY_RD_DATA0)
#define DRAMC_REG_RK0_DUMMY_RD_DATA1 (DRAMC_NAO_BASE + RK0_DUMMY_RD_DATA1)
#define DRAMC_REG_RK0_DUMMY_RD_DATA2 (DRAMC_NAO_BASE + RK0_DUMMY_RD_DATA2)
#define DRAMC_REG_RK0_DUMMY_RD_DATA3 (DRAMC_NAO_BASE + RK0_DUMMY_RD_DATA3)
#define DRAMC_REG_RK0_DQSIENDLY (DRAMC_NAO_BASE + RK0_DQSIENDLY)
#define DRAMC_REG_RK0_DQSIENUIDLY (DRAMC_NAO_BASE + RK0_DQSIENUIDLY)
#define DRAMC_REG_RK0_DQSIENUIDLY_P1 (DRAMC_NAO_BASE + RK0_DQSIENUIDLY_P1)
#define DRAMC_REG_RK0_DQS_STBCALDEC_CNT1 (DRAMC_NAO_BASE + RK0_DQS_STBCALDEC_CNT1)
#define DRAMC_REG_RK0_DQS_STBCALDEC_CNT2 (DRAMC_NAO_BASE + RK0_DQS_STBCALDEC_CNT2)
#define DRAMC_REG_RK0_DQS_STBCALINC_CNT1 (DRAMC_NAO_BASE + RK0_DQS_STBCALINC_CNT1)
#define DRAMC_REG_RK0_DQS_STBCALINC_CNT2 (DRAMC_NAO_BASE + RK0_DQS_STBCALINC_CNT2)
#define DRAMC_REG_RK0_PI_DQ_CAL (DRAMC_NAO_BASE + RK0_PI_DQ_CAL)
#define DRAMC_REG_RK0_DQSG_RETRY_FLAG (DRAMC_NAO_BASE + RK0_DQSG_RETRY_FLAG)
#define DRAMC_REG_RK0_PI_DQM_CAL (DRAMC_NAO_BASE + RK0_PI_DQM_CAL)
#define DRAMC_REG_RK0_DQS0_STBCAL_CNT (DRAMC_NAO_BASE + RK0_DQS0_STBCAL_CNT)
#define DRAMC_REG_RK0_DQS1_STBCAL_CNT (DRAMC_NAO_BASE + RK0_DQS1_STBCAL_CNT)
#define DRAMC_REG_RK0_DQS2_STBCAL_CNT (DRAMC_NAO_BASE + RK0_DQS2_STBCAL_CNT)
#define DRAMC_REG_RK0_DQS3_STBCAL_CNT (DRAMC_NAO_BASE + RK0_DQS3_STBCAL_CNT)
#define DRAMC_REG_RK1_DQSOSC_STATUS (DRAMC_NAO_BASE + RK1_DQSOSC_STATUS)
#define DRAMC_REG_RK1_DQSOSC_DELTA (DRAMC_NAO_BASE + RK1_DQSOSC_DELTA)
#define DRAMC_REG_RK1_DQSOSC_DELTA2 (DRAMC_NAO_BASE + RK1_DQSOSC_DELTA2)
#define DRAMC_REG_RK1_CURRENT_TX_SETTING1 (DRAMC_NAO_BASE + RK1_CURRENT_TX_SETTING1)
#define DRAMC_REG_RK1_CURRENT_TX_SETTING2 (DRAMC_NAO_BASE + RK1_CURRENT_TX_SETTING2)
#define DRAMC_REG_RK1_CURRENT_TX_SETTING3 (DRAMC_NAO_BASE + RK1_CURRENT_TX_SETTING3)
#define DRAMC_REG_RK1_CURRENT_TX_SETTING4 (DRAMC_NAO_BASE + RK1_CURRENT_TX_SETTING4)
#define DRAMC_REG_RK1_DUMMY_RD_DATA0 (DRAMC_NAO_BASE + RK1_DUMMY_RD_DATA0)
#define DRAMC_REG_RK1_DUMMY_RD_DATA1 (DRAMC_NAO_BASE + RK1_DUMMY_RD_DATA1)
#define DRAMC_REG_RK1_DUMMY_RD_DATA2 (DRAMC_NAO_BASE + RK1_DUMMY_RD_DATA2)
#define DRAMC_REG_RK1_DUMMY_RD_DATA3 (DRAMC_NAO_BASE + RK1_DUMMY_RD_DATA3)
#define DRAMC_REG_RK1_DQSIENDLY (DRAMC_NAO_BASE + RK1_DQSIENDLY)
#define DRAMC_REG_RK1_DQSIENUIDLY (DRAMC_NAO_BASE + RK1_DQSIENUIDLY)
#define DRAMC_REG_RK1_DQSIENUIDLY_P1 (DRAMC_NAO_BASE + RK1_DQSIENUIDLY_P1)
#define DRAMC_REG_RK1_DQS_STBCALDEC_CNT1 (DRAMC_NAO_BASE + RK1_DQS_STBCALDEC_CNT1)
#define DRAMC_REG_RK1_DQS_STBCALDEC_CNT2 (DRAMC_NAO_BASE + RK1_DQS_STBCALDEC_CNT2)
#define DRAMC_REG_RK1_DQS_STBCALINC_CNT1 (DRAMC_NAO_BASE + RK1_DQS_STBCALINC_CNT1)
#define DRAMC_REG_RK1_DQS_STBCALINC_CNT2 (DRAMC_NAO_BASE + RK1_DQS_STBCALINC_CNT2)
#define DRAMC_REG_RK1_PI_DQ_CAL (DRAMC_NAO_BASE + RK1_PI_DQ_CAL)
#define DRAMC_REG_RK1_DQSG_RETRY_FLAG (DRAMC_NAO_BASE + RK1_DQSG_RETRY_FLAG)
#define DRAMC_REG_RK1_PI_DQM_CAL (DRAMC_NAO_BASE + RK1_PI_DQM_CAL)
#define DRAMC_REG_RK1_DQS0_STBCAL_CNT (DRAMC_NAO_BASE + RK1_DQS0_STBCAL_CNT)
#define DRAMC_REG_RK1_DQS1_STBCAL_CNT (DRAMC_NAO_BASE + RK1_DQS1_STBCAL_CNT)
#define DRAMC_REG_RK1_DQS2_STBCAL_CNT (DRAMC_NAO_BASE + RK1_DQS2_STBCAL_CNT)
#define DRAMC_REG_RK1_DQS3_STBCAL_CNT (DRAMC_NAO_BASE + RK1_DQS3_STBCAL_CNT)
#define DRAMC_REG_RK2_DQSOSC_STATUS (DRAMC_NAO_BASE + RK2_DQSOSC_STATUS)
#define DRAMC_REG_RK2_DQSOSC_DELTA (DRAMC_NAO_BASE + RK2_DQSOSC_DELTA)
#define DRAMC_REG_RK2_DQSOSC_DELTA2 (DRAMC_NAO_BASE + RK2_DQSOSC_DELTA2)
#define DRAMC_REG_RK2_CURRENT_TX_SETTING1 (DRAMC_NAO_BASE + RK2_CURRENT_TX_SETTING1)
#define DRAMC_REG_RK2_CURRENT_TX_SETTING2 (DRAMC_NAO_BASE + RK2_CURRENT_TX_SETTING2)
#define DRAMC_REG_RK2_CURRENT_TX_SETTING3 (DRAMC_NAO_BASE + RK2_CURRENT_TX_SETTING3)
#define DRAMC_REG_RK2_CURRENT_TX_SETTING4 (DRAMC_NAO_BASE + RK2_CURRENT_TX_SETTING4)
#define DRAMC_REG_RK2_DUMMY_RD_DATA0 (DRAMC_NAO_BASE + RK2_DUMMY_RD_DATA0)
#define DRAMC_REG_RK2_DUMMY_RD_DATA1 (DRAMC_NAO_BASE + RK2_DUMMY_RD_DATA1)
#define DRAMC_REG_RK2_DUMMY_RD_DATA2 (DRAMC_NAO_BASE + RK2_DUMMY_RD_DATA2)
#define DRAMC_REG_RK2_DUMMY_RD_DATA3 (DRAMC_NAO_BASE + RK2_DUMMY_RD_DATA3)
#define DRAMC_REG_RK2_DQSIENDLY (DRAMC_NAO_BASE + RK2_DQSIENDLY)
#define DRAMC_REG_RK2_DQSIENUIDLY (DRAMC_NAO_BASE + RK2_DQSIENUIDLY)
#define DRAMC_REG_RK2_DQSIENUIDLY_P1 (DRAMC_NAO_BASE + RK2_DQSIENUIDLY_P1)
#define DRAMC_REG_RK2_DQS_STBCALDEC_CNT1 (DRAMC_NAO_BASE + RK2_DQS_STBCALDEC_CNT1)
#define DRAMC_REG_RK2_DQS_STBCALDEC_CNT2 (DRAMC_NAO_BASE + RK2_DQS_STBCALDEC_CNT2)
#define DRAMC_REG_RK2_DQS_STBCALINC_CNT1 (DRAMC_NAO_BASE + RK2_DQS_STBCALINC_CNT1)
#define DRAMC_REG_RK2_DQS_STBCALINC_CNT2 (DRAMC_NAO_BASE + RK2_DQS_STBCALINC_CNT2)
#define DRAMC_REG_RK2_PI_DQ_CAL (DRAMC_NAO_BASE + RK2_PI_DQ_CAL)
#define DRAMC_REG_RK2_DQSG_RETRY_FLAG (DRAMC_NAO_BASE + RK2_DQSG_RETRY_FLAG)
#define DRAMC_REG_RK2_PI_DQM_CAL (DRAMC_NAO_BASE + RK2_PI_DQM_CAL)
#define DRAMC_REG_RK2_DQS0_STBCAL_CNT (DRAMC_NAO_BASE + RK2_DQS0_STBCAL_CNT)
#define DRAMC_REG_RK2_DQS1_STBCAL_CNT (DRAMC_NAO_BASE + RK2_DQS1_STBCAL_CNT)
#define DRAMC_REG_RK2_DQS2_STBCAL_CNT (DRAMC_NAO_BASE + RK2_DQS2_STBCAL_CNT)
#define DRAMC_REG_RK2_DQS3_STBCAL_CNT (DRAMC_NAO_BASE + RK2_DQS3_STBCAL_CNT)

/*======= DRAMC AO ==============*/
#define DRAMC_AO_BASE		DRAMC_AO_BASE_VIRTUAL

#define DRAMC_REG_DDRCONF0 (DRAMC_AO_BASE + DDRCONF0)
#define DRAMC_REG_DRAMCTRL (DRAMC_AO_BASE + DRAMCTRL)
#define DRAMC_REG_MISCTL0 (DRAMC_AO_BASE + MISCTL0)
#define DRAMC_REG_PERFCTL0 (DRAMC_AO_BASE + PERFCTL0)
#define DRAMC_REG_ARBCTL (DRAMC_AO_BASE + ARBCTL)
#define DRAMC_REG_RSTMASK (DRAMC_AO_BASE + RSTMASK)
#define DRAMC_REG_PADCTRL (DRAMC_AO_BASE + PADCTRL)
#define DRAMC_REG_CKECTRL (DRAMC_AO_BASE + CKECTRL)
#define DRAMC_REG_RKCFG (DRAMC_AO_BASE + RKCFG)
#define DRAMC_REG_DRAMC_PD_CTRL (DRAMC_AO_BASE + DRAMC_PD_CTRL)
#define DRAMC_REG_CLKAR (DRAMC_AO_BASE + CLKAR)
#define DRAMC_REG_CLKCTRL (DRAMC_AO_BASE + CLKCTRL)
#define DRAMC_REG_SELFREF_HWSAVE_FLAG (DRAMC_AO_BASE + SELFREF_HWSAVE_FLAG)
#define DRAMC_REG_SREFCTRL (DRAMC_AO_BASE + SREFCTRL)
#define DRAMC_REG_REFCTRL0 (DRAMC_AO_BASE + REFCTRL0)
#define DRAMC_REG_REFCTRL1 (DRAMC_AO_BASE + REFCTRL1)
#define DRAMC_REG_REFRATRE_FILTER (DRAMC_AO_BASE + REFRATRE_FILTER)
#define DRAMC_REG_ZQCS (DRAMC_AO_BASE + ZQCS)
#define DRAMC_REG_MRS (DRAMC_AO_BASE + MRS)
#define DRAMC_REG_SPCMD (DRAMC_AO_BASE + SPCMD)
#define DRAMC_REG_SPCMDCTRL (DRAMC_AO_BASE + SPCMDCTRL)
#define DRAMC_REG_PPR_CTRL (DRAMC_AO_BASE + PPR_CTRL)
#define DRAMC_REG_MPC_OPTION (DRAMC_AO_BASE + MPC_OPTION)
#define DRAMC_REG_REFQUE_CNT (DRAMC_AO_BASE + REFQUE_CNT)
#define DRAMC_REG_HW_MRR_FUN (DRAMC_AO_BASE + HW_MRR_FUN)
#define DRAMC_REG_MRR_BIT_MUX1 (DRAMC_AO_BASE + MRR_BIT_MUX1)
#define DRAMC_REG_MRR_BIT_MUX2 (DRAMC_AO_BASE + MRR_BIT_MUX2)
#define DRAMC_REG_MRR_BIT_MUX3 (DRAMC_AO_BASE + MRR_BIT_MUX3)
#define DRAMC_REG_MRR_BIT_MUX4 (DRAMC_AO_BASE + MRR_BIT_MUX4)
#define DRAMC_REG_TEST2_0 (DRAMC_AO_BASE + TEST2_0)
#define DRAMC_REG_TEST2_1 (DRAMC_AO_BASE + TEST2_1)
#define DRAMC_REG_TEST2_2 (DRAMC_AO_BASE + TEST2_2)
#define DRAMC_REG_TEST2_3 (DRAMC_AO_BASE + TEST2_3)
#define DRAMC_REG_TEST2_4 (DRAMC_AO_BASE + TEST2_4)
#define DRAMC_REG_LBTEST (DRAMC_AO_BASE + LBTEST)
#define DRAMC_REG_CATRAINING1 (DRAMC_AO_BASE + CATRAINING1)
#define DRAMC_REG_CATRAINING2 (DRAMC_AO_BASE + CATRAINING2)
#define DRAMC_REG_WRITE_LEV (DRAMC_AO_BASE + WRITE_LEV)
#define DRAMC_REG_MR_GOLDEN (DRAMC_AO_BASE + MR_GOLDEN)
#define DRAMC_REG_SLP4_TESTMODE (DRAMC_AO_BASE + SLP4_TESTMODE)
#define DRAMC_REG_DQSOSCR (DRAMC_AO_BASE + DQSOSCR)
#define DRAMC_REG_DUMMY_RD (DRAMC_AO_BASE + DUMMY_RD)
#define DRAMC_REG_SHUCTRL (DRAMC_AO_BASE + SHUCTRL)
#define DRAMC_REG_SHUCTRL1 (DRAMC_AO_BASE + SHUCTRL1)
#define DRAMC_REG_SHUCTRL2 (DRAMC_AO_BASE + SHUCTRL2)
#define DRAMC_REG_SHUCTRL3 (DRAMC_AO_BASE + SHUCTRL3)
#define DRAMC_REG_STBCAL (DRAMC_AO_BASE + STBCAL)
#define DRAMC_REG_STBCAL1 (DRAMC_AO_BASE + STBCAL1)
#define DRAMC_REG_EYESCAN (DRAMC_AO_BASE + EYESCAN)
#define DRAMC_REG_DVFSDLL (DRAMC_AO_BASE + DVFSDLL)
#define DRAMC_REG_PRE_TDQSCK1 (DRAMC_AO_BASE + PRE_TDQSCK1)
#define DRAMC_REG_IMPCAL (DRAMC_AO_BASE + IMPCAL)
#define DRAMC_REG_IMPEDAMCE_CTRL1 (DRAMC_AO_BASE + IMPEDAMCE_CTRL1)
#define DRAMC_REG_IMPEDAMCE_CTRL2 (DRAMC_AO_BASE + IMPEDAMCE_CTRL2)
#define DRAMC_REG_IMPEDAMCE_CTRL3 (DRAMC_AO_BASE + IMPEDAMCE_CTRL3)
#define DRAMC_REG_IMPEDAMCE_CTRL4 (DRAMC_AO_BASE + IMPEDAMCE_CTRL4)
#define DRAMC_REG_DRAMC_DBG_SEL1 (DRAMC_AO_BASE + DRAMC_DBG_SEL1)
#define DRAMC_REG_DRAMC_DBG_SEL2 (DRAMC_AO_BASE + DRAMC_DBG_SEL2)
#define DRAMC_REG_RK0_DQSOSC (DRAMC_AO_BASE + RK0_DQSOSC)
#define DRAMC_REG_RK0_DUMMY_RD_WDATA0 (DRAMC_AO_BASE + RK0_DUMMY_RD_WDATA0)
#define DRAMC_REG_RK0_DUMMY_RD_WDATA1 (DRAMC_AO_BASE + RK0_DUMMY_RD_WDATA1)
#define DRAMC_REG_RK0_DUMMY_RD_WDATA2 (DRAMC_AO_BASE + RK0_DUMMY_RD_WDATA2)
#define DRAMC_REG_RK0_DUMMY_RD_WDATA3 (DRAMC_AO_BASE + RK0_DUMMY_RD_WDATA3)
#define DRAMC_REG_RK0_DUMMY_RD_ADR (DRAMC_AO_BASE + RK0_DUMMY_RD_ADR)
#define DRAMC_REG_RK0_DUMMY_RD_BK (DRAMC_AO_BASE + RK0_DUMMY_RD_BK)
#define DRAMC_REG_DRAMC_KEY9 (DRAMC_AO_BASE + DRAMC_KEY9)
#define DRAMC_REG_DRAMC_KEY10 (DRAMC_AO_BASE + DRAMC_KEY10)
#define DRAMC_REG_DRAMC_KEY11 (DRAMC_AO_BASE + DRAMC_KEY11)
#define DRAMC_REG_DRAMC_KEY12 (DRAMC_AO_BASE + DRAMC_KEY12)
#define DRAMC_REG_DRAMC_KEY0 (DRAMC_AO_BASE + DRAMC_KEY0)
#define DRAMC_REG_DRAMC_KEY1 (DRAMC_AO_BASE + DRAMC_KEY1)
#define DRAMC_REG_DRAMC_KEY2 (DRAMC_AO_BASE + DRAMC_KEY2)
#define DRAMC_REG_DRAMC_KEY3 (DRAMC_AO_BASE + DRAMC_KEY3)
#define DRAMC_REG_DRAMC_KEY4 (DRAMC_AO_BASE + DRAMC_KEY4)
#define DRAMC_REG_DRAMC_KEY5 (DRAMC_AO_BASE + DRAMC_KEY5)
#define DRAMC_REG_DRAMC_KEY6 (DRAMC_AO_BASE + DRAMC_KEY6)
#define DRAMC_REG_DRAMC_KEY7 (DRAMC_AO_BASE + DRAMC_KEY7)
#define DRAMC_REG_DRAMC_KEY8 (DRAMC_AO_BASE + DRAMC_KEY8)
#define DRAMC_REG_RK1_DUMMY_RD_WDATA0 (DRAMC_AO_BASE + RK1_DUMMY_RD_WDATA0)
#define DRAMC_REG_RK1_DUMMY_RD_WDATA1 (DRAMC_AO_BASE + RK1_DUMMY_RD_WDATA1)
#define DRAMC_REG_RK1_DUMMY_RD_WDATA2 (DRAMC_AO_BASE + RK1_DUMMY_RD_WDATA2)
#define DRAMC_REG_RK1_DUMMY_RD_WDATA3 (DRAMC_AO_BASE + RK1_DUMMY_RD_WDATA3)
#define DRAMC_REG_RK1_DUMMY_RD_ADR (DRAMC_AO_BASE + RK1_DUMMY_RD_ADR)
#define DRAMC_REG_RK1_DUMMY_RD_BK (DRAMC_AO_BASE + RK1_DUMMY_RD_BK)
#define DRAMC_REG_SHU_ACTIM0 (DRAMC_AO_BASE + SHU_ACTIM0)
#define DRAMC_REG_SHU_ACTIM1 (DRAMC_AO_BASE + SHU_ACTIM1)
#define DRAMC_REG_SHU_ACTIM2 (DRAMC_AO_BASE + SHU_ACTIM2)
#define DRAMC_REG_SHU_ACTIM3 (DRAMC_AO_BASE + SHU_ACTIM3)
#define DRAMC_REG_SHU_ACTIM4 (DRAMC_AO_BASE + SHU_ACTIM4)
#define DRAMC_REG_SHU_ACTIM5 (DRAMC_AO_BASE + SHU_ACTIM5)
#define DRAMC_REG_SHU_ACTIM6 (DRAMC_AO_BASE + SHU_ACTIM6)
#define DRAMC_REG_SHU_ACTIM_XRT (DRAMC_AO_BASE + SHU_ACTIM_XRT)
#define DRAMC_REG_SHU_AC_TIME_05T (DRAMC_AO_BASE + SHU_AC_TIME_05T)
#define DRAMC_REG_SHU_AC_DERATING0 (DRAMC_AO_BASE + SHU_AC_DERATING0)
#define DRAMC_REG_SHU_AC_DERATING1 (DRAMC_AO_BASE + SHU_AC_DERATING1)
#define DRAMC_REG_SHU_AC_DERATING_05T (DRAMC_AO_BASE + SHU_AC_DERATING_05T)
#define DRAMC_REG_SHU_CONF0 (DRAMC_AO_BASE + SHU_CONF0)
#define DRAMC_REG_SHU_CONF1 (DRAMC_AO_BASE + SHU_CONF1)
#define DRAMC_REG_SHU_CONF2 (DRAMC_AO_BASE + SHU_CONF2)
#define DRAMC_REG_SHU_CONF3 (DRAMC_AO_BASE + SHU_CONF3)
#define DRAMC_REG_SHU_CONF4 (DRAMC_AO_BASE + SHU_CONF4)
#define DRAMC_REG_SHU_RANKCTL (DRAMC_AO_BASE + SHU_RANKCTL)
#define DRAMC_REG_SHU_CKECTRL (DRAMC_AO_BASE + SHU_CKECTRL)
#define DRAMC_REG_SHU_ODTCTRL (DRAMC_AO_BASE + SHU_ODTCTRL)
#define DRAMC_REG_SHU_IMPCAL1 (DRAMC_AO_BASE + SHU_IMPCAL1)
#define DRAMC_REG_SHU1_DQSOSC_PRD (DRAMC_AO_BASE + SHU1_DQSOSC_PRD)
#define DRAMC_REG_SHU_DQSOSCR (DRAMC_AO_BASE + SHU_DQSOSCR)
#define DRAMC_REG_SHU_DQSOSCR2 (DRAMC_AO_BASE + SHU_DQSOSCR2)
#define DRAMC_REG_SHU_RODTENSTB (DRAMC_AO_BASE + SHU_RODTENSTB)
#define DRAMC_REG_SHU_PIPE (DRAMC_AO_BASE + SHU_PIPE)
#define DRAMC_REG_SHU_TEST1 (DRAMC_AO_BASE + SHU_TEST1)
#define DRAMC_REG_SHU_SELPH_CA1 (DRAMC_AO_BASE + SHU_SELPH_CA1)
#define DRAMC_REG_SHU_SELPH_CA2 (DRAMC_AO_BASE + SHU_SELPH_CA2)
#define DRAMC_REG_SHU_SELPH_CA3 (DRAMC_AO_BASE + SHU_SELPH_CA3)
#define DRAMC_REG_SHU_SELPH_CA4 (DRAMC_AO_BASE + SHU_SELPH_CA4)
#define DRAMC_REG_SHU_SELPH_CA5 (DRAMC_AO_BASE + SHU_SELPH_CA5)
#define DRAMC_REG_SHU_SELPH_CA6 (DRAMC_AO_BASE + SHU_SELPH_CA6)
#define DRAMC_REG_SHU_SELPH_CA7 (DRAMC_AO_BASE + SHU_SELPH_CA7)
#define DRAMC_REG_SHU_SELPH_CA8 (DRAMC_AO_BASE + SHU_SELPH_CA8)
#define DRAMC_REG_SHU_SELPH_DQS0 (DRAMC_AO_BASE + SHU_SELPH_DQS0)
#define DRAMC_REG_SHU_SELPH_DQS1 (DRAMC_AO_BASE + SHU_SELPH_DQS1)
#define DRAMC_REG_SHU1_DRVING1 (DRAMC_AO_BASE + SHU1_DRVING1)
#define DRAMC_REG_SHU1_DRVING2 (DRAMC_AO_BASE + SHU1_DRVING2)
#define DRAMC_REG_SHU1_DRVING3 (DRAMC_AO_BASE + SHU1_DRVING3)
#define DRAMC_REG_SHU1_DRVING4 (DRAMC_AO_BASE + SHU1_DRVING4)
#define DRAMC_REG_SHU1_DRVING5 (DRAMC_AO_BASE + SHU1_DRVING5)
#define DRAMC_REG_SHU1_DRVING6 (DRAMC_AO_BASE + SHU1_DRVING6)
#define DRAMC_REG_SHU1_WODT (DRAMC_AO_BASE + SHU1_WODT)
#define DRAMC_REG_SHU1_DQSG (DRAMC_AO_BASE + SHU1_DQSG)
#define DRAMC_REG_SHU_SCINTV (DRAMC_AO_BASE + SHU_SCINTV)
#define DRAMC_REG_SHU_MISC (DRAMC_AO_BASE + SHU_MISC)
#define DRAMC_REG_SHU_HWSET_MR2 (DRAMC_AO_BASE + SHU_HWSET_MR2)
#define DRAMC_REG_SHU_HWSET_MR13 (DRAMC_AO_BASE + SHU_HWSET_MR13)
#define DRAMC_REG_SHU_HWSET_VRCG (DRAMC_AO_BASE + SHU_HWSET_VRCG)
#define DRAMC_REG_SHURK0_DQSCTL (DRAMC_AO_BASE + SHURK0_DQSCTL)
#define DRAMC_REG_SHURK0_DQSIEN (DRAMC_AO_BASE + SHURK0_DQSIEN)
#define DRAMC_REG_SHURK0_DQSCAL (DRAMC_AO_BASE + SHURK0_DQSCAL)
#define DRAMC_REG_SHU1RK0_PI (DRAMC_AO_BASE + SHU1RK0_PI)
#define DRAMC_REG_SHU1RK0_DQSOSC (DRAMC_AO_BASE + SHU1RK0_DQSOSC)
#define DRAMC_REG_SHURK0_SELPH_ODTEN0 (DRAMC_AO_BASE + SHURK0_SELPH_ODTEN0)
#define DRAMC_REG_SHURK0_SELPH_ODTEN1 (DRAMC_AO_BASE + SHURK0_SELPH_ODTEN1)
#define DRAMC_REG_SHURK0_SELPH_DQSG0 (DRAMC_AO_BASE + SHURK0_SELPH_DQSG0)
#define DRAMC_REG_SHURK0_SELPH_DQSG1 (DRAMC_AO_BASE + SHURK0_SELPH_DQSG1)
#define DRAMC_REG_SHURK0_SELPH_DQ0 (DRAMC_AO_BASE + SHURK0_SELPH_DQ0)
#define DRAMC_REG_SHURK0_SELPH_DQ1 (DRAMC_AO_BASE + SHURK0_SELPH_DQ1)
#define DRAMC_REG_SHURK0_SELPH_DQ2 (DRAMC_AO_BASE + SHURK0_SELPH_DQ2)
#define DRAMC_REG_SHURK0_SELPH_DQ3 (DRAMC_AO_BASE + SHURK0_SELPH_DQ3)
#define DRAMC_REG_SHURK1_DQSCTL (DRAMC_AO_BASE + SHURK1_DQSCTL)
#define DRAMC_REG_SHURK1_DQSIEN (DRAMC_AO_BASE + SHURK1_DQSIEN)
#define DRAMC_REG_SHURK1_DQSCAL (DRAMC_AO_BASE + SHURK1_DQSCAL)
#define DRAMC_REG_SHU1RK1_PI (DRAMC_AO_BASE + SHU1RK1_PI)
#define DRAMC_REG_SHU1RK1_DQSOSC (DRAMC_AO_BASE + SHU1RK1_DQSOSC)
#define DRAMC_REG_SHURK1_SELPH_ODTEN0 (DRAMC_AO_BASE + SHURK1_SELPH_ODTEN0)
#define DRAMC_REG_SHURK1_SELPH_ODTEN1 (DRAMC_AO_BASE + SHURK1_SELPH_ODTEN1)
#define DRAMC_REG_SHURK1_SELPH_DQSG0 (DRAMC_AO_BASE + SHURK1_SELPH_DQSG0)
#define DRAMC_REG_SHURK1_SELPH_DQSG1 (DRAMC_AO_BASE + SHURK1_SELPH_DQSG1)
#define DRAMC_REG_SHURK1_SELPH_DQ0 (DRAMC_AO_BASE + SHURK1_SELPH_DQ0)
#define DRAMC_REG_SHURK1_SELPH_DQ1 (DRAMC_AO_BASE + SHURK1_SELPH_DQ1)
#define DRAMC_REG_SHURK1_SELPH_DQ2 (DRAMC_AO_BASE + SHURK1_SELPH_DQ2)
#define DRAMC_REG_SHURK1_SELPH_DQ3 (DRAMC_AO_BASE + SHURK1_SELPH_DQ3)
#define DRAMC_REG_SHU1RK1_DQS2DQ_CAL1 (DRAMC_AO_BASE + SHU1RK1_DQS2DQ_CAL1)
#define DRAMC_REG_SHU1RK1_DQS2DQ_CAL2 (DRAMC_AO_BASE + SHU1RK1_DQS2DQ_CAL2)
#define DRAMC_REG_SHU1RK1_DQS2DQ_CAL3 (DRAMC_AO_BASE + SHU1RK1_DQS2DQ_CAL3)
#define DRAMC_REG_SHU1RK1_DQS2DQ_CAL4 (DRAMC_AO_BASE + SHU1RK1_DQS2DQ_CAL4)
#define DRAMC_REG_SHU1RK1_DQS2DQ_CAL5 (DRAMC_AO_BASE + SHU1RK1_DQS2DQ_CAL5)
#define DRAMC_REG_DMPINMUX_CTRL (DRAMC_AO_BASE + DMPINMUX_CTRL)

/*================ DDRPHY ================*/
#define DDRPHY_BASE_ADDR	DDRPHY_BASE_ADDR_VIRTUAL

#define DDRPHY_PLL1 (DDRPHY_BASE_ADDR + PLL1)
#define DDRPHY_PLL2 (DDRPHY_BASE_ADDR + PLL2)
#define DDRPHY_PLL3 (DDRPHY_BASE_ADDR + PLL3)
#define DDRPHY_PLL4 (DDRPHY_BASE_ADDR + PLL4)
#define DDRPHY_PLL5 (DDRPHY_BASE_ADDR + PLL5)
#define DDRPHY_PLL6 (DDRPHY_BASE_ADDR + PLL6)
#define DDRPHY_PLL7 (DDRPHY_BASE_ADDR + PLL7)
#define DDRPHY_PLL8 (DDRPHY_BASE_ADDR + PLL8)
#define DDRPHY_B0_DLL_ARPI0 (DDRPHY_BASE_ADDR + B0_DLL_ARPI0)
#define DDRPHY_B0_DLL_ARPI1 (DDRPHY_BASE_ADDR + B0_DLL_ARPI1)
#define DDRPHY_B0_DLL_ARPI2 (DDRPHY_BASE_ADDR + B0_DLL_ARPI2)
#define DDRPHY_B0_DLL_ARPI3 (DDRPHY_BASE_ADDR + B0_DLL_ARPI3)
#define DDRPHY_B0_DLL_ARPI4 (DDRPHY_BASE_ADDR + B0_DLL_ARPI4)
#define DDRPHY_B0_DLL_ARPI5 (DDRPHY_BASE_ADDR + B0_DLL_ARPI5)
#define DDRPHY_B0_DQ0 (DDRPHY_BASE_ADDR + B0_DQ0)
#define DDRPHY_B0_DQ1 (DDRPHY_BASE_ADDR + B0_DQ1)
#define DDRPHY_B0_DQ2 (DDRPHY_BASE_ADDR + B0_DQ2)
#define DDRPHY_B0_DQ3 (DDRPHY_BASE_ADDR + B0_DQ3)
#define DDRPHY_B0_DQ4 (DDRPHY_BASE_ADDR + B0_DQ4)
#define DDRPHY_B0_DQ5 (DDRPHY_BASE_ADDR + B0_DQ5)
#define DDRPHY_B0_DQ6 (DDRPHY_BASE_ADDR + B0_DQ6)
#define DDRPHY_B0_DQ7 (DDRPHY_BASE_ADDR + B0_DQ7)
#define DDRPHY_B0_DQ8 (DDRPHY_BASE_ADDR + B0_DQ8)
#define DDRPHY_B0_CKGEN_DLL0 (DDRPHY_BASE_ADDR + B0_CKGEN_DLL0)
#define DDRPHY_B0_CKGEN_DLL1 (DDRPHY_BASE_ADDR + B0_CKGEN_DLL1)
#define DDRPHY_B0_DQ9 (DDRPHY_BASE_ADDR + B0_DQ9)
#define DDRPHY_B1_DLL_ARPI0 (DDRPHY_BASE_ADDR + B1_DLL_ARPI0)
#define DDRPHY_B1_DLL_ARPI1 (DDRPHY_BASE_ADDR + B1_DLL_ARPI1)
#define DDRPHY_B1_DLL_ARPI2 (DDRPHY_BASE_ADDR + B1_DLL_ARPI2)
#define DDRPHY_B1_DLL_ARPI3 (DDRPHY_BASE_ADDR + B1_DLL_ARPI3)
#define DDRPHY_B1_DLL_ARPI4 (DDRPHY_BASE_ADDR + B1_DLL_ARPI4)
#define DDRPHY_B1_DLL_ARPI5 (DDRPHY_BASE_ADDR + B1_DLL_ARPI5)
#define DDRPHY_B1_DQ0 (DDRPHY_BASE_ADDR + B1_DQ0)
#define DDRPHY_B1_DQ1 (DDRPHY_BASE_ADDR + B1_DQ1)
#define DDRPHY_B1_DQ2 (DDRPHY_BASE_ADDR + B1_DQ2)
#define DDRPHY_B1_DQ3 (DDRPHY_BASE_ADDR + B1_DQ3)
#define DDRPHY_B1_DQ4 (DDRPHY_BASE_ADDR + B1_DQ4)
#define DDRPHY_B1_DQ5 (DDRPHY_BASE_ADDR + B1_DQ5)
#define DDRPHY_B1_DQ6 (DDRPHY_BASE_ADDR + B1_DQ6)
#define DDRPHY_B1_DQ7 (DDRPHY_BASE_ADDR + B1_DQ7)
#define DDRPHY_B1_DQ8 (DDRPHY_BASE_ADDR + B1_DQ8)
#define DDRPHY_SEL_MUX0 (DDRPHY_BASE_ADDR + SEL_MUX0)
#define DDRPHY_SEL_MUX1 (DDRPHY_BASE_ADDR + SEL_MUX1)
#define DDRPHY_CA_DLL_ARPI0 (DDRPHY_BASE_ADDR + CA_DLL_ARPI0)
#define DDRPHY_CA_DLL_ARPI1 (DDRPHY_BASE_ADDR + CA_DLL_ARPI1)
#define DDRPHY_CA_DLL_ARPI2 (DDRPHY_BASE_ADDR + CA_DLL_ARPI2)
#define DDRPHY_CA_DLL_ARPI3 (DDRPHY_BASE_ADDR + CA_DLL_ARPI3)
#define DDRPHY_CA_DLL_ARPI4 (DDRPHY_BASE_ADDR + CA_DLL_ARPI4)
#define DDRPHY_CA_DLL_ARPI5 (DDRPHY_BASE_ADDR + CA_DLL_ARPI5)
#define DDRPHY_CA_CMD0 (DDRPHY_BASE_ADDR + CA_CMD0)
#define DDRPHY_CA_CMD1 (DDRPHY_BASE_ADDR + CA_CMD1)
#define DDRPHY_CA_CMD2 (DDRPHY_BASE_ADDR + CA_CMD2)
#define DDRPHY_CA_CMD3 (DDRPHY_BASE_ADDR + CA_CMD3)
#define DDRPHY_CA_CMD4 (DDRPHY_BASE_ADDR + CA_CMD4)
#define DDRPHY_CA_CMD5 (DDRPHY_BASE_ADDR + CA_CMD5)
#define DDRPHY_CA_CMD6 (DDRPHY_BASE_ADDR + CA_CMD6)
#define DDRPHY_CA_CMD7 (DDRPHY_BASE_ADDR + CA_CMD7)
#define DDRPHY_CA_CMD8 (DDRPHY_BASE_ADDR + CA_CMD8)
#define DDRPHY_CA_CMD9 (DDRPHY_BASE_ADDR + CA_CMD9)
#define DDRPHY_CA_CKGEN_DLL0 (DDRPHY_BASE_ADDR + CA_CKGEN_DLL0)
#define DDRPHY_CA_CKGEN_DLL1 (DDRPHY_BASE_ADDR + CA_CKGEN_DLL1)
#define DDRPHY_MISC_VREF_CTRL (DDRPHY_BASE_ADDR + MISC_VREF_CTRL)
#define DDRPHY_MISC_IMP_CTRL0 (DDRPHY_BASE_ADDR + MISC_IMP_CTRL0)
#define DDRPHY_MISC_IMP_CTRL1 (DDRPHY_BASE_ADDR + MISC_IMP_CTRL1)
#define DDRPHY_MISC_SHU_OPT (DDRPHY_BASE_ADDR + MISC_SHU_OPT)
#define DDRPHY_MISC_SPM_CTRL0 (DDRPHY_BASE_ADDR + MISC_SPM_CTRL0)
#define DDRPHY_MISC_SPM_CTRL1 (DDRPHY_BASE_ADDR + MISC_SPM_CTRL1)
#define DDRPHY_MISC_SPM_CTRL2 (DDRPHY_BASE_ADDR + MISC_SPM_CTRL2)
#define DDRPHY_MISC_CG_CTRL0 (DDRPHY_BASE_ADDR + MISC_CG_CTRL0)

/* cc notes: from Azalea REVIEW??. User define */
#define MISC_CG_CTRL0_RG_APB_CLK_FREQ_MSK_DIS	ffld(1,29,AC_MSKB3)
#define MISC_CG_CTRL0_RG_DA_RREF_CK_SEL		ffld(1,28,AC_MSKB3)
#define MISC_CG_CTRL0_RG_CG_INFRA_OFF_DISABLE	ffld(1,19,AC_MSKB2)
#define MISC_CG_CTRL0_RG_CG_IDLE_SYNC_EN	ffld(1,18,AC_MSKB2)
#define MISC_CG_CTRL0_RG_CG_RX_COMB1_OFF_DISABLE	ffld(1,17,AC_MSKB2)
#define MISC_CG_CTRL0_RG_CG_RX_COMB0_OFF_DISABLE	ffld(1,16,AC_MSKB2)
#define MISC_CG_CTRL0_RG_CG_RX_CMD_OFF_DISABLE	ffld(1,15,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_COMB1_OFF_DISABLE	ffld(1,14,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_COMB0_OFF_DISABLE	ffld(1,13,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_CMD_OFF_DISABLE		ffld(1,12,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_COMB_OFF_DISABLE	ffld(1,11,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_PHY_OFF_DISABLE		ffld(1,10,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_DRAMC_OFF_DISABLE	ffld(1,9,AC_MSKB1)
#define MISC_CG_CTRL0_RG_CG_EMI_OFF_DISABLE		ffld(1,8,AC_MSKB1)
#define MISC_CG_CTRL0_CLK_MEM_INV		ffld(1,6,AC_MSKB0)
#define MISC_CG_CTRL0_CLK_MEM_SEL		ffld(2,4,AC_MSKB0)
#define MISC_CG_CTRL0_W_CHG_MEM			ffld(1,0,AC_MSKB0)

#define DDRPHY_MISC_CG_CTRL1 (DDRPHY_BASE_ADDR + MISC_CG_CTRL1)
#define DDRPHY_MISC_CG_CTRL2 (DDRPHY_BASE_ADDR + MISC_CG_CTRL2)
#define DDRPHY_MISC_CG_CTRL3 (DDRPHY_BASE_ADDR + MISC_CG_CTRL3)
#define DDRPHY_MISC_CG_CTRL4 (DDRPHY_BASE_ADDR + MISC_CG_CTRL4)
#define DDRPHY_MISC_CG_CTRL5 (DDRPHY_BASE_ADDR + MISC_CG_CTRL5)
#define DDRPHY_MISC_CTRL0 (DDRPHY_BASE_ADDR + MISC_CTRL0)
#define DDRPHY_MISC_CTRL1 (DDRPHY_BASE_ADDR + MISC_CTRL1)
#define DDRPHY_MISC_CTRL2 (DDRPHY_BASE_ADDR + MISC_CTRL2)
#define DDRPHY_MISC_CTRL3 (DDRPHY_BASE_ADDR + MISC_CTRL3)
#define DDRPHY_MISC_PHY_RGS0 (DDRPHY_BASE_ADDR + MISC_PHY_RGS0)
#define DDRPHY_MISC_PHY_RGS1 (DDRPHY_BASE_ADDR + MISC_PHY_RGS1)
#define DDRPHY_MISC_PHY_RGS2 (DDRPHY_BASE_ADDR + MISC_PHY_RGS2)
#define DDRPHY_MISC_PHY_RGS3 (DDRPHY_BASE_ADDR + MISC_PHY_RGS3)
#define DDRPHY_MISC_STBERR_RK0_R (DDRPHY_BASE_ADDR + MISC_STBERR_RK0_R)
#define DDRPHY_MISC_STBERR_RK1_R (DDRPHY_BASE_ADDR + MISC_STBERR_RK1_R)
#define DDRPHY_MISC_STBERR_RK0_F (DDRPHY_BASE_ADDR + MISC_STBERR_RK0_F)
#define DDRPHY_MISC_STBERR_RK1_F (DDRPHY_BASE_ADDR + MISC_STBERR_RK1_F)
#define DDRPHY_MISC_STBERR_RK2_R (DDRPHY_BASE_ADDR + MISC_STBERR_RK2_R)
#define DDRPHY_MISC_STBERR_RK2_F (DDRPHY_BASE_ADDR + MISC_STBERR_RK2_F)
#define DDRPHY_MISC_DQO1 (DDRPHY_BASE_ADDR + MISC_DQO1)
#define DDRPHY_MISC_CAO1 (DDRPHY_BASE_ADDR + MISC_CAO1)
#define DDRPHY_MISC_RXDVS0 (DDRPHY_BASE_ADDR + MISC_RXDVS0)
#define DDRPHY_MISC_RXDVS1 (DDRPHY_BASE_ADDR + MISC_RXDVS1)
#define DDRPHY_MISC_RXDVS2 (DDRPHY_BASE_ADDR + MISC_RXDVS2)
#define DDRPHY_B1_RXDVS0 (DDRPHY_BASE_ADDR + B1_RXDVS0)
#define DDRPHY_SHU1_B0_DQ0 (DDRPHY_BASE_ADDR + SHU1_B0_DQ0)
#define DDRPHY_SHU1_B0_DQ1 (DDRPHY_BASE_ADDR + SHU1_B0_DQ1)
#define DDRPHY_SHU1_B0_DQ2 (DDRPHY_BASE_ADDR + SHU1_B0_DQ2)
#define DDRPHY_SHU1_B0_DQ3 (DDRPHY_BASE_ADDR + SHU1_B0_DQ3)
#define DDRPHY_SHU1_B0_DQ4 (DDRPHY_BASE_ADDR + SHU1_B0_DQ4)
#define DDRPHY_SHU1_B0_DQ5 (DDRPHY_BASE_ADDR + SHU1_B0_DQ5)
#define DDRPHY_SHU1_B0_DQ6 (DDRPHY_BASE_ADDR + SHU1_B0_DQ6)
#define DDRPHY_SHU1_B0_DQ7 (DDRPHY_BASE_ADDR + SHU1_B0_DQ7)
#define DDRPHY_SHU1_B1_DQ0 (DDRPHY_BASE_ADDR + SHU1_B1_DQ0)
#define DDRPHY_SHU1_B1_DQ1 (DDRPHY_BASE_ADDR + SHU1_B1_DQ1)
#define DDRPHY_SHU1_B1_DQ2 (DDRPHY_BASE_ADDR + SHU1_B1_DQ2)
#define DDRPHY_SHU1_B1_DQ3 (DDRPHY_BASE_ADDR + SHU1_B1_DQ3)
#define DDRPHY_SHU1_B1_DQ4 (DDRPHY_BASE_ADDR + SHU1_B1_DQ4)
#define DDRPHY_SHU1_B1_DQ5 (DDRPHY_BASE_ADDR + SHU1_B1_DQ5)
#define DDRPHY_SHU1_B1_DQ6 (DDRPHY_BASE_ADDR + SHU1_B1_DQ6)
#define DDRPHY_SHU1_B1_DQ7 (DDRPHY_BASE_ADDR + SHU1_B1_DQ7)
#define DDRPHY_SHU1_CA_CMD0 (DDRPHY_BASE_ADDR + SHU1_CA_CMD0)
#define DDRPHY_SHU1_CA_CMD1 (DDRPHY_BASE_ADDR + SHU1_CA_CMD1)
#define DDRPHY_SHU1_CA_CMD2 (DDRPHY_BASE_ADDR + SHU1_CA_CMD2)
#define DDRPHY_SHU1_CA_CMD3 (DDRPHY_BASE_ADDR + SHU1_CA_CMD3)
#define DDRPHY_SHU1_CA_CMD4 (DDRPHY_BASE_ADDR + SHU1_CA_CMD4)
#define DDRPHY_SHU1_CA_CMD5 (DDRPHY_BASE_ADDR + SHU1_CA_CMD5)
#define DDRPHY_SHU1_CA_CMD6 (DDRPHY_BASE_ADDR + SHU1_CA_CMD6)
#define DDRPHY_SHU1_CA_CMD7 (DDRPHY_BASE_ADDR + SHU1_CA_CMD7)
/* cc add. User define */
	#define SHU1_CA_CMD7_RG_ARCMD_REV_CKE_DRVN ffld(5, 8, AC_MSKB1)

#define DDRPHY_SHU1_PLL0 (DDRPHY_BASE_ADDR + SHU1_PLL0)
#define DDRPHY_SHU1_PLL1 (DDRPHY_BASE_ADDR + SHU1_PLL1)
#define DDRPHY_SHU1_PLL4 (DDRPHY_BASE_ADDR + SHU1_PLL4)
#define DDRPHY_SHU1_PLL5 (DDRPHY_BASE_ADDR + SHU1_PLL5)
#define DDRPHY_SHU1_PLL6 (DDRPHY_BASE_ADDR + SHU1_PLL6)
#define DDRPHY_SHU1_PLL7 (DDRPHY_BASE_ADDR + SHU1_PLL7)
#define DDRPHY_SHU1_PLL8 (DDRPHY_BASE_ADDR + SHU1_PLL8)
#define DDRPHY_SHU1_PLL9 (DDRPHY_BASE_ADDR + SHU1_PLL9)
#define DDRPHY_SHU1_PLL10 (DDRPHY_BASE_ADDR + SHU1_PLL10)
#define DDRPHY_SHU1_PLL11 (DDRPHY_BASE_ADDR + SHU1_PLL11)
#define DDRPHY_SHU1_R0_B0_DQ0 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ0)
#define DDRPHY_SHU1_R0_B0_DQ1 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ1)
#define DDRPHY_SHU1_R0_B0_DQ2 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ2)
#define DDRPHY_SHU1_R0_B0_DQ3 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ3)
#define DDRPHY_SHU1_R0_B0_DQ4 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ4)
#define DDRPHY_SHU1_R0_B0_DQ5 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ5)
#define DDRPHY_SHU1_R0_B0_DQ6 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ6)
#define DDRPHY_SHU1_R0_B0_DQ7 (DDRPHY_BASE_ADDR + SHU1_R0_B0_DQ7)
#define DDRPHY_SHU1_R0_B1_DQ0 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ0)
#define DDRPHY_SHU1_R0_B1_DQ1 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ1)
#define DDRPHY_SHU1_R0_B1_DQ2 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ2)
#define DDRPHY_SHU1_R0_B1_DQ3 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ3)
#define DDRPHY_SHU1_R0_B1_DQ4 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ4)
#define DDRPHY_SHU1_R0_B1_DQ5 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ5)
#define DDRPHY_SHU1_R0_B1_DQ6 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ6)
#define DDRPHY_SHU1_R0_B1_DQ7 (DDRPHY_BASE_ADDR + SHU1_R0_B1_DQ7)
#define DDRPHY_SHU1_R0_CA_CMD0 (DDRPHY_BASE_ADDR + SHU1_R0_CA_CMD0)
#define DDRPHY_SHU1_R0_CA_CMD1 (DDRPHY_BASE_ADDR + SHU1_R0_CA_CMD1)
#define DDRPHY_SHU1_R0_CA_CMD9 (DDRPHY_BASE_ADDR + SHU1_R0_CA_CMD9)
#define DDRPHY_SHU1_R0_CA_CMD10 (DDRPHY_BASE_ADDR + SHU1_R0_CA_CMD10)
#define DDRPHY_SHU1_R1_B0_DQ0 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ0)
#define DDRPHY_SHU1_R1_B0_DQ1 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ1)
#define DDRPHY_SHU1_R1_B0_DQ2 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ2)
#define DDRPHY_SHU1_R1_B0_DQ3 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ3)
#define DDRPHY_SHU1_R1_B0_DQ4 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ4)
#define DDRPHY_SHU1_R1_B0_DQ5 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ5)
#define DDRPHY_SHU1_R1_B0_DQ6 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ6)
#define DDRPHY_SHU1_R1_B0_DQ7 (DDRPHY_BASE_ADDR + SHU1_R1_B0_DQ7)
#define DDRPHY_SHU1_R1_B1_DQ0 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ0)
#define DDRPHY_SHU1_R1_B1_DQ1 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ1)
#define DDRPHY_SHU1_R1_B1_DQ2 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ2)
#define DDRPHY_SHU1_R1_B1_DQ3 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ3)
#define DDRPHY_SHU1_R1_B1_DQ4 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ4)
#define DDRPHY_SHU1_R1_B1_DQ5 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ5)
#define DDRPHY_SHU1_R1_B1_DQ6 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ6)
#define DDRPHY_SHU1_R1_B1_DQ7 (DDRPHY_BASE_ADDR + SHU1_R1_B1_DQ7)
#define DDRPHY_SHU1_R1_CA_CMD0 (DDRPHY_BASE_ADDR + SHU1_R1_CA_CMD0)
#define DDRPHY_SHU1_R1_CA_CMD1 (DDRPHY_BASE_ADDR + SHU1_R1_CA_CMD1)
#define DDRPHY_SHU1_R1_CA_CMD9 (DDRPHY_BASE_ADDR + SHU1_R1_CA_CMD9)
#define DDRPHY_SHU1_R1_CA_CMD10 (DDRPHY_BASE_ADDR + SHU1_R1_CA_CMD10)
#define DDRPHY_TOGGLE_CNT (DDRPHY_BASE_ADDR + TOGGLE_CNT)
#define DDRPHY_DQ_ERR_CNT0 (DDRPHY_BASE_ADDR + DQ_ERR_CNT0)
#define DDRPHY_DQ_ERR_CNT1 (DDRPHY_BASE_ADDR + DQ_ERR_CNT1)
#define DDRPHY_DQ_ERR_CNT2 (DDRPHY_BASE_ADDR + DQ_ERR_CNT2)
#define DDRPHY_DQ_ERR_CNT3 (DDRPHY_BASE_ADDR + DQ_ERR_CNT3)
#define DDRPHY_DQS0_ERR_CNT (DDRPHY_BASE_ADDR + DQS0_ERR_CNT)
#define DDRPHY_DQS1_ERR_CNT (DDRPHY_BASE_ADDR + DQS1_ERR_CNT)
#define DDRPHY_DQS2_ERR_CNT (DDRPHY_BASE_ADDR + DQS2_ERR_CNT)
#define DDRPHY_DQS3_ERR_CNT (DDRPHY_BASE_ADDR + DQS3_ERR_CNT)

#endif
/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */
#include <reg.h>
#include <platform/pmic_wrap_init.h>
#include <platform/mtk_timer.h>
#include <platform/pmic.h>
#include "platform/mt_reg_base.h"

#define PWRAP_INT_GPIO_BASE (IO_PHYS+0x00005610)

#define pwrap_log(fmt, arg ...)   printf("[PWRAP] " fmt, ## arg)
#define pwrap_err(fmt, arg ...)	  printf("[PWRAP] ERROR,line=%d" fmt, \
					__LINE__, ## arg)

static s32 pwrap_read_nochk(u32 adr, u32 *rdata);
static s32 pwrap_write_nochk(u32 adr, u32 wdata);

extern lk_bigtime_t current_time_hires(void);

/* define macro and inline function (for do while loop) */

typedef u32 (*loop_condition_fp)(u32);

static inline u32 wait_for_fsm_vldclr(u32 x)
{
	return ((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_WFVLDCLR;
}

static inline u32 wait_for_sync(u32 x)
{
	return ((x >> RDATA_SYNC_IDLE_SHIFT) & RDATA_SYNC_IDLE_MASK) !=
		WACS_SYNC_IDLE;
}

static inline u32 wait_for_idle_and_sync(u32 x)
{
	return ((((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_IDLE) || (((x >> RDATA_SYNC_IDLE_SHIFT) &
		RDATA_SYNC_IDLE_MASK)!= WACS_SYNC_IDLE));
}

static inline u32 wait_for_cipher_ready(u32 x)
{
	return x != 3;
}

static inline u32 wait_for_state_idle(u32 timeout_us, void *wacs_register,
				      void *wacs_vldclr_register,
				      u32 *read_reg)
{
	u32 reg_rdata;
	lk_bigtime_t start_time = 0, timeout_time = 0;

	start_time = current_time_hires();
	timeout_time = start_time + timeout_us;

	do {
		reg_rdata = read32((wacs_register));
		/* if last read command timeout,clear vldclr bit
		   read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;
		   write:FSM_REQ-->idle */
		switch (((reg_rdata >> RDATA_WACS_FSM_SHIFT) &
			RDATA_WACS_FSM_MASK)) {
		case WACS_FSM_WFVLDCLR:
			write32(wacs_vldclr_register, 1);
			pwrap_err("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
			break;
		case WACS_FSM_WFDLE:
			pwrap_err("WACS_FSM = WACS_FSM_WFDLE\n");
			break;
		case WACS_FSM_REQ:
			pwrap_err("WACS_FSM = WACS_FSM_REQ\n");
			break;
		default:
			break;
		}

		if (current_time_hires() >= timeout_time)
			return E_PWR_WAIT_IDLE_TIMEOUT;

	} while (((reg_rdata >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		 WACS_FSM_IDLE);        /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static inline u32 wait_for_state_ready(loop_condition_fp fp, u32 timeout_us,
				       void *wacs_register, u32 *read_reg)
{
	u32 reg_rdata;
	lk_bigtime_t start_time = 0, timeout_time = 0;

	start_time = current_time_hires();
	timeout_time = start_time + timeout_us;

	do {
		reg_rdata = read32((wacs_register));

		if (current_time_hires() >= timeout_time) {
			pwrap_err("timeout when waiting for idle\n");
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
	} while (fp(reg_rdata));        /* IDLE State */
	if (read_reg)
		*read_reg = reg_rdata;
	return 0;
}

static s32 pwrap_wacs(u32 write, u32 adr, u32 wdata, u32 *rdata, u32 init_check)
{
	u32 reg_rdata = 0;
	u32 wacs_write = 0;
	u32 wacs_adr = 0;
	u32 wacs_cmd = 0;
	u32 return_value = 0;

	if (init_check) {
		reg_rdata = read32(&mt8518_pwrap->wacs2_rdata);
		/* Prevent someone to used pwrap before pwrap init */
		if (((reg_rdata >> RDATA_INIT_DONE_SHIFT) &
		    RDATA_INIT_DONE_MASK) != WACS_INIT_DONE) {
			pwrap_err("initialization isn't finished \n");
			return E_PWR_NOT_INIT_DONE;
		}
	}
	reg_rdata = 0;
	/* Check IDLE in advance */
	return_value = wait_for_state_idle(TIMEOUT_WAIT_IDLE_US,
					   &mt8518_pwrap->wacs2_rdata,
					   &mt8518_pwrap->wacs2_vldclr,
					   0);
	if (return_value != 0) {
		pwrap_err("wait_for_fsm_idle fail,return_value=%d\n",
			  return_value);
		return E_PWR_WAIT_IDLE_TIMEOUT;
	}
	wacs_write = write << 31;
	wacs_adr = (adr >> 1) << 16;
	wacs_cmd = wacs_write | wacs_adr | wdata;

	write32(&mt8518_pwrap->wacs2_cmd, wacs_cmd);
	if (write == 0) {
		if (NULL == rdata) {
			pwrap_err("rdata is a NULL pointer\n");
			return E_PWR_INVALID_ARG;
		}
		return_value = wait_for_state_ready(wait_for_fsm_vldclr,
						    TIMEOUT_READ_US,
						    &mt8518_pwrap->wacs2_rdata,
						    &reg_rdata);
		if (return_value != 0) {
			pwrap_err("wait_for_fsm_vldclr fail,return_value=%d\n",
				  return_value);
			return E_PWR_WAIT_IDLE_TIMEOUT_READ;
		}
		*rdata = ((reg_rdata >> RDATA_WACS_RDATA_SHIFT)
			  & RDATA_WACS_RDATA_MASK);
		write32(&mt8518_pwrap->wacs2_vldclr, 1);
	}

	return 0;
}

/* external API for pmic_wrap user */

s32 pwrap_wacs2(u32 write, u32 adr, u32 wdata, u32 *rdata)
{
	return pwrap_wacs(write, adr, wdata, rdata, 1);
}

s32 pwrap_read(u32 adr, u32 *rdata)
{
	return pwrap_wacs(0, adr, 0, rdata, 1);
}

s32 pwrap_write(u32 adr, u32 wdata)
{
	return pwrap_wacs(1, adr, wdata, 0, 1);
}

static s32 pwrap_read_nochk(u32 adr, u32 *rdata)
{
	return pwrap_wacs(0, adr, 0, rdata, 0);
}

static s32 pwrap_write_nochk(u32 adr, u32 wdata)
{
	return pwrap_wacs(1, adr, wdata, 0, 0);
}

/* call it in pwrap_init,mustn't check init done */
static s32 pwrap_init_dio( u32 dio_en )
{
  u32 arb_en_backup=0x0;
  u32 rdata=0x0;
  u32 return_value=0;

  arb_en_backup = read32(&mt8518_pwrap->hiprio_arb_en);

  write32(&mt8518_pwrap->hiprio_arb_en , WACS2); // only WACS2

  pwrap_write_nochk(DEW_DIO_EN, dio_en);

  // Check IDLE & INIT_DONE in advance
  return_value =
  wait_for_state_ready(wait_for_idle_and_sync,
  				TIMEOUT_WAIT_IDLE_US,
  				&mt8518_pwrap->wacs2_rdata,
  				0);
  if(return_value!=0)
  {
    pwrap_err("pwrap_init_dio fail,return_value=%d\n", return_value);
    return return_value;
  }

  write32(&mt8518_pwrap->dio_en, dio_en);
  // Read Test
  pwrap_read_nochk(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    pwrap_err("[Dio_mode][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=0x5aa5\n", dio_en, rdata);
    return E_PWR_READ_TEST_FAIL;
  }

  write32(&mt8518_pwrap->hiprio_arb_en , arb_en_backup);

  return 0;
}


static int pwrap_init_cipher(void)
{
	u32 rdata = 0;
	u32 return_value = 0;
	lk_bigtime_t start_time = 0, timeout_time = 0;
	u32 arb_en_backup = 0;

	arb_en_backup = read32(&mt8518_pwrap->hiprio_arb_en);

	write32(&mt8518_pwrap->hiprio_arb_en, WACS2);	/* only WACS2 */

	/* Config cipher mode @AP */
	write32(&mt8518_pwrap->cipher_swrst, 0x1);
	write32(&mt8518_pwrap->cipher_swrst, 0x0);
	write32(&mt8518_pwrap->cipher_key_sel, 0x1);
	write32(&mt8518_pwrap->cipher_iv_sel, 0x2);
	write32(&mt8518_pwrap->cipher_en, 0x1);

	/* Config cipher mode @PMIC */
	pwrap_write_nochk(DEW_CIPHER_SWRST,   0x1);
	pwrap_write_nochk(DEW_CIPHER_SWRST,   0x0);
	pwrap_write_nochk(DEW_CIPHER_KEY_SEL, 0x1);
	pwrap_write_nochk(DEW_CIPHER_IV_SEL,  0x2);
	pwrap_write_nochk(DEW_CIPHER_LOAD,    0x1);
	pwrap_write_nochk(DEW_CIPHER_START,   0x1);

	/* wait for cipher data ready@AP */
	return_value = wait_for_state_ready(wait_for_cipher_ready,
			     TIMEOUT_WAIT_IDLE_US,
			     &mt8518_pwrap->cipher_rdy,
			     0);
	if (return_value != 0) {
		pwrap_err("pwrap_init_cipher fail,return_value=%d\n", return_value);
		return return_value;
	}

	/* wait for cipher data ready@PMIC */
	start_time = current_time_hires();
	timeout_time = start_time + 0xFFFFFF;
	do
	{
		if (current_time_hires() >= timeout_time)
		{
			pwrap_err("timeout when waiting for cipher data ready@PMIC\n");
			return E_PWR_WAIT_IDLE_TIMEOUT;
		}
		pwrap_read_nochk(DEW_CIPHER_RDY,&rdata);
	} while( rdata != 0x1 ); /* cipher_ready */

	pwrap_write_nochk(DEW_CIPHER_MODE, 0x1);

	/* wait for cipher mode idle */
	return_value = wait_for_state_ready(wait_for_idle_and_sync,
			     TIMEOUT_WAIT_IDLE_US,
			     &mt8518_pwrap->wacs2_rdata,
			     0);
	if (return_value != 0) {
		pwrap_err("pwrap_init_cipher fail,return_value=%d\n", return_value);
		return return_value;
	}

	write32(&mt8518_pwrap->cipher_mode, 0x1);

	/* Read Test */
	pwrap_read_nochk(DEW_READ_TEST, &rdata);
	if (rdata != DEFAULT_VALUE_READ_TEST) {
		pwrap_err("pwrap_init_cipher fail, READ_TEST rdata=%x\n", rdata);
		return E_PWR_READ_TEST_FAIL;
	}

	write32(&mt8518_pwrap->hiprio_arb_en, arb_en_backup);

	return 0;
}

/*
 * pwrap_init_sidly - configure serial input delay
 *
 * This configures the serial input delay. We can configure 0, 2, 4 or 6ns
 * delay. Do a read test with all possible values and chose the best delay.
 */
static s32 pwrap_init_sidly(void)
{
	u32 arb_en_backup = 0;
	u32 rdata = 0;
	s32 ind = 0;
	u32 tmp1 = 0;
	u32 tmp2 = 0;
	u32 result_faulty = 0;
	u32 result = 0;
	u32 leading_one, tailing_one;

	arb_en_backup = read32(&mt8518_pwrap->hiprio_arb_en);

	write32(&mt8518_pwrap->hiprio_arb_en, WACS2);	/* only WACS2 */

	/* --------------------------------------------------------------------- */
	/* Scan all possible input strobe by READ_TEST */
	/* --------------------------------------------------------------------- */
	/* 24 sampling clock edge */
	for (ind = 0; ind < 24; ind++)	/* 24 sampling clock edge */
	{
		write32(&mt8518_pwrap->si_ck_con, (ind >> 2) & 0x7);
		write32(&mt8518_pwrap->sidly, 0x3 - (ind & 0x3));
		pwrap_read_nochk(DEW_READ_TEST, &rdata);
		if( rdata == DEFAULT_VALUE_READ_TEST )
		{
			pwrap_log("_pwrap_init_sidly [Read Test] pass,index=%d rdata=%x\n", ind,rdata);
			result |= (0x1 << ind);
		}
		else
			pwrap_log("_pwrap_init_sidly [Read Test] fail,index=%d,rdata=%x\n", ind,rdata);
	}
	/* --------------------------------------------------------------------- */
	/* Locate the leading one and trailing one of PMIC 1/2 */
	/* --------------------------------------------------------------------- */
	for( ind=23 ; ind>=0 ; ind-- )
	{
		if( result & (0x1 << ind) ) break;
	}
	leading_one = ind;
	
	for( ind=0 ; ind<24 ; ind++ )
	{
		if( result & (0x1 << ind) ) break;
	}
	tailing_one = ind;

	/* --------------------------------------------------------------------- */
	/* Check the continuity of pass range */
	/* --------------------------------------------------------------------- */
	tmp1 = (0x1 << (leading_one+1)) - 1;
	tmp2 = (0x1 << tailing_one) - 1;
	if( (tmp1 - tmp2) != result )
	{
		/*TERR = "[DrvPWRAP_InitSiStrobe] Fail, tmp1:%d, tmp2:%d", tmp1, tmp2*/
		pwrap_err("_pwrap_init_sidly Fail,tmp1=%x,tmp2=%x\n", tmp1,tmp2);
		result_faulty = 0x1;
	}

	/* --------------------------------------------------------------------- */
	/* Config SICK and SIDLY to the middle point of pass range */
	/* --------------------------------------------------------------------- */
	ind = (leading_one + tailing_one)/2;
	write32(&mt8518_pwrap->si_ck_con , (ind >> 2) & 0x7);
	write32(&mt8518_pwrap->sidly , 0x3 - (ind & 0x3));

	/* --------------------------------------------------------------------- */
	/* Restore */
	/* --------------------------------------------------------------------- */
	write32(&mt8518_pwrap->hiprio_arb_en, arb_en_backup);

	if( result_faulty == 0 )
		return 0;
	else
	{
		pwrap_err("_pwrap_init_sidly Fail,result=%x\n", result);
		return result_faulty;
	}
}


static s32 pwrap_reset_spislv(void)
{
	u32 ret = 0;
	u32 return_value = 0;

	write32(&mt8518_pwrap->hiprio_arb_en, 0);
	write32(&mt8518_pwrap->wrap_en, 0);
	write32(&mt8518_pwrap->mux_sel, 1);
	write32(&mt8518_pwrap->man_en, 1);
	write32(&mt8518_pwrap->dio_en, 0);

	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_CSL << 8));
	/* to reset counter */
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_OUTS << 8));
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_CSH << 8));
	/*
	 * In order to pull CSN signal to PMIC,
	 * PMIC will count it then reset spi slave
	*/
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_OUTS << 8));
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_OUTS << 8));
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_OUTS << 8));
	write32(&mt8518_pwrap->man_cmd, (OP_WR << 13) | (OP_OUTS << 8));

	return_value = wait_for_state_ready(wait_for_sync,
					    TIMEOUT_WAIT_IDLE_US,
					    &mt8518_pwrap->wacs2_rdata, 0);
	if (return_value != 0) {
		pwrap_err("pwrap_reset_spislv fail,return_value=%d\n", return_value);
		ret = E_PWR_TIMEOUT;
	}

	write32(&mt8518_pwrap->man_en, 0);
	write32(&mt8518_pwrap->mux_sel, 0);

	return ret;
}

static s32 pwrap_init_reg_clock(enum pmic_regck regck_sel)
{
	u32 wdata = 0;
	u32 rdata = 0;

	/* Set reg clk freq */
	pwrap_read_nochk(PMIC_TOP_CKCON2, &rdata); 
	if (regck_sel == 1)
		wdata = (rdata & (~(0x3 << 10))) | (0x1 << 10);
	else
		wdata = rdata & (~(0x3 << 10));

	pwrap_write_nochk(PMIC_TOP_CKCON2, wdata);
	pwrap_read_nochk(PMIC_TOP_CKCON2, &rdata);
	if (rdata != wdata) {
		pwrap_err("pwrap_init_reg_clock,PMIC_TOP_CKCON2 Write [15]=1 Fail, rdata=%x\n",
				rdata);
	return E_PWR_INIT_REG_CLOCK;
	}

	/* Config SPI Waveform according to reg clk */
	switch (regck_sel) {
	case REG_CLOCK_12MHZ:
		write32(&mt8518_pwrap->rddmy, 0xc);
		write32(&mt8518_pwrap->cshext_write, 0x0);
		write32(&mt8518_pwrap->cshext_read, 0x4);
		write32(&mt8518_pwrap->cslext_start, 0x0);
		write32(&mt8518_pwrap->cslext_end, 0x4);
		break;
	case REG_CLOCK_24MHZ:
		write32(&mt8518_pwrap->rddmy, 0x2);
		write32(&mt8518_pwrap->cshext_write, 0x5);
		write32(&mt8518_pwrap->cshext_read, 0x5);
		write32(&mt8518_pwrap->cslext_start, 0xf);
		write32(&mt8518_pwrap->cslext_end, 0xf);
		break;
	default:
		write32(&mt8518_pwrap->rddmy, 0xf);
		write32(&mt8518_pwrap->cshext_write, 0xf);
		write32(&mt8518_pwrap->cshext_read, 0xf);
		write32(&mt8518_pwrap->cslext_start, 0xf);
		write32(&mt8518_pwrap->cslext_end, 0xf);
		break;
	}

	return 0;
}

s32 pwrap_init(void)
{
	s32 sub_return = 0;
	s32 sub_return1 = 0;
	u32 rdata = 0x0;
	u32 cg_mask = 0;
	u32 gpio_val;

	/* Turn off module clock */
	cg_mask = PWRAP_CG_TMR| PWRAP_CG_SPI| PWRAP_CG_SYS;
	write32(MODULE_SW_CG_1_SET, cg_mask);

	/* dummy read to add latency (to wait clock turning off) */
	rdata = read32(&mt8518_pwrap->swrst);

	/* Toggle module reset */
	write32(&mt8518_pwrap->swrst, 1);

	/*reset pwrap_spictrl config swrst register in toprgu*/
	/*rdata = read32(MODULE_SW_WDT_RST);

	write32(MODULE_SW_WDT_RST, (rdata | 0x00004000) | (0x88 << 24));
	write32(MODULE_SW_WDT_RST, (rdata & 0xffffbfff) | (0x88 << 24));*/

	write32(&mt8518_pwrap->swrst, 0);

	/* Turn on module clock */
	write32(MODULE_SW_CG_1_CLR, cg_mask);

	/* Turn on module clock dcm (in global_con) CHECK ME*/
	/*write32(MODULE_SW_CG_3_SET, read32(MODULE_SW_CG_3_SET) | PMIC_B_DCM_EN | PMIC_SPI_DCM_EN);*/

	/*set 0x1000F0AC[30]=1 for CK pin normal work*/
	rdata = read32(&mt8518_pwrap->int_en);
	write32(&mt8518_pwrap->int_en, (rdata | (0x1 << 30)));

	/* Enable DCM */
	write32(&mt8518_pwrap->dcm_en, 3);
	write32(&mt8518_pwrap->dcm_dbc_prd, 0);

	/*Set slave to MT6397/MT6391*/
	write32(&mt8518_pwrap->op_type, 1);
	write32(&mt8518_pwrap->msb_first, 0);

	/* Reset SPISLV */
	sub_return = pwrap_reset_spislv();
	if (sub_return != 0) {
		pwrap_err("error,pwrap_reset_spislv fail,sub_return=%x\n",
			  sub_return);
		return E_PWR_INIT_RESET_SPI;
	}
	/* Enable WACS2 */
	write32(&mt8518_pwrap->wrap_en, 1);
	write32(&mt8518_pwrap->hiprio_arb_en, WACS2);
	write32(&mt8518_pwrap->wacs2_en, 1);

	/*According to MT6391*/
	write32(&mt8518_pwrap->rddmy, 0x8);

	/* SIDLY setting */
	sub_return = pwrap_init_sidly();
	if (sub_return != 0) {
		pwrap_err("error,pwrap_init_sidly fail,sub_return=%x\n",
			  sub_return);
		return E_PWR_INIT_SIDLY;
	}
	/*
	 * SPI Waveform Configuration
	 * safe mode/12MHz/24MHz/
	 */
	sub_return = pwrap_init_reg_clock(REG_CLOCK_24MHZ);
	if (sub_return != 0) {
		pwrap_err("error,pwrap_init_reg_clock fail,sub_return=%x\n",
			  sub_return);
		return E_PWR_INIT_REG_CLOCK;
	}

	/*PMIC MT6391 settings*/
	sub_return = pwrap_write_nochk(PMIC_WRP_CKPDN, 0);
	sub_return = pwrap_write_nochk(PMIC_WRP_RST_CON, 0);

	/* Enable DIO mode */
	sub_return = pwrap_init_dio(1);
	if (sub_return != 0) {
		pwrap_err("pwrap_init_dio fail, sub_return=%x\n", sub_return);
		return E_PWR_INIT_DIO;
	}

	/* Enable Encryption */
	sub_return = pwrap_init_cipher();
	if (sub_return != 0) {
		pwrap_err("pwrap_init_cipher fail, sub_return=%x\n", sub_return);
		return E_PWR_INIT_CIPHER;
	}

	/*
	 * Write test using WACS2,
	 * make sure the read/write function ready
	 */
	sub_return = pwrap_write_nochk(DEW_WRITE_TEST, WRITE_TEST_VALUE);
	sub_return1 = pwrap_read_nochk(DEW_WRITE_TEST, &rdata);
	if ((rdata != WRITE_TEST_VALUE) || (sub_return != 0)
	    || (sub_return1 != 0)) {
		pwrap_err("write error, rdata=%x, return=%x, return1=%x\n",
			  rdata, sub_return, sub_return1);
		return E_PWR_INIT_WRITE_TEST;
	}

	/* Signature Checking - Using CRC
	 * should be the last to modify WRITE_TEST
	 */
	sub_return = pwrap_write_nochk(DEW_CRC_EN, 0x1);
	if (sub_return != 0) {
		pwrap_err("enable CRC fail,sub_return=%x\n", sub_return);
		return E_PWR_INIT_ENABLE_CRC;
	}
	write32(&mt8518_pwrap->crc_en, 0x1);
	write32(&mt8518_pwrap->sig_mode, 0x0);
	write32(&mt8518_pwrap->sig_adr, read32(&mt8518_pwrap->sig_adr) | DEW_CRC_VAL);

	/* PMIC_WRAP enables */
	write32(&mt8518_pwrap->hiprio_arb_en, 0x7f);
	write32(&mt8518_pwrap->wacs0_en, 0x1);
	write32(&mt8518_pwrap->wacs1_en, 0x1);
	write32(&mt8518_pwrap->staupd_prd, 0x5);

	/*initial settings for wachdod and interrupts*/
	/*write32(&mt8518_pwrap->wdt_unit, 0xf);
	write32(&mt8518_pwrap->wdt_src_en, 0xffffffff);
	write32(&mt8518_pwrap->timer_en, 0x1);
	write32(&mt8518_pwrap->int_en, 0x7ffffff9);*/

	/* Initialization Done */
	write32(&mt8518_pwrap->init_done2, 0x1);
	write32(&mt8518_pwrap->init_done0, 0x1);
	write32(&mt8518_pwrap->init_done1, 0x1);

	/*set gpio27 PMIC_INT to pull down cause the default state is pull up and wrong in E1 IC*/
	gpio_val = (read32(PWRAP_INT_GPIO_BASE) & (~(0x1 << 11))) | (0x0 << 11);
	write32(PWRAP_INT_GPIO_BASE, gpio_val);

	return 0;
}

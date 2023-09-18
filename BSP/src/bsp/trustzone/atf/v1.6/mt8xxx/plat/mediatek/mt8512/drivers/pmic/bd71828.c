// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2020. All rights reserved.
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
 */

#include <bd71828.h>
#include <mt_i2c_atf.h>
#include <mtk_plat_common.h>

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define BD71828_I2C_ID       1
#define I2C_SPEED_400_KHZ    400 /* kHz */

static uint8_t bd71828_slave_addr = 0x4b;
/**********************************************************
  *   Init Setting
  *********************************************************/
struct pmic_setting {
	unsigned short addr;
	unsigned short val;
	unsigned short mask;
	unsigned char shift;
};

static struct pmic_setting suspend_setting[] = {
	/* [7:0]: BUCK2_SUSP_VID, Vcore 550 mv */
	{0x16, 0x8, 0xFF, 0},
	/* [7:0]: BUCK7_SUSP_VID, Vsram_core 600 mv */
	{0x33, 0x10, 0xFF, 0},
};

static struct pmic_setting idle_setting[] = {
	/* [7:0]: BUCK2_SUSP_VID, Vcore 600 mv */
	{0x16, 0x10, 0xFF, 0},
	/* [7:0]: BUCK7_SUSP_VID, Vsram_core 800 mv */
	{0x33, 0x30, 0xFF, 0},
};

/**********************************************************
  *
  *   [I2C Function For Read/Write bd71828]
  *
  *********************************************************/
static int bd71828_write_byte(uint8_t addr, uint8_t value)
{
	int ret_code = -1;
	uint8_t write_data[2];
	uint16_t len;

	write_data[0]= addr;
	write_data[1] = value;
	len = 2;

	ret_code = mtk_i2c_write(BD71828_I2C_ID, bd71828_slave_addr,
		I2C_SPEED_400_KHZ, write_data, len);

	return ret_code;
}

static int bd71828_read_byte(uint8_t addr, uint8_t *dataBuffer)
{
	int ret_code = -1;
	uint16_t len;
	*dataBuffer = addr;

	len = 1;

	ret_code = mtk_i2c_write_read(BD71828_I2C_ID, bd71828_slave_addr,
		I2C_SPEED_400_KHZ, dataBuffer, dataBuffer, len, len);
	return ret_code;
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
static int bd71828_config_interface(uint8_t RegNum,
	uint8_t val, uint8_t MASK, uint8_t SHIFT)
{
	uint8_t bd71828_reg = 0;
	int ret = -1;

	ret = bd71828_read_byte(RegNum, &bd71828_reg);

	bd71828_reg &= ~(MASK << SHIFT);
	bd71828_reg |= (val << SHIFT);

	ret = bd71828_write_byte(RegNum, bd71828_reg);

	return ret;
}

void bd71828_set_suspend_setting(void)
{
	for (int i = 0; i < ARRAY_SIZE(suspend_setting); i++)
		bd71828_config_interface(
			suspend_setting[i].addr, suspend_setting[i].val,
			suspend_setting[i].mask, suspend_setting[i].shift);
}

void bd71828_set_idle_setting(void)
{
	for (int i = 0; i < ARRAY_SIZE(idle_setting); i++)
		bd71828_config_interface(
			idle_setting[i].addr, idle_setting[i].val,
			idle_setting[i].mask, idle_setting[i].shift);
}

void bd71828_power_off(void)
{
	/* set reserved2 to HIBERNATION */
	bd71828_config_interface(0xf0, 0x4, 0xff, 0x0);

	/* PS_CTRL_1, HBNT_MODE [1:1] = 2'b1 */
	bd71828_config_interface(0x2a, 0x1, 0x1, 0x6);
	bd71828_config_interface(0xd, 0x3, 0x3, 0x4);
	bd71828_config_interface(0x4, 0x1, 0x1, 0x1);
}

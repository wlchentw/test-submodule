/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2015. All rights reserved.
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

#include <platform/mt_i2c.h>
#include <platform/pmic_6398.h>

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define mt6398_SLAVE_ADDR   0x62

/**********************************************************
  *   Global Variable
  *********************************************************/
#define mt6398_I2C_ID       1

/**********************************************************
  *
  *   [I2C Function For Read/Write mt6398]
  *
  *********************************************************/
u32 mt6398_write_byte(u8 addr, u8 value)
{
	int ret_code = 0;
	u8 write_data[2];
	u16 len;

	write_data[0]= addr;
	write_data[1] = value;
	len = 2;

	ret_code = mtk_i2c_write(mt6398_I2C_ID, mt6398_SLAVE_ADDR, 400, write_data, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}

u32 mt6398_read_byte (u8 addr, u8 *dataBuffer)
{
	int ret_code = 0;
	u16 len;
	*dataBuffer = addr;

	len = 1;

	ret_code = mtk_i2c_write_read(mt6398_I2C_ID, mt6398_SLAVE_ADDR, 400,
								dataBuffer, dataBuffer, len, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
u32 pmic_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT)
{
	u8 mt6398_reg = 0;
	u32 ret = 0;

	ret = mt6398_read_byte(RegNum, &mt6398_reg);

	mt6398_reg &= (MASK << SHIFT);
	*val = (mt6398_reg >> SHIFT);

	return ret;
}

u32 pmic_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT)
{
	u8 mt6398_reg = 0;
	u32 ret = 0;

	ret = mt6398_read_byte(RegNum, &mt6398_reg);

	mt6398_reg &= ~(MASK << SHIFT);
	mt6398_reg |= (val << SHIFT);

	ret = mt6398_write_byte(RegNum, mt6398_reg);

	return ret;
}

int pmic_detect_powerkey(void)
{
	u8 val=0;

	pmic_read_interface(0x1E, &val, 0x01, 7);

	if (val==1) {
		printf("pl pmic powerkey Release\n");
		return 0;
	} else {
		printf("pl pmic powerkey Press\n");
		return 1;
	}
}

void PMIC_INIT_SETTING_V1(void)
{
	unsigned int ret = 0;

	//ret = pmic_config_interface(0x40, 0x98, 0xFF, 0); //enter test mode
	//ret = pmic_config_interface(0x4C, 0x1, 0xFF, 0); //disable reset i2c slave function

	ret = pmic_config_interface(0x0B, 0x1, 0x01, 3); //WDOG_RST_EN  enable watchdog reset
	ret = pmic_config_interface(0x45, 0x0, 0x03, 6); //CH1_SR set slew rate to 20mV/us

	/*low power setting*/
	ret = pmic_config_interface(0x0C, 0x1, 0x01, 7); //SLEEP_SEL enter sleep mode control by pin SUSPEND
	ret = pmic_config_interface(0x0C, 0x0, 0x01, 1);  //set vproc poweroff when enter suspend mode

	if (ret)
		printf("[pmic6398_init] PMIC MT6398 init setting fail\n");
	else
		printf("[pmic6398_init] PMIC MT6398 init setting success\n");
}

//==============================================================================
// PMIC6398 Init Code
//==============================================================================
void pmic_init_mt6398 (void)
{
	printf("[pmic6398_init] Preloader INIT Start..................\n");

	/* pmic initial setting */
	PMIC_INIT_SETTING_V1();

	printf("[pmic6398_init] Preloader INIT Done...................\n");
}


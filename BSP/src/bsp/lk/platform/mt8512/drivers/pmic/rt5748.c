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
#include <platform/rt5748.h>

#define VOLT_TO_BUCK_VAL(volt)  (((volt) - 300000 + 5000 - 1) / 5000)
#define BUCK_VAL_TO_VOLT(val)  (((val) * 5000) + 300000)

/**********************************************************
  *
  *   [I2C Function For Read/Write rt5749]
  *
  *********************************************************/
u32 rt5749_write_byte(u8 addr, u8 value, u8 slave_addr, u8 i2c_bus)
{
    int ret_code = 0;
    u8 write_data[2];
    u8 len;

    write_data[0]= addr;
    write_data[1] = value;

    len = 2;

    ret_code = mtk_i2c_write(i2c_bus, slave_addr, 400, write_data, len);

    if(ret_code == 0)
        return 0; // ok
    else
        return -1; // fail
}

u32 rt5749_read_byte (u8 addr, u8 *dataBuffer, u8 slave_addr, u8 i2c_bus)
{
    int ret_code = 0;
    u8 len;
    *dataBuffer = addr;

    len = 1;

    ret_code = mtk_i2c_write_read(i2c_bus, slave_addr, 400,
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
u32 rt5749_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT, u8 slave_addr, u8 i2c_bus)
{
    u8 rt5749_reg = 0;
    u32 ret = 0;

    ret = rt5749_read_byte(RegNum, &rt5749_reg, slave_addr, i2c_bus);

    rt5749_reg &= (MASK << SHIFT);
    *val = (rt5749_reg >> SHIFT);

    return ret;
}

u32 rt5749_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT, u8 slave_addr, u8 i2c_bus)
{
    u8 rt5749_reg = 0;
    u32 ret = 0;

    ret = rt5749_read_byte(RegNum, &rt5749_reg, slave_addr, i2c_bus);

    rt5749_reg &= ~(MASK << SHIFT);
    rt5749_reg |= (val << SHIFT);

    ret = rt5749_write_byte(RegNum, rt5749_reg, slave_addr, i2c_bus);

    return ret;
}

int rt5749_regulator_get_voltage(BUCK_USER_TYPE type)
{
	int ret = 0;
	u8 val;
	int volt;

	switch (type)
	{
	case VCORE:
		ret = rt5749_read_interface(RT5749_REG_VSEL1, &val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VCORE_SLAVE_ADDR, rt5749_VCORE_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_get_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		volt = BUCK_VAL_TO_VOLT(val);
		break;
	case VCORE_SRAM:
		ret = rt5749_read_interface(RT5749_REG_VSEL1, &val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VCORE_SRAM_SLAVE_ADDR, rt5749_VCORE_SRAM_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_get_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		volt = BUCK_VAL_TO_VOLT(val);
		break;
	case VPROC_SRAM:
		ret = rt5749_read_interface(RT5749_REG_VSEL1, &val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VPROC_SRAM_SLAVE_ADDR, rt5749_VPROC_SRAM_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_get_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		volt = BUCK_VAL_TO_VOLT(val);
		break;
	default:
		printf("BUCK_USER_TYPE don't support! Please use or VCORE or VCORE_SRAM or VPROC_SRAM\n");
		return -1;
	}

	return volt;
}

int rt5749_regulator_set_voltage(BUCK_USER_TYPE type, unsigned int volt)
{
	int ret = 0;
	u8 val;

	val = VOLT_TO_BUCK_VAL(volt);

	switch (type)
	{
	case VCORE:
		ret = rt5749_config_interface(RT5749_REG_VSEL1, val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VCORE_SLAVE_ADDR, rt5749_VCORE_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_set_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		break;
	case VCORE_SRAM:
		ret = rt5749_config_interface(RT5749_REG_VSEL1, val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VCORE_SRAM_SLAVE_ADDR, rt5749_VCORE_SRAM_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_set_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		break;
	case VPROC_SRAM:
		ret = rt5749_config_interface(RT5749_REG_VSEL1, val, rt5749_vselh_vol_mask, rt5749_vselh_vol_shift,
						rt5749_VPROC_SRAM_SLAVE_ADDR, rt5749_VPROC_SRAM_I2C_ID);
		if (ret < 0)
		{
			printf("[regulator_set_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
			return ret;
		}
		break;
	default:
		printf("BUCK_USER_TYPE don't support! Please use or VCORE or VCORE_SRAM or VPROC_SRAM\n");
		return -1;
	}

	return ret;
}


void RT5749_INIT_SETTING_V1(void)
{
	rt5749_config_interface(RT5749_REG_CTRL1, 0x2, rt5749_rampup_rate_mask, rt5749_rampup_rate_shift,
		rt5749_VCORE_SLAVE_ADDR, rt5749_VCORE_I2C_ID); //set vcore slew rate to 6mV/us
	rt5749_config_interface(RT5749_REG_VSEL0, 0x3C, rt5749_vsell_vol_mask, rt5749_vsell_vol_shift,
		rt5749_VCORE_SLAVE_ADDR, rt5749_VCORE_I2C_ID);  //set deepidle vcore=0.6V
}

//==============================================================================
// BUCK RT5749 Init Code
//==============================================================================
void rt5749_init (void)
{
    printf("[buck5749_init] Preloader Start..................\n");

    /* buck initial setting */
    RT5749_INIT_SETTING_V1();

    printf("[buck5749_init] Done...................\n");
}


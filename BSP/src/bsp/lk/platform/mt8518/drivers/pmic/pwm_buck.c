/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2018. All rights reserved.
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
#include <platform/pwm-buck.h>
#include <platform/pwm_buck_property.h>
#include <platform/pwm.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <reg.h>

struct pwm_buck pwm_buck_prop[2];

/*actually when voltage is adjusted, pwm is enabled*/
static int pwm_is_enabled(void)
{
	return 1;
}

static int _regulator_get_voltage(unsigned int n, struct pwm_volt_map *table, unsigned int pwm_num)
{
	unsigned int pwm_period;
	unsigned int duty_cycle_period;
	unsigned int duty_cycle;
	unsigned int i;

	pwm_period = pwm_get_period(pwm_num);   //pwm_period is the clk number in one period

	duty_cycle_period = pwm_get_duty_cycle(pwm_num);   //duty_cycle_period is the clk number in one period for the high level

	duty_cycle = ((duty_cycle_period * 100) +  pwm_period - 1) / pwm_period;

	for (i = 0; i < n; i++) {
		if (table[i].duty_cycle == duty_cycle)
			return table[i].volt;
	}

	BUCKERR("Do not recognize the duty_cycle and voltage mapping relation!\n");

	return -BUCK_EINVAL;
}

static int _regulator_set_voltage(unsigned int n, struct pwm_volt_map *table, unsigned int volt, unsigned int pwm_num)
{
	unsigned int pwm_period;
	unsigned int duty_cycle_period;
	unsigned int duty_cycle;
	unsigned int i;

	pwm_period = pwm_get_period(pwm_num);

	if (table[0].volt >= volt) {
		duty_cycle = table[0].duty_cycle;
		goto set_volt;
	}

	if (table[n-1].volt <= volt) {
		duty_cycle = table[n-1].duty_cycle;
		goto set_volt;
	}

	for (i = 0; i < n-1; i++) {
		if ((table[i].volt < volt) && (table[i+1].volt >= volt)) {
			duty_cycle = table[i+1].duty_cycle;
			goto set_volt;
		}
	}

	return -BUCK_EINVAL;

set_volt:
	duty_cycle_period = pwm_period * duty_cycle / 100;

	pwm_set_duty(pwm_num, duty_cycle_period);
	pwm_enable(pwm_num);

	return BUCK_OK;
}

static int _regulator_is_enabled(void)
{
	return pwm_is_enabled();
}

static int _regulator_enable(unsigned int enable, unsigned int pwm_num)
{
	int ret = 0;

	pwm_enable(pwm_num);

	return ret;
}

int regulator_get_voltage()
{
	int ret = 0;

	ret = _regulator_get_voltage(pwm_buck_prop[0].n_table, pwm_buck_prop[0].table, PWM_NUM_VCORE);
	if (ret < 0)
	{
		BUCKERR("[regulator_get_voltage] _regulator_get_voltage fail, ret = %d!\n", ret);
		return ret;
	}

	return ret;
}

int regulator_set_voltage(unsigned int volt)
{
	int ret = 0;

	ret = _regulator_set_voltage(pwm_buck_prop[0].n_table, pwm_buck_prop[0].table, volt, PWM_NUM_VCORE);
	if (ret < 0)
	{
		BUCKERR("[regulator_set_voltage] _regulator_set_voltage fail, ret = %d!\n", ret);
		return ret;
	}

	return ret;
}


int regulator_is_enabled()
{
	int ret = 0;

	ret = _regulator_is_enabled();
	if (ret < 0)
	{
		BUCKERR("[regulator_is_enabled] _regulator_is_enabled fail, ret = %d!\n", ret);
		return ret;
	}

	return ret;
}


int regulator_enable(int enable)
{
	int is_enabled = 0;
	int ret = 0;

	is_enabled = regulator_is_enabled();

	if (is_enabled < 0)
	{
		BUCKERR("[regulator_enable] regulator_is_enabled fail, ret = %d!\n", is_enabled);
		return is_enabled;
	}

	if (is_enabled == enable)
	{
		BUCKERR("[regulator_enable] regulator is already %d!\n", enable);
		return BUCK_OK;
	}

	ret = _regulator_enable(enable, PWM_NUM_VCORE);
	if (ret < 0)
	{
		BUCKERR("[regulator_enable] _regulator_enable fail, ret = %d!\n", ret);
		return ret;
	}

	return ret;
}


static int PWM_BUCK_VCORE_INIT_SETTING(void)
{
	uint32_t gpio_reg;

	pwm_buck_prop[0].table = vcore_map;
	pwm_buck_prop[0].n_table = sizeof(vcore_map) / sizeof(struct pwm_volt_map);

	/* set pwm period*/
	pwm_buck_prop[0].pwm_period = PWM_PERIOD_INIT;
	if (!pwm_buck_prop[0].pwm_period) {
		BUCKERR("There is no vcore pwm buck period!\n");
		goto err;
	}
	pwm_config_freq(PWM_NUM_VCORE, pwm_buck_prop[0].pwm_period);

	gpio_reg = (readl(GPIO_VCORE_BASE) &
			   (~(0x7 << GPIO_VCORE_PWM_OFFSET))) |
			   (GPIO_VCORE_PWM_MODE << GPIO_VCORE_PWM_OFFSET);
	writel(gpio_reg, GPIO_VCORE_BASE);

	pwm_set_duty(PWM_NUM_VCORE, 0);  //for 1.1V
	pwm_enable(PWM_NUM_VCORE);

	return BUCK_OK;
err:
	return -BUCK_EINVAL;
}


//==============================================================================
// BUCK RT5748 Init Code
//==============================================================================
int pwm_buck_init (void)
{
	int ret;

	printf("[pwm_buck_init] Init Start..................\n");

	/*Do the initial setting for struct pwm_buck*/
	//memset(pwm_buck_prop, 0, sizeof(struct pwm_buck)*2);

	ret = PWM_BUCK_VCORE_INIT_SETTING();
	if (ret)
		goto err1;

	return BUCK_OK;

err1:
	return -BUCK_EINVAL;
}


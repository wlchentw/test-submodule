/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "mt8512_sloa_fs.h"
#include <linux/string.h>

#define BUCK_NUM 2
#define DIS_VCORE_DVS_BIT 6
#define DIS_VPORC_SRAM_DVS_BIT 9
#define I2C_TYPE_SHIFT 20
#define CHANNEL_SHIFT 21

/*
 * type: vcore/vproc buck ctrl type(pwm/i2c/gpio)
 * channel: buck pwm/i2c channel
 * latency: wakeup latency
 * voltage_fix: fix voltage or not
 */
struct buckProperty {
	unsigned int type;
	unsigned int channel;
	unsigned int latency;
	unsigned int voltage_fix;
};

struct pwrslt {
	unsigned int merge_ctrl;
	unsigned int fix;
	struct buckProperty buck[BUCK_NUM];
};

enum {
	VCORE_CORE_SRAM = 0,
	VPROC_PROC_SRAM,
	POWER_STATE_MAX,
};

enum BUCK_TYPE {
	I2C = 1,
	PWM,
	GPIO,
};

struct pwrslt pwr_slt[POWER_STATE_MAX];

const u8 *buckcompatible[POWER_STATE_MAX][BUCK_NUM] = { {
	"vcore-solution", "vcore-sram-solution",},
	{
	"vproc-solution", "vproc-sram-solution",}
};

static unsigned int flag, flag2;
static int init_power_node(char *matches, int state_idx)
{
	int err, i;
	const struct device_node *state_node, *buck_node;
	const char *desc;

	state_node = of_find_compatible_node(NULL, NULL, matches);

	err =  of_property_read_u32(state_node, "fix",
				   &pwr_slt[state_idx].fix);
	if (err)
		pr_debug("missing fix property\n");

	else if (pwr_slt[state_idx].fix) {
		if (state_idx == VCORE_CORE_SRAM) {
			flag2 |= 0xc0;
			return 0;
		}
		if (state_idx == VPROC_PROC_SRAM) {
			flag |= 0x1 << DIS_VPORC_SRAM_DVS_BIT;
			sloa_flag_req(PCM_FLAG_SET, DPIDLE_OPT, flag);
			return 0;
		}
	}

	err = of_property_read_u32(state_node, "merge",
				   &pwr_slt[state_idx].merge_ctrl);

	if (err)
		pr_debug("missing merge property\n");

	for (i = 0; i < (pwr_slt[state_idx].merge_ctrl ^ 0x1) + 1; i++) {

		buck_node = of_find_compatible_node(NULL, NULL,
					buckcompatible[state_idx][i]);

		err = of_property_read_u32(buck_node, "voltage-fix",
				&pwr_slt[state_idx].buck[i].voltage_fix);

		if (err)
			pr_debug("missing voltage-fix property\n");
		else if (pwr_slt[state_idx].buck[i].voltage_fix) {
			flag2 |= (i + 1) << DIS_VCORE_DVS_BIT;
			break;
		}

		err = of_property_read_string(buck_node, "type", &desc);

		if (!err) {
			if (!strcmp("i2c", desc)) {
				pwr_slt[state_idx].buck[i].type = I2C;
				flag2 |= (0x1 << I2C_TYPE_SHIFT);
			}
			else if (!strcmp("pwm", desc)) {
				pwr_slt[state_idx].buck[i].type = PWM;
				flag2 |= 0x200;
			} else if (!strcmp("gpio", desc)) {
				pwr_slt[state_idx].buck[i].type = GPIO;
				if (i == 1)
					flag2 |= 0x100;
				else if (i == 0)
					flag2 |= 0x2;
			}
		} else
			pr_debug("missing type property\n");

		err = of_property_read_u32(buck_node, "channel",
					   &pwr_slt[state_idx].buck[i].channel);

		if (pwr_slt[state_idx].buck[i].type == I2C)
			flag2 |= pwr_slt[state_idx].buck[i].channel
						<< CHANNEL_SHIFT;

		if (err)
			pr_debug("missing channel property\n");


		err = of_property_read_u32(buck_node, "latency",
					   &pwr_slt[state_idx].buck[i].latency);

		if (err)
			pr_debug("missing latency property\n");

	}

	if (pwr_slt[state_idx].merge_ctrl && state_idx == 0)
		flag2 |= 0x1 << (DIS_VCORE_DVS_BIT + 1);

	if (flag2) {
		sloa_flag_req(PCM_FLAG2_SET, SUSPEND_OPT, flag2);
		sloa_flag_req(PCM_FLAG2_SET, DPIDLE_OPT, flag2);
	}

	return 0;
}

#define POWER_SOLUTION  "mt8512,power-solution"

int dt_probe_power_solution(void)
{
	struct device_node *state_node, *power_node;
	int i, err = 0;
	unsigned int device_type = 0, clk_on = 0, fix = 0;
	char *power_state_compatible_node[2] = { "mt8512,vcore-core-sram",
						"mt8512,vproc-proc-sram" };
	const char *desc;

	power_node = of_find_compatible_node(NULL, NULL, POWER_SOLUTION);

	if (!power_node) {
		pr_debug("info:cannot find topckgen node mt8512,power-solution");
		goto fail;
	}

	err = of_property_read_u32(power_node, "fix",
				   &fix);
	if (!err) {
		if (fix) {
			sloa_flag_req(PCM_FLAG_SET, SUSPEND_OPT, 0x1 << 9);
			sloa_flag_req(PCM_FLAG1_SET, SUSPEND_OPT, 0x1 << 21);
			sloa_flag_req(PCM_FLAG2_SET, SUSPEND_OPT, 0xc0);
			sloa_flag_req(PCM_FLAG_SET, DPIDLE_OPT, 0x1 << 9);
			sloa_flag_req(PCM_FLAG2_SET, DPIDLE_OPT, 0xc0 | 0x1800);
		}
	}

	err = of_property_read_u32(power_node, "device-names",
				   &device_type);
	if (!err) {
		if (device_type) {
			sloa_suspend_src_req(APSRC_REQ, 1);
			sloa_dpidle_src_req(APSRC_REQ, 1);
		}
	}

	sloa_dpidle_src_req(F26M_REQ, 1);

	err = of_property_read_u32(power_node, "clk-on",
				   &clk_on);

	if (!err) {
		if (clk_on)
			sloa_26m_on(true);
	}

	err = of_property_read_string(power_node, "buck-name", &desc);

	if (!err) {
		if (!strcmp("bd", desc))
			sloa_flag_req(PCM_FLAG1_SET, SUSPEND_OPT, 0x1 << 31);
		else if (!strcmp("rt", desc))
			sloa_flag_req(PCM_FLAG1_SET, SUSPEND_OPT, 0x1 << 30);
	}

	for (i = 0; ; i++) {
		state_node = of_parse_phandle(power_node, "power-states", i);
		if (!state_node)
			break;

		if (i == POWER_STATE_MAX)
			break;

		err = init_power_node(power_state_compatible_node[i], i);
		if (err) {
			pr_debug("Parsing power node failed with err\n");
			break;
		}
		of_node_put(state_node);
	}

	of_node_put(state_node);
	of_node_put(power_node);
	if (err)
		return err;

	return i;
fail:
	return -ENODEV;
}
late_initcall(dt_probe_power_solution);


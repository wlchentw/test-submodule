/*
 * Copyright (C) 2020 MediaTek Inc.
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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/mfd/m2296.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct m2296_data {
	int num_regulators;
	struct m2296_chip *chip;
};

/*VOLDO2 & VOLDO3
 * Setting the output voltage of LDO2 in Normal/Sleep mode, Default=4’b1010
 * 1111 3.30V 1011 2.00V 0111 1.10V 0011 0.90V
 * 1110 3.00V 1010 1.80V 0110 1.05V 0010 0.85V
 * 1101 2.80V 1001 1.50V 0101 1.00V 0001 0.80V
 * 1100 2.50V 1000 1.20V 0100 0.95V 0000 0.75V
 */
static const int ldo2_volt_table1[] = {
	750000, 800000, 850000, 900000,
	950000, 1000000, 1050000, 1100000,
	1200000, 1500000, 1800000, 2000000,
	2500000, 2800000, 3000000, 3300000,
};

/*VOLDO4 & VOLDO6 & VOLDO7 & VOLDO8
 * Setting the output voltage of LDO4, Default=4’b1100
 * 1111 3.00V 1011 2.80V 0111 2.50V 0011 1.80V
 * 1110 2.95V 1010 2.75V 0110 2.05V 0010 1.60V
 * 1101 2.90V 1001 2.70V 0101 2.00V 0001 1.50V
 * 1100 2.85V 1000 2.55V 0100 1.85V 0000 1.40V
 */
static const int ldo4_volt_table1[] = {
	1400000, 1500000, 1600000, 1800000,
	1850000, 2000000, 2050000, 2500000,
	2550000, 2700000, 2750000, 2800000,
	2850000, 2900000, 2950000, 3000000,
};

/* VOLDO5 [1:0]
 * Setting the output voltage of LDO5, Default=2’b00
 * VOLDO5[1:0]] LDO5
 * 11 1.80V
 * 10 3.00V
 * 01 3.15V
 * 00 3.30V
 */
static const int ldo5_volt_table1[] = {
	3300000, 3150000, 3000000, 1800000,
};

/* VOLDO9 & VOLDO10
 * 1111 1.100V 1011 1.000V 0111 0.900V 0011 0.800V
 * 1110 1.075V 1010 0.975V 0110 0.875V 0010 0.775V
 * 1101 1.050V 1001 0.950V 0101 0.850V 0001 0.750V
 * 1100 1.025V 1000 0.925V 0100 0.825V 0000 0.725V
 */
static const int ldo9_volt_table1[] = {
	725000, 750000, 775000, 800000,
	825000, 850000, 875000, 900000,
	925000, 950000, 975000, 1000000,
	1025000, 1050000, 1075000, 1100000,
};

static int m2296_regulator_get_status(struct regulator_dev *rdev)
{
	struct m2296_regulator_drvdata *drvdata = rdev_get_drvdata(rdev);
	struct regulator_desc *rdesc = &drvdata->rdesc;
	int rid = rdesc->id;

	dev_info(&rdev->dev, "[%s], rid %d\n", __func__, rid);

	return rid;
}

/*
 * Regulator operations
 */
static const struct regulator_ops m2296_linear_regulator_ops = {
	.list_voltage = regulator_list_voltage_linear,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	/*.set_mode = m2296_regulator_set_mode,*/
	/*.get_mode = m2296_regulator_get_mode,*/
	.get_status = m2296_regulator_get_status,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
};

/**volt table **/
static const struct regulator_ops m2996_volt_table_ops = {
	.list_voltage = regulator_list_voltage_table,
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.map_voltage = regulator_map_voltage_iterate,
	.set_voltage_sel = regulator_set_voltage_sel_regmap,
	.get_voltage_sel = regulator_get_voltage_sel_regmap,
	.set_voltage_time_sel = regulator_set_voltage_time_sel,
	.get_status = m2296_regulator_get_status,
};

/* Fixed regulator */
static const struct regulator_ops mt2296_fixed_regulator_ops = {
	.enable = regulator_enable_regmap,
	.disable = regulator_disable_regmap,
	.is_enabled = regulator_is_enabled_regmap,
	.get_status = m2296_regulator_get_status,
};

/*
 * Regulator descriptors
 */
#define M2296_REGULATOR_FIXED(_name, _match) \
{ \
	.rdesc = { \
		.name = #_name, \
		.of_match = #_match, \
		.ops = &mt2296_fixed_regulator_ops, \
		.type = REGULATOR_VOLTAGE, \
		.id = _name##_id, \
		.owner = THIS_MODULE, \
		.n_voltages = 1, \
		.fixed_uV = _name##_fixed_uV, \
		.enable_reg = _name##_enable_reg, \
		.enable_mask = _name##_enable_mask, \
		.enable_val = _name##_enable_mask, \
		.disable_val = 0, \
	}, \
}

#define M2296_REGULATOR_LINEAR(_name, _match) \
{ \
	.rdesc = { \
		.name = #_name, \
		.of_match = #_match, \
		.ops = &m2296_linear_regulator_ops, \
		.type = REGULATOR_VOLTAGE, \
		.id = _name##_id, \
		.owner = THIS_MODULE, \
		.n_voltages = _name##_n_voltages, \
		.linear_min_sel = _name##_linear_min_sel, \
		.min_uV = _name##_min_uV, \
		.uV_step = _name##_uV_step, \
		.vsel_reg = _name##_vsel_reg, \
		.vsel_mask = _name##_vsel_mask, \
		.enable_reg = _name##_enable_reg, \
		.enable_mask = _name##_enable_mask, \
		.enable_val = _name##_enable_mask, \
		.disable_val = 0, \
	}, \
}

#define M2296_REGULATOR_TABLE(_name, _match, _match_table) \
{ \
	.rdesc = { \
		.name = #_name, \
		.of_match = #_match, \
		.ops = &m2996_volt_table_ops, \
		.type = REGULATOR_VOLTAGE, \
		.id = _name##_id, \
		.owner = THIS_MODULE, \
		.n_voltages = ARRAY_SIZE(_match_table), \
		.volt_table = _match_table, \
		.vsel_reg = _name##_vsel_reg, \
		.vsel_mask = _name##_vsel_mask, \
		.enable_reg = _name##_enable_reg, \
		.enable_mask = _name##_enable_mask, \
		.enable_val = _name##_enable_mask, \
		.disable_val = 0, \
	}, \
}

static struct m2296_regulator_drvdata
			m2296_regulator_drvdata_tbl[M2296_NUM_REGULATORS] = {
	M2296_REGULATOR_LINEAR(m2296_ldo1, m2296_ldo1),
	M2296_REGULATOR_TABLE(m2296_ldo2, m2296_ldo2, ldo2_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo3, m2296_ldo3, ldo2_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo4, m2296_ldo4, ldo4_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo5, m2296_ldo5, ldo5_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo6, m2296_ldo6, ldo4_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo7, m2296_ldo7, ldo4_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo8, m2296_ldo8, ldo4_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo9, m2296_ldo9, ldo9_volt_table1),
	M2296_REGULATOR_TABLE(m2296_ldo10, m2296_ldo10, ldo9_volt_table1),
	/*M2296_REGULATOR_FOUR_FIXED(m2296_rtcout,m2296_rtcout);*/
	M2296_REGULATOR_LINEAR(m2296_dcdc1, m2296_dcdc1),
	M2296_REGULATOR_LINEAR(m2296_dcdc2, m2296_dcdc2),
	M2296_REGULATOR_FIXED(m2296_dcdc3, m2296_dcdc3),
	M2296_REGULATOR_FIXED(m2296_dcdc4, m2296_dcdc4),
};

static int m2296_register_regulator(struct m2296_data *m2296)
{
	int ret = 0, i = 0;
	struct device_node *np = m2296->chip->dev->of_node;
	struct device_node *chd_np;
	struct device_node *regulators_np;
	struct regulator_desc *rdesc;
	struct regulator_config rcfg = {};
	struct regulator_init_data *init_data;
	struct regulator_dev *rdev;
	struct m2296_regulator_drvdata *drvdata;
	struct regulation_constraints *constraints;

	regulators_np = of_find_node_by_name(np, "regulators");
	if (!regulators_np) {
		dev_info(m2296->chip->dev, "could not find regulators sub-node\n");
		return -EINVAL;
	}

	m2296->num_regulators = of_get_child_count(regulators_np);
	dev_info(m2296->chip->dev, "num_regulators %d\n",
			m2296->num_regulators);
	if (m2296->num_regulators != M2296_NUM_REGULATORS) {
		dev_info(m2296->chip->dev, "m2296->num_regulators=%d, enum-max=%d\n",
			m2296->num_regulators, M2296_NUM_REGULATORS);
		return -EINVAL;
	}

	for (i = 0; i < m2296->num_regulators; i++) {
		drvdata = &m2296_regulator_drvdata_tbl[i];
		rdesc = &drvdata->rdesc;
		chd_np = of_get_child_by_name(regulators_np, rdesc->name);
		if (!chd_np) {
			dev_info(m2296->chip->dev, "cannot find the chd_np\n");
			continue;
		}

		init_data = of_get_regulator_init_data(m2296->chip->dev,
					chd_np, rdesc);
		if (!init_data) {
			dev_info(m2296->chip->dev, "cannot get the init_data from chd_np\n");
			continue;
		}

		rcfg.init_data = init_data;

		rcfg.dev = m2296->chip->dev;
		rcfg.regmap = m2296->chip->regmap;
		rcfg.driver_data = drvdata;
		rcfg.of_node = chd_np;

		rdev = devm_regulator_register(m2296->chip->dev, rdesc, &rcfg);
		if (IS_ERR(rdev)) {
			ret = PTR_ERR(rdev);
			dev_info(m2296->chip->dev, "register %s fail(%d)\n",
					rdesc->name, ret);
			return ret;
		}

		constraints = rdev->constraints;
		constraints->valid_modes_mask |= REGULATOR_MODE_NORMAL |
						 REGULATOR_MODE_STANDBY |
						 REGULATOR_MODE_FAST;
		constraints->valid_ops_mask |= REGULATOR_CHANGE_MODE;
	}

	return 0;
}

static int m2296_probe(struct platform_device *pdev)
{
	struct m2296_chip *chip = NULL;
	struct m2296_data *m2296;
	int err = -1;

	chip = dev_get_drvdata(pdev->dev.parent);
	if (!chip) {
		dev_info(&pdev->dev, "No chip found\n");
		return -EINVAL;
	}

	m2296 = devm_kzalloc(&pdev->dev, sizeof(*m2296), GFP_KERNEL);
	if (!m2296)
		return -ENOMEM;

	m2296->chip = chip;
	m2296->num_regulators = 0;
	platform_set_drvdata(pdev, m2296);

	err = m2296_register_regulator(m2296);
	if (err < 0) {
		dev_info(&pdev->dev, "register regulator failed\n");
		devm_kfree(&pdev->dev, m2296);
		return -EINVAL;
	}

	return 0;
}

static struct platform_driver m2296_regulator = {
	.driver = {
		.name = "m2296-pmic"
	},
	.probe = m2296_probe,
	//.remove = m2296_remove,
};

module_platform_driver(m2296_regulator);

MODULE_DESCRIPTION("M2296 Management IC driver");
MODULE_LICENSE("GPL v2");

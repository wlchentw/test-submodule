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

#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/mfd/m2296.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/kthread.h>

struct m2296_chip *g_m2296_chip;

static inline int m2296_i2c_read_byte(struct m2296_chip *chip, u8 reg, u8 *data)
{
	int ret = 0;
	int cureg = 0;

	if (chip->regmap != NULL) {
		ret = regmap_read(chip->regmap, reg, &cureg);
		if (ret < 0) {
			dev_info(chip->dev, "read register[0x%02x] failed [%d]\n",
					reg, ret);
			return ret;
		}
		*data = (cureg & 0xFF);
	}

	return 0;
}

int m2296_chip_i2c_read_byte(struct m2296_chip *chip, u8 cmd, u8 *data)
{
	int ret = 0;

	mutex_lock(&chip->rw_lock);
	ret = m2296_i2c_read_byte(chip, cmd, data);
	mutex_unlock(&chip->rw_lock);

	return ret;
}

static inline int m2296_i2c_write_byte(struct m2296_chip *chip, u8 reg, u8 data)
{
	int ret = 0;

	if (chip->regmap != NULL)
		ret = regmap_write(chip->regmap, reg, data);
	if (ret < 0)
		dev_info(chip->dev, "reg 0x%02X = 0x%02X fail(%d)\n",
				reg, data, ret);

	return ret;
}

int m2296_chip_i2c_write_byte(struct m2296_chip *chip, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&chip->rw_lock);
	ret = m2296_i2c_write_byte(chip, cmd, data);
	mutex_unlock(&chip->rw_lock);

	return ret;
}

static inline int __m2296_i2c_block_read
			(struct m2296_chip *chip, u8 reg, u32 len, u8 *data)
{
	int ret = 0;

	ret = regmap_bulk_read(chip->regmap, reg, data, len);

	return ret;
}

static int m2296_chip_i2_block_read
			(struct m2296_chip *chip, u8 reg, u32 len, u8 *data)
{
	int ret = 0;

	mutex_lock(&chip->rw_lock);
	ret = __m2296_i2c_block_read(chip, reg, len, data);
	mutex_unlock(&chip->rw_lock);

	return ret;
}

int m2296_update(struct m2296_chip *chip,
			unsigned int reg, unsigned int mask,
			unsigned int data)
{
	int rval = -1;

	if (chip->regmap != NULL)
		rval = regmap_update_bits(chip->regmap, reg, mask, data);
	else
		dev_info(chip->dev,
		"%s regmap is NULL,cannot update\n", __func__);

	return rval;
}

///sysfs
static ssize_t m2296_dump_regs
		(struct m2296_chip *chip, char *buf, size_t bufLen)
{
	u8 regValue = 0;
	int err = -1;
	int icount = 0;
	int len = 0;
	char tmp[] = "[Register] [Value]";
	int reglist[] = {
		M2296_REG_INTR_A,
		M2296_REG_INTR_B,
		M2296_REG_INTR_A_MASK,
		M2296_REG_INTR_B_MASK,
		M2296_REG_STATUS_A,
		M2296_REG_STATUS_B,
		M2296_REG_START_UP_STATUS,
		M2296_REG_SYS_CONTROL,
		M2296_REG_DCDC1_VOLT,
		M2296_REG_DCDC1_SLPVOLT,
		M2296_REG_DC1_MODE_CONTROL,
		M2296_REG_DC2DC3_MODE_CONTROL,
		M2296_REG_DC4_MODE_CONTROL,
		M2296_REG_LDO1LDO2_VOLT,
		M2296_REG_LDO3LDO4_VOLT,
		M2296_REG_LDO5LDO6_VOLT,
		M2296_REG_LDO7LDO8_VOLT,
		M2296_REG_LDO9_VOLT,
		M2296_REG_LDO10_RTCLDO_VOLT,
		M2296_REG_LDO1LDO2_SLPVOLT,
		M2296_REG_LDO3_SLPVOLT,
		M2296_REG_DCDC2_VOLT,
		M2296_REG_LDO7LDO8_SLPVOLT,
		M2296_REG_LDO9_SLPVOLT,
		M2296_REG_LDO10_SLPVOLT,
		M2296_REG_LDO1_LDO4_MODE_CONTROL,
		M2296_REG_LDO5_LDO8_MODE_CONTROL,
		M2296_REG_LDO9_LDO10_MODE_CONTROL,
		M2296_REG_LDO1_LDO8_ONOFF_CONTROL,
		M2296_REG_LDO9_DC4_ONOFF_CONTROL,
		M2296_REG_ADC_CODE_VBAT_BIT11_BIT4,
		M2296_REG_ADC_CODE_VBAT_BIT3_BIT0,
		M2296_REG_ADC_CODE_ADC1_BIT11_BIT4,
		M2296_REG_ADC_CODE_ADC1_BIT3_BIT0,
		M2296_REG_ADC_CODE_ADC2_BIT11_BIT4,
		M2296_REG_ADC_CODE_ADC2_BIT3_BIT0,
		M2296_REG_RTC_CONTROL1,
		M2296_REG_RTC_CONTROL2,
		M2296_REG_Seconds_BCD,
		M2296_REG_Minutes_BCD,
		M2296_REG_Hours_BCD,
		M2296_REG_Days_BCD,
		M2296_REG_Weekdays,
		M2296_REG_Months_Century_BCD,
		M2296_REG_Years_BCD,
		M2296_REG_Minutes_Alarm,
		M2296_REG_Hour_Alarm,
		M2296_REG_Day_Alarm_BCD,
		M2296_REG_Weekday_Alarm,
		M2296_REG_Second_Alarm,
		M2296_REG_Timer_Control,
		M2296_REG_Timer_Countdown_Value,
		M2296_REG_ENDISCH1,
		M2296_REG_ENDISCH2,
		M2296_REG_PWRKEY_Control1,
		M2296_REG_PWRKEY_Control2,
		M2296_REG_VERSION,
		M2296_REG_DCDC_Fault_Status,
		M2296_REG_LDO_Fault_Status,
		M2296_REG_DCDC_Interrupt_Mask,
		M2296_REG_LDO_Interrupt_Mask,
		M2296_REG_User_Reserved,
		/*M2296_REG_GMT_Testing,*/
	};
	int regNum = ARRAY_SIZE(reglist);
	//int regFirst = M2296_REG_INTR_A;
	//int regNum = (M2296_MAX_REGISTER + 0x01);
	int curReg = 0;

	len += snprintf(buf + len, bufLen - len, "%s\n", tmp);
	for (icount = 0; icount < regNum; icount++) {
		//curReg = (regFirst + icount * 0x01);
		// the last reg is 0xf1
		//if (icount == regNum) {
		//	curReg = M2296_REG_GMT_Testing;
		//}
		curReg = reglist[icount];
		err = m2296_chip_i2c_read_byte(chip, curReg, &regValue);
		if (err != 0) {
			dev_info(chip->dev, "read reg:0x%02x failed:%d\n",
					curReg, regValue);
			continue;
		}
		len += snprintf((buf + len), (bufLen - len),
			"0x%02x::0x%02x\n", curReg, regValue);
	}

	return len;
}

static ssize_t m2296_show_allreg(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct m2296_chip *chip = dev_get_drvdata(dev);

	return m2296_dump_regs(chip, buf, PAGE_SIZE);
}

static DEVICE_ATTR(allreg, 0444, m2296_show_allreg, NULL);

u8 g_reg_value;
static ssize_t show_pmic_access(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	pr_notice("[show_pmic_access] 0x%x\n", g_reg_value);
	return sprintf(buf, "%04X\n", g_reg_value);
}

static ssize_t store_pmic_access(struct device *dev,
		struct device_attribute *attr, const char *buf,
				 size_t size)
{
	int ret = 0;
	char temp_buf[32];
	char *pvalue;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;
	struct m2296_chip *chip = dev_get_drvdata(dev);

	strncpy(temp_buf, buf, sizeof(temp_buf));
	temp_buf[sizeof(temp_buf) - 1] = 0;
	pvalue = temp_buf;

	if (size != 0) {
		if (size > 5) {
			ret = kstrtouint(strsep(&pvalue, " "), 16,
					&reg_address);
			if (ret)
				return ret;
			ret = kstrtouint(pvalue, 16, &reg_value);
			if (ret)
				return ret;
			pr_notice("[store_pmic_access] write reg 0x%x with 0x%x!\n",
					reg_address, reg_value);
			ret = m2296_chip_i2c_write_byte(chip, reg_address,
						reg_value);
		} else {
			ret = kstrtouint(pvalue, 16, &reg_address);
			if (ret)
				return ret;
			ret = m2296_chip_i2c_read_byte(chip, reg_address,
						&g_reg_value);
			pr_notice("[store_pmic_access] read reg 0x%x with 0x%x!\n",
				reg_address, g_reg_value);
		}
	}
	return size;
}

static DEVICE_ATTR(pmic_access, 0664, show_pmic_access, store_pmic_access);

static struct attribute *m2296_attributes[] = {
	&dev_attr_allreg.attr,
	&dev_attr_pmic_access.attr,
	NULL,
};

static const struct attribute_group m2296_group = {
	.attrs = m2296_attributes,
};

static int m2296_i2c_chip_sysfs_create(struct m2296_chip *chip)
{
	int err = -1;

	err = sysfs_create_group(&chip->dev->kobj, &m2296_group);
	if (err < 0) {
		dev_info(chip->dev, "create the sysfs failed:%d\n",
				err);
	}

	return err;
}

static int m2296_i2c_chip_remove_sysfs(struct m2296_chip *chip)
{
	sysfs_remove_group(&chip->dev->kobj, &m2296_group);

	return 0;
}

static int m2296_i2c_chip_clear_interrupt(struct m2296_chip *chip)
{
	int err = -1;

	err = m2296_update(chip, M2296_REG_INTR_A,
					M2296_INTR_A_INT, INTR_A_INT_HIZ);
	if (err != 0) {
		dev_info(chip->dev,
			"m2296 update clear interrupt reg failed:%d\n", err);
		return err;
	}

	return err;
}

static int m2296_pmic_power_it_keys(struct m2296_chip *chip, int pressed)
{
	if (!chip->powkeycode)
		return -1;

	input_report_key(chip->input_dev, chip->powkeycode, pressed);
	input_sync(chip->input_dev);
	if (pressed)
		__pm_stay_awake(chip->pwrkey_lock);
	else
		__pm_wakeup_event(chip->pwrkey_lock, HZ/2);

	dev_info(chip->dev, "[M2296_PMICKEYS] (%s) key =%d using PMIC\n",
			pressed ? "pressed" : "released", chip->powkeycode);

	return 0;
}

static int pressed_flag;

static int m2296_get_pwron_status(struct m2296_chip *chip)
{
	int pressed = 0;
	int err = -1;
	u8 data = 0;

	mdelay(30);  //for powerkey debounce
	err = m2296_chip_i2c_read_byte(chip, M2296_REG_STATUS_A, &data);
	if (err != 0) {
		dev_info(chip->dev, "read PWRON failed:%d\n", err);
		return -1;
	}

	/** BIT5 PWRON **/
	if ((data >> 0x05) & 0x01)
		pressed = 1;

	if (pressed == 1 && pressed_flag == 0) {
		pressed_flag = 1;
		return pressed;
	} else if (pressed == 0 && pressed_flag == 1) {
		pressed_flag = 0;
		return pressed;
	} else
		return -1;
}

//turn off some voltage which donot use in this project
int m2296_i2c_chip_close(struct m2296_chip *chip)
{
	int err = -1;

	/** close ONLDO1 & ONLDO8 **/
	err = m2296_update(chip,
		M2296_REG_LDO1_LDO8_ONOFF_CONTROL, M2296_ONLDO1,
		ONLDO1_OFF);
	err |= m2296_update(chip,
		M2296_REG_LDO1_LDO8_ONOFF_CONTROL, M2296_ONLDO8,
		ONLDO8_OFF);
	if (err != 0) {
		dev_info(chip->dev,
		"m2296 update ONLDO1 & ONLDO8 failed:%d\n", err);
		return err;
	}

	/**close ONLDO10 & ONLDO9 **/
	err = m2296_update(chip,
		M2296_REG_LDO9_DC4_ONOFF_CONTROL, M2296_ONLDO9,
		(unsigned int)~M2296_ONLDO9);
	err |= m2296_update(chip,
		M2296_REG_LDO9_DC4_ONOFF_CONTROL, M2296_ONLDO10,
		(unsigned int)~M2296_ONLDO10);
	if (err != 0) {
		dev_info(chip->dev,
			"m2296 update ONLDO9& ONLDO10 failed:%d\n", err);
		return err;
	}

	return 0;
}

///initialize
static int m2296_i2c_chip_init(struct m2296_chip *chip)
{
	int err = -1;

	/** turn off some voltage ---these donot use this project **/
	err = m2296_i2c_chip_close(chip);
	if (err < 0) {
		dev_info(chip->dev, "close some voltage failed:%d\n", err);
		return err;
	}

	/**maybe we should clear the interrupt **/
	err = m2296_i2c_chip_clear_interrupt(chip);
	if (err != 0) {
		dev_info(chip->dev, "m2296 i2c chip clear interrupt failed:%d\n",
				err);
		return -1;
	}

	return 0;
}

///version read and check
static int m2296_i2c_chip_check_version(struct m2296_chip *chip)
{
	u8 version = -1;
	int ret = -1;

	ret = i2c_smbus_read_byte_data(chip->client, M2296_REG_VERSION);
	if (ret < 0) {
		dev_info(chip->dev, "m2296 i2c read failed:%d\n", ret);
		return false;
	}

	version = (ret & 0xff);
	dev_info(chip->dev, "version=0x%02x\n", version);

	if (version == M2296_CHIP_ID) {
		chip->dev_rev = version;
		return true;
	}

	return false;
}

//power key work
static int m2296_regmap_pmic_power_it_keys(struct m2296_chip *chip)
{
	int pressed = 0;
	int err = -1;

	pressed = m2296_get_pwron_status(chip);
	if (pressed < 0) {
		dev_info(chip->dev, "cannot get PWRON_IT status:%d\n", pressed);
		return -1;
	}

	err = m2296_pmic_power_it_keys(chip, pressed);
	if (err < 0) {
		dev_info(chip->dev, "report powekey failed:%d\n", err);
		return -1;
	}

	/** we need record,if we had report pressed,need report released **/
	if (pressed)
		chip->bpwron_it_release = true;
	else
		chip->bpwron_it_release = false;

	return 0;
}

//read irq reg and return the value
enum m2296_interrupt_type m2296_get_interrupt_type(struct m2296_chip *chip)
{
	int err = -1;
	u8 buf[M2296_IRQIDX_MAX] = { 0x00};
	enum m2296_interrupt_type m_inter_type = M2296_REG_INT_UNKNOWN;

	err = m2296_chip_i2_block_read(chip, M2296_REG_INTR_A,
		M2296_IRQIDX_MAX, buf);
	if (err < 0) {
		dev_info(chip->dev,
			"read INTR_A & INTR_B & INTR_A_MASK & INTR_B_MASK fail(%d)\n",
			err);
		return M2296_REG_INT_UNKNOWN;
	}

	if ((buf[0] >> 0x02) & 0x01)
		m_inter_type = M2296_REG_INT_EOC_ADC;
	else if ((buf[0] >> 0x03) & 0x01)
		m_inter_type = M2296_REG_INT_PWRON_IT;
	else if ((buf[0] >> 0x04) & 0x01)
		m_inter_type = M2296_REG_INT_PWRON_LP;
	else if ((buf[0] >> 0x05) & 0x01)
		m_inter_type = M2296_REG_INT_PWRON;
	else if ((buf[1] >> 0x00) & 0x01)
		m_inter_type = M2296_REG_INT_RTC_ALARM;
	else if ((buf[1] >> 0x03) & 0x01)
		m_inter_type = M2296_REG_INT_T1MIN;
	else if ((buf[1] >> 0x04) & 0x01)
		m_inter_type = M2296_REG_INT_T1HOUR;

	return m_inter_type;
}

static int m2296_handle_pre_irq(void *irq_drv_data)
{
	struct m2296_chip *chip = irq_drv_data;
	enum m2296_interrupt_type m_inter_type = M2296_REG_INT_UNKNOWN;
	u8 data1 = 0, data2 = 0;
	int err;

	m_inter_type = m2296_get_interrupt_type(chip);

	if (m_inter_type == M2296_REG_INT_PWRON) {
		m2296_regmap_pmic_power_it_keys(chip);

		/** BIT5 PWRON **/
		do {
			err = m2296_chip_i2c_read_byte(chip,
					M2296_REG_STATUS_A, &data1);
			if (err != 0) {
				dev_info(chip->dev, "read PWRON failed:%d\n",
						err);
				return -1;
			}
			mdelay(1);
			err = m2296_chip_i2c_read_byte(chip,
					M2296_REG_STATUS_A, &data2);
			if (err != 0) {
				dev_info(chip->dev, "read PWRON failed:%d\n",
						err);
				return -1;
			}
		} while (((data1 >> 0x05) & 0x01) | ((data2 >> 0x05) & 0x01));

		m2296_regmap_pmic_power_it_keys(chip);
	}

	return 0;
}

static int m2296_handle_post_irq(void *irq_drv_data)
{
	struct m2296_chip *chip = irq_drv_data;
	int ret = -1;

	ret = m2296_update(chip, M2296_REG_INTR_A,
					M2296_INTR_A_INT, INTR_A_INT_HIZ);
	if (ret) {
		dev_info(chip->dev, "Failed to clear interrupt\n");
		return ret;
	}

	return 0;
}

static struct regmap_irq m2296_irqs[] = {
	REGMAP_IRQ_REG
	(M2296_REG_INT_PWRON_LP, M2296_REG_INTR_A_OFFSET, INTR_A_PWRON_LP),
	REGMAP_IRQ_REG
	(M2296_REG_INT_PWRON, M2296_REG_INTR_A_OFFSET, INTR_A_PWRON),
	REGMAP_IRQ_REG
	(M2296_REG_INT_RTC_ALARM, M2296_REG_INTR_B_OFFSET, INTR_B_RTC_ALARM),
};

static struct regmap_irq_chip m2219_irq_chip = {
	.name = "m2296_irq",
	.irqs = m2296_irqs,
	.num_irqs = ARRAY_SIZE(m2296_irqs),
	.num_regs = 0x02,
	.irq_reg_stride = 1,
	.status_base = M2296_REG_INTR_A,
	.mask_base = M2296_REG_INTR_A_MASK,
	.ack_base = M2296_REG_INTR_A,
	.mask_invert = true,
	.handle_pre_irq = m2296_handle_pre_irq,
	.handle_post_irq = m2296_handle_post_irq,
};

/////mfd_cell
static struct mfd_cell m2296_mfd_cells[] = {
	{ .name = "m2296-pmic", },
};

///regmap config
static struct regmap_config m2296_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = M2296_MAX_REGISTER,
};

////regmap irq
static int m2296_i2c_chip_regmap_irq_register(struct m2296_chip *chip)
{
	int err = -1;


	m2219_irq_chip.irq_drv_data = chip;
	err = devm_regmap_add_irq_chip(chip->dev, chip->regmap,
		       chip->irq, IRQF_ONESHOT, 0,
		       &m2219_irq_chip,
		       &chip->reg_irq_data);
	if (err) {
		dev_info(chip->dev, "Failed to add IRQ chip\n");
		return -1;
	}

	device_init_wakeup(chip->dev, true);

	return 0;
}

static int m2296_pmic_key_setup(struct m2296_chip *chip, const char *name)
{
	int err = -1;

	err = of_property_read_u32(chip->dev->of_node, name, &chip->powkeycode);
	dev_info(chip->dev, "powkeycode[%d]\n", chip->powkeycode);
	if (err)
		return -1;

	if (!chip->powkeycode)
		return -1;

	__set_bit(chip->powkeycode, chip->input_dev->keybit);

	return 0;
}

//input dev register
static int m2296_i2c_chip_input_dev_register(struct m2296_chip *chip)
{
	int err = -1;

	chip->pwrkey_lock = wakeup_source_register("pwrkey wakelock");
	if (!chip->pwrkey_lock)
		return -ENOMEM;

	chip->input_dev = devm_input_allocate_device(chip->dev);
	if (!chip->input_dev)
		return -ENOMEM;

	chip->input_dev->name = "m2296-pmic-keys";
	chip->input_dev->id.bustype = BUS_HOST;
	chip->input_dev->id.vendor = 0x0001;
	chip->input_dev->id.product = 0x0001;
	chip->input_dev->id.version = 0x0001;
	chip->input_dev->dev.parent = chip->dev;

	__set_bit(EV_KEY, chip->input_dev->evbit);

	err = m2296_pmic_key_setup(chip, "mediatek,pwrkey-code");
	if (err < 0) {
		dev_info(chip->dev, "m2296 set pmic key failed:%d\n", err);
		input_free_device(chip->input_dev);
		return -1;
	}

	err = input_register_device(chip->input_dev);
	if (err) {
		input_free_device(chip->input_dev);
		return -1;
	}

	input_set_drvdata(chip->input_dev, chip);

	return 0;
}

static int m2296_i2c_chip_probe(struct m2296_chip *chip)
{
	int err = -1;

	pressed_flag = 0;

	chip->regmap = devm_regmap_init_i2c(chip->client, &m2296_regmap);
	if (IS_ERR(chip->regmap)) {
		dev_info(chip->dev, "Failed to initialize Regmap\n");
		return PTR_ERR(chip->regmap);
	}

	if (chip->irq < 0) {
		dev_info(chip->dev, "No IRQ configured\n");
		return -EINVAL;
	}

	err = m2296_i2c_chip_init(chip);
	if (err < 0) {
		dev_info(chip->dev, "Failed to init the m2296 chip\n");
		return err;
	}

	/** register input dev **/
	err = m2296_i2c_chip_input_dev_register(chip);
	if (err < 0) {
		dev_info(chip->dev, "input dev register failed:%d\n", err);
		return err;
	}

	err = m2296_i2c_chip_regmap_irq_register(chip);
	if (err < 0) {
		dev_info(chip->dev, "Failed to register regmap irq\n");
		return err;
	}

	err = devm_mfd_add_devices(chip->dev, PLATFORM_DEVID_AUTO,
			   m2296_mfd_cells,
			   ARRAY_SIZE(m2296_mfd_cells), NULL, 0,
			   regmap_irq_get_domain(chip->reg_irq_data));
	if (err) {
		dev_info(chip->dev, "Failed to create subdevices\n");
		return err;
	}

	return 0;
}

static void m2296_power_off(void)
{
	pr_notice("[%s] power off system\n", __func__);

	if (g_m2296_chip) {
		m2296_chip_i2c_write_byte(g_m2296_chip,
				M2296_REG_SYS_CONTROL, 0x00);
	}

	while (1) {
		mdelay(1000);
		pr_notice("[%s] power off\n", __func__);
	};
}

static int m2296_i2c_chip_is_system_power(struct m2296_chip *chip)
{
	if (of_device_is_system_power_controller(chip->client->dev.of_node)) {
		g_m2296_chip = chip;
		if (!pm_power_off)
			pm_power_off = m2296_power_off;
		else
			dev_info(chip->dev, "poweroff callback already assigned\n");
	}

	return 0;
}


static int m2296_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int ret;
	struct m2296_chip *chip = NULL;

	if (!client->irq) {
		dev_info(&client->dev, "No IRQ configured\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = &client->dev;
	chip->irq = client->irq;
	chip->bpwron_it_release = false;
	mutex_init(&chip->rw_lock);
	i2c_set_clientdata(client, chip);

	ret = m2296_i2c_chip_check_version(chip);
	if (ret == 0) {
		dev_info(&client->dev, "check version failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = m2296_i2c_chip_probe(chip);
	if (ret != 0) {
		dev_info(&client->dev, "chip probe failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = m2296_i2c_chip_is_system_power(chip);
	if (ret < 0) {
		dev_info(&client->dev, "check is system power failed:%d\n",
				ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = m2296_i2c_chip_sysfs_create(chip);
	if (ret < 0)
		dev_info(&client->dev, "chip create sysfs failed:%d\n", ret);

	dev_info(chip->dev, "m2296 PMU probe successfully!\n");

	return 0;
}

#ifdef CONFIG_PM_SLEEP
int m2296_i2c_chip_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct m2296_chip *chip = i2c_get_clientdata(client);
	bool bwakeup = false;
	int err = -1;

	bwakeup = device_may_wakeup(dev);
	if ((bwakeup) && (chip->irq >= 0)) {
		err = enable_irq_wake(chip->irq);
		if (err) {
			dev_info(chip->dev, "%s: set_irq_wakeup failed\n",
					__func__);
			return err;
		}
	}

	return 0;
}

int m2296_i2c_chip_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct m2296_chip *chip = i2c_get_clientdata(client);
	bool bwakeup = false;
	int err = -1;

	bwakeup = device_may_wakeup(dev);
	if ((bwakeup) && (chip->irq >= 0)) {
		err = disable_irq_wake(chip->irq);
		if (err) {
			dev_info(chip->dev, "%s: disable_irq_wake failed\n",
					__func__);
			return err;
		}
	}

	return 0;
}

static const struct dev_pm_ops m2296_pm_ops = {
	.suspend = m2296_i2c_chip_suspend,
	.resume = m2296_i2c_chip_resume,
};
#endif

static int m2296_i2c_chip_remove(struct i2c_client *client)
{
	struct m2296_chip *chip = i2c_get_clientdata(client);

	mutex_destroy(&chip->rw_lock);
	m2296_i2c_chip_remove_sysfs(chip);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id m2296_of_match[] = {
	{ .compatible = "mediatek, m2296", },
	{ },
};
#endif

static const struct i2c_device_id m2296_dev_id[] = {
	{"m2296", 0},
	{},
};

static struct i2c_driver m2296_drv = {
	.driver = {
		.name = "mediatek-m2296",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = m2296_of_match,
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &m2296_pm_ops,
#endif
	},
	.probe = m2296_i2c_probe,
	.remove = m2296_i2c_chip_remove,
	.id_table = m2296_dev_id,
};

module_i2c_driver(m2296_drv);

MODULE_DESCRIPTION("M2296 Management IC driver");
MODULE_LICENSE("GPL v2");

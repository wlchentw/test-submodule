// SPDX-License-Identifier: GPL-2.0-or-later
//
// Copyright (C) 2019 ROHM Semiconductors
//
// ROHM BD71828 PMIC driver

#include <linux/gpio_keys.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/mfd/core.h>
#include <linux/mfd/rohm-bd71828.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/pm.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/reboot.h>
#include <asm/system_misc.h>

//extern void battery_stop_schedule(void);

//extern void max20342_VCCINTOnBAT(void);

static struct gpio_keys_button button = {
	.code = KEY_POWER,
	.gpio = -1,
	.type = EV_KEY,
	.wakeup = 1,
};

static struct bd71828_pwrkey_platform_data bd71828_powerkey_data = {
	.buttons = &button,
	.nbuttons = 1,
	.name = "bd71828-pwrkey",
};

static const struct resource rtc_irqs[] = {
	DEFINE_RES_IRQ_NAMED(BD71828_INT_RTC0, "bd71828-rtc-alm-0"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_RTC1, "bd71828-rtc-alm-1"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_RTC2, "bd71828-rtc-alm-2"),
};

static const struct resource charger_irqs[] = {
	//DEFINE_RES_IRQ_NAMED(BD71828_INT_LONGPUSH, "bd71828-pwr-longpush"),
	//DEFINE_RES_IRQ_NAMED(BD71828_INT_MIDPUSH, "bd71828-pwr-midpush"),
	//DEFINE_RES_IRQ_NAMED(BD71828_INT_PUSH, "bd71828-pwr-push"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_DCIN_DET, "bd71828-pwr-dcin-in"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_DCIN_RMV, "bd71828-pwr-dcin-out"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_BAT_MON_RES, "bd71828-vbat-normal"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_BAT_MON_DET, "bd71828-vbat-low"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_BAT_HI_DET, "bd71828-btemp-hi"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_BAT_HI_RES, "bd71828-btemp-cool"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_BAT_LOW_DET, "bd71828-btemp-lo"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_BAT_LOW_RES,
					"bd71828-btemp-warm"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_CHIP_OVER_125_RES,
					"bd71828-temp-125-under"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_CHIP_OVER_125_DET,
					"bd71828-temp-125-over"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_CHIP_OVER_VF_DET,
					"bd71828-temp-hi"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_TEMP_CHIP_OVER_VF_RES,
					"bd71828-temp-norm"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_CHG_TOPOFF_TO_DONE, "bd71828-charger-done"),
	DEFINE_RES_IRQ_NAMED(BD71828_INT_CHG_WDG_TIME, "bd71828-charger-wdg-expired"),
};

static const struct resource lid_irqs[] = {
	DEFINE_RES_IRQ_NAMED(BD71828_INT_VSYS_HALL_TOGGLE, "bd71828-hall"),
};

static struct mfd_cell bd71828_mfd_cells[] = {
	{ .name = "bd71828-pmic", },
	{ .name = "bd71828-gpio", },
	{ .name = "bd71828-led", },
	/*
	 * We use BD71837 driver to drive the clock block. Only differences to
	 * BD70528 clock gate are the register address and mask.
	 */
	{ .name = "bd718xx-clk", },
	{
		.name = "bd71827-power",
		.resources = charger_irqs,
		.num_resources = ARRAY_SIZE(charger_irqs),
	}, {
		.name = "bd70528-rtc",
		.resources = rtc_irqs,
		.num_resources = ARRAY_SIZE(rtc_irqs),
	}/*, {
		.name = "bd71828-lid",
		.resources = lid_irqs,
		.num_resources = ARRAY_SIZE(lid_irqs),
	}*/, {
		.name = "bd71828-pwrkey",
		.platform_data = &bd71828_powerkey_data,
		.pdata_size = sizeof(bd71828_powerkey_data),
	},
};

static const struct regmap_range volatile_ranges[] = {
	{
		.range_min = BD71828_REG_PS_CTRL_1,
		.range_max = BD71828_REG_PS_CTRL_1,
	}, {
		.range_min = BD71828_REG_PS_CTRL_3,
		.range_max = BD71828_REG_PS_CTRL_3,
	}, {
		.range_min = BD71828_REG_RTC_SEC,
		.range_max = BD71828_REG_RTC_YEAR,
	}, {
		/*
		 * For now make all charger registers volatile because many
		 * needs to be and because the charger block is not that
		 * performance critical. TBD: Check which charger registers
		 * could be cached
		 */
		.range_min = BD71828_REG_CHG_STATE,
		.range_max = BD71828_REG_CHG_FULL,
	}, {
		.range_min = BD71828_REG_INT_MAIN,
		.range_max = BD71828_REG_IO_STAT,
	},
};

static const struct regmap_access_table volatile_regs = {
	.yes_ranges = &volatile_ranges[0],
	.n_yes_ranges = ARRAY_SIZE(volatile_ranges),
};

static struct regmap_config bd71828_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &volatile_regs,
	.max_register = BD71828_MAX_REGISTER,
	.cache_type = REGCACHE_NONE,
};

/*
 * Mapping of main IRQ register bits to sub-IRQ register offsets so that we can
 * access corect sub-IRQ registers based on bits that are set in main IRQ
 * register.
 */

unsigned int bit0_offsets[] = {11};		/* RTC IRQ register */
unsigned int bit1_offsets[] = {10};		/* TEMP IRQ register */
unsigned int bit2_offsets[] = {6, 7, 8, 9};	/* BAT MON IRQ registers */
unsigned int bit3_offsets[] = {5};		/* BAT IRQ register */
unsigned int bit4_offsets[] = {4};		/* CHG IRQ register */
unsigned int bit5_offsets[] = {3};		/* VSYS IRQ register */
unsigned int bit6_offsets[] = {1, 2};		/* DCIN IRQ registers */
unsigned int bit7_offsets[] = {0};		/* BUCK IRQ register */

static struct regmap_irq_sub_irq_map bd71828_sub_irq_offsets[] = {
	REGMAP_IRQ_MAIN_REG_OFFSET(bit0_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit1_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit2_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit3_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit4_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit5_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit6_offsets),
	REGMAP_IRQ_MAIN_REG_OFFSET(bit7_offsets),
};

static struct regmap_irq bd71828_irqs[] = {
	REGMAP_IRQ_REG(BD71828_INT_BUCK1_OCP, 0, BD71828_INT_BUCK1_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK2_OCP, 0, BD71828_INT_BUCK2_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK3_OCP, 0, BD71828_INT_BUCK3_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK4_OCP, 0, BD71828_INT_BUCK4_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK5_OCP, 0, BD71828_INT_BUCK5_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK6_OCP, 0, BD71828_INT_BUCK6_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BUCK7_OCP, 0, BD71828_INT_BUCK7_OCP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_PGFAULT, 0, BD71828_INT_PGFAULT_MASK),
	/* DCIN1 interrupts */
	REGMAP_IRQ_REG(BD71828_INT_DCIN_DET, 1, BD71828_INT_DCIN_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_DCIN_RMV, 1, BD71828_INT_DCIN_RMV_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CLPS_OUT, 1, BD71828_INT_CLPS_OUT_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CLPS_IN, 1, BD71828_INT_CLPS_IN_MASK),
	/* DCIN2 interrupts */
	REGMAP_IRQ_REG(BD71828_INT_DCIN_MON_RES, 2,
		       BD71828_INT_DCIN_MON_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_DCIN_MON_DET, 2,
		       BD71828_INT_DCIN_MON_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_LONGPUSH, 2, BD71828_INT_LONGPUSH_MASK),
	REGMAP_IRQ_REG(BD71828_INT_MIDPUSH, 2, BD71828_INT_MIDPUSH_MASK),
	REGMAP_IRQ_REG(BD71828_INT_SHORTPUSH, 2, BD71828_INT_SHORTPUSH_MASK),
	REGMAP_IRQ_REG(BD71828_INT_PUSH, 2, BD71828_INT_PUSH_MASK),
	REGMAP_IRQ_REG(BD71828_INT_WDOG, 2, BD71828_INT_WDOG_MASK),
	REGMAP_IRQ_REG(BD71828_INT_SWRESET, 2, BD71828_INT_SWRESET_MASK),
	/* Vsys */
	REGMAP_IRQ_REG(BD71828_INT_VSYS_UV_RES, 3,
		       BD71828_INT_VSYS_UV_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_UV_DET, 3,
		       BD71828_INT_VSYS_UV_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_LOW_RES, 3,
		       BD71828_INT_VSYS_LOW_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_LOW_DET, 3,
		       BD71828_INT_VSYS_LOW_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_HALL_IN, 3,
		       BD71828_INT_VSYS_HALL_IN_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_HALL_TOGGLE, 3,
		       BD71828_INT_VSYS_HALL_TOGGLE_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_MON_RES, 3,
		       BD71828_INT_VSYS_MON_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_VSYS_MON_DET, 3,
		       BD71828_INT_VSYS_MON_DET_MASK),
	/* Charger */
	REGMAP_IRQ_REG(BD71828_INT_CHG_DCIN_ILIM, 4,
		       BD71828_INT_CHG_DCIN_ILIM_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_TOPOFF_TO_DONE, 4,
		       BD71828_INT_CHG_TOPOFF_TO_DONE_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_WDG_TEMP, 4,
		       BD71828_INT_CHG_WDG_TEMP_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_WDG_TIME, 4,
		       BD71828_INT_CHG_WDG_TIME_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_RECHARGE_RES, 4,
		       BD71828_INT_CHG_RECHARGE_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_RECHARGE_DET, 4,
		       BD71828_INT_CHG_RECHARGE_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_RANGED_TEMP_TRANSITION, 4,
		       BD71828_INT_CHG_RANGED_TEMP_TRANSITION_MASK),
	REGMAP_IRQ_REG(BD71828_INT_CHG_STATE_TRANSITION, 4,
		       BD71828_INT_CHG_STATE_TRANSITION_MASK),
	/* Battery */
	REGMAP_IRQ_REG(BD71828_INT_BAT_TEMP_NORMAL, 5,
		       BD71828_INT_BAT_TEMP_NORMAL_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_TEMP_ERANGE, 5,
		       BD71828_INT_BAT_TEMP_ERANGE_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_TEMP_WARN, 5,
		       BD71828_INT_BAT_TEMP_WARN_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_REMOVED, 5,
		       BD71828_INT_BAT_REMOVED_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_DETECTED, 5,
		       BD71828_INT_BAT_DETECTED_MASK),
	REGMAP_IRQ_REG(BD71828_INT_THERM_REMOVED, 5,
		       BD71828_INT_THERM_REMOVED_MASK),
	REGMAP_IRQ_REG(BD71828_INT_THERM_DETECTED, 5,
		       BD71828_INT_THERM_DETECTED_MASK),
	/* Battery Mon 1 */
	REGMAP_IRQ_REG(BD71828_INT_BAT_DEAD, 6, BD71828_INT_BAT_DEAD_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_SHORTC_RES, 6,
		       BD71828_INT_BAT_SHORTC_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_SHORTC_DET, 6,
		       BD71828_INT_BAT_SHORTC_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_LOW_VOLT_RES, 6,
		       BD71828_INT_BAT_LOW_VOLT_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_LOW_VOLT_DET, 6,
		       BD71828_INT_BAT_LOW_VOLT_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_VOLT_RES, 6,
		       BD71828_INT_BAT_OVER_VOLT_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_VOLT_DET, 6,
		       BD71828_INT_BAT_OVER_VOLT_DET_MASK),
	/* Battery Mon 2 */
	REGMAP_IRQ_REG(BD71828_INT_BAT_MON_RES, 7,
		       BD71828_INT_BAT_MON_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_MON_DET, 7,
		       BD71828_INT_BAT_MON_DET_MASK),
	/* Battery Mon 3 (Coulomb counter) */
	REGMAP_IRQ_REG(BD71828_INT_BAT_CC_MON1, 8,
		       BD71828_INT_BAT_CC_MON1_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_CC_MON2, 8,
		       BD71828_INT_BAT_CC_MON2_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_CC_MON3, 8,
		       BD71828_INT_BAT_CC_MON3_MASK),
	/* Battery Mon 4 */
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_1_RES, 9,
		       BD71828_INT_BAT_OVER_CURR_1_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_1_DET, 9,
		       BD71828_INT_BAT_OVER_CURR_1_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_2_RES, 9,
		       BD71828_INT_BAT_OVER_CURR_2_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_2_DET, 9,
		       BD71828_INT_BAT_OVER_CURR_2_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_3_RES, 9,
		       BD71828_INT_BAT_OVER_CURR_3_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_BAT_OVER_CURR_3_DET, 9,
		       BD71828_INT_BAT_OVER_CURR_3_DET_MASK),
	/* Temperature */
	REGMAP_IRQ_REG(BD71828_INT_TEMP_BAT_LOW_RES, 10,
		       BD71828_INT_TEMP_BAT_LOW_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_BAT_LOW_DET, 10,
		       BD71828_INT_TEMP_BAT_LOW_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_BAT_HI_RES, 10,
		       BD71828_INT_TEMP_BAT_HI_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_BAT_HI_DET, 10,
		       BD71828_INT_TEMP_BAT_HI_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_CHIP_OVER_125_RES, 10,
		       BD71828_INT_TEMP_CHIP_OVER_125_RES_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_CHIP_OVER_125_DET, 10,
		       BD71828_INT_TEMP_CHIP_OVER_125_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_CHIP_OVER_VF_DET, 10,
		       BD71828_INT_TEMP_CHIP_OVER_VF_DET_MASK),
	REGMAP_IRQ_REG(BD71828_INT_TEMP_CHIP_OVER_VF_RES, 10,
		       BD71828_INT_TEMP_CHIP_OVER_VF_RES_MASK),
	/* RTC Alarm */
	REGMAP_IRQ_REG(BD71828_INT_RTC0, 11, BD71828_INT_RTC0_MASK),
	REGMAP_IRQ_REG(BD71828_INT_RTC1, 11, BD71828_INT_RTC1_MASK),
	REGMAP_IRQ_REG(BD71828_INT_RTC2, 11, BD71828_INT_RTC2_MASK),
};

static struct regmap_irq_chip bd71828_irq_chip = {
	.name = "bd71828_irq",
	.main_status = BD71828_REG_INT_MAIN,
	.irqs = &bd71828_irqs[0],
	.num_irqs = ARRAY_SIZE(bd71828_irqs),
	.status_base = BD71828_REG_INT_BUCK,
	.mask_base = BD71828_REG_INT_MASK_BUCK,
	.ack_base = BD71828_REG_INT_BUCK,
	.mask_invert = true,
	.init_ack_masked = true,
	.num_regs = 12,
	.num_main_regs = 1,
	.sub_reg_offsets = &bd71828_sub_irq_offsets[0],
	.num_main_status_bits = 8,
	.irq_reg_stride = 1,
};

static unsigned int g_reg_address;
static ssize_t show_reg_access(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct rohm_regmap_dev *chip;
	unsigned int reg_value = 0;

	chip = dev_get_drvdata(dev);
	regmap_read(chip->regmap, g_reg_address, &reg_value);

	pr_info("[%s] 0x%x = 0x%x\n", __func__, g_reg_address, reg_value);
	return sprintf(buf, "0x%x = 0x%x\n", g_reg_address, reg_value);
}

static ssize_t store_reg_access(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf,
				 size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;
	struct rohm_regmap_dev *chip;

	chip = dev_get_drvdata(dev);
	if (buf != NULL && size != 0) {
		pr_notice("[%s] size is %d, buf is %s\n"
			, __func__, (int)size, buf);

		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		val = strsep(&pvalue, " ");
		if (addr)
			ret = kstrtou32(addr, 16, (unsigned int *)&reg_address);
		if (val) {
			ret = kstrtou32(val, 16, (unsigned int *)&reg_value);
			pr_notice("[%s] write PMU reg 0x%x with value 0x%x !\n"
				, __func__, reg_address, reg_value);
			ret = regmap_write(chip->regmap,
				reg_address, reg_value);
		} else {
			ret = regmap_read(chip->regmap,
				reg_address, &reg_value);
			pr_notice("[%s] read PMU reg 0x%x with value 0x%x !\n"
				, __func__, reg_address, reg_value);
		}
		g_reg_address = reg_address;
	}

	return size;
}

static DEVICE_ATTR(reg_access, 0664, show_reg_access, store_reg_access);

struct rohm_regmap_dev *bd71828_chip;

static void bd71828_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	printk ("[%s-%d] ...\n", __func__, __LINE__);

	#if 0
	battery_stop_schedule();
	#endif
	while (true) {
		printk ("[%s-%d] %s rebooting ...\n", __func__, __LINE__, cmd);
		regmap_write(bd71828_chip->regmap, 6, 0x01);
		mdelay(500);
	}
}

static void bd71828_power_off(void)
{
	pr_notice("[%s] power off system\n", __func__);

	#if 0
	battery_stop_schedule();
	max20342_VCCINTOnBAT();

	//pr_notice("[%s] %d\n", __func__,__LINE__);
	if (bd71828_chip) {
		{
			//keep power off date-time
			unsigned int value;

			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_SEC, &value);
			regmap_write(bd71828_chip->regmap, 0xf4, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_MINUTE, &value);
			regmap_write(bd71828_chip->regmap, 0xf5, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_HOUR, &value);
			regmap_write(bd71828_chip->regmap, 0xf6, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_WEEK, &value);
			regmap_write(bd71828_chip->regmap, 0xf7, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_DAY, &value);
			regmap_write(bd71828_chip->regmap, 0xf8, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_MONTH, &value);
			regmap_write(bd71828_chip->regmap, 0xf9, value);
			regmap_read(bd71828_chip->regmap, BD71828_REG_RTC_YEAR, &value);
			regmap_write(bd71828_chip->regmap, 0xfa, value);
		}
		//pr_notice("[%s] %d\n", __func__,__LINE__);
		regmap_write(bd71828_chip->regmap, BD71828_REG_RESETSRC, 0xff);
		mdelay(500);
		//pr_notice("[%s] %d\n", __func__,__LINE__);
		regmap_write(bd71828_chip->regmap, BD71828_REG_INT_DCIN2, 0xff);
		mdelay (500);
		//pr_notice("[%s] %d\n", __func__,__LINE__);
		//regmap_write(bd71828_chip->regmap, BD71828_REG_PS_CTRL_1, 0x2);
	}
	#endif

	//pr_notice("[%s] %d\n", __func__,__LINE__);
	while (1) {
		pr_notice("[%s] powering off\n", __func__);
		regmap_write(bd71828_chip->regmap, BD71828_REG_PS_CTRL_1, 0x2);
		mdelay(1000);
	};
}

static int bd71828_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct rohm_regmap_dev *chip;
	struct regmap_irq_chip_data *irq_data;
	int ret;
	int button_irq = BD71828_INT_PUSH;
	unsigned int data;

	if (!i2c->irq) {
		dev_err(&i2c->dev, "No IRQ configured\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&i2c->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	dev_set_drvdata(&i2c->dev, chip);

	chip->chip_type = ROHM_CHIP_TYPE_BD71828;
	chip->regmap = devm_regmap_init_i2c(i2c, &bd71828_regmap);
	if (IS_ERR(chip->regmap)) {
		dev_err(&i2c->dev, "Failed to initialize Regmap\n");
		return PTR_ERR(chip->regmap);
	}

	{
		struct i2c_board_info info;

		memset(&info, 0, sizeof(struct i2c_board_info));
		info.addr = i2c->addr+1;
		info.platform_data = chip;
		strlcpy(info.type, "bd71828-otp", I2C_NAME_SIZE);
		chip->i2c_otp = i2c_new_device(i2c->adapter,&info);
		if(NULL == chip->i2c_otp) {
			dev_err(&i2c->dev,"Failed to create bd71828 otp\n");
		}

		chip->regmap_otp = devm_regmap_init_i2c(chip->i2c_otp, &bd71828_regmap);
		if (IS_ERR(chip->regmap_otp)) {
			dev_err(&i2c->dev, "Failed to initialize Regmap otp\n");
			return PTR_ERR(chip->regmap_otp);
		}
	}

	ret = regmap_read(chip->regmap, 0x00, &data);
	if (ret < 0) {
		dev_err(&i2c->dev, "device access error\n");
		return -ENODEV;
	}
	dev_err(&i2c->dev, "PMIC ROHM_CHIP_TYPE_BD71828 PMIC ID 0x00 :%02x\n",data);

	// turn on OTP access
	regmap_write(chip->regmap, 0xfe, 0x8c);
	regmap_write(chip->regmap, 0xff, 0x01);
	ret = regmap_read(chip->regmap_otp, 0x00, &data);
	if (ret < 0) {
		dev_err(&i2c->dev, "device access error\n");
		return -ENODEV;
	}
	dev_err(&i2c->dev, "PMIC ROHM_CHIP_TYPE_BD71828 PMIC OTP ID 0x00 :%02x\n",data);
	regmap_write(chip->regmap, 0xfe, 0x8c);
	regmap_write(chip->regmap, 0xff, 0x00);

	ret = devm_regmap_add_irq_chip(&i2c->dev, chip->regmap,
				       i2c->irq, IRQF_TRIGGER_LOW|IRQF_ONESHOT, 0,
				       &bd71828_irq_chip, &irq_data);
	if (ret) {
		dev_err(&i2c->dev, "Failed to add IRQ chip\n");
		return ret;
	}

	dev_dbg(&i2c->dev, "Registered %d IRQs for chip\n",
		bd71828_irq_chip.num_irqs);

	/*ret = regmap_irq_get_virq(irq_data, BD71828_INT_PUSH);//BD71828_INT_SHORTPUSH;

	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to get the power-key IRQ\n");
		return ret;
	}

	button.irq = ret;*/

	if (button_irq) {
		ret = regmap_irq_get_virq(irq_data, button_irq);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get the power-key IRQ\n");
			return ret;
		}
		bd71828_powerkey_data.irq_push = button.irq = ret;

		ret = regmap_irq_get_virq(irq_data,BD71828_INT_SHORTPUSH);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get the power-key IRQ\n");
			return ret;
		}
		bd71828_powerkey_data.irq_short_push = ret;

		ret = regmap_irq_get_virq(irq_data, BD71828_INT_MIDPUSH);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get the power-key IRQ\n");
			return ret;
		}
		bd71828_powerkey_data.irq_mid_push = ret;

		ret = regmap_irq_get_virq(irq_data,BD71828_INT_LONGPUSH);
		if (ret < 0) {
			dev_err(&i2c->dev, "Failed to get the power-key IRQ\n");
			return ret;
		}
		bd71828_powerkey_data.irq_long_push = ret;
	}

	ret = devm_mfd_add_devices(&i2c->dev, PLATFORM_DEVID_AUTO,
				   bd71828_mfd_cells,
				   ARRAY_SIZE(bd71828_mfd_cells), NULL, 0,
				   regmap_irq_get_domain(irq_data));
	if (ret)
		dev_err(&i2c->dev, "Failed to create subdevices\n");


	if (of_device_is_system_power_controller(i2c->dev.of_node)) {
		bd71828_chip = chip;
		if (!pm_power_off)
			pm_power_off = bd71828_power_off;
		else
			dev_warn(&i2c->dev, "poweroff callback already assigned\n");
	}

	ret = device_create_file(&(i2c->dev), &dev_attr_reg_access);

	arm_pm_restart = bd71828_restart;

	return ret;
}

static const struct of_device_id bd71828_of_match[] = {
	{ .compatible = "rohm,bd71828", },
	{ },
};

static const struct i2c_device_id bd71828_dev_id[] = {
	{"bd71828", 0},
	{},
};

static struct i2c_driver bd71828_drv = {
	.driver = {
		.name = "rohm-bd71828",
		.of_match_table = bd71828_of_match,
	},
	.probe = bd71828_i2c_probe,
	.id_table = bd71828_dev_id,
};

module_i2c_driver(bd71828_drv);

MODULE_AUTHOR("Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>");
MODULE_DESCRIPTION("ROHM BD71828 Power Management IC driver");
MODULE_LICENSE("GPL");

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
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/pm_runtime.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/mutex.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/kthread.h>

#if defined(CONFIG_REGMAP)
#include <linux/regmap.h>
#endif

#ifdef CONFIG_RT_REGMAP
#include <mt-plat/rt-regmap.h>
#endif
#ifdef CONFIG_MTK_CHARGER_CLASS
#include <linux/power/mtk_charger_class.h>
#endif /* CONFIG_MTK_CHARGER_CLASS */
#ifdef CONFIG_CHARGER_MTK
#include <mt-plat/charger_type.h>
#endif/* CONFIG_CHARGER_MTK */

#include "g2230_charger.h"
#define G2230_DRV_VERSION	"1.0.0_MTK"

 /* ISET_DCIN<2:0> <--> DCIN Current limit */
static const u32 g2230_iset_dcin_values[] = {
	  150000,   450000,   850000,  1000000,
	 1500000,  2000000,  2500000,  3000000
};

static inline int g2230_i2c_read_byte(struct g2230_chip *chip, u8 reg, u8 *data)
{
	int ret = 0;

	int cureg = 0;

	ret = regmap_read(chip->regmap, reg, &cureg);
	if (ret < 0) {
		dev_dbg(chip->dev,
			 "<%s,%d>,read register[0x%02x] failed [%d]\n",
			 __func__, __LINE__, reg, ret);
		return ret;
	}
	*data = (cureg & 0xFF);

	return 0;
}

static inline int g2230_i2c_write_byte(struct g2230_chip *chip, u8 cmd, u8 data)
{
	int ret = 0;

	ret = regmap_write(chip->regmap, cmd, data);
	if (ret < 0)
		dev_dbg(chip->dev, "%s reg 0x%02X = 0x%02X fail(%d)\n",
			 __func__, cmd, data, ret);

	return ret;
}


#ifndef CONFIG_CHARGER_MTK
static int g2230_chip_i2c_write_byte(struct g2230_chip *chip, u8 reg, u8 data)
{
	int ret = 0;

	mutex_lock(&chip->io_lock);
	ret = g2230_i2c_write_byte(chip, reg, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}
#endif

static int g2230_chip_i2c_read_byte(struct g2230_chip *chip, u8 reg, u8 *data)
{
	int ret = 0;

	mutex_lock(&chip->io_lock);
	ret = g2230_i2c_read_byte(chip, reg, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static inline int __g2230_i2c_block_write(struct g2230_chip *chip, u8 reg,
					  u32 len, const u8 *data)
{
	int ret = 0;

	ret = regmap_bulk_write(chip->regmap, reg, data, len);

	return ret;
}

static int g2230_i2c_block_write(struct g2230_chip *chip, u8 reg, u32 len,
				 const u8 *data)
{
	int ret = 0;

	mutex_lock(&chip->io_lock);
	ret = __g2230_i2c_block_write(chip, reg, len, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static inline int __g2230_i2c_block_read(struct g2230_chip *chip, u8 reg,
					 u32 len, u8 *data)
{
	int ret = 0;

	ret = regmap_bulk_read(chip->regmap, reg, data, len);

	return ret;
}

static int g2230_i2c_block_read(struct g2230_chip *chip, u8 reg, u32 len,
				u8 *data)
{
	int ret = 0;

	mutex_lock(&chip->io_lock);
	ret = __g2230_i2c_block_read(chip, reg, len, data);
	mutex_unlock(&chip->io_lock);

	return ret;
}

static int g2230_update(struct g2230_chip *chip, unsigned int reg,
			unsigned int mask, unsigned int data)
{
	int rval = -1;

	rval = regmap_update_bits(chip->regmap, reg, mask, data);

	return rval;
}

/*
 * Return the index in 'tbl' of greatest value that is less than or equal to
 * 'val'.  The index range returned is 0 to 'tbl_size' - 1.  Assumes that
 * the values in 'tbl' are sorted from smallest to largest and 'tbl_size'
 * is less than 2^8.
 */
static u8 g2230_find_idx(const u32 tbl[], int tbl_size, int v)
{
	int i;

	for (i = 1; i < tbl_size; i++)
		if (v < tbl[i])
			break;

	return i - 1;
}

static int g2230_find_value(const u32 tbl[], int tbl_size, int v)
{
	v = (v >= tbl_size) ? (tbl_size - 1) : v;

	return tbl[v];
}

static u32 g2230_chip_find_closest_real_value(u32 min, u32 max, u32 step,
					      u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

void __attribute__((weak)) Charger_Detect_Init(void)
{
	pr_info("%s need usb porting!\n", __func__);
}

void __attribute__((weak)) Charger_Detect_Release(void)
{
	pr_info("%s need usb porting!\n", __func__);
}

static void g2230_set_usbsw_state(struct g2230_chip *chip, int state)
{
	dev_info(chip->dev, "[%s,%d],state = %d,bUsbDetect_Init=%d\n", __func__,
		 __LINE__, state, chip->bUsbDetect_Init);

	if (!chip->bUsbDetect_Init && (state == G2230_USBSW_CHG)) {
		Charger_Detect_Init();
		chip->bUsbDetect_Init = true;
	} else if (chip->bUsbDetect_Init) {
		Charger_Detect_Release();
		chip->bUsbDetect_Init = false;
	}
}

#ifndef CONFIG_CHARGER_MTK
static bool g2230_chip_is_Enchin_enable(struct g2230_chip *chip)
{
	int err = -1;
	bool isEnchin = false;
	u8 data = 0;

	if (!chip->online)
		return false;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A0, &data);
	if (err != 0) {
		dev_info(chip->dev, "i2c read ENCHIN failed:%d\n", err);
		return false;
	}

	if (((data >> A0_ENCHIN_SHIFT_NUM) & BIT0_MASK) == A0_ENCHIN_ENABLE)
		isEnchin = true;

	return isEnchin;
}

static bool g2230_get_ilimadj_en_status(struct g2230_chip *chip)
{
	int err = -1;
	bool bEn_ilimadj = false;
	u8 data = 0;

	/**EN_ILIMADJ**/
	err = g2230_i2c_read_byte(chip, G2230_CHIP_REG_A7, &data);
	if (err != 0) {
		dev_info(chip->dev, "g2230 read EN_ILIMADJ failed:%d\n", err);
		return -1;
	}
	// dev_info(chip->dev,"[%s,%d],read EN_ILIMADJ
	// ok:0x%02x,(data>0x02)=0x%02x\n", __func__,__LINE__,data,(data >>
	// 0x02));
	if ((data >> 0x02) & BIT0_MASK)
		bEn_ilimadj = true;

	return bEn_ilimadj;
}

// STANDARD_HOST
static int g2230_chip_set_standard_host_charger_cur(struct g2230_chip *chip)
{
	int err = -1;
	u8 data = 0;
	bool bEn_ilimadj = false;

	bEn_ilimadj = g2230_get_ilimadj_en_status(chip);
	// dev_info(chip->dev,"[%s,%d],bEn_ilimadj[%d]\n",
	// __func__,__LINE__,bEn_ilimadj);
	/**set ISET_DCIN--DCIN Current limit**/
	if (!bEn_ilimadj) {
		err = g2230_i2c_read_byte(chip, G2230_CHIP_REG_A1, &data);
		data = (data & 0xfc);
		err |= g2230_chip_i2c_write_byte(chip, G2230_CHIP_REG_A1, data);
		err |= g2230_update(chip, G2230_CHIP_REG_A1, G2230_A1_ISET_DCIN,
				    A1_ISET_DCIN_450mA);
		if (err != 0) {
			dev_info(chip->dev,
				 "g2230 update ISET_DCIN failed:%d\n", err);
			return -1;
		}
	}

	/** ISETA -set limited to battery **/
	err = g2230_update(chip, G2230_CHIP_REG_A3, G2230_A3_ISETA, 0x00);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update ISETA failed:%d\n", err);
		return -1;
	}

	return 0;
}

// CHARGING_HOST
static int g2230_chip_set_charging_host_charger_cur(struct g2230_chip *chip)
{
	int err = -1;
	u8 data = 0;
	bool bEn_ilimadj = false;

	/**ISET_DCIN:set limited from USB --DCIN Current limit**/
	bEn_ilimadj = g2230_get_ilimadj_en_status(chip);
	// dev_info(chip->dev,"[%s,%d],bEn_ilimadj[%d]\n",
	// __func__,__LINE__,bEn_ilimadj);
	if (!bEn_ilimadj) {
		err = g2230_i2c_read_byte(chip, G2230_CHIP_REG_A1, &data);
		data = (data & 0xfc);
		err |= g2230_chip_i2c_write_byte(chip, G2230_CHIP_REG_A1, data);
		err |= g2230_update(chip, G2230_CHIP_REG_A1, G2230_A1_ISET_DCIN,
				    A1_ISET_DCIN_1500mA);
		if (err != 0) {
			dev_info(chip->dev,
				 "g2230 update ISET_DCIN failed:%d\n", err);
			return -1;
		}
	}

	/** ISETA :set limited to battery **/
	err = g2230_update(chip, G2230_CHIP_REG_A3, G2230_A3_ISETA, 0x3c);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update ISETA failed:%d\n", err);
		return -1;
	}

	return 0;
}

static int g2230_chip_set_charging_cur(struct g2230_chip *chip)
{
	int err = -1;
	enum charger_type chg_type = CHARGER_UNKNOWN;
	unsigned int fail_count = 0;

reEnchincheck:
	/** first,get charger type **/
	chg_type = g2230_chip_get_charger_type(chip);

	if (chip->chg_type != chg_type) {
		chip->chg_type = chg_type;
		/** need reset the charging current **/
		chip->bChargingCurHadSet = false;
	}

	dev_info(
		chip->dev,
		"[%s,%d]: bChargingCurHadSet[%d],chip->chg_type[%d],chg_type[%d]\n",
		__func__, __LINE__, chip->bChargingCurHadSet, chip->chg_type,
		chg_type);

	if (chip->bChargingCurHadSet) {
		dev_info(chip->dev,
			 "charging current had setted,no need re-set again\n");
		return 0;
	}

	if (chip->chg_type == CHARGER_UNKNOWN) {
		dev_info(chip->dev, "[%s,%d], get charger type is error:%d\n",
			 __func__, __LINE__, chip->chg_type);
		return -1;
	}

	if (fail_count < 3) {
		fail_count++;
		if (!g2230_chip_is_Enchin_enable(chip)) {
			dev_info(chip->dev, "ENCHIN is disabled,try enable\n");
			err = g2230_chip_enable_charging(chip, true);
			goto reEnchincheck;
		}
	} else {
		dev_info(chip->dev, "Retry %d times,ENCHIN still disabled\n",
			 fail_count);
		fail_count = 0;
		return -1;
	}

	if ((chip->chg_type == STANDARD_HOST) ||
	    (chip->chg_type == NONSTANDARD_CHARGER)) {
		err = g2230_chip_set_standard_host_charger_cur(chip);
		if (err < 0) {
			dev_info(
				chip->dev,
				"g2230 set usb(STANDARD_HOST) charger current failed:%d\n",
				err);
			return -1;
		}
	} else if (chip->chg_type == CHARGING_HOST) {
		err = g2230_chip_set_charging_host_charger_cur(chip);
		if (err < 0) {
			dev_info(
				chip->dev,
				"g2230 set charging(CHARGING_HOST) charger current failed:%d\n",
				err);
			return -1;
		}
	} else if (chip->chg_type == STANDARD_CHARGER) {
		err = g2230_chip_set_charging_host_charger_cur(chip);
		if (err < 0) {
			dev_info(
				chip->dev,
				"g2230 set charging(STANDARD_CHARGER) charger current failed:%d\n",
				err);
			return -1;
		}
	}

	return 0;
}
#endif

static int g2230_chip_enable_charging(struct g2230_chip *chip, bool en)
{
	int err = 0;

	/** ENCHIN enable or disable***/
	if (en) {
		err = g2230_update(chip, G2230_CHIP_REG_A0, G2230_A0_ENCHIN,
				   A0_ENCHIN_ENABLE);
	} else {
		err = g2230_update(chip, G2230_CHIP_REG_A0, G2230_A0_ENCHIN,
				   A0_ENCHIN_DISABLE);
	}
	if (err != 0) {
		dev_info(chip->dev, "g2230 update ENCHIN failed:%d\n", err);
		return -1;
	}
	dev_info(chip->dev, "[%s,%d]: bChargingCurHadSet = %d\n",
			__func__, __LINE__,  chip->bChargingCurHadSet);
	if (!en) {
		if (chip->bChargingCurHadSet)
			chip->bChargingCurHadSet = false;
	}

	return err;
}

enum charger_type g2230_chip_get_charger_type(struct g2230_chip *chip)
{
	u8 data = 0;
	int err = -1;
	enum charger_type chg_type = CHARGER_UNKNOWN;

	/** CHGTYPIN **/
	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A16, &data);
	if (err != 0) {
		dev_info(chip->dev, "i2c read CHGTYPIN failed:%d\n", err);
		return CHARGER_UNKNOWN;
	}

	/**CHGTYPIN had read completed,now we need release the usb  07-30**/
	// g2230_set_usbsw_state(chip,G2230_USBSW_USB);

	/** Stand Downstream Port (SDP)  **/
	if (((data >> A16_CHGTYPIN_SHIFT_NUM) & A16_CHGTYPIN_SHIFT_MASK) ==
	    A16_CHGTYPIN_SDP)
		chg_type = STANDARD_HOST;
	/** Charging Downstream Port (CDP)  **/
	else if (((data >> A16_CHGTYPIN_SHIFT_NUM) & A16_CHGTYPIN_SHIFT_MASK) ==
		 A16_CHGTYPIN_CDP)
		chg_type = CHARGING_HOST;
	/** Dedicated Charging Port (DCP)  **/
	else if (((data >> A16_CHGTYPIN_SHIFT_NUM) & A16_CHGTYPIN_SHIFT_MASK) ==
		 A16_CHGTYPIN_DCP)
		chg_type = STANDARD_CHARGER;

	return chg_type;
}

static int g2230_chip_clear_interrupt(struct g2230_chip *chip)
{
	int err = -1;

	/** A10 --B7 INT --set High to clear interrupt **/
	err = g2230_update(chip, G2230_CHIP_REG_A10_INT1_STATUS, G2230_A10_INT,
			   A10_INT_HIGH);
	if (err != 0) {
		dev_info(chip->dev,
			 "g2230 update clear interrupt reg failed:%d\n", err);
		return err;
	}

	return err;
}

static int g2230_inform_psy_changed(struct g2230_chip *chip)
{
	int err = 0;
	union power_supply_propval propval;
	bool vbus_good = atomic_read(&chip->vbus_good);
	struct power_supply *chg_psy;
	enum charger_type chg_type = CHARGER_UNKNOWN;

	dev_info(chip->dev, "%s vbus_good = %d, type = %d\n", __func__,
		 vbus_good, chip->chg_type);

	/* get power supply */
	chg_psy = power_supply_get_by_name("charger");
	if (!chg_psy) {
		dev_info(chip->dev, "%s: get charger psy failed\n", __func__);
		return -EINVAL;
	}

	/* inform chg det power supply */
	propval.intval = vbus_good;
	err = power_supply_set_property(chg_psy, POWER_SUPPLY_PROP_ONLINE,
					&propval);
	if (err < 0)
		dev_info(chip->dev, "%s psy online fail(%d)\n", __func__, err);

	chg_type = g2230_chip_get_charger_type(chip);

	if (vbus_good) {
		if (chg_type != CHARGER_UNKNOWN)
			propval.intval = chg_type;
		else
			propval.intval = CHARGER_UNKNOWN;
	} else {
		propval.intval = CHARGER_UNKNOWN;
	}
	err = power_supply_set_property(chg_psy, POWER_SUPPLY_PROP_CHARGE_TYPE,
					&propval);
	if (err < 0)
		dev_info(chip->dev, "%s psy type fail(%d)\n", __func__, err);

	power_supply_put(chg_psy);

	return err;
}

#ifndef CONFIG_TCPC_CLASS
static bool g2230_is_vbusgood(struct g2230_chip *chip)
{
	int err = 0;
	bool vbus_good = false;
	u8 buf = 0;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A10_INT1_STATUS,
				       &buf);
	if (err != 0) {
		dev_info(chip->dev, "i2c read DCDET failed:%d\n", err);
		return false;
	}

	/** DCDET is valid **/
	vbus_good = ((buf >> 0x00) & BIT0_MASK);

	// dev_info(chip->dev, "%s vbus_good = %d\n", __func__, vbus_good);

	return vbus_good;
}

static int __g2230_dcin_pg_work(struct g2230_chip *chip)
{
	bool vbusgood = false;
	int err = -1;

	mutex_lock(&chip->dcdet_lock);
	vbusgood = g2230_is_vbusgood(chip);
	atomic_set(&chip->vbus_good, vbusgood);
	if (vbusgood) {
		g2230_set_usbsw_state(chip, G2230_USBSW_CHG);
		err = g2230_chip_enable_chgdet(chip, true);
	} else {
		g2230_set_usbsw_state(chip, G2230_USBSW_USB);
		err = g2230_chip_enable_chgdet(chip, false);
	}
	g2230_inform_psy_changed(chip);
	mutex_unlock(&chip->dcdet_lock);

	return 0;
}
#endif

// g2230_##_name##_irq_handler
static int g2230_dcin_pg_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n",  __func__, __LINE__);
#ifndef CONFIG_TCPC_CLASS
	__g2230_dcin_pg_work(chip);
#endif
	return 0;
}

static int g2230_batlow_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_nobat_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_eoc_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_chgdone_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_charger_safe_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_bst_fault_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_chgrun_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_inlimit_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_thr_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_ts_meter_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
	return 0;
}

static int g2230_dcdet_irq_handler(struct g2230_chip *chip)
{
	dev_info(chip->dev, "[%s,%d]\n", __func__, __LINE__);
#if 0 /** ndef CONFIG_TCPC_CLASS **/
	bool vbusgood = false;

	mutex_lock(&chip->dcdet_lock);
	vbusgood = g2230_is_vbusgood(chip);
	atomic_set(&chip->vbus_good, vbusgood);
	if (vbusgood) {
		g2230_chip_enable_chgdet(chip, true);
		g2230_set_usbsw_state(chip, G2230_USBSW_CHG);
	} else {
		g2230_set_usbsw_state(chip, G2230_USBSW_USB);
		g2230_chip_enable_chgdet(chip, false);
	}
	g2230_inform_psy_changed(chip);
	mutex_unlock(&chip->dcdet_lock);
#endif

	return 0;
}

/* evt[4]= { a10, a11 ,a12,a13} */
/* 0000 0000 ==> 0000 0000==>00 00 0000 */
static const struct g2230_irq_mapping_table g2230_irq_mapping_tbl[] = {
	/** G2230_INTERRUPT_DCDET **/
	G2230_IRQ_MAPPING(dcdet, 0),
	/**G2230_INTERRUPT_TS_METER **/
	G2230_IRQ_MAPPING(ts_meter, 3),
	/**G2230_INTERRUPT_THR**/
	G2230_IRQ_MAPPING(thr, 4),
	/** G2230_INTERRUPT_INLIMIT**/
	G2230_IRQ_MAPPING(inlimit, 5),
	/** G2230_INTERRUPT_CHGRUN **/
	G2230_IRQ_MAPPING(chgrun, 6),
	/** G2230_INTERRUPT_BST_FAULT **/
	G2230_IRQ_MAPPING(bst_fault, 17),
	/** G2230_INTERRUPT_SAFE **/
	G2230_IRQ_MAPPING(charger_safe, 18),
	/** G2230_INTERRUPT_CHGDONE **/
	G2230_IRQ_MAPPING(chgdone, 19),
	/** G2230_INTERRUPT_EOC **/
	G2230_IRQ_MAPPING(eoc, 20),
	/** G2230_INTERRUPT_NO_BAT **/
	G2230_IRQ_MAPPING(nobat, 21),
	/** G2230_INTERRUPT_BATLOW **/
	G2230_IRQ_MAPPING(batlow, 22),
	/** G2230_INTERRUPT_DCIN_PG **/
	G2230_IRQ_MAPPING(dcin_pg, 23),
};

static int g2230_get_interrupt_status(struct g2230_chip *chip, u8 *buf)
{
	int err = -1;
	u8 mask[G2230_IRQIDX_MAX] = {0};
	int icount = 0;
	int jcount = 0;
	int inum = 0;
	bool brealValue = false;

	do {
		/** 1.first read **/
		err = g2230_i2c_block_read(chip, G2230_CHIP_REG_A10_INT1_STATUS,
					   G2230_IRQIDX_MAX, buf);
		if (err < 0) {
			dev_info(chip->dev, "%s read buf fail(%d)\n", __func__,
				 err);
			return -1;
		}
		/** 2. sleep 200,then twice read **/
		msleep(200);
		err = g2230_i2c_block_read(chip, G2230_CHIP_REG_A10_INT1_STATUS,
					   G2230_IRQIDX_MAX, mask);
		if (err < 0) {
			dev_info(chip->dev, "%s read mask fail(%d)\n", __func__,
				 err);
			return -1;
		}
		/** compare twice read value **/
		for (jcount = 0; jcount < G2230_IRQIDX_MAX; jcount++) {
			if (buf[jcount] == mask[jcount])
				inum++;
			if (inum == G2230_IRQIDX_MAX)
				brealValue = true;
		}

		/** twice read value is the same,mean value is right **/
		if (brealValue) {
			/** DCDET is valid,so evt[2] cannot equal 0x00 **/
			if ((buf[0] != 0x00) && ((buf[0] >> 0x00) & 0x1)) {
				if (buf[2] != 0x00)
					break;
			}
			/** DCDET isnot valid,we donot need to check evt[2] **/
			else if ((buf[0] != 0x00) &&
				 (!((buf[0] >> 0x00) & 0x1)))
				break;
			/** evt[0] is 0x00 **/
			else if (buf[0] == 0x00)
				break;
		}
		/** reassignment inum for next loop and record the loop num **/
		inum = 0;
		icount++;
		if (icount > 5)
			break;
		msleep(200);
	} while (1);
	if (icount >= 5) {
		dev_info(chip->dev,
			 "%s read evt fail(%d) times get reg value is wrong\n",
			 __func__, icount);
		return -1;
	}

	return 0;
}

static int g2230_irq_mapping_table_work(struct g2230_chip *chip)
{
	int err = -1;
	int i = -1;
	int irqnum = 0, irqbit = 0;
	u8 evt[G2230_IRQIDX_MAX] = {0};
	bool bDcdetValid = false;

	// pm_stay_awake(chip->dev);

	err = g2230_get_interrupt_status(chip, evt);
	if (err < 0) {
		dev_info(chip->dev, "get interrupt status failed:%d\n", err);
		g2230_chip_clear_interrupt(chip);
		return -1;
	}

	/**read over,need clear the interrupt **/
	err = g2230_chip_clear_interrupt(chip);
	if (err != 0) {
		dev_info(chip->dev, "g2230 clear interrupt failed:%d\n", err);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(g2230_irq_mapping_tbl); i++) {
		irqnum = g2230_irq_mapping_tbl[i].num / 8;
		if (irqnum >= G2230_IRQIDX_MAX)
			continue;
		irqbit = g2230_irq_mapping_tbl[i].num % 8;

		if (evt[irqnum] & (1 << irqbit)) {
			if (!chip->bDcdetDisConnect) {
				if (strcmp("dcin_pg",
					   g2230_irq_mapping_tbl[i].name) ==
				    0) {
					chip->bDcdetDisConnect = true;
					chip->iDecdetNum = i;
					chip->iDecdetIRQnum = irqnum;
					chip->iDecdetIRQbit = irqbit;
				}
			}
			g2230_irq_mapping_tbl[i].hdlr(chip);
		}
	}

	/** usb/dcdc had removed,we need process the interrupt again **/
	bDcdetValid = (evt[chip->iDecdetIRQnum] & (1 << chip->iDecdetIRQbit));
	dev_info(chip->dev, "[%s,%d]b DcdetValid=[%d],\n",
					__func__, __LINE__, bDcdetValid);
	if ((chip->bDcdetDisConnect) && (!bDcdetValid)) {
		g2230_irq_mapping_tbl[chip->iDecdetNum].hdlr(chip);
		chip->bDcdetDisConnect = false;
		chip->iDecdetNum = 0;
		chip->iDecdetIRQnum = 0;
		chip->iDecdetIRQbit = 0;
	}

	return 0;
}

static void g2230_interrupt_work(struct work_struct *work)
{
	struct g2230_chip *chip = container_of(work, struct g2230_chip,
					       g2230_interrupt_delay_work.work);

	g2230_irq_mapping_table_work(chip);

	enable_irq(chip->chg_irq);
}

static irqreturn_t g2230_interrupt(int irq, void *dev_id)
{
	struct g2230_chip *chip = dev_id;

	disable_irq_nosync(chip->chg_irq);

	if (!chip) {
		pr_info("%s: Invalid chip\n", __func__);
		return IRQ_HANDLED;
	}

	schedule_delayed_work(&chip->g2230_interrupt_delay_work,
			      msecs_to_jiffies(100));

	return IRQ_HANDLED;
}

static int g2230_threaded_init(struct g2230_chip *chip)
{
	int err = 0;
	unsigned long irqflags = 0;

	if (gpio_is_valid(chip->chg_int_gpio))
		chip->chg_irq = gpio_to_irq(chip->chg_int_gpio);
	else
		chip->chg_irq = -1;

	if (chip->chg_irq < 0) {
		dev_info(chip->dev, "irq value is error:%d\n", chip->chg_irq);
		return -1;
	}

	irqflags = IRQF_ONESHOT | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING |
		   IRQF_NO_SUSPEND | IRQF_EARLY_RESUME;

	err = devm_request_threaded_irq(chip->dev, chip->chg_irq, NULL,
					g2230_interrupt, irqflags,
					chip->dev->driver->name, chip);
	if (err) {
		dev_info(chip->dev, "request irq failed\n");
		return err;
	}

	device_init_wakeup(chip->dev, true);
	disable_irq(chip->chg_irq);
	return err;
}

static inline int __g2230_get_aicr(struct g2230_chip *chip, u32 *uA)
{
	int ret = 0;
	u8 data = 0;

	ret = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A1, &data);
	data = data & G2230_A1_ISET_DCIN;
	*uA = g2230_find_value(g2230_iset_dcin_values,
				ARRAY_SIZE(g2230_iset_dcin_values), data);
	return ret;
}

static inline int __g2230_set_aicr(struct g2230_chip *chip, u32 uA)
{
	u8 reg_data = 0;

	dev_dbg(chip->dev, "%s: ichg = %u\n", __func__, uA);
	reg_data = g2230_find_idx(g2230_iset_dcin_values,
				ARRAY_SIZE(g2230_iset_dcin_values), uA);

	return g2230_update(chip, G2230_CHIP_REG_A1,
					G2230_A1_ISET_DCIN, reg_data);
}

static inline int __g2230_set_ichg(struct g2230_chip *chip, u32 uA)
{
	u8 reg_data = 0;

	dev_dbg(chip->dev, "%s: ichg = %u\n", __func__, uA);
	if (uA >= G2230_ISETA_ICHG_MIN) {
		if (uA > G2230_ISETA_ICHG_MAX)
			uA = G2230_ISETA_ICHG_MAX;
		reg_data = (uA - G2230_ISETA_ICHG_MIN) / G2230_ISETA_ICHG_STEP;
	}

	return g2230_update(chip, G2230_CHIP_REG_A3, G2230_A3_ISETA,
					reg_data << G2230_ISETA_SHIFT_ICHG);
}

static int __g2230_get_ichg(struct g2230_chip *chip, u32 *ichg)
{
	int err = 0;
	u8 data = 0;
	u8 reg_ichg = 0;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A3, &data);
	if (err != 0) {
		dev_info(chip->dev, "g2230 read ISETA failed:%d\n", err);
		return -1;
	}

	reg_ichg = (data & G2230_ISETA_MASK_ICHG) >> G2230_ISETA_SHIFT_ICHG;

	*ichg = g2230_chip_find_closest_real_value(
		G2230_ISETA_ICHG_MIN, G2230_ISETA_ICHG_MAX,
		G2230_ISETA_ICHG_STEP, reg_ichg);

	return 0;
}

static inline int __g2230_set_safetimer_enable(struct g2230_chip *chip, bool en)
{
	int ret;

	dev_dbg(chip->dev, "%s: en = %u\n", __func__, en);
	if (en)
		ret = g2230_update(chip, G2230_CHIP_REG_A5, G2230_A5_EN_TIMER,
							A5_EN_TIMER_ENABLE);
	else
		ret = g2230_update(chip, G2230_CHIP_REG_A5, G2230_A5_EN_TIMER,
							A5_EN_TIMER_DISABLE);
	return ret;
}

static inline int __g2230_set_otg_enable(struct g2230_chip *chip, bool en)
{
	int ret;

	dev_dbg(chip->dev, "%s: en = %u\n", __func__, en);
	if (en)
		ret = g2230_update(chip, G2230_CHIP_REG_A0, G2230_A0_ENOTGI2C,
						A0_ENOTGI2C_OTG_ENABLE);
	else
		ret = g2230_update(chip, G2230_CHIP_REG_A0, G2230_A0_ENOTGI2C,
						A0_ENOTGI2C_OTG_DISENABLE);
	return ret;
}

static inline int __g2230_set_cv(struct g2230_chip *chip, u32 uV)
{
	u8 reg_data = 0;

	dev_dbg(chip->dev, "%s: cv = %u\n", __func__, uV);
	if (uV >= A4_VSETA_MIN) {
		reg_data = (uV - A4_VSETA_MIN) / A4_VSETA_STEP;
		if (reg_data > G2230_A4_VSETA_MAX)
			reg_data = G2230_A4_VSETA_MAX;
	}

	return g2230_update(chip, G2230_CHIP_REG_A4, G2230_A4_VSETA,
					reg_data << G2230_A4_VSETA_SHFT);
}

static inline int __g2230_get_cv(struct g2230_chip *chip, u32 *uV)
{
	u8 ret = 0;
	u8 data = 0;

	ret = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A4, &data);
	if (ret < 0)
		return ret;
	data = (data & G2230_A4_VSETA) >> G2230_A4_VSETA_SHFT;
	if (data > G2230_A4_VSETA_MAX)
		data = G2230_A4_VSETA_MAX;

	*uV = A4_VSETA_MIN + A4_VSETA_STEP * data;
	return ret;
}

static inline int __g2230_dump_registers(struct g2230_chip *chip)
{
	u8 regValue = 0;
	int ret = -1;
	int icount = 0;
	int reglist[] = {
		G2230_CHIP_REG_A0,
		G2230_CHIP_REG_A1,
		G2230_CHIP_REG_A2,
		G2230_CHIP_REG_A3,
		G2230_CHIP_REG_A4,
		G2230_CHIP_REG_A5,
		G2230_CHIP_REG_A6,
		G2230_CHIP_REG_A7,
		G2230_CHIP_REG_A8,
		G2230_CHIP_REG_A9,
		G2230_CHIP_REG_A10_INT1_STATUS,
		G2230_CHIP_REG_A11_INT1_MASK,
		G2230_CHIP_REG_A12_INT2_STATUS,
		G2230_CHIP_REG_A13_INT2_MASK,
		G2230_CHIP_REG_A14,
		G2230_CHIP_REG_A15,
		G2230_CHIP_REG_A16,
		G2230_CHIP_REG_A17_VER,
	};
	int regNum = ARRAY_SIZE(reglist);
	struct power_supply *batt_psy;
	union power_supply_propval batt_val;

	batt_psy = power_supply_get_by_name("rt-fuelgauge");
	if (!batt_psy)
		return -ENODEV;

	/* g2230_kick_wdt(chip); */

	for (icount = 0; icount < regNum; icount++) {
		ret = g2230_chip_i2c_read_byte(chip, reglist[icount],
					       &regValue);
		if (ret != 0) {
			dev_info(chip->dev,
				 "<%s,%d>,read reg:0x%02x failed:%d\n",
				 __func__, __LINE__, reglist[icount], regValue);
			continue;
		}
		if (icount == (regNum - 1))
			regValue = (regValue & 0x3f);

		dev_info(chip->dev, "%s reg0x%02X = 0x%02X\n", __func__,
						reglist[icount], regValue);
	}

	ret = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A10_INT1_STATUS,
								   &regValue);
	regValue = regValue & G2230_A10_TS_METER;

	if (regValue == A10_TS_METER_COLD)
		batt_val.intval = -10;
	else if (regValue == A10_TS_METER_COOL)
		batt_val.intval = 5;
	else if (regValue == A10_TS_METER_WARM)
		batt_val.intval = 50;
	else if (regValue == A10_TS_METER_HOT)
		batt_val.intval = 65;
	else
		batt_val.intval = 25;
	ret = power_supply_set_property(batt_psy, POWER_SUPPLY_PROP_TEMP,
					&batt_val);

	return 0;
}

static int g2230_get_tchg(struct charger_device *chg_dev, int *tchg_min,
			  int *tchg_max)
{
	return 0;
}

static int g2230_enable_otg(struct charger_device *chg_dev, bool en)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "[%s,%d],en->%d\n", __func__, __LINE__, en);
	return __g2230_set_otg_enable(chip, en);
}

static int g2230_enable_chg_type_det(struct charger_device *chg_dev, bool en)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);
	int err = -1;
	bool bEnableChgDet = false;
	enum charger_type chg_type = CHARGER_UNKNOWN;
	u8 data;
	bool vbus = false;

	vbus = g2230_is_vbusgood(chip);
	if (vbus) {
		dev_info(chip->dev,
			 "vbus is invalid,donot need get the charging type\n");
		return -1;
	}

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A7, &data);
	if (err != 0) {
		dev_info(chip->dev, "i2c read CHG_2DET & CHG_1DET failed:%d\n",
			 err);
		return -1;
	}

	/**CHG_2DET & CHG_1DET all are enabled **/
	if (((data >> A7_CHG_1DET_SHIFT_NUM) & BIT0_MASK) &&
	    ((data >> A7_CHG_2DET_SHIFT_NUM) & BIT0_MASK))
		bEnableChgDet = true;

	if (en && bEnableChgDet)
		chg_type = g2230_chip_get_charger_type(chip);

	return 0;
}

static int g2230_enable_safety_timer(struct charger_device *chg_dev, bool en)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "[%s,%d],en->%d\n", __func__, __LINE__, en);
	return __g2230_set_safetimer_enable(chip, en);
}

static int g2230_get_min_aicr(struct charger_device *chg_dev, u32 *uA)
{
	*uA = 100000;

	return 0;
}

static int g2230_get_min_ichg(struct charger_device *chg_dev, u32 *uA)
{
	*uA = 500000;

	return 0;
}

static int g2230_set_ieoc(struct charger_device *chg_dev, u32 uA)
{
	return 0;
}

enum g2230_chg_status
g2230_get_charging_cur_status(struct g2230_chip *chip)
{
	enum g2230_chg_status chg_stat = G2230_CHG_STATUS_UNKNOWN;
	int err = -1;
	u8 chgdone = 0;
	u8 data = 0;
	bool vbus = false;

	dev_info(chip->dev, "[%s,%d] online=%d, isCharging=%d\n",
			__func__, __LINE__, chip->online, chip->isCharging);
	if (!chip->online) {
		dev_info(chip->dev, "online is falsed\n");
		return G2230_CHG_STATUS_UNKNOWN;
	}

	if (!chip->isCharging) {
		dev_info(chip->dev, "charging is not enabled\n");
		return G2230_CHG_STATUS_UNKNOWN;
	}

	vbus = g2230_is_vbusgood(chip);
	if (!vbus) {
		dev_info(chip->dev, "DCDET is invalid\n");
		return G2230_CHG_STATUS_UNKNOWN;
	}

	/**
	 * CHGDONE show the charger status. CHGDONE is reset by recharging.
	 * CHGDONE Charger Status
	 * 0 During Charging or No Battery
	 * 1 Charging done
	 **/
	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A12_INT2_STATUS,
				       &data);
	if (err != 0) {
		dev_info(chip->dev, "i2c read CHGDONE failed:%d\n", err);
		return G2230_CHG_STATUS_UNKNOWN;
	}

	chgdone = ((data << A12_CHGDONE_SHIFT_NUM) & BIT0_MASK);
	if (chgdone == 0x01) {
		chg_stat = G2230_CHG_STATUS_DONE;
	} else {
		// data = 0;
		// err =
		// g2230_chip_i2c_read_byte(chip,
		//			G2230_CHIP_REG_A10_INT1_STATUS, &data);
		// if (err != 0) {
		//	dev_info(chip->dev,"i2c read CHGRUN failed:%d\n",err);
		//	return G2230_CHG_STATUS_UNKNOWN;
		//}
		// if ((data >> 0x06) & 0x1)
		chg_stat = G2230_CHG_STATUS_PROGRESS;
	}

	return chg_stat;
}

static int g2230_is_charging_done(struct charger_device *chg_dev, bool *done)
{
	enum g2230_chg_status chg_stat = G2230_CHG_STATUS_UNKNOWN;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	chg_stat = g2230_get_charging_cur_status(chip);

	/* Return is charging done or not */
	switch (chg_stat) {
	case G2230_CHG_STATUS_PROGRESS:
		*done = false;
		break;
	case G2230_CHG_STATUS_DONE:
		*done = true;
		break;
	default:
		*done = false;
		break;
	}

	return 0;
}

static int g2230_set_mivr(struct charger_device *chg_dev, u32 uV)
{
	return 0;
}

static int g2230_kick_wdt(struct charger_device *chg_dev)
{
	int err = 0;
	enum g2230_chg_status chg_status = G2230_CHG_STATUS_UNKNOWN;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "[%s,%d],chg_status[%d]\n", __func__, __LINE__,
		 chg_status);

	return err;
}

static int g2230_get_cv(struct charger_device *chg_dev, u32 *cv)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	return __g2230_get_cv(chip, cv);
}

static int g2230_set_cv(struct charger_device *chg_dev, u32 uV)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "%s: cv = %u\n", __func__, uV);
	return __g2230_set_cv(chip, uV);
}

static int g2230_get_aicr(struct charger_device *chg_dev, u32 *uA)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	return __g2230_get_aicr(chip, uA);
}

static int g2230_set_aicr(struct charger_device *chg_dev, u32 uA)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "%s: aicr = %u\n", __func__, uA);
	return __g2230_set_aicr(chip, uA);
}

static int g2230_get_ichg(struct charger_device *chg_dev, u32 *uA)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);
	int err = 0;

	err = __g2230_get_ichg(chip, uA);
	if (err < 0) {
		dev_info(chip->dev, "get charging current failed:%d\n", err);
		return -1;
	}

	return 0;
}

static int g2230_set_ichg(struct charger_device *chg_dev, u32 uA)
{
	int err = 0;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);
	bool vbus = false;

#ifndef CONFIG_TCPC_CLASS
	/** connect USB/DCDC with power on,when power on to root(main
	 *interface),disconnect
	 * the USB/DCDC,device cannot receive the interrupt,so here check
	 * the vbus every-times
	 ***/
	vbus = g2230_is_vbusgood(chip);
	// dev_info(chip->dev,"[%s,%d],vbus=%d\n",__func__,__LINE__,vbus);
	if (!vbus) {
		atomic_set(&chip->vbus_good, 0);
		g2230_inform_psy_changed(chip);
	}
#endif

#ifdef CONFIG_CHARGER_MTK
	__g2230_set_ichg(chip, uA);
#else
	mutex_lock(&chip->ichg_lock);
	err = g2230_chip_set_charging_cur(chip);
	mutex_unlock(&chip->ichg_lock);
#endif
	if (err < 0) {
		dev_info(chip->dev, "set charging cur failed:%d\n", err);
		return err;
	}
	chip->bChargingCurHadSet = true;
	chip->isCharging = true;

	return err;
}

static int g2230_enable_charging(struct charger_device *chg_dev, bool en)
{
	int err = 0;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "[%s,%d], chip->online=%d\n",
				__func__, __LINE__, chip->online);
	if (chip->online) {
		err = g2230_chip_enable_charging(chip, chip->online);
		if (err < 0) {
			dev_info(chip->dev, "enable charging failed:%d\n", err);
			return err;
		}
	}

	return 0;
}

static int g2230_plug_in(struct charger_device *chg_dev)
{
	int err = 0;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);
	u8 data = 0;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A10_INT1_STATUS,
				       &data);
	if (err != 0) {
		dev_info(chip->dev, "read DCDET failed:%d", err);
		return err;
	}

	if ((data >> 0x00) & BIT0_MASK)
		chip->online = true;

	dev_info(chip->dev, "[%s,%d],data=0x%02x,online=0x%02x\n", __func__,
		 __LINE__, data, chip->online);

	return err;
}

static int g2230_plug_out(struct charger_device *chg_dev)
{
	int err = 0;
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);
	u8 data = 0;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A10_INT1_STATUS,
				       &data);
	if (err != 0) {
		dev_info(chip->dev, "read DCDET failed:%d", err);
		return err;
	}

	if (((data >> 0x00) & BIT0_MASK) == 0x00)
		chip->online = false;

	dev_info(chip->dev, "[%s,%d],data=0x%02x,online=0x%02x\n", __func__,
		 __LINE__, data, chip->online);
	g2230_set_usbsw_state(chip, G2230_USBSW_USB);

	/**Disable charger **/
	err = g2230_chip_enable_charging(chip, chip->online);
	if (err < 0) {
		dev_info(chip->dev, "Disable charging failed:%d\n", err);
		return err;
	}

	err = g2230_chip_enable_chgdet(chip, false);
	if (err < 0) {
		dev_info(chip->dev, "Disable CHG_1DET & CHG_2DET failed:%d\n",
			 err);
		return err;
	}
	chip->bChargingCurHadSet = false;
	chip->isCharging = false;

	return err;
}

static int g2230_do_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	switch (event) {
	case EVENT_EOC:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
		break;
	case EVENT_RECHARGE:
		charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
		break;
	default:
		break;
	}
	return 0;
}

static int g2230_dump_registers(struct charger_device *chg_dev)
{
	struct g2230_chip *chip = dev_get_drvdata(&chg_dev->dev);

	return __g2230_dump_registers(chip);
}

static struct charger_ops g2230_chg_ops = {
	/* Normal charging */
	.plug_out = g2230_plug_out,
	.plug_in = g2230_plug_in,
	.enable = g2230_enable_charging,
	.get_charging_current = g2230_get_ichg,
	.set_charging_current = g2230_set_ichg,
	.get_input_current = g2230_get_aicr,
	.set_input_current = g2230_set_aicr,
	.get_constant_voltage = g2230_get_cv,
	.set_constant_voltage = g2230_set_cv,
	.kick_wdt = g2230_kick_wdt,
	.set_mivr = g2230_set_mivr,
	.is_charging_done = g2230_is_charging_done,
	.set_eoc_current = g2230_set_ieoc,
	/* .reset_eoc_state = g2230_reset_eoc_state, */
	.get_min_charging_current = g2230_get_min_ichg,
	.get_min_input_current = g2230_get_min_aicr,

	/* Safety timer */
	.enable_safety_timer = g2230_enable_safety_timer,
	/* .is_safety_timer_enabled = g2230_is_safety_timer_enable, */

	/* Power path */
	/* .enable_powerpath = g2230_enable_power_path, */
	/* .is_powerpath_enabled = g2230_is_power_path_enable, */

	/* Charger type detection */
	.enable_chg_type_det = g2230_enable_chg_type_det,

	/* OTG */
	.enable_otg = g2230_enable_otg,

	/* ADC */
	.get_tchg_adc = g2230_get_tchg,

	/* Event */
	.event = g2230_do_event,

	.dump_registers = g2230_dump_registers,
};

static int g2230_charger_device_register(struct g2230_chip *chip)
{
	int err = -1;

	/* Register charger device */
	chip->chg_dev =
		charger_device_register(chip->chg_dev_name, chip->dev, chip,
					&g2230_chg_ops, &chip->chg_props);
	if (IS_ERR_OR_NULL(chip->chg_dev)) {
		err = PTR_ERR(chip->chg_dev);
		dev_info(chip->dev,
			 "charger device register for g2230 failed:%d\n", err);
		return err;
	}

	return 0;
}

static int g2230_parse_dt(struct device *dev, struct g2230_chip *chip)
{
	struct device_node *np = dev->of_node;
	int err = -1;

	chip->chg_ce_gpio = of_get_named_gpio_flags(np, "chg_ce", 0, NULL);
	if (!gpio_is_valid(chip->chg_ce_gpio))
		return -1;

	chip->chg_int_gpio = of_get_named_gpio_flags(np, "chg_int", 0, NULL);
	if (!gpio_is_valid(chip->chg_int_gpio))
		return -1;

	chip->chg_dev_name = "primary_chg";
	err = of_property_read_string(np, "charger_name", &chip->chg_dev_name);
	if (err && (err != -EINVAL))
		dev_info(dev, "Unable to read name\n");

	/* Alias name is in charger properties but not in desc */
	if (of_property_read_string(np, "chg_alias_name",
				    &(chip->chg_props.alias_name)) < 0) {
		dev_info(chip->dev, "%s: no chg alias name\n", __func__);
		chip->chg_props.alias_name = "g2230_chg";
	}

	dev_info(chip->dev, "[%s,%d], ce_gpio=%d, chg_int_gpio=%d, name=%s\n",
				__func__, __LINE__, chip->chg_ce_gpio,
				chip->chg_int_gpio, chip->chg_dev_name);
	return 0;
}

static int g2230_chip_poweron_check(struct g2230_chip *chip)
{
	bool bDcDetValid = false;

#ifndef CONFIG_TCPC_CLASS
	bDcDetValid = g2230_is_vbusgood(chip);
	dev_info(chip->dev, "[%s,%d],bDcDetValid=%d\n", __func__, __LINE__,
		 bDcDetValid);
	if (bDcDetValid)
		atomic_set(&chip->vbus_good, 1);
#endif
	g2230_inform_psy_changed(chip);

	return 0;
}

static int g2230_chip_enable_chgdet(struct g2230_chip *chip, bool en)
{
	int err = -1;

	/** CHG_2DET & CHG_1DET control
	 * EN_ILIMADJ--donot use auto mode.
	 **/
	dev_info(chip->dev, "[%s,%d],en==%d\n", __func__, __LINE__, en);
	if (en)
		err = g2230_update(chip, G2230_CHIP_REG_A7,
				   G2230_A7_CHG_1DET_2DET,
				   A7_CHG_1DET_2DET_ENABLE);
	else
		err = g2230_update(chip, G2230_CHIP_REG_A7,
				   G2230_A7_CHG_1DET_2DET,
				   A7_CHG_1DET_2DET_DISABLE);
	if (err != 0) {
		dev_info(chip->dev,
			 "g2230 update CHG_2DET & CHG_1DET failed:%d\n", err);
		return -1;
	}

	return 0;
}

static int g2230_chip_init(struct g2230_chip *chip)
{
	int err = -1;
	// u8 data = 0;

#if 0
	/* enable ENSHIP but after test:CAN NOT enable shipping mode */
	/* if enable, we need connect usb for booton */
	err = g2230_update(chip, G2230_CHIP_REG_A0, G2230_A0_ENSHIP,
							A0_ENSHIP_ENABLE);
	err = g2230_i2c_read_byte(chip, G2230_CHIP_REG_A0, &data);
	dev_info(chip->dev, "[%s,%d],data =0x%02x\n", __func__, __LINE__, data);
	data = (data | 0x08);
	dev_info(chip->dev, "[%s,%d],data =0x%02x\n", __func__, __LINE__, data);
	err |= g2230_i2c_write_byte(chip, G2230_CHIP_REG_A0, data);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update ENSHIP failed:%d\n", err);
		return -1;
	}
#endif

	/** set VBLOW to 3.4v**/
	err = g2230_update(chip, G2230_CHIP_REG_A2, G2230_A2_VBLOW,
			   A2_VBLOW_34V);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update VBLOW failed:%d\n", err);
		return -1;
	}

	/**set *VSYS_MIN to 3.4v**/
	err = g2230_update(chip, G2230_CHIP_REG_A2, G2230_A2_VSYSMIN,
			   A2_VSYSMIN_34V);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update VSYS_MIN failed:%d\n", err);
		return -1;
	}

	/** set VSETA to 4.35v --battery is full **/
	err = g2230_update(chip, G2230_CHIP_REG_A4, G2230_A4_VSETA,
			   A4_VSETA_4_35V);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update VSETA failed:%d\n", err);
		return -1;
	}

	err = g2230_chip_enable_chgdet(chip, false);
	if (err != 0) {
		dev_info(chip->dev,
			 "g2230 chip set auto chargint type failed:%d\n", err);
		return err;
	}

#if 0
	/* disable the TS_METER */
	err = g2230_update(chip, G2230_CHIP_REG_A11_INT1_MASK,
					G2230_A11_MASK_TS, A11_MASK_TS);
	if (err != 0) {
		dev_info(chip->dev, "g2230 update MASK_TS failed:%d\n", err);
		return -1;
	}
#endif

	/** clear the interrupt **/
	err = g2230_chip_clear_interrupt(chip);
	if (err != 0) {
		dev_info(chip->dev, "g2230 chip clear interrupt failed:%d\n",
			 err);
		return -1;
	}

	return err;
}

#if defined(CONFIG_REGMAP)
static const struct regmap_config g2230_regmap = {
	.reg_bits = 8, .val_bits = 8, .max_register = G2230_CHIP_MAX_REG,
};

static int g2230_regmap_init(struct g2230_chip *chip)
{
	int rval = 0;

	chip->regmap = devm_regmap_init_i2c(chip->client, &g2230_regmap);
	if (IS_ERR(chip->regmap)) {
		rval = PTR_ERR(chip->regmap);
		dev_info(chip->dev, "<%s,%d>,fail : allocate reg. map: %d\n",
			 __func__, __LINE__, rval);
		return rval;
	}

	return rval;
}
#else
static int g2230_regmap_init(struct g2230_chip *chip)
{
	return 0;
}
#endif

static bool g2230_check_version(struct g2230_chip *chip)
{
	u8 version = -1;
	int ret = -1;

	ret = i2c_smbus_read_byte_data(chip->client, G2230_CHIP_REG_A17_VER);
	if (ret < 0) {
		dev_info(chip->dev, "i2c read failed:%d\n", ret);
		return false;
	}

	version = ((ret & 0xff) & 0x3f); /**only 0~5 bit is valid **/
	dev_info(chip->dev, "[%s,%d],version=0x%02x\n", __func__, __LINE__,
		 version);
	if (version == G2230_CHIP_ID)
		return true;

	return false;
}

static int g2230_gpio_init(struct g2230_chip *chip)
{
	int err = -1;

	if (gpio_is_valid(chip->chg_ce_gpio)) {
		err = devm_gpio_request_one(chip->dev, chip->chg_ce_gpio,
					    GPIOF_OUT_INIT_LOW, "chg_ce_gpio");
		if (err < 0) {
			dev_info(chip->dev, "che ce gpio request failed");
			return err;
		}
	}

	if (gpio_is_valid(chip->chg_int_gpio)) {
		err = devm_gpio_request_one(chip->dev, chip->chg_int_gpio,
					    GPIOF_IN, "chg_int_gpio");
		if (err < 0) {
			dev_info(chip->dev, "che int gpio request failed");
			return err;
		}
	}

	return err;
}

/* create sysfs node for g2230 */
static ssize_t g2230_show_charger_cur(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	u32 curchg = 0;
	int err = -1;

	err = __g2230_get_ichg(chip, &curchg);
	if (err < 0)
		dev_info(chip->dev, "get charging current failed:%d\n", err);

	return snprintf(buf, PAGE_SIZE, "%d\n", curchg);
}

static ssize_t g2230_store_charger_cur(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	int ret = -1;
	int ivalue = -1;
	u8 intstatus = -1;
	u8 irqstatus = -1;

	ret = kstrtoint(buf, 0, &ivalue);
	if (ret < 0) {
		dev_notice(dev, "%s parsing number fail(%d)\n", __func__, ret);
		return count;
	}

	if (ivalue == 1) {
		ret = g2230_chip_i2c_read_byte(
			chip, G2230_CHIP_REG_A10_INT1_STATUS, &intstatus);
	} else if (ivalue == 2) {
		if (gpio_is_valid(chip->chg_int_gpio))
			gpio_set_value_cansleep(chip->chg_int_gpio, 0);
	} else if (ivalue == 3) {
		if (gpio_is_valid(chip->chg_int_gpio))
			gpio_set_value_cansleep(chip->chg_int_gpio, 1);
	} else if (ivalue == 4) {
		ret = g2230_chip_i2c_read_byte(
			chip, G2230_CHIP_REG_A12_INT2_STATUS, &irqstatus);
	} else if (ivalue == 6) {
		chip->online = true;
	}

	return count;
}

static ssize_t g2230_dump_regs(struct g2230_chip *chip, char *buf,
			       size_t bufLen)
{
	u8 regValue = 0;
	int err = -1;
	int icount = 0;
	int len = 0;
	char tmp[] = "[Register] [Value]";
	int reglist[] = {
		G2230_CHIP_REG_A0,
		G2230_CHIP_REG_A1,
		G2230_CHIP_REG_A2,
		G2230_CHIP_REG_A3,
		G2230_CHIP_REG_A4,
		G2230_CHIP_REG_A5,
		G2230_CHIP_REG_A6,
		G2230_CHIP_REG_A7,
		G2230_CHIP_REG_A8,
		G2230_CHIP_REG_A9,
		G2230_CHIP_REG_A10_INT1_STATUS,
		G2230_CHIP_REG_A11_INT1_MASK,
		G2230_CHIP_REG_A12_INT2_STATUS,
		G2230_CHIP_REG_A13_INT2_MASK,
		G2230_CHIP_REG_A14,
		G2230_CHIP_REG_A15,
		G2230_CHIP_REG_A16,
		G2230_CHIP_REG_A17_VER,
	};
	int regNum = ARRAY_SIZE(reglist);

	len += snprintf(buf + len, bufLen - len, "%s\n", tmp);
	for (icount = 0; icount < regNum; icount++) {
		err = g2230_chip_i2c_read_byte(chip, reglist[icount],
					       &regValue);
		if (err != 0) {
			dev_info(chip->dev,
				 "<%s,%d>,read reg:0x%02x failed:%d\n",
				 __func__, __LINE__, reglist[icount], regValue);
			continue;
		}
		if (icount == (regNum - 1))
			regValue = (regValue & 0x3f);

		len += snprintf((buf + len), (bufLen - len), "0x%02x::0x%02x\n",
				reglist[icount], regValue);
	}

	return len;
}

static ssize_t g2230_show_allreg(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);

	return g2230_dump_regs(chip, buf, PAGE_SIZE);
}

static ssize_t g2230_store_allreg(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	int ret = -1;
	int ivalue = -1;
	u8 regvalue = -1;

	ret = kstrtoint(buf, 0, &ivalue);
	if (ret < 0) {
		dev_notice(dev, "%s parsing number fail(%d)\n", __func__, ret);
		return count;
	}
	if (ivalue == 0) {
		ret = g2230_chip_i2c_read_byte(
			chip, G2230_CHIP_REG_A12_INT2_STATUS, &regvalue);
		dev_info(chip->dev, "[%s,%d],ret =%d,regvalue=0x%02x\n",
			 __func__, __LINE__, ret, regvalue);
	} else if (ivalue == 1) {
		g2230_i2c_block_write(chip, G2230_CHIP_REG_A14, 0x02, 0x00);
	}

	return count;
}

static ssize_t g2230_show_online(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", chip->online);
}

static ssize_t g2230_show_isCharging(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	int err = -1;
	bool isEnchin = false;
	u8 data = 0;

	err = g2230_chip_i2c_read_byte(chip, G2230_CHIP_REG_A0, &data);

	if (((data >> A0_ENCHIN_SHIFT_NUM) & BIT0_MASK) == 0x00)
		isEnchin = true;

	return snprintf(buf, PAGE_SIZE, "ENCHIN:%d:%d\n", isEnchin,
			chip->isCharging);
}

static ssize_t g2230_show_bChargingCurHadSet(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", chip->bChargingCurHadSet);
}

static ssize_t g2230_show_ChargingCurStatus(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	enum g2230_chg_status chg_stat = G2230_CHG_STATUS_UNKNOWN;

	chg_stat = g2230_get_charging_cur_status(chip);

	return snprintf(buf, PAGE_SIZE, "CHARGING_STATUS:%d\n", chg_stat);
}

static ssize_t g2230_show_charger_cv(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	u32 cv = 0;
	int err = -1;

	err = __g2230_get_cv(chip, &cv);
	if (err < 0)
		dev_info(chip->dev, "get charging cv failed:%d\n", err);

	return snprintf(buf, PAGE_SIZE, "%d\n", cv);
}

static ssize_t g2230_store_charger_cv(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);
	int ret = -1;
	u32 ivalue = -1;

	ret = kstrtoint(buf, 0, &ivalue);
	if (ret < 0) {
		dev_notice(dev, "%s parsing number fail(%d)\n", __func__, ret);
		return count;
	}
	ret = g2230_update(chip, G2230_CHIP_REG_A4, G2230_A4_VSETA,
						ivalue << G2230_A4_VSETA_SHFT);

	ret = __g2230_get_cv(chip, &ivalue);
	dev_dbg(chip->dev, "%s: cv = %u\n", __func__, ivalue);

	return count;
}

static struct device_attribute g2230_attrs[] = {

	__ATTR(charger_cur, 0644, g2230_show_charger_cur,
			g2230_store_charger_cur),
	__ATTR(allreg, 0644, g2230_show_allreg, g2230_store_allreg),
	__ATTR(online, 0444, g2230_show_online, NULL),
	__ATTR(isCharging, 0444, g2230_show_isCharging, NULL),
	__ATTR(bChargingCurHadSet, 0444, g2230_show_bChargingCurHadSet, NULL),
	__ATTR(ChargingCurStatus, 0444, g2230_show_ChargingCurStatus, NULL),
	__ATTR(charger_cv, 0644, g2230_show_charger_cv, g2230_store_charger_cv),
};

static struct attribute *g2230_attributes[] = {&g2230_attrs[0].attr,
					       &g2230_attrs[1].attr,
					       &g2230_attrs[2].attr,
					       &g2230_attrs[3].attr,
					       &g2230_attrs[4].attr,
					       &g2230_attrs[5].attr,
					       &g2230_attrs[6].attr,
					       NULL};

static const struct attribute_group g2230_group = {
	.attrs = g2230_attributes,
};

static int g2230_sysfs_create(struct g2230_chip *chip)
{
	int err = -1;

	err = sysfs_create_group(&chip->dev->kobj, &g2230_group);
	if (err < 0) {
		dev_info(chip->dev, "<%s,%d>,create the sysfs failed:%d\n",
			 __func__, __LINE__, err);
	}

	return err;
}

static int g2230_remove_sysfs(struct g2230_chip *chip)
{
	sysfs_remove_group(&chip->dev->kobj, &g2230_group);

	return 0;
}


static int g2230_probe(struct i2c_client *client,
		       const struct i2c_device_id *id)
{
	int ret = 0;
	struct g2230_chip *chip = NULL;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = &client->dev;
	mutex_init(&chip->io_lock);
	mutex_init(&chip->ichg_lock);
	mutex_init(&chip->dcdet_lock);
	chip->chg_type = CHARGER_UNKNOWN;
	chip->isCharging = false;
	chip->online = false;
	chip->bChargingCurHadSet = false;
	chip->bDcdetDisConnect = false;
	chip->bUsbDetect_Init = false;
	chip->iDecdetNum = 0;
	chip->iDecdetIRQnum = 0;
	chip->iDecdetIRQbit = 0;
	atomic_set(&chip->vbus_good, 0);
	i2c_set_clientdata(client, chip);

	ret = g2230_parse_dt(&client->dev, chip);
	if (ret < 0) {
		dev_info(&client->dev, "parse failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_check_version(chip);
	if (ret == 0) {
		dev_info(&client->dev, "check version failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_regmap_init(chip);
	if (ret < 0) {
		dev_info(&client->dev, "regmap failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_gpio_init(chip);
	if (ret < 0) {
		dev_info(&client->dev, "gpio init failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_chip_init(chip);
	if (ret < 0) {
		dev_info(&client->dev, "chip init failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_charger_device_register(chip);
	if (ret != 0) {
		dev_info(&client->dev,
			 "g2230 charger device register failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	INIT_DELAYED_WORK(&chip->g2230_interrupt_delay_work,
			  g2230_interrupt_work);

	ret = g2230_threaded_init(chip);
	if (ret < 0) {
		dev_info(&client->dev, "create threaded failed:%d\n", ret);
		devm_kfree(&client->dev, chip);
		return ret;
	}

	ret = g2230_sysfs_create(chip);
	if (ret < 0) {
		dev_info(&client->dev, "create sys filesystem failed:%d\n",
			 ret);
	}

	if (chip->chg_irq)
		enable_irq(chip->chg_irq);

	/** now we need check the usb/dcdc is valid */
	ret = g2230_chip_poweron_check(chip);
	if (ret < 0) {
		dev_info(chip->dev,
			 "g2230 poweron check USB/DCDC status failed.\n");
	}
	dev_info(&client->dev, "g2230 probe successfully\n");

	return 0;
}

static int g2230_remove(struct i2c_client *client)
{
	struct g2230_chip *chip = i2c_get_clientdata(client);

	mutex_destroy(&chip->io_lock);
	mutex_destroy(&chip->ichg_lock);
	mutex_destroy(&chip->dcdet_lock);

	g2230_remove_sysfs(chip);

	return 0;
}

static const struct of_device_id g2230_of_device_id[] = {
	{ .compatible = "gmt,g2230", },
	{},
};
MODULE_DEVICE_TABLE(of, g2230_of_device_id);

static const struct i2c_device_id g2230_i2c_device_id[] = {
	{ "g2230", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, g2230_i2c_device_id);

#ifdef CONFIG_PM_SLEEP
static int g2230_suspend(struct device *dev)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);

	dev_info(chip->dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		enable_irq_wake(chip->chg_irq);
	return 0;
}

static int g2230_resume(struct device *dev)
{
	struct g2230_chip *chip = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(chip->chg_irq);
	return 0;
}

static SIMPLE_DEV_PM_OPS(g2230_pm_ops, g2230_suspend, g2230_resume);
#endif

static struct i2c_driver g2230_i2c_driver = {
	.driver = {

			.name = "g2230",
			.owner = THIS_MODULE,
			.of_match_table = of_match_ptr(g2230_of_device_id),
#ifdef CONFIG_PM_SLEEP
			.pm = &g2230_pm_ops,
#endif
		},
	.probe = g2230_probe,
	.remove = g2230_remove,
	.id_table = g2230_i2c_device_id,
};
module_i2c_driver(g2230_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Wentao He <wentao.he@mediatek.com>");
MODULE_DESCRIPTION("G2230 Charger Driver");
MODULE_VERSION(G2230_DRV_VERSION);
/*
 * Release Note
 * 1.0.0
 * (1) Initial released
 */

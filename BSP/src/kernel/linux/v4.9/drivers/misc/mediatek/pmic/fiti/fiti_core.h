/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Accelerometer Sensor Driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *****************************************************************************/


#ifndef __FITI_CORE_H_
#define __FITI_CORE_H_

#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/jiffies.h>


//#define PMIC_FPGA	1


#define VCOM_SETTING_STEP	(5000/255)


enum FITI_POWER_STATE_ENUM {
	POWER_OFF = 0,
	POWER_ON_GOING = 1,
	POWER_ON = 2,
	POWER_OFF_GOING = 3,
};


enum {
	FITI_9929 = 0x00,
	FITI_9930 = 0x01,
};

struct fiti_pmic_setting {
	int VCOM_SETTING;
	int VPOS;
	int VNEG;
};

struct fiti_context {
	int EPD_PMIC_EN;
	int EPD_EN_TS;
	int EPD_PMIC_NM_EN;
	int fiti_id;
	bool power_good_status;
	bool power_off_status;
	int power_good_irq;
	struct fiti_pmic_setting pmic_setting;
	struct regulator *reg_edp;
	struct pinctrl *pctrl;
	struct pinctrl_state *pin_state_active;
	struct pinctrl_state *pin_state_inactive;
	int version;

	spinlock_t power_state_lock;
	enum FITI_POWER_STATE_ENUM current_state;
	enum FITI_POWER_STATE_ENUM target_state;

	/* work for enable & disable power */
	//struct work_struct power_work;
	struct delayed_work power_work;
	/* thread for enable & disable power */
	struct workqueue_struct *power_workqueue;
	int iIsNightMode;
};

enum {
	REG_TMST_VALUE = 0x00,
	REG_FUNC_ADJUST = 0x01,
	REG_VCOM_SETTING = 0x02,
	REG_NM = 0x10,
};

enum {
	FITI9930_TMST_VALUE = 0x00,
	FITI9930_VCOM_SETTING = 0x01,
	FITI9930_VPOS_VNEG_SETTING = 0x02,
	FITI9930_PWRON_DELAY = 0x03,
	FITI9930_CONTROL_REG1 = 0x0B,
	FITI9930_CONTROL_REG2 = 0x0C,
	FITI9930_REG_NUM
};

int fiti_i2c_write(unsigned char reg, unsigned char writedata);
int fiti_i2c_read(unsigned char reg, unsigned char *rd_buf);
int fiti_read_temperature(void);

int fiti_read_vcom(void);
void fiti_setting_get_from_waveform(char *waveform_addr);
int fiti_write_vcom(int vcom_mv);
void fiti_set_version(unsigned int version);

void edp_vdd_disable(void);
void edp_vdd_enable(void);
void fiti_power_enable(bool enable,int iDelayms);
void fiti_wait_power_good(void);
bool fiti_pmic_judge_power_on_going(void);
bool fiti_pmic_judge_power_off(void);
bool fiti_pmic_judge_power_on(void);

int fiti_pmic_get_current_state(void);
int fiti_pmic_pwrwork_pending(void);

#endif

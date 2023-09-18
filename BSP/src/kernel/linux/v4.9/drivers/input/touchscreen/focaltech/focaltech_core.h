/*
 *
 * FocalTech fts TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
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
 */

#ifndef __LINUX_FTS_H__
#define __LINUX_FTS_H__
/*******************************************************************************
 *
 * File Name: focaltech.c
 *
 * Author: mshl
 *
 * Created: 2014-09
 *
 * Modify by mshl on 2015-04-30
 *
 * Abstract:
 *
 * Reference:
 *
 ******************************************************************************/

/*******************************************************************************
 * Included header files
 ******************************************************************************/
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#endif
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#if defined(CONFIG_SENSORS)
#include <linux/sensors.h>
#endif
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mount.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>
#include <linux/ioctl.h>
//#include "ft_gesture_lib.h"
#include <focaltech_common.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
/*******************************************************************************
 * Private constant and macro definitions using #define
 ******************************************************************************/
#define FTS_DRIVER_INFO  "nxp_Ver 1.1 2017-05-06"
#define FT5X06_ID		0x55
#define FT5X16_ID		0x0A
#define FT5X36_ID		0x14
#define FT6X06_ID		0x06
#define FT6X36_ID		0x36

#define FT5316_ID		0x0A
#define FT5306I_ID		0x55

#define FTS_MAX_POINTS                        10

#define FTS_WORKQUEUE_NAME	"fts_wq"

#define FTS_DEBUG_DIR_NAME	"fts_debug"

#define FOCALTECH_I2C_NAME "fts_ts"
#define FOCALTECH_I2C_ADD  0x38

#define FTS_INFO_MAX_LEN		512
#define FTS_FW_NAME_MAX_LEN	50

#define FTS_REG_ID		0xA3
#define FTS_REG_FW_VER		0xA6
#define FTS_REG_FW_VENDOR_ID	0xA8
#define FTS_REG_POINT_RATE					0x88

#define FTS_FACTORYMODE_VALUE	0x40
#define FTS_WORKMODE_VALUE	0x00

#define FTS_STORE_TS_INFO(buf, id, name, max_tch, group_id, fw_vkey_support, \
			fw_name, fw_maj, fw_min, fw_sub_min) \
			snprintf(buf, FTS_INFO_MAX_LEN, \
				"controller\t= focaltech\n" \
				"model\t\t= 0x%x\n" \
				"name\t\t= %s\n" \
				"max_touches\t= %d\n" \
				"drv_ver\t\t= %s\n" \
				"group_id\t= 0x%x\n" \
				"fw_vkey_support\t= %s\n" \
				"fw_name\t\t= %s\n" \
				"fw_ver\t\t= %d.%d.%d\n", id, name, \
				max_tch, FTS_DRIVER_INFO, group_id, \
				fw_vkey_support, fw_name, fw_maj, fw_min, \
				fw_sub_min)

#define FTS_DBG_EN 0
#if FTS_DBG_EN
#define FTS_DBG(fmt, args...) pr_err("[FTS][%s,%d]"fmt, __func__, __LINE__, \
				## args)
#define FTS_ERR(fmt, args...) pr_err("[FTS][%s,%d]"fmt, __func__, __LINE__, \
				## args)
#else
#define FTS_ERR(fmt, args...) pr_err("[FTS][%s,%d]"fmt, __func__, __LINE__, \
				## args)
#define FTS_DBG(fmt, args...) do {} while (0)
#endif


/*******************************************************************************
 * Private enumerations, structures and unions using typedef
 ******************************************************************************/

struct fts_Upgrade_Info {
	u8 CHIP_ID;
	u8 TPD_MAX_POINTS;
	u8 AUTO_CLB;
	u16 delay_aa;			/*delay of write FT_UPGRADE_AA */
	u16 delay_55;			/*delay of write FT_UPGRADE_55 */
	u8 upgrade_id_1;		/*upgrade id 1 */
	u8 upgrade_id_2;		/*upgrade id 2 */
	u16 delay_readid;		/*delay of read id */
	u16 delay_erase_flash;		/*delay of earse flash*/
};

struct fts_ts_platform_data {
	struct fts_Upgrade_Info info;
	const char *name;
	const char *fw_name;
	u32 irqflags;
	u32 irq_gpio;
	u32 irq_gpio_flags;
	u32 reset_gpio;
	u32 reset_gpio_flags;
#if defined(CONFIG_TOUCH_I2C_WACOM)
	u32 wacom_status;
#endif
	u32 family_id;
	u32 x_max;
	u32 y_max;
	u32 x_min;
	u32 y_min;
	u32 panel_minx;
	u32 panel_miny;
	u32 panel_maxx;
	u32 panel_maxy;
	u32 group_id;
	u32 hard_rst_dly;
	u32 soft_rst_dly;
	u32 num_max_touches;
	bool fw_vkey_support;
	bool no_force_update;
	bool i2c_pull_up;
	bool ignore_id_check;
	bool psensor_support;
	bool power_regulator;
	int (*power_init)(bool);
	int (*power_on)(bool);
};

#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
struct ts_event {
	int x; /*x coordinate */
	int y; /*y coordinate */
	int p; /* pressure */
	int flag; /* touch event flag: 0 -- down; 1-- up; 2 -- contact */
	int id;   /*touch ID */
	int area;
};
#else
struct ts_event {
	u16 au16_x[FTS_MAX_POINTS];	/*x coordinate */
	u16 au16_y[FTS_MAX_POINTS];	/*y coordinate */
	u8 au8_touch_event[FTS_MAX_POINTS];	/*touch event: */
					/* 0 -- down; 1-- up; 2 -- contact */
	u8 au8_finger_id[FTS_MAX_POINTS];	/*touch ID */
	u16 pressure;
	u8 touch_point;
	u8 point_num;
};
#endif

struct fts_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct device *dev;
#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
	/* multi-touch */
	struct ts_event *events;
#else
	struct ts_event event;
#endif
	const struct fts_ts_platform_data *pdata;
	struct fts_psensor_platform_data *psensor_pdata;
	struct work_struct touch_event_work;
	struct workqueue_struct *ts_workqueue;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	char fw_name[FTS_FW_NAME_MAX_LEN];
	bool loading_fw;
	u8 family_id;
	struct dentry *dir;
	u16 addr;
	bool suspended;
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	bool bEarlySuspended;
	bool bNeedReport;
	bool bEnableNoSuspended;
	bool bInTouch;
#endif
	char *ts_info;
	//u8 *tch_data;
	//u32 tch_data_len;
#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
	int pnt_buf_size;
	u8 *point_buf;
	int touch_point;
	int point_num;
#endif
	u8 fw_ver[3];
	u8 fw_vendor_id;
	int touchs;
	int irq;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#if defined(CONFIG_USE_PINCTRL_GPIO)
	struct pinctrl *ts_pinctrl;
	struct pinctrl_state *pinctrl_state_active;
	struct pinctrl_state *pinctrl_state_suspend;
	struct pinctrl_state *pinctrl_state_release;
#endif
};

#if defined(CONFIG_SENSORS)
struct fts_psensor_platform_data {
	struct input_dev *input_psensor_dev;
	struct sensors_classdev ps_cdev;
	int tp_psensor_opened;
	char tp_psensor_data; /* 0 near, 1 far */
	struct fts_ts_data *data;
};
#endif
/*******************************************************************************
 * Static variables
 ******************************************************************************/


/*******************************************************************************
 * Global variable or extern global variabls/functions
 ******************************************************************************/
//Function Switches: define to open,  comment to close
#define FTS_GESTRUE_EN 0
#define GTP_ESD_PROTECT 0
#define FTS_APK_DEBUG
#define FTS_SYSFS_DEBUG
#define FTS_CTL_IIC
//#define CONFIG_TOUCHSCREEN_FTS_PSENSOR
//#define FTS_AUTO_UPGRADE
extern struct fts_Upgrade_Info fts_updateinfo_curr;
extern struct i2c_client *fts_i2c_client;
extern struct fts_ts_data *fts_wq_data;
extern struct input_dev *fts_input_dev;

static DEFINE_MUTEX(i2c_rw_access);

//Getstre functions
extern int fts_Gesture_init(struct input_dev *input_dev);
extern int fts_read_Gestruedata(void);
extern int fetch_object_sample(unsigned char *buf, short pointnum);
extern void init_para(int x_pixel, int y_pixel, int time_slot,
			int cut_x_pixel, int cut_y_pixel);

//upgrade functions
extern void fts_update_fw_vendor_id(struct fts_ts_data *data);
extern void fts_update_fw_ver(struct fts_ts_data *data);
extern void fts_get_upgrade_array(struct fts_ts_data *data);
extern int fts_ctpm_auto_upgrade(struct i2c_client *client);
extern int fts_fw_upgrade(struct device *dev, bool force);
extern int fts_ctpm_auto_clb(struct i2c_client *client);
extern int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
				char *firmware_name);
extern int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client);
extern int fts_ctpm_get_i_file_ver(void);

//Apk and functions
extern int fts_create_apk_debug_channel(struct i2c_client *client);
extern void fts_release_apk_debug_channel(void);

//ADB functions
extern int fts_create_sysfs(struct i2c_client *client);
extern int fts_remove_sysfs(struct i2c_client *client);

//char device for old apk
extern int fts_rw_iic_drv_init(struct i2c_client *client);
extern void  fts_rw_iic_drv_exit(void);

//Base functions
extern int fts_i2c_read(struct i2c_client *client, char *writebuf,
		int writelen, char *readbuf, int readlen);
extern int fts_i2c_write(struct i2c_client *client,
		char *writebuf, int writelen);
extern int fts_read_reg(struct i2c_client *client, u8 addr, u8 *val);
extern int fts_write_reg(struct i2c_client *client, u8 addr, const u8 val);

/*******************************************************************************
 * Static function prototypes
 ******************************************************************************/
extern int fts_power_control(struct fts_ts_data *fts_data, bool on);

#endif

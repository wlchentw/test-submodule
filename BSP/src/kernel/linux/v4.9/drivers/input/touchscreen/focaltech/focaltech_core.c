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
 * VERSION	DATE			AUTHOR
 *	1.0		2014-09			mshl
 *
 */

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
//user defined include header files
#include "focaltech_core.h"

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
//#include <linux/suspend.h>
#endif
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
/* Early-suspend level */
#define FTS_SUSPEND_LEVEL 1
#endif
#if FTS_GESTRUE_EN
#include "ft_gesture_lib.h"
#endif

extern void imx_virtual_send_wakeup_events(void);
extern void imx_virtual_send_pagedown_events(void);

/*******************************************************************************
 * Private enumerations, structures and unions using typedef
 ******************************************************************************/

/*******************************************************************************
 * Static variables
 ******************************************************************************/

/*******************************************************************************
 * Global variable or extern global variabls/functions
 ******************************************************************************/
struct i2c_client *fts_i2c_client;
//struct fts_ts_data *fts_wq_data;
struct input_dev *fts_input_dev;

#if !defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
static unsigned int buf_count_add;
static unsigned int buf_count_neg;

u8 buf_touch_data[30 * POINT_READ_BUF] = { 0 };
#endif
/*******************************************************************************
 * Static function prototypes
 ******************************************************************************/
static int fts_ts_start(struct device *dev);
static int fts_ts_stop(struct device *dev);
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
extern bool wacom_mem_cancel_work(void);
#endif


#if defined(CONFIG_SENSORS)
static struct sensors_classdev __maybe_unused sensors_proximity_cdev = {
	.name = "fts-proximity",
	.vendor = "FocalTech",
	.version = 1,
	.handle = SENSORS_PROXIMITY_HANDLE,
	.type = SENSOR_TYPE_PROXIMITY,
	.max_range = "5.0",
	.resolution = "5.0",
	.sensor_power = "0.1",
	.min_delay = 0,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.enabled = 0,
	.delay_msec = 200,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
};
/*******************************************************************************
 *  Name: fts_psensor_support_enabled
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/

static inline bool fts_psensor_support_enabled(void)
{
	return config_enabled(CONFIG_TOUCHSCREEN_FTS_PSENSOR);
}
#endif
/*******************************************************************************
 *  Name: fts_i2c_read
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_i2c_read(struct i2c_client *client, char *writebuf,
		int writelen, char *readbuf, int readlen)
{
	int ret;

	mutex_lock(&i2c_rw_access);

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
		.addr = client->addr,
		.flags = 0,
		.len = writelen,
		.buf = writebuf,
			 },
			{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = readlen,
		.buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev,
				"%s: i2c read error.\n", __func__);
	} else {
		struct i2c_msg msgs[] = {
			{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = readlen,
		.buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}

	mutex_unlock(&i2c_rw_access);

	return ret;
}

/*******************************************************************************
 *  Name: fts_i2c_write
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_i2c_write(struct i2c_client *client, char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
		.addr = client->addr,
		.flags = 0,
		.len = writelen,
		.buf = writebuf,
	},
	};
	mutex_lock(&i2c_rw_access);
	ret = i2c_transfer(client->adapter, msgs, 1);
	if (ret < 0)
		dev_err(&client->dev, "%s: i2c write error.\n", __func__);

	mutex_unlock(&i2c_rw_access);

	return ret;
}

/*******************************************************************************
 *  Name: fts_write_reg
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_write_reg(struct i2c_client *client, u8 addr, const u8 val)
{
	u8 buf[2] = {0};

	buf[0] = addr;
	buf[1] = val;

	return fts_i2c_write(client, buf, sizeof(buf));
}

/*******************************************************************************
 *  Name: fts_read_reg
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_read_reg(struct i2c_client *client, u8 addr, u8 *val)
{
	return fts_i2c_read(client, &addr, 1, val, 1);
}

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
/*
 * return:
 *  1  ---finger had touched
 *  0  --- finger not touch
 */
bool fts_getTPTouchState(void)
{
	bool bInTouch = false;

	if (fts_i2c_client != NULL) {
		struct fts_ts_data *ftsdata =
			i2c_get_clientdata(fts_i2c_client);

		bInTouch = ftsdata->bInTouch;
	}

	return bInTouch;
}

int fts_set_enable_wakeup(bool bEnable)
{
	if (fts_i2c_client != NULL) {
		struct fts_ts_data *ftsdata =
			i2c_get_clientdata(fts_i2c_client);

		ftsdata->bEnableNoSuspended = bEnable;
	}

	return 0;
}
#endif

#if defined(CONFIG_SENSORS)
#ifdef CONFIG_TOUCHSCREEN_FTS_PSENSOR
/*******************************************************************************
 *  Name: fts_psensor_enable
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void fts_psensor_enable(struct fts_ts_data *ftsdata, int enable)
{
	u8 state;
	int ret = -1;

	if (ftsdata->client == NULL)
		return;

	fts_read_reg(ftsdata->client, FTS_REG_PSENSOR_ENABLE, &state);
	if (enable)
		state |= FTS_PSENSOR_ENABLE_MASK;
	else
		state &= ~FTS_PSENSOR_ENABLE_MASK;

	ret = fts_write_reg(ftsdata->client, FTS_REG_PSENSOR_ENABLE, state);
	if (ret < 0)
		dev_err(&ftsdata->client->dev,
			"write psensor switch command failed\n");
}

/*******************************************************************************
 *  Name: fts_psensor_enable_set
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_psensor_enable_set(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct fts_psensor_platform_data *psensor_pdata =
		container_of(sensors_cdev,
			struct fts_psensor_platform_data, ps_cdev);
	struct fts_ts_data *ftsdata = psensor_pdata->ftsdata;
	struct input_dev *input_dev = ftsdata->psensor_pdata->input_psensor_dev;

	mutex_lock(&input_dev->mutex);
	fts_psensor_enable(ftsdata, enable);
	psensor_pdata->tp_psensor_data = FTS_PSENSOR_ORIGINAL_STATE_FAR;
	if (enable)
		psensor_pdata->tp_psensor_opened = 1;
	else
		psensor_pdata->tp_psensor_opened = 0;
	mutex_unlock(&input_dev->mutex);
	return enable;
}

/*******************************************************************************
 *  Name: fts_read_tp_psensor_data
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_read_tp_psensor_data(struct fts_ts_data *ftsdata)
{
	u8 psensor_status;
	char tmp;
	int ret = 1;

	fts_read_reg(ftsdata->client,
			FTS_REG_PSENSOR_STATUS, &psensor_status);

	tmp = ftsdata->psensor_pdata->tp_psensor_data;
	if (psensor_status == FTS_PSENSOR_STATUS_NEAR)
		ftsdata->psensor_pdata->tp_psensor_data =
						FTS_PSENSOR_FAR_TO_NEAR;
	else if (psensor_status == FTS_PSENSOR_STATUS_FAR)
		ftsdata->psensor_pdata->tp_psensor_data =
						FTS_PSENSOR_NEAR_TO_FAR;

	if (tmp != ftsdata->psensor_pdata->tp_psensor_data) {
		dev_dbg(&ftsdata->client->dev,
				"%s sensor ftsdata changed\n", __func__);
		ret = 0;
	}
	return ret;
}
#else
/*******************************************************************************
 *  Name: fts_psensor_enable_set
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_psensor_enable_set(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	return enable;
}

/*******************************************************************************
 *  Name: fts_read_tp_psensor_data
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_read_tp_psensor_data(struct fts_ts_data *ftsdata)
{
	return 0;
}
#endif
#endif
/*
 * reset the IC.
 */
void fts_reset(struct fts_ts_data *ftsdata)
{
	int err = -1;

	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {

		err = gpio_direction_output(ftsdata->pdata->reset_gpio, 0);
		if (err) {
			dev_err(&ftsdata->client->dev,
				"set_direction for reset gpio failed\n");
			return;
		}
		msleep(ftsdata->pdata->hard_rst_dly);
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 1);
	}

}
/*******************************************************************************
 *  Name: fts_ts_interrupt
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static irqreturn_t fts_ts_interrupt(int irq, void *dev_id)
{
	struct fts_ts_data *fts_ts = dev_id;
	//static bool bNeedReportCancel = false;
#if defined(CONFIG_TOUCH_I2C_WACOM)
	int wacom_status_value = -1;

	if (gpio_is_valid(fts_ts->pdata->wacom_status))
		wacom_status_value =
			gpio_get_value_cansleep(fts_ts->pdata->wacom_status);
	else
		wacom_status_value = -2;
	//FTS_ERR("wacom_status_value=%d,\n",wacom_status_value);
	if (wacom_status_value == 0) {
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
		fts_ts->bInTouch = false;
#endif
		return IRQ_HANDLED;
	}
#endif

	/** wakeup the devices **/
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	wacom_mem_cancel_work();
	if (fts_ts->bEarlySuspended) {
		imx_virtual_send_wakeup_events();
		fts_ts->bNeedReport = true;
	}
#endif

	disable_irq_nosync(fts_ts->irq);

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	fts_ts->bInTouch = true;
#endif

	if (!fts_ts) {
		pr_err("%s: Invalid fts_ts\n", __func__);
		return IRQ_HANDLED;
	}
	//dev_info(fts_ts->dev,"<%s,%d>,\n",__func__,__LINE__);

	queue_work(fts_ts->ts_workqueue, &fts_ts->touch_event_work);

	return IRQ_HANDLED;
}

/*******************************************************************************
 *  Name: fts_read_Touchdata
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
static int fts_read_Touchdata(struct fts_ts_data *ftsdata)
{
	int ret = 0;
	int i = 0;
	u8 pointid;
	int base;
#if FTS_GESTRUE_EN
	u8 state = 0;
#endif
	struct ts_event *events = ftsdata->events;
	int max_touch_num = ftsdata->pdata->num_max_touches;
	u8 *buf = ftsdata->point_buf;

	ftsdata->point_num = 0;
	ftsdata->touch_point = 0;

	memset(buf, 0xFF, ftsdata->pnt_buf_size);
	buf[0] = 0x00;

#if FTS_GESTRUE_EN
	if (ftsdata->suspended) {
		fts_read_reg(ftsdata->client, 0xB0, &state);
		if (state == 1) {
			fts_read_Gestruedata();
			return 1;
		}
	}
#endif


	/* cannot use do.while for read,because when it can read,
	 * the right value had ignore
	 */
	ret = fts_i2c_read(ftsdata->client, buf, 1, buf, ftsdata->pnt_buf_size);
	if (ret < 0) {
		dev_err(&ftsdata->client->dev,
			"read touchdata failed, ret:%d\n", ret);
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
		ftsdata->bInTouch = false;
#endif
		return ret;
	}

	ftsdata->point_num = (buf[FTS_TOUCH_POINT_NUM] & 0x0F);

	if (ftsdata->point_num > max_touch_num) {
		dev_err(&ftsdata->client->dev,
			"invalid point_num(%d)\n", ftsdata->point_num);
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
		ftsdata->bInTouch = false;
#endif
		return -EIO;
	}

	for (i = 0; i < max_touch_num; i++) {
		base = FTS_ONE_TCH_LEN * i;

		pointid = (buf[FTS_TOUCH_ID_POS + base]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		else if (pointid >= max_touch_num) {
			dev_err(&ftsdata->client->dev,
				"ID(%d) beyond max_touch_number\n", pointid);
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
			ftsdata->bInTouch = false;
#endif
			return -EINVAL;
		}

		ftsdata->touch_point++;

		events[i].x = ((buf[FTS_TOUCH_X_H_POS + base] & 0x0F) << 8) +
		    (buf[FTS_TOUCH_X_L_POS + base] & 0xFF);
		events[i].y = ((buf[FTS_TOUCH_Y_H_POS + base] & 0x0F) << 8) +
		    (buf[FTS_TOUCH_Y_L_POS + base] & 0xFF);
		events[i].flag = buf[FTS_TOUCH_EVENT_POS + base] >> 6;
		events[i].id = buf[FTS_TOUCH_ID_POS + base] >> 4;
		events[i].area = buf[FTS_TOUCH_AREA_POS + base] >> 4;
		events[i].p =  buf[FTS_TOUCH_PRE_POS + base];

		if (EVENT_DOWN(events[i].flag) && (ftsdata->point_num == 0)) {
			dev_err(&ftsdata->client->dev,
				"abnormal touch ftsdata from fw\n");
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
			ftsdata->bInTouch = false;
#endif
		return -EIO;
		}
	}
	if (ftsdata->touch_point == 0) {
		dev_err(&ftsdata->client->dev, "no touch point information\n");
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
		ftsdata->bInTouch = false;
#endif
	return -EIO;
	}

	return 0;
}

static int fts_report_value(struct fts_ts_data *ftsdata)
{
	int i = 0;
	int uppoint = 0;
	int touchs = 0;
	bool va_reported = false;
	u32 max_touch_num = ftsdata->pdata->num_max_touches;
	struct ts_event *events = ftsdata->events;

	for (i = 0; i < ftsdata->touch_point; i++) {

		if (events[i].id >= max_touch_num)
			break;

		va_reported = true;
		input_mt_slot(ftsdata->input_dev, events[i].id);

		if (EVENT_DOWN(events[i].flag)) {
			input_mt_report_slot_state(ftsdata->input_dev,
				MT_TOOL_FINGER, true);
			if (events[i].p <= 0)
				events[i].p = 0x3f;
			input_report_abs(ftsdata->input_dev,
				ABS_MT_PRESSURE, events[i].p);

			if (events[i].area <= 0)
				events[i].area = 0x09;
			input_report_abs(ftsdata->input_dev,
				ABS_MT_TOUCH_MAJOR, events[i].area);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_POSITION_X, events[i].x);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_POSITION_Y, events[i].y);

			touchs |= BIT(events[i].id);
			ftsdata->touchs |= BIT(events[i].id);

			FTS_DBG("[B]P%d(%d, %d)[p:%d,tm:%d] DOWN!\n",
				events[i].id, events[i].x, events[i].y,
				events[i].p, events[i].area);
		} else {
			uppoint++;
			FTS_DBG("[B]P%d (%d, %d)[p:%d,tm:%d] UP!\n",
				events[i].id, events[i].x, events[i].y,
				events[i].p, events[i].area);
			if (events[i].area == 14 && events[i].p == 238) {
				input_report_abs(ftsdata->input_dev,
				    ABS_MT_TOUCH_MAJOR, 0xFF);//events[i].area);
				input_report_abs(ftsdata->input_dev,
					ABS_MT_PRESSURE, 0xFF);//events[i].p);
				input_report_key(ftsdata->input_dev,
					BTN_TOUCH, 1);
				input_sync(ftsdata->input_dev);
			} else {
				input_report_abs(ftsdata->input_dev,
					ABS_MT_TOUCH_MAJOR, events[i].area);
				input_report_abs(ftsdata->input_dev,
					ABS_MT_PRESSURE, events[i].p);
			}
			input_mt_report_slot_state(ftsdata->input_dev,
				MT_TOOL_FINGER, false);

			ftsdata->touchs &= ~BIT(events[i].id);
		}
	}

	if (unlikely(ftsdata->touchs ^ touchs)) {
		for (i = 0; i < max_touch_num; i++)  {
			if (BIT(i) & (ftsdata->touchs ^ touchs)) {
				va_reported = true;
				input_mt_slot(ftsdata->input_dev, i);
				input_mt_report_slot_state(ftsdata->input_dev,
					MT_TOOL_FINGER, false);
			}
		}
	}
	ftsdata->touchs = touchs;

	if (va_reported) {
		/* touchs==0, there's no point but key */
		if (EVENT_NO_DOWN(ftsdata) || (!touchs)) {
			//FTS_DBG("[B]Points All Up!\n\n");
			input_report_key(ftsdata->input_dev, BTN_TOUCH, 0);
		} else {
			input_report_key(ftsdata->input_dev, BTN_TOUCH, 1);
		}
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
		ftsdata->bInTouch = false;
#endif
	}

	input_sync(ftsdata->input_dev);

	return 0;
}

#else

static int fts_read_Touchdata(struct fts_ts_data *ftsdata)
{
	u8 buf[POINT_READ_BUF] = { 0 };
	int ret = -1;
	//int rc = 0;

#if FTS_GESTRUE_EN
	u8 state = 0;

	if (ftsdata->suspended) {
		fts_read_reg(ftsdata->client, 0xB0, &state);
		//FTS_DBG("tpd fts_read_Gestruedata state=%d\n",state);
		if (state == 1) {
			fts_read_Gestruedata();
			return 1;
		}
	}
#endif

#ifdef CONFIG_TOUCHSCREEN_FTS_PSENSOR
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support &&
		ftsdata->psensor_pdata->tp_psensor_opened) {
		rc = fts_read_tp_psensor_data(ftsdata);
		if (!rc) {
			if (ftsdata->suspended)
				pm_wakeup_event(&ftsdata->client->dev,
					FTS_PSENSOR_WAKEUP_TIMEOUT);
			input_report_abs(
				ftsdata->psensor_pdata->input_psensor_dev,
				ABS_DISTANCE,
				ftsdata->psensor_pdata->tp_psensor_data);
			input_sync(ftsdata->psensor_pdata->input_psensor_dev);
			if (ftsdata->suspended)
				return 1;
		}
		if (ftsdata->suspended)
			return 1;
	}
	#endif

	ret = fts_i2c_read(ftsdata->client, buf, 1, buf, POINT_READ_BUF);
	if (ret < 0) {
		dev_err(&ftsdata->client->dev,
			"%s read touchdata failed.\n", __func__);
	/*
	 * maybe we should reset thr IC.
	 * we can do this for all porject.
	 * -wen -09-15
	 */
	fts_reset(ftsdata);
		return ret;
	}

	buf_count_add++;
	memcpy(buf_touch_data + (((buf_count_add-1) % 30) * POINT_READ_BUF),
		buf, sizeof(u8)*POINT_READ_BUF);

	return 0;
}

/*******************************************************************************
 *  Name: fts_report_value
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void fts_report_value(struct fts_ts_data *ftsdata)
{
	struct ts_event *event = &ftsdata->event;
	int i;
	int uppoint = 0;
	int touchs = 0;
	u8 pointid = FTS_MAX_ID;
	u8 buf[POINT_READ_BUF] = { 0 };

	buf_count_neg++;

	memcpy(buf,
	  (buf_touch_data + (((buf_count_neg - 1) % 30) * POINT_READ_BUF)),
	  sizeof(u8)*POINT_READ_BUF);

	memset(event, 0, sizeof(struct ts_event));

	event->point_num = buf[FTS_TOUCH_POINT_NUM] & 0x0F;
	event->touch_point = 0;
	for (i = 0; i < FTS_MAX_POINTS; i++) {
		pointid = (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;
		if (pointid >= FTS_MAX_ID)
			break;
		event->touch_point++;

		event->au16_x[i] =
		  (s16) (buf[FTS_TOUCH_X_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
		  8 | (s16) buf[FTS_TOUCH_X_L_POS + FTS_ONE_TCH_LEN * i];
		event->au16_y[i] =
		  (s16) (buf[FTS_TOUCH_Y_H_POS + FTS_ONE_TCH_LEN * i] & 0x0F) <<
		  8 | (s16) buf[FTS_TOUCH_Y_L_POS + FTS_ONE_TCH_LEN * i];
		event->au8_touch_event[i] =
		  buf[FTS_TOUCH_EVENT_POS + FTS_ONE_TCH_LEN * i] >> 6;
		event->au8_finger_id[i] =
		  (buf[FTS_TOUCH_ID_POS + FTS_ONE_TCH_LEN * i]) >> 4;

		if ((event->au8_touch_event[i] == 0
			|| event->au8_touch_event[i] == 2)
			&& (event->point_num == 0))
			return;
	}

	event->pressure = FTS_PRESS;
	//FTS_DBG("==%d =%d\n",buf[7],buf[8]);
	for (i = 0; i < event->touch_point; i++) {
		input_mt_slot(ftsdata->input_dev, event->au8_finger_id[i]);
		//FTS_DBG("au8_touch_event=%d\n",event->au8_touch_event[i]);
		if (event->au8_touch_event[i] == FTS_TOUCH_DOWN
			|| event->au8_touch_event[i] == FTS_TOUCH_CONTACT) {
		//FTS_DBG("x=%d,y=%d\n",event->au16_x[i],event->au16_y[i]);
			input_mt_report_slot_state(ftsdata->input_dev,
				MT_TOOL_FINGER, true);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_TOUCH_MAJOR, event->pressure);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_POSITION_X, event->au16_x[i]);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_POSITION_Y, event->au16_y[i]);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_PRESSURE, buf[8]);
			input_report_abs(ftsdata->input_dev,
				ABS_MT_DISTANCE, buf[8]);
			touchs |= BIT(event->au8_finger_id[i]);
			ftsdata->touchs |= BIT(event->au8_finger_id[i]);
		} else {
			uppoint++;
			input_mt_report_slot_state(ftsdata->input_dev,
				MT_TOOL_FINGER, false);
			ftsdata->touchs &= ~BIT(event->au8_finger_id[i]);
		}
	}

	if (unlikely(ftsdata->touchs ^ touchs)) {
		for (i = 0; i < FTS_MAX_POINTS; i++) {
			if (BIT(i) & (ftsdata->touchs ^ touchs)) {
				input_mt_slot(ftsdata->input_dev, i);
				input_mt_report_slot_state(ftsdata->input_dev,
					MT_TOOL_FINGER, false);
			}
		}
	}
	ftsdata->touchs = touchs;
	//FTS_DBG("touch_point=%d\n\n",event->touch_point);
	if (event->touch_point == uppoint)
		input_report_key(ftsdata->input_dev, BTN_TOUCH, 0);
	else
		input_report_key(ftsdata->input_dev,
			BTN_TOUCH, event->touch_point > 0);

	input_sync(ftsdata->input_dev);
}
#endif
/*******************************************************************************
 *  Name: fts_touch_irq_work
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void fts_touch_irq_work(struct work_struct *work)
{
	int ret = -1;
	struct fts_ts_data *fts_ts = NULL;

	fts_ts = container_of(work, struct fts_ts_data, touch_event_work);

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	if (fts_ts->bNeedReport) {
		imx_virtual_send_pagedown_events();
		fts_ts->bNeedReport = false;
	}
#endif

	ret = fts_read_Touchdata(fts_ts);
	if (ret == 0)
		fts_report_value(fts_ts);

	enable_irq(fts_ts->irq);
}
/*
 * reset gpio set to low
 */
static int fts_reset_gpio_low(struct fts_ts_data *ftsdata)
{
	int err = -1;

	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {

		msleep(ftsdata->pdata->hard_rst_dly);
		gpio_direction_output(ftsdata->pdata->reset_gpio, 1);
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 0);
		msleep(ftsdata->pdata->hard_rst_dly);
	} else {
		return err;
	}

	return 0;
}


/*******************************************************************************
 *  Name: fts_devm_gpio_request
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_devm_gpio_request(struct fts_ts_data *ftsdata)
{
	int err = -1;

	if (gpio_is_valid(ftsdata->pdata->irq_gpio)) {
		err = devm_gpio_request_one(ftsdata->dev,
			ftsdata->pdata->irq_gpio,
			GPIOF_DIR_IN,
			"fts_irq_gpio");
		if (err) {
			dev_info(&ftsdata->client->dev, "irq gpio request failed");
			goto err_out;
		}
	}

	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
		err = devm_gpio_request_one(ftsdata->dev,
			ftsdata->pdata->reset_gpio,
			GPIOF_DIR_OUT,
			"fts_reset_gpio");
		if (err) {
			dev_info(&ftsdata->client->dev, "reset gpio request failed");
			goto err_out;
		}
	}

err_out:
	return err;

}

/*******************************************************************************
 *  Name: fts_gpio_configure
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_gpio_configure(struct fts_ts_data *ftsdata, bool on)
{
	int err = -1;

	if (on) {
		if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
			msleep(ftsdata->pdata->hard_rst_dly);
			gpio_direction_output(ftsdata->pdata->reset_gpio, 1);
			gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 1);
			msleep(ftsdata->pdata->hard_rst_dly);
			gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 0);
			msleep(ftsdata->pdata->hard_rst_dly);
			gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 1);
		}
	} else {
		if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
			/*
			 * This is intended to save leakage current
			 * only. Even if the call(gpio_direction_input)
			 * fails, only leakage current will be more but
			 * functionality will not be affected.
			 */
			err = gpio_direction_input(ftsdata->pdata->reset_gpio);
			if (err) {
				dev_err(&ftsdata->client->dev,
				    "unable to set direction for gpio [%d]\n",
				    ftsdata->pdata->irq_gpio);
			}
		}
	}
	return 0;

}

/*******************************************************************************
 *  Name: fts_power_on
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_power_on(struct fts_ts_data *ftsdata, bool on)
{
	int rc = -1;

	if (on) {
		rc = regulator_enable(ftsdata->vdd);
		if (rc) {
			dev_err(&ftsdata->client->dev,
				"Regulator vdd enable failed rc=%d\n", rc);
			return rc;
		}

	} else {
		rc = regulator_disable(ftsdata->vdd);
		if (rc) {
			dev_err(&ftsdata->client->dev,
				"Regulator vdd disable failed rc=%d\n", rc);
			return rc;
		}

	}

	return rc;
}

/*******************************************************************************
 *  Name: fts_power_init
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_power_init(struct fts_ts_data *ftsdata, bool on)
{
	int rc;

	if (!on) {
		dev_err(&ftsdata->client->dev, "fts_power_init false\n");
		goto pwr_deinit;
	}

	ftsdata->vdd = regulator_get(&ftsdata->client->dev, "m2296_ldo8");
	if (IS_ERR(ftsdata->vdd)) {
		rc = PTR_ERR(ftsdata->vdd);
		dev_err(&ftsdata->client->dev,
			"Regulator get failed vdd rc=%d\n", rc);
		return rc;
	}

	if (regulator_count_voltages(ftsdata->vdd) > 0) {
		rc = regulator_set_voltage(ftsdata->vdd,
			FTS_VTG_MAX_UV, FTS_VTG_MAX_UV);
		if (rc) {
			dev_err(&ftsdata->client->dev,
				"Regulator set_vtg failed vdd rc=%d\n", rc);
			goto reg_vdd_put;
		}
	}

	return 0;

/*reg_vcc_i2c_put:
 *	regulator_put(ftsdata->vcc_i2c);
 *reg_vdd_set_vtg:
 *	if (regulator_count_voltages(ftsdata->vdd) > 0)
 *		regulator_set_voltage(ftsdata->vdd, 0, FTS_VTG_MAX_UV);
 */
reg_vdd_put:
	regulator_put(ftsdata->vdd);
	return rc;

pwr_deinit:
	if (regulator_count_voltages(ftsdata->vdd) > 0)
		regulator_set_voltage(ftsdata->vdd, 0, FTS_VTG_MAX_UV);

	regulator_put(ftsdata->vdd);

	//if (regulator_count_voltages(ftsdata->vcc_i2c) > 0)
	//	regulator_set_voltage(ftsdata->vcc_i2c, 0, FTS_I2C_VTG_MAX_UV);

	//regulator_put(ftsdata->vcc_i2c);
	return 0;
}

/*** power on/off the voltage **/
int fts_power_control(struct fts_ts_data *fts_data, bool on)
{
	int err = -1;

	if (fts_data->pdata->power_on) {
		err = fts_data->pdata->power_on(on);
		//FTS_DBG("err=%d\n",err);
		if (err < 0) {
			dev_err(fts_data->dev,
				"power control failed:%d\n", err);
			return err;
		}
	} else if (fts_data->pdata->power_regulator) {
		err = fts_power_on(fts_data, on);
		if (err < 0) {
			dev_err(fts_data->dev,
				"power control failed:%d\n", err);
			return err;
		}
	}

	return 0;
}


#if defined(CONFIG_USE_PINCTRL_GPIO)
/*******************************************************************************
 *  Name: fts_ts_pinctrl_init
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_pinctrl_init(struct fts_ts_data *fts_data)
{
	int retval;

	/* Get pinctrl if target uses pinctrl */
	fts_data->ts_pinctrl = devm_pinctrl_get(&(fts_data->client->dev));
	if (IS_ERR_OR_NULL(fts_data->ts_pinctrl)) {
		retval = PTR_ERR(fts_data->ts_pinctrl);
		dev_err(&fts_data->client->dev,
			"Target does not use pinctrl %d\n", retval);
		goto err_pinctrl_get;
	}

	fts_data->pinctrl_state_active =
	    pinctrl_lookup_state(fts_data->ts_pinctrl, PINCTRL_STATE_ACTIVE);
	if (IS_ERR_OR_NULL(fts_data->pinctrl_state_active)) {
		retval = PTR_ERR(fts_data->pinctrl_state_active);
		dev_err(&fts_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_ACTIVE, retval);
		goto err_pinctrl_lookup;
	}

	fts_data->pinctrl_state_suspend =
	    pinctrl_lookup_state(fts_data->ts_pinctrl, PINCTRL_STATE_SUSPEND);
	if (IS_ERR_OR_NULL(fts_data->pinctrl_state_suspend)) {
		retval = PTR_ERR(fts_data->pinctrl_state_suspend);
		dev_err(&fts_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_SUSPEND, retval);
		goto err_pinctrl_lookup;
	}

	fts_data->pinctrl_state_release =
	    pinctrl_lookup_state(fts_data->ts_pinctrl, PINCTRL_STATE_RELEASE);
	if (IS_ERR_OR_NULL(fts_data->pinctrl_state_release)) {
		retval = PTR_ERR(fts_data->pinctrl_state_release);
		dev_dbg(&fts_data->client->dev,
			"Can not lookup %s pinstate %d\n",
			PINCTRL_STATE_RELEASE, retval);
	}

	return 0;

err_pinctrl_lookup:
	devm_pinctrl_put(fts_data->ts_pinctrl);
err_pinctrl_get:
	fts_data->ts_pinctrl = NULL;
	return retval;
}
#endif

#ifdef CONFIG_PM_SLEEP
/*******************************************************************************
 *  Name: fts_ts_start
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_start(struct device *dev)
{
	struct fts_ts_data *ftsdata = dev_get_drvdata(dev);
	int err;
	fts_reset_gpio_low(ftsdata);

	err = fts_power_control(ftsdata, true);
	if (err < 0) {
		dev_err(dev, "power on failed\n");
		return err;
	}

#if defined(CONFIG_USE_PINCTRL_GPIO)
	if (ftsdata->ts_pinctrl) {
		err = pinctrl_select_state(ftsdata->ts_pinctrl,
			ftsdata->pinctrl_state_active);
		if (err < 0)
			dev_err(dev, "Cannot get active pinctrl state\n");
	}
#endif

	err = fts_gpio_configure(ftsdata, true);
	if (err < 0) {
		dev_err(&ftsdata->client->dev,
			"failed to put gpios in resue state\n");
		goto err_gpio_configuration;
	}

	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 0);
		msleep(ftsdata->pdata->hard_rst_dly);
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 1);
	}

	msleep(ftsdata->pdata->soft_rst_dly);

	if (ftsdata->irq > 0)
		enable_irq(ftsdata->irq);
	ftsdata->suspended = false;

	return 0;

err_gpio_configuration:
#if defined(CONFIG_USE_PINCTRL_GPIO)
	if (ftsdata->ts_pinctrl) {
		err = pinctrl_select_state(ftsdata->ts_pinctrl,
			ftsdata->pinctrl_state_suspend);
		if (err < 0)
			dev_err(dev, "Cannot get suspend pinctrl state\n");
	}
#endif

	err = fts_power_control(ftsdata, false);
	if (err < 0)
		dev_err(dev, "power off failed\n");
	return err;
}

/*******************************************************************************
 *  Name: fts_ts_stop
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_stop(struct device *dev)
{
	struct fts_ts_data *ftsdata = dev_get_drvdata(dev);
	char txbuf[2];
	int i, err;

	if (ftsdata->irq > 0)
		disable_irq(ftsdata->irq);

	/* release all touches */
	for (i = 0; i < ftsdata->pdata->num_max_touches; i++) {
		input_mt_slot(ftsdata->input_dev, i);
		input_mt_report_slot_state(ftsdata->input_dev,
			MT_TOOL_FINGER, 0);
	}
	input_mt_report_pointer_emulation(ftsdata->input_dev, false);
	input_sync(ftsdata->input_dev);

	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
		txbuf[0] = FTS_REG_PMODE;
		txbuf[1] = FTS_PMODE_HIBERNATE;
		fts_i2c_write(ftsdata->client, txbuf, sizeof(txbuf));
	}

	err = fts_power_control(ftsdata, false);
	if (err < 0) {
		dev_err(dev, "power off failed\n");
		goto pwr_off_fail;
	}


#if defined(CONFIG_USE_PINCTRL_GPIO)
	if (ftsdata->ts_pinctrl) {
		err = pinctrl_select_state(ftsdata->ts_pinctrl,
			ftsdata->pinctrl_state_suspend);
		//dev_err(dev, "[%s,%d],err=%d\n",__func__,__LINE__,err);
		if (err < 0) {
			dev_err(dev, "Cannot get suspend pinctrl state\n");
			goto gpio_configure_fail;
		}
	}
#endif


	err = fts_gpio_configure(ftsdata, false);
	if (err < 0) {
		dev_err(&ftsdata->client->dev,
			"failed to put gpios in suspend state\n");
		goto gpio_configure_fail;
	}

	ftsdata->suspended = true;

	return 0;

gpio_configure_fail:
#if defined(CONFIG_USE_PINCTRL_GPIO)
	if (ftsdata->ts_pinctrl) {
		err = pinctrl_select_state(ftsdata->ts_pinctrl,
			ftsdata->pinctrl_state_active);
		if (err < 0)
			dev_err(dev, "Cannot get active pinctrl state\n");
	}
#endif

	err = fts_power_control(ftsdata, true);
	if (err < 0)
		dev_err(dev, "power on failed\n");
pwr_off_fail:
	if (gpio_is_valid(ftsdata->pdata->reset_gpio)) {
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 0);
		msleep(ftsdata->pdata->hard_rst_dly);
		gpio_set_value_cansleep(ftsdata->pdata->reset_gpio, 1);
	}
	if (ftsdata->irq > 0)
		enable_irq(ftsdata->irq);
	return err;
}

/*******************************************************************************
 *  Name: fts_ts_suspend
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fts_ts_data *ftsdata = i2c_get_clientdata(client);
	//struct fts_ts_data *ftsdata = dev_get_drvdata(dev);
	//char txbuf[2];
	//char i;
	//int err;

#if FTS_GESTRUE_EN/** 0 **/
	fts_write_reg(client, 0xd0, 0x01);
	if (fts_updateinfo_curr.CHIP_ID == 0x54
	   || fts_updateinfo_curr.CHIP_ID == 0x58) {
		fts_write_reg(client, 0xd1, 0xff);
		fts_write_reg(client, 0xd2, 0xff);
		fts_write_reg(client, 0xd5, 0xff);
		fts_write_reg(client, 0xd6, 0xff);
		fts_write_reg(client, 0xd7, 0xff);
		fts_write_reg(client, 0xd8, 0xff);
	}

	ftsdata->suspended = true;

	return 0;
#endif

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	if (ftsdata->bEnableNoSuspended) {
		if (device_may_wakeup(dev)) {
			ftsdata->bEarlySuspended = true;
			enable_irq_wake(ftsdata->irq);
			return 0;
		}
	}
#endif

	if (ftsdata->loading_fw) {
		dev_info(dev, "Firmware loading in process...\n");
		return 0;
	}

	if (ftsdata->suspended) {
		dev_info(dev, "Already in suspend state\n");
		return 0;
	}

#if defined(CONFIG_SENSORS)
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support &&
		device_may_wakeup(dev) &&
		ftsdata->psensor_pdata->tp_psensor_opened) {

		err = enable_irq_wake(ftsdata->irq);
		if (err) {
			dev_err(&ftsdata->client->dev,
				"%s: set_irq_wake failed\n", __func__);
		}
		ftsdata->suspended = true;
		return err;
	}
#endif

	return fts_ts_stop(dev);
}

/*******************************************************************************
 *  Name: fts_ts_resume
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fts_ts_data *ftsdata = i2c_get_clientdata(client);
	//struct fts_ts_data *ftsdata = dev_get_drvdata(dev);
	int err;
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	if (ftsdata->bEnableNoSuspended) {
		if (device_may_wakeup(dev)) {
			ftsdata->bEarlySuspended = false;
			disable_irq_wake(ftsdata->irq);
			return 0;
		}
	}
#endif

	dev_info(dev, "<%s,%d>,ftsdata->suspended=%d\n",
		__func__, __LINE__, ftsdata->suspended);
	if (!ftsdata->suspended) {
		dev_dbg(dev, "Already in awake state\n");
		return 0;
	}

#if defined(CONFIG_SENSORS)
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support &&
		device_may_wakeup(dev) &&
		ftsdata->psensor_pdata->tp_psensor_opened) {
		err = disable_irq_wake(ftsdata->irq);
		if (err) {
			dev_err(&ftsdata->client->dev,
				"%s: disable_irq_wake failed\n", __func__);
		}
		ftsdata->suspended = false;
		return err;
	}
#endif

	err = fts_ts_start(dev);
	if (err < 0)
		return err;

	return 0;
}

static const struct dev_pm_ops fts_ts_pm_ops = {
//#if (!defined(CONFIG_FB) && !defined(CONFIG_HAS_EARLYSUSPEND))
	.suspend = fts_ts_suspend,
	.resume = fts_ts_resume,
//#endif
};

#else
/*******************************************************************************
 *  Name: fts_ts_suspend
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_suspend(struct device *dev)
{
	return 0;
}

/*******************************************************************************
 *  Name: fts_ts_resume
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_resume(struct device *dev)
{
	return 0;
}

#endif

#if defined(CONFIG_FB)
/*******************************************************************************
 *  Name: fb_notifier_callback
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *ftsdata)
{
	struct fb_event *evdata = ftsdata;
	int *blank;
	struct fts_ts_data *fts_data =
		container_of(self, struct fts_ts_data, fb_notif);

	if (evdata && evdata->data && event == FB_EVENT_BLANK &&
			fts_data && fts_data->client) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK)
			fts_ts_resume(&fts_data->client->dev);
		else if (*blank == FB_BLANK_POWERDOWN)
			fts_ts_suspend(&fts_data->client->dev);
	}

	return 0;
}
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
/*******************************************************************************
 *  Name: fts_ts_early_suspend
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void fts_ts_early_suspend(struct early_suspend *handler)
{
	struct fts_ts_data *ftsdata = container_of(handler,
						   struct fts_ts_data,
						   early_suspend);

	fts_ts_suspend(&ftsdata->client->dev);
}

/*******************************************************************************
 *  Name: fts_ts_late_resume
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void fts_ts_late_resume(struct early_suspend *handler)
{
	struct fts_ts_data *ftsdata = container_of(handler,
						   struct fts_ts_data,
						   early_suspend);

	fts_ts_resume(&ftsdata->client->dev);
}
#endif


#ifdef CONFIG_OF
/*******************************************************************************
 *  Name: fts_get_dt_coords
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_get_dt_coords(struct device *dev, char *name,
				struct fts_ts_platform_data *pdata)
{
	u32 coords[FTS_COORDS_ARR_SIZE];
	struct property *prop;
	struct device_node *np = dev->of_node;
	int coords_size, rc;

	prop = of_find_property(np, name, NULL);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;

	coords_size = prop->length / sizeof(u32);
	FTS_DBG("coords_size=%d\n", coords_size);
	if (coords_size != FTS_COORDS_ARR_SIZE) {
		dev_err(dev, "invalid %s\n", name);
		return -EINVAL;
	}

	rc = of_property_read_u32_array(np, name, coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read %s\n", name);
		return rc;
	}

	if (!strcmp(name, "focaltech,panel-coords")) {
		pdata->panel_minx = coords[0];
		pdata->panel_miny = coords[1];
		pdata->panel_maxx = coords[2];
		pdata->panel_maxy = coords[3];
		FTS_DBG(
	"panel_minx = %d,panel_miny = %d,panel_maxx = %d,panel_maxy = %d\n",
			pdata->panel_minx,
			pdata->panel_miny,
			pdata->panel_maxx,
			pdata->panel_maxy);
	} else if (!strcmp(name, "focaltech,display-coords")) {
		pdata->x_min = coords[0];
		pdata->y_min = coords[1];
		pdata->x_max = coords[2];
		pdata->y_max = coords[3];
		FTS_DBG("x_min = %d,y_min = %d,x_max = %d,y_max = %d\n",
			pdata->x_min, pdata->y_min, pdata->x_max, pdata->y_max);
	} else {
		dev_err(dev, "unsupported property %s\n", name);
		return -EINVAL;
	}

	return 0;
}

/*******************************************************************************
 *  Name: fts_parse_dt
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_parse_dt(struct device *dev, struct fts_ts_platform_data *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 temp_val, num_buttons;
	u32 button_map[MAX_BUTTONS];

	pdata->name = "focaltech";
	rc = of_property_read_string(np, "focaltech,name", &pdata->name);
	FTS_DBG("pdata->name = %s,\n", pdata->name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read name\n");
		return rc;
	}

	rc = fts_get_dt_coords(dev, "focaltech,panel-coords", pdata);
	if (rc && (rc != -EINVAL))
		return rc;

	rc = fts_get_dt_coords(dev, "focaltech,display-coords", pdata);
	if (rc)
		return rc;

	pdata->i2c_pull_up = of_property_read_bool(np, "focaltech,i2c-pull-up");
	FTS_DBG("pdata->i2c_pull_up=%d\n", pdata->i2c_pull_up);

	pdata->no_force_update = of_property_read_bool(np,
		"focaltech,no-force-update");
	FTS_DBG("pdata->no_force_update=%d\n", pdata->no_force_update);
	/* reset, irq gpio info */
	pdata->reset_gpio = of_get_named_gpio_flags(np,
		"focaltech,reset-gpio", 0, &pdata->reset_gpio_flags);
	FTS_DBG("reset_gpio = %d\n", pdata->reset_gpio);
	if (pdata->reset_gpio < 0)
		return pdata->reset_gpio;

	pdata->irq_gpio = of_get_named_gpio_flags(np, "focaltech,irq-gpio", 0,
			&pdata->irq_gpio_flags);
	FTS_DBG("irq_gpio = %d\n", pdata->irq_gpio);
	if (pdata->irq_gpio < 0)
		return pdata->irq_gpio;

#if defined(CONFIG_TOUCH_I2C_WACOM)
	pdata->wacom_status = of_get_named_gpio(np, "wacom,pdctb", 0);
#endif

	pdata->fw_name = "ft_fw.bin";
	rc = of_property_read_string(np, "focaltech,fw-name", &pdata->fw_name);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw name\n");
		return rc;
	}

	rc = of_property_read_u32(np, "focaltech,group-id", &temp_val);
	if (!rc)
		pdata->group_id = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,hard-reset-delay-ms",
		&temp_val);
	if (!rc)
		pdata->hard_rst_dly = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,soft-reset-delay-ms",
		&temp_val);
	if (!rc)
		pdata->soft_rst_dly = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,num-max-touches", &temp_val);
	if (!rc)
		pdata->num_max_touches = temp_val;
	else
		return rc;

	rc = of_property_read_u32(np, "focaltech,fw-delay-aa-ms", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay aa\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.delay_aa =  temp_val;
	}

	rc = of_property_read_u32(np, "focaltech,fw-delay-55-ms", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay 55\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.delay_55 =  temp_val;
	}

	rc = of_property_read_u32(np, "focaltech,fw-upgrade-id1", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw upgrade id1\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.upgrade_id_1 =  temp_val;
	}

	rc = of_property_read_u32(np, "focaltech,fw-upgrade-id2", &temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw upgrade id2\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.upgrade_id_2 =  temp_val;
	}

	rc = of_property_read_u32(np, "focaltech,fw-delay-readid-ms",
		&temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay read id\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.delay_readid =  temp_val;
	}

	rc = of_property_read_u32(np, "focaltech,fw-delay-era-flsh-ms",
		&temp_val);
	if (rc && (rc != -EINVAL)) {
		dev_err(dev, "Unable to read fw delay erase flash\n");
		return rc;
	} else if (rc != -EINVAL) {
		pdata->info.delay_erase_flash =  temp_val;
	}

	pdata->info.AUTO_CLB = of_property_read_bool(np,
		"focaltech,fw-auto-cal");

	pdata->fw_vkey_support = of_property_read_bool(np,
		"focaltech,fw-vkey-support");

	pdata->ignore_id_check = of_property_read_bool(np,
		"focaltech,ignore-id-check");

	pdata->psensor_support = of_property_read_bool(np,
		"focaltech,psensor-support");

	pdata->power_regulator = of_property_read_bool(np,
		"focaltech,power-regulator-support");

	FTS_DBG("AUTO_CLB=%d,fw_vkey_support =%d,ignore_id_check = %d,psensor_support=%d,power_regulator=%d\n",
		pdata->info.AUTO_CLB,
		pdata->fw_vkey_support,
		pdata->ignore_id_check,
		pdata->psensor_support,
		pdata->power_regulator);
	rc = of_property_read_u32(np, "focaltech,family-id", &temp_val);
	if (!rc)
		pdata->family_id = temp_val;
	else
		return rc;

	prop = of_find_property(np, "focaltech,button-map", NULL);
	if (prop) {
		num_buttons = prop->length / sizeof(temp_val);
		if (num_buttons > MAX_BUTTONS)
			return -EINVAL;

		rc = of_property_read_u32_array(np, "focaltech,button-map",
						button_map, num_buttons);
		if (rc) {
			dev_err(dev, "Unable to read key codes\n");
			return rc;
		}
	}

	return 0;
}
#endif

/*******************************************************************************
 *  Name: fts_debug_addr_is_valid
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static bool fts_debug_addr_is_valid(int addr)
{
	if (addr < 0 || addr > 0xFF) {
		pr_err("FT reg address is invalid: 0x%x\n", addr);
		return false;
	}

	return true;
}

/*******************************************************************************
 *  Name: fts_debug_data_set
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_data_set(void *_data, u64 val)
{
	struct fts_ts_data *ftsdata = _data;

	mutex_lock(&ftsdata->input_dev->mutex);

	if (fts_debug_addr_is_valid(ftsdata->addr))
		dev_info(&ftsdata->client->dev,
			"Writing into FT registers not supported\n");

	mutex_unlock(&ftsdata->input_dev->mutex);

	return 0;
}

/*******************************************************************************
 *  Name: fts_debug_data_get
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_data_get(void *_data, u64 *val)
{
	struct fts_ts_data *ftsdata = _data;
	int rc;
	u8 reg;

	mutex_lock(&ftsdata->input_dev->mutex);

	if (fts_debug_addr_is_valid(ftsdata->addr)) {
		rc = fts_read_reg(ftsdata->client, ftsdata->addr, &reg);
		if (rc < 0)
			dev_err(&ftsdata->client->dev,
				"FT read register 0x%x failed (%d)\n",
				ftsdata->addr, rc);
		else
			*val = reg;
	}

	mutex_unlock(&ftsdata->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_data_fops, fts_debug_data_get,
	fts_debug_data_set, "0x%02llX\n");

/*******************************************************************************
 *  Name: fts_debug_addr_set
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_addr_set(void *_data, u64 val)
{
	struct fts_ts_data *ftsdata = _data;

	if (fts_debug_addr_is_valid(val)) {
		mutex_lock(&ftsdata->input_dev->mutex);
		ftsdata->addr = val;
		mutex_unlock(&ftsdata->input_dev->mutex);
	}

	return 0;
}

/*******************************************************************************
 *  Name: fts_debug_addr_get
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_addr_get(void *_data, u64 *val)
{
	struct fts_ts_data *ftsdata = _data;

	mutex_lock(&ftsdata->input_dev->mutex);

	if (fts_debug_addr_is_valid(ftsdata->addr))
		*val = ftsdata->addr;

	mutex_unlock(&ftsdata->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_addr_fops, fts_debug_addr_get,
	fts_debug_addr_set, "0x%02llX\n");

/*******************************************************************************
 *  Name: fts_debug_suspend_set
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_suspend_set(void *_data, u64 val)
{
	struct fts_ts_data *ftsdata = _data;

	mutex_lock(&ftsdata->input_dev->mutex);

	if (val)
		fts_ts_suspend(&ftsdata->client->dev);
	else
		fts_ts_resume(&ftsdata->client->dev);

	mutex_unlock(&ftsdata->input_dev->mutex);

	return 0;
}

/*******************************************************************************
 *  Name: fts_debug_suspend_get
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_debug_suspend_get(void *_data, u64 *val)
{
	struct fts_ts_data *ftsdata = _data;

	mutex_lock(&ftsdata->input_dev->mutex);
	*val = ftsdata->suspended;
	mutex_unlock(&ftsdata->input_dev->mutex);

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debug_suspend_fops, fts_debug_suspend_get,
	fts_debug_suspend_set, "%lld\n");

/*******************************************************************************
 *  Name: fts_debug_dump_info
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
int fts_debug_dump_info(struct seq_file *m, void *v)
{
	struct fts_ts_data *ftsdata = m->private;

	seq_printf(m, "%s\n", ftsdata->ts_info);

	return 0;
}

/*******************************************************************************
 *  Name: debugfs_dump_info_open
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int debugfs_dump_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, fts_debug_dump_info, inode->i_private);
}

static const struct file_operations debug_dump_info_fops = {
	.owner		= THIS_MODULE,
	.open		= debugfs_dump_info_open,
	.read		= seq_read,
	.release	= single_release,
};

/*******************************************************************************
 *  Name: fts_ts_probe
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct fts_ts_platform_data *pdata;
#if defined(CONFIG_SENSORS)
	struct fts_psensor_platform_data *psensor_pdata;
	struct input_dev *psensor_input_dev;
#endif
	struct fts_ts_data *ftsdata;
	struct input_dev *input_dev;
	struct dentry *temp;
	//struct device_node *node;
	u8 reg_value = 0;
	u8 reg_addr = 0;
	int err = 0, len = 0;
	int icount = 0;
#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
	int point_num;
#endif
#ifdef CONFIG_OF
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct fts_ts_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = fts_parse_dt(&client->dev, pdata);
		if (err) {
			dev_err(&client->dev, "DT parsing failed\n");
			return err;
		}
	} else {
		pdata = client->dev.platform_data;
	}
#else
	/*if we didnot supported the device tree,
	 * the pdata would from board files
	 */
	pdata = client->dev.platform_data;

#endif

	if (!pdata) {
		dev_err(&client->dev, "Invalid pdata\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		return -ENODEV;
	}

	ftsdata = devm_kzalloc(&client->dev, sizeof(struct fts_ts_data),
			GFP_KERNEL);
	if (!ftsdata) {
		dev_err(&client->dev, "Not enough memory\n");
		return -ENOMEM;
	}

	//fts_wq_data = ftsdata;

	FTS_DBG("fw_name=%s\n", pdata->fw_name);
	if (pdata->fw_name) {
		len = strlen(pdata->fw_name);
		FTS_DBG("len =%d\n", len);
		if (len > FTS_FW_NAME_MAX_LEN - 1) {
			dev_err(&client->dev, "Invalid firmware name\n");
			return -EINVAL;
		}

		strlcpy(ftsdata->fw_name, pdata->fw_name, len + 1);
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "failed to allocate input device\n");
		return -ENOMEM;
	}

	ftsdata->input_dev = input_dev;
	ftsdata->client = client;
	ftsdata->pdata = pdata;
	ftsdata->dev = &client->dev;
#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	ftsdata->bEarlySuspended = false;
	ftsdata->bNeedReport = false;
	ftsdata->bEnableNoSuspended = false;
	ftsdata->bInTouch = false;
#endif

	input_dev->name = "fts_ts";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	input_set_drvdata(input_dev, ftsdata);
	i2c_set_clientdata(client, ftsdata);

#if defined(CONFIG_FOCALTECH_EREA_REPORT_SUPPORT)
	__set_bit(EV_SYN, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, pdata->num_max_touches, INPUT_MT_DIRECT);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
		pdata->x_min, pdata->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
		pdata->y_min, pdata->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 0xFF, 0, 0);

	point_num = pdata->num_max_touches;
	ftsdata->pnt_buf_size = point_num * FTS_ONE_TCH_LEN + 3;
	ftsdata->point_buf = (u8 *)kzalloc(ftsdata->pnt_buf_size, GFP_KERNEL);
	if (!ftsdata->point_buf) {
		dev_err(&client->dev,
			"failed to alloc memory for point buf!\n");
		err = -ENOMEM;
		goto unreg_inputdev;
	}

	ftsdata->events =
(struct ts_event *)kzalloc(point_num * sizeof(struct ts_event), GFP_KERNEL);
	if (!ftsdata->events) {
		dev_err(&client->dev,
			"failed to alloc memory for point events!\n");
		err = -ENOMEM;
		goto unreg_inputdev;
	}

#else /* pair with CONFIG_FOCALTECH_EREA_REPORT_SUPPORT */

	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(EV_ABS, input_dev->evbit);
	__set_bit(BTN_TOUCH, input_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_mt_init_slots(input_dev, pdata->num_max_touches, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
		pdata->x_min, pdata->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
		pdata->y_min, pdata->y_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_DISTANCE, 0, 255, 0, 0);
#endif /* pair with CONFIG_FOCALTECH_EREA_REPORT_SUPPORT */

	err = fts_devm_gpio_request(ftsdata);//request gpio
	if (err < 0) {
		dev_info(&client->dev, "Failed to request the gpios\n");
		goto free_gpio;
	}
	/* wen :2017-o6-12 16:51
	 * IC REQUEST:
	 * reset low-->power on --> reset high-->read/write
	 * 1. reset low
	 */
	err = fts_reset_gpio_low(ftsdata);
	if (err < 0) {
		dev_err(&client->dev, "Failed to configure the gpios\n");
		goto free_gpio;
	}
	/*
	 * 2.power init and on
	 */
	if (pdata->power_init) {
		err = pdata->power_init(true);
		//FTS_DBG("err=%d\n",err);
		if (err < 0) {
			dev_err(&client->dev, "pdata->power_init power init failed");
			goto unreg_inputdev;
		}
	} else if (pdata->power_regulator) {
		err = fts_power_init(ftsdata, true);
		if (err < 0) {
			dev_err(&client->dev, "fts_power_init power init failed");
			goto unreg_inputdev;
		}
	}

	err = fts_power_control(ftsdata, true);
	if (err < 0) {
		dev_err(&client->dev, "power on failed\n");
		goto pwr_deinit;
	}

#if defined(CONFIG_USE_PINCTRL_GPIO)
	err = fts_ts_pinctrl_init(ftsdata);
	//dev_err(&client->dev,"[%s,%d],err=%d\n",__func__,__LINE__,err);
	if (!err && ftsdata->ts_pinctrl) {
		/*
		 * Pinctrl handle is optional. If pinctrl handle is found
		 * let pins to be configured in active state. If not
		 * found continue further without error.
		 */
		err = pinctrl_select_state(ftsdata->ts_pinctrl,
			ftsdata->pinctrl_state_active);
		if (err < 0) {
			dev_err(&client->dev,
				"failed to select pin to active state");
		}
	}
#endif
	/*
	 * 3.reset high
	 */
	err = fts_gpio_configure(ftsdata, true);
	if (err < 0) {
		dev_err(&client->dev, "Failed to configure the gpios\n");
		goto free_gpio;
	}

	/*
	 * 4.read register.
	 *   i2c test
	 * check the controller id
	 */
	do {
		reg_addr = FTS_REG_ID;

		msleep(100);
		err = fts_i2c_read(client, &reg_addr, 1, &reg_value, 1);
		if (err < 0) {
			dev_err(&client->dev, "version read failed\n");
			break;
		}

		if ((reg_value == 0x54) || (reg_value == 0x58))
			break;

		msleep(100);
		icount++;
	} while (icount <= 4);

	if (err < 0) {
		dev_info(&client->dev, "I2C test Failed:%d\n", err);
		//free all pins
		fts_gpio_configure(ftsdata, false);
		//then power off the voltage
		fts_power_control(ftsdata, false);
		return -ENOMEM;
	}

	dev_info(&client->dev, "Device ID = 0x%x\n", reg_value);


	/* make sure CTP already finish startup process */
	msleep(ftsdata->pdata->soft_rst_dly);

	INIT_WORK(&ftsdata->touch_event_work, fts_touch_irq_work);
	ftsdata->ts_workqueue = create_workqueue(FTS_WORKQUEUE_NAME);
	if (!ftsdata->ts_workqueue) {
		err = -ESRCH;
		devm_kfree(&client->dev, ftsdata);
		goto exit_create_singlethread;
	}

	err = input_register_device(input_dev);
	if (err) {
		dev_err(&client->dev, "Input device registration failed\n");
		devm_kfree(&client->dev, ftsdata);
		goto free_inputdev;
	}

	ftsdata->family_id = pdata->family_id;

	fts_i2c_client = client;
	fts_input_dev = input_dev;
	//fts_wq_data = ftsdata;

	fts_get_upgrade_array(ftsdata);

#if 0//def CONFIG_OF
	//node = of_find_compatible_node(NULL, NULL, "focaltech,fts");
	FTS_DBG("node->full_name=%s,name=%s\n",
		client->dev.of_node->full_name, client->dev.of_node->name);
	ftsdata->irq = irq_of_parse_and_map(client->dev.of_node, 0);
#else
	if (gpio_is_valid(ftsdata->pdata->irq_gpio))
		ftsdata->irq = gpio_to_irq(ftsdata->pdata->irq_gpio);
	else
		ftsdata->irq = -1;
#endif
	FTS_DBG("irq=%d\n", ftsdata->irq);

	if (ftsdata->irq >= 0) {
		err = request_threaded_irq(ftsdata->irq, NULL,
		fts_ts_interrupt,
		pdata->irqflags | IRQF_ONESHOT | IRQF_TRIGGER_FALLING
		| IRQF_NO_SUSPEND | IRQF_EARLY_RESUME,
		client->dev.driver->name, ftsdata);

		FTS_DBG("err =%d\n", err);
		if (err) {
			dev_err(&client->dev, "request irq failed\n");
			devm_kfree(&client->dev, ftsdata);
			goto free_gpio;
		}

		disable_irq(ftsdata->irq);
	}

#if defined(CONFIG_SENSORS)
	FTS_DBG("psensor_support=%d,pdata->psensor_support=%d\n",
		fts_psensor_support_enabled(), ftsdata->pdata->psensor_support);
	/*
	 * ftsdata->pdata->psensor_support set to false for test first,
	 * if Failed, we try to other method. wen
	 */
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support) {
		device_init_wakeup(&client->dev, 1);
		psensor_pdata = devm_kzalloc(&client->dev,
				sizeof(struct ft5x06_psensor_platform_data),
				GFP_KERNEL);
		if (!psensor_pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			goto irq_free;
		}
		ftsdata->psensor_pdata = psensor_pdata;

		psensor_input_dev = input_allocate_device();
		if (!psensor_input_dev) {
			dev_err(&ftsdata->client->dev,
				"Failed to allocate device\n");
			goto free_psensor_pdata;
		}

		__set_bit(EV_ABS, psensor_input_dev->evbit);
		input_set_abs_params(psensor_input_dev,
					ABS_DISTANCE, 0, 1, 0, 0);
		psensor_input_dev->name = "proximity";
		psensor_input_dev->id.bustype = BUS_I2C;
		psensor_input_dev->dev.parent = &ftsdata->client->dev;
		ftsdata->psensor_pdata->input_psensor_dev = psensor_input_dev;

		err = input_register_device(psensor_input_dev);
		if (err) {
			dev_err(&ftsdata->client->dev,
				"Unable to register device, err=%d\n", err);
			goto free_psensor_input_dev;
		}

		psensor_pdata->ps_cdev = sensors_proximity_cdev;
		psensor_pdata->ps_cdev.sensors_enable =
					fts_psensor_enable_set;
		psensor_pdata->ftsdata = ftsdata;

		err = sensors_classdev_register(&client->dev,
						&psensor_pdata->ps_cdev);
		if (err)
			goto unregister_psensor_input_device;
	}
#endif

	ftsdata->dir = debugfs_create_dir(FTS_DEBUG_DIR_NAME, NULL);
	if (ftsdata->dir == NULL || IS_ERR(ftsdata->dir)) {
		pr_err("debugfs_create_dir failed(%ld)\n",
			PTR_ERR(ftsdata->dir));
		err = PTR_ERR(ftsdata->dir);
	}

	temp = debugfs_create_file("addr", 0600, ftsdata->dir, ftsdata,
				   &debug_addr_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		devm_kfree(&client->dev, ftsdata);
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("ftsdata", 0600, ftsdata->dir, ftsdata,
				   &debug_data_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		devm_kfree(&client->dev, ftsdata);
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("suspend", 0600, ftsdata->dir,
					ftsdata, &debug_suspend_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		devm_kfree(&client->dev, ftsdata);
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	temp = debugfs_create_file("dump_info", 0600, ftsdata->dir,
					ftsdata, &debug_dump_info_fops);
	if (temp == NULL || IS_ERR(temp)) {
		pr_err("debugfs_create_file failed: rc=%ld\n", PTR_ERR(temp));
		devm_kfree(&client->dev, ftsdata);
		err = PTR_ERR(temp);
		goto free_debug_dir;
	}

	ftsdata->ts_info = devm_kzalloc(&client->dev,
				FTS_INFO_MAX_LEN, GFP_KERNEL);
	if (!ftsdata->ts_info) {
		dev_err(&client->dev, "Not enough memory\n");
		devm_kfree(&client->dev, ftsdata);
		goto free_debug_dir;
	}

	/*get some register information */
	reg_addr = FTS_REG_POINT_RATE;
	fts_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		dev_err(&client->dev, "report rate read failed");

	dev_info(&client->dev, "report rate = %dHz\n", reg_value * 10);

	reg_addr = FTS_REG_THGROUP;
	err = fts_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		dev_err(&client->dev, "threshold read failed");

	dev_dbg(&client->dev, "touch threshold = %d\n", reg_value * 4);

	fts_update_fw_ver(ftsdata);
	fts_update_fw_vendor_id(ftsdata);

	FTS_STORE_TS_INFO(ftsdata->ts_info,
		ftsdata->family_id, ftsdata->pdata->name,
		ftsdata->pdata->num_max_touches,
		ftsdata->pdata->group_id,
		ftsdata->pdata->fw_vkey_support ? "yes" : "no",
		ftsdata->pdata->fw_name,
		ftsdata->fw_ver[0],
		ftsdata->fw_ver[1],
		ftsdata->fw_ver[2]);

	#ifdef FTS_APK_DEBUG
		fts_create_apk_debug_channel(client);
	#endif

	#ifdef FTS_SYSFS_DEBUG
		fts_create_sysfs(client);
	#endif

	#ifdef FTS_AUTO_UPGRADE
		fts_ctpm_auto_upgrade(client);
	#endif

	#ifdef FTS_CTL_IIC
		if (fts_rw_iic_drv_init(client) < 0) {
			dev_err(&client->dev,
			"%s:[FTS] create fts control iic driver failed\n",
			__func__);
		}
	#endif

	#if FTS_GESTRUE_EN
		fts_Gesture_init(input_dev);
		init_para(1872, 1408, 0, 0, 0);
	#endif

#if defined(CONFIG_FB)
	ftsdata->fb_notif.notifier_call = fb_notifier_callback;

	err = fb_register_client(&ftsdata->fb_notif);

	if (err)
		dev_err(&client->dev,
			"Unable to register fb_notifier: %d\n", err);
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
	ftsdata->early_suspend.level =
		EARLY_SUSPEND_LEVEL_BLANK_SCREEN + FTS_SUSPEND_LEVEL;
	ftsdata->early_suspend.suspend = fts_ts_early_suspend;
	ftsdata->early_suspend.resume = fts_ts_late_resume;
	register_early_suspend(&ftsdata->early_suspend);
#endif

	if (ftsdata->irq > 0) {
		enable_irq(ftsdata->irq);
		//enable_irq_wake(ftsdata->irq);
	}

#if defined(CONFIG_MANUAL_VIRTUAL_POWER_KEY)
	device_init_wakeup(ftsdata->dev, 1);
#endif

	return 0;

free_debug_dir:
	debugfs_remove_recursive(ftsdata->dir);
#if defined(CONFIG_SENSORS)
free_psensor_class_sysfs:
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support)
		sensors_classdev_unregister(&psensor_pdata->ps_cdev);
unregister_psensor_input_device:
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support)
		input_unregister_device(
			ftsdata->psensor_pdata->input_psensor_dev);
free_psensor_input_dev:
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support)
		input_free_device(ftsdata->psensor_pdata->input_psensor_dev);
free_psensor_pdata:
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support) {
		devm_kfree(&client->dev, psensor_pdata);
		ftsdata->psensor_pdata = NULL;
	}
irq_free:
	if ((fts_psensor_support_enabled()
		&& ftsdata->pdata->psensor_support)){
		device_init_wakeup(&client->dev, 0);
	}
	if (ftsdata->irq > 0)
		free_irq(ftsdata->irq, ftsdata);
#endif
free_gpio:
	if (gpio_is_valid(pdata->reset_gpio))
		gpio_free(pdata->reset_gpio);
	if (gpio_is_valid(pdata->irq_gpio))
		gpio_free(pdata->irq_gpio);
exit_create_singlethread:
	FTS_ERR("==singlethread error =\n");
	i2c_set_clientdata(client, NULL);
	//kfree(ftsdata);
	devm_kfree(&client->dev, ftsdata);
pwr_deinit:
	if (pdata->power_init)
		pdata->power_init(false);
	else if (pdata->power_regulator)
		fts_power_init(ftsdata, false);
unreg_inputdev:
	input_unregister_device(input_dev);
	input_dev = NULL;
free_inputdev:
	input_free_device(input_dev);
	return err;
}

/*******************************************************************************
 *  Name: fts_ts_remove
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int fts_ts_remove(struct i2c_client *client)
{
	struct fts_ts_data *ftsdata = i2c_get_clientdata(client);

	cancel_work_sync(&ftsdata->touch_event_work);
	destroy_workqueue(ftsdata->ts_workqueue);

	debugfs_remove_recursive(ftsdata->dir);

#if defined(CONFIG_SENSORS)
	if (fts_psensor_support_enabled() && ftsdata->pdata->psensor_support) {

		device_init_wakeup(&client->dev, 0);
		sensors_classdev_unregister(&ftsdata->psensor_pdata->ps_cdev);
	    input_unregister_device(ftsdata->psensor_pdata->input_psensor_dev);
		devm_kfree(&client->dev, ftsdata->psensor_pdata);
		ftsdata->psensor_pdata = NULL;
	}
#endif

#ifdef FTS_APK_DEBUG
		fts_release_apk_debug_channel();
#endif

#ifdef FTS_SYSFS_DEBUG
		fts_remove_sysfs(client);
#endif


#ifdef FTS_CTL_IIC
		fts_rw_iic_drv_exit();
#endif


#if defined(CONFIG_FB)
	if (fb_unregister_client(&ftsdata->fb_notif))
		dev_err(&client->dev, "Error occurred while unregistering fb_notifier.\n");
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ftsdata->early_suspend);
#endif
	free_irq(client->irq, ftsdata);

	if (gpio_is_valid(ftsdata->pdata->reset_gpio))
		gpio_free(ftsdata->pdata->reset_gpio);

	if (gpio_is_valid(ftsdata->pdata->irq_gpio))
		gpio_free(ftsdata->pdata->irq_gpio);

	fts_power_control(ftsdata, false);

	if (ftsdata->pdata->power_init)
		ftsdata->pdata->power_init(false);
	else if (ftsdata->pdata->power_regulator)
		fts_power_init(ftsdata, false);

	input_unregister_device(ftsdata->input_dev);

	return 0;
}

static void fts_ts_shutdown(struct i2c_client *client)
{
	struct fts_ts_data *ftsdata = i2c_get_clientdata(client);
	int err = -1;

	if (gpio_is_valid(ftsdata->pdata->reset_gpio))
		gpio_free(ftsdata->pdata->reset_gpio);

	if (gpio_is_valid(ftsdata->pdata->irq_gpio))
		gpio_free(ftsdata->pdata->irq_gpio);

	err = fts_power_control(ftsdata, false);
	if (err < 0) {
		dev_err(&client->dev, "power off failed\n");
		return;
	}
}

static const struct i2c_device_id fts_ts_id[] = {
	{FOCALTECH_I2C_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, fts_ts_id);

#ifdef CONFIG_OF
static const struct of_device_id fts_match_table[] = {
	{ .compatible = "focaltech,fts",},
	{ },
};
#endif

static struct i2c_driver fts_ts_driver = {
	.probe = fts_ts_probe,
	.remove = fts_ts_remove,
	.shutdown = fts_ts_shutdown,
	.driver = {
		.name = FOCALTECH_I2C_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = fts_match_table,
#endif
#ifdef CONFIG_PM_SLEEP
		.pm = &fts_ts_pm_ops,
#endif
	},
	.id_table = fts_ts_id,
};

/*******************************************************************************
 *  Name: fts_ts_init
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static int __init fts_ts_init(void)
{
	return i2c_add_driver(&fts_ts_driver);
}

/*******************************************************************************
 *  Name: fts_ts_exit
 *  Brief:
 *  Input:
 *  Output:
 *  Return:
 ******************************************************************************/
static void __exit fts_ts_exit(void)
{
	i2c_del_driver(&fts_ts_driver);
}

late_initcall(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_DESCRIPTION("FocalTech fts TouchScreen driver");
MODULE_LICENSE("GPL v2");

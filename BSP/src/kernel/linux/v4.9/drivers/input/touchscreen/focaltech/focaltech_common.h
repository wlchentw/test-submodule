/*************************************************************************
 *	> File Name: focaltech_common.h
 *	> Author:
 *	> Mail:
 *	> Created Time: 2018年08月18日 星期六 18时48分24秒
 ************************************************************************/

#ifndef _FOCALTECH_COMMON_H
#define _FOCALTECH_COMMON_H
/*******************************************************************************
 * Private constant and macro definitions using #define
 ******************************************************************************/
#define FTS_META_REGS		3
#define FTS_ONE_TCH_LEN		6
#define FTS_TCH_LEN(x)		(FTS_META_REGS + FTS_ONE_TCH_LEN * x)

#define FTS_PRESS		0x7F
#define FTS_MAX_ID		0x0F
#define FTS_TOUCH_X_H_POS	3
#define FTS_TOUCH_X_L_POS	4
#define FTS_TOUCH_Y_H_POS	5
#define FTS_TOUCH_Y_L_POS	6
#define FTS_TOUCH_PRE_POS   7
#define FTS_TOUCH_AREA_POS  8
#define FTS_TOUCH_POINT_NUM		2
#define FTS_TOUCH_EVENT_POS	3
#define FTS_TOUCH_ID_POS		5

#define FTS_TOUCH_DOWN		0
#define FTS_TOUCH_UP		1
#define FTS_TOUCH_CONTACT	2
#define EVENT_DOWN(flag) \
	((flag == FTS_TOUCH_DOWN) || (flag == FTS_TOUCH_CONTACT))
#define EVENT_UP(flag)                      (flag == FTS_TOUCH_UP)
#define EVENT_NO_DOWN(data)                 (!data->point_num)
#define KEY_EN(data)                        (data->pdata->have_key)
#define TOUCH_IS_KEY(y, key_y)              (y == key_y)
#define TOUCH_IN_RANGE(val, key_val, half) \
	((val > (key_val - half)) && (val < (key_val + half)))
#define TOUCH_IN_KEY(x, key_x)  TOUCH_IN_RANGE(x, key_x, FTS_KEY_WIDTH)

#define POINT_READ_BUF	(3 + FTS_ONE_TCH_LEN * FTS_MAX_POINTS)

/*register address*/
#define FTS_REG_DEV_MODE		0x00
#define FTS_DEV_MODE_REG_CAL	0x02

#define FTS_REG_PMODE		0xA5

#define FTS_REG_POINT_RATE	0x88
#define FTS_REG_THGROUP		0x80

/* power register bits*/
#define FTS_PMODE_ACTIVE		0x00
#define FTS_PMODE_MONITOR	0x01
#define FTS_PMODE_STANDBY	0x02
#define FTS_PMODE_HIBERNATE	0x03

#define FTS_STATUS_NUM_TP_MASK	0x0F

#define FTS_VTG_MIN_UV		2600000
#define FTS_VTG_MAX_UV	    3000000//2800000/*3300000*/
#define FTS_I2C_VTG_MIN_UV		1800000
#define FTS_I2C_VTG_MAX_UV		1800000

#define FTS_COORDS_ARR_SIZE	4
#define MAX_BUTTONS		4

#define FTS_8BIT_SHIFT		8
#define FTS_4BIT_SHIFT		4

/* psensor register address*/
#define FTS_REG_PSENSOR_ENABLE	0xB0
#define FTS_REG_PSENSOR_STATUS	0x01

/* psensor register bits*/
#define FTS_PSENSOR_ENABLE_MASK	0x01
#define FTS_PSENSOR_STATUS_NEAR	0xC0
#define FTS_PSENSOR_STATUS_FAR	0xE0
#define FTS_PSENSOR_FAR_TO_NEAR	0
#define FTS_PSENSOR_NEAR_TO_FAR	1
#define FTS_PSENSOR_ORIGINAL_STATE_FAR	1
#define FTS_PSENSOR_WAKEUP_TIMEOUT	500

#define PINCTRL_STATE_ACTIVE	"pmx_ts_active"
#define PINCTRL_STATE_SUSPEND	"pmx_ts_suspend"
#define PINCTRL_STATE_RELEASE	"pmx_ts_release"
#endif

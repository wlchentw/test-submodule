// SPDX-License-Identifier: MediaTekProprietary
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
#ifndef __HWTCON_IOCTL_CMD_H__
#define __HWTCON_IOCTL_CMD_H__

#include <linux/types.h>

/* HWTCON_FLAG_xx */
#define HWTCON_FLAG_USE_DITHERING_Y1		0x2000
#define HWTCON_FLAG_USE_DITHERING_Y4		0x4000
#define HWTCON_FLAG_FORCE_A2_OUTPUT		0x10

/* temperature use sensor. */
#define TEMP_USE_SENSOR			0x100000

#define UPDATE_MODE_PARTIAL			0x0
#define UPDATE_MODE_FULL			0x1

enum WAVEFORM_MODE_ENUM {
	WAVEFORM_MODE_INIT = 0,
	WAVEFORM_MODE_DU = 1,
	WAVEFORM_MODE_GC16 = 2,
	WAVEFORM_MODE_GL16 = 3,
	WAVEFORM_MODE_GLR16 = 4,
	WAVEFORM_MODE_REAGL = 4,
	WAVEFORM_MODE_A2 = 6,
	WAVEFORM_MODE_GCK16 = 8,
	WAVEFORM_MODE_GLKW16 = 9,
	WAVEFORM_MODE_AUTO = 257,
};

#define WAVEFORM_TYPE_4BIT 0x1
#define WAVEFORM_TYPE_5BIT (WAVEFORM_TYPE_4BIT << 1)

enum hwtcon_dithering_mode {
	HWTCON_FLAG_USE_DITHERING_PASSTHROUGH = 0x0,
	HWTCON_FLAG_USE_DITHERING_FLOYD_STEINBERG,
	HWTCON_FLAG_USE_DITHERING_ATKINSON,
	HWTCON_FLAG_USE_DITHERING_ORDERED,
	HWTCON_FLAG_USE_DITHERING_QUANT_ONLY,
	HWTCON_FLAG_USE_DITHERING_MAX,
};

struct hwtcon_waveform_modes {
	/* waveform mode index for WAVEFORM_MODE_INIT */
	int mode_init;
	/* waveform mode index for WAVEFORM_MODE_DU */
	int mode_du;
	/* waveform mode index for WAVEFORM_MODE_GC16 */
	int mode_gc16;
	/* waveform mode index for WAVEFORM_MODE_GL16 */
	int mode_gl16;
	/* waveform mode index for WAVEFORM_MODE_A2 */
	int mode_a2;
	/* waveform mode index for WAVEFORM_MODE_REAGL */
	int mode_reagl;
};

struct hwtcon_rect {
	__u32 top;
	__u32 left;
	__u32 width;
	__u32 height;
};

struct hwtcon_update_marker_data {
	__u32 update_marker;
	__u32 collision_test;
};


struct hwtcon_update_data {
	struct hwtcon_rect update_region;
	/* which waveform to use for the update, du, gc4, gc8 gc16 etc */
	__u32 waveform_mode;
	__u32 update_mode;     /* full update or partial update */
	/* Unique number used by both application
	 * and driver to identify an update
	 */
	__u32 update_marker;
	unsigned int flags;    /* one or more HWTCON_FLAGs defined above */
	int dither_mode;       /* one of the dither modes defined above */
};


struct hwtcon_panel_info {
	char wf_file_name[100];
	int vcom_value;
	/* temperature */
	int temp;
	/* temperature zone */
	int temp_zone;
};

/* ioctl commds */
#define HWTCON_IOCTL_MAGIC_NUMBER 'F' 

#define HWTCON_SET_NIGHTMODE _IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x26, int32_t)


/* Set the mapping between waveform types and waveform mode index */
#define HWTCON_SET_WAVEFORM_MODES _IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x2B, \
	struct hwtcon_waveform_modes)

/* Set the temperature for screen updates.
 * If temperature specified is TEMP_USE_SENSOR,
 * use the temperature read from the temperature sensor.
 * Otherwise use the temperature specified
 */
#define HWTCON_SET_TEMPERATURE _IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x2C, int32_t)

#define HWTCON_SET_AUTO_UPDATE_MODE      _IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x2D, __u32)

/* Get the temperature currently used for screen updates.
 * If the temperature set by command FB_SET_TEMPERATURE
 * is not equal to TEMP_USE_SENSOR,
 * return that temperature value.
 * Otherwise, return the temperature read from the temperature sensor
 */
#define HWTCON_GET_TEMPERATURE	_IOR(HWTCON_IOCTL_MAGIC_NUMBER, 0x38, int32_t)

/* Send update info to update the Eink panel display */
#define HWTCON_SEND_UPDATE		_IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x2E, \
	struct hwtcon_update_data)


/* Wait until the specified send_update request
 * (specified by hwtcon_update_marker_data) is
 * submitted to HWTCON to display or timeout (5 seconds)
 */
#define HWTCON_WAIT_FOR_UPDATE_SUBMISSION _IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x37, \
	__u32)

/* Wait until the specified send_update request
 * (specified by hwtcon_update_marker_data) is
 * already completed (Eink panel updated) or timeout (5 seconds)
 */
#define HWTCON_WAIT_FOR_UPDATE_COMPLETE	_IOWR(HWTCON_IOCTL_MAGIC_NUMBER, \
	0x2F, struct hwtcon_update_marker_data)

/* Copy the content of the working buffer to user space */
#define HWTCON_GET_WORK_BUFFER _IOWR(HWTCON_IOCTL_MAGIC_NUMBER, 0x34, \
	unsigned long)

/* Set the power down delay so the driver won't shut down the HWTCON immediately
 * after all the updates are done.
 * Instead it will wait until the "DELAY" time has elapsed to skip the
 * powerdown and powerup sequences if an update comes before that.
 */
#define HWTCON_SET_PWRDOWN_DELAY	_IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x30, int32_t)

/* Get the power down delay set in HWTCON_SET_PWRDOWN_DELAY command */
#define HWTCON_GET_PWRDOWN_DELAY	_IOR(HWTCON_IOCTL_MAGIC_NUMBER, 0x31, int32_t)

/* Pause updating the screen.
 * Any HWTCON_SEND_UPDATE request will be discarded.
 */
#define HWTCON_SET_PAUSE		_IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x33, __u32)


/* Resume updating the screen. */
#define HWTCON_SET_RESUME	_IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x35, __u32)

/* Get the screen updating flag set by HWTCON_SET_PAUSE or HWTCON_SET_RESUME */
#define HWTCON_GET_PAUSE		_IOW(HWTCON_IOCTL_MAGIC_NUMBER, 0x34, __u32)

#define HWTCON_GET_PANEL_INFO _IOR(HWTCON_IOCTL_MAGIC_NUMBER, \
	0x130, struct hwtcon_panel_info)

#endif /* __HWTCON_IOCTL_CMD_H__ */

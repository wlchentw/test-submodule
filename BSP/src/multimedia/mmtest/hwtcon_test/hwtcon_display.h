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
#ifndef __HWTCON_DISPLAY_H__
#define __HWTCON_DISPLAY_H__
#include "hwtcon_ioctl_cmd.h"


#define TCON_ERR(string, args...) printf("[E HWTCON]"string" @%s,%u\n", ##args, __func__, __LINE__)
#define TCON_LOG(string, args...) printf("[D HWTCON]"string" @%s,%u\n", ##args, __func__, __LINE__)

#define HWTCON_TIME unsigned long long
#define true 1
#define false 0

typedef unsigned char bool;
typedef unsigned char u8;
typedef unsigned int u32;

enum TCON_STATUS {
	TCON_STATUS_OK = 0,
	TCON_STATUS_WAIT_IMG_BUF_IDLE_TIMEOUT = -1,
	TCON_STATUS_MAP_KERNEL_BUF_FAIL = -2,
	TCON_STATUS_OPEN_FD_FAIL = -3,
	TCON_STATUS_QUERY_FB_FAIL = -4,
	TCON_STATUS_INVALID_DISP_REGION = -5,
	TCON_STATUS_OPEN_FILE_FAIL = -6,
	TCON_STATUS_READ_FILE_FAIL = -7,
};



struct rect {
	int x;
	int y;
	int width;
	int height;
};

struct image_t
{
    int xres;       // image's visual width, in pixels
    int xlen;       // image's actual width, used for rowbyte & memory size calculations
	int	yres;       // image's height
	int bpp;        // image's pixel (bit) depth
        
    u8 *start;     // pointer to start of image
};

struct eink_t
{
    int row_bytes, size, fd;
    u8 *buffer;
    struct image_t screen_image;
    int size_buf_kernel;
    u8 *buf_kernel;
	bool record_time;
	u8 target_time;
};


/* open device for display. */
int hwtcon_open_device(void);
/* close device. */
void hwtcon_close_device(void);

int hwtcon_get_panel_info(int *panel_width, int *panel_height);
struct eink_t *hwtcon_display_get_eink_info(void);
HWTCON_TIME hwtcon_display_get_time(void);
HWTCON_TIME hwtcon_display_get_time_duration_ms(HWTCON_TIME start,
	HWTCON_TIME end);

int hwtcon_display_set_night_mode(int enable);

/*
* input: char *buffer, the display buffer va address.
	* input: int buffer_width, the display buffer valid data width.
	* input: int buffer_height, the display buffer height.
	* input: int buffer_pitch, the display buffer width.
	* input: struct rect display_region, specify the display region (x, y, width, height)
	* input: int wave_mode: the waveform mode, default use 0.
	* input: update_mode: full update: 1, partial update: 0, default use 1.
	* output: int, retun the error code when encount error. Return 0 if no error. 
*/
int hwtcon_display_no_copy(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct rect display_region,
	int update_mode,
	int waveform_mode,
	int temperature);

void hwtcon_display_set_update_data(struct hwtcon_update_data *update_data,
	struct rect region,
	int update_mode,
	int waveform_mode);

int hwtcon_display_region(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct hwtcon_update_data *update_data,
	int temperature,
	bool wait_submission,
	bool wait_complete);

void hwtcon_display_set_flag(unsigned int flag);
int hwtcon_display_set_rotation(int rotate);
char *hwtcon_display_get_img_buffer(void);
/* copy buffer to image buffer. */
int hwtcon_fill_buffer_with_color(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	int buffer_color,
	struct rect display_region);

#endif /* __HWTCON_DISPLAY_H__ */

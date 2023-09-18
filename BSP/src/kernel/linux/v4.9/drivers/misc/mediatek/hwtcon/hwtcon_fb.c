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

#include "hwtcon_fb.h"
#include "hwtcon_def.h"
#include "hwtcon_debug.h"
#include "hwtcon_core.h"
#include "fiti_core.h"
#include "hwtcon_wf_lut_config.h"
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/pm_qos.h>
#include <linux/device.h>
#include "mtk_mdp_path.h"
#include "mtk_vcu.h"
#include "hwtcon_epd.h"
#include "hwtcon_driver.h"
#include "../cfa/cfa_driver.h"
#include <linux/videodev2.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>

static struct fb_info *g_framebuffer_info;

extern int mtk_mdp_fb_suspend(void);
extern void mtk_mdp_fb_resume(void);

static enum hrtimer_restart _hrtint_xon_on_ctrl(struct hrtimer *timer)
{
#if 0
	struct fb_private_info *fb_data = 
		container_of(timer, struct fb_private_info, hrt_xon_on_ctrl);
#endif

	hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,0);

	return HRTIMER_NORESTART;
}
static enum hrtimer_restart _hrtint_xon_off_ctrl(struct hrtimer *timer)
{
#if 0
	struct fb_private_info *fb_data = 
		container_of(timer, struct fb_private_info, hrt_xon_off_ctrl);
#endif

	hwtcon_fb_xon_ctrl(XON_CTRLMODE_0,0);

	return HRTIMER_NORESTART;
}

int hwtcon_fb_xon_ctrl(int iCtrlMode,unsigned long dwDelayus)
{
	int iRet = 0;

	if(!hwtcon_fb_info()) {
		TCON_ERR("%s() framebuffer object not exist !",__func__);
		return -1;
	}

	if(!hwtcon_fb_info()->gpio_xon_desc) {
		TCON_ERR("%s() xon not exist !",__func__);
		return -2;
	}

	if(0==dwDelayus) {
		if(XON_CTRLMODE_1==iCtrlMode) {
			// turn on directly .
			gpiod_set_value(hwtcon_fb_info()->gpio_xon_desc,1);
			hwtcon_fb_info()->jiffies_xon_onoff = jiffies;
			TCON_LOG("%s XON=1",__func__);
		}
		else if(XON_CTRLMODE_0==iCtrlMode){
			// turn off directly .
			gpiod_set_value(hwtcon_fb_info()->gpio_xon_desc,0);
			hwtcon_fb_info()->jiffies_xon_onoff = jiffies;
			
			TCON_LOG("%s XON=0",__func__);
		}
		else if(XON_CTRLMODE_OFF_DEF_1_TMR==iCtrlMode) {
			hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,hwtcon_fb_info()->off_xon_1_delay_us);
		}
		else if(XON_CTRLMODE_OFF_DEF_0_TMR==iCtrlMode) {
			hwtcon_fb_xon_ctrl(XON_CTRLMODE_0,hwtcon_fb_info()->off_xon_0_delay_us);
		}
		else if(XON_CTRLMODE_OFF_DEF_0_TMR_DAY==iCtrlMode) {
			hwtcon_fb_xon_ctrl(XON_CTRLMODE_0,hwtcon_fb_info()->off_xon_0_day_delay_us);
		}
		else if(XON_CTRLMODE_CANCEL_1_TMR==iCtrlMode) {
			// cancel turn on timer .
			TCON_LOG("a. XON 1 timer cancel\n");
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_on_ctrl);
		}
		else if(XON_CTRLMODE_CANCEL_0_TMR==iCtrlMode) {
			// cancel turn off timer .
			TCON_LOG("a. XON 0 timer cancel\n");
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_off_ctrl);
		}
		else if(XON_CTRLMODE_1_TMR_IS_PENDING==iCtrlMode) {
			iRet = hrtimer_is_queued(&hwtcon_fb_info()->hrt_xon_on_ctrl);
		}
		else if(XON_CTRLMODE_0_TMR_IS_PENDING==iCtrlMode) {
			iRet = hrtimer_is_queued(&hwtcon_fb_info()->hrt_xon_off_ctrl);
		}
		else if(XON_CTRLMODE_GET_CUR_STAT==iCtrlMode) {
			iRet = gpiod_get_value(hwtcon_fb_info()->gpio_xon_desc);
		}
		else {
			TCON_ERR("unsupported XON ctrl mode %d ,delayus=%d!\n",iCtrlMode,(int)dwDelayus);
		}

	}
	else {

		// control xon by timer .
		if(XON_CTRLMODE_1==iCtrlMode) {
			// turn on by timer . 
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_on_ctrl);
			TCON_LOG("XON 1 after %d us\n",(int)dwDelayus);
			//hwtcon_fb_info()->hrt_xon_on_ctrl.function = _hrtint_xon_on_ctrl;
			hrtimer_start(&hwtcon_fb_info()->hrt_xon_on_ctrl,
				ktime_set(dwDelayus/999999,(dwDelayus%1000000) * 1000),
				HRTIMER_MODE_REL);
		}
		else if(XON_CTRLMODE_0==iCtrlMode) {
			// turn off by timer .
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_off_ctrl);
			TCON_LOG("XON 0 after %d us\n",(int)dwDelayus);
			//hwtcon_fb_info()->hrt_xon_on_ctrl.function = _hrtint_xon_on_ctrl;
			hrtimer_start(&hwtcon_fb_info()->hrt_xon_off_ctrl,
				ktime_set(dwDelayus/999999,(dwDelayus%1000000) * 1000),
				HRTIMER_MODE_REL);
		}
		else if(XON_CTRLMODE_CANCEL_1_TMR==iCtrlMode) {
			// cancel turn on timer .
			TCON_LOG("b. XON 1 timer cancel\n");
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_on_ctrl);
		}
		else if(XON_CTRLMODE_CANCEL_0_TMR==iCtrlMode) {
			// cancel turn off timer .
			TCON_LOG("b. XON 0 timer cancel\n");
			hrtimer_cancel(&hwtcon_fb_info()->hrt_xon_off_ctrl);
		}
		else {
			TCON_ERR("unsupported XON ctrl mode %d ,delayus=%d!\n",iCtrlMode,(int)dwDelayus);
		}
	}
	return iRet ;	
}

static int hwtcon_fb_open(struct fb_info *info, int user)
{
	return 0;
}

static int hwtcon_fb_release(struct fb_info *info, int user)
{
	return 0;
}

/* no use */
static int hwtcon_fb_setcolreg(unsigned int regno, unsigned int red,
	unsigned int green,
	unsigned int blue, unsigned int transp, struct fb_info *info)
{
	return 0;
}

/* no use */
static int hwtcon_fb_pan_display_proxy(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	return 0;
}

/* no use */
static int hwtcon_fb_set_par(struct fb_info *fbi)
{
#if 0
	TCON_LOG(" hwtcon change other color format info");
	fbi->fix.line_length = fbi->var.bits_per_pixel / 8;
	switch (fbi->var.bits_per_pixel) {
	case 8:
		fbi->fix.visual = FB_VISUAL_MONO10;
		break;
	case 16:
	case 24:
	case 32:
		fbi->fix.visual = FB_VISUAL_TRUECOLOR;
		break;
	}
#endif
	return 0;
}

u32 hwtcon_fb_get_cur_color_format(void)
{
	u32 format = 0;
	switch (g_framebuffer_info->var.bits_per_pixel) {
	case 8:
		format = V4L2_PIX_FMT_Y8;
		break;
	case 16:
		format = V4L2_PIX_FMT_RGB565;
		break;
	case 24:
		format = V4L2_PIX_FMT_RGB24;
		break;
	case 32:
		format = V4L2_PIX_FMT_ARGB32;
		break;
	default:
		format = V4L2_PIX_FMT_ARGB32;
		break;
	}
	return format;
}

static void hwtcon_fb_pixel_format(struct fb_var_screeninfo *var) {
	switch (var->bits_per_pixel) {
	case 24:/*RGB*/
		var->red =    (struct fb_bitfield){0, 8, 0};
		var->green =  (struct fb_bitfield){8, 8, 0};
		var->blue =   (struct fb_bitfield){16, 8, 0};
		var->transp = (struct fb_bitfield){0, 0, 0};
		break;
	case 32:/*RGBA*/
		var->red =    (struct fb_bitfield){0, 8, 0};
		var->green =  (struct fb_bitfield){8, 8, 0};
		var->blue =   (struct fb_bitfield){16, 8, 0};
		var->transp = (struct fb_bitfield){24, 8, 0};
		break;
	case 8:
	default:			/*Y8*/
		var->red =    (struct fb_bitfield){0, 0, 0};
		var->green =  (struct fb_bitfield){0, 0, 0};
		var->blue =   (struct fb_bitfield){0, 0, 0};
		var->transp = (struct fb_bitfield){0, 0, 0};
		break;
	}
}

static int hwtcon_fb_check_rotate(struct fb_var_screeninfo *var,
		struct fb_info *info)
{
	var->rotate = var->rotate % 4;
	switch (var->rotate) {
	case HWTCON_ROTATE_0:
	case HWTCON_ROTATE_180:
		var->xres = hw_tcon_get_edp_width();
		var->yres = hw_tcon_get_edp_height();
		break;
	case HWTCON_ROTATE_90:
	case HWTCON_ROTATE_270:
		var->xres = hw_tcon_get_edp_height();
		var->yres = hw_tcon_get_edp_width();
		break;
	default:
		TCON_ERR("invalid rotate:%d check fail",
			var->rotate);
		return -1;
	}

	//var->xres_virtual = FB_ALIGN(var->xres);
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
	info->fix.line_length = var->xres_virtual;

	switch (hwtcon_device_info()->color_format) {
		case EPD_COLOR_FORMAT_RGB565:
			var->bits_per_pixel = 16;
			info->fix.line_length =
				info->fix.line_length << 1; /* 2 byte/pixel */
			((struct fb_private_info *)(info->par))->mdp_src_format = V4L2_PIX_FMT_RGB565;
			break;
		case EPD_COLOR_FORMAT_ARGB32:
			var->bits_per_pixel = 32;
			info->fix.line_length =
				info->fix.line_length << 2; /* 4 byte/pixel */
			((struct fb_private_info *)(info->par))->mdp_src_format = V4L2_PIX_FMT_ARGB32;
			break;
		case EPD_COLOR_FORMAT_Y8:
		default:
			var->bits_per_pixel = 8;
			((struct fb_private_info *)(info->par))->mdp_src_format = V4L2_PIX_FMT_Y8;
			break;
	}
	//printk("%s:line_length=%d\n",__func__,info->fix.line_length);
	return 0;
}

int hwtcon_fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	int status = 0;

	if (hwtcon_fb_check_rotate(var, info))
		return -1;
	/* 0 = color, 1 = grayscale, 2 = night mode */
	if (var->grayscale == 0 || var->grayscale == 1 || var->grayscale == 2)
		TCON_LOG(" check grayscale success");
	else {
		TCON_ERR(" check grayscale fail");	
		return -EINVAL;
	}

	/*
	if (var->grayscale == 0 && (!hw_tcon_get_epd_type())) {
		TCON_ERR(" It is a B/W panel, can not be used by color");
		return -EINVAL;
	}
	*/

	if (var->bits_per_pixel == 8 || var->bits_per_pixel == 16
		|| var->bits_per_pixel == 24 || var->bits_per_pixel == 32)
		TCON_LOG(" check pixel format success");
	else {
		TCON_ERR(" check pixel format fail");	
		return -EINVAL;
	}

	hwtcon_fb_pixel_format(var);
	/* wait all task done before call fb_set_par to change color format*/
	status = hwtcon_core_wait_all_task_done();
	if (status)
		TCON_ERR(" fail to wait hw idle befor change fb");
	return 0;
}

void hwtcon_fb_flush_update(void)
{
	mutex_lock(&hwtcon_fb_info()->update_queue_mutex);

	/* hwtcon busy,need to wait it finish update*/
	hwtcon_core_wait_all_task_done();

	mutex_unlock(&hwtcon_fb_info()->update_queue_mutex);
}

static int hwtcon_core_update_task_wf_mode(
	struct hwtcon_update_data *update_data)
{
	enum WAVEFORM_MODE_ENUM wf_mode = update_data->waveform_mode;

	switch (wf_mode) {
	case WAVEFORM_MODE_INIT:
	case WAVEFORM_MODE_DU:
	case WAVEFORM_MODE_A2:
	case WAVEFORM_MODE_AUTO:
		break;
	case WAVEFORM_MODE_GL16:
		if(hwtcon_fb_info()->night_mode_wfm_mapping) {
			wf_mode = WAVEFORM_MODE_GCK16;
		}
		break;
	case WAVEFORM_MODE_GLR16:
		if(hwtcon_fb_info()->night_mode_wfm_mapping) {
			wf_mode = WAVEFORM_MODE_GCK16;
		}
		break;
	case WAVEFORM_MODE_GC16:
		if(hwtcon_fb_info()->night_mode_wfm_mapping) {
			if (update_data->update_mode == UPDATE_MODE_PARTIAL) {
				wf_mode = WAVEFORM_MODE_GCK16;
			}
			else {
				wf_mode = WAVEFORM_MODE_GLKW16;
			}
		}
		else {
			if (update_data->update_mode == UPDATE_MODE_PARTIAL)
				wf_mode = WAVEFORM_MODE_GL16;
		}
		break;
	case WAVEFORM_MODE_GCK16:
	case WAVEFORM_MODE_GLKW16:
		break;
	case WAVEFORM_MODE_GCC16:
		break;
	default:
		TCON_ERR("invalid waveform mode:%d(%s)",
			wf_mode,
			hwtcon_core_get_wf_mode_name(wf_mode));
		return -1;
	}

	update_data->waveform_mode = wf_mode;
	return 0;
}

int hwtcon_fb_get_width(void)
{
	return g_framebuffer_info->var.xres;
}
EXPORT_SYMBOL(hwtcon_fb_get_width);

int hwtcon_fb_get_height(void)
{
	return g_framebuffer_info->var.yres;
}
EXPORT_SYMBOL(hwtcon_fb_get_height);

int hwtcon_fb_get_virtual_width(void)
{
	return g_framebuffer_info->var.xres_virtual;
}

int hwtcon_fb_check_update_data_invalid(
	struct hwtcon_update_data *update_data)
{
	struct rect panel_region = {0, 0,
		hw_tcon_get_edp_width(),
		hw_tcon_get_edp_height()};
	struct rect task_region = {0};

	task_region = hwtcon_core_get_update_data_region(update_data);

	if (update_data->update_mode != UPDATE_MODE_FULL &&
		update_data->update_mode != UPDATE_MODE_PARTIAL) {
		TCON_ERR("invalid update_mode:%d", update_data->update_mode);
		return HWTCON_STATUS_INVALID_PARAM;
	}

	if (!hwtcon_rect_contain_00(&panel_region, &task_region)) {
		TCON_ERR("invalid region[%d %d %d %d] panel[%d %d] rot:%d",
			task_region.x,
			task_region.y,
			task_region.width,
			task_region.height,
			hw_tcon_get_edp_width(),
			hw_tcon_get_edp_height(),
			hwtcon_fb_get_rotation());
		return HWTCON_STATUS_INVALID_PARAM;
	}

	if (hwtcon_core_update_task_wf_mode(update_data) != 0) {
		TCON_ERR("invalid waveform_mode:%d update_mode:%d",
			update_data->waveform_mode,
			update_data->update_mode);
		return HWTCON_STATUS_INVALID_PARAM;
	}
	return 0;
}

int hwtcon_fb_set_temperature(int iTemp)
{
	int iOldTemp = hwtcon_fb_info()->temperature;
	TCON_LOG("%s set temperature:%d",__func__, iTemp);

	hwtcon_fb_info()->temperature = iTemp;
	return iOldTemp;
}

static int hwtcon_fb_ioctl_set_temperature(void *arg)
{
	int value = 0;

	if (copy_from_user(&value, arg, sizeof(value)) != 0) {
		TCON_ERR("copy_from_user fail");
		return HWTCON_STATUS_COPY_FROM_USER_FAIL;
	}
	hwtcon_fb_set_temperature(value);
	return 0;
}

static int hwtcon_fb_ioctl_get_temperature(void *arg)
{
	int value = hwtcon_core_read_temperature();

	if (copy_to_user((void *)arg, &value, sizeof(value)) != 0) {
		TCON_ERR("copy_to_user fail");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}

	return 0;
}

static int hwtcon_fb_ioctl_send_update(void *arg)
{
	int status = 0;
	struct hwtcon_update_data update_data;

	if (hwtcon_fb_info()->ignore_request) {
		TCON_LOG("ignore request");
		return 0;
	}

	memset(&update_data, 0, sizeof(update_data));
	if (copy_from_user(&update_data, (void *)arg,
		sizeof(update_data)) != 0) {
		TCON_ERR("copy_from_user fail");
		return HWTCON_STATUS_COPY_FROM_USER_FAIL;
	}

	status = hwtcon_fb_check_update_data_invalid(&update_data);
	if (status != 0)
		return status;

#if 0 // test only 
	HWTCON_FLAG_SET_CFA_MODE(update_data.flags,HWTCON_CFA_MODE_NTX_SF);
#endif 

	status = hwtcon_core_submit_task(&update_data);

	return status;
}


static int hwtcon_fb_ioctl_wait_for_task_triggered(void *arg)
{
	u32 update_marker = 0;

	if (copy_from_user(&update_marker, arg, sizeof(u32)) != 0) {
		TCON_ERR("copy_from_user fail");
		return HWTCON_STATUS_COPY_FROM_USER_FAIL;
	}

	return hwtcon_core_wait_for_task_triggered(update_marker);
}

static int hwtcon_fb_ioctl_wait_for_task_displayed(void *arg)
{
	u32 update_marker = 0;

	if (copy_from_user(&update_marker, arg, sizeof(u32)) != 0) {
		TCON_ERR("copy_from_user fail");
		return HWTCON_STATUS_COPY_FROM_USER_FAIL;
	}

	return hwtcon_core_wait_for_task_displayed(update_marker);
}

static int hwtcon_fb_ioctl_get_working_buffer(void *arg)
{
	if (copy_to_user(arg, hwtcon_fb_info()->wb_buffer_va,
		hwtcon_fb_info()->wb_buffer_size) != 0) {
		TCON_ERR("copy_to_user fail");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}
	return 0;
}


static int hwtcon_fb_ioctl_set_power_down_delay_time(void *arg)
{
	int delay_time = 0;

	if (copy_from_user(&delay_time, arg, sizeof(delay_time)) != 0) {
		TCON_ERR("copy from user fail");
		return HWTCON_STATUS_COPY_FROM_USER_FAIL;
	}

	/* check param valid */
	if (delay_time < 0) {
		TCON_ERR("invalid delay time:%d", delay_time);
		return HWTCON_STATUS_INVALID_PARAM;
	}

	TCON_LOG("%s() new delayms = %d",__func__,delay_time);

	hwtcon_fb_info()->power_down_delay_ms = delay_time;
	return 0;
}

static int hwtcon_fb_ioctl_get_power_down_delay_time(void *arg)
{
	int delay_time = hwtcon_fb_info()->power_down_delay_ms;

	if (copy_to_user(arg, &delay_time, sizeof(delay_time)) != 0) {
		TCON_ERR("invalid copy to user");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}

	return 0;
}

static int hwtcon_fb_ioctl_set_pause(void *arg)
{
	hwtcon_fb_info()->ignore_request = true;
	return 0;
}

static int hwtcon_fb_ioctl_set_resume(void *arg)
{
	hwtcon_fb_info()->ignore_request = false;
	return 0;
}

static int hwtcon_fb_ioctl_get_pause(void *arg)
{
	u32 ignore_request = hwtcon_fb_info()->ignore_request;

	if (copy_to_user(arg, &ignore_request, sizeof(ignore_request)) != 0) {
		TCON_ERR("copy to user fail");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}

	return 0;
}


int hwtcon_fb_set_cfa_mode(int cfa_mode) 
{
	int iRet ;
	
	if(cfa_get_mode_name_by_id(cfa_mode)) {
		TCON_LOG("set cfa_mode:%d", cfa_mode);
		hwtcon_fb_info()->cfa_mode = cfa_mode;
		iRet = 0;
	}
	else {
		iRet = -1;
	}

	return iRet;
}

static int hwtcon_fb_ioctl_set_cfa_mode(void *arg)
{
	hwtcon_fb_info()->cfa_mode = true;
	return 0;
}
static int hwtcon_fb_ioctl_get_cfa_mode(void *arg)
{
	u32 cfa_mode = hwtcon_fb_info()->cfa_mode;

	if (copy_to_user(arg, &cfa_mode, sizeof(cfa_mode)) != 0) {
		TCON_ERR("copy to user fail");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}

	return 0;
}


static int hwtcon_fb_ioctl_get_panel_info(void *arg)
{
	struct hwtcon_panel_info info;

	memset(&info, 0, sizeof(info));
	info.temp = hwtcon_core_read_temperature();
	info.temp_zone = hwtcon_core_read_temp_zone();
#ifndef FPGA_EARLY_PORTING
	/* TODO */
	info.vcom_value = fiti_read_vcom();
#endif
	snprintf(info.wf_file_name,	sizeof(info.wf_file_name), "%s",
		wf_lut_get_wf_info()->wf_file_name);

	if (copy_to_user(arg, &info, sizeof(info)) != 0) {
		TCON_ERR("copy to user fail");
		return HWTCON_STATUS_COPY_TO_USER_FAIL;
	}
	return 0;
}

int hwtcon_fb_ioctl_set_night_mode(bool night_mode ,bool invert_fb, int nm_wfm_mapping)
{

	hwtcon_fb_info()->enable_night_mode = night_mode;
	hwtcon_fb_info()->invert_fb = invert_fb;
	hwtcon_fb_info()->night_mode_wfm_mapping = nm_wfm_mapping;

	TCON_LOG("night_mode %d,invert_fb %d,nm_wfm_map=%d",
		hwtcon_fb_info()->enable_night_mode,
		hwtcon_fb_info()->invert_fb,
		hwtcon_fb_info()->night_mode_wfm_mapping);

	return 0;
}

bool hwtcon_core_use_night_mode(void)
{
	return (hwtcon_fb_info()->enable_night_mode);
}

bool hwtcon_core_invert_fb(void)
{
	return (hwtcon_fb_info()->invert_fb);
}

static int hwtcon_fb_ioctl(struct fb_info *info, unsigned int cmd,
	unsigned long arg)
{
	switch (cmd) {
	case HWTCON_SET_WAVEFORM_MODES:
		break;
	case HWTCON_SET_TEMPERATURE:
		return hwtcon_fb_ioctl_set_temperature((void *)arg);
	case HWTCON_GET_TEMPERATURE:
		return hwtcon_fb_ioctl_get_temperature((void *)arg);
	case HWTCON_SEND_UPDATE:
		return hwtcon_fb_ioctl_send_update((void *)arg);
	case HWTCON_WAIT_FOR_UPDATE_SUBMISSION:
		return hwtcon_fb_ioctl_wait_for_task_triggered((void *)arg);
	case HWTCON_WAIT_FOR_UPDATE_COMPLETE:
		return hwtcon_fb_ioctl_wait_for_task_displayed((void *)arg);
	case HWTCON_GET_WORK_BUFFER:
		return hwtcon_fb_ioctl_get_working_buffer((void *)arg);
	case HWTCON_SET_PWRDOWN_DELAY:
		return hwtcon_fb_ioctl_set_power_down_delay_time((void *)arg);
	case HWTCON_GET_PWRDOWN_DELAY:
		return hwtcon_fb_ioctl_get_power_down_delay_time((void *)arg);
	case HWTCON_SET_PAUSE:
		return hwtcon_fb_ioctl_set_pause((void *)arg);
	case HWTCON_SET_RESUME:
		return hwtcon_fb_ioctl_set_resume((void *)arg);
	case HWTCON_GET_PAUSE:
		return hwtcon_fb_ioctl_get_pause((void *)arg);
	case HWTCON_GET_PANEL_INFO:
		return hwtcon_fb_ioctl_get_panel_info((void *)arg);
	case HWTCON_SET_AUTO_UPDATE_MODE:
		return 0;
	case HWTCON_SET_NIGHTMODE:
		{
			u32 night_mode;

			if (copy_from_user(&night_mode, (void *)arg,
				sizeof(night_mode)) != 0) {
				TCON_ERR("copy from user fail");
				return HWTCON_STATUS_COPY_FROM_USER_FAIL;
			}
			hwtcon_fb_info()->enable_night_mode_by_wfm = false;
			return hwtcon_fb_ioctl_set_night_mode(night_mode,1,0);
		}
	case HWTCON_SET_CFA_MODE:
		return hwtcon_fb_ioctl_set_cfa_mode((void *)arg);
	case HWTCON_GET_CFA_MODE:
		return hwtcon_fb_ioctl_get_cfa_mode((void *)arg);
	default:
		TCON_ERR("err cmd:0x%08x dir:0x%x type:0x%x nr:0x%x size:0x%x",
			cmd,
			_IOC_DIR(cmd),
			_IOC_TYPE(cmd),
			_IOC_NR(cmd),
			_IOC_SIZE(cmd));
		return HWTCON_STATUS_INVALID_IOCTL_CMD;
	}

	TCON_ERR("ioctl cmd:0x%x not implement", cmd);
	return 0;
}

#ifdef CONFIG_COMPAT
static int hwtcon_fb_compat_ioctl(struct fb_info *info, unsigned int cmd,
	unsigned long arg)
{
	return hwtcon_fb_ioctl(info, cmd, arg);
}
#endif


/* frame buffer already have mmap implement,
 * this function can be removed.
 */
static int hwtcon_fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	int status = 0;

	if (hw_tcon_get_epd_type()) {
		/* vmalloc buffer */
		struct sg_table *table = NULL;
		unsigned long addr = 0L;
		unsigned long offset = 0L;
		struct scatterlist *sg = NULL;
		int i;

		table = ((struct fb_private_info *)(info->par))->fb_cache_buf->sgt;
		addr = vma->vm_start;
		offset = vma->vm_pgoff * PAGE_SIZE;

		for_each_sg(table->sgl, sg, table->nents, i) {
			struct page *page = sg_page(sg);
			unsigned long remainder = vma->vm_end - addr;
			unsigned long len = sg->length;

			if (offset >= sg->length) {
				offset -= sg->length;
				continue;
			} else if (offset) {
				page += offset / PAGE_SIZE;
				len = sg->length - offset;
				offset = 0;
			}
			len = min(len, remainder);
			status = remap_pfn_range(vma, addr, page_to_pfn(page), len,
					vma->vm_page_prot);
			if (status) {
				TCON_ERR("%s remap fail 0x%p, %lu, %lu, %lu, %d.\n",
					   __func__, vma, addr,
					   page_to_pfn(page), len, status);
				return status;
			}
			addr += len;
			if (addr >= vma->vm_end)
				return 0;
		}
	}else {
	status = dma_mmap_attrs(((struct fb_private_info *)(info->par))->dev,
		vma,
		((struct fb_private_info *)(info->par))->fb_buffer_va,
		((struct fb_private_info *)(info->par))->fb_buffer_pa,
		((struct fb_private_info *)(info->par))->fb_buffer_size,
		0);
	if (status != 0)
		TCON_ERR("mmap fail: status:%d size:%d",
		status,
		((struct fb_private_info *)
		(info->par))->fb_buffer_size);
	}
	return status;
}


static int hwtcon_fb_setcmap(struct fb_cmap *cmap, struct fb_info *info)
{
	/* color map */
	mdp_update_cmap_lut(cmap->red, cmap->len);
	return 0;
}

static struct fb_ops hwtcon_fb_ops = {
	.owner = THIS_MODULE,
	.fb_open = hwtcon_fb_open,
	.fb_release = hwtcon_fb_release,
	.fb_setcolreg = hwtcon_fb_setcolreg,
	.fb_pan_display = hwtcon_fb_pan_display_proxy,
	.fb_check_var = hwtcon_fb_check_var,
	#if 0
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	#endif
	.fb_set_par = hwtcon_fb_set_par,
	.fb_ioctl = hwtcon_fb_ioctl,
#ifdef CONFIG_COMPAT
	.fb_compat_ioctl = hwtcon_fb_compat_ioctl,
#endif
	.fb_mmap = hwtcon_fb_mmap,
	.fb_setcmap = hwtcon_fb_setcmap,

};

static struct task_struct *pThread1;
static struct task_struct *pThread2;
static int hwtcon_fb_kthread_create(void)
{
	struct sched_param param = {2};

	if (pThread1 == NULL) {
		pThread1 = kthread_run(hwtcon_core_dispatch_pipeline, NULL,
			"dispatch_pipeline");
		if (IS_ERR(pThread1)) {
			TCON_ERR("create thread dispatch_pipeline failed");
			return HWTCON_STATUS_CREAT_THREAD_FAIL;
		}

		/* adjust thread priority. */
		sched_setscheduler(pThread1, SCHED_RR, &param);
	}

	if (pThread2 == NULL) {
		pThread2 = kthread_run(hwtcon_core_dispatch_mdp, NULL,
			"dispatch_mdp");
		if (IS_ERR(pThread2)) {
			TCON_ERR("create thread hwtcon_dispatch_mdp failed");
			return HWTCON_STATUS_CREAT_THREAD_FAIL;
		}
		/* adjust thread priority. */
		sched_setscheduler(pThread2, SCHED_RR, &param);
	}
	#if 0
	/* hwtcon_core_start_lut_assign_done_trigger_loop(); */
	#endif

	return 0;
}

static int hwtcon_fb_kthread_destroy(void)
{
#if 0
	if (pThread1)
		kthread_stop(pThread1);
	if (pThread2)
		kthread_stop(pThread2);
#endif
	return 0;
}

#if 0
void hwtcon_fb_read_temperature_from_sensor(struct work_struct *workItem)
{
	if (!fiti_pmic_judge_power_on_going()) {
		/* Do not use fiti when fiti is in POWER_ON_GOING state */
		hwtcon_fb_info()->temperature = fiti_read_temperature();
		TCON_LOG("read sensor temperature[%d]",
			hwtcon_fb_info()->temperature);
	}
	schedule_delayed_work(&hwtcon_fb_info()->read_sensor_work,
			msecs_to_jiffies(SENSOR_READ_INTERVAL_MS));
}
#endif


static int hwtcon_fb_init_fb_info(struct fb_info *info)
{
	info->fbops = &hwtcon_fb_ops;
	info->flags = FBINFO_FLAG_DEFAULT;
	info->screen_buffer = ((struct fb_private_info *)
		(info->par))->fb_buffer_va;
	info->screen_size = ((struct fb_private_info *)
		(info->par))->fb_buffer_size;

	/* fb_fix_screeninfo */
	strncpy(info->fix.id, HWTCON_DRIVER_NAME, sizeof(info->fix.id));
	info->fix.smem_start = ((struct fb_private_info *)
		(info->par))->fb_buffer_pa;
	info->fix.smem_len = ((struct fb_private_info *)
		(info->par))->fb_buffer_size;
	info->fix.type = FB_TYPE_PACKED_PIXELS;
	if (1 == hwtcon_device_info()->color_format) {
		info->fix.visual = FB_VISUAL_MONO10;
		info->var.grayscale = 1; /* 0 = color, 1 = grayscale, */
		info->var.nonstd = 0; /* non standard pixel format */
	} else {
		info->fix.visual = FB_VISUAL_TRUECOLOR;
		info->var.grayscale = 0; /* 0 = color, 1 = grayscale, */
		info->var.nonstd = 0; /* non standard pixel format */
	}
	info->fix.xpanstep = 0;
	info->fix.ypanstep = 0;
	info->fix.ywrapstep = 0;
	info->fix.line_length =
		hw_tcon_get_edp_width();	/* 1 byte/pixel */
	info->fix.accel = FB_ACCEL_NONE;

	/* fb_var_screeninfo */
	info->var.rotate = hw_tcon_get_edp_rotate();

	hwtcon_fb_check_rotate(&info->var, info); /* init xres yres*/
	//info->fix.line_length = info->var.xres_virtual;

	info->var.left_margin = 0;
	info->var.right_margin = 0;
	info->var.upper_margin = 0;
	info->var.lower_margin = 0;

	info->var.pixclock = hw_tcon_get_edp_clk() / 2;
	info->var.hsync_len = hw_tcon_get_edp_blank_hsa();
	info->var.vsync_len = hw_tcon_get_edp_blank_vsa();

	info->var.bits_per_pixel = hwtcon_device_info()->color_format<<3;
	hwtcon_fb_pixel_format(&info->var);

	info->var.xoffset = 0;
	info->var.yoffset = 0;
	info->var.activate = FB_ACTIVATE_NOW;

	return 0;
}



static size_t calculate_frame_size(void)
{
	size_t rot0, rot3 = 0;
	switch (hwtcon_device_info()->color_format) {
	case 1:
		rot0 = FB_ALIGN(hw_tcon_get_edp_width()) * hw_tcon_get_edp_height();
		rot3 = hw_tcon_get_edp_width() * FB_ALIGN(hw_tcon_get_edp_height());
		break;
	case 2:
		rot0 = FB_ALIGN(hw_tcon_get_edp_width()) * hw_tcon_get_edp_height() * 2;
		rot3 = hw_tcon_get_edp_width() * FB_ALIGN(hw_tcon_get_edp_height()) * 2;
		break;
	case 3:
		rot0 = FB_ALIGN(hw_tcon_get_edp_width()) * hw_tcon_get_edp_height() * 3;
		rot3 = hw_tcon_get_edp_width() * FB_ALIGN(hw_tcon_get_edp_height()) * 3;
		break;
	case 4:
		rot0 = FB_ALIGN(hw_tcon_get_edp_width()) * hw_tcon_get_edp_height() * 4;
		rot3 = hw_tcon_get_edp_width() * FB_ALIGN(hw_tcon_get_edp_height()) * 4;
		break;
	default:
		rot0 = FB_ALIGN(hw_tcon_get_edp_width()) * hw_tcon_get_edp_height();
		rot3 = hw_tcon_get_edp_width() * FB_ALIGN(hw_tcon_get_edp_height());
		break;
	}

	return (rot0 > rot3 ? rot0 : rot3);
}



static int hwtcon_fb_init_private_fb_info(
	struct fb_private_info *private_info, struct platform_device *pdev)
{
	unsigned long flags;
	int i = 0;

	private_info->regulator_vcore = regulator_get(&pdev->dev, "vcore");
	if (private_info->regulator_vcore == NULL)
		TCON_ERR("get vcore regulator fail");

	private_info->vcore_req =
		kzalloc(sizeof(struct pm_qos_request), GFP_KERNEL);
	/* request vcore voltage 0.65V */
	pm_qos_add_request(private_info->vcore_req,
		PM_QOS_VCORE_OPP,
		PM_QOS_VCORE_OPP_DEFAULT_VALUE);

	private_info->mdp_src_format = V4L2_PIX_FMT_ARGB32;
	private_info->jiffies_xon_onoff = jiffies;
	private_info->dev = &pdev->dev;
	/* set private_info as dev->data */
	dev_set_drvdata(&pdev->dev, private_info);

	/* allocate frame buffer */
	if (hw_tcon_get_epd_type()) {	
		private_info->fb_buffer_size = calculate_frame_size();
		private_info->fb_cache_buf = hwtcon_fb_alloc_cached_mva_buf(&pdev->dev,
			private_info->fb_buffer_size,
			GFP_KERNEL);
		if (private_info->fb_cache_buf == NULL) {
			TCON_ERR("allocate fb buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->fb_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->fb_buffer_va = private_info->fb_cache_buf->alloc_va;
		private_info->fb_buffer_pa = private_info->fb_cache_buf->dma_handle;
	} else {
		private_info->fb_buffer_size = calculate_frame_size();
		private_info->fb_buffer_va = (char *)dma_alloc_coherent(&pdev->dev,
			private_info->fb_buffer_size,
			&private_info->fb_buffer_pa,
			GFP_KERNEL);
		if (private_info->fb_buffer_va == NULL) {
			TCON_ERR("allocate fb buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->fb_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
	}
	/* allocate temp buffer */
	private_info->tmp_buffer_size = private_info->fb_buffer_size;
	private_info->tmp_buffer_va = (char *)dma_alloc_coherent(&pdev->dev,
		private_info->tmp_buffer_size,
		&private_info->tmp_buffer_pa,
		GFP_KERNEL);
	if (private_info->tmp_buffer_va == NULL) {
		TCON_ERR("allocate tmp buffer fail");
			return HWTCON_STATUS_FB_ALLOC_FAIL;
	}
	if (hw_tcon_get_epd_type()) {
		/* allocate color buffer */
		private_info->color_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->color_cache_buf = hwtcon_fb_alloc_cached_mva_buf(&pdev->dev,
			private_info->color_buffer_size, GFP_KERNEL);
		if (private_info->color_cache_buf == NULL) {
	        TCON_ERR("allocate color_buffer fail");
	        return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->color_buffer_va = private_info->color_cache_buf->alloc_va;
		private_info->color_buffer_pa = private_info->color_cache_buf->dma_handle;

		/* allocate color info buffer */
		private_info->cinfo_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->cinfo_cache_buf = hwtcon_fb_alloc_cached_mva_buf(&pdev->dev,
			private_info->cinfo_buffer_size, GFP_KERNEL);
		if (private_info->cinfo_cache_buf == NULL) {
	        TCON_ERR("allocate cinfo buffer fail");
	        return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->cinfo_buffer_va = private_info->cinfo_cache_buf->alloc_va;
		private_info->cinfo_buffer_pa = private_info->cinfo_cache_buf->dma_handle;

		/* allocate image buffer */
		private_info->img_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height() + 4;
		private_info->img_cache_buf = hwtcon_fb_alloc_cached_mva_buf(&pdev->dev,
			private_info->img_buffer_size,
			GFP_KERNEL);
		if (private_info->img_cache_buf == NULL) {
			TCON_ERR("allocate image buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->img_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->img_buffer_va = private_info->img_cache_buf->alloc_va;
		private_info->img_buffer_pa = private_info->img_cache_buf->dma_handle;

		/* allocate regal buffer */
		private_info->temp_img_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->temp_img_cache_buf = hwtcon_fb_alloc_cached_mva_buf(&pdev->dev,
			private_info->temp_img_buffer_size,
			GFP_KERNEL);
		if (private_info->temp_img_cache_buf == NULL) {
			TCON_ERR("allocate temp image buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->temp_img_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->temp_img_buffer_va = private_info->temp_img_cache_buf->alloc_va;
		private_info->temp_img_buffer_pa = private_info->temp_img_cache_buf->dma_handle;
	}else {
		/* allocate image buffer */
		private_info->img_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->img_buffer_va = (char *)dma_alloc_coherent(&pdev->dev,
		private_info->img_buffer_size,
		&private_info->img_buffer_pa,
		GFP_KERNEL);
		if (private_info->img_buffer_va == NULL) {
			TCON_ERR("allocate image buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->img_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		/* allocate regal buffer */
		private_info->temp_img_buffer_size =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->temp_img_buffer_va =
			(char *)dma_alloc_coherent(&pdev->dev,
			private_info->temp_img_buffer_size,
			&private_info->temp_img_buffer_pa,
			GFP_KERNEL);
		if (private_info->temp_img_buffer_va == NULL) {
			TCON_ERR("allocate temp image buffer fail w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->temp_img_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
	}
	/* allocate debug image buffer info */
	private_info->debug_img_dump_path[0]='\0';
	private_info->debug_img_buffer_counter = 0;
	for (i = 0; i < MAX_DEBUG_IMAGE_BUFFER_COUNT; i++) {
		private_info->debug_img_buffer_size[i] =
			hw_tcon_get_edp_width() * hw_tcon_get_edp_height();
		private_info->debug_img_buffer_va[i] =
			(char *)dma_alloc_coherent(&pdev->dev,
			private_info->debug_img_buffer_size[i],
			&private_info->debug_img_buffer_pa[i],
			GFP_KERNEL);
		if (private_info->debug_img_buffer_va[i] == NULL) {
			TCON_ERR("allocate debug image buffer fail");
			TCON_ERR("w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->debug_img_buffer_size[i]);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
		private_info->debug_img_buffer_name[i] =
			vzalloc(MAX_FILE_NAME_LEN);
		if (private_info->debug_img_buffer_name[i] == NULL) {
			TCON_ERR("allocate buffer name fail");
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
	}

	private_info->wb_buffer_size =
		hw_tcon_get_edp_width() *
		hw_tcon_get_edp_height() * 2;

	if (hwtcon_device_info()->reserved_buf_ready) {
		/* use reserved buffer */
		private_info->waveform_va =
			hwtcon_device_info()->reserved_buf_va;
		private_info->waveform_pa =
			hwtcon_device_info()->reserved_buf_mva;
		private_info->waveform_size = WAVEFORM_SIZE;

		private_info->wb_buffer_va =
			hwtcon_device_info()->reserved_buf_va + WAVEFORM_SIZE;
		private_info->wb_buffer_pa =
			hwtcon_device_info()->reserved_buf_mva + WAVEFORM_SIZE;
	} else {
		/* use dma alloc buffer */
		/* allocate working buffer */
		private_info->wb_buffer_va = (char *)dma_alloc_coherent(&pdev->dev,
				private_info->wb_buffer_size,
				&private_info->wb_buffer_pa,
			GFP_KERNEL);
		if (private_info->wb_buffer_va == NULL) {
			TCON_ERR("allocate working buffer fail");
			TCON_ERR("w:%d h:%d size:%d",
				hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height(),
				private_info->wb_buffer_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}

		/* allocate waveform buffer */
		private_info->waveform_size = WAVEFORM_SIZE;
		private_info->waveform_va =
			(char *)dma_alloc_coherent(&pdev->dev,
				private_info->waveform_size,
				&private_info->waveform_pa,
				GFP_KERNEL);
		if (private_info->waveform_va == NULL) {
			TCON_ERR("allocate waveform buffer fail size:%d",
				private_info->waveform_size);
			return HWTCON_STATUS_FB_ALLOC_FAIL;
		}
	}

	hwtcon_debug_err_printf("fb buffer va: %p pa:0x%08x size:0x%x\n",
			private_info->fb_buffer_va,
			private_info->fb_buffer_pa,
			private_info->fb_buffer_size);
	if (hw_tcon_get_epd_type()) {
		hwtcon_debug_err_printf("color buffer va: %p pa:0x%08x size:0x%x\n",
			private_info->color_buffer_va,
			private_info->color_buffer_pa,
			private_info->color_buffer_size);
		hwtcon_debug_err_printf("cinfo buffer va: %p pa:0x%08x size:0x%x\n",
			private_info->cinfo_buffer_va,
			private_info->cinfo_buffer_pa,
			private_info->cinfo_buffer_size);
	}
	hwtcon_debug_err_printf("img buffer va: %p pa:0x%08x size:0x%x\n",
		private_info->img_buffer_va,
		private_info->img_buffer_pa,
		private_info->img_buffer_size);
	hwtcon_debug_err_printf("temp img buffer va: %p pa:0x%08x size:0x%x\n",
		private_info->temp_img_buffer_va,
		private_info->temp_img_buffer_pa,
		private_info->temp_img_buffer_size);
	hwtcon_debug_err_printf("wb va:%p pa:0x%08x size:0x%x reserve:%d\n",
		private_info->wb_buffer_va,
		private_info->wb_buffer_pa,
		private_info->wb_buffer_size,
		hwtcon_device_info()->reserved_buf_ready);
	hwtcon_debug_err_printf("waveform buffer va:%p pa:0x%08x size:0x%x reserve:%d\n",
		private_info->waveform_va,
		private_info->waveform_pa,
		private_info->waveform_size,
		hwtcon_device_info()->reserved_buf_ready);

	private_info->temperature = EINK_DEFAULT_TEMPERATURE;
	private_info->pipeline_busy = false;
	private_info->hwtcon_first_call = true;

	/*
	INIT_DELAYED_WORK(&private_info->read_sensor_work,
		hwtcon_fb_read_temperature_from_sensor);

	schedule_delayed_work(&private_info->read_sensor_work,
			msecs_to_jiffies(0));
	*/

	private_info->mmsys_power_enable = false;
	mutex_init(&private_info->mmsys_power_enable_lock);
	private_info->hwtcon_clk_enable = false;
	spin_lock_init(&private_info->hwtcon_clk_enable_lock);

	spin_lock_init(&private_info->hwtcon_tcon_reg_lock);

	/* start timer for mmsys power down timeout. */
	setup_timer(&private_info->mmsys_power_timer,
		hwtcon_core_handle_mmsys_power_down_cb,
		0L);

	private_info->power_down_delay_ms = EINK_DEFAULT_POWER_DOWN_TIME;
	private_info->ignore_request = false;
	private_info->current_temp_zone = -1;
	private_info->enable_night_mode = false;
	private_info->enable_night_mode_by_wfm = true;
	private_info->invert_fb = false;
	private_info->night_mode_wfm_mapping = 0;
	private_info->current_night_mode = -1;
	private_info->cfa_convert_threads = 2;

	if (0!=hw_tcon_get_epd_type()) {
		private_info->cfa_mode = HWTCON_CFA_MODE_DEFAULT;
	}


	/* init timer for LUT release */
	for (i = 0; i < MAX_LUT_REGION_COUNT; i++)
		setup_timer(&private_info->timer_lut_release[i],
		hwtcon_core_handle_lut_release_timeout_cb,
		(unsigned long)i);
	/* allocate dma buffer for lut_release_slot */
	if (cmdqBackupAllocateSlot(&private_info->lut_release_slot, LUT_RELEASE_MAX) != 0) {
		TCON_ERR("allocate lut_release_slot fail");
	}
	for (i = 0; i < LUT_RELEASE_MAX; i++)
		cmdqBackupWriteSlot(private_info->lut_release_slot, i, 0);

	init_completion(&private_info->wb_frame_done_completion);

	spin_lock_init(&private_info->lut_free_lock);
	spin_lock_init(&private_info->lut_active_lock);
	spin_lock_init(&private_info->g_update_order_lock);

	wakeup_source_init(&private_info->wake_lock, "hwtcon_wakelock");

	spin_lock_irqsave(&private_info->g_update_order_lock, flags);
	private_info->g_update_order = 0;
	spin_unlock_irqrestore(&private_info->g_update_order_lock, flags);

	spin_lock_irqsave(&private_info->lut_free_lock, flags);
	private_info->lut_free = LUT_BIT_ALL_SET;
	spin_unlock_irqrestore(&private_info->lut_free_lock, flags);

	spin_lock_irqsave(&private_info->lut_active_lock, flags);
	private_info->lut_active = 0LL;
	spin_unlock_irqrestore(&private_info->lut_active_lock, flags);

	/* init task list. */
	spin_lock_init(&private_info->fb_global_marker_list.lock);
	INIT_LIST_HEAD(&private_info->fb_global_marker_list.list);

	spin_lock_init(&private_info->free_task_list.lock);
	INIT_LIST_HEAD(&private_info->free_task_list.list);

	spin_lock_init(&private_info->wait_for_mdp_task_list.lock);
	INIT_LIST_HEAD(&private_info->wait_for_mdp_task_list.list);

	spin_lock_init(&private_info->mdp_done_task_list.lock);
	INIT_LIST_HEAD(&private_info->mdp_done_task_list.list);

	spin_lock_init(&private_info->pipeline_processing_task_list.lock);
	INIT_LIST_HEAD(&private_info->pipeline_processing_task_list.list);

	spin_lock_init(&private_info->pipeline_done_task_list.lock);
	INIT_LIST_HEAD(&private_info->pipeline_done_task_list.list);

	spin_lock_init(&private_info->collision_task_list.lock);
	INIT_LIST_HEAD(&private_info->collision_task_list.list);

	/* init wait queue */
	init_waitqueue_head(&private_info->power_state_change_wait_queue);
	init_waitqueue_head(&private_info->wf_lut_release_wait_queue);
	init_waitqueue_head(&private_info->mdp_trigger_wait_queue);
	init_waitqueue_head(&private_info->pipeline_trigger_wait_queue);
	init_waitqueue_head(&private_info->task_state_wait_queue);
	TCON_ERR("hwtcon init_waitqueue_head success");


	/* init update queue */
	mutex_init(&private_info->update_queue_mutex);
	mutex_init(&private_info->image_buffer_access_mutex);

	/* create work queue. */
	private_info->wq_power_down_mmsys =
		create_singlethread_workqueue("power_down_mmsys_domain");
	private_info->wq_pipeline_written_done =
		create_singlethread_workqueue("handle_pieline_written_done");
	private_info->wq_wf_lut_display_done =
		create_singlethread_workqueue("handle_wf_lut_display_done");

	/* init work */
	INIT_WORK(&hwtcon_fb_info()->wk_power_down_mmsys,
		hwtcon_core_handle_mmsys_power_down);

	return 0;
}

static int hwtcon_fb_release_private_fb_info(
	struct fb_private_info *private_info, struct platform_device *pdev)
{
	int i = 0;

	if (private_info->vcore_req) {
		pm_qos_remove_request(private_info->vcore_req);
		kfree(private_info->vcore_req);
		private_info->vcore_req = NULL;
	}

	if (private_info->regulator_vcore) {
		regulator_put(private_info->regulator_vcore);
		private_info->regulator_vcore = NULL;
	}
	if (hw_tcon_get_epd_type()) {
		if (private_info->fb_cache_buf != NULL)
			hwtcon_fb_free_cached_mva_buf(private_info->fb_cache_buf);

		if (private_info->img_cache_buf != NULL)
			hwtcon_fb_free_cached_mva_buf(private_info->img_cache_buf);

		if (private_info->temp_img_cache_buf != NULL)
			hwtcon_fb_free_cached_mva_buf(private_info->temp_img_cache_buf);

		if (private_info->color_cache_buf != NULL)
			hwtcon_fb_free_cached_mva_buf(private_info->color_cache_buf);

		if (private_info->cinfo_cache_buf != NULL)
			hwtcon_fb_free_cached_mva_buf(private_info->cinfo_cache_buf);
	} else {
	if (private_info->fb_buffer_va != NULL)
		dma_free_coherent(&pdev->dev, private_info->fb_buffer_size,
			private_info->fb_buffer_va,
			private_info->fb_buffer_pa);

	if (private_info->img_buffer_va != NULL)
		dma_free_coherent(&pdev->dev, private_info->img_buffer_size,
			private_info->img_buffer_va,
			private_info->img_buffer_pa);

	if (private_info->temp_img_buffer_va != NULL)
		dma_free_coherent(&pdev->dev,
			private_info->temp_img_buffer_size,
			private_info->temp_img_buffer_va,
			private_info->temp_img_buffer_pa);

	if (private_info->wb_buffer_va != NULL)
			dma_free_coherent(&pdev->dev,
				private_info->wb_buffer_size,
				private_info->wb_buffer_va,
				private_info->wb_buffer_pa);

	if (private_info->waveform_va != NULL)
			dma_free_coherent(&pdev->dev,
				private_info->waveform_size,
				private_info->waveform_va,
				private_info->waveform_pa);
	}

	/* release debug image buffer info */
	for (i = 0; i < MAX_DEBUG_IMAGE_BUFFER_COUNT; i++) {
		if (private_info->debug_img_buffer_va[i] != NULL)
			dma_free_coherent(&pdev->dev,
				private_info->debug_img_buffer_size[i],
				private_info->debug_img_buffer_va[i],
				private_info->debug_img_buffer_pa[i]);

		if (private_info->debug_img_buffer_name[i] != NULL)
			vfree(private_info->debug_img_buffer_name[i]);
	}	
	if (private_info->wq_pipeline_written_done)
		destroy_workqueue(private_info->wq_pipeline_written_done);

	if (private_info->wq_power_down_mmsys)
		destroy_workqueue(private_info->wq_power_down_mmsys);

	if (private_info->wq_wf_lut_display_done)
		destroy_workqueue(private_info->wq_wf_lut_display_done);
/*
	if (delayed_work_pending(&private_info->read_sensor_work))
		cancel_delayed_work(&private_info->read_sensor_work);
*/

	return 0;
}

struct fb_private_info *hwtcon_fb_info(void)
{
	return (struct fb_private_info *)g_framebuffer_info->par;
}
EXPORT_SYMBOL(hwtcon_fb_info);

u32 hwtcon_fb_get_rotation(void)
{
	return g_framebuffer_info->var.rotate;
}
EXPORT_SYMBOL(hwtcon_fb_get_rotation);

void hwtcon_fb_get_resolution(u32 *width, u32 *height)
{
	if (width)
		*width = g_framebuffer_info->var.xres;
	if (height)
		*height = g_framebuffer_info->var.yres;
}

u32 hwtcon_fb_get_grayscale(void)
{
	return g_framebuffer_info->var.grayscale;
}

static ssize_t cfa_mode_read(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t cfa_mode_write(struct device *dev, struct device_attribute *attr,const char *buf, size_t count);
static DEVICE_ATTR(cfa_mode, S_IWUSR | S_IRUSR|S_IROTH|S_IRGRP,
	   	cfa_mode_read, cfa_mode_write);

static ssize_t cfa_mode_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	//_dumpMDPSrcFmt();
	sprintf (buf,"%s\n",cfa_get_mode_name_by_id(hwtcon_fb_info()->cfa_mode));
	//printk("%s():%s=%d\n",__FUNCTION__,buf,(int)(hwtcon_fb_info()->mdp_src_format));
	return strlen(buf);
}

static ssize_t cfa_mode_write(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	int iCFA_mode;
	char cCFA_modeA[32] = {0,};

	
	sscanf(buf,"%s",cCFA_modeA);
	iCFA_mode = cfa_get_id_by_mode_name(cCFA_modeA);
	if(-1!=iCFA_mode) {
		hwtcon_fb_info()->cfa_mode = iCFA_mode;
	}
	else {
		char *pszCFA_mode;

		printk("avalible cfa modes : \n");
		pszCFA_mode = cfa_get_mode_name(1,&iCFA_mode);
		while(pszCFA_mode)	{
			printk(" %s(%d)\n",pszCFA_mode,iCFA_mode);
			pszCFA_mode = cfa_get_mode_name(0,&iCFA_mode);
		}
	}

	return strlen(buf);
}


static ssize_t mdp_src_format_read(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t mdp_src_format_write(struct device *dev, struct device_attribute *attr,const char *buf, size_t count);
static DEVICE_ATTR(mdp_src_format, S_IWUSR | S_IRUSR|S_IROTH|S_IRGRP,
	   	mdp_src_format_read, mdp_src_format_write);


typedef struct tagMDP_SRC_FMT_ITEM {
	u32 fmt_id;
	char *fmt_name;
} MDP_SRC_FMT_ITEM;
#define MDP_SRC_FMT_ITEM_DATA(fmt)	{V4L2_PIX_FMT_##fmt,#fmt}

static MDP_SRC_FMT_ITEM gtMDP_SRC_FMTA[] = {
	MDP_SRC_FMT_ITEM_DATA(Y8),
	MDP_SRC_FMT_ITEM_DATA(ARGB32),
};

static u32 _getMDPSrcFmtIdByName(const char *szFmtName)
{
	int i;
	u32 dwRet = (u32)-1;
	for(i=0;i<sizeof(gtMDP_SRC_FMTA)/sizeof(gtMDP_SRC_FMTA[0]);i++) {
		if(0==strcmp(szFmtName,gtMDP_SRC_FMTA[i].fmt_name)) {
			dwRet = gtMDP_SRC_FMTA[i].fmt_id;
			break;
		}
	}
	return dwRet;
}
static char *_getMDPSrcFmtNameById(u32 dwFmtId)
{
	int i;
	char *pszRet = 0;
	for(i=0;i<sizeof(gtMDP_SRC_FMTA)/sizeof(gtMDP_SRC_FMTA[0]);i++) {
		if(gtMDP_SRC_FMTA[i].fmt_id==dwFmtId) {
			pszRet = (char *)gtMDP_SRC_FMTA[i].fmt_name;
			break;
		}
	}
	return pszRet;
}
#if 0
static void _dumpMDPSrcFmt(void)
{
	int i;
	printk("%s() = {\n",__FUNCTION__);
	for(i=0;i<sizeof(gtMDP_SRC_FMTA)/sizeof(gtMDP_SRC_FMTA[0]);i++) {
		printk("{\"%s\" ,%d},\n",gtMDP_SRC_FMTA[i].fmt_name,(int)gtMDP_SRC_FMTA[i].fmt_id);
	}
	printk("}\n");
}
#endif
static ssize_t mdp_src_format_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	//_dumpMDPSrcFmt();
	sprintf (buf,"%s\n",_getMDPSrcFmtNameById(hwtcon_fb_info()->mdp_src_format));
	//printk("%s():%s=%d\n",__FUNCTION__,buf,(int)(hwtcon_fb_info()->mdp_src_format));
	return strlen(buf);
}

static ssize_t mdp_src_format_write(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	u32 dwMDPSrcId;
	char cMDPSrcFmtA[32] = {0,};

	
	sscanf(buf,"%s",cMDPSrcFmtA);
	dwMDPSrcId = _getMDPSrcFmtIdByName(cMDPSrcFmtA);

	if(dwMDPSrcId!=(u32)-1) {
		hwtcon_fb_info()->mdp_src_format = dwMDPSrcId;
		switch(dwMDPSrcId) {
		case V4L2_PIX_FMT_Y8:
			g_framebuffer_info->fix.line_length = hw_tcon_get_edp_width();
			g_framebuffer_info->fix.visual = FB_VISUAL_TRUECOLOR;
			g_framebuffer_info->var.bits_per_pixel = 8;
			g_framebuffer_info->var.grayscale = 1;
			g_framebuffer_info->var.nonstd = 1;
			break;
		case V4L2_PIX_FMT_RGB565:
			g_framebuffer_info->fix.line_length = hw_tcon_get_edp_width()<<1;
			g_framebuffer_info->fix.visual = FB_VISUAL_TRUECOLOR;
			g_framebuffer_info->var.bits_per_pixel = 16;
			g_framebuffer_info->var.grayscale = 0;
			g_framebuffer_info->var.nonstd = 0;
			break;
		case V4L2_PIX_FMT_ARGB32:
			g_framebuffer_info->fix.line_length = hw_tcon_get_edp_width()<<2;
			g_framebuffer_info->fix.visual = FB_VISUAL_TRUECOLOR;
			g_framebuffer_info->var.bits_per_pixel = 32;
			g_framebuffer_info->var.grayscale = 0;
			g_framebuffer_info->var.nonstd = 0;
			break;
		default:
			break;
		}
	}

	//printk("%s():%s=%d,line_length=%d\n",__FUNCTION__,cMDPSrcFmtA,dwMDPSrcId,g_framebuffer_info->fix.line_length);

	return strlen(buf);
}




static ssize_t vcom_read(struct device *dev, struct device_attribute *attr,char *buf);
static ssize_t vcom_write(struct device *dev, struct device_attribute *attr,const char *buf, size_t count);
static DEVICE_ATTR(vcom, S_IWUSR | S_IRUSR|S_IROTH|S_IRGRP, vcom_read, vcom_write);


static ssize_t vcom_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	int iVCOM_mV;
	iVCOM_mV = fiti_read_vcom();
	sprintf (buf,"%dmV\n",iVCOM_mV);
	return strlen(buf);
}

static ssize_t vcom_write(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	int iVCOM_mV;
	int iChk=-1;

	sscanf(buf,"%dmV",&iVCOM_mV);
	//printk("%s():user input %dmV\n",__FUNCTION__,iVCOM_mV);

	if(iVCOM_mV<0) {
		// 
		iChk = fiti_write_vcom(iVCOM_mV);
		if(iChk<0) {
			printk(KERN_ERR"VCOM %dmV write failed !\n",iVCOM_mV);
		}
	}
	else {
		printk(KERN_ERR"%s():vcom set value should<0 (%dmV)\n",
			__FUNCTION__,iVCOM_mV);
	}

	return strlen(buf);
}




static const struct attribute *sysfs_hwtcon_attrs[] = {
	&dev_attr_vcom.attr,
	&dev_attr_mdp_src_format.attr,
	&dev_attr_cfa_mode.attr,
	NULL,
};

static void hwtcon_fb_create_sys_attr(struct device * fb_dev)
{
	int	err;
	err = sysfs_create_files(&fb_dev->kobj, sysfs_hwtcon_attrs);
	if (err) {
		TCON_ERR("Can't create hwtcon attr sysfs !\n");
	}
}

int hwtcon_fb_register_fb(struct platform_device *pdev)
{
	int status = 0;
	struct device_node *np = pdev->dev.of_node;
	enum of_gpio_flags flag;
	int ret;

	/* allocate fb_info & fb_private_info */
	g_framebuffer_info = framebuffer_alloc(sizeof(struct fb_private_info),
		&pdev->dev);
	if (g_framebuffer_info == NULL) {
		TCON_ERR("framebuffer_alloc fail");
		return HWTCON_STATUS_FB_STRUCT_ALLOC_FAIL;
	}

	if (of_find_property(np, "gpio_xon", NULL)) {
		hwtcon_fb_info()->gpio_xon = of_get_named_gpio_flags(np, "gpio_xon", 0, &flag);
		if (hwtcon_fb_info()->gpio_xon == -EPROBE_DEFER) {
			dev_info(&pdev->dev, "XON GPIO requested is not here yet\n");
		}
		else {
			if (!gpio_is_valid(hwtcon_fb_info()->gpio_xon)) {
				dev_warn(&pdev->dev, "No dt property: gpio_xon\n");
			} else {

				ret = devm_gpio_request_one(&pdev->dev,
						    hwtcon_fb_info()->gpio_xon,
						    (flag & OF_GPIO_ACTIVE_LOW)
						    ? GPIOF_OUT_INIT_HIGH :
						    GPIOF_OUT_INIT_LOW,
						    "xon");
				if (ret) {
					dev_err(&pdev->dev, "failed to request xon gpio"
						" %d: %d\n", hwtcon_fb_info()->gpio_xon, ret);
					return -EINVAL;
				}
				else {
					hwtcon_fb_info()->gpio_xon_desc = gpio_to_desc(hwtcon_fb_info()->gpio_xon);
					dev_info(&pdev->dev, "Night mode XON ready ! INIT=%d,val=%d\n",
						(flag & OF_GPIO_ACTIVE_LOW)?0:1,
						gpiod_get_value(hwtcon_fb_info()->gpio_xon_desc));

					hrtimer_init(&hwtcon_fb_info()->hrt_xon_on_ctrl,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
					hwtcon_fb_info()->hrt_xon_on_ctrl.function = _hrtint_xon_on_ctrl;

					hrtimer_init(&hwtcon_fb_info()->hrt_xon_off_ctrl,CLOCK_MONOTONIC,HRTIMER_MODE_REL);
					hwtcon_fb_info()->hrt_xon_off_ctrl.function = _hrtint_xon_off_ctrl;
				}

			}
		}
	}

	if (of_find_property(np, "off_xon_1_delay_us", NULL)) {
		if(of_property_read_u32(np,"off_xon_1_delay_us",&(hwtcon_fb_info()->off_xon_1_delay_us))) {
			dev_err(&pdev->dev, "off_xon_1_delay_us reading failed !\n");
		}
	}

	if (of_find_property(np, "off_xon_0_delay_us", NULL)) {
		if(of_property_read_u32(np,"off_xon_0_delay_us",&(hwtcon_fb_info()->off_xon_0_delay_us))) {
			dev_err(&pdev->dev, "off_xon_0_delay_us reading failed !\n");
			hwtcon_fb_info()->off_xon_0_delay_us = 350000;
		}
	}

	if (of_find_property(np, "off_xon_0_day_delay_us", NULL)) {
		if(of_property_read_u32(np,"off_xon_0_day_delay_us",&(hwtcon_fb_info()->off_xon_0_day_delay_us))) {
			dev_err(&pdev->dev, "off_xon_0_day_delay_us reading failed !\n");
		}
	}
	
	hwtcon_fb_info()->nm_xon_on_with_vcom=0;
	if (of_find_property(np, "nm_xon_on_with_vcom", NULL)) {
		if(of_property_read_u32(np,"nm_xon_on_with_vcom",&(hwtcon_fb_info()->nm_xon_on_with_vcom))) {
			dev_err(&pdev->dev, "nm_xon_on_with_vcom reading failed !\n");
		}
		else {
			dev_info(&pdev->dev, "nm_xon_on_with_vcom=0x%x\n",hwtcon_fb_info()->nm_xon_on_with_vcom);
		}
	}

	status = hwtcon_fb_init_private_fb_info(
		(struct fb_private_info *)g_framebuffer_info->par, pdev);
	if (status != 0)
		return status;

	status = hwtcon_fb_init_fb_info(g_framebuffer_info);
	if (status != 0)
		return status;

	/* register frame buffer */
	status = register_framebuffer(g_framebuffer_info);
	if (status != 0) {
		TCON_ERR("register frame buffer fail:%d", status);
		return status;
	}


	status = hwtcon_fb_kthread_create();

	hwtcon_fb_create_sys_attr(&pdev->dev);

	return status;
}

int hwtcon_fb_unregister_fb(struct platform_device *pdev)
{
	if (g_framebuffer_info == NULL)
		return 0;

	hwtcon_fb_kthread_destroy();

	/* unregister frame buffer */
	unregister_framebuffer(g_framebuffer_info);

	hwtcon_fb_release_private_fb_info(
		(struct fb_private_info *)g_framebuffer_info->par, pdev);

	/* release fb_info & fb_private_info */
	framebuffer_release(g_framebuffer_info);
	return 0;
}

int hwtcon_fb_suspend(struct device *dev)
{

	if(mtk_mdp_fb_suspend()<0) {
		TCON_ERR("%s MDP fb suspend failed  !",__func__);
		return -10;
	}

	if(hwtcon_core_get_current_mdp_task()) {
		TCON_ERR("%s MDP task is pending !",__func__);
		return -11;
	}
	if(hwtcon_core_get_current_update_task()) {
		TCON_ERR("%s update task is pending !",__func__);
		return -12;
	}

	if(!g_framebuffer_info) {
		return 0;
	}

	if(timer_pending(&hwtcon_fb_info()->mmsys_power_timer)) {
		TCON_ERR("%s EPD power off timer is pending !",__func__);
		return -1;
	}

	if(!fiti_pmic_judge_power_off()) {
		TCON_ERR("%s EPD power still on ?!",__func__);
		return -3;
	}

	if(fiti_pmic_pwrwork_pending()) {
		TCON_ERR("%s PMIC power work is pending !",__func__);
		return -7;
	}

	if(1==hwtcon_fb_xon_ctrl(XON_CTRLMODE_0_TMR_IS_PENDING,0)) {
		TCON_ERR("%s xon off timer pending !",__func__);
		return -4;
	}

	if(1==hwtcon_fb_xon_ctrl(XON_CTRLMODE_1_TMR_IS_PENDING,0)) {
		TCON_ERR("%s xon on timer pending !",__func__);
		return -5;
	}


	if( time_is_after_jiffies(hwtcon_fb_info()->jiffies_xon_onoff+msecs_to_jiffies(60)) ) {
		TCON_ERR("%s waiting for xon stable !",__func__);
		return -6;
	}

	hwtcon_fb_xon_ctrl(XON_CTRLMODE_0,0);
	edp_vdd_disable();



	return 0;
}

int hwtcon_fb_resume(struct device *dev)
{


	//mtk_mdp_fb_resume();

	if(!g_framebuffer_info) {
		return 0;
	}

	edp_vdd_enable();
	hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,0);
	return 0;
}

int hwtcon_fb_poweroff(struct device *dev)
{
	u32 dwChkErrors = 0;

	if(!g_framebuffer_info) {
		return 0;
	}


	do {

		dwChkErrors = 0;

		if(timer_pending(&hwtcon_fb_info()->mmsys_power_timer)) {
			TCON_ERR("%s EPD power off timer is pending !",__func__);
			dwChkErrors |= 0x1;
		}

		if(POWER_OFF_GOING==fiti_pmic_get_current_state()) {
			TCON_ERR("%s wait EPD turning off ?!",__func__);
			dwChkErrors |= 0x20;
		}

		if(fiti_pmic_pwrwork_pending()) {
			TCON_ERR("%s PMIC power work is pending !",__func__);
			dwChkErrors |= 0x40;
		}

		if(!fiti_pmic_judge_power_off()) {
			TCON_ERR("%s EPD power still on ?!",__func__);
			dwChkErrors |= 0x2;
		}

		if(1==hwtcon_fb_xon_ctrl(XON_CTRLMODE_0_TMR_IS_PENDING,0)) {
			TCON_ERR("%s xon off timer pending !",__func__);
			dwChkErrors |= 0x4;
		}

		if(1==hwtcon_fb_xon_ctrl(XON_CTRLMODE_1_TMR_IS_PENDING,0)) {
			TCON_ERR("%s xon on timer pending !",__func__);
			dwChkErrors |= 0x8;
		}


		if( time_is_after_jiffies(hwtcon_fb_info()->jiffies_xon_onoff+msecs_to_jiffies(60)) ) {
			TCON_ERR("%s waiting for xon stable !",__func__);
			dwChkErrors |= 0x10;
		}


		if(dwChkErrors) {
			msleep(500);
		}
		else {
			break;
		}

	} while(dwChkErrors);

	return 0;
}
struct hwtcon_cache_buf *hwtcon_fb_alloc_cached_mva_buf(struct device *dev,
	size_t size,
	const gfp_t flag)
{
	int i = 0;
	int status = 0;
	struct hwtcon_cache_buf *buf = NULL;
	int entry_count = 0;

	buf = vzalloc(sizeof(*buf));
	if (!buf) {
		TCON_ERR("vmalloc hwtcon cache buffer fail");
		return NULL;
	}

	buf->dev = dev;
	buf->size = size;
	buf->alloc_va = vzalloc(size);
	if (!buf->alloc_va) {
		TCON_ERR("allocate va size:%ld fail", (unsigned long)size);
		goto ALLOC_VA_FAIL;
	}

	buf->sgt = vzalloc(sizeof(struct sg_table));
	if (!buf->sgt) {
		TCON_ERR("alloc sgt fail");
		goto ALLOC_SGT_FAIL;
	}

	buf->n_pages = (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
	buf->pages = (struct page **)vmalloc(buf->n_pages * sizeof(struct page *));
	if (buf->pages == NULL) {
		TCON_ERR("allocate pages with size:%zu fail",
			buf->n_pages * sizeof(struct page *));
		goto ALLOC_PAGES_FAIL;
	}

	for (i = 0; i < buf->n_pages; i++)
		*(buf->pages + i) = vmalloc_to_page(buf->alloc_va + PAGE_SIZE * i);

	status = sg_alloc_table_from_pages(buf->sgt, buf->pages, buf->n_pages, 0UL, size, flag);
	if (status != 0) {
		TCON_ERR("alloc table from pages fail:%d", status);
		goto ALLOC_TABLE_FAIL;
	}

	entry_count = dma_map_sg_attrs(dev, buf->sgt->sgl, buf->sgt->nents, DMA_BIDIRECTIONAL, 0);
	if (entry_count <= 0) {
		TCON_ERR("fail to map scatterlist");
		goto MAP_SG_FAIL;
	}

	buf->dma_handle = sg_dma_address(buf->sgt->sgl);

	return buf;

MAP_SG_FAIL:
	sg_free_table(buf->sgt);
ALLOC_TABLE_FAIL:
	vfree(buf->pages);
ALLOC_PAGES_FAIL:
	vfree(buf->sgt);
ALLOC_SGT_FAIL:
	vfree(buf->alloc_va);
ALLOC_VA_FAIL:
	vfree(buf);
	return NULL;
}

void hwtcon_fb_free_cached_mva_buf(struct hwtcon_cache_buf *buf)
{
	dma_unmap_sg_attrs(buf->dev, buf->sgt->sgl, buf->sgt->nents, DMA_BIDIRECTIONAL, 0);
	sg_free_table(buf->sgt);

	vfree(buf->pages);
	vfree(buf->sgt);
	vfree(buf->alloc_va);
	vfree(buf);
}

/* invalid cache, set cache data invalid.
 * when CPU try to read this buffer, CPU will ignore cache content, and read from dram.
 * called after Hardware change the dram data, or before CPU read/write this buffer.
 */
void hwtcon_fb_invalid_cache(struct hwtcon_cache_buf *buf)
{
	dma_sync_sg_for_cpu(buf->dev, buf->sgt->sgl, buf->sgt->nents, DMA_TO_DEVICE);
}

/* clean cache, flush cache content to dram.
 * called after CPU change the cache data. or before HW read/write this buffer.
 */
void hwtcon_fb_clean_cache(struct hwtcon_cache_buf *buf)
{
	dma_sync_sg_for_device(buf->dev, buf->sgt->sgl, buf->sgt->nents, DMA_TO_DEVICE);
}

void hwtcon_fb_set_last_event_sync(const char *pszEvtMsg)
{
	hwtcon_fb_info()->time_last_event_sync = timeofday_ms();
}

HWTCON_TIME hwtcon_fb_get_last_event_sync(void)
{
	return hwtcon_fb_info()->time_last_event_sync;
}


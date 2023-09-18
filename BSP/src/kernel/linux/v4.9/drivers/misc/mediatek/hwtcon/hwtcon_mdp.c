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
#include <linux/videodev2.h>
#include "hwtcon_fb.h"
#include "hwtcon_epd.h"
#include "hwtcon_debug.h"
#include "hwtcon_mdp.h"
#include "hwtcon_def.h"
#include "mtk_mdp_gamma.h"
#include "hwtcon_driver.h"

int hwtcon_mdp_copy_buffer_with_region(char *dst_buffer,
	int dst_pitch, const struct rect *dst_region,
	char *src_buffer, int src_pitch,
	const struct rect *src_region)
{
	int i = 0;
	int j = 0;

	if (src_region->width != dst_region->width ||
		src_region->height != dst_region->height) {
		TCON_ERR("copy src & dst region not match");
		return HWTCON_STATUS_REGION_NOT_MATCH;
	}

	if (dst_pitch < dst_region->width) {
		TCON_ERR("invalid region: dst(%d %d %d %d) dst pitch:%d",
			dst_region->x,
			dst_region->y,
			dst_region->width,
			dst_region->height,
			dst_pitch);
		return HWTCON_STATUS_INVALID_PARAM;
	}
	if (src_pitch < src_region->width) {
		TCON_ERR("invalid region: src(%d %d %d %d) src pitch:%d",
			src_region->x,
			src_region->y,
			src_region->width,
			src_region->height,
			src_pitch);
		return HWTCON_STATUS_INVALID_PARAM;
	}

	for (i = 0; i < src_region->width; i++)
		for (j = 0; j < src_region->height; j++) {
			/* copy src region(src_region.x, src_region.y)
			 * to dst(dst_region.x, dst_region.y)
			 */
			dst_buffer[(dst_region->y + j) * dst_pitch +
				(dst_region->x + i)] =
				src_buffer[(src_region->y + j) * src_pitch +
				(src_region->x + i)] >> 4 << 4;
		}

	return 0;
}

int hwtcon_mdp_convert(struct hwtcon_task *task)
{
	u32 src_buffer_pa = 0;
	u32 dst_buffer_pa = 0;
	u32 src_buffer_width = 0;
	u32 dst_buffer_pitch = 0;
	u32 src_buffer_height = 0;
	u32 dst_buffer_height = 0;
	struct rect src_region = {0};
	struct rect dst_region = {0};
	/* 
	select one format below:
	u32 src_format = V4L2_PIX_FMT_ARGB32;
	u32 src_format = V4L2_PIX_FMT_RGB565;
	u32 src_format = V4L2_PIX_FMT_Y8;
	*/
	//u32 src_format = V4L2_PIX_FMT_Y8;
	u32 src_format = task->mdp_src_format;

	u32 dst_format = V4L2_PIX_FMT_Y4_M0;
	u8 dither_enable = 0;
  	u32 dither_mode = MDP_DITHER_ALGO_Y8_Y4_S;	
	u8 invert_enable = 0;
	int rotate ;
	u32 gamma_flag = 0;
	int cfa_mode,task_cfa_mode;

	task_cfa_mode = HWTCON_FLAG_GET_CFA_MODE(task->update_data.flags);
	if(task_cfa_mode) {
		cfa_mode = task_cfa_mode;
	}
	else {
		cfa_mode = (hwtcon_fb_info()->cfa_mode<=0)?0:hwtcon_fb_info()->cfa_mode;
	}
	

	TCON_LOG("cfa_mode:%d",cfa_mode);

	
	if(HWTCON_CFA_MODE_NTX==cfa_mode || 
	 HWTCON_CFA_MODE_NTX_SF==cfa_mode)
	{
		rotate = hwtcon_fb_get_rotation();
		if(0==rotate||2==rotate) {
			hwtcon_core_get_mdp_input_buffer_info(task, &src_buffer_pa,
				&src_buffer_width, &src_buffer_height);
		}
		else {
			hwtcon_core_get_mdp_input_buffer_info(task, &src_buffer_pa,
				&src_buffer_height, &src_buffer_width);
		}
		dst_region = hwtcon_core_get_task_region(task);
		src_region = dst_region;
		rotate = 0;
	}
	else {
		rotate = hwtcon_fb_get_rotation() * 90;
		src_region = hwtcon_core_get_mdp_region(task);
		dst_region = hwtcon_core_get_task_region(task);
		hwtcon_core_get_mdp_input_buffer_info(task, &src_buffer_pa,
			&src_buffer_width, &src_buffer_height);
	}


	hwtcon_core_get_task_buffer_info(task, &dst_buffer_pa,
			&dst_buffer_pitch, &dst_buffer_height);


	if (hw_tcon_get_epd_type()) {
		if ( (HWTCON_FLAG_CFA_SKIP&task->update_data.flags) || (0==cfa_mode) ) 
		{
			src_format = hwtcon_fb_get_cur_color_format();
		} else {
			src_format = V4L2_PIX_FMT_Y8;
			dst_format = V4L2_PIX_FMT_Y4_M0; //cfa use y8 as input
			TCON_LOG("mdp handle normal");
			//dst_buffer_pa += 2;
		}
	} else
		src_format = task->mdp_src_format;

	if (task->update_data.flags & HWTCON_FLAG_USE_DITHERING) {
		dither_enable = 1;
		dither_mode = task->update_data.dither_mode;
		switch (dither_mode)
		{
		case MDP_DITHER_ALGO_Y8_Y1_Q:
		case MDP_DITHER_ALGO_Y8_Y1_B:
		case MDP_DITHER_ALGO_Y8_Y1_S:
			dst_format = V4L2_PIX_FMT_Y1_M1;
			break;
		default:

			break;
		}
	}

	if (hwtcon_core_invert_fb()) 
		invert_enable = 1;

       TCON_LOG("[MARKER]:%d MDP convert, dither:%d, invert:%d",
		task->update_data.update_marker,
               dither_enable, invert_enable);
	TCON_LOG("rotate:%d degree gamma:0x%08x",
		 rotate, gamma_flag);
	//if (rotate) 
	{
		TCON_LOG("[MARKER]:%d src width:%d height:%d",
			task->update_data.update_marker,
			src_buffer_width, src_buffer_height);
		TCON_LOG("region:[%d %d %d %d]",
			src_region.x,
			src_region.y,
			src_region.width,
			src_region.height);
		TCON_LOG("MARKER:%d dst width:%d height:%d",
			task->update_data.update_marker,
			dst_buffer_pitch, dst_buffer_height);
		TCON_LOG("region:[%d %d %d %d]",
			dst_region.x,
			dst_region.y,
			dst_region.width,
			dst_region.height);
	}

	if (hw_tcon_get_epd_type()) {
		if ( (task->update_data.flags&HWTCON_FLAG_CFA_SKIP) || (0==cfa_mode) ) 
			hwtcon_fb_clean_cache(hwtcon_fb_info()->fb_cache_buf); 
		else
			hwtcon_fb_clean_cache(hwtcon_fb_info()->color_cache_buf);
	}
	easy_mtk_mdp_func_v2(src_buffer_width, src_buffer_height,
				dst_buffer_pitch, dst_buffer_height,
				src_format, dst_format,
				src_buffer_width, dst_buffer_pitch,
				src_buffer_pa, dst_buffer_pa,
				src_region.x,
				src_region.y,
				src_region.width,
				src_region.height,
				dither_enable, dither_mode,
				invert_enable,
				rotate, gamma_flag, hw_tcon_get_edp_fliph(), 0);

	return 0;
}

int hwtcon_mdp_memcpy(dma_addr_t dst_buffer, dma_addr_t src_buffer)
{
	int buffer_width = hw_tcon_get_edp_width();
	int buffer_height = hw_tcon_get_edp_height();

	return easy_mtk_mdp_func_v2(buffer_width,
				buffer_height,
				buffer_width,
				buffer_height,
				V4L2_PIX_FMT_Y8, V4L2_PIX_FMT_Y8,
				buffer_width, buffer_width,
				src_buffer, dst_buffer,
				0,
				0,
				buffer_width,
				buffer_height,
				0, MDP_DITHER_ALGO_Y8_Y4_S,
				0, 0, 0, hw_tcon_get_edp_fliph(), 0);
}
			
int hwtcon_mdp_need_cfa(const struct hwtcon_task *task)
{
	if (hw_tcon_get_epd_type() == 0)
		return 0;
	if (hwtcon_fb_get_cur_color_format() == V4L2_PIX_FMT_Y8)
		return 0;
	else if (task->update_data.flags&HWTCON_FLAG_CFA_SKIP)
		return 0; 
	else if (HWTCON_FLAG_GET_CFA_MODE(task->update_data.flags))
		return 1;
	else if (hwtcon_fb_info()->cfa_mode<=0)
		return 0; 
	return 1;
}


/*
 * Copyright (C) 2021 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __MTKFB_H
#define __MTKFB_H

#include <linux/types.h>
#include <linux/fb.h>

#define MTKFB_DRIVER "mt8512-mtkfb"

static u32 MTK_FB_XRES = 800;
static u32 MTK_FB_YRES = 480;
static u32 MTK_FB_BPP = 32;
static u32 MTK_FB_PAGES = 2;

#define MTK_FB_LINE (MTK_FB_XRES * (MTK_FB_BPP >> 3))
#define MTK_FB_SIZE (MTK_FB_LINE * MTK_FB_YRES * MTK_FB_PAGES)

#define MTKFB_IOCTL_PAGE_FLIP		_IOWR('F', 1, unsigned int)
#define MTKFB_IOCTL_WAIT_FOR_VSYNC	_IOWR('F', 2, struct mtkfb_vsync_config)

enum MTK_COLOR_FORMAT {
	COLOR_FORMAT_RGB565 = 0,
	COLOR_FORMAT_RGB888 = 1,
	COLOR_FORMAT_ARGB8888 = 2,

	COLOR_FORMAT_UNKNOWN = 32,
};

struct mtkfb_device {
	void *fb_va_base;      /* MPU virtual address */
	dma_addr_t fb_pa_base; /* Bus physical address */
	unsigned long fb_size_in_byte;

	struct fb_info *fb_info; /* Linux fbdev framework data */
	struct device *dev;

	/* transformations. rotate is stored in fb_info->var */
	int xscale, yscale, mirror;
	u32 pseudo_palette[17];

	int fb_index;
	unsigned long fb_addr[3]; /* Up to three fb buffer */

	wait_queue_head_t vsync_wq;
	unsigned long long vsync_ts;

	enum MTK_COLOR_FORMAT inFormat;
};

struct mtkfb_vsync_config {
	unsigned int vsync_cnt;
	unsigned long long vsync_ts;
};

extern struct mtkfb_device *mtkfb_get_mtkfb_info(void);

#endif /* __MTKFB_H */


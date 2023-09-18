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

#ifndef __DDP_PATH_H__
#define __DDP_PATH_H__

#include <drm/drm_fb_helper.h>
#include <linux/io.h>


struct mtk_ddp_path {
	struct device *dev;
	struct device *larb_dev;
	struct clk *rdma_clk;
	struct clk *pipeline5;
	struct clk *pipeline7;
	void __iomem *regs;
};

extern struct platform_driver mtk_ddp_path_driver;

#endif // __DDP_PATH_H__


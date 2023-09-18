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

#ifndef _DDP_RDMA_H_
#define _DDP_RDMA_H_

#include <linux/io.h>
#include <stdbool.h>

#include "mtkfb.h"

enum MTK_RDMA_MODE {
	RDMA_MODE_DIRECT_LINK = 0,
	RDMA_MODE_MEMORY = 1,
};

struct mtk_ddp_rdma {
	struct clk *clk;
	void __iomem *regs;
	int irq;
	struct device *dev;
};

extern struct platform_driver mtk_ddp_rdma_driver;

#endif // _DDP_RDMA_H_


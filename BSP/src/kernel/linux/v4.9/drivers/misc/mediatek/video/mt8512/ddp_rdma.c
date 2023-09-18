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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/component.h>
#include <linux/iommu.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>

#include "ddp_rdma.h"
#include "mtkfb.h"

static size_t mtk_rdma_log_on;
#define MTK_RDMA_LOG(fmt, arg...)				\
	do {							\
		if (mtk_rdma_log_on)				\
			pr_info("DISP/RDMA " fmt, ##arg);	\
	} while (0)

#define MTK_RDMA_ERR(fmt, args...) pr_info("DISP/RDMA " fmt, ##args)

#define DISP_REG_RDMA_INT_ENABLE		0x0000

#define DISP_REG_RDMA_INT_STATUS		0x0004
#define RDMA_TARGET_LINE_INT			BIT(5)
#define RDMA_FIFO_UNDERFLOW_INT			BIT(4)
#define RDMA_EOF_ABNORMAL_INT			BIT(3)
#define RDMA_FRAME_END_INT			BIT(2)
#define RDMA_FRAME_START_INT			BIT(1)
#define RDMA_REG_UPDATE_INT			BIT(0)

#define DISP_REG_RDMA_GLOBAL_CON		0x0010
#define RDMA_ENGINE_EN				BIT(0)
#define RDMA_MODE_SEL				BIT(1)

#define DISP_REG_RDMA_SIZE_CON_0		0x0014
#define DISP_REG_RDMA_SIZE_CON_1		0x0018
#define DISP_REG_RDMA_TARGET_LINE		0x001c

#define DISP_REG_RDMA_MEM_CON			0x0024
#define MEM_CON_FLD_MEM_MODE_INPUT_SWAP		BIT(8)

#define DISP_REG_RDMA_MEM_SRC_PITCH		0x002c
#define DISP_RDMA_MEM_GMC_SETTING_0		0x0030
#define DISP_RDMA_MEM_SLOW_CON			0x0034
#define DISP_REG_RDMA_FIFO_CON			0x0040
#define DISP_REG_RDMA_MEM_START_ADDR		0x0f00

static void rdma_update_bits(struct mtk_ddp_rdma *ddp_rdma,
		unsigned int reg, unsigned int mask, unsigned int val)
{
	unsigned int tmp = readl(ddp_rdma->regs + reg);

	tmp = (tmp & ~mask) | (val & mask);
	writel(tmp, ddp_rdma->regs + reg);
}

static void mtk_ddp_path_show_regs(struct mtk_ddp_rdma *ddp_rdma)
{
	unsigned int tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_INT_ENABLE);

	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_INT_ENABLE = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_INT_STATUS);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_INT_STATUS = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_GLOBAL_CON);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_GLOBAL_CON = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_SIZE_CON_0);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_SIZE_CON_0 = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_SIZE_CON_1);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_SIZE_CON_1 = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_TARGET_LINE);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_TARGET_LINE = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_MEM_CON);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_MEM_CON = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_MEM_SRC_PITCH);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_MEM_SRC_PITCH = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_FIFO_CON);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_FIFO_CON = 0x%08x\n", tmp);

	tmp = readl(ddp_rdma->regs + DISP_REG_RDMA_MEM_START_ADDR);
	MTK_RDMA_LOG("ddp_rdma DISP_REG_RDMA_MEM_START_ADDR = 0x%08x\n", tmp);
}

static void mtk_rdma_enable_vblank(struct mtk_ddp_rdma *ddp_rdma)
{
	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_INT_ENABLE,
		RDMA_FRAME_END_INT, RDMA_FRAME_END_INT);
}

static void mtk_rdma_disable_vblank(struct mtk_ddp_rdma *ddp_rdma)
{
	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_INT_ENABLE,
		RDMA_FRAME_END_INT, 0);
}

static void mtk_rdma_start(struct mtk_ddp_rdma *ddp_rdma)
{
	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_GLOBAL_CON, RDMA_ENGINE_EN,
			 RDMA_ENGINE_EN);
}

static void mtk_rdma_stop(struct mtk_ddp_rdma *ddp_rdma)
{
	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_GLOBAL_CON, RDMA_ENGINE_EN,
		0);
}

static void mtk_rdma_config(struct mtk_ddp_rdma *ddp_rdma,
		enum MTK_RDMA_MODE mode, unsigned long address,
		enum MTK_COLOR_FORMAT inFormat,
		unsigned int width, unsigned int height)
{
	unsigned int input_swap = 0;
	unsigned int input_format_reg = 0;
	unsigned int pitch = 0;

	switch (inFormat) {
	case COLOR_FORMAT_RGB565:
		input_format_reg = 0x0;
		input_swap = 0;
		pitch = width * 2;
		break;
	case COLOR_FORMAT_RGB888:
		input_format_reg = 0x1;
		input_swap = 1;
		pitch = width * 3;
		break;
	case COLOR_FORMAT_ARGB8888:
		input_format_reg = 0x3;
		input_swap = 1;
		pitch = width * 4;
		break;
	default:
		MTK_RDMA_ERR("Unsupport this color format, format = %d\n",
			inFormat);
		return;
	}

	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_GLOBAL_CON, RDMA_MODE_SEL,
			 (mode<<1));

	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_SIZE_CON_0, 0x1fff, width);
	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_SIZE_CON_1, 0xfffff, height);

	if (mode == RDMA_MODE_MEMORY) {
		rdma_update_bits(ddp_rdma, DISP_REG_RDMA_MEM_CON, 0xf0,
			(input_format_reg << 4));
		rdma_update_bits(ddp_rdma, DISP_REG_RDMA_MEM_CON,
			MEM_CON_FLD_MEM_MODE_INPUT_SWAP,
			(input_swap << 8));
	}

	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_MEM_START_ADDR, 0xffffffff,
		address);

	rdma_update_bits(ddp_rdma, DISP_REG_RDMA_MEM_SRC_PITCH, 0xffff,
		pitch);
}

static irqreturn_t mtk_ddp_rdma_irq_handler(int irq, void *dev_id)
{
	struct mtk_ddp_rdma *ddp_rdma = dev_id;
	struct mtkfb_device *fb_dev = NULL;
	struct fb_info *fbi = NULL;

	//MTK_RDMA_LOG("%s!\n", __func__);

	/* Clear frame completion interrupt */
	writel(0x0, ddp_rdma->regs + DISP_REG_RDMA_INT_STATUS);

	fb_dev = mtkfb_get_mtkfb_info();
	if (!fb_dev || !fb_dev->fb_info) {
		MTK_RDMA_ERR("failed to get fbinfo dev!\n");
		return -EINVAL;
	}
	fbi = fb_dev->fb_info;

	mtk_rdma_config(ddp_rdma, RDMA_MODE_MEMORY,
			fb_dev->fb_addr[fb_dev->fb_index],
			fb_dev->inFormat, fbi->var.width, fbi->var.height);

	fb_dev->vsync_ts = ktime_to_ns(ktime_get());
	wake_up_interruptible(&fb_dev->vsync_wq);

	return IRQ_HANDLED;
}

static int mtk_ddp_rdma_probe(struct platform_device *pdev)
{
	struct mtk_ddp_rdma *ddp_rdma = NULL;
	struct device *dev = &pdev->dev;
	struct resource *regs;
	struct mtkfb_device *fb_dev = NULL;
	struct fb_info *fbi = NULL;
	int irq = 0;
	int ret = 0;
	int i = 0;

	MTK_RDMA_LOG("%s start!\n", __func__);

	ddp_rdma = devm_kzalloc(dev, sizeof(*ddp_rdma), GFP_KERNEL);
	if (!ddp_rdma) {
		MTK_RDMA_ERR("failed to allocate ddp_rdma!\n");
		return -ENOMEM;
	}

	ddp_rdma->dev = dev;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		MTK_RDMA_ERR("failed to get rdma irq!\n");
		return -EINVAL;
	}

	ret = devm_request_irq(dev, irq, mtk_ddp_rdma_irq_handler,
			       IRQF_TRIGGER_NONE, dev_name(dev), ddp_rdma);
	if (ret < 0) {
		MTK_RDMA_ERR("Failed to request irq %d: %d\n", irq, ret);
		return ret;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddp_rdma->regs = devm_ioremap_resource(dev, regs);
	if (!ddp_rdma->regs) {
		MTK_RDMA_ERR("failed to ioremap rdma regs addr!\n");
		return -ENOMEM;
	}

	fb_dev = mtkfb_get_mtkfb_info();
	if (!fb_dev || !fb_dev->fb_info) {
		iounmap(ddp_rdma->regs);
		MTK_RDMA_ERR("failed to get fbinfo dev!\n");
		return -EPROBE_DEFER;
	}

	fbi = fb_dev->fb_info;

	mtk_rdma_config(ddp_rdma, RDMA_MODE_MEMORY, fbi->fix.smem_start,
		fb_dev->inFormat, fbi->var.width, fbi->var.height);

	for (i = 0; i < MTK_FB_PAGES; i++)
		fb_dev->fb_addr[i] = fbi->fix.smem_start +
			i * fbi->fix.line_length * fbi->var.height;

	mtk_rdma_enable_vblank(ddp_rdma);
	mtk_rdma_start(ddp_rdma);

	platform_set_drvdata(pdev, ddp_rdma);

	mtk_ddp_path_show_regs(ddp_rdma);

	MTK_RDMA_LOG("%s end!\n", __func__);

	return 0;
}

static int mtk_ddp_rdma_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ddp_rdma *ddp_rdma = dev_get_drvdata(dev);

	mtk_rdma_disable_vblank(ddp_rdma);
	mtk_rdma_stop(ddp_rdma);

	return 0;
}

static const struct of_device_id mtk_ddp_rdma_of_ids[] = {
	{
		.compatible = "mediatek,mt8512-disp-rdma",
	},
	{
	}
};

struct platform_driver mtk_ddp_rdma_driver = {
	.probe = mtk_ddp_rdma_probe,
	.remove = mtk_ddp_rdma_remove,
	.driver = {
		.name = "mediatek-disp-path-rdma",
		.owner	= THIS_MODULE,
		.of_match_table = mtk_ddp_rdma_of_ids,
		},
};


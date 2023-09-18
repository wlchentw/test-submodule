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

#include <drm/drm_crtc.h>
#include <drm/drm_dp_helper.h>

#include <soc/mediatek/smi.h>

#include "ddp_path.h"

static size_t ddp_path_log_on;
#define DDP_PATH_LOG(fmt, arg...)				\
	do {							\
		if (ddp_path_log_on)				\
			pr_info("DISP/DDP_PATH " fmt, ##arg);	\
	} while (0)

#define DDP_PATH_ERR(fmt, args...) pr_info("DISP/DDP_PATH " fmt, ##args)


#define MMSYS_CG_CON0			0x0100
#define MMSYS_CG_CON0_DISP_RDMA		BIT(4)
#define MMSYS_CG_CON0_DISP_DPI		BIT(5)

static void ddp_path_update_bits(struct mtk_ddp_path *ddp_path,
		unsigned int reg, unsigned int mask, unsigned int val)
{
	unsigned int tmp = readl(ddp_path->regs + reg);

	tmp = (tmp & ~mask) | (val & mask);
	writel(tmp, ddp_path->regs + reg);
}

static void mtk_ddp_path_show_regs(struct mtk_ddp_path *ddp_path)
{
	unsigned int tmp = readl(ddp_path->regs + MMSYS_CG_CON0);

	DDP_PATH_LOG("ddp_path MMSYS_CG_CON0 = 0x%08x\n", tmp);
}

static void mtk_ddp_path_clock_enable(struct mtk_ddp_path *ddp_path)
{
	int ret = 0;

	ret = pm_runtime_get_sync(ddp_path->dev);
	if (ret < 0) {
		DDP_PATH_ERR("Failed to enable power domain: %d\n", ret);
		return;
	}

	ret = mtk_smi_larb_get(ddp_path->larb_dev);
	if (ret) {
		DDP_PATH_ERR("Failed to get larb: %d\n", ret);
		return;
	}

	if (ddp_path->rdma_clk) {
		ret = clk_prepare_enable(ddp_path->rdma_clk);
		if (ret) {
			DDP_PATH_ERR("Failed to enable rdma clock: %d\n",
				ret);
			return;
		}
	}

	if (ddp_path->pipeline5) {
		ret = clk_prepare_enable(ddp_path->pipeline5);
		if (ret) {
			DDP_PATH_ERR("Failed to enable pipeline5 clock: %d\n",
				ret);
			return;
		}
	}

	if (ddp_path->pipeline7) {
		ret = clk_prepare_enable(ddp_path->pipeline7);
		if (ret) {
			DDP_PATH_ERR("Failed to enable pipeline7 clock: %d\n",
				ret);
			return;
		}
	}

//	ddp_path_update_bits(ddp_path, MMSYS_CG_CON0,
//		0xffffffff, 0);
}

static void mtk_ddp_path_clock_disable(struct mtk_ddp_path *ddp_path)
{
	int ret = 0;

//	ddp_path_update_bits(ddp_path, MMSYS_CG_CON0,
//		0xffffffff, 0xffffffff);

	clk_disable_unprepare(ddp_path->rdma_clk);
	clk_disable_unprepare(ddp_path->pipeline5);
	clk_disable_unprepare(ddp_path->pipeline7);

	mtk_smi_larb_put(ddp_path->larb_dev);

	ret = pm_runtime_put(ddp_path->dev);
	if (ret < 0) {
		DDP_PATH_ERR("Failed to disable power domain: %d\n", ret);
		return;
	}

}

static int mtk_ddp_path_probe(struct platform_device *pdev)
{
	struct mtk_ddp_path *ddp_path = NULL;
	struct device *dev = &pdev->dev;
	struct resource *regs;
	struct device_node *larb_node;
	struct platform_device *larb_pdev;

	DDP_PATH_LOG("%s start!\n", __func__);

	if (!iommu_present(&platform_bus_type)) {
		DDP_PATH_ERR("iommu not ready!\n");
		return -EPROBE_DEFER;
	}

	ddp_path = devm_kzalloc(dev, sizeof(*ddp_path), GFP_KERNEL);
	if (!ddp_path) {
		DDP_PATH_ERR("failed to allocate ddp_path!\n");
		return -ENOMEM;
	}

	ddp_path->dev = dev;

	ddp_path->rdma_clk = devm_clk_get(dev, "disp_rdma");
	if (IS_ERR(ddp_path->rdma_clk)) {
		DDP_PATH_ERR("Failed to get engine clock: %d\n",
			PTR_ERR(ddp_path->rdma_clk));
		return PTR_ERR(ddp_path->rdma_clk);
	}

	ddp_path->pipeline5 = devm_clk_get(dev, "pipeline5");
	if (IS_ERR(ddp_path->pipeline5)) {
		DDP_PATH_ERR("Failed to get engine pipeline5 clock: %d\n",
			PTR_ERR(ddp_path->pipeline5));
		return PTR_ERR(ddp_path->pipeline5);
	}

	ddp_path->pipeline7 = devm_clk_get(dev, "pipeline7");
	if (IS_ERR(ddp_path->pipeline7)) {
		DDP_PATH_ERR("Failed to get engine pipeline7 clock: %d\n",
			PTR_ERR(ddp_path->pipeline7));
		return PTR_ERR(ddp_path->pipeline7);
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ddp_path->regs = devm_ioremap_resource(dev, regs);
	if (IS_ERR(ddp_path->regs)) {
		DDP_PATH_ERR("Failed to map mutex registers\n");
		return PTR_ERR(ddp_path->regs);
	}

	larb_node = of_parse_phandle(dev->of_node, "mediatek,larb", 0);
	if (!larb_node) {
		DDP_PATH_ERR("Missing mediadek,larb phandle in %s node\n",
			dev->of_node->full_name);
		return -EINVAL;
	}

	larb_pdev = of_find_device_by_node(larb_node);
	if ((!larb_pdev) || (!larb_pdev->dev.driver)) {
		DDP_PATH_ERR("Waiting for larb device %s\n",
			 larb_node->full_name);
		of_node_put(larb_node);
		return -EPROBE_DEFER;
	}
	of_node_put(larb_node);

	ddp_path->larb_dev = &larb_pdev->dev;

	pm_runtime_enable(dev);
	mtk_ddp_path_clock_enable(ddp_path);

	platform_set_drvdata(pdev, ddp_path);

	mtk_ddp_path_show_regs(ddp_path);

	DDP_PATH_LOG("%s end!\n", __func__);

	return 0;
}

static int mtk_ddp_path_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_ddp_path *ddp_path = dev_get_drvdata(dev);

	mtk_ddp_path_clock_disable(ddp_path);
	pm_runtime_disable(&pdev->dev);

	return 0;
}

static const struct of_device_id mtk_ddp_path_of_ids[] = {
	{
		.compatible = "mediatek,mt8512-disp-mmsys",
	},
	{
	}
};

struct platform_driver mtk_ddp_path_driver = {
	.probe		= mtk_ddp_path_probe,
	.remove		= mtk_ddp_path_remove,
	.driver		= {
		.name	= "mediatek-disp-path",
		.owner	= THIS_MODULE,
		.of_match_table = mtk_ddp_path_of_ids,
	},
};


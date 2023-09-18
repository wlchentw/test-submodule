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

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/compat.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>

#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/dma-buf.h>
#include <linux/leds.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/suspend.h>

#include <linux/io.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <linux/dma-buf.h>

#include "mtkfb.h"
#include "ddp_rdma.h"
#include "ddp_path.h"

static size_t mtkfb_log_on;
#define MTKFB_LOG(fmt, arg...)					\
	do {							\
		if (mtkfb_log_on)				\
			pr_info("DISP/MTKFB " fmt, ##arg);	\
	} while (0)

#define MTKFB_ERR(fmt, args...) pr_info("DISP/MTKFB " fmt, ##args)

struct mtkfb_device *g_fbdev;

struct mtkfb_device *mtkfb_get_mtkfb_info(void)
{
	return g_fbdev;
}

static int mtkfb_open(struct fb_info *info, int user)
{
	return 0;
}

static int mtkfb_release(struct fb_info *info, int user)
{
	return 0;
}

static int mtkfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			   u_int transp, struct fb_info *info)
{
	int r = 0;
	unsigned int bpp, m;

	bpp = info->var.bits_per_pixel;
	m = 1 << bpp;
	if (regno >= m) {
		r = -EINVAL;
		goto exit;
	}

	switch (bpp) {
	case 16:
		/* RGB 565 */
		((u32 *)(info->pseudo_palette))[regno] =
			((red & 0xF800) | ((green & 0xFC00) >> 5) |
			 ((blue & 0xF800) >> 11));
		break;
	case 32:
		/* ARGB8888 */
		((u32 *)(info->pseudo_palette))[regno] =
			(0xff000000) | ((red & 0xFF00) << 8) |
			((green & 0xFF00)) | ((blue & 0xFF00) >> 8);
		break;

	/* TODO: RGB888, BGR888, ABGR8888 */

	default:
		MTKFB_ERR("bpp is not support, bpp + %d\n", bpp);
		break;
	}

exit:
	return r;
}

static int mtkfb_check_var(struct fb_var_screeninfo *var,
			    struct fb_info *info)
{
	return 0;
}

static int mtkfb_set_par(struct fb_info *info)
{
	return 0;
}

static int mtkfb_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	struct fb_fix_screeninfo *fix = &info->fix;

	MTKFB_LOG("%s bpp = %d, xoffset = %d, yoffset = %d\n",
		__func__, var->bits_per_pixel, var->xoffset, var->yoffset);

	switch (var->bits_per_pixel) {
	case 16:
		g_fbdev->inFormat = COLOR_FORMAT_RGB565;
		break;
	case 24:
		g_fbdev->inFormat = COLOR_FORMAT_RGB888;
		break;
	case 32:
		g_fbdev->inFormat = COLOR_FORMAT_ARGB8888;
		break;
	default:
		MTKFB_ERR("this bpp is not supported!\n");
		return -EINVAL;
	}
	var->bits_per_pixel = MTK_FB_BPP;

	g_fbdev->fb_index = var->yoffset / var->yres;
	if (g_fbdev->fb_index + 1 > MTK_FB_PAGES) {
		MTKFB_ERR("this voffest not supported!\n");
		return -EINVAL;
	}

	return 0;
}

static int mtkfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct mtkfb_device *fbdev = (struct mtkfb_device *)info->par;

	MTKFB_LOG(
		"mtkfb: start: %lu, offset: %lu, size: %lu\n",
		 vma->vm_start, vma->vm_pgoff,
		 vma->vm_end - vma->vm_start);

	vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_page_prot =
		pgprot_writecombine(vm_get_page_prot(vma->vm_flags));

	vma->vm_flags &= ~VM_PFNMAP;
	vma->vm_pgoff = 0;

	return dma_mmap_attrs(fbdev->dev, vma, info->screen_base,
		info->fix.smem_start, info->fix.smem_len,
		DMA_ATTR_WRITE_COMBINE);
}

int _ioctl_wait_vsync(unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct mtkfb_vsync_config vsync_config;
	int ret = 0;
	unsigned long long cur_time;

	if (copy_from_user(&vsync_config, argp, sizeof(vsync_config))) {
		MTKFB_ERR("[FB]: copy_from_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	cur_time = ktime_to_ns(ktime_get());
	ret = wait_event_interruptible_timeout(g_fbdev->vsync_wq,
			cur_time < g_fbdev->vsync_ts, HZ);
	if (ret == 0)
		MTKFB_ERR("%s timeout!\n", __func__);

	vsync_config.vsync_cnt++;
	vsync_config.vsync_ts = g_fbdev->vsync_ts;
	MTKFB_LOG("%s vsync_cnt=%d, vsync_ts=%lld\n", __func__,
		vsync_config.vsync_cnt, vsync_config.vsync_ts);

	if (copy_to_user(argp, &vsync_config, sizeof(vsync_config))) {
		MTKFB_ERR("[FB]: copy_to_user failed! line:%d\n", __LINE__);
		return -EFAULT;
	}

	return ret;
}

static int mtkfb_ioctl(struct fb_info *info, unsigned int cmd,
		       unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	MTKFB_LOG("mtkfb_ioctl, info=%p, cmd nr=0x%08x, cmd size=0x%08x\n",
		info,
		(unsigned int)_IOC_NR(cmd),
		(unsigned int)_IOC_SIZE(cmd));

	switch (cmd) {
	case MTKFB_IOCTL_PAGE_FLIP:
		if (g_fbdev->fb_index)
			g_fbdev->fb_index = 0;
		else
			g_fbdev->fb_index = 1;
		return 0;
	case MTKFB_IOCTL_WAIT_FOR_VSYNC:
		return _ioctl_wait_vsync(arg);
	default:
		MTKFB_ERR(
			"mtkfb_ioctl Not support, info=%p, cmd=0x%08x, arg=0x%lx\n",
			info, (unsigned int)cmd, arg);
		return -EINVAL;
	}
}

static struct fb_ops mtkfb_ops = {
	.owner = THIS_MODULE,
	.fb_open = mtkfb_open,
	.fb_release = mtkfb_release,
	.fb_setcolreg = mtkfb_setcolreg,
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.fb_check_var = mtkfb_check_var,
	.fb_set_par = mtkfb_set_par,
	.fb_pan_display = mtkfb_pan_display,
	.fb_mmap = mtkfb_mmap,
	.fb_ioctl = mtkfb_ioctl,
};

static void mtkfb_set_fb_fix(struct mtkfb_device *fbdev)
{
	struct fb_info *fbi = fbdev->fb_info;
	struct fb_fix_screeninfo *fix = &fbi->fix;
	struct fb_var_screeninfo *var = &fbi->var;

	strncpy(fix->id, MTKFB_DRIVER, sizeof(fix->id));
	fix->type = FB_TYPE_PACKED_PIXELS;

	switch (var->bits_per_pixel) {
	case 16:
	case 24:
	case 32:
		fix->visual = FB_VISUAL_TRUECOLOR;
		break;
	case 1:
	case 2:
	case 4:
	case 8:
		fix->visual = FB_VISUAL_PSEUDOCOLOR;
		break;
	default:
		MTKFB_ERR("bpp is not support, bpp = %d\n",
				var->bits_per_pixel);
		break;
	}

	fix->accel = FB_ACCEL_NONE;
	fix->line_length = MTK_FB_LINE;

	fix->smem_len = fbdev->fb_size_in_byte;
	fix->smem_start = fbdev->fb_pa_base;

	fix->xpanstep = 0;
	fix->ypanstep = 1;
}

static void mtkfb_fbinfo_init(struct mtkfb_device *fbdev)
{
	struct fb_info *fbi = fbdev->fb_info;
	struct fb_var_screeninfo *var = &fbi->var;

	fbi->fbops = &mtkfb_ops;
	fbi->flags = FBINFO_FLAG_DEFAULT;
	fbi->screen_base = (char *)fbdev->fb_va_base;
	fbi->screen_size = fbdev->fb_size_in_byte;
	fbi->pseudo_palette = fbdev->pseudo_palette;

	var->xres = MTK_FB_XRES;
	var->yres = MTK_FB_YRES;
	var->xres_virtual = MTK_FB_XRES;
	var->yres_virtual = MTK_FB_YRES * MTK_FB_PAGES;
	var->bits_per_pixel = MTK_FB_BPP;

	switch (var->bits_per_pixel) {
	case 16:
		var->red.offset = 11;
		var->green.offset = 5;
		var->blue.offset = 0;
		var->red.length = 5;
		var->green.length = 6;
		var->blue.length = 5;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 24:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.offset = 0;
		var->transp.length = 0;
		break;
	case 32:
		var->red.offset = 16;
		var->green.offset = 8;
		var->blue.offset = 0;
		var->red.length = 8;
		var->green.length = 8;
		var->blue.length = 8;
		var->transp.offset = 24;
		var->transp.length = 8;
		break;
	default:
		break;
	}

	var->xoffset = 0;
	var->yoffset = 0;
	var->width = MTK_FB_XRES;
	var->height = MTK_FB_YRES;
	var->activate = FB_ACTIVATE_NOW;
	var->accel_flags = FB_ACCELF_TEXT;

	mtkfb_set_fb_fix(fbdev);
}

static int mtkfb_probe(struct platform_device *pdev)
{
	struct mtkfb_device *fbdev = NULL;
	struct fb_info *fbi = NULL;
	int ret = 0;

	MTKFB_LOG("%s start!\n", __func__);

	fbi = framebuffer_alloc(sizeof(struct mtkfb_device), &pdev->dev);
	if (!fbi) {
		MTKFB_ERR("unable to allocate memory for device info\n");
		return -ENOMEM;
	}

	fbdev = (struct mtkfb_device *)fbi->par;
	fbdev->fb_info = fbi;
	fbdev->inFormat = COLOR_FORMAT_ARGB8888;
	fbdev->fb_index = 0;
	fbdev->vsync_ts = 0L;
	fbdev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, fbdev);

	init_waitqueue_head(&fbdev->vsync_wq);

	fbdev->fb_size_in_byte = MTK_FB_SIZE;
#if 0
	fbdev->fb_va_base = dma_alloc_coherent(&pdev->dev,
		fbdev->fb_size_in_byte, &fbdev->fb_pa_base, GFP_KERNEL);
	if (!fbdev->fb_va_base) {
		framebuffer_release(fbdev->fb_info);
		MTKFB_ERR("failed to allocate framebuffer memory\n");
		return -ENOMEM;
	}
#else
	fbdev->fb_va_base = dma_alloc_attrs(&pdev->dev, fbdev->fb_size_in_byte,
					  &fbdev->fb_pa_base, GFP_KERNEL,
					  DMA_ATTR_WRITE_COMBINE);
	if (!fbdev->fb_va_base) {
		framebuffer_release(fbdev->fb_info);
		MTKFB_ERR("failed to allocate %zx byte dma buffer",
			fbdev->fb_size_in_byte);
		return -ENOMEM;
	}
#endif

	MTKFB_LOG(
		"MTK_FB_XRES=%d, MTKFB_YRES=%d, MTKFB_BPP=%d, MTK_FB_PAGES=%d, MTKFB_LINE=%d, MTKFB_SIZE=%d, fb_va = %p, fb_pa = %08x\n",
		MTK_FB_XRES, MTK_FB_YRES, MTK_FB_BPP, MTK_FB_PAGES,
		MTK_FB_LINE, MTK_FB_SIZE,
		fbdev->fb_va_base, (unsigned int)fbdev->fb_pa_base);

	mtkfb_fbinfo_init(fbdev);

	ret = register_framebuffer(fbi);
	if (ret != 0) {
		dma_free_coherent(&pdev->dev, fbdev->fb_size_in_byte,
			fbdev->fb_va_base, fbdev->fb_pa_base);
		framebuffer_release(fbdev->fb_info);
		MTKFB_ERR("register_framebuffer failed\n");
		return ret;
	}

	g_fbdev = fbdev;

	MTKFB_LOG("%s end!\n", __func__);

	return 0;
}

static int mtkfb_remove(struct platform_device *pdev)
{
	struct mtkfb_device *fbdev = dev_get_drvdata(&pdev->dev);

	unregister_framebuffer(fbdev->fb_info);

#if 0
	if (fbdev->fb_va_base)
		dma_free_coherent(&pdev->dev, fbdev->fb_size_in_byte,
			fbdev->fb_va_base, fbdev->fb_pa_base);
#else
	if (fbdev->fb_va_base)
		dma_free_attrs(&pdev->dev, fbdev->fb_size_in_byte,
			fbdev->fb_va_base, fbdev->fb_pa_base,
			DMA_ATTR_WRITE_COMBINE);
#endif
	if (fbdev->fb_info)
		framebuffer_release(fbdev->fb_info);

	return 0;
}

static const struct of_device_id mtkfb_of_ids[] = {
	{
		.compatible = "mediatek,MTKFB",
	},
	{
	}
};

static struct platform_driver mtkfb_driver = {
	.probe = mtkfb_probe,
	.remove = mtkfb_remove,
	.driver = {
		.name = MTKFB_DRIVER,
		.owner	= THIS_MODULE,
		.of_match_table = mtkfb_of_ids,
		},
};

static struct platform_driver * const mtk_ddp_drivers[] = {
	&mtk_ddp_path_driver,
	&mtkfb_driver,
	&mtk_ddp_rdma_driver,
};

static int __init mtkfb_init(void)
{
	int ret = 0;
	int i = 0;

	MTKFB_LOG("%s start!\n", __func__);

	for (i = 0; i < ARRAY_SIZE(mtk_ddp_drivers); i++) {
		ret = platform_driver_register(mtk_ddp_drivers[i]);
		if (ret < 0) {
			MTKFB_ERR("Failed to register %s driver: %d\n",
			       mtk_ddp_drivers[i]->driver.name, ret);
			goto err;
		}
	}

	MTKFB_LOG("%s end!\n", __func__);

	return 0;

err:
	while (--i >= 0)
		platform_driver_unregister(mtk_ddp_drivers[i]);

	return ret;
}

static void __exit mtkfb_exit(void)
{
	int i = 0;

	for (i = ARRAY_SIZE(mtk_ddp_drivers) - 1; i >= 0; i--)
		platform_driver_unregister(mtk_ddp_drivers[i]);
}

late_initcall(mtkfb_init);
module_exit(mtkfb_exit);

MODULE_DESCRIPTION("MEDIATEK framebuffer driver");
MODULE_AUTHOR("Changle Yu <Changle.Yu@mediatek.com>");
MODULE_LICENSE("GPL");


/*
 * Copyright (c) 2015-2016 MediaTek Inc.
 * Author: Houlong Wei <houlong.wei@mediatek.com>
 *         Ming Hsiu Tsai <minghsiu.tsai@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>

#include "mtk_mdp_core.h"
#include "mtk_mdp_m2m.h"
#include "mtk_mdp_regs.h"
#ifdef CONFIG_VIDEO_MEDIATEK_VCU
#include "mtk_mdp_vpu.h"
#else
#include "mtk_vpu.h"
#endif


/**
 *  struct mtk_mdp_pix_limit - image pixel size limits
 *  @org_w: source pixel width
 *  @org_h: source pixel height
 *  @target_rot_dis_w: pixel dst scaled width with the rotator is off
 *  @target_rot_dis_h: pixel dst scaled height with the rotator is off
 *  @target_rot_en_w: pixel dst scaled width with the rotator is on
 *  @target_rot_en_h: pixel dst scaled height with the rotator is on
 */
struct mtk_mdp_pix_limit {
	u16 org_w;
	u16 org_h;
	u16 target_rot_dis_w;
	u16 target_rot_dis_h;
	u16 target_rot_en_w;
	u16 target_rot_en_h;
};

static struct mtk_mdp_pix_align mtk_mdp_size_align = {
	.org_w			= 16,
	.org_h			= 16,
	.target_w		= 2,
	.target_h		= 2,
};

static const struct mtk_mdp_fmt mtk_mdp_formats[] = {
	{
		.pixelformat	= V4L2_PIX_FMT_MT21C,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.align		= &mtk_mdp_size_align,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV12,
		.depth		= { 12 },
		.row_depth	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV21M,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV21,
		.depth		= { 12 },
		.row_depth	= { 8, 4 },
		.num_planes	= 1,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVU420M,
		.depth		= { 8, 2, 2 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV420M,
		.depth		= { 8, 2, 2 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 3,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVU420,
		.depth		= { 12 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 1,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV420,
		.depth		= { 12 },
		.row_depth	= { 8, 2, 2 },
		.num_planes	= 1,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.depth		= { 16 },
		.row_depth	= { 8, 4, 4 },
		.num_planes	= 1,
		.num_comp	= 3,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV16,
		.depth		= { 16 },
		.row_depth	= { 8, 8 },
		.num_planes	= 1,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV16M,
		.depth		= { 8, 8 },
		.row_depth	= { 8, 8 },
		.num_planes	= 2,
		.num_comp	= 2,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_UYVY,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_VYUY,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_ARGB32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_ABGR32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_XRGB32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_XBGR32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_RGB24,
		.depth		= { 24 },
		.row_depth	= { 24 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_BGR24,
		.depth		= { 24 },
		.row_depth	= { 24 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y8,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y4_M0,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y4_M1,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y4_M2,
		.depth		= { 4 },
		.row_depth	= { 4 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y2_M0,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y2_M1,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y2_M2,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y2_M3,
		.depth		= { 4 },
		.row_depth	= { 4 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y1_M0,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y1_M1,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y1_M2,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y1_M2,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}, {
		.pixelformat	= V4L2_PIX_FMT_Y1_M3,
		.depth		= { 4 },
		.row_depth	= { 4 },
		.num_planes	= 1,
		.num_comp	= 1,
		.flags		= MTK_MDP_FMT_FLAG_OUTPUT |
				  MTK_MDP_FMT_FLAG_CAPTURE,
	}
};

static struct mtk_mdp_pix_limit mtk_mdp_size_max = {
	.target_rot_dis_w	= 4096,
	.target_rot_dis_h	= 4096,
	.target_rot_en_w	= 4096,
	.target_rot_en_h	= 4096,
};

static struct mtk_mdp_pix_limit mtk_mdp_size_min = {
	.org_w			= 8,
	.org_h			= 8,
	.target_rot_dis_w	= 8,
	.target_rot_dis_h	= 8,
	.target_rot_en_w	= 8,
	.target_rot_en_h	= 8,
};

/* align size for normal raster scan pixel format */
static struct mtk_mdp_pix_align mtk_mdp_rs_align = {
	.org_w			= 1,//2,
	.org_h			= 1,//2,
	.target_w		= 1,//2,
	.target_h		= 1,//2,
};

static struct mtk_mdp_variant mtk_mdp_default_variant = {
	.pix_max		= &mtk_mdp_size_max,
	.pix_min		= &mtk_mdp_size_min,
	.pix_align		= &mtk_mdp_rs_align,
	.h_scale_up_max		= 32,
	.v_scale_up_max		= 32,
	.h_scale_down_max	= 32,
	.v_scale_down_max	= 128,
};

static const struct mtk_mdp_fmt *mtk_mdp_find_fmt(u32 pixelformat, u32 type)
{
	u32 i, flag;

	flag = V4L2_TYPE_IS_OUTPUT(type) ? MTK_MDP_FMT_FLAG_OUTPUT :
					   MTK_MDP_FMT_FLAG_CAPTURE;

	for (i = 0; i < ARRAY_SIZE(mtk_mdp_formats); ++i) {
		if (!(mtk_mdp_formats[i].flags & flag))
			continue;
		if (mtk_mdp_formats[i].pixelformat == pixelformat)
			return &mtk_mdp_formats[i];
	}
	return NULL;
}

static const struct mtk_mdp_fmt *mtk_mdp_find_fmt_by_index(u32 index, u32 type)
{
	u32 i, flag, num = 0;

	flag = V4L2_TYPE_IS_OUTPUT(type) ? MTK_MDP_FMT_FLAG_OUTPUT :
					   MTK_MDP_FMT_FLAG_CAPTURE;

	for (i = 0; i < ARRAY_SIZE(mtk_mdp_formats); ++i) {
		if (!(mtk_mdp_formats[i].flags & flag))
			continue;
		if (index == num)
			return &mtk_mdp_formats[i];
		num++;
	}
	return NULL;
}

static void mtk_mdp_bound_align_image(u32 *w, unsigned int wmin,
				      unsigned int wmax, unsigned int align_w,
				      u32 *h, unsigned int hmin,
				      unsigned int hmax, unsigned int align_h)
{
	int org_w, org_h, step_w, step_h;
	int walign, halign;

	org_w = *w;
	org_h = *h;
	walign = ffs(align_w) - 1;
	halign = ffs(align_h) - 1;
	v4l_bound_align_image(w, wmin, wmax, walign, h, hmin, hmax, halign, 0);

	step_w = 1 << walign;
	step_h = 1 << halign;
	if (*w < org_w && (*w + step_w) <= wmax)
		*w += step_w;
	if (*h < org_h && (*h + step_h) <= hmax)
		*h += step_h;
}

static const struct mtk_mdp_fmt *mtk_mdp_try_fmt_mplane(struct mtk_mdp_ctx *ctx,
							struct v4l2_format *f)
{
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct mtk_mdp_fmt *fmt;
	u32 max_w, max_h, align_w, align_h;
	u32 min_w, min_h, org_w, org_h;
	int i;

	fmt = mtk_mdp_find_fmt(pix_mp->pixelformat, f->type);
	if (!fmt)
		fmt = mtk_mdp_find_fmt_by_index(0, f->type);
	if (!fmt) {
		dev_dbg(&ctx->mdp_dev->pdev->dev,
			"pixelformat format 0x%X invalid\n",
			pix_mp->pixelformat);
		return NULL;
	}

	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->pixelformat = fmt->pixelformat;
	if (!V4L2_TYPE_IS_OUTPUT(f->type)) {
		pix_mp->colorspace = ctx->colorspace;
		pix_mp->xfer_func = ctx->xfer_func;
		pix_mp->ycbcr_enc = ctx->ycbcr_enc;
		pix_mp->quantization = ctx->quant;
	}
	memset(pix_mp->reserved, 0, sizeof(pix_mp->reserved));

	max_w = variant->pix_max->target_rot_dis_w;
	max_h = variant->pix_max->target_rot_dis_h;

	if (fmt->align == NULL) {
		/* use default alignment */
		align_w = variant->pix_align->org_w;
		align_h = variant->pix_align->org_h;
	} else {
		align_w = fmt->align->org_w;
		align_h = fmt->align->org_h;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		min_w = variant->pix_min->org_w;
		min_h = variant->pix_min->org_h;
	} else {
		min_w = variant->pix_min->target_rot_dis_w;
		min_h = variant->pix_min->target_rot_dis_h;
	}

	mtk_mdp_dbg(2, "[%d] type:%d, wxh:%ux%u, align:%ux%u, max:%ux%u",
		    ctx->id, f->type, pix_mp->width, pix_mp->height,
		    align_w, align_h, max_w, max_h);
	/*
	 * To check if image size is modified to adjust parameter against
	 * hardware abilities
	 */
	org_w = pix_mp->width;
	org_h = pix_mp->height;

	mtk_mdp_bound_align_image(&pix_mp->width, min_w, max_w, align_w,
				  &pix_mp->height, min_h, max_h, align_h);

	if (org_w != pix_mp->width || org_h != pix_mp->height)
		mtk_mdp_dbg(1, "[%d] size change:%ux%u to %ux%u", ctx->id,
			    org_w, org_h, pix_mp->width, pix_mp->height);
	pix_mp->num_planes = fmt->num_planes;

	for (i = 0; i < pix_mp->num_planes; ++i) {
		int bpl = (pix_mp->width * fmt->row_depth[i]) / 8;
		int sizeimage = (pix_mp->width * pix_mp->height *
			fmt->depth[i]) / 8;

		if (pix_mp->plane_fmt[i].bytesperline < bpl)
			pix_mp->plane_fmt[i].bytesperline = bpl;
		if (pix_mp->plane_fmt[i].sizeimage < sizeimage)
			pix_mp->plane_fmt[i].sizeimage = sizeimage;

		memset(pix_mp->plane_fmt[i].reserved, 0,
		       sizeof(pix_mp->plane_fmt[i].reserved));
		mtk_mdp_dbg(2, "[%d] p%d, bpl:%u (%u), sizeimage:%u (%u)",
			ctx->id, i,
			pix_mp->plane_fmt[i].bytesperline, (u32)bpl,
			pix_mp->plane_fmt[i].sizeimage, (u32)sizeimage);
	}

	return fmt;
}

static struct mtk_mdp_frame *mtk_mdp_ctx_get_frame(struct mtk_mdp_ctx *ctx,
					    enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return &ctx->s_frame;
	return &ctx->d_frame;
}

static void mtk_mdp_check_crop_change(u32 new_w, u32 new_h, u32 *w, u32 *h)
{
	if (new_w != *w || new_h != *h) {
		mtk_mdp_dbg(1, "size change:%dx%d to %dx%d",
			    *w, *h, new_w, new_h);

		*w = new_w;
		*h = new_h;
	}
}

static int mtk_mdp_try_crop(struct mtk_mdp_ctx *ctx, u32 type,
			    struct v4l2_rect *r)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	u32 align_w, align_h, new_w, new_h;
	u32 min_w, min_h, max_w, max_h;

	if (r->top < 0 || r->left < 0) {
		dev_err(&ctx->mdp_dev->pdev->dev,
			"doesn't support negative values for top & left\n");
		return -EINVAL;
	}

	mtk_mdp_dbg(2, "[%d] type:%d, set wxh:%dx%d", ctx->id, type,
		    r->width, r->height);

	frame = mtk_mdp_ctx_get_frame(ctx, type);
	max_w = frame->width;
	max_h = frame->height;
	new_w = r->width;
	new_h = r->height;

	if (V4L2_TYPE_IS_OUTPUT(type)) {
		align_w = 1;
		align_h = 1;
		min_w = 8;
		min_h = 8;
	} else {
		align_w = variant->pix_align->target_w;
		align_h = variant->pix_align->target_h;
		if (ctx->rotation == 90 ||
		    ctx->rotation == 270) {
			max_w = frame->height;
			max_h = frame->width;
			min_w = variant->pix_min->target_rot_en_w;
			min_h = variant->pix_min->target_rot_en_h;
			new_w = r->height;
			new_h = r->width;
		} else {
			min_w = variant->pix_min->target_rot_dis_w;
			min_h = variant->pix_min->target_rot_dis_h;
		}
	}

	mtk_mdp_dbg(2, "[%d] align:%dx%d, min:%dx%d, new:%dx%d", ctx->id,
		    align_w, align_h, min_w, min_h, new_w, new_h);

	mtk_mdp_bound_align_image(&new_w, min_w, max_w, align_w,
				  &new_h, min_h, max_h, align_h);

	if (!V4L2_TYPE_IS_OUTPUT(type) &&
		(ctx->rotation == 90 ||
		ctx->rotation == 270))
		mtk_mdp_check_crop_change(new_h, new_w,
					  &r->width, &r->height);
	else
		mtk_mdp_check_crop_change(new_w, new_h,
					  &r->width, &r->height);

	mtk_mdp_dbg(2, "[%d] crop l,t,w,h:%d,%d,%d,%d, max:%dx%d", ctx->id,
		    r->left, r->top, r->width,
		    r->height, max_w, max_h);
	return 0;
}

static inline struct mtk_mdp_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_mdp_ctx, fh);
}

static inline struct mtk_mdp_ctx *ctrl_to_ctx(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct mtk_mdp_ctx, ctrl_handler);
}

void mtk_mdp_ctx_state_lock_set(struct mtk_mdp_ctx *ctx, u32 state)
{
	mutex_lock(&ctx->slock);
	ctx->state |= state;
	mutex_unlock(&ctx->slock);
}

static void mtk_mdp_ctx_state_lock_clear(struct mtk_mdp_ctx *ctx, u32 state)
{
	mutex_lock(&ctx->slock);
	ctx->state &= ~state;
	mutex_unlock(&ctx->slock);
}

static bool mtk_mdp_ctx_state_is_set(struct mtk_mdp_ctx *ctx, u32 mask)
{
	bool ret;

	mutex_lock(&ctx->slock);
	ret = (ctx->state & mask) == mask;
	mutex_unlock(&ctx->slock);
	return ret;
}

static void mtk_mdp_ctx_lock(struct vb2_queue *vq)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_lock(&ctx->mdp_dev->lock);
}

static void mtk_mdp_ctx_unlock(struct vb2_queue *vq)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);

	mutex_unlock(&ctx->mdp_dev->lock);
}

static void mtk_mdp_set_frame_size(struct mtk_mdp_frame *frame, int width,
				   int height)
{
	frame->width = width;
	frame->height = height;
	frame->crop.width = width;
	frame->crop.height = height;
	frame->crop.left = 0;
	frame->crop.top = 0;
}

static int mtk_mdp_m2m_start_streaming(struct vb2_queue *q, unsigned int count)
{
	struct mtk_mdp_ctx *ctx = q->drv_priv;
	int ret;

	ret = pm_runtime_get_sync(&ctx->mdp_dev->pdev->dev);
	if (ret < 0)
		mtk_mdp_dbg(1, "[%d] pm_runtime_get_sync failed:%d",
			    ctx->id, ret);

	return 0;
}

static void *mtk_mdp_m2m_buf_remove(struct mtk_mdp_ctx *ctx,
				    enum v4l2_buf_type type)
{
	if (V4L2_TYPE_IS_OUTPUT(type))
		return v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	else
		return v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
}

static void mtk_mdp_m2m_stop_streaming(struct vb2_queue *q)
{
	struct mtk_mdp_ctx *ctx = q->drv_priv;
	struct vb2_buffer *vb;

	vb = mtk_mdp_m2m_buf_remove(ctx, q->type);
	while (vb != NULL) {
		v4l2_m2m_buf_done(to_vb2_v4l2_buffer(vb), VB2_BUF_STATE_ERROR);
		vb = mtk_mdp_m2m_buf_remove(ctx, q->type);
	}

	pm_runtime_put(&ctx->mdp_dev->pdev->dev);
}

static void mtk_mdp_m2m_job_abort(void *priv)
{
}

/* The color format (num_planes) must be already configured. */
static void mtk_mdp_prepare_addr(struct mtk_mdp_ctx *ctx,
				 struct vb2_buffer *vb,
				 struct mtk_mdp_frame *frame,
				 struct mtk_mdp_addr *addr)
{
	u32 pix_size, planes, i;
	u32 pix_size_tmp;
	u32 offset;

	pix_size = frame->width * frame->height;
	planes = min_t(u32, frame->fmt->num_planes, ARRAY_SIZE(addr->addr));

	for (i = 0; i < planes; i++) {
		addr->addr[i] = vb2_dma_contig_plane_dma_addr(vb, i);
		if (vb->vb2_queue->memory == (u32)VB2_MEMORY_USERPTR) {
			if (V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
				addr->addr[i] += vb->planes[i].data_offset;
			} else {
				offset = frame->crop.top * frame->pitch[i] +
					frame->crop.left;
				addr->addr[i] += offset;
			}
		}
	}

	if (planes == 1 && frame->fmt->num_comp > 1) {
		pix_size_tmp = pix_size;
		if (frame->width < frame->pitch[0])
			pix_size = frame->pitch[0]*frame->height;

		if ((frame->fmt->pixelformat == V4L2_PIX_FMT_YVU420) ||
			(frame->fmt->pixelformat == V4L2_PIX_FMT_YUV420)) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = (dma_addr_t)(addr->addr[1] +
					(pix_size >> 2));
			frame->pitch[1] = frame->pitch[0]>>1;
			frame->pitch[2] = frame->pitch[0]>>1;
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_YUV422P) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = (dma_addr_t)(addr->addr[1] +
					(pix_size >> 1));
			frame->pitch[1] = frame->pitch[0]>>1;
			frame->pitch[2] = frame->pitch[0]>>1;
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_NV12 ||
			frame->fmt->pixelformat == V4L2_PIX_FMT_NV21) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = 0;
			frame->pitch[1] = frame->pitch[0];
			frame->pitch[2] = 0;
		} else if (frame->fmt->pixelformat == V4L2_PIX_FMT_NV16) {
			addr->addr[1] = (dma_addr_t)(addr->addr[0] + pix_size);
			addr->addr[2] = 0;
			frame->pitch[1] = frame->pitch[0];
			frame->pitch[2] = 0;
		} else {
			dev_err(&ctx->mdp_dev->pdev->dev,
				"Invalid pixelformat:0x%x\n",
				frame->fmt->pixelformat);
		}

		pix_size = pix_size_tmp;
	}
	mtk_mdp_dbg(3, "[%d] planes:%d, size%u, pitch:%u,%u,%u, addr:%p,%p,%p",
		ctx->id, planes, pix_size, frame->pitch[0], frame->pitch[1],
		frame->pitch[2], (void *)addr->addr[0],
		(void *)addr->addr[1], (void *)addr->addr[2]);
}

static void mtk_mdp_m2m_get_bufs(struct mtk_mdp_ctx *ctx)
{
	struct mtk_mdp_frame *s_frame, *d_frame;
	struct vb2_buffer *src_vb, *dst_vb;
	struct vb2_v4l2_buffer *src_vbuf, *dst_vbuf;

	s_frame = &ctx->s_frame;
	d_frame = &ctx->d_frame;

	src_vb = v4l2_m2m_next_src_buf(ctx->m2m_ctx);
	mtk_mdp_prepare_addr(ctx, src_vb, s_frame, &s_frame->addr);

	dst_vb = v4l2_m2m_next_dst_buf(ctx->m2m_ctx);
	mtk_mdp_prepare_addr(ctx, dst_vb, d_frame, &d_frame->addr);

	src_vbuf = to_vb2_v4l2_buffer(src_vb);
	dst_vbuf = to_vb2_v4l2_buffer(dst_vb);
	dst_vbuf->vb2_buf.timestamp = src_vbuf->vb2_buf.timestamp;
}

static void mtk_mdp_process_done(void *priv, int vb_state)
{
	struct mtk_mdp_dev *mdp = priv;
	struct mtk_mdp_ctx *ctx;
	struct vb2_buffer *src_vb, *dst_vb;
	struct vb2_v4l2_buffer *src_vbuf = NULL, *dst_vbuf = NULL;

	ctx = v4l2_m2m_get_curr_priv(mdp->m2m_dev);
	if (!ctx)
		return;

	src_vb = v4l2_m2m_src_buf_remove(ctx->m2m_ctx);
	src_vbuf = to_vb2_v4l2_buffer(src_vb);
	dst_vb = v4l2_m2m_dst_buf_remove(ctx->m2m_ctx);
	dst_vbuf = to_vb2_v4l2_buffer(dst_vb);

	dst_vbuf->vb2_buf.timestamp = src_vbuf->vb2_buf.timestamp;
	dst_vbuf->timecode = src_vbuf->timecode;
	dst_vbuf->flags &= ~V4L2_BUF_FLAG_TSTAMP_SRC_MASK;
	dst_vbuf->flags |= src_vbuf->flags & V4L2_BUF_FLAG_TSTAMP_SRC_MASK;

	v4l2_m2m_buf_done(src_vbuf, vb_state);
	v4l2_m2m_buf_done(dst_vbuf, vb_state);
	v4l2_m2m_job_finish(ctx->mdp_dev->m2m_dev, ctx->m2m_ctx);
}

static void mtk_mdp_m2m_worker(struct work_struct *work)
{
	struct mtk_mdp_ctx *ctx =
				container_of(work, struct mtk_mdp_ctx, work);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	enum vb2_buffer_state buf_state = VB2_BUF_STATE_ERROR;
	int ret;

	if (mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_CTX_ERROR)) {
		dev_err(&mdp->pdev->dev, "ctx is in error state");
		goto worker_end;
	}

	ret = mdp_wait_sync_work();
	if (!ret) {
		int cnt = mdp_get_sync_work_nr();

		pr_info("mdp sync work nr %d\n", cnt);
	}

	mdp_latency_work_inc();

	mtk_mdp_m2m_get_bufs(ctx);

	mtk_mdp_hw_set_input_addr(ctx, &ctx->s_frame.addr);
	mtk_mdp_hw_set_output_addr(ctx, &ctx->d_frame.addr);

	mtk_mdp_hw_set_in_size(ctx);
	mtk_mdp_hw_set_in_image_format(ctx);

	mtk_mdp_hw_set_out_size(ctx);
	mtk_mdp_hw_set_out_image_format(ctx);

	mtk_mdp_hw_set_rotation(ctx);
	mtk_mdp_hw_set_global_alpha(ctx);

	mtk_mdp_hw_set_pq_info(ctx);

	ctx->vpu.vsi->cmdq.engine_flag = ctx->eng_flag;

	ret = mtk_mdp_vpu_process(&ctx->vpu);
	if (ret) {
		dev_err(&mdp->pdev->dev, "processing failed: %d", ret);
		goto worker_end;
	}

	buf_state = VB2_BUF_STATE_DONE;

worker_end:
	mtk_mdp_process_done(mdp, buf_state);

	mdp_latency_work_dec();
	mdp_wakeup_latency_work();
}

static void mtk_mdp_m2m_device_run(void *priv)
{
	struct mtk_mdp_ctx *ctx = priv;

	queue_work(ctx->mdp_dev->job_wq, &ctx->work);
}

static int mtk_mdp_m2m_queue_setup(struct vb2_queue *vq,
			unsigned int *num_buffers, unsigned int *num_planes,
			unsigned int sizes[], struct device *alloc_devs[])
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_mdp_frame *frame;
	int i;

	frame = mtk_mdp_ctx_get_frame(ctx, vq->type);
	*num_planes = frame->fmt->num_planes;
	for (i = 0; i < frame->fmt->num_planes; i++)
		sizes[i] = frame->payload[i];
	mtk_mdp_dbg(2, "[%d] type:%d, planes:%d, buffers:%d, size:%u,%u",
		    ctx->id, vq->type, *num_planes, *num_buffers,
		    sizes[0], sizes[1]);
	return 0;
}

static int mtk_mdp_m2m_buf_prepare(struct vb2_buffer *vb)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct mtk_mdp_frame *frame;
	int i;

	frame = mtk_mdp_ctx_get_frame(ctx, vb->vb2_queue->type);

	if (!V4L2_TYPE_IS_OUTPUT(vb->vb2_queue->type)) {
		for (i = 0; i < frame->fmt->num_planes; i++)
			vb2_set_plane_payload(vb, i, frame->payload[i]);
	}

	return 0;
}

static void mtk_mdp_m2m_buf_queue(struct vb2_buffer *vb)
{
	struct mtk_mdp_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_m2m_buf_queue(ctx->m2m_ctx, to_vb2_v4l2_buffer(vb));
}

static struct vb2_ops mtk_mdp_m2m_qops = {
	.queue_setup	 = mtk_mdp_m2m_queue_setup,
	.buf_prepare	 = mtk_mdp_m2m_buf_prepare,
	.buf_queue	 = mtk_mdp_m2m_buf_queue,
	.wait_prepare	 = mtk_mdp_ctx_unlock,
	.wait_finish	 = mtk_mdp_ctx_lock,
	.stop_streaming	 = mtk_mdp_m2m_stop_streaming,
	.start_streaming = mtk_mdp_m2m_start_streaming,
};

static int mtk_mdp_m2m_querycap(struct file *file, void *fh,
				struct v4l2_capability *cap)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	strlcpy(cap->driver, mdp->driver, sizeof(cap->driver));
	strlcpy(cap->card, mdp->pdev->name, sizeof(cap->card));
	strlcpy(cap->bus_info, mdp->platform, sizeof(cap->bus_info));

	return 0;
}

static int mtk_mdp_enum_fmt_mplane(struct v4l2_fmtdesc *f, u32 type)
{
	const struct mtk_mdp_fmt *fmt;

	fmt = mtk_mdp_find_fmt_by_index(f->index, type);
	if (!fmt)
		return -EINVAL;

	f->pixelformat = fmt->pixelformat;

	return 0;
}

static int mtk_mdp_m2m_enum_fmt_mplane_vid_cap(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	return mtk_mdp_enum_fmt_mplane(f, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
}

static int mtk_mdp_m2m_enum_fmt_mplane_vid_out(struct file *file, void *priv,
				       struct v4l2_fmtdesc *f)
{
	return mtk_mdp_enum_fmt_mplane(f, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
}

static int mtk_mdp_m2m_g_fmt_mplane(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_frame *frame;
	struct v4l2_pix_format_mplane *pix_mp;
	int i;

	mtk_mdp_dbg(2, "[%d] type:%d", ctx->id, f->type);

	frame = mtk_mdp_ctx_get_frame(ctx, f->type);
	pix_mp = &f->fmt.pix_mp;

	pix_mp->width = frame->width;
	pix_mp->height = frame->height;
	pix_mp->field = V4L2_FIELD_NONE;
	pix_mp->pixelformat = frame->fmt->pixelformat;
	pix_mp->num_planes = frame->fmt->num_planes;
	pix_mp->colorspace = ctx->colorspace;
	pix_mp->xfer_func = ctx->xfer_func;
	pix_mp->ycbcr_enc = ctx->ycbcr_enc;
	pix_mp->quantization = ctx->quant;
	mtk_mdp_dbg(2, "[%d] wxh:%dx%d", ctx->id,
		    pix_mp->width, pix_mp->height);

	for (i = 0; i < pix_mp->num_planes; ++i) {
		pix_mp->plane_fmt[i].bytesperline = (frame->width *
			frame->fmt->depth[i]) / 8;
		pix_mp->plane_fmt[i].sizeimage = (frame->width *
			frame->height * frame->fmt->depth[i]) / 8;

		mtk_mdp_dbg(2, "[%d] p%d, bpl:%d, sizeimage:%d", ctx->id, i,
			    pix_mp->plane_fmt[i].bytesperline,
			    pix_mp->plane_fmt[i].sizeimage);
	}

	return 0;
}

static int mtk_mdp_m2m_try_fmt_mplane(struct file *file, void *fh,
				      struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	if (!mtk_mdp_try_fmt_mplane(ctx, f))
		return -EINVAL;
	return 0;
}

static int mtk_mdp_m2m_s_fmt_mplane(struct file *file, void *fh,
				    struct v4l2_format *f)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct vb2_queue *vq;
	struct mtk_mdp_frame *frame;
	struct v4l2_pix_format_mplane *pix_mp;
	const struct mtk_mdp_fmt *fmt;
	int i;

	mtk_mdp_dbg(2, "[%d] type:%d", ctx->id, f->type);

	frame = mtk_mdp_ctx_get_frame(ctx, f->type);
	fmt = mtk_mdp_try_fmt_mplane(ctx, f);
	if (!fmt) {
		mtk_mdp_err("[%d] try_fmt failed, type:%d", ctx->id, f->type);
		return -EINVAL;
	}
	frame->fmt = fmt;

	vq = v4l2_m2m_get_vq(ctx->m2m_ctx, f->type);
	if (vb2_is_streaming(vq)) {
		dev_info(&ctx->mdp_dev->pdev->dev, "queue %d busy", f->type);
		return -EBUSY;
	}

	pix_mp = &f->fmt.pix_mp;
	for (i = 0; i < frame->fmt->num_planes; i++) {
		frame->payload[i] = pix_mp->plane_fmt[i].sizeimage;
		frame->pitch[i] = pix_mp->plane_fmt[i].bytesperline;
	}

	mtk_mdp_set_frame_size(frame, pix_mp->width, pix_mp->height);
	if (V4L2_TYPE_IS_OUTPUT(f->type)) {
		ctx->colorspace = pix_mp->colorspace;
		ctx->xfer_func = pix_mp->xfer_func;
		ctx->ycbcr_enc = pix_mp->ycbcr_enc;
		ctx->quant = pix_mp->quantization;
	}

	if (V4L2_TYPE_IS_OUTPUT(f->type))
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_SRC_FMT);
	else
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_DST_FMT);

	mtk_mdp_dbg(2, "[%d] type:%d, frame:%dx%d", ctx->id, f->type,
		    frame->width, frame->height);

	return 0;
}

static int mtk_mdp_m2m_reqbufs(struct file *file, void *fh,
			       struct v4l2_requestbuffers *reqbufs)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);

	if (reqbufs->count == 0) {
		if (reqbufs->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
			mtk_mdp_ctx_state_lock_clear(ctx, MTK_MDP_SRC_FMT);
		else
			mtk_mdp_ctx_state_lock_clear(ctx, MTK_MDP_DST_FMT);
	}

	return v4l2_m2m_reqbufs(file, ctx->m2m_ctx, reqbufs);
}

static int mtk_mdp_m2m_streamon(struct file *file, void *fh,
				enum v4l2_buf_type type)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	int ret;

	/* The source and target color format need to be set */
	if (V4L2_TYPE_IS_OUTPUT(type)) {
		if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_SRC_FMT))
			return -EINVAL;
	} else if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_DST_FMT)) {
		return -EINVAL;
	}

	if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_VPU_INIT)) {
		ret = mtk_mdp_vpu_init(&ctx->vpu);
		if (ret < 0) {
			dev_err(&ctx->mdp_dev->pdev->dev,
				"vpu init failed %d\n",
				ret);
			return -EINVAL;
		}
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_VPU_INIT);
	}

	return v4l2_m2m_streamon(file, ctx->m2m_ctx, type);
}

static inline bool mtk_mdp_is_target_compose(u32 target)
{
	if (target == V4L2_SEL_TGT_COMPOSE_DEFAULT
	    || target == V4L2_SEL_TGT_COMPOSE_BOUNDS
	    || target == V4L2_SEL_TGT_COMPOSE)
		return true;
	return false;
}

static inline bool mtk_mdp_is_target_crop(u32 target)
{
	if (target == V4L2_SEL_TGT_CROP_DEFAULT
	    || target == V4L2_SEL_TGT_CROP_BOUNDS
	    || target == V4L2_SEL_TGT_CROP)
		return true;
	return false;
}

static int mtk_mdp_m2m_g_selection(struct file *file, void *fh,
				       struct v4l2_selection *s)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	bool valid = false;

	if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (mtk_mdp_is_target_compose(s->target))
			valid = true;
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (mtk_mdp_is_target_crop(s->target))
			valid = true;
	}
	if (!valid) {
		mtk_mdp_dbg(1, "[%d] invalid type:%d,%u", ctx->id, s->type,
			    s->target);
		return -EINVAL;
	}

	frame = mtk_mdp_ctx_get_frame(ctx, s->type);

	switch (s->target) {
	case V4L2_SEL_TGT_COMPOSE_DEFAULT:
	case V4L2_SEL_TGT_COMPOSE_BOUNDS:
	case V4L2_SEL_TGT_CROP_BOUNDS:
	case V4L2_SEL_TGT_CROP_DEFAULT:
		s->r.left = 0;
		s->r.top = 0;
		s->r.width = frame->width;
		s->r.height = frame->height;
		return 0;

	case V4L2_SEL_TGT_COMPOSE:
	case V4L2_SEL_TGT_CROP:
		s->r.left = frame->crop.left;
		s->r.top = frame->crop.top;
		s->r.width = frame->crop.width;
		s->r.height = frame->crop.height;
		return 0;
	}

	return -EINVAL;
}

static int mtk_mdp_check_scaler_ratio(struct mtk_mdp_variant *var, int src_w,
				      int src_h, int dst_w, int dst_h, int rot)
{
	int tmp_w, tmp_h;

	if (rot == 90 || rot == 270) {
		tmp_w = dst_h;
		tmp_h = dst_w;
	} else {
		tmp_w = dst_w;
		tmp_h = dst_h;
	}

	if ((src_w / tmp_w) > var->h_scale_down_max ||
	    (src_h / tmp_h) > var->v_scale_down_max ||
	    (tmp_w / src_w) > var->h_scale_up_max ||
	    (tmp_h / src_h) > var->v_scale_up_max)
		return -EINVAL;

	return 0;
}

static int mtk_mdp_m2m_s_selection(struct file *file, void *fh,
				   struct v4l2_selection *s)
{
	struct mtk_mdp_frame *frame;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct v4l2_rect new_r;
	struct mtk_mdp_variant *variant = ctx->mdp_dev->variant;
	int ret;
	bool valid = false;

	if (s->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		if (s->target == V4L2_SEL_TGT_COMPOSE)
			valid = true;
	} else if (s->type == V4L2_BUF_TYPE_VIDEO_OUTPUT) {
		if (s->target == V4L2_SEL_TGT_CROP)
			valid = true;
	}
	if (!valid) {
		mtk_mdp_dbg(1, "[%d] invalid type:%d,%u", ctx->id, s->type,
			    s->target);
		return -EINVAL;
	}

	new_r = s->r;
	ret = mtk_mdp_try_crop(ctx, s->type, &new_r);
	if (ret)
		return ret;

	if (mtk_mdp_is_target_crop(s->target))
		frame = &ctx->s_frame;
	else
		frame = &ctx->d_frame;

	/* Check to see if scaling ratio is within supported range */
	if (mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_DST_FMT | MTK_MDP_SRC_FMT)) {
		if (V4L2_TYPE_IS_OUTPUT(s->type)) {
			ret = mtk_mdp_check_scaler_ratio(variant, new_r.width,
				new_r.height, ctx->d_frame.crop.width,
				ctx->d_frame.crop.height,
				ctx->rotation);
		} else {
			ret = mtk_mdp_check_scaler_ratio(variant,
				ctx->s_frame.crop.width,
				ctx->s_frame.crop.height, new_r.width,
				new_r.height, ctx->rotation);
		}

		if (ret) {
			dev_info(&ctx->mdp_dev->pdev->dev,
				"Out of scaler range");
			return -EINVAL;
		}
	}

	s->r = new_r;
	frame->crop = new_r;

	return 0;
}

enum {
	MTK_MDP_EXT_CON_SET_SHARPNESS = 0,
	MTK_MDP_EXT_CON_SET_RSZ,
	MTK_MDP_EXT_CON_SET_GAMMA,
	MTK_MDP_EXT_CON_SET_DTH,
	MTK_MDP_EXT_CON_SET_INVERT
};

enum {
	MTK_MDP_EXT_CON_GET_SHARPNESS = 0,
	MTK_MDP_EXT_CON_GET_RSZ,
	MTK_MDP_EXT_CON_GET_GAMMA,
	MTK_MDP_EXT_CON_GET_DTH,
	MTK_MDP_EXT_CON_GET_INVERT
};

struct SharpParam {
	uint32_t en;
	uint32_t level;
};

struct GammaParam {
	uint32_t en;
	uint32_t type;
	u8 lut[256];
};

struct DthParam {
	uint32_t en;
	uint32_t algo;
};

struct RszParam {
	uint32_t algo;
};

union mtk_mdp_ext_con_data {
	struct SharpParam sharp;
	struct GammaParam gamma;
	struct DthParam dth;
	struct RszParam rsz;
	uint32_t invert;
};

struct mtk_mdp_ext_con_param {
	unsigned int cmd;
	union mtk_mdp_ext_con_data data;
};

static int mtk_mdp_s_ext_controls(
		struct file *file, void *fh, struct v4l2_ext_controls *s)
{
	int ret = 0;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_ext_con_param param;

	if (copy_from_user(&param,
		(void *)s->controls->ptr,
		sizeof(struct mtk_mdp_ext_con_param)) != 0U) {
		ret = -1;

		return ret;
	}

	switch (param.cmd) {
	case MTK_MDP_EXT_CON_SET_SHARPNESS:
		ctx->pq.sharpness_enable = param.data.sharp.en;
		ctx->pq.sharpness_level = param.data.sharp.level;
		break;
	case MTK_MDP_EXT_CON_SET_RSZ:
		ctx->pq.rsz_algo = param.data.rsz.algo;
		break;
	case MTK_MDP_EXT_CON_SET_GAMMA:
		ctx->pq.gamma_enable = param.data.gamma.en;
		ctx->pq.gamma_type = param.data.gamma.type;
		memcpy(ctx->pq.gamma_table, param.data.gamma.lut, 256);
		break;
	case MTK_MDP_EXT_CON_SET_DTH:
		ctx->pq.dth_enable = param.data.dth.en;
		ctx->pq.dth_algo = param.data.dth.algo;
		break;
	case MTK_MDP_EXT_CON_SET_INVERT:
		ctx->pq.invert = param.data.invert;
		break;
	default:
		ret = -2;
		break;
	}

	return ret;
}

static int mtk_mdp_g_ext_controls(
	struct file *file, void *fh, struct v4l2_ext_controls *s)
{
	return 0;
}

static int mtk_mdp_s_control(
	struct file *file, void *fh, struct v4l2_control *ctrl)
{
	int ret = 0;
	struct mtk_mdp_ctx *ctx = fh_to_ctx(fh);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	u32 state = MTK_MDP_DST_FMT | MTK_MDP_SRC_FMT;

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ctx->hflip = ctrl->value;
		break;
	case V4L2_CID_VFLIP:
		ctx->vflip = ctrl->value;
		break;
	case V4L2_CID_ROTATE:
		if (mtk_mdp_ctx_state_is_set(ctx, state)) {
			ret = mtk_mdp_check_scaler_ratio(variant,
					ctx->s_frame.crop.width,
					ctx->s_frame.crop.height,
					ctx->d_frame.crop.width,
					ctx->d_frame.crop.height,
					ctrl->value);

			if (ret)
				return -EINVAL;
		}

		ctx->rotation = ctrl->value;
		break;
	case V4L2_CID_ALPHA_COMPONENT:
		ctx->d_frame.alpha = ctrl->value;
		break;
	default:
		ret = -2;
		break;
	}

	return ret;
}

static int mtk_mdp_g_control(
	struct file *file, void *fh, struct v4l2_control *s)
{
	return 0;
}

static const struct v4l2_ioctl_ops mtk_mdp_m2m_ioctl_ops = {
	.vidioc_querycap		= mtk_mdp_m2m_querycap,
	.vidioc_enum_fmt_vid_cap_mplane	= mtk_mdp_m2m_enum_fmt_mplane_vid_cap,
	.vidioc_enum_fmt_vid_out_mplane	= mtk_mdp_m2m_enum_fmt_mplane_vid_out,
	.vidioc_g_fmt_vid_cap_mplane	= mtk_mdp_m2m_g_fmt_mplane,
	.vidioc_g_fmt_vid_out_mplane	= mtk_mdp_m2m_g_fmt_mplane,
	.vidioc_try_fmt_vid_cap_mplane	= mtk_mdp_m2m_try_fmt_mplane,
	.vidioc_try_fmt_vid_out_mplane	= mtk_mdp_m2m_try_fmt_mplane,
	.vidioc_s_fmt_vid_cap_mplane	= mtk_mdp_m2m_s_fmt_mplane,
	.vidioc_s_fmt_vid_out_mplane	= mtk_mdp_m2m_s_fmt_mplane,
	.vidioc_reqbufs			= mtk_mdp_m2m_reqbufs,
	.vidioc_create_bufs		= v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf			= v4l2_m2m_ioctl_expbuf,
	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,
	.vidioc_querybuf		= v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf			= v4l2_m2m_ioctl_qbuf,
	.vidioc_dqbuf			= v4l2_m2m_ioctl_dqbuf,
	.vidioc_streamon		= mtk_mdp_m2m_streamon,
	.vidioc_streamoff		= v4l2_m2m_ioctl_streamoff,
	.vidioc_g_selection		= mtk_mdp_m2m_g_selection,
	.vidioc_s_selection		= mtk_mdp_m2m_s_selection,
	.vidioc_s_ext_ctrls             = mtk_mdp_s_ext_controls,
	.vidioc_g_ext_ctrls             = mtk_mdp_g_ext_controls,
	.vidioc_s_ctrl			= mtk_mdp_s_control,
	.vidioc_g_ctrl			= mtk_mdp_g_control
};

static int mtk_mdp_m2m_queue_init(void *priv, struct vb2_queue *src_vq,
				  struct vb2_queue *dst_vq)
{
	struct mtk_mdp_ctx *ctx = priv;
	int ret;

	memset(src_vq, 0, sizeof(*src_vq));
	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_mdp_m2m_qops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->dev = &ctx->mdp_dev->pdev->dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	memset(dst_vq, 0, sizeof(*dst_vq));
	dst_vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	dst_vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_mdp_m2m_qops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->dev = &ctx->mdp_dev->pdev->dev;

	return vb2_queue_init(dst_vq);
}

#define V4L2_CID_MDP_SHARPNESS_ENABLE        (V4L2_CID_USER_MTK_MDP_BASE + 0)
#define V4L2_CID_MDP_SHARPNESS_LEVEL         (V4L2_CID_USER_MTK_MDP_BASE + 1)
#define V4L2_CID_MDP_DYNAMIC_CONTRAST_ENABLE (V4L2_CID_USER_MTK_MDP_BASE + 2)
#define V4L2_CID_MDP_BRIGHTNESS_ENABLE       (V4L2_CID_USER_MTK_MDP_BASE + 7)
#define V4L2_CID_MDP_BRIGHTNESS_LEVEL        (V4L2_CID_USER_MTK_MDP_BASE + 8)
#define V4L2_CID_MDP_CONTRAST_ENABLE         (V4L2_CID_USER_MTK_MDP_BASE + 9)
#define V4L2_CID_MDP_CONTRAST_LEVEL          (V4L2_CID_USER_MTK_MDP_BASE + 10)
#define V4L2_CID_MDP_GET_CONTRAST_LEVEL      (V4L2_CID_USER_MTK_MDP_BASE + 17)
#define V4L2_CID_MDP_GET_BRIGHTNESS_LEVEL    (V4L2_CID_USER_MTK_MDP_BASE + 18)
#define V4L2_CID_MDP_STORE_CONTRAST_LEVEL    (V4L2_CID_USER_MTK_MDP_BASE + 19)
#define V4L2_CID_MDP_STORE_BRIGHTNESS_LEVEL  (V4L2_CID_USER_MTK_MDP_BASE + 20)

/* Specify eng_flag for test */
#define V4L2_CID_MDP_ENG                     (V4L2_CID_USER_MTK_MDP_BASE + 30)

static int vpu_init_state(struct mtk_mdp_ctx *ctx)
{
	int ret = 0;

	if (!mtk_mdp_ctx_state_is_set(ctx, MTK_MDP_VPU_INIT)) {
		ret = mtk_mdp_vpu_init(&ctx->vpu);
		if (ret < 0) {
			dev_info(&ctx->mdp_dev->pdev->dev,
				"vpu init failed %d\n",
				ret);
			return -EINVAL;
		}
		mtk_mdp_ctx_state_lock_set(ctx, MTK_MDP_VPU_INIT);
	}

	return ret;
}

static int mtk_mdp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_mdp_ctx *ctx = ctrl_to_ctx(ctrl);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_variant *variant = mdp->variant;
	u32 state = MTK_MDP_DST_FMT | MTK_MDP_SRC_FMT;
	int ret = 0;

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

	if (ctrl->id == V4L2_CID_MDP_STORE_CONTRAST_LEVEL ||
		ctrl->id == V4L2_CID_MDP_STORE_BRIGHTNESS_LEVEL) {
		ret = vpu_init_state(ctx);
		if (ret)
			return ret;
		if (ctrl->id == V4L2_CID_MDP_STORE_CONTRAST_LEVEL) {
			ctx->vpu.pq_cfg.store_para_type |= MTK_MDP_CONT_FLAG;
			ctx->vpu.pq_cfg.get_para_type = 0;
			ctx->vpu.pq_cfg.store_cont_level =
					(unsigned int)ctrl->val;
			mtk_mdp_send_pq_msg(&ctx->vpu,
				(uint32_t)AP_MDP_PQ_CONFIG);
		} else if (ctrl->id == V4L2_CID_MDP_STORE_BRIGHTNESS_LEVEL) {
			ctx->vpu.pq_cfg.store_para_type |= MTK_MDP_BRIT_FLAG;
			ctx->vpu.pq_cfg.get_para_type = 0;
			ctx->vpu.pq_cfg.store_brit_level =
					(unsigned int)ctrl->val;
			mtk_mdp_send_pq_msg(&ctx->vpu,
				(uint32_t)AP_MDP_PQ_CONFIG);
		}
		return ret;
	}

	switch (ctrl->id) {
	case V4L2_CID_HFLIP:
		ctx->hflip = ctrl->val;
		break;
	case V4L2_CID_VFLIP:
		ctx->vflip = ctrl->val;
		break;
	case V4L2_CID_ROTATE:
		if (mtk_mdp_ctx_state_is_set(ctx, state)) {
			ret = mtk_mdp_check_scaler_ratio(variant,
					ctx->s_frame.crop.width,
					ctx->s_frame.crop.height,
					ctx->d_frame.crop.width,
					ctx->d_frame.crop.height,
					ctx->ctrls.rotate->val);

			if (ret)
				return -EINVAL;
		}

		ctx->rotation = ctrl->val;
		break;
	case V4L2_CID_ALPHA_COMPONENT:
		ctx->d_frame.alpha = ctrl->val;
		break;
	case V4L2_CID_MDP_SHARPNESS_ENABLE:
		ctx->pq.sharpness_enable = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_SHARPNESS_LEVEL:
		ctx->pq.sharpness_level = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_DYNAMIC_CONTRAST_ENABLE:
		ctx->pq.dynamic_contrast_enable = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_BRIGHTNESS_ENABLE:
		ctx->pq.brightness_enable = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_BRIGHTNESS_LEVEL:
		ctx->pq.brightness_level = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_CONTRAST_ENABLE:
		ctx->pq.contrast_enable = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_CONTRAST_LEVEL:
		ctx->pq.contrast_level = (unsigned int)ctrl->val;
		break;
	case V4L2_CID_MDP_ENG:
		ctx->eng_flag = (u32)ctrl->val;
		break;
	}

	return 0;
}

static int mtk_mdp_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct mtk_mdp_ctx *ctx = ctrl_to_ctx(ctrl);
	int ret = -EINVAL;

	if (ctrl->id == V4L2_CID_MDP_GET_CONTRAST_LEVEL ||
		ctrl->id == V4L2_CID_MDP_GET_BRIGHTNESS_LEVEL) {
		ret = vpu_init_state(ctx);

		if (ret)
			return ret;

		if (ctrl->id == V4L2_CID_MDP_GET_CONTRAST_LEVEL) {
			ctx->vpu.pq_cfg.get_para_type |= MTK_MDP_CONT_FLAG;
			ctx->vpu.pq_cfg.store_para_type = 0;
			ret = mtk_mdp_send_pq_msg(&ctx->vpu,
				(uint32_t)AP_MDP_PQ_CONFIG);
			ctrl->val = ctx->vpu.pq_cfg_ack.get_cont_level;
		} else if (ctrl->id == V4L2_CID_MDP_GET_BRIGHTNESS_LEVEL) {
			ctx->vpu.pq_cfg.get_para_type |= MTK_MDP_BRIT_FLAG;
			ctx->vpu.pq_cfg.store_para_type = 0;
			ret = mtk_mdp_send_pq_msg(&ctx->vpu,
				(uint32_t)AP_MDP_PQ_CONFIG);
			ctrl->val = ctx->vpu.pq_cfg_ack.get_brit_level;
		}

	}
	return ret;
}

static const struct v4l2_ctrl_ops mtk_mdp_ctrl_ops = {
	.g_volatile_ctrl = mtk_mdp_g_ctrl,
	.s_ctrl = mtk_mdp_s_ctrl,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_sharpness_enable = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_SHARPNESS_ENABLE,
	.name = "MDP SHARPNESS ENABLE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_sharpness_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_SHARPNESS_LEVEL,
	.name = "MDP SHARPNESS LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 4096,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_dynamic_contrast_enable = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_DYNAMIC_CONTRAST_ENABLE,
	.name = "MDP DYNAMIC CONTRAST ENABLE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_eng_flag = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_ENG,
	.name = "MDP ENG FLAG",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 0x7fffffff,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_brightness_enable = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_BRIGHTNESS_ENABLE,
	.name = "MDP BRIGHTNESS ENABLE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_brightness_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_BRIGHTNESS_LEVEL,
	.name = "MDP BRIGHTNESS LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 512,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_contrast_enable = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_CONTRAST_ENABLE,
	.name = "MDP CONTRAST ENABLE",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_contrast_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_CONTRAST_LEVEL,
	.name = "MDP CONTRAST LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 512,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_get_brightness_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_GET_CONTRAST_LEVEL,
	.name = "MDP GET BRIGHTNESS LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_get_contrast_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_GET_BRIGHTNESS_LEVEL,
	.name = "MDP GET CONTRAST LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_store_brightness_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_STORE_BRIGHTNESS_LEVEL,
	.name = "MDP STORE BRIGHTNESS LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 512,
	.step = 1,
	.def = 0,
};

static const struct v4l2_ctrl_config mtk_mdp_ctrl_store_contrast_level = {
	.ops = &mtk_mdp_ctrl_ops,
	.id = V4L2_CID_MDP_STORE_CONTRAST_LEVEL,
	.name = "MDP STORE CONTRAST LEVEL",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 512,
	.step = 1,
	.def = 0,
};

static int mtk_mdp_ctrls_create(struct mtk_mdp_ctx *ctx)
{
	v4l2_ctrl_handler_init(&ctx->ctrl_handler, MTK_MDP_MAX_CTRL_NUM);

	ctx->ctrls.rotate = v4l2_ctrl_new_std(&ctx->ctrl_handler,
			&mtk_mdp_ctrl_ops, V4L2_CID_ROTATE, 0, 270, 90, 0);
	ctx->ctrls.hflip = v4l2_ctrl_new_std(&ctx->ctrl_handler,
					     &mtk_mdp_ctrl_ops,
					     V4L2_CID_HFLIP,
					     0, 1, 1, 0);
	ctx->ctrls.vflip = v4l2_ctrl_new_std(&ctx->ctrl_handler,
					     &mtk_mdp_ctrl_ops,
					     V4L2_CID_VFLIP,
					     0, 1, 1, 0);
	ctx->ctrls.global_alpha = v4l2_ctrl_new_std(&ctx->ctrl_handler,
						    &mtk_mdp_ctrl_ops,
						    V4L2_CID_ALPHA_COMPONENT,
						    0, 255, 1, 0);

	ctx->ctrls.ctrl_sharpness_enable =
		v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_sharpness_enable,
			NULL);

	ctx->ctrls.ctrl_sharpness_level =
		v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_sharpness_level,
			NULL);

	ctx->ctrls.ctrl_dynamic_contrast_enable =
		v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_dynamic_contrast_enable,
			NULL);

	ctx->ctrls.eng_flag = v4l2_ctrl_new_custom(&ctx->ctrl_handler,
			&mtk_mdp_eng_flag, NULL);

	ctx->ctrls.ctrl_brightness_enable = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_brightness_enable,
			NULL);

	ctx->ctrls.ctrl_brightness_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_brightness_level,
			NULL);

	ctx->ctrls.ctrl_contrast_enable = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_contrast_enable,
			NULL);

	ctx->ctrls.ctrl_contrast_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_contrast_level,
			NULL);

	ctx->ctrls.ctrl_store_contrast_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_store_contrast_level,
			NULL);

	ctx->ctrls.ctrl_store_brightness_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_store_brightness_level,
			NULL);

	ctx->ctrls.ctrl_get_contrast_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_get_contrast_level,
			NULL);
	ctx->ctrls.ctrl_get_contrast_level->flags
		|= V4L2_CTRL_FLAG_VOLATILE;

	ctx->ctrls.ctrl_get_brightness_level = v4l2_ctrl_new_custom(
			&ctx->ctrl_handler,
			&mtk_mdp_ctrl_get_brightness_level,
			NULL);
	ctx->ctrls.ctrl_get_brightness_level->flags
		|= V4L2_CTRL_FLAG_VOLATILE;

	ctx->ctrls_rdy = ctx->ctrl_handler.error == 0;

	if (ctx->ctrl_handler.error) {
		int err = ctx->ctrl_handler.error;

		v4l2_ctrl_handler_free(&ctx->ctrl_handler);
		dev_err(&ctx->mdp_dev->pdev->dev,
			"Failed to create control handlers\n");
		return err;
	}

	return 0;
}

static void mtk_mdp_set_default_params(struct mtk_mdp_ctx *ctx)
{
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;
	struct mtk_mdp_frame *frame;

	frame = mtk_mdp_ctx_get_frame(ctx, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	frame->fmt = mtk_mdp_find_fmt_by_index(0,
					V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE);
	frame->width = mdp->variant->pix_min->org_w;
	frame->height = mdp->variant->pix_min->org_h;
	frame->payload[0] = frame->width * frame->height;
	frame->payload[1] = frame->payload[0] / 2;

	frame = mtk_mdp_ctx_get_frame(ctx, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	frame->fmt = mtk_mdp_find_fmt_by_index(0,
					V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	frame->width = mdp->variant->pix_min->target_rot_dis_w;
	frame->height = mdp->variant->pix_min->target_rot_dis_h;
	frame->payload[0] = frame->width * frame->height;
	frame->payload[1] = frame->payload[0] / 2;

}

static int mtk_mdp_m2m_open(struct file *file)
{
	struct mtk_mdp_dev *mdp = video_drvdata(file);
	struct video_device *vfd = video_devdata(file);
	struct mtk_mdp_ctx *ctx = NULL;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	if (mutex_lock_interruptible(&mdp->lock)) {
		ret = -ERESTARTSYS;
		goto err_lock;
	}

	mutex_init(&ctx->slock);
	ctx->id = mdp->id_counter++;
	v4l2_fh_init(&ctx->fh, vfd);
	file->private_data = &ctx->fh;
	if (0) {
		ret = mtk_mdp_ctrls_create(ctx);
		if (ret)
			goto error_ctrls;
		/* Use separate control handler per file handle */
		ctx->fh.ctrl_handler = &ctx->ctrl_handler;
	} else
		ctx->fh.ctrl_handler = NULL;
	v4l2_fh_add(&ctx->fh);
	INIT_LIST_HEAD(&ctx->list);

	ctx->mdp_dev = mdp;
	mtk_mdp_set_default_params(ctx);

	INIT_WORK(&ctx->work, mtk_mdp_m2m_worker);
	ctx->m2m_ctx = v4l2_m2m_ctx_init(mdp->m2m_dev, ctx,
					 mtk_mdp_m2m_queue_init);
	if (IS_ERR(ctx->m2m_ctx)) {
		dev_err(&mdp->pdev->dev, "Failed to initialize m2m context");
		ret = PTR_ERR(ctx->m2m_ctx);
		goto error_m2m_ctx;
	}
	ctx->fh.m2m_ctx = ctx->m2m_ctx;
	if (mdp->ctx_num++ == 0) {
		ret = vpu_load_firmware(mdp->vpu_dev);
		if (ret < 0) {
			dev_err(&mdp->pdev->dev,
				"vpu_load_firmware failed %d\n", ret);
			goto err_load_vpu;
		}

		ret = mtk_mdp_vpu_register(mdp->pdev);
		if (ret < 0) {
			dev_err(&mdp->pdev->dev,
				"mdp_vpu register failed %d\n", ret);
			goto err_load_vpu;
		}
	}

	list_add(&ctx->list, &mdp->ctx_list);

	ret = cmdq_pkt_create(&ctx->cmdq_handle);
	if (ret) {
		dev_err(&mdp->pdev->dev, "Create cmdq ptk failed %d\n", ret);
		goto err_create_cmdq_pkt;
	}

	mutex_unlock(&mdp->lock);

	mtk_mdp_dbg(1, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);

	return 0;

err_create_cmdq_pkt:
err_load_vpu:
	mdp->ctx_num--;
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
error_m2m_ctx:
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
error_ctrls:
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&mdp->lock);
err_lock:
	kfree(ctx);

	return ret;
}

static int mtk_mdp_m2m_release(struct file *file)
{
	struct mtk_mdp_ctx *ctx = fh_to_ctx(file->private_data);
	struct mtk_mdp_dev *mdp = ctx->mdp_dev;

	flush_workqueue(mdp->job_wq);
	mutex_lock(&mdp->lock);
	v4l2_m2m_ctx_release(ctx->m2m_ctx);
	v4l2_ctrl_handler_free(&ctx->ctrl_handler);
	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);
	mtk_mdp_vpu_deinit(&ctx->vpu);
	mdp->ctx_num--;
	list_del_init(&ctx->list);
	cmdq_pkt_destroy(ctx->cmdq_handle);

	mtk_mdp_dbg(1, "%s [%d]", dev_name(&mdp->pdev->dev), ctx->id);

	mutex_unlock(&mdp->lock);
	kfree(ctx);

	return 0;
}

static const struct v4l2_file_operations mtk_mdp_m2m_fops = {
	.owner		= THIS_MODULE,
	.open		= mtk_mdp_m2m_open,
	.release	= mtk_mdp_m2m_release,
	.poll		= v4l2_m2m_fop_poll,
	.unlocked_ioctl	= video_ioctl2,
	.mmap		= v4l2_m2m_fop_mmap,
};

static struct v4l2_m2m_ops mtk_mdp_m2m_ops = {
	.device_run	= mtk_mdp_m2m_device_run,
	.job_abort	= mtk_mdp_m2m_job_abort,
};

int mtk_mdp_register_m2m_device(struct mtk_mdp_dev *mdp)
{
	struct device *dev = &mdp->pdev->dev;
	int ret;

	mdp->variant = &mtk_mdp_default_variant;
	mdp->vdev = video_device_alloc();
	if (!mdp->vdev) {
		dev_err(dev, "failed to allocate video device\n");
		ret = -ENOMEM;
		goto err_video_alloc;
	}
	mdp->vdev->device_caps = V4L2_CAP_VIDEO_M2M_MPLANE | V4L2_CAP_STREAMING;
	mdp->vdev->fops = &mtk_mdp_m2m_fops;
	mdp->vdev->ioctl_ops = &mtk_mdp_m2m_ioctl_ops;
	mdp->vdev->release = video_device_release;
	mdp->vdev->lock = &mdp->lock;
	mdp->vdev->vfl_dir = VFL_DIR_M2M;
	mdp->vdev->v4l2_dev = &mdp->v4l2_dev;
	snprintf(mdp->vdev->name, sizeof(mdp->vdev->name), "%s:m2m",
		 MTK_MDP_MODULE_NAME);
	video_set_drvdata(mdp->vdev, mdp);

	mdp->m2m_dev = v4l2_m2m_init(&mtk_mdp_m2m_ops);
	if (IS_ERR(mdp->m2m_dev)) {
		dev_err(dev, "failed to initialize v4l2-m2m device\n");
		ret = PTR_ERR(mdp->m2m_dev);
		goto err_m2m_init;
	}

	ret = video_register_device(mdp->vdev, VFL_TYPE_GRABBER, 2);
	if (ret) {
		dev_err(dev, "failed to register video device\n");
		goto err_vdev_register;
	}

	v4l2_info(&mdp->v4l2_dev, "driver registered as /dev/video%d\n",
		  mdp->vdev->num);
	return 0;

err_vdev_register:
	v4l2_m2m_release(mdp->m2m_dev);
err_m2m_init:
	video_device_release(mdp->vdev);
err_video_alloc:

	return ret;
}

void mtk_mdp_unregister_m2m_device(struct mtk_mdp_dev *mdp)
{
	video_unregister_device(mdp->vdev);
	v4l2_m2m_release(mdp->m2m_dev);
}

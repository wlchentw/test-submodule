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

#include <drm/drmP.h>
#include <drm/drm_crtc.h>
#include <drm/drm_crtc_helper.h>
#include <linux/kernel.h>
#include <linux/component.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>

#include "mtk_dpi_regs.h"
#include <mtk_dpi_panel.h>

enum mtk_dpi_out_bit_num {
	MTK_DPI_OUT_BIT_NUM_8BITS,
	MTK_DPI_OUT_BIT_NUM_10BITS,
	MTK_DPI_OUT_BIT_NUM_12BITS,
	MTK_DPI_OUT_BIT_NUM_16BITS
};

enum mtk_dpi_out_yc_map {
	MTK_DPI_OUT_YC_MAP_RGB,
	MTK_DPI_OUT_YC_MAP_CYCY,
	MTK_DPI_OUT_YC_MAP_YCYC,
	MTK_DPI_OUT_YC_MAP_CY,
	MTK_DPI_OUT_YC_MAP_YC
};

enum mtk_dpi_out_channel_swap {
	MTK_DPI_OUT_CHANNEL_SWAP_RGB,
	MTK_DPI_OUT_CHANNEL_SWAP_GBR,
	MTK_DPI_OUT_CHANNEL_SWAP_BRG,
	MTK_DPI_OUT_CHANNEL_SWAP_RBG,
	MTK_DPI_OUT_CHANNEL_SWAP_GRB,
	MTK_DPI_OUT_CHANNEL_SWAP_BGR
};

enum mtk_dpi_out_color_format {
	MTK_DPI_COLOR_FORMAT_RGB,
	MTK_DPI_COLOR_FORMAT_RGB_FULL,
	MTK_DPI_COLOR_FORMAT_YCBCR_444,
	MTK_DPI_COLOR_FORMAT_YCBCR_422,
	MTK_DPI_COLOR_FORMAT_XV_YCC,
	MTK_DPI_COLOR_FORMAT_YCBCR_444_FULL,
	MTK_DPI_COLOR_FORMAT_YCBCR_422_FULL
};

struct mtk_dpi {
	void __iomem *regs_dpi;
	void __iomem *regs_tcon_top;
	struct device *dev;
	struct device *panel_dev;
	struct clk *engine_clk;
	struct clk *pixel_clk;
	struct clk *tvd_clk;
	int irq;
	struct drm_display_mode mode;
	const struct mtk_dpi_conf *conf;
	enum mtk_dpi_out_color_format color_format;
	enum mtk_dpi_out_yc_map yc_map;
	enum mtk_dpi_out_bit_num bit_num;
	enum mtk_dpi_out_channel_swap channel_swap;
	struct dpi_funcs *funcs;
	struct panel_dpi *panel;
	bool power_sta;
	u8 power_ctl;
};

enum mtk_dpi_polarity {
	MTK_DPI_POLARITY_RISING,
	MTK_DPI_POLARITY_FALLING,
};

enum mtk_dpi_power_ctl {
	DPI_POWER_START = BIT(0),
	DPI_POWER_ENABLE = BIT(1),
};

struct mtk_dpi_polarities {
	enum mtk_dpi_polarity de_pol;
	enum mtk_dpi_polarity ck_pol;
	enum mtk_dpi_polarity hsync_pol;
	enum mtk_dpi_polarity vsync_pol;
};

struct mtk_dpi_sync_param {
	u32 sync_width;
	u32 front_porch;
	u32 back_porch;
	bool shift_half_line;
};

struct mtk_dpi_yc_limit {
	u16 y_top;
	u16 y_bottom;
	u16 c_top;
	u16 c_bottom;
};

struct mtk_dpi_conf {
	unsigned int (*cal_factor)(int clock);
	const u32 reg_h_fre_con;
};

struct dpi_funcs {
	void (*unprepare)(struct mtk_dpi *dpi);
	void (*prepare)(struct mtk_dpi *dpi);

	void (*disable)(struct mtk_dpi *dpi);
	void (*enable)(struct mtk_dpi *dpi);
};

static void mtk_dpi_mask(struct mtk_dpi *dpi, u32 offset, u32 val, u32 mask)
{
	u32 tmp = readl(dpi->regs_dpi + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, dpi->regs_dpi + offset);
}

static void mtk_dpi_tcon_mask(struct mtk_dpi *dpi, u32 offset, u32 val,
			      u32 mask)
{
	u32 tmp = readl(dpi->regs_tcon_top + offset) & ~mask;

	tmp |= (val & mask);
	writel(tmp, dpi->regs_tcon_top + offset);
}

static void mtk_dpi_sw_reset(struct mtk_dpi *dpi, bool reset)
{
	mtk_dpi_mask(dpi, DPI_RET, reset ? RST : 0, RST);
}

static void mtk_dpi_enable(struct mtk_dpi *dpi)
{
	mtk_dpi_mask(dpi, DPI_EN, EN, EN);
}

static void mtk_dpi_disable(struct mtk_dpi *dpi)
{
	mtk_dpi_mask(dpi, DPI_EN, 0, EN);
}

static void mtk_dpi_config_hsync(struct mtk_dpi *dpi,
				 struct mtk_dpi_sync_param *sync)
{
	mtk_dpi_mask(dpi, DPI_TGEN_HWIDTH,
		     sync->sync_width << HPW, HPW_MASK);
	mtk_dpi_mask(dpi, DPI_TGEN_HPORCH,
		     sync->back_porch << HBP, HBP_MASK);
	mtk_dpi_mask(dpi, DPI_TGEN_HPORCH, sync->front_porch << HFP,
		     HFP_MASK);
}

static void mtk_dpi_config_vsync(struct mtk_dpi *dpi,
				 struct mtk_dpi_sync_param *sync,
				 u32 width_addr, u32 porch_addr)
{
	mtk_dpi_mask(dpi, width_addr,
		     sync->sync_width << VSYNC_WIDTH_SHIFT,
		     VSYNC_WIDTH_MASK);
	mtk_dpi_mask(dpi, width_addr,
		     sync->shift_half_line << VSYNC_HALF_LINE_SHIFT,
		     VSYNC_HALF_LINE_MASK);
	mtk_dpi_mask(dpi, porch_addr,
		     sync->back_porch << VSYNC_BACK_PORCH_SHIFT,
		     VSYNC_BACK_PORCH_MASK);
	mtk_dpi_mask(dpi, porch_addr,
		     sync->front_porch << VSYNC_FRONT_PORCH_SHIFT,
		     VSYNC_FRONT_PORCH_MASK);
}

static void mtk_dpi_config_vsync_lodd(struct mtk_dpi *dpi,
				      struct mtk_dpi_sync_param *sync)
{
	mtk_dpi_config_vsync(dpi, sync, DPI_TGEN_VWIDTH, DPI_TGEN_VPORCH);
}

static void mtk_dpi_config_vsync_leven(struct mtk_dpi *dpi,
				       struct mtk_dpi_sync_param *sync)
{
	mtk_dpi_config_vsync(dpi, sync, DPI_TGEN_VWIDTH_LEVEN,
			     DPI_TGEN_VPORCH_LEVEN);
}

static void mtk_dpi_config_vsync_rodd(struct mtk_dpi *dpi,
				      struct mtk_dpi_sync_param *sync)
{
	mtk_dpi_config_vsync(dpi, sync, DPI_TGEN_VWIDTH_RODD,
			     DPI_TGEN_VPORCH_RODD);
}

static void mtk_dpi_config_vsync_reven(struct mtk_dpi *dpi,
				       struct mtk_dpi_sync_param *sync)
{
	mtk_dpi_config_vsync(dpi, sync, DPI_TGEN_VWIDTH_REVEN,
			     DPI_TGEN_VPORCH_REVEN);
}

static void mtk_dpi_config_pol(struct mtk_dpi *dpi,
			       struct mtk_dpi_polarities *dpi_pol)
{
	unsigned int pol;

	pol = (dpi_pol->ck_pol == MTK_DPI_POLARITY_RISING ? 0 : CK_POL) |
	      (dpi_pol->de_pol == MTK_DPI_POLARITY_RISING ? 0 : DE_POL) |
	      (dpi_pol->hsync_pol == MTK_DPI_POLARITY_RISING ? 0 : HSYNC_POL) |
	      (dpi_pol->vsync_pol == MTK_DPI_POLARITY_RISING ? 0 : VSYNC_POL);
	mtk_dpi_mask(dpi, DPI_OUTPUT_SETTING, pol,
		     CK_POL | DE_POL | HSYNC_POL | VSYNC_POL);
}

static void mtk_dpi_config_3d(struct mtk_dpi *dpi, bool en_3d)
{
	mtk_dpi_mask(dpi, DPI_CON, en_3d ? TDFP_EN : 0, TDFP_EN);
}

static void mtk_dpi_config_interface(struct mtk_dpi *dpi, bool inter)
{
	mtk_dpi_mask(dpi, DPI_CON, inter ? INTL_EN : 0, INTL_EN);
}

static void mtk_dpi_config_fb_size(struct mtk_dpi *dpi, u32 width, u32 height)
{
	mtk_dpi_mask(dpi, DPI_SIZE, width << HSIZE, HSIZE_MASK);
	mtk_dpi_mask(dpi, DPI_SIZE, height << VSIZE, VSIZE_MASK);
}

static void mtk_dpi_config_channel_limit(struct mtk_dpi *dpi,
					 struct mtk_dpi_yc_limit *limit)
{
	mtk_dpi_mask(dpi, DPI_Y_LIMIT, limit->y_bottom << Y_LIMINT_BOT,
		     Y_LIMINT_BOT_MASK);
	mtk_dpi_mask(dpi, DPI_Y_LIMIT, limit->y_top << Y_LIMINT_TOP,
		     Y_LIMINT_TOP_MASK);
	mtk_dpi_mask(dpi, DPI_C_LIMIT, limit->c_bottom << C_LIMIT_BOT,
		     C_LIMIT_BOT_MASK);
	mtk_dpi_mask(dpi, DPI_C_LIMIT, limit->c_top << C_LIMIT_TOP,
		     C_LIMIT_TOP_MASK);
}

static void mtk_dpi_config_bit_num(struct mtk_dpi *dpi,
				   enum mtk_dpi_out_bit_num num)
{
	u32 val;

	switch (num) {
	case MTK_DPI_OUT_BIT_NUM_8BITS:
		val = OUT_BIT_8;
		break;
	case MTK_DPI_OUT_BIT_NUM_10BITS:
		val = OUT_BIT_10;
		break;
	case MTK_DPI_OUT_BIT_NUM_12BITS:
		val = OUT_BIT_12;
		break;
	case MTK_DPI_OUT_BIT_NUM_16BITS:
		val = OUT_BIT_16;
		break;
	default:
		val = OUT_BIT_8;
		break;
	}
	mtk_dpi_mask(dpi, DPI_OUTPUT_SETTING, val << OUT_BIT,
		     OUT_BIT_MASK);
}

static void mtk_dpi_config_yc_map(struct mtk_dpi *dpi,
				  enum mtk_dpi_out_yc_map map)
{
	u32 val;

	switch (map) {
	case MTK_DPI_OUT_YC_MAP_RGB:
		val = YC_MAP_RGB;
		break;
	case MTK_DPI_OUT_YC_MAP_CYCY:
		val = YC_MAP_CYCY;
		break;
	case MTK_DPI_OUT_YC_MAP_YCYC:
		val = YC_MAP_YCYC;
		break;
	case MTK_DPI_OUT_YC_MAP_CY:
		val = YC_MAP_CY;
		break;
	case MTK_DPI_OUT_YC_MAP_YC:
		val = YC_MAP_YC;
		break;
	default:
		val = YC_MAP_RGB;
		break;
	}

	mtk_dpi_mask(dpi, DPI_OUTPUT_SETTING, val << YC_MAP, YC_MAP_MASK);
}

static void mtk_dpi_config_channel_swap(struct mtk_dpi *dpi,
					enum mtk_dpi_out_channel_swap swap)
{
	u32 val;

	switch (swap) {
	case MTK_DPI_OUT_CHANNEL_SWAP_RGB:
		val = SWAP_RGB;
		break;
	case MTK_DPI_OUT_CHANNEL_SWAP_GBR:
		val = SWAP_GBR;
		break;
	case MTK_DPI_OUT_CHANNEL_SWAP_BRG:
		val = SWAP_BRG;
		break;
	case MTK_DPI_OUT_CHANNEL_SWAP_RBG:
		val = SWAP_RBG;
		break;
	case MTK_DPI_OUT_CHANNEL_SWAP_GRB:
		val = SWAP_GRB;
		break;
	case MTK_DPI_OUT_CHANNEL_SWAP_BGR:
		val = SWAP_BGR;
		break;
	default:
		val = SWAP_RGB;
		break;
	}

	mtk_dpi_mask(dpi, DPI_OUTPUT_SETTING, val << CH_SWAP, CH_SWAP_MASK);
}

static void mtk_dpi_config_yuv422_enable(struct mtk_dpi *dpi, bool enable)
{
	mtk_dpi_mask(dpi, DPI_CON, enable ? YUV422_EN : 0, YUV422_EN);
}

static void mtk_dpi_config_csc_enable(struct mtk_dpi *dpi, bool enable)
{
	mtk_dpi_mask(dpi, DPI_CON, enable ? CSC_ENABLE : 0, CSC_ENABLE);
}

static void mtk_dpi_config_swap_input(struct mtk_dpi *dpi, bool enable)
{
	mtk_dpi_mask(dpi, DPI_CON, enable ? IN_RB_SWAP : 0, IN_RB_SWAP);
}

static void mtk_dpi_config_2n_h_fre(struct mtk_dpi *dpi)
{
	mtk_dpi_mask(dpi, dpi->conf->reg_h_fre_con, H_FRE_2N, H_FRE_2N);
}

static void mtk_dpi_config_color_format(struct mtk_dpi *dpi,
					enum mtk_dpi_out_color_format format)
{
	if ((format == MTK_DPI_COLOR_FORMAT_YCBCR_444) ||
	    (format == MTK_DPI_COLOR_FORMAT_YCBCR_444_FULL)) {
		mtk_dpi_config_yuv422_enable(dpi, false);
		mtk_dpi_config_csc_enable(dpi, true);
		mtk_dpi_config_swap_input(dpi, false);
		mtk_dpi_config_channel_swap(dpi, MTK_DPI_OUT_CHANNEL_SWAP_BGR);
	} else if ((format == MTK_DPI_COLOR_FORMAT_YCBCR_422) ||
		   (format == MTK_DPI_COLOR_FORMAT_YCBCR_422_FULL)) {
		mtk_dpi_config_yuv422_enable(dpi, true);
		mtk_dpi_config_csc_enable(dpi, true);
		mtk_dpi_config_swap_input(dpi, true);
		mtk_dpi_config_channel_swap(dpi, MTK_DPI_OUT_CHANNEL_SWAP_RGB);
	} else {
		mtk_dpi_config_yuv422_enable(dpi, false);
		mtk_dpi_config_csc_enable(dpi, false);
		mtk_dpi_config_swap_input(dpi, false);
		mtk_dpi_config_channel_swap(dpi, MTK_DPI_OUT_CHANNEL_SWAP_RGB);
	}
}

static void mtk_dpi_power_off(struct mtk_dpi *dpi, enum mtk_dpi_power_ctl pctl)
{
	dpi->power_ctl &= ~pctl;

	if ((dpi->power_ctl & DPI_POWER_START) ||
	    (dpi->power_ctl & DPI_POWER_ENABLE))
		return;

	if (!dpi->power_sta)
		return;

	mtk_dpi_disable(dpi);
	clk_disable_unprepare(dpi->pixel_clk);
	clk_disable_unprepare(dpi->engine_clk);
	dpi->power_sta = false;
}

static int mtk_dpi_power_on(struct mtk_dpi *dpi, enum mtk_dpi_power_ctl pctl)
{
	int ret;

	dpi->power_ctl |= pctl;

	if (!(dpi->power_ctl & DPI_POWER_START) &&
	    !(dpi->power_ctl & DPI_POWER_ENABLE))
		return 0;

	if (dpi->power_sta)
		return 0;

	ret = clk_prepare_enable(dpi->engine_clk);
	if (ret) {
		dev_info(dpi->dev, "Failed to enable engine clock: %d\n", ret);
		goto err_eng;
	}

	ret = clk_prepare_enable(dpi->pixel_clk);
	if (ret) {
		dev_info(dpi->dev, "Failed to enable pixel clock: %d\n", ret);
		goto err_pixel;
	}

	mtk_dpi_enable(dpi);
	dpi->power_sta = true;
	return 0;

err_pixel:
	clk_disable_unprepare(dpi->engine_clk);
err_eng:
	dpi->power_ctl &= ~pctl;
	return ret;
}

static int mtk_dpi_config_dpi_pin_mode(struct mtk_dpi *dpi)
{
	mtk_dpi_tcon_mask(dpi, 0x40, 1 << 6, 1 << 6);

	mtk_dpi_tcon_mask(dpi, 0xa0, 0 << 0, 0x3f << 0);
	mtk_dpi_tcon_mask(dpi, 0xa0, 1 << 6, 0x3f << 6);
	mtk_dpi_tcon_mask(dpi, 0xa0, 2 << 12, 0x3f << 12);
	mtk_dpi_tcon_mask(dpi, 0xa0, 3 << 18, 0x3f << 18);
	mtk_dpi_tcon_mask(dpi, 0xa0, 4 << 24, 0x3f << 24);

	mtk_dpi_tcon_mask(dpi, 0xa4, 5 << 0, 0x3f << 0);
	mtk_dpi_tcon_mask(dpi, 0xa4, 6 << 6, 0x3f << 6);
	mtk_dpi_tcon_mask(dpi, 0xa4, 7 << 12, 0x3f << 12);
	mtk_dpi_tcon_mask(dpi, 0xa4, 8 << 18, 0x3f << 18);
	mtk_dpi_tcon_mask(dpi, 0xa4, 9 << 24, 0x3f << 24);

	mtk_dpi_tcon_mask(dpi, 0xa8, 10 << 0, 0x3f << 0);
	mtk_dpi_tcon_mask(dpi, 0xa8, 11 << 6, 0x3f << 6);
	mtk_dpi_tcon_mask(dpi, 0xa8, 12 << 12, 0x3f << 12);
	mtk_dpi_tcon_mask(dpi, 0xa8, 13 << 18, 0x3f << 18);
	mtk_dpi_tcon_mask(dpi, 0xa8, 14 << 24, 0x3f << 24);

	mtk_dpi_tcon_mask(dpi, 0xac, 15 << 0, 0x3f << 0);
	mtk_dpi_tcon_mask(dpi, 0xac, 16 << 6, 0x3f << 6);
	mtk_dpi_tcon_mask(dpi, 0xac, 17 << 12, 0x3f << 12);
	mtk_dpi_tcon_mask(dpi, 0xac, 18 << 18, 0x3f << 18);
	mtk_dpi_tcon_mask(dpi, 0xac, 19 << 24, 0x3f << 24);

	mtk_dpi_tcon_mask(dpi, 0xb0, 20 << 0, 0x3f << 0);
	mtk_dpi_tcon_mask(dpi, 0xb0, 21 << 6, 0x3f << 6);
	mtk_dpi_tcon_mask(dpi, 0xb0, 22 << 12, 0x3f << 12);
	mtk_dpi_tcon_mask(dpi, 0xb0, 23 << 18, 0x3f << 18);
	mtk_dpi_tcon_mask(dpi, 0xb0, 24 << 24, 0x3f << 24);
}

static int mtk_dpi_set_display_mode(struct mtk_dpi *dpi)
{
	struct mtk_dpi_yc_limit limit;
	struct mtk_dpi_polarities dpi_pol;
	struct mtk_dpi_sync_param hsync;
	struct mtk_dpi_sync_param vsync_lodd = { 0 };
	struct mtk_dpi_sync_param vsync_leven = { 0 };
	struct mtk_dpi_sync_param vsync_rodd = { 0 };
	struct mtk_dpi_sync_param vsync_reven = { 0 };
	unsigned long pix_rate;
	unsigned long pll_rate;
	unsigned int factor;
	struct panel_dpi *panel = dev_get_drvdata(dpi->panel_dev);

	if (!dpi) {
		dev_info(dpi->dev, "invalid argument\n");
		return -EINVAL;
	}

	pix_rate = panel->video_mode.pixelclock;
	factor = dpi->conf->cal_factor(panel->video_mode.pixelclock);
	pll_rate = pix_rate * factor;

	dev_dbg(dpi->dev, "Want PLL %lu Hz, pixel clock %lu Hz\n",
		pll_rate, pix_rate);

	clk_set_rate(dpi->tvd_clk, pll_rate);
	pll_rate = clk_get_rate(dpi->tvd_clk);

	pix_rate = pll_rate / factor;
	clk_set_rate(dpi->pixel_clk, pix_rate);
	pix_rate = clk_get_rate(dpi->pixel_clk);

	dev_info(dpi->dev, "Got  PLL %lu Hz, pixel clock %lu Hz\n",
		 pll_rate, pix_rate);

	limit.c_bottom = 0x0000;
	limit.c_top = 0x0fff;
	limit.y_bottom = 0x0000;
	limit.y_top = 0x0fff;

	dpi_pol.ck_pol = MTK_DPI_POLARITY_RISING;
	dpi_pol.de_pol = MTK_DPI_POLARITY_RISING;
	dpi_pol.hsync_pol = panel->video_mode.flags & DRM_MODE_FLAG_PHSYNC ?
			    MTK_DPI_POLARITY_FALLING : MTK_DPI_POLARITY_RISING;
	dpi_pol.vsync_pol = panel->video_mode.flags & DRM_MODE_FLAG_PVSYNC ?
			    MTK_DPI_POLARITY_FALLING : MTK_DPI_POLARITY_RISING;

	hsync.sync_width = panel->video_mode.hsync_len;
	hsync.back_porch = panel->video_mode.hback_porch;
	hsync.front_porch = panel->video_mode.hfront_porch;
	hsync.shift_half_line = false;

	vsync_lodd.sync_width = panel->video_mode.vsync_len;
	vsync_lodd.back_porch = panel->video_mode.vback_porch;
	vsync_lodd.front_porch = panel->video_mode.vfront_porch;
	vsync_lodd.shift_half_line = false;

	if (panel->video_mode.flags & DRM_MODE_FLAG_INTERLACE &&
	    panel->video_mode.flags & DRM_MODE_FLAG_3D_MASK) {
		vsync_leven = vsync_lodd;
		vsync_rodd = vsync_lodd;
		vsync_reven = vsync_lodd;
		vsync_leven.shift_half_line = true;
		vsync_reven.shift_half_line = true;
	} else if (panel->video_mode.flags & DRM_MODE_FLAG_INTERLACE &&
		   !(panel->video_mode.flags & DRM_MODE_FLAG_3D_MASK)) {
		vsync_leven = vsync_lodd;
		vsync_leven.shift_half_line = true;
	} else if (!(panel->video_mode.flags & DRM_MODE_FLAG_INTERLACE) &&
		   panel->video_mode.flags & DRM_MODE_FLAG_3D_MASK) {
		vsync_rodd = vsync_lodd;
	}
	mtk_dpi_sw_reset(dpi, true);
	mtk_dpi_config_pol(dpi, &dpi_pol);

	mtk_dpi_config_hsync(dpi, &hsync);
	mtk_dpi_config_vsync_lodd(dpi, &vsync_lodd);
	mtk_dpi_config_vsync_rodd(dpi, &vsync_rodd);
	mtk_dpi_config_vsync_leven(dpi, &vsync_leven);
	mtk_dpi_config_vsync_reven(dpi, &vsync_reven);

	mtk_dpi_config_3d(dpi,
			  !!(panel->video_mode.flags & DRM_MODE_FLAG_3D_MASK));
	mtk_dpi_config_interface(dpi, !!(panel->video_mode.flags &
					 DRM_MODE_FLAG_INTERLACE));
	if (panel->video_mode.flags & DRM_MODE_FLAG_INTERLACE)
		mtk_dpi_config_fb_size(dpi, panel->video_mode.hactive,
				       panel->video_mode.vactive / 2);
	else
		mtk_dpi_config_fb_size(dpi, panel->video_mode.hactive,
				       panel->video_mode.vactive);

	mtk_dpi_config_channel_limit(dpi, &limit);
	mtk_dpi_config_bit_num(dpi, dpi->bit_num);
	mtk_dpi_config_channel_swap(dpi, dpi->channel_swap);
	mtk_dpi_config_yc_map(dpi, dpi->yc_map);
	mtk_dpi_config_color_format(dpi, dpi->color_format);
	mtk_dpi_sw_reset(dpi, false);

	return 0;
}

static void mtk_dpi_encoder_disable(struct mtk_dpi *dpi)
{
	int ret;

	panel_dpi_disable(panel_dev);

	ret = pm_runtime_put_sync(dpi->dev);
	if (ret < 0)
		dev_info(dpi->dev, "Failed to disable power domain: %d\n", ret);

	mtk_dpi_power_off(dpi, DPI_POWER_ENABLE);

	panel_dpi_unprepare(panel_dev);
}

static void mtk_dpi_encoder_enable(struct mtk_dpi *dpi)
{
	int ret;

	ret = pm_runtime_get_sync(dpi->dev);
	if (ret < 0)
		dev_info(dpi->dev, "Failed to enable power domain: %d\n", ret);

	panel_dpi_prepare(panel_dev);

	mtk_dpi_power_on(dpi, DPI_POWER_ENABLE);
	mtk_dpi_set_display_mode(dpi);
	mtk_dpi_config_dpi_pin_mode(dpi);

	panel_dpi_enable(panel_dev);
}

static struct dpi_funcs mtk_dpi_funcs = {
	.disable = mtk_dpi_encoder_disable,
	.enable = mtk_dpi_encoder_enable,
};

static unsigned int mt8512_calculate_factor(int clock)
{
	return 1;
}

static const struct mtk_dpi_conf mt8512_conf = {
	.cal_factor = mt8512_calculate_factor,
	.reg_h_fre_con = 0xe0,
};

static const struct of_device_id mtk_dpi_of_ids[] = {
	{ .compatible = "mediatek,mt8512-dpi",
	  .data = &mt8512_conf,
	}
};
MODULE_DEVICE_TABLE(of, mtk_dpi_of_ids);

static int mtk_dpi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_dpi *dpi;
	struct resource *mem;
	struct device_node *ep, *panel_node = NULL;
	struct platform_device *panel_pdev;
	int comp_id;
	const struct of_device_id *match;
	int ret;

	match = of_match_node(mtk_dpi_of_ids, dev->of_node);
	if (!match)
		return -ENODEV;

	dpi = devm_kzalloc(dev, sizeof(*dpi), GFP_KERNEL);
	if (!dpi)
		return -ENOMEM;

	dpi->dev = dev;
	dpi->conf = (struct mtk_dpi_conf *)match->data;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dpi->regs_dpi = devm_ioremap_resource(dev, mem);
	if (IS_ERR(dpi->regs_dpi)) {
		ret = PTR_ERR(dpi->regs_dpi);
		dev_info(dev, "Failed to regs_dpi mem resource: %d\n", ret);
		return ret;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	dpi->regs_tcon_top = devm_ioremap_resource(dev, mem);
	if (IS_ERR(dpi->regs_tcon_top)) {
		ret = PTR_ERR(dpi->regs_tcon_top);
		dev_info(dev,
			"Failed to regs_tcon_top mem resource: %d\n", ret);
		return ret;
	}

	dpi->engine_clk = devm_clk_get(dev, "engine");
	if (IS_ERR(dpi->engine_clk)) {
		ret = PTR_ERR(dpi->engine_clk);
		dev_info(dev, "Failed to get engine clock: %d\n", ret);
		return ret;
	}

	dpi->pixel_clk = devm_clk_get(dev, "pixel");
	if (IS_ERR(dpi->pixel_clk)) {
		ret = PTR_ERR(dpi->pixel_clk);
		dev_info(dev, "Failed to get pixel clock: %d\n", ret);
		return ret;
	}

	dpi->tvd_clk = devm_clk_get(dev, "pll");
	if (IS_ERR(dpi->tvd_clk)) {
		ret = PTR_ERR(dpi->tvd_clk);
		dev_info(dev, "Failed to get tvdpll clock: %d\n", ret);
		return ret;
	}

	ep = of_graph_get_next_endpoint(dev->of_node, NULL);
	if (ep) {
		panel_node = of_graph_get_remote_port_parent(ep);
		of_node_put(ep);
	}
	if (!panel_node) {
		dev_info(dev, "Failed to find bridge node\n");
		return -ENODEV;
	}

	panel_pdev = of_find_device_by_node(panel_node);

	if (!panel_pdev) {
		dev_info(dev, "Failed to find panel_pdev\n");
		return -EPROBE_DEFER;
	}

	dpi->funcs = &mtk_dpi_funcs;
	dpi->panel_dev = &panel_pdev->dev;
	dev_info(dev, "Found panel node: %s\n", panel_node->full_name);

	platform_set_drvdata(pdev, dpi);

	pm_runtime_enable(dev);

	mtk_dpi_encoder_enable(dpi);

	return 0;
}

static int mtk_dpi_remove(struct platform_device *pdev)
{
	pm_runtime_disable(&pdev->dev);

	return 0;
}

struct platform_driver mtk_dpi_driver = {
	.probe = mtk_dpi_probe,
	.remove = mtk_dpi_remove,
	.driver = {
		.name = "mediatek-dpi",
		.of_match_table = mtk_dpi_of_ids,
	},
};

static int __init mtk_dpi_init(void)
{
	int err;

	err = platform_driver_register(&mtk_dpi_driver);
	if (err < 0)
		return err;

	return 0;
}

late_initcall(mtk_dpi_init);


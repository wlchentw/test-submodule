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

#include <linux/backlight.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <video/display_timing.h>
#include <video/of_display_timing.h>
#include <video/videomode.h>

#include <drm/drmP.h>
#include <mtk_dpi_panel.h>

struct dpi_panel_funcs {
	int (*disable)(struct panel_dpi *panel);
	int (*unprepare)(struct panel_dpi *panel);
	int (*prepare)(struct panel_dpi *panel);
	int (*enable)(struct panel_dpi *panel);
};

int panel_dpi_disable(struct panel_dpi *panel)
{
	if (panel->backlight) {
		panel->backlight->props.power = FB_BLANK_POWERDOWN;
		panel->backlight->props.state |= BL_CORE_FBBLANK;
		backlight_update_status(panel->backlight);
	}

	return 0;
}

int panel_dpi_unprepare(struct panel_dpi *panel)
{
	if (panel->enable_gpio)
		gpiod_set_value_cansleep(panel->enable_gpio, 0);

	if (panel->supply)
		regulator_disable(panel->supply);

	return 0;
}

int panel_dpi_prepare(struct panel_dpi *panel)
{
	if (panel->supply) {
		int err;

		err = regulator_enable(panel->supply);
		if (err < 0) {
			dev_info(panel->dev, "failed to enable supply: %d\n",
				err);
			return err;
		}
	}

	if (panel->enable_gpio)
		gpiod_set_value_cansleep(panel->enable_gpio, 1);

	return 0;
}

int panel_dpi_enable(struct panel_dpi *panel)
{
	if (panel->backlight) {
		panel->backlight->props.state &= ~BL_CORE_FBBLANK;
		panel->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(panel->backlight);
	}

	return 0;
}

static struct dpi_panel_funcs panel_dpi_funcs = {
	.disable = panel_dpi_disable,
	.unprepare = panel_dpi_unprepare,
	.prepare = panel_dpi_prepare,
	.enable = panel_dpi_enable,
};

static int parse_timing_property(const struct device_node *np, const char *name,
				 struct timing_entry *result)
{
	struct property *prop;
	int length, cells, ret;

	prop = of_find_property(np, name, &length);
	if (!prop) {
		pr_info("%s: could not find property %s\n",
		       of_node_full_name(np), name);
		return -EINVAL;
	}

	cells = length / sizeof(u32);
	if (cells == 1) {
		ret = of_property_read_u32(np, name, &result->typ);
		result->min = result->typ;
		result->max = result->typ;
	} else if (cells == 3) {
		ret = of_property_read_u32_array(np, name, &result->min, cells);
	} else {
		pr_info("%s: illegal timing specification in %s\n",
		       of_node_full_name(np), name);
		return -EINVAL;
	}

	return ret;
}

static int of_parse_display_timing(const struct device_node *np,
				   struct display_timing *dt)
{
	u32 val = 0;
	int ret = 0;

	memset(dt, 0, sizeof(*dt));

	ret |= parse_timing_property(np, "hback-porch", &dt->hback_porch);
	ret |= parse_timing_property(np, "hfront-porch", &dt->hfront_porch);
	ret |= parse_timing_property(np, "hactive", &dt->hactive);
	ret |= parse_timing_property(np, "hsync-len", &dt->hsync_len);
	ret |= parse_timing_property(np, "vback-porch", &dt->vback_porch);
	ret |= parse_timing_property(np, "vfront-porch", &dt->vfront_porch);
	ret |= parse_timing_property(np, "vactive", &dt->vactive);
	ret |= parse_timing_property(np, "vsync-len", &dt->vsync_len);
	ret |= parse_timing_property(np, "clock-frequency", &dt->pixelclock);

	dt->flags = 0;
	if (!of_property_read_u32(np, "vsync-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_VSYNC_HIGH :
				DISPLAY_FLAGS_VSYNC_LOW;
	if (!of_property_read_u32(np, "hsync-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_HSYNC_HIGH :
				DISPLAY_FLAGS_HSYNC_LOW;
	if (!of_property_read_u32(np, "de-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_DE_HIGH :
				DISPLAY_FLAGS_DE_LOW;
	if (!of_property_read_u32(np, "pixelclk-active", &val))
		dt->flags |= val ? DISPLAY_FLAGS_PIXDATA_POSEDGE :
				DISPLAY_FLAGS_PIXDATA_NEGEDGE;

	if (of_property_read_bool(np, "interlaced"))
		dt->flags |= DISPLAY_FLAGS_INTERLACED;
	if (of_property_read_bool(np, "doublescan"))
		dt->flags |= DISPLAY_FLAGS_DOUBLESCAN;
	if (of_property_read_bool(np, "doubleclk"))
		dt->flags |= DISPLAY_FLAGS_DOUBLECLK;

	if (ret) {
		pr_info("%s: error reading timing properties\n",
		       of_node_full_name(np));
		return -EINVAL;
	}

	return 0;
}

int of_get_display_timing(struct device_node *np, const char *name,
			  struct display_timing *dt)
{
	struct device_node *timing_np;

	if (!np)
		return -EINVAL;

	timing_np = of_get_child_by_name(np, name);
	if (!timing_np) {
		pr_info("%s: could not find node '%s'\n",
		       of_node_full_name(np), name);
		return -ENOENT;
	}

	return of_parse_display_timing(timing_np, dt);
}

void videomode_from_timing(const struct display_timing *dt,
			   struct videomode *vm)
{
	vm->pixelclock = dt->pixelclock.typ;
	vm->hactive = dt->hactive.typ;
	vm->hfront_porch = dt->hfront_porch.typ;
	vm->hback_porch = dt->hback_porch.typ;
	vm->hsync_len = dt->hsync_len.typ;

	vm->vactive = dt->vactive.typ;
	vm->vfront_porch = dt->vfront_porch.typ;
	vm->vback_porch = dt->vback_porch.typ;
	vm->vsync_len = dt->vsync_len.typ;

	vm->flags = dt->flags;
}

static int panel_dpi_parse_dt(struct panel_dpi *panel)
{
	struct device_node *np = panel->dev->of_node;
	struct display_timing timing;
	int ret;

	ret = of_get_display_timing(np, "panel-timing", &timing);
	if (ret < 0)
		return ret;

	videomode_from_timing(&timing, &panel->video_mode);

	ret = of_property_read_u32(np, "width-mm", &panel->width_mm);
	if (ret < 0)
		dev_dbg(panel->dev, "invalid or missing %s DT property ",
			"width-mm\n");

	ret = of_property_read_u32(np, "height-mm", &panel->height_mm);
	if (ret < 0)
		dev_dbg(panel->dev, "invalid or missing %s DT property ",
			"height-mm\n");

	return 0;
}

struct panel_dpi *panel_dev;

static int panel_dpi_probe(struct platform_device *pdev)
{
	struct panel_dpi *panel;
	struct device_node *np;
	int ret;

	panel = devm_kzalloc(&pdev->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	panel->dev = &pdev->dev;

	ret = panel_dpi_parse_dt(panel);
	if (ret < 0)
		return ret;

	panel->supply = devm_regulator_get_optional(panel->dev, "power");
	if (IS_ERR(panel->supply)) {
		ret = PTR_ERR(panel->supply);

		if (ret != -ENODEV) {
			if (ret != -EPROBE_DEFER)
				dev_info(panel->dev, "failed to request regulator: %d\n",
					ret);
			return ret;
		}

		panel->supply = NULL;
	}

	/* Get GPIOs and backlight controller. */
	panel->enable_gpio = devm_gpiod_get_optional(panel->dev, "enable",
						     GPIOD_OUT_LOW);
	if (IS_ERR(panel->enable_gpio)) {
		ret = PTR_ERR(panel->enable_gpio);
		dev_info(panel->dev, "failed to request %s GPIO: %d\n",
			"enable", ret);
		return ret;
	}

	panel->reset_gpio = devm_gpiod_get_optional(panel->dev, "reset",
						     GPIOD_OUT_HIGH);
	if (IS_ERR(panel->reset_gpio)) {
		ret = PTR_ERR(panel->reset_gpio);
		dev_info(panel->dev, "failed to request %s GPIO: %d\n",
			"reset", ret);
		return ret;
	}

	np = of_parse_phandle(panel->dev->of_node, "backlight", 0);
	if (np) {
		panel->backlight = of_find_backlight_by_node(np);
		of_node_put(np);

		if (!panel->backlight)
			return -EPROBE_DEFER;
	}

	panel->funcs = &panel_dpi_funcs;
	dev_info(panel->dev, "panel dpi probe done\n");

	panel_dev = panel;

	dev_set_drvdata(panel->dev, panel);
	return 0;

error:
	put_device(&panel->backlight->dev);
	return ret;
}

static int panel_dpi_remove(struct platform_device *pdev)
{
	struct panel_dpi *panel = dev_get_drvdata(&pdev->dev);

	panel_dpi_disable(panel);

	if (panel->backlight)
		put_device(&panel->backlight->dev);

	return 0;
}

static const struct of_device_id panel_dpi_of_table[] = {
	{ .compatible = "panel-dpi", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, panel_dpi_of_table);

static struct platform_driver panel_dpi_driver = {
	.probe		= panel_dpi_probe,
	.remove		= panel_dpi_remove,
	.driver		= {
		.name	= "panel-dpi",
		.of_match_table = panel_dpi_of_table,
	},
};

static int __init panel_dpi_init(void)
{
	int err;

	err = platform_driver_register(&panel_dpi_driver);
	if (err < 0)
		return err;

	return 0;
}

module_init(panel_dpi_init);

MODULE_AUTHOR("Jitao shi <jitao.shi@mediatek.com>");
MODULE_DESCRIPTION("DPI Panel Driver");
MODULE_LICENSE("GPL");


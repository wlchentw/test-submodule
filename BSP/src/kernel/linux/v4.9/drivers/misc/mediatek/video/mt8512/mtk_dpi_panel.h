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

struct panel_dpi {
	struct device *dev;
	const char *label;
	unsigned int width_mm;
	unsigned int height_mm;
	struct videomode video_mode;
	unsigned int bus_format;
	bool data_mirror;

	struct backlight_device *backlight;
	struct regulator *supply;
	struct dpi_panel_funcs *funcs;

	struct gpio_desc *enable_gpio;
	struct gpio_desc *reset_gpio;
};

extern struct panel_dpi *panel_dev;

int panel_dpi_prepare(struct panel_dpi *panel);
int panel_dpi_enable(struct panel_dpi *panel);
int panel_dpi_disable(struct panel_dpi *panel);
int panel_dpi_unprepare(struct panel_dpi *panel);


/*
 * mt8518-afe-controls.h  --  Mediatek 8518 audio controls
 *
 * Copyright (c) 2018 MediaTek Inc.
 * Author: Hidalgo Huang <hidalgo.huang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MT8518_AFE_CONTROLS_H_
#define _MT8518_AFE_CONTROLS_H_

struct snd_soc_platform;

int mt8518_afe_add_controls(struct snd_soc_platform *platform);

#endif

/*
 * mt8518-afe-debug.h  --  Mediatek 8518 audio debugfs
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

#ifndef __MT8518_AFE_DEBUG_H__
#define __MT8518_AFE_DEBUG_H__

struct mtk_base_afe;

void mt8518_afe_init_debugfs(struct mtk_base_afe *afe);

void mt8518_afe_cleanup_debugfs(struct mtk_base_afe *afe);

#endif

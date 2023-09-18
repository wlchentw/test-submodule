/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Accelerometer Sensor Driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *****************************************************************************/

#ifndef __HWTCON_RECT_H__
#define __HWTCON_RECT_H__
#include <linux/types.h>

#define MAX_RECT_SETS 20
struct rect {
	int x;
	int y;
	int width;
	int height;
};

struct rects {
	int count;
	struct rect regions[MAX_RECT_SETS];
};

enum RECT_RELATION_ENUM {
	RECT_RELATION_NON_INTERSECT = 0,
	RECT_RELATION_INTERSECT = 1,
	RECT_RELATION_CONTAIN = 2,
};

enum RECT_RELATION_ENUM hwtcon_rect_check_relationship(const struct rect *rect1,
	const struct rect *rect2,
	struct rect *merge_region);

/* check if two rects have collision. */
bool hwtcon_rect_have_collision(const struct rect *rect0,
	const struct rect *rect1);

/* calculate rect instersection */
bool hwtcon_rect_calc_intersection(const struct rect *rect0,
	const struct rect *rect1,
	struct rect *collision_rect);

struct rects hwtcon_rect_minus_00(const struct rect *rect1,
	const struct rect *minus_rect);
struct rects hwtcon_rect_minus_10(const struct rects *rect_set,
	const struct rect *minus_rect);
struct rects hwtcon_rect_minus_01(const struct rect *region,
	const struct rects *minus_rect_set);

/* check rect1 & rect2 are same. */
bool hwtcon_rect_compare(const struct rect *rect1,
	const struct rect *rect2);

/* check if big rect contains small rect. */
bool hwtcon_rect_contain_00(const struct rect *big_rect,
	const struct rect *small_rect);
bool hwtcon_rect_contain_10(const struct rects *rects,
	const struct rect *small_rect);
bool hwtcon_rect_contain_01(const struct rect *big_rect,
	const struct rects *rects);

struct rects hwtcon_rect_unit_00(const struct rect *rect1,
	const struct rect *rect2);
struct rects hwtcon_rect_unit_01(const struct rect *rect1,
	const struct rects *rects);
struct rects hwtcon_rect_unit_11(const struct rects *rects1,
	const struct rects *rects2);

#endif

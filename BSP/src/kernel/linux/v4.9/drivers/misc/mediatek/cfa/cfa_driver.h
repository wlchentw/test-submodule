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

#ifndef __CFA_DRIVER_H__
#define __CFA_DRIVER_H__
#include <linux/types.h>

void cfa_color_handle(unsigned char *color_buffer,unsigned char *gray_buffer, 
		unsigned int width,	unsigned int height,
		unsigned int enhance, unsigned int thread_id,
		unsigned int start_x,unsigned int start_y);

char * cfa_get_mode_name_by_id(int cfa_mode_id);
int cfa_get_id_by_mode_name(const char *cfa_mode_name);

char * cfa_get_mode_name(int iIsFirst,int *O_piModeId);

int cfa_load_einklut(char *pszCFA_LUT_filename);

#endif /* __CFA_DRIVER_H__ */


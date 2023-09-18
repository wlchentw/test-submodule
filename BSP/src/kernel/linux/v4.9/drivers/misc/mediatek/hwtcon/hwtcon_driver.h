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

#ifndef __HWTCON_DRIVER_H__
#define __HWTCON_DRIVER_H__
#include <linux/types.h>

#include "hwtcon_core.h"

extern struct semaphore color_sem1, color_sem2;

enum HW_VERSION_ENUM {
	HW_VERSION_MT8110_1 = 0xCA00,
	HW_VERSION_MT8110_2 = 0xCA01,
	HW_VERSION_MT8113 = 0xCA02,
};

enum MMSYS_CLOCK_RATE_ENUM {
	CLOCK_312M = 312000000,
	CLOCK_416M = 416000000,
};

enum VCORE_VOLTAGE_ENUM {
	VCORE_0P8 = 800000,
	VCORE_0P7 = 700000,
	VCORE_0P65 = 650000,
};

enum VCORE_LEVEL_ENUM {
	VCORE_LEVEL_0P8 = 0,   /* VCORE:0.8V */
	VCORE_LEVEL_0P7_1 = 1, /* VCORE:0.7V */
	VCORE_LEVEL_0P7_2 = 2, /* VCORE:0.7V */
	VCORE_LEVEL_0P65 = 3,  /* VCORE:0.65V */
};

struct hwtcon_clock_info {
	struct clk *pipeline0;
	struct clk *pipeline1;
	struct clk *pipeline2;
	struct clk *pipeline3;
	struct clk *pipeline4;
	struct clk *pipeline5;
	struct clk *pipeline7;
	struct clk *dpi_tmp0;
	struct clk *dpi_tmp1;
	struct clk *mm_sel;
	struct clk *pll_d3;
	struct clk *pll_d2;
};



struct hwtcon_device_info_struct {
	struct device *dev;
	struct device *larb_dev;

	/* mmsys reg */
	u32 mmsys_reg_pa;
	char *mmsys_reg_va;

	/* imgsys reg */
	u32 imgsys_reg_pa;
	char *imgsys_reg_va;

	/* reserved buffer in dts */
	bool reserved_buf_ready;
	struct sg_table reserved_sgt;
	char *reserved_buf_va;
	u32 reserved_buf_mva;
	u32 reserved_buf_pa;
	u32 reserved_buf_size;

	/* HWTCON irq */
	u32 irq_id[IRQ_HWTCON_MAX];

	u32 mmsys_clk_rate;
	u32 vcore_voltage;

	u32 color_format;/* 1: Y8 2:RGB565 3: RGB888 4: RGBA8888*/

	#ifndef FPGA_EARLY_PORTING
	struct hwtcon_clock_info clock_info;
	#endif
};
struct hwtcon_device_info_struct *hwtcon_device_info(void);

char *hwtcon_driver_get_wf_file_path(void);
char *hwtcon_driver_get_cfa_file_path(void);

/* control pipeline & dpi clock */
int hwtcon_driver_enable_clock(bool enable);
int hwtcon_driver_enable_smi_clk(bool enable);
int hwtcon_driver_prepare_clk(void);
int hwtcon_driver_unprepare_clk(void);
int hwtcon_driver_enable_mmsys_power(bool enable);
int hwtcon_driver_enable_pipeline_clk(bool enable);
int hwtcon_driver_enable_dpi_clk(bool enable);
void hwtcon_driver_force_enable_mmsys_domain(bool enable);
u32 hwtcon_driver_get_epd_index(void);
char *hwtcon_driver_get_wf_file_path(void);
void hwtcon_driver_set_wf_file_path(char *file_path);
bool hwtcon_driver_pipeline_clk_is_enable(void);
unsigned long hwtcon_driver_get_dpi_clock_rate(void);
unsigned long hwtcon_driver_get_mmsys_clock_rate(void);
void hwtcon_driver_handle_released_lut_flag(u64 released_lut);
void hwtcon_driver_cmdq_lut_release_start_loop(void);
void hwtcon_driver_cmdq_lut_release_stop_loop(void);

#endif /* __HWTCON_DRIVER_H__ */

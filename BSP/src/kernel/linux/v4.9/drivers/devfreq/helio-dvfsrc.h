/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __HELIO_DVFSRC_H
#define __HELIO_DVFSRC_H

#include <linux/delay.h>
#include <linux/devfreq.h>
#include <linux/io.h>

#if defined(CONFIG_MACH_MT6765)
#include <helio-dvfsrc-mt6765.h>
#elif defined(CONFIG_MACH_MT8518)
#include <helio-dvfsrc-mt8518.h>
#elif defined(CONFIG_MACH_MT6761)
#include <helio-dvfsrc-mt6761.h>
#elif defined(CONFIG_MACH_MT8512)
#include <helio-dvfsrc-mt8512.h>
#else
#include <helio-dvfsrc-mt67xx.h>
#endif

#include "helio-dvfsrc-opp.h"

struct reg_config {
	u32 offset;
	u32 val;
};

struct helio_dvfsrc {
	struct devfreq		*devfreq;
	int irq;
	struct device *dev;
	bool qos_enabled;
	bool dvfsrc_enabled;
	int dvfsrc_flag;

	void __iomem		*regs;
	void __iomem		*sram_regs;
	struct regmap		*spm_map;

	struct clk *clk_dvfsrc;
	struct clk *clk_i2csel;
	struct clk *clk_i2c0;
	struct clk *clk_i2c1;
	struct clk *clk_i2c2;
	struct clk *clk_pwmsel;
	struct clk *clk_infrapwm;
	struct clk *clk_pwm5;

	u32 pwr_index;

	struct notifier_block	pm_qos_memory_bw_nb;
	struct notifier_block	pm_qos_cpu_memory_bw_nb;
	struct notifier_block	pm_qos_gpu_memory_bw_nb;
	struct notifier_block	pm_qos_mm_memory_bw_nb;
	struct notifier_block	pm_qos_other_memory_bw_nb;
	struct notifier_block	pm_qos_ddr_opp_nb;
	struct notifier_block	pm_qos_vcore_opp_nb;
	struct notifier_block	pm_qos_scp_vcore_request_nb;
	struct notifier_block	pm_qos_power_model_ddr_request_nb;
	struct notifier_block	pm_qos_power_model_vcore_request_nb;
	struct notifier_block	pm_qos_vcore_dvfs_force_opp_nb;
	struct notifier_block	pm_qos_isp_hrt_bw_nb;
	struct notifier_block	pm_qos_apu_memory_bw_nb;

	struct reg_config	*init_config;

	bool opp_forced;
	char			force_start[20];
	char			force_end[20];
	int (*suspend)(struct helio_dvfsrc *dvfsrc_dev);
	int (*resume)(struct helio_dvfsrc *dvfsrc_dev);
};

#define DVFSRC_TIMEOUT		1000

#define QOS_TOTAL_BW_BUF_SIZE	8

#define QOS_TOTAL_BW_BUF(idx)	(idx * 4)
#define QOS_TOTAL_BW		(QOS_TOTAL_BW_BUF_SIZE * 4)
#define QOS_CPU_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x4)
#define QOS_GPU_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x8)
#define QOS_MM_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0xC)
#define QOS_OTHER_BW		(QOS_TOTAL_BW_BUF_SIZE * 4 + 0x10)

#define QOS_CM_GPU_ONOFF	(0x70)
#define QOS_CM_GPU_OPP		(0x74)
#define QOS_CM_RESERVE_2	(0x78)
#define QOS_CM_RESERVE_3	(0x7C)

#define SPM_FLAG_DIS_VCORE_DVS		(1U << 3)
#define SPM_FLAG_DIS_VCORE_DFS		(1U << 4)
#define SPM_FLAG_RUN_COMMON_SCENARIO	(1U << 10)
#define SPM_FLAG_DRAM_LPDDR_TYPE	(1U << 17)

#define PCM_REG15_DATA			0x13C
#define SPM_SW_RSV_9			0x658
#define SPM_DVFS_EVENT_STA		0x69C

/* PMIC */
#define vcore_pmic_to_uv(pmic)	\
	(((pmic) * VCORE_STEP_UV) + VCORE_BASE_UV)
#define vcore_uv_to_pmic(uv)	/* pmic >= uv */	\
	((((uv) - VCORE_BASE_UV) + (VCORE_STEP_UV - 1)) / VCORE_STEP_UV)

#define dvfsrc_wait_for_completion(condition, timeout)			\
({								\
	int ret = 0;						\
	if (is_dvfsrc_enabled())				\
		ret = 1;					\
	while (!(condition) && ret > 0) {			\
		if (ret++ >= timeout) {				\
			pr_err("timeout %x\n", get_cur_vcore_dvfs_opp());\
			ret = -EBUSY;				\
		}						\
		udelay(1);					\
	}							\
	ret;							\
})

enum {
	QOS_EMI_BW_TOTAL = 0,
	QOS_EMI_BW_TOTAL_W,
	QOS_EMI_BW_CPU,
	QOS_EMI_BW_GPU,
	QOS_EMI_BW_MM,
	QOS_EMI_BW_OTHER,
	QOS_EMI_BW_TOTAL_AVE,
	QOS_EMI_BW_NUM
};

extern int is_qos_enabled(void);
extern int is_dvfsrc_enabled(void);
extern int is_opp_forced(void);
extern int dvfsrc_get_emi_bw(int type);
extern int get_vcore_dvfs_level(void);
extern void mtk_spmfw_init(int dvfsrc_en, int skip_check);
extern struct reg_config *dvfsrc_get_init_conf(void);
extern void helio_dvfsrc_enable(int dvfsrc_en);
extern void helio_dvfsrc_flag_set(int flag);
extern int helio_dvfsrc_flag_get(void);
extern char *dvfsrc_dump_reg(char *ptr);
extern u32 dvfsrc_read(u32 offset);
extern void dvfsrc_write(u32 offset, u32 val);
extern u32 dvfsrc_sram_read(u32 offset);
extern void dvfsrc_sram_write(u32 offset, u32 val);
extern void dvfsrc_opp_table_init(void);
extern void helio_dvfsrc_reg_config(struct reg_config *config);
extern void helio_dvfsrc_sram_reg_init(void);
extern int spm_get_spmfw_idx(void);
extern void dvfsrc_restore(void);

extern int helio_dvfsrc_add_interface(struct device *dev);
extern void helio_dvfsrc_remove_interface(struct device *dev);
extern void dvfsrc_opp_level_mapping(void);
extern void helio_dvfsrc_sspm_ipi_init(int dvfsrc_en);
extern void get_opp_info(char *p);
extern void get_dvfsrc_reg(char *p);
extern void get_dvfsrc_record(char *p);
extern void get_spm_reg(char *p);
extern void spm_dvfs_pwrap_cmd(int pwrap_cmd, int pwrap_vcore);
extern int helio_dvfsrc_platform_init(struct helio_dvfsrc *dvfsrc);
extern u32 spm_get_dvfs_level(void);
extern u32 spm_get_dvfs_final_level(void);
extern u32 spm_get_pcm_reg9_data(void);
extern void dvfsrc_set_power_model_ddr_request(unsigned int level);
/* met profile function */
extern int vcorefs_get_opp_info_num(void);
extern char **vcorefs_get_opp_info_name(void);
extern unsigned int *vcorefs_get_opp_info(void);
extern int vcorefs_get_src_req_num(void);
extern char **vcorefs_get_src_req_name(void);
extern unsigned int *vcorefs_get_src_req(void);
extern u32 vcorefs_get_md_scenario(void);

extern int dpidle_i2c0_clk_req(bool req);

#endif /* __HELIO_DVFSRC_H */


/*
 * Copyright (C) 2017 MediaTek Inc.
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

unsigned int reg_dump_addr_off[] = {
	0x0000,
	0x0004,
	0x0008,
	0x000C,
	0x0010,
	0x0014,
	0x0018,
	0x001c,
	0x0024,
	0x0028,
	0x002c,
	0x0030,
	0x0034,
	0x0038,
	0x003c,
	0x0040,
	0x0044,
	0x0048,
	0x004c,
	0x0050,
	0x0054,
	0x0058,
	0x005c,
	0x0060,
	0x0064,
	0x0068,
	0x006c,
	0x0070,
	0x0074,
	0x0078,
	0x007c,
	0x0080,
	0x0084,
	0x0088,
	0x008c,
	0x0090,
	0x0094,
	0x0098,
	0x009C,
	0x00A0,
	0x00A4,
	0x00A8,
	0x00B0,
	0x00B4,
	0x00B8,
	0x00BC,
	0x00C0,
	0x00C4,
	0x00C8,
	0x00CC,
	0x00F0,
	0x00F4,
	0x00F8,
	0x00FC,
	0x0C00,
	0x0C04,
	0x0C08,
	0x0C0C,
	0x0C10,
	0x0C14,
	0x0C18,
	0x0C1C,
	0x0C20,
	0x0C24,
	0x0C28,
	0x0C2C,
	0x0C30,
	0x0C34,
	0x0C38,
	0x0C3C,
	0x0C40,
	0x0C44,
	0x0C48,
	0x0C4C,
	0x0C50,
	0x0C54,
	0x0C58,
	0x0C5C,
	0x0C60,
	0x0C64,
	0x0C68,
	0x0C6C,
	0x0C70,
	0x0C74,
	0x0C78,
	0x0C7C,
	0x0C80,
	0x0C84,
	0x0C88,
	0x0CF0,
	0x0CF4,
	0x0CF8,
	0x0CFC,
	0x0F00,
	0x0F04,
	0x0F08,
	0x0F0C,
	0x0F10,
	0x0F14,
	0x0F18,
	0x0F1C,
	0x0F20,
	0x0F24,
	0x0F28,
	0x0F2C,
	0x0F30,
};

/*
 * @file    mt_svs.c
 * @brief   Driver for SVS
 *
 */

#define __MT_SVS_C__
#define pr_fmt(fmt)				"[SVS] "fmt

/*
 * Include files
 */

/* system includes */
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/nvmem-consumer.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <mt-plat/aee.h>
#include <mach/mtk_thermal.h>

#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#endif

/* extra header used */
#include <linux/regulator/consumer.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>

/* local includes */
#include "mtk_svs.h"

#define SVS_COMPATIBLE_NODE	"mediatek,mt8518-svs"

struct svs_det;
struct svs_ctrl;
struct pm_qos_request qos_request = { {0} };

static void svs_set_svs_volt(struct svs_det *det);
static void svs_restore_svs_volt(struct svs_det *det);
static int create_procfs(void);

/*
 * set SVS log enable by procfs interface
 */
static int svs_log_en;

static int is_svs_initializing;

struct clk *main_clk;

#define CONFIG_SVS_SHOWLOG	1

#define SVS_GET_REAL_VAL	(1)	/* get val from efuse */
#define SET_PMIC_VOLT		(1)	/* apply PMIC voltage */

#define DUMP_DATA_TO_DE		(0)

#define LOG_INTERVAL		 (2LL * NSEC_PER_SEC)
#define NR_FREQ			 8

/*
 * 100 us, This is the SVS Detector sampling time
 * as represented in cycles of bclk_ck during INIT. 52 MHz
 */
#define DETWINDOW_VAL		0xa28

/*
 * multiply 10 or divide 10 is for SVS equation. SVS unit is 10uV
 * Regulator API return uV. E.g: regulator_get_voltage(det->reg);
 * unit uv: micro voltage
 */
#define SVS_VOLT_TO_PMIC_VAL(volt)  ((((volt) / 10) - 60000 + 625 - 1) / 625)
#define SVS_PMIC_VAL_TO_VOLT(pmic)  ((((pmic) * 625) + 60000) * 10)

/* offset 0x0(0 steps) for CPU/GPU DVFS */
#define SVS_PMIC_OFFSET (0)

#define DTHI_VAL	0x01
#define DTLO_VAL	0xfe
#define DETMAX_VAL	0xffff
#define AGECONFIG_VAL	0x555555
#define AGEM_VAL	0x0
#define VCO_VAL		0x30
#define DCCONFIG_VAL	0x555555

/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))

#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
/*
 * Genearte a mask wher MSB to LSB are all 0b1
 * @r:	Range in the form of MSB:LSB
 */
#define BITMASK(r)	\
	(((unsigned int) -1 >> (31 - MSB(r))) & ~((1U << LSB(r)) - 1))

/*
 * Set value at MSB:LSB. For example, BITS(7:3, 0x5A)
 * will return a value where bit 3 to bit 7 is 0x5A
 * @r:	Range in the form of MSB:LSB
 */
/* BITS(MSB:LSB, value) => Set value at MSB:LSB  */
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

/*
 * LOG
 */
#define FUNC_LV_MODULE          BIT(0)	/* module, platform driver interface */
#define FUNC_LV_CPUFREQ         BIT(1)	/* cpufreq driver interface          */
#define FUNC_LV_API             BIT(2)	/* mt_cpufreq driver global function */
#define FUNC_LV_LOCAL           BIT(3)	/* mt_cpufreq driver lcaol function  */
#define FUNC_LV_HELP            BIT(4)	/* mt_cpufreq driver help function   */

static unsigned int func_lv_mask;

#if defined(CONFIG_SVS_SHOWLOG)
#define FUNC_ENTER(lv)    do { if ((lv) & func_lv_mask) \
		pr_debug(">> %s()\n", __func__); \
		} while (0)
#define FUNC_EXIT(lv)     do { if ((lv) & func_lv_mask) \
		pr_debug("<< %s():%d\n", __func__, __LINE__); \
		} while (0)
#else
#define FUNC_ENTER(lv)
#define FUNC_EXIT(lv)
#endif				/* CONFIG_CPU_DVFS_SHOWLOG */

/*
 * REG ACCESS
 */

#define svs_read(addr)	__raw_readl(addr)
#define svs_read_field(addr, range)	\
	((svs_read(addr) & BITMASK(range)) >> LSB(range))

#define svs_write(addr, val)	mt_reg_sync_writel(val, addr)
/*
 * Write a field of a register.
 * @addr:	Address of the register
 * @range:	The field bit range in the form of MSB:LSB
 * @val:	The value to be written to the field
 */
#define svs_write_field(addr, range, val)	\
	svs_write(addr, (svs_read(addr) & ~BITMASK(range)) | BITS(range, val))

/*
 * Helper macros
 */

/* SVS detector is disabled by who */
enum {
	BY_PROCFS = BIT(0),
	BY_INIT_ERROR = BIT(1),
	BY_MON_ERROR = BIT(2),
};

#ifdef CONFIG_OF

void __iomem *svs_base;
static u32 svs_irq_number;
#endif

/* Definition for Picachu */
#define PI_SVS_MTDES_START_BIT		(0)
#define PI_SVS_BDES_START_BIT		(8)
#define PI_SVS_MDES_START_BIT		(16)

static unsigned int informSVSisReady;

/*
 * iterate over list of detectors
 * @det:	the detector * to use as a loop cursor.
 */
#define for_each_det(det) \
	for (det = svs_detectors; \
	     det < (svs_detectors + ARRAY_SIZE(svs_detectors)); \
	     det++)

/*
 * iterate over list of detectors and its controller
 * @det:	the detector * to use as a loop cursor.
 * @ctrl:	the svs_ctrl * to use as ctrl pointer of current det.
 */
#define for_each_det_ctrl(det, ctrl)				\
	for (det = svs_detectors,				\
	     ctrl = id_to_svs_ctrl(det->ctrl_id);		\
	     det < (svs_detectors + ARRAY_SIZE(svs_detectors)); \
	     det++,						\
	     ctrl = id_to_svs_ctrl(det->ctrl_id))

/*
 * iterate over list of controllers
 * @pos:	the svs_ctrl * to use as a loop cursor.
 */
#define for_each_ctrl(ctrl) \
	for (ctrl = svs_ctrls; \
	     ctrl < (svs_ctrls + ARRAY_SIZE(svs_ctrls)); \
	     ctrl++)

/*
 * Given a svs_det * in svs_detectors. Return the id.
 * @det:	pointer to a svs_det in svs_detectors
 */
#define det_to_id(det)	((det) - &svs_detectors[0])

/*
 * Given a svs_ctrl * in svs_ctrls. Return the id.
 * @det:	pointer to a svs_ctrl in svs_ctrls
 */
#define ctrl_to_id(ctrl)	((ctrl) - &svs_ctrls[0])

/*
 * Check if a detector has a feature
 * @det:	pointer to a svs_det to be check
 * @feature:	enum svs_features to be checked
 */
#define HAS_FEATURE(det, feature)	((det)->features & feature)

#define PERCENT(numerator, denominator)	\
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

#define PERCENT_U64(numerator, denominator)	\
	(unsigned char)(div_u64(((numerator) * 100 + (denominator) - 1), \
	(denominator)))

#define SVS_PHASE_INIT01 0
#define SVS_PHASE_INIT02 1
#define SVS_PHASE_MON 2
#define NR_SVS_PHASE 3

enum {
	SVS_VOLT_NONE = 0,
	SVS_VOLT_UPDATE = BIT(0),
	SVS_VOLT_RESTORE = BIT(1),
};

struct svs_ctrl {
	const char *name;
	int det_id;
	struct completion init_done;
	/* atomic_t in_init; */
	/* for voltage setting thread */
	wait_queue_head_t wq;
	int volt_update;
	struct task_struct *thread;
};

struct svs_det_ops {
	/* interface to SVS*/
	void (*enable)(struct svs_det *det, int reason);
	void (*disable)(struct svs_det *det, int reason);
	void (*disable_locked)(struct svs_det *det, int reason);
	void (*switch_bank)(struct svs_det *det);

	int (*init01)(struct svs_det *det);
	int (*init02)(struct svs_det *det);
	int (*mon_mode)(struct svs_det *det);

	int (*get_status)(struct svs_det *det);
	void (*dump_status)(struct svs_det *det);

	void (*set_phase)(struct svs_det *det, int phase);

	/* interface to thermal */
	int (*get_temp)(struct svs_det *det);

	/* interface to DVFS */
	int (*get_volt)(struct svs_det *det);
	int (*set_volt)(struct svs_det *det);
	void (*restore_default_volt)(struct svs_det *det);
	void (*get_freq_volt_table)(struct svs_det *det);
};

enum svs_features {
	FEA_INIT01 = BIT(SVS_PHASE_INIT01),
	FEA_INIT02 = BIT(SVS_PHASE_INIT02),
	FEA_MON = BIT(SVS_PHASE_MON),
};

struct svs_det {
	const char *name;
	struct svs_det_ops *ops;
	struct device *dev;
	struct cpufreq_policy svs_cpufreq_policy;
	struct regulator *reg;
	int dev_id;
	int status;
	int features;
	int ctrl_id;

	/* devinfo */
	unsigned int SVSINITEN;
	unsigned int SVSMONEN;
	unsigned int MDES;
	unsigned int BDES;
	unsigned int DCMDET;
	unsigned int DCBDET;
	unsigned int AGEDELTA;
	unsigned int MTDES;

	/* constant */
	unsigned int DETWINDOW;
	unsigned int VMAX;
	unsigned int VMIN;
	unsigned int DTHI;
	unsigned int DTLO;
	unsigned int VBOOT;
	unsigned int DETMAX;
	unsigned int AGECONFIG;
	unsigned int AGEM;
	unsigned int DVTFIXED;
	unsigned int VCO;
	unsigned int DCCONFIG;

	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* for debug */
	unsigned int dcvalues[NR_SVS_PHASE];

	unsigned int svs_freqpct30[NR_SVS_PHASE];
	unsigned int svs_26c[NR_SVS_PHASE];
	unsigned int svs_vop30[NR_SVS_PHASE];
	unsigned int svs_svsen[NR_SVS_PHASE];
	unsigned int reg_dump_data[ARRAY_SIZE(reg_dump_addr_off)][NR_SVS_PHASE];
	/* slope */
	unsigned int MTS;
	unsigned int BTS;

	/* dvfs */
	unsigned int num_freq_tbl;
	unsigned int freq_base;

	/* Use this to limit bank frequency */
	unsigned long dvfs_max_freq_khz;
	unsigned long dvfs_min_freq_khz;
	/* percentage to maximum freq */
	unsigned char freq_table_percent[NR_FREQ];
	u64 freq_table[NR_FREQ];		/* in KHz */
	/* signed-off volt table in uVolt */
	int volt_table[NR_FREQ];

	unsigned int volt_tbl[NR_FREQ];			/* pmic value */
	unsigned int volt_tbl_init2[NR_FREQ];		/* pmic value */
	unsigned int volt_tbl_pmic[NR_FREQ];		/* pmic value */
	int volt_offset;

	unsigned int pi_efuse;

	int disabled;
	unsigned char set_volt_to_upower; /* only when init2, set v to UPT */
};


struct svs_devinfo {
	/* M_HW_RES0 10009264 */
	unsigned int Bodybias:1;
	unsigned int SVS_T:1;
	unsigned int EPS:1;
	unsigned int ANALOG:1;
	unsigned int FT_PGM:4;
	unsigned int reserved1:2;
	unsigned int REV_ECO:2;
	unsigned int PACKAGE:2;
	unsigned int CODE:2;
	unsigned int SAMPLE_ID:16;

	/* M_HW_RES1 10009268 */
	unsigned int CPU_BDES:8;
	unsigned int CPU_MDES:8;
	unsigned int CPU_DCBDET:8;
	unsigned int CPU_DCMDET:8;

	/* M_HW_RES2 1000926c */
	unsigned int CPU_SPEC:3;
	unsigned int Turbo:1;
	unsigned int CPU_DVFS_LOW:2;
	unsigned int SVSINITEN:1;
	unsigned int SVSMONEN:1;
	unsigned int reserved2:8;
	unsigned int CPU_MTDES:8;
	unsigned int CPU_AGEDELTA:8;
};

/*
 *Local variable definition
 */
static int svs_probe(struct platform_device *pdev);
static int svs_suspend(struct platform_device *pdev, pm_message_t state);
static int svs_resume(struct platform_device *pdev);

/*
 * lock
 */
static DEFINE_SPINLOCK(svs_spinlock);

/*
 * SVS controllers
 */
struct svs_ctrl svs_ctrls[NR_SVS_CTRL] = {
	[SVS_CTRL_CPU] = {
		.name = "SVS_CTRL_CPU",
		.det_id = SVS_DET_CPU,
		},
};

/*
 * SVS detectors
 */
static void base_ops_enable(struct svs_det *det, int reason);
static void base_ops_disable(struct svs_det *det, int reason);
static void base_ops_disable_locked(struct svs_det *det, int reason);
static void base_ops_switch_bank(struct svs_det *det);

static int base_ops_init01(struct svs_det *det);
static int base_ops_init02(struct svs_det *det);
static int base_ops_mon_mode(struct svs_det *det);

static int base_ops_get_status(struct svs_det *det);
static void base_ops_dump_status(struct svs_det *det);

static void base_ops_set_phase(struct svs_det *det, int phase);
static int base_ops_get_temp(struct svs_det *det);
static int base_ops_get_volt(struct svs_det *det);
static int base_ops_set_volt(struct svs_det *det);
static void base_ops_restore_default_volt(struct svs_det *det);
static void base_ops_get_freq_volt_table(struct svs_det *det);

static int get_volt_cpu(struct svs_det *det);
static int set_volt_cpu(struct svs_det *det);
static void restore_default_volt_cpu(struct svs_det *det);
static void get_freq_volt_table_cpu(struct svs_det *det);

unsigned int mt_svs_is_enabled(void)
{
	return informSVSisReady;
}

int is_svs_initialized_done(void)
{
	return is_svs_initializing ? -EBUSY : 0;
}

#define BASE_OP(fn)	.fn = base_ops_ ## fn
static struct svs_det_ops svs_det_base_ops = {
	BASE_OP(enable),
	BASE_OP(disable),
	BASE_OP(disable_locked),
	BASE_OP(switch_bank),

	BASE_OP(init01),
	BASE_OP(init02),
	BASE_OP(mon_mode),

	BASE_OP(get_status),
	BASE_OP(dump_status),

	BASE_OP(set_phase),

	BASE_OP(get_temp),

	BASE_OP(get_volt),
	BASE_OP(set_volt),
	BASE_OP(restore_default_volt),
	BASE_OP(get_freq_volt_table),
};

static struct svs_det_ops cpu_det_ops = {
	.get_volt = get_volt_cpu,
	.set_volt = set_volt_cpu,
	.restore_default_volt = restore_default_volt_cpu,
	.get_freq_volt_table = get_freq_volt_table_cpu,
};

static struct svs_det svs_detectors[NR_SVS_DET] = {
	[SVS_DET_CPU] = {
			 .name = "SVS_DET_CPU",
			 .ops = &cpu_det_ops,
			 .ctrl_id = SVS_CTRL_CPU,
			 .features = FEA_INIT01 | FEA_INIT02 | FEA_MON,
			 .freq_base = 1508000000, /* 1508Mhz */
			 .VBOOT = 0x54,
			 .VMAX = 0x54,
			 .VMIN = 0x30,
			 },
};

static struct svs_devinfo svs_devinfo;

static unsigned int svs_level;	/* debug info */
unsigned int stress_result = 1;	/* ATE stress */

/*
 * timer for log
 */
static struct hrtimer svs_log_timer;

static struct svs_det *id_to_svs_det(int id)
{
	if (likely(id < NR_SVS_DET))
		return &svs_detectors[id];
	else
		return NULL;
}

static struct svs_ctrl *id_to_svs_ctrl(int id)
{
	if (likely(id < NR_SVS_CTRL))
		return &svs_ctrls[id];
	else
		return NULL;
}

static void base_ops_enable(struct svs_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);
	det->disabled &= ~reason;
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_switch_bank(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	/* SYSTEMCLK_CG_EN = 31:31 */
	/* PTPODCORE0EN = 16:16 */

	svs_write_field(SVS_SVSCORESEL, 31:31, 0x1);
	svs_write_field(SVS_SVSCORESEL, 16:16, 0x1);
	/* APBSEL = 3:0 */
	svs_write_field(SVS_SVSCORESEL, 3:0, det->ctrl_id);

	if (svs_log_en)
		pr_err("SVS_SVSCORESEL = 0x%X\n", svs_read(SVS_SVSCORESEL));

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable_locked(struct svs_det *det, int reason)
{
	FUNC_ENTER(FUNC_LV_HELP);

	/* disable SVS */
	svs_write(SVS_SVSEN, 0x0);

	/* Clear SVS interrupt SVSINTSTS */
	svs_write(SVS_SVSINTSTS, 0x00ffffff);

	switch (reason) {
	case BY_MON_ERROR:
		/* set init2 value to DVFS table (PMIC) */
		memcpy(det->volt_tbl, det->volt_tbl_init2,
			sizeof(det->volt_tbl_init2));
		#ifdef CONFIG_MTK_UNIFY_POWER
		det->set_volt_to_upower = 0;
		#endif
		svs_set_svs_volt(det);
		break;

	case BY_INIT_ERROR:
	case BY_PROCFS:
	default:
		/* restore default DVFS table (PMIC) */
		svs_restore_svs_volt(det);
		break;
	}

	pr_notice("Disable SVS[%s] done.\n", det->name);
	det->disabled |= reason;

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_disable(struct svs_det *det, int reason)
{
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_svs_lock(&flags);
	det->ops->switch_bank(det);
	det->ops->disable_locked(det, reason);
	mt_svs_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_init01(struct svs_det *det)
{
	/* struct svs_ctrl *ctrl = id_to_svs_ctrl(det->ctrl_id); */

	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT01))) {
		pr_notice("det %s has no INIT01\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		pr_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	/* atomic_inc(&ctrl->in_init); */
	/* svs_init01_prepare(det); */
	/* det->ops->dump_status(det); */
	det->ops->set_phase(det, SVS_PHASE_INIT01);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_init02(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	if (unlikely(!HAS_FEATURE(det, FEA_INIT02))) {
		pr_notice("det %s has no INIT02\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		pr_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	det->ops->set_phase(det, SVS_PHASE_INIT02);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_mon_mode(struct svs_det *det)
{
	struct TS_SVS ts_info;
	enum thermal_bank_name ts_bank;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!HAS_FEATURE(det, FEA_MON)) {
		pr_notice("det %s has no MON mode\n", det->name);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	if (det->disabled & BY_PROCFS) {
		pr_notice("[%s] Disabled by PROCFS\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -2;
	}

	if (det->ctrl_id == SVS_CTRL_CPU) {
		ts_bank = THERMAL_BANK0;
	} else {
		pr_err("undefine det->ctrl_id = %d\n", det->ctrl_id);
		WARN_ON(1);
		ts_bank = THERMAL_BANK0;
	}

#ifdef CONFIG_THERMAL
	get_thermal_slope_intercept(&ts_info, ts_bank);
#else
	ts_info.ts_MTS = 0x1fb;
	ts_info.ts_BTS = 0x6d1;
#endif

	det->MTS = ts_info.ts_MTS;
	det->BTS = ts_info.ts_BTS;

	if ((det->SVSINITEN == 0x0) || (det->SVSMONEN == 0x0)) {
		pr_err("SVSINITEN = 0x%08X, SVSMONEN = 0x%08X\n",
			det->SVSINITEN, det->SVSMONEN);
		FUNC_EXIT(FUNC_LV_HELP);
		return 1;
	}

	det->ops->set_phase(det, SVS_PHASE_MON);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_get_status(struct svs_det *det)
{
	int status;
	unsigned long flags;

	FUNC_ENTER(FUNC_LV_HELP);

	mt_svs_lock(&flags);
	det->ops->switch_bank(det);
	status = (svs_read(SVS_SVSEN) != 0) ? 1 : 0;
	mt_svs_unlock(&flags);

	FUNC_EXIT(FUNC_LV_HELP);

	return status;
}

static void base_ops_dump_status(struct svs_det *det)
{
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	pr_notice("[%s]\n", det->name);

	pr_notice("SVSINITEN = 0x%08X\n", det->SVSINITEN);
	pr_notice("SVSMONEN = 0x%08X\n", det->SVSMONEN);
	pr_notice("MDES = 0x%08X\n", det->MDES);
	pr_notice("BDES = 0x%08X\n", det->BDES);
	pr_notice("DCMDET = 0x%08X\n", det->DCMDET);

	pr_notice("DCCONFIG = 0x%08X\n", det->DCCONFIG);
	pr_notice("DCBDET = 0x%08X\n", det->DCBDET);

	pr_notice("AGECONFIG = 0x%08X\n", det->AGECONFIG);
	pr_notice("AGEM = 0x%08X\n", det->AGEM);

	pr_notice("AGEDELTA = 0x%08X\n", det->AGEDELTA);
	pr_notice("DVTFIXED = 0x%08X\n", det->DVTFIXED);
	pr_notice("MTDES = 0x%08X\n", det->MTDES);
	pr_notice("VCO = 0x%08X\n", det->VCO);

	pr_notice("DETWINDOW = 0x%08X\n", det->DETWINDOW);
	pr_notice("VMAX = 0x%08X\n", det->VMAX);
	pr_notice("VMIN = 0x%08X\n", det->VMIN);
	pr_notice("DTHI = 0x%08X\n", det->DTHI);
	pr_notice("DTLO = 0x%08X\n", det->DTLO);
	pr_notice("VBOOT = 0x%08X\n", det->VBOOT);
	pr_notice("DETMAX = 0x%08X\n", det->DETMAX);

	pr_notice("DCVOFFSETIN = 0x%08X\n", det->DCVOFFSETIN);
	pr_notice("AGEVOFFSETIN = 0x%08X\n", det->AGEVOFFSETIN);

	pr_notice("MTS = 0x%08X\n", det->MTS);
	pr_notice("BTS = 0x%08X\n", det->BTS);

	pr_notice("num_freq_tbl = %d\n", det->num_freq_tbl);

	for (i = 0; i < det->num_freq_tbl; i++)
		pr_notice("freq_table_percent[%d] = %d\n", i,
			det->freq_table_percent[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		pr_notice("volt_tbl[%d] = %d\n", i, det->volt_tbl[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		pr_notice("volt_tbl_init2[%d] = %d\n", i,
			det->volt_tbl_init2[i]);

	for (i = 0; i < det->num_freq_tbl; i++)
		pr_notice("volt_tbl_pmic[%d] = %d\n", i, det->volt_tbl_pmic[i]);

	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_set_phase(struct svs_det *det, int phase)
{
	unsigned int i, filter, val;

	FUNC_ENTER(FUNC_LV_HELP);

	det->ops->switch_bank(det);
	/* config SVS register */
	svs_write(SVS_DESCHAR,
			((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	svs_write(SVS_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) | (det->DVTFIXED & 0xff)));
	svs_write(SVS_DETCHAR,
			((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	svs_write(SVS_AGECHAR,
			((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	svs_write(SVS_DCCONFIG, det->DCCONFIG);
	svs_write(SVS_AGECONFIG, det->AGECONFIG);
	svs_write(SVS_SVSCHKSHIFT,
			((svs_read(SVS_SVSCHKSHIFT) & ~0xff) | 0x77));

	if (phase == SVS_PHASE_MON)
		svs_write(SVS_TSCALCS,
				((det->BTS << 12) & 0xfff000)
				| (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		svs_write(SVS_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		svs_write(SVS_RUNCONFIG, val);
	}

	svs_write(SVS_FREQPCT30,
		  ((det->freq_table_percent[3] << 24) & 0xff000000) |
		  ((det->freq_table_percent[2] << 16) & 0xff0000) |
		  ((det->freq_table_percent[1] << 8) & 0xff00) |
		  (det->freq_table_percent[0] & 0xff));
	svs_write(SVS_FREQPCT74,
		  ((det->freq_table_percent[7] << 24) & 0xff000000) |
		  ((det->freq_table_percent[6] << 16) & 0xff0000) |
		  ((det->freq_table_percent[5] << 8) & 0xff00) |
		  ((det->freq_table_percent[4]) & 0xff));
	svs_write(SVS_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000) |
		  ((det->VMIN << 16) & 0xff0000) |
		  ((det->DTHI << 8) & 0xff00) | (det->DTLO & 0xff));
	svs_write(SVS_VBOOT, (((det->VBOOT) & 0xff)));
	svs_write(SVS_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	svs_write(SVS_SVSCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending SVS interrupt & config SVSINTEN */
	svs_write(SVS_SVSINTSTS, 0xffffffff);

	switch (phase) {
	case SVS_PHASE_INIT01:
		svs_write(SVS_SVSINTEN, 0x00005f01);
		/* enable SVS INIT measurement */
		svs_write(SVS_SVSEN, 0x00000001);
		break;

	case SVS_PHASE_INIT02:
		svs_write(SVS_SVSINTEN, 0x00005f01);

		svs_write(SVS_INIT2VALS,
			((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			(det->DCVOFFSETIN & 0xffff));

		/* enable SVS INIT measurement */
		svs_write(SVS_SVSEN, 0x00000005);
		break;

	case SVS_PHASE_MON:
		svs_write(SVS_SVSINTEN, 0x00FF0000);
		/* enable SVS monitor mode */
		svs_write(SVS_SVSEN, 0x00000002);
		break;

	default:
		WARN_ON(1);
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);
}

static int base_ops_get_temp(struct svs_det *det)
{
	int temperature = -1;

#ifdef CONFIG_THERMAL
	if (det_to_id(det) == SVS_DET_CPU) {
		/* TS1 temp */
		temperature = get_immediate_ts1_wrap();
	} else {
		pr_err("[Error] Wrong svs det_id = %d\n", (int)det_to_id(det));
		WARN_ON(1);
	}
#else
	temperature = 25000;
#endif
	return temperature;
}

static int base_ops_get_volt(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	pr_warn("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int base_ops_set_volt(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	pr_warn("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void base_ops_restore_default_volt(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	pr_warn("[%s] default func\n", __func__);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void base_ops_get_freq_volt_table(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

	det->freq_table_percent[0] = 100;
	det->num_freq_tbl = 1;

	FUNC_EXIT(FUNC_LV_HELP);
}

/* Will return 10uV */
static int get_volt_cpu(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	return regulator_get_voltage(det->reg); /* unit uv: micro voltage */
}

/* volt_tbl_pmic is convert from 10uV */
static int set_volt_cpu(struct svs_det *det)
{
	int opp_index;

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);

	for (opp_index = 0; opp_index < det->num_freq_tbl; opp_index++) {
		if (svs_log_en)
			pr_err("Set Voltage[%d] = %d\n",
			opp_index,
			SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[opp_index]));
			dev_pm_opp_adjust_voltage(det->dev,
				det->freq_table[opp_index],
			SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[opp_index]));
	}
	return 0;
}

static void restore_default_volt_cpu(struct svs_det *det)
{
	int opp_index;

	FUNC_ENTER(FUNC_LV_HELP);

	for (opp_index = 0; opp_index < det->num_freq_tbl; opp_index++)
		dev_pm_opp_adjust_voltage(det->dev, det->freq_table[opp_index],
				det->volt_table[opp_index]);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void get_freq_volt_table_cpu(struct svs_det *det)
{
	struct dev_pm_opp *opp;
	int i, count;
	unsigned long rate;

	FUNC_ENTER(FUNC_LV_HELP);

	/* det->freq_base = mt_cpufreq_max_frequency_by_DVS(cpu, 0); */
	/* XXX: defined @ svs_detectors[] */
	det->dev = get_cpu_device(det->dev_id);

	if (!det->dev)
		pr_err("failed to get cpu%d device\n", det->dev_id);

	/* Assume CPU DVFS OPP table is already initialized by cpufreq driver */
	rcu_read_lock();
	count = dev_pm_opp_get_opp_count(det->dev);
	if (count > NR_FREQ)
		pr_err("We only support OPP %d entries but got %d?\n",
			NR_FREQ, count);

	for (i = 0, rate = (unsigned long)-1; i < NR_FREQ && i < count;
			i++, rate--) {
		opp = dev_pm_opp_find_freq_floor(det->dev, &rate);
		if (IS_ERR(opp)) {
			pr_err("error opp entry!!, err = %ld\n", PTR_ERR(opp));
			break;
		}

		det->freq_table[i] = rate;
		det->volt_table[i] = dev_pm_opp_get_voltage(opp);
		det->freq_table_percent[i] =
			PERCENT_U64(det->freq_table[i], det->freq_base);

		if (svs_log_en)
			pr_err("cpu: freq_table[%d] = %llu, volt_table[%d] = %d, freq_percent[%d] = %d\n",
			  i, det->freq_table[i], i, det->volt_table[i],
			  i, det->freq_table_percent[i]);
	}
	rcu_read_unlock();

	det->num_freq_tbl = i;

	FUNC_EXIT(FUNC_LV_HELP);
}

void mt_svs_lock(unsigned long *flags)
{
	spin_lock_irqsave(&svs_spinlock, *flags);
}
EXPORT_SYMBOL(mt_svs_lock);

void mt_svs_unlock(unsigned long *flags)
{
	spin_unlock_irqrestore(&svs_spinlock, *flags);
}
EXPORT_SYMBOL(mt_svs_unlock);

/*
 * timer for log
 */
static enum hrtimer_restart svs_log_timer_func(struct hrtimer *timer)
{
	struct svs_det *det;

	 FUNC_ENTER(FUNC_LV_HELP);

	 for_each_det(det) {
		pr_notice("SVS_LOG: SVS [%s](%d) -(%d, %d, %d, %d, %d, %d, %d, %d)-(%d, %d, %d, %d, %d, %d, %d, %d)\n",
			   det->name, det->ops->get_temp(det),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
			   SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
			   det->freq_table_percent[0],
			   det->freq_table_percent[1],
			   det->freq_table_percent[2],
			   det->freq_table_percent[3],
			   det->freq_table_percent[4],
			   det->freq_table_percent[5],
			   det->freq_table_percent[6],
			   det->freq_table_percent[7]);
	}

	hrtimer_forward_now(timer, ns_to_ktime(LOG_INTERVAL));
	FUNC_EXIT(FUNC_LV_HELP);

	return HRTIMER_RESTART;
}

/*
 * Thread for voltage setting
 */
static int svs_volt_thread_handler(void *data)
{
	struct svs_ctrl *ctrl = (struct svs_ctrl *)data;
	struct svs_det *det = id_to_svs_det(ctrl->det_id);

	 FUNC_ENTER(FUNC_LV_HELP);

	do {
		wait_event_interruptible(ctrl->wq, ctrl->volt_update);

		if ((ctrl->volt_update & SVS_VOLT_UPDATE) && det->ops->set_volt)
			det->ops->set_volt(det);

		if ((ctrl->volt_update & SVS_VOLT_RESTORE)
				&& det->ops->restore_default_volt)
			det->ops->restore_default_volt(det);

		ctrl->volt_update = SVS_VOLT_NONE;

	} while (!kthread_should_stop());

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static void inherit_base_det(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_HELP);

#define INIT_OP(ops, func)					\
		do {							\
			if (ops->func == NULL)				\
				ops->func = svs_det_base_ops.func;	\
		} while (0)

	INIT_OP(det->ops, disable);
	INIT_OP(det->ops, disable_locked);
	INIT_OP(det->ops, switch_bank);
	INIT_OP(det->ops, init01);
	INIT_OP(det->ops, init02);
	INIT_OP(det->ops, mon_mode);
	INIT_OP(det->ops, get_status);
	INIT_OP(det->ops, dump_status);
	INIT_OP(det->ops, set_phase);
	INIT_OP(det->ops, get_temp);
	INIT_OP(det->ops, get_volt);
	INIT_OP(det->ops, set_volt);
	INIT_OP(det->ops, restore_default_volt);
	INIT_OP(det->ops, get_freq_volt_table);

	FUNC_EXIT(FUNC_LV_HELP);
}
static void svs_init_ctrl(struct svs_ctrl *ctrl)
{
	FUNC_ENTER(FUNC_LV_HELP);

	init_completion(&ctrl->init_done);

	init_waitqueue_head(&ctrl->wq);
	ctrl->thread = kthread_run(svs_volt_thread_handler, ctrl, ctrl->name);

	if (IS_ERR(ctrl->thread))
		pr_err("Create %s thread failed: %ld\n", ctrl->name,
			  PTR_ERR(ctrl->thread));

	FUNC_EXIT(FUNC_LV_HELP);
}

static int limit_mcusys_env(struct svs_det *det)
{
	int ret = 0;
	int opp_index;
	int u_vboot = SVS_PMIC_VAL_TO_VOLT(det->VBOOT);

	if (svs_log_en)
		pr_err("%s()\n", __func__);

	/* configure regulator to PWM mode */
	ret = regulator_set_mode(det->reg, REGULATOR_MODE_FAST);
	if (ret)
		pr_err("%s: Failed to set regulator in PWM mode, ret = %d\n",
			det->name, ret);

	/* Backup current cpufreq policy */
	ret = cpufreq_get_policy(&det->svs_cpufreq_policy, det->dev_id);
	if (ret) {
		pr_err("%s: cpufreq is not ready. ret = %d\n", __func__, ret);
		return ret;
	}

	if (svs_log_en) {
		pr_err("svs_cpufreq_policy.max = %d\n",
				det->svs_cpufreq_policy.max);
		pr_err("svs_cpufreq_policy.min = %d\n",
				det->svs_cpufreq_policy.min);
	}

	/* Force CPUFreq to switch to OPP with VBOOT volt */
	for (opp_index = 0; opp_index < det->num_freq_tbl; opp_index++) {

		/* Find the fastest freq and update corresponding
		 * voltage to VBOOT. Cpufreq will be fixed to
		 * that frequency until initialization of SVS is done.
		 */
		if (det->volt_table[opp_index] <= u_vboot &&
			(!det->dvfs_max_freq_khz || !det->dvfs_min_freq_khz)) {

			det->dvfs_max_freq_khz =
				div_u64(det->freq_table[opp_index], 1000);
			det->dvfs_min_freq_khz =
				div_u64(det->freq_table[opp_index], 1000);
			ret = dev_pm_opp_adjust_voltage(det->dev,
						det->freq_table[opp_index],
						u_vboot);
			pr_err("det->freq_table[%i]= %d\n", opp_index, u_vboot);
			if (ret) {
				pr_err("Fail dev_pm_opp_adjust_voltage(), ret = %d\n",
						ret);
				return ret;
			}

			break;
		}
	}

	/* call svs_cpufreq_notifier(), try to fix dvfs freq */
	cpufreq_update_policy(det->dev_id);

	return ret;
}

#define _BIT_(_bit_)                        \
		(unsigned int)(1 << (_bit_))
#define _BITMASK_(_bits_)                   \
		(((unsigned int) -1 >> (31 - ((1) ? _bits_))) \
		& ~((1U << ((0) ? _bits_)) - 1))
#define _GET_BITS_VAL_(_bits_, _val_)       \
		(((_val_) & (_BITMASK_(_bits_))) >> ((0) ? _bits_))

static int svs_init_det(struct svs_det *det, struct svs_devinfo *devinfo)
{
	int det_id = det_to_id(det);
	int ret;

	FUNC_ENTER(FUNC_LV_HELP);

	if (svs_log_en)
		pr_notice("det name=%s,det_id=%d\n", det->name, det_id);

	inherit_base_det(det);

	/* init with devinfo */
	det->SVSINITEN = devinfo->SVSINITEN;
	det->SVSMONEN = devinfo->SVSMONEN;

	/* init with constant */
	det->DETWINDOW = DETWINDOW_VAL;

	det->DTHI = DTHI_VAL;
	det->DTLO = DTLO_VAL;
	det->DETMAX = DETMAX_VAL;

	det->AGECONFIG = AGECONFIG_VAL;
	det->AGEM = AGEM_VAL;
	det->VCO = VCO_VAL;
	det->DCCONFIG = DCCONFIG_VAL;
#if 0
	if (det->ops->get_volt != NULL) {
		det->VBOOT = SVS_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));
		pr_alert("@%s(), det->VBOOT = %d\n", __func__, det->VBOOT);
	}
#endif

	/* get DVFS frequency table */
	det->ops->get_freq_volt_table(det);

	switch (det_id) {
	case SVS_DET_CPU:
		det->MDES = devinfo->CPU_MDES;
		det->BDES = devinfo->CPU_BDES;
		det->DCMDET = devinfo->CPU_DCMDET;
		det->DCBDET = devinfo->CPU_DCBDET;
		det->AGEDELTA = devinfo->CPU_AGEDELTA;
		det->MTDES = devinfo->CPU_MTDES;
		det->DVTFIXED = 0x06;
		ret = limit_mcusys_env(det);
		break;
	default:
		pr_err("[%s]: Unknown det_id %d\n", __func__, det_id);
		WARN_ON(1);
		break;
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return ret;
}

#ifdef CONFIG_MTK_UNIFY_POWER
static enum upower_bank transfer_svs_to_upower_bank(unsigned int det_id)
{
	enum upower_bank bank;

	switch (det_id) {
	case SVS_DET_CPU:
		bank = UPOWER_BANK_L;
		break;
	default:
		bank = NR_UPOWER_BANK;
		break;
	}
	return bank;
}

static void svs_update_init2_volt_to_upower(struct svs_det *det,
	unsigned int *pmic_volt)
{
	unsigned int volt_tbl[NR_FREQ];
	enum upower_bank bank;
	int i;

	for (i = 0; i < det->num_freq_tbl; i++)
		volt_tbl[i] = SVS_PMIC_VAL_TO_VOLT(pmic_volt[i]) / 10;

	bank = transfer_svs_to_upower_bank(det_to_id(det));
	if (bank < NR_UPOWER_BANK)
		upower_update_volt_by_svs(bank, volt_tbl, det->num_freq_tbl);

}
#endif

static int unlimit_mcusys_env(struct svs_det *det)
{
	int ret;

	if (svs_log_en)
		pr_err("%s()\n", __func__);

	/* configure regulator to normal mode */
	ret = regulator_set_mode(det->reg, REGULATOR_MODE_NORMAL);
	if (ret)
		pr_err("%s: Failed to set regulator in normal mode, ret = %d\n",
				det->name, ret);

	if (svs_log_en) {
		pr_err("%s: svs_cpufreq_policy.max = %d\n",
				det->name, det->svs_cpufreq_policy.max);
		pr_err("%s: svs_cpufreq_policy.min = %d\n",
				det->name, det->svs_cpufreq_policy.min);
	}

	/* Backup current cpufreq policy */
	if (det->svs_cpufreq_policy.max && det->svs_cpufreq_policy.min) {
		/* unlimit CPUFreq OPP range */
		det->dvfs_max_freq_khz = det->svs_cpufreq_policy.max;
		det->dvfs_min_freq_khz = det->svs_cpufreq_policy.min;
		cpufreq_update_policy(det->dev_id);
	} else {
		pr_err("%s(): cpufreq is not ready.\n", __func__);
	}

	return ret;
}

static int svs_init_det_done(struct svs_det *det)
{
	int det_id = det_to_id(det);
	int ret;

	switch (det_id) {
	case SVS_DET_CPU:
		ret = unlimit_mcusys_env(det);
		break;
	default:
		pr_err("unknown det id\n");
		WARN_ON(1);
		break;
	}

	return 0;
}

static void svs_set_svs_volt(struct svs_det *det)
{
#if SET_PMIC_VOLT
	int i, cur_temp, low_temp_offset;
	unsigned int clamp_signed_off_vmax;
	struct svs_ctrl *ctrl = id_to_svs_ctrl(det->ctrl_id);

	cur_temp = det->ops->get_temp(det);

	if (svs_log_en)
		pr_err("%s(): cur_temp = %d\n", __func__, cur_temp);
#ifdef CONFIG_MTK_UNIFY_POWER
		upower_update_degree_by_svs(
			transfer_svs_to_upower_bank(det_to_id(det)),
			cur_temp / 1000);
#endif

	if (cur_temp <= 33000)
		low_temp_offset = 10;
	else
		low_temp_offset = 0;

	for (i = 0; i < det->num_freq_tbl; i++) {
		clamp_signed_off_vmax =
				SVS_VOLT_TO_PMIC_VAL(det->volt_table[i]);
		det->volt_tbl_pmic[i] =
		clamp(det->volt_tbl[i] + det->volt_offset + low_temp_offset,
		det->VMIN, clamp_signed_off_vmax);
		pr_err("%s(): V_offset=%d,T_offset=%d,clamp_vmax = %d,volt_pmic[%d]=%d\n",
				__func__, det->volt_offset, low_temp_offset,
				clamp_signed_off_vmax, i,
				det->volt_tbl_pmic[i]);
	}

	ctrl->volt_update |= SVS_VOLT_UPDATE;
	wake_up_interruptible(&ctrl->wq);
	/* only when set_volt_to_upower == 0, */
	/* the volt will be apply to upower */
#ifdef CONFIG_MTK_UNIFY_POWER

	if (det->set_volt_to_upower == 0) {
		svs_update_init2_volt_to_upower(det, det->volt_tbl_pmic);
		det->set_volt_to_upower = 1;
	}
#endif

#endif

	FUNC_ENTER(FUNC_LV_HELP);
	FUNC_EXIT(FUNC_LV_HELP);
}

static void svs_restore_svs_volt(struct svs_det *det)
{
#if SET_PMIC_VOLT
	struct svs_ctrl *ctrl = id_to_svs_ctrl(det->ctrl_id);

	ctrl->volt_update |= SVS_VOLT_RESTORE;
	wake_up_interruptible(&ctrl->wq);
#endif

	 FUNC_ENTER(FUNC_LV_HELP);
	 FUNC_EXIT(FUNC_LV_HELP);
}

static inline void handle_init01_isr(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	if (svs_log_en)
		pr_notice("@ %s(%s)\n", __func__, det->name);

	det->dcvalues[SVS_PHASE_INIT01] = svs_read(SVS_DCVALUES);
	det->svs_freqpct30[SVS_PHASE_INIT01] = svs_read(SVS_FREQPCT30);
	det->svs_26c[SVS_PHASE_INIT01] = svs_read(SVS_SVSINTEN + 0x10);
	det->svs_vop30[SVS_PHASE_INIT01] = svs_read(SVS_VOP30);
	det->svs_svsen[SVS_PHASE_INIT01] = svs_read(SVS_SVSEN);

	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++)
			det->reg_dump_data[i][SVS_PHASE_INIT01] =
			    svs_read(SVS_BASEADDR + reg_dump_addr_off[i]);
	}

	pr_notice("%s: %s: VDN74:0x%08x, VDN30:0x%08x, DCVALUES:0x%08x\n",
			   det->name, __func__, svs_read(SVS_VDESIGN74),
			   svs_read(SVS_VDESIGN30), svs_read(SVS_DCVALUES));

	/*
	 * Read & store 16 bit values DCVALUES.DCVOFFSET and
	 * AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	det->DCVOFFSETIN = ~(svs_read(SVS_DCVALUES) & 0xffff) + 1;
	/* check if DCVALUES is minus and set DCVOFFSETIN to zero */
	if (det->DCVOFFSETIN & 0x8000) {
		pr_notice("%s: DCVOFFSETIN is minus, set 0\n", det->name);
		det->DCVOFFSETIN = 0;
	}

	det->AGEVOFFSETIN = svs_read(SVS_AGEVALUES) & 0xffff;

	/*
	 * Set SVSEN.SVSINITEN/SVSEN.SVSINIT2EN = 0x0 &
	 * Clear SVS INIT interrupt SVSINTSTS = 0x00000001
	 */
	svs_write(SVS_SVSEN, 0x0);
	svs_write(SVS_SVSINTSTS, 0x1);
	/* svs_init01_finish(det); */
	det->ops->init02(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init02_isr(struct svs_det *det)
{
	unsigned int temp;
	int i;
	struct svs_ctrl *ctrl = id_to_svs_ctrl(det->ctrl_id);

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (svs_log_en)
		pr_notice("@ %s(%s)\n", __func__, det->name);

	det->dcvalues[SVS_PHASE_INIT02] = svs_read(SVS_DCVALUES);
	det->svs_freqpct30[SVS_PHASE_INIT02] = svs_read(SVS_FREQPCT30);
	det->svs_26c[SVS_PHASE_INIT02] = svs_read(SVS_SVSINTEN + 0x10);
	det->svs_vop30[SVS_PHASE_INIT02] = svs_read(SVS_VOP30);
	det->svs_svsen[SVS_PHASE_INIT02] = svs_read(SVS_SVSEN);
	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][SVS_PHASE_INIT02] =
			    svs_read(SVS_BASEADDR + reg_dump_addr_off[i]);
		}
	}
	pr_notice("%s: %s: VOP74:0x%08x,VOP30:0x%08x,DCVALUES:0x%08x\n",
			det->name, __func__, svs_read(SVS_VOP74),
			svs_read(SVS_VOP30), svs_read(SVS_DCVALUES));

	temp = svs_read(SVS_VOP30);
	/* SVS_VOP30=>pmic value */
	det->volt_tbl[0] = (temp & 0xff);
	det->volt_tbl[1] = ((temp >> 8) & 0xff);
	det->volt_tbl[2] = ((temp >> 16) & 0xff);
	det->volt_tbl[3] = ((temp >> 24) & 0xff);

	temp = svs_read(SVS_VOP74);
	/* SVS_VOP74=>pmic value */
	det->volt_tbl[4] = (temp & 0xff);
	det->volt_tbl[5] = ((temp >> 8) & 0xff);
	det->volt_tbl[6] = ((temp >> 16) & 0xff);
	det->volt_tbl[7] = ((temp >> 24) & 0xff);

	/* backup to volt_tbl_init2 */
	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	if (svs_log_en)
		for (i = 0; i < NR_FREQ; i++)
			pr_notice("svs_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
				det->name, i, det->volt_tbl[i],
				SVS_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));

	svs_set_svs_volt(det);

	if (stress_result == 1)
		stress_result = 0;

	/*
	 * Set SVSEN.SVSINITEN/SVSEN.SVSINIT2EN = 0x0 &
	 * Clear SVS INIT interrupt SVSINTSTS = 0x00000001
	 */
	svs_write(SVS_SVSEN, 0x0);
	svs_write(SVS_SVSINTSTS, 0x1);

	/* atomic_dec(&ctrl->in_init); */
	complete(&ctrl->init_done);
	det->ops->mon_mode(det);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_init_err_isr(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);
	pr_notice("====================================================\n");
	pr_notice("SVS init err: SVSEN(%p) = 0x%08X, SVSINTSTS(%p) = 0x%08X\n",
			SVS_SVSEN, svs_read(SVS_SVSEN),
			SVS_SVSINTSTS, svs_read(SVS_SVSINTSTS));
	pr_notice("SVS_SMSTATE0 (%p) = 0x%08X\n",
			SVS_SMSTATE0, svs_read(SVS_SMSTATE0));
	pr_notice("SVS_SMSTATE1 (%p) = 0x%08X\n",
			SVS_SMSTATE1, svs_read(SVS_SMSTATE1));
	pr_notice("====================================================\n");

	{
		struct svs_ctrl *ctrl = id_to_svs_ctrl(det->ctrl_id);
		/* atomic_dec(&ctrl->in_init); */
		complete(&ctrl->init_done);
	}
#if 0
	aee_kernel_warning("mt_svs", "@%s():%d, VBOOT(%s) = 0x%08X\n",
			__func__, __LINE__, det->name, det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_INIT_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_mode_isr(struct svs_det *det)
{
	unsigned int temp;
	int i;

	FUNC_ENTER(FUNC_LV_LOCAL);

	if (svs_log_en)
		pr_notice("@ %s(%s)\n", __func__, det->name);

	det->dcvalues[SVS_PHASE_MON] = svs_read(SVS_DCVALUES);
	det->svs_freqpct30[SVS_PHASE_MON] = svs_read(SVS_FREQPCT30);
	det->svs_26c[SVS_PHASE_MON] = svs_read(SVS_SVSINTEN + 0x10);
	det->svs_vop30[SVS_PHASE_MON] = svs_read(SVS_VOP30);
	det->svs_svsen[SVS_PHASE_MON] = svs_read(SVS_SVSEN);

	{
		int i;

		for (i = 0; i < ARRAY_SIZE(reg_dump_addr_off); i++) {
			det->reg_dump_data[i][SVS_PHASE_MON] =
			    svs_read(SVS_BASEADDR + reg_dump_addr_off[i]);
		}
	}

	/* check if thermal sensor init completed? */
	temp = (svs_read(SVS_TEMP) & 0xff);

	if ((temp > 0x4b) && (temp < 0xd3)) {
		pr_notice("thermal sensor init not completed.(temp = 0x%08X)\n",
				temp);
		goto out;
	}

	temp = svs_read(SVS_VOP30);
	det->volt_tbl[0] = (temp & 0xff);
	det->volt_tbl[1] = ((temp >> 8) & 0xff);
	det->volt_tbl[2] = ((temp >> 16) & 0xff);
	det->volt_tbl[3] = ((temp >> 24) & 0xff);

	temp = svs_read(SVS_VOP74);
	det->volt_tbl[4] = (temp & 0xff);
	det->volt_tbl[5] = ((temp >> 8) & 0xff);
	det->volt_tbl[6] = ((temp >> 16) & 0xff);
	det->volt_tbl[7] = ((temp >> 24) & 0xff);

	if (svs_log_en)
		for (i = 0; i < NR_FREQ; i++)
			pr_notice("svs_detectors[%s].volt_tbl[%d] = 0x%08X (%d)\n",
				det->name, i, det->volt_tbl[i],
				SVS_PMIC_VAL_TO_VOLT(det->volt_tbl[i]));

	svs_set_svs_volt(det);
out:
	/* Clear SVS INIT interrupt SVSINTSTS = 0x00ff0000 */
	svs_write(SVS_SVSINTSTS, 0x00ff0000);

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static inline void handle_mon_err_isr(struct svs_det *det)
{
	FUNC_ENTER(FUNC_LV_LOCAL);

	/* SVS Monitor mode error handler */
	pr_notice("====================================================\n");
	pr_notice("SVS mon err: SVSEN(%p) = 0x%08X, SVSINTSTS(%p) = 0x%08X\n",
		     SVS_SVSEN, svs_read(SVS_SVSEN),
		     SVS_SVSINTSTS, svs_read(SVS_SVSINTSTS));
	pr_notice("SVS_SMSTATE0 (%p) = 0x%08X\n",
					SVS_SMSTATE0, svs_read(SVS_SMSTATE0));
	pr_notice("SVS_SMSTATE1 (%p) = 0x%08X\n",
					SVS_SMSTATE1, svs_read(SVS_SMSTATE1));
	pr_notice("SVS_TEMP (%p) = 0x%08X\n", SVS_TEMP, svs_read(SVS_TEMP));
	pr_notice("SVS_TEMPMSR0 (%p) = 0x%08X\n",
					SVS_TEMPMSR0, svs_read(SVS_TEMPMSR0));
	pr_notice("SVS_TEMPMSR1 (%p) = 0x%08X\n",
					SVS_TEMPMSR1, svs_read(SVS_TEMPMSR1));
	pr_notice("SVS_TEMPMSR2 (%p) = 0x%08X\n",
					SVS_TEMPMSR2, svs_read(SVS_TEMPMSR2));
	pr_notice("SVS_TEMPMONCTL0 (%p) = 0x%08X\n",
					SVS_TEMPMONCTL0,
					svs_read(SVS_TEMPMONCTL0));
	pr_notice("SVS_TEMPMSRCTL1 (%p) = 0x%08X\n",
					SVS_TEMPMSRCTL1,
					svs_read(SVS_TEMPMSRCTL1));
	pr_notice("====================================================\n");
#if 0
	aee_kernel_warning("mt_svs", "@%s():%d, VBOOT(%s) = 0x%08X\n",
					__func__, __LINE__,
					det->name, det->VBOOT);
#endif
	det->ops->disable_locked(det, BY_MON_ERROR);

	FUNC_EXIT(FUNC_LV_LOCAL);
}
static inline void svs_isr_handler(struct svs_det *det)
{
	unsigned int SVSINTSTS, SVSEN;

	FUNC_ENTER(FUNC_LV_LOCAL);

	SVSINTSTS = svs_read(SVS_SVSINTSTS);
	SVSEN = svs_read(SVS_SVSEN);

	if (svs_log_en) {
		pr_notice("[%s]\n", det->name);
		pr_notice("SVSINTSTS = 0x%08X\n", SVSINTSTS);
		pr_notice("SVS_SVSEN = 0x%08X\n", SVSEN);
		pr_notice("SVS_SVSCORESEL = 0x%X\n", svs_read(SVS_SVSCORESEL));
	}

	if (SVSINTSTS == 0x1) {	/* SVS init1 or init2 */
		if ((SVSEN & 0x7) == 0x1)	/* SVS init1 */
			handle_init01_isr(det);
		else if ((SVSEN & 0x7) == 0x5)	/* SVS init2 */
			handle_init02_isr(det);
		else {
			handle_init_err_isr(det);
		}
	} else if ((SVSINTSTS & 0x00ff0000) != 0x0)
		handle_mon_mode_isr(det);
	else {
		if (((SVSEN & 0x7) == 0x1) || ((SVSEN & 0x7) == 0x5))
			handle_init_err_isr(det);
		else
			handle_mon_err_isr(det);
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

static irqreturn_t svs_isr(int irq, void *dev_id)
{
	unsigned long flags;
	struct svs_det *det = NULL;
	int i;

	FUNC_ENTER(FUNC_LV_MODULE);

	mt_svs_lock(&flags);

	for (i = 0; i < NR_SVS_CTRL; i++) {
		/* if (i == SVS_CTRL_VCORE) */
		/* continue; */

		if ((BIT(i) & svs_read(SVS_SVSINTST)))
			continue;

		det = &svs_detectors[i];

		det->ops->switch_bank(det);
		/* mt_svs_reg_dump_locked(); */
		svs_isr_handler(det);
	}

	mt_svs_unlock(&flags);

	FUNC_EXIT(FUNC_LV_MODULE);

	return IRQ_HANDLED;
}

void svs_init01(void)
{
	struct svs_det *det;
	struct svs_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_LOCAL);

	for_each_det_ctrl(det, ctrl) {
		{
			unsigned long flag;
			unsigned int vboot;

			vboot = SVS_VOLT_TO_PMIC_VAL(det->ops->get_volt(det));

			if (svs_log_en)
				pr_alert("@%s(),vboot = %0x\n",
						__func__, vboot);

			if (vboot != det->VBOOT) {
				pr_err("%s: VBOOT mis-match\n", det->name);
				pr_err("@%s():%d, get_volt(%s) = 0x%08X, VBOOT = 0x%08X\n",
					  __func__, __LINE__, det->name,
					  vboot, det->VBOOT);
#if 0
aee_kernel_warning("mt_svs", "@%s():%d,volt(%s)=0x%08X,VBOOT=0x%08X\n",
					__func__, __LINE__, det->name,
					vboot, det->VBOOT);
#endif

				return;
			}

			mt_svs_lock(&flag);
			det->ops->init01(det);
			mt_svs_unlock(&flag);

			wait_for_completion(&ctrl->init_done);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

void svs_init02(void)
{
	struct svs_det *det;
	struct svs_ctrl *ctrl;

	 FUNC_ENTER(FUNC_LV_LOCAL);

	 for_each_det_ctrl(det, ctrl) {
		if (HAS_FEATURE(det, FEA_MON)) {
			unsigned long flag;

			 mt_svs_lock(&flag);
			 det->ops->init02(det);
			 mt_svs_unlock(&flag);
		}
	}

	FUNC_EXIT(FUNC_LV_LOCAL);
}

#if EN_SVS_OD

void svs_set_pi_efuse(int id, unsigned int pi_efuse)
{
	struct svs_det *det = id_to_svs_det(id);

	det->pi_efuse = pi_efuse;
}

int get_devinfo(struct svs_devinfo *p, struct device *dev)
{
	unsigned int *M_HW_RES = (unsigned int *)p;
	unsigned int *svs_calibration_data;
	unsigned int mtdes_idx, bdes_mdes_idx;
	struct svs_det *det;

	struct nvmem_cell *cell;
	char *svs_cell_name = "svs_calibration";
	size_t len;

	FUNC_ENTER(FUNC_LV_HELP);

	cell = nvmem_cell_get(dev, svs_cell_name);
	if (IS_ERR(cell)) {
		pr_err("cannot get %s from device tree\n", svs_cell_name);
		return -ENODEV;
	}

	svs_calibration_data = (unsigned int *) nvmem_cell_read(cell, &len);
	nvmem_cell_put(cell);

	if (IS_ERR(M_HW_RES)) {
		pr_err("get invalid address from nvmem framework\n");
		return -EFAULT;
	} else if (len != sizeof(unsigned int) * 6) {
		pr_err("len = %zu, invalid length of svs calibration data\n",
				len);
		return -EINVAL;
	}

	pr_crit("svs_calibration_data[0] = 0x%08x\n", svs_calibration_data[0]);
	pr_crit("svs_calibration_data[1] = 0x%08x\n", svs_calibration_data[1]);
	pr_crit("svs_calibration_data[2] = 0x%08x\n", svs_calibration_data[2]);

	/* Update MTDES/BDES/MDES if they are modified by PICACHU. */
	for_each_det(det) {
		if (!det->pi_efuse)
			continue;

		bdes_mdes_idx = (det->ctrl_id << 1) + 1;
		mtdes_idx = bdes_mdes_idx + 1;

		/* Update BDES */
		svs_calibration_data[bdes_mdes_idx] =
		(svs_calibration_data[bdes_mdes_idx] & 0xFFFFFF00) |
		((det->pi_efuse >> PI_SVS_BDES_START_BIT) & 0xff);

		/* Update MDES */
		svs_calibration_data[bdes_mdes_idx] =
		(svs_calibration_data[bdes_mdes_idx] & 0xFFFF00FF) |
		(((det->pi_efuse >> PI_SVS_BDES_START_BIT) & 0x0000FF00));

		/* Update MTDES */
		svs_calibration_data[mtdes_idx] =
		(svs_calibration_data[mtdes_idx] & 0xFF00FFFF) |
		(((det->pi_efuse >> PI_SVS_MTDES_START_BIT) & 0xff) << 16);
	}

	M_HW_RES[0] = svs_calibration_data[0];
	M_HW_RES[1] = svs_calibration_data[1];
	M_HW_RES[2] = svs_calibration_data[2];

	pr_crit("M_HW_RES[0] = 0x%08x\n", M_HW_RES[0]);
	pr_crit("M_HW_RES[1] = 0x%08x\n", M_HW_RES[1]);
	pr_crit("M_HW_RES[2] = 0x%08x\n", M_HW_RES[2]);

	if (svs_log_en) {
		pr_crit("p->SVSINITEN= 0x%08x\n", p->SVSINITEN);
		pr_crit("p->SVSMONEN = 0x%08x\n", p->SVSMONEN);
	}

	kfree(svs_calibration_data);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int svs_get_regulator(struct platform_device *pdev, struct svs_det *det)
{
	int det_id = det_to_id(det);
	char vproc[8] = "vproc";

	switch (det_id) {
	case SVS_DET_CPU:
		det->reg = devm_regulator_get_optional(&pdev->dev, vproc);
		if (IS_ERR(det->reg)) {
			pr_err("Failed to get %s regulator, err = %ld\n",
				vproc, PTR_ERR(det->reg));
			return PTR_ERR(det->reg);
		}
		break;
	default:
		pr_err("unkwon det_id = %d\n", det_id);
		WARN_ON(1);
		break;
	}

	return 0;
}

static int svs_cpufreq_notifier(struct notifier_block *nb,
	unsigned long event,	void *data)
{
	struct cpufreq_policy *policy = data;
	struct svs_det *svs_det_cpu = &svs_detectors[0];

	switch (event) {
	case CPUFREQ_ADJUST:
		if (!svs_det_cpu->dvfs_min_freq_khz
			|| !svs_det_cpu->dvfs_max_freq_khz)
			return NOTIFY_DONE;

		if (svs_log_en) {
			pr_err("start to run %s()\n", __func__);
			pr_err("mcusys: dvfs_max_freq = %ld\n",
				svs_det_cpu->dvfs_max_freq_khz);
			pr_err("mcusys: dvfs_min_freq = %ld\n",
				svs_det_cpu->dvfs_min_freq_khz);
		}

		cpufreq_verify_within_limits(policy,
				svs_det_cpu->dvfs_min_freq_khz,
				svs_det_cpu->dvfs_max_freq_khz);
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_DONE;
}

static struct notifier_block svs_cpufreq_notifier_block = {
	.notifier_call = svs_cpufreq_notifier,
};

static int svs_cpufreq_notifier_registration(int registration)
{
	int ret;

	if (registration)
		ret = cpufreq_register_notifier(&svs_cpufreq_notifier_block,
			CPUFREQ_POLICY_NOTIFIER);
	else
		ret = cpufreq_unregister_notifier(&svs_cpufreq_notifier_block,
			CPUFREQ_POLICY_NOTIFIER);

	if (ret) {
		pr_err("%s(): failed error number (%d)\n",
		registration ? "register_notifier" : "unregister_notifier",
		ret);
		WARN_ON(1);
	}

	return ret;
}


static int svs_probe(struct platform_device *pdev)
{
	int ret;
	struct svs_det *det;
	struct svs_ctrl *ctrl;

	FUNC_ENTER(FUNC_LV_MODULE);

	ret = get_devinfo(&svs_devinfo, &pdev->dev);
	if (ret)
		return ret;

	if (svs_devinfo.SVSINITEN == 0) {
		pr_err("SVSINITEN = 0x%08X\n", svs_devinfo.SVSINITEN);
		FUNC_EXIT(FUNC_LV_MODULE);
		return -EINVAL;
	}

	/* This might return -EPROBE_DEFER */
	for_each_det(det) {
		ret = svs_get_regulator(pdev, det);
		if (ret)
			return ret;
	}

	main_clk = devm_clk_get(&pdev->dev, "main_clk");
	if (IS_ERR(main_clk)) {
		dev_dbg(&pdev->dev, "failed to get clock: %ld\n",
			PTR_ERR(main_clk));
		return PTR_ERR(main_clk);
	}

	ret = clk_prepare_enable(main_clk);
	if (ret)
		return ret;

	/*
	 * init timer for log / volt
	 */
	hrtimer_init(&svs_log_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	svs_log_timer.function = svs_log_timer_func;

	create_procfs();

	/* set SVS IRQ */
	ret = request_irq(svs_irq_number, svs_isr, IRQF_TRIGGER_LOW,
		"svs", NULL);
	if (WARN_ON(ret)) {
		pr_err("SVS IRQ register failed (%d)\n", ret);
		return -EINVAL;
	}

	if (svs_log_en)
		pr_notice("Set SVS IRQ OK.\n");

	ret = svs_cpufreq_notifier_registration(true);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_UNIFY_POWER
	informSVSisReady = 1;
#endif

	/*
	 * we have to make sure all CPUs are on and working at VBOOT volt.
	 * Add a pm_qos request to prevent CPUs from
	 * entering CPU off idle state.
	 */
	pm_qos_add_request(&qos_request, PM_QOS_CPU_DMA_LATENCY, 1);

	/* atomic_set(&svs_init01_cnt, 0); */
	for_each_ctrl(ctrl) {
		svs_init_ctrl(ctrl);
	}

	is_svs_initializing = true;

	for_each_det(det) {
		ret = svs_init_det(det, &svs_devinfo);
		if (ret)
			break;
	}

	if (!ret)
		svs_init01();

	for_each_det(det)
		svs_init_det_done(det);

	is_svs_initializing = false;

	svs_cpufreq_notifier_registration(false);
	pm_qos_remove_request(&qos_request);

	if (svs_log_en)
		pr_err("SVS init is done\n");

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static int svs_suspend(struct platform_device *pdev, pm_message_t state)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	/* turn on system clock gating */
	svs_write_field(SVS_SVSCORESEL, 31:31, 0x0);

	clk_disable_unprepare(main_clk);

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}
static int svs_resume(struct platform_device *pdev)
{
	FUNC_ENTER(FUNC_LV_MODULE);

	clk_prepare_enable(main_clk);

	/* turn off system clock gating */
	svs_write_field(SVS_SVSCORESEL, 31:31, 0x1);

	svs_init02();
	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id mt_svs_of_match[] = {
	{ .compatible = SVS_COMPATIBLE_NODE, },
	{},
};
#endif
static struct platform_driver svs_driver = {
	.remove = NULL,
	.shutdown = NULL,
	.probe = svs_probe,
	.suspend = svs_suspend,
	.resume = svs_resume,
	.driver = {
		.name = "mt-svs",
#ifdef CONFIG_OF
		.of_match_table = mt_svs_of_match,
#endif
	},
};

/*
 * return current SVS stauts
 */
int mt_svs_status(int id)
{
	struct svs_det *det = id_to_svs_det(id);

	FUNC_ENTER(FUNC_LV_API);

	WARN_ON(!det);
	WARN_ON(!det->ops);
	WARN_ON(!det->ops->get_status);

	FUNC_EXIT(FUNC_LV_API);

	return det->ops->get_status(det);
}

#ifdef CONFIG_PROC_FS
/*
 *
 * PROCFS interface for debugging
 *
 */

/*
 * show current SVS stauts
 */
static int svs_debug_proc_show(struct seq_file *m, void *v)
{
	struct svs_det *det = (struct svs_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "SVS[%s] %s (svs_level = 0x%08X)\n",
		    det->name,
		    det->ops->get_status(det) ? "enabled" : "disable",
		    svs_level);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
/*
 * set SVS status by procfs interface
 */
static ssize_t svs_debug_proc_write(struct file *file,
					const char __user *buffer,
					size_t count, loff_t *pos)
{
	int ret;
	int enabled = 0;
	char *buf = (char *)__get_free_page(GFP_USER);
	struct svs_det *det = (struct svs_det *)PDE_DATA(file_inode(file));
	int rc;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	rc = kstrtoint(buf, 10, &enabled);
	if (rc < 0)
		ret = -EINVAL;
	else {
		ret = 0;
		if (enabled == 0)
			det->ops->disable(det, BY_PROCFS);
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

/*
 * show current SVS data
 */
static int svs_dump_proc_show(struct seq_file *m, void *v)
{
	struct svs_det *det;
	int *val = (int *)&svs_devinfo;
	int i;

	FUNC_ENTER(FUNC_LV_HELP);

	for (i = 0; i < sizeof(struct svs_devinfo) / sizeof(unsigned int); i++)
		seq_printf(m, "M_HW_RES%d\t= 0x%08X\n", i, val[i]);

	seq_printf(m, "SVS_PMIC_OFFSET = 0x%X\n", SVS_PMIC_OFFSET);

	for_each_det(det) {
		seq_printf(m, "SVS_VBOOT[%s]\t= 0x%08X\n",
			det->name, det->VBOOT);

		for (i = SVS_PHASE_INIT01; i < NR_SVS_PHASE; i++) {
			seq_printf(m, "Bank_number = %d\n", det->ctrl_id);

			if (i < SVS_PHASE_MON)
				seq_printf(m, "mode = init%d\n", i + 1);
			else
				seq_puts(m, "mode = mon mode\n");
#if 0
			seq_printf(m,
				"dcvalues=0x%08X, svs_freqpct30=0x%08X, svs_26c=0x%08X,",
					det->dcvalues[i],
					det->svs_freqpct30[i],
					det->svs_26c[i]);

			seq_printf(m, "svs_vop30=0x%08X,svs_svsen= 0x%08X\n",
				det->svs_vop30[i], det->svs_svsen[i]);
#endif

			{
				int j;

				for (j = 0; j < ARRAY_SIZE(reg_dump_addr_off);
					j++)
					seq_printf(m, "0x%lx = 0x%08x\n",
					  (unsigned long)SVS_BASEADDR +
						 reg_dump_addr_off[j],
						 det->reg_dump_data[j][i]);
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current voltage
 */
static int svs_cur_volt_proc_show(struct seq_file *m, void *v)
{
	struct svs_det *det = (struct svs_det *)m->private;
	u32 rdata = 0;

	FUNC_ENTER(FUNC_LV_HELP);

	rdata = det->ops->get_volt(det);

	if (rdata != 0)
		seq_printf(m, "%d\n", rdata);
	else
		seq_printf(m, "SVS[%s] read current voltage fail\n", det->name);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

/*
 * show current SVS status
 */
static int svs_status_proc_show(struct seq_file *m, void *v)
{
	struct svs_det *det = (struct svs_det *)m->private;
	int i, count;
	unsigned long rate;
	struct dev_pm_opp *opp;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m,
	"SVS_LOG: SVS [%s] (%d) - (%d, %d, %d, %d, %d, %d, %d, %d) - (%d, %d, %d, %d, %d, %d, %d, %d)\n",
		    det->name, det->ops->get_temp(det),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[0]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[1]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[2]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[3]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[4]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[5]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[6]),
		    SVS_PMIC_VAL_TO_VOLT(det->volt_tbl_pmic[7]),
		    det->freq_table_percent[0], det->freq_table_percent[1],
		    det->freq_table_percent[2], det->freq_table_percent[3],
		    det->freq_table_percent[4], det->freq_table_percent[5],
		    det->freq_table_percent[6], det->freq_table_percent[7]);

	seq_printf(m, "%s: svs_dvfs_max_freq_khz = %lu\n",
				det->name, det->dvfs_max_freq_khz);
	seq_printf(m, "%s: svs_dvfs_min_freq_khz = %lu\n",
				det->name, det->dvfs_min_freq_khz);
	seq_printf(m, "%s: svs_cpufreq_policy.max = %d\n",
				det->name, det->svs_cpufreq_policy.max);
	seq_printf(m, "%s: svs_cpufreq_policy.min = %d\n",
				det->name, det->svs_cpufreq_policy.min);

	/* Assume CPU DVFS OPP table is already initialized by cpufreq driver */
	rcu_read_lock();
	count = dev_pm_opp_get_opp_count(det->dev);
	if (count > NR_FREQ)
		seq_printf(m, "We only support OPP %d entries but got %d?\n",
				NR_FREQ, count);

	for (i = 0, rate = (unsigned long)-1;
			i < NR_FREQ && i < count; i++, rate--) {
		opp = dev_pm_opp_find_freq_floor(det->dev, &rate);
		if (IS_ERR(opp)) {
			seq_printf(m, "error opp entry!!, err = %ld\n",
					PTR_ERR(opp));
			break;
		}

		seq_printf(m, "freq[%d] = %ld, voltage = %ld\n",
				i, rate, dev_pm_opp_get_voltage(opp));
	}
	rcu_read_unlock();


	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}

static int svs_log_en_proc_show(struct seq_file *m, void *v)
{
	FUNC_ENTER(FUNC_LV_HELP);
	seq_printf(m, "%d\n", svs_log_en);
	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
static ssize_t svs_log_en_proc_write(struct file *file,
				     const char __user *buffer,
				     size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *)__get_free_page(GFP_USER);
	int rc;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	ret = -EINVAL;

	rc = kstrtoint(buf, 10, &svs_log_en);
	if (rc < 0) {
		pr_notice("bad argument!! Should be \"0\" or \"1\"\n");
		goto out;
	}

	ret = 0;

	switch (svs_log_en) {
	case 0:
		pr_notice("svs log disabled.\n");
		hrtimer_cancel(&svs_log_timer);
		break;

	case 1:
		pr_notice("svs log enabled.\n");
		hrtimer_start(&svs_log_timer,
				ns_to_ktime(LOG_INTERVAL),
				HRTIMER_MODE_REL);
		break;

	default:
		pr_err("bad argument!! Should be \"0\" or \"1\"\n");
		ret = -EINVAL;
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}


/*
 * show SVS offset
 */
static int svs_offset_proc_show(struct seq_file *m, void *v)
{
	struct svs_det *det = (struct svs_det *)m->private;

	FUNC_ENTER(FUNC_LV_HELP);

	seq_printf(m, "%d\n", det->volt_offset);

	FUNC_EXIT(FUNC_LV_HELP);

	return 0;
}
/*
 * set SVS offset by procfs
 */
static ssize_t svs_offset_proc_write(struct file *file,
				     const char __user *buffer,
				     size_t count, loff_t *pos)
{
	int ret;
	char *buf = (char *)__get_free_page(GFP_USER);
	int offset = 0;
	struct svs_det *det = (struct svs_det *)PDE_DATA(file_inode(file));
	int rc;

	FUNC_ENTER(FUNC_LV_HELP);

	if (!buf) {
		FUNC_EXIT(FUNC_LV_HELP);
		return -ENOMEM;
	}

	ret = -EINVAL;

	if (count >= PAGE_SIZE)
		goto out;

	ret = -EFAULT;

	if (copy_from_user(buf, buffer, count))
		goto out;

	buf[count] = '\0';

	rc = kstrtoint(buf, 10, &offset);
	if (rc < 0) {
		ret = -EINVAL;
		pr_notice("bad argument_1!! argument should be \"0\"\n");
	} else {
		ret = 0;
		det->volt_offset = offset;
		svs_set_svs_volt(det);
	}

out:
	free_page((unsigned long)buf);
	FUNC_EXIT(FUNC_LV_HELP);

	return (ret < 0) ? ret : count;
}

#define PROC_FOPS_RW(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
		.write          = name ## _proc_write,			\
	}

#define PROC_FOPS_RO(name)					\
	static int name ## _proc_open(struct inode *inode,	\
		struct file *file)				\
	{							\
		return single_open(file, name ## _proc_show,	\
			PDE_DATA(inode));			\
	}							\
	static const struct file_operations name ## _proc_fops = {	\
		.owner          = THIS_MODULE,				\
		.open           = name ## _proc_open,			\
		.read           = seq_read,				\
		.llseek         = seq_lseek,				\
		.release        = single_release,			\
	}

#define PROC_ENTRY(name)	{__stringify(name), &name ## _proc_fops}

PROC_FOPS_RW(svs_debug);
PROC_FOPS_RO(svs_dump);
PROC_FOPS_RW(svs_log_en);
PROC_FOPS_RO(svs_status);
PROC_FOPS_RO(svs_cur_volt);
PROC_FOPS_RW(svs_offset);

static int create_procfs(void)
{
	struct proc_dir_entry *svs_dir = NULL;
	struct proc_dir_entry *det_dir = NULL;
	int i;
	struct svs_det *det;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	struct pentry det_entries[] = {
		PROC_ENTRY(svs_debug),
		PROC_ENTRY(svs_status),
		PROC_ENTRY(svs_cur_volt),
		PROC_ENTRY(svs_offset),
	};

	struct pentry svs_entries[] = {
		PROC_ENTRY(svs_dump),
		PROC_ENTRY(svs_log_en),
	};

	FUNC_ENTER(FUNC_LV_HELP);

	svs_dir = proc_mkdir("svs", NULL);

	if (!svs_dir) {
		pr_err("[%s]: mkdir /proc/svs failed\n", __func__);
		FUNC_EXIT(FUNC_LV_HELP);
		return -1;
	}

	for (i = 0; i < ARRAY_SIZE(svs_entries); i++) {
		if (!proc_create
		    (svs_entries[i].name, 0664, svs_dir,
		     svs_entries[i].fops)) {
			pr_err("[%s]: create /proc/svs/%s failed\n", __func__,
				  svs_entries[i].name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -3;
		}
	}

	for_each_det(det) {
		det_dir = proc_mkdir(det->name, svs_dir);

		if (!det_dir) {
			pr_err("[%s]: mkdir /proc/svs/%s failed\n", __func__,
					det->name);
			FUNC_EXIT(FUNC_LV_HELP);
			return -2;
		}

		for (i = 0; i < ARRAY_SIZE(det_entries); i++) {
			if (!proc_create_data
			    (det_entries[i].name, 0664, det_dir,
			     det_entries[i].fops, det)) {
				pr_err("[%s]: create /proc/svs/%s/%s failed\n",
						__func__,
					  det->name, det_entries[i].name);
				FUNC_EXIT(FUNC_LV_HELP);
				return -3;
			}
		}
	}

	FUNC_EXIT(FUNC_LV_HELP);
	return 0;
}
#endif

/*
 * Module driver
 */
static int __init svs_init(void)
{
	int err = 0;
	struct device_node *node;

	node = of_find_compatible_node(NULL, NULL, SVS_COMPATIBLE_NODE);
	if (!node) {
		pr_err("error: cannot find node %s\n", SVS_COMPATIBLE_NODE);
		return -ENODEV;
	}

	/* Setup IO addresses */
	svs_base = of_iomap(node, 0);
	if (!svs_base) {
		pr_err("error: cannot iomap %s\n", SVS_COMPATIBLE_NODE);
		return -EFAULT;
	}

	/* get svs irq num */
	svs_irq_number = irq_of_parse_and_map(node, 0);
	if (!svs_irq_number) {
		pr_err("get irq num failed=0x%x\n", svs_irq_number);
		return 0;
	}

	err = platform_driver_register(&svs_driver);
	if (err) {
		pr_err("SVS driver callback register failed..\n");
		FUNC_EXIT(FUNC_LV_MODULE);
		return err;
	}

	FUNC_EXIT(FUNC_LV_MODULE);

	return 0;
}

static void __exit svs_exit(void)
{
	FUNC_ENTER(FUNC_LV_MODULE);
	pr_notice("SVS de-initialization\n");
	FUNC_EXIT(FUNC_LV_MODULE);
}
late_initcall(svs_init);
#endif

MODULE_DESCRIPTION("MediaTek SVS Driver v1.0");
MODULE_LICENSE("GPL");

#undef __MT_SVS_C__

/*
 * bd71827-power.c
 * @file ROHM BD71827 Charger driver
 *
 * Copyright 2016.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

//#define DEBUG
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/power_supply.h>
#include <linux/mfd/rohm-bd71827.h>
#include <linux/mfd/rohm-bd71828.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/power/ntx_charger_type.h>
#include <linux/bcd.h>
#include <linux/rtc.h>

#include "../../power/supply/ntx_charger_misc.h"

#include "../../../arch/arm/mach-mediatek/ntx_hwconfig.h"

#define MAX(X, Y) ((X) >= (Y) ? (X) : (Y))
#define uAMP_TO_mAMP(ma) ((ma) / 1000)
#define mAMP_TO_uAMP(ma) ((ma) * 1000)

/* BD71828 and BD71827 common defines */
#define BD7182x_MASK_VBAT_U		0x1f
#define BD7182x_MASK_VDCIN_U	0x0f
#define BD7182x_MASK_IBAT_U		0x3f
#define BD7182x_MASK_CURDIR_DISCHG	0x80
#define BD7182x_MASK_CC_CCNTD_HI	0x0FFF
#define BD7182x_MASK_CC_CCNTD		0x0FFFFFFF
#define BD7182x_MASK_CHG_STATE		0x7f
#define BD7182x_MASK_CC_FULL_CLR	0x10
#define BD7182x_MASK_BAT_TEMP		0x07
#define BD7182x_MASK_DCIN_DET		0x01
#define BD7182x_MASK_CONF_PON		0x01
#define BD7182x_MASK_BAT_STAT		0x3f
#define BD7182x_MASK_DCIN_STAT		0x07

#define BD7182x_MASK_CCNTRST		0x80
#define BD7182x_MASK_CCNTENB		0x40
#define BD7182x_MASK_CCCALIB		0x20
#define BD7182x_MASK_WDT_AUTO		0x40
#define BD7182x_MASK_VBAT_ALM_LIMIT_U	0x01
#define BD7182x_MASK_CHG_EN		0x01

#define BD7182x_DCIN_COLLAPSE_DEFAULT	0x36

/* Measured min and max value clear bits */
#define BD7182x_MASK_VSYS_MIN_AVG_CLR	0x10
#define BD7182x_MASK_VBAT_MIN_AVG_CLR	0x01

#define BD71828_MASK_CHG_IFST        			0x1f
#define BD71828_MASK_ILIM_DCIN        			0x3f

#define JITTER_DEFAULT			10000		/* 10 seconds */
#define JITTER_REPORT_CAP		10000		/* 10 seconds */
#define BATTERY_CAP_MAH_DEFAULT	1529
//#define MAX_VOLTAGE_DEFAULT_VALUE		ocv_table_default[0]
#define MIN_VOLTAGE_DEFAULT_VALUE		3400000
#define THR_VOLTAGE_DEFAULT_VALUE		4250000
#define MAX_VOLTAGE_DEFAULT_PT158098	4200000
#define MIN_VOLTAGE_DEFAULT_PT158098	3600000
#define THR_VOLTAGE_DEFAULT_PT158098	4150000
#define MAX_VOLTAGE_DEFAULT_EVE188595QH	4400000
#define MIN_VOLTAGE_DEFAULT_EVE188595QH	3520000
#define THR_VOLTAGE_DEFAULT_EVE188595QH	4350000

#define BATTERY_CAP_MAH_DEFAULT_28_PR284983N	1431//1464
#define BATTERY_CAP_MAH_DEFAULT_28_PT286090		2032
#define MAX_VOLTAGE_DEFAULT_28					4200000
#define MAX_VOLTAGE_DEFAULT_28_4v4				4400000
#define MAX_VOLTAGE_DEFAULT_78					4200000
#define MIN_VOLTAGE_DEFAULT_28_PR284983N		3600000
#define MIN_VOLTAGE_DEFAULT_28_PT286090			3600000
#define MIN_VOLTAGE_DEFAULT_28_EVE2275A7GH		3600000
#define MAX_CURRENT_DEFAULT		890000		/* uA */
#define AC_NAME					"bd71827_ac"
#define BAT_NAME				"bd71827_bat"
#define BATTERY_FULL_DEFAULT	100

#define BY_BAT_VOLT				0
#define BY_VBATLOAD_REG			1
#define INIT_COULOMB			BY_VBATLOAD_REG

#define CALIB_CURRENT_A2A3		0xCE9E

/*
 * VBAT Low voltage detection Threshold
 * 0x00D4*16mV = 212*0.016 = 3.392v
 */
#define VBAT_LOW_TH				0x00D4
//#define RS_30mOHM
//#ifdef RS_30mOHM
//#define A10s_mAh(s)		((s) * 1000 / (360 * 3))
//#define mAh_A10s(m)		((m) * (360 * 3) / 1000)
//#else
//#define A10s_mAh(s)		((s) * 1000 / 360)
//#define mAh_A10s(m)		((m) * 360 / 1000)
//#endif
#define A10s_mAh(s,factor)		((s) * 100000 / (360 * factor))
#define mAh_A10s(m,factor)		((m) * (360 * factor) / 100000)

#define THR_RELAX_CURRENT_DEFAULT	5		/* mA */
#define THR_RELAX_TIME_DEFAULT		(60 * 60)	/* sec. */

#define DGRD_CYC_CAP_DEFAULT		88	/* 1 micro Ah unit */
#define DGRD_CYC_CAP_DEFAULT_28_PR284983N		815	/* 1 micro Ah unit */
#define DGRD_CYC_CAP_DEFAULT_28_PT286090		400	/* 1 micro Ah unit */

#define DGRD_TEMP_H_DEFAULT			45	/* 1 degrees C unit */
#define DGRD_TEMP_M_DEFAULT			25	/* 1 degrees C unit */
#define DGRD_TEMP_L_DEFAULT			5	/* 1 degrees C unit */
#define DGRD_TEMP_VL_DEFAULT		0	/* 1 degrees C unit */

#define SOC_EST_MAX_NUM_DEFAULT		5
#define DGRD_TEMP_CAP_H_DEFAULT		(0)	/* 1 micro Ah unit */
#define DGRD_TEMP_CAP_M_DEFAULT		(0)	/* 1 micro Ah unit */
#define DGRD_TEMP_CAP_L_DEFAULT		(0)	/* 1 micro Ah unit */
#define SOC_EST_MAX_NUM_DEFAULT_28_PR284983N	5
#define SOC_EST_MAX_NUM_DEFAULT_28_PT286090		5
#define DGRD_TEMP_CAP_H_DEFAULT_28_PR284983N	(0)		/* 1 micro Ah unit */
#define DGRD_TEMP_CAP_M_DEFAULT_28_PR284983N	(0)		/* 1 micro Ah unit */
#define DGRD_TEMP_CAP_L_DEFAULT_28_PR284983N	(0)		/* 1 micro Ah unit */

#define PWRCTRL_NORMAL				0x22
#define PWRCTRL_RESET				0x23

static int MAX_VOLTAGE_DEFAULT = MAX_VOLTAGE_DEFAULT_PT158098;
static int MIN_VOLTAGE_DEFAULT = MIN_VOLTAGE_DEFAULT_PT158098;
static int THR_VOLTAGE_DEFAULT = THR_VOLTAGE_DEFAULT_PT158098;

#define BATTERY_CAP_MAH_PT158098	2344
#define DGRD_CYC_CAP_PT158098		338
#define SOC_90_MAX_CURR_PT158098	600000 // 600mA

#define BATTERY_CAP_MAH_EVE188595QH	1891
#define DGRD_CYC_CAP_EVE188595QH	98
#define SOC_90_MAX_CURR_EVE188595QH	400000 // 400mA

enum {
    BD_71879_CHARGER_TYPE_NONE = 0,
    BD_71879_CHARGER_TYPE_DCP,
    BD_71879_CHARGER_TYPE_SDP,
};

extern void mt_usb_connect(void);
extern void mt_usb_disconnect(void);
extern int dpidle_extern_ctrl(const char *buf, size_t count);
extern void max20342_VCCINTOnBAT(void);
int through_voltage;
/* TODO: Evaluate which members of "pwr" are really updated/read from separate
 * threads and actually do require memory barriers. Furthermore, evaluate
 * if the smp_rmb() is only required at start of update cycle / start of
 * request callbacks. This current 'call barrier for every access to "pwr"
 * is probably terrible for cache usage on the system...
 */

struct pwr_regs {
	u8 vbat_init;
	u8 vbat_init2;
	u8 vbat_init3;
	u8 vbat_avg;
	u8 ibat;
	u8 ibat_avg;
	u8 vsys_avg;
	u8 vbat_min_avg;
	u8 meas_clear;
	u8 vsys_min_avg;
	u8 btemp_vth;
	u8 chg_state;
	u8 coulomb3;
	u8 coulomb2;
	u8 coulomb1;
	u8 coulomb0;
	u8 coulomb_ctrl;
	u8 vbat_rex_avg;
	u8 rex_clear_reg;
	u8 rex_clear_mask;
	u8 coulomb_full3;
	u8 cc_full_clr;
	u8 coulomb_chg3;
	u8 bat_temp;
	u8 dcin_stat;
	u8 dcin_collapse_limit;
	u8 chg_set1;
	u8 chg_en;
	u8 vbat_alm_limit_u;
	u8 batcap_mon_limit_u;
	u8 conf;
	u8 bat_stat;
	u8 vdcin;
#ifdef PWRCTRL_HACK
	u8 pwrctrl;

#endif // PWRCTRL_HACK
};

struct soc_curr_table {
	u32 soc;
	u32 curr;
};

struct pwr_regs pwr_regs_bd71827 = {
	.vbat_init = BD71827_REG_VM_OCV_PRE_U,
	.vbat_init2 = BD71827_REG_VM_OCV_PST_U,
	.vbat_init3 = BD71827_REG_VM_OCV_PWRON_U,
	.vbat_avg = BD71827_REG_VM_SA_VBAT_U,
	.ibat = BD71827_REG_CC_CURCD_U,
	.ibat_avg = BD71827_REG_CC_SA_CURCD_U,
	.vsys_avg = BD71827_REG_VM_SA_VSYS_U,
	.vbat_min_avg = BD71827_REG_VM_SA_VBAT_MIN_U,
	.meas_clear = BD71827_REG_VM_SA_MINMAX_CLR,
	.vsys_min_avg = BD71827_REG_VM_SA_VSYS_MIN_U,
	.btemp_vth = BD71827_REG_VM_BTMP,
	.chg_state = BD71827_REG_CHG_STATE,
	.coulomb3 = BD71827_REG_CC_CCNTD_3,
	.coulomb2 = BD71827_REG_CC_CCNTD_2,
	.coulomb1 = BD71827_REG_CC_CCNTD_1,
	.coulomb0 = BD71827_REG_CC_CCNTD_0,
	.coulomb_ctrl = BD71827_REG_CC_CTRL,
	.vbat_rex_avg = BD71827_REG_REX_SA_VBAT_U,
	.rex_clear_reg = BD71827_REG_REX_CTRL_1,
	.rex_clear_mask = BD71827_REX_CLR_MASK,
	.coulomb_full3 = BD71827_REG_FULL_CCNTD_3,
	.cc_full_clr = BD71827_REG_FULL_CTRL,
	.coulomb_chg3 = BD71827_REG_CCNTD_CHG_3,
	.bat_temp = BD71827_REG_BAT_TEMP,
	.dcin_stat = BD71827_REG_DCIN_STAT,
	.dcin_collapse_limit = BD71827_REG_DCIN_CLPS,
	.chg_set1 = BD71827_REG_CHG_SET1,
	.chg_en = BD71827_REG_CHG_SET1,
	.vbat_alm_limit_u = BD71827_REG_ALM_VBAT_TH_U,
	.batcap_mon_limit_u = BD71827_REG_CC_BATCAP1_TH_U,
	.conf = BD71827_REG_CONF,
	.bat_stat = BD71827_REG_BAT_STAT,
	.vdcin = BD71827_REG_VM_DCIN_U,
#ifdef PWRCTRL_HACK
	.pwrctrl = BD71827_REG_PWRCTRL,
	.hibernate_mask = 0x1,
#endif
};

struct pwr_regs pwr_regs_bd71828 = {
	.vbat_init = BD71828_REG_VBAT_INITIAL1_U,
	.vbat_init2 = BD71828_REG_VBAT_INITIAL2_U,
	.vbat_init3 = BD71828_REG_OCV_PWRON_U,
	.vbat_avg = BD71828_REG_VBAT_U,
	.ibat = BD71828_REG_IBAT_U,
	.ibat_avg = BD71828_REG_IBAT_AVG_U,
	.vsys_avg = BD71828_REG_VSYS_AVG_U,
	.vbat_min_avg = BD71828_REG_VBAT_MIN_AVG_U,
	.meas_clear = BD71828_REG_MEAS_CLEAR,
	.vsys_min_avg = BD71828_REG_VSYS_MIN_AVG_U,
	.btemp_vth = BD71828_REG_VM_BTMP_U,
	.chg_state = BD71828_REG_CHG_STATE,
	.coulomb3 = BD71828_REG_CC_CNT3,
	.coulomb2 = BD71828_REG_CC_CNT2,
	.coulomb1 = BD71828_REG_CC_CNT1,
	.coulomb0 = BD71828_REG_CC_CNT0,
	.coulomb_ctrl = BD71828_REG_COULOMB_CTRL,
	.vbat_rex_avg = BD71828_REG_VBAT_REX_AVG_U,
	.rex_clear_reg = BD71828_REG_COULOMB_CTRL2,
	.rex_clear_mask = BD71828_MASK_REX_CC_CLR,
	.coulomb_full3 = BD71828_REG_CC_CNT_FULL3,
	.cc_full_clr = BD71828_REG_COULOMB_CTRL2,
	.coulomb_chg3 = BD71828_REG_CC_CNT_CHG3,
	.bat_temp = BD71828_REG_BAT_TEMP,
	.dcin_stat = BD71828_REG_DCIN_STAT,
	.dcin_collapse_limit = BD71828_REG_DCIN_CLPS,
	.chg_set1 = BD71828_REG_CHG_SET1,
	.chg_en   = BD71828_REG_CHG_EN,
	.vbat_alm_limit_u = BD71828_REG_ALM_VBAT_LIMIT_U,
	.batcap_mon_limit_u = BD71828_REG_BATCAP_MON_LIMIT_U,
	.conf = BD71828_REG_CONF,
	.bat_stat = BD71828_REG_BAT_STAT,
	.vdcin = BD71828_REG_VDCIN_U,
#ifdef PWRCTRL_HACK
	.pwrctrl = BD71828_REG_PS_CTRL_1,
	.hibernate_mask = 0x2,
#endif
};

static long ciritical_low_count = 0;

/*static int ocv_table_default[23] = {
	4350000,
	4325945,
	4255935,
	4197476,
	4142843,
	4090615,
	4047113,
	3987352,
	3957835,
	3920815,
	3879834,
	3827010,
	3807239,
	3791379,
	3779925,
	3775038,
	3773530,
	3756695,
	3734099,
	3704867,
	3635377,
	3512942,
	3019825
};*/	/* unit 1 micro V */

static int soc_table_default[23] = {
	1000, 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300, 250, 200, 150, 100, 50, 0, -50
	/* unit 0.1% */
};
/*
static int vdr_table_h_default[23] = {
	100,
	100,
	102,
	104,
	105,
	108,
	111,
	115,
	122,
	138,
	158,
	96,
	108,
	112,
	117,
	123,
	137,
	109,
	131,
	150,
	172,
	136,
	218
};

static int vdr_table_m_default[23] = {
	100,
	100,
	100,
	100,
	102,
	104,
	114,
	110,
	127,
	141,
	139,
	96,
	102,
	106,
	109,
	113,
	130,
	134,
	149,
	188,
	204,
	126,
	271
};

static int vdr_table_l_default[23] = {
	100,
	100,
	98,
	96,
	96,
	96,
	105,
	94,
	108,
	105,
	95,
	89,
	90,
	92,
	99,
	112,
	129,
	143,
	155,
	162,
	156,
	119,
	326
};

static int vdr_table_vl_default[23] = {
	100,
	100,
	98,
	96,
	95,
	97,
	101,
	92,
	100,
	97,
	91,
	89,
	90,
	93,
	103,
	115,
	128,
	139,
	148,
	148,
	156,
	246,
	336
};*/

static int ocv_table_PT158098[] = {
		4200000,
		4186060,
		4134359,
		4090624,
		4053244,
		4005836,
		3976006,
		3948288,
		3921468,
		3892550,
		3854286,
		3827328,
		3809671,
		3796680,
		3786781,
		3779702,
		3771635,
		3755782,
		3736253,
		3707997,
		3692077,
		3655858,
		3274324
};	/* unit 1 micro V */

static int vdr_table_h_PT158098[] = {
		100,
		100,
		103,
		107,
		110,
		112,
		116,
		118,
		120,
		124,
		110,
		100,
		100,
		102,
		107,
		107,
		107,
		112,
		113,
		110,
		118,
		140,
		471
};

static int vdr_table_m_PT158098[] = {
		100,
		100,
		103,
		106,
		113,
		114,
		120,
		123,
		128,
		125,
		108,
		104,
		100,
		102,
		107,
		109,
		112,
		113,
		112,
		109,
		114,
		151,
		497
};

static int vdr_table_l_PT158098[] = {
		100,
		100,
		102,
		107,
		115,
		120,
		122,
		122,
		123,
		124,
		119,
		112,
		112,
		115,
		120,
		126,
		132,
		134,
		125,
		129,
		145,
		207,
		515
};

static int vdr_table_vl_PT158098[] = {
		100,
		100,
		108,
		115,
		131,
		134,
		137,
		139,
		135,
		133,
		124,
		121,
		117,
		116,
		120,
		125,
		131,
		134,
		134,
		133,
		161,
		245,
		484
};

struct soc_curr_table calc_soc_table_PT158098[6] = {
	{95, 250},
	{96, 215},
	{97, 180},
	{98, 145},
	{99, 110},
	{100, 72},
};

static int ocv_table_EVE188595QH[] = {
		4400000,
		4385330,
		4335359,
		4282533,
		4227757,
		4173553,
		4121564,
		4072749,
		4027620,
		3980255,
		3930099,
		3888426,
		3860709,
		3837971,
		3818966,
		3802755,
		3765940,
		3760014,
		3736093,
		3706274,
		3693249,
		3604151,
		3075562
};	/* unit 1 micro V */

static int vdr_table_h_EVE188595QH[] = {
		100,
		100,
		101,
		102,
		104,
		106,
		109,
		113,
		120,
		121,
		112,
		103,
		105,
		108,
		113,
		118,
		94,
		106,
		105,
		103,
		113,
		113,
		333
};

static int vdr_table_m_EVE188595QH[] = {
		100,
		100,
		100,
		101,
		101,
		103,
		106,
		112,
		110,
		105,
		100,
		95,
		93,
		90,
		90,
		95,
		90,
		95,
		101,
		101,
		114,
		115,
		285
};

static int vdr_table_l_EVE188595QH[] = {
		100,
		100,
		89,
		89,
		88,
		88,
		88,
		90,
		91,
		91,
		87,
		85,
		85,
		88,
		93,
		96,
		95,
		112,
		119,
		129,
		167,
		269,
		563
};

static int vdr_table_vl_EVE188595QH[] = {
		100,
		100,
		100,
		100,
		99,
		99,
		94,
		94,
		93,
		90,
		86,
		84,
		85,
		85,
		93,
		99,
		100,
		117,
		142,
		188,
		292,
		452,
		557
};

struct soc_curr_table calc_soc_table_EVE188595QH[6] = {
	{95, 235},
	{96, 202},
	{97, 169},
	{98, 136},
	{99, 103},
	{100, 70},
};


static int ocv_table_28_PR284983N[23] = {
	4200000, 4166666, 4116873, 4074990, 4035777, 4000425, 3969471, 3941807, 3916760, 3892890, 3863633, 3827848, 3806835, 3794639, 3785042, 3778677, 3773142, 3762234, 3745469, 3722882, 3695570, 3673969, 3598702
};

static int vdr_table_h_28_PR284983N[23] = {
	100, 100,  82,  84,  86,  88,  95, 100, 100, 105,  99,  93,  92,  92,  90,  95, 105, 114, 110, 111, 113, 120, 161
};

static int vdr_table_m_28_PR284983N[23] = {
	100, 100, 108, 116, 124, 128, 133, 141, 142, 147, 142, 116, 109, 111, 113, 119, 127, 128, 125, 126, 131, 136, 181
};

static int vdr_table_l_28_PR284983N[23] = {
	100,  85, 105, 111, 119, 122, 130, 128, 131, 132, 128, 117, 117, 119, 124, 133, 140, 151, 161, 173, 192, 207, 234
};

static int vdr_table_vl_28_PR284983N[23] = {
	100, 100, 105, 106, 108, 108, 110, 110, 112, 116, 115, 108, 105, 106, 108, 116, 121, 126, 133, 143, 153, 164, 177
};
int use_load_bat_params;

static int battery_cap_mah;
static int battery_cap;

static int charge_battery_cap;
static int discharge_battery_cap;

int dgrd_cyc_cap;

int soc_est_max_num;

int dgrd_temp_cap_h;
int dgrd_temp_cap_m;
int dgrd_temp_cap_l;

static unsigned int battery_cycle;

int ocv_table[23];
int soc_table[23];
int vdr_table_h[23];
int vdr_table_m[23];
int vdr_table_l[23];
int vdr_table_vl[23];

struct bd7182x_soc_data {
	int    vbus_status;		/* < last vbus status */
	int    charge_status;		/* < last charge status */
	int    bat_status;		/* < last bat status */

	int	bat_online;		/* < battery connect */
	int	charger_online;		/* < charger connect */
	int	vcell;			/* < battery voltage */
	int	vsys;			/* < system voltage */
	int	vcell_min;		/* < minimum battery voltage */
	int	vsys_min;		/* < minimum system voltage */
	int	rpt_status;		/* < battery status report */
	int	prev_rpt_status;	/* < previous battery status report */
	int	bat_health;		/* < battery health */
	int	designed_cap;		/* < battery designed capacity */
	int	full_cap;		/* < battery capacity */
	int	curr;			/* < battery current from ADC */
	int	curr_avg;		/* < average battery current */
	int	temp;			/* < battery tempature */
	u32	coulomb_cnt;		/* < Coulomb Counter */
	int	state_machine;		/* < initial-procedure state machine */
	u32	soc_norm;		/* < State Of Charge using full
					 * capacity without by load
					 */
	u32	soc;			/* < State Of Charge using full
					 * capacity with by load
					 */
	u32	clamp_soc;		/* < Clamped State Of Charge using
					 * full capacity with by load
					 */
	int	relax_time;		/* < Relax Time */
	u32	cycle;			/* < Charging and Discharging cycle
					 * number
					 */
};

/* @brief power device */
struct bd71827_power {
	struct rohm_regmap_dev *mfd;	/* < parent for access register */
	struct power_supply *ac;	/* < alternating current power */
	struct power_supply *bat;	/* < battery power */
	int gauge_delay;		/* < Schedule to call gauge algorithm */
	struct bd7182x_soc_data d_r;	/* < SOC algorithm data for reporting */
	struct bd7182x_soc_data d_w;	/* < internal SOC algorithm data */
	spinlock_t dlock;
	struct delayed_work bd_work;	/* < delayed work for timed work */
	struct delayed_work usb_work;	/* < delayed work for timed work */

	struct pwr_regs *regs;
	/* Reg val to uA */
	int curr_factor;
	int (*get_temp)(struct bd71827_power *pwr, int *temp);

	int charger_type;
	int pre_charger_type;
	u32 boot_jiffies;
	u32 suspend_resume;
	u32 resume_jiffies;
	u32 change_charger;
	time64_t update_seconds;
	int schedule_work;
	
	int charge_curr_factor;
	int discharge_curr_factor;
	
	u32 calc_soc_jiffies;
};

#define CALIB_NORM			0
#define CALIB_START			1
#define CALIB_GO			2

enum {
	STAT_POWER_ON,
	STAT_INITIALIZED,
};

extern volatile NTX_HWCONFIG *gptHWCFG;

static int cc_calc_curve_en = 0;
static int cc_calc_skip = 0;

struct soc_curr_table calc_soc_table[6] = {
	{95, 250},
	{96, 215},
	{97, 180},
	{98, 145},
	{99, 110},
	{100, 72},
};
static u32 curr_90_max = 600000;
static u32 curr_90_avg = 70000;
static u32 curr_90_min = 250000;


void bd71828_charger_detect(void);

int bd71828_get_charger_detect(void);

extern void usb_plug_handler(void *dummy);

extern int check_usb_state_by_ep(void);

static unsigned int bd71827_calc_soc_org(u32 cc, int designed_cap);

static struct platform_device *gPwr_dev;

static int suspend_status = 0;

static void update_soc_data(struct bd71827_power *pwr);

static int setCurrentLimit(struct bd71827_power *pwr, unsigned int currentLimit)
{
	int val, ret;

	if(currentLimit < 50)
	{
		currentLimit = 50;
	}
	dev_info(pwr->mfd->dev, "%s() set current limit %d mA\n", __func__, currentLimit);

	val = currentLimit/50 - 1;

	/* set current limit*/
	val |= 0xC0;
	ret = regmap_write(pwr->mfd->regmap, BD71828_REG_DCIN_SET, val);
	return ret;
}

static int setChargeCurrent(struct bd71827_power *pwr, unsigned int chargeCurrent)
{
	int val, ret;

	if(chargeCurrent < 100)
	{
		chargeCurrent = 100;
	}
	dev_info(pwr->mfd->dev, "%s() set charge current %d mA\n", __func__, chargeCurrent);

	val = chargeCurrent/25;

	/* set charge current*/
	val |= 0x40;
	ret = regmap_write(pwr->mfd->regmap, BD71828_REG_CHG_IFST, val);
	return ret;
}

/*static int getCurrentLimit(struct bd71827_power *pwr)
{
	int ret, val;

	ret = regmap_read(pwr->mfd->regmap, BD71828_REG_ILIM_STAT, &val);
	if (ret) {
		dev_err(pwr->mfd->dev,"%s(): error (%d) on reading current limit\n",__func__,ret);
		return ret;
	}
	val = 50 * (1 + val);
	if(val > 2000) {
		val = 2000;
	}
	return val;
}

static int getChargeCurrent(struct bd71827_power *pwr)
{
	int ret, val;

	ret = regmap_read(pwr->mfd->regmap, BD71828_REG_CHG_IFST, &val);
	if (ret) {
		dev_err(pwr->mfd->dev,"%s(): error (%d) on reading charge current\n",__func__,ret);
		return ret;
	}
	val = 25 * (val & BD71828_MASK_ILIM_DCIN);
	if(val < 100) {
		val = 100;
	}
	return val;
}*/

static int resetBatteryCharger(struct bd71827_power *pwr)
{
	int ret, val;

	val = 0x02;
	ret = regmap_write(pwr->mfd->regmap, BD71828_REG_CHG_INIT, val);
	mdelay(100);
	val = 0x00;
	ret += regmap_write(pwr->mfd->regmap, BD71828_REG_CHG_INIT, val);

	return ret;
}

static int setChargerParam(struct bd71827_power *pwr, unsigned currentLimit, unsigned chargeCurrent)
{
	/* set current limit*/
	setCurrentLimit(pwr, currentLimit);

	/* set charge current*/
	setChargeCurrent(pwr, chargeCurrent);

	/* Battery charger reset */
	resetBatteryCharger(pwr);

	return 0;
}

void battery_charge_current(int mode)
{
	static int flag=0;
	struct bd71827_power *pwr;

	if(!gPwr_dev)
		return;

	pwr = platform_get_drvdata(gPwr_dev);
	if(!flag) {
		dev_info(pwr->mfd->dev, "\n>>>>> Call dpidle_ctrl\n");
		dpidle_extern_ctrl("spm_f26m_req 0x1", strlen("spm_f26m_req 0x1"));
		flag = 1;
	}
	dev_info(pwr->mfd->dev, "Change battery charge current\n");
	switch(pwr->charger_type) {
		case BD_71879_CHARGER_TYPE_DCP:
			if( (21==gptHWCFG->m_val.bBattery) || (26==gptHWCFG->m_val.bBattery)) {
				// 1200mAx2 , 2050mA .
				setChargerParam(pwr, 2000, 1500);
			}
			else {
				setChargerParam(pwr, 2000, 1300);
			}
			dev_dbg(pwr->mfd->dev, "set cc_calc_curve_en to 1\n");
            cc_calc_curve_en = 1;
            cc_calc_skip = 0;
			break;
		case BD_71879_CHARGER_TYPE_SDP:
			setChargerParam(pwr, 500, 500);
			dev_dbg(pwr->mfd->dev, "set cc_calc_curve_en to 0\n");
            cc_calc_curve_en = 0;
            cc_calc_skip = 0;
			break;
		case BD_71879_CHARGER_TYPE_NONE:
		default:
			setChargerParam(pwr, 0, 0);
			dev_dbg(pwr->mfd->dev, "set cc_calc_curve_en to 0\n");
            cc_calc_curve_en = 0;
            cc_calc_skip = 0;
			break;
	}
}
EXPORT_SYMBOL_GPL(battery_charge_current);

void config_battery_charger(int mode)
{
#if 0
	struct bd71827_power *pwr;

	if(!gPwr_dev)
		return;

	pwr = platform_get_drvdata(gPwr_dev);
	dev_info(pwr->mfd->dev, "\nConfig battery charger type\n");
	switch(mode) {
		case DCP_CHARGER:
			pwr->charger_type = BD_71879_CHARGER_TYPE_DCP;
			break;
		case SDP_CHARGER:
		case CDP_CHARGER:
			pwr->charger_type = BD_71879_CHARGER_TYPE_SDP;
			break;
		case NO_CHARGER_PLUGGED:
		default:
			pwr->charger_type = BD_71879_CHARGER_TYPE_NONE;
			battery_charge_current(pwr->charger_type);
			usb_plug_handler(NULL);
			break;
	}
	//cancel_delayed_work_sync(&pwr->bd_work);
	//pwr->change_charger = 1;
	//schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(1000));
	cancel_delayed_work_sync(&pwr->usb_work);
	schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(500));
#endif
}
EXPORT_SYMBOL_GPL(config_battery_charger);

static int bd7182x_write16(struct bd71827_power *pwr, int reg, u16 val)
{
	val = cpu_to_be16(val);

	return regmap_bulk_write(pwr->mfd->regmap, reg, &val, sizeof(val));
}

static int bd7182x_read16_himask(struct bd71827_power *pwr, int reg, int himask,
				 u16 *val)
{
	struct regmap *regmap = pwr->mfd->regmap;
	int ret;
	u8 *tmp = (u8 *)val;

	ret = regmap_bulk_read(regmap, reg, val, sizeof(*val));
	if (!ret) {
		*tmp &= himask;
		*val = be16_to_cpu(*val);
	}
	return ret;
}

#if INIT_COULOMB == BY_VBATLOAD_REG
#define INITIAL_OCV_REGS 3
/* @brief get initial battery voltage and current
 * @param pwr power device
 * @return 0
 */
static int bd71827_get_init_bat_stat(struct bd71827_power *pwr,
				     int *ocv)
{
	int ret;
	int i;
	u8 regs[INITIAL_OCV_REGS] = {
		pwr->regs->vbat_init,
		pwr->regs->vbat_init2,
		pwr->regs->vbat_init3
	};
	uint16_t vals[INITIAL_OCV_REGS];

	*ocv = 0;
	for (i = 0; i < INITIAL_OCV_REGS; i++) {
		ret = bd7182x_read16_himask(pwr, regs[i], BD7182x_MASK_VBAT_U,
					    &vals[i]);
		if (ret) {
			dev_err(pwr->mfd->dev,
				"Failed to read initial battery voltage\n");
			return ret;
		}
		*ocv = MAX(vals[i], *ocv);

		dev_dbg(pwr->mfd->dev, "VM_OCV_%d = %d\n", i,
			((int)vals[i]) * 1000);
	}

	*ocv *= 1000;
	return ret;
}
#endif

/* @brief get battery average voltage
 * @param pwr power device
 * @param vcell pointer to return back voltage in unit uV.
 * @return 0
 */
static int bd71827_get_vbat(struct bd71827_power *pwr, int *vcell)
{
	u16 tmp_vcell;
	int ret,count,tempValue=0,totalValue=0;
	static int vbat_record[10] = {0}, index=0;

	ret = bd7182x_read16_himask(pwr, pwr->regs->vbat_avg,
				    BD7182x_MASK_VBAT_U, &tmp_vcell);
	if (ret)
		dev_err(pwr->mfd->dev,
			"Failed to read battery average voltage\n");
	else
		/**vcell*/tempValue = ((int)tmp_vcell);// * 1000;

	if(pwr->suspend_resume) {
		for(count=0;count<10;count++)
			vbat_record[count] = 0;
		index = 0;
	}

	vbat_record[index++] = tempValue;
	if(index>=10)
		index = 0;
	for(count=0;count<10;count++) {
		if(vbat_record[count] == 0)
			break;
		totalValue += vbat_record[count];
	}

	*vcell =  (totalValue * 1000)/count;

	return ret;
}

#if INIT_COULOMB == BY_BAT_VOLT
/* @brief get battery average voltage and current
 * @param pwr power device
 * @param vcell pointer to return back voltage in unit uV.
 * @param curr  pointer to return back current in unit uA.
 * @return 0
 */
static int bd71827_get_vbat_curr(struct bd71827_power *pwr,
				int *vcell, int *curr)
{
	int ret;

	ret = bd71827_get_vbat(pwr, vcell);
	*curr = 0;

	return ret;
}
#endif

/* @brief get battery current and battery average current from DS-ADC
 * @param pwr power device
 * @param current in unit uA
 * @param average current in unit uA
 * @return 0
 */
static int bd71827_get_current_ds_adc(struct bd71827_power *pwr,
					int *curr, int *curr_avg)
{
	u16 tmp_curr;
	char *tmp = (char *)&tmp_curr;
	int dir = 1;
	int regs[] = { pwr->regs->ibat, pwr->regs->ibat_avg };
	int *vals[] = { curr, curr_avg };
	int ret, i;

	for (dir = 1, i = 0; i < ARRAY_SIZE(regs); i++) {
		ret = regmap_bulk_read(pwr->mfd->regmap, regs[i], &tmp_curr,
				       sizeof(tmp_curr));
		if (ret)
			break;

		if (*tmp & BD7182x_MASK_CURDIR_DISCHG)
			dir = -1;

		*tmp &= BD7182x_MASK_IBAT_U;
		tmp_curr = be16_to_cpu(tmp_curr);

		*vals[i] = dir * ((int)tmp_curr) * pwr->curr_factor;
	}

	return ret;
}

/* @brief get system average voltage
 * @param pwr power device
 * @param vcell pointer to return back voltage in unit uV.
 * @return 0
 */
static int bd71827_get_vsys(struct bd71827_power *pwr, int *vsys)
{
	u16 tmp_vsys;
	int ret,count,tempValue=0,totalValue=0;
	static int vsys_record[10] = {0}, index=0;

	ret = bd7182x_read16_himask(pwr, pwr->regs->vsys_avg,
				    BD7182x_MASK_VBAT_U, &tmp_vsys);
	if (ret)
		dev_err(pwr->mfd->dev,
			"Failed to read system average voltage\n");
	else
		/**vsys*/tempValue = ((int)tmp_vsys);// * 1000;

	if(pwr->suspend_resume) {
		for(count=0;count<10;count++)
			vsys_record[count] = 0;
		index = 0;
	}

	vsys_record[index++] = tempValue;
	if(index>=10)
		index = 0;
	for(count=0;count<10;count++) {
		if(vsys_record[count] == 0)
			break;
		totalValue += vsys_record[count];
	}

	*vsys =  (totalValue * 1000)/count;

	return ret;
}

/* @brief get battery minimum average voltage
 * @param pwr power device
 * @param vcell pointer to return back voltage in unit uV.
 * @return 0
 */
static int bd71827_get_vbat_min(struct bd71827_power *pwr, int *vcell)
{
	u16 tmp_vcell;
	int ret,count,tempValue=0,totalValue=0;
	static int vbat_min_record[10] = {0}, index=0;

	ret = bd7182x_read16_himask(pwr, pwr->regs->vbat_min_avg,
				    BD7182x_MASK_VBAT_U, &tmp_vcell);
	if (ret)
		dev_err(pwr->mfd->dev,
			"Failed to read battery min average voltage\n");
	else
		ret = regmap_update_bits(pwr->mfd->regmap,
					 pwr->regs->meas_clear,
					 BD7182x_MASK_VBAT_MIN_AVG_CLR,
					 BD7182x_MASK_VBAT_MIN_AVG_CLR);

	/**vcell*/tempValue = ((int)tmp_vcell);// * 1000;

	if(pwr->suspend_resume) {
		for(count=0;count<10;count++)
			vbat_min_record[count] = 0;
		index = 0;
	}

	vbat_min_record[index++] = tempValue;
	if(index>=10)
		index = 0;
	for(count=0;count<10;count++) {
		if(vbat_min_record[count] == 0)
			break;
		totalValue += vbat_min_record[count];
	}

	*vcell =  (totalValue * 1000)/count;

	return ret;
}

/* @brief get system minimum average voltage
 * @param pwr power device
 * @param vcell pointer to return back voltage in unit uV.
 * @return 0
 */
static int bd71827_get_vsys_min(struct bd71827_power *pwr, int *vcell)
{
	u16 tmp_vcell;
	int ret,count,tempValue=0,totalValue=0;
	static int vsys_min_record[10] = {0}, index=0;

	ret = bd7182x_read16_himask(pwr, pwr->regs->vsys_min_avg,
				    BD7182x_MASK_VBAT_U, &tmp_vcell);
	if (ret)
		dev_err(pwr->mfd->dev,
			"Failed to read system min average voltage\n");
	else
		ret = regmap_update_bits(pwr->mfd->regmap,
					 pwr->regs->meas_clear,
					 BD7182x_MASK_VSYS_MIN_AVG_CLR,
					 BD7182x_MASK_VSYS_MIN_AVG_CLR);

	/**vcell*/tempValue = ((int)tmp_vcell);// * 1000;

	if(pwr->suspend_resume) {
		for(count=0;count<10;count++)
			vsys_min_record[count] = 0;
		index = 0;
	}

	vsys_min_record[index++] = tempValue;
	if(index>=10)
		index = 0;
	for(count=0;count<10;count++) {
		if(vsys_min_record[count] == 0)
			break;
		totalValue += vsys_min_record[count];
	}

	*vcell =  (totalValue * 1000)/count;

	return ret;
}

/* @brief get battery capacity
 * @param ocv open circuit voltage
 * @return capcity in unit 0.1 percent
 */
static int bd71827_voltage_to_capacity(int ocv)
{
	int i = 0;
	int soc;

	if (ocv > ocv_table[0]) {
		soc = soc_table[0];
	} else {
		for (i = 0; soc_table[i] != -50; i++) {
			if ((ocv <= ocv_table[i]) && (ocv > ocv_table[i + 1])) {
				soc = (soc_table[i] - soc_table[i + 1]) *
				      (ocv - ocv_table[i + 1]) /
				      (ocv_table[i] - ocv_table[i + 1]);
				soc += soc_table[i + 1];
				break;
			}
		}
		if (soc_table[i] == -50)
			soc = soc_table[i];
	}
	return soc;
}

/* @brief get battery temperature
 * @param pwr power device
 * @return temperature in unit deg.Celsius
 */
static int bd71827_get_temp(struct bd71827_power *pwr, int *temp)
{
	struct regmap *regmap = pwr->mfd->regmap;
	int ret;
	int t;

	ret = regmap_read(regmap, pwr->regs->btemp_vth, &t);
	t = 200 - t;

	if (ret || t > 200) {
		dev_err(pwr->mfd->dev, "Failed to read battery temperature\n");
		*temp = 200;
	} else {
		*temp = t;
	}

	return ret;
}

static int bd71828_get_temp(struct bd71827_power *pwr, int *temp)
{
	u16 t;
	int ret;
	int tmp = 200 * 10000;

	ret = bd7182x_read16_himask(pwr, pwr->regs->btemp_vth,
				    BD71828_MASK_VM_BTMP_U, &t);
	if (ret || t > 3200)
		dev_err(pwr->mfd->dev,
			"Failed to read system min average voltage\n");

	tmp -= 625ULL * (unsigned int)t;
	*temp = tmp / 10000;

	return ret;
}

static int bd71827_reset_coulomb_count(struct bd71827_power *pwr,
				       struct bd7182x_soc_data *wd);

/* @brief get battery charge status
 * @param pwr power device
 * @return 0 at success or negative error code.
 */
static int bd71827_charge_status(struct bd71827_power *pwr,
				 struct bd7182x_soc_data *wd, int mode)
{
	unsigned int state;
	int ret;

	wd->prev_rpt_status = wd->rpt_status;

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->chg_state, &state);
	if (ret) {
		dev_err(pwr->mfd->dev, "charger status reading failed (%d)\n",
			ret);
		return ret;
	}

	state &= BD7182x_MASK_CHG_STATE;

	dev_dbg(pwr->mfd->dev, "%s(): CHG_STATE %d\n", __func__, state);

	switch (state) {
	case 0x00:
		wd->rpt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		wd->bat_health = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x01:
	case 0x02:
	case 0x03:
	case 0x0E:
		wd->rpt_status = POWER_SUPPLY_STATUS_CHARGING;
		wd->bat_health = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x0F:
		wd->rpt_status = POWER_SUPPLY_STATUS_FULL;
		wd->bat_health = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
	case 0x14:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
		wd->rpt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		wd->bat_health = POWER_SUPPLY_HEALTH_OVERHEAT;
		break;
	case 0x30:
	case 0x31:
	case 0x32:
	case 0x40:
		wd->rpt_status = POWER_SUPPLY_STATUS_DISCHARGING;
		wd->bat_health = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case 0x7f:
	default:
		wd->rpt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		wd->bat_health = POWER_SUPPLY_HEALTH_DEAD;
		break;
	}

	if(mode)
		ret = bd71827_reset_coulomb_count(pwr, wd);

	return ret;
}

#if INIT_COULOMB == BY_BAT_VOLT
static int bd71827_calib_voltage(struct bd71827_power *pwr, int *ocv)
{
	int r, curr, volt, ret;

	bd71827_get_vbat_curr(pwr, &volt, &curr);

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->chg_state, &r);
	if (ret) {
		dev_err(pwr->mfd->dev, "Charger state reading failed (%d)\n",
			ret);
	} else if (curr > 0) {
		/* voltage increment caused by battery inner resistor */
		if (r == 3)
			volt -= 100 * 1000;
		else if (r == 2)
			volt -= 50 * 1000;
	}
	*ocv = volt;

	return 0;
}
#endif
static int __write_cc(struct bd71827_power *pwr, u16 bcap,
		      unsigned int reg, u32 *new)
{
	int ret;
	u32 tmp;
	u16 *swap_hi = (u16 *)&tmp;
	u16 *swap_lo = swap_hi + 1;

	*swap_hi = cpu_to_be16(bcap & BD7182x_MASK_CC_CCNTD_HI);
	*swap_lo = 0;

	ret = regmap_bulk_write(pwr->mfd->regmap, reg, &tmp, sizeof(tmp));
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to write coulomb counter\n");
		return ret;
	}
	if (new)
		*new = cpu_to_be32(tmp);

	return ret;
}

static int __write_cc_32(struct bd71827_power* pwr, u16 bcap_hi, u16 bcap_lo,
		      unsigned int reg, u32 *new)
{
	int ret;
	u32 tmp;
	u16 *swap_hi = (u16 *)&tmp;
	u16 *swap_lo = swap_hi + 1;

	*swap_hi = cpu_to_be16(bcap_hi & BD7182x_MASK_CC_CCNTD_HI);
	*swap_lo = cpu_to_be16(bcap_lo);

	ret = regmap_bulk_write(pwr->mfd->regmap, reg, &tmp, sizeof(tmp));
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to write coulomb counter\n");
		return ret;
	}
	if (new)
		*new = cpu_to_be32(tmp);

	return ret;
}

static int write_cc(struct bd71827_power *pwr, u16 bcap)
{
	int ret;
	u32 new;

	ret = __write_cc(pwr, bcap, pwr->regs->coulomb3, &new);
	if (!ret)
		pwr->d_w.coulomb_cnt = new;

	return ret;
}

static int write_cc_32(struct bd71827_power* pwr, u16 bcap_hi, u16 bcap_lo)
{
	int ret;
	u32 new;
	ret = __write_cc_32(pwr, bcap_hi, bcap_lo, pwr->regs->coulomb3, &new);
	if (!ret)
		pwr->d_w.coulomb_cnt = new;

	return ret;
}

static int stop_cc(struct bd71827_power *pwr)
{
	struct regmap *r = pwr->mfd->regmap;

	return regmap_update_bits(r, pwr->regs->coulomb_ctrl,
				  BD7182x_MASK_CCNTENB, 0);
}

static int start_cc(struct bd71827_power *pwr)
{
	struct regmap *r = pwr->mfd->regmap;

	return regmap_update_bits(r, pwr->regs->coulomb_ctrl,
				  BD7182x_MASK_CCNTENB, BD7182x_MASK_CCNTENB);
}

static int update_cc(struct bd71827_power *pwr, u16 bcap)
{
	int ret;

	ret = stop_cc(pwr);
	if (ret)
		goto err_out;

	ret = write_cc(pwr, bcap);
	if (ret)
		goto enable_out;

	ret = start_cc(pwr);
	if (ret)
		goto enable_out;

	return 0;

enable_out:
	start_cc(pwr);
err_out:
	dev_err(pwr->mfd->dev, "Coulomb counter write failed  (%d)\n", ret);
	return ret;
}
static int update_cc_32(struct bd71827_power* pwr, u16 bcap_hi, u16 bcap_lo)
{
	int ret;

	ret = stop_cc(pwr);
	if (ret)
		goto err_out;

	ret = write_cc_32(pwr, bcap_hi, bcap_lo);
	if (ret)
		goto enable_out;

	ret = start_cc(pwr);
	if (ret)
		goto enable_out;

	return 0;

enable_out:
	start_cc(pwr);
err_out:
	dev_err(pwr->mfd->dev, "Coulomb counter write failed  (%d)\n", ret);
	return ret;
}

static int __read_cc(struct bd71827_power *pwr, u32 *cc, unsigned int reg)
{
	int ret;
	u32 tmp_cc;

	ret = regmap_bulk_read(pwr->mfd->regmap, reg, &tmp_cc, sizeof(tmp_cc));
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read coulomb counter\n");
		return ret;
	}
	*cc = be32_to_cpu(tmp_cc) & BD7182x_MASK_CC_CCNTD;

	return 0;
}

static int read_cc_full(struct bd71827_power *pwr, u32 *cc)
{
	return __read_cc(pwr, cc, pwr->regs->coulomb_full3);
}

static int read_cc(struct bd71827_power *pwr, u32 *cc)
{
	return __read_cc(pwr, cc, pwr->regs->coulomb3);
}

static int limit_cc(struct bd71827_power *pwr, struct bd7182x_soc_data *wd,
			 u32 *soc_org)
{
	uint16_t bcap;
	int ret;

	*soc_org = 100;
	bcap = wd->designed_cap + wd->designed_cap / 200;
	ret = update_cc(pwr, bcap);

	dev_dbg(pwr->mfd->dev,  "Limit Coulomb Counter\n");
	dev_dbg(pwr->mfd->dev,  "CC_CCNTD = %d\n", wd->coulomb_cnt);

	return ret;
}

static int sync_cc(struct bd71827_power* pwr, struct bd7182x_soc_data *wd,
		    u32 soc_org)
{
	uint16_t bcap;
	int ret;

	bcap = ((wd->designed_cap + wd->designed_cap / 200) * soc_org) / 100;
	ret = update_cc(pwr, bcap);

	dev_err(pwr->mfd->dev,  "Sync Coulomb Counter\n");
	dev_err(pwr->mfd->dev,  "CC_CCNTD = %d\n", wd->coulomb_cnt);

	return ret;
}

/* @brief set initial coulomb counter value from battery voltage
 * @param pwr power device
 * @return 0
 */
static int calibration_coulomb_counter(struct bd71827_power *pwr,
				       struct bd7182x_soc_data *wd)
{
	struct regmap *regmap = pwr->mfd->regmap;
	u32 bcap;
	int soc, ocv, ret = 0, tmpret = 0;

#if INIT_COULOMB == BY_VBATLOAD_REG
	/* Get init OCV by HW */
	bd71827_get_init_bat_stat(pwr, &ocv);

	dev_dbg(pwr->mfd->dev, "ocv %d\n", ocv);
#elif INIT_COULOMB == BY_BAT_VOLT
	bd71827_calib_voltage(pwr, &ocv);
#endif

	/* Get init soc from ocv/soc table */
	soc = bd71827_voltage_to_capacity(ocv);
	dev_dbg(pwr->mfd->dev, "soc %d[0.1%%]\n", soc);
	if (soc < 0)
		soc = 0;
	bcap = wd->designed_cap * soc / 1000;

	tmpret = write_cc(pwr, bcap + wd->designed_cap / 200);
	if (tmpret)
		goto enable_cc_out;

	dev_dbg(pwr->mfd->dev, "%s() CC_CCNTD = %d\n", __func__,
		wd->coulomb_cnt);

enable_cc_out:
	/* Start canceling offset of the DS ADC. This needs 1 second at least */
	ret = regmap_update_bits(regmap, pwr->regs->coulomb_ctrl,
				 BD7182x_MASK_CCCALIB, BD7182x_MASK_CCCALIB);

	return (tmpret) ? tmpret : ret;
}

struct rohm_rtc {
	u8 sec;
	u8 min;
	u8 hour;
	u8 week;
	u8 day;
	u8 month;
	u8 year;
} __packed;

static inline void rtc2time(struct rohm_rtc *r, struct rtc_time *t)
{
	t->tm_sec = bcd2bin(r->sec & ROHM_BD1_MASK_RTC_SEC);
	t->tm_min = bcd2bin(r->min & ROHM_BD1_MASK_RTC_MINUTE);
	t->tm_hour = bcd2bin(r->hour & ROHM_BD1_MASK_RTC_HOUR);
	/*
	 * If RTC is in 12H mode, then bit ROHM_BD1_MASK_RTC_HOUR_PM
	 * is not BCD value but tells whether it is AM or PM
	 */
	if (!(r->hour & ROHM_BD1_MASK_RTC_HOUR_24H)) {
		t->tm_hour %= 12;
		if (r->hour & ROHM_BD1_MASK_RTC_HOUR_PM)
			t->tm_hour += 12;
	}
	t->tm_mday = bcd2bin(r->day & ROHM_BD1_MASK_RTC_DAY);
	t->tm_mon = bcd2bin(r->month & ROHM_BD1_MASK_RTC_MONTH) - 1;
	t->tm_year = 100 + bcd2bin(r->year & ROHM_BD1_MASK_RTC_YEAR);
	t->tm_wday = bcd2bin(r->week & ROHM_BD1_MASK_RTC_WEEK);
}

static int bd71827_record_rtc(struct bd71827_power *pwr, struct bd7182x_soc_data *wd)
{
	struct rohm_rtc current_rtc, last_poweroff_rtc;
	struct rtc_time current_time, last_poweroff_time;
	time64_t current_seconds, last_poweroff_seconds;
	int ret;

	ret = regmap_bulk_read(pwr->mfd->regmap, BD71828_REG_RTC_SEC, &current_rtc, sizeof(current_rtc));
	ret |= regmap_bulk_read(pwr->mfd->regmap, 0xf4, &last_poweroff_rtc, sizeof(last_poweroff_rtc));
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read RTC time (err %d)\n", ret);
	}
	else {
		rtc2time(&current_rtc, &current_time);
		rtc2time(&last_poweroff_rtc, &last_poweroff_time);
		current_seconds = rtc_tm_to_time64(&current_time);
		last_poweroff_seconds = rtc_tm_to_time64(&last_poweroff_time);
		if(current_seconds < (last_poweroff_seconds+60)) {
			return 0;
		}
		else {
			//keep power off date-time
			unsigned int value;

			//regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_SEC, &value);
			//regmap_write(pwr->mfd->regmap, 0xf4, value);
			regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_MINUTE, &value);
			regmap_write(pwr->mfd->regmap, 0xf5, value);
			regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_HOUR, &value);
			regmap_write(pwr->mfd->regmap, 0xf6, value);
			//regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_WEEK, &value);
			//regmap_write(pwr->mfd->regmap, 0xf7, value);
			regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_DAY, &value);
			regmap_write(pwr->mfd->regmap, 0xf8, value);
			regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_MONTH, &value);
			regmap_write(pwr->mfd->regmap, 0xf9, value);
			regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_YEAR, &value);
			regmap_write(pwr->mfd->regmap, 0xfa, value);
		}
	}

	return 0;
}

/* @brief adjust coulomb counter values at relaxed state
 * @param pwr power device
 * @return 0
 */
static int bd71827_adjust_coulomb_count(struct bd71827_power *pwr,
					struct bd7182x_soc_data *wd, int mode)
{
	int relax_ocv;
	u16 tmp;
	struct regmap *regmap = pwr->mfd->regmap;
	int ret;
	struct rohm_rtc current_rtc, last_poweroff_rtc;
	struct rtc_time current_time, last_poweroff_time;
	time64_t current_seconds, last_poweroff_seconds;

	if(mode) {
		ret = regmap_bulk_read(regmap, BD71828_REG_RTC_SEC, &current_rtc, sizeof(current_rtc));
		ret |= regmap_bulk_read(regmap, 0xf4, &last_poweroff_rtc, sizeof(last_poweroff_rtc));
		if (ret) {
			dev_err(pwr->mfd->dev, "Failed to read RTC time (err %d)\n", ret);
		}
		else {
			rtc2time(&current_rtc, &current_time);
			rtc2time(&last_poweroff_rtc, &last_poweroff_time);
			current_seconds = rtc_tm_to_time64(&current_time);
			last_poweroff_seconds = rtc_tm_to_time64(&last_poweroff_time);
			dev_err(pwr->mfd->dev, "Current second %llu\n", current_seconds);
			dev_err(pwr->mfd->dev, "Last power off second %llu\n", last_poweroff_seconds);
			if(current_seconds < (last_poweroff_seconds+1296000)) {
				dev_err(pwr->mfd->dev, "Till HBNT over 15 dsys to relax check\n");
				return 0;
			}
			else
				dev_err(pwr->mfd->dev, "Check Relax voltage\n");
		}
	}

	ret = bd7182x_read16_himask(pwr, pwr->regs->vbat_rex_avg,
			BD7182x_MASK_VBAT_U, &tmp);
	if (ret)
		return ret;

	relax_ocv = ((int)tmp) * 1000;

	if (relax_ocv != 0) {
		u32 bcap;
		int soc;

		dev_err(pwr->mfd->dev,  "%s(): relax_ocv = 0x%x\n", __func__, relax_ocv);
		/* Clear Relaxed Coulomb Counter */
		ret = regmap_update_bits(regmap, pwr->regs->rex_clear_reg,
					 pwr->regs->rex_clear_mask,
					 pwr->regs->rex_clear_mask);
		if (ret)
			return ret;

		/* Get soc at relaxed state from ocv/soc table */
		soc = bd71827_voltage_to_capacity(relax_ocv);
		dev_dbg(pwr->mfd->dev,  "soc %d[0.1%%]\n", soc);
		if (soc < 0)
			soc = 0;

		bcap = wd->designed_cap * soc / 1000;
		bcap = (bcap + wd->designed_cap / 200);

		ret = update_cc(pwr, bcap);
		if (ret)
			return ret;

		dev_err(pwr->mfd->dev,
			"Adjust Coulomb Counter at Relaxed State\n");
		dev_err(pwr->mfd->dev, "CC_CCNTD = %d\n",
			wd->coulomb_cnt);
		dev_err(pwr->mfd->dev,
			"relaxed_ocv:%d, bcap:%d, soc:%d, coulomb_cnt:%d\n",
			relax_ocv, bcap, soc, wd->coulomb_cnt);

		/* If the following commented out code is enabled,
		 * the SOC is not clamped at the relax time.
		 */
		/* Reset SOCs */
		/* bd71827_calc_soc_org(pwr, wd); */
		/* wd->soc_norm = wd->soc_org; */
		/* wd->soc = wd->soc_norm; */
		/* wd->clamp_soc = wd->soc; */
	}

	return ret;
}

/* @brief reset coulomb counter values at full charged state
 * @param pwr power device
 * @return 0
 */
static int bd71827_reset_coulomb_count(struct bd71827_power *pwr,
				       struct bd7182x_soc_data *wd)
{
	u32 full_charged_coulomb_cnt;
	struct regmap *regmap = pwr->mfd->regmap;
	int ret;

	ret = read_cc_full(pwr, &full_charged_coulomb_cnt);
	if (ret) {
		dev_err(pwr->mfd->dev, "failed to read full coulomb counter\n");
		return ret;
	}

	if (full_charged_coulomb_cnt != 0) {
		int diff_coulomb_cnt;
		u32 cc;
		u16 bcap;

		dev_err(pwr->mfd->dev, "%s(): full_charged_coulomb_cnt=0x%x\n", __func__, full_charged_coulomb_cnt);
		/* Clear Full Charged Coulomb Counter */
		ret = regmap_update_bits(regmap, pwr->regs->cc_full_clr,
					 BD7182x_MASK_CC_FULL_CLR,
					 BD7182x_MASK_CC_FULL_CLR);

		bd71827_charge_status(pwr, wd, 0);
		dev_err(pwr->mfd->dev, "battery status %d\n", wd->rpt_status);
		if(wd->rpt_status != POWER_SUPPLY_STATUS_FULL) {
			dev_err(pwr->mfd->dev, "Battery charge status isn't POWER_SUPPLY_STATUS_FULL !!!!!! Ignored!!!!!\n");
			return 0;
		}

		ret = read_cc(pwr, &cc);
		if (ret)
			return ret;

		diff_coulomb_cnt = full_charged_coulomb_cnt - cc;

		diff_coulomb_cnt = diff_coulomb_cnt >> 16;
		if (diff_coulomb_cnt > 0)
			diff_coulomb_cnt = 0;

		dev_err(pwr->mfd->dev,  "diff_coulomb_cnt = %d\n",
			diff_coulomb_cnt);

		bcap = wd->designed_cap + wd->designed_cap / 200 +
		       diff_coulomb_cnt;
		ret = update_cc(pwr, bcap);
		if (ret)
			return ret;
		dev_err(pwr->mfd->dev,
			"Reset Coulomb Counter at POWER_SUPPLY_STATUS_FULL\n");
		dev_err(pwr->mfd->dev, "CC_CCNTD = %d\n", wd->coulomb_cnt);
	}

	return 0;
}

/* @brief get battery parameters, such as voltages, currents, temperatures.
 * @param pwr power device
 * @return 0
 */
static int bd71827_get_voltage_current(struct bd71827_power *pwr,
				       struct bd7182x_soc_data *wd)
{
	int ret;
	int temp, temp2;

	if (pwr->mfd->chip_type != ROHM_CHIP_TYPE_BD71828 &&
	    pwr->mfd->chip_type != ROHM_CHIP_TYPE_BD71827) {
		return -EINVAL;
	}

	ret = bd71827_get_vbat(pwr, &temp);
	if (ret)
		return ret;

	wd->vcell = temp;
	ret = bd71827_get_current_ds_adc(pwr, &temp, &temp2);
	if (ret)
		return ret;
	wd->curr_avg = temp2;
	wd->curr = temp;

	/* Read detailed vsys */
	ret = bd71827_get_vsys(pwr, &temp);
	if (ret)
		return ret;

	wd->vsys = temp;
	dev_dbg(pwr->mfd->dev,  "VM_VSYS = %d\n", temp);

	/* Read detailed vbat_min */
	ret = bd71827_get_vbat_min(pwr, &temp);
	if (ret)
		return ret;
	wd->vcell_min = temp;
	dev_dbg(pwr->mfd->dev,  "VM_VBAT_MIN = %d\n", temp);

	/* Read detailed vsys_min */
	ret = bd71827_get_vsys_min(pwr, &temp);
	if (ret)
		return ret;

	wd->vsys_min = temp;
	dev_dbg(pwr->mfd->dev,  "VM_VSYS_MIN = %d\n", temp);

	/* Get tempature */
	ret = pwr->get_temp(pwr, &temp);

	if (ret)
		return ret;

	wd->temp = temp;

	return 0;
}

/* @brief adjust coulomb counter values at relaxed state by SW
 * @param pwr power device
 * @return 0
 */

static int bd71827_adjust_coulomb_count_sw(struct bd71827_power *pwr,
					   struct bd7182x_soc_data *wd)
{
	int tmp_curr_mA, ret;

	tmp_curr_mA = uAMP_TO_mAMP(wd->curr);
	if ((tmp_curr_mA * tmp_curr_mA) <=
	    (THR_RELAX_CURRENT_DEFAULT * THR_RELAX_CURRENT_DEFAULT))
		 /* No load */
		wd->relax_time = wd->relax_time + (JITTER_DEFAULT / 1000);
	else
		wd->relax_time = 0;

	if (wd->relax_time >= THR_RELAX_TIME_DEFAULT) {
		/* Battery is relaxed. */
		u32 bcap;
		int soc, ocv;

		dev_err(pwr->mfd->dev,  "%s(): pwr->relax_time = 0x%x\n", __func__, wd->relax_time);
		wd->relax_time = 0;

		/* Get OCV */
		ocv = wd->vcell;

		/* Get soc at relaxed state from ocv/soc table */
		soc = bd71827_voltage_to_capacity(ocv);
		dev_err(pwr->mfd->dev,  "soc %d[0.1%%]\n", soc);
		if (soc < 0)
			soc = 0;

		bcap = wd->designed_cap * soc / 1000;

		ret = update_cc(pwr, bcap + wd->designed_cap / 200);
		if (ret)
			return ret;

		dev_err(pwr->mfd->dev,
			"Adjust Coulomb Counter by SW at Relaxed State\n");
		dev_err(pwr->mfd->dev, "CC_CCNTD = %d\n", wd->coulomb_cnt);

		/* If the following commented out code is enabled,
		 * the SOC is not clamped at the relax time.
		 */
		/* Reset SOCs */
		/* bd71827_calc_soc_org(pwr, wd); */
		/* wd->soc_norm = wd->soc_org; */
		/* wd->soc = wd->soc_norm; */
		/* wd->clamp_soc = wd->soc; */
	}

	return 0;
}

/* @brief get coulomb counter values
 * @param pwr power device
 * @return 0
 */
static int bd71827_coulomb_count(struct bd71827_power *pwr,
				 struct bd7182x_soc_data *wd)
{
	int ret = 0;

	dev_dbg(pwr->mfd->dev, "%s(): pwr->state_machine = 0x%x\n", __func__,
		wd->state_machine);
	if (wd->state_machine == STAT_POWER_ON) {
		wd->state_machine = STAT_INITIALIZED;
		/* Start Coulomb Counter */
		ret = start_cc(pwr);
	} else if (wd->state_machine == STAT_INITIALIZED) {
		u32 cc;
		int delta_cc;
		u32 last_cc;
		
		last_cc = wd->coulomb_cnt;
		ret = read_cc(pwr, &cc);
		delta_cc = cc - wd->coulomb_cnt;
		
		dev_dbg(pwr->mfd->dev, "%s(): last_cc=%d,cur_cc=%d,delta_cc=%d\n", __func__,
										wd->coulomb_cnt,cc,delta_cc);
#if 0
		if ((pwr->charger_type == BD_71879_CHARGER_TYPE_DCP) && (wd->curr > 800000) && (wd->soc < 90)) {
			uint16_t bcap;
			uint16_t bcap_lo;

			dev_dbg(pwr->mfd->dev, "%s(): fast charge status, adjust cc,cc=%d,curr=%d\n", __func__,
									wd->coulomb_cnt,wd->curr);

			bcap = (wd->coulomb_cnt >> 16) + 4; /* add constant cc 311296*/
			bcap_lo = (wd->coulomb_cnt & 0xffff) + 49152;
			update_cc_32(pwr, bcap, bcap_lo);
		}
#else
		if (pwr->charger_type == BD_71879_CHARGER_TYPE_DCP && cc_calc_curve_en) {
			if (cc_calc_skip) {
				uint16_t bcap_hi;
				uint16_t bcap_lo;

				dev_dbg(pwr->mfd->dev, "%s(): fast charge status, adjust cc,cc=%d,curr=%d, cc_calc_curve_en=%d\n", __func__,
									wd->coulomb_cnt,wd->curr,cc_calc_curve_en);
				delta_cc = (delta_cc * 980 / 1000); // 0.98
				
				bcap_hi = (wd->coulomb_cnt >> 16) + (delta_cc >> 16);
				bcap_lo = (wd->coulomb_cnt & 0xffff) + (delta_cc & 0xffff);
				update_cc_32(pwr, bcap_hi, bcap_lo);
				if (wd->curr >= 1510000)
					cc_calc_skip = 1;
				else
					cc_calc_skip = 0;
			} else
				cc_calc_skip = 1;
		}
#endif
		ret = read_cc(pwr, &cc);
		if (pwr->charger_type == BD_71879_CHARGER_TYPE_DCP && cc_calc_curve_en) {
			dev_dbg(pwr->mfd->dev, "check new_cc=%d,last_cc=%d,cc_calc_curve_en=%d\n", cc, last_cc, cc_calc_curve_en);
			if (cc < last_cc || wd->soc >= 90) {
				cc_calc_curve_en = 0;
				cc_calc_skip = 0;
			}
		}
		wd->coulomb_cnt = cc;
	}
    dev_dbg(pwr->mfd->dev, "%s(): state_machine = 0x%x,cc=%d,cc_calc_curve_en=%d\n", __func__,
		wd->state_machine,wd->coulomb_cnt,cc_calc_curve_en);
	return ret;
}

/* @brief calc cycle
 * @param pwr power device
 * @return 0
 */
static int bd71827_update_cycle(struct bd71827_power *pwr,
				struct bd7182x_soc_data *wd)
{
	int tmpret, ret;
	u16 charged_coulomb_cnt;

	ret = bd7182x_read16_himask(pwr, pwr->regs->coulomb_chg3, 0xff,
				    &charged_coulomb_cnt);
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read charging CC (%d)\n",
			ret);
		return ret;
	}

	dev_dbg(pwr->mfd->dev, "%s(): charged_coulomb_cnt = 0x%x\n", __func__,
		(int)charged_coulomb_cnt);
	if (charged_coulomb_cnt >= wd->designed_cap) {
		wd->cycle++;
		dev_dbg(pwr->mfd->dev,  "Update cycle = %d\n", wd->cycle);
		battery_cycle = wd->cycle;
		charged_coulomb_cnt -= wd->designed_cap;

		ret = stop_cc(pwr);
		if (ret)
			return ret;

		ret = bd7182x_write16(pwr, pwr->regs->coulomb_chg3,
				      charged_coulomb_cnt);
		if (ret) {
			dev_err(pwr->mfd->dev,
				"Failed to update charging CC (%d)\n", ret);
		}

		tmpret = start_cc(pwr);
		if (tmpret)
			return tmpret;
	}
	return ret;
}

/* @brief calc full capacity value by Cycle and Temperature
 * @param pwr power device
 * @return 0
 */
static int bd71827_calc_full_cap(struct bd71827_power *pwr,
				 struct bd7182x_soc_data *wd)
{
	u32 designed_cap_uAh;
	u32 full_cap_uAh;

	/* Calculate full capacity by cycle */
	designed_cap_uAh = A10s_mAh(wd->designed_cap, pwr->curr_factor) * 1000;

	if (dgrd_cyc_cap * wd->cycle >= designed_cap_uAh) {
		/* Battry end of life? */
		wd->full_cap = 1;
		return 0;
	}

	full_cap_uAh = designed_cap_uAh - dgrd_cyc_cap * wd->cycle;
	wd->full_cap = mAh_A10s(uAMP_TO_mAMP(full_cap_uAh), pwr->curr_factor);
	dev_dbg(pwr->mfd->dev,  "Calculate full capacity by cycle\n");
	dev_dbg(pwr->mfd->dev,  "%s() pwr->full_cap = %d\n", __func__,
		wd->full_cap);

	/* Calculate full capacity by temperature */
	dev_dbg(pwr->mfd->dev,  "Temperature = %d\n", wd->temp);
	if (wd->temp >= DGRD_TEMP_M_DEFAULT) {
		full_cap_uAh += (wd->temp - DGRD_TEMP_M_DEFAULT) *
				dgrd_temp_cap_h;
		wd->full_cap = mAh_A10s(uAMP_TO_mAMP(full_cap_uAh), pwr->curr_factor);
	} else if (wd->temp >= DGRD_TEMP_L_DEFAULT) {
		full_cap_uAh += (wd->temp - DGRD_TEMP_M_DEFAULT) *
				dgrd_temp_cap_m;
		wd->full_cap = mAh_A10s(uAMP_TO_mAMP(full_cap_uAh), pwr->curr_factor);
	} else {
		full_cap_uAh += (DGRD_TEMP_L_DEFAULT - DGRD_TEMP_M_DEFAULT) *
				dgrd_temp_cap_m;
		full_cap_uAh += (wd->temp - DGRD_TEMP_L_DEFAULT) *
				dgrd_temp_cap_l;
		wd->full_cap = mAh_A10s(uAMP_TO_mAMP(full_cap_uAh), pwr->curr_factor);
	}

	if (wd->full_cap < 1)
		wd->full_cap = 1;

	dev_dbg(pwr->mfd->dev,  "Calculate full capacity by cycle and temperature\n");
	dev_dbg(pwr->mfd->dev,  "%s() pwr->full_cap = %d\n", __func__,
		wd->full_cap);

	return 0;
}

/* @brief calculate SOC values by designed capacity
 * @param pwr power device
 * @return 0
 */
static unsigned int bd71827_calc_soc_org(u32 cc, int designed_cap)
{
	return (cc >> 16) * 100 / designed_cap;
}

/* @brief calculate SOC values by full capacity
 * @param pwr power device
 * @return 0
 */
static int bd71827_calc_soc_norm(struct bd71827_power *pwr,
				 struct bd7182x_soc_data *wd)
{
	int lost_cap;
	int mod_coulomb_cnt;

	lost_cap = wd->designed_cap - wd->full_cap;
	dev_dbg(pwr->mfd->dev,  "%s() lost_cap = %d\n", __func__, lost_cap);

	mod_coulomb_cnt = (wd->coulomb_cnt >> 16) - lost_cap;
	if ((mod_coulomb_cnt > 0) && (wd->full_cap > 0))
		wd->soc_norm = mod_coulomb_cnt * 100 / wd->full_cap;
	else
		wd->soc_norm = 0;

	if (wd->soc_norm > 100)
		wd->soc_norm = 100;

	dev_dbg(pwr->mfd->dev,  "%s() pwr->soc_norm = %d\n", __func__,
		wd->soc_norm);

	return 0;
}

/* @brief get OCV value by SOC
 * @param pwr power device
 * @return 0
 */
int bd71827_get_ocv(struct bd71827_power *pwr, int dsoc)
{
	int i = 0;
	int ocv = 0;

	if (dsoc > soc_table[0]) {
		ocv = MAX_VOLTAGE_DEFAULT;
	} else if (dsoc == 0) {
		ocv = ocv_table[21];
	} else {
		i = 0;
		while (i < 22) {
			if ((dsoc <= soc_table[i]) &&
					(dsoc > soc_table[i + 1])) {
				ocv = (ocv_table[i] - ocv_table[i + 1]) *
				      (dsoc - soc_table[i + 1]) /
				      (soc_table[i] - soc_table[i + 1]) +
				      ocv_table[i + 1];
				break;
			}
			i++;
		}
		if (i == 22)
			ocv = ocv_table[22];
	}
	dev_dbg(pwr->mfd->dev,  "%s() ocv = %d\n", __func__, ocv);
	return ocv;
}

static void calc_vdr(int *res, int *vdr, int temp, int dgrd_temp,
		     int *vdr_hi, int dgrd_temp_hi, int items)
{
	int i;

	for (i = 0; i < items; i++)
		res[i] = vdr[i] + (temp - dgrd_temp) * (vdr_hi[i] - vdr[i]) /
			 (dgrd_temp_hi - dgrd_temp);
}

/* @brief get VDR(Voltage Drop Rate) value by SOC
 * @param pwr power device
 * @return 0
 */
static int bd71827_get_vdr(struct bd71827_power *pwr, int dsoc,
			   struct bd7182x_soc_data *wd)
{
	int i = 0;
	int vdr = 100;
	int vdr_table[23];

	/* Calculate VDR by temperature */
	if (wd->temp >= DGRD_TEMP_H_DEFAULT)
		for (i = 0; i < 23; i++)
			vdr_table[i] = vdr_table_h[i];
	else if (wd->temp >= DGRD_TEMP_M_DEFAULT)
		calc_vdr(vdr_table, vdr_table_m, wd->temp, DGRD_TEMP_M_DEFAULT,
			 vdr_table_h, DGRD_TEMP_H_DEFAULT, 23);
	else if (wd->temp >= DGRD_TEMP_L_DEFAULT)
		calc_vdr(vdr_table, vdr_table_l, wd->temp, DGRD_TEMP_L_DEFAULT,
			 vdr_table_m, DGRD_TEMP_M_DEFAULT, 23);
	else if (wd->temp >= DGRD_TEMP_VL_DEFAULT)
		calc_vdr(vdr_table, vdr_table_vl, wd->temp,
			 DGRD_TEMP_VL_DEFAULT, vdr_table_l, DGRD_TEMP_L_DEFAULT,
			 23);
	else
		for (i = 0; i < 23; i++)
			vdr_table[i] = vdr_table_vl[i];

	if (dsoc > soc_table[0]) {
		vdr = 100;
	} else if (dsoc == 0) {
		vdr = vdr_table[21];
	} else {
		for (i = 0; i < 22; i++)
			if ((dsoc <= soc_table[i]) &&
				(dsoc > soc_table[i + 1])) {
				vdr = (vdr_table[i] - vdr_table[i + 1]) *
				      (dsoc - soc_table[i + 1]) /
				      (soc_table[i] - soc_table[i + 1]) +
				      vdr_table[i + 1];
				break;
			}
		if (i == 22)
			vdr = vdr_table[22];
	}
	dev_dbg(pwr->mfd->dev, "%s() vdr = %d\n", __func__, vdr);
	return vdr;
}

/* @brief calculate SOC value by full_capacity and load
 * @param pwr power device
 * @return OCV
 */

static void soc_not_charging(struct bd71827_power *pwr,
			    struct bd7182x_soc_data *wd)
{
	int ocv_table_load[23];
	int i;
	int ocv;
	int lost_cap;
	int mod_coulomb_cnt;
	int dsoc;

	lost_cap = wd->designed_cap - wd->full_cap;
	mod_coulomb_cnt = (wd->coulomb_cnt >> 16) - lost_cap;
	dsoc = mod_coulomb_cnt * 1000 /  wd->full_cap;
	dev_dbg(pwr->mfd->dev,  "%s() dsoc = %d\n", __func__,
		dsoc);

	ocv = bd71827_get_ocv(pwr, dsoc);
	for (i = 1; i < 23; i++) {
		ocv_table_load[i] = ocv_table[i] - (ocv - wd->vcell);
		if (ocv_table_load[i] <= MIN_VOLTAGE_DEFAULT) {
			dev_dbg(pwr->mfd->dev,
				"%s() ocv_table_load[%d] = %d\n", __func__,
				i, ocv_table_load[i]);
			break;
		}
	}
	if (i < 23) {
		int j, k, m;
		int dv;
		int lost_cap2, new_lost_cap2;
		int mod_coulomb_cnt2, mod_full_cap;
		int dsoc0;
		int vdr, vdr0;

		dv = (ocv_table_load[i - 1] - ocv_table_load[i]) / 5;
		for (j = 1; j < 5; j++) {
			if ((ocv_table_load[i] + dv * j) >
			    MIN_VOLTAGE_DEFAULT) {
				break;
			}
		}
		lost_cap2 = ((21 - i) * 5 + (j - 1)) * wd->full_cap / 100;
		dev_dbg(pwr->mfd->dev, "%s() lost_cap2-1 = %d\n", __func__,
			lost_cap2);
		for (m = 0; m < soc_est_max_num; m++) {
			new_lost_cap2 = lost_cap2;
			dsoc0 = lost_cap2 * 1000 / wd->full_cap;
			if ((dsoc >= 0 && dsoc0 > dsoc) ||
			    (dsoc < 0 && dsoc0 < dsoc))
				dsoc0 = dsoc;

			dev_dbg(pwr->mfd->dev, "%s() dsoc0(%d) = %d\n",
				__func__, m, dsoc0);

			vdr = bd71827_get_vdr(pwr, dsoc, wd);
			vdr0 = bd71827_get_vdr(pwr, dsoc0, wd);

			for (k = 1; k < 23; k++) {
				ocv_table_load[k] = ocv_table[k] -
						    (ocv - wd->vcell) * vdr0
						    / vdr;
				if (ocv_table_load[k] <= MIN_VOLTAGE_DEFAULT) {
					dev_dbg(pwr->mfd->dev,
						"%s() ocv_table_load[%d] = %d\n",
						__func__, k, ocv_table_load[k]);
					break;
				}
			}
			if (k < 23) {
				dv = (ocv_table_load[k - 1] -
				     ocv_table_load[k]) / 5;
				for (j = 1; j < 5; j++)
					if ((ocv_table_load[k] + dv * j) >
					     MIN_VOLTAGE_DEFAULT)
						break;

				new_lost_cap2 = ((21 - k) * 5 + (j - 1)) *
						wd->full_cap / 100;
				if (soc_est_max_num == 1)
					lost_cap2 = new_lost_cap2;
				else
					lost_cap2 +=
					(new_lost_cap2 - lost_cap2) /
					(2 * (soc_est_max_num - m));

				dev_dbg(pwr->mfd->dev,
					"%s() lost_cap2-2(%d) = %d\n", __func__,
					m, lost_cap2);
			}
			if (new_lost_cap2 == lost_cap2)
				break;
		}
		mod_coulomb_cnt2 = mod_coulomb_cnt - lost_cap2;
		mod_full_cap = wd->full_cap - lost_cap2;
		if ((mod_coulomb_cnt2 > 0) && (mod_full_cap > 0))
			wd->soc = mod_coulomb_cnt2 * 100 / mod_full_cap;
		else
			wd->soc = 0;

		dev_dbg(pwr->mfd->dev,  "%s() pwr->soc(by load) = %d\n",
			__func__, wd->soc);
	}
}

static int pre_soc = 0;
static void soc_charging(struct bd71827_power *pwr,
			    struct bd7182x_soc_data *wd)
{
	int i, need_calc_soc = 0;
	u32 cc, tmp_soc = 0, tmp_curr, tmp_next_curr;
	uint16_t bcap;
	u32 calc_soc = 0;
	
	if (pwr->charger_type == BD_71879_CHARGER_TYPE_DCP) {
		if (pre_soc == 89 && wd->soc == 90) {
			pwr->calc_soc_jiffies = jiffies;
			if (wd->curr > curr_90_min) {
				curr_90_max = wd->curr;
				curr_90_avg = (wd->curr - curr_90_min) / 5;
			}
		}
		pre_soc = wd->soc;
	
		if (wd->soc >= 90 && wd->soc < 95) {
			if (wd->curr >= curr_90_max) { // current > curr_90_max , keep 90%
				dev_dbg(pwr->mfd->dev, "%s(): curr > curr_90_max keep soc = %d\n", __func__, wd->soc);
				if (wd->soc > 90)
					tmp_soc = wd->soc;
				else
					tmp_soc = 90;
				need_calc_soc = 1;
			} else if (wd->curr < curr_90_max && wd->curr >= curr_90_min) { // curr_90_min <= current < curr_90_max
				calc_soc = (curr_90_max - wd->curr) / curr_90_avg;
				dev_dbg(pwr->mfd->dev, "%s(): curr_90_min < curr < curr_90_max calc_soc = %d\n", __func__, calc_soc);
				tmp_soc = 90 + calc_soc;
				if (tmp_soc > wd->soc + 1) {
					if (time_after(jiffies, pwr->calc_soc_jiffies + msecs_to_jiffies(120000))) { // keep prev soc 2min
						pwr->calc_soc_jiffies = jiffies;
						tmp_soc = wd->soc + 1;
					} else
						tmp_soc = wd->soc;
				} else if (tmp_soc < wd->soc)
					tmp_soc = wd->soc;
				need_calc_soc = 1;
			} else if (wd->curr < curr_90_min) { // current < curr_90_min
				dev_dbg(pwr->mfd->dev, "%s(): curr <= curr_90_min need calc_soc = 95\n", __func__);
				if (time_after(jiffies, pwr->calc_soc_jiffies + msecs_to_jiffies(120000))) { // keep prev soc 2min
						pwr->calc_soc_jiffies = jiffies;
						tmp_soc = wd->soc + 1;
				} else
					tmp_soc = wd->soc;
				need_calc_soc = 1;
			} 
		} else if (wd->soc >= 95 && wd->soc < 100) {
			for (i = 0; i < 5; i++) {
				tmp_curr = calc_soc_table[i].curr * 1000;
				tmp_next_curr = calc_soc_table[i+1].curr * 1000;
				dev_dbg(pwr->mfd->dev, "%s(): i=%d, tmp_curr=%d, tmp_next_curr=%d\n", __func__,
						i, tmp_curr, tmp_next_curr);
				if (wd->curr > tmp_next_curr) {
					tmp_soc = calc_soc_table[i].soc;
					dev_dbg(pwr->mfd->dev, "%s(): curr > next_curr tmp_soc=%d\n", __func__, tmp_soc);
					need_calc_soc = 1;
				} else if (i == 4 && wd->curr < tmp_next_curr) {
					tmp_soc = wd->soc + 1;
					dev_dbg(pwr->mfd->dev, "%s(): curr < next_curr tmp_soc=%d\n", __func__, tmp_soc);
					need_calc_soc = 1;
				} 
				
				if (need_calc_soc)
					break;
			}
			
			if (need_calc_soc) {
				if (tmp_soc < wd->soc)
					tmp_soc = wd->soc;
				else if (tmp_soc >= wd->soc + 1) {
					if (time_after(jiffies, pwr->calc_soc_jiffies + msecs_to_jiffies(120000))) { // keep prev soc 2min
						tmp_soc = wd->soc + 1;
						pwr->calc_soc_jiffies = jiffies;
					} else
						tmp_soc = wd->soc;
				}
			}
			
		}
		
		dev_dbg(pwr->mfd->dev, "%s(): need_calc_soc(%d) cc=%d, curr=%d, tmp_soc=%d\n", __func__,
						need_calc_soc, wd->coulomb_cnt, wd->curr, tmp_soc);
		
		if (need_calc_soc) {
			bcap = wd->designed_cap * tmp_soc / 100;

			update_cc(pwr, bcap + wd->designed_cap / 200);
				
			read_cc(pwr, &cc);
			wd->coulomb_cnt = cc;
			dev_dbg(pwr->mfd->dev, "%s(): fast charge status, update cc,cc=%d,curr=%d\n", __func__,
								wd->coulomb_cnt,wd->curr);
		}
	}
	
 	return;
}

static int bd71827_calc_soc(struct bd71827_power *pwr,
			    struct bd7182x_soc_data *wd)
{
	wd->soc = wd->soc_norm;

	 /* Adjust for 0% between thr_voltage and min_voltage */
	switch (wd->rpt_status) {
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if (wd->vcell <= THR_VOLTAGE_DEFAULT)
			soc_not_charging(pwr, wd);
		else
			wd->soc = 100;
		break;
	default:
		break;
	}

	switch (wd->rpt_status) {/* Adjust for 0% and 100% */
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if (wd->vcell <= MIN_VOLTAGE_DEFAULT)
			wd->soc = 0;
		else if (wd->soc == 0)
			wd->soc = 1;
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		soc_charging(pwr, wd);
		if (wd->soc == 100)
			wd->soc = 99;
		break;
	default:
		break;
	}
	dev_dbg(pwr->mfd->dev,  "%s() pwr->soc = %d\n", __func__, wd->soc);
	return 0;
}

/* @brief calculate Clamped SOC value by full_capacity and load
 * @param pwr power device
 * @return OCV
 */
static int bd71827_calc_soc_clamp(struct bd71827_power *pwr,
				  struct bd7182x_soc_data *wd)
{
	switch (wd->rpt_status) {/* Adjust for 0% and 100% */
	case POWER_SUPPLY_STATUS_DISCHARGING:
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		if((wd->soc <= wd->clamp_soc && !ciritical_low_count) || ((wd->vcell < through_voltage) && ciritical_low_count))
			wd->clamp_soc = wd->soc;
		break;
	case POWER_SUPPLY_STATUS_FULL:
		wd->clamp_soc = 100;
		break;
	default:
		wd->clamp_soc = wd->soc;
		break;
	}
	dev_dbg(pwr->mfd->dev,  "%s() pwr->clamp_soc = %d\n", __func__,
		wd->clamp_soc);
	return 0;
}

/* @brief get battery and DC online status
 * @param pwr power device
 * @return 0
 */
static int bd71827_get_online(struct bd71827_power *pwr,
			      struct bd7182x_soc_data *wd)
{
	int r, ret;

#if 0
#define TS_THRESHOLD_VOLT	0xD9
	r = bd71827_reg_read(pwr->mfd, BD71827_REG_VM_VTH);
	pwr->bat_online = (r > TS_THRESHOLD_VOLT);
#endif
#if 0
	r = bd71827_reg_read(pwr->mfd, BD71827_REG_BAT_STAT);
	if (r >= 0 && (r & BAT_DET_DONE))
		pwr->bat_online = (r & BAT_DET) != 0;
#endif
#if 1
#define BAT_OPEN	0x7
	ret = regmap_read(pwr->mfd->regmap, pwr->regs->bat_temp, &r);
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read battery temperature\n");
		return ret;
	}
	wd->bat_online = ((r & BD7182x_MASK_BAT_TEMP) != BAT_OPEN);
#endif
	ret = regmap_read(pwr->mfd->regmap, pwr->regs->dcin_stat, &r);
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read DCIN status\n");
		return ret;
	}
	wd->charger_online = ((r & BD7182x_MASK_DCIN_DET) != 0);

	dev_dbg(pwr->mfd->dev,
		"%s(): pwr->bat_online = %d, pwr->charger_online = %d\n",
		__func__, wd->bat_online, wd->charger_online);

	return 0;
}

/* @brief init bd71827 sub module charger
 * @param pwr power device
 * @return 0
 */
static int bd71827_init_hardware(struct bd71827_power *pwr,
				 struct bd7182x_soc_data *wd)
{
	int r, temp, ret;
	uint16_t tmp;
	u32 cc, sorg;

	ret = regmap_write(pwr->mfd->regmap, pwr->regs->dcin_collapse_limit,
			   BD7182x_DCIN_COLLAPSE_DEFAULT);
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to write DCIN collapse limit\n");
		return ret;
	}

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->conf, &r);
	if (ret) {
		dev_err(pwr->mfd->dev, "Failed to read CONF register\n");
		return ret;
	}

	/* Always set default Battery Capacity ? */
	dev_dbg(pwr->mfd->dev, "charge-battery-cap=%d, discharge-battery-cap=%d\n", charge_battery_cap, discharge_battery_cap);
	wd->designed_cap = battery_cap;
	wd->full_cap = battery_cap;
	/* Why BD71827_REG_CC_BATCAP_U is not used? */
	// bd71827_reg_read16(pwr->mfd, BD71827_REG_CC_BATCAP_U);

	if (r & BD7182x_MASK_CONF_PON) {
		/* Init HW, when the battery is inserted. */

		dev_info(pwr->mfd->dev, "%s() BD7182x_MASK_CONF_PON on\n", __func__);
		ret = regmap_update_bits(pwr->mfd->regmap, pwr->regs->conf,
					 BD7182x_MASK_CONF_PON, 0);
		if (ret) {
			dev_err(pwr->mfd->dev, "Failed to clear CONF register\n");
			return ret;
		}

		/* Stop Coulomb Counter */
		ret = stop_cc(pwr);
		if (ret)
			return ret;

		/* Set Coulomb Counter Reset bit*/
		ret = regmap_update_bits(pwr->mfd->regmap,
					 pwr->regs->coulomb_ctrl,
					 BD7182x_MASK_CCNTRST,
					 BD7182x_MASK_CCNTRST);
		if (ret)
			return ret;

		/* Clear Coulomb Counter Reset bit*/
		ret = regmap_update_bits(pwr->mfd->regmap,
					 pwr->regs->coulomb_ctrl,
					 BD7182x_MASK_CCNTRST, 0);
		if (ret)
			return ret;

		/* Clear Relaxed Coulomb Counter */
		ret = regmap_update_bits(pwr->mfd->regmap,
					 pwr->regs->rex_clear_reg,
					 pwr->regs->rex_clear_mask,
					 pwr->regs->rex_clear_mask);

		/* Set initial Coulomb Counter by HW OCV */
		calibration_coulomb_counter(pwr, wd);

		/* WDT_FST auto set */
		ret = regmap_update_bits(pwr->mfd->regmap, pwr->regs->chg_set1,
					 BD7182x_MASK_WDT_AUTO,
					 BD7182x_MASK_WDT_AUTO);
		if (ret)
			return ret;

		/* VBAT Low voltage detection Setting, added by John Zhang*/
		/* bd71827_reg_write16(mfd,
		 * BD71827_REG_ALM_VBAT_TH_U, VBAT_LOW_TH);
		 */

		/*ret = bd7182x_write16(pwr, pwr->regs->vbat_alm_limit_u,
				      VBAT_LOW_TH);
		if (ret)
			return ret;*/

		ret = bd7182x_write16(pwr, pwr->regs->batcap_mon_limit_u,
				      battery_cap * 9 / 10);
		if (ret)
			return ret;

		/* Set Battery Capacity Monitor threshold1 as 90% */
		dev_dbg(pwr->mfd->dev, "BD71827_REG_CC_BATCAP1_TH = %d\n",
			(battery_cap * 9 / 10));

		/* Enable LED ON when charging
		 * Should we do this decision here? Should the enabling be
		 * in LED driver and come from DT?
		 * bd71827_set_bits(pwr->mfd,
		 * BD71827_REG_LED_CTRL, CHGDONE_LED_EN);
		 */
		wd->state_machine = STAT_POWER_ON;
	} else {
		dev_info(pwr->mfd->dev, "%s() BD7182x_MASK_CONF_PON off\n", __func__);
		wd->state_machine = STAT_INITIALIZED;	// STAT_INITIALIZED
	}

	ret = bd7182x_write16(pwr, pwr->regs->vbat_alm_limit_u,
			      VBAT_LOW_TH);
	if (ret)
		return ret;

	bd7182x_read16_himask(pwr, pwr->regs->vbat_alm_limit_u , 0x01, &tmp);
	through_voltage = tmp*16000;
	dev_err(pwr->mfd->dev,  "%s() battery through_voltage = %d\n", __func__, through_voltage);

	ret = pwr->get_temp(pwr, &temp);
	if (ret)
		return ret;

	wd->temp = temp;
	dev_dbg(pwr->mfd->dev,  "Temperature = %d\n", wd->temp);
	bd71827_adjust_coulomb_count(pwr, wd, 1);
	bd71827_reset_coulomb_count(pwr, wd);
	ret = read_cc(pwr, &cc);
	if (ret)
		return ret;

	wd->coulomb_cnt = cc;
	dev_err(pwr->mfd->dev,  "%s() coulomb_cnt = %d\n", __func__, cc);
	/* If we boot up with CC stopped and both REX and FULL CC being 0
	 * - then the bd71827_adjust_coulomb_count and
	 * bd71827_reset_coulomb_count wont start CC. Just start CC here for
	 * now to mimic old operation where bd71827_calc_soc_org did
	 * always stop and start cc.
	 */
	start_cc(pwr);
	sorg = bd71827_calc_soc_org(wd->coulomb_cnt, wd->designed_cap);
	if (sorg > 100)
		limit_cc(pwr, wd, &sorg);

	wd->soc_norm = sorg;
	wd->soc = wd->soc_norm;
	wd->clamp_soc = wd->soc;
	dev_dbg(pwr->mfd->dev,  "%s() CC_CCNTD = %d\n",
		__func__, wd->coulomb_cnt);
	dev_dbg(pwr->mfd->dev,  "%s() pwr->soc = %d\n", __func__, wd->soc);
	dev_dbg(pwr->mfd->dev,  "%s() pwr->clamp_soc = %d\n",
		__func__, wd->clamp_soc);

	wd->cycle = battery_cycle;
	wd->curr = 0;
	wd->relax_time = 0;

	update_soc_data(pwr);
	return 0;
}

/* @brief set bd71827 battery parameters
 * @param pwr power device
 * @return 0
 */
static int bd71827_set_battery_parameters(struct bd71827_power *pwr)
{
	//struct bd71827 *mfd = pwr->mfd;
	int i;
	unsigned int value;

	if (use_load_bat_params == 0) {
		switch(gptHWCFG->m_val.bBattery) {

			case 14:	//PR-284983N-1500mA
				MAX_VOLTAGE_DEFAULT = MAX_VOLTAGE_DEFAULT_28;
				MIN_VOLTAGE_DEFAULT = MIN_VOLTAGE_DEFAULT_28_PR284983N;
				//THR_VOLTAGE_DEFAULT = ;

				battery_cap_mah = BATTERY_CAP_MAH_DEFAULT_28_PR284983N;
				dgrd_cyc_cap = DGRD_CYC_CAP_DEFAULT_28_PR284983N;
				soc_est_max_num = SOC_EST_MAX_NUM_DEFAULT_28_PR284983N;
				dgrd_temp_cap_h = DGRD_TEMP_CAP_H_DEFAULT_28_PR284983N;
				dgrd_temp_cap_m = DGRD_TEMP_CAP_M_DEFAULT_28_PR284983N;
				dgrd_temp_cap_l = DGRD_TEMP_CAP_L_DEFAULT_28_PR284983N;			
				for (i = 0; i < 23; i++) {
					soc_table[i] = soc_table_default[i];
					ocv_table[i] = ocv_table_28_PR284983N[i];
					vdr_table_h[i] = vdr_table_h_28_PR284983N[i];
					vdr_table_m[i] = vdr_table_m_28_PR284983N[i];
					vdr_table_l[i] = vdr_table_l_28_PR284983N[i];
					vdr_table_vl[i] = vdr_table_vl_28_PR284983N[i];
				}
				break;
			case 21: // 1200mAx2 .
				MAX_VOLTAGE_DEFAULT = MAX_VOLTAGE_DEFAULT_PT158098;
				MIN_VOLTAGE_DEFAULT = MIN_VOLTAGE_DEFAULT_PT158098;
				THR_VOLTAGE_DEFAULT = THR_VOLTAGE_DEFAULT_PT158098;
				//battery_cap_mah = BATTERY_CAP_MAH_DEFAULT;
				battery_cap_mah = BATTERY_CAP_MAH_PT158098;
				//dgrd_cyc_cap = DGRD_CYC_CAP_DEFAULT;
				dgrd_cyc_cap = DGRD_CYC_CAP_PT158098;
				soc_est_max_num = SOC_EST_MAX_NUM_DEFAULT;
				dgrd_temp_cap_h = DGRD_TEMP_CAP_H_DEFAULT;
				dgrd_temp_cap_m = DGRD_TEMP_CAP_M_DEFAULT;
				dgrd_temp_cap_l = DGRD_TEMP_CAP_L_DEFAULT;
				for (i = 0; i < 23; i++) {
					//ocv_table[i] = ocv_table_default[i];
					soc_table[i] = soc_table_default[i];
					//vdr_table_h[i] = vdr_table_h_default[i];
					//vdr_table_m[i] = vdr_table_m_default[i];
					//vdr_table_l[i] = vdr_table_l_default[i];
					//vdr_table_vl[i] = vdr_table_vl_default[i];
					ocv_table[i] = ocv_table_PT158098[i];
					vdr_table_h[i] = vdr_table_h_PT158098[i];
					vdr_table_m[i] = vdr_table_m_PT158098[i];
					vdr_table_l[i] = vdr_table_l_PT158098[i];
					vdr_table_vl[i] = vdr_table_vl_PT158098[i];
				}
				curr_90_max = SOC_90_MAX_CURR_PT158098;
				memcpy(&calc_soc_table, &calc_soc_table_PT158098, sizeof(calc_soc_table_PT158098));
				break;
			case 26: // 2050mA .
				MAX_VOLTAGE_DEFAULT = MAX_VOLTAGE_DEFAULT_EVE188595QH;
				MIN_VOLTAGE_DEFAULT = MIN_VOLTAGE_DEFAULT_EVE188595QH;
				THR_VOLTAGE_DEFAULT = THR_VOLTAGE_DEFAULT_EVE188595QH;
				battery_cap_mah = BATTERY_CAP_MAH_EVE188595QH;
				dgrd_cyc_cap = DGRD_CYC_CAP_EVE188595QH;
				soc_est_max_num = SOC_EST_MAX_NUM_DEFAULT;
				dgrd_temp_cap_h = DGRD_TEMP_CAP_H_DEFAULT;
				dgrd_temp_cap_m = DGRD_TEMP_CAP_M_DEFAULT;
				dgrd_temp_cap_l = DGRD_TEMP_CAP_L_DEFAULT;
				for (i = 0; i < 23; i++) {
					soc_table[i] = soc_table_default[i];
					ocv_table[i] = ocv_table_EVE188595QH[i];
					vdr_table_h[i] = vdr_table_h_EVE188595QH[i];
					vdr_table_m[i] = vdr_table_m_EVE188595QH[i];
					vdr_table_l[i] = vdr_table_l_EVE188595QH[i];
					vdr_table_vl[i] = vdr_table_vl_EVE188595QH[i];
				}
				curr_90_max = SOC_90_MAX_CURR_EVE188595QH;
				memcpy(&calc_soc_table, &calc_soc_table_EVE188595QH, sizeof(calc_soc_table_EVE188595QH));
				break;
		}
	}

	for (i = 0; i < 23; i++)
		soc_table[i] = soc_table_default[i];
	
	curr_90_min = calc_soc_table[0].curr * 1000;
	curr_90_avg = (curr_90_max - curr_90_min) / 5;

	charge_battery_cap = mAh_A10s(battery_cap_mah, pwr->charge_curr_factor);
	discharge_battery_cap = mAh_A10s(battery_cap_mah, pwr->discharge_curr_factor);
	battery_cap = discharge_battery_cap;
	smp_wmb(); /* wait for sync */

   if(gptHWCFG->m_val.bPCB == 111) {
       //change charge voltage to 4.35V
       value = 0x44;
       regmap_write(pwr->mfd->regmap, 0x7D, value);
       regmap_write(pwr->mfd->regmap, 0x7E, value);
       regmap_write(pwr->mfd->regmap, 0x7F, value);
       value = 0x64;
       regmap_write(pwr->mfd->regmap, 0x81, value);
   }


	return 0;
}

static void update_soc_data(struct bd71827_power *pwr)
{
	spin_lock(&pwr->dlock);
	pwr->d_r = pwr->d_w;
	spin_unlock(&pwr->dlock);
}

int bd71828_charge_rsense_update(int mode)
{
	static int flag=0;
	struct bd71827_power *pwr;
	struct bd7182x_soc_data *wd;
	int ret;
	u32 bcap;
	int soc;
	
#if 0
	int ocv;
#endif

	if(!gPwr_dev)
		return -1;

	pwr = platform_get_drvdata(gPwr_dev);
	if(!flag) {
		dev_info(pwr->mfd->dev, "\n>>>>> Call dpidle_ctrl\n");
		dpidle_extern_ctrl("spm_f26m_req 0x1", strlen("spm_f26m_req 0x1"));
		flag = 1;
	}
	
	wd = &pwr->d_w;
	if (pwr->charger_type != BD_71879_CHARGER_TYPE_NONE) {
		dev_info(pwr->mfd->dev, "Change battery charge rsense setting\n");
		pwr->curr_factor = pwr->charge_curr_factor;
		battery_cap = charge_battery_cap;
		wd->designed_cap = battery_cap;
		wd->full_cap = battery_cap;
	} else {
		dev_info(pwr->mfd->dev, "Change battery discharge rsense setting\n");
		pwr->curr_factor = pwr->discharge_curr_factor;
		battery_cap = discharge_battery_cap;
		wd->designed_cap = battery_cap;
		wd->full_cap = battery_cap;
	}
	
	/* Stop Coulomb Counter */
	/*ret = stop_cc(pwr);
	if (ret)
		return ret;*/

	ret = bd7182x_write16(pwr, pwr->regs->batcap_mon_limit_u,
			      battery_cap * 9 / 10);
	if (ret)
		return ret;

	/* Set Battery Capacity Monitor threshold1 as 90% */
	dev_dbg(pwr->mfd->dev, "BD71827_REG_CC_BATCAP1_TH = %d\n",
		(battery_cap * 9 / 10));
	
	soc = wd->soc;
	
	bcap = wd->designed_cap * soc / 100;
	
	ret = update_cc(pwr, bcap + wd->designed_cap / 200);
	if (ret)
		return ret;

	dev_err(pwr->mfd->dev,
		"Adjust Coulomb Counter by SW at Charge state change\n");
	dev_err(pwr->mfd->dev, "CC_CCNTD = %d\n", wd->coulomb_cnt);
	
	//start_cc(pwr);
	
	return 0;
}

static void usb_work_callback(struct work_struct *work)
{
	struct bd71827_power *pwr;
	struct delayed_work *delayed_work;
	struct bd7182x_soc_data *wd;
	int ret;
	int status;

	delayed_work = container_of(work, struct delayed_work, work);
	pwr = container_of(delayed_work, struct bd71827_power, usb_work);
	wd = &pwr->d_w;
	
	if (suspend_status) {
		schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(500));
		return;
	}

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->dcin_stat, &status);
	if (ret)
		goto err_out;

	status &= BD7182x_MASK_DCIN_STAT;
	if (status != wd->vbus_status/* && pwr->pre_charger_type != pwr->charger_type*/) {
		dev_dbg(pwr->mfd->dev, "DCIN_STAT CHANGED from 0x%X to 0x%X\n",
			wd->vbus_status, status);

		wd->vbus_status = status;
		//pwr->pre_charger_type = pwr->charger_type;
		//schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(0));
	}

	bd71828_charger_detect();
	if (wd->vbus_status & BD7182x_MASK_DCIN_STAT) {
		if(pwr->charger_type == BD_71879_CHARGER_TYPE_SDP) {
			printk(KERN_ERR"%s()-[%d] mt_usb_connect!\n", __func__, __LINE__);
			mt_usb_connect();
		}
	}
	else {
		printk(KERN_ERR"%s()-[%d] mt_usb_disconnect!\n", __func__, __LINE__);
		mt_usb_disconnect();
	}
	pwr->pre_charger_type = pwr->charger_type;
	bd71828_charge_rsense_update(pwr->charger_type);
	battery_charge_current(pwr->charger_type);
	usb_plug_handler(NULL);

err_out:
	return ;
}

/* @brief timed work function called by system
 *  read battery capacity,
 *  sense change of charge status, etc.
 * @param work work struct
 * @return  void
 */

static void bd_work_callback(struct work_struct *work)
{
	struct bd71827_power *pwr;
	struct delayed_work *delayed_work;
	int status, changed = 0, ret;
	unsigned int sorg;
	static int cap_counter;
	const char *errstr = "DCIN status reading failed";
	struct bd7182x_soc_data *wd;
	struct rohm_rtc current_rtc;
	struct rtc_time current_time;
	static int giVBUS_status =-1;

	delayed_work = container_of(work, struct delayed_work, work);
	pwr = container_of(delayed_work, struct bd71827_power, bd_work);
	wd = &pwr->d_w;

	dev_dbg(pwr->mfd->dev, "%s(): in\n", __func__);
	
	if (suspend_status) {
		schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(500));
		return;
	}

	if(giVBUS_status != wd->vbus_status) {
		changed = 1;
		giVBUS_status = wd->vbus_status;
	}

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->bat_stat, &status);

	errstr = "battery status reading failed";
	if (ret)
		goto err_out;

	status &= BD7182x_MASK_BAT_STAT;
	status &= ~BAT_DET_DONE;
	if (status != wd->bat_status) {
		dev_dbg(pwr->mfd->dev, "BAT_STAT CHANGED from 0x%X to 0x%X\n",
			wd->bat_status, status);
		if((wd->clamp_soc < wd->soc_norm) && (wd->vbus_status==0) && (status==1))
			sync_cc(pwr, wd, wd->clamp_soc);
		wd->bat_status = status;
		changed = 1;
	}

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->chg_state, &status);
	errstr = "Charger state reading failed";
	if (ret)
		goto err_out;

	status &= BD7182x_MASK_CHG_STATE;

	if (status != wd->charge_status) {
		dev_dbg(pwr->mfd->dev, "CHG_STATE CHANGED from 0x%X to 0x%X\n",
			wd->charge_status, status);
		wd->charge_status = status;
	}
	ret = bd71827_get_voltage_current(pwr, wd);
	errstr = "Failed to get current voltage";
	if (ret)
		goto err_out;

	errstr = "Failed to adjust coulomb count";
	ret = bd71827_adjust_coulomb_count(pwr, wd, 0);
	if (ret)
		goto err_out;

	errstr = "Failed to reset coulomb count";
	ret = bd71827_reset_coulomb_count(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to adjust coulomb count (sw)";
	ret = bd71827_adjust_coulomb_count_sw(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to get coulomb count";
	ret = bd71827_coulomb_count(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to perform update cycle";
	ret = bd71827_update_cycle(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to calculate full capacity";
	ret = bd71827_calc_full_cap(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to calculate org state of charge";
	sorg = bd71827_calc_soc_org(wd->coulomb_cnt, wd->designed_cap);
	if (sorg > 100)
		ret = limit_cc(pwr, wd, &sorg);
	if (ret)
		goto err_out;

	errstr = "Failed to calculate norm state of charge";
	ret = bd71827_calc_soc_norm(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to calculate state of charge";
	ret = bd71827_calc_soc(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to calculate clamped state of charge";
	ret = bd71827_calc_soc_clamp(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to get charger online status";
	ret = bd71827_get_online(pwr, wd);
	if (ret)
		goto err_out;

	errstr = "Failed to get charger state";
	ret = bd71827_charge_status(pwr, wd, 1);
	if (ret)
		goto err_out;

	errstr = "Failed to record rtc time.";
	ret = bd71827_record_rtc(pwr, wd);
	if (ret)
		goto err_out;

	if(pwr->suspend_resume) {
		pwr->suspend_resume = 0;
		changed = 1;
	}
	//if(pwr->change_charger)
	//	pwr->change_charger = 0;

	if (changed || cap_counter++ > JITTER_REPORT_CAP / JITTER_DEFAULT) {
		power_supply_changed(pwr->ac);
		power_supply_changed(pwr->bat);
		cap_counter = 0;
	}
	ret = regmap_bulk_read(pwr->mfd->regmap, BD71828_REG_RTC_SEC, &current_rtc, sizeof(current_rtc));
	rtc2time(&current_rtc, &current_time);
	pwr->update_seconds = rtc_tm_to_time64(&current_time);

printk(KERN_DEBUG"bd71827-power %s():soc(c=%d,v=%d,clamp_soc=%d),vbat(stat=(%d,%d),v=%d,vmin=%d),vsys(v=%d,vmin=%d),curr=%d(avg=%d),cc=%d,dcap=%d,fcap=%d\n",
 __func__,
 wd->soc_norm,wd->soc,wd->clamp_soc,
 wd->bat_status,wd->rpt_status,wd->vcell,wd->vcell_min,
 wd->vsys,wd->vsys_min,
 wd->curr,wd->curr_avg,
 wd->coulomb_cnt,wd->designed_cap,wd->full_cap);

	pwr->gauge_delay = JITTER_DEFAULT;
	if(pwr->schedule_work)
		schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(JITTER_DEFAULT));
	update_soc_data(pwr);
	ciritical_low_count = 0;
	return;
err_out:
	dev_err(pwr->mfd->dev, "fuel-gauge cycle error %d - %s\n", ret,
		(errstr) ? errstr : "Unknown error");
}

/* @brief get property of power supply ac
 * @param psy power supply device
 * @param psp property to get
 * @param val property value to return
 * @retval 0  success
 * @retval negative fail
 */
static int bd71827_charger_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct bd71827_power *pwr = dev_get_drvdata(psy->dev.parent);
	u32 vot;
	u16 tmp;
	int ret;
	//struct bd7182x_soc_data *wr = &pwr->d_r;

	smp_rmb(); /* wait for sync */
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		spin_lock(&pwr->dlock);
		val->intval = pwr->pre_charger_type;//wr->charger_online;
		spin_unlock(&pwr->dlock);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		ret = bd7182x_read16_himask(pwr, pwr->regs->vdcin,
					    BD7182x_MASK_VDCIN_U, &tmp);
		if (ret)
			return ret;

		vot = tmp;
		val->intval = 5000 * vot;		// 5 milli volt steps
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/* @brief get property of power supply bat
 *  @param psy power supply device
 *  @param psp property to get
 *  @param val property value to return
 *  @retval 0  success
 *  @retval negative fail
 */

static int bd71827_battery_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
	struct bd71827_power *pwr = dev_get_drvdata(psy->dev.parent);
	struct bd7182x_soc_data *wr = &pwr->d_r;
	int ret = 0;

	spin_lock(&pwr->dlock);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = wr->rpt_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = wr->bat_health;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (wr->rpt_status == POWER_SUPPLY_STATUS_CHARGING)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = wr->bat_online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = wr->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = wr->clamp_soc;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
	{
		u32 t;

		t = wr->coulomb_cnt >> 16;
		t = A10s_mAh(t, pwr->curr_factor);
		if (t > A10s_mAh(wr->designed_cap, pwr->curr_factor))
			t = A10s_mAh(wr->designed_cap, pwr->curr_factor);
		/* uA to report */
		val->intval = t * 1000;
		break;
	}
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = wr->bat_online;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		val->intval = BATTERY_FULL_DEFAULT *
			      A10s_mAh(wr->designed_cap, pwr->curr_factor) * 10;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = BATTERY_FULL_DEFAULT *
			      A10s_mAh(wr->full_cap, pwr->curr_factor) * 10;
		break;
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		val->intval = wr->curr_avg;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = wr->curr;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = wr->temp * 10; /* 0.1 degrees C unit */
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = MAX_VOLTAGE_DEFAULT;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN:
		val->intval = MIN_VOLTAGE_DEFAULT;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = MAX_CURRENT_DEFAULT;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	spin_unlock(&pwr->dlock);

	return ret;
}

/* @brief ac properties */
static enum power_supply_property bd71827_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

/* @brief bat properies */
static enum power_supply_property bd71827_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MIN,
	POWER_SUPPLY_PROP_CURRENT_MAX,
};

static ssize_t bd71827_sysfs_show_charger_type(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	ssize_t ret;

	smp_rmb(); /* wait for sync */
    switch(pwr->charger_type)
    {
        case BD_71879_CHARGER_TYPE_DCP:
			ret = sprintf(buf, "DCP\n");
            break;
        case BD_71879_CHARGER_TYPE_SDP:
			ret = sprintf(buf, "SDP\n");
            break;
        case BD_71879_CHARGER_TYPE_NONE:
        default:
			ret = sprintf(buf, "NONE\n");
            break;
    }
	return ret;
}

static DEVICE_ATTR(charger_type, 0644,
		bd71827_sysfs_show_charger_type, NULL);

static ssize_t bd71827_sysfs_set_charging(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf,
					   size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	int ret;
	unsigned int val;

	ret = kstrtoint(buf, 0, &val);
	if ((ret != 0) || (val > 1)) {
		dev_warn(dev, "use 0/1 to disable/enable charging, %d\n", ret);
		return -EINVAL;
	}

	if (val == 1)
		ret = regmap_update_bits(pwr->mfd->regmap, pwr->regs->chg_en,
				BD7182x_MASK_CHG_EN, BD7182x_MASK_CHG_EN);
	else
		ret = regmap_update_bits(pwr->mfd->regmap, pwr->regs->chg_en,
				 BD7182x_MASK_CHG_EN, 0);
	if (ret)
		return ret;

	return count;
}

static ssize_t bd71827_sysfs_show_charging(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	int chg_en, ret;

	ret = regmap_read(pwr->mfd->regmap, pwr->regs->chg_en, &chg_en);
	if (ret)
		return ret;

	chg_en &= BD7182x_MASK_CHG_EN;
	smp_rmb(); /* wait for sync */
	return sprintf(buf, "%x\n", pwr->d_w.charger_online && chg_en);
}

static DEVICE_ATTR(charging, 0644,
		bd71827_sysfs_show_charging, bd71827_sysfs_set_charging);

static ssize_t bd71827_sysfs_set_gauge(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf,
					   size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	int ret, delay;

	ret = kstrtoint(buf, 0, &delay);
	if (ret != 0) {
		dev_err(pwr->mfd->dev, "error: write a integer string");
		return -EINVAL;
	}

	if (delay == -1) {
		dev_info(pwr->mfd->dev, "Gauge schedule cancelled\n");
		cancel_delayed_work(&pwr->bd_work);
		return count;
	}

	dev_info(pwr->mfd->dev, "Gauge schedule in %d\n", delay);
	pwr->gauge_delay = delay;
	smp_wmb(); /* wait for sync */
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(delay));

	return count;
}

static ssize_t bd71827_sysfs_show_gauge(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	ssize_t ret;

	smp_rmb(); /* wait for sync */
	ret = sprintf(buf, "Gauge schedule in %d\n",
		      pwr->gauge_delay);
	return ret;
}

static DEVICE_ATTR(gauge, 0644,
		bd71827_sysfs_show_gauge, bd71827_sysfs_set_gauge);
		
static ssize_t bd71827_sysfs_show_register(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct bd71827_power *pwr = power_supply_get_drvdata(psy);
	int temp,count,offset=0;
	ssize_t ret=0;

	smp_rmb();

	sprintf(&buf[0], "00: ");
	offset = 4;
	for (count=0;count<0x100;count++) {
		regmap_read(pwr->mfd->regmap, count, &temp);
		if((count+1)%16==0 && count && ((count+1)/16)<16)
			ret = sprintf(&buf[offset], "%02x \n%02d: ", temp, (count+1)/16);
		else if((count+1)%16==0 && count && ((count+1)/16)==16)
			ret = sprintf(&buf[offset], "%02x \n", temp);
		else
			ret = sprintf(&buf[offset], "%02x ", temp);
		offset += ret;
	}

	return offset;
}

static DEVICE_ATTR(register, S_IWUSR | S_IRUGO,
		bd71827_sysfs_show_register, NULL);

static ssize_t batt_params_read(struct device *dev, struct device_attribute *attr,char *buf)
{
	int len = 0,total_len=0;
	char *pcBuf = buf;
	int i;

	len = sprintf(pcBuf,"Battery=%d (0x%02x)\n",gptHWCFG->m_val.bBattery,gptHWCFG->m_val.bBattery);
	pcBuf += len;
	total_len += len;

	len = sprintf(pcBuf,"ocv_table[] = {\n");pcBuf += len;total_len += len;
	for(i=0;i<23;i++) {
			len = sprintf(pcBuf,"[%d] %d uV\n",i,ocv_table[i]);pcBuf += len;total_len += len;
	}
	len = sprintf(pcBuf,"}\n");pcBuf += len;total_len += len;

	len = sprintf(pcBuf,"0%% = %d uV\n",MIN_VOLTAGE_DEFAULT);pcBuf += len;total_len += len;
	len = sprintf(pcBuf,"100%% = %d uV\n",MAX_VOLTAGE_DEFAULT);pcBuf += len;total_len += len;
	len = sprintf(pcBuf,"Battery_Full_Cap=%d mAh\n",BATTERY_CAP_MAH_DEFAULT);pcBuf += len;total_len += len;
	
	len = sprintf(pcBuf,"charging curr 90%% = %d mA\n",curr_90_max);pcBuf += len;total_len += len;
	len = sprintf(pcBuf,"charging curr 90%% avg = %d uA\n",curr_90_avg);pcBuf += len;total_len += len;
	len = sprintf(pcBuf,"calc_soc_table[] = {\n");pcBuf += len;total_len += len;
	for(i=0;i<6;i++) {
			len = sprintf(pcBuf,"[%d] %d = %d mA\n",i,calc_soc_table[i].soc,calc_soc_table[i].curr);pcBuf += len;total_len += len;
	}
	
	

	return total_len;
}

static DEVICE_ATTR(batt_params, S_IRUGO, batt_params_read, NULL);

static struct attribute *bd71827_sysfs_attributes[] = {
	&dev_attr_charger_type.attr,
	&dev_attr_charging.attr,
	&dev_attr_gauge.attr,
	&dev_attr_register.attr,
	&dev_attr_batt_params.attr,
	NULL,
};

static const struct attribute_group bd71827_sysfs_attr_group = {
	.attrs = bd71827_sysfs_attributes,
};

/* @brief powers supplied by bd71827_ac */
static char *bd71827_ac_supplied_to[] = {
	BAT_NAME,
};

static const struct power_supply_desc bd71827_ac_desc = {
	.name		= AC_NAME,
	.type		= POWER_SUPPLY_TYPE_MAINS,
	.properties	= bd71827_charger_props,
	.num_properties	= ARRAY_SIZE(bd71827_charger_props),
	.get_property	= bd71827_charger_get_property,
};

static const struct power_supply_desc bd71827_battery_desc = {
	.name		= BAT_NAME,
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties	= bd71827_battery_props,
	.num_properties	= ARRAY_SIZE(bd71827_battery_props),
	.get_property	= bd71827_battery_get_property,
};

#ifdef PWRCTRL_HACK
/* This is not-so-pretty hack for allowing external code to call
 * bd71827_chip_hibernate() without this power device-data
 */
static struct bd71827_power *hack;
static DEFINE_SPINLOCK(pwrlock);

static struct bd71827_power *get_power(void)
{
	mutex_lock(&pwrlock);
	if (!hack) {
		mutex_unlock(&pwrlock);
		return -ENOENT;
	}
	return hack;
}

static void put_power(void)
{
	mutex_unlock(&pwrlock);
}

static int set_power(struct bd71827_power *pwr)
{
	mutex_lock(&pwrlock);
	hack = pwr;
	mutex_unlock(&pwrlock);
}

static void free_power(void)
{
	mutex_lock(&pwrlock);
	hack = NULL;
	mutex_unlock(&pwrlock);
}

/* called from pm inside machine_halt */
void bd71827_chip_hibernate(void)
{
	struct bd71827_power *pwr = get_power();

	if (IS_ERR(pwr)) {
		pr_err("bd71827_chip_hibernate called before probe finished\n");
		return PTR_ERR(pwr);
	}

	/* programming sequence in EANAB-151 */
	regmap_update_bits(pwr->mfd->regmap, pwr->regs->pwrctrl,
			   pwr->regs->hibernate_mask, 0);
	regmap_update_bits(pwr->mfd->regmap, pwr->regs->pwrctrl,
			   pwr->regs->hibernate_mask,
			   pwr->regs->hibernate_mask);
	put_power();
}
#endif

#define RSENS_CURR 10000000000LLU
static int bd7182x_set_chip_specifics(struct bd71827_power *pwr, int chg_rsens_ohm, int dischg_rsens_ohm)
{
	u64 tmp = RSENS_CURR;
	u64 dis_tmp = RSENS_CURR;

	switch (pwr->mfd->chip_type) {
	case ROHM_CHIP_TYPE_BD71828:
		pwr->regs = &pwr_regs_bd71828;
		pwr->get_temp = bd71828_get_temp;
		break;
	case ROHM_CHIP_TYPE_BD71827:
		pwr->regs = &pwr_regs_bd71827;
		pwr->get_temp = bd71827_get_temp;
		dev_warn(pwr->mfd->dev, "BD71817 not tested\n");
		break;
	default:
		dev_err(pwr->mfd->dev, "Unknown PMIC\n");
		return -EINVAL;
	}

	/* Reg val to uA */
	do_div(tmp, chg_rsens_ohm);

	pwr->curr_factor = tmp;
	pr_info("Setting curr-factor to %u\n", pwr->curr_factor);
	
	pwr->charge_curr_factor = tmp;
	pr_info("Setting charge-curr-factor to %u\n", pwr->charge_curr_factor);
	
	/* Reg val to uA */
	do_div(dis_tmp, dischg_rsens_ohm);
	
	pwr->discharge_curr_factor = dis_tmp;
	pr_info("Setting discharge-curr-factor to %u\n", pwr->discharge_curr_factor);
	
	return 0;
}

void bd71828_charger_detect(void)
{
	struct bd71827_power *pwr = platform_get_drvdata(gPwr_dev);
	struct bd7182x_soc_data *wd;
	int iChk;
	
	wd = &pwr->d_w;

	if(wd->vbus_status&BD7182x_MASK_DCIN_STAT) {
		iChk = ntx_charger_cc_detect(0);
		pr_debug("cc detect result %d \n",iChk);
		if( (iChk<0) || (NO_CHARGER_PLUGGED==iChk) ) {
#if 0
                       iChk = SDP_CHARGER;
#else
			// no cc detector or detect nothing .
			iChk = ntx_charger_gadget_detect(3000);
			if(iChk<0) {
				pr_info("gadget detector not exist !? not implemented !? \n");
			}
			else {
				pr_debug("%s(%d):gadget detect result = %d\n",__func__,__LINE__,iChk);
			}
#endif
		}
		if( (iChk<0) || (NO_CHARGER_PLUGGED==iChk) ) {
			pwr->charger_type = BD_71879_CHARGER_TYPE_SDP;
		}
		else {
			switch(iChk) {
			case DCP_CHARGER:
				pwr->charger_type = BD_71879_CHARGER_TYPE_DCP;
				break;
			default:
			case SDP_OVRLIM_CHARGER:
			case SDP_CHARGER:
			case CDP_CHARGER:
				pwr->charger_type = BD_71879_CHARGER_TYPE_SDP;
				break;
			}
		}
	}
	else {
		pwr->charger_type = BD_71879_CHARGER_TYPE_NONE;
	}
}

int bd71828_get_charger_detect(void)
{
	struct bd71827_power *pwr = platform_get_drvdata(gPwr_dev);

    switch(pwr->charger_type)
    {
        case BD_71879_CHARGER_TYPE_NONE:
            return NO_CHARGER_PLUGGED;
        case BD_71879_CHARGER_TYPE_DCP:
            return DCP_CHARGER;
        case BD_71879_CHARGER_TYPE_SDP:
            return SDP_CHARGER;
        default:
            dev_info(pwr->mfd->dev, "Undefined charger type\n");
            return SDP_CHARGER;
    }
}


#if 0
static irqreturn_t bd7182x_short_push(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	kobject_uevent(&pwr->mfd->dev->kobj, KOBJ_OFFLINE);
	dev_info(pwr->mfd->dev, "POWERON_SHORT\n");

	return IRQ_HANDLED;
}
#endif
/*
static irqreturn_t bd7182x_long_push(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	kobject_uevent(&pwr->mfd->dev->kobj, KOBJ_OFFLINE);
	dev_info(pwr->mfd->dev, "POWERON_LONG\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd7182x_mid_push(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	kobject_uevent(&pwr->mfd->dev->kobj, KOBJ_OFFLINE);
	dev_info(pwr->mfd->dev, "POWERON_MID\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd7182x_push(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	kobject_uevent(&pwr->mfd->dev->kobj, KOBJ_ONLINE);
	dev_info(pwr->mfd->dev, "POWERON_PRESS\n");

	return IRQ_HANDLED;
}
*/
static irqreturn_t bd7182x_dcin_removed(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~DCIN removed\n");

	cancel_delayed_work_sync(&pwr->bd_work);
	pwr->suspend_resume = 1;
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(1000));
	//pwr->charger_type = BD_71879_CHARGER_TYPE_NONE;
	//usb_plug_handler(NULL);

	cancel_delayed_work_sync(&pwr->usb_work);
	schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(500));

	return IRQ_HANDLED;
}

static irqreturn_t bd7182x_dcin_detected(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~DCIN inserted\n");

	cancel_delayed_work_sync(&pwr->bd_work);
	pwr->suspend_resume = 1;
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(1000));
	//pwr->charger_type = BD_71879_CHARGER_TYPE_SDP;
	//usb_plug_handler(NULL);

	cancel_delayed_work_sync(&pwr->usb_work);
	schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(1000));

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_vbat_low_res(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	if (time_is_after_jiffies(pwr->boot_jiffies + msecs_to_jiffies(10000)))
		return IRQ_HANDLED;
	dev_info(pwr->mfd->dev, "\n~~~ VBAT LOW Resumed ...\n");
	return IRQ_HANDLED;
}

static irqreturn_t bd71827_vbat_low_det(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	if (time_is_after_jiffies(pwr->boot_jiffies + msecs_to_jiffies(10000)))
		return IRQ_HANDLED;
	dev_info(pwr->mfd->dev, "\n~~~ VBAT LOW Detected ...\n");
	if(pwr->d_r.clamp_soc == 0) {
		power_supply_changed(pwr->bat);
		printk(KERN_INFO "PMU:%s Set ciritical_low_flag = 1 **********\n", __func__);
	}
	ciritical_low_count++;
	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_bat_hi_det(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ Overtemp Detected ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_bat_hi_res(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ Overtemp Resumed ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_bat_low_det(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ Lowtemp Detected ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_bat_low_res(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ Lowtemp Resumed ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_vf_det(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ VF Detected ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_vf_res(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ VF Resumed ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_vf125_det(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ VF125 Detected ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_temp_vf125_res(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ VF125 Resumed ...\n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_charger_done(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	cancel_delayed_work_sync(&pwr->bd_work);
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(0));
	dev_info(pwr->mfd->dev, "\n~~~ CHARGER DONE ... \n");

	return IRQ_HANDLED;
}

static irqreturn_t bd71827_charger_wdg_expired(int irq, void *data)
{
	struct bd71827_power *pwr = (struct bd71827_power *)data;

	dev_info(pwr->mfd->dev, "\n~~~ CHARGER WHDOG EXPIRED ... \n");
	resetBatteryCharger(pwr);

	return IRQ_HANDLED;
}

struct bd7182x_irq_res {
	const char *name;
	irq_handler_t handler;
};

#define BDIRQ(na, hn) { .name = (na), .handler = (hn) }

int bd7182x_get_irqs(struct platform_device *pdev, struct bd71827_power *pwr)
{
	int i, irq, ret;
	static const struct bd7182x_irq_res irqs[] = {
/*		BDIRQ("bd71828-pwr-longpush", bd7182x_long_push),
		BDIRQ("bd71828-pwr-midpush", bd7182x_mid_push),
		BDIRQ("bd71828-pwr-shortpush", bd7182x_short_push),
		BDIRQ("bd71828-pwr-push", bd7182x_push), */
		BDIRQ("bd71828-pwr-dcin-in", bd7182x_dcin_detected),
		BDIRQ("bd71828-pwr-dcin-out", bd7182x_dcin_removed),
		BDIRQ("bd71828-vbat-normal", bd71827_vbat_low_res),
		BDIRQ("bd71828-vbat-low", bd71827_vbat_low_det),
		BDIRQ("bd71828-btemp-hi", bd71827_temp_bat_hi_det),
		BDIRQ("bd71828-btemp-cool", bd71827_temp_bat_hi_res),
		BDIRQ("bd71828-btemp-lo", bd71827_temp_bat_low_det),
		BDIRQ("bd71828-btemp-warm", bd71827_temp_bat_low_res),
		BDIRQ("bd71828-temp-hi", bd71827_temp_vf_det),
		BDIRQ("bd71828-temp-norm", bd71827_temp_vf_res),
		BDIRQ("bd71828-temp-125-over", bd71827_temp_vf125_det),
		BDIRQ("bd71828-temp-125-under", bd71827_temp_vf125_res),
		BDIRQ("bd71828-charger-done", bd71827_charger_done),
		BDIRQ("bd71828-charger-wdg-expired", bd71827_charger_wdg_expired),
	};

	for (i = 0; i < ARRAY_SIZE(irqs); i++) {
		irq = platform_get_irq_byname(pdev, irqs[i].name);

		ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
						irqs[i].handler, 0,
						irqs[i].name, pwr);
		if (ret)
			break;
	}

	return ret;
}

#define RSENS_DEFAULT_30MOHM 30000000

int dt_get_rsens(struct device *dev, int *chg_rsens_ohm, int *dischg_rsens_ohm)
{
	if (dev->of_node) {
		int ret;
		unsigned int rs;

		ret = of_property_read_u32(dev->of_node,
			"rohm,charge-sense-resistor", &rs);
		if (ret) {
			ret = of_property_read_u32(dev->of_node,
				"rohm,charger-sense-resistor", &rs);
			if (ret) {
				if (ret == -EINVAL)
					return 0;

				dev_err(dev, "Bad Charge RSENS dt property\n");
				return ret;
			}
		}

		*chg_rsens_ohm = (int)rs;
		
		ret = of_property_read_u32(dev->of_node,
			"rohm,discharge-sense-resistor", &rs);
		if (ret) {
			ret = of_property_read_u32(dev->of_node,
				"rohm,charger-sense-resistor", &rs);
			if (ret) {
				if (ret == -EINVAL)
					return 0;

				dev_err(dev, "Bad DisCharge RSENS dt property\n");
				return ret;
			}
		}
		
		*dischg_rsens_ohm = (int)rs;
	}
	return 0;
}

void battery_stop_schedule(void)
{
	struct bd71827_power *pwr;

	if(!gPwr_dev)
		return;

	pwr = platform_get_drvdata(gPwr_dev);
	pwr->schedule_work = 0;
	//cancel_delayed_work_sync(&pwr->bd_work);
	cancel_delayed_work(&pwr->bd_work);
}
EXPORT_SYMBOL_GPL(battery_stop_schedule);

static int bd71827_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct bd71827_power *pwr = platform_get_drvdata(pdev);
	struct rohm_rtc current_rtc;
	struct rtc_time current_time;
	time64_t current_seconds;

	regmap_bulk_read(pwr->mfd->regmap, BD71828_REG_RTC_SEC, &current_rtc, sizeof(current_rtc));
	rtc2time(&current_rtc, &current_time);
	current_seconds = rtc_tm_to_time64(&current_time);

	if(delayed_work_pending(&pwr->usb_work))	{
		printk(KERN_ERR"%s() charge detect work is pending !\n",__func__);
		return -1;
	}

	if(work_busy(&pwr->usb_work.work)) {
		printk(KERN_ERR"%s() charge detect work is busy !\n",__func__);
		return -1;
	}

	if((pwr->update_seconds < current_seconds) && ((pwr->update_seconds+1800) < current_seconds)) {
	//if(pwr->suspend_resume/* || pwr->change_charger*/) {
		printk(KERN_ERR"%s() wait one time call for schedule work!!!\n",__func__);
		return -1;
	}
	cancel_delayed_work_sync(&pwr->bd_work);
	suspend_status = 1;

	if (time_is_after_jiffies(pwr->resume_jiffies + msecs_to_jiffies(1000))) {
		printk(KERN_ERR"%s() wait resume_jiffies !!!\n",__func__);
		return -1;
	}
	pwr->suspend_resume = 0;
	return 0;
};

static int bd71827_power_resume(struct platform_device *pdev)
{
	struct bd71827_power *pwr = platform_get_drvdata(pdev);

	pwr->resume_jiffies = jiffies;
	pwr->suspend_resume = 1;
	cancel_delayed_work_sync(&pwr->bd_work);
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(500));
	suspend_status = 0;
	return 0;
};

/* @brief probe pwr device
 * @param pdev platform device of bd71827_power
 * @retval 0 success
 * @retval negative fail
 */
static int bd71827_power_probe(struct platform_device *pdev)
{
//	struct bd71827 *bd71827 = dev_get_drvdata(pdev->dev.parent);
	struct rohm_regmap_dev *mfd;
	struct bd71827_power *pwr;
	struct power_supply_config ac_cfg = {};
	struct power_supply_config bat_cfg = {};
	int ret;
	int charge_rsens_ohm = RSENS_DEFAULT_30MOHM;
	int discharge_rsens_ohm = RSENS_DEFAULT_30MOHM;

	mfd = dev_get_drvdata(pdev->dev.parent);

	pwr = devm_kzalloc(&pdev->dev, sizeof(*pwr), GFP_KERNEL);
	if (!pwr)
		return -ENOMEM;

	pwr->charger_type = BD_71879_CHARGER_TYPE_NONE;
	pwr->pre_charger_type = BD_71879_CHARGER_TYPE_NONE;
	pwr->resume_jiffies = jiffies;
	pwr->calc_soc_jiffies = jiffies;
	pwr->suspend_resume = 0;
	pwr->change_charger = 0;
	pwr->schedule_work = 1;
	pwr->mfd = mfd;
	pwr->mfd->dev = &pdev->dev;
	spin_lock_init(&pwr->dlock);

	ret = dt_get_rsens(pdev->dev.parent, &charge_rsens_ohm, &discharge_rsens_ohm);
	if (ret)
		return ret;

	dev_info(pwr->mfd->dev, "Charge RSENS prop found %u\n", charge_rsens_ohm);
	dev_info(pwr->mfd->dev, "DisCharge RSENS prop found %u\n", discharge_rsens_ohm);

	ret = bd7182x_set_chip_specifics(pwr, charge_rsens_ohm, discharge_rsens_ohm);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, pwr);

	if (battery_cycle <= 0)
		battery_cycle = 0;

	dev_info(pwr->mfd->dev, "battery_cycle = %d\n", battery_cycle);

	/* If the product often power up/down and the power down
	 * time is long, the Coulomb Counter may have a drift.
	 */
	/* If so, it may be better accuracy to enable Coulomb Counter
	 * using following commented out code
	 */
	/* for counting Coulomb when the product is power up(including
	 * sleep).
	 */
	/* The condition  */
	/* (1) Product often power up and down, the power down time
	 * is long and there is no power consumed in power down time.
	 */
	/* (2) Kernel must call this routin at power up time. */
	/* (3) Kernel must call this routin at charging time. */
	/* (4) Must use this code with "Stop Coulomb Counter"
	 * code in bd71827_power_remove() function
	 */
	/* Start Coulomb Counter */
	/* bd71827_set_bits(pwr->mfd, pwr->regs->coulomb_ctrl,
	 * BD7182x_MASK_CCNTENB);
	 */

	bd71827_set_battery_parameters(pwr);

	bd71827_init_hardware(pwr, &pwr->d_w);

	bat_cfg.drv_data = pwr;
	pwr->bat = devm_power_supply_register(&pdev->dev, &bd71827_battery_desc,
					 &bat_cfg);
	if (IS_ERR(pwr->bat)) {
		ret = PTR_ERR(pwr->bat);
		dev_err(&pdev->dev, "failed to register bat: %d\n", ret);
		return ret;
	}

	ac_cfg.supplied_to = bd71827_ac_supplied_to;
	ac_cfg.num_supplicants = ARRAY_SIZE(bd71827_ac_supplied_to);
	ac_cfg.drv_data = pwr;
	pwr->ac = devm_power_supply_register(&pdev->dev,
		&bd71827_ac_desc, &ac_cfg);
	if (IS_ERR(pwr->ac)) {
		ret = PTR_ERR(pwr->ac);
		dev_err(&pdev->dev, "failed to register ac: %d\n", ret);
		return ret;
	}

	/*ret = bd7182x_get_irqs(pdev, pwr);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQs: %d\n", ret);
		return ret;
	};*/

	/* Configure wakeup capable */
	device_set_wakeup_capable(pwr->mfd->dev, 1);
	device_set_wakeup_enable(pwr->mfd->dev, 1);

	ret = sysfs_create_group(&pwr->bat->dev.kobj,
				 &bd71827_sysfs_attr_group);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to register sysfs interface\n");
		return ret;
	}

	INIT_DELAYED_WORK(&pwr->bd_work, bd_work_callback);
	INIT_DELAYED_WORK(&pwr->usb_work, usb_work_callback);

	/* Schedule timer to check current status */
	pwr->gauge_delay = 0;
	smp_wmb(); /* wait for sync */
	schedule_delayed_work(&pwr->bd_work, msecs_to_jiffies(0));

	{
		printk(KERN_INFO "%s modify the OTP config for battery temperature\n", __func__);
		regmap_write(pwr->mfd->regmap, 0xfe, 0x8c);
		regmap_write(pwr->mfd->regmap, 0xff, 0x01);
		regmap_write(pwr->mfd->regmap_otp, 0x80, 0xa0);		//T3 fall
		regmap_write(pwr->mfd->regmap_otp, 0x81, 0xa0);		//T3 rise
		regmap_write(pwr->mfd->regmap_otp, 0x82, 0x9f);		//T4 fall
		regmap_write(pwr->mfd->regmap_otp, 0x83, 0x9d);		//T4 rise
		regmap_write(pwr->mfd->regmap_otp, 0x84, 0x9f);		//T5 fall
		regmap_write(pwr->mfd->regmap_otp, 0x85, 0x9d);		//T5 rise
		regmap_write(pwr->mfd->regmap, 0xfe, 0x8c);
		regmap_write(pwr->mfd->regmap, 0xff, 0x00);
	}

	pwr->boot_jiffies = jiffies;
	gPwr_dev = pdev;

	ntx_charger_register_gadget_detector(check_usb_state_by_ep);
	schedule_delayed_work(&pwr->usb_work, msecs_to_jiffies(5000));

	ret = bd7182x_get_irqs(pdev, pwr);
	if (ret) {
		dev_err(&pdev->dev, "failed to request IRQs: %d\n", ret);
		return ret;
	};

	return 0;
}

/* @brief remove pwr device
 * @param pdev platform device of bd71827_power
 * @return 0
 */

static int bd71827_power_remove(struct platform_device *pdev)
{
	struct bd71827_power *pwr = platform_get_drvdata(pdev);

	/* If the product often power up/down and the power down time
	 * is long, the Coulomb Counter may have a drift.
	 */
	/* If so, it may be better accuracy to disable Coulomb Counter
	 * using following commented out code
	 */
	/* for stopping counting Coulomb when the product is power
	 * down(without sleep).
	 */
	/* The condition  */
	/* (1) Product often power up and down, the power down time
	 * is long and there is no power consumed in power down time.
	 */
	/* (2) Kernel must call this routin at power down time. */
	/* (3) Must use this code with "Start Coulomb Counter"
	 * code in bd71827_power_probe() function
	 */
	/* Stop Coulomb Counter */
	/* bd71827_clear_bits(pwr->mfd,
	 * pwr->regs->coulomb_ctrl, BD7182x_MASK_CCNTENB);
	 */

	sysfs_remove_group(&pwr->bat->dev.kobj, &bd71827_sysfs_attr_group);
	cancel_delayed_work(&pwr->bd_work);

	return 0;
}

static void bd71827_power_shutdown(struct platform_device *pdev)
{
	struct bd71827_power *pwr = platform_get_drvdata(pdev);

	pr_info("%s-%d\n", __func__, __LINE__);
	battery_stop_schedule();
	#if 0
	{
		//keep power off date-time
		unsigned int value;

		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_SEC, &value);
		regmap_write(pwr->mfd->regmap, 0xf4, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_MINUTE, &value);
		regmap_write(pwr->mfd->regmap, 0xf5, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_HOUR, &value);
		regmap_write(pwr->mfd->regmap, 0xf6, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_WEEK, &value);
		regmap_write(pwr->mfd->regmap, 0xf7, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_DAY, &value);
		regmap_write(pwr->mfd->regmap, 0xf8, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_MONTH, &value);
		regmap_write(pwr->mfd->regmap, 0xf9, value);
		regmap_read(pwr->mfd->regmap, BD71828_REG_RTC_YEAR, &value);
		regmap_write(pwr->mfd->regmap, 0xfa, value);
	}
	#endif
	//pr_notice("[%s] %d\n", __func__,__LINE__);
	regmap_write(pwr->mfd->regmap, BD71828_REG_RESETSRC, 0xff);
	mdelay(100);
	//pr_notice("[%s] %d\n", __func__,__LINE__);
	regmap_write(pwr->mfd->regmap, BD71828_REG_INT_DCIN2, 0xff);
	mdelay (100);
	max20342_VCCINTOnBAT();
};

static struct platform_driver bd71827_power_driver = {
	.driver = {
		.name = "bd71827-power",
		.owner = THIS_MODULE,
	},
	.probe = bd71827_power_probe,
	.remove = bd71827_power_remove,
	.suspend = bd71827_power_suspend,
	.resume = bd71827_power_resume,
	.shutdown = bd71827_power_shutdown,
};

module_platform_driver(bd71827_power_driver);

module_param(use_load_bat_params, int, 0444);
MODULE_PARM_DESC(use_load_bat_params, "use_load_bat_params:Use loading battery parameters");

module_param(battery_cap_mah, int, 0444);
MODULE_PARM_DESC(battery_cap_mah, "battery_cap_mah:Battery capacity (mAh)");

module_param(dgrd_cyc_cap, int, 0444);
MODULE_PARM_DESC(dgrd_cyc_cap, "dgrd_cyc_cap:Degraded capacity per cycle (uAh)");

module_param(soc_est_max_num, int, 0444);
MODULE_PARM_DESC(soc_est_max_num, "soc_est_max_num:SOC estimation max repeat number");

module_param(dgrd_temp_cap_h, int, 0444);
MODULE_PARM_DESC(dgrd_temp_cap_h, "dgrd_temp_cap_h:Degraded capacity at high temperature (uAh)");

module_param(dgrd_temp_cap_m, int, 0444);
MODULE_PARM_DESC(dgrd_temp_cap_m, "dgrd_temp_cap_m:Degraded capacity at middle temperature (uAh)");

module_param(dgrd_temp_cap_l, int, 0444);
MODULE_PARM_DESC(dgrd_temp_cap_l, "dgrd_temp_cap_l:Degraded capacity at low temperature (uAh)");

module_param(battery_cycle, uint, 0644);
MODULE_PARM_DESC(battery_parameters, "battery_cycle:battery charge/discharge cycles");

module_param_array(ocv_table, int, NULL, 0444);
MODULE_PARM_DESC(ocv_table, "ocv_table:Open Circuit Voltage table (uV)");

module_param_array(vdr_table_h, int, NULL, 0444);
MODULE_PARM_DESC(vdr_table_h, "vdr_table_h:Voltage Drop Ratio temperatyre high area table");

module_param_array(vdr_table_m, int, NULL, 0444);
MODULE_PARM_DESC(vdr_table_m, "vdr_table_m:Voltage Drop Ratio temperatyre middle area table");

module_param_array(vdr_table_l, int, NULL, 0444);
MODULE_PARM_DESC(vdr_table_l, "vdr_table_l:Voltage Drop Ratio temperatyre low area table");

module_param_array(vdr_table_vl, int, NULL, 0444);
MODULE_PARM_DESC(vdr_table_vl, "vdr_table_vl:Voltage Drop Ratio temperatyre very low area table");

MODULE_AUTHOR("Cong Pham <cpham2403@gmail.com>");
MODULE_DESCRIPTION("ROHM BD71827/BD71828 PMIC Battery Charger driver");
MODULE_LICENSE("GPL");

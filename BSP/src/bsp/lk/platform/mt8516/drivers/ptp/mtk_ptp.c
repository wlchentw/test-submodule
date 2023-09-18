/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly
* prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
*
* BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
* THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
* RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
* AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
* NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
* SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
* SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
* THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
* ACKNOWLEDGES
* THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL
* PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE
* RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO RECEIVER'S
* SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
* RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
* LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
* AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
* OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
* MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* The following software/firmware and/or related documentation
* ("MediaTek Software") have been modified by MediaTek Inc. All revisions are
* subject to any receiver\'s applicable license agreements with MediaTek Inc.
*/

/*=============================================================
 * Include files
 *=============================================================
 */

#include "platform/sec_devinfo.h"
#include "platform/gpu_freq.h"
#include "platform/mt_typedefs.h"
#include "platform/mtk_ptp.h"
#include "platform/pll.h"
#include "platform.h"
#include "platform/spm_mtcmos.h"
#include <platform/pmic.h>
#include "reg.h"
#include "string.h"

/*=============================================================
 * Macro definition
 *=============================================================
 */
#define ARRAY_SIZE(x) (sizeof((x))/(sizeof(x[0])))

#define PTP_LOG_EN		0
#define NR_FREQ			8
#define TEST_FAKE_EFUSE		0
#define NUM_PTP_DET		2

#define DETWINDOW_VAL		0x514
#define PTP_VOLT_TO_PMIC_VAL(volt) \
	((((volt) / 10) - 70000 + 625 - 1) / 625) /* unit: pmic step */
#define PTP_PMIC_VAL_TO_VOLT(pmic) \
	((((pmic) * 625) + 70000) * 10) /* unit: micro voltage */
#define PTPOD_PMIC_OFFSET	(0x10)

#define DTHI_VAL		0x01
#define DTLO_VAL		0xfe
#define DETMAX_VAL		0xffff
#define AGECONFIG_VAL		0x555555
#define AGEM_VAL		0x0
#define DVTFIXED_VAL		0x6
#define VCO_VAL			0x10
#define DCCONFIG_VAL		0x555555

/*
 * bit operation
 */
#undef  BIT
#define BIT(bit)	(1U << (bit))
#define MSB(range)	(1 ? range)
#define LSB(range)	(0 ? range)
#define BITMASK(r)	(((unsigned) -1 >> (31 - MSB(r))) & \
			 ~((1U << LSB(r)) - 1))
#define BITS(r, val)	((val << LSB(r)) & BITMASK(r))

#define for_each_det(det) \
	for (det = &ptp_detectors[PTP_DET_GPUSYS]; \
	     det < (ptp_detectors + ARRAY_SIZE(ptp_detectors)); det++)
#define det_to_id(det)	((det) - &ptp_detectors[0])

#define PERCENT(numerator, denominator) \
	(unsigned char)(((numerator) * 100 + (denominator) - 1) / (denominator))

#define ptp_notice(fmt, args...)	printf("[PTP] " fmt, ##args)

/*
 * REG ACCESS
 */
#define ptp_read(addr)			readl(addr)
#define ptp_read_field(addr, range) \
	((ptp_read(addr) & BITMASK(range)) >> LSB(range))
#define ptp_write(addr, val)		writel(val, addr)
#define ptp_write_field(addr, range, val) \
	ptp_write(addr, (ptp_read(addr) & ~BITMASK(range)) | BITS(range, val))

enum ptp_ctrl_id {
	PTP_CTRL_MCUSYS = 0,
	PTP_CTRL_GPUSYS = 1,
	NR_PTP_CTRL,
};

enum ptp_det_id {
	PTP_DET_MCUSYS = PTP_CTRL_MCUSYS,
	PTP_DET_GPUSYS = PTP_CTRL_GPUSYS,
	NR_PTP_DET,     /* 1 or 2 */
};

static int ptp_gpufreq_get_max_freq(void);
static int set_vcore_to_pmic(enum ptp_det_id det_id, unsigned int target_volt);


/*=============================================================
 * Local type definition
 *=============================================================
 */

enum ptp_phase {
	PTP_PHASE_INIT01,
	PTP_PHASE_INIT02,
	PTP_PHASE_MON,

	NR_PTP_PHASE,
};

enum ptp_features {
	FEA_INIT01	= BIT(PTP_PHASE_INIT01),
	FEA_INIT02	= BIT(PTP_PHASE_INIT02),
	FEA_MON		= BIT(PTP_PHASE_MON),
};

struct ptp_device_info {
	/* M_HW_RES0 */
	unsigned int CPU_BDES		: 8;
	unsigned int CPU_MDES		: 8;
	unsigned int CPU_DCBDET		: 8;
	unsigned int CPU_DCMDET		: 8;

	/* M_HW_RES1 */
	unsigned int CPU_SPEC		: 3;
	unsigned int CPU_Turbo		: 1;
	unsigned int CPU_DVFS_LOW	: 2;
	unsigned int PTPINITEN		: 1; /* CPU_INIT */
	unsigned int PTPMONEN		: 1; /* CPU_MON */
	unsigned int CPU_LEAKAGE	: 8;
	unsigned int CPU_MTDES		: 8;
	unsigned int CPU_AGEDELTA	: 8;

	/* M_HW_RES2 */
	unsigned int GPU_BDES		: 8;
	unsigned int GPU_MDES		: 8;
	unsigned int GPU_DCBDET		: 8;
	unsigned int GPU_DCMDET		: 8;

	/* M_HW_RES3 */
	unsigned int GPU_SPEC		: 3;
	unsigned int GPU_Turbo		: 1;
	unsigned int GPU_DVFS_LOW	: 2;
	unsigned int GPU_INIT		: 1;
	unsigned int GPU_MON		: 1;
	unsigned int GPU_LEAKAGE	: 8;
	unsigned int GPU_MTDES		: 8;
	unsigned int GPU_AGEDELTA	: 8;

	/* M_HW_RES4 */
	unsigned int Lot_ID6		: 2;
	unsigned int Lot_ID5		: 6;
	unsigned int Lot_ID4		: 6;
	unsigned int Lot_ID3		: 6;
	unsigned int Lot_ID2		: 6;
	unsigned int Lot_ID1		: 6;

	/* M_HW_RES5 */
	unsigned int PTPOD5_Y		: 8;
	unsigned int PTPOD5_X		: 8;
	unsigned int PTPOD5_Random	: 1;
	unsigned int PTPOD5_Wfr_ID	: 4;
	unsigned int PTPOD5_Lot_ID7	: 7;
	unsigned int PTPOD5_Lot_ID6	: 4;

	/* PTPOD6 */
	unsigned int PTPOD6_Y		: 8;
	unsigned int PTPOD6_X		: 8;
	unsigned int PTPOD6_Random	: 1;
	unsigned int PTPOD6_Wfr_ID	: 4;
	unsigned int PTPOD6_Lot_ID7	: 7;
	unsigned int PTPOD6_Lot_ID6	: 4;
};

struct ptp_det {
	const char *name;
	struct ptp_det_ops *ops;
	int status;
	int features;
	enum ptp_ctrl_id ctrl_id;
	enum ptp_det_id det_id;

	/* devinfo */
	unsigned int PTPINITEN;
	unsigned int PTPMONEN;
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

	/* Generated by PTP init01. Used in PTP init02 */
	unsigned int DCVOFFSETIN;
	unsigned int AGEVOFFSETIN;

	/* slope */
	unsigned int MTS;
	unsigned int BTS;

	/* dvfs */
	unsigned int freq_base; /* PTP reference freq_base*/
	unsigned int freq_max;  /* DVFS max freq */
	unsigned int freq_min;  /* DVFS min freq */
	unsigned char freq_tbl[NR_FREQ]; /* percentage based on freq_base */

	unsigned int volt_tbl[NR_FREQ];
	unsigned int volt_tbl_init2[NR_FREQ];
	unsigned int volt_tbl_pmic[NR_FREQ];
	int volt_offset;

	int disabled; /* Disabled by error or sysfs */
};

struct ptp_det_ops {
	void (*switch_bank)(struct ptp_det *det);
};

static void turn_on_mfg_power_domain(void)
{
	/* Enable MFG-top clock */
	ptp_write_field(CLR_CLK_GATING_CTRL8, 7:6, 0x3);

	/* Enable MFG-related power-domain */
	spm_mtcmos_ctrl_mfg_async(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg_2d(STA_POWER_ON);
	spm_mtcmos_ctrl_mfg(STA_POWER_ON);
}

static void turn_off_mfg_power_domain(void)
{
	/* Disable MFG-related power-domain */
	spm_mtcmos_ctrl_mfg_async(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg_2d(STA_POWER_DOWN);
	spm_mtcmos_ctrl_mfg(STA_POWER_DOWN);

	/* Disable MFG-top clock */
	ptp_write_field(SET_CLK_GATING_CTRL8, 7:6, 0x3);
}

/*=============================================================
 *Local variable definition
 *=============================================================
 */
static void base_ops_switch_bank(struct ptp_det *det)
{
	/* Enable Top thermal clock */
	ptp_write_field(CLR_CLK_GATING_CTRL1, 1:1, 0x1);
#if PTP_LOG_EN
	ptp_notice("%s: CLK_GATING_CTRL1 = 0x%x\n",
		   det->name, ptp_read(CLK_GATING_CTRL1));
#endif
	turn_on_mfg_power_domain();

	/* SYSTEMCLK_CG_EN = 31:31 */
	/* PTPODCORE1EN = 17:17 */
	/* PTPODCORE0EN = 16:16 */

	ptp_write_field(PTP_PTPCORESEL, 31:31, 0x1);
	ptp_write_field(PTP_PTPCORESEL, 17:17, 0x1);
	ptp_write_field(PTP_PTPCORESEL, 16:16, 0x1);

	/* APBSEL = 3:0 */
	ptp_write_field(PTP_PTPCORESEL, 3:0, det->ctrl_id);
#if PTP_LOG_EN
	ptp_notice("PTP_PTPCORESEL = 0x%x\n", ptp_read(PTP_PTPCORESEL));
#endif
}

static struct ptp_device_info ptp_devinfo;

static struct ptp_det_ops gpu_det_ops = {
	.switch_bank		= base_ops_switch_bank,
};

static struct ptp_det ptp_detectors[NUM_PTP_DET] = {
	[PTP_DET_GPUSYS] = {
		.name		= "PTP_DET_GPUSYS",
		.ops		= &gpu_det_ops,
		.ctrl_id	= PTP_CTRL_GPUSYS,
		.features	= FEA_INIT01 | FEA_INIT02 | FEA_MON,
		.freq_base	= 500,  /* 500Mhz */
		.VBOOT		= 0x38, /* 1.15v */
		.VMAX		= 0x51, /* 1.30625v */
		.VMIN		= 0x38, /* 1.15v */
		.volt_offset	= 0,
	},
};


/*=============================================================
 * Local function definition
 *=============================================================
 */
static void get_devinfo(struct ptp_device_info *p)
{
	unsigned int *M_HW_RES = (unsigned int *)p, i;

#if TEST_FAKE_EFUSE
	M_HW_RES[0] = 0x10BD3C1B;
	M_HW_RES[1] = 0x0055C0FA;
	M_HW_RES[2] = 0x10BD3C1B;
	M_HW_RES[3] = 0x0055C0FA;
	M_HW_RES[4] = 0x00000000;
	M_HW_RES[5] = 0x00000000;
	M_HW_RES[6] = 0x00000000;
#else
	M_HW_RES[0] = seclib_get_devinfo_with_index(1);
	M_HW_RES[1] = seclib_get_devinfo_with_index(2);
	M_HW_RES[2] = seclib_get_devinfo_with_index(3);
	M_HW_RES[3] = seclib_get_devinfo_with_index(4);
	M_HW_RES[4] = seclib_get_devinfo_with_index(5);
	M_HW_RES[5] = seclib_get_devinfo_with_index(6);
	M_HW_RES[6] = seclib_get_devinfo_with_index(7);
#endif

	for (i = 0; i <= 6; i++)
		ptp_notice("M_HW_RES%d = 0x%x\n", i, M_HW_RES[i]);
}

static int ptp_init_det(struct ptp_det *det, struct ptp_device_info *devinfo)
{
	unsigned int vcore_pmic = 0, vcore_pmic_orig;
	enum ptp_det_id det_id = det_to_id(det);

	/* init with devinfo */
	det->PTPINITEN	= devinfo->PTPINITEN;
	det->PTPMONEN	= devinfo->PTPMONEN;

	/* init with constant */
	det->DETWINDOW	= DETWINDOW_VAL;

	det->DTHI	= DTHI_VAL;
	det->DTLO	= DTLO_VAL;
	det->DETMAX	= DETMAX_VAL;

	det->AGECONFIG	= AGECONFIG_VAL;
	det->AGEM	= AGEM_VAL;
	det->DVTFIXED	= DVTFIXED_VAL;
	det->VCO	= VCO_VAL;
	det->DCCONFIG	= DCCONFIG_VAL;

	switch (det_id) {
	case PTP_DET_GPUSYS:
		det->MDES	= devinfo->GPU_MDES;
		det->BDES	= devinfo->GPU_BDES;
		det->DCMDET	= devinfo->GPU_DCMDET;
		det->DCBDET	= devinfo->GPU_DCBDET;
		det->AGEDELTA	= devinfo->GPU_AGEDELTA;
		det->MTDES	= devinfo->GPU_MTDES;
		det->DVTFIXED	= 0x04;
		/* GPU reference base freq */
		det->freq_tbl[0] = PERCENT(det->freq_max, det->freq_base);

		/* Get original Vcore voltage */
		pmic_read_interface(0x318, &vcore_pmic_orig, 0xFFFF, 0);

		set_vcore_to_pmic(det_id, det->VBOOT + PTPOD_PMIC_OFFSET);
		pmic_read_interface(0x318, &vcore_pmic, 0xFFFF, 0);

		if (det->VBOOT != (vcore_pmic - PTPOD_PMIC_OFFSET)) {
			ptp_notice("%s: un-match. VBOOT = 0x%x pmic = 0x%x\n",
				   det->name, det->VBOOT,
				   (vcore_pmic - PTPOD_PMIC_OFFSET));
			ptp_notice("Restore early Vcore voltage\n");
			set_vcore_to_pmic(det_id, vcore_pmic_orig);
			pmic_read_interface(0x318, &vcore_pmic_orig, 0xFFFF, 0);
			ptp_notice("Vcore is %duV now.\n",
				   PTP_PMIC_VAL_TO_VOLT(vcore_pmic_orig));
			return -1;
		}

		break;

	default:
		ptp_notice("[%s]: Unknown det_id %d\n", __func__, det_id);
		break;
	}

	return 0;
}

static void base_ops_set_phase(struct ptp_det *det, enum ptp_phase phase)
{
	unsigned int i, filter, val;

	det->ops->switch_bank(det);

	/* config PTP register */
	ptp_write(PTP_DESCHAR,
		  ((det->BDES << 8) & 0xff00) | (det->MDES & 0xff));
	ptp_write(PTP_TEMPCHAR,
		  (((det->VCO << 16) & 0xff0000) |
		   ((det->MTDES << 8) & 0xff00) |
		   (det->DVTFIXED & 0xff)));
	ptp_write(PTP_DETCHAR,
		  ((det->DCBDET << 8) & 0xff00) | (det->DCMDET & 0xff));
	ptp_write(PTP_AGECHAR,
		  ((det->AGEDELTA << 8) & 0xff00) | (det->AGEM & 0xff));
	ptp_write(PTP_DCCONFIG, det->DCCONFIG);
	ptp_write(PTP_AGECONFIG, det->AGECONFIG);

	if (phase == PTP_PHASE_MON)
		ptp_write(PTP_TSCALCS,
			  ((det->BTS << 12) & 0xfff000) | (det->MTS & 0xfff));

	if (det->AGEM == 0x0)
		ptp_write(PTP_RUNCONFIG, 0x80000000);
	else {
		val = 0x0;

		for (i = 0; i < 24; i += 2) {
			filter = 0x3 << i;

			if (((det->AGECONFIG) & filter) == 0x0)
				val |= (0x1 << i);
			else
				val |= ((det->AGECONFIG) & filter);
		}

		ptp_write(PTP_RUNCONFIG, val);
	}

	ptp_write(PTP_FREQPCT30,
		  ((det->freq_tbl[3] << 24) & 0xff000000) |
		  ((det->freq_tbl[2] << 16) & 0xff0000)	  |
		  ((det->freq_tbl[1] << 8) & 0xff00)	  |
		  (det->freq_tbl[0] & 0xff));
	ptp_write(PTP_FREQPCT74,
		  ((det->freq_tbl[7] << 24) & 0xff000000) |
		  ((det->freq_tbl[6] << 16) & 0xff0000)	  |
		  ((det->freq_tbl[5] << 8) & 0xff00)	  |
		  ((det->freq_tbl[4]) & 0xff));
	ptp_write(PTP_LIMITVALS,
		  ((det->VMAX << 24) & 0xff000000) |
		  ((det->VMIN << 16) & 0xff0000)   |
		  ((det->DTHI << 8) & 0xff00)	   |
		  (det->DTLO & 0xff));
	ptp_write(PTP_VBOOT, (((det->VBOOT) & 0xff)));
	ptp_write(PTP_DETWINDOW, (((det->DETWINDOW) & 0xffff)));
	ptp_write(PTP_PTPCONFIG, (((det->DETMAX) & 0xffff)));

	/* clear all pending PTP interrupt & config PTPINTEN */
	ptp_write(PTP_PTPINTSTS, 0xffffffff);

	switch (phase) {
	case PTP_PHASE_INIT01:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000001);
		break;

	case PTP_PHASE_INIT02:
		ptp_write(PTP_PTPINTEN, 0x00005f01);
		ptp_write(PTP_INIT2VALS,
			  ((det->AGEVOFFSETIN << 16) & 0xffff0000) |
			  (det->DCVOFFSETIN & 0xffff));
		/* enable PTP INIT measurement */
		ptp_write(PTP_PTPEN, 0x00000005);
		break;

	case PTP_PHASE_MON:
		ptp_write(PTP_PTPINTEN, 0x00FF0000);
		/* enable PTP monitor mode */
		ptp_write(PTP_PTPEN, 0x00000002);
		break;

	default:
		ptp_notice("[%s]: Unknown phase %d\n", __func__, phase);
		break;
	}
}

#if PTP_LOG_EN
static void mt_ptp_reg_dump(void)
{
	unsigned long long addr;

	addr = PTP_REVISIONID;
	ptp_notice("0x%llx = 0x%x\n", addr, ptp_read(addr));

	for (addr = PTP_DESCHAR; addr <= PTP_THSLPEVEB; addr += 4)
		ptp_notice("0x%llx = 0x%x\n", addr, ptp_read(addr));
}
#else
static void mt_ptp_reg_dump(void)
{
}
#endif

static inline void handle_init01_isr(struct ptp_det *det)
{
	ptp_notice("%s: %s: VDN74:0x%08x, VDN30:0x%08x, DCVALUES:0x%08x\n",
		   det->name, __func__, ptp_read(PTP_VDESIGN74),
		   ptp_read(PTP_VDESIGN30), ptp_read(PTP_DCVALUES));
	/*
	 * Read & store 16 bit values DCVALUES.DCVOFFSET and
	 * AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
	 */
	/* det->DCVOFFSETIN = ~(ptp_read(PTP_DCVALUES) & 0xffff) + 1; */
	det->DCVOFFSETIN = 0; /* for MT8167 only */
	det->AGEVOFFSETIN = ptp_read(PTP_AGEVALUES) & 0xffff;

	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);
}

static inline void handle_init02_isr(struct ptp_det *det)
{
	unsigned int temp;
	int i;

	ptp_notice("%s: %s: VOP74:0x%08x, VOP30:0x%08x, DCVALUES:0x%08x\n",
		   det->name, __func__, ptp_read(PTP_VOP74),
		   ptp_read(PTP_VOP30), ptp_read(PTP_DCVALUES));

	temp = ptp_read(PTP_VOP30);
	det->volt_tbl[0] = temp & 0xff;
	det->volt_tbl[1] = (temp >> 8) & 0xff;
	det->volt_tbl[2] = (temp >> 16) & 0xff;
	det->volt_tbl[3] = (temp >> 24) & 0xff;

	temp = ptp_read(PTP_VOP74);
	det->volt_tbl[4] = temp & 0xff;
	det->volt_tbl[5] = (temp >> 8) & 0xff;
	det->volt_tbl[6] = (temp >> 16) & 0xff;
	det->volt_tbl[7] = (temp >> 24) & 0xff;

	for (i = 0; i < NR_FREQ; i++)
		det->volt_tbl[i] = det->volt_tbl[i] + PTPOD_PMIC_OFFSET;

	memcpy(det->volt_tbl_init2, det->volt_tbl, sizeof(det->volt_tbl_init2));

	ptp_write(PTP_PTPEN, 0x0);
	ptp_write(PTP_PTPINTSTS, 0x1);
}

static inline void handle_init_err_isr(struct ptp_det *det)
{
	ptp_notice("====================================================\n");
	ptp_notice("%s: %s()\n", det->name, __func__);
	ptp_notice("PTPEN(0x%llx) = 0x%x, PTPINTSTS(0x%llx) = 0x%x\n",
		   PTP_PTPEN, ptp_read(PTP_PTPEN),
		   PTP_PTPINTSTS, ptp_read(PTP_PTPINTSTS));
	ptp_notice("PTP_SMSTATE0 (0x%llx) = 0x%x\n",
		   PTP_SMSTATE0, ptp_read(PTP_SMSTATE0));
	ptp_notice("PTP_SMSTATE1 (0x%llx) = 0x%x\n",
		   PTP_SMSTATE1, ptp_read(PTP_SMSTATE1));
	ptp_notice("====================================================\n");
#if 0
	det->ops->disable_locked(det, BY_INIT_ERROR);
#endif
}

static inline void ptp_isr_handler(struct ptp_det *det)
{
	unsigned int PTPINTSTS, PTPEN;

	PTPINTSTS = ptp_read(PTP_PTPINTSTS);
	PTPEN = ptp_read(PTP_PTPEN);

	if (PTPINTSTS == 0x1) { /* PTP init1 or init2 */
		if ((PTPEN & 0x7) == 0x1)   /* PTP init1 */
			handle_init01_isr(det);
		else if ((PTPEN & 0x7) == 0x5)   /* PTP init2 */
			handle_init02_isr(det);
		else {
			/*
			 * error : init1 or init2,
			 */
			handle_init_err_isr(det);
		}
	} else { /* PTP error handler */
		/* init 1  || init 2 error handler */
		if (((PTPEN & 0x7) == 0x1) || ((PTPEN & 0x7) == 0x5))
			handle_init_err_isr(det);
	}
}

static void ptp_isr(void)
{
	struct ptp_det *det;
#if PTP_LOG_EN
	ptp_notice("%s(): PTP_PTPODINTST = 0x%x\n",
		   __func__, ptp_read(PTP_PTPODINTST));
#endif
	while (BIT(PTP_CTRL_GPUSYS) & ptp_read(PTP_PTPODINTST))
		;

	det = &ptp_detectors[PTP_DET_GPUSYS];
	det->ops->switch_bank(det);

	ptp_isr_handler(det);
	mt_ptp_reg_dump();
}

static int set_vcore_to_pmic(enum ptp_det_id det_id, unsigned int target_volt)
{
	if (det_id == PTP_DET_GPUSYS) {
		pmic_config_interface(0x312, target_volt, 0x7F, 0);
		pmic_config_interface(0x314, target_volt, 0x7F, 0);
#if PTP_LOG_EN
		/* Read Vcore Voltage */
		pmic_read_interface(0x318, &target_volt, 0xFFFF, 0);
		ptp_notice("%s(): Vcore pmic = 0x%x\n", __func__, target_volt);
#endif
	}

	return 0;
}

/*=============================================================
 * Global function definition
 *=============================================================
 */

void ptp_init(void)
{
	int ret;
	unsigned int vcore_pmic, vcore_ptp_volt = 0;
	struct ptp_det *det;
	u32 reg_val;

	get_devinfo(&ptp_devinfo);
	det = &ptp_detectors[PTP_DET_GPUSYS];

#if TEST_FAKE_EFUSE
	det->freq_max = 574;
#else
	det->freq_max = mt_gpufreq_get_max_freq();
	det->freq_min = mt_gpufreq_get_min_freq();

	if (ptp_devinfo.PTPINITEN == 0) {
		ptp_notice("PTPINITEN = 0x%x\n", ptp_devinfo.PTPINITEN);
		return;
	} else if (det->freq_max <= det->freq_min) {
		ptp_notice("GPU's max freq is %dMHz <= %dMHz\n",
			   det->freq_max, det->freq_min);
		ptp_notice("PTPOD doesn't need to tune down Vcore voltage\n");
		return;
	}
#endif
	ptp_notice("GPU's max freq = %dMHz\n", det->freq_max);

	ret = ptp_init_det(det, &ptp_devinfo);
	if (ret != 0)
		return;

	/* pmic PWM */
	pmic_config_interface(0x304, 0x1, 0x1, 8);
	pmic_read_interface(0x304, &reg_val, 0xFFFF, 0);
	ptp_notice("Vcore(0x304) mode = 0x%x\n", reg_val);

	/* PTPOD init01 */
	base_ops_set_phase(det, PTP_PHASE_INIT01);
	ptp_isr();

	/* PTPOD init02 */
	base_ops_set_phase(det, PTP_PHASE_INIT02);
	ptp_isr();

	vcore_pmic = det->volt_tbl_init2[0];

	/* Set PTPOD Vcore voltage */
	pmic_config_interface(0x312, vcore_pmic, 0x7F, 0);
	pmic_config_interface(0x314, vcore_pmic, 0x7F, 0);

	/* pmic auto */
	pmic_config_interface(0x304, 0x0, 0x1, 8);
	pmic_read_interface(0x304, &reg_val, 0xFFFF, 0);
	ptp_notice("Vcore(0x304) mode = 0x%x\n", reg_val);

	/* Read PMIC Vcore step */
	pmic_read_interface(0x318, &vcore_pmic, 0xFFFF, 0);
	vcore_ptp_volt = PTP_PMIC_VAL_TO_VOLT(vcore_pmic);

	ptp_notice("Right now, Vcore is %duV\n", vcore_ptp_volt);

	/* Disable Top thermal clock */
	ptp_write_field(SET_CLK_GATING_CTRL1, 1:1, 0x1);
#if PTP_LOG_EN
	ptp_notice("CLK_GATING_CTRL1 = 0x%x\n", ptp_read(CLK_GATING_CTRL1));
#endif
	turn_off_mfg_power_domain();
}

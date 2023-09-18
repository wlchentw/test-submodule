/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek
 * Software if you have agreed to and been bound by the applicable license
 * agreement with MediaTek ("License Agreement") and been granted explicit
 * permission to do so within the License Agreement ("Permitted User").
 * If you are not a Permitted User, please cease any access or use of MediaTek
 * Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
 * DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE.
 * MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A
 * PARTICULAR STANDARD OR OPEN FORUM.
 * RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
 * LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/** @file dramc_actiming_api.c
 *  Basic DRAMC API implementation
 */

/* Include files */
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>


/*
 * JESD209-4B: tRFCab has 4 settings for 7 density settings
 * (130, 180, 280, 380)
 * tRFCAB_NUM: Used to indicate tRFCab group
 * (since some densities share the same tRFCab)
 */
enum tRFCABIdx {
	tRFCAB_130 = 0,
	tRFCAB_180,
	tRFCAB_280,
	tRFCAB_380,
	tRFCAB_NUM
};

enum ACTimeIdx {
	GRP_DDR1600_ACTIM = 0,
	GRP_DDR1866_ACTIM,
	GRP_DDR2400_ACTIM,
	GRP_ACTIM_NUM
};

/*
 * ACTiming struct declaration
 * (declared here due fld_wid for each register type)
 * Should include all fields from ACTiming excel file
 * (And update the correct values in UpdateACTimingReg()
 * Note: DQSINCTL, DATLAT aren't in ACTiming excel file
 * (internal delay parameters)
 */

typedef struct _ACTime_T {
	unsigned char dramType, cbtMode, readDBI;
	unsigned short freqGroup;
	unsigned char readLat, writeLat;
	/* DQSINCTL, DATLAT aren't in ACTiming excel file */
	unsigned char dqsinctl, datlat;

	/* DRAMC_REG_SHU_ACTIM0 =================================== */
	unsigned short trcd;
	unsigned short trrd;
	unsigned short twr;
	unsigned short twtr;

	/* DRAMC_REG_SHU_ACTIM1 =================================== */
	unsigned short trc;
	unsigned short tras;
	unsigned short trp;
	unsigned short trpab;

	/* DRAMC_REG_SHU_ACTIM2 =================================== */
	unsigned short tfaw;

	unsigned short trtw_ODT_off;
	unsigned short trtw_ODT_on;

	unsigned short trtp;
	unsigned short txp;

	/* DRAMC_REG_SHU_ACTIM3 =================================== */
	unsigned short refcnt;
	unsigned short trfc;
	unsigned short trfcpb;

	/* DRAMC_REG_SHU_ACTIM4 =================================== */
	unsigned short tzqcs;
	unsigned short refcnt_fr_clk;
	unsigned short txrefcnt;

	/* DRAMC_REG_SHU_ACTIM5 =================================== */
	unsigned short tmrr2w_ODT_off;
	unsigned short tmrr2w_ODT_on;

	unsigned short twtpd;
	unsigned short trtpd;

	/* DRAMC_REG_SHU_ACTIM_XRT ================================= */
	unsigned short xrtw2w;
	unsigned short xrtw2r;
	unsigned short xrtr2w;
	unsigned short xrtr2r;

	/* DRAMC_REG_SHU_AC_TIME_05T ================================ */
	unsigned short twtr_05T : 1;
	unsigned short trtw_ODT_off_05T : 1;
	unsigned short trtw_ODT_on_05T : 1;

	unsigned short twtpd_05T : 1;
	unsigned short trtpd_05T : 1;
	unsigned short tfaw_05T : 1;
	unsigned short trrd_05T : 1;
	unsigned short twr_05T : 1;
	unsigned short tras_05T : 1;
	unsigned short trpab_05T : 1;
	unsigned short trp_05T : 1;
	unsigned short trcd_05T : 1;
	unsigned short trtp_05T : 1;
	unsigned short txp_05T : 1;
	unsigned short trfc_05T : 1;
	unsigned short trfcpb_05T : 1;
	unsigned short trc_05T : 1;

	/* cc add for DDR4 BG timing */
	unsigned short tbg_wtr;
	unsigned short tbg_ccd;
	unsigned short tbg_rrd;
	unsigned short tbg_wtr_05T : 1;
	unsigned short tbg_ccd_05T : 1;
	unsigned short tbg_rrd_05T : 1;

	/* Other ACTiming reg fields =================================== */
	unsigned short r_dmcatrain_intv;
	unsigned short r_dmmrw_intv;
	unsigned short r_dmfspchg_prdcnt;
	unsigned short ckeprd;
	unsigned short ckelckcnt;
	unsigned short zqlat2;
} ACTime_T;

/* ac_timing_tbl[] forward declaration */
const ACTime_T ac_timing_tbl[TOTAL_AC_TIMING_NUMBER];

/* ac_timing_tbl: All freq's ACTiming from ACTiming excel file
 * Note: !!All ACTiming adjustments should not be set in -
 * table should be moved into UpdateACTimingReg()!!
 *       Or else preloader's highest freq ACTimings may be set to different
 * values than expected.
 */
const ACTime_T ac_timing_tbl[TOTAL_AC_TIMING_NUMBER] = {
/* lp4 */
#if SUPPORT_TYPE_LPDDR4
#if SUPPORT_LP4_DDR2400_ACTIM
	/* LP4_DDR2400 ACTiming--------------------------------- */
	/* AC_TIME_LP4_NORM_DDR1600_DBI_OFF */
	/* LP4-1600, 800MHz, RDBI_OFF, normal mode */
	{
		.dramType = TYPE_LPDDR4, .freqGroup = DDR2400_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 4,	.tras_05T = 1,
        .trp = 3,	.trp_05T = 1,
        .trpab = 1,	.trpab_05T = 0,
        .trc = 10,	.trc_05T = 0,
        .trfc = 72,	.trfc_05T = 0,
        .trfcpb = 30,	.trfcpb_05T = 0,
        .txp = 0,	.txp_05T = 1,
        .trtp = 1,	.trtp_05T = 0,
        .trcd = 4,	.trcd_05T = 1,
        .twr = 10,	.twr_05T = 1,
        .twtr = 6,	.twtr_05T = 1,
        .trrd = 1,	.trrd_05T = 0,
        .tfaw = 3,	.tfaw_05T = 0,
        .trtw_ODT_off = 5,	.trtw_ODT_off_05T = 0,
        .trtw_ODT_on = 7,	.trtw_ODT_on_05T = 0,
        .refcnt = 73,
        .refcnt_fr_clk = 101,
        .txrefcnt = 91,
        .tzqcs = 25,
        .xrtw2w = 5,
        .xrtw2r = 3,
        .xrtr2w = 6,
        .xrtr2r = 8,
        .r_dmcatrain_intv = 9,
        .r_dmmrw_intv = 0xf, //Berson: LP3/4 both use this field -> Formula may change, set to 0xF for now
        .r_dmfspchg_prdcnt = 75,
        .trtpd = 9,	.trtpd_05T = 0,
        .twtpd = 9,	.twtpd_05T = 0,
        .tmrr2w_ODT_off = 6,
        .tmrr2w_ODT_on = 8,
        .ckeprd = 2,
        .ckelckcnt = 0,
        .zqlat2 = 9,

		.dqsinctl = 2,
		.datlat = 15
	},
#endif	/* SUPPORT_LP4_DDR1600_ACTIM */
#if SUPPORT_LP4_DDR1600_ACTIM
	/* LP4_DDR1600 ACTiming--------------------------------- */
	/* AC_TIME_LP4_NORM_DDR1600_DBI_OFF */
	/* LP4-1600, 800MHz, RDBI_OFF, normal mode */
	{
		.dramType = TYPE_LPDDR4, .freqGroup = DDR1600_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0,	.tras_05T = 0,
        .trp = 2,	.trp_05T = 0,
        .trpab = 0,	.trpab_05T = 1,
        .trc = 4,	.trc_05T = 0,
        .trfc = 44,	.trfc_05T = 0,
        .trfcpb = 16,	.trfcpb_05T = 0,
        .txp = 0,	.txp_05T = 0,
        .trtp = 1,	.trtp_05T = 1,
        .trcd = 3,	.trcd_05T = 0,
        .twr = 7,	.twr_05T = 1,
        .twtr = 4,	.twtr_05T = 1,
        .trrd = 0,	.trrd_05T = 0,
        .tfaw = 0,	.tfaw_05T = 0,
        .trtw_ODT_off = 3,	.trtw_ODT_off_05T = 0,
        .trtw_ODT_on = 4,	.trtw_ODT_on_05T = 0,
        .refcnt = 48,
        .refcnt_fr_clk = 101,
        .txrefcnt = 62,
        .tzqcs = 16,
        .xrtw2w = 5,
        .xrtw2r = 3,
        .xrtr2w = 3,
        .xrtr2r = 8,
        .r_dmcatrain_intv = 8,
        .r_dmmrw_intv = 0xf, //Berson: LP3/4 both use this field -> Formula may change, set to 0xF for now
        .r_dmfspchg_prdcnt = 50,
        .trtpd = 6,	.trtpd_05T = 0,
        .twtpd = 6,	.twtpd_05T = 0,
        .tmrr2w_ODT_off = 3,
        .tmrr2w_ODT_on = 5,
        .ckeprd = 1,
        .ckelckcnt = 0, //LP3 doesn't use this field
        .zqlat2 = 6, //LP3 doesn't use this field

		.dqsinctl = 1,
		.datlat = 13
	},
#endif	/* SUPPORT_LP4_DDR1600_ACTIM */
#endif /* SUPPORT_TYPE_LPDDR4 */

#if SUPPORT_TYPE_PCDDR4
#if SUPPORT_PC4_DDR2667_ACTIM /* From DE sim waveform */
	{
		.dramType = TYPE_PCDDR4, .freqGroup = DDR2666_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x3,
		.tras_05T = 0,
		.trp = 0x3,
		.trp_05T = 0,
		.trpab = 0x0,
		.trpab_05T = 0,
		.trc = 0x8,
		.trc_05T = 0,
		.trfc = 0x69,
		.trfc_05T = 0,
		.trfcpb = 0x0,
		.trfcpb_05T = 0,
		.txp = 0x0,
		.txp_05T = 0,
		.trtp = 0x1,
		.trtp_05T = 0,
		.trcd = 0x4,
		.trcd_05T = 0,
		.twr = 0x9,
		.twr_05T = 0,
		.twtr = 0x4,
		.twtr_05T = 1,
		.trrd = 0x0,
		.trrd_05T = 0,
		.tfaw = 0x4,
		.tfaw_05T = 0,
		.trtw_ODT_off = 0x6,
		.trtw_ODT_off_05T = 0,
		.trtw_ODT_on = 0x6,
		.trtw_ODT_on_05T = 0,
		.refcnt = 0x31,
		.refcnt_fr_clk = 0x66,
		.txrefcnt = 0xd6,
		.tzqcs = 0x20,
		.xrtw2w = 0x7,
		.xrtw2r = 0x2,
		.xrtr2w = 0x0,
		.xrtr2r = 0x7,
		.r_dmcatrain_intv = 0x4,

		.tbg_wtr = 0x6,
		.tbg_wtr_05T = 0x1,
		.tbg_ccd = 0x1,
		.tbg_ccd_05T = 0x0,
		.tbg_rrd = 0x0,
		.tbg_rrd_05T = 0x1,

		.r_dmmrw_intv = 0x15,
		.r_dmfspchg_prdcnt = 0x32,
		.trtpd = 0xa,
		.trtpd_05T = 0,
		.twtpd = 0xa,
		.twtpd_05T = 1,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x1,
		.ckeprd = 0x2,
		.ckelckcnt = 0,
		.zqlat2 = 0x1f,

		.dqsinctl = 2,
		.datlat = 0xf,
	},
#endif

#if SUPPORT_PC4_DDR2400_ACTIM /* From DE sim waveform */
	{
		.dramType = TYPE_PCDDR4, .freqGroup = DDR2400_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x3,
		.tras_05T = 0,
		.trp = 0x3,
		.trp_05T = 0,
		.trpab = 0x0,
		.trpab_05T = 0,
		.trc = 0x8,
		.trc_05T = 0,
		.trfc = 0x69,
		.trfc_05T = 0,
		.trfcpb = 0x0,
		.trfcpb_05T = 0,
		.txp = 0x0,
		.txp_05T = 0,
		.trtp = 0x1,
		.trtp_05T = 0,
		.trcd = 0x4,
		.trcd_05T = 0,
		.twr = 0x9,
		.twr_05T = 0,
		.twtr = 0x4,
		.twtr_05T = 1,
		.trrd = 0x0,
		.trrd_05T = 0,
		.tfaw = 0x4,
		.tfaw_05T = 0,
		.trtw_ODT_off = 0x6,
		.trtw_ODT_off_05T = 0,
		.trtw_ODT_on = 0x6,
		.trtw_ODT_on_05T = 0,
		.refcnt = 0x31,
		.refcnt_fr_clk = 0x66,
		.txrefcnt = 0xd6,
		.tzqcs = 0x20,
		.xrtw2w = 0x7,
		.xrtw2r = 0x2,
		.xrtr2w = 0x0,
		.xrtr2r = 0x7,
		.r_dmcatrain_intv = 0x4,

		.tbg_wtr = 0x6,
		.tbg_wtr_05T = 0x1,
		.tbg_ccd = 0x1,
		.tbg_ccd_05T = 0x0,
		.tbg_rrd = 0x0,
		.tbg_rrd_05T = 0x1,

		.r_dmmrw_intv = 0x15,
		.r_dmfspchg_prdcnt = 0x32,
		.trtpd = 0xa,
		.trtpd_05T = 0,
		.twtpd = 0xa,
		.twtpd_05T = 1,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x1,
		.ckeprd = 0x2,
		.ckelckcnt = 0,
		.zqlat2 = 0x1f,

		.dqsinctl = 1,
		.datlat = 0xf,
	},
#endif

#if SUPPORT_PC4_DDR1866_ACTIM
	{ /* From 7580 */
		.dramType = TYPE_PCDDR4, .freqGroup = DDR1866_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x0,
		.tras_05T = 0,
		.trp = 0x1,
		.trp_05T = 0x1,
		.trpab = 0x0,
		.trpab_05T = 0x0,
		.trc = 0x3,
		.trc_05T = 0x0,
		.trfc = 0x46,
		.trfc_05T = 0x0,
		.trfcpb = 0x18,
		.trfcpb_05T = 0x0,
		.txp = 0x0,
		.txp_05T = 0x0,
		.trtp = 0x0,
		.trtp_05T = 0x0,
		.trcd = 0x2,
		.trcd_05T = 0x1,
		.twr = 0x6,
		.twr_05T = 0x1,
		.twtr = 0x2,
		.twtr_05T = 0x1,
		.trrd = 0x0,
		.trrd_05T = 0x1,
		.tfaw = 0x0,
		.tfaw_05T = 0x1,
		.trtw_ODT_off = 0x6,
		.trtw_ODT_off_05T = 0x0,
		.trtw_ODT_on = 0x6,
		.trtw_ODT_on_05T = 0x0,
		.refcnt = 0x31,
		.refcnt_fr_clk = 0x5c,
		.txrefcnt = 0xd6,
		.tzqcs = 0x20,
		.xrtw2w = 0x6,
		.xrtw2r = 0x2,
		.xrtr2w = 0x0,
		.xrtr2r = 0x7,
		.r_dmcatrain_intv = 8, /* NOT used */

		/* DDR4 specific */
		.tbg_wtr = 0x3,
		.tbg_wtr_05T = 0x1,
		.tbg_ccd = 0x0,
		.tbg_ccd_05T = 0x1,
		.tbg_rrd = 0x0,
		.tbg_rrd_05T = 0x0,

		.r_dmmrw_intv = 0xf,  /* NOT used */
		.r_dmfspchg_prdcnt = 50,  /* NOT used */
		.trtpd = 0xc,
		.trtpd_05T = 0x0,
		.twtpd = 0x10,
		.twtpd_05T = 0x0,
		.tmrr2w_ODT_off = 0x2,
		.tmrr2w_ODT_on = 0x5,
		.ckeprd = 2,  /* NOT used */
		.ckelckcnt = 0,  /* NOT used */
		.zqlat2 = 6,  /* NOT used */

		.dqsinctl = 0x2,
		.datlat = 0xf,
	},
#endif
#endif /* SUPPORT_TYPE_PCDDR4 */

#if SUPPORT_TYPE_LPDDR3
#if SUPPORT_LP3_DDR1866_ACTIM
	{ /* From 7580 */
		.dramType = TYPE_LPDDR3, .freqGroup = DDR1866_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0xb,
		.tras_05T = 0,
		.trp = 0x9,
		.trp_05T = 0x0,
		.trpab = 0x1,
		.trpab_05T = 0x0,
		.trc = 0x14,
		.trc_05T = 0x0,
		.trfc = 0x49,
		.trfc_05T = 0x0,
		.trfcpb = 0x19,
		.trfcpb_05T = 0x0,
		.txp = 0x0,
		.txp_05T = 0x0,
		.trtp = 0x2,
		.trtp_05T = 0x0,
		.trcd = 0x9,
		.trcd_05T = 0x0,
		.twr = 0xd,
		.twr_05T = 0x1,
		.twtr = 0x8,
		.twtr_05T = 0x1,
		.trrd = 0x3,
		.trrd_05T = 0x0,
		.tfaw = 0xc,
		.tfaw_05T = 0x0,
		.trtw_ODT_off = 0x5,
		.trtw_ODT_off_05T = 0x1,
		.trtw_ODT_on = 0x7,
		.trtw_ODT_on_05T = 0x1,
		.refcnt = 0x0,
		.refcnt_fr_clk = 0x65,
		.txrefcnt = 0x60,
		.tzqcs = 0x23,
		.xrtw2w = 0x8,
		.xrtw2r = 0x1,
		.xrtr2w = 0x4,
		.xrtr2r = 0xc,
		.r_dmcatrain_intv = 8, /* NOT used */

		.r_dmmrw_intv = 0xf,  /* NOT used */
		.r_dmfspchg_prdcnt = 50,  /* NOT used */
		.trtpd = 0x8,
		.trtpd_05T = 0x0,
		.twtpd = 0x9,
		.twtpd_05T = 0x0,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x3,
		.ckeprd = 2,
		.ckelckcnt = 0,  /* NOT used */
		.zqlat2 = 6,  /* NOT used */

		.dqsinctl = 0x2,
		.datlat = 0x13,
	},
#endif

#if SUPPORT_LP3_DDR1600_ACTIM
	{
		.dramType = TYPE_LPDDR3, .freqGroup = DDR1600_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x9,
		.tras_05T = 0,
		.trp = 0x9,
		.trp_05T = 0x0,
		.trpab = 0x1,
		.trpab_05T = 0x0,
		.trc = 0x14,
		.trc_05T = 0x0,
		.trfc = 0x49,
		.trfc_05T = 0x0,
		.trfcpb = 0x19,
		.trfcpb_05T = 0x0,
		.txp = 0x0,
		.txp_05T = 0x0,
		.trtp = 0x2,
		.trtp_05T = 0x0,
		.trcd = 0x9,
		.trcd_05T = 0x0,
		.twr = 0xb,
		.twr_05T = 0x1,
		.twtr = 0x6,
		.twtr_05T = 0x1,
		.trrd = 0x3,
		.trrd_05T = 0x0,
		.tfaw = 0xc,
		.tfaw_05T = 0x0,
		.trtw_ODT_off = 0x5,
		.trtw_ODT_off_05T = 0x1,
		.trtw_ODT_on = 0x7,
		.trtw_ODT_on_05T = 0x1,
		.refcnt = 0x0,
		.refcnt_fr_clk = 0x65,
		.txrefcnt = 0x60,
		.tzqcs = 0x23,
		.xrtw2w = 0x8,
		.xrtw2r = 0x1,
		.xrtr2w = 0x4,
		.xrtr2r = 0xc,
		.r_dmcatrain_intv = 8, /* NOT used */

		.r_dmmrw_intv = 0xf,  /* NOT used */
		.r_dmfspchg_prdcnt = 50,  /* NOT used */
		.trtpd = 0x8,
		.trtpd_05T = 0x0,
		.twtpd = 0x9,
		.twtpd_05T = 0x0,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x3,
		.ckeprd = 2,
		.ckelckcnt = 0,  /* NOT used */
		.zqlat2 = 6,  /* NOT used */

		.dqsinctl = 0x2,
		.datlat = 0x11,
	},
#endif
#endif /* SUPPORT_TYPE_LPDDR3 */

#if SUPPORT_TYPE_PCDDR3
#if SUPPORT_PC3_DDR1866_ACTIM
	{ /* From 7580 */
		.dramType = TYPE_PCDDR3, .freqGroup = DDR1866_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x9,
		.tras_05T = 0,
		.trp = 0x5,
		.trp_05T = 0x1,
		.trpab = 0x0,
		.trpab_05T = 0x0,
		.trc = 0xb,
		.trc_05T = 0x1,
		.trfc = 0x81,
		.trfc_05T = 0x0,
		.trfcpb = 0x18,
		.trfcpb_05T = 0x0,
		.txp = 0x2,
		.txp_05T = 0x1,
		.trtp = 0x4,
		.trtp_05T = 0x0,
		.trcd = 0x6,
		.trcd_05T = 0x1,
		.twr = 0xf,
		.twr_05T = 0x0,
		.twtr = 0x7,
		.twtr_05T = 0x1,
		.trrd = 0x3,
		.trrd_05T = 0x0,
		.tfaw = 0x8,
		.tfaw_05T = 0x0,
		.trtw_ODT_off = 0x9,
		.trtw_ODT_off_05T = 0x0,
		.trtw_ODT_on = 0xc,
		.trtw_ODT_on_05T = 0x0,
		.refcnt = 0x0,
		.refcnt_fr_clk = 0x97,
		.txrefcnt = 0xff,
		.tzqcs = 0x25,
		.xrtw2w = 0x8,
		.xrtw2r = 0x5,
		.xrtr2w = 0x5,
		.xrtr2r = 0xb,
		.r_dmcatrain_intv = 8, /* NOT used */

		.r_dmmrw_intv = 0xf,  /* NOT used */
		.r_dmfspchg_prdcnt = 50,  /* NOT used */
		.trtpd = 0x8,
		.trtpd_05T = 0x0,
		.twtpd = 0x9,
		.twtpd_05T = 0x0,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x3,
		.ckeprd = 2,  /* NOT used */
		.ckelckcnt = 0,  /* NOT used */
		.zqlat2 = 6,  /* NOT used */

		.dqsinctl = 0x2,
		.datlat = 0xf,
	},
#endif

#if SUPPORT_PC3_DDR1600_ACTIM
	{ /* From 7580 */
		.dramType = TYPE_PCDDR3, .freqGroup = DDR1600_FREQ,
		.cbtMode = CBT_NORMAL_MODE,
		.readDBI = DBI_OFF, .readLat = 14, .writeLat = 8,

		.tras = 0x6,
		.tras_05T = 0,
		.trp = 0x4,
		.trp_05T = 0x1,
		.trpab = 0x0,
		.trpab_05T = 0x0,
		.trc = 0xb,
		.trc_05T = 0x1,
		.trfc = 0x81,
		.trfc_05T = 0x0,
		.trfcpb = 0x18,
		.trfcpb_05T = 0x0,
		.txp = 0x0,
		.txp_05T = 0x1,
		.trtp = 0x2,
		.trtp_05T = 0x0,
		.trcd = 0x4,
		.trcd_05T = 0x1,
		.twr = 0xb,
		.twr_05T = 0x0,
		.twtr = 0x7,
		.twtr_05T = 0x1,
		.trrd = 0x2,
		.trrd_05T = 0x0,
		.tfaw = 0x8,
		.tfaw_05T = 0x0,
		.trtw_ODT_off = 0x9,
		.trtw_ODT_off_05T = 0x0,
		.trtw_ODT_on = 0xc,
		.trtw_ODT_on_05T = 0x0,
		.refcnt = 0x0,
		.refcnt_fr_clk = 0x97,
		.txrefcnt = 0xff,
		.tzqcs = 0x1e,
		.xrtw2w = 0x8,
		.xrtw2r = 0x4,
		.xrtr2w = 0x4,
		.xrtr2r = 0xb,
		.r_dmcatrain_intv = 8, /* NOT used */

		.r_dmmrw_intv = 0xf,  /* NOT used */
		.r_dmfspchg_prdcnt = 50,  /* NOT used */
		.trtpd = 0x8,
		.trtpd_05T = 0x0,
		.twtpd = 0x9,
		.twtpd_05T = 0x0,
		.tmrr2w_ODT_off = 0x1,
		.tmrr2w_ODT_on = 0x3,
		.ckeprd = 2,  /* NOT used */
		.ckelckcnt = 0,  /* NOT used */
		.zqlat2 = 6,  /* NOT used */

		.dqsinctl = 0x1,
		.datlat = 0xf,
	},
#endif
#endif /* SUPPORT_TYPE_PCDDR3 */
};

typedef struct {
	unsigned char trfc:8;
	unsigned char trfrc_05t:1;
	unsigned short txrefcnt_val:10;
} optimizeACTime;
const optimizeACTime tRFCab_Opt[GRP_ACTIM_NUM][tRFCAB_NUM] = {
	/* For freqGroup DDR1600 */
	{
		/* tRFCab = 130 */
		{.trfc = 14, .trfrc_05t = 0, .txrefcnt_val = 32},
		/* tRFCab = 180 */
		{.trfc = 24, .trfrc_05t = 0, .txrefcnt_val = 42},
		/* tRFCab = 280 */
		{.trfc = 44, .trfrc_05t = 0, .txrefcnt_val = 62},
		/* tRFCab = 380 */
		{.trfc = 64, .trfrc_05t = 0, .txrefcnt_val = 82}
	},
	/* For freqGroup DDR3200 */
	{
		/* tRFCab = 130 */
		{.trfc = 40, .trfrc_05t = 0, .txrefcnt_val = 59},
		/* tRFCab = 180 */
		{.trfc = 60, .trfrc_05t = 0, .txrefcnt_val = 79},
		/* tRFCab = 280 */
		{.trfc = 100, .trfrc_05t = 0, .txrefcnt_val = 119},
		/* tRFCab = 380 */
		{.trfc = 140, .trfrc_05t = 0, .txrefcnt_val = 159}
	},
	/* For freqGroup DDR3733 */
	{
		/* tRFCab = 130 */
		{.trfc = 48, .trfrc_05t = 1, .txrefcnt_val = 68},
		/* tRFCab = 180 */
		{.trfc = 72, .trfrc_05t = 0, .txrefcnt_val = 92},
		/* tRFCab = 280 */
		{.trfc = 118, .trfrc_05t = 1, .txrefcnt_val = 138},
		/* tRFCab = 380 */
		{.trfc = 165, .trfrc_05t = 0, .txrefcnt_val = 185}
	},
};

/* Optimize all-bank refresh parameters (by density) for LP4 */
void dramc_ac_timing_optimize(DRAMC_CTX_T *p)
{
	/* TRFC: tRFCab
	 *       Refresh Cycle Time (All Banks)
	 * TXREFCNT: tXSR max((tRFCab + 7.5ns), 2nCK)
	 *           Min self refresh time (Entry to Exit)
	 * execute_optimize: Indicate if ACTimings are updated
	 * at the end of this function
	 */
	unsigned char rf_cab_grp_idx = 0, freq_grp_idx = 0,
		execute_optimize = ENABLE;
	unsigned char trfc = 101, trfc_05_t = 0, tx_ref_cnt = 118;

	/*
	* Values retrieved from 1. Alaska ACTiming excel file
	* 2. JESD209-4B Refresh requirement table
	*/

	show_msg((INFO, "[ACTimingOptimize]"));

	/* already read MR8 for density update */
	/* Set tRFCab group idx p->density = MR8 OP[5:2] */
	switch (p->density) {
	case 0x0:	/* 4Gb per die  (2Gb per channel),  tRFCab=130 */
		rf_cab_grp_idx = tRFCAB_130;
		break;
	case 0x1:	/* 6Gb per die  (3Gb per channel),  tRFCab=180 */
	case 0x2:	/* 8Gb per die  (4Gb per channel),  tRFCab=180 */
		rf_cab_grp_idx = tRFCAB_180;
		break;
	case 0x3:	/* 12Gb per die (6Gb per channel),  tRFCab=280 */
	case 0x4:	/* 16Gb per die (8Gb per channel),  tRFCab=280 */
		rf_cab_grp_idx = tRFCAB_280;
		break;
	case 0x5:	/* 24Gb per die (12Gb per channel), tRFCab=380 */
	case 0x6:	/* 32Gb per die (16Gb per channel), tRFCab=380 */
		rf_cab_grp_idx = tRFCAB_380;
		break;
	default:
		execute_optimize = DISABLE;
		show_err("MR8 density err!\n");
	}

	switch (p->freqGroup) {
	case DDR1600_FREQ:
		freq_grp_idx = GRP_DDR1600_ACTIM;
		break;
	case DDR1866_FREQ:
		freq_grp_idx = GRP_DDR1866_ACTIM;
		break;
	case DDR2400_FREQ:
		freq_grp_idx = GRP_DDR2400_ACTIM;
		break;
	default:
		execute_optimize = DISABLE;
		show_err("freqGroup err!\n");
	}

	trfc = tRFCab_Opt[freq_grp_idx][rf_cab_grp_idx].trfc;
	trfc_05_t = tRFCab_Opt[freq_grp_idx][rf_cab_grp_idx].trfrc_05t;
	tx_ref_cnt = tRFCab_Opt[freq_grp_idx][rf_cab_grp_idx].txrefcnt_val;

	/*
	 * Only execute ACTimingOptimize(write to regs)
	 * when corresponding values have been found
	 */
	if (execute_optimize == ENABLE) {
		io_32_write_fld_align_all(
			DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM3),
			trfc, SHU_ACTIM3_TRFC);
		io_32_write_fld_align_all(
			DRAMC_REG_ADDR(DRAMC_REG_SHU_AC_TIME_05T), trfc_05_t,
			SHU_AC_TIME_05T_TRFC_05T);
		io_32_write_fld_align_all(DRAMC_REG_ADDR(DRAMC_REG_SHU_ACTIM4),
			tx_ref_cnt, SHU_ACTIM4_TXREFCNT);

		show_msg((INFO,
			"%s %u, TRFC %u, TRFC_05T %u, TXREFCNT %u\n",
			"Density (MR8 OP[5:2])",
			p->density, trfc, trfc_05_t, tx_ref_cnt));
	}
}

/*
 *  UpdateACTimingReg()
 *  ACTiming related register field update
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @param  ACTbl           Pointer to correct ACTiming table struct
 *  @retval status          (DRAM_STATUS_T): DRAM_OK or DRAM_FAIL
 */
DRAM_STATUS_T ddr_update_ac_timing_reg(DRAMC_CTX_T *p, const ACTime_T *ac_tbl)
{
	/*
	 * ac_tbl_final: Use to set correct ACTiming values and
	 * write into registers
	 * Variable used in step 1 (decide to use odt on or off ACTiming)
	 * ACTiming regs that have ODT on/off values ->
	 * declare variables to save the wanted value ->
	 * Used to retrieve correct SHU_ACTIM2_TR2W value and
	 * write into final register field
	 */
	ACTime_T ac_tbl_final;
	DRAM_ODT_MODE_T r2w_odt_onoff = p->odt_onoff;
	unsigned char trtw, trtw_05t, tmrr2w;
#ifdef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	unsigned char rankinctl = 0;
#endif
	/* Used to store tmp ACTiming values */
	unsigned char rodt_tracking_saveing_mck = 0, root = 0,
		tx_rank_in_ctl = 0, datlat_dsel = 0;
    //unsigned char tx_dly = 0, //need_check

	/* ACTiming regs that aren't currently in ACTime_T struct */
	unsigned char trefbw = 0;	/* REFBW_FR (tREFBW) for LP3 */

	if (ac_tbl == NULL)
		return DRAM_FAIL;
	ac_tbl_final = *ac_tbl;

	/*
	 * Step 1: Perform ACTiming table adjustments according to
	 * different usage/scenarios--
	 */
	r2w_odt_onoff = p->odt_onoff;

	/*
	 * ACTimings that have different values for odt on/off,
	 * retrieve the correct one and store in local variable
	 */
	if (r2w_odt_onoff == ODT_ON)	/* odt_on */ {
		trtw = ac_tbl_final.trtw_ODT_on;
		trtw_05t = ac_tbl_final.trtw_ODT_on_05T;
		tmrr2w = ac_tbl_final.tmrr2w_ODT_on;
	} else {	/* odt_off */
		trtw = ac_tbl_final.trtw_ODT_off;
		trtw_05t = ac_tbl_final.trtw_ODT_off_05T;
		tmrr2w = ac_tbl_final.tmrr2w_ODT_off;
	}

#if 0
	/* Override the above tRTW & tRTW_05T selection for Hynix LPDDR4P dram
	 * (always use odt_on's value for tRTW)
	 * (temp solution, need to discuss with SY)
	 */
	if (p->dram_type == TYPE_LPDDR4P) {
		trtw = ac_tbl_final.trtw_ODT_on;
		trtw_05t = ac_tbl_final.trtw_ODT_on_05T;
	}
#endif

	/*
	 * REFBW_FR in LP3 ACTiming excel file (value == 176 for all freqs),
	 * LP4 doesn't use this register -> set to 0
	 */
	trefbw = 0;

#if (ENABLE_RODT_TRACKING)
	/*
	 * set to 0, let TRTW & XRTR2W setting values
	 * are the smae with DV-sim's value that DE provided
	 */
	rodt_tracking_saveing_mck = 0;

#endif

	/* Update values that are used by rodt_tracking_saveing_mck */
	trtw = trtw - rodt_tracking_saveing_mck;
	ac_tbl_final.xrtr2w = ac_tbl_final.xrtr2w - rodt_tracking_saveing_mck;

	/* DATLAT related */
	datlat_dsel = ac_tbl_final.datlat - 3;

#ifndef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	ac_tbl_final.xrtr2r = 12;
#endif

	/* ----Step 2: Perform register writes for entries in ac_tbl_final
	 * struct & ACTiming excel file
	 * (all actiming adjustments should be done in Step 1)-------
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM0,
		p_fld(ac_tbl_final.trcd, SHU_ACTIM0_TRCD) |
		p_fld(ac_tbl_final.trrd, SHU_ACTIM0_TRRD) |
		p_fld(ac_tbl_final.twr, SHU_ACTIM0_TWR) |
		p_fld(ac_tbl_final.twtr, SHU_ACTIM0_TWTR));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM1,
		p_fld(ac_tbl_final.trc, SHU_ACTIM1_TRC) |
		p_fld(ac_tbl_final.tras, SHU_ACTIM1_TRAS) |
		p_fld(ac_tbl_final.trp, SHU_ACTIM1_TRP) |
		p_fld(ac_tbl_final.trpab, SHU_ACTIM1_TRPAB));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM2,
		p_fld(ac_tbl_final.tfaw, SHU_ACTIM2_TFAW) |
		p_fld(trtw, SHU_ACTIM2_TR2W) |
		p_fld(ac_tbl_final.trtp, SHU_ACTIM2_TRTP) |
		p_fld(ac_tbl_final.txp, SHU_ACTIM2_TXP));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM3,
		p_fld(ac_tbl_final.trfc, SHU_ACTIM3_TRFC) |
		p_fld(ac_tbl_final.refcnt, SHU_ACTIM3_REFCNT) |
		p_fld(ac_tbl_final.trfcpb, SHU_ACTIM3_TRFCPB));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM4,
		p_fld(ac_tbl_final.tzqcs, SHU_ACTIM4_TZQCS) |
		p_fld(ac_tbl_final.refcnt_fr_clk, SHU_ACTIM4_REFCNT_FR_CLK) |
		p_fld(ac_tbl_final.txrefcnt, SHU_ACTIM4_TXREFCNT));

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM5,
		p_fld(tmrr2w, SHU_ACTIM5_TMRR2W) |
		p_fld(ac_tbl_final.twtpd, SHU_ACTIM5_TWTPD) |
		p_fld(ac_tbl_final.trtpd, SHU_ACTIM5_TR2PD));

	if (p->dram_type == TYPE_PCDDR4) {
		io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM6,
			p_fld(ac_tbl_final.tbg_ccd, SHU_ACTIM6_BGTCCD) |
			p_fld(ac_tbl_final.tbg_wtr, SHU_ACTIM6_BGTWTR) |
			p_fld(ac_tbl_final.tbg_rrd, SHU_ACTIM6_BGTRRD));
	}

	io_32_write_fld_multi_all(DRAMC_REG_SHU_ACTIM_XRT,
		p_fld(ac_tbl_final.xrtw2w, SHU_ACTIM_XRT_XRTW2W) |
		p_fld(ac_tbl_final.xrtw2r, SHU_ACTIM_XRT_XRTW2R) |
		p_fld(ac_tbl_final.xrtr2w, SHU_ACTIM_XRT_XRTR2W) |
		p_fld(ac_tbl_final.xrtr2r, SHU_ACTIM_XRT_XRTR2R));

	/* AC timing 0.5T */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_AC_TIME_05T,
		p_fld(ac_tbl_final.twtr_05T, SHU_AC_TIME_05T_TWTR_M05T) |
		p_fld(trtw_05t, SHU_AC_TIME_05T_TR2W_05T) |
		p_fld(ac_tbl_final.twtpd_05T, SHU_AC_TIME_05T_TWTPD_M05T) |
		p_fld(ac_tbl_final.trtpd_05T, SHU_AC_TIME_05T_TR2PD_05T) |
		p_fld(ac_tbl_final.tfaw_05T, SHU_AC_TIME_05T_TFAW_05T) |
		p_fld(ac_tbl_final.trrd_05T, SHU_AC_TIME_05T_TRRD_05T) |
		p_fld(ac_tbl_final.twr_05T, SHU_AC_TIME_05T_TWR_M05T) |
		p_fld(ac_tbl_final.tras_05T, SHU_AC_TIME_05T_TRAS_05T) |
		p_fld(ac_tbl_final.trpab_05T, SHU_AC_TIME_05T_TRPAB_05T) |
		p_fld(ac_tbl_final.trp_05T, SHU_AC_TIME_05T_TRP_05T) |
		p_fld(ac_tbl_final.trcd_05T, SHU_AC_TIME_05T_TRCD_05T) |
		p_fld(ac_tbl_final.trtp_05T, SHU_AC_TIME_05T_TRTP_05T) |
		p_fld(ac_tbl_final.txp_05T, SHU_AC_TIME_05T_TXP_05T) |
		p_fld(ac_tbl_final.trfc_05T, SHU_AC_TIME_05T_TRFC_05T) |
		p_fld(ac_tbl_final.trfcpb_05T, SHU_AC_TIME_05T_TRFCPB_05T) |
		p_fld(ac_tbl_final.tbg_ccd_05T, SHU_AC_TIME_05T_BGTCCD_05T) |
		p_fld(ac_tbl_final.tbg_wtr_05T, SHU_AC_TIME_05T_BGTWTR_05T) |
		p_fld(ac_tbl_final.trc_05T, SHU_AC_TIME_05T_TRC_05T));

	/*
	 * CATRAIN_INTV isn't a shuffle register,
	 * but only affects LP4 CBT timing intv. during calibration
	 */
	/* CATRAIN_INTV is calculated based on CATRAINLAT = 0 */
	io_32_write_fld_multi_all(DRAMC_REG_CATRAINING1,
		p_fld(ac_tbl_final.r_dmcatrain_intv,
		CATRAINING1_CATRAIN_INTV) |
	#if NEED_REVIEW
		p_fld(0x5, CATRAINING1_CATRAINLAT));
	#else
		p_fld(CLEAR_FLD, CATRAINING1_CATRAINLAT));
	#endif

	/* DQSINCTL related */
	io_32_write_fld_align_all(DRAMC_REG_SHURK0_DQSCTL,
		ac_tbl_final.dqsinctl, SHURK0_DQSCTL_DQSINCTL);
	io_32_write_fld_align_all(DRAMC_REG_SHURK1_DQSCTL,
		ac_tbl_final.dqsinctl, SHURK1_DQSCTL_R1DQSINCTL);

	io_32_write_fld_align_all(DRAMC_REG_SHU_ODTCTRL,
		ac_tbl_final.dqsinctl, SHU_ODTCTRL_RODT);

	/* DATLAT related, tREFBW */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_CONF1,
		p_fld(ac_tbl_final.datlat, SHU_CONF1_DATLAT) |
		p_fld(datlat_dsel, SHU_CONF1_DATLAT_DSEL) |
		p_fld(datlat_dsel-1, SHU_CONF1_DATLAT_DSEL_PHY) |
		p_fld(trefbw, SHU_CONF1_REFBW_FR));

	/*
	 * FSPCHG_PRDCNT: LPDDR4 tFC constraint
	 */
	io_32_write_fld_align_all(DRAMC_REG_SHU_CONF2,
		ac_tbl_final.r_dmfspchg_prdcnt,
		SHU_CONF2_FSPCHG_PRDCNT);

	/*
	 * TODO: MRW_INTV can be set to different values for each freq,
	 * request new forumula/values from Berson
	 */
	io_32_write_fld_multi_all(DRAMC_REG_SHU_SCINTV,
		p_fld(ac_tbl_final.r_dmmrw_intv, SHU_SCINTV_MRW_INTV) |
		p_fld(ac_tbl_final.zqlat2, SHU_SCINTV_SCINTV)); /* cc change. REVIEW?? */

	/* CKEPRD - CKE pulse width */
	io_32_write_fld_align_all(DRAMC_REG_SHU_CKECTRL, ac_tbl_final.ckeprd,
		SHU_CKECTRL_CKEPRD);

	/*
	 * Step 3: Perform register writes/calculation for other regs
	 * (That aren't in ac_tbl_final struct)
	 */
#ifdef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY
	/*
	 * Ininital setting values are the same,
	 * RANKINCTL_RXDLY = RANKINCTL = RANKINCTL_ROOT1
	 * XRTR2R setting will be updated in RxdqsGatingPostProcess
	 */
	rankinctl =
		io_32_read_fld_align(DRAMC_REG_SHU_RANKCTL,
		SHU_RANKCTL_RANKINCTL);
	io_32_write_fld_align_all(DRAMC_REG_SHU_RANKCTL, rankinctl,
		SHU_RANKCTL_RANKINCTL_RXDLY);
#endif

	/* Update related RG of XRTW2W */
	if (p->frequency <= DDR1600_FREQ) {
		root = 0;
		tx_rank_in_ctl = 0;
		//tx_dly = 1; //need_check
	} else if (p->frequency <= DDR3200_FREQ) {
		root = 0;
		tx_rank_in_ctl = 1;
		//tx_dly = 2;
	} else if (p->frequency <= DDR3733_FREQ) {
		root = 1;
		tx_rank_in_ctl = 1;
		//tx_dly = 2;
	} else { /* (p->frequency == DDR4266_FREQ) */
		root = 1;
		tx_rank_in_ctl = 2;
		//tx_dly = 3;
	}
	io_32_write_fld_multi_all(DRAMC_REG_SHU_RANKCTL,
		p_fld(root, SHU_RANKCTL_TXRANKINCTL_ROOT) |
		p_fld(tx_rank_in_ctl, SHU_RANKCTL_TXRANKINCTL));

	return DRAM_OK;
}

/*
 * get_ac_timing_idx()
 *  Retrieve internal ac_timing_tbl's index according to dram type, freqGroup,
 * Read DBI status
 *  @param p                Pointer of context created by DramcCtxCreate.
 *  @retval timing_idx     Return ac_timing_tbl entry's index
 */
static unsigned char get_ac_timing_idx(DRAMC_CTX_T *p)
{
	unsigned char timing_idx = 0xff, tmp_idx;
	unsigned char tmp_dram_type = p->dram_type;

	/* LP4/LP4P/LP4X use same table */
#if 0
	if ((tmp_dram_type == TYPE_LPDDR4X) || (tmp_dram_type == TYPE_LPDDR4P))
		tmp_dram_type = TYPE_LPDDR4;
#endif

	/* p->frequency may not be in ACTimingTable, use p->freqGroup */
	/* LP4 byte/mixed mode dram both use byte mode ACTiming */
	for (tmp_idx = 0; tmp_idx < TOTAL_AC_TIMING_NUMBER; tmp_idx++) {
		if ((ac_timing_tbl[tmp_idx].dramType == tmp_dram_type) &&
			(ac_timing_tbl[tmp_idx].freqGroup == p->freqGroup) &&
			(ac_timing_tbl[tmp_idx].readDBI
			== p->dbi_r_onoff[p->dram_fsp]) &&
			(ac_timing_tbl[tmp_idx].cbtMode
			== get_dram_cbt_mode(p))
		   ) {
			timing_idx = tmp_idx;
			show_msg((INFO, "match AC timing %d\n", timing_idx));
			break;
		}
	}
	return timing_idx;
}

DRAM_STATUS_T ddr_update_ac_timing(DRAMC_CTX_T *p)
{
	unsigned char timing_idx = 0;

	show_msg3((INFO, "[UpdateACTiming] "));

	/* Retrieve ACTimingTable's corresponding index */
	timing_idx = get_ac_timing_idx(p);

	if (timing_idx == 0xff) {
		show_err("Error, no match AC timing, not apply table\n");
		return DRAM_FAIL;
	}
	/* Set ACTiming registers */
	show_msg3((INFO, "timing_idx = %d\n", timing_idx));
	ddr_update_ac_timing_reg(p, &ac_timing_tbl[timing_idx]);

	return DRAM_OK;
}

#if (FOR_DV_SIMULATION_USED == 0)
DRAM_STATUS_T ddr_update_ac_timing_emi(DRAMC_CTX_T *p,
		AC_TIMING_EXTERNAL_T *ac_reg_from_emi)
{
	unsigned char timing_idx = 0;
	ACTime_T ACTime;

	show_msg3((INFO, "[ddr_update_ac_timing_emi]\n"));

	if (ac_reg_from_emi == NULL)
		return DRAM_FAIL;

	/* Retrieve ACTimingTable's corresponding index */
	timing_idx = get_ac_timing_idx(p);
	ACTime = ac_timing_tbl[timing_idx];

	/* Overwrite AC timing from emi settings */
	ACTime.dramType = p->dram_type;
	/*Will use MDL ac timing, Others from internal ac timing*/
	ACTime.trp = ac_reg_from_emi->AC_TIME_EMI_TRP;
	ACTime.trpab = ac_reg_from_emi->AC_TIME_EMI_TRPAB;
	ACTime.trc = ac_reg_from_emi->AC_TIME_EMI_TRC;
	ACTime.trcd = ac_reg_from_emi->AC_TIME_EMI_TRCD;

	ACTime.trp_05T = ac_reg_from_emi->AC_TIME_EMI_TRP_05T;
	ACTime.trpab_05T = ac_reg_from_emi->AC_TIME_EMI_TRPAB_05T;
	ACTime.trc_05T = ac_reg_from_emi->AC_TIME_EMI_TRC_05T;
	ACTime.trcd_05T = ac_reg_from_emi->AC_TIME_EMI_TRCD_05T;

	/* Set ACTiming registers */
	ddr_update_ac_timing_reg(p, &ACTime);

	return DRAM_OK;
}
#endif

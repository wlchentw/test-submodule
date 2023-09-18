/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("Media Tek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MT_EMI_H
#define MT_EMI_H

#define DRAM_BASE 0x40000000ULL
#define DDR_BASE DRAM_BASE

int get_dram_channel_nr(void);
int get_dram_rank_nr(void);
void get_dram_rank_size(unsigned long long dram_rank_size[]);
typedef struct _AC_TIMING_EXTERNAL_T {
	/* U 00 */
	unsigned int AC_TIME_EMI_FREQUENCY : 16;
	unsigned int AC_TIME_EMI_TRAS : 8;
	unsigned int AC_TIME_EMI_TRP : 8;

	/* U 01 */
	unsigned int AC_TIME_EMI_TRPAB : 8;
	unsigned int AC_TIME_EMI_TRC : 8;
	unsigned int AC_TIME_EMI_TRFC : 8;
	unsigned int AC_TIME_EMI_TRFCPB : 8;

	/* U 02 */
	unsigned int AC_TIME_EMI_TXP : 8;
	unsigned int AC_TIME_EMI_TRTP : 8;
	unsigned int AC_TIME_EMI_TRCD : 8;
	unsigned int AC_TIME_EMI_TWR : 8;

	/* U 03 */
	unsigned int AC_TIME_EMI_TWTR : 8;
	unsigned int AC_TIME_EMI_TRRD : 8;
	unsigned int AC_TIME_EMI_TFAW : 8;
	unsigned int AC_TIME_EMI_TRTW_ODT_OFF : 4;
	unsigned int AC_TIME_EMI_TRTW_ODT_ON : 4;

	/* U 04 */
	unsigned int AC_TIME_EMI_REFCNT : 8;	/*(REFFRERUN = 0) */
	unsigned int AC_TIME_EMI_REFCNT_FR_CLK : 8;	/*(REFFRERUN = 1) */
	unsigned int AC_TIME_EMI_TXREFCNT : 8;
	unsigned int AC_TIME_EMI_TZQCS : 8;

	/* U 05 */
	unsigned int AC_TIME_EMI_TRTPD : 8;	/* LP4/LP3 */
	unsigned int AC_TIME_EMI_TWTPD : 8;	/* LP4/LP3 */
	unsigned int AC_TIME_EMI_TMRR2W_ODT_OFF : 8;	/* LP4 */
	unsigned int AC_TIME_EMI_TMRR2W_ODT_ON : 8;	/* LP4 */

	/* U 06 */
	/* Byte0 */
	unsigned int AC_TIME_EMI_TRAS_05T : 2;
	unsigned int AC_TIME_EMI_TRP_05T : 2;
	unsigned int AC_TIME_EMI_TRPAB_05T : 2;
	unsigned int AC_TIME_EMI_TRC_05T : 2;
	/* Byte1 */
	unsigned int AC_TIME_EMI_TRFC_05T : 2;
	unsigned int AC_TIME_EMI_TRFCPB_05T : 2;
	unsigned int AC_TIME_EMI_TXP_05T : 2;
	unsigned int AC_TIME_EMI_TRTP_05T : 2;
	/* Byte2 */
	unsigned int AC_TIME_EMI_TRCD_05T : 2;
	unsigned int AC_TIME_EMI_TWR_05T : 2;
	unsigned int AC_TIME_EMI_TWTR_05T : 2;
	unsigned int AC_TIME_EMI_TRRD_05T : 2;
	/* Byte3 */
	unsigned int AC_TIME_EMI_TFAW_05T : 2;
	unsigned int AC_TIME_EMI_TRTW_ODT_OFF_05T : 2;
	unsigned int AC_TIME_EMI_TRTW_ODT_ON_05T : 2;
	unsigned int AC_TIME_EMI_TRTPD_05T : 2;	/* LP4/LP3  */

	/* U 07 */
	/* Byte0 */
	unsigned int AC_TIME_EMI_TWTPD_05T : 2;	/* LP4/LP3 */
	unsigned int AC_TIME_EMI_TMRR2W_ODT_OFF_05T : 2; /* Useless, no 0.5T */
	unsigned int AC_TIME_EMI_TMRR2W_ODT_ON_05T : 2; /* Useless, no 0.5T */

} AC_TIMING_EXTERNAL_T;

typedef struct {
	unsigned int sub_version;	/* sub_version: 0x1 for new version */
	unsigned int type;
/*
typedef enum {
	TYPE_PCDDR3 = 0,
	TYPE_PCDDR4,
	TYPE_LPDDR3,
	TYPE_LPDDR4,
	TYPE_MAX,
} DRAM_DRAM_TYPE_T;
*/
	unsigned int id_length;	/* EMMC and NAND ID checking length */
	unsigned int fw_id_length;	/* FW ID checking length */
	unsigned char ID[16];
	unsigned char fw_id[8];	/* To save fw id */
	unsigned int EMI_CONA_VAL;	/*@0x3000 */
	unsigned int EMI_CONH_VAL;

	union {
		unsigned int DRAMC_ACTIME_UNION[8];
		AC_TIMING_EXTERNAL_T AcTimeEMI;
	};

	unsigned long long DRAM_RANK_SIZE[4];
	unsigned int EMI_CONF_VAL;
	unsigned int CHN0_EMI_CONA_VAL;
	unsigned int CHN1_EMI_CONA_VAL;
	/* Single field to store LP4 dram type (normal, byte, mixed) */
	unsigned int dram_cbt_mode_extern;
	unsigned int reserved[6];

	unsigned int iLPDDR3_MODE_REG_5;
	unsigned int PIN_MUX_TYPE;
} EMI_SETTINGS;

extern int emi_setting_index;
extern EMI_SETTINGS emi_settings[];
extern EMI_SETTINGS default_emi_setting;

#include <platform/x_hal_io.h>

/* =============pmic related api for ETT HQA test ============== */

//#define HVCORE_HVDRAM
#define NVCORE_NVDRAM
//#define LVCORE_LVDRAM

// MT8518A_M2 check JP78 or cd124
#define HQA_VDRAM_HV_PCDDR4		1260000
#define HQA_VDRAM_NV_PCDDR4		1200000
#define HQA_VDRAM_LV_PCDDR4		1140000

// MT8518A_M1 check J4 or CD10
#define HQA_VDRAM_HV_LPDDR4		1170000
#define HQA_VDRAM_NV_LPDDR4		1125000
#define HQA_VDRAM_LV_LPDDR4		1087500

// MT8518A + MT8518B use same vcore voltage
#define HQA_VCORE_HV			1050000
#define HQA_VCORE_NV			1000000
#define HQA_VCORE_LV			950000

// MT8518A_M2 check JP79
#define HQA_VCORE_HV_PCDDR4		HQA_VCORE_HV
#define HQA_VCORE_NV_PCDDR4		HQA_VCORE_NV
#define HQA_VCORE_LV_PCDDR4		HQA_VCORE_LV

// MT8518A_M1 check J1
#define HQA_VCORE_HV_LPDDR4		HQA_VCORE_HV
#define HQA_VCORE_NV_LPDDR4		HQA_VCORE_NV
#define HQA_VCORE_LV_LPDDR4		HQA_VCORE_LV


#define STD_VCORE	800000
#define STD_VIO18_CAL	0x0
#define STD_VIO18_TRIM	0x0
#define STD_VDRAM	1125000
#define STD_VDDQ	600000
#define STD_VDDQ_4P	400000

#define MAX_VSRAM	1309375
#define MAX_VCORE	1196875
#define MAX_VDRAM	1309375
#define MAX_VDDQ	1190625

#define UNIT_VCORE	6250
#define UNIT_VIO18	10000
#define UNIT_VDRAM	6250
#define UNIT_VDDQ	6250

#define SEL_PREFIX_VCORE	STD_VCORE
#define SEL_PREFIX_VIO18_CAL	STD_VIO18_CAL
#define SEL_PREFIX_VIO18_TRIM	STD_VIO18_TRIM
#define SEL_PREFIX_VDRAM	STD_VDRAM
#define SEL_PREFIX_VDDQ	STD_VDDQ

typedef struct {
	unsigned long long full_sys_addr;
	unsigned int addr;
	unsigned int row;
	unsigned int col;
	unsigned char ch;
	unsigned char rk;
	unsigned char bk;
	unsigned char dummy;
} dram_addr_t;

unsigned int get_dummy_read_addr(dram_addr_t *dram_addr);

#endif

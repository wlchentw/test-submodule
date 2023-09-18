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

#include <debug.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <platform.h>
#include <reg.h>
#include "platform/mt_reg_base.h"
#include <platform/typedefs.h>
#include <platform/dramc_common.h>
#include <platform/emi.h>
#include <platform/custom_emi.h>

#if WITH_PMIC_MT6395
#include <platform/pmic_6395.h>
#endif

#if WITH_MTK_PMIC_WRAP_AND_PMIC
#include <platform/pmic_wrap_init.h>
#include <platform/pmic.h>
#endif

#if WITH_VCORE_PWM_BUCK
#include <platform/pwm-buck.h>
#elif WITH_VCORE_I2C_BUCK
#include <platform/rt5748.h>
#endif

static int dram_main(void)
{
	int ret = 0;
	EMI_SETTINGS *emi_set;

	if (emi_setting_index == -1)
		emi_set = &default_emi_setting;

	show_msg((INFO, "8518A-M1V1 TYPE_LPDDR4: Dram Type: %d, \n" \
		"8518A-M2V1 TYPE_PCDDR4: Dram Type: %d, \n" \
		"8518B-M1V1 TYPE_LPDDR3: Dram Type: %d, \n" \
		"8518B-M2V1 TYPE_PCDDR3: Dram Type: %d, \n",
		TYPE_LPDDR4, TYPE_PCDDR4, TYPE_LPDDR3, TYPE_PCDDR3));

//vmem adjust
#if WITH_PMIC_MT6395
#if defined(LVCORE_LVDRAM)
	pmic_config_interface(0x20, 0x1, 0x3, 0); // -5%
	show_msg((INFO, "8518A-M2V1, 8518A-P1V1 and all 8518B vmem vdram LV:-5%%\n"));
#elif defined(HVCORE_HVDRAM)
	pmic_config_interface(0x20, 0x2, 0x3, 0); // +5%
	show_msg((INFO, "8518A-M2V1, 8518A-P1V1 and all 8518B vmem vdram HV:+5%%\n"));
#else
	show_msg((INFO, "8518A-M2V1, 8518A-P1V1 and all 8518B vmem vdram NV:+0%%\n"));
#endif
#else
	show_msg((INFO, "8518A-M1V1 no support software vmem vdram adjust!\n"
	"please connect hardware team rework EVB to adjust voltage!\n"));
#endif

//vcore adjust
#if WITH_VCORE_I2C_BUCK // only 8518A-M2V1 and 8518A-P1V1 have rt5748 to adjust vcore buck
#if defined(LVCORE_LVDRAM)
	rt5748_config_interface(0x00, 0x82, 0xff, 0, rt5748_I2C_ID_M2); //0.95v -5%
	show_msg((INFO, "8518A-M2V1 and 8518A-P1V1 vcore LV:-5%% (0.95v)\n"));
#elif defined(HVCORE_HVDRAM)
	rt5748_config_interface(0x00, 0x96, 0xff, 0, rt5748_I2C_ID_M2); //1.05v +5%
	show_msg((INFO, "8518A-M2V1 and 8518A-P1V1 vcore HV:+5%% (1.05v)\n"));
#else
	show_msg((INFO, "8518A-M2V1 and 8518A-P1V1 vcore NV:+0%% (1.00v)\n"));
#endif
#elif WITH_MTK_PMIC_WRAP_AND_PMIC // only 8518A-M1V1 have MT6391 to adjust vcore
#if defined(LVCORE_LVDRAM)
	pmic_config_interface(0x027A, 0x28, 0x7f, 0);
	pmic_config_interface(0x0278, 0x28, 0x7f, 0);  //0.95v -5%
	show_msg((INFO, "8518A-M1V1 vcore LV:-5%% (0.95v)\n"));
#elif defined(HVCORE_HVDRAM)
	pmic_config_interface(0x027A, 0x38, 0x7f, 0);
	pmic_config_interface(0x0278, 0x38, 0x7f, 0);  //1.05v +5%
	show_msg((INFO, "8518A-M1V1 vcore HV:+5%% (1.05v)\n"));
#else
	pmic_config_interface(0x027A, 0x30, 0x7f, 0);
	pmic_config_interface(0x0278, 0x30, 0x7f, 0);  //1.00v
	show_msg((INFO, "8518A-M1V1 vcore NV:+0%% (1.00v)\n"));
#endif
#elif WITH_VCORE_PWM_BUCK  // 8518B all borads have pwm(rt8097) to adjust vcore buck
#if defined(LVCORE_LVDRAM)
	regulator_set_voltage(900000);    //0.9v -10%
	show_msg((INFO, "8518B vcore LV (0.9v)\n"));
#elif defined(HVCORE_HVDRAM)
	show_msg((INFO, "8518B no support software vcore HV(1.1v) adjust!\n"
	"please connect hardware team rework EVB to adjust voltage!\n"));
#else
	show_msg((INFO, "8518B vcore NV (1.0v)\n"));
#endif
	show_msg((INFO, "8518B get vcore voltage:%d\n", regulator_get_voltage()));
#endif

	/* create mapping for DRAM access */
	ret = init_dram((emi_set->type & 0xF), emi_set->dram_cbt_mode_extern,
			NULL, NORMAL_USED);

	return ret;
}

void mt_mem_init(void)
{
    dram_main();
}

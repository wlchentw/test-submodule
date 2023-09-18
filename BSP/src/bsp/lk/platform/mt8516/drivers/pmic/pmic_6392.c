/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
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
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */
#include <reg.h>
#include <platform/pmic_wrap.h>
#include <platform/mtk_timer.h>
#include <platform/pmic.h>

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC-Charger Type Detection
//////////////////////////////////////////////////////////////////////////////////////////
CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
int g_first_check=0;

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC access API
//////////////////////////////////////////////////////////////////////////////////////////
u32 pmic_read_interface (u32 RegNum, u32 *val, u32 MASK, u32 SHIFT)
{
	u32 return_value = 0;
	u32 pmic6392_reg = 0;
	u32 rdata;

	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic6392_reg=rdata;
	if (return_value!=0) {
		printf("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	pmic6392_reg &= (MASK << SHIFT);
	*val = (pmic6392_reg >> SHIFT);

	return return_value;
}

u32 pmic_config_interface (u32 RegNum, u32 val, u32 MASK, u32 SHIFT)
{
	u32 return_value = 0;
	u32 pmic6392_reg = 0;
	u32 rdata;

	if (val > MASK) {
		printf("[pmic_config_interface] Invalid data, Reg[%x]: MASK = 0x%x, val = 0x%x\n",
			RegNum, MASK, val);
		return E_PWR_INVALID_DATA;
	}

	return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
	pmic6392_reg=rdata;
	if (return_value!=0) {
		printf("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	pmic6392_reg &= ~(MASK << SHIFT);
	pmic6392_reg |= (val << SHIFT);

	return_value= pwrap_wacs2(1, (RegNum), pmic6392_reg, &rdata);
	if (return_value!=0) {
		printf("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
		return return_value;
	}

	return return_value;
}

void upmu_set_rg_bc11_bb_ctrl(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON18),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_BB_CTRL_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_BB_CTRL_SHIFT)
	                         );
}

void upmu_set_rg_bc11_rst(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON18),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_RST_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_RST_SHIFT)
	                         );
}

void upmu_set_rg_bc11_vsrc_en(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON18),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_VSRC_EN_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_VSRC_EN_SHIFT)
	                         );
}

u32 upmu_get_rgs_bc11_cmp_out(void)
{
	u32 ret=0;
	u32 val=0;

	ret=pmic_read_interface( (u32)(MT6392_CHR_CON18),
	                         (&val),
	                         (u32)(MT6392_PMIC_RGS_BC11_CMP_OUT_MASK),
	                         (u32)(MT6392_PMIC_RGS_BC11_CMP_OUT_SHIFT)
	                       );

	return val;
}

void upmu_set_rg_bc11_vref_vth(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON19),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_VREF_VTH_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_VREF_VTH_SHIFT)
	                         );
}

void upmu_set_rg_bc11_cmp_en(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON19),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_CMP_EN_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_CMP_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_ipd_en(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON19),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_IPD_EN_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_IPD_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_ipu_en(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON19),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_IPU_EN_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_IPU_EN_SHIFT)
	                         );
}

void upmu_set_rg_bc11_bias_en(u32 val)
{
	u32 ret=0;

	ret=pmic_config_interface( (u32)(MT6392_CHR_CON19),
	                           (u32)(val),
	                           (u32)(MT6392_PMIC_RG_BC11_BIAS_EN_MASK),
	                           (u32)(MT6392_PMIC_RG_BC11_BIAS_EN_SHIFT)
	                         );
}

extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);

void hw_bc11_init(void)
{
	Charger_Detect_Init();

	//RG_BC11_BIAS_EN=1
	upmu_set_rg_bc11_bias_en(0x1);
	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//BC11_RST=1
	upmu_set_rg_bc11_rst(0x1);
	//BC11_BB_CTRL=1
	upmu_set_rg_bc11_bb_ctrl(0x1);

	mdelay(100);
}


u32 hw_bc11_DCD(void)
{
	u32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1.0] = 10
	upmu_set_rg_bc11_ipu_en(0x2);
	//RG_BC11_IPD_EN[1.0] = 01
	upmu_set_rg_bc11_ipd_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=01
	upmu_set_rg_bc11_vref_vth(0x1);
	//RG_BC11_CMP_EN[1.0] = 10
	upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(400);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);

	return wChargerAvail;
}


u32 hw_bc11_stepA1(void)
{
	u32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1.0] = 10
	upmu_set_rg_bc11_ipu_en(0x2);
	//RG_BC11_VREF_VTH = [1:0]=10
	upmu_set_rg_bc11_vref_vth(0x2);
	//RG_BC11_CMP_EN[1.0] = 10
	upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


u32 hw_bc11_stepB1(void)
{
	u32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1.0] = 01
	//upmu_set_rg_bc11_ipu_en(0x1);
	upmu_set_rg_bc11_ipd_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=10
	//upmu_set_rg_bc11_vref_vth(0x2);
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 01
	upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}


u32 hw_bc11_stepC1(void)
{
	u32 wChargerAvail = 0;

	//RG_BC11_IPU_EN[1.0] = 01
	upmu_set_rg_bc11_ipu_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=10
	upmu_set_rg_bc11_vref_vth(0x2);
	//RG_BC11_CMP_EN[1.0] = 01
	upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);

	return  wChargerAvail;
}


u32 hw_bc11_stepA2(void)
{
	u32 wChargerAvail = 0;

	//RG_BC11_VSRC_EN[1.0] = 10
	upmu_set_rg_bc11_vsrc_en(0x2);
	//RG_BC11_IPD_EN[1:0] = 01
	upmu_set_rg_bc11_ipd_en(0x1);
	//RG_BC11_VREF_VTH = [1:0]=00
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 01
	upmu_set_rg_bc11_cmp_en(0x1);

	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


 u32 hw_bc11_stepB2(void)
 {
	u32 wChargerAvail = 0;

	//RG_BC11_VSRC_EN[1:0]= 01
	upmu_set_rg_bc11_vsrc_en(0x1);
	//RG_BC11_IPD_EN[1:0]= 10
	upmu_set_rg_bc11_ipd_en(0x2);
	//RG_BC11_VREF_VTH = [1:0]= 00
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 10
	upmu_set_rg_bc11_cmp_en(0x2);

	mdelay(80);

	wChargerAvail = upmu_get_rgs_bc11_cmp_out();

	//RG_BC11_VSRC_EN[1:0]= 00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);

	return  wChargerAvail;
}


void hw_bc11_done(void)
{
	//RG_BC11_VSRC_EN[1:0]=00
	upmu_set_rg_bc11_vsrc_en(0x0);
	//RG_BC11_VREF_VTH = [1:0]=0
	upmu_set_rg_bc11_vref_vth(0x0);
	//RG_BC11_CMP_EN[1.0] = 00
	upmu_set_rg_bc11_cmp_en(0x0);
	//RG_BC11_IPU_EN[1.0] = 00
	upmu_set_rg_bc11_ipu_en(0x0);
	//RG_BC11_IPD_EN[1.0] = 00
	upmu_set_rg_bc11_ipd_en(0x0);
	//RG_BC11_BIAS_EN=0
	upmu_set_rg_bc11_bias_en(0x0);

	Charger_Detect_Release();
}

CHARGER_TYPE hw_charger_type_detection(void)
{
	CHARGER_TYPE charger_tye;

	/********* Step initial  ***************/
	hw_bc11_init();

	/********* Step DCD ***************/
	if (1 == hw_bc11_DCD()) {
		/********* Step A1 ***************/
		if (1 == hw_bc11_stepA1()) {
			/********* Step B1 ***************/
			if (1 == hw_bc11_stepB1()) {
				//charger_tye = NONSTANDARD_CHARGER;
				//printf("step B1 : Non STANDARD CHARGER!\r\n");
				charger_tye = APPLE_2_1A_CHARGER;
				printf("step B1 : Apple 2.1A CHARGER!\r\n");
			} else {
				//charger_tye = APPLE_2_1A_CHARGER;
				//printf("step B1 : Apple 2.1A CHARGER!\r\n");
				charger_tye = NONSTANDARD_CHARGER;
				printf("step B1 : Non STANDARD CHARGER!\r\n");
			}
		} else {
			/********* Step C1 ***************/
			if (1 == hw_bc11_stepC1()) {
				charger_tye = APPLE_1_0A_CHARGER;
				printf("step C1 : Apple 1A CHARGER!\r\n");
			} else {
				charger_tye = APPLE_0_5A_CHARGER;
				printf("step C1 : Apple 0.5A CHARGER!\r\n");
			}
		}

	} else {
		/********* Step A2 ***************/
		if (1 == hw_bc11_stepA2()) {
			/********* Step B2 ***************/
			if (1 == hw_bc11_stepB2()) {
				charger_tye = STANDARD_CHARGER;
				printf("step B2 : STANDARD CHARGER!\r\n");
			} else {
				charger_tye = CHARGING_HOST;
				printf("step B2 :  Charging Host!\r\n");
			}
		} else {
			charger_tye = STANDARD_HOST;
			printf("step A2 : Standard USB Host!\r\n");
		}

	}

	/********* Finally setting *******************************/
	hw_bc11_done();

	return charger_tye;
}

CHARGER_TYPE mt_charger_type_detection(void)
{
	if ( g_first_check == 0 ) {
		g_first_check = 1;
		g_ret = hw_charger_type_detection();
	} else {
		printf("[mt_charger_type_detection] Got data !!, %d, %d\r\n", g_first_check);
	}

	return g_ret;
}

u32 pmic_IsUsbCableIn (void)
{
	u32 ret=0;
	u32 val=0;

	ret=pmic_read_interface( (u32)(MT6392_CHR_CON0),
	                         (&val),
	                         (u32)(MT6392_PMIC_RGS_CHRDET_MASK),
	                         (u32)(MT6392_PMIC_RGS_CHRDET_SHIFT)
	                       );

	if (val)
		return PMIC_CHRDET_EXIST;
	else
		return PMIC_CHRDET_NOT_EXIST;
}

void PMIC_INIT_SETTING_V1(void)
{

}

//==============================================================================
// PMIC6392 Init Code
//==============================================================================
void pmic_init (void)
{
	printf("[pmic6392_init] LK Start..................\n");

	/* pmic initial setting */
	PMIC_INIT_SETTING_V1();

	printf("[pmic6392_init] Done...................\n");
}


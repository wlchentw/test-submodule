/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 *
 * $Author: zhishang.liu $
 * $Date: 2019/24/5 $
 * $RCSfile: pi_main.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file pi_main.c
 *  Basic PSRAMC API implementation
 */

#ifdef SUPPORT_TYPE_PSRAM

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include "..\Common\pd_common.h"
//#include "Register.h"
//=============================================================================
//  Include Files
//=============================================================================
#if (FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0)
#if __ETT__
#include <common.h>
#include <ett_common.h>
#include <api.h>
#endif
#endif

#define DV_SIMULATION_INIT_C    1
#define DV_SIMULATION_BEFORE_K  1
#define DV_SIMULATION_MIOCKJMETER  1
#define DV_SIMULATION_SW_IMPED 0
#define DV_SIMULATION_LP4_ZQ 1
#define DV_SIMULATION_CA_TRAINING 0
#define DV_SIMULATION_WRITE_LEVELING  0
#define DV_SIMULATION_GATING 1
#define DV_SIMULATION_DATLAT 1
#define DV_SIMULATION_RX_PERBIT    1
#define DV_SIMULATION_TX_PERBIT    1// Please enable with write leveling
#define DV_SIMULATION_AFTER_K   1
#define DV_SIMULATION_DBI_ON   0
#define DV_SIMULATION_RUNTIME_CONFIG 0
#define DV_SIMULATION_RUN_TIME_MRW 0
//U8 gu1BroadcastIsLP4 = TRUE;

#if (FOR_DV_SIMULATION_USED==0)
#include <platform/emi.h>
#endif
#include <platform/dramc_common.h>
#include <platform/dramc_api.h>
#include <platform/x_hal_io.h>
#if (FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0)
#ifndef MT6761_FPGA
//#include <pmic.h>
#endif
#endif

#if ! __ETT__
#if (FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0)
//#include <platform/pmic.h>
#endif
#endif

#if 1
void DPI_SW_main_PSRAM(DRAMC_CTX_T *DramConfig)
#else
void DPI_SW_main_LP4(DRAMC_CTX_T *DramConfig)
#endif
{
	DRAM_STATUS_T VrefStatus;
	//U8 gu1BroadcastIsLP4 = TRUE;
	DramConfig->support_channel_num = CHANNEL_SINGLE;
	DramConfig->channel = CHANNEL_A;
	DramConfig->support_rank_num = RANK_SINGLE;
		// DramRank
	DramConfig->rank = RANK_0;
	DramConfig->freq_sel = PSRAM_1600;
	DramConfig->shu_type = DRAM_DFS_SHUFFLE_1;
	// DRAM type
	DramConfig->dram_type = TYPE_PSRAM;
	// DRAM Fast switch point type, only for LP4, useless in LP3
	// bus width
	DramConfig->data_width = DATA_WIDTH_8BIT;
	// DRAMC internal test engine-2 parameters in calibration
	DramConfig->test2_1 = DEFAULT_TEST2_1_CAL;
	DramConfig->test2_2 = DEFAULT_TEST2_2_CAL;
	// DRAMC test pattern in calibration
	DramConfig->test_pattern = TEST_XTALK_PATTERN;
	// DRAMC operation clock frequency in MHz
	DramConfig->frequency = 800;
	// u2DelayCellTimex100
	DramConfig->u2DelayCellTimex100 = 0;
	DramConfig->vendor_id = 0x1;
	DramConfig->density = 0;
	//DramConfig->ranksize = {0,0};

	DramConfig->enable_rx_scan_vref =DISABLE;
	DramConfig->enable_tx_scan_vref =DISABLE;

	//DramcBroadcastOnOff(DRAMC_BROADCAST_ON); //LP4 broadcast on

	//Global_Option_Init(DramConfig);
	Psram_Global_Option_Init2(DramConfig);

	vPsramcInit_PreSettings(DramConfig);

	PsramPhyFreqSel(DramConfig, DramConfig->freq_sel);

    mcSHOW_DBG_MSG("PSRAM calibration start!\n");

#if DV_SIMULATION_INIT_C
	PSramcInitSetting(DramConfig);
#if DV_SIMULATION_MIOCKJMETER
#ifdef ENABLE_MIOCK_JMETER
	PSramcMiockJmeter(DramConfig);
#endif
#endif
#endif

 #if ENABLE_RX_TRACKING_LP4
// DramcRxInputDelayTrackingInit_byFreq(DramConfig);
// DramcRxInputDelayTrackingInit_Common(DramConfig);
// DramcRxInputDelayTrackingHW(DramConfig);
 PsramRxInputTrackingSetting(DramConfig);
 #endif

#if DV_SIMULATION_BEFORE_K
	vApplyPsramConfigBeforeCalibration(DramConfig);
#endif

#if DV_SIMULATION_GATING
    //DramcRxdqsGatingPreProcess(DramConfig);
#endif


    #if DV_SIMULATION_GATING
		PSramcRxdqsGatingCal(DramConfig);
    #endif


    #if DV_SIMULATION_TX_PERBIT
		PSramcTxWindowPerbitCal(DramConfig, TX_DQ_DQS_MOVE_DQ_DQM, FALSE);
		VrefStatus = PSramcTxWindowPerbitCal((DRAMC_CTX_T *) DramConfig, TX_DQ_DQS_MOVE_DQ_ONLY,DramConfig->enable_tx_scan_vref);
    #endif

    #if DV_SIMULATION_DATLAT
		PSramcRxdatlatScan(DramConfig,fcDATLAT_USE_DEFAULT);
    #endif

    #if DV_SIMULATION_RX_PERBIT
		PSramcRxWindowPerbitCal(DramConfig, 1);
    #endif

#if DV_SIMULATION_AFTER_K
	vApplyPsramConfigAfterCalibration(DramConfig);
#endif


#if DV_SIMULATION_RUN_TIME_MRW
	enter_pasr_dpd_config(0, 0xFF);
#endif

}

#endif

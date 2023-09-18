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
 * $Author: Zhishang.Liu $
 * $Date: 2019/24/5 $
 * $RCSfile: pi_calibration_api.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file pi_calibration_api.c
 *  Basic PSRAMC calibration API implementation
 */

#ifdef SUPPORT_TYPE_PSRAM

//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include "..\Common\pd_common.h"
#include <platform/dramc_common.h>
#include <platform/x_hal_io.h>

#include <platform/dramc_api.h>


/* Definitions to make IMPCAL_VREF_SEL function more readable */
#define IMPCAL_STAGE_DRVP     1
#define IMPCAL_STAGE_DRVN     2
#define IMPCAL_STAGE_TRACKING 3

/* LP4 IMP_VREF_SEL ============================== */
#define IMP_LP4_VREF_SEL               0x1b
#define IMP_LP3_VREF_SEL               0x2b

/* LP4X IMP_VREF_SEL ============================== */
#define IMP_LP4X_TERM_VREF_SEL     0x1b

/* LP4X IMP_VREF_SEL w/o term ==== */
#if (ENABLE_DQ3200_UNTERM == 1)
    #define IMP_DRVP_LP4X_UNTERM_VREF_SEL   0x3a
    #define IMP_DRVN_LP4X_UNTERM_VREF_SEL   0x2a
    #define IMP_TRACK_LP4X_UNTERM_VREF_SEL  0x3a
#else
    #define IMP_DRVP_LP4X_UNTERM_VREF_SEL   0x1a
    #define IMP_DRVN_LP4X_UNTERM_VREF_SEL   0x16
    #define IMP_TRACK_LP4X_UNTERM_VREF_SEL  0x1a
#endif

#if (SW_CHANGE_FOR_SIMULATION ||FOR_DV_SIMULATION_USED)
#define TX_VREF_RANGE_BEGIN       0
#define TX_VREF_RANGE_END           2 // binary 110010
#define TX_VREF_RANGE_STEP         2
#else
#define TX_VREF_RANGE_BEGIN       16
#define TX_VREF_RANGE_END           50 // binary 110010
#define TX_VREF_RANGE_STEP         2
#endif
#define TX_DQ_UI_TO_PI_TAP         64 // 1 PI = tCK/64, total 128 PI, 1UI = 32 PI
#define LP4_TX_VREF_DATA_NUM 50
#define LP4_TX_VREF_PASS_CONDITION 0
#define LP4_TX_VREF_BOUNDARY_NOT_READY 0xff

typedef struct _PASS_WIN_DATA_BY_VREF_T
{
    U16 u2VrefUsed;
    U16 u2WinSum_byVref;
    U16 u2MinWinSize_byVref;
} PASS_WIN_DATA_BY_VREF_T;

#ifdef FOR_HQA_TEST_USED
U16 gFinalRXPerbitWin[CHANNEL_NUM][RANK_MAX][DQ_DATA_WIDTH];
U16 gFinalTXPerbitWin[CHANNEL_NUM][RANK_MAX][DQ_DATA_WIDTH];
U16 gFinalTXPerbitWin_min_max[CHANNEL_NUM][RANK_MAX];
U16 gFinalTXPerbitWin_min_margin[CHANNEL_NUM][RANK_MAX];
U16 gFinalTXPerbitWin_min_margin_bit[CHANNEL_NUM][RANK_MAX];
S8 gFinalClkDuty[CHANNEL_NUM];
U32 gFinalClkDutyMinMax[CHANNEL_NUM][2];
S8 gFinalDQSDuty[CHANNEL_NUM][DQS_NUMBER];
U32 gFinalDQSDutyMinMax[CHANNEL_NUM][DQS_NUMBER][2];
#endif

#if (EYESCAN_LOG || PSRAM_RX_EYE)
#define VREF_TOTAL_NUM_WITH_RANGE 51+30 //range0 0~50 + range1 21~50
#define EYESCAN_BROKEN_NUM 3
#define EYESCAN_DATA_INVALID 0x7fff
S16  gEyeScan_Min[VREF_TOTAL_NUM_WITH_RANGE][DQ_DATA_WIDTH_PSRAM][EYESCAN_BROKEN_NUM];
S16  gEyeScan_Max[VREF_TOTAL_NUM_WITH_RANGE][DQ_DATA_WIDTH_PSRAM][EYESCAN_BROKEN_NUM];
U16  gEyeScan_CaliDelay[DQS_NUMBER];
U8  gEyeScan_WinSize[VREF_TOTAL_NUM_WITH_RANGE][DQ_DATA_WIDTH_PSRAM];
S8  gEyeScan_DelayCellPI[DQ_DATA_WIDTH_PSRAM];
U8  gEyeScan_ContinueVrefHeight[DQ_DATA_WIDTH_PSRAM];
U16  gEyeScan_TotalPassCount[DQ_DATA_WIDTH_PSRAM];
#endif

#define PASS_RANGE_NA   0x7fff
#if (SW_CHANGE_FOR_SIMULATION ||FOR_DV_SIMULATION_USED)
#define RX_VREF_RANGE_BEGIN       0
#define RX_VREF_RANGE_END          2
#define RX_VREF_RANGE_STEP         2
#else
#define RX_VREF_RANGE_BEGIN_ODT_OFF     16
#define RX_VREF_RANGE_BEGIN_ODT_ON      6
#define RX_VREF_RANGE_END          31   //bit5 useless (Justin)   //field is 6 bit, but can only use 0~31
#define RX_VREF_RANGE_STEP         1
#endif

/* LP4P IMP_VREF_SEL ============================== */
#define IMP_DRVP_LP4P_VREF_SEL        0x13
#define IMP_DRVN_LP4P_VREF_SEL        0xf
#define IMP_TRACK_LP4P_VREF_SEL       0x13
static U32 gDramcSwImpedanceResule[2][4]={{0,0,0,0},{0,0,0,0}};//ODT_ON/OFF x DRVP/DRVN/ODTP/ODTN
U16 u2gdelay_cell_ps;
static U8 ucg_num_dlycell_perT = 49; //from 60807
static U16 u2rx_window_sum;
U8 gFinalRXVrefDQ[CHANNEL_NUM][RANK_MAX];
U8 gFinalTXVrefDQ[CHANNEL_NUM][RANK_MAX];

static unsigned short rx_perbit_win_min_max = 0;
static unsigned short rx_perbit_win_min_max_idx = 0;

extern const U8  uiPSRAM_DQ_Mapping_POP[CHANNEL_NUM][8];

#if PINMUX_AUTO_TEST_PER_BIT_RX
U16 gPsramFinalRXPerbitWinSiz[CHANNEL_NUM][8];
#endif
#if PINMUX_AUTO_TEST_PER_BIT_TX
U16 gPsramFinalTXPerbitFirstPass[CHANNEL_NUM][8];
#endif

/* Impedance have a total of 19 steps, but the HW value mapping to hardware is 0~15, 29~31
* This function adjusts passed value u1ImpVal by adjust step count "u1AdjStepCnt"
* After adjustment, if value is 1. Too large (val > 31) -> set to max 31
*                               2. Too small (val < 0) -> set to min 0
*                               3. Value is between 15 & 29, adjust accordingly ( 15 < value < 29 )
* returns: Impedance value after adjustment
*/

#if PINMUX_AUTO_TEST_PER_BIT_RX
U8 gPsram_RX_check_per_bit_flag = 0;
U8 gPsram_RX_Check_per_bit_idx = 0;

void CheckPsramRXDelayCell(DRAMC_CTX_T *p)
{
    S32 ii;
    for (ii = 0; ii < 4; ii++)
    {
        mcSHOW_DBG_MSG2("#### B0[%x]", u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2 + ii*4)));
    }
}


void SetPsramRxPerBitDelayCellForPinMuxCheck(DRAMC_CTX_T *p, U8 U1bitIdx)
{
    U32 u4delay = 0x3f;
    switch(U1bitIdx)
    {
        case 0:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2),
                P_Fld(u4delay, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0)); //bit0
            break;
        case 1:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2),
                P_Fld(u4delay, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0));//bit1
            break;
        case 2:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ3),
                P_Fld(u4delay, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0)); //bit2
            break;
        case 3:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ3),
                P_Fld(u4delay, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0)); //bit3
            break;
        case 4:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ4),
                P_Fld(u4delay, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0)); //bit4
            break;
        case 5:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ4),
                P_Fld(u4delay, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0)); //bit5
            break;
        case 6:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ5),
                P_Fld(u4delay, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0)); //bit6
            break;
        case 7:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ5),
                P_Fld(u4delay, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0) |
                P_Fld(u4delay, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0)); //bit7
            break;
        case 8:
            vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ2),
                P_Fld(u4delay, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1) |
                P_Fld(u4delay, SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1)); //bit8
            break;
        default:
            mcSHOW_DBG_MSG("Not assign correct index\n");
    }
}

void CheckPsramRxPinMux(DRAMC_CTX_T *p)//By view of DRAM in LP4
{
    U8 u1Rxdelay[8] = {0};
    U8 u1RxIndex[8] = {0};
    U8 i = 0;
    U8 j = 0;
    U16 u2Min = 0xffff;
    U8 u1MinIdx = 0;

    gPsram_RX_check_per_bit_flag = 1;

    for (i = 0; i < 8; i +=1)
    {
        gPsram_RX_Check_per_bit_idx = i;

        PSramcRxWindowPerbitCal(p, 1); //RDDQC

        u2Min = 0xffff;

        for(j = 0; j < 8; j++)
        {
            if(gPsramFinalRXPerbitWinSiz[p->channel][j] <= u2Min)
            {
                u2Min = gPsramFinalRXPerbitWinSiz[p->channel][j];
                u1MinIdx = j;
            }
        }
        u1RxIndex[i] = u1MinIdx;
    }

    for(i = 0; i < 8; i++)
    {
        mcSHOW_DBG_MSG("CH[%d] Rank[%d] APHY_RX[%d]-->DRAMC(-->RDDQC-->O1)[%d]\n", p->channel, p->rank, i, u1RxIndex[i]);
        if(u1RxIndex[i] != uiPSRAM_DQ_Mapping_POP[p->channel][i])
        {
            mcSHOW_ERR_MSG("!RX APHY DRAMC DQ is not mapping directly\n");
            while(1);
        }
    }

    gPsram_RX_check_per_bit_flag = 0;

    return;
}
#endif

#if PINMUX_AUTO_TEST_PER_BIT_TX
U8 gPsram_TX_check_per_bit_flag = 0;
void CheckPsramTxPinMux(DRAMC_CTX_T *p)
{
    U8 u1Txdelay[8] = {0};
    U8 u1TxIndex[8] = {0};
    U8 i = 0;
    U8 j = 0;
    U16 u2Min = 0xffff;
    U8 u1MinIdx = 0;

    gPsram_TX_check_per_bit_flag = 1;

    for(i = 0; i < 8; i++)
    {
        memset(u1Txdelay, 0, sizeof(u1Txdelay));
        u1Txdelay[i] = 0xf;
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0), P_Fld(u1Txdelay[7], SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0)
            | P_Fld(u1Txdelay[6], SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0)
            | P_Fld(u1Txdelay[5], SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0)
            | P_Fld(u1Txdelay[4], SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0)
            | P_Fld(u1Txdelay[3], SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0)
            | P_Fld(u1Txdelay[2], SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0)
            | P_Fld(u1Txdelay[1], SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0)
            | P_Fld(u1Txdelay[0], SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
        PSramcTxWindowPerbitCal(p, TX_DQ_DQS_MOVE_DQ_ONLY, FALSE);
        mcSHOW_DBG_MSG(("set 1 ranks set:0xf\n"));

        u2Min = 0xffff;
        for(j = 0; j < 8; j++)
        {
            if(gPsramFinalTXPerbitFirstPass[p->channel][j] <= u2Min)
            {
                u2Min = gPsramFinalTXPerbitFirstPass[p->channel][j];
                u1MinIdx = j;
            }
        }
        u1TxIndex[i] = u1MinIdx;
    }

    for(i = 0; i < 8; i++)
    {
        mcSHOW_DBG_MSG("CH[%d] Rank[%d] APHY_TX[%d]-->DRAMC(->TA2->)[%d]\n", p->channel, p->rank, i, u1TxIndex[i]);
        if(u1TxIndex[i] != uiPSRAM_DQ_Mapping_POP[p->channel][i])
        {
            mcSHOW_ERR_MSG(("!TX APHY DRAMC DQ is not mapping directly\n"));
            while(1);
        }
    }

    gPsram_TX_check_per_bit_flag = 0;

    return;
}
#endif

static U32 SwImpedanceAdjust(U32 u4ImpVal, S8 s1StepCnt)
{
    S32 S4ImpedanceTemp = (S32)u4ImpVal;

   // Perform impedance value adjustment
    S4ImpedanceTemp += s1StepCnt;
    /* After adjustment, if value is 1. Too large (val > 31) -> set to max 31
     *                               2. Too small (val < 0) -> set to min 0
     *                               3. Value is between 15 & 29, adjust accordingly ( 15 < value < 29 )
     */
    if ((S4ImpedanceTemp > 15) && (S4ImpedanceTemp < 29)) //Value is between 15 & 29 ( 15 < value < 29)
    {
        S4ImpedanceTemp = S4ImpedanceTemp - 16 + 29;
    }

    if (S4ImpedanceTemp > 31) //Value after adjustment too large -> set to max 31
    {
        S4ImpedanceTemp = 31;
    }
    else if (S4ImpedanceTemp < 0) //Value after adjustment too small -> set to min 0
    {
        S4ImpedanceTemp = 0;
    }

    return (U32)S4ImpedanceTemp;
}
#if PRINT_CALIBRATION_SUMMARY
#ifdef FIX_BUILD_FAIL
static void vSetCalibrationResult(DRAMC_CTX_T *p, U8 ucCalType, U8 ucResult)
{
    p->aru4CalExecuteFlag[p->channel][p->rank] |= (1<<ucCalType); // ececution done
    if (ucResult == DRAM_OK)  // Calibration OK
    {
        p->aru4CalResultFlag[p->channel][p->rank] &= (~(1<<ucCalType));
    }
    else  //Calibration fail
    {
        p->aru4CalResultFlag[p->channel][p->rank] |= (1<<ucCalType);
    }
}
#endif
#if 0  //no use now, disable for saving code size.
void vGetCalibrationResult_All(DRAMC_CTX_T *p, U8 u1Channel, U8 u1Rank, U8 u1FreqType, U32 *u4CalExecute, U32 *u4CalResult)
{
    *u4CalExecute = p->aru4CalExecuteFlag[u1Channel][u1Rank];
    *u4CalResult = p->aru4CalResultFlag[u1Channel][u1Rank];
}

void vGetCalibrationResult(DRAMC_CTX_T *p, U8 ucCalType, U8 *ucCalExecute, U8 *ucCalResult)
{
    U32 ucCalResult_All, ucCalExecute_All;

    ucCalExecute_All = p->aru4CalExecuteFlag[p->channel][p->rank];
    ucCalResult_All = p->aru4CalResultFlag[p->channel][p->rank];

    *ucCalExecute = (U8)((ucCalExecute_All >>ucCalType) & 0x1);
    *ucCalResult =  (U8)((ucCalResult_All >>ucCalType) & 0x1);
}
#endif

static const char *szCalibStatusName[DRAM_CALIBRATION_MAX]=
{
    "ZQ Calibration",
    "SW Impedance",
    "CA Training",
    "Write leveling",
    "RX DQS gating",
    "RX DATLAT",
    "RX DQ/DQS(RDDQC)",
    "RX DQ/DQS(Engine)",
    "TX DQ/DQS",
};

static void vPrintCalibrationBasicInfo(DRAMC_CTX_T *p)
{
#if __ETT__
    mcSHOW_DBG_MSG("===============================================================================\n"
                    "Dram Type= %d, Freq= %u, FreqGroup= %u, CH_%d, rank %d\n"
                    "===============================================================================\n",
                        p->dram_type, DDRPhyFMeter(), p->freqGroup, p->channel, p->rank);
#else
    mcSHOW_DBG_MSG("===============================================================================\n"
                    "Dram Type= %d, Freq= %u, CH_%d, rank %d\n"
                    "===============================================================================\n",
                       p->dram_type, DDRPhyFMeter(), p->channel, p->rank);
#endif
}
#ifdef FIX_BUILD_FAIL
static void vPrintCalibrationResult(DRAMC_CTX_T *p)
{
    U8 ucCHIdx, ucRankIdx, ucCalIdx;
    U32 ucCalResult_All, ucCalExecute_All;
    U8 ucCalResult, ucCalExecute;
    U8 u1CalibrationFail;

    mcSHOW_DBG_MSG("\n\n[Calibration Summary] Freqency %d\n", p->frequency);

    //for(ucFreqIdx=0; ucFreqIdx<DRAM_DFS_SHUFFLE_MAX; ucFreqIdx++)
    {
        //mcSHOW_DBG_MSG("==Freqency = %d==\n", get_Freq_by_shuffleIndex(p,ucFreqIdx));
        for(ucCHIdx=0; ucCHIdx<p->support_channel_num; ucCHIdx++)
        {
            for(ucRankIdx=0; ucRankIdx<RANK_MAX; ucRankIdx++)
            {
                u1CalibrationFail =0;
                ucCalExecute_All = p->aru4CalExecuteFlag[ucCHIdx][ucRankIdx];
                ucCalResult_All = p->aru4CalResultFlag[ucCHIdx][ucRankIdx];
                mcSHOW_DBG_MSG("CH %d, Rank %d\n", ucCHIdx, ucRankIdx);
                //mcSHOW_DBG_MSG("[vPrintCalibrationResult] Channel = %d, Rank= %d, Freq.= %d, (ucCalExecute_All 0x%x, ucCalResult_All 0x%x)\n", ucCHIdx, ucRankIdx, ucFreqIdx, ucCalExecute_All, ucCalResult_All);

                for(ucCalIdx =0; ucCalIdx<DRAM_CALIBRATION_MAX; ucCalIdx++)
                {
                    ucCalExecute = (U8)((ucCalExecute_All >>ucCalIdx) & 0x1);
                    ucCalResult =  (U8)((ucCalResult_All >>ucCalIdx) & 0x1);

                    if(ucCalExecute==1 && ucCalResult ==1) // excuted and fail
                    {
                        u1CalibrationFail =1;
                        mcSHOW_DBG_MSG("%s: %s\n", szCalibStatusName[ucCalIdx], ((ucCalResult == 0) ? "OK" : "Fail"));
                    }
                }

                if(u1CalibrationFail ==0)
                {
                    mcSHOW_DBG_MSG("All Pass.\n");
                }
                mcSHOW_DBG_MSG("\n");
            }
        }
    }

    memset(p->aru4CalResultFlag, 0, sizeof(p->aru4CalResultFlag));
    memset(p->aru4CalExecuteFlag, 0, sizeof(p->aru4CalExecuteFlag));
}
#endif
#endif

void PsramcSwImpedanceSaveRegister(DRAMC_CTX_T *p, U8 ca_term_option, U8 dq_term_option, U8 save_to_where)
{

	//DQ
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING1 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVP], SHU1_DRVING1_DQDRVP2) | P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVN], SHU1_DRVING1_DQDRVN2));
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING2 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVP], SHU1_DRVING2_DQDRVP1) | P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVN], SHU1_DRVING2_DQDRVN1) | P_Fld((!dq_term_option), SHU1_DRVING2_DIS_IMPCAL_ODT_EN));

	//DQS
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING1 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVP], SHU1_DRVING1_DQSDRVP2) | P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVN], SHU1_DRVING1_DQSDRVN2));
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING1 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVP], SHU1_DRVING1_DQSDRVP1) | P_Fld(gDramcSwImpedanceResule[dq_term_option][DRVN], SHU1_DRVING1_DQSDRVN1));

	//CMD & CLK
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING2 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[ca_term_option][DRVP], SHU1_DRVING2_CMDDRVP2) | P_Fld(gDramcSwImpedanceResule[ca_term_option][DRVN], SHU1_DRVING2_CMDDRVN2));
	vIO32WriteFldMulti_All((DRAMC_REG_SHU1_DRVING2 + save_to_where * SHU_GRP_DRAMC_OFFSET), P_Fld(gDramcSwImpedanceResule[ca_term_option][DRVP], SHU1_DRVING2_CMDDRVP1) | P_Fld(gDramcSwImpedanceResule[ca_term_option][DRVN], SHU1_DRVING2_CMDDRVN1));


	//CKE
	// CKE is full swing.
	// LP3 CKE DRVP/DRVN is same above
	//DRVP[4:0] = RG_TX_ARCMD_PU_PRE<1:0>, RG_TX_ARCLK_DRVN_PRE<2:0>
	vIO32WriteFldAlign_All((DDRPHY_SHU1_CA_CMD3 + save_to_where * SHU_GRP_DDRPHY_OFFSET), (gDramcSwImpedanceResule[dq_term_option][DRVP]>>3)&0x3, SHU1_CA_CMD3_RG_TX_ARCMD_PU_PRE);
	vIO32WriteFldAlign_All((DDRPHY_SHU1_CA_CMD0 + save_to_where * SHU_GRP_DDRPHY_OFFSET), 0x0, SHU1_CA_CMD0_RG_TX_ARCLK_DRVN_PRE);
	//DRVN[4:0] = RG_ARCMD_REV<12:8>
#if (fcFOR_CHIP_ID == fcSchubert)
	vIO32WriteFldAlign_All((DDRPHY_SHU1_CA_DLL1 + save_to_where * SHU_GRP_DDRPHY_OFFSET), gDramcSwImpedanceResule[dq_term_option][DRVN], RG_ARCMD_REV_BIT_1208_TX_CKE_DRVN);
#endif

}


/* Refer to "IMPCAL Settings" document register "RG_RIMP_VREF_SEL" settings */
static void vImpCalVrefSel(DRAMC_CTX_T *p, U8 term_option, U8 u1ImpCalStage)
{
    U8 u1RegTmpValue = 0;

	u1RegTmpValue = IMP_LP3_VREF_SEL;

    // dbg msg after vref_sel selection
    mcSHOW_DBG_MSG3("[vImpCalVrefSel] IMP_VREF_SEL 0x%x, IMPCAL stage:%u, term_option:%u\n",
                      u1RegTmpValue, u1ImpCalStage, term_option);

    /* Set IMP_VREF_SEL register field's value */
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD11), u1RegTmpValue, SHU1_CA_CMD11_RG_RIMP_VREF_SEL);

    return;
}

U32 u4gpRegBackupVlaue[64];
#ifdef FIX_BUILD_FAIL
static void DramcBackupRegisters(DRAMC_CTX_T *p, U32 *backup_addr, U32 backup_num)
{
    U32 u4RegIdx;

#if __ETT__
    if (backup_num>64)
    {
        mcSHOW_ERR_MSG("[DramcBackupRegisters] backup number over 64!!!\n");
        while(1);
    }
#endif

    for(u4RegIdx=0; u4RegIdx < backup_num; u4RegIdx++)
    {
        u4gpRegBackupVlaue[u4RegIdx] = u4IO32Read4B(backup_addr[u4RegIdx]);
        //mcSHOW_DBG_MSG("Backup Reg(0x%X) = 0x%X\n", backup_addr[u4RegIdx], u4gpRegBackupVlaue[u4RegIdx]);
    }
}

static void DramcRestoreRegisters(DRAMC_CTX_T *p, U32 *restore_addr, U32 restore_num)
{
    U32 u4RegIdx;

    for(u4RegIdx=0; u4RegIdx < restore_num; u4RegIdx++)
    {
        vIO32Write4B(restore_addr[u4RegIdx], u4gpRegBackupVlaue[u4RegIdx]);
        //mcSHOW_DBG_MSG("Restore Reg(0x%X) = 0x%X\n", restore_addr[u4RegIdx], u4gpRegBackupVlaue[u4RegIdx]);
    }
}
#endif
static void PSramPhyReset(DRAMC_CTX_T *p)
{
	// Everest change reset order : reset DQS before DQ, move PHY reset to final.
	//if(u1IsLP4Family(p->dram_type))
	{
		vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RX_SET0), 1, RX_SET0_RDATRST);// read data counter reset

		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM), 1, MISC_CTRL1_PSRAM_R_DMPHYRST_PSRAM);
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 1, MISC_CTRL1_R_DMPHYRST);
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(0, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(0, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));

		//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(0, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) |P_Fld(0, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
    #ifdef LOOPBACK_TEST
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_CA_CMD10), P_Fld(0, CA_CMD10_RG_RX_ARCLK_STBEN_RESETB) | P_Fld(0, CA_CMD10_RG_RX_ARCMD_STBEN_RESETB));
    #endif
		mcDELAY_US(1);//delay 10ns
    #ifdef LOOPBACK_TEST
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_CA_CMD10), P_Fld(1, CA_CMD10_RG_RX_ARCLK_STBEN_RESETB) | P_Fld(1, CA_CMD10_RG_RX_ARCMD_STBEN_RESETB));
    #endif
		//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(1, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) | P_Fld(1, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 0, MISC_CTRL1_R_DMPHYRST);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM), 0, MISC_CTRL1_PSRAM_R_DMPHYRST_PSRAM);
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));

		vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RX_SET0), 0, RX_SET0_RDATRST);// read data counter reset
	}

}

//for ImpedanceCal
#define fcR_EXT 60   // EXT resistor : 60ohm
#define fcR_PN_OCD 35
#define fcR_NN_OCD 35    // ODT resistor : 35ohm

//-------------------------------------------------------------------------
/** Round_Operation
 *  Round operation of A/B
 *  @param  A
 *  @param  B
 *  @retval round(A/B)
 */
//-------------------------------------------------------------------------
 static U16 Round_Operation(U16 A, U16 B)
{
    U16 temp;

    if (B == 0)
    {
        return 0xffff;
    }

    temp = A/B;

    if ((A-temp*B) >= ((temp+1)*B-A))
	{
        return (temp+1);
	}
    else
	{
        return temp;
	}
}



DRAM_STATUS_T PsramcSwImpedanceCal(DRAMC_CTX_T *p, U8 u1Para, U8 term_option)
{
	U32 u4ImpxDrv, u4ImpCalResult;
	U32 u4DRVP_Result =0xff,u4ODTN_Result =0xff, u4DRVN_Result =0xff;
	U32 u4BaklReg_DDRPHY_MISC_IMP_CTRL0, u4BaklReg_DDRPHY_MISC_IMP_CTRL1, u4BaklReg_DRAMC_REG_IMPCAL;
	U8 u1ByteIdx, u1RegTmpValue;
	U8 backup_channel;
	U8 backup_broadcast;
	U16 u2value;

	backup_broadcast = GetDramcBroadcast();

	//DramcBroadcastOnOff(DRAMC_BROADCAST_OFF);

	// Darren
	vIO32WriteFldMulti_All(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
										   | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));

	vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL2, 0x0, MISC_SPM_CTRL2_PHY_SPM_CTL2);
	vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL0, 0x0, MISC_SPM_CTRL0_PHY_SPM_CTL0);

	//Disable IMP HW Tracking
	//Hw Imp tracking disable for all channels Because SwImpCal will be K again when resume from DDR reserved mode
	vIO32WriteFldAlign_All(DRAMC_REG_IMPCAL, 0, IMPCAL_IMPCAL_HW);

	backup_channel = p->channel;
	vSetPHY2ChannelMapping(p, CHANNEL_A);

	//Register backup
	//u4BaklReg_DDRPHY_MISC_IMP_CTRL0 = u4IO32Read4B((DDRPHY_MISC_IMP_CTRL0));
	//u4BaklReg_DDRPHY_MISC_IMP_CTRL1 = u4IO32Read4B((DDRPHY_MISC_IMP_CTRL1));
	u4BaklReg_DRAMC_REG_IMPCAL = u4IO32Read4B(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL));

	/* Set IMP_VREF_SEL value for DRVP */
	vImpCalVrefSel(p, term_option, IMPCAL_STAGE_DRVP);
#if 0
	if(u1IsLP4Family(p->dram_type))
	{
		//RG_IMPCAL_VREF_SEL (now set in vImpCalVrefSel())
		//RG_IMPCAL_LP3_EN=0, RG_IMPCAL_LP4_EN=1
#if 0
		vIO32WriteFldMulti( (DDRPHY_MISC_IMP_CTRL1), P_Fld(1, MISC_IMP_CTRL1_RG_RIMP_BIAS_EN) | \
							P_Fld(0, MISC_IMP_CTRL1_RG_RIMP_PRE_EN) | P_Fld(1, MISC_IMP_CTRL1_RG_RIMP_VREF_EN));
#else
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1), P_Fld(0, MISC_IMP_CTRL1_RG_RIMP_PRE_EN));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), P_Fld(0, IMPCAL_IMPCAL_CALI_ENN) | P_Fld(1, IMPCAL_IMPCAL_IMPPDP) | \
							P_Fld(1, IMPCAL_IMPCAL_IMPPDN));	//RG_RIMP_BIAS_EN and RG_RIMP_VREF_EN move to IMPPDP and IMPPDN
#endif
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0), P_Fld(1, MISC_IMP_CTRL0_RG_IMP_EN) | \
							P_Fld(0, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL) | P_Fld(1, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));
	}
#endif
#if 1//ENABLE_LP3_SW
	//else //LPDDR3
	{
		//RG_IMPCAL_VREF_SEL (now set in vImpCalVrefSel())
		//RG_IMPCAL_LP3_EN=0, RG_IMPCAL_LP4_EN=1
		//RG_IMPCAL_ODT_EN=0
#if 0
		vIO32WriteFldMulti((DDRPHY_MISC_IMP_CTRL1), P_Fld(0, MISC_IMP_CTRL1_RG_RIMP_ODT_EN)| P_Fld(1, MISC_IMP_CTRL1_RG_RIMP_BIAS_EN) | \
							P_Fld(0, MISC_IMP_CTRL1_RG_RIMP_PRE_EN) | P_Fld(1, MISC_IMP_CTRL1_RG_RIMP_VREF_EN));
#else
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1), P_Fld(0, MISC_IMP_CTRL1_RG_RIMP_PRE_EN));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), P_Fld(0, IMPCAL_IMPCAL_CALI_ENN) | P_Fld(1, IMPCAL_IMPCAL_IMPPDP) | \
							P_Fld(1, IMPCAL_IMPCAL_IMPPDN));	//RG_RIMP_BIAS_EN and RG_RIMP_VREF_EN move to IMPPDP and IMPPDN, ODT_EN move to CALI_ENN
#endif
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0), P_Fld(1, MISC_IMP_CTRL0_RG_IMP_EN) | \
							P_Fld(1, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL) | P_Fld(0, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));

    #ifdef ETT_PRINT_FORMAT
		mcSHOW_DBG_MSG2("0x%X=0x%X\n", DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1), u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1)));
		mcSHOW_DBG_MSG2("0x%X=0x%X\n", DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0), u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0)));
    #else
		mcSHOW_DBG_MSG2("0x%8x=0x%8x\n", DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1), u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL1)));
		mcSHOW_DBG_MSG2("0x%8x=0x%8x\n", DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0), u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_MISC_IMP_CTRL0)));
    #endif
	}
#endif
	mcDELAY_US(1);

	// K pull up
	mcSHOW_DBG_MSG2("\n\tK DRVP\n");
	mcFPRINTF(fp_A60501, "\n\tK DRVP\n");
	//PUCMP_EN=1
	//ODT_EN=0
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 1, IMPCAL_IMPCAL_CALI_EN);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 1, IMPCAL_IMPCAL_CALI_ENP);  //PUCMP_EN move to CALI_ENP
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 0, IMPCAL_IMPCAL_CALI_ENN);  //ODT_EN move to CALI_ENN
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1), P_Fld(0, SHU_IMPCAL1_IMPDRVN)|P_Fld(0, SHU_IMPCAL1_IMPDRVP));

	//DRVP=0
	//DRV05=1
    u1RegTmpValue = 1;

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD11), u1RegTmpValue, SHU1_CA_CMD11_RG_RIMP_REV);

	//OCDP Flow
	//If RGS_TX_OCD_IMPCALOUTX=0
	//RG_IMPX_DRVP++;
	//Else save and keep RG_IMPX_DRVP value, and assign to DRVP
	for(u4ImpxDrv=0; u4ImpxDrv<32; u4ImpxDrv++)
	{
		if(u4ImpxDrv==16) //0~15, 29~31
			u4ImpxDrv = 29;
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1), u4ImpxDrv, SHU_IMPCAL1_IMPDRVP);
		mcDELAY_US(1);
		u4ImpCalResult = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS_CMD), MISC_PHY_RGS_CMD_RGS_RIMPCALOUT);
		mcSHOW_DBG_MSG2("1. OCD DRVP=%d CALOUT=%d\n", u4ImpxDrv, u4ImpCalResult);
		mcFPRINTF(fp_A60501, "1. OCD DRVP=%d CALOUT=%d\n", u4ImpxDrv, u4ImpCalResult);

		if((u4ImpCalResult ==1) && (u4DRVP_Result == 0xff))//first found
		{
			u4DRVP_Result = u4ImpxDrv;
			mcSHOW_DBG_MSG2("\n1. OCD DRVP calibration OK! DRVP=%d\n\n", u4DRVP_Result);
			mcFPRINTF(fp_A60501, "\n1. OCD DRVP calibration OK! DRVP=%d\n\n", u4DRVP_Result);
			break;
		}
	}

	//LP4: ODTN calibration, LP3: DRVN calibration
	mcSHOW_DBG_MSG2("\n\n\tK ODTN\n");
	mcFPRINTF(fp_A60501, "\n\tK ODTN\n");


	/* Set IMP_VREF_SEL value for DRVN */
	vImpCalVrefSel(p, term_option, IMPCAL_STAGE_DRVN);

	//PUCMP_EN=0
	//LPDDR4 : ODT_EN=1
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 0, IMPCAL_IMPCAL_CALI_ENP);  //PUCMP_EN move to CALI_ENP

#if 0
	if(p->dram_type == TYPE_LPDDR4 || p->dram_type == TYPE_LPDDR4X)
	{
		if (term_option == 1)
		{
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 1, IMPCAL_IMPCAL_CALI_ENN);  //ODT_EN move to CALI_ENN
		}
	}
#endif
	//DRVP=DRVP_FINAL
	//DRVN=0
	//DRV05=1
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1), P_Fld(u4DRVP_Result, SHU_IMPCAL1_IMPDRVP) | P_Fld(0, SHU_IMPCAL1_IMPDRVN));

	u1RegTmpValue = 1;

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD11), u1RegTmpValue, SHU1_CA_CMD11_RG_RIMP_REV);


	//If RGS_TX_OCD_IMPCALOUTX=1
	//RG_IMPX_DRVN++;
	//Else save RG_IMPX_DRVN value and assign to DRVN
	for(u4ImpxDrv=0; u4ImpxDrv<32 ; u4ImpxDrv++)
	{
		if(u4ImpxDrv==16) //0~15, 29~31
			u4ImpxDrv = 29;

		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_SHU_IMPCAL1), u4ImpxDrv, SHU_IMPCAL1_IMPDRVN);
		mcDELAY_US(1);
		u4ImpCalResult = u4IO32ReadFldAlign((DDRPHY_MISC_PHY_RGS_CMD), MISC_PHY_RGS_CMD_RGS_RIMPCALOUT);
		mcSHOW_DBG_MSG2("3. OCD ODTN=%d ,CALOUT=%d\n", u4ImpxDrv, u4ImpCalResult);
		mcFPRINTF(fp_A60501, "3. OCD ODTN=%d ,CALOUT=%d\n", u4ImpxDrv, u4ImpCalResult);

		if((u4ImpCalResult ==0) &&(u4ODTN_Result == 0xff))//first found
		{
			u4ODTN_Result = u4ImpxDrv;
			mcSHOW_DBG_MSG2("\n3. OCD ODTN calibration OK! ODTN=%d\n\n", u4ODTN_Result);
			mcFPRINTF(fp_A60501, "\n3. OCD ODTN calibration OK! ODTN=%d\n\n", u4ODTN_Result);
			break;
		}
	}

	//Register Restore
	vIO32Write4B(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), u4BaklReg_DRAMC_REG_IMPCAL);
	//vIO32Write4B((DDRPHY_MISC_IMP_CTRL0), u4BaklReg_DDRPHY_MISC_IMP_CTRL0);
	//vIO32Write4B((DDRPHY_MISC_IMP_CTRL1), u4BaklReg_DDRPHY_MISC_IMP_CTRL1);


/*** default value if K fail
	LP3:  DRVP=8, DRVN=9
	LP4:  DRVP=6, DRVN=9, ODTN=14
	LP4X(UT): DRVP=12, DRVN=9
	LP4X(T):  DRVP=5, DRVN=9, ODTN=14
	LP4P: DRVP=8, DRVN=10
***/
#if 0
	if(u1IsLP4Family(p->dram_type))
	{
#if (fcFOR_CHIP_ID == fcSchubert)
		u4DRVN_Result = 9; //fixed value from DE YingMin Liao
#endif

		if(u4DRVP_Result==0xff)
		{
			mcFPRINTF(fp_A60501, "\n[CHIP_SCAN]1. OCD DRVP calibration FAIL! \n\n");
#if (fcFOR_CHIP_ID == fcSchubert)
			u4DRVP_Result = 31;
#endif
		}
		if(u4ODTN_Result==0xff || u4DRVP_Result==0xff)
		{
			mcFPRINTF(fp_A60501, "\n[CHIP_SCAN]3. OCD ODTN calibration FAIL! \n\n");
#if (fcFOR_CHIP_ID == fcSchubert)
			u4ODTN_Result = 31;
#endif
		}

		mcSHOW_DBG_MSG("[SwImpedanceCal] DRVP=%d, DRVN=%d, ODTN=%d\n", u4DRVP_Result, u4DRVN_Result, u4ODTN_Result);
		mcFPRINTF(fp_A60501, "[SwImpedanceCal] DRVP=%d, DRVN=%d, ODTN=%d\n", u4DRVP_Result, u4DRVN_Result, u4ODTN_Result);

    #if 0//HYNIX_IMPX_ADJUST
		if(u1Para)
		{
			u4ODTN_Result= ImpedanceAdjustment_Hynix(u4ODTN_Result, u1Para);
		}
    #endif
		if((p->dram_type == TYPE_LPDDR4X || p->dram_type == TYPE_LPDDR4P) && (term_option ==0))
		{
			gDramcSwImpedanceResule[term_option][DRVP] = u4DRVP_Result;
			gDramcSwImpedanceResule[term_option][DRVN] = u4ODTN_Result;    //Justin : LP4X unterm DRVN is ODTN * 2
			gDramcSwImpedanceResule[term_option][ODTP] = 0;
			gDramcSwImpedanceResule[term_option][ODTN] = 15;	//Justin : LP4X unterm, ODTN is useless
		}
		else
		{
			gDramcSwImpedanceResule[term_option][DRVP] = (u4DRVP_Result<=3) ? (u4DRVP_Result * 3) : u4DRVP_Result;
			gDramcSwImpedanceResule[term_option][DRVN] = (u4DRVN_Result<=3) ? (u4DRVN_Result * 3) : u4DRVN_Result;
			gDramcSwImpedanceResule[term_option][ODTP] = 0;
			gDramcSwImpedanceResule[term_option][ODTN] = (u4ODTN_Result<=3) ? (u4ODTN_Result * 3) : u4ODTN_Result;
		}
	}
#endif
#if 1//ENABLE_LP3_SW
	//else	//LPDDR3
	{
		u4DRVN_Result = u4ODTN_Result;
		if(u4DRVN_Result==0xff || u4DRVP_Result==0xff)
		{
			u4DRVP_Result = 8;
			u4DRVN_Result = 9;
		}

		gDramcSwImpedanceResule[term_option][DRVP] = (u4DRVP_Result<=3) ? (u4DRVP_Result * 3) : u4DRVP_Result;
		gDramcSwImpedanceResule[term_option][DRVN] = (u4DRVN_Result<=3) ? (u4DRVN_Result * 3) : u4DRVN_Result;
		gDramcSwImpedanceResule[term_option][ODTP] = 0;
		gDramcSwImpedanceResule[term_option][ODTN] = (u4ODTN_Result<=3) ? (u4ODTN_Result * 3) : u4ODTN_Result;
	}
#endif

	/* Set IMP_VREF_SEL value for TRACKING/RUN-TIME */
	vImpCalVrefSel(p, term_option, IMPCAL_STAGE_TRACKING);

#if RUNTIME_SHMOO_RELEATED_FUNCTION && SUPPORT_SAVE_TIME_FOR_CALIBRATION
	{
		U8 u1drv;
		{
			for (u1drv=0; u1drv<4; u1drv++)
			{
				if(p->femmc_Ready==0)
					p->pSavetimeData->u1SwImpedanceResule[term_option][u1drv] = gDramcSwImpedanceResule[term_option][u1drv];
				else
					gDramcSwImpedanceResule[term_option][u1drv] = p->pSavetimeData->u1SwImpedanceResule[term_option][u1drv];
			}
		}
	}
#endif
	mcSHOW_DBG_MSG("term_option=%d, Reg: DRVP=%d, DRVN=%d, ODTN=%d\n", term_option, gDramcSwImpedanceResule[term_option][DRVP],
										gDramcSwImpedanceResule[term_option][DRVN], gDramcSwImpedanceResule[term_option][ODTN]);

#if APPLY_SIGNAL_WAVEFORM_SETTINGS_ADJUST
#if 0
	if((p->dram_type == TYPE_LPDDR4X || p->dram_type == TYPE_LPDDR4P) && (term_option ==0))
	{
		gDramcSwImpedanceResule[term_option][DRVP] = SwImpedanceAdjust(gDramcSwImpedanceResule[term_option][DRVP], gDramcSwImpedanceAdjust[term_option][DRVP]);
		gDramcSwImpedanceResule[term_option][DRVN] = SwImpedanceAdjust(gDramcSwImpedanceResule[term_option][DRVN], gDramcSwImpedanceAdjust[term_option][ODTN]);
	}
	else
#endif
#if 1//PSRAM_SPEC
	u2value= ((U16) (gDramcSwImpedanceResule[term_option][DRVP]+2)) * fcR_EXT;
	u2value = Round_Operation(u2value, fcR_PN_OCD) - 2;
	gDramcSwImpedanceResule[term_option][DRVP] = (U8) u2value;

	u2value= ((U16) (gDramcSwImpedanceResule[term_option][DRVN]+2)) * fcR_EXT;
	u2value = Round_Operation(u2value, fcR_PN_OCD) - 2;
	gDramcSwImpedanceResule[term_option][DRVN] = (U8) u2value;

#else
	{
		gDramcSwImpedanceResule[term_option][DRVP] = SwImpedanceAdjust(gDramcSwImpedanceResule[term_option][DRVP], gDramcSwImpedanceAdjust[term_option][DRVP]);
		gDramcSwImpedanceResule[term_option][ODTN] = SwImpedanceAdjust(gDramcSwImpedanceResule[term_option][ODTN], gDramcSwImpedanceAdjust[term_option][ODTN]);
	}
#endif
	mcSHOW_DBG_MSG("term_option=%d, Reg: DRVP=%d, DRVN=%d, ODTN=%d (After Adjust)\n", term_option, gDramcSwImpedanceResule[term_option][DRVP],
										gDramcSwImpedanceResule[term_option][DRVN], gDramcSwImpedanceResule[term_option][ODTN]);
#endif

#if SIMULATION_SW_IMPED && 1//ENABLE_LP3_SW
	//LP3 updates impedance parameters to shuffle "SaveRegister" here, un-term only
//	if((p->dram_type == TYPE_LPDDR3)||1)
		PsramcSwImpedanceSaveRegister(p, term_option, term_option, DRAM_DFS_SHUFFLE_1);
#endif


#if defined(SLT)
	if (gDramcSwImpedanceResule[term_option][DRVP] >= 31 && (term_option ==1) ) {
		mcSHOW_DBG_MSG("SLT_BIN2\n");
		while(1);
	}
#endif


	vSetCalibrationResult(p, DRAM_CALIBRATION_SW_IMPEDANCE, DRAM_OK);
	mcSHOW_DBG_MSG3("[PsramcSwImpedanceCal] Done\n\n");
	mcFPRINTF(fp_A60501, "[PsramcSwImpedanceCal] Done\n\n");

	vSetPHY2ChannelMapping(p, backup_channel);
	//DramcBroadcastOnOff(backup_broadcast);

	return DRAM_OK;
}


DRAM_STATUS_T PSramcMiockJmeter(DRAMC_CTX_T *p)
{
	U8 u1ByteIdx;
	U32 u4RevBX[1];
	U8 ucsearch_state=0, ucdqs_dly, fgcurrent_value, fginitial_value, ucstart_period=0, ucmiddle_period=0, ucend_period=0;
	U32 u4sample_cnt, u4ones_cnt[1], u4MPDIV_IN_SEL;
	U16 u2real_freq, u2real_period;

	U8 u1ShuLevel;
	U32 u4PLL5_ADDR;
	U32 u4PLL8_ADDR;
	U32 u4CA_CMD6;
	U32 u4SDM_PCW;
	U32 u4PREDIV;
	U32 u4POSDIV;
	U32 u4CKDIV4;
	U32 u4VCOFreq;
	U32 u4DataRate;
	U8 u1RxGatingPI=0, u1RxGatingPI_start=0, u1RxGatingPI_end=63;

	// error handling
	if (!p)
	{
		mcSHOW_ERR_MSG("context NULL\n");
		return DRAM_FAIL;
	}

#if (fcFOR_CHIP_ID == fcSchubert)
	u1RxGatingPI_start = 4;
	u1RxGatingPI_end = 63;
#endif

	u2gdelay_cell_ps=0;

	U32 u4RegBackupAddress[] =
	{
		(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN)),
		(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ6)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ5)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ3)),
		(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ7)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ4)),
		(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7_PSRAM)),
		(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2_PSRAM)),
		(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7)),
		(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1)),
		(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2)),
		((DDRPHY_CA_DLL_ARPI2)),

	};


	//backup register value
	//if(u1IsLP4Family(p->dram_type))
	{
		DramcBackupRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
	}

	//DLL off to fix middle transion from high to low or low to high at high vcore
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI2), 0x0, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
#if 0//PSRAM_SPEC
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2_PSRAM), 0x0, B0_DLL_ARPI2_PSRAM_RG_ARDLL_PHDET_EN_B0_PSRAM);
#endif
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2), 0x0, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);

	//vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_DLL_ARPI2), 0x0, B1_DLL_ARPI2_RG_ARDLL_PHDET_EN_B1);

	//if(u1IsLP4Family(p->dram_type))
	{
		//MCK4X CG
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM), 0, MISC_CTRL1_PSRAM_R_DMDQSIENCG_EN_PSRAM);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 0, MISC_CTRL1_R_DMDQSIENCG_EN);

		// Bypass DQS glitch-free mode
		// RG_RX_*RDQ_EYE_DLY_DQS_BYPASS_B**
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), 1, B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0);
//		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), 1, B1_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B1);

		//Enable DQ eye scan
		//RG_??_RX_EYE_SCAN_EN
		//RG_??_RX_VREF_EN
		//RG_??_RX_SMT_EN
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 1, EYESCAN_RG_RX_EYE_SCAN_EN);
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), P_Fld(0x1, EYESCAN_EYESCAN_DQS_SYNC_EN)
											| P_Fld(0x1, EYESCAN_EYESCAN_NEW_DQ_SYNC_EN)
											| P_Fld(0x1, EYESCAN_EYESCAN_DQ_SYNC_EN));
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), 1, B0_DQ5_RG_RX_ARDQ_EYE_EN_B0);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), 1, B1_DQ5_RG_RX_ARDQ_EYE_EN_B1);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), 1, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), 1, B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ3), 1, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ3), 1, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1);

		//JM_SEL
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), 1, B0_DQ6_RG_RX_ARDQ_JM_SEL_B0);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), 1, B1_DQ6_RG_RX_ARDQ_JM_SEL_B1);
	}


	//Enable MIOCK jitter meter mode ( RG_RX_MIOCK_JIT_EN=1)
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 1, EYESCAN_RG_RX_MIOCK_JIT_EN);

	//Disable DQ eye scan (b'1), for counter clear
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 0, EYESCAN_RG_RX_EYE_SCAN_EN);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_STBCAL1), 0, STBCAL1_DQSERRCNT_DIS);

	//Using test engine to switch back to RK0, or the gating PI cannont be adjust successfully.
	//lzs psram only one rank no need, or simulation will hangup
	//DramcEngine2Run(p, TE_OP_READ_CHECK, p->test_pattern);

	for (u1RxGatingPI=u1RxGatingPI_start; u1RxGatingPI<u1RxGatingPI_end; u1RxGatingPI+=4)
	{
		mcSHOW_DBG_MSG("\n[DramcMiockJmeter] u1RxGatingPI = %d\n", u1RxGatingPI);

	ucsearch_state = 0;

	//to see 1T(H,L) or 1T(L,H) from delaycell=0 to 127
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0), u1RxGatingPI, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0);

	for (ucdqs_dly=0; ucdqs_dly<128; ucdqs_dly++)
	{

		//Set DQS delay (RG_??_RX_DQS_EYE_DLY)
		//if(u1IsLP4Family(p->dram_type))
		{
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ4), ucdqs_dly, B0_DQ4_RG_RX_ARDQS_EYE_R_DLY_B0);
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ4), ucdqs_dly, B0_DQ4_RG_RX_ARDQS_EYE_F_DLY_B0);
			//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ4), ucdqs_dly, B1_DQ4_RG_RX_ARDQS_EYE_R_DLY_B1);
			//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ4), ucdqs_dly, B1_DQ4_RG_RX_ARDQS_EYE_F_DLY_B1);
		}

		PSramPhyReset(p);

		//Reset eye scan counters (reg_sw_rst): 1 to 0
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 1, EYESCAN_REG_SW_RST);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 0, EYESCAN_REG_SW_RST);

		//Enable DQ eye scan (b'1)
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 1, EYESCAN_RG_RX_EYE_SCAN_EN);

		//2ns/sample, here we delay 1ms about 500 samples
		mcDELAY_US(10);

		//Disable DQ eye scan (b'1), for counter latch
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_EYESCAN), 0, EYESCAN_RG_RX_EYE_SCAN_EN);

		//Read the counter values from registers (toggle_cnt*, dqs_err_cnt*);
		u4sample_cnt = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_TOGGLE_CNT), TOGGLE_CNT_TOGGLE_CNT);
		u4ones_cnt[0] = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_DQS0_ERR_CNT), DQS0_ERR_CNT_DQS0_ERR_CNT);
		//u4ones_cnt[1] = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_DQS1_ERR_CNT), DQS1_ERR_CNT_DQS1_ERR_CNT);
		//u4ones_cnt[2] = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_DQS2_ERR_CNT), DQS2_ERR_CNT_DQS2_ERR_CNT);
		//u4ones_cnt[3] = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DRAMC_REG_DQS3_ERR_CNT), DQS3_ERR_CNT_DQS3_ERR_CNT);
#ifdef ETT_PRINT_FORMAT
		mcSHOW_DBG_MSG("%d : %d, %d\n", ucdqs_dly, u4sample_cnt, u4ones_cnt[0]);
#else
		mcSHOW_DBG_MSG("%3d : %8d, %8d\n", ucdqs_dly, u4sample_cnt, u4ones_cnt[0]);
#endif

		/*
		//Disable DQ eye scan (RG_RX_EYE_SCAN_EN=0, RG_RX_*RDQ_VREF_EN_B*=0, RG_RX_*RDQ_EYE_VREF_EN_B*=0, RG_RX_*RDQ_SMT_EN_B*=0)
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_STBCAL_F), 0, STBCAL_F_RG_EX_EYE_SCAN_EN);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_EYE2), 0, EYE2_RG_RX_ARDQ_VREF_EN_B0);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_EYEB1_2), 0, EYEB1_2_RG_RX_ARDQ_VREF_EN_B1);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_EYE2), 0, EYE2_RG_RX_ARDQ_EYE_VREF_EN_B0);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_EYEB1_2), 0, EYEB1_2_RG_RX_ARDQ_EYE_VREF_EN_B1);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_TXDQ3), 0, TXDQ3_RG_RX_ARDQ_SMT_EN_B0);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_RXDQ13),0, RXDQ13_RG_RX_ARDQ_SMT_EN_B1);
		*/

		//change to boolean value
		if (u4ones_cnt[0] < (u4sample_cnt/2))
		{
			fgcurrent_value = 0;
		}
		else
		{
			fgcurrent_value = 1;
		}

#if 1//more than 1T data
		{
			if (ucsearch_state==0)
			{
				//record initial value at the beginning
				fginitial_value = fgcurrent_value;
				ucsearch_state = 1;
			}
			else if (ucsearch_state==1)
			{
				// check if change value
				if (fgcurrent_value != fginitial_value)
				{
					// start of the period
					fginitial_value = fgcurrent_value;
					ucstart_period = ucdqs_dly;
					ucsearch_state = 2;
				}
			}
			else if (ucsearch_state==2)
			{
				// check if change value
				if (fgcurrent_value != fginitial_value)
				{
					fginitial_value = fgcurrent_value;
					ucmiddle_period = ucdqs_dly;
					ucsearch_state = 3;
				}
			}
			else if (ucsearch_state==3)
			{
				// check if change value
				if (fgcurrent_value != fginitial_value)
				{
					// end of the period, break the loop
					ucend_period = ucdqs_dly;
					ucsearch_state = 4;
					break;
				}
			}
			else
			{
				//nothing
			}
		}
#else //only 0.5T data
		{
			if (ucsearch_state==0)
			{
				//record initial value at the beginning
				fginitial_value = fgcurrent_value;
				ucsearch_state = 1;
			}
			else if (ucsearch_state==1)
			{
				// check if change value
				if (fgcurrent_value != fginitial_value)
				{
					// start of the period
					fginitial_value = fgcurrent_value;
					ucstart_period = ucdqs_dly;
					ucsearch_state = 2;
				}
			}
			else if (ucsearch_state==2)
			{
				// check if change value
				if (fgcurrent_value != fginitial_value)
				{
					// end of the period, break the loop
					ucend_period = ucdqs_dly;
					ucsearch_state = 4;
				   break;
				}
			}
		}
#endif
	}

	if((ucsearch_state==4) || (ucsearch_state==3))
		break;

	}

	//restore to orignal value
	//if(u1IsLP4Family(p->dram_type))
	{
		DramcRestoreRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
	}

	if(ucsearch_state!=4)
	{
		if (ucsearch_state!=3)
		{
			mcSHOW_DBG_MSG("\n\tMIOCK jitter meter - ch=%d\n", p->channel);
			mcSHOW_DBG_MSG("\tLess than 0.5T data. Cannot calculate delay cell time\n\n");
			if(p->frequency==667)
				ucg_num_dlycell_perT = 180;   //for LP3/4 lookup table used
			else if(p->frequency==600)
				ucg_num_dlycell_perT = 208;
			else if(p->frequency==800)
				ucg_num_dlycell_perT = 178;
			else
				ucg_num_dlycell_perT = 0;

			return DRAM_FAIL;
		}
		else
		{
			//Calculate 1 delay cell = ? ps
			// 1T = ? delay cell
			ucg_num_dlycell_perT = (ucmiddle_period - ucstart_period)*2;
			// 1T = ? ps
		}
	}
	else
	{
		//Calculate 1 delay cell = ? ps
		// 1T = ? delay cell
		ucg_num_dlycell_perT = (ucend_period - ucstart_period);
		// 1T = ? ps
	}

	u1ShuLevel = u4IO32ReadFldAlign(DRAMC_REG_SHUSTATUS, SHUSTATUS_SHUFFLE_LEVEL);
	u4PLL5_ADDR = DDRPHY_SHU1_PLL5 + SHU_GRP_DDRPHY_OFFSET * u1ShuLevel;
	u4PLL8_ADDR = DDRPHY_SHU1_PLL8 + SHU_GRP_DDRPHY_OFFSET * u1ShuLevel;
	u4CA_CMD6 = DDRPHY_SHU1_CA_CMD6 + SHU_GRP_DDRPHY_OFFSET * u1ShuLevel;
	u4SDM_PCW = u4IO32ReadFldAlign(u4PLL5_ADDR, SHU1_PLL5_RG_RPHYPLL_SDM_PCW);
	u4PREDIV = u4IO32ReadFldAlign(u4PLL8_ADDR, SHU1_PLL8_RG_RPHYPLL_PREDIV);
	u4POSDIV = u4IO32ReadFldAlign(u4PLL8_ADDR, SHU1_PLL8_RG_RPHYPLL_POSDIV);
	u4CKDIV4 = u4IO32ReadFldAlign(u4CA_CMD6, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA);
	u4VCOFreq = ((52>>u4PREDIV)*(u4SDM_PCW>>8))>>u4POSDIV;
	u4DataRate = u4VCOFreq>>u4CKDIV4;
	u2real_freq = u4DataRate>>1;

	u2real_period = (U16) (1000000/u2real_freq);
	//calculate delay cell time

	u2gdelay_cell_ps = u2real_period*100 / ucg_num_dlycell_perT;
    p->ucnum_dlycell_perT = ucg_num_dlycell_perT;
    p->u2DelayCellTimex100 = u2gdelay_cell_ps;

	if (ucsearch_state==4)
	{
		mcSHOW_DBG_MSG("\n\tMIOCK jitter meter\tch=%d\n\n"
						"1T = (%d-%d) = %d dly cells\n"
						"Clock freq = %d MHz, period = %d ps, 1 dly cell = %d/100 ps\n",
							p->channel,
							ucend_period, ucstart_period, ucg_num_dlycell_perT,
							u2real_freq, u2real_period, u2gdelay_cell_ps);
	}
	else
	{
		mcSHOW_DBG_MSG("\n\tMIOCK jitter meter\tch=%d\n\n"
						"1T = (%d-%d)*2 = %d dly cells\n"
						"Clock freq = %d MHz, period = %d ps, 1 dly cell = %d/100 ps\n",
							p->channel,
							ucmiddle_period, ucstart_period, ucg_num_dlycell_perT,
							u2real_freq, u2real_period, u2gdelay_cell_ps);
	}

	return DRAM_OK;

// log example
/* dly: sample_cnt	 DQS0_cnt  DQS1_cnt
	0 : 10962054,		 0, 	   0
	1 : 10958229,		 0, 	   0
	2 : 10961109,		 0, 	   0
	3 : 10946916,		 0, 	   0
	4 : 10955421,		 0, 	   0
	5 : 10967274,		 0, 	   0
	6 : 10893582,		 0, 	   0
	7 : 10974762,		 0, 	   0
	8 : 10990278,		 0, 	   0
	9 : 10972026,		 0, 	   0
   10 :  7421004,		 0, 	   0
   11 : 10943883,		 0, 	   0
   12 : 10984275,		 0, 	   0
   13 : 10955268,		 0, 	   0
   14 : 10960326,		 0, 	   0
   15 : 10952451,		 0, 	   0
   16 : 10956906,		 0, 	   0
   17 : 10960803,		 0, 	   0
   18 : 10944108,		 0, 	   0
   19 : 10959939,		 0, 	   0
   20 : 10959246,		 0, 	   0
   21 : 11002212,		 0, 	   0
   22 : 10919700,		 0, 	   0
   23 : 10977489,		 0, 	   0
   24 : 11009853,		 0, 	   0
   25 : 10991133,		 0, 	   0
   26 : 10990431,		 0, 	   0
   27 : 10970703,	 11161, 	   0
   28 : 10970775,	257118, 	   0
   29 : 10934442,  9450467, 	   0
   30 : 10970622, 10968475, 	   0
   31 : 10968831, 10968831, 	   0
   32 : 10956123, 10956123, 	   0
   33 : 10950273, 10950273, 	   0
   34 : 10975770, 10975770, 	   0
   35 : 10983024, 10983024, 	   0
   36 : 10981701, 10981701, 	   0
   37 : 10936782, 10936782, 	   0
   38 : 10889523, 10889523, 	   0
   39 : 10985913, 10985913,    55562
   40 : 10970235, 10970235,   272294
   41 : 10996056, 10996056,  9322868
   42 : 10972350, 10972350, 10969738
   43 : 10963917, 10963917, 10963917
   44 : 10967895, 10967895, 10967895
   45 : 10961739, 10961739, 10961739
   46 : 10937097, 10937097, 10937097
   47 : 10937952, 10937952, 10937952
   48 : 10926018, 10926018, 10926018
   49 : 10943793, 10943793, 10943793
   50 : 10954638, 10954638, 10954638
   51 : 10968048, 10968048, 10968048
   52 : 10944036, 10944036, 10944036
   53 : 11012112, 11012112, 11012112
   54 : 10969137, 10969137, 10969137
   55 : 10968516, 10968516, 10968516
   56 : 10952532, 10952532, 10952532
   57 : 10985832, 10985832, 10985832
   58 : 11002527, 11002527, 11002527
   59 : 10950660, 10873571, 10950660
   60 : 10949022, 10781797, 10949022
   61 : 10974366, 10700617, 10974366
   62 : 10972422,  1331974, 10972422
   63 : 10926567,		 0, 10926567
   64 : 10961658,		 0, 10961658
   65 : 10978893,		 0, 10978893
   66 : 10962828,		 0, 10962828
   67 : 10957599,		 0, 10957599
   68 : 10969227,		 0, 10969227
   69 : 10960722,		 0, 10960722
   70 : 10970937,		 0, 10963180
   71 : 10962054,		 0, 10711639
   72 : 10954719,		 0, 10612707
   73 : 10958778,		 0,   479589
   74 : 10973898,		 0, 	   0
   75 : 11004156,		 0, 	   0
   76 : 10944261,		 0, 	   0
   77 : 10955340,		 0, 	   0
   78 : 10998153,		 0, 	   0
   79 : 10998774,		 0, 	   0
   80 : 10953234,		 0, 	   0
   81 : 10960020,		 0, 	   0
   82 : 10923831,		 0, 	   0
   83 : 10951362,		 0, 	   0
   84 : 10965249,		 0, 	   0
   85 : 10949103,		 0, 	   0
   86 : 10948707,		 0, 	   0
   87 : 10941147,		 0, 	   0
   88 : 10966572,		 0, 	   0
   89 : 10971333,		 0, 	   0
   90 : 10943721,		 0, 	   0
   91 : 10949337,		 0, 	   0
   92 : 10965942,		 0, 	   0
   93 : 10970397,		 0, 	   0
   94 : 10956429,		 0, 	   0
   95 : 10939896,		 0, 	   0
   96 : 10967112,		 0, 	   0
   97 : 10951911,		 0, 	   0
   98 : 10953702,		 0, 	   0
   99 : 10971090,		 0, 	   0
  100 : 10939590,		 0, 	   0
  101 : 10993392,		 0, 	   0
  102 : 10975932,		 0, 	   0
  103 : 10949499,	 40748, 	   0
  104 : 10962522,	258638, 	   0
  105 : 10951524,	275292, 	   0
  106 : 10982475,	417642, 	   0
  107 : 10966887, 10564347, 	   0
  ===============================================================================
	  MIOCK jitter meter - channel=0
  ===============================================================================
  1T = (107-29) = 78 delay cells
  Clock frequency = 936 MHz, Clock period = 1068 ps, 1 delay cell = 13 ps
*/
}


void vResetPsramDelayChainBeforeCalibration(DRAMC_CTX_T *p)
{
    U8 u1RankIdx, u1RankIdxBak;

    u1RankIdxBak = u1GetRank(p);

    vIO32WriteFldMulti_All(DDRPHY_SHU1_R0_CA_CMD0, P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA5_DLY)
            | P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA4_DLY)
            | P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA3_DLY)
            | P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA2_DLY)
            | P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA1_DLY)
            | P_Fld(0x0, SHU1_R0_CA_CMD0_RK0_TX_ARCA0_DLY));
    vIO32WriteFldMulti_All(DDRPHY_SHU1_R0_B0_DQ0, P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0)
            | P_Fld(0x0, SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
#if 0
	vIO32WriteFldMulti_All(DDRPHY_SHU1_R0_B1_DQ0, P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1)
            | P_Fld(0x0, SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1));
#endif
    vIO32WriteFldAlign_All(DDRPHY_SHU1_R0_B0_DQ1, 0x0, SHU1_R0_B0_DQ1_RK0_TX_ARDQM0_DLY_B0);
  //  vIO32WriteFldAlign_All(DDRPHY_SHU1_R0_B1_DQ1, 0x0, SHU1_R0_B1_DQ1_RK0_TX_ARDQM0_DLY_B1);

}

void PsramcRxInputDelayTrackingInit_byFreq(DRAMC_CTX_T *p)
{
    U8 u1DVS_Delay;
    //Monitor window size setting
    //DDRPHY.SHU*_B*_DQ5.RG_RX_ARDQS0_DVS_DLY_B* (suggested value from A-PHY owner)
//WHITNEY_TO_BE_PORTING
#if (fcFOR_CHIP_ID == fcSchubert)
    //          Speed   Voltage     DVS_DLY
    //======================================
    //SHU1      3200    0.8V        3
    //SHU2      2667    0.8V-0.7V   4
    //SHU3      1600    0.7V-0.65V  5
    if(p->freqGroup == 2132)
    {
        u1DVS_Delay =2;
    }
    else
    if(p->freqGroup == 1866)
    {
        u1DVS_Delay =3;
    }
    else
    if(p->freqGroup == 1600)
    {
        u1DVS_Delay =3;
    }
    else if(p->freqGroup == 1333 || p->freqGroup == 1200)
    {
        u1DVS_Delay =4;
    }
    else// if(p->freqGroup == 800)
    {
        u1DVS_Delay =5;
    }
#endif

    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), u1DVS_Delay, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
   // vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), u1DVS_Delay, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);

    /* Bianco HW design issue: run-time PBYTE flag will lose it's function and become per-bit -> set to 0 */
	//#if PSRAM_SPEC
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7_PSRAM), P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM)
                                                            | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM));

	//#endif
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7), P_Fld(0x0, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0)
                                                            | P_Fld(0x0, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0));

  //  vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ7), P_Fld(0x0, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1)
   //                                                         | P_Fld(0x0, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B1));
}


void vApplyPsramConfigBeforeCalibration(DRAMC_CTX_T *p)
{
	U8 read_xrtw2w, shu_index;
	U8 u1RankIdx, u1RankIdxBak;

	//Clk free run
#if (SW_CHANGE_FOR_SIMULATION==0)
	EnablePsramcPhyDCM(p, 0);
#endif

	//Set LP3/LP4 Rank0/1 CA/TX delay chain to 0
#if (FOR_DV_SIMULATION_USED==0)
	//CA0~9 per bit delay line -> CHA_CA0 CHA_CA3 CHA_B0_DQ6 CHA_B0_DQ7 CHA_B0_DQ2 CHA_B0_DQ5 CHA_B0_DQ4 CHA_B0_DQ1 CHA_B0_DQ0 CHA_B0_DQ3
	vResetPsramDelayChainBeforeCalibration(p);
#endif

	{

		// Disable RX delay tracking
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2), 0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0);
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2), 0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B1);

		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2), 0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0);
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2), 0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B1);

		//RX delay mux, delay vlaue from reg.
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2), 0x0, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0);
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2), 0x0, R0_B1_RXDVS2_R_RK0_DVS_MODE_B1);
		vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_CA_RXDVS2), 0x0, R0_CA_RXDVS2_R_RK0_DVS_MODE_CA);
	}

	// ARPI_DQ SW mode mux, TX DQ use 1: PHY Reg 0: DRAMC Reg
#if 1//PSRAM_SPEC
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM), 1, MISC_CTRL1_PSRAM_R_DMARPIDQ_SW_PSRAM);
#endif
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 1, MISC_CTRL1_R_DMARPIDQ_SW);

    vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_IMPCAL), 1, IMPCAL_IMPCAL_USING_SYNC);
	//RG mode
	vIO32WriteFldAlign_All(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
	vIO32WriteFldAlign_All(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
	vIO32WriteFldAlign_All(DDRPHY_CA_CMD6, 0x1, CA_CMD6_RG_RX_ARCMD_BIAS_PS);

#if 1//ENABLE_RX_TRACKING_LP4
	//if(u1IsLP4Family(p->dram_type))
	{
		PsramcRxInputDelayTrackingInit_byFreq(p);
	}
    vIO32WriteFldAlign(PSRAMC_REG_DUMMY_RD, 1, DUMMY_RD_DMY_RD_RX_TRACK);
#endif

#ifdef LOOPBACK_TEST
#ifdef LPBK_INTERNAL_EN
	DramcLoopbackTest_settings(p, 0);	//0: internal loopback test 1: external loopback test
#else
	DramcLoopbackTest_settings(p, 1);	//0: internal loopback test 1: external loopback test
#endif
#endif

#if ENABLE_TMRRI_NEW_MODE
	//SetCKE2RankIndependent(p);
#endif


#ifdef IMPEDANCE_TRACKING_ENABLE
	//if(u1IsLP4Family(p->dram_type))
	{
		// set correct setting to control IMPCAL HW Tracking in shuffle RG
		// if p->freq >= 1333, enable IMP HW tracking(SHU1_DRVING1_DIS_IMPCAL_HW=0), else SHU1_DRVING1_DIS_IMPCAL_HW = 1
		U8 u1DisImpHw;

		u1DisImpHw = (p->frequency >= 1333)?0:1;
		vIO32WriteFldAlign_All(DRAMC_REG_SHU1_DRVING1, u1DisImpHw, SHU1_DRVING1_DIS_IMPCAL_HW);
	}
//	else
//	{
//		vIO32WriteFldAlign_All(DRAMC_REG_SHU1_DRVING1, 1, SHU1_DRVING1_DIS_IMPCAL_HW);
//	}
#endif
}


// Use gating old burst mode to find gating window boundary
// Set the begining of window as new burst mode gating window center.
#define LP4_GATING_OLD_BURST_MODE 1
#define LP4_GATING_LEAD_LAG_FLAG_JUDGE LP4_GATING_OLD_BURST_MODE

#if 1//PSRAM_SPEC
void PSramcGatingMode(DRAMC_CTX_T *p, U8 u1Mode)
{
	// mode 0:	old burst mode
	// mode 1:	7UI or 8UI gating window length mode (depends on if DQS_GW_7UI is defined)
#if (fcFOR_CHIP_ID == fcSchubert)
	/* There are currently 2 ways to set GatingMode (sets different registers)
	 * 1. Alaska
	 * 2. Bianco, Whitney, Kibo+(Olympus)
	 */
	U8 u1VrefSel = 0;

	//mcSHOW_DBG_MSG3("[GatingMode] ");
	if (u1Mode == 0) /* old mode */
	{
		u1VrefSel = 1;
		//mcSHOW_DBG_MSG3("old 8UI\n");
	}
	else /* 7UI or 8UI gating window length mode */
	{
#ifdef DQS_GW_7UI
		u1VrefSel = 2;
		//mcSHOW_DBG_MSG3("new 7UI\n");
#else
		u1VrefSel = 1;
		//mcSHOW_DBG_MSG3("new 8UI\n");
#endif
	}

	/* BIAS_VREF_SEL is used as switch for old, new burst modes */
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), u1VrefSel, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);
	//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), u1VrefSel, B1_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B1);
#endif /* fcFOR_CHIP_ID */

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), 1, B0_DQ9_PSRAM_RG_RX_ARDQS0_DQSIENMODE_B0_PSRAM);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), 1, B0_DQ9_RG_RX_ARDQS0_DQSIENMODE_B0);



	/* Perform reset (makes sure PHY's behavior works as the above setting) */
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(0, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(0, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));

	//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(0, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) |P_Fld(0, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
	mcDELAY_US(1);//delay 10ns
	//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(1, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) |P_Fld(1, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));

#if __ETT__
	if (!(u1IsLP4Family(p->dram_type)  || (p->dram_type == TYPE_PSRAM)))
	{  // GatingMode() is designed for LP4 only (only resets current channel PHY)
		mcSHOW_ERR_MSG("GatingMode err!(LP4/pSRAM only)\n");
	}
#endif
}
#endif

DRAM_STATUS_T PSramcRxdqsGatingCal(DRAMC_CTX_T *p)
{
	U8	ucdly_fine_xT, ucDQS_GW_FINE_STEP;
	U8	ucdqs_count_R, ucdqs_count_F;
	U32 u4dlyUI,u4BestdlyUI,u4BestdlyPI;
	U8	ucdqscount_pass = 0;
	U8	ucGatingPass = 0;
	U32 u4FirstPassUI,u4FirstPassPI;
	U8	ucGatingErrorStatus = 0;
	U32 u4Tmp,u4DebugCnt,u4value;
	U8 u1DQS_lead = 0, u1DQS_lag = 0, u1DQS_high = 0, u1DQS_transition = 0, u1DQSGatingFound = 0;
	U8 ucCoarseTune=0, ucCoarseStart=8, ucCoarseEnd=8;

	u4FirstPassUI = 0;
	u4FirstPassPI = 0;
	u4BestdlyUI = 0;
	u4BestdlyPI = 0;

	// error handling
	if (!p)
	{
		mcSHOW_ERR_MSG("context NULL\n");
		return DRAM_FAIL;
	}

	U32 u4RegBackupAddress[] =
	{
		(DRAMC_REG_ADDR(DDRPHY_B0_DQ6)),

	};

	//Register backup
	DramcBackupRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_STBENCMPEN);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_STB_GERRSTOP);
	
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_DQSGCNTEN);
	mcDELAY_US(4);//wait 1 auto refresh after DQS Counter enable

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_DQSG_CNT_RST);
	mcDELAY_US(1);//delay 2T
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 0, PSRAM_STBCAL_CTRL3_DQSG_CNT_RST);

	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1_PSRAM), u1GetRank(p), MISC_CTRL1_PSRAM_R_DMSTBENCMP_RK_OPT_PSRAM);
	PSramcEngine2Init(p, 0, 0x23,TEST_AUDIO_PATTERN, 0);

	mcSHOW_DBG_MSG2("[Psram Gating]\n");
	vPrintCalibrationBasicInfo(p);


	ucDQS_GW_FINE_STEP = DQS_GW_FINE_STEP;

	/* ucCoarseStart
	 * 1. Depends on current freq's DQSINCTL setting
	 * 2. Preserves ~4UI before actual DQS delay value
	 */
	if (p->freqGroup == 1200)  //2133
	{
		ucCoarseStart = 20;
	}
	else//1600
	{
		ucCoarseStart = 3;
	}

	ucCoarseEnd = ucCoarseStart + 15;

	mcSHOW_DBG_MSG2("Psram Gating UI from %d : %d \n",ucCoarseStart,ucCoarseEnd);
	mcSHOW_DBG_MSG2("(UI PI) | Gating Error | dqs_cnt_r  dqs_cnt_f | (dqs_lead dqs_lag)\n");
	for (ucCoarseTune = ucCoarseStart; ucCoarseTune < ucCoarseEnd; ucCoarseTune += DQS_GW_COARSE_STEP)
	{
		u4dlyUI = ucCoarseTune;
		vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0,u4dlyUI,PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0);

		for (ucdly_fine_xT=DQS_GW_FINE_START; ucdly_fine_xT<DQS_GW_FINE_END; ucdly_fine_xT+=ucDQS_GW_FINE_STEP)
		{

			u4value = ucdly_fine_xT;
			vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0, u4value, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0);

			//reset phy, reset read data counter
			PSramPhyReset(p);

			//reset DQS counter
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_DQSG_CNT_RST);
			mcDELAY_US(1);//delay 2T
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 0, PSRAM_STBCAL_CTRL3_DQSG_CNT_RST);

			//reset Gating ERR
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_STB_GERR_RST);
			mcDELAY_US(1);//delay 2T
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 0, PSRAM_STBCAL_CTRL3_STB_GERR_RST);

			 PSramcGatingMode(p, 1);
			 PSramcEngine2Run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);// trigger read to make lead/lag flag update.

			 u1DQS_lead = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS_STBEN_B0), MISC_PHY_RGS_STBEN_B0_AD_RX_ARDQS0_STBEN_LEAD_B0);
			 u1DQS_lag	= u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_PHY_RGS_STBEN_B0), MISC_PHY_RGS_STBEN_B0_AD_RX_ARDQS0_STBEN_LAG_B0);

			 //read gating error status;
			 ucGatingErrorStatus = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_GATING_CTRL_STATUS0), GATING_CTRL_STATUS0_STB_GATING_ERROR);

			  //read DQS counter
			 u4DebugCnt = u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_GATING_CTRL_STATUS1));
			 ucdqs_count_F = (u4DebugCnt >> 8) & 0xff;	//[16:8]
			 ucdqs_count_R = (u4DebugCnt) & 0xff;;		//[7:0]

			 // condition 1. check dqs error counts
			 if((ucdqs_count_F == 0x46)&&(ucdqs_count_R == 0x46))
				ucdqscount_pass = 1;
			 else
				ucdqscount_pass = 0;

			 // condition 2. check lead/lad transition(1->0), combile condition 1 pass
			 if(((u1DQS_lead == 1) && (u1DQS_lag == 1))&&(ucdqscount_pass == 1))
			 {
				 u1DQS_high ++;
			 }
			 else if((u1DQS_high > 0)&&(u1DQS_transition == 0))  //avoid continuous pass condition being break by glitch, if break, retry to count the pass condition
			 {
				 u1DQS_high = 0;
			 }

			 if(u1DQS_high * ucDQS_GW_FINE_STEP >16)//>16 PI prevent glitch
			 {
				 if((u1DQS_lead == 1) && (u1DQS_lag == 1))
				 {
					u1DQS_transition =1;
					ucGatingPass = 0;
					u4FirstPassUI = u4dlyUI;
					u4FirstPassPI = u4value;
				 }
				 else if(((u1DQS_lead == 1) && (u1DQS_lag == 0)) ||((u1DQS_lead == 0) && (u1DQS_lag == 1)))
				 {
					 if(u1DQS_transition == 1)
					 {
						 mcSHOW_DBG_MSG2("[Byte 0] Lead/lag falling Transition (UI PI) = (%d %d)\n",u4dlyUI,u4value);
					 }
					 u1DQS_transition ++;
					 ucGatingPass ++;
				 }
				 else if((u1DQS_lead == 0) && (u1DQS_lag == 0))
				 {
					 ucGatingPass ++;
				 }
#if 0
				 if(ucGatingPass == 1)//restore first lead/lag UI and PI for golden position
				 {
					 u4FirstPassUI = u4dlyUI;
					 u4FirstPassPI = u4value;
					 mcSHOW_DBG_MSG("First UI/PI Pass Lead/lag Transition (%d, %d)\n", u4FirstPassUI,u4FirstPassPI);
				 }
#endif
			 }

			 //3. exit the gating scan loop
			 if((ucGatingPass * ucDQS_GW_FINE_STEP > 8)&&(ucdly_fine_xT < DQS_GW_FINE_END))
			 {
				mcSHOW_DBG_MSG("[Byte 0] Lead/lag Transition tap number (%d)\n", u1DQS_transition);
				mcSHOW_DBG_MSG("Pass Window > 1UI,Psram Gating position found, Early Break!\n");
				u1DQS_high = 0;
				ucdly_fine_xT = DQS_GW_FINE_END;//break loop
				ucCoarseTune = ucCoarseEnd; 	 //break loop
			 }
			 else if((ucdly_fine_xT == (DQS_GW_FINE_END - 4))&&(ucCoarseTune == (ucCoarseEnd-1))&&(ucGatingPass == 0))// not found transition tap
			 {
				  mcSHOW_DBG_MSG("Psram Gating Fail!!!, not found Transition tap!!!\n");
				  vSetCalibrationResult(p, DRAM_CALIBRATION_GATING, DRAM_FAIL);
			 }

			mcSHOW_DBG_MSG2("(%d %d) | %d | 0x%x 0x%x | (%d %d) \n",u4dlyUI,u4value,ucGatingErrorStatus,ucdqs_count_F,ucdqs_count_R,u1DQS_lead,u1DQS_lag);					
		}
	}

	PSramcEngine2End(p);

#if !SW_CHANGE_FOR_SIMULATION
	vSetCalibrationResult(p, DRAM_CALIBRATION_GATING, DRAM_OK);
#endif

	//4.find center position
	// -- UI and PI window center
	u4Tmp = ((u4FirstPassUI<<5)+ u4FirstPassPI );//total PI

	if(u1DQS_transition > 1)
		u4Tmp = u4Tmp + (((u1DQS_transition - 1) * ucDQS_GW_FINE_STEP) >> 1); //move to center, if tansition tap more than one tap.

	u4BestdlyUI = u4Tmp>>5;    //calc UI position
	u4BestdlyPI = u4Tmp&(0x1f);  //calc PI position

	mcSHOW_DBG_MSG3("\n\tdqs input gating window, final dly value\n\n");
	mcFPRINTF(fp_A60501, "\n\tdqs input gating window, final dly value\n\n");

	/*TINFO="best DQS%d delay(2T, 0.5T, PI) = (%d, %d, %d)\n", dqs_i, ucbest_coarse_tune2T[dqs_i], ucbest_coarse_tune0p5T[dqs_i], ucbest_fine_tune[dqs_i]*/
	mcSHOW_DBG_MSG("\nbest DQS0 dly(UI, PI) = (%d, %d)\n", u4BestdlyUI, u4BestdlyPI);
	mcFPRINTF(fp_A60501,"best DQS0 dly(UI, PI) = (%d, %d)\n", u4BestdlyUI, u4BestdlyPI);

	mcSHOW_DBG_MSG("\n\n");
	mcFPRINTF(fp_A60501,"\n\n");

	//Restore registers
	DramcRestoreRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =1;
#endif

	//set Gating Best UI and PI
	vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0,u4BestdlyUI,PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0);
	vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0,u4BestdlyPI,PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0);

	//enable RODT
	vIO32WriteFldAlign(DDRPHY_PSRAM_RODT_CTRL0,1,PSRAM_RODT_CTRL0_RODTEN);

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =0;
#endif

	//mcDELAY_US(1);//delay 2T
	//DramPhyCGReset(p, 0);
	PSramPhyReset(p);	//reset phy, reset read data counter

	/*TINFO="[PsamcRxdqsGatingCal] Done\n"*/
	mcSHOW_DBG_MSG3("[PsramcRxdqsGatingCal] Done\n\n");
	mcFPRINTF(fp_A60501, "[PsramcRxdqsGatingCal] Done\n\n");

	return DRAM_OK;
	// log example
	/*******
	   will be add later
	  */
}


static void psram_dle_factor_handler(DRAMC_CTX_T *p, U8 curr_val, U8 pip_num)
{
    U8 u1DLECG_OptionStartEXT2=0, u1DLECG_OptionStartEXT3=0;
    U8 u1DLECG_OptionLastEXT2=0, u1DLECG_OptionLastEXT3=0;

#if REG_ACCESS_PORTING_DGB
    RegLogEnable =1;
    mcSHOW_DBG_MSG("\n[REG_ACCESS_PORTING_FUNC] dle_factor_handler\n");
    mcFPRINTF(fp_A60501, "\n[REG_ACCESS_PORTING_FUNC] dle_factor_handler\n");
#endif

    #if REG_SHUFFLE_REG_CHECK
    ShuffleRegCheck =1;
    #endif

    if(curr_val<2)
        curr_val =2;

    // Datlat_dsel = datlat -1, only 1 TX pipe
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_PSRAM_RDAT),
                                        P_Fld(curr_val, PSRAM_RDAT_DATLAT) |
                                        P_Fld(curr_val -1, PSRAM_RDAT_DATLAT_DSEL) |
                                        P_Fld(curr_val -1, PSRAM_RDAT_DATLAT_DSEL_PHY));

     vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_SHU_RX_CG_SET0), P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND1)
                | P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND1)
                | P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND2)
                | P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND2)
                | P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND3)
                | P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND3));

    #if REG_SHUFFLE_REG_CHECK
    ShuffleRegCheck =0;
    #endif

    PSramPhyReset(p);

#if REG_ACCESS_PORTING_DGB
    RegLogEnable =0;
#endif
}

static U8 aru1RxDatlatResult[CHANNEL_NUM][RANK_MAX];
U8 PSramcRxdatlatScan(DRAMC_CTX_T *p, DRAM_DATLAT_CALIBRATION_TYTE_T use_rxtx_scan)
{
	U8 ii, ucStartCalVal=0;
	U32 u4prv_register_080;
	U32 u4err_value= 0xffffffff;
	U8 ucfirst, ucbegin, ucsum, ucbest_step, ucpipe_num =0;
	U16 u2DatlatBegin;

	// error handling
	if (!p)
	{
		mcSHOW_ERR_MSG("context NULL\n");
		return DRAM_FAIL;
	}
#if REG_ACCESS_PORTING_DGB
	RegLogEnable =1;
	mcSHOW_DBG_MSG("\n[REG_ACCESS_PORTING_FUNC]   DramcRxdatlatCal\n");
	mcFPRINTF(fp_A60501, "\n[REG_ACCESS_PORTING_FUNC]	 DramcRxdatlatCal\n");
#endif

	mcSHOW_DBG_MSG("\n[DATLAT]\n"
					"Freq=%d, CH%d RK%d, use_rxtx_scan=%d\n\n",
						 p->frequency, p->channel, p->rank, use_rxtx_scan);

	mcFPRINTF(fp_A60501, "\n\tDATLAT calibration\n");
	mcFPRINTF(fp_A60501, "\tch=%d(1:cha), rank=%d, use_rxtx_scan=%d\n\n",
							 p->channel, p->rank, use_rxtx_scan);

	// [11:10] DQIENQKEND 01 -> 00 for DATLAT calibration issue, DQS input enable will refer to DATLAT
	// if need to enable this (for power saving), do it after all calibration done
	//u4prv_register_0d8 = u4IO32Read4B(DRAMC_REG_ADDR(DRAMC_REG_MCKDLY));
	//vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_PADCTRL), P_Fld(0, PADCTRL_DQIENQKEND) | P_Fld(0, PADCTRL_DQIENLATEBEGIN));

	// pre-save
	// 0x07c[6:4]	DATLAT bit2-bit0
	u4prv_register_080 = u4IO32Read4B(DRAMC_REG_ADDR(DDRPHY_PSRAM_RDAT));


	// init best_step to default
	ucbest_step = (U8) u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_RDAT), PSRAM_RDAT_DATLAT);
	mcSHOW_DBG_MSG("DATLAT Default: 0x%x\n", ucbest_step);
	mcFPRINTF(fp_A60501, "DATLAT Default: 0x%x\n", ucbest_step);

	// 1.set DATLAT 0-15 (0-21 for MT6595)
	// 2.enable engine1 or engine2
	// 3.check result  ,3~4 taps pass
	// 4.set DATLAT 2nd value for optimal

	// Initialize
	ucfirst = 0xff;
	ucbegin = 0;
	ucsum = 0;

	PSramcEngine2Init(p, 0, 0x10, TEST_AUDIO_PATTERN, 0);
#if (FOR_DV_SIMULATION_USED==0)
		u2DatlatBegin=8;
#else
		u2DatlatBegin=10;
#endif

#if (SUPPORT_SAVE_TIME_FOR_CALIBRATION && BYPASS_DATLAT)
	if(p->femmc_Ready==1)
	{
		ucbest_step = p->pSavetimeData->u1RxDatlat_Save[p->channel][p->rank];
	}
	else
#endif
	{
		for (ii = u2DatlatBegin; ii < DATLAT_TAP_NUMBER; ii++)
		{
			// 1
			psram_dle_factor_handler(p, ii, ucpipe_num);

			// 2
			if(use_rxtx_scan == fcDATLAT_USE_DEFAULT)
			{
				u4err_value = PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
			}

			// 3
			if (u4err_value == 0)
			{
				if (ucbegin == 0)
				{
					// first tap which is pass
					ucfirst = ii;
					ucbegin = 1;
				}
				if (ucbegin == 1)
				{
					ucsum++;

					if(ucsum >4)
						break;	//early break.
				}
			}
			else
			{
				if (ucbegin == 1)
				{
					// pass range end
					ucbegin = 0xff;
				}
			}

    #ifdef ETT_PRINT_FORMAT
			mcSHOW_DBG_MSG("%d, 0x%X, sum=%d\n", ii, u4err_value, ucsum);
    #else
			mcSHOW_DBG_MSG("TAP=%2d, err_value=0x%8x,	sum=%d\n", ii, u4err_value, ucsum);
    #endif
			mcFPRINTF(fp_A60501, "TAP=%2d, err_value=0x%8x, begin=%d, first=%3d, sum=%d\n", ii, u4err_value, ucbegin, ucfirst, ucsum);
		}

		PSramcEngine2End(p);

		// 4
		if (ucsum == 0)
		{
			mcSHOW_ERR_MSG("no DATLAT taps pass, DATLAT calibration fail!\n");
		}
		else if (ucsum <= 3)
		{
			ucbest_step = ucfirst + (ucsum>>1);
		}
		else // window is larger than 3
		{
			ucbest_step = ucfirst + 2;
		}
	}
	aru1RxDatlatResult[p->channel][p->rank] = ucbest_step;

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
	if(p->femmc_Ready==0)
	{
		p->pSavetimeData->u1RxDatlat_Save[p->channel][p->rank] = ucbest_step;
	}
#endif

	mcSHOW_DBG_MSG("best_step=%d\n\n", ucbest_step);
	mcFPRINTF(fp_A60501, "best_step=%d\n", ucbest_step);


#ifdef FOR_HQA_TEST_USED
	HQA_Log_Message_for_Report(p, p->channel, p->rank, HQA_REPORT_FORMAT2, "DATLAT", 0, ucbest_step, NULL);
#endif

#if (SUPPORT_SAVE_TIME_FOR_CALIBRATION && BYPASS_DATLAT)
	if(p->femmc_Ready==1)
	{
		psram_dle_factor_handler(p, ucbest_step, ucpipe_num);
		vSetCalibrationResult(p, DRAM_CALIBRATION_DATLAT, DRAM_OK);
	}
	else
#endif
	{
		if(ucsum <4)
		{
			mcSHOW_DBG_MSG2("[NOTICE] CH%d, DatlatSum %d\n", p->channel, ucsum);
			mcFPRINTF(fp_A60501, "[NOTICE] CH%d, DatlatSum  %d\n", p->channel, ucsum);
		}

		if (ucsum == 0)
		{
			mcSHOW_ERR_MSG("DATLAT calibration fail, write back to default values!\n");
			vIO32Write4B(DRAMC_REG_ADDR(DDRPHY_PSRAM_RDAT), u4prv_register_080);
			vSetCalibrationResult(p, DRAM_CALIBRATION_DATLAT, DRAM_FAIL);
		}
		else
		{
			psram_dle_factor_handler(p, ucbest_step, ucpipe_num);
			vSetCalibrationResult(p, DRAM_CALIBRATION_DATLAT, DRAM_OK);
		}
	}
	// [11:10] DQIENQKEND 01 -> 00 for DATLAT calibration issue, DQS input enable will refer to DATLAT
	// if need to enable this (for power saving), do it after all calibration done
	vIO32WriteFldMulti(DRAMC_REG_ADDR(DRAMC_REG_PADCTRL), P_Fld(1, PADCTRL_DQIENQKEND) | P_Fld(1, PADCTRL_DQIENLATEBEGIN));


	//reset Gating ERR
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 1, PSRAM_STBCAL_CTRL3_STB_GERR_RST);
	mcDELAY_US(1);//delay 2T
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_PSRAM_STBCAL_CTRL3), 0, PSRAM_STBCAL_CTRL3_STB_GERR_RST);

	mcSHOW_DBG_MSG3("[DramcRxdatlatCal] Done\n");
	mcFPRINTF(fp_A60501, "[DramcRxdatlatCal] Done\n");

#if REG_ACCESS_PORTING_DGB
	RegLogEnable =0;
#endif

	return ucsum;
}


void PsramSetRxDqDqsDelay(DRAMC_CTX_T *p, S16 iDelay )
{
	U8 ii, u1ByteIdx;
	U32 u4value;
	U8 dl_value[8];

	if (iDelay <=0)
	{
		//if(u1IsLP4Family(p->dram_type))
		{
			// Set DQS delay
			vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6), \
									P_Fld((-iDelay /*+gu2RX_DQS_Duty_Offset[0][0]*/),SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |P_Fld((-iDelay/*+gu2RX_DQS_Duty_Offset[0][1]*/),SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0));
		}

		PSramPhyReset(p);
	}
	else
	{
		// Adjust DQM output delay.
		//if(u1IsLP4Family(p->dram_type))
		{
			vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6), \
									P_Fld(iDelay,SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0) |P_Fld(iDelay,SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));
		}

		PSramPhyReset(p);

		// Adjust DQ output delay.
		//if(u1IsLP4Family(p->dram_type))
		{
			u4value = ((U32) iDelay) | (((U32)iDelay)<<8) | (((U32)iDelay)<<16) | (((U32)iDelay)<<24);
			for (ii=0; ii<4; ii++)
			{
				vIO32Write4B(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2+ ii*4), u4value);//DQ0~DQ7
			}
#if PINMUX_AUTO_TEST_PER_BIT_RX
            if(gPsram_RX_check_per_bit_flag)
            {
                SetPsramRxPerBitDelayCellForPinMuxCheck(p, gPsram_RX_Check_per_bit_idx);
                //CheckRXDelayCell(p);
            }
#endif
		}
	}
}

#define RX_DELAY_PRE_CAL 1
#if RX_DELAY_PRE_CAL
static S16 s2RxDelayPreCal=PASS_RANGE_NA;
#endif
S16 gu2RX_DQS_Duty_Offset[DQS_NUMBER][2];
DRAM_STATUS_T PSramcRxWindowPerbitCal(DRAMC_CTX_T *p, U8 u1UseTestEngine)
{
	U8 ii, u1BitIdx, u1ByteIdx;
	U32 u1vrefidx;
	U8 ucbit_first, ucbit_last;
	S16 iDelay=0, u4DelayBegin=0, u4DelayEnd, u4DelayStep=1;
	S16 iDutyOffset=0, u4DutyOffsetBegin, u4DutyOffsetEnd, u4DutyOffsetStep=4;
	U32 uiFinishCount;
	U32 u4value, u4err_value, u4fail_bit;
	PASS_WIN_DATA_T WinPerBit[8], FinalWinPerBit[8];
	S32 iDQSDlyPerbyte = 0, iDQMDlyPerbyte = 0;//, iFinalDQSDly[DQS_NUMBER];
	U8 u1VrefScanEnable;
	U16 u2VrefLevel, u2TempWinSum, u2TmpDQMSum, u2FinalVref=0xb;
	U16 u2VrefBegin, u2VrefEnd, u2VrefStep;
	U32 u4fail_bit_R, u4fail_bit_F;
	U32 u4AddrOfst = 0x50;
	U8	u1RXEyeScanEnable;
	U8 dl_value[8]={0,0,0,0,0,0,0,0};
	U8 backup_rank;
	U32 min_winsize, min_bit;

#if 0//PPORT_SAVE_TIME_FOR_CALIBRATION
   S16 u1minfirst_pass=0xff,u1minlast_pass=0xff,u4Delayoffset;
   U8  u1AllBitPassCount;
#endif

#if (EYESCAN_LOG || PSRAM_RX_EYE)
	U8 EyeScan_index[DQ_DATA_WIDTH_PSRAM];
	U8 u1pass_in_this_vref_flag[DQ_DATA_WIDTH_PSRAM];
#endif


#if REG_ACCESS_PORTING_DGB
	RegLogEnable =1;
	mcSHOW_DBG_MSG("\n[REG_ACCESS_PORTING_FUNC] DramcRxWindowPerbitCal\n");
	mcFPRINTF(fp_A60501, "\n[REG_ACCESS_PORTING_FUNC] DramcRxWindowPerbitCal\n");
#endif

	// error handling
	if (!p)
	{
		mcSHOW_ERR_MSG("context NULL\n");
		return DRAM_FAIL;
	}

	backup_rank = u1GetRank(p);

	u1RXEyeScanEnable = (gRX_EYE_Scan_flag==1 && ((gRX_EYE_Scan_only_higheset_freq_flag==1 && p->frequency == u2DFSGetHighestFreq(p)) || gRX_EYE_Scan_only_higheset_freq_flag==0));

#if (EYESCAN_LOG || PSRAM_RX_EYE)
	//if (u1IsLP4Family(p->dram_type))
	{
		for(u1vrefidx=0; u1vrefidx<RX_VREF_RANGE_END+1;u1vrefidx++)
		{
			for (u1BitIdx = 0; u1BitIdx < DQ_DATA_WIDTH_PSRAM; u1BitIdx++)
			{
				for(ii=0; ii<EYESCAN_BROKEN_NUM; ii++)
				{
					gEyeScan_Min[u1vrefidx][u1BitIdx][ii] = EYESCAN_DATA_INVALID;
					gEyeScan_Max[u1vrefidx][u1BitIdx][ii] = EYESCAN_DATA_INVALID;

					gEyeScan_ContinueVrefHeight[u1BitIdx] = 0;
					gEyeScan_TotalPassCount[u1BitIdx] = 0;
				}
			}
		}
	}
#endif

	//defult set result fail. When window found, update the result as oK
	if(u1UseTestEngine)
	{
		vSetCalibrationResult(p, DRAM_CALIBRATION_RX_PERBIT, DRAM_FAIL);
		PSramcEngine2Init(p, 0, 0x23, TEST_AUDIO_PATTERN, 0);
	}
	else
	{
		mcSHOW_DBG_MSG("PSRAM RX not support RDDQC!!!\n");
		return DRAM_FAIL;
		//vSetCalibrationResult(p, DRAM_CALIBRATION_RX_RDDQC, DRAM_FAIL);
	}

	// for loop, different Vref,
	u2rx_window_sum = 0;

	if((u1UseTestEngine==1) && (p->enable_rx_scan_vref==ENABLE) && (p->rank==RANK_0)) //only apply RX Vref Scan for Rank 0 (Rank 0 and 1 use the same Vref reg)
		u1VrefScanEnable =1;
	else
		u1VrefScanEnable =0;

#if 0
	if (gRX_EYE_Scan_flag==1 && u1VrefScanEnable==0)
	{
		if(u1IsLP4Family(p->dram_type) && (u1UseTestEngine==1) && (p->enable_rx_scan_vref==ENABLE) && (p->rank==RANK_1)) //also need to K rank1 for debug
			u1VrefScanEnable =1;
	}
#endif

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
	if(p->femmc_Ready==1 && (p->Bypass_RDDQC || p->Bypass_RXWINDOW))
	{
		mcSHOW_DBG_MSG("[FAST_K] Bypass RX Calibration\n");
	}
	else
#endif
	{
#if VENDER_JV_LOG
#if 0 //BU don't want customer knows our RX's ability
		if(u1UseTestEngine ==1)
			vPrintCalibrationBasicInfo_ForJV(p);
#endif
#else
		vPrintCalibrationBasicInfo(p);
#endif
		mcSHOW_DBG_MSG2("Start DQ dly to find pass range UseTestEngine =%d\n", u1UseTestEngine);
		mcSHOW_DBG_MSG2("x-axis: bit #, y-axis: DQ dly (%d~%d)\n", (-MAX_RX_DQSDLY_TAPS), MAX_RX_DQDLY_TAPS);
		mcFPRINTF(fp_A60501, "Start RX DQ/DQS calibration UseTestEngine =%d\n", u1UseTestEngine);
		mcFPRINTF(fp_A60501, "x-axis is bit #; y-axis is DQ delay\n");
	}
	mcSHOW_DBG_MSG("RX Vref Scan = %d\n", u1VrefScanEnable);
	mcFPRINTF(fp_A60501, "RX Vref Scan= %d\n", u1VrefScanEnable);

	if(u1VrefScanEnable)
	{
#if (SW_CHANGE_FOR_SIMULATION ||FOR_DV_SIMULATION_USED)
		u2VrefBegin =RX_VREF_RANGE_BEGIN;
#else
		if (gRX_EYE_Scan_flag==0)
		{
			if(p->odt_onoff)
			{
				u2VrefBegin = RX_VREF_RANGE_BEGIN_ODT_ON;
			}
			else
			{
				u2VrefBegin = RX_VREF_RANGE_BEGIN_ODT_OFF;
			}
		}
		else
			u2VrefBegin = 0;//Lewis@20160817: Enlarge RX Vref range for eye scan
#if PSRAM_RX_EYE
		u2VrefBegin = 0;
#endif
#endif

		u2VrefEnd =RX_VREF_RANGE_END;
		u2VrefStep=RX_VREF_RANGE_STEP;
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), 1, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), 1, B1_DQ5_RG_RX_ARDQ_VREF_EN_B1);
	}
	else //LPDDR3 or diable RX Vref
	{
		u2VrefBegin = 0;
		u2VrefEnd = 0; // SEL[4:0] = 01110
		u2VrefStep =1; //don't care, just make for loop break;
	}

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
	if(p->femmc_Ready==1 && (p->Bypass_RDDQC || p->Bypass_RXWINDOW))
	{
		if(u1VrefScanEnable)
		{
			// load RX Vref from eMMC
    #if ( SUPPORT_SAVE_TIME_FOR_CALIBRATION && BYPASS_VREF_CAL)
			//u2VrefBegin = p->pSavetimeData->u1RxWinPerbitVref_Save[p->channel];
			//u2VrefEnd=u2VrefBegin+1;
			u2FinalVref = p->pSavetimeData->u1RxWinPerbitVref_Save[p->channel];
    #endif
		}

		// load RX DQS and DQM delay from eMMC
		//for (u1ByteIdx=0; u1ByteIdx<DQS_NUMBER_LP4; u1ByteIdx++)
		{
			iDQSDlyPerbyte = p->pSavetimeData->u1RxWinPerbit_DQS[p->channel][p->rank][0];
			iDQMDlyPerbyte = p->pSavetimeData->u1RxWinPerbit_DQM[p->channel][p->rank][0];
		}

		// load RX DQ delay from eMMC
		for (u1BitIdx=0; u1BitIdx<8; u1BitIdx++)
		{
			FinalWinPerBit[u1BitIdx].best_dqdly= p->pSavetimeData->u1RxWinPerbit_DQ[p->channel][p->rank][u1BitIdx];
		}

		if(u1UseTestEngine)
			vSetCalibrationResult(p, DRAM_CALIBRATION_RX_PERBIT, DRAM_OK);
		else
			vSetCalibrationResult(p, DRAM_CALIBRATION_RX_RDDQC, DRAM_OK);
	}
	else
#endif
	{
		//if(u1IsLP4Family(p->dram_type))
		{
    #if RX_DELAY_PRE_CAL
			if(u1UseTestEngine==0)
    #endif
			{
				if(p->frequency >=1600)
				{
					u4DelayBegin= -26;
				}
				else if(p->frequency >= 1140)
				{
					u4DelayBegin= -30;
				}
				else if(p->frequency >=800)
				{
					u4DelayBegin= -48;
				}
				else
				{
					u4DelayBegin= -48; //-MAX_RX_DQSDLY_TAPS;
				}

				s2RxDelayPreCal =PASS_RANGE_NA;
			}
    #if RX_DELAY_PRE_CAL
			else
			{
				//u4DelayBegin = s2RxDelayPreCal -10;  // for test engine
				u4DelayBegin = -48; //-MAX_RX_DQSDLY_TAPS;
			}
    #endif

			//mcSHOW_DBG_MSG("RX_DELAY_PRE_CAL: RX delay begin = %d\n", u4DelayBegin);

			u4DelayEnd = MAX_RX_DQDLY_TAPS;

			if(p->frequency <850)
				u4DelayStep =4;  //1600
			else
				u4DelayStep =1;//2667, 3200

			if(u1UseTestEngine==0) //if RDDQD, roughly calibration
				u4DelayStep <<= 1;
		}

		u4DutyOffsetBegin = 0;
		u4DutyOffsetEnd = 0;
		u4DutyOffsetStep = 1;

#if !REDUCE_LOG_FOR_PRELOADER
		mcSHOW_DBG_MSG("\nRX DQS R/F Scan, iDutyOffset= %d\n", iDutyOffset);
		mcFPRINTF(fp_A60501, "\nRX DQS R/F Scan, iDutyOffset= %d\n", iDutyOffset);
#endif

		for(u2VrefLevel = u2VrefBegin; u2VrefLevel <= u2VrefEnd; u2VrefLevel += u2VrefStep)
		{
			min_winsize = 0xFFFF;
			min_bit = 0xFF;

			if(u1VrefScanEnable ==1)
			{
        #if !REDUCE_LOG_FOR_PRELOADER
				mcSHOW_DBG_MSG("\n\tRX VrefLevel=%d\n", u2VrefLevel);
				mcFPRINTF(fp_A60501, "\n\tRX VrefLevel=%d\n", u2VrefLevel);
        #endif
#if 0
        #if VENDER_JV_LOG
				if(u1UseTestEngine ==1)
				mcSHOW_JV_LOG_MSG("\n\tRX VrefLevel=%d\n", u2VrefLevel);
        #endif
#endif

				//Set RX Vref Here
				vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), u2VrefLevel, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);  // LP4 and LP4x with term: 0xe
				//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), u2VrefLevel, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);  // LP4 and LP4x with term: 0xe
			}

			// 1.delay DQ ,find the pass widnow (left boundary).
			// 2.delay DQS find the pass window (right boundary).
			// 3.Find the best DQ / DQS to satify the middle value of the overall pass window per bit
			// 4.Set DQS delay to the max per byte, delay DQ to de-skew

			for (iDutyOffset=u4DutyOffsetBegin; iDutyOffset<=u4DutyOffsetEnd; iDutyOffset+=u4DutyOffsetStep)
			{
				// initialize parameters
				u2TempWinSum =0;
				uiFinishCount =0;

				for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
				{
					WinPerBit[u1BitIdx].first_pass = (S16)PASS_RANGE_NA;
					WinPerBit[u1BitIdx].last_pass = (S16)PASS_RANGE_NA;
					FinalWinPerBit[u1BitIdx].first_pass = (S16)PASS_RANGE_NA;
					FinalWinPerBit[u1BitIdx].last_pass = (S16)PASS_RANGE_NA;

            #if (EYESCAN_LOG || PSRAM_RX_EYE)
					//if (u1IsLP4Family(p->dram_type))
					{
						gEyeScan_CaliDelay[u1BitIdx/8] = 0;
						gEyeScan_DelayCellPI[u1BitIdx] = 0;
						EyeScan_index[u1BitIdx] = 0;
						u1pass_in_this_vref_flag[u1BitIdx] = 0;
					}
            #endif
				}

				// Adjust DQM output delay to 0
				//if(u1IsLP4Family(p->dram_type))
				{
					vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6), \
											P_Fld(0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0) |P_Fld(0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));
					//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ6), \
					//				P_Fld(0,SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_R_DLY_B1) |P_Fld(0, SHU1_R0_B1_DQ6_RK0_RX_ARDQM0_F_DLY_B1));

				}

				// Adjust DQ output delay to 0
				//every 2bit dq have the same delay register address
				//if(u1IsLP4Family(p->dram_type))
				{
					for (ii=0; ii<4; ii++)
					{
						vIO32Write4B(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2+ ii*4), 0);//DQ0~DQ7
					//	vIO32Write4B(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ2+ ii*4), 0);//DQ8~DQ15
					}
				}

				for (iDelay=u4DelayBegin; iDelay<=u4DelayEnd; iDelay+= u4DelayStep)
				{

					PsramSetRxDqDqsDelay(p, iDelay);

					if(u1UseTestEngine)
					{
						u4err_value = PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
					}
					else
					{
						u4err_value = 0xffff;
					}

					if((u1VrefScanEnable==0) || u1RXEyeScanEnable)
					{
                #ifdef ETT_PRINT_FORMAT
						if(u4err_value !=0)
						{
							mcSHOW_DBG_MSG2("%d, [LSB]", iDelay);
						}
                #else
						mcSHOW_DBG_MSG2("iDelay= %4d, [LSB]", iDelay);
                #endif
						mcFPRINTF(fp_A60501, "iDelay= %4d, [LSB]", iDelay);
						//mcSHOW_DBG_MSG2("u4err_value %x, u1MRRValue %x\n", u4err_value, u1MRRValue);
						//mcFPRINTF(fp_A60501, "u4err_value!!	%x", u4err_value);
					}

					// check fail bit ,0 ok ,others fail
					for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
					{
						u4fail_bit = u4err_value&((U32)1<<u1BitIdx);

						if(WinPerBit[u1BitIdx].first_pass== PASS_RANGE_NA)
						{
							if(u4fail_bit==0) //compare correct: pass
							{
								WinPerBit[u1BitIdx].first_pass = iDelay;

                        #if RX_DELAY_PRE_CAL
								if(u1UseTestEngine==0 && (s2RxDelayPreCal ==PASS_RANGE_NA))
								{
									s2RxDelayPreCal = iDelay;
								}

								if(u1UseTestEngine==1 && iDelay==u4DelayBegin)
								{
									mcSHOW_ERR_MSG("RX_DELAY_PRE_CAL: Warning, possible miss RX window boundary\n");
                            #if __ETT__
									while(1);
                            #endif
								}

                        #endif

                        #if (EYESCAN_LOG || PSRAM_RX_EYE)
								//if (u1IsLP4Family(p->dram_type))
								{
									u1pass_in_this_vref_flag[u1BitIdx]=1;
								}
                        #endif
							}
						}
						else if(WinPerBit[u1BitIdx].last_pass == PASS_RANGE_NA)
						{
							//mcSHOW_DBG_MSG("fb%d \n", u4fail_bit);

							if(u4fail_bit !=0) //compare error : fail
							{
								WinPerBit[u1BitIdx].last_pass  = (iDelay-1);
							}
							else if (iDelay==u4DelayEnd)
							{
								WinPerBit[u1BitIdx].last_pass  = iDelay;
							}

							if(WinPerBit[u1BitIdx].last_pass  !=PASS_RANGE_NA)
							{
								if((WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass) >= (FinalWinPerBit[u1BitIdx].last_pass -FinalWinPerBit[u1BitIdx].first_pass))
								{
                            #if 0 //for debug
									if(FinalWinPerBit[u1BitIdx].last_pass != PASS_RANGE_NA)
									{
										mcSHOW_DBG_MSG2(("Bit[%d] Bigger window update %d > %d\n", u1BitIdx, \
											(WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass), (FinalWinPerBit[u1BitIdx].last_pass -FinalWinPerBit[u1BitIdx].first_pass)));
										mcFPRINTF((fp_A60501,"Bit[%d] Bigger window update %d > %d\n", u1BitIdx, \
											(WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass), (FinalWinPerBit[u1BitIdx].last_pass -FinalWinPerBit[u1BitIdx].first_pass)));
									}
                            #endif

									//if window size bigger than 7, consider as real pass window. If not, don't update finish counte and won't do early break;
									if((WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass) >7)
										uiFinishCount |= (1<<u1BitIdx);

									//update bigger window size
									FinalWinPerBit[u1BitIdx].first_pass = WinPerBit[u1BitIdx].first_pass;
									FinalWinPerBit[u1BitIdx].last_pass = WinPerBit[u1BitIdx].last_pass;
								}

                        #if (EYESCAN_LOG || PSRAM_RX_EYE)
								if(u1UseTestEngine)
								{
									if (EyeScan_index[u1BitIdx] < EYESCAN_BROKEN_NUM)
									{
										gEyeScan_Min[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]] = WinPerBit[u1BitIdx].first_pass;
										gEyeScan_Max[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]] = WinPerBit[u1BitIdx].last_pass;
										//mcSHOW_DBG_MSG3("u2VrefLevel=%d, u1BitIdx=%d, index=%d (%d, %d)==\n",u2VrefLevel, u1BitIdx, EyeScan_index[u1BitIdx], gEyeScan_Min[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]], gEyeScan_Max[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]]);
										EyeScan_index[u1BitIdx]=EyeScan_index[u1BitIdx]+1;
									}
								}
                        #endif

								//reset tmp window
								WinPerBit[u1BitIdx].first_pass = PASS_RANGE_NA;
								WinPerBit[u1BitIdx].last_pass = PASS_RANGE_NA;
							}
						}

						if((u1VrefScanEnable==0) || u1RXEyeScanEnable)
						{
					#ifdef ETT_PRINT_FORMAT
							if(u4err_value !=0)
                    #endif
							{
								if(u1BitIdx%DQS_BIT_NUMBER ==0)
								{
									mcSHOW_DBG_MSG2(" ");
									mcFPRINTF(fp_A60501, " ");
								}

								if (u4fail_bit == 0)
								{
									mcSHOW_DBG_MSG2("o");
									mcFPRINTF(fp_A60501, "o");
                            #if (EYESCAN_LOG || PSRAM_RX_EYE)
									//if (u1IsLP4Family(p->dram_type))
									{
										 gEyeScan_TotalPassCount[u1BitIdx]+=u4DelayStep;
									}
                            #endif
								}
								else
								{
									mcSHOW_DBG_MSG2("x");
									mcFPRINTF(fp_A60501, "x");
								}
							}
#if (EYESCAN_LOG || PSRAM_RX_EYE)
							else
							{
								//if (u1IsLP4Family(p->dram_type))
								{
									gEyeScan_TotalPassCount[u1BitIdx]+=u4DelayStep;
								}
							}
#endif
						}
					}

					if((u1VrefScanEnable==0)  || u1RXEyeScanEnable)
					{
                #ifdef ETT_PRINT_FORMAT
						if(u4err_value !=0)
                #endif
						{
							mcSHOW_DBG_MSG2(" [MSB]\n");
							mcFPRINTF(fp_A60501, " [MSB]\n");
						}
					}

					//if all bits widnow found and all bits turns to fail again, early break;
					if((uiFinishCount == 0xff))
					{
						if(u1UseTestEngine)
							vSetCalibrationResult(p, DRAM_CALIBRATION_RX_PERBIT, DRAM_OK);
						else
							vSetCalibrationResult(p, DRAM_CALIBRATION_RX_RDDQC, DRAM_OK);

						if((u1VrefScanEnable==0)  || u1RXEyeScanEnable)
						{
							if((u4err_value&0xff) == 0xff)
								{
                                #if !REDUCE_LOG_FOR_PRELOADER
										mcSHOW_DBG_MSG("\nRX all bits window found, early break!\n");
                                #endif
										break;	//early break
								}
						}
					}
				}

				for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
				{
					FinalWinPerBit[u1BitIdx].win_size = FinalWinPerBit[u1BitIdx].last_pass - FinalWinPerBit[u1BitIdx].first_pass + (FinalWinPerBit[u1BitIdx].last_pass==FinalWinPerBit[u1BitIdx].first_pass?0:1);
#if PINMUX_AUTO_TEST_PER_BIT_RX
                    gPsramFinalRXPerbitWinSiz[p->channel][u1BitIdx] = FinalWinPerBit[u1BitIdx].win_size;
#endif
					u2TempWinSum += FinalWinPerBit[u1BitIdx].win_size;	//Sum of CA Windows for vref selection

					if ( FinalWinPerBit[u1BitIdx].win_size < min_winsize) {
                        min_bit = u1BitIdx;
                        min_winsize =  FinalWinPerBit[u1BitIdx].win_size;
                    }

#if 0 //BU don't want customer knows our RX's ability
            #if VENDER_JV_LOG
					if(u1UseTestEngine ==1)
					{
						U8 shuffleIdx;
						shuffleIdx = get_shuffleIndex_by_Freq(p);
						mcSHOW_JV_LOG_MSG("RX Bit%d, %d%%\n", u1BitIdx,  ((FinalWinPerBit[u1BitIdx].win_size*u2gdelay_cell_ps_all[shuffleIdx][0]*p->frequency*2)+(1000000-1))/1000000);
					}
            #endif
#endif
#if (EYESCAN_LOG || PSRAM_RX_EYE)
					//if (u1IsLP4Family(p->dram_type))
					{
						gEyeScan_WinSize[u2VrefLevel][u1BitIdx] = FinalWinPerBit[u1BitIdx].win_size;
					}
#endif
				}

        #if 1//!REDUCE_LOG_FOR_PRELOADER
				mcSHOW_DBG_MSG2("RX Vref %d, Window Sum %d\n", u2VrefLevel, u2TempWinSum);
				mcFPRINTF(fp_A60501, "RX Vref %d, Window Sum %d\n", u2VrefLevel, u2TempWinSum);
        #endif

        #if 0
				if((u1IsLP4Family(p->dram_type) &&(u2TempWinSum <250)) || \
					((p->dram_type == TYPE_LPDDR3) &&(u2TempWinSum <1100)))
				{
						mcSHOW_DBG_MSG2("\n\n[NOTICE] CH %d, TestEngine %d, RX_sum %d\n", p->channel, u1UseTestEngine, u2TempWinSum);
						mcFPRINTF(fp_A60501, "\n\n[NOTICE] CH %d, TestEngine %d, RX_sum %d\n", p->channel, u1UseTestEngine, u2TempWinSum);
				}
        #endif

				if(u2TempWinSum >u2rx_window_sum)
				{
					mcSHOW_DBG_MSG3("\nBetter RX Vref found %d, Window Sum %d > %d\n", u2VrefLevel, u2TempWinSum, u2rx_window_sum);
					mcFPRINTF(fp_A60501, "\nBetter RX Vref found %d, Window Sum %d > %d\n", u2VrefLevel, u2TempWinSum, u2rx_window_sum);

					rx_perbit_win_min_max = min_winsize;
					rx_perbit_win_min_max_idx = min_bit;
					u2rx_window_sum =u2TempWinSum;
					u2FinalVref = u2VrefLevel;

					for (u1BitIdx=0; u1BitIdx<p->data_width; u1BitIdx++)
					{
						FinalWinPerBit[u1BitIdx].win_center = (FinalWinPerBit[u1BitIdx].last_pass + FinalWinPerBit[u1BitIdx].first_pass)>>1;	 // window center of each DQ bit

						if((u1VrefScanEnable==0) || u1RXEyeScanEnable)
						{
                    #ifdef ETT_PRINT_FORMAT
							mcSHOW_DBG_MSG("iDelay=%d, Bit %d, Center %d (%d ~ %d) %d\n", iDelay, u1BitIdx, FinalWinPerBit[u1BitIdx].win_center, FinalWinPerBit[u1BitIdx].first_pass, FinalWinPerBit[u1BitIdx].last_pass, FinalWinPerBit[u1BitIdx].win_size);
                    #else
							mcSHOW_DBG_MSG("iDelay=%d, Bit %2d, Center %3d (%4d ~ %4d) %d\n", iDelay, u1BitIdx, FinalWinPerBit[u1BitIdx].win_center, FinalWinPerBit[u1BitIdx].first_pass, FinalWinPerBit[u1BitIdx].last_pass, FinalWinPerBit[u1BitIdx].win_size);
                    #endif
							mcFPRINTF(fp_A60501, "iDelay=%d, Bit %2d, Center %3d (%4d ~ %4d)\n", iDelay, u1BitIdx, FinalWinPerBit[u1BitIdx].win_center, FinalWinPerBit[u1BitIdx].first_pass, FinalWinPerBit[u1BitIdx].last_pass, FinalWinPerBit[u1BitIdx].win_size);
						}

#ifdef FOR_HQA_TEST_USED
						if(u1UseTestEngine ==1)
						{
							gFinalRXPerbitWin[p->channel][p->rank][u1BitIdx] = FinalWinPerBit[u1BitIdx].win_size;
						}
#endif
					}
				}
			}

#if (EYESCAN_LOG || PSRAM_RX_EYE)
			//if (u1IsLP4Family(p->dram_type))
			{
				for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
				{
					if (u1pass_in_this_vref_flag[u1BitIdx]) gEyeScan_ContinueVrefHeight[u1BitIdx]++;  //count pass number of continue vref
				}
			}
#endif
			#if (PSRAM_RX_EYE == 0)
			if((u2TempWinSum < (u2rx_window_sum*95/100)) && gRX_EYE_Scan_flag == 0)
			{
				mcSHOW_DBG_MSG("\nRX Vref found, early break!\n");
				u2VrefLevel = u2VrefEnd;
				break;//max vref found, early break;
			}
			#endif
		}

		if(u1UseTestEngine)
		{
			PSramcEngine2End(p);
		}

#ifdef DRAM_SLT
			int bit_idx;
			mcSHOW_INFO_MSG("\n");
			for (bit_idx = 0; bit_idx < p->data_width; bit_idx++) {
#ifdef DRAM_SLT
			mcSHOW_INFO_MSG("[Eye Check][%d][RX] Bit%d Center %d (%d ~ %d) %d\n",\
							p->frequency * 2,\
							bit_idx, FinalWinPerBit[bit_idx].win_center, FinalWinPerBit[bit_idx].first_pass,\
							FinalWinPerBit[bit_idx].last_pass, FinalWinPerBit[bit_idx].win_size);
#endif
			}
#ifdef DRAM_SLT
			mcSHOW_INFO_MSG("\n[Eye Check][%d][RX] Best Vref %d, Window Min %d at DQ%d, Window Sum %d\n\n",\
							p->frequency*2,\
							u2FinalVref, rx_perbit_win_min_max, rx_perbit_win_min_max_idx, u2rx_window_sum);
#endif
#ifdef ENABLE_MIOCK_JMETER
			if (p->frequency >= PERBIT_THRESHOLD_FREQ) {
				if(((rx_perbit_win_min_max*100)/(p->ucnum_dlycell_perT>>1)) < RX_WIN_CRITERIA) {
					mcSHOW_ERR_MSG("[Eye Check]RX margin fail@DQ%d, 0.%dUI<0.%dUI\n",
									rx_perbit_win_min_max_idx, ((rx_perbit_win_min_max*100)/(p->ucnum_dlycell_perT>>1)), RX_WIN_CRITERIA);
#ifdef DRAM_SLT
					dram_slt_set(p, DRAM_CALIBRATION_RX_PERBIT, DRAM_FAIL);
#endif
				}
			}
#endif
#endif

		// 3
		//As per byte, check max DQS delay in 8-bit. Except for the bit of max DQS delay, delay DQ to fulfill setup time = hold time
		for (u1ByteIdx = 0; u1ByteIdx < (p->data_width/DQS_BIT_NUMBER); u1ByteIdx++)
		{
			u2TmpDQMSum =0;

			ucbit_first =DQS_BIT_NUMBER*u1ByteIdx;
			ucbit_last = DQS_BIT_NUMBER*u1ByteIdx+DQS_BIT_NUMBER-1;
			iDQSDlyPerbyte = MAX_RX_DQSDLY_TAPS;

			for (u1BitIdx = ucbit_first; u1BitIdx <= ucbit_last; u1BitIdx++)
			{
				// find out min Center value
				if(FinalWinPerBit[u1BitIdx].win_center < iDQSDlyPerbyte)
				{
					iDQSDlyPerbyte = FinalWinPerBit[u1BitIdx].win_center;
				}

				//mcSHOW_DBG_MSG("bit#%2d : center=(%2d)\n", u1BitIdx, FinalWinPerBit[u1BitIdx].win_center);
				//mcFPRINTF(fp_A60501, "bit#%2d : center=(%2d)\n", u1BitIdx, FinalWinPerBit[u1BitIdx].win_center);
			}

			//mcSHOW_DBG_MSG("----seperate line----\n");
			//mcFPRINTF(fp_A60501, "----seperate line----\n");

			if (iDQSDlyPerbyte  > 0)  // Delay DQS=0, Delay DQ only
			{
				iDQSDlyPerbyte  = 0;
			}
			else  //Need to delay DQS
			{
				iDQSDlyPerbyte = -iDQSDlyPerbyte ;
			}

			// we delay DQ or DQS to let DQS sample the middle of rx pass window for all the 8 bits,
			for (u1BitIdx = ucbit_first; u1BitIdx <= ucbit_last; u1BitIdx++)
			{
				FinalWinPerBit[u1BitIdx].best_dqdly = iDQSDlyPerbyte + FinalWinPerBit[u1BitIdx].win_center;
				u2TmpDQMSum += FinalWinPerBit[u1BitIdx].best_dqdly;
#if (EYESCAN_LOG || PSRAM_RX_EYE)
				//if (u1IsLP4Family(p->dram_type))
				{
					gEyeScan_DelayCellPI[u1BitIdx] = FinalWinPerBit[u1BitIdx].best_dqdly;
				}
#endif
			}

			// calculate DQM as average of 8 DQ delay
			iDQMDlyPerbyte = u2TmpDQMSum/DQS_BIT_NUMBER;

#ifdef FOR_HQA_REPORT_USED
			HQA_Log_Message_for_Report(p, p->channel, p->rank, HQA_REPORT_FORMAT0, "RX_Window_Center_DQS", u1ByteIdx, iDQSDlyPerbyte, NULL);
			HQA_Log_Message_for_Report(p, p->channel, p->rank, HQA_REPORT_FORMAT0, "RX_Window_Center_DQM", u1ByteIdx, iDQMDlyPerbyte, NULL);
			for (u1BitIdx = ucbit_first; u1BitIdx <= ucbit_last; u1BitIdx++)
			{
				HQA_Log_Message_for_Report(p, p->channel, p->rank, HQA_REPORT_FORMAT1, "RX_Window_Center_DQ", u1BitIdx, FinalWinPerBit[u1BitIdx].win_center, NULL);
			}
#endif
		}
	}


	//Set RX Final Vref Here
	if(u1VrefScanEnable==1)
	{
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), u2FinalVref, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);  // LP4 and LP4x with term: 0xe
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), u2FinalVref, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);  // LP4 and LP4x with term: 0xe

		mcSHOW_DBG_MSG("\nFinal RX Vref %d, apply to both rank0 and 1\n", u2FinalVref);
		mcFPRINTF(fp_A60501, "\nFinal RX Vref %d, apply to both rank0 and 1\n", u2FinalVref);
#if 0 //BU don't want customer knows our RX's ability
#if VENDER_JV_LOG
		mcSHOW_JV_LOG_MSG("\nFinal RX Vref %d, apply to both rank0 and 1\n", u2FinalVref);
#endif
#endif

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
		if(p->femmc_Ready==0)
		{
			p->pSavetimeData->u1RxWinPerbitVref_Save[p->channel]=u2FinalVref;
		}
#endif

		// When only calibrate RX Vref for Rank 0, apply the same value for Rank 1.
		gFinalRXVrefDQ[p->channel][p->rank] = (U8) u2FinalVref;
	}

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =1;
#endif

	gu2RX_DQS_Duty_Offset[0][0]=gu2RX_DQS_Duty_Offset[0][1]=gu2RX_DQS_Duty_Offset[1][0]=gu2RX_DQS_Duty_Offset[1][1] = 0;

	// set dqs delay, (dqm delay)
	for (u1ByteIdx = 0; u1ByteIdx < (p->data_width/DQS_BIT_NUMBER); u1ByteIdx++)
	{
		// Set DQS & DQM delay
		//if(u1IsLP4Family(p->dram_type))
		{
			vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ6+ u4AddrOfst*u1ByteIdx), \
				P_Fld(((U32)iDQSDlyPerbyte+gu2RX_DQS_Duty_Offset[u1ByteIdx][0]),SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0) |P_Fld(((U32)iDQSDlyPerbyte+gu2RX_DQS_Duty_Offset[u1ByteIdx][1]),SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0) |
				P_Fld(((U32)iDQMDlyPerbyte),SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0) |P_Fld(((U32)iDQMDlyPerbyte),SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0));

    #if SUPPORT_SAVE_TIME_FOR_CALIBRATION
			if(p->femmc_Ready==0)
			{
				p->pSavetimeData->u1RxWinPerbit_DQS[p->channel][p->rank][u1ByteIdx]=(U32)iDQSDlyPerbyte;
				p->pSavetimeData->u1RxWinPerbit_DQM[p->channel][p->rank][u1ByteIdx]=(U32)iDQMDlyPerbyte;
			}
    #endif
		}
	}

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =0;
#endif

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =1;
#endif

	// set dq delay
	//if(u1IsLP4Family(p->dram_type))
	{
		for (u1BitIdx=0; u1BitIdx<DQS_BIT_NUMBER; u1BitIdx+=2)
		{
			 vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ2+u1BitIdx*2), \
											P_Fld(((U32)FinalWinPerBit[uiPSRAM_DQ_Mapping_POP[p->channel][u1BitIdx]].best_dqdly),SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0) |\
											P_Fld(((U32)FinalWinPerBit[uiPSRAM_DQ_Mapping_POP[p->channel][u1BitIdx]].best_dqdly),SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0)| \
											P_Fld(((U32)FinalWinPerBit[uiPSRAM_DQ_Mapping_POP[p->channel][u1BitIdx+1]].best_dqdly),SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0) |\
											P_Fld(((U32)FinalWinPerBit[uiPSRAM_DQ_Mapping_POP[p->channel][u1BitIdx+1]].best_dqdly),SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0));
        #if 0
			 vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ2+u1BitIdx*2), \
											P_Fld(((U32)FinalWinPerBit[u1BitIdx+8].best_dqdly),SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_R_DLY_B1) | \
											P_Fld((U32)FinalWinPerBit[u1BitIdx+8].best_dqdly,SHU1_R0_B1_DQ2_RK0_RX_ARDQ0_F_DLY_B1)| \
											P_Fld(((U32)FinalWinPerBit[u1BitIdx+9].best_dqdly),SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_R_DLY_B1) |\
											P_Fld((U32)FinalWinPerBit[u1BitIdx+9].best_dqdly,SHU1_R0_B1_DQ2_RK0_RX_ARDQ1_F_DLY_B1));
        #endif
			//mcSHOW_DBG_MSG(("u1BitId %d  Addr 0x%2x = %2d %2d %2d %2d \n", u1BitIdx, DRAMC_REG_ADDR(DDRPHY_RXDQ1+u1BitIdx*2), \
			//				  FinalWinPerBit[u1BitIdx].best_dqdly, FinalWinPerBit[u1BitIdx+1].best_dqdly,  FinalWinPerBit[u1BitIdx+8].best_dqdly, FinalWinPerBit[u1BitIdx+9].best_dqdly));
		}

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
		if(p->femmc_Ready==0)
		{
			for (u1BitIdx=0; u1BitIdx<16; u1BitIdx++)
			{
				p->pSavetimeData->u1RxWinPerbit_DQ[p->channel][p->rank][u1BitIdx]=(U32)FinalWinPerBit[u1BitIdx].best_dqdly;

			}
		}
#endif
	}

	PSramPhyReset(p);

	vPrintCalibrationBasicInfo(p);

#ifdef ETT_PRINT_FORMAT
	//if(u1IsLP4Family(p->dram_type))
	{
		mcSHOW_DBG_MSG("DQS Delay:\nDQS0 = %d,"
						"DQM Delay:\nDQM0 = %d/n",
							iDQSDlyPerbyte,
							iDQMDlyPerbyte);
	}
#else
	//if(u1IsLP4Family(p->dram_type))
	{
		mcSHOW_DBG_MSG("DQS Delay:\nDQS0 = %2d,"
						"DQM Delay:\nDQM0 = %2d\n",
							iDQSDlyPerbyte,
							iDQMDlyPerbyte);
	}
#endif
	mcSHOW_DBG_MSG("DQ Delay:\n");

	mcFPRINTF(fp_A60501, "\tdramc_rxdqs_perbit_swcal\n");
	mcFPRINTF(fp_A60501, "\tchannel=%d(1:cha) \n", p->channel);
	mcFPRINTF(fp_A60501, "\tbus width=%d\n\n", p->data_width);

	//if(u1IsLP4Family(p->dram_type))
	{
		mcFPRINTF(fp_A60501, "DQS Delay:\n DQS0 = %2d\n", iDQSDlyPerbyte);
		mcFPRINTF(fp_A60501, "DQM Delay:\n DQM0 = %2d\n", iDQMDlyPerbyte);
	}
	mcFPRINTF(fp_A60501, "DQ Delay:\n");

	for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx=u1BitIdx+4)
	{
#ifdef ETT_PRINT_FORMAT
		mcSHOW_DBG_MSG("DQ%d =%d, DQ%d =%d, DQ%d =%d, DQ%d =%d\n", u1BitIdx, FinalWinPerBit[u1BitIdx].best_dqdly, u1BitIdx+1, FinalWinPerBit[u1BitIdx+1].best_dqdly, u1BitIdx+2, FinalWinPerBit[u1BitIdx+2].best_dqdly, u1BitIdx+3, FinalWinPerBit[u1BitIdx+3].best_dqdly);
#else
		mcSHOW_DBG_MSG("DQ%2d =%2d, DQ%2d =%2d, DQ%2d =%2d, DQ%2d =%2d\n", u1BitIdx, FinalWinPerBit[u1BitIdx].best_dqdly, u1BitIdx+1, FinalWinPerBit[u1BitIdx+1].best_dqdly, u1BitIdx+2, FinalWinPerBit[u1BitIdx+2].best_dqdly, u1BitIdx+3, FinalWinPerBit[u1BitIdx+3].best_dqdly);
#endif
		mcFPRINTF(fp_A60501, "DQ%2d =%2d, DQ%2d =%2d, DQ%2d =%2d, DQ%2d=%2d\n", u1BitIdx, FinalWinPerBit[u1BitIdx].best_dqdly, u1BitIdx+1, FinalWinPerBit[u1BitIdx+1].best_dqdly, u1BitIdx+2, FinalWinPerBit[u1BitIdx+2].best_dqdly, u1BitIdx+3, FinalWinPerBit[u1BitIdx+3].best_dqdly);
	}
	mcSHOW_DBG_MSG("\n\n");
	mcFPRINTF(fp_A60501, "\n\n");

	// BU request RX & TX window size log.
#if 0//def RELEASE  // for parsing tool
	if(u1UseTestEngine==1)
	{
		mcSHOW_DBG_MSG4("RX CH%d R%d ,Freq %d\n", p->channel, p->rank, p->frequency);
		for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
		{
			mcSHOW_DBG_MSG4("%d: %d\n", u1BitIdx, gFinalRXPerbitWin[p->channel][p->rank][u1BitIdx]);
		}
	}
#endif

#if REG_SHUFFLE_REG_CHECK
	ShuffleRegCheck =0;
#endif
	mcSHOW_DBG_MSG3("[DramcRxWindowPerbitCal] Done\n");
	mcFPRINTF(fp_A60501, "[DramcRxWindowPerbitCal] Done\n");

#if REG_ACCESS_PORTING_DGB
	RegLogEnable =0;
#endif

#if PSRAM_RX_EYE
	mcSHOW_DBG_MSG3("\n\n === RX eye scan Vref:(%d, %d, %d) delay:(-16, %d, %d) ===\n\n",
		u2VrefBegin, u2VrefEnd, u2VrefStep, u4DelayEnd, u4DelayStep);
	for (ii=0; ii < DQ_DATA_WIDTH_PSRAM; ii++)
	{
		mcSHOW_DBG_MSG3("\n\n--- bit[%d] ---\n", ii);
		for (u2VrefLevel = u2VrefBegin; u2VrefLevel <= u2VrefEnd; u2VrefLevel += u2VrefStep)
		{
			mcSHOW_DBG_MSG3("  ");
			//for (iDelay=u4DelayBegin; iDelay<=u4DelayEnd; iDelay+= u4DelayStep)
			for (iDelay = -64; iDelay <= u4DelayEnd; iDelay += u4DelayStep)
			{
				if ((iDelay > gEyeScan_Max[u2VrefLevel][ii][2]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][2]))
					mcSHOW_DBG_MSG3("x");
				else if ((iDelay > gEyeScan_Min[u2VrefLevel][ii][2]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][2]))
					mcSHOW_DBG_MSG3("o");
				else if ((iDelay > gEyeScan_Max[u2VrefLevel][ii][1]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][1]))
					mcSHOW_DBG_MSG3("x");
				else if ((iDelay > gEyeScan_Min[u2VrefLevel][ii][1]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][1]))
					mcSHOW_DBG_MSG3("o");
				else if ((iDelay > gEyeScan_Max[u2VrefLevel][ii][0]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][0]))
					mcSHOW_DBG_MSG3("x");
				else if ((iDelay > gEyeScan_Min[u2VrefLevel][ii][0]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][0]))
					mcSHOW_DBG_MSG3("o");
				else
					mcSHOW_DBG_MSG3("x");
			}

			mcSHOW_DBG_MSG3("  Vref[%d]: win=%d, 0(%d, %d), 1(%d, %d), 2(%d, %d)\n",
				u2VrefLevel,
				gEyeScan_WinSize[u2VrefLevel][ii],
				gEyeScan_Min[u2VrefLevel][ii][0],
				gEyeScan_Max[u2VrefLevel][ii][0],
				gEyeScan_Min[u2VrefLevel][ii][1],
				gEyeScan_Max[u2VrefLevel][ii][1],
				gEyeScan_Min[u2VrefLevel][ii][2],
				gEyeScan_Max[u2VrefLevel][ii][2]);
		}
	}
	mcSHOW_DBG_MSG3("\n\n");
#endif

return DRAM_OK;

	// Log example	==> Neec to update
	/*
------------------------------------------------------
Start calculate dq time and dqs time /
Find max DQS delay per byte / Adjust DQ delay to align DQS...
------------------------------------------------------
bit# 0 : dq time=11 dqs time= 8
bit# 1 : dq time=11 dqs time= 8
bit# 2 : dq time=11 dqs time= 6
bit# 3 : dq time=10 dqs time= 8
bit# 4 : dq time=11 dqs time= 8
bit# 5 : dq time=10 dqs time= 8
bit# 6 : dq time=11 dqs time= 8
bit# 7 : dq time= 9 dqs time= 6
----seperate line----
bit# 8 : dq time=12 dqs time= 7
bit# 9 : dq time=10 dqs time= 8
bit#10 : dq time=11 dqs time= 8
bit#11 : dq time=10 dqs time= 8
bit#12 : dq time=11 dqs time= 8
bit#13 : dq time=11 dqs time= 8
bit#14 : dq time=11 dqs time= 8
bit#15 : dq time=12 dqs time= 8
----seperate line----
bit#16 : dq time=11 dqs time= 7
bit#17 : dq time=10 dqs time= 8
bit#18 : dq time=11 dqs time= 7
bit#19 : dq time=11 dqs time= 6
bit#20 : dq time=10 dqs time= 9
bit#21 : dq time=11 dqs time=10
bit#22 : dq time=11 dqs time=10
bit#23 : dq time= 9 dqs time= 9
----seperate line----
bit#24 : dq time=12 dqs time= 6
bit#25 : dq time=13 dqs time= 6
bit#26 : dq time=13 dqs time= 7
bit#27 : dq time=11 dqs time= 7
bit#28 : dq time=12 dqs time= 8
bit#29 : dq time=10 dqs time= 8
bit#30 : dq time=13 dqs time= 7
bit#31 : dq time=11 dqs time= 8
----seperate line----
==================================================
	dramc_rxdqs_perbit_swcal_v2
	channel=2(2:cha, 3:chb) apply = 1
==================================================
DQS Delay :
 DQS0 = 0 DQS1 = 0 DQS2 = 0 DQS3 = 0
DQ Delay :
DQ 0 =	1 DQ 1 =  1 DQ 2 =	2 DQ 3 =  1
DQ 4 =	1 DQ 5 =  1 DQ 6 =	1 DQ 7 =  1
DQ 8 =	2 DQ 9 =  1 DQ10 =	1 DQ11 =  1
DQ12 =	1 DQ13 =  1 DQ14 =	1 DQ15 =  2
DQ16 =	2 DQ17 =  1 DQ18 =	2 DQ19 =  2
DQ20 =	0 DQ21 =  0 DQ22 =	0 DQ23 =  0
DQ24 =	3 DQ25 =  3 DQ26 =	3 DQ27 =  2
DQ28 =	2 DQ29 =  1 DQ30 =	3 DQ31 =  1
_______________________________________________________________
   */
}


void vApplyPsramConfigAfterCalibration(DRAMC_CTX_T *p)
{
	U8 shu_index;
/*================================
	PHY RX Settings
==================================*/

	vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL4_PSRAM, 0x11400000, MISC_CG_CTRL4_PSRAM_R_PHY_MCK_CG_CTRL_PSRAM);
	//vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL4, 0x11400000, MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);
	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ8_PSRAM, 0x1, SHU1_B0_DQ8_PSRAM_R_RMRX_TOPHY_CG_IG_B0_PSRAM);

	/* Burst mode settings are removed from here due to
	 *	1. Set in UpdateInitialSettings_LP4
	 *	2. DQS Gating ensures new burst mode is switched when to done
	 *	   (or doesn't switch gatingMode at all, depending on "LP4_GATING_OLD_BURST_MODE")
	 */

	vIO32WriteFldAlign_All(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN);

	//DA mode
	vIO32WriteFldAlign_All(DDRPHY_B0_DQ6, 0x0, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
	//vIO32WriteFldAlign_All(DDRPHY_B1_DQ6, 0x0, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
	vIO32WriteFldAlign_All(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_BIAS_PS);

	vIO32WriteFldAlign_All(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0);
	//vIO32WriteFldAlign_All(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1);
	vIO32WriteFldAlign_All(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN);


/*================================
	IMPCAL Settings
==================================*/
	vIO32WriteFldMulti_All(DRAMC_REG_IMPCAL, P_Fld(0, IMPCAL_IMPCAL_IMPPDP) | P_Fld(0, IMPCAL_IMPCAL_IMPPDN));	  //RG_RIMP_BIAS_EN and RG_RIMP_VREF_EN move to IMPPDP and IMPPDN
	vIO32WriteFldAlign_All(DDRPHY_MISC_IMP_CTRL0, 0, MISC_IMP_CTRL0_RG_IMP_EN);

	//Prevent M_CK OFF because of hardware auto-sync
	vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0, Fld(4,0));

	//DFS- fix Gating Tracking settings
	vIO32WriteFldAlign_All(DDRPHY_MISC_CTRL0_PSRAM, 0, MISC_CTRL0_PSRAM_R_STBENCMP_DIV4CK_EN_PSRAM);
	vIO32WriteFldAlign_All(DDRPHY_MISC_CTRL1_PSRAM, 0, MISC_CTRL1_PSRAM_R_DMSTBENCMP_RK_OPT_PSRAM);
    vIO32WriteFldMulti(DDRPHY_PSRAM_STBCAL_CTRL3, P_Fld(0x0, PSRAM_STBCAL_CTRL3_STB_GERRSTOP)
			 | P_Fld(0x0, PSRAM_STBCAL_CTRL3_DQSGCNTEN));

#if 1
	vIO32WriteFldAlign(PSRAMC_REG_REFCTRL0, 0x0, REFCTRL0_REFDIS);
#endif

//#if 0 //this should mark
	vIO32WriteFldMulti(DDRPHY_PSRAM_RODT_CTRL0, P_Fld(0x10, PSRAM_RODT_CTRL0_RODTENSTB_EXT)
				| P_Fld(0x4, PSRAM_RODT_CTRL0_RODTENSTB__UI_OFFSET));
	vIO32WriteFldMulti(DDRPHY_PSRAM_RODT_CTRL1, P_Fld(0x2, PSRAM_RODT_CTRL1_RODTENCGEN_TAIL)
				| P_Fld(0x2, PSRAM_RODT_CTRL1_RODTENCGEN_HEAD));

	vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_RX_CTRL0, P_Fld(0x1, PSRAM_APHY_RX_CTRL0_RX_APHY_CTRL_DCM_OPT)
				| P_Fld(0x2, PSRAM_APHY_RX_CTRL0_RX_IN_GATE_EN_TAIL)
				| P_Fld(0x1, PSRAM_APHY_RX_CTRL0_RX_IN_GATE_EN_HEAD));
	vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_RX_CTRL1, P_Fld(0x0, PSRAM_APHY_RX_CTRL0_RX_IN_GATE_EN_HEAD)
				| P_Fld(0x2, PSRAM_APHY_RX_CTRL0_RX_IN_GATE_EN_TAIL));
//#endif

      if(0) //2133 mp setting
   {

		vIO32WriteFldAlign(PSRAMC_REG_DUMMY_RD, 0x1, DUMMY_RD_DUMMY_RD_EN);
		vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL0, 0x1, ZQC_CTRL0_ZQCSDISB);
		vIO32WriteFldAlign(PSRAMC_REG_DRAMC_PD_CTRL, 0x1, PSRAMC_PD_CTRL_DCMEN);
		vIO32WriteFldMulti(PSRAMC_REG_HMR4, P_Fld(0x0, HMR4_REFRDIS)
			| P_Fld(0x1, HMR4_REFR_PERIOD_OPT));
		vIO32WriteFldAlign(PSRAMC_REG_REFCTRL1, 0x1, REFCTRL1_REFPEND_OPT1);
		vIO32WriteFldAlign(PSRAMC_REG_SREF_DPD_CTRL, 0x0, SREF_DPD_CTRL_SREF_CG_OPT);
		vIO32WriteFldAlign(PSRAMC_REG_ACTIMING_CTRL, 0x0, ACTIMING_CTRL_SEQCLKRUN);

		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8, P_Fld(0x0, SHU1_B0_DQ8_R_DMRANK_CHG_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMRANK_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMSTBEN_SYNC_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMRXDLY_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0)
		                                    | P_Fld(0x0, SHU1_B0_DQ8_R_RMRODTEN_CG_IG_B0)
		                                    | P_Fld(0, SHU1_B0_DQ8_R_DMRANK_RXDLY_PIPE_CG_IG_B0));		

		vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD8, P_Fld(0x0, SHU1_CA_CMD8_R_DMRANK_CHG_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_DMRANK_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_DMDQSIEN_RDSEL_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_DMDQSIEN_FLAG_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_DMDQSIEN_FLAG_SYNC_CG_IG_CA)
											| P_Fld(0x1, SHU1_CA_CMD8_R_DMSTBEN_SYNC_CG_IG_CA)
											| P_Fld(0x1, SHU1_CA_CMD8_R_DMRXDLY_CG_IG_CA)
											| P_Fld(0x1, SHU1_CA_CMD8_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_CA)
											| P_Fld(0x1, SHU1_CA_CMD8_R_DMRXDVS_RDSEL_PIPE_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_RMRX_TOPHY_CG_IG_CA)
											| P_Fld(0x0, SHU1_CA_CMD8_R_RMRODTEN_CG_IG_CA));
   }

	vIO32WriteFldMulti(PSRAMC_REG_TEST2_A3, P_Fld(0x0, TEST2_A3_TESTAUDPAT)
		| P_Fld(0x1, TEST2_A3_TEST2WREN2_HW_EN));

       vIO32WriteFldAlign(PSRAMC_REG_DUMMY_RD, 0x1, DUMMY_RD_DMY_RD_RX_TRACK);

}

static void TxPrintWidnowInfo(DRAMC_CTX_T *p, PASS_WIN_DATA_T WinPerBitData[])
{
    U8 u1BitIdx;

    for (u1BitIdx = 0; u1BitIdx < DQS_BIT_NUMBER; u1BitIdx++)
    {
    #ifdef ETT_PRINT_FORMAT
        mcSHOW_DBG_MSG("TX Bit%d (%d~%d) %d %d", \
            u1BitIdx, WinPerBitData[u1BitIdx].first_pass, WinPerBitData[u1BitIdx].last_pass,
            WinPerBitData[u1BitIdx].win_size, WinPerBitData[u1BitIdx].win_center);
    #else
        mcSHOW_DBG_MSG("TX Bit%2d (%2d~%2d) %2d %2d", \
            u1BitIdx, WinPerBitData[u1BitIdx].first_pass, WinPerBitData[u1BitIdx].last_pass,
            WinPerBitData[u1BitIdx].win_size, WinPerBitData[u1BitIdx].win_center);
    #endif
        mcFPRINTF(fp_A60501,"TX Bit%2d (%2d~%2d) %2d %2d", \
            u1BitIdx, WinPerBitData[u1BitIdx].first_pass, WinPerBitData[u1BitIdx].last_pass,
            WinPerBitData[u1BitIdx].win_size, WinPerBitData[u1BitIdx].win_center);

        //if(u1IsLP4Family(p->dram_type))
        {
            mcSHOW_DBG_MSG("\n");
            mcFPRINTF(fp_A60501,"\n");
        }
    }
    mcSHOW_DBG_MSG("\n");
    mcFPRINTF(fp_A60501,"\n");
}

#define TX_TDQS2DQ_PRE_CAL 0

U8  u1TX_CA_PreCal_Result;
U16 u2TX_DQ_PreCal_PSRAM;

static void PSramTXScanRange_PI(DRAMC_CTX_T *p, DRAM_TX_PER_BIT_CALIBRATION_TYTE_T calType, U16 *pu2Begin, U16 *pu2End)
{
    U8 u1MCK2UI;
    U32 u4RegValue_TXDLY, u4RegValue_dly;
    U8 ucdq_ui_large_bak, ucdq_ui_small_bak;
    U16 u2SmallestVirtualDelay;
    U16 u2DQDelayBegin=0, u2DQDelayEnd=0;

    u4RegValue_TXDLY= u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_SHU_SELPH_DQS0));
    u4RegValue_dly= u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_SHU_SELPH_DQS1));

    u1MCK2UI= 3;

    // find DQS delay
    ucdq_ui_large_bak = (u4RegValue_TXDLY) &0x7;// MCK
    ucdq_ui_small_bak = (u4RegValue_dly) &0x7;// UI
    //wrlevel_dqs_final_delay[u1ByteIdx]  ==> PI

    //LP4 : Virtual Delay = 256 * MCK + 32*UI + PI;
    //LP3 : Virtual Delay = 128 * MCK + 32*UI + PI;
    //PSRAM:Virtual Delay = 256 * MCK + 32*UI + PI;
    u2SmallestVirtualDelay = (((ucdq_ui_large_bak <<u1MCK2UI) + ucdq_ui_small_bak)<<5);// + wrlevel_dqs_final_delay[u1ByteIdx];

    // (1)PSRAM will first K 2 UI range which can guarantee CA always OK(set CA center align with CK in initial setting )
    // (2)The sencod time will calibrate DQ perbit,  reference TX postion of (1)
    if(calType==TX_DQ_DQS_MOVE_DQ_DQM)
    {
        u2DQDelayBegin = u2SmallestVirtualDelay-12; //lzs mark here move DQ -12PI, but must make sure CA can ok
        u2DQDelayEnd = u2DQDelayBegin + 64;  //scan range 2UI
    }
    else //if(calType==TX_DQ_DQS_MOVE_DQ_ONLY)
    {
	    u2DQDelayBegin = u2TX_DQ_PreCal_PSRAM-16;
        u2DQDelayEnd = u2DQDelayBegin + 64;
    }


    *pu2Begin = u2DQDelayBegin;
    *pu2End = u2DQDelayEnd;

    #if 1
    mcSHOW_DBG_MSG("TXScanRange_PI %d~%d\n", u2DQDelayBegin,u2DQDelayEnd);
    #endif
}


static U16 TxChooseVref(DRAMC_CTX_T *p, PASS_WIN_DATA_BY_VREF_T pVrefInfo[], U8 u1VrefNum)
{
    U8 u1VrefIdx;
    U8 u1VrefPassBegin=LP4_TX_VREF_BOUNDARY_NOT_READY, u1VrefPassEnd=LP4_TX_VREF_BOUNDARY_NOT_READY, u1TempPassNum=0, u1MaxVerfPassNum=0;
    U8 u1VrefPassBegin_Final=LP4_TX_VREF_BOUNDARY_NOT_READY, u1VrefPassEnd_Final=LP4_TX_VREF_BOUNDARY_NOT_READY;
    U16 u2MaxMinSize=0, u2MaxWinSum=0;
    U16 u2FinalVref=0;

    for(u1VrefIdx=0; u1VrefIdx < u1VrefNum; u1VrefIdx++)
    {
        mcSHOW_DBG_MSG("Vref=%d, minWin=%d, winSum=%d\n",
            pVrefInfo[u1VrefIdx].u2VrefUsed,
            pVrefInfo[u1VrefIdx].u2MinWinSize_byVref,
            pVrefInfo[u1VrefIdx].u2WinSum_byVref);

        #if LP4_TX_VREF_PASS_CONDITION
        if((pVrefInfo[u1VrefIdx].u2MinWinSize_byVref > LP4_TX_VREF_PASS_CONDITION))
        {
            if(u1VrefPassBegin ==LP4_TX_VREF_BOUNDARY_NOT_READY)
            {
                u1VrefPassBegin = pVrefInfo[u1VrefIdx].u2VrefUsed;
                u1TempPassNum =1;
            }
            else
                u1TempPassNum ++;

            if(u1VrefIdx==u1VrefNum-1)
            {
                u1VrefPassEnd = pVrefInfo[u1VrefIdx].u2VrefUsed;
                if(u1TempPassNum > u1MaxVerfPassNum)
                {
                    u1VrefPassBegin_Final= u1VrefPassBegin;
                    u1VrefPassEnd_Final = u1VrefPassEnd;
                    u1MaxVerfPassNum= u1TempPassNum;
                }
            }
        }
        else
        {
            if((u1VrefPassBegin != LP4_TX_VREF_BOUNDARY_NOT_READY) && (u1VrefPassEnd==LP4_TX_VREF_BOUNDARY_NOT_READY))
            {
                u1VrefPassEnd = pVrefInfo[u1VrefIdx].u2VrefUsed-TX_VREF_RANGE_STEP;
                if(u1TempPassNum > u1MaxVerfPassNum)
                {
                    u1VrefPassBegin_Final= u1VrefPassBegin;
                    u1VrefPassEnd_Final = u1VrefPassEnd;
                    u1MaxVerfPassNum= u1TempPassNum;
                }
                u1VrefPassBegin=0xff;
                u1VrefPassEnd=0xff;
                u1TempPassNum =0;
            }
        }
        #endif
    }

    #if LP4_TX_VREF_PASS_CONDITION
    //if((u1VrefPassBegin_Final !=LP4_TX_VREF_BOUNDARY_NOT_READY) && (u1VrefPassEnd_Final!=LP4_TX_VREF_BOUNDARY_NOT_READY))
    if(u1MaxVerfPassNum>0)
    {
        // vref pass window found
        u2FinalVref = (u1VrefPassBegin_Final + u1VrefPassEnd_Final) >>1;
        mcSHOW_DBG_MSG("[TxChooseVref] Window > %d, Vref (%d~%d), Final Vref %d\n",LP4_TX_VREF_PASS_CONDITION, u1VrefPassBegin_Final, u1VrefPassEnd_Final, u2FinalVref);
    }
    else
    #endif
    {
        // not vref found
        for(u1VrefIdx=0; u1VrefIdx < u1VrefNum; u1VrefIdx++)
        {
            if((pVrefInfo[u1VrefIdx].u2MinWinSize_byVref > u2MaxMinSize) ||
                ((pVrefInfo[u1VrefIdx].u2MinWinSize_byVref == u2MaxMinSize) && (pVrefInfo[u1VrefIdx].u2WinSum_byVref > u2MaxWinSum)))
            {
                u2MaxMinSize = pVrefInfo[u1VrefIdx].u2MinWinSize_byVref;
                u2MaxWinSum = pVrefInfo[u1VrefIdx].u2WinSum_byVref;
                u2FinalVref = pVrefInfo[u1VrefIdx].u2VrefUsed;
            }
        }

        mcSHOW_DBG_MSG("[TxChooseVref] Min win %d, Win sum %d, Final Vref %d\n", u2MaxMinSize, u2MaxWinSum, u2FinalVref);
    }

    return u2FinalVref;
}


void PsramTxGetCADelay(DRAMC_CTX_T *p, DRAM_TX_PER_BIT_CALIBRATION_TYTE_T calType,U8* pu1UILarge_CA, U8* pu1UISmall_CA, U16 *puiUITotal)
{
	U8 u1TmpValue1,u1TmpValue2;
	U16 uiTmpValue;


	if(calType == TX_DQ_DQS_MOVE_DQ_DQM)
	{

	    u1TmpValue1 = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHU_SELPH_CA3), SHU_SELPH_CA3_TXDLY_RA0);
	    u1TmpValue2 = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHU_SELPH_CA7), SHU_SELPH_CA7_DLY_RA0);
        uiTmpValue = (u1TmpValue1<<3) + u1TmpValue2; //total UI
	}
	else
	{
		uiTmpValue = u1TX_CA_PreCal_Result;
		u1TmpValue1 = uiTmpValue>>3;
		u1TmpValue2 = uiTmpValue%8;
	}

	*pu1UILarge_CA = u1TmpValue1;
	*pu1UISmall_CA = u1TmpValue2;
	*puiUITotal = uiTmpValue;
	#if 0
	mcSHOW_DBG_MSG2(("PsramTxGetCADelay (%d, %d, %d)pu1UILarge_CA: %d, pu1UISmall_CA: %d, puiUITotal: %d\n",\
					  u1TmpValue1,u1TmpValue2,uiTmpValue,*pu1UILarge_CA,*pu1UISmall_CA,*puiUITotal));
	#endif

}
void PsramTxWinTransferDelayToUIPI(DRAMC_CTX_T *p, DRAM_TX_PER_BIT_CALIBRATION_TYTE_T calType, U16 uiDelay, U8 u1AdjustPIToCenter, U8* pu1UILarge_DQ, U8* pu1UISmall_DQ, U8* pu1UILarge_DQOE, U8* pu1UISmall_DQOE,U8* pu1PI)
{
	U8 u1Small_ui_to_large, u1PI;
	U16 u2TmpValue;

	//in PSRAM, 8 small UI =	1 large UI
	// small UI is normal meaning, Large UI is MCK based

    u1Small_ui_to_large =  3;

	u1PI = uiDelay & (31);
	*pu1PI = u1PI;
	u2TmpValue = (uiDelay>>5); //total small UI

	*pu1UILarge_DQ = (u2TmpValue >> u1Small_ui_to_large); //large UI = total small UI / 8
	*pu1UISmall_DQ = u2TmpValue - ((u2TmpValue >> u1Small_ui_to_large) <<u1Small_ui_to_large); //small UI = total small UI % large UI, what is mean just offset base on large UI

	// calculate DQ  OE according to DQ UI
    u2TmpValue -= 8;

	*pu1UISmall_DQOE = u2TmpValue - ((u2TmpValue >> u1Small_ui_to_large) <<u1Small_ui_to_large);
	*pu1UILarge_DQOE = (u2TmpValue >> u1Small_ui_to_large);

}

void PSramTXSetDelayReg_DQ(DRAMC_CTX_T *p, U8 u1UpdateRegUI, U8 ucdq_ui_large, U8 ucdq_oen_ui_large, U8 ucdq_ui_small, U8 ucdq_oen_ui_small, U8 ucdql_pi)
{
	if(u1UpdateRegUI)
	{
		vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ0), \
		                     P_Fld(ucdq_ui_large, SHURK_SELPH_DQ0_TXDLY_DQ0) |
		                     P_Fld(ucdq_oen_ui_large, SHURK_SELPH_DQ0_TXDLY_OEN_DQ0));
		                     //P_Fld(ucdq_oen_ui_large[0], SHURK_SELPH_DQ0_TXDLY_OEN_CA0));
#if 0
		vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA3, P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA7)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA6)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA5)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA4)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA3)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA2)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA1)
			 | P_Fld(ucdq_ui_large[0], SHU_SELPH_CA3_TXDLY_RA0));
#endif
		// DLY_DQ[2:0]
		vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ2), \
		                     P_Fld(ucdq_ui_small, SHURK_SELPH_DQ2_DLY_DQ0) |
		                     P_Fld(ucdq_oen_ui_small, SHURK_SELPH_DQ2_DLY_OEN_DQ0));
		                     //P_Fld(ucdq_oen_ui_small[0], SHURK_SELPH_DQ2_DLY_OEN_CA0));
#if 0
		 vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA7, P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA7)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA6)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA5)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA4)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA3)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA2)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA1)
			  | P_Fld(ucdq_ui_small[0], SHU_SELPH_CA7_DLY_RA0));
#endif
	}

   // if(u1IsLP4Family(p->dram_type))
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), ucdql_pi, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
     //   vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_PI), ucdql_pi[0], SHURK_PI_RK0_ARPI_DQ_B0);
    }
}

void PSramTXSetDelayReg_CA(DRAMC_CTX_T *p, U8 u1UpdateRegUI, U16 uica_ui_total, U16 *pca_ui_delay)
{
   U8 ucca_large_ui, ucca_small_ui;
   U8 ucca_oen_small,ucca_oen_large;
   U16 uiTmp1;

   uiTmp1 = uica_ui_total;

	if(u1UpdateRegUI)
	{
	    ucca_large_ui = uiTmp1>>3; //Large UI
	    ucca_small_ui = uiTmp1%8;  //small UI

		vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA3, P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA7)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA6)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA5)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA4)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA3)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA2)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA1)
			 | P_Fld(ucca_large_ui, SHU_SELPH_CA3_TXDLY_RA0));
		 vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA7, P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA7)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA6)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA5)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA4)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA3)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA2)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA1)
			  | P_Fld(ucca_small_ui, SHU_SELPH_CA7_DLY_RA0));

	}
	*pca_ui_delay = uiTmp1;

	uiTmp1 = uiTmp1 - 3;     //CA OEN = CA - 3UI
	ucca_oen_large = uiTmp1>>3; //Large UI
	ucca_oen_small = uiTmp1%8;  //small UI


	//mcSHOW_DBG_MSG2("PSramTXSetDelayReg_CA OEN(%d ,%d,  %d, %d)\n", uiTmp1, ucca_oen_large, ucca_oen_small, *pca_ui_delay);

	vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ0), ucca_oen_large, SHURK_SELPH_DQ0_TXDLY_OEN_CA0);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ2), ucca_oen_small, SHURK_SELPH_DQ2_DLY_OEN_CA0);
   // vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), ui_pi_delay, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
}


void PSramTXSetDelayReg_DQM(DRAMC_CTX_T *p, U8 u1UpdateRegUI, U8 ucdqm_ui_large, U8 ucdqm_oen_ui_large, U8 ucdqm_ui_small, U8 ucdqm_oen_ui_small, U8 ucdqm_pi)
{
    if(u1UpdateRegUI)
    {
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ1),
                                     P_Fld(ucdqm_ui_large, SHURK_SELPH_DQ1_TXDLY_DQM0) |
                                     P_Fld(ucdqm_oen_ui_large, SHURK_SELPH_DQ1_TXDLY_OEN_DQM0));

         // DLY_DQM[2:0]
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ3),
                                     P_Fld(ucdqm_ui_small, SHURK_SELPH_DQ3_DLY_DQM0) |
                                     P_Fld(ucdqm_oen_ui_small, SHURK_SELPH_DQ3_DLY_OEN_DQM0));
    }

 //   if(u1IsLP4Family(p->dram_type))
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), ucdqm_pi, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0);
    }
}

void PsramTXSetVref(DRAMC_CTX_T *p, U8 u1VrefValue)
{
    U32 u32Mrr;
    U8 u1TempOPValue = (u1VrefValue & 0xf);
    U8 u1MRIdx = 6; //MR6

	u32Mrr = PsramcModeRegRead(p, u1MRIdx);
	mcSHOW_DBG_MSG("Current Vref Value: %x\n",u32Mrr);

	PsramcModeRegWrite(p, u1MRIdx, u1TempOPValue);
	mcSHOW_DBG_MSG("Set Vref Value: %x\n",u1TempOPValue);
}

#if PSRAM_VREF_FROM_RTN
void PsramTXSetRTNVref(DRAMC_CTX_T *p, U8 u1VrefValue)
{
	//vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x1, PLL3_RG_RPHYPLL_TSTOP_EN) | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
	vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x0, PLL3_RG_RPHYPLL_TSTOP_EN) | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
	vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, 0x1, MISC_VREF_CTRL_RG_RVREF_VREF_EN); //LP3 VREF

    vIO32WriteFldMulti_All(DDRPHY_SHU1_MISC0, P_Fld(u1VrefValue, SHU1_MISC0_RG_RVREF_SEL_CMD)
		| P_Fld(0x1, SHU1_MISC0_RG_RVREF_DDR3_SEL)
		| P_Fld(0x0, SHU1_MISC0_RG_RVREF_DDR4_SEL)
		| P_Fld(u1VrefValue, SHU1_MISC0_RG_RVREF_SEL_DQ));
}
#endif

static void PsramTXPerbitCalibrationInit(DRAMC_CTX_T *p, U8 calType)
{
	//Set TX delay chain to 0
	if(calType !=TX_DQ_DQS_MOVE_DQM_ONLY)
	{
	#if PINMUX_AUTO_TEST_PER_BIT_TX
        if(gPsram_TX_check_per_bit_flag == 1)
        {
            //not reset delay cell
        }
        else
    #endif
    	{
		vIO32Write4B(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0), 0);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ1), 0x0, SHU1_R0_B0_DQ1_RK0_TX_ARDQM0_DLY_B0);
	}
	}

	//Use HW TX tracking value
	//R_DMARPIDQ_SW :drphy_conf (0x170[7])(default set 1)
	//	 0: DQS2DQ PI setting controlled by HW
	//R_DMARUIDQ_SW : Dramc_conf(0x156[15])(default set 1)
	//	  0: DQS2DQ UI setting controlled by HW
	///TODO: need backup original setting?
	//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_CTRL1), 1, MISC_CTRL1_R_DMARPIDQ_SW);
	//vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_DQSOSCR), 1, DQSOSCR_ARUIDQ_SW);

}

static void PsramTXScanRange_Vref(DRAMC_CTX_T *p, U8 u1VrefScanEnable, U16 *pu2Begin, U16 *pu2End, U16 *pu2Setp)
{
	U16 u2VrefBegin, u2FinalRange, u2VrefEnd;

	if(u1VrefScanEnable)
	{
#if (SUPPORT_SAVE_TIME_FOR_CALIBRATION && BYPASS_VREF_CAL)
		if(p->femmc_Ready==1)
		{
			// if fast K, use TX Vref that saved.
			u2VrefBegin = p->pSavetimeData->u1TxWindowPerbitVref_Save[p->channel][p->rank];
			u2VrefEnd = u2VrefBegin+1;
		}
		else
#endif
		{

				//
				u2VrefBegin = 10;   // 0000   60%
				u2VrefEnd = 12;    //1110    38.44%
		}
	}
	else //LPDDR3, the for loop will only excute u2VrefLevel=TX_VREF_RANGE_END/2.
	{
		u2VrefBegin = 0;
		u2VrefEnd = 0;
	}

	*pu2Begin = u2VrefBegin;
	*pu2End = u2VrefEnd;
	*pu2Setp = 2;

}

DRAM_STATUS_T PSramcTxWindowPerbitCal(DRAMC_CTX_T *p, DRAM_TX_PER_BIT_CALIBRATION_TYTE_T calType, U8 u1VrefScanEnable)
{
	U8 u1BitTemp, u1BitIdx, u1ByteIdx, u1RankIdx;
	U32 uiFinishCount;
	PASS_WIN_DATA_T WinPerBit[8] = {0,0,0,0,0}, VrefWinPerBit[8] = {0,0,0,0,0}, FinalWinPerBit[8] = {0,0,0,0,0};

	U16 uiDelay, u2DQDelayBegin, u2DQDelayEnd;
	INT8 iCAchange;
	U8 ucdq_pi, ucdq_ui_small, ucdq_ui_large,ucdq_oen_ui_small, ucdq_oen_ui_large;
	U8 ucdq_ui_small_reg_value, u1UpdateRegUI;	// for UI and TXDLY change check, if different , set reg.
	U8 ucca_ui_small_reg_value,ucca_ui_Large_reg_value;
	U16 uica_ui_total,uiCa_pass_ui_delay;
	PASS_WIN_DATA_T WinuiCa_pass_ui_delay[8] ={0,0,0,0,0};

	U8 ucdq_reg_pi, ucdq_reg_ui_large, ucdq_reg_ui_small;
	U8 ucdq_reg_oen_ui_large, ucdq_reg_oen_ui_small;
	U8 ucdq_reg_dqm_pi, ucdq_reg_dqm_ui_large, ucdq_reg_dqm_ui_small;
	U8 ucdq_reg_dqm_oen_ui_large, ucdq_reg_dqm_oen_ui_small;

#if 1//TX_DQM_CALC_MAX_MIN_CENTER
	U16 u2Center_min = 0xffff,u2Center_max = 0;
#endif
	U8 u1EnableDelayCell=0, u1DelayCellOfst[8]={0};
	U32 u4err_value, u4fail_bit;
	U16 u2FinalRange=0, u2FinalVref=0xd;
	U16 u2VrefLevel, u2VrefBegin, u2VrefEnd, u2VrefStep;
	U16 u2TempWinSum, u2MaxWindowSum=0;//, u2tx_window_sum[LP4_TX_VREF_DATA_NUM]={0};
	U32 u4TempRegValue;
	U8 u1min_bit, u1min_winsize=0;
	U8 u1VrefIdx =0;
	U8 u1PIDiff;
	PASS_WIN_DATA_BY_VREF_T VrefInfo[15];
#if PSRAM_TX_EYE
	U8 ii, u1vrefidx;
	U8 EyeScan_index[DQ_DATA_WIDTH_PSRAM];
#endif
    U32 tx_perbit_win_min_max = 0, tx_perbit_win_min_max_idx = 0;

	if (!p)
	{
		mcSHOW_ERR_MSG("context NULL\n");
		return DRAM_FAIL;
	}

#if VENDER_JV_LOG
	if((calType ==TX_DQ_DQS_MOVE_DQ_ONLY && u1IsLP4Family(p->dram_type)) || (calType ==TX_DQ_DQS_MOVE_DQ_DQM && !u1IsLP4Family(p->dram_type)))
		vPrintCalibrationBasicInfo_ForJV(p);
#else
		vPrintCalibrationBasicInfo(p);
#endif

#if PSRAM_TX_EYE
    //set initial values
    for(u1vrefidx=0; u1vrefidx<VREF_TOTAL_NUM_WITH_RANGE;u1vrefidx++)
    {
        for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
        {
            for(ii=0; ii<EYESCAN_BROKEN_NUM; ii++)
            {
                gEyeScan_Min[u1vrefidx][u1BitIdx][ii] = EYESCAN_DATA_INVALID;
                gEyeScan_Max[u1vrefidx][u1BitIdx][ii] = EYESCAN_DATA_INVALID;
            }
            gEyeScan_ContinueVrefHeight[u1BitIdx] = 0;
            gEyeScan_TotalPassCount[u1BitIdx] = 0;
        }
    }
#endif

	PsramTXPerbitCalibrationInit(p, calType);
	PSramTXScanRange_PI(p, calType, &u2DQDelayBegin, &u2DQDelayEnd);
	PsramTXScanRange_Vref(p, u1VrefScanEnable, &u2VrefBegin, &u2VrefEnd, &u2VrefStep);
	//mcSHOW_DBG_MSG("calType = %d\n",calType);

#if PSRAM_TX_EYE
	if (u1VrefScanEnable)
	{
#if PSRAM_VREF_FROM_RTN
		u2VrefBegin = 0;
		u2VrefEnd = 31;
		u2VrefStep = 1;
#else
		u2VrefBegin = 0;
		u2VrefEnd = 0xf;
		u2VrefStep = 1;
#endif
	}
#endif

	vSetCalibrationResult(p, DRAM_CALIBRATION_TX_PERBIT, DRAM_FAIL);

#if 1
	mcSHOW_DBG_MSG("[PsramTxWindowPerbitCal] calType=%d, VrefScanEnable %d (VrefBegin %d, u2VrefEnd %d)\n"
					"\nBegin, DQ Scan Range %d~%d\n",
					calType, u1VrefScanEnable, u2VrefBegin, u2VrefEnd, u2DQDelayBegin, u2DQDelayEnd);

	mcFPRINTF(fp_A60501, "[PsramTxWindowPerbitCal] calType=%d, VrefScanEnable %d\n"
					"\nBegin, DQ Scan Range %d~%d\n",
					calType, u1VrefScanEnable, u2DQDelayBegin, u2DQDelayEnd);
#endif

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
	if(p->femmc_Ready==1 && (p->Bypass_TXWINDOW))
	{
		for (u1ByteIdx=0; u1ByteIdx<(p->data_width/DQS_BIT_NUMBER); u1ByteIdx++)
		{
			u2Center_min=p->pSavetimeData->u1TxCenter_min_Save[p->channel][p->rank][u1ByteIdx];
			u2Center_max=p->pSavetimeData->u1TxCenter_max_Save[p->channel][p->rank][u1ByteIdx];

			for (u1BitIdx=0; u1BitIdx<DQS_BIT_NUMBER; u1BitIdx++)
			{
				u1BitTemp = u1ByteIdx*DQS_BIT_NUMBER+u1BitIdx;
				FinalWinPerBit[u1BitTemp].win_center= p->pSavetimeData->u1Txwin_center_Save[p->channel][p->rank][u1BitTemp];
			}
		}

		vSetCalibrationResult(p, DRAM_CALIBRATION_TX_PERBIT, DRAM_OK);
	}
	else
#endif
	{
		PSramcEngine2Init(p, 0, 0x23, TEST_AUDIO_PATTERN, 0);

		for(u2VrefLevel = u2VrefBegin; u2VrefLevel <= u2VrefEnd; u2VrefLevel += u2VrefStep)
		{
			// SET tx Vref (DQ) here, LP3 no need to set this.
			if(u1VrefScanEnable)
			{

        #if VENDER_JV_LOG
				if(calType ==TX_DQ_DQS_MOVE_DQ_ONLY)
				{
					mcSHOW_JV_LOG_MSG("\n\n\tLP4 TX VrefRange %d, VrefLevel=%d\n", u2FinalRange, u2VrefLevel);
				}
        #endif
#if PSRAM_VREF_FROM_RTN
				PsramTXSetRTNVref(p, u2VrefLevel);
#else
				PsramTXSetVref(p, u2VrefLevel);
#endif
			}
			else
			{
				mcSHOW_DBG_MSG("\n\n\tTX Vref Scan disable\n");
				mcFPRINTF(fp_A60501, "\n\n\tTX Vref Scan disable\n");
			}

			// initialize parameters
			uiFinishCount = 0;
			u2TempWinSum =0;
			//ucdq_ui_small_reg_value = 0xff;
			ucdq_ui_small_reg_value = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SHURK_SELPH_DQ2), SHURK_SELPH_DQ2_DLY_DQ0);

			PsramTxGetCADelay(p,calType,&ucca_ui_Large_reg_value,&ucca_ui_small_reg_value,&uica_ui_total);

			for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
			{
				WinPerBit[u1BitIdx].first_pass = (S16)PASS_RANGE_NA;
				WinPerBit[u1BitIdx].last_pass = (S16)PASS_RANGE_NA;
				VrefWinPerBit[u1BitIdx].first_pass = (S16)PASS_RANGE_NA;
				VrefWinPerBit[u1BitIdx].last_pass = (S16)PASS_RANGE_NA;
#if (PSRAM_TX_EYE == 1)
				gEyeScan_DelayCellPI[u1BitIdx] = 0;
	            EyeScan_index[u1BitIdx] = 0;
#endif
			}

			//Move DQ delay ,  1 PI = tCK/64, total 128 PI, 1UI = 32 PI
			//For data rate 3200, max tDQS2DQ is 2.56UI (82 PI)
			//For data rate 4266, max tDQS2DQ is 3.41UI (109 PI)
			for (uiDelay = u2DQDelayBegin; uiDelay <u2DQDelayEnd; uiDelay = uiDelay+1)
			{
				PsramTxWinTransferDelayToUIPI(p, calType, uiDelay, 0, &ucdq_ui_large, &ucdq_ui_small, &ucdq_oen_ui_large, &ucdq_oen_ui_small,&ucdq_pi);

				// Check if TX UI changed, if not change , don't need to set reg again
				//UpdateRegUI: 0-->no change current UI, 1-->increase current UI, 2-->decrease current UI
#if 1
				iCAchange = (ucdq_ui_small - ucdq_ui_small_reg_value);
				uica_ui_total += iCAchange;
				ucdq_ui_small_reg_value = ucdq_ui_small;
				u1UpdateRegUI = 1;
#endif

#if 0
				if(ucdq_ui_small_reg_value > ucdq_ui_small)
				{
					u1UpdateRegUI=2;
					uica_ui_total -= (ucdq_ui_small_reg_value - ucdq_ui_small);
					ucdq_ui_small_reg_value = ucdq_ui_small;
				}
				else if(ucdq_ui_small_reg_value < ucdq_ui_small)
				{
					u1UpdateRegUI=1;
					uica_ui_total += (ucdq_ui_small - ucdq_ui_small_reg_value);
					ucdq_ui_small_reg_value = ucdq_ui_small;
				}
				else
					u1UpdateRegUI=0;

#endif
				//for(u1ByteIdx=0; u1ByteIdx < 1; u1ByteIdx++) //psram only one dqs

				if(u1UpdateRegUI)
				{
					ucdq_reg_ui_large = ucdq_ui_large;
					ucdq_reg_ui_small = ucdq_ui_small;
					ucdq_reg_oen_ui_large = ucdq_oen_ui_large;
					ucdq_reg_oen_ui_small = ucdq_oen_ui_small;

					ucdq_reg_dqm_ui_large = ucdq_ui_large;
					ucdq_reg_dqm_ui_small = ucdq_ui_small;
					ucdq_reg_dqm_oen_ui_large = ucdq_oen_ui_large;
					ucdq_reg_dqm_oen_ui_small = ucdq_oen_ui_small;
				}

				ucdq_reg_pi = ucdq_pi;
				ucdq_reg_dqm_pi = ucdq_pi;

			#if 0
				mcSHOW_DBG_MSG2(("TX u1UpdateRegUI: %d, uiDelay: %d, ucdq_ui_large: %d, ucdq_ui_small: %d, ucdq_pi: %d, ucdq_oen_ui_large: %d, ucdq_oen_ui_small: %d\n", \
					u1UpdateRegUI,uiDelay,ucdq_ui_large,ucdq_ui_small,ucdq_pi,ucdq_oen_ui_large,ucdq_oen_ui_small));
				mcSHOW_DBG_MSG2(("TX ucca_ui_Large_reg_value: %d, ucca_ui_small_reg_value: %d, uica_ui_total: %d\n",\
								  ucca_ui_Large_reg_value,ucca_ui_small_reg_value,uica_ui_total));
			#endif
				if(calType ==TX_DQ_DQS_MOVE_DQ_ONLY || calType== TX_DQ_DQS_MOVE_DQ_DQM)
				{
					PSramTXSetDelayReg_DQ(p, u1UpdateRegUI, ucdq_reg_ui_large, ucdq_reg_oen_ui_large, ucdq_reg_ui_small, ucdq_reg_oen_ui_small, ucdq_reg_pi);
				}

				if(calType ==TX_DQ_DQS_MOVE_DQM_ONLY || calType== TX_DQ_DQS_MOVE_DQ_DQM)
				{

					PSramTXSetDelayReg_DQM(p, u1UpdateRegUI, ucdq_reg_dqm_ui_large, ucdq_reg_dqm_oen_ui_large, ucdq_reg_dqm_ui_small, ucdq_reg_dqm_oen_ui_small, ucdq_reg_dqm_pi);
				}
				PSramTXSetDelayReg_CA(p,u1UpdateRegUI,uica_ui_total,&uiCa_pass_ui_delay);

				// audio +xtalk pattern
				//u4err_value=0;
				PSramcEngine2SetPat(p,TEST_AUDIO_PATTERN, 0,0);
				u4err_value = PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_AUDIO_PATTERN);
				PSramcEngine2SetPat(p,TEST_XTALK_PATTERN, 0,0);
				u4err_value |= PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_XTALK_PATTERN);

				//if(u1VrefScanEnable==0 && (calType != TX_DQ_DQS_MOVE_DQM_ONLY))
				{
					//mcSHOW_DBG_MSG("Delay=%3d |%2d %2d %3d| %2d %2d| 0x%8x [0]",uiDelay, ucdq_ui_large,ucdq_ui_small, ucdq_pi, ucdq_oen_ui_large,ucdq_oen_ui_small, u4err_value);
					mcSHOW_DBG_MSG2("Delay=%3d | %2d %2d | %2d %2d %2d | 0x%x [LSB]", uiDelay, uica_ui_total>>3,uica_ui_total%8,ucdq_ui_large, ucdq_ui_small, ucdq_pi, u4err_value);
					mcFPRINTF(fp_A60501, "Delay=%3d | %2d %2d %3d| 0x%8x [LSB]", uiDelay, ucdq_ui_large,ucdq_ui_small, ucdq_pi, u4err_value);
				}

				// check fail bit ,0 ok ,others fail
				for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
				{
					u4fail_bit = u4err_value&((U32)1<<u1BitIdx);

					//if(u1VrefScanEnable==0 && (calType != TX_DQ_DQS_MOVE_DQM_ONLY))
					{

							if(u1BitIdx%DQS_BIT_NUMBER ==0)
							{
								mcSHOW_DBG_MSG2(" ");
								mcFPRINTF(fp_A60501, " ");
							}

							if (u4fail_bit == 0)
							{
								mcSHOW_DBG_MSG2("o");
								mcFPRINTF(fp_A60501, "o");
							}
							else
							{
								mcSHOW_DBG_MSG2("x");
								mcFPRINTF(fp_A60501, "x");
							}

					}

					if(WinPerBit[u1BitIdx].first_pass== PASS_RANGE_NA)
					{
						if(u4fail_bit==0) //compare correct: pass
						{
							WinPerBit[u1BitIdx].first_pass = uiDelay;
							WinuiCa_pass_ui_delay[u1BitIdx].first_pass = uiCa_pass_ui_delay;
                    #if 0//TX_TDQS2DQ_PRE_CAL
							if((p->frequency == LP4_LOWEST_FREQ) && (u2DQS2DQ_Pre_Cal[p->channel][p->rank]==0))
							{
								u2DQS2DQ_Pre_Cal[p->channel][p->rank] = (uiDelay - u2DQDelayBegin) *850 / p->frequency;
							}

							if(uiDelay==u2DQDelayBegin)
							{
								mcSHOW_ERR_MSG("TX_TDQS2DQ_PRE_CAL: Warning, possible miss TX window boundary\n");

								u2DQDelayBegin -= 5;

								uiDelay = u2DQDelayBegin;

								break;

                        #if 0//__ETT__
								while(1);
                        #endif
							}
                    #endif
						}
					}
					else if(WinPerBit[u1BitIdx].last_pass == PASS_RANGE_NA)
					{
						if(u4fail_bit !=0) //compare error : fail
						{
							WinPerBit[u1BitIdx].last_pass  = (uiDelay-1);
							WinuiCa_pass_ui_delay[u1BitIdx].last_pass = uiCa_pass_ui_delay;
						}
						else if (uiDelay==(u2DQDelayEnd-1))
						{
							WinPerBit[u1BitIdx].last_pass  = uiDelay;
							WinuiCa_pass_ui_delay[u1BitIdx].last_pass = uiCa_pass_ui_delay;
						}

						if(WinPerBit[u1BitIdx].last_pass  !=PASS_RANGE_NA)
						{
							if((WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass) >= (VrefWinPerBit[u1BitIdx].last_pass -VrefWinPerBit[u1BitIdx].first_pass))
							{
								if((VrefWinPerBit[u1BitIdx].last_pass != PASS_RANGE_NA) && (VrefWinPerBit[u1BitIdx].last_pass -VrefWinPerBit[u1BitIdx].first_pass)>0)
								{
									mcSHOW_DBG_MSG2("Bit[%d] Bigger window update %d > %d, window broken?\n", u1BitIdx, \
										(WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass), (VrefWinPerBit[u1BitIdx].last_pass -VrefWinPerBit[u1BitIdx].first_pass));
									mcFPRINTF(fp_A60501,"Bit[%d] Bigger window update %d > %d\n", u1BitIdx, \
										(WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass), (VrefWinPerBit[u1BitIdx].last_pass -VrefWinPerBit[u1BitIdx].first_pass));

								}

								//if window size bigger than 7, consider as real pass window. If not, don't update finish counte and won't do early break;
								if((WinPerBit[u1BitIdx].last_pass -WinPerBit[u1BitIdx].first_pass) >7)
									uiFinishCount |= (1<<u1BitIdx);

								//update bigger window size
								VrefWinPerBit[u1BitIdx].first_pass = WinPerBit[u1BitIdx].first_pass;
								VrefWinPerBit[u1BitIdx].last_pass = WinPerBit[u1BitIdx].last_pass;
							}

#if PSRAM_TX_EYE
							if (EyeScan_index[u1BitIdx] < EYESCAN_BROKEN_NUM)
							{
								gEyeScan_Min[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]] = WinPerBit[u1BitIdx].first_pass;
								gEyeScan_Max[u2VrefLevel][u1BitIdx][EyeScan_index[u1BitIdx]] = WinPerBit[u1BitIdx].last_pass;
								EyeScan_index[u1BitIdx]=EyeScan_index[u1BitIdx]+1;
							}
#endif

							//reset tmp window
							WinPerBit[u1BitIdx].first_pass = PASS_RANGE_NA;
							WinPerBit[u1BitIdx].last_pass = PASS_RANGE_NA;
						}
					 }
				}

				//if(u1VrefScanEnable==0 && (calType != TX_DQ_DQS_MOVE_DQM_ONLY))
				{
					//if(u4err_value != 0)

						mcSHOW_DBG_MSG2(" [MSB]\n");
						mcFPRINTF(fp_A60501, " [MSB]\n");

				}

				//if all bits widnow found and all bits turns to fail again, early break;
				if(uiFinishCount == 0xff)
				{
					vSetCalibrationResult(p, DRAM_CALIBRATION_TX_PERBIT, DRAM_OK);

            #if !REDUCE_LOG_FOR_PRELOADER
            #ifdef ETT_PRINT_FORMAT
					mcSHOW_DBG_MSG2("TX calibration finding left boundary early break. PI DQ delay=0x%B\n", uiDelay);
            #else
					mcSHOW_DBG_MSG2("TX calibration finding left boundary early break. PI DQ delay=0x%2x\n", uiDelay);
            #endif
            #endif
					break;	//early break
				}
				else  //no pass TX window found
				{
					if(uiDelay == (u2DQDelayEnd-1))
					{
						///TODO: When Tx not found pass window, maybe should check RX/DATLAT or waveform?
						mcSHOW_DBG_MSG2("No TX pass window found!!!\n");
					}
				}
			}


			// (1) calculate per bit window size
			// (2) find out min win of all DQ bits
			// (3) calculate perbit window center
			u1min_winsize = 0xff;
			u1min_bit = 0xff;
			for (u1BitIdx = 0; u1BitIdx < p->data_width; u1BitIdx++)
			{
				//if(VrefWinPerBit[u1BitIdx].last_pass == VrefWinPerBit[u1BitIdx].first_pass)
				if(VrefWinPerBit[u1BitIdx].first_pass == PASS_RANGE_NA)
					VrefWinPerBit[u1BitIdx].win_size = 0;
				else
					VrefWinPerBit[u1BitIdx].win_size= VrefWinPerBit[u1BitIdx].last_pass- VrefWinPerBit[u1BitIdx].first_pass +1;

				if (VrefWinPerBit[u1BitIdx].win_size < u1min_winsize)
				{
					u1min_bit = u1BitIdx;
					u1min_winsize = VrefWinPerBit[u1BitIdx].win_size;
				}

				u2TempWinSum += VrefWinPerBit[u1BitIdx].win_size;  //Sum of CA Windows for vref selection

#if PSRAM_TX_EYE
				gEyeScan_WinSize[u2VrefLevel][u1BitIdx] = VrefWinPerBit[u1BitIdx].win_size;
#endif

        #if VENDER_JV_LOG
				if((calType ==TX_DQ_DQS_MOVE_DQ_ONLY && u1IsLP4Family(p->dram_type)) || (calType ==TX_DQ_DQS_MOVE_DQ_DQM && !u1IsLP4Family(p->dram_type)))
				{
					mcSHOW_JV_LOG_MSG("TX Bit%d, %d%%\n", u1BitIdx,  (VrefWinPerBit[u1BitIdx].win_size*100+31)/32);
				}
        #endif


				// calculate per bit window position and print
				VrefWinPerBit[u1BitIdx].win_center= (VrefWinPerBit[u1BitIdx].first_pass + VrefWinPerBit[u1BitIdx].last_pass) >> 1;
		#if PINMUX_AUTO_TEST_PER_BIT_TX
                gPsramFinalTXPerbitFirstPass[p->channel][u1BitIdx] = VrefWinPerBit[u1BitIdx].first_pass;
		#endif
			}

			uiCa_pass_ui_delay= (WinuiCa_pass_ui_delay[u1min_bit].first_pass + WinuiCa_pass_ui_delay[u1min_bit].last_pass)>>1;

			mcSHOW_DBG_MSG3("Min Bit=%d, winsize=%d\n",u1min_bit, u1min_winsize);
    #if 1//__ETT__
			if(u1VrefScanEnable==0)
			{
				//mcSHOW_DBG_MSG("\n\tCH=%d, VrefRange= %d, VrefLevel = %d\n", p->channel, u2FinalRange, u2VrefLevel);
				//mcFPRINTF(fp_A60501,"\n\tchannel=%d(2:cha, 3:chb)  u2VrefLevel = %d\n", p->channel, u2VrefLevel);
				TxPrintWidnowInfo(p, VrefWinPerBit);
			}
    #endif

			if(u1VrefScanEnable==1)
			{
				if(u2TempWinSum > u2MaxWindowSum)
					u2MaxWindowSum= u2TempWinSum;

				VrefInfo[u1VrefIdx].u2VrefUsed = u2VrefLevel;
				VrefInfo[u1VrefIdx].u2MinWinSize_byVref= u1min_winsize;
				VrefInfo[u1VrefIdx].u2WinSum_byVref = u2TempWinSum;
				u1VrefIdx ++;
			}

			if (u1min_winsize > tx_perbit_win_min_max) {
                tx_perbit_win_min_max = u1min_winsize;
                tx_perbit_win_min_max_idx = u1min_bit;
             }

#if (PSRAM_TX_EYE == 0)
    #if LP4_TX_VREF_PASS_CONDITION
			if(u1VrefScanEnable && (u2TempWinSum < (u2MaxWindowSum*95/100)) &&(u1min_winsize < LP4_TX_VREF_PASS_CONDITION))
    #else
			if(u1VrefScanEnable && (u2TempWinSum < (u2MaxWindowSum*95/100)))
    #endif
			{
				mcSHOW_DBG_MSG("\nTX Vref early break, caculate TX vref\n");
				break;
			}
#endif
		}

		PSramcEngine2End(p);

		if(u1VrefScanEnable==0)// ..if time domain (not vref scan) , calculate window center of all bits.
		{
			// Calculate the center of DQ pass window
			// Record center sum of each byte
        #if 1//TX_DQM_CALC_MAX_MIN_CENTER
				u2Center_min = 0xffff;
				u2Center_max = 0;
        #endif

				for (u1BitIdx=0; u1BitIdx<DQS_BIT_NUMBER; u1BitIdx++)
				{
					u1BitTemp = u1BitIdx;
					memcpy(FinalWinPerBit, VrefWinPerBit, sizeof(PASS_WIN_DATA_T)*8);

					if(FinalWinPerBit[u1BitTemp].win_center < u2Center_min)
						u2Center_min = FinalWinPerBit[u1BitTemp].win_center;

					if(FinalWinPerBit[u1BitTemp].win_center > u2Center_max)
						u2Center_max = FinalWinPerBit[u1BitTemp].win_center;

            #ifdef FOR_HQA_TEST_USED
					if((u1IsLP4Family(p->dram_type) && calType == TX_DQ_DQS_MOVE_DQ_ONLY && (u1VrefScanEnable==0))
						|| (!u1IsLP4Family(p->dram_type) && (calType == TX_DQ_DQS_MOVE_DQ_DQM)))
					{
						gFinalTXPerbitWin[p->channel][p->rank][u1BitTemp] = FinalWinPerBit[u1BitTemp].win_size;
					}
            #endif
				}

    #if SUPPORT_SAVE_TIME_FOR_CALIBRATION
			if(p->femmc_Ready==0)//save firtst run pass value
			{
				for (u1ByteIdx=0; u1ByteIdx<(p->data_width/DQS_BIT_NUMBER); u1ByteIdx++)
				{
					if(calType == TX_DQ_DQS_MOVE_DQ_ONLY) // && u1VrefScanEnable==0
					{
						p->pSavetimeData->u1TxCenter_min_Save[p->channel][p->rank][u1ByteIdx]=u2Center_min;
						p->pSavetimeData->u1TxCenter_max_Save[p->channel][p->rank][u1ByteIdx]=u2Center_max;

						for (u1BitIdx=0; u1BitIdx<DQS_BIT_NUMBER; u1BitIdx++)
						{
							u1BitTemp = u1ByteIdx*DQS_BIT_NUMBER+u1BitIdx;
							p->pSavetimeData->u1Txwin_center_Save[p->channel][p->rank][u1BitTemp]=FinalWinPerBit[u1BitTemp].win_center;
						}
					}
				}
			}
    #endif
		}
	}

#ifdef DRAM_SLT
	int bit_idx;
	mcSHOW_INFO_MSG("\n");
	for (bit_idx = 0; bit_idx < DQS_BIT_NUMBER; bit_idx++) {
#ifdef DRAM_SLT
	mcSHOW_INFO_MSG("[Eye Check][%d][TX] Bit%d (%d~%d) %d %d,	Bit%d (%d~%d) %d %d,\n", \
		p->frequency * 2, bit_idx, VrefWinPerBit[bit_idx].first_pass, VrefWinPerBit[bit_idx].last_pass, \
		VrefWinPerBit[bit_idx].win_size, VrefWinPerBit[bit_idx].win_center, bit_idx+8, VrefWinPerBit[bit_idx+8].first_pass, \
		VrefWinPerBit[bit_idx+8].last_pass, VrefWinPerBit[bit_idx+8].win_size, VrefWinPerBit[bit_idx+8].win_center);
#endif
	}
#ifdef DRAM_SLT
	mcSHOW_INFO_MSG("\n[Eye Check][%d][TX] Best Vref %d, Window Min %d at DQ%d\n\n",\
		p->frequency*2,\
		u2FinalVref, tx_perbit_win_min_max, tx_perbit_win_min_max_idx);
#endif
	if (tx_perbit_win_min_max < TX_WIN_CRITERIA) {
		mcSHOW_ERR_MSG("[Eye Check]TX margin fail@DQ%d, %dPI<%dPI\n",
			tx_perbit_win_min_max_idx, tx_perbit_win_min_max, TX_WIN_CRITERIA);
#ifdef DRAM_SLT
		dram_slt_set(p, DRAM_CALIBRATION_TX_PERBIT, DRAM_FAIL);
#endif
	}
#endif

	// SET tx Vref (DQ) = u2FinalVref, LP3 no need to set this.
	if(u1VrefScanEnable)
	{
#if SUPPORT_SAVE_TIME_FOR_CALIBRATION && BYPASS_VREF_CAL
		if(p->femmc_Ready==1 && (p->Bypass_TXWINDOW))
		{
			u2FinalVref = p->pSavetimeData->u1TxWindowPerbitVref_Save[p->channel][p->rank];
		}
		else
#endif
		{
			u2FinalVref = TxChooseVref(p, VrefInfo, u1VrefIdx);
    #if SUPPORT_SAVE_TIME_FOR_CALIBRATION
			if(p->femmc_Ready==0)////save firtst run Vref value
			{
				p->pSavetimeData->u1TxWindowPerbitVref_Save[p->channel][p->rank]=u2FinalVref;
			}
    #endif
		}

#if PSRAM_VREF_FROM_RTN
		PsramTXSetRTNVref(p, u2FinalVref);
#else
		PsramTXSetVref(p, u2FinalVref);
#endif

#if (PSRAM_TX_EYE == 0)
		return DRAM_OK;
#endif
	}


	// LP3 only use "TX_DQ_DQS_MOVE_DQ_DQM" scan
	// first freq 800(LP4-1600) doesn't support jitter meter(data < 1T), therefore, don't use delay cell
	if((calType == TX_DQ_DQS_MOVE_DQ_ONLY) && (p->frequency >= 800) && (p->u2DelayCellTimex100!=0))
	{
		u1EnableDelayCell =1;
		mcSHOW_DBG_MSG("[TX_PER_BIT_DELAY_CELL] DelayCellTimex100 =%d/100 ps\n", p->u2DelayCellTimex100);
	}
	else
		u1EnableDelayCell =0;

	//Calculate the center of DQ pass window
	//average the center delay
		mcSHOW_DBG_MSG(" == TX Byte 0 ==\n");
		if(u1EnableDelayCell==0)
		{
			uiDelay = ((u2Center_min + u2Center_max)>>1); //(max +min)/2
			u2TX_DQ_PreCal_PSRAM = uiDelay;  //LP4 only, for tDQS2DQ
			u1TX_CA_PreCal_Result = uiCa_pass_ui_delay;
		}
		else // if(calType == TX_DQ_DQS_MOVE_DQ_ONLY)
		{
			uiDelay = u2Center_min; // for DQ PI delay , will adjust with delay cell
			u2TX_DQ_PreCal_PSRAM = ((u2Center_min + u2Center_max)>>1);	// for DQM PI delay

#if PSRAM_TX_EYE
		gEyeScan_CaliDelay[u1ByteIdx] = uiDelay ;
#endif

    #if 0//SUPPORT_SAVE_TIME_FOR_CALIBRATION//BYPASS_TX_PER_BIT_DELAY_CELL
			if(p->femmc_Ready==1 && (p->Bypass_TXWINDOW))
			{
				for (u1BitIdx = 0; u1BitIdx < DQS_BIT_NUMBER; u1BitIdx++)
				{
					u1BitTemp = u1ByteIdx*DQS_BIT_NUMBER+u1BitIdx;
					u1DelayCellOfst[u1BitTemp] = p->pSavetimeData->u1TX_PerBit_DelayLine_Save[p->channel][p->rank][u1BitTemp];
				}
			}
			else
    #endif
			{
				// calculate delay cell perbit
				for (u1BitIdx = 0; u1BitIdx < DQS_BIT_NUMBER; u1BitIdx++)
				{
					u1BitTemp = u1BitIdx;
					u1PIDiff = FinalWinPerBit[u1BitTemp].win_center - u2Center_min;
					if(p->u2DelayCellTimex100 !=0)
					{
						u1DelayCellOfst[u1BitTemp] = (u1PIDiff*100000000/(p->frequency<<6))/p->u2DelayCellTimex100;
						if (u1DelayCellOfst[u1BitTemp] > 15)
                        {
                            mcSHOW_DBG_MSG("[warning] Tx DQ%d delay cell %d > 15 \n", u1BitTemp, u1DelayCellOfst[u1BitTemp]);
                            u1DelayCellOfst[u1BitTemp] = 15;
                        }
                #if 0//SUPPORT_SAVE_TIME_FOR_CALIBRATION
						p->pSavetimeData->u1TX_PerBit_DelayLine_Save[p->channel][p->rank][u1BitTemp] = u1DelayCellOfst[u1BitTemp];
                #endif
						mcSHOW_DBG_MSG("u1DelayCellOfst[%d]=%d cells (%d PI)\n",  u1BitTemp, u1DelayCellOfst[u1BitTemp], u1PIDiff);
					}
					else
					{
						mcSHOW_ERR_MSG("Error: Cell time (p->u2DelayCellTimex100) is 0 \n");
						break;
					}
				}
			}
		}

	   PsramTxWinTransferDelayToUIPI(p, calType, uiDelay, 1, &ucdq_reg_ui_large, &ucdq_reg_ui_small, \
								&ucdq_reg_oen_ui_large, &ucdq_reg_oen_ui_small,&ucdq_reg_pi);
	   PsramTxWinTransferDelayToUIPI(p, calType, u2TX_DQ_PreCal_PSRAM, 1, &ucdq_reg_dqm_ui_large, &ucdq_reg_dqm_ui_small,\
								&ucdq_reg_dqm_oen_ui_large, &ucdq_reg_dqm_oen_ui_small, &ucdq_reg_dqm_pi);
		if(calType ==TX_DQ_DQS_MOVE_DQ_ONLY || calType== TX_DQ_DQS_MOVE_DQ_DQM)
		{

			mcSHOW_DBG_MSG("Update CA	dly = (%d, %d)	CA	OEN =(%d ,%d)\n",
							uiCa_pass_ui_delay>>3, uiCa_pass_ui_delay&0x7, (uiCa_pass_ui_delay-3)>>3, (uiCa_pass_ui_delay-3)&0x7);

			mcSHOW_DBG_MSG("Update DQ	dly =%d (%d ,%d, %d)  DQ  OEN =(%d ,%d)\n",
							uiDelay, ucdq_reg_ui_large, ucdq_reg_ui_small, ucdq_reg_pi,\
							ucdq_reg_oen_ui_large, ucdq_reg_oen_ui_small);

			mcFPRINTF(fp_A60501,"Byte%d, PI DQ dly %d\n",	u1ByteIdx, uiDelay);
			mcFPRINTF(fp_A60501,"Final DQ PI dly(LargeUI, SmallUI, PI) =(%d ,%d, %d)\n", ucdq_reg_ui_large, ucdq_reg_ui_small, ucdq_reg_pi);
			mcFPRINTF(fp_A60501,"OEN DQ PI dly(LargeUI, SmallUI, PI) =(%d ,%d, %d)\n\n", ucdq_reg_oen_ui_large, ucdq_reg_oen_ui_small, ucdq_reg_pi);
		}

		//if(calType ==TX_DQ_DQS_MOVE_DQM_ONLY || calType== TX_DQ_DQS_MOVE_DQ_DQM)
		{
			mcSHOW_DBG_MSG("Update DQM dly =%d (%d ,%d, %d)  DQM OEN =(%d ,%d)\n",
					u2TX_DQ_PreCal_PSRAM, ucdq_reg_dqm_ui_large, ucdq_reg_dqm_ui_small, ucdq_reg_dqm_pi, \
					ucdq_reg_dqm_oen_ui_large, ucdq_reg_dqm_oen_ui_small);
		}


#if REG_ACCESS_PORTING_DGB
	RegLogEnable =1;
#endif

		/* p->rank = RANK_0, save to Reg Rank0 and Rank1, p->rank = RANK_1, save to Reg Rank1 */
		//for(u1RankIdx=p->rank; u1RankIdx<1; u1RankIdx++)
		{
			//vSetRank(p,u1RankIdx);

			if(calType ==TX_DQ_DQS_MOVE_DQ_ONLY || calType== TX_DQ_DQS_MOVE_DQ_DQM)
			{
				PSramTXSetDelayReg_DQ(p, TRUE, ucdq_reg_ui_large, ucdq_reg_oen_ui_large, ucdq_reg_ui_small, ucdq_reg_oen_ui_small, ucdq_reg_pi);
			}

			PSramTXSetDelayReg_DQM(p, TRUE, ucdq_reg_dqm_ui_large, ucdq_reg_dqm_oen_ui_large, ucdq_reg_dqm_ui_small, ucdq_reg_dqm_oen_ui_small, ucdq_reg_dqm_pi);

			PSramTXSetDelayReg_CA(p,TRUE,uiCa_pass_ui_delay,&uiCa_pass_ui_delay);

			 //if(u1IsLP4Family(p->dram_type))
			 {
				 if(u1EnableDelayCell)
				 {
					 vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ0),
							   P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][7]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ7_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][6]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ6_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][5]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ5_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][4]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ4_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][3]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ3_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][2]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ2_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][1]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ1_DLY_B0)
							 | P_Fld(u1DelayCellOfst[uiPSRAM_DQ_Mapping_POP[p->channel][0]] , SHU1_R0_B0_DQ0_RK0_TX_ARDQ0_DLY_B0));
                #if 0
					 vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ0),
								P_Fld(u1DelayCellOfst[15] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ7_DLY_B1)
							 | P_Fld(u1DelayCellOfst[14] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ6_DLY_B1)
							 | P_Fld(u1DelayCellOfst[13] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ5_DLY_B1)
							 | P_Fld(u1DelayCellOfst[12] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ4_DLY_B1)
							 | P_Fld(u1DelayCellOfst[11] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ3_DLY_B1)
							 | P_Fld(u1DelayCellOfst[10] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ2_DLY_B1)
							 | P_Fld(u1DelayCellOfst[9] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ1_DLY_B1)
							 | P_Fld(u1DelayCellOfst[8] , SHU1_R0_B1_DQ0_RK0_TX_ARDQ0_DLY_B1));
				#endif
				 }
			}

		}

#if REG_ACCESS_PORTING_DGB
	RegLogEnable =0;
#endif

	mcSHOW_DBG_MSG3("[TxWindowPerbitCal] Done\n\n");
	mcFPRINTF(fp_A60501, "[TxWindowPerbitCal] Done\n\n");

#if 0
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DRAMC_REG_PADCTL4), 1, PADCTL4_CKEFIXON);  // test only
#endif

#if PSRAM_RX_EYE
	if (u1VrefScanEnable)
	{
		mcSHOW_DBG_MSG3("\n\n === TX eye scan Vref:(%d, %d, %d) delay:(%d, %d, 1) ===\n\n",
			u2VrefBegin, u2VrefEnd, u2VrefStep, u2DQDelayBegin, u2DQDelayEnd);
		for (ii=0; ii < DQ_DATA_WIDTH_PSRAM; ii++)
		{
			mcSHOW_DBG_MSG3("\n\n--- bit[%d] ---\n", ii);
			for (u2VrefLevel = u2VrefBegin; u2VrefLevel <= u2VrefEnd; u2VrefLevel += u2VrefStep)
			{
				mcSHOW_DBG_MSG3("  ");
				for (uiDelay = u2DQDelayBegin; uiDelay <u2DQDelayEnd; uiDelay = uiDelay+1)
				{
					if ((uiDelay > gEyeScan_Max[u2VrefLevel][ii][2]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][2]))
						mcSHOW_DBG_MSG3("x");
					else if ((uiDelay > gEyeScan_Min[u2VrefLevel][ii][2]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][2]))
						mcSHOW_DBG_MSG3("o");
					else if ((uiDelay > gEyeScan_Max[u2VrefLevel][ii][1]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][1]))
						mcSHOW_DBG_MSG3("x");
					else if ((uiDelay > gEyeScan_Min[u2VrefLevel][ii][1]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][1]))
						mcSHOW_DBG_MSG3("o");
					else if ((uiDelay > gEyeScan_Max[u2VrefLevel][ii][0]) && (EYESCAN_DATA_INVALID != gEyeScan_Max[u2VrefLevel][ii][0]))
						mcSHOW_DBG_MSG3("x");
					else if ((uiDelay > gEyeScan_Min[u2VrefLevel][ii][0]) && (EYESCAN_DATA_INVALID != gEyeScan_Min[u2VrefLevel][ii][0]))
						mcSHOW_DBG_MSG3("o");
					else
						mcSHOW_DBG_MSG3("x");
				}

				mcSHOW_DBG_MSG3("  Vref[%d]: win=%d, 0(%d, %d), 1(%d, %d), 2(%d, %d)\n",
					u2VrefLevel,
					gEyeScan_WinSize[u2VrefLevel][ii],
					gEyeScan_Min[u2VrefLevel][ii][0],
					gEyeScan_Max[u2VrefLevel][ii][0],
					gEyeScan_Min[u2VrefLevel][ii][1],
					gEyeScan_Max[u2VrefLevel][ii][1],
					gEyeScan_Min[u2VrefLevel][ii][2],
					gEyeScan_Max[u2VrefLevel][ii][2]);
			}
		}
		mcSHOW_DBG_MSG3("\n\n");
	}
#endif

	return DRAM_OK;
}

#define DutyPrintAllLog         1
#define DutyPrintCalibrationLog 1
#define DUTY_OFFSET_START -8
#define DUTY_OFFSET_END 8

#define CLOCK_PI_START 0
#define CLOCK_PI_END 63
#define CLOCK_PI_STEP 2

#define ClockDutyFailLowerBound 4500    // 45%
#define ClockDutyFailUpperBound 5500    // 55%
#define ClockDutyMiddleBound    5000    // 50%

static void DramcClockDutySetClkDelayCell(DRAMC_CTX_T *p, U8 u1RankIdx, S8 scDutyDelay, U8 use_rev_bit)
{
    U8 u1ShuffleIdx = 0;
    U32 save_offset;
    U8 ucDelay, ucDelayB;
    U8 ucRev_Bit0=0, ucRev_Bit1=0;

//    mcSHOW_DBG_MSG("CH%d, Final CLK duty delay cell = %d\n", p->channel, scDutyDelay);

    if (scDutyDelay<0)
    {
        ucDelay = -scDutyDelay;
        ucDelayB = 0;

        if (use_rev_bit)
        {
            ucRev_Bit0 = 1;
            ucRev_Bit1 = 0;
        }
    }
    else if (scDutyDelay>0)
    {
        ucDelay = 0;
        ucDelayB= scDutyDelay;

        if (use_rev_bit)
        {
            ucRev_Bit0 = 0;
            ucRev_Bit1 = 1;
        }
    }
    else
    {
        ucDelay = 0;
        ucDelayB= 0;

        if (use_rev_bit)
        {
            ucRev_Bit0 = 0;
            ucRev_Bit1 = 0;
        }
    }

#if DUTY_SCAN_V2_ONLY_K_HIGHEST_FREQ
    for(u1ShuffleIdx = 0; u1ShuffleIdx<DRAM_DFS_SHUFFLE_MAX; u1ShuffleIdx++)
#endif
    {
        save_offset = u1ShuffleIdx * SHU_GRP_DDRPHY_OFFSET + u1RankIdx*0x100;
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD1) + save_offset, P_Fld(ucDelay, SHU1_R0_CA_CMD1_RK0_TX_ARCLK_DLY) | P_Fld(ucDelay, SHU1_R0_CA_CMD1_RK0_TX_ARCLKB_DLY));
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD0) + save_offset, P_Fld(ucDelayB, SHU1_R0_CA_CMD0_RK0_TX_ARCLK_DLYB) | P_Fld(ucDelayB, SHU1_R0_CA_CMD0_RK0_TX_ARCLKB_DLYB));

        save_offset = u1ShuffleIdx * SHU_GRP_DDRPHY_OFFSET;
        vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD3), P_Fld(ucRev_Bit0, SHU1_CA_CMD3_RG_TX_ARCMD_PU_BIT0) | P_Fld(ucRev_Bit1, SHU1_CA_CMD3_RG_TX_ARCMD_PU_BIT1));
    }
}

static void DQSDutyScan_SetDqsDelayCell(DRAMC_CTX_T *p, S8 *scDutyDelay, U8 use_rev_bit)
{
    U8 u1ShuffleIdx = 0, u1DQSIdx, u1RankIdx = 0;
    U32 save_offset;
    U8 u1Delay[2], u1DelayB[2];
    U8 ucRev_Bit0[2]={0,0}, ucRev_Bit1[2]={0,0};

//    mcSHOW_DBG_MSG("CH%d, Final DQS0 duty delay cell = %d\n", p->channel, scDutyDelay[0]);
//    mcSHOW_DBG_MSG("CH%d, Final DQS1 duty delay cell = %d\n", p->channel, scDutyDelay[1]);

    for(u1DQSIdx=0; u1DQSIdx<2; u1DQSIdx++)
    {
        if(scDutyDelay[u1DQSIdx] <0)
        {
            u1Delay[u1DQSIdx]  = -(scDutyDelay[u1DQSIdx]);
            u1DelayB[u1DQSIdx]  =0;

            if (use_rev_bit)
            {
                ucRev_Bit0[u1DQSIdx] = 1;
                ucRev_Bit1[u1DQSIdx] = 0;
            }
        }
        else if(scDutyDelay[u1DQSIdx] >0)
        {
            u1Delay[u1DQSIdx]  = 0;
            u1DelayB[u1DQSIdx]  = scDutyDelay[u1DQSIdx];

            if (use_rev_bit)
            {
                ucRev_Bit0[u1DQSIdx] = 0;
                ucRev_Bit1[u1DQSIdx] = 1;
            }
        }
        else
        {
            u1Delay[u1DQSIdx]  = 0;
            u1DelayB[u1DQSIdx]  =0;

            if (use_rev_bit)
            {
                ucRev_Bit0[u1DQSIdx] = 0;
                ucRev_Bit1[u1DQSIdx] = 0;
            }
        }
    }

#if DUTY_SCAN_V2_ONLY_K_HIGHEST_FREQ
    for(u1ShuffleIdx = 0; u1ShuffleIdx<DRAM_DFS_SHUFFLE_MAX; u1ShuffleIdx++)
#endif
    {
        for(u1RankIdx = 0; u1RankIdx<RANK_MAX; u1RankIdx++)
        {
            for(u1DQSIdx = 0; u1DQSIdx<2; u1DQSIdx++)
            {
                save_offset = u1ShuffleIdx * SHU_GRP_DDRPHY_OFFSET + u1RankIdx*0x100 + u1DQSIdx*0x50;
                vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ1) + save_offset, P_Fld(u1Delay[u1DQSIdx], SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLY_B0) | P_Fld(u1Delay[u1DQSIdx], SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLY_B0));
                vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ1) + save_offset, P_Fld(u1DelayB[u1DQSIdx], SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLYB_B0) | P_Fld(u1DelayB[u1DQSIdx], SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLYB_B0));

                    save_offset = u1ShuffleIdx * SHU_GRP_DDRPHY_OFFSET + u1DQSIdx*0x80;
                    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1) + save_offset, P_Fld(ucRev_Bit0[u1DQSIdx], RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN) | P_Fld(ucRev_Bit1[u1DQSIdx], RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN));
            }
        }
    }
}

static S8 DutyScan_Offset_Convert(U8 val)
{
    U8 calibration_sequence[15]={0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};

    return ((S8)(calibration_sequence[val]>8 ? 0-(calibration_sequence[val]&0x7) : calibration_sequence[val]));

}

static void DutyScan_Offset_Calibration(DRAMC_CTX_T *p)
{
    U8 calibration_sequence[15]={0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9, 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7};
    U8 i, read_val_b0, read_val_b1, read_val_ca;
    U8 cal_i_b0=0xff, cal_i_b1=0xff, cal_i_ca=0xff;

#if VENDER_JV_LOG
    vPrintCalibrationBasicInfo_ForJV(p);
#else
    vPrintCalibrationBasicInfo(p);
#endif

#if DutyPrintCalibrationLog
    mcSHOW_DBG_MSG("[Duty_Offset_Calibration]\n\n");
#endif

    //B0
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), P_Fld(1, B0_DQ6_RG_RX_ARDQ_LPBK_EN_B0) | P_Fld(0, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0) | P_Fld(1, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), P_Fld(1, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN) | P_Fld(2, RG_ARDQ_REV_BIT_2221_DATA_SWAP));
//    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ3), P_Fld(1, SHU1_B0_DQ3_DQ_REV_B0_BIT_06) | P_Fld(0, SHU1_B0_DQ3_DQ_REV_B0_BIT_05) | P_Fld(1, SHU1_B0_DQ3_DQ_REV_B0_BIT_04));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), P_Fld(0, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_BYPASS_B0) | P_Fld(0xB, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0));
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), 1, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ0), 0, B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), P_Fld(1, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0) | P_Fld(0x1, B0_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B0));
    mcDELAY_US(1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), 1, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT2);

    //B1
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), P_Fld(1, B1_DQ6_RG_RX_ARDQ_LPBK_EN_B1) | P_Fld(0, B1_DQ6_RG_RX_ARDQ_DDR4_SEL_B1) | P_Fld(1, B1_DQ6_RG_RX_ARDQ_DDR3_SEL_B1));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), P_Fld(1, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN) | P_Fld(2, RG_ARDQ_REV_BIT_2221_DATA_SWAP));
//    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ3), P_Fld(1, SHU1_B1_DQ3_DQ_REV_B1_BIT_06) | P_Fld(0, SHU1_B1_DQ3_DQ_REV_B1_BIT_05) | P_Fld(1, SHU1_B1_DQ3_DQ_REV_B1_BIT_04));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), P_Fld(0, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_BYPASS_B1) | P_Fld(0xB, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1));
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), 1, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ0), 0, B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), P_Fld(1, B1_DQ5_RG_RX_ARDQ_VREF_EN_B1) | P_Fld(0x1, B1_DQ5_RG_RX_ARDQ_EYE_VREF_EN_B1));
    mcDELAY_US(1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), 1, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT2);

    //CA
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_CA_CMD6), P_Fld(1, CA_CMD6_RG_RX_ARCMD_LPBK_EN) | P_Fld(0, CA_CMD6_RG_RX_ARCMD_DDR4_SEL) | P_Fld(1, CA_CMD6_RG_RX_ARCMD_DDR3_SEL));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_DLL1), P_Fld(1, RG_ARCMD_REV_BIT_20_DATA_SWAP_EN) | P_Fld(2, RG_ARCMD_REV_BIT_2221_DATA_SWAP));
//    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD3), P_Fld(1, SHU1_CA_CMD3_ARCMD_REV_BIT_06) | P_Fld(0, SHU1_CA_CMD3_ARCMD_REV_BIT_05) | P_Fld(1, SHU1_CA_CMD3_ARCMD_REV_BIT_04));
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD5), P_Fld(0, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_BYPASS) | P_Fld(0xB, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL));
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), 1, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_CMD0), 0, CA_CMD0_RG_RX_ARCA2_OFFC);
    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_CA_CMD5), P_Fld(1, CA_CMD5_RG_RX_ARCMD_VREF_EN) | P_Fld(0x1, CA_CMD5_RG_RX_ARCMD_EYE_VREF_EN));
    mcDELAY_US(1);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), 1, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT2);

    mcDELAY_US(1);

    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), 1, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT0);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), 1, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT0);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), 1, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT0);

#if DutyPrintCalibrationLog
    mcSHOW_DBG_MSG("\tB0\tB1\tCA\n");
    mcSHOW_DBG_MSG("===========================\n");
#endif

    for(i=0; i<15; i++)
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ0), calibration_sequence[i], B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ0), calibration_sequence[i], B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_CMD0), calibration_sequence[i], CA_CMD0_RG_RX_ARCA2_OFFC);

        mcDELAY_US(1);

        read_val_b0 = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1), MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B0_BIT2);
        read_val_b1 = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1), MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B1_BIT2);
        read_val_ca = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_CMD_O1), MISC_AD_RX_CMD_O1_AD_RX_ARCA2_O1);

#if DutyPrintCalibrationLog
        mcSHOW_DBG_MSG("%d\t%d\t%d\t%d\n", DutyScan_Offset_Convert(i), read_val_b0, read_val_b1, read_val_ca);
#endif

        if (read_val_b0 == 0 && cal_i_b0==0xff)
        {
            cal_i_b0 = i;
        }

        if (read_val_b1 == 0 && cal_i_b1==0xff)
        {
            cal_i_b1 = i;
        }

        if (read_val_ca == 0 && cal_i_ca==0xff)
        {
            cal_i_ca = i;
        }
    }

    if (cal_i_b0==0 || cal_i_b1==0 || cal_i_ca==0)
    {
#if DutyPrintCalibrationLog
        mcSHOW_ERR_MSG("offset calibration i=-7 and AD_RX_*RDQ_O1_B*<2>/AD_RX_*RCA2_O1 ==0 !!\n");
#endif
#if __ETT__
        while(1);
#endif
    }
    else
    if ((read_val_b0==1 && cal_i_b0==0xff) || (read_val_b1==1 && cal_i_b1==0xff) || (read_val_ca==1 && cal_i_ca==0xff))
    {
#if DutyPrintCalibrationLog
        mcSHOW_ERR_MSG("offset calibration i=7 and AD_RX_*RDQ_O1_B*<2>/AD_RX_*RCA2_O1 ==1 !!\n");
#endif
#if __ETT__
        while(1);
#endif

    }
    else
    {
#if DutyPrintCalibrationLog
        mcSHOW_DBG_MSG("===========================\n");
        mcSHOW_DBG_MSG("\tB0:%d\tB1:%d\tCA:%d\n",DutyScan_Offset_Convert(cal_i_b0),DutyScan_Offset_Convert(cal_i_b1),DutyScan_Offset_Convert(cal_i_ca));
#endif
    }

    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), 0, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT0);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), 0, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT0);
    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), 0, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT0);

    if (cal_i_b0!=0xff) vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ0), calibration_sequence[cal_i_b0], B0_DQ0_RG_RX_ARDQ2_OFFC_B0);
    if (cal_i_b1!=0xff) vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ0), calibration_sequence[cal_i_b1], B1_DQ0_RG_RX_ARDQ2_OFFC_B1);
    if (cal_i_ca!=0xff) vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_CMD0), calibration_sequence[cal_i_ca], CA_CMD0_RG_RX_ARCA2_OFFC);

    return;
}


S8 gcFinal_K_Duty_clk_delay_cell[1];
static DRAM_STATUS_T DutyScan_Calibration_Flow(DRAMC_CTX_T *p, U8 k_type, U8 use_rev_bit)
{
    S8 scinner_duty_ofst, scFinal_clk_delay_cell[DQS_NUMBER]={0,0};
    S8 scinner_duty_ofst_start = 0, scinner_duty_ofst_end = 0;
    S32 scdqs_dly, s4PICnt, s4PIBegin, s4PIEnd, s4PICnt_mod64;
    S8 i, swap_idx, ucdqs_i, ucdqs_i_count=2;
    U8 u1ByteIdx;
    U8 ucDelay, ucDelayB;
    U8 ucRev_Bit0=0, ucRev_Bit1=0;
    U32 u4DutyDiff, u4DutyDiff_Limit=900;

    U8 vref_sel_value[2], cal_out_value;
    S32 duty_value[2];
    S32 final_duty;

    U32 ucperiod_duty_max=0, ucperiod_duty_min=0xffffffff, ucperiod_duty_max_clk_dly=0, ucperiod_duty_min_clk_dly=0;
    U32 ucperiod_duty_averige=0, ucFinal_period_duty_averige[DQS_NUMBER]={0,0}, ucmost_approach_50_percent=0xffffffff;
    U32 ucFinal_period_duty_max[DQS_NUMBER] = {0,0}, ucFinal_period_duty_min[DQS_NUMBER] = {0,0};
    U32 ucFinal_duty_max_clk_dly[DQS_NUMBER]={0},ucFinal_duty_min_clk_dly[DQS_NUMBER]={0};
    U8 early_break_count=0;
    U8 str_clk_duty[]="CLK", str_dqs_duty[]="DQS", str_dq_duty[]="DQ", str_dqm_duty[]="DQM";
    U8 *str_who_am_I=str_clk_duty;

    mcSHOW_DBG_MSG("\n[DutyScan_Calibration_Flow] %s Calibration\n", use_rev_bit==0 ? "First" : "Second");
    mcSHOW_DBG_MSG("\n[DutyScan_Calibration_Flow] k_type=%d, use_rev_bit=%d\n", k_type, use_rev_bit);
    /*TINFO="\n[DutyScan_Calibration_Flow] k_type=%d\n", k_type */


    if (k_type == DutyScan_Calibration_K_CLK)
    {
#if 0
        // DQS duty test 3
        //mcSHOW_ERR_MSG("\n[*PHDET_EN*=0]\n");
        mcSHOW_ERR_MSG("\n[*PI*RESETB*=0  *PHDET_EN*=0  *PI_RESETB*=1]\n");
        /*TINFO="\n[*PI*RESETB*=0  *PHDET_EN*=0  *PI_RESETB*=1]\n" */
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI0), 0x0, CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI2), 0x0, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI0), 0x1, CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
#else
#if DutyPrintCalibrationLog
        mcSHOW_DBG_MSG("\n[  *PHDET_EN*=0  \n");
#endif
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_DLL_ARPI2), 0x0, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
#endif
    }
    else
    {
        // DQS duty test 3
        //mcSHOW_ERR_MSG("\n[*PHDET_EN*=0]\n");
#if DutyPrintCalibrationLog
        mcSHOW_DBG_MSG("[*PI*RESETB*=0  *PHDET_EN*=0  *PI_RESETB*=1]\n");
#endif
        /*TINFO="[*PI*RESETB*=0  *PHDET_EN*=0  *PI_RESETB*=1]\n" */
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI0), 0x0, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DLL_ARPI0), 0x0, B1_DLL_ARPI0_RG_ARPI_RESETB_B1);
	//	mcDELAY_US(3);

	    #if 0//PSRAM_SPEC
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2_PSRAM), 0x0, B0_DLL_ARPI2_PSRAM_RG_ARDLL_PHDET_EN_B0_PSRAM);
		#endif
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI2), 0x0, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);

        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DLL_ARPI2), 0x0, B1_DLL_ARPI2_RG_ARDLL_PHDET_EN_B1);

        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DLL_ARPI0), 0x1, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DLL_ARPI0), 0x1, B1_DLL_ARPI0_RG_ARPI_RESETB_B1);
    }

    //CLK Source Select (DQ/DQM/DQS/CLK)
    if (k_type == DutyScan_Calibration_K_DQ) // K DQ
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ8), 0, B0_DQ8_RG_TX_ARDQ_CAP_DET_B0);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), 0, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1);

        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ8), 0, B1_DQ8_RG_TX_ARDQ_CAP_DET_B1);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), 0, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1);

        ucdqs_i_count = 1;
        str_who_am_I = (U8*)str_dq_duty;

        scinner_duty_ofst_start = 0;
        scinner_duty_ofst_end = 0;
    }
    else if (k_type == DutyScan_Calibration_K_DQM) // K DQM
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ8), 0, B0_DQ8_RG_TX_ARDQ_CAP_DET_B0);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), 1, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1);

        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ8), 0, B1_DQ8_RG_TX_ARDQ_CAP_DET_B1);
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), 1, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1);

        ucdqs_i_count = 1;
        str_who_am_I = (U8*)str_dqm_duty;

        scinner_duty_ofst_start = 0;
        scinner_duty_ofst_end = 0;
    }
    else if (k_type == DutyScan_Calibration_K_DQS) // K DQS
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ8), 1, B0_DQ8_RG_TX_ARDQ_CAP_DET_B0);

        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ8), 1, B1_DQ8_RG_TX_ARDQ_CAP_DET_B1);

        ucdqs_i_count = 1;
        str_who_am_I = (U8*)str_dqs_duty;

        scinner_duty_ofst_start = DUTY_OFFSET_START;
        scinner_duty_ofst_end = DUTY_OFFSET_END;

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        p->pSavetimeData->u1dqs_use_rev_bit = use_rev_bit;
#endif
    }
    else if (k_type == DutyScan_Calibration_K_CLK) // K CLK
    {
        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_CA_CMD9), 1, CA_CMD9_RG_TX_ARCMD_CAP_DET);

        ucdqs_i_count = 1;
        str_who_am_I = (U8*)str_clk_duty;

        scinner_duty_ofst_start = DUTY_OFFSET_START;
        scinner_duty_ofst_end = DUTY_OFFSET_END;

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        p->pSavetimeData->u1clk_use_rev_bit = use_rev_bit;
#endif
    }

#if defined(YH_SWEEP_IC) || FT_DSIM_USED
        scinner_duty_ofst_start=0;
        scinner_duty_ofst_end=0;
#endif

#if (fcFOR_CHIP_ID == fcCannon)
    if (k_type == DutyScan_Calibration_K_CLK)
    {
        u4DutyDiff_Limit = 530;
    }
    else
    {
        u4DutyDiff_Limit = 580;
    }
#endif

#if (fcFOR_CHIP_ID == fcSchubert)
#if !defined(YH_SWEEP_IC)
    if (k_type == DutyScan_Calibration_K_CLK && p->channel == CHANNEL_A)
    {
		s4PIBegin = 16;
        s4PIEnd = 16;
    }
    else
#endif
#endif
    {
        s4PIBegin = CLOCK_PI_START;
        s4PIEnd = CLOCK_PI_END;
    }

    for(ucdqs_i=0; ucdqs_i<ucdqs_i_count; ucdqs_i++)
    {
#if DutyPrintCalibrationLog
        if (k_type == DutyScan_Calibration_K_CLK)
        {
            mcSHOW_DBG_MSG("\n[CLK Duty scan]\n");
            /*TINFO="\n[CLK Duty scan]\n"*/
        }
        else
        {
            mcSHOW_DBG_MSG("\n[%s B%d Duty scan]\n", str_who_am_I, ucdqs_i);
            /*TINFO="\n[%s B%d Duty scan]\n", str_who_am_I, ucdqs_i */
        }
#endif

        ucmost_approach_50_percent=0xffffffff;
        early_break_count=0;

        for(scinner_duty_ofst=scinner_duty_ofst_start; scinner_duty_ofst<=scinner_duty_ofst_end; scinner_duty_ofst = scinner_duty_ofst+1)
        {
            ucperiod_duty_max = 0;
            ucperiod_duty_min = 100000;

            if (scinner_duty_ofst<0)
            {
                ucDelay = -scinner_duty_ofst;
                ucDelayB = 0;

                if (use_rev_bit)
                {
                    ucRev_Bit0 = 1;
                    ucRev_Bit1 = 0;
                }
            }
            else if (scinner_duty_ofst>0)
            {
                ucDelay = 0;
                ucDelayB= scinner_duty_ofst;

                if (use_rev_bit)
                {
                    ucRev_Bit0 = 0;
                    ucRev_Bit1 = 1;
                }
            }
            else
            {
                ucDelay = 0;
                ucDelayB= 0;

                if (use_rev_bit)
                {
                    ucRev_Bit0 = 0;
                    ucRev_Bit1 = 0;
                }
            }

            if (k_type == DutyScan_Calibration_K_DQS)
            {
                if (ucdqs_i==0)
                {
                    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ1), P_Fld(ucDelay, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLY_B0)
                                                                                | P_Fld(ucDelay, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLY_B0)
                                                                                | P_Fld(ucDelayB, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0_DLYB_B0)
                                                                                | P_Fld(ucDelayB, SHU1_R0_B0_DQ1_RK0_TX_ARDQS0B_DLYB_B0));

                    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), P_Fld(ucRev_Bit0, RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN)
                                                                            | P_Fld(ucRev_Bit1, RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN));

                }
                else
                {
                    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ1), P_Fld(ucDelay, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0_DLY_B1)
                                                                                | P_Fld(ucDelay, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0B_DLY_B1)
                                                                                | P_Fld(ucDelayB, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0_DLYB_B1)
                                                                                | P_Fld(ucDelayB, SHU1_R0_B1_DQ1_RK0_TX_ARDQS0B_DLYB_B1));

                    vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), P_Fld(ucRev_Bit0, RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN)
                                                                            | P_Fld(ucRev_Bit1, RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN));
                }

            }

            if (k_type == DutyScan_Calibration_K_CLK)
            {
                vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD1), P_Fld(ucDelay, SHU1_R0_CA_CMD1_RK0_TX_ARCLK_DLY)
                                                                            | P_Fld(ucDelay, SHU1_R0_CA_CMD1_RK0_TX_ARCLKB_DLY));
                vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD0), P_Fld(ucDelayB, SHU1_R0_CA_CMD0_RK0_TX_ARCLK_DLYB)
                                                                            | P_Fld(ucDelayB, SHU1_R0_CA_CMD0_RK0_TX_ARCLKB_DLYB));

                vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD3), P_Fld(ucRev_Bit0, SHU1_CA_CMD3_RG_TX_ARCMD_PU_BIT0)
                                                                        | P_Fld(ucRev_Bit1, SHU1_CA_CMD3_RG_TX_ARCMD_PU_BIT1));
            }

            for(s4PICnt=s4PIBegin; s4PICnt<=s4PIEnd; s4PICnt+=2)
            {
                s4PICnt_mod64 = (s4PICnt+64)&0x3f;//s4PICnt_mod64 = (s4PICnt+64)%64;

#if DutyPrintAllLog
                //if(scinner_duty_ofst!=DUTY_OFFSET_START)
                    mcSHOW_DBG_MSG("PI= %d\n", s4PICnt_mod64);
#endif

                if (k_type == DutyScan_Calibration_K_DQS)
                {
                    if (ucdqs_i==0)
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), s4PICnt_mod64, SHU1_R0_B0_DQ7_RK0_ARPI_PBYTE_B0);
                    }
                    else
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7), s4PICnt_mod64, SHU1_R0_B1_DQ7_RK0_ARPI_PBYTE_B1);
                    }
                }
                else
                if (k_type == DutyScan_Calibration_K_CLK)
                {
                    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9), s4PICnt_mod64, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK);
                }
                else
                if (k_type == DutyScan_Calibration_K_DQ)
                {
                    if (ucdqs_i==0)
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), s4PICnt_mod64, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
                    }
                    else
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7), s4PICnt_mod64, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1);
                    }
                }
                else
                if (k_type == DutyScan_Calibration_K_DQM)
                {
                    if (ucdqs_i==0)
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), s4PICnt_mod64, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0);
                    }
                    else
                    {
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B1_DQ7), s4PICnt_mod64, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1);
                    }
                }

                for(swap_idx=0; swap_idx<2; swap_idx++)
                {
                    if (k_type == DutyScan_Calibration_K_CLK)
                    {
                        if (swap_idx==0)
                        {
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_DLL1), 2, RG_ARCMD_REV_BIT_2221_DATA_SWAP);
                        }
                        else
                        {
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_DLL1), 3, RG_ARCMD_REV_BIT_2221_DATA_SWAP);
                        }

                        vref_sel_value[swap_idx]= 0;
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD5), vref_sel_value[swap_idx]>>1, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
                        vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT1);
                    }
                    else
                    {
                        if (ucdqs_i==0)
                        {
                            if (swap_idx==0)
                            {
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), 2, RG_ARDQ_REV_BIT_2221_DATA_SWAP);
                            }
                            else
                            {
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DLL1), 3, RG_ARDQ_REV_BIT_2221_DATA_SWAP);
                            }

                            vref_sel_value[swap_idx]= 0;
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), vref_sel_value[swap_idx]>>1, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT1);
                        }
                        else
                        {
                            if (swap_idx==0)
                            {
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), 2, RG_ARDQ_REV_BIT_2221_DATA_SWAP);
                            }
                            else
                            {
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DLL1), 3, RG_ARDQ_REV_BIT_2221_DATA_SWAP);
                            }

                            vref_sel_value[swap_idx]= 0;
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), vref_sel_value[swap_idx]>>1, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT1);
                        }
                    }

                    for(i=5; i>=0; i--)
                    {
                        if (k_type == DutyScan_Calibration_K_CLK)
                        {
                            vref_sel_value[swap_idx] |= (1<<i);
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD5), vref_sel_value[swap_idx]>>1, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
                            vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_CA_CMD0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_CA_CMD0_RG_TX_ARCLK_DRVP_PRE_BIT1);

                            mcDELAY_US(1);

                            cal_out_value = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_CMD_O1), MISC_AD_RX_CMD_O1_AD_RX_ARCA2_O1);

                            if (cal_out_value == 0)
                            {
                                vref_sel_value[swap_idx] &= ~(1<<i);
                            }
                        }
                        else
                        {
                            if (ucdqs_i==0)
                            {
                                vref_sel_value[swap_idx] |= (1<<i);
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), vref_sel_value[swap_idx]>>1, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_B0_DQ0_RG_TX_ARDQS0_DRVP_PRE_B0_BIT1);

                                mcDELAY_US(1);

                                cal_out_value = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1), MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B0_BIT2);
                            }
                            else
                            {
                                vref_sel_value[swap_idx] |= (1<<i);
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), vref_sel_value[swap_idx]>>1, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);
                                vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ0), (vref_sel_value[swap_idx]&1)==1?0:1, SHU1_B1_DQ0_RG_TX_ARDQS0_DRVP_PRE_B1_BIT1);

                                mcDELAY_US(1);

                                cal_out_value = u4IO32ReadFldAlign(DRAMC_REG_ADDR(DDRPHY_MISC_AD_RX_DQ_O1), MISC_AD_RX_DQ_O1_AD_RX_ARDQ_O1_B1_BIT2);
                            }

#if DutyPrintAllLog
                            //mcSHOW_DBG_MSG("Fra i=%d vref_sel_value[swap_idx]=%x, cal_out=%d\n",i, vref_sel_value[swap_idx], cal_out_value);
#endif

                            if (cal_out_value == 0)
                            {
                                vref_sel_value[swap_idx] &= ~(1<<i);
                            }
                        }
                    }
                }


                for(swap_idx=0; swap_idx<2; swap_idx++)
                {
                    if (vref_sel_value[swap_idx]<=31)
                    {
                        duty_value[swap_idx] = (vref_sel_value[swap_idx]-23)*69+5050;
                    }
                    else
                    {
                        duty_value[swap_idx] = (vref_sel_value[swap_idx]-32)*55+5600;
                    }
                }

#if DutyPrintAllLog
                mcSHOW_DBG_MSG("\t[%d][%d] B%d : Vref_Sel=0x%x, Swap Vref_Sel=0x%x\n", scinner_duty_ofst, s4PICnt, ucdqs_i, vref_sel_value[0], vref_sel_value[1]);
                mcSHOW_DBG_MSG("\t[%d][%d] B%d : duty_value=%d, Swap duty_value=%d\n", scinner_duty_ofst, s4PICnt, ucdqs_i, duty_value[0], duty_value[1]);
#endif

                final_duty = 5000+((duty_value[0]-duty_value[1])/2);

                if (final_duty > (S32) ucperiod_duty_max)
                {
                    ucperiod_duty_max = final_duty;
                    ucperiod_duty_max_clk_dly = s4PICnt;
                }
                if (final_duty < (S32) ucperiod_duty_min)
                {
                    ucperiod_duty_min = final_duty;
                    ucperiod_duty_min_clk_dly = s4PICnt;
                }

#if DutyPrintAllLog
                mcSHOW_DBG_MSG("\t[%d][%d] B%d : Final_Duty=%d\n", scinner_duty_ofst, s4PICnt, ucdqs_i, final_duty);
#endif
            }



            ucperiod_duty_averige = (ucperiod_duty_max + ucperiod_duty_min)>>1;

#if DutyPrintCalibrationLog
            if (k_type==DutyScan_Calibration_K_CLK)
            {
                mcSHOW_DBG_MSG("[%d] CLK\n",scinner_duty_ofst);
                /*TINFO="[%d] CLK\n",scinner_duty_ofst */
            }
            else
            {
                mcSHOW_DBG_MSG("[%d] %s%d\n",scinner_duty_ofst, str_who_am_I, ucdqs_i);
                /*TINFO="[%d] %s%d\n",scinner_duty_ofst, str_who_am_I, ucdqs_i */
            }
#endif

#if DutyPrintCalibrationLog
            mcSHOW_DBG_MSG("\tMAX Duty = %d%%(X100), CLK PI=%d\n",ucperiod_duty_max, ucperiod_duty_max_clk_dly);
            /*TINFO="\tMAX Duty = %d%%(X100), CLK PI=%d\n",ucperiod_duty_max, ucperiod_duty_max_clk_dly */
            mcSHOW_DBG_MSG("\tMIN Duty = %d%%(X100), CLK PI=%d\n",ucperiod_duty_min, ucperiod_duty_min_clk_dly);
            /*TINFO="\tMIN Duty = %d%%(X100), CLK PI=%d\n",ucperiod_duty_min, ucperiod_duty_min_clk_dly */
            mcSHOW_DBG_MSG("\tAVG Duty = %d%%(X100)\n", ucperiod_duty_averige);
            /*TINFO="\tAVG Duty = %d%%(X100)\n", ucperiod_duty_averige */
#endif

            if (ucperiod_duty_averige >= ClockDutyMiddleBound)
            {
                if ((scinner_duty_ofst<=0 && ((ucperiod_duty_averige-ClockDutyMiddleBound+(ucperiod_duty_max-ucperiod_duty_min)/2) <= ucmost_approach_50_percent)) ||
                     (scinner_duty_ofst>0 && ((ucperiod_duty_averige-ClockDutyMiddleBound+(ucperiod_duty_max-ucperiod_duty_min)/2) < ucmost_approach_50_percent)))
                {
                    ucmost_approach_50_percent = ucperiod_duty_averige-ClockDutyMiddleBound+(ucperiod_duty_max-ucperiod_duty_min)/2;
                    scFinal_clk_delay_cell[ucdqs_i] = scinner_duty_ofst;
                    ucFinal_period_duty_averige[ucdqs_i] = ucperiod_duty_averige;
                    ucFinal_period_duty_max[ucdqs_i] = ucperiod_duty_max;
                    ucFinal_period_duty_min[ucdqs_i] = ucperiod_duty_min;
                    ucFinal_duty_max_clk_dly[ucdqs_i] = ucperiod_duty_max_clk_dly;
                    ucFinal_duty_min_clk_dly[ucdqs_i] = ucperiod_duty_min_clk_dly;
#if DutyPrintCalibrationLog
                    mcSHOW_DBG_MSG("\t!!! ucmost_approach_50_percent = %d%%(X100) !!!\n",ucmost_approach_50_percent);
                    /*TINFO="!!! ucmost_approach_50_percent = %d%%(X100) !!!\n",ucmost_approach_50_percent */
#endif
                    early_break_count = 0;
                }
                else
                {
                    if (scinner_duty_ofst>0) early_break_count ++;
#if DutyPrintAllLog==0
                    if (early_break_count>=2) break; //early break;
#endif
                }
            }
            else
            {
                if ((scinner_duty_ofst<=0 && ((ClockDutyMiddleBound-ucperiod_duty_averige+(ucperiod_duty_max-ucperiod_duty_min)/2) <= ucmost_approach_50_percent)) ||
                    (scinner_duty_ofst>0 && ((ClockDutyMiddleBound-ucperiod_duty_averige+(ucperiod_duty_max-ucperiod_duty_min)/2) < ucmost_approach_50_percent)))
                {
                    ucmost_approach_50_percent = ClockDutyMiddleBound-ucperiod_duty_averige+(ucperiod_duty_max-ucperiod_duty_min)/2;
                    scFinal_clk_delay_cell[ucdqs_i] = scinner_duty_ofst;
                    ucFinal_period_duty_averige[ucdqs_i] = ucperiod_duty_averige;
                    ucFinal_period_duty_max[ucdqs_i] = ucperiod_duty_max;
                    ucFinal_period_duty_min[ucdqs_i] = ucperiod_duty_min;
                    ucFinal_duty_max_clk_dly[ucdqs_i] = ucperiod_duty_max_clk_dly;
                    ucFinal_duty_min_clk_dly[ucdqs_i] = ucperiod_duty_min_clk_dly;
#if DutyPrintCalibrationLog
                    mcSHOW_DBG_MSG("\t!!! ucmost_approach_50_percent = %d%%(X100) !!!\n",ucmost_approach_50_percent);
                    /*TINFO="!!! ucmost_approach_50_percent = %d%%(X100) !!!\n",ucmost_approach_50_percent */
#endif
                    early_break_count = 0;
                }
                else
                {
                    if (scinner_duty_ofst>0) early_break_count ++;
#if DutyPrintAllLog==0
                    if (early_break_count>=2) break; //early break;
#endif
                }
            }

#if DutyPrintCalibrationLog
            mcSHOW_DBG_MSG("\n");
            /*TINFO="\n" */
#endif
        }
    }

    for(ucdqs_i=0; ucdqs_i<ucdqs_i_count; ucdqs_i++)
    {
        //for SLT, use ERR_MSG to force print log
        if (k_type == DutyScan_Calibration_K_CLK)
        {
            mcSHOW_DBG_MSG("\n==%s ==\n", str_who_am_I, ucdqs_i);
            /*TINFO="\n==%s ==\n", str_who_am_I */
        }
        else
        {
            mcSHOW_DBG_MSG("\n==%s %d ==\n", str_who_am_I, ucdqs_i);
            /*TINFO="\n==%s %d ==\n", str_who_am_I, ucdqs_i */
        }
        mcSHOW_DBG_MSG("Final %s duty delay cell = %d\n", str_who_am_I, scFinal_clk_delay_cell[ucdqs_i]);
        /*TINFO="Final %s duty delay cell = %d\n", str_who_am_I, scFinal_clk_delay_cell[ucdqs_i] */
        mcSHOW_DBG_MSG("[%d] MAX Duty = %d%%(X100), DQS PI = %d\n",scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_max[ucdqs_i], ucFinal_duty_max_clk_dly[ucdqs_i]);
        /*TINFO="[%d] MAX Duty = %d%%(X100), DQS PI = %d\n",scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_max[ucdqs_i], ucFinal_duty_max_clk_dly[ucdqs_i] */
        mcSHOW_DBG_MSG("[%d] MIN Duty = %d%%(X100), DQS PI = %d\n",scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_min[ucdqs_i], ucFinal_duty_min_clk_dly[ucdqs_i]);
        /*TINFO="[%d] MIN Duty = %d%%(X100), DQS PI = %d\n",scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_min[ucdqs_i], ucFinal_duty_min_clk_dly[ucdqs_i] */
        mcSHOW_DBG_MSG("[%d] AVG Duty = %d%%(X100)\n", scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_averige[ucdqs_i]);
        /*TINFO="[%d] AVG Duty = %d%%(X100)\n", scFinal_clk_delay_cell[ucdqs_i], ucFinal_period_duty_averige[ucdqs_i] */
    }

#if FT_DSIM_USED
    FT_Duty_Compare_PassFail(p->channel, k_type, ucFinal_period_duty_max[0] , ucFinal_period_duty_min[0],ucFinal_period_duty_max[1] , ucFinal_period_duty_min[1]);
#else
    for(ucdqs_i=0; ucdqs_i<ucdqs_i_count; ucdqs_i++)
    {
        u4DutyDiff = ucFinal_period_duty_max[ucdqs_i] - ucFinal_period_duty_min[ucdqs_i];

#if DQS_DUTY_SLT_CONDITION_TEST
        if (k_type == DutyScan_Calibration_K_CLK || (k_type == DutyScan_Calibration_K_DQS))
        {
            u4DQSDutyDiff_Rec[p->channel][ucdqs_i][u1GlobalTestCnt]=u4DutyDiff;

            u4DQSDutyDutyDly[p->channel][ucdqs_i] = scFinal_clk_delay_cell[ucdqs_i];

            if(u4DutyDiff > u4DQSDutyDiff_Max[p->channel][ucdqs_i])
                u4DQSDutyDiff_Max[p->channel][ucdqs_i] = u4DutyDiff;

            if(u4DutyDiff < u4DQSDutyDiff_Min[p->channel][ucdqs_i])
                u4DQSDutyDiff_Min[p->channel][ucdqs_i] = u4DutyDiff;

            u4DQSDutyDiff_Avrg[p->channel][ucdqs_i]  += u4DutyDiff;
        }
#endif

#if defined(YH_SWEEP_IC)
        gYH_Sweep_IC_test_result[k_type][p->channel][ucdqs_i].maxduty = ucFinal_period_duty_max[ucdqs_i];
        gYH_Sweep_IC_test_result[k_type][p->channel][ucdqs_i].minduty = ucFinal_period_duty_min[ucdqs_i];
        gYH_Sweep_IC_test_result[k_type][p->channel][ucdqs_i].dutydiff = u4DutyDiff;
        gYH_Sweep_IC_test_result[k_type][p->channel][ucdqs_i].avgduty = ucFinal_period_duty_averige[ucdqs_i];
#else
        if ((((k_type == DutyScan_Calibration_K_CLK) || (k_type == DutyScan_Calibration_K_DQS)) && (u4DutyDiff < u4DutyDiff_Limit)) ||
           (((k_type == DutyScan_Calibration_K_DQ) || (k_type == DutyScan_Calibration_K_DQM)) && ((u4DutyDiff < u4DutyDiff_Limit) && (ucFinal_period_duty_averige[ucdqs_i] >= 4550 && ucFinal_period_duty_averige[ucdqs_i] <= 5450))))
        {
            if (k_type == DutyScan_Calibration_K_CLK)
            {
                mcSHOW_DBG_MSG("\nCH%d %s Duty spec in!! Max-Min= %d%%\n",p->channel, str_who_am_I, u4DutyDiff);
                /*TINFO="\nCH%d %s Duty spec in!! Max-Min= %d%%\n",p->channel, str_who_am_I, u4DutyDiff */
            }
            else
            {
                mcSHOW_DBG_MSG("\nCH%d %s %d Duty spec in!! Max-Min= %d%%\n",p->channel, str_who_am_I, ucdqs_i, u4DutyDiff);
                /*TINFO="\nCH%d %s %d Duty spec in!! Max-Min= %d%%\n",p->channel, str_who_am_I, ucdqs_i, u4DutyDiff */
            }
        }
        else
        {
            if (k_type == DutyScan_Calibration_K_CLK)
            {
                mcSHOW_DBG_MSG("\nCH%d %s Duty spec out!! Max-Min= %d%% >%d%%\n", p->channel, str_who_am_I, u4DutyDiff, u4DutyDiff_Limit);
                /*TINFO="\nCH%d %s Duty spec out!! Max-Min= %d%% >8%%\n", p->channel, str_who_am_I, u4DutyDiff */
            }
            else
            {
                mcSHOW_DBG_MSG("\nCH%d %s %d Duty spec out!! Max-Min= %d%% >%d%%\n", p->channel, str_who_am_I, ucdqs_i, u4DutyDiff, u4DutyDiff_Limit);
                /*TINFO="\nCH%d %s %d Duty spec out!! Max-Min= %d%% >8%%\n", p->channel, str_who_am_I, ucdqs_i, u4DutyDiff */
            }

            #if defined(SLT)
                    while(1); //stop here
            #endif

            #if __ETT__

                #if DQS_DUTY_SLT_CONDITION_TEST
                retStatus = DRAM_FAIL;
                #else
                    while(1); //stop here
                #endif

            #endif
        }
#endif
    }

#endif

    if ((k_type == DutyScan_Calibration_K_DQS)&&0)
    {
#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        if(p->femmc_Ready==0)
        {
            p->pSavetimeData->s1DQSDuty_clk_delay_cell[p->channel][0] = scFinal_clk_delay_cell[0];
            p->pSavetimeData->s1DQSDuty_clk_delay_cell[p->channel][1] = scFinal_clk_delay_cell[1];
        }
#endif

        // backup K DQS final values
        gcFinal_K_Duty_clk_delay_cell[0] = scFinal_clk_delay_cell[0];
        gcFinal_K_Duty_clk_delay_cell[1] = scFinal_clk_delay_cell[1];

        DQSDutyScan_SetDqsDelayCell(p, scFinal_clk_delay_cell, use_rev_bit);

#ifdef FOR_HQA_TEST_USED
        gFinalDQSDuty[p->channel][0] = scFinal_clk_delay_cell[0];
        gFinalDQSDuty[p->channel][1] = scFinal_clk_delay_cell[1];
        gFinalDQSDutyMinMax[p->channel][0][0] = ucFinal_period_duty_min[0];
        gFinalDQSDutyMinMax[p->channel][0][1] = ucFinal_period_duty_max[0];
        gFinalDQSDutyMinMax[p->channel][1][0] = ucFinal_period_duty_min[1];
        gFinalDQSDutyMinMax[p->channel][1][1] = ucFinal_period_duty_max[1];
#endif
    }

    if (k_type == DutyScan_Calibration_K_CLK)
    {
        DramcClockDutySetClkDelayCell(p, RANK_0, scFinal_clk_delay_cell[0], use_rev_bit);
      //  DramcClockDutySetClkDelayCell(p, RANK_1, scFinal_clk_delay_cell[0], use_rev_bit);

        // backup K CLK final values
        gcFinal_K_Duty_clk_delay_cell[0] = scFinal_clk_delay_cell[0];

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        if(p->femmc_Ready==0)
        {
            p->pSavetimeData->s1ClockDuty_clk_delay_cell[p->channel][RANK_0] = scFinal_clk_delay_cell[0];
       //     p->pSavetimeData->s1ClockDuty_clk_delay_cell[p->channel][RANK_1] = scFinal_clk_delay_cell[0];
        }
#endif

#ifdef FOR_HQA_TEST_USED
        gFinalClkDuty[p->channel] = scFinal_clk_delay_cell[0];
        gFinalClkDutyMinMax[p->channel][0] = ucFinal_period_duty_min[0];
        gFinalClkDutyMinMax[p->channel][1] = ucFinal_period_duty_max[0];
#endif
    }

    PSramPhyReset(p);

    mcSHOW_DBG_MSG("[DutyScan_Calibration_Flow] ====Done====\n");
    /*TINFO="[DutyScan_Calibration_Flow] ====Done====\n" */

    return DRAM_OK;
}

void PsramcNewDutyCalibration(DRAMC_CTX_T *p)
{
    U8 u1ChannelIdx, u1backup_channel;

#if(DQS_DUTY_SLT_CONDITION_TEST)
        U16 u2TestCnt, u2FailCnt=0, u2TestCntTotal =20; //fra 400;
        U8 u1ByteIdx, u1PI_FB;
        U32 u4Variance;
#endif
    U8 use_rev_bit=0;

    DRAM_STATUS_T u2FailStatusByCh[2]={DRAM_OK,DRAM_OK};

    //backup register value
#if FT_DSIM_USED==0
    U32 u4RegBackupAddress[] =
    {
        (DDRPHY_B0_DQ6),
        (DDRPHY_SHU1_B0_DLL1),
        (DDRPHY_SHU1_B0_DQ5),
        (DDRPHY_SHU1_B0_DQ0),
        (DDRPHY_B0_DQ0),
        (DDRPHY_B0_DQ5),
        (DDRPHY_B0_DQ8),
        (DDRPHY_SHU1_R0_B0_DQ7),
        (DDRPHY_B0_DLL_ARPI0),
        #if 1//PSRAM_SPEC
        (DDRPHY_B0_DLL_ARPI2_PSRAM),
		#endif
        (DDRPHY_B0_DLL_ARPI2),
        (DDRPHY_B1_DQ6),
        (DDRPHY_SHU1_B1_DLL1),
        (DDRPHY_SHU1_B1_DQ5),
        (DDRPHY_SHU1_B1_DQ0),
        (DDRPHY_B1_DQ0),
        (DDRPHY_B1_DQ5),
        (DDRPHY_B1_DQ8),
        (DDRPHY_SHU1_R0_B1_DQ7),
        (DDRPHY_B1_DLL_ARPI0),
        (DDRPHY_B1_DLL_ARPI2),


        (DDRPHY_CA_CMD6),
        (DDRPHY_SHU1_CA_DLL1),
        (DDRPHY_SHU1_CA_CMD5),
        (DDRPHY_SHU1_CA_CMD0),
        (DDRPHY_CA_CMD0),
        (DDRPHY_CA_CMD5),
        (DDRPHY_CA_CMD9),
//        (DDRPHY_SHU1_CA_CMD3),
        (DDRPHY_SHU1_R0_CA_CMD9),
        (DDRPHY_CA_DLL_ARPI0),
        (DDRPHY_CA_DLL_ARPI2),


#if 0
        (DDRPHY_B0_DQ6 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B0_DLL1 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B0_DQ5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B0_DQ0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B0_DQ0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B0_DQ5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B0_DQ8 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_R0_B0_DQ7 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B0_DLL_ARPI0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B0_DLL_ARPI2 + SHIFT_TO_CHB_ADDR),

        (DDRPHY_B1_DQ6 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B1_DLL1 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B1_DQ5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_B1_DQ0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B1_DQ0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B1_DQ5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B1_DQ8 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_R0_B1_DQ7 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B1_DLL_ARPI0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_B1_DLL_ARPI2 + SHIFT_TO_CHB_ADDR),


        (DDRPHY_CA_CMD6 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_CA_DLL1 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_CA_CMD5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_CA_CMD0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_CA_CMD0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_CA_CMD5 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_CA_CMD9 + SHIFT_TO_CHB_ADDR),
//        (DDRPHY_SHU1_CA_CMD3 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_SHU1_R0_CA_CMD9 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_CA_DLL_ARPI0 + SHIFT_TO_CHB_ADDR),
        (DDRPHY_CA_DLL_ARPI2 + SHIFT_TO_CHB_ADDR)
  #endif
    };
#endif

#if (fcFOR_CHIP_ID == fcSchubert)
//    if (get_Chip_Version(p) == IC_VERSION_E1) return;  //Merlot E1 can't support duty calibration
#endif

#if !FT_DSIM_USED
#if DUTY_SCAN_V2_ONLY_K_HIGHEST_FREQ
    if((p->frequency == u2DFSGetHighestFreq(p)) && (Get_PRE_MIOCK_JMETER_HQA_USED_flag()==0))
#else
    if(Get_PRE_MIOCK_JMETER_HQA_USED_flag()==0)
#endif
#endif
    {
       // if(u1IsLP4Family(p->dram_type))
        {
            U8 u1ChannelIdx;
            u1backup_channel = vGetPHY2ChannelMapping(p);

            #if SUPPORT_SAVE_TIME_FOR_CALIBRATION
            if(p->femmc_Ready==1)
            {
                for(u1ChannelIdx=CHANNEL_A; u1ChannelIdx<p->support_channel_num; u1ChannelIdx++)
                {
                    vSetPHY2ChannelMapping(p, u1ChannelIdx);
                    DramcClockDutySetClkDelayCell(p, RANK_0, p->pSavetimeData->s1ClockDuty_clk_delay_cell[p->channel][RANK_0], p->pSavetimeData->u1clk_use_rev_bit);
                  //  DramcClockDutySetClkDelayCell(p, RANK_1, p->pSavetimeData->s1ClockDuty_clk_delay_cell[p->channel][RANK_1], p->pSavetimeData->u1clk_use_rev_bit);
                    DQSDutyScan_SetDqsDelayCell(p, p->pSavetimeData->s1DQSDuty_clk_delay_cell[p->channel], p->pSavetimeData->u1dqs_use_rev_bit);
                }
                return;
            }
            else
            #endif
            {
                //Clk free run
                EnablePsramcPhyDCM(p, 0);

                for(u1ChannelIdx=CHANNEL_A; u1ChannelIdx<p->support_channel_num; u1ChannelIdx++)
                {
                    vSetPHY2ChannelMapping(p, u1ChannelIdx);

                //Fix rank to rank0
                vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), 0, RKCFG_TXRANK);
                vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), 1, RKCFG_TXRANKFIX);

                    //backup register value
                    #if FT_DSIM_USED==0
                    DramcBackupRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
                    #endif

                    DutyScan_Offset_Calibration(p);

                            #if defined(YH_SWEEP_IC)
                                if (p->channel == CHANNEL_B)
                                {
                                    u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_CLK, 0);

                                    if (gcFinal_K_Duty_clk_delay_cell[0] == 8 || gcFinal_K_Duty_clk_delay_cell[0] == -8)
                                    {
                                        u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_CLK, 1);
                                    }
                                }
                            #else
                                u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_CLK, 0);

                                if (gcFinal_K_Duty_clk_delay_cell[0] == 8 || gcFinal_K_Duty_clk_delay_cell[0] == -8)
                                {
                                    u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_CLK, 1);
                                }
                            #endif

                                u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_DQS, 0);
                                use_rev_bit=0;

                                if (gcFinal_K_Duty_clk_delay_cell[0] == 8 || gcFinal_K_Duty_clk_delay_cell[0] == -8 || gcFinal_K_Duty_clk_delay_cell[1] == 8 || gcFinal_K_Duty_clk_delay_cell[1] == -8)
                                {
                                    u2FailStatusByCh[u1ChannelIdx]= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_DQS, 1);
                                    use_rev_bit=1;
                                }
                            #if defined(YH_SWEEP_IC)
                                u2FailStatusByCh[u1ChannelIdx]|= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_DQ, 0);

                                u2FailStatusByCh[u1ChannelIdx]|= DutyScan_Calibration_Flow(p, DutyScan_Calibration_K_DQM, 0);
                            #endif

                    #if FT_DSIM_USED==0
                    //restore to orignal value
                    DramcRestoreRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
                    #endif

                    //Set K DQS MCK4X_DLY_EN and MCK4XB_DLY_EN again, this is especially for K DQS because other bit fields need to be restored.
                    DQSDutyScan_SetDqsDelayCell(p, gcFinal_K_Duty_clk_delay_cell, use_rev_bit);

                    vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), 0, RKCFG_TXRANK);
                    vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_RKCFG), 0, RKCFG_TXRANKFIX);
                }
            }

            vSetPHY2ChannelMapping(p, u1backup_channel);
        }

#if !FT_DSIM_USED
#if defined(YH_SWEEP_IC)
        YH_Sweep_IC_Print_Result(p);
        while(1); //stop here
#endif
#endif
    }
}

void PsramRxInputTrackingSetting(DRAMC_CTX_T *p)
{
	U8 u1DVS_Delay;
	//Monitor window size setting
	//DDRPHY.SHU*_B*_DQ5.RG_RX_ARDQS0_DVS_DLY_B* (suggested value from A-PHY owner)
//WHITNEY_TO_BE_PORTING
	//			Speed	Voltage 	DVS_DLY
	//======================================
	//SHU1		3200	0.8V		3
	//SHU2		2667	0.8V-0.7V	4
	//SHU3		1600	0.7V-0.65V	5
    if(p->freqGroup == 1333 || p->freqGroup == 1200)
	{
		u1DVS_Delay =4;
	}
	else// if(p->freqGroup == 800)
	{
		u1DVS_Delay =5;
	}

	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ5), u1DVS_Delay, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B1_DQ5), u1DVS_Delay, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);

	/* Bianco HW design issue: run-time PBYTE flag will lose it's function and become per-bit -> set to 0 */
	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7_PSRAM), P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM)
															| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM));

	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_SHU1_B0_DQ7), P_Fld(0x0, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0)
															| P_Fld(0x0, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0));

	//Enable RX_FIFO macro DIV4 clock CG
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_MISC_CG_CTRL1), 0xffffffff, MISC_CG_CTRL1_R_DVS_DIV4_CG_CTRL);

	//DVS mode to RG mode
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2),0x0, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2),0x0, R0_B1_RXDVS2_R_RK0_DVS_MODE_B1);

//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R1_B0_RXDVS2),0x0, R1_B0_RXDVS2_R_RK1_DVS_MODE_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R1_B1_RXDVS2),0x0, R1_B1_RXDVS2_R_RK1_DVS_MODE_B1);

	//Tracking lead/lag counter >> Rx DLY adjustment fixed to 1
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_RXDVS0),0x0, B0_RXDVS0_R_DMRXDVS_CNTCMP_OPT_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_RXDVS0),0x0, B1_RXDVS0_R_DMRXDVS_CNTCMP_OPT_B1);

	//DQIEN pre-state option to block update for RX ASVA  1-2
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_RXDVS0),0x1, B0_RXDVS0_R_DMRXDVS_DQIENPRE_OPT_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_RXDVS0),0x1, B1_RXDVS0_R_DMRXDVS_DQIENPRE_OPT_B1);

	//Turn off F_DLY individual calibration option (CTO_AGENT_RDAT cannot separate DR/DF error)
	//tracking rising and update rising/falling together
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2),0x0, R0_B0_RXDVS2_R_RK0_DVS_FDLY_MODE_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2),0x0, R0_B1_RXDVS2_R_RK0_DVS_FDLY_MODE_B1);

//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R1_B0_RXDVS2),0x0, R1_B0_RXDVS2_R_RK1_DVS_FDLY_MODE_B0);
//	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_R1_B1_RXDVS2),0x0, R1_B1_RXDVS2_R_RK1_DVS_FDLY_MODE_B1);

	    //DQ/DQM/DQS DLY MAX/MIN value under Tracking mode
    /* Byte 0 */
    /* DQS, DQ, DQM (DQ, DQM are tied together now) -> controlled using DQM MAX_MIN */
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS7), P_Fld(0x0, R0_B0_RXDVS7_RG_RK0_ARDQ_MIN_DLY_B0) | P_Fld(0x3f, R0_B0_RXDVS7_RG_RK0_ARDQ_MAX_DLY_B0)
                                                              | P_Fld(0x0, R0_B0_RXDVS7_RG_RK0_ARDQS0_MIN_DLY_B0) | P_Fld(0x7f, R0_B0_RXDVS7_RG_RK0_ARDQS0_MAX_DLY_B0));
    //Threshold for LEAD/LAG filter
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS1), P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LEAD) | P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LAG));
//    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS1), P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LEAD) | P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LAG));

    //DQ/DQS Rx DLY adjustment for tracking mode
    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2), P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_DQ_SCALE_B0) | P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_DQS_SCALE_B0));
//    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2), P_Fld(0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_RIS_DQ_SCALE_B1) | P_Fld(0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_RIS_DQS_SCALE_B1));

    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2), P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_DQ_SCALE_B0) | P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_DQS_SCALE_B0));
//    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B1_RXDVS2), P_Fld(0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_FAL_DQ_SCALE_B1) | P_Fld(0x0, R0_B1_RXDVS2_R_RK0_RX_DLY_FAL_DQS_SCALE_B1));



	//Rx DLY tracking setting (Static)
	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_B0_RXDVS0),
	                                   P_Fld(1, B0_RXDVS0_R_RX_DLY_TRACK_SPM_CTRL_B0) |
	                                   P_Fld(0, B0_RXDVS0_R_RX_RANKINCTL_B0)|
	                                   P_Fld(1, B0_RXDVS0_R_RX_RANKINSEL_B0));

	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(0x1, B0_DQ9_PSRAM_R_DMRXDVS_RDSEL_LAT_B0_PSRAM| P_Fld(0, B0_DQ9_PSRAM_R_DMRXDVS_VALID_LAT_B0_PSRAM)));
	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(0x1, B0_DQ9_R_DMRXDVS_RDSEL_LAT_B0 | P_Fld(0, B0_DQ9_R_DMRXDVS_VALID_LAT_B0)));

	//   vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(0x1, B1_DQ9_R_DMRXDVS_RDSEL_LAT_B1) | P_Fld(0, B1_DQ9_R_DMRXDVS_VALID_LAT_B1));
	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_CA_CMD10), P_Fld(0,CA_CMD10_R_DMRXDVS_RDSEL_LAT_CA) | P_Fld(0, CA_CMD10_R_DMRXDVS_VALID_LAT_CA));

	/* DMRXTRACK_DQM_B* (rxdly_track SM DQM enable) -> need to be set to 1 if R_DBI is on
	*  They are shuffle regs -> move setting to DramcSetting_Olympus_LP4_ByteMode()
	*/

	//Enable A-PHY DVS LEAD/LAG
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_DQ5), 0x0, B0_DQ5_RG_RX_ARDQS0_DVS_EN_B0);
	//   vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_DQ5), 0x1, B1_DQ5_RG_RX_ARDQS0_DVS_EN_B1);

	//Rx DLY tracking function CG enable
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_RXDVS0), 0x1, B0_RXDVS0_R_RX_DLY_TRACK_CG_EN_B0);
	//    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_RXDVS0), 0x1, B1_RXDVS0_R_RX_DLY_TRACK_CG_EN_B1);

	//Rx DLY tracking lead/lag counter enable
	vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B0_RXDVS0), 0x1, B0_RXDVS0_R_RX_DLY_TRACK_ENA_B0);
	//    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DDRPHY_B1_RXDVS0), 0x1, B1_RXDVS0_R_RX_DLY_TRACK_ENA_B1);


	//Rx DLY tracking update enable (HW mode)
	vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DDRPHY_R0_B0_RXDVS2),
	                                       P_Fld(2, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0) |
	                                       P_Fld(1, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0)|
	                                       P_Fld(1, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0));

}

#endif

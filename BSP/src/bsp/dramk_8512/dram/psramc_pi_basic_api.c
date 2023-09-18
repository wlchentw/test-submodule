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
 * $RCSfile: pi_basic_api.c,v $
 * $Revision: #1 $
 *
 *---------------------------------------------------------------------------*/

/** @file pi_basic_api.c
 *  Basic PSRAMC API implementation
 */

#ifdef SUPPORT_TYPE_PSRAM
//-----------------------------------------------------------------------------
// Include files
//-----------------------------------------------------------------------------
//#include "..\Common\pd_common.h"
//#include "Register.h"
#if __ETT__
#include "processor.h"
#endif
#include <platform/dramc_common.h>
#include <platform/emi.h>
#include <platform/x_hal_io.h>
#if (FOR_DV_SIMULATION_USED==0)
#include <platform/pll.h>
#endif

typedef struct _ACTimePsram_T
{
    U8 dramType;
    U16 freq;
    U8 readLat, writeLat;
    U8 dqsinctl, datlat; //DQSINCTL, DATLAT aren't in ACTiming excel file

    //CE_CNT0===================================
    U16 tce_mrw :Fld_wid(CE_CNT0_TCE_MRW);
    U16 tce_hsleepx :Fld_wid(CE_CNT0_TCE_HSLEEPX);
    U16 tce_wr :Fld_wid(CE_CNT0_TCE_WR);
    U16 tce_rd :Fld_wid(CE_CNT0_TCE_RD);

    //SHU_ACTIM0 ===================================
    U16 tcem :Fld_wid(SHU_ACTIM0_TCEM);
    U16 txhs :Fld_wid(SHU_ACTIM0_TXHS);
    U16 trc :Fld_wid(SHU_ACTIM0_TRC);
    U16 trfc :Fld_wid(SHU_ACTIM0_TRFC);

    //SHU_ACTIM1 ===================================
    U16 tcphw :Fld_wid(SHU_ACTIM1_TCPHW);
    U16 tcphr_l :Fld_wid(SHU_ACTIM1_TCPHR_L);
    U16 tcphr_s :Fld_wid(SHU_ACTIM1_TCPHR_S);

    //SHU_ACTIM2 ===================================
    U16 tmrw :Fld_wid(SHU_ACTIM2_TMRW);

	//REFCNT=======================================
	U16 trefcnt_fr_clk1:Fld_wid(REFCNT_FR_CLK1_REFCNT_FR_CLK_1X);
	U16 trefcnt_fr_clk2:Fld_wid(REFCNT_FR_CLK1_REFCNT_FR_CLK_2X);

    //SHU_ACTIM3 ===================================
    U16 tzqcs :Fld_wid(SHU_ACTIM3_TZQCS);
    U16 tzqcl :Fld_wid(SHU_ACTIM3_TZQCL);

    //SHU_ACTIM4 ===================================
    U16 txrefcnt :Fld_wid(SHU_ACTIM4_TXREFCNT);

     //CE_ CNT_05T ===================================
	U16 tce_mrw_05T :Fld_wid(CE_CNT_05T_TCE_MRW_05T);
	U16 tce_hsleepx_05T :Fld_wid(CE_CNT_05T_TCE_HSLEEPX_05T);
	U16 tce_wr_05T :Fld_wid(CE_CNT_05T_TCE_WR_05T);
	U16 tce_rd_05T :Fld_wid(CE_CNT_05T_TCE_RD_05T);

	//AC_TIMEING_05T ===================================
    U16 tmrw_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TMRW_05T);
    U16 tcphw_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHW_05T);
    U16 tcphr_l_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHR_L_05T);
    U16 tcphr_s_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCPHR_S_05T);
    U16 tcem_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TCEM_05T);
    U16 txhs_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TXHS_05T);
    U16 trc_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TRC_05T);
    U16 trfc_05T :Fld_wid(PSRAMC_SHU_AC_TIME_05T_TRFC_05T);

	//AC_TIMEING_05T_B ===================================
    U16 tzqcs_05T :Fld_wid(SHU_AC_TIME_05T_B_TZQCS_05T);
    U16 tzqcl_05T :Fld_wid(SHU_AC_TIME_05T_B_TZQCL_05T);

} ACTimePsram_T;

const ACTimePsram_T ACTimingPsramTbl[2] =
{
	//PSRAM 1600/////
	{
		.dramType = TYPE_PSRAM, .freq = 1600,
		//.readLat = 40,	.writeLat = 16,
		.tce_mrw = 1,
		.tce_wr = 5, .tce_rd = 10,
		.tce_hsleepx = 20,
		.tcem = 0,   .trfc = 12,
		.trc = 12,   .txhs = 20,
		.tcphr_l = 14,   .tcphr_s = 11,
		.tcphw = 10,	.tmrw = 20,
		.tzqcl = 200,	.tzqcs = 100,
		.txrefcnt = 20,

		.trefcnt_fr_clk1 = 101,
		.trefcnt_fr_clk2 = 202,

		.tce_wr_05T = 0,	.tce_rd_05T = 0,
		.tce_hsleepx_05T = 1,

		.tcem_05T = 1,	.trfc_05T = 0,
		.trc_05T = 0,	.txhs_05T = 0,

		.tcphr_l_05T = 0,	.tcphr_s_05T = 0,
		.tcphw_05T = 1, .tmrw_05T = 1,

		.tzqcl_05T = 0,	.tzqcs_05T = 0

		//DQSINCTL, DATLAT aren't in ACTiming excel file
		//.dqsinctl = 7,	 .datlat = 18
	},

	// PSRAM 3200/////
	{
		.dramType = TYPE_PSRAM, .freq = 2133,
		//.readLat = 40,	.writeLat = 16,

		.tce_mrw = 1,
		.tce_wr = 6, .tce_rd = 12,
		.tce_hsleepx = 26,
		.tcem = 0,   .trfc = 16,
		.trc = 16,   .txhs = 26,
		.tcphr_l = 17,   .tcphr_s = 13,
		.tcphw = 13,	.tmrw = 26,
		.tzqcl = 261,	.tzqcs = 130,
		.txrefcnt = 25,

		.trefcnt_fr_clk1 = 101,
		.trefcnt_fr_clk2 = 202,

		.tce_wr_05T = 0,	.tce_rd_05T = 0,
		.tce_hsleepx_05T = 1,

		.tcem_05T = 1,	.trfc_05T = 0,
		.trc_05T = 0,	.txhs_05T = 1,

		.tcphr_l_05T = 1,	.tcphr_s_05T = 1,
		.tcphw_05T = 1, .tmrw_05T = 1,

		.tzqcl_05T = 0,	.tzqcs_05T = 1

		//DQSINCTL, DATLAT aren't in ACTiming excel file
		//.dqsinctl = 7,	 .datlat = 18
	},
};

const U8 uiPSRAM_DQ_Mapping_POP[CHANNEL_NUM][8] =
{
    //CH-A
    {
        6, 7, 4, 5, 2, 3, 0, 1
    },
};

//MRR PSRAM->PSRAMC
static const U8 uiPSRAM_MRR_Mapping_POP[CHANNEL_NUM][8] =
{
    //CH-A
    {
        0, 1, 2, 3, 4, 5, 6, 7
        //6, 7, 4, 5, 2, 3, 0, 1
    },
};

static void Set_PSRAM_MRR_Pinmux_Mapping(DRAMC_CTX_T *p)
{
    U8 *uiPSRAM_MRR_Mapping = NULL;
    U8 backup_channel;
    DRAM_CHANNEL_T chIdx = CHANNEL_A;

    //Backup channel & broadcast
    backup_channel = vGetPHY2ChannelMapping(p);
    //backup_broadcast = GetDramcBroadcast();

   // DramcBroadcastOnOff(DRAMC_BROADCAST_OFF); //Disable broadcast

    for(chIdx = CHANNEL_A; chIdx < p->support_channel_num; chIdx++)
    {
        vSetPHY2ChannelMapping(p, chIdx);

        //Set MRR pin mux
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_MRR_BIT_MUX1), P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][0], MRR_BIT_MUX1_MRR_BIT0_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][1], MRR_BIT_MUX1_MRR_BIT1_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][2], MRR_BIT_MUX1_MRR_BIT2_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][3], MRR_BIT_MUX1_MRR_BIT3_SEL));
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_MRR_BIT_MUX2), P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][4], MRR_BIT_MUX2_MRR_BIT4_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][5], MRR_BIT_MUX2_MRR_BIT5_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][6], MRR_BIT_MUX2_MRR_BIT6_SEL)
                                    | P_Fld(uiPSRAM_MRR_Mapping_POP[chIdx][7], MRR_BIT_MUX2_MRR_BIT7_SEL));
    }

    //Recover channel & broadcast
    vSetPHY2ChannelMapping(p, backup_channel);
    //DramcBroadcastOnOff(backup_broadcast);
}

#ifdef DUMMY_READ_FOR_TRACKING
void PsramcDummyReadAddressSetting(DRAMC_CTX_T *p)
{
#if 1//ndef CERVINO_TO_BE_PORTING
    U8 backup_channel= p->channel, backup_rank= p->rank;
    U8 channelIdx, rankIdx;
    dram_addr_t dram_addr;

    for (channelIdx=CHANNEL_A; channelIdx<CHANNEL_NUM; channelIdx++)
    {
        vSetPHY2ChannelMapping(p, channelIdx);
        for (rankIdx=RANK_0; rankIdx<p->support_rank_num; rankIdx++)
        {
            vSetRank(p, rankIdx);

            dram_addr.ch = channelIdx;
            dram_addr.rk = rankIdx;
            dram_addr.col = 0;
            dram_addr.row = 0;
            dram_addr.bk = 0;
            //get_dummy_read_addr(&dram_addr);
            mcSHOW_DBG_MSG3("=== dummy read address: CH_%d, RK%d, row: 0x%x, bk: %d, col: 0x%x\n\n",
                    channelIdx,rankIdx,dram_addr.row,dram_addr.bk,dram_addr.col);

            vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_RK_DUMMY_RD_ADR), P_Fld(dram_addr.col, RK_DUMMY_RD_ADR_DMY_RD_COL_ADR)
                                | P_Fld(0, RK_DUMMY_RD_ADR_DMY_RD_LEN));
            vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_RK_DUMMY_RD_ADR2), P_Fld(dram_addr.bk, RK_DUMMY_RD_ADR2_DMY_RD_BK)
                                | P_Fld(dram_addr.row, RK_DUMMY_RD_ADR2_DMY_RD_ROW_ADR));
        }
    }

    vSetPHY2ChannelMapping(p, backup_channel);
    vSetRank(p, backup_rank);
#endif
}

void PsramcDummyReadForTrackingEnable(DRAMC_CTX_T *p)
{
    vIO32WriteFldAlign_All(PSRAMC_REG_DUMMY_RD, 1, DUMMY_RD_RANK_NUM);
    /* Dummy read pattern (Better efficiency during rx dly tracking) DE: YH Tsai, Wei-jen */
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA0, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA0_DMY_RD_RK0_WDATA0
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA1, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA1_DMY_RD_RK0_WDATA1
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA2, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA2_DMY_RD_RK0_WDATA2
    vIO32Write4B_All(PSRAMC_REG_RK_DUMMY_RD_WDATA3, 0xAAAA5555); // Field RK0_DUMMY_RD_WDATA3_DMY_RD_RK0_WDATA3

    //vIO32WriteFldAlign_All(PSRAMC_REG_TEST2_A4, 4, TEST2_A4_TESTAGENTRKSEL);//Dummy Read rank selection is controlled by Test Agent

#if 0//__ETT__
    /* indicate ROW_ADR = 2 for Dummy Write pattern address, in order to avoid pattern will be overwrited by MEM_TEST(test range 0xffff)
     * Pattern locates: 0x40010000, 0x40010100, 0x80010000, 0x80010100 */
    dram_addr_t dram_addr;

    dram_addr.ch = 0;
    dram_addr.rk = 0;
    get_dummy_read_addr(&dram_addr);

    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DRAMC_REG_RK0_DUMMY_RD_ADR), P_Fld(dram_addr.row, RK0_DUMMY_RD_ADR_DMY_RD_RK0_ROW_ADR)
                        | P_Fld(dram_addr.col, RK0_DUMMY_RD_ADR_DMY_RD_RK0_COL_ADR)
                        | P_Fld(0, RK0_DUMMY_RD_ADR_DMY_RD_RK0_LEN));
    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DRAMC_REG_RK0_DUMMY_RD_BK), dram_addr.bk, RK0_DUMMY_RD_BK_DMY_RD_RK0_BK);

    dram_addr.rk = 1;
    get_dummy_read_addr(&dram_addr);

    vIO32WriteFldMulti_All(DRAMC_REG_ADDR(DRAMC_REG_RK1_DUMMY_RD_ADR), P_Fld(dram_addr.row, RK1_DUMMY_RD_ADR_DMY_RD_RK1_ROW_ADR)
                        | P_Fld(dram_addr.col, RK1_DUMMY_RD_ADR_DMY_RD_RK1_COL_ADR)
                        | P_Fld(0, RK1_DUMMY_RD_ADR_DMY_RD_RK1_LEN));
    vIO32WriteFldAlign_All(DRAMC_REG_ADDR(DRAMC_REG_RK1_DUMMY_RD_BK), dram_addr.bk, RK1_DUMMY_RD_BK_DMY_RD_RK1_BK);

    /* trigger dummy write pattern 0xAAAA5555 */
    vIO32WriteFldAlign_All(DRAMC_REG_DUMMY_RD, 0x1, DUMMY_RD_DMY_WR_DBG);
    vIO32WriteFldAlign_All(DRAMC_REG_DUMMY_RD, 0x0, DUMMY_RD_DMY_WR_DBG);
#else
    PsramcDummyReadAddressSetting(p);
#endif

    /* DUMMY_RD_RX_TRACK = 1:
     * During "RX input delay tracking enable" and "DUMMY_RD_EN=1" Dummy read will force a read command to a certain rank,
     * ignoring whether or not EMI has executed a read command to that certain rank in the past 4us.
     */
    if(p->frequency > 800)
    {
        vIO32WriteFldAlign_All(PSRAMC_REG_DUMMY_RD, 1, DUMMY_RD_DUMMY_RD_EN);
        mcSHOW_DBG_MSG("High Freq DUMMY_READ_FOR_TRACKING: ON\n");
    }
    else
    {
        mcSHOW_DBG_MSG("Low Freq DUMMY_READ_FOR_TRACKING: OFF\n");
    }
    return;
}

#endif


void Psram_Global_Option_Init2(DRAMC_CTX_T *p)
{
    U8 u1rank_num=0;

#if(FOR_DV_SIMULATION_USED==0 && SW_CHANGE_FOR_SIMULATION==0)
    EMI_SETTINGS *emi_set;

	emi_set = get_emi_setting();

    mcSHOW_DBG_MSG("Rank info CONA[0x%x]\n", emi_set->EMI_CONA_VAL);

#if 0//(fcFOR_CHIP_ID == fcSchubert)
	if (emi_set->PIN_MUX_TYPE) {
		p->bDLP3 = 1;
		mcSHOW_DBG_MSG("\n\nusing DLP3\n\n");
	}
#endif

    p->vendor_id = emi_set->iLPDDRX_MODE_REG_5;

    u1rank_num = ((emi_set->EMI_CONA_VAL >> 17) & 0x1)==1 ? 0 : 1;
#endif
    //SetRankInfoToConf(p, u1rank_num);
    //vSetRankNumber(p);
	Set_PSRAM_MRR_Pinmux_Mapping(p);
}

void vPsramcInit_PreSettings(DRAMC_CTX_T *p)
{
#if 0//PSRAM_SPEC
    vIO32WriteFldMulti(DDRPHY_CA_CMD8, P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_UP) | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_DN)
								 | P_Fld(0x1, CA_CMD8_RG_TX_RRESETB_DDR3_SEL) | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_DDR4_SEL)
								 | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVP) | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVN));
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1_PSRAM, 0x1, MISC_CTRL1_PSRAM_R_DMRRESETB_I_OPT_PSRAM); //Change to glitch-free path
    //replace DDRCONF0_GDDR3RST with MISC_CTRL1_R_DMDA_RRESETB_I
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1_PSRAM, 0x0, MISC_CTRL1_PSRAM_R_DMDA_RRESETB_I_PSRAM);
    vIO32WriteFldAlign(DDRPHY_MISC_CTRL1_PSRAM, 0x1, MISC_CTRL1_PSRAM_R_DMDA_RRESETB_E_PSRAM);
#endif
	/* PAD_RRESETB control sequence */
	//remove twice dram reset pin pulse before dram power on sequence flow
	vIO32WriteFldMulti(DDRPHY_CA_CMD8, P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_UP) | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_PULL_DN)
	                                 | P_Fld(0x1, CA_CMD8_RG_TX_RRESETB_DDR3_SEL) | P_Fld(0x0, CA_CMD8_RG_TX_RRESETB_DDR4_SEL)
	                                 | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVP) | P_Fld(0xa, CA_CMD8_RG_RRESETB_DRVN));
	vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMRRESETB_I_OPT); //Change to glitch-free path
	//replace DDRCONF0_GDDR3RST with MISC_CTRL1_R_DMDA_RRESETB_I
	vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMDA_RRESETB_I);
	vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x1, MISC_CTRL1_R_DMDA_RRESETB_E);

    return;
}

void PsramPhyFreqSel(DRAMC_CTX_T *p, DRAM_PLL_FREQ_SEL_T sel)
{
    p->freq_sel = sel;

    switch(p->freq_sel)
    {
        case PSRAM_2133:
            p->frequency=1066;
            break;
        case PSRAM_1600:
            p->frequency=800;
            break;
        default:
            p->frequency=800;
            break;
    }

	if (p->frequency <= 800) // DDR1600, DDR800
	{
		p->freqGroup = 800;
	}
	else if (p->frequency <= 1200) //DDR2133 DDR2280
	{
		p->freqGroup = 1200;
	}
	else
	{
        p->freqGroup = 800;
	}

	gu4TermFreq = 1200; //This is for TX per BIT k used
    mcSHOW_DBG_MSG3("[setFreqGroup] p-> frequency %u, freqGroup: %u\n", p->frequency, p->freqGroup);

    //p->shu_type = get_shuffleIndex_by_Freq(p);
    //setFreqGroup(p); /* Set p->freqGroup to support freqs not in ACTimingTable */

}

void PSramcEngine2SetPat(DRAMC_CTX_T *p, U8 testaudpat, U8 log2loopcount, U8 Use_Len1_Flag)
{

	if ((Use_Len1_Flag != 0)&&(testaudpat != TEST_AUDIO_PATTERN))
	{
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
			 P_Fld(1, TEST2_A4_TEST_REQ_LEN1));   //test agent 2 with cmd length = 0
	}
	else
	{
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
			 P_Fld(0, TEST2_A4_TEST_REQ_LEN1));   //test agent 2 with cmd length = 0
	}

	if (testaudpat == TEST_XTALK_PATTERN)   // xtalk
    {
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			 P_Fld(0, TEST2_A3_TESTAUDPAT) |
			 P_Fld(0, TEST2_A3_AUTO_GEN_PAT) |
			 P_Fld(0, TEST2_A3_HFIDPAT) |
			 P_Fld(0, TEST2_A3_TEST_AID_EN));
        // select XTALK pattern
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
			 P_Fld(0, TEST2_A4_TESTDMITGLPAT) |
			 P_Fld(1, TEST2_A4_TESTXTALKPAT)); //dont use audio pattern

    }
    else if (testaudpat == TEST_AUDIO_PATTERN)   // audio
    {
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
			 P_Fld(0, TEST2_A4_TESTAUDBITINV) |
			 P_Fld(0, TEST2_A4_TESTXTALKPAT));
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			 P_Fld(1, TEST2_A3_TESTAUDPAT) |
			 P_Fld(0, TEST2_A3_AUTO_GEN_PAT) |
			 P_Fld(0, TEST2_A3_HFIDPAT) |
			 P_Fld(0, TEST2_A3_TEST_AID_EN));
    }
    else   // ISI
    {
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4),
			 P_Fld(0, TEST2_A4_TESTDMITGLPAT) |
			 P_Fld(0, TEST2_A4_TESTXTALKPAT));
        // select ISI pattern
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			 P_Fld(0, TEST2_A3_TESTAUDPAT) |
			 P_Fld(1, TEST2_A3_AUTO_GEN_PAT) |
			 P_Fld(1, TEST2_A3_HFIDPAT) |
			 P_Fld(1, TEST2_A3_TEST_AID_EN));
    }
}

 DRAM_STATUS_T PSramcEngine2Init(DRAMC_CTX_T *p, U32 u4Base, U32 u4Offset, U8 testaudpat, U8 log2loopcount)
{
    U8 Use_Len1_Flag;

    // error handling
    if (!p)
    {
        mcSHOW_ERR_MSG("context is NULL\n");
        return DRAM_FAIL;
    }

    // check loop number validness
//    if ((log2loopcount > 15) || (log2loopcount < 0))		// U8 >=0 always.
    if (log2loopcount > 15)
    {
        mcSHOW_ERR_MSG("wrong param: log2loopcount > 15\n");
        return DRAM_FAIL;
    }

    Use_Len1_Flag = (testaudpat & 0x80) >> 7;   //len1 mapping to (testpattern + 8) or testpattern
    testaudpat = testaudpat & 0x7f;

	/////attention1 lzs mark here below are test agents setting, maybe should move into K flow code
		vIO32WriteFldAlign(PSRAMC_REG_TEST2_A0, 0x1, TEST2_A0_WDT_BY_DRAMC_DIS);
		vIO32WriteFldMulti(PSRAMC_REG_TEST2_A3, P_Fld(0x1, TEST2_A3_TEST2WREN2_HW_EN)
					| P_Fld(0x1, TEST2_A3_TESTCLKRUN));
		//vIO32WriteFldMulti(PSRAMC_REG_TEST2_A4, P_Fld(0x0, TEST2_A4_TESTXTALKPAT)
		//			| P_Fld(0x11, TEST2_A4_TESTAUDINIT));
		//vIO32WriteFldAlign(PSRAMC_REG_RK_TEST2_A1, 0x10000, RK_TEST2_A1_TEST2_BASE);
	////////end attention1 /////////////////////////


    // 1.set pattern ,base address ,offset address
    // 2.select  ISI pattern or audio pattern or xtalk pattern
    // 3.set loop number
    // 4.enable read or write
    // 5.loop to check DM_CMP_CPT
    // 6.return CMP_ERR
    // currently only implement ucengine_status = 1, others are left for future extension

    // 1
   // vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A4), P_Fld(test2_1>>24,TEST2_A4_TESTXTALKPAT)|P_Fld(test2_2>>24,TEST2_0_TEST2_PAT1));

#if (FOR_DV_SIMULATION_USED==1 || SW_CHANGE_FOR_SIMULATION==1)
    //DV sim memory 0~0x100 has values, can't used
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), (u4Base) & 0x00ffffff, RK_TEST2_A1_TEST2_BASE);
#else
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), (u4Base+0x200000) & 0x00ffffff, RK_TEST2_A1_TEST2_BASE);
#endif
    vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A2), u4Offset & 0x00ffffff, TEST2_A2_TEST2_OFF);  //lzs,  this should set 0x20 after K

    // 2 & 3
    // (TESTXTALKPAT, TESTAUDPAT) = 00 (ISI), 01 (AUD), 10 (XTALK), 11 (UNKNOWN)
    PSramcEngine2SetPat(p, testaudpat, log2loopcount, Use_Len1_Flag);

    return DRAM_OK;
}

 void PSramcEngine2CheckComplete(DRAMC_CTX_T *p)
{
    U32 u4loop_count = 0;

    while(u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TESTRPT), TESTRPT_DM_CMP_CPT_RK0) != 1)
    {
        mcDELAY_US(1);
        u4loop_count++;
        if ((u4loop_count > 3) && (u4loop_count <= MAX_CMP_CPT_WAIT_LOOP))
        {
           // mcSHOW_ERR_MSG("PSRAMC_REG_TESTRPT: %d\n", u4loop_count);
        }
        else if(u4loop_count > MAX_CMP_CPT_WAIT_LOOP)
        {
            /*TINFO="fcWAVEFORM_MEASURE_A %d: time out\n", u4loop_count*/
            mcSHOW_DBG_MSG("PSramcEngine2CheckComplete %d :time out\n", u4loop_count);
            mcFPRINTF(fp_A60501, "PSramcEngine2CheckComplete %d: time out\n", u4loop_count);
            break;
        }
    }
}

U32 PSramcEngine2Compare(DRAMC_CTX_T *p, DRAM_TE_OP_T wr)
{
	U32 u4result = 0xffffffff;
	U8  u1status = 1; //RK0

	if (wr == TE_OP_WRITE_READ_CHECK)
	{
	 	// read data compare ready check
	 	PSramcEngine2CheckComplete(p);

	 	// disable write
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			 P_Fld(0, TEST2_A3_TEST2W) |
			 P_Fld(0, TEST2_A3_TEST2R) |
			 P_Fld(0, TEST2_A3_TEST1));

		 mcDELAY_US(1);

		 // enable read
		 vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			 P_Fld(0, TEST2_A3_TEST2W) |
			 P_Fld(1, TEST2_A3_TEST2R) |
			 P_Fld(0, TEST2_A3_TEST1));
	}

	// 5
	// read data compare ready check
	PSramcEngine2CheckComplete(p);

	// delay 10ns after ready check from DE suggestion (1ms here)
	//mcDELAY_US(1);

	u4result = (u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_TESTRPT)) >> 4) & u1status; //CMP_ERR_RK0

 return u4result;
}

U32 PSramcEngine2Run(DRAMC_CTX_T *p, DRAM_TE_OP_T wr, U8 testaudpat)
{
    U32 u4result = 0xffffffff;

    // 4
    if (wr == TE_OP_READ_CHECK)
    {
        // enable read,
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W) | P_Fld(1, TEST2_A3_TEST2R));
    }
    else if (wr == TE_OP_WRITE_READ_CHECK)
    {
        // enable write
        vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(1, TEST2_A3_TEST2W) | P_Fld(0, TEST2_A3_TEST2R));
    }
	//5
    PSramcEngine2CheckComplete(p);

    // delay 10ns after ready check from DE suggestion (1ms here)
    mcDELAY_US(1);

    // 6
    // return CMP_ERR, 0 is ok ,others are fail,diable test2w or test2r
    // get result
    // or all result
    u4result = (u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_CMP_ERR)));
    // disable read
    vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W) | P_Fld(0, TEST2_A3_TEST2R));

    return ((u4result >> 8) | (u4result & 0xff));
}

 void PSramcEngine2End(DRAMC_CTX_T *p)
{
    vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3), P_Fld(0, TEST2_A3_TEST2W));
}


static U32 TestPSramEngineCompare(DRAMC_CTX_T *p)
{
    U8 jj;
    U32 u4err_value;

	//  if(p->test_pattern <= TEST_XTALK_PATTERN)
	PSramcEngine2Init(p, 0, 0x1,TEST_AUDIO_PATTERN, 0);
	u4err_value = PSramcEngine2Run(p, TE_OP_READ_CHECK, TEST_AUDIO_PATTERN);
	PSramcEngine2End(p);
	mcSHOW_ERR_MSG("TEST AUDIO PATTERN result: %d\n",u4err_value);

	PSramcEngine2Init(p, 0, 0x23,TEST_XTALK_PATTERN, 0);
	u4err_value = PSramcEngine2Run(p, TE_OP_WRITE_READ_CHECK, TEST_XTALK_PATTERN);
	PSramcEngine2End(p);
	mcSHOW_ERR_MSG("TEST_XTALK_PATTERN result: %d\n",u4err_value);


	PSramcEngine2Init(p, 0, 0x23,TEST_ISI_PATTERN, 0);
	u4err_value = PSramcEngine2Run(p, TE_OP_READ_CHECK, TEST_ISI_PATTERN);
	PSramcEngine2End(p);
	mcSHOW_ERR_MSG("TEST_ISI_PATTERN result: %d\n",u4err_value);

    return u4err_value;
}

void PSramc_TA2_Test_Run_Time_HW_Set_Column_Num(DRAMC_CTX_T * p)
{
	U8 u1ChannelIdx = 0, shu_index;
	U32 u4matypeR0 = 0, u4matypeR1 = 0;
	U32 u4matype = 0;
	DRAM_CHANNEL_T eOriChannel = p->channel;

	for(u1ChannelIdx = 0; u1ChannelIdx<p->support_channel_num; u1ChannelIdx++)
	{
		vSetPHY2ChannelMapping(p, u1ChannelIdx);
		u4matype = u4IO32Read4B(EMI_APB_BASE);
		u4matypeR0 = ((u4matype>>(4+u1ChannelIdx*16))&0x3)+1; //refer to init_ta2_single_channel()
		u4matypeR1 = ((u4matype>>(6+u1ChannelIdx*16))&0x3)+1; //refer to init_ta2_single_channel()

		if(p->support_rank_num==RANK_SINGLE)
		{
			u4matype = u4matypeR0;
		}
		else //dual rank
		{
			u4matype = (u4matypeR0 > u4matypeR1) ? u4matypeR1 : u4matypeR0; //get min value
		}

		for(shu_index = DRAM_DFS_SHUFFLE_1; shu_index < DRAM_DFS_SHUFFLE_MAX; shu_index++)
			vIO32WriteFldAlign(DRAMC_REG_ADDR(DRAMC_REG_SHU_CONF0) + SHU_GRP_DRAMC_OFFSET*shu_index, u4matype, SHU_CONF0_MATYPE);
	}
	vSetPHY2ChannelMapping(p, eOriChannel);

	return;
}

#define TA2_RANK0_ADDRESS   (0x40000000)
void PSramc_TA2_Test_Run_Time_HW_Presetting(DRAMC_CTX_T * p, U32 len)
{
	U32 u4BaseR0, u4Offset, u4Addr;

	u4BaseR0 = TA2_RANK0_ADDRESS & 0x1fffffff;

	//u4BaseR0 = u4Addr >> 4;
	u4Offset = (len >> 4);//16B per pattern //len should be >>2 or test engine will time out
	u4Offset = (u4Offset == 0) ? 1 : u4Offset;  //halt if u4Offset = 0
	mcSHOW_DBG_MSG("=== TA2 HW\n");

	vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_RK_TEST2_A1), u4BaseR0, RK_TEST2_A1_TEST2_BASE);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A2), u4Offset, TEST2_A2_TEST2_OFF);

	//TA2_Test_Run_Time_HW_Set_Column_Num(p);

	return;
}

#define TA2_PAT TEST_XTALK_PATTERN
void PSramc_TA2_Test_Run_Time_Pat_Setting(DRAMC_CTX_T *p, U8 PatSwitch)
{
	static U8 u1Pat = TA2_PAT, u1loop = 1;
	U8 u1ChannelIdx = 0;
	DRAM_CHANNEL_T eOriChannel = p->channel;


	if (u1loop || (PatSwitch == TA2_PAT_SWITCH_ON))
	{
		mcSHOW_DBG_MSG("TA2 PAT: %s\n",
		(u1Pat == TEST_XTALK_PATTERN) ? "XTALK" : (u1Pat == TEST_AUDIO_PATTERN) ? "AUDIO" : "WSI");
		for (u1ChannelIdx = CHANNEL_A; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++)
		{
			p->channel = u1ChannelIdx;
			PSramcEngine2SetPat(p, u1Pat, p->support_rank_num - 1, 0);
		}
		p->channel = eOriChannel;
		if (PatSwitch)
			u1Pat = (u1Pat + 1) % 3;
		else
			u1loop = 0;
	}
	return;
}

void PSramc_TA2_Test_Run_Time_HW_Write(DRAMC_CTX_T * p, U8 u1Enable)
{
	DRAM_CHANNEL_T eOriChannel = p->channel;
	U8 u1ChannelIdx;

	for(u1ChannelIdx =0; u1ChannelIdx<p->support_channel_num; u1ChannelIdx++)
	{
		p->channel = u1ChannelIdx;
		vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			P_Fld(u1Enable, TEST2_A3_TEST2W) |
			P_Fld(0, TEST2_A3_TEST2R));
	}
	p->channel = eOriChannel;
	return;
}

void PSramc_TA2_Test_Run_Time_HW_Status(DRAMC_CTX_T * p)
{
	U8 u1ChannelIdx = 0;
	U8 u1RankIdx = 0;
	U32 u4loop_count = 0;
	U32 u4ErrRegField = 0;
	U32 u4ErrorValue = 0;
	static U32 err_count = 0;
	static U32 pass_count = 0;
	DRAM_CHANNEL_T eOriChannel = p->channel;

	for(u1ChannelIdx = 0; u1ChannelIdx < p->support_channel_num; u1ChannelIdx++)
	{
		p->channel = u1ChannelIdx;
		u4ErrorValue = PSramcEngine2Compare(p, TE_OP_WRITE_READ_CHECK);
		if (u4ErrorValue) //RK0
		{
			mcSHOW_DBG_MSG("=== HW channel(%d) u4ErrorValue: 0x%x, bit error: 0x%x\n",
				u1ChannelIdx, u4ErrorValue, u4IO32Read4B(DRAMC_REG_ADDR(PSRAMC_REG_CMP_ERR)));
#if defined(SLT)
			while(1);
#endif
		}
		for(u1RankIdx =0 ; u1RankIdx < p->support_rank_num; u1RankIdx++)
		{
			if(u4ErrorValue & (1 << u1RankIdx))
			{
				err_count++;
				mcSHOW_DBG_MSG("HW channel(%d) Rank(%d), TA2 failed, pass_cnt:%d, err_cnt:%d\n",
					u1ChannelIdx, u1RankIdx, pass_count, err_count);
				mcSHOW_TIME_MSG("HW channel(%d) Rank(%d), TA2 failed, pass_cnt:%d, err_cnt:%d\n",
					u1ChannelIdx, u1RankIdx, pass_count, err_count);
			}
			else
			{
				pass_count++;
				mcSHOW_DBG_MSG("HW channel(%d) Rank(%d), TA2 pass, pass_cnt:%d, err_cnt:%d\n",
					u1ChannelIdx, u1RankIdx, pass_count, err_count);
				mcSHOW_TIME_MSG("HW channel(%d) Rank(%d), TA2 pass, pass_cnt:%d, err_cnt:%d\n",
					u1ChannelIdx, u1RankIdx, pass_count, err_count);
			}
	  	}
		vIO32WriteFldMulti(DRAMC_REG_ADDR(PSRAMC_REG_TEST2_A3),
			P_Fld(0, TEST2_A3_TEST2W) |
			P_Fld(0, TEST2_A3_TEST2R) |
			P_Fld(0, TEST2_3_TEST1));
	}
	p->channel = eOriChannel;

	return;
}

void PSramc_TA2_Test_Run_Time_HW(DRAMC_CTX_T * p)
{
	DRAM_CHANNEL_T channel_bak = p->channel;
	DRAM_RANK_T rank_bak = p->rank;

	PSramc_TA2_Test_Run_Time_HW_Presetting(p, 0x200);  //TEST2_2_TEST2_OFF = 0x400
	PSramc_TA2_Test_Run_Time_Pat_Setting(p, TA2_PAT_SWITCH_OFF);
	PSramc_TA2_Test_Run_Time_HW_Write(p, ENABLE);
	//mcDELAY_MS(1);
	PSramc_TA2_Test_Run_Time_HW_Status(p);

	p->channel = channel_bak;
	p->rank = rank_bak;
	return;
}

 void PsramcModeRegWrite(DRAMC_CTX_T *p, U8 u1MRIdx, U8 u1Value)
{
    U32 counter=0;

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1MRIdx, SWCMD_CTRL0_MRSMA);
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1Value, SWCMD_CTRL0_MRWOP);

    // MRW command will be fired when MRWEN 0->1
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 1, SWCMD_EN_MRWEN);

    // wait MRW command fired.
    while(u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_MRW_RESPONSE) ==0)
    {
        counter++;
        mcSHOW_DBG_MSG2("wait MRW command MR%d =0x%x fired (%d)\n",  u1MRIdx, u1Value, counter);
        mcDELAY_US(1);
    }

    // Set MRWEN =0 for next time MRW.
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0, SWCMD_EN_MRWEN);

    if(1) // this should control by switch
    {
        mcSHOW_DBG_MSG2("Write MR%d =0x%x\n",  u1MRIdx, u1Value);
        mcFPRINTF(fp_A60501, "Write MR%d =0x%x\n", u1MRIdx, u1Value);
    }
}

U8 PsramcModeRegRead(DRAMC_CTX_T *p, U8 u1MRIdx)
{
	U32 counter=0;
	U8 mrr_data;

	vIO32WriteFldAlign(PSRAMC_REG_SWCMD_CTRL0, u1MRIdx, SWCMD_CTRL0_MRSMA);

	// MRR command will be fired when MRREN 0->1
	vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 1, SWCMD_EN_MRREN);

	// wait MRW command fired.
	while(u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_MRR_RESPONSE) ==0)
	{
		counter++;
		mcSHOW_DBG_MSG2("wait MRR command  MR%d fired (%d)\n",  u1MRIdx, counter);
		mcDELAY_US(1);
	}

	// Read out mode register value
	mrr_data = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_MRR_STATUS), PSRAM_MRR_STATUS_MRR_DATA);

	// Set MRREN =0 for next time MRR.
	vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0, SWCMD_EN_MRREN);

	if(1) // this should control by switch
	{
		mcSHOW_DBG_MSG2("Read MR%d = 0x%x\n",	u1MRIdx, mrr_data);
		mcFPRINTF(fp_A60501, "Read MR%d \n", u1MRIdx);
	}

	return mrr_data;
}

static void PsramReleaseDMSUS(DRAMC_CTX_T *p)
{
	vIO32WriteFldMulti(DDRPHY_MISC_CTRL1_PSRAM, P_Fld(0x1, MISC_CTRL1_PSRAM_R_DMDQSIENCG_EN_PSRAM)
				| P_Fld(0x1, MISC_CTRL1_PSRAM_R_DM_TX_ARCMD_OE_PSRAM)
				| P_Fld(0x1, MISC_CTRL1_PSRAM_R_DM_TX_ARCLK_OE_PSRAM));

	vIO32WriteFldMulti(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
											   | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
}

static void PsramDataWindowTest(DRAMC_CTX_T *p)
{
#if 0
	U32 u4Mrr = 0;
    U16 u2Tmp;
	U8 i,j,u1FirstUIPass = 0,u1LastUIPass = 0,u1CAStart = 16;
	U8 u1FirstPIPass =0, u1LastPIpass =0;
	U8 passcnt =0;

	PsramcModeRegRead(p,0x1);	  //read predefine data for DQ calibration
	u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS,PSRAM_MRR_STATUS_MRR_REG);
    u4Mrr = (u4Mrr & 0xff);

	mcSHOW_DBG_MSG2("MRR2 read data: 0x%x\n",u4Mrr);

	for(i=u1CAStart; i<u1CAStart+5; i= i+1)
	{
		PSramTXSetDelayReg_CA(p, TRUE, i,&u2Tmp); // CA setting (2, 1), CA OEN (1, 6)

		for(j = 0; j<32; j=j+4)
		{

			mcSHOW_DBG_MSG2("scan range(%d, %d)\n",i,j);
		    vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), j, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);

			PsramcModeRegRead(p,0x20);	  //read predefine data for DQ calibration
			u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS,PSRAM_MRR_STATUS_MRR_REG);
		    u4Mrr = (u4Mrr & 0xff);
			mcSHOW_DBG_MSG2("MRR32 read data: 0x%x\n",u4Mrr);

			u2Tmp = (u4Mrr<<8);

			PsramcModeRegRead(p,0x28);	  //read predefine data for DQ calibration
			u4Mrr = u4IO32ReadFldAlign(PSRAMC_REG_MRR_STATUS,PSRAM_MRR_STATUS_MRR_REG);
		    u4Mrr = (u4Mrr & 0xff);
			mcSHOW_DBG_MSG2("MRR40 read data: 0x%x\n",u4Mrr);

			u2Tmp = (u2Tmp+ u4Mrr);

			if((u2Tmp == 0xff00)&&(u1FirstUIPass == 0))
			{
			   passcnt ++;
			   if(passcnt == 1)
			   	{
				    u1FirstUIPass = i;
					u1FirstPIPass = j;
			   	}
				mcSHOW_DBG_MSG2("CA pass window (%d, %d)\n",i,j);
			}
			else if((passcnt > 10)&&(u2Tmp == 0xff00))
			{
			    u1LastUIPass = i;
				u1LastPIpass = j;
			    mcSHOW_DBG_MSG2("CA pass window found! (%d ~ %d)\n",u1FirstUIPass,u1LastUIPass);
				i = u1CAStart+5; //break;
				break;
			}
			else if((passcnt > 10)&&(u2Tmp != 0xff00))
			{
			   passcnt = 0;
			   mcSHOW_DBG_MSG2("CA fail window %d\n",i);
			}

		}
	}

	u1FirstUIPass = (u1FirstUIPass + u1LastUIPass)>>1;
	u1FirstPIPass = (((u1LastUIPass - u1FirstUIPass)>2?((u1LastUIPass - u1FirstUIPass-1)<<5):((u1LastUIPass - u1FirstUIPass)<<5)) + u1LastPIpass-u1FirstPIPass)>>1;
	u1FirstPIPass = u1FirstPIPass + u1FirstPIPass;
	u1FirstPIPass = (u1FirstPIPass>32?(u1FirstPIPass-32):u1FirstPIPass);

	mcSHOW_DBG_MSG2("final CA center(%d %d)\n",u1FirstUIPass,u1FirstPIPass);
	PSramTXSetDelayReg_CA(p, TRUE, u1FirstUIPass,&u2Tmp); // CA setting (2, 1), CA OEN (1, 6)
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), u1FirstPIPass, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0);
#endif
}

static void PSRAMCModeRegInit(DRAMC_CTX_T *p)
{

	if(p->frequency == 800)
	{
		PsramcModeRegWrite(p,0,0x11);

		PsramcModeRegWrite(p,2,0x20); //enable Temperature mode
	}
	else //2133
	{
		PsramcModeRegWrite(p,0,0x13);

		PsramcModeRegWrite(p,2,0x20); //enable Temperature mode
	}
	//PsramDataWindowTest(p);
}



static void vPSRAM_AutoRefreshSwitch(DRAMC_CTX_T *p, U8 option)
{
	if (option == ENABLE)
	{
		//enable autorefresh
		vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_REFCTRL0), 0, REFCTRL0_REFDIS); 	 //REFDIS=0, enable auto refresh
	}
	else
	{
		//disable autorefresh
		vIO32WriteFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_REFCTRL0), 1, REFCTRL0_REFDIS); 	 //REFDIS=1, disable auto refresh

		//because HW will actually disable autorefresh after refresh_queue empty, so we need to wait quene empty.
		mcDELAY_US(u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_MISC_STATUSA), MISC_STATUSA_REFRESH_QUEUE_CNT)*4);	//wait refresh_queue_cnt * 3.9us
	}

}

static DRAM_STATUS_T PSRAMCPowerOn(DRAMC_CTX_T *p)
{
    U32 u4Response;
    U32 u4TimeCnt = TIME_OUT_CNT;

    mcSHOW_DBG_MSG("[PSRAM PowerOn Start!]\n");

	vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x1, CKECTRL_CKEFIXON);
	//vIO32WriteFldAlign(PSRAMC_REG_DRAMC_PD_CTRL, 0x0, PSAMC_PD_CTRL_APHYCKCG_FIXOFF);

    mcDELAY_US(3); //tPU and Toggle CK>100 cycles

    //global reset
    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x1, SWCMD_EN_RESETEN);
	do
	{
	    u4Response = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_RESET_RESPONSE);
	    u4TimeCnt --;
	    mcDELAY_US(1);  // tRST

	    mcSHOW_DBG_MSG3("%d- ", u4TimeCnt);
	    mcFPRINTF(fp_A60501, "%d- ", u4TimeCnt);
	}while((u4Response==0) &&(u4TimeCnt>0));

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x0, SWCMD_EN_RESETEN);

	mcDELAY_US(10);	// tRST

    //release TCKFIXON
    vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x0, CKECTRL_CKEFIXON);

	//u1PowerOn=1;
	mcSHOW_DBG_MSG("APPLY_PSRAM_POWER_INIT_SEQUENCE Done!\n");

    u4TimeCnt = TIME_OUT_CNT;


    mcSHOW_DBG_MSG("[PSRAM ZQCalibration Start!]\n");
    mcDELAY_US(1);

    vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL1, 0x05, ZQC_CTRL1_ZQCSAD);
    vIO32WriteFldAlign(PSRAMC_REG_ZQC_CTRL1, 0x0, ZQC_CTRL1_ZQCSOP);

    vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x1, SWCMD_EN_ZQCEN);
	do
	{
	    u4Response = u4IO32ReadFldAlign(DRAMC_REG_ADDR(PSRAMC_REG_SPCMDRESP), PSRAM_SPCMDRESP_ZQC_RESPONSE);
	    u4TimeCnt --;
	    mcDELAY_US(1);  // Wait tZQCAL(min) 1us or wait next polling

	    mcSHOW_DBG_MSG3("%d- ", u4TimeCnt);
	    mcFPRINTF(fp_A60501, "%d- ", u4TimeCnt);
	}while((u4Response==0) &&(u4TimeCnt>0));

    if(u4TimeCnt==0)//time out
    {
        vSetCalibrationResult(p, DRAM_CALIBRATION_ZQ, DRAM_FAIL);
        mcSHOW_DBG_MSG("PSRAM ZQCAL Start fail (time out)\n");
        mcFPRINTF(fp_A60501, "PSRAM ZQCAL Start fail (time out)\n");
        return DRAM_FAIL;
    }
    else
    {
		vIO32WriteFldAlign(PSRAMC_REG_SWCMD_EN, 0x0, SWCMD_EN_ZQCEN);
    }

    mcSHOW_DBG_MSG("[PSRAM ZQCalibration Done!]\n");

    return DRAM_OK;
}



DRAM_STATUS_T PsramUpdateACTimingReg(DRAMC_CTX_T *p, const ACTimePsram_T *ACTbl)
{
	ACTimePsram_T ACTblFinal;

	if(ACTbl == NULL)
		return DRAM_FAIL;

	ACTblFinal = *ACTbl;

	vIO32WriteFldMulti(PSRAMC_REG_CE_CNT0, P_Fld(ACTblFinal.tce_wr, CE_CNT0_TCE_WR)
				| P_Fld(ACTblFinal.tce_mrw, CE_CNT0_TCE_MRW)
				| P_Fld(ACTblFinal.tce_rd, CE_CNT0_TCE_RD)
				| P_Fld(ACTblFinal.tce_hsleepx, CE_CNT0_TCE_HSLEEPX));

	vIO32WriteFldMulti(PSRAMC_REG_CE_CNT_05T, P_Fld(ACTblFinal.tce_wr_05T, CE_CNT_05T_TCE_WR_05T)
				| P_Fld(ACTblFinal.tce_rd_05T, CE_CNT_05T_TCE_RD_05T)
				| P_Fld(ACTblFinal.tce_hsleepx_05T, CE_CNT_05T_TCE_HSLEEPX_05T));

	vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM0, P_Fld(ACTblFinal.tcem, SHU_ACTIM0_TCEM)
				| P_Fld(ACTblFinal.txhs, SHU_ACTIM0_TXHS)
				| P_Fld(ACTblFinal.trc, SHU_ACTIM0_TRC)
				| P_Fld(ACTblFinal.trfc, SHU_ACTIM0_TRFC));
	vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM1, P_Fld(ACTblFinal.tcphw, SHU_ACTIM1_TCPHW)
				| P_Fld(ACTblFinal.tcphr_l, SHU_ACTIM1_TCPHR_L)
				| P_Fld(ACTblFinal.tcphr_s, SHU_ACTIM1_TCPHR_S));

	vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T, P_Fld(ACTblFinal.trfc_05T, PSRAMC_SHU_AC_TIME_05T_TRFC_05T)
				| P_Fld(ACTblFinal.trc_05T, PSRAMC_SHU_AC_TIME_05T_TRC_05T)
				| P_Fld(ACTblFinal.txhs_05T, PSRAMC_SHU_AC_TIME_05T_TXHS_05T)
				| P_Fld(ACTblFinal.tcem_05T, PSRAMC_SHU_AC_TIME_05T_TCEM_05T)
				| P_Fld(ACTblFinal.tcphr_s_05T, PSRAMC_SHU_AC_TIME_05T_TCPHR_S_05T)
				| P_Fld(ACTblFinal.tcphr_l_05T, PSRAMC_SHU_AC_TIME_05T_TCPHR_L_05T)
				| P_Fld(ACTblFinal.tcphw_05T, PSRAMC_SHU_AC_TIME_05T_TCPHW_05T)
				| P_Fld(ACTblFinal.tmrw_05T, PSRAMC_SHU_AC_TIME_05T_TMRW_05T));
	vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM2, P_Fld(ACTblFinal.tmrw, SHU_ACTIM2_TMRW));
	vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM3, P_Fld(ACTblFinal.tzqcl, SHU_ACTIM3_TZQCL)
				| P_Fld(ACTblFinal.tzqcs, SHU_ACTIM3_TZQCS));
	vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T_B, P_Fld(ACTblFinal.tzqcl_05T, SHU_AC_TIME_05T_B_TZQCL_05T)
				| P_Fld(ACTblFinal.tzqcs_05T, SHU_AC_TIME_05T_B_TZQCS_05T));

	vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM4, P_Fld(ACTblFinal.txrefcnt, SHU_ACTIM4_TXREFCNT));

	vIO32WriteFldMulti(PSRAMC_REG_REFCNT_FR_CLK1, P_Fld(ACTblFinal.trefcnt_fr_clk1, REFCNT_FR_CLK1_REFCNT_FR_CLK_1X)
				| P_Fld(ACTblFinal.trefcnt_fr_clk2, REFCNT_FR_CLK1_REFCNT_FR_CLK_2X));

	vIO32WriteFldMulti(PSRAMC_REG_ZQC_CTRL1, P_Fld(0x5, ZQC_CTRL1_ZQCSAD)
				| P_Fld(0x0, ZQC_CTRL1_ZQCSOP));

	return DRAM_OK;
}

void EnablePsramcPhyDCM(DRAMC_CTX_T *p, BOOL bEn)//Should refer to "vSetChannelNumber"
{
    U32 u4WbrBackup = GetDramcBroadcast();//Just for bring up
    INT8 i1ShuIdx = 0;
    INT8 i1ShuffleMax = DRAM_DFS_SHUFFLE_1;

   // DramcBroadcastOnOff(DRAMC_BROADCAST_OFF);//Just for bring up

    //PIC:Lynx, different from sylvia, related to PHY 0x298[22][20]
    #if 1
    vIO32WriteFldAlign_All(DDRPHY_MISC_CTRL0_PSRAM, 0x1, MISC_CTRL0_PSRAM_R_DMSHU_PHYDCM_FORCEOFF_PSRAM);
	#endif
    //vIO32WriteFldAlign_All(DDRPHY_MISC_CTRL0, 0x1, MISC_CTRL0_R_DMSHU_PHYDCM_FORCEOFF);

    //APHY PICG DQ & DQM PIC:JOE
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0x0, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0_OPT);
    vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0x1, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0);
    //vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI1, 0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA_OPT);


    vIO32WriteFldAlign_All(DDRPHY_SHU1_B0_DLL0 , 0x1, SHU1_B0_DLL0_RG_ARPISM_MCK_SEL_B0_SHU);//PI_CLOCK_CG_IMPROVEMENT PIC:JOE
    //vIO32WriteFldAlign_All(DDRPHY_SHU1_B1_DLL0 + SHU_GRP_DDRPHY_OFFSET * i1ShuIdx, 0x1, SHU1_B1_DLL0_RG_ARPISM_MCK_SEL_B1_SHU);//PI_CLOCK_CG_IMPROVEMENT PIC:JOE
    //vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_DLL0 + SHU_GRP_DDRPHY_OFFSET * i1ShuIdx, 0x1, SHU1_CA_DLL0_RG_ARPISM_MCK_SEL_CA_SHU);//PI_CLOCK_CG_IMPROVEMENT PIC:JOE


    if(bEn)//DCM on
    {
        //vIO32WriteFldMulti_All(DRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, DRAMC_PD_CTRL_DCMENNOTRFC)
               // | P_Fld(0x1, DRAMC_PD_CTRL_COMBCLKCTRL)
               // | P_Fld(0x1, DRAMC_PD_CTRL_PHYCLKDYNGEN)
               // | P_Fld(0x1, DRAMC_PD_CTRL_DCMEN) | P_Fld(0x0, DRAMC_PD_CTRL_COMBPHY_CLKENSAME)
               // | P_Fld(0x1, DRAMC_PD_CTRL_DCMEN2));

#if 1//PSRAM_SPEC
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL0, P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF_PSRAMC)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PSRAMC_CHA_CK_OFF)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_DRAMC_CHB_CK_OFF)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_INFRA_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_COMB1_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_COMB0_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_RX_CMD_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB1_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB0_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_CMD_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_COMB_OFF_DISABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PHY_OFF_DIABLE)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_DRAMC_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_EMI_OFF_DISABLE));

		vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_COMBCLKCTRL)
				| P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)
				| P_Fld(0x0, PSAMC_PD_CTRL_MIOCKCTRLOFF)
				| P_Fld(0x0, PSAMC_PD_CTRL_COMBPHY_CLKENSAME)
				| P_Fld(0x0, PSAMC_PD_CTRL_PHYGLUECLKRUN)
				| P_Fld(0x1, PSAMC_PD_CTRL_DCMENNOTRFC)
				| P_Fld(0x1, PSRAMC_PD_CTRL_DCMEN));

		vIO32WriteFldMulti(PSRAMC_REG_TX_CG_SET0, P_Fld(0x0, TX_CG_SET0_SELPH_4LCG_DIS)
			| P_Fld(0x0, TX_CG_SET0_SELPH_CMD_CG_DIS));

		vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x0, RX_CG_SET0_RDYCKAR);
		vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x0, CLKAR_REQQUECLKRUN);
		vIO32WriteFldAlign(PSRAMC_REG_SREF_DPD_CTRL, 0x0, SREF_DPD_CTRL_SREF_CG_OPT);
		vIO32WriteFldAlign(PSRAMC_REG_ACTIMING_CTRL, 0x0, ACTIMING_CTRL_SEQCLKRUN);

		vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x0, CKECTRL_CKEFIXON);
		vIO32WriteFldAlign(PSRAMC_REG_TEST2_A3, 0x0, TEST2_A3_TESTCLKRUN);

#endif

        //mem_dcm
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BE);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BF);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x806003BE);//divided freq change to 1/4
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL2, P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG)
                                               | P_Fld(0x17, MISC_CG_CTRL2_RG_MEM_DCM_APB_SEL)
                                               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_ON)
                                               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DCM_EN)
                                               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_EN)
                                               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_CNT)
                                               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FSEL)
                                               | P_Fld(0x3, MISC_CG_CTRL2_RG_MEM_DCM_IDLE_FSEL)
                                               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_OFF)
                                               | P_Fld(0x0, MISC_CG_CTRL2_RG_PHY_CG_OFF_DISABLE)
                                               | P_Fld(0x0, MISC_CG_CTRL2_RG_PIPE0_CG_OFF_DISABLE)
                                               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_CG_OFF_DISABLE));
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x1, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);

        //TX pipe/sync cell CG
        #if 1//PSRAM_SPEC
        vIO32WriteFldMulti_All(DDRPHY_MISC_CTRL3_PSRAM, P_Fld(0x0, MISC_CTRL3_PSRAM_R_DDRPHY_COMB_CG_IG_PSRAM)
                | P_Fld(0x0, MISC_CTRL3_PSRAM_R_DDRPHY_RX_PIPE_CG_IG_PSRAM));

		vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD7_PSRAM, P_Fld(0x1, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CS_NEW_PSRAM)
				| P_Fld(0x1, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CLK_NEW_PSRAM)
				| P_Fld(0x1, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CMD_NEW_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

		vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQM_NEW_B0_PSRAM)
				| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQS_NEW_B0_PSRAM)
				| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQ_NEW_B0_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE
		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
	                                        | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_RMRODTEN_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_RXDLY_PIPE_CG_IG_B0_PSRAM));		
		#endif
        vIO32WriteFldMulti_All(DDRPHY_MISC_CTRL3, P_Fld(0x0, MISC_CTRL3_R_DDRPHY_COMB_CG_IG)
                | P_Fld(0x0, MISC_CTRL3_R_DDRPHY_RX_PIPE_CG_IG));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL0, P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_DQSIENCG_EN)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQSIEN)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQ)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQS)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQM)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_CG_MCK)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL0_OPT2_MPDIV_CG)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_ARPI_MPDIV_CG_DQ_OPT));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL1, P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_CLKIENCG_EN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLKIEN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CMD)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLK)
                | P_Fld(0x1, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CS)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_MPDIV_CG_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCTL_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCK_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CMD_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CLK_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CMD_NEW)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CS_NEW));
        vIO32WriteFldMulti_All(DDRPHY_MISC_CG_CTRL5, P_Fld(0x1, MISC_CG_CTRL5_R_CA_DLY_DCM_EN)
                | P_Fld(0x1, MISC_CG_CTRL5_R_DQ0_DLY_DCM_EN)
                | P_Fld(0x1, MISC_CG_CTRL5_R_DQ1_DLY_DCM_EN)
                | P_Fld(0x1, MISC_CG_CTRL5_R_CA_PI_DCM_EN)
                | P_Fld(0x1, MISC_CG_CTRL5_R_DQ0_PI_DCM_EN)
                | P_Fld(0x1, MISC_CG_CTRL5_R_DQ1_PI_DCM_EN));
    }
    else//DCM off
    {
        //vIO32WriteFldMulti_All(DRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x0, DRAMC_PD_CTRL_DCMENNOTRFC)
                //| P_Fld(0x0, DRAMC_PD_CTRL_COMBCLKCTRL)
                //| P_Fld(0x0, DRAMC_PD_CTRL_PHYCLKDYNGEN)
                //| P_Fld(0x0, DRAMC_PD_CTRL_DCMEN) | P_Fld(0x1, DRAMC_PD_CTRL_COMBPHY_CLKENSAME)
                //| P_Fld(0x0, DRAMC_PD_CTRL_DCMEN2));
#if PSRAM_SPEC

		vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL0, P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF_PSRAMC)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_PSRAMC_CHA_CK_OFF)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_NAO_FORCE_OFF)
                | P_Fld(0x0, MISC_CG_CTRL0_RG_CG_DRAMC_CHB_CK_OFF)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_INFRA_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_COMB1_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_COMB0_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_RX_CMD_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB1_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB0_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_CMD_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_COMB_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_PHY_OFF_DIABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_DRAMC_OFF_DISABLE)
                | P_Fld(0x1, MISC_CG_CTRL0_RG_CG_EMI_OFF_DISABLE));
        vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x0, PSAMC_PD_CTRL_COMBCLKCTRL)
				| P_Fld(0x0, PSAMC_PD_CTRL_PHYCLKDYNGEN)
				| P_Fld(0x1, PSAMC_PD_CTRL_MIOCKCTRLOFF)
			| P_Fld(0x1, PSAMC_PD_CTRL_COMBPHY_CLKENSAME)
				| P_Fld(0x1, PSAMC_PD_CTRL_PHYGLUECLKRUN)
			| P_Fld(0x0, PSAMC_PD_CTRL_DCMENNOTRFC)
			| P_Fld(0x0, PSRAMC_PD_CTRL_DCMEN));

		vIO32WriteFldMulti(PSRAMC_REG_TX_CG_SET0, P_Fld(0x1, TX_CG_SET0_SELPH_4LCG_DIS)
			| P_Fld(0x1, TX_CG_SET0_SELPH_CMD_CG_DIS));

		vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x1, RX_CG_SET0_RDYCKAR);
		vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x1, CLKAR_REQQUECLKRUN);
		vIO32WriteFldAlign(PSRAMC_REG_SREF_DPD_CTRL, 0x1, SREF_DPD_CTRL_SREF_CG_OPT);
		vIO32WriteFldAlign(PSRAMC_REG_ACTIMING_CTRL, 0x1, ACTIMING_CTRL_SEQCLKRUN);

        vIO32WriteFldAlign(PSRAMC_REG_CKECTRL, 0x1, CKECTRL_CKEFIXON);
	vIO32WriteFldAlign(PSRAMC_REG_TEST2_A3, 0x1, TEST2_A3_TESTCLKRUN);

#endif
        //mem_dcm
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037E);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037F);//divided freq change to 1/4
        //vIO32Write4B_All(DDRPHY_MISC_CG_CTRL2, 0x8060037E);//divided freq change to 1/4
        vIO32WriteFldMulti(DDRPHY_MISC_CG_CTRL2, P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG)
               | P_Fld(0x17, MISC_CG_CTRL2_RG_MEM_DCM_APB_SEL)
               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_ON)
               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_DCM_EN)
               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_EN)
               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_DBC_CNT)
               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FSEL)
               | P_Fld(0x3, MISC_CG_CTRL2_RG_MEM_DCM_IDLE_FSEL)
               | P_Fld(0x0, MISC_CG_CTRL2_RG_MEM_DCM_FORCE_OFF)
               | P_Fld(0x0, MISC_CG_CTRL2_RG_PHY_CG_OFF_DISABLE)
               | P_Fld(0x0, MISC_CG_CTRL2_RG_PIPE0_CG_OFF_DISABLE)
               | P_Fld(0x1, MISC_CG_CTRL2_RG_MEM_DCM_CG_OFF_DISABLE));
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x1, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x0, MISC_CG_CTRL2_RG_MEM_DCM_APB_TOG);
        //TX pipe/sync cell CG
        #if 1//PSRAM_SPEC
        vIO32WriteFldMulti_All(DDRPHY_MISC_CTRL3_PSRAM, P_Fld(0x1, MISC_CTRL3_PSRAM_R_DDRPHY_COMB_CG_IG_PSRAM)
                | P_Fld(0x1, MISC_CTRL3_PSRAM_R_DDRPHY_RX_PIPE_CG_IG_PSRAM));
		vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD7_PSRAM, P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CS_NEW_PSRAM)
				| P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CLK_NEW_PSRAM)
				| P_Fld(0x0, SHU1_CA_CMD7_PSRAM_R_DMTX_ARPI_CG_CMD_NEW_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

		vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQM_NEW_B0_PSRAM)
				| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQS_NEW_B0_PSRAM)
				| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMTX_ARPI_CG_DQ_NEW_B0_PSRAM));//PI_CLOCK_CG_IMPROVEMENT PIC:JOE

		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
	                                        | P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_RMRODTEN_CG_IG_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRANK_RXDLY_PIPE_CG_IG_B0_PSRAM));
		#endif
        vIO32WriteFldMulti_All(DDRPHY_MISC_CTRL3, P_Fld(0x1, MISC_CTRL3_R_DDRPHY_COMB_CG_IG)
                | P_Fld(0x1, MISC_CTRL3_R_DDRPHY_RX_PIPE_CG_IG));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL0, P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_DQSIENCG_EN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQSIEN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQ)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQS)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_DQM)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_CG_MCK)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_OPT2_MPDIV_CG)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL0_ARPI_MPDIV_CG_DQ_OPT));
        vIO32WriteFldMulti(DDRPHY_PSRAM_APHY_PICG_CTRL1, P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_CLKIENCG_EN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLKIEN)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CMD)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CLK)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_OPT2_CG_CS)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_MPDIV_CG_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCTL_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_MCK_CA_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CMD_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_ARPI_CG_CLK_OPT)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CMD_NEW)
                | P_Fld(0x0, PSRAM_APHY_PICG_CTRL1_TX_ARPI_CG_CS_NEW));
        vIO32WriteFldMulti_All(DDRPHY_MISC_CG_CTRL5, P_Fld(0x0, MISC_CG_CTRL5_R_CA_DLY_DCM_EN)
                | P_Fld(0x0, MISC_CG_CTRL5_R_DQ0_DLY_DCM_EN)
                | P_Fld(0x0, MISC_CG_CTRL5_R_DQ1_DLY_DCM_EN)
                | P_Fld(0x0, MISC_CG_CTRL5_R_CA_PI_DCM_EN)
                | P_Fld(0x0, MISC_CG_CTRL5_R_DQ0_PI_DCM_EN)
                | P_Fld(0x0, MISC_CG_CTRL5_R_DQ1_PI_DCM_EN));
    }

#if 0
    /* Different part */
    //if(u1IsLP4Family(p->dram_type))
    {
        LP4EnableDramcPhyDCM2Channel(p, bEn);
    }

    #if 0//ENABLE_LP3_SW
    else//LP3
    {
        LP3EnableDramcPhyDCM(p, bEn);
    }
    #endif

    DramcBroadcastOnOff(u4WbrBackup);//Just for bring up
#endif
}

void PsramDDRPhyPLLSetting(DRAMC_CTX_T *p)
{
    U8 i;
	U8 u1CAP_SEL;
	U8 u1MIDPICAP_SEL;
	U8 u1VTH_SEL;
	U16 u2SDM_PCW = 0;
	U8 u1CA_DLL_Mode[2];
	U8 iChannel=CHANNEL_A;
	U8 u1BRPI_MCTL_EN_CA = 0;
#if ENABLE_TMRRI_NEW_MODE
	U8 u1RankIdx;
#endif

	u1VTH_SEL = 0x2; /* RG_*RPI_MIDPI_VTH_SEL[1:0] is 2 for all freqs */

#if (fcFOR_CHIP_ID == fcSchubert)
	//Freq > 1333, CAP_SEL=0
	//Freq 801~1333, CAP_SEL=2
	//Freq <= 800, CAP_SEL=3

	if(p->frequency <= 933)
	{
		u1CAP_SEL = 0x3;
	}
	else
	{
		u1CAP_SEL = 0x0;
	}
#endif

	if (p->frequency <= 1866)
	{
		u1MIDPICAP_SEL = 0x1;
	}
	else
	{
		u1MIDPICAP_SEL = 0x0;
	}


	vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL4, 0xfe, SHU1_PLL4_RG_RPHYPLL_RESERVED);
	vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL6, 0xfe, SHU1_PLL6_RG_RCLRPLL_RESERVED);

#if (fcFOR_CHIP_ID == fcSchubert)
    #if 0
		u1BRPI_MCTL_EN_CA=1;
		u1CA_DLL_Mode[CHANNEL_A] = u1CA_DLL_Mode[CHANNEL_B] = DLL_MASTER;
		vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT+((U32)CHANNEL_A<<POS_BANK_NUM), 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
		vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT+SHIFT_TO_CHB_ADDR, 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
		vIO32WriteFldMulti(DDRPHY_CKMUX_SEL+((U32)CHANNEL_A<<POS_BANK_NUM), P_Fld(0, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(0, CKMUX_SEL_FB_CK_MUX));
		vIO32WriteFldMulti(DDRPHY_CKMUX_SEL+SHIFT_TO_CHB_ADDR, P_Fld(2, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(2, CKMUX_SEL_FB_CK_MUX));
		vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD0, 0x0, SHU1_CA_CMD0_RG_FB_CK_MUX);
    #else
		u1CA_DLL_Mode[CHANNEL_A] = DLL_MASTER;
		u1CA_DLL_Mode[CHANNEL_B] = DLL_SLAVE;
		vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT+((U32)CHANNEL_A<<POS_BANK_NUM), 1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
		//vIO32WriteFldAlign(DDRPHY_MISC_SHU_OPT+SHIFT_TO_CHB_ADDR, 2, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN);
		vIO32WriteFldMulti(DDRPHY_CKMUX_SEL+((U32)CHANNEL_A<<POS_BANK_NUM), P_Fld(1, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(1, CKMUX_SEL_FB_CK_MUX));
		//vIO32WriteFldMulti(DDRPHY_CKMUX_SEL+SHIFT_TO_CHB_ADDR, P_Fld(1, CKMUX_SEL_FMEM_CK_MUX) | P_Fld(1, CKMUX_SEL_FB_CK_MUX));
		vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD0, 0x1, SHU1_CA_CMD0_RG_FB_CK_MUX);
    #endif

#if ENABLE_DLL_ALL_SLAVE_MODE
	if (p->frequency <= 933)
	{
		u1CA_DLL_Mode[CHANNEL_A] = u1CA_DLL_Mode[CHANNEL_B] = DLL_SLAVE;
	}
#endif

#if 0
	if(u1CA_DLL_Mode[CHANNEL_A]==DLL_SLAVE)//All slave mode
	{
		vIO32WriteFldAlign_All(DRAMC_REG_DVFSDLL, 1, DVFSDLL_R_BYPASS_1ST_DLL_SHU1);
	}
	else
	{
		vIO32WriteFldAlign_All(DRAMC_REG_DVFSDLL, 0, DVFSDLL_R_BYPASS_1ST_DLL_SHU1);
	}
#endif

	//for(iChannel=CHANNEL_A; iChannel<=CHANNEL_B; iChannel++)
//	{
		if(u1CA_DLL_Mode[iChannel]==DLL_MASTER)
		{
			vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL0+((U32)iChannel<<POS_BANK_NUM), P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_PHDET_OUT_SEL_CA)
						| P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_PHDET_IN_SWAP_CA)
						| P_Fld(0x6, SHU1_CA_DLL0_RG_ARDLL_GAIN_CA)
						| P_Fld(0x9, SHU1_CA_DLL0_RG_ARDLL_IDLECNT_CA)
						| P_Fld(0x8, SHU1_CA_DLL0_RG_ARDLL_P_GAIN_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHJUMP_EN_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDIV_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_FAST_PSJP_CA));
			vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL1+((U32)iChannel<<POS_BANK_NUM), P_Fld(0x1, SHU1_CA_DLL1_RG_ARDLL_PD_CK_SEL_CA) | P_Fld(0x0, SHU1_CA_DLL1_RG_ARDLL_FASTPJ_CK_SEL_CA));
			vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6+((U32)iChannel<<POS_BANK_NUM), 1, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP); // RG_*RPI_RESERVE_CA[1] 1'b1 tracking leaf(slave)
		}
		else
		{
			vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL0+((U32)iChannel<<POS_BANK_NUM), P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDET_OUT_SEL_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDET_IN_SWAP_CA)
						| P_Fld(0x7, SHU1_CA_DLL0_RG_ARDLL_GAIN_CA)
						| P_Fld(0x7, SHU1_CA_DLL0_RG_ARDLL_IDLECNT_CA)
						| P_Fld(0x8, SHU1_CA_DLL0_RG_ARDLL_P_GAIN_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHJUMP_EN_CA)
						| P_Fld(0x1, SHU1_CA_DLL0_RG_ARDLL_PHDIV_CA)
						| P_Fld(0x0, SHU1_CA_DLL0_RG_ARDLL_FAST_PSJP_CA));
			vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL1+((U32)iChannel<<POS_BANK_NUM), P_Fld(0x0, SHU1_CA_DLL1_RG_ARDLL_PD_CK_SEL_CA) | P_Fld(0x1, SHU1_CA_DLL1_RG_ARDLL_FASTPJ_CK_SEL_CA));
			vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6+((U32)iChannel<<POS_BANK_NUM), 0, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP); // RG_*RPI_RESERVE_CA[1] 1'b1 tracking leaf(slave)
		}
//	}
#endif


	U32 u4RegBackupAddress[] =
	{
		(DDRPHY_B0_DQ7),
		(DDRPHY_CA_CMD7),
	};

	//if(p->vendor_id==VENDOR_SAMSUNG && p->dram_type==TYPE_LPDDR3)
	{
#if 0
		mcSHOW_DBG_MSG("DDRPhyPLLSetting-DMSUS\n");
		vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL0, 0x0, MISC_SPM_CTRL0_PHY_SPM_CTL0);
		vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL2, 0x0, MISC_SPM_CTRL2_PHY_SPM_CTL2);
		vIO32WriteFldMulti_All(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
												   | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x1, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
#else
		DramcBackupRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
		vIO32WriteFldMulti_All(DDRPHY_B0_DQ7, P_Fld(0x1, B0_DQ7_RG_TX_ARDQ_PULL_DN_B0) | P_Fld(0x1, B0_DQ7_RG_TX_ARDQM0_PULL_DN_B0)
												   | P_Fld(0x1, B0_DQ7_RG_TX_ARDQS0_PULL_DN_B0) | P_Fld(0x1, B0_DQ7_RG_TX_ARDQS0B_PULL_DN_B0));
//		vIO32WriteFldMulti_All(DDRPHY_B1_DQ7, P_Fld(0x1, B1_DQ7_RG_TX_ARDQ_PULL_DN_B1) | P_Fld(0x1, B1_DQ7_RG_TX_ARDQM0_PULL_DN_B1)
//												   | P_Fld(0x1, B1_DQ7_RG_TX_ARDQS0_PULL_DN_B1) | P_Fld(0x1, B1_DQ7_RG_TX_ARDQS0B_PULL_DN_B1));
		vIO32WriteFldMulti_All(DDRPHY_CA_CMD7, P_Fld(0x1, CA_CMD7_RG_TX_ARCMD_PULL_DN) | P_Fld(0x1, CA_CMD7_RG_TX_ARCS_PULL_DN)
												   | P_Fld(0x1, CA_CMD7_RG_TX_ARCLK_PULL_DN) | P_Fld(0x1, CA_CMD7_RG_TX_ARCLKB_PULL_DN));

		// DMSUS replaced by CA_CMD2_RG_TX_ARCMD_OE_DIS, CMD_OE_DIS(1) will prevent illegal command ouput
		// And DRAM 1st reset_n pulse will disappear if use CA_CMD2_RG_TX_ARCMD_OE_DIS
		vIO32WriteFldAlign_All(DDRPHY_CA_CMD2, 1, CA_CMD2_RG_TX_ARCMD_OE_DIS);
#endif
	}

	//26M
	vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x0, MISC_CG_CTRL0_CLK_MEM_SEL);

#ifdef USE_CLK26M
	vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_RG_DA_RREF_CK_SEL);
#endif


	//MIDPI
	vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
				| P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
//	vIO32WriteFldMulti_All(DDRPHY_SHU1_B1_DQ6, P_Fld(0x0, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1)
//				| P_Fld(0x0, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1));
	vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD6, P_Fld(0x0, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA)
				| P_Fld(0x0, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA));

	vIO32WriteFldMulti_All(DDRPHY_PLL4, P_Fld(0x0, PLL4_RG_RPHYPLL_ADA_MCK8X_EN)
				| P_Fld(0x0, PLL4_RG_RPHYPLL_RESETB));

	//PLL
	vIO32WriteFldAlign_All(DDRPHY_PLL1, 0x0, PLL1_RG_RPHYPLL_EN);
	vIO32WriteFldAlign_All(DDRPHY_PLL2, 0x0, PLL2_RG_RCLRPLL_EN);

	//DLL
	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI2, 0x0, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0x0, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);

//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI2, 0x0, B1_DLL_ARPI2_RG_ARDLL_PHDET_EN_B1);

	vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2_PSRAM, P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_FB2DLL_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCTL_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_FB_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQS_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQM_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQ_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQSIEN_B0_PSRAM)
				| P_Fld(0x1, B0_DLL_ARPI2_PSRAM_RG_ARPI_MPDIV_CG_B0_PSRAM));

	vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2, P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0)
				| P_Fld(0x1, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));
#if 0
	vIO32WriteFldMulti_All(DDRPHY_B1_DLL_ARPI2, P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_MCK_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_MCTL_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_FB_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_DQS_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_DQM_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_DQ_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B1)
				| P_Fld(0x1, B1_DLL_ARPI2_RG_ARPI_MPDIV_CG_B1));
#endif
	vIO32WriteFldMulti_All(DDRPHY_CA_DLL_ARPI2, P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_MCK_CA)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_CA)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_MCTL_CA)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_FB_CA)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_CS)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_CLK)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_CMD)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN)
				| P_Fld(0x1, CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA));

	//RESETB
	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI0, 0x0, CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 0x0, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI0, 0x0, B1_DLL_ARPI0_RG_ARPI_RESETB_B1);

	mcDELAY_US(1);

	///TODO: PLL/MIDPI Settings
	//Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
#ifdef USE_CLK26M
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV) | P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_PREDIV));
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV) | P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_PREDIV));
#else //MPLL 52M
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV) | P_Fld(0x1, SHU1_PLL8_RG_RPHYPLL_PREDIV));
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV) | P_Fld(0x1, SHU1_PLL10_RG_RCLRPLL_PREDIV));
#endif

	if(p->frequency==800)
	{
		u2SDM_PCW = 0x7600;
	}
	else if(p->frequency == 1066)
	{
		u2SDM_PCW = 0x5000;
	}
	else
	{
		u2SDM_PCW = 0X7600;
	}

	/* SDM_PCW: Feedback divide ratio (8-bit integer + 8-bit fraction)
	 * PLL_SDM_FRA_EN: SDMPLL fractional mode enable (0:Integer mode, 1:Fractional mode)
	 */
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL5, P_Fld(u2SDM_PCW, SHU1_PLL5_RG_RPHYPLL_SDM_PCW)
											| P_Fld(0x0, SHU1_PLL5_RG_RPHYPLL_SDM_FRA_EN)); // Disable fractional mode
	vIO32WriteFldMulti_All(DDRPHY_SHU1_PLL7, P_Fld(u2SDM_PCW, SHU1_PLL7_RG_RCLRPLL_SDM_PCW)
											| P_Fld(0x0, SHU1_PLL7_RG_RCLRPLL_SDM_FRA_EN)); // Disable fractional mode


	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI0, 1, CA_DLL_ARPI0_RG_ARMCTLPLL_CK_SEL_CA);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 1, B0_DLL_ARPI0_RG_ARMCTLPLL_CK_SEL_B0);
//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI0, 1, B1_DLL_ARPI0_RG_ARMCTLPLL_CK_SEL_B1);
	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI1, 0, CA_DLL_ARPI1_RG_ARPI_CLKIEN_JUMP_EN);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI1, 0, B0_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B0);
//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI1, 0, B1_DLL_ARPI1_RG_ARPI_MCTL_JUMP_EN_B1);


	vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(u1VTH_SEL, SHU1_B0_DQ6_RG_ARPI_MIDPI_VTH_SEL_B0)
				| P_Fld(u1CAP_SEL, SHU1_B0_DQ6_RG_ARPI_CAP_SEL_B0)
				| P_Fld(u1MIDPICAP_SEL, SHU1_B0_DQ6_RG_ARPI_MIDPI_CAP_SEL_B0));
//	vIO32WriteFldMulti_All(DDRPHY_SHU1_B1_DQ6, P_Fld(u1VTH_SEL, SHU1_B1_DQ6_RG_ARPI_MIDPI_VTH_SEL_B1)
//				| P_Fld(u1CAP_SEL, SHU1_B1_DQ6_RG_ARPI_CAP_SEL_B1)
//				| P_Fld(u1MIDPICAP_SEL, SHU1_B1_DQ6_RG_ARPI_MIDPI_CAP_SEL_B1));
	vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD6, P_Fld(u1VTH_SEL, SHU1_CA_CMD6_RG_ARPI_MIDPI_VTH_SEL_CA)
				| P_Fld(u1CAP_SEL, SHU1_CA_CMD6_RG_ARPI_CAP_SEL_CA)
				| P_Fld(u1MIDPICAP_SEL, SHU1_CA_CMD6_RG_ARPI_MIDPI_CAP_SEL_CA));


	///TODO: RESETB
	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI0, 0x1, CA_DLL_ARPI0_RG_ARPI_RESETB_CA);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI0, 0x1, B0_DLL_ARPI0_RG_ARPI_RESETB_B0);
//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI0, 0x1, B1_DLL_ARPI0_RG_ARPI_RESETB_B1);
	mcDELAY_US(1);

	///TODO: PLL EN
	vIO32WriteFldAlign_All(DDRPHY_PLL1, 0x1, PLL1_RG_RPHYPLL_EN);
	vIO32WriteFldAlign_All(DDRPHY_PLL2, 0x1, PLL2_RG_RCLRPLL_EN);
	if(FOR_DV_SIMULATION_USED == 1)
	{
	    for(i =0; i<100; i++)
	        mcDELAY_US(1);
	}
	else
		mcDELAY_US(100);

	///TODO: MIDPI Init 1
	vIO32WriteFldMulti_All(DDRPHY_PLL4, P_Fld(0x1, PLL4_RG_RPHYPLL_ADA_MCK8X_EN)
				| P_Fld(0x1, PLL4_RG_RPHYPLL_RESETB));

	mcDELAY_US(1);


	///TODO: MIDPI Init 2
	/* MIDPI Settings (Olympus): DA_*RPI_MIDPI_EN, DA_*RPI_MIDPI_CKDIV4_EN
	 * Justin suggests use frequency > 933 as boundary
	 */
	if(p->frequency > 933)
	{
		vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
					| P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
//		vIO32WriteFldMulti_All(DDRPHY_SHU1_B1_DQ6, P_Fld(0x1, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1)
//					| P_Fld(0x0, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1));
		vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD6, P_Fld(0x1, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA)
					| P_Fld(0x0, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA));
	}
	else
	{
		vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0)
					| P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0));
//		vIO32WriteFldMulti_All(DDRPHY_SHU1_B1_DQ6, P_Fld(0x0, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1)
//					| P_Fld(0x1, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1));
		vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_CMD6, P_Fld(0x0, SHU1_CA_CMD6_RG_ARPI_MIDPI_EN_CA)
					| P_Fld(0x1, SHU1_CA_CMD6_RG_ARPI_MIDPI_CKDIV4_EN_CA));
	}
	mcDELAY_US(1);

    #if (fcFOR_CHIP_ID == fcSchubert)
		vIO32WriteFldMulti(DDRPHY_CA_DLL_ARPI3, P_Fld(0x1, CA_DLL_ARPI3_RG_ARPI_MCTL_EN_CA)
					| P_Fld(0x1, CA_DLL_ARPI3_RG_ARPI_FB_EN_CA)
					| P_Fld(0x1, CA_DLL_ARPI3_RG_ARPI_CS_EN)
					| P_Fld(0x1, CA_DLL_ARPI3_RG_ARPI_CLK_EN)
					| P_Fld(0x1, CA_DLL_ARPI3_RG_ARPI_CMD_EN));

    #endif
	vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI3, P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_FB_EN_B0)
				| P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQS_EN_B0)
				| P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQM_EN_B0)
				| P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQ_EN_B0)
				| P_Fld(0x1, B0_DLL_ARPI3_RG_ARPI_DQSIEN_EN_B0));

	vIO32WriteFldMulti_All(DDRPHY_CA_DLL_ARPI2, P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_MCK_CA)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_CA)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_MCTL_CA)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_FB_CA)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_CS)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_CLK)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_CG_CMD)
				| P_Fld(0x0, CA_DLL_ARPI2_RG_ARPI_MPDIV_CG_CA));
	vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2_PSRAM, P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCK_FB2DLL_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_MCTL_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_FB_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQS_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQM_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQ_B0_PSRAM)
				| P_Fld(0x0, B0_DLL_ARPI2_PSRAM_RG_ARPI_MPDIV_CG_B0_PSRAM));

	vIO32WriteFldMulti_All(DDRPHY_B0_DLL_ARPI2, P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCK_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCK_FB2DLL_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_MCTL_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_FB_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQS_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQM_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_CG_DQ_B0)
				| P_Fld(0x0, B0_DLL_ARPI2_RG_ARPI_MPDIV_CG_B0));

#if (fcFOR_CHIP_ID == fcSchubert)
		vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI2, 1, CA_DLL_ARPI2_RG_ARPI_CG_CLKIEN);
		vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2_PSRAM, 0, B0_DLL_ARPI2_PSRAM_RG_ARPI_CG_DQSIEN_B0_PSRAM);
		vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0, B0_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B0);
//		vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI2, 0, B1_DLL_ARPI2_RG_ARPI_CG_DQSIEN_B1);
#endif

	mcDELAY_US(2);

	vIO32WriteFldAlign_All(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_CLK_MEM_SEL);
	mcDELAY_US(1);

	//DLL
	vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI2, 0x1, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
	mcDELAY_US(1);
	vIO32WriteFldAlign_All(DDRPHY_CA_DLL_ARPI2, 0x1, CA_DLL_ARPI2_RG_ARDLL_PHDET_EN_CA);
	mcDELAY_US(1);
	//vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2_PSRAM, 0x1, B0_DLL_ARPI2_PSRAM_RG_ARDLL_PHDET_EN_B0_PSRAM);
	vIO32WriteFldAlign_All(DDRPHY_B0_DLL_ARPI2, 0x1, B0_DLL_ARPI2_RG_ARDLL_PHDET_EN_B0);
	mcDELAY_US(1);
//	vIO32WriteFldAlign_All(DDRPHY_B1_DLL_ARPI2, 0x1, B1_DLL_ARPI2_RG_ARDLL_PHDET_EN_B1);
//	mcDELAY_US(1);

	//if(p->vendor_id==VENDOR_SAMSUNG && p->dram_type==TYPE_LPDDR3)
	{
#if 0
		mcSHOW_DBG_MSG("DDRPhyPLLSetting-DMSUS\n\n");
		vIO32WriteFldMulti_All(DDRPHY_MISC_SPM_CTRL1, P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B0)
												   | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_B1) | P_Fld(0x0, MISC_SPM_CTRL1_RG_ARDMSUS_10_CA));
		vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL0, 0xffffffff, MISC_SPM_CTRL0_PHY_SPM_CTL0);
		vIO32WriteFldAlign_All(DDRPHY_MISC_SPM_CTRL2, 0xffffffff, MISC_SPM_CTRL2_PHY_SPM_CTL2);
#else
		// DMSUS replaced by CA_CMD2_RG_TX_ARCMD_OE_DIS, CMD_OE_DIS(1) will prevent illegal command ouput
		// And DRAM 1st reset_n pulse will disappear if use CA_CMD2_RG_TX_ARCMD_OE_DIS
		vIO32WriteFldAlign_All(DDRPHY_CA_CMD2, 0, CA_CMD2_RG_TX_ARCMD_OE_DIS);
		DramcRestoreRegisters(p, u4RegBackupAddress, sizeof(u4RegBackupAddress)/sizeof(U32));
#endif

		//#if !PSRAM_SPEC
		mcSHOW_DBG_MSG("PsramDDRPhyPLLSetting-CKEON\n\n");
//		CKEFixOnOff(p, CKE_WRITE_TO_ALL_RANK, CKE_DYNAMIC, CKE_WRITE_TO_ALL_CHANNEL);
	   //#endif
	}

	DDRPhyFreqMeter();
}



void PsramDDRPhyReservedRGSetting(DRAMC_CTX_T *p)
{
	U8 u1HYST_SEL=0;
	U8 u1MIDPI_CAP_SEL2=0;
	U8 u1LP3_SEL=0;
	U8 u1SER_RST_MODE=1;
	U8 u1TX_READ_BASE_EN=1;
	U8 u1ARPI_BIT4to10=0;
	U8 u1PSMUX_DRV_SEL=0;
	U8 u1Bypass_SR = 1;

	if(p->frequency<=1333)
		u1HYST_SEL=1;

	if(p->frequency<1333)
		u1MIDPI_CAP_SEL2=1;
	else
		u1MIDPI_CAP_SEL2=0;

	if(p->frequency>=933)
		u1PSMUX_DRV_SEL=1;

	if(p->frequency<=933)
		u1ARPI_BIT4to10=1;

	u1Bypass_SR = 1;

	vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL4, 1, RG_PLL_RESERVE_BIT_13_PLL_FS_EN);
	vIO32WriteFldAlign_All(DDRPHY_SHU1_PLL6, 1, RG_PLL_RESERVE_BIT_13_PLL_FS_EN);

	//PI
	vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD6, P_Fld(0x1, RG_ARPI_RESERVE_BIT_00_TX_CG_EN) // RG_*RPI_RESERVE_B0[0] 1'b1 prevent leakage
										| P_Fld(1, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP)
										| P_Fld(u1HYST_SEL, RG_ARPI_RESERVE_BIT_02_HYST_SEL)
										| P_Fld(u1MIDPI_CAP_SEL2, RG_ARPI_RESERVE_BIT_03_MIDPI_CAP_SEL)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_04_8PHASE_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_05_PSMUX_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_06_PSMUX_XLATCH_FORCEDQS)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_07_SMT_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_08_SMT_XLATCH_FORCE_DQS)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_09_BUFGP_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_10_BUFGP_XLATCH_FORCE_DQS)
										| P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_11_BYPASS_SR)
										| P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_12_BYPASS_SR_DQS)
										| P_Fld(0, RG_ARPI_RESERVE_BIT_13_CG_SYNC_ENB)
										| P_Fld(u1LP3_SEL, RG_ARPI_RESERVE_BIT_14_LP3_SEL)
										| P_Fld(u1PSMUX_DRV_SEL, RG_ARPI_RESERVE_BIT_15_PSMUX_DRV_SEL));


	vIO32WriteFldMulti_All(DDRPHY_SHU1_CA_DLL1, P_Fld(0x1, RG_ARCMD_REV_BIT_00_TX_LSH_DQ_CG_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_01_TX_LSH_DQS_CG_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_02_TX_LSH_DQM_CG_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_03_RX_DQS_GATE_EN_MODE)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_04_RX_DQSIEN_RB_DLY)
										| P_Fld(u1SER_RST_MODE, RG_ARCMD_REV_BIT_05_RX_SER_RST_MODE)
										| P_Fld(0x1, RG_ARCMD_REV_BIT_06_MCK4X_SEL_CKE0)
										| P_Fld(0x1, RG_ARCMD_REV_BIT_07_MCK4X_SEL_CKE1)
										| P_Fld(0x4, RG_ARCMD_REV_BIT_1208_TX_CKE_DRVN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_13_TX_DDR3_CKE_SEL)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_14_TX_DDR4_CKE_SEL)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_15_TX_DDR4P_CKE_SEL)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_1716_TX_LP4Y_SEL)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_18_RX_LP4Y_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_19_RX_DQSIEN_FORCE_ON_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_20_DATA_SWAP_EN)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_2221_DATA_SWAP)
										| P_Fld(0x0, RG_ARCMD_REV_BIT_23_NA));

	vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DQ6, P_Fld(0x1, RG_ARPI_RESERVE_BIT_00_TX_CG_EN) // RG_*RPI_RESERVE_B0[0] 1'b1 prevent leakage
										| P_Fld(0, RG_ARPI_RESERVE_BIT_01_DLL_FAST_PSJP)
										| P_Fld(u1HYST_SEL, RG_ARPI_RESERVE_BIT_02_HYST_SEL)
										| P_Fld(u1MIDPI_CAP_SEL2, RG_ARPI_RESERVE_BIT_03_MIDPI_CAP_SEL)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_04_8PHASE_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_05_PSMUX_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_06_PSMUX_XLATCH_FORCEDQS)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_07_SMT_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_08_SMT_XLATCH_FORCE_DQS)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_09_BUFGP_XLATCH_FORCE)
										| P_Fld(u1ARPI_BIT4to10, RG_ARPI_RESERVE_BIT_10_BUFGP_XLATCH_FORCE_DQS)
										| P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_11_BYPASS_SR)
										| P_Fld(u1Bypass_SR, RG_ARPI_RESERVE_BIT_12_BYPASS_SR_DQS)
										| P_Fld(0, RG_ARPI_RESERVE_BIT_13_CG_SYNC_ENB)
										| P_Fld(u1LP3_SEL, RG_ARPI_RESERVE_BIT_14_LP3_SEL)
										| P_Fld(u1PSMUX_DRV_SEL, RG_ARPI_RESERVE_BIT_15_PSMUX_DRV_SEL));

	//TX
	vIO32WriteFldMulti_All(DDRPHY_SHU1_B0_DLL1, P_Fld(0x0, RG_ARDQ_REV_BIT_00_DQS_MCK4X_DLY_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_01_DQS_MCK4XB_DLY_EN)
										| P_Fld(u1TX_READ_BASE_EN, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_03_RX_DQS_GATE_EN_MODE)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_04_RX_DQSIEN_RB_DLY)
										| P_Fld(u1SER_RST_MODE, RG_ARDQ_REV_BIT_05_RX_SER_RST_MODE)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_06_MCK4X_SEL_DQ1)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_07_MCK4X_SEL_DQ5)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE)
										| P_Fld(u1TX_READ_BASE_EN, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_1110_DRVN_PRE)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_1312_DRVP_PRE)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_14_TX_PRE_DATA_SEL)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_15_TX_PRE_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_1716_TX_LP4Y_SEL)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_18_RX_LP4Y_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_19_RX_DQSIEN_FORCE_ON_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_20_DATA_SWAP_EN)
										| P_Fld(0x0, RG_ARDQ_REV_BIT_2221_DATA_SWAP)
										| P_Fld(0x1, RG_ARDQ_REV_BIT_23_NA));


}

//#define PSRAM_MODEL_32MB

DRAM_STATUS_T PsramUpdateACTimingReg(DRAMC_CTX_T *p, const ACTimePsram_T *ACTbl);

void PSramcInitSetting(DRAMC_CTX_T *p)
{

	U8 u1CAP_SEL;
	U8 u1MIDPICAP_SEL;
	//U16 u2SDM_PCW = 0; // SDM_PCW are set in DDRPhyPLLSetting()
	U8 u1TXDLY_CMD;
	U16 u2Tmp;
	//mcSHOW_ERR_MSG("lzs debug PSramcInitSetting!!!\n");
	//PSRAM_AutoRefreshCKEOff(p);

	  //before switch clock from 26M to PHY, need to init PHY clock first
	vIO32WriteFldMulti_All(DDRPHY_CKMUX_SEL, P_Fld(0x1, CKMUX_SEL_R_PHYCTRLMUX)  //move CKMUX_SEL_R_PHYCTRLMUX to here (it was originally between MISC_CG_CTRL0_CLK_MEM_SEL and MISC_CTRL0_R_DMRDSEL_DIV2_OPT)
										| P_Fld(0x1, CKMUX_SEL_R_PHYCTRLDCM)); // PHYCTRLDCM 1: follow DDRPHY_conf DCM settings, 0: follow infra DCM settings

	//chg_mem_en = 1
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_W_CHG_MEM);
	//26M
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x0, MISC_CG_CTRL0_CLK_MEM_SEL);

	vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0x0, MISC_CTRL0_PSRAM_R_DMRDSEL_DIV2_OPT_PSRAM);
	//vIO32WriteFldAlign(DDRPHY_MISC_CTRL0, 0x0, MISC_CTRL0_R_DMRDSEL_DIV2_OPT);

	//					 0 ===LP4_3200_intial_setting_shu1 begin===
	//Francis : pin mux issue, need to set CHD
	vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL2, 0x0, MISC_SPM_CTRL2_PHY_SPM_CTL2);
	vIO32WriteFldAlign(DDRPHY_MISC_SPM_CTRL0, 0x0, MISC_SPM_CTRL0_PHY_SPM_CTL0);
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL2, 0x6003bf, MISC_CG_CTRL2_RG_MEM_DCM_CTL);
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL4_PSRAM, 0x333f3f00, MISC_CG_CTRL4_PSRAM_R_PHY_MCK_CG_CTRL_PSRAM);
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL4, 0x11400000, MISC_CG_CTRL4_R_PHY_MCK_CG_CTRL);

	vIO32WriteFldMulti(DDRPHY_SHU1_PLL1, P_Fld(0x1, SHU1_PLL1_R_SHU_AUTO_PLL_MUX)
				| P_Fld(0x7, SHU1_PLL1_SHU1_PLL1_RFU));
   // vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x1, SHU1_B0_DQ7_PSRAM_MIDPI_ENABLE_PSRAM)
   //			  | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_MIDPI_DIV4_ENABLE_PSRAM)
   //			  | P_Fld(0, SHU1_B0_DQ7_PSRAM_R_DMRANKRXDVS_B0_PSRAM));

	vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7, P_Fld(0x1, SHU1_B0_DQ7_MIDPI_ENABLE)
				| P_Fld(0x0, SHU1_B0_DQ7_MIDPI_DIV4_ENABLE)
				| P_Fld(0, SHU1_B0_DQ7_R_DMRANKRXDVS_B0));

	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x0, SHU1_B0_DQ7_PSRAM_R_DMRANKRXDVS_B0_PSRAM);

	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL4, 0xfe, SHU1_PLL4_RG_RPHYPLL_RESERVED);
	vIO32WriteFldMulti(DDRPHY_SHU1_PLL4, P_Fld(0x1, SHU1_PLL4_RG_RPHYPLL_IBIAS) | P_Fld(0x1, SHU1_PLL4_RG_RPHYPLL_ICHP) | P_Fld(0x2, SHU1_PLL4_RG_RPHYPLL_FS));
	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL6, 0xfe, SHU1_PLL6_RG_RCLRPLL_RESERVED);
	vIO32WriteFldMulti(DDRPHY_SHU1_PLL6, P_Fld(0x1, SHU1_PLL6_RG_RCLRPLL_IBIAS) | P_Fld(0x1, SHU1_PLL6_RG_RCLRPLL_ICHP) | P_Fld(0x2, SHU1_PLL6_RG_RCLRPLL_FS));
	vIO32WriteFldAlign(DDRPHY_SHU1_PLL14, 0x0, SHU1_PLL14_RG_RPHYPLL_SDM_SSC_PH_INIT);
	vIO32WriteFldAlign(DDRPHY_SHU1_PLL20, 0x0, SHU1_PLL20_RG_RCLRPLL_SDM_SSC_PH_INIT);
	vIO32WriteFldMulti(DDRPHY_CA_CMD2, P_Fld(0x0, CA_CMD2_RG_TX_ARCMD_OE_DIS)
				| P_Fld(0x0, CA_CMD2_RG_TX_ARCMD_ODTEN_DIS)
				| P_Fld(0x0, CA_CMD2_RG_TX_ARCLK_OE_DIS)
				| P_Fld(0x0, CA_CMD2_RG_TX_ARCLK_ODTEN_DIS));
	vIO32WriteFldMulti(DDRPHY_B0_DQ2, P_Fld(0x0, B0_DQ2_RG_TX_ARDQ_OE_DIS_B0)
				| P_Fld(0x0, B0_DQ2_RG_TX_ARDQ_ODTEN_DIS_B0)
				| P_Fld(0x0, B0_DQ2_RG_TX_ARDQS0_OE_DIS_B0)
				| P_Fld(0x0, B0_DQ2_RG_TX_ARDQS0_ODTEN_DIS_B0));
#if 0 //Correct settings are set in UpdateInitialSettings_LP4()
	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x0, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);
	vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x7, B1_DQ9_R_IN_GATE_EN_LOW_OPT_B1);
	vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_IN_GATE_EN_LOW_OPT_CA);
#endif

	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x0, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);
	vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMRXDVS_RDSEL_LAT_B0_PSRAM);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_R_DMRXDVS_RDSEL_LAT_B1);
	vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_DMRXDVS_RDSEL_LAT_CA);

	vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x1, B0_RXDVS0_R_RX_DLY_TRACK_CG_EN_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_RXDVS0, 0x1, B1_RXDVS0_R_RX_DLY_TRACK_CG_EN_B1);
	vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x1, B0_RXDVS0_R_DMRXDVS_DQIENPRE_OPT_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_RXDVS0, 0x1, B1_RXDVS0_R_DMRXDVS_DQIENPRE_OPT_B1);
	vIO32WriteFldAlign(DDRPHY_R0_B0_RXDVS2, 0x1, R0_B0_RXDVS2_R_RK0_DVS_FDLY_MODE_B0);
	//vIO32WriteFldAlign(DDRPHY_R1_B0_RXDVS2, 0x1, R1_B0_RXDVS2_R_RK1_DVS_FDLY_MODE_B0);
	vIO32WriteFldAlign(DDRPHY_R0_B1_RXDVS2, 0x1, R0_B1_RXDVS2_R_RK0_DVS_FDLY_MODE_B1);
	//vIO32WriteFldAlign(DDRPHY_R1_B1_RXDVS2, 0x1, R1_B1_RXDVS2_R_RK1_DVS_FDLY_MODE_B1);
	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x3, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ5, 0x3, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);
	vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS1, P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LEAD)
				| P_Fld(0x2, R0_B0_RXDVS1_R_RK0_B0_DVS_TH_LAG));
	//vIO32WriteFldMulti(DDRPHY_R1_B0_RXDVS1, P_Fld(0x2, R1_B0_RXDVS1_R_RK1_B0_DVS_TH_LEAD)
	//			| P_Fld(0x2, R1_B0_RXDVS1_R_RK1_B0_DVS_TH_LAG));
	vIO32WriteFldMulti(DDRPHY_R0_B1_RXDVS1, P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LEAD)
				| P_Fld(0x2, R0_B1_RXDVS1_R_RK0_B1_DVS_TH_LAG));
	//vIO32WriteFldMulti(DDRPHY_R1_B1_RXDVS1, P_Fld(0x2, R1_B1_RXDVS1_R_RK1_B1_DVS_TH_LEAD)
	//			| P_Fld(0x2, R1_B1_RXDVS1_R_RK1_B1_DVS_TH_LAG));

	vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS2, P_Fld(0x2, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0)
				| P_Fld(0x1, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0)
				| P_Fld(0x1, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0));
	//vIO32WriteFldMulti(DDRPHY_R1_B0_RXDVS2, P_Fld(0x2, R1_B0_RXDVS2_R_RK1_DVS_MODE_B0)
	//			| P_Fld(0x1, R1_B0_RXDVS2_R_RK1_RX_DLY_RIS_TRACK_GATE_ENA_B0)
	//			| P_Fld(0x1, R1_B0_RXDVS2_R_RK1_RX_DLY_FAL_TRACK_GATE_ENA_B0));
	vIO32WriteFldMulti(DDRPHY_R0_B1_RXDVS2, P_Fld(0x2, R0_B1_RXDVS2_R_RK0_DVS_MODE_B1)
				| P_Fld(0x1, R0_B1_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B1)
				| P_Fld(0x1, R0_B1_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B1));
	//vIO32WriteFldMulti(DDRPHY_R1_B1_RXDVS2, P_Fld(0x2, R1_B1_RXDVS2_R_RK1_DVS_MODE_B1)
	//			| P_Fld(0x1, R1_B1_RXDVS2_R_RK1_RX_DLY_RIS_TRACK_GATE_ENA_B1)
	//			| P_Fld(0x1, R1_B1_RXDVS2_R_RK1_RX_DLY_FAL_TRACK_GATE_ENA_B1));

	vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 0x0, B0_RXDVS0_R_RX_DLY_TRACK_CG_EN_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_RXDVS0, 0x0, B1_RXDVS0_R_RX_DLY_TRACK_CG_EN_B1);
	vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM);
	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0);

	//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1);
#if 0//LEGACY_DELAY_CELL
	LegacyDlyCellInitLP4_DDR3200(p);
#endif
#if 0 //lzs temp mark
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ7, P_Fld(0x1f, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0)
				| P_Fld(0x1f, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B1_DQ7, P_Fld(0x1f, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1)
				| P_Fld(0x1f, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	vIO32WriteFldMulti(DDRPHY_SHU1_R1_B0_DQ7, P_Fld(0x1f, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0)
				| P_Fld(0x1f, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R1_B1_DQ7, P_Fld(0x1f, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1)
				| P_Fld(0x1f, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
#endif
	vIO32WriteFldMulti(DDRPHY_B0_DQ4, P_Fld(0x10, B0_DQ4_RG_RX_ARDQS_EYE_R_DLY_B0)
				| P_Fld(0x10, B0_DQ4_RG_RX_ARDQS_EYE_F_DLY_B0));
	vIO32WriteFldMulti(DDRPHY_B0_DQ5, P_Fld(0x0, B0_DQ5_RG_RX_ARDQ_EYE_EN_B0)
				| P_Fld(0x0, B0_DQ5_RG_RX_ARDQ_EYE_SEL_B0)
				| P_Fld(0x1, B0_DQ5_RG_RX_ARDQ_VREF_EN_B0)
				| P_Fld(0x10, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0)
				| P_Fld(0x10, B0_DQ5_B0_DQ5_RFU));
	vIO32WriteFldMulti(DDRPHY_B0_DQ6, P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_EYE_DLY_DQS_BYPASS_B0)
				| P_Fld(0x1, B0_DQ6_RG_TX_ARDQ_DDR3_SEL_B0)
				| P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_DDR3_SEL_B0)
				| P_Fld(0x0, B0_DQ6_RG_TX_ARDQ_DDR4_SEL_B0)
				| P_Fld(0x0, B0_DQ6_RG_RX_ARDQ_DDR4_SEL_B0)
				| P_Fld(0x0, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0)
				| P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0)
				| P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0)
				| P_Fld(0x0, B0_DQ6_RG_TX_ARDQ_SER_MODE_B0));
	vIO32WriteFldMulti(DDRPHY_B0_DQ5, P_Fld(0x1, B0_DQ5_RG_RX_ARDQ_EYE_STBEN_RESETB_B0)
				| P_Fld(0x0, B0_DQ5_B0_DQ5_RFU));
	//vIO32WriteFldMulti(DDRPHY_B1_DQ4, P_Fld(0x10, B1_DQ4_RG_RX_ARDQS_EYE_R_DLY_B1)
	//			| P_Fld(0x10, B1_DQ4_RG_RX_ARDQS_EYE_F_DLY_B1));
	vIO32WriteFldMulti(DDRPHY_CA_CMD3, P_Fld(0x1, CA_CMD3_RG_RX_ARCMD_IN_BUFF_EN)
				| P_Fld(0x1, CA_CMD3_RG_ARCMD_RESETB)
				| P_Fld(0x1, CA_CMD3_RG_TX_ARCMD_EN));
	vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR4_SEL)
				| P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_BIAS_VREF_SEL)
				| P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN));
	/* ARCMD_DRVP, DRVN , ARCLK_DRVP, DRVN not used anymore
	vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD1, P_Fld(0x1, SHU1_CA_CMD1_RG_TX_ARCMD_DRVN)
				| P_Fld(0x1, SHU1_CA_CMD1_RG_TX_ARCMD_DRVP));
	vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD2, P_Fld(0x1, SHU1_CA_CMD2_RG_TX_ARCLK_DRVN)
				| P_Fld(0x1, SHU1_CA_CMD2_RG_TX_ARCLK_DRVP));
	 */
	//vIO32WriteFldMulti(DDRPHY_SHU2_CA_CMD1, P_Fld(0x1, SHU2_CA_CMD1_RG_TX_ARCMD_DRVN)
	//			  | P_Fld(0x1, SHU2_CA_CMD1_RG_TX_ARCMD_DRVP));
	//vIO32WriteFldMulti(DDRPHY_SHU2_CA_CMD2, P_Fld(0x1, SHU2_CA_CMD2_RG_TX_ARCLK_DRVN)
	//			  | P_Fld(0x1, SHU2_CA_CMD2_RG_TX_ARCLK_DRVP));

#if 0 //PSRAM_VREF_FROM_RTN
	vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x0, PLL3_RG_RPHYPLL_TSTOP_EN) | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
	vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, 0x1, MISC_VREF_CTRL_RG_RVREF_VREF_EN); //LP3 VREF
    vIO32WriteFldMulti_All(DDRPHY_SHU1_MISC0, P_Fld(14, SHU1_MISC0_RG_RVREF_SEL_CMD)
		| P_Fld(0x1, SHU1_MISC0_RG_RVREF_DDR3_SEL)
		| P_Fld(0x0, SHU1_MISC0_RG_RVREF_DDR4_SEL)
		| P_Fld(14, SHU1_MISC0_RG_RVREF_SEL_DQ));
#else
	vIO32WriteFldMulti(DDRPHY_PLL3, P_Fld(0x0, PLL3_RG_RPHYPLL_TSTOP_EN) | P_Fld(0x1, PLL3_RG_RPHYPLL_TST_EN));
	vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, 0x1, MISC_VREF_CTRL_RG_RVREF_VREF_EN); //LP3 VREF
	vIO32WriteFldAlign(DDRPHY_MISC_IMP_CTRL1, 0x1, MISC_IMP_CTRL1_RG_RIMP_SUS_ECO_OPT);
#endif

	vIO32WriteFldAlign(DDRPHY_B0_DQ3, 0x1, B0_DQ3_RG_ARDQ_RESETB_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ3, 0x1, B1_DQ3_RG_ARDQ_RESETB_B1);

	mcDELAY_US(1);

	//Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
	vIO32WriteFldMulti(DDRPHY_SHU1_PLL8, P_Fld(0x0, SHU1_PLL8_RG_RPHYPLL_POSDIV) | P_Fld(0x1, SHU1_PLL8_RG_RPHYPLL_PREDIV));
	//vIO32WriteFldAlign(DDRPHY_SHU2_PLL8, 0x0, SHU2_PLL8_RG_RPHYPLL_POSDIV);
	//vIO32WriteFldAlign(DDRPHY_SHU3_PLL8, 0x0, SHU3_PLL8_RG_RPHYPLL_POSDIV);
	//vIO32WriteFldAlign(DDRPHY_SHU4_PLL8, 0x0, SHU4_PLL8_RG_RPHYPLL_POSDIV);

	mcDELAY_US(1);

	vIO32WriteFldMulti(DDRPHY_SHU1_PLL9, P_Fld(0x0, SHU1_PLL9_RG_RPHYPLL_MONCK_EN)
				| P_Fld(0x0, SHU1_PLL9_RG_RPHYPLL_MONVC_EN)
				| P_Fld(0x1, SHU1_PLL9_RG_RPHYPLL_LVROD_EN)
				| P_Fld(0x2, SHU1_PLL9_RG_RPHYPLL_RST_DLY));
	vIO32WriteFldMulti(DDRPHY_SHU1_PLL11, P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_MONCK_EN)
				| P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_MONVC_EN)
				| P_Fld(0x0, SHU1_PLL11_RG_RCLRPLL_LVROD_EN)
				| P_Fld(0x1, SHU1_PLL11_RG_RCLRPLL_RST_DLY));

	mcDELAY_US(1);

	//Ref clock should be 20M~30M, if MPLL=52M, Pre-divider should be set to 1
	vIO32WriteFldMulti(DDRPHY_SHU1_PLL10, P_Fld(0x0, SHU1_PLL10_RG_RCLRPLL_POSDIV) | P_Fld(0x1, SHU1_PLL10_RG_RCLRPLL_PREDIV));
	//vIO32WriteFldAlign(DDRPHY_SHU2_PLL10, 0x0, SHU2_PLL10_RG_RCLRPLL_POSDIV);
	//vIO32WriteFldAlign(DDRPHY_SHU3_PLL10, 0x0, SHU3_PLL10_RG_RCLRPLL_POSDIV);
	//vIO32WriteFldAlign(DDRPHY_SHU4_PLL10, 0x0, SHU4_PLL10_RG_RCLRPLL_POSDIV);

	mcDELAY_US(1);


	///TODO: MIDPI Init 1
	vIO32WriteFldMulti(DDRPHY_PLL4, P_Fld(0x0, PLL4_RG_RPHYPLL_AD_MCK8X_EN)
				| P_Fld(0x1, PLL4_PLL4_RFU)
				| P_Fld(0x1, PLL4_RG_RPHYPLL_MCK8X_SEL));


	mcDELAY_US(1);

	vIO32WriteFldAlign(DDRPHY_SHU1_PLL0, 0x3, SHU1_PLL0_RG_RPHYPLL_TOP_REV); // debug1111, org:3 -> mdf:0
	//vIO32WriteFldAlign(DDRPHY_SHU2_PLL0, 0x3, SHU2_PLL0_RG_RPHYPLL_TOP_REV);
	//vIO32WriteFldAlign(DDRPHY_SHU3_PLL0, 0x3, SHU3_PLL0_RG_RPHYPLL_TOP_REV);
	//vIO32WriteFldAlign(DDRPHY_SHU4_PLL0, 0x3, SHU4_PLL0_RG_RPHYPLL_TOP_REV);

	mcDELAY_US(1);


	//vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI1, 0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA);
	vIO32WriteFldMulti(DDRPHY_B0_DQ3, P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_STBENCMP_EN_B0)
				| P_Fld(0x1, B0_DQ3_RG_TX_ARDQ_EN_B0)
				| P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0));



#if (fcFOR_CHIP_ID == fcSchubert)
#if 1//#ifndef BIANCO_TO_BE_PORTING
	vIO32WriteFldAlign(DDRPHY_SHU1_CA_DLL0, 0x1, SHU1_CA_DLL0_RG_ARPISM_MCK_SEL_CA_SHU);
	//vIO32WriteFldAlign(DDRPHY_SHU1_CA_DLL0+SHIFT_TO_CHB_ADDR, 0x1, SHU1_CA_DLL0_RG_ARPISM_MCK_SEL_CA_SHU);
#endif
#endif

	vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL0, P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_PHDET_IN_SWAP_B0)
				| P_Fld(0x7, SHU1_B0_DLL0_RG_ARDLL_GAIN_B0)
				| P_Fld(0x7, SHU1_B0_DLL0_RG_ARDLL_IDLECNT_B0)
				| P_Fld(0x8, SHU1_B0_DLL0_RG_ARDLL_P_GAIN_B0)
				| P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_PHJUMP_EN_B0)
				| P_Fld(0x1, SHU1_B0_DLL0_RG_ARDLL_PHDIV_B0)
				| P_Fld(0x0, SHU1_B0_DLL0_RG_ARDLL_FAST_PSJP_B0));

	vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD5, 0x0, SHU1_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
	//vIO32WriteFldAlign(DDRPHY_SHU2_CA_CMD5, 0x0, SHU2_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
	//vIO32WriteFldAlign(DDRPHY_SHU3_CA_CMD5, 0x0, SHU3_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
	//vIO32WriteFldAlign(DDRPHY_SHU4_CA_CMD5, 0x0, SHU4_CA_CMD5_RG_RX_ARCMD_VREF_SEL);
	vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD0, P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCMD_PRE_EN)
				| P_Fld(0x4, SHU1_CA_CMD0_RG_TX_ARCLK_DRVN_PRE)
				| P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCLK_PRE_EN));
	//vIO32WriteFldMulti(DDRPHY_SHU2_CA_CMD0, P_Fld(0x1, SHU2_CA_CMD0_RG_TX_ARCMD_PRE_EN)
	//			  | P_Fld(0x4, SHU2_CA_CMD0_RG_TX_ARCLK_DRVN_PRE)
	//			  | P_Fld(0x1, SHU2_CA_CMD0_RG_TX_ARCLK_PRE_EN));
	//vIO32WriteFldMulti(DDRPHY_SHU3_CA_CMD0, P_Fld(0x1, SHU3_CA_CMD0_RG_TX_ARCMD_PRE_EN)
	//			  | P_Fld(0x4, SHU3_CA_CMD0_RG_TX_ARCLK_DRVN_PRE)
	//			  | P_Fld(0x1, SHU3_CA_CMD0_RG_TX_ARCLK_PRE_EN));
	//vIO32WriteFldMulti(DDRPHY_SHU4_CA_CMD0, P_Fld(0x1, SHU4_CA_CMD0_RG_TX_ARCMD_PRE_EN)
	//			  | P_Fld(0x4, SHU4_CA_CMD0_RG_TX_ARCLK_DRVN_PRE)
	//			  | P_Fld(0x1, SHU4_CA_CMD0_RG_TX_ARCLK_PRE_EN));
#if (fcFOR_CHIP_ID == fcSchubert)
	vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6, 0x3, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
	//vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6+SHIFT_TO_CHB_ADDR, 0x1, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
#else
	vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD6, 0x3, SHU1_CA_CMD6_RG_ARPI_RESERVE_CA);
#endif
	//vIO32WriteFldAlign(DDRPHY_SHU2_CA_CMD6, 0x3, SHU2_CA_CMD6_RG_ARPI_RESERVE_CA);
	//vIO32WriteFldAlign(DDRPHY_SHU3_CA_CMD6, 0x3, SHU3_CA_CMD6_RG_ARPI_RESERVE_CA);
	//vIO32WriteFldAlign(DDRPHY_SHU4_CA_CMD6, 0x3, SHU4_CA_CMD6_RG_ARPI_RESERVE_CA);
	//vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD3, 0x4e1, SHU1_CA_CMD3_RG_ARCMD_REV);
	//vIO32WriteFldAlign(DDRPHY_SHU2_CA_CMD7, 0x4e1, SHU2_CA_CMD7_RG_ARCMD_REV);
	//vIO32WriteFldAlign(DDRPHY_SHU3_CA_CMD7, 0x4e1, SHU3_CA_CMD7_RG_ARCMD_REV);
	//vIO32WriteFldAlign(DDRPHY_SHU4_CA_CMD7, 0x4e1, SHU4_CA_CMD7_RG_ARCMD_REV);
	//vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7, P_Fld(0x00, SHU1_B0_DQ7_RG_ARDQ_REV_B0)
	//			  | P_Fld(0x0, SHU1_B0_DQ7_DQ_REV_B0_BIT_05));
	//vIO32WriteFldAlign(DDRPHY_SHU2_B0_DQ7, 0x20, SHU2_B0_DQ7_RG_ARDQ_REV_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU3_B0_DQ7, 0x20, SHU3_B0_DQ7_RG_ARDQ_REV_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU4_B0_DQ7, 0x20, SHU4_B0_DQ7_RG_ARDQ_REV_B0);
	//vIO32WriteFldMulti(DDRPHY_SHU1_B1_DQ7, P_Fld(0x00, SHU1_B1_DQ7_RG_ARDQ_REV_B1)
	//			  | P_Fld(0x0, SHU1_B1_DQ7_DQ_REV_B1_BIT_05));

 //lynx added
	//vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7, 0x0, SHU1_B0_DQ7_RG_ARDQ_REV_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x0, SHU1_B1_DQ7_RG_ARDQ_REV_B1);
 //
	//vIO32WriteFldAlign(DDRPHY_SHU2_B1_DQ7, 0x20, SHU2_B1_DQ7_RG_ARDQ_REV_B1);
	//vIO32WriteFldAlign(DDRPHY_SHU3_B1_DQ7, 0x20, SHU3_B1_DQ7_RG_ARDQ_REV_B1);
	//vIO32WriteFldAlign(DDRPHY_SHU4_B1_DQ7, 0x20, SHU4_B1_DQ7_RG_ARDQ_REV_B1);
	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ6, 0x1, SHU1_B0_DQ6_RG_ARPI_RESERVE_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU2_B0_DQ6, 0x1, SHU2_B0_DQ6_RG_ARPI_RESERVE_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU3_B0_DQ6, 0x1, SHU3_B0_DQ6_RG_ARPI_RESERVE_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU4_B0_DQ6, 0x1, SHU4_B0_DQ6_RG_ARPI_RESERVE_B0);
	//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ6, 0x1, SHU1_B1_DQ6_RG_ARPI_RESERVE_B1);
	//vIO32WriteFldAlign(DDRPHY_SHU2_B1_DQ6, 0x1, SHU2_B1_DQ6_RG_ARPI_RESERVE_B1);
	//vIO32WriteFldAlign(DDRPHY_SHU3_B1_DQ6, 0x1, SHU3_B1_DQ6_RG_ARPI_RESERVE_B1);
	//vIO32WriteFldAlign(DDRPHY_SHU4_B1_DQ6, 0x1, SHU4_B1_DQ6_RG_ARPI_RESERVE_B1);
	vIO32WriteFldMulti(DDRPHY_MISC_SHU_OPT, P_Fld(0x1, MISC_SHU_OPT_R_CA_SHU_PHDET_SPM_EN)
				| P_Fld(0x1, MISC_SHU_OPT_R_CA_SHU_PHY_GATING_RESETB_SPM_EN)
				| P_Fld(0x2, MISC_SHU_OPT_R_DQB1_SHU_PHDET_SPM_EN)
				| P_Fld(0x1, MISC_SHU_OPT_R_DQB1_SHU_PHY_GATING_RESETB_SPM_EN)
				| P_Fld(0x2, MISC_SHU_OPT_R_DQB0_SHU_PHDET_SPM_EN)
				| P_Fld(0x1, MISC_SHU_OPT_R_DQB0_SHU_PHY_GATING_RESETB_SPM_EN));

	mcDELAY_US(9);

#if (fcFOR_CHIP_ID == fcSchubert)
	vIO32WriteFldMulti(DDRPHY_SHU1_CA_DLL1, P_Fld(0x1, SHU1_CA_DLL1_RG_ARDLL_PD_CK_SEL_CA) | P_Fld(0x0, SHU1_CA_DLL1_RG_ARDLL_FASTPJ_CK_SEL_CA));
#endif
	vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(0x0, SHU1_B0_DLL1_RG_ARDLL_PD_CK_SEL_B0) | P_Fld(0x1, SHU1_B0_DLL1_RG_ARDLL_FASTPJ_CK_SEL_B0));
	//vIO32WriteFldMulti(DDRPHY_SHU1_B1_DLL1, P_Fld(0x0, SHU1_B1_DLL1_RG_ARDLL_PD_CK_SEL_B1) | P_Fld(0x1, SHU1_B1_DLL1_RG_ARDLL_FASTPJ_CK_SEL_B1));

	mcDELAY_US(1);

	vIO32WriteFldAlign(DDRPHY_PLL2, 0x0, PLL2_RG_RCLRPLL_EN);
	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL4, 0xff, SHU1_PLL4_RG_RPHYPLL_RESERVED);
	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL6, 0xff, SHU1_PLL6_RG_RCLRPLL_RESERVED);
	vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0xf, MISC_CG_CTRL0_CLK_MEM_DFS_CFG);

	mcDELAY_US(1);

	PsramDDRPhyReservedRGSetting(p);
	PsramDDRPhyPLLSetting(p);

#if 0
	//rollback tMRRI design change
#if ENABLE_TMRRI_NEW_MODE
	//fix rank at 0 to trigger new TMRRI setting
	vIO32WriteFldAlign(DRAMC_REG_RKCFG, 0, RKCFG_TXRANK);
	vIO32WriteFldAlign(DRAMC_REG_RKCFG, 1, RKCFG_TXRANKFIX);
	vIO32WriteFldAlign(DRAMC_REG_DRSCTRL, 0x0, DRSCTRL_RK_SCINPUT_OPT);// new mode, HW_MRR: R_DMMRRRK, SW_MRR: R_DMMRSRK
	vIO32WriteFldMulti(DRAMC_REG_DRAMCTRL, P_Fld(0x0, DRAMCTRL_MRRIOPT) | P_Fld(0x0, DRAMCTRL_TMRRIBYRK_DIS) | P_Fld(0x1, DRAMCTRL_TMRRICHKDIS));
	vIO32WriteFldAlign(DRAMC_REG_SPCMDCTRL, 0x1, SPCMDCTRL_SC_PG_UPD_OPT);
	vIO32WriteFldMulti(DRAMC_REG_SPCMDCTRL, P_Fld(0x0, SPCMDCTRL_SC_PG_MPRW_DIS) | P_Fld(0x0, SPCMDCTRL_SC_PG_STCMD_AREF_DIS)
											 | P_Fld(0x0, SPCMDCTRL_SC_PG_OPT2_DIS) | P_Fld(0x0, SPCMDCTRL_SC_PG_MAN_DIS));
	vIO32WriteFldMulti(DRAMC_REG_MPC_OPTION, P_Fld(0x1, MPC_OPTION_ZQ_BLOCKALE_OPT) | P_Fld(0x1, MPC_OPTION_MPC_BLOCKALE_OPT2) |
											   P_Fld(0x1, MPC_OPTION_MPC_BLOCKALE_OPT1) | P_Fld(0x1, MPC_OPTION_MPC_BLOCKALE_OPT));
	//fix rank at 0 to trigger new TMRRI setting
	vIO32WriteFldAlign(DRAMC_REG_RKCFG, 0, RKCFG_TXRANK);
	vIO32WriteFldAlign(DRAMC_REG_RKCFG, 0, RKCFG_TXRANKFIX);
#else
	vIO32WriteFldAlign(DRAMC_REG_DRSCTRL, 0x1, DRSCTRL_RK_SCINPUT_OPT);// old mode, HW/SW MRR: R_DMMRRRK
	vIO32WriteFldMulti(DRAMC_REG_DRAMCTRL, P_Fld(0x1, DRAMCTRL_MRRIOPT) | P_Fld(0x1, DRAMCTRL_TMRRIBYRK_DIS) | P_Fld(0x0, DRAMCTRL_TMRRICHKDIS));
	vIO32WriteFldAlign(DRAMC_REG_SPCMDCTRL, 0x0, SPCMDCTRL_SC_PG_UPD_OPT);
	vIO32WriteFldMulti(DRAMC_REG_SPCMDCTRL, P_Fld(0x1, SPCMDCTRL_SC_PG_MPRW_DIS) | P_Fld(0x1, SPCMDCTRL_SC_PG_STCMD_AREF_DIS)
											 | P_Fld(0x1, SPCMDCTRL_SC_PG_OPT2_DIS) | P_Fld(0x1, SPCMDCTRL_SC_PG_MAN_DIS));
#endif
	vIO32WriteFldAlign(DRAMC_REG_CKECTRL, 0x1, CKECTRL_RUNTIMEMRRCKEFIX);//Set Run time MRR CKE fix to 1 in tMRRI old mode to avoid no ACK from precharge all
	vIO32WriteFldAlign(DRAMC_REG_CKECTRL, 0x0, CKECTRL_RUNTIMEMRRMIODIS);
#endif
	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0);

	vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1);
#if 0
	vIO32WriteFldMulti(DRAMC_REG_SHURK1_DQSIEN, P_Fld(0xf, SHURK1_DQSIEN_R1DQS3IEN)
				| P_Fld(0xf, SHURK1_DQSIEN_R1DQS2IEN)
				| P_Fld(0xf, SHURK1_DQSIEN_R1DQS1IEN)
				| P_Fld(0xf, SHURK1_DQSIEN_R1DQS0IEN));
	vIO32WriteFldMulti(DRAMC_REG_STBCAL1, P_Fld(0x0, STBCAL1_DLLFRZ_MON_PBREF_OPT)
				| P_Fld(0x1, STBCAL1_STB_FLAGCLR)
				| P_Fld(0x1, STBCAL1_STBCNT_MODESEL));
	vIO32WriteFldMulti(DRAMC_REG_SHU_DQSG_RETRY, P_Fld(0x1, SHU_DQSG_RETRY_R_RETRY_USE_BURST_MDOE)
				| P_Fld(0x1, SHU_DQSG_RETRY_R_RDY_SEL_DLE)
				| P_Fld(0x6, SHU_DQSG_RETRY_R_DQSIENLAT)
				| P_Fld(0x1, SHU_DQSG_RETRY_R_RETRY_ONCE));
	vIO32WriteFldMulti(DRAMC_REG_SHU1_DRVING1, P_Fld(0xa, SHU1_DRVING1_DQSDRVP2) | P_Fld(0xa, SHU1_DRVING1_DQSDRVN2)
				| P_Fld(0xa, SHU1_DRVING1_DQSDRVP1) | P_Fld(0xa, SHU1_DRVING1_DQSDRVN1)
				| P_Fld(0xa, SHU1_DRVING1_DQDRVP2) | P_Fld(0xa, SHU1_DRVING1_DQDRVN2));
	vIO32WriteFldMulti(DRAMC_REG_SHU1_DRVING2, P_Fld(0xa, SHU1_DRVING2_DQDRVP1) | P_Fld(0xa, SHU1_DRVING2_DQDRVN1)
				| P_Fld(0xa, SHU1_DRVING2_CMDDRVP2) | P_Fld(0xa, SHU1_DRVING2_CMDDRVN2)
				| P_Fld(0xa, SHU1_DRVING2_CMDDRVP1) | P_Fld(0xa, SHU1_DRVING2_CMDDRVN1));
	vIO32WriteFldMulti(DRAMC_REG_SHU1_DRVING3, P_Fld(0xa, SHU1_DRVING3_DQSODTP2) | P_Fld(0xa, SHU1_DRVING3_DQSODTN2)
				| P_Fld(0xa, SHU1_DRVING3_DQSODTP) | P_Fld(0xa, SHU1_DRVING3_DQSODTN)
				| P_Fld(0xa, SHU1_DRVING3_DQODTP2) | P_Fld(0xa, SHU1_DRVING3_DQODTN2));
	vIO32WriteFldMulti(DRAMC_REG_SHU1_DRVING4, P_Fld(0xa, SHU1_DRVING4_DQODTP1) | P_Fld(0xa, SHU1_DRVING4_DQODTN1)
				| P_Fld(0xa, SHU1_DRVING4_CMDODTP2) | P_Fld(0xa, SHU1_DRVING4_CMDODTN2)
				| P_Fld(0xa, SHU1_DRVING4_CMDODTP1) | P_Fld(0xa, SHU1_DRVING4_CMDODTN1));
	/*
	vIO32WriteFldMulti(DRAMC_REG_SHU2_DRVING1, P_Fld(0x14a, SHU2_DRVING1_DQSDRV2)
				| P_Fld(0x14a, SHU2_DRVING1_DQSDRV1)
				| P_Fld(0x14a, SHU2_DRVING1_DQDRV2));
	vIO32WriteFldMulti(DRAMC_REG_SHU2_DRVING2, P_Fld(0x14a, SHU2_DRVING2_DQDRV1)
				| P_Fld(0x14a, SHU2_DRVING2_CMDDRV2)
				| P_Fld(0x14a, SHU2_DRVING2_CMDDRV1));
	vIO32WriteFldMulti(DRAMC_REG_SHU2_DRVING3, P_Fld(0x14a, SHU2_DRVING3_DQSODT2)
				| P_Fld(0x14a, SHU2_DRVING3_DQSODT1)
				| P_Fld(0x14a, SHU2_DRVING3_DQODT2));
	vIO32WriteFldMulti(DRAMC_REG_SHU2_DRVING4, P_Fld(0x14a, SHU2_DRVING4_DQODT1)
				| P_Fld(0x14a, SHU2_DRVING4_CMDODT2)
				| P_Fld(0x14a, SHU2_DRVING4_CMDODT1));
	vIO32WriteFldMulti(DRAMC_REG_SHU3_DRVING1, P_Fld(0x14a, SHU3_DRVING1_DQSDRV2)
				| P_Fld(0x14a, SHU3_DRVING1_DQSDRV1)
				| P_Fld(0x14a, SHU3_DRVING1_DQDRV2));
	vIO32WriteFldMulti(DRAMC_REG_SHU3_DRVING2, P_Fld(0x14a, SHU3_DRVING2_DQDRV1)
				| P_Fld(0x14a, SHU3_DRVING2_CMDDRV2)
				| P_Fld(0x14a, SHU3_DRVING2_CMDDRV1));
	vIO32WriteFldMulti(DRAMC_REG_SHU3_DRVING3, P_Fld(0x14a, SHU3_DRVING3_DQSODT2)
				| P_Fld(0x14a, SHU3_DRVING3_DQSODT1)
				| P_Fld(0x14a, SHU3_DRVING3_DQODT2));
	vIO32WriteFldMulti(DRAMC_REG_SHU3_DRVING4, P_Fld(0x14a, SHU3_DRVING4_DQODT1)
				| P_Fld(0x14a, SHU3_DRVING4_CMDODT2)
				| P_Fld(0x14a, SHU3_DRVING4_CMDODT1));
	vIO32WriteFldMulti(DRAMC_REG_SHU4_DRVING1, P_Fld(0x14a, SHU4_DRVING1_DQSDRV2)
				| P_Fld(0x14a, SHU4_DRVING1_DQSDRV1)
				| P_Fld(0x14a, SHU4_DRVING1_DQDRV2));
	vIO32WriteFldMulti(DRAMC_REG_SHU4_DRVING2, P_Fld(0x14a, SHU4_DRVING2_DQDRV1)
				| P_Fld(0x14a, SHU4_DRVING2_CMDDRV2)
				| P_Fld(0x14a, SHU4_DRVING2_CMDDRV1));
	*/
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x08a8)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x08ac)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x08b0)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x08b4)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x0ea8)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x0eac)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x0eb0)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x0eb4)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x14a8)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x14ac)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x14b0)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x14b4)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x1aa8)) = 0x14a5294a;
	//	*((UINT32P)(DRAMC1_AO_BASE + 0x1aac)) = 0x14a5294a;
	vIO32WriteFldMulti(DRAMC_REG_SHUCTRL2, P_Fld(0x0, SHUCTRL2_HWSET_WLRL)
				| P_Fld(0x1, SHUCTRL2_SHU_PERIOD_GO_ZERO_CNT)
				| P_Fld(0x1, SHUCTRL2_R_DVFS_OPTION)
				| P_Fld(0x1, SHUCTRL2_R_DVFS_PARK_N)
				| P_Fld(0x1, SHUCTRL2_R_DVFS_DLL_CHA)
				| P_Fld(0xa, SHUCTRL2_R_DLL_IDLE));
	vIO32WriteFldAlign(DRAMC_REG_DVFSDLL, 0x1, DVFSDLL_DLL_LOCK_SHU_EN);
	vIO32WriteFldMulti(DRAMC_REG_DDRCONF0, P_Fld(0x1, DDRCONF0_LPDDR4EN)
				| P_Fld(0x1, DDRCONF0_DM64BITEN)
				| P_Fld(0x1, DDRCONF0_BC4OTF)
				| P_Fld(0x1, DDRCONF0_BK8EN));
	vIO32WriteFldMulti(DRAMC_REG_STBCAL2, P_Fld(0x1, STBCAL2_STB_GERR_B01)
				| P_Fld(0x1, STBCAL2_STB_GERRSTOP)
				| P_Fld(0x1, EYESCAN_EYESCAN_RD_SEL_OPT));
	vIO32WriteFldAlign(DRAMC_REG_STBCAL2, 0x1, STBCAL2_STB_GERR_RST);
	vIO32WriteFldAlign(DRAMC_REG_STBCAL2, 0x0, STBCAL2_STB_GERR_RST);
	vIO32WriteFldAlign(DRAMC_REG_CLKAR, 0x1, CLKAR_PSELAR);
#endif
	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_R_DMDQSIEN_RDSEL_LAT_B0);
	vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMDQSIEN_RDSEL_LAT_B0_PSRAM);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_R_DMDQSIEN_RDSEL_LAT_B1);
	vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_DMDQSIEN_RDSEL_LAT_CA);
#if 0
	if (vGet_Dram_CBT_Mode(p) == CBT_BYTE_MODE1)
	{
		vIO32WriteFldMulti(DDRPHY_MISC_CTRL0_PSRAM, P_Fld(0x0, MISC_CTRL0_PSRAM_R_STBENCMP_DIV4CK_EN_PSRAM)
					| P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMDQSIEN_FIFO_EN_PSRAM)
					| P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMSTBEN_OUTSEL_PSRAM)
					| P_Fld(0xf, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM));
	}
	else
#endif
	{
		vIO32WriteFldMulti(DDRPHY_MISC_CTRL0_PSRAM, P_Fld(0x0, MISC_CTRL0_PSRAM_R_STBENCMP_DIV4CK_EN_PSRAM)
					| P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMDQSIEN_FIFO_EN_PSRAM)
					| P_Fld(0x1, MISC_CTRL0_PSRAM_R_DMSTBEN_OUTSEL_PSRAM)
					| P_Fld(0xf, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM));

		vIO32WriteFldMulti(DDRPHY_MISC_CTRL0, P_Fld(0x0, MISC_CTRL0_R_STBENCMP_DIV4CK_EN)
			| P_Fld(0x1, MISC_CTRL0_R_DMDQSIEN_FIFO_EN)
			| P_Fld(0x1, MISC_CTRL0_IMPCAL_CDC_ECO_OPT)
			| P_Fld(0x1, MISC_CTRL0_IMPCAL_LP_ECO_OPT)
			| P_Fld(0x1, MISC_CTRL0_R_DMSTBEN_OUTSEL)
			| P_Fld(0x0, MISC_CTRL0_R_DMDQSIEN_SYNCOPT));
	}
	//vIO32WriteFldMulti(DDRPHY_MISC_CTRL1, P_Fld(0x1, MISC_CTRL1_R_DMDA_RRESETB_E) //Already set in vDramcInit_PreSettings()

	vIO32WriteFldAlign(DDRPHY_B0_RXDVS0, 1, B0_RXDVS0_R_HWSAVE_MODE_ENA_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_RXDVS0, 1, B1_RXDVS0_R_HWSAVE_MODE_ENA_B1);
	vIO32WriteFldAlign(DDRPHY_CA_RXDVS0, 0, CA_RXDVS0_R_HWSAVE_MODE_ENA_CA);

	vIO32WriteFldAlign(DDRPHY_CA_CMD7, 0x0, CA_CMD7_RG_TX_ARCMD_PULL_DN);
	vIO32WriteFldAlign(DDRPHY_CA_CMD7, 0x0, CA_CMD7_RG_TX_ARCS_PULL_DN); // Added by Lingyun.Wu, 11-15
	vIO32WriteFldAlign(DDRPHY_B0_DQ7, 0x0, B0_DQ7_RG_TX_ARDQ_PULL_DN_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ7, 0x0, B1_DQ7_RG_TX_ARDQ_PULL_DN_B1);
	//vIO32WriteFldAlign(DDRPHY_CA_CMD8, 0x0, CA_CMD8_RG_TX_RRESETB_PULL_DN); //Already set in vDramcInit_PreSettings()
#if 0
	vIO32WriteFldMulti(DRAMC_REG_SHU_CONF0, P_Fld(0x2, SHU_CONF0_MATYPE)
				| P_Fld(0x1, SHU_CONF0_BL4)
				| P_Fld(0x1, SHU_CONF0_FREQDIV4)
				| P_Fld(0x1, SHU_CONF0_REFTHD)
				| P_Fld(0x1, SHU_CONF0_ADVPREEN)
				| P_Fld(0x3f, SHU_CONF0_DMPGTIM));
	vIO32WriteFldMulti(DRAMC_REG_SHU_ODTCTRL, P_Fld(0x1, SHU_ODTCTRL_RODTE)
				| P_Fld(0x1, SHU_ODTCTRL_RODTE2)
				| P_Fld(0x1, SHU_ODTCTRL_TWODT)
				//| P_Fld(0x5, SHU_ODTCTRL_RODT) //Set in UpdateACTimingReg()
				| P_Fld(0x1, SHU_ODTCTRL_WOEN)
				| P_Fld(0x1, SHU_ODTCTRL_ROEN));
#endif

	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7, 0x1, SHU1_B0_DQ7_R_DMRODTEN_B0);

	vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x1, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);
	//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x1, SHU1_B1_DQ7_R_DMRODTEN_B1);
#if 0 //lzs temp mark
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B1_DQ7, P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1)
				| P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ7, P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0)
				| P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R1_B1_DQ7, P_Fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1)
				| P_Fld(0x14, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
	vIO32WriteFldMulti(DDRPHY_SHU1_R1_B0_DQ7, P_Fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0)
				| P_Fld(0x14, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
#endif
	mcDELAY_US(1);

	//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_RG_RX_ARDQS0_DQSIENMODE_B1);

	vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_RG_RX_ARDQS0_DQSIENMODE_B0);
	vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_RG_RX_ARDQS0_DQSIENMODE_B0_PSRAM);
	vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);
	//vIO32WriteFldAlign(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B1);
#if 0
	vIO32WriteFldAlign(DRAMC_REG_STBCAL, 0x1, STBCAL_DQSIENMODE);
	vIO32WriteFldMulti(DRAMC_REG_SREFCTRL, P_Fld(0x0, SREFCTRL_SREF_HW_EN)
				| P_Fld(0x8, SREFCTRL_SREFDLY));
	vIO32WriteFldMulti(DRAMC_REG_SHU_CKECTRL, P_Fld(0x3, SHU_CKECTRL_SREF_CK_DLY)
				| P_Fld(0x3, SHU_CKECTRL_TCKESRX));
				//| P_Fld(0x3, SHU_CKECTRL_CKEPRD));
	vIO32WriteFldMulti(DRAMC_REG_SHU_PIPE, P_Fld(0x1, SHU_PIPE_READ_START_EXTEND1)
				| P_Fld(0x1, SHU_PIPE_DLE_LAST_EXTEND1));
	vIO32WriteFldMulti(DRAMC_REG_CKECTRL, P_Fld(0x1, CKECTRL_CKEON)
				| P_Fld(0x1, CKECTRL_CKETIMER_SEL));
	vIO32WriteFldAlign(DRAMC_REG_RKCFG, 0x1, RKCFG_CKE2RANK_OPT2);
	if (vGet_Dram_CBT_Mode(p) == CBT_BYTE_MODE1)
	{
		vIO32WriteFldMulti(DRAMC_REG_SHU_CONF2, P_Fld(0x1, SHU_CONF2_WPRE2T)
					| P_Fld(0x7, SHU_CONF2_DCMDLYREF));
					//| P_Fld(0x64, SHU_CONF2_FSPCHG_PRDCNT)); //ACTiming related -> set in UpdateACTiming_Reg()
		vIO32WriteFldAlign(DRAMC_REG_SPCMDCTRL, 0x1, SPCMDCTRL_CLR_EN);
		//vIO32WriteFldAlign(DRAMC_REG_SHU_SCINTV, 0xf, SHU_SCINTV_MRW_INTV); (Set in UpdateACTimingReg())
		vIO32WriteFldAlign(DRAMC_REG_SHUCTRL1, 0x40, SHUCTRL1_FC_PRDCNT);
	}
	else
	{
		vIO32WriteFldMulti(DRAMC_REG_SHU_CONF2, P_Fld(0x1, SHU_CONF2_WPRE2T)
					| P_Fld(0x7, SHU_CONF2_DCMDLYREF));
					//| P_Fld(0x64, SHU_CONF2_FSPCHG_PRDCNT)); //ACTiming related -> set in UpdateACTiming_Reg()
		vIO32WriteFldAlign(DRAMC_REG_SPCMDCTRL, 0x1, SPCMDCTRL_CLR_EN);
		//vIO32WriteFldAlign(DRAMC_REG_SHU_SCINTV, 0xf, SHU_SCINTV_MRW_INTV); (Set in UpdateACTimingReg())
		vIO32WriteFldAlign(DRAMC_REG_SHUCTRL1, 0x40, SHUCTRL1_FC_PRDCNT);
	}
	vIO32WriteFldAlign(DRAMC_REG_SHUCTRL, 0x1, SHUCTRL_LPSM_BYPASS_B);
	vIO32WriteFldMulti(DRAMC_REG_REFCTRL1, P_Fld(0x0, REFCTRL1_SREF_PRD_OPT) | P_Fld(0x0, REFCTRL1_PSEL_OPT1) | P_Fld(0x0, REFCTRL1_PSEL_OPT2) | P_Fld(0x0, REFCTRL1_PSEL_OPT3));
	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL4, 0xfe, SHU1_PLL4_RG_RPHYPLL_RESERVED);
	//vIO32WriteFldAlign(DDRPHY_SHU1_PLL6, 0xfe, SHU1_PLL6_RG_RCLRPLL_RESERVED);
	vIO32WriteFldMulti(DRAMC_REG_REFRATRE_FILTER, P_Fld(0x1, REFRATRE_FILTER_PB2AB_OPT) | P_Fld(0x0, REFRATRE_FILTER_PB2AB_OPT1));
#endif

	  //  vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x1, SHU1_B0_DQ7_PSRAM_R_DMDQMDBI_SHU_B0_PSRAM);
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x1, SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1);
	vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0x0, MISC_CTRL0_PSRAM_R_DMDQSIEN_SYNCOPT_PSRAM);
 //   vIO32WriteFldAlign(DRAMC_REG_SHU_STBCAL, 0x1, SHU_STBCAL_DQSG_MODE);
 //   vIO32WriteFldAlign(DRAMC_REG_STBCAL, 0x1, STBCAL_SREF_DQSGUPD);
	//M17_Remap:vIO32WriteFldAlign(DDRPHY_MISC_CTRL1, 0x0, MISC_CTRL1_R_DMDQMDBI);
	/* RX Tracking DQM SM enable (actual values are set in DramcRxInputDelayTrackingHW()) */

		//				 66870 ===ddrphy_shu1_lp4_3200_CHA begin===
		#if 0
		if (vGet_Dram_CBT_Mode(p) == CBT_BYTE_MODE1)
		{
				vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7, 0x1, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0);
			//	vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x1, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1);
		}
		else
		#endif
		{
					   // vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7, P_Fld(0x1, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B0)
					  //	  | P_Fld(0x1, SHU1_B0_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B0));
				vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
							| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM));
			//	vIO32WriteFldMulti(DDRPHY_SHU1_B1_DQ7, P_Fld(0x1, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B1)
			//				| P_Fld(0x1, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_FLAG_OPT_B1));
//francis remove : it will make CLRPLL frequency wrong!
//francis remove				vIO32WriteFldMulti(DDRPHY_SHU1_PLL7, P_Fld(0x3d00, SHU1_PLL7_RG_RCLRPLL_SDM_PCW)
//francis remove							| P_Fld(0x1, SHU1_PLL7_RG_RCLRPLL_SDM_PCW_CHG));
		}
		//				 67761 ===ddrphy_shu1_lp4_3200_CHA end===

		//NOT included in parsing tool
	//	  vIO32WriteFldAlign(DRAMC_REG_SHU_DQS2DQ_TX, 0x0, SHU_DQS2DQ_TX_OE2DQ_OFFSET);

	///TODO: DDR3733
	if (p->freqGroup == 800)
	{
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x0, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);

		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x0, SHU1_B1_DQ7_R_DMRODTEN_B1);

		////DDRPHY0-SHU3
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x4, SHU1_B0_DQ5_RG_RX_ARDQS0_DVS_DLY_B0);
		//vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ6, P_Fld(0x1, SHU1_B0_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B0)
		//			  | P_Fld(0x0, SHU1_B0_DQ6_RG_ARPI_MIDPI_EN_B0));
		#if 0
		if (vGet_Dram_CBT_Mode(p) == CBT_BYTE_MODE1)
		{
			vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x1, SHU1_B0_DQ7_PSRAM_MIDPI_DIV4_ENABLE_PSRAM)
						| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_MIDPI_ENABLE_PSRAM));

		}
		else
		#endif
		{
		   vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
					   | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMDQMDBI_SHU_B0_PSRAM)
					   | P_Fld(0x1, SHU1_B0_DQ7_PSRAM_MIDPI_DIV4_ENABLE_PSRAM)
					   | P_Fld(0x0, SHU1_B0_DQ7_PSRAM_MIDPI_ENABLE_PSRAM));

		}
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ5, 0x4, SHU1_B1_DQ5_RG_RX_ARDQS0_DVS_DLY_B1);
		//vIO32WriteFldMulti(DDRPHY_SHU1_B1_DQ6, P_Fld(0x1, SHU1_B1_DQ6_RG_ARPI_MIDPI_CKDIV4_EN_B1)
		//			  | P_Fld(0x0, SHU1_B1_DQ6_RG_ARPI_MIDPI_EN_B1));
		{
	//		vIO32WriteFldMulti(DDRPHY_SHU1_B1_DQ7, P_Fld(0x0, SHU1_B1_DQ7_R_DMRXDVS_PBYTE_DQM_EN_B1)
	//					| P_Fld(0x0, SHU1_B1_DQ7_R_DMDQMDBI_SHU_B1));
		}

#if 0 //lzs temp mark
		vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ7, P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0)
					| P_Fld(0x1a, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0));
		vIO32WriteFldMulti(DDRPHY_SHU1_R0_B1_DQ7, P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQM_B1)
					| P_Fld(0x1a, SHU1_R0_B1_DQ7_RK0_ARPI_DQ_B1));
		vIO32WriteFldMulti(DDRPHY_SHU1_R1_B0_DQ7, P_Fld(0x26, SHU1_R1_B0_DQ7_RK1_ARPI_DQM_B0)
					| P_Fld(0x26, SHU1_R1_B0_DQ7_RK1_ARPI_DQ_B0));
		vIO32WriteFldMulti(DDRPHY_SHU1_R1_B1_DQ7, P_Fld(0x26, SHU1_R1_B1_DQ7_RK1_ARPI_DQM_B1)
					| P_Fld(0x26, SHU1_R1_B1_DQ7_RK1_ARPI_DQ_B1));
#endif
#if 0//LEGACY_RX_DLY
		LegacyRxDly_LP4_DDR1600(p);
#endif
#if 0//LEGACY_DELAY_CELL
		LegacyDlyCellInitLP4_DDR1600(p);
#endif
	}


		{
			vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x0, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);
	 // 	  vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7, 0x0, SHU1_B0_DQ7_R_DMRODTEN_B0);
			//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x0, SHU1_B1_DQ7_R_DMRODTEN_B1);
			vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD0, P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCMD_PRE_EN) // OE Suspend EN
					| P_Fld(0x1, SHU1_CA_CMD0_RG_TX_ARCLK_PRE_EN)); //ODT Suspend EN
		}

		//close RX DQ/DQS tracking to save power
		vIO32WriteFldMulti(DDRPHY_R0_B0_RXDVS2, P_Fld(0x0, R0_B0_RXDVS2_R_RK0_DVS_MODE_B0)
					| P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_RIS_TRACK_GATE_ENA_B0)
					| P_Fld(0x0, R0_B0_RXDVS2_R_RK0_RX_DLY_FAL_TRACK_GATE_ENA_B0));
	//	vIO32WriteFldMulti(DDRPHY_R1_B0_RXDVS2, P_Fld(0x0, R1_B0_RXDVS2_R_RK1_DVS_MODE_B0)
	//				| P_Fld(0x0, R1_B0_RXDVS2_R_RK1_RX_DLY_RIS_TRACK_GATE_ENA_B0)
	//				| P_Fld(0x0, R1_B0_RXDVS2_R_RK1_RX_DLY_FAL_TRACK_GATE_ENA_B0));

		//wei-jen: RX rank_sel for CA is not used(Bianco), set it's dly to 0 to save power

		vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD7_PSRAM, 0, SHU1_CA_CMD7_PSRAM_R_DMRANKRXDVS_CA_PSRAM);
	   // vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD7, 0, SHU1_CA_CMD7_R_DMRANKRXDVS_CA);

		//DDRPhyTxRxInitialSettings_LP4
		vIO32WriteFldAlign(DDRPHY_CA_CMD3, 0x0, CA_CMD3_RG_RX_ARCMD_STBENCMP_EN);

		vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_RG_RX_ARCLK_DQSIENMODE);

		vIO32WriteFldMulti(DDRPHY_B0_DQ3, P_Fld(0x1, B0_DQ3_RG_RX_ARDQ_IN_BUFF_EN_B0)
							| P_Fld(0x1, B0_DQ3_RG_RX_ARDQM0_IN_BUFF_EN_B0)
							| P_Fld(0x1, B0_DQ3_RG_RX_ARDQS0_IN_BUFF_EN_B0));
		vIO32WriteFldMulti(DDRPHY_CA_CMD3, P_Fld(0x0, CA_CMD3_RG_RX_ARCMD_IN_BUFF_EN)
								| P_Fld(0x0, CA_CMD3_RG_RX_ARCLK_IN_BUFF_EN));

		vIO32WriteFldAlign(DDRPHY_B0_DQ3, 0x0, B0_DQ3_RG_RX_ARDQ_SMT_EN_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ3, 0x0, B1_DQ3_RG_RX_ARDQ_SMT_EN_B1);

		vIO32WriteFldAlign(DDRPHY_B0_DQ5, 0x0, B0_DQ5_RG_RX_ARDQS0_DVS_EN_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ5, 0x1, B1_DQ5_RG_RX_ARDQS0_DVS_EN_B1);
		vIO32WriteFldAlign(DDRPHY_CA_CMD5, 0x0, CA_CMD5_RG_RX_ARCLK_DVS_EN);

		//LP4 no need, follow LP3 first.
		//vIO32WriteFldAlign(DDRPHY_MISC_VREF_CTRL, P_Fld(0x1, MISC_VREF_CTRL_RG_RVREF_DDR3_SEL)
		//										  | P_Fld(0x0, MISC_VREF_CTRL_RG_RVREF_DDR4_SEL));


		vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x1, CA_CMD6_RG_TX_ARCMD_DDR3_SEL)
										| P_Fld(0x0, CA_CMD6_RG_TX_ARCMD_DDR4_SEL)
										| P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR3_SEL)
										| P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_DDR4_SEL));
		vIO32WriteFldMulti(DDRPHY_MISC_IMP_CTRL0, P_Fld(0x1, MISC_IMP_CTRL0_RG_RIMP_DDR3_SEL)
										| P_Fld(0x0, MISC_IMP_CTRL0_RG_RIMP_DDR4_SEL));


		vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x0, B0_DQ6_RG_RX_ARDQ_O1_SEL_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_O1_SEL_B1);
		vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_O1_SEL);

		vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_BIAS_PS_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_BIAS_PS_B1);
		vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x1, CA_CMD6_RG_RX_ARCMD_BIAS_PS);

		vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x1, CA_CMD6_RG_RX_ARCMD_RES_BIAS_EN);

		vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x0, B0_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ6, 0x0, B1_DQ6_RG_TX_ARDQ_ODTEN_EXT_DIS_B1);
		vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_TX_ARCMD_ODTEN_EXT_DIS);

		vIO32WriteFldAlign(DDRPHY_B0_DQ6, 0x1, B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ6, 0x1, B1_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B1);
		vIO32WriteFldAlign(DDRPHY_CA_CMD6, 0x0, CA_CMD6_RG_RX_ARCMD_RPRE_TOG_EN);



		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x1, SHU1_B0_DQ5_RG_RX_ARDQ_VREF_SEL_B0);
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ5, 0xE, SHU1_B1_DQ5_RG_RX_ARDQ_VREF_SEL_B1);
		vIO32WriteFldAlign(DDRPHY_B0_DQ5, 0x10, B0_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ5, 0xE, B1_DQ5_RG_RX_ARDQ_EYE_VREF_SEL_B1);

		vIO32WriteFldMulti(DDRPHY_B0_DQ8, P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_EN_LP4P_B0)
											| P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_EN_CAP_LP4P_B0)
											| P_Fld(0x0, B0_DQ8_RG_TX_ARDQ_CAP_DET_B0));
											//| P_Fld(0x1, B0_DQ8_RG_RX_ARDQS_DQSSTB_CG_EN_B0) // Field only exists for 10nm APHY
											//	corresponds to B0_DQ6_RG_RX_ARDQ_RPRE_TOG_EN_B0 for 16nm APHY
		vIO32WriteFldMulti(DDRPHY_CA_CMD9, P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_EN_LP4P)
											| P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_EN_CAP_LP4P)
											| P_Fld(0x0, CA_CMD9_RG_TX_ARCMD_CAP_DET)
											| P_Fld(0x1, CA_CMD9_RG_ARDLL_RESETB_CA));



		/* BIAS_VREF_SEL is used as switch for old, new burst modes */
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ6), 2, B0_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B0);
		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ6), 2, B1_DQ6_RG_RX_ARDQ_BIAS_VREF_SEL_B1);

		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), 1, B0_DQ9_PSRAM_RG_RX_ARDQS0_DQSIENMODE_B0_PSRAM);
		vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), 1, B0_DQ9_RG_RX_ARDQS0_DQSIENMODE_B0);

		//vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), 1, B1_DQ9_RG_RX_ARDQS0_DQSIENMODE_B1);


		/* Perform reset (makes sure PHY's behavior works as the above setting) */
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(0, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(0, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(0, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));

		//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(0, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) |P_Fld(0, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
		mcDELAY_US(1);//delay 10ns
		//vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B1_DQ9), P_Fld(1, B1_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B1) |P_Fld(1, B1_DQ9_RG_RX_ARDQ_STBEN_RESETB_B1));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9), P_Fld(1, B0_DQ9_RG_RX_ARDQS0_STBEN_RESETB_B0) |P_Fld(1, B0_DQ9_RG_RX_ARDQ_STBEN_RESETB_B0));
		vIO32WriteFldMulti(DRAMC_REG_ADDR(DDRPHY_B0_DQ9_PSRAM), P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQS0_STBEN_RESETB_B0_PSRAM) |P_Fld(1, B0_DQ9_PSRAM_RG_RX_ARDQ_STBEN_RESETB_B0_PSRAM));
		vIO32WriteFldAlign(DDRPHY_CA_CMD8, 0x1, CA_CMD8_RG_TX_RRESETB_DDR3_SEL);
		vIO32WriteFldAlign(DDRPHY_CA_CMD8, 0x0, CA_CMD8_RG_TX_RRESETB_DDR4_SEL); //TODO: Remove if register default value is 0
		//End of DDRPhyTxRxInitialSettings_LP4

		//Update setting for Bianco
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ5, 0x0, SHU1_B0_DQ5_RG_ARPI_FB_B0);
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ5, 0x0, SHU1_B1_DQ5_RG_ARPI_FB_B1);
		vIO32WriteFldAlign(DDRPHY_SHU1_CA_CMD5, 0x0, SHU1_CA_CMD5_RG_ARPI_FB_CA);


		//Reserved bits usage, check with PHY owners
		vIO32WriteFldAlign_All(DDRPHY_SHU1_B0_DQ6, 0x0, SHU1_B0_DQ6_RG_ARPI_OFFSET_DQSIEN_B0);
		//vIO32WriteFldAlign_All(DDRPHY_SHU1_B1_DQ6, 0x0, SHU1_B1_DQ6_RG_ARPI_OFFSET_DQSIEN_B1);
		vIO32WriteFldAlign_All(DDRPHY_SHU1_CA_CMD6, 0x0, SHU1_CA_CMD6_RG_ARPI_OFFSET_CLKIEN);

		//IMP Tracking Init Settings
		//Write (DRAMC _BASE+ 0x219) [31:0] = 32'h80080020//DDR3200 default
		//SHU_IMPCAL1_IMPCAL_CHKCYCLE should > 12.5/MCK, 1:4 mode will disable imp tracking -> don't care


		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ8_PSRAM, P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRANK_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMSTBEN_SYNC_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_B0_PSRAM)
											| P_Fld(0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_B0_PSRAM));
										  //  | P_Fld(0x0, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_UPD_FORCE_EN_B0_PSRAM)
										  //  | P_Fld(0x7fff, SHU1_B0_DQ8_PSRAM_R_DMRXDVS_UPD_FORCE_CYC_B0_PSRAM));


		vIO32WriteFldMulti(DDRPHY_SHU1_CA_CMD8_PSRAM, P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRANK_CHG_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRANK_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_RDSEL_TOG_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_RDSEL_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_FLAG_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMDQSIEN_FLAG_SYNC_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMSTBEN_SYNC_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDLY_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_RDSEL_TOG_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x1, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_RDSEL_PIPE_CG_IG_CA_PSRAM)
											| P_Fld(0x0, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_UPD_FORCE_EN_CA_PSRAM)
											| P_Fld(0x7fff, SHU1_CA_CMD8_PSRAM_R_DMRXDVS_UPD_FORCE_CYC_CA_PSRAM));
		vIO32WriteFldAlign(DDRPHY_MISC_CTRL3_PSRAM, 0x1, MISC_CTRL3_PSRAM_R_DDRPHY_COMB_CG_IG_PSRAM);
        vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI2_PSRAM, 0x1, CA_DLL_ARPI2_PSRAM_RG_ARPI_CG_CLKIEN_PSRAM);
		/* Bianco HW design issue: run-time PBYTE (B0, B1) flags will lose it's function and become per-bit -> set to 0 */
		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_DQM_EN_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_PBYTE_FLAG_OPT_B0_PSRAM)
											| P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXDVS_DQM_FLAGSEL_B0_PSRAM));

#if 1//#ifndef BIANCO_TO_BE_PORTING
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DLL0, 0x1, SHU1_B0_DLL0_RG_ARPISM_MCK_SEL_B0_SHU);
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DLL0, 0x1, SHU1_B1_DLL0_RG_ARPISM_MCK_SEL_B1_SHU);
		//vIO32WriteFldAlign(DDRPHY_SHU1_CA_DLL0, 0x1, SHU1_CA_DLL0_RG_ARPISM_MCK_SEL_CA_SHU); move to DramcSetting_Olympus_LP4_ByteMode()

		vIO32WriteFldAlign(DDRPHY_CA_DLL_ARPI1, 0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA);
#endif
		//end _K_

		vIO32WriteFldMulti(DDRPHY_B0_DQ6, P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_OP_BIAS_SW_EN_B0)
									| P_Fld(0x1, B0_DQ6_RG_RX_ARDQ_BIAS_EN_B0));
		vIO32WriteFldMulti(DDRPHY_CA_CMD6, P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_OP_BIAS_SW_EN)
										| P_Fld(0x0, CA_CMD6_RG_RX_ARCMD_BIAS_EN));
		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DQ7_PSRAM, P_Fld(0x0, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQS_LAT_B0_PSRAM)
										| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQS_EN_B0_PSRAM)
										| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQ_LAT_B0_PSRAM)
										| P_Fld(0x1, SHU1_B0_DQ7_PSRAM_R_DMRXRANK_DQ_EN_B0_PSRAM));

		vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x4, B0_DQ9_PSRAM_R_IN_GATE_EN_LOW_OPT_B0_PSRAM);
		vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x4, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);

		//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x4, B1_DQ9_R_IN_GATE_EN_LOW_OPT_B1);
#if 0
		//Modify for corner IC failed at HQA test XTLV
		vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x7, B0_DQ9_PSRAM_R_IN_GATE_EN_LOW_OPT_B0_PSRAM);
		vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x7, B0_DQ9_R_IN_GATE_EN_LOW_OPT_B0);

		//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x7, B1_DQ9_R_IN_GATE_EN_LOW_OPT_B1);
#endif
		vIO32WriteFldAlign(DDRPHY_CA_CMD10, 0x0, CA_CMD10_R_IN_GATE_EN_LOW_OPT_CA);
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ8_PSRAM, 0x1, SHU1_B0_DQ8_PSRAM_R_DMRXDLY_CG_IG_B0_PSRAM);
		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ8, 0x1, SHU1_B1_DQ8_R_DMRXDLY_CG_IG_B1);

#if 0//ENABLE_TX_WDQS
		mcSHOW_DBG_MSG("Enable WDQS\n");
		//Check reserved bits with PHY integrator
		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(1, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN)  | P_Fld(1, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
											| P_Fld(!p->odt_onoff, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE));
		vIO32WriteFldAlign(DDRPHY_SHU1_B0_DQ7_PSRAM, 0x1, SHU1_B0_DQ7_PSRAM_R_DMRODTEN_B0_PSRAM);

		//vIO32WriteFldAlign(DDRPHY_SHU1_B1_DQ7, 0x1, SHU1_B1_DQ7_R_DMRODTEN_B1);
#if ENABLE_RODT_TRACKING_SAVE_MCK
		SetTxWDQSStatusOnOff(1);
#endif

#else //WDQS and reak pull are disable
		//Check reserved bits with PHY integrator
		vIO32WriteFldMulti(DDRPHY_SHU1_B0_DLL1, P_Fld(0, RG_ARDQ_REV_BIT_09_TX_READ_BASE_EN) | P_Fld(0, RG_ARDQ_REV_BIT_02_TX_READ_BASE_EN_DQSB)
											| P_Fld(0, RG_ARDQ_REV_BIT_08_TX_ODT_DISABLE));
#endif

		vIO32WriteFldAlign(DDRPHY_MISC_CTRL3_PSRAM, 0x1, MISC_CTRL3_PSRAM_R_DDRPHY_RX_PIPE_CG_IG_PSRAM);
		vIO32WriteFldAlign(DDRPHY_MISC_CG_CTRL0, 0x1, MISC_CG_CTRL0_RG_IDLE_SRC_SEL);
		//DE review Bianco
		/* ARPISM_MCK_SEL_B0, B1 set to 1 (Joe): "Due to TX_PICG modify register is set to 1,
		 * ARPISM_MCK_SEL_Bx should be 1 to fulfill APHY TX OE spec for low freq (Ex: DDR1600)"
		 */
		vIO32WriteFldMulti(DDRPHY_B0_DLL_ARPI1, P_Fld(0x0, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0_OPT)
											| P_Fld(0x1, B0_DLL_ARPI1_RG_ARPISM_MCK_SEL_B0));
        vIO32WriteFldMulti(DDRPHY_CA_DLL_ARPI1, P_Fld(0x0, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA_OPT)
											| P_Fld(0x1, CA_DLL_ARPI1_RG_ARPISM_MCK_SEL_CA));

		vIO32WriteFldAlign(DDRPHY_MISC_CTRL0_PSRAM, 0, MISC_CTRL0_PSRAM_R_DMSHU_PHYDCM_FORCEOFF_PSRAM);

		vIO32WriteFldAlign(DDRPHY_MISC_RXDVS2, 1, MISC_RXDVS2_R_DMRXDVS_SHUFFLE_CTRL_CG_IG);

		vIO32WriteFldMulti(DDRPHY_CA_TX_MCK, P_Fld(0x1, CA_TX_MCK_R_DMRESET_FRPHY_OPT) | P_Fld(0xa, CA_TX_MCK_R_DMRESETB_DRVP_FRPHY) | P_Fld(0xa, CA_TX_MCK_R_DMRESETB_DRVN_FRPHY));


		//[Cervino DVT]RX FIFO debug feature, MP setting should enable debug function
		vIO32WriteFldAlign(DDRPHY_B0_DQ9_PSRAM, 0x1, B0_DQ9_PSRAM_R_DMRXFIFO_STBENCMP_EN_B0_PSRAM);
		vIO32WriteFldAlign(DDRPHY_B0_DQ9, 0x1, B0_DQ9_R_DMRXFIFO_STBENCMP_EN_B0);
		//vIO32WriteFldAlign(DDRPHY_B1_DQ9, 0x1, B1_DQ9_R_DMRXFIFO_STBENCMP_EN_B1);
		vIO32WriteFldMulti(DDRPHY_MISC_CTRL1, P_Fld(0x1, MISC_CTRL1_R_DMDQSIENCG_EN)
				| P_Fld(0x1, MISC_CTRL1_R_DM_TX_ARCMD_OE)
				| P_Fld(0x1, MISC_CTRL1_R_DM_TX_ARCLK_OE));

#ifndef LOOPBACK_TEST
	DDRPhyFreqMeter();
#endif


///TODO: DVFS_Enable


#ifndef LOOPBACK_TEST
	DDRPhyFMeter_Init();
#endif

#if 0//ENABLE_DUTY_SCAN_V2   lzs mark, this will be do after psram init
#ifdef DDR_INIT_TIME_PROFILING
	U32 u4low_tick0, u4high_tick0, u4low_tick1, u4high_tick1;
#if __ETT__
	u4low_tick0 = GPT_GetTickCount(&u4high_tick0);
#else
	u4low_tick0 = get_timer(0);
#endif
#endif

#ifndef DUMP_INIT_RG_LOG_TO_DE
	if (Get_MDL_Used_Flag()==NORMAL_USED)
	{
		DramcNewDutyCalibration(p);
	}
#endif
#ifdef DDR_INIT_TIME_PROFILING
#if __ETT__
	u4low_tick1 = GPT_GetTickCount(&u4high_tick1);
	gu4DutyCalibrationTime = ((u4low_tick1-u4low_tick0)*76)/1000000;
#else
	u4low_tick1 = get_timer(u4low_tick0);
	gu4DutyCalibrationTime = u4low_tick1;
#endif
#endif
#endif

#if 1 // 20190625 move release dmsus here
    PsramReleaseDMSUS(p);
#endif

   // DVFSSettings(p);

	vIO32WriteFldAlign_All(DDRPHY_DVFS_EMI_CLK, 1, DVFS_EMI_CLK_RG_52M_104M_SEL); //Set DVFS_SM's clk

	vIO32WriteFldAlign_All(DDRPHY_DVFS_EMI_CLK, 1, DVFS_EMI_CLK_RG_DLL_SHUFFLE);

	/////////lzs add here for PSRAMC init setting/////////////////////

#if 1// lzs move CK = CK +0.5UI, CA = CA +0.5UI
	vIO32WriteFldAlign(DDRPHY_SHU1_R0_CA_CMD9, 0x10, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CLK);
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_CA_CMD9), 0x10, SHU1_R0_CA_CMD9_RG_RK0_ARPI_CMD);
#endif

 //TX init setting follow simulation result
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), 0x10, SHU1_R0_B0_DQ7_RK0_ARPI_DQ_B0); //CA/DQ move 0.5UI to edge align CK
	vIO32WriteFldAlign(DRAMC_REG_ADDR(DDRPHY_SHU1_R0_B0_DQ7), 0x10, SHU1_R0_B0_DQ7_RK0_ARPI_DQM_B0); //

	vIO32WriteFldAlign(PSRAMC_REG_SHU_SELPH_CA1, 0x2, SHU_SELPH_CA1_TXDLY_CS);

	PSramTXSetDelayReg_CA(p, TRUE, 17,&u2Tmp); // CA setting (2, 1), CA OEN (1, 6)

	if(p->frequency == 800)
	{
		vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS0, P_Fld(0x1, SHU_SELPH_DQS0_TXDLY_OEN_DQS0)
					| P_Fld(0x3, SHU_SELPH_DQS0_TXDLY_DQS0));

		PSramTXSetDelayReg_DQ(p, TRUE, 3, 2, 3, 3, 16); //DQ (3, 3, 16), DQ OEN (2, 3)
		PSramTXSetDelayReg_DQM(p, TRUE, 3, 2, 3, 3, 16); //DQM (3, 3, 16), DQM OEN (2, 3)

	}
	else//2133
	{
		vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS0, P_Fld(0x2, SHU_SELPH_DQS0_TXDLY_OEN_DQS0)
					| P_Fld(0x4, SHU_SELPH_DQS0_TXDLY_DQS0));

		PSramTXSetDelayReg_DQ(p, TRUE, 4, 3, 3, 3, 16); //DQ (4, 3, 16), DQ OEN (3, 3)
		PSramTXSetDelayReg_DQM(p, TRUE, 4, 3, 3, 3, 16); //DQM (4, 3, 16), DQM OEN (3, 3)

	}

#ifdef PSRAM_MODEL_32MB
	vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS1, P_Fld(0x3, SHU_SELPH_DQS1_DLY_OEN_DQS0)
				| P_Fld(0x5, SHU_SELPH_DQS1_DLY_DQS0));
#else
	vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_DQS1, P_Fld(0x3, SHU_SELPH_DQS1_DLY_OEN_DQS0)
				| P_Fld(0x3, SHU_SELPH_DQS1_DLY_DQS0));
#endif

#ifdef PSRAM_MODEL_32MB //32MB
	vIO32WriteFldAlign(PSRAMC_REG_DDRCOMMON0, 0x1, DDRCOMMON0_PSRAM_256M_EN);
#endif


#if 0
	vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA3, P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA7)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA6)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA5)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA4)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA3)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA2)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA1)
				| P_Fld(0x2, SHU_SELPH_CA3_TXDLY_RA0));

	vIO32WriteFldMulti(PSRAMC_REG_SHU_SELPH_CA7, P_Fld(1, SHU_SELPH_CA7_DLY_RA7)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA6)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA5)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA4)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA3)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA2)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA1)
				 | P_Fld(1, SHU_SELPH_CA7_DLY_RA0));

	vIO32WriteFldMulti(PSRAMC_REG_SHURK_SELPH_DQ0, P_Fld(0x1, SHURK_SELPH_DQ0_TXDLY_OEN_CA0)
				| P_Fld(0x2, SHURK_SELPH_DQ0_TXDLY_OEN_DQ0)
				| P_Fld(0x3, SHURK_SELPH_DQ0_TXDLY_DQ0));
	vIO32WriteFldMulti(PSRAMC_REG_SHURK_SELPH_DQ1, P_Fld(0x2, SHURK_SELPH_DQ1_TXDLY_OEN_DQM0)
				| P_Fld(0x3, SHURK_SELPH_DQ1_TXDLY_DQM0));
	vIO32WriteFldMulti(PSRAMC_REG_SHURK_SELPH_DQ2, P_Fld(0x6, SHURK_SELPH_DQ2_DLY_OEN_CA0)
				| P_Fld(0x3, SHURK_SELPH_DQ2_DLY_OEN_DQ0)
				| P_Fld(0x3, SHURK_SELPH_DQ2_DLY_DQ0));
	vIO32WriteFldMulti(PSRAMC_REG_SHURK_SELPH_DQ3, P_Fld(0x3, SHURK_SELPH_DQ3_DLY_OEN_DQM0)
				| P_Fld(0x3, SHURK_SELPH_DQ3_DLY_DQM0));
#endif


	////gating and datlat setting follow simulation result
	vIO32WriteFldAlign(DDRPHY_PSRAM_DQSIEN_CTRL0,0x07,PSRAM_DQSIEN_CTRL0_DQSINCTL_RK0);
	if(p->frequency == 800) //1600 setting
	{
		vIO32WriteFldMulti(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0, P_Fld(0x8, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0)
					 | P_Fld(0xc, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0));

		vIO32WriteFldMulti(DDRPHY_PSRAM_RDAT, P_Fld(0xe, PSRAM_RDAT_DATLAT)
				| P_Fld(0xd, PSRAM_RDAT_DATLAT_DSEL)
				| P_Fld(0xd, PSRAM_RDAT_DATLAT_DSEL_PHY));
	}
	else //follow 2133 simulation result
	{
		vIO32WriteFldMulti(DDRPHY_PSRAM_DQSIEN_DLY_CTRL0, P_Fld(0x1a, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_UI_RK0_B0)
					 | P_Fld(0x10, PSRAM_DQSIEN_DLY_CTRL0_DQSIEN_PI_RK0_B0));

		vIO32WriteFldMulti(DDRPHY_PSRAM_RDAT, P_Fld(0x10, PSRAM_RDAT_DATLAT)
				| P_Fld(0xf, PSRAM_RDAT_DATLAT_DSEL)
				| P_Fld(0xf, PSRAM_RDAT_DATLAT_DSEL_PHY));
	}


	/////RX setting follow simulation //////////////////////////////////////
	////DQ delay////////////////////
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ2, P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ1_R_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ2_RK0_RX_ARDQ0_R_DLY_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ3, P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ3_R_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ3_RK0_RX_ARDQ2_R_DLY_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ4, P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ5_R_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ4_RK0_RX_ARDQ4_R_DLY_B0));
	vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ5, P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ7_R_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_F_DLY_B0)
				| P_Fld(0x0, SHU1_R0_B0_DQ5_RK0_RX_ARDQ6_R_DLY_B0));
	/////DQS and DQM delay///////
	if(p->frequency == 800) //1600 setting
	{
		vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ6, P_Fld(0x14, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0)
					| P_Fld(0x14, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0)
					| P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0)
					| P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	}
	else //2133 setting
	{
		vIO32WriteFldMulti(DDRPHY_SHU1_R0_B0_DQ6, P_Fld(0xb, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_F_DLY_B0)
						| P_Fld(0xb, SHU1_R0_B0_DQ6_RK0_RX_ARDQS0_R_DLY_B0)
						| P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_F_DLY_B0)
						| P_Fld(0x0, SHU1_R0_B0_DQ6_RK0_RX_ARDQM0_R_DLY_B0));
	}


#if 0
	vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)   //when 2133 should change this value
			 | P_Fld(0x04, PSAMC_PD_CTRL_APHYPI_CKCGH_CNT)	//when 2133 should change this value
			 | P_Fld(0x1, PSRAMC_PD_CTRL_DCMEN)
			 | P_Fld(0x0, PSAMC_PD_CTRL_DCMEN2));
#else  // add for review before K and after K
	vIO32WriteFldMulti(PSRAMC_REG_DRAMC_PD_CTRL, P_Fld(0x1, PSAMC_PD_CTRL_PHYCLKDYNGEN)
				| P_Fld(0x0, PSAMC_PD_CTRL_APHYCKCG_FIXOFF)
				| P_Fld(0x0, PSAMC_PD_CTRL_MIOCKCTRLOFF));
#endif


	vIO32WriteFldAlign(PSRAMC_REG_SHU_DCM_CTRL0,0x01,SHU_DCM_CTRL0_DDRPHY_CLK_EN_OPT);

	vIO32WriteFldAlign(PSRAMC_REG_DLLFRZ_CTRL, 0x1, DLLFRZ_CTRL_DLLFRZ);
	vIO32WriteFldAlign(PSRAMC_REG_CE_CTRL,0x3f,CE_CTRL_WRCS_NOT_MASK);

	vIO32WriteFldAlign(PSRAMC_REG_TX_CG_SET0, 0x1, TX_CG_SET0_SELPH_CMD_CG_DIS);
	vIO32WriteFldAlign(PSRAMC_REG_RX_CG_SET0, 0x01,RX_CG_SET0_RDYCKAR);
	vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x1, CLKAR_REQQUECLKRUN);
	vIO32WriteFldAlign(PSRAMC_REG_CLKAR, 0x0, CLKAR_DCMREF_OPT);

	vIO32WriteFldAlign(PSRAMC_REG_REFCTRL0, 0x1, REFCTRL0_REFDIS);
	vIO32WriteFldAlign(PSRAMC_REG_REFCTRL1, 0x1, REFCTRL1_REFPEND_OPT1);

	vIO32WriteFldAlign(PSRAMC_REG_SHU_MATYPE, 0x3, SHU_MATYPE_MATYPE);
	vIO32WriteFldMulti(PSRAMC_REG_SHU_DCM_CTRL0, P_Fld(0x1, SHU_DCM_CTRL0_FASTWAKE2)
				| P_Fld(0x1, SHU_DCM_CTRL0_DDRPHY_CLK_EN_OPT));

	vIO32WriteFldMulti(PSRAMC_REG_SHU_RX_CG_SET0, P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND1)
				| P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND2)
				| P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND2)
				| P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND3)
				| P_Fld(0x1, SHU_RX_CG_SET0_READ_START_EXTEND1)
				| P_Fld(0x1, SHU_RX_CG_SET0_DLE_LAST_EXTEND3));

    vIO32WriteFldMulti(PSRAMC_REG_DUMMY_RD, P_Fld(0, DUMMY_RD_DMYRD_HPRI_DIS)
				| P_Fld(1, DUMMY_RD_DUMMY_RD_SW)
				| P_Fld(1, DUMMY_RD_DUMMY_RD_PA_OPT)
				| P_Fld(0, DUMMY_RD_DMY_RD_RX_TRACK)
				| P_Fld(1, DUMMY_RD_RANK_NUM));
    vIO32WriteFldMulti(PSRAMC_REG_DUMMY_RD_INTV, P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT6)
				| P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT5)
				| P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_CNT3)
				| P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT4)
				| P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT3)
				| P_Fld(1, DUMMY_RD_INTV_DUMMY_RD_1_CNT1));

#if 1 //AC Timing setting 1600
    if(p->frequency == 800)
		PsramUpdateACTimingReg(p, &ACTimingPsramTbl[0]);
	else //2133
		PsramUpdateACTimingReg(p, &ACTimingPsramTbl[1]);
#else
					vIO32WriteFldMulti(PSRAMC_REG_CE_CNT0, P_Fld(0x5, CE_CNT0_TCE_WR)
								| P_Fld(0xa, CE_CNT0_TCE_RD)
								| P_Fld(0x14, CE_CNT0_TCE_HSLEEPX));
					vIO32WriteFldMulti(PSRAMC_REG_CE_CNT_05T, P_Fld(0x0, CE_CNT_05T_TCE_WR_05T)
								| P_Fld(0x0, CE_CNT_05T_TCE_RD_05T)
								| P_Fld(0x1, CE_CNT_05T_TCE_HSLEEPX_05T));

					vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM0, P_Fld(0x0, SHU_ACTIM0_TCEM)
								| P_Fld(0x14, SHU_ACTIM0_TXHS)
								| P_Fld(0xc, SHU_ACTIM0_TRC)
								| P_Fld(0xc, SHU_ACTIM0_TRFC));
					vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM1, P_Fld(0xa, SHU_ACTIM1_TCPHW)
								| P_Fld(0xe, SHU_ACTIM1_TCPHR_L)
								| P_Fld(0xb, SHU_ACTIM1_TCPHR_S));

					vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T, P_Fld(0x0, PSRAMC_SHU_AC_TIME_05T_TRFC_05T)
								| P_Fld(0x0, PSRAMC_SHU_AC_TIME_05T_TRC_05T)
								| P_Fld(0x0, PSRAMC_SHU_AC_TIME_05T_TXHS_05T)
								| P_Fld(0x1, PSRAMC_SHU_AC_TIME_05T_TCEM_05T)
								| P_Fld(0x0, PSRAMC_SHU_AC_TIME_05T_TCPHR_S_05T)
								| P_Fld(0x0, PSRAMC_SHU_AC_TIME_05T_TCPHR_L_05T)
								| P_Fld(0x1, PSRAMC_SHU_AC_TIME_05T_TCPHW_05T)
								| P_Fld(0x1, PSRAMC_SHU_AC_TIME_05T_TMRW_05T));
					vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM2, P_Fld(0x14, SHU_ACTIM2_TMRW));
					vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM3, P_Fld(0xc8, SHU_ACTIM3_TZQCL)
								| P_Fld(0x64, SHU_ACTIM3_TZQCS));
					vIO32WriteFldMulti(PSRAMC_REG_SHU_AC_TIME_05T_B, P_Fld(0x0, SHU_AC_TIME_05T_B_TZQCL_05T)
								| P_Fld(0x0, SHU_AC_TIME_05T_B_TZQCS_05T));

					vIO32WriteFldMulti(PSRAMC_REG_SHU_ACTIM4, P_Fld(0x14, SHU_ACTIM4_TXREFCNT));

					vIO32WriteFldMulti(PSRAMC_REG_ZQC_CTRL1, P_Fld(0x5, ZQC_CTRL1_ZQCSAD)
								| P_Fld(0x0, ZQC_CTRL1_ZQCSOP));

#endif


#if 0
	PsramReleaseDMSUS(p);
#endif
	mcDELAY_US(1);

#if 1//DV_SIMULATION_SW_IMPED
	   PsramcSwImpedanceCal(p,1, 0);  //without term
		   //update ODTP/ODTN of term to unterm
#endif

#if 0
	  PsramcNewDutyCalibration(p);
#endif


	PSRAMCPowerOn(p);
	PSRAMCModeRegInit(p);
   // TestPSramEngineCompare(p);
   // PsramDataWindowTest(p);

	vPSRAM_AutoRefreshSwitch(p, ENABLE);

}

#endif




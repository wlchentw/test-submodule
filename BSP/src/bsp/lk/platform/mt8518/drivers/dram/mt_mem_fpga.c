/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its
 * licensors. Without the prior written permission of MediaTek and/or its
 * licensors, any reproduction, modification, use or disclosure of MediaTek
 * Software, and information contained herein, in whole or in part, shall be
 * strictly prohibited. You may only use, reproduce, modify, or distribute
 * (as applicable) MediaTek Software if you have agreed to and been bound by
 * the applicable license agreement with MediaTek ("License Agreement") and
 * been granted explicit permission to do so within the License Agreement
 * ("Permitted User").  If you are not a Permitted User, please cease any
 * access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
 * DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY
 * WARRANTY WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH
 * MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND
 * RECEIVER AGREES TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM
 * RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE
 * RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED
 * IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY
 * MEDIATEK SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM
 * TO A PARTICULAR STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE
 * REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO
 * MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <reg.h>
#include "platform/mtk_timer.h"
#include "platform/mt_reg_base.h"

void fpga_emi_init(void)
{
	/*
	 * &NR_RNKS=2
	 * EMI
	 * 0x1033_6***

	 * DRAMC
	 * 0x1033_2***

	 * DDRPHY
	 * 0x1033_5***
	 */

	/* EMI setting */
	writel(0x3010, EMI_BASE + 0x0000);
	writel(0xff000400, EMI_BASE + 0x0060);
	writel(0x00100000, EMI_BASE + 0x0068);
	writel(0x4002010, DRAMC_BASE + 0x5000);
	writel(0x01, DRAMC_BASE + 0x5010);

	writel(0x10101003, DRAMC_BASE + 0x21EC);
	gpt4_busy_wait_ms(1);
	writel(0x0283488c, DRAMC_BASE + 0x207C);
	writel(0x00005180, DRAMC_BASE + 0x2054);
	writel(0x2d000001, DRAMC_BASE + 0x21E8);

	writel(0x10101010, DRAMC_BASE + 0x000C);
	writel(0x10101010, DRAMC_BASE + 0x0010);
	writel(0x10101010, DRAMC_BASE + 0x006C);
	writel(0x10101010, DRAMC_BASE + 0x0070);
	writel(0x21000000, DRAMC_BASE + 0x0184);
	writel(0x10101010, DRAMC_BASE + 0x018C);
	writel(0x0000061e, DRAMC_BASE + 0x0014);
	writel(0x00001010, DRAMC_BASE + 0x0018);
	writel(0x01111010, DRAMC_BASE + 0x001C);
	writel(0x01031001, DRAMC_BASE + 0x0020);
	writel(0x03111010, DRAMC_BASE + 0x001C);
	writel(0x00002828, DRAMC_BASE + 0x003C);

	writel(0x00001010, DRAMC_BASE + 0x0058);
	writel(0x0000061e, DRAMC_BASE + 0x0074);
	writel(0x00001010, DRAMC_BASE + 0x0078);
	writel(0x01111010, DRAMC_BASE + 0x007C);
	writel(0x01031001, DRAMC_BASE + 0x0080);
	writel(0x03111010, DRAMC_BASE + 0x007C);
	writel(0x00001010, DRAMC_BASE + 0x009C);
	writel(0x00001010, DRAMC_BASE + 0x00B8);
	writel(0x00000000, DRAMC_BASE + 0x0180);
	writel(0x21000000, DRAMC_BASE + 0x0184);
	writel(0x000001e2, DRAMC_BASE + 0x0188);

	writel(0x00000022, DRAMC_BASE + 0x0400);
	writel(0x00000000, DRAMC_BASE + 0x0404);
	writel(0x00000000, DRAMC_BASE + 0x0408);
	writel(0x00000000, DRAMC_BASE + 0x040C);
	writel(0x00000000, DRAMC_BASE + 0x0410);
	writel(0x00000000, DRAMC_BASE + 0x0414);
	writel(0x00000000, DRAMC_BASE + 0x0418);
	writel(0x00000000, DRAMC_BASE + 0x041C);
	writel(0xaa000040, DRAMC_BASE + 0x0420);
	writel(0x00000000, DRAMC_BASE + 0x0424);
	writel(0x00000000, DRAMC_BASE + 0x0428);
	writel(0x00000000, DRAMC_BASE + 0x042C);
	writel(0xffff0003, DRAMC_BASE + 0x0430);
	writel(0xffff0000, DRAMC_BASE + 0x0434);
	writel(0x00000c0c, DRAMC_BASE + 0x0438);
	writel(0x00660600, DRAMC_BASE + 0x043C);
	writel(0x00660600, DRAMC_BASE + 0x0440);
	writel(0x80690608, DRAMC_BASE + 0x0444);
	writel(0xc0690608, DRAMC_BASE + 0x0448);
	writel(0x00000000, DRAMC_BASE + 0x044C);
	writel(0x00000000, DRAMC_BASE + 0x0450);
	writel(0x00000000, DRAMC_BASE + 0x0454);
	writel(0x00000000, DRAMC_BASE + 0x0458);
	writel(0x00000000, DRAMC_BASE + 0x0460);
	writel(0x00000000, DRAMC_BASE + 0x0468);
	writel(0x00000000, DRAMC_BASE + 0x0088);
	writel(0x00000000, DRAMC_BASE + 0x0028);
	writel(0x00000000, DRAMC_BASE + 0x0454);
	gpt4_busy_wait_ms(1);
	writel(0x00010000, DRAMC_BASE + 0x0454);
	gpt4_busy_wait_ms(1);
	writel(0x80000000, DRAMC_BASE + 0x0404);
	gpt4_busy_wait_ms(1);
	writel(0x00008c8c, DRAMC_BASE + 0x0438);
	gpt4_busy_wait_ms(1);
	writel(0x00660e00, DRAMC_BASE + 0x043C);
	writel(0x00660e00, DRAMC_BASE + 0x0440);
	writel(0x80690e08, DRAMC_BASE + 0x0444);
	writel(0xc0690e08, DRAMC_BASE + 0x0448);
	gpt4_busy_wait_ms(1);
	writel(0xffff0000, DRAMC_BASE + 0x0430);
	writel(0x0000271e, DRAMC_BASE + 0x0014);
	writel(0x0000071e, DRAMC_BASE + 0x0074);

	/* To DRAMC */
	writel(0x33333300, DRAMC_BASE + 0x2438);
	writel(0x51515100, DRAMC_BASE + 0x243C);
	writel(0x00000000, DRAMC_BASE + 0x2400);
	writel(0x00303000, DRAMC_BASE + 0x2404);
	writel(0x22223300, DRAMC_BASE + 0x2430);
	/* gated */
	writel(0x51515100, DRAMC_BASE + 0x2434);
/*	writel(0x62626200, DRAMC_BASE + 0x2434);	*/

	writel(0x00020335, DRAMC_BASE + 0x2418);
	writel(0x00000000, DRAMC_BASE + 0x2408);
	writel(0x00000000, DRAMC_BASE + 0x240C);
	/* gated */
	writel(0x55455545, DRAMC_BASE + 0x2410);
/*	writel(0x56855545, DRAMC_BASE + 0x2410);	*/

	writel(0x11112222, DRAMC_BASE + 0x241C);
	writel(0x11112222, DRAMC_BASE + 0x2420);
	writel(0x33333333, DRAMC_BASE + 0x2440);
	writel(0x33333333, DRAMC_BASE + 0x2444);
	writel(0x77777777, DRAMC_BASE + 0x2448);
	writel(0x44444444, DRAMC_BASE + 0x244C);
	writel(0x11112222, DRAMC_BASE + 0x2424);

	/* DQ dly */
	writel(0x000f0a0a, DRAMC_BASE + 0x2428);
	/* DQS dly */
	writel(0x00050055, DRAMC_BASE + 0x242C);
	writel(0x00ff00f0, DRAMC_BASE + 0x2458);
	writel(0xa0000000, DRAMC_BASE + 0x2454);

	writel(0x00000000, DRAMC_BASE + 0x21E0);
	writel(0x040000c1, DRAMC_BASE + 0x21F8);
	writel(0x30430000, DRAMC_BASE + 0x21C4);
	writel(0x00000000, DRAMC_BASE + 0x223C);

	/* refresh setting */
	writel(0x00403318, DRAMC_BASE + 0x2008);
	writel(0x90182840, DRAMC_BASE + 0x21dc);

	writel(0x2d000001, DRAMC_BASE + 0x21E8);
	writel(0x2200110d, DRAMC_BASE + 0x2048);
	writel(0x00000001, DRAMC_BASE + 0x208C);
	writel(0x00100510, DRAMC_BASE + 0x20D8);
	writel(0x00082023, DRAMC_BASE + 0x20E4);
	writel(0x99169952, DRAMC_BASE + 0x20B8);
	writel(0x99109950, DRAMC_BASE + 0x20BC);
	writel(0x00000000, DRAMC_BASE + 0x2090);
	writel(0x83000000, DRAMC_BASE + 0x20DC);

	writel(0x10000000, DRAMC_BASE + 0x20E0);
	writel(0x00000000, DRAMC_BASE + 0x2118);
	writel(0x10000000, DRAMC_BASE + 0x20F0);
	writel(0x0100000c, DRAMC_BASE + 0x25C0);
	writel(0x01000000, DRAMC_BASE + 0x20F4);
	writel(0x00000000, DRAMC_BASE + 0x2168);
	writel(0x70000000, DRAMC_BASE + 0x2130);
	writel(0x00300510, DRAMC_BASE + 0x20D8);
	writel(0xf00086a3, DRAMC_BASE + 0x2004);
	writel(0xc0000011, DRAMC_BASE + 0x2124);
	writel(0x00b08000, DRAMC_BASE + 0x2138);
	writel(0x00000000, DRAMC_BASE + 0x2094);
	writel(0x00000000, DRAMC_BASE + 0x2098);
	writel(0x11110000, DRAMC_BASE + 0x21C0);
	writel(0x00000008, DRAMC_BASE + 0x2240);

	writel(0x668e4719, DRAMC_BASE + 0x2000);
	writel(0x17030000, DRAMC_BASE + 0x20FC);
	writel(0x040000c1, DRAMC_BASE + 0x21F8);
	writel(0x10000003, DRAMC_BASE + 0x21EC);
	writel(0x0781548c, DRAMC_BASE + 0x207C);

	/* DATA LATENCY - Important setting */
	/* There are 4 stage read data fifo */

	/*writel(0x70070700, DRAMC_BASE + 0x2080);*/
	/*writel(0x70090700, DRAMC_BASE + 0x2080);*/

	writel(0x70070700, DRAMC_BASE + 0x2080);

	writel(0xd0000000, DRAMC_BASE + 0x2028);
	writel(0x2d000001, DRAMC_BASE + 0x21E8);
	writel(0x00000000, DRAMC_BASE + 0x2158);
	writel(0x00697780, DRAMC_BASE + 0x2110);
	writel(0xf07486a3, DRAMC_BASE + 0x2004);
	writel(0x00000027, DRAMC_BASE + 0x20E4);

	/* ERIC */
	/* MRS */
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x0000000d, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	/* ODT */
	writel(0x0000000b, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00540001, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00120002, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00310003, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);
}

void fpga_emi_init_2ch(void)
{
	writel(0x02, INFRACFG_AO_BASE + 0x0284);

	writel(0xA050A150, EMI_BASE + 0x0000);

	writel(0xA050A150, EMI_BASE + 0x0000);
	/* writel(0x004c0000, EMI_BASE + 0x0038); */
	writel(0x00000000, EMI_BASE + 0x0038);
	writel(0x00000000, EMI_BASE + 0x0068);
	writel(0xff000400, EMI_BASE + 0x0060);
	/* writel(0x400a051, DRAMC_BASE + 0x5000); */
	writel(0x0400A050, DRAMC_BASE + 0x5000);
	writel(0x0400A050, DRAMC_BASE + 0x5000);
	/* 2rank_6GB */
	/* writel(0x44ca051, DRAMC_BASE + 0x5000); */
	writel(0x00000001, DRAMC_BASE + 0x5010);

	writel(0x10101003, DRAMC_BASE + 0x21EC);
	gpt4_busy_wait_ms(1);
	writel(0x0283488c, DRAMC_BASE + 0x207C);
	writel(0x00005180, DRAMC_BASE + 0x2054);
	writel(0x2d000001, DRAMC_BASE + 0x21E8);

	writel(0x10101010, DRAMC_BASE + 0x000C);
	writel(0x10101010, DRAMC_BASE + 0x0010);
	writel(0x10101010, DRAMC_BASE + 0x006C);
	writel(0x10101010, DRAMC_BASE + 0x0070);
	writel(0x21000000, DRAMC_BASE + 0x0184);
	writel(0x10101010, DRAMC_BASE + 0x018C);
	writel(0x0000061e, DRAMC_BASE + 0x0014);
	writel(0x00001010, DRAMC_BASE + 0x0018);
	writel(0x01111010, DRAMC_BASE + 0x001C);
	writel(0x01031001, DRAMC_BASE + 0x0020);
	writel(0x03111010, DRAMC_BASE + 0x001C);
	writel(0x00002828, DRAMC_BASE + 0x003C);

	writel(0x00001010, DRAMC_BASE + 0x0058);
	writel(0x0000061e, DRAMC_BASE + 0x0074);
	writel(0x00001010, DRAMC_BASE + 0x0078);
	writel(0x01111010, DRAMC_BASE + 0x007C);
	writel(0x01031001, DRAMC_BASE + 0x0080);
	writel(0x03111010, DRAMC_BASE + 0x007C);
	writel(0x00001010, DRAMC_BASE + 0x009C);
	writel(0x00001010, DRAMC_BASE + 0x00B8);
	writel(0x00000000, DRAMC_BASE + 0x0180);
	writel(0x21000000, DRAMC_BASE + 0x0184);
	writel(0x000001e2, DRAMC_BASE + 0x0188);

	writel(0x00000022, DRAMC_BASE + 0x0400);
	writel(0x00000000, DRAMC_BASE + 0x0404);
	writel(0x00000000, DRAMC_BASE + 0x0408);
	writel(0x00000000, DRAMC_BASE + 0x040C);
	writel(0x00000000, DRAMC_BASE + 0x0410);
	writel(0x00000000, DRAMC_BASE + 0x0414);
	writel(0x00000000, DRAMC_BASE + 0x0418);
	writel(0x00000000, DRAMC_BASE + 0x041C);
	writel(0xaa000040, DRAMC_BASE + 0x0420);
	writel(0x00000000, DRAMC_BASE + 0x0424);
	writel(0x00000000, DRAMC_BASE + 0x0428);
	writel(0x00000000, DRAMC_BASE + 0x042C);
	writel(0xffff0003, DRAMC_BASE + 0x0430);
	writel(0xffff0000, DRAMC_BASE + 0x0434);
	writel(0x00000c0c, DRAMC_BASE + 0x0438);
	writel(0x00660600, DRAMC_BASE + 0x043C);
	writel(0x00660600, DRAMC_BASE + 0x0440);
	writel(0x80690608, DRAMC_BASE + 0x0444);
	writel(0xc0690608, DRAMC_BASE + 0x0448);
	writel(0x00000000, DRAMC_BASE + 0x044C);
	writel(0x00000000, DRAMC_BASE + 0x0450);
	writel(0x00000000, DRAMC_BASE + 0x0454);
	writel(0x00000000, DRAMC_BASE + 0x0458);
	writel(0x00000000, DRAMC_BASE + 0x0460);
	writel(0x00000000, DRAMC_BASE + 0x0468);
	writel(0x00000000, DRAMC_BASE + 0x0088);
	writel(0x00000000, DRAMC_BASE + 0x0028);
	writel(0x00000000, DRAMC_BASE + 0x0454);
	gpt4_busy_wait_ms(1);
	writel(0x00010000, DRAMC_BASE + 0x0454);
	gpt4_busy_wait_ms(1);
	writel(0x80000000, DRAMC_BASE + 0x0404);
	gpt4_busy_wait_ms(1);
	writel(0x00008c8c, DRAMC_BASE + 0x0438);
	gpt4_busy_wait_ms(1);
	writel(0x00660e00, DRAMC_BASE + 0x043C);
	writel(0x00660e00, DRAMC_BASE + 0x0440);
	writel(0x80690e08, DRAMC_BASE + 0x0444);
	writel(0xc0690e08, DRAMC_BASE + 0x0448);
	gpt4_busy_wait_ms(1);
	writel(0xffff0000, DRAMC_BASE + 0x0430);
	writel(0x0000271e, DRAMC_BASE + 0x0014);
	writel(0x0000071e, DRAMC_BASE + 0x0074);

	/* To DRAMC */
	writel(0x33333300, DRAMC_BASE + 0x2438);
	writel(0x51515100, DRAMC_BASE + 0x243C);
	writel(0x00000000, DRAMC_BASE + 0x2400);
	writel(0x00303000, DRAMC_BASE + 0x2404);
	writel(0x22223300, DRAMC_BASE + 0x2430);
	/* gated */
	writel(0x51515100, DRAMC_BASE + 0x2434);
	/* writel(0x62626200, DRAMC_BASE + 0x2434); */

	writel(0x00020335, DRAMC_BASE + 0x2418);
	writel(0x00000000, DRAMC_BASE + 0x2408);
	writel(0x00000000, DRAMC_BASE + 0x240C);
	/* gated */
	writel(0x55455545, DRAMC_BASE + 0x2410);
	/* writel(0x56855545, DRAMC_BASE + 0x2410); */

	writel(0x11112222, DRAMC_BASE + 0x241C);
	writel(0x11112222, DRAMC_BASE + 0x2420);
	writel(0x33333333, DRAMC_BASE + 0x2440);
	writel(0x33333333, DRAMC_BASE + 0x2444);
	writel(0x77777777, DRAMC_BASE + 0x2448);
	writel(0x44444444, DRAMC_BASE + 0x244C);
	writel(0x11112222, DRAMC_BASE + 0x2424);

	/* DQ dly */
	writel(0x000f0a0a, DRAMC_BASE + 0x2428);
	/* DQS dly */
	writel(0x00050055, DRAMC_BASE + 0x242C);
	writel(0x00ff00f0, DRAMC_BASE + 0x2458);
	writel(0xa0000000, DRAMC_BASE + 0x2454);

	writel(0x00000000, DRAMC_BASE + 0x21E0);
	writel(0x040000c1, DRAMC_BASE + 0x21F8);
	writel(0x30430000, DRAMC_BASE + 0x21C4);
	writel(0x00000000, DRAMC_BASE + 0x223C);

	/* refresh setting */
	writel(0x00403318, DRAMC_BASE + 0x2008);
	/* writel(0x90182840, DRAMC_BASE + 0x21dc); */
	writel(0x90062840, DRAMC_BASE + 0x21dc);

	writel(0x2d000001, DRAMC_BASE + 0x21E8);
	writel(0x2200110d, DRAMC_BASE + 0x2048);
	writel(0x00000001, DRAMC_BASE + 0x208C);
	writel(0x00100510, DRAMC_BASE + 0x20D8);
	writel(0x00082023, DRAMC_BASE + 0x20E4);
	writel(0x99169952, DRAMC_BASE + 0x20B8);
	writel(0x99109950, DRAMC_BASE + 0x20BC);
	writel(0x00000000, DRAMC_BASE + 0x2090);
	writel(0x83000000, DRAMC_BASE + 0x20DC);

	writel(0x10000000, DRAMC_BASE + 0x20E0);
	writel(0x00000000, DRAMC_BASE + 0x2118);
	writel(0x10000000, DRAMC_BASE + 0x20F0);
	writel(0x0100000c, DRAMC_BASE + 0x25C0);
	writel(0x01000000, DRAMC_BASE + 0x20F4);
	writel(0x00000000, DRAMC_BASE + 0x2168);
	writel(0x70000000, DRAMC_BASE + 0x2130);
	writel(0x00300510, DRAMC_BASE + 0x20D8);
	writel(0xf00086a3, DRAMC_BASE + 0x2004);
	writel(0xc0000011, DRAMC_BASE + 0x2124);
	writel(0x00b08000, DRAMC_BASE + 0x2138);
	writel(0x00000000, DRAMC_BASE + 0x2094);
	writel(0x00000000, DRAMC_BASE + 0x2098);
	writel(0x11110000, DRAMC_BASE + 0x21C0);
	writel(0x00000008, DRAMC_BASE + 0x2240);

	writel(0x668e4719, DRAMC_BASE + 0x2000);
	writel(0x17030000, DRAMC_BASE + 0x20FC);
	writel(0x040000c1, DRAMC_BASE + 0x21F8);
	writel(0x10000003, DRAMC_BASE + 0x21EC);
	writel(0x0781548c, DRAMC_BASE + 0x207C);

	/* 2CH diff */
	/* 2ch orig fail writel(0x70070700, DRAMC_BASE + 0x2080); */
	writel(0x70070700, DRAMC_BASE + 0x2080);
	/* 2CH diff */

	writel(0xd0000000, DRAMC_BASE + 0x2028);
	writel(0x2d000001, DRAMC_BASE + 0x21E8);
	writel(0x00000000, DRAMC_BASE + 0x2158);
	/* 1CH bk;writel(0x00697780, DRAMC_BASE + 0x2110); */

	/* 2CH diff */
	/* writel(0x00697780, DRAMC_BASE + 0x2110); */
	writel(0x006977b1, DRAMC_BASE + 0x2110);
	/* 2CH diff */

	writel(0xf07486a3, DRAMC_BASE + 0x2004);
	writel(0x00000027, DRAMC_BASE + 0x20E4);

	/* 2CH diff */
	 /*eric 2ranks parameter */
	 /*gated rk1 */
	writel(0x00020335, DRAMC_BASE + 0x2418);
	writel(0x22223300, DRAMC_BASE + 0x2438);
	writel(0x51515100, DRAMC_BASE + 0x243C);

	writel(0x00000000, DRAMC_BASE + 0x2400);
	writel(0x00000000, DRAMC_BASE + 0x2408);
	writel(0x00000000, DRAMC_BASE + 0x240C);

	/* TX rk1 */
	writel(0x11112222, DRAMC_BASE + 0x2440);
	writel(0x11112222, DRAMC_BASE + 0x2444);
	/* DQ */
	writel(0x00220022, DRAMC_BASE + 0x2448);
	/* DQS */
	writel(0x44444477, DRAMC_BASE + 0x244C);

	/* rank swap [3] */
	/* writel0x00697788(, DRAMC_BASE + 0x2110); */

	/* eric add for 2ranks */
	/* CKE */
	writel(0x00010c1a, DRAMC_BASE + 0x2348);
	writel(0x10100003, DRAMC_BASE + 0x21EC);

	/* 2CH diff */
	/* ERIC */
	/* MRS */
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x0000000d, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	/* ODT */
	writel(0x0000000b, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00540001, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00120002, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00310003, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	/* MR15 */
	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00FF000F, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);

	/* MR20 */
	writel(0x00000000, DRAMC_BASE + 0x21E4);
	writel(0x00FF0014, DRAMC_BASE + 0x2088);
	writel(0x00000001, DRAMC_BASE + 0x21E4);
	gpt4_busy_wait_ms(1);
}

/* Init DDR */
void mt_mem_init_fpga(void)
{
	int flag;

	flag = readl(DRAMC_BASE);
	if (flag & (1 << 31))
		fpga_emi_init_2ch();
	else
		fpga_emi_init();
}

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
/* Include Files */

#include <platform/dramc_common.h>
#include <platform/reg_emi_reg.h>
#include <platform.h>
#include <platform/emi.h>
#include <platform/mt_reg_base.h>
#ifdef LASTPC_READY
#include <platform/lastpc.h>
#endif

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif

static unsigned int get_dramc_addr(dram_addr_t *dram_addr,
	unsigned int offset);

/*  Global Variables */
int emi_setting_index = -1;
extern EMI_SETTINGS default_emi_setting;

/*  External references */
extern char *opt_dle_value;

#define EMI_APB_BASE EMI_BASE

#define EMI_REG_CONA	(EMI_APB_BASE + EMI_CONA)
#define EMI_REG_CONF	(EMI_APB_BASE + EMI_CONF)
#define EMI_REG_CONH	(EMI_APB_BASE + EMI_CONH)

/* No check, do not use in bringup*/
#define CQ_DMA_G_DMA_CON 0x018
#define CQ_DMA_G_DMA_SRC_ADDR 0x01c
#define CQ_DMA_G_DMA_SRC_ADDR2 0x060
#define CQ_DMA_G_DMA_DST_ADDR 0x020
#define CQ_DMA_G_DMA_DST_ADDR2 0x064
#define CQ_DMA_G_DMA_LEN1 0x024
#define CQ_DMA_G_DMA_EN 0x008

#define PHY_ADDR_OFFSET 0x40000000

void EMI_ESL_Setting1(void)
{
	//ESL setting
	//# Row =14bit
	*((UINT32P)(EMI_APB_BASE+0x00000010))=0x0a1a0b1a;
	*((UINT32P)(EMI_APB_BASE+0x00000018))=0x3657587a;
	*((UINT32P)(EMI_APB_BASE+0x00000020))=0xffff0848;
	*((UINT32P)(EMI_APB_BASE+0x00000028))=0x00000000;
	*((UINT32P)(EMI_APB_BASE+0x00000030))=0x2b2b2a38;
	*((UINT32P)(EMI_APB_BASE+0x00000038))=0x00000000;
	*((UINT32P)(EMI_APB_BASE+0x00000040))=0x00008803;
	*((UINT32P)(EMI_APB_BASE+0x00000060))=0x00000dff;
	#if SUPPORT_TYPE_PCDDR4
	*((UINT32P)(EMI_APB_BASE+0x00000060))|=  (0x1 << 11); /*CONM[11] :age_slow*/
	#endif
	*((UINT32P)(EMI_APB_BASE+0x00000078))=0x12000000;
	*((UINT32P)(EMI_APB_BASE+0x0000007c))=0x00002300;

	*((UINT32P)(EMI_APB_BASE+0x000000d0))=0xffffffff;
	*((UINT32P)(EMI_APB_BASE+0x000000d8))=0xff888888;

	*((UINT32P)(EMI_APB_BASE+0x000000e8))=0x00200027;
	*((UINT32P)(EMI_APB_BASE+0x000000f0))=0x38460000;
	*((UINT32P)(EMI_APB_BASE+0x000000f8))=0x00000000;
	*((UINT32P)(EMI_APB_BASE+0x000000fc))=0x400f000f;

	*((UINT32P)(EMI_APB_BASE+0x00000100))=0xffff5c5b; //cpu
	*((UINT32P)(EMI_APB_BASE+0x00000108))=0xffff5c5b; //cpu

	*((UINT32P)(EMI_APB_BASE+0x00000110))=0xffff7042;//M2
	*((UINT32P)(EMI_APB_BASE+0x00000128))=0xffff7042;//M5
	*((UINT32P)(EMI_APB_BASE+0x00000130))=0xffff7042;//M6
	*((UINT32P)(EMI_APB_BASE+0x00000138))=0xffff7043;//MM

	*((UINT32P)(EMI_APB_BASE+0x00000140))=0x20406188;
	*((UINT32P)(EMI_APB_BASE+0x00000144))=0x20406188;
	*((UINT32P)(EMI_APB_BASE+0x00000148))=0x37684848;
	*((UINT32P)(EMI_APB_BASE+0x0000014c))=0x3719595e;
	*((UINT32P)(EMI_APB_BASE+0x00000150))=0x64f3fc79;
	*((UINT32P)(EMI_APB_BASE+0x00000154))=0x64f3fc79;
	*((UINT32P)(EMI_APB_BASE+0x00000158))=0x00000000;

}

void emi_esl_setting2(void)
{

}

void emi_patch(void)
{
}

void emi_init(DRAMC_CTX_T *p)
{
	EMI_SETTINGS *emi_set;

	if (emi_setting_index == -1)
		emi_set = &default_emi_setting;

	EMI_ESL_Setting1(); //Copy Paste from DE

	*((volatile unsigned *)(EMI_APB_BASE + EMI_CONA)) = emi_set->EMI_CONA_VAL;
    *((volatile unsigned *)(EMI_APB_BASE + EMI_CONH)) = emi_set->EMI_CONH_VAL; /* rank size*/
    *((volatile unsigned *)(EMI_APB_BASE + EMI_CONF)) = emi_set->EMI_CONF_VAL; /* scramble */
    *((volatile unsigned *)(EMI_APB_BASE + EMI_CONM))|=  (0x1 << 10); /* bit 10: emi enable */

    DSB;
	p->vendor_id = emi_set->iLPDDR3_MODE_REG_5;
}

void emi_init2(void)
{
	emi_esl_setting2();
	emi_patch();
}

int get_dram_channel_nr(void)
{
    return 1;
}

int get_dram_rank_nr(void)
{
	int emi_cona;

	emi_cona = default_emi_setting.EMI_CONA_VAL;

	if ((emi_cona & (1 << 17)) != 0)
		return 2; /* 2 Ranks */
	else
		return 1; /* 1 Rank */
}

#define get_col_bit(value, shift)	(((value >> shift) & 0x03) + 9)
#define get_row_bit(value, shift1, shift2)	\
	(((((value >> shift1) & 0x01) << 2) + ((value >> shift2) & 0x03)) + 13)
#define get_row_bit1(value1, value2, shift1, shift2)	\
	(((((value1 >> shift1) & 0x01) << 2) + \
	((value2 >> shift2) & 0x03)) + 13)
void get_dram_rank_size_by_emi_cona(u64 dram_rank_size[])
{
	unsigned col_bit, row_bit;
	unsigned emi_cona = default_emi_setting.EMI_CONA_VAL;
	unsigned shift_for_16bit = 1;
	u64 rank0_size;

	if (emi_cona & 0x2)  /* bit 1 */
		shift_for_16bit = 0;

	dram_rank_size[0] = 0;
	dram_rank_size[1] = 0;

	/* EMI */

	col_bit = get_col_bit(emi_cona, 4);
	row_bit = get_row_bit(emi_cona, 24, 12);
	rank0_size = ((u64) (1 << (row_bit + col_bit))) *
			((u64) (4 >> shift_for_16bit) * 8);
	dram_rank_size[0] = rank0_size;

	show_msg((INFO,
		"DRAM rank0 size:0x%llx,\nDRAM rank1 size=0x%llx\n",
		dram_rank_size[0], dram_rank_size[1]));
}

void get_dram_rank_size(unsigned long long dram_rank_size[])
{
	get_dram_rank_size_by_emi_cona(dram_rank_size);
}

#ifdef DUMMY_READ_FOR_TRACKING
unsigned long long phy_addr_to_dram_addr1(dram_addr_t *dram_addr,
	unsigned long long phy_addr)
{
	unsigned long long rank_size[4];
	unsigned int emi_conf, bit_xor, rank_num;
	unsigned int index, bit_shift;

	get_dram_rank_size_by_emi_cona(rank_size);
	emi_conf = *((volatile unsigned int *)EMI_REG_CONF) >> 8;
	rank_num = (unsigned int)get_dram_rank_nr();

	phy_addr -= PHY_ADDR_OFFSET;
	for (index = 0; index < rank_num; index++) {
		if (phy_addr >= rank_size[index])
			phy_addr -= rank_size[index];
		else
			break;
	}

	for (index = 11; index < 17; index++) {
		bit_xor = (emi_conf >> (4 * (index - 11))) & 0xf;
		bit_xor &= phy_addr >> 16;
		for (bit_shift = 0; bit_shift < 4; bit_shift++)
			phy_addr ^= ((bit_xor >> bit_shift) & 0x1) << index;
	}
	return phy_addr;
}

void phy_addr_to_dram_addr(dram_addr_t *dram_addr, unsigned long long phy_addr)
{
	unsigned int emi_cona;
	unsigned int ch_num;
	unsigned int bit_shift, ch_pos, ch_width;
	unsigned int temp;

	emi_cona = *((volatile unsigned int *)EMI_REG_CONA);
	ch_num = (unsigned int)get_dram_channel_nr();

	phy_addr = phy_addr_to_dram_addr1(dram_addr, phy_addr);

	if (ch_num > 1) {
		ch_pos = ((emi_cona >> 2) & 0x3) + 7;

		for (ch_width = bit_shift = 0; bit_shift < 4; bit_shift++) {
			if ((unsigned int)(1 << bit_shift) >= ch_num)
				break;
			ch_width++;
		}

		switch (ch_width) {
		case 2:
			dram_addr->addr = ((phy_addr
				& ~(((0x1 << 2) << ch_pos) - 1)) >> 2);
			break;
		default:
			dram_addr->addr = ((phy_addr
				& ~(((0x1 << 1) << ch_pos) - 1)) >> 1);
			break;
		}

		dram_addr->addr |= (phy_addr & ((0x1 << ch_pos) - 1));
	}

	temp = dram_addr->addr >> 1;
	switch ((emi_cona >> 4) & 0x3) {
	case 0:
		dram_addr->col = temp & 0x1FF;
		temp = temp >> 9;
		break;
	case 1:
		dram_addr->col = temp & 0x3FF;
		temp = temp >> 10;
		break;
	case 2:
	default:
		dram_addr->col = temp & 0x7FF;
		temp = temp >> 11;
		break;
	}
	dram_addr->bk = temp & 0x7;
	temp = temp >> 3;

	dram_addr->row = temp;
}

#define CQ_DMA_BASE			0x0

void put_dummy_read_pattern(unsigned long long dst_pa, unsigned int src_pa,
	unsigned int len)
{
#if (CQ_DMA_BASE == 0) /* cc add in case Unexpected result */
	show_err2("%s while CQ_DMA_BASE is not configured\n",
		"[Error] put_dummy_read_pattern() invoked");
	while(1);
#endif

	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_CON)) = 7 << 16;

	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_SRC_ADDR))
		= src_pa;
	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_SRC_ADDR2)) = 0;

	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_DST_ADDR))
		= dst_pa & 0xffffffff;
	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_DST_ADDR2))
		= dst_pa >> 32;

	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_LEN1)) = len;
	DSB;
	*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_EN)) = 0x1;

	while
		(*((volatile unsigned int *)(CQ_DMA_BASE + CQ_DMA_G_DMA_EN)));
}

static unsigned int get_dramc_addr(dram_addr_t *dram_addr, unsigned int offset)
{
	unsigned int ch_num, rank_num;
	unsigned long long dummy_read_addr;
	unsigned long long rank_size[4];
	unsigned int index;
	unsigned int *src_addr;

	ch_num = (unsigned int)get_dram_channel_nr();
	rank_num = (unsigned int)get_dram_rank_nr();
	get_dram_rank_size_by_emi_cona(rank_size);
	dummy_read_addr = PHY_ADDR_OFFSET;
	src_addr = (unsigned int *)PHY_ADDR_OFFSET;

	if (dram_addr->ch >= ch_num) {
		show_msg((INFO, "[DRAMC] invalid channel: %d\n",
			dram_addr->ch));
		return 0;
	}

	if (dram_addr->rk >= rank_num) {
		show_msg((INFO, "[DRAMC] invalid rank: %d\n",
			dram_addr->rk));
		return 0;
	}

	for (index = 0; index <= dram_addr->rk; index++)
		dummy_read_addr += rank_size[index];
	dummy_read_addr -= offset;

	/* cc notes: only consider CH0?? */
	if (dram_addr->ch == 0)
		dummy_read_addr &= ~(0x100);

	if (offset == 0x20) { /* 32bit */
		for (index = 0; index < 4; index++)
			*(src_addr + index) = PATTERN3;
		put_dummy_read_pattern
			(dummy_read_addr, (unsigned int)src_addr, 16);
	}

	dram_addr->full_sys_addr = dummy_read_addr;
	phy_addr_to_dram_addr(dram_addr, dummy_read_addr);

	return dram_addr->addr;
}

unsigned int get_dummy_read_addr(dram_addr_t *dram_addr)
{
	return get_dramc_addr(dram_addr, 0x20);
}
#endif

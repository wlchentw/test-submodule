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
 /** @file hal_io.cpp
 *  hal_io.cpp provides functions of register access
 */

#include <platform/x_hal_io.h>
#include <platform/dramc_common.h>
#include <platform/dramc_api.h>

#define ADDR_OFFSET_100		0x100
#define ADDR_OFFSET_200		0x200
#define ADDR_OFFSET_8		0x8

/* IF set, when io_32_write_xxx_all() is invoked,
 * the value will be written to both PHY B01 and
 * PHY B23 even if the data width is 16BIT.
 *
 * Mainly used during initial stage since some
 * RGs (B0_DLL_ARPI0 for example) are defined
 * in both B01/B23, but their content are different,
 * and writting only to B23 may not work.
 * Other hands, it's difficult for SW to decide
 * which Byte shall be written for these RGs.
 * So a simple and safe way (but cause efficient downgrade)
 * is to tell the IO interface to write to both bytes.
 */
static unsigned int sw_broadcast = 0;

void io_set_sw_broadcast(unsigned int value)
{
	sw_broadcast = value;
}

unsigned int io_get_sw_broadcast(void)
{
	return sw_broadcast;
}

unsigned int io_get_set_sw_broadcast(unsigned int value)
{
	unsigned int ret;

	ret = sw_broadcast;
	sw_broadcast = value;

	return ret;
}

/* Although B23 may be used for LP4 and DDR3X16,
 * but some regs are located in B01 only, including
 * CA related registers, PLL related, some MISC
 * related registers.
 * ...
 */
static inline unsigned int is_reg_in_b01_only(U64 addr)
{
	unsigned int is;

	is = 0;

	if (addr < 0x80) {
		is = 1; /* PLL0~PLL8 */
	} else if (addr >= 0x13c && addr < 0x284) {
		is = 1; /* SEL_MAX0 ~ MISC_SPM_CTRL2 */
	} else if (addr >= 0xd00 && addr < 0xe00) {
		is = 1; /* SHU1_CA_CMD0 ~ SHU1_PLL11 */
	} else if (addr >= 0xea0 && addr < 0xf00) {
		is = 1; /* SHU1_RK0_CA_CDM0 ~ SHU1_RK0_CA_CMD10 */
	}

	return is;
}

/* If sw_broadcast is not set (means SW does not want to touch
 * both channels) while B23 is used, the IO shall
 * be mapped to PHY B23.
 */
static inline unsigned int is_to_b23_only(DRAMC_CTX_T *p, U64 reg)
{
	unsigned int is;

	is = 0;

	if ((sw_broadcast == 0) &&
		(is_lp4_family(p->dram_type) ||
		((p->dram_type == TYPE_PCDDR3) &&
		 (p->data_width == DATA_WIDTH_16BIT)))) {
		is = 1;
	}

	return is;
}

#define reg32_type(reg32)	\
	((reg32) & (0xf << POS_BANK_NUM))

unsigned long long reg_base_addr_traslate(DRAMC_CTX_T *p, U64 reg_addr)
{
	U64 reg_type;
	unsigned int offset;
	U64 base_addr = 0;

#if 0
	offset = reg_addr & 0xffff;
	reg_type = reg32_type(reg_addr);

	if (reg_type >= DDRPHY_BASE_ADDR_VIRTUAL) {
		if (is_reg_in_b01_only(offset)) {
			offset += DDRPHY_BASE_ADDR_VIRTUAL;
		} else if (is_to_b23_only(p, reg_addr)) {
			/* LP4 and DDR3X16 uses B23 */
			offset += DDRPHY_B23_BASE_VIRTUAL;
		} else {
			offset += reg_type;
		}
	} else {
		/* DRAMC */
		offset += reg_type;
	}

	reg_type = ((offset -
			DRAMC_NAO_BASE_VIRTUAL) >> POS_BANK_NUM) & 0xf;
	offset &= 0xffff;
#else
	reg_type = ((reg_addr -
		DRAMC_NAO_BASE_VIRTUAL) >> POS_BANK_NUM) & 0xf;
	offset = reg_addr & 0xffff;
#endif

	if (reg_addr < DRAMC_NAO_BASE_VIRTUAL ||
		reg_addr >= MAX_BASE_VIRTUAL)
		return reg_addr;

	switch (reg_type) {
	case 0:
	case 1:
		base_addr = DRAMC_NAO_BASE_PHYS;
		break;
	case 2:
	case 3:
		base_addr = DRAMC_AO_BASE_PHYS;
		break;
	case 4:
		base_addr = DDRPHY_BASE_PHYS;
		break;
	case 5:
		base_addr = DDRPHY_B23_BASE_PHYS;
		break;
	default:
		show_msg((CRITICAL, "[Error] %s 0x%llx\n",
			"Addr Translate: Unrecognized reg addr:", reg_addr));
		break;
	}

	return (base_addr + offset);
}

unsigned int dram_register_read(DRAMC_CTX_T *p, U64 reg_addr)
{
	unsigned int reg_value;

	reg_addr = reg_base_addr_traslate(p, reg_addr);

#if (FOR_DV_SIMULATION_USED == 1)
	reg_value = register_read_c(reg_addr);
#else
	reg_value = readl(reg_addr);
#endif

	return reg_value;
}

unsigned char dram_register_write(DRAMC_CTX_T *p, U64 reg_addr,
unsigned int reg_value)
{
	unsigned char ucstatus;

	ucstatus = 0;

	reg_addr = reg_base_addr_traslate(p, reg_addr);

#if (FOR_DV_SIMULATION_USED == 1)
	register_write_c(reg_addr, reg_value);
#else
	writel(reg_value, reg_addr);
#endif
	DSB;

	return ucstatus;
}

void io32_write_4b_msk2(DRAMC_CTX_T *p, U64 reg32,
	unsigned int val32, unsigned int msk32)
{
	unsigned int u4Val;

	val32 &= msk32;

	u4Val = dram_register_read(p, reg32);
	u4Val = ((u4Val & ~msk32) | val32);
	dram_register_write(p, reg32, u4Val);
}

void io32_write_4b_all2(DRAMC_CTX_T *p, U64 reg32, unsigned int val32)
{
	unsigned char ii, all_count;
	U64 reg_type = reg32_type(reg32);

	reg32 &= 0xffff;
	all_count = 1;

	/* cc notes: For DDRPHY, we have B01 and B23 to set.
	 * For DRAMC, nothing to do.
	 */
#if 0
	if ((reg_type >= DDRPHY_BASE_ADDR_VIRTUAL) &&
		((p->data_width == DATA_WIDTH_32BIT) ||
		(sw_broadcast == 1))) {
		reg32 += DDRPHY_BASE_ADDR_VIRTUAL;

		/* PLL related, shall only access B01 REG */
		if (!is_reg_in_b01_only(reg32))
			all_count ++;
	} else {
		reg32 += reg_type;
	}
#else
	if (reg_type >= DDRPHY_BASE_ADDR_VIRTUAL) {
		reg32 += DDRPHY_BASE_ADDR_VIRTUAL;
		all_count ++;
	} else {
		reg32 += reg_type;
	}
#endif

	for (ii = 0; ii < all_count; ii++) {
		io32_write_4b(reg32 + ((unsigned int) ii << POS_BANK_NUM),
			val32);
	}

}

void io32_write_4b_msk_all2(DRAMC_CTX_T *p, U64 reg32,
	unsigned int val32, unsigned int msk32)
{
	unsigned int u4Val;
	unsigned char ii, all_count;
	U64 reg_type = reg32_type(reg32);

	reg32 &= 0xffff;
	all_count = 1;

	/* cc notes: For DDRPHY, we have B01 and B23 to set for LP3.
	 * For DRAMC, nothing to do.
	 */
#if 0
	if ((reg_type >= DDRPHY_BASE_ADDR_VIRTUAL) &&
		((p->data_width == DATA_WIDTH_32BIT) ||
		(sw_broadcast == 1))) {
		reg32 += DDRPHY_BASE_ADDR_VIRTUAL;

		/* PLL related, shall only access B01 REG */
		if (!is_reg_in_b01_only(reg32))
			all_count ++;
	} else {
		reg32 += reg_type;
	}
#else
	if (reg_type >= DDRPHY_BASE_ADDR_VIRTUAL) {
		reg32 += DDRPHY_BASE_ADDR_VIRTUAL;
		all_count ++;
	} else {
		reg32 += reg_type;
	}
#endif

	for (ii = 0; ii < all_count; ii++) {
		u4Val = dram_register_read(p, reg32 +
			((unsigned int) ii << POS_BANK_NUM));
		u4Val = ((u4Val & ~msk32) | val32);
		dram_register_write(p, reg32 +
			((unsigned int) ii << POS_BANK_NUM), u4Val);
	}
}

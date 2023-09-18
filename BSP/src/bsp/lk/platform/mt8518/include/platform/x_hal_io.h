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

#ifndef X_HAL_IO_H
#define X_HAL_IO_H

#include <platform/dramc_api.h>
#include <reg.h>
#include <platform/typedefs.h>


/* field access macro---------------------------------------------------------*/

/* access method*/
#define AC_FULLB0       1
#define AC_FULLB1       2
#define AC_FULLB2       3
#define AC_FULLB3       4
#define AC_FULLW10      5
#define AC_FULLW21      6
#define AC_FULLW32      7
#define AC_FULLDW       8
#define AC_MSKB0        11
#define AC_MSKB1        12
#define AC_MSKB2        13
#define AC_MSKB3        14
#define AC_MSKW10       15
#define AC_MSKW21       16
#define AC_MSKW32       17
#define AC_MSKDW        18

/* cc add. Used to generate bit mask for bits definition.
 * for example, for XXX[4:3], this will generate 0x18 as the mask for it.
 */
#define MSB_POS		(sizeof(unsigned long) * 8 - 1)
#define GENMASK(msb, lsb) \
	(((~0UL) << (lsb)) & ((~0UL) >> (MSB_POS - (msb))))
#define BIT(nr)		(1 << (nr))

/* NEW_CODA comment old code*/
/* #define Fld2Msk32(fld) ... */

extern unsigned int dram_register_read(DRAMC_CTX_T *p, U64 u4reg_addr);
extern unsigned char dram_register_write(DRAMC_CTX_T *p,
	U64 u4reg_addr, unsigned int u4reg_value);

extern void io32_write_4b_msk2(DRAMC_CTX_T *p, U64 reg32,
	unsigned int val32, unsigned int msk32);
extern void io32_write_4b_msk_all2(DRAMC_CTX_T *p, U64 reg32,
	unsigned int val32, unsigned int msk32);
extern void io32_write_4b_all2(DRAMC_CTX_T *p, U64 reg32,
	unsigned int val32);

/* public Macro for general use. */
#define io32_read_4b(reg32) dram_register_read(p, reg32)
#define io32_write_4b(reg32, val32) dram_register_write(p, reg32, val32)
#define io32_write_4b_all(reg32, val32) io32_write_4b_all2(p, reg32, val32)
#define io32_write_4b_msk(reg32, val32, msk32) \
	io32_write_4b_msk2(p, reg32, val32, msk32)
#define io32_write_4b_msk_all(reg32, val32, msk32) \
	io32_write_4b_msk_all2(p, reg32, val32, msk32)

#define ffld(wid, shft, ac)	GENMASK(shft + wid - 1, shft)
#define fld_wid(fld) (unsigned char)(__builtin_popcount(fld))

/*
 * This macro should be used with "io_32_write_fld_multi"
 * or "io_32_write_fld_multi_all"
 * where "upk" variable is declared by these two functions.
*/
#define p_fld(val, msk) \
	((sizeof(upk) > 1) ? ((unsigned int)msk) : ((unsigned int)msk & \
	((unsigned int)(val) << (__builtin_ffsl((unsigned int)msk) - 1))))

#define io_32_read_fld_align(reg32, msk) \
	((((unsigned int)msk) == 0xffffffff) ? io32_read_4b(reg32) : \
	((io32_read_4b(reg32) & ((unsigned int)msk)) >> \
	(__builtin_ffsl((unsigned int)msk) - 1)))

#define io_32_write_fld_align(reg32, val, msk) \
	((((unsigned int)msk) == 0xffffffff) ? \
	(io32_write_4b((reg32), (val))) : \
	(io32_write_4b_msk((reg32), ((unsigned int)(val) << \
	(__builtin_ffsl((unsigned int)msk) - 1)), ((unsigned int)msk))))

/* The "upk" variable is used in the "p_fld" macro. */
#define io_32_write_fld_multi(reg32, list) \
{ \
	unsigned short upk; \
	signed int msk = (signed int)(list); \
	{ \
		unsigned char upk; \
		((unsigned int)msk == 0xffffffff) ? \
		(io32_write_4b(reg32, (list))) : (((unsigned int)msk) ? \
		io32_write_4b_msk((reg32), (list), ((unsigned int)msk)) : 0); \
	} \
}

#define io_32_write_fld_align_all(reg32, val, msk) \
	((((unsigned int)msk) == 0xffffffff) ? \
	(io32_write_4b_all((reg32), (val))) : \
	(io32_write_4b_msk_all((reg32), \
	((unsigned int)(val) << (__builtin_ffsl((unsigned int)msk) - 1)), \
	((unsigned int)msk))))

/* The "upk" variable is used in the "p_fld" macro. */
#define io_32_write_fld_multi_all(reg32, list) \
{ \
	unsigned short upk; \
	signed int msk = (signed int)(list); \
	{ \
		unsigned char upk; \
		((unsigned int)msk == 0xffffffff) ? \
		(io32_write_4b_all(reg32, (list))) : (((unsigned int)msk) ? \
		io32_write_4b_msk_all((reg32), (list), \
		((unsigned int)msk)) : 0); \
	} \
}

void io_set_sw_broadcast(unsigned int value);
unsigned int io_get_sw_broadcast(void);
unsigned int io_get_set_sw_broadcast(unsigned int value);

/* NEW_CODA end */

#endif

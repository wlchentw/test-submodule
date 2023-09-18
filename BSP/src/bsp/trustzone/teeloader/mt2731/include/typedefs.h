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
 * MediaTek Inc. (C) 2017. All rights reserved.
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
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;
typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
typedef signed int intx;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned int uintx;

typedef volatile unsigned char *P_U8;
typedef volatile signed char *P_S8;
typedef volatile unsigned short *P_U16;
typedef volatile signed short *P_S16;
typedef volatile unsigned int *P_U32;
typedef volatile signed int *P_S32;
typedef unsigned long long *P_U64;
typedef signed long long *P_S64;

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;

//------------------------------------------------------------------
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned short USHORT;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;
typedef signed int DWORD;
typedef void VOID;
typedef unsigned char BYTE;
typedef float FLOAT;

typedef char *LPCSTR;
typedef short *LPWSTR;

//------------------------------------------------------------------
typedef char __s8;
typedef unsigned char __u8;
typedef short __s16;
typedef unsigned short __u16;
typedef int __s32;
typedef unsigned int __u32;
typedef long long __s64;
typedef unsigned long long __u64;
typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

typedef unsigned long uintptr_t;
typedef u64 uint64_t;
typedef u32 uint32_t;
typedef u32 size_t;

//------------------------------------------------------------------
#ifndef FALSE
#define FALSE   (0U)
#endif
#ifndef TRUE
#define TRUE    (1U)
#endif

#ifndef NULL
#define NULL    (0U)
#endif

/*==== EXPORTED MACRO ===================================================*/
#define READ_REGISTER_UINT32(reg) \
    (*(volatile UINT32 * const)(reg))

#define WRITE_REGISTER_UINT32(reg, val) \
    (*(volatile UINT32 * const)(reg)) = (val)

#define READ_REGISTER_UINT16(reg) \
    (*(volatile UINT16 * const)(reg))

#define WRITE_REGISTER_UINT16(reg, val) \
    (*(volatile UINT16 * const)(reg)) = (val)

#define READ_REGISTER_UINT8(reg) \
    (*(volatile UINT8 * const)(reg))

#define WRITE_REGISTER_UINT8(reg, val) \
    (*(volatile UINT8 * const)(reg)) = (val)

#define INREG8(x)                   READ_REGISTER_UINT8((UINT8*)(x))
#define OUTREG8(x, y)               WRITE_REGISTER_UINT8((UINT8*)(x), (UINT8)(y))
#define SETREG8(x, y)               OUTREG8(x, INREG8(x)|(y))
#define CLRREG8(x, y)               OUTREG8(x, INREG8(x)&~(y))
#define MASKREG8(x, y, z)           OUTREG8(x, (INREG8(x)&~(y))|(z))

#define INREG16(x)                  READ_REGISTER_UINT16((UINT16*)(x))
#define OUTREG16(x, y)              WRITE_REGISTER_UINT16((UINT16*)(x),(UINT16)(y))
#define SETREG16(x, y)              OUTREG16(x, INREG16(x)|(y))
#define CLRREG16(x, y)              OUTREG16(x, INREG16(x)&~(y))
#define MASKREG16(x, y, z)          OUTREG16(x, (INREG16(x)&~(y))|(z))

#define INREG32(x)                  READ_REGISTER_UINT32((UINT32*)(x))
#define OUTREG32(x, y)              WRITE_REGISTER_UINT32((UINT32*)(x), (UINT32)(y))
#define SETREG32(x, y)              OUTREG32(x, INREG32(x)|(y))
#define CLRREG32(x, y)              OUTREG32(x, INREG32(x)&~(y))
#define MASKREG32(x, y, z)          OUTREG32(x, (INREG32(x)&~(y))|(z))

#define DRV_Reg8(addr)              INREG8(addr)
#define DRV_WriteReg8(addr, data)   OUTREG8(addr, data)
#define DRV_SetReg8(addr, data)     SETREG8(addr, data)
#define DRV_ClrReg8(addr, data)     CLRREG8(addr, data)

#define DRV_Reg16(addr)             INREG16(addr)
#define DRV_WriteReg16(addr, data)  OUTREG16(addr, data)
#define DRV_SetReg16(addr, data)    SETREG16(addr, data)
#define DRV_ClrReg16(addr, data)    CLRREG16(addr, data)

#define DRV_Reg32(addr)             INREG32(addr)
#define DRV_WriteReg32(addr, data)  OUTREG32(addr, data)
#define DRV_SetReg32(addr, data)    SETREG32(addr, data)
#define DRV_ClrReg32(addr, data)    CLRREG32(addr, data)

#define __raw_readb(REG)            DRV_Reg8(REG)
#define __raw_readw(REG)            DRV_Reg16(REG)
#define __raw_readl(REG)            DRV_Reg32(REG)
#define __raw_writeb(VAL, REG)      DRV_WriteReg8(REG,VAL)
#define __raw_writew(VAL, REG)      DRV_WriteReg16(REG,VAL)
#define __raw_writel(VAL, REG)      DRV_WriteReg32(REG,VAL)

#define printf	print

#endif /* __TYPEDEFS_H__ */

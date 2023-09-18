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

#ifndef DEVICE_APC_H
#define DEVICE_APC_H

#include "typedefs.h"

/* DEVAPC  = "DEVAPC"
 * DEVAPC0 = "DEVAPC_AO_INFRA_PERI"
 * DEVAPC1 = "DEVAPC_AO_MM"
 */

#define DEVAPC_BASE          (0x10207000U)
#define DEVAPC0_BASE         (0x1000E000U)
#define DEVAPC1_BASE         (0x1001C000U)

/*******************************************************************************
 * REGISTER ADDRESS DEFINATION
 ******************************************************************************/
#define PERM_OFF(x) (0x4 * x)

/* DEVAPC_AO_INFRA_PERI */
#define DEVAPC0_D0_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0000+PERM_OFF(n)))
#define DEVAPC0_D1_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0100+PERM_OFF(n)))
#define DEVAPC0_D2_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0200+PERM_OFF(n)))
#define DEVAPC0_D3_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0300+PERM_OFF(n)))
#define DEVAPC0_D4_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0400+PERM_OFF(n)))
#define DEVAPC0_D5_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0500+PERM_OFF(n)))
#define DEVAPC0_D6_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0600+PERM_OFF(n)))
#define DEVAPC0_D7_APC(n)     ((volatile unsigned int*)(DEVAPC0_BASE+0x0700+PERM_OFF(n)))

#define DEVAPC0_APC_CON       ((volatile unsigned int*)(DEVAPC0_BASE+0x0F00))
#define DEVAPC0_MAS_DOM_0     ((volatile unsigned int*)(DEVAPC0_BASE+0x0A00))
#define DEVAPC0_MAS_DOM_1     ((volatile unsigned int*)(DEVAPC0_BASE+0x0A04))
#define DEVAPC0_MAS_DOM_2     ((volatile unsigned int*)(DEVAPC0_BASE+0x0A08))
#define DEVAPC0_MAS_SEC_0     ((volatile unsigned int*)(DEVAPC0_BASE+0x0B00))

/* DEVAPC_AO_MM */
#define DEVAPC1_APC_CON       ((volatile unsigned int*)(DEVAPC1_BASE+0x0F00))

/* DEVAPC */
#define DEVAPC_APC_CON        ((volatile unsigned int*)(DEVAPC_BASE+0x0F00))


typedef enum {
    NS_TRANSACTION = 0,
    S_TRANSACTION,
} E_TRANSACTION;

typedef enum {
    DOMAIN_0 = 0,
    DOMAIN_1,
    DOMAIN_2,
    DOMAIN_3,
    DOMAIN_4,
    DOMAIN_5,
    DOMAIN_6,
    DOMAIN_7,
} E_DOMAIN;

typedef enum {
    NO_PROTECTION = 0,
    SEC_RW_ONLY,
    SEC_RW_NSEC_R,
    NOT_ACCESSIBLE,
} E_SLAVE_PERMISSION;

static inline unsigned int uffs(unsigned int x)
{
    unsigned int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

#define reg_read32(reg)        (*(volatile u32* const)(reg))
#define reg_write32(reg,val)   ((*(volatile u32* const)(reg)) = (val))

#define reg_set_bits(reg,bs)   ((*(volatile u32*)(reg)) |= (u32)(bs))
#define reg_clr_bits(reg,bs)   ((*(volatile u32*)(reg)) &= ~((u32)(bs)))

#define reg_set_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = reg_read32(reg); \
        tv &= ~(field); \
        tv |= ((val) << (uffs((unsigned int)field) - 1)); \
        reg_write32(reg,tv); \
    } while(0)

#define reg_get_field(reg,field,val) \
    do {    \
        volatile unsigned int tv = reg_read32(reg); \
        val = ((tv & (field)) >> (uffs((unsigned int)field) - 1)); \
    } while(0)



typedef enum {
    MASTER_NFI = 0,
    MASTER_SSUSB_XHCI = 1,
    MASTER_PWM = 3,
    MASTER_MSDC0 = 5,
    MASTER_MSDC1 = 6,
} E_MASTER;


#define MODULE_TRANSACTION(index, is_secure) (is_secure << index)
#define DAPC_SET_MASTER_TRANSACTION(devapc_register, is_secure) reg_write32(devapc_register, is_secure)

#define MODULE_DOMAIN(index, domain) (domain << (4 * (index % 8)))
#define DAPC_SET_MASTER_DOMAIN(devapc_register, domain) reg_write32(devapc_register, domain)

#define MODULE_PERMISSION(index, permission) (permission << (2 * (index % 16)))
#define DAPC_SET_SLAVE_PERMISSION(devapc_register, permission) reg_write32(devapc_register, permission)

typedef enum{
    INFRA_AO_SEJ = 10,
    INFRA_AO_DEVICE_APC_AO_INFRA_PERI = 14,
    INFRA_AO_DEVICE_APC_AO_MM = 28,
    INFRASYS_DEVICE_APC = 39,
} E_DEVAPC0_SLAVE;


void tz_dapc_sec_init(void);
void tz_dapc_sec_postinit(void);
void tz_dapc_set_master_transaction(unsigned int  master_index , E_TRANSACTION permisssion_control);
#endif

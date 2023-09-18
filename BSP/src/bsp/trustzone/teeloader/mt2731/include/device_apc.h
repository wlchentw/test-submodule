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

/* #define DEVAPC_UT */

/******************************************************************************
 * SIP CMD DEFINITION
 ******************************************************************************/
#define SIP_APC_MODULE_SET	0x1
#define SIP_APC_MM2ND_SET	0x2
#define SIP_APC_MASTER_SET	0x3

/******************************************************************************
 * FUNCTION DEFINITION
 ******************************************************************************/
void tz_apc_common_init(void);
void tz_apc_common_postinit(void);
void devapc_init(void);
int handle_sramrom_vio(uint64_t *vio_sta, uint64_t *vio_addr);
unsigned int devapc_perm_get(int, int, int);
uint64_t sip_tee_apc_request(uint32_t cmd, uint32_t x1, uint32_t x2, uint32_t x3);

/******************************************************************************
 * STRUCTURE DEFINITION
 ******************************************************************************/
enum E_TRANSACTION {
	NON_SECURE_TRANSACTION = 0,
	SECURE_TRANSACTION,
	E_TRANSACTION_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum APC_ATTR {
	E_NO_PROTECTION = 0,
	E_SEC_RW_ONLY,
	E_SEC_RW_NS_R,
	E_FORBIDDEN,
	E_APC_ATTR_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum E_MASK_DOM {
	E_DOMAIN_0 = 0,
	E_DOMAIN_1,
	E_DOMAIN_2,
	E_DOMAIN_3,
	E_DOMAIN_4,
	E_DOMAIN_5,
	E_DOMAIN_6,
	E_DOMAIN_7,
	E_DOMAIN_8,
	E_DOMAIN_9,
	E_DOMAIN_10,
	E_DOMAIN_11,
	E_DOMAIN_12,
	E_DOMAIN_13,
	E_DOMAIN_14,
	E_DOMAIN_15,
	E_MASK_DOM_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum DAPC_MASTER_TYPE {
	E_DAPC_MASTER = 0,
	E_DAPC_INFRACFG_AO_MASTER,
	E_DAPC_MASTER_TYPE_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum DAPC_SLAVE_TYPE {
	E_DAPC_INFRA_SLAVE = 0,
	E_DAPC_SRAMROM_SLAVE,
	E_DAPC_MD_SLAVE,
	E_DAPC_OTHERS_SLAVE,
	E_DAPC_SLAVE_TYPE_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

enum DAPC_PD_SLAVE_TYPE {
	E_DAPC_PD_INFRA_MM_MD_SLAVE = 0,
	E_DAPC_PD_SLAVE_TYPE_RESERVRD = 0x7FFFFFFF  /* force enum to use 32 bits */
};

struct INFRA_PERI_DEVICE_INFO {
	unsigned char       d0_permission;
	unsigned char       d1_permission;
	unsigned char       d9_permission;
};

#define DAPC_INFRA_ATTR(DEV_NAME, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3) \
{(unsigned char)PERM_ATTR1, (unsigned char)PERM_ATTR2, (unsigned char)PERM_ATTR3}

struct MD_DEVICE_INFO {
	unsigned char       d0_permission;
};

#define DAPC_MD_ATTR(DEV_NAME, PERM_ATTR1) {(unsigned char)PERM_ATTR1}

enum DEVAPC_ERR_STATUS {
	DEVAPC_OK = 0x0,

	DEVAPC_ERR_GENERIC = 0x1000,
	DEVAPC_ERR_INVALID_CMD = 0x1001,
	DEVAPC_ERR_SLAVE_TYPE_NOT_SUPPORTED = 0x1002,
	DEVAPC_ERR_SLAVE_IDX_NOT_SUPPORTED = 0x1003,
	DEVAPC_ERR_DOMAIN_NOT_SUPPORTED = 0x1004,
	DEVAPC_ERR_PERMISSION_NOT_SUPPORTED = 0x1005,
	DEVAPC_ERR_OUT_OF_BOUNDARY = 0x1006,
};

/******************************************************************************
 * UTILITY DEFINITION
 ******************************************************************************/

#define devapc_writel(VAL, REG)		__raw_writel(VAL, REG)
#define devapc_readl(REG)		__raw_readl(REG)

static void tz_set_field(volatile u32 *reg, u32 field, u32 val)
{
	u32 tv = (u32)*reg;
	tv &= ~(field);
	tv |= val;
	*reg = tv;
}

#define reg_set_field(r, f, v)	tz_set_field((volatile u32 *)r, f, v)

/******************************************************************************
 *
 * REGISTER ADDRESS DEFINITION
 *
 ******************************************************************************/
#define DEVAPC_AO_INFRA_BASE        0x1001C000
#define DEVAPC_PD_INFRA_BASE        0x10207000

#define SRAMROM_BASE                0x10214000
#define INFRACFG_AO_BASE            0x10001000
#define SECURITY_AO_BASE            0x1001A000

/* #define BLOCKED_REG_BASE            0x10400000 */

/*******************************************************************************************/
/* Device APC AO */
#define DEVAPC_SYS0_D0_APC_0           ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0000))
#define DEVAPC_SYS1_D0_APC_0           ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x1000))
#define DEVAPC_SYS2_D0_APC_0           ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x2000))

#define DEVAPC_INFRA_MAS_DOM_0         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0900))
#define DEVAPC_INFRA_MAS_DOM_1         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0904))
#define DEVAPC_INFRA_MAS_DOM_2         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0908))
#define DEVAPC_INFRA_MAS_DOM_3         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x090C))
#define DEVAPC_INFRA_MAS_DOM_4         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0910))

#define DEVAPC_INFRA_MAS_SEC_0         ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0A00))

#define DEVAPC_INFRA_APC_CON           ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0F00))

#define DEVAPC_SRAMROM_DOM_REMAP_0_0   ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0800))
#define DEVAPC_SRAMROM_DOM_REMAP_0_1   ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0804))
#define DEVAPC_SRAMROM_DOM_REMAP_1_0   ((volatile unsigned int *)(DEVAPC_AO_INFRA_BASE+0x0810))

/* MD is combined into DEVAPC_AO SYS2 */

/*******************************************************************************************/
/* Device APC PD */
#define DEVAPC_PD_INFRA_VIO_MASK(index) \
	((uintptr_t)(DEVAPC_PD_INFRA_BASE + 0x4 * index))

#define DEVAPC_PD_INFRA_VIO_STA(index) \
	((uintptr_t)(DEVAPC_PD_INFRA_BASE + 0x400 + 0x4 * index))

#define DEVAPC_PD_INFRA_VIO_DBG0       ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0900))
#define DEVAPC_PD_INFRA_VIO_DBG1       ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0904))
#define DEVAPC_PD_INFRA_VIO_DBG2       ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0908))

#define DEVAPC_PD_INFRA_APC_CON        ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0F00))

#define DEVAPC_PD_INFRA_VIO_SHIFT_STA  ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0F10))
#define DEVAPC_PD_INFRA_VIO_SHIFT_SEL  ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0F14))
#define DEVAPC_PD_INFRA_VIO_SHIFT_CON  ((volatile unsigned int *)(DEVAPC_PD_INFRA_BASE+0x0F20))

/*******************************************************************************************/

#define INFRA_AO_SEC_CON		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0F80))

/* INFRACFG AO */
#define INFRA_AO_SEC_CG_CON0		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0F84))
#define INFRA_AO_SEC_CG_CON1		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0F88))
#define INFRA_AO_SEC_CG_CON2		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0F9C))
#define INFRA_AO_SEC_CG_CON3		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0FA4))

#define INFRACFG_AO_DEVAPC_CON		((volatile unsigned int *)(INFRACFG_AO_BASE+0x0710))
#define INFRACFG_AO_DEVAPC_MAS_DOM	((volatile unsigned int *)(INFRACFG_AO_BASE+0x0714))
#define INFRACFG_AO_DEVAPC_MAS_SEC	((volatile unsigned int *)(INFRACFG_AO_BASE+0x0718))

/* PMS(MD devapc) */
/* #define AP2MD1_PMS_CTRL_EN             ((unsigned int *)0x100018AC) */
/* #define AP2MD1_PMS_CTRL_EN_LOCK        ((unsigned int *)0x100018A8) */

/*******************************************************************************************/

#define SRAMROM_SEC_VIO_STA            ((volatile unsigned int *)(SRAMROM_BASE+0x010))
#define SRAMROM_SEC_VIO_ADDR           ((volatile unsigned int *)(SRAMROM_BASE+0x014))
#define SRAMROM_SEC_VIO_CLR            ((volatile unsigned int *)(SRAMROM_BASE+0x018))

#define SRAMROM_ROM_SEC_VIO_STA        ((volatile unsigned int *)(SRAMROM_BASE+0x110))
#define SRAMROM_ROM_SEC_VIO_ADDR       ((volatile unsigned int *)(SRAMROM_BASE+0x114))
#define SRAMROM_ROM_SEC_VIO_CLR        ((volatile unsigned int *)(SRAMROM_BASE+0x118))


#define SRAMROM_SEC_CTRL               ((volatile unsigned int *)(SECURITY_AO_BASE+0x010))
#define SRAMROM_SEC_CTRL2              ((volatile unsigned int *)(SECURITY_AO_BASE+0x018))
#define SRAMROM_SEC_CTRL5              ((volatile unsigned int *)(SECURITY_AO_BASE+0x024))
#define SRAMROM_SEC_CTRL6              ((volatile unsigned int *)(SECURITY_AO_BASE+0x028))
#define SRAMROM_SEC_ADDR               ((volatile unsigned int *)(SECURITY_AO_BASE+0x050))
#define SRAMROM_SEC_ADDR1              ((volatile unsigned int *)(SECURITY_AO_BASE+0x054))
#define SRAMROM_SEC_ADDR2              ((volatile unsigned int *)(SECURITY_AO_BASE+0x058))

#define SRAMROM_SEC_ADDR_SEC0_SEC_EN       (28)
#define SRAMROM_SEC_ADDR_SEC1_SEC_EN       (29)
#define SRAMROM_SEC_ADDR_SEC2_SEC_EN       (30)
#define SRAMROM_SEC_ADDR_SEC3_SEC_EN       (31)

/* SEC means region (0~3) */
#define SRAMROM_SEC_CTRL_SEC0_DOM0_SHIFT   (0)
#define SRAMROM_SEC_CTRL_SEC0_DOM1_SHIFT   (3)
#define SRAMROM_SEC_CTRL_SEC0_DOM2_SHIFT   (6)
#define SRAMROM_SEC_CTRL_SEC0_DOM3_SHIFT   (9)
#define SRAMROM_SEC_CTRL_SEC1_DOM0_SHIFT   (16)
#define SRAMROM_SEC_CTRL_SEC1_DOM1_SHIFT   (19)
#define SRAMROM_SEC_CTRL_SEC1_DOM2_SHIFT   (22)
#define SRAMROM_SEC_CTRL_SEC1_DOM3_SHIFT   (25)

#define SRAMROM_SEC_CTRL2_SEC0_DOM4_SHIFT  (0)
#define SRAMROM_SEC_CTRL2_SEC0_DOM5_SHIFT  (3)
#define SRAMROM_SEC_CTRL2_SEC0_DOM6_SHIFT  (6)
#define SRAMROM_SEC_CTRL2_SEC0_DOM7_SHIFT  (9)
#define SRAMROM_SEC_CTRL2_SEC1_DOM4_SHIFT  (16)
#define SRAMROM_SEC_CTRL2_SEC1_DOM5_SHIFT  (19)
#define SRAMROM_SEC_CTRL2_SEC1_DOM6_SHIFT  (22)
#define SRAMROM_SEC_CTRL2_SEC1_DOM7_SHIFT  (25)

#define SRAMROM_SEC_CTRL5_SEC2_DOM0_SHIFT  (0)
#define SRAMROM_SEC_CTRL5_SEC2_DOM1_SHIFT  (3)
#define SRAMROM_SEC_CTRL5_SEC2_DOM2_SHIFT  (6)
#define SRAMROM_SEC_CTRL5_SEC2_DOM3_SHIFT  (9)
#define SRAMROM_SEC_CTRL5_SEC3_DOM0_SHIFT  (16)
#define SRAMROM_SEC_CTRL5_SEC3_DOM1_SHIFT  (19)
#define SRAMROM_SEC_CTRL5_SEC3_DOM2_SHIFT  (22)
#define SRAMROM_SEC_CTRL5_SEC3_DOM3_SHIFT  (25)

#define SRAMROM_SEC_CTRL6_SEC2_DOM4_SHIFT  (0)
#define SRAMROM_SEC_CTRL6_SEC2_DOM5_SHIFT  (3)
#define SRAMROM_SEC_CTRL6_SEC2_DOM6_SHIFT  (6)
#define SRAMROM_SEC_CTRL6_SEC2_DOM7_SHIFT  (9)
#define SRAMROM_SEC_CTRL6_SEC3_DOM4_SHIFT  (16)
#define SRAMROM_SEC_CTRL6_SEC3_DOM5_SHIFT  (19)
#define SRAMROM_SEC_CTRL6_SEC3_DOM6_SHIFT  (22)
#define SRAMROM_SEC_CTRL6_SEC3_DOM7_SHIFT  (25)


#define SRAMROM_SEC_CTRL_SEC0_DOM0_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC0_DOM0_SHIFT)
#define SRAMROM_SEC_CTRL_SEC0_DOM1_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC0_DOM1_SHIFT)
#define SRAMROM_SEC_CTRL_SEC0_DOM2_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC0_DOM2_SHIFT)
#define SRAMROM_SEC_CTRL_SEC0_DOM3_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC0_DOM3_SHIFT)
#define SRAMROM_SEC_CTRL_SEC1_DOM0_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC1_DOM0_SHIFT)
#define SRAMROM_SEC_CTRL_SEC1_DOM1_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC1_DOM1_SHIFT)
#define SRAMROM_SEC_CTRL_SEC1_DOM2_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC1_DOM2_SHIFT)
#define SRAMROM_SEC_CTRL_SEC1_DOM3_MASK   (0x7 << SRAMROM_SEC_CTRL_SEC1_DOM3_SHIFT)

#define SRAMROM_SEC_CTRL2_SEC0_DOM4_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC0_DOM4_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC0_DOM5_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC0_DOM5_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC0_DOM6_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC0_DOM6_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC0_DOM7_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC0_DOM7_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC1_DOM4_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC1_DOM4_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC1_DOM5_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC1_DOM5_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC1_DOM6_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC1_DOM6_SHIFT)
#define SRAMROM_SEC_CTRL2_SEC1_DOM7_MASK  (0x7 << SRAMROM_SEC_CTRL2_SEC1_DOM7_SHIFT)

#define SRAMROM_SEC_CTRL5_SEC2_DOM0_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC2_DOM0_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC2_DOM1_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC2_DOM1_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC2_DOM2_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC2_DOM2_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC2_DOM3_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC2_DOM3_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC3_DOM0_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC3_DOM0_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC3_DOM1_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC3_DOM1_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC3_DOM2_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC3_DOM2_SHIFT)
#define SRAMROM_SEC_CTRL5_SEC3_DOM3_MASK  (0x7 << SRAMROM_SEC_CTRL5_SEC3_DOM3_SHIFT)

#define SRAMROM_SEC_CTRL6_SEC2_DOM4_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC2_DOM4_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC2_DOM5_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC2_DOM5_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC2_DOM6_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC2_DOM6_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC2_DOM7_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC2_DOM7_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC3_DOM4_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC3_DOM4_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC3_DOM5_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC3_DOM5_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC3_DOM6_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC3_DOM6_SHIFT)
#define SRAMROM_SEC_CTRL6_SEC3_DOM7_MASK  (0x7 << SRAMROM_SEC_CTRL6_SEC3_DOM7_SHIFT)

#define PERMIT_S_RW_NS_RW       (0x0)
#define PERMIT_S_RW_NS_BLOCK    (0x1)
#define PERMIT_S_RW_NS_RO       (0x2)
#define PERMIT_S_RW_NS_WO       (0x3)
#define PERMIT_S_RO_NS_RO       (0x4)
#define PERMIT_S_BLOCK_NS_BLOCK (0x7)


/* Set the region 0 size of the on-chip SRAM and the region 1 size will be (192KB - size_of_region_0) */
#define TZ_SRAMROM_SET_REGION_0_SIZE_KB(size)	(devapc_writel(((size & 0xff) << 10), SRAMROM_SEC_ADDR))

/******************************************************************************
 * Variable DEFINITION
 ******************************************************************************/
/* If you config register INFRA_AO_SEC_CON(address 0x10000F80) bit[4] = 1,
 * the domain comes from device_apc. By default this register is 0,
 * the domain comes form MD1
 */
#define FORCE_MD1_SIGNAL_FROM_DAPC      ((0x1) << 4)

/* PROTECT BIT FOR INFRACFG AO */
#define SEJ_CG_PROTECT_BIT              ((0x1) << 5)
#define TRNG_CG_PROTECT_BIT             ((0x1) << 9)
#define DEVAPC_CG_PROTECT_BIT           ((0x1) << 20)

#define SRAM_SEC_VIO_BIT                (0x1)
#define ROM_SEC_VIO_BIT                 (0x1)

/*******************************************************************************************/
/* Master domain/secure bit definition */
#define MASTER_SPM_DOM_INDEX		(18)
#define MASTER_SPM_SEC_INDEX		(19)
#define MASTER_INFRA_MAX_INDEX		(19)

/* Below master should be set in INFRACFG_AO */
#define MASTER_INFRACFG_AO_MAX_INDEX	5
#define MASTER_APMCU_INDEX		0
#define MASTER_MD_INDEX			1
#define MASTER_HSM_INDEX		2
#define MASTER_USB_INDEX		3
#define MASTER_SSUSB_INDEX		4
#define MASTER_MSDC0_INDEX		5

/*******************************************************************************************/
/* Master domain remap */
#define MASTER_DOM_RMP_INIT		(0xFFFFFFFF)
#define SRAMROM_RMP_AP			(0x7 << 0)	// Infra domain 0

#define MD_RMP_AP			(0x3 << 0)	// Infra domain 0

/*******************************************************************************************/
#define MOD_NO_IN_1_DEVAPC              16
#define MAS_DOM_NO_IN_1_DEVAPC		4

/* infra/sramrom/MD support maximum domain num */
#define DEVAPC_INFRA_DOM_MAX		16
#define DEVAPC_SRAMROM_DOM_MAX		8
#define DEVAPC_MD_DOM_MAX		4

/* infra/sramrom/MD APC number per domain */
#define DEVAPC_INFRA_APC_NUM		10
#define DEVAPC_SRAMROM_APC_NUM		1
#define DEVAPC_MD_APC_NUM		3

/* infra/sramrom/MD support maximum ctrl index */
#define SLAVE_INFRA_MAX_INDEX		146
#define SLAVE_SRAMROM_MAX_INDEX		0
#define SLAVE_MD_MAX_INDEX		35

#define VIO_MASK_STA_NUM		13
#define SRAMROM_VIO_INDEX		355
#define DEVAPC_CTRL_SRAMROM_INDEX	0
/* devapc can only handle vio index 0 ~ sramrom */
#define VIOLATION_MAX_INDEX		SRAMROM_VIO_INDEX
#define VIOLATION_TRIGGERED		1

#endif /* DEVICE_APC_H */

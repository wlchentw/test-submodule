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

#include "print.h"
#include "typedefs.h"
#include "tz_init.h"
#include "tz_emi_mpu.h"
#include "tz_emi_reg.h"

#define MOD "[TZ_EMI_MPU]"

#define readl(addr) (__raw_readl(addr))
#define writel(b,addr) __raw_writel(b,addr)
#define IOMEM(reg) (reg)

#define TEE_DEBUG
#ifdef TEE_DEBUG
#define DBG_MSG(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#else
#define DBG_MSG(str, ...) do {} while(0)
#define DBG_INFO(str, ...) do {print(str, ##__VA_ARGS__);} while(0)
#endif


/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned long start, unsigned long end, int region, unsigned int access_permission)
{
    int ret = 0;

    if (end <= start)
    {
        DBG_MSG("%s, Invalid address! End address should larger than start address.\n", MOD);
        return -1;
    }


    if((end >> 31) && !(start >> 31))
    {
        DBG_MSG("%s, Invalid address! MPU region should not across 32bit. Please divide the memory into two regions.\n", MOD);
        return -1;
    }

    if ((readl(PERIAXI_BUS_CTL3) & PERISYS_4G_SUPPORT) == 0)
    {
        start = start - EMI_PHY_OFFSET;
        end = end - EMI_PHY_OFFSET;
        DBG_MSG("%s, MPU 2GB mode.\n", MOD);
    }
    else
        DBG_MSG("%s, MPU 4GB mode.\n", MOD);

    /*Address 64KB alignment*/
    start = start >> 16;
    end = end >> 16;

    switch (region) {
    case 0:
        writel(0, EMI_MPU_APC0);
        writel(start, EMI_MPU_SA0);
        writel(end, EMI_MPU_EA0);
        writel(access_permission, EMI_MPU_APC0);
        break;

    case 1:
        writel(0, EMI_MPU_APC1);
        writel(start, EMI_MPU_SA1);
        writel(end, EMI_MPU_EA1);
        writel(access_permission, EMI_MPU_APC1);
        break;

    case 2:
        writel(0, EMI_MPU_APC2);
        writel(start, EMI_MPU_SA2);
        writel(end, EMI_MPU_EA2);
        writel(access_permission, EMI_MPU_APC2);
        break;

    case 3:
        writel(0, EMI_MPU_APC3);
        writel(start, EMI_MPU_SA3);
        writel(end, EMI_MPU_EA3);
        writel(access_permission, EMI_MPU_APC3);
        break;

    case 4:
        writel(0, EMI_MPU_APC4);
        writel(start, EMI_MPU_SA4);
        writel(end, EMI_MPU_EA4);
        writel(access_permission, EMI_MPU_APC4);
        break;

    case 5:
        writel(0, EMI_MPU_APC5);
        writel(start, EMI_MPU_SA5);
        writel(end, EMI_MPU_EA5);
        writel(access_permission, EMI_MPU_APC5);
        break;

    case 6:
        writel(0, EMI_MPU_APC6);
        writel(start, EMI_MPU_SA6);
        writel(end, EMI_MPU_EA6);
        writel(access_permission, EMI_MPU_APC6);
        break;

    case 7:
        writel(0, EMI_MPU_APC7);
        writel(start, EMI_MPU_SA7);
        writel(end, EMI_MPU_EA7);
        writel(access_permission, EMI_MPU_APC7);
        break;

    default:
        ret = -1;
        break;
    }

    return ret;
}

/* sample code for scenario as below: */
/* mpu region2: 0x40000000 - 0xe0000000 (2.5GB) is secure RW and non-secure RW for domain 0, 3.*/
/* mpu region3: 0xe0000000 - 0xe0080000 (512K) is secure RW and non-secure RW for domain 0, 2, 3.*/
/* mpu region4: 0xe0080000 - 0xe0880000 (8M) is secure and non-secure RW for domain 2, 3.*/
/* mpu region5: 0xe0880000 - 0xec880000 (384M) is secure RW and non-secure RW for domain 1, 2, 3.*/
/* mpu region6: 0xec880000 - 0xec8e0000 (384K) is secure RW and non-secure RW for domain 0, 1, 3.*/
/* mpu region7: 0xec8e0000 - 0xf48e0000 (128M) is secure RW for domain 0, 3.*/

void tz_emi_mpu_init_2(void)
{
    int ret = 0;
    unsigned int sec_mem_mpu_attr;

    /*region2 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_RW, \
        TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_RW);
    ret += emi_mpu_set_region_protection(0x40000000,               /*START_ADDR*/
                                            0xe0000000,           /*END_ADDR*/
                                            2,                    /*region*/
                                            sec_mem_mpu_attr);

    /*region3 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_RW, \
        TZ_MPU_SEC_RW_NSEC_RW, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_RW);
    ret += emi_mpu_set_region_protection(0xe0000000,               /*START_ADDR*/
                                            0xe0080000,           /*END_ADDR*/
                                            3,                    /*region*/
                                            sec_mem_mpu_attr);

    /*region4 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_RW, \
        TZ_MPU_SEC_RW_NSEC_RW, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_DENY_NSEC_DENY);
    ret += emi_mpu_set_region_protection(0xe0080000,               /*START_ADDR*/
                                            0xe0880000,           /*END_ADDR*/
                                            4,                    /*region*/
                                            sec_mem_mpu_attr);

    /*region5 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_RW, \
        TZ_MPU_SEC_RW_NSEC_RW, TZ_MPU_SEC_RW_NSEC_RW, TZ_MPU_SEC_DENY_NSEC_DENY);
    ret += emi_mpu_set_region_protection(0xe0880000,               /*START_ADDR*/
                                            0xec880000,           /*END_ADDR*/
                                            5,                    /*region*/
                                            sec_mem_mpu_attr);

    /*region6 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_RW, \
        TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_RW, TZ_MPU_SEC_RW_NSEC_RW);
    ret += emi_mpu_set_region_protection(0xec880000,               /*START_ADDR*/
                                            0xec8e0000,           /*END_ADDR*/
                                            6,                    /*region*/
                                            sec_mem_mpu_attr);

    /*region7 mpu*/
    sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_DENY, \
        TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_DENY);
    ret += emi_mpu_set_region_protection(0xec8e0000,               /*START_ADDR*/
                                            0xf48e0000,           /*END_ADDR*/
                                            7,                    /*region*/
                                            sec_mem_mpu_attr);

    if(ret)
        DBG_MSG("%s MPU error!!\n", MOD);

}

void tz_emi_mpu_init(u32 start_add, u32 end_addr, u32 mpu_region)
{
    int ret = 0;
    unsigned int sec_mem_mpu_attr;
    unsigned int sec_mem_phy_start, sec_mem_phy_end;

    /* Caculate start/end address */
    sec_mem_phy_start = start_add;
    sec_mem_phy_end = end_addr;

    switch(mpu_region)
    {
        case SECURE_OS_MPU_REGION_ID:
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_DENY, \
                TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_DENY);
            break;

        case ATF_MPU_REGION_ID:
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(TZ_MPU_SEC_RW_NSEC_DENY, \
                TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_DENY_NSEC_DENY, TZ_MPU_SEC_RW_NSEC_DENY);
            break;

        default:
            DBG_MSG("%s Warning - MPU region '%d' is not supported for preloader!\n", MOD, mpu_region);
            return;
    }

    DBG_MSG("%s MPU [0x%x-0x%x]\n", MOD, sec_mem_phy_start, sec_mem_phy_end);

    ret = emi_mpu_set_region_protection(sec_mem_phy_start,  /*START_ADDR*/
                                        sec_mem_phy_end,    /*END_ADDR*/
                                        mpu_region,         /*region*/
                                        sec_mem_mpu_attr);

    if(ret)
    {
        DBG_MSG("%s MPU error!!\n", MOD);
    }
}

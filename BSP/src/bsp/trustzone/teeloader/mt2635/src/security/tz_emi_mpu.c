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
 * MediaTek Inc. (C) 2016. All rights reserved.
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

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @region: EMI MPU region id
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
int emi_mpu_set_region_protection(unsigned int start, unsigned int end, int region, unsigned int access_permission)
{
    int ret = 0;
    unsigned int tmp, tmp2;
    unsigned int ax_pm, ax_pm2;

    if((end != 0) || (start !=0))
    {
        /*Address 64KB alignment*/
        start -= EMI_PHY_OFFSET;
        end -= EMI_PHY_OFFSET;
        start = start >> 16;
        end = end >> 16;

        if (end <= start)
        {
            return -1;
        }
    }

    ax_pm  = (access_permission << 16) >> 16;
    ax_pm2 = (access_permission >> 16);

    switch (region) {
    case 0:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUI)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUI_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUI);
        writel(0, EMI_MPUI_2ND);
        writel((start << 16) | end, EMI_MPUA);
        writel(tmp2 | ax_pm2, EMI_MPUI_2ND);
        writel(tmp | ax_pm, EMI_MPUI);
        break;

    case 1:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUI)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUI_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUI);
        writel(0, EMI_MPUI_2ND);
        writel((start << 16) | end, EMI_MPUB);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUI_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUI);
        break;

    case 2:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUJ)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUJ_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUJ);
        writel(0, EMI_MPUJ_2ND);
        writel((start << 16) | end, EMI_MPUC); 
        writel(tmp2 | ax_pm2, EMI_MPUJ_2ND);
        writel(tmp | ax_pm, EMI_MPUJ);
        break; 

    case 3:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUJ)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUJ_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUJ);
        writel(0, EMI_MPUJ_2ND);
        writel((start << 16) | end, EMI_MPUD);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUJ_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUJ);
        break;

    case 4:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUK)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUK_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUK);
        writel(0, EMI_MPUK_2ND);
        writel((start << 16) | end, EMI_MPUE);
        writel(tmp2 | ax_pm2, EMI_MPUK_2ND);
        writel(tmp | ax_pm, EMI_MPUK);
        break;

    case 5:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUK)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUK_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUK);
        writel(0, EMI_MPUK_2ND);
        writel((start << 16) | end, EMI_MPUF);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUK_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUK);
        break;  

    case 6:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUL)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUL_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUL);
        writel(0, EMI_MPUL_2ND);
        writel((start << 16) | end, EMI_MPUG);
        writel(tmp2 | ax_pm2, EMI_MPUL_2ND);
        writel(tmp | ax_pm, EMI_MPUL);
        break;

    case 7:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUL)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUL_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUL);
        writel(0, EMI_MPUL_2ND);
        writel((start << 16) | end, EMI_MPUH);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUL_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUL);
        break;

#if defined(MACH_TYPE_MT6735) || defined(MACH_TYPE_MT6753) || defined(MACH_TYPE_MT6737T)
    case 8:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUI2)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUI2_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUI2);
        writel(0, EMI_MPUI2_2ND);
        writel((start << 16) | end, EMI_MPUA2);
        writel(tmp2 | ax_pm2, EMI_MPUI2_2ND);
        writel(tmp | ax_pm, EMI_MPUI2);
        break;

    case 9:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUI2)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUI2_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUI2);
        writel(0, EMI_MPUI2_2ND);
        writel((start << 16) | end, EMI_MPUB2);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUI2_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUI2);
        break;

    case 10:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUJ2)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUJ2_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUJ2);
        writel(0, EMI_MPUJ2_2ND);
        writel((start << 16) | end, EMI_MPUC2);
        writel(tmp2 | ax_pm2, EMI_MPUJ2_2ND);
        writel(tmp | ax_pm, EMI_MPUJ2);
        break;

    case 11:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUJ2)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUJ2_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUJ2);
        writel(0, EMI_MPUJ2_2ND);
        writel((start << 16) | end, EMI_MPUD2);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUJ2_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUJ2);
        break;

    case 12:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUK2)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUK2_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUK2);
        writel(0, EMI_MPUK2_2ND);
        writel((start << 16) | end, EMI_MPUE2);
        writel(tmp2 | ax_pm2, EMI_MPUK2_2ND);
        writel(tmp | ax_pm, EMI_MPUK2);
        break;

    case 13:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUK2)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUK2_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUK2);
        writel(0, EMI_MPUK2_2ND);
        writel((start << 16) | end, EMI_MPUF2);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUK2_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUK2);
        break;

    case 14:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUL2)) & 0xFFFF0000;
        tmp2 = readl(IOMEM(EMI_MPUL2_2ND)) & 0xFFFF0000;
        writel(0, EMI_MPUL2);
        writel(0, EMI_MPUL2_2ND);
        writel((start << 16) | end, EMI_MPUG2);
        writel(tmp2 | ax_pm2, EMI_MPUL2_2ND);
        writel(tmp | ax_pm, EMI_MPUL2);
        break;

    case 15:
        //Marcos: Clear access right before setting MPU address (Mt6582 design)
        tmp = readl(IOMEM(EMI_MPUL2)) & 0x0000FFFF;
        tmp2 = readl(IOMEM(EMI_MPUL2_2ND)) & 0x0000FFFF;
        writel(0, EMI_MPUL2);
        writel(0, EMI_MPUL2_2ND);
        writel((start << 16) | end, EMI_MPUH2);
        writel(tmp2 | (ax_pm2 << 16), EMI_MPUL2_2ND);
        writel(tmp | (ax_pm << 16), EMI_MPUL2);
        break;
#endif

    default:
        ret = -1;
        break;
    }

    return ret;
}

void tz_emi_mpu_init(u32 start_add, u32 end_addr, u32 mpu_region)
{
    int ret = 0;
    unsigned int sec_mem_mpu_attr;
    unsigned int sec_mem_phy_start, sec_mem_phy_end;

    /* Caculate start/end address */
    sec_mem_phy_start = start_add;
    sec_mem_phy_end = end_addr;

    // For MT6589
    //==================================================================================================================
    //            | Region |  D0(AP)  |  D1(MD0)  |  D2(Conn) |  D3(MD32) |  D4(MM)  |  D5(MD1)  |  D6(MFG)  |  D7(N/A)
    //------------+---------------------------------------------------------------------------------------------------
    // Secure OS  |    0   |RW(S)     |Forbidden  |Forbidden  |Forbidden  |RW(S)     |Forbidden  |Forbidden  |Forbidden
    //------------+---------------------------------------------------------------------------------------------------
    // MD0 ROM    |    1   |RO(S/NS)  |RO(S/NS)   |Forbidden  |Forbidden
    //------------+------------------------------------------------------
    // MD0 R/W+   |    2   |Forbidden |No protect |Forbidden  |Forbidden
    //------------+------------------------------------------------------
    // MD1 ROM    |    3   |RO(S/NS)  |Forbidden  |RO(S/NS)   |Forbidden
    //------------+------------------------------------------------------
    // MD1 R/W+   |    4   |Forbidden |Forbidden  |No protect |Forbidden
    //------------+------------------------------------------------------
    // MD0 Share  |    5   |No protect|No protect |Forbidden  |Forbidden
    //------------+------------------------------------------------------
    // MD1 Share  |    6   |No protect|Forbidden  |No protect |Forbidden
    //------------+------------------------------------------------------
    // AP         |    7   |No protect|Forbidden  |Forbidden  |No protect
    //===================================================================

    switch(mpu_region)
    {
        case SECURE_OS_MPU_REGION_ID:
        #if defined(MACH_TYPE_MT6735) || defined(MACH_TYPE_MT6753) || defined(MACH_TYPE_MT6737T)
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW);
		#else  //MT6735M
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(LOCK, SEC_RW, FORBIDDEN, FORBIDDEN, SEC_RW);
	#endif
            break;

        case ATF_MPU_REGION_ID:
        #if defined(MACH_TYPE_MT6735) || defined(MACH_TYPE_MT6753) || defined(MACH_TYPE_MT6737T)
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW);
		#else  //MT6735M
            sec_mem_mpu_attr = SET_ACCESS_PERMISSON(LOCK, FORBIDDEN, FORBIDDEN, FORBIDDEN, SEC_RW);
	#endif
            break;

        default:
            print("%s Warning - MPU region '%d' is not supported in pre-loader!\n", MOD, mpu_region);
            return;
    }

    print("%s MPU [0x%x-0x%x]\n", MOD, sec_mem_phy_start, sec_mem_phy_end);

    ret = emi_mpu_set_region_protection(sec_mem_phy_start,  /*START_ADDR*/
                                        sec_mem_phy_end,    /*END_ADDR*/
                                        mpu_region,         /*region*/
                                        sec_mem_mpu_attr);

    if(ret)
    {
        print("%s MPU error!!\n", MOD);
    }
}

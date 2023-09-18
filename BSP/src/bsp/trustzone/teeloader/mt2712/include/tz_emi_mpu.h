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
 * MediaTek Inc. (C) 2017 All rights reserved.
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

#ifndef TZ_EMI_MPU_H
#define TZ_EMI_MPU_H

#define EMI_PHY_OFFSET      (0x40000000UL)
#define EMI_MPU_ALIGNMENT   (0x10000UL)
#define PERIAXI_BUS_CTL3    (0x10003208UL)
#define PERISYS_4G_SUPPORT  (0x1 << 11)


typedef enum
{
    TZ_MPU_SEC_RW_NSEC_RW = 0,      /* read and write for both secure and non-secure access */
    TZ_MPU_SEC_RW_NSEC_DENY = 1,    /* read and write for secure access */
    TZ_MPU_SEC_RW_NSEC_RO = 2,      /* read and write for secure access and read only for non-secure access */
    TZ_MPU_SEC_RW_NSEC_WO = 3,      /* read and write for secure access and write only for non-secure access */
    TZ_MPU_SEC_RO_NSEC_RO = 4,      /* read only for both secure access and non-secure access */
    TZ_MPU_SEC_DENY_NSEC_DENY = 5,  /* Any access is prohibited */
    TZ_MPU_SEC_RO_NSEC_RW = 6       /* read and write for non-secure access and read only for secure access */
} tz_mpu_permission;

#define SECURE_OS_MPU_REGION_ID    (0)
#define ATF_MPU_REGION_ID          (1)

/*SET_ACCESS_PERMISSON is used to merge domain permission into one setting*/
#define SET_ACCESS_PERMISSON(d3, d2, d1, d0) (((d3) << 9) | ((d2) << 6) | ((d1) << 3) | (d0))


#endif /* TZ_EMI_MPU_H */

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

#ifndef TZ_EMI_REG_H
#define TZ_EMI_REG_H

#define EMI_MPU_BASE                (0x10226000)

#define EMI_MPU_SA0                 ((P_U32)(EMI_MPU_BASE+0x100))  /* EMI MPU start addr 0 */
#define EMI_MPU_SA1                 ((P_U32)(EMI_MPU_BASE+0x104))  /* EMI MPU start addr 1 */
#define EMI_MPU_SA2                 ((P_U32)(EMI_MPU_BASE+0x108))  /* EMI MPU start addr 2 */
#define EMI_MPU_SA3                 ((P_U32)(EMI_MPU_BASE+0x10C))  /* EMI MPU start addr 3 */
#define EMI_MPU_SA4                 ((P_U32)(EMI_MPU_BASE+0x110))  /* EMI MPU start addr 4 */
#define EMI_MPU_SA5                 ((P_U32)(EMI_MPU_BASE+0x114))  /* EMI MPU start addr 5 */
#define EMI_MPU_SA6                 ((P_U32)(EMI_MPU_BASE+0x118))  /* EMI MPU start addr 6 */
#define EMI_MPU_SA7                 ((P_U32)(EMI_MPU_BASE+0x11C))  /* EMI MPU start addr 7 */

#define EMI_MPU_EA0                 ((P_U32)(EMI_MPU_BASE+0x200))  /* EMI MPU end addr 0 */
#define EMI_MPU_EA1                 ((P_U32)(EMI_MPU_BASE+0x204))  /* EMI MPU end addr 1 */
#define EMI_MPU_EA2                 ((P_U32)(EMI_MPU_BASE+0x208))  /* EMI MPU end addr 2 */
#define EMI_MPU_EA3                 ((P_U32)(EMI_MPU_BASE+0x20C))  /* EMI MPU end addr 3 */
#define EMI_MPU_EA4                 ((P_U32)(EMI_MPU_BASE+0x210))  /* EMI MPU end addr 4 */
#define EMI_MPU_EA5                 ((P_U32)(EMI_MPU_BASE+0x214))  /* EMI MPU end addr 5 */
#define EMI_MPU_EA6                 ((P_U32)(EMI_MPU_BASE+0x218))  /* EMI MPU end addr 6 */
#define EMI_MPU_EA7                 ((P_U32)(EMI_MPU_BASE+0x21C))  /* EMI MPU end addr 7 */

#define EMI_MPU_APC0                ((P_U32)(EMI_MPU_BASE+0x300))  /* EMI MPU APC 0 */
#define EMI_MPU_APC1                ((P_U32)(EMI_MPU_BASE+0x304))  /* EMI MPU APC 1 */
#define EMI_MPU_APC2                ((P_U32)(EMI_MPU_BASE+0x308))  /* EMI MPU APC 2 */
#define EMI_MPU_APC3                ((P_U32)(EMI_MPU_BASE+0x30C))  /* EMI MPU APC 3 */
#define EMI_MPU_APC4                ((P_U32)(EMI_MPU_BASE+0x310))  /* EMI MPU APC 4 */
#define EMI_MPU_APC5                ((P_U32)(EMI_MPU_BASE+0x314))  /* EMI MPU APC 5 */
#define EMI_MPU_APC6                ((P_U32)(EMI_MPU_BASE+0x318))  /* EMI MPU APC 6 */
#define EMI_MPU_APC7                ((P_U32)(EMI_MPU_BASE+0x31C))  /* EMI MPU APC 7 */

#define EMI_MPU_CTRL_D0             ((P_U32)(EMI_MPU_BASE+0x800))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D1             ((P_U32)(EMI_MPU_BASE+0x804))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D2             ((P_U32)(EMI_MPU_BASE+0x808))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D3             ((P_U32)(EMI_MPU_BASE+0x80C))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D4             ((P_U32)(EMI_MPU_BASE+0x810))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D5             ((P_U32)(EMI_MPU_BASE+0x814))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D6             ((P_U32)(EMI_MPU_BASE+0x818))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D7             ((P_U32)(EMI_MPU_BASE+0x81C))  /* EMI MPU DOMAIN CTRL 0 */

#define EMI_MPU_CTRL_D0             ((P_U32)(EMI_MPU_BASE+0x800))  /* EMI MPU DOMAIN CTRL 0 */
#define EMI_MPU_CTRL_D1             ((P_U32)(EMI_MPU_BASE+0x804))  /* EMI MPU DOMAIN CTRL 1 */
#define EMI_MPU_CTRL_D2             ((P_U32)(EMI_MPU_BASE+0x808))  /* EMI MPU DOMAIN CTRL 2 */
#define EMI_MPU_CTRL_D3             ((P_U32)(EMI_MPU_BASE+0x80C))  /* EMI MPU DOMAIN CTRL 3 */
#define EMI_MPU_CTRL_D4             ((P_U32)(EMI_MPU_BASE+0x810))  /* EMI MPU DOMAIN CTRL 4 */
#define EMI_MPU_CTRL_D5             ((P_U32)(EMI_MPU_BASE+0x814))  /* EMI MPU DOMAIN CTRL 5 */
#define EMI_MPU_CTRL_D6             ((P_U32)(EMI_MPU_BASE+0x818))  /* EMI MPU DOMAIN CTRL 6 */
#define EMI_MPU_CTRL_D7             ((P_U32)(EMI_MPU_BASE+0x81C))  /* EMI MPU DOMAIN CTRL 7 */

#define EMI_MPU_MASK_D0             ((P_U32)(EMI_MPU_BASE+0x900))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D1             ((P_U32)(EMI_MPU_BASE+0x904))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D2             ((P_U32)(EMI_MPU_BASE+0x908))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D3             ((P_U32)(EMI_MPU_BASE+0x90C))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D4             ((P_U32)(EMI_MPU_BASE+0x910))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D5             ((P_U32)(EMI_MPU_BASE+0x914))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D6             ((P_U32)(EMI_MPU_BASE+0x918))  /* EMI MPU DOMAIN MASK 0 */
#define EMI_MPU_MASK_D7             ((P_U32)(EMI_MPU_BASE+0x91C))  /* EMI MPU DOMAIN MASK 0 */

#endif /* TZ_EMI_REG_H */

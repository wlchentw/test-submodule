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

u8 MTEE_IMG_VFY_PUBK[256] = {
	0xDA, 0xCD, 0x8B, 0x5F, 0xDA, 0x8A, 0x76, 0x6F, 0xB7, 0xBC, 0xAA, 0x43, 0xF0, 0xB1, 0x69, 0x15,
	0xCE, 0x7B, 0x47, 0x71, 0x4F, 0x13, 0x95, 0xFD, 0xEB, 0xCF, 0x12, 0xA2, 0xD4, 0x11, 0x55, 0xB0,
	0xFB, 0x58, 0x7A, 0x51, 0xFE, 0xCC, 0xCB, 0x4D, 0xDA, 0x1C, 0x8E, 0x5E, 0xB9, 0xEB, 0x69, 0xB8,
	0x6D, 0xAF, 0x2C, 0x62, 0x0F, 0x6C, 0x27, 0x35, 0x21, 0x5A, 0x5F, 0x22, 0xC0, 0xB6, 0xCE, 0x37,
	0x7A, 0xA0, 0xD0, 0x7E, 0xB3, 0x8E, 0xD3, 0x40, 0xB5, 0x62, 0x9F, 0xC2, 0x89, 0x04, 0x94, 0xB0,
	0x78, 0xA6, 0x3D, 0x6D, 0x07, 0xFD, 0xEA, 0xCD, 0xBE, 0x3E, 0x7F, 0x27, 0xFD, 0xE4, 0xB1, 0x43,
	0xF4, 0x9D, 0xB4, 0x97, 0x14, 0x37, 0xE6, 0xD0, 0x0D, 0x9E, 0x18, 0xB5, 0x6F, 0x02, 0xDA, 0xBE,
	0xB0, 0x00, 0x0B, 0x6E, 0x79, 0x51, 0x6D, 0x0C, 0x80, 0x74, 0xB5, 0xA4, 0x25, 0x69, 0xFD, 0x0D,
	0x91, 0x96, 0x65, 0x5D, 0x2A, 0x40, 0x30, 0xD4, 0x2D, 0xFE, 0x05, 0xE9, 0xF6, 0x48, 0x83, 0xE6,
	0xD5, 0xF7, 0x9A, 0x5B, 0xFA, 0x3E, 0x70, 0x14, 0xC9, 0xA6, 0x28, 0x53, 0xDC, 0x1F, 0x21, 0xD5,
	0xD6, 0x26, 0xF4, 0xD0, 0x84, 0x6D, 0xB1, 0x64, 0x52, 0x18, 0x7D, 0xD7, 0x76, 0xE8, 0x88, 0x6B,
	0x48, 0xC2, 0x10, 0xC9, 0xE2, 0x08, 0x05, 0x9E, 0x7C, 0xAF, 0xC9, 0x97, 0xFD, 0x2C, 0xA2, 0x10,
	0x77, 0x5C, 0x1A, 0x5D, 0x9A, 0xA2, 0x61, 0x25, 0x2F, 0xB9, 0x75, 0x26, 0x8D, 0x97, 0x0C, 0x62,
	0x73, 0x38, 0x71, 0xD5, 0x78, 0x14, 0x09, 0x8A, 0x45, 0x3D, 0xF9, 0x2B, 0xC6, 0xCA, 0x19, 0x02,
	0x5C, 0xD9, 0xD4, 0x30, 0xF0, 0x2E, 0xE4, 0x6F, 0x80, 0xDE, 0x6C, 0x63, 0xEA, 0x80, 0x2B, 0xEF,
	0x90, 0x67, 0x3A, 0xAC, 0x4C, 0x66, 0x67, 0xF2, 0x88, 0x3F, 0xB4, 0x50, 0x1F, 0xA7, 0x74, 0x55
};

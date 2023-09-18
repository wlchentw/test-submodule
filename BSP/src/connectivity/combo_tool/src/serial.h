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
* MediaTek Inc. (C) 2014. All rights reserved.
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

#ifndef _LINUX_SERIAL_H
#define _LINUX_SERIAL_H

struct serial_struct {
 int type;
 int line;
 unsigned int port;
 int irq;
 int flags;
 int xmit_fifo_size;
 int custom_divisor;
 int baud_base;
 unsigned short close_delay;
 char io_type;
 char reserved_char[1];
 int hub6;
 unsigned short closing_wait;
 unsigned short closing_wait2;
 unsigned char *iomem_base;
 unsigned short iomem_reg_shift;
 unsigned int port_high;
 unsigned long iomap_base;
};

#define ASYNCB_HUP_NOTIFY  0
#define ASYNCB_FOURPORT   1
#define ASYNCB_SAK   2
#define ASYNCB_SPLIT_TERMIOS  3
#define ASYNCB_SPD_HI   4
#define ASYNCB_SPD_VHI   5
#define ASYNCB_SKIP_TEST  6
#define ASYNCB_AUTO_IRQ   7
#define ASYNCB_SESSION_LOCKOUT  8
#define ASYNCB_PGRP_LOCKOUT  9
#define ASYNCB_CALLOUT_NOHUP 10
#define ASYNCB_HARDPPS_CD 11
#define ASYNCB_SPD_SHI  12
#define ASYNCB_LOW_LATENCY 13
#define ASYNCB_BUGGY_UART 14
#define ASYNCB_AUTOPROBE 15

#endif /* _LINUX_SERIAL_H */

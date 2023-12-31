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

#ifndef _BT_KAL_H
#define _BT_KAL_H

#include "os_dep.h"

#ifndef EXPORT_SYMBOL
#define EXPORT_SYMBOL
#endif


/*
* Structure Definitions
*/

#define HCI_CMD_PARM_LEN   256
#define MAX_EVENT_SIZE     256

typedef struct _HCI_CMD_T {
  UINT16 opCode;  /* HCI command OpCode */
  UINT8  len;     /* Length of the command parameters */
  UINT8  parms[HCI_CMD_PARM_LEN];
} HCI_CMD_T;

typedef struct _HCI_EVENT_T {
  UINT8 event;    /* HCI event type */
  UINT8 len;      /* Length of the event parameters */
  UINT8 *parms;   /* Event specific parameters */
} HCI_EVENT_T;

typedef enum _ENUM_BT_STATUS_T {
  BT_STATUS_SUCCESS = 0,
  BT_STATUS_FAILED,
  BT_STATUS_PENDING,
  BT_STATUS_BUSY,
  BT_STATUS_NO_RESOURCES,
  BT_STATUS_NOT_FOUND,
  BT_STATUS_DEVICE_NOT_FOUND,
  BT_STATUS_CONNECTION_FAILED,
  BT_STATUS_TIMEOUT,
  BT_STATUS_NO_CONNECTION,
  BT_STATUS_INVALID_PARM,
  BT_STATUS_IN_PROGRESS,
  BT_STATUS_RESTRICTED,
  BT_STATUS_INVALID_TYPE,
  BT_STATUS_HCI_INIT_ERR,
  BT_STATUS_NOT_SUPPORTED,
  BT_STATUS_IN_USE,
  BT_STATUS_SDP_CONT_STATE,
  BT_STATUS_CANCELLED,
  BT_STATUS_NOSERVICES,
  BT_STATUS_SCO_REJECT,
  BT_STATUS_CHIP_REASON,
  BT_STATUS_BLOCK_LIST,
  BT_STATUS_SCATTERNET_REJECT
} ENUM_BT_STATUS_T;


/*
* Function Declaration
*/

typedef INT32 (*SETUP_UART_PARAM_T)(INT32 comPort, UINT32 u4Baud, UINT32 u4FlowControl);

BOOL BT_SendHciCommand(INT32 comPort, HCI_CMD_T *pHciCommand);

BOOL BT_ReadExpectedEvent(
  INT32   comPort,
  UINT8*  pEventPacket,
  UINT32  u4MaxBufSz,
  UINT32* pu4PktLen,
  UINT8   ucExpectedEventCode,
  BOOL    fCheckCompleteOpCode,
  UINT16  u2ExpectedOpCode,
  BOOL    fCheckCommandStatus,
  UINT8   ucExpectedStatus
);

int bt_send_data(int fd, unsigned char *buf, unsigned int len);
int bt_receive_data(int fd, unsigned char *buf, unsigned int len);

#endif

/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2013
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/

/* FILE NAME:  c_bt_mw_gatt.h
 * AUTHOR: ZWEI CHEN
 * PURPOSE:
 *      It provides GATTC and GATTS API to APP.
 * NOTES:
 */


#ifndef _C_BT_MW_BLE_H_
#define _C_BT_MW_BLE_H_

/* INCLUDE FILE DECLARATIONS
 */

#include "u_bt_mw_gatt.h"
#include "bt_mw_gatts.h"

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */


/* DATA TYPE DECLARATIONS
 */


/******************** gattc ********************/
/**
 * FUNCTION NAME: c_btm_bt_gattc_base_init
 * PURPOSE:
 *      The function is used to register APP callback function.
 * INPUT:
 *      func               -- gatt client app callback function structure
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                    -- Operate success.
 *      BT_ERR_STATUS_PARM_INVALID    -- paramter invalid.
 * NOTES:
 *      None
 */
#if defined(MTK_LINUX_C4A_BLE_SETUP)

//extern VOID c_btm_bt_gatts_request_exec_write_cbk(INT32 conn_id, INT32 trans_id, CHAR *bda, INT32 exec_write)

extern BOOL c_btm_gatts_isSupported(VOID);

extern INT32 c_btm_gatts_initialize(VOID);
extern INT32 c_btm_gattc_initialize(VOID);
extern BOOL c_btm_gatts_reg_gattServer_callback(BT_APP_BLE_GATTS_CB_FUNC_T *func);
BOOL c_btm_gatts_reg_gattServer_callback(BT_APP_BLE_GATTS_CB_FUNC_T *func);
VOID c_btm_bt_gattc_adv_enable_cbk(INT32 status);

extern INT32 c_btm_gatts_setDeviceName(CHAR * name);
extern INT32 c_btm_gatts_addService(CHAR * service_uuid);
extern INT32 c_btm_gatts_startService(CHAR * service_uuid, INT32 srvc_handle);



extern INT32 c_btm_gatts_sendResponse(INT32 conn_id,
                                          INT32 trans_id,
                                          INT32 status,
                                          INT32 offset,
                                          INT32 handle,
                                          CHAR *p_value,
                                          INT32 value_len,
                                          INT32 auth_req);

extern INT32 c_btm_gattc_setAdvertisement(CHAR * service_uuid, CHAR * advertised_uuid,
                                  CHAR * advertise_data, CHAR * manufacturer_data, BOOL transmit_name);

extern INT32 c_btm_gattc_setScanResponse(CHAR * service_uuid, CHAR * advertised_uuid, CHAR * advertise_data, CHAR * manufacturer_data, BOOL transmit_name);

extern INT32 c_btm_gattc_stopAdvertisement(INT32 client_if);
extern INT32 c_btm_gattc_EnableAdvertisement(INT32 client_if);

extern INT32 c_btm_gatts_send_response_offset(INT32 conn_id,
                                                  INT32 trans_id,
                                                  INT32 status,
                                                  INT32 offset,
                                                  INT32 handle,
                                                  CHAR *p_value,
                                                  INT32 value_len,
                                                  INT32 auth_req);


#endif

#endif /*  _C_BT_MW_GATT_H_  */

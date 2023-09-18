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
 * MediaTek Inc. (C) 2016-2017. All rights reserved.
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


/* FILE NAME:  c_bt_mw_gattc.h
 * AUTHOR: Xuemei Yang
 * PURPOSE:
 *      It provides GATTC API to APP.
 * NOTES:
 */


#ifndef _C_BT_MW_GATTC_H_
#define _C_BT_MW_GATTC_H_

/* INCLUDE FILE DECLARATIONS
 */

#include "u_bt_mw_gattc.h"

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */


/* DATA TYPE DECLARATIONS
 */

/******************** gattc ********************/
/**
 * FUNCTION NAME: c_btm_gattc_register_callback
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
extern INT32 c_btm_gattc_register_callback(BT_APP_GATTC_CB_FUNC_T *func);

/******************** gattc ********************/
/**
 * FUNCTION NAME: c_btm_bt_gattc_unregister_callback
 * PURPOSE:
 *      The function is used to unregister APP callback function.
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

extern INT32 c_btm_bt_gattc_unregister_callback();

/**
extern INT32 c_btm_bt_gattc_unregister_callback();
 * FUNCTION NAME: c_btm_gattc_register_app_sync
 * PURPOSE:
 *      The function is used to synchronous register gatt client APP.
 * INPUT:
 *      uuid               -- app uuid
 * OUTPUT:
 *      client_if          -- registered app identifer
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_register_app_sync(CHAR *uuid, INT32 *client_if);

/**
 * FUNCTION NAME: c_btm_gattc_register_app
 * PURPOSE:
 *      The function is used to register gatt client APP.
 * INPUT:
 *      uuid               -- app uuid
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_register_app(CHAR *uuid);

/**
 * FUNCTION NAME: c_btm_gattc_unregister_app
 * PURPOSE:
 *      The function is used to deregister gatt client APP.
 * INPUT:
 *      client_if              -- registered app identifer
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_unregister_app(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_scan
 * PURPOSE:
 *      The function is used for gatt client scan remote device or stop scan.
 * INPUT:
 *      start              -- start scan or stop scan. (1:start scan, 0:stop  scan)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_scan(BOOL start);

/**
 * FUNCTION NAME: c_btm_gattc_connect
 * PURPOSE:
 *      The function is used for gatt client connect with remote device.
 * INPUT:
 *      client_if              -- registered app identifer
 *      bt_addr                -- remote device bt address
 *      is_direct              -- is direct connection or background connection
 *      transport              -- transport type:(0 : auto, 1 : BREDR, 2 : LE)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_connect(INT32 client_if, CHAR *bt_addr,
                                         UINT8 is_direct, INT32 transport);

/**
 * FUNCTION NAME: c_btm_gattc_disconnect
 * PURPOSE:
 *      The function is used for gatt client disconnect with remote device.
 * INPUT:
 *      client_if              -- registered app identifer
 *      bt_addr                -- remote device bt address
 *      conn_id                -- connection id
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_disconnect(INT32 client_if, CHAR *bt_addr, INT32 conn_id);

/**
 * FUNCTION NAME: c_btm_gattc_listen
 * PURPOSE:
 *      The function is used for gatt client listening operation.
 * INPUT:
 *      client_if              -- registered app identifer
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_listen(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_refresh
 * PURPOSE:
 *      The function is used for gatt client refresh operation.
 * INPUT:
 *      client_if              -- registered app identifer
 *      bt_addr                -- remote device bt address
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_refresh(INT32 client_if, CHAR *bt_addr);

/**
 * FUNCTION NAME: c_btm_gattc_search_service
 * PURPOSE:
 *      The function is used for gatt client search service on the remote device.
 * INPUT:
 *      conn_id                -- connection id
 *      uuid                   -- service uuid
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_search_service(INT32 conn_id, CHAR *uuid);

/**
 * FUNCTION NAME: c_btm_gattc_get_gatt_db
 * PURPOSE:
 *      The function is used for gatt client to get gatt database on on loacal cache database.
 * INPUT:
 *      conn_id                -- connection id
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_get_gatt_db(INT32 conn_id);

/**
 * FUNCTION NAME: c_btm_gattc_read_char
 * PURPOSE:
 *      The function is used for gatt client read characteristic from remote device
 * INPUT:
 *      conn_id                -- connection id
 *      char_handle         -- characteristic handle in database
 *      auth_req               -- authentication request flag
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_read_char(INT32 conn_id, INT32 char_handle,
                                           INT32 auth_req);

/**
 * FUNCTION NAME: c_btm_gattc_read_descr
 * PURPOSE:
 *      The function is used for gatt client read char description from remote device
 * INPUT:
 *      conn_id                -- connection id
 *      descr_handle       -- char descriptor handle in database
 *      auth_req              -- authentication request flag
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_read_descr(INT32 conn_id, INT32 descr_handle,
                                             INT32 auth_req);

/**
 * FUNCTION NAME: c_btm_gattc_write_char
 * PURPOSE:
 *      The function is used for gatt client write characteristic to remote device
 * INPUT:
 *      conn_id                -- connection id
 *      char_handle         -- characteristic handle in database
 *      write_type             -- write type(1:WRITE_TYPE_NO_RSP  2:WRITE_TYPE_REQUEST
 *                                                          3:WRITE_TYPE_PREPARE)
 *      len                    -- write characteristic value length
 *      auth_req               -- authentication request flag
 *      value                  -- write characteristic value
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_write_char(INT32 conn_id, INT32 char_handle,
                                            INT32 write_type, INT32 len,
                                            INT32 auth_req, CHAR *value);


/**
 * FUNCTION NAME: c_btm_gattc_execute_write
 * PURPOSE:
 *      The function is used for gatt client exucute write characteristic to remote device
 * INPUT:
 *      conn_id                -- connection id
 *      execute                -- execute write or cancel execute write(1:execute write
 *                                        0:cancel execute write)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_execute_write(INT32 conn_id, INT32 execute);


/**
 * FUNCTION NAME: c_btm_gattc_write_descr
 * PURPOSE:
 *      The function is used for gatt client write char description to remote device
 * INPUT:
 *      conn_id                -- connection id
 *      descr_handle       -- char descriptor handle in database
 *      write_type           -- write type(1:WRITE_TYPE_NO_RSP  2:WRITE_TYPE_REQUEST  3:WRITE_TYPE_PREPARE)
 *      len                       -- write characteristic value length
 *      auth_req             -- authentication request flag
 *      value                  -- write characteristic value
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_write_descr(INT32 conn_id, INT32 descr_handle,
                                              INT32 write_type, INT32 len,
                                              INT32 auth_req, CHAR *value);

/**
 * FUNCTION NAME: c_btm_gattc_set_reg_noti
 * PURPOSE:
 *      The function is used for gatt client register or deregister notification
 * INPUT:
 *      client_if                -- registered client app identifier
 *      bt_addr                -- remote device bt address
 *      char_handle         -- characteristic handle in database
 *      enable                   -- register or deregister notification. (1:register, 0:deregister)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_set_reg_noti(INT32 client_if, CHAR *bt_addr,
                                         INT32 char_handle, BOOL enable);

/**
 * FUNCTION NAME: c_btm_gattc_read_rssi
 * PURPOSE:
 *      The function is used for gatt client read the received signal strength indicator
 * INPUT:
 *      client_if              -- registered client app identifier
 *      bt_addr                -- remote device bt address
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_read_rssi(INT32 client_if, CHAR *bt_addr);

/**
 * FUNCTION NAME: c_btm_gattc_get_connect_result_info
 * PURPOSE:
 *      The function is used for gatt client to get connection result information
 * INPUT:
 *      None
 * OUTPUT:
 *      connect_rst_info       -- connection result information
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
extern VOID c_btm_gattc_get_connect_result_info(BT_GATTC_CONNECT_RST_T *connect_rst_info);

/**
 * FUNCTION NAME: c_btm_gattc_get_disconnect_result_info
 * PURPOSE:
 *      The function is used for gatt client to get disconnection result information
 * INPUT:
 *      None
 * OUTPUT:
 *      disconnect_rst_info       -- disconnection result information
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
extern VOID c_btm_gattc_get_disconnect_result_info(BT_GATTC_CONNECT_RST_T *disconnect_rst_info);

/**
 * FUNCTION NAME: c_btm_gattc_read_rssi_result_info
 * PURPOSE:
 *      The function is used for gatt client to read received signal strength indicator result inforamtion
 * INPUT:
 *      None
 * OUTPUT:
 *      get_remote_rssi_info     -- received signal strength indicator result inforamtion
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
extern VOID c_btm_gattc_read_rssi_result_info(BT_GATTC_GET_REMOTE_RSSI_T *get_remote_rssi_info);

/**
 * FUNCTION NAME: c_btm_gattc_conn_parameter_update
 * PURPOSE:
 *      The function is used for gatt client to update connection paramters
 * INPUT:
 *      bt_addr                -- remote device bt address
 *      min_interval           -- the minimum allowed connection interval
 *      max_interval           -- the maximum allowed connection interval
 *      latency                -- the maximum allowed connection latency
 *      timeout                -- define the link supervision timeout
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_conn_parameter_update(CHAR *bt_addr,
                                                             INT32 min_interval,
                                                             INT32 max_interval,
                                                             INT32 latency,
                                                             INT32 timeout);

/**
 * FUNCTION NAME: c_btm_gattc_set_scan_parameters
 * PURPOSE:
 *      The function is used for gatt client to set scan paramters
 * INPUT:
 *      client_if              -- registered app identifer
 *      scan_interval          -- how frequently the controller should scan
 *      scan_window            -- how long the controller should scan
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_set_scan_parameters(INT32 client_if,
                                                          INT32 scan_interval,
                                                          INT32 scan_window);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_enable
 * PURPOSE:
 *      The function is used for gatt to enable multiple advertising
 * INPUT:
 *      client_if              -- registered client app identifier
 *      min_interval           -- minimum advertising interval
 *      max_interval           -- maximum advertising interval
 *      adv_type               -- advertising type(0:ADV_IND  1:ADV_DIRECT_IND   2:ADV_SCAN_IND
 *                                                 3:ADV_NONCONN_IND  4:SCAN_RSP)
 *      chnl_map               -- advertising channel map
 *      tx_power               -- transmit power(unit in dBm)
 *      timeout_s              -- not used.
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_enable(INT32 client_if, INT32 min_interval,
                                                     INT32 max_interval, INT32 adv_type,
                                                     INT32 chnl_map, INT32 tx_power,
                                                     INT32 timeout_s);

/**
 * FUNCTION NAME: c_btm_gattc_start_advertising_set
 * PURPOSE:
 *      The function is used for start advertising set
 * INPUT:
 *      client_if                    -- registered client app identifier
 *      p_adv_param                  -- advertising parameters
 *      p_adv_data                   -- advertising data
 *      p_adv_scan_rsp_data          -- advertising scan response data
 *      p_adv_peri_param             -- periodic advertising parameters
 *      p_adv_peri_adv_data          -- periodic advertising data
 *      duration                     -- duration.
 *      maxExtAdvEvents              --max extended advertising event
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_start_advertising_set(INT32 client_if,
                            BT_GATTC_ADVERTISING_PARAMS_T *p_adv_param,
                            BT_GATTC_ADVERTISING_DATA_T   *p_adv_data,
                            BT_GATTC_ADVERTISING_DATA_T   *p_adv_scan_rsp_data,
                            BT_GATTC_PERI_ADV_PERIODIC_PARAMS_T *p_adv_peri_param,
                            BT_GATTC_ADVERTISING_DATA_T *p_adv_peri_adv_data,
                            UINT16 duration,
                            UINT8 maxExtAdvEvents);

/**
 * FUNCTION NAME: c_btm_gattc_advertising_set_param
 * PURPOSE:
 *      The function is used for set advertising parameters
 * INPUT:
 *      client_if         -- registered client app identifier
 *      p_adv_param       -- advertising parameters
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                     -- Operate success.
 *      BT_ERR_STATUS_FAIL             -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID     -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_advertising_set_param(INT32 client_if,
                                 BT_GATTC_ADVERTISING_PARAMS_T *p_adv_param);

/**
 * FUNCTION NAME: c_btm_gattc_advertising_set_peri_param
 * PURPOSE:
 *      The function is used for set periodic advertising periodic parameters
 * INPUT:
 *      client_if              -- registered client app identifier
 *      p_adv_param            -- advertising parameters
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                      -- Operate success.
 *      BT_ERR_STATUS_FAIL              -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID      -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_advertising_set_peri_param(INT32 client_if,
                                  BT_GATTC_PERI_ADV_PERIODIC_PARAMS_T *p_peri_param);

/**
 * FUNCTION NAME: c_btm_gattc_advertising_peri_enable
 * PURPOSE:
 *      The function is used for only enable periodic advertising
 * INPUT:
 *      client_if              -- registered client app identifier
 *      enable                 -- enable(0: disbale 1: enable)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                      -- Operate success.
 *      BT_ERR_STATUS_FAIL              -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID      -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_advertising_peri_enable(INT32 client_if, UINT8 enable);

/**
 * FUNCTION NAME: c_btm_gattc_advertising_set_data
 * PURPOSE:
 *      The function is used for set periodic advertising data
 * INPUT:
 *      client_if               -- registered client app identifier
 *      adv_data_type           -- advertising data type(0: adv data
 *                                                       --1: adv scan response data 2: periodic adv data )
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_advertising_set_data(INT32 client_if, UINT8 adv_data_type,
                                       BT_GATTC_ADVERTISING_DATA_T *p_adv_data);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_enable_sync
 * PURPOSE:
 *      The function is used for gatt to enable multiple advertising
 * INPUT:
 *      client_if              -- registered client app identifier
 *      min_interval           -- minimum advertising interval
 *      max_interval           -- maximum advertising interval
 *      adv_type               -- advertising type(0:ADV_IND  1:ADV_DIRECT_IND   2:ADV_SCAN_IND
 *                                                 3:ADV_NONCONN_IND  4:SCAN_RSP)
 *      chnl_map               -- advertising channel map
 *      tx_power               -- transmit power(unit in dBm)
 *      timeout_s              -- not used.
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_enable_sync(INT32 client_if,
                                                            INT32 min_interval,
                                                            INT32 max_interval,
                                                            INT32 adv_type,
                                                            INT32 chnl_map,
                                                            INT32 tx_power,
                                                            INT32 timeout_s);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_update
 * PURPOSE:
 *      The function is used for gatt to update multiple advertising parameters
 * INPUT:
 *      client_if              -- registered client app identifier
 *      min_interval           -- minimum advertising interval
 *      max_interval           -- maximum advertising interval
 *      adv_type               -- advertising type(0:ADV_IND  1:ADV_DIRECT_IND   2:ADV_SCAN_IND
 *                                                 3:ADV_NONCONN_IND  4:SCAN_RSP)
 *      chnl_map               -- advertising channel map
 *      tx_power               -- transmit power(unit in dBm)
 *      timeout_s              -- not used.
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_update(INT32 client_if, INT32 min_interval,
                                                     INT32 max_interval, INT32 adv_type,
                                                     INT32 chnl_map, INT32 tx_power,
                                                     INT32 timeout_s);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_setdata
 * PURPOSE:
 *      The function is used for gatt to set multiple advertising data
 * INPUT:
 *      client_if              -- registered client app identifier
 *      set_scan_rsp           -- set scan response flag
 *      include_name           -- include device name flag
 *      include_txpower        -- include transmit power flag
 *      appearance             -- appearance data
 *      manufacturer_len       -- manufacturer data length
 *      manufacturer_data      -- manufacturer data
 *      service_data_len       -- service data length
 *      service_data           -- service data
 *      service_uuid_len       -- service uuid length
 *      service_uuid           -- service uuid
 *      le_name_len            -- device name length
 *      le_name                -- device name
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_setdata(INT32 client_if,
                                                     UINT8 set_scan_rsp,
                                                     UINT8 include_name,
                                                     UINT8 include_txpower,
                                                     INT32 appearance,
                                                     INT32 manufacturer_len,
                                                     CHAR* manufacturer_data,
                                                     INT32 service_data_len,
                                                     CHAR* service_data,
                                                     INT32 service_uuid_len,
                                                     CHAR* service_uuid);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_setdata_sync
 * PURPOSE:
 *      The function is used for gatt to set multiple advertising data
 * INPUT:
 *      client_if              -- registered client app identifier
 *      set_scan_rsp           -- set scan response flag
 *      include_name           -- include device name flag
 *      include_txpower        -- include transmit power flag
 *      appearance             -- appearance data
 *      manufacturer_len       -- manufacturer data length
 *      manufacturer_data      -- manufacturer data
 *      service_data_len       -- service data length
 *      service_data           -- service data
 *      service_uuid_len       -- service uuid length
 *      service_uuid           -- service uuid
 *      le_name_len            -- device name length
 *      le_name                -- device name
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_setdata_sync(INT32 client_if,
                                                             UINT8 set_scan_rsp,
                                                             UINT8 include_name,
                                                             UINT8 include_txpower,
                                                             INT32 appearance,
                                                             INT32 manufacturer_len,
                                                             CHAR* manufacturer_data,
                                                             INT32 service_data_len,
                                                             CHAR* service_data,
                                                             INT32 service_uuid_len,
                                                             CHAR* service_uuid);

/**
 * FUNCTION NAME: c_btm_gattc_multi_adv_disable
 * PURPOSE:
 *      The function is used for gatt to disable multiple advertising
 * INPUT:
 *      client_if              -- registered client app identifier
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_multi_adv_disable(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_stop_advertising_set
 * PURPOSE:
 *      The function is used for stop advertising set
 * INPUT:
 *      client_if              -- registered client app identifier
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_stop_advertising_set(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_batchscan_cfg_storage
 * PURPOSE:
 *      The function is used for gatt to configure the batchscan storage
 * INPUT:
 *      client_if             --  registered AP identifier
 *      batch_scan_full_max   -- full max storage space(in %) for batch scan
 *      batch_scan_trunc_max   --  truncate max storage space(in %)  for batch scan
 *      batch_scan_notiy_threshold   --  notify threshold value (in %)  for batch scan storage
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_batchscan_cfg_storage(INT32 client_if,
                                                           INT32 batch_scan_full_max,
                                                           INT32 batch_scan_trunc_max,
                                                           INT32 batch_scan_notify_threshold);

/**
 * FUNCTION NAME: c_btm_gattc_batchscan_enb_batch_scan
 * PURPOSE:
 *      The function is used for gatt to enable batch scan
 * INPUT:
 *      client_if             --  registered AP identifier
 *      scan_mode       --  scan mode , disable: 0x00, truncate mode enable: 0x01,
 *                                     full mode enable: 0x02, truncate and full mode enable: 0x03
 *      scan_interval    --  how frequently to scan
 *      scan_window    -- how long to scan
 *      addr_type         -- address type, public:0x00, random: 0x01
 *      discard_rule    -- discard rule, discard oldest advertisement:0,
 *                                   discard advertisement with weakest rssi: 1
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_batchscan_enb_batch_scan(INT32 client_if,
                                                                 INT32 scan_mode,
                                                                 INT32 scan_interval,
                                                                 INT32 scan_window,
                                                                 INT32 addr_type,
                                                                 INT32 discard_rule);

/**
 * FUNCTION NAME: c_btm_gattc_batchscan_dis_batch_scan
 * PURPOSE:
 *      The function is used for gatt to disable batch scan
 * INPUT:
 *      client_if             --  registered AP identifier
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_batchscan_dis_batch_scan(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_batchscan_read_reports
 * PURPOSE:
 *      The function is used for gatt to read out batchscan reports
 * INPUT:
 *      client_if             --  registered AP identifier
 *      scan_mode       --  scan mode, truncate mode: 1,   full mode: 2
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_batchscan_read_reports(INT32 client_if,
                                                                 INT32 scan_mode);

/**
 * FUNCTION NAME: c_btm_gattc_scan_filter_param_setup
 * PURPOSE:
 *      The function is used for gatt to setup scan filter params
 * INPUT:
 *      scan_filt_param.client_if      -- registered AP identifier
 *      scan_filt_param.action       -- 0: add   1: delete  2: clear
 *      scan_filt_param.filt_index     -- filter index
 *      scan_filt_param.feat_seln     -- filter selection number,
 *                                                        Bit 0: Set to enable Broadcast Address filter
 *                                                        Bit 1: Set to enable Service Data Change filter
 *                                                        Bit 2: Set to enable Service UUID check
 *                                                        Bit 3: Set to enable Service Solicitation UUID check
 *                                                        Bit 4: Set to enable Local Name check
 *                                                        Bit 5: Set to enable Manufacturer Data Check
 *                                                        Bit 6: Set to enable Service Data Check
 *      scan_filt_param.list_logic_type     -- the logic to list for each filter field
 *      scan_filt_param.filt_logic_type     -- the logic to filter for each filter field,
 *                                                                 it only applicable for (Bit 3~Bit 6) four fields
 *                                                                 of APCF_Feature_Selection
 *      scan_filt_param.rssi_high_thres     -- upper threshold of the rssi value
 *      scan_filt_param.rssi_low_thres     -- lower threshold of the rssi value
 *      scan_filt_param.dely_mode     --   0: immediate    1:on_found   2: batched
 *      scan_filt_param.found_timeout     -- time for firmware to linger and collect
 *                                                                 additional advertisements before reporting,
 *                                                                 valid only if dely_mode is on_found
 *      scan_filt_param.lost_timeout     -- If an advertisement, after being found,
 *                                                              is not seen contiguously for the lost_timeout period,
 *                                                              it will be reported lost. Valid only if dely_mode is on _found
 *      scan_filt_param.found_timeout_cnt   -- count of found timeout
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_scan_filter_param_setup(BT_GATTC_FILT_PARAM_SETUP_T scan_filt_param);

/**
 * FUNCTION NAME: c_btm_gattc_scan_filter_enable
 * PURPOSE:
 *      The function is used for gatt to enable or disable scan filter feature
 * INPUT:
 *      client_if             --  registered AP identifier
 *      enable              --  enable or disable scan filter(1: enable, 0:disable)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_scan_filter_enable(INT32 client_if, BOOL enable);

/**
 * FUNCTION NAME: c_btm_gattc_scan_filter_add_remove
 * PURPOSE:
 *      The function is used for gatt to configure a scan filter condition
 * INPUT:
 *      client_if             --  registered AP identifier
 *      action               -- 0: add    1: delete  2: clear
 *      filt_type            -- 0: addr    1: service data   2: service uuid   3: solicited service uuid
 *                                   4: local name   5: manufacture data      6: service data pattern
 *      filt_index          -- filter index
 *      company_id       -- company id
 *      company_id_mask    -- the mask of company id
 *      p_uuid    -- bluetooth 128-bit UUID, e.g. "49557E51D81511E488300800200C9A66"
 *      p_uuid_mask    -- the mask of Bluetooth 128-bit UUID
 *      bd_addr    -- bluetooth device address, e.g. "AA:BB:CC:DD:EE:FF"
 *                            [NAP-AA:BB][UAP-CC][LAP-DD:EE:FF]
 *      addr_type    -- address type
 *      data_len    -- length of value
 *      p_data      -- value
 *      mask_len    -- the length of mask
 *      p_mask    -- mask
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_scan_filter_add_remove(INT32 client_if,
                                                             INT32 action,
                                                             INT32 filt_type,
                                                             INT32 filt_index,
                                                             INT32 company_id,
                                                             INT32 company_id_mask,
                                                             const CHAR *p_uuid,
                                                             const CHAR *p_uuid_mask,
                                                             const CHAR *bd_addr,
                                                             CHAR addr_type,
                                                             INT32 data_len,
                                                             CHAR *p_data,
                                                             INT32 mask_len,
                                                             CHAR *p_mask);

/**
 * FUNCTION NAME: c_btm_gattc_scan_filter_clear
 * PURPOSE:
 *      The function is used for gatt to clear all scan filter conditions for specific filter index
 * INPUT:
 *      client_if             --  registered AP identifier
 *      filt_index           --  filter index
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_scan_filter_clear(INT32 client_if, INT32 filt_index);


/**
 * FUNCTION NAME: c_btm_gattc_get_device_type
 * PURPOSE:
 *      The function is used for gatt to get the type of the remote device
 * INPUT:
 *      bd_addr   -- address of discovered device(input), e.g. "AA:BB:CC:DD:EE:FF"
 *                            [NAP-AA:BB][UAP-CC][LAP-DD:EE:FF]
 * OUTPUT:
 *      None
 * RETURN:
 *       0: unknown
 *       1: BR/EDR
 *       2: LE
 *       3: dual mode
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_get_device_type(CHAR *bt_addr);

/**
 * FUNCTION NAME: c_btm_gattc_set_local_le_name
 * PURPOSE:
 *      The function is used for gatt to set local le device name
 * INPUT:
 *      client_if             --  registered AP identifier
 *      le_name           --  local le device name
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_set_local_le_name(INT32 client_if, CHAR *le_name);

/**
 * FUNCTION NAME: c_btm_gattc_set_local_disc_mode
 * PURPOSE:
 *      The function is used for gatt to set advertising discoverable mode
 * INPUT:
 *      client_if             --  registered AP identifier
 *      disc_mode        --  discoverable mode (default:0,
 *                                     6:BTM_BLE_GEN_DISC_FLAG  | BTM_BLE_BREDR_NOT_SPT)
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_set_local_disc_mode(INT32 client_if, INT32 disc_mode);

/**
 * FUNCTION NAME: c_btm_gattc_get_local_adv_rpa
 * PURPOSE:
 *      The function is used for gatt to get local advertising rpa
 * INPUT:
 *      client_if             --  registered AP identifier
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_get_local_adv_rpa(INT32 client_if);

/**
 * FUNCTION NAME: c_btm_gattc_configure_mtu
 * PURPOSE:
 *      The function is used for gatt to configure the MTU for a given connection
 * INPUT:
 *      conn_id             --  identifier of connection
 *      mtu                   --  maximum transmit unit
 * OUTPUT:
 *      None
 * RETURN:
 *      BT_SUCCESS                        -- Operate success.
 *      BT_ERR_STATUS_FAIL                -- Operate fail.
 *      BT_ERR_STATUS_PARM_INVALID        -- paramter is invalid.
 * NOTES:
 *      None
 */
extern INT32 c_btm_gattc_configure_mtu(INT32 conn_id, INT32 mtu);

/*for test mode interface*/
extern INT32 c_btm_gattc_test_command(INT32 command, BT_GATTC_TEST_PARAMS_T test_params);

/* for pts test start */
extern INT32 c_btm_gattc_read_using_char_uuid(INT32 conn_id,
                                                          INT32 start_handle,
                                                          INT32 end_handle,
                                                          CHAR *uuid,
                                                          INT32 auth_req);

extern INT32 c_btm_gattc_read_long_characteristic(INT32 conn_id,
                                                             INT32 handle,
                                                             INT32 offset,
                                                             INT32 auth_req);

extern INT32 c_btm_gattc_read_multi_characteristic(INT32 conn_id,
                                                              INT32 num_attr,
                                                              UINT16 *handles,
                                                              INT32 auth_req);

extern INT32 c_btm_gattc_read_long_descr(INT32 conn_id,
                                                    INT32 handle,
                                                    INT32 offset,
                                                    INT32 auth_req);

extern INT32 c_btm_gattc_write_long_characteristic(INT32 conn_id,
                                                              INT32 char_handle,
                                                              INT32 write_type,
                                                              INT32 len,
                                                              INT32 offset,
                                                              INT32 auth_req,
                                                              CHAR *value);

extern INT32 c_btm_gattc_write_long_descr(INT32 conn_id,
                                                    INT32 descr_handle,
                                                    INT32 write_type,
                                                    INT32 len,
                                                    INT32 offset,
                                                    INT32 auth_req,
                                                    CHAR *value);
extern INT32 c_btm_gattc_set_pts_flag(UINT8 pts_flag);
/*for pts test end*/

extern INT32 c_btm_gattc_set_preferred_phy(CHAR *bt_addr, UINT8 tx_phy,
                                        UINT8 rx_phy, UINT16 phy_options);

extern INT32 c_btm_gattc_read_phy(CHAR *bt_addr);

extern INT32 c_btm_gattc_set_adv_ext_param(INT32 client_if, UINT16 event_properties,
    UINT8 primary_phy, UINT8 secondary_phy, UINT8 scan_req_notify_enable);

#endif /*  _C_BT_MW_GATTC_H_  */


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


/* FILE NAME:  u_bt_mw_gattc.h
 * AUTHOR: Xuemei Yang
 * PURPOSE:
 *      It provides GATTC structure to APP.
 * NOTES:
 */


#ifndef _U_BT_MW_GATTC_H_
#define _U_BT_MW_GATTC_H_

/*-----------------------------------------------------------------------------
                    include files
-----------------------------------------------------------------------------*/
#include "u_bt_mw_common.h"
#include "u_bt_mw_gatt.h"

#define BT_GATTC_MAX_MTU_SIZE 517
#define BT_GATTC_MIN_MTU_SIZE 23

#define BLE_ADV_TX_POWER_LEVEL_TYPE 0x0A

#define MTK_LINUX_GATT   TRUE

#if MTK_LINUX_GATT
#define MTK_LINUX_GATTC_LE_NAME  1
#define MTK_LINUX_GATTC_RPA      1
#define MTK_LINUX_GATTC_PTS_TEST 0
#define MTK_LINUX_GATTC_DISC_MODE 1
#else
#define MTK_LINUX_GATTC_LE_NAME  0
#define MTK_LINUX_GATTC_RPA      0
#define MTK_LINUX_GATTC_PTS_TEST 0
#define MTK_LINUX_GATTC_DISC_MODE 0
#endif

typedef struct
{
    INT32 client_if;
    CHAR uuid[BT_GATT_MAX_UUID_LEN];
} BT_GATTC_REG_CLIENT_T;

typedef struct
{
    INT32 rssi;
    CHAR btaddr[MAX_BDADDR_LEN];
    CHAR adv_data[BT_GATT_MAX_ATTR_LEN];
} BT_GATTC_SCAN_RST_T;

typedef struct
{
    INT32 conn_id;
    INT32 client_if;
    CHAR btaddr[MAX_BDADDR_LEN];
} BT_GATTC_CONNECT_RST_T;

#define APP_BLE_ADV_CHNL_37    (0x01 << 0)
#define APP_BLE_ADV_CHNL_38    (0x01 << 1)
#define APP_BLE_ADV_CHNL_39    (0x01 << 2)
typedef UINT8 tAPP_BLE_ADV_CHNL_MAP;

#define ADV_IND                     0x00
#define ADV_DIRECT_IND_HIGH_DUTY    0x01
#define ADV_SCAN_IND                0x02
#define ADV_NONCONN_IND             0x03
#define ADV_DIRECT_IND_LOW_DUTY     0x04
typedef UINT8 tAPP_ADV_TYPE;

#define ADV_CONNECTABLE                     (0x01 << 0)
#define ADV_SCANABLE                        (0x01 << 1)
#define ADV_DIRECTED                        (0x01 << 2)
#define ADV_DIRECTED_CONNECABLE_HIG_DUTY    (0x01 << 3)
#define ADV_USE_LEGACY                      (0x01 << 4)
#define ADV_ANONYMOUS                       (0x01 << 5)
#define ADV_INCLUDE_TXPOWER                 (0x01 << 6)
typedef UINT16 tAPP_ADV_EVENT_PROPERTIES;

#define PERIODIC_ADV_INCLUDE_TXPOWER         (0x01 << 6)
typedef UINT16 tAPP_PERIODIC_ADV_PROPERTIES;

/*tx power Recommended value {-21, -15, -7, 1, 9}*/
#define BLE_ADV_TX_POWER_MIN        -21           /* minimum tx power */
#define BLE_ADV_TX_POWER_LOW        -15           /* low tx power */
#define BLE_ADV_TX_POWER_MID        -7           /* middle tx power */
#define BLE_ADV_TX_POWER_UPPER      1           /* upper tx power */
#define BLE_ADV_TX_POWER_MAX        9           /* maximum tx power */

typedef struct
{
    UINT16                    adv_int_min;            /* minimum adv interval */
    UINT16                    adv_int_max;            /* maximum adv interval */
    tAPP_ADV_TYPE             adv_type;               /* adv event type */
    tAPP_BLE_ADV_CHNL_MAP     channel_map;            /* adv channel map */
    UINT8                     adv_filter_policy;      /* advertising filter policy */
    INT8                      tx_power;               /* adv tx power */
    tAPP_ADV_EVENT_PROPERTIES advertising_event_properties;
    UINT8                     primary_advertising_phy;
    UINT8                     secondary_advertising_phy;
    UINT8                     scan_request_notification_enable;
}BT_GATTC_ADVERTISING_PARAMS_T;


#define MAX_SIZE_ADV_DATA_LEN 1024
typedef struct
{
    UINT32 adv_len;
    UINT8 adv_data[MAX_SIZE_ADV_DATA_LEN];
}BT_GATTC_ADVERTISING_DATA_T;

typedef struct {
  UINT8 enable;
  UINT16 min_interval;
  UINT16 max_interval;
  tAPP_PERIODIC_ADV_PROPERTIES periodic_advertising_properties;
}BT_GATTC_PERI_ADV_PERIODIC_PARAMS_T;

typedef struct
{
    INT32 conn_id;
    INT32 status;
    INT32 mtu;
} BT_GATTC_MTU_RST_T;

typedef struct
{
    INT32 conn_id;
    INT32 registered;
    UINT32 handle;
} BT_GATTC_GET_REG_NOTI_RST_T;

/** Attribute change notification parameters */
typedef struct
{
    UINT8             value[BT_GATT_MAX_ATTR_LEN];
    CHAR             bda[MAX_BDADDR_LEN];
    UINT32           handle;
    UINT32            len;
    UINT8             is_notify;
} BT_GATTC_NOTI_PARAMS_T;


typedef struct
{
    INT32 conn_id;
    BT_GATTC_NOTI_PARAMS_T notify_data;
} BT_GATTC_GET_NOTIFY_T;

/** Buffer type for unformatted reads/writes */
typedef struct
{
    UINT8             value[BT_GATT_MAX_ATTR_LEN];
    UINT32            len;
} BT_GATTC_UNFORMATTED_VALUE_T;

/** Parameters for GATT read operations */
typedef struct
{
    UINT32            handle;
    BT_GATTC_UNFORMATTED_VALUE_T value;
    UINT32            value_type;
    UINT8             status;
} BT_GATTC_READ_PARAMS_T;

typedef struct
{
    INT32 conn_id;
    BT_GATTC_READ_PARAMS_T read_data;
} BT_GATTC_READ_CHAR_RST_T;

/** Parameters for GATT write operations */

typedef struct
{
    INT32 status;
    INT32 conn_id;
    UINT32 handle;
} BT_GATTC_WRITE_CHAR_RST_T;

typedef struct
{
    INT32 conn_id;
    BT_GATTC_READ_PARAMS_T read_data;
} BT_GATTC_READ_DESCR_RST_T;


typedef struct
{
    INT32 status;
    INT32 conn_id;
    UINT32 handle;
} BT_GATTC_WRITE_DESCR_RST_T;

typedef struct
{
    INT32 client_if;
    CHAR btaddr[MAX_BDADDR_LEN];
    INT32 rssi;
} BT_GATTC_GET_REMOTE_RSSI_T;

typedef struct
{
    INT32 conn_id;
    INT32 client_if;
    INT32 rssi;
    CHAR btaddr[MAX_BDADDR_LEN];
} BT_GATTC_CONNECT_STATE_OR_RSSI_T;

typedef enum
{
    BT_GATTC_REGISTER_CLIENT = 0,
    BT_GATTC_CONNECT,
    BT_GATTC_DISCONNECT,
    BT_GATTC_GET_RSSI_DONE,

    BT_GATTC_EVENT_MAX
}BT_GATTC_EVENT_T;

typedef struct
{
    INT32 client_if;
    INT32 action;
    INT32 avbl_space;
} BT_GATTC_SCAN_FILTER_PARAM_T;

typedef struct
{
    INT32 client_if;
    INT32 enable;
} BT_GATTC_SCAN_FILTER_STATUS_T;

typedef struct
{
    INT32 client_if;
    INT32 action;
    INT32 filt_type;
    INT32 avbl_space;
} BT_GATTC_SCAN_FILTER_CFG_T;

typedef struct
{
    UINT8  client_if;
    UINT8  action;
    UINT8  filt_index;
    UINT32 feat_seln;
    UINT32 list_logic_type;
    UINT8  filt_logic_type;
    UINT8  rssi_high_thres;
    UINT8  rssi_low_thres;
    UINT8  dely_mode;
    UINT32 found_timeout;
    UINT32 lost_timeout;
    UINT8  found_timeout_cnt;
    UINT32  num_of_tracking_entries;
} BT_GATTC_FILT_PARAM_SETUP_T;

typedef enum
{
    BT_GATTC_DB_PRIMARY_SERVICE,
    BT_GATTC_DB_SECONDARY_SERVICE,
    BT_GATTC_DB_INCLUDED_SERVICE,
    BT_GATTC_DB_CHARACTERISTIC,
    BT_GATTC_DB_DESCRIPTOR,
} BT_GATTC_DB_ATTRIBUTE_TYPE_T;

typedef struct
{
    UINT32           id;
    CHAR            uuid[BT_GATT_MAX_UUID_LEN];
    BT_GATTC_DB_ATTRIBUTE_TYPE_T type;
    UINT32            attribute_handle;

    /*
     * If |type| is |BTGATT_DB_PRIMARY_SERVICE|, or
     * |BTGATT_DB_SECONDARY_SERVICE|, this contains the start and end attribute
     * handles.
     */
    UINT32            start_handle;
    UINT32            end_handle;

    /*
     * If |type| is |BTGATT_DB_CHARACTERISTIC|, this contains the properties of
     * the characteristic.
     */
    UINT8             properties;
} BT_GATTC_DB_ELEMENT_T;

typedef struct
{
    INT32 conn_id;
    INT32 count;
    BT_GATTC_DB_ELEMENT_T *gatt_db_element;
}BT_GATTC_GET_GATT_DB_T;

typedef struct
{
    INT32 status;
    INT32 enable;
} BT_GATTC_ADV_ENABLED_T;


typedef struct
{
    CHAR                *bda1;
    CHAR                *uuid1;
    UINT32              u1;
    UINT32              u2;
    UINT32              u3;
    UINT32              u4;
    UINT32              u5;
} BT_GATTC_TEST_PARAMS_T;

typedef struct
{
    CHAR btaddr[MAX_BDADDR_LEN];
    UINT8 tx_phy;
    UINT8 rx_phy;
    UINT8 status;
} BT_GATTC_PHY_UPDATED_T;

typedef VOID (*BtAppGATTCEventCbk)(BT_GATTC_EVENT_T bt_gatt_event, BT_GATTC_CONNECT_STATE_OR_RSSI_T *bt_gattc_connect_state_or_rssi);
typedef VOID (*BtAppGATTCRegClientCbk)(BT_GATTC_REG_CLIENT_T *pt_reg_client_result);
typedef VOID (*BtAppGATTCScanCbk)(BT_GATTC_SCAN_RST_T *pt_scan_result);
typedef VOID (*BtAppGATTCGetGattDbCbk)(BT_GATTC_GET_GATT_DB_T *pt_get_gatt_db_result);
typedef VOID (*BtAppGATTCGetRegNotiCbk)(BT_GATTC_GET_REG_NOTI_RST_T *pt_get_reg_noti_result);
typedef VOID (*BtAppGATTCNotifyCbk)(BT_GATTC_GET_NOTIFY_T *pt_notify);
typedef VOID (*BtAppGATTCReadCharCbk)(BT_GATTC_READ_CHAR_RST_T *pt_read_char);
typedef VOID (*BtAppGATTCWriteCharCbk)(BT_GATTC_WRITE_CHAR_RST_T *pt_write_char);
typedef VOID (*BtAppGATTCReadDescCbk)(BT_GATTC_READ_DESCR_RST_T *pt_read_desc);
typedef VOID (*BtAppGATTCWriteDescCbk)(BT_GATTC_WRITE_DESCR_RST_T *pt_write_desc);
typedef VOID (*BtAppGATTCScanFilterParamCbk)(BT_GATTC_SCAN_FILTER_PARAM_T *pt_scan_filter_param);
typedef VOID (*BtAppGATTCScanFilterStatusCbk)(BT_GATTC_SCAN_FILTER_STATUS_T *pt_scan_filter_status);
typedef VOID (*BtAppGATTCScanFilterCfgCbk)(BT_GATTC_SCAN_FILTER_CFG_T *pt_scan_filter_cfg);
typedef VOID (*BtAppGATTCAdvEnableCbk)(BT_GATTC_ADV_ENABLED_T *pt_adv_enabled);
typedef VOID (*BtAppGATTCConfigMtuCbk)(BT_GATTC_MTU_RST_T *pt_config_mtu_result);
typedef VOID (*BtAppGATTCPhyUpdatedCbk)(BT_GATTC_PHY_UPDATED_T *pt_phy_updated);
typedef VOID (*BtAppGATTCPeriAdvEnableCbk)(BT_GATTC_ADV_ENABLED_T *pt_peri_adv_enabled);

typedef struct
{
    BtAppGATTCEventCbk bt_gattc_event_cb;
    BtAppGATTCRegClientCbk bt_gattc_reg_client_cb;
    BtAppGATTCScanCbk bt_gattc_scan_cb;
    BtAppGATTCGetGattDbCbk bt_gattc_get_gatt_db_cb;
    BtAppGATTCGetRegNotiCbk bt_gattc_get_reg_noti_cb;
    BtAppGATTCNotifyCbk bt_gattc_notify_cb;
    BtAppGATTCReadCharCbk bt_gattc_read_char_cb;
    BtAppGATTCWriteCharCbk bt_gattc_write_char_cb;
    BtAppGATTCReadDescCbk bt_gattc_read_desc_cb;
    BtAppGATTCWriteDescCbk bt_gattc_write_desc_cb;
    BtAppGATTCScanFilterParamCbk bt_gattc_scan_filter_param_cb;
    BtAppGATTCScanFilterStatusCbk bt_gattc_scan_filter_status_cb;
    BtAppGATTCScanFilterCfgCbk bt_gattc_scan_filter_cfg_cb;
    BtAppGATTCAdvEnableCbk     bt_gattc_adv_enable_cb;
    BtAppGATTCConfigMtuCbk     bt_gattc_config_mtu_cb;
    BtAppGATTCPhyUpdatedCbk    bt_gattc_phy_updated_cb;
    BtAppGATTCPeriAdvEnableCbk bt_gattc_peri_adv_enable_cb;
}BT_APP_GATTC_CB_FUNC_T;

#endif /*  _U_BT_MW_GATTC_H_ */





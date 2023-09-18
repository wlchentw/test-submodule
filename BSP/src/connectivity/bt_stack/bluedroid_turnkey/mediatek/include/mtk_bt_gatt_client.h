/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef MTK_BT_GATT_CLIENT_H
#define MTK_BT_GATT_CLIENT_H

#include <stdint.h>
#include <hardware/bt_gatt_types.h>
#include <hardware/bt_common_types.h>
#include <hardware/bt_gatt_client.h>

__BEGIN_DECLS

typedef void (*get_adv_rpa_callback)(int client_if, int status, bt_bdaddr_t* rpa);

/** Callback invoked when the PHY for a given connection changes */
typedef void (*phy_updated_callback)(const bt_bdaddr_t *bd_addr, uint8_t tx_phy,
                                     uint8_t rx_phy, uint8_t status);
typedef void (*peri_adv_enable_callback)(int client_if, int status, int enable);

typedef struct {
    get_adv_rpa_callback get_adv_rpa_cb;
    phy_updated_callback phy_updated_cb;
    peri_adv_enable_callback  peri_adv_enable_cb;
} btgatt_ex_client_callbacks_t;

typedef struct
{
    UINT16                  adv_int_min;            /* minimum adv interval */
    UINT16                  adv_int_max;            /* maximum adv interval */
    UINT8                   adv_type;               /* adv event type */
    UINT8                   channel_map;            /* adv channel map */
    UINT8                   adv_filter_policy;      /* advertising filter policy */
    INT8                    tx_power;               /* adv tx power */
    UINT16                  advertising_event_properties;
    UINT8                   primary_advertising_phy;
    UINT8                   secondary_advertising_phy;
    UINT8                   scan_request_notification_enable;
}tBTIF_BLE_ADV_PARAMS;

#define MAX_BTIF_SIZE_ADV_DATA_LEN 1024
typedef struct
{
    UINT32 adv_len;
    UINT8 adv_data[MAX_BTIF_SIZE_ADV_DATA_LEN];
}tBTIF_BLE_PERI_ADV_DATA;

typedef struct {
  uint8_t enable;
  uint16_t min_interval;
  uint16_t max_interval;
  uint16_t periodic_advertising_properties;
}tBTIF_BLE_PERIODIC_ADV_PARAMS;

typedef struct
{
    tBTIF_BLE_ADV_PARAMS     adv_params;
    tBTIF_BLE_PERI_ADV_DATA  adv_data;
    tBTIF_BLE_PERI_ADV_DATA  adv_scan_rsp_data;
    tBTIF_BLE_PERIODIC_ADV_PARAMS peri_params;
    tBTIF_BLE_PERI_ADV_DATA  peri_adv_data;
    UINT16                  duration;
    UINT8                   maxExtAdvEvents;
}tBTIF_BLE_ADVERTISING_SET;

/** Represents the standard BT-GATT client interface. */
typedef struct {
    /** set local lename*/
    bt_status_t (*set_local_le_name)(int client_if, char *p_name);

    /** get local rpa address*/
    bt_status_t (*get_adv_rpa)(int client_if);

    /** set discoverable mode*/
    bt_status_t (*set_discoverable_mode)(int client_if, int disc_mode);
#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
    /** read using characteristic uuid*/
    bt_status_t (*read_using_characteristic_uuid)(int conn_id, uint16_t start_handle,
         uint16_t end_handle, bt_uuid_t *char_uuid, int auth_req);
    /** read long characteristic*/
    bt_status_t (*read_long_characteristic)(int conn_id, uint16_t handle, uint16_t offset,
        int auth_req);

    /** read multiple characteristic*/
    bt_status_t (*read_multi_characteristic)(int conn_id, uint8_t num_attr, uint16_t *handles,
        int auth_req);

    /** read long char descr*/
    bt_status_t (*read_long_char_descr)(int conn_id, uint16_t handle, uint16_t offset,
        int auth_req);

    /** write long char*/
    bt_status_t (*write_long_char)(int conn_id, uint16_t handle, int write_type, int len,
        uint16_t offset, int auth_req, char *p_value);

    /** write long char descr*/
    bt_status_t (*write_long_char_descr)(int conn_id, uint16_t handle, int write_type, int len,
        uint16_t offset, int auth_req, char *p_value);

    /** pts test mode flag*/
    bt_status_t (*set_pts_test_flag)(uint8_t pts_flag);
#endif

    bt_status_t (*set_preferred_phy)(const bt_bdaddr_t *bd_addr, uint8_t tx_phy,
                                     uint8_t rx_phy, uint16_t phy_options);

    bt_status_t (*read_phy)(const bt_bdaddr_t *bd_addr);

    bt_status_t (*set_adv_ext_param)(int client_if, uint16_t event_properties, uint8_t primary_phy,
                                     uint8_t secondary_phy, uint8_t scan_req_notify_enable);
    bt_status_t (*start_advertising_set)(int client_if,
                          tBTIF_BLE_ADVERTISING_SET *p_advertising_set);
    bt_status_t (*stop_advertising_set)(int client_if);
    bt_status_t (*set_periodic_param)(int client_if, tBTIF_BLE_PERIODIC_ADV_PARAMS *p_peri_param);
    bt_status_t (*set_advertising_data)(int client_if,
                 UINT8 adv_data_type, tBTIF_BLE_PERI_ADV_DATA *p_adv_data);
    bt_status_t (*set_advertising_param)(int client_if, tBTIF_BLE_ADV_PARAMS *p_param);
    bt_status_t (*peri_adv_enable)(int client_if, UINT8 enable);
} btgatt_ex_client_interface_t;

__END_DECLS

#endif /* MTK_BT_GATT_CLIENT_H */

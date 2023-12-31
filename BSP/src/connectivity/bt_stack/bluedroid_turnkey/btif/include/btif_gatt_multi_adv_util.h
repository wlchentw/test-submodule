/******************************************************************************
 *
 *  Copyright (C) 2014  Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

#pragma once

#include <hardware/bluetooth.h>

#include "bta_api.h"

#define CLNT_IF_IDX 0
#define INST_ID_IDX 1
#define INST_ID_IDX_MAX (INST_ID_IDX + 1)
#define INVALID_ADV_INST -1
#define STD_ADV_INSTID 0

/* Default ADV flags for general and limited discoverability */
#define ADV_FLAGS_LIMITED BTA_DM_LIMITED_DISC
#define ADV_FLAGS_GENERAL BTA_DM_GENERAL_DISC

typedef struct
{
    int client_if;
    BOOLEAN set_scan_rsp;
    BOOLEAN include_name;
    BOOLEAN include_txpower;
    int min_interval;
    int max_interval;
    int appearance;
    uint16_t manufacturer_len;
    uint8_t* p_manufacturer_data;
    uint16_t service_data_len;
    uint8_t* p_service_data;
    uint16_t service_uuid_len;
    uint8_t* p_service_uuid;
} btif_adv_data_t;

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
typedef struct
{
    tBTA_BLE_AD_MASK mask;
    tBTA_BLE_ADV_DATA data;
} btgatt_ext_adv_data_t;
#endif

typedef struct
{
    UINT8 client_if;
    tBTA_BLE_AD_MASK mask;
    tBTA_BLE_ADV_DATA data;
    tBTA_BLE_ADV_PARAMS param;
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    tBTA_BLE_ADVERTISING_SET advertising_set;
#endif
    alarm_t *multi_adv_timer;
    int timeout_s;
} btgatt_multi_adv_inst_cb;

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
typedef struct
{
    UINT8 client_if;
    UINT8 adv_data_type;
    tBTA_BLE_PERI_ADV_DATA  adv_data;
} btgatt_adv_data_cb;

typedef struct
{
    UINT8 client_if;
    UINT8 enable;
} btgatt_adv_peri_enable_cb;
#endif

typedef struct
{
    INT8 *clntif_map;
    // Includes the stored data for standard LE instance
    btgatt_multi_adv_inst_cb *inst_cb;

} btgatt_multi_adv_common_data;

extern btgatt_multi_adv_common_data *btif_obtain_multi_adv_data_cb();

extern void btif_gattc_incr_app_count(void);
extern void btif_gattc_decr_app_count(void);
extern int btif_multi_adv_add_instid_map(int client_if, int inst_id,
        BOOLEAN gen_temp_instid);
extern int btif_multi_adv_instid_for_clientif(int client_if);
extern int btif_gattc_obtain_idx_for_datacb(int value, int clnt_inst_index);
extern void btif_gattc_clear_clientif(int client_if, BOOLEAN stop_timer);
extern void btif_gattc_cleanup_inst_cb(int inst_id, BOOLEAN stop_timer);
extern void btif_gattc_cleanup_multi_inst_cb(btgatt_multi_adv_inst_cb *p_inst_cb,
                                                    BOOLEAN stop_timer);
extern BOOLEAN btif_gattc_copy_datacb(int arrindex, const btif_adv_data_t *p_adv_data,
                                            BOOLEAN bInstData);
extern void btif_gattc_adv_data_packager(int client_if, bool set_scan_rsp,
                bool include_name, bool include_txpower, int min_interval, int max_interval,
                int appearance, int manufacturer_len, char* manufacturer_data,
                int service_data_len, char* service_data, int service_uuid_len,
                char* service_uuid, btif_adv_data_t *p_multi_adv_inst);
extern void btif_gattc_adv_data_cleanup(btif_adv_data_t* adv);
void btif_multi_adv_timer_ctrl(int client_if, alarm_callback_t cb);


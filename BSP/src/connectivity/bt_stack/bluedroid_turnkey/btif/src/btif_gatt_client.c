/******************************************************************************
 *
 *  Copyright (C) 2009-2014 Broadcom Corporation
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

/*******************************************************************************
 *
 *  Filename:      btif_gatt_client.c
 *
 *  Description:   GATT client implementation
 *
 *******************************************************************************/

#define LOG_TAG "bt_btif_gattc"

#include <errno.h>
#include <hardware/bluetooth.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device/include/controller.h"


#include "btcore/include/bdaddr.h"
#include "btif_common.h"
#include "btif_util.h"

#if (defined(BLE_INCLUDED) && (BLE_INCLUDED == TRUE))
#include <hardware/bt_gatt.h>

#include "bta_api.h"
#include "bta_gatt_api.h"
#include "btif_config.h"
#include "btif_dm.h"
#include "btif_gatt.h"
#include "btif_gatt_multi_adv_util.h"
#include "btif_gatt_util.h"
#include "btif_storage.h"
#include "btif_storage.h"
#include "osi/include/log.h"
#include "vendor_api.h"
#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
#include "mtk_bt_gatt.h"
#endif

/* add for GATTC queue begin */
#include "bta_gattc_queue.h"
/* add for GATTC queue end */

/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define CHECK_BTGATT_INIT() if (bt_gatt_callbacks == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTGATT not initialized", __FUNCTION__);\
        return BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
#define CHECK_BTGATT_EX_INIT() if (bt_gatt_ex_callbacks == NULL)\
    {\
        LOG_WARN(LOG_TAG, "%s: BTGATT extend not initialized", __FUNCTION__);\
        return BT_STATUS_NOT_READY;\
    } else {\
        LOG_VERBOSE(LOG_TAG, "%s", __FUNCTION__);\
    }
#endif

#define BLE_RESOLVE_ADDR_MSB                 0x40   /* bit7, bit6 is 01 to be resolvable random */
#define BLE_RESOLVE_ADDR_MASK                0xc0   /* bit 6, and bit7 */
#define BTM_BLE_IS_RESOLVE_BDA(x)           ((x[0] & BLE_RESOLVE_ADDR_MASK) == BLE_RESOLVE_ADDR_MSB)

typedef enum {
    BTIF_GATTC_REGISTER_APP = 1000,
    BTIF_GATTC_UNREGISTER_APP,
    BTIF_GATTC_SCAN_START,
    BTIF_GATTC_SCAN_STOP,
    BTIF_GATTC_OPEN,
    BTIF_GATTC_CLOSE,
    BTIF_GATTC_SEARCH_SERVICE,
    BTIF_GATTC_READ_CHAR,
    BTIF_GATTC_READ_CHAR_DESCR,
    BTIF_GATTC_WRITE_CHAR,
    BTIF_GATTC_WRITE_CHAR_DESCR,
    BTIF_GATTC_EXECUTE_WRITE,
    BTIF_GATTC_REG_FOR_NOTIFICATION,
    BTIF_GATTC_DEREG_FOR_NOTIFICATION,
    BTIF_GATTC_REFRESH,
    BTIF_GATTC_READ_RSSI,
    BTIF_GATTC_LISTEN,
    BTIF_GATTC_SET_ADV_DATA,
    BTIF_GATTC_CONFIGURE_MTU,
    BTIF_GATTC_CONN_PARAM_UPDT,
    BTIF_GATTC_SCAN_FILTER_PARAM_SETUP,
    BTIF_GATTC_SCAN_FILTER_CONFIG,
    BTIF_GATTC_SCAN_FILTER_CLEAR,
    BTIF_GATTC_SCAN_FILTER_ENABLE,
    BTIF_GATTC_SET_SCAN_PARAMS,
    BTIF_GATTC_ADV_INSTANCE_ENABLE,
    BTIF_GATTC_ADV_INSTANCE_UPDATE,
    BTIF_GATTC_ADV_INSTANCE_SET_DATA,
    BTIF_GATTC_ADV_INSTANCE_DISABLE,
    BTIF_GATTC_CONFIG_STORAGE_PARAMS,
    BTIF_GATTC_ENABLE_BATCH_SCAN,
    BTIF_GATTC_READ_BATCH_SCAN_REPORTS,
    BTIF_GATTC_DISABLE_BATCH_SCAN,
    BTIF_GATTC_GET_GATT_DB
#if defined(MTK_LINUX_GATTC_LE_NAME) && (MTK_LINUX_GATTC_LE_NAME == TRUE)
    , BTIF_GATTC_SET_LOCAL_LE_NAME
#endif
#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
    , BTIF_GATTC_GET_ADV_RPA
#endif
#if defined(MTK_LINUX_GATTC_DISC_MODE) && (MTK_LINUX_GATTC_DISC_MODE == TRUE)
    ,BTIF_GATTC_SET_DISC_MODE
#endif
#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
    , BTIF_GATTC_READ_USING_CHAR_UUID,
    BTIF_GATTC_READ_LONG_CHAR,
    BTIF_GATTC_READ_MULTIPLE,
    BTIF_GATTC_READ_LONG_CHAR_DESCR,
    BTIF_GATTC_WRITE_LONG_CHAR,
    BTIF_GATTC_WRITE_LONG_CHAR_DESCR,
    BTIF_GATTC_SET_PTS_FLAG
#endif
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    , BTIF_GATTC_SET_PREFERRED_PHY,
    BTIF_GATTC_READ_PHY,
    BTIF_GATTC_SET_ADV_EXT_PARAM,
    BTIF_GATTC_START_ADVERTISING_SET,
    BTIF_GATTC_STOP_ADVERTISING_SET,
    BTIF_GATTC_SET_PERIODIC_PARAM,
    BTIF_GATTC_PERIODIC_ADV_ENABLE,
    BTIF_GATTC_SET_ADVERTISING_DATA,
    BTIF_GATTC_SET_ADVERTISING_PARAM,
#endif
} btif_gattc_event_t;

#define BTIF_GATT_MAX_OBSERVED_DEV 40

#define BTIF_GATT_OBSERVE_EVT   0x1000
#define BTIF_GATTC_RSSI_EVT     0x1001
#define BTIF_GATTC_SCAN_FILTER_EVT  0x1003
#define BTIF_GATTC_SCAN_PARAM_EVT   0x1004

#define ENABLE_BATCH_SCAN 1
#define DISABLE_BATCH_SCAN 0

/*******************************************************************************
**  Local type definitions
********************************************************************************/
typedef struct
{
    uint8_t report_format;
    uint16_t data_len;
    uint8_t num_records;
    uint8_t *p_rep_data;
} btgatt_batch_reports;

typedef struct
{
    uint8_t  status;
    uint8_t  client_if;
    uint8_t  action;
    uint8_t  avbl_space;
    uint8_t  lost_timeout;
    tBLE_ADDR_TYPE addr_type;
    uint8_t  batch_scan_full_max;
    uint8_t  batch_scan_trunc_max;
    uint8_t  batch_scan_notify_threshold;
    tBTA_BLE_BATCH_SCAN_MODE scan_mode;
    uint32_t scan_interval;
    uint32_t scan_window;
    tBTA_BLE_DISCARD_RULE discard_rule;
    btgatt_batch_reports  read_reports;
} btgatt_batch_track_cb_t;

typedef tBTA_DM_BLE_PF_FILT_PARAMS btgatt_adv_filt_param_t;

typedef struct
{
    uint8_t     client_if;
    uint8_t     action;
    tBTA_DM_BLE_PF_COND_TYPE filt_type;
    bt_bdaddr_t bd_addr;
    uint8_t     value[BTGATT_MAX_ATTR_LEN];
    uint8_t     value_len;
    uint8_t     filt_index;
    uint16_t    conn_id;
    uint16_t    company_id_mask;
    bt_uuid_t   uuid;
    bt_uuid_t   uuid_mask;
    uint8_t     value_mask[BTGATT_MAX_ATTR_LEN];
    uint8_t     value_mask_len;
    uint8_t     has_mask;
    uint8_t     addr_type;
    uint8_t     status;
    tBTA_DM_BLE_PF_AVBL_SPACE avbl_space;
    tBTA_DM_BLE_SCAN_COND_OP cond_op;
    btgatt_adv_filt_param_t adv_filt_param;
} btgatt_adv_filter_cb_t;

typedef struct
{
    uint8_t     value[BTGATT_MAX_ATTR_LEN];
    uint8_t     inst_id;
    bt_bdaddr_t bd_addr;
    btgatt_srvc_id_t srvc_id;
    btgatt_srvc_id_t incl_srvc_id;
    btgatt_gatt_id_t char_id;
    btgatt_gatt_id_t descr_id;
    uint16_t    handle;
    bt_uuid_t   uuid;
    bt_uuid_t   uuid_mask;
    uint16_t    conn_id;
    uint16_t    len;
    uint16_t    mask;
    uint32_t    scan_interval;
    uint32_t    scan_window;
    uint8_t     client_if;
    uint8_t     action;
    uint8_t     is_direct;
    uint8_t     search_all;
    uint8_t     auth_req;
    uint8_t     write_type;
    uint8_t     status;
    uint8_t     addr_type;
    uint8_t     start;
    uint8_t     has_mask;
    int8_t      rssi;
    uint8_t     flag;
    tBT_DEVICE_TYPE device_type;
    btgatt_transport_t transport;
#if defined(MTK_LINUX_GATTC_LE_NAME) && (MTK_LINUX_GATTC_LE_NAME == TRUE)
    uint8_t     le_name[MTK_LINUX_GATT_MAX_LE_NAME_LEN + 1];
#endif
#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
    bt_bdaddr_t rpa;  //local rpa address
#endif
#if defined(MTK_LINUX_GATTC_DISC_MODE) && (MTK_LINUX_GATTC_DISC_MODE == TRUE)
    uint32_t disc_mode;
#endif
#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
    uint16_t    s_handle; //read using uuid
    uint16_t    e_handle;//read using uuid
    uint16_t    offset;  //read long char
    uint8_t     num_attr;  //read multiple char
    uint16_t    handles[BTA_GATTC_MULTI_MAX]; //read multiple char
    uint8_t     pts_flag;
#endif
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    uint8_t      tx_phy;
    uint8_t      rx_phy;
    uint16_t     phy_options;
    uint16_t     event_properties;
    uint8_t      primary_phy;
    uint8_t      secondary_phy;
    uint8_t      scan_req_notify_enable;
#endif
} __attribute__((packed)) btif_gattc_cb_t;

typedef struct
{
    bt_bdaddr_t bd_addr;
    uint16_t    min_interval;
    uint16_t    max_interval;
    uint16_t    timeout;
    uint16_t    latency;
} btif_conn_param_cb_t;

typedef struct
{
    bt_bdaddr_t bd_addr;
    BOOLEAN     in_use;
}__attribute__((packed)) btif_gattc_dev_t;

typedef struct
{
    btif_gattc_dev_t remote_dev[BTIF_GATT_MAX_OBSERVED_DEV];
    uint8_t            addr_type;
    uint8_t            next_storage_idx;
}__attribute__((packed)) btif_gattc_dev_cb_t;

/*******************************************************************************
**  Static variables
********************************************************************************/

extern const btgatt_callbacks_t *bt_gatt_callbacks;
static btif_gattc_dev_cb_t  btif_gattc_dev_cb;
static btif_gattc_dev_cb_t  *p_dev_cb = &btif_gattc_dev_cb;
static uint8_t rssi_request_client_if;

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
extern const btgatt_ex_callbacks_t *bt_gatt_ex_callbacks;
#endif

/*******************************************************************************
**  Static functions
********************************************************************************/
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
extern UINT8 bta_gattc_read_phy_callback(BD_ADDR bd_addr, uint8_t tx_phy, uint8_t rx_phy,
                                                  uint8_t status);
#endif

static bt_status_t btif_gattc_multi_adv_disable(int client_if);
static void btif_multi_adv_stop_cb(void *data)
{
    int client_if = PTR_TO_INT(data);
    btif_gattc_multi_adv_disable(client_if); // Does context switch
}

static btgattc_error_t btif_gattc_translate_btm_status(tBTM_STATUS status)
{
    switch(status)
    {
       case BTM_SUCCESS:
       case BTM_SUCCESS_NO_SECURITY:
            return BT_GATTC_COMMAND_SUCCESS;

       case BTM_CMD_STARTED:
            return BT_GATTC_COMMAND_STARTED;

       case BTM_BUSY:
            return BT_GATTC_COMMAND_BUSY;

       case BTM_CMD_STORED:
            return BT_GATTC_COMMAND_STORED;

       case BTM_NO_RESOURCES:
            return BT_GATTC_NO_RESOURCES;

       case BTM_MODE_UNSUPPORTED:
       case BTM_WRONG_MODE:
       case BTM_MODE4_LEVEL4_NOT_SUPPORTED:
            return BT_GATTC_MODE_UNSUPPORTED;

       case BTM_ILLEGAL_VALUE:
       case BTM_SCO_BAD_LENGTH:
            return BT_GATTC_ILLEGAL_VALUE;

       case BTM_UNKNOWN_ADDR:
            return BT_GATTC_UNKNOWN_ADDR;

       case BTM_DEVICE_TIMEOUT:
            return BT_GATTC_DEVICE_TIMEOUT;

       case BTM_FAILED_ON_SECURITY:
       case BTM_REPEATED_ATTEMPTS:
       case BTM_NOT_AUTHORIZED:
            return BT_GATTC_SECURITY_ERROR;

       case BTM_DEV_RESET:
       case BTM_ILLEGAL_ACTION:
            return BT_GATTC_INCORRECT_STATE;

       case BTM_BAD_VALUE_RET:
            return BT_GATTC_INVALID_CONTROLLER_OUTPUT;

       case BTM_DELAY_CHECK:
            return BT_GATTC_DELAYED_ENCRYPTION_CHECK;

       case BTM_ERR_PROCESSING:
       default:
          return BT_GATTC_ERR_PROCESSING;
    }
}

static void btapp_gattc_req_data(UINT16 event, char *p_dest, char *p_src)
{
    tBTA_GATTC *p_dest_data = (tBTA_GATTC*) p_dest;
    tBTA_GATTC *p_src_data = (tBTA_GATTC*) p_src;

    if (!p_src_data || !p_dest_data)
       return;

    // Copy basic structure first
    maybe_non_aligned_memcpy(p_dest_data, p_src_data, sizeof(*p_src_data));

    // Allocate buffer for request data if necessary
    switch (event)
    {
        case BTA_GATTC_READ_CHAR_EVT:
        case BTA_GATTC_READ_DESCR_EVT:

            if (p_src_data->read.p_value != NULL)
            {
                p_dest_data->read.p_value = osi_malloc(sizeof(tBTA_GATT_UNFMT));

                memcpy(p_dest_data->read.p_value, p_src_data->read.p_value,
                       sizeof(tBTA_GATT_UNFMT));

                // Allocate buffer for att value if necessary
                if (p_src_data->read.p_value->len > 0 &&
                    p_src_data->read.p_value->p_value != NULL) {
                    p_dest_data->read.p_value->p_value =
                        osi_malloc(p_src_data->read.p_value->len);
                    memcpy(p_dest_data->read.p_value->p_value,
                           p_src_data->read.p_value->p_value,
                           p_src_data->read.p_value->len);
                }
            } else {
                BTIF_TRACE_WARNING("%s :Src read.p_value ptr is NULL for event  0x%x",
                                    __FUNCTION__, event);
                p_dest_data->read.p_value = NULL;

            }
            break;

        default:
            break;
    }
}

static void btapp_gattc_free_req_data(UINT16 event, tBTA_GATTC *p_data)
{
    switch (event)
    {
        case BTA_GATTC_READ_CHAR_EVT:
        case BTA_GATTC_READ_DESCR_EVT:
            if (p_data != NULL && p_data->read.p_value != NULL)
            {
                if (p_data->read.p_value->len > 0)
                    osi_free_and_reset((void **)&p_data->read.p_value->p_value);

                osi_free_and_reset((void **)&p_data->read.p_value);
            }
            break;

        default:
            break;
    }
}

static void btif_gattc_init_dev_cb(void)
{
    memset(p_dev_cb, 0, sizeof(btif_gattc_dev_cb_t));
}

static void btif_gattc_add_remote_bdaddr (BD_ADDR p_bda, uint8_t addr_type)
{
    uint8_t i;
    for (i = 0; i < BTIF_GATT_MAX_OBSERVED_DEV; i++)
    {
        if (!p_dev_cb->remote_dev[i].in_use )
        {
            memcpy(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN);
            p_dev_cb->addr_type = addr_type;
            p_dev_cb->remote_dev[i].in_use = TRUE;
            LOG_VERBOSE(LOG_TAG, "%s device added idx=%d", __FUNCTION__, i  );
            break;
        }
    }

    if ( i == BTIF_GATT_MAX_OBSERVED_DEV)
    {
        i= p_dev_cb->next_storage_idx;
        memcpy(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN);
        p_dev_cb->addr_type = addr_type;
        p_dev_cb->remote_dev[i].in_use = TRUE;
        LOG_VERBOSE(LOG_TAG, "%s device overwrite idx=%d", __FUNCTION__, i  );
        p_dev_cb->next_storage_idx++;
        if (p_dev_cb->next_storage_idx >= BTIF_GATT_MAX_OBSERVED_DEV)
               p_dev_cb->next_storage_idx = 0;
    }
}

static BOOLEAN btif_gattc_find_bdaddr (BD_ADDR p_bda)
{
    uint8_t i;
    for (i = 0; i < BTIF_GATT_MAX_OBSERVED_DEV; i++)
    {
        if (p_dev_cb->remote_dev[i].in_use &&
            !memcmp(p_dev_cb->remote_dev[i].bd_addr.address, p_bda, BD_ADDR_LEN))
        {
            return TRUE;
        }
    }
    return FALSE;
}

static void btif_gattc_update_properties ( btif_gattc_cb_t *p_btif_cb )
{
    uint8_t remote_name_len;
    uint8_t *p_eir_remote_name=NULL;
    bt_bdname_t bdname;

    p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                         BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);

    if (p_eir_remote_name == NULL)
    {
        p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                BT_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
    }

    if (p_eir_remote_name)
    {
        memcpy(bdname.name, p_eir_remote_name, remote_name_len);
        bdname.name[remote_name_len]='\0';

        LOG_VERBOSE(LOG_TAG, "%s BLE device name=%s len=%d dev_type=%d", __FUNCTION__, bdname.name,
              remote_name_len, p_btif_cb->device_type  );
        btif_dm_update_ble_remote_properties( p_btif_cb->bd_addr.address,   bdname.name,
                                               p_btif_cb->device_type);
    }
}

static void btif_gattc_upstreams_evt(uint16_t event, char* p_param)
{
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE) && defined(MTK_LINUX)
    BTIF_TRACE_DEBUG(LOG_TAG, "%s: Event %d", __FUNCTION__, event);
#else
    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);
#endif

    tBTA_GATTC *p_data = (tBTA_GATTC*) p_param;
    switch (event)
    {
        case BTA_GATTC_REG_EVT:
        {
            bt_uuid_t app_uuid;
            bta_to_btif_uuid(&app_uuid, &p_data->reg_oper.app_uuid);
            HAL_CBACK(bt_gatt_callbacks, client->register_client_cb
                , p_data->reg_oper.status
                , p_data->reg_oper.client_if
                , &app_uuid
            );
            break;
        }

        case BTA_GATTC_DEREG_EVT:
            break;

        case BTA_GATTC_READ_CHAR_EVT:
        {
            btgatt_read_params_t data;
            set_read_value(&data, &p_data->read);

            /* add for GATTC queue begin */
            mark_as_not_executing(p_data->read.conn_id);
            gatt_execute_next_op(p_data->read.conn_id);
            /* add for GATTC queue end */

            HAL_CBACK(bt_gatt_callbacks, client->read_characteristic_cb
                , p_data->read.conn_id, p_data->read.status, &data);
            break;
        }

        case BTA_GATTC_WRITE_CHAR_EVT:
        case BTA_GATTC_PREP_WRITE_EVT:
        {
            /* add for GATTC queue begin */
            mark_as_not_executing(p_data->write.conn_id);
            gatt_execute_next_op(p_data->write.conn_id);
            /* add for GATTC queue end */

            HAL_CBACK(bt_gatt_callbacks, client->write_characteristic_cb,
                p_data->write.conn_id, p_data->write.status, p_data->write.handle);
            break;
        }

        case BTA_GATTC_EXEC_EVT:
        {
            HAL_CBACK(bt_gatt_callbacks, client->execute_write_cb
                , p_data->exec_cmpl.conn_id, p_data->exec_cmpl.status
            );
            break;
        }

        case BTA_GATTC_SEARCH_CMPL_EVT:
        {
            HAL_CBACK(bt_gatt_callbacks, client->search_complete_cb
                , p_data->search_cmpl.conn_id, p_data->search_cmpl.status);
            break;
        }

        case BTA_GATTC_SEARCH_RES_EVT:
        {
            /* Do nothing */
            break;
        }

        case BTA_GATTC_READ_DESCR_EVT:
        {
            btgatt_read_params_t data;
            set_read_value(&data, &p_data->read);

            /* add for GATTC queue begin */
            mark_as_not_executing(p_data->read.conn_id);
            gatt_execute_next_op(p_data->read.conn_id);
            /* add for GATTC queue end */

            HAL_CBACK(bt_gatt_callbacks, client->read_descriptor_cb
                , p_data->read.conn_id, p_data->read.status, &data);
            break;
        }

        case BTA_GATTC_WRITE_DESCR_EVT:
        {
            /* add for GATTC queue begin */
            mark_as_not_executing(p_data->write.conn_id);
            gatt_execute_next_op(p_data->write.conn_id);
            /* add for GATTC queue end */

            HAL_CBACK(bt_gatt_callbacks, client->write_descriptor_cb,
                p_data->write.conn_id, p_data->write.status, p_data->write.handle);
            break;
        }

        case BTA_GATTC_NOTIF_EVT:
        {
            btgatt_notify_params_t data;

            bdcpy(data.bda.address, p_data->notify.bda);
            memcpy(data.value, p_data->notify.value, p_data->notify.len);

            data.handle = p_data->notify.handle;
            data.is_notify = p_data->notify.is_notify;
            data.len = p_data->notify.len;

            HAL_CBACK(bt_gatt_callbacks, client->notify_cb, p_data->notify.conn_id, &data);

            if (p_data->notify.is_notify == FALSE)
                BTA_GATTC_SendIndConfirm(p_data->notify.conn_id, p_data->notify.handle);

            break;
        }

        case BTA_GATTC_OPEN_EVT:
        {
            bt_bdaddr_t bda;
            bdcpy(bda.address, p_data->open.remote_bda);

            /* add for GATTC queue begin */
            if (p_data->open.status == BTA_GATT_OK)
            {
                gatt_op_queue_clean(p_data->open.conn_id);
            }
            /* add for GATTC queue end */

            HAL_CBACK(bt_gatt_callbacks, client->open_cb, p_data->open.conn_id
                , p_data->open.status, p_data->open.client_if, &bda);

            if (GATT_DEF_BLE_MTU_SIZE != p_data->open.mtu && p_data->open.mtu)
            {
                HAL_CBACK(bt_gatt_callbacks, client->configure_mtu_cb, p_data->open.conn_id
                    , p_data->open.status , p_data->open.mtu);
            }

            if (p_data->open.status == BTA_GATT_OK)
                btif_gatt_check_encrypted_link(p_data->open.remote_bda, p_data->open.transport);
            break;
        }

        case BTA_GATTC_CLOSE_EVT:
        {
            bt_bdaddr_t bda;
            bdcpy(bda.address, p_data->close.remote_bda);
            HAL_CBACK(bt_gatt_callbacks, client->close_cb, p_data->close.conn_id
                , p_data->status, p_data->close.client_if, &bda);
            break;
        }

        case BTA_GATTC_ACL_EVT:
            LOG_DEBUG(LOG_TAG, "BTA_GATTC_ACL_EVT: status = %d", p_data->status);
            /* Ignore for now */
            break;

        case BTA_GATTC_CANCEL_OPEN_EVT:
            break;

        case BTIF_GATT_OBSERVE_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            uint8_t remote_name_len;
            uint8_t *p_eir_remote_name=NULL;
            bt_device_type_t dev_type;
            bt_property_t properties;

            p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                         BTM_EIR_COMPLETE_LOCAL_NAME_TYPE, &remote_name_len);

            if (p_eir_remote_name == NULL)
            {
                p_eir_remote_name = BTM_CheckEirData(p_btif_cb->value,
                                BT_EIR_SHORTENED_LOCAL_NAME_TYPE, &remote_name_len);
            }

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            /*modified for no name issue(get directed packet first), change "||" to "&&" */
            if ((p_btif_cb->addr_type != BLE_ADDR_RANDOM) && (p_eir_remote_name))
#else
            if ((p_btif_cb->addr_type != BLE_ADDR_RANDOM) || (p_eir_remote_name))
#endif
            {
               if (!btif_gattc_find_bdaddr(p_btif_cb->bd_addr.address))
               {
                  btif_gattc_add_remote_bdaddr(p_btif_cb->bd_addr.address, p_btif_cb->addr_type);
                  btif_gattc_update_properties(p_btif_cb);
               }
            }

             dev_type =  p_btif_cb->device_type;
             BTIF_STORAGE_FILL_PROPERTY(&properties,
                        BT_PROPERTY_TYPE_OF_DEVICE, sizeof(dev_type), &dev_type);
             btif_storage_set_remote_device_property(&(p_btif_cb->bd_addr), &properties);

            btif_storage_set_remote_addr_type( &p_btif_cb->bd_addr, p_btif_cb->addr_type);

            HAL_CBACK(bt_gatt_callbacks, client->scan_result_cb,
                      &p_btif_cb->bd_addr, p_btif_cb->rssi, p_btif_cb->value);
            break;
        }

        case BTIF_GATTC_RSSI_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->read_remote_rssi_cb, p_btif_cb->client_if,
                      &p_btif_cb->bd_addr, p_btif_cb->rssi, p_btif_cb->status);
            break;
        }

        case BTA_GATTC_LISTEN_EVT:
        {
            HAL_CBACK(bt_gatt_callbacks, client->listen_cb
                , p_data->reg_oper.status
                , p_data->reg_oper.client_if
            );
            break;
        }

        case BTA_GATTC_CFG_MTU_EVT:
        {
            HAL_CBACK(bt_gatt_callbacks, client->configure_mtu_cb, p_data->cfg_mtu.conn_id
                , p_data->cfg_mtu.status , p_data->cfg_mtu.mtu);
            break;
        }

        case BTA_GATTC_MULT_ADV_ENB_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            if (0xFF != p_btif_cb->inst_id)
                btif_multi_adv_add_instid_map(p_btif_cb->client_if, p_btif_cb->inst_id, false);
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_enable_cb
                    , p_btif_cb->client_if
                    , p_btif_cb->status
                );
            btif_multi_adv_timer_ctrl(p_btif_cb->client_if,
                                      (p_btif_cb->status == BTA_GATT_OK) ?
                                      btif_multi_adv_stop_cb : NULL);
            break;
        }

        case BTA_GATTC_MULT_ADV_UPD_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_update_cb
                , p_btif_cb->client_if
                , p_btif_cb->status
            );
            btif_multi_adv_timer_ctrl(p_btif_cb->client_if,
                                      (p_btif_cb->status == BTA_GATT_OK) ?
                                      btif_multi_adv_stop_cb : NULL);
            break;
        }

        case BTA_GATTC_MULT_ADV_DATA_EVT:
         {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            btif_gattc_clear_clientif(p_btif_cb->client_if, FALSE);
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_data_cb
                , p_btif_cb->client_if
                , p_btif_cb->status
            );
            break;
        }

        case BTA_GATTC_MULT_ADV_DIS_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            btif_gattc_clear_clientif(p_btif_cb->client_if, TRUE);
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_disable_cb
                , p_btif_cb->client_if
                , p_btif_cb->status
            );
            break;
        }
#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
        case BTA_GATTC_GET_ADV_RPA_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            LOG_DEBUG(LOG_TAG, "BTA_GATTC_GET_ADV_RPA_EVT: status = %d", p_btif_cb->status);
            HAL_CBACK(bt_gatt_ex_callbacks, ex_client->get_adv_rpa_cb
                , p_btif_cb->client_if
                , p_btif_cb->status
                , &p_btif_cb->rpa
            );
            break;
        }
#endif
        case BTA_GATTC_ADV_DATA_EVT:
        {
            btif_gattc_cleanup_inst_cb(STD_ADV_INSTID, FALSE);
            /* No HAL callback available */
            break;
        }

        case BTA_GATTC_CONGEST_EVT:
            HAL_CBACK(bt_gatt_callbacks, client->congestion_cb
                , p_data->congest.conn_id
                , p_data->congest.congested
            );
            break;

        case BTA_GATTC_BTH_SCAN_CFG_EVT:
        {
            btgatt_batch_track_cb_t *p_data = (btgatt_batch_track_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->batchscan_cfg_storage_cb
                , p_data->client_if
                , p_data->status
            );
            break;
        }

        case BTA_GATTC_BTH_SCAN_ENB_EVT:
        {
            btgatt_batch_track_cb_t *p_data = (btgatt_batch_track_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->batchscan_enb_disable_cb
                    , ENABLE_BATCH_SCAN
                    , p_data->client_if
                    , p_data->status);
            break;
        }

        case BTA_GATTC_BTH_SCAN_DIS_EVT:
        {
            btgatt_batch_track_cb_t *p_data = (btgatt_batch_track_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->batchscan_enb_disable_cb
                    , DISABLE_BATCH_SCAN
                    , p_data->client_if
                    , p_data->status);
            break;
        }

        case BTA_GATTC_BTH_SCAN_THR_EVT:
        {
            btgatt_batch_track_cb_t *p_data = (btgatt_batch_track_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->batchscan_threshold_cb
                    , p_data->client_if);
            break;
        }

        case BTA_GATTC_BTH_SCAN_RD_EVT:
        {
            btgatt_batch_track_cb_t *p_data = (btgatt_batch_track_cb_t*) p_param;
            uint8_t *p_rep_data = NULL;

            if (p_data->read_reports.data_len > 0 && NULL != p_data->read_reports.p_rep_data)
            {
                p_rep_data = osi_malloc(p_data->read_reports.data_len);
                memcpy(p_rep_data, p_data->read_reports.p_rep_data, p_data->read_reports.data_len);
            }

            HAL_CBACK(bt_gatt_callbacks, client->batchscan_reports_cb
                    , p_data->client_if, p_data->status, p_data->read_reports.report_format
                    , p_data->read_reports.num_records, p_data->read_reports.data_len, p_rep_data);
            osi_free(p_rep_data);
            break;
        }

        case BTA_GATTC_SCAN_FLT_CFG_EVT:
        {
            btgatt_adv_filter_cb_t *p_btif_cb = (btgatt_adv_filter_cb_t*) p_param;
            HAL_CBACK(bt_gatt_callbacks, client->scan_filter_cfg_cb, p_btif_cb->action,
                      p_btif_cb->client_if, p_btif_cb->status, p_btif_cb->cond_op,
                      p_btif_cb->avbl_space);
            break;
        }

        case BTA_GATTC_SCAN_FLT_PARAM_EVT:
        {
            btgatt_adv_filter_cb_t *p_data = (btgatt_adv_filter_cb_t*) p_param;
            BTIF_TRACE_DEBUG("BTA_GATTC_SCAN_FLT_PARAM_EVT: %d, %d, %d, %d",p_data->client_if,
                p_data->action, p_data->avbl_space, p_data->status);
            HAL_CBACK(bt_gatt_callbacks, client->scan_filter_param_cb
                    , p_data->action, p_data->client_if, p_data->status
                    , p_data->avbl_space);
            break;
        }

        case BTA_GATTC_SCAN_FLT_STATUS_EVT:
        {
            btgatt_adv_filter_cb_t *p_data = (btgatt_adv_filter_cb_t*) p_param;
            BTIF_TRACE_DEBUG("BTA_GATTC_SCAN_FLT_STATUS_EVT: %d, %d, %d",p_data->client_if,
                p_data->action, p_data->status);
            HAL_CBACK(bt_gatt_callbacks, client->scan_filter_status_cb
                    , p_data->action, p_data->client_if, p_data->status);
            break;
        }

        case BTA_GATTC_ADV_VSC_EVT:
        {
            btgatt_track_adv_info_t *p_data = (btgatt_track_adv_info_t*)p_param;
            btgatt_track_adv_info_t adv_info_data;

            memset(&adv_info_data, 0, sizeof(btgatt_track_adv_info_t));

            btif_gatt_move_track_adv_data(&adv_info_data, p_data);
            HAL_CBACK(bt_gatt_callbacks, client->track_adv_event_cb, &adv_info_data);
            break;
        }

        case BTIF_GATTC_SCAN_PARAM_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t *)p_param;
            HAL_CBACK(bt_gatt_callbacks, client->scan_parameter_setup_completed_cb,
                      p_btif_cb->client_if, btif_gattc_translate_btm_status(p_btif_cb->status));
            break;
        }
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
        case BTA_GATTC_PHY_UPDATE_EVT:
        {
            bta_gattc_read_phy_callback(p_data->phy_update.bd_addr,
                                        p_data->phy_update.tx_phy,
                                        p_data->phy_update.rx_phy,
                                        p_data->phy_update.status);
            break;
        }

        case BTA_GATTC_EXT_ADV_ENB_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            if (0xFF != p_btif_cb->inst_id)
                btif_multi_adv_add_instid_map(p_btif_cb->client_if, p_btif_cb->inst_id, false);
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_enable_cb
                    , p_btif_cb->client_if
                    , p_btif_cb->status
                );
            break;
        }

        case BTA_GATTC_EXT_ADV_DISABLE_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            //btif_gattc_clear_clientif(p_btif_cb->client_if, TRUE);
            HAL_CBACK(bt_gatt_callbacks, client->multi_adv_disable_cb
                , p_btif_cb->client_if
                , p_btif_cb->status
            );
            break;
        }

        case BTA_GATTC_STOP_EXT_ADV_SET_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            btif_gattc_clear_clientif(p_btif_cb->client_if, TRUE);
            break;
        }

        case BTA_GATTC_PERI_ADV_ENB_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            BTIF_TRACE_DEBUG("BTA_GATTC_PERI_ADV_ENB_EVT: client=%d, status=%d, inst=%d",
                            p_btif_cb->client_if, p_btif_cb->status, p_btif_cb->inst_id);
            if (0xFF != p_btif_cb->inst_id)
            {
                btif_multi_adv_add_instid_map(p_btif_cb->client_if, p_btif_cb->inst_id, false);
            }

            HAL_CBACK(bt_gatt_ex_callbacks, ex_client->peri_adv_enable_cb,
                    p_btif_cb->client_if, p_btif_cb->status, 1);
            break;
        }

        case BTA_GATTC_PERI_ADV_DISABLE_EVT:
        {
            btif_gattc_cb_t *p_btif_cb = (btif_gattc_cb_t*) p_param;
            BTIF_TRACE_DEBUG("BTA_GATTC_PERI_ADV_DISABLE_EVT: client=%d, status=%d",
                            p_btif_cb->client_if, p_btif_cb->status);
            btif_gattc_clear_clientif(p_btif_cb->client_if, TRUE);

            HAL_CBACK(bt_gatt_ex_callbacks, ex_client->peri_adv_enable_cb,
                        p_btif_cb->client_if, p_btif_cb->status, 0);
            break;
        }
#endif
        default:
            LOG_ERROR(LOG_TAG, "%s: Unhandled event (%d)!", __FUNCTION__, event);
            break;
    }

    btapp_gattc_free_req_data(event, p_data);
}

static void bta_gattc_cback(tBTA_GATTC_EVT event, tBTA_GATTC *p_data)
{
    bt_status_t status = btif_transfer_context(btif_gattc_upstreams_evt,
                    (uint16_t) event, (void*) p_data, sizeof(tBTA_GATTC), btapp_gattc_req_data);
    ASSERTC(status == BT_STATUS_SUCCESS, "Context transfer failed!", status);
}

static void bta_gattc_multi_adv_cback(tBTA_BLE_MULTI_ADV_EVT event, UINT8 inst_id,
                                    void *p_ref, tBTA_STATUS call_status)
{
    btif_gattc_cb_t btif_cb;
    tBTA_GATTC_EVT upevt;
    uint8_t client_if = 0;

    if (NULL == p_ref)
    {
        BTIF_TRACE_WARNING("%s Invalid p_ref received",__FUNCTION__);
    }
    else
    {
        client_if = *(UINT8 *) p_ref;
    }

    BTIF_TRACE_DEBUG("%s -Inst ID %d, Status:%x, client_if:%d",__FUNCTION__,inst_id, call_status,
                       client_if);
    btif_cb.status = call_status;
    btif_cb.client_if = client_if;
    btif_cb.inst_id = inst_id;

    switch(event)
    {
        case BTA_BLE_MULTI_ADV_ENB_EVT:
            upevt = BTA_GATTC_MULT_ADV_ENB_EVT;
            break;

        case BTA_BLE_MULTI_ADV_DISABLE_EVT:
            upevt = BTA_GATTC_MULT_ADV_DIS_EVT;
            break;

        case BTA_BLE_MULTI_ADV_PARAM_EVT:
            upevt = BTA_GATTC_MULT_ADV_UPD_EVT;
            break;

        case BTA_BLE_MULTI_ADV_DATA_EVT:
            upevt = BTA_GATTC_MULT_ADV_DATA_EVT;
            break;
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
        case BTA_BLE_ADV_PERI_ENABLE_EVT:
            upevt = BTA_GATTC_PERI_ADV_ENB_EVT;
            break;

        case BTM_BLE_ADV_PERI_DISABLE_EVT:
            upevt = BTA_GATTC_PERI_ADV_DISABLE_EVT;
            break;

        case BTM_BLE_EXT_ADV_ENB_EVT:
            upevt = BTA_GATTC_EXT_ADV_ENB_EVT;
            break;

        case BTM_BLE_EXT_ADV_DISABLE_EVT:
            upevt = BTA_GATTC_EXT_ADV_DISABLE_EVT;
            break;

        case BTA_BLE_REMOVE_ADVERTISING_SET_EVT:
            upevt = BTA_GATTC_STOP_EXT_ADV_SET_EVT;
            break;
#endif
        default:
            return;
    }

    bt_status_t status = btif_transfer_context(btif_gattc_upstreams_evt, (uint16_t) upevt,
                        (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
    ASSERTC(status == BT_STATUS_SUCCESS, "Context transfer failed!", status);
}

#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
static void bta_gattc_get_adv_rpa_cback(tBTA_BLE_GET_ADV_RPA_EVT event, UINT8 inst_id,
                                    bt_bdaddr_t *rpa, void *p_ref, tBTA_STATUS call_status)
{
    btif_gattc_cb_t btif_cb;
    tBTA_GATTC_EVT upevt;
    uint8_t client_if = 0;

    if (NULL == p_ref)
    {
        BTIF_TRACE_WARNING("%s Invalid p_ref received",__FUNCTION__);
    }
    else
    {
        client_if = *(UINT8 *) p_ref;
    }

    BTIF_TRACE_DEBUG("%s -Inst ID %d, Status:%x, client_if:%d",__FUNCTION__,inst_id, call_status,
                       client_if);
    btif_cb.status = call_status;
    btif_cb.client_if = client_if;
    btif_cb.inst_id = inst_id;
    bdcpy(btif_cb.rpa.address, rpa->address);
    BTIF_TRACE_DEBUG("%s, address = %02X:%02X:%02X:%02X:%02X:%02X",__func__,
            btif_cb.rpa.address[0], btif_cb.rpa.address[1], btif_cb.rpa.address[2],
            btif_cb.rpa.address[3], btif_cb.rpa.address[4], btif_cb.rpa.address[5]);
    if (BTA_BLE_GET_ADV_RPA_EVT == event)
    {
        upevt = BTA_GATTC_GET_ADV_RPA_EVT;

        bt_status_t status = btif_transfer_context(btif_gattc_upstreams_evt, (uint16_t) upevt,
                        (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
        ASSERTC(status == BT_STATUS_SUCCESS, "Context transfer failed!", status);
    }
}
#endif

static void bta_gattc_set_adv_data_cback(tBTA_STATUS call_status)
{
    UNUSED(call_status);
    btif_gattc_cb_t btif_cb;
    btif_cb.status = call_status;
    btif_cb.action = 0;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_ADV_DATA_EVT,
                          (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_batch_scan_setup_cb (tBTA_BLE_BATCH_SCAN_EVT evt,
                                            tBTA_DM_BLE_REF_VALUE ref_value, tBTA_STATUS status)
{
    UINT8 upevt = 0;
    btgatt_batch_track_cb_t btif_scan_track_cb;

    btif_scan_track_cb.status = status;
    btif_scan_track_cb.client_if = ref_value;
    BTIF_TRACE_DEBUG("bta_batch_scan_setup_cb-Status:%x, client_if:%d, evt=%d",
            status, ref_value, evt);

    switch(evt)
    {
        case BTA_BLE_BATCH_SCAN_ENB_EVT:
        {
           upevt = BTA_GATTC_BTH_SCAN_ENB_EVT;
           break;
        }

        case BTA_BLE_BATCH_SCAN_DIS_EVT:
        {
           upevt = BTA_GATTC_BTH_SCAN_DIS_EVT;
           break;
        }

        case BTA_BLE_BATCH_SCAN_CFG_STRG_EVT:
        {
           upevt = BTA_GATTC_BTH_SCAN_CFG_EVT;
           break;
        }

        case BTA_BLE_BATCH_SCAN_DATA_EVT:
        {
           upevt = BTA_GATTC_BTH_SCAN_RD_EVT;
           break;
        }

        case BTA_BLE_BATCH_SCAN_THRES_EVT:
        {
           upevt = BTA_GATTC_BTH_SCAN_THR_EVT;
           break;
        }

        default:
            return;
    }

    btif_transfer_context(btif_gattc_upstreams_evt, upevt,(char*) &btif_scan_track_cb,
                          sizeof(btgatt_batch_track_cb_t), NULL);

}

static void bta_batch_scan_threshold_cb(tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_batch_track_cb_t btif_scan_track_cb;
    btif_scan_track_cb.status = 0;
    btif_scan_track_cb.client_if = ref_value;

    BTIF_TRACE_DEBUG("%s - client_if:%d",__FUNCTION__, ref_value);

    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_BTH_SCAN_THR_EVT,
                          (char*) &btif_scan_track_cb, sizeof(btgatt_batch_track_cb_t), NULL);
}

static void bta_batch_scan_reports_cb(tBTA_DM_BLE_REF_VALUE ref_value, UINT8 report_format,
                                            UINT8 num_records, UINT16 data_len,
                                            UINT8* p_rep_data, tBTA_STATUS status)
{
    btgatt_batch_track_cb_t btif_scan_track_cb;
    memset(&btif_scan_track_cb, 0, sizeof(btgatt_batch_track_cb_t));
    BTIF_TRACE_DEBUG("%s - client_if:%d, %d, %d, %d",__FUNCTION__, ref_value, status, num_records,
                                    data_len);

    btif_scan_track_cb.status = status;

    btif_scan_track_cb.client_if = ref_value;
    btif_scan_track_cb.read_reports.report_format = report_format;
    btif_scan_track_cb.read_reports.data_len = data_len;
    btif_scan_track_cb.read_reports.num_records = num_records;

    if (data_len > 0)
    {
        btif_scan_track_cb.read_reports.p_rep_data = osi_malloc(data_len);
        memcpy(btif_scan_track_cb.read_reports.p_rep_data, p_rep_data, data_len);
        osi_free(p_rep_data);
    }

    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_BTH_SCAN_RD_EVT,
        (char*) &btif_scan_track_cb, sizeof(btgatt_batch_track_cb_t), NULL);

    if (data_len > 0)
        osi_free_and_reset((void **)&btif_scan_track_cb.read_reports.p_rep_data);
}

static void bta_scan_results_cb (tBTA_DM_SEARCH_EVT event, tBTA_DM_SEARCH *p_data)
{
    btif_gattc_cb_t btif_cb;
    uint8_t len;

    switch (event)
    {
        case BTA_DM_INQ_RES_EVT:
        {
            bdcpy(btif_cb.bd_addr.address, p_data->inq_res.bd_addr);
            btif_cb.device_type = p_data->inq_res.device_type;
            btif_cb.rssi = p_data->inq_res.rssi;
            btif_cb.addr_type = p_data->inq_res.ble_addr_type;
            btif_cb.flag = p_data->inq_res.flag;
            if (p_data->inq_res.p_eir)
            {
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
                memcpy(btif_cb.value, p_data->inq_res.p_eir, BLE_ADV_DATA_MAX);
#else
                memcpy(btif_cb.value, p_data->inq_res.p_eir, 62);
#endif
                if (BTM_CheckEirData(p_data->inq_res.p_eir, BTM_EIR_COMPLETE_LOCAL_NAME_TYPE,
                                      &len))
                {
                    p_data->inq_res.remt_name_not_required  = TRUE;
                }
            }
        }
        break;

        case BTA_DM_INQ_CMPL_EVT:
        {
            BTIF_TRACE_DEBUG("%s  BLE observe complete. Num Resp %d",
                              __FUNCTION__,p_data->inq_cmpl.num_resps);
            return;
        }

        default:
        BTIF_TRACE_WARNING("%s : Unknown event 0x%x", __FUNCTION__, event);
        return;
    }
/* update COD for GATT OBSERVER */
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#if defined(RC_COD_UPDATE_IN_OBSERVE) && (RC_COD_UPDATE_IN_OBSERVE == TRUE)
    {
        uint8_t num_properties = 0;
        bt_property_t properties[3];
        bt_bdaddr_t bdaddr;
        bt_status_t status;
        uint32_t cod;

        memset(properties, 0, sizeof(properties));
        bdcpy(bdaddr.address, p_data->inq_res.bd_addr);

        cod = devclass2uint(p_data->inq_res.dev_class);
        BTIF_TRACE_DEBUG("%s cod is 0x%06x", __func__, cod);
        if (cod == 0) {
            /* Try to retrieve cod from storage */
            BTIF_TRACE_DEBUG("%s cod is 0, checking cod from storage", __func__);
            BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                        BT_PROPERTY_CLASS_OF_DEVICE, sizeof(uint32_t), &cod);
            status = btif_storage_get_remote_device_property(&bdaddr, &properties[num_properties]);
            BTIF_TRACE_DEBUG("%s cod retrieved from storage is 0x%06x", __func__, cod);
            if (cod == 0) {
                BTIF_TRACE_DEBUG("%s cod is again 0, set as unclassified", __func__);
                cod = 0x1f00;
            }
        }

        BTIF_STORAGE_FILL_PROPERTY(&properties[num_properties],
                                    BT_PROPERTY_CLASS_OF_DEVICE, sizeof(uint32_t), &cod);
        status = btif_storage_set_remote_device_property(&bdaddr, &properties[num_properties]);
        ASSERTC(status == BT_STATUS_SUCCESS, "failed to save remote device class", status);
        num_properties++;

    }
#endif
#endif
    btif_transfer_context(btif_gattc_upstreams_evt, BTIF_GATT_OBSERVE_EVT,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_track_adv_event_cb(tBTA_DM_BLE_TRACK_ADV_DATA *p_track_adv_data)
{
    btgatt_track_adv_info_t btif_scan_track_cb;
    BTIF_TRACE_DEBUG("%s",__FUNCTION__);
    btif_gatt_move_track_adv_data(&btif_scan_track_cb,
                (btgatt_track_adv_info_t*)p_track_adv_data);

    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_ADV_VSC_EVT,
                          (char*) &btif_scan_track_cb, sizeof(btgatt_track_adv_info_t), NULL);
}

static void btm_read_rssi_cb (tBTM_RSSI_RESULTS *p_result)
{
    if (!p_result)
      return;

    btif_gattc_cb_t btif_cb;

    bdcpy(btif_cb.bd_addr.address, p_result->rem_bda);
    btif_cb.rssi = p_result->rssi;
    btif_cb.status = p_result->status;
    btif_cb.client_if = rssi_request_client_if;
    btif_transfer_context(btif_gattc_upstreams_evt, BTIF_GATTC_RSSI_EVT,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_scan_param_setup_cb(tGATT_IF client_if, tBTM_STATUS status)
{
    btif_gattc_cb_t btif_cb;

    btif_cb.status = status;
    btif_cb.client_if = client_if;
    btif_transfer_context(btif_gattc_upstreams_evt, BTIF_GATTC_SCAN_PARAM_EVT,
                          (char *)&btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void bta_scan_filt_cfg_cb(tBTA_DM_BLE_PF_ACTION action, tBTA_DM_BLE_SCAN_COND_OP cfg_op,
                                tBTA_DM_BLE_PF_AVBL_SPACE avbl_space, tBTA_STATUS status,
                                tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_adv_filter_cb_t btif_cb;
    btif_cb.status = status;
    btif_cb.action = action;
    btif_cb.cond_op = cfg_op;
    btif_cb.avbl_space = avbl_space;
    btif_cb.client_if = ref_value;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_CFG_EVT,
                          (char*) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static void bta_scan_filt_param_setup_cb(UINT8 action_type,
                                        tBTA_DM_BLE_PF_AVBL_SPACE avbl_space,
                                        tBTA_DM_BLE_REF_VALUE ref_value, tBTA_STATUS status)
{
    btgatt_adv_filter_cb_t btif_cb;

    btif_cb.status = status;
    btif_cb.action = action_type;
    btif_cb.client_if = ref_value;
    btif_cb.avbl_space = avbl_space;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_PARAM_EVT,
                          (char*) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static void bta_scan_filt_status_cb(UINT8 action, tBTA_STATUS status,
                                    tBTA_DM_BLE_REF_VALUE ref_value)
{
    btgatt_adv_filter_cb_t btif_cb;

    btif_cb.status = status;
    btif_cb.action = action;
    btif_cb.client_if = ref_value;
    btif_transfer_context(btif_gattc_upstreams_evt, BTA_GATTC_SCAN_FLT_STATUS_EVT,
                          (char*) &btif_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static void btgattc_free_event_data(UINT16 event, char *event_data)
{
    switch (event)
    {
        case BTIF_GATTC_ADV_INSTANCE_SET_DATA:
        case BTIF_GATTC_SET_ADV_DATA:
        {
            btif_adv_data_t *adv_data = (btif_adv_data_t *)event_data;
            btif_gattc_adv_data_cleanup(adv_data);
            break;
        }

        default:
            break;
    }
}

static void btgattc_handle_event(uint16_t event, char* p_param)
{
    tBTA_GATT_STATUS           status;
    tBT_UUID                   uuid;
    tBTA_GATT_UNFMT            descr_val;

#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
    tBTA_GATTC_MULTI           read_multi;
#endif

    btif_gattc_cb_t* p_cb = (btif_gattc_cb_t*) p_param;
    if (!p_cb) return;

    LOG_VERBOSE(LOG_TAG, "%s: Event %d", __FUNCTION__, event);

    switch (event)
    {
        case BTIF_GATTC_REGISTER_APP:
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            btif_gattc_incr_app_count();
            BTA_GATTC_AppRegister(&uuid, bta_gattc_cback);
            break;

        case BTIF_GATTC_UNREGISTER_APP:
            btif_gattc_clear_clientif(p_cb->client_if, TRUE);
            btif_gattc_decr_app_count();
            BTA_GATTC_AppDeregister(p_cb->client_if);
            break;

        case BTIF_GATTC_SCAN_START:
            btif_gattc_init_dev_cb();
            BTA_DmBleObserve(TRUE, 0, bta_scan_results_cb);
            break;

        case BTIF_GATTC_SCAN_STOP:
            BTA_DmBleObserve(FALSE, 0, 0);
            break;

        case BTIF_GATTC_OPEN:
        {
            // Ensure device is in inquiry database
            int addr_type = 0;
            int device_type = 0;
            tBTA_GATT_TRANSPORT transport = BTA_GATT_TRANSPORT_LE;

            if (btif_get_address_type(p_cb->bd_addr.address, &addr_type) &&
                btif_get_device_type(p_cb->bd_addr.address, &device_type) &&
                device_type != BT_DEVICE_TYPE_BREDR)
            {
                BTA_DmAddBleDevice(p_cb->bd_addr.address, addr_type, device_type);
            }

            // Check for background connections
            if (!p_cb->is_direct)
            {
                // Check for privacy 1.0 and 1.1 controller and do not start background
                // connection if RPA offloading is not supported, since it will not
                // connect after change of random address
                if (!controller_get_interface()->supports_ble_privacy() &&
                   (p_cb->addr_type == BLE_ADDR_RANDOM) &&
                   BTM_BLE_IS_RESOLVE_BDA(p_cb->bd_addr.address))
                {
                    tBTM_BLE_VSC_CB vnd_capabilities;
                    BTM_BleGetVendorCapabilities(&vnd_capabilities);
                    if (!vnd_capabilities.rpa_offloading)
                    {
                        HAL_CBACK(bt_gatt_callbacks, client->open_cb, 0, BT_STATUS_UNSUPPORTED,
                                        p_cb->client_if, &p_cb->bd_addr);
                        return;
                    }
                }
                BTA_DmBleSetBgConnType(BTM_BLE_CONN_AUTO, NULL);
            }

            // Determine transport
            if (p_cb->transport != GATT_TRANSPORT_AUTO)
            {
                transport = p_cb->transport;
            } else {
                switch(device_type)
                {
                    case BT_DEVICE_TYPE_BREDR:
                        transport = BTA_GATT_TRANSPORT_BR_EDR;
                        break;

                    case BT_DEVICE_TYPE_BLE:
                        transport = BTA_GATT_TRANSPORT_LE;
                        break;

                    case BT_DEVICE_TYPE_DUMO:
                        if (p_cb->transport == GATT_TRANSPORT_LE)
                            transport = BTA_GATT_TRANSPORT_LE;
                        else
                            transport = BTA_GATT_TRANSPORT_BR_EDR;
                        break;
                }
            }

            // Connect!
            BTIF_TRACE_DEBUG ("%s Transport=%d, device type=%d",
                                __func__, transport, device_type);
            BTA_GATTC_Open(p_cb->client_if, p_cb->bd_addr.address, p_cb->is_direct, transport);
            break;
        }

        case BTIF_GATTC_CLOSE:
            /* add for GATTC queue begin */
            gatt_op_queue_clean(p_cb->conn_id);
            /* add for GATTC queue end */

            // Disconnect established connections
            if (p_cb->conn_id != 0)
                BTA_GATTC_Close(p_cb->conn_id);
            else
                BTA_GATTC_CancelOpen(p_cb->client_if, p_cb->bd_addr.address, TRUE);

            // Cancel pending background connections (remove from whitelist)
            BTA_GATTC_CancelOpen(p_cb->client_if, p_cb->bd_addr.address, FALSE);
            break;

        case BTIF_GATTC_SEARCH_SERVICE:
        {
            if (p_cb->search_all)
            {
                BTA_GATTC_ServiceSearchRequest(p_cb->conn_id, NULL);
            } else {
                btif_to_bta_uuid(&uuid, &p_cb->uuid);
                BTA_GATTC_ServiceSearchRequest(p_cb->conn_id, &uuid);
            }
            break;
        }

        case BTIF_GATTC_GET_GATT_DB:
        {
            btgatt_db_element_t *db = NULL;
            int count = 0;
            BTA_GATTC_GetGattDb(p_cb->conn_id, 0x0000, 0xFFFF, &db, &count);

            HAL_CBACK(bt_gatt_callbacks, client->get_gatt_db_cb,
                p_cb->conn_id, db, count);
            osi_free(db);
            break;
        }
#if defined(MTK_LINUX_GATTC_LE_NAME) && (MTK_LINUX_GATTC_LE_NAME == TRUE)
        case BTIF_GATTC_SET_LOCAL_LE_NAME:
        {
            BTA_GATTC_SetLocalLeName((char *)p_cb->le_name);
            break;
        }
#endif
#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
        case BTIF_GATTC_GET_ADV_RPA:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            if (inst_id >=0)
            {
                BTA_BleGetAdvRPA((UINT8)inst_id, bta_gattc_get_adv_rpa_cback);
            }
            else
            {
                BTIF_TRACE_ERROR("%s invalid instance ID in BTIF_GATTC_GET_ADV_RPA",__FUNCTION__);
            }
            break;
        }
#endif
#if defined(MTK_LINUX_GATTC_DISC_MODE) && (MTK_LINUX_GATTC_DISC_MODE == TRUE)
        case BTIF_GATTC_SET_DISC_MODE:
        {
            BTA_GATTC_SetDiscMode((UINT32)p_cb->disc_mode);
            break;
        }
#endif
#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
        case BTIF_GATTC_READ_USING_CHAR_UUID:
            btif_to_bta_uuid(&uuid, &p_cb->uuid);
            BTA_GATTC_ReadUsingCharacteristicUUID(p_cb->conn_id, p_cb->s_handle,
                                                  p_cb->e_handle, &uuid,
                                                  p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_LONG_CHAR:
            BTA_GATTC_ReadLongCharacteristic(p_cb->conn_id, p_cb->handle, p_cb->offset,
                                              p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_MULTIPLE:
            read_multi.num_attr = p_cb->num_attr;
            if (read_multi.num_attr > 0)
            {
                memcpy(read_multi.handles, p_cb->handles, sizeof(UINT16) * read_multi.num_attr);
            }
            BTA_GATTC_ReadMultiple(p_cb->conn_id, &read_multi, p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_LONG_CHAR_DESCR:
            BTA_GATTC_ReadLongCharDescr(p_cb->conn_id, p_cb->handle, p_cb->offset,
                                              p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_LONG_CHAR:
            BTA_GATTC_WriteLongCharValue(p_cb->conn_id, p_cb->handle, p_cb->write_type,
                                     p_cb->len, p_cb->offset, p_cb->value, p_cb->auth_req);
            break;
        case BTIF_GATTC_WRITE_LONG_CHAR_DESCR:
            BTA_GATTC_WriteLongCharDescrValue(p_cb->conn_id, p_cb->handle, p_cb->write_type,
                                     p_cb->len, p_cb->offset, p_cb->value, p_cb->auth_req);
            break;
        case BTIF_GATTC_SET_PTS_FLAG:
            BTA_GATTC_SetPtsFlag(p_cb->pts_flag);
            break;
#endif

        /* modify for GATTC queue begin */
#if 1
        case BTIF_GATTC_READ_CHAR:
            gatt_queue_auth_read_op(GATT_READ_CHAR, p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_CHAR_DESCR:
            gatt_queue_auth_read_op(GATT_READ_DESC, p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR:
            gatt_queue_auth_write_op(GATT_WRITE_CHAR, p_cb->conn_id,
                                p_cb->handle, p_cb->len, p_cb->value,
                                p_cb->write_type, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR_DESCR:
            gatt_queue_auth_write_op(GATT_WRITE_DESC, p_cb->conn_id,
                                p_cb->handle, p_cb->len, p_cb->value,
                                p_cb->write_type, p_cb->auth_req);
            break;
#else
        case BTIF_GATTC_READ_CHAR:
            BTA_GATTC_ReadCharacteristic(p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_READ_CHAR_DESCR:
            BTA_GATTC_ReadCharDescr(p_cb->conn_id, p_cb->handle, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR:
            BTA_GATTC_WriteCharValue(p_cb->conn_id, p_cb->handle, p_cb->write_type,
                                     p_cb->len, p_cb->value, p_cb->auth_req);
            break;

        case BTIF_GATTC_WRITE_CHAR_DESCR:
            descr_val.len = p_cb->len;
            descr_val.p_value = p_cb->value;

            BTA_GATTC_WriteCharDescr(p_cb->conn_id, p_cb->handle,
                                     p_cb->write_type, &descr_val,
                                     p_cb->auth_req);
            break;
#endif
        /* modify for GATTC queue end */

        case BTIF_GATTC_EXECUTE_WRITE:
            BTA_GATTC_ExecuteWrite(p_cb->conn_id, p_cb->action);
            break;

        case BTIF_GATTC_REG_FOR_NOTIFICATION:
            status = BTA_GATTC_RegisterForNotifications(p_cb->client_if,
                                    p_cb->bd_addr.address, p_cb->handle);

            HAL_CBACK(bt_gatt_callbacks, client->register_for_notification_cb,
                p_cb->conn_id, 1, status, p_cb->handle);
            break;

        case BTIF_GATTC_DEREG_FOR_NOTIFICATION:
            status = BTA_GATTC_DeregisterForNotifications(p_cb->client_if,
                                        p_cb->bd_addr.address, p_cb->handle);

            HAL_CBACK(bt_gatt_callbacks, client->register_for_notification_cb,
                p_cb->conn_id, 0, status, p_cb->handle);
            break;

        case BTIF_GATTC_REFRESH:
            BTA_GATTC_Refresh(p_cb->bd_addr.address);
            break;

        case BTIF_GATTC_READ_RSSI:
            rssi_request_client_if = p_cb->client_if;
            BTM_ReadRSSI (p_cb->bd_addr.address, (tBTM_CMPL_CB *)btm_read_rssi_cb);
            break;

        case BTIF_GATTC_SCAN_FILTER_PARAM_SETUP:
        {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            if (1 == p_adv_filt_cb->adv_filt_param.dely_mode)
               BTA_DmBleTrackAdvertiser(p_adv_filt_cb->client_if, bta_track_adv_event_cb);
            BTA_DmBleScanFilterSetup(p_adv_filt_cb->action, p_adv_filt_cb->filt_index,
                &p_adv_filt_cb->adv_filt_param, NULL, bta_scan_filt_param_setup_cb,
                p_adv_filt_cb->client_if);
            break;
        }

        case BTIF_GATTC_SCAN_FILTER_CONFIG:
        {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            tBTA_DM_BLE_PF_COND_PARAM cond;
            memset(&cond, 0, sizeof(cond));

            switch (p_adv_filt_cb->filt_type)
            {
                case BTA_DM_BLE_PF_ADDR_FILTER: // 0
                    bdcpy(cond.target_addr.bda, p_adv_filt_cb->bd_addr.address);
                    cond.target_addr.type = p_adv_filt_cb->addr_type;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                              p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                              &cond, bta_scan_filt_cfg_cb,
                                              p_adv_filt_cb->client_if);
                    break;

                case BTA_DM_BLE_PF_SRVC_DATA: // 1
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                            p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                            NULL, bta_scan_filt_cfg_cb, p_adv_filt_cb->client_if);
                    break;

                case BTA_DM_BLE_PF_SRVC_UUID: // 2
                {
                    tBTA_DM_BLE_PF_COND_MASK uuid_mask;

                    cond.srvc_uuid.p_target_addr = NULL;
                    cond.srvc_uuid.cond_logic = BTA_DM_BLE_PF_LOGIC_AND;
                    btif_to_bta_uuid(&cond.srvc_uuid.uuid, &p_adv_filt_cb->uuid);

                    cond.srvc_uuid.p_uuid_mask = NULL;
                    if (p_adv_filt_cb->has_mask)
                    {
                        btif_to_bta_uuid_mask(&uuid_mask, &p_adv_filt_cb->uuid_mask);
                        cond.srvc_uuid.p_uuid_mask = &uuid_mask;
                    }
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                              p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                              &cond, bta_scan_filt_cfg_cb,
                                              p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_SRVC_SOL_UUID: // 3
                {
                    cond.solicitate_uuid.p_target_addr = NULL;
                    cond.solicitate_uuid.cond_logic = BTA_DM_BLE_PF_LOGIC_AND;
                    btif_to_bta_uuid(&cond.solicitate_uuid.uuid, &p_adv_filt_cb->uuid);
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                              p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                              &cond, bta_scan_filt_cfg_cb,
                                              p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_LOCAL_NAME: // 4
                {
                    cond.local_name.data_len = p_adv_filt_cb->value_len;
                    cond.local_name.p_data = p_adv_filt_cb->value;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                              p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                              &cond, bta_scan_filt_cfg_cb,
                                              p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_MANU_DATA: // 5
                {
                    cond.manu_data.company_id = p_adv_filt_cb->conn_id;
                    cond.manu_data.company_id_mask = p_adv_filt_cb->company_id_mask;
                    cond.manu_data.data_len = p_adv_filt_cb->value_len;
                    cond.manu_data.p_pattern = p_adv_filt_cb->value;
                    cond.manu_data.p_pattern_mask = p_adv_filt_cb->value_mask;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                              p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                              &cond, bta_scan_filt_cfg_cb,
                                              p_adv_filt_cb->client_if);
                    break;
                }

                case BTA_DM_BLE_PF_SRVC_DATA_PATTERN: //6
                {
                    cond.srvc_data.data_len = p_adv_filt_cb->value_len;
                    cond.srvc_data.p_pattern = p_adv_filt_cb->value;
                    cond.srvc_data.p_pattern_mask = p_adv_filt_cb->value_mask;
                    BTA_DmBleCfgFilterCondition(p_adv_filt_cb->action,
                                                p_adv_filt_cb->filt_type, p_adv_filt_cb->filt_index,
                                                &cond, bta_scan_filt_cfg_cb,
                                                p_adv_filt_cb->client_if);
                   break;
                }

                default:
                    LOG_ERROR(LOG_TAG, "%s: Unknown filter type (%d)!", __FUNCTION__, p_cb->action);
                    break;
            }
            break;
        }

        case BTIF_GATTC_SCAN_FILTER_CLEAR:
        {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            BTA_DmBleCfgFilterCondition(BTA_DM_BLE_SCAN_COND_CLEAR, BTA_DM_BLE_PF_TYPE_ALL,
                                        p_adv_filt_cb->filt_index, NULL, bta_scan_filt_cfg_cb,
                                        p_adv_filt_cb->client_if);
            break;
        }

        case BTIF_GATTC_SCAN_FILTER_ENABLE:
        {
            btgatt_adv_filter_cb_t *p_adv_filt_cb = (btgatt_adv_filter_cb_t *) p_param;
            BTA_DmEnableScanFilter(p_adv_filt_cb->action, bta_scan_filt_status_cb,
                                   p_adv_filt_cb->client_if);
            break;
        }

        case BTIF_GATTC_LISTEN:
#if (defined(BLE_PERIPHERAL_MODE_SUPPORT) && (BLE_PERIPHERAL_MODE_SUPPORT == TRUE))
            BTA_GATTC_Listen(p_cb->client_if, p_cb->start, NULL);
#else
            BTA_GATTC_Broadcast(p_cb->client_if, p_cb->start);
#endif
            break;

        case BTIF_GATTC_SET_ADV_DATA:
        {
            const btif_adv_data_t *p_adv_data = (btif_adv_data_t*) p_param;
            const int cbindex = CLNT_IF_IDX;
            if (cbindex >= 0 && btif_gattc_copy_datacb(cbindex, p_adv_data, false))
            {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
                if (!p_adv_data->set_scan_rsp)
                {
                    BTA_DmBleSetAdvConfig(p_multi_adv_data_cb->inst_cb[cbindex].mask,
                        &p_multi_adv_data_cb->inst_cb[cbindex].data, bta_gattc_set_adv_data_cback);
                }
                else
                {
                    BTA_DmBleSetScanRsp(p_multi_adv_data_cb->inst_cb[cbindex].mask,
                        &p_multi_adv_data_cb->inst_cb[cbindex].data, bta_gattc_set_adv_data_cback);
                }
            }
            else
            {
                BTIF_TRACE_ERROR("%s:%s: failed to get instance data cbindex: %d",
                                 __func__, "BTIF_GATTC_SET_ADV_DATA", cbindex);
            }
            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_ENABLE:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;

            int cbindex = -1, arrindex = -1;

            arrindex = btif_multi_adv_add_instid_map(p_inst_cb->client_if,INVALID_ADV_INST, true);
            if (arrindex >= 0)
                cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);

            if (cbindex >= 0 && arrindex >= 0)
            {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
                if (controller_get_interface()->supports_ble_extended_advertising())
                {
                    if (p_multi_adv_data_cb->inst_cb[cbindex].param.primary_advertising_phy)
                    {
                        p_inst_cb->param.advertising_event_properties = p_multi_adv_data_cb->inst_cb[cbindex].param.advertising_event_properties;
                        p_inst_cb->param.primary_advertising_phy = p_multi_adv_data_cb->inst_cb[cbindex].param.primary_advertising_phy;
                        p_inst_cb->param.secondary_advertising_phy = p_multi_adv_data_cb->inst_cb[cbindex].param.secondary_advertising_phy;
                        p_inst_cb->param.scan_request_notification_enable = p_multi_adv_data_cb->inst_cb[cbindex].param.scan_request_notification_enable;
                    }
                    else
                    {
                        p_inst_cb->param.advertising_event_properties = 0x0010;
                        p_inst_cb->param.primary_advertising_phy = 0x01;
                        p_inst_cb->param.secondary_advertising_phy = 0x01;
                        p_inst_cb->param.scan_request_notification_enable = 0x00;
                    }
                }
#endif
                memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].param,
                       &p_inst_cb->param, sizeof(tBTA_BLE_ADV_PARAMS));
                p_multi_adv_data_cb->inst_cb[cbindex].timeout_s = p_inst_cb->timeout_s;
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                            p_multi_adv_data_cb->clntif_map[arrindex + arrindex]);
                BTA_BleEnableAdvInstance(&(p_multi_adv_data_cb->inst_cb[cbindex].param),
                    bta_gattc_multi_adv_cback,
                    &(p_multi_adv_data_cb->clntif_map[arrindex + arrindex]));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_ENABLE_ADV",__FUNCTION__);
                BTA_BleEnableAdvInstance(&p_inst_cb->param, bta_gattc_multi_adv_cback, NULL);
            }
            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_UPDATE:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;
            if (NULL == p_inst_cb)
            {
                BTIF_TRACE_ERROR("%s p_inst_cb NULL error, BTIF_GATTC_ADV_INSTANCE_UPDATE", __FUNCTION__);
                break;
            }
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            int cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);
            if (inst_id >= 0 && cbindex >= 0)
            {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
                if (controller_get_interface()->supports_ble_extended_advertising())
                {
                    if (p_multi_adv_data_cb->inst_cb[cbindex].param.primary_advertising_phy)
                    {
                        p_inst_cb->param.advertising_event_properties = p_multi_adv_data_cb->inst_cb[cbindex].param.advertising_event_properties;
                        p_inst_cb->param.primary_advertising_phy = p_multi_adv_data_cb->inst_cb[cbindex].param.primary_advertising_phy;
                        p_inst_cb->param.secondary_advertising_phy = p_multi_adv_data_cb->inst_cb[cbindex].param.secondary_advertising_phy;
                        p_inst_cb->param.scan_request_notification_enable = p_multi_adv_data_cb->inst_cb[cbindex].param.scan_request_notification_enable;
                    }
                    else
                    {
                        p_inst_cb->param.advertising_event_properties = 0x0010;
                        p_inst_cb->param.primary_advertising_phy = 0x01;
                        p_inst_cb->param.secondary_advertising_phy = 0x01;
                        p_inst_cb->param.scan_request_notification_enable = 0x00;
                    }
                }
#endif
                memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].param, &p_inst_cb->param,
                        sizeof(tBTA_BLE_ADV_PARAMS));
                BTA_BleUpdateAdvInstParam((UINT8)inst_id,
                    &(p_multi_adv_data_cb->inst_cb[cbindex].param));
            }
            else
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_UPDATE_ADV", __FUNCTION__);
            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_SET_DATA:
        {
            btif_adv_data_t *p_adv_data = (btif_adv_data_t*) p_param;
            int cbindex = btif_gattc_obtain_idx_for_datacb(p_adv_data->client_if, CLNT_IF_IDX);
            int inst_id = btif_multi_adv_instid_for_clientif(p_adv_data->client_if);
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
            if (controller_get_interface()->supports_ble_extended_advertising())
            {
                if (inst_id >= 0 && cbindex >= 0)
                {
                    //btif_adv_data_t *p_adv_data = (btif_adv_data_t*) p_param;
                    btgatt_ext_adv_data_t ext_adv_data;
                    if (btif_gattc_copy_ext_datacb(p_adv_data, &ext_adv_data))
                    {
                        BTA_BleSetExtAdvData(p_adv_data->client_if, p_adv_data->set_scan_rsp,
                                             ext_adv_data.mask,
                                             &ext_adv_data.data);
                        memset(&ext_adv_data.data, 0, sizeof(ext_adv_data.data));
                    }
                    BTIF_TRACE_DEBUG("%s cbindex=%d, inst_id=%d",
                                      __FUNCTION__, cbindex, inst_id);
                }
                else
                {
                    BTIF_TRACE_ERROR(
                        "%s:%s: failed to get invalid instance data: inst_id:%d "
                        "cbindex:%d",
                        __func__, "BTIF_GATTC_ADV_INSTANCE_SET_DATA", inst_id, cbindex);
                }
                break;
            }
#endif
            if (inst_id >= 0 && cbindex >= 0 && btif_gattc_copy_datacb(cbindex, p_adv_data, true))
            {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb =
                    btif_obtain_multi_adv_data_cb();
                BTA_BleCfgAdvInstData(
                    (UINT8)inst_id,
                    p_adv_data->set_scan_rsp,
                    p_multi_adv_data_cb->inst_cb[cbindex].mask,
                    &p_multi_adv_data_cb->inst_cb[cbindex].data);
            }
            else
            {
                BTIF_TRACE_ERROR(
                    "%s:%s: failed to get invalid instance data: inst_id:%d "
                    "cbindex:%d",
                    __func__, "BTIF_GATTC_ADV_INSTANCE_SET_DATA", inst_id, cbindex);
            }
            break;
        }

        case BTIF_GATTC_ADV_INSTANCE_DISABLE:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            if (inst_id >=0)
                BTA_BleDisableAdvInstance((UINT8)inst_id);
            else
                BTIF_TRACE_ERROR("%s invalid instance ID in BTIF_GATTC_DISABLE_ADV",__FUNCTION__);
            break;
        }

        case BTIF_GATTC_CONFIGURE_MTU:
            BTA_GATTC_ConfigureMTU(p_cb->conn_id, p_cb->len);
            break;

        case BTIF_GATTC_CONN_PARAM_UPDT:
        {
            btif_conn_param_cb_t *p_conn_param_cb = (btif_conn_param_cb_t*) p_param;
            if (BTA_DmGetConnectionState(p_conn_param_cb->bd_addr.address))
            {
                BTA_DmBleUpdateConnectionParams(p_conn_param_cb->bd_addr.address,
                               p_conn_param_cb->min_interval, p_conn_param_cb->max_interval,
                               p_conn_param_cb->latency, p_conn_param_cb->timeout);
            } else {
                BTA_DmSetBlePrefConnParams(p_conn_param_cb->bd_addr.address,
                               p_conn_param_cb->min_interval, p_conn_param_cb->max_interval,
                               p_conn_param_cb->latency, p_conn_param_cb->timeout);
            }
            break;
        }

        case BTIF_GATTC_SET_SCAN_PARAMS:
        {
            BTA_DmSetBleScanParams(p_cb->client_if, p_cb->scan_interval, p_cb->scan_window,
                                   BTM_BLE_SCAN_MODE_ACTI, bta_scan_param_setup_cb);
            break;
        }

        case BTIF_GATTC_CONFIG_STORAGE_PARAMS:
        {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleSetStorageParams(p_scan_track_cb->batch_scan_full_max,
               p_scan_track_cb->batch_scan_trunc_max, p_scan_track_cb->batch_scan_notify_threshold,
               bta_batch_scan_setup_cb, bta_batch_scan_threshold_cb, bta_batch_scan_reports_cb,
               (tBTA_DM_BLE_REF_VALUE) p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_ENABLE_BATCH_SCAN:
        {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleEnableBatchScan(p_scan_track_cb->scan_mode, p_scan_track_cb->scan_interval,
               p_scan_track_cb->scan_window, p_scan_track_cb->discard_rule,
               p_scan_track_cb->addr_type, p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_DISABLE_BATCH_SCAN:
        {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleDisableBatchScan(p_scan_track_cb->client_if);
            break;
        }

        case BTIF_GATTC_READ_BATCH_SCAN_REPORTS:
        {
            btgatt_batch_track_cb_t *p_scan_track_cb = (btgatt_batch_track_cb_t *) p_param;
            BTA_DmBleReadScanReports(p_scan_track_cb->scan_mode, p_scan_track_cb->client_if);
            break;
        }

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
        case BTIF_GATTC_SET_PREFERRED_PHY:
            BTA_GATTC_SetPreferredPhy(p_cb->bd_addr.address, p_cb->tx_phy, p_cb->rx_phy, p_cb->phy_options);
            break;

        case BTIF_GATTC_READ_PHY:
            BTA_GATTC_ReadPhy(p_cb->bd_addr.address);
            break;

        case BTIF_GATTC_SET_ADV_EXT_PARAM:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*)p_param;

            int cbindex = -1, arrindex = -1;

            arrindex = btif_multi_adv_add_instid_map(p_inst_cb->client_if, INVALID_ADV_INST, true);
            if (arrindex >= 0)
                cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);

            if (cbindex >= 0 && arrindex >= 0)
            {
                btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
                p_multi_adv_data_cb->inst_cb[cbindex].param.advertising_event_properties = p_inst_cb->param.advertising_event_properties;
                p_multi_adv_data_cb->inst_cb[cbindex].param.primary_advertising_phy = p_inst_cb->param.primary_advertising_phy;
                p_multi_adv_data_cb->inst_cb[cbindex].param.secondary_advertising_phy = p_inst_cb->param.secondary_advertising_phy;
                p_multi_adv_data_cb->inst_cb[cbindex].param.scan_request_notification_enable = p_inst_cb->param.scan_request_notification_enable;
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_SET_ADV_EXT_PARAM", __FUNCTION__);
            }
            break;
        }
        case BTIF_GATTC_START_ADVERTISING_SET:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;
            int cbindex = -1, arrindex = -1;
            arrindex = btif_multi_adv_add_instid_map(p_inst_cb->client_if,INVALID_ADV_INST, true);
            if (arrindex >= 0)
            {
                cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);
            }
            BTIF_TRACE_DEBUG("%s, arrindex: %d, cbindex=%d", __FUNCTION__, arrindex, cbindex);

            btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

            if (cbindex >= 0 && arrindex >= 0)
            {
                memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].advertising_set,
                   &p_inst_cb->advertising_set, sizeof(tBTA_BLE_ADVERTISING_SET));
                p_multi_adv_data_cb->inst_cb[cbindex].timeout_s =
                                                p_inst_cb->advertising_set.duration;
                BTIF_TRACE_DEBUG("%s, peri_adv_len: %d", __FUNCTION__,
                    p_multi_adv_data_cb->inst_cb[cbindex].advertising_set.peri_adv_data.adv_len);
                //p_multi_adv_data_cb->inst_cb[cbindex].timeout_s = p_inst_cb->timeout_s;
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                        p_multi_adv_data_cb->clntif_map[arrindex + arrindex]);
                BTA_BleStartAdvertisingSet(&(p_multi_adv_data_cb->inst_cb[cbindex].advertising_set),
                     bta_gattc_multi_adv_cback,
                    &(p_multi_adv_data_cb->clntif_map[arrindex + arrindex]));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_ENABLE_ADV",__FUNCTION__);
            }
            break;
        }
        case BTIF_GATTC_STOP_ADVERTISING_SET:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            if (inst_id >=0)
                BTA_BleStopAdvertisingSet((UINT8)inst_id);
            else
                BTIF_TRACE_ERROR("%s invalid instance ID in BTIF_GATTC_DISABLE_ADV",__FUNCTION__);
            break;
        }
        case BTIF_GATTC_SET_PERIODIC_PARAM:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;

            int cbindex = -1;
            BTIF_TRACE_DEBUG("%s, client_if=%d", __FUNCTION__, p_inst_cb->client_if);
            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);
            BTIF_TRACE_DEBUG("%s, inst_id=%d,cbindex=%d", __FUNCTION__, inst_id, cbindex);

            btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
            if (cbindex >= 0)
            {
                memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].advertising_set.peri_params,
                   &p_inst_cb->advertising_set.peri_params, sizeof(tBTA_BLE_PERIODIC_ADV_PARAMS));
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                        p_multi_adv_data_cb->clntif_map[cbindex + cbindex]);
                BTA_BleAdvSetPeriodicParam(inst_id,
                    &(p_multi_adv_data_cb->inst_cb[cbindex].advertising_set.peri_params),
                    bta_gattc_multi_adv_cback,
                    &(p_multi_adv_data_cb->clntif_map[cbindex + cbindex]));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_SET_PERIODIC_PARAM", __FUNCTION__);
            }
            break;
        }
        case BTIF_GATTC_PERIODIC_ADV_ENABLE:
        {
            int cbindex = -1;
            btgatt_adv_peri_enable_cb *p_peri_adv_nable_cb =
                                           (btgatt_adv_peri_enable_cb *)p_param;
            int inst_id = btif_multi_adv_instid_for_clientif(p_peri_adv_nable_cb->client_if);
            UINT8 enable = p_peri_adv_nable_cb->enable;
            BTIF_TRACE_DEBUG("%s, inst value: %d", __FUNCTION__, inst_id);
            cbindex = btif_gattc_obtain_idx_for_datacb(p_peri_adv_nable_cb->client_if, CLNT_IF_IDX);
            btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();

            if (cbindex >= 0)
            {
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                        p_multi_adv_data_cb->clntif_map[cbindex + cbindex]);
                BTA_BleAdvPeriodicEnable(inst_id, enable,
                                     bta_gattc_multi_adv_cback,
                     &(p_multi_adv_data_cb->clntif_map[cbindex + cbindex]));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_PERIODIC_ADV_ENABLE", __FUNCTION__);
            }
            break;
        }
        case BTIF_GATTC_SET_ADVERTISING_PARAM:
        {
            btgatt_multi_adv_inst_cb *p_inst_cb = (btgatt_multi_adv_inst_cb*) p_param;

            int cbindex = -1;

            int inst_id = btif_multi_adv_instid_for_clientif(p_inst_cb->client_if);
            cbindex = btif_gattc_obtain_idx_for_datacb(p_inst_cb->client_if, CLNT_IF_IDX);

            btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
            if (cbindex >= 0)
            {
                memcpy(&p_multi_adv_data_cb->inst_cb[cbindex].advertising_set.adv_params,
                   &p_inst_cb->advertising_set.adv_params, sizeof(tBTA_BLE_ADV_PARAMS));
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                        p_multi_adv_data_cb->clntif_map[cbindex + cbindex]);
                BTA_BleAdvertisingSetParam((UINT8)inst_id,
                    &(p_multi_adv_data_cb->inst_cb[cbindex].advertising_set.adv_params));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_SET_ADVERTISING_PARAM", __FUNCTION__);
            }
            break;
        }
        case BTIF_GATTC_SET_ADVERTISING_DATA:
        {
            btgatt_adv_data_cb *p_adv_data_cb = (btgatt_adv_data_cb*) p_param;
            tBTA_BLE_PERI_ADV_DATA advertising_data;
            memset(&advertising_data, 0, sizeof(tBTA_BLE_PERI_ADV_DATA));

            int cbindex = btif_gattc_obtain_idx_for_datacb(p_adv_data_cb->client_if, CLNT_IF_IDX);;

            int inst_id = btif_multi_adv_instid_for_clientif(p_adv_data_cb->client_if);
            /* need use client if*/
            btgatt_multi_adv_common_data *p_multi_adv_data_cb = btif_obtain_multi_adv_data_cb();
            if (cbindex >= 0)
            {
                memcpy(&advertising_data, &(p_adv_data_cb->adv_data), sizeof(tBTA_BLE_PERI_ADV_DATA));
                BTIF_TRACE_DEBUG("%s, client_if value: %d", __FUNCTION__,
                                               p_adv_data_cb->client_if);
                BTA_BleAdvSetAdvertisingData(inst_id,
                                p_adv_data_cb->adv_data_type,
                                &advertising_data,
                                bta_gattc_multi_adv_cback,
                                &(p_multi_adv_data_cb->clntif_map[cbindex + cbindex]));
            }
            else
            {
                /* let the error propagate up from BTA layer */
                BTIF_TRACE_ERROR("%s invalid index in BTIF_GATTC_SET_ADVERTISING_DATA", __FUNCTION__);
            }
            break;
        }
#endif

        default:
            LOG_ERROR(LOG_TAG, "%s: Unknown event (%d)!", __FUNCTION__, event);
            break;
    }

    btgattc_free_event_data(event, p_param);
}

/*******************************************************************************
**  Client API Functions
********************************************************************************/

static bt_status_t btif_gattc_register_app(bt_uuid_t *uuid)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    memcpy(&btif_cb.uuid, uuid, sizeof(bt_uuid_t));
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REGISTER_APP,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_unregister_app(int client_if )
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_UNREGISTER_APP,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_scan( bool start )
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    return btif_transfer_context(btgattc_handle_event, start ? BTIF_GATTC_SCAN_START : BTIF_GATTC_SCAN_STOP,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_open(int client_if, const bt_bdaddr_t *bd_addr,
                                        bool is_direct,int transport)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.is_direct = is_direct ? 1 : 0;
    btif_cb.transport = (btgatt_transport_t)transport;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_OPEN,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_close( int client_if, const bt_bdaddr_t *bd_addr, int conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.conn_id = (uint16_t) conn_id;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CLOSE,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_listen(int client_if, bool start)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.start = start ? 1 : 0;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_LISTEN,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static void btif_gattc_deep_copy(UINT16 event, char *p_dest, char *p_src)
{
    switch (event)
    {
        case BTIF_GATTC_ADV_INSTANCE_SET_DATA:
        case BTIF_GATTC_SET_ADV_DATA:
        {
            const btif_adv_data_t *src = (btif_adv_data_t*) p_src;
            btif_adv_data_t *dst = (btif_adv_data_t*) p_dest;
            maybe_non_aligned_memcpy(dst, src, sizeof(*src));

            if (src->p_manufacturer_data)
            {
                dst->p_manufacturer_data = osi_malloc(src->manufacturer_len);
                memcpy(dst->p_manufacturer_data, src->p_manufacturer_data,
                       src->manufacturer_len);
            }

            if (src->p_service_data)
            {
                dst->p_service_data = osi_malloc(src->service_data_len);
                memcpy(dst->p_service_data, src->p_service_data, src->service_data_len);
            }

            if (src->p_service_uuid)
            {
                dst->p_service_uuid = osi_malloc(src->service_uuid_len);
                memcpy(dst->p_service_uuid, src->p_service_uuid, src->service_uuid_len);
            }
            break;
        }

        default:
            ASSERTC(false, "Unhandled deep copy", event);
            break;
    }
}

static bt_status_t btif_gattc_set_adv_data(int client_if, bool set_scan_rsp, bool include_name,
                bool include_txpower, int min_interval, int max_interval, int appearance,
                uint16_t manufacturer_len, char* manufacturer_data,
                uint16_t service_data_len, char* service_data,
                uint16_t service_uuid_len, char* service_uuid)
{
    CHECK_BTGATT_INIT();
    btif_adv_data_t adv_data;

    btif_gattc_adv_data_packager(client_if, set_scan_rsp, include_name,
        include_txpower, min_interval, max_interval, appearance, manufacturer_len,
        manufacturer_data, service_data_len, service_data, service_uuid_len, service_uuid,
        &adv_data);

    bt_status_t status = btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_ADV_DATA,
                       (char*) &adv_data, sizeof(adv_data), btif_gattc_deep_copy);
    btif_gattc_adv_data_cleanup(&adv_data);
    return status;
}

static bt_status_t btif_gattc_refresh( int client_if, const bt_bdaddr_t *bd_addr )
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REFRESH,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_search_service(int conn_id, bt_uuid_t *filter_uuid )
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.search_all = filter_uuid ? 0 : 1;
    if (filter_uuid)
        memcpy(&btif_cb.uuid, filter_uuid, sizeof(bt_uuid_t));
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SEARCH_SERVICE,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_get_gatt_db(int conn_id)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_GET_GATT_DB,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

#if defined(MTK_LINUX_GATTC_LE_NAME) && (MTK_LINUX_GATTC_LE_NAME == TRUE)
static bt_status_t btif_gattc_set_local_le_name(int client_if, char *p_name)
{
    CHECK_BTGATT_INIT();
    UINT16  le_name_len = 0;
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    memset(btif_cb.le_name, 0, MTK_LINUX_GATT_MAX_LE_NAME_LEN + 1);
    if (NULL != p_name)
    {
        le_name_len = strlen(p_name);
        le_name_len = le_name_len > MTK_LINUX_GATT_MAX_LE_NAME_LEN ? MTK_LINUX_GATT_MAX_LE_NAME_LEN : le_name_len;
        memcpy(btif_cb.le_name, p_name, le_name_len);
        btif_cb.le_name[le_name_len] = '\0';
    }
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_LOCAL_LE_NAME,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
#endif

#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
static bt_status_t btif_gattc_get_adv_rpa(int client_if)
{
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (uint8_t) client_if;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_GET_ADV_RPA,
                           (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
}
#endif

#if defined(MTK_LINUX_GATTC_DISC_MODE) && (MTK_LINUX_GATTC_DISC_MODE == TRUE)
static bt_status_t btif_gattc_set_discoverable_mode(int client_if, int disc_mode)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    btif_cb.disc_mode = (uint32_t)disc_mode;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_DISC_MODE,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
#endif

#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
static bt_status_t btif_gattc_read_using_characteristic_uuid(int conn_id, uint16_t start_handle,
         uint16_t end_handle, bt_uuid_t *char_uuid, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.s_handle = (uint16_t) start_handle;
    btif_cb.e_handle = (uint16_t) end_handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    if (char_uuid)
        memcpy(&btif_cb.uuid, char_uuid, sizeof(bt_uuid_t));

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_USING_CHAR_UUID,
                           (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_read_long_characteristic(int conn_id, uint16_t handle,
         uint16_t offset, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.offset = (uint16_t) offset;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_LONG_CHAR,
                           (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}


static bt_status_t btif_gattc_read_multi_characteristic(int conn_id, uint8_t num_attr,
         uint16_t *handles, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.num_attr = (uint8_t) num_attr;
    btif_cb.auth_req = (uint8_t) auth_req;
    if (btif_cb.num_attr > 0)
    {
        memcpy(btif_cb.handles, handles, sizeof(uint16_t) * btif_cb.num_attr);
    }
    BTIF_TRACE_DEBUG("%s, btif_cb.handles[0] = %d, btif_cb.handles[1] = %d",__FUNCTION__,
        btif_cb.handles[0], btif_cb.handles[1]);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_MULTIPLE,
                           (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_read_long_char_descr(int conn_id, uint16_t handle,
         uint16_t offset, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.offset = (uint16_t) offset;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_LONG_CHAR_DESCR,
                           (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_write_long_char(int conn_id, uint16_t handle, int write_type,
                                         int len, uint16_t offset, int auth_req, char* p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    btif_cb.offset = (uint16_t) offset;
    memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_LONG_CHAR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_write_long_char_descr(int conn_id, uint16_t handle, int write_type,
                                         int len, uint16_t offset, int auth_req, char* p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    btif_cb.offset = (uint16_t) offset;
    memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_LONG_CHAR_DESCR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}
static bt_status_t btif_gattc_set_pts_test_flag(uint8_t pts_flag)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.pts_flag = (uint8_t) pts_flag;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_PTS_FLAG,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}


#endif

static bt_status_t btif_gattc_read_char(int conn_id, uint16_t handle, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_CHAR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_read_char_descr(int conn_id, uint16_t handle, int auth_req)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_CHAR_DESCR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_write_char(int conn_id, uint16_t handle, int write_type,
                                         int len, int auth_req, char* p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_CHAR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_write_char_descr(int conn_id, uint16_t handle,
                                               int write_type, int len, int auth_req,
                                               char* p_value)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.handle = (uint16_t) handle;
    btif_cb.auth_req = (uint8_t) auth_req;
    btif_cb.write_type = (uint8_t) write_type;
    btif_cb.len = len > BTGATT_MAX_ATTR_LEN ? BTGATT_MAX_ATTR_LEN : len;
    memcpy(btif_cb.value, p_value, btif_cb.len);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_WRITE_CHAR_DESCR,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_execute_write(int conn_id, int execute)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = (uint16_t) conn_id;
    btif_cb.action = (uint8_t) execute;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_EXECUTE_WRITE,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_reg_for_notification(int client_if, const bt_bdaddr_t *bd_addr,
                                                   uint16_t handle)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.handle = handle;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_REG_FOR_NOTIFICATION,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_dereg_for_notification(int client_if, const bt_bdaddr_t *bd_addr,
                                                     uint16_t handle)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.handle = handle;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_DEREG_FOR_NOTIFICATION,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_read_remote_rssi(int client_if, const bt_bdaddr_t *bd_addr)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = (uint8_t) client_if;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_RSSI,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_configure_mtu(int conn_id, int mtu)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.conn_id = conn_id;
    btif_cb.len = mtu; // Re-use len field
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CONFIGURE_MTU,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static bt_status_t btif_gattc_conn_parameter_update(const bt_bdaddr_t *bd_addr, int min_interval,
                    int max_interval, int latency, int timeout)
{
    CHECK_BTGATT_INIT();
    btif_conn_param_cb_t btif_cb;
    btif_cb.min_interval = min_interval;
    btif_cb.max_interval = max_interval;
    btif_cb.latency = latency;
    btif_cb.timeout = timeout;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CONN_PARAM_UPDT,
                                 (char*) &btif_cb, sizeof(btif_conn_param_cb_t), NULL);
}

static bt_status_t btif_gattc_scan_filter_param_setup(btgatt_filt_param_setup_t
                                                      filt_param)
{
    CHECK_BTGATT_INIT();
    BTIF_TRACE_DEBUG("%s", __FUNCTION__);
    btgatt_adv_filter_cb_t btif_filt_cb;
    memset(&btif_filt_cb, 0, sizeof(btgatt_adv_filter_cb_t));
    btif_filt_cb.client_if = filt_param.client_if;
    btif_filt_cb.action = filt_param.action;
    btif_filt_cb.filt_index = filt_param.filt_index;
    btif_filt_cb.adv_filt_param.feat_seln = filt_param.feat_seln;
    btif_filt_cb.adv_filt_param.list_logic_type = filt_param.list_logic_type;
    btif_filt_cb.adv_filt_param.filt_logic_type = filt_param.filt_logic_type;
    btif_filt_cb.adv_filt_param.rssi_high_thres = filt_param.rssi_high_thres;
    btif_filt_cb.adv_filt_param.rssi_low_thres = filt_param.rssi_low_thres;
    btif_filt_cb.adv_filt_param.dely_mode = filt_param.dely_mode;
    btif_filt_cb.adv_filt_param.found_timeout = filt_param.found_timeout;
    btif_filt_cb.adv_filt_param.lost_timeout = filt_param.lost_timeout;
    btif_filt_cb.adv_filt_param.found_timeout_cnt = filt_param.found_timeout_cnt;
    btif_filt_cb.adv_filt_param.num_of_tracking_entries = filt_param.num_of_tracking_entries;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SCAN_FILTER_PARAM_SETUP,
                                 (char*) &btif_filt_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static bt_status_t btif_gattc_scan_filter_add_remove(int client_if, int action,
                              int filt_type, int filt_index, int company_id,
                              int company_id_mask, const bt_uuid_t *p_uuid,
                              const bt_uuid_t *p_uuid_mask, const bt_bdaddr_t *bd_addr,
                              char addr_type, int data_len, char* p_data, int mask_len,
                              char* p_mask)
{
    CHECK_BTGATT_INIT();
    btgatt_adv_filter_cb_t btif_filt_cb;
    memset(&btif_filt_cb, 0, sizeof(btgatt_adv_filter_cb_t));
    BTIF_TRACE_DEBUG("%s, %d, %d", __FUNCTION__, action, filt_type);

    /* If data is passed, both mask and data have to be the same length */
    if (data_len != mask_len && NULL != p_data && NULL != p_mask)
        return BT_STATUS_PARM_INVALID;

    btif_filt_cb.client_if = client_if;
    btif_filt_cb.action = action;
    btif_filt_cb.filt_index = filt_index;
    btif_filt_cb.filt_type = filt_type;
    btif_filt_cb.conn_id = company_id;
    btif_filt_cb.company_id_mask = company_id_mask ? company_id_mask : 0xFFFF;
    if (bd_addr)
        bdcpy(btif_filt_cb.bd_addr.address, bd_addr->address);

    btif_filt_cb.addr_type = addr_type;
    btif_filt_cb.has_mask = (p_uuid_mask != NULL);

    if (p_uuid != NULL)
        memcpy(&btif_filt_cb.uuid, p_uuid, sizeof(bt_uuid_t));
    if (p_uuid_mask != NULL)
        memcpy(&btif_filt_cb.uuid_mask, p_uuid_mask, sizeof(bt_uuid_t));
    if (p_data != NULL && data_len != 0)
    {
        memcpy(btif_filt_cb.value, p_data, data_len);
        btif_filt_cb.value_len = data_len;
        if (NULL != p_mask)
        {
            memcpy(btif_filt_cb.value_mask, p_mask, mask_len);
        }
        btif_filt_cb.value_mask_len = mask_len;
    }
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SCAN_FILTER_CONFIG,
                                 (char*) &btif_filt_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static bt_status_t btif_gattc_scan_filter_clear(int client_if, int filt_index)
{
    CHECK_BTGATT_INIT();
    BTIF_TRACE_DEBUG("%s, %d", __FUNCTION__, filt_index);

    btgatt_adv_filter_cb_t btif_filt_cb;
    memset(&btif_filt_cb, 0, sizeof(btgatt_adv_filter_cb_t));
    btif_filt_cb.client_if = client_if;
    btif_filt_cb.filt_index = filt_index;
    btif_filt_cb.action = BTA_DM_BLE_SCAN_COND_CLEAR;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SCAN_FILTER_CONFIG,
                                 (char*) &btif_filt_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static bt_status_t btif_gattc_scan_filter_enable(int client_if, bool enable)
{
    int action = 0;
    CHECK_BTGATT_INIT();
    BTIF_TRACE_DEBUG("%s, %d", __FUNCTION__, enable);

    btgatt_adv_filter_cb_t btif_filt_cb;
    memset(&btif_filt_cb, 0, sizeof(btgatt_adv_filter_cb_t));
    btif_filt_cb.client_if = client_if;
    if (true == enable)
        action = 1;
    btif_filt_cb.action = action;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SCAN_FILTER_ENABLE,
                                 (char*) &btif_filt_cb, sizeof(btgatt_adv_filter_cb_t), NULL);
}

static bt_status_t btif_gattc_set_scan_parameters(int client_if, int scan_interval,
                                                  int scan_window)
{
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    btif_cb.client_if = client_if;
    btif_cb.scan_interval = scan_interval;
    btif_cb.scan_window = scan_window;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_SCAN_PARAMS,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
}

static int btif_gattc_get_device_type( const bt_bdaddr_t *bd_addr )
{
    int device_type = 0;
    char bd_addr_str[18] = {0};

    bdaddr_to_string(bd_addr, bd_addr_str, sizeof(bd_addr_str));
    if (btif_config_get_int(bd_addr_str, "DevType", &device_type))
        return device_type;
    return 0;
}

static bt_status_t btif_gattc_multi_adv_enable(int client_if, int min_interval, int max_interval,
                                            int adv_type, int chnl_map, int tx_power, int timeout_s)
{
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (uint8_t) client_if;

    adv_cb.param.adv_int_min = min_interval;
    adv_cb.param.adv_int_max = max_interval;
    adv_cb.param.adv_type = adv_type;
    adv_cb.param.channel_map = chnl_map;
    adv_cb.param.adv_filter_policy = 0;
    adv_cb.param.tx_power = tx_power;
    adv_cb.timeout_s = timeout_s;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_ADV_INSTANCE_ENABLE,
                             (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
}

static bt_status_t btif_gattc_start_advertising_set(int client_if,
                          tBTIF_BLE_ADVERTISING_SET *p_advertising_set)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb peri_adv_cb;
    memset(&peri_adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    peri_adv_cb.client_if = (uint8_t) client_if;

    tBTA_BLE_ADV_PARAMS *p_peri_adv_param = &(peri_adv_cb.advertising_set.adv_params);
    tBTA_BLE_PERIODIC_ADV_PARAMS *p_peri_adv_peri_param =
        &(peri_adv_cb.advertising_set.peri_params);
    /*  adv params   */
    p_peri_adv_param->advertising_event_properties =
        p_advertising_set->adv_params.advertising_event_properties;
    p_peri_adv_param->adv_int_max = p_advertising_set->adv_params.adv_int_max;
    p_peri_adv_param->adv_int_min = p_advertising_set->adv_params.adv_int_min;
    p_peri_adv_param->channel_map = p_advertising_set->adv_params.channel_map;
    p_peri_adv_param->tx_power    = p_advertising_set->adv_params.tx_power;
    p_peri_adv_param->primary_advertising_phy =
        p_advertising_set->adv_params.primary_advertising_phy;
    p_peri_adv_param->secondary_advertising_phy =
        p_advertising_set->adv_params.secondary_advertising_phy;
    p_peri_adv_param->scan_request_notification_enable =
        p_advertising_set->adv_params.scan_request_notification_enable;

    peri_adv_cb.advertising_set.duration = p_advertising_set->duration;
    peri_adv_cb.advertising_set.maxExtAdvEvents =
        p_advertising_set->maxExtAdvEvents;

    BTIF_TRACE_DEBUG("%s second-phy:%d", __FUNCTION__, p_peri_adv_param->secondary_advertising_phy);

    /* adv data*/
    if (p_advertising_set->adv_data.adv_len != 0)
    {
        peri_adv_cb.advertising_set.adv_data.adv_len =
        p_advertising_set->adv_data.adv_len;
        memcpy(peri_adv_cb.advertising_set.adv_data.adv_data,
        p_advertising_set->adv_data.adv_data, p_advertising_set->adv_data.adv_len);
    }
    BTIF_TRACE_DEBUG("%s p_advertising_set->adv_data.adv_len:%d", __FUNCTION__, p_advertising_set->adv_data.adv_len);

    /* adv scan response data*/
    if (p_advertising_set->adv_scan_rsp_data.adv_len != 0)
    {
        peri_adv_cb.advertising_set.adv_scan_rsp_data.adv_len =
        p_advertising_set->adv_scan_rsp_data.adv_len;
        memcpy(peri_adv_cb.advertising_set.adv_scan_rsp_data.adv_data,
        p_advertising_set->adv_scan_rsp_data.adv_data, p_advertising_set->adv_scan_rsp_data.adv_len);
    }

    BTIF_TRACE_DEBUG("%s p_advertising_set->adv_scan_rsp_data.adv_len:%d", __FUNCTION__, p_advertising_set->adv_scan_rsp_data.adv_len);

    /*  peri adv peri param  */
    p_peri_adv_peri_param->enable = p_advertising_set->peri_params.enable;
    p_peri_adv_peri_param->max_interval =
        p_advertising_set->peri_params.max_interval;
    p_peri_adv_peri_param->min_interval =
        p_advertising_set->peri_params.min_interval;
    p_peri_adv_peri_param->periodic_advertising_properties =
        p_advertising_set->peri_params.periodic_advertising_properties;

    /* adv periodic adv data*/
    if (p_advertising_set->peri_adv_data.adv_len != 0)
    {
        peri_adv_cb.advertising_set.peri_adv_data.adv_len =
        p_advertising_set->peri_adv_data.adv_len;
        memcpy(peri_adv_cb.advertising_set.peri_adv_data.adv_data,
        p_advertising_set->peri_adv_data.adv_data, p_advertising_set->peri_adv_data.adv_len);
    }
    BTIF_TRACE_DEBUG("%s czw check p_advertising_set->peri_adv_data.adv_len:%d", __FUNCTION__, p_advertising_set->peri_adv_data.adv_len);

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_START_ADVERTISING_SET,
                             (char*) &peri_adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_advertising_set_peri_param(int client_if,
                                tBTIF_BLE_PERIODIC_ADV_PARAMS *p_peri_param)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
        CHECK_BTGATT_INIT();
        btgatt_multi_adv_inst_cb advrtising_cb;
        memset(&advrtising_cb, 0, sizeof(btgatt_multi_adv_inst_cb));

        tBTA_BLE_PERIODIC_ADV_PARAMS *p_peri_adv_peri_param =
                          &(advrtising_cb.advertising_set.peri_params);
        advrtising_cb.client_if = (uint8_t) client_if;

        /*  peri adv peri param  */
        p_peri_adv_peri_param->enable = p_peri_param->enable;
        p_peri_adv_peri_param->max_interval = p_peri_param->max_interval;
        p_peri_adv_peri_param->min_interval = p_peri_param->min_interval;
        p_peri_adv_peri_param->periodic_advertising_properties =
                           p_peri_param->periodic_advertising_properties;

        return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_PERIODIC_PARAM,
                                 (char*)(&advrtising_cb), sizeof(btgatt_multi_adv_inst_cb), NULL);
    }
    else
    {
        return BT_STATUS_UNSUPPORTED;
    }
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_advertising_peri_enable(int client_if,
                                                                 UINT8 enable)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
        CHECK_BTGATT_INIT();
        btgatt_adv_peri_enable_cb adv_peri_enable_cb;
        memset(&adv_peri_enable_cb, 0, sizeof(btgatt_adv_peri_enable_cb));

        /*  peri adv peri param  */
        adv_peri_enable_cb.client_if = (UINT8)client_if;
        adv_peri_enable_cb.enable    = enable;

        return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_PERIODIC_ADV_ENABLE,
                                 (char*) &adv_peri_enable_cb, sizeof(btgatt_adv_peri_enable_cb), NULL);
    }
    else
    {
        return BT_STATUS_UNSUPPORTED;
    }
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_advertising_set_param(int client_if,
                                tBTIF_BLE_ADV_PARAMS *p_param)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb advrtising_cb;
    memset(&advrtising_cb, 0, sizeof(btgatt_multi_adv_inst_cb));

    tBTA_BLE_ADVERTISING_PARAMS *p_adv_param =
                      &(advrtising_cb.advertising_set.adv_params);
    advrtising_cb.client_if = (uint8_t) client_if;

    /*  peri adv peri param  */
    p_adv_param->advertising_event_properties =
                 p_param->advertising_event_properties;
    p_adv_param->adv_filter_policy = p_param->adv_filter_policy;
    p_adv_param->adv_int_max = p_param->adv_int_max;
    p_adv_param->adv_int_min = p_param->adv_int_min;
    p_adv_param->adv_type = p_param->adv_type;
    p_adv_param->channel_map = p_param->channel_map;
    p_adv_param->primary_advertising_phy = p_param->primary_advertising_phy;
    p_adv_param->scan_request_notification_enable =
                      p_param->scan_request_notification_enable;
    p_adv_param->secondary_advertising_phy = p_param->secondary_advertising_phy;
    p_adv_param->tx_power = p_param->tx_power;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_ADVERTISING_PARAM,
                             (char*) &advrtising_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_advertising_set_data(int client_if,
                 UINT8 adv_data_type, tBTIF_BLE_PERI_ADV_DATA *p_adv_data)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
    CHECK_BTGATT_INIT();
    btgatt_adv_data_cb adv_data_cb;
    memset(&adv_data_cb, 0, sizeof(btgatt_adv_data_cb));
    adv_data_cb.client_if = (UINT8)client_if;
    adv_data_cb.adv_data_type = adv_data_type;
    if (p_adv_data != NULL)
    {
        memcpy(&adv_data_cb.adv_data, p_adv_data, sizeof(tBTIF_BLE_PERI_ADV_DATA));
    }
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_ADVERTISING_DATA,
                             (char*) &adv_data_cb, sizeof(btgatt_adv_data_cb), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_stop_advertising_set(int client_if)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    BTIF_TRACE_DEBUG ("%s client_if=%d", __func__, client_if);
    CHECK_BTGATT_INIT();

    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (UINT8) client_if;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_STOP_ADVERTISING_SET,
                           (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif


}

static bt_status_t btif_gattc_multi_adv_update(int client_if, int min_interval, int max_interval,
                                            int adv_type, int chnl_map,int tx_power, int timeout_s)
{
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (uint8_t) client_if;

    adv_cb.param.adv_int_min = min_interval;
    adv_cb.param.adv_int_max = max_interval;
    adv_cb.param.adv_type = adv_type;
    adv_cb.param.channel_map = chnl_map;
    adv_cb.param.adv_filter_policy = 0;
    adv_cb.param.tx_power = tx_power;
    adv_cb.timeout_s = timeout_s;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_ADV_INSTANCE_UPDATE,
                         (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
}

static bt_status_t btif_gattc_multi_adv_setdata(int client_if, bool set_scan_rsp,
                bool include_name, bool incl_txpower, int appearance,
                int manufacturer_len, char* manufacturer_data,
                int service_data_len, char* service_data,
                int service_uuid_len, char* service_uuid)
{
    CHECK_BTGATT_INIT();

    btif_adv_data_t multi_adv_data_inst;
    memset(&multi_adv_data_inst, 0, sizeof(multi_adv_data_inst));

    const int min_interval = 0;
    const int max_interval = 0;

    btif_gattc_adv_data_packager(client_if, set_scan_rsp, include_name, incl_txpower,
        min_interval, max_interval, appearance, manufacturer_len, manufacturer_data,
        service_data_len, service_data, service_uuid_len, service_uuid, &multi_adv_data_inst);

    bt_status_t status = btif_transfer_context(
        btgattc_handle_event, BTIF_GATTC_ADV_INSTANCE_SET_DATA,
        (char *)&multi_adv_data_inst, sizeof(multi_adv_data_inst),
        btif_gattc_deep_copy);
    btif_gattc_adv_data_cleanup(&multi_adv_data_inst);
    return status;
}

static bt_status_t btif_gattc_multi_adv_disable(int client_if)
{
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (uint8_t) client_if;

    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_ADV_INSTANCE_DISABLE,
                           (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
}

static bt_status_t btif_gattc_cfg_storage(int client_if,int batch_scan_full_max,
    int batch_scan_trunc_max, int batch_scan_notify_threshold)
{
    CHECK_BTGATT_INIT();
    btgatt_batch_track_cb_t bt_scan_cb;
    memset(&bt_scan_cb, 0, sizeof(btgatt_batch_track_cb_t));
    bt_scan_cb.client_if = (uint8_t) client_if;
    bt_scan_cb.batch_scan_full_max = batch_scan_full_max;
    bt_scan_cb.batch_scan_trunc_max = batch_scan_trunc_max;
    bt_scan_cb.batch_scan_notify_threshold = batch_scan_notify_threshold;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_CONFIG_STORAGE_PARAMS,
                                 (char*) &bt_scan_cb, sizeof(btgatt_batch_track_cb_t), NULL);
}

static bt_status_t btif_gattc_enb_batch_scan(int client_if,int scan_mode, int scan_interval,
                int scan_window, int addr_type, int discard_rule)
{
    CHECK_BTGATT_INIT();
    btgatt_batch_track_cb_t bt_scan_cb;
    memset(&bt_scan_cb, 0, sizeof(btgatt_batch_track_cb_t));
    bt_scan_cb.client_if = (uint8_t) client_if;
    bt_scan_cb.scan_mode = scan_mode;
    bt_scan_cb.scan_interval = scan_interval;
    bt_scan_cb.scan_window = scan_window;
    bt_scan_cb.discard_rule = discard_rule;
    bt_scan_cb.addr_type = addr_type;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_ENABLE_BATCH_SCAN,
                                 (char*) &bt_scan_cb, sizeof(btgatt_batch_track_cb_t), NULL);
}

static bt_status_t btif_gattc_dis_batch_scan(int client_if)
{
    CHECK_BTGATT_INIT();
    btgatt_batch_track_cb_t bt_scan_cb;
    memset(&bt_scan_cb, 0, sizeof(btgatt_batch_track_cb_t));
    bt_scan_cb.client_if = (uint8_t) client_if;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_DISABLE_BATCH_SCAN,
                                 (char*) &bt_scan_cb, sizeof(btgatt_batch_track_cb_t), NULL);
}

static bt_status_t btif_gattc_read_batch_scan_reports(int client_if, int scan_mode)
{
    CHECK_BTGATT_INIT();
    btgatt_batch_track_cb_t bt_scan_cb;
    memset(&bt_scan_cb, 0, sizeof(btgatt_batch_track_cb_t));
    bt_scan_cb.client_if = (uint8_t) client_if;
    bt_scan_cb.scan_mode = scan_mode;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_BATCH_SCAN_REPORTS,
                                 (char*) &bt_scan_cb, sizeof(btgatt_batch_track_cb_t), NULL);
}

extern bt_status_t btif_gattc_test_command_impl(int command, btgatt_test_params_t* params);

static bt_status_t btif_gattc_test_command(int command, btgatt_test_params_t* params)
{
    return btif_gattc_test_command_impl(command, params);
}

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
UINT8 bta_gattc_read_phy_callback(BD_ADDR bd_addr, uint8_t tx_phy, uint8_t rx_phy, uint8_t status)
{
    bt_bdaddr_t bda;
    bdcpy(bda.address, bd_addr);
    LOG_DEBUG(LOG_TAG, "bta_gattc_read_phy_callback: status = %d tx_phy= %d, rx_phy = %d", status, tx_phy, rx_phy);
    HAL_CBACK(bt_gatt_ex_callbacks, ex_client->phy_updated_cb, &bda, tx_phy, rx_phy, status);
    return 0;
}
#endif

static bt_status_t btif_gattc_set_preferred_phy(const bt_bdaddr_t *bd_addr, uint8_t tx_phy,
                                                uint8_t rx_phy, uint16_t phy_options)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    btif_cb.tx_phy = tx_phy;
    btif_cb.rx_phy = rx_phy;
    btif_cb.phy_options = phy_options;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_PREFERRED_PHY,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_read_phy(const bt_bdaddr_t *bd_addr)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    CHECK_BTGATT_INIT();
    btif_gattc_cb_t btif_cb;
    bdcpy(btif_cb.bd_addr.address, bd_addr->address);
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_READ_PHY,
                                 (char*) &btif_cb, sizeof(btif_gattc_cb_t), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

static bt_status_t btif_gattc_set_adv_ext_param(int client_if, uint16_t event_properties,
    uint8_t primary_phy, uint8_t secondary_phy, uint8_t scan_req_notify_enable)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    CHECK_BTGATT_INIT();
    btgatt_multi_adv_inst_cb adv_cb;
    memset(&adv_cb, 0, sizeof(btgatt_multi_adv_inst_cb));
    adv_cb.client_if = (uint8_t) client_if;

    adv_cb.param.advertising_event_properties = event_properties;
    adv_cb.param.primary_advertising_phy = primary_phy;
    adv_cb.param.secondary_advertising_phy = secondary_phy;
    adv_cb.param.scan_request_notification_enable = scan_req_notify_enable;
    return btif_transfer_context(btgattc_handle_event, BTIF_GATTC_SET_ADV_EXT_PARAM,
                             (char*) &adv_cb, sizeof(btgatt_multi_adv_inst_cb), NULL);
#else
    return BT_STATUS_UNSUPPORTED;
#endif
}

const btgatt_client_interface_t btgattClientInterface = {
    btif_gattc_register_app,
    btif_gattc_unregister_app,
    btif_gattc_scan,
    btif_gattc_open,
    btif_gattc_close,
    btif_gattc_listen,
    btif_gattc_refresh,
    btif_gattc_search_service,
    btif_gattc_read_char,
    btif_gattc_write_char,
    btif_gattc_read_char_descr,
    btif_gattc_write_char_descr,
    btif_gattc_execute_write,
    btif_gattc_reg_for_notification,
    btif_gattc_dereg_for_notification,
    btif_gattc_read_remote_rssi,
    btif_gattc_scan_filter_param_setup,
    btif_gattc_scan_filter_add_remove,
    btif_gattc_scan_filter_clear,
    btif_gattc_scan_filter_enable,
    btif_gattc_get_device_type,
    btif_gattc_set_adv_data,
    btif_gattc_configure_mtu,
    btif_gattc_conn_parameter_update,
    btif_gattc_set_scan_parameters,
    btif_gattc_multi_adv_enable,
    btif_gattc_multi_adv_update,
    btif_gattc_multi_adv_setdata,
    btif_gattc_multi_adv_disable,
    btif_gattc_cfg_storage,
    btif_gattc_enb_batch_scan,
    btif_gattc_dis_batch_scan,
    btif_gattc_read_batch_scan_reports,
    btif_gattc_test_command,
    btif_gattc_get_gatt_db
};

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
const btgatt_ex_client_interface_t btgattExClientInterface = {
    btif_gattc_set_local_le_name,
    btif_gattc_get_adv_rpa,
    btif_gattc_set_discoverable_mode,
#if defined(MTK_LINUX_GATTC_PTS_TEST) && (MTK_LINUX_GATTC_PTS_TEST == TRUE)
    btif_gattc_read_using_characteristic_uuid,
    btif_gattc_read_long_characteristic,
    btif_gattc_read_multi_characteristic,
    btif_gattc_read_long_char_descr,
    btif_gattc_write_long_char,
    btif_gattc_write_long_char_descr,
    btif_gattc_set_pts_test_flag,
#endif
    btif_gattc_set_preferred_phy,
    btif_gattc_read_phy,
    btif_gattc_set_adv_ext_param,
    btif_gattc_start_advertising_set,
    btif_gattc_stop_advertising_set,
    btif_gattc_advertising_set_peri_param,
    btif_gattc_advertising_set_data,
    btif_gattc_advertising_set_param,
    btif_gattc_advertising_peri_enable,
};
#endif

#endif

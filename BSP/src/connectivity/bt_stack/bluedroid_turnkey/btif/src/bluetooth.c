/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

/************************************************************************************
 *
 *  Filename:      bluetooth.c
 *
 *  Description:   Bluetooth HAL implementation
 *
 ***********************************************************************************/

#define LOG_TAG "bt_btif"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <hardware/bluetooth.h>
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
#include "mtk_bt_av.h"
#else
#include <hardware/bt_av.h>
#endif
#include <hardware/bt_gatt.h>
#include <hardware/bt_hf.h>
#include <hardware/bt_hf_client.h>
#include <hardware/bt_hh.h>
#include <hardware/bt_hl.h>
#include <hardware/bt_mce.h>
#include <hardware/bt_pan.h>
#if defined(MTK_LINUX_AVRCP_PLUS) && (MTK_LINUX_AVRCP_PLUS == TRUE)
#include "mtk_bt_rc.h"
#else
#include <hardware/bt_rc.h>
#endif
#include <hardware/bt_sdp.h>
#include <hardware/bt_sock.h>

#include "bt_utils.h"
#include "btif_api.h"
#include "btif_debug.h"
#include "btsnoop.h"
#include "btsnoop_mem.h"
#include "device/include/interop.h"
#include "osi/include/allocation_tracker.h"
#include "osi/include/alarm.h"
#include "osi/include/log.h"
#include "osi/include/metrics.h"
#include "osi/include/osi.h"
#include "osi/include/wakelock.h"
#include "osi/include/alarm.h"
#include "stack_manager.h"
#include "btif_config.h"
#include "btif_storage.h"
#include "btif/include/btif_debug_btsnoop.h"
#include "btif/include/btif_debug_conn.h"
#include "btif/include/btif_debug_l2c.h"
#include "btif/include/btif_media.h"
#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
#include "stack/include/hcimsgs.h"
#endif

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
#include "mtk_bt_gatt.h"
#endif

#if defined(MTK_LINUX_HID) && (MTK_LINUX_HID == TRUE)
#include "mtk_bt_hh.h"
#endif

#if defined(MTK_LINUX_HFP) && (MTK_LINUX_HFP == TRUE)
#include "mtk_bt_hf_client.h"
#endif
/************************************************************************************
**  Static variables
************************************************************************************/

bt_callbacks_t *bt_hal_cbacks = NULL;
bool restricted_mode = FALSE;

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
btgap_ex_callbacks_t *bt_gap_ex_cbacks = NULL;
alarm_t *bt_gap_scan_timer = NULL;
#endif
/************************************************************************************
**  Externs
************************************************************************************/

/* list all extended interfaces here */

/* handsfree profile */
extern bthf_interface_t *btif_hf_get_interface();
/* handsfree profile - client */
extern bthf_client_interface_t *btif_hf_client_get_interface();
#if defined(MTK_LINUX_HFP) && (MTK_LINUX_HFP == TRUE)
extern bthf_client_ex_interface_t *btif_hf_ex_client_get_interface();
#endif
/* advanced audio profile */
extern btav_interface_t *btif_av_get_src_interface();
extern btav_interface_t *btif_av_get_sink_interface();
/*rfc l2cap*/
extern btsock_interface_t *btif_sock_get_interface();
/* hid host profile */
extern bthh_interface_t *btif_hh_get_interface();
/* health device profile */
extern bthl_interface_t *btif_hl_get_interface();
/*pan*/
extern btpan_interface_t *btif_pan_get_interface();
/*map client*/
extern btmce_interface_t *btif_mce_get_interface();
#if BLE_INCLUDED == TRUE
/* gatt */
extern btgatt_interface_t *btif_gatt_get_interface();

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
extern btgatt_ex_interface_t *btif_gatt_ex_get_interface();
#endif
#endif

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
extern const btgap_ex_interface_t *btif_gap_ex_get_interface();
#endif

#if defined(MTK_LINUX_HID) && (MTK_LINUX_HID == TRUE)
extern const bthh_ex_interface_t *btif_hh_ex_get_interface();
#endif

/* avrc target */
extern btrc_interface_t *btif_rc_get_interface();
/* avrc controller */
extern btrc_interface_t *btif_rc_ctrl_get_interface();
#if defined(MTK_LINUX_AVRCP_PLUS) && (MTK_LINUX_AVRCP_PLUS == TRUE)
extern btrc_ctrl_ex_interface_t *btif_rc_ex_get_interface(void);
extern btrc_ctrl_ex_interface_t *btif_rc_tg_ex_get_interface(void);
#endif

#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
extern const btav_ext_interface_t *btif_av_get_ext_interface(void);
extern const btav_src_ext_interface_t *btif_av_get_src_ext_interface(void);
extern const btav_sink_ext_interface_t *btif_av_get_sink_ext_interface(void);
#endif

/*SDP search client*/
extern btsdp_interface_t *btif_sdp_get_interface();

#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
extern btrc_interface_t *btif_b3ds_get_interface();
#endif

/************************************************************************************
**  Functions
************************************************************************************/
static int cancel_discovery(void);

static bool interface_ready(void) {
  return bt_hal_cbacks != NULL;
}

static bool is_profile(const char *p1, const char *p2) {
  assert(p1);
  assert(p2);
  return strlen(p1) == strlen(p2) && strncmp(p1, p2, strlen(p2)) == 0;
}

#if defined(MTK_BT_PERFORMANCE_ANALYSIS) && (MTK_BT_PERFORMANCE_ANALYSIS == TRUE)
#include "osi/include/properties.h"
enum{
    BPERF_STATE_UNKNOWN,
    BPERF_STATE_THREAD_RUNNING,
    BPERF_STATE_THREAD_STOPPED
};

pthread_mutex_t bperf_thread_lock;
static pthread_t bperf_per_thread;
static unsigned int bperf_per_thread_status;
static unsigned int bperf_per_thread_should_stop;
static pthread_t bperf_rssi_thread;
static unsigned int bperf_rssi_thread_status;
static unsigned int bperf_rssi_thread_should_stop;
static pthread_t bperf_handle_op_thread;
static unsigned int bperf_handle_op_thread_status;
static unsigned int bperf_handle_op_thread_should_stop;

extern bt_status_t btif_dm_get_vendor_rssi(const uint8_t index);
extern bt_status_t btif_dm_get_vendor_per();
extern bt_status_t btif_dm_start_inquiry();
extern bt_status_t btif_av_connect_disconnect();
extern bt_status_t btif_hf_connect_disconnect();

static void *_bperf_per_thread(void *arg)
{
    bperf_per_thread_status = BPERF_STATE_THREAD_RUNNING;
    while(!bperf_per_thread_should_stop)
    {
        sleep(1);

        // Get PER
        btif_dm_get_vendor_per();
    }
    bperf_per_thread_should_stop = 0;
    bperf_per_thread_status = BPERF_STATE_THREAD_STOPPED;
    return 0;
}

static void *_bperf_rssi_thread(void *arg)
{
    uint8_t i=0;

    bperf_rssi_thread_status = BPERF_STATE_THREAD_RUNNING;
    while(!bperf_rssi_thread_should_stop)
    {
        sleep(1);

        // Get RSSI
        for ( i = 0 ; i < MAX_L2CAP_LINKS ; i++ )
        {
            usleep(10000);
            btif_dm_get_vendor_rssi(i);
        }
    }
    bperf_rssi_thread_should_stop = 0;
    bperf_rssi_thread_status = BPERF_STATE_THREAD_STOPPED;
    return 0;
}


static void *_bperf_handle_op_thread(void *arg)
{

    bperf_handle_op_thread_status = BPERF_STATE_THREAD_RUNNING;
    while(!bperf_handle_op_thread_should_stop)
    {
        sleep(1);

        // Get Inquiry setting from property then set to Bluedroid
        btif_dm_start_inquiry();

        // Get A2DP connect/disconnect setting from property the set to Bluedroid
        btif_av_connect_disconnect();

        // Get HFP connect/disconnect setting from property the set to Bluedroid
        btif_hf_connect_disconnect();
    }
    bperf_handle_op_thread_should_stop = 0;
    bperf_handle_op_thread_status = BPERF_STATE_THREAD_STOPPED;
    return 0;
}

void _bperf_init()
{
  char prop_value_char[PROPERTY_VALUE_MAX];;
  if (osi_property_get("bt.bperf.enable", prop_value_char, "") &&
        strncmp(prop_value_char, "1", 1) == 0)
  {
    LOG_INFO(LOG_TAG, "%s : bt.bperf.enable = 1", __func__);
    pthread_mutex_init(&bperf_thread_lock, NULL);
    pthread_mutex_lock(&bperf_thread_lock);
    if ( bperf_per_thread_status != BPERF_STATE_THREAD_RUNNING && !bperf_per_thread_should_stop)
    {
      pthread_create(&bperf_per_thread, NULL, _bperf_per_thread, NULL);
    }
    if ( bperf_rssi_thread_status != BPERF_STATE_THREAD_RUNNING && !bperf_rssi_thread_should_stop)
    {
      pthread_create(&bperf_rssi_thread, NULL, _bperf_rssi_thread, NULL);
    }
    if ( bperf_handle_op_thread_status != BPERF_STATE_THREAD_RUNNING && !bperf_handle_op_thread_should_stop)
    {
      pthread_create(&bperf_handle_op_thread, NULL, _bperf_handle_op_thread, NULL);
    }
    pthread_mutex_unlock(&bperf_thread_lock);
  }
}

void _bperf_deinit()
{
  char prop_value_char[PROPERTY_VALUE_MAX];;
  if (osi_property_get("bt.bperf.enable", prop_value_char, "") &&
        strncmp(prop_value_char, "1", 1) == 0)
  {
    LOG_INFO(LOG_TAG, "%s : bt.bperf.enable = 1", __func__);
    pthread_mutex_lock(&bperf_thread_lock);
    bperf_per_thread_should_stop = 1;
    bperf_rssi_thread_should_stop = 1;
    bperf_handle_op_thread_should_stop = 1;
    pthread_join(bperf_per_thread, NULL);
    pthread_join(bperf_rssi_thread, NULL);
    pthread_join(bperf_handle_op_thread, NULL);
    pthread_mutex_unlock(&bperf_thread_lock);
    pthread_mutex_destroy(&bperf_thread_lock);
  }
}
#endif

/*****************************************************************************
**
**   BLUETOOTH HAL INTERFACE FUNCTIONS
**
*****************************************************************************/

static int init(bt_callbacks_t *callbacks) {
  LOG_INFO(LOG_TAG, "%s", __func__);

  if (interface_ready())
    return BT_STATUS_DONE;

#ifdef BLUEDROID_DEBUG
  allocation_tracker_init();
#endif

  bt_hal_cbacks = callbacks;

  stack_manager_get_interface()->init_stack();
  btif_debug_init();
#if defined(MTK_LINUX_STACK_LOG2FILE) && (MTK_LINUX_STACK_LOG2FILE == TRUE)
  log_init();
#endif

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
  bt_gap_scan_timer = alarm_new("bt_gap_scan_timer");
#endif
  return BT_STATUS_SUCCESS;
}

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
static bool interface_ex_ready(void) {
  return bt_gap_ex_cbacks != NULL;
}

static int init_ex(btgap_ex_callbacks_t *callbacks)
{
  LOG_INFO(LOG_TAG, "%s", __func__);

  if (interface_ex_ready())
    return BT_STATUS_DONE;

  bt_gap_ex_cbacks = callbacks;
  return BT_STATUS_SUCCESS;
}
#endif

static int enable(bool start_restricted) {
  LOG_INFO(LOG_TAG, "%s: start restricted = %d", __func__, start_restricted);

  restricted_mode = start_restricted;

  if (!interface_ready())
    return BT_STATUS_NOT_READY;

  stack_manager_get_interface()->start_up_stack_async();
#if defined(MTK_BT_PERFORMANCE_ANALYSIS) && (MTK_BT_PERFORMANCE_ANALYSIS == TRUE)
  _bperf_init();
#endif

  return BT_STATUS_SUCCESS;
}

static int disable(void) {
  if (!interface_ready())
    return BT_STATUS_NOT_READY;

#if defined(MTK_BT_PERFORMANCE_ANALYSIS) && (MTK_BT_PERFORMANCE_ANALYSIS == TRUE)
  _bperf_deinit();
#endif


  stack_manager_get_interface()->shut_down_stack_async();
  return BT_STATUS_SUCCESS;
}

static void cleanup(void) {
#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
  if (NULL != bt_gap_scan_timer)
  {
    alarm_free(bt_gap_scan_timer);
    bt_gap_scan_timer = NULL;
  }
#endif
  stack_manager_get_interface()->clean_up_stack();
#if defined(MTK_LINUX_STACK_LOG2FILE) && (MTK_LINUX_STACK_LOG2FILE == TRUE)
  log_deinit();
#endif
}

bool is_restricted_mode() {
  return restricted_mode;
}

static int get_adapter_properties(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_adapter_properties();
}

static int get_adapter_property(bt_property_type_t type)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_adapter_property(type);
}

static int set_adapter_property(const bt_property_t *property)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_set_adapter_property(property);
}

int get_remote_device_properties(bt_bdaddr_t *remote_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_device_properties(remote_addr);
}

int get_remote_device_property(bt_bdaddr_t *remote_addr, bt_property_type_t type)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_device_property(remote_addr, type);
}

int set_remote_device_property(bt_bdaddr_t *remote_addr, const bt_property_t *property)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_set_remote_device_property(remote_addr, property);
}

int get_remote_service_record(bt_bdaddr_t *remote_addr, bt_uuid_t *uuid)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_get_remote_service_record(remote_addr, uuid);
}

int get_remote_services(bt_bdaddr_t *remote_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_get_remote_services(remote_addr);
}

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
static void discovery_timeout(void *data)
{
    LOG_INFO(LOG_TAG, "%s", __func__);
    cancel_discovery();
}
#endif

static int start_discovery(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
    if (NULL != bt_gap_scan_timer)
    {
        alarm_set(bt_gap_scan_timer, 10000, discovery_timeout, NULL);
    }
#endif

    return btif_dm_start_discovery();
}

static int cancel_discovery(void)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
    if (NULL != bt_gap_scan_timer)
    {
        alarm_cancel(bt_gap_scan_timer);
    }
#endif
    return btif_dm_cancel_discovery();
}

static int create_bond(const bt_bdaddr_t *bd_addr, int transport)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_create_bond(bd_addr, transport);
}

static int create_bond_out_of_band(const bt_bdaddr_t *bd_addr, int transport,
                                   const bt_out_of_band_data_t *oob_data)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_create_bond_out_of_band(bd_addr, transport, oob_data);
}

static int cancel_bond(const bt_bdaddr_t *bd_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_cancel_bond(bd_addr);
}

static int remove_bond(const bt_bdaddr_t *bd_addr)
{
    if (is_restricted_mode() && !btif_storage_is_restricted_device(bd_addr))
        return BT_STATUS_SUCCESS;

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_remove_bond(bd_addr);
}

static int get_connection_state(const bt_bdaddr_t *bd_addr)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return 0;

    return btif_dm_get_connection_state(bd_addr);
}

static int pin_reply(const bt_bdaddr_t *bd_addr, uint8_t accept,
                 uint8_t pin_len, bt_pin_code_t *pin_code)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_pin_reply(bd_addr, accept, pin_len, pin_code);
}

static int ssp_reply(const bt_bdaddr_t *bd_addr, bt_ssp_variant_t variant,
                       uint8_t accept, uint32_t passkey)
{
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dm_ssp_reply(bd_addr, variant, accept, passkey);
}

static int read_energy_info()
{
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;
    btif_dm_read_energy_info();
    return BT_STATUS_SUCCESS;
}

static void dump(int fd, const char **arguments)
{
    if (arguments != NULL && arguments[0] != NULL) {
      if (strncmp(arguments[0], "--proto-bin", 11) == 0) {
        metrics_write_base64(fd, true);
        return;
      }
    }
    btif_debug_conn_dump(fd);
    btif_debug_bond_event_dump(fd);
    btif_debug_a2dp_dump(fd);
    btif_debug_l2c_dump(fd);
    btif_debug_config_dump(fd);
    wakelock_debug_dump(fd);
    alarm_debug_dump(fd);
#if defined(BTSNOOP_MEM) && (BTSNOOP_MEM == TRUE)
    btif_debug_btsnoop_dump(fd);
#endif

    close(fd);
}

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
static int send_hci(uint8_t *buf, uint8_t len)
{
    LOG_INFO(LOG_TAG, "%s", __func__);
    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btsnd_hcic_send_hci(buf, len);
}

extern int A2D_SetLHDCLicenseKey(char *name, uint8_t *data, int data_len);
static int set_lhdc_key(char *name, uint8_t *data, int data_len)
{
    LOG_INFO(LOG_TAG, "%s", __func__);

    return A2D_SetLHDCLicenseKey(name, data, data_len);
}

static int get_rssi(const bt_bdaddr_t *bd_addr)
{
    LOG_INFO(LOG_TAG, "%s", __func__);

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;
    return btif_dm_get_rssi(bd_addr);
}
#endif

static const void* get_profile_interface (const char *profile_id)
{
    LOG_INFO(LOG_TAG, "get_profile_interface %s", profile_id);

    /* sanity check */
    if (interface_ready() == FALSE)
        return NULL;

    /* check for supported profile interfaces */
    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_ID))
        return btif_hf_get_interface();

    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_CLIENT_ID))
        return btif_hf_client_get_interface();

#if defined(MTK_LINUX_HFP) && (MTK_LINUX_HFP == TRUE)
    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_CLIENT_EX_ID))
        return btif_hf_ex_client_get_interface();
#endif
    if (is_profile(profile_id, BT_PROFILE_SOCKETS_ID))
        return btif_sock_get_interface();

    if (is_profile(profile_id, BT_PROFILE_PAN_ID))
        return btif_pan_get_interface();

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_ID))
        return btif_av_get_src_interface();

#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_EXT_ID))
        return btif_av_get_ext_interface();

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SRC_EXT_ID))
        return btif_av_get_src_ext_interface();

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SINK_EXT_ID))
        return btif_av_get_sink_ext_interface();
#endif

    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SINK_ID))
        return btif_av_get_sink_interface();

    if (is_profile(profile_id, BT_PROFILE_HIDHOST_ID))
        return btif_hh_get_interface();

#if defined(MTK_LINUX_HID) && (MTK_LINUX_HID == TRUE)
    if (is_profile(profile_id, BT_PROFILE_HIDHOST_EX_ID))
        return btif_hh_ex_get_interface();
#endif

    if (is_profile(profile_id, BT_PROFILE_HEALTH_ID))
        return btif_hl_get_interface();

    if (is_profile(profile_id, BT_PROFILE_SDP_CLIENT_ID))
        return btif_sdp_get_interface();

#if ( BTA_GATT_INCLUDED == TRUE && BLE_INCLUDED == TRUE)
    if (is_profile(profile_id, BT_PROFILE_GATT_ID))
        return btif_gatt_get_interface();

#if defined(MTK_LINUX_GATT) && (MTK_LINUX_GATT == TRUE)
    if (is_profile(profile_id, BT_PROFILE_GATT_EX_ID))
        return btif_gatt_ex_get_interface();
#endif
#endif

    if (is_profile(profile_id, BT_PROFILE_AV_RC_ID))
        return btif_rc_get_interface();

    if (is_profile(profile_id, BT_PROFILE_AV_RC_CTRL_ID))
        return btif_rc_ctrl_get_interface();

#if defined(MTK_LINUX_AVRCP_PLUS) && (MTK_LINUX_AVRCP_PLUS == TRUE)
    if (is_profile(profile_id, BT_PROFILE_AVRCP_EX_ID))
        return btif_rc_ex_get_interface();

    if (is_profile(profile_id, BT_PROFILE_AVRCP_TG_EX_ID))
        return btif_rc_tg_ex_get_interface();
#endif

#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
#define BT_PROFILE_B3DS_ID "b3ds"
    if (is_profile(profile_id, BT_PROFILE_B3DS_ID))
        return btif_b3ds_get_interface();
#endif

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
    if (is_profile(profile_id, BT_PROFILE_GAP_EX_ID))
        return btif_gap_ex_get_interface();
#endif
    return NULL;
}

int dut_mode_configure(uint8_t enable)
{
    LOG_INFO(LOG_TAG, "dut_mode_configure");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dut_mode_configure(enable);
}

int dut_mode_send(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    LOG_INFO(LOG_TAG, "dut_mode_send");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_dut_mode_send(opcode, buf, len);
}

#if BLE_INCLUDED == TRUE
int le_test_mode(uint16_t opcode, uint8_t* buf, uint8_t len)
{
    LOG_INFO(LOG_TAG, "le_test_mode");

    /* sanity check */
    if (interface_ready() == FALSE)
        return BT_STATUS_NOT_READY;

    return btif_le_test_mode(opcode, buf, len);
}
#endif

int config_hci_snoop_log(uint8_t enable)
{
    LOG_INFO(LOG_TAG, "config_hci_snoop_log");

    if (!interface_ready())
        return BT_STATUS_NOT_READY;

    btsnoop_get_interface()->set_api_wants_to_log(enable);
    return BT_STATUS_SUCCESS;
}

static int set_os_callouts(bt_os_callouts_t *callouts) {
    wakelock_set_os_callouts(callouts);
    return BT_STATUS_SUCCESS;
}

static int config_clear(void) {
    LOG_INFO(LOG_TAG, "%s", __func__);
    return btif_config_clear() ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

static const bt_interface_t bluetoothInterface = {
    sizeof(bluetoothInterface),
    init,
    enable,
    disable,
    cleanup,
    get_adapter_properties,
    get_adapter_property,
    set_adapter_property,
    get_remote_device_properties,
    get_remote_device_property,
    set_remote_device_property,
    get_remote_service_record,
    get_remote_services,
    start_discovery,
    cancel_discovery,
    create_bond,
    create_bond_out_of_band,
    remove_bond,
    cancel_bond,
    get_connection_state,
    pin_reply,
    ssp_reply,
    get_profile_interface,
    dut_mode_configure,
    dut_mode_send,
#if BLE_INCLUDED == TRUE
    le_test_mode,
#else
    NULL,
#endif
    config_hci_snoop_log,
    set_os_callouts,
    read_energy_info,
    dump,
    config_clear,
    interop_database_clear,
    interop_database_add,
};

const bt_interface_t* bluetooth__get_bluetooth_interface ()
{
    /* fixme -- add property to disable bt interface ? */

    return &bluetoothInterface;
}

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
static const btgap_ex_interface_t btgapExInterface = {
    sizeof(btgapExInterface),
    init_ex,
    get_rssi,
    send_hci,
    set_lhdc_key,
};

const btgap_ex_interface_t *btif_gap_ex_get_interface()
{
    return &btgapExInterface;
}
#endif

static int close_bluetooth_stack(struct hw_device_t* device)
{
    UNUSED(device);
    cleanup();
    return 0;
}

#if defined(MTK_LINUX_EXPORT_API) && (MTK_LINUX_EXPORT_API == TRUE)
EXPORT_SYMBOL int open_bluetooth_stack(const struct hw_module_t *module, UNUSED_ATTR char const *name, struct hw_device_t **abstraction) {
#else
static int open_bluetooth_stack(const struct hw_module_t *module, UNUSED_ATTR char const *name, struct hw_device_t **abstraction) {
#endif
  static bluetooth_device_t device = {
    .common = {
      .tag = HARDWARE_DEVICE_TAG,
      .version = 0,
      .close = close_bluetooth_stack,
    },
    .get_bluetooth_interface = bluetooth__get_bluetooth_interface
  };

  device.common.module = (struct hw_module_t *)module;
  *abstraction = (struct hw_device_t *)&device;
  return 0;
}

static struct hw_module_methods_t bt_stack_module_methods = {
    .open = open_bluetooth_stack,
};

EXPORT_SYMBOL struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = BT_HARDWARE_MODULE_ID,
    .name = "Bluetooth Stack",
    .author = "The Android Open Source Project",
    .methods = &bt_stack_module_methods
};

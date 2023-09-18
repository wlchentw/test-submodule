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

#define LOG_TAG "btif_av"

#include "btif_av.h"

#include <assert.h>
#include <string.h>

#include <system/audio.h>
#include <hardware/bluetooth.h>
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
#include "mtk_bt_av.h"
#include <sys/resource.h>
#include <sys/time.h>
extern thread_t *bt_workqueue_thread;
extern thread_t *thread; //hci_thread
#else
#include <hardware/bt_av.h>
#endif

#include "bt_utils.h"
#include "bta_api.h"
#include "btif_media.h"
#include "btif_profile_queue.h"
#include "btif_util.h"
#include "btu.h"
#include "bt_common.h"
#include "osi/include/allocator.h"
#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
#include "interop_mtk.h"
#endif

#if defined(MTK_A2DP_SNK_AAC_CODEC) && (MTK_A2DP_SNK_AAC_CODEC == TRUE)
#include "a2d_aac.h"
#endif

#include "a2d_vendor.h"
#include "a2d_lhdc.h"

/*****************************************************************************
**  Constants & Macros
******************************************************************************/
#define BTIF_AV_SERVICE_NAME "Advanced Audio"
#define BTIF_AVK_SERVICE_NAME "Advanced Audio Sink"

#define BTIF_TIMEOUT_AV_OPEN_ON_RC_MS  (2 * 1000)

#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
#define BTIF_TIMER_A2DP_DELAY_START_CMD  (1 * 1000)
#endif

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#define BTIF_TIMEOUT_AV_OPEN_RC_MS (3 * 1000)
#endif

#if defined(MTK_A2DP_SRC_STE_CODEC) && (MTK_A2DP_SRC_STE_CODEC == TRUE) || defined(MTK_A2DP_SNK_STE_CODEC) && (MTK_A2DP_SNK_STE_CODEC == TRUE)
#include"btm_int.h" //for get acl handle from btm layer in btif layer
#include "vendor.h" //send ioctl cmd from btif to  driver

typedef struct  {
    UINT16 handle; //acl handle
    UINT8 method;  //GPIO &auto event
    UINT32 period; //the interval to trigger notify BTCLK&sysclk
    UINT16 active_slots;//should be less than period
}bt_stereo_para; // parameter of ioctl cmd to driver to enabl e btclk &sysclk feature
#endif //MTK_A2DP_SRC_STE_CODEC  &&MTK_A2DP_SRC_STE_CODEC
#if defined(MTK_A2DP_SRC_DUMP_PCM_DATA) && (MTK_A2DP_SRC_DUMP_PCM_DATA == TRUE)
FILE *src_outputPcmSampleFile;
char src_outputFilename[50] = "/data/misc/bluedroid/src_output_sample.pcm";
#endif

typedef enum {
    BTIF_AV_STATE_IDLE = 0x0,
    BTIF_AV_STATE_OPENING,
    BTIF_AV_STATE_OPENED,
    BTIF_AV_STATE_STARTED,
    BTIF_AV_STATE_CLOSING
} btif_av_state_t;

/* Should not need dedicated suspend state as actual actions are no
   different than open state. Suspend flags are needed however to prevent
   media task from trying to restart stream during remote suspend or while
   we are in the process of a local suspend */

#define BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING 0x1
#define BTIF_AV_FLAG_REMOTE_SUSPEND        0x2
#define BTIF_AV_FLAG_PENDING_START         0x4
#define BTIF_AV_FLAG_PENDING_STOP          0x8
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#define BTIF_AV_FLAG_REMOTE_SUSPENDING     0x20
#endif
/*****************************************************************************
**  Local type definitions
******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
typedef struct
{
    UINT8 codec_type;
    UINT8 codec_info[AVDT_CODEC_SIZE];
} btif_av_avk_config;
#endif

typedef struct
{
    tBTA_AV_HNDL bta_handle;
    bt_bdaddr_t peer_bda;
    btif_sm_handle_t sm_handle;
    UINT8 flags;
    tBTA_AV_EDR edr;
    UINT8 peer_sep;  /* sep type of peer device */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_avk_config avk_config;
    UINT8 in_use; /* Used to check the cb is in use or not*/
    BOOLEAN active; /* is active */
#endif
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
    UINT16 uuid;  /* uuid of initiator */
#endif
} btif_av_cb_t;

typedef struct
{
    bt_bdaddr_t *target_bda;
    uint16_t uuid;
} btif_av_connect_req_t;

typedef struct
{
    int sample_rate;
    int channel_count;
    bt_bdaddr_t peer_bd;
} btif_av_sink_config_req_t;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
typedef struct
{
    bt_bdaddr_t *target_bda;
} btif_av_active_req_t;
#endif

#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
typedef struct
{
    UINT8 codec_type;
    UINT8 enable;
} btif_av_codec_enable_req_t;
#endif

/*****************************************************************************
**  Static variables
******************************************************************************/
static btav_callbacks_t *bt_av_src_callbacks = NULL;
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
static btav_src_ext_callbacks_t *bt_av_src_ext_callbacks = NULL;
static btav_sink_ext_callbacks_t *bt_av_sink_ext_callbacks = NULL;
static btav_ext_callbacks_t *bt_av_ext_callbacks = NULL; // for both A2DP SRC & SNK
#endif
static btav_callbacks_t *bt_av_sink_callbacks = NULL;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static btif_av_cb_t btif_av_cbs[BTA_AV_NUM_CONNS];
#else
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
static btif_av_cb_t btif_av_cb = {0, {{0}}, 0, 0, 0, 0, 0, 0};
#else
static btif_av_cb_t btif_av_cb = {0, {{0}}, 0, 0, 0, 0, 0};
#endif
#endif


#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static alarm_t *av_open_on_rc_timers[BTA_AV_NUM_CONNS] = {NULL};
#else
static alarm_t *av_open_on_rc_timer = NULL;
#endif

#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static alarm_t *tle_av_delay_start_cmds[BTA_AV_NUM_CONNS] = {NULL};
#else
static alarm_t *tle_av_delay_start_cmd = NULL;
#endif
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static int btif_av_sink_max_cnt = 2; // to support multipoint
static int btif_av_src_max_cnt = 2;
#endif

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#include <pthread.h>
static pthread_mutex_t btif_av_lock = PTHREAD_MUTEX_INITIALIZER;
#define BTIF_AV_LOCK() do{                      \
        pthread_mutex_lock(&btif_av_lock);      \
    } while(0)

#define BTIF_AV_UNLOCK() do{                      \
        pthread_mutex_unlock(&btif_av_lock);      \
    } while(0)
#else
#define BTIF_AV_LOCK()
#define BTIF_AV_UNLOCK()
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
/* both interface and media task needs to be ready to alloc incoming request */
#define CHECK_BTAV_INIT() if (((bt_av_src_callbacks == NULL) && (bt_av_sink_callbacks == NULL)) \
        || (btif_av_cbs[0].sm_handle == NULL))\
{\
     BTIF_TRACE_WARNING("%s: BTAV not initialized", __FUNCTION__);\
     return BT_STATUS_NOT_READY;\
}\
else\
{\
     BTIF_TRACE_EVENT("%s", __FUNCTION__);\
}
#else
/* both interface and media task needs to be ready to alloc incoming request */
#define CHECK_BTAV_INIT() if (((bt_av_src_callbacks == NULL) &&(bt_av_sink_callbacks == NULL)) \
        || (btif_av_cb.sm_handle == NULL))\
{\
     BTIF_TRACE_WARNING("%s: BTAV not initialized", __FUNCTION__);\
     return BT_STATUS_NOT_READY;\
}\
else\
{\
     BTIF_TRACE_EVENT("%s", __FUNCTION__);\
}
#endif

/* Helper macro to avoid code duplication in the state machine handlers */
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
 #define CHECK_RC_EVENT(e, d)       \
  case BTA_AV_RC_OPEN_EVT:         \
  case BTA_AV_RC_BROWSE_OPEN_EVT:  \
  case BTA_AV_RC_CLOSE_EVT:        \
  case BTA_AV_RC_BROWSE_CLOSE_EVT: \
  case BTA_AV_REMOTE_CMD_EVT:      \
  case BTA_AV_VENDOR_CMD_EVT:      \
  case BTA_AV_META_MSG_EVT:        \
  case BTA_AV_RC_FEAT_EVT:         \
  case BTA_AV_REMOTE_RSP_EVT: {    \
    btif_rc_handler(e, d);         \
  } break;
#else
 #define CHECK_RC_EVENT(e, d) \
    case BTA_AV_RC_OPEN_EVT: \
    case BTA_AV_RC_CLOSE_EVT: \
    case BTA_AV_REMOTE_CMD_EVT: \
    case BTA_AV_VENDOR_CMD_EVT: \
    case BTA_AV_META_MSG_EVT: \
    case BTA_AV_RC_FEAT_EVT: \
    case BTA_AV_REMOTE_RSP_EVT: \
    { \
         btif_rc_handler(e, d);\
    }break;
#endif //(end -- #if defined(MTK_AVRCP_TG_15_BROWSE))

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *p_data, void *p_cb);
static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *data, void *p_cb);
static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *data, void *p_cb);
static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *data, void *p_cb);
static BOOLEAN btif_av_state_closing_handler(btif_sm_event_t event, void *data, void *p_cb);

static int btif_av_get_dev_idx_by_bda(const bt_bdaddr_t* bd_addr);
static int btif_av_get_dev_idx_by_handle(const UINT32 handle);
static int btif_av_find_or_alloc_dev_idx(const bt_bdaddr_t* bd_addr);
static int btif_av_get_active_idx(void);
static int btif_av_assign_bta_handle(tBTA_AV_HNDL handle);
static void btif_av_get_avk_codec_config(btif_av_avk_config *avk_config,
    btif_av_sink_config_req_t *config_req);
#else
static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *p_data);
static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *data);
static BOOLEAN btif_av_state_closing_handler(btif_sm_event_t event, void *data);
static void btif_av_handle_media_sink_cfg(tBTA_AV_MEDIA *p_data);
#endif


#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
static bt_status_t btif_av_get_codec_config(bt_bdaddr_t *bd_addr);
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == FALSE)
static void btif_av_handle_media_sink_cfg(tBTA_AV_MEDIA *p_data);
#endif

static const btif_sm_handler_t btif_av_state_handlers[] =
{
    btif_av_state_idle_handler,
    btif_av_state_opening_handler,
    btif_av_state_opened_handler,
    btif_av_state_started_handler,
    btif_av_state_closing_handler
};

static void btif_av_event_free_data(btif_sm_event_t event, void *p_data);

/*************************************************************************
** Extern functions
*************************************************************************/
extern void btif_rc_handler(tBTA_AV_EVT event, tBTA_AV *p_data);
extern BOOLEAN btif_rc_get_connected_peer(BD_ADDR peer_addr);

#if defined(MTK_A2DP_MULTI_AVRCP) && (MTK_A2DP_MULTI_AVRCP == TRUE)
extern UINT8 btif_rc_get_connected_peer_handle(BD_ADDR peer_addr);
extern int btif_rc_get_device_addr_by_handle(uint8_t handle, bt_bdaddr_t *addr);
#else
extern UINT8 btif_rc_get_connected_peer_handle(void);
#endif
extern void btif_rc_check_handle_pending_play (BD_ADDR peer_addr, BOOLEAN bSendToApp);

#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
extern void btif_rc_check_pending_cmd (BD_ADDR peer_addr);
#endif

extern fixed_queue_t *btu_general_alarm_queue;

/*****************************************************************************
** Local helper functions
******************************************************************************/

const char *dump_av_sm_state_name(btif_av_state_t state)
{
    switch (state)
    {
        CASE_RETURN_STR(BTIF_AV_STATE_IDLE)
        CASE_RETURN_STR(BTIF_AV_STATE_OPENING)
        CASE_RETURN_STR(BTIF_AV_STATE_OPENED)
        CASE_RETURN_STR(BTIF_AV_STATE_STARTED)
        CASE_RETURN_STR(BTIF_AV_STATE_CLOSING)
        default: return "UNKNOWN_STATE";
    }
}

const char *dump_av_sm_event_name(btif_av_sm_event_t event)
{
    switch((int)event)
    {
        CASE_RETURN_STR(BTA_AV_ENABLE_EVT)
        CASE_RETURN_STR(BTA_AV_REGISTER_EVT)
        CASE_RETURN_STR(BTA_AV_OPEN_EVT)
        CASE_RETURN_STR(BTA_AV_CLOSE_EVT)
        CASE_RETURN_STR(BTA_AV_START_EVT)
        CASE_RETURN_STR(BTA_AV_STOP_EVT)
        CASE_RETURN_STR(BTA_AV_PROTECT_REQ_EVT)
        CASE_RETURN_STR(BTA_AV_PROTECT_RSP_EVT)
        CASE_RETURN_STR(BTA_AV_RC_OPEN_EVT)
        CASE_RETURN_STR(BTA_AV_RC_CLOSE_EVT)
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
        CASE_RETURN_STR(BTA_AV_RC_BROWSE_OPEN_EVT)
        CASE_RETURN_STR(BTA_AV_RC_BROWSE_CLOSE_EVT)
#endif
        CASE_RETURN_STR(BTA_AV_REMOTE_CMD_EVT)
        CASE_RETURN_STR(BTA_AV_REMOTE_RSP_EVT)
        CASE_RETURN_STR(BTA_AV_VENDOR_CMD_EVT)
        CASE_RETURN_STR(BTA_AV_VENDOR_RSP_EVT)
        CASE_RETURN_STR(BTA_AV_RECONFIG_EVT)
        CASE_RETURN_STR(BTA_AV_SUSPEND_EVT)
        CASE_RETURN_STR(BTA_AV_PENDING_EVT)
        CASE_RETURN_STR(BTA_AV_META_MSG_EVT)
        CASE_RETURN_STR(BTA_AV_REJECT_EVT)
        CASE_RETURN_STR(BTA_AV_RC_FEAT_EVT)
        CASE_RETURN_STR(BTA_AV_OFFLOAD_START_RSP_EVT)
        CASE_RETURN_STR(BTIF_SM_ENTER_EVT)
        CASE_RETURN_STR(BTIF_SM_EXIT_EVT)
        CASE_RETURN_STR(BTIF_AV_CONNECT_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_DISCONNECT_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_START_STREAM_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_STOP_STREAM_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_SUSPEND_STREAM_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_SINK_CONFIG_REQ_EVT)
        CASE_RETURN_STR(BTIF_AV_OFFLOAD_START_REQ_EVT)
#ifdef USE_AUDIO_TRACK
        CASE_RETURN_STR(BTIF_AV_SINK_FOCUS_REQ_EVT)
#endif
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
        CASE_RETURN_STR(BTIF_AV_CODEC_ENABLE_REQ_EVT)
#endif
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        CASE_RETURN_STR(BTIF_AV_ACTIVE_REQ_EVT)
#endif
        CASE_RETURN_STR(BTA_AV_MEDIA_SINK_CFG_EVT)
        default: return "UNKNOWN_EVENT";
   }
}

/****************************************************************************
**  Local helper functions
*****************************************************************************/
/*******************************************************************************
**
** Function         btif_initiate_av_open_timer_timeout
**
** Description      Timer to trigger AV open if the remote headset establishes
**                  RC connection w/o AV connection. The timer is needed to IOP
**                  with headsets that do establish AV after RC connection.
**
** Returns          void
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static void btif_initiate_av_open_timer_timeout(void *data)
#else
static void btif_initiate_av_open_timer_timeout(UNUSED_ATTR void *data)
#endif
{
    BD_ADDR peer_addr;
    btif_av_connect_req_t connect_req;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)data-btif_av_cbs]
    bdcpy(peer_addr, btif_av_cb.peer_bda.address);
#endif

    /* is there at least one RC connection - There should be */
    if (btif_rc_get_connected_peer(peer_addr)) {
#if !defined(MTK_A2DP_DUAL_AUDIO) || (MTK_A2DP_DUAL_AUDIO == FALSE)
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        if (btif_sm_get_state(btif_av_cb.sm_handle) >= BTIF_AV_STATE_OPENING
            && !bdaddr_equals((bt_bdaddr_t *)&peer_addr, &btif_av_cb.peer_bda))
        {
            BTIF_TRACE_DEBUG("%s other dev is on going, disconnect rc", __FUNCTION__);
            BTA_AvCloseRc(btif_av_cb.bta_handle);
            return;
        }
#endif
#endif
       BTIF_TRACE_DEBUG("%s Issuing connect to the remote RC peer", __FUNCTION__);
       /* In case of AVRCP connection request, we will initiate SRC connection */
       connect_req.target_bda = (bt_bdaddr_t*)&peer_addr;
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
       if (btif_av_both_enable() &&
        (UUID_SERVCLASS_AUDIO_SINK == btif_av_cb.uuid
            || UUID_SERVCLASS_AUDIO_SOURCE == btif_av_cb.uuid))
       {
            connect_req.uuid = btif_av_cb.uuid;
            btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CONNECT_REQ_EVT, (char*)&connect_req);
            return;
       }
#endif
       if (bt_av_src_callbacks != NULL)
           connect_req.uuid = UUID_SERVCLASS_AUDIO_SOURCE;
       else if(bt_av_sink_callbacks != NULL)
           connect_req.uuid = UUID_SERVCLASS_AUDIO_SINK;
       btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CONNECT_REQ_EVT, (char*)&connect_req);
    }
    else
    {
        BTIF_TRACE_ERROR("%s No connected RC peers", __FUNCTION__);
    }

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*****************************************************************************
**  Static functions
******************************************************************************/

/*******************************************************************************
**
** Function         btif_report_connection_state
**
** Description      Updates the components via the callbacks about the connection
**                  state of a2dp connection.
**
** Returns          None
**
*******************************************************************************/
static void btif_report_connection_state(btav_connection_state_t state, bt_bdaddr_t *bd_addr)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[idx]
int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (-1 == idx)
    {
        return;
    }
#endif

#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
    // callback the configured codec to the MW layer
    if (state == BTAV_CONNECTION_STATE_CONNECTED)
    {
        btif_av_get_codec_config(bd_addr);
    }
#endif
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
    if (btif_av_both_enable())
    {
        if (bt_av_sink_callbacks != NULL
          && (btif_av_cb.peer_sep == AVDT_TSEP_SRC
          || (btif_av_cb.uuid == UUID_SERVCLASS_AUDIO_SINK
          && btif_av_cb.peer_sep == AVDT_TSEP_INVALID))) {
            HAL_CBACK(bt_av_sink_callbacks, connection_state_cb, state, bd_addr);
        }
        else if (bt_av_src_callbacks != NULL)
        {
            HAL_CBACK(bt_av_src_callbacks, connection_state_cb, state, bd_addr);
        }
        return;
    }
#endif

    if (bt_av_sink_callbacks != NULL) {
        HAL_CBACK(bt_av_sink_callbacks, connection_state_cb, state, bd_addr);
    } else if (bt_av_src_callbacks != NULL) {
        HAL_CBACK(bt_av_src_callbacks, connection_state_cb, state, bd_addr);
    }

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*******************************************************************************
**
** Function         btif_report_audio_state
**
** Description      Updates the components via the callbacks about the audio
**                  state of a2dp connection. The state is updated when either
**                  the remote ends starts streaming (started state) or whenever
**                  it transitions out of started state (to opened or streaming)
**                  state.
**
** Returns          None
**
*******************************************************************************/
static void btif_report_audio_state(btav_audio_state_t state, bt_bdaddr_t *bd_addr)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[idx]
int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (-1 == idx)
    {
        return;
    }
#endif

#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
    if (bt_av_sink_callbacks != NULL && btif_av_cb.peer_sep == AVDT_TSEP_SRC) {
        HAL_CBACK(bt_av_sink_callbacks, audio_state_cb, state, bd_addr);
    } else if (bt_av_src_callbacks != NULL && btif_av_cb.peer_sep == AVDT_TSEP_SNK) {
        HAL_CBACK(bt_av_src_callbacks, audio_state_cb, state, bd_addr);
    }
#else
    if (bt_av_sink_callbacks != NULL) {
        HAL_CBACK(bt_av_sink_callbacks, audio_state_cb, state, bd_addr);
    } else if (bt_av_src_callbacks != NULL) {
        HAL_CBACK(bt_av_src_callbacks, audio_state_cb, state, bd_addr);
    }
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

#if (defined(MTK_A2DP_SRC_STE_CODEC) && (MTK_A2DP_SRC_STE_CODEC == TRUE)) \
 || (defined(MTK_A2DP_SNK_STE_CODEC) && (MTK_A2DP_SNK_STE_CODEC == TRUE))
void btif_av_enable_btclk(bt_bdaddr_t *addr, BOOLEAN enable, UINT32 period)
{
    /*get current acl handle,then send ioctl cmd to driver to enable read btclk&sysclk function */
    tACL_CONN *acl_handle = btm_bda_to_acl ((UINT8 *)(addr->address), BT_TRANSPORT_BR_EDR);
    /*enable read btclk and sysclk features*/
    bt_stereo_para enable_clk_para = {0};

    APPL_TRACE_WARNING("btstereo is enable=%d, period=0x%x", enable, period);

    if (acl_handle == NULL)
    {
        BTIF_TRACE_WARNING("current acl handle is NULL");
        enable_clk_para.handle = 0;
    }
    else
    {
        enable_clk_para.handle = acl_handle->hci_handle;
    }

    if (TRUE == enable)
    {
        enable_clk_para.method = 0x11;//GPIO,auto event
        enable_clk_para.period = period;
        enable_clk_para.active_slots = 32;//20/0.625ms=32
    }
    else
    {
        enable_clk_para.period = 0; //period set 0 means close the get btclk&sysclk feature
    }
    vendor_get_interface()->send_command(
           (vendor_opcode_t)BT_VND_OP_A2DP_ENABLE_BTSYSCLK, (void*)&enable_clk_para);
    APPL_TRACE_DEBUG("handle = %d, method = %d, period = %d, active_slots = %d",
        enable_clk_para.handle, enable_clk_para.method, enable_clk_para.period,
        enable_clk_para.active_slots);
}
#endif

/*****************************************************************************
**
** Function     btif_av_state_idle_handler
**
** Description  State managing disconnected AV link
**
** Returns      TRUE if event was processed, FALSE otherwise
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *p_data, void *p_cb)
#else
static BOOLEAN btif_av_state_idle_handler(btif_sm_event_t event, void *p_data)
#endif
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)p_cb-btif_av_cbs]
    BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda, "");
#endif

    BTIF_TRACE_DEBUG("%s event:%s flags %x", __FUNCTION__,
                     dump_av_sm_event_name(event), btif_av_cb.flags);

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
            /* clear the peer_bda */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#else
            memset(&btif_av_cb.peer_bda, 0, sizeof(bt_bdaddr_t));
#endif
            btif_av_cb.flags = 0;
            btif_av_cb.edr = 0;
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
            btif_av_cb.peer_sep = AVDT_TSEP_INVALID;
            btif_av_cb.uuid = 0;
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda,
                "free btif_av_cbs[%d], rc is connected %d",
                (btif_av_cb_t*)p_cb-btif_av_cbs,
                btif_rc_get_connected_peer(btif_av_cb.peer_bda.address));
            btif_a2dp_on_idle(&btif_av_cb.peer_bda);

            if (FALSE == btif_rc_get_connected_peer(btif_av_cb.peer_bda.address))
            {
                btif_av_cb.active = 0;
                btif_av_cb.in_use = 0;

                memset(&btif_av_cb.peer_bda, 0, sizeof(bt_bdaddr_t));
            }
            else
            {
                BTIF_TRACE_DEBUG("btif_av_state_idle_handler:BTIF_AV_CONNECT_REQ_EVT");
            }
#else
            btif_a2dp_on_idle();
#endif

            break;

        case BTIF_SM_EXIT_EVT:
            break;

        case BTA_AV_ENABLE_EVT:
            break;

        case BTA_AV_REGISTER_EVT:
            btif_av_cb.bta_handle = ((tBTA_AV*)p_data)->registr.hndl;
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            if (!bdaddr_is_empty(&btif_av_cb.peer_bda))
            {
                btif_av_connect_req_t connect_req;

                BTIF_TRACE_WARNING("registered, do pending conn req");

                connect_req.target_bda = &btif_av_cb.peer_bda;

                if(bt_av_sink_callbacks != NULL)
                   connect_req.uuid = UUID_SERVCLASS_AUDIO_SINK;
               else if(bt_av_src_callbacks != NULL)
                   connect_req.uuid = UUID_SERVCLASS_AUDIO_SOURCE;
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                if (btif_av_both_enable())
                {
                    connect_req.uuid = btif_av_cb.uuid;
                }
#endif
                btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CONNECT_REQ_EVT, (char*)&connect_req);
            }
#endif
            break;

        case BTA_AV_PENDING_EVT:
        case BTIF_AV_CONNECT_REQ_EVT:
        {
            if (event == BTIF_AV_CONNECT_REQ_EVT)
            {
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
                /* if disable HFP, a2dp connect req will be clear from connectq
                 */
                if (0 == ((btif_av_connect_req_t*)p_data)->uuid)
                {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                    BTIF_TRACE_DEBUG("btif_av_state_idle_handler:BTIF_AV_CONNECT_REQ_EVT");
#endif
                    btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED,
                        ((btif_av_connect_req_t*)p_data)->target_bda);
                    break;
                }
#endif
                memcpy(&btif_av_cb.peer_bda, ((btif_av_connect_req_t*)p_data)->target_bda,
                    sizeof(bt_bdaddr_t));
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                btif_av_cb.uuid = ((btif_av_connect_req_t*)p_data)->uuid;
#endif

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
                /* if connect req is too quickly and register event is not
                 * received from bta, so it should pending and do it again in
                 * case BTA_AV_REGISTER_EVT
                 */
                if (btif_av_cb.bta_handle != 0)
                {
                    BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                        TRUE, BTA_SEC_AUTHENTICATE, ((btif_av_connect_req_t*)p_data)->uuid);

                }
                else
                {
                    BTIF_TRACE_WARNING("not register, pending and check next");
                    break;
                }
#else
                BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                        TRUE, BTA_SEC_AUTHENTICATE, ((btif_av_connect_req_t*)p_data)->uuid);
#endif
            }
            else if (event == BTA_AV_PENDING_EVT)
            {
                bdcpy(btif_av_cb.peer_bda.address, ((tBTA_AV*)p_data)->pend.bd_addr);
                if (bt_av_src_callbacks != NULL)
                {
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                    BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                      TRUE, BTA_SEC_AUTHENTICATE, AVDT_TSEP_SRC);
                    btif_av_cb.uuid = UUID_SERVCLASS_AUDIO_SOURCE;
#else
                    BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                      TRUE, BTA_SEC_AUTHENTICATE, UUID_SERVCLASS_AUDIO_SOURCE);
#endif
                }
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                else
#endif
                if (bt_av_sink_callbacks != NULL)
                {
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                    BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                                TRUE, BTA_SEC_AUTHENTICATE, AVDT_TSEP_SNK);
                    btif_av_cb.uuid = UUID_SERVCLASS_AUDIO_SINK;
#else
                    BTA_AvOpen(btif_av_cb.peer_bda.address, btif_av_cb.bta_handle,
                                TRUE, BTA_SEC_AUTHENTICATE, UUID_SERVCLASS_AUDIO_SINK);
#endif
                }
            }
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_OPENING);
        } break;

        case BTA_AV_RC_OPEN_EVT:
            /* IOP_FIX: Jabra 620 only does RC open without AV open whenever it connects. So
             * as per the AV WP, an AVRC connection cannot exist without an AV connection. Therefore,
             * we initiate an AV connection if an RC_OPEN_EVT is received when we are in AV_CLOSED state.
             * We initiate the AV connection after a small 3s timeout to avoid any collisions from the
             * headsets, as some headsets initiate the AVRC connection first and then
             * immediately initiate the AV connection
             *
             * TODO: We may need to do this only on an AVRCP Play. FixMe
             */

            BTIF_TRACE_DEBUG("BTA_AV_RC_OPEN_EVT received w/o AV");
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            alarm_set_on_queue(av_open_on_rc_timers[(btif_av_cb_t*)p_cb-btif_av_cbs],
                               BTIF_TIMEOUT_AV_OPEN_ON_RC_MS,
                               btif_initiate_av_open_timer_timeout, &btif_av_cb,
#else
            alarm_set_on_queue(av_open_on_rc_timer,
                               BTIF_TIMEOUT_AV_OPEN_ON_RC_MS,
                               btif_initiate_av_open_timer_timeout, NULL,
#endif
                               btu_general_alarm_queue);
            btif_rc_handler(event, p_data);
            break;

#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
        case BTA_AV_RC_BROWSE_OPEN_EVT:
            BTIF_TRACE_DEBUG("BTA_AV_RC_BROWSE_OPEN_EVT received");
            btif_rc_handler(event, (tBTA_AV*)p_data);
            break;
#endif
           /*
            * In case Signalling channel is not down
            * and remote started Streaming Procedure
            * we have to handle config and open event in
            * idle_state. We hit these scenarios while running
            * PTS test case for AVRCP Controller
            */
        case BTIF_AV_SINK_CONFIG_REQ_EVT:
        {
            btif_av_sink_config_req_t req;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            memcpy(&btif_av_cb.avk_config, p_data, sizeof(btif_av_cb.avk_config));
            btif_av_get_avk_codec_config(&btif_av_cb.avk_config, &req);
            bdaddr_copy(&req.peer_bd, &btif_av_cb.peer_bda);
#else
            // copy to avoid alignment problems
            memcpy(&req, p_data, sizeof(req));
#endif

            BTIF_TRACE_WARNING("BTIF_AV_SINK_CONFIG_REQ_EVT %d %d", req.sample_rate,
                    req.channel_count);
            if (bt_av_sink_callbacks != NULL) {
                HAL_CBACK(bt_av_sink_callbacks, audio_config_cb, &(req.peer_bd),
                        req.sample_rate, req.channel_count);
            }
        } break;

        case BTA_AV_OPEN_EVT:
        {
            tBTA_AV *p_bta_data = (tBTA_AV*)p_data;
            btav_connection_state_t state;
            btif_sm_state_t av_state;
            BTIF_TRACE_DEBUG("status:%d, edr 0x%x",p_bta_data->open.status,
                               p_bta_data->open.edr);

            if (p_bta_data->open.status == BTA_AV_SUCCESS)
            {
                 state = BTAV_CONNECTION_STATE_CONNECTED;
                 av_state = BTIF_AV_STATE_OPENED;
                 btif_av_cb.edr = p_bta_data->open.edr;
                 btif_av_cb.peer_sep = p_bta_data->open.sep;
                 btif_a2dp_set_peer_sep(p_bta_data->open.sep);
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                 btif_rc_check_pending_cmd(p_bta_data->open.bd_addr);
#endif
            }
            else
            {
                BTIF_TRACE_WARNING("BTA_AV_OPEN_EVT::FAILED status: %d",
                                     p_bta_data->open.status );
                state = BTAV_CONNECTION_STATE_DISCONNECTED;
                av_state  = BTIF_AV_STATE_IDLE;
            }
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            /* when signal channel is not disconnect and open again, there is no
             * pending event and connect req, so this address should be set here
             */
            if(bdaddr_is_empty(&btif_av_cb.peer_bda))
            {
                memcpy(&btif_av_cb.peer_bda, p_bta_data->open.bd_addr, sizeof(bt_bdaddr_t));
            }
#endif
            /* inform the application of the event */
            btif_report_connection_state(state, &(btif_av_cb.peer_bda));
            /* change state to open/idle based on the status */
            btif_sm_change_state(btif_av_cb.sm_handle, av_state);
#if (defined(MTK_A2DP_SRC_DUMP_PCM_DATA) && (MTK_A2DP_SRC_DUMP_PCM_DATA == TRUE))
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                src_outputPcmSampleFile = fopen(src_outputFilename, "ab");
                if (src_outputPcmSampleFile != NULL)
                {
                    BTIF_TRACE_DEBUG("%s() open file %s success.",__FUNCTION__, src_outputFilename);
                }
                else
                {
                    BTIF_TRACE_ERROR("%s() open file %s fail!.",__FUNCTION__, src_outputFilename);
                }
            }
#endif
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                /* if queued PLAY command,  send it now */
                btif_rc_check_handle_pending_play(p_bta_data->open.bd_addr,
                                             (p_bta_data->open.status == BTA_AV_SUCCESS));
            }
            else if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
                /* if queued PLAY command,  send it now */
                btif_rc_check_handle_pending_play(p_bta_data->open.bd_addr, FALSE);
                /* Bring up AVRCP connection too */
                BTA_AvOpenRc(btif_av_cb.bta_handle);
            }
            btif_queue_advance();
        } break;
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
        case BTIF_AV_CODEC_ENABLE_REQ_EVT:
        {
            btif_av_codec_enable_req_t *data = (btif_av_codec_enable_req_t*)p_data;
            BTA_AvCodecEnable(data->codec_type , data->enable);
        }break;
#endif
        case BTA_AV_REMOTE_CMD_EVT:
        case BTA_AV_VENDOR_CMD_EVT:
        case BTA_AV_META_MSG_EVT:
        case BTA_AV_RC_FEAT_EVT:
        case BTA_AV_REMOTE_RSP_EVT:
            btif_rc_handler(event, (tBTA_AV*)p_data);
            break;

        case BTA_AV_RC_CLOSE_EVT:
            BTIF_TRACE_DEBUG("BTA_AV_RC_CLOSE_EVT: Stopping AV timer.");

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            alarm_cancel(av_open_on_rc_timers[(btif_av_cb_t*)p_cb-btif_av_cbs]);
            /* if a2dp disconnected firstly, rc close to free the cb */
            btif_av_cb.active = 0;
            btif_av_cb.in_use = 0;
            memset(&btif_av_cb.peer_bda, 0, sizeof(bt_bdaddr_t));
#else
            alarm_cancel(av_open_on_rc_timer);
#endif
            btif_rc_handler(event, p_data);
            break;

        case BTIF_AV_OFFLOAD_START_REQ_EVT:
            BTIF_TRACE_ERROR("BTIF_AV_OFFLOAD_START_REQ_EVT: Stream not Started IDLE");
            btif_a2dp_on_offload_started(BTA_AV_FAIL);
            break;

        default:
            BTIF_TRACE_WARNING("%s : unhandled event:%s", __FUNCTION__,
                                dump_av_sm_event_name(event));
            return FALSE;

    }

    return TRUE;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}
/*****************************************************************************
**
** Function        btif_av_state_opening_handler
**
** Description     Intermediate state managing events during establishment
**                 of avdtp channel
**
** Returns         TRUE if event was processed, FALSE otherwise
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *p_data, void *p_cb)
#else
static BOOLEAN btif_av_state_opening_handler(btif_sm_event_t event, void *p_data)
#endif
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)p_cb-btif_av_cbs]
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    tBTA_AV *p_bta_data = NULL;
    BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda,"");
#endif
    BTIF_TRACE_DEBUG("%s event:%s flags %x", __FUNCTION__,
                     dump_av_sm_event_name(event), btif_av_cb.flags);
    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
            /* inform the application that we are entering connecting state */
            btif_report_connection_state(BTAV_CONNECTION_STATE_CONNECTING, &(btif_av_cb.peer_bda));
            break;

        case BTIF_SM_EXIT_EVT:
            break;

        case BTA_AV_REJECT_EVT:
            BTIF_TRACE_DEBUG(" Received  BTA_AV_REJECT_EVT ");
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
            break;

        case BTA_AV_OPEN_EVT:
        {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            p_bta_data = (tBTA_AV*)p_data;
#else
            tBTA_AV *p_bta_data = (tBTA_AV*)p_data;
#endif
            btav_connection_state_t state;
            btif_sm_state_t av_state;
            BTIF_TRACE_DEBUG("status:%d, edr 0x%x",p_bta_data->open.status,
                               p_bta_data->open.edr);
            if (p_bta_data->open.status == BTA_AV_SUCCESS)
            {
                 state = BTAV_CONNECTION_STATE_CONNECTED;
                 av_state = BTIF_AV_STATE_OPENED;
                 btif_av_cb.edr = p_bta_data->open.edr;
                 btif_av_cb.peer_sep = p_bta_data->open.sep;
                 btif_a2dp_set_peer_sep(p_bta_data->open.sep);
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
                 btif_rc_check_pending_cmd(p_bta_data->open.bd_addr);
#endif
            }
            else
            {
                BTIF_TRACE_WARNING("BTA_AV_OPEN_EVT::FAILED status: %d",
                                     p_bta_data->open.status );
                BD_ADDR peer_addr;
#if defined(MTK_A2DP_MULTI_AVRCP) && (MTK_A2DP_MULTI_AVRCP == TRUE)
                if (btif_rc_get_connected_peer(btif_av_cb.peer_bda.address))
#else
                if ((btif_rc_get_connected_peer(peer_addr))
                    &&(!bdcmp(btif_av_cb.peer_bda.address, peer_addr)))
#endif
                {
                    /*
                     * Disconnect AVRCP connection, if
                     * A2DP conneciton failed, for any reason
                     */
                    BTIF_TRACE_WARNING(" Disconnecting AVRCP ");
#if defined(MTK_A2DP_MULTI_AVRCP) && (MTK_A2DP_MULTI_AVRCP == TRUE)
                    BTA_AvCloseRc(btif_rc_get_connected_peer_handle(p_bta_data->open.bd_addr));
#else
                    BTA_AvCloseRc(btif_rc_get_connected_peer_handle());
#endif
                }
                state = BTAV_CONNECTION_STATE_DISCONNECTED;
                av_state  = BTIF_AV_STATE_IDLE;
            }

            /* inform the application of the event */
            btif_report_connection_state(state, &(btif_av_cb.peer_bda));
            /* change state to open/idle based on the status */
            btif_sm_change_state(btif_av_cb.sm_handle, av_state);
            if (BTIF_AV_STATE_OPENED == av_state)
            {
#if (defined(MTK_A2DP_SRC_STE_CODEC) && (MTK_A2DP_SRC_STE_CODEC == TRUE)) \
     || (defined(MTK_A2DP_SNK_STE_CODEC) && (MTK_A2DP_SNK_STE_CODEC == TRUE))
                if (BTIF_AV_CODEC_STE == bta_av_get_current_codec())
                {
                    /* 100ms=0xa0, 1000ms=0x640 */
                    btif_av_enable_btclk(&(btif_av_cb.peer_bda), TRUE, 0xa0);
                }
#endif

#if (defined(MTK_A2DP_SRC_DUMP_PCM_DATA) && (MTK_A2DP_SRC_DUMP_PCM_DATA == TRUE))
                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                {
                    src_outputPcmSampleFile = fopen(src_outputFilename, "ab");
                    if (src_outputPcmSampleFile != NULL)
                    {
                        BTIF_TRACE_DEBUG("%s() open file %s success.",__FUNCTION__, src_outputFilename);
                    }
                    else
                    {
                        BTIF_TRACE_ERROR("%s() open file %s fail!.",__FUNCTION__, src_outputFilename);
                    }
                }
#endif
                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                {
                    /* if queued PLAY command,  send it now */
                    btif_rc_check_handle_pending_play(p_bta_data->open.bd_addr,
                                                 (p_bta_data->open.status == BTA_AV_SUCCESS));
                }
                else if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
                {
                    /* if queued PLAY command,  send it now */
                    btif_rc_check_handle_pending_play(p_bta_data->open.bd_addr, FALSE);
                    /* Bring up AVRCP connection too */
                    BTA_AvOpenRc(btif_av_cb.bta_handle);
                }
#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE)  \
        && defined(MTK_PTS_AV_SEC_CTRL) && (MTK_PTS_AV_SEC_CTRL == TRUE) //tt pts test -- to send Security Control.
                //for AVDTP/SNK/INT/SIG/SEC/BV-01-C
                //    AVDTP/SRC/INT/SIG/SEC/BV-01-C
                BTA_AvProtectReq(btif_av_cb.bta_handle, NULL, 0);
#endif
            }

            btif_queue_advance();
        } break;
#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE) \
    && defined(MTK_PTS_AV_SEC_CTRL) && (MTK_PTS_AV_SEC_CTRL == TRUE)  //tt pts test --  for AVDTP/SNK/ACP/SIG/SEC/BV-02-C to do Security Conrol response.
        case BTA_AV_PROTECT_REQ_EVT:
        {
          /*                                                      Octet
              Sevice Category = Content Protection (0x04)           0
              LOSC = (n-1 bytes)  = 4-1=3                           1
              CP_TYPE_LSB  (0x00) a magic number by me(titan)       2
              CP_TYPE_MSB  (0x00)                                   3
               CP_TYPE specific Values(0x00)                        4
          */
          //UINT8 p_data[] = {0x04,0x03,0x00,0x00,0x00};
          //UINT16 data_len = sizeof(p_data);
          UINT8 error_code = 0;

          BTIF_TRACE_DEBUG("do BTA_AvProtectRsp(handle = %d)",btif_av_cb.bta_handle);
          //BTA_AvProtectRsp(btif_av_cb.bta_handle, error_code, p_data,data_len);
          BTA_AvProtectRsp(btif_av_cb.bta_handle, error_code, NULL,0);

        } break;
#endif //(end -- tt pts test)
        case BTIF_AV_SINK_CONFIG_REQ_EVT:
        {
            btif_av_sink_config_req_t req;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            memcpy(&btif_av_cb.avk_config, p_data, sizeof(btif_av_cb.avk_config));
            btif_av_get_avk_codec_config(&btif_av_cb.avk_config, &req);
            bdaddr_copy(&req.peer_bd, &btif_av_cb.peer_bda);
#else
            // copy to avoid alignment problems
            memcpy(&req, p_data, sizeof(req));
#endif

            BTIF_TRACE_WARNING("BTIF_AV_SINK_CONFIG_REQ_EVT %d %d", req.sample_rate,
                    req.channel_count);
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
            /* this event only send to sink callback, so it can send without checking */
            if (bt_av_sink_callbacks != NULL) {
                HAL_CBACK(bt_av_sink_callbacks, audio_config_cb, &(btif_av_cb.peer_bda),
                    req.sample_rate, req.channel_count);
            }
#else
            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC && bt_av_sink_callbacks != NULL) {
                HAL_CBACK(bt_av_sink_callbacks, audio_config_cb, &(btif_av_cb.peer_bda),
                        req.sample_rate, req.channel_count);
            }
#endif
        } break;

        case BTIF_AV_CONNECT_REQ_EVT:
            // Check for device, if same device which moved to opening then ignore callback
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            if (memcmp (((btif_av_connect_req_t*)p_data)->target_bda, &(btif_av_cb.peer_bda),
                sizeof(btif_av_cb.peer_bda)) == 0)
#else
            if (memcmp ((bt_bdaddr_t*)p_data, &(btif_av_cb.peer_bda),
                sizeof(btif_av_cb.peer_bda)) == 0)
#endif
            {
                BTIF_TRACE_DEBUG("%s: Same device moved to Opening state,ignore Connect Req", __func__);
                btif_queue_advance();
                break;
            }
            else
            {
                BTIF_TRACE_DEBUG("%s: Moved from idle by Incoming Connection request", __func__);
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
                btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED,
                        ((btif_av_connect_req_t*)p_data)->target_bda);
#else
                btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, (bt_bdaddr_t*)p_data);
#endif
                btif_queue_advance();
                break;
            }

        case BTA_AV_PENDING_EVT:
            // Check for device, if same device which moved to opening then ignore callback
            if (memcmp (((tBTA_AV*)p_data)->pend.bd_addr, &(btif_av_cb.peer_bda),
                sizeof(btif_av_cb.peer_bda)) == 0)
            {
                BTIF_TRACE_DEBUG("%s: Same device moved to Opening state,ignore Pending Req", __func__);
                break;
            }
            else
            {
                BTIF_TRACE_DEBUG("%s: Moved from idle by outgoing Connection request", __func__);
                BTA_AvDisconnect(((tBTA_AV*)p_data)->pend.bd_addr);
                break;
            }

        case BTIF_AV_OFFLOAD_START_REQ_EVT:
            btif_a2dp_on_offload_started(BTA_AV_FAIL);
            BTIF_TRACE_ERROR("BTIF_AV_OFFLOAD_START_REQ_EVT: Stream not Started OPENING");
            break;

        case BTA_AV_CLOSE_EVT:
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_stopped(NULL, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_stopped(NULL);
#endif
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED,
                    &(btif_av_cb.peer_bda));
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
            break;

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        case BTIF_AV_DISCONNECT_REQ_EVT:
             BTIF_TRACE_DEBUG("%s: BTIF_AV_DISCONNECT_REQ_EVT", __func__);
             //BTA_AvDisconnect((*(BD_ADDR*)p_data);
             BTA_AvClose(btif_av_cb.bta_handle);
             if (btif_av_cb.peer_sep == AVDT_TSEP_SRC) {
                 BTIF_TRACE_DEBUG("%s: close AVRCP ", __func__);
                 BTA_AvCloseRc(btif_av_cb.bta_handle);
             }
             BTIF_TRACE_DEBUG("%s: report connection status ", __func__);
             /* inform the application that we are disconnecting */
             btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTING, &(btif_av_cb.peer_bda));
             btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_CLOSING);
             btif_queue_advance();
             break;
#endif

        CHECK_RC_EVENT(event, p_data);

        default:
            BTIF_TRACE_WARNING("%s : unhandled event:%s", __FUNCTION__,
                                dump_av_sm_event_name(event));
            return FALSE;

   }
   return TRUE;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*****************************************************************************
**
** Function        btif_av_state_closing_handler
**
** Description     Intermediate state managing events during closing
**                 of avdtp channel
**
** Returns         TRUE if event was processed, FALSE otherwise
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_closing_handler(btif_sm_event_t event, void *p_data, void *p_cb)
#else
static BOOLEAN btif_av_state_closing_handler(btif_sm_event_t event, void *p_data)
#endif
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)p_cb-btif_av_cbs]
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda,"");
#endif
    BTIF_TRACE_DEBUG("%s event:%s flags %x", __FUNCTION__,
                     dump_av_sm_event_name(event), btif_av_cb.flags);

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                /* immediately stop transmission of frames */
                btif_a2dp_set_tx_flush(TRUE);
                /* wait for audioflinger to stop a2dp */
            }
            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                btif_a2dp_set_rx_flush(TRUE, &btif_av_cb.peer_bda);
#else
                btif_a2dp_set_rx_flush(TRUE);
#endif
            }
            break;

        case BTA_AV_STOP_EVT:
        case BTIF_AV_STOP_STREAM_REQ_EVT:
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                /* immediately flush any pending tx frames while suspend is pending */
                btif_a2dp_set_tx_flush(TRUE);
            }

            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                btif_a2dp_set_rx_flush(TRUE, &btif_av_cb.peer_bda);
#else
                btif_a2dp_set_rx_flush(TRUE);
#endif
            }
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_stopped(NULL, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_stopped(NULL);
#endif
            break;

        case BTIF_SM_EXIT_EVT:
            break;

        case BTA_AV_CLOSE_EVT:
            /* inform the application that we are disconnecting */
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
            break;

        /* Handle the RC_CLOSE event for the cleanup */
        case BTA_AV_RC_CLOSE_EVT:
            btif_rc_handler(event, (tBTA_AV*)p_data);
            break;

#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
        /* Handle the RC_BROWSE_CLOSE event for tetsing*/
        case BTA_AV_RC_BROWSE_CLOSE_EVT:
            btif_rc_handler(event, (tBTA_AV*)p_data);
        break;
#endif

        case BTIF_AV_OFFLOAD_START_REQ_EVT:
            btif_a2dp_on_offload_started(BTA_AV_FAIL);
            BTIF_TRACE_ERROR("BTIF_AV_OFFLOAD_START_REQ_EVT: Stream not Started Closing");
            break;
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        case BTA_AV_OPEN_EVT:
            {
                tBTA_AV *p_bta_data = (tBTA_AV*)p_data;
                if (p_bta_data->open.status != BTA_AV_SUCCESS)
                {
                    BTIF_TRACE_ERROR("BTA_AV_OPEN_EVT fail");
                    btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
                    btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
                }
            }
            break;
#endif
        default:
            BTIF_TRACE_WARNING("%s : unhandled event:%s", __FUNCTION__,
                                dump_av_sm_event_name(event));
            return FALSE;
   }
   return TRUE;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*****************************************************************************
**
** Function     btif_av_state_opened_handler
**
** Description  Handles AV events while AVDTP is in OPEN state
**
** Returns      TRUE if event was processed, FALSE otherwise
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *p_data, void *p_cb)
#else
static BOOLEAN btif_av_state_opened_handler(btif_sm_event_t event, void *p_data)
#endif
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)p_cb-btif_av_cbs]
#endif
    tBTA_AV *p_av = (tBTA_AV*)p_data;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda,"");
#endif

    BTIF_TRACE_DEBUG("%s event:%s flags %x", __FUNCTION__,
                     dump_av_sm_event_name(event), btif_av_cb.flags);

    if ( (event == BTA_AV_REMOTE_CMD_EVT) && (btif_av_cb.flags & BTIF_AV_FLAG_REMOTE_SUSPEND) &&
         (p_av->remote_cmd.rc_id == BTA_AV_RC_PLAY) )
    {
        BTIF_TRACE_EVENT("%s: Resetting remote suspend flag on RC PLAY", __FUNCTION__);
        btif_av_cb.flags &= ~BTIF_AV_FLAG_REMOTE_SUSPEND;
    }
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    //Occur the AVDTP start event when music is not played.
    else if (((event == BTA_AV_SUSPEND_EVT) || (event == BTA_AV_STOP_EVT)) && (btif_av_cb.flags & BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING))
    {
        BTIF_TRACE_EVENT("%s: STOP_FOR_START_EVT_FROM_REMOTE ", __FUNCTION__);
        /* a2dp suspended, stop media task until resumed */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        btif_a2dp_on_suspended(&p_av->suspend, &(btif_av_cb.peer_bda));
#else
        btif_a2dp_on_suspended(&p_av->suspend);
#endif
        btif_report_audio_state(BTAV_AUDIO_STATE_STOPPED, &(btif_av_cb.peer_bda));
        /* suspend completed and state changed, clear pending status */
        btif_av_cb.flags &= ~BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING;
        btif_av_cb.flags &= ~BTIF_AV_FLAG_REMOTE_SUSPENDING;
    }
#endif
    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
            btif_av_cb.flags &= ~BTIF_AV_FLAG_PENDING_STOP;
            btif_av_cb.flags &= ~BTIF_AV_FLAG_PENDING_START;
            break;

        case BTIF_SM_EXIT_EVT:
            btif_av_cb.flags &= ~BTIF_AV_FLAG_PENDING_START;
            break;

        case BTIF_AV_START_STREAM_REQ_EVT:
            if (btif_av_cb.peer_sep != AVDT_TSEP_SRC)
                btif_a2dp_setup_codec();
            BTA_AvStart();
            btif_av_cb.flags |= BTIF_AV_FLAG_PENDING_START;
            break;

        case BTA_AV_START_EVT:
        {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            BTIF_TRACE_EVENT("BTA_AV_START_EVT status %d, suspending %d, init %d, channel:%d",
                p_av->start.status, p_av->start.suspending, p_av->start.initiator, p_av->start.hndl);
#else
            BTIF_TRACE_EVENT("BTA_AV_START_EVT status %d, suspending %d, init %d",
                p_av->start.status, p_av->start.suspending, p_av->start.initiator);
#endif

            if ((p_av->start.status == BTA_SUCCESS) && (p_av->start.suspending == TRUE))
                return TRUE;

            /* if remote tries to start a2dp when DUT is a2dp source
             * then suspend. In case a2dp is sink and call is active
             * then disconnect the AVDTP channel
             */
            if (!(btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START))
            {
                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                {
                    BTIF_TRACE_EVENT("%s: trigger suspend as remote initiated!!", __FUNCTION__);
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                    btif_dispatch_sm_event_by_bda(BTIF_AV_SUSPEND_STREAM_REQ_EVT,
                        NULL, 0, &btif_av_cb.peer_bda, btif_av_cb.bta_handle);
#else
                    btif_dispatch_sm_event(BTIF_AV_SUSPEND_STREAM_REQ_EVT, NULL, 0);
#endif
                }
            }

            /*  In case peer is A2DP SRC we do not want to ack commands on UIPC*/
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                if (btif_a2dp_on_started(&p_av->start,
                    ((btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START) != 0)))
                {
                    /* only clear pending flag after acknowledgement */
                    btif_av_cb.flags &= ~BTIF_AV_FLAG_PENDING_START;
                }
            }

            /* remain in open state if status failed */
            if (p_av->start.status != BTA_AV_SUCCESS)
                return FALSE;

            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                btif_a2dp_set_rx_flush(FALSE, &btif_av_cb.peer_bda);
#else
                btif_a2dp_set_rx_flush(FALSE); /*  remove flush state, ready for streaming*/
#endif
            }

            /* change state to started, send acknowledgement if start is pending */
            if (btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START) {
                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                    btif_a2dp_on_started(NULL, TRUE);
                /* pending start flag will be cleared when exit current state */
            }
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_STARTED);
        } break;

        case BTIF_AV_DISCONNECT_REQ_EVT:
            BTA_AvClose(btif_av_cb.bta_handle);
            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC) {
                BTA_AvCloseRc(btif_av_cb.bta_handle);
            }

            /* inform the application that we are disconnecting */
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTING, &(btif_av_cb.peer_bda));
            break;

        case BTA_AV_CLOSE_EVT:
             /* avdtp link is closed */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_stopped(NULL, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_stopped(NULL);
#endif
            /* inform the application that we are disconnected */
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
#if defined(MTK_A2DP_SRC_STE_CODEC) && (MTK_A2DP_SRC_STE_CODEC == TRUE) \
 || defined(MTK_A2DP_SNK_STE_CODEC) && (MTK_A2DP_SNK_STE_CODEC == TRUE)
            btif_av_enable_btclk(&btif_av_cb.peer_bda, FALSE, 0);
#endif /* MTK_A2DP_SRC_STE_CODEC&MTK_A2DP_SNK_STE_CODEC */

            /* change state to idle, send acknowledgement if start is pending */
            if (btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START) {
                btif_a2dp_ack_fail();
                /* pending start flag will be cleared when exit current state */
            }
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
#if (defined(MTK_A2DP_SRC_DUMP_PCM_DATA) && (MTK_A2DP_SRC_DUMP_PCM_DATA == TRUE))
            if (src_outputPcmSampleFile)
            {
                fclose(src_outputPcmSampleFile);
                BTIF_TRACE_DEBUG("%s() close file %s success.",__FUNCTION__, src_outputFilename);
            }
            else
            {
                BTIF_TRACE_DEBUG("%s() file %s may already closed.",__FUNCTION__, src_outputFilename);
            }
            src_outputPcmSampleFile = NULL;
#endif
            break;

        case BTA_AV_RECONFIG_EVT:
            if((btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START) &&
                (p_av->reconfig.status == BTA_AV_SUCCESS))
            {
               APPL_TRACE_WARNING("reconfig done BTA_AVstart()");
               BTA_AvStart();
            }
            else if(btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START)
            {
               btif_av_cb.flags &= ~BTIF_AV_FLAG_PENDING_START;
               btif_a2dp_ack_fail();
            }
            break;

        case BTIF_AV_CONNECT_REQ_EVT:
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            if (memcmp (((btif_av_connect_req_t*)p_data)->target_bda, &(btif_av_cb.peer_bda),
                sizeof(btif_av_cb.peer_bda)) == 0)
#else
            if (memcmp ((bt_bdaddr_t*)p_data, &(btif_av_cb.peer_bda),
                sizeof(btif_av_cb.peer_bda)) == 0)
#endif
            {
                BTIF_TRACE_DEBUG("%s: Ignore BTIF_AV_CONNECT_REQ_EVT for same device", __func__);
            }
            else
            {
                BTIF_TRACE_DEBUG("%s: Moved to opened by Other Incoming Conn req", __func__);
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
                btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED,
                        ((btif_av_connect_req_t*)p_data)->target_bda);
#else
                btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED,
                        (bt_bdaddr_t*)p_data);
#endif
            }
            btif_queue_advance();
            break;

        case BTIF_AV_OFFLOAD_START_REQ_EVT:
            btif_a2dp_on_offload_started(BTA_AV_FAIL);
            BTIF_TRACE_ERROR("BTIF_AV_OFFLOAD_START_REQ_EVT: Stream not Started Opened");
            break;
#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)
        case BTIF_AV_CODEC_ENABLE_REQ_EVT:
        {
            btif_av_codec_enable_req_t *data = (btif_av_codec_enable_req_t*)p_data;
            BTA_AvCodecEnable(data->codec_type , data->enable);
            break;
        }
#endif
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        case BTIF_AV_ACTIVE_REQ_EVT:
        {
            int active_idx = btif_av_get_active_idx();
            if (active_idx != -1)
            {
                btif_av_cbs[active_idx].active = FALSE;
                btif_a2dp_on_stopped(NULL, &btif_av_cbs[active_idx].peer_bda);
            }
            btif_av_cb.active = TRUE;
            btif_reset_decoder((UINT8*)(btif_av_cb.avk_config.codec_info), &btif_av_cb.peer_bda);
            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
                BTA_AvActiveSrc(&btif_av_cb.peer_bda);
            }
            else if(btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                BTA_AvActiveSink(&btif_av_cb.peer_bda);
            }
            else
            {
                BTIF_TRACE_DEBUG("%s: invalid peer_sep", __func__);
                break ;
            }

            BTIF_TRACE_DEBUG("%s: BTIF_AV_ACTIVE_REQ_EVT", __func__);
            break;
        }
#endif
        CHECK_RC_EVENT(event, p_data);

        default:
            BTIF_TRACE_WARNING("%s : unhandled event:%s", __FUNCTION__,
                               dump_av_sm_event_name(event));
            return FALSE;

    }
    return TRUE;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*****************************************************************************
**
** Function     btif_av_state_started_handler
**
** Description  Handles AV events while A2DP stream is started
**
** Returns      TRUE if event was processed, FALSE otherwise
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *p_data, void *p_cb)
#else
static BOOLEAN btif_av_state_started_handler(btif_sm_event_t event, void *p_data)
#endif
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#define btif_av_cb btif_av_cbs[(btif_av_cb_t*)p_cb-btif_av_cbs]
#endif

    tBTA_AV *p_av = (tBTA_AV*)p_data;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    BTIF_AV_PRINT_BDA(&btif_av_cb.peer_bda,"");
#endif

    BTIF_TRACE_DEBUG("%s event:%s flags %x", __FUNCTION__,
                     dump_av_sm_event_name(event), btif_av_cb.flags);

    switch (event)
    {
        case BTIF_SM_ENTER_EVT:
            /* we are again in started state, clear any remote suspend flags */
            btif_av_cb.flags &= ~BTIF_AV_FLAG_REMOTE_SUSPEND;

            /**
             * Report to components above that we have entered the streaming
             * stage, this should usually be followed by focus grant.
             * see update_audio_focus_state()
             */
            btif_report_audio_state(BTAV_AUDIO_STATE_STARTED, &(btif_av_cb.peer_bda));

            /* increase the a2dp consumer task priority temporarily when start
            ** audio playing, to avoid overflow the audio packet queue. */
            adjust_priority_a2dp(TRUE);
            break;

        case BTIF_SM_EXIT_EVT:
            /* restore the a2dp consumer task priority when stop audio playing. */
            adjust_priority_a2dp(FALSE);

            break;

        case BTIF_AV_START_STREAM_REQ_EVT:
            /* we were remotely started, just ack back the local request */
            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                btif_a2dp_on_started(NULL, TRUE);
            break;

        /* fixme -- use suspend = true always to work around issue with BTA AV */
        case BTIF_AV_STOP_STREAM_REQ_EVT:
        case BTIF_AV_SUSPEND_STREAM_REQ_EVT:
#if defined(MTK_COMMON) && (MTK_COMMON == TRUE) && defined(MTK_LINUX)
            BTIF_TRACE_DEBUG("%s flags:%d", __FUNCTION__, btif_av_cb.flags);
            if (btif_av_cb.flags & BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING)
            {
                BTIF_TRACE_WARNING("%s event:%s already process!!!", __FUNCTION__,
                                 dump_av_sm_event_name(event), btif_av_cb.flags);
                break;
            }
#endif
            /* set pending flag to ensure btif task is not trying to restart
               stream while suspend is in progress */
            btif_av_cb.flags |= BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING;

            /* if we were remotely suspended but suspend locally, local suspend
               always overrides */
            btif_av_cb.flags &= ~BTIF_AV_FLAG_REMOTE_SUSPEND;

            if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
            /* immediately stop transmission of frames while suspend is pending */
                btif_a2dp_set_tx_flush(TRUE);
            }

            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC) {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
                btif_a2dp_on_stopped(NULL, &(btif_av_cb.peer_bda));
#else
                btif_a2dp_set_rx_flush(TRUE);
                btif_a2dp_on_stopped(NULL);
#endif
            }
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            BTA_AvStop(btif_av_cb.bta_handle, TRUE);
#else
            BTA_AvStop(TRUE);
#endif
            break;

        case BTIF_AV_DISCONNECT_REQ_EVT:
            /* request avdtp to close */
            BTA_AvClose(btif_av_cb.bta_handle);
            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC) {
                BTA_AvCloseRc(btif_av_cb.bta_handle);
            }

            /* inform the application that we are disconnecting */
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTING, &(btif_av_cb.peer_bda));

            /* wait in closing state until fully closed */
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_CLOSING);
            break;

        case BTA_AV_SUSPEND_EVT:

            BTIF_TRACE_EVENT("BTA_AV_SUSPEND_EVT status %d, init %d",
                 p_av->suspend.status, p_av->suspend.initiator);

            /* a2dp suspended, stop media task until resumed */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_suspended(&p_av->suspend, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_suspended(&p_av->suspend);
#endif

            /* if not successful, remain in current state */
            if (p_av->suspend.status != BTA_AV_SUCCESS)
            {
                btif_av_cb.flags &= ~BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING;

                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                {
                /* suspend failed, reset back tx flush state */
                    btif_a2dp_set_tx_flush(FALSE);
                }
                return FALSE;
            }

            if (p_av->suspend.initiator != TRUE)
            {
                /* remote suspend, notify HAL and await audioflinger to
                   suspend/stop stream */

                /* set remote suspend flag to block media task from restarting
                   stream only if we did not already initiate a local suspend */
                if ((btif_av_cb.flags & BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING) == 0)
                    btif_av_cb.flags |= BTIF_AV_FLAG_REMOTE_SUSPEND;

                btif_report_audio_state(BTAV_AUDIO_STATE_REMOTE_SUSPEND, &(btif_av_cb.peer_bda));
            }
            else
            {
                btif_report_audio_state(BTAV_AUDIO_STATE_STOPPED, &(btif_av_cb.peer_bda));
            }

            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_OPENED);

            /* suspend completed and state changed, clear pending status */
            btif_av_cb.flags &= ~BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING;
            break;

        case BTA_AV_STOP_EVT:
            btif_av_cb.flags |= BTIF_AV_FLAG_PENDING_STOP;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_stopped(&p_av->suspend, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_stopped(&p_av->suspend);
#endif

            btif_report_audio_state(BTAV_AUDIO_STATE_STOPPED, &(btif_av_cb.peer_bda));

            /* if stop was successful, change state to open */
            if (p_av->suspend.status == BTA_AV_SUCCESS)
                btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_OPENED);
            break;

        case BTA_AV_CLOSE_EVT:
            btif_av_cb.flags |= BTIF_AV_FLAG_PENDING_STOP;

            /* avdtp link is closed */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
            btif_a2dp_on_stopped(NULL, &(btif_av_cb.peer_bda));
#else
            btif_a2dp_on_stopped(NULL);
#endif

            /* inform the application that we are disconnected */
            btif_report_connection_state(BTAV_CONNECTION_STATE_DISCONNECTED, &(btif_av_cb.peer_bda));
            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_IDLE);
#if (defined(MTK_A2DP_SRC_DUMP_PCM_DATA) && (MTK_A2DP_SRC_DUMP_PCM_DATA == TRUE))
            if (src_outputPcmSampleFile)
            {
                fclose(src_outputPcmSampleFile);
                BTIF_TRACE_DEBUG("%s() close file %s success.",__FUNCTION__, src_outputFilename);
            }
            else
            {
                BTIF_TRACE_DEBUG("%s() file %s may already closed.",__FUNCTION__, src_outputFilename);
            }
            src_outputPcmSampleFile = NULL;
#endif
            break;

        case BTIF_AV_OFFLOAD_START_REQ_EVT:
            BTA_AvOffloadStart(btif_av_cb.bta_handle);
            break;

        case BTA_AV_OFFLOAD_START_RSP_EVT:

            btif_a2dp_on_offload_started(p_av->status);
            break;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        case BTA_AV_START_EVT:
        {
            BTIF_TRACE_WARNING("BTA_AV_START_EVT status %d, suspending %d, init %d, channel:%d",
                p_av->start.status, p_av->start.suspending, p_av->start.initiator, p_av->start.hndl);

            if ((p_av->start.status == BTA_SUCCESS) && (p_av->start.suspending == TRUE))
                return TRUE;

            /* remain in start state if status failed */
            if (p_av->start.status != BTA_AV_SUCCESS)
                return FALSE;

            /* if remote tries to start a2dp when DUT is a2dp source
                     * then suspend. In case a2dp is sink and call is active
                     * then disconnect the AVDTP channel
                     */
            if (!(btif_av_cb.flags & BTIF_AV_FLAG_PENDING_START))
            {
                if (btif_av_cb.peer_sep == AVDT_TSEP_SNK)
                {
                    BTIF_TRACE_WARNING("%s: trigger suspend as remote initiated!!", __FUNCTION__);
                    btif_dispatch_sm_event_by_bda(BTIF_AV_SUSPEND_STREAM_REQ_EVT,
                        NULL, 0, &btif_av_cb.peer_bda, btif_av_cb.bta_handle);
                }
            }

            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
                btif_a2dp_set_rx_flush(FALSE, &btif_av_cb.peer_bda);
            }

            btif_sm_change_state(btif_av_cb.sm_handle, BTIF_AV_STATE_STARTED);
            break;
        }
        case BTIF_AV_ACTIVE_REQ_EVT:
        {
            int active_idx = btif_av_get_active_idx();
            if (active_idx != -1)
            {
                btif_av_cbs[active_idx].active = FALSE;
                btif_a2dp_on_stopped(NULL, &btif_av_cbs[active_idx].peer_bda);
// Add TCI cmd to reduce acl priority
#ifdef MTK_A2DP_SILENT_STREAM_PRIORITY
                BTIF_TRACE_WARNING("%s:handle 0x%x Set ACL Priority to LOW for A2DP",
                                    __func__, btif_av_cbs[active_idx].bta_handle);
                BTA_AvSetAclPriority(btif_av_cbs[active_idx].bta_handle, BTA_AV_ACL_PRIORITY_NORMAL);
#endif
            }
            btif_av_cb.active = TRUE;
            btif_reset_decoder((UINT8*)(btif_av_cb.avk_config.codec_info), &btif_av_cb.peer_bda);

            if (btif_av_cb.peer_sep == AVDT_TSEP_SRC)
            {
// Add TCI cmd to increase acl priority
#ifdef MTK_A2DP_SILENT_STREAM_PRIORITY
                BTIF_TRACE_WARNING("%s:handle 0x%x Set ACL Priority to HIGH for A2DP",
                                    __func__, btif_av_cb.bta_handle);
                BTA_AvSetAclPriority(btif_av_cb.bta_handle, BTA_AV_ACL_PRIORITY_HIGH);
#endif
                BTIF_TRACE_ERROR("%s:active src.", __func__);

                BTA_AvActiveSrc(&btif_av_cb.peer_bda);
            }
            else if(btif_av_cb.peer_sep == AVDT_TSEP_SNK)
            {
                BTA_AvActiveSink(&btif_av_cb.peer_bda);
            }
            else
            {
                BTIF_TRACE_DEBUG("%s: invalid peer_sep", __func__);
                break ;
            }

            BTIF_TRACE_DEBUG("%s: BTIF_AV_ACTIVE_REQ_EVT", __func__);
            break;
        }
#endif
        CHECK_RC_EVENT(event, p_data);

        default:
            BTIF_TRACE_WARNING("%s : unhandled event:%s", __FUNCTION__,
                                 dump_av_sm_event_name(event));
            return FALSE;

    }
    return TRUE;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

/*****************************************************************************
**  Local event handlers
******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static void btif_av_handle_event_by_bda(UINT16 event,
    char* p_param, const bt_bdaddr_t *addr, UINT32 handle)
{
    BTIF_AV_PRINT_BDA(addr, "event:%s handle=0x%x",
                     dump_av_sm_event_name((btif_av_sm_event_t)event), handle);

    switch(event)
    {
        case BTIF_AV_CLEANUP_REQ_EVT:
            BTIF_TRACE_EVENT("%s: BTIF_AV_CLEANUP_REQ_EVT", __FUNCTION__);
#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            for (int idx=0;idx<BTA_AV_NUM_CONNS;idx++)
            {
                btif_sm_shutdown(btif_av_cbs[idx].sm_handle);
                btif_av_cbs[idx].sm_handle = NULL;
            }
#endif

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#else
            btif_a2dp_stop_media_task();
#endif

#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            if (bt_av_src_callbacks != NULL && bt_av_sink_callbacks == NULL)
            {
                bt_av_src_callbacks = NULL;
                BTIF_TRACE_DEBUG("cleanup bt_av_src_callbacks");
            }
#endif
            break;
        case BTA_AV_REGISTER_EVT:
            {
                int idx = btif_av_assign_bta_handle(((tBTA_AV*)p_param)->registr.hndl);
                if (idx != -1)
                {
                    BTIF_TRACE_DEBUG("[%d] register handle 0x%x",
                        idx, btif_av_cbs[idx].bta_handle);
                    btif_sm_dispatch(btif_av_cbs[idx].sm_handle, event, (void*)p_param);
                }
                else
                    BTIF_TRACE_DEBUG("register handle fail");

                btif_av_event_free_data(event, p_param);
            }
            break;
        default:
            {
                int idx = -1;
                if (!bdaddr_is_empty(addr))
                {
                    idx = btif_av_find_or_alloc_dev_idx(addr);
                }
                else
                {
                    idx = btif_av_get_dev_idx_by_handle(handle);
                }

                if (-1 != idx)
                {
                    btif_sm_dispatch(btif_av_cbs[idx].sm_handle, event, (void*)p_param);
                }
                else
                {
                    BTIF_TRACE_ERROR("%s event:%s no sm handle", __func__,
                             dump_av_sm_event_name((btif_av_sm_event_t)event));
                }
                btif_av_event_free_data(event, p_param);
            }
    }
}
#else
static void btif_av_handle_event(UINT16 event, char* p_param)
{
    BTIF_TRACE_EVENT("%s event:%s", __func__,
                     dump_av_sm_event_name((btif_av_sm_event_t)event));
    switch(event)
    {
        case BTIF_AV_CLEANUP_REQ_EVT:
            BTIF_TRACE_EVENT("%s: BTIF_AV_CLEANUP_REQ_EVT", __FUNCTION__);
#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            /* Also shut down the AV state machine */
            btif_sm_shutdown(btif_av_cb.sm_handle);
            btif_av_cb.sm_handle = NULL;
#endif

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#else
            btif_a2dp_stop_media_task();
#endif

#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
            if (bt_av_src_callbacks != NULL && bt_av_sink_callbacks == NULL)
            {
                bt_av_src_callbacks = NULL;
                BTIF_TRACE_DEBUG("cleanup bt_av_src_callbacks");
            }
#endif
            break;
        case BTA_AV_MEDIA_SINK_CFG_EVT:
            BTIF_AV_LOCK();
            btif_av_handle_media_sink_cfg(p_param);
            BTIF_AV_UNLOCK();
            break;
        case BTA_AV_REGISTER_EVT:
            if (btif_av_cb.sm_handle == NULL)
            {
                btif_av_cb.bta_handle = ((tBTA_AV*)p_param)->registr.hndl;
                BTIF_TRACE_DEBUG("%s: BTA AV Handle updated", __func__);
            }
            /* FALLTHROUGH */
        default:
            BTIF_AV_LOCK();
            btif_sm_dispatch(btif_av_cb.sm_handle, event, (void*)p_param);
            BTIF_AV_UNLOCK();
            btif_av_event_free_data(event, p_param);
    }
}
#endif

void btif_av_event_deep_copy(UINT16 event, char *p_dest, char *p_src)
{
    tBTA_AV *av_src = (tBTA_AV *)p_src;
    tBTA_AV *av_dest = (tBTA_AV *)p_dest;

    // First copy the structure
    maybe_non_aligned_memcpy(av_dest, av_src, sizeof(*av_src));

    switch (event)
    {
        case BTA_AV_META_MSG_EVT:
            if (av_src->meta_msg.p_data && av_src->meta_msg.len)
            {
                av_dest->meta_msg.p_data = osi_calloc(av_src->meta_msg.len);
                memcpy(av_dest->meta_msg.p_data, av_src->meta_msg.p_data,
                       av_src->meta_msg.len);
            }

            if (av_src->meta_msg.p_msg)
            {
                av_dest->meta_msg.p_msg = osi_calloc(sizeof(tAVRC_MSG));
                memcpy(av_dest->meta_msg.p_msg, av_src->meta_msg.p_msg,
                       sizeof(tAVRC_MSG));
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
                tAVRC_MSG* p_msg_src = av_src->meta_msg.p_msg;
                tAVRC_MSG* p_msg_dest = av_dest->meta_msg.p_msg;

                if ((p_msg_src->hdr.opcode == AVRC_OP_VENDOR) &&
                        (p_msg_src->vendor.p_vendor_data && p_msg_src->vendor.vendor_len))
                {
                    p_msg_dest->vendor.p_vendor_data =
                        (UINT8*)osi_calloc(p_msg_src->vendor.vendor_len);
                    memcpy(p_msg_dest->vendor.p_vendor_data,
                        p_msg_src->vendor.p_vendor_data, p_msg_src->vendor.vendor_len);
                }
#else
                if (av_src->meta_msg.p_msg->vendor.p_vendor_data &&
                    av_src->meta_msg.p_msg->vendor.vendor_len)
                {
                    av_dest->meta_msg.p_msg->vendor.p_vendor_data = osi_calloc(
                        av_src->meta_msg.p_msg->vendor.vendor_len);
                    memcpy(av_dest->meta_msg.p_msg->vendor.p_vendor_data,
                        av_src->meta_msg.p_msg->vendor.p_vendor_data,
                        av_src->meta_msg.p_msg->vendor.vendor_len);
                }
#endif
            }
            break;

        default:
            break;
    }
}

static void btif_av_event_free_data(btif_sm_event_t event, void *p_data)
{
    switch (event)
    {
        case BTA_AV_META_MSG_EVT:
            {
                tBTA_AV *av = (tBTA_AV *)p_data;
                osi_free_and_reset((void **)&av->meta_msg.p_data);

                if (av->meta_msg.p_msg) {
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
                    if (av->meta_msg.p_msg->hdr.opcode == AVRC_OP_VENDOR) {
                        osi_free(av->meta_msg.p_msg->vendor.p_vendor_data);
                    }
#else
                    osi_free(av->meta_msg.p_msg->vendor.p_vendor_data);
#endif
                    osi_free_and_reset((void **)&av->meta_msg.p_msg);
                }
            }
            break;

        default:
            break;
    }
}

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static void btif_av_handle_bta_av_event(tBTA_AV_EVT event, tBTA_AV *p_data)
{
  bt_bdaddr_t peer_address;
  tBTA_AV_HNDL bta_handle = 0;
  memset(&peer_address, 0, sizeof(peer_address));
  BTIF_TRACE_EVENT("%s event:%d", __func__, event);

  switch (event) {
    case BTA_AV_ENABLE_EVT: {
      return;  // Nothing to do
    }
    case BTA_AV_REGISTER_EVT: {
      break;  // Nothing else to do
    }
    case BTA_AV_OPEN_EVT: {
      bdaddr_copy(&peer_address, p_data->open.bd_addr);
      bta_handle = p_data->open.hndl;
      break;
    }
    case BTA_AV_CLOSE_EVT: {
      bta_handle = p_data->close.hndl;
      break;
    }
    case BTA_AV_START_EVT: {
      bta_handle = p_data->start.hndl;
      break;
    }
    case BTA_AV_SUSPEND_EVT:
    case BTA_AV_STOP_EVT: {
      bta_handle = p_data->suspend.hndl;
      break;
    }
    case BTA_AV_PROTECT_REQ_EVT: {
      bta_handle = p_data->protect_req.hndl;
      break;
    }
    case BTA_AV_PROTECT_RSP_EVT: {
      bta_handle = p_data->protect_rsp.hndl;
      break;
    }
    case BTA_AV_RC_OPEN_EVT: {
      memcpy(peer_address.address, p_data->rc_open.peer_addr, sizeof(peer_address.address));
      break;
    }
    case BTA_AV_RC_CLOSE_EVT: {
      memcpy(peer_address.address, p_data->rc_close.peer_addr, sizeof(peer_address.address));
      break;
    }
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
    case BTA_AV_RC_BROWSE_OPEN_EVT: {
      peer_address = p_data->rc_browse_open.peer_addr;
      break;
    }
    case BTA_AV_RC_BROWSE_CLOSE_EVT: {
      peer_address = p_data->rc_browse_close.peer_addr;
      break;
    }
#endif
    case BTA_AV_REMOTE_CMD_EVT:
    case BTA_AV_REMOTE_RSP_EVT:
    case BTA_AV_VENDOR_CMD_EVT:
    case BTA_AV_VENDOR_RSP_EVT:
    case BTA_AV_META_MSG_EVT:
    case BTA_AV_OFFLOAD_START_RSP_EVT: {
      // events are received from the AVRCP module.
      int idx = btif_av_get_active_idx();
      if (idx != -1)
      {
        bdaddr_copy(&peer_address, &btif_av_cbs[idx].peer_bda);
      }
      else
      {
            BTIF_TRACE_EVENT("%s rc_handle:%d", __func__, p_data->remote_cmd.rc_handle);
            btif_rc_get_device_addr_by_handle(p_data->remote_cmd.rc_handle, &peer_address);
      }
      break;
    }
    case BTA_AV_RECONFIG_EVT: {
      bta_handle = p_data->reconfig.hndl;
      break;
    }
    case BTA_AV_PENDING_EVT: {
      memcpy(peer_address.address, p_data->pend.bd_addr, sizeof(peer_address.address));
      break;
    }
    case BTA_AV_REJECT_EVT: {
      memcpy(peer_address.address, p_data->reject.bd_addr, sizeof(peer_address.address));
      bta_handle = p_data->reject.hndl;
      break;
    }
    case BTA_AV_RC_FEAT_EVT: {
      memcpy(peer_address.address, p_data->rc_feat.peer_addr, sizeof(peer_address.address));
      break;
    }
  }

  {
      char buf[18];

      BTIF_TRACE_DEBUG("%s: peer_address=%s handle=0x%x", __func__,
           bdaddr_to_string(&peer_address, buf, sizeof(buf)), bta_handle);
  }

    btif_transfer_context_by_bda(btif_av_handle_event_by_bda, event,
                          (char*)p_data, sizeof(tBTA_AV),
                          btif_av_event_deep_copy, &peer_address, bta_handle);
}
#endif

static void bte_av_callback(tBTA_AV_EVT event, tBTA_AV *p_data)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_handle_bta_av_event(event, p_data);
#else
    btif_transfer_context(btif_av_handle_event, event,
                          (char*)p_data, sizeof(tBTA_AV), btif_av_event_deep_copy);
#endif
}

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static void btif_av_get_avk_codec_config(btif_av_avk_config *avk_config,
    btif_av_sink_config_req_t *config_req)
{
    tA2D_STATUS a2d_status;
    tA2D_SBC_CIE sbc_cie;

    if (avk_config->codec_type == BTA_AV_CODEC_SBC)
    {
        /* send a command to BT Media Task */
        a2d_status = A2D_ParsSbcInfo(&sbc_cie, (UINT8 *)(avk_config->codec_info), FALSE);
        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req->sample_rate = btif_a2dp_get_track_frequency(sbc_cie.samp_freq);
            config_req->channel_count = btif_a2dp_get_track_channel_count(sbc_cie.ch_mode);
        } else {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsSbcInfo fail:%d", a2d_status);
        }
    }
#if defined(MTK_A2DP_SNK_AAC_CODEC) && (MTK_A2DP_SNK_AAC_CODEC == TRUE)
    if (avk_config->codec_type == BTA_AV_CODEC_M24)
    {
        tA2D_AAC_CIE aac_cie;
        /* send a command to BT Media Task */
        a2d_status = A2D_ParsAACInfo(&aac_cie, (UINT8 *)(avk_config->codec_info), FALSE);
        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req->sample_rate = btif_a2dp_get_aac_track_frequency(aac_cie.samp_freq);
            config_req->channel_count = btif_a2dp_get_aac_track_channel_count(aac_cie.channels);
        }
        else
        {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsAACInfo fail:%d", a2d_status);
        }
    }
#endif
    if (avk_config->codec_type == BTA_AV_CODEC_NON)
    {
        tA2D_LHDC_CIE lhdc;
        /* send a command to BT Media Task */
        a2d_status = A2D_ParsLHDCInfo(&lhdc, (UINT8 *)(avk_config->codec_info), FALSE);
        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req->sample_rate = btif_a2dp_get_lhdc_track_frequency(lhdc.samp_freq);
            config_req->channel_count = btif_a2dp_get_lhdc_track_channel_count(lhdc.channels);
        }
        else
        {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsLHDCInfo fail:%d", a2d_status);
        }
    }
}
#endif


#if !defined(MTK_A2DP_DUAL_AUDIO) || (MTK_A2DP_DUAL_AUDIO == FALSE)
static void btif_av_handle_media_sink_cfg(tBTA_AV_MEDIA *p_data)
{
    tA2D_STATUS a2d_status;
    tA2D_SBC_CIE sbc_cie;
    btif_av_sink_config_req_t config_req;

    /* send a command to BT Media Task */
    if (p_data->avk_config.codec_type == BTA_AV_CODEC_SBC ||
        p_data->avk_config.codec_type == BTA_AV_CODEC_STE)
    {
        /* send a command to BT Media Task */
        btif_reset_decoder((UINT8*)(p_data->avk_config.codec_info));
        if (p_data->avk_config.codec_type == BTA_AV_CODEC_STE)
        {
            a2d_status = A2D_ParsSteInfo(&sbc_cie, (UINT8 *)(p_data->avk_config.codec_info), FALSE);
        }
        else
        {
            a2d_status = A2D_ParsSbcInfo(&sbc_cie, (UINT8 *)(p_data->avk_config.codec_info), FALSE);
        }

        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req.sample_rate = btif_a2dp_get_track_frequency(sbc_cie.samp_freq);
            config_req.channel_count = btif_a2dp_get_track_channel_count(sbc_cie.ch_mode);
            memcpy(&config_req.peer_bd,(UINT8*)(p_data->avk_config.bd_addr),
                                                              sizeof(config_req.peer_bd));
            btif_transfer_context(btif_av_handle_event, BTIF_AV_SINK_CONFIG_REQ_EVT,
                                     (char*)&config_req, sizeof(config_req), NULL);
        } else {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsSbcInfo fail:%d", a2d_status);
        }
    }

#if defined(MTK_A2DP_SNK_AAC_CODEC) && (MTK_A2DP_SNK_AAC_CODEC == TRUE)
    if (p_data->avk_config.codec_type == BTA_AV_CODEC_M24)
    {
        tA2D_AAC_CIE aac_cie;
        BTIF_TRACE_DEBUG("%s, BTA_AV_MEDIA_SINK_CFG_EVT", __FUNCTION__);
        /* send a command to BT Media Task */
        btif_reset_decoder((UINT8*)(p_data->avk_config.codec_info));
        a2d_status = A2D_ParsAACInfo(&aac_cie, (UINT8 *)(p_data->avk_config.codec_info), FALSE);
        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req.sample_rate = btif_a2dp_get_aac_track_frequency(aac_cie.samp_freq);
            config_req.channel_count = btif_a2dp_get_aac_track_channel_count(aac_cie.channels);
            memcpy(&config_req.peer_bd,(UINT8*)(p_data->avk_config.bd_addr),
                                                              sizeof(config_req.peer_bd));
            btif_transfer_context(btif_av_handle_event, BTIF_AV_SINK_CONFIG_REQ_EVT,
                                     (char*)&config_req, sizeof(config_req), NULL);
        }
        else
        {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsAACInfo fail:%d", a2d_status);
        }
    }
#endif

    if (p_data->avk_config.codec_type == BTA_AV_CODEC_NON)
    {
        tA2D_LHDC_CIE lhdc;
        BTIF_TRACE_DEBUG("%s, BTA_AV_MEDIA_SINK_CFG_EVT", __FUNCTION__);
        /* send a command to BT Media Task */
        btif_reset_decoder((UINT8*)(p_data->avk_config.codec_info));
        a2d_status = A2D_ParsLHDCInfo(&lhdc, (UINT8 *)(p_data->avk_config.codec_info), FALSE);
        if (a2d_status == A2D_SUCCESS) {
            /* Switch to BTIF context */
            config_req.sample_rate = btif_a2dp_get_lhdc_track_frequency(lhdc.samp_freq);
            config_req.channel_count = btif_a2dp_get_lhdc_track_channel_count(lhdc.channels);
            memcpy(&config_req.peer_bd,(UINT8*)(p_data->avk_config.bd_addr),
                                                              sizeof(config_req.peer_bd));
            btif_transfer_context(btif_av_handle_event, BTIF_AV_SINK_CONFIG_REQ_EVT,
                                     (char*)&config_req, sizeof(config_req), NULL);
        }
        else
        {
            APPL_TRACE_ERROR("ERROR dump_codec_info A2D_ParsLHDCInfo fail:%d", a2d_status);
        }
    }
}
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static void bte_av_media_callback_by_bda(tBTA_AV_EVT event, tBTA_AV_MEDIA *p_data, bt_bdaddr_t* bd_addr)
#else
static void bte_av_media_callback(tBTA_AV_EVT event, tBTA_AV_MEDIA *p_data)
#endif
{
    btif_sm_state_t state;
    UINT8 que_len;

    if (event == BTA_AV_MEDIA_DATA_EVT)/* Switch to BTIF_MEDIA context */
    {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        int idx = btif_av_get_dev_idx_by_bda(bd_addr);
        if (-1 == idx)
        {
            BTIF_TRACE_DEBUG("%s not device, drop data", __FUNCTION__);
            return;
        }
        if (FALSE == btif_av_cbs[idx].active)
        {
            BTIF_TRACE_DEBUG("%s not active, drop data", __FUNCTION__);
            return;
        }
        state= btif_sm_get_state(btif_av_cbs[idx].sm_handle);
#else
        state= btif_sm_get_state(btif_av_cb.sm_handle);
#endif
        if ( (state == BTIF_AV_STATE_STARTED) || /* send SBC packets only in Started State */
             (state == BTIF_AV_STATE_OPENED) )
        {
            que_len = btif_media_sink_enque_buf((BT_HDR *)p_data);
            //BTIF_TRACE_DEBUG(" Packets in Que %d",que_len);
        }
        else
        {
            return;
        }
    }

    if (event == BTA_AV_MEDIA_SINK_CFG_EVT)
    {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        btif_av_avk_config avk_config;
        avk_config.codec_type = p_data->avk_config.codec_type;
        memcpy(avk_config.codec_info, p_data->avk_config.codec_info, sizeof(avk_config.codec_info));
        btif_transfer_context_by_bda(btif_av_handle_event_by_bda, BTIF_AV_SINK_CONFIG_REQ_EVT,
                      (char*)&avk_config, sizeof(avk_config),
                      NULL, bd_addr, 0);
        return;
#else
        btif_transfer_context(btif_av_handle_event, BTA_AV_MEDIA_SINK_CFG_EVT,
                                     (char*)p_data, sizeof(*p_data), NULL);

#endif
    }
}

#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE)

#include "osi/include/thread.h"
#include "osi/include/properties.h"

static thread_t *pts_av_test_thread;

static void btif_av_pts_test_thread(UNUSED_ATTR void *context) {
    char value[PROPERTY_VALUE_MAX] = {0};

    /* properties
       [1]Use "setprop bluetooth.pts.avdt.recfg on" to trigger avdt reconfiguration
           -- AVDTP/SNK/INT/SIG/SMG/BV-13-C
           -- AVDTP/SNK/INT/SIG/SMG/ESR05/BV-13-C

       [2] Use "setprop bluetooth.pts.avdt.audio 00" to send advdt audio command.(here 00 is hex value)
                  01: av start
                  02: av stop
           -- AVDTP/SNK/INT/SIG/SMG/BV-19-C
    */

    while(1){
        if ((osi_property_get("bluetooth.pts.avdt.recfg", value, "off"))
                && (!strcmp(value, "on"))){

            BTIF_TRACE_EVENT("[pts] run force_reconfig");

            //force to suspend and do reconfiguration.
            extern void bta_av_co_pts_force_reconfig(void);
            bta_av_co_pts_force_reconfig();

            osi_property_set("bluetooth.pts.avdt.recfg","off");
        }else if((osi_property_get("bluetooth.pts.avdt.audio", value, "00"))
                    && (strcmp(value, "00"))){
            UINT8 option;
            ascii_2_hex(value,2,&option);

            BTIF_TRACE_EVENT("[pts] audio opetion %d",option);

            if(option == 1){
                BTA_AvStart();
            }else if(option == 2){
                //btif_dispatch_sm_event(BTIF_AV_STOP_STREAM_REQ_EVT, NULL, 0);
                //BTA_AvStop(FALSE);
                //BTA_AvClose(btif_av_cb.bta_handle);
                BTA_AvClose(btif_av_cb.bta_handle);
            }
            osi_property_set("bluetooth.pts.avdt.audio","00");
        }

        sleep(1);
    }

}

static void btif_av_pts_test_init()
{
    //1. create a thread to check property
    // 2. check the property and do sth.
    pts_av_test_thread = thread_new("pts_av_test");
    BTIF_TRACE_EVENT("[pts] start %s()",__func__);

    thread_post(pts_av_test_thread, btif_av_pts_test_thread, NULL);
}

#endif //(end #ifdef MTK_PTS_AV_TEST


#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
void btif_av_init_cb(void)
{
    memset(&btif_av_cbs, 0, sizeof(btif_av_cbs));
}
#endif

#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
bt_status_t btif_av_init_both(int service_id)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    if (btif_av_cbs[0].sm_handle == NULL)
#else
    if (btif_av_cb.sm_handle == NULL)
#endif
    {

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        if (!btif_a2dp_start_media_task())
            return BT_STATUS_FAIL;
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
        {
             /* if this timer is not canceled, and do power off then power on,
             * this timer's queue is a invaid queue
             */
            if (NULL == av_open_on_rc_timers[xx])
            {
                av_open_on_rc_timers[xx] = alarm_new("btif_av.av_open_on_rc_timer");
            }

#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
            if (NULL == tle_av_delay_start_cmds[xx])
            {
                tle_av_delay_start_cmds[xx] = alarm_new("btif_av.tle_av_delay_start_cmd");
            }
#endif
            /* Also initialize the AV state machine , to avoid sm_handle is null after av register*/
            btif_av_cbs[xx].sm_handle =
                btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE, &btif_av_cbs[xx]);
        }
#else /* !MTK_A2DP_DUAL_AUDIO */
        /* if this timer is not canceled, and do power off then power on,
         * this timer's queue is a invaid queue
         */
        if (NULL == av_open_on_rc_timer)
        {
            av_open_on_rc_timer = alarm_new("btif_av.av_open_on_rc_timer");
        }

#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
        if (NULL == tle_av_delay_start_cmd)
        {
            tle_av_delay_start_cmd = alarm_new("btif_av.tle_av_delay_start_cmd");
        }
#endif

        if (!btif_a2dp_start_media_task())
            return BT_STATUS_FAIL;

        /* Also initialize the AV state machine */
        btif_av_cb.sm_handle =
                btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE);
#endif

#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE)
        btif_av_pts_test_init();
#endif
    }

    if (NULL == bt_av_sink_callbacks && BTA_A2DP_SINK_SERVICE_ID == service_id)
    {
        btif_enable_service(service_id);

        btif_a2dp_on_init();
    }

    if (NULL == bt_av_src_callbacks && BTA_A2DP_SOURCE_SERVICE_ID == service_id)
    {
        btif_enable_service(service_id);
    }
    return BT_STATUS_SUCCESS;
}
#endif

/*******************************************************************************
**
** Function         btif_av_init
**
** Description      Initializes btif AV if not already done
**
** Returns          bt_status_t
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
bt_status_t btif_av_init(int service_id)
{
    if ((NULL == bt_av_sink_callbacks) && (NULL == bt_av_src_callbacks))
    {
        btif_av_init_cb();
    }

#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
    return btif_av_init_both(service_id);
#endif

    if (btif_av_cbs[0].sm_handle == NULL)
    {
        if (!btif_a2dp_start_media_task())
            return BT_STATUS_FAIL;

#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
        {
            BTIF_AV_LOCK();
            /* Also initialize the AV state machine , to avoid sm_handle is null after av register*/
            btif_av_cbs[xx].sm_handle =
                btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE, &btif_av_cbs[xx]);
            BTIF_AV_UNLOCK();
        }
        btif_enable_service(service_id);
#else
        btif_enable_service(service_id);

        for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
        {
            alarm_free(av_open_on_rc_timers[xx]);
            av_open_on_rc_timers[xx] = alarm_new("btif_av.av_open_on_rc_timer");

#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
            alarm_free(tle_av_delay_start_cmds[xx]);
            tle_av_delay_start_cmds[xx] = alarm_new("btif_av.tle_av_delay_start_cmd");
#endif
            BTIF_AV_LOCK();
            /* Also initialize the AV state machine */
            btif_av_cbs[xx].sm_handle =
                    btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE, &btif_av_cbs[xx]);
            BTIF_AV_UNLOCK();
        }
#endif

        btif_a2dp_on_init();

#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE)
        btif_av_pts_test_init();
#endif
    }

    return BT_STATUS_SUCCESS;
}
#else
bt_status_t btif_av_init(int service_id)
{
#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
    return btif_av_init_both(service_id);
#endif

    if (btif_av_cb.sm_handle == NULL)
    {
        alarm_free(av_open_on_rc_timer);
        av_open_on_rc_timer = alarm_new("btif_av.av_open_on_rc_timer");


#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
        alarm_free(tle_av_delay_start_cmd);
        tle_av_delay_start_cmd = alarm_new("btif_av.tle_av_delay_start_cmd");
#endif

        if (!btif_a2dp_start_media_task())
            return BT_STATUS_FAIL;

        BTIF_AV_LOCK();
#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        /* Also initialize the AV state machine , to avoid sm_handle is null after av register*/
        btif_av_cb.sm_handle =
                btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE);

        btif_enable_service(service_id);
#else
        btif_enable_service(service_id);

        /* Also initialize the AV state machine */
        btif_av_cb.sm_handle =
                btif_sm_init((const btif_sm_handler_t*)btif_av_state_handlers, BTIF_AV_STATE_IDLE);
#endif
        BTIF_AV_UNLOCK();

        btif_a2dp_on_init();

#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE)
        btif_av_pts_test_init();
#endif
    }

    return BT_STATUS_SUCCESS;
}
#endif

/*******************************************************************************
**
** Function         init_src
**
** Description      Initializes the AV interface for source mode
**
** Returns          bt_status_t
**
*******************************************************************************/

static bt_status_t init_src(btav_callbacks_t* callbacks)
{
    BTIF_TRACE_EVENT("%s()", __func__);

    bt_status_t status = btif_av_init(BTA_A2DP_SOURCE_SERVICE_ID);
    if (status == BT_STATUS_SUCCESS)
        bt_av_src_callbacks = callbacks;

    return status;
}

/*******************************************************************************
**
** Function         init_sink
**
** Description      Initializes the AV interface for sink mode
**
** Returns          bt_status_t
**
*******************************************************************************/

static bt_status_t init_sink(btav_callbacks_t* callbacks)
{
    BTIF_TRACE_EVENT("%s()", __func__);

    bt_status_t status = btif_av_init(BTA_A2DP_SINK_SERVICE_ID);
    if (status == BT_STATUS_SUCCESS)
        bt_av_sink_callbacks = callbacks;

    return status;
}

#ifdef USE_AUDIO_TRACK
/*******************************************************************************
**
** Function         update_audio_focus_state
**
** Description      Updates the final focus state reported by components calling
**                  this module.
**
** Returns          None
**
*******************************************************************************/
void update_audio_focus_state(int state)
{
    BTIF_TRACE_DEBUG("%s state %d ",__func__, state);
    btif_a2dp_set_audio_focus_state(state);
}

/*******************************************************************************
**
** Function         update_audio_track_gain
**
** Description      Updates the track gain (used for ducking).
**
** Returns          None
**
*******************************************************************************/
void update_audio_track_gain(float gain)
{
    BTIF_TRACE_DEBUG("%s gain %f ",__func__, gain);
    btif_a2dp_set_audio_track_gain(gain);
}
#endif

/*******************************************************************************
**
** Function         connect
**
** Description      Establishes the AV signalling channel with the remote headset
**
** Returns          bt_status_t
**
*******************************************************************************/

static bt_status_t connect_int(bt_bdaddr_t *bd_addr, uint16_t uuid)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
int idx = -1;
#define btif_av_cb btif_av_cbs[idx]
#endif
    btif_av_connect_req_t connect_req;
    connect_req.target_bda = bd_addr;
    connect_req.uuid = uuid;
    BTIF_TRACE_EVENT("%s uuid=0x%x", __FUNCTION__, uuid);

    if (uuid == ~UUID_SERVCLASS_AUDIO_SINK)
    {
        HAL_CBACK(bt_av_sink_callbacks, connection_state_cb,
            BTAV_CONNECTION_STATE_DISCONNECTED, bd_addr);
        return BT_STATUS_FAIL;
    }
    else if (uuid == ~UUID_SERVCLASS_AUDIO_SOURCE)
    {
        HAL_CBACK(bt_av_src_callbacks, connection_state_cb,
            BTAV_CONNECTION_STATE_DISCONNECTED, bd_addr);
        return BT_STATUS_FAIL;
    }

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    idx = btif_av_find_or_alloc_dev_idx(bd_addr);
    if (idx < 0)
    {
        if (UUID_SERVCLASS_AUDIO_SINK == uuid)
        {
            HAL_CBACK(bt_av_sink_callbacks, connection_state_cb,
                BTAV_CONNECTION_STATE_DISCONNECTED, bd_addr);

        }
        else
        {
            HAL_CBACK(bt_av_src_callbacks, connection_state_cb,
                BTAV_CONNECTION_STATE_DISCONNECTED, bd_addr);
        }
        btif_queue_advance();
        return BT_STATUS_FAIL;
    }
#endif

    btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CONNECT_REQ_EVT, (char*)&connect_req);

    return BT_STATUS_SUCCESS;
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
#undef btif_av_cb
#endif
}

#if defined(MTK_PTS_AVRCP_CT_PTH_BV_01_C) && (MTK_PTS_AVRCP_CT_PTH_BV_01_C == TRUE)
    // There is no need src_connect_sink, also this define couldn't in LinuxBluedroid

#else
static bt_status_t src_connect_sink(bt_bdaddr_t *bd_addr)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    CHECK_BTAV_INIT();

    return btif_queue_connect(UUID_SERVCLASS_AUDIO_SOURCE, bd_addr, connect_int);
}
#endif

static bt_status_t sink_connect_src(bt_bdaddr_t *bd_addr)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    CHECK_BTAV_INIT();
    return btif_queue_connect(UUID_SERVCLASS_AUDIO_SINK, bd_addr, connect_int);
}

/*******************************************************************************
**
** Function         disconnect
**
** Description      Tears down the AV signalling channel with the remote headset
**
** Returns          bt_status_t
**
*******************************************************************************/
static bt_status_t disconnect(bt_bdaddr_t *bd_addr)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    CHECK_BTAV_INIT();

    /* Switch to BTIF context */
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    return btif_transfer_context_by_bda(btif_av_handle_event_by_bda, BTIF_AV_DISCONNECT_REQ_EVT,
                                 (char*)bd_addr, sizeof(bt_bdaddr_t), NULL, bd_addr, 0);
#else
    return btif_transfer_context(btif_av_handle_event, BTIF_AV_DISCONNECT_REQ_EVT,
                                 (char*)bd_addr, sizeof(bt_bdaddr_t), NULL);
#endif
}

/*******************************************************************************
**
** Function         cleanup
**
** Description      Shuts down the AV interface and does the cleanup
**
** Returns          None
**
*******************************************************************************/
static void cleanup(int service_uuid)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#else
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_transfer_context_by_bda(btif_av_handle_event_by_bda, BTIF_AV_CLEANUP_REQ_EVT, NULL, 0, NULL, NULL, 0);
#else
    btif_transfer_context(btif_av_handle_event, BTIF_AV_CLEANUP_REQ_EVT, NULL, 0, NULL);
#endif
#endif

    btif_disable_service(service_uuid);

#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    BTIF_TRACE_EVENT("return cleanup");
    return;
#endif

    BTIF_AV_LOCK();
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        /* Also shut down the AV state machine */
        btif_sm_shutdown(btif_av_cbs[xx].sm_handle);
        btif_av_cbs[xx].sm_handle = NULL;
    }
#else
    /* Also shut down the AV state machine */
    btif_sm_shutdown(btif_av_cb.sm_handle);
    btif_av_cb.sm_handle = NULL;
#endif
    BTIF_AV_UNLOCK();

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    btif_a2dp_stop_media_task();
    BTIF_TRACE_EVENT("release connect_queue when a2dp cleanup");
    btif_queue_release();
#endif
}

static void cleanup_src(void) {
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    if (bt_av_src_callbacks)
    {
#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        if (bt_av_sink_callbacks == NULL)
            cleanup(BTA_A2DP_SOURCE_SERVICE_ID);
        else
            bt_av_src_callbacks = NULL;
#else
        bt_av_src_callbacks = NULL;
        if (bt_av_sink_callbacks == NULL)
            cleanup(BTA_A2DP_SOURCE_SERVICE_ID);
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        else
            btif_clean_service_id(BTA_A2DP_SOURCE_SERVICE_ID);
#endif
#endif
    }
}

static void cleanup_sink(void) {
    BTIF_TRACE_EVENT("%s", __FUNCTION__);

    if (bt_av_sink_callbacks)
    {
#if !defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        if (bt_av_src_callbacks == NULL)
            cleanup(BTA_A2DP_SINK_SERVICE_ID);
        bt_av_sink_callbacks = NULL;
        BTIF_TRACE_DEBUG("cleanup bt_av_sink_callbacks");
#else
        bt_av_sink_callbacks = NULL;
        if (bt_av_src_callbacks == NULL)
            cleanup(BTA_A2DP_SINK_SERVICE_ID);
#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
        else
            btif_clean_service_id(BTA_A2DP_SINK_SERVICE_ID);
#endif
#endif
    }
}

static const btav_interface_t bt_av_src_interface = {
    sizeof(btav_interface_t),
    init_src,
#if defined(MTK_PTS_AVRCP_CT_PTH_BV_01_C) && (MTK_PTS_AVRCP_CT_PTH_BV_01_C == TRUE)
    sink_connect_src,
#else
    src_connect_sink,
#endif
    disconnect,
    cleanup_src,
    NULL,
    NULL,
};

static const btav_interface_t bt_av_sink_interface = {
    sizeof(btav_interface_t),
    init_sink,
    sink_connect_src,
    disconnect,
    cleanup_sink,
#ifdef USE_AUDIO_TRACK
    update_audio_focus_state,
    update_audio_track_gain,
#else
    NULL,
    NULL,
#endif
};

/*******************************************************************************
**
** Function         btif_av_get_sm_handle
**
** Description      Fetches current av SM handle
**
** Returns          None
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
btif_sm_handle_t btif_av_get_sm_handle_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    return btif_av_cbs[idx].sm_handle;
}
#else
btif_sm_handle_t btif_av_get_sm_handle(void)
{
    return btif_av_cb.sm_handle;
}
#endif

/*******************************************************************************
**
** Function         btif_av_get_addr
**
** Description      Fetches current AV BD address
**
** Returns          BD address
**
*******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
bt_bdaddr_t btif_av_get_active_addr(void)
{
    bt_bdaddr_t bda;
    for (int xx=0;xx<BTA_AV_NUM_CONNS;xx++)
    {
        if(btif_av_cbs[xx].active == TRUE)
        {
            return btif_av_cbs[xx].peer_bda;
        }
    }
    memset(&bda, 0, sizeof(bda));
    return bda;
}

BOOLEAN btif_av_check_addr(bt_bdaddr_t *addr)
{
    for (int xx=0;xx<BTA_AV_NUM_CONNS;xx++)
    {
        if(btif_av_cbs[xx].in_use &&
            0 == bdcmp(btif_av_cbs[xx].peer_bda.address, addr->address))
        {
            return TRUE;
        }
    }
    return FALSE;
}
#else
bt_bdaddr_t btif_av_get_addr(void)
{
    return btif_av_cb.peer_bda;
}
#endif

/*******************************************************************************
** Function         btif_av_is_sink_enabled
**
** Description      Checks if A2DP Sink is enabled or not
**
** Returns          TRUE if A2DP Sink is enabled, false otherwise
**
*******************************************************************************/

BOOLEAN btif_av_is_sink_enabled(void)
{
    return (bt_av_sink_callbacks != NULL) ? TRUE : FALSE;
}

/*******************************************************************************
**
** Function         btif_av_stream_ready
**
** Description      Checks whether AV is ready for starting a stream
**
** Returns          None
**
*******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_stream_ready_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    btif_sm_state_t state = btif_sm_get_state(btif_av_cbs[idx].sm_handle);

    BTIF_TRACE_DEBUG("btif_av_stream_ready_by_bda : sm hdl %p, state %u, flags %x",
        btif_av_cbs[idx].sm_handle, state, btif_av_cbs[idx].flags);

    /* also make sure main adapter is enabled */
    if (btif_is_enabled() == 0)
    {
        BTIF_TRACE_EVENT("main adapter not enabled");
        return FALSE;
    }

    /* check if we are remotely suspended or stop is pending */
    if (btif_av_cbs[idx].flags & (BTIF_AV_FLAG_REMOTE_SUSPEND|BTIF_AV_FLAG_PENDING_STOP))
        return FALSE;

    return (state == BTIF_AV_STATE_OPENED);
}
#else
BOOLEAN btif_av_stream_ready(void)
{
    btif_sm_state_t state = btif_sm_get_state(btif_av_cb.sm_handle);

    BTIF_TRACE_DEBUG("btif_av_stream_ready : sm hdl %p, state %u, flags %x",
        btif_av_cb.sm_handle, state, btif_av_cb.flags);

    /* also make sure main adapter is enabled */
    if (btif_is_enabled() == 0)
    {
        BTIF_TRACE_EVENT("main adapter not enabled");
        return FALSE;
    }

    /* check if we are remotely suspended or stop is pending */
    if (btif_av_cb.flags & (BTIF_AV_FLAG_REMOTE_SUSPEND|BTIF_AV_FLAG_PENDING_STOP))
        return FALSE;

    return (state == BTIF_AV_STATE_OPENED);
}
#endif

/*******************************************************************************
**
** Function         btif_av_stream_started_ready
**
** Description      Checks whether AV ready for media start in streaming state
**
** Returns          None
**
*******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_stream_started_ready_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    btif_sm_state_t state = btif_sm_get_state(btif_av_cbs[idx].sm_handle);

    BTIF_TRACE_DEBUG("btif_av_stream_started : sm hdl %d, state %d, flags %x",
                btif_av_cbs[idx].sm_handle, state, btif_av_cbs[idx].flags);

    /* disallow media task to start if we have pending actions */
    if (btif_av_cbs[idx].flags & (BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING | BTIF_AV_FLAG_REMOTE_SUSPEND
        | BTIF_AV_FLAG_PENDING_STOP))
        return FALSE;

    return (state == BTIF_AV_STATE_STARTED);
}
#else
BOOLEAN btif_av_stream_started_ready(void)
{
    btif_sm_state_t state = btif_sm_get_state(btif_av_cb.sm_handle);

    BTIF_TRACE_DEBUG("btif_av_stream_started : sm hdl %p, state %u, flags %x",
                btif_av_cb.sm_handle, state, btif_av_cb.flags);

    /* disallow media task to start if we have pending actions */
    if (btif_av_cb.flags & (BTIF_AV_FLAG_LOCAL_SUSPEND_PENDING | BTIF_AV_FLAG_REMOTE_SUSPEND
        | BTIF_AV_FLAG_PENDING_STOP))
        return FALSE;

    return (state == BTIF_AV_STATE_STARTED);
}
#endif

/*******************************************************************************
**
** Function         btif_dispatch_sm_event
**
** Description      Send event to AV statemachine
**
** Returns          None
**
*******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
void btif_dispatch_sm_event_by_bda(btif_av_sm_event_t event, void *p_data,
    int len, const bt_bdaddr_t *addr, UINT32 handle)
{
    /* Switch to BTIF context */
    btif_transfer_context_by_bda(btif_av_handle_event_by_bda, event,
                          (char*)p_data, len, NULL, addr, handle);
}
#else
/* used to pass events to AV statemachine from other tasks */
void btif_dispatch_sm_event(btif_av_sm_event_t event, void *p_data, int len)
{
    /* Switch to BTIF context */
    btif_transfer_context(btif_av_handle_event, event,
                          (char*)p_data, len, NULL);
}
#endif

/*******************************************************************************
**
** Function         btif_av_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btif_av_execute_service(BOOLEAN b_enable)
{
    if (b_enable)
    {
        /* TODO: Removed BTA_SEC_AUTHORIZE since the Java/App does not
         * handle this request in order to allow incoming connections to succeed.
         * We need to put this back once support for this is added */

        /* Added BTA_AV_FEAT_NO_SCO_SSPD - this ensures that the BTA does not
         * auto-suspend av streaming on AG events(SCO or Call). The suspend shall
         * be initiated by the app/audioflinger layers */
        /* Support for browsing for SDP record should work only if we enable BROWSE
         * while registering. */
#if (AVRC_METADATA_INCLUDED == TRUE)
        BTA_AvEnable(BTA_SEC_AUTHENTICATE,
            BTA_AV_FEAT_RCTG|BTA_AV_FEAT_METADATA|BTA_AV_FEAT_VENDOR|BTA_AV_FEAT_NO_SCO_SSPD
#if (AVRC_ADV_CTRL_INCLUDED == TRUE)
            |BTA_AV_FEAT_RCCT
            |BTA_AV_FEAT_ADV_CTRL
#endif
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
            |BTA_AV_FEAT_BROWSE
#endif
#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE) //tt add to test PTS     -- AVDTP/SRC/INT/SIG/SEC/BV-01-C
            |BTA_AV_FEAT_PROTECT
#endif
            ,bte_av_callback);
#else
        BTA_AvEnable(BTA_SEC_AUTHENTICATE, (BTA_AV_FEAT_RCTG | BTA_AV_FEAT_NO_SCO_SSPD),
                    bte_av_callback);
#endif
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        for(int xx = 0; xx < btif_av_src_max_cnt; xx++)
        {
            BTA_AvRegister(BTA_AV_CHNL_AUDIO+xx, BTIF_AV_SERVICE_NAME, 0, bte_av_media_callback_by_bda,
                                                             UUID_SERVCLASS_AUDIO_SOURCE);
        }
#else
        BTA_AvRegister(BTA_AV_CHNL_AUDIO, BTIF_AV_SERVICE_NAME, 0, bte_av_media_callback,
                                                             UUID_SERVCLASS_AUDIO_SOURCE);
#endif
    }
    else {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        for(int xx = 0; xx < btif_av_src_max_cnt; xx++)
        {
            BTA_AvDeregister(btif_av_cbs[xx].bta_handle);
        }
#else
         BTA_AvDeregister(btif_av_cb.bta_handle);
#endif
         BTA_AvDisable();
     }
     return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_av_sink_execute_service
**
** Description      Initializes/Shuts down the service
**
** Returns          BT_STATUS_SUCCESS on success, BT_STATUS_FAIL otherwise
**
*******************************************************************************/
bt_status_t btif_av_sink_execute_service(BOOLEAN b_enable)
{
    if (b_enable)
    {
        /* Added BTA_AV_FEAT_NO_SCO_SSPD - this ensures that the BTA does not
         * auto-suspend av streaming on AG events(SCO or Call). The suspend shall
         * be initiated by the app/audioflinger layers */
        BTA_AvEnable(BTA_SEC_AUTHENTICATE, BTA_AV_FEAT_NO_SCO_SSPD|BTA_AV_FEAT_RCCT|
                                            BTA_AV_FEAT_METADATA|BTA_AV_FEAT_VENDOR|
                                            BTA_AV_FEAT_ADV_CTRL|BTA_AV_FEAT_RCTG
#if defined(MTK_AVRCP_TG_15_BROWSE)&&(MTK_AVRCP_TG_15_BROWSE == TRUE)
                                            |BTA_AV_FEAT_BROWSE
#endif
#if defined(MTK_PTS_AV_TEST) && (MTK_PTS_AV_TEST == TRUE) //tt add to test PTS     -- AVDTP/SNK/ACP/SIG/SEC/BV-02-C
                                            |BTA_AV_FEAT_PROTECT
#endif
                                            ,bte_av_callback);

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        for(int xx = 0; xx < btif_av_sink_max_cnt; xx++)
        {
            BTA_AvRegister(BTA_AV_CHNL_AUDIO+xx, BTIF_AVK_SERVICE_NAME, 0, bte_av_media_callback_by_bda,
                                                                UUID_SERVCLASS_AUDIO_SINK);
        }
#else
        BTA_AvRegister(BTA_AV_CHNL_AUDIO, BTIF_AVK_SERVICE_NAME, 0, bte_av_media_callback,
                                                                UUID_SERVCLASS_AUDIO_SINK);
#endif
    }
    else {
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
        for(int xx = 0; xx < btif_av_sink_max_cnt; xx++)
        {
            BTA_AvDeregister(btif_av_cbs[xx].bta_handle);
        }
#else
        BTA_AvDeregister(btif_av_cb.bta_handle);
#endif
        BTA_AvDisable();
    }
    return BT_STATUS_SUCCESS;
}

/*******************************************************************************
**
** Function         btif_av_get_src_interface
**
** Description      Get the AV callback interface for A2DP source profile
**
** Returns          btav_interface_t
**
*******************************************************************************/
const btav_interface_t *btif_av_get_src_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_av_src_interface;
}

/*******************************************************************************
**
** Function         btif_av_get_sink_interface
**
** Description      Get the AV callback interface for A2DP sink profile
**
** Returns          btav_interface_t
**
*******************************************************************************/
const btav_interface_t *btif_av_get_sink_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_av_sink_interface;
}

/*******************************************************************************
**
** Function         btif_av_is_connected
**
** Description      Checks if av has a connected sink
**
** Returns          BOOLEAN
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_is_connected_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    btif_sm_state_t state = btif_sm_get_state(btif_av_cbs[idx].sm_handle);
    return ((state == BTIF_AV_STATE_OPENED) || (state ==  BTIF_AV_STATE_STARTED));
}
#else
BOOLEAN btif_av_is_connected(void)
{
    btif_sm_state_t state = btif_sm_get_state(btif_av_cb.sm_handle);
    return ((state == BTIF_AV_STATE_OPENED) || (state ==  BTIF_AV_STATE_STARTED));
}
#endif


#if defined(MTK_A2DP_SRC_SINK_BOTH) && (MTK_A2DP_SRC_SINK_BOTH == TRUE)
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_peer_is_src_by_bda(bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;
    return (btif_av_cbs[idx].peer_sep == AVDT_TSEP_SRC);
}
#else
BOOLEAN btif_av_peer_is_src(void)
{
    return (btif_av_cb.peer_sep == AVDT_TSEP_SRC);
}
#endif

BOOLEAN btif_av_both_enable(void)
{
    return (bt_av_src_callbacks != NULL && bt_av_sink_callbacks != NULL);
}
#endif

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
/*******************************************************************************
 * **
 * ** Function         btif_av_is_idle
 * **
 * ** Description      Checks if av in idle state
 * **
 * ** Returns          BOOLEAN
 * **
 * *******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_is_idle_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    btif_sm_state_t state = btif_sm_get_state(btif_av_cbs[idx].sm_handle);
    return (state ==  BTIF_AV_STATE_IDLE);
}
#else
BOOLEAN btif_av_is_idle(void)
{
    btif_sm_state_t state = btif_sm_get_state(btif_av_cb.sm_handle);
        return (state ==  BTIF_AV_STATE_IDLE);
}
#endif
#endif

/*******************************************************************************
**
** Function         btif_av_is_peer_edr
**
** Description      Check if the connected a2dp device supports
**                  EDR or not. Only when connected this function
**                  will accurately provide a true capability of
**                  remote peer. If not connected it will always be false.
**
** Returns          TRUE if remote device is capable of EDR
**
*******************************************************************************/

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_is_peer_edr_by_bda(const bt_bdaddr_t* bd_addr)
{
    ASSERTC(btif_av_is_connected_by_bda(bd_addr), "No active a2dp connection", 0);

    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    if (btif_av_cbs[idx].edr)
        return TRUE;
    else
        return FALSE;
}
#else
BOOLEAN btif_av_is_peer_edr(void)
{
    ASSERTC(btif_av_is_connected(), "No active a2dp connection", 0);
    if (btif_av_cb.edr)
        return TRUE;
    else
        return FALSE;
}
#endif

/******************************************************************************
**
** Function        btif_av_clear_remote_suspend_flag
**
** Description     Clears btif_av_cd.flags if BTIF_AV_FLAG_REMOTE_SUSPEND is set
**
** Returns          void
******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
void btif_av_clear_remote_suspend_flag_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return;

    BTIF_TRACE_DEBUG("%s: flag :%x",__func__, btif_av_cbs[idx].flags);
    btif_av_cbs[idx].flags &= ~BTIF_AV_FLAG_REMOTE_SUSPEND;
}
#else
void btif_av_clear_remote_suspend_flag(void)
{
    BTIF_TRACE_DEBUG("%s: flag :%x",__func__, btif_av_cb.flags);
    btif_av_cb.flags &= ~BTIF_AV_FLAG_REMOTE_SUSPEND;
}
#endif


#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_is_black_peer_by_bda(const bt_bdaddr_t* bd_addr)
{
    BTIF_TRACE_DEBUG("%s", __func__);

    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    if (sizeof(btif_av_cbs[idx].peer_bda) != 0) {
        if (interop_mtk_match_addr_name(INTEROP_MTK_A2DP_DELAY_START_CMD, (const bt_bdaddr_t *)&btif_av_cbs[idx].peer_bda)) {
            alarm_set_on_queue(tle_av_delay_start_cmds[idx], BTIF_TIMER_A2DP_DELAY_START_CMD,
                   btif_media_av_delay_start_cmd_hdlr, tle_av_delay_start_cmds[idx],
                   btu_general_alarm_queue);
            BTIF_TRACE_DEBUG("%s return true", __func__);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}
#else
/*****************************************************************************
**
** Function         btif_av_is_black_peer
**
** Description      Some special device want perform START cmd itself first
**                  If it not send START cmd, will close current link(ex. Tiggo5).
**                  So for this special device, we need delay send START cmd
**                  which from DUT to receive the special device cmd.
**
** Return           TRUE if it is special peer device
**
****************************************************************************/
BOOLEAN btif_av_is_black_peer(void)
{
    BTIF_TRACE_DEBUG("%s", __func__);

    if (sizeof(btif_av_cb.peer_bda) != 0) {
        if (interop_mtk_match_addr_name(INTEROP_MTK_A2DP_DELAY_START_CMD, (const bt_bdaddr_t *)&btif_av_cb.peer_bda)) {
            alarm_set_on_queue(tle_av_delay_start_cmd, BTIF_TIMER_A2DP_DELAY_START_CMD,
                   btif_media_av_delay_start_cmd_hdlr, tle_av_delay_start_cmd,
                   btu_general_alarm_queue);
            BTIF_TRACE_DEBUG("%s return true", __func__);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

#endif
#endif

/*******************************************************************************
**
** Function         btif_av_peer_supports_3mbps
**
** Description      Check if the connected A2DP device supports
**                  3 Mbps EDR. This function only works if connected.
**                  If not connected it will always be false.
**
** Returns          TRUE if remote device is EDR and supports 3 Mbps
**
*******************************************************************************/
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
BOOLEAN btif_av_peer_supports_3mbps_by_bda(const bt_bdaddr_t* bd_addr)
{
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (idx < 0) return FALSE;

    BOOLEAN is3mbps = ((btif_av_cbs[idx].edr & BTA_AV_EDR_3MBPS) != 0);
    BTIF_TRACE_DEBUG("%s: connected %d, edr_3mbps %d", __func__,
            btif_av_is_connected_by_bda(bd_addr), is3mbps);
    return (btif_av_is_connected_by_bda(bd_addr) && is3mbps);
}
#else
BOOLEAN btif_av_peer_supports_3mbps(void)
{
    BOOLEAN is3mbps = ((btif_av_cb.edr & BTA_AV_EDR_3MBPS) != 0);
    BTIF_TRACE_DEBUG("%s: connected %d, edr_3mbps %d", __func__,
            btif_av_is_connected(), is3mbps);
    return (btif_av_is_connected() && is3mbps);
}
#endif

#if defined(MTK_SUPPORT_A2DP_2M_TRANSMISSION) && (MTK_SUPPORT_A2DP_2M_TRANSMISSION == TRUE)
/*******************************************************************************
**
** Function         btif_av_peer_supports_2mbps
**
** Description      Check if the connected A2DP device supports
**                  2 Mbps EDR. This function will only work while connected.
**                  If not connected it will always return false.
**
** Returns          TRUE if remote device is EDR and supports 2 Mbps
**
*******************************************************************************/
BOOLEAN btif_av_peer_supports_2mbps(void)
{
    BOOLEAN is2mbps = ((btif_av_cb.edr & BTA_AV_EDR_2MBPS) != 0);
    BTIF_TRACE_DEBUG("%s: connected %d, edr_2mbps %d", __func__,
            btif_av_is_connected(), is2mbps);
    return (btif_av_is_connected() && is2mbps);
}
#endif

#if defined(MTK_LINUX_A2DP_PLUS) && (MTK_LINUX_A2DP_PLUS == TRUE)

bt_status_t btif_av_change_thread_priority(int priority)
{
    BTIF_TRACE_EVENT("## %s priority:%d##", __FUNCTION__, priority);

    thread_set_priority(bt_workqueue_thread, priority);
    thread_set_priority(thread, priority);              // hci_thread

    return BT_STATUS_SUCCESS;
}

static bt_status_t init_src_ext( btav_src_ext_callbacks_t* ext_callbacks, uint8_t src_link_num)
{
    BTIF_TRACE_EVENT("## %s ##, src link num:%d", __FUNCTION__, src_link_num);
    bt_status_t result = BT_STATUS_SUCCESS;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_src_max_cnt = src_link_num;
    //btif_av_src_max_cnt = 2; // ck: temp patch to support connecting two devices
#endif

    bt_av_src_ext_callbacks = ext_callbacks;

    return result;
}

static bt_status_t init_sink_ext( btav_sink_ext_callbacks_t* ext_callbacks, uint8_t sink_link_num)
{
    BTIF_TRACE_EVENT("## %s ##, sink link num:%d", __FUNCTION__, sink_link_num);
    bt_status_t result = BT_STATUS_SUCCESS;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_sink_max_cnt = sink_link_num;
    //btif_av_sink_max_cnt = 2; // ck: temp patch to support connecting two devices
#endif

    bt_av_sink_ext_callbacks = ext_callbacks;

    return result;
}

static bt_status_t init_av_ext( btav_ext_callbacks_t* ext_callbacks )
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);

    bt_status_t result = BT_STATUS_SUCCESS;

    if (bt_av_ext_callbacks)
    {
        return BT_STATUS_SUCCESS;
    }

    bt_av_ext_callbacks = ext_callbacks;

    return result;
}

static bt_status_t btif_av_codec_enable( uint8_t codec_type, int enable)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);
    BTIF_TRACE_EVENT("codec_type[%d], enable[%d]", codec_type, enable);

    btif_av_codec_enable_req_t codec_enable_req;
    codec_enable_req.codec_type = codec_type;
    codec_enable_req.enable = enable;

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    // clayton: todo. Need to find the correct btif_av_cb?
    btif_sm_dispatch(btif_av_cbs[0].sm_handle, BTIF_AV_CODEC_ENABLE_REQ_EVT, (char*)&codec_enable_req);
#else
    btif_sm_dispatch(btif_av_cb.sm_handle, BTIF_AV_CODEC_ENABLE_REQ_EVT, (char*)&codec_enable_req);
#endif
    return BT_STATUS_SUCCESS;
}

static bt_status_t btif_av_get_codec_config(bt_bdaddr_t *bd_addr)
{
    bt_status_t result = BT_STATUS_SUCCESS;

    UINT16 minmtu;
    UINT8 codectype;
    UINT8 codec_info[AVDT_CODEC_SIZE*2] = {0};

    codectype = bta_av_get_current_codec();
    BTIF_TRACE_EVENT("## %s codectype=%d##", __FUNCTION__, codectype);

    switch (codectype)
    {
#if (defined(MTK_A2DP_SRC_STE_CODEC) && (MTK_A2DP_SRC_STE_CODEC == TRUE)) || (defined(MTK_A2DP_SNK_STE_CODEC) && (MTK_A2DP_SNK_STE_CODEC == TRUE))
        case BTIF_AV_CODEC_STE:
            BTIF_TRACE_EVENT("codectype is STE");
            if (FALSE == bta_av_co_audio_get_ste_config((tA2D_SBC_CIE*)&codec_info, &minmtu))
            {
                BTIF_TRACE_ERROR("%s.get STE codec info error", __func__);
            }
            break;
#endif
        case BTIF_AV_CODEC_SBC:
            BTIF_TRACE_EVENT("codectype is SBC");
            if (FALSE == bta_av_co_audio_get_sbc_config((tA2D_SBC_CIE*)&codec_info, &minmtu))
            {
                BTIF_TRACE_ERROR("%s.get SBC codec info error", __func__);
            }
            break;
#if (defined(MTK_A2DP_SRC_AAC_CODEC) && (MTK_A2DP_SRC_AAC_CODEC == TRUE)) || (defined(MTK_A2DP_SNK_AAC_CODEC) && (MTK_A2DP_SNK_AAC_CODEC == TRUE))
        case BTIF_AV_CODEC_AAC:
            BTIF_TRACE_EVENT("codectype is AAC");
            if (FALSE == bta_av_co_audio_get_aac_config((tA2D_AAC_CIE*)&codec_info, &minmtu))
            {
                BTIF_TRACE_ERROR("%s.get AAC codec info error", __func__);
            }
            break;
#endif
        case BTIF_AV_CODEC_NONE:
            BTIF_TRACE_EVENT("codectype is vendor");
            if (FALSE == bta_av_co_audio_get_vendor_codec_config(&codec_info, &minmtu, BTIF_AV_CODEC_NONE))
            {
                BTIF_TRACE_ERROR("%s.get vendor codec info error", __func__);
            }
            break;
        default:
            result = BT_STATUS_FAIL;
            break;
    }


    if (bt_av_ext_callbacks != NULL)
    {
        HAL_CBACK(bt_av_ext_callbacks, audio_codec_config_cb, bd_addr, codectype, codec_info);
    }
    else
    {
        BTIF_TRACE_ERROR("%s.bt_av_ext_callbacks is NULL", __func__);
    }

    return result;
}

static void btif_av_src_adjust_bitpool(int bitpool)
{
    BTIF_TRACE_EVENT("## %s ##", __FUNCTION__);

    btif_av_src_change_bitpool(bitpool);
}

void btif_av_active_src(bt_bdaddr_t *bd_addr, int active)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_active_req_t active_req;
    active_req.target_bda = bd_addr;
    BTIF_AV_PRINT_BDA(bd_addr, "active to %d", active);
    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (-1 == idx)
    {
        BTIF_TRACE_ERROR("%s: no dev", __func__);
        return;
    }

    btif_sm_dispatch(btif_av_cbs[idx].sm_handle, BTIF_AV_ACTIVE_REQ_EVT, (char*)&active_req);

#endif
}

void btif_av_active_sink(bt_bdaddr_t *bd_addr, int active)
{
#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
    btif_av_active_req_t active_req;
    active_req.target_bda = bd_addr;
    BTIF_AV_PRINT_BDA(bd_addr, "active to %d", active);

    int idx = btif_av_get_dev_idx_by_bda(bd_addr);
    if (-1 == idx)
    {
        BTIF_TRACE_DEBUG("%s: no dev", __func__);
        return;
    }

    btif_sm_dispatch(btif_av_cbs[idx].sm_handle, BTIF_AV_ACTIVE_REQ_EVT, (char*)&active_req);

#endif
}////THL



static const btav_src_ext_interface_t bt_av_src_ext_interface = {
    sizeof(btav_src_ext_interface_t),
    init_src_ext,
    btif_av_src_adjust_bitpool,
    btif_av_active_sink,
};

static const btav_sink_ext_interface_t bt_av_sink_ext_interface = {
    sizeof(btav_sink_ext_interface_t),
    init_sink_ext,
    btif_av_active_src,
};

static const btav_ext_interface_t bt_av_ext_interface = {
    sizeof(btav_ext_interface_t),
    init_av_ext,
    btif_av_change_thread_priority,
    btif_av_codec_enable,
};

/*******************************************************************************
**
** Function         btif_av_get_src_ext_interface
**
** Description      Get the A2DP (src) expanded callback interface
**
** Returns          btav_src_ext_interface_t
**
*******************************************************************************/
const btav_src_ext_interface_t *btif_av_get_src_ext_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_av_src_ext_interface;
}

/*******************************************************************************
**
** Function         btif_av_get_ext_interface
**
** Description      Get the A2DP  expanded callback interface
**
** Returns          btav_ext_interface_t
**
*******************************************************************************/
const btav_ext_interface_t *btif_av_get_ext_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_av_ext_interface;
}

const btav_sink_ext_interface_t *btif_av_get_sink_ext_interface(void)
{
    BTIF_TRACE_EVENT("%s", __FUNCTION__);
    return &bt_av_sink_ext_interface;
}

#endif

#if defined(MTK_BT_PERFORMANCE_ANALYSIS) && (MTK_BT_PERFORMANCE_ANALYSIS == TRUE)
/*******************************************************************************
**
** Function         btif_av_connect_disconnect
**
** Description      Connect/Disconnect A2DP
**
** Returns          bt_status_t
**
*******************************************************************************/
#include "osi/include/properties.h"
bt_status_t btif_av_connect_disconnect()
{
    char prop_value_char[PROPERTY_VALUE_MAX];;
    bt_bdaddr_t addr;

    if (osi_property_get("bt.a2dp.conn", prop_value_char, ""))
    {
        if (strlen(prop_value_char) == 0)
        {
            // Do nothing
        }
        else if (strlen(prop_value_char) == 17)
        {
            if ( string_to_bdaddr(prop_value_char, &addr) )
            {
                BTIF_TRACE_DEBUG("%s, start A2DP connect : %s", __FUNCTION__, prop_value_char);
                osi_property_set("bt.a2dp.conn", "");
                src_connect_sink(&addr);
                return BT_STATUS_SUCCESS;
            }
            else
                goto conn_disconn_error;
        }
        else
        {
            goto conn_disconn_error;
        }
    }

    if (osi_property_get("bt.a2dp.disc", prop_value_char, ""))
    {
        if (strlen(prop_value_char) == 0)
        {
            // Do nothing
        }
        else if (strlen(prop_value_char) == 17)
        {
            if ( string_to_bdaddr(prop_value_char, &addr) )
            {
                BTIF_TRACE_DEBUG("%s, start A2DP disconnect : %s", __FUNCTION__, prop_value_char);
                osi_property_set("bt.a2dp.disc", "");
                disconnect(&addr);
                return BT_STATUS_SUCCESS;
            }
            else
                goto conn_disconn_error;
        }
        else
        {
            goto conn_disconn_error;
        }
    }
    return BT_STATUS_SUCCESS;

conn_disconn_error:
    BTIF_TRACE_DEBUG("%s, Usage (start A2DP connect) : setprop bt.a2dp.conn XX:XX:XX:XX:XX:XX", __FUNCTION__);
    BTIF_TRACE_DEBUG("%s, Usage (start A2DP disconnect) : setprop bt.a2dp.disc XX:XX:XX:XX:XX:XX", __FUNCTION__);
    osi_property_set("bt.a2dp.conn", "");
    osi_property_set("bt.a2dp.disc", "");
    return BT_STATUS_SUCCESS;
}
#endif

#if defined(MTK_A2DP_DUAL_AUDIO) && (MTK_A2DP_DUAL_AUDIO == TRUE)
static int btif_av_get_dev_idx_by_bda(const bt_bdaddr_t* bd_addr)
{
    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if(bdaddr_equals(&btif_av_cbs[xx].peer_bda, bd_addr)
            && !bdaddr_is_empty(bd_addr))
            return xx;
    }
    BTIF_AV_PRINT_BDA(bd_addr, "%s not found", __func__);
    return -1;
}

static int btif_av_get_dev_idx_by_handle(const UINT32 handle)
{
    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if((btif_av_cbs[xx].bta_handle == handle) && (0 != handle))
            return xx;
    }
    BTIF_TRACE_ERROR("%s handle 0x%x not found", __func__, handle);
    return -1;
}

static int btif_av_find_or_alloc_dev_idx(const bt_bdaddr_t* bd_addr)
{
    int xx = 0;
    int max_conn = btif_av_src_max_cnt;
    for(xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if(bdaddr_equals(&btif_av_cbs[xx].peer_bda, bd_addr)
            && !bdaddr_is_empty(bd_addr))
            return xx;
    }

    //if (max_conn > btif_av_sink_max_cnt)
    if (max_conn < btif_av_sink_max_cnt)
    {
        max_conn = btif_av_sink_max_cnt;
    }

    if (max_conn > BTA_AV_NUM_CONNS)
    {
        max_conn = BTA_AV_NUM_CONNS;
    }

    BTIF_TRACE_DEBUG("%s@%d maximum connection:%d, src:%d, sink:%d",
        __FUNCTION__, __LINE__, max_conn,
        btif_av_src_max_cnt, btif_av_sink_max_cnt);

    for(xx = 0; xx < max_conn; xx++)
    {
        BTIF_AV_PRINT_BDA(&btif_av_cbs[xx].peer_bda,
            "alloc btif_av_cbs[%d] in use:%d", xx, btif_av_cbs[xx].in_use);
        if (0 == btif_av_cbs[xx].in_use)
        {
            btif_av_cbs[xx].in_use = 1;
            bdaddr_copy(&btif_av_cbs[xx].peer_bda, bd_addr);
            return xx;
        }
    }

    BTIF_AV_PRINT_BDA(&btif_av_cbs[xx].peer_bda, "alloc fail");
    return -1;
}

static int btif_av_get_active_idx(void)
{
    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if(true == btif_av_cbs[xx].active)
            return xx;
    }
    return -1;
}

static int btif_av_assign_bta_handle(tBTA_AV_HNDL handle)
{
    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if(handle == btif_av_cbs[xx].bta_handle)
        {
            return xx;
        }
    }

    for(int xx = 0; xx < BTA_AV_NUM_CONNS; xx++)
    {
        if(0 == btif_av_cbs[xx].bta_handle)
        {
            btif_av_cbs[xx].bta_handle = handle;
            return xx;
        }
    }
    return -1;
}

#endif


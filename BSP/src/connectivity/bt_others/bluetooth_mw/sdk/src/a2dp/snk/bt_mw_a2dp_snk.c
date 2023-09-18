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

/* FILE NAME:  bt_mw_a2dp_snk.c
 * PURPOSE:
 *  {1. What is covered in this file - function and scope.}
 *  {2. Related documents or hardware information}
 * NOTES:
 *  {Something must be known or noticed}
 *  {1. How to use these functions - Give an example.}
 *  {2. Sequence of messages if applicable.}
 *  {3. Any design limitation}
 *  {4. Any performance limitation}
 *  {5. Is it a reusable component}
 *
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <string.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <bt_audio_track.h>
#include "bt_mw_a2dp_snk.h"
#include "c_mw_config.h"
#include "linuxbt_gap_if.h"
#include "bt_mw_message_queue.h"

#include <dlfcn.h>


/* NAMING CONSTANT DECLARATIONS
 */
#define BT_A2DP_PLAYBACK_RE_PLAY_TIMEOUT_MS (10000)

#define BT_A2DP_PLAYER_MAX                  (3)

#define BT_A2DP_SINK_DUMP_PCM_FILE BT_TMP_PATH"/sink_pcm_dump.pcm"

/* MACRO FUNCTION DECLARATIONS
 */

#define BT_MW_A2DP_SINK_LOCK() do{                           \
    if(g_bt_mw_a2dp_snk_cb.inited)                           \
        pthread_mutex_lock(&g_bt_mw_a2dp_snk_cb.lock);       \
    } while(0)

#define BT_MW_A2DP_SINK_UNLOCK() do{                         \
    if(g_bt_mw_a2dp_snk_cb.inited)                           \
        pthread_mutex_unlock(&g_bt_mw_a2dp_snk_cb.lock);     \
    } while(0)

#define BT_MW_A2DP_SINK_CHECK_INITED(ret)    do {                \
        if (FALSE == g_bt_mw_a2dp_snk_cb.inited)                 \
        {                                                        \
            BT_DBG_ERROR(BT_DEBUG_A2DP, "a2dp sink not init");   \
            return ret;                                          \
        }                                                        \
    }while(0)

typedef enum
{
    BT_MW_A2DP_SINK_PLAYER_STOPPED,
    BT_MW_A2DP_SINK_PLAYER_STOPPING,
    BT_MW_A2DP_SINK_PLAYER_STARTED,
    BT_MW_A2DP_SINK_PLAYER_STARTING,
    BT_MW_A2DP_SINK_PLAYER_PENDING, /* sample rate is not update, so pending it */
} BT_MW_A2DP_SINK_PLAYER_STATUS;
/* DATA TYPE DECLARATIONS
 */

typedef struct
{
    BOOL inited;

    INT32 ChannelCnt;
    INT32 SampleRate;
    INT32 BitDepth;

    BOOL ratio_set; /* true: bt/wifi ratio is set, false: default */

    BT_MW_A2DP_SINK_PLAYER_STATUS status;
    struct timespec stop_ts; /* player stop timestamp */

    BT_A2DP_PLAYER *cur_player;
    CHAR active_addr[MAX_BDADDR_LEN];
    pthread_mutex_t lock;
} BT_MW_A2DP_SNK_CB;

typedef struct
{
    VOID *dlhandle;
    BT_A2DP_PLAYER player;
} BT_MW_A2DP_SNK_PLAYER;

/* GLOBAL VARIABLE DECLARATIONS
 */
/* LOCAL SUBPROGRAM DECLARATIONS
 */
static VOID bt_mw_a2dp_sink_playback_start(VOID);
static void bt_mw_a2dp_sink_playback_init(int trackFreq, int channelType);
static void bt_mw_a2dp_sink_playback_deinit(void);
static void bt_mw_a2dp_sink_playback_play(void);
static void bt_mw_a2dp_sink_playback_pause(void);
static int bt_mw_a2dp_sink_playback_send_data(void *audioBuffer, int bufferLen);

static VOID bt_mw_a2dp_sink_report_player_event(BT_A2DP_PLAYER_EVENT event);

static INT32 bt_mw_a2dp_sink_player_init(VOID);

static INT32 bt_mw_a2dp_sink_player_deinit(VOID);

static VOID bt_mw_a2dp_sink_playback_dump(BT_A2DP_SINK_DUMP_FLAG flag);

static VOID bt_mw_a2dp_sink_playback_start(VOID);

static void bt_mw_a2dp_sink_playback_stop(void);

static CHAR* bt_mw_a2dp_sink_get_status_str(BT_MW_A2DP_SINK_PLAYER_STATUS status);

static CHAR* bt_mw_a2dp_sink_player_event_str(BT_A2DP_PLAYER_EVENT event);

/* STATIC VARIABLE DECLARATIONS
 */
static BT_MW_A2DP_SNK_CB g_bt_mw_a2dp_snk_cb = {0};

static BT_MW_A2DP_SNK_PLAYER g_bt_mw_a2dp_snk_players[BT_A2DP_PLAYER_MAX] = {0};

BtifAvrcpAudioTrack g_bt_mw_a2dp_sink_track =
{
    sizeof(BtifAvrcpAudioTrack),
    bt_mw_a2dp_sink_playback_init,
    bt_mw_a2dp_sink_playback_deinit,
    bt_mw_a2dp_sink_playback_play,
    bt_mw_a2dp_sink_playback_pause,
    bt_mw_a2dp_sink_playback_send_data,
};

FILE *s_bt_mw_a2dp_sink_pcm_fp = NULL;
char s_bt_mw_a2dp_sink_pcm_file[128];

pthread_mutex_t s_bt_mw_a2dp_sink_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t s_bt_mw_a2dp_sink_signal = PTHREAD_COND_INITIALIZER;


/* EXPORTED SUBPROGRAM BODIES
 */
INT32 bt_mw_a2dp_sink_register_player(VOID *dlhandle, BT_A2DP_PLAYER *player)
{
    /*sort sink player, the newest register player always in the first in the array*/
    INT32 free_pos = 0;

    BT_CHECK_POINTER(BT_DEBUG_A2DP, player);
    BT_CHECK_POINTER(BT_DEBUG_A2DP, dlhandle);
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "player %p(%s)", player, player->name);

    if (NULL == g_bt_mw_a2dp_snk_players[0].player.init)
    {
        memcpy(&g_bt_mw_a2dp_snk_players[0].player,
            player, sizeof(BT_A2DP_PLAYER));
        g_bt_mw_a2dp_snk_players[0].dlhandle = dlhandle;

        if ((NULL != player->init) && (TRUE == g_bt_mw_a2dp_snk_cb.inited))
        {
            player->init(bt_mw_a2dp_sink_report_player_event);
        }
        return BT_SUCCESS;
    }

    for (free_pos = 1; free_pos < BT_A2DP_PLAYER_MAX; free_pos ++)
    {
        if (NULL == g_bt_mw_a2dp_snk_players[free_pos].player.init)
        {
            memmove(&g_bt_mw_a2dp_snk_players[1],
                &g_bt_mw_a2dp_snk_players[0],
                sizeof(g_bt_mw_a2dp_snk_players[0])*free_pos);
            memcpy(&g_bt_mw_a2dp_snk_players[0].player,
                player, sizeof(BT_A2DP_PLAYER));
            g_bt_mw_a2dp_snk_players[0].dlhandle = dlhandle;

            if ((NULL != player->init) && (TRUE == g_bt_mw_a2dp_snk_cb.inited))
            {
                player->init(bt_mw_a2dp_sink_report_player_event);
            }
            return BT_SUCCESS;
        }
    }

    return BT_ERR_STATUS_NOMEM;
}

INT32 bt_mw_a2dp_sink_unregister_player(CHAR *player_name)
{
    INT32 idx = 0;

    BT_CHECK_POINTER(BT_DEBUG_A2DP, player_name);
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "player (%s)", player_name);

    for (idx = 0; idx < BT_A2DP_PLAYER_MAX; idx ++)
    {
        if (strncmp(g_bt_mw_a2dp_snk_players[idx].player.name, player_name,
            sizeof(g_bt_mw_a2dp_snk_players[idx].player.name)) == 0)
        {
            if ((NULL != g_bt_mw_a2dp_snk_players[idx].player.deinit)
                && (TRUE == g_bt_mw_a2dp_snk_cb.inited))
            {
                g_bt_mw_a2dp_snk_players[idx].player.deinit();
            }

            if (NULL != g_bt_mw_a2dp_snk_players[idx].dlhandle)
            {
                dlclose(g_bt_mw_a2dp_snk_players[idx].dlhandle);
            }
            memset(&g_bt_mw_a2dp_snk_players[idx],
                0, sizeof(g_bt_mw_a2dp_snk_players[idx]));
            BT_DBG_NORMAL(BT_DEBUG_A2DP, "unreg player (%s) success", player_name);
            return BT_SUCCESS;
        }
    }
    BT_DBG_WARNING(BT_DEBUG_A2DP, "no player (%s)", player_name);
    return BT_ERR_STATUS_FAIL;
}


/**
 * FUNCTION NAME: bt_mw_a2dp_sink_adjust_buffer_time
 * PURPOSE:
 *      The function is used for adjust buffer time when send data to playback
 * INPUT:
 *      buffer time
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */
INT32 bt_mw_a2dp_sink_adjust_buffer_time(UINT32 buffer_time)
{
    BT_MW_A2DP_SINK_CHECK_INITED(BT_ERR_STATUS_FAIL);
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "buffer_time=%u", buffer_time);
    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->adjust_buf_time)
    {
        g_bt_mw_a2dp_snk_cb.cur_player->adjust_buf_time(buffer_time);
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "callback is NULL");
    }
    return BT_SUCCESS;
}

VOID bt_mw_a2dp_sink_start_player(VOID)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    bt_mw_a2dp_sink_playback_start();
    bt_mw_a2dp_sink_playback_play();
    BT_MW_A2DP_SINK_UNLOCK();
    return;
}

VOID bt_mw_a2dp_sink_stop_player(VOID)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    bt_mw_a2dp_sink_playback_pause();
    bt_mw_a2dp_sink_playback_stop();
    BT_MW_A2DP_SINK_UNLOCK();
    return;
}

INT32 bt_mw_a2dp_sink_player_set(INT32 flag, UINT32 value)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED(BT_ERR_STATUS_FAIL);
#if 0
    BT_MW_A2DP_SINK_LOCK();
    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->set)
    {
        g_bt_mw_a2dp_snk_cb.cur_player->set(flag, value);
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "play callback is NULL");
    }
    BT_MW_A2DP_SINK_UNLOCK();
#endif
    return BT_SUCCESS;
}

INT32 bt_mw_a2dp_sink_player_load(CHAR* player_so_path)
{
    void *dlhandle = NULL;
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "player_so_path=%s", player_so_path);

    dlhandle = dlopen(player_so_path, RTLD_LAZY);
    if (!dlhandle)
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "%s open fail(%s)", player_so_path, dlerror());
        return BT_ERR_STATUS_FAIL;
    }
    BT_A2DP_PLAYER_MODULE *p_module = dlsym(dlhandle, "PLAYER_MODULE_INFO_SYM");
    if (p_module == NULL)
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "open fail, BT_A2DP_PLAYER_MODULE *p_module");
        dlclose(dlhandle);
        dlhandle = NULL;
        return BT_ERR_STATUS_FAIL;
    }
    if (dlerror() != NULL)
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "%s get symbol fail(%s)", player_so_path, dlerror());
        dlclose(dlhandle);
        dlhandle = NULL;
        return BT_ERR_STATUS_FAIL;
    }
    BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT sink player lib open success!");

    BT_MW_A2DP_SINK_LOCK();
    if (BT_SUCCESS != bt_mw_a2dp_sink_register_player(dlhandle, p_module->methods->get_player()))
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "%s get symbol fail(%s)", player_so_path, dlerror());
        dlclose(dlhandle);
        dlhandle = NULL;
        BT_MW_A2DP_SINK_UNLOCK();
        return BT_ERR_STATUS_NOMEM;
    }
    BT_MW_A2DP_SINK_UNLOCK();

    BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT sink player register success!");

    return BT_SUCCESS;
}

INT32 bt_mw_a2dp_sink_player_unload(CHAR *player_name)
{
    INT32 i4_ret = BT_SUCCESS;

    BT_MW_A2DP_SINK_LOCK();
    i4_ret = bt_mw_a2dp_sink_unregister_player(player_name);
    BT_MW_A2DP_SINK_UNLOCK();

    return i4_ret;
}


INT32 bt_mw_a2dp_sink_init(VOID)
{
    pthread_mutexattr_t attr;
    if (TRUE == g_bt_mw_a2dp_snk_cb.inited)
    {
        return BT_SUCCESS;
    }
    memset(&g_bt_mw_a2dp_snk_cb, 0, sizeof(g_bt_mw_a2dp_snk_cb));


    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&g_bt_mw_a2dp_snk_cb.lock, &attr);

    BT_MW_A2DP_SINK_LOCK();
    bt_mw_a2dp_sink_player_init();

    g_bt_mw_a2dp_snk_cb.inited = TRUE;
    BT_MW_A2DP_SINK_UNLOCK();

    return BT_SUCCESS;
}

INT32 bt_mw_a2dp_sink_deinit(VOID)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "inited=%d", g_bt_mw_a2dp_snk_cb.inited);
    BT_MW_A2DP_SINK_LOCK();
    if (FALSE == g_bt_mw_a2dp_snk_cb.inited)
    {
        BT_MW_A2DP_SINK_UNLOCK();
        return BT_SUCCESS;
    }
    bt_mw_a2dp_sink_stop_player();
    BT_MW_A2DP_SINK_UNLOCK();
    bt_mw_a2dp_sink_player_deinit();

    pthread_mutex_destroy(&g_bt_mw_a2dp_snk_cb.lock);

    memset(&g_bt_mw_a2dp_snk_cb, 0, sizeof(g_bt_mw_a2dp_snk_cb));

    return BT_SUCCESS;
}

INT32 bt_mw_a2dp_sink_select_player(CHAR *addr, BT_A2DP_CODEC_TYPE codec)
{
    INT32 idx = 0;
    BT_A2DP_PLAYER *player = NULL;

    BT_CHECK_POINTER(BT_DEBUG_A2DP, addr);
    BT_MW_A2DP_SINK_CHECK_INITED(BT_ERR_STATUS_FAIL);

    BT_MW_A2DP_SINK_LOCK();

    BT_DBG_INFO(BT_DEBUG_A2DP, "addr %s codec: 0x%x", addr, codec);

    strncpy(g_bt_mw_a2dp_snk_cb.active_addr, addr, MAX_BDADDR_LEN-1);
    g_bt_mw_a2dp_snk_cb.active_addr[MAX_BDADDR_LEN-1] = '\0';

    for (idx = 0;idx < BT_A2DP_PLAYER_MAX;idx ++)
    {
        if (NULL == g_bt_mw_a2dp_snk_players[idx].player.init)
        {
            BT_DBG_INFO(BT_DEBUG_A2DP, "g_bt_mw_a2dp_snk_players[%d] is NULL",
                          idx);
        }

        if (NULL == g_bt_mw_a2dp_snk_cb.cur_player)
        {
            BT_DBG_INFO(BT_DEBUG_A2DP, "cur_player is NULL");
        }

        if (NULL != g_bt_mw_a2dp_snk_players[idx].player.init)
        {
            player = &g_bt_mw_a2dp_snk_players[idx].player;
            BT_DBG_INFO(BT_DEBUG_A2DP, "support_codec_mask: 0x%x, "
                          "name: %s, codec: 0x%x",
                          player->support_codec_mask, player->name, codec);
            if (player->support_codec_mask & (1 << codec))
            {
                if (player == g_bt_mw_a2dp_snk_cb.cur_player)
                {
                    BT_DBG_NORMAL(BT_DEBUG_A2DP, "no change %s, codec=0x%x",
                        player->name, codec);
                    break;
                }

                if (NULL != g_bt_mw_a2dp_snk_cb.cur_player)
                {
                    if ((BT_MW_A2DP_SINK_PLAYER_STARTED == g_bt_mw_a2dp_snk_cb.status)
                        || (BT_MW_A2DP_SINK_PLAYER_STARTING == g_bt_mw_a2dp_snk_cb.status))
                    {
                        BT_DBG_NORMAL(BT_DEBUG_A2DP, "stop %s before change",
                            g_bt_mw_a2dp_snk_cb.cur_player->name);
                        bt_mw_a2dp_sink_playback_pause();
                        bt_mw_a2dp_sink_playback_stop();
                    }
                }
                BT_DBG_NORMAL(BT_DEBUG_A2DP, "change to %s",
                    g_bt_mw_a2dp_snk_players[idx].player.name);
                g_bt_mw_a2dp_snk_cb.cur_player = &g_bt_mw_a2dp_snk_players[idx].player;
                break;
            }
        }
    }
    BT_MW_A2DP_SINK_UNLOCK();

    return BT_SUCCESS;
}

VOID bt_mw_a2dp_sink_dump(BT_A2DP_SINK_DUMP_FLAG flag)
{
    bt_mw_a2dp_sink_playback_dump(flag);

    return;
}

VOID bt_mw_a2dp_sink_dump_pcm_start(char *dump_file_name)
{
    if (s_bt_mw_a2dp_sink_pcm_fp != NULL)
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "dump is started, skip it(%s)",
            dump_file_name==NULL?"NULL":dump_file_name);
        return;
    }

    if (dump_file_name == NULL)
    {
        strncpy(s_bt_mw_a2dp_sink_pcm_file, BT_A2DP_SINK_DUMP_PCM_FILE, 127);
    }
    else
    {
        if (strlen(dump_file_name) == 0)
        {
            strncpy(s_bt_mw_a2dp_sink_pcm_file, BT_A2DP_SINK_DUMP_PCM_FILE, 127);
        }
        else
        {
            strncpy(s_bt_mw_a2dp_sink_pcm_file, dump_file_name, 127);
        }
    }
#if 0
    BT_MW_A2DP_SINK_LOCK();
    strncat(s_bt_mw_a2dp_sink_pcm_file, g_bt_mw_a2dp_snk_cb.active_addr, 127);
    BT_MW_A2DP_SINK_UNLOCK();
#endif
    s_bt_mw_a2dp_sink_pcm_file[127] = 0;

    BT_DBG_NORMAL(BT_DEBUG_A2DP, "dump pcm file:%s", s_bt_mw_a2dp_sink_pcm_file);

    s_bt_mw_a2dp_sink_pcm_fp = fopen(s_bt_mw_a2dp_sink_pcm_file, "wb+");
    if (s_bt_mw_a2dp_sink_pcm_fp == NULL)
    {
        BT_DBG_ERROR(BT_DEBUG_A2DP, "open(%s) fail", s_bt_mw_a2dp_sink_pcm_file);
        return;
    }

    return;
}

VOID bt_mw_a2dp_sink_dump_pcm_stop(VOID)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    if (s_bt_mw_a2dp_sink_pcm_fp)
    {
        fclose(s_bt_mw_a2dp_sink_pcm_fp);
        s_bt_mw_a2dp_sink_pcm_fp = NULL;
    }
    return;
}


/* LOCAL SUBPROGRAM BODIES
 */


/**
 * FUNCTION NAME: bt_mw_a2dp_sink_playback_start
 * PURPOSE:
 *      The function is used for init playback and triggered by APP layer.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */

static VOID bt_mw_a2dp_sink_playback_start(VOID)
{
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    if (g_bt_mw_a2dp_snk_cb.cur_player == NULL)
    {
        BT_MW_A2DP_SINK_UNLOCK();
        return;
    }

    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "status=%s, player=%s",
        bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status),
        g_bt_mw_a2dp_snk_cb.cur_player->name);

    if ((NULL != g_bt_mw_a2dp_snk_cb.cur_player->start)
        && ((BT_MW_A2DP_SINK_PLAYER_STOPPED == g_bt_mw_a2dp_snk_cb.status)
            || (BT_MW_A2DP_SINK_PLAYER_PENDING == g_bt_mw_a2dp_snk_cb.status)
            || (BT_MW_A2DP_SINK_PLAYER_STOPPING == g_bt_mw_a2dp_snk_cb.status)))
    {
#if 0
        //bt_ratio: 04, wifi_ratio: 01, slot: (24*4+5):31
        if (!g_bt_mw_a2dp_snk_cb.ratio_set)
        {
            linuxbt_gap_set_bt_wifi_ratio(0x04, 0x01);
            BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT/Wifi: 4/1");
            g_bt_mw_a2dp_snk_cb.ratio_set = TRUE;
        }
#endif
        if (0 != g_bt_mw_a2dp_snk_cb.SampleRate)
        {
            g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_STARTING;
            g_bt_mw_a2dp_snk_cb.cur_player->start(g_bt_mw_a2dp_snk_cb.SampleRate,
                g_bt_mw_a2dp_snk_cb.ChannelCnt,
                g_bt_mw_a2dp_snk_cb.BitDepth);
        }
        else
        {
            BT_DBG_WARNING(BT_DEBUG_A2DP, "sample rate is 0, device not acitve");
            g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_PENDING;
        }
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "start_cb=%p or not stopped(%d)",
            g_bt_mw_a2dp_snk_cb.cur_player->start, g_bt_mw_a2dp_snk_cb.status);
    }
    BT_MW_A2DP_SINK_UNLOCK();
}


/**
 * FUNCTION NAME: bt_mw_a2dp_sink_playback_init
 * PURPOSE:
 *      The function is used for init playback and triggered by stack audio track.
 * INPUT:
 *      trackFreq                -- samplerate
 *      channelType            --channel number, MSB16: bitdepth, LSB16: channeltype
 * OUTPUT:
 *      None
 * RETURN:
 *      None
 * NOTES:
 *      None
 */

static void bt_mw_a2dp_sink_playback_init(int trackFreq, int channelType)
{
    int bitDepth = channelType >> 16;
    channelType &= 0xFFFF;
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "fs:%d channel:%d bitDepth:%d",
        trackFreq, channelType, bitDepth);
    BT_MW_A2DP_SINK_CHECK_INITED();

    BT_MW_A2DP_SINK_LOCK();

    /* init it again, when a device wants to open audio track, we should update
     * it. In case phone connect to DUT and app don't start player, it will
     * trigger data_come right now, it's not good.
     */
    clock_gettime(CLOCK_MONOTONIC, &g_bt_mw_a2dp_snk_cb.stop_ts);

    if ((trackFreq == g_bt_mw_a2dp_snk_cb.SampleRate) &&
        (channelType == g_bt_mw_a2dp_snk_cb.ChannelCnt) &&
        (bitDepth == g_bt_mw_a2dp_snk_cb.BitDepth))
    {
        BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "no change, status=%s, ratio_set=%d",
            bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status),
            g_bt_mw_a2dp_snk_cb.ratio_set);
#if DISPATCH_A2DP_WITH_PLAYBACK
        if ((BT_MW_A2DP_SINK_PLAYER_STARTED == g_bt_mw_a2dp_snk_cb.status)
            || (BT_MW_A2DP_SINK_PLAYER_STARTING == g_bt_mw_a2dp_snk_cb.status))
        {
            if (!g_bt_mw_a2dp_snk_cb.ratio_set)
            {
                linuxbt_gap_set_bt_wifi_ratio(0x04, 0x01);
                BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT/Wifi: 4/1");
                g_bt_mw_a2dp_snk_cb.ratio_set = TRUE;
            }
        }
#endif
        BT_MW_A2DP_SINK_UNLOCK();
        return;
    }
    g_bt_mw_a2dp_snk_cb.SampleRate = trackFreq;
    g_bt_mw_a2dp_snk_cb.ChannelCnt = channelType;
    g_bt_mw_a2dp_snk_cb.BitDepth = bitDepth;

#if DISPATCH_A2DP_WITH_PLAYBACK
    if (BT_MW_A2DP_SINK_PLAYER_PENDING == g_bt_mw_a2dp_snk_cb.status)
    {
        bt_mw_a2dp_sink_playback_start();
        bt_mw_a2dp_sink_playback_play();
    }
    else if ((BT_MW_A2DP_SINK_PLAYER_STARTED == g_bt_mw_a2dp_snk_cb.status)
        || (BT_MW_A2DP_SINK_PLAYER_STARTING == g_bt_mw_a2dp_snk_cb.status))
    {
        /* reset player */
        bt_mw_a2dp_sink_playback_pause();
        bt_mw_a2dp_sink_playback_stop();

        bt_mw_a2dp_sink_playback_start();
        bt_mw_a2dp_sink_playback_play();

    }
    else
    {
        BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT playback init should trigger by APP");
    }
#else
    if (g_bt_mw_a2dp_snk_cb.cur_player == NULL)
    {
        BT_MW_A2DP_SINK_UNLOCK();
        return;
    }
    if (g_bt_mw_a2dp_snk_cb.cur_player->start &&
        (BT_MW_A2DP_SINK_PLAYER_PENDING == g_bt_mw_a2dp_snk_cb.status))
    {
#if 0
        //bt_ratio: 04, wifi_ratio: 01, slot: (24*4+5):31
        linuxbt_gap_set_bt_wifi_ratio(0x04, 0x01);
        BT_DBG_ERROR(BT_DEBUG_A2DP, "BT/Wifi: 4/1");
#endif
        g_bt_mw_a2dp_snk_cb.cur_player->player.start(trackFreq, channelType, bitDepth);
        g_bt_mw_a2dp_snk_cb.cur_player->status = BT_MW_A2DP_SINK_PLAYER_STARTING;
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "start callback is NULL");
    }
#endif
    BT_MW_A2DP_SINK_UNLOCK();
}

static void bt_mw_a2dp_sink_playback_stop(void)
{
    BT_A2DP_PLAYER *cur_player = NULL;
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "status=%s",
        bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status));
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    cur_player = g_bt_mw_a2dp_snk_cb.cur_player;

    if (cur_player == NULL)
    {
        BT_MW_A2DP_SINK_UNLOCK();
        return;
    }

    if (cur_player->stop)
    {
        if ((BT_MW_A2DP_SINK_PLAYER_STARTING == g_bt_mw_a2dp_snk_cb.status)
            || (BT_MW_A2DP_SINK_PLAYER_STARTED == g_bt_mw_a2dp_snk_cb.status))
        {
#if 0
            //bt_ratio: 01, wifi_ratio: 02, slot: (24/2+5):31
            if (g_bt_mw_a2dp_snk_cb.ratio_set)
            {
                linuxbt_gap_set_bt_wifi_ratio(0x01, 0x02);
                BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT/Wifi: 1/2");
                g_bt_mw_a2dp_snk_cb.ratio_set = FALSE;
            }
#endif
            g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_STOPPING;
            cur_player->stop();

            //g_bt_mw_a2dp_snk_cb.SampleRate = 0;
            //g_bt_mw_a2dp_snk_cb.ChannelCnt = 0;

            clock_gettime(CLOCK_MONOTONIC, &g_bt_mw_a2dp_snk_cb.stop_ts);
            BT_DBG_WARNING(BT_DEBUG_A2DP, "playback deinit@%lu.%09lu",
                g_bt_mw_a2dp_snk_cb.stop_ts.tv_sec,
                g_bt_mw_a2dp_snk_cb.stop_ts.tv_nsec/1000);
        }
        else if (BT_MW_A2DP_SINK_PLAYER_PENDING == g_bt_mw_a2dp_snk_cb.status)
        {
            g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_STOPPED;
            BT_DBG_WARNING(BT_DEBUG_A2DP, "stopped directly");
        }
    }
    else
    {
        BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "stop_cb is NULL");
    }
    BT_MW_A2DP_SINK_UNLOCK();
}

void bt_mw_a2dp_sink_playback_deinit(void)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "status=%s, ratio_set=%d",
        bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status),
        g_bt_mw_a2dp_snk_cb.ratio_set);
    BT_MW_A2DP_SINK_LOCK();
#if 0
    //bt_ratio: 01, wifi_ratio: 02, slot: (24/2+5):31
    /* if audio track deinit(a2dp disconnected), restore the ratio */
    if (g_bt_mw_a2dp_snk_cb.ratio_set)
    {
        linuxbt_gap_set_bt_wifi_ratio(0x01, 0x02);
        BT_DBG_NORMAL(BT_DEBUG_A2DP, "BT/Wifi: 1/2");
        g_bt_mw_a2dp_snk_cb.ratio_set = FALSE;
    }
#endif
    g_bt_mw_a2dp_snk_cb.SampleRate = 0;
    g_bt_mw_a2dp_snk_cb.ChannelCnt = 0;
    BT_MW_A2DP_SINK_UNLOCK();
}


static void bt_mw_a2dp_sink_playback_play(void)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->play)
    {
        g_bt_mw_a2dp_snk_cb.cur_player->play();
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "play callback is NULL");
    }
    BT_MW_A2DP_SINK_UNLOCK();
}

static void bt_mw_a2dp_sink_playback_pause(void)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->pause)
    {
        g_bt_mw_a2dp_snk_cb.cur_player->pause();
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP,
            "pause callback is NULL, cur_player=%p",
            g_bt_mw_a2dp_snk_cb.cur_player);
    }
    BT_MW_A2DP_SINK_UNLOCK();
}

static int bt_mw_a2dp_sink_playback_send_data(void *audioBuffer, int bufferLen)
{
    BT_DBG_MINOR(BT_DEBUG_A2DP, "status=%s",
        bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status));
    BT_MW_A2DP_SINK_CHECK_INITED(BT_ERR_STATUS_FAIL);

    if (s_bt_mw_a2dp_sink_pcm_fp)
    {
        fwrite ((audioBuffer), 1, (size_t)bufferLen, s_bt_mw_a2dp_sink_pcm_fp);
    }

    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->write)
    {
        if (BT_MW_A2DP_SINK_PLAYER_STARTED == g_bt_mw_a2dp_snk_cb.status)
        {

            g_bt_mw_a2dp_snk_cb.cur_player->write(audioBuffer, bufferLen);
        }
        else if(BT_MW_A2DP_SINK_PLAYER_STOPPED == g_bt_mw_a2dp_snk_cb.status)
        {
            struct timespec tv;
            clock_gettime(CLOCK_MONOTONIC, &tv);
            if (((tv.tv_sec - g_bt_mw_a2dp_snk_cb.stop_ts.tv_sec) * 1000
                + ((tv.tv_nsec - g_bt_mw_a2dp_snk_cb.stop_ts.tv_nsec) / 1000000))
                > BT_A2DP_PLAYBACK_RE_PLAY_TIMEOUT_MS)
            {
                /* update this time or it will trigger many times */
                clock_gettime(CLOCK_MONOTONIC, &g_bt_mw_a2dp_snk_cb.stop_ts);
                BT_DBG_WARNING(BT_DEBUG_A2DP,
                    "playback deinit, still rx data@%lu.%06lu, trigger playing",
                    tv.tv_sec, tv.tv_nsec);
                bt_mw_a2dp_sink_report_player_event(BT_A2DP_ALSA_PB_EVENT_DATA_COME);
            }

        }
        else
        {
            BT_DBG_INFO(BT_DEBUG_A2DP, "status=%s drop it, len=%d",
                bt_mw_a2dp_sink_get_status_str(g_bt_mw_a2dp_snk_cb.status), bufferLen);
#if 0
            if (g_bt_mw_a2dp_snk_cb.ChannelCnt != 0
                && g_bt_mw_a2dp_snk_cb.SampleRate != 0
                && BT_MW_A2DP_SINK_PLAYER_STARTING == g_bt_mw_a2dp_snk_cb.status)
            {
                struct timespec outTime;
                int us_delay = 0;

                if (g_bt_mw_a2dp_snk_cb.BitDepth == 24)
                {
                    us_delay = (int)(((int64_t)bufferLen * (1000000 /
                        (g_bt_mw_a2dp_snk_cb.ChannelCnt * 4))) /
                        g_bt_mw_a2dp_snk_cb.SampleRate);
                }
                else
                {
                    us_delay = (int)(((int64_t)bufferLen * (1000000 /
                        (g_bt_mw_a2dp_snk_cb.ChannelCnt * 2))) /
                        g_bt_mw_a2dp_snk_cb.SampleRate);
                }
                BT_DBG_WARNING(BT_DEBUG_A2DP, "emulate play, wait %d us", us_delay);
                //usleep(us_delay);
                clock_gettime(CLOCK_MONOTONIC, &outTime);
                outTime.tv_nsec += us_delay*1000;
                if (outTime.tv_nsec > 1000000000) {
                    outTime.tv_nsec -= 1000000000;
                    outTime.tv_sec += 1;
                }
                pthread_mutex_lock(&s_bt_mw_a2dp_sink_lock);
                pthread_cond_timedwait(&s_bt_mw_a2dp_sink_signal,
                    &s_bt_mw_a2dp_sink_lock, &outTime);
                pthread_mutex_unlock(&s_bt_mw_a2dp_sink_lock);
            }
#endif
        }
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "write callback is NULL");
        return BT_ERR_STATUS_FAIL;
    }
    return BT_SUCCESS;
}

static VOID bt_mw_a2dp_sink_report_player_event(BT_A2DP_PLAYER_EVENT event)
{
    tBTMW_MSG btmw_msg = {0};
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "event=%d(%s) active_addr=%s",
        event, bt_mw_a2dp_sink_player_event_str(event),
        g_bt_mw_a2dp_snk_cb.active_addr);
    BT_MW_A2DP_SINK_CHECK_INITED();
    memset(&btmw_msg, 0, sizeof(btmw_msg));
    BT_MW_A2DP_SINK_LOCK();

    if ((BT_A2DP_ALSA_PB_EVENT_START == event)
        && (BT_MW_A2DP_SINK_PLAYER_STARTING ==
            g_bt_mw_a2dp_snk_cb.status))
    {
        g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_STARTED;
    }
    else if ((BT_A2DP_ALSA_PB_EVENT_STOP == event)
        && (BT_MW_A2DP_SINK_PLAYER_STOPPING ==
            g_bt_mw_a2dp_snk_cb.status))
    {
        g_bt_mw_a2dp_snk_cb.status = BT_MW_A2DP_SINK_PLAYER_STOPPED;
    }

    strncpy(btmw_msg.data.a2dp_msg.addr, g_bt_mw_a2dp_snk_cb.active_addr, MAX_BDADDR_LEN - 1);

    BT_MW_A2DP_SINK_UNLOCK();

    btmw_msg.hdr.event = BTMW_A2DP_PLAYER_REPORT_EVENT;
    btmw_msg.data.a2dp_msg.data.player_event = event;

    btmw_msg.hdr.len = sizeof(btmw_msg.data.a2dp_msg);
    linuxbt_send_msg(&btmw_msg);
}

static INT32 bt_mw_a2dp_sink_player_init(VOID)
{
    INT32 idx = 0;
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");

    for (idx = 0;idx < BT_A2DP_PLAYER_MAX;idx ++)
    {
        if (NULL != g_bt_mw_a2dp_snk_players[idx].player.init)
        {
            if (NULL != g_bt_mw_a2dp_snk_players[idx].player.init)
            {
                g_bt_mw_a2dp_snk_players[idx].player.init(bt_mw_a2dp_sink_report_player_event);
            }

            if (NULL == g_bt_mw_a2dp_snk_cb.cur_player)
            {
                /* select first one as the default player */
                g_bt_mw_a2dp_snk_cb.cur_player =
                    &g_bt_mw_a2dp_snk_players[idx].player;
                BT_DBG_NORMAL(BT_DEBUG_A2DP, "select player[%d] %s",
                    idx, g_bt_mw_a2dp_snk_cb.cur_player->name);
            }
        }
    }
    return BtifAvrcpAudioTrackInit(&g_bt_mw_a2dp_sink_track);
}

static INT32 bt_mw_a2dp_sink_player_deinit(VOID)
{
    INT32 idx = 0;
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED(BT_ERR_STATUS_FAIL);

    for (idx = 0;idx < BT_A2DP_PLAYER_MAX;idx ++)
    {
        if (NULL != g_bt_mw_a2dp_snk_players[idx].player.deinit)
        {
            g_bt_mw_a2dp_snk_players[idx].player.deinit();
        }
    }

    g_bt_mw_a2dp_snk_cb.cur_player = NULL;

    return BtifAvrcpAudioTrackDeinit();
}

static VOID bt_mw_a2dp_sink_playback_dump(BT_A2DP_SINK_DUMP_FLAG flag)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_A2DP, "");
    BT_MW_A2DP_SINK_CHECK_INITED();
    BT_MW_A2DP_SINK_LOCK();
    if (g_bt_mw_a2dp_snk_cb.cur_player &&
        g_bt_mw_a2dp_snk_cb.cur_player->dump)
    {
        g_bt_mw_a2dp_snk_cb.cur_player->dump(flag);
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_A2DP, "dump callback is NULL");
    }
    BT_MW_A2DP_SINK_UNLOCK();
}


static CHAR* bt_mw_a2dp_sink_get_status_str(BT_MW_A2DP_SINK_PLAYER_STATUS status)
{
     switch(status)
     {
         BT_MW_A2DP_CASE_RETURN_STR(BT_MW_A2DP_SINK_PLAYER_STOPPED, "stopped");
         BT_MW_A2DP_CASE_RETURN_STR(BT_MW_A2DP_SINK_PLAYER_STOPPING, "stopping");
         BT_MW_A2DP_CASE_RETURN_STR(BT_MW_A2DP_SINK_PLAYER_STARTED, "started");
         BT_MW_A2DP_CASE_RETURN_STR(BT_MW_A2DP_SINK_PLAYER_STARTING, "starting");
         BT_MW_A2DP_CASE_RETURN_STR(BT_MW_A2DP_SINK_PLAYER_PENDING, "start pending");

         default: return "unknown";
    }
}

static CHAR* bt_mw_a2dp_sink_player_event_str(BT_A2DP_PLAYER_EVENT event)
{
     switch(event)
     {
         BT_MW_A2DP_CASE_RETURN_STR(BT_A2DP_ALSA_PB_EVENT_STOP, "stopped");
         BT_MW_A2DP_CASE_RETURN_STR(BT_A2DP_ALSA_PB_EVENT_STOP_FAIL, "stop fail");
         BT_MW_A2DP_CASE_RETURN_STR(BT_A2DP_ALSA_PB_EVENT_START, "started");
         BT_MW_A2DP_CASE_RETURN_STR(BT_A2DP_ALSA_PB_EVENT_START_FAIL, "start fail");
         BT_MW_A2DP_CASE_RETURN_STR(BT_A2DP_ALSA_PB_EVENT_DATA_COME, "data come");

         default: return "unknown";
    }
}


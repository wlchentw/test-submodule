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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>

#include "osi/include/config.h"
#include "osi/include/log.h"
//#include "c_mw_config.h"
//#include "bt_mw_common.h"
#include "bt_mw_log.h"
//#include "bluetooth_sync.h"

typedef UINT8 tBTTRC_MW_LAYER_ID;

typedef UINT8 (tBTTRC_SET_MW_TRACE_LEVEL)(UINT8);

typedef struct {
    const tBTTRC_MW_LAYER_ID         layer_id_start;
    const tBTTRC_MW_LAYER_ID         layer_id_end;
    tBTTRC_SET_MW_TRACE_LEVEL        *p_f;
    const char                       *trc_name;
    UINT8                            trace_level;
}tBTTRC_MW_FUNC_MAP;


#ifndef MW_DEFAULT_CONF_TRACE_LEVEL
#define MW_DEFAULT_CONF_TRACE_LEVEL BT_MW_TRACE_LEVEL_WARNING
#endif

#if defined(MTK_LINUX_MW_STACK_LOG2FILE) && (MTK_LINUX_MW_STACK_LOG2FILE == TRUE)
const char *BT_LOG_PATH_KEY = "BtStackFileName";
const char *BT_LOG2FILE_TURNED_ON_KEY = "BtStackLog2File";
const char *BT_LOG2FILE_ONLY_ON_KEY = "BtStackLogOnly2File";
const char *BT_SHOULD_SAVE_LAST_KEY = "BtStackSaveLog";
const char *BT_LOG_LEVEL_KEY = "BtStackLogLevel";
const char *BT_A2DP_LOG_LEVEL_KEY = "BtStackA2dpLogLevel";
const char *BT_PICUS_LOG_PARAM_KEY = "BtPicusParam";
const char *BT_LOG_MODE_KEY = "BtStackLogSlice";
const char *BT_MAX_LOG_FILE_SIZE_KEY = "BtStackLogFileSize";
const char *BT_MAX_FILE_NUM_KEY = "BtStackLogMaxFileNum";
const char *BT_LOG_BUF_SIZE_KEY = "BtStackLogBufSize";
const char *BT_LOG_TIMEOUT_KEY = "BtStackLogTimeout";
#endif
const char *MW_TRACE_CONFIG_ENABLED_KEY = "MWTraceConf";

static config_t *bt_mw_config;

#if defined(MTK_LINUX_STACK_TRAC_CONFIG) && (MTK_LINUX_STACK_TRAC_CONFIG == TRUE)
extern void set_trace_log_level(const char *trc_name, UINT8 trace_level);
#endif
//extern void save_log2file_init(STACK_LOG_SETTING *log_setting);

extern UINT8 audio_set_trace_level(UINT8 new_level);

EXPORT_SYMBOL BT_MW_TRC_MAP btmw_trc_map[] =
{
    {BT_DBG_COMM, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_GAP, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_A2DP, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_AVRCP, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_HID, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_SPP, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_GATT, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_HFP, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_PB, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_UPL, BT_MW_TRACE_LEVEL_API},
    {BT_DBG_MESH, BT_MW_TRACE_LEVEL_API}
};

#if 0
static bt_error_str_t errorTbl[] =
{
    {BT_ERR_SUCCESS, "Success"},
    {BT_ERR_NULL_POINTER, "Null Pointer"},
    {BT_ERR_OUT_OF_RANGE, "Out of Range"},
    {BT_ERR_UNKNOWN_CMD, "Unknown Command"},
    {BT_ERR_INVALID_PARAM, "Invalid input paramter"},
    {BT_ERR_INVALID_PARAM_NUMS, "Invalid input paramter number"},
    {BT_ERR_NOT_SUPPORT, "Not supported"},
    {BT_ERR_FAILED, "Failed"},
};
#endif

static UINT8 BTMW_Set_Comm_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_COMM].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_COMM].trace_level;
}

static UINT8 BTMW_Set_Gap_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_GAP].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_GAP].trace_level;
}

static UINT8 BTMW_Set_A2dp_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_A2DP].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_A2DP].trace_level;
}

static UINT8 BTMW_Set_Avrcp_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_AVRCP].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_AVRCP].trace_level;
}

static UINT8 BTMW_Set_Hid_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_HID].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_SPP].trace_level;
}

static UINT8 BTMW_Set_Spp_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_SPP].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_SPP].trace_level;
}

static UINT8 BTMW_Set_Gatt_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_GATT].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_SPP].trace_level;
}

static UINT8 BTMW_Set_Hfp_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_HFP].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_HFP].trace_level;
}

static UINT8 BTMW_Set_Pb_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_PB].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_PB].trace_level;
}

static UINT8 BTMW_Set_Upl_TraceLevel(UINT8 new_level)
{
    if (new_level != 0xFF)
    {
        btmw_trc_map[BT_DEBUG_UPL].trace_level = new_level;
    }

    return btmw_trc_map[BT_DEBUG_UPL].trace_level;
}


/* make sure list is order by increasing layer id!!! */
static tBTTRC_MW_FUNC_MAP btmw_trc_set_level_map[] =
{
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Comm_TraceLevel, "TRC_MW_COMM", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Gap_TraceLevel, "TRC_MW_GAP", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_A2dp_TraceLevel, "TRC_MW_A2DP", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Avrcp_TraceLevel, "TRC_MW_AVRCP", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Hid_TraceLevel, "TRC_MW_HID", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Spp_TraceLevel, "TRC_MW_SPP", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Gatt_TraceLevel, "TRC_MW_GATT", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Hfp_TraceLevel, "TRC_MW_HFP", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Pb_TraceLevel, "TRC_MW_PB", MW_DEFAULT_CONF_TRACE_LEVEL},
    {BT_MW_TRC_ID_START, BT_MW_TRC_ID_END, BTMW_Set_Upl_TraceLevel, "TRC_MW_UPL", MW_DEFAULT_CONF_TRACE_LEVEL},

    {0, 0, NULL, NULL, MW_DEFAULT_CONF_TRACE_LEVEL}
};

static int s_bt_mw_dump_info_fd = -1;

static void bt_mw_load_levels_from_config(const config_t *config)
{
    tBTTRC_MW_FUNC_MAP *functions;
    assert(config != NULL);

    for (functions = &btmw_trc_set_level_map[0]; functions->trc_name; ++functions)
    {
        printf("BT_MW_InitTraceLevels -- %s\n", functions->trc_name);
        int value = config_get_int(config, CONFIG_DEFAULT_SECTION, functions->trc_name, -1);
        if (value != -1)
            functions->trace_level = value;

        if (functions->p_f)
            functions->p_f(functions->trace_level);
  }
}

static bool bt_mw_get_trace_config_enabled(void)
{
    return config_get_bool(bt_mw_config, CONFIG_DEFAULT_SECTION, MW_TRACE_CONFIG_ENABLED_KEY, false);
}

#if defined(MTK_LINUX_MW_STACK_LOG2FILE) && (MTK_LINUX_MW_STACK_LOG2FILE == TRUE)
static const char *get_btmw_log_path(void)
{
  return config_get_string(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG_PATH_KEY, "/data/misc/bluetooth/logs/bt_stack.log");
}

static bool get_btmw_log2file_turned_on(void)
{
  return config_get_bool(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG2FILE_TURNED_ON_KEY, false);
}

static bool get_btmw_log2file_only_on(void)
{
  return config_get_bool(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG2FILE_ONLY_ON_KEY, true);
}

static bool get_btmw_should_save_last(void)
{
  return config_get_bool(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_SHOULD_SAVE_LAST_KEY, false);
}

static int get_btmw_log_level(void)
{
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG_LEVEL_KEY, 0xF);
}

static int get_btmw_a2dp_log_level(void)
{
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_A2DP_LOG_LEVEL_KEY, 1);
}

static bool get_btmw_log_mode(void) {
  return config_get_bool(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG_MODE_KEY, false);
}

static int get_btmw_max_log_file_size(void) {
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_MAX_LOG_FILE_SIZE_KEY, 0);
}

static int get_btmw_max_file_num(void) {
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_MAX_FILE_NUM_KEY, 0);
}

static int get_btmw_log_buf_size(void) {
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG_BUF_SIZE_KEY, 0);
}

static int get_btmw_log_timeout(void) {
  return config_get_int(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_LOG_TIMEOUT_KEY, 0);
}

extern int ipcd_exec(char *cmd, char *priv);
static const char *get_btmw_picus_log_param(void)
{
  if (bt_mw_config == NULL) return NULL;
  return config_get_string(bt_mw_config, CONFIG_DEFAULT_SECTION, BT_PICUS_LOG_PARAM_KEY, "");
}

void mw_log_start_picus(void)
{
    const char *picus_param = NULL;

#ifdef ENABLE_IPCD
    ipcd_exec("killall picus", NULL);
#endif

    picus_param = get_btmw_picus_log_param();

    if (picus_param == NULL) return;

    BT_DBG_NORMAL(BT_DEBUG_COMM, "picus param:(%s)", picus_param);
    if (strlen(picus_param) > 0)
    {
        CHAR picus_cmd[255] = "picus ";
        /* make sure picus_param and "&"can be set to picus_cmd!!! */
        if (strlen(picus_param) < (255 - strlen("picus ") - strlen("&")))
        {
            strncat(picus_cmd, picus_param, 255);
            strncat(picus_cmd, "&", 255);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_COMM, "picus param over length:%d", strlen(picus_param));
            return;
        }
#ifdef ENABLE_IPCD
        ipcd_exec(picus_cmd, NULL);
#endif
    }
}

void mw_log_stop_picus(void)
{
    BT_MW_FUNC_ENTER(BT_DEBUG_COMM, "");
#ifdef ENABLE_IPCD
    ipcd_exec("killall picus", NULL);
#endif
}


#endif

void mw_log_init(void)
{
    STACK_LOG_SETTING log_setting;
    const char *path = BLUETOOTH_STACK_CONFIG_FOLDER"/bt_stack.conf";
    int a2dp_log_level = 0;

    /* In case of configuration files in /data/misc/bluedroid/ are deleted,
    * configuration files in /etc/bluetooth/ can be alternates
    */
    if((path == NULL) || access(path, R_OK))
    {
        if(!access("/etc/bluetooth/bt_stack.conf", R_OK))
        {
            path = "/etc/bluetooth/bt_stack.conf";
        }
    }

    assert(path != NULL);

    BT_DBG_WARNING(BT_DEBUG_COMM, "%s attempt to load mw conf from %s", __func__, path);

    bt_mw_config = config_new(path);
    if (!bt_mw_config)
    {
        BT_DBG_NORMAL(BT_DEBUG_COMM, "%s file >%s< not found", __func__, path);
        return;
    }

    config_dump(bt_mw_config); /* dump config  */

    if (bt_mw_get_trace_config_enabled())
    {
        bt_mw_load_levels_from_config(bt_mw_config);
    }
    else
    {
        BT_DBG_WARNING(BT_DEBUG_COMM, "using compile default trace settings");
    }

#if defined(MTK_LINUX_MW_STACK_LOG2FILE) && (MTK_LINUX_MW_STACK_LOG2FILE == TRUE)
    memset((void*)&log_setting, 0, sizeof(log_setting));
    log_setting.log2file = get_btmw_log2file_turned_on();
    log_setting.only2file = get_btmw_log2file_only_on();
    log_setting.save_last = get_btmw_should_save_last();
    log_setting.log_path = get_btmw_log_path();
    log_setting.log_level = get_btmw_log_level();
    log_setting.log_slice = get_btmw_log_mode();
    log_setting.max_file_num = get_btmw_max_file_num();
    log_setting.log_size = get_btmw_max_log_file_size();
    log_setting.buf_size = get_btmw_log_buf_size();
    log_setting.timeout = get_btmw_log_timeout();

    save_log2file_init(&log_setting);

    a2dp_log_level = get_btmw_a2dp_log_level();
    audio_set_trace_level(a2dp_log_level);
#endif
}

EXPORT_SYMBOL void bt_mw_log_setStackLevel(const char *trc_name, int level)
{
    tBTTRC_MW_FUNC_MAP *functions;
    for (functions = &btmw_trc_set_level_map[0]; functions->trc_name; ++functions)
    {
        if (0 == strcmp(functions->trc_name, trc_name))
        {
            BT_DBG_NORMAL(BT_DEBUG_COMM, "set mw log level -- %s-%d",
                trc_name, level);
            if (level != -1)
                functions->trace_level = level;

            if (functions->p_f)
                functions->p_f(functions->trace_level);

            return;
        }
    }

#if defined(MTK_LINUX_STACK_TRAC_CONFIG) && (MTK_LINUX_STACK_TRAC_CONFIG == TRUE)
    set_trace_log_level(trc_name, level);
#else
    BT_DBG_WARNING(BT_DEBUG_COMM, "cannot set stack trace settings");
#endif
}

INT32 bt_get_dbg_level(BT_DEBUG_LAYER_NAME_T layer)
{
    INT32 i = 0;
    if (BT_DEBUG_MAX <= layer)
    {
        for (i = 0; i < BT_DEBUG_MAX; i++)
        {
            BT_DBG_ERROR(BT_DEBUG_COMM, "layer:%d level:%d", i, btmw_trc_map[i].trace_level);
        }
        return -1;
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_COMM, "layer:%d level:%d", layer, btmw_trc_map[layer].trace_level);
        return btmw_trc_map[layer].trace_level;
    }
}

VOID bt_set_dbg_level(BT_DEBUG_LAYER_NAME_T layer, INT32 level)
{
    INT32 i = 0;
    if (BT_DEBUG_MAX <= layer)
    {
        for (i = 0; i < BT_DEBUG_MAX; i++)
        {
            btmw_trc_map[i].trace_level = level;
        }
    }
    else
    {
        btmw_trc_map[layer].trace_level = level;
    }
    BT_DBG_WARNING(BT_DEBUG_COMM, "layer:%d level:%d", layer, level);
}


EXPORT_SYMBOL VOID bt_mw_dump_info_begin(CHAR *dump_file_name)
{
    char log_path[64] = "/tmp/bt_dump/";

    if (access(log_path, F_OK) != 0)
    {
        if (mkdir(log_path, 0770) == -1)
        {
            printf("mkdir %s error! %s, use /tmp/\n", log_path, (char*)strerror(errno));
            strncpy(log_path, "/tmp/", strlen(log_path));
        }
    }

    strncat(log_path, dump_file_name, 63);
    if (-1 == s_bt_mw_dump_info_fd)
    {
        s_bt_mw_dump_info_fd = open(log_path, O_WRONLY | O_CREAT | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
        if (-1 == s_bt_mw_dump_info_fd)
        {
            printf("%s unable to open '%s': %s\n", __func__, log_path, strerror(errno));
        }
    }
}

EXPORT_SYMBOL VOID bt_mw_dump_info_write(const char *format, ...)
{
    va_list args;
    char msg[500] = {0};

    va_start(args, format);
    vsnprintf(msg, sizeof(msg), format, args);
    va_end(args);
    if (s_bt_mw_dump_info_fd != -1)
    {
        write(s_bt_mw_dump_info_fd, msg, strlen(msg));
    }
    return;
}

EXPORT_SYMBOL VOID bt_mw_dump_info_end(VOID)
{
    if (s_bt_mw_dump_info_fd != -1)
    {
        close(s_bt_mw_dump_info_fd);
        s_bt_mw_dump_info_fd = -1;
    }
}


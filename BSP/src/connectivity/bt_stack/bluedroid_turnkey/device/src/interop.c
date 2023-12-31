/******************************************************************************
 *
 *  Copyright (C) 2015 Google, Inc.
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

#define LOG_TAG "bt_device_interop"

#include <assert.h>
#include <string.h> // For memcmp

#include "btcore/include/module.h"
#include "device/include/interop.h"
#include "device/include/interop_database.h"
#include "osi/include/allocator.h"
#include "osi/include/list.h"
#include "osi/include/log.h"

#define CASE_RETURN_STR(const) case const: return #const;

static list_t *interop_list = NULL;

static const char* interop_feature_string_(const interop_feature_t feature);
static void interop_free_entry_(void *data);
static void interop_lazy_init_(void);
static bool interop_match_fixed_(const interop_feature_t feature, const bt_bdaddr_t *addr);
static bool interop_match_dynamic_(const interop_feature_t feature, const bt_bdaddr_t *addr);

// Interface functions

bool interop_match_addr(const interop_feature_t feature, const bt_bdaddr_t *addr) {
  assert(addr);

  if (interop_match_fixed_(feature, addr) || interop_match_dynamic_(feature, addr)) {
    char bdstr[20] = {0};
    LOG_WARN(LOG_TAG, "%s() Device %s is a match for interop workaround %s.",
          __func__, bdaddr_to_string(addr, bdstr, sizeof(bdstr)),
                        interop_feature_string_(feature));
    return true;
  }

  return false;
}

bool interop_match_name(const interop_feature_t feature, const char *name) {
  assert(name);

  const size_t db_size = sizeof(interop_name_database) / sizeof(interop_name_entry_t);
  for (size_t i = 0; i != db_size; ++i) {
    if (feature == interop_name_database[i].feature &&
        strlen(name) >= interop_name_database[i].length &&
        strncmp(name, interop_name_database[i].name, interop_name_database[i].length) == 0) {
      return true;
    }
  }

  return false;
}

void interop_database_add(const uint16_t feature, const bt_bdaddr_t *addr, size_t length) {
  assert(addr);
  assert(length > 0);
  assert(length < sizeof(bt_bdaddr_t));

  interop_addr_entry_t *entry = osi_calloc(sizeof(interop_addr_entry_t));
  memcpy(&entry->addr, addr, length);
  entry->feature = feature;
  entry->length = length;

  interop_lazy_init_();
  list_append(interop_list, entry);
}

void interop_database_clear() {
  if (interop_list)
    list_clear(interop_list);
}

// Module life-cycle functions

static future_t *interop_clean_up(void) {
  list_free(interop_list);
  interop_list = NULL;
  return future_new_immediate(FUTURE_SUCCESS);
}

EXPORT_SYMBOL module_t interop_module = {
  .name = INTEROP_MODULE,
  .init = NULL,
  .start_up = NULL,
  .shut_down = NULL,
  .clean_up = interop_clean_up,
  .dependencies = {NULL},
};

// Local functions

static const char* interop_feature_string_(const interop_feature_t feature) {
  switch (feature) {
    CASE_RETURN_STR(INTEROP_DISABLE_LE_SECURE_CONNECTIONS)
    CASE_RETURN_STR(INTEROP_AUTO_RETRY_PAIRING)
    CASE_RETURN_STR(INTEROP_DISABLE_ABSOLUTE_VOLUME)
    CASE_RETURN_STR(INTEROP_DISABLE_AUTO_PAIRING)
    CASE_RETURN_STR(INTEROP_KEYBOARD_REQUIRES_FIXED_PIN)
    CASE_RETURN_STR(INTEROP_2MBPS_LINK_ONLY)
#if defined(MTK_STACK_CONFIG_BL) && (MTK_STACK_CONFIG_BL == TRUE)
    CASE_RETURN_STR(INTEROP_MTK_LE_CONN_INT_MIN_LIMIT_ACCEPT)
    CASE_RETURN_STR(INTEROP_MTK_HOGP_CONN_INT_UPDATE_ADJUST)
    CASE_RETURN_STR(INTEROP_MTK_LE_CONN_TIMEOUT_ADJUST)
    CASE_RETURN_STR(INTEROP_MTK_HFP_DEALY_OPEN_SCO)
    CASE_RETURN_STR(INTEROP_MTK_HID_DISABLE_SDP)
    CASE_RETURN_STR(INTEROP_MTK_HFP_15_ESCO_NOT_USE_MSBC)
    CASE_RETURN_STR(INTEROP_MTK_AV_CONNECT_DO_SDP_EVEN_FAIL)
    CASE_RETURN_STR(INTEROP_MTK_AVRCP_DO_RELEASE_KEY)
    CASE_RETURN_STR(INTEROP_MTK_A2DP_DELAY_START_CMD)
    CASE_RETURN_STR(INTEROP_MTK_AVRCP_DISABLE_SONG_POS)
    CASE_RETURN_STR(INTEROP_MTK_HFP_15_NOT_USE_ESCO)
    CASE_RETURN_STR(INTEROP_MTK_A2DP_NOT_SET_SCMT)
    CASE_RETURN_STR(INTEROP_MTK_AVRCP_15_TO_14)
    CASE_RETURN_STR(INTEROP_MTK_AVRCP_15_TO_13)
    CASE_RETURN_STR(INTEROP_MTK_HID_NOT_DO_SNIFF_SUBRATING)
    CASE_RETURN_STR(INTEROP_MTK_CLOSE_AVDTP_SIG_CH)
    CASE_RETURN_STR(INTEROP_MTK_HID_NOT_SET_SLAVE)
    CASE_RETURN_STR(INTEROP_MTK_HFP_ACL_COLLISION)
    CASE_RETURN_STR(INTEROP_MTK_FLUSH_L2C_A2D_BUF)
#endif
  }

  return "UNKNOWN";
}

static void interop_free_entry_(void *data) {
  interop_addr_entry_t *entry = (interop_addr_entry_t *)data;
  osi_free(entry);
}

static void interop_lazy_init_(void) {
  if (interop_list == NULL) {
    interop_list = list_new(interop_free_entry_);
  }
}

static bool interop_match_dynamic_(const interop_feature_t feature, const bt_bdaddr_t *addr) {
  if (interop_list == NULL || list_length(interop_list) == 0)
    return false;

  const list_node_t *node = list_begin(interop_list);
  while (node != list_end(interop_list)) {
    interop_addr_entry_t *entry = list_node(node);
    assert(entry);

    if (feature == entry->feature && memcmp(addr, &entry->addr, entry->length) == 0)
      return true;

    node = list_next(node);
  }
  return false;
}

static bool interop_match_fixed_(const interop_feature_t feature, const bt_bdaddr_t *addr) {
  assert(addr);

  const size_t db_size = sizeof(interop_addr_database) / sizeof(interop_addr_entry_t);
  for (size_t i = 0; i != db_size; ++i) {
    if (feature == interop_addr_database[i].feature &&
        memcmp(addr, &interop_addr_database[i].addr, interop_addr_database[i].length) == 0) {
      return true;
    }
  }

  return false;
}

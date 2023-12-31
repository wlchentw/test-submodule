/******************************************************************************
 *
 *  Copyright (C) 2014 Google, Inc.
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

#define LOG_TAG "bt_controller"

#include "device/include/controller.h"

#include <assert.h>

#include "bt_types.h"
#include "btcore/include/event_mask.h"
#include "btcore/include/module.h"
#include "btcore/include/version.h"
#include "hcimsgs.h"
#include "osi/include/future.h"
#include "stack/include/btm_ble_api.h"

#if defined(MTK_STACK_CONFIG_LOG) && (MTK_STACK_CONFIG_LOG == TRUE)
#include "osi/include/log.h"
#include "mdroid_stack_config.h"
#endif

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
const bt_event_mask_t BLE_EVENT_MASK = { "\x00\x00\x00\x00\x00\x02\x1e\x7f" };
#else
const bt_event_mask_t BLE_EVENT_MASK = { "\x00\x00\x00\x00\x00\x00\x06\x7f" };
#endif

#if (BLE_INCLUDED)
const bt_event_mask_t CLASSIC_EVENT_MASK = { HCI_DUMO_EVENT_MASK_EXT };
#else
const bt_event_mask_t CLASSIC_EVENT_MASK = { HCI_LISBON_EVENT_MASK_EXT };
#endif

#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
const bt_event_mask_t CLASSIC_EVENT_MASK_PAGE_2 = { HCI_PAGE_2_EVENT_MASK };
#endif

// TODO(zachoverflow): factor out into common module
const uint8_t SCO_HOST_BUFFER_SIZE = 0xff;

#define HCI_SUPPORTED_COMMANDS_ARRAY_SIZE 64
#define MAX_FEATURES_CLASSIC_PAGE_COUNT 3
#define BLE_SUPPORTED_STATES_SIZE         8
#define BLE_SUPPORTED_FEATURES_SIZE       8
#define MAX_LOCAL_SUPPORTED_CODECS_SIZE   8

static const hci_t *hci;
static const hci_packet_factory_t *packet_factory;
static const hci_packet_parser_t *packet_parser;

static bt_bdaddr_t address;
static bt_version_t bt_version;

static uint8_t supported_commands[HCI_SUPPORTED_COMMANDS_ARRAY_SIZE];
static bt_device_features_t features_classic[MAX_FEATURES_CLASSIC_PAGE_COUNT];
static uint8_t last_features_classic_page_index;

static uint16_t acl_data_size_classic;
static uint16_t acl_data_size_ble;
static uint16_t acl_buffer_count_classic;
static uint8_t acl_buffer_count_ble;

static uint8_t ble_white_list_size;
static uint8_t ble_resolving_list_max_size;
static uint8_t ble_supported_states[BLE_SUPPORTED_STATES_SIZE];
static bt_device_features_t features_ble;
static uint16_t ble_suggested_default_data_length;
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
static uint16_t ble_maxium_advertising_data_length;
static uint8_t ble_number_of_supported_advertising_sets;
#endif
static uint8_t local_supported_codecs[MAX_LOCAL_SUPPORTED_CODECS_SIZE];
static uint8_t number_of_local_supported_codecs = 0;

static bool readable;
static bool ble_supported;
static bool simple_pairing_supported;
static bool secure_connections_supported;

#define AWAIT_COMMAND(command) future_await(hci->transmit_command_futured(command))

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
/**
 * This function is called to set link key and pincode timeout to controller.
 */
static void btm_write_pin_code_link_key_timeout(void)
{
#ifndef VENDOR_OPCODE_MTK
#define VENDOR_OPCODE_MTK               0xFC66
#endif
#ifndef MTK_PIN_CODE_LINK_KEY_TIMEOUT
#define MTK_PIN_CODE_LINK_KEY_TIMEOUT   {0x00, 0x10, 0x40, 0x9C}
#endif
#define MTK_CFG_TIMEOUT_PARAM_LEN       0x04
#define MTK_CFG_BUF_LEN                 (MTK_CFG_TIMEOUT_PARAM_LEN + 5)
  UINT16 opcode = VENDOR_OPCODE_MTK;
  UINT8 param_len = MTK_CFG_TIMEOUT_PARAM_LEN;
  // set pin code timeout 25s, link key timeout 2.56s.
  UINT8 param[MTK_CFG_BUF_LEN] = MTK_PIN_CODE_LINK_KEY_TIMEOUT;
  BTM_VendorSpecificCommand(opcode, param_len, param, NULL);
}
#endif

// Module lifecycle functions

static future_t *start_up(void) {
  BT_HDR *response;

  // Send the initial reset command
  response = AWAIT_COMMAND(packet_factory->make_reset());
  packet_parser->parse_generic_command_complete(response);

  // Request the classic buffer size next
  response = AWAIT_COMMAND(packet_factory->make_read_buffer_size());
  packet_parser->parse_read_buffer_size_response(
      response, &acl_data_size_classic, &acl_buffer_count_classic);

  // Tell the controller about our buffer sizes and buffer counts next
  // TODO(zachoverflow): factor this out. eww l2cap contamination. And why just a hardcoded 10?
  response = AWAIT_COMMAND(
    packet_factory->make_host_buffer_size(
      L2CAP_MTU_SIZE,
      SCO_HOST_BUFFER_SIZE,
      L2CAP_HOST_FC_ACL_BUFS,
      10
    )
  );

  packet_parser->parse_generic_command_complete(response);

  // Read the local version info off the controller next, including
  // information such as manufacturer and supported HCI version
  response = AWAIT_COMMAND(packet_factory->make_read_local_version_info());
  packet_parser->parse_read_local_version_info_response(response, &bt_version);

  // Read the bluetooth address off the controller next
  response = AWAIT_COMMAND(packet_factory->make_read_bd_addr());
  packet_parser->parse_read_bd_addr_response(response, &address);

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
  btm_write_pin_code_link_key_timeout();
#endif

  // Request the controller's supported commands next
  response = AWAIT_COMMAND(packet_factory->make_read_local_supported_commands());
  packet_parser->parse_read_local_supported_commands_response(
    response,
    supported_commands,
    HCI_SUPPORTED_COMMANDS_ARRAY_SIZE
  );

  // Read page 0 of the controller features next
  uint8_t page_number = 0;
  response = AWAIT_COMMAND(packet_factory->make_read_local_extended_features(page_number));
  packet_parser->parse_read_local_extended_features_response(
    response,
    &page_number,
    &last_features_classic_page_index,
    features_classic,
    MAX_FEATURES_CLASSIC_PAGE_COUNT
  );

  assert(page_number == 0);
  page_number++;

  // Inform the controller what page 0 features we support, based on what
  // it told us it supports. We need to do this first before we request the
  // next page, because the controller's response for page 1 may be
  // dependent on what we configure from page 0
  simple_pairing_supported = HCI_SIMPLE_PAIRING_SUPPORTED(features_classic[0].as_array);
  if (simple_pairing_supported) {
    response = AWAIT_COMMAND(packet_factory->make_write_simple_pairing_mode(HCI_SP_MODE_ENABLED));
    packet_parser->parse_generic_command_complete(response);
  }

#if (BLE_INCLUDED == TRUE)
  if (HCI_LE_SPT_SUPPORTED(features_classic[0].as_array)) {
    uint8_t simultaneous_le_host = HCI_SIMUL_LE_BREDR_SUPPORTED(features_classic[0].as_array) ? BTM_BLE_SIMULTANEOUS_HOST : 0;
    response = AWAIT_COMMAND(
      packet_factory->make_ble_write_host_support(BTM_BLE_HOST_SUPPORT, simultaneous_le_host)
    );

    packet_parser->parse_generic_command_complete(response);

    // If we modified the BT_HOST_SUPPORT, we will need ext. feat. page 1
    if (last_features_classic_page_index < 1)
      last_features_classic_page_index = 1;
  }
#endif

  // Done telling the controller about what page 0 features we support
  // Request the remaining feature pages
  while (page_number <= last_features_classic_page_index &&
         page_number < MAX_FEATURES_CLASSIC_PAGE_COUNT) {
    response = AWAIT_COMMAND(packet_factory->make_read_local_extended_features(page_number));
    packet_parser->parse_read_local_extended_features_response(
      response,
      &page_number,
      &last_features_classic_page_index,
      features_classic,
      MAX_FEATURES_CLASSIC_PAGE_COUNT
    );

    page_number++;
  }

#if (SC_MODE_INCLUDED == TRUE)
  secure_connections_supported = HCI_SC_CTRLR_SUPPORTED(features_classic[2].as_array);
  if (secure_connections_supported) {
    response = AWAIT_COMMAND(packet_factory->make_write_secure_connections_host_support(HCI_SC_MODE_ENABLED));
    packet_parser->parse_generic_command_complete(response);
  }
#endif

#if (BLE_INCLUDED == TRUE)
  ble_supported = last_features_classic_page_index >= 1 && HCI_LE_HOST_SUPPORTED(features_classic[1].as_array);
  if (ble_supported) {
    // Request the ble white list size next
    response = AWAIT_COMMAND(packet_factory->make_ble_read_white_list_size());
    packet_parser->parse_ble_read_white_list_size_response(response, &ble_white_list_size);

    // Request the ble buffer size next
    response = AWAIT_COMMAND(packet_factory->make_ble_read_buffer_size());
    packet_parser->parse_ble_read_buffer_size_response(
      response,
      &acl_data_size_ble,
      &acl_buffer_count_ble
    );

    // Response of 0 indicates ble has the same buffer size as classic
    if (acl_data_size_ble == 0)
      acl_data_size_ble = acl_data_size_classic;

    // Request the ble supported states next
    response = AWAIT_COMMAND(packet_factory->make_ble_read_supported_states());
    packet_parser->parse_ble_read_supported_states_response(
      response,
      ble_supported_states,
      sizeof(ble_supported_states)
    );

    // Request the ble supported features next
    response = AWAIT_COMMAND(packet_factory->make_ble_read_local_supported_features());
    packet_parser->parse_ble_read_local_supported_features_response(
      response,
      &features_ble
    );

    if (HCI_LE_ENHANCED_PRIVACY_SUPPORTED(features_ble.as_array)) {
        response = AWAIT_COMMAND(packet_factory->make_ble_read_resolving_list_size());
        packet_parser->parse_ble_read_resolving_list_size_response(
            response,
            &ble_resolving_list_max_size);
    }

    if (HCI_LE_DATA_LEN_EXT_SUPPORTED(features_ble.as_array)) {
        response = AWAIT_COMMAND(packet_factory->make_ble_read_suggested_default_data_length());
        packet_parser->parse_ble_read_suggested_default_data_length_response(
            response,
            &ble_suggested_default_data_length);
    }

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (HCI_LE_EXTENDED_ADVERTISING_SUPPORTED(features_ble.as_array)) {
        response = AWAIT_COMMAND(
            packet_factory->make_ble_read_maximum_advertising_data_length());
        packet_parser->parse_ble_read_maximum_advertising_data_length(
            response, &ble_maxium_advertising_data_length);

        response = AWAIT_COMMAND(
            packet_factory->make_ble_read_number_of_supported_advertising_sets());
        packet_parser->parse_ble_read_number_of_supported_advertising_sets(
            response, &ble_number_of_supported_advertising_sets);
    } else {
      /* If LE Excended Advertising is not supported, use the default value */
      ble_maxium_advertising_data_length = 31;
    }
#endif

    // Set the ble event mask next
    response = AWAIT_COMMAND(packet_factory->make_ble_set_event_mask(&BLE_EVENT_MASK));
    packet_parser->parse_generic_command_complete(response);
  }
#endif

  if (simple_pairing_supported) {
    response = AWAIT_COMMAND(packet_factory->make_set_event_mask(&CLASSIC_EVENT_MASK));
    packet_parser->parse_generic_command_complete(response);
  }

#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
    response = AWAIT_COMMAND(packet_factory->make_set_event_mask_2(&CLASSIC_EVENT_MASK_PAGE_2));
    packet_parser->parse_generic_command_complete(response);
#endif

  // read local supported codecs
  if(HCI_READ_LOCAL_CODECS_SUPPORTED(supported_commands)) {
    response = AWAIT_COMMAND(packet_factory->make_read_local_supported_codecs());
    packet_parser->parse_read_local_supported_codecs_response(
        response,
        &number_of_local_supported_codecs, local_supported_codecs);
  }

#if defined(MTK_STACK_CONFIG_LOG) && (MTK_STACK_CONFIG_LOG == TRUE)
  {
    const stack_config_pack_hexlist_t *p_sconf_fwlog_hex = stack_config_fwlog_hexs_get_interface();

    if (p_sconf_fwlog_hex->get_pack_hexlists()) {
      const uint8_t *p_hexary = p_sconf_fwlog_hex->get_whole_hexlists();
      uint8_t *p_hexcmd = NULL;

      int i, k, j = 0, param_len;
      for (j = 0; j < MTK_STACK_CONFIG_NUM_OF_HEXLIST; j++)
      {
        p_hexcmd = (uint8_t *)(p_hexary + j*MTK_STACK_CONFIG_NUM_OF_HEXITEMS);

        if (p_hexcmd[0] == 0)
          break;

        param_len = p_hexcmd[3];
        LOG_INFO(LOG_TAG, "%s M_BTCONF hci tx c%d: opcode 0x%02x / param len %d", __func__, j, *(uint16_t *)(p_hexcmd + 1), param_len);

        for (k = 0; k < param_len; k++) {
          LOG_INFO(LOG_TAG, "%02x", *(p_hexcmd + 4 + k));
        }

        response = AWAIT_COMMAND(packet_factory->make_set_cmd_from_ext(*(uint16_t *)(p_hexcmd + 1), p_hexcmd + 4, p_hexcmd[3]));
        LOG_INFO(LOG_TAG, "M_BTCONF hci rx: 0x%02x 0x%02x 0x%02x 0x%02x", response->event, response->len, response->offset, response->layer_specific);
        for (i = 0; i < response->len; i++)
          LOG_INFO(LOG_TAG, "%02x", response->data[i]);

        packet_parser->parse_generic_command_complete(response);
      }
    }
  }
#endif /* if MTK_STACK_CONFIG_LOG == TRUE */

  assert(HCI_READ_ENCR_KEY_SIZE_SUPPORTED(supported_commands));

  readable = true;
  return future_new_immediate(FUTURE_SUCCESS);
}

static future_t *shut_down(void) {
  readable = false;
  return future_new_immediate(FUTURE_SUCCESS);
}

EXPORT_SYMBOL const module_t controller_module = {
  .name = CONTROLLER_MODULE,
  .init = NULL,
  .start_up = start_up,
  .shut_down = shut_down,
  .clean_up = NULL,
  .dependencies = {
    HCI_MODULE,
    NULL
  }
};

// Interface functions

static bool get_is_ready(void) {
  return readable;
}

static const bt_bdaddr_t *get_address(void) {
  assert(readable);
  return &address;
}

static const bt_version_t *get_bt_version(void) {
  assert(readable);
  return &bt_version;
}

// TODO(zachoverflow): hide inside, move decoder inside too
static const bt_device_features_t *get_features_classic(int index) {
  assert(readable);
  assert(index < MAX_FEATURES_CLASSIC_PAGE_COUNT);
  return &features_classic[index];
}

static uint8_t get_last_features_classic_index(void) {
  assert(readable);
  return last_features_classic_page_index;
}

static uint8_t *get_local_supported_codecs(uint8_t *number_of_codecs) {
  assert(readable);
  if(number_of_local_supported_codecs) {
    *number_of_codecs = number_of_local_supported_codecs;
    return local_supported_codecs;
  }
  return NULL;
}

static const bt_device_features_t *get_features_ble(void) {
  assert(readable);
  assert(ble_supported);
  return &features_ble;
}

static const uint8_t *get_ble_supported_states(void) {
  assert(readable);
  assert(ble_supported);
  return ble_supported_states;
}

static bool supports_simple_pairing(void) {
  assert(readable);
  return simple_pairing_supported;
}

static bool supports_secure_connections(void) {
  assert(readable);
  return secure_connections_supported;
}

static bool supports_simultaneous_le_bredr(void) {
  assert(readable);
  return HCI_SIMUL_LE_BREDR_SUPPORTED(features_classic[0].as_array);
}

static bool supports_reading_remote_extended_features(void) {
  assert(readable);
  return HCI_READ_REMOTE_EXT_FEATURES_SUPPORTED(supported_commands);
}

static bool supports_interlaced_inquiry_scan(void) {
  assert(readable);
  return HCI_LMP_INTERLACED_INQ_SCAN_SUPPORTED(features_classic[0].as_array);
}

static bool supports_rssi_with_inquiry_results(void) {
  assert(readable);
  return HCI_LMP_INQ_RSSI_SUPPORTED(features_classic[0].as_array);
}

static bool supports_extended_inquiry_response(void) {
  assert(readable);
  return HCI_EXT_INQ_RSP_SUPPORTED(features_classic[0].as_array);
}

static bool supports_master_slave_role_switch(void) {
  assert(readable);
  return HCI_SWITCH_SUPPORTED(features_classic[0].as_array);
}

static bool supports_ble(void) {
  assert(readable);
  return ble_supported;
}

static bool supports_ble_privacy(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_ENHANCED_PRIVACY_SUPPORTED(features_ble.as_array);
}

static bool supports_ble_set_privacy_mode() {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_ENHANCED_PRIVACY_SUPPORTED(features_ble.as_array) &&
         HCI_LE_SET_PRIVACY_MODE_SUPPORTED(supported_commands);
}

static bool supports_ble_packet_extension(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_DATA_LEN_EXT_SUPPORTED(features_ble.as_array);
}

static bool supports_ble_connection_parameters_request(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_CONN_PARAM_REQ_SUPPORTED(features_ble.as_array);
}

static uint16_t get_acl_data_size_classic(void) {
  assert(readable);
  return acl_data_size_classic;
}

static uint16_t get_acl_data_size_ble(void) {
  assert(readable);
  assert(ble_supported);
  return acl_data_size_ble;
}

static uint16_t get_acl_packet_size_classic(void) {
  assert(readable);
  return acl_data_size_classic + HCI_DATA_PREAMBLE_SIZE;
}

static uint16_t get_acl_packet_size_ble(void) {
  assert(readable);
  return acl_data_size_ble + HCI_DATA_PREAMBLE_SIZE;
}

static uint16_t get_ble_suggested_default_data_length(void) {
  assert(readable);
  assert(ble_supported);
  return ble_suggested_default_data_length;
}

static uint16_t get_acl_buffer_count_classic(void) {
  assert(readable);
  return acl_buffer_count_classic;
}

static uint8_t get_acl_buffer_count_ble(void) {
  assert(readable);
  assert(ble_supported);
  return acl_buffer_count_ble;
}

static uint8_t get_ble_white_list_size(void) {
  assert(readable);
  assert(ble_supported);
  return ble_white_list_size;
}

static uint8_t get_ble_resolving_list_max_size(void) {
  assert(readable);
  assert(ble_supported);
  return ble_resolving_list_max_size;
}

static void set_ble_resolving_list_max_size(int resolving_list_max_size) {
  // Setting "resolving_list_max_size" to 0 is done during cleanup,
  // hence we ignore the "readable" flag already set to false during shutdown.
  if (resolving_list_max_size != 0) {
    assert(readable);
  }
  assert(ble_supported);
  ble_resolving_list_max_size = resolving_list_max_size;
}

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
static bool supports_ble_2m_phy(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_2M_PHY_SUPPORTED(features_ble.as_array);
}

static bool supports_ble_coded_phy(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_CODED_PHY_SUPPORTED(features_ble.as_array);
}

static bool supports_ble_extended_advertising(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_EXTENDED_ADVERTISING_SUPPORTED(features_ble.as_array);
}

static bool supports_ble_periodic_advertising(void) {
  assert(readable);
  assert(ble_supported);
  return HCI_LE_PERIODIC_ADVERTISING_SUPPORTED(features_ble.as_array);
}

static uint16_t get_ble_maxium_advertising_data_length(void) {
  assert(readable);
  assert(ble_supported);
  return ble_maxium_advertising_data_length;
}

static uint8_t get_ble_number_of_supported_advertising_sets(void) {
  assert(readable);
  assert(ble_supported);
  return ble_number_of_supported_advertising_sets;
}

static uint8_t get_le_all_initiating_phys() {
  uint8_t phy = PHY_LE_1M;

  if (supports_ble_2m_phy()) phy |= PHY_LE_2M;
  if (supports_ble_coded_phy()) phy |= PHY_LE_CODED;
  return phy;
}
#endif

static const controller_t interface = {
  get_is_ready,

  get_address,
  get_bt_version,

  get_features_classic,
  get_last_features_classic_index,

  get_features_ble,
  get_ble_supported_states,

  supports_simple_pairing,
  supports_secure_connections,
  supports_simultaneous_le_bredr,
  supports_reading_remote_extended_features,
  supports_interlaced_inquiry_scan,
  supports_rssi_with_inquiry_results,
  supports_extended_inquiry_response,
  supports_master_slave_role_switch,

  supports_ble,
  supports_ble_packet_extension,
  supports_ble_connection_parameters_request,
  supports_ble_privacy,
  supports_ble_set_privacy_mode,

  get_acl_data_size_classic,
  get_acl_data_size_ble,

  get_acl_packet_size_classic,
  get_acl_packet_size_ble,
  get_ble_suggested_default_data_length,

  get_acl_buffer_count_classic,
  get_acl_buffer_count_ble,

  get_ble_white_list_size,

  get_ble_resolving_list_max_size,
  set_ble_resolving_list_max_size,
  get_local_supported_codecs
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
  ,supports_ble_2m_phy,
  supports_ble_coded_phy,
  supports_ble_extended_advertising,
  supports_ble_periodic_advertising,
  get_ble_maxium_advertising_data_length,
  get_ble_number_of_supported_advertising_sets,
  get_le_all_initiating_phys,
#endif
};

EXPORT_SYMBOL const controller_t *controller_get_interface() {
  static bool loaded = false;
  if (!loaded) {
    loaded = true;

    hci = hci_layer_get_interface();
    packet_factory = hci_packet_factory_get_interface();
    packet_parser = hci_packet_parser_get_interface();
  }

  return &interface;
}

const controller_t *controller_get_test_interface(
    const hci_t *hci_interface,
    const hci_packet_factory_t *packet_factory_interface,
    const hci_packet_parser_t *packet_parser_interface) {

  hci = hci_interface;
  packet_factory = packet_factory_interface;
  packet_parser = packet_parser_interface;
  return &interface;
}

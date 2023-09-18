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

/* FILE NAME:  linuxbt_gap_if.c
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
 */

#include <stdlib.h>
#include <string.h>

#include "bt_mw_common.h"
#include "bluetooth.h"
#include "linuxbt_common.h"
#include "linuxbt_gap_if.h"
#include "bt_mw_gap.h"

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
#include "mtk_bluetooth.h"
#endif
#include"bt_mw_message_queue.h"

#define UNUSED_ATTR __attribute__((unused))

#define LINUXBT_GAP_PROTOCOL_ATT             0x0007

/* Define common 16-bit service class UUIDs
*/
#define LINUXBT_GAP_SERVICE_DISCOVERY_SERVER 0X1000
#define LINUXBT_GAP_BROWSE_GROUP_DESCRIPTOR  0X1001
#define LINUXBT_GAP_PUBLIC_BROWSE_GROUP      0X1002
#define LINUXBT_GAP_SERIAL_PORT              0X1101
#define LINUXBT_GAP_LAN_ACCESS_USING_PPP     0X1102
#define LINUXBT_GAP_DIALUP_NETWORKING        0X1103
#define LINUXBT_GAP_IRMC_SYNC                0X1104
#define LINUXBT_GAP_OBEX_OBJECT_PUSH         0X1105
#define LINUXBT_GAP_OBEX_FILE_TRANSFER       0X1106
#define LINUXBT_GAP_IRMC_SYNC_COMMAND        0X1107
#define LINUXBT_GAP_HEADSET                  0X1108
#define LINUXBT_GAP_CORDLESS_TELEPHONY       0X1109
#define LINUXBT_GAP_AUDIO_SOURCE             0X110A
#define LINUXBT_GAP_AUDIO_SINK               0X110B
#define LINUXBT_GAP_AV_REM_CTRL_TARGET       0X110C  /* Audio/Video Control profile */
#define LINUXBT_GAP_ADV_AUDIO_DISTRIBUTION   0X110D  /* Advanced Audio Distribution profile */
#define LINUXBT_GAP_AV_REMOTE_CONTROL        0X110E  /* Audio/Video Control profile */
#define LINUXBT_GAP_AV_REM_CTRL_CONTROL      0X110F  /* Audio/Video Control profile */
#define LINUXBT_GAP_INTERCOM                 0X1110
#define LINUXBT_GAP_FAX                      0X1111
#define LINUXBT_GAP_HEADSET_AUDIO_GATEWAY    0X1112
#define LINUXBT_GAP_WAP                      0X1113
#define LINUXBT_GAP_WAP_CLIENT               0X1114
#define LINUXBT_GAP_PANU                     0X1115  /* PAN profile */
#define LINUXBT_GAP_NAP                      0X1116  /* PAN profile */
#define LINUXBT_GAP_GN                       0X1117  /* PAN profile */
#define LINUXBT_GAP_DIRECT_PRINTING          0X1118  /* BPP profile */
#define LINUXBT_GAP_REFERENCE_PRINTING       0X1119  /* BPP profile */
#define LINUXBT_GAP_IMAGING                  0X111A  /* Imaging profile */
#define LINUXBT_GAP_IMAGING_RESPONDER        0X111B  /* Imaging profile */
#define LINUXBT_GAP_IMAGING_AUTO_ARCHIVE     0X111C  /* Imaging profile */
#define LINUXBT_GAP_IMAGING_REF_OBJECTS      0X111D  /* Imaging profile */
#define LINUXBT_GAP_HF_HANDSFREE             0X111E  /* Handsfree profile */
#define LINUXBT_GAP_AG_HANDSFREE             0X111F  /* Handsfree profile */
#define LINUXBT_GAP_DIR_PRT_REF_OBJ_SERVICE  0X1120  /* BPP profile */
#define LINUXBT_GAP_REFLECTED_UI             0X1121  /* BPP profile */
#define LINUXBT_GAP_BASIC_PRINTING           0X1122  /* BPP profile */
#define LINUXBT_GAP_PRINTING_STATUS          0X1123  /* BPP profile */
#define LINUXBT_GAP_HUMAN_INTERFACE          0X1124  /* HID profile */
#define LINUXBT_GAP_CABLE_REPLACEMENT        0X1125  /* HCRP profile */
#define LINUXBT_GAP_HCRP_PRINT               0X1126  /* HCRP profile */
#define LINUXBT_GAP_HCRP_SCAN                0X1127  /* HCRP profile */
#define LINUXBT_GAP_COMMON_ISDN_ACCESS       0X1128  /* CAPI Message Transport Protocol*/
#define LINUXBT_GAP_VIDEO_CONFERENCING_GW    0X1129  /* Video Conferencing profile */
#define LINUXBT_GAP_UDI_MT                   0X112A  /* Unrestricted Digital Information profile */
#define LINUXBT_GAP_UDI_TA                   0X112B  /* Unrestricted Digital Information profile */
#define LINUXBT_GAP_VCP                      0X112C  /* Video Conferencing profile */
#define LINUXBT_GAP_SAP                      0X112D  /* SIM Access profile */
#define LINUXBT_GAP_PBAP_PCE                 0X112E  /* Phonebook Access - PCE */
#define LINUXBT_GAP_PBAP_PSE                 0X112F  /* Phonebook Access - PSE */
#define LINUXBT_GAP_PHONE_ACCESS             0x1130
#define LINUXBT_GAP_HEADSET_HS               0x1131  /* Headset - HS, from HSP v1.2 */
#if defined(MTK_B3DS_SUPPORT) && (MTK_B3DS_SUPPORT == TRUE)
#define LINUXBT_GAP_3D_DISPLAY               0x1137  /* B3DS_INCLUDED */
#define LINUXBT_GAP_3D_GLASSES               0x1138  /* B3DS_INCLUDED */
#define LINUXBT_GAP_3D_SYNCHRONIZATION       0x1139  /* B3DS_INCLUDED */
#endif
#define LINUXBT_GAP_PNP_INFORMATION          0X1200  /* Device Identification */
#define LINUXBT_GAP_GENERIC_NETWORKING       0X1201
#define LINUXBT_GAP_GENERIC_FILETRANSFER     0X1202
#define LINUXBT_GAP_GENERIC_AUDIO            0X1203
#define LINUXBT_GAP_GENERIC_TELEPHONY        0X1204
#define LINUXBT_GAP_UPNP_SERVICE             0X1205  /* UPNP_Service [ESDP] */
#define LINUXBT_GAP_UPNP_IP_SERVICE          0X1206  /* UPNP_IP_Service [ESDP] */
#define LINUXBT_GAP_ESDP_UPNP_IP_PAN         0X1300  /* UPNP_IP_PAN [ESDP] */
#define LINUXBT_GAP_ESDP_UPNP_IP_LAP         0X1301  /* UPNP_IP_LAP [ESDP] */
#define LINUXBT_GAP_ESDP_UPNP_IP_L2CAP       0X1302  /* UPNP_L2CAP [ESDP] */
#define LINUXBT_GAP_VIDEO_SOURCE             0X1303  /* Video Distribution Profile (VDP) */
#define LINUXBT_GAP_VIDEO_SINK               0X1304  /* Video Distribution Profile (VDP) */
#define LINUXBT_GAP_VIDEO_DISTRIBUTION       0X1305  /* Video Distribution Profile (VDP) */
#define LINUXBT_GAP_HDP_PROFILE              0X1400  /* Health Device profile (HDP) */
#define LINUXBT_GAP_HDP_SOURCE               0X1401  /* Health Device profile (HDP) */
#define LINUXBT_GAP_HDP_SINK                 0X1402  /* Health Device profile (HDP) */
#define LINUXBT_GAP_MAP_PROFILE              0X1134  /* MAP profile UUID */
#define LINUXBT_GAP_MESSAGE_ACCESS           0X1132  /* Message Access Service UUID */
#define LINUXBT_GAP_MESSAGE_NOTIFICATION     0X1133  /* Message Notification Service UUID */

#define LINUXBT_GAP_GAP_SERVER               0x1800
#define LINUXBT_GAP_GATT_SERVER              0x1801
#define LINUXBT_GAP_IMMEDIATE_ALERT          0x1802      /* immediate alert */
#define LINUXBT_GAP_LINKLOSS                 0x1803      /* Link Loss Alert */
#define LINUXBT_GAP_TX_POWER                 0x1804      /* TX power */
#define LINUXBT_GAP_CURRENT_TIME             0x1805      /* Link Loss Alert */
#define LINUXBT_GAP_DST_CHG                  0x1806      /* DST Time change */
#define LINUXBT_GAP_REF_TIME_UPD             0x1807      /* reference time update */
#define LINUXBT_GAP_THERMOMETER              0x1809      /* Thermometer UUID */
#define LINUXBT_GAP_DEVICE_INFO              0x180A      /* device info service */
#define LINUXBT_GAP_NWA                      0x180B      /* Network availability */
#define LINUXBT_GAP_HEART_RATE               0x180D      /* Heart Rate service */
#define LINUXBT_GAP_PHALERT                  0x180E      /* phone alert service */
#define LINUXBT_GAP_BATTERY                  0x180F     /* battery service */
#define LINUXBT_GAP_BPM                      0x1810      /*  blood pressure service */
#define LINUXBT_GAP_ALERT_NOTIFICATION       0x1811      /* alert notification service */
#define LINUXBT_GAP_LE_HID                   0x1812     /*  HID over LE */
#define LINUXBT_GAP_SCAN_PARAM               0x1813      /* Scan Parameter service */
#define LINUXBT_GAP_GLUCOSE                  0x1808      /* Glucose Meter Service */
#define LINUXBT_GAP_RSC                      0x1814      /* RUNNERS SPEED AND CADENCE SERVICE      */
#define LINUXBT_GAP_CSC                      0x1816      /* Cycling SPEED AND CADENCE SERVICE      */


static bluetooth_device_t *g_bt_device = NULL;
static const bt_interface_t *g_bt_interface = NULL;

extern BOOL fg_bt_scan_ongoing;
CHAR mp_name[16];

extern int open_bluetooth_stack(const struct hw_module_t *module,
                                UNUSED_ATTR char const *name,
                                struct hw_device_t **abstraction) ;

// Callback functions declaration
void linuxbt_gap_state_changed_cb(bt_state_t state);

void linuxbt_gap_properties_cb(bt_status_t status,
                               int num_properties,
                               bt_property_t *properties);

void linuxbt_gap_remote_device_properties_cb(bt_status_t status,
        bt_bdaddr_t *bd_addr,
        int num_properties,
        bt_property_t *properties);

void linuxbt_gap_device_found_cb(int num_properties,
                                 bt_property_t *properties);

void linuxbt_gap_discovery_state_changed_cb(bt_discovery_state_t state);

void linuxbt_gap_pin_request_cb(bt_bdaddr_t *remote_bd_addr,
                                bt_bdname_t *bd_name, uint32_t cod,bool min_16_digit);


void linuxbt_gap_ssp_request_cb(bt_bdaddr_t *remote_bd_addr,
                                bt_bdname_t *bd_name,
                                uint32_t cod,
                                bt_ssp_variant_t pairing_variant,
                                uint32_t pass_key);

void linuxbt_gap_bond_state_changed_cb(bt_status_t status,
                                       bt_bdaddr_t *remote_bd_addr,
                                       bt_bond_state_t state);

void linuxbt_gap_acl_state_changed_cb(bt_status_t status,
                                      bt_bdaddr_t *remote_bd_addr,
                                      bt_acl_state_t state);

void linuxbt_gap_acl_disconnect_reason_callback(bt_bdaddr_t *remote_bd_addr, uint8_t reason);
void linuxbt_gap_get_rssi_cb(bt_status_t status, bt_bdaddr_t *remote_bd_addr , int rssi_value);
void linuxbt_gap_get_bonded_device_cb();

static CHAR* linuxbt_gap_get_service_str(UINT16 uuid);
static CHAR* linuxbt_gap_get_property_type_str(bt_property_type_t type);
static CHAR* linuxbt_gap_get_dev_type_str(UINT32 type);
static CHAR* linuxbt_gap_get_bond_state_str(UINT32 bond_state);


bool bt_set_wake_alarm(uint64_t delay_millis, bool should_wake, alarm_cb cb, void *data)
{
    return true;
}

int bt_acquire_wake_lock(const char *lock_name)
{
    return 0;
}

int bt_release_wake_lock(const char *lock_name)
{
    return 0;
}

static bt_os_callouts_t g_callouts =
{
    sizeof(bt_os_callouts_t),
    bt_set_wake_alarm,
    bt_acquire_wake_lock,
    bt_release_wake_lock,
};

static bt_callbacks_t g_bt_callbacks =
{
    sizeof(bt_callbacks_t),
    linuxbt_gap_state_changed_cb,
    linuxbt_gap_properties_cb,
    linuxbt_gap_remote_device_properties_cb,
    linuxbt_gap_device_found_cb,
    linuxbt_gap_discovery_state_changed_cb,
    linuxbt_gap_pin_request_cb,
    linuxbt_gap_ssp_request_cb,
    linuxbt_gap_bond_state_changed_cb,
    linuxbt_gap_acl_state_changed_cb,
    NULL,
    NULL,
    NULL,
    NULL,
};


const UINT16 linuxbt_gap_service_id_to_uuid_lkup_tbl [32] =
{
    LINUXBT_GAP_PNP_INFORMATION,         /* Reserved */
    LINUXBT_GAP_SERIAL_PORT,             /* BTA_SPP_SERVICE_ID */
    LINUXBT_GAP_DIALUP_NETWORKING,       /* BTA_DUN_SERVICE_ID */
    LINUXBT_GAP_AUDIO_SOURCE,            /* BTA_A2DP_SOURCE_SERVICE_ID */
    LINUXBT_GAP_LAN_ACCESS_USING_PPP,    /* BTA_LAP_SERVICE_ID */
    LINUXBT_GAP_HEADSET,                 /* BTA_HSP_HS_SERVICE_ID */
    LINUXBT_GAP_HF_HANDSFREE,            /* BTA_HFP_HS_SERVICE_ID */
    LINUXBT_GAP_OBEX_OBJECT_PUSH,        /* BTA_OPP_SERVICE_ID */
    LINUXBT_GAP_OBEX_FILE_TRANSFER,      /* BTA_FTP_SERVICE_ID */
    LINUXBT_GAP_CORDLESS_TELEPHONY,      /* BTA_CTP_SERVICE_ID */
    LINUXBT_GAP_INTERCOM,                /* BTA_ICP_SERVICE_ID */
    LINUXBT_GAP_IRMC_SYNC,               /* BTA_SYNC_SERVICE_ID */
    LINUXBT_GAP_DIRECT_PRINTING,         /* BTA_BPP_SERVICE_ID */
    LINUXBT_GAP_IMAGING_RESPONDER,       /* BTA_BIP_SERVICE_ID */
    LINUXBT_GAP_PANU,                    /* BTA_PANU_SERVICE_ID */
    LINUXBT_GAP_NAP,                     /* BTA_NAP_SERVICE_ID */
    LINUXBT_GAP_GN,                      /* BTA_GN_SERVICE_ID */
    LINUXBT_GAP_SAP,                     /* BTA_SAP_SERVICE_ID */
    LINUXBT_GAP_AUDIO_SINK,              /* BTA_A2DP_SERVICE_ID */
    LINUXBT_GAP_AV_REMOTE_CONTROL,       /* BTA_AVRCP_SERVICE_ID */
    LINUXBT_GAP_HUMAN_INTERFACE,         /* BTA_HID_SERVICE_ID */
    LINUXBT_GAP_VIDEO_SINK,              /* BTA_VDP_SERVICE_ID */
    LINUXBT_GAP_PBAP_PSE,                /* BTA_PBAP_SERVICE_ID */
    LINUXBT_GAP_HEADSET_AUDIO_GATEWAY,   /* BTA_HSP_SERVICE_ID */
    LINUXBT_GAP_AG_HANDSFREE,            /* BTA_HFP_SERVICE_ID */
    LINUXBT_GAP_MESSAGE_ACCESS,          /* BTA_MAP_SERVICE_ID */
    LINUXBT_GAP_MESSAGE_NOTIFICATION,    /* BTA_MN_SERVICE_ID */
    LINUXBT_GAP_HDP_PROFILE,             /* BTA_HDP_SERVICE_ID */
    LINUXBT_GAP_PBAP_PCE,                /* BTA_PCE_SERVICE_ID */
    LINUXBT_GAP_PROTOCOL_ATT             /* BTA_GATT_SERVICE_ID */
};



#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
static const btgap_ex_interface_t *linuxbt_gap_ex_interface = NULL;

static btgap_ex_callbacks_t linuxbt_gap_ex_callbacks =
{
    sizeof(btgap_ex_callbacks_t),
    linuxbt_gap_get_rssi_cb,
    linuxbt_gap_acl_disconnect_reason_callback,
};

INT32 linuxbtgap_send_hci_handler(CHAR *ptr)
{
    int i = 0;
    uint8_t   rpt_size = 0;
    uint8_t   hex_bytes_filled;
    uint8_t hex_buf[200] = {0};
    uint16_t   hex_len = 0;
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, ptr);
    BT_CHECK_POINTER(BT_DEBUG_GAP, linuxbt_gap_ex_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "hci=%s", ptr);


    if (FALSE == bt_mw_gap_is_power_on())
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "power is off, skip");
        return BT_ERR_STATUS_FAIL;
    }

    rpt_size = strlen(ptr);
    hex_len = (strlen(ptr) + 1) / 2;

    BT_DBG_INFO(BT_DEBUG_GAP, "rpt_size=%ld, hex_len=%ld", (unsigned long)rpt_size, (unsigned long)hex_len);
    hex_bytes_filled = ascii_2_hex(ptr, hex_len, hex_buf);
    BT_DBG_INFO(BT_DEBUG_GAP, "hex_bytes_filled=%ld", (unsigned long)hex_bytes_filled);
    for (i=0;i<hex_len;i++)
    {
        BT_DBG_NOTICE(BT_DEBUG_GAP, "hex values= %02X",hex_buf[i]);
    }
    if (hex_bytes_filled)
    {
        ret = linuxbt_gap_ex_interface->send_hci((uint8_t*)hex_buf, hex_bytes_filled);
        BT_DBG_INFO(BT_DEBUG_GAP, "send_hci");
        return linuxbt_return_value_convert(ret);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "hex_bytes_filled <= 0");
        return BT_ERR_STATUS_PARM_INVALID;
    }
    // return 0;

}

INT32 linuxbtgap_set_lhdc_key_data_handler(CHAR *name, UINT8 *data, INT32 data_len)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, name);
    BT_CHECK_POINTER(BT_DEBUG_GAP, data);
    BT_CHECK_POINTER(BT_DEBUG_GAP, linuxbt_gap_ex_interface);

    ret = linuxbt_gap_ex_interface->set_lhdc_key_data(name, data, data_len);
    BT_DBG_INFO(BT_DEBUG_GAP, "set_lhdc_key_data");
    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_get_rssi_handler(CHAR *pbt_addr)
{
    bt_bdaddr_t bdaddr;
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, pbt_addr);
    BT_CHECK_POINTER(BT_DEBUG_GAP, linuxbt_gap_ex_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "addr=%s", pbt_addr);

    memset(&bdaddr, 0, sizeof(bt_bdaddr_t));

    linuxbt_btaddr_stoh(pbt_addr, &bdaddr);
    ret = linuxbt_gap_ex_interface->get_rssi(&bdaddr);
    return linuxbt_return_value_convert(ret);
}
#endif

INT32 linuxbt_gap_enable_handler(VOID)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "");

    ret = g_bt_interface->enable(false);
    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_disable_handler(VOID)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "");

    ret = g_bt_interface->disable();
    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_get_adapter_properties_handler(VOID)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "");

    ret = g_bt_interface->get_adapter_properties();
    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_set_device_name_handler(CHAR *pname)
{
    bt_property_t property;
    bt_property_t *property_p;
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, pname);
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "pname=%s", pname);

    memset(&property, 0, sizeof(bt_property_t));

    property_p = &property;

    property_p->type = BT_PROPERTY_BDNAME;
    property_p->len = strlen(pname);
    property_p->val = pname;

    strcpy((char*)mp_name,pname);

    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "mp_name=%s", mp_name);

    ret = g_bt_interface->set_adapter_property(property_p);
    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_set_scan_mode(INT32 mode)
{
    bt_property_t property;
    bt_property_t *property_p;
    bt_scan_mode_t scan_mode;
    bt_status_t ret = BT_STATUS_SUCCESS;

    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "mode=%d", mode);

    memset(&property, 0, sizeof(bt_property_t));
    scan_mode = (bt_scan_mode_t)mode;

    property_p = &property;
    property_p->type = BT_PROPERTY_ADAPTER_SCAN_MODE;
    property_p->len = sizeof(bt_scan_mode_t);
    property_p->val = (void*)&scan_mode;

    ret = g_bt_interface->set_adapter_property(property_p);

    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_start_discovery_handler(VOID)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "");

    ret = g_bt_interface->start_discovery();

    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_cancel_discovery_handler(VOID)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "");

    ret = g_bt_interface->cancel_discovery();

    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_create_bond_handler(CHAR *pbt_addr, INT32 transport)
{
    FUNC_ENTRY;
    bt_bdaddr_t bdaddr;
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, pbt_addr);
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "addr=%s", pbt_addr);

    memset(&bdaddr, 0, sizeof(bt_bdaddr_t));
    linuxbt_btaddr_stoh(pbt_addr, &bdaddr);

    ret = g_bt_interface->create_bond(&bdaddr, transport);

    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_remove_bond_handler(CHAR *pbt_addr)
{
    bt_bdaddr_t bdaddr;
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, pbt_addr);
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "addr=%s", pbt_addr);

    memset(&bdaddr, 0, sizeof(bt_bdaddr_t));
    linuxbt_btaddr_stoh(pbt_addr, &bdaddr);

    ret = g_bt_interface->remove_bond(&bdaddr);

    return linuxbt_return_value_convert(ret);
}

INT32 linuxbt_gap_config_hci_snoop_log_handler(UINT8 enable)
{
    bt_status_t ret = BT_STATUS_SUCCESS;
    BT_CHECK_POINTER(BT_DEBUG_GAP, g_bt_interface);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "enable=%d", enable);

    ret = g_bt_interface->config_hci_snoop_log(enable);

    return linuxbt_return_value_convert(ret);
}

UINT32 linuxbt_gap_parse_uuid2service(bt_uuid_t *p_uuid_128, INT32 num_uuid)
{
    UINT8 xx, yy;
    UINT32 service = 0;
    bt_uuid_t uuid_128;
    BT_CHECK_POINTER(BT_DEBUG_GAP, p_uuid_128);
    for( xx = 0; xx < num_uuid; xx++ )
    {
        for( yy = 0; yy < 32; yy++ )
        {
            linuxbt_uuid16_to_uuid128(linuxbt_gap_service_id_to_uuid_lkup_tbl[yy], &uuid_128);
            if(0 == memcmp(p_uuid_128 + xx, &uuid_128, sizeof(uuid_128)))
            {
                BT_DBG_INFO(BT_DEBUG_GAP, "service=%s",
                    linuxbt_gap_get_service_str(linuxbt_gap_service_id_to_uuid_lkup_tbl[yy]));
                service |= (1 << yy);
                break;
            }
        }

        /* for HSP v1.2 only device */
        linuxbt_uuid16_to_uuid128(LINUXBT_GAP_HEADSET_HS, &uuid_128);
        if(0 == memcmp(p_uuid_128 + xx, &uuid_128, sizeof(uuid_128)))
        {
            BT_DBG_INFO(BT_DEBUG_GAP, "service=%s",
                linuxbt_gap_get_service_str(LINUXBT_GAP_HEADSET_HS));
            service |= (1 << BT_GAP_HSP_SERVICE_ID);
        }

        linuxbt_uuid16_to_uuid128(LINUXBT_GAP_HDP_SOURCE, &uuid_128);
        if(0 == memcmp(p_uuid_128 + xx, &uuid_128, sizeof(uuid_128)))
        {
            BT_DBG_INFO(BT_DEBUG_GAP, "service=%s",
                linuxbt_gap_get_service_str(LINUXBT_GAP_HDP_SOURCE));
            service |= (1 << BT_GAP_HDP_SERVICE_ID);
        }

        linuxbt_uuid16_to_uuid128(LINUXBT_GAP_HDP_SINK, &uuid_128);
        if(0 == memcmp(p_uuid_128 + xx, &uuid_128, sizeof(uuid_128)))
        {
            BT_DBG_INFO(BT_DEBUG_GAP, "service=%s",
                linuxbt_gap_get_service_str(LINUXBT_GAP_HDP_SINK));
            service |= (1 << BT_GAP_HDP_SERVICE_ID);
        }

        linuxbt_uuid16_to_uuid128(LINUXBT_GAP_LE_HID, &uuid_128);
        if (0 == memcmp(p_uuid_128 + xx, &uuid_128, sizeof(uuid_128)))
        {
            BT_DBG_INFO(BT_DEBUG_GAP, "service=%s",
                linuxbt_gap_get_service_str(LINUXBT_GAP_LE_HID));
            service |= (1 << BT_GAP_HID_SERVICE_ID);
        }
    }

    return service;
}

UINT32 linuxbt_gap_parse_device_properties(BLUETOOTH_DEVICE *device,
                                                   int num_properties,
                                                   bt_property_t *properties)
{
    bt_property_t *property;
    CHAR *name;
    bt_bdaddr_t* btaddr;
    UINT8 bonded_dev_num = 0;
    UINT32 prop_mask = 0;

    BT_CHECK_POINTER(BT_DEBUG_GAP, properties);
    //BT_DBG_NORMAL(BT_DEBUG_GAP, "======Receive Properties pointer = : %p=======",p_msg->properties);
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "============Propertie num : %d================",num_properties);
    for (UINT8 i = 0; i < num_properties; i++)
    {
        property = &properties[i];
        prop_mask |= (1 << property->type);
        switch (property->type)
        {
        case BT_PROPERTY_BDNAME:
            name = (CHAR *)property->val;
            //BT_DBG_NORMAL(BT_DEBUG_GAP, "Propertie name raw data : 0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,0x%X,",name,name+1,name+2,name+3,name+4,name+5,name+6,name+7,name+8);
            if (strlen(name) > 0)
            {
                strncpy(device->name, name, property->len>MAX_NAME_LEN?MAX_NAME_LEN:property->len);
                device->name[((property->len > MAX_NAME_LEN) ? MAX_NAME_LEN : property->len)] = '\0';
                BT_DBG_NORMAL(BT_DEBUG_GAP, "bdname = %s",
                                          device->name);
            }
            else
            {
                BT_DBG_NORMAL(BT_DEBUG_GAP, "type = %ld, len = %ld, bdname is null",
                                          (long)property->type,
                                          (long)property->len);
            }
            break;
        case BT_PROPERTY_BDADDR:
            btaddr = (bt_bdaddr_t *)property->val;
            linuxbt_btaddr_htos(btaddr, device->bdAddr);
            BT_DBG_NORMAL(BT_DEBUG_GAP, "bdaddr = %s", device->bdAddr);
            break;
        case BT_PROPERTY_CLASS_OF_DEVICE:
            device->cod= *((UINT32 *)(property->val));
            BT_DBG_NORMAL(BT_DEBUG_GAP, "cod = 0x%x", (UINT32)(device->cod));
            break;
        case BT_PROPERTY_REMOTE_RSSI:
            device->rssi = *((INT8*)(property->val));
            BT_DBG_NORMAL(BT_DEBUG_GAP, "rssi = %d", device->rssi);
            break;
        case BT_PROPERTY_TYPE_OF_DEVICE:
            device->devicetype = *((UINT32 *)(property->val));
            BT_DBG_NORMAL(BT_DEBUG_GAP, "devtype = %s",
                linuxbt_gap_get_dev_type_str(device->devicetype));
            break;
        case BT_PROPERTY_ADAPTER_SCAN_MODE:
            device->scan_mode= *((UINT32 *)(property->val));
            BT_DBG_NORMAL(BT_DEBUG_GAP, "scan mode = %d", *((UINT32 *)(property->val)));
            break;
        case BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT:
            BT_DBG_NORMAL(BT_DEBUG_GAP, "disc_timeout = %d", *((UINT32 *)(property->val)));
            break;
        case BT_PROPERTY_UUIDS:
            device->service =
                linuxbt_gap_parse_uuid2service((bt_uuid_t*)property->val,
                    property->len/sizeof(bt_uuid_t));
            BT_DBG_NORMAL(BT_DEBUG_GAP, "uuid2service = 0x%x", device->service);
            break;
        case BT_PROPERTY_ADAPTER_BONDED_DEVICES:
            {
                CHAR s_addr[MAX_BDADDR_LEN];
                bonded_dev_num = property->len / sizeof(bt_bdaddr_t);
                btaddr = (bt_bdaddr_t *)property->val;
                for (UINT8 k=0; k<bonded_dev_num; k++)
                {
                    BT_DBG_NORMAL(BT_DEBUG_GAP,
                        "bonded_addr = %02X:%02X:%02X:%02X:%02X:%02X",
                              btaddr[k].address[0], btaddr[k].address[1],
                              btaddr[k].address[2], btaddr[k].address[3],
                              btaddr[k].address[4], btaddr[k].address[5]);

                    linuxbt_btaddr_htos(&btaddr[k], s_addr);
                    bt_mw_gap_add_bonded_dev(s_addr);
                }
            }
            break;
        case BT_PROPERTY_REMOTE_SERVICE:
            device->service = *((UINT32 *)property->val);
            BT_DBG_NORMAL(BT_DEBUG_GAP, "service = 0x%x", device->service);
            break;
        case BT_PROPERTY_REMOTE_FRIENDLY_NAME:
            name = (CHAR *)property->val;
            BT_DBG_INFO(BT_DEBUG_GAP, "alias = %s", name);
            break;
        case BT_PROPERTY_REMOTE_VERSION_INFO:
            {
                bt_remote_version_t *version =
                    (bt_remote_version_t *)property->val;
                BT_DBG_INFO(BT_DEBUG_GAP, "version = %d.%d, manufacturer=%d",
                    version->version, version->sub_ver, version->manufacturer);
            }
            break;
        case BT_PROPERTY_LOCAL_LE_FEATURES:
            {
                btgap_ext_local_le_features_t *ex_local_le_features =
                    (btgap_ext_local_le_features_t *)property->val;
                bt_local_le_features_t local_le_features =
                    ex_local_le_features->local_le_features;
                BT_DBG_INFO(BT_DEBUG_GAP, "le feature, version = 0x%x",
                    local_le_features.version_supported);

                device->le_maximum_advertising_data_length =
                    ex_local_le_features->le_maximum_advertising_data_length;
                BT_DBG_INFO(BT_DEBUG_GAP, "le feature, device le_maximum_advertising_data_length = %d",
                    device->le_maximum_advertising_data_length);
            }
            break;
        default:
            BT_DBG_INFO(BT_DEBUG_GAP, "[GAP] type = %s(%d) len=%d",
                linuxbt_gap_get_property_type_str(property->type),
                property->type, property->len);
            break;
        }
    }
    BT_DBG_NORMAL(BT_DEBUG_GAP, "============Properties End================");

    return prop_mask;
}

void linuxbt_gap_properties_cb(bt_status_t status,
                               int num_properties,
                               bt_property_t *properties)
{

    tBTMW_MSG msg = {0};
    BT_DBG_NORMAL(BT_DEBUG_GAP, " status: %ld", (long)status);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, properties);
    if (BT_STATUS_SUCCESS == status)
    {
        msg.data.device_info.prop_mask =
            linuxbt_gap_parse_device_properties(&(msg.data.device_info.device), num_properties, properties);
        msg.data.device_info.device_kind = BT_DEVICE_LOCAL;
        msg.hdr.event = BTMW_GAP_DEVICE_INFO_EVT;
        msg.hdr.len = sizeof(tBTMW_GAP_DEVICE_INFO);
        linuxbt_send_msg(&msg);
    }
}

void linuxbt_gap_remote_device_properties_cb(bt_status_t status,
        bt_bdaddr_t *bd_addr,
        int num_properties,
        bt_property_t *properties)
{
    tBTMW_MSG msg = {0};
    BT_DBG_NORMAL(BT_DEBUG_GAP, " status: %ld", (long)status);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, properties);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, bd_addr);
    if (BT_STATUS_SUCCESS != status)
    {
        return;
    }

    msg.data.device_info.prop_mask =
            linuxbt_gap_parse_device_properties(&(msg.data.device_info.device), num_properties, properties);
    if(strlen(msg.data.device_info.device.bdAddr) == 0)
    {
        linuxbt_btaddr_htos(bd_addr, msg.data.device_info.device.bdAddr);
    }
    BT_DBG_NORMAL(BT_DEBUG_GAP, "status:%d, device:%s", status, msg.data.device_info.device.bdAddr);

    if (fg_bt_scan_ongoing)
    {
        // When at the end of scan, host get the address of remote device, will go this.
        msg.data.device_info.device_kind = BT_DEVICE_SCAN;
    }
    else
    {
        // only when BT enable, will go this.
        msg.data.device_info.device_kind = BT_DEVICE_BONDED;
    }
    msg.hdr.event = BTMW_GAP_DEVICE_INFO_EVT;
    msg.hdr.len = sizeof(tBTMW_GAP_DEVICE_INFO);

    if(strlen(msg.data.device_info.device.bdAddr) || strlen(msg.data.device_info.device.name))
    {
        linuxbt_send_msg(&msg);
    }
}

void linuxbt_gap_device_found_cb(int num_properties,
                                 bt_property_t *properties)
{
    tBTMW_MSG msg = {0};
    BT_DBG_NORMAL(BT_DEBUG_GAP, "device found");
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, properties);

    msg.data.device_info.prop_mask =
        linuxbt_gap_parse_device_properties(&(msg.data.device_info.device), num_properties, properties);
    msg.data.device_info.device_kind = BT_DEVICE_SCAN;
    msg.hdr.event = BTMW_GAP_DEVICE_INFO_EVT;
    msg.hdr.len = sizeof(tBTMW_GAP_DEVICE_INFO);
    linuxbt_send_msg(&msg);

}


void linuxbt_gap_pin_request_cb(bt_bdaddr_t *remote_bd_addr,
                                bt_bdname_t *bd_name, uint32_t cod,bool min_16_digit)
{
    BT_DBG_NORMAL(BT_DEBUG_GAP, "%s()", __FUNCTION__);

    bt_pin_code_t pin;
    bt_status_t ret = BT_STATUS_SUCCESS;
    uint8_t fg_accept = 1;
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, g_bt_interface);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, remote_bd_addr);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, bd_name);

    memset(pin.pin, 0, 16);

    bt_gap_get_pin_code_cb(remote_bd_addr, &pin, (UINT8 *)&fg_accept);
    if (0 == strlen((CHAR*)&pin.pin))
    {
        if (strncmp("xs-soundbar", (CHAR*)&bd_name->name, 11) == 0)
        {
            BT_DBG_NORMAL(BT_DEBUG_GAP, "PIN CODE default:0901");
            pin.pin[0] = 0x30;
            pin.pin[1] = 0x39;
            pin.pin[2] = 0x30;
            pin.pin[3] = 0x31;
        }
        else
        {
            BT_DBG_NORMAL(BT_DEBUG_GAP, "PIN CODE default:0000");
            pin.pin[0] = 0x30;
            pin.pin[1] = 0x30;
            pin.pin[2] = 0x30;
            pin.pin[3] = 0x30;
        }
    }

    ret = g_bt_interface->pin_reply(remote_bd_addr, (uint8_t)fg_accept, 4, &pin);
    if (BT_SUCCESS != linuxbt_return_value_convert(ret))
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "pin_reply error!\n");
    }
}


void linuxbt_gap_ssp_request_cb(bt_bdaddr_t *remote_bd_addr,
                                bt_bdname_t *bd_name, uint32_t cod,
                                bt_ssp_variant_t pairing_variant,
                                uint32_t passkey)
{
    BT_DBG_NORMAL(BT_DEBUG_GAP, "%s()", __FUNCTION__);
    bt_status_t ret = BT_STATUS_SUCCESS;
    uint8_t fg_accept = 1;
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, remote_bd_addr);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, bd_name);
    BT_CHECK_POINTER_RETURN(BT_DEBUG_GAP, g_bt_interface);

    if (remote_bd_addr)
    {
        bt_bdaddr_t *btaddr = remote_bd_addr;

        BT_DBG_INFO(BT_DEBUG_GAP, "REMOTE BDADDR = %02X:%02X:%02X:%02X:%02X:%02X",
                      btaddr->address[0], btaddr->address[1], btaddr->address[2],
                      btaddr->address[3], btaddr->address[4], btaddr->address[5]);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "remote_bd_addr is NULL!");
    }
    if (NULL == g_bt_interface)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get GAP interface");
        return;
    }
    if (bd_name)
    {
        BT_DBG_NORMAL(BT_DEBUG_GAP, "BDNAME = %s", bd_name->name);
    }
    BT_DBG_INFO(BT_DEBUG_GAP, "cod = 0x%08X, pairing_variant = %ld, passkey = %ld.", cod, (long)pairing_variant, (unsigned long)passkey);
    BT_DBG_INFO(BT_DEBUG_GAP, "passkey = %ld.", (unsigned long)passkey);

    bt_gap_get_passkey_cb(remote_bd_addr, passkey, (UINT8 *)&fg_accept);
    ret = g_bt_interface->ssp_reply(remote_bd_addr, pairing_variant, fg_accept, passkey);
    if (BT_SUCCESS != linuxbt_return_value_convert(ret))
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "ssp_reply error!\n");
    }
}

void linuxbt_gap_state_changed_cb(bt_state_t state)
{
    tBTMW_MSG msg = {0};
    BT_DBG_NORMAL(BT_DEBUG_GAP, "%s() state: %ld", __FUNCTION__, (long)state);

    switch (state)
    {
    case BT_STATE_OFF:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT STATE OFF");
        msg.hdr.event = BTMW_GAP_STATE_EVT;
        msg.hdr.len = sizeof(tBTMW_GAP_STATE);
        msg.data.gap_state.state = GAP_STATE_OFF;

        linuxbt_send_msg(&msg);
        break;

    case BT_STATE_ON:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT STATE ON");
        msg.hdr.event = BTMW_GAP_STATE_EVT;
        msg.hdr.len = sizeof(tBTMW_GAP_STATE);
        msg.data.gap_state.state = GAP_STATE_ON;
        linuxbt_send_msg(&msg);
        break;

    default:
        break;
    }
}

void linuxbt_gap_discovery_state_changed_cb(bt_discovery_state_t state)
{
    tBTMW_MSG msg = {0};
    BT_DBG_NORMAL(BT_DEBUG_GAP, " state: %ld", (long)state);

    switch (state)
    {
    case BT_DISCOVERY_STOPPED:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT Search Device Stop.");
        msg.hdr.event = BTMW_GAP_STATE_EVT;
        msg.hdr.len = sizeof(tBTMW_GAP_STATE);
        msg.data.gap_state.state = GAP_STATE_DISCOVERY_STOPED;
        linuxbt_send_msg(&msg);
        break;

    case BT_DISCOVERY_STARTED:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT Search Device Start...");
        msg.hdr.event = BTMW_GAP_STATE_EVT;
        msg.hdr.len = sizeof(tBTMW_GAP_STATE);
        msg.data.gap_state.state = GAP_STATE_DISCOVERY_STARTED;
        linuxbt_send_msg(&msg);
        break;
    default:
        break;
    }
}

void linuxbt_gap_bond_state_changed_cb(bt_status_t status,
                                       bt_bdaddr_t *remote_bd_addr,
                                       bt_bond_state_t state)
{
    tBTMW_MSG msg = {0};
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "status=(%s)%d, state =(%s)%d",
        linuxbt_get_bt_status_str(status), status,
        linuxbt_gap_get_bond_state_str(state), state);

    if (BT_STATUS_SUCCESS == status)
    {
        switch (state)
        {
        case BT_BOND_STATE_NONE:
            msg.data.gap_state.state = GAP_STATE_UNPAIR_SUCCESS;
            BT_DBG_NOTICE(BT_DEBUG_GAP, "state is unpair success.");
            break;
        case BT_BOND_STATE_BONDING:
            msg.data.gap_state.state = GAP_STATE_BONDING;
            BT_DBG_NOTICE(BT_DEBUG_GAP, "state is bonding.");
            break;
        case BT_BOND_STATE_BONDED:
            msg.data.gap_state.state = GAP_STATE_BONDED;
            BT_DBG_NOTICE(BT_DEBUG_GAP, "state is bonded.");
            break;
        default:
            break;
        }
    }
    else
    {
        if (BT_BOND_STATE_NONE == state)
        {
            msg.data.gap_state.state = GAP_STATE_NO_BOND;
            BT_DBG_NOTICE(BT_DEBUG_GAP, "state is no bond(%s).",
                linuxbt_get_bt_status_str(status));
        }
    }

    if (remote_bd_addr)
    {
        bt_bdaddr_t *btaddr = remote_bd_addr;
        BT_DBG_NOTICE(BT_DEBUG_GAP, "REMOTE BDADDR = %02X:%02X:%02X:%02X:%02X:%02X",
                          btaddr->address[0], btaddr->address[1], btaddr->address[2],
                          btaddr->address[3], btaddr->address[4], btaddr->address[5]);
        linuxbt_btaddr_htos(btaddr, msg.data.gap_state.bd_addr);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "remote_bd_addr is NULL!");
    }

    msg.hdr.event = BTMW_GAP_STATE_EVT;
    msg.hdr.len = sizeof(tBTMW_GAP_STATE);
    linuxbt_send_msg(&msg);

}

void linuxbt_gap_acl_state_changed_cb(bt_status_t status, bt_bdaddr_t *remote_bd_addr,
                                      bt_acl_state_t state)
{
    tBTMW_MSG msg = {0};
    BT_MW_FUNC_ENTER(BT_DEBUG_GAP, "status=(%s)%d, state = %d",
        linuxbt_get_bt_status_str(status), status, state);

    switch (status)
    {
    case BT_STATUS_SUCCESS:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT bond status is successful(%ld), ", (long)status);
        break;
    default:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "BT bond status is failed(%ld), ", (long)status);
        break;
    }
    //bt_gap_state.status = status;

    switch (state)
    {
    case BT_ACL_STATE_CONNECTED:
        msg.data.gap_state.state = GAP_STATE_ACL_CONNECTED;
        BT_DBG_NOTICE(BT_DEBUG_GAP, "acl is connected.");

        break;
    case BT_ACL_STATE_DISCONNECTED:
        BT_DBG_NOTICE(BT_DEBUG_GAP, "acl is disconnected.");
        msg.data.gap_state.state = GAP_STATE_ACL_DISCONNECTED;
        break;
    default:
        break;
    }

    if (remote_bd_addr)
    {
        bt_bdaddr_t *btaddr = remote_bd_addr;
        linuxbt_btaddr_htos(btaddr, msg.data.gap_state.bd_addr);
        BT_DBG_NORMAL(BT_DEBUG_GAP, "REMOTE BDADDR = %02X:%02X:%02X:%02X:%02X:%02X",
                          btaddr->address[0], btaddr->address[1], btaddr->address[2],
                          btaddr->address[3], btaddr->address[4], btaddr->address[5]);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "remote_bd_addr is NULL!");
    }

    msg.hdr.event = BTMW_GAP_STATE_EVT;
    msg.hdr.len = sizeof(tBTMW_GAP_STATE);
    linuxbt_send_msg(&msg);

}

void linuxbt_gap_acl_disconnect_reason_callback(bt_bdaddr_t *remote_bd_addr, uint8_t reason)
{
    tBTMW_MSG msg = {0};
    if (NULL == remote_bd_addr)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "null pointer of remote_bd_addr");
        return;
    }

    BT_DBG_NORMAL(BT_DEBUG_GAP, "REMOTE BDADDR = %02X:%02X:%02X:%02X:%02X:%02X",
                remote_bd_addr->address[0], remote_bd_addr->address[1], remote_bd_addr->address[2],
                remote_bd_addr->address[3], remote_bd_addr->address[4], remote_bd_addr->address[5]);
    BT_DBG_NORMAL(BT_DEBUG_GAP, "disconnect reason = %ld",(unsigned long)reason);

    linuxbt_btaddr_htos(remote_bd_addr, msg.data.gap_state.bd_addr);
    msg.data.gap_state.state = GAP_STATE_ACL_DISCONNECTED;
    msg.data.gap_state.reason = reason ;
    msg.hdr.event = BTMW_GAP_STATE_EVT;
    msg.hdr.len = sizeof(tBTMW_GAP_STATE);
    linuxbt_send_msg(&msg);

}



void linuxbt_gap_get_rssi_cb(bt_status_t status, bt_bdaddr_t *remote_bd_addr , int rssi_value)
{
    BT_DBG_NORMAL(BT_DEBUG_GAP, "%s()  %ld ", __FUNCTION__, (long)(rssi_value));
    bt_gap_get_rssi_result_cb(rssi_value);
}

const void *linuxbt_gap_get_profile_interface(const char *profile_id)
{
    if (NULL != g_bt_interface)
    {
        return g_bt_interface->get_profile_interface(profile_id);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get GAP interface");
        return NULL;
    }
}

int linuxbt_gap_init(void)
{
    FUNC_ENTRY;

    bt_status_t ret = BT_STATUS_SUCCESS;
    // Init bluetooth interface.
    open_bluetooth_stack(NULL, "Stack Linux", (struct hw_device_t**)&g_bt_device);
    if (NULL == g_bt_device)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to open Bluetooth stack.");
        return BT_ERR_STATUS_NOT_READY;
    }

    g_bt_interface = g_bt_device->get_bluetooth_interface();
    if (NULL == g_bt_interface)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get Bluetooth interface");
        return BT_ERR_STATUS_FAIL;
    }
    /*
        1. This will register callback function for OS callouts, and set is_native value as false in wakelock.c.
            And is_native will affact the behvaior of the function alarm_set which will launch a timer.
            When is_native is true, the "/sys/power/wake_lock" and  "/sys/power/wake_unlock" will be used in the execution of alarm_set.
            Once the set_os_callouts not invoked and "/sys/power/wake_lock" or "/sys/power/wake_unlock" not available, alarm_set will fail to launch a timer,
            this will result to no sound come out issue when in A2DP (both Sink & Source) application.
        2. the callback function do nothing.

        3. Very important note: this shouldn't be marked when poring to other project/platform.
      */
    g_bt_interface->set_os_callouts(&g_callouts);

    ret = g_bt_interface->init(&g_bt_callbacks);
    if (BT_SUCCESS != linuxbt_return_value_convert(ret))
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "gap init error!");
    }

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
    linuxbt_gap_ex_interface = (btgap_ex_interface_t *) linuxbt_gap_get_profile_interface(BT_PROFILE_GAP_EX_ID);
    if (NULL == linuxbt_gap_ex_interface)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get Bluetooth extended interface");
        return BT_ERR_STATUS_FAIL;
    }

    ret = linuxbt_gap_ex_interface->init(&linuxbt_gap_ex_callbacks);
    if (BT_SUCCESS != linuxbt_return_value_convert(ret))
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "gap init error!");
    }
#endif

    return linuxbt_return_value_convert(ret);
}

int linuxbt_gap_deinit(void)
{
    //bt_status_t ret = BT_STATUS_SUCCESS;

    if (NULL != g_bt_interface)
    {
        //g_bt_interface->disable();
        g_bt_interface->cleanup();
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get GAP interface");
        return BT_ERR_STATUS_FAIL;
    }


    return BT_SUCCESS;//linuxbt_return_value_convert(ret);
}

int linuxbt_gap_set_bt_wifi_ratio(uint8_t bt_ratio, uint8_t wifi_ratio)
{
    uint8_t cmd[5] = {0};

    if (FALSE == bt_mw_gap_is_power_on())
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "power is off, skip");
        return BT_ERR_STATUS_FAIL;
    }

    cmd[0] = 0xf1;
    cmd[1] = 0xfc;
    cmd[2] = 0x02;
    cmd[3] = bt_ratio;
    cmd[4] = wifi_ratio;

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)
    linuxbt_gap_ex_interface->send_hci(cmd, sizeof(cmd));
#endif

    return BT_SUCCESS;
}

#if defined(MTK_LINUX_GAP) && (MTK_LINUX_GAP == TRUE)

int linuxbt_interop_database_add(uint16_t feature, bt_bdaddr_t *remote_bd_addr,size_t len)
{
    if(NULL == g_bt_interface)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get GAP interface");
        return BT_ERR_STATUS_FAIL;
    }
    g_bt_interface->interop_database_add(feature, remote_bd_addr,len);
    return BT_SUCCESS;
}

int linuxbt_interop_database_clear()
{
    if(NULL == g_bt_interface)
    {
        BT_DBG_ERROR(BT_DEBUG_GAP, "Failed to get GAP interface");
        return BT_ERR_STATUS_FAIL;
    }
    g_bt_interface->interop_database_clear();
    return BT_SUCCESS;
}

#endif

static CHAR* linuxbt_gap_get_service_str(UINT16 uuid)
{
     switch(uuid)
     {
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_PNP_INFORMATION, "pnp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_SERIAL_PORT, "spp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_DIALUP_NETWORKING, "dun");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_AUDIO_SOURCE, "a2dp_source");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_LAN_ACCESS_USING_PPP, "lap");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_HEADSET, "hsp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_HF_HANDSFREE, "hfp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_OBEX_OBJECT_PUSH, "opp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_OBEX_FILE_TRANSFER, "ftp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_CORDLESS_TELEPHONY, "ctp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_INTERCOM, "icp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_IRMC_SYNC, "sync");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_DIRECT_PRINTING, "bpp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_IMAGING_RESPONDER, "bip");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_PANU, "panu");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_NAP, "nap");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_GN, "gn");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_SAP, "sap");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_AUDIO_SINK, "a2dp_sink");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_AV_REMOTE_CONTROL, "avrcp_ct");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_HUMAN_INTERFACE, "hid");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_VIDEO_SINK, "vdp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_PBAP_PSE, "pbap");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_HEADSET_AUDIO_GATEWAY, "hsp_gw");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_AG_HANDSFREE, "hfp_ag");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_MESSAGE_ACCESS, "map");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_MESSAGE_NOTIFICATION, "mn");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_HDP_PROFILE, "hdp");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_PBAP_PCE, "pbap_pce");
         BT_MW_GAP_CASE_RETURN_STR(LINUXBT_GAP_PROTOCOL_ATT, "gatt");
         default: return "unknown";
    }
}

static CHAR* linuxbt_gap_get_property_type_str(bt_property_type_t type)
{
     switch(type)
     {
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_BDNAME, "name");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_BDADDR, "addr");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_UUIDS, "uuids");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_CLASS_OF_DEVICE, "cod");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_TYPE_OF_DEVICE, "tod");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_SERVICE_RECORD, "service_record");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_ADAPTER_SCAN_MODE, "scan_mode");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_ADAPTER_BONDED_DEVICES, "bond_dev");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_ADAPTER_DISCOVERY_TIMEOUT, "disc_timeout");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_REMOTE_FRIENDLY_NAME, "alias");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_REMOTE_RSSI, "rssi");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_REMOTE_VERSION_INFO, "version");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_LOCAL_LE_FEATURES, "le_feature");
         BT_MW_GAP_CASE_RETURN_STR(BT_PROPERTY_REMOTE_SERVICE, "service");
         default: return "unknown";
    }
}

static CHAR* linuxbt_gap_get_dev_type_str(UINT32 type)
{
     switch(type)
     {
         BT_MW_GAP_CASE_RETURN_STR(BT_GAP_DEVICE_TYPE_BREDR, "bredr");
         BT_MW_GAP_CASE_RETURN_STR(BT_GAP_DEVICE_TYPE_BLE, "ble");
         BT_MW_GAP_CASE_RETURN_STR(BT_GAP_DEVICE_TYPE_DUMO, "dumo");
         default: return "unknown";
    }
}

static CHAR* linuxbt_gap_get_bond_state_str(UINT32 bond_state)
{
     switch(bond_state)
     {
         BT_MW_GAP_CASE_RETURN_STR(BT_BOND_STATE_NONE, "none");
         BT_MW_GAP_CASE_RETURN_STR(BT_BOND_STATE_BONDING, "bonding");
         BT_MW_GAP_CASE_RETURN_STR(BT_BOND_STATE_BONDED, "bonded");
         default: return "unknown";
    }
}


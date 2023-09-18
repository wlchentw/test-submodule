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

#include <string.h>

#include "bt_target.h"
#include "device/include/controller.h"

#if (BLE_INCLUDED == TRUE)
#include "bt_types.h"
#include "hcimsgs.h"
#include "btu.h"
#include "btm_int.h"
#include "bt_utils.h"
#include "hcidefs.h"
#include "btm_ble_api.h"

/************************************************************************************
**  Constants & Macros
************************************************************************************/
/* length of each multi adv sub command */
#define BTM_BLE_MULTI_ADV_ENB_LEN                       3
#define BTM_BLE_MULTI_ADV_SET_PARAM_LEN                 24
#define BTM_BLE_MULTI_ADV_WRITE_DATA_LEN                (BTM_BLE_AD_DATA_LEN + 3)
#define BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR_LEN           8

#define BTM_BLE_MULTI_ADV_CB_EVT_MASK   0xF0
#define BTM_BLE_MULTI_ADV_SUBCODE_MASK  0x0F

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
const int  NUM_OF_ADVERTISER = 1;
UINT16 g_adv_data_len;
#endif
/************************************************************************************
**  Static variables
************************************************************************************/
tBTM_BLE_MULTI_ADV_CB  btm_multi_adv_cb;
tBTM_BLE_MULTI_ADV_INST_IDX_Q btm_multi_adv_idx_q;

/************************************************************************************
**  Externs
************************************************************************************/
extern fixed_queue_t *btu_general_alarm_queue;
extern void btm_ble_update_dmt_flag_bits(UINT8 *flag_value,
                                               const UINT16 connect_mode, const UINT16 disc_mode);

/*******************************************************************************
**
** Function         btm_ble_multi_adv_enq_op_q
**
** Description      enqueue a multi adv operation in q to check command complete
**                  status.
**
** Returns          void
**
*******************************************************************************/
void btm_ble_multi_adv_enq_op_q(UINT8 opcode, UINT8 inst_id, UINT8 cb_evt)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_BLE_MULTI_ADV_OPQ  *p_op_q = &btm_multi_adv_cb.op_q;

    p_op_q->p_inst_id[p_op_q->next_idx] = inst_id;

    p_op_q->p_sub_code[p_op_q->next_idx] = (opcode |(cb_evt << 4));

    p_op_q->next_idx = (p_op_q->next_idx + 1) %  BTM_BleMaxMultiAdvInstanceCount();
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_deq_op_q
**
** Description      dequeue a multi adv operation from q when command complete
**                  is received.
**
** Returns          void
**
*******************************************************************************/
void btm_ble_multi_adv_deq_op_q(UINT8 *p_opcode, UINT8 *p_inst_id, UINT8 *p_cb_evt)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_BLE_MULTI_ADV_OPQ  *p_op_q = &btm_multi_adv_cb.op_q;

    *p_inst_id = p_op_q->p_inst_id[p_op_q->pending_idx] & 0x7F;
    *p_cb_evt = (p_op_q->p_sub_code[p_op_q->pending_idx] >> 4);
    *p_opcode = (p_op_q->p_sub_code[p_op_q->pending_idx] & BTM_BLE_MULTI_ADV_SUBCODE_MASK);

    p_op_q->pending_idx = (p_op_q->pending_idx + 1) %  BTM_BleMaxMultiAdvInstanceCount();
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_vsc_cmpl_cback
**
** Description      Multi adv VSC complete callback
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
void btm_ble_multi_adv_vsc_cmpl_cback (tBTM_VSC_CMPL *p_params)
{
    UINT8  status, subcode;
    UINT8  *p = p_params->p_param_buf, inst_id;
    UINT16  len = p_params->param_len;
    tBTM_BLE_MULTI_ADV_INST *p_inst ;
    UINT8   cb_evt = 0, opcode;

    if (len  < 2)
    {
        BTM_TRACE_ERROR("wrong length for btm_ble_multi_adv_vsc_cmpl_cback");
        return;
    }

    STREAM_TO_UINT8(status, p);
    STREAM_TO_UINT8(subcode, p);

    btm_ble_multi_adv_deq_op_q(&opcode, &inst_id, &cb_evt);

    BTM_TRACE_DEBUG("op_code = %02x inst_id = %d cb_evt = %02x", opcode, inst_id, cb_evt);

    if (opcode != subcode || inst_id == 0)
    {
        BTM_TRACE_ERROR("get unexpected VSC cmpl, expect: %d get: %d",subcode,opcode);
        return;
    }

    p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    switch (subcode)
    {
        case BTM_BLE_MULTI_ADV_ENB:
        {
            BTM_TRACE_DEBUG("BTM_BLE_MULTI_ADV_ENB status = %d", status);

            /* Mark as not in use here, if instance cannot be enabled */
            if (HCI_SUCCESS != status && BTM_BLE_MULTI_ADV_ENB_EVT == cb_evt)
                btm_multi_adv_cb.p_adv_inst[inst_id-1].in_use = FALSE;
            break;
        }

        case BTM_BLE_MULTI_ADV_SET_PARAM:
        {
            BTM_TRACE_DEBUG("BTM_BLE_MULTI_ADV_SET_PARAM status = %d", status);
            break;
        }

        case BTM_BLE_MULTI_ADV_WRITE_ADV_DATA:
        {
            BTM_TRACE_DEBUG("BTM_BLE_MULTI_ADV_WRITE_ADV_DATA status = %d", status);
            break;
        }

        case BTM_BLE_MULTI_ADV_WRITE_SCAN_RSP_DATA:
        {
            BTM_TRACE_DEBUG("BTM_BLE_MULTI_ADV_WRITE_SCAN_RSP_DATA status = %d", status);
            break;
        }

        case BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR:
        {
            BTM_TRACE_DEBUG("BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR status = %d", status);
            break;
        }

        default:
            break;
    }

    if (cb_evt != 0 && p_inst->p_cback != NULL)
    {
        (p_inst->p_cback)(cb_evt, inst_id, p_inst->p_ref, status);
    }
    return;
}


/*******************************************************************************
**
** Function         btm_ble_multi_adv_ext_cmpl_cback
**
** Description      Extended and Periodic Multi adv complete callback
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
void btm_ble_multi_adv_ext_cmpl_cback (tBTM_EXT_CMPL *p_params)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    UINT8  status;
    UINT8  *p = p_params->p_param_buf, inst_id;
    UINT16  len = p_params->param_len;
    tBTM_BLE_MULTI_ADV_INST *p_inst;
    UINT8   cb_evt = 0, opcode;

    if (len < 1)
    {
        BTM_TRACE_ERROR("wrong length for btm_ble_multi_adv_ext_cmpl_cback");
        return;
    }

    btm_ble_multi_adv_deq_op_q(&opcode, &inst_id, &cb_evt);
    BTM_TRACE_DEBUG("op_code = %02x inst_id = %d cb_evt = %02x", opcode, inst_id, cb_evt);
    if (inst_id == 0)
    {
        BTM_TRACE_ERROR("get unexpected EXT cmpl, inst_id is 0");
        return;
    }

    p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
    STREAM_TO_UINT8(status, p);

    switch (p_params->opcode)
    {
        case HCI_LE_SET_EXT_ADVERTISING_ENABLE:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_EXT_ADVERTISING_ENABLE status = %d", status);

            /* Mark as not in use here, if instance cannot be enabled */
            if (HCI_SUCCESS != status && BTM_BLE_MULTI_ADV_ENB_EVT == cb_evt)
                btm_multi_adv_cb.p_adv_inst[inst_id-1].in_use = FALSE;
            break;
        }

        case HCI_LE_SET_EXT_ADVERTISING_PARAM:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_EXT_ADVERTISING_PARAM status = %d", status);
            break;
        }

        case HCI_LE_SET_EXT_ADVERTISING_DATA:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_EXT_ADVERTISING_DATA status = %d", status);
            break;
        }

        case HCI_LE_SET_EXT_ADVERTISING_SCAN_RESP:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_EXT_ADVERTISING_SCAN_RESP status = %d", status);
            break;
        }

        case HCI_LE_SET_EXT_ADVERTISING_RANDOM_ADDRESS:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_EXT_ADVERTISING_RANDOM_ADDRESS status = %d", status);
            break;
        }

        case HCI_LE_SET_PERIODIC_ADVERTISING_PARAM:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_PERIODIC_ADVERTISING_PARAM status = %d", status);
            break;
        }

        case HCI_LE_SET_PERIODIC_ADVERTISING_DATA:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_PERIODIC_ADVERTISING_DATA status = %d", status);
            break;
        }

        case HCI_LE_SET_PERIODIC_ADVERTISING_ENABLE:
        {
            BTM_TRACE_DEBUG("HCI_LE_SET_PERIODIC_ADVERTISING_ENABLE status = %d", status);
            if (HCI_SUCCESS != status && BTM_BLE_MULTI_ADV_ENB_EVT == cb_evt)
            {
                btm_multi_adv_cb.p_adv_inst[inst_id-1].in_use = FALSE;
            }
            break;
        }

        default:
            break;
    }

    if (cb_evt != 0 && p_inst->p_cback != NULL)
    {
        (p_inst->p_cback)(cb_evt, inst_id, p_inst->p_ref, status);
    }
    return;
}
#endif

/*******************************************************************************
**
** Function         btm_ble_enable_multi_adv
**
** Description      This function enable the customer specific feature in controller
**
** Parameters       enable: enable or disable
**                  inst_id:    adv instance ID, can not be 0
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_enable_multi_adv (BOOLEAN enable, UINT8 inst_id, UINT8 cb_evt)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTM_TRACE_DEBUG("Enable extended multi advertising");
        tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
        tBTM_STATUS     rt;
        /* cmd_length = header_size + num_of_of_advertiser * size_per_advertiser */
        const UINT16 cmd_length = 2 + NUM_OF_ADVERTISER * 4;
        UINT8 param[cmd_length];
        memset(param, 0, cmd_length);

        if (cb_evt == BTM_BLE_MULTI_ADV_ENB_EVT)
        {
            cb_evt = BTM_BLE_EXT_ADV_ENB_EVT;
        }
        if (cb_evt == BTM_BLE_MULTI_ADV_DISABLE_EVT)
        {
            cb_evt = BTM_BLE_EXT_ADV_DISABLE_EVT;
        }

        UINT8* pp = param;
        UINT8_TO_STREAM(pp, enable);

        UINT8_TO_STREAM(pp, NUM_OF_ADVERTISER);
        UINT8_TO_STREAM(pp, inst_id);
        UINT16_TO_STREAM(pp, p_inst->duration);
        /*default max_extended_advertising_events*/
        UINT8_TO_STREAM(pp, p_inst->maxExtAdvEvents);

        if ((rt = BTM_ExtendedCommand(HCI_LE_SET_EXT_ADVERTISING_ENABLE,
                                        cmd_length,
                                        param,
                                        btm_ble_multi_adv_ext_cmpl_cback))
                                         == BTM_CMD_STARTED)
        {
            btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_ENB, inst_id, cb_evt);
        }
        return rt;
    }
#endif /*MTK_BLE_BT_5_0==TRUE*/

    UINT8           param[BTM_BLE_MULTI_ADV_ENB_LEN], *pp;
    UINT8           enb = enable ? 1: 0;
    tBTM_STATUS     rt;

    pp = param;
    memset(param, 0, BTM_BLE_MULTI_ADV_ENB_LEN);

    UINT8_TO_STREAM (pp, BTM_BLE_MULTI_ADV_ENB);
    UINT8_TO_STREAM (pp, enb);
    UINT8_TO_STREAM (pp, inst_id);

    BTM_TRACE_EVENT (" btm_ble_enable_multi_adv: enb %d, Inst ID %d",enb,inst_id);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    BTM_BLE_MULTI_ADV_ENB_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_MULTI_ADV_ENB, inst_id, cb_evt);
    }
    return rt;
}
/*******************************************************************************
**
** Function         btm_ble_map_adv_tx_power
**
** Description      return the actual power in dBm based on the mapping in config file
**
** Parameters       advertise parameters used for this instance.
**
** Returns          tx power in dBm
**
*******************************************************************************/
int btm_ble_tx_power[BTM_BLE_ADV_TX_POWER_MAX + 1] = BTM_BLE_ADV_TX_POWER;
char btm_ble_map_adv_tx_power(int tx_power_index)
{
    if(0 <= tx_power_index && tx_power_index <= BTM_BLE_ADV_TX_POWER_MAX)
        return (char)btm_ble_tx_power[tx_power_index];
    return 0;
}

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
tBTM_STATUS SetExtendedAdvertisingData(UINT8 handle, UINT8 operation,
                                  UINT8 fragment_preference,
                                  UINT8 data_length, UINT8* data)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_STATUS rt;
    const UINT16 cmd_length = 4 + data_length;
    UINT8 param[cmd_length];
    memset(param, 0, cmd_length);

    UINT8* pp = param;
    UINT8_TO_STREAM(pp, handle);
    UINT8_TO_STREAM(pp, operation);
    UINT8_TO_STREAM(pp, fragment_preference);
    UINT8_TO_STREAM(pp, data_length);
    ARRAY_TO_STREAM(pp, data, data_length);

    if ((rt = BTM_ExtendedCommand(HCI_LE_SET_EXT_ADVERTISING_DATA,
                                    cmd_length,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA, handle, BTM_BLE_MULTI_ADV_DATA_EVT);
    }
    return rt;
}

tBTM_STATUS SetExtendedScanResponseData(UINT8 handle, UINT8 operation,
                                   UINT8 fragment_preference,
                                   UINT8 scan_response_data_length,
                                   UINT8* scan_response_data)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_STATUS rt;
    const UINT16 cmd_length = 4 + scan_response_data_length;
    UINT8 param[cmd_length];
    memset(param, 0, cmd_length);

    UINT8* pp = param;
    UINT8_TO_STREAM(pp, handle);
    UINT8_TO_STREAM(pp, operation);
    UINT8_TO_STREAM(pp, fragment_preference);
    UINT8_TO_STREAM(pp, scan_response_data_length);
    ARRAY_TO_STREAM(pp, scan_response_data, scan_response_data_length);

    if ((rt = BTM_ExtendedCommand(HCI_LE_SET_EXT_ADVERTISING_SCAN_RESP,
                                    cmd_length,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA, handle, BTM_BLE_MULTI_ADV_DATA_EVT);
    }
    return rt;
}

tBTM_STATUS SetPeriodicAdvertisingData(UINT8 inst_id, UINT8 operation,
                                  UINT8 adv_data_length, UINT8* adv_data)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_STATUS rt;
    const UINT16 HCI_LE_SET_PRIODIC_ADVERTISING_DATA_LEN =
        3 + adv_data_length;
    uint8_t param[HCI_LE_SET_PRIODIC_ADVERTISING_DATA_LEN];
    memset(param, 0, HCI_LE_SET_PRIODIC_ADVERTISING_DATA_LEN);
    uint8_t* pp = param;
    UINT8_TO_STREAM(pp, inst_id);
    UINT8_TO_STREAM(pp, operation);
    UINT8_TO_STREAM(pp, adv_data_length);
    ARRAY_TO_STREAM(pp, adv_data, adv_data_length);

    if ((rt = BTM_ExtendedCommand(HCI_LE_SET_PERIODIC_ADVERTISING_DATA,
                                    HCI_LE_SET_PRIODIC_ADVERTISING_DATA_LEN,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_PERIODIC_MULTI_ADV_WRITE_ADV_DATA, inst_id, BTM_BLE_ADV_SET_PERI_DATA_EVT);
    }
    return rt;
}

tBTM_STATUS SetPeriodicAdvertisingParameters(UINT8 inst_id,
                                        UINT16 periodic_adv_int_min,
                                        UINT16 periodic_adv_int_max,
                                        UINT16 periodic_properties)
{
    tBTM_STATUS rt;
    const UINT16 HCI_LE_SET_PRIODIC_ADVERTISING_PARAM_LEN = 7;
    UINT8 param[HCI_LE_SET_PRIODIC_ADVERTISING_PARAM_LEN];
    memset(param, 0, HCI_LE_SET_PRIODIC_ADVERTISING_PARAM_LEN);

    UINT8* pp = param;
    UINT8_TO_STREAM(pp, inst_id);
    UINT16_TO_STREAM(pp, periodic_adv_int_min);
    UINT16_TO_STREAM(pp, periodic_adv_int_max);
    UINT16_TO_STREAM(pp, periodic_properties);

    if ((rt = BTM_ExtendedCommand(HCI_LE_SET_PERIODIC_ADVERTISING_PARAM,
                                    HCI_LE_SET_PRIODIC_ADVERTISING_PARAM_LEN,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_PERIODIC_MULTI_ADV_SET_PARAM, inst_id, BTM_BLE_ADV_SET_PERI_PARAM_EVT);
    }
    return rt;
}

tBTM_STATUS SetPeriodicAdvertisingEnable(UINT8 enable, UINT8 inst_id, UINT8 cb_evt)
{
    tBTM_STATUS rt;
    const UINT16 HCI_LE_ENABLE_PRIODIC_ADVERTISEMENT_LEN = 2;
    UINT8 param[HCI_LE_ENABLE_PRIODIC_ADVERTISEMENT_LEN];
    memset(param, 0, HCI_LE_ENABLE_PRIODIC_ADVERTISEMENT_LEN);
    UINT8* pp = param;
    UINT8_TO_STREAM(pp, enable);
    UINT8_TO_STREAM(pp, inst_id);

    if ((rt = BTM_ExtendedCommand(HCI_LE_SET_PERIODIC_ADVERTISING_ENABLE,
                                    HCI_LE_ENABLE_PRIODIC_ADVERTISEMENT_LEN,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(BTM_BLE_PERIODIC_MULTI_ADV_ENB, inst_id, cb_evt);
    }
    BTM_TRACE_DEBUG("BTM: debug %s:", __func__);
    return rt;
}

tBTM_STATUS SetDataAdvDataSender(UINT8 evt_type, UINT8 inst_id,
                            UINT8 operation, UINT8 length, UINT8* data)
{
    BTM_TRACE_DEBUG("%s begin, evt_type=%d, inst_id=%d, operation=0X%02x, length=%d", __FUNCTION__, evt_type, inst_id, operation, length);
    tBTM_STATUS rt;
    switch (evt_type)
    {
        case BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA:
        {
            rt = SetExtendedAdvertisingData(inst_id, operation, 0x01, length, data);
            break;
        }
        case BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA:
        {
            rt = SetExtendedScanResponseData(inst_id, operation, 0x01, length, data);
            break;
        }
        case BTM_BLE_PERIODIC_MULTI_ADV_WRITE_ADV_DATA:
        {
            rt = SetPeriodicAdvertisingData(inst_id, operation, length, data);
            break;
        }
        default:
            rt = BTM_ILLEGAL_VALUE;
            break;

    }
    return rt;
}

static void DivideAndSendDataRecursively(UINT8 evt_type, BOOLEAN isFirst,
                                         UINT8 inst_id, UINT8* data, UINT32 len, UINT32 offset)
{
    BTM_TRACE_DEBUG("%s begin, isFirst=%d, len=%d, offset=%d", __FUNCTION__, isFirst, len, offset);

    const UINT8 INTERMEDIATE = 0x00;  // Intermediate fragment of fragmented data
    const UINT8 FIRST = 0x01;  // First fragment of fragmented data
    const UINT8 LAST = 0x02;   // Last fragment of fragmented data
    const UINT8 COMPLETE = 0x03;  // Complete extended advertising data

    UINT32 dataSize = len;
    if (!isFirst && offset == dataSize)
    {
      /* if we got error writing data, or reached the end of data */
      BTM_TRACE_DEBUG("%s, reach end, isFirst=%d, offset=%d, dataSize=%d", __FUNCTION__, isFirst,
          offset, dataSize);
      return;
    }

    BOOLEAN moreThanOnePacket = dataSize - offset > BTM_BLE_EXT_ADV_DATA_LEN;
    UINT8 operation = isFirst ? moreThanOnePacket ? FIRST : COMPLETE
                                : moreThanOnePacket ? INTERMEDIATE : LAST;
    UINT8 length = moreThanOnePacket ? BTM_BLE_EXT_ADV_DATA_LEN : dataSize - offset;
    UINT32 newOffset = offset + length;
    BTM_TRACE_DEBUG("%s newOffset=%d", __FUNCTION__, newOffset);

    SetDataAdvDataSender(evt_type, inst_id, operation, length, data + offset);

    DivideAndSendDataRecursively(evt_type, false, inst_id, data, len, newOffset);
}

tBTM_STATUS DivideAndSendData(UINT8 evt_type, UINT8 inst_id, UINT8* data, UINT32 len)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    DivideAndSendDataRecursively(evt_type, true, inst_id, data, len, 0);
    return BTM_SUCCESS;
}

/*******************************************************************************
**
** Function         btm_ble_extended_multi_adv_set_data
**
** Description      This function configure a Extended Multi-ADV instance with the specified
**                  adv data or scan response data.
**
** Parameters       is_scan_rsp: is this scan response. if no, set as adv data.
**                  p_data: pointer to the adv data structure.
**                  len:    data len of the adv data
**
** Returns          status
**
*******************************************************************************/
EXPORT_SYMBOL tBTM_STATUS btm_ble_extended_adv_set_data(UINT32 client_if, BOOLEAN is_scan_rsp,
                                                        tBTM_BLE_AD_MASK data_mask,
                                                        tBTM_BLE_ADV_DATA *p_data)
{
    BTM_TRACE_EVENT("%s", __FUNCTION__);
    UINT8   data[BTM_BLE_EXT_ADV_DATA_LEN] = {0};
    UINT8  *pp = data;
    UINT8 inst_id = 0;
    UINT8 i = 0;
    BOOLEAN find_inst = FALSE;

    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];

    BTM_TRACE_EVENT("%s", __FUNCTION__);

    if (is_scan_rsp == TRUE)
    {
        if (BTM_BleMaxMultiAdvInstanceCount() == 0)
            return BTM_NO_RESOURCES;
        for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i++, p_inst++)
        {
            if (TRUE == p_inst->in_use && (*(UINT8 *)(p_inst->p_ref)) == (UINT8)client_if)
            {
                inst_id = p_inst->inst_id;
                break;
            }
        }

        p_inst->adv_data_scan_rsp.is_scan_rsp = is_scan_rsp;
        p_inst->adv_data_scan_rsp.data_mask = data_mask;
        memcpy(&(p_inst->adv_data_scan_rsp.adv_data), p_data, sizeof(tBTM_BLE_ADV_DATA));
        btm_ble_build_adv_data(&(p_inst->adv_data_scan_rsp.data_mask), &pp, &(p_inst->adv_data_scan_rsp.adv_data));
        p_inst->adv_data_scan_rsp.adv_data_len = g_adv_data_len;

        BTM_TRACE_EVENT("%s ext_adv_data_scan_rsp.adv_data_len = %d", __FUNCTION__, p_inst->adv_data_scan_rsp.adv_data_len);
        SetExtendedScanResponseData(inst_id, 0x03, 0x01, p_inst->adv_data_scan_rsp.adv_data_len, data);
    }
    else
    {
        if (BTM_BleMaxMultiAdvInstanceCount() == 0)
            return BTM_NO_RESOURCES;
        for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i++, p_inst++)
        {
            if (TRUE == p_inst->in_use && (*(UINT8 *)(p_inst->p_ref)) == (UINT8)client_if)
            {
                inst_id = p_inst->inst_id;
                find_inst = TRUE;
                break;
            }
        }
        if (find_inst == FALSE)
        {
            p_inst = &btm_multi_adv_cb.p_adv_inst[0];
            for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
            {
                if (FALSE == p_inst->in_use)
                {
                    inst_id = p_inst->inst_id;
                    break;
                }
            }
        }
        BTM_TRACE_EVENT("%s set data inst_id = %d", __FUNCTION__, inst_id);
        p_inst->adv_data_no_scan_rsp.is_scan_rsp = is_scan_rsp;
        p_inst->adv_data_no_scan_rsp.data_mask = data_mask;
        memcpy(&(p_inst->adv_data_no_scan_rsp.adv_data), p_data, sizeof(tBTM_BLE_ADV_DATA));
        btm_ble_build_adv_data(&(p_inst->adv_data_no_scan_rsp.data_mask), &pp, &(p_inst->adv_data_no_scan_rsp.adv_data));
        p_inst->adv_data_no_scan_rsp.adv_data_len = g_adv_data_len;

        BTM_TRACE_EVENT("%s ext_adv_data_scan_rsp.adv_data_len = %d", __FUNCTION__, p_inst->adv_data_no_scan_rsp.adv_data_len);
        SetExtendedAdvertisingData(inst_id, 0x03, 0x01, p_inst->adv_data_no_scan_rsp.adv_data_len, data);
    }
#if 0
    UINT8  *pp = ext_adv_data.adv_data;

    tBTM_STATUS rt;
    btm_ble_update_dmt_flag_bits(&p_data->flag, btm_cb.btm_inq_vars.connectable_mode,
                                 btm_cb.btm_inq_vars.discoverable_mode);
    memset(ext_adv_data.adv_data, 0, BTM_BLE_EXT_ADV_DATA_LEN);

    btm_ble_build_adv_data(&data_mask, &pp, p_data);

    ext_adv_data.is_scan_rsp = is_scan_rsp;
    ext_adv_data.adv_len = sizeof(pp)/sizeof(UINT8);
    BTM_TRACE_DEBUG("ext_adv_data: is_scan_rsp=%d, adv_len:%d", ext_adv_data.is_scan_rsp, ext_adv_data.adv_len);

    BTM_TRACE_DEBUG("is_scan_rsp = %d", is_scan_rsp);
    if (is_scan_rsp)
    {
        rt = DivideAndSendData(BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA, inst_id, param, BTM_BLE_EXT_ADV_DATA_LEN);
    }
    else
    {
        rt = DivideAndSendData(BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA, inst_id, param, BTM_BLE_EXT_ADV_DATA_LEN);
    }
#endif
    return BTM_SUCCESS;
}

BOOLEAN is_connectable(UINT16 adv_event_properties)
{
    return adv_event_properties & BLE_EXT_ADV_CONNECTABLE;
}

tBTM_STATUS SetMultiAdvertisingData (UINT8 inst_id, BOOLEAN is_scan_rsp, UINT8 *p_data, UINT8 len)
{
    UINT8       param[BTM_BLE_MULTI_ADV_WRITE_DATA_LEN], *pp = param;
    UINT8       sub_code = (is_scan_rsp) ?
                           BTM_BLE_MULTI_ADV_WRITE_SCAN_RSP_DATA : BTM_BLE_MULTI_ADV_WRITE_ADV_DATA;
    UINT8       *p_len;
    tBTM_STATUS rt;
    UINT8 *pp_temp = (UINT8*)(param + BTM_BLE_MULTI_ADV_WRITE_DATA_LEN -1);
    tBTM_BLE_VSC_CB cmn_ble_vsc_cb;
    UINT8 i =0;

    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    BTM_BleGetVendorCapabilities(&cmn_ble_vsc_cb);
    if (0 == cmn_ble_vsc_cb.adv_inst_max)
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    BTM_TRACE_EVENT("SetMultiAdvertisingData called with inst_id:%d", inst_id);
    if (inst_id > BTM_BLE_MULTI_ADV_MAX || inst_id == BTM_BLE_MULTI_ADV_DEFAULT_STD)
        return BTM_ILLEGAL_VALUE;

    memset(param, 0, BTM_BLE_MULTI_ADV_WRITE_DATA_LEN);

    /*sub code*/
    UINT8_TO_STREAM(pp, sub_code);
    p_len = pp ++;
    if (!is_scan_rsp && is_connectable(p_inst->advertising_event_properties))
    {
        UINT8 flags_val = BTM_GENERAL_DISCOVERABLE;
        if (p_inst->duration)
        {
            flags_val = BTM_LIMITED_DISCOVERABLE;
        }
        UINT8_TO_STREAM(pp, 2); //length
        UINT8_TO_STREAM(pp, BTM_BLE_AD_TYPE_FLAG);
        UINT8_TO_STREAM(pp, flags_val);
    }

    for ( i = 0; i < len; i++)
    {
        UINT8 type = p_data[i + 1];
        if (type == HCI_EIR_TX_POWER_LEVEL_TYPE)
        {
            p_data[i + 2] =p_inst->tx_power;
        }
    }
    //btm_ble_build_adv_data(&data_mask, &pp, p_data);
    ARRAY_TO_STREAM(pp, p_data, len);
    *p_len = (UINT8)(pp - param - 2);
    UINT8_TO_STREAM(pp_temp, inst_id);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    (UINT8)BTM_BLE_MULTI_ADV_WRITE_DATA_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(sub_code, inst_id, BTM_BLE_MULTI_ADV_DATA_EVT);
    }
    return rt;
}

tBTM_STATUS RemoveAdvertisingSet(UINT8 handle)
{
    BTM_TRACE_DEBUG("%s", __FUNCTION__);
    tBTM_STATUS rt;
    UINT8 param = 0;
    UINT8* pp = &param;
    UINT8_TO_STREAM(pp, handle)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        if ((rt = BTM_ExtendedCommand(HCI_LE_REMOVE_ADVERTISING_SET,
                                    1,
                                    &param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
                                     == BTM_CMD_STARTED)
        {
            btm_ble_multi_adv_enq_op_q(BTM_BLE_REMOVE_ADVERTISING_SET, handle,
                                       BTM_BLE_REMOVE_ADVERTISING_SET_EVT);
        }
        return rt;
    }
}

#endif

/*******************************************************************************
**
** Function         btm_ble_multi_adv_set_params
**
** Description      This function enable the customer specific feature in controller
**
** Parameters       advertise parameters used for this instance.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_multi_adv_set_params (tBTM_BLE_MULTI_ADV_INST *p_inst,
                                          tBTM_BLE_ADV_PARAMS *p_params,
                                          UINT8 cb_evt)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTM_TRACE_DEBUG("Set extended multi advertise param");
        tBTM_STATUS  rt;
        const UINT16 HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN = 25;
        const UINT8  PEER_ADDRESS_TYPE = 0x00;
        const UINT8  SECONDARY_MAX_SKIP = 0x01;  //0x00
        const UINT8  ADVERTISING_SID = 0x01;
        UINT8 param[HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN];
        memset(param, 0, HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN);

        // TODO: disable only if was enabled, currently no use scenario needs
        // that,
        // we always set parameters before enabling
        // GetHciInterface()->Enable(false, inst_id, Bind(DoNothing));
        p_inst->advertising_event_properties =
            p_params->advertising_event_properties;
        p_inst->tx_power = p_params->tx_power;
        p_inst->advertising_interval = p_params->adv_int_min;

        uint8_t* pp = param;
        UINT8_TO_STREAM(pp, p_inst->inst_id);
        UINT16_TO_STREAM(pp, p_params->advertising_event_properties);
        UINT24_TO_STREAM(pp, p_params->adv_int_min);
        UINT24_TO_STREAM(pp, p_params->adv_int_max);

        if (p_params->channel_map == 0 || p_params->channel_map > BTM_BLE_DEFAULT_ADV_CHNL_MAP)
             p_params->channel_map = BTM_BLE_DEFAULT_ADV_CHNL_MAP;
        UINT8_TO_STREAM(pp, p_params->channel_map);

        /*own_address_type*/
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
        if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
        {
            UINT8_TO_STREAM(pp, BLE_ADDR_RANDOM);
        }
        else
#endif
        {
            UINT8_TO_STREAM(pp, BLE_ADDR_PUBLIC);
        }

        BTM_TRACE_EVENT ("%s:Adv Min %d, Adv Max %d,adv_type %d",
             __FUNCTION__,
             p_params->adv_int_min,p_params->adv_int_max,p_params->adv_type);

        UINT8_TO_STREAM(pp, PEER_ADDRESS_TYPE);
        /*peer_address*/
        BDADDR_TO_STREAM(pp, bd_addr_null);

        if (p_params->adv_filter_policy >= AP_SCAN_CONN_POLICY_MAX)
             p_params->adv_filter_policy = AP_SCAN_CONN_ALL;
        UINT8_TO_STREAM(pp, p_params->adv_filter_policy);

        if (p_params->tx_power > BTM_BLE_ADV_TX_POWER_MAX)
             p_params->tx_power = BTM_BLE_ADV_TX_POWER_MAX;
        INT8_TO_STREAM(pp, btm_ble_map_adv_tx_power(p_params->tx_power));

        BTM_TRACE_EVENT("%s:Chnl Map %d, Adv_fltr policy %d, Inst id:%d, TX Power level:%d",
             __FUNCTION__,
             p_params->channel_map, p_params->adv_filter_policy,
             p_inst->inst_id, p_params->tx_power);

        UINT8_TO_STREAM(pp, p_params->primary_advertising_phy);
        UINT8_TO_STREAM(pp, SECONDARY_MAX_SKIP);
        UINT8_TO_STREAM(pp, p_params->secondary_advertising_phy);
        UINT8_TO_STREAM(pp, ADVERTISING_SID);
        UINT8_TO_STREAM(pp, p_params->scan_request_notification_enable);

        if ((rt = BTM_ExtendedCommand(HCI_LE_SET_EXT_ADVERTISING_PARAM,
                                    HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
           == BTM_CMD_STARTED)
        {
            p_inst->adv_evt = p_params->adv_type;

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
            if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
            {
                alarm_set_on_queue(p_inst->adv_raddr_timer,
                                   BTM_BLE_PRIVATE_ADDR_INT_MS,
                                   btm_ble_adv_raddr_timer_timeout, p_inst,
                                   btu_general_alarm_queue);
            }
#endif
            btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_SET_PARAM, p_inst->inst_id, cb_evt);
        }
        return rt;
    }
#endif /*MTK_BLE_BT_5_0*/

    UINT8           param[BTM_BLE_MULTI_ADV_SET_PARAM_LEN], *pp;
    tBTM_STATUS     rt;
    BD_ADDR         dummy ={0,0,0,0,0,0};

    pp = param;
    memset(param, 0, BTM_BLE_MULTI_ADV_SET_PARAM_LEN);

    UINT8_TO_STREAM(pp, BTM_BLE_MULTI_ADV_SET_PARAM);

    UINT16_TO_STREAM (pp, p_params->adv_int_min);
    UINT16_TO_STREAM (pp, p_params->adv_int_max);
    UINT8_TO_STREAM  (pp, p_params->adv_type);

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
    if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
    {
        UINT8_TO_STREAM  (pp, BLE_ADDR_RANDOM);
        BDADDR_TO_STREAM (pp, p_inst->rpa);
    }
    else
#endif
    {
        UINT8_TO_STREAM  (pp, BLE_ADDR_PUBLIC);
        BDADDR_TO_STREAM (pp, controller_get_interface()->get_address()->address);
    }

    BTM_TRACE_EVENT (" btm_ble_multi_adv_set_params,Min %d, Max %d,adv_type %d",
        p_params->adv_int_min,p_params->adv_int_max,p_params->adv_type);

    UINT8_TO_STREAM  (pp, 0);
    BDADDR_TO_STREAM (pp, dummy);

    if (p_params->channel_map == 0 || p_params->channel_map > BTM_BLE_DEFAULT_ADV_CHNL_MAP)
        p_params->channel_map = BTM_BLE_DEFAULT_ADV_CHNL_MAP;
    UINT8_TO_STREAM (pp, p_params->channel_map);

    if (p_params->adv_filter_policy >= AP_SCAN_CONN_POLICY_MAX)
        p_params->adv_filter_policy = AP_SCAN_CONN_ALL;
    UINT8_TO_STREAM (pp, p_params->adv_filter_policy);

    UINT8_TO_STREAM (pp, p_inst->inst_id);

    if (p_params->tx_power > BTM_BLE_ADV_TX_POWER_MAX)
        p_params->tx_power = BTM_BLE_ADV_TX_POWER_MAX;
    UINT8_TO_STREAM (pp, btm_ble_map_adv_tx_power(p_params->tx_power));

    BTM_TRACE_EVENT("set_params:Chnl Map %d,adv_fltr policy %d,ID:%d, TX Power%d",
        p_params->channel_map,p_params->adv_filter_policy,p_inst->inst_id,p_params->tx_power);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    BTM_BLE_MULTI_ADV_SET_PARAM_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
           == BTM_CMD_STARTED)
    {
        p_inst->adv_evt = p_params->adv_type;

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
        if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE) {
            alarm_set_on_queue(p_inst->adv_raddr_timer,
                               BTM_BLE_PRIVATE_ADDR_INT_MS,
                               btm_ble_adv_raddr_timer_timeout, p_inst,
                               btu_general_alarm_queue);
        }
#endif
        btm_ble_multi_adv_enq_op_q(BTM_BLE_MULTI_ADV_SET_PARAM, p_inst->inst_id, cb_evt);
    }
    return rt;
}

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
/*******************************************************************************
**
** Function         btm_ble_advertising_set_params
**
** Description      This function enable the customer specific feature in controller
**
** Parameters       advertise parameters used for this instance.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_advertising_set_params (tBTM_BLE_MULTI_ADV_INST *p_inst,
                                          tBTM_BLE_ADVERTISING_PARAMS *p_params,
                                          UINT8 cb_evt)
{

    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTM_TRACE_DEBUG("Set extended multi advertise param");
        tBTM_STATUS  rt;
        const UINT16 HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN = 25;
        const UINT8  PEER_ADDRESS_TYPE = 0x00;
        const UINT8  SECONDARY_MAX_SKIP = 0x01;  //0x00
        const UINT8  ADVERTISING_SID = 0x01;
        UINT8 param[HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN];
        memset(param, 0, HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN);

        // TODO: disable only if was enabled, currently no use scenario needs
        // that,
        // we always set parameters before enabling
        // GetHciInterface()->Enable(false, inst_id, Bind(DoNothing));
        p_inst->advertising_event_properties =
            p_params->advertising_event_properties;
        p_inst->tx_power = p_params->tx_power;
        p_inst->advertising_interval = p_params->adv_int_min;

        uint8_t* pp = param;
        UINT8_TO_STREAM(pp, p_inst->inst_id);
        UINT16_TO_STREAM(pp, p_params->advertising_event_properties);
        UINT24_TO_STREAM(pp, p_params->adv_int_min);
        UINT24_TO_STREAM(pp, p_params->adv_int_max);

        if (p_params->channel_map == 0 || p_params->channel_map > BTM_BLE_DEFAULT_ADV_CHNL_MAP)
             p_params->channel_map = BTM_BLE_DEFAULT_ADV_CHNL_MAP;
        UINT8_TO_STREAM(pp, p_params->channel_map);

        /*own_address_type*/
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
        if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
        {
            UINT8_TO_STREAM(pp, BLE_ADDR_RANDOM);
        }
        else
#endif
        {
            UINT8_TO_STREAM(pp, BLE_ADDR_PUBLIC);
        }

        BTM_TRACE_EVENT ("%s:Adv Min %d, Adv Max %d,adv_type %d",
             __FUNCTION__,
             p_params->adv_int_min,p_params->adv_int_max,p_params->adv_type);

        UINT8_TO_STREAM(pp, PEER_ADDRESS_TYPE);
        /*peer_address*/
        BDADDR_TO_STREAM(pp, bd_addr_null);

        if (p_params->adv_filter_policy >= AP_SCAN_CONN_POLICY_MAX)
             p_params->adv_filter_policy = AP_SCAN_CONN_ALL;
        UINT8_TO_STREAM(pp, p_params->adv_filter_policy);

        // use app set txpower
        INT8_TO_STREAM(pp, (p_params->tx_power));

        BTM_TRACE_EVENT("%s:Chnl Map %d, Adv_fltr policy %d, Inst id:%d, TX Power level:%d",
             __FUNCTION__,
             p_params->channel_map, p_params->adv_filter_policy,
             p_inst->inst_id, p_params->tx_power);

        UINT8_TO_STREAM(pp, p_params->primary_advertising_phy);
        UINT8_TO_STREAM(pp, SECONDARY_MAX_SKIP);
        UINT8_TO_STREAM(pp, p_params->secondary_advertising_phy);
        UINT8_TO_STREAM(pp, ADVERTISING_SID);
        UINT8_TO_STREAM(pp, p_params->scan_request_notification_enable);

        if ((rt = BTM_ExtendedCommand(HCI_LE_SET_EXT_ADVERTISING_PARAM,
                                    HCI_LE_SET_EXT_ADVERTISING_PARAM_LEN,
                                    param,
                                    btm_ble_multi_adv_ext_cmpl_cback))
           == BTM_CMD_STARTED)
        {
            p_inst->adv_evt = p_params->adv_type;

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
            if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
            {
                alarm_set_on_queue(p_inst->adv_raddr_timer,
                                   BTM_BLE_PRIVATE_ADDR_INT_MS,
                                   btm_ble_adv_raddr_timer_timeout, p_inst,
                                   btu_general_alarm_queue);
            }
#endif
            btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_SET_PARAM, p_inst->inst_id, cb_evt);
        }
        return rt;
    }


    UINT8           param[BTM_BLE_MULTI_ADV_SET_PARAM_LEN], *pp;
    tBTM_STATUS     rt;
    BD_ADDR         dummy ={0,0,0,0,0,0};

    pp = param;
    memset(param, 0, BTM_BLE_MULTI_ADV_SET_PARAM_LEN);

    UINT8_TO_STREAM(pp, BTM_BLE_MULTI_ADV_SET_PARAM);

    UINT16_TO_STREAM (pp, p_params->adv_int_min);
    UINT16_TO_STREAM (pp, p_params->adv_int_max);
    UINT8_TO_STREAM  (pp, p_params->adv_type);

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
    if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
    {
        UINT8_TO_STREAM  (pp, BLE_ADDR_RANDOM);
        BDADDR_TO_STREAM (pp, p_inst->rpa);
    }
    else
#endif
    {
        UINT8_TO_STREAM  (pp, BLE_ADDR_PUBLIC);
        BDADDR_TO_STREAM (pp, controller_get_interface()->get_address()->address);
    }

    BTM_TRACE_EVENT (" btm_ble_advertising_set_params,Min %d, Max %d,adv_type %d",
        p_params->adv_int_min,p_params->adv_int_max,p_params->adv_type);

    UINT8_TO_STREAM  (pp, 0);
    BDADDR_TO_STREAM (pp, dummy);

    if (p_params->channel_map == 0 || p_params->channel_map > BTM_BLE_DEFAULT_ADV_CHNL_MAP)
        p_params->channel_map = BTM_BLE_DEFAULT_ADV_CHNL_MAP;
    UINT8_TO_STREAM (pp, p_params->channel_map);

    if (p_params->adv_filter_policy >= AP_SCAN_CONN_POLICY_MAX)
        p_params->adv_filter_policy = AP_SCAN_CONN_ALL;
    UINT8_TO_STREAM (pp, p_params->adv_filter_policy);

    UINT8_TO_STREAM (pp, p_inst->inst_id);

    // use app set tx power
    INT8_TO_STREAM (pp, p_params->tx_power);

    BTM_TRACE_EVENT("set_params:Chnl Map %d,adv_fltr policy %d,ID:%d, TX Power%d",
        p_params->channel_map,p_params->adv_filter_policy,p_inst->inst_id,p_params->tx_power);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    BTM_BLE_MULTI_ADV_SET_PARAM_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
           == BTM_CMD_STARTED)
    {
        p_inst->adv_evt = p_params->adv_type;

#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
        if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE) {
            alarm_set_on_queue(p_inst->adv_raddr_timer,
                               BTM_BLE_PRIVATE_ADDR_INT_MS,
                               btm_ble_adv_raddr_timer_timeout, p_inst,
                               btu_general_alarm_queue);
        }
#endif
        btm_ble_multi_adv_enq_op_q(BTM_BLE_MULTI_ADV_SET_PARAM, p_inst->inst_id, cb_evt);
    }
    return rt;
}

/*******************************************************************************
**
** Function         btm_ble_periodic_multi_adv_set_params
**
** Description      This function configure instance with the specified parameters
**
** Parameters       advertise parameters used for this instance.
**
** Returns          status
**
*******************************************************************************/
EXPORT_SYMBOL tBTM_STATUS btm_ble_periodic_multi_adv_set_params (tBTM_BLE_MULTI_ADV_INST *p_inst,
                                          tBTM_BLE_PERIODIC_ADV_PARAMS *p_params,
                                          UINT8 cb_evt)
{
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        return SetPeriodicAdvertisingParameters(p_inst->inst_id,
                                       p_params->min_interval,
                                       p_params->max_interval,
                                       p_params->periodic_advertising_properties);
    }
    else
    {
        return BTM_ILLEGAL_ACTION;
    }
}

/*******************************************************************************
**
** Function         btm_ble_ext_multi_adv_set_data
**
** Description      This function configure instance with the specified data
**
**
** Parameters       inst_id: adv instance ID
**                  p_data: pointer to the adv data structure.
**                  len:    data len of the adv data
**
** Returns          status
**
*******************************************************************************/
EXPORT_SYMBOL tBTM_STATUS btm_ble_ext_multi_adv_set_data(UINT8 evt_type, UINT8 inst_id, UINT8 *p_data, UINT32 len)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        /* modify correct txpower to adv data*/
        int i = 0;
        tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id -1];
        for ( i = 0; i < len; i++)
        {
            UINT8 type = p_data[i + 1];
            if (type == HCI_EIR_TX_POWER_LEVEL_TYPE)
            {
                p_data[i + 2] = p_inst->tx_power;
            }
        }
        return DivideAndSendData(evt_type, inst_id, p_data, len);
    }
    else
#endif
    {
        if (evt_type == BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA)
        {
            SetMultiAdvertisingData(inst_id, 0, p_data, len);
        }
        else if (evt_type == BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA)
        {
            SetMultiAdvertisingData(inst_id, 1, p_data, len);
        }
        else
        {
            return BTM_ILLEGAL_ACTION;
        }
    }
}

/*******************************************************************************
**
** Function         btm_ble_periodic_multi_adv_enable
**
** Description      This function enable the customer specific feature in controller
**
** Parameters       enable: enable or disable
**                  inst_id:    adv instance ID, can not be 0
**
** Returns          status
**
*******************************************************************************/
EXPORT_SYMBOL tBTM_STATUS btm_ble_periodic_multi_adv_enable (BOOLEAN enable, UINT8 inst_id, UINT8 cb_evt)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        if (!p_inst->in_use)
        {
            return BTM_ILLEGAL_ACTION;
        }
        return SetPeriodicAdvertisingEnable(enable, inst_id, cb_evt);
    }
    else
    {
        return BTM_ILLEGAL_ACTION;
    }
}
#endif


/*******************************************************************************
**
** Function         btm_ble_multi_adv_write_rpa
**
** Description      This function write the random address for the adv instance into
**                  controller
**
** Parameters
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS btm_ble_multi_adv_write_rpa (tBTM_BLE_MULTI_ADV_INST *p_inst, BD_ADDR random_addr)
{
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (controller_get_interface()->supports_ble_extended_advertising())
    {
        BTM_TRACE_DEBUG("extended multi adv write RPA");
        tBTM_STATUS rt;
        const int LE_SET_ADVERTISING_SET_RANDOM_ADDRESS_LEN = 7;
        UINT8 param[LE_SET_ADVERTISING_SET_RANDOM_ADDRESS_LEN];
        memset(param, 0, LE_SET_ADVERTISING_SET_RANDOM_ADDRESS_LEN);

        UINT8* pp = param;
        UINT8_TO_STREAM(pp, p_inst->inst_id);
        BDADDR_TO_STREAM(pp, random_addr);

        if ((rt = BTM_ExtendedCommand (HCI_LE_SET_EXT_ADVERTISING_RANDOM_ADDRESS,
                                        LE_SET_ADVERTISING_SET_RANDOM_ADDRESS_LEN,
                                        param,
                                        btm_ble_multi_adv_ext_cmpl_cback)) == BTM_CMD_STARTED)
        {
            /* start a periodical timer to refresh random addr */
            /* TODO: is the above comment correct - is the timer periodical? */
            alarm_set_on_queue(p_inst->adv_raddr_timer,
                               BTM_BLE_PRIVATE_ADDR_INT_MS,
                               btm_ble_adv_raddr_timer_timeout, p_inst,
                               btu_general_alarm_queue);
            btm_ble_multi_adv_enq_op_q(BTM_BLE_EXT_MULTI_ADV_SET_RANDOM_ADDR,
                                       p_inst->inst_id, 0);
        }
        return rt;
    }
#endif /*MTK_BLE_BT_5_0*/

    UINT8           param[BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR_LEN], *pp = param;
    tBTM_STATUS     rt;

    BTM_TRACE_EVENT ("%s-BD_ADDR:%02x-%02x-%02x-%02x-%02x-%02x,inst_id:%d",
                      __FUNCTION__, random_addr[5], random_addr[4], random_addr[3], random_addr[2],
                      random_addr[1], random_addr[0], p_inst->inst_id);

    memset(param, 0, BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR_LEN);

    UINT8_TO_STREAM (pp, BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR);
    BDADDR_TO_STREAM(pp, random_addr);
    UINT8_TO_STREAM(pp,  p_inst->inst_id);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback)) == BTM_CMD_STARTED)
    {
        /* start a periodical timer to refresh random addr */
        /* TODO: is the above comment correct - is the timer periodical? */
        alarm_set_on_queue(p_inst->adv_raddr_timer,
                           BTM_BLE_PRIVATE_ADDR_INT_MS,
                           btm_ble_adv_raddr_timer_timeout, p_inst,
                           btu_general_alarm_queue);
        btm_ble_multi_adv_enq_op_q(BTM_BLE_MULTI_ADV_SET_RANDOM_ADDR,
                                   p_inst->inst_id, 0);
    }
    return rt;
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_gen_rpa_cmpl
**
** Description      RPA generation completion callback for each adv instance. Will
**                  continue write the new RPA into controller.
**
** Returns          none.
**
*******************************************************************************/
void btm_ble_multi_adv_gen_rpa_cmpl(tBTM_RAND_ENC *p)
{
#if (SMP_INCLUDED == TRUE)
    tSMP_ENC    output;
    UINT8 index = 0;
    tBTM_BLE_MULTI_ADV_INST *p_inst = NULL;

     /* Retrieve the index of adv instance from stored Q */
    if (btm_multi_adv_idx_q.front == -1)
    {
        BTM_TRACE_ERROR(" %s can't locate advertise instance", __FUNCTION__);
        return;
    }
    else
    {
        index = btm_multi_adv_idx_q.inst_index_queue[btm_multi_adv_idx_q.front];
        if (btm_multi_adv_idx_q.front == btm_multi_adv_idx_q.rear)
        {
            btm_multi_adv_idx_q.front = -1;
            btm_multi_adv_idx_q.rear = -1;
        }
        else
        {
            btm_multi_adv_idx_q.front = (btm_multi_adv_idx_q.front + 1) % BTM_BLE_MULTI_ADV_MAX;
        }
    }

    p_inst = &(btm_multi_adv_cb.p_adv_inst[index]);

    BTM_TRACE_EVENT ("btm_ble_multi_adv_gen_rpa_cmpl inst_id = %d", p_inst->inst_id);
    if (p)
    {
        p->param_buf[2] &= (~BLE_RESOLVE_ADDR_MASK);
        p->param_buf[2] |= BLE_RESOLVE_ADDR_MSB;

        p_inst->rpa[2] = p->param_buf[0];
        p_inst->rpa[1] = p->param_buf[1];
        p_inst->rpa[0] = p->param_buf[2];
#if defined(MTK_LINUX_GAP_PTS_TEST) && (MTK_LINUX_GAP_PTS_TEST == TRUE)
        BTM_TRACE_DEBUG("btm_cb.devcb.id_keys.irk is 0x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        btm_cb.devcb.id_keys.irk[15], btm_cb.devcb.id_keys.irk[14],btm_cb.devcb.id_keys.irk[13], btm_cb.devcb.id_keys.irk[12],
                        btm_cb.devcb.id_keys.irk[11], btm_cb.devcb.id_keys.irk[10],btm_cb.devcb.id_keys.irk[9], btm_cb.devcb.id_keys.irk[8],
                        btm_cb.devcb.id_keys.irk[7], btm_cb.devcb.id_keys.irk[6],btm_cb.devcb.id_keys.irk[5], btm_cb.devcb.id_keys.irk[4],
                        btm_cb.devcb.id_keys.irk[3], btm_cb.devcb.id_keys.irk[2],btm_cb.devcb.id_keys.irk[1], btm_cb.devcb.id_keys.irk[0]);
#endif
        if (!SMP_Encrypt(btm_cb.devcb.id_keys.irk, BT_OCTET16_LEN, p->param_buf, 3, &output))
        {
            BTM_TRACE_DEBUG("generate random address failed");
        }
        else
        {
            /* set hash to be LSB of rpAddress */
            p_inst->rpa[5] = output.param_buf[0];
            p_inst->rpa[4] = output.param_buf[1];
            p_inst->rpa[3] = output.param_buf[2];
        }

        if (p_inst->inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD &&
            p_inst->inst_id < BTM_BleMaxMultiAdvInstanceCount())
        {
            /* set it to controller */
            btm_ble_multi_adv_write_rpa(p_inst, p_inst->rpa);
        }
    }
#endif
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_configure_rpa
**
** Description      This function set the random address for the adv instance
**
** Parameters       advertise parameters used for this instance.
**
** Returns          none
**
*******************************************************************************/
void btm_ble_multi_adv_configure_rpa (tBTM_BLE_MULTI_ADV_INST *p_inst)
{
    if (btm_multi_adv_idx_q.front == (btm_multi_adv_idx_q.rear + 1) % BTM_BLE_MULTI_ADV_MAX)
    {
        BTM_TRACE_ERROR("outstanding rand generation exceeded max allowed ");
        return;
    }
    else
    {
        if (btm_multi_adv_idx_q.front == -1)
        {
            btm_multi_adv_idx_q.front = 0;
            btm_multi_adv_idx_q.rear = 0;
        }
        else
        {
            btm_multi_adv_idx_q.rear = (btm_multi_adv_idx_q.rear + 1) % BTM_BLE_MULTI_ADV_MAX;
        }
        btm_multi_adv_idx_q.inst_index_queue[btm_multi_adv_idx_q.rear] = p_inst->index;
    }
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    btm_ext_gen_resolvable_private_addr((void *)btm_ble_multi_adv_gen_rpa_cmpl);
#else
    btm_gen_resolvable_private_addr((void *)btm_ble_multi_adv_gen_rpa_cmpl);
#endif
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_reenable
**
** Description      This function re-enable adv instance upon a connection establishment.
**
** Parameters       advertise parameters used for this instance.
**
** Returns          none.
**
*******************************************************************************/
void btm_ble_multi_adv_reenable(UINT8 inst_id)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    if (TRUE == p_inst->in_use)
    {
        BTM_TRACE_DEBUG("btm_ble_multi_adv_reenable p_inst->adv_evt:%d", p_inst->adv_evt);
        if (p_inst->adv_evt != BTM_BLE_CONNECT_DIR_EVT)
            btm_ble_enable_multi_adv (TRUE, p_inst->inst_id, 0);
        else
          /* mark directed adv as disabled if adv has been stopped */
        {
            (p_inst->p_cback)(BTM_BLE_MULTI_ADV_DISABLE_EVT,p_inst->inst_id,p_inst->p_ref,0);
             p_inst->in_use = FALSE;
        }
     }
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_enb_privacy
**
** Description      This function enable/disable privacy setting in multi adv
**
** Parameters       enable: enable or disable the adv instance.
**
** Returns          none.
**
*******************************************************************************/
void btm_ble_multi_adv_enb_privacy(BOOLEAN enable)
{
    UINT8 i;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];
    for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
    {
        p_inst->in_use = FALSE;
        if (enable)
            btm_ble_multi_adv_configure_rpa(p_inst);
        else
            alarm_cancel(p_inst->adv_raddr_timer);
    }
}

/*******************************************************************************
**
** Function         BTM_BleEnableAdvInstance
**
** Description      This function enable a Multi-ADV instance with the specified
**                  adv parameters
**
** Parameters       p_params: pointer to the adv parameter structure, set as default
**                            adv parameter when the instance is enabled.
**                  p_cback: callback function for the adv instance.
**                  p_ref:  reference data attach to the adv instance to be enabled.
**
** Returns          status
**
*******************************************************************************/
#ifdef MTK_MESH_SUPPORT
typedef struct
{
    UINT8   len;
    UINT8   data[BTM_BLE_ADV_DATA_LEN_MAX];
} MESH_ADV_DATA;
MESH_ADV_DATA advertising_data;

#define BTM_BLE_MESH_CLIENT_IF  17

EXPORT_SYMBOL tBTM_STATUS BTM_BleSetAdvInstData (UINT8 inst_id, BOOLEAN is_scan_rsp, UINT8 *p_data, UINT8 len)
{
    UINT8       param[BTM_BLE_MULTI_ADV_WRITE_DATA_LEN], *pp = param;
    UINT8       sub_code = (is_scan_rsp) ?
                           BTM_BLE_MULTI_ADV_WRITE_SCAN_RSP_DATA : BTM_BLE_MULTI_ADV_WRITE_ADV_DATA;
    UINT8       *p_len;
    tBTM_STATUS rt;
    UINT8 *pp_temp = (UINT8*)(param + BTM_BLE_MULTI_ADV_WRITE_DATA_LEN -1);
    tBTM_BLE_VSC_CB cmn_ble_vsc_cb;

    BTM_BleGetVendorCapabilities(&cmn_ble_vsc_cb);
    if (0 == cmn_ble_vsc_cb.adv_inst_max)
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    BTM_TRACE_EVENT("BTM_BleCfgAdvInstData called with inst_id:%d", inst_id);
    if (inst_id > BTM_BLE_MULTI_ADV_MAX || inst_id == BTM_BLE_MULTI_ADV_DEFAULT_STD)
        return BTM_ILLEGAL_VALUE;

    memset(param, 0, BTM_BLE_MULTI_ADV_WRITE_DATA_LEN);

    UINT8_TO_STREAM(pp, sub_code);
    p_len = pp ++;
    //btm_ble_build_adv_data(&data_mask, &pp, p_data);
    ARRAY_TO_STREAM(pp, p_data, len);
    *p_len = (UINT8)(pp - param - 2);
    UINT8_TO_STREAM(pp_temp, inst_id);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    (UINT8)BTM_BLE_MULTI_ADV_WRITE_DATA_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(sub_code, inst_id, BTM_BLE_MULTI_ADV_DATA_EVT);
    }
    return rt;
}

EXPORT_SYMBOL tBTM_STATUS BTM_BleGetPubAddr(BD_ADDR pub_addr)
{
    memcpy(pub_addr, controller_get_interface()->get_address()->address, BD_ADDR_LEN);
    return BTM_SUCCESS;
}

EXPORT_SYMBOL tBTM_STATUS BTM_BleSaveAdvData (UINT8 *p_data, UINT8 len)
{
    advertising_data.len = len;
    memcpy(advertising_data.data, p_data, len);
    return BTM_SUCCESS;
}

#endif

#ifdef MTK_MESH_SUPPORT
EXPORT_SYMBOL tBTM_STATUS BTM_BleEnableAdvInstance (tBTM_BLE_ADV_PARAMS *p_params,
                                      tBTM_BLE_MULTI_ADV_CBACK *p_cback,void *p_ref)
#else
tBTM_STATUS BTM_BleEnableAdvInstance (tBTM_BLE_ADV_PARAMS *p_params,
                                      tBTM_BLE_MULTI_ADV_CBACK *p_cback,void *p_ref)
#endif
{
    UINT8 i;
    tBTM_STATUS rt = BTM_NO_RESOURCES;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];

    BTM_TRACE_EVENT("BTM_BleEnableAdvInstance called");
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (0 == BTM_BleMaxMultiAdvInstanceCount())
#else
    if (0 == btm_cb.cmn_ble_vsc_cb.adv_inst_max)
#endif
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    if (NULL == p_inst)
    {
        BTM_TRACE_ERROR("Invalid instance in BTM_BleEnableAdvInstance");
        return BTM_ERR_PROCESSING;
    }

    for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
    {
        if (FALSE == p_inst->in_use)
        {
            p_inst->in_use = TRUE;
            /* configure adv parameter */
            if (p_params)
                rt = btm_ble_multi_adv_set_params(p_inst, p_params, 0);
            else
                rt = BTM_CMD_STARTED;

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
            if (controller_get_interface()->supports_ble_extended_advertising())
            {
                btm_ble_multi_adv_write_rpa(p_inst, p_inst->rpa);
            }
#endif

#ifdef MTK_MESH_SUPPORT
            if (BTM_BLE_MESH_CLIENT_IF == (*(UINT8 *)(p_ref)))
            {
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
                if (controller_get_interface()->supports_ble_extended_advertising())
                {
                    rt = SetExtendedAdvertisingData(p_inst->inst_id, 0x03, 0x01, advertising_data.len, advertising_data.data);
                }
                else
                {
                    rt = BTM_BleSetAdvInstData(p_inst->inst_id, FALSE, advertising_data.data, advertising_data.len);
                }
#else
                rt = BTM_BleSetAdvInstData(p_inst->inst_id, FALSE, advertising_data.data, advertising_data.len);
#endif
            }
#endif

            /* enable adv */
            BTM_TRACE_EVENT("btm_ble_enable_multi_adv being called with inst_id:%d",
                p_inst->inst_id);

            if (BTM_CMD_STARTED == rt)
            {
                if ((rt = btm_ble_enable_multi_adv (TRUE, p_inst->inst_id,
                          BTM_BLE_MULTI_ADV_ENB_EVT)) == BTM_CMD_STARTED)
                {
                    p_inst->p_cback = p_cback;
                    p_inst->p_ref   = p_ref;
                }
            }
            BTM_TRACE_EVENT("btm_ble_enable_multi_adv being called with p_inst->p_ref:%d",
                (*(UINT8 *)(p_inst->p_ref)));

            if (BTM_CMD_STARTED != rt)
            {
                p_inst->in_use = FALSE;
                BTM_TRACE_ERROR("BTM_BleEnableAdvInstance failed");
            }
            break;
        }
    }
    return rt;
}

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
tBTM_STATUS BTM_BleAdvertisingSetParam (UINT8 inst_id, tBTM_BLE_ADVERTISING_PARAMS *p_params)
{
    tBTM_STATUS rt = BTM_ILLEGAL_VALUE;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    BTM_TRACE_EVENT("BTM_BleAdvertisingSetParam called with inst_id:%d", inst_id);

    if (0 == BTM_BleMaxMultiAdvInstanceCount())
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    if (inst_id <  BTM_BleMaxMultiAdvInstanceCount() &&
        inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD &&
        p_params != NULL)
    {
        if (FALSE == p_inst->in_use)
        {
            BTM_TRACE_DEBUG("adv instance %d is not active", inst_id);
            return BTM_WRONG_MODE;
        }
        else
            btm_ble_enable_multi_adv(FALSE, inst_id, 0);

        if (BTM_CMD_STARTED == btm_ble_advertising_set_params(p_inst, p_params, 0))
            rt = btm_ble_enable_multi_adv(TRUE, inst_id, BTM_BLE_MULTI_ADV_PARAM_EVT);
    }
    return rt;
}

tBTM_STATUS BTM_BleStartAdvertisingSet (tBTM_BLE_ADVERTISING_SET *p_adv_info,
                       tBTM_BLE_MULTI_ADV_CBACK *p_cback, void *p_ref)
{
    UINT8 i;
    tBTM_STATUS rt = BTM_NO_RESOURCES;
    tBTM_BLE_ADVERTISING_PARAMS *p_adv_param = &p_adv_info->adv_params;
    tBTM_BLE_PERI_ADV_DATA *p_adv_data = &p_adv_info->adv_data;
    tBTM_BLE_PERI_ADV_DATA *p_adv_scan_rsp_data = &p_adv_info->adv_scan_rsp_data;
    tBTM_BLE_PERIODIC_ADV_PARAMS *p_adv_peri_param =
                                    &p_adv_info->peri_params;
    tBTM_BLE_PERI_ADV_DATA *p_peri_adv_data = &p_adv_info->peri_adv_data;
    UINT16 duration = p_adv_info->duration;
    UINT8 maxExtAdvEvents = p_adv_info->maxExtAdvEvents;

    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];
    BTM_TRACE_EVENT("BTM_BleStartAdvertisingSet called");
    if (0 == BTM_BleMaxMultiAdvInstanceCount())
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    if (NULL == p_inst)
    {
        BTM_TRACE_ERROR("Invalid instance in BTM_BleStartAdvertisingSet");
        return BTM_ERR_PROCESSING;
    }
    if (!controller_get_interface()->supports_ble_extended_advertising())
    {
        BTM_TRACE_ERROR("Controller does not support extend ADV");
        return BTM_ERR_PROCESSING;
    }
    for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
    {
        if (FALSE == p_inst->in_use)
        {
            p_inst->in_use = TRUE;
            p_inst->tx_power = p_adv_param->tx_power;
            /* 1.configure adv parameter */
            if (p_adv_param)
            {
                BTM_TRACE_EVENT("1-set adv pamram inst=%d", p_inst->inst_id);
                BTM_TRACE_EVENT("1-set adv second-phy=%d", p_adv_param->secondary_advertising_phy);
                rt = btm_ble_advertising_set_params(p_inst, p_adv_param, BTM_BLE_MULTI_ADV_PARAM_EVT);
            }
            else
            {
                rt = BTM_CMD_STARTED;
            }
            /* 2.set random address */
            BTM_TRACE_EVENT("2-set random address inst=%d", p_inst->inst_id);
            btm_ble_multi_adv_write_rpa(p_inst, p_inst->rpa);

            /*3. set adv data */
            if (p_adv_data->adv_len != 0)
            {
                /* can fragment */
                BTM_TRACE_EVENT("3-set adv data inst=%d", p_inst->inst_id);
                btm_ble_ext_multi_adv_set_data(BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA,
                    p_inst->inst_id, p_adv_data->adv_data, p_adv_data->adv_len);
            }

            /*4. set adv scan response data */
            if (p_adv_scan_rsp_data->adv_len != 0)
            {
                /*  can fragment  */
                BTM_TRACE_EVENT("4-set adv scan response data inst=%d", p_inst->inst_id);
                btm_ble_ext_multi_adv_set_data(BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA,
                    p_inst->inst_id, p_adv_scan_rsp_data->adv_data, p_adv_scan_rsp_data->adv_len);
            }

            /* 5. set periodic adv param   */
            if (p_adv_peri_param->enable)
            {
                BTM_TRACE_EVENT("5-set periodic adv param inst=%d", p_inst->inst_id);

                btm_ble_periodic_multi_adv_set_params(p_inst, p_adv_peri_param, 0);
                /* 6. set periodic adv data*/
                if (p_peri_adv_data->adv_len != 0)
                {
                    BTM_TRACE_EVENT("6-set periodic adv data inst=%d, peri_adv_len=%d",
                        p_inst->inst_id, p_peri_adv_data->adv_len);
                    btm_ble_ext_multi_adv_set_data(BTM_BLE_PERIODIC_MULTI_ADV_WRITE_ADV_DATA,
                        p_inst->inst_id, p_peri_adv_data->adv_data, p_peri_adv_data->adv_len);
                }

                /* 7. enable periodic adv  */
                BTM_TRACE_EVENT("7-btm_ble_enable_peri_adv being called with inst_id:%d",
                    p_inst->inst_id);
                rt = btm_ble_periodic_multi_adv_enable(p_adv_peri_param->enable,
                          p_inst->inst_id, BTM_BLE_ADV_PERI_ENABLE_EVT);

                BTM_TRACE_EVENT("8-btm_ble_periodic_multi_adv_enable being called with p_inst->p_ref:%d",
                                     (*(UINT8 *)(p_ref)));
                if (BTM_CMD_STARTED == rt)
                {
                    p_inst->periodic_enabled = TRUE;
                }
                else
                {
                    p_inst->in_use = FALSE;
                    BTM_TRACE_ERROR("BTM_BleStartAdvertisingSet failed");
                    return rt;
                }
            }

            p_inst->duration = duration;
            p_inst->maxExtAdvEvents = maxExtAdvEvents;
            BTM_TRACE_EVENT("9-enable_multi_adv inst_id:%d, duration=%d, maxevent=%d",
                p_inst->inst_id, p_inst->duration, p_inst->maxExtAdvEvents);
            rt = btm_ble_enable_multi_adv (TRUE, p_inst->inst_id,
                          BTM_BLE_MULTI_ADV_ENB_EVT);
            if (BTM_CMD_STARTED == rt)
            {
                p_inst->p_cback = p_cback;
                p_inst->p_ref   = p_ref;
            }
            else
            {
                p_inst->in_use = FALSE;
                BTM_TRACE_ERROR("BTM_BleEnableAdvInstance failed");
            }
            break;
        }
    }
    return rt;
}

EXPORT_SYMBOL tBTM_STATUS BTM_BlePeriodicAdvsetParams (UINT8 inst_id,
                                          tBTM_BLE_PERIODIC_ADV_PARAMS *p_params,
                                          UINT8 cb_evt)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
    return btm_ble_periodic_multi_adv_set_params(p_inst, p_params, 0);
}

EXPORT_SYMBOL tBTM_STATUS BTM_BlePeriodicAdvEnable (UINT8 inst_id, UINT8 enable)
{
    tBTM_STATUS rt = BTM_NO_RESOURCES;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
    rt = btm_ble_periodic_multi_adv_enable(enable, inst_id, BTM_BLE_ADV_PERI_ENABLE_EVT);
    if (BTM_CMD_STARTED == rt)
    {
        p_inst->periodic_enabled = TRUE;
    }
    return rt;
}

EXPORT_SYMBOL tBTM_STATUS BTM_BleExtOrMultiAdvSetdata(BTM_BLE_ADV_DATA_TYPE adv_data_type,
                                                      UINT8 inst_id, UINT8 *p_data, UINT32 len)
{
    UINT8 btm_evt_type = 0;
    if (adv_data_type == BTM_BLE_ADV_DATA)
    {
        btm_evt_type = BTM_BLE_EXT_MULTI_ADV_WRITE_ADV_DATA;
    }
    else if (adv_data_type == BTM_BLE_ADV_SCAN_RSP_DATA)
    {
        btm_evt_type = BTM_BLE_EXT_MULTI_ADV_WRITE_SCAN_RSP_DATA;
    }
    else if (adv_data_type == BTM_BLE_ADV_PERIODIC_DATA)
    {
        btm_evt_type = BTM_BLE_PERIODIC_MULTI_ADV_WRITE_ADV_DATA;
    }
    else
    {
       return BTM_ILLEGAL_VALUE;
    }

    return btm_ble_ext_multi_adv_set_data(btm_evt_type, inst_id, p_data, len);
}

EXPORT_SYMBOL tBTM_STATUS BTM_BleStopAdvertisingSet (UINT8 inst_id)
{
     tBTM_STATUS rt = BTM_ILLEGAL_VALUE;
     tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

     BTM_TRACE_EVENT("BTM_BleStopAdvertisingSet with inst_id:%d", inst_id);
     if (0 == BTM_BleMaxMultiAdvInstanceCount())
     {
         BTM_TRACE_ERROR("Controller does not support peri ADV");
         return BTM_ERR_PROCESSING;
     }
     if (inst_id < BTM_BleMaxMultiAdvInstanceCount() &&
         inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD)
     {
         rt = btm_ble_enable_multi_adv(FALSE, inst_id, BTM_BLE_EXT_ADV_DISABLE_EVT);
         if (BTM_CMD_STARTED != rt)
         {
             BTM_TRACE_EVENT("disable ext adv in periodic have a iisue inst_id:%d", inst_id);
         }
         if (p_inst->periodic_enabled)
         {
             rt = btm_ble_periodic_multi_adv_enable(FALSE, inst_id,
                                          BTM_BLE_ADV_PERI_DISABLE_EVT);
             if (BTM_CMD_STARTED != rt)
             {
                 BTM_TRACE_EVENT("disable periodic adv in periodic have a iisue inst_id:%d", inst_id);
             }
         }
         if ((rt = RemoveAdvertisingSet(inst_id)) == BTM_CMD_STARTED)
         {
            alarm_cancel(btm_multi_adv_cb.p_adv_inst[inst_id - 1].adv_raddr_timer);
            btm_multi_adv_cb.p_adv_inst[inst_id - 1].in_use = FALSE;
         }
    }
    return rt;
}
#endif

/*******************************************************************************
**
** Function         BTM_BleUpdateAdvInstParam
**
** Description      This function update a Multi-ADV instance with the specified
**                  adv parameters.
**
** Parameters       inst_id: adv instance ID
**                  p_params: pointer to the adv parameter structure.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS BTM_BleUpdateAdvInstParam (UINT8 inst_id, tBTM_BLE_ADV_PARAMS *p_params)
{
    tBTM_STATUS rt = BTM_ILLEGAL_VALUE;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    BTM_TRACE_EVENT("BTM_BleUpdateAdvInstParam called with inst_id:%d", inst_id);

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (0 == BTM_BleMaxMultiAdvInstanceCount())
#else
    if (0 == btm_cb.cmn_ble_vsc_cb.adv_inst_max)
#endif
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    if (inst_id <  BTM_BleMaxMultiAdvInstanceCount() &&
        inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD &&
        p_params != NULL)
    {
        if (FALSE == p_inst->in_use)
        {
            BTM_TRACE_DEBUG("adv instance %d is not active", inst_id);
            return BTM_WRONG_MODE;
        }
        else
            btm_ble_enable_multi_adv(FALSE, inst_id, 0);

        if (BTM_CMD_STARTED == btm_ble_multi_adv_set_params(p_inst, p_params, 0))
            rt = btm_ble_enable_multi_adv(TRUE, inst_id, BTM_BLE_MULTI_ADV_PARAM_EVT);
    }
    return rt;
}

/*******************************************************************************
**
** Function         BTM_BleCfgAdvInstData
**
** Description      This function configure a Multi-ADV instance with the specified
**                  adv data or scan response data.
**
** Parameters       inst_id: adv instance ID
**                  is_scan_rsp: is this scan response. if no, set as adv data.
**                  data_mask: adv data mask.
**                  p_data: pointer to the adv data structure.
**
** Returns          status
**
*******************************************************************************/
tBTM_STATUS BTM_BleCfgAdvInstData (UINT8 inst_id, BOOLEAN is_scan_rsp,
                                    tBTM_BLE_AD_MASK data_mask,
                                    tBTM_BLE_ADV_DATA *p_data)
{
    UINT8       param[BTM_BLE_MULTI_ADV_WRITE_DATA_LEN], *pp = param;
    UINT8       sub_code = (is_scan_rsp) ?
                           BTM_BLE_MULTI_ADV_WRITE_SCAN_RSP_DATA : BTM_BLE_MULTI_ADV_WRITE_ADV_DATA;
    UINT8       *p_len;
    tBTM_STATUS rt;
    UINT8 *pp_temp = (UINT8*)(param + BTM_BLE_MULTI_ADV_WRITE_DATA_LEN -1);
    tBTM_BLE_VSC_CB cmn_ble_vsc_cb;

    BTM_BleGetVendorCapabilities(&cmn_ble_vsc_cb);
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (0 == BTM_BleMaxMultiAdvInstanceCount())
#else
    if (0 == cmn_ble_vsc_cb.adv_inst_max)
#endif
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    btm_ble_update_dmt_flag_bits(&p_data->flag, btm_cb.btm_inq_vars.connectable_mode,
                                        btm_cb.btm_inq_vars.discoverable_mode);

    BTM_TRACE_EVENT("BTM_BleCfgAdvInstData called with inst_id:%d", inst_id);
    if (inst_id > BTM_BLE_MULTI_ADV_MAX || inst_id == BTM_BLE_MULTI_ADV_DEFAULT_STD)
        return BTM_ILLEGAL_VALUE;

    memset(param, 0, BTM_BLE_MULTI_ADV_WRITE_DATA_LEN);

    UINT8_TO_STREAM(pp, sub_code);
    p_len = pp ++;
    btm_ble_build_adv_data(&data_mask, &pp, p_data);
    *p_len = (UINT8)(pp - param - 2);
    UINT8_TO_STREAM(pp_temp, inst_id);

    if ((rt = BTM_VendorSpecificCommand (HCI_BLE_MULTI_ADV_OCF,
                                    (UINT8)BTM_BLE_MULTI_ADV_WRITE_DATA_LEN,
                                    param,
                                    btm_ble_multi_adv_vsc_cmpl_cback))
                                     == BTM_CMD_STARTED)
    {
        btm_ble_multi_adv_enq_op_q(sub_code, inst_id, BTM_BLE_MULTI_ADV_DATA_EVT);
    }
    return rt;
}

/*******************************************************************************
**
** Function         BTM_BleDisableAdvInstance
**
** Description      This function disables a Multi-ADV instance.
**
** Parameters       inst_id: adv instance ID
**
** Returns          status
**
*******************************************************************************/
#ifdef MTK_MESH_SUPPORT
EXPORT_SYMBOL tBTM_STATUS BTM_BleDisableAdvInstance (UINT8 inst_id)
#else
tBTM_STATUS BTM_BleDisableAdvInstance (UINT8 inst_id)
#endif
{
     tBTM_STATUS rt = BTM_ILLEGAL_VALUE;
     tBTM_BLE_VSC_CB cmn_ble_vsc_cb;

     BTM_TRACE_EVENT("BTM_BleDisableAdvInstance with inst_id:%d", inst_id);

     BTM_BleGetVendorCapabilities(&cmn_ble_vsc_cb);

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
     if (0 == BTM_BleMaxMultiAdvInstanceCount())
#else
     if (0 == cmn_ble_vsc_cb.adv_inst_max)
#endif
     {
         BTM_TRACE_ERROR("Controller does not support Multi ADV");
         return BTM_ERR_PROCESSING;
     }
     if (inst_id < BTM_BleMaxMultiAdvInstanceCount() &&
         inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD)
     {
         if ((rt = btm_ble_enable_multi_adv(FALSE, inst_id, BTM_BLE_MULTI_ADV_DISABLE_EVT))
            == BTM_CMD_STARTED)
         {
#ifdef MTK_MESH_SUPPORT
            tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
            if (*(UINT8 *)(p_inst->p_ref) != BTM_BLE_MESH_CLIENT_IF)
            {
                btm_ble_multi_adv_configure_rpa(&btm_multi_adv_cb.p_adv_inst[inst_id - 1]);
            }
            memset(advertising_data.data, 0, advertising_data.len);
            advertising_data.len = 0;
#else
            btm_ble_multi_adv_configure_rpa(&btm_multi_adv_cb.p_adv_inst[inst_id - 1]);
            memset(&(btm_multi_adv_cb.p_adv_inst[inst_id - 1].adv_data_no_scan_rsp),
            0,  sizeof(tBTM_BLE_ADV_DATA));
            memset(&(btm_multi_adv_cb.p_adv_inst[inst_id - 1].adv_data_scan_rsp),
            0,  sizeof(tBTM_BLE_ADV_DATA));
            BTM_TRACE_WARNING("disable adv clear adv data");
#endif
            alarm_cancel(btm_multi_adv_cb.p_adv_inst[inst_id - 1].adv_raddr_timer);
            btm_multi_adv_cb.p_adv_inst[inst_id - 1].in_use = FALSE;
         }
     }
    return rt;
}
#if defined(MTK_LINUX_GATTC_RPA) && (MTK_LINUX_GATTC_RPA == TRUE)
tBTM_STATUS BTM_BleGetAdvRPA(UINT8 inst_id, bt_bdaddr_t *rpa)
{
    tBTM_STATUS rt = BTM_ILLEGAL_VALUE;
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];

    BTM_TRACE_EVENT("BTM_BleGetAdvRPA called with inst_id:%d", inst_id);

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    if (0 == BTM_BleMaxMultiAdvInstanceCount())
#else
    if (0 == btm_cb.cmn_ble_vsc_cb.adv_inst_max)
#endif
    {
        BTM_TRACE_ERROR("Controller does not support Multi ADV");
        return BTM_ERR_PROCESSING;
    }

    if (inst_id <  BTM_BleMaxMultiAdvInstanceCount() &&
        inst_id != BTM_BLE_MULTI_ADV_DEFAULT_STD)
    {
        if (FALSE == p_inst->in_use)
        {
            BTM_TRACE_DEBUG("adv instance %d is not active", inst_id);
            return BTM_WRONG_MODE;
        }
        else
        {
#if defined(BLE_PRIVACY_SPT) && (BLE_PRIVACY_SPT == TRUE)
            if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE)
            {
                memcpy(rpa->address, p_inst->rpa, BD_ADDR_LEN);
            }
            else
#endif
            {
                memcpy(rpa->address, controller_get_interface()->get_address()->address, BD_ADDR_LEN);
            }
            rt = BTM_SUCCESS;
        }
    }
    return rt;
}

#endif
/*******************************************************************************
**
** Function         btm_ble_multi_adv_vse_cback
**
** Description      VSE callback for multi adv events.
**
** Returns
**
*******************************************************************************/
void btm_ble_multi_adv_vse_cback(UINT8 len, UINT8 *p)
{
    UINT8   sub_event;
    UINT8   adv_inst, idx;
    UINT16  conn_handle;
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    UINT8   status;
#endif

    /* Check if this is a BLE RSSI vendor specific event */
    STREAM_TO_UINT8(sub_event, p);
    len--;

#if defined(MTK_LINUX) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
#else
    BTM_TRACE_EVENT("btm_ble_multi_adv_vse_cback called with event:%d", sub_event);
#endif
    if ((sub_event == HCI_VSE_SUBCODE_BLE_MULTI_ADV_ST_CHG) && (len >= 4))
    {
        STREAM_TO_UINT8(adv_inst, p);
        ++p;
        STREAM_TO_UINT16(conn_handle, p);

        if ((idx = btm_handle_to_acl_index(conn_handle)) != MAX_L2CAP_LINKS)
        {
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
            if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE &&
                adv_inst <= BTM_BLE_MULTI_ADV_MAX && adv_inst !=  BTM_BLE_MULTI_ADV_DEFAULT_STD)
            {
                memcpy(btm_cb.acl_db[idx].conn_addr, btm_multi_adv_cb.p_adv_inst[adv_inst - 1].rpa,
                                BD_ADDR_LEN);
            }
#endif
        }

        if (adv_inst < BTM_BleMaxMultiAdvInstanceCount() &&
            adv_inst !=  BTM_BLE_MULTI_ADV_DEFAULT_STD)
        {
            BTM_TRACE_EVENT("btm_ble_multi_adv_reenable called");
            btm_ble_multi_adv_reenable(adv_inst);
        }
        /* re-enable connectibility */
        else if (adv_inst == BTM_BLE_MULTI_ADV_DEFAULT_STD)
        {
            if (btm_cb.ble_ctr_cb.inq_var.connectable_mode == BTM_BLE_CONNECTABLE)
            {
                btm_ble_set_connectability ( btm_cb.ble_ctr_cb.inq_var.connectable_mode );
            }
        }

    }
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    else if (sub_event == HCI_LE_ADVERTISING_SET_TERMINATED_EVT)
    {
        STREAM_TO_UINT8(status, p);
        STREAM_TO_UINT8(adv_inst, p);
        STREAM_TO_UINT16(conn_handle, p);

        if ((idx = btm_handle_to_acl_index(conn_handle)) != MAX_L2CAP_LINKS)
        {
#if (defined BLE_PRIVACY_SPT && BLE_PRIVACY_SPT == TRUE)
            if (btm_cb.ble_ctr_cb.privacy_mode != BTM_PRIVACY_NONE &&
                adv_inst <= BTM_BLE_MULTI_ADV_MAX && adv_inst !=  BTM_BLE_MULTI_ADV_DEFAULT_STD)
            {
                memcpy(btm_cb.acl_db[idx].conn_addr, btm_multi_adv_cb.p_adv_inst[adv_inst - 1].rpa,
                                BD_ADDR_LEN);
            }
#endif
        }
        BTM_TRACE_EVENT("btm_ble_multi_adv_vse_cback conn_handle=%d", conn_handle);
        BTM_TRACE_EVENT("btm_ble_multi_adv_vse_cback status=%d", status);
        if (conn_handle != 0 && (status == 0))
        {
            if ((adv_inst < BTM_BleMaxMultiAdvInstanceCount()) &&
                adv_inst !=  BTM_BLE_MULTI_ADV_DEFAULT_STD)
            {
                BTM_TRACE_EVENT("terminated to btm_ble_multi_adv_reenable called");
                btm_ble_multi_adv_reenable(adv_inst);
            }
        }
    }
#endif
}
/*******************************************************************************
**
** Function         btm_ble_multi_adv_init
**
** Description      This function initialize the multi adv control block.
**
** Parameters       None
**
** Returns          void
**
*******************************************************************************/
void btm_ble_multi_adv_init()
{
    UINT8 i = 0;
    memset(&btm_multi_adv_cb, 0, sizeof(tBTM_BLE_MULTI_ADV_CB));
    memset (&btm_multi_adv_idx_q,0, sizeof (tBTM_BLE_MULTI_ADV_INST_IDX_Q));
    btm_multi_adv_idx_q.front = -1;
    btm_multi_adv_idx_q.rear = -1;

#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    btm_cb.extended_adv_support = controller_get_interface()->supports_ble_extended_advertising();
    btm_cb.periodic_adv_support= controller_get_interface()->supports_ble_periodic_advertising();
    btm_cb.adv_set_max= controller_get_interface()->get_ble_number_of_supported_advertising_sets();
    if (BTM_BleMaxMultiAdvInstanceCount() > 0)
    {
        btm_multi_adv_cb.p_adv_inst = osi_calloc(sizeof(tBTM_BLE_MULTI_ADV_INST) *
                                                 BTM_BleMaxMultiAdvInstanceCount());

        btm_multi_adv_cb.op_q.p_sub_code = osi_calloc(sizeof(UINT8) * BTM_BleMaxMultiAdvInstanceCount());

        btm_multi_adv_cb.op_q.p_inst_id = osi_calloc(sizeof(UINT8) * BTM_BleMaxMultiAdvInstanceCount());
    }
#else
    if (btm_cb.cmn_ble_vsc_cb.adv_inst_max > 0) {
        btm_multi_adv_cb.p_adv_inst = osi_calloc(sizeof(tBTM_BLE_MULTI_ADV_INST) *
                                                 (btm_cb.cmn_ble_vsc_cb.adv_inst_max));

        btm_multi_adv_cb.op_q.p_sub_code = osi_calloc(sizeof(UINT8) *
                                                      (btm_cb.cmn_ble_vsc_cb.adv_inst_max));

        btm_multi_adv_cb.op_q.p_inst_id = osi_calloc(sizeof(UINT8) *
                                                     (btm_cb.cmn_ble_vsc_cb.adv_inst_max));
    }
#endif

    /* Initialize adv instance indices and IDs. */
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
    for (i = 0; i < BTM_BleMaxMultiAdvInstanceCount(); i++) {
#else
    for (i = 0; i < btm_cb.cmn_ble_vsc_cb.adv_inst_max; i++) {
#endif
        btm_multi_adv_cb.p_adv_inst[i].index = i;
        btm_multi_adv_cb.p_adv_inst[i].inst_id = i + 1;
        btm_multi_adv_cb.p_adv_inst[i].adv_raddr_timer =
            alarm_new("btm_ble.adv_raddr_timer");
    }

    BTM_RegisterForVSEvents(btm_ble_multi_adv_vse_cback, TRUE);
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_cleanup
**
** Description      This function cleans up multi adv control block.
**
** Parameters
** Returns          void
**
*******************************************************************************/
void btm_ble_multi_adv_cleanup(void)
{
    if (btm_multi_adv_cb.p_adv_inst) {
#if defined(MTK_BLE_BT_5_0) && (MTK_BLE_BT_5_0 == TRUE)
        for (size_t i = 0; i < BTM_BleMaxMultiAdvInstanceCount(); i++) {
#else
        for (size_t i = 0; i < btm_cb.cmn_ble_vsc_cb.adv_inst_max; i++) {
#endif
            alarm_free(btm_multi_adv_cb.p_adv_inst[i].adv_raddr_timer);
        }
        osi_free_and_reset((void **)&btm_multi_adv_cb.p_adv_inst);
    }

    osi_free_and_reset((void **)&btm_multi_adv_cb.op_q.p_sub_code);
    osi_free_and_reset((void **)&btm_multi_adv_cb.op_q.p_inst_id);
}

/*******************************************************************************
**
** Function         btm_ble_multi_adv_get_ref
**
** Description      This function obtains the reference pointer for the instance ID provided
**
** Parameters       inst_id - Instance ID
**
** Returns          void*
**
*******************************************************************************/
void* btm_ble_multi_adv_get_ref(UINT8 inst_id)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = NULL;
    if (inst_id < BTM_BleMaxMultiAdvInstanceCount())
    {
        p_inst = &btm_multi_adv_cb.p_adv_inst[inst_id - 1];
        if (NULL != p_inst)
            return p_inst->p_ref;
    }

    return NULL;
}

/*******************************************************************************
**
** Function         btm_ble_start_multi_adv
**
** Description      This function is called to restart multi adv
**
** Parameters      void
**
** Returns          void
**
*******************************************************************************/
void btm_ble_start_multi_adv(void)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];
    tBTM_BLE_CB *p_ble_cb = &btm_cb.ble_ctr_cb;
    int i = 0;

    for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
    {
        if (p_inst->in_use)
        {
            BTM_TRACE_DEBUG("btm_ble_start_multi_adv p_inst->adv_evt:%d, inst_id:%d", p_inst->adv_evt, p_inst->inst_id);
            if(BTM_CMD_STARTED != btm_ble_enable_multi_adv (TRUE, p_inst->inst_id, 0))
            {
                p_inst->in_use = FALSE;
                BTM_TRACE_ERROR("btm_ble_start_multi_adv failed");
            }
            /** disable the BTM_BLE_RL_ADV flag, since multi adv can not use with adv at the same time*/
            p_ble_cb->suspended_rl_state &= ~BTM_BLE_RL_ADV;
         }
     }

}

/*******************************************************************************
**
** Function         btm_ble_stop_multi_adv
**
** Description      This function is called to stop multi adv
**
** Parameters      void
**
** Returns          void
**
*******************************************************************************/
void btm_ble_stop_multi_adv(void)
{
    tBTM_BLE_MULTI_ADV_INST *p_inst = &btm_multi_adv_cb.p_adv_inst[0];
    tBTM_BLE_CB *p_ble_cb = &btm_cb.ble_ctr_cb;
    int i = 0;

    for (i = 0; i <  BTM_BleMaxMultiAdvInstanceCount(); i ++, p_inst++)
    {
        if (p_inst->in_use)
        {
            BTM_TRACE_DEBUG("btm_ble_stop_multi_adv p_inst->adv_evt:%d, inst_id:%d", p_inst->adv_evt, p_inst->inst_id);
            if(BTM_CMD_STARTED != btm_ble_enable_multi_adv (FALSE, p_inst->inst_id, 0))
            {
                p_inst->in_use = FALSE;
                BTM_TRACE_ERROR("btm_ble_stop_multi_adv failed");
            }
            p_ble_cb->suspended_rl_state |= BTM_BLE_RL_ADV;
         }
     }
}


#endif


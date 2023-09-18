/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2013
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/

/* FILE NAME:  c_bt_mw_ble.c
 * AUTHOR:
 * PURPOSE:
 *      It provides GATTC and GATTS API to APP.
 * NOTES:
 */


/*-----------------------------------------------------------------------------
                    include files
 ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include "bt_mw_common.h"
#include "c_bt_mw_gatt.h"
#include "c_bt_mw_gattc.h"
#include "c_bt_mw_gatts.h"
#include "c_bt_mw_gap.h"
#include "bt_mw_gatts.h"
#include <stddef.h>

/******************** gattc ********************/
#if defined(MTK_LINUX_C4A_BLE_SETUP)


#include <time.h>
#include <sys/time.h>

extern INT32 ascii_2_hex(CHAR *p_ascii, INT32 len, UINT8 *p_hex);


static CHAR * p_deviceName = NULL;
static BOOL set_scan_for_name_flag = 0;

INT32 reg_flag = 0;
static INT32 btm_client_if = -1;
static INT32 btm_server_if = -1;
static INT32 err_log_flag = 0;


#define CC_TEMP \
do \
{   \
    struct timeval now; \
    gettimeofday(&now, NULL);  \
    BT_DBG_ERROR(BT_DEBUG_GATT, "cccccccc: %s: %d: tv_sec = %d, tv_usec = %d \n", __func__, __LINE__, (int)now.tv_sec, (int)now.tv_usec);    \
}while(0)


/*GATT server*/
BtAppGATTSEventCbk MtkRpcBtGattsAppCbk = NULL;
BtAppGATTSRegServerCbk MtkRpcBtGattsRegSeverCbk = NULL;
BtAppGATTSAddSrvcCbk MtkRpcBtGattsAddSrvcCbk = NULL;
//BtAppGATTSAddInclCbk MtkRpcBtGattsAddInclCbk = NULL;
//BtAppGATTSAddCharCbk MtkRpcBtGattsAddCharCbk = NULL;
//BtAppGATTSAddDescCbk MtkRpcBtGattsAddDescCbk = NULL;
BtAppGATTSOpSrvcCbk MtkRpcBtGattsOpSrvcCbk = NULL;
BtAppGATTSReqReadCbk MtkRpcBtGattsReqReadCharCbk = NULL;
BtAppGATTSReqReadCbk MtkRpcBtGattsReqReadDescCbk = NULL;
BtAppGATTSReqWriteCbk MtkRpcBtGattsReqWriteCharCbk = NULL;
BtAppGATTSReqWriteCbk MtkRpcBtGattsReqWriteDescCbk = NULL;
//BtAppGATTSIndSentCbk MtkRpcBtGattsIndSentCbk = NULL;
BtAppGATTSExecWriteCbk MtkRpcBtGattsExecWriteCbk;
BT_GATTS_REG_SERVER_RST_T MtkRpcBtGattsRegServerRst;
BT_GATTS_ADD_SRVC_RST_T MtkRpcBtGattsAddServerRst;

BtAppGATTCRegClientCbk MtkRpcBtGattcRegClientCbk = NULL;
BtAppGATTCAdvEnableCbk MtkRpcBtGattcAdvEnableCbk = NULL;



// this no need callback to fluoride
EXPORT_SYMBOL static VOID c_btm_gatts_reg_server_cbk(BT_GATTS_REG_SERVER_RST_T *bt_gatts_reg_server)
{
    FUNC_ENTRY;
    if (NULL == bt_gatts_reg_server)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatt_event is NULL\n");
        return;
    }
    btm_server_if = bt_gatts_reg_server->server_if;
    reg_flag |= 1 << 1;
    if (0x3 == (reg_flag & 0x3))
    {
        if (NULL != MtkRpcBtGattsRegSeverCbk)
        {
             // callback to fluoride
            MtkRpcBtGattsRegSeverCbk(bt_gatts_reg_server);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] null pointer of bt_gatt_evt_fct_cbk\n");
        }
    }
    else
    {
         
        MtkRpcBtGattsRegServerRst.server_if = bt_gatts_reg_server->server_if;
        memcpy(MtkRpcBtGattsRegServerRst.app_uuid, bt_gatts_reg_server->app_uuid, strlen(bt_gatts_reg_server->app_uuid) + 1);
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATT] reg_flag = %ld\n", (long)reg_flag);
    }

    return;
}


EXPORT_SYMBOL static VOID c_btm_gatts_add_srvc_cbk(BT_GATTS_ADD_SRVC_RST_T *bt_gatts_add_srvc)
{
    FUNC_ENTRY;
    if (NULL == bt_gatts_add_srvc)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatt_event is NULL\n");
        return;
    }
    else
    {
        BT_DBG_NORMAL(BT_DEBUG_GATT, "callabck by start service!\n");
    }
    MtkRpcBtGattsAddServerRst.server_if = bt_gatts_add_srvc->server_if;
    MtkRpcBtGattsAddServerRst.srvc_handle = bt_gatts_add_srvc->srvc_handle;
    memcpy(&MtkRpcBtGattsAddServerRst.srvc_id, &(bt_gatts_add_srvc->srvc_id), sizeof(bt_gatts_add_srvc->srvc_id));

    MtkRpcBtGattsAddSrvcCbk(&MtkRpcBtGattsAddServerRst);

    return;
}

EXPORT_SYMBOL static VOID c_btm_gatts_start_srvc_cbk(BT_GATTS_SRVC_OP_TYPE_T op_type, BT_GATTS_SRVC_RST_T *bt_gatts_srvc)
{
    FUNC_ENTRY;
    if (NULL == bt_gatts_srvc)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatt_event is NULL\n");
        return;
    }
    else
    {
        if(NULL != MtkRpcBtGattsOpSrvcCbk && op_type == BT_GATTS_START_SRVC)
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "MtkRpcBtGattsOpSrvcCbk Enter\n");
            MtkRpcBtGattsOpSrvcCbk(op_type, bt_gatts_srvc);  //	callback to onServiceStarted
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "No to call MtkRpcBtGattsOpSrvcCbk\n");
        }
    }
    return;
}


EXPORT_SYMBOL static VOID c_btm_gatts_req_read_cbk(BT_GATTS_REQ_READ_RST_T *bt_gatts_read)
{
    FUNC_ENTRY;
    if (NULL == bt_gatts_read)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatt_event is NULL\n");
        return;
    }
   
    if (NULL != MtkRpcBtGattsReqReadCharCbk)
    {
        MtkRpcBtGattsReqReadCharCbk(bt_gatts_read);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "no register MtkRpcBtGattsReqReadCharCbk\n");
    }

    return;
}

EXPORT_SYMBOL static VOID c_btm_gatts_req_write_cbk(BT_GATTS_REQ_WRITE_RST_T *bt_gatts_write)
{
    FUNC_ENTRY;
    if (NULL == bt_gatts_write)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatts_write is NULL\n");
        return;
    }
    if (NULL != MtkRpcBtGattsReqWriteCharCbk)
    {
        MtkRpcBtGattsReqWriteCharCbk(bt_gatts_write);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "no register MtkRpcBtGattsReqReadCharCbk\n");
    }

    return;

}



EXPORT_SYMBOL VOID c_btm_bt_gatts_request_exec_write_cbk(INT32 conn_id, INT32 trans_id, CHAR *bda, INT32 exec_write)
{
    FUNC_ENTRY;
    BT_DBG_WARNING(BT_DEBUG_GATT, "[GATT] conn_id =%d, trans_id =%d, exec_write = %d\n", conn_id, trans_id, exec_write);
    if (exec_write == 1)
    {
        if (NULL != MtkRpcBtGattsExecWriteCbk)
        {
            MtkRpcBtGattsExecWriteCbk(conn_id, trans_id, bda, exec_write);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] no register MtkRpcBtGattsExecWriteCbk!\n");
        }
    }
}

EXPORT_SYMBOL VOID c_btm_bt_gatts_connected_cbk(BT_GATTS_EVENT_T bt_gatts_event)
{
    FUNC_ENTRY;
    if (NULL != MtkRpcBtGattsAppCbk)
    {
        MtkRpcBtGattsAppCbk(bt_gatts_event);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] no register MtkRpcBtGattsAppCbk\n");
    }

}

EXPORT_SYMBOL VOID c_btm_bt_gattc_adv_enable_cbk(INT32 status)
{
    FUNC_ENTRY;
    if (NULL != MtkRpcBtGattcAdvEnableCbk)
    {
        MtkRpcBtGattcAdvEnableCbk(status);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] no register MtkRpcBtGattcAdvEnableCbk\n");
    }

}


EXPORT_SYMBOL static VOID c_btm_gattc_reg_client_cbk(BT_GATTC_REG_CLIENT_T *bt_gatt_event)
{
    //FUNC_ENTRY;
    if (NULL == bt_gatt_event)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "bt_gatt_event is NULL\n");
        return;
    }
    if (NULL != MtkRpcBtGattcRegClientCbk)
    {
        MtkRpcBtGattcRegClientCbk(bt_gatt_event);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] null pointer of MtkRpcBtGattcRegClientCbk\n");
    }
    btm_client_if = bt_gatt_event->client_if;
    reg_flag |= 1 << 0;
    if (0x3 == (reg_flag & 0x3))
    {
        if (NULL != MtkRpcBtGattsRegSeverCbk)
        {
            MtkRpcBtGattsRegSeverCbk(&MtkRpcBtGattsRegServerRst);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] null pointer of MtkRpcBtGattsRegSeverCbk\n");
        }
    }
    else
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATT] reg_flag = %ld\n", (long)reg_flag);
    }

    return;
}

// call by fluoride_new initialize
EXPORT_SYMBOL INT32 c_btm_gatts_initialize(VOID)
{
    FUNC_ENTRY;
    //CC_TEMP;
    INT32 sec = 0;
    INT32 i4_loop = 100;
    BT_LOCAL_DEV local_dev;
    INT32 i4_ret = 0;
    memset(&local_dev, 0, sizeof(BT_LOCAL_DEV));
    i4_ret = c_btm_get_local_dev_info(&local_dev);
	BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] c_btm_gatts_initialize,local_dev.addr=%p, local_dev.name=%s, local_dev.state=%d\n",local_dev.bdAddr, local_dev.name, local_dev.state);
    
    while (0 < i4_loop && (local_dev.state != GAP_STATE_ON))
    {
        usleep(100*1000);
        sec++;
        i4_loop--;
        i4_ret = c_btm_get_local_dev_info(& local_dev);
    }
    BT_DBG_INFO(BT_DEBUG_GATT, "[GATT] sleep %ld s, fg_bt_power_on = %d\n", (long)sec, local_dev.state);
    INT32 ret = BT_SUCCESS;
    BT_APP_GATTS_CB_FUNC_T gatts_func;
    memset(&gatts_func, 0, sizeof(BT_APP_GATTS_CB_FUNC_T));
    memset(&MtkRpcBtGattsRegServerRst, 0, sizeof(BT_GATTS_REG_SERVER_RST_T));
    memset(&MtkRpcBtGattsAddServerRst, 0, sizeof(BT_GATTS_ADD_SRVC_RST_T));

    gatts_func.bt_gatts_event_cb = c_btm_bt_gatts_connected_cbk; // need callback to viv2
    gatts_func.bt_gatts_reg_server_cb = c_btm_gatts_reg_server_cbk; //  no need callback to viv2
    gatts_func.bt_gatts_add_srvc_cb = c_btm_gatts_add_srvc_cbk;   // need callback to viv2
    //gatts_func.bt_gatts_add_char_cb = c_btm_gatts_add_char_cbk_v2;  // no need, char save in v1v2
    gatts_func.bt_gatts_op_srvc_cb = c_btm_gatts_start_srvc_cbk;
    gatts_func.bt_gatts_req_read_cb = c_btm_gatts_req_read_cbk; // need callback to viv2
    gatts_func.bt_gatts_req_write_cb = c_btm_gatts_req_write_cbk; // need callback to viv2
    gatts_func.bt_gatts_exec_write_cb = c_btm_bt_gatts_request_exec_write_cbk; // need callback to viv2
    ret = c_btm_gatts_register_callback(&gatts_func);
    if (BT_SUCCESS != ret)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] Register BT_APP_GATTS_CB_FUNC failed.\n");
        return ret;

    }
    ret = c_btm_gatts_register_server_sync(BT_GATTS_SERVER_UUID, &btm_server_if);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] register server success!\n");
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] register server failed, ret = %ld\n", (long)ret);
        return ret;
    }

    //extern INT32 bluetooth_set_virtual_sniffer(INT32 enable);
    //bluetooth_set_virtual_sniffer(1);
    //CC_TEMP;
    return ret;
}


EXPORT_SYMBOL INT32 c_btm_gattc_initialize(VOID)
{
    INT32 ret = BT_SUCCESS;
    BT_LOCAL_DEV local_dev;
    memset(&local_dev, 0, sizeof(BT_LOCAL_DEV));
    ret = c_btm_get_local_dev_info(&local_dev);
	BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] c_btm_gattc_initialize,local_dev.addr=%p, local_dev.name=%s, local_dev.state=%d\n",local_dev.bdAddr, local_dev.name, local_dev.state);
    BT_APP_GATTC_CB_FUNC_T gatt_func;
    memset(&gatt_func, 0, sizeof(BT_APP_GATTC_CB_FUNC_T));
    gatt_func.bt_gattc_reg_client_cb = c_btm_gattc_reg_client_cbk;
    gatt_func.bt_gattc_adv_enable_cb = c_btm_bt_gattc_adv_enable_cbk;
    ret = c_btm_gattc_register_callback(&gatt_func);
    if (BT_SUCCESS != ret)
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATT] Register BT_APP_GATT_CB_FUNC failed.\n");
        return ret;
    }
    ret = c_btm_gattc_register_app_sync(BT_GATTC_APP_UUID, &btm_client_if);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] register client success!\n");
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] register client failed, ret = %ld\n", (long)ret);
        return ret;
    }
    return ret;
}



/**
 * FUNCTION NAME: c_btm_gatts_isSupported
 * PURPOSE:
 *      The function is used to judge the device supports bluetooth operations or not.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      TRUE      -- support bluetooth operations.
 * NOTES:
 *      None
 */
EXPORT_SYMBOL BOOL c_btm_gatts_isSupported(VOID)
{
    //FUNC_ENTRY;
    return TRUE;
}

EXPORT_SYMBOL BOOL c_btm_gatts_reg_gattServer_callback(BT_APP_BLE_GATTS_CB_FUNC_T *func)
{
    FUNC_ENTRY;
    MtkRpcBtGattsAppCbk = func->bt_gatts_event_cb; //connect status
    MtkRpcBtGattsRegSeverCbk = func->bt_gatts_reg_server_cb;
    MtkRpcBtGattsAddSrvcCbk = func->bt_gatts_add_srvc_cb;
    //MtkRpcBtGattsOpSrvcCbk = func->bt_gatts_op_srvc_cb;
    MtkRpcBtGattsReqReadCharCbk = func->bt_gatts_req_read_char_cb;
    MtkRpcBtGattsReqReadDescCbk = func->bt_gatts_req_read_desc_cb;
    MtkRpcBtGattsReqWriteCharCbk = func->bt_gatts_req_write_char_cb;
    MtkRpcBtGattsReqWriteDescCbk = func->bt_gatts_req_write_desc_cb;
    MtkRpcBtGattsExecWriteCbk = func->bt_gatts_exec_write_cb;
    MtkRpcBtGattsOpSrvcCbk = func->bt_gatts_op_srvc_cb;
    return TRUE;
}

EXPORT_SYMBOL BOOL c_btm_gattc_reg_gattClient_callback(BT_APP_GATTC_CB_FUNC_T *func)
{
    FUNC_ENTRY;
    MtkRpcBtGattcRegClientCbk = func->bt_gattc_reg_client_cb;
    MtkRpcBtGattcAdvEnableCbk = func->bt_gattc_adv_enable_cb;
    return TRUE;
}





EXPORT_SYMBOL INT32 c_btm_gatts_setDeviceName(CHAR * name)
{
    //FUNC_ENTRY;

    if ((NULL == name) || (0 == strncmp(name, "", strlen(name))))
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "name is NULL\n");
        return BT_ERR_STATUS_PARM_INVALID;
    }

    if (NULL != p_deviceName)
    {
        if (strlen(name) > strlen(p_deviceName))
        {
            free(p_deviceName);
            p_deviceName = NULL;
            p_deviceName = malloc(strlen(name) + 1);
            if (NULL == p_deviceName)
            {
                BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] set device name, failed: %s\n", name);
                return BT_ERR_STATUS_NOMEM;
            }
        }
    }
    else
    {
        p_deviceName = malloc(strlen(name) + 1);
        if (NULL == p_deviceName)
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] set device name, failed: %s\n", name);
            return BT_ERR_STATUS_NOMEM;
        }
    }

    strncpy(p_deviceName, name, strlen(name));
    p_deviceName[strlen(name)] = '\0';
    set_scan_for_name_flag = 1;
//    scan_transmit_name = TRUE;

    return BT_SUCCESS;
}



EXPORT_SYMBOL INT32 c_btm_gatts_addService(CHAR * service_uuid)
{
    FUNC_ENTRY;
    CHAR uuid[BT_GATT_MAX_UUID_LEN];
    INT32 ret = BT_SUCCESS;
    INT32 srvc_handle = 0;

    if ((NULL == service_uuid) || (0 == strncmp(service_uuid, "", strlen(service_uuid))))
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "service_uuid is NULL\n");
        return BT_ERR_STATUS_PARM_INVALID;
    }

    //linuxbt_uuid_stoh(service_uuid, &uuid);
    bluetooth_uuid_stos(service_uuid, uuid);

    BT_DBG_NOTICE(BT_DEBUG_GATT, "[GATT] service_uuid = %s, uuid = %s\n", service_uuid, uuid);

    ret = c_btm_gatts_add_service_sync(btm_server_if, uuid, 1, 255, &srvc_handle);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] Create service uuid:'%s'\n", service_uuid);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] Create service failed, uuid:'%s'\n", service_uuid);
        return BT_ERR_STATUS_FAIL;
    }

    return ret;
}

EXPORT_SYMBOL INT32 c_btm_gatts_startService(CHAR * service_uuid, INT32 srvc_handle)
{
    FUNC_ENTRY;
    CHAR uuid[BT_GATT_MAX_UUID_LEN];
    INT32 ret = BT_SUCCESS;
    

    if ((NULL == service_uuid) || (0 == strncmp(service_uuid, "", strlen(service_uuid))))
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "service_uuid is NULL\n");
        return BT_ERR_STATUS_PARM_INVALID;
    }

    //linuxbt_uuid_stoh(service_uuid, &uuid);
    bluetooth_uuid_stos(service_uuid, uuid);

    BT_DBG_NOTICE(BT_DEBUG_GATT, "[GATT] service_uuid = %s, uuid = %s\n", service_uuid, uuid);

    ret = c_btm_gatts_start_service_sync(btm_server_if, srvc_handle, 0);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] Start server uuid:'%s'\n", service_uuid);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] Start server failed, uuid:'%s'\n", service_uuid);
        return ret;
    }
    ret = c_btm_gattc_multi_adv_enable(btm_client_if, 2048, 2048, 0, 0, 1, 0);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] enable advertisement for start server uuid:'%s'\n", service_uuid);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] enable advertisement for start server failed, uuid:'%s'\n", service_uuid);
        return ret;
    }
    return ret;
}

EXPORT_SYMBOL INT32 c_btm_gatts_send_response_offset(INT32 conn_id,
                                                  INT32 trans_id,
                                                  INT32 status,
                                                  INT32 offset,
                                                  INT32 handle,
                                                  CHAR *p_value,
                                                  INT32 value_len,
                                                  INT32 auth_req)
{
    return bluetooth_gatts_send_response_offset(conn_id, trans_id, status, 
                                                offset, handle, p_value, value_len,auth_req);
}


EXPORT_SYMBOL INT32 c_btm_gattc_setAdvertisement(CHAR * service_uuid,
                                                CHAR * advertised_uuid,
                                                CHAR * advertise_data,
                                                CHAR * manufacturer_data,
                                                BOOL transmit_name)
{
    FUNC_ENTRY;
    INT32 ret = BT_SUCCESS;
    BT_DBG_NORMAL(BT_DEBUG_GATT, "[GATT] service_uuid = %s, advertised_uuid = %s, advertise_data = %s, transmit_name = %d\n",
                    service_uuid, advertised_uuid, advertise_data, transmit_name);
    CHAR adv_advertise_data[32] = {0};
    INT32 adv_advertise_data_len = 0;

    adv_advertise_data_len = ascii_2_hex(advertise_data, strlen(advertise_data), (UINT8 *)adv_advertise_data);
    ret= c_btm_gattc_multi_adv_setdata(btm_client_if,
                                          FALSE,
                                          transmit_name,
                                          FALSE,
                                          0,
                                          0,       //  strlen(manufacturer_data),
                                          NULL,    //  manufacturer_data,
                                          adv_advertise_data_len,
                                          adv_advertise_data,
                                          strlen(advertised_uuid),
                                          advertised_uuid); 
    if (BT_SUCCESS == ret)

    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] Set advertisement for start server uuid:'%s'\n", service_uuid);
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] Set advertisement for start server failed, uuid:'%s'\n", service_uuid);
    }
    return ret;
}

EXPORT_SYMBOL INT32 c_btm_gattc_setScanResponse(CHAR * service_uuid,
                                                CHAR * advertised_uuid,
                                                CHAR * advertise_data,
                                                CHAR * manufacturer_data,
                                                BOOL transmit_name)
{
    INT32 ret = BT_SUCCESS;
    if(0 == transmit_name)
    {
        CHAR hex_manufacturer[32] = {0};
        INT32 hex_manufacturer_data_len = 0;

        hex_manufacturer_data_len = ascii_2_hex(manufacturer_data, strlen(manufacturer_data), (UINT8 *)hex_manufacturer);
        ret = c_btm_gattc_multi_adv_setdata(btm_client_if,
                                               TRUE,
                                               transmit_name,
                                               FALSE,
                                               0,
                                               hex_manufacturer_data_len,
                                               hex_manufacturer,
                                               0,       //   scan_advertise_data_len,
                                               NULL,    //     scan_advertise_data,
                                               0,       //  strlen(advertised_uuid),
                                               NULL    //  advertised_uuid,
                                               );

        if (BT_SUCCESS == ret)
        {
            BT_DBG_INFO(BT_DEBUG_GATT,"[GATTS] Set scan response for start server uuid:'%s'\n", service_uuid);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] Set scan response for start server failed, uuid:'%s'\n", service_uuid);
        }
    }

    else if(1 == set_scan_for_name_flag)
    {
         ret = c_btm_gattc_multi_adv_setdata_sync(btm_client_if,
                                           TRUE,
                                           TRUE,
                                           FALSE,
                                           0,
                                           0,
                                           NULL,
                                           0,       //  scan_advertise_data_len,
                                           NULL,    //  scan_advertise_data,
                                           0,       //  strlen(advertised_uuid),
                                           NULL    //  advertised_uuid,
                                           );


        if (BT_SUCCESS == ret)
        {
            BT_DBG_INFO(BT_DEBUG_GATT,"[GATTS] Set scan response for start server uuid:'%s'\n", service_uuid);
        }
        else
        {
            BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] Set scan response for start server failed, uuid:'%s'\n", service_uuid);
        }
    }
    return ret;
}

EXPORT_SYMBOL INT32 c_btm_gattc_stopAdvertisement(INT32 client_if)
{
    FUNC_ENTRY;
    INT32 ret = BT_SUCCESS;
    ret = bluetooth_gattc_multi_adv_disable_sync(client_if);
    return ret;
}

EXPORT_SYMBOL INT32 c_btm_gattc_EnableAdvertisement(INT32 client_if)
{
    FUNC_ENTRY;
    INT32 ret = BT_SUCCESS;
    ret = c_btm_gattc_multi_adv_enable(client_if, 2048, 2048, 0, 0, 1, 0);
    if (BT_SUCCESS == ret)
    {
        BT_DBG_INFO(BT_DEBUG_GATT, "[GATTS] enable advertisement\n");
    }
    else
    {
        BT_DBG_ERROR(BT_DEBUG_GATT, "[GATTS] enable advertisement failed\n");
    }
    return ret;
}

#endif



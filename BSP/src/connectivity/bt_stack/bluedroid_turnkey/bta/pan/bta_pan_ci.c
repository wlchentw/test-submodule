/******************************************************************************
 *
 *  Copyright (C) 2004-2012 Broadcom Corporation
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

/******************************************************************************
 *
 *  This is the implementation file for data gateway call-in functions.
 *
 ******************************************************************************/

#include "bt_target.h"

#include <string.h>

#include "bt_common.h"
#include "pan_api.h"
#include "bta_api.h"
#include "bta_pan_api.h"
#include "bta_pan_ci.h"
#include "bta_pan_int.h"
#include "bt_utils.h"

#if defined(BTA_PAN_INCLUDED) && (BTA_PAN_INCLUDED == TRUE)

/*******************************************************************************
**
** Function         bta_pan_ci_tx_ready
**
** Description      This function sends an event to PAN indicating the phone is
**                  ready for more data and PAN should call bta_pan_co_tx_path().
**                  This function is used when the TX data path is configured
**                  to use a pull interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_tx_ready(UINT16 handle)
{
    BT_HDR *p_buf = (BT_HDR *)osi_malloc(sizeof(BT_HDR));

    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_TX_READY_EVT;

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_ready
**
** Description      This function sends an event to PAN indicating the phone
**                  has data available to send to PAN and PAN should call
**                  bta_pan_co_rx_path().  This function is used when the RX
**                  data path is configured to use a pull interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_ready(UINT16 handle)
{
    BT_HDR *p_buf = (BT_HDR *)osi_malloc(sizeof(BT_HDR));

    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_READY_EVT;

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_tx_flow
**
** Description      This function is called to enable or disable data flow on
**                  the TX path.  The phone should call this function to
**                  disable data flow when it is congested and cannot handle
**                  any more data sent by bta_pan_co_tx_write() or
**                  bta_pan_co_tx_writebuf().  This function is used when the
**                  TX data path is configured to use a push interface.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_tx_flow(UINT16 handle, BOOLEAN enable)
{
    tBTA_PAN_CI_TX_FLOW  *p_buf =
        (tBTA_PAN_CI_TX_FLOW *)osi_malloc(sizeof(tBTA_PAN_CI_TX_FLOW));

    p_buf->hdr.layer_specific = handle;
    p_buf->hdr.event = BTA_PAN_CI_TX_FLOW_EVT;
    p_buf->enable = enable;

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_write
**
** Description      This function is called to send data to PAN when the RX path
**                  is configured to use a push interface.  The function copies
**                  data to an event buffer and sends it to PAN.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_write(UINT16 handle, BD_ADDR dst, BD_ADDR src, UINT16 protocol,
                            UINT8 *p_data, UINT16 len, BOOLEAN ext)
{
    BT_HDR *p_buf = (BT_HDR *)osi_malloc(PAN_BUF_SIZE);

    p_buf->offset = PAN_MINIMUM_OFFSET;

    /* copy all other params before the data */
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->src, src);
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->dst, dst);
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol = protocol;
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext = ext;
    p_buf->len=len;

    /* copy data */
    memcpy((UINT8 *)(p_buf + 1) + p_buf->offset, p_data, len);

    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_WRITEBUF_EVT;

    bta_sys_sendmsg(p_buf);
}

/*******************************************************************************
**
** Function         bta_pan_ci_rx_writebuf
**
** Description      This function is called to send data to the phone when
**                  the RX path is configured to use a push interface with
**                  zero copy.  The function sends an event to PAN containing
**                  the data buffer. The buffer will be freed by BTA; the
**                  phone must not free the buffer.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_rx_writebuf(UINT16 handle, BD_ADDR dst, BD_ADDR src, UINT16 protocol,
                            BT_HDR *p_buf, BOOLEAN ext)
{

    /* copy all other params before the data */
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->src, src);
    bdcpy(((tBTA_PAN_DATA_PARAMS *)p_buf)->dst, dst);
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol = protocol;
    ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext = ext;

    p_buf->layer_specific = handle;
    p_buf->event = BTA_PAN_CI_RX_WRITEBUF_EVT;
    bta_sys_sendmsg(p_buf);
}




/*******************************************************************************
**
** Function         bta_pan_ci_readbuf
**
** Description
**
**
** Returns          void
**
*******************************************************************************/
BT_HDR * bta_pan_ci_readbuf(UINT16 handle, BD_ADDR src, BD_ADDR dst, UINT16* p_protocol,
                                 BOOLEAN* p_ext, BOOLEAN* p_forward)
{
    tBTA_PAN_SCB * p_scb;
    BT_HDR * p_buf = NULL;

    p_scb = bta_pan_scb_by_handle(handle);

    if (NULL == p_scb)
    {
       return NULL;
    }

    p_buf = (BT_HDR *)fixed_queue_try_dequeue(p_scb->data_queue);
    if (p_buf != NULL)
    {
        bdcpy(src,((tBTA_PAN_DATA_PARAMS *)p_buf)->src);
        bdcpy(dst,((tBTA_PAN_DATA_PARAMS *)p_buf)->dst);
        *p_protocol = ((tBTA_PAN_DATA_PARAMS *)p_buf)->protocol;
        *p_ext = ((tBTA_PAN_DATA_PARAMS *)p_buf)->ext;
        *p_forward = ((tBTA_PAN_DATA_PARAMS *)p_buf)->forward;
    }

    return p_buf;
}


/*******************************************************************************
**
** Function         bta_pan_ci_set_mfilters
**
** Description      This function is called to set multicast filters
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_set_mfilters(UINT16 handle, UINT16 num_mcast_filters, UINT8 *p_start_array,
                                                    UINT8 *p_end_array)
{

    PAN_SetMulticastFilters(handle, num_mcast_filters, p_start_array, p_end_array);

}


/*******************************************************************************
**
** Function         bta_pan_ci_set_mfilters
**
** Description      This function is called to set protocol filters
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_ci_set_pfilters(UINT16 handle, UINT16 num_filters, UINT16 *p_start_array, UINT16 *p_end_array)
{

    PAN_SetProtocolFilters(handle, num_filters, p_start_array, p_end_array );

}
#else

void bta_pan_ci_tx_ready(UINT16 handle)
{
    UNUSED(handle);
}

void bta_pan_ci_rx_ready(UINT16 handle)
{
    UNUSED(handle);
}

void bta_pan_ci_tx_flow(UINT16 handle, BOOLEAN enable)
{
    UNUSED(handle);
    UNUSED(enable);
}

void bta_pan_ci_rx_writebuf(UINT16 handle, BD_ADDR src, BD_ADDR dst, UINT16 protocol, BT_HDR *p_buf, BOOLEAN ext)
{
    UNUSED(handle);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(protocol);
    UNUSED(p_buf);
    UNUSED(ext);
}

BT_HDR * bta_pan_ci_readbuf(UINT16 handle, BD_ADDR src, BD_ADDR dst, UINT16 *p_protocol,
                            BOOLEAN* p_ext, BOOLEAN* p_forward)
{
    UNUSED(handle);
    UNUSED(src);
    UNUSED(dst);
    UNUSED(p_protocol);
    UNUSED(p_ext);
    UNUSED(p_forward);
    return NULL;
}

void bta_pan_ci_set_pfilters(UINT16 handle, UINT16 num_filters, UINT16 *p_start_array, UINT16 *p_end_array)
{
    UNUSED(handle);
    UNUSED(num_filters);
    UNUSED(p_start_array);
    UNUSED(p_end_array);
}

void bta_pan_ci_set_mfilters(UINT16 handle, UINT16 num_mcast_filters, UINT8 *p_start_array,
                             UINT8 *p_end_array)
{
    UNUSED(handle);
    UNUSED(num_mcast_filters);
    UNUSED(p_start_array);
    UNUSED(p_end_array);
}

#endif /* BTA_PAN_API */

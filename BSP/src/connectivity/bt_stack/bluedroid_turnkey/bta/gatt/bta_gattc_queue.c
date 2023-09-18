/******************************************************************************
 *
 *  Copyright (C) 2003-2012 Broadcom Corporation
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
#include "bta_gattc_queue.h"

/* Holds pending GATT operations */
typedef struct {
    UINT8 type;
    UINT16 conn_id;
    UINT16 handle;
    tBTA_GATT_AUTH_REQ auth_req;

    /* write-specific fields */
    tBTA_GATTC_WRITE_TYPE write_type;
    UINT16  len;
    UINT8   p_value[GATT_MAX_ATTR_LEN];
} gatt_operation;

static list_t *gatt_op_queue = NULL; // list of gatt_operation
static list_t *gatt_op_queue_executing = NULL; // list of UINT16 connection ids that currently execute

static void mark_as_executing(UINT16 conn_id) {
    UINT16 *executing_conn_id = osi_malloc(sizeof(UINT16));
    *executing_conn_id = conn_id;
    if (!gatt_op_queue_executing)
        gatt_op_queue_executing = list_new(osi_free);

    list_append(gatt_op_queue_executing, executing_conn_id);
}

static bool rm_exec_conn_id(void *data, void *context) {
    UINT16 *conn_id = context;
    UINT16 *conn_id2 = data;
    if (*conn_id == *conn_id2)
        list_remove(gatt_op_queue_executing, data);

    return TRUE;
}

static bool exec_list_contains(void *data, void *context) {
    UINT16 *conn_id = context;
    UINT16 *conn_id2 = data;
    if (*conn_id == *conn_id2)
        return FALSE;

    return TRUE;
}

static bool rm_op_by_conn_id(void *data, void *context) {
    UINT16 *conn_id = context;
    gatt_operation *op = data;
    if (op->conn_id == *conn_id)
        list_remove(gatt_op_queue, data);

    return TRUE;
}

static bool find_op_by_conn_id(void *data, void *context) {
    UINT16 *conn_id = context;
    gatt_operation *op = data;
    if(op->conn_id == *conn_id)
        return FALSE;

    return TRUE;
}

void mark_as_not_executing(UINT16 conn_id) {
    if (gatt_op_queue_executing)
        list_foreach(gatt_op_queue_executing, rm_exec_conn_id, &conn_id);
}

void gatt_op_queue_clean(UINT16 conn_id) {
    if (gatt_op_queue)
        list_foreach(gatt_op_queue, rm_op_by_conn_id, &conn_id);

    mark_as_not_executing(conn_id);
}

void gatt_execute_next_op(UINT16 conn_id) {
    APPL_TRACE_DEBUG("%s:", __func__, conn_id);
    if (!gatt_op_queue || list_is_empty(gatt_op_queue)) {
        APPL_TRACE_DEBUG("%s: op queue is empty", __func__);
        return;
    }

    list_node_t *op_node = list_foreach(gatt_op_queue, find_op_by_conn_id, &conn_id);
    if (op_node == NULL) {
        APPL_TRACE_DEBUG("%s: no more operations queued for conn_id %d", __func__, conn_id);
        return;
    }
    gatt_operation *op = list_node(op_node);

    if (gatt_op_queue_executing && list_foreach(gatt_op_queue_executing, exec_list_contains, &conn_id)) {
        APPL_TRACE_DEBUG("%s: can't enqueue next op, already executing", __func__);
        return;
    }

    if (op->type == GATT_READ_CHAR) {
        mark_as_executing(conn_id);
        BTA_GATTC_ReadCharacteristic(op->conn_id, op->handle, op->auth_req);
        list_remove(gatt_op_queue, op);

    } else if (op->type == GATT_READ_DESC) {
        mark_as_executing(conn_id);
        BTA_GATTC_ReadCharDescr(op->conn_id, op->handle, op->auth_req);
        list_remove(gatt_op_queue, op);
    } else if (op->type == GATT_WRITE_CHAR) {
        mark_as_executing(conn_id);
        BTA_GATTC_WriteCharValue(op->conn_id, op->handle, op->write_type, op->len,
                                 op->p_value, op->auth_req);

        list_remove(gatt_op_queue, op);
    } else if (op->type == GATT_WRITE_DESC) {
        tBTA_GATT_UNFMT value;
        value.len = op->len;
        value.p_value = op->p_value;

        mark_as_executing(conn_id);
        BTA_GATTC_WriteCharDescr(op->conn_id, op->handle, BTA_GATTC_TYPE_WRITE,
                                 &value, op->auth_req);
        list_remove(gatt_op_queue, op);
    }
}

void gatt_queue_read_op(UINT8 op_type, UINT16 conn_id, UINT16 handle) {
  if (gatt_op_queue == NULL) {
    gatt_op_queue = list_new(osi_free);
  }

  gatt_operation *op = osi_malloc(sizeof(gatt_operation));
  op->type = op_type;
  op->conn_id = conn_id;
  op->handle = handle;
  op->auth_req = BTA_GATT_AUTH_REQ_NONE;

  list_append(gatt_op_queue, op);
  gatt_execute_next_op(conn_id);
}

void gatt_queue_write_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, UINT16 len,
                        UINT8 *p_value, tBTA_GATTC_WRITE_TYPE write_type) {
  if (gatt_op_queue == NULL) {
    gatt_op_queue = list_new(osi_free);
  }

  gatt_operation *op = osi_malloc(sizeof(gatt_operation));
  op->type = op_type;
  op->conn_id = conn_id;
  op->handle = handle;
  op->write_type = write_type;
  op->auth_req = BTA_GATT_AUTH_REQ_NONE;
  op->len = len;
  memcpy(op->p_value, p_value, len);

  list_append(gatt_op_queue, op);
  gatt_execute_next_op(conn_id);
}

void gatt_queue_auth_read_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, tBTA_GATT_AUTH_REQ auth_req) {
  if (gatt_op_queue == NULL) {
    gatt_op_queue = list_new(osi_free);
  }

  gatt_operation *op = osi_malloc(sizeof(gatt_operation));
  op->type = op_type;
  op->conn_id = conn_id;
  op->handle = handle;
  op->auth_req = auth_req;

  list_append(gatt_op_queue, op);
  gatt_execute_next_op(conn_id);
}

void gatt_queue_auth_write_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, UINT16 len,
                        UINT8 *p_value, tBTA_GATTC_WRITE_TYPE write_type, tBTA_GATT_AUTH_REQ auth_req) {
  if (gatt_op_queue == NULL) {
    gatt_op_queue = list_new(osi_free);
  }

  gatt_operation *op = osi_malloc(sizeof(gatt_operation));
  op->type = op_type;
  op->conn_id = conn_id;
  op->handle = handle;
  op->write_type = write_type;
  op->auth_req = auth_req;
  op->len = len;
  memcpy(op->p_value, p_value, len);

  list_append(gatt_op_queue, op);
  gatt_execute_next_op(conn_id);
}

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
#include "bta_gatt_api.h"

#define GATT_READ_CHAR 0
#define GATT_READ_DESC 1
#define GATT_WRITE_CHAR 2
#define GATT_WRITE_DESC 3

void mark_as_not_executing(UINT16 conn_id);
void gatt_op_queue_clean(UINT16 conn_id);
void gatt_execute_next_op(UINT16 conn_id);
void gatt_queue_read_op(UINT8 op_type, UINT16 conn_id, UINT16 handle);
void gatt_queue_write_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, UINT16 len,
                        UINT8 *p_value, tBTA_GATTC_WRITE_TYPE write_type);
void gatt_queue_auth_read_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, tBTA_GATT_AUTH_REQ auth_req);
void gatt_queue_auth_write_op(UINT8 op_type, UINT16 conn_id, UINT16 handle, UINT16 len,
                        UINT8 *p_value, tBTA_GATTC_WRITE_TYPE write_type, tBTA_GATT_AUTH_REQ auth_req);
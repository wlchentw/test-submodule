/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
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

#ifndef BTIF_HH_H
#define BTIF_HH_H

#include <hardware/bluetooth.h>
#include <hardware/bt_hh.h>
#include <pthread.h>
#include <stdint.h>
#include "bta_hh_api.h"
#include "btu.h"
#include "osi/include/semaphore.h"


/*******************************************************************************
**  Constants & Macros
********************************************************************************/

#define BTIF_HH_MAX_HID         8
#define BTIF_HH_MAX_ADDED_DEV   32

#define BTIF_HH_MAX_KEYSTATES            3
#define BTIF_HH_KEYSTATE_MASK_NUMLOCK    0x01
#define BTIF_HH_KEYSTATE_MASK_CAPSLOCK   0x02
#define BTIF_HH_KEYSTATE_MASK_SCROLLLOCK 0x04


/*******************************************************************************
**  Type definitions and return values
********************************************************************************/

typedef enum
{
    BTIF_HH_DISABLED =   0,
    BTIF_HH_ENABLED,
    BTIF_HH_DISABLING,
    BTIF_HH_DEV_UNKNOWN,
    BTIF_HH_DEV_CONNECTING,
    BTIF_HH_DEV_CONNECTED,
    BTIF_HH_DEV_DISCONNECTED
} BTIF_HH_STATUS;

typedef struct
{
    bthh_connection_state_t       dev_status;
    UINT8                         dev_handle;
    bt_bdaddr_t                   bd_addr;
    tBTA_HH_ATTR_MASK             attr_mask;
    UINT8                         sub_class;
    UINT8                         app_id;
    int                           fd;
    BOOLEAN                       ready_for_data;
    pthread_t                     hh_poll_thread_id;
    UINT8                         hh_keep_polling;
    alarm_t                       *vup_timer;
    BOOLEAN                       local_vup; // Indicated locally initiated VUP
#if defined(MTK_LINUX_HID_KEY_QUEUE) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)
    fixed_queue_t                *keys_queue;
    pthread_mutex_t               lock;
#endif /* defined(MTK_LINUX_HID_KEY_QUEUE) && defined(MTK_COMMON) && (MTK_COMMON == TRUE)*/
} btif_hh_device_t;

/* Control block to maintain properties of devices */
typedef struct
{
    UINT8             dev_handle;
    bt_bdaddr_t       bd_addr;
    tBTA_HH_ATTR_MASK attr_mask;
} btif_hh_added_device_t;

/**
 * BTIF-HH control block to maintain added devices and currently
 * connected hid devices
 */
typedef struct
{
    BTIF_HH_STATUS          status;
    btif_hh_device_t        devices[BTIF_HH_MAX_HID];
    UINT32                  device_num;
    btif_hh_added_device_t  added_devices[BTIF_HH_MAX_ADDED_DEV];
    btif_hh_device_t        *p_curr_dev;
} btif_hh_cb_t;

#if defined(MTK_COMMON) && (MTK_COMMON == TRUE)
/* Check MTK HID driver kernel version, if it lager than 3.10 return true.*/
#if ((defined(MTK_HID_DRIVER_KERNEL_3_18) && (MTK_HID_DRIVER_KERNEL_3_18 == TRUE)) || \
        (defined(MTK_HID_DRIVER_KERNEL_4_4) && (MTK_HID_DRIVER_KERNEL_4_4 == TRUE)))
#ifndef MTK_HID_DRIVER_KERNEL_LARGER_THAN_3_10
#define MTK_HID_DRIVER_KERNEL_LARGER_THAN_3_10    TRUE
#endif
#endif
#endif

/*******************************************************************************
**  Functions
********************************************************************************/

extern btif_hh_cb_t btif_hh_cb;

extern btif_hh_device_t *btif_hh_find_connected_dev_by_handle(UINT8 handle);
extern void btif_hh_remove_device(bt_bdaddr_t bd_addr);
BOOLEAN btif_hh_add_added_dev(bt_bdaddr_t bda, tBTA_HH_ATTR_MASK attr_mask);
extern bt_status_t btif_hh_virtual_unplug(bt_bdaddr_t *bd_addr);
extern void btif_hh_disconnect(bt_bdaddr_t *bd_addr);
extern void btif_hh_setreport(btif_hh_device_t *p_dev, bthh_report_type_t r_type,
                    UINT16 size, UINT8* report);

BOOLEAN btif_hh_add_added_dev(bt_bdaddr_t bd_addr, tBTA_HH_ATTR_MASK attr_mask);

#endif

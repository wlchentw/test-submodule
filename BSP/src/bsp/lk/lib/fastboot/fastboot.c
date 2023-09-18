/* Copyright Statement:
 * Copyright (c) 2016 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <debug.h>
#include <dev/udc.h>
#include <err.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <platform/udc-common.h>
#include <string.h>
#include <stdlib.h>
#include <platform/mtk_timer.h>
#include <platform/bd71828.h>
#include <platform.h>

#include <platform/ntx_hw.h>
#include "fastboot.h"
#include "../../platform/mt8512/drivers/gpio/gpio.h"

#define MAX_RSP_SIZE 64
/* MAX_USBFS_BULK_SIZE: if use USB3 QMU GPD mode: cannot exceed 63 * 1024 */
#define MAX_USBFS_BULK_SIZE (16 * 1024)

static event_t usb_online;
static event_t txn_done;
static struct udc_endpoint *in, *out;
static struct udc_request *req;
int txn_status;

void *download_base;
unsigned download_max;
unsigned download_size;
unsigned fastboot_state = STATE_OFFLINE;

timer_t wdt_timer;
struct fastboot_cmd *cmdlist;
extern BOOT_ARGUMENT_T *g_boot_arg;

static void req_complete(struct udc_request *req, unsigned actual, int status)
{
    txn_status = status;
    req->length = actual;
    event_signal(&txn_done, 0);
}

void fastboot_register(const char *prefix,
                       void (*handle)(const char *arg, void *data, unsigned sz))
{
    struct fastboot_cmd *cmd;

    cmd = malloc(sizeof(*cmd));
    if (cmd) {
        cmd->prefix = prefix;
        cmd->prefix_len = strlen(prefix);
        cmd->handle = handle;
        cmd->next = cmdlist;
        cmdlist = cmd;
    }
}

struct fastboot_var *varlist;

void fastboot_publish(const char *name, const char *value)
{
    struct fastboot_var *var;
    var = malloc(sizeof(*var));
    if (var) {
        var->name = name;
        var->value = value;
        var->next = varlist;
        varlist = var;
    }
}

int usb_read(void *_buf, unsigned len)
{
    int r;
    unsigned xfer;
    unsigned char *buf = _buf;
    int count = 0;

    if (fastboot_state == STATE_ERROR)
        goto oops;

    while (len > 0) {
        xfer = (len > MAX_USBFS_BULK_SIZE) ? MAX_USBFS_BULK_SIZE : len;
        req->buffer = buf;
        req->length = xfer;
        req->complete = req_complete;
        r = udc_request_queue(out, req);
        if (r < 0) {
            dprintf(INFO, "usb_read() queue failed\n");
            goto oops;
        }
        event_wait(&txn_done);

        if (txn_status < 0) {
            dprintf(INFO, "usb_read() transaction failed\n");
            goto oops;
        }

        count += req->length;
        buf += req->length;
        len -= req->length;

        /* short transfer? */
        if (req->length != xfer) break;
    }

    return count;

oops:
    fastboot_state = STATE_ERROR;
    return -1;
}

int usb_write(void *buf, unsigned len)
{
    int r;

    if (fastboot_state == STATE_ERROR)
        goto oops;

    req->buffer = buf;
    req->length = len;
    req->complete = req_complete;
    r = udc_request_queue(in, req);
    if (r < 0) {
        dprintf(INFO, "usb_write() queue failed\n");
        goto oops;
    }
    event_wait(&txn_done);
    if (txn_status < 0) {
        dprintf(INFO, "usb_write() transaction failed\n");
        goto oops;
    }
    return req->length;

oops:
    fastboot_state = STATE_ERROR;
    return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
    char *response;

    //if (fastboot_state != STATE_COMMAND)
        //return;

    if (reason == 0)
        reason = "";

    response = (char *)memalign(CACHE_LINE, MAX_RSP_SIZE);
    if (!response) {
        dprintf(CRITICAL, "fastboot: can't allocate memory\n");
        return;
    }
    snprintf(response, MAX_RSP_SIZE, "%s%s", code, reason);
    fastboot_state = STATE_COMPLETE;

    usb_write(response, strlen(response));
    free(response);

}

void fastboot_info(const char *reason)
{
    char *response;

    if (fastboot_state != STATE_COMMAND)
        return;

    if (reason == 0)
        return;

    response = (char *)memalign(CACHE_LINE, MAX_RSP_SIZE);
    if (!response) {
        dprintf(CRITICAL, "fastboot: can't allocate memory\n");
        return;
    }
    snprintf(response, MAX_RSP_SIZE, "INFO%s", reason);

    usb_write(response, strlen(response));
    free(response);
}

void fastboot_fail(const char *reason)
{
    fastboot_ack("FAIL", reason);
    /* add debug log */
    dprintf(ALWAYS, "%s\n", reason);
}

void fastboot_okay(const char *info)
{
    fastboot_ack("OKAY", info);
}

static int fastboot_command_loop(void)
{
    struct fastboot_cmd *cmd;
    int r;
    char *buffer = (char *)memalign(CACHE_LINE, MAX_RSP_SIZE);

    if (!buffer) {
        dprintf(CRITICAL, "fastboot: can't allocate memory\n");
        return ERR_NO_MEMORY;
    }
    dprintf(ALWAYS,"fastboot: processing commands\n");

again:
    while ((fastboot_state != STATE_ERROR) && (fastboot_state != STATE_RETURN)) {
        r = usb_read(buffer, MAX_RSP_SIZE);
        if (r < 0) break; /* no input command */
        buffer[r] = 0;
        dprintf(ALWAYS, "fastboot: %s[len:%d]\n", buffer, r);
        dprintf(ALWAYS, "fastboot:[download_base:%p][download_size:0x%x]\n",download_base,(unsigned int)download_size);

        /* Pick up matched command and handle it */
        for (cmd = cmdlist; cmd; cmd = cmd->next) {
            if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
                continue;
            fastboot_state = STATE_COMMAND;
            dprintf(ALWAYS,"fastboot:[cmd:%s]-[arg:%s]\n", cmd->prefix,  buffer + cmd->prefix_len);
            cmd->handle((const char *) buffer + cmd->prefix_len, (void *) download_base, download_size);
            if (fastboot_state == STATE_COMMAND)
                fastboot_fail("unknown reason");
            goto again;
        }
        dprintf(ALWAYS,"[unknown command]*[%s]*\n", buffer);
        fastboot_fail("unknown command");

    }
    if (fastboot_state != STATE_RETURN)
        fastboot_state = STATE_OFFLINE;
    dprintf(ALWAYS,"fastboot: oops!\n");
    free(buffer);
    return fastboot_state;
}

static int fastboot_handler(void *arg)
{
    int status = 0;
	thread_t *t = get_current_thread();

    while (status != STATE_RETURN) {
        event_wait(&usb_online);
        status = fastboot_command_loop();
		/*
		if(fastboot_state == STATE_OFFLINE) {
			dprintf(ALWAYS,"fastboot offline, return!\n");
			return 0;
		}
		*/
    }
    return 0;
}

static void fastboot_notify(struct udc_gadget *gadget, unsigned event)
{
    if (event == UDC_EVENT_ONLINE) {
        event_signal(&usb_online, 0);
    } else if (event == UDC_EVENT_OFFLINE) {
        event_unsignal(&usb_online);
    }
}

static struct udc_endpoint *fastboot_endpoints[2];

static struct udc_gadget fastboot_gadget = {
    .notify     = fastboot_notify,
    .ifc_class  = 0xff,
    .ifc_subclass   = 0x42,
    .ifc_protocol   = 0x03,
    .ifc_endpoints  = 2,
    .ifc_string = "fastboot",
    .ept        = fastboot_endpoints,
};

int fastboot_init(void *base, unsigned size, int mode)
{
    thread_t *thr;
    NTX_HWCONFIG *hwcfg = gethwconfig();
    dprintf(ALWAYS, "fastboot_init()\n");
    download_base = base;
    download_max = size;

    event_init(&usb_online, 0, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&txn_done, 0, EVENT_FLAG_AUTOUNSIGNAL);

#if MT8516
    in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
#endif
#if (MT8518 || MT8512)
    in = udc_endpoint_alloc(UDC_BULK_IN, 512);
#endif

    if (!in)
        goto fail_alloc_in;
#if MT8516
    out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
#endif
#if (MT8518 || MT8512)
    out = udc_endpoint_alloc(UDC_BULK_OUT, 512);
#endif
    if (!out)
        goto fail_alloc_out;

    fastboot_endpoints[0] = in;
    fastboot_endpoints[1] = out;

    req = udc_request_alloc();
    if (!req)
        goto fail_alloc_req;

    if (udc_register_gadget(&fastboot_gadget))
        goto fail_udc_register;

    thr = thread_create("fastboot", fastboot_handler, 0, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    if (!thr) {
        goto fail_alloc_in;
    }
    thread_resume(thr);

    udc_start();
	if(mode & 0x03) {
		thread_join(thr, NULL, INFINITE_TIME);
	}
	else {
		const int iDelaymsPerTry = 50, iMaxTry = 400;
		int i;
		int iLedEnterState = ntx_led(NTX_ON_LED,-1);
		int iLedState = iLedEnterState;
		int iPwrKeyState = -1,iPwrKeyLastState = -1;
		int iSkipPwrKey=0;

		dprintf(ALWAYS, "led state=%d\n",iLedEnterState);
		

		for(i=0;i<iMaxTry;i++) {

			iPwrKeyState = bd71828_powerkey_status();
			if(iPwrKeyLastState!=iPwrKeyState) {
				dprintf(ALWAYS, "power key %d->%d \n",
					iPwrKeyLastState,iPwrKeyState);
				if( (-1==iPwrKeyLastState) && (2==iPwrKeyState) ) {
					// force long power key boot.
					iSkipPwrKey=1;
				}
			}
			iPwrKeyLastState=iPwrKeyState;

			if(iPwrKeyState>=0) {
				g_boot_arg->powerkey_status = iPwrKeyState;
                save_updown_key_status();
				if( (0==iSkipPwrKey) && (2==iPwrKeyState) && (i>10) /*flash at least 10 times*/) {
					// Middle power key 
					dprintf(ALWAYS, "power key middle press !\n");
					break;
				}
			}

			if(!g_boot_arg->powerkey_status) {
				dprintf(ALWAYS, "power key released !\n");
				break;
			}

			if(0xc==hwcfg->m_val.bKeyPad) {
				// no keypad .
				// led invert .
				iLedState = (iLedState==1)? 0:1;
				ntx_led(NTX_ON_LED,iLedState);
			}


			mdelay(iDelaymsPerTry);
			if(fastboot_state == STATE_COMMAND || fastboot_state == STATE_COMPLETE) {
				dprintf(ALWAYS, "fastboot_state=%d\n",fastboot_state);
				thread_join(thr, NULL, INFINITE_TIME);
			}
		}

		if(0xc==hwcfg->m_val.bKeyPad) {
			// restore the led state .
			dprintf(ALWAYS, "restore led state=%d\n",iLedEnterState);
			ntx_led(NTX_ON_LED,iLedEnterState);
		}
		else {
			dprintf(ALWAYS, "invert led state=>\n",iLedEnterState?0:1);
			ntx_led(NTX_ON_LED,iLedEnterState?0:1);
		}
	}
    udc_stop();

    return 0;

fail_udc_register:
    udc_request_free(req);
fail_alloc_req:
    udc_endpoint_free(out);
fail_alloc_out:
    udc_endpoint_free(in);
fail_alloc_in:
    return -1;
}


/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 *
 * MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/*
 * Event loop based on select() loop
 * Copyright (c) 2002-2005, Jouni Malinen <j@w1.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include <sys/stat.h>
#include <grp.h>
#include <stddef.h>
//#include <cutils/sockets.h>
//#include <android/log.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <dirent.h>
//#include <cutils/properties.h>
#include <sys/un.h>
#include <dirent.h>
#include <linux/limits.h>
//#include <cutils/sockets.h>
//#include <cutils/memory.h>

#include "os_linux.h"
#include "stp_dump.h"
#include "eloop.h"

struct eloop_sock {
    int sock;
    void *eloop_data;
    void *user_data;
    eloop_sock_handler handler;
};

struct eloop_timeout {
    struct os_time time;
    void *eloop_data;
    void *user_data;
    eloop_timeout_handler handler;
    struct eloop_timeout *next;
};

struct eloop_signal {
    int sig;
    void *user_data;
    eloop_signal_handler handler;
    int signaled;
};

struct eloop_sock_table {
    int count;
    struct eloop_sock *table;
    int changed;
};

struct eloop_data {
    void *user_data;

    int max_sock;

    struct eloop_sock_table readers;
    struct eloop_sock_table writers;
    struct eloop_sock_table exceptions;

    struct eloop_timeout *timeout;

    int signal_count;
    struct eloop_signal *signals;
    int signaled;
    int pending_terminate;

    int terminate;
    int reader_table_changed;
};

static struct eloop_data eloop;


int eloop_init(void *user_data)
{
    os_memset(&eloop, 0, sizeof(eloop));
    eloop.user_data = user_data;
    return 0;
}


static int eloop_sock_table_add_sock(struct eloop_sock_table *table,
                                     int sock, eloop_sock_handler handler,
                                     void *eloop_data, void *user_data)
{
    struct eloop_sock *tmp;

    if (table == NULL)
    {
        ALOGI("table = NULL\n");
        return -1;
    }
    tmp = (struct eloop_sock *)
        os_realloc(table->table,
               (table->count + 1) * sizeof(struct eloop_sock));
    if (tmp == NULL)
    {
		ALOGI( "memory allocation for eloop_sock failed\n");
        return -1;
    }
    tmp[table->count].sock = sock;
    tmp[table->count].eloop_data = eloop_data;
    tmp[table->count].user_data = user_data;
    tmp[table->count].handler = handler;
    table->count++;
    table->table = tmp;
    if (sock > eloop.max_sock)
    {
        eloop.max_sock = sock;
		ALOGI("set sock number to %d\n", eloop.max_sock);
    }
    table->changed = 1;

    return 0;
}


static void eloop_sock_table_remove_sock(struct eloop_sock_table *table,
                                         int sock)
{
    int i;

    if (table == NULL || table->table == NULL || table->count == 0)
        return;

    for (i = 0; i < table->count; i++) {
        if (table->table[i].sock == sock)
            break;
    }
    if (i == table->count)
        return;
    if (i != table->count - 1) {
        os_memmove(&table->table[i], &table->table[i + 1],
               (table->count - i - 1) *
               sizeof(struct eloop_sock));
    }
    table->count--;
    table->changed = 1;
}


static void eloop_sock_table_set_fds(struct eloop_sock_table *table,
                     fd_set *fds)
{
    int i;

    FD_ZERO(fds);

    if (table->table == NULL)
        return;

    for (i = 0; i < table->count; i++)
        FD_SET(table->table[i].sock, fds);
}


static void eloop_sock_table_dispatch(struct eloop_sock_table *table,
                      fd_set *fds)
{
    int i;

    if (table == NULL || table->table == NULL)
    {
        /*stp_printf(MSG_ERROR, "table == NULL || table->table == NULL\n");*/
        return;
    }
    table->changed = 0;
    for (i = 0; i < table->count; i++) {
        if (FD_ISSET(table->table[i].sock, fds)) {
            table->table[i].handler(table->table[i].sock,
                        table->table[i].eloop_data,
                        table->table[i].user_data);
            if (table->changed)
                break;
        }
    }
}


static void eloop_sock_table_destroy(struct eloop_sock_table *table)
{
    if (table) {
        int i;
        for (i = 0; i < table->count && table->table; i++) {
            printf("ELOOP: remaining socket: sock=%d "
                   "eloop_data=%p user_data=%p handler=%p\n",
                   table->table[i].sock,
                   table->table[i].eloop_data,
                   table->table[i].user_data,
                   table->table[i].handler);
        }
        os_free(table->table);
    }
}


int eloop_register_read_sock(int sock, eloop_sock_handler handler,
                 void *eloop_data, void *user_data)
{
    return eloop_register_sock(sock, EVENT_TYPE_READ, handler,
                   eloop_data, user_data);
}


void eloop_unregister_read_sock(int sock)
{
    eloop_unregister_sock(sock, EVENT_TYPE_READ);
}


static struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type)
{
    switch (type) {
    case EVENT_TYPE_READ:
        return &eloop.readers;
    case EVENT_TYPE_WRITE:
        return &eloop.writers;
    case EVENT_TYPE_EXCEPTION:
        return &eloop.exceptions;
    }
    ALOGE("tyep (%d) error\n", type);
    return NULL;
}


int eloop_register_sock(int sock, eloop_event_type type,
            eloop_sock_handler handler,
            void *eloop_data, void *user_data)
{
    struct eloop_sock_table *table;

    table = eloop_get_sock_table(type);
    return eloop_sock_table_add_sock(table, sock, handler,
                     eloop_data, user_data);
}


void eloop_unregister_sock(int sock, eloop_event_type type)
{
    struct eloop_sock_table *table;

    table = eloop_get_sock_table(type);
    eloop_sock_table_remove_sock(table, sock);
}


int eloop_register_timeout(unsigned int secs, unsigned int usecs,
               eloop_timeout_handler handler,
               void *eloop_data, void *user_data)
{
    struct eloop_timeout *timeout, *tmp, *prev;

    timeout = os_malloc(sizeof(*timeout));
    if (timeout == NULL)
        return -1;
    if (os_get_time(&timeout->time) < 0) {
        os_free(timeout);
        return -1;
    }
    timeout->time.sec += secs;
    timeout->time.usec += usecs;
    while (timeout->time.usec >= 1000000) {
        timeout->time.sec++;
        timeout->time.usec -= 1000000;
    }
    timeout->eloop_data = eloop_data;
    timeout->user_data = user_data;
    timeout->handler = handler;
    timeout->next = NULL;

    if (eloop.timeout == NULL) {
        eloop.timeout = timeout;
        return 0;
    }

    prev = NULL;
    tmp = eloop.timeout;
    while (tmp != NULL) {
        if (os_time_before(&timeout->time, &tmp->time))
            break;
        prev = tmp;
        tmp = tmp->next;
    }

    if (prev == NULL) {
        timeout->next = eloop.timeout;
        eloop.timeout = timeout;
    } else {
        timeout->next = prev->next;
        prev->next = timeout;
    }

    return 0;
}


int eloop_cancel_timeout(eloop_timeout_handler handler,
             void *eloop_data, void *user_data)
{
    struct eloop_timeout *timeout, *prev, *next;
    int removed = 0;

    prev = NULL;
    timeout = eloop.timeout;
    while (timeout != NULL) {
        next = timeout->next;

        if (timeout->handler == handler &&
            (timeout->eloop_data == eloop_data ||
             eloop_data == ELOOP_ALL_CTX) &&
            (timeout->user_data == user_data ||
             user_data == ELOOP_ALL_CTX)) {
            if (prev == NULL)
                eloop.timeout = next;
            else
                prev->next = next;
            os_free(timeout);
            removed++;
        } else
            prev = timeout;

        timeout = next;
    }

    return removed;
}


int eloop_is_timeout_registered(eloop_timeout_handler handler,
                void *eloop_data, void *user_data)
{
    struct eloop_timeout *tmp;

    tmp = eloop.timeout;
    while (tmp != NULL) {
        if (tmp->handler == handler &&
            tmp->eloop_data == eloop_data &&
            tmp->user_data == user_data)
            return 1;

        tmp = tmp->next;
    }

    return 0;
}


#ifndef CONFIG_NATIVE_WINDOWS
static void eloop_handle_alarm(int sig)
{
    fprintf(stderr, "eloop: could not process SIGINT or SIGTERM in two "
        "seconds. Looks like there\n"
        "is a bug that ends up in a busy loop that "
        "prevents clean shutdown.\n"
        "Killing program forcefully.\n");
    exit(1);
}
#endif /* CONFIG_NATIVE_WINDOWS */


static void eloop_handle_signal(int sig)
{
    int i;

#ifndef CONFIG_NATIVE_WINDOWS
    if ((sig == SIGINT || sig == SIGTERM) && !eloop.pending_terminate) {
        /* Use SIGALRM to break out from potential busy loops that
         * would not allow the program to be killed. */
        eloop.pending_terminate = 1;
        signal(SIGALRM, eloop_handle_alarm);
        alarm(2);
    }
#endif /* CONFIG_NATIVE_WINDOWS */

    eloop.signaled++;
    for (i = 0; i < eloop.signal_count; i++) {
        if (eloop.signals[i].sig == sig) {
            eloop.signals[i].signaled++;
            break;
        }
    }
}


static void eloop_process_pending_signals(void)
{
    int i;

    if (eloop.signaled == 0)
        return;
    eloop.signaled = 0;

    if (eloop.pending_terminate) {
#ifndef CONFIG_NATIVE_WINDOWS
        alarm(0);
#endif /* CONFIG_NATIVE_WINDOWS */
        eloop.pending_terminate = 0;
    }

    for (i = 0; i < eloop.signal_count; i++) {
        if (eloop.signals[i].signaled) {
            eloop.signals[i].signaled = 0;
            eloop.signals[i].handler(eloop.signals[i].sig,
                         eloop.user_data,
                         eloop.signals[i].user_data);
        }
    }
}


int eloop_register_signal(int sig, eloop_signal_handler handler,
              void *user_data)
{
    struct eloop_signal *tmp;

    tmp = (struct eloop_signal *)
        os_realloc(eloop.signals,
               (eloop.signal_count + 1) *
               sizeof(struct eloop_signal));
    if (tmp == NULL)
        return -1;

    tmp[eloop.signal_count].sig = sig;
    tmp[eloop.signal_count].user_data = user_data;
    tmp[eloop.signal_count].handler = handler;
    tmp[eloop.signal_count].signaled = 0;
    eloop.signal_count++;
    eloop.signals = tmp;
    signal(sig, eloop_handle_signal);

    return 0;
}


int eloop_register_signal_terminate(eloop_signal_handler handler,
                    void *user_data)
{
    int ret = eloop_register_signal(SIGINT, handler, user_data);
    if (ret == 0)
        ret = eloop_register_signal(SIGTERM, handler, user_data);
    if (ret == 0)
        ret = eloop_register_signal(SIGSEGV, handler, user_data);
    return ret;
}


int eloop_register_signal_reconfig(eloop_signal_handler handler,
                   void *user_data)
{
#ifdef CONFIG_NATIVE_WINDOWS
    return 0;
#else /* CONFIG_NATIVE_WINDOWS */
    return eloop_register_signal(SIGHUP, handler, user_data);
#endif /* CONFIG_NATIVE_WINDOWS */
}


void eloop_run(void)
{
    fd_set *rfds, *wfds, *efds;
    int res;
    struct timeval _tv;
    struct os_time tv, now;

    rfds = os_malloc(sizeof(*rfds));
    wfds = os_malloc(sizeof(*wfds));
    efds = os_malloc(sizeof(*efds));
    if (rfds == NULL || wfds == NULL || efds == NULL) {
        ALOGE("eloop_run - malloc failed\n");
        goto out;
    }

    while (!eloop.terminate &&
           (eloop.timeout || eloop.readers.count > 0 ||
        eloop.writers.count > 0 || eloop.exceptions.count > 0)) {
        if (eloop.timeout) {
            os_get_time(&now);
            if (os_time_before(&now, &eloop.timeout->time))
                os_time_sub(&eloop.timeout->time, &now, &tv);
            else
                tv.sec = tv.usec = 0;
#if 1
            printf("next timeout in %lu.%06lu sec\n",
                   tv.sec, tv.usec);
#endif
            _tv.tv_sec = tv.sec;
            _tv.tv_usec = tv.usec;
        }

        eloop_sock_table_set_fds(&eloop.readers, rfds);
        eloop_sock_table_set_fds(&eloop.writers, wfds);
        eloop_sock_table_set_fds(&eloop.exceptions, efds);
        res = select(eloop.max_sock + 1, rfds, wfds, efds,
                 eloop.timeout ? &_tv : NULL);
        if (res < 0 && errno != EINTR && errno != 0) {
            perror("select");
			ALOGE("select on socket failed\n");
            goto out;
        }
        eloop_process_pending_signals();

        /* check if some registered timeouts have occurred */
        if (eloop.timeout) {
            struct eloop_timeout *tmp;

            os_get_time(&now);
            if (!os_time_before(&now, &eloop.timeout->time)) {
                tmp = eloop.timeout;
                eloop.timeout = eloop.timeout->next;
                tmp->handler(tmp->eloop_data,
                         tmp->user_data);
                os_free(tmp);
            }

        }

        if (res <= 0)
            continue;

        eloop_sock_table_dispatch(&eloop.readers, rfds);
        eloop_sock_table_dispatch(&eloop.writers, wfds);
        eloop_sock_table_dispatch(&eloop.exceptions, efds);
    }

out:
	ALOGI("eloop_run exit\n");
    os_free(rfds);
    os_free(wfds);
    os_free(efds);
}


void eloop_terminate(void)
{
    eloop.terminate = 1;
}


void eloop_destroy(void)
{
    struct eloop_timeout *timeout, *prev;
    struct os_time now;

    timeout = eloop.timeout;
    if (timeout)
        os_get_time(&now);
    while (timeout != NULL) {
        int sec, usec;
        prev = timeout;
        timeout = timeout->next;
        sec = prev->time.sec - now.sec;
        usec = prev->time.usec - now.usec;
        if (prev->time.usec < now.usec) {
            sec--;
            usec += 1000000;
        }
        printf("ELOOP: remaining timeout: %d.%06d eloop_data=%p "
               "user_data=%p handler=%p\n",
               sec, usec, prev->eloop_data, prev->user_data,
               prev->handler);
        os_free(prev);
    }
    eloop_sock_table_destroy(&eloop.readers);
    eloop_sock_table_destroy(&eloop.writers);
    eloop_sock_table_destroy(&eloop.exceptions);
    os_free(eloop.signals);
}


int eloop_terminated(void)
{
    return eloop.terminate;
}


void eloop_wait_for_read_sock(int sock)
{
    fd_set rfds;

    if (sock < 0)
        return;

    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    select(sock + 1, &rfds, NULL, NULL, NULL);
}


void * eloop_get_user_data(void)
{
    return eloop.user_data;
}

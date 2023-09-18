/*
 * Copyright (c) 2018 MediaTek Inc.
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

/**
 * @file mtk_wrapper.h
 * @brief Header file of LK 2.0 wrapper for post-init
 *
 * LK 2.0 wrapper includes interface functions, which provides software
 * environment functions. Several interfaces, virtual/physical address
 * translation, muxtex/semaphore and debug are provided to execute the
 * corresponding system calls.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <kernel/mutex.h>
#include <kernel/semaphore.h>
#include <kernel/vm.h>
#include <debug.h>
#include <platform.h>

#ifndef __REG_H
/* low level macros for accessing memory mapped hardware registers */
#define REG64(addr) ((volatile uint64_t *)(uintptr_t)(addr))
#define REG32(addr) ((volatile uint32_t *)(uintptr_t)(addr))
#define REG16(addr) ((volatile uint16_t *)(uintptr_t)(addr))
#define REG8(addr) ((volatile uint8_t *)(uintptr_t)(addr))

#define RMWREG64(addr, startbit, width, val) *REG64(addr) = (*REG64(addr) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit))
#define RMWREG32(addr, startbit, width, val) *REG32(addr) = (*REG32(addr) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit))
#define RMWREG16(addr, startbit, width, val) *REG16(addr) = (*REG16(addr) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit))
#define RMWREG8(addr, startbit, width, val) *REG8(addr) = (*REG8(addr) & ~(((1<<(width)) - 1) << (startbit))) | ((val) << (startbit))

#define writel(v, a) (*REG32(a) = (v))
#define readl(a) (*REG32(a))
#define writeb(v, a) (*REG8(a) = (v))
#define readb(a) (*REG8(a))
#endif

#ifndef __DEBUG_H
#if !defined(LK_DEBUGLEVEL)
#define LK_DEBUGLEVEL 0
#endif

/* debug levels */
#define CRITICAL 0
#define ALWAYS 0
#define INFO 1
#define SPEW 2

#define dprintf(level, x...) do { if ((level) <= LK_DEBUGLEVEL) { _dprintf(x); } } while (0)
#endif

enum wrap_handler_return {
    WRAP_INT_NO_RESCHEDULE = 0,
    WRAP_INT_RESCHEDULE,
};

typedef mutex_t wrap_mutex;
typedef semaphore_t wrap_sem;
typedef enum wrap_handler_return (*wrap_int_handler)(void *arg);

//mutex interface functions
void plat_wrap_mutex_init(wrap_mutex *m);
void plat_wrap_mutex_destroy(wrap_mutex *m);
status_t plat_wrap_mutex_acquire_timeout(wrap_mutex *m, uint32_t timeout);
status_t plat_wrap_mutex_acquire(wrap_mutex *m);
status_t plat_wrap_mutex_release(wrap_mutex *m);
//semaphore interface functions
void plat_wrap_sem_init(wrap_sem *sem, unsigned int value);
void plat_wrap_sem_destroy(wrap_sem *sem);
int plat_wrap_sem_post(wrap_sem *sem);
status_t plat_wrap_sem_wait(wrap_sem *sem);
status_t plat_wrap_sem_trywait(wrap_sem *sem);
status_t plat_wrap_sem_timedwait(wrap_sem *sem, uint32_t timeout);
//virtual/physical address translation function
void *plat_wrap_paddr_to_kvaddr(unsigned long pa);
unsigned long plat_wrap_kvaddr_to_paddr(void *ptr);
//system delay function
void plat_wrap_delay(uint32_t usecs);
//interrupt function
void plat_wrap_register_int_handler(unsigned int vector, wrap_int_handler handler, void *arg);
int plat_wrap_unmask_interrupt(unsigned int vector);
int plat_wrap_mask_interrupt(unsigned int vector);
void plat_wrap_irq_set_polarity(unsigned int vector, unsigned int polarity);
void plat_wrap_irq_set_sensitive(unsigned int vector, unsigned int sens);

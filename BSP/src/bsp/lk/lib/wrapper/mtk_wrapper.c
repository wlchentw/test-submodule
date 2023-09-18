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

#include <mtk_wrapper.h>
#include <trace.h>
#include <platform/interrupts.h>
#include <platform/mt_irq.h>

#define LOCAL_TRACE 0

/* mutex*/
void plat_wrap_mutex_init(wrap_mutex *m)
{
    LTRACEF("init mutex\n");

    mutex_init(m);
}

void plat_wrap_mutex_destroy(wrap_mutex *m)
{
    LTRACEF("destroy mutex\n");

    mutex_destroy(m);
}

status_t plat_wrap_mutex_acquire_timeout(wrap_mutex *m, uint32_t timeout)
{
    LTRACEF("mutex acquire timeout\n");

    return mutex_acquire_timeout(m, timeout);
}

status_t plat_wrap_mutex_acquire(wrap_mutex *m)
{
    LTRACEF("mutex acquire\n");

    return mutex_acquire(m);
}

status_t plat_wrap_mutex_release(wrap_mutex *m)
{
    LTRACEF("release mutex\n");

    return mutex_release(m);
}

/* semaphore*/
void plat_wrap_sem_init(wrap_sem *sem, unsigned int value)
{
    LTRACEF("init semaphore\n");

    sem_init(sem, value);
}

void plat_wrap_sem_destroy(wrap_sem *sem)
{
    LTRACEF("destroy semaphore\n");

    sem_destroy(sem);
}

int plat_wrap_sem_post(wrap_sem *sem)
{
    LTRACEF("semaphore post\n");

    return sem_post(sem, true);
}

status_t plat_wrap_sem_wait(wrap_sem *sem)
{
    LTRACEF("semaphore wait\n");

    return sem_wait(sem);
}

status_t plat_wrap_sem_trywait(wrap_sem *sem)
{
    LTRACEF("semaphore trywait\n");

    return sem_trywait(sem);
}

status_t plat_wrap_sem_timedwait(wrap_sem *sem, uint32_t timeout)
{
    LTRACEF("semaphore timedwait\n");

    return sem_timedwait(sem, timeout);
}

/* vm*/
void *plat_wrap_paddr_to_kvaddr(unsigned long pa)
{
    LTRACEF("physical address translate to virtual address\n");

    return paddr_to_kvaddr(pa);
}

unsigned long plat_wrap_kvaddr_to_paddr(void *ptr)
{
    LTRACEF("physical address translate to virtual address\n");

    return kvaddr_to_paddr(ptr);
}

/* timer*/
//delay usecs
void plat_wrap_delay(uint32_t usecs)
{
    LTRACEF("system delay\n");

    spin(usecs);
}

/* interrupt*/
void plat_wrap_register_int_handler(unsigned int vector, wrap_int_handler handler, void *arg)
{
    LTRACEF("register interrupt\n");

    register_int_handler(vector, (int_handler)handler, arg);
}

int plat_wrap_unmask_interrupt(unsigned int vector)
{
    LTRACEF("enable interrupt\n");

    return unmask_interrupt(vector);
}

int plat_wrap_mask_interrupt(unsigned int vector)
{
    LTRACEF("disable interrupt\n");

    return mask_interrupt(vector);
}

void plat_wrap_irq_set_polarity(unsigned int vector, unsigned int polarity)
{
    LTRACEF("set interrupt polarity\n");

    mt_irq_set_polarity(vector, polarity);
}

void plat_wrap_irq_set_sensitive(unsigned int vector, unsigned int sens)
{
    LTRACEF("set interrupt sensitive\n");

    mt_irq_set_sens(vector, sens);
}

/*
 * Copyright (c) 2017 MediaTek Inc.
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
#include <err.h>
#include <mtk_wrapper.h>
#include <stdio.h>
#include <stdlib.h>
#include <unittest.h>
#include <platform.h>

/**
 * lib/unittest defines several macros for unittest,
 * EXPECT_EQ, EXPECT_NEQ, EXPECT_LE, EXPECT_LT, EXPECT_GE, EXPECT_GT,
 * EXPECT_TRUE, EXPECT_FALSE, EXPECT_BYTES_EQ, EXPECT_BYTES_NE, EXPECT_EQ_LL,
 * ASSERT_NOT_NULL.
 * For detail, please refer to lib/unittest/include/unittest.h
 */
//irq function unittest
bool timerEnable = false;
#define ARM_GENERIC_TIMER_PHYSICAL_INT 30

static enum wrap_handler_return wrap_plat_platform_timer_handle(void *arg)
{
    timerEnable = true;
    plat_wrap_mask_interrupt(ARM_GENERIC_TIMER_PHYSICAL_INT);
    return WRAP_INT_NO_RESCHEDULE;
}

static bool test_plat_wrap_irq_function(void)
{
#define WRAP_LEVEL_SENSITIVE 1
#define WRAP_POLARITY_LOW   0
    int count = 5;

    plat_wrap_irq_set_polarity(ARM_GENERIC_TIMER_PHYSICAL_INT, WRAP_LEVEL_SENSITIVE);
    plat_wrap_irq_set_sensitive(ARM_GENERIC_TIMER_PHYSICAL_INT, WRAP_POLARITY_LOW);
    plat_wrap_register_int_handler(ARM_GENERIC_TIMER_PHYSICAL_INT, &wrap_plat_platform_timer_handle, NULL);
    plat_wrap_unmask_interrupt(ARM_GENERIC_TIMER_PHYSICAL_INT);
    while (1) {
        if (count > 0) {
            plat_wrap_delay(1000000);
            if (timerEnable) {
                break;
            }
            count --;
        } else
            return false;
    }

    return true;
}

//delay function unittest
static bool test_plat_wrap_delay_function(void)
{
    BEGIN_TEST;

    unsigned long long startTime = 0;
    unsigned long long endTime = 0;
    int delayTime = 1000000; //one second
    int delayThreshold = 500;

    startTime = current_time_hires();
    plat_wrap_delay(1000000);
    endTime = current_time_hires();
    EXPECT_LE((delayTime - delayThreshold), endTime - startTime, "delay time is less than expected value");
    EXPECT_GE((delayTime + delayThreshold), endTime - startTime, "delay time is greater than expected value");

    END_TEST;
}

//address translate function unittest
static bool test_plat_wrap_addr_trans_function(void)
{
#define EMI_PHY_BASE (0x10203000)

    BEGIN_TEST;

    unsigned long paddr;
    void *vaddr = plat_wrap_paddr_to_kvaddr(EMI_PHY_BASE);
    paddr = plat_wrap_kvaddr_to_paddr(vaddr);
    EXPECT_EQ(EMI_PHY_BASE, paddr, "fail to virtual/phyical address translate");

    END_TEST;
}

//mutex function unittest
wrap_mutex m;

static bool test_plat_wrap_mutex_function(void)
{
    BEGIN_TEST;

    plat_wrap_mutex_init(&m);
    EXPECT_EQ(NO_ERROR, plat_wrap_mutex_acquire(&m), "fail to mutex acquire");
    EXPECT_EQ(NO_ERROR, plat_wrap_mutex_release(&m), "fail to mutex release");
    plat_wrap_mutex_destroy(&m);

    END_TEST;
}

//semphore function unittest
wrap_sem sem;
static bool test_plat_wrap_sem_function(void)
{
    BEGIN_TEST;

    unsigned int value = 1;
    plat_wrap_sem_init(&sem, value);
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_wait(&sem), "fail to semaphore wait function");
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_post(&sem), "fail to semaphore post function");
    plat_wrap_sem_destroy(&sem);

    END_TEST;
}

static bool test_plat_wrap_sem_trywait_function(void)
{
    BEGIN_TEST;

    unsigned int value = 1;
    plat_wrap_sem_init(&sem, value);
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_trywait(&sem), "fail to semaphore trywait function");
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_post(&sem), "fail to semaphore post function");
    plat_wrap_sem_destroy(&sem);

    END_TEST;
}

static bool test_plat_wrap_sem_timedwait_function(void)
{
    BEGIN_TEST;

    unsigned int value = 1;
    uint32_t timeout = 1;
    plat_wrap_sem_init(&sem, value);
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_timedwait(&sem, timeout), "fail to semaphore timedwait function");
    EXPECT_EQ(NO_ERROR, plat_wrap_sem_post(&sem), "fail to semaphore post function");
    plat_wrap_sem_destroy(&sem);

    END_TEST;
}

BEGIN_TEST_CASE(wrapper_tests);
RUN_TEST(test_plat_wrap_irq_function);
RUN_TEST(test_plat_wrap_delay_function);
RUN_TEST(test_plat_wrap_addr_trans_function);
RUN_TEST(test_plat_wrap_mutex_function);
RUN_TEST(test_plat_wrap_sem_function);
RUN_TEST(test_plat_wrap_sem_trywait_function);
RUN_TEST(test_plat_wrap_sem_timedwait_function);
END_TEST_CASE(wrapper_tests);

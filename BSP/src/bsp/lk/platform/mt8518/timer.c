/*
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

#include <platform/mtk_timer.h>
#include <platform/mt_reg_base.h>

void set_cntfrq(unsigned long freq)
{
#if ARCH_ARM64

#else
    __asm__ volatile("mcr p15, 0, %0, c14, c0, 0\n" :: "r"(freq));
#endif
}

/* delay msec mseconds */
extern lk_time_t current_time(void);
void mdelay(unsigned long msec)
{
    lk_time_t start = current_time();

    while (start + msec > current_time());
}

/* delay usec useconds */
extern lk_bigtime_t current_time_hires(void);
void udelay(unsigned long usec)
{
    lk_bigtime_t start = current_time_hires();

    while (start + usec > current_time_hires());
}

/*
 * busy wait
 */
void gpt_busy_wait_us(u32 timeout_us)
{
    udelay(timeout_us);
}

void gpt_busy_wait_ms(u32 timeout_ms)
{
    mdelay(timeout_ms);
}

/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <arch_helpers.h>
#include <debug.h>
#include <mcucfg.h>
#include <mmio.h>
#include <mt_cpuxgpt.h>
#include <platform_def.h>


#define GPT1_CON      (APXGPT_BASE + 0x0010)
#define GPT1_CLK      (APXGPT_BASE + 0x0014)
#define GPT1_COUNT    (APXGPT_BASE + 0x0018)
#define GPT1_COMPARE  (APXGPT_BASE + 0x001C)

#define BIT_SYS_13M_CLK  (0)
#define BIT_RTC_32K_CLK  (1)


uint64_t normal_time_base;
uint64_t atf_time_base;


void sched_clock_init(uint64_t normal_base, uint64_t atf_base)
{
	normal_time_base += normal_base;
	atf_time_base = atf_base;
}

uint64_t sched_clock(void)
{
	uint64_t cval;

	cval = (((read_cntpct_el0() - atf_time_base) * 1000) / SYS_COUNTER_FREQ_IN_MHZ)
		- normal_time_base;
	return cval;
}


void setup_syscnt(void)
{
	/* maybe use
	 * arch-timer or 64-bit systimer counter
	 * as syscnt in ATF world.
	 */
}

/*
 * Return: 0 - remaining_count <= sleep_time (ms)
 * Return: 1 - remaining_count >  sleep_time (ms)
 */
int check_apxgpt1_exceed_sleep_time(unsigned int ms)
{
	unsigned int clk = 0;
	unsigned int value;
	int compare;

	clk = mmio_read_32(GPT1_CLK);
	value = mmio_read_32(GPT1_COMPARE) - mmio_read_32(GPT1_COUNT);
	if ((clk >> 4) & BIT_RTC_32K_CLK)
	{
		/* clock: 32768 count */
		compare = (int)value - ms * 32768 / 1000;
		if (compare > 0)
			return 1;
	}
	else
	{
		/* clock: 13000000 count */
		/* compare = value - ms * 13000000 / 1000; */
		compare = (int)value - ms * 13000;
		if (compare > 0)
			return 1;
	}

	return 0;
}

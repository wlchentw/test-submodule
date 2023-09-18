// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
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
#include <debug.h>
#include <mmio.h>
#include <platform.h>
#include <mt_spm.h>
#include <mt_spm_internal.h>
#include <mt_spm_reg.h>
#include <mt_spm_hotplug.h>
#include <mt_spm_mcdi.h>
#include <uart.h>
#include <mtk_plat_common.h>
#include <mcucfg.h>

#define cpu_sw_no_wait_for_q_channel	(1<<1)
#define ENABLE_HP_LOG		0

/*
 * System Power Manager (SPM) is a hardware module, which controls cpu or
 * system power for different power scenarios using different firmware.
 * This driver controls the cpu power in cpu hotplug flow.
 */

static void spm_hotplug_wfi_sel_enter(unsigned long mpidr)
{
	int core_id_val = mpidr & MPIDR_CPU_MASK;

#if ENABLE_HP_LOG
	INFO("spm_hotplug_wfi_sel_enter core_id_val=%d\n", core_id_val);
#endif

	mmio_clrbits_32(SPM_SW_RSV_1, 1 << (core_id_val + 16));
	mmio_write_32((MCUCFG_BASE + 0x1c34),
			mmio_read_32((MCUCFG_BASE + 0x1c34))
			| cpu_sw_no_wait_for_q_channel);

	/* SPM WFI Select by core number */
	switch (core_id_val) {
	case 0:
		mmio_write_32(MP0_CPU0_IRQ_MASK, 1);
		mmio_write_32(MP0_CPU0_WFI_EN, 1);
		break;
	case 1:
		mmio_write_32(MP0_CPU1_IRQ_MASK, 1);
		mmio_write_32(MP0_CPU1_WFI_EN, 1);
		break;
	default:
		break;
	}
}

static void spm_hotplug_wfi_sel_leave(unsigned long mpidr)
{
	int core_id_val = mpidr & MPIDR_CPU_MASK;

#if ENABLE_HP_LOG
	INFO("spm_hotplug_wfi_sel_leave core_id_val=%d\n", core_id_val);
#endif

	mmio_setbits_32(SPM_SW_RSV_1, 1 << (core_id_val + 16));

	switch (core_id_val) {
	case 0:
		mmio_write_32(MP0_CPU0_WFI_EN, 0);
		mmio_write_32(MP0_CPU0_IRQ_MASK, 0);
		break;
	case 1:
		mmio_write_32(MP0_CPU1_WFI_EN, 0);
		mmio_write_32(MP0_CPU1_IRQ_MASK, 0);
		break;
	default:
		break;
	}
}

#include <assert.h>
static void spm_go_to_hotplug(void)
{
//	__spm_cmd_check(PCM_DVFS_INI_CMD);

	unsigned int cmd;

	cmd = mmio_read_32(SPM_SW_RSV_6);

#if ENABLE_HP_LOG
		INFO("spm_go_to_hotplug SPM_SW_RSV_6 = 0x%x\n", cmd);
#endif

	if (cmd != PCM_DVFS_INI_CMD)
		assert(0);
}

void spm_hotplug_on(unsigned long mpidr)
{
	spm_lock_get();

#if ENABLE_HP_LOG
	console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
	INFO("spm_hotplug_on\n");
#endif
	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) & ~(0x1 << 7));
	spm_go_to_hotplug();
	/* turn on CPUx */
	spm_hotplug_wfi_sel_leave(mpidr);

#if ENABLE_HP_LOG
	console_uninit();
#endif

	spm_lock_release();
}

void spm_hotplug_off(unsigned long mpidr)
{
	spm_lock_get();

#if ENABLE_HP_LOG
	console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
	INFO("spm_hotplug_off\n");
#endif
	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) | (0x1 << 7));

	spm_go_to_hotplug();
	spm_hotplug_wfi_sel_enter(mpidr);

#if ENABLE_HP_LOG
	console_uninit();
#endif

	spm_lock_release();
}

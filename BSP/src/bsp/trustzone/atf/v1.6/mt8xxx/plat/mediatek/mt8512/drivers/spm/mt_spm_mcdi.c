// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
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
#include <console.h>
#include <debug.h>
#include <mmio.h>
#include <mt_spm.h>
#include <mt_spm_internal.h>
#include <mt_spm_reg.h>
#include <mt_spm_mcdi.h>
#include <mtk_plat_common.h>
#include <plat_pm.h>
#include <platform.h>
#include <platform_def.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * System Power Manager (SPM) is a hardware module, which controls cpu or
 * system power for different power scenarios using different firmware.
 * This driver controls the cpu power in cpu idle power saving state.
 */

static struct wake_status spm_wakesta; /* record last wakesta */
static unsigned int resource_usage;
static uint32_t idle_loop;

#define LOOP_THRESHOLD 10 /*add by wenzhen for fix build error*/

#define WAKE_SRC_FOR_MCDI \
	(WAKE_SRC_R12_KP_F32K_WAKEUP_EVENT_2_1 | \
	 WAKE_SRC_R12_WDT_32K_EVENT_B | \
	 WAKE_SRC_R12_APXGPT_EVENT_B | \
	 WAKE_SRC_R12_CONN2AP_WAKEUP_B | \
	 WAKE_SRC_R12_EINT_EVENT_B | \
	 WAKE_SRC_R12_CONN_WDT_IRQ | \
	 WAKE_SRC_R12_IRRX_WAKEUP | \
	 WAKE_SRC_R12_LOW_BATTERY_IRQ_B | \
	 WAKE_SRC_R12_DSP_WAKEUP_B | \
	 WAKE_SRC_R12_USB_CONNECT | \
	 WAKE_SRC_R12_USB_POWERDWN_B | \
	 WAKE_SRC_R12_SYS_TIMER_EVENT_B | \
	 WAKE_SRC_R12_EINT_SECURE | \
	 WAKE_SRC_R12_NIC_IRQ | \
	 WAKE_SRC_R12_AFE_IRQ_MCU_B | \
	 WAKE_SRC_R12_THERM_CTRL_EVENT_B | \
	 WAKE_SRC_R12_SYS_CIRQ_B | \
	 WAKE_SRC_R12_MSDC2_WAKEUP | \
	 WAKE_SRC_R12_DSPWDT_IRQ_B | \
	 WAKE_SRC_R12_SEJ | \
	 WAKE_SRC_R12_CPU_IRQOUTS_AND | \
	 WAKE_SRC_R12_CPU_WFI_AND_B)

static struct pwr_ctrl mcdi_ctrl = {
	.wake_src = WAKE_SRC_FOR_MCDI,
	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = WFI_OP_OR,
	.mp0_cputop_idle_mask = 1,
	.mp1_cputop_idle_mask = 1,
	.mcusys_idle_mask = 1,
	.conn_mask_b = 0x1,
	.conn_apsrc_sel = 0,
	.conn_srcclkena_sel_mask = 0x1,

	/* SPM_SRC_MASK */
	.csyspwreq_mask = 0,
	/* idle control */
	.idle_switch = 1,

	/* Auto-gen End */
};

struct spm_lp_scen __spm_mcdi = {
	.pwrctrl = &mcdi_ctrl,
};

void spm_mcdi_args(uint64_t x1, uint64_t x2, uint64_t x3)
{
	struct pwr_ctrl *pwrctrl;

	pwrctrl = __spm_mcdi.pwrctrl;
	pwrctrl->pcm_flags = x1;
	pwrctrl->pcm_flags1 = x2;

	/* get spm resource request from kernel */
	resource_usage = x3;
}

void spm_mcdi_wfi_sel_enter(unsigned long mpidr)
{
	int core_id_val = mpidr & MPIDR_CPU_MASK;

	mmio_clrbits_32(SPM_SW_RSV_1, 1 << (core_id_val + 16));

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

void spm_mcdi_wfi_sel_leave(unsigned long mpidr)
{
	int core_id_val = mpidr & MPIDR_CPU_MASK;

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

void go_to_mcdi_before_wfi_no_resume(void)
{
	struct pwr_ctrl *pwrctrl;
	uint64_t mpidr = read_mpidr();
	uint32_t cpu = platform_get_core_pos(mpidr);
	int core_id_val = mpidr & MPIDR_CPU_MASK;

	pwrctrl = __spm_mcdi.pwrctrl;

	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) | (0x1 << (core_id_val + 1)));

	__spm_set_power_control(pwrctrl);
	__spm_set_wakeup_event(pwrctrl);

	assert(mmio_read_32(SPM_SW_RSV_6) == PCM_DVFS_INI_CMD);

	if (!pwrctrl->wdt_disable)
		__spm_set_pcm_wdt(1);

	spm_mcdi_wfi_sel_enter(mpidr);

	if (pwrctrl->log_en && (idle_loop % LOOP_THRESHOLD == 0)) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		INFO("cpu%d: \"%s\", wake = 0x%x, sw = 0x%x 0x%x, req = 0x%x\n",
		     cpu, spm_get_firmware_version(), pwrctrl->wake_src,
		     pwrctrl->pcm_flags, pwrctrl->pcm_flags1,
		     mmio_read_32(SPM_SRC_REQ));
		console_uninit();
	}

}

void go_to_mcdi_after_wfi(void)
{
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl;
	uint64_t mpidr = read_mpidr();
	int core_id_val = mpidr & MPIDR_CPU_MASK;

	pwrctrl = __spm_mcdi.pwrctrl;
	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) & ~(0x1 << (core_id_val + 1)));

	if (!pwrctrl->wdt_disable)
		__spm_set_pcm_wdt(0);

	spm_mcdi_wfi_sel_leave(mpidr);
	__spm_get_wakeup_status(&spm_wakesta);
	__spm_clean_after_wakeup();

	if (pwrctrl->log_en && (idle_loop % LOOP_THRESHOLD == 0)) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		__spm_output_wake_reason(&spm_wakesta, pcmdesc);
		console_uninit();
	}

	idle_loop++;
}

void spm_mcdi(void)
{
	spm_lock_get();
	go_to_mcdi_before_wfi_no_resume();
	spm_lock_release();
}

void spm_mcdi_finish(void)
{
	spm_lock_get();
	go_to_mcdi_after_wfi();
	spm_lock_release();
}

void spm_mcdi_clear_cputop_pwrctrl_for_cluster_on(void)
{
	mmio_clrbits_32(SPM_SW_RSV_1, SPM_FLAG2_MP0_PDN);
}

void spm_mcdi_set_cputop_pwrctrl_for_cluster_off(uint32_t cpu)
{
	if (__spm_is_last_online_cpu(cpu)) {
		mmio_setbits_32(SPM_SW_RSV_1, SPM_FLAG2_MP0_PDN);
	}
}


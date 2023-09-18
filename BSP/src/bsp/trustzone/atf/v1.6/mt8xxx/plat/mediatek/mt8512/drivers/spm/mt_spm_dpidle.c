// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
#include <arch_helpers.h>
#include <console.h>
#include <debug.h>
#include <mmio.h>
#include <mt_spm.h>
#include <mt_spm_internal.h>
#include <mt_spm_reg.h>
#include <mt_spm_vcorefs.h>
#include <mtk_plat_common.h>
#include <plat_pm.h>
#include <platform.h>
#include <platform_def.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>
#include "voltage_def.h"

static struct wake_status spm_wakesta; /* record last wakesta */
static unsigned int resource_usage;
static unsigned char is_sleep_dpidle;
static unsigned int dpidle_pcm_timer_val;
static unsigned int dpidle_wake_src;
static uint32_t idle_loop;

#define CLK_CFG_4		(0x10000080)
#define CLK_CFG_UPDATE		(0x10000004)

#define MIN_GPT_TIME_IDLE	(0x80) /* 39000 / 13M ~= 3ms */
#define LOOP_THRESHOLD		(51)

#define WAKE_SRC_FOR_DPIDLE \
	(WAKE_SRC_R12_PCM_TIMER | \
	WAKE_SRC_R12_KP_F32K_WAKEUP_EVENT_2_1 | \
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
	 WAKE_SRC_R12_UART0_IRQ_B | \
	 WAKE_SRC_R12_AFE_IRQ_MCU_B | \
	 WAKE_SRC_R12_THERM_CTRL_EVENT_B | \
	 WAKE_SRC_R12_SYS_CIRQ_B | \
	 WAKE_SRC_R12_MSDC2_WAKEUP | \
	 WAKE_SRC_R12_DSPWDT_IRQ_B | \
	 WAKE_SRC_R12_SEJ)

#define DP_PCM_FLAGS \
	(SPM_FLAG_DEEPIDLE_OPTION)

static struct pwr_ctrl dpidle_ctrl = {
	.wake_src = WAKE_SRC_FOR_DPIDLE,
	.pcm_flags = DP_PCM_FLAGS,

	/* Auto-gen Start */

	/* SPM_AP_STANDBY_CON */
	.wfi_op = 0x0,
	.mp0_cputop_idle_mask = 0,
	.mp1_cputop_idle_mask = 0,
	.mcusys_idle_mask = 0,
	.mm_mask_b = 0,
	.md_ddr_en_0_dbc_en = 0,
	.md_ddr_en_1_dbc_en = 0,
	.md_mask_b = 0,
	.sspm_mask_b = 0,
	.scp_mask_b = 0,
	.srcclkeni_mask_b = 0,
	.md_apsrc_1_sel = 0,
	.md_apsrc_0_sel = 0,
	.conn_ddr_en_dbc_en = 0,
	.conn_mask_b = 0x1,
	.conn_apsrc_sel = 0,
	.conn_srcclkena_sel_mask = 0x1,

	/* SPM_SRC_REQ */
	.spm_apsrc_req = 0,
	.spm_f26m_req = 0,
	.spm_infra_req = 0,
	.spm_vrf18_req = 0,
	.spm_ddren_req = 0,
	.spm_rsv_src_req = 0,
	.spm_ddren_2_req = 0,
	.cpu_md_dvfs_sop_force_on = 0,

	/* SPM_SRC_MASK */
	.csyspwreq_mask = 0,
	.ccif0_md_event_mask_b = 0,
	.ccif0_ap_event_mask_b = 0,
	.ccif1_md_event_mask_b = 0,
	.ccif1_ap_event_mask_b = 0,
	.ccif2_md_event_mask_b = 0,
	.ccif2_ap_event_mask_b = 0,
	.ccif3_md_event_mask_b = 0,
	.ccif3_ap_event_mask_b = 0,
	.md_srcclkena_0_infra_mask_b = 0,
	.md_srcclkena_1_infra_mask_b = 0,
	.conn_srcclkena_infra_mask_b = 0x1,
	.ufs_infra_req_mask_b = 0,
	.srcclkeni_infra_mask_b = 0,
	.md_apsrc_req_0_infra_mask_b = 0,
	.md_apsrc_req_1_infra_mask_b = 0,
	.conn_apsrcreq_infra_mask_b = 0x1,
	.ufs_srcclkena_mask_b = 0,
	.md_vrf18_req_0_mask_b = 0,
	.md_vrf18_req_1_mask_b = 0,
	.ufs_vrf18_req_mask_b = 0,
	.gce_vrf18_req_mask_b = 0x1,
	.conn_infra_req_mask_b = 0x1,
	.gce_apsrc_req_mask_b = 0x1,
	.disp0_apsrc_req_mask_b = 0x1,
	.disp1_apsrc_req_mask_b = 0x1,
	.mfg_req_mask_b = 0,
	.vdec_req_mask_b = 0,
	.mcu_apsrcreq_infra_mask_b = 0x1,

	/* SPM_SRC2_MASK */
	.md_ddr_en_0_mask_b = 0,
	.md_ddr_en_1_mask_b = 0,
	.conn_ddr_en_mask_b = 0x1,
	.ddren_md32_apsrc_req_mask_b = 0,
	.ddren_scp_apsrc_req_mask_b = 0,
	.disp0_ddren_mask_b = 0x1,
	.disp1_ddren_mask_b = 0x1,
	.gce_ddren_mask_b = 0x1,
	.ddren_emi_self_refresh_ch0_mask_b = 0,
	.ddren_emi_self_refresh_ch1_mask_b = 0,
	.mcu_apsrc_req_mask_b = 0,
	.mcu_ddren_mask_b = 0,

	/* SPM_SRC3_MASK */
	.md_ddr_en_2_0_mask_b = 0,
	.md_ddr_en_2_1_mask_b = 0,
	.conn_ddr_en_2_mask_b = 0x1,
	.ddren2_md32_apsrc_req_mask_b = 0,
	.ddren2_scp_apsrc_req_mask_b = 0,
	.disp0_ddren2_mask_b = 0x1,
	.disp1_ddren2_mask_b = 0x1,
	.gce_ddren2_mask_b = 0x1,
	.ddren2_emi_self_refresh_ch0_mask_b = 0x1,
	.ddren2_emi_self_refresh_ch1_mask_b = 0x1,
	.mcu_ddren_2_mask_b = 0x1,

	/* SPARE_SRC_REQ_MASK */
	.spare1_ddren_mask_b = 0x1,
	.spare1_apsrc_req_mask_b = 0x1,
	.spare1_vrf18_req_mask_b = 0x1,
	.spare1_infra_req_mask_b = 0x1,
	.spare1_srcclkena_mask_b = 0x1,
	.spare1_ddren2_mask_b = 0x1,
	.spare2_ddren_mask_b = 0x1,
	.spare2_apsrc_req_mask_b = 0,
	.spare2_vrf18_req_mask_b = 0,
	.spare2_infra_req_mask_b = 0,
	.spare2_srcclkena_mask_b = 0,
	.spare2_ddren2_mask_b = 0x1,

	/* SPM_WAKEUP_EVENT_MASK */
	.spm_wakeup_event_mask = 0xC0000000,

	/* SPM_WAKEUP_EVENT_EXT_MASK */
	.spm_wakeup_event_ext_mask = 0xFFFFFFFF,

	/* MP0_CPU0_WFI_EN */
	.mp0_cpu0_wfi_en = 1,

	/* MP0_CPU1_WFI_EN */
	.mp0_cpu1_wfi_en = 1,

	/* MP0_CPU2_WFI_EN */
	.mp0_cpu2_wfi_en = 1,

	/* MP0_CPU3_WFI_EN */
	.mp0_cpu3_wfi_en = 1,

	/* MP1_CPU0_WFI_EN */
	.mp1_cpu0_wfi_en = 0,

	/* MP1_CPU1_WFI_EN */
	.mp1_cpu1_wfi_en = 0,

	/* MP1_CPU2_WFI_EN */
	.mp1_cpu2_wfi_en = 0,

	/* MP1_CPU3_WFI_EN */
	.mp1_cpu3_wfi_en = 0,

	/* idle control */
	.idle_switch = 1,
	//.infra1_idle_mask = 0x7A060116,
	//.infra0_idle_mask = 0x81BF8700,
	.infra1_idle_mask = 0x22040116,
	.infra0_idle_mask = 0x91BF8300,
	.infra2_idle_mask = 0x0800004B,
	.infra3_idle_mask = 0x07000780,
	.infra4_idle_mask = 0x0000008E,
	.mmsys0_idle_mask = 0x23F800FF,
	.mmsys_oth_idle_mask = 0x00000112,
	.apu_idle_mask = 0x00000312,
	.image0_idle_mask = 0x00A18915,
	.image1_idle_mask = 0x0000000E,
	.image_oth_idle_mask = 0x00000112,
	/* Auto-gen End */
};

struct spm_lp_scen __spm_dpidle = {
	.pwrctrl = &dpidle_ctrl,
};

void spm_dpidle_args(uint64_t x1, uint64_t x2, uint64_t x3)
{
	struct pwr_ctrl *pwrctrl;

	pwrctrl = __spm_dpidle.pwrctrl;
	pwrctrl->pcm_flags = x1;
	pwrctrl->pcm_flags1 = x2;

	/* get spm resource request from kernel */
	resource_usage = x3;
}

void spm_sleep_dpidle_args(uint64_t x1, uint64_t x2, uint64_t x3)
{
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

	is_sleep_dpidle = true;

	/* backup original dpidle setting */
	dpidle_pcm_timer_val = pwrctrl->timer_val;
	dpidle_wake_src = pwrctrl->wake_src;

	pwrctrl->timer_val = x1;
	pwrctrl->wake_src = x2;
}
#if 0
void loop_spm(uint32_t t)
{
	mmio_write_32(0x10006650, t);
	while(mmio_read_32(0x10006654) != t);
}
#endif

static uint32_t standby_backup;
static uint32_t flag_backup;

static int dpidle_flag = 0;

void print_log(int i)
{
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;

	if (dpidle_flag && (pwrctrl->mp1_cpu2_wfi_en == 0x1)) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		INFO("end %d\n", i);
		console_uninit();
	}
}

static void spm_dpidle_pre_process(struct pwr_ctrl *pwrctrl)
{

	pwrctrl->pcm_flags |= SPM_FLAG_DIS_VPROC_VSRAM_DVS | SPM_FLAG_DIS_BUS_CLOCK_OFF;
	pwrctrl->pcm_flags2 |= SPM_FLAG2_DIS_VCORE_SRAM_CTRL | SPM_FLAG2_DIS_VCORE_CTRL | SPM_FLAG2_ENABLE_SUSPEND_PIN;

	mmio_write_32(0x10000204, (mmio_read_32(0x10000204) | 0x1));

	mmio_write_32(0x10000068, mmio_read_32(0x10000068) & ~(0x7 << 24));
	mmio_write_32(CLK_CFG_UPDATE,
		      mmio_read_32(CLK_CFG_UPDATE) | (0x1 << 11));
	/*set srclkena0 to gpio mode*/
	mmio_write_32(0x10005240, mmio_read_32(0x10005240) & ~(0x7 << 9));
	mmio_write_32(0x10005150, mmio_read_32(0x10005150) | (0x1 << 31));
	mmio_write_32(0x100050B0, mmio_read_32(0x100050B0) | (0x1 << 31));
	/*set end*/
	standby_backup = mmio_read_32(SPM_AP_STANDBY_CON);
	/*backup vcore_dvfs flag*/
	flag_backup = spm_read(SPM_SW_RSV_1);
	mmio_write_32(0x100060B8, mmio_read_32(0x100060B8) | (0x1 << 1)); //add for don't switch suspend pin
}

static void spm_dpidle_post_process(struct pwr_ctrl *pwrctrl)
{
	/* audio intbus clk change back to normal clk */
	mmio_write_32(0x10000068, mmio_read_32(0x10000068) | (0x1 << 24));
	mmio_write_32(CLK_CFG_UPDATE,
		      mmio_read_32(CLK_CFG_UPDATE) | (0x1 << 11));

	mmio_write_32(SPM_AP_STANDBY_CON, standby_backup);
	mmio_write_32(SPM_SW_RSV_1, flag_backup);

	mmio_write_32(SPM_SW_RSV_6, PCM_DVFS_INI_CMD);
	while (mmio_read_32(SPM_SW_RSV_6) != PCM_DVFS_INI_CMD);

	mmio_write_32(0x10000204, (mmio_read_32(0x10000204) & ~0x1));
	mmio_write_32(0x10005240, mmio_read_32(0x10005240) | (0x1 << 9));
	mmio_write_32(0x100060B8, mmio_read_32(0x100060B8) & ~(0x1 << 1));  //add for don't switch suspend pin
}

void go_to_dpidle_before_wfi_no_resume(void)
{
	struct pwr_ctrl *pwrctrl;
	uint64_t mpidr = read_mpidr();
	uint32_t cpu = platform_get_core_pos(mpidr);

	pwrctrl = __spm_dpidle.pwrctrl;
	spm_dpidle_pre_process(pwrctrl);
	__spm_set_xo_type_ctrl(pwrctrl);
	__spm_set_cpu_status(cpu);
	__spm_set_power_control(pwrctrl);
	__spm_set_wakeup_event(pwrctrl);
	__spm_set_pcm_flags(pwrctrl);
	__spm_cmd_check(PCM_SUSPEMD_DPIDLE_CMD);
	if (!pwrctrl->wdt_disable)
		__spm_set_pcm_wdt(1);

	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) | 0x1);
	is_sleep_dpidle = 1;
	if (pwrctrl->mp1_cpu2_wfi_en == 0x1) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		INFO("cpu%d: \"%s\", wake = 0x%x, sw = 0x%x 0x%x 0x%x, SPM_SW_RSV_8 = 0x%x\n",
		     cpu, spm_get_firmware_version(), pwrctrl->wake_src,
		     pwrctrl->pcm_flags, pwrctrl->pcm_flags1, pwrctrl->pcm_flags2,
		     mmio_read_32(SPM_SW_RSV_8));
		console_uninit();
	}
}

static void go_to_dpidle_after_wfi(void)
{
	struct pcm_desc *pcmdesc = NULL;
	struct pwr_ctrl *pwrctrl;

	pwrctrl = __spm_dpidle.pwrctrl;
	if (!pwrctrl->wdt_disable)
		__spm_set_pcm_wdt(0);

	__spm_get_wakeup_status(&spm_wakesta);
	__spm_clean_after_wakeup();
	__spm_clean_idle_block_cnt(pwrctrl);
	is_sleep_dpidle = 0;

	mmio_write_32(SPM_SW_RSV_5, mmio_read_32(SPM_SW_RSV_5) & ~0x1);
	if (pwrctrl->mp1_cpu2_wfi_en == 0x1) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		__spm_output_wake_reason(&spm_wakesta, pcmdesc);
		INFO("SPM_SW_RSV_8 = 0x%x\n", mmio_read_32(SPM_SW_RSV_8));
		console_uninit();
	}
	spm_dpidle_post_process(pwrctrl);

	idle_loop++;
}

int spm_is_dpidle_resume(void)
{
	return is_sleep_dpidle;
}

static int spm_is_last_cpu(uint32_t cpu)
{
	int ret;

	ret = __spm_is_last_online_cpu(cpu);

	return ret;
}

static int spm_is_cmd_clr(struct pwr_ctrl *pwrctrl)
{
	uint64_t mpidr = read_mpidr();
	uint32_t cpu = platform_get_core_pos(mpidr);
	int ret = 0;

	mmio_write_32(SPM_SW_RSV_6, 0x0);/*clean MCDI flag*/

	mmio_write_32(SPM_RSV_CON, mmio_read_32(SPM_RSV_CON) & ~(0x1 << 1));

	while ((mmio_read_32(SPM_RSV_CON) & 0x2 ) != 0x2){
		if (pwrctrl->mp1_cpu3_wfi_en == 0x1){
			console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
			INFO("rsv_con\n");
			console_uninit();
		}
	}
	ret = __spm_is_last_online_cpu(cpu);

	if (!ret)
		mmio_write_32(SPM_SW_RSV_6, PCM_DVFS_INI_CMD); /*restore MCDI cmd*/

	return ret;
}

int spm_can_dpidle_enter(void)
{
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	uint64_t mpidr = read_mpidr();
	uint32_t cpu = platform_get_core_pos(mpidr);

	if ((pwrctrl->pcm_flags1_cust >> 31) == 0x1){
		spm_is_cmd_clr(pwrctrl);
		mmio_write_32(MP0_CPU0_IRQ_MASK, 1);
		mmio_write_32(MP0_CPU0_WFI_EN, 1);
		return 1;
	}
	if (!pwrctrl->idle_switch) {
		pwrctrl->by_swt++;
		goto dpidle_enter_fail;
	} else if (!__spm_does_system_boot_120s()) {
		pwrctrl->by_boot++;
		goto dpidle_enter_fail;
	} else if (__spm_is_idle_blocked_by_clk(pwrctrl)) {
		pwrctrl->by_clk++;
		goto dpidle_enter_fail;
	} else if (__spm_gpt_countdown_time() <= MIN_GPT_TIME_IDLE) {
		pwrctrl->by_gpt++;
		goto dpidle_enter_fail;
	} else if (!spm_is_last_cpu(cpu)) {
		pwrctrl->by_cpu++;
		goto dpidle_enter_fail;
	} else if (!spm_is_cmd_clr(pwrctrl)) {
		pwrctrl->mp1_cpu1_wfi_en++;
		goto dpidle_enter_fail;
	}

	if (cpu < 4)
		pwrctrl->cpu_cnt[cpu]++;

	return 1;

dpidle_enter_fail:
	return 0;
}

void spm_dpidle(void)
{
	struct pwr_ctrl *pwrctrl = __spm_dpidle.pwrctrl;
	spm_lock_get();
	if (pwrctrl->mp1_cpu2_wfi_en == 0x1) {
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		INFO("start \n");
		console_uninit();
	}
	dpidle_flag = 1;
	go_to_dpidle_before_wfi_no_resume();
	spm_lock_release();
}

void spm_dpidle_finish(void)
{
	spm_lock_get();
	go_to_dpidle_after_wfi();
	dpidle_flag = 0;
	spm_lock_release();
}

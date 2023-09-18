// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
#include <arch_helpers.h>
#include <debug.h>
#include <delay_timer.h>
#include <mmio.h>
#include <mt_spm.h>
#include <mt_spm_internal.h>
#include <mt_spm_reg.h>
#include <mt_spm_vcorefs.h>
#include <plat_pm.h>
#include <platform.h>
#include <platform_def.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "voltage_def.h"
#include <uart.h>
#include <console.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <mtk_plat_common.h>
#include "assert.h"

#define DVFSRC_LEVEL         (DVFSRC_BASE + 0xDC)
#define DVFSRC_SEC_SW_REQ    (DVFSRC_BASE + 0x304)
#define SPM_DVFS_TIMEOUT       1000     /* 1ms */

static struct pwr_ctrl vcorefs_ctrl = {
	.wake_src		= R12_PCM_TIMER,
	.conn_mask_b = 0x1,
	.conn_apsrc_sel = 0,
	.conn_srcclkena_sel_mask = 0x1,
	/* default VCORE DVFS is disabled */
	/*.pcm_flags = SPM_FLAG_DIS_VCORE_DVS,// | SPM_FLAG_DIS_VCORE_DFS,*/
};

struct spm_lp_scen __spm_vcorefs = {
	.pwrctrl	= &vcorefs_ctrl,
};

#if 0
static struct reg_config spm_vcorefs_init_configs[][32] = {
	{
/* MISC */
		{SPM_DVS_LEVEL,			(0x1 << 2)},
		{SPM_DFS_LEVEL,			(0x1 << 2)},
		{SPM_SW_RSV_9,			(0x1 << 31) | (0x1 << 14)},
		{DVFSRC_EVENT_MASK_CON,	0x1FFFF},
		{DVFSRC_EVENT_SEL,		0x0},
/* Level */
		{SPM_SW_RSV_12,			0x00000001},/*DVS_LEVEL0*/
		{SPM_SW_RSV_13,			0x00000006},/*DVS_LEVEL1*/
		{SPM_SW_RSV_14,			0xFFFFFFF8},/*DVS_LEVEL2*/
		{SPM_SW_RSV_15,			0x0000000C},/*DFS_LEVEL1*/
		{SPM_SW_RSV_16,			0xFFFFFFF0},/*DFS_LEVEL2*/
		{SPM_SW_RSV_17,			0x00020004},/*REST LEVEL*/
		{-1, 0},
	},
		/* NULL */
	{
		{ -1, 0 },
	},
}
#endif

static inline int set_dvfs_cmd_value(uint32_t tmp, uint32_t slave_id, uint32_t addr, uint32_t i2c_value)
{
	tmp = (tmp & ~I2C_SLAVE_ID) | (slave_id << I2C_SLAVE_ID_SHIFT);
	tmp = (tmp & ~I2C_REG_ADDR) | (addr << I2C_ADDR_SHIFT);
	tmp = (tmp & ~I2C_REG_VALUE)  | (i2c_value << I2C_VALUE_SHIFT);
	return tmp;
}

static void spm_vcorefs_i2c_setting(uint64_t power_flags)
{
	uint32_t tmp = 0;
	uint32_t baseaddr = I2C0_BASE_ADDR;
	uint32_t cgshift = 24;

#if 0
	/*I2C MUX CG*/
	if ((mmio_read_32(I2C_CLK_STA) & (0x1 << 15)) >> 15) {
		mmio_write_32(I2C_CLK_CLR, 0x1 << 15);
		INFO("spm_vcorefs_i2c_setting I2C CG: ....\n");
	}
#endif
	switch (power_flags) {
	case VCORE_I2C0_VSRAM_I2C0:
		baseaddr = I2C0_BASE_ADDR;
		cgshift = I2C0_CG_SHIFT;
		INFO("spm_vcorefs_i2c_setting: i2c0 setting done\n");
		break;
	case VCORE_I2C1_VSRAM_I2C1:
		baseaddr = I2C1_BASE_ADDR;
		cgshift = I2C1_CG_SHIFT;
		INFO("spm_vcorefs_i2c_setting: i2c1 setting done\n");
		break;
	case VCORE_I2C2_VSRAM_I2C2:
		baseaddr = I2C2_BASE_ADDR;
		cgshift = I2C2_CG_SHIFT;
		INFO("spm_vcorefs_i2c_setting: i2c2 setting done\n");
		break;
	default:
		baseaddr = I2C0_BASE_ADDR;
		cgshift = I2C0_CG_SHIFT;
		INFO("spm_vcorefs_i2c_setting: unsupport power solution !!!!\n");
		break;
	}
	/*I2CX CG*/
	if ((mmio_read_32(I2C_CG_STA) & (0x1 << cgshift)) >> cgshift) {
		mmio_write_32(I2C_CG_CLR, 0x1 << cgshift);
		INFO("spm_vcorefs_i2c_setting: need enable I2CX CG ...\n");
	}
	/*I2CX device reg*/
	mmio_write_32(baseaddr + 0x50, 0x1);		/*softreset*/
	mmio_write_32(baseaddr + 0x40, 0x3);		/*set io open drain*/
	mmio_write_32(baseaddr + 0x54, 0x0);		/*close dcm*/
	mmio_write_32(baseaddr + 0x20, 0x12a);		/*speed*/

	tmp = mmio_read_32(SPM_DVFS_CMD1);
	mmio_write_32(SPM_DVFS_CMD1, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x46)); /*set 0p65*/
	tmp = mmio_read_32(SPM_DVFS_CMD2);
	mmio_write_32(SPM_DVFS_CMD2, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x50)); /*set 0p7*/
	tmp = mmio_read_32(SPM_DVFS_CMD3);
	mmio_write_32(SPM_DVFS_CMD3, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x64)); /*set 0p8*/
	/*vcore_sram */
	tmp = mmio_read_32(SPM_DVFS_CMD4);
	mmio_write_32(SPM_DVFS_CMD4,
		set_dvfs_cmd_value(tmp, VCORE_SRAM_SLAVE_ID, VCORE_SRAM_ADDR, VCORE_SRAM_VOL));	/*set 0p8v*/
	tmp = mmio_read_32(SPM_DVFS_CMD5);
	mmio_write_32(SPM_DVFS_CMD5,
		set_dvfs_cmd_value(tmp, VCORE_SRAM_SLAVE_ID, VCORE_SRAM_ADDR, 0x78));	/*set 0p9*/

	mmio_write_32(SPM_SW_RSV_18, baseaddr);/*i2cx base addr*/

}

static void spm_vcorefs_pwm_setting(void)
{
#if 0
	mmio_write_32(PWM_CLK_CLR, (PWM_CLK_MASK << PWM_CLK_SHIFT));
	mmio_write_32(PWM_CLK_SET, (0x0 << PWM_CLK_SHIFT));
	mmio_write_32(PWM_CLK_CLR, (PWM_PDN_MASK << PWM_PDN_SHIFT));/*PWM MUX CG*/

	mmio_write_32(0x10001084, 0x200000);/*infra pwm CG*/
#endif
	mmio_write_32(0x100010A8, 0x1);/*pwm5 clk*/

	mmio_write_32(SPM_DVFS_CMD1, 0x18); /*0.65*/
	mmio_write_32(SPM_DVFS_CMD2, 0x10); /*0.7*/
	mmio_write_32(SPM_DVFS_CMD3, 0x0); /*0.8*/
}


void spm_request_dvfs_opp(uint64_t id, uint64_t reg)
{
	int i = 0;

	INFO("spm_request_dvfs_opp: begin\n");

	switch (id) {
	case 0: /* ZQTX */
		INFO("spm_request_dvfs_opp: 0\n");
		mmio_write_32(DVFSRC_SEC_SW_REQ, reg);
		if (reg != 0x0) {
			while ((mmio_read_32(DVFSRC_LEVEL) >> 16) < 0x4) {
				if (i >= SPM_DVFS_TIMEOUT)
					break;
				udelay(1);
				i++;
			}
			return;
		}
		break;
	default:
		break;
	}
	INFO("spm_request_dvfs_opp: finish\n");
}

static void spm_vcorefs_pre_process(struct pwr_ctrl *pwrctrl, uint64_t power_flags)
{
	switch (power_flags) {
	case VCORE_I2C0_VSRAM_I2C0:
	case VCORE_I2C1_VSRAM_I2C1:
	case VCORE_I2C2_VSRAM_I2C2:
		spm_vcorefs_i2c_setting(power_flags);
		break;
	case VCORE_PWM_VSARM_GPIO:
		spm_vcorefs_pwm_setting();
		pwrctrl->pcm_flags2 |= SPM_FLAG2_VCORE_GPIO_DVS | SPM_FLAG2_VCORE_PWM_DVS;
		INFO("spm_vcorefs_pre_process: pwm setting done\n");
		break;
	case VCORE_GPIO:
		pwrctrl->pcm_flags2 |= SPM_FLAG2_VCORE_GPIO_DVFS | SPM_FLAG2_DIS_VCORE_SRAM_CTRL;
		INFO("spm_vcorefs_pre_process: gpio setting done\n");
		break;
	default:
		spm_vcorefs_pwm_setting();
		pwrctrl->pcm_flags2 |= SPM_FLAG2_VCORE_GPIO_DVS | SPM_FLAG2_VCORE_PWM_DVS;
		INFO("spm_vcorefs_pre_process: unsupport power solution!!!!\n");
		break;
	}
}

static void spm_go_to_vcorefs(uint64_t power_flags)
{
	struct pwr_ctrl *pwrctrl;

	pwrctrl = __spm_vcorefs.pwrctrl;
	spm_vcorefs_pre_process(pwrctrl, power_flags);
	__spm_set_power_control(pwrctrl);
	__spm_set_wakeup_event(pwrctrl);
	__spm_set_pcm_flags(pwrctrl);
	assert(spm_read(SPM_SW_RSV_6) == PCM_DVFS_INI_CMD);
	/*if (spm_read(SPM_SW_RSV_6) != PCM_DVFS_INI_CMD) {
		INFO("spm_go_to_vcorefs: check cmd again: ....\n");
		__spm_cmd_check(PCM_DVFS_INI_CMD);
	}*/
	INFO("spm_go_to_vcorefs: ....\n");
}
static void spm_dvfsfw_init(uint64_t boot_up_opp, uint64_t spm_idx)
{
	INFO("spm_dvfsfw_init: __spmfw_idx = 0x%x\n", __spmfw_idx);
	switch (spm_idx) {
	case SPMFW_LP4_3200:
	case SPMFW_LP3_1866:
		INFO("SPMFW_LP3_1866 or SPMFW_LP4_3200: spm_idx = 0x%llx\n", spm_idx);
		/* default voltage level is max */
		mmio_write_32(SPM_DVS_LEVEL, (0x1 << 2));
		/* default frequence level is max */
		mmio_write_32(SPM_DFS_LEVEL, (0x1 << 2));
		mmio_write_32(SPM_SW_RSV_9, (0x1 << 31) | (0x1 << 14));
		mmio_write_32(SPM_SW_RSV_12, 0x00000001);/*DVS_LEVEL0*/
		mmio_write_32(SPM_SW_RSV_13, 0x00000006);/*DVS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_14, 0xFFFFFFF8);/*DVS_LEVEL2*/
		mmio_write_32(SPM_SW_RSV_15, 0x0000000C);/*DFS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_16, 0xFFFFFFF0);/*DFS_LEVEL2*/
		mmio_write_32(SPM_SW_RSV_17, 0x00020004);/*REST LEVEL*/
		break;
	case SPMFW_LP4X_2400:
		INFO("SPMFW_LP4X_2400: spm_idx = 0x%llx\n", spm_idx);
		/* default voltage level is max */
		mmio_write_32(SPM_DVS_LEVEL, (0x1 << 3));
		/* default frequence level is max */
		mmio_write_32(SPM_DFS_LEVEL, (0x1 << 2));
		mmio_write_32(SPM_SW_RSV_9, (0x1 << 31) | (0x1 << 14));
		mmio_write_32(SPM_SW_RSV_12, 0x00000011);/*DVS_LEVEL0*/
		mmio_write_32(SPM_SW_RSV_13, 0x00000022);/*DVS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_14, 0x00000044);/*DVS_LEVEL2*/
		mmio_write_32(SPM_SW_RSV_11, 0xFFFFFF88);/*DVS_LEVEL3*/
		mmio_write_32(SPM_SW_RSV_15, 0x000000F0);/*DFS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_16, 0xFFFFFF00);/*DFS_LEVEL2*/
		/*mmio_write_32(SPM_SW_RSV_17, 0x00020004);*//*REST LEVEL*/
		break;
	case SPMFW_LP4X_3200:
		INFO("SPMFW_LP4X_3200: spm_idx = 0x%llx\n", spm_idx);
		/* default voltage level is max */
		mmio_write_32(SPM_DVS_LEVEL, (0x1 << 3));
		/* default frequence level is max */
		mmio_write_32(SPM_DFS_LEVEL, (0x1 << 2));
		mmio_write_32(SPM_SW_RSV_9, (0x1 << 31) | (0x1 << 14));
		mmio_write_32(SPM_SW_RSV_12, 0x00000001);/*DVS_LEVEL0*/
		mmio_write_32(SPM_SW_RSV_13, 0x0000000A);/*DVS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_14, 0x00000014);/*DVS_LEVEL2*/
		mmio_write_32(SPM_SW_RSV_11, 0xFFFFFFE0);/*DVS_LEVEL3*/
		mmio_write_32(SPM_SW_RSV_15, 0x00000038);/*DFS_LEVEL1*/
		mmio_write_32(SPM_SW_RSV_16, 0xFFFFFFC0);/*DFS_LEVEL2*/
		mmio_write_32(SPM_SW_RSV_17, 0x00010002);/*REST LEVEL*/
		break;
	default:
		INFO("Un-support DRAM type\n");
		break;
	}
	mmio_write_32(DVFSRC_EVENT_MASK_CON, 0x1FFFF);
	mmio_write_32(DVFSRC_EVENT_SEL, 0x0);
}

static void spm_vcorefs_freq_hopping(uint64_t gps_on)
{
	int i = 0;

	if (gps_on) {
		mmio_write_32(SW2SPM_MAILBOX_0, (mmio_read_32(SW2SPM_MAILBOX_0) & ~0x3) | 0x3);
		mmio_write_32(SPM_CPU_WAKEUP_EVENT, 1);
		while ((mmio_read_32(SPM2SW_MAILBOX_0) & 0x1) != 0x1) {
			if (i >= SPM_DVFS_TIMEOUT)
				break;
			udelay(1);
			i++;
		}
		mmio_write_32(SW2SPM_MAILBOX_0, (mmio_read_32(SW2SPM_MAILBOX_0) & ~0x1));
		mmio_write_32(SPM_CPU_WAKEUP_EVENT, 0);
		mmio_write_32(SPM2SW_MAILBOX_0, (mmio_read_32(SPM2SW_MAILBOX_0) & ~0x1));
	} else {
		mmio_write_32(SW2SPM_MAILBOX_0, (mmio_read_32(SW2SPM_MAILBOX_0) & ~0x3) | 0x1);
		__spm_send_cpu_wakeup_event();
		while ((mmio_read_32(SPM2SW_MAILBOX_0) & 0x1) != 0x1) {
			if (i >= SPM_DVFS_TIMEOUT)
				break;
			udelay(1);
			i++;
		}
		mmio_write_32(SW2SPM_MAILBOX_0, (mmio_read_32(SW2SPM_MAILBOX_0) & ~0x1));
		mmio_write_32(SPM_CPU_WAKEUP_EVENT, 0);
		mmio_write_32(SPM2SW_MAILBOX_0, (mmio_read_32(SPM2SW_MAILBOX_0) & ~0x1));
	}
}

void spm_vcorefs_args(uint64_t x1, uint64_t x2, uint64_t x3)
{
	uint64_t cmd = x1;

	switch (cmd) {
	case VCOREFS_SMC_CMD_0:
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		spm_dvfsfw_init(x2, x3);
		console_uninit();
		break;
	case VCOREFS_SMC_CMD_1:
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		spm_go_to_vcorefs(x2);
		console_uninit();
		break;
	case VCOREFS_SMC_CMD_2:
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		spm_request_dvfs_opp(x2, x3);
		console_uninit();
		break;
	case VCOREFS_SMC_CMD_4:
		console_init(gteearg.atf_log_port, UART_CLOCK, UART_BAUDRATE);
		spm_vcorefs_freq_hopping(x2);
		console_uninit();
		break;
	default:
		break;
	}
}


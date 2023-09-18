/*
 * Copyright (c) 2015,  ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms,  with or without
 * modification,  are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,  this
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
 * AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  WHETHER IN
 * CONTRACT,  STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <console.h>
#include <debug.h>
#include <delay_timer.h>
#include <mcucfg.h>
#include <mmio.h>
#include <mtspmc.h>
#include <mtspmc_private.h>
#include <mt_spm_reg.h>
#include <plat_private.h>
#include <platform.h>
#include <platform_def.h>
#include <psci.h>

unsigned int cpu_bitmask;

#if CONFIG_SPMC_SPARK == 1
static void spark_set_ldo(int cluster, int cpu)
{
	uintptr_t reg;
	unsigned int sparkvretcntrl = 0x3f;	/* 34=.5 3f=.6 */

	assert(cpu >= 0 && cpu < 4);

	VERBOSE("%s: sparkvretcntrl=0x%x\n", __func__, sparkvretcntrl);
	reg = per_cluster(cluster, MCUCFG_SPARK2LDO);

	/*
	 * each core in LITTLE cluster can control its
	 * spark voltage
	 */
	mmio_clrsetbits_32(reg, 0x3f << (cpu << 3),
			   sparkvretcntrl << (cpu << 3));
}

static void spark_set_retention(int tick)
{
	uint64_t cpuectlr;

	cpuectlr = read_cpuectlr();
	cpuectlr &= ~0x7ULL;
	cpuectlr |= tick & 0x7;
	write_cpuectlr(cpuectlr);
}

void spark_enable(int cluster, int cpu)
{
	uintptr_t reg;

	/* only L cluster (cluster0) in MT6765 has SPARK */
	if (cluster)
		return;

	spark_set_ldo(cluster, cpu);
	spark_set_retention(1);

	reg = per_cpu(cluster, cpu, MCUCFG_SPARK);
	mmio_setbits_32(reg, SW_SPARK_EN);

	VERBOSE("%s: 0x%x: %x\n", __func__, reg, mmio_read_32(reg));
}

void spark_disable(int cluster, int cpu)
{
	uintptr_t reg;

	/* only L cluster (cluster0) in MT6765 has SPARK */
	if (cluster)
		return;

	spark_set_retention(0);

	reg = per_cpu(cluster, cpu, MCUCFG_SPARK);
	mmio_clrbits_32(reg, SW_SPARK_EN);

	VERBOSE("%s: 0x%x: %x\n", __func__, reg, mmio_read_32(reg));
}
#else /* CONFIG_SPMC_SPARK == 1 */
void spark_enable(int cluster, int cpu)
{
}

void spark_disable(int cluster, int cpu)
{
}
#endif /* CONFIG_SPMC_SPARK == 0 */

void set_cpu_retention_control(int retention_value)
{
	uint64_t cpuectlr;

	cpuectlr = read_cpuectlr();
	cpuectlr = ((cpuectlr >> 3) << 3);
	cpuectlr |= retention_value;
	write_cpuectlr(cpuectlr);
}

void mcucfg_set_bootaddr(int cluster, int cpu, uintptr_t bootaddr)
{
	mmio_write_32(per_cpu(cluster, cpu, MCUCFG_BOOTADDR), bootaddr);
}

uintptr_t mcucfg_get_bootaddr(int cluster, int cpu)
{
	return mmio_read_32(per_cpu(cluster, cpu, MCUCFG_BOOTADDR));
}

void mcucfg_init_archstate(int cluster, int cpu, int arm64)
{
	uintptr_t reg;

	reg = per_cluster(cluster, MCUCFG_INITARCH);

	if (arm64)
		mmio_setbits_32(reg, 1 << (12 + cpu));
	else
		mmio_clrbits_32(reg, 1 << (12 + cpu));
}

int spmc_init(void)
{
	int err = 0;

	/* TINFO="enable SPM register control" */
	mmio_write_32(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0) |
		AUDAFE_MTCMOS_CG_EN | AUDSRC_MTCMOS_CG_EN);
	/*
	 *  PRINTF_SPMC("POWERON_CONFIG_EN_0x%x=0x%x\n",
	 *	POWERON_CONFIG_EN, mmio_read_32(POWERON_CONFIG_EN));
	 */
#if CONFIG_SPMC_MODE != 0
	/* de-assert Bypass SPMC  0: SPMC mode	1: Legacy mode */
	mmio_write_32(BYPASS_SPMC, 0x0);
	/*
	 * PRINTF_SPMC("BYPASS_SPMC_0x%x=0x%x\n", BYPASS_SPMC,
	 *  mmio_read_32(BYPASS_SPMC));
	 */
	/* udelay(200); */
	PRINTF_SPMC("[%s]change to SPMC mode !!!\n", __func__);
#endif
/* MP0 SPMC power Ctrl signals */
	mmio_write_32(MP0_CPU0_PWR_CON,
		mmio_read_32(MP0_CPU0_PWR_CON) & ~PWR_ON_2ND);
	mmio_write_32(MP0_CPU0_PWR_CON,
		mmio_read_32(MP0_CPU0_PWR_CON) | PWR_RST_B);
	mmio_write_32(MP0_CPU1_PWR_CON,
		mmio_read_32(MP0_CPU1_PWR_CON) & ~PWR_ON_2ND);
	mmio_write_32(MP0_CPU1_PWR_CON,
		mmio_read_32(MP0_CPU1_PWR_CON) | PWR_RST_B);

	mmio_write_32(MP0_CPUTOP_PWR_CON,
		mmio_read_32(MP0_CPUTOP_PWR_CON) & ~PWR_ON_2ND);
	mmio_write_32(MP0_CPUTOP_PWR_CON,
		mmio_read_32(MP0_CPUTOP_PWR_CON) | PWR_RST_B);
	mmio_write_32(MP0_CPUTOP_PWR_CON,
		mmio_read_32(MP0_CPUTOP_PWR_CON) & ~PWR_CLK_DIS);
/* MP1 SPMC power Ctrl signals */

/* setup_sw_cluster_default_state(0x10,STA_POWER_ON); */
/* setup_sw_core_default_state(0x0,STA_POWER_ON); */

	cpu_bitmask = 1;

	return err;
}

int spmc_cputop_mpx_onoff(int cputop_mpx, int state, int mode)
{
	int err = 0;
	unsigned int mpx_mask = 0;
	unsigned int MPx_CPUTOP_SPMC = 0, MPx_CPUTOP_PWR_CON = 0;
	unsigned int MPx_CPUTOP_PWR_STA_MASK = 0, MPx_CA7_MISC_CONFIG = 0;
#if SPMC_DVT
	unsigned int MPx_SNOOP_CTRL = 0, MPx_AXI_CONFIG = 0;
#endif
	/* TINFO="enable SPM register control" */
	mmio_write_32(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0) |
		AUDAFE_MTCMOS_CG_EN | AUDSRC_MTCMOS_CG_EN);

	/*
	 * PRINTF_SPMC(">>>>>>>> %s cputop_mpx=%d state=%d mode=%d\n",
	 * __func__, cputop_mpx, state, mode);
	 */

	if (cputop_mpx == CPUTOP_MP0) {
		mpx_mask = (1 << IDX_PROTECT_ICC0_CACTIVE) |
		    (1 << IDX_PROTECT_ICD0_CACTIVE) |
		    (1 << IDX_PROTECT_MP0_CACTIVE) |
		    (1 << IDX_PROTECT_L2C0_CACTIVE);
		MPx_CPUTOP_SPMC = MP0_CPUTOP_SPMC_CTL;
		MPx_CPUTOP_PWR_CON = MP0_CPUTOP_PWR_CON;
		MPx_CA7_MISC_CONFIG = MP0_CA7_MISC_CONFIG;
#if SPMC_DVT
		MPx_AXI_CONFIG = MP0_AXI_CONFIG;
		MPx_SNOOP_CTRL = MP0_SNOOP_CTRL;
#endif
	}

	/*
	 * PRINTF_SPMC("MPx_CPUTOP_PWR_STA_MASK=%x\n",
	 * MPx_CPUTOP_PWR_STA_MASK);
	 */
	/* PRINTF_SPMC("MPx_AXI_CONFIG=%x\n", MPx_AXI_CONFIG); */
	/* PRINTF_SPMC("mpx_mask=%x\n", mpx_mask); */
	/* PRINTF_SPMC("MPx_SNOOP_CTRL=%x\n", MPx_SNOOP_CTRL); */
	MPx_CPUTOP_PWR_STA_MASK = PWR_ACK;
	if (state == STA_POWER_DOWN) {
#if SPMC_DVT			/* by callee in plat_affinst_off() */
/* ############### BUS PROTECT ENABLE and SNOOP/DVM DISABLE ############### */
		mmio_write_32(MPx_SNOOP_CTRL, mmio_read_32(MPx_SNOOP_CTRL)
			& ~MPx_SNOOP_ENABLE);
		while (mmio_read_32(MPx_SNOOP_STATUS) & (1 << 0))
			;
		mmio_write_32(MPx_AXI_CONFIG, mmio_read_32(MPx_AXI_CONFIG)
			| MPx_AXI_CONFIG_acinactm);
#endif
/* ############### BUS PROTECT ENABLE and SNOOP/DVM DISABLE ############### */

		if (mode == MODE_AUTO_SHUT_OFF) {
			/*
			 * Cluster<n>Core0 set cluster<n>.SPMC.MP<n>.
			 * sw_no_wait_for_q_channel (mcucfg_reg) 0
			 */
			mmio_write_32(MPx_CPUTOP_SPMC,
				mmio_read_32(MPx_CPUTOP_SPMC)
					& ~sw_no_wait_for_q_channel);
			/*
			 * Cluster<n>Core0 set cluster<n>.SPMC.MP<n>.
			 * sw_coq_dis (mcucfg_reg) 0
			 */
			mmio_write_32(MPx_CPUTOP_SPMC,
				mmio_read_32(MPx_CPUTOP_SPMC) & ~sw_coq_dis);
		} else {	/* MODE_SPMC_HW or MODE_DORMANT */
			/*
			 * Set cluster<n>.SPMC.MP<n>.
			 * sw_no_wait_for_q_channel (mcucfg_reg) 1
			 */
			mmio_write_32(MPx_CPUTOP_SPMC,
				mmio_read_32(MPx_CPUTOP_SPMC)
					| sw_no_wait_for_q_channel);
			mmio_write_32(MPx_CPUTOP_SPMC,
				mmio_read_32(MPx_CPUTOP_SPMC) | sw_coq_dis);
			/* TINFO="Wait STANDBYWFIL2 for Cluster 0" */
			while ((mmio_read_32(MPx_CA7_MISC_CONFIG)
				& MPx_CA7_MISC_CONFIG_standbywfil2)
					!= MPx_CA7_MISC_CONFIG_standbywfil2)
				;
			/* TINFO="Set ADB pwrdnreqn for Cluster 0" */
			mmio_write_32(MCUSYS_PROTECTEN_SET, mpx_mask);
			/* TINFO="Wait ADB pwrdnreqn for Cluster 0" */
			while ((mmio_read_32(MCUSYS_PROTECTEN_STA1) & mpx_mask)
				!= mpx_mask)
				;
		}

		if (mode == MODE_DORMANT) {
			/* Set mp<n>_spmc_sram_dormant_en 0 */
			mmio_write_32(SPMC_DORMANT_ENABLE,
					mmio_read_32(SPMC_DORMANT_ENABLE)
						| (MP0_SPMC_SRAM_DORMANT_EN
							<< cputop_mpx));
		} else { /* MODE_SPMC_HW or MODE_AUTO_SHUT_OFF */
			/* Set mp<n>_spmc_sram_dormant_en 0 */
			mmio_write_32(SPMC_DORMANT_ENABLE,
					mmio_read_32(SPMC_DORMANT_ENABLE)
						& ~(MP0_SPMC_SRAM_DORMANT_EN
							<< cputop_mpx));
		}
		/* Set mp<n>_spmc_pwr_on_cputop 0 */
		mmio_write_32(MPx_CPUTOP_PWR_CON,
			mmio_read_32(MPx_CPUTOP_PWR_CON) & ~PWR_ON);
		if (mode == MODE_SPMC_HW) {
			/* TINFO="Wait until PWR_STATUS = 0" */
			while (mmio_read_32(MPx_CPUTOP_PWR_CON)
				& MPx_CPUTOP_PWR_STA_MASK)
				;
			mmio_write_32(CPU_EXT_BUCK_ISO,
				mmio_read_32(CPU_EXT_BUCK_ISO)
					| (0x1 << cputop_mpx));
		}
		/* TINFO="Finish to turn off MP0_CPUTOP" */
	} else {
		/* STA_POWER_ON */
		/* TINFO="Start to turn on MP0_CPUTOP" */
		/* mp2_vproc_ext_off=0(DUAL_VCORE_VCA15MPWR_ISO_AON=0)*/
		mmio_write_32(CPU_EXT_BUCK_ISO,
			mmio_read_32(CPU_EXT_BUCK_ISO) & ~(0x1 << cputop_mpx));
		/* TINFO="Set PWR_ON = 1" */
		mmio_write_32(MPx_CPUTOP_PWR_CON,
			mmio_read_32(MPx_CPUTOP_PWR_CON) | PWR_ON);
		/* TINFO="Wait until PWR_STATUS = 1" */
		while (!(mmio_read_32(MPx_CPUTOP_PWR_CON)
			& MPx_CPUTOP_PWR_STA_MASK))
			;
		PRINTF_SPMC("MPx_CPUTOP_PWR_CON_0x%x=0x%x\n",
			MPx_CPUTOP_PWR_CON, mmio_read_32(MPx_CPUTOP_PWR_CON));

/* ############## BUS PROTECT DISABLE and SNOOP/DVM ENABLE ############## */
		/* TINFO="Release bus protect" */
		/* TINFO="Release ADB pwrdnreqn for Cluster 0" */
		/*
		 * mmio_write_32(INFRA_TOPAXI_PROTECTEN_1,
		 * mmio_read_32(INFRA_TOPAXI_PROTECTEN_1) & ~mpx_mask);
		 */
		mmio_write_32(MCUSYS_PROTECTEN_CLR, mpx_mask);
		/*
		 * PRINTF_SPMC("MCUSYS_PROTECTEN_CLR_0x%x=0x%x\n",
		 * MCUSYS_PROTECTEN_CLR,
		 * mmio_read_32(MCUSYS_PROTECTEN_CLR));
		 */
		/* TINFO="Wait ADB ~pwrdnreqn for Cluster 0" */
		while ((mmio_read_32(MCUSYS_PROTECTEN_STA1) & mpx_mask) != 0)
			;
		/*
		 * PRINTF_SPMC("MCUSYS_PROTECTEN_STA1_0x%x=0x%x\n",
		 * MCUSYS_PROTECTEN_STA1,
		 * mmio_read_32(MCUSYS_PROTECTEN_STA1));
		 */

#if SPMC_DVT /* by callee in plat_affinst_off() */
		/* Program MP<n>_AXI_CONFIG acinactm to 0 */
		mmio_write_32(MPx_AXI_CONFIG,
			mmio_read_32(MPx_AXI_CONFIG)
				& ~MPx_AXI_CONFIG_acinactm);
		/*
		 * PRINTF_SPMC("MPx_AXI_CONFIG_0x%x=0x%x\n",
		 * MPx_AXI_CONFIG,mmio_read_32(MPx_AXI_CONFIG));
		 */
		mmio_write_32(MPx_SNOOP_CTRL,
			mmio_read_32(MPx_SNOOP_CTRL) | MPx_SNOOP_ENABLE);
		while (mmio_read_32(MPx_SNOOP_STATUS) & (1 << 0))
			;
#endif
/* ############## BUS PROTECT DISABLE and SNOOP/DVM ENABLE ############## */
	}
	return err;
}

int spmc_cpu_corex_onoff(int linear_id, int state, int mode)
{
	int err = 0;
	unsigned int CPUSYSx_CPUx_SPMC_CTL = 0, MPx_CPUx_PWR_CON = 0,
		MPx_CPUx_STANDBYWFI = 0, MPx_CPUx_PWR_STA_MASK = 0;

	/* TINFO="enable SPM register control" */
	mmio_write_32(POWERON_CONFIG_EN, (SPM_PROJECT_CODE << 16) | (0x1 << 0) |
		AUDAFE_MTCMOS_CG_EN | AUDSRC_MTCMOS_CG_EN);

	/*
	 * PRINTF_SPMC(">>>>>>>> %s >>>>>>>>linear_id=%d state=%d mode=%d\n",
	 * __func__, linear_id,state, mode);
	 */
	switch (linear_id) {
	case 0:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU0_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU0_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU0_PWR_CON;
		break;
	case 1:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU1_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU1_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU1_PWR_CON;
		break;
	default:
		PRINTF_SPMC("%s: CPU%d not exists\n", __func__, (int)linear_id);
	/* ERROR("[ATF]: %s() CPU%d not exists\n", __func__, (int)linear_id); */
	/* assert(0); */
	}

	MPx_CPUx_PWR_STA_MASK = PWR_ACK;

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off MP0_CPU0" */
		if (!(cpu_bitmask & (1 << linear_id))) {
			PRINTF_SPMC("core%d already turn off!",
				linear_id);
			PRINTF_SPMC("cpu_bitmask=0x%x\n",
				cpu_bitmask);
			return 0;
		}
		if (mode == MODE_AUTO_SHUT_OFF) {
			mmio_write_32(CPUSYSx_CPUx_SPMC_CTL,
					mmio_read_32(CPUSYSx_CPUx_SPMC_CTL)
					& ~cpu_sw_no_wait_for_q_channel);
			set_cpu_retention_control(1);
		} else {
			mmio_write_32(CPUSYSx_CPUx_SPMC_CTL,
				mmio_read_32(CPUSYSx_CPUx_SPMC_CTL)
					| cpu_sw_no_wait_for_q_channel);
			/*
			 * PRINTF_SPMC("CPU_IDLE_STA_0x%x=0x%x\n",
			 * CPU_IDLE_STA,mmio_read_32(CPU_IDLE_STA));
			 */
			while (!(mmio_read_32(CPU_IDLE_STA)
				&MPx_CPUx_STANDBYWFI))
				;
		}

		/* TINFO="Set PWR_ON = 0" */
		mmio_write_32(MPx_CPUx_PWR_CON,
			mmio_read_32(MPx_CPUx_PWR_CON) & ~PWR_ON);
		/*
		 * PRINTF_SPMC("MPx_CPUx_PWR_CON_0x%x=0x%x\n",
		 * MPx_CPUx_PWR_CON, mmio_read_32(MPx_CPUx_PWR_CON));
		 */
		/* TINFO="Wait until CPU_PWR_STATUS = 0 */
		if (mode == MODE_SPMC_HW) {
			while (mmio_read_32(MPx_CPUx_PWR_CON)
				& MPx_CPUx_PWR_STA_MASK)
				;
			PRINTF_SPMC("MPx_CPUx_PWR_CON_0x%x=0x%x\n",
				MPx_CPUx_PWR_CON,
					mmio_read_32(MPx_CPUx_PWR_CON));
		}
		cpu_bitmask &= ~(1 << linear_id);
		/* PRINTF_SPMC("cpu_bitmask=0x%x\n", cpu_bitmask); */
		/* TINFO="Finish to turn off MP0_CPU0" */
	} else {
		/* TINFO="Start to turn on MP0_CPU0" */
		/* TINFO="Set PWR_ON = 1" */
		mmio_write_32(MPx_CPUx_PWR_CON,
			mmio_read_32(MPx_CPUx_PWR_CON) | PWR_ON);
		PRINTF_SPMC("MPx_CPUx_PWR_CON_0x%x=0x%x\n",
			MPx_CPUx_PWR_CON, mmio_read_32(MPx_CPUx_PWR_CON));
		/*
		 * TINFO="Wait until CPU_PWR_STATUS = 1
		 * and CPU_PWR_STATUS_2ND = 1"
		 */
		while ((mmio_read_32(MPx_CPUx_PWR_CON) & MPx_CPUx_PWR_STA_MASK)
			!= MPx_CPUx_PWR_STA_MASK)
			;
		/*
		 * PRINTF_SPMC("MPx_CPUx_PWR_CON_0x%x=0x%x\n",
		 * MPx_CPUx_PWR_CON, mmio_read_32(MPx_CPUx_PWR_CON));
		 */
		cpu_bitmask |= (1 << linear_id);
		PRINTF_SPMC("cpu_bitmask=0x%x\n", cpu_bitmask);
		/* TINFO="Finish to turn on MP0_CPU0" */
		/*
		 * PRINTF_SPMC("[AT] PowerOn CPU %d successfully\n",
		 *  linear_id);
		 */
	}
	return err;
}

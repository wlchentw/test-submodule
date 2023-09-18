/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
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

#include <debug.h>
#include <err.h>
#include <reg.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <lk/init.h>
#include <platform/interrupts.h>
#include <platform/mt_irq.h>
#include <platform/mt_gic_v3.h>
#include <platform/mt_reg_base.h>
#include <arch/ops.h>
#include <platform/gic.h>
#include <trace.h>

#if WITH_LIB_SM
#include <lib/sm.h>
#include <lib/sm/sm_err.h>
#endif

#define LOCAL_TRACE 0

#if ARCH_ARM
#include <arch/arm.h>
#define iframe arm_iframe
#define IFRAME_PC(frame) ((frame)->pc)
#endif
#if ARCH_ARM64
#include <arch/arm64.h>
#define iframe arm64_iframe_short
#define IFRAME_PC(frame) ((frame)->elr)
#endif

/* helpers for later ICC encode macros
 * Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 */
#define __stringify_1(x)     #x
#define __stringify(x)       __stringify_1(x)

#define GIC_READ(a) readl(a)
#define GIC_WRITE(a, v) writel(v, a)

static status_t arm_gic_set_secure_locked(u_int irq, bool secure);
static spin_lock_t gicd_lock;

#if WITH_LIB_SM
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_IRQ_FIQ
#else
#define GICD_LOCK_FLAGS SPIN_LOCK_FLAG_INTERRUPTS
#endif

#define GIC_MAX_PER_CPU_INT 32

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define GIC_REG_COUNT(bit_per_reg) DIV_ROUND_UP(MAX_INT, (bit_per_reg))
#define DEFINE_GIC_SHADOW_REG(name, bit_per_reg, init_val, init_from) \
	uint32_t (name)[GIC_REG_COUNT(bit_per_reg)] = { \
		[(init_from / bit_per_reg) ... \
		 (GIC_REG_COUNT(bit_per_reg) - 1)] = (init_val) \
	}

__asm__ (
"       .irp    num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30\n"
"       .equ    __reg_num_x\\num, \\num\n"
"       .endr\n"
"       .equ    __reg_num_xzr, 31\n"
"\n"
"       .macro  mrs_s, rt, sreg\n"
"       .inst   0xd5300000|(\\sreg)|(__reg_num_\\rt)\n"
"       .endm\n"
"\n"
"       .macro  msr_s, sreg, rt\n"
"       .inst   0xd5100000|(\\sreg)|(__reg_num_\\rt)\n"
"       .endm\n"
);

/* since gcc not support most ARMv8 ICC sysreg in asm,
 * we learn Linux's way to encode them */
#define sys_reg(op0, op1, crn, crm, op2) \
        ((((op0)-2)<<19)|((op1)<<16)|((crn)<<12)|((crm)<<8)|((op2)<<5))

#if WITH_LIB_SM
static bool arm_gic_non_secure_interrupts_frozen;

static bool arm_gic_interrupt_change_allowed(int irq)
{
	if (!arm_gic_non_secure_interrupts_frozen)
		return true;

	TRACEF("change to interrupt %d ignored after booting ns\n", irq);
	return false;
}

static void suspend_resume_fiq(bool resume_gicc, bool resume_gicd);
#else
static bool arm_gic_interrupt_change_allowed(int irq)
{
	return true;
}

static void suspend_resume_fiq(bool resume_gicc, bool resume_gicd)
{
}
#endif

/*******************************************************************************
 * This function does some minimal GICv3 configuration. The Firmware itself does
 * not fully support GICv3 at this time and relies on GICv2 emulation as
 * provided by GICv3. This function allows software (like Linux) in later stages
 * to use full GICv3 features.
 ******************************************************************************/
static void gicv3_cpuif_setup(void)
{
	/* set all SGI/PPI as non-secure GROUP1 by default.
	   rdist_base + 64K == SGI_base */
	GIC_WRITE(GIC_REDIS_BASE+SZ_64K+GICE_V3_IGROUP0, 0xffffffff);
	GIC_WRITE(GIC_REDIS_BASE+SZ_64K+GICE_V3_IGRPMOD0, 0x0);
}


static void mt_git_dist_rwp(void)
{
	/*
	 * check GICD_CTLR.RWP for done check
	 */
	while (GIC_READ(GIC_DIST_BASE + GICD_CTLR) & GICD_CTLR_RWP) {

	}
}

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table_per_cpu[GIC_MAX_PER_CPU_INT][SMP_MAX_CPUS];
static struct int_handler_struct int_handler_table_shared[MAX_INT-GIC_MAX_PER_CPU_INT];

static struct int_handler_struct *get_int_handler(unsigned int vector, uint cpu)
{
	if (vector < GIC_MAX_PER_CPU_INT)
		return &int_handler_table_per_cpu[vector][cpu];
	else
		return &int_handler_table_shared[vector - GIC_MAX_PER_CPU_INT];
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	struct int_handler_struct *h;
	uint cpu = arch_curr_cpu_num();

	spin_lock_saved_state_t state;

	if (vector >= MAX_INT)
		panic("register_int_handler: vector out of range %d\n", vector);

	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	if (arm_gic_interrupt_change_allowed(vector)) {
		h = get_int_handler(vector, cpu);
		h->handler = handler;
		h->arg = arg;
	}

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
}

#if WITH_LIB_SM
static DEFINE_GIC_SHADOW_REG(gicd_igroupr, 32, ~0U, 0);
#endif
static DEFINE_GIC_SHADOW_REG(gicd_itargetsr, 4, 0x01010101, 32);

static void gic_set_enable(uint vector, bool enable)
{
	int reg = vector / 32;
	uint32_t mask = 1ULL << (vector % 32);

	if (enable)
		GIC_READ(GIC_DIST_BASE + GICD_ISENABLER + reg * 4) = mask;
	else
		GIC_READ(GIC_DIST_BASE + GICD_ICENABLER + reg * 4) = mask;
}

static void arm_gic_init_percpu(uint level)
{
#if WITH_LIB_SM
	GIC_READ(GIC_REDIS_BASE + GICC_CTLR) = 0xb; // enable GIC0 and select fiq mode for secure
	GIC_READ(GIC_DIST_BASE + GICD_IGROUPR) = ~0U; /* GICD_IGROUPR0 is banked */
#else
	GIC_READ(GIC_REDIS_BASE + GICC_CTLR) = 1; // enable GIC0
#endif
	GIC_READ(GIC_REDIS_BASE + GICC_PMR) = 0xFF; // unmask interrupts at all priority levels
}

LK_INIT_HOOK_FLAGS(arm_gic_init_percpu,
       arm_gic_init_percpu,
       LK_INIT_LEVEL_PLATFORM_EARLY, LK_INIT_FLAG_SECONDARY_CPUS);

static void arm_gic_suspend_cpu(uint level)
{
	suspend_resume_fiq(false, false);
}

LK_INIT_HOOK_FLAGS(arm_gic_suspend_cpu, arm_gic_suspend_cpu,
		LK_INIT_LEVEL_PLATFORM, LK_INIT_FLAG_CPU_SUSPEND);

static void arm_gic_resume_cpu(uint level)
{
	spin_lock_saved_state_t state;
	bool resume_gicd = false;

	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);
	if (!(GIC_READ(GIC_DIST_BASE + GICD_CTLR) & 1)) {
		dprintf(SPEW, "%s: distibutor is off, calling arm_gic_init instead\n", __func__);
		arm_gic_init();
		resume_gicd = true;
	} else {
		arm_gic_init_percpu(0);
	}
	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);
	suspend_resume_fiq(true, resume_gicd);
}

LK_INIT_HOOK_FLAGS(arm_gic_resume_cpu, arm_gic_resume_cpu,
		LK_INIT_LEVEL_PLATFORM, LK_INIT_FLAG_CPU_RESUME);

static int arm_gic_max_cpu(void)
{
	return (GIC_READ(GIC_DIST_BASE + GICD_TYPER) >> 5) & 0x7;
}

/*******************************************************************************
 * Enable secure interrupts and use FIQs to route them. Disable legacy bypass
 * and set the priority mask register to allow all interrupts to trickle in.
 ******************************************************************************/
void arm_gic_redist_init(void)
{
	/* set all SGI/PPI as non-secure GROUP1 by default.
	   rdist_base + 64K == SGI_base */
	GIC_WRITE(GIC_REDIS_BASE+SZ_64K+GICE_V3_IGROUP0, 0xffffffff);
	GIC_WRITE(GIC_REDIS_BASE+SZ_64K+GICE_V3_IGRPMOD0, 0x0);
}

static void arm_gic_distif_init(void)
{
	unsigned int i, ctrl, irq_set;

	/* Disable the distributor before going further */
	ctrl = GIC_READ(GIC_DIST_BASE + GICD_CTLR);
	ctrl &= ~(GICD_CTLR_ENABLE_GRP0 | GICD_CTLR_ENGRP1NS | GICD_CTLR_ENGRP1S);
	GIC_WRITE(GIC_DIST_BASE + GICD_CTLR, ctrl);

	mt_git_dist_rwp();

	/*
	 * Mark out non-secure SPI interrupts. The number of interrupts is
	 * calculated as 32 * (IT_LINES + 1). We do 32 at a time.
	 */
	irq_set = (GIC_READ(GIC_DIST_BASE + GICD_TYPER)&IT_LINES_NO_MASK) + 1;
	irq_set = irq_set * 32;

	/* set all SPI as non-secure group1 by default,
	 * index from 1, because GICE_V3_IGROUP0, GICE_V3_IGRPMOD0 are RES0,
	 * equivalent function is provided by GICR_IGROUPR0, GICE_V3_IGRPMOD0,
	 * which are both initialized in gic_cpuif_init() */
	for (i = 32; i < irq_set; i += 32) {
		GIC_WRITE(GIC_DIST_BASE+GICD_IGROUPR + i * 4 / 32, 0x0);
		GIC_WRITE(GIC_DIST_BASE+GICD_IGRPMODR + i * 4 / 32, 0xffffffff);
	}

	/*
	 * Set all global interrupts to be level triggered, active low.
	 */
	for (i = 32; i < irq_set; i += 16) {
		GIC_WRITE(GIC_DIST_BASE + GICD_ICFGR + i * 4 / 16, 0);
	}

	/*
	 * Set all global interrupts to this CPU only.
	 */
	if (arm_gic_max_cpu() > 0) {
		/* Set external interrupts to target cpu 0 */
		for (i = 32; i < irq_set; i += 4) {
			GIC_READ(GIC_DIST_BASE + GICD_ITARGETSR + (i / 4) * 4) = gicd_itargetsr[i / 4];
		}
	}

	/*
	 * Set priority on all interrupts.
	 */
	for (i = 0; i < irq_set; i += 4) {
		GIC_WRITE(GIC_DIST_BASE + GICD_IPRIORITYR + i * 4 / 4, 0xA0A0A0A0);
	}

	/*
	 * Disable all interrupts.
	 */
	for (i = 0; i < irq_set; i += 32) {
		GIC_WRITE(GIC_DIST_BASE + GICD_ICENABLER + i * 4 / 32, 0xFFFFFFFF);
	}

	/*
	 * Clear all active status
	 */
	for (i = 0; i < irq_set; i += 32) {
		GIC_WRITE(GIC_DIST_BASE + GICD_ICACTIVER + i * 4 / 32, 0xFFFFFFFF);
	}

	/*
	 * Clear all pending status
	 */
	for (i = 0; i < irq_set; i += 32) {
		GIC_WRITE(GIC_DIST_BASE + GICD_ICPENDR + i * 4 / 32, 0xFFFFFFFF);
	}

	/* enable all groups & ARE */
	ctrl = GICD_CTLR_ENABLE_GRP0 | GICD_CTLR_ENGRP1NS | GICD_CTLR_ENGRP1S |
		GICD_CTLR_ARE | GICD_CTLR_ARE_NS;
	GIC_WRITE(GIC_DIST_BASE + GICD_CTLR, ctrl);

	mt_git_dist_rwp();
}

void clear_sec_pol_ctl_en(void)
{
	unsigned int i;

	/* total 19 polarity ctrl registers */
	for (i = 0; i <= NR_INT_POL_CTL-1; i++) {
		GIC_WRITE((SEC_POL_CTL_EN0 + (i * 4)), 0);
	}
}

void arm_gic_init(void)
{
	LTRACEF("[LK GIC] before arm_gic_cpuif_setup\n");
	arm_gic_redist_init();

	LTRACEF("[LK GIC] before arm_gic_distif_init\n");
	arm_gic_distif_init();

#if WITH_LIB_SM
	GIC_READ(GIC_DIST_BASE + GICD_CTLR) = 3; // enable GIC0 ns interrupts
	/*
	 * Iterate through all IRQs and set them to non-secure
	 * mode. This will allow the non-secure side to handle
	 * all the interrupts we don't explicitly claim.
	 */
	for (i = 32; i < MAX_INT; i += 32) {
		u_int reg = i / 32;
		GIC_READ(GIC_DIST_BASE + GICD_IGROUPR + reg * 4) = gicd_igroupr[reg];
	}
#endif
	arm_gic_init_percpu(0);

	LTRACEF("[LK GIC] before clear_sec_pol_ctl_en\n");
	clear_sec_pol_ctl_en();

}

static status_t arm_gic_set_secure_locked(u_int irq, bool secure)
{
#if WITH_LIB_SM
	int reg = irq / 32;
	uint32_t mask = 1ULL << (irq % 32);

	if (irq >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (secure)
		GIC_READ(GIC_DIST_BASE + GICD_IGROUPR + reg * 4) = (gicd_igroupr[reg] &= ~mask);
	else
		GIC_READ(GIC_DIST_BASE + GICD_IGROUPR + reg * 4) = (gicd_igroupr[reg] |= mask);
	LTRACEF("irq %d, secure %d, GICD_IGROUP%d = %x\n",
		irq, secure, reg, GIC_READ(GIC_DIST_BASE + GICD_IGROUPR + reg * 4);
#endif
	return NO_ERROR;
}

static status_t arm_gic_set_target_locked(u_int irq, u_int cpu_mask, u_int enable_mask)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	u_int old_val;
	u_int new_val;

	cpu_mask = (cpu_mask & 0xff) << shift;
	enable_mask = (enable_mask << shift) & cpu_mask;

	old_val = GIC_READ(GIC_DIST_BASE + GICD_ITARGETSR + reg * 4);
	new_val = (gicd_itargetsr[reg] & ~cpu_mask) | enable_mask;
	GIC_READ(GIC_DIST_BASE + GICD_ITARGETSR + reg * 4) = gicd_itargetsr[reg] = new_val;
	LTRACEF("irq %i, GICD_ITARGETSR%d %x => %x (got %x)\n",
		irq, reg, old_val, new_val, GIC_READ(GIC_DIST_BASE + GICD_ITARGETSR + reg));

	return NO_ERROR;
}

static status_t arm_gic_get_priority(u_int irq)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	return (GIC_READ(GIC_DIST_BASE + GICD_IPRIORITYR + reg * 4) >> shift) & 0xff;
}

static status_t arm_gic_set_priority_locked(u_int irq, uint8_t priority)
{
	u_int reg = irq / 4;
	u_int shift = 8 * (irq % 4);
	u_int mask = 0xff << shift;
	uint32_t regval;

	regval = GIC_READ(GIC_DIST_BASE + GICD_IPRIORITYR + reg);
	LTRACEF("irq %i, old GICD_IPRIORITYR%d = %x\n", irq, reg, regval);
	regval = (regval & ~mask) | ((uint32_t)priority << shift);
	GIC_READ(GIC_DIST_BASE + GICD_IPRIORITYR + reg * 4) = regval;
	LTRACEF("irq %i, new GICD_IPRIORITYR%d = %x, req %x\n",
		irq, reg, GIC_READ(GIC_DIST_BASE + GICD_IPRIORITYR + reg * 4), regval);

	return 0;
}

status_t arm_gic_sgi(u_int irq, u_int flags, u_int cpu_mask)
{
	u_int val =
		((flags & ARM_GIC_SGI_FLAG_TARGET_FILTER_MASK) << 24) |
		((cpu_mask & 0xff) << 16) |
		((flags & ARM_GIC_SGI_FLAG_NS) ? (1U << 15) : 0) |
		(irq & 0xf);

	if (irq >= 16)
		return ERR_INVALID_ARGS;

	LTRACEF("GICD_SGIR: %x\n", val);

	GIC_READ(GIC_DIST_BASE + GICD_SGIR) = val;

	return NO_ERROR;
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (arm_gic_interrupt_change_allowed(vector))
		gic_set_enable(vector, false);

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	if (arm_gic_interrupt_change_allowed(vector))
		gic_set_enable(vector, true);

	return NO_ERROR;
}

static uint64_t gicc_read_hppir1_el1(void)
{
	uint64_t val = 0;

	__asm__ volatile("mrs_s %0, " __stringify(ICC_HPPIR1_EL1) : "=r" (val));

	return val;
}

uint32_t arm_gic_get_pending_interrupt_id(void)
{
	return gicc_read_hppir1_el1();
}

uint64_t gicc_read_iar1_el1(void)
{
	u64 irqstat;
	__asm__ volatile("mrs_s %0, " __stringify(ICC_IAR1_EL1) : "=r" (irqstat));
	return irqstat;
}

uint64_t arm_gic_acknowledge_irq(void)
{
	return gicc_read_iar1_el1();
}

static void gicc_write_eoi1_el1(uint32_t irq)
{
	__asm__ volatile("msr_s " __stringify(ICC_EOIR1_EL1) ", %0" : : "r" (irq));
}

static void arm_gic_end_of_irq(uint32_t id)
{
	gicc_write_eoi1_el1(id);
}

uint64_t gicc_read_iar0_el1(void)
{
	u64 irqstat;
	__asm__ volatile("mrs_s %0, " __stringify(ICC_IAR0_EL1) : "=r" (irqstat));
	return irqstat;
}

uint64_t arm_gic_acknowledge_fiq(void)
{
	return gicc_read_iar0_el1();
}

static void gicc_write_eoi0_el1(uint32_t irq)
{
	__asm__ volatile("msr_s " __stringify(ICC_EOIR0_EL1) ", %0" : : "r" (irq));
}

static void arm_gic_end_of_fiq(uint32_t id)
{
	gicc_write_eoi0_el1(id);
}

static
enum handler_return __platform_irq(struct iframe *frame)
{
	// get the current vector
	uint32_t iar = arm_gic_acknowledge_irq();
	unsigned int vector = iar & 0x3ff;

	if (vector >= 0x3fe) {
		// spurious
		return INT_NO_RESCHEDULE;
	}

	THREAD_STATS_INC(interrupts);
	KEVLOG_IRQ_ENTER(vector);

	uint cpu = arch_curr_cpu_num();

	LTRACEF_LEVEL(2, "iar 0x%x cpu %u currthread %p vector %d pc 0x%lx\n", iar, cpu,
	       get_current_thread(), vector, (uintptr_t)IFRAME_PC(frame));

	// deliver the interrupt
	enum handler_return ret;

	ret = INT_NO_RESCHEDULE;
	struct int_handler_struct *handler = get_int_handler(vector, cpu);
	if (handler->handler)
		ret = handler->handler(handler->arg);

	arm_gic_end_of_irq(iar);

	LTRACEF_LEVEL(2, "cpu %u exit %d\n", cpu, ret);

	KEVLOG_IRQ_EXIT(vector);

	return ret;
}

enum handler_return platform_irq(struct iframe *frame)
{
#if WITH_LIB_SM
	uint32_t ahppir = arm_gic_get_pending_interrupt_id();
	uint32_t pending_irq = ahppir & 0x3ff;
	struct int_handler_struct *h;
	uint cpu = arch_curr_cpu_num();

	LTRACEF("ahppir %d\n", ahppir);
	if (pending_irq < MAX_INT && get_int_handler(pending_irq, cpu)->handler) {
		enum handler_return ret = 0;
		uint32_t irq;
		uint8_t old_priority;
		spin_lock_saved_state_t state;

		spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

		/* Temporarily raise the priority of the interrupt we want to
		 * handle so another interrupt does not take its place before
		 * we can acknowledge it.
		 */
		old_priority = arm_gic_get_priority(pending_irq);
		arm_gic_set_priority_locked(pending_irq, 0);
		DSB;
		irq = arm_gic_acknowledge_irq() & 0x3ff;
		arm_gic_set_priority_locked(pending_irq, old_priority);

		spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

		LTRACEF("irq %d\n", irq);
		if (irq < MAX_INT && (h = get_int_handler(pending_irq, cpu))->handler)
			ret = h->handler(h->arg);
		else
			TRACEF("unexpected irq %d != %d may get lost\n", irq, pending_irq);
		arm_gic_end_of_fiq(irq);

		return ret;
	}
	return sm_handle_irq();
#else
	return __platform_irq(frame);
#endif
}

void platform_fiq(struct iframe *frame)
{
#if WITH_LIB_SM
	sm_handle_fiq();
#else
	PANIC_UNIMPLEMENTED;
#endif
}

#if WITH_LIB_SM
static status_t arm_gic_get_next_irq_locked(u_int min_irq, bool per_cpu)
{
	u_int irq;
	u_int max_irq = per_cpu ? GIC_MAX_PER_CPU_INT : MAX_INT;
	uint cpu = arch_curr_cpu_num();

	if (!per_cpu && min_irq < GIC_MAX_PER_CPU_INT)
		min_irq = GIC_MAX_PER_CPU_INT;

	for (irq = min_irq; irq < max_irq; irq++)
		if (get_int_handler(irq, cpu)->handler)
			return irq;

	return SM_ERR_END_OF_INPUT;
}

long smc_intc_get_next_irq(smc32_args_t *args)
{
	status_t ret;
	spin_lock_saved_state_t state;

	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	arm_gic_non_secure_interrupts_frozen = true;
	ret = arm_gic_get_next_irq_locked(args->params[0], args->params[1]);
	LTRACEF("min_irq %d, per_cpu %d, ret %d\n",
		args->params[0], args->params[1], ret);

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

	return ret;
}

static u_long enabled_fiq_mask[BITMAP_NUM_WORDS(MAX_INT)];

static void bitmap_update_locked(u_long *bitmap, u_int bit, bool set)
{
	u_long mask = 1UL << BITMAP_BIT_IN_WORD(bit);

	bitmap += BITMAP_WORD(bit);
	if (set)
		*bitmap |= mask;
	else
		*bitmap &= ~mask;
}

long smc_intc_request_fiq(smc32_args_t *args)
{
	u_int fiq = args->params[0];
	bool enable = args->params[1];
	spin_lock_saved_state_t state;

	dprintf(SPEW, "%s: fiq %d, enable %d\n", __func__, fiq, enable);
	spin_lock_save(&gicd_lock, &state, GICD_LOCK_FLAGS);

	arm_gic_set_secure_locked(fiq, true);
	arm_gic_set_target_locked(fiq, ~0, ~0);
	arm_gic_set_priority_locked(fiq, 0);

	gic_set_enable(fiq, enable);
	bitmap_update_locked(enabled_fiq_mask, fiq, enable);

	dprintf(SPEW, "%s: fiq %d, enable %d done\n", __func__, fiq, enable);

	spin_unlock_restore(&gicd_lock, state, GICD_LOCK_FLAGS);

	return NO_ERROR;
}

static u_int current_fiq[8] = { 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff, 0x3ff };

static bool update_fiq_targets(u_int cpu, bool enable, u_int triggered_fiq, bool resume_gicd)
{
	u_int i, j;
	u_long mask;
	u_int fiq;
	bool smp = arm_gic_max_cpu() > 0;
	bool ret = false;

	spin_lock(&gicd_lock); /* IRQs and FIQs are already masked */
	for (i = 0; i < BITMAP_NUM_WORDS(MAX_INT); i++) {
		mask = enabled_fiq_mask[i];
		while (mask) {
			j = _ffz(~mask);
			mask &= ~(1UL << j);
			fiq = i * BITMAP_BITS_PER_WORD + j;
			if (fiq == triggered_fiq)
				ret = true;
			LTRACEF("cpu %d, irq %i, enable %d\n", cpu, fiq, enable);
			if (smp)
				arm_gic_set_target_locked(fiq, 1U << cpu, enable ? ~0 : 0);
			if (!smp || resume_gicd)
				gic_set_enable(fiq, enable);
		}
	}
	spin_unlock(&gicd_lock);
	return ret;
}

static void suspend_resume_fiq(bool resume_gicc, bool resume_gicd)
{
	u_int cpu = arch_curr_cpu_num();

	ASSERT(cpu < 8);

	update_fiq_targets(cpu, resume_gicc, ~0, resume_gicd);
}

status_t sm_intc_fiq_enter(void)
{
	u_int cpu = arch_curr_cpu_num();
	u_int irq = arm_gic_acknowledge_fiq() & 0x3ff;
	bool fiq_enabled;

	ASSERT(cpu < 8);

	LTRACEF("cpu %d, irq %i\n", cpu, irq);

	if (irq >= 1020) {
		LTRACEF("spurious fiq: cpu %d, old %d, new %d\n", cpu, current_fiq[cpu], irq);
		return ERR_NO_MSG;
	}

	fiq_enabled = update_fiq_targets(cpu, false, irq, false);
	arm_gic_end_of_fiq(irq);

	if (current_fiq[cpu] != 0x3ff) {
		dprintf(INFO, "more than one fiq active: cpu %d, old %d, new %d\n", cpu, current_fiq[cpu], irq);
		return ERR_ALREADY_STARTED;
	}

	if (!fiq_enabled) {
		dprintf(INFO, "got disabled fiq: cpu %d, new %d\n", cpu, irq);
		return ERR_NOT_READY;
	}

	current_fiq[cpu] = irq;

	return 0;
}

void sm_intc_fiq_exit(void)
{
	u_int cpu = arch_curr_cpu_num();

	ASSERT(cpu < 8);

	LTRACEF("cpu %d, irq %i\n", cpu, current_fiq[cpu]);
	if (current_fiq[cpu] == 0x3ff) {
		dprintf(INFO, "%s: no fiq active, cpu %d\n", __func__, cpu);
		return;
	}
	update_fiq_targets(cpu, true, current_fiq[cpu], false);
	current_fiq[cpu] = 0x3ff;
}
#endif

/* vim: set ts=4 sw=4 noexpandtab: */

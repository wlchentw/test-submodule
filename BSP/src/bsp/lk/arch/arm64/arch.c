/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <arch/arm64/mmu.h>
#include <arch/mp.h>
#include <kernel/thread.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif
#include <lk/init.h>
#include <lk/main.h>
#include <platform.h>
#include <target.h>
#include <trace.h>

#define LOCAL_TRACE 0

#if WITH_SMP
/* smp boot lock */
static spin_lock_t arm_boot_cpu_lock = 1;
static volatile int secondaries_to_init = 0;
#endif

static void arm64_cpu_early_init(void)
{
    /* set the vector base */
    ARM64_WRITE_SYSREG(VBAR_EL1, (uint64_t)&arm64_exception_base);

    /* switch to EL1 */
    unsigned int current_el = ARM64_READ_SYSREG(CURRENTEL) >> 2;
    if (current_el > 1) {
        arm64_el3_to_el1();
    }

    arch_enable_fiqs();
}

void arch_early_init(void)
{
    arm64_cpu_early_init();
    platform_init_mmu_mappings();
}

void arch_init(void)
{
#if WITH_SMP
    arch_mp_init_percpu();

    LTRACEF("midr_el1 0x%llx\n", ARM64_READ_SYSREG(midr_el1));

    secondaries_to_init = SMP_MAX_CPUS - 1; /* TODO: get count from somewhere else, or add cpus as they boot */

    lk_init_secondary_cpus(secondaries_to_init);

    LTRACEF("releasing %d secondary cpus\n", secondaries_to_init);

    /* release the secondary cpus */
    spin_unlock(&arm_boot_cpu_lock);

    /* flush the release of the lock, since the secondary cpus are running without cache on */
    arch_clean_cache_range((addr_t)&arm_boot_cpu_lock, sizeof(arm_boot_cpu_lock));
#endif
}

void arch_quiesce(void)
{
}

void arch_idle(void)
{
    __asm__ volatile("wfi");
}

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
    LTRACEF("entry %p, args 0x%lx 0x%lx 0x%lx 0x%lx\n", entry, arg0, arg1, arg2, arg3);

    arch_disable_ints();

    /* give target and platform a chance to put hardware into a suitable
     * state for chain loading.
     */
    target_quiesce();
    platform_quiesce();

    paddr_t entry_pa;
    paddr_t loader_pa;

#if WITH_KERNEL_VM
    entry_pa = kvaddr_to_paddr(entry);
    if (entry_pa == (paddr_t)NULL) {
        panic("error translating entry physical address\n");
    }

    LTRACEF("entry pa 0x%lx\n", entry_pa);

    loader_pa = kvaddr_to_paddr((void *)&arm64_chain_load);
    if (loader_pa == (paddr_t)NULL) {
        panic("error translating loader physical address\n");
    }

    LTRACEF("loader pa 0x%lx\n", loader_pa);

    /* TTBR0_EL1 already contains the physical address mapping */
    ARM64_WRITE_SYSREG(tcr_el1, (uint64_t)MMU_TCR_FLAGS_IDENT);
#else
    entry_pa = (paddr_t)entry;
    loader_pa = (paddr_t)&arm64_chain_load;
#endif

    LTRACEF("disabling instruction/data cache\n");
    arch_disable_cache(UCACHE);

    /* put the booting cpu back into close to a default state */
    arch_quiesce();

    LTRACEF("branching to physical address of loader\n");

    /* branch to the physical address version of the chain loader routine */
    void (*loader)(paddr_t entry, ulong, ulong, ulong, ulong) __NO_RETURN = (void *)loader_pa;
    loader(entry_pa, arg0, arg1, arg2, arg3);
}

#if WITH_SMP
void arm64_secondary_entry(ulong asm_cpu_num)
{
    uint cpu = arch_curr_cpu_num();
    if (cpu != asm_cpu_num)
        return;

    arm64_cpu_early_init();

    spin_lock(&arm_boot_cpu_lock);
    spin_unlock(&arm_boot_cpu_lock);

    /* run early secondary cpu init routines up to the threading level */
    lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

    arch_mp_init_percpu();

    LTRACEF("cpu num %d\n", cpu);

    /* we're done, tell the main cpu we're up */
    atomic_add(&secondaries_to_init, -1);
    __asm__ volatile("sev");

    lk_secondary_cpu_entry();
}
#endif


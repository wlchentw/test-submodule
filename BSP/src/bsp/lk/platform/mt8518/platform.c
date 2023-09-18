/*
 * Copyright (c) 2018 MediaTek Inc.
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

#include <arch.h>
#include <arch/arm64/mmu.h>
#include <arch/ops.h>
#include <assert.h>
#include <debug.h>
#include <dev/timer/arm_generic.h>
#include <dev/uart.h>
#include <err.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/mt8518.h>
#include <platform/mt_gic_v3.h>
#include <platform/pll.h>
#include <platform/dramc_api.h>
#include <lib/mempool.h>
#if WITH_MTK_PMIC_WRAP_AND_PMIC
#include <platform/pmic_wrap_init.h>
#include <platform/pmic.h>
#endif
#if WITH_PMIC_MT6395
#include <platform/pmic_6395.h>
#endif
#if WITH_VCORE_PWM_BUCK
#include <platform/pwm-buck.h>
#endif
#if WITH_VCORE_I2C_BUCK
#include <platform/rt5748.h>
#endif
#include <platform/pwm.h>

#if WITH_KERNEL_VM
#define L2C_MAPPING_IDX 0
#define PERIPHERAL_MAPPING_IDX 1
#define SRAM_MAPPING_IDX 2
#define DRAM_MAPPING_IDX 3
#define GIC_MAPPING_IDX 4
#include <platform/mtk_wdt.h>

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    {
        .phys = MEMORY_BASE_PHYS,
        .virt = MEMORY_BASE_VIRT,
        .size = MEMORY_APERTURE_SIZE,
        .flags = 0,
        .name = "l2c"
    },
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
    },
    {
        .phys = VERSION_BASE_PHYS,
        .virt = VERSION_BASE_VIRT,
        .size = VERSION_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "icversion"
    },
    {
        .phys = GIC_BASE_PHYS,
        .virt = GIC_BASE_VIRT,
        .size = GIC_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "gic"
    },
    {
        .phys = DRAM_BASE_PHY,
        .virt = DRAM_BASE_VIRT,
        .size = 0x20000000,
        .flags = MMU_INITIAL_MAPPING_FLAG_UNCACHED,
        .name = "dram"
    },
    /* reserved for internal sram */
    { 0 },
    /* reserved for dram */
    { 0 },
    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "sdram",
    .base = SRAM_BASE_PHYS,
    .size = SRAM_BASE_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

/* only enable el1 dcache */
static void dcache_enable(void)
{
    uint32_t sctlr;

    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr) : : "cc");
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr | (1 << 2)) : "cc");
    asm volatile("isb");
}

uint32_t lk_dram_sz =  0x10000000; //get_dram_size();;
void *dram_map(paddr_t pa)
{
    paddr_t dram_phy = DRAM_BASE_PHY;

    if (pa >= dram_phy && pa <= (dram_phy + lk_dram_sz - 1)) {
        return (void *)(DRAM_BASE_VIRT + (pa - dram_phy));
    }

    return NULL;
}

#endif /* WITH_KERNEL_VM */

void platform_early_init(void)
{
    uart_init_early();

    /* initialize the interrupt controller */
    arm_gic_init();

    arm_generic_timer_init(ARM_GENERIC_TIMER_PHYSICAL_INT, 13000000);

    mtk_wdt_init();

#if WITH_KERNEL_VM
    arch_disable_cache(DCACHE);
#endif

#if !(FPGA_PLATFORM)
    mt_pll_init();
#endif

    pwm_init();

#if WITH_MTK_PMIC_WRAP_AND_PMIC
    pwrap_init();
    pmic_init();
#endif

#if WITH_PMIC_MT6395
    pmic_init_mt6395();
#endif

#if WITH_VCORE_PWM_BUCK
    pwm_buck_init();
#endif

#if WITH_VCORE_I2C_BUCK
    rt5748_init();
#endif

#if !(FPGA_PLATFORM)
    /* mt_pll_post_init should be invoked after pmic_init */
    mt_pll_post_init();
#endif

	/* check DDR-reserve mode */
	check_ddr_reserve_status();

#if !(FPGA_PLATFORM)
    mt_mem_init();
#endif

#if WITH_KERNEL_VM
    dcache_enable();

    /* add DRAM to mmu_initial_mappings for physical-to-virtual translation */
    mmu_initial_mappings[DRAM_MAPPING_IDX].phys = DRAM_BASE_PHY;
    mmu_initial_mappings[DRAM_MAPPING_IDX].virt = DRAM_BASE_VIRT;
#if FPGA_PLATFORM
    mmu_initial_mappings[DRAM_MAPPING_IDX].size = 0x10000000;
#else
    mmu_initial_mappings[DRAM_MAPPING_IDX].size = 0x10000000;//get_dram_size();
#endif
    mmu_initial_mappings[DRAM_MAPPING_IDX].flags = 0;
    mmu_initial_mappings[DRAM_MAPPING_IDX].name = "dram";

    /* mapping internel sram to cacheable memory */
    arch_mmu_map(SRAM_BASE_VIRT, SRAM_BASE_PHYS,  SRAM_BASE_SIZE >> PAGE_SIZE_SHIFT, 0);
    /* add intrenal sram to mmu_initial_mappings for heap */
    mmu_initial_mappings[SRAM_MAPPING_IDX].phys = SRAM_BASE_PHYS;
    mmu_initial_mappings[SRAM_MAPPING_IDX].virt = SRAM_BASE_VIRT;
    mmu_initial_mappings[SRAM_MAPPING_IDX].size = SRAM_BASE_SIZE;
    mmu_initial_mappings[SRAM_MAPPING_IDX].flags = 0;
    mmu_initial_mappings[SRAM_MAPPING_IDX].name = "sram";

    pmm_add_arena(&arena);
#endif
}

void platform_init(void)
{
    int ret;
    ret = mempool_init((void *)CACHED_MEMPOOL_ADDR, CACHED_MEMPOOL_SIZE,
                       MEMPOOL_CACHE);
    if (ret != NO_ERROR)
        platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_PANIC);

}

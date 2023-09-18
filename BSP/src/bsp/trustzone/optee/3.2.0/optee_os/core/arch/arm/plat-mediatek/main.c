// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2015, Linaro Limited
 */

#include <console.h>
#include <drivers/gic.h>
#include <drivers/serial8250_uart.h>
#include <kernel/generic_boot.h>
#include <kernel/panic.h>
#include <kernel/pm_stubs.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <stdint.h>
#include <tee/entry_std.h>
#include <tee/entry_fast.h>

static void main_fiq(void);

register_phys_mem(MEM_AREA_IO_NSEC,
		  CONSOLE_UART_BASE, SERIAL8250_UART_REG_SIZE);
register_phys_mem(MEM_AREA_IO_SEC, GICC_BASE, GICC_SIZE);
register_phys_mem(MEM_AREA_IO_SEC, GICD_BASE, GICD_SIZE);

#if defined(DRAM0_BASE) && defined(DRAM0_SIZE)
register_dynamic_shm(DRAM0_BASE, TZDRAM_BASE - DRAM0_BASE);
register_dynamic_shm(TZDRAM_BASE + TZDRAM_SIZE,
		     DRAM0_SIZE - (TZDRAM_BASE + TZDRAM_SIZE - DRAM0_BASE));
#endif

#ifdef CFG_WITH_GIC_INIT
#if defined(PLATFORM_FLAVOR_mt6885)
#define WDT_IRQ_BIT_ID 465
static enum itr_return __maybe_unused wdt_dummy_ihr(struct itr_handler *handler)
{
	/* just for setting wdt interupt back to group0, should not be run */
	panic();
	return ITRR_HANDLED;
}

static struct itr_handler wdt_itr = {
	.it = WDT_IRQ_BIT_ID,
	.handler = wdt_dummy_ihr,
};
#endif
#endif
static const struct thread_handlers handlers = {
	.std_smc = tee_entry_std,
	.fast_smc = tee_entry_fast,
	.nintr = main_fiq,
	.cpu_on = cpu_on_handler,
	.cpu_off = pm_do_nothing,
	.cpu_suspend = pm_do_nothing,
	.cpu_resume = pm_do_nothing,
	.system_off = pm_do_nothing,
	.system_reset = pm_do_nothing,
};

static struct gic_data gic_data;

void main_init_gic(void)
{
	vaddr_t gicc_base;
	vaddr_t gicd_base;

	gicc_base = (vaddr_t)phys_to_virt_io(GICC_BASE);
	gicd_base = (vaddr_t)phys_to_virt_io(GICD_BASE);

	if (!gicc_base || !gicd_base)
		panic();

#ifdef CFG_WITH_GIC_INIT
	gic_init(&gic_data, gicc_base, gicd_base);
#else
	gic_init_base_addr(&gic_data, gicc_base, gicd_base);
#endif
	itr_init(&gic_data.chip);

#ifdef CFG_WITH_GIC_INIT
#if defined(PLATFORM_FLAVOR_mt6885)
	itr_add(&wdt_itr); /* set wdt itr to group0 for atf */
#endif
#endif
}

void main_secondary_init_gic(void)
{
	gic_cpu_init(&gic_data);
}

static struct serial8250_uart_data console_data;

const struct thread_handlers *generic_boot_get_handlers(void)
{
	return &handlers;
}

static void main_fiq(void)
{
	gic_it_handle(&gic_data);
}

void console_init(void)
{
	serial8250_uart_init(&console_data, CONSOLE_UART_BASE,
			     CONSOLE_UART_CLK_IN_HZ, CONSOLE_BAUDRATE);
	register_serial_console(&console_data.chip);
}

/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <arch.h>
#include <asm_macros.S>
#include <context.h>
#include <cpu_data.h>
#if defined(MTK_PLAT_PORTING_LAYER)
#include <log.h>
#endif
#include <plat_macros.S>
#include <platform_def.h>
#include <utils_def.h>

	.globl	report_unhandled_exception
	.globl	report_unhandled_interrupt
	.globl	el3_panic

#if CRASH_REPORTING
#if defined(MTK_PLAT_PORTING_LAYER)
#define REG_SIZE	0x8
#define ATF_RAM_DUMP_LOCK_PATTERN	0x5ac8
#define ATF_RAM_DUMP_START_FLAG		0xa7fd07eea7fd07ee
#define ATF_RAM_DUMP_END_FLAG 		0xe1de2de3de4de5d6
#define ATF_RAM_DUMP_BAD_FLAG		0xbadbadbadbadbadb
#endif

	/* ------------------------------------------------------
	 * The below section deals with dumping the system state
	 * when an unhandled exception is taken in EL3.
	 * The layout and the names of the registers which will
	 * be dumped during a unhandled exception is given below.
	 * ------------------------------------------------------
	 */
.section .rodata.crash_prints, "aS"
print_spacer:
	.asciz	" =\t\t0x"

gp_regs:
	.asciz	"x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",\
		"x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",\
		"x16", "x17", "x18", "x19", "x20", "x21", "x22",\
		"x23", "x24", "x25", "x26", "x27", "x28", "x29", ""
el3_sys_regs:
	.asciz	"scr_el3", "sctlr_el3", "cptr_el3", "tcr_el3",\
		"daif", "mair_el3", "spsr_el3", "elr_el3", "ttbr0_el3",\
		"esr_el3", "far_el3", ""

non_el3_sys_regs:
	.asciz	"spsr_el1", "elr_el1", "spsr_abt", "spsr_und",\
		"spsr_irq", "spsr_fiq", "sctlr_el1", "actlr_el1", "cpacr_el1",\
		"csselr_el1", "sp_el1", "esr_el1", "ttbr0_el1", "ttbr1_el1",\
		"mair_el1", "amair_el1", "tcr_el1", "tpidr_el1", "tpidr_el0",\
		"tpidrro_el0", "dacr32_el2", "ifsr32_el2", "par_el1",\
		"mpidr_el1", "afsr0_el1", "afsr1_el1", "contextidr_el1",\
		"vbar_el1", "cntp_ctl_el0", "cntp_cval_el0", "cntv_ctl_el0",\
		"cntv_cval_el0", "cntkctl_el1", "sp_el0", "isr_el1", ""

panic_msg:
	.asciz "PANIC in EL3 at x30 = 0x"
excpt_msg:
	.asciz "Unhandled Exception in EL3.\nx30 =\t\t0x"
intr_excpt_msg:
	.asciz "Unhandled Interrupt Exception in EL3.\nx30 =\t\t0x"
#if defined(MTK_PLAT_PORTING_LAYER)
dump_reg_msg:
	.asciz "Dump address :\t\t0x"
dump_last_lr_msg:
	.asciz "Dump last LR address :\t\t0x"
timestamp_msg:
	.asciz "Timestamp:\t0x"
print_dot:
	.asciz	"."
#endif
	/*
	 * Helper function to print newline to console.
	 */
func print_newline
	mov	x0, '\n'
	b	plat_crash_console_putc
endfunc print_newline

	/*
	 * Helper function to print from crash buf.
	 * The print loop is controlled by the buf size and
	 * ascii reg name list which is passed in x6. The
	 * function returns the crash buf address in x0.
	 * Clobbers : x0 - x7, sp
	 */
func size_controlled_print
	/* Save the lr */
	mov	sp, x30
	/* load the crash buf address */
	mrs	x7, tpidr_el3
test_size_list:
	/* Calculate x5 always as it will be clobbered by asm_print_hex */
	mrs	x5, tpidr_el3
	add	x5, x5, #CPU_DATA_CRASH_BUF_SIZE
	/* Test whether we have reached end of crash buf */
	cmp	x7, x5
	b.eq	exit_size_print
	ldrb	w4, [x6]
	/* Test whether we are at end of list */
	cbz	w4, exit_size_print
	mov	x4, x6
	/* asm_print_str updates x4 to point to next entry in list */
	bl	asm_print_str
	/* update x6 with the updated list pointer */
	mov	x6, x4
	adr	x4, print_spacer
	bl	asm_print_str
	ldr	x4, [x7], #REGSZ
	bl	asm_print_hex
	bl	print_newline
	b	test_size_list
exit_size_print:
	mov	x30, sp
	ret
endfunc size_controlled_print

	/*
	 * Helper function to store x8 - x15 registers to
	 * the crash buf. The system registers values are
	 * copied to x8 to x15 by the caller which are then
	 * copied to the crash buf by this function.
	 * x0 points to the crash buf. It then calls
	 * size_controlled_print to print to console.
	 * Clobbers : x0 - x7, sp
	 */
func str_in_crash_buf_print
	/* restore the crash buf address in x0 */
	mrs	x0, tpidr_el3
	stp	x8, x9, [x0]
	stp	x10, x11, [x0, #REGSZ * 2]
	stp	x12, x13, [x0, #REGSZ * 4]
	stp	x14, x15, [x0, #REGSZ * 6]
	b	size_controlled_print
endfunc str_in_crash_buf_print

	/* ------------------------------------------------------
	 * This macro calculates the offset to crash buf from
	 * cpu_data and stores it in tpidr_el3. It also saves x0
	 * and x1 in the crash buf by using sp as a temporary
	 * register.
	 * ------------------------------------------------------
	 */
	.macro prepare_crash_buf_save_x0_x1
	/* we can corrupt this reg to free up x0 */
	mov	sp, x0
	/* tpidr_el3 contains the address to cpu_data structure */
	mrs	x0, tpidr_el3
#if defined(MTK_PLAT_PORTING_LAYER)
	/* Do not update tpidr_el3 for nested crash.
	 * tpidr_el3 is pointed to cpu_data, which for now is a buffer for save gpreg
	 * temporarily.
	 */
#else
	/* Calculate the Crash buffer offset in cpu_data */
	add	x0, x0, #CPU_DATA_CRASH_BUF_OFFSET
	/* Store crash buffer address in tpidr_el3 */
	msr	tpidr_el3, x0
#endif
	str	x1, [x0, #REGSZ]
	mov	x1, sp
	str	x1, [x0]
	.endm

#if defined(MTK_PLAT_PORTING_LAYER)
	/*
	*Take x9 as dump stack base address
	*Clobbers: x4, x9, x10
	*/
	.macro print_dump_stack
	/* Last LR */
	adr	x4, dump_last_lr_msg
	bl	asm_print_str
	mrs	x4, elr_el2
	bl	asm_print_hex
	bl	print_newline
	adr	x4, dump_reg_msg
	bl	asm_print_str
	mov x4, x9
	bl	asm_print_hex
	bl	print_newline

	add  x10, x9, #0x100
print_dump_loop:
	cmp	x9, x10
	b.eq	exit_print_dump
	ldr	x4, [x9], #8
	bl	asm_print_hex
	bl	print_newline
	b	print_dump_loop
exit_print_dump:
	bl	print_newline
	.endm

	.macro ram_dump
	/* See if ram dump is done by other CPUs*/
	mov	w2, #ATF_RAM_DUMP_LOCK_PATTERN
	adr	x0, atf_ram_dump_lock
	ldr	w1, [x0]
	cmp	w1, w2
	b.eq	done
	str	w2, [x0]
	/* Disable MMU, which imply cache flush & invalidate */
	bl disable_mmu_el3
	/* Sanity check ram dump base address and size */
	/* x22=ram dump base, x23=ram dump size, x24=flag, x25=ram dump end address */
	adr 	x22, atf_ram_dump_base
	ldr 	x22, [x22]
	adr 	x23, atf_ram_dump_size
	ldr 	x23, [x23]
	sub	x23, x23, #8
	add	x25, x22, x23

	ldr   x2, =DRAM_EMI_BASE
	cmp	x22, x2
	b.hi	start		/*if(atf_ram_dump_base > DRAM_EMI_BASE){jump start:}*/

	b	done
start:
	ldr	x24, =ATF_RAM_DUMP_START_FLAG
	str	x24, [x25]
	/* x20=RAM1 size, x21=RAM2 size */
	mov	x0, x23
	ldr 	x1, =BL31_TZRAM_SIZE
/*	ldr 	x2, =TZRAM2_SIZE */
	cmp x0, x1
	/* if(atf_ram_dump_size > BL31_TZRAM_SIZE) */
	/* x20=BL31_TZRAM_SIZE else x20=atf_ram_dump_size */
	b.ls	1f
	mov x20, x1
/*	sub	x0, x0, x1 */
/*	cmp	x0, x2 */
	/* if((atf_ram_dump_size-BL31_TZRAM_SIZE) > TZRAM2_SIZE) */
	/* x21=TZRAM2_SIZE else x21=atf_ram_dump_size-BL31_TZRAM_SIZE */
/*	b.ls	2f */
/*	mov	x21, x2 */
	b 3f
1:
	mov x20, x0
/*	mov x21, #0	*/
	b 3f
2:
/*	mov x21, x0 */
3:
	mov	x0, x22
	ldr 	x1, =BL31_BASE
	mov	x2, x20
	bl	memcpy16
/*	ldr 	x1,=TZRAM2_BASE */
/*	mov	x2, x21	*/
/*	bl	memcpy16	*/
	ldr	x24, =ATF_RAM_DUMP_END_FLAG
	str	x24, [x25]
done:
#endif
	.endm

	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when an unhandled exception
	 * occurs. It prints the CPU state via the crash console
	 * making use of the crash buf. This function will
	 * not return.
	 * -----------------------------------------------------
	 */
func report_unhandled_exception
	prepare_crash_buf_save_x0_x1
	adr	x0, excpt_msg
	mov	sp, x0
	/* This call will not return */
	b	do_crash_reporting
endfunc report_unhandled_exception


	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when an unhandled interrupt
	 * occurs. It prints the CPU state via the crash console
	 * making use of the crash buf. This function will
	 * not return.
	 * -----------------------------------------------------
	 */
func report_unhandled_interrupt
	prepare_crash_buf_save_x0_x1
	adr	x0, intr_excpt_msg
	mov	sp, x0
	/* This call will not return */
	b	do_crash_reporting
endfunc report_unhandled_interrupt

	/* -----------------------------------------------------
	 * This function allows to report a crash (if crash
	 * reporting is enabled) when panic() is invoked from
	 * C Runtime. It prints the CPU state via the crash
	 * console making use of the crash buf. This function
	 * will not return.
	 * -----------------------------------------------------
	 */
func el3_panic
	msr	spsel, #1
	prepare_crash_buf_save_x0_x1
	adr	x0, panic_msg
	mov	sp, x0
	/* This call will not return */
	b	do_crash_reporting
endfunc el3_panic

	/* ------------------------------------------------------------
	 * The common crash reporting functionality. It requires x0
	 * and x1 has already been stored in crash buf, sp points to
	 * crash message and tpidr_el3 contains the crash buf address.
	 * The function does the following:
	 *   - Retrieve the crash buffer from tpidr_el3
	 *   - Store x2 to x6 in the crash buffer
	 *   - Initialise the crash console.
	 *   - Print the crash message by using the address in sp.
	 *   - Print x30 value to the crash console.
	 *   - Print x0 - x7 from the crash buf to the crash console.
	 *   - Print x8 - x29 (in groups of 8 registers) using the
	 *     crash buf to the crash console.
	 *   - Print el3 sys regs (in groups of 8 registers) using the
	 *     crash buf to the crash console.
	 *   - Print non el3 sys regs (in groups of 8 registers) using
	 *     the crash buf to the crash console.
	 * ------------------------------------------------------------
	 */
func do_crash_reporting
#if defined(MTK_PLAT_PORTING_LAYER)
	/*
	 * Check teearg->atf_log_buf_size.
	 * Exit if atf_log_buf_size equals 0
	 */
	adr	x0, ptr_atf_crash_flag
	ldr	x0, [x0]
	mov_imm x1, ATF_BAD_PTR
	cmp	w0, w1
	beq	2f
	/*
	 * set atf crash magic number
	 */
1:
	adr	x0, ptr_atf_crash_flag
	ldr	x0, [x0]
	mov_imm x1, ATF_CRASH_MAGIC_NO
	str	w1, [x0]	/* p_atf_log_ctrl->atf_crash_flag = 0xdead1abf */
	ldr	w0, [x0]	/* can't use w3 return addr, w4, start of buffer addr */
	cmp	w0, w1
	b.ne	1b
2:
#endif
	/* Retrieve the crash buf from tpidr_el3 */
	mrs	x0, tpidr_el3
	/* Store x2 - x6, x30 in the crash buffer */
	stp	x2, x3, [x0, #REGSZ * 2]
	stp	x4, x5, [x0, #REGSZ * 4]
	stp	x6, x30, [x0, #REGSZ * 6]

#if defined(MTK_PLAT_PORTING_LAYER)
	/* Print the crash message. sp points to the crash message */
	bl	print_newline
	/* Pring timestamp */
	mrs	x2, cntpct_el0
	ldr	x3, =atf_time_base
	ldr	x3, [x3]
	sub	x2, x2, x3
	mov	x3, #1000
	mul	x2, x2, x3
	mov	x3, #SYS_COUNTER_FREQ_IN_MHZ
	udiv	x2, x2, x3
	ldr	x3, =normal_time_base
	ldr	x3, [x3]
	sub	x2, x2, x3
	mov x6, x2
	/* sched_clock is in x6 */

	adr x4, timestamp_msg
	bl	asm_print_str
	mov	x4, x6
	bl	asm_print_hex

	mov	x0, ','
	bl	plat_crash_console_putc
	mov	x0, ' '
	bl	plat_crash_console_putc

	ldr x3, =1000000000
	udiv	x4, x6, x3	/* sec_time = cur_time / ATF_SCHED_CLOCK_UNIT */
	/* print sec_time in decimal */
	bl	asm_print_dec
	adr x4, print_dot
	bl	asm_print_str
	ldr x3, =1000000000
	udiv	x4, x6, x3	/* sec_time = cur_time / ATF_SCHED_CLOCK_UNIT */
	msub	x1, x4, x3, x6	/* ns_time = (cur_time % ATF_SCHED_CLOCK_UNIT)/1000 */
	mov x3, #1000
	udiv	x1, x1, x3
	mov x4, x1
	/* print ns_time in decimal */
	bl	asm_print_dec
	bl	print_newline
#else
	/* Initialize the crash console */
	bl	plat_crash_console_init
	/* Verify the console is initialized */
	cbz	x0, crash_panic
	/* Print the crash message. sp points to the crash message */
#endif
	mov	x4, sp
	bl	asm_print_str
	/* load the crash buf address */
	mrs	x0, tpidr_el3
	/* report x30 first from the crash buf */
	ldr	x4, [x0, #REGSZ * 7]
	bl	asm_print_hex
	bl	print_newline
	/* Load the crash buf address */
	mrs	x0, tpidr_el3
	/* Now mov x7 into crash buf */
	str	x7, [x0, #REGSZ * 7]

	/* Report x0 - x29 values stored in crash buf*/
	/* Store the ascii list pointer in x6 */
	adr	x6, gp_regs
	/* Print x0 to x7 from the crash buf */
	bl	size_controlled_print
	/* Store x8 - x15 in crash buf and print */
	bl	str_in_crash_buf_print
	/* Load the crash buf address */
	mrs	x0, tpidr_el3
	/* Store the rest of gp regs and print */
	stp	x16, x17, [x0]
	stp	x18, x19, [x0, #REGSZ * 2]
	stp	x20, x21, [x0, #REGSZ * 4]
	stp	x22, x23, [x0, #REGSZ * 6]
	bl	size_controlled_print
	/* Load the crash buf address */
	mrs	x0, tpidr_el3
	stp	x24, x25, [x0]
	stp	x26, x27, [x0, #REGSZ * 2]
	stp	x28, x29, [x0, #REGSZ * 4]
	bl	size_controlled_print

	/* Print the el3 sys registers */
	adr	x6, el3_sys_regs
	mrs	x8, scr_el3
	mrs	x9, sctlr_el3
	mrs	x10, cptr_el3
	mrs	x11, tcr_el3
	mrs	x12, daif
	mrs	x13, mair_el3
	mrs	x14, spsr_el3
	mrs	x15, elr_el3
	bl	str_in_crash_buf_print
	mrs	x8, ttbr0_el3
	mrs	x9, esr_el3
	mrs	x10, far_el3
	bl	str_in_crash_buf_print

	/* Print the non el3 sys registers */
	adr	x6, non_el3_sys_regs
	mrs	x8, spsr_el1
	mrs	x9, elr_el1
	mrs	x10, spsr_abt
	mrs	x11, spsr_und
	mrs	x12, spsr_irq
	mrs	x13, spsr_fiq
	mrs	x14, sctlr_el1
	mrs	x15, actlr_el1
	bl	str_in_crash_buf_print
	mrs	x8, cpacr_el1
	mrs	x9, csselr_el1
	mrs	x10, sp_el1
	mrs	x11, esr_el1
	mrs	x12, ttbr0_el1
	mrs	x13, ttbr1_el1
	mrs	x14, mair_el1
	mrs	x15, amair_el1
	bl	str_in_crash_buf_print
	mrs	x8, tcr_el1
	mrs	x9, tpidr_el1
	mrs	x10, tpidr_el0
	mrs	x11, tpidrro_el0
	mrs	x12, dacr32_el2
	mrs	x13, ifsr32_el2
	mrs	x14, par_el1
	mrs	x15, mpidr_el1
	bl	str_in_crash_buf_print
	mrs	x8, afsr0_el1
	mrs	x9, afsr1_el1
	mrs	x10, contextidr_el1
	mrs	x11, vbar_el1
	mrs	x12, cntp_ctl_el0
	mrs	x13, cntp_cval_el0
	mrs	x14, cntv_ctl_el0
	mrs	x15, cntv_cval_el0
	bl	str_in_crash_buf_print
	mrs	x8, cntkctl_el1
	mrs	x9, sp_el0
	mrs	x10, isr_el1
	bl	str_in_crash_buf_print

	/* Get the cpu specific registers to report */
	bl	do_cpu_reg_dump
	bl	str_in_crash_buf_print

	/* Print some platform registers */
	plat_crash_print_regs

#if defined(MTK_PLAT_PORTING_LAYER)
	/* Print the interconnect registers */
	plat_print_interconnect_regs

	/* Since we have dumped all context, we can crash GP registers */
	mrs x9, sp_el0
	print_dump_stack
#ifdef MTK_ATF_RAM_DUMP
	/* Full ram dump */
	ram_dump
#endif
#endif
	bl	plat_crash_console_flush

	/* Done reporting */
	no_ret	plat_panic_handler
endfunc do_crash_reporting

#else	/* CRASH_REPORTING */
func report_unhandled_exception
report_unhandled_interrupt:
	no_ret	plat_panic_handler
endfunc report_unhandled_exception
#endif	/* CRASH_REPORTING */


func crash_panic
	no_ret	plat_panic_handler
endfunc crash_panic

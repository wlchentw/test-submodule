/*
 * Copyright (c) 2014-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <debug.h>
#if defined(MTK_PLAT_PORTING_LAYER)
#include <log.h>

	.globl	asm_print_dec
#endif

	.globl	asm_print_str
	.globl	asm_print_hex
	.globl	asm_assert
	.globl	do_panic

/* Since the max decimal input number is 65536 */
#define MAX_DEC_DIVISOR		10000
/* The offset to add to get ascii for numerals '0 - 9' */
#define ASCII_OFFSET_NUM	0x30

#if ENABLE_ASSERTIONS
.section .rodata.assert_str, "aS"
assert_msg1:
	.asciz "ASSERT: File "
assert_msg2:
	.asciz " Line "

	/*
	 * This macro is intended to be used to print the
	 * line number in decimal. Used by asm_assert macro.
	 * The max number expected is 65536.
	 * In: x4 = the decimal to print.
	 * Clobber: x30, x0, x1, x2, x5, x6
	 */
	.macro asm_print_line_dec
	mov	x6, #10		/* Divide by 10 after every loop iteration */
	mov	x5, #MAX_DEC_DIVISOR
dec_print_loop:
	udiv	x0, x4, x5		/* Get the quotient */
	msub	x4, x0, x5, x4		/* Find the remainder */
	add	x0, x0, #ASCII_OFFSET_NUM		/* Convert to ascii */
	bl	plat_crash_console_putc
	udiv	x5, x5, x6		/* Reduce divisor */
	cbnz	x5, dec_print_loop
	.endm


/* ---------------------------------------------------------------------------
 * Assertion support in assembly.
 * The below function helps to support assertions in assembly where we do not
 * have a C runtime stack. Arguments to the function are :
 * x0 - File name
 * x1 - Line no
 * Clobber list : x30, x0, x1, x2, x3, x4, x5, x6.
 * ---------------------------------------------------------------------------
 */
func asm_assert
#if LOG_LEVEL >= LOG_LEVEL_INFO
	/*
	 * Only print the output if LOG_LEVEL is higher or equal to
	 * LOG_LEVEL_INFO, which is the default value for builds with DEBUG=1.
	 */
	mov	x5, x0
	mov	x6, x1
	/* Ensure the console is initialized */
	bl	plat_crash_console_init
	/* Check if the console is initialized */
	cbz	x0, _assert_loop
	/* The console is initialized */
	adr	x4, assert_msg1
	bl	asm_print_str
	mov	x4, x5
	bl	asm_print_str
	adr	x4, assert_msg2
	bl	asm_print_str
	/* Check if line number higher than max permitted */
	tst	x6, #~0xffff
	b.ne	_assert_loop
	mov	x4, x6
	asm_print_line_dec
	bl	plat_crash_console_flush
_assert_loop:
#endif /* LOG_LEVEL >= LOG_LEVEL_INFO */
#if defined(MTK_PLAT_PORTING_LAYER)
	/*
	 * Check teearg->atf_log_buf_size.
	 * Exit if atf_log_buf_size equals 0
	 */
	adr	x2, ptr_atf_crash_flag
	ldr	x2, [x2]
	mov_imm x1, ATF_BAD_PTR
	cmp	w2, w1
	beq	2f
	/*
	 * set atf crash magic number
	 */
1:
	adr	x2, ptr_atf_crash_flag
	ldr	x2, [x2]
	mov_imm x1, ATF_CRASH_MAGIC_NO
	str	w1, [x2]	/* p_atf_log_ctrl->atf_crash_flag = 0xdead1abf */

	ldr	w2, [x2]	/* can't use w3 return addr, w4, start of buffer addr */
	cmp	w2, w1
	b.ne	1b
2:
#endif

	no_ret	plat_panic_handler
endfunc asm_assert
#endif /* ENABLE_ASSERTIONS */

/*
 * This function prints a string from address in x4.
 * In: x4 = pointer to string.
 * Clobber: x30, x0, x1, x2, x3
 */
func asm_print_str
	mov	x3, x30
1:
	ldrb	w0, [x4], #0x1
	cbz	x0, 2f
	bl	plat_crash_console_putc
	b	1b
2:
	ret	x3
endfunc asm_print_str

/*
 * This function prints a hexadecimal number in x4.
 * In: x4 = the hexadecimal to print.
 * Clobber: x30, x0 - x3, x5
 */
func asm_print_hex
	mov	x3, x30
	mov	x5, #64  /* No of bits to convert to ascii */
1:
	sub	x5, x5, #4
	lsrv	x0, x4, x5
	and	x0, x0, #0xf
	cmp	x0, #0xA
	b.lo	2f
	/* Add by 0x27 in addition to ASCII_OFFSET_NUM
	 * to get ascii for characters 'a - f'.
	 */
	add	x0, x0, #0x27
2:
	add	x0, x0, #ASCII_OFFSET_NUM
	bl	plat_crash_console_putc
	cbnz	x5, 1b
	ret	x3
endfunc asm_print_hex

#if defined(MTK_PLAT_PORTING_LAYER)
func asm_print_dec
	mov	x3, x30
	/* This timestamp can shows boot up time
	 * aounrd 115 days.
	 */
	ldr x5, =100000000
	mov x1, #10
3:
	udiv x5, x5, x1
	udiv	x0, x4, x5
	cbz	x0, 3b
1:
	add	x0, x0, #ASCII_OFFSET_NUM
	bl	plat_crash_console_putc
	udiv	x0, x4, x5
	msub	x4, x0, x5, x4
	mov x1, #10
	udiv x5, x5, x1
	cbz	x5, 2f
	udiv	x0, x4, x5
	b 1b
2:
	ret	x3
endfunc asm_print_dec
#endif
	/***********************************************************
	 * The common implementation of do_panic for all BL stages
	 ***********************************************************/

.section .rodata.panic_str, "aS"
	panic_msg: .asciz "PANIC at PC : 0x"

/* ---------------------------------------------------------------------------
 * do_panic assumes that it is invoked from a C Runtime Environment ie a
 * valid stack exists. This call will not return.
 * Clobber list : if CRASH_REPORTING is not enabled then x30, x0 - x6
 * ---------------------------------------------------------------------------
 */

/* This is for the non el3 BL stages to compile through */
	.weak el3_panic

func do_panic
#if CRASH_REPORTING
	str	x0, [sp, #-0x10]!
	mrs	x0, currentel
	ubfx	x0, x0, #2, #2
	cmp	x0, #0x3
	ldr	x0, [sp], #0x10
	b.eq	el3_panic
#endif

panic_common:
/*
 * el3_panic will be redefined by the BL31
 * crash reporting mechanism (if enabled)
 */
el3_panic:
	mov	x6, x30
	bl	plat_crash_console_init
	/* Check if the console is initialized */
	cbz	x0, _panic_handler
	/* The console is initialized */
	adr	x4, panic_msg
	bl	asm_print_str
	mov	x4, x6
	/* The panic location is lr -4 */
	sub	x4, x4, #4
	bl	asm_print_hex

	bl	plat_crash_console_flush

_panic_handler:
	/* Pass to plat_panic_handler the address from where el3_panic was
	 * called, not the address of the call from el3_panic. */
	mov	x30, x6
	b	plat_panic_handler
endfunc do_panic

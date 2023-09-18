// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2020, MediaTek inc.
 */

#include <asm.S>

#if defined(__thumb2__)
.macro MODE
	.thumb
	.thumb_func
.endm
#elif defined(__thumb__)
#define MODE .thumb_func
#else
#define MODE .code 32
#endif

/*
 * This assembly source is used both in kernel and userland
 * hence define unwind resources that match both environments.
 */
#if defined(CFG_UNWIND)
#define LOCAL_UNWIND(...)	__VA_ARGS__
#else
#define LOCAL_UNWIND(...)
#endif

/*
 * void __aeabi_memset(void *dest, size_t n, int c) { memset(dest, c, n); }
 */
MODE
FUNC __aeabi_memset , :
LOCAL_UNWIND(.fnstart)
	push	{r1, r2}
	pop	{r2, r1}
	b	memset
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memset

/*
 * void __aeabi_memset4(void *dest, size_t n, int c) { memset(dest, c, n); }
 */
MODE
FUNC __aeabi_memset4 , :
LOCAL_UNWIND(.fnstart)
	b	__aeabi_memset
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memset4

/*
 * void __aeabi_memset8(void *dest, size_t n, int c) { memset(dest, c, n); }
 */
MODE
FUNC __aeabi_memset8 , :
LOCAL_UNWIND(.fnstart)
	b	__aeabi_memset
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memset8

/*
 * void __aeabi_memclr(void *dest, size_t n) { __aeabi_memset(dest, n, 0); }
 */
MODE
FUNC __aeabi_memclr , :
LOCAL_UNWIND(.fnstart)
	push	{r2, lr}
	mov	r2, #0
	bl	__aeabi_memset
	pop	{r2, pc}
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memclr

/*
 * void __aeabi_memclr4(void *dest, size_t n) { __aeabi_memset(dest, n, 0); }
 */
MODE
FUNC __aeabi_memclr4 , :
LOCAL_UNWIND(.fnstart)
	b	__aeabi_memclr
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memclr4

/*
 * void __aeabi_memclr8(void *dest, size_t n) { __aeabi_memset(dest, n, 0); }
 */
MODE
FUNC __aeabi_memclr8 , :
LOCAL_UNWIND(.fnstart)
	b	__aeabi_memclr
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memclr8

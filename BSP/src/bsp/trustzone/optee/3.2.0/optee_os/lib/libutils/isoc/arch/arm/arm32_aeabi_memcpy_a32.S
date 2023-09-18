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
 * void __aeabi_memcpy(void *dest, void *src, size_t n) { memcpy(dest, src, n); }
 */
MODE
FUNC __aeabi_memcpy , :
LOCAL_UNWIND(.fnstart)
	b	memcpy
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memcpy

/*
 * void __aeabi_memcpy4(void *dest, void *src, size_t n) { memcpy(dest, src, n); }
 */
MODE
FUNC __aeabi_memcpy4 , :
LOCAL_UNWIND(.fnstart)
	b	memcpy
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memcpy4

/*
 * void __aeabi_memcpy8(void *dest, void *src, size_t n) { memcpy(dest, src, n); }
 */
MODE
FUNC __aeabi_memcpy8 , :
LOCAL_UNWIND(.fnstart)
	b	memcpy
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memcpy8

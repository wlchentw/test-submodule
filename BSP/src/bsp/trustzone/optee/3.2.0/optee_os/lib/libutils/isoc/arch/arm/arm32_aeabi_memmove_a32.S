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
 * void __aeabi_memmove(void *dest, void *src, size_t n) { memmove(dest, src, n); }
 */
MODE
FUNC __aeabi_memmove , :
LOCAL_UNWIND(.fnstart)
	b	memmove
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memmove

/*
 * void __aeabi_memmove4(void *dest, void *src, size_t n) { memmove(dest, src, n); }
 */
MODE
FUNC __aeabi_memmove4 , :
LOCAL_UNWIND(.fnstart)
	b	memmove
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memmove4

/*
 * void __aeabi_memmove8(void *dest, void *src, size_t n) { memmove(dest, src, n); }
 */
MODE
FUNC __aeabi_memmove8 , :
LOCAL_UNWIND(.fnstart)
	b	memmove
LOCAL_UNWIND(.fnend)
END_FUNC __aeabi_memmove8

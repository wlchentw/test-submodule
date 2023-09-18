/*
 * Copyright (c) 2017 MediaTek Inc.
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
#pragma once

/* System related head file*/
#include <platform/mt_irq.h>
#include <platform/interrupts.h>
#include <platform/mt_reg_base.h>
#include <platform/timer.h>
#include <platform/mtk_timer.h>
//#include <platform/mt8516.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
//#include <kernel/vm.h>
#include <arch/ops.h>
#include <sys/types.h>
#include <platform.h>
#include <reg.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <assert.h>
#include <kernel/vm.h>
#ifdef MTK_GPT_SCHEME_SUPPORT
#include <partition.h>
#endif
#include <debug.h>

#define MT8512_NFI

/* Error codes */
#ifndef EIO
#define EIO         5   /* I/O error */
#define ENOMEM      12  /* Out of memory */
#define EFAULT      14  /* Bad address */
#define EBUSY       16  /* Device or resource busy */
#define EINVAL      22  /* Invalid argument */
#define ENOSPC      28  /* No space left on device */
#define EBADMSG     77  /* Trying to read unreadable message */
#define ETIMEDOUT   110 /* Connection timed out */
#endif

/* Data types define */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifndef loff_t
typedef u64 loff_t;
#endif
#ifndef status_t
typedef int status_t;
#endif
#ifndef bool
typedef char bool;
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef false
#define false 0
#define true 1
#endif

#ifndef BUG_ON
#define BUG_ON(cond)    assert(!(cond))
#endif

/* Common define */
#define NAND_BIT(nr)        (1UL << (nr))
#define NAND_GENMASK(h, l)  (((~0UL) << (l)) & (~0UL >> ((sizeof(unsigned long) * 8) - 1 - (h))))
#define DIV_ROUND_UP(n,d)   (((n) + (d) - 1) / (d))

#define ALIGN(S,A) ((S + A) & ~(A))

#define max(a, b)       (a > b ? a : b)
#define min(a, b)       (a > b ? b : a)
//#define clamp(val, lo, hi)  min((typeof(val))max(val, lo), hi)

#if 0//ndef containerof
#define containerof(ptr, type, member) \
    ((type *)((unsigned long)(ptr) - __builtin_offsetof(type, member)))
#endif

#define MTK_NAND_TIMEOUT        (500000)

#define KB(x)           ((x) * 1024UL)
#define MB(x)           (KB(x) * 1024UL)

/*
 * wait until cond gets true or timeout.
 *
 * cond : C expression to wait
 * timeout : usecs
 *
 * Returns:
 * 0 : if cond = false after timeout elapsed.
 * 1 : if cond = true after timeout elapsed,
 * or the remain usecs if cond = true before timeout elapsed.
 */
#define nand_time size_t

/* Mutex/lock related */
#define nand_lock_t mutex_t

/* completion related */
#define nand_completion_t   event_t

static inline nand_time nand_current_time(void)
{
	return current_time();
}

static inline void *nand_memalign(size_t boundary, size_t size)
{
	return memalign(boundary, size);
}

/*
 * allocate memory and memset zero, see calloc
 * @nmemb:  Number of element to allocate
 * @size:   Size of each element
 */
static inline void *nand_malloc(size_t size)
{
	return calloc(1, size);
}

/*
 * Free a buffer allocated by os_calloc
 * @buf:  Buffer to free. os_free will just return if it is NULL.
 */
static inline void nand_free(void *buf)
{
	free(buf);
}

/* see memcpy */
static inline void *nand_memcpy(void *dest, const void *src, u64 n)
{
	return memcpy(dest, src, n);
}

/* see strncmp */
static inline int nand_strncmp(char const *cs, char const *ct, size_t count)
{
	return strncmp(cs, ct, count);
}

static inline int nand_memcmp(const void *cs, const void *ct, size_t count)
{
	return memcmp(cs, ct, count);
}

/* see memset */
static inline void *nand_memset(void *s, int c, u64 n)
{
	return memset(s, c, n);
}

/* Abort the system. Should only be used when debug. */
static inline void nand_abort(char *s)
{
	panic("Nand abort: %s\n", s);
}

static inline void nand_lock_init(nand_lock_t *m)
{
	//mutex_init(m);
}

static inline status_t nand_lock(nand_lock_t *m)
{
	return 0;//mutex_acquire(m);
}

static inline status_t nand_unlock(nand_lock_t *m)
{
	return 0;//mutex_release(m);
}

static inline void nand_init_completion(nand_completion_t *x)
{
	event_init(x, false, EVENT_FLAG_AUTOUNSIGNAL);
}

static inline void nand_complete(nand_completion_t *x)
{
	event_signal(x, false);
}

static inline status_t nand_wait_for_completion_timeout(nand_completion_t *x, nand_time t)
{
	return event_wait_timeout(x, t);
}

static inline u32 nand_kvaddr_to_paddr(const u8 *buf)
{
	u32 addr;

#ifdef WITH_KERNEL_VM
	addr = (u32)kvaddr_to_paddr(buf);
#else
	addr = (u32)(unsigned long)buf;
#endif

	return addr;
}

static inline u32 nand_dma_map(const u8 *buf, size_t len, bool flag, void *arg)
{
	if (flag)
		arch_clean_cache_range((addr_t)buf, (size_t)len);
	else
		arch_clean_invalidate_cache_range((addr_t)buf, (size_t)len);
	return 0;
}

static inline void nand_dma_unmap(const u8 *buf, size_t len, bool flag, void *arg)
{
	if (flag)
		arch_clean_cache_range((addr_t)buf, (size_t)len);
	else
		arch_clean_invalidate_cache_range((addr_t)buf, (size_t)len);
}

#define check_with_timeout(cond, timeout)  \
({                                         \
    nand_time __ret;                       \
    if (cond) {                            \
         __ret = timeout;                  \
    } else {                               \
        nand_time __end = nand_current_time() + timeout;       \
        for (;;) {                                             \
            nand_time __now = nand_current_time();             \
            if (cond) {                                        \
                __ret = (__end > __now) ? (__end - __now) : 1; \
                break;            \
            }                     \
            if (__end <= __now) { \
                __ret = 0;        \
                break;            \
            }                     \
        }                         \
    }                             \
    __ret;                        \
})

#define mtk_nand_udelay(a)  udelay(a)
#define mtk_nand_mdelay(a)  mdelay(a)

/* Nand print info related */
#define NAND_DEBUG_FLAG 0

#if NAND_DEBUG_FLAG
#define nand_debug(fmt, ...) dprintf(CRITICAL, "NAND debug::%s %d: " fmt "\n",\
    __func__, __LINE__,  ##__VA_ARGS__)

#define nand_info(fmt, ...) dprintf(CRITICAL, "NAND info::%s %d: " fmt "\n",\
	__func__, __LINE__,  ##__VA_ARGS__)
#else
#define nand_debug(fmt, ...)    do {} while (0)
#define nand_info(fmt, ...)    do {} while (0)
#endif

/* Nand error messages */
#define nand_err(fmt, ...) dprintf(CRITICAL, "NAND error::%s %d: " fmt "\n",\
	__func__, __LINE__,  ##__VA_ARGS__)

/* Nand register RW function re-define */
#define nand_readb(a)   (*(volatile u8 * const)(a))
#define nand_readw(a)   (*(volatile u16 * const)(a))
#define nand_readl(a)   (*(volatile u32 * const)(a))

#define nand_writeb(v, a)   (*(volatile u8 * const)(a)) = (v)
#define nand_writew(v, a)  (*(volatile u16 * const)(a)) = (v)
#define nand_writel(v, a)   (*(volatile u32 * const)(a)) = (v)

/* Nand Base register define */
#define NAND_NFI_BASE       NFI_BASE
#define NAND_NFIECC_BASE    NFIECC_BASE
//#define NAND_DRAM_BASE_VIRT DRAM_BASE_VIRT
//#define NAND_NFI_IRQ_BIT_ID NFI_IRQ_BIT_ID
//#define NAND_NFIECC_IRQ_BIT_ID  NFIECC_IRQ_BIT_ID

#define NAND_IRQ_NONE       INT_NO_RESCHEDULE
#define NAND_IRQ_HANDLED    INT_RESCHEDULE

/* reserve 1M dram buffer for system memory issue */
/* unsigned char g_data_buf[16384+2048]; */
#define NAND_DRAM_BUF_DATABUF_ADDR  (NAND_BUF_ADDR)
#define NAND_DRAM_BUF_DATABUF_SIZE  (4096+256)
#define NAND_DRAM_BUF_NFCBUF_ADDR   (NAND_DRAM_BUF_DATABUF_ADDR + NAND_DRAM_BUF_DATABUF_SIZE)
#define NAND_DRAM_BUF_NFCBUF_SIZE   (4096+256)
#define NAND_DRAM_BUF_ECCDE_ADDR    (NAND_DRAM_BUF_NFCBUF_ADDR + NAND_DRAM_BUF_NFCBUF_SIZE)
#define NAND_DRAM_BUF_ECCDE_SIZE    (4096+256)
#define NAND_DRAM_BUF_ECCEN_ADDR    (NAND_DRAM_BUF_ECCDE_ADDR + NAND_DRAM_BUF_ECCDE_SIZE)
#define NAND_DRAM_BUF_ECCEN_SIZE    (4096+256)
#define NAND_DRAM_BUF_BAD_MAP_ADDR  (NAND_DRAM_BUF_ECCEN_ADDR + NAND_DRAM_BUF_ECCEN_SIZE)
#define NAND_DRAM_BUF_BAD_MAP_SIZE  (16384)

/* Nand EFUSE register define */
#define EFUSE_RANDOM_CFG    ((volatile u32 *)(IO_PHYS+0x9020))
#define EFUSE_RANDOM_ENABLE 0x00001000

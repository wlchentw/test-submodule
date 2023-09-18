/*
 * Copyright (C) 2017 MediaTek Inc.
 * Licensed under either
 *     BSD Licence, (see NOTICE for more details)
 *     GNU General Public License, version 2.0, (see NOTICE for more details)
 */

#ifndef __NANDX_OS_H__
#define __NANDX_OS_H__

#include <arch/ops.h>
#include <debug.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <reg.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/mt_reg_base.h>
#include <platform/timer.h>
#include <platform/mtk_timer.h>

#define pr_err(fmt, ...) \
	dprintf(CRITICAL, "[ERR]" fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
	dprintf(ALWAYS, "[WARN]" fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...) \
	dprintf(INFO, fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...) \
	do {} while (0)

#define NANDX_LOG(fmt, ...) dprintf(CRITICAL, fmt, ##__VA_ARGS__)

#define NANDX_ASSERT    assert

#define NANDX_BULK_IO_USE_DRAM  1
#define NANDX_NFI_BUF_ADDR      NAND_BUF_ADDR
#define NANDX_NFI_BUF_ADDR_LEN  (4096 + 256)
#define NANDX_CORE_BUF_ADDR     (NANDX_NFI_BUF_ADDR + NANDX_NFI_BUF_ADDR_LEN)
#define NANDX_CORE_BUF_LEN      (2 * (4096 + 256))
#define NANDX_BBT_BUF_ADDR      (NANDX_CORE_BUF_ADDR + NANDX_CORE_BUF_LEN)
#define NANDX_BBT_BUF_LEN       (8192)
#define NANDX_BBT_MAN_BUF_ADDR  (NANDX_BBT_BUF_ADDR + NANDX_BBT_BUF_LEN)
#define NANDX_BBT_MAN_BUF_LEN   (8192)
#define NANDX_UT_SRC_ADDR       (NANDX_BBT_MAN_BUF_ADDR + NANDX_BBT_MAN_BUF_LEN)
#define NANDX_UT_SRC_LEN        0x41000
#define NANDX_UT_DST_ADDR       (NANDX_UT_SRC_ADDR + NANDX_UT_SRC_LEN)
#define NANDX_UT_DST_LEN        0x41000

#define nandx_udelay(x)         udelay(x)

static inline void *mem_alloc(u32 count, u32 size)
{
	return calloc(count, size);
}

static inline void mem_free(void *mem)
{
	if (mem)
		free(mem);
}

#define nandx_irq_register(dev, irq, irq_handler, name, data)        (0)
#define nandx_event_create()     NULL
#define nandx_event_destroy(event)
#define nandx_event_complete(event)
#define nandx_event_init(event)
#define nandx_event_wait_complete(event, timeout)        true

static inline u64 get_current_time_us(void)
{
	return current_time();
}

static inline u32 nandx_dma_map(void *dev, void *buf, u64 len,
				enum nand_dma_operation op)
{
	u32 addr;

#if WITH_KERNEL_VM
	addr = (u32)kvaddr_to_paddr(buf);
#else
	addr = (u32)buf;
#endif

	if (op == NDMA_FROM_DEV)
		arch_clean_cache_range((addr_t)buf, (size_t)len);
	else
		arch_clean_invalidate_cache_range((addr_t)buf, (size_t)len);

	return addr;
}

static inline void nandx_dma_unmap(void *dev, void *buf, void *addr,
				   u64 len,
				   enum nand_dma_operation op)
{
	if (op == NDMA_FROM_DEV)
		arch_clean_cache_range((addr_t)buf, len);
	else
		arch_clean_invalidate_cache_range((addr_t)buf, len);
}

#define container_of    containerof

#endif /* __NANDX_OS_H__ */

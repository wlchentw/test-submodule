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

#include <compiler.h>
#include <stddef.h>
#include <stdint.h>

__BEGIN_CDECLS

/* mempool type */
enum {
    MEMPOOL_CACHE = 0,
    MEMPOOL_UNCACHE,

    MAX_MEMPOOL_TYPE,
    MEMPOOL_ANY /* a pseudo type to indicate any mempool */
};

/* init mempool of type
 *
 * We will always return cache line aligned memory address to caller,
 * so the start address of each mempool should be aligned to a cache line.
 */
int mempool_init(void *mem, size_t size, uint32_t type);

/* alloc mem of type from mempool
 *
 * returned memory with following characteristics,
 * - start address aligned to a cache line
 * - size round up to multiple of CACHE_LINE
 */
void *mempool_alloc(size_t size, uint32_t type);

/* free mem back to mempool */
void mempool_free(void *mem);

#if LK_DEBUGLEVEL > 1

/* clear mempool, used for test code */
void mempool_clear(void);

#endif

__END_CDECLS

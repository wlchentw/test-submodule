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
#include <unittest.h>
#include <lib/mempool.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

struct mempool_range {
    void *start;
    size_t size;
    uint32_t type;
};

struct mempool_range test_mempool_range[] = {
    { (void *)0x40000000, 0x20000, MEMPOOL_CACHE },
    { (void *)0x40020000, 0x20000, MEMPOOL_UNCACHE },
    { (void *)0x60000000, 0x20000, MEMPOOL_UNCACHE },
    { (void *)0x60020000, 0x20000, MEMPOOL_CACHE }
};

#define NUM_OF_TEST_MEM_RANGE   (sizeof(test_mempool_range)/sizeof(struct mempool_range))

static int test_init_mempool(uint32_t type)
{
    uint32_t i;
    uint32_t s_type, e_type;
    int ret;
    struct mempool_range *m;

    mempool_clear();

    ret = 0;
    for (i = 0; i < NUM_OF_TEST_MEM_RANGE; i++) {
        m = &test_mempool_range[i];
        if ((type != MEMPOOL_ANY) && (m->type != type))
            continue;

        ret = mempool_init(m->start, m->size, m->type);
        if (ret != NO_ERROR)
            return ret;
    }

    return NO_ERROR;
}

static bool test_mempool_init(void)
{
    BEGIN_TEST;

    int ret;

    mempool_clear();

    /* input arguments check */
    ret = mempool_init(NULL, 0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_INVALID_ARGS, ret, "mempool_init should fail with mem:NULL");
    ret = mempool_init((void *)0x40000000, 0, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_INVALID_ARGS, ret, "mempool_init should fail with size:0");
    ret = mempool_init((void *)0x40000000, 0x20000, MAX_MEMPOOL_TYPE);
    EXPECT_EQ(ERR_INVALID_ARGS, ret,
              "mempool_init should fail with invalid type");
    ret = mempool_init((void *)0x40000003, 0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_INVALID_ARGS, ret,
              "mempool_init should fail with non cache line aligned address");

    EXPECT_EQ(NO_ERROR, test_init_mempool(MEMPOOL_ANY), "fail to init mempool");

    /* memory pool region overlap check */
    ret = mempool_init((void *)0x40000000, 0x1000, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in pool");
    ret = mempool_init((void *)0x40001000, 0x1000, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in pool");
    ret = mempool_init((void *)0x40000000, 0x1000, MEMPOOL_UNCACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in other pool");
    ret = mempool_init((void *)0x60000000, 0x1000, MEMPOOL_UNCACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in pool");
    ret = mempool_init((void *)0x60008000, 0x20000, MEMPOOL_UNCACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in pool");
    ret = mempool_init((void *)0x60001000, 0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(ERR_ALREADY_EXISTS, ret,
              "mempool_init should fail when memory exists in other pool");

    /* add new memory to pools */
    ret = mempool_init((void *)0x40040000, 0x30000, MEMPOOL_CACHE);
    EXPECT_EQ(NO_ERROR, ret,
              "fail to init memory of MEMPOOL_CACHE with new memory range");
    ret = mempool_init((void *)0x40080000, 0x40000, MEMPOOL_UNCACHE);
    EXPECT_EQ(NO_ERROR, ret,
              "fail to init memory of MEMPOOL_UNCACHE with new memory range");

    END_TEST;
}

static bool test_mempool_alloc_free(void)
{
    BEGIN_TEST;

    int i;
    uintptr_t mem;

    EXPECT_EQ(NO_ERROR, test_init_mempool(MEMPOOL_ANY), "fail to init mempool");

    /* input argument check */
    mem = (uintptr_t)mempool_alloc(0, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem, "mempool_alloc should fail when size = 0");
    mem = (uintptr_t)mempool_alloc(0x1000, MAX_MEMPOOL_TYPE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail with invalid mempool type");

    /* alloc fail test */
    mem = (uintptr_t)mempool_alloc(0x40000, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when not enough memory");
    mem = (uintptr_t)mempool_alloc(0x30000, MEMPOOL_UNCACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when not enough memory");

    /* free test */
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40000000UL, mem, "fail to alloc from cache pool");
    mempool_free((void *)mem);
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40000000UL, mem,
              "fail to alloc size:0x20000 after free from cache pool");

    /* alloc from 2nd mempool */
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_CACHE);
    EXPECT_EQ(0x60020000UL, mem, "fail to alloc from 2nd cache pool");
    mempool_free((void *)0x40000000UL);
    mempool_free((void *)0x60020000UL);

    /* round up size to multiple cache line alloc test */
    mempool_alloc(0x3, MEMPOOL_CACHE);
    mempool_alloc(0x8, MEMPOOL_CACHE);
    mempool_alloc(0x11, MEMPOOL_CACHE);
    mempool_alloc(0x21, MEMPOOL_CACHE);
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
    i = ROUNDUP(0x3, CACHE_LINE) +
        ROUNDUP(0x8, CACHE_LINE) +
        ROUNDUP(0x11, CACHE_LINE) +
        ROUNDUP(0x21, CACHE_LINE);
    EXPECT_EQ(0x40000000UL + i, mem,
              "fail to round up alloc size from cache pool");

    test_init_mempool(MEMPOOL_ANY);

    mem = (uintptr_t)mempool_alloc(0xffff, MEMPOOL_CACHE);
    EXPECT_EQ(0x40000000UL, mem, "fail to alloc size: 0xffff from cache pool");
    mem = (uintptr_t)mempool_alloc(0x10000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40010000UL, mem,
              "fail to alloc size: 0x10000 after round up size alloc");
    mem = (uintptr_t)mempool_alloc(0x1ffff, MEMPOOL_CACHE);
    EXPECT_EQ(0x60020000UL, mem, "fail to alloc from 2nd cache pool");
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no enoguh memory");

    /* alloc test from uncache pool */
    mem = (uintptr_t)mempool_alloc(0x10000, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x40020000UL, mem, "fail to alloc from uncache pool");
    mem = (uintptr_t)mempool_alloc(0x1ffff, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x60000000UL, mem, "fail to alloc from uncache pool");
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x40030000UL, mem, "fail to alloc from uncache pool");

    /* free all previously allocated mem */
    mempool_free((void *)0x40000000UL);
    mempool_free((void *)0x40010000UL);
    mempool_free((void *)0x60020000UL);
    mempool_free((void *)0x40020000UL);
    mempool_free((void *)0x40030000UL);
    mempool_free((void *)0x60000000UL);

    /* interleaved alloc and free test */
    for (i = 0; i < 0x20000; i += 0x1000) {
        mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
        EXPECT_EQ(0x40000000UL + i, mem,
                  "fail to continuously alloc from 1st cache pool");
    }

    for (i = 0; i < 0x20000; i += 0x1000) {
        mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
        EXPECT_EQ(0x60020000UL + i, mem,
                  "fail to continuously alloc from 1st cache pool");
    }

    for (i = 0; i < 0x20000; i += 0x1000) {
        mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_UNCACHE);
        EXPECT_EQ(0x40020000UL + i, mem,
                  "fail to continuously alloc frmo 1st uncache pool");
    }

    for (i = 0; i < 0x20000; i += 0x1000) {
        mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_UNCACHE);
        EXPECT_EQ(0x60000000UL + i, mem,
                  "fail to continuously alloc frmo 1st uncache pool");
    }

    mempool_free((void *)0x40000000);
    mempool_free((void *)0x40002000);
    mempool_free((void *)0x4001f000);
    mempool_free((void *)0x40020000);
    mempool_free((void *)0x40022000);
    mempool_free((void *)0x4003f000);
    mempool_free((void *)0x4003e000);

    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no enough memory");
    mempool_free((void *)0x40001000);
    mem = (uintptr_t)mempool_alloc(0x3000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40000000, mem,
              "fail to alloc size:0x3000 from 1st cache pool after free");
    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no enough memory");
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
    EXPECT_EQ(0x4001f000, mem,
              "fail to alloc size:0x1000 at the end of 1st cache pool");

    mem = (uintptr_t)mempool_alloc(0x4000, MEMPOOL_UNCACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no enough memory");
    mempool_free((void *)0x4003d000);
    mempool_free((void *)0x4003c000);
    mem = (uintptr_t)mempool_alloc(0x4000, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x4003c000, mem,
              "fail to alloc size:0x4000 at the end of 1st uncache pool");
    mempool_free((void *)0x40021000);
    mem = (uintptr_t)mempool_alloc(0x4000, MEMPOOL_UNCACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no enough memory");
    mempool_free((void *)0x40023000);
    mem = (uintptr_t)mempool_alloc(0x4000, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x40020000, mem,
              "fail to alloc size:0x4000 at the begin of 1st uncache pool");

    END_TEST;
}

static bool test_mempool_alloc_any(void)
{
    BEGIN_TEST;

    uintptr_t mem;

    /* init mempool with both cached and uncached mempool */
    EXPECT_EQ(NO_ERROR, test_init_mempool(MEMPOOL_ANY), "fail to init mempool");

    mem = (uintptr_t)mempool_alloc(0x4000, MEMPOOL_ANY);
    EXPECT_EQ(0x40000000, mem,
              "fail to alloc from cache mem for MEMPOOL_ANY");
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_ANY);
    EXPECT_EQ(0x60020000, mem,
              "fail to alloc from uncached mem for MEMPOOL_ANY");
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_ANY);
    EXPECT_EQ(0x40004000, mem,
              "fail to alloc from cached mem for MEMPOOL_ANY");
    mem = (uintptr_t)mempool_alloc(0x1000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40005000, mem,
              "fail to alloc from cached mem for MEMPOOL_CACHE");
    mem = (uintptr_t)mempool_alloc(0x1f000, MEMPOOL_ANY);
    EXPECT_EQ(0x40020000, mem,
              "fail to alloc from uncached mem for MEMPOOL_ANY");

    /* init only cached mempool */
    EXPECT_EQ(NO_ERROR, test_init_mempool(MEMPOOL_CACHE),
              "fail to init mempool");

    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_ANY);
    EXPECT_EQ(0x40000000, mem,
              "fail to alloc from cached mem for MEMPOOL_ANY");
    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_UNCACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no uncached mempool");
    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_CACHE);
    EXPECT_EQ(0x40002000, mem,
              "fail to alloc from cached mem for MEMPOOL_CACHE");
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_ANY);
    EXPECT_EQ(0x60020000, mem,
              "fail to alloc from cached mem for MEMPOOL_CACHE");

    /* init only uncached mempool */
    EXPECT_EQ(NO_ERROR, test_init_mempool(MEMPOOL_UNCACHE),
              "fail to init mempool");

    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_ANY);
    EXPECT_EQ(0x40020000, mem,
              "fail to alloc from uncached mem for MEMPOOL_ANY");
    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_CACHE);
    EXPECT_EQ((uintptr_t)NULL, mem,
              "mempool_alloc should fail when no cached mempool");
    mem = (uintptr_t)mempool_alloc(0x2000, MEMPOOL_UNCACHE);
    EXPECT_EQ(0x40022000, mem,
              "fail to alloc from uncached mem for MEMPOOL_UNCACHE");
    mem = (uintptr_t)mempool_alloc(0x20000, MEMPOOL_ANY);
    EXPECT_EQ(0x60000000, mem,
              "fail to alloc from uncached mem for MEMPOOL_ANY");

    END_TEST;
}

BEGIN_TEST_CASE(mempool_tests);
RUN_TEST(test_mempool_init);
RUN_TEST(test_mempool_alloc_free);
RUN_TEST(test_mempool_alloc_any);
END_TEST_CASE(mempool_tests);

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

#include <assert.h>
#include <err.h>
#include <kernel/mutex.h>
#include <lib/console.h>
#include <lib/mempool.h>
#include <list.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>

/* [TODO] lib mempool is not thread safe for now, extra work needed */

#define LOCAL_TRACE 0

struct mem_chunk {
    struct list_node node;
    void *start;
    size_t len;
    bool free;
};

struct mempool {
    struct list_node chunk_list;
    void *start;
    size_t len;
};

static struct mempool pool[MAX_MEMPOOL_TYPE] = {
    { LIST_INITIAL_VALUE(pool[0].chunk_list), NULL, 0 },
    { LIST_INITIAL_VALUE(pool[1].chunk_list), NULL, 0 }
};

int mempool_init(void *mem, size_t size, uint32_t type)
{
    int i;
    struct mem_chunk *chunk;

    LTRACEF("pool init %p, size %zx, type %d\n", mem, size, type);

    /* check input arg, mem address should be aligned to a cache line */
    if (!mem || !IS_ALIGNED(mem, CACHE_LINE) ||
            !size || (type >= MAX_MEMPOOL_TYPE))
        return ERR_INVALID_ARGS;

    /* check if the mem address already inited */
    for (i = 0; i < MAX_MEMPOOL_TYPE; i++) {
        list_for_every_entry(&pool[i].chunk_list, chunk,
                             struct mem_chunk, node) {
            if (chunk->start <= mem && ((chunk->start + chunk->len) > mem))
                return ERR_ALREADY_EXISTS;
        }
    }

    chunk = (struct mem_chunk *)malloc(sizeof(struct mem_chunk));
    if (!chunk)
        return ERR_NO_MEMORY;

    chunk->start = mem;
    chunk->len = size;
    chunk->free = true;
    list_add_tail(&pool[type].chunk_list, &chunk->node);

    return NO_ERROR;
}

void *mempool_alloc(size_t size, uint32_t type)
{
    bool found;
    uint32_t i, s_type, e_type;
    size_t alloc_size;
    struct mem_chunk *chunk, *new_chunk;

    LTRACEF("pool alloc size %zx, type %d\n", size, type);

    if (!size ||
            ((type >= MAX_MEMPOOL_TYPE) && (type != MEMPOOL_ANY)))
        return NULL;

    alloc_size = ROUNDUP(size, CACHE_LINE);
    found = false;

    s_type = e_type = type;
    if (type == MEMPOOL_ANY) {
        s_type = 0;
        e_type = MAX_MEMPOOL_TYPE - 1;
    }
    for (i = s_type; i <= e_type; i++) {
        list_for_every_entry(&pool[i].chunk_list, chunk,
                             struct mem_chunk, node) {
            if (chunk->len < alloc_size || !chunk->free)
                continue;

            found = true;
            break;
        }

        if (found)
            break;
    }

    new_chunk = NULL;
    if (found) {
        /* if the chunk len happend to equal to alloc size, just return it */
        if (chunk->len == alloc_size) {
            chunk->free = false;
            new_chunk = chunk;
        } else {
            new_chunk = (struct mem_chunk *)malloc(sizeof(struct mem_chunk));
            if (new_chunk) {
                new_chunk->start = chunk->start;
                new_chunk->len = alloc_size;
                new_chunk->free = false;
                chunk->start = chunk->start + alloc_size;
                chunk->len -= alloc_size;
                list_add_before(&chunk->node, &new_chunk->node);
            }
        }
    }

    return new_chunk ? new_chunk->start : NULL;
}

void mempool_free(void *ptr)
{
    int i;
    bool found;
    struct mem_chunk *chunk, *prev, *next;

    LTRACEF("pool free %p\n", ptr);

    if (NULL == ptr)
        return;

    /* walk through list to find matched chunk */
    found = false;
    for (i = 0; i < MAX_MEMPOOL_TYPE; i++) {
        list_for_every_entry(&pool[i].chunk_list, chunk,
                             struct mem_chunk, node) {
            if (!chunk->free && (chunk->start == ptr)) {
                found = true;
                break;
            }
        }

        if (found) {
            /* merge with adjecent chunk if possible */
            prev = list_prev_type(&pool[i].chunk_list, &chunk->node,
                                  struct mem_chunk, node);
            if (prev && prev->free && ((prev->start + prev->len) == ptr)) {
                chunk->len += prev->len;
                chunk->start = prev->start;
                list_delete(&prev->node);
                free(prev);
                prev = NULL;
            }

            next = list_next_type(&pool[i].chunk_list, &chunk->node,
                                  struct mem_chunk, node);
            if (next && next->free &&
                    ((chunk->start + chunk->len) == next->start)) {
                chunk->len += next->len;
                list_delete(&next->node);
                free(next);
                next = NULL;
            }

            chunk->free = true;

            break;
        }
    }
}

void mempool_clear(void)
{
    int i;
    struct mem_chunk *chunk;
    struct mem_chunk *temp;

    /* delete every node in the list */
    for (i = 0; i < MAX_MEMPOOL_TYPE; i++) {
        list_for_every_entry_safe(&pool[i].chunk_list, chunk, temp,
                                  struct mem_chunk, node) {
            list_delete(&chunk->node);
            free(chunk);
            chunk = NULL;
        }
        pool[i].start = NULL;
        pool[i].len = 0;
    }
}

#if LK_DEBUGLEVEL > 1

#include <lib/console.h>

static int cmd_mempool(int argc, const cmd_args *argv);
static void show_usage(const char *cmd);

STATIC_COMMAND_START
STATIC_COMMAND("mempool", "mempool debug commands", &cmd_mempool)
STATIC_COMMAND_END(mempool);

static void show_usage(const char *cmd)
{
    printf("usage:\n");
    printf("\t%s init <address> <size> <type>\n", cmd);
    printf("\t%s info\n", cmd);
    printf("\t%s alloc <size> <type>\n", cmd);
    printf("\t%s free <address>\n", cmd);
}

static void mempool_dump(void)
{
    int i;
    struct mem_chunk *chunk;

    for (i = 0; i < MAX_MEMPOOL_TYPE; i++) {
        printf("dump mempool type %d\n", i);
        list_for_every_entry(&pool[i].chunk_list, chunk,
                             struct mem_chunk, node) {
            printf("start %p, len %zx, free %d, type %d\n",
                   chunk->start, chunk->len, chunk->free, i);
        }
    }
}

static int cmd_mempool(int argc, const cmd_args *argv)
{
    int ret;
    void *p;

    if (argc < 2) {
notenoughargs:
        printf("not enough arguments\n");
usage:
        show_usage(argv[0].str);
        return -1;
    }

    if (strcmp(argv[1].str, "init") == 0) {
        if (argc < 5)
            goto notenoughargs;

        ret = mempool_init((void *)argv[2].u, argv[3].u, argv[4].i);
        if (ret != NO_ERROR)
            printf("mempool_init failed, ret %d\n", ret);
    } else if (strcmp(argv[1].str, "info") == 0) {
        mempool_dump();
    } else if (strcmp(argv[1].str, "alloc") == 0) {
        if (argc < 4)
            goto notenoughargs;

        p = mempool_alloc(argv[2].u, argv[3].i);
        if (!p) {
            printf("mempool alloc failed, size %lu, type %ld\n",
                   argv[2].u, argv[3].i);
            mempool_dump();
        }
    } else if (strcmp(argv[1].str, "free") == 0) {
        if (argc < 3)
            goto notenoughargs;

        mempool_free((void *)(uintptr_t)argv[2].u);
    } else {
        printf("unrecognized command\n");
        goto usage;
    }

    return 0;
}
#endif

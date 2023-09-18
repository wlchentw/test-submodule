/*
 * Copyright (c) 2018 MediaTek Inc.
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
#include <debug.h>
#include <err.h>
#include <errno.h>
#include <lib/nftl.h>
#include <sys/types.h>

int nftl_block_isbad(struct nftl_info *info, u32 page)
{
    if (page >= info->total_size / info->write_size)
        return ERR_INVALID_ARGS;

    return info->block_isbad(info, page);
}

int nftl_block_markbad(struct nftl_info *info, u32 page)
{
    if (page >= info->total_size / info->write_size)
        return ERR_INVALID_ARGS;

    return info->block_markbad(info, page);
}

int nftl_block_mapping(struct nftl_info *info, u32 *page)
{
    u32 page_per_block = info->erase_size / info->write_size;
    u32 count = info->total_size / info->write_size;
    u32 i, block;
    int ret = 0;

    if (*page >= info->total_size / info->write_size)
        return ERR_INVALID_ARGS;

    block = *page / page_per_block + 1;

    for (i = 0; i < count; i += page_per_block) {
        if (nftl_block_isbad(info, i))
            continue;

        if (--block == 0)
            break;
    }

    if (block == 0)
        *page = i + *page % page_per_block;
    else
        ret = ERR_OUT_OF_RANGE;

    return ret;
}

ssize_t nftl_erase(struct nftl_info *info, off_t offset, ssize_t len)
{
    if (offset < 0 || len < 0 || (u64)offset >= info->total_size
        || (u64)len > info->total_size - offset)
        return ERR_INVALID_ARGS;

    return info->erase(info, offset, len);
}

ssize_t nftl_read(struct nftl_info *info, void *buf, off_t offset, ssize_t len)
{
    if (offset < 0 || len < 0 || (u64)offset >= info->total_size
        || (u64)len > info->total_size - offset)
        return ERR_INVALID_ARGS;

    return info->read(info, buf, offset, len);
}

ssize_t nftl_write(struct nftl_info *info, const void *buf, off_t offset,
                   ssize_t len)
{
    if (offset < 0 || len < 0 || (u64)offset >= info->total_size
        || (u64)len > info->total_size - offset)
        return ERR_INVALID_ARGS;

    return info->write(info, buf, offset, len);
}

int nftl_ioctl(struct nftl_info *info, int request, void *argp)
{
    return info->ioctl(info, request, argp);
}


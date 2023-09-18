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
#pragma once

#include <list.h>
#include <sys/types.h>

struct nftl_info {
    char *name;
    char *label;
    u64 total_size;
    u32 erase_size;
    u32 write_size;

    int (*block_isbad)(struct nftl_info *info, u32 page);
    int (*block_markbad)(struct nftl_info *info, u32 page);
    ssize_t (*erase)(struct nftl_info *info, off_t offset, ssize_t len);
    ssize_t (*read)(struct nftl_info *info, void *buf, off_t offset,
                    ssize_t len);
    ssize_t (*write)(struct nftl_info *info, const void *buf, off_t offset,
                     ssize_t len);
    int (*ioctl)(struct nftl_info *info, int request, void *argp);
};

int nftl_mount_bdev(struct nftl_info *info);
struct nftl_info *nftl_search_by_address(u64 address, u64 *start);
struct nftl_info *nftl_open(const char *name);
void nftl_close(struct nftl_info *info);
int nftl_add_part(const char *main_part, const char *sub_part,
                  const char *sub_label, u64 offset, u64 len);
struct nftl_info *nftl_add_master(const char *name);
int nftl_delete_part(const char *name);
void nftl_dump_parts(void);
int nftl_block_mapping(struct nftl_info *info, u32 *page);
int nftl_block_isbad(struct nftl_info *info, u32 page);
int nftl_block_markbad(struct nftl_info *info, u32 page);
ssize_t nftl_erase(struct nftl_info *info, off_t offset, ssize_t len);
ssize_t nftl_read(struct nftl_info *info, void *buf, off_t offset, ssize_t len);
ssize_t nftl_write(struct nftl_info *info, const void *buf, off_t offset,
                   ssize_t len);
int nftl_ioctl(struct nftl_info *info, int request, void *argp);
int nftl_partition_get_offset(const char *name);
int nftl_partition_get_size(const char *name);

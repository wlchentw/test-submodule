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
 
#include "../slc.h"
#include "../slc_os.h"

void mtk_nand_chip_test(struct mtk_nand_chip *chip)
{
	struct mtk_nand_ops ops;
	u8 *buf_w, *buf_r;
	int total_block, i, j;
	int ret = 0;

	total_block = chip->totalsize/chip->blocksize;
	nand_info("mtk_nand_unit_test start total_block: %d", total_block);

	buf_w = nand_malloc(chip->pagesize * 2);
	if (buf_w == NULL) {
		nand_err("malloc buf_w failed: %d \n", chip->pagesize);
		return;
	}

	buf_r = nand_malloc(chip->pagesize * 2);
	if (buf_r == NULL) {
		nand_err("malloc buf_r failed: %d \n", chip->pagesize);
		return;
	}

	for (i = 0; i < chip->pagesize*2; i++)
		buf_w[i] = i;

	for (i = 0; i < 10; i++) {
	//for (i = total_block-1; ; i--) {

		if (mtk_nand_block_isbad(chip, i*chip->page_per_block)) {
			nand_info("check bad blk: %d", i);
			//continue;
		}

		nand_info("test blk: %d", i);
		nand_memset(&ops, 0, sizeof(ops));
		ops.mode = NAND_OPS_ERASE_POLL;
		ops.offset = (u64)(i * chip->blocksize);
		ops.len = chip->blocksize;

		ret = mtk_nand_erase(chip, &ops);
		if (ret) {
			nand_err("Erase failed at blk: %d", i);
			continue;
		}

		for (j = i*chip->page_per_block;
		        j < i*chip->page_per_block+ chip->page_per_block; j++) {

			nand_memset(&ops, 0, sizeof(ops));
			ops.mode = NAND_OPS_ECC_DMA_POLL;
			ops.offset = (u64)(j * chip->pagesize);
			ops.len = (u64)chip->pagesize;
			ops.datbuf = buf_w;

			ret = mtk_nand_write(chip, &ops);
			if (ret) {
				nand_err("Write failed at blk:%d, page:%d", i, j);
				break;
			}

			nand_memset(&ops, 0, sizeof(ops));
			nand_memset(buf_r, 0x5A, chip->pagesize);
			ops.mode = NAND_OPS_ECC_DMA_POLL;
			ops.offset = (u64)(j * chip->pagesize);
			ops.len = (u64)chip->pagesize;
			ops.datbuf = buf_r;

			ret = mtk_nand_read(chip, &ops);
			if (ret) {
				nand_err("Read failed at blk:%d page:%d", i, j);
				break;
			}

			/* compare the read buf and write buf */
			if (nand_memcmp(buf_r, buf_w, chip->pagesize)) {
				nand_err("compare failed! addr:0x%x, buf_r:0x%x, %x, %x, %x, %x buf_w:0x%x, %x, %x, %x, %x", 
				(int)ops.offset, buf_r[0], buf_r[1], buf_r[2], buf_r[3], buf_r[4]
				, buf_w[0], buf_w[1], buf_w[2], buf_w[3], buf_w[4]);
			}
		}

		nand_memset(&ops, 0, sizeof(ops));
		ops.mode = NAND_OPS_ERASE_POLL;
		ops.offset = (u64)(i * chip->blocksize);
		ops.len = chip->blocksize;			
		ret = mtk_nand_erase(chip, &ops);
		if (ret) {
			nand_err("Erase failed at blk: %d", i);
			continue;
		}
		//if(i == 0)
		//	break;
	}

	nand_info("mtk_nand_chip_test start end");

	if (buf_r != NULL)
		nand_free(buf_r);
	if (buf_w != NULL)
		nand_free(buf_w);

}


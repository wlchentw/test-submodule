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
#include "slc.h"
#include "nfi/nfi.h"
#include "bbt/bbt.h"
#include "test/slc_test.h"

static inline int nand_ffs(unsigned int x)
{
	return __builtin_ffs(x);
}

#define NOTALIGNED(x)   ((x & (chip->subpagesize - 1)) != 0)

u64 g_nand_size = 0;

static int mtk_nand_do_read_ops(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops);

static int mtk_nand_is_dram_buf(u8* buf)
{
	//return (buf < NAND_DRAM_BASE_VIRT) ? 0 : 1;
	return 1;
}

static int mtk_nand_get_controller(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	nand_lock(&nfc->lock);

	return 0;
}

static int mtk_nand_release_controller(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	nand_unlock(&nfc->lock);

	return 0;
}

static int mtk_nand_wait_func(struct mtk_nand_chip *chip, int polling)
{
	int status;
	unsigned long timeo = 1000000;

	chip->cmdfunc(chip, NAND_CMD_STATUS, -1, -1);

	if (!polling) {
		if (chip->wait_busy_irq(chip))
			nand_err("nand dev ready timeout");
	} else {
		if (!check_with_timeout(chip->dev_ready(chip), timeo))
			nand_err("nand dev ready timeout");
	}

	status = (int)chip->read_byte(chip);

	return status;
}

void mtk_nand_wait_ready(struct mtk_nand_chip *chip)
{
	unsigned long timeo = 1000000;

	if (!check_with_timeout(chip->dev_ready(chip), timeo))
		nand_err("nand dev ready timeout");

}


static int mtk_nand_check_wp(struct mtk_nand_chip *chip)
{
	/* Check the WP bit */
	chip->cmdfunc(chip, NAND_CMD_STATUS, -1, -1);
	return (chip->read_byte(chip) & NAND_STATUS_WP) ? 0 : 1;
}

static int mtk_nand_block_bad(struct mtk_nand_chip *chip, u64 ofs)
{
	int page, res = 0, i = 0;
	u16 bad;

	if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
		ofs += chip->blocksize - chip->pagesize;

	page = (int)(ofs / chip->pagesize) % chip->page_per_chip;

	do {
		chip->cmdfunc(chip, NAND_CMD_READOOB, chip->badblockpos, page);
		bad = chip->read_byte(chip);

		if (chip->badblockbits == 8)
			res = bad != 0xFF;

		ofs += chip->pagesize;
		page = (int)(ofs / chip->pagesize) % chip->page_per_chip;
		i++;
	} while (!res && i < 2 && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE));

	return res;
}

static int mtk_nand_block_checkbad(struct mtk_nand_chip *chip, u32 page)
{
	struct mtk_nand_ops ops;
	int ret = 0;

	/* block align */
	page = page / chip->page_per_block * chip->page_per_block;

	if (chip->bbt) {
		ret = mtk_nand_isbad_bbt(chip, page / chip->page_per_block, 1);
		return ret;
	}
#if 0
	/* be careful of randomizer on case, may get a wrong feedback by chip->read_byte */
	if (!(chip->options & NAND_NEED_SCRAMBLING)) {
		ret = chip->block_bad(chip, (u64)page * chip->pagesize);
	} else 
#endif
	{
		nand_memset(&ops, 0, sizeof(ops));
		ops.mode = NAND_OPS_ECC_DMA_POLL;
		ops.offset = (u64)page * chip->pagesize;
		ops.len = chip->pagesize;
		ops.datbuf = chip->databuf;
		mtk_nand_do_read_ops(chip, &ops);
		ret = (chip->oob_poi[chip->badblockpos] != 0xFF);
	}

	return ret;
}

int mtk_nand_block_isbad(struct mtk_nand_chip *nand, u32 page)
{
	int ret = 0;

	if (!nand->bbt) {
		mtk_nand_get_controller(nand);
		ret = mtk_nand_block_checkbad(nand, page);
		mtk_nand_release_controller(nand);
	} else {
		ret = mtk_nand_isbad_bbt(nand, page/nand->page_per_block, 1);
	}

	return ret;
}

int nand_reset(struct mtk_nand_chip *chip, int chipnr)
{
	/* power on sequence delay */
	mtk_nand_udelay(300);

	/*
	 * The CS line has to be released before we can apply the new NAND
	 * interface settings, hence this weird ->select_chip() dance.
	 */
	chip->select_chip(chip, chipnr);
	chip->cmdfunc(chip, NAND_CMD_RESET, -1, -1);
	chip->select_chip(chip, -1);

	return 0;
}

static inline int mtk_nand_opcode_8bits(unsigned int command)
{
	switch (command) {
		case NAND_CMD_READID:
		case NAND_CMD_PARAM:
		case NAND_CMD_GET_FEATURES:
		case NAND_CMD_SET_FEATURES:
			return 1;
		default:
			break;
	}
	return 0;
}

static void mtk_nand_command_lp(struct mtk_nand_chip *chip, unsigned int command,
                                int column, int page_addr)
{
	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		column += chip->pagesize;
		command = NAND_CMD_READ0;
	}

	/* Command latch cycle */
	chip->cmd_ctrl(chip, command, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);

	if (column != -1 || page_addr != -1) {
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;

		/* Serially input address */
		if (column != -1) {
			chip->cmd_ctrl(chip, column, ctrl);
			ctrl &= ~NAND_CTRL_CHANGE;

			/* Only output a single addr cycle for 8bits opcodes. */
			if (!mtk_nand_opcode_8bits(command))
				chip->cmd_ctrl(chip, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(chip, page_addr, ctrl);
			chip->cmd_ctrl(chip, page_addr >> 8,
			               NAND_NCE | NAND_ALE);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(chip, page_addr >> 16,
				               NAND_NCE | NAND_ALE);
		}
	}
	chip->cmd_ctrl(chip, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * Program and erase have their own busy handlers status, sequential
	 * in and status need no delay.
	 */
	switch (command) {
		case NAND_CMD_CACHEDPROG:
		case NAND_CMD_PAGEPROG:
		case NAND_CMD_ERASE1:
		case NAND_CMD_ERASE2:
		case NAND_CMD_SEQIN:
		case NAND_CMD_STATUS:
			return;

		case NAND_CMD_RNDOUT:
			/* No ready / busy check necessary */
			chip->cmd_ctrl(chip, NAND_CMD_RNDOUTSTART,
			               NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			chip->cmd_ctrl(chip, NAND_CMD_NONE,
			               NAND_NCE | NAND_CTRL_CHANGE);
			return;

		case NAND_CMD_READ0:
			chip->cmd_ctrl(chip, NAND_CMD_READSTART,
			               NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
			chip->cmd_ctrl(chip, NAND_CMD_NONE,
			               NAND_NCE | NAND_CTRL_CHANGE);

		/* This applies to read commands */
		default:
			break;
	}

	//nand_debug("command:0x%x column:0x%x page_addr:0x%x", command, column, page_addr);

	mtk_nand_wait_ready(chip);

	//nand_debug("command:0x%x column:0x%x page_addr:0x%x", command, column, page_addr);
	
}

int mtk_nand_block_markbad(struct mtk_nand_chip *chip, u32 page)
{
	int ret = 0;

	if (mtk_nand_block_isbad(chip, (u64)(page*chip->pagesize))) {
		return 0;
	} else {
		/* Mark block bad in BBT */
		if (chip->bbt) {
			ret = mtk_nand_markbad_bbt(chip, (u64)(page*chip->pagesize));
		}
	}

	return ret;
}

void lk_nand_irq_handler(unsigned int irq)
{
	/* no need irq handler for lk, we use polling */
	return;
}

static void mtk_nand_set_defaults(struct mtk_nand_chip *chip)
{
	/* chip_delay setup set 20us if not */
	chip->chip_delay = 20;

	/* command function*/
	chip->cmdfunc = mtk_nand_command_lp;

	/* wait function */
	chip->waitfunc = mtk_nand_wait_func;

	/* bad block check */
	chip->block_bad = mtk_nand_block_isbad;
	/* bad block mark */
	chip->block_markbad = mtk_nand_block_markbad;

	/* scan bbt, disable bbt here */
	/* chip->scan_bbt = mtk_nand_default_bbt; */

	/* variable defalut value */
	chip->badblockbits = 8;
	chip->badblockpos = 0;

	chip->activechip = -1;

	/* scan bbt */
	if (chip->options & NAND_BBT_SUPPORT) {
		chip->scan_bbt = mtk_nand_default_bbt;
		/* BBT options setting, must align for all drivers */
		chip->bbt_options |= (NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB);
	}
}

int mtk_nand_flash_get(struct mtk_nand_chip *chip, int maxchips)
{
	u32 i;
	u8 id_data[8];
	struct mtk_nand_flash_dev *type = nand_flash_devs;

	nand_reset(chip, 0);

	/* Select the device */
	chip->select_chip(chip, 0);

	/* Send the command for reading device ID */
	chip->cmdfunc(chip, NAND_CMD_READID, 0x00, -1);

	/* Read entire ID string */
	for (i = 0; i < 8; i++) {
		id_data[i] = chip->read_byte(chip);
	}
	nand_info("nand id: %x %x %x %x %x %x",
	          id_data[0], id_data[1], id_data[2],id_data[3], id_data[4],  id_data[5]);


	for (; type->name != NULL; type++) {
		if (!nand_strncmp(type->id, id_data, type->id_len)) {
			nand_info("nand found [%s]", type->name);
			break;
		}
	}

	chip->select_chip(chip, -1);
	if (!type->name) {
		return -ENODEV;
	}

	chip->numchips = 1;

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		/* See comment in nand_get_flash_type for reset */
		nand_reset(chip, i);

		chip->select_chip(chip, i);
		/* Send the command for reading device ID */
		chip->cmdfunc(chip, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */
		if (id_data[0] != chip->read_byte(chip) ||
		        id_data[1] != chip->read_byte(chip)) {
			chip->select_chip(chip, -1);
			break;
		}
		nand_info("chip %d is found", i);
		chip->select_chip(chip, -1);
		chip->numchips++;
	}

	/* set nand chip parameters */
	chip->pagesize = type->pagesize;
	chip->oobsize = type->oobsize;
	chip->bits_per_cell = type->bits_per_cell;
	/* KB to B */
	chip->chipsize = ((u64)type->chipsize) << 10;
	chip->blocksize = type->erasesize;
	chip->bbt_erase_shift = nand_ffs(type->erasesize) - 1;
	chip->bbt_options |= type->bbt_options;
	chip->options |= type->options;
	chip->ecc_size = type->ecc_size;
	chip->ecc_strength = type->ecc_strength;
	chip->fdm_ecc_size = type->fdmeccsize;

	chip->totalsize = i * chip->chipsize;

	chip->acctiming = type->acctiming;

	nand_info("chip acctiming %x should equal type->acctiming %x\n",
		chip->acctiming, type->acctiming);

	chip->ecc_steps = chip->pagesize / chip->ecc_size;
	if (nand_is_slc(chip)) {
		if (chip->ecc_steps == 2)
			chip->subpagesize = chip->pagesize / 2;
		else if (chip->ecc_steps > 2)
			chip->subpagesize = chip->pagesize / 4;
		else
			chip->subpagesize = chip->pagesize;
	}
	chip->page_per_block = chip->blocksize / chip->pagesize;
	chip->page_per_chip = chip->chipsize / chip->pagesize;

	chip->lbasize = chip->pagesize;
	/* change lbacnt if want to reserve blocks */
	chip->lbacnt = chip->totalsize / chip->lbasize;

	chip->databuf = (u8 *)nand_memalign(4, chip->pagesize + chip->oobsize);
	if (!chip->databuf)
		return -ENOMEM;
	chip->oob_poi = chip->databuf + chip->pagesize;

	mtk_nand_set_bbt_options(chip, id_data[0]);

	nand_info("pagesize:%d, oobsize:%d, blocksize:0x%x totalsize:0x%x",
		chip->pagesize, chip->oobsize, chip->blocksize, chip->totalsize);

	return 0;
}

int mtk_nand_scan(struct mtk_nand_chip *chip, int maxchips)
{
	int ret;

	/* Set the defaults */
	mtk_nand_set_defaults(chip);

	ret = mtk_nand_flash_get(chip, maxchips);
	if (ret) {
		nand_err("no nand device found");
		return ret;
	}

	/* ret = chip->scan_bbt(chip); */

	return ret;
}

static int mtk_nand_fill_ecc_oob()
{
	return 0;
}

static int mtk_nand_fill_free_oob()
{
	return 0;
}

static int mtk_nand_transfer_ecc_oob()
{
	return 0;
}

static int mtk_nand_transfer_free_oob()
{
	return 0;
}

static int mtk_nand_do_read_ops(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	int chipnr, page, realpage, col, bytes, aligned;
	u8 *buf, *oob_ecc, *oob_free, *bufpoi;
	u64 readlen = ops->len, from = ops->offset;
	u32 max_bitflips = 0;
	u32 ecc_failures = chip->stats.failed;
	int ret = 0, ecc_fail = 0;

	chipnr = (int)(from / chip->chipsize);
	chip->select_chip(chip, chipnr);

	realpage = (int)(from / chip->pagesize);
	page = realpage % chip->page_per_chip;

	col = (int)(from & (chip->pagesize - 1));

	buf = ops->datbuf;
	oob_ecc = ops->oobeccbuf;
	oob_free = ops->oobfreebuf;

	nand_debug("realpage:0x%x col:0x%x", realpage, col);

	while (1) {
		bytes = min(chip->pagesize - col, readlen);
		aligned = (bytes == chip->pagesize);
		/* workaround for dma to sram */
		if (!mtk_nand_is_dram_buf(buf))
			aligned = 0;
		bufpoi = aligned ? buf : chip->databuf;

		/* send read page command */
		nand_debug("[nand] read page %d chip %d", page, chipnr);
		#ifndef MT8518_NFI
		mtk_nfc_randomizer_enable(chip, page, RAND_DECODE, 0);
		#endif
		chip->cmdfunc(chip, NAND_CMD_READ0, 0x00, page);

		if (!aligned) {
			if (ops->mode == NAND_OPS_ECC_DMA_IRQ)
				ret = chip->read_subpage_ecc_dma_irq(chip, col, bytes, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_DMA_POLL)
				ret = chip->read_subpage_ecc_dma_polling(chip, col, bytes, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_IRQ)
				ret = chip->read_subpage_ecc_pio_irq(chip, col, bytes, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_POLL)
				ret = chip->read_subpage_ecc_pio_polling(chip, col, bytes, bufpoi, page);
		} else {
			if (ops->mode == NAND_OPS_RAW_DMA_IRQ)
				ret = chip->read_page_raw_dma_irq(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_RAW_DMA_POLL)
				ret = chip->read_page_raw_dma_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_RAW_PIO_IRQ)
				ret = chip->read_page_raw_pio_irq(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_RAW_PIO_POLL)
				ret = chip->read_page_raw_pio_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_DMA_IRQ)
				ret = chip->read_page_ecc_dma_irq(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_DMA_POLL)
				ret = chip->read_page_ecc_dma_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_IRQ)
				ret = chip->read_page_ecc_pio_irq(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_POLL)
				ret = chip->read_page_ecc_pio_polling(chip, bufpoi, page);
		}
		#ifndef MT8518_NFI
		mtk_nfc_randomizer_disable(chip);
		#endif
		if (ret < 0)
			break;

		max_bitflips = max(max_bitflips, ret);

		if (chip->stats.failed - ecc_failures) {
			ecc_fail = 1;
			break;
		}

		if (!aligned)
			nand_memcpy(buf, chip->databuf + col, bytes);
		if (!oob_ecc)
			mtk_nand_transfer_ecc_oob();
		else if (!oob_free)
			mtk_nand_transfer_free_oob();
	
		nand_debug("page:0x%x data[0~7] %2x %2x %2x %2x  %2x %2x %2x %2x", 
			page, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);	
	
		readlen -= bytes;
		buf += bytes;

		if (!readlen)
			break;

		/* For subsequent reads align to page boundary */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage % chip->page_per_chip;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(chip, -1);
			chip->select_chip(chip, chipnr);
		}
	}
	chip->select_chip(chip, -1);

	if (ecc_fail) {
		nand_err("uncorrect error at page:0x%x", page);
		return -EBADMSG;
	}

	return max_bitflips;
}

int mtk_nand_read(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	int ret;

	mtk_nand_get_controller(chip);
	ret = mtk_nand_do_read_ops(chip, ops);
	mtk_nand_release_controller(chip);

	return ret;
}

static int mtk_nand_do_write_ops(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	int chipnr, realpage, page, col, bytes, aligned;
	u32 writelen = ops->len;
	u64 to = ops->offset;
	u8 *buf = ops->datbuf;
	u8 *oob_ecc = ops->oobeccbuf;
	u8 *oob_free = ops->oobfreebuf;
	u8 *bufpoi;
	int ret = 0, status, polling_wait = 1;

	/* Reject writes, which are not subpage aligned */
	if (NOTALIGNED(to) || NOTALIGNED(ops->len)) {
		nand_err("attempt to write non page aligned data (offset 0x%llx, len 0x%llx)", to, ops->len);
		return -EINVAL;
	}

	col = to & (chip->pagesize - 1);
	chipnr = (int)(to / chip->chipsize);
	chip->select_chip(chip, chipnr);

	/* Check, if it is write protected */
	if (mtk_nand_check_wp(chip)) {
		ret = -EIO;
		nand_err("write protected!");
		goto err_out;
	}

	realpage = (int)(to / chip->pagesize);
	page = realpage % chip->page_per_chip;

	while (1) {
		bytes = min(chip->pagesize - col, writelen);
		aligned = (bytes == chip->pagesize);
		/* workaround for dma to sram */
		if (!mtk_nand_is_dram_buf(buf))
			aligned = 0;
		bufpoi = aligned ? buf : chip->databuf;

		if (!aligned) {
			nand_memset(chip->databuf, 0xff, chip->pagesize);
			nand_memcpy(chip->databuf + col, buf, bytes);
		}
		nand_memset(chip->oob_poi, 0xff, chip->oobsize);
		if (!oob_ecc)
			mtk_nand_fill_ecc_oob();
		else if (!oob_free)
			mtk_nand_fill_free_oob();

		/* nand_debug("[nand] write page %d chip %d", page, chipnr); */
		#ifndef MT8518_NFI
		mtk_nfc_randomizer_enable(chip, page, RAND_ENCODE, 0);
		#endif
		chip->cmdfunc(chip, NAND_CMD_SEQIN, 0x00, page);

		if (!aligned) {
			if (ops->mode == NAND_OPS_ECC_DMA_IRQ) {
				polling_wait = 0;
				ret = chip->write_subpage_ecc_dma_irq(chip, col, bytes, bufpoi, page);
			} else if (ops->mode == NAND_OPS_ECC_DMA_POLL)
				ret = chip->write_subpage_ecc_dma_polling(chip, col, bytes, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_IRQ) {
				polling_wait = 0;
				ret = chip->write_subpage_ecc_pio_irq(chip, col, bytes, bufpoi, page);
			} else if (ops->mode == NAND_OPS_ECC_PIO_POLL)
				ret = chip->write_subpage_ecc_pio_polling(chip, col, bytes, bufpoi, page);
		} else {
			if (ops->mode == NAND_OPS_RAW_DMA_IRQ) {
				polling_wait = 0;
				ret = chip->write_page_raw_dma_irq(chip, bufpoi, page);
			} else if (ops->mode == NAND_OPS_RAW_DMA_POLL)
				ret = chip->write_page_raw_dma_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_RAW_PIO_IRQ) {
				polling_wait = 0;
				ret = chip->write_page_raw_pio_irq(chip, bufpoi, page);
			} else if (ops->mode == NAND_OPS_RAW_PIO_POLL)
				ret = chip->write_page_raw_pio_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_DMA_IRQ) {
				polling_wait = 0;
				ret = chip->write_page_ecc_dma_irq(chip, bufpoi, page);
			} else if (ops->mode == NAND_OPS_ECC_DMA_POLL)
				ret = chip->write_page_ecc_dma_polling(chip, bufpoi, page);
			else if (ops->mode == NAND_OPS_ECC_PIO_IRQ) {
				polling_wait = 0;
				ret = chip->write_page_ecc_pio_irq(chip, bufpoi, page);
			} else if (ops->mode == NAND_OPS_ECC_PIO_POLL)
				ret = chip->write_page_ecc_pio_polling(chip, bufpoi, page);
		}
		#ifndef MT8518_NFI
		mtk_nfc_randomizer_disable(chip);
		#endif
		if (ret < 0)
			break;

		chip->cmdfunc(chip, NAND_CMD_PAGEPROG, -1, -1);
		status = chip->waitfunc(chip, polling_wait);
		if (status & NAND_STATUS_FAIL) {
			ret = -EIO;
			nand_err("write failed at page 0x%x status:0x%x", realpage, status);
			goto err_out;
		}

		writelen -= bytes;
		if (!writelen)
			break;

		col = 0;
		buf += bytes;
		realpage++;

		page = realpage % chip->page_per_chip;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(chip, -1);
			chip->select_chip(chip, chipnr);
		}
	}

err_out:
	chip->select_chip(chip, -1);
	if (ret < 0) {
		if (!mtk_nand_block_checkbad(chip, page)) 
			mtk_nand_markbad_bbt(chip, page*chip->pagesize);
	}	

	return ret;
}

int mtk_nand_write(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	int ret;

	mtk_nand_get_controller(chip);
	ret = mtk_nand_do_write_ops(chip, ops);
	mtk_nand_release_controller(chip);

	return ret;
}

static int mtk_nand_do_erase_ops(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	u64 offset = ops->offset;
	u64 eraselen = ops->len;
	int page, status, ret = 0, chipnr, polling_wait = 0;

	if ((offset % chip->blocksize) || (eraselen % chip->blocksize)) {
		nand_err("erase is not aligned (off 0x%llx, len 0x%llx)", offset, eraselen);
		return -EINVAL;
	}

	page = (int)(offset / chip->pagesize);
	chipnr = (int)(offset / chip->chipsize);

	nand_debug("page:0x%x, chipnr:0x%x", page, chipnr);

	chip->select_chip(chip, chipnr);

	/* Check, if it is write protected */
	if (mtk_nand_check_wp(chip)) {
		ret = -EIO;
		nand_err("write protected!");
		goto err_out;
	}

	while (1) {
		if (mtk_nand_block_checkbad(chip, page)) {
			nand_err("attempt to erase bad block at page 0x%x", page);
		}

		nand_debug("[nand] erase page %d chip %d", page, chipnr);
		chip->cmdfunc(chip, NAND_CMD_ERASE1, -1, (page % chip->page_per_chip));
		chip->cmdfunc(chip, NAND_CMD_ERASE2, -1, -1);
		if (ops->mode == NAND_OPS_ERASE_IRQ)
			polling_wait = 0;
		else if (ops->mode == NAND_OPS_ERASE_POLL)
			polling_wait = 1;
		status = chip->waitfunc(chip, polling_wait);

		if (status & NAND_STATUS_FAIL) {
			ret = -EIO;
			nand_err("erase failed at page 0x%x status:0x%x", page, status);
			goto err_out;
		}

		eraselen -= chip->blocksize;
		if (!eraselen)
			break;
		page += chip->page_per_block;

		if (eraselen && !(page % chip->page_per_chip)) {
			chipnr++;
			chip->select_chip(chip, -1);
			chip->select_chip(chip, chipnr);
		}
	}
err_out:
	chip->select_chip(chip, -1);
	if (ret < 0) {
		if (!mtk_nand_block_checkbad(chip, page)) 
			mtk_nand_markbad_bbt(chip, page*chip->pagesize);
	}

	return ret;
}

int mtk_nand_erase(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops)
{
	int ret;

	mtk_nand_get_controller(chip);
	ret = mtk_nand_do_erase_ops(chip, ops);
	mtk_nand_release_controller(chip);

	return ret;
}


int mtk_nand_init(struct mtk_nand_chip **ext_nand)
{
	struct mtk_nand_chip *chip;
	int ret;

	ret = mtk_nfc_nand_chip_init(ext_nand);

	chip = *ext_nand;

#if MTK_NAND_UNIT_TEST
	mtk_nand_chip_test(chip);
#endif
	if (chip->scan_bbt)
		ret = chip->scan_bbt(chip);

	return ret;
}


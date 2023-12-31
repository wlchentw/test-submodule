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

/* Description:
 *
 * When nand_scan_bbt is called, then it tries to find the bad block table
 * depending on the options in the BBT descriptor(s). If no flash based BBT
 * (NAND_BBT_USE_FLASH) is specified then the device is scanned for factory
 * marked good / bad blocks. This information is used to create a memory BBT.
 * Once a new bad block is discovered then the "factory" information is updated
 * on the device.
 * If a flash based BBT is specified then the function first tries to find the
 * BBT on flash. If a BBT is found then the contents are read and the memory
 * based BBT is created. If a mirrored BBT is selected then the mirror is
 * searched too and the versions are compared. If the mirror has a greater
 * version number, then the mirror BBT is used to build the memory based BBT.
 * If the tables are not versioned, then we "or" the bad block information.
 * If one of the BBTs is out of date or does not exist it is (re)created.
 * If no BBT exists at all then the device is scanned for factory marked
 * good / bad blocks and the bad block tables are created.
 *
 * For manufacturer created BBTs like the one found on M-SYS DOC devices
 * the BBT is searched and read but never created
 *
 * The auto generated bad block table is located in the last good blocks
 * of the device. The table is mirrored, so it can be updated eventually.
 * The table is marked in the OOB area with an ident pattern and a version
 * number which indicates which of both tables is more up to date. If the NAND
 * controller needs the complete OOB area for the ECC information then the
 * option NAND_BBT_NO_OOB should be used (along with NAND_BBT_USE_FLASH, of
 * course): it moves the ident pattern and the version byte into the data area
 * and the OOB area will remain untouched.
 *
 * The table uses 2 bits per block
 * 11b:     block is good
 * 00b:     block is factory marked bad
 * 01b, 10b:    block is marked bad due to wear
 *
 * The memory bad block table uses the following scheme:
 * 00b:     block is good
 * 01b:     block is marked bad due to wear
 * 10b:     block is reserved (to protect the bbt area)
 * 11b:     block is factory marked bad
 *
 * Multichip devices like DOC store the bad block info per floor.
 *
 * Following assumptions are made:
 * - bbts start at a page boundary, if autolocated on a block boundary
 * - the space necessary for a bbt in FLASH does not exceed a block boundary
 *
 */
#include "bbt.h"

static int mtk_nand_update_bbt(struct mtk_nand_chip *chip, loff_t offs);

static inline uint8_t bbt_get_entry(struct mtk_nand_chip *chip, int block)
{
	uint8_t entry = chip->bbt[block >> BBT_ENTRY_SHIFT];
	entry >>= (block & BBT_ENTRY_MASK) * 2;
	return entry & BBT_ENTRY_MASK;
}

static inline void bbt_mark_entry(struct mtk_nand_chip *chip, int block,
                                  uint8_t mark)
{
	uint8_t msk = (mark & BBT_ENTRY_MASK) << ((block & BBT_ENTRY_MASK) * 2);
	chip->bbt[block >> BBT_ENTRY_SHIFT] |= msk;
}

static int check_pattern_no_oob(uint8_t *buf, struct mtk_nand_bbt_descr *td)
{
	if (nand_memcmp(buf, td->pattern, td->len))
		return -1;
	return 0;
}

/**
 * check_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @len: the length of buffer to search
 * @paglen: the pagelength
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers.
 */
static int check_pattern(uint8_t *buf, int len, int paglen, struct mtk_nand_bbt_descr *td)
{
	if (td->options & NAND_BBT_NO_OOB)
		return check_pattern_no_oob(buf, td);

	/* Compare the pattern */
	if (nand_memcmp(buf + paglen + td->offs, td->pattern, td->len))
		return -1;

	return 0;
}

/**
 * check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int check_short_pattern(uint8_t *buf, struct mtk_nand_bbt_descr *td)
{
	/* Compare the pattern */
	if (nand_memcmp(buf + td->offs, td->pattern, td->len))
		return -1;
	return 0;
}

/**
 * add_marker_len - compute the length of the marker in data area
 * @td: BBT descriptor used for computation
 *
 * The length will be 0 if the marker is located in OOB area.
 */
static u32 add_marker_len(struct mtk_nand_bbt_descr *td)
{
	u32 len;

	if (!(td->options & NAND_BBT_NO_OOB))
		return 0;

	len = td->len;
	if (td->options & NAND_BBT_VERSION)
		len++;
	return len;
}

/**
 * read_bbt - [GENERIC] Read the bad block table starting from page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @page: the starting page
 * @num: the number of bbt descriptors to read
 * @td: the bbt describtion table
 * @offs: block number offset in the table
 *
 * Read the bad block table starting from page.
 */
static int read_bbt(struct mtk_nand_chip *chip, uint8_t *buf, int page, int num,
                    struct mtk_nand_bbt_descr *td, int offs)
{
	int ret = 0, i, j, act = 0;
	size_t retlen, len, totlen;
	loff_t from;
	int bits = td->options & NAND_BBT_NRBITS_MSK;
	uint8_t msk = (uint8_t)((1 << bits) - 1);
	u32 marker_len;
	int reserved_block_code = td->reserved_block_code;
	struct mtk_nand_ops ops;

	nand_debug("page:%d num:%d", page, num);

	totlen = (num * bits) >> 3;
	marker_len = add_marker_len(td);
	from = ((loff_t)page) * chip->pagesize;

	while (totlen) {
		len = min(totlen, (size_t)(1 << chip->bbt_erase_shift));
		if (marker_len) {
			/*
			 * In case the BBT marker is not in the OOB area it
			 * will be just in the first page.
			 */
			len -= marker_len;
			from += marker_len;
			marker_len = 0;
		}
		nand_memset(&ops, 0, sizeof(ops));
		ops.mode = NAND_OPS_ECC_DMA_POLL;
		ops.offset = (u64)from;
		ops.len = (u64)max(len, chip->pagesize);
		ops.datbuf = buf;

		ret = mtk_nand_read(chip, &ops);
		if (ret < 0) {
			if (ret = -EBADMSG) {
				nand_info("nand_bbt: ECC error in BBT at 0x%012llx",
				          from & ~chip->pagesize);
				return ret;
			} else {
				nand_info("nand_bbt: error reading BBT");
				return ret;
			}
		}

		/* Analyse data */
		for (i = 0; i < len; i++) {
			uint8_t dat = buf[i];
			for (j = 0; j < 8; j += bits, act++) {
				uint8_t tmp = (dat >> j) & msk;
				if (tmp == msk)
					continue;
				if (reserved_block_code && (tmp == reserved_block_code)) {
					nand_info("nand_read_bbt: reserved block at 0x%012llx",
					          (loff_t)(offs + act) /chip->blocksize/* >> chip->bbt_erase_shift */);
					bbt_mark_entry(chip, offs + act,
					               BBT_BLOCK_RESERVED);
					continue;
				}
				/*
				 * Leave it for now, if it's matured we can
				 * move this message to nand_debug.
				 */
				nand_debug("nand_read_bbt: bad block at 0x%x",
				          (loff_t)(offs + act) /chip->blocksize/* >> chip->bbt_erase_shift */);
				/* Factory marked bad or worn out? */
				if (tmp == 0)
					bbt_mark_entry(chip, offs + act,
					               BBT_BLOCK_FACTORY_BAD);
				else
					bbt_mark_entry(chip, offs + act,
					               BBT_BLOCK_WORN);
			}
		}
		totlen -= len;
		from += len;
	}
	return ret;
}

/**
 * read_abs_bbt - [GENERIC] Read the bad block table starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @chip: read the table for a specific chip, -1 read all chips; applies only if
 *        NAND_BBT_PERCHIP option is set
 *
 * Read the bad block table for all chips starting at a given page. We assume
 * that the bbt bits are in consecutive order.
 */
static int read_abs_bbt(struct mtk_nand_chip *this, uint8_t *buf, struct mtk_nand_bbt_descr *td, int chip)
{
	int res = 0, i;

	if (td->options & NAND_BBT_PERCHIP) {
		int offs = 0;
		for (i = 0; i < this->numchips; i++) {
			if (chip == -1 || chip == i)
				res = read_bbt(this, buf, td->pages[i],
				               this->chipsize >> this->bbt_erase_shift,
				               td, offs);
			if (res)
				return res;
			offs += this->chipsize >> this->bbt_erase_shift;
		}
	} else {
		res = read_bbt(this, buf, td->pages[0],
		               this->totalsize >> this->bbt_erase_shift, td, 0);
		if (res)
			return res;
	}
	return 0;
}

/* BBT marker is in the first page, no OOB */
static int scan_read_data(struct mtk_nand_chip *chip, uint8_t *buf, loff_t offs,
                          struct mtk_nand_bbt_descr *td)
{
	size_t retlen;
	size_t len;
	struct mtk_nand_ops ops;

	len = td->len;
	if (td->options & NAND_BBT_VERSION)
		len++;

	nand_debug("len:0x%x, offs:0x%lx", len, offs);

	nand_memset(&ops, 0, sizeof(ops));
	ops.mode = NAND_OPS_ECC_DMA_POLL;
	ops.offset = (u64)offs;
	ops.len = (u64)max(len, chip->pagesize);
	ops.datbuf = buf;

	return mtk_nand_read(chip, &ops);
}

/**
 * scan_read_oob - [GENERIC] Scan data+OOB region to buffer
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @offs: offset at which to scan
 * @len: length of data region to read
 *
 * Scan read data from data+OOB. May traverse multiple pages, interleaving
 * page,OOB,page,OOB,... in buf. Completes transfer and returns the "strongest"
 * ECC condition (error or bitflip). May quit on the first (non-ECC) error.
 */
static int scan_read_oob(struct mtk_nand_chip *chip, uint8_t *buf, loff_t offs,
                         size_t len)
{
	struct mtk_nand_ops ops;

	nand_debug("len:0x%x, offs:0x%lx", len, offs);

	nand_memset(&ops, 0, sizeof(ops));
	ops.mode = NAND_OPS_ECC_DMA_POLL;
	ops.offset = (u64)offs;
	ops.len = (u64)max(len, chip->pagesize);
	ops.datbuf = buf;

	return mtk_nand_read(chip, &ops);
}

static int scan_read(struct mtk_nand_chip *chip, uint8_t *buf, loff_t offs,
                     size_t len, struct mtk_nand_bbt_descr *td)
{
	nand_debug("td->options:0x%x", td->options);
	if (td->options & NAND_BBT_NO_OOB)
		return scan_read_data(chip, buf, offs, td);
	else
		return scan_read_oob(chip, buf, offs, len);
}

/* Scan write data with oob to flash */
static int scan_write_bbt(struct mtk_nand_chip *chip, loff_t offs, size_t len,
                          uint8_t *buf, uint8_t *oob)
{
	struct mtk_nand_ops ops;

	nand_debug("len:0x%x, offs:0x%lx", len, offs);

	nand_memset(&ops, 0, sizeof(ops));
	ops.mode = NAND_OPS_ECC_DMA_POLL;
	ops.offset = offs;
	ops.len = len;
	ops.datbuf = buf;
	ops.oobeccbuf = oob;

	return mtk_nand_write(chip, &ops);
}

static u32 bbt_get_ver_offs(struct mtk_nand_chip *chip, struct mtk_nand_bbt_descr *td)
{
	u32 ver_offs = td->veroffs;

	if (!(td->options & NAND_BBT_NO_OOB))
		ver_offs += chip->pagesize;
	return ver_offs;
}

/**
 * read_abs_bbts - [GENERIC] Read the bad block table(s) for all chips starting at a given page
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Read the bad block table(s) for all chips starting at a given page. We
 * assume that the bbt bits are in consecutive order.
 */
static void read_abs_bbts(struct mtk_nand_chip *this, uint8_t *buf,
                          struct mtk_nand_bbt_descr *td, struct mtk_nand_bbt_descr *md)
{
	/* Read the primary version, if available */
	if (td->options & NAND_BBT_VERSION) {
		scan_read(this, buf, (loff_t)td->pages[0] * this->pagesize,
		          this->pagesize, td);
		td->version[0] = buf[bbt_get_ver_offs(this, td)];
		nand_info("Bad block table at page %d, version 0x%02X",
		          td->pages[0], td->version[0]);
	}

	/* Read the mirror version, if available */
	if (md && (md->options & NAND_BBT_VERSION)) {
		scan_read(this, buf, (loff_t)md->pages[0] * this->pagesize,
		          this->pagesize, md);
		md->version[0] = buf[bbt_get_ver_offs(this, md)];
		nand_info("Bad block table at page %d, version 0x%02X",
		          md->pages[0], md->version[0]);
	}
}

/* Scan a given block partially */
static int scan_block_fast(struct mtk_nand_chip *chip, struct mtk_nand_bbt_descr *bd,
                           loff_t offs, uint8_t *buf, int numpages)
{
	int j, ret;
	struct mtk_nand_ops ops;

	nand_memset(&ops, 0, sizeof(ops));
	ops.mode = NAND_OPS_ECC_DMA_POLL;
	ops.len = (u64)chip->pagesize;
	ops.datbuf = buf;

	nand_debug("numpages:0x%x, offs:%lx", numpages, offs);

	for (j = 0; j < numpages; j++) {
		ops.offset = (u64)offs;

		/*
		 * Read the full oob until read_oob is fixed to handle single
		 * byte reads for 16 bit buswidth.
		 */
		ret = mtk_nand_read(chip, &ops);
		/* Ignore ECC errors when checking for BBM */
		if (ret == -EBADMSG) {
			nand_info("Found ECC at offs:0x%lx", offs);
			return 1;
		}

		//if (check_short_pattern(buf, bd))
		if (check_short_pattern(chip->oob_poi, bd))
			return 1;

		offs += chip->pagesize;
	}
	return 0;
}

/**
 * create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int create_bbt(struct mtk_nand_chip *this, uint8_t *buf,
                      struct mtk_nand_bbt_descr *bd, int chip)
{
	int i, numblocks, numpages;
	int startblock;
	loff_t from;

	nand_info("Scanning device for bad blocks");

	if (bd->options & NAND_BBT_SCAN2NDPAGE)
		numpages = 2;
	else
		numpages = 1;

	if (chip == -1) {
		numblocks = this->totalsize/this->blocksize;
		//startblock = 0; 
		//from = 0;
		startblock = 8;   //from PL/LK address
		from = 0;
	} else {
		if (chip >= this->numchips) {
			nand_err("create_bbt(): chipnr (%d) > available chips (%d)",
			         chip + 1, this->numchips);
			return -EINVAL;
		}
		numblocks = this->totalsize/this->blocksize;
		startblock = chip * numblocks;
		numblocks += startblock;
		from = (loff_t)startblock /this->blocksize/* >> this->bbt_erase_shift */;
	}

	if (this->bbt_options & NAND_BBT_SCANLASTPAGE)
		from += this->blocksize - (this->pagesize * numpages);

	for (i = startblock; i < numblocks; i++) {
		int ret;

		/* BUG_ON(bd->options & NAND_BBT_NO_OOB); */

		ret = scan_block_fast(this, bd, from, buf, numpages);
		if (ret < 0)
			return ret;

		if (ret) {
			bbt_mark_entry(this, i, BBT_BLOCK_FACTORY_BAD);
			nand_err("Bad eraseblock %d at 0x%x",
			         i, (unsigned long)from);
			/* this->ecc_stats.badblocks++; */
		}

		from += this->blocksize;
	}
	return 0;
}

/**
 * search_bbt - [GENERIC] scan the device for a specific bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 *
 * Read the bad block table by searching for a given ident pattern. Search is
 * preformed either from the beginning up or from the end of the device
 * downwards. The search starts always at the start of a block. If the option
 * NAND_BBT_PERCHIP is given, each chip is searched for a bbt, which contains
 * the bad block information of this chip. This is necessary to provide support
 * for certain DOC devices.
 *
 * The bbt ident pattern resides in the oob area of the first page in a block.
 */
static int search_bbt(struct mtk_nand_chip *this, uint8_t *buf, struct mtk_nand_bbt_descr *td)
{
	int i, chips;
	int startblock, block, dir;
	int scanlen = this->pagesize + this->oobsize;
	int bbtblocks;

	nand_debug("td->options:0x%x", td->options);

	/* Search direction top -> down? */
	if (td->options & NAND_BBT_LASTBLOCK) {
		startblock = (this->totalsize >> this->bbt_erase_shift) - 1;
		dir = -1;
	} else {
		startblock = 0;
		dir = 1;
	}

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chips = this->numchips;
		bbtblocks = this->chipsize >> this->bbt_erase_shift;
		startblock &= bbtblocks - 1;
	} else {
		chips = 1;
		bbtblocks = this->totalsize >> this->bbt_erase_shift;
	}

	for (i = 0; i < chips; i++) {
		/* Reset version information */
		td->version[i] = 0;
		td->pages[i] = -1;
		/* Scan the maximum number of blocks */
		for (block = 0; block < td->maxblocks; block++) {

			int actblock = startblock + dir * block;
			loff_t offs = (loff_t)actblock << this->bbt_erase_shift;

			/* Read first page */
			scan_read(this, buf, offs, this->pagesize, td);
			if (!check_pattern(buf, scanlen, this->pagesize, td)) {
				td->pages[i] = actblock * this->page_per_block;
				if (td->options & NAND_BBT_VERSION) {
					offs = bbt_get_ver_offs(this, td);
					td->version[i] = buf[offs];
				}
				break;
			}
		}
		startblock += this->chipsize >> this->bbt_erase_shift;
	}
	/* Check, if we found a bbt for each requested chip */
	for (i = 0; i < chips; i++) {
		if (td->pages[i] == -1)
			nand_err("Bad block table not found for chip %d", i);
		else
			nand_info("Bad block table found at page %d, version 0x%x",
			          td->pages[i], td->version[i]);
	}

	return 0;
}

/**
 * search_read_bbts - [GENERIC] scan the device for bad block table(s)
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 *
 * Search and read the bad block table(s).
 */
static void search_read_bbts(struct mtk_nand_chip *chip, uint8_t *buf,
                             struct mtk_nand_bbt_descr *td,
                             struct mtk_nand_bbt_descr *md)
{
	nand_debug("td->options:0x%x", td->options);

	/* Search the primary table */
	search_bbt(chip, buf, td);

	/* Search the mirror table */
	if (md)
		search_bbt(chip, buf, md);
}

/**
 * write_bbt - [GENERIC] (Re)write the bad block table
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @td: descriptor for the bad block table
 * @md: descriptor for the bad block table mirror
 * @chipsel: selector for a specific chip, -1 for all
 *
 * (Re)write the bad block table.
 */
static int write_bbt(struct mtk_nand_chip *this, uint8_t *buf,
                     struct mtk_nand_bbt_descr *td, struct mtk_nand_bbt_descr *md,
                     int chipsel)
{
	int i, res, chip = 0;
	int bits, startblock, dir, page, offs, numblocks, sft, sftmsk;
	int nrchips, pageoffs, ooboffs;
	uint8_t msk[4];
	uint8_t rcode = td->reserved_block_code;
	size_t retlen, len = 0;
	loff_t to;
	struct mtk_nand_ops ops;

	nand_debug("td->options:0x%x", td->options);

	if (!rcode)
		rcode = 0xff;
	/* Write bad block table per chip rather than per device? */
	if (td->options & NAND_BBT_PERCHIP) {
		numblocks = (int)(this->chipsize /this->blocksize/* >> this->bbt_erase_shift */);
		/* Full device write or specific chip? */
		if (chipsel == -1) {
			nrchips = this->numchips;
		} else {
			nrchips = chipsel + 1;
			chip = chipsel;
		}
	} else {
		numblocks = (int)(this->totalsize /this->blocksize/* >> this->bbt_erase_shift */);
		nrchips = 1;
	}

	/* Loop through the chips */
	for (; chip < nrchips; chip++) {
		/*
		 * There was already a version of the table, reuse the page
		 * This applies for absolute placement too, as we have the
		 * page nr. in td->pages.
		 */
		if (td->pages[chip] != -1) {
			page = td->pages[chip];
			goto write;
		}

		/*
		 * Automatic placement of the bad block table. Search direction
		 * top -> down?
		 */
		if (td->options & NAND_BBT_LASTBLOCK) {
			startblock = numblocks * (chip + 1) - 1;
			dir = -1;
		} else {
			startblock = chip * numblocks;
			dir = 1;
		}

		for (i = 0; i < td->maxblocks; i++) {
			int block = startblock + dir * i;
			/* Check, if the block is bad */
			switch (bbt_get_entry(this, block)) {
				case BBT_BLOCK_WORN:
				case BBT_BLOCK_FACTORY_BAD:
					continue;
			}
			page = block * this->page_per_block;
			/* Check, if the block is used by the mirror table */
			if (!md || md->pages[chip] != page)
				goto write;
		}
		nand_err("No space left to write bad block table");
		return -ENOSPC;
write:

		/* Set up shift count and masks for the flash table */
		bits = td->options & NAND_BBT_NRBITS_MSK;
		msk[2] = ~rcode;
		switch (bits) {
			case 1:
				sft = 3;
				sftmsk = 0x07;
				msk[0] = 0x00;
				msk[1] = 0x01;
				msk[3] = 0x01;
				break;
			case 2:
				sft = 2;
				sftmsk = 0x06;
				msk[0] = 0x00;
				msk[1] = 0x01;
				msk[3] = 0x03;
				break;
			case 4:
				sft = 1;
				sftmsk = 0x04;
				msk[0] = 0x00;
				msk[1] = 0x0C;
				msk[3] = 0x0f;
				break;
			case 8:
				sft = 0;
				sftmsk = 0x00;
				msk[0] = 0x00;
				msk[1] = 0x0F;
				msk[3] = 0xff;
				break;
			default:
				return -EINVAL;
		}

		to = ((loff_t)page) * this->pagesize;

		/* Must we save the block contents? */
		if (td->options & NAND_BBT_SAVECONTENT) {
			/* Make it block aligned */
			to &= ~(this->blocksize - 1);
			len = 1 << this->bbt_erase_shift;

			nand_memset(&ops, 0, sizeof(ops));
			ops.mode = NAND_OPS_ECC_DMA_POLL;
			ops.offset = (u64)to;
			ops.len = (u64)this->blocksize;
			ops.datbuf = buf;

			res = mtk_nand_read(this, &ops);
			if (res < 0) {
				nand_err("nand_bbt: ECC error while reading block for writing bad block table");
			}
			/* Read oob data */
			ops.oobecclen = (len / this->pagesize) * this->oobsize;
			ops.oobeccbuf = &buf[len];
			res = mtk_nand_read(this, &ops);
			if (res < 0)
				goto outerr;

			/* Calc the byte offset in the buffer */
			pageoffs = page - (int)(to / this->pagesize);
			offs = pageoffs * this->pagesize;
			/* Preset the bbt area with 0xff */
			nand_memset(&buf[offs], 0xff, (size_t)(numblocks >> sft));
			ooboffs = len + (pageoffs * this->oobsize);

		} else if (td->options & NAND_BBT_NO_OOB) {
			ooboffs = 0;
			offs = td->len;
			/* The version byte */
			if (td->options & NAND_BBT_VERSION)
				offs++;
			/* Calc length */
			len = (size_t)(numblocks >> sft);
			len += offs;
			/* Make it page aligned! */
			len = ALIGN(len, this->pagesize);
			/* Preset the buffer with 0xff */
			nand_memset(buf, 0xff, len);
			/* Pattern is located at the begin of first page */
			nand_memcpy(buf, td->pattern, td->len);
		} else {
			/* Calc length */
			len = (size_t)(numblocks >> sft);
			/* Make it page aligned! */
			len = ALIGN(len, this->pagesize);
			/* Preset the buffer with 0xff */
			nand_memcpy(buf, 0xff, len +
			            (len / this->pagesize)* this->oobsize);
			offs = 0;
			ooboffs = len;
			/* Pattern is located in oob area of first page */
			nand_memcpy(&buf[ooboffs + td->offs], td->pattern, td->len);
		}

		if (td->options & NAND_BBT_VERSION)
			buf[ooboffs + td->veroffs] = td->version[chip];

		/* Walk through the memory table */
		for (i = 0; i < numblocks; i++) {
			uint8_t dat;
			int sftcnt = (i << (3 - sft)) & sftmsk;
			dat = bbt_get_entry(this, chip * numblocks + i);
			/* Do not store the reserved bbt blocks! */
			buf[offs + (i >> sft)] &= ~(msk[dat] << sftcnt);
		}

		nand_memset(&ops, 0, sizeof(ops));
		ops.mode = NAND_OPS_ERASE_POLL;
		ops.offset = to;
		ops.len = 1 << this->bbt_erase_shift;
		res = mtk_nand_erase(this, &ops);
		if (res < 0)
			goto outerr;

		res = scan_write_bbt(this, to, len, buf,
		                     td->options & NAND_BBT_NO_OOB ? NULL :
		                     &buf[len]);
		if (res < 0)
			goto outerr;

		nand_info("Bad block table written to 0x%x, version 0x%x",
		          (unsigned long)to, td->version[chip]);

		/* Mark it as used */
		td->pages[chip] = page;
	}
	return 0;

outerr:
	nand_err("nand_bbt: error while writing bad block table %d", res);
	return res;
}

/**
 * nand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static inline int nand_memory_bbt(struct mtk_nand_chip *chip, struct mtk_nand_bbt_descr *bd)
{

	return create_bbt(chip, chip->databuf, bd, -1);
}

/**
 * check_create - [GENERIC] create and write bbt(s) if necessary
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks the results of the previous call to read_bbt and creates
 * / updates the bbt(s) if necessary. Creation is necessary if no bbt was found
 * for the chip/device. Update is necessary if one of the tables is missing or
 * the version nr. of one table is less than the other.
 */
static int check_create(struct mtk_nand_chip *this, uint8_t *buf, struct mtk_nand_bbt_descr *bd)
{
	int i, chips, writeops, create, chipsel, res, res2;
	struct mtk_nand_bbt_descr *td = this->bbt_td;
	struct mtk_nand_bbt_descr *md = this->bbt_md;
	struct mtk_nand_bbt_descr *rd, *rd2;

	nand_debug("td->options:0x%x", td->options);

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP)
		chips = this->numchips;
	else
		chips = 1;

	for (i = 0; i < chips; i++) {
		writeops = 0;
		create = 0;
		rd = NULL;
		rd2 = NULL;
		res = res2 = 0;
		/* Per chip or per device? */
		chipsel = (td->options & NAND_BBT_PERCHIP) ? i : -1;
		/* Mirrored table available? */
		if (md) {
			if (td->pages[i] == -1 && md->pages[i] == -1) {
				create = 1;
				writeops = 0x03;
			} else if (td->pages[i] == -1) {
				rd = md;
				writeops = 0x01;
			} else if (md->pages[i] == -1) {
				rd = td;
				writeops = 0x02;
			} else if (td->version[i] == md->version[i]) {
				rd = td;
				if (!(td->options & NAND_BBT_VERSION))
					rd2 = md;
			} else if (((int8_t)(td->version[i] - md->version[i])) > 0) {
				rd = td;
				writeops = 0x02;
			} else {
				rd = md;
				writeops = 0x01;
			}
		} else {
			if (td->pages[i] == -1) {
				create = 1;
				writeops = 0x01;
			} else {
				rd = td;
			}
		}

		if (create) {
			/* Create the bad block table by scanning the device? */
			if (!(td->options & NAND_BBT_CREATE))
				continue;

			/* Create the table in memory by scanning the chip(s) */
			if (!(this->bbt_options & NAND_BBT_CREATE_EMPTY))
				create_bbt(this, buf, bd, chipsel);

			td->version[i] = 1;
			if (md)
				md->version[i] = 1;
		}

		/* Read back first? */
		if (rd) {
			res = read_abs_bbt(this, buf, rd, chipsel);
			if (res < 0) {
				/* Mark table as invalid */
				rd->pages[i] = -1;
				rd->version[i] = 0;
				i--;
				continue;
			}
		}
		/* If they weren't versioned, read both */
		if (rd2) {
			res2 = read_abs_bbt(this, buf, rd2, chipsel);
			if (res2  < 0) {
				/* Mark table as invalid */
				rd2->pages[i] = -1;
				rd2->version[i] = 0;
				i--;
				continue;
			}
		}

		/* Scrub the flash table(s)? */
		/* if (mtd_is_bitflip(res) || mtd_is_bitflip(res2))
		    writeops = 0x03; */

		/* Update version numbers before writing */
		if (md) {
			td->version[i] = max(td->version[i], md->version[i]);
			md->version[i] = td->version[i];
		}

		nand_debug("writeops:0x%x", writeops);
		/* Write the bad block table to the device? */
		if ((writeops & 0x01) && (td->options & NAND_BBT_WRITE)) {
			res = write_bbt(this, buf, td, md, chipsel);
			if (res < 0)
				return res;
		}

		/* Write the mirror bad block table to the device? */
		if ((writeops & 0x02) && md && (md->options & NAND_BBT_WRITE)) {
			res = write_bbt(this, buf, md, td, chipsel);
			if (res < 0)
				return res;
		}
	}
	return 0;
}

/**
 * mark_bbt_regions - [GENERIC] mark the bad block table regions
 * @mtd: MTD device structure
 * @td: bad block table descriptor
 *
 * The bad block table regions are marked as "bad" to prevent accidental
 * erasures / writes. The regions are identified by the mark 0x02.
 */
static void mark_bbt_region(struct mtk_nand_chip *this, struct mtk_nand_bbt_descr *td)
{
	int i, j, chips, block, nrblocks, update;
	uint8_t oldval;

	nand_debug("td->options:0x%x", td->options);

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chips = this->numchips;
		nrblocks = (int)(this->chipsize >> this->bbt_erase_shift);
	} else {
		chips = 1;
		nrblocks = (int)(this->totalsize >> this->bbt_erase_shift);
	}

	for (i = 0; i < chips; i++) {
		if ((td->options & NAND_BBT_ABSPAGE) ||
		        !(td->options & NAND_BBT_WRITE)) {
			if (td->pages[i] == -1)
				continue;
			block = td->pages[i] / this->page_per_block;
			oldval = bbt_get_entry(this, block);
			bbt_mark_entry(this, block, BBT_BLOCK_RESERVED);
			if ((oldval != BBT_BLOCK_RESERVED) &&
			        td->reserved_block_code)
				mtk_nand_update_bbt(this, (loff_t)block <<
				                    this->bbt_erase_shift);
			continue;
		}
		update = 0;
		if (td->options & NAND_BBT_LASTBLOCK)
			block = ((i + 1) * nrblocks) - td->maxblocks;
		else
			block = i * nrblocks;
		for (j = 0; j < td->maxblocks; j++) {
			oldval = bbt_get_entry(this, block);
			bbt_mark_entry(this, block, BBT_BLOCK_RESERVED);
			if (oldval != BBT_BLOCK_RESERVED)
				update = 1;
			block++;
		}
		/*
		 * If we want reserved blocks to be recorded to flash, and some
		 * new ones have been marked, then we need to update the stored
		 * bbts.  This should only happen once.
		 */
		if (update && td->reserved_block_code)
			mtk_nand_update_bbt(this, (loff_t)(block - 1) <<
			                    this->bbt_erase_shift);
	}
}

/**
 * verify_bbt_descr - verify the bad block description
 * @mtd: MTD device structure
 * @bd: the table to verify
 *
 * This functions performs a few sanity checks on the bad block description
 * table.
 */
static void verify_bbt_descr(struct mtk_nand_chip *this, struct mtk_nand_bbt_descr *bd)
{
	u32 pattern_len;
	u32 bits;
	u32 table_size;

	if (!bd)
		return;
	nand_debug("bd->options:0x%x", bd->options);

	pattern_len = bd->len;
	bits = bd->options & NAND_BBT_NRBITS_MSK;

	BUG_ON((this->bbt_options & NAND_BBT_NO_OOB) &&
	       !(this->bbt_options & NAND_BBT_USE_FLASH));
	BUG_ON(!bits);

	if (bd->options & NAND_BBT_VERSION)
		pattern_len++;

	if (bd->options & NAND_BBT_NO_OOB) {
		BUG_ON(!(this->bbt_options & NAND_BBT_USE_FLASH));
		BUG_ON(!(this->bbt_options & NAND_BBT_NO_OOB));
		BUG_ON(bd->offs);
		if (bd->options & NAND_BBT_VERSION)
			BUG_ON(bd->veroffs != bd->len);
		BUG_ON(bd->options & NAND_BBT_SAVECONTENT);
	}

	if (bd->options & NAND_BBT_PERCHIP)
		table_size = this->chipsize >> this->bbt_erase_shift;
	else
		table_size = this->totalsize >> this->bbt_erase_shift;
	table_size >>= 3;
	table_size *= bits;
	if (bd->options & NAND_BBT_NO_OOB)
		table_size += pattern_len;
	BUG_ON(table_size > (1 << this->bbt_erase_shift));
}

/**
 * nand_scan_bbt - [NAND Interface] scan, find, read and maybe create bad block table(s)
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function checks, if a bad block table(s) is/are already available. If
 * not it scans the device for manufacturer marked good / bad blocks and writes
 * the bad block table(s) to the selected place.
 *
 * The bad block table memory is allocated here. It must be freed by calling
 * the nand_free_bbt function.
 */
static int nand_scan_bbt(struct mtk_nand_chip *this, struct mtk_nand_bbt_descr *bd)
{
	int len, res;
	uint8_t *buf;
	struct mtk_nand_bbt_descr *td = this->bbt_td;
	struct mtk_nand_bbt_descr *md = this->bbt_md;

	len = (this->totalsize>> (this->bbt_erase_shift + 2)) ? : 1;

	nand_debug("len:%x", len);
	/*
	 * Allocate memory (2bit per block) and clear the memory bad block
	 * table.
	 */
	this->bbt = nand_malloc(len);
	if (!this->bbt)
		return -ENOMEM;
	nand_memset(this->bbt, 0, len);

	/*
	 * If no primary table decriptor is given, scan the device to build a
	 * memory based bad block table.
	 */
	if (!td) {
		if ((res = nand_memory_bbt(this, bd))) {
			nand_err("nand_bbt: can't scan flash and build the RAM-based BBT");
			goto err;
		}
		return 0;
	}
	verify_bbt_descr(this, td);
	verify_bbt_descr(this, md);

	/* Allocate a temporary buffer for one page buffer incl. oob */
	len = (1 << this->bbt_erase_shift);
	len += (len / this->pagesize) * this->oobsize;
	buf = NAND_DRAM_BUF_DATABUF_ADDR;/* nand_malloc(len); */
	if (!buf) {
		res = -ENOMEM;
		goto err;
	}

	/* Is the bbt at a given page? */
	if (td->options & NAND_BBT_ABSPAGE) {
		read_abs_bbts(this, buf, td, md);
	} else {
		/* Search the bad block table using a pattern in oob */
		search_read_bbts(this, buf, td, md);
	}

	res = check_create(this, buf, bd);
	if (res)
		goto err;

	/* Prevent the bbt regions from erasing / writing */
	mark_bbt_region(this, td);
	if (md)
		mark_bbt_region(this, md);

	/* nand_free(buf); */
	return 0;

err:
	nand_free(this->bbt);
	this->bbt = NULL;
	return res;
}

/**
 * nand_update_bbt - update bad block table(s)
 * @mtd: MTD device structure
 * @offs: the offset of the newly marked block
 *
 * The function updates the bad block table(s).
 */
static int mtk_nand_update_bbt(struct mtk_nand_chip *this, loff_t offs)
{
	int len, res = 0;
	int chip, chipsel;
	uint8_t *buf;
	struct mtk_nand_bbt_descr *td = this->bbt_td;
	struct mtk_nand_bbt_descr *md = this->bbt_md;

	if (!this->bbt || !td)
		return -EINVAL;

	nand_debug("offs:0x%llx", offs);
	/* Allocate a temporary buffer for one eraseblock incl. oob */
	len = (1 << this->bbt_erase_shift);
	len += (len /this->pagesize) * this->oobsize;

	buf = NAND_DRAM_BUF_DATABUF_ADDR;/* nand_malloc(len); */
	if (!buf)
		return -ENOMEM;

	/* Do we have a bbt per chip? */
	if (td->options & NAND_BBT_PERCHIP) {
		chip = (int)(offs / this->chipsize);
		chipsel = chip;
	} else {
		chip = 0;
		chipsel = -1;
	}

	td->version[chip]++;
	if (md)
		md->version[chip]++;

	/* Write the bad block table to the device? */
	if (td->options & NAND_BBT_WRITE) {
		res = write_bbt(this, buf, td, md, chipsel);
		if (res < 0)
			goto out;
	}
	/* Write the mirror bad block table to the device? */
	if (md && (md->options & NAND_BBT_WRITE)) {
		res = write_bbt(this, buf, md, td, chipsel);
	}

out:
	/* nand_free(buf); */
	return res;
}

/*
 * Define some generic bad / good block scan pattern which are used
 * while scanning a device for factory marked good / bad blocks.
 */
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };

/* Generic flash bbt descriptors */
static uint8_t bbt_pattern[] = {'B', 'b', 't', '0' };
static uint8_t mirror_pattern[] = {'1', 't', 'b', 'B' };

static struct mtk_nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = bbt_pattern
};

static struct mtk_nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = mirror_pattern
};

static struct mtk_nand_bbt_descr bbt_main_no_oob_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
	| NAND_BBT_NO_OOB,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = bbt_pattern
};

static struct mtk_nand_bbt_descr bbt_mirror_no_oob_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP
	| NAND_BBT_NO_OOB,
	.len = 4,
	.veroffs = 4,
	.maxblocks = NAND_BBT_SCAN_MAXBLOCKS,
	.pattern = mirror_pattern
};

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)
/**
 * nand_create_badblock_pattern - [INTERN] Creates a BBT descriptor structure
 * @this: NAND chip to create descriptor for
 *
 * This function allocates and initializes a mtk_nand_bbt_descr for BBM detection
 * based on the properties of @this. The new descriptor is stored in
 * this->badblock_pattern. Thus, this->badblock_pattern should be NULL when
 * passed to this function.
 */
static int nand_create_badblock_pattern(struct mtk_nand_chip *this)
{
	struct mtk_nand_bbt_descr *bd;

	if (this->badblock_pattern) {
		nand_err("Bad block pattern already allocated; not replacing");
		return -EINVAL;
	}

	bd = nand_malloc(sizeof(*bd));
	if (!bd)
		return -ENOMEM;

	bd->options = this->bbt_options & BADBLOCK_SCAN_MASK;
	bd->offs = this->badblockpos;
	bd->len = 1;
	bd->pattern = scan_ff_pattern;
	bd->options |= NAND_BBT_DYNAMICSTRUCT;
	this->badblock_pattern = bd;

	return 0;
}

/*
 * Set the bad block marker/indicator (BBM/BBI) patterns according to some
 * heuristic patterns using various detected parameters (e.g., manufacturer,
 * page size, cell-type information).
 */
void mtk_nand_set_bbt_options(struct mtk_nand_chip *chip, u8 maf_id)
{
	/*
	 * Bad block marker is stored in the last page of each block on Samsung
	 * and Hynix MLC devices; stored in first two pages of each block on
	 * Micron devices with 2KiB pages and on SLC Samsung, Hynix, Toshiba,
	 * AMD/Spansion, and Macronix.  All others scan only the first page.
	 */
	if (!nand_is_slc(chip) &&
	        (maf_id == NAND_MFR_SAMSUNG ||
	         maf_id == NAND_MFR_HYNIX))
		chip->bbt_options |= NAND_BBT_SCANLASTPAGE;
	else if ((nand_is_slc(chip) &&
	          (maf_id == NAND_MFR_SAMSUNG ||
	           maf_id == NAND_MFR_HYNIX ||
	           maf_id == NAND_MFR_TOSHIBA ||
	           maf_id == NAND_MFR_AMD ||
	           maf_id == NAND_MFR_MACRONIX)) ||
	         (chip->pagesize == 2048 &&
	          maf_id == NAND_MFR_MICRON))
		chip->bbt_options |= NAND_BBT_SCAN2NDPAGE;
}

/**
 * nand_default_bbt - [NAND Interface] Select a default bad block table for the device
 * @mtd: MTD device structure
 *
 * This function selects the default bad block table support for the device and
 * calls the nand_scan_bbt function.
 */
int mtk_nand_default_bbt(struct mtk_nand_chip *chip)
{
	int i, ret = 0;

	/* Is a flash based bad block table requested? */
	if (chip->bbt_options & NAND_BBT_USE_FLASH) {
		/* Use the default pattern descriptors */
		if (chip->bbt_options & NAND_BBT_NO_OOB) {
			chip->bbt_td = &bbt_main_no_oob_descr;
			chip->bbt_md = &bbt_mirror_no_oob_descr;
		} else {
			chip->bbt_td = &bbt_main_descr;
			chip->bbt_md = &bbt_mirror_descr;
		}
	} else {
		chip->bbt_td = NULL;
		chip->bbt_md = NULL;
	}

	if (!chip->badblock_pattern) {
		ret = nand_create_badblock_pattern(chip);
		if (ret)
			return ret;
	}

	nand_debug("chip->bbt_options:%x", chip->bbt_options);

	ret = nand_scan_bbt(chip, chip->badblock_pattern);

	nand_info("BBT check total block:%d", chip->totalsize >> chip->bbt_erase_shift);
	for (i = 0; i < chip->totalsize >> chip->bbt_erase_shift; i++) {
		if (bbt_get_entry(chip, i) == BBT_BLOCK_WORN)
			nand_info("Checked WORN bad blk: %d", i);
		else if (bbt_get_entry(chip, i) == BBT_BLOCK_FACTORY_BAD)
			nand_info("Checked Factory bad blk: %d", i);
		else if (bbt_get_entry(chip, i) != BBT_BLOCK_GOOD)
			nand_debug("Checked Reserved blk: %d", i);
	}

	return ret;
}

/**
 * nand_isreserved_bbt - [NAND Interface] Check if a block is reserved
 * @mtd: MTD device structure
 * @offs: offset in the device
 */
int nand_isreserved_bbt(struct mtk_nand_chip *this, loff_t offs)
{
	return bbt_get_entry(this, offs>>this->bbt_erase_shift) == BBT_BLOCK_RESERVED;
}

/**
 * nand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
 */
int mtk_nand_isbad_bbt(struct mtk_nand_chip *this, int block, int allowbbt)
{
	int res;

	res = bbt_get_entry(this, block);

	nand_debug("bbt info for (block %d) 0x%02x\n",
	           block, res);

	switch (res) {
		case BBT_BLOCK_GOOD:
			return 0;
		case BBT_BLOCK_WORN:
			return 1;
		case BBT_BLOCK_RESERVED:
			return allowbbt ? 0 : 1;
	}
	return 1;
}

/**
 * mtk_nand_markbad_bbt - [NAND Interface] Mark a block bad in the BBT
 * @mtd: MTD device structure
 * @offs: offset of the bad block
 */
int mtk_nand_markbad_bbt(struct mtk_nand_chip *this, loff_t offs)
{
	int block, ret = 0;

	block = (int)(offs >> this->bbt_erase_shift);
	
	nand_info("block:%d", block);

	/* Mark bad block in memory */
	bbt_mark_entry(this, block, BBT_BLOCK_WORN);

	/* Update flash-based bad block table */
	if (this->bbt_options & NAND_BBT_USE_FLASH)
		ret = mtk_nand_update_bbt(this, offs);

	return ret;
}


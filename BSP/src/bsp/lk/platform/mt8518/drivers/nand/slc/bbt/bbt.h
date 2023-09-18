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

#include "../slc.h"
#include "../slc_os.h"
#include "../nfi/nfi.h"
#include "../ecc/ecc.h"

#define BBT_BLOCK_GOOD      0x00
#define BBT_BLOCK_WORN      0x01
#define BBT_BLOCK_RESERVED      0x02
#define BBT_BLOCK_FACTORY_BAD   0x03

#define BBT_ENTRY_MASK      0x03
#define BBT_ENTRY_SHIFT     2

/* The maximum number of NAND chips in an array */
#define NAND_MAX_CHIPS      2

/**
 * struct nand_bbt_descr - bad block table descriptor
 * @options:    options for this descriptor
 * @pages:  the page(s) where we find the bbt, used with option BBT_ABSPAGE
 *      when bbt is searched, then we store the found bbts pages here.
 *      Its an array and supports up to 8 chips now
 * @offs:   offset of the pattern in the oob area of the page
 * @veroffs:    offset of the bbt version counter in the oob are of the page
 * @version:    version read from the bbt page during scan
 * @len:    length of the pattern, if 0 no pattern check is performed
 * @maxblocks:  maximum number of blocks to search for a bbt. This number of
 *      blocks is reserved at the end of the device where the tables are
 *      written.
 * @reserved_block_code: if non-0, this pattern denotes a reserved (rather than
 *              bad) block in the stored bbt
 * @pattern:    pattern to identify bad block table or factory marked good /
 *      bad blocks, can be NULL, if len = 0
 *
 * Descriptor for the bad block table marker and the descriptor for the
 * pattern which identifies good and bad blocks. The assumption is made
 * that the pattern and the version count are always located in the oob area
 * of the first block.
 */
struct mtk_nand_bbt_descr {
	int options;
	int pages[NAND_MAX_CHIPS];
	int offs;
	int veroffs;
	uint8_t version[NAND_MAX_CHIPS];
	int len;
	int maxblocks;
	int reserved_block_code;
	uint8_t *pattern;
};

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA    0x98
#define NAND_MFR_SAMSUNG    0xec
#define NAND_MFR_FUJITSU    0x04
#define NAND_MFR_NATIONAL   0x8f
#define NAND_MFR_RENESAS    0x07
#define NAND_MFR_STMICRO    0x20
#define NAND_MFR_HYNIX      0xad
#define NAND_MFR_MICRON 0x2c
#define NAND_MFR_AMD        0x01
#define NAND_MFR_MACRONIX   0xc2
#define NAND_MFR_EON        0x92
#define NAND_MFR_SANDISK    0x45
#define NAND_MFR_INTEL      0x89
#define NAND_MFR_ATO        0x9b
#define NAND_MFR_WINBOND    0xef

/* Options for the bad block table descriptors */

/* The number of bits used per block in the bbt on the device */
#define NAND_BBT_NRBITS_MSK 0x0000000F
#define NAND_BBT_1BIT       0x00000001
#define NAND_BBT_2BIT       0x00000002
#define NAND_BBT_4BIT       0x00000004
#define NAND_BBT_8BIT       0x00000008
/* The bad block table is in the last good block of the device */
#define NAND_BBT_LASTBLOCK  0x00000010
/* The bbt is at the given page, else we must scan for the bbt */
#define NAND_BBT_ABSPAGE    0x00000020
/* bbt is stored per chip on multichip devices */
#define NAND_BBT_PERCHIP    0x00000080
/* bbt has a version counter at offset veroffs */
#define NAND_BBT_VERSION    0x00000100
/* Create a bbt if none exists */
#define NAND_BBT_CREATE 0x00000200
/*
 * Create an empty BBT with no vendor information. Vendor's information may be
 * unavailable, for example, if the NAND controller has a different data and OOB
 * layout or if this information is already purged. Must be used in conjunction
 * with NAND_BBT_CREATE.
 */
#define NAND_BBT_CREATE_EMPTY   0x00000400
/* Write bbt if neccecary */
#define NAND_BBT_WRITE      0x00002000
/* Read and write back block contents when writing bbt */
#define NAND_BBT_SAVECONTENT    0x00004000
/* Search good / bad pattern on the first and the second page */
#define NAND_BBT_SCAN2NDPAGE    0x00008000
/* Search good / bad pattern on the last page of the eraseblock */
#define NAND_BBT_SCANLASTPAGE   0x00010000
/*
 * Use a flash based bad block table. By default, OOB identifier is saved in
 * OOB area. This option is passed to the default bad block table function.
 */
#define NAND_BBT_USE_FLASH  0x00020000
/*
 * Do not store flash based bad block table marker in the OOB area; store it
 * in-band.
 */
#define NAND_BBT_NO_OOB     0x00040000
/*
 * Do not write new bad block markers to OOB; useful, e.g., when ECC covers
 * entire spare area. Must be used with NAND_BBT_USE_FLASH.
 */
#define NAND_BBT_NO_OOB_BBM 0x00080000

/*
 * Flag set by nand_create_default_bbt_descr(), marking that the nand_bbt_descr
 * was allocated dynamicaly and must be freed in nand_release(). Has no meaning
 * in nand_chip.bbt_options.
 */
#define NAND_BBT_DYNAMICSTRUCT  0x80000000

/* The maximum number of blocks to scan for a bbt */
#define NAND_BBT_SCAN_MAXBLOCKS 4

/*
 * Constants for oob configuration
 */
#define NAND_SMALL_BADBLOCK_POS     5
#define NAND_LARGE_BADBLOCK_POS     0


/**
 * struct  mtk_bbt_info - [GENERIC] Bad Block Table data structure
 * @bbt_erase_shift:    [INTERN] number of address bits in a bbt entry
 * @badblockpos:    [INTERN] position of the bad block marker in the oob area
 * @options:        options for this descriptor
 * @bbt:        [INTERN] bad block table pointer
 * @isbad_bbt:      function to determine if a block is bad
 * @badblock_pattern:   [REPLACEABLE] bad block scan pattern used for
 *          initial bad block scan
 * @priv:       [OPTIONAL] pointer to private bbm date
 */
struct mtk_bbt_info {
	int bbt_erase_shift;
	int badblockpos;
	int options;

	uint8_t *bbt;

	/* int (*isbad_bbt)(struct mtk_nand_chip *chip, loff_t ofs, int allowbbt); */

	/* TODO Add more NAND specific fileds */
	struct mtk_nand_bbt_descr *badblock_pattern;

	void *priv;
};

struct mtk_nand_chip;

extern int mtk_nand_default_bbt(struct mtk_nand_chip *chip);
extern int mtk_nand_markbad_bbt(struct mtk_nand_chip *this, loff_t offs);
extern int mtk_nand_isbad_bbt(struct mtk_nand_chip *this, int block, int allowbbt);
extern void mtk_nand_set_bbt_options(struct mtk_nand_chip *chip, u8 maf_id);

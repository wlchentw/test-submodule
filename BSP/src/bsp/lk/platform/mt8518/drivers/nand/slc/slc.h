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

#include "slc_os.h"
#include "ecc/ecc.h"

/* Select the chip by setting nCE to low */
#define NAND_NCE        0x01
/* Select the command latch by setting CLE to high */
#define NAND_CLE        0x02
/* Select the address latch by setting ALE to high */
#define NAND_ALE        0x04

#define NAND_CTRL_CLE       (NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE       (NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE    0x80

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0      0
#define NAND_CMD_READ1      1
#define NAND_CMD_RNDOUT     5
#define NAND_CMD_PAGEPROG   0x10
#define NAND_CMD_READOOB    0x50
#define NAND_CMD_ERASE1     0x60
#define NAND_CMD_STATUS     0x70
#define NAND_CMD_SEQIN      0x80
#define NAND_CMD_RNDIN      0x85
#define NAND_CMD_READID     0x90
#define NAND_CMD_ERASE2     0xd0
#define NAND_CMD_PARAM      0xec
#define NAND_CMD_GET_FEATURES   0xee
#define NAND_CMD_SET_FEATURES   0xef
#define NAND_CMD_RESET      0xff

#define NAND_CMD_LOCK       0x2a
#define NAND_CMD_UNLOCK1    0x23
#define NAND_CMD_UNLOCK2    0x24

/* Extended commands for large page devices */
#define NAND_CMD_READSTART  0x30
#define NAND_CMD_RNDOUTSTART    0xE0
#define NAND_CMD_CACHEDPROG 0x15

#define NAND_CMD_NONE       -1

/* Status bits */
#define NAND_STATUS_FAIL        0x01
#define NAND_STATUS_FAIL_N1     0x02
#define NAND_STATUS_TRUE_READY  0x20
#define NAND_STATUS_READY       0x40
#define NAND_STATUS_WP      0x80

/* chip options definition */
/*
 * Some MLC NANDs need data scrambling to limit bitflips caused by repeated
 * patterns.
 */
#define NAND_NEED_SCRAMBLING    0x00002000
#define NAND_BBT_SUPPORT        0x00000001

#define NAND_MAX_ID_LEN 8

struct mtk_nand_flash_dev {
	const char *name;
	u8 id[NAND_MAX_ID_LEN];
	u8 id_len;

	/* unit: KByte */
	u32 chipsize;
	u32 erasesize;
	u32 pagesize;
	u16 oobsize;
	u32 fdmeccsize;
	u8 bits_per_cell;

	/* customized setting if need */
	u32 acctiming;
	u32 ecc_size;
	u32 ecc_strength;
	u32 bbt_options;
	u32 options;
};

enum {
	NAND_OPS_RAW_DMA_POLL = 0,
	NAND_OPS_RAW_DMA_IRQ,
	NAND_OPS_RAW_PIO_POLL,
	NAND_OPS_RAW_PIO_IRQ,
	NAND_OPS_ECC_DMA_POLL,
	NAND_OPS_ECC_DMA_IRQ,
	NAND_OPS_ECC_PIO_POLL,
	NAND_OPS_ECC_PIO_IRQ,

	NAND_OPS_ERASE_POLL,
	NAND_OPS_ERASE_IRQ,
};

struct mtk_nand_ops {
	u32 mode;
	u64 offset;
	u64 len;
	u8 *datbuf;
	/* ecc protected oob data */
	u8 *oobeccbuf;
	u32 oobecclen;
	/* ecc unprotected oob data */
	u8 *oobfreebuf;
	u32 oobfreelen;
};

struct mtk_nand_chip {
	u8 (*read_byte)(struct mtk_nand_chip *nand);
	void (*write_byte)(struct mtk_nand_chip *nand, u8 byte);
	void (*write_buf)(struct mtk_nand_chip *nand, const u8 *buf, int len);
	void (*read_buf)(struct mtk_nand_chip *nand, u8 *buf, int len);
	void (*select_chip)(struct mtk_nand_chip *nand, int chip);
	void (*cmd_ctrl)(struct mtk_nand_chip *nand, int dat, unsigned int ctrl);
	int (*dev_ready)(struct mtk_nand_chip *nand);
	int (*wait_busy_irq)(struct mtk_nand_chip *nand);
	void (*cmdfunc)(struct mtk_nand_chip *nand, unsigned command, int column,
	                int page_addr);
	int(*waitfunc)(struct mtk_nand_chip *this, int polling);
	int (*scan_bbt)(struct mtk_nand_chip *chip);

	int (*block_bad)(struct mtk_nand_chip *chip, u32 page);
	int (*block_markbad)(struct mtk_nand_chip *chip, u32 page);

	int (*write_page_ecc_dma_polling)(struct mtk_nand_chip *chip, const u8 *buf,
	                                  int page);
	int (*write_page_ecc_dma_irq)(struct mtk_nand_chip *chip, const u8 *buf,
	                              int page);
	int (*write_page_ecc_pio_polling)(struct mtk_nand_chip *chip, const u8 *buf,
	                                  int page);
	int (*write_page_ecc_pio_irq)(struct mtk_nand_chip *chip, const u8 *buf,
	                              int page);
	int (*write_page_raw_dma_polling)(struct mtk_nand_chip *chip, const u8 *buf,
	                                  int page);
	int (*write_page_raw_dma_irq)(struct mtk_nand_chip *chip, const u8 *buf,
	                              int page);
	int (*write_page_raw_pio_polling)(struct mtk_nand_chip *chip, const u8 *buf,
	                                  int page);
	int (*write_page_raw_pio_irq)(struct mtk_nand_chip *chip, const u8 *buf,
	                              int page);
	int (*write_subpage_ecc_dma_polling)(struct mtk_nand_chip *chip, u32 offset,
	                                     u32 data_len, const u8 *buf, int page);
	int (*write_subpage_ecc_dma_irq)(struct mtk_nand_chip *chip, u32 offset,
	                                 u32 data_len, const u8 *buf, int page);
	int (*write_subpage_ecc_pio_polling)(struct mtk_nand_chip *chip, u32 offset,
	                                     u32 data_len, const u8 *buf, int page);
	int (*write_subpage_ecc_pio_irq)(struct mtk_nand_chip *chip, u32 offset,
	                                 u32 data_len, const u8 *buf, int page);

	int (*read_subpage_ecc_dma_polling)(struct mtk_nand_chip *chip, u32 off,
	                                    u32 len, u8 *p, int pg);
	int (*read_subpage_ecc_dma_irq)(struct mtk_nand_chip *chip, u32 off,
	                                u32 len, u8 *p, int pg);
	int (*read_subpage_ecc_pio_polling)(struct mtk_nand_chip *chip, u32 off,
	                                    u32 len, u8 *p, int pg);
	int (*read_subpage_ecc_pio_irq)(struct mtk_nand_chip *chip, u32 off,
	                                u32 len, u8 *p, int pg);
	int (*read_page_ecc_dma_polling)(struct mtk_nand_chip *chip, u8 *p, int pg);
	int (*read_page_ecc_dma_irq)(struct mtk_nand_chip *chip, u8 *p, int pg);
	int (*read_page_ecc_pio_polling)(struct mtk_nand_chip *chip, u8 *p, int pg);
	int (*read_page_ecc_pio_irq)(struct mtk_nand_chip *chip, u8 *p, int pg);
	int (*read_page_raw_dma_polling)(struct mtk_nand_chip *chip, u8 *buf, int page);
	int (*read_page_raw_dma_irq)(struct mtk_nand_chip *chip, u8 *buf, int page);
	int (*read_page_raw_pio_polling)(struct mtk_nand_chip *chip, u8 *buf, int page);
	int (*read_page_raw_pio_irq)(struct mtk_nand_chip *chip, u8 *buf, int page);

	/* nand device information */
	u64 totalsize;
	/* unit: Byte */
	u64 chipsize;
	u32 pagesize;
	u32 oobsize;
	u32 blocksize;
	u32 ecc_size;
	u32 ecc_strength;
	u32 ecc_steps;
	u32 subpagesize;
	u32 fdm_ecc_size;
	u8 bits_per_cell;
	u32 page_per_chip;
	u32 page_per_block;
	int chip_delay;
	u32 acctiming;
	u32 options;
	u8 numchips;
	int activechip;

	u8 *databuf;
	u8 *oob_poi;

	/* BBT related */
	u32 bbt_options;
	int badblockpos;
	int badblockbits;
	u32 bbt_erase_shift;

	u8 *bbt;
	struct mtk_nand_bbt_descr *bbt_td;
	struct mtk_nand_bbt_descr *bbt_md;
	struct mtk_nand_bbt_descr *badblock_pattern;

	/* block device information if need */
	u32 lbasize;
	u32 lbacnt;

	struct mtk_ecc_stats stats;

	void *priv;
};

static inline void *nand_get_controller_data(struct mtk_nand_chip *chip)
{
	return chip->priv;
}

static inline void nand_set_controller_data(struct mtk_nand_chip *chip, void *priv)
{
	chip->priv = priv;
}

static inline bool nand_is_slc(struct mtk_nand_chip *chip)
{
	return chip->bits_per_cell == 1;
}

extern struct mtk_nand_flash_dev nand_flash_devs[];

extern int mtk_nand_erase(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops);
extern int mtk_nand_write(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops);
extern int mtk_nand_read(struct mtk_nand_chip *chip, struct mtk_nand_ops *ops);
extern int mtk_nand_block_isbad(struct mtk_nand_chip *nand, u32 page);
extern int mtk_nand_init(struct mtk_nand_chip **ext_nand);
extern int mtk_nand_scan(struct mtk_nand_chip *chip, int maxchips);
extern void lk_nand_irq_handler(unsigned int irq);
extern int nand_get_alignment(void);
extern int nand_write_img(u64 addr, void *data, u32 img_sz, u32 partition_size, int img_type);

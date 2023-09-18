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
#include "nfi.h"

/* NAND controller register definition */
#define NFI_CNFG        (0x00)
#define     CNFG_AHB        NAND_BIT(0)
#define     CNFG_READ_EN        NAND_BIT(1)
#define     CNFG_DMA_BURST_EN   NAND_BIT(2)
#define     CNFG_RESEED_SEC_EN  NAND_BIT(4)
#define     CNFG_RAND_SEL       NAND_BIT(5)
#define     CNFG_RAND_MASK      (3 << 4)
#define     CNFG_BYTE_RW        NAND_BIT(6)
#define     CNFG_HW_ECC_EN      NAND_BIT(8)
#define     CNFG_AUTO_FMT_EN    NAND_BIT(9)
#define     CNFG_OP_READ        (1 << 12)
#define     CNFG_OP_PROGRAM     (3 << 12)
#define     CNFG_OP_CUST        (6 << 12)
#define     CNFG_OP_MASK        (7 << 12)
#define NFI_PAGEFMT     (0x04)
#define     PAGEFMT_FDM_ECC_SHIFT   (12)
#define     PAGEFMT_FDM_SHIFT   (8)
#define     PAGEFMT_SPARE_16    (0)
#define     PAGEFMT_SPARE_26    (1)
#define     PAGEFMT_SPARE_27    (2)
#define     PAGEFMT_SPARE_28    (3)
#define     PAGEFMT_SPARE_32    (4)
#define     PAGEFMT_SPARE_36    (5)
#define     PAGEFMT_SPARE_40    (6)
#define     PAGEFMT_SPARE_44    (7)
#define     PAGEFMT_SPARE_48    (8)
#define     PAGEFMT_SPARE_49    (9)
#define     PAGEFMT_SPARE_50    (0xa)
#define     PAGEFMT_SPARE_51    (0xb)
#define     PAGEFMT_SPARE_52    (0xc)
#define     PAGEFMT_SPARE_62    (0xd)
#define     PAGEFMT_SPARE_61    (0xe)
#define     PAGEFMT_SPARE_63    (0xf)
#define     PAGEFMT_SPARE_64    (0x10)
#define     PAGEFMT_SPARE_67    (0x11)
#define     PAGEFMT_SPARE_74    (0x12)
#define     PAGEFMT_SPARE_SHIFT (16)
#define     PAGEFMT_SEC_SEL_512 NAND_BIT(2)
#define     PAGEFMT_512_2K      (0)
#define     PAGEFMT_2K_4K       (1)
#define     PAGEFMT_4K_8K       (2)
#define     PAGEFMT_8K_16K      (3)
/* NFI control */
#define NFI_CON         (0x08)
#define     CON_FIFO_FLUSH      NAND_BIT(0)
#define     CON_NFI_RST     NAND_BIT(1)
#define     CON_BRD         NAND_BIT(8)  /* burst  read */
#define     CON_BWR         NAND_BIT(9) /* burst  write */
#define     CON_SEC_SHIFT       (12)
/* Timming control register */
#define NFI_ACCCON      (0x0c)
#define NFI_INTR_EN     (0x10)
#define     INTR_BUSY_RETURN_EN NAND_BIT(4)
#define     INTR_AHB_DONE_EN    NAND_BIT(6)
#define NFI_INTR_STA        (0x14)
#define NFI_CMD         (0x20)
#define NFI_ADDRNOB     (0x30)
#define NFI_COLADDR     (0x34)
#define NFI_ROWADDR     (0x38)
#define NFI_STRDATA     (0x40)
#define     STAR_EN         (1)
#define     STAR_DE         (0)
#define NFI_CNRNB       (0x44)
#define NFI_DATAW       (0x50)
#define NFI_DATAR       (0x54)
#define NFI_PIO_DIRDY       (0x58)
#define     PIO_DI_RDY      (0x01)
#define NFI_STA         (0x60)
#define     STA_CMD         NAND_BIT(0)
#define     STA_ADDR        NAND_BIT(1)
#define     STA_BUSY        NAND_BIT(8)
#define     STA_EMP_PAGE        NAND_BIT(12)
#define     NFI_FSM_CUSTDATA    (0xe << 16)
#define     NFI_FSM_MASK        (0xf << 16)
#define NFI_ADDRCNTR        (0x70)
#define     CNTR_MASK       NAND_GENMASK(16, 12)
#define     ADDRCNTR_SEC_SHIFT  (12)
#define     ADDRCNTR_SEC(val) \
         (((val) & CNTR_MASK) >> ADDRCNTR_SEC_SHIFT)
#define NFI_STRADDR     (0x80)
#define NFI_BYTELEN     (0x84)
#define NFI_CSEL        (0x90)
#define NFI_FDML(x)     (0xa0 + (x) * sizeof(u32) * 2)
#define NFI_FDMM(x)     (0xa4 + (x) * sizeof(u32) * 2)
#define NFI_FDM_MAX_SIZE    (8)
#define NFI_FDM_MIN_SIZE    (1)
#define NFI_DEBUG_CON1      (0x220)
#define     BYPASS_MASTER_EN    NAND_BIT(15)
#define NFI_MASTER_STA      (0x224)
#define     MASTER_STA_MASK     (0x3)
#define NFI_RANDOM_CNFG     (0x238)
#define     RAN_ENCODE_EN       NAND_BIT(0)
#define     ENCODE_SEED_SHIFT   (1)
#define     RAN_DECODE_EN       NAND_BIT(16)
#define     DECODE_SEED_SHIFT   (17)
#define     RAN_SEED_MASK       (0x7fff)
#define NFI_EMPTY_THRESH    (0x23c)
#define NFI_SNAND_CNFG      (0x55c)

#define MTK_RESET_TIMEOUT   (1000000)
#define MTK_MAX_SECTOR      (16)
#define MTK_NAND_MAX_NSELS  (2)

void swap(char *x, char *y)
{
        char tmp;
        tmp = *x;
        *x = *y;
        *y = tmp;
}

u32 clamp(u32 fValue, u32 fMin, u32 fMax)
{
        return fValue > fMax ? fMax : (fValue < fMin ? fMin : fValue);
}

static inline struct mtk_nfc_nand_chip *to_mtk_nand(struct mtk_nand_chip *chip)
{
	return containerof(chip, struct mtk_nfc_nand_chip, chip);
}

static inline u8 *data_ptr(struct mtk_nand_chip *chip, const u8 *p, int i)
{
	return (u8 *)p + i * chip->ecc_size;
}

static inline int mtk_data_len(struct mtk_nand_chip *chip)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);

	return chip->ecc_size + mtk_nand->spare_per_sector;
}

static inline u8 *mtk_data_ptr(struct mtk_nand_chip *chip,  int i)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	return nfc->buffer + i * mtk_data_len(chip);
}

static inline u8 *oob_ptr(struct mtk_nand_chip *chip, int i)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	u8 *poi;

	/* map the sector's FDM data to free oob:
	 * the beginning of the oob area stores the FDM data of bad mark sectors
	 */
	if (i < mtk_nand->bad_mark.sec)
		poi = chip->oob_poi + (i + 1) * mtk_nand->fdm.reg_size;
	else if (i == mtk_nand->bad_mark.sec)
		poi = chip->oob_poi;
	else
		poi = chip->oob_poi + i * mtk_nand->fdm.reg_size;

	return poi;
}

static inline u8 *mtk_oob_ptr(struct mtk_nand_chip *chip, int i)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	return nfc->buffer + i * mtk_data_len(chip) + chip->ecc_size;
}

static inline void nfi_writel(struct mtk_nfc *nfc, u32 val, u32 reg)
{
	//nand_debug("val:0x%x, reg:0x%x nfc->regs:%x", val, reg, nfc->regs);
	nand_writel(val, nfc->regs + reg);
	//nand_debug("val:0x%x, reg:0x%x", val, reg);
}

static inline void nfi_writew(struct mtk_nfc *nfc, u16 val, u32 reg)
{
	//nand_debug("val:0x%x, reg:0x%x nfc->regs:%x", val, reg, nfc->regs);
	nand_writew(val, nfc->regs + reg);
	//nand_debug("val:0x%x, reg:0x%x nfc->regs:%x", val, reg, nfc->regs);	
}

static inline void nfi_writeb(struct mtk_nfc *nfc, u8 val, u32 reg)
{
	//nand_debug("val:0x%x, reg:0x%x nfc->regs:%x", val, reg, nfc->regs);
	nand_writeb(val, nfc->regs + reg);
}

static inline u32 nfi_readl(struct mtk_nfc *nfc, u32 reg)
{
	//nand_debug("reg:0x%x nfc->regs:%x", reg, nfc->regs);

	return nand_readl(nfc->regs + reg);
}

static inline u16 nfi_readw(struct mtk_nfc *nfc, u32 reg)
{
	//nand_debug("reg:0x%x nfc->regs:%x", reg, nfc->regs);
	return nand_readw(nfc->regs + reg);
}

static inline u8 nfi_readb(struct mtk_nfc *nfc, u32 reg)
{
	//nand_debug("reg:0x%x nfc->regs:%x", reg, nfc->regs);
	return nand_readb(nfc->regs + reg);
}

static void mtk_nfc_dump_reg(struct mtk_nfc *nfc)
{
	u32 i;

	nand_info("nfi and nfiecc registers");
	for (i = 0; i < 0x2000; i+=4) {
		if (!(i % 16))
			nand_info("\n0x%x: ", i);
		nand_info("%x", nfi_readl(nfc, i));
	}

	nand_info("nfi clock setting");
	nand_info("0x10000004 = 0x%x", *((volatile u32*)0x10000004));
	nand_info("0x1000007c = 0x%x", *((volatile u32*)0x1000007c));
	nand_info("0x10000024 = 0x%x", *((volatile u32*)0x10000024));
	nand_info("0x1000003c = 0x%x", *((volatile u32*)0x1000003c));
	nand_info("0x10000070 = 0x%x", *((volatile u32*)0x10000070));

	nand_info("nfi gpio setting");
	nand_info("0x10005320 = 0x%x", *((volatile u32*)0x10005320));
	nand_info("0x10005330 = 0x%x", *((volatile u32*)0x10005330));
	nand_info("0x10005340 = 0x%x", *((volatile u32*)0x10005340));
	nand_info("0x10005460 = 0x%x", *((volatile u32*)0x10005460));
	nand_info("0x10005470 = 0x%x", *((volatile u32*)0x10005470));
	nand_info("0x10005480 = 0x%x", *((volatile u32*)0x10005480));
	nand_info("0x10005e60 = 0x%x", *((volatile u32*)0x10005e60));
	nand_info("0x10005d00 = 0x%x", *((volatile u32*)0x10005d00));
	nand_info("0x10005d60 = 0x%x", *((volatile u32*)0x10005d60));
	nand_info("0x10005d70 = 0x%x", *((volatile u32*)0x10005d70));
	nand_info("0x10005c10 = 0x%x", *((volatile u32*)0x10005c10));
	nand_info("0x10005c60 = 0x%x", *((volatile u32*)0x10005c60));
	nand_info("0x10005c70 = 0x%x", *((volatile u32*)0x10005c70));

}

static void mtk_nfc_hw_reset(struct mtk_nfc *nfc)
{
	//nand_debug("enter");

	/* reset all registers and force the NFI master to terminate */
	nfi_writel(nfc, CON_FIFO_FLUSH | CON_NFI_RST, NFI_CON);

	/* wait for the master to finish the last transaction */
	if (!check_with_timeout(!(nfi_readl(nfc, NFI_MASTER_STA) & MASTER_STA_MASK),
	                        MTK_RESET_TIMEOUT))
		nand_err("NFI HW reset timeout!");

	/* ensure any status register affected by the NFI master is reset */
	nfi_writel(nfc, CON_FIFO_FLUSH | CON_NFI_RST, NFI_CON);
	nfi_writew(nfc, STAR_DE, NFI_STRDATA);

	//nand_debug("end");

}

/* Randomizer define */
#define SS_SEED_NUM     128
#define RAND_SEED_SHIFT(op) ((op) == RAND_ENCODE ? ENCODE_SEED_SHIFT : DECODE_SEED_SHIFT)
#define RAND_EN(op)     ((op) == RAND_ENCODE ? RAN_ENCODE_EN : RAN_DECODE_EN)
static u16 ss_randomizer_seed[SS_SEED_NUM] = {
	0x576A, 0x05E8, 0x629D, 0x45A3, 0x649C, 0x4BF0, 0x2342, 0x272E,
	0x7358, 0x4FF3, 0x73EC, 0x5F70, 0x7A60, 0x1AD8, 0x3472, 0x3612,
	0x224F, 0x0454, 0x030E, 0x70A5, 0x7809, 0x2521, 0x484F, 0x5A2D,
	0x492A, 0x043D, 0x7F61, 0x3969, 0x517A, 0x3B42, 0x769D, 0x0647,
	0x7E2A, 0x1383, 0x49D9, 0x07B8, 0x2578, 0x4EEC, 0x4423, 0x352F,
	0x5B22, 0x72B9, 0x367B, 0x24B6, 0x7E8E, 0x2318, 0x6BD0, 0x5519,
	0x1783, 0x18A7, 0x7B6E, 0x7602, 0x4B7F, 0x3648, 0x2C53, 0x6B99,
	0x0C23, 0x67CF, 0x7E0E, 0x4D8C, 0x5079, 0x209D, 0x244A, 0x747B,
	0x350B, 0x0E4D, 0x7004, 0x6AC3, 0x7F3E, 0x21F5, 0x7A15, 0x2379,
	0x1517, 0x1ABA, 0x4E77, 0x15A1, 0x04FA, 0x2D61, 0x253A, 0x1302,
	0x1F63, 0x5AB3, 0x049A, 0x5AE8, 0x1CD7, 0x4A00, 0x30C8, 0x3247,
	0x729C, 0x5034, 0x2B0E, 0x57F2, 0x00E4, 0x575B, 0x6192, 0x38F8,
	0x2F6A, 0x0C14, 0x45FC, 0x41DF, 0x38DA, 0x7AE1, 0x7322, 0x62DF,
	0x5E39, 0x0E64, 0x6D85, 0x5951, 0x5937, 0x6281, 0x33A1, 0x6A32,
	0x3A5A, 0x2BAC, 0x743A, 0x5E74, 0x3B2E, 0x7EC7, 0x4FD2, 0x5D28,
	0x751F, 0x3EF8, 0x39B1, 0x4E49, 0x746B, 0x6EF6, 0x44BE, 0x6DB7
};

static void mtk_nfc_randomizer_init(struct mtk_nand_chip *chip)
{
	/* check whether randomizer efuse is on */
	if ((*EFUSE_RANDOM_CFG) & EFUSE_RANDOM_ENABLE)
		chip->options |= NAND_NEED_SCRAMBLING;
}

void mtk_nfc_randomizer_enable(struct mtk_nand_chip *chip, int page,
                               enum mtk_randomizer_operation rand, int repage)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 reg = 0;
	u32 loop = SS_SEED_NUM;

	if (!(chip->options & NAND_NEED_SCRAMBLING))
		return;

	/* nand_debug("page:0x%x repage %d", page, repage); */

	mtk_nfc_hw_reset(nfc);

	/* randomizer type and reseed type setup */
	reg = nfi_readl(nfc, NFI_CNFG) | CNFG_RAND_SEL;
	if (repage)
		reg &= ~CNFG_RESEED_SEC_EN;
	else
		reg |= CNFG_RESEED_SEC_EN;
	nfi_writel(nfc, reg, NFI_CNFG);

	/* randomizer seed and type setup */
	if (chip->page_per_block <= SS_SEED_NUM)
		loop = chip->page_per_block;
	reg = (ss_randomizer_seed[page % loop] & RAN_SEED_MASK) << RAND_SEED_SHIFT(rand);
	reg |= RAND_EN(rand);

	nfi_writel(nfc, reg, NFI_RANDOM_CNFG);
}

void mtk_nfc_randomizer_disable(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	if (!(chip->options & NAND_NEED_SCRAMBLING))
		return;

	nfi_writel(nfc, 0, NFI_RANDOM_CNFG);

	mtk_nfc_hw_reset(nfc);
}

static int mtk_nfc_send_command(struct mtk_nfc *nfc, u8 command)
{
	nfi_writel(nfc, command, NFI_CMD);

	if (!check_with_timeout(!(nfi_readl(nfc, NFI_STA) & STA_CMD), MTK_NAND_TIMEOUT))
		nand_err("send cmd 0x%x timeout", command);

	return 0;
}

static int mtk_nfc_send_address(struct mtk_nfc *nfc, int addr)
{
	nfi_writel(nfc, addr, NFI_COLADDR);
	nfi_writel(nfc, 0, NFI_ROWADDR);
	nfi_writew(nfc, 1, NFI_ADDRNOB);

	if (!check_with_timeout(!(nfi_readl(nfc, NFI_STA) & STA_ADDR), MTK_NAND_TIMEOUT))
		nand_err("send cmd 0x%x timeout", addr);

	return 0;
}

static int mtk_nfc_irq_wait(struct mtk_nfc *nfc, u32 timeout)
{
	int ret;

	ret = nand_wait_for_completion_timeout(&nfc->done, timeout);
	if (ret != 0) {
		nand_err("failed to get event INT=0x%x",
		         nfi_readw(nfc, NFI_INTR_EN));
		return ret;
	}

	return 0;
}

static enum handler_return mtk_nfc_interrupt_handler(void *arg)
{
	struct mtk_nfc *nfc = arg;
	u16 sta, ien;

	sta = nfi_readw(nfc, NFI_INTR_STA);
	ien = nfi_readw(nfc, NFI_INTR_EN);
	if (!(sta & ien))
		return NAND_IRQ_NONE;

	nfi_writew(nfc, ~sta & ien, NFI_INTR_EN);

	/* MUST BE *false*! otherwise, schedule in interrupt */
	nand_complete(&nfc->done);

	return NAND_IRQ_HANDLED;
}

static int mtk_nfc_request_irq(struct mtk_nfc *nfc)
{
	nand_init_completion(&nfc->done);
	//mtk_nand_request_irq(NAND_NFI_IRQ_BIT_ID, &mtk_nfc_interrupt_handler, nfc);
	return 0;
}

static int mtk_nfc_hw_runtime_config(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	u32 fmt, spare;

	if (!chip->pagesize)
		return -EINVAL;

	/* nand_debug("spare_per_sector:%d, ecc_size:%d, acctiming:0x%x",
		mtk_nand->spare_per_sector, mtk_nand->fdm.ecc_size, mtk_nand->acctiming); */

	spare = mtk_nand->spare_per_sector;

	switch (chip->pagesize) {
		case 512:
			fmt = PAGEFMT_512_2K | PAGEFMT_SEC_SEL_512;
			break;
		case KB(2):
			if (chip->ecc_size == 512)
				fmt = PAGEFMT_2K_4K | PAGEFMT_SEC_SEL_512;
			else
				fmt = PAGEFMT_512_2K;
			break;
		case KB(4):
			if (chip->ecc_size == 512)
				fmt = PAGEFMT_4K_8K | PAGEFMT_SEC_SEL_512;
			else
				fmt = PAGEFMT_2K_4K;
			break;
		case KB(8):
			if (chip->ecc_size == 512)
				fmt = PAGEFMT_8K_16K | PAGEFMT_SEC_SEL_512;
			else
				fmt = PAGEFMT_4K_8K;
			break;
		case KB(16):
			fmt = PAGEFMT_8K_16K;
			break;
		default:
			nand_err("invalid page len: %d", chip->pagesize);
			return -EINVAL;
	}

	/*
	 * the hardware will double the value for this eccsize, so we need to
	 * halve it
	 */
	if (chip->ecc_size == 1024)
		spare >>= 1;

	switch (spare) {
		case 16:
			fmt |= (PAGEFMT_SPARE_16 << PAGEFMT_SPARE_SHIFT);
			break;
		case 26:
			fmt |= (PAGEFMT_SPARE_26 << PAGEFMT_SPARE_SHIFT);
			break;
		case 27:
			fmt |= (PAGEFMT_SPARE_27 << PAGEFMT_SPARE_SHIFT);
			break;
		case 28:
			fmt |= (PAGEFMT_SPARE_28 << PAGEFMT_SPARE_SHIFT);
			break;
		case 32:
			fmt |= (PAGEFMT_SPARE_32 << PAGEFMT_SPARE_SHIFT);
			break;
		case 36:
			fmt |= (PAGEFMT_SPARE_36 << PAGEFMT_SPARE_SHIFT);
			break;
		case 40:
			fmt |= (PAGEFMT_SPARE_40 << PAGEFMT_SPARE_SHIFT);
			break;
		case 44:
			fmt |= (PAGEFMT_SPARE_44 << PAGEFMT_SPARE_SHIFT);
			break;
		case 48:
			fmt |= (PAGEFMT_SPARE_48 << PAGEFMT_SPARE_SHIFT);
			break;
		case 49:
			fmt |= (PAGEFMT_SPARE_49 << PAGEFMT_SPARE_SHIFT);
			break;
		case 50:
			fmt |= (PAGEFMT_SPARE_50 << PAGEFMT_SPARE_SHIFT);
			break;
		case 51:
			fmt |= (PAGEFMT_SPARE_51 << PAGEFMT_SPARE_SHIFT);
			break;
		case 52:
			fmt |= (PAGEFMT_SPARE_52 << PAGEFMT_SPARE_SHIFT);
			break;
		case 62:
			fmt |= (PAGEFMT_SPARE_62 << PAGEFMT_SPARE_SHIFT);
			break;
		case 63:
			fmt |= (PAGEFMT_SPARE_63 << PAGEFMT_SPARE_SHIFT);
			break;
		case 64:
			fmt |= (PAGEFMT_SPARE_64 << PAGEFMT_SPARE_SHIFT);
			break;
		default:
			nand_err("invalid spare per sector %d", spare);
			return -EINVAL;
	}

	fmt |= mtk_nand->fdm.reg_size << PAGEFMT_FDM_SHIFT;
	fmt |= mtk_nand->fdm.ecc_size << PAGEFMT_FDM_ECC_SHIFT;
	nfi_writel(nfc, fmt, NFI_PAGEFMT);

	nfc->ecc_cfg.strength = chip->ecc_strength;
	nfc->ecc_cfg.len = chip->ecc_size + mtk_nand->fdm.ecc_size;

	return 0;
}

static void mtk_nfc_select_chip(struct mtk_nand_chip *chip, int chip_num)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	if ((chip_num < 0) || (chip_num == chip->activechip))
		return;

	if(chip_num > 0)
		nand_info("chip_num:%d", chip_num);

	if (!mtk_nfc_hw_runtime_config(chip)) {
		chip->activechip = chip_num;
	}

	nfi_writel(nfc, chip_num, NFI_CSEL);
}

static int mtk_nfc_dev_ready(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	//nand_debug("0x%x", nfi_readl(nfc, NFI_STA));

	if (nfi_readl(nfc, NFI_STA) & STA_BUSY)
		return 0;

	return 1;
}

static int mtk_nfc_wait_busy_irq(struct mtk_nand_chip *chip)
{
	int ret;
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	/* set wait busy interrupt */
	nfi_writew(nfc, INTR_BUSY_RETURN_EN, NFI_INTR_EN);

	/* wait interrupt */
	ret = mtk_nfc_irq_wait(nfc, MTK_NAND_TIMEOUT);
	if (!ret) {
		nand_err("wait busy interrupt timeout");
		nfi_writew(nfc, 0, NFI_INTR_EN);
		return -ETIMEDOUT;
	}

	return 0;
}

static void mtk_nfc_cmd_ctrl(struct mtk_nand_chip *chip, int dat, unsigned int ctrl)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u16 reg;

	//nand_debug("ctrl:0x%x dat:0x%x", ctrl, dat);
	if (ctrl & NAND_ALE) {
		mtk_nfc_send_address(nfc, dat);
	} else if (ctrl & NAND_CLE) {
		mtk_nfc_hw_reset(nfc);

		reg = nfi_readw(nfc, NFI_CNFG);
		reg &= CNFG_RAND_MASK;
		reg |= CNFG_OP_CUST;
		nfi_writew(nfc, reg, NFI_CNFG);
		mtk_nfc_send_command(nfc, dat);
	}

	//nand_debug("ctrl:0x%x dat:0x%x", ctrl, dat);

}

static inline void mtk_nfc_wait_ioready(struct mtk_nfc *nfc)
{
	if (!check_with_timeout((nfi_readl(nfc, NFI_PIO_DIRDY) & PIO_DI_RDY), MTK_NAND_TIMEOUT)) {
		nand_err("data not ready");
		nand_err("cntr 0x%x cnfg 0x%x fmt 0x%x con 0x%x",
		         nfi_readl(nfc, NFI_BYTELEN), nfi_readl(nfc, NFI_CNFG),
		         nfi_readl(nfc, NFI_PAGEFMT), nfi_readl(nfc, NFI_CON));
	}
}

static inline u8 mtk_nfc_read_byte(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 reg;

	/* after each byte read, the NFI_STA reg is reset by the hardware */
	reg = nfi_readl(nfc, NFI_STA) & NFI_FSM_MASK;
	if (reg != NFI_FSM_CUSTDATA) {
		reg = nfi_readw(nfc, NFI_CNFG);
		reg |= CNFG_BYTE_RW | CNFG_READ_EN;
		nfi_writew(nfc, reg, NFI_CNFG);

		/*
		 * set to max sector to allow the HW to continue reading over
		 * unaligned accesses
		 */
		reg = (MTK_MAX_SECTOR << CON_SEC_SHIFT) | CON_BRD;
		nfi_writel(nfc, reg, NFI_CON);

		/* trigger to fetch data */
		nfi_writew(nfc, STAR_EN, NFI_STRDATA);
	}

	mtk_nfc_wait_ioready(nfc);

	return nfi_readb(nfc, NFI_DATAR);
}

static void mtk_nfc_read_buf(struct mtk_nand_chip *chip, u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++)
		buf[i] = mtk_nfc_read_byte(chip);
}

static void mtk_nfc_write_byte(struct mtk_nand_chip *chip, u8 byte)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 reg;

	reg = nfi_readl(nfc, NFI_STA) & NFI_FSM_MASK;

	if (reg != NFI_FSM_CUSTDATA) {
		reg = nfi_readw(nfc, NFI_CNFG) | CNFG_BYTE_RW;
		nfi_writew(nfc, reg, NFI_CNFG);

		reg = MTK_MAX_SECTOR << CON_SEC_SHIFT | CON_BWR;
		nfi_writel(nfc, reg, NFI_CON);

		nfi_writew(nfc, STAR_EN, NFI_STRDATA);
	}

	mtk_nfc_wait_ioready(nfc);
	nfi_writeb(nfc, byte, NFI_DATAW);
}

static void mtk_nfc_write_buf(struct mtk_nand_chip *chip, const u8 *buf, int len)
{
	int i;

	for (i = 0; i < len; i++)
		mtk_nfc_write_byte(chip, buf[i]);
}

static int mtk_nfc_sector_encode(struct mtk_nand_chip *chip, u8 *data, int dma, int polling)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	int size = chip->ecc_size + mtk_nand->fdm.reg_size;

	if (dma)
		nfc->ecc_cfg.mode = ECC_DMA_MODE;
	else
		nfc->ecc_cfg.mode = ECC_PIO_MODE;
	nfc->ecc_cfg.op = ECC_ENCODE;

	return mtk_ecc_encode(nfc->ecc, &nfc->ecc_cfg, data, size, polling);
}

static void mtk_nfc_no_bad_mark_swap(struct mtk_nand_chip *a, u8 *b, int c)
{
	/* nop */
}

static void mtk_nfc_bad_mark_swap(struct mtk_nand_chip *chip, u8 *buf, int raw)
{
	struct mtk_nfc_nand_chip *nand = to_mtk_nand(chip);
	u32 bad_pos = nand->bad_mark.pos;

	if (raw)
		bad_pos += nand->bad_mark.sec * mtk_data_len(chip);
	else
		bad_pos += nand->bad_mark.sec * chip->ecc_size;

	swap(chip->oob_poi, buf + bad_pos);
}

static int mtk_nfc_format_subpage(struct mtk_nand_chip *chip, u32 offset,
                                  u32 len, const u8 *buf, int dma, int polling)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 start, end;
	int i, ret;

	start = offset / chip->ecc_size;
	end = DIV_ROUND_UP(offset + len, chip->ecc_size);

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(mtk_data_ptr(chip, i), data_ptr(chip, buf, i),
		            chip->ecc_size);

		if (start > i || i >= end)
			continue;

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		nand_memcpy(mtk_oob_ptr(chip, i), oob_ptr(chip, i), fdm->reg_size);

		/* program the CRC back to the OOB */
		ret = mtk_nfc_sector_encode(chip, mtk_data_ptr(chip, i), dma, polling);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static void mtk_nfc_format_page(struct mtk_nand_chip *chip, const u8 *buf)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 i;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	for (i = 0; i < chip->ecc_steps; i++) {
		if (buf)
			nand_memcpy(mtk_data_ptr(chip, i), data_ptr(chip, buf, i),
			            chip->ecc_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		nand_memcpy(mtk_oob_ptr(chip, i), oob_ptr(chip, i), fdm->reg_size);
	}
}

static inline void mtk_nfc_read_fdm(struct mtk_nand_chip *chip, u32 start,
                                    u32 sectors)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 vall, valm;
	u8 *oobptr;
	u32 i, j;

	for (i = 0; i < sectors; i++) {
		oobptr = oob_ptr(chip, start + i);
		vall = nfi_readl(nfc, NFI_FDML(i));
		valm = nfi_readl(nfc, NFI_FDMM(i));

		for (j = 0; j < fdm->reg_size; j++)
			oobptr[j] = (j >= 4 ? valm : vall) >> ((j % 4) * 8);
	}
}

static inline void mtk_nfc_write_fdm(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 vall, valm;
	u8 *oobptr;
	u32 i, j;

	for (i = 0; i < chip->ecc_steps; i++) {
		oobptr = oob_ptr(chip, i);
		vall = 0;
		valm = 0;
		for (j = 0; j < 8; j++) {
			if (j < 4)
				vall |= (j < fdm->reg_size ? oobptr[j] : 0xff)
				        << (j * 8);
			else
				valm |= (j < fdm->reg_size ? oobptr[j] : 0xff)
				        << ((j - 4) * 8);
		}
		nfi_writel(nfc, vall, NFI_FDML(i));
		nfi_writel(nfc, valm, NFI_FDMM(i));
	}
}

static int mtk_nfc_do_write_page(struct mtk_nand_chip *chip,
                                 const u8 *buf, int page, int len, int raw, int dma, int polling)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 *buf32 = (u32 *)buf;
	u32 addr, reg, i;
	u32 data_len = chip->ecc_size;
	int ret = 0, byterw;

	addr = nand_kvaddr_to_paddr(buf);
	if (dma) {
		reg = nfi_readw(nfc, NFI_CNFG) | CNFG_AHB | CNFG_DMA_BURST_EN;
		nfi_writew(nfc, reg, NFI_CNFG);
		nand_dma_map(buf, len, true, NULL);
	}

	nfi_writel(nfc, chip->ecc_steps << CON_SEC_SHIFT, NFI_CON);
	nfi_writel(nfc, addr, NFI_STRADDR);

	if (dma && (!polling)) {
		nfi_writew(nfc, INTR_AHB_DONE_EN, NFI_INTR_EN);
	}

	reg = nfi_readl(nfc, NFI_CON) | CON_BWR;
	nfi_writel(nfc, reg, NFI_CON);
	nfi_writew(nfc, STAR_EN, NFI_STRDATA);

	if (!dma) {
		if (raw)
			data_len = mtk_data_len(chip);
		data_len *= chip->ecc_steps;

		if (data_len & 0x3) {
			reg = nfi_readw(nfc, NFI_CNFG) | CNFG_BYTE_RW;
			nfi_writew(nfc, reg, NFI_CNFG);
			byterw = 1;
		} else {
			data_len >>= 2;
			byterw = 0;
		}

		for (i = 0; i < data_len; i++) {
			mtk_nfc_wait_ioready(nfc);
			if (!byterw)
				nfi_writel(nfc, buf32[i],NFI_DATAW);
			else
				nfi_writeb(nfc, buf[i], NFI_DATAW);
		}
	}

	if (dma && (!polling)) {
#ifdef MTK_NAND_IRQ_EN
		ret = mtk_nfc_irq_wait(nfc, MTK_NAND_TIMEOUT);
#endif
		if (!ret) {
			nand_err("program ahb done timeout");
			nfi_writew(nfc, 0, NFI_INTR_EN);
			ret = -ETIMEDOUT;
			goto timeout;
		}
	}

	if (!check_with_timeout(ADDRCNTR_SEC(nfi_readl(nfc, NFI_ADDRCNTR)) >= chip->ecc_steps,
	                        MTK_NAND_TIMEOUT))
		nand_err("do page write timeout");

timeout:
	if (dma)
		nand_dma_unmap(buf, len, false, NULL);

	nfi_writel(nfc, 0, NFI_CON);

	return ret;
}

static int mtk_nfc_write_page(struct mtk_nand_chip *chip,
                              const u8 *buf, int page, int raw, int dma, int polling)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	u32 len;
	const u8 *bufpoi;
	u32 reg;
	int ret;

	if (!raw) {
		/* OOB => FDM: from register,  ECC: from HW */
		reg = nfi_readw(nfc, NFI_CNFG) | CNFG_AUTO_FMT_EN;
		nfi_writew(nfc, reg | CNFG_HW_ECC_EN, NFI_CNFG);

		nfc->ecc_cfg.op = ECC_ENCODE;
		nfc->ecc_cfg.mode = ECC_NFI_MODE;
		ret = mtk_ecc_enable(nfc->ecc, &nfc->ecc_cfg, polling);
		if (ret) {
			/* clear NFI config */
			reg = nfi_readw(nfc, NFI_CNFG);
			reg &= ~(CNFG_AUTO_FMT_EN | CNFG_HW_ECC_EN);
			nfi_writew(nfc, reg, NFI_CNFG);

			return ret;
		}

		nand_memcpy(nfc->buffer, buf, chip->pagesize);
		mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, raw);
		bufpoi = nfc->buffer;

		/* write OOB into the FDM registers (OOB area in MTK NAND) */
		mtk_nfc_write_fdm(chip);
	} else {
		bufpoi = buf;
	}

	len = chip->pagesize + (raw ? chip->oobsize : 0);
	ret = mtk_nfc_do_write_page(chip, bufpoi, page, len, raw, dma, polling);

	if (!raw)
		mtk_ecc_disable(nfc->ecc);

	return ret;
}

static int mtk_nfc_write_page_ecc_dma_polling(struct mtk_nand_chip *chip, const u8 *buf,
        int page)
{
	return mtk_nfc_write_page(chip, buf, page, 0, 1, 1);
}

static int mtk_nfc_write_page_ecc_dma_irq(struct mtk_nand_chip *chip, const u8 *buf,
        int page)
{
	return mtk_nfc_write_page(chip, buf, page, 0, 1, 0);
}

static int mtk_nfc_write_page_ecc_pio_polling(struct mtk_nand_chip *chip, const u8 *buf,
        int page)
{
	return mtk_nfc_write_page(chip, buf, page, 0, 0, 1);
}

static int mtk_nfc_write_page_ecc_pio_irq(struct mtk_nand_chip *chip, const u8 *buf,
        int page)
{
	return mtk_nfc_write_page(chip, buf, page, 0, 0, 0);
}

static int mtk_nfc_write_page_raw_dma_polling(struct mtk_nand_chip *chip,
        const u8 *buf, int pg)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	mtk_nfc_format_page(chip, buf);
	return mtk_nfc_write_page(chip, nfc->buffer, pg, 1, 1, 1);
}

static int mtk_nfc_write_page_raw_dma_irq(struct mtk_nand_chip *chip,
        const u8 *buf, int pg)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	mtk_nfc_format_page(chip, buf);
	return mtk_nfc_write_page(chip, nfc->buffer, pg, 1, 1, 0);
}

static int mtk_nfc_write_page_raw_pio_polling(struct mtk_nand_chip *chip,
        const u8 *buf, int pg)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	mtk_nfc_format_page(chip, buf);
	return mtk_nfc_write_page(chip, nfc->buffer, pg, 1, 0, 1);
}

static int mtk_nfc_write_page_raw_pio_irq(struct mtk_nand_chip *chip,
        const u8 *buf, int pg)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);

	mtk_nfc_format_page(chip, buf);
	return mtk_nfc_write_page(chip, nfc->buffer, pg, 1, 0, 0);
}

static int mtk_nfc_write_subpage_ecc_dma_polling(struct mtk_nand_chip *chip, u32 offset,
        u32 data_len, const u8 *buf, int page)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	int ret;

	ret = mtk_nfc_format_subpage(chip, offset, data_len, buf, 1, 1);
	if (ret < 0)
		return ret;

	/* use the data in the private buffer (now with FDM and CRC) */
	return mtk_nfc_write_page(chip, nfc->buffer, page, 1, 1, 1);
}

static int mtk_nfc_write_subpage_ecc_dma_irq(struct mtk_nand_chip *chip, u32 offset,
        u32 data_len, const u8 *buf, int page)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	int ret;

	ret = mtk_nfc_format_subpage(chip, offset, data_len, buf, 1, 0);
	if (ret < 0)
		return ret;

	/* use the data in the private buffer (now with FDM and CRC) */
	return mtk_nfc_write_page(chip, nfc->buffer, page, 1, 1, 0);
}

static int mtk_nfc_write_subpage_ecc_pio_polling(struct mtk_nand_chip *chip, u32 offset,
        u32 data_len, const u8 *buf, int page)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	int ret;

	ret = mtk_nfc_format_subpage(chip, offset, data_len, buf, 0, 1);
	if (ret < 0)
		return ret;

	/* use the data in the private buffer (now with FDM and CRC) */
	return mtk_nfc_write_page(chip, nfc->buffer, page, 1, 0, 1);
}

static int mtk_nfc_write_subpage_ecc_pio_irq(struct mtk_nand_chip *chip, u32 offset,
        u32 data_len, const u8 *buf, int page)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	int ret;

	ret = mtk_nfc_format_subpage(chip, offset, data_len, buf, 0, 0);
	if (ret < 0)
		return ret;

	/* use the data in the private buffer (now with FDM and CRC) */
	return mtk_nfc_write_page(chip, nfc->buffer, page, 1, 0, 0);
}

static int mtk_nfc_update_ecc_stats(struct mtk_nand_chip *chip, u8 *buf, u32 sectors)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_ecc_stats stats;
	int rc, i;

	rc = nfi_readl(nfc, NFI_STA) & STA_EMP_PAGE;
	if (rc) {
		nand_memset(buf, 0xff, sectors * chip->ecc_size);
		for (i = 0; i < sectors; i++)
			nand_memset(oob_ptr(chip, i), 0xff, mtk_nand->fdm.reg_size);
		return 0;
	}

	mtk_ecc_get_stats(nfc->ecc, &stats, sectors);
	chip->stats.corrected += stats.corrected;
	chip->stats.failed += stats.failed;

	return stats.bitflips;
}

static int mtk_nfc_read_subpage(struct mtk_nand_chip *chip,
                                u32 data_offs, u32 readlen,
                                u8 *bufpoi, int page, int raw, int dma, int polling)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_ecc_stats stats;
	u32 spare = mtk_nand->spare_per_sector;
	u32 column, sectors, start, end, reg;
	u32 addr, i, j;
	int bitflips = 0;
	u32 len, correct = 0, fail = 0;
	u8 *buf;
	u32 *buf32;
	u32 data_len = chip->ecc_size;
	int rc, byterw;

	start = data_offs / chip->ecc_size;
	end = DIV_ROUND_UP(data_offs + readlen, chip->ecc_size);

	sectors = end - start;
	column = start * (chip->ecc_size + spare);

	len = sectors * chip->ecc_size + ((raw || !dma) ? sectors * spare : 0);
	buf = bufpoi + start * (chip->ecc_size + ((raw || !dma) ? sectors * spare : 0));
	buf32 = (u32 *)buf;

	if (column != 0)
		chip->cmdfunc(chip, NAND_CMD_RNDOUT, column, -1);

	addr = nand_kvaddr_to_paddr(buf);

	reg = nfi_readw(nfc, NFI_CNFG);
	reg |= CNFG_READ_EN;
	if (dma)
		reg |= CNFG_DMA_BURST_EN | CNFG_AHB;
	if (!raw) {
		reg |= CNFG_HW_ECC_EN;
		if (dma)
			reg |= CNFG_AUTO_FMT_EN;
		nfi_writew(nfc, reg, NFI_CNFG);

		nfc->ecc_cfg.mode = ECC_NFI_MODE;
		nfc->ecc_cfg.sectors = sectors;
		nfc->ecc_cfg.op = ECC_DECODE;
		if (dma) {
			nfc->ecc_cfg.deccon = ECC_DEC_CORRECT;
		} else {
			nfc->ecc_cfg.deccon = ECC_DEC_LOCATE;
		}
		rc = mtk_ecc_enable(nfc->ecc, &nfc->ecc_cfg, polling);
		if (rc) {
			nand_err("ecc enable failed");
			/* clear NFI_CNFG */
			reg &= ~(CNFG_DMA_BURST_EN | CNFG_AHB | CNFG_READ_EN |
			         CNFG_AUTO_FMT_EN | CNFG_HW_ECC_EN);
			nfi_writew(nfc, reg, NFI_CNFG);
			/* error handle */
			return rc;
		}
	} else {
		nfi_writew(nfc, reg, NFI_CNFG);
	}

	if (dma)
		nand_dma_map(buf, len, false, NULL);

	nfi_writel(nfc, sectors << CON_SEC_SHIFT, NFI_CON);
	nfi_writel(nfc, addr, NFI_STRADDR);

	if (dma && (!polling)) {
		nfi_writew(nfc, INTR_AHB_DONE_EN, NFI_INTR_EN);
	}
	reg = nfi_readl(nfc, NFI_CON) | CON_BRD;
	nfi_writel(nfc, reg, NFI_CON);
	nfi_writew(nfc, STAR_EN, NFI_STRDATA);

	if (!dma) {
		data_len = mtk_data_len(chip);

		if (data_len & 0x3) {
			reg = nfi_readw(nfc, NFI_CNFG) | CNFG_BYTE_RW;
			nfi_writew(nfc, reg, NFI_CNFG);
			byterw = 1;
		} else {
			data_len >>= 2;
			byterw = 0;
		}
		if (!raw) {
			stats.bitflips = 0;
			correct = chip->stats.corrected;
			fail = chip->stats.failed;
		}
		for (i = 0; i < sectors; i++) {
			for (j = 0; j < data_len; j++) {
				mtk_nfc_wait_ioready(nfc);
				if (!byterw)
					*(buf32 + (i * data_len) + j) = nfi_readl(nfc, NFI_DATAR);
				else
					*(buf + (i * data_len) + j) = nfi_readb(nfc, NFI_DATAR);
			}
			if (!raw) {
				rc = mtk_ecc_cpu_correct(nfc->ecc, &stats, buf +
				                         (i * (byterw ? data_len : (data_len << 2))), i, polling);
				if (rc < 0)
					goto disecc;
				chip->stats.corrected += stats.corrected;
				chip->stats.failed += stats.failed;
				if (stats.failed) {
					nand_info("sectoer %d uncorrect", i);
				}
			}
		}
		if (!raw) {
			bitflips = stats.bitflips;
			rc = nfi_readl(nfc, NFI_STA) & STA_EMP_PAGE;
			if (rc) {
				nand_info("page %d is empty", page);
				nand_memset(buf, 0xff, sectors * mtk_data_len(chip));
				bitflips = 0;
				chip->stats.corrected = correct;
				chip->stats.failed = fail;
			}
		}
	}

	if (dma && (!polling)) {
		rc = mtk_nfc_irq_wait(nfc, MTK_NAND_TIMEOUT);
		if (!rc) {
			nand_err("read ahb/dma done timeout");
		}
	}

	if (!check_with_timeout(ADDRCNTR_SEC(nfi_readl(nfc, NFI_BYTELEN)) >= sectors,
	                        MTK_NAND_TIMEOUT)) {
		nand_err("subpage done timeout %d", nfi_readl(nfc, NFI_BYTELEN));
		nand_err("cnfg 0x%x fmt 0x%x\n con 0x%x", nfi_readl(nfc, NFI_CNFG),
		         nfi_readl(nfc, NFI_PAGEFMT), nfi_readl(nfc, NFI_CON));
		bitflips = -EIO;
	} else {
		if ((!raw) && dma) {
			bitflips = 0;
			rc = mtk_ecc_wait_done(nfc->ecc, ECC_DECODE, polling);
			if (!rc)
				rc = mtk_ecc_wait_decode_fsm_idle(nfc->ecc);
			bitflips = rc < 0 ? -ETIMEDOUT :
			           mtk_nfc_update_ecc_stats(chip, buf, sectors);
			nand_dma_unmap(buf, len, false, NULL);
			mtk_nfc_read_fdm(chip, start, sectors);
			bitflips = rc < 0 ? -ETIMEDOUT :
			           mtk_nfc_update_ecc_stats(chip, buf, sectors);
		}
	}

	if (raw)
		goto done;

disecc:
	mtk_ecc_disable(nfc->ecc);

	if (!dma)
		goto done;

	if (clamp(mtk_nand->bad_mark.sec, start, end) == mtk_nand->bad_mark.sec)
		mtk_nand->bad_mark.bm_swap(chip, bufpoi, raw);
done:
	nfi_writel(nfc, 0, NFI_CON);

	return bitflips;
}

static int mtk_nfc_read_subpage_ecc_dma_polling(struct mtk_nand_chip *chip, u32 off,
        u32 len, u8 *p, int pg)
{
	return mtk_nfc_read_subpage(chip, off, len, p, pg, 0, 1, 1);
}

static int mtk_nfc_read_subpage_ecc_dma_irq(struct mtk_nand_chip *chip, u32 off,
        u32 len, u8 *p, int pg)
{
	return mtk_nfc_read_subpage(chip, off, len, p, pg, 0, 1, 0);
}

static int mtk_nfc_read_subpage_ecc_pio_polling(struct mtk_nand_chip *chip, u32 off,
        u32 len, u8 *p, int pg)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 sectors, start, end;
	int i, ret;

	start = off / chip->ecc_size;
	end = DIV_ROUND_UP(off + len, chip->ecc_size);
	sectors = end - start;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, off, len, nfc->buffer, pg, 0, 0, 1);
	if (ret < 0)
		return ret;

	for (i = start; i < end; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (p)
			nand_memcpy(data_ptr(chip, p, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_subpage_ecc_pio_irq(struct mtk_nand_chip *chip, u32 off,
        u32 len, u8 *p, int pg)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	u32 sectors, start, end;
	int i, ret;

	start = off / chip->ecc_size;
	end = DIV_ROUND_UP(off + len, chip->ecc_size);
	sectors = end - start;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, off, len, nfc->buffer, pg, 0, 0, 0);
	if (ret < 0)
		return ret;

	for (i = start; i < end; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (p)
			nand_memcpy(data_ptr(chip, p, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_ecc_dma_polling(struct mtk_nand_chip *chip, u8 *p,
        int pg)
{
	return mtk_nfc_read_subpage(chip, 0, chip->pagesize, p, pg, 0, 1, 1);
}

static int mtk_nfc_read_page_ecc_dma_irq(struct mtk_nand_chip *chip, u8 *p,
        int pg)
{
	return mtk_nfc_read_subpage(chip, 0, chip->pagesize, p, pg, 0, 1, 0);
}

static int mtk_nfc_read_page_ecc_pio_polling(struct mtk_nand_chip *chip, u8 *p,
        int pg)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer, pg, 0, 0, 1);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (p)
			nand_memcpy(data_ptr(chip, p, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_ecc_pio_irq(struct mtk_nand_chip *chip, u8 *p,
        int pg)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer, pg, 0, 0, 0);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (p)
			nand_memcpy(data_ptr(chip, p, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_raw_dma_polling(struct mtk_nand_chip *chip,
        u8 *buf, int page)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer,
	                           page, 1, 1, 1);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (buf)
			nand_memcpy(data_ptr(chip, buf, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_raw_dma_irq(struct mtk_nand_chip *chip,
        u8 *buf, int page)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer,
	                           page, 1, 1, 0);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (buf)
			nand_memcpy(data_ptr(chip, buf, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_raw_pio_polling(struct mtk_nand_chip *chip,
        u8 *buf, int page)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer,
	                           page, 1, 0, 1);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (buf)
			nand_memcpy(data_ptr(chip, buf, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static int mtk_nfc_read_page_raw_pio_irq(struct mtk_nand_chip *chip,
        u8 *buf, int page)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	struct mtk_nfc_fdm *fdm = &mtk_nand->fdm;
	int i, ret;

	nand_memset(nfc->buffer, 0xff, chip->pagesize + chip->oobsize);
	ret = mtk_nfc_read_subpage(chip, 0, chip->pagesize, nfc->buffer,
	                           page, 1, 0, 0);
	if (ret < 0)
		return ret;

	for (i = 0; i < chip->ecc_steps; i++) {
		nand_memcpy(oob_ptr(chip, i), mtk_oob_ptr(chip, i), fdm->reg_size);

		if (i == mtk_nand->bad_mark.sec)
			mtk_nand->bad_mark.bm_swap(chip, nfc->buffer, 1);

		if (buf)
			nand_memcpy(data_ptr(chip, buf, i), mtk_data_ptr(chip, i),
			            chip->ecc_size);
	}

	return ret;
}

static void mtk_nfc_set_timing(struct mtk_nand_chip *chip)
{
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 reg;

	/*
	 * ACCON: access timing control register
	 * -------------------------------------
	 * 31:28: minimum required time for CS post pulling down after accessing
	 *  the device
	 * 27:22: minimum required time for CS pre pulling down before accessing
	 *  the device
	 * 21:16: minimum required time from NCEB low to NREB low
	 * 15:12: minimum required time from NWEB high to NREB low.
	 * 11:08: write enable hold time
	 * 07:04: write wait states
	 * 03:00: read wait states
	 */
	if (chip->acctiming) {
		nfi_writel(nfc, chip->acctiming, NFI_ACCCON);
		if ((chip->acctiming == 0x10804111) 
			|| (chip->acctiming == 0x10804122)){
			/* Set strobe_sel to delay 1 cycle for NRE */
			reg = nfi_readl(nfc, NFI_DEBUG_CON1);
			reg &= ~(0x3 << 3);
			reg |= (0x1 << 3);
			nfi_writel(nfc, reg, NFI_DEBUG_CON1);
		}
	} else
		nfi_writel(nfc, 0x10804222, NFI_ACCCON);
}

static void mtk_nfc_hw_init(struct mtk_nand_chip *chip)
{
	struct mtk_nfc_nand_chip *mtk_nand = to_mtk_nand(chip);
	struct mtk_nfc *nfc = nand_get_controller_data(chip);
	u32 reg;

	/* Change to NFI mode */
	nfi_writel(nfc, 0, NFI_SNAND_CNFG);

	nfi_writel(nfc, 0, NFI_CSEL);

	/* disable bypass_master_en */
	reg = nfi_readl(nfc, NFI_DEBUG_CON1);
	reg &= ~BYPASS_MASTER_EN;
	nfi_writel(nfc, reg, NFI_DEBUG_CON1);
	/*
	 * ACCON: access timing control register
	 * -------------------------------------
	 * 31:28: minimum required time for CS post pulling down after accessing
	 *  the device
	 * 27:22: minimum required time for CS pre pulling down before accessing
	 *  the device
	 * 21:16: minimum required time from NCEB low to NREB low
	 * 15:12: minimum required time from NWEB high to NREB low.
	 * 11:08: write enable hold time
	 * 07:04: write wait states
	 * 03:00: read wait states
	 */
	if (chip->acctiming)
		nfi_writel(nfc, chip->acctiming, NFI_ACCCON);
	else
		nfi_writel(nfc, 0x30C77FFF, NFI_ACCCON);

	/*
	 * CNRNB: nand ready/busy register
	 * -------------------------------
	 * 7:4: timeout register for polling the NAND busy/ready signal
	 * 0  : poll the status of the busy/ready signal after [7:4]*16 cycles.
	 */
	nfi_writew(nfc, 0xf1, NFI_CNRNB);
	nfi_writew(nfc, PAGEFMT_8K_16K, NFI_PAGEFMT);

	mtk_nfc_hw_reset(nfc);

	nfi_readl(nfc, NFI_INTR_STA);
	nfi_writel(nfc, 0, NFI_INTR_EN);
}

static void mtk_nfc_set_fdm(struct mtk_nfc_fdm *fdm, struct mtk_nand_chip *nand)
{
	struct mtk_nfc_nand_chip *chip = to_mtk_nand(nand);
	u32 ecc_bytes;

	ecc_bytes = DIV_ROUND_UP(nand->ecc_strength * ECC_PARITY_BITS, 8);

	fdm->reg_size = chip->spare_per_sector - ecc_bytes;
	if (fdm->reg_size > NFI_FDM_MAX_SIZE)
		fdm->reg_size = NFI_FDM_MAX_SIZE;

	/* bad block mark storage */
	fdm->ecc_size = nand->fdm_ecc_size > NFI_FDM_MAX_SIZE ? NFI_FDM_MAX_SIZE : nand->fdm_ecc_size;
}

static void mtk_nfc_set_bad_mark_ctl(struct mtk_nfc_bad_mark_ctl *bm_ctl,
                                     struct mtk_nand_chip *nand)
{
	/* mt8561 no swap */
	if (0) { /*(nand->pagesize == 512)*/
		bm_ctl->bm_swap = mtk_nfc_no_bad_mark_swap;
	} else {
		bm_ctl->bm_swap = mtk_nfc_bad_mark_swap;
		bm_ctl->sec = nand->pagesize / mtk_data_len(nand);
		bm_ctl->pos = nand->pagesize % mtk_data_len(nand);
	}
}

static void mtk_nfc_set_spare_per_sector(u32 *sps, struct mtk_nand_chip *nand)
{
	u32 spare[] = {16, 26, 27, 28, 32, 36, 40, 44,
	               48, 49, 50, 51, 52, 62, 63, 64
	              };
	u32 eccsteps, i;

	eccsteps = nand->pagesize / nand->ecc_size;
	*sps = nand->oobsize / eccsteps;

	nand_debug("pagesize:%d, oobsize:%d, ecc_size:%d",
		nand->pagesize, nand->oobsize, nand->ecc_size);
	
	if (nand->ecc_size == 1024)
		*sps >>= 1;

	for (i = 0; i < sizeof(spare) / sizeof(u32); i++) {
		if (*sps <= spare[i]) {
			if (*sps == spare[i])
				*sps = spare[i];
			else if (i != 0)
				*sps = spare[i - 1];
			break;
		}
	}

	if (i >= sizeof(spare) / sizeof(u32))
		*sps = spare[sizeof(spare) / sizeof(u32) - 1];

	if (nand->ecc_size == 1024)
		*sps <<= 1;
}

static void dump_nand_info(struct mtk_nand_chip *chip)
{
	nand_info("------------dump nand info ------------\n");
	nand_info("totalsize 			0x%llx\n", chip->totalsize);
	nand_info("chipsize 			0x%llx\n", chip->chipsize);
	nand_info("pagesize 			0x%x\n", chip->pagesize);
	nand_info("oobsize 			0x%x\n", chip->oobsize);
	nand_info("blocksize 			0x%x\n", chip->blocksize);
	nand_info("ecc_size 			0x%x\n", chip->ecc_size);
	nand_info("ecc_strength 		0x%x\n", chip->ecc_strength);
	nand_info("ecc_steps 			0x%x\n", chip->ecc_steps);
	nand_info("subpagesize 			0x%x\n", chip->subpagesize);
	nand_info("fdm_ecc_size 		0x%x\n", chip->fdm_ecc_size);
	nand_info("bits_per_cell 		0x%x\n", chip->bits_per_cell);
	nand_info("page_per_chip 		0x%x\n", chip->page_per_chip);
	nand_info("page_per_block 		0x%x\n", chip->page_per_block);
	nand_info("chip_delay 			0x%x\n", chip->chip_delay);
	nand_info("acctiming 			0x%x\n", chip->acctiming);
	nand_info("options 			0x%x\n", chip->options);
	nand_info("numchips 			0x%x\n", chip->numchips);
	nand_info("activechip			0x%x\n", chip->activechip);
	nand_info("bbt_options 			0x%x\n", chip->bbt_options);
	nand_info("badblockpos 			0x%x\n", chip->badblockpos);
	nand_info("badblockbits 		0x%x\n", chip->badblockbits);
	nand_info("bbt_erase_shift 		0x%x\n", chip->bbt_erase_shift);
	nand_info("lbasize 			0x%x\n", chip->lbasize);
	nand_info("lbacnt 			0x%x\n\n", chip->lbacnt);
}

void nand_gpio_cfg_bit32(u64 addr, u32 field , u32 val)
{
	u32 tv = (unsigned int)(*(volatile u32*)(addr));
	tv &= ~(field);	tv |= val;
	(*(volatile u32*)(addr) = (u32)(tv));
}

#define NFI_GPIO_CFG_BIT32(reg,field,val)   nand_gpio_cfg_bit32(reg, field, val)

static void mtk_nfc_gpio_init(void)
{
/* Nand GPIO register define */
#define NFI_GPIO_BASE               (IO_PHYS+0x5000)
/* For NFI GPIO setting *//* NCLE */
#define NFI_GPIO_MODE1              (NFI_GPIO_BASE + 0x300)
/* NCEB1/NCEB0/NREB */
#define NFI_GPIO_MODE2              (NFI_GPIO_BASE + 0x310)
/* NRNB/NREB_C/NDQS_C */
#define NFI_GPIO_MODE3              (NFI_GPIO_BASE + 0x320)
#define NFI_GPIO_PUPD_CTRL0         (NFI_GPIO_BASE + 0xE00)
#define NFI_GPIO_PUPD_CTRL1         (NFI_GPIO_BASE + 0xE10)
#define NFI_GPIO_PUPD_CTRL2         (NFI_GPIO_BASE + 0xE20)
#define NFI_GPIO_PUPD_CTRL6         (NFI_GPIO_BASE + 0xE60)
/* Drving */
#define NFI_GPIO_DRV_MODE0          (NFI_GPIO_BASE + 0xD00)
#define NFI_GPIO_DRV_MODE6          (NFI_GPIO_BASE + 0xD60)
#define NFI_GPIO_DRV_MODE7          (NFI_GPIO_BASE + 0xD70)
//TDSEL,
#define NFI_GPIO_TDSEL6_EN          (NFI_GPIO_BASE + 0xB60)
#define NFI_GPIO_TDSEL7_EN          (NFI_GPIO_BASE + 0xB70)
//RDSEL, no need for 1.8V
#define NFI_GPIO_RDSEL1_EN          (NFI_GPIO_BASE + 0xC10)
#define NFI_GPIO_RDSELE_EN          (NFI_GPIO_BASE + 0xCE0)
#define NFI_GPIO_RDSELF_EN          (NFI_GPIO_BASE + 0xCF0)

	NFI_GPIO_CFG_BIT32(NFI_GPIO_MODE1, 0x7FFF,
		(1 << 0) | (1 << 3) | (1 << 6) | (2 << 9) | (2  << 12));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_MODE2, 0x7FFF,
		(2 << 0) | (2 << 3) | (2 << 6) | (2 << 9) | (2 << 12));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_MODE3, 0xFFF,
		(2 << 0) | (2 << 3) | (2 << 6) | (2 << 9));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_PUPD_CTRL6, (0xFFF << 4), (0x111 << 4));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_PUPD_CTRL1, 0xFFFF, 0x6666);
	NFI_GPIO_CFG_BIT32(NFI_GPIO_PUPD_CTRL2, 0xFFF, 0x616);
	NFI_GPIO_CFG_BIT32(NFI_GPIO_PUPD_CTRL0, 0xFFFF, 0x6666);
	/*only for 3.3V */
	NFI_GPIO_CFG_BIT32(NFI_GPIO_DRV_MODE6, (0xFF << 8), (0x11 << 8));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_DRV_MODE7, 0xFFF, 0x111);
	NFI_GPIO_CFG_BIT32(NFI_GPIO_TDSEL6_EN, (0xFF << 8), (0xAA << 8));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_TDSEL7_EN, 0xFFF, 0xAAA);
	/*only for 3.3v */
	NFI_GPIO_CFG_BIT32(NFI_GPIO_RDSEL1_EN, (0x3F << 6), (0xC << 6));
	NFI_GPIO_CFG_BIT32(NFI_GPIO_RDSELE_EN, 0x3F3F, 0xC0C);
	NFI_GPIO_CFG_BIT32(NFI_GPIO_RDSELF_EN, 0x3F3F, 0xC0C);
}

static void mtk_nfc_clk_init(void)
{
	u32 reg;
/* Nand clock select register define */
#define NFI_CLK_SEL1               (IO_PHYS+0x4)

	reg = (unsigned int)(*(volatile u32*)(NFI_CLK_SEL1));
	reg &= ~(0x7);
	/* 215MHz */
	reg |= 0x4;
	(*(volatile u32*)(NFI_CLK_SEL1) = (u32)(reg));
}

struct mtk_nand_chip *g_nand_chip;
int mtk_nfc_nand_chip_init(struct mtk_nand_chip **ext_nand)
{
	struct mtk_nfc *nfc;
	struct mtk_nfc_nand_chip *chip;
	struct mtk_nand_chip *nand;
	int ret = 0;

	nfc = (struct mtk_nfc *)nand_malloc(sizeof(*nfc));
	if (!nfc)
		return -ENOMEM;
	nand_memset(nfc, 0, sizeof(*nfc));
	nfc->regs = NAND_NFI_BASE;

	chip = (struct mtk_nfc_nand_chip *)nand_malloc(sizeof(*chip));
	if (!chip) {
		goto free_nfc;
		ret = -ENOMEM;
	}
	nand_memset(chip, 0, sizeof(*chip));
	
	nand_debug("nfc->regs:0x%lx nfc:0x%x chip:0x%x NAND_NFI_BASE:0x%x NFI_BASE:0x%x IO_PHYS:0x%x\n", 
		(u32)nfc->regs, (u32)nfc, (u32)chip, NAND_NFI_BASE, NFI_BASE, IO_PHYS);

#if 0
	/* register interrupt handler */
	mtk_nfc_request_irq(nfc);
#endif

	nand = &chip->chip;
	*ext_nand = nand;

	nand_set_controller_data(nand, nfc);

	nand->dev_ready = mtk_nfc_dev_ready;
	nand->wait_busy_irq = mtk_nfc_wait_busy_irq;
	nand->select_chip = mtk_nfc_select_chip;
	nand->write_byte = mtk_nfc_write_byte;
	nand->write_buf = mtk_nfc_write_buf;
	nand->read_byte = mtk_nfc_read_byte;
	nand->read_buf = mtk_nfc_read_buf;
	nand->cmd_ctrl = mtk_nfc_cmd_ctrl;

	nand->write_page_ecc_dma_irq = mtk_nfc_write_page_ecc_dma_irq;
	nand->write_page_ecc_dma_polling = mtk_nfc_write_page_ecc_dma_polling;
	nand->write_page_ecc_pio_irq = mtk_nfc_write_page_ecc_pio_irq;
	nand->write_page_ecc_pio_polling = mtk_nfc_write_page_ecc_pio_polling;
	nand->write_page_raw_dma_irq = mtk_nfc_write_page_raw_dma_irq;
	nand->write_page_raw_dma_polling = mtk_nfc_write_page_raw_dma_polling;
	nand->write_page_raw_pio_irq = mtk_nfc_write_page_raw_pio_irq;
	nand->write_page_raw_pio_polling = mtk_nfc_write_page_raw_pio_polling;
	nand->write_subpage_ecc_dma_irq = mtk_nfc_write_subpage_ecc_dma_irq;
	nand->write_subpage_ecc_dma_polling = mtk_nfc_write_subpage_ecc_dma_polling;
	nand->write_subpage_ecc_pio_irq = mtk_nfc_write_subpage_ecc_pio_irq;
	nand->write_subpage_ecc_pio_polling = mtk_nfc_write_subpage_ecc_pio_polling;

	nand->read_subpage_ecc_dma_irq = mtk_nfc_read_subpage_ecc_dma_irq;
	nand->read_subpage_ecc_dma_polling = mtk_nfc_read_subpage_ecc_dma_polling;
	nand->read_subpage_ecc_pio_irq = mtk_nfc_read_subpage_ecc_pio_irq;
	nand->read_subpage_ecc_pio_polling = mtk_nfc_read_subpage_ecc_pio_polling;
	nand->read_page_ecc_dma_irq = mtk_nfc_read_page_ecc_dma_irq;
	nand->read_page_ecc_dma_polling = mtk_nfc_read_page_ecc_dma_polling;
	nand->read_page_ecc_pio_irq = mtk_nfc_read_page_ecc_pio_irq;
	nand->read_page_ecc_pio_polling = mtk_nfc_read_page_ecc_pio_polling;
	nand->read_page_raw_dma_irq = mtk_nfc_read_page_raw_dma_irq;
	nand->read_page_raw_dma_polling = mtk_nfc_read_page_raw_dma_polling;
	nand->read_page_raw_pio_irq = mtk_nfc_read_page_raw_pio_irq;
	nand->read_page_raw_pio_polling = mtk_nfc_read_page_raw_pio_polling;

	mtk_nfc_gpio_init();
	mtk_nfc_clk_init();

#ifndef MT8518_NFI
	mtk_nfc_randomizer_init(nand);
#endif

	mtk_nfc_hw_init(nand);

	ret = mtk_nand_scan(nand, 1 /*MTK_NAND_MAX_NSELS*/);
	if (ret)
		goto free_chip;

	mtk_nfc_set_spare_per_sector(&chip->spare_per_sector, nand);
	mtk_nfc_set_fdm(&chip->fdm, nand);
	mtk_nfc_set_bad_mark_ctl(&chip->bad_mark, nand);
	mtk_nfc_set_timing(nand);

	ret = mtk_ecc_hw_init(&nfc->ecc);
	if (ret)
		goto free_chip;

	/* nfc->buffer = (u8 *)NAND_DRAM_BUF_NFCBUF_ADDR; */
	nfc->buffer = (u8 *)nand_memalign(4, nand->pagesize + nand->oobsize);
	if (!nfc->buffer) {
		ret = -ENOMEM;
		goto free_chip;
	}

	nand_lock_init(&nfc->lock);
	g_nand_chip = nand;
	nand_info("nand chip init done.\n");

	dump_nand_info(nand);

	return 0;

free_chip:
	nand_free(chip);
free_nfc:
	nand_free(nfc);

	return ret;
}


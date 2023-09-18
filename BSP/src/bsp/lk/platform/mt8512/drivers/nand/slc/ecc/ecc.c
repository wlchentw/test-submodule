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
#include "ecc.h"
#include "../slc_os.h"

#define ECC_IDLE_MASK       NAND_BIT(0)
#define ECC_IRQ_EN      NAND_BIT(0)
#define ECC_OP_ENABLE       (1)
#define ECC_OP_DISABLE      (0)

#define ECC_ENCCON      (0x00)
#define ECC_ENCCNFG     (0x04)
#define     ECC_CNFG_4BIT       (0)
#define     ECC_CNFG_6BIT       (1)
#define     ECC_CNFG_8BIT       (2)
#define     ECC_CNFG_10BIT      (3)
#define     ECC_CNFG_12BIT      (4)
#define     ECC_CNFG_14BIT      (5)
#define     ECC_CNFG_16BIT      (6)
#define     ECC_CNFG_18BIT      (7)
#define     ECC_CNFG_20BIT      (8)
#define     ECC_CNFG_22BIT      (9)
#define     ECC_CNFG_24BIT      (0xa)
#define     ECC_CNFG_28BIT      (0xb)
#define     ECC_CNFG_32BIT      (0xc)
#define     ECC_CNFG_36BIT      (0xd)
#define     ECC_CNFG_40BIT      (0xe)
#define     ECC_CNFG_44BIT      (0xf)
#define     ECC_CNFG_48BIT      (0x10)
#define     ECC_CNFG_52BIT      (0x11)
#define     ECC_CNFG_56BIT      (0x12)
#define     ECC_CNFG_60BIT      (0x13)
#define     ECC_CNFG_68BIT      (0x14)
#define     ECC_CNFG_72BIT      (0x15)
#define     ECC_CNFG_80BIT      (0x16)
#define     ECC_MODE_SHIFT      (5)
#define     ECC_MS_SHIFT        (16)
#define ECC_ENCDIADDR       (0x08)
#define ECC_ENCIDLE     (0x0c)
#define ECC_ENCSTA      (0x7c)
#define     ENC_IDLE    NAND_BIT(0)
#define ECC_ENCIRQ_EN       (0x80)
#define ECC_ENCIRQ_STA      (0x84)
#define     PG_IRQ_SEL      NAND_BIT(1)
#define ECC_PIO_DIRDY       (0x90)
#define     PIO_DI_RDY      (0x01)
#define ECC_PIO_DI      (0x94)
#define ECC_DECCON      (0x100)
#define ECC_DECCNFG     (0x104)
#define     DEC_EMPTY_EN        NAND_BIT(31)
#define     DEC_CON_SHIFT   (12)
#define ECC_DECDIADDR       (0x108)
#define ECC_DECIDLE     (0x10c)
#define ECC_DECENUM(x)      (0x114 + (x) * sizeof(u32))
#define     ERR_MASK        (0x1f)
#define ECC_DECDONE     (0x124)
#define ECC_DECIRQ_EN       (0x200)
#define ECC_DECIRQ_STA      (0x204)
#define ECC_DECFSM      (0x208)
#define     FSM_MASK    (0x3f3fff0f)
#define     FSM_IDLE    (0x01011101)
#define ECC_BYPASS      (0x20c)
#define     ECC_BYPASS_EN       NAND_BIT(0)
#ifdef MT8512_NFI
#define ECC_ENCPAR(x)      (0x300 + (x) * sizeof(u32))
#define ECC_DECEL(x)        (0x500 + (x) * sizeof(u32))
#else
#define ECC_ENCPAR(x)       (0x10 + (x) * sizeof(u32))
#define ECC_ENCPAR_EXT(x)   (0x300 + (x) * sizeof(u32))

#define ECC_DECEL(x)        (0x128 + (x) * sizeof(u32))
#define ECC_DECEL_EXT(x)    (0x400 + (x) * sizeof(u32))
#endif

#define ECC_TIMEOUT     (500000)

#define ECC_IDLE_REG(op)    ((op) == ECC_ENCODE ? ECC_ENCIDLE : ECC_DECIDLE)
#define ECC_CTL_REG(op)     ((op) == ECC_ENCODE ? ECC_ENCCON : ECC_DECCON)
#define ECC_IRQ_REG(op)     ((op) == ECC_ENCODE ? \
                    ECC_ENCIRQ_EN : ECC_DECIRQ_EN)


struct mtk_ecc {
	nand_lock_t lock;
	nand_completion_t done;

	u64 regs;
	u32 sectors;
	u8 *buffer; /* for buffer not aligned issue */
};

static inline void mtk_ecc_wait_ioready(struct mtk_ecc *ecc)
{
	if (!check_with_timeout((nand_readl(ecc->regs +  ECC_PIO_DIRDY) & PIO_DI_RDY), ECC_TIMEOUT))
		nand_err("ecc io not ready");
}

static void mtk_ecc_config(struct mtk_ecc *ecc, struct mtk_ecc_config *config)
{
	u32 ecc_bit = ECC_CNFG_4BIT, dec_sz, enc_sz;
	u32 reg;

	switch (config->strength) {
		case 4:
			ecc_bit = ECC_CNFG_4BIT;
			break;
		case 6:
			ecc_bit = ECC_CNFG_6BIT;
			break;
		case 8:
			ecc_bit = ECC_CNFG_8BIT;
			break;
		case 10:
			ecc_bit = ECC_CNFG_10BIT;
			break;
		case 12:
			ecc_bit = ECC_CNFG_12BIT;
			break;
		case 14:
			ecc_bit = ECC_CNFG_14BIT;
			break;
		case 16:
			ecc_bit = ECC_CNFG_16BIT;
			break;
		case 18:
			ecc_bit = ECC_CNFG_18BIT;
			break;
		case 20:
			ecc_bit = ECC_CNFG_20BIT;
			break;
		case 22:
			ecc_bit = ECC_CNFG_22BIT;
			break;
		case 24:
			ecc_bit = ECC_CNFG_24BIT;
			break;
		case 28:
			ecc_bit = ECC_CNFG_28BIT;
			break;
		case 32:
			ecc_bit = ECC_CNFG_32BIT;
			break;
		case 36:
			ecc_bit = ECC_CNFG_36BIT;
			break;
		case 40:
			ecc_bit = ECC_CNFG_40BIT;
			break;
		case 44:
			ecc_bit = ECC_CNFG_44BIT;
			break;
		case 48:
			ecc_bit = ECC_CNFG_48BIT;
			break;
		case 52:
			ecc_bit = ECC_CNFG_52BIT;
			break;
		case 56:
			ecc_bit = ECC_CNFG_56BIT;
			break;
		case 60:
			ecc_bit = ECC_CNFG_60BIT;
			break;
		case 68:
			ecc_bit = ECC_CNFG_68BIT;
			break;
		case 72:
			ecc_bit = ECC_CNFG_72BIT;
			break;
		case 80:
			ecc_bit = ECC_CNFG_80BIT;
			break;
		default:
			nand_err("invalid strength %d, default to 4 bits",
			         config->strength);
			break;
	}

	if (config->op == ECC_ENCODE) {
		/* configure ECC encoder (in bits) */
		enc_sz = config->len << 3;

		reg = ecc_bit | (config->mode << ECC_MODE_SHIFT);
		reg |= (enc_sz << ECC_MS_SHIFT);
		nand_writel(reg, ecc->regs + ECC_ENCCNFG);

		if (config->mode == ECC_DMA_MODE) {
			if (config->addr & 0x3)
				nand_err("ecc encode address is not 4B aligned !!", config->addr);
			nand_writel(config->addr, ecc->regs + ECC_ENCDIADDR);
		}

	} else {
		/* configure ECC decoder (in bits) */
		dec_sz = (config->len << 3) +
		         config->strength * ECC_PARITY_BITS;

		reg = ecc_bit | (config->mode << ECC_MODE_SHIFT);
		reg |= (dec_sz << ECC_MS_SHIFT) | (config->deccon << DEC_CON_SHIFT);
		reg |= DEC_EMPTY_EN;
		nand_writel(reg, ecc->regs + ECC_DECCNFG);

		if (config->mode == ECC_DMA_MODE) {
			if (config->addr & 0x3)
				nand_err("ecc decode address is not 4B aligned !!", config->addr);
			nand_writel(config->addr, ecc->regs + ECC_DECDIADDR);
		}

		if (config->sectors)
			ecc->sectors = 1 << (config->sectors - 1);
	}
}

static inline void mtk_ecc_wait_idle(struct mtk_ecc *ecc,
                                     enum mtk_ecc_operation op)
{
	if (!check_with_timeout(nand_readl(ecc->regs + ECC_IDLE_REG(op)) & ECC_IDLE_MASK,
	                        ECC_TIMEOUT))
		nand_err("%s NOT idle", op == ECC_ENCODE ? "encoder" : "decoder");
}

#ifdef MTK_NAND_IRQ_EN
static int mtk_ecc_irq_wait(struct mtk_ecc *ecc, u32 timeout)
{
	int ret;

	ret = nand_wait_for_completion_timeout(&ecc->done, timeout);
	if (ret != 0) {
		nand_err("failed to get completion timeout");
		return ret;
	}

	return 0;
}

static enum handler_return mtk_ecc_interrupt_handler(void *arg)
{
	struct mtk_ecc *ecc = arg;
	enum mtk_ecc_operation op;
	u32 dec, enc;

	dec = nand_readw(ecc->regs + ECC_DECIRQ_STA) & ECC_IRQ_EN;
	if (dec) {
		op = ECC_DECODE;
		dec = nand_readw(ecc->regs + ECC_DECDONE);
		if (dec & ecc->sectors) {
			ecc->sectors = 0;
			nand_complete(&ecc->done);
		} else {
			return NAND_IRQ_NONE;
		}
	} else {
		enc = nand_readl(ecc->regs + ECC_ENCIRQ_STA) & ECC_IRQ_EN;
		if (enc) {
			op = ECC_ENCODE;
			nand_complete(&ecc->done);
		} else {
			return NAND_IRQ_NONE;
		}
	}

	nand_writel(0, ecc->regs + ECC_IRQ_REG(op));

	return NAND_IRQ_HANDLED;
}

static int mtk_ecc_request_irq(struct mtk_ecc *ecc)
{
	nand_init_completion(&ecc->done);
	mtk_nand_request_irq(NAND_NFIECC_IRQ_BIT_ID, &mtk_ecc_interrupt_handler, ecc);

	return 0;
}
#endif

int mtk_ecc_hw_init(struct mtk_ecc **ext_ecc)
{
	struct mtk_ecc *ecc;
	u32 reg;
	int ret = 0;

	ecc = (struct mtk_ecc *)nand_malloc(sizeof(*ecc));
	if (!ecc)
		return -ENOMEM;

	nand_memset(ecc, 0, sizeof(*ecc));
#if 1
	ecc->buffer = (u8 *)nand_memalign(4, ECC_MAX_CODESIZE);
	if (!ecc->buffer) {
		ret = -ENOMEM;
		nand_err("failed to malloc ecc temp buffer %d", ECC_MAX_CODESIZE);
		goto free_ecc;
	}
#endif
	*ext_ecc = ecc;

	ecc->regs = NAND_NFIECC_BASE;

	mtk_ecc_wait_idle(ecc, ECC_ENCODE);
	nand_writew(ECC_OP_DISABLE, ecc->regs + ECC_ENCCON);

	mtk_ecc_wait_idle(ecc, ECC_DECODE);
	nand_writel(ECC_OP_DISABLE, ecc->regs + ECC_DECCON);

	nand_lock_init(&ecc->lock);

#ifdef MTK_NAND_IRQ_EN
	/* register interrupt handler */
	mtk_ecc_request_irq(ecc);
#endif
	/* disable ecc bypass */
	reg = nand_readl(ecc->regs + ECC_BYPASS);
	reg &= ~ECC_BYPASS_EN;
	nand_writel(reg, ecc->regs + ECC_BYPASS);

	return 0;

free_ecc:
	nand_free(ecc);
	return ret;
}


int mtk_ecc_enable(struct mtk_ecc *ecc, struct mtk_ecc_config *config, int polling)
{
	enum mtk_ecc_operation op = config->op;

	nand_lock(&ecc->lock);

	mtk_ecc_wait_idle(ecc, op);
	mtk_ecc_config(ecc, config);
	nand_writew(ECC_OP_ENABLE, ecc->regs + ECC_CTL_REG(op));

	if (!polling) {
		nand_writew(ECC_IRQ_EN, ecc->regs + ECC_IRQ_REG(op));
	}

	return 0;
}

void mtk_ecc_disable(struct mtk_ecc *ecc)
{
	enum mtk_ecc_operation op = ECC_ENCODE;

	/* find out the running operation */
	if (nand_readw(ecc->regs + ECC_CTL_REG(op)) != ECC_OP_ENABLE)
		op = ECC_DECODE;

	/* disable it */
	mtk_ecc_wait_idle(ecc, op);
	nand_writew(0, ecc->regs + ECC_IRQ_REG(op));
	nand_writew(ECC_OP_DISABLE, ecc->regs + ECC_CTL_REG(op));

	nand_unlock(&ecc->lock);
}

void mtk_ecc_get_stats(struct mtk_ecc *ecc, struct mtk_ecc_stats *stats,
                       int sectors)
{
	u32 offset, i, err;
	u32 bitflips = 0;

	stats->corrected = 0;
	stats->failed = 0;

	for (i = 0; i < sectors; i++) {
		offset = (i >> 2);
		err = nand_readl(ecc->regs + ECC_DECENUM(offset));
		err = err >> ((i % 4) * 8);
		err &= ERR_MASK;
		if (err == ERR_MASK) {
			/* uncorrectable errors */
			stats->failed++;
			nand_err("sector %d is uncorrect", i);
			continue;
		}

		stats->corrected += err;
		bitflips = max(bitflips, err);
	}

	stats->bitflips = bitflips;
}

int mtk_ecc_wait_done(struct mtk_ecc *ecc, enum mtk_ecc_operation op, int polling)
{
	int ret = 0;

	if (!polling) {
#ifdef MTK_NAND_IRQ_EN		
		ret = mtk_ecc_irq_wait(ecc, ECC_TIMEOUT);
		if (!ret)
			nand_err("mtk_ecc_wait_done timeout");
#endif
		return -ETIMEDOUT;
	} else {
		if (op == ECC_ENCODE) {
			if (!check_with_timeout((nand_readl(ecc->regs + ECC_ENCSTA) & ENC_IDLE), ECC_TIMEOUT)) {
				nand_err("encoder timeout");
				return -ETIMEDOUT;
			}
		} else {
			if (!check_with_timeout((nand_readw(ecc->regs + ECC_DECDONE) & ecc->sectors), ECC_TIMEOUT)) {
				nand_err("decoder timeout");
				return -ETIMEDOUT;
			}
		}
	}

	return 0;
}

int mtk_ecc_wait_decode_fsm_idle(struct mtk_ecc *ecc)
{
	/* decode done does not stands for ecc all work done.
	 * we need check syn, bma, chien, autoc all idle.
	 * just check it when ECC_DECCNFG[13:12] is 3, which means auto correct.*/
	if (!check_with_timeout(((nand_readl(ecc->regs + ECC_DECFSM) & FSM_MASK) == FSM_IDLE), ECC_TIMEOUT)) {
		nand_err("decode fsm(0x%x) is not idle", nand_readl(ecc->regs + ECC_DECFSM));
		return -ETIMEDOUT;
	}

	return 0;
}

int mtk_ecc_encode(struct mtk_ecc *ecc, struct mtk_ecc_config *config,
                   u8 *data, u32 bytes, int polling)
{
	u32 addr = (u32)data;
	u8 *p;
	u8 *buf = data;
	u32 len, i, val = 0;
	int ret = 0;

	/* encoder memory address should be 4B aligned */
	if ((config->mode == ECC_DMA_MODE) && (addr & 0x3)) {
		/* buf =(u8 *)NAND_DRAM_BUF_ECCDE_ADDR; */
		buf = ecc->buffer;
		nand_memcpy(buf, data, bytes);
	}

	addr = nand_kvaddr_to_paddr(buf);

	if (config->mode == ECC_DMA_MODE)
		nand_dma_map(buf, bytes, true, NULL);

	config->op = ECC_ENCODE;
	config->addr = addr;
	config->len = bytes;
	ret = mtk_ecc_enable(ecc, config, polling);
	if (ret)
		goto freebuf;

	if (config->mode == ECC_PIO_MODE) {
		for (i = 0; i < ((config->len + 3) >> 2); i++) {
			mtk_ecc_wait_ioready(ecc);
			nand_writel(*((u32 *)data + i), ecc->regs + ECC_PIO_DI);
		}
	}

	ret = mtk_ecc_wait_done(ecc, ECC_ENCODE, polling);
	if (ret)
		goto timeout;

	mtk_ecc_wait_idle(ecc, ECC_ENCODE);

	/* Program ECC bytes to OOB: per sector oob = FDM + ECC + SPARE */
	len = (config->strength * ECC_PARITY_BITS + 7) >> 3;
	p = data + bytes;

	/* write the parity bytes generated by the ECC back to the OOB region */
	for (i = 0; i < len; i++) {
		if ((i % 4) == 0) {
#ifdef MT8512_NFI
			val = nand_readl(ecc->regs + ECC_ENCPAR(i / 4));
#else
			if (i < 108)
				val = nand_readl(ecc->regs + ECC_ENCPAR(i / 4));
			else
				val = nand_readl(ecc->regs + ECC_ENCPAR_EXT((i / 4) - 27));
#endif
		}
		p[i] = (val >> ((i % 4) * 8)) & 0xff;
	}
timeout:
	mtk_ecc_disable(ecc);
freebuf:
	if (config->mode == ECC_DMA_MODE) {
		nand_dma_unmap(buf, bytes, false, NULL);
	}

	return ret;
}

int mtk_ecc_decode(struct mtk_ecc *ecc, struct mtk_ecc_config *config,
                   u8 *data, u32 len, int polling)
{
	struct mtk_ecc_stats stats;
	u8* buf = data;
	u32 addr = (u32)data, decodesize, i;
	int ret;

	decodesize = len + ((config->strength * ECC_PARITY_BITS + 7) >> 3);
	if ((decodesize & 0x3)
	        || ((config->mode == ECC_DMA_MODE) && (addr & 0x3))) {
		decodesize += 4 - (decodesize & 0x3);
		/* buf = (u8 *)NAND_DRAM_BUF_ECCEN_ADDR; */
		buf = ecc->buffer;
	}
	if (config->mode == ECC_DMA_MODE)
		nand_dma_map(buf, decodesize, false, NULL);

	addr = nand_kvaddr_to_paddr(buf);

	config->op = ECC_DECODE;
	config->addr = addr;
	config->len = len;
	ret = mtk_ecc_enable(ecc, config, polling);
	if (ret)
		goto freebuf;

	if (config->mode == ECC_PIO_MODE) {
		for (i = 0; i < (decodesize >> 2); i++) {
			mtk_ecc_wait_ioready(ecc);
			*((u32 *)buf + i) = nand_readl(ecc->regs + ECC_PIO_DI);
		}
	}

	stats.bitflips = 0;
	ret = mtk_ecc_cpu_correct(ecc, &stats, buf, 0, polling);
	if (ret)
		goto disecc;

	if (config->mode == ECC_DMA_MODE)
		nand_dma_unmap(buf, decodesize, false, NULL);

	if (buf != data)
		nand_memcpy(data, buf, len);

disecc:
	mtk_ecc_disable(ecc);

freebuf:

	return ret;
}

int mtk_ecc_cpu_correct(struct mtk_ecc *ecc, struct mtk_ecc_stats *stats, u8 *data, u32 sector, int polling)
{
	u32 err, offset, i;
	u32 loc, byteloc, bitloc;
	int ret;

	ecc->sectors = 1 << sector;
	ret = mtk_ecc_wait_done(ecc, ECC_DECODE, polling);
	if (ret)
		return ret;

	stats->corrected = 0;
	stats->failed = 0;

	offset = (sector >> 2);
	err = nand_readl(ecc->regs + ECC_DECENUM(offset));
	err = err >> ((sector % 4) * 8);
	err &= ERR_MASK;
	if (err == ERR_MASK) {
		/* uncorrectable errors */
		stats->failed++;
		return 0;
	}

	stats->corrected += err;
	stats->bitflips = max(stats->bitflips, err);

	for (i = 0; i < err; i++) {
#ifdef MT8512_NFI
		loc = nand_readl(ecc->regs + ECC_DECEL(i >> 1));
#else
		if (i < 60)
			loc = nand_readl(ecc->regs + ECC_DECEL(i >> 1));
		else
			loc = nand_readl(ecc->regs + ECC_DECEL_EXT((i >> 1) - 30));
#endif
		loc >>= ((i & 0x1) << 4);
		byteloc = loc >> 3;
		bitloc = loc & 0x7;
		data[byteloc] ^= (1 << bitloc);
	}

	return 0;
}

void mtk_ecc_adjust_strength(u32 *p)
{
	u32 ecc[] = {4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36,
	             40, 44, 48, 52, 56, 60, 68, 72, 80
	            };
	int i;

	for (i = 0; i < sizeof(ecc) / sizeof(u32); i++) {
		if (*p <= ecc[i]) {
			if (!i)
				*p = ecc[i];
			else if (*p != ecc[i])
				*p = ecc[i - 1];
			return;
		}
	}

	*p = ecc[sizeof(ecc) / sizeof(u32) - 1];
}


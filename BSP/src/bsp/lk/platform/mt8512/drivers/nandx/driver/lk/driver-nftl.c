#include <lib/bio.h>
#include <lib/partition.h>
#include <malloc.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <platform/nand.h>
#include <platform/pll.h>
#include <lib/nftl.h>
#include <kernel/mutex.h>

#include "nandx_core.h"
#include "nandx_util.h"
#include "bbt.h"

typedef int (*func_nandx_operation)(u8 *, u8 *, u64, size_t);
struct nandx_info nandxi;
static mutex_t nandx_lock;

static ssize_t erase_operation(off_t offset, size_t len, bool force_erase)
{
	u32 block_size = nandxi.block_size;
	ssize_t bytes_erase = 0;
	int ret;

	while (len) {
		if (bbt_is_bad(&nandxi, offset) && (!force_erase)) {
			pr_warn("block(0x%x) is bad, not erase\n", offset);
		} else {
			ret = nandx_erase(offset, block_size);
			if (ret < 0)
				pr_warn("erase fail at blk %d, force:%d\n",
					offset, force_erase);
			else
				bytes_erase += block_size;
		}

		offset += block_size;
		len -= block_size;
	}

	pr_debug("%s end, bytes_erase:0x%x\n", __func__, bytes_erase);
	return bytes_erase;
}

static ssize_t nand_force_erase_all()
{
	ssize_t ret;

	mutex_acquire(&nandx_lock);
	ret = erase_operation(0, nandxi.total_size, true);
	mutex_release(&nandx_lock);
	return ret;
}

static ssize_t nand_erase(struct nftl_info *nftl, off_t offset, size_t len)
{
	ssize_t ret;

	mutex_acquire(&nandx_lock);
	ret = erase_operation(offset, len, false);
	mutex_release(&nandx_lock);
	return ret;
}

static ssize_t rw_operation(void *buf, off_t offset, size_t len, bool read)
{
	struct nandx_split64 split = {0};
	func_nandx_operation operation;
	int ret, i, pages;
	size_t read_len = 0;
	u8 *lbuf = (u8 *)buf;
	u64 val;

	operation = read ? nandx_read : nandx_write;

	nandx_split(&split, offset, len, val, nandxi.page_size);

	if (split.head_len) {
		ret = operation(lbuf, NULL, split.head, split.head_len);
		lbuf += split.head_len;
	}

	if (split.body_len) {
		pages = div_down(split.body_len, nandxi.page_size);
		for (i = 0; i < pages; i++) {
			operation(lbuf + i * nandxi.page_size , NULL,
				  split.body + i * nandxi.page_size,
				  nandxi.page_size);
		}

		lbuf += split.body_len;
	}

	if (split.tail_len) {
		operation(lbuf, NULL, split.tail, split.tail_len);
	}

	return len;
}

static ssize_t nand_read(struct nftl_info *nftl, void *buf,
			off_t offset, size_t len)
{
	ssize_t ret;

	mutex_acquire(&nandx_lock);
	ret = rw_operation(buf, offset, len, true);
	mutex_release(&nandx_lock);
	return ret;
}

static ssize_t nand_write(struct nftl_info *nftl, void *buf,
			off_t offset, size_t len)
{
	ssize_t ret;

	mutex_acquire(&nandx_lock);
	ret = rw_operation(buf, offset, len, false);
	mutex_release(&nandx_lock);
	return ret;
}

static int nand_ioctl(struct nftl_info *info, int request, void *argp)
{
	int ret;

	switch (request) {
	case NAND_IOCTL_FORCE_FORMAT_ALL:
		ret = nand_force_erase_all();
		break;

	default:
		return -EOPNOTSUPP;
	}

	return ret;
}

static int nand_is_bad_block(struct nftl_info *nftl, u32 page)
{
	int ret;

	mutex_acquire(&nandx_lock);
	ret = bbt_is_bad(&nandxi, (off_t)page * nandxi.page_size);
	mutex_release(&nandx_lock);
	return ret;
}

static int nand_block_markbad(struct nftl_info *info, u32 page)
{
	int ret = 0;

	if(bbt_is_bad(&nandxi, (off_t)page * nandxi.page_size)) {
		return 0;
	} else {
		/* Mark block bad in BBT */
		ret = bbt_mark_bad(&nandxi, (off_t)page * nandxi.page_size);
	}

	return ret;
}

static void nand_gpio_init(void)
{
	nandx_set_bits32(GPIO_BASE + 0x250, 0xFFF << 18, 0x6DB << 18);
	nandx_set_bits32(GPIO_BASE + 0x260, 0x7 << 6 | 0x7, 0x3 << 6 | 0x3);

	nandx_set_bits32(GPIO_BASE + 0x740, 0xf << 16, 1 << 16);
	nandx_set_bits32(GPIO_BASE + 0x750, 0xf | (0xF << 8), 1 | (1 << 8));
}

static u32 nand_clock_init(void)
{
	/* use default clk 26Mhz as temporary solution
	 * should return correct value
	 */
	return 26* 1000 * 1000;
}

static void nand_hard_reset(void)
{
	u32 val;

	val = readl(INFRACFG_AO_BASE + 0x130);
	val |= BIT(15);
	writel(val, INFRACFG_AO_BASE + 0x130);

	nandx_udelay(5);

	val = readl(INFRACFG_AO_BASE + 0x134);
	val |= BIT(15);
	writel(val, INFRACFG_AO_BASE + 0x134);
}

int nand_init_device()
{
	struct nfi_resource res = {
			NANDX_MT8512, NULL,
			(void *)NFIECC_BASE, 0,
			(void *)NFI_BASE, 0,
			26000000, NULL, 0, 32
	};
	struct nftl_info *nftl;
	int ret = 0, arg = 1;

	pr_debug("%s @ %d ...\n", __func__, __LINE__);

	nand_gpio_init();

	res.clock_1x = nand_clock_init();

	nand_hard_reset();

	ret = nandx_init(&res);
	if (ret) {
		pr_err("nandx init error (%d)!\n", ret);
		return ret;
	}

	nandx_ioctl(NFI_CTRL_DMA, &arg);
	nandx_ioctl(NFI_CTRL_ECC, &arg);
	nandx_ioctl(NFI_CTRL_BAD_MARK_SWAP, &arg);
	nandx_ioctl(CORE_CTRL_NAND_INFO, &nandxi);

	ret = scan_bbt(&nandxi);
	if (ret) {
		pr_err("bbt init error (%d)!\n", ret);
		return ret;
	}

	nftl = nftl_add_master("nand0");
	if (!nftl)
		return -ENOMEM;

	nftl->erase_size = nandxi.block_size;
	nftl->write_size = nandxi.page_size;
	nftl->total_size = nandxi.total_size;
	nftl->block_isbad = nand_is_bad_block;
	nftl->block_markbad = nand_block_markbad;
	nftl->erase = nand_erase;
	nftl->read = nand_read;
	nftl->write = nand_write;
	nftl->ioctl = nand_ioctl;

	ret = nftl_mount_bdev(nftl);
	if(ret)
		pr_err("nftl mount bdev fail.\n");

	mutex_init(&nandx_lock);

	return ret;
}


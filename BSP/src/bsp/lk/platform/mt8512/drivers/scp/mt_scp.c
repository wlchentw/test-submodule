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
#include <app.h>
#include <reg.h>
#include <errno.h>
#include <string.h>
#include <lib/bio.h>
#include <lib/mempool.h>
#include <platform/mt8512.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_scp.h>

void start_scpsys(void)
{
	u32 reg;
	reg = readl(SCP_BASE_CFG + CMSYS_CLKGAT_CTL);
	reg |= CPUCK_EN;
	writel(reg, SCP_BASE_CFG + CMSYS_CLKGAT_CTL);

	reg = readl(SCP_BASE_CFG + CMSYS_RESET_CTL);
	reg |= CPU_RST_SW;
	writel(reg, SCP_BASE_CFG + CMSYS_RESET_CTL);
}

void stop_scpsys(void)
{
	u32 reg;

	reg = readl(SCP_BASE_CFG + CMSYS_RESET_CTL);
	reg &= ~CPU_RST_SW;
	writel(reg, SCP_BASE_CFG + CMSYS_RESET_CTL);

	reg = readl(SCP_BASE_CFG + CMSYS_CLKGAT_CTL);
	reg &= ~CPUCK_EN;
	writel(reg, SCP_BASE_CFG + CMSYS_CLKGAT_CTL);

}

static int get_scpsys(const char *name, union fm_hdr_t *fm_hdr, void *buf)
{
    bdev_t *bdev;
    size_t totalsize; //sram size

    bdev = bio_open_by_label(name);
    if (!bdev) {
        dprintf(CRITICAL, "Partition [%s] is not exist.\n", name);
        return -ENODEV;
    }

    totalsize = bio_read(bdev, fm_hdr, 0, sizeof(union fm_hdr_t));
    if (totalsize <= 0) {
        dprintf(CRITICAL, "error reading scp header\n");
        return totalsize;
    }

    if (fm_hdr->info.magic != PART_MAGIC || fm_hdr->info.dsize > MAX_SCPSYS_SIZE) {
	printf("scp: firmware information incorrect!\n");
			return -EINVAL;
    }

    totalsize = bio_read(bdev, buf, sizeof(union fm_hdr_t),
                         fm_hdr->info.dsize);
    if (totalsize <= 0) {
        dprintf(CRITICAL, "error reading scp data\n");
        return totalsize;
    }

    dprintf(CRITICAL, "scp: load scp image success!\n");
    return 0;
}

int load_scpsys(void)
{
    int err = 0;
    void *buf = mempool_alloc(MAX_SCPSYS_SIZE, MEMPOOL_ANY);
    union fm_hdr_t *fm_hdr = mempool_alloc(sizeof(union fm_hdr_t), MEMPOOL_ANY);

    if (!buf || !fm_hdr)
    {
        dprintf(CRITICAL, "scp: fail to alloc memory!\n");
        err = -1;
        goto done;
    }

    //load cmsys from flash to fit_data->buf.
    err = get_scpsys(SCPSYS_PART_NAME, fm_hdr, buf);
    if (err) {
        dprintf(CRITICAL, "scp: fail to load scp image!\n");
        err = -1;
        goto done;
    }
    if (fm_hdr->info.dsize > (SCP_BASE_CFG - SCP_BASE_SRAM))
    {
        dprintf(CRITICAL, "scp: fail fm_hdr->info.dsize is overflow!\n");
        err = -1;
        goto done;
    }
    memcpy(SCP_BASE_SRAM, buf, fm_hdr->info.dsize);

done:
    mempool_free(buf);
    mempool_free(fm_hdr);
    return err;
}


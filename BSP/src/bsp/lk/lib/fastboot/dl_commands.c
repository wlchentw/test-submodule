/*
 * Copyright (c) 2016 MediaTek Inc.
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
#include <arch/ops.h>
#include <ctype.h>
#include <debug.h>
#include <dev/timer/arm_generic.h>
#include <dev/udc.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <lib/bio.h>
#include <lib/partition.h>
#include <platform.h>
#include <platform/mt_reg_base.h>
#include <platform/mtk_wdt.h>
#include <stdlib.h>
#include <string.h>
#include <target.h>
#include "dl_commands.h"
#include "fastboot.h"
#include "fit.h"
#include "sparse_format.h"
#include <lib/mempool.h>
#include <libfdt.h>
#include <errno.h>
#if (MT8518 || MT8512)
#include <platform/mmc_rpmb.h>
#endif
#include <platform/nand.h>
#include <platform/bd71828.h>
#include <platform/ntx_hw.h>
#include "version.h"

#define MODULE_NAME "FASTBOOT_DOWNLOAD"
#define MAX_RSP_SIZE 64

extern void *download_base;
extern unsigned download_max;
extern unsigned download_size;
extern unsigned fastboot_state;
extern void *kernel_buf;
extern void *tz_buf;
/*LXO: !Download related command*/

#define ROUND_TO_PAGE(x,y) (((x) + (y)) & (~(y)))
#define INVALID_PTN -1

lk_bigtime_t start_time_ms;
#define TIME_STAMP current_time_hires()
#define TIME_START {start_time_ms = current_time_hires();}
#define TIME_ELAPSE (current_time_hires() - start_time_ms)

extern int usb_write(void *buf, unsigned len);
extern int usb_read(void *buf, unsigned len);


extern struct fastboot_var *varlist;


//#define ARGPARSE_DEBUG		1
#define  ARGBUF_SIZE 1024
static char gcArgsBufA[ARGBUF_SIZE];
static int _parse_args(const char *arg,char **ppcArg,int iArgs)
{
	int iChk=0,iArgIdx=0,i;
	char *pc;

	for(i=0;i<iArgs;i++) {
		ppcArg[i] = 0;
	}

	for (pc=arg,i=0;;pc++,i++ ) {
		if(i>=ARGBUF_SIZE) {
			dprintf(ALWAYS,"%s(%d) arg size > %d \n",__func__,__LINE__,ARGBUF_SIZE);
			iChk=-1;
			break;
		}
		
		if(iArgIdx>=iArgs) {
			iChk=iArgIdx;
			dprintf(ALWAYS,"%s(%d) %d args parsed ready ! \n",__func__,__LINE__,iChk);
			break;
		}

		gcArgsBufA[i] = *pc;
#ifdef ARGPARSE_DEBUG //[
		dprintf(ALWAYS,"%c",gcArgsBufA[i]);
#endif //]ARGPARSE_DEBUG

		if(*pc=='\0') {
			iChk=iArgIdx;
			break;
		}
		else if( (*pc==' ') || (*pc=='\t') ) {
			gcArgsBufA[i] = '\0';
			if(0!=ppcArg[iArgIdx]) {
				iArgIdx++;
			}
		}
		else {
			if(0==ppcArg[iArgIdx]) {
				ppcArg[iArgIdx] = &gcArgsBufA[i];
			}
		}
	}

#ifdef ARGPARSE_DEBUG //[
	dprintf(ALWAYS,"\n");

	for(i=0;i<iArgs;i++) {
		dprintf(ALWAYS,"arg[%d]=%s\n",i,ppcArg[i]);
	}
#endif //]ARGPARSE_DEBUG

	return iChk;
}




void cmd_getvar(const char *arg, void *data, unsigned sz)
{
    struct fastboot_var *var;
    char response[MAX_RSP_SIZE];

    if (!strcmp(arg, "all")) {
        for (var = varlist; var; var = var->next) {
            snprintf(response, MAX_RSP_SIZE,"\t%s: %s", var->name, var->value);
            fastboot_info(response);
        }
        fastboot_okay("Done!!");
        return;
    }
    for (var = varlist; var; var = var->next) {
        if (!strcmp(var->name, arg)) {
            fastboot_okay(var->value);
            return;
        }
    }
    fastboot_okay("");
}

void cmd_reboot(const char *arg, void *data, unsigned sz)
{
    dprintf(ALWAYS, "rebooting the device\n");
    fastboot_okay("");
    mtk_arch_reset(1); /* bypass pwr key when reboot */
}

void cmd_reboot_bootloader(const char *arg, void *data, unsigned sz)
{
    dprintf(ALWAYS, "rebooting the device to bootloader\n");
    fastboot_okay("");
    set_clr_fastboot_mode(true);
    mtk_arch_reset(1); /* bypass pwr key when reboot */
}

void cmd_reboot_recovery(const char *arg, void *data, unsigned sz)
{
    dprintf(ALWAYS, "rebooting the device to recovery\n");
    fastboot_okay("");
    set_clr_recovery_mode(true);
    mtk_arch_reset(1); /* bypass pwr key when reboot */
}

static void fastboot_fail_wrapper(const char *msg)
{
    fastboot_fail(msg);
}

static void fastboot_ok_wrapper(const char *msg, unsigned data_size)
{
    fastboot_okay("");
}

void cmd_download(const char *arg, void *data, unsigned sz)
{
    char response[MAX_RSP_SIZE];
    char *endptr;
    unsigned len = strtoul(arg, &endptr, 16);
    int r;

    download_size = 0;
    if (len > download_max) {
        fastboot_fail_wrapper("data is too large");
        return;
    }

    snprintf(response, MAX_RSP_SIZE, "DATA%08x", len);
    if (usb_write(response, strlen(response)) < 0) {
        return;
    }
    TIME_START;
    r = usb_read(download_base, len);
    if ((r < 0) || ((unsigned) r != len)) {
        fastboot_fail_wrapper("Read USB error");
        fastboot_state = STATE_ERROR;
        return;
    }
    download_size = len;

    fastboot_ok_wrapper("USB Transmission OK", len);
}

void cmd_flash_img(const char *arg, void *data, unsigned sz)
{
    bdev_t *bdev;
    int ret = 0;
    off_t round_size;

    if (!strcmp(arg, "download:tz")) {
        cmd_download_tz(arg, data, sz);
    } else if (!strcmp(arg, "download:boot")) {
        cmd_download_boot(arg, data, sz);
    } else {
		if(strcmp(arg,"vcom")==0){
			printf("[NTX] label vcom\n");
			bdev = bio_open_by_label("hwcfg");
		}
		else if(strcmp(arg,"sn")==0){
			printf("[NTX] label sn\n");
			bdev = bio_open_by_label("hwcfg");
        }

        else
            bdev = bio_open_by_label(arg);
        if (!bdev) {
            bdev = bio_open(arg);
            if (!bdev) {
                fastboot_fail_wrapper("Partition is not exist.");
                return;
            }
        }

        round_size = (off_t)ROUND_TO_PAGE(sz,bdev->block_size-1);
        dprintf(INFO, "src size is %lld, dst size is %lld\n", round_size, bdev->total_size);
        if (bdev->total_size && round_size > bdev->total_size) {
            fastboot_fail_wrapper("Image size is too large.");
            goto closebdev;
        }

        ret = partition_update(bdev->name,arg, 0x0, data, sz);
        if (ret < 0) {
            fastboot_fail_wrapper("Flash write failure.");
            goto closebdev;
        }

        ret = partition_publish(bdev->name, 0x0);
        if (ret > 0)
            bio_dump_devices();

        fastboot_okay("");
closebdev:
        bio_close(bdev);
    }
    return;
}

void cmd_flash_sparse_img(const char *arg, void *data, unsigned sz)
{
    bdev_t *bdev;
    size_t ret = 0;
    unsigned int chunk;
    unsigned int chunk_data_sz;
    uint32_t *fill_buf = NULL;
    uint32_t fill_val, pre_fill_val, erase_val;
    uint32_t chunk_blk_cnt = 0;
    uint32_t i;
    sparse_header_t *sparse_header;
    chunk_header_t *chunk_header;
    uint32_t total_blocks = 0;
    unsigned long long size = 0;

    bdev = bio_open_by_label(arg);
    if (!bdev) {
        fastboot_fail_wrapper("Partition is not exist.");
        return;
    }

    size = bdev->total_size;
    if (!size) {
        fastboot_fail_wrapper("Size is uncorrect.");
        goto closebdev;
    }

    /* Read and skip over sparse image header */
    sparse_header = (sparse_header_t *) data;
    dprintf(ALWAYS, "Image size span 0x%llx, partition size 0x%llx\n", (unsigned long long)sparse_header->total_blks*sparse_header->blk_sz, size);
    if ((unsigned long long)sparse_header->total_blks*sparse_header->blk_sz > size) {
        fastboot_fail("sparse image size span overflow.");
        goto closebdev;
    }

    data += sparse_header->file_hdr_sz;
    if (sparse_header->file_hdr_sz > sizeof(sparse_header_t)) {
        /* Skip the remaining bytes in a header that is longer than
         * we expected.
         */
        data += (sparse_header->file_hdr_sz - sizeof(sparse_header_t));
    }

    dprintf(INFO, "=== Sparse Image Header ===\n");
    dprintf(INFO, "magic: 0x%x\n", sparse_header->magic);
    dprintf(INFO, "major_version: 0x%x\n", sparse_header->major_version);
    dprintf(INFO, "minor_version: 0x%x\n", sparse_header->minor_version);
    dprintf(INFO, "file_hdr_sz: %d\n", sparse_header->file_hdr_sz);
    dprintf(INFO, "chunk_hdr_sz: %d\n", sparse_header->chunk_hdr_sz);
    dprintf(INFO, "blk_sz: %d\n", sparse_header->blk_sz);
    dprintf(INFO, "total_blks: %d\n", sparse_header->total_blks);
    dprintf(INFO, "total_chunks: %d\n", sparse_header->total_chunks);

    fill_buf = (uint32_t *)memalign(CACHE_LINE, ROUNDUP(sparse_header->blk_sz, CACHE_LINE));
    if (!fill_buf) {
        fastboot_fail_wrapper("Malloc failed for: CHUNK_TYPE_FILL");
        goto closebdev;
    }
    erase_val = (int)bdev->erase_byte|bdev->erase_byte<<8|bdev->erase_byte<<16|bdev->erase_byte<<24;
    pre_fill_val = erase_val;
    dprintf(INFO, "Init previous fill value to 0x%08x\n", pre_fill_val);

    /* Start processing chunks */
    for (chunk=0; chunk<sparse_header->total_chunks; chunk++) {
        /* Read and skip over chunk header */
        chunk_header = (chunk_header_t *)data;
        data += sizeof(chunk_header_t);

        dprintf(INFO, "=== Chunk Header ===\n");
        dprintf(INFO, "chunk_type: 0x%x\n", chunk_header->chunk_type);
        dprintf(INFO, "chunk_data_sz: 0x%x\n", chunk_header->chunk_sz);
        dprintf(INFO, "total_size: 0x%x\n", chunk_header->total_sz);

        if (sparse_header->chunk_hdr_sz > sizeof(chunk_header_t)) {
            /* Skip the remaining bytes in a header that is longer than
             * we expected.
             */
            data += (sparse_header->chunk_hdr_sz - sizeof(chunk_header_t));
        }

        chunk_data_sz = sparse_header->blk_sz * chunk_header->chunk_sz;
        switch (chunk_header->chunk_type) {
            case CHUNK_TYPE_RAW:
                if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz +
                                               chunk_data_sz)) {
                    fastboot_fail("Bogus chunk size for chunk type Raw");
                    goto error;
                }

                dprintf(INFO, "Raw: start block addr: 0x%x\n", total_blocks);

                dprintf(INFO, "addr 0x%llx, partsz 0x%x\n", ((unsigned long long)total_blocks*sparse_header->blk_sz) , chunk_data_sz);
                ret = (size_t)bio_write(bdev, data, ((unsigned long long)total_blocks*sparse_header->blk_sz), chunk_data_sz);
                if (ret != chunk_data_sz) {
                    fastboot_fail_wrapper("flash write failure");
                    goto error;
                }

                total_blocks += chunk_header->chunk_sz;
                data += chunk_data_sz;
                break;

            case CHUNK_TYPE_DONT_CARE:
                dprintf(INFO, "!!Blank: start: 0x%x  offset: 0x%x\n", total_blocks, chunk_header->chunk_sz);
                total_blocks += chunk_header->chunk_sz;
                break;

            case CHUNK_TYPE_FILL:
                dprintf(INFO, "%s %d: CHUNK_TYPE_FILL=0x%x size=%d chunk_data_sz=%d\n", __FUNCTION__, __LINE__, *(uint32_t *)data, ROUNDUP(sparse_header->blk_sz, CACHE_LINE), chunk_data_sz);
                if (chunk_header->total_sz != (sparse_header->chunk_hdr_sz + sizeof(uint32_t))) {
                    fastboot_fail_wrapper("Bogus chunk size for chunk type FILL");
                    goto error;
                }

                fill_val = *(uint32_t *)data;
                data = (char *) data + sizeof(uint32_t);
                chunk_blk_cnt = chunk_data_sz / sparse_header->blk_sz;

                if (fill_val != pre_fill_val) {
                    pre_fill_val = fill_val;
                    for (i = 0; i < (sparse_header->blk_sz / sizeof(fill_val)); i++)
                        fill_buf[i] = fill_val;
                }

                for (i = 0; i < chunk_blk_cnt; i++) {
                    if (fill_val == erase_val) {
                        /* To assume partition already erased */
                        /* skip fill value as same as erase_byte */
                        dprintf(INFO, "skip CHUNK_TYPE_FILL with value '%x'\n", fill_val);
                    } else {
                        ret = (size_t)bio_write(bdev, fill_buf, ((uint64_t)total_blocks*sparse_header->blk_sz), sparse_header->blk_sz);
                        if (ret != sparse_header->blk_sz) {
                            fastboot_fail_wrapper("CHUNK_TYPE_FILL flash write failure");
                            goto error;
                        }
                    }
                    total_blocks++;
                }
                break;

            case CHUNK_TYPE_CRC:
                if (chunk_header->total_sz != sparse_header->chunk_hdr_sz) {
                    fastboot_fail_wrapper("Bogus chunk size for chunk type Dont Care");
                    goto error;
                }
                total_blocks += chunk_header->chunk_sz;
                data += chunk_data_sz;
                break;

            default:
                fastboot_fail_wrapper("Unknown chunk type");
                goto error;
        }
    }

    dprintf(ALWAYS, "Wrote %d blocks, expected to write %d blocks\n",
            total_blocks, sparse_header->total_blks);

    if (total_blocks != sparse_header->total_blks) {
        fastboot_fail_wrapper("sparse image write failure");
    } else {
        fastboot_okay("");
    }

error:
    free(fill_buf);
closebdev:
    bio_close(bdev);

    return;
}

void cmd_flash(const char *arg, void *data, unsigned sz)
{
    sparse_header_t *sparse_header;

    dprintf(ALWAYS, "cmd_flash: %s, %d\n", arg, (u32)sz);

    if (sz  == 0) {
        fastboot_okay("");
        return;
    }

    TIME_START;

// security check, only unlocked device can flash partitions
#if !(defined FORCE_DISABLE_FLASH_CHECK)
    int ret=-1;
    unsigned char blk[256]={0};

    ret=mmc_rpmb_block_read(INDEX_RPMB_UNLOCK,&blk[0]);
    if(ret != 0){
        dprintf(CRITICAL, "mmc_rpmb_block_read fail %d.\n", ret);
        return;
    }

    if(blk[0] != AVB_DEVICE_UNLOCK){
        dprintf(ALWAYS, "Device not unlocked, cmd_flash refused \n");
        return;
    }
#endif

    sparse_header = (sparse_header_t *) data;
    if (sparse_header->magic != SPARSE_HEADER_MAGIC){
        printf("[MTK][%s_%d] flash_image \n",__FUNCTION__,__LINE__);
        cmd_flash_img(arg, data, sz);
    }
    else {
        printf("[MTK][%s_%d] sparse_image \n",__FUNCTION__,__LINE__);
        cmd_flash_sparse_img(arg, data, sz);
    }

    return;
}

void cmd_dump(const char *arg, void *data, unsigned sz)
{
    #define DUMP_BUF_LEN 512
    bdev_t *bdev;
    char *buf;
    int loop, allisff, i, j;

    dprintf(ALWAYS, "%s %s \n", __func__, arg);

    /* use a small buf to loop data */
    buf = mempool_alloc(DUMP_BUF_LEN, MEMPOOL_ANY);
    if (!buf) {
        dprintf(CRITICAL, "alloc buf fail\n");
        goto out;
    }

    bdev = bio_open(arg + 1);
    if (!bdev) {
	dprintf(CRITICAL, "open device %s fail\n", arg);
        goto out;
    }

    loop = bdev->total_size / DUMP_BUF_LEN;
    for (i = 0; i < loop; i++) {
        allisff = 1;
        bio_read(bdev, buf, i * DUMP_BUF_LEN, DUMP_BUF_LEN);

        for (j = 0; j < DUMP_BUF_LEN; j++) {
            if (buf[j] != 0xff)
	        allisff = 0;
            if (!(j % 16))
            dprintf(ALWAYS, "\n0x");
            dprintf(ALWAYS, "%2x ", buf[j]);
        }

	if (allisff)
            break;
    }

    dprintf(ALWAYS, "\n");
    fastboot_okay("");

out:
    if (bdev)
        bio_close(bdev);

    if (buf)
        mempool_free(buf);
}

void cmd_erase(const char *arg, void *data, unsigned sz)
{
#define MAX_ERASE_SIZE  (INT_MAX & ~(bdev->block_size - 1))
    bdev_t *bdev;
    ssize_t ret = 0, erase_len = 0;
	char response[MAX_RSP_SIZE];
    dprintf(ALWAYS, "cmd_erase %s\n", arg);

    if (!strcmp(arg, "force-format-all")) {
	bdev = bio_open("nand0"); /* should always success. */

	dprintf(ALWAYS, "force fromat whole NAND.\n");
	bio_ioctl(bdev, NAND_IOCTL_FORCE_FORMAT_ALL, NULL);
	erase_len = bdev->total_size;
	goto done;
    }

    unsigned erase_flag = 0;
	if (!strcmp(arg, "MBR_NAND")) {
		/* MBR case*/
		erase_flag = 1;
		bdev = bio_open_by_label("nand0");
	} else if (!strcmp(arg, "force_format")) {
		erase_flag = 2;
		bdev = bio_open_by_label(arg);
	} else if (!strcmp(arg, "force_test")) {
		erase_flag = 3;
		bdev = bio_open_by_label(arg);
	} else {
		bdev = bio_open_by_label(arg);
	}

#if (MT8518 || MT8512)
    bdev = bio_open_by_label(arg);
#endif

    if (!bdev) {
			if (erase_flag)
				bdev = bio_open("nand0");
			else
				bdev = bio_open(arg);
			if (!bdev) {
				fastboot_fail_wrapper("Partition is not exist.");
				return;
		}
	}

    if (!bdev->total_size) {
        fastboot_fail_wrapper("Size is uncorrect.");
        goto closebdev;
    }

    dprintf(INFO, "%s %s %lld %d\n", bdev->name, bdev->label, bdev->total_size, bdev->block_count);

	if (erase_flag == 1) {
		dprintf(ALWAYS, "Erase nand MBR here block size:0x%x\n", bdev->block_size);
#ifdef SUPPORT_GPT_FIXED_LBS
		ret = bio_erase(bdev, 0, bdev->block_size * 512);
#else
		ret = bio_erase(bdev, 0, bdev->block_size * 384);
#endif
		if (ret < 0) {
			fastboot_fail_wrapper("Erase failure.");
			return;
		}
		erase_len = ret;
		goto  done;
	}

	if (erase_flag == 2) {
		dprintf(ALWAYS, "Erase nand force fromat here, just for debug, not for all nand!!!\n");
		bio_ioctl(bdev, NAND_IOCTL_FORCE_FORMAT_ALL, NULL);
		erase_len = bdev->total_size;
		goto  done;
	}

	if (erase_flag == 3) {
		dprintf(ALWAYS, "Erase nand and do P/E/R Test here, just for debug!!!\n");
		bio_ioctl(bdev, NAND_IOCTL_FORCE_TEST_ALL, NULL);
		erase_len = bdev->total_size;
		goto  done;
	}

    if (bdev->total_size > (off_t)MAX_ERASE_SIZE) {
        off_t offset = 0LL, size = bdev->total_size;
        size_t len;
        do {
            len = (size_t)MIN(size, (off_t)MAX_ERASE_SIZE);
            ret = bio_erase(bdev, offset, len);
            if (ret < 0) {
                fastboot_fail_wrapper("Erase failure.");
                goto closebdev;
            }
            erase_len += ret;
            size -= len;
            offset += len;
        } while (size);
    } else {
        ret = bio_erase(bdev, 0, bdev->total_size);
        if (ret < 0) {
            fastboot_fail_wrapper("Erase failure.");
            goto closebdev;
        }
		erase_len = ret;
    }

    ret = partition_unpublish(bdev->name);
    if (ret > 0)
        bio_dump_devices();
done:
    snprintf(response, MAX_RSP_SIZE, "request sz: 0x%llx, real erase len: 0x%lx",
			 bdev->total_size, erase_len);
	fastboot_info(response);

    fastboot_okay("");

closebdev:
    bio_close(bdev);
    return;
}
//to support AVB2.0 cmd: fastboot flashing [lock | unlock]
#ifdef AVB_ENABLE_DEVICE_STATE_CHANGE
int cmd_flash_lock_state(const char *arg)
{
    unsigned char blk[256]={0};
    int ret=-1;

    dprintf(INFO, "start to flash  device state [%s] \n",arg);

    if (!strcmp(arg, " lock")) {
        blk[0]=0;
        dprintf(INFO, "start to flash lock state \n");
        ret=mmc_rpmb_block_write(INDEX_RPMB_UNLOCK,&blk[0]);
        dprintf(INFO, "end to flash lock state %d \n",ret);
    }
    else if (!strcmp(arg, " unlock")){
        blk[0]=AVB_DEVICE_UNLOCK;
        ret=mmc_rpmb_block_write(INDEX_RPMB_UNLOCK,&blk[0]);
        dprintf(INFO, "end to flash unlock state %d \n",ret);
    }
    else{
        dprintf(ALWAYS, "cmd %s not supported \n",arg);
    }

    dprintf(ALWAYS, "cmd %s finish %d  \n",arg,ret);

    return ret;
}

void cmd_flashing(const char *arg, void *data, unsigned sz)
{
    char response[MAX_RSP_SIZE];
    int ret=-1;

    if(arg ==  NULL){
        dprintf(ALWAYS, "invalid parameter \n");
        return ;
    }

    dprintf(ALWAYS, "cmd_flashing: %s, \n", arg);

    ret=cmd_flash_lock_state(arg);

    snprintf(response, MAX_RSP_SIZE, "flashing state %s", (ret==0)?"sucessful":"fail");
    fastboot_info(response);

    fastboot_okay("");

    return;
}
#endif

extern unsigned fastboot_state;
void cmd_continue_boot(const char *arg, void *data, unsigned sz)
{
    fastboot_okay("");
    /* set state to leave fastboot command loop and handler */
    fastboot_state = STATE_RETURN;
    mtk_wdt_init();
}

void cmd_download_tz(const char *arg, void *data, unsigned sz)
{
    int ret = 0;
    void *fit;

    if (sz < MAX_TEE_DRAM_SIZE) {
        ret = fit_get_image_from_buffer((char *)data, &fit, (char *)tz_buf);
        if (ret) {
            dprintf(CRITICAL, "%s can't get fit header\n", (char *)TZ_PART_NAME);
            fastboot_fail_wrapper("can't get fit header");
        }
        fastboot_okay("");
    } else {
        dprintf(CRITICAL, "%s size is too large\n", (char *)TZ_PART_NAME);
        fastboot_fail_wrapper("size is too large");
    }
}

void cmd_download_boot(const char *arg, void *data, unsigned sz)
{
    int ret = 0;
    void *fit;

    if (sz < MAX_KERNEL_SIZE) {
        ret = fit_get_image_from_buffer((char *)data, &fit, (char *)kernel_buf);
        if (ret) {
            dprintf(CRITICAL, "%s can't get fit header\n", (char *)BOOT_PART_NAME);
            fastboot_fail_wrapper("can't get fit header");
        }
        fastboot_okay("");
    } else {
        dprintf(CRITICAL, "%s size is too large\n", (char *)BOOT_PART_NAME);
        fastboot_fail_wrapper("size is too large");
    }
}
/**********END********/
/*LXO: END!Download related command*/
/*********efuse support start*********/
static int split(char *buf, char *argv[], int num)
{
    int i = 0;
    char *pch;
    pch = strtok(buf, " ");
    while(pch != NULL && i < num) {
        argv[i++] = pch;
        pch = strtok(NULL, " ");
    }
    return i;
}

extern int read_efuse(unsigned int index, unsigned char *data, unsigned int len)__attribute__((weak));
extern int write_efuse(unsigned int index, const unsigned char *data, unsigned int len)__attribute__((weak));
extern void enable_vefuse(void)__attribute__((weak));
extern void disable_vefuse(void)__attribute__((weak));
void cmd_efuse_write(const char *arg, void *data, unsigned sz)
{
#define EFUSE_WIRTE_ARGV_NUM 3
#define EFUSE_WRITE_BUF_SIZE 32
    char buf[MAX_RSP_SIZE];
    char *argv[MAX_RSP_SIZE];
    int argc, ret = 0, i;
    char res[MAX_RSP_SIZE];
    char write[EFUSE_WRITE_BUF_SIZE];
    unsigned long write_val;

    if (!write_efuse) {
        fastboot_fail_wrapper("efuse write is not supported");
        return;
    }

    strncpy(buf, arg, sizeof(buf));
    argc = split(buf, argv, EFUSE_WIRTE_ARGV_NUM);

    if (argc!=EFUSE_WIRTE_ARGV_NUM) {
        fastboot_fail_wrapper("number of arguments should be three");
        return;
    }

    if (!(isdigit(argv[0][0]) && isdigit(argv[2][0]))) {
        fastboot_fail_wrapper("idx and len should be digit");
        return;
    }

    if (strlen(argv[1]) != (atol(argv[2]) * 2)) {
        fastboot_fail_wrapper("data len is not equal to byte len");
        return;
    }

    for (i = 0;i < strlen(argv[1]);i++) {
        if (!(isxdigit(argv[1][i]))) {
            fastboot_fail_wrapper("data should be hex");
            return;
        }
    }

    write_val = strtol(argv[1], NULL, 16);

    memcpy(write, &write_val, sizeof(write));

    dprintf(INFO, "idx %u, data %s, len %u\n", atoui(argv[0]), argv[1], atoui(argv[2]));

    if (enable_vefuse)
        enable_vefuse();

    ret = write_efuse(atoui(argv[0]), (const unsigned char *)write, atoui(argv[2]));

    if (disable_vefuse)
        disable_vefuse();

    if (!ret) {
        snprintf(res, MAX_RSP_SIZE, "efuse write ok 0x%lx", write_val);
        fastboot_info(res);
        fastboot_okay("");
    } else {
        snprintf(res, MAX_RSP_SIZE, "efuse write fail %d", ret);
        fastboot_fail_wrapper(res);
    }
}

void cmd_efuse_read(const char *arg, void *data, unsigned sz)
{
#define EFUSE_READ_ARGV_NUM 2
#define EFUSE_READ_BUF_SIZE 32
    char read_data[EFUSE_READ_BUF_SIZE];
    char buf[MAX_RSP_SIZE];
    char *argv[MAX_RSP_SIZE];
    int argc, ret = 0, i;
    char res[MAX_RSP_SIZE];
    unsigned long read_val = 0;

    if (!read_efuse) {
        fastboot_fail_wrapper("efuse read is not supported");
        return;
    }

    strncpy(buf, arg, sizeof(buf));
    argc = split(buf, argv, EFUSE_READ_ARGV_NUM);

    if (argc!=EFUSE_READ_ARGV_NUM) {
        fastboot_fail_wrapper("number of arguments should be two");
        return;
    }

    if (!(isdigit(argv[0][0]) && isdigit(argv[1][0]))) {
        fastboot_fail_wrapper("arguments should be digit");
        return;
    }

    dprintf(INFO, "idx %u, len %u\n", atoui(argv[0]), atoui(argv[1]));

    ret = read_efuse(atoui(argv[0]), (unsigned char *)read_data, atoui(argv[1]));

    if (!ret) {
        for (i = 0;i < atoui(argv[1]);i++) {
            read_val |= (*(read_data + i) << (i * 8)) ;
        }
        snprintf(res, MAX_RSP_SIZE, "efuse read ok 0x%lx", read_val);
        fastboot_info(res);
        fastboot_okay("");
    } else {
        snprintf(res, MAX_RSP_SIZE, "efuse read fail %d", ret);
        fastboot_fail_wrapper(res);
    }
}
/*********efuse support end**********/

/*********wifi bt eth0 support start*********/
void * load_dtbo_partition(void)
{
    void *dtbo_buf;
    int ret;
    void * fit;
    dtbo_buf = mempool_alloc(MAX_DTBO_SIZE, MEMPOOL_ANY);
    if (!dtbo_buf) {
        dprintf(CRITICAL, "alloc dtbo buf fail\n");
        return NULL;
    }
    /* check if dtbo is existed */
    ret = fit_get_image(DTBO_PART_NAME, &fit, dtbo_buf);
    if (ret == 0) {
        return dtbo_buf;
    }
    mempool_free(dtbo_buf);
    return NULL;
}
int write_to_dtbo(const char *label, void *load_buf)
{
    bdev_t *bdev;
    struct fdt_header fdt;
    size_t totalsize;
    int fdt_len, ret = 0;

    fdt_len = sizeof(struct fdt_header);
    bdev = bio_open_by_label(label);
    if (!bdev) {
        dprintf(CRITICAL, "Partition [%s] is not exist.\n", label);
        return -ENODEV;
    }
    bio_read(bdev, &fdt, 0, fdt_len);
    ret = fdt_check_header(&fdt);
    if (ret) {
        dprintf(CRITICAL, "[%s] check header failed\n", label);
        return ret;
    }
    totalsize = fdt_totalsize(&fdt);
    bio_write(bdev, load_buf, 0, totalsize);

    return 0;
}

void cmd_wifimac_write(const char *arg, void *data, unsigned sz)
{
    int ret;
    int nodeoffset;
    unsigned char macaddr[6]={0x00,0x0C,0x43,0x28,0x80,0x48};
    int i,j=0;
    unsigned char tempbuf;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;


    if(strlen(arg)!=13)
    {
        fastboot_fail_wrapper("argument length is wrong");
        return;
    }
    for(i=1;i<13;i++)
    {
        tempbuf=0;
        if(((arg[i]>='0'&&arg[i]<='9') || (arg[i]>='a'&&arg[i]<='f') || (arg[i]>='A'&&arg[i]<='F'))&&
            ((arg[i+1]>='0'&&arg[i+1]<='9') || (arg[i+1]>='a'&&arg[i+1]<='f') || (arg[i+1]>='A'&&arg[i+1]<='F')))
        {
            if(arg[i]>='0'&&arg[i]<='9')
                tempbuf = (arg[i]-'0')*16;
            else if(arg[i]>='a'&&arg[i]<='f' )
                tempbuf = (arg[i]-'a'+10)*16;
            else
                tempbuf = (arg[i]-'A'+10)*16;
            if(arg[i+1]>='0'&&arg[i+1]<='9')
                tempbuf = tempbuf+arg[i+1]-'0';
            else if(arg[i+1]>='a'&&arg[i+1]<='f' )
                tempbuf = tempbuf+arg[i+1]-'a'+10;
            else
                tempbuf = tempbuf+arg[i+1]-'A'+10;
            macaddr[j] = tempbuf;
            i++;
            j++;
        }
        else
        {
            fastboot_fail_wrapper("argument is not in 0~9, a~f, A~F");
            return;
        }
    }

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@0/__overlay__/connectivity-combo@0");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find wifi mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    ret = fdt_setprop(fdt_dtbo,nodeoffset,"aucMacAddress",macaddr,6);
    if (ret)
    {
        fastboot_fail_wrapper("write wifi mac address failed");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = fdt_pack(fdt_dtbo);
    if (ret != 0)
    {
        fastboot_fail_wrapper("fdt_pack error");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = write_to_dtbo(DTBO_PART_NAME,fdt_dtbo);
    if (ret)
    {
        fastboot_fail_wrapper("write to dtbo partition failed");
        mempool_free(fdt_dtbo);
        return;
    }
    snprintf(res, MAX_RSP_SIZE, "wifi mac write ok 0x%02x%02x%02x%02x%02x%02x",
            macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}

void cmd_wifimac_read(const char *arg, void *data, unsigned sz)
{
    int nodeoffset;
    const unsigned char * macaddr;
    int readlen;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;
    dprintf(ALWAYS,"cmd_wifimac_read() arg is %s\n",arg);

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@0/__overlay__/connectivity-combo@0");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find wifi mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    macaddr = fdt_getprop(fdt_dtbo, nodeoffset, "aucMacAddress", &readlen);
    if(macaddr == NULL)
    {
        fastboot_fail_wrapper("read wifi mac addr failed in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_getprop() readlen is %d\n", readlen);
    for(int i=0;i<readlen;i++)
    {
        dprintf(ALWAYS,"macaddr[%d] is 0x%02x\n",i, macaddr[i]);
    }
    snprintf(res, MAX_RSP_SIZE, "wifi mac read ok 0x%02x%02x%02x%02x%02x%02x",
            macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}
void cmd_btmac_write(const char *arg, void *data, unsigned sz)
{
    int ret;
    int nodeoffset;
    unsigned char macaddr[6]={0x00,0x0C,0x43,0x28,0x80,0x48};
    int i,j=0;
    unsigned char tempbuf;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;


    if(strlen(arg)!=13)
    {
        fastboot_fail_wrapper("argument length is wrong");
        return;
    }
    for(i=1;i<13;i++)
    {
        tempbuf=0;
        if(((arg[i]>='0'&&arg[i]<='9') || (arg[i]>='a'&&arg[i]<='f') || (arg[i]>='A'&&arg[i]<='F'))&&
            ((arg[i+1]>='0'&&arg[i+1]<='9') || (arg[i+1]>='a'&&arg[i+1]<='f') || (arg[i+1]>='A'&&arg[i+1]<='F')))
        {
            if(arg[i]>='0'&&arg[i]<='9')
                tempbuf = (arg[i]-'0')*16;
            else if(arg[i]>='a'&&arg[i]<='f' )
                tempbuf = (arg[i]-'a'+10)*16;
            else
                tempbuf = (arg[i]-'A'+10)*16;
            if(arg[i+1]>='0'&&arg[i+1]<='9')
                tempbuf = tempbuf+arg[i+1]-'0';
            else if(arg[i+1]>='a'&&arg[i+1]<='f' )
                tempbuf = tempbuf+arg[i+1]-'a'+10;
            else
                tempbuf = tempbuf+arg[i+1]-'A'+10;
            macaddr[j] = tempbuf;
            i++;
            j++;
        }
        else
        {
            fastboot_fail_wrapper("argument is not in 0~9, a~f, A~F");
            return;
        }
    }

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@0/__overlay__/connectivity-combo@0");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find bt mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    ret = fdt_setprop(fdt_dtbo,nodeoffset,"btAddr",macaddr,6);
    if (ret)
    {
        fastboot_fail_wrapper("write bt mac address failed");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = fdt_pack(fdt_dtbo);
    if (ret != 0)
    {
        fastboot_fail_wrapper("fdt_pack error");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = write_to_dtbo(DTBO_PART_NAME,fdt_dtbo);
    if (ret)
    {
        fastboot_fail_wrapper("write to dtbo partition failed");
        mempool_free(fdt_dtbo);
        return;
    }
    snprintf(res, MAX_RSP_SIZE, "bt mac write ok 0x%02x%02x%02x%02x%02x%02x",
            macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}

void cmd_btmac_read(const char *arg, void *data, unsigned sz)
{
    int nodeoffset;
    const unsigned char * macaddr;
    int readlen;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;
    dprintf(ALWAYS,"cmd_btmac_read() arg is %s\n",arg);

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@0/__overlay__/connectivity-combo@0");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find bt mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    macaddr = fdt_getprop(fdt_dtbo, nodeoffset, "btAddr", &readlen);
    if(macaddr == NULL)
    {
        fastboot_fail_wrapper("read bt mac addr failed in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_getprop() readlen is %d\n", readlen);
    for(int i=0;i<readlen;i++)
    {
        dprintf(ALWAYS,"macaddr[%d] is 0x%02x\n",i, macaddr[i]);
    }
    snprintf(res, MAX_RSP_SIZE, "bt mac read ok 0x%02x%02x%02x%02x%02x%02x",
            macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}
void cmd_ethmac_write(const char *arg, void *data, unsigned sz)
{
    int ret;
    int nodeoffset;
    unsigned char macaddr[6]={0x00,0x0C,0x43,0x28,0x80,0x48};
    int i,j=0;
    unsigned char tempbuf;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;


    if(strlen(arg)!=13)
    {
        fastboot_fail_wrapper("argument length is wrong");
        return;
    }
    for(i=1;i<13;i++)
    {
        tempbuf=0;
        if(((arg[i]>='0'&&arg[i]<='9') || (arg[i]>='a'&&arg[i]<='f') || (arg[i]>='A'&&arg[i]<='F'))&&
            ((arg[i+1]>='0'&&arg[i+1]<='9') || (arg[i+1]>='a'&&arg[i+1]<='f') || (arg[i+1]>='A'&&arg[i+1]<='F')))
        {
            if(arg[i]>='0'&&arg[i]<='9')
                tempbuf = (arg[i]-'0')*16;
            else if(arg[i]>='a'&&arg[i]<='f' )
                tempbuf = (arg[i]-'a'+10)*16;
            else
                tempbuf = (arg[i]-'A'+10)*16;
            if(arg[i+1]>='0'&&arg[i+1]<='9')
                tempbuf = tempbuf+arg[i+1]-'0';
            else if(arg[i+1]>='a'&&arg[i+1]<='f' )
                tempbuf = tempbuf+arg[i+1]-'a'+10;
            else
                tempbuf = tempbuf+arg[i+1]-'A'+10;
            macaddr[j] = tempbuf;
            i++;
            j++;
        }
        else
        {
            fastboot_fail_wrapper("argument is not in 0~9, a~f, A~F");
            return;
        }
    }

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@1/__overlay__/eth@1101c000");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find ethernet mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    ret = fdt_setprop(fdt_dtbo, nodeoffset, "mac-address", macaddr, 6);
    if (ret)
    {
        fastboot_fail_wrapper("write ethernet mac address failed");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = fdt_pack(fdt_dtbo);
    if (ret != 0)
    {
        fastboot_fail_wrapper("fdt_pack error");
        mempool_free(fdt_dtbo);
        return;
    }
    ret = write_to_dtbo(DTBO_PART_NAME, fdt_dtbo);
    if (ret)
    {
        fastboot_fail_wrapper("write to dtbo partition failed");
        mempool_free(fdt_dtbo);
        return;
    }
    snprintf(res, MAX_RSP_SIZE, "ethernet mac write ok 0x%02x%02x%02x%02x%02x%02x",
             macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}

void cmd_ethmac_read(const char *arg, void *data, unsigned sz)
{
    int nodeoffset;
    const unsigned char * macaddr;
    int readlen;
    char res[MAX_RSP_SIZE];
    void *fdt_dtbo = NULL;
    dprintf(ALWAYS,"cmd_ethmac_read() arg is %s\n",arg);

    fdt_dtbo = load_dtbo_partition();
    if(!fdt_dtbo)
    {
        fastboot_fail_wrapper("load dtbo partition fail");
        return;
    }

    nodeoffset = fdt_path_offset(fdt_dtbo,"/fragment@1/__overlay__/eth@1101c000");
    if(nodeoffset == -1)
    {
        fastboot_fail_wrapper("can't find ethernet mac node in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_path_offset() nodeoffset is %d\n",nodeoffset);
    macaddr = fdt_getprop(fdt_dtbo, nodeoffset, "mac-address", &readlen);
    if(macaddr == NULL)
    {
        fastboot_fail_wrapper("read ethernet mac addr failed in dtbo");
        mempool_free(fdt_dtbo);
        return;
    }
    dprintf(ALWAYS,"fdt_getprop() readlen is %d\n", readlen);
    for(int i=0;i<readlen;i++)
    {
        dprintf(ALWAYS,"macaddr[%d] is 0x%02x\n",i, macaddr[i]);
    }
    snprintf(res, MAX_RSP_SIZE, "ethernet mac read ok 0x%02x%02x%02x%02x%02x%02x",
             macaddr[0],macaddr[1],macaddr[2],macaddr[3],macaddr[4],macaddr[5]);
    fastboot_info(res);
    fastboot_okay("");
    mempool_free(fdt_dtbo);
}
/*********wifi bt eth0 support end*********/

void cmd_lkver_read(const char *arg, void *data, unsigned sz)
{
    dprintf(ALWAYS,"\n%s\n",LK_VERSION);
    fastboot_info(LK_VERSION);
    fastboot_okay("");
}


void cmd_led_ctrl(const char *arg, void *data, unsigned sz)
{
	char *pArgsA[2];
	int iArgs;

	dprintf(ALWAYS,"%s(%d) arg=\"%s\"\n",__func__,__LINE__,arg);

	iArgs = _parse_args(arg,&pArgsA,2);

	if(0==strcmp(pArgsA[0],"green")) {
		// green led 
		//dprintf(ALWAYS,"green led ");
		if(0==strcmp(pArgsA[1],"1")) {
			// on
			//dprintf(ALWAYS,"on");
			ntx_led(NTX_ON_LED,1);
		}
		else if(0==strcmp(pArgsA[1],"0")) {
			// off
			//dprintf(ALWAYS,"off");
			ntx_led(NTX_ON_LED,0);
		}
	}
	else if(0==strcmp(pArgsA[0],"auto")) {
		//bd71828_led_ctrl(BD71828_LED_ID_AUTO,1);
	}
	else {
		fastboot_info("which led do you want to controlled ?");
		dprintf(ALWAYS,"%s(%d) which led do you want to controlled ?\n",__func__,__LINE__,arg);
	}
	
	fastboot_okay("");
}

void cmd_poweroff(const char *arg, void *data, unsigned sz)
{
	dprintf(ALWAYS,"%s(%d)\n",__func__,__LINE__);
    fastboot_info("powering off");
	//fastboot_okay("");
	bd71828_poweroff();
}



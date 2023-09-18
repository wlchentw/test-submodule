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
#include <assert.h>
#include <errno.h>
#include <libfdt.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/mempool.h>
#include <platform.h>
#include <platform/mt_uart.h>
#include <platform/mtk_key.h>
#include <platform/mtk_wdt.h>
#include <rpmb/include/rpmb_mac.h>
#include <dl_commands.h>

#include "fit.h"
#include "avb.h"

/* BL33 load and entry point address */
#define CFG_BL33_LOAD_EP_ADDR   (BL33_ADDR)
#define ERR_ADDR    (0xffffffff)

#define RECOVERY_BOOT 1
#define FASTBOOT_BOOT 2
#define NORMAL_BOOT   0

typedef void (*jump32_func_type)(uint32_t addr, uint32_t arg1, uint32_t arg2) __NO_RETURN;
typedef void (*jump64_func_type)(uint64_t bl31_addr, uint64_t bl33_addr, uint64_t arg1) __NO_RETURN;

struct fit_load_data {
    char *part_name;
    char *recovery_part_name;
    void *buf;
    u32 boot_mode;
    ulong kernel_entry;
    ulong dtb_load;
    ulong trustedos_entry;
};

/* global variables, also used in dl_commands.c */
void *kernel_buf;
void *tz_buf;

#if ARCH_ARM64
void el3_mtk_sip(uint32_t smc_fid, uint64_t bl31_addr, uint64_t bl33_addr)
{
    jump64_func_type jump64_func = (jump64_func_type)bl31_addr;
    (*jump64_func)(bl31_addr, bl33_addr, 0UL);
}
#endif

void prepare_bl2_exit(ulong smc_fid, ulong bl31_addr, ulong bl33_addr, ulong arg1)
{
#if ARCH_ARM64
    /* switch to el3 via smc, and will jump to el3_mtk_sip from smc handler */
    __asm__ volatile("smc #0\n\t");
#else
    jump32_func_type jump32_func = (jump32_func_type)bl31_addr;
    (*jump32_func)(bl33_addr, 0, 0);
#endif
}

static void setup_bl33(uint *bl33, ulong fdt, ulong kernel_ep)
{
    bl33[12] = (ulong)fdt;
    bl33[14] = (unsigned)0;
    bl33[16] = (unsigned)0;
    bl33[18] = (unsigned)0;
    bl33[20] = (ulong)kernel_ep;
    bl33[21] = (ulong)0;
    bl33[22] = (unsigned)MACH_TYPE;
}

static int extract_fdt(void *fdt, int size)
{
    int ret = 0;

    /* DTB maximum size is 2MB */
    ret = fdt_open_into(fdt, fdt, size);
    if (ret) {
        dprintf(CRITICAL, "open fdt failed\n");
        return ret;
    }
    ret = fdt_check_header(fdt);
    if (ret) {
        dprintf(CRITICAL, "check fdt failed\n");
        return ret;
    }

    return ret;
}

static bool download_check(void)
{
    if (check_fastboot_mode()) {
        set_clr_fastboot_mode(false);
        return true;
    } else {
        return (check_uart_enter() || check_download_key());
    }
}

static bool recovery_check(void)
{
    if (check_recovery_mode()) {
        set_clr_recovery_mode(false);
        return true;
    } else
        return false;
}

static int fit_load_images(void *fit, struct fit_load_data *fit_data,bool need_verified)
{
    int ret;
    int verify_kernel;
    int verify_ramdisk;
    int verify_fdt;
    int verify_trustedos;

    /* TODO: decide verify policy with config. currently verify kernel only. */
    verify_kernel = need_verified;
    verify_ramdisk = need_verified;
    verify_fdt = need_verified;
    verify_trustedos = need_verified;

    ret = fit_load_image(NULL, "kernel", fit, NULL, &fit_data->kernel_entry, need_verified);
    if (verify_kernel && ret && (ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load kernel failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "tee", fit, NULL, &fit_data->trustedos_entry, need_verified);
    if (verify_trustedos && ret&& (ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load trustedos failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "ramdisk", fit, NULL, NULL, need_verified);
    if (verify_ramdisk && ret && (ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load ramdisk failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "fdt", fit, &fit_data->dtb_load, NULL, need_verified);
    if (ret && (ret != -ENOENT))
        fit_data->dtb_load = ERR_ADDR;

    if (verify_fdt && ret && (ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load fdt failed\n", fit_data->part_name);
        return ret;
    }

    return 0;
}


static int fit_load_thread(void *arg,bool need_veriried)
{
    int err;
    void *fit;
    struct fit_load_data *fit_data = (struct fit_load_data *)arg;

    if (fit_data->boot_mode == FASTBOOT_BOOT) {
        fit = fit_data->buf;
        err = fit_load_images(fit, fit_data,need_veriried);
        return err;
    }

    while (fit_data->boot_mode == NORMAL_BOOT) {
        err = fit_get_image(fit_data->part_name, &fit, fit_data->buf);
        if (err)
            break;

        err = fit_load_images(fit, fit_data, need_veriried);
        if (err)
            break;

        return 0;
    }

    dprintf(CRITICAL, "%s try recovery mode !!\n", fit_data->recovery_part_name);
    // RECOVERY_BOOT
    err = fit_get_image(fit_data->recovery_part_name, &fit, fit_data->buf);
    if (err)
        return err;

    err = fit_load_images(fit, fit_data, need_veriried);

    return err;
}

#define MAX_CMDLINE_LENGTH 2048
static int cmdlineoverlay(void* boot_dtb, char* cmdline, int len, char* ab_suffix)
{
    int chosen_node_offset = 0;
    int ret = -1;
    char* new_cmd;

    new_cmd = mempool_alloc(MAX_CMDLINE_LENGTH,MEMPOOL_ANY);
    memset(new_cmd,0,MAX_CMDLINE_LENGTH);

    ret = extract_fdt(boot_dtb, MAX_DTB_SIZE);
    if (ret != 0) {
        dprintf(CRITICAL, "extract_fdt error.\n");
        return -1;
    }

    chosen_node_offset = fdt_path_offset(boot_dtb, "/chosen");

    const char *cmdline_read;
    int length;
    cmdline_read = fdt_getprop(boot_dtb, chosen_node_offset, "bootargs", &length);

    memcpy(new_cmd,cmdline_read,strlen(cmdline_read));
    new_cmd[strlen(cmdline_read)] = ' ';
    memcpy(new_cmd+strlen(cmdline_read)+1,cmdline,strlen(cmdline));

#ifdef AB_OTA_UPDATER
    new_cmd[strlen(new_cmd)] = ' ';
    memcpy(new_cmd+strlen(new_cmd),"androidboot.slot=",strlen("androidboot.slot="));
    memcpy(new_cmd+strlen(new_cmd),ab_suffix+1,1);
#else
    //unused ab_suffix;
#endif

    dprintf(INFO, "new cmdline: %s ,length:%lu\n", new_cmd, strlen(new_cmd));

    ret = fdt_setprop(boot_dtb, chosen_node_offset, "bootargs", new_cmd, strlen(new_cmd) + 1);

    if (ret != 0) {
        dprintf(CRITICAL, "fdt_setprop error.\n");
        return -1;
    }

    ret = fdt_pack(boot_dtb);
    if (ret != 0) {
        dprintf(CRITICAL, "fdt_pack error.\n");
        return -1;
    }

    return 0;
}

extern void ext_boot(void);
static void avbboot_task(const struct app_descriptor *app, void *args)
{
    void *fit, *dtbo_buf;
    struct fit_load_data tz,bootimg;
    int ret_tz;
    int ret;
    u32 boot_mode = NORMAL_BOOT;
    AvbSlotVerifyResult verify_result;
    AvbSlotVerifyData* verify_data;

    uint bl33[] = { 0xea000005,  /* b BL33_32_ENTRY  | ands x5, x0, x0  */
                    0x58000160,  /* .word 0x58000160 | ldr x0, _X0      */
                    0x58000181,  /* .word 0x58000181 | ldr x1, _X1      */
                    0x580001a2,  /* .word 0x580001a2 | ldr x2, _X2      */
                    0x580001c3,  /* .word 0x580001c3 | ldr x3, _X3      */
                    0x580001e4,  /* .word 0x580001e4 | ldr x4, _X4      */
                    0xd61f0080,  /* .word 0xd61f0080 | br  x4           */
                    /* BL33_32_ENTRY:   |                  */
                    0xe59f0030,  /*    ldr r0, _R0   | .word 0xe59f0030 */
                    0xe59f1030,  /*    ldr r1, _R1   | .word 0xe59f1030 */
                    0xe59f2004,  /*    ldr r2, _X0   | .word 0xe59f2004 */
                    0xe59ff020,  /*    ldr pc, _X4   | .word 0xe59ff020 */
                    0x00000000,  /*      .word   0x00000000 */
                    0x00000000,  /* _X0: .word   0x00000000 */
                    0x00000000,  /*      .word   0x00000000 */
                    0x00000000,  /* _X1: .word   0x00000000 */
                    0x00000000,  /*      .word   0x00000000 */
                    0x00000000,  /* _X2: .word   0x00000000 */
                    0x00000000,  /*      .word   0x00000000 */
                    0x00000000,  /* _X3: .word   0x00000000 */
                    0x00000000,  /*      .word   0x00000000 */
                    0x00000000,  /* _X4: .word   0x00000000 */
                    0x00000000,  /* _R0: .word   0x00000000 */
                    0x00000000,  /* _R1: .word   0x00000000 */
                    0x00000000   /*      .word   0x00000000 */
                  };

#if (defined AVB_ENABLE_ANTIROLLBACK) || (defined AVB_ENABLE_DEVICE_STATE_CHANGE)
    rpmb_init();
#endif

    /* recovery */
    if (recovery_check()) {
        boot_mode = RECOVERY_BOOT;
    }

    /* fastboot */
    if (download_check()) {
        ext_boot();
        boot_mode = FASTBOOT_BOOT;
    }

    tz_buf = mempool_alloc(MAX_TEE_DRAM_SIZE, MEMPOOL_ANY);
    if (!tz_buf) {
        dprintf(CRITICAL, "alloc buf fail, kernel %p, tz %p\n",
                kernel_buf, tz_buf);
        return;
    }

    tz.part_name = (char *)TZ_PART_NAME;
    tz.recovery_part_name = (char *)RECOVERY_TZ_PART_NAME;
    tz.boot_mode = boot_mode;
    tz.buf = tz_buf;

    ret_tz = fit_load_thread(&tz,NEED_VERIFIED);

    if (ret_tz) {
        dprintf(CRITICAL, "load tz image failed\n");
        return;
    }

    verify_result = android_verified_boot_2_0(&verify_data);
    if(verify_result == AVB_SLOT_VERIFY_RESULT_OK || is_device_unlocked())
    {
        bootimg.part_name = (char*)BOOT_PART_NAME;
        ret = fit_load_images(get_partition_data(bootimg.part_name,verify_data),&bootimg,NO_VERIFIED);
        if(ret)
        {
            dprintf(CRITICAL, "load boot image failed\n");
            return;
        }

        if(cmdlineoverlay((void*)bootimg.dtb_load,verify_data->cmdline,strlen(verify_data->cmdline),verify_data->ab_suffix))
        {
            dprintf(CRITICAL, "cmdline overlay fail\n");
            return;
        }
    }
    else
    {
        bootimg.recovery_part_name = (char *)RECOVERY_BOOT_PART_NAME;
        bootimg.boot_mode = RECOVERY_BOOT;
        bootimg.buf = mempool_alloc(MAX_KERNEL_SIZE, MEMPOOL_ANY);
        ret = fit_load_thread(&bootimg,NEED_VERIFIED);
        if(ret)
        {
            dprintf(CRITICAL, "load recovery image failed\n");
            return;
        }
    }

    dtbo_buf = mempool_alloc(MAX_DTBO_SIZE, MEMPOOL_ANY);
    if (!dtbo_buf) {
        dprintf(CRITICAL, "alloc dtbo buf fail\n");
        return;
    }

    /* check if dtbo is existed */
    ret = fit_get_image(DTBO_PART_NAME, &fit, dtbo_buf);
    if (ret == 0) {
        void *fdt_dtbo;
        void *fdt_dtb;

        if (bootimg.dtb_load == ERR_ADDR) {
            dprintf(CRITICAL, "dtbo failed, no dtb\n");
            return;
        }
        fdt_dtb = (void *)bootimg.dtb_load;

        /* extract fdt */
        ret = extract_fdt(fdt_dtb, MAX_DTB_SIZE);
        if (ret) {
            dprintf(CRITICAL, "extract fdt failed\n");
            return;
        }

        dprintf(ALWAYS, "[fitboot] do overlay\n");
        fdt_dtbo = (void *)dtbo_buf;
        ret = fdt_overlay_apply(fdt_dtb, fdt_dtbo);
        if (ret) {
            dprintf(CRITICAL, "fdt merge failed, ret %d\n", ret);
            return;
        }

        /* pack fdt */
        ret = fdt_pack(fdt_dtb);
        if (ret) {
            dprintf(CRITICAL, "ft pack failed\n");
            return;
        }
    }

    /* load bl33 for tz to jump*/
    extern __WEAK paddr_t kvaddr_to_paddr(void *ptr);
    addr_t fdt_pa = kvaddr_to_paddr?kvaddr_to_paddr((void *)bootimg.dtb_load):bootimg.dtb_load;
    setup_bl33(bl33, fdt_pa, (uint)(bootimg.kernel_entry));
    memmove((void *)CFG_BL33_LOAD_EP_ADDR, bl33, sizeof(bl33));

    ulong bl33_pa = CFG_BL33_LOAD_EP_ADDR;
    ulong smc_fid = 0xc3200000UL; /* only used in ARCH_ARM64 */

#if ARCH_ARM64 && WITH_KERNEL_VM
    /* 64-bit LK use non identity mapping VA, VA to PA translation needed */
    bl33_pa = (ulong)kvaddr_to_paddr((void *)CFG_BL33_LOAD_EP_ADDR);
#endif
    dprintf(ALWAYS, "LK run time: %lld (us)\n", current_time_hires());
    dprintf(ALWAYS, "jump to tz %p\n", (void *)tz.kernel_entry);
    arch_chain_load((void *)prepare_bl2_exit, smc_fid, tz.kernel_entry, bl33_pa, 0UL);
}

APP_START(avbboot)
.entry = avbboot_task,
 .flags = 0,
  APP_END

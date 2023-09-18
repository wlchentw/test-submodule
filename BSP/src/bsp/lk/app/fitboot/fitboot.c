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
#ifdef MTK_TINYSYS_SCP_SUPPORT
#include <platform/mt_scp.h>
#endif

#include "fit.h"

#include <stdio.h>
#include <lib/bio.h>
#include <platform/emi.h>
#include <string.h>
#include <platform/interrupts.h>

/* BL33 load and entry point address */
#define CFG_BL33_LOAD_EP_ADDR   (BL33_ADDR)
#define ERR_ADDR    (0xffffffff)

#define RECOVERY_BOOT 1
#define FASTBOOT_BOOT 2
#define NORMAL_BOOT   0
#define UPG_SUCCEED   2

//#define AUTO_DRAM_DETECT_BOOT
void *kernel_buf = NULL;
void *tz_buf = NULL;;
void *dtbo_buf;
int fastboot_mode=0;

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

typedef struct boot_flag boot_flag;
struct boot_flag
{
	int lastboot;
	int usea;
	int useb;
	int current;
};

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
#if MT8512 && ARCH_ARM
    /* mask timer interrupt to make wfi work in warm reset */
    mask_interrupt(ARM_GENERIC_TIMER_PHYSICAL_INT);
    arch_chain_load(arm_warm_reset, bl31_addr, 0, 0, 0);
#else
    jump32_func_type jump32_func = (jump32_func_type)bl31_addr;
    (*jump32_func)(bl33_addr, 0, 0);
#endif
#endif
}

static void setup_bl33(uint *bl33, ulong fdt, ulong kernel_ep)
{

    bl33[12] = (ulong)fdt;
    bl33[14] = (unsigned)0;
    bl33[16] = (unsigned)0;
    bl33[18] = (unsigned)0;
    bl33[20] = (ulong)kernel_ep;
#if WITH_UBOOT
    bl33[21] = (ulong)fdt;
#else
    bl33[21] = (ulong)0;
#endif
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
extern int errno;
u32 set_currently_boot_flag(int last_flag, u32 current_flag, const char *part_name)
{
	int ret = 0;
	long len = 0;
	u32 writesize = 2048;
	int index = -1;
	unsigned long long ptn = 0;
	unsigned long long size = 0;
	char *buf;
	boot_flag set_flag;

	/* read partition */
	struct bdev *nand_MISC = bio_open_by_label("misc");
	buf = malloc(writesize);
	if (!buf) {
		dprintf(CRITICAL, "malloc fail, buf is NULL\n");
		return -1;
	}
	memset(buf, 0, writesize);
	
	len = bio_read(nand_MISC, buf, 0, sizeof(boot_flag));
	if (len < 0) {
		dprintf(CRITICAL, "%s read error. LINE: %d\n", part_name, __LINE__);
		return -1;
	}

	/* dump flag for debug */
	dprintf(CRITICAL, "current boot flag is %d\n", current_flag);

	/* set currently flag to buf */
	set_flag.lastboot = last_flag;
	set_flag.current = current_flag;
	set_flag.usea = -1;
	set_flag.useb = -1;

	memset(buf, 0, writesize);
	memcpy(buf, (void*)&set_flag, sizeof(boot_flag));
   
	/* write buf to offset 0, which size is 2048 */
	len = bio_write(nand_MISC, (char*)buf, 0, (u32)writesize);
	if (len <=  0) {
		dprintf(CRITICAL, "nand write fail, return : %ld,  error: \n",len, strerror(errno));
		dprintf(CRITICAL, "buf: %s\n", buf);
		ret = -1;
		goto err;
	}
	dprintf(CRITICAL, "set flag: lastboot = %d, use A = %d, use B = %d, current = %d\n", set_flag.lastboot, set_flag.usea, set_flag.useb, set_flag.current);
	if (buf) {
        free(buf);
        buf = NULL;
    }
	return 0;
err:
	if (buf) {
        free(buf);
        buf = NULL;
    }
	return ret;
}

u32 check_boot_partition(const char *part_name)
{
	int ret = 0;
	boot_flag flag;
	u32 boot = 0;
	
	struct bdev *nand_MISC = bio_open_by_label("misc");
	int len = -1;
	char *buf;

	if (!nand_MISC) {
		printf("failed to open MISC\n");
		return 0;
	}
	printf("open MISC successfully\n");

	/* read partition */
	buf = malloc(sizeof(boot_flag));
    if (!buf) {
        printf("failed maloc buf\n");
        return 0;
    }
	len = bio_read(nand_MISC, buf, 0, sizeof(boot_flag));
	memcpy(&flag, (void*)buf, sizeof(boot_flag));
	if (len < 0) {
		dprintf(CRITICAL, "read %s: boot flag read error. LINE: %d\n", part_name, __LINE__);
		return -1;
	}

	/* dump flag for debug */
	dprintf(CRITICAL, "lastboot = %d, use A = %d, use B = %d, current = %d\n", flag.lastboot, flag.usea, flag.useb, flag.current);

	/* make dicision */
	if (flag.lastboot == 0) {
		if (flag.useb == UPG_SUCCEED){
			boot = 1;
			dprintf(CRITICAL,"***last succeed boot from A system,upgrade B succeed***\n");
			dprintf(CRITICAL,"***now boot from system B***\n");
		}
		else{
			boot = 0;
			dprintf(CRITICAL,"***last succeed boot from A system,upgrade B failed or no upgrade B***\n");
			dprintf(CRITICAL,"***now boot from system A***\n");
		}
	} else if (flag.lastboot == 1) {
		if (flag.usea == UPG_SUCCEED){
			boot = 0;
			dprintf(CRITICAL,"***last succeed boot from B system,upgrade A succeed***\n");
			dprintf(CRITICAL,"***now boot from system A***\n");
		}
		else{
			boot = 1;
			dprintf(CRITICAL,"***last succeed boot from B system,upgrade A failed or no upgrade A***\n");
			dprintf(CRITICAL,"***now boot from system B***\n");
		}
	} else {
		dprintf(CRITICAL, "boot flag is not match, use default boot partition\n");
		boot = 0;
	}

	if ((flag.current != boot) || (flag.usea == UPG_SUCCEED) || (flag.useb == UPG_SUCCEED)) {
		ret = bio_erase(nand_MISC, 0, nand_MISC->total_size);  //erase total_size
		printf("bio erase ret %d\n", ret);
		ret = set_currently_boot_flag(flag.lastboot, boot, part_name);
		if(ret!=0)
			dprintf(CRITICAL, "set flags fail. LINE: %d\n", __LINE__);
	}
	if (buf) {
			free(buf);
			buf = NULL;
		}
	return boot;
}

U32 swapInt32( U32 value )
{
	 return ((value & 0x000000FF) << 24) |
		((value & 0x0000FF00) << 8) |
		((value & 0x00FF0000) >> 8) |
               ((value & 0xFF000000) >> 24) ;
}

int fit_set_mtee_array( U64 dram_size, U32 mtee_start_addr, unsigned char setval_mtee[16], unsigned char setval[16] )
{
	U32 dram_start_addr = DRAM_BASE_PHY;
	U64 mtee_size = dram_start_addr+dram_size-mtee_start_addr;

	dram_size = swapInt32(dram_size);
	dram_start_addr = swapInt32(dram_start_addr);
	mtee_start_addr = swapInt32( mtee_start_addr );
	mtee_size = swapInt32(mtee_size);

	memcpy( (void*)&setval[12]      , (void const*) &dram_size       , 4 );
	memcpy( (void*)&setval[4]       , (void const*) &dram_start_addr , 4 );
	memcpy( (void*)&setval_mtee[4]  , (void const*) &mtee_start_addr , 4 );
	memcpy( (void*)&setval_mtee[12] , (void const*) &mtee_size       , 4 );

	return 0;
}

int fit_change_dtb_memorysize( const void *data, U32 mtee_start_addr )
{
		unsigned char setval_mtee[16] = {0} , setval[16] = {0};
		const void *regval;
  		/* if changesize_flag==true ,change dtb size,else do not change dtb size */
		bool changesize_flag = true;
		int len,len_reg;

		len = fdt_path_offset(data, "/memory");
		regval = fdt_getprop(data, len, "reg", &len_reg);

		unsigned char* regval_int = (unsigned char*)regval;
  		/* get used memory from dtb */
		int dts_memory = regval_int[12];
		extern __WEAK U64 get_dram_size(void);
		U64 dram_size = get_dram_size();
		/*get platform actually size */
		dprintf(CRITICAL, "get_dram_size: %llx\n", dram_size);
		/* set mtee array */
		fit_set_mtee_array( dram_size, mtee_start_addr, setval_mtee,setval );

		if(changesize_flag)
		{
			len = fdt_path_offset(data, "/reserved-memory");
			int teelen = fdt_subnode_offset(data,len, "mtee-reserved-memory");
			int ret1 = fdt_setprop((void*)data, teelen, "reg", setval_mtee, 16);
                  	/* set mtee error */
			if (ret1)
			{
				dprintf(CRITICAL, " fdt_setprop mtee return error %d\n", ret1);
				return ret1;
			}
			len = fdt_path_offset(data, "/memory");
			int ret = fdt_setprop((void*)data, len, "reg", setval, 16);
                  	/* set memory error */
			if (ret)
			{
				dprintf(CRITICAL, " fdt_setprop memory return error %d\n", ret);
				return ret;
			}
		}
		return 0;
}

static int emmc_cmdlineoverlay(void* boot_dtb, char* cmdline, int len)
{
	int chosen_node_offset = 0;
	int ret = -1;
	ret = extract_fdt(boot_dtb, MAX_DTB_SIZE);
	if (ret != 0) {
		dprintf(CRITICAL, "extract_fdt error.\n");
		return -1;
	}

	chosen_node_offset = fdt_path_offset(boot_dtb, "/chosen");
	char *cmdline_read;
	int lenth;
	cmdline_read = fdt_getprop(boot_dtb, chosen_node_offset, "bootargs", &lenth);
	dprintf(CRITICAL, "dtsi cmdline: %s ,lenth:%d\n", cmdline_read, lenth);
	char *pos1;
	char *pos2;
	pos1 = strstr(cmdline_read,"root=/dev/mmcblk0p");
	if( pos1 == NULL) {
		dprintf(CRITICAL, "no root= in cmdline, error!\n");
		return -1;
	}
	pos2 = strstr(pos1, " ");
	if( pos2 == NULL) {
		dprintf(CRITICAL, "no root= in cmdline, error!\n");
		return -1;
	}
	if( (pos2 - pos1 -18) <=0 ) {
		dprintf(CRITICAL, "no root= in cmdline, error!\n");
		return -1;
	}

	char *mtdnum_str = NULL;
	char mtdnum_str_new[3];
	mtdnum_str = malloc((pos2-pos1-18)+1);  /*The 18 is string "root=/dev/mmcblk0p" length, 1 for the end of the string*/
	if (!mtdnum_str) {
		dprintf(CRITICAL, "malloc fail, mtdnum_str is NULL\n");
		return -1;
	}
	strncpy(mtdnum_str, pos1+18, (pos2-pos1-18));
	mtdnum_str[pos2-pos1-18] = '\0';
	int mtdnum = atoi(mtdnum_str);
	mtdnum ++;
	sprintf(mtdnum_str_new, "%d", mtdnum);
	if (mtdnum >= 10) {
		strncpy(pos1+18, mtdnum_str_new, 2);
	}
	else {
		strncpy(pos1+18, mtdnum_str_new, 1);
	}
	printf("cmdline new: %s , length: %d", cmdline_read, strlen(cmdline_read));
	ret = fdt_setprop(boot_dtb, chosen_node_offset, "bootargs", cmdline_read, strlen(cmdline_read) + 1);
	if (ret != 0) {
		dprintf(CRITICAL, "fdt_setprop error.\n");
		goto err;
	}
	ret = fdt_pack(boot_dtb);
	if (ret != 0) {
		dprintf(CRITICAL, "fdt_pack error.\n");
		goto err;
	}

	free(mtdnum_str);
	mtdnum_str= NULL;
	return 0;
err:
	free(mtdnum_str);
	mtdnum_str= NULL;
	return -1;
}

static int cmdlineoverlay(void* boot_dtb, char* cmdline, int len)
{
	int chosen_node_offset = 0;
	int ret = -1;
	ret = extract_fdt(boot_dtb, MAX_DTB_SIZE);
	if (ret != 0) {
		dprintf(CRITICAL, "extract_fdt error.\n");
		return -1;
	}
	
	chosen_node_offset = fdt_path_offset(boot_dtb, "/chosen");
	char *cmdline_read;
	int lenth;
	cmdline_read = fdt_getprop(boot_dtb, chosen_node_offset, "bootargs", &lenth);
	dprintf(CRITICAL, "dtsi cmdline: %s ,lenth:%d\n", cmdline_read, lenth);
	char *pos1;
	char *pos2;
	pos1 = strstr(cmdline_read,"ubi.mtd=");
	if( pos1 == NULL) {
		dprintf(CRITICAL, "no ubi.mtd= in cmdline, error!\n");
		return -1;
	}
	pos2 = strstr(pos1, ",");
	if( pos2 == NULL) {
		dprintf(CRITICAL, "no ubi.mtd= in cmdline, error!\n");
		return -1;
	}
	if( (pos2 - pos1 -8) <=0 ) {
		dprintf(CRITICAL, "no ubi.mtd= in cmdline, error!\n");
		return -1;
	}
	
	char *mtdnum_str = NULL;
	char mtdnum_str_new[3];
	mtdnum_str = malloc((pos2-pos1-8)+1); /* The 8 is string "ubi.mtd=" length, 1 for the end of the string  */
	if (!mtdnum_str) {
		dprintf(CRITICAL, "malloc fail, mtdnum_str is NULL\n");
		return -1;
	}
	strncpy(mtdnum_str, pos1+8, (pos2-pos1-8));
	mtdnum_str[pos2-pos1-8] = '\0';
	int mtdnum = atoi(mtdnum_str);
	mtdnum ++;
	sprintf(mtdnum_str_new, "%d", mtdnum);
	if (mtdnum >= 10) {
        	char half_before[256] = {'\0'};
        	char half_behind[256] = {'\0'};
        	strncpy(half_before, cmdline_read, pos2-cmdline_read);
        	char *pos3 = strstr(half_before, "ubi.mtd=");
        	strncpy(pos3+8, mtdnum_str_new, 2);
        	strncpy(half_behind, pos2, strlen(pos2)+1);
        	cmdline_read = strncat(half_before, half_behind, strlen(half_behind));
	}
	else {
		strncpy(pos1+8, mtdnum_str_new, 1);
	}
	printf("cmdline new: %s , length: %d", cmdline_read, strlen(cmdline_read));
	ret = fdt_setprop(boot_dtb, chosen_node_offset, "bootargs", cmdline_read, strlen(cmdline_read) + 1);
	if (ret != 0) {
		dprintf(CRITICAL, "fdt_setprop error.\n");
		goto err;
	}
	ret = fdt_pack(boot_dtb);
	if (ret != 0) {
		dprintf(CRITICAL, "fdt_pack error.\n");
		goto err;
	}

	free(mtdnum_str);
	mtdnum_str= NULL;
	return 0;
err:
	free(mtdnum_str);
	mtdnum_str= NULL;
	return -1;
}

static bool download_check(void)
{
	//dprintf(ALWAYS,"%s\n",__func__);
    if (check_fastboot_mode()) {
        set_clr_fastboot_mode(false);
        return true;
    } else {
		if(check_uart_enter())
			fastboot_mode |= 0x01;
		if(check_download_key())
			fastboot_mode |= 0x02;
		if(check_usb_status())
			fastboot_mode |= 0x04;
		if(check_updown_key_status())
			fastboot_mode |= 0x08;
		if(fastboot_mode)
			return true;
		else
        return false;
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

static void do_load_scp(void)
{
#ifdef MTK_TINYSYS_SCP_SUPPORT
    if (load_scpsys() < 0) {
        /* if it load scp fail, turn off SCP hw*/
    }
#endif
}

static int fit_load_images(void *fit, struct fit_load_data *fit_data)
{
    int ret;
    int verify_kernel;
    int verify_ramdisk;
    int verify_fdt;
    int verify_trustedos;

    /* TODO: decide verify policy with config. currently verify kernel only. */
    verify_kernel = NEED_VERIFIED;
    verify_ramdisk = NEED_VERIFIED;
    verify_fdt = NEED_VERIFIED;
    verify_trustedos = NEED_VERIFIED;

    ret = fit_load_image(NULL, "kernel", fit, NULL, &fit_data->kernel_entry, verify_kernel);
    if (verify_kernel && ret &&(ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load kernel failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "tee", fit, NULL, &fit_data->trustedos_entry, verify_trustedos);
    if (verify_trustedos && ret&& (ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load trustedos failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "ramdisk", fit, NULL, NULL, verify_ramdisk);
    if (verify_ramdisk && ret &&(ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load ramdisk failed\n", fit_data->part_name);
        return ret;
    }

    ret = fit_load_image(NULL, "fdt", fit, &fit_data->dtb_load, NULL, verify_fdt);
    if (ret &&(ret != -ENOENT))
        fit_data->dtb_load = ERR_ADDR;

    if (verify_fdt && ret &&(ret != -ENOENT)) {
        dprintf(CRITICAL, "%s load fdt failed\n", fit_data->part_name);
        return ret;
    }

    return 0;
}


static int fit_load_thread(void *arg)
{
    int err;
    void *fit;
    struct fit_load_data *fit_data = (struct fit_load_data *)arg;

    if (fit_data->boot_mode == FASTBOOT_BOOT) {
        fit = fit_data->buf;
        err = fit_load_images(fit, fit_data);
        return err;
    }

    while (fit_data->boot_mode == NORMAL_BOOT) {
        err = fit_get_image(fit_data->part_name, &fit, fit_data->buf);
        if (err)
            break;

        err = fit_load_images(fit, fit_data);
        if (err)
            break;

        return 0;
    }

#if (MT8518 || MT8512)
	//remove recovery mode here----
    dprintf(CRITICAL, "%s try recovery mode !!\n", fit_data->part_name);
    // RECOVERY_BOOT
    err = fit_get_image(fit_data->recovery_part_name, &fit, fit_data->buf);
    if (err)
        return err;

    err = fit_load_images(fit, fit_data);
#endif

    return err;
}

extern void ext_boot(int mode);
#if FPGA_PLATFORM
static void fitboot_task(const struct app_descriptor *app, void *args)
{
    dprintf(ALWAYS, "fitboot_task \n");
	struct fit_load_data tz, bootimg;
    //ulong smc_fid = 0xc3200000UL; /* only used in ARCH_ARM64 */
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
	    /* load bl33 for tz to jump*/
    extern __WEAK paddr_t kvaddr_to_paddr(void *ptr);
	bootimg.dtb_load=0x50000000;
	bootimg.kernel_entry=0x40008000;
	tz.kernel_entry = 0x42ff6000;

    addr_t fdt_pa = bootimg.dtb_load;

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
#else
static void fitboot_task(const struct app_descriptor *app, void *args)
{
    void *fit;
    struct fit_load_data tz, bootimg;
    thread_t *tz_t, *bootimg_t;
    int ret_tz, ret_bootimg;
    int ret;
    u32 boot_mode = NORMAL_BOOT;

    uint bl33[] = { 0xea000005,  /* b BL33_32_ENTRY  | ands x5, x0, x0  */
                    0x58000160,  /* .word 0x58000160 | ldr x0, _X0      */
                    0x58000181,  /* .word 0x58000181 | ldr x1, _X1      */
                    0x580001a2,  /* .word 0x580001a2 | ldr x2, _X2      */
                    0x580001c3,  /* .word 0x580001c3 | ldr x3, _X3      */
                    0x580001e4,  /* .word 0x580001e4 | ldr x4, _X4      */
                    0xd61f0080,  /* .word 0xd61f0080 | br  x4           */
                    0xe59f0030,  /* BL33_32_ENTRY:   |                  */
                                 /*    ldr r0, _R0   | .word 0xe59f0030 */
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
    /* alloc kernel and tz buffer from mempool */
    kernel_buf = mempool_alloc(MAX_KERNEL_SIZE, MEMPOOL_ANY);
    tz_buf = mempool_alloc(MAX_TEE_DRAM_SIZE, MEMPOOL_ANY);
    dtbo_buf = mempool_alloc(MAX_DTBO_SIZE, MEMPOOL_ANY);
    if (!kernel_buf || !tz_buf || !dtbo_buf) {
        dprintf(CRITICAL, "alloc buf fail, kernel %p, tz %p, dtbo_buf %p\n",
                kernel_buf, tz_buf, dtbo_buf);
        return;
    }
#ifdef WITH_RECOVERYBOOT
    /* recovery */
    if (recovery_check()) {
        boot_mode = RECOVERY_BOOT;
    }
#endif

    /* fastboot */
    if (download_check()) {
        ext_boot(fastboot_mode);
        boot_mode = NORMAL_BOOT;//FASTBOOT_BOOT;
    }


    /* create a bootimg thread to load kernel + dtb */

    bootimg.part_name = (char *)BOOT_PART_NAME;

	/*1.choose A/B boot & tz img.*/
	u32 boot_part = 0;
	boot_part = check_boot_partition("misc");
	if (boot_part == 0) {
		dprintf(CRITICAL, "choose first boot partition:%s  , tee choose: %s\n",(char *)BOOT_PART_NAME, (char *)TZ_PART_NAME);
		bootimg.part_name = (char *)BOOT_PART_NAME;
		tz.part_name = (char *)TZ_PART_NAME;
		//cmdlineoverlay(bootimg.dtb_load, NULL, 0); from b partition,need to set

	} else if (boot_part == 1) {
		dprintf(CRITICAL, "choose second boot partition: %s  , tee choose: %s\n", (char *)RECOVERY_BOOT_PART_NAME, (char *)RECOVERY_TZ_PART_NAME);
		bootimg.part_name = (char *)RECOVERY_BOOT_PART_NAME;
		tz.part_name = (char *)RECOVERY_TZ_PART_NAME;

	} else {
		dprintf(CRITICAL, "unknow boot_part (%d), using first boot partition\n", boot_part);
		bootimg.part_name = (char *)BOOT_PART_NAME;
		tz.part_name = (char *)TZ_PART_NAME;
	}

    //bootimg.part_name = (char *)BOOT_PART_NAME;
    bootimg.recovery_part_name = (char *)RECOVERY_BOOT_PART_NAME;
    bootimg.boot_mode = boot_mode;
    bootimg.buf = kernel_buf;
    bootimg_t = thread_create("bootimg_ctl", fit_load_thread, &bootimg,
                              DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);

    /* create a tz thread to load tz */;
#if (MT8518 || MT8512)
    tz.part_name = (char *)TZ_PART_NAME;
#endif
    tz.recovery_part_name = (char *)RECOVERY_TZ_PART_NAME;
    tz.boot_mode = boot_mode;
    tz.buf = tz_buf;
    tz_t = thread_create("tz_ctl", fit_load_thread, &tz,
                         DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);


    if (!bootimg_t || !tz_t) {
        dprintf(CRITICAL, "create load threads failed\n");
        return;
    }

    thread_resume(bootimg_t);
    thread_resume(tz_t);
#if (MT8518 || MT8512)
    do_load_scp();
#endif


    thread_join(bootimg_t, &ret_bootimg, INFINITE_TIME);
    thread_join(tz_t, &ret_tz, INFINITE_TIME);

    if (ret_bootimg) {
        dprintf(CRITICAL, "load boot image failed\n");
        return;
    }

    if (ret_tz) {
        dprintf(CRITICAL, "load tz image failed\n");
        return;
    }

#if MT8516
#ifdef AUTO_DRAM_DETECT_BOOT
	dprintf(ALWAYS,"auto dram detect boot\n");
  	/*change dram and tee set size*/
  	fit_change_dtb_memorysize( (void *)bootimg.dtb_load ,tz.trustedos_entry );
#endif
#endif

#if !WITH_UBOOT
	/*2.overlay cmdline to choose A/B rootfs*/
	int cmd_ret = -1;
	if (boot_part == 1) {
		dprintf(CRITICAL, "load second partitions, need to overlay cmdline\n");
		cmd_ret = cmdlineoverlay((void *)bootimg.dtb_load, NULL, 0);
		if (cmd_ret)
		emmc_cmdlineoverlay((void *)bootimg.dtb_load, NULL, 0); //from b partition,need to set
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
#endif

    /* load bl33 for tz to jump*/
    extern __WEAK paddr_t kvaddr_to_paddr(void *ptr);
#if ARCH_ARM64 && WITH_KERNEL_VM
    paddr_t fdt_pa = kvaddr_to_paddr ? kvaddr_to_paddr((void *)bootimg.dtb_load) : bootimg.dtb_load;
#else
    paddr_t fdt_pa = bootimg.dtb_load;
#endif

#if WITH_UBOOT
    fdt_pa = BOOT_ARGUMENT_LOCATION;
#if ARCH_ARM64 && WITH_KERNEL_VM
    /* 64-bit LK use non identity mapping VA, VA to PA translation needed */
    fdt_pa = (ulong)kvaddr_to_paddr((void *)BOOT_ARGUMENT_LOCATION);
#endif
#endif

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

#endif
APP_START(fitboot)
.entry = fitboot_task,
 .flags = 0,
  APP_END

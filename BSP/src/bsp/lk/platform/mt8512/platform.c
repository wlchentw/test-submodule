/*
 * Copyright (c) 2018 MediaTek Inc.
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

#include <arch.h>
#if ARCH_ARM64
#include <arch/arm64/mmu.h>
#else
#include <arch/arm/mmu.h>
#endif
#include <arch/ops.h>
#include <assert.h>
#include <debug.h>
#include <dev/timer/arm_generic.h>
#include <dev/uart.h>
#include <err.h>
#include <kernel/vm.h>
#include <platform.h>
#include <platform/mt8512.h>
#include <platform/mt_gic_v3.h>
#include <platform/pll.h>
#include <platform/mt_i2c.h>
#include <platform/dramc_api.h>
#include <lib/mempool.h>
#include <platform/mtk_wdt.h>
#include <platform/mtk_timer.h>
#if WITH_PMIC_MT6398
#include <platform/pmic_6398.h>
#endif
#if WITH_PMIC_BD71828
#include <platform/bd71828.h>
#endif

#if WITH_MAX20342
#include <platform/max20342.h>
#endif

#if WITH_PMIC_M2296
#include <platform/m2296.h>
#endif
#if WITH_VCORE_PWM_BUCK
#include <platform/pwm-buck.h>
#endif
#if WITH_VCORE_I2C_BUCK
#include <platform/rt5748.h>
#endif
#include <platform/pwm.h>
#include <platform/mmc_core.h>

#include <platform/ntx_hw.h>
#include "../../include/version.h"
#include "drivers/gpio/gpio.h"

#if WITH_KERNEL_VM
#define L2C_MAPPING_IDX 0
#define PERIPHERAL_MAPPING_IDX 1
#define SRAM_MAPPING_IDX 5
#define DRAM_MAPPING_IDX 4
#define GIC_MAPPING_IDX 3

#include <platform/mtk_wdt.h>



/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    {
        .phys = MEMORY_BASE_PHYS,
        .virt = MEMORY_BASE_VIRT,
        .size = MEMORY_APERTURE_SIZE,
        .flags = 0,
        .name = "l2c"
    },
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
    },
    {
        .phys = VERSION_BASE_PHYS,
        .virt = VERSION_BASE_VIRT,
        .size = VERSION_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "icversion"
    },
    {
        .phys = GIC_BASE_PHYS,
        .virt = GIC_BASE_VIRT,
        .size = GIC_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "gic"
    },
    {
        .phys = DRAM_BASE_PHY,
        .virt = DRAM_BASE_VIRT,
#if WITH_KERNEL_VM
        .size = 0x8000000,
#else
        .size = 0x10000000,
#endif
        .flags = MMU_INITIAL_MAPPING_FLAG_UNCACHED,
        .name = "dram"
    },
    /* reserved for internal sram */
    { 0 },
    /* reserved for dram */
    { 0 },
    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "sdram",
    .base = SRAM_BASE_PHYS,
    .size = SRAM_BASE_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};


//int giPowerKeyState;


/* only enable el1 dcache */
static void dcache_enable(void)
{
    uint32_t sctlr;

    asm volatile("mrs %0, sctlr_el1" : "=r" (sctlr) : : "cc");
    asm volatile("msr sctlr_el1, %0" : : "r" (sctlr | (1 << 2)) : "cc");
    asm volatile("isb");
}
#if WITH_KERNEL_VM
uint32_t lk_dram_sz =  0x4000000; //get_dram_size();;
#else
uint32_t lk_dram_sz =  0x10000000; //get_dram_size();;
#endif

void *dram_map(paddr_t pa)
{
    paddr_t dram_phy = DRAM_BASE_PHY;

    if (pa >= dram_phy && pa <= (dram_phy + lk_dram_sz - 1)) {
        return (void *)(DRAM_BASE_VIRT + (pa - dram_phy));
    }

    return NULL;
}

#endif /* WITH_KERNEL_VM */

extern void mtk_wdt_init(void);
extern void mtk_wdt_transfer_status(void);

#if WITH_UBOOT
BOOT_ARGUMENT_T *g_boot_arg;

static void hex_dump(const char *prefix, unsigned char *buf, int len)
{
   int i;

   if(!buf || !len)
       return;

   dprintf(ALWAYS,"%s:\n", prefix);
   for (i = 0; i < len; i++) {
	   if (i != 0 && !(i % 16))
		 dprintf(ALWAYS,"\n");
	   dprintf(ALWAYS,"%02x", *(buf + i));
   }
   dprintf(ALWAYS,"\n");
}

/*
 * return 1 if rpmb key is provisioned else returns 0
 */
unsigned int get_rpmb_key_status(void)
{
    unsigned int ret = 0;
#if WITH_LIB_RPMB
    unsigned char blk[256];
    /* mmc_rpmb_block_read returns 0 only when the rpmb key is provisioned */
    ret = mmc_rpmb_block_read(0, blk) ? 0 : 1;
#endif
    return ret;
}

void platform_set_boot_args(void)
{
    int i = 0;
    int efuse_data_offset = 0;
    int len = sizeof(efuse_len_info) / sizeof((efuse_len_info)[0]);
    unsigned char data[64];


    g_boot_arg = (BOOT_ARGUMENT_T *)BOOT_ARGUMENT_LOCATION;
    memset((void *)g_boot_arg, 0, sizeof(BOOT_ARGUMENT_T));
    g_boot_arg->magic_number_begin = BOOT_ARGUMENT_MAGIC;
    g_boot_arg->magic_number_end = BOOT_ARGUMENT_MAGIC;
    g_boot_arg->dram_size = get_dram_size();
    g_boot_arg->rpmb_key_status = get_rpmb_key_status();
    g_boot_arg->powerkey_status = bd71828_powerkey_status();
    g_boot_arg->usb_status = bd71828_dcin_state();
	dprintf(ALWAYS,"%s boot_arg=0x%0x,powerkey=%d,dcin=%d\n",__func__,
			BOOT_ARGUMENT_LOCATION,(int)g_boot_arg->powerkey_status,(int)g_boot_arg->usb_status);
    config_updown_key();

    for (i = 0 ; i < len; i++) {
        seclib_efuse_read(i, data, efuse_len_info[i]);
        //printf("efuse index %d len: %d", i, efuse_len_info[i]);
        //hex_dump("efuse hex", data, efuse_len_info[i]);
        memcpy(g_boot_arg->efuse_data + efuse_data_offset, data, efuse_len_info[i]);
        efuse_data_offset = efuse_data_offset + efuse_len_info[i];
    }
    //hex_dump("BOOT_ARGUMENT_LOCATION hex", g_boot_arg, sizeof(BOOT_ARGUMENT_T));
}


void platform_early_init(void)
{
    uart_init_early();

    dprintf(ALWAYS, "\nLittle Kernel: %s\n\n", LK_VERSION);

#ifdef  DDR_RESERVE_MODE
    dprintf(ALWAYS, "WDT_MODE:0x%x, DRAMC_CTL:0x%x\n", readl(MTK_WDT_MODE), readl(MTK_WDT_DRAMC_CTL));
#endif

    dprintf(ALWAYS, "EP_EN 0\n");
	// EP_EN set output low . 
    mt_set_gpio_dir(GPIO11, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO11, GPIO_OUT_ZERO);

    dprintf(ALWAYS, "I2C0 OUT 0\n");
	// SDA0. SCL0 set output low . 
    mt_set_gpio_mode(GPIO64, 0);
    mt_set_gpio_dir(GPIO64, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO64, GPIO_OUT_ZERO);

    mt_set_gpio_mode(GPIO65, 0);
    mt_set_gpio_dir(GPIO65, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO65, GPIO_OUT_ZERO);

    //udelay(50);
    /* initialize the interrupt controller */
    arm_gic_init();

    arm_generic_timer_init(ARM_GENERIC_TIMER_PHYSICAL_INT, 13000000);
 
	//mdelay(10);
    dprintf(ALWAYS, "PWRALL 1\n");
	// PWRALL (EP_VMAX) set output low .
    mt_set_gpio_dir(GPIO13, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO13, GPIO_OUT_ONE);
	mdelay(21);
    dprintf(ALWAYS, "PWRALL->EP_VMAX 1\n");


 
    dprintf(ALWAYS, "i2c_hw_init()\n");
	i2c_hw_init();
#if !(FPGA_PLATFORM)
#ifdef  DDR_RESERVE_MODE
	check_ddr_reserve_status();
#endif
    mtk_wdt_init();
#endif
#if WITH_KERNEL_VM
    arch_disable_cache(DCACHE);
#endif

#if WITH_PMIC_M2296
    mt2296_sw_reset();
#endif

#if !(FPGA_PLATFORM)
    mt_pll_init();
#endif

    pwm_init();

#if WITH_PMIC_MT6398
    pmic_init_mt6398();
#endif


#if WITH_MAX20342
	max20342_init();
#endif 

#if WITH_PMIC_BD71828
    pmic_init_bd71828();
	ntx_led(NTX_ON_LED,1);
#endif

#if WITH_PMIC_M2296
    m2296_hw_init();
#endif

#if WITH_VCORE_PWM_BUCK
    pwm_buck_init();
#endif
#if !FPGA_PLATFORM && !SUPPORT_TYPE_PSRAM
{
	int val = 0;

	val = *(volatile unsigned int *)(IO_PHYS + 0x00232168);
	val |= (1<<23);
	*(volatile unsigned int *)(IO_PHYS + 0x00232168) = val;
}
#endif
#if WITH_VCORE_I2C_BUCK
    rt5749_init();
#endif

#if !(FPGA_PLATFORM)
    /* mt_pll_post_init should be invoked after pmic_init */
    mt_pll_post_init();
#endif

#if EMMC_PROJECT
    dprintf(ALWAYS, "EMMC project: dram init is after emmc init, refer to platform_init!\n");
#else
    dprintf(ALWAYS, "dram init is before emmc init!\n");
/* ******be careful: for emmc, move these to platform_init ****** */
#if !(FPGA_PLATFORM)
	/* check DDR-reserve mode */

    mt_mem_init();
#endif
    // dram init end

#if WITH_KERNEL_VM
    dcache_enable();
    /* ******be careful: for emmc, move these to platform_init ****** */
    /* add DRAM to mmu_initial_mappings for physical-to-virtual translation */
    mmu_initial_mappings[DRAM_MAPPING_IDX].phys = DRAM_BASE_PHY;
    mmu_initial_mappings[DRAM_MAPPING_IDX].virt = DRAM_BASE_VIRT;
#if FPGA_PLATFORM
    mmu_initial_mappings[DRAM_MAPPING_IDX].size = 0x10000000;
#else
    mmu_initial_mappings[DRAM_MAPPING_IDX].size = get_dram_size();
#endif
    mmu_initial_mappings[DRAM_MAPPING_IDX].flags = 0;
    mmu_initial_mappings[DRAM_MAPPING_IDX].name = "dram";

    /* ******be careful: for emmc, move these to platform_init ****** */
    /* mapping internel sram to cacheable memory */
    arch_mmu_map(SRAM_BASE_VIRT, SRAM_BASE_PHYS,  SRAM_BASE_SIZE >> PAGE_SIZE_SHIFT, 0);
    /* add intrenal sram to mmu_initial_mappings for heap */
    mmu_initial_mappings[SRAM_MAPPING_IDX].phys = SRAM_BASE_PHYS;
    mmu_initial_mappings[SRAM_MAPPING_IDX].virt = SRAM_BASE_VIRT;
    mmu_initial_mappings[SRAM_MAPPING_IDX].size = SRAM_BASE_SIZE;
    mmu_initial_mappings[SRAM_MAPPING_IDX].flags = 0;
    mmu_initial_mappings[SRAM_MAPPING_IDX].name = "sram";

    pmm_add_arena(&arena);
    /* ******be careful: for emmc, move these to platform_init ****** */
#endif

/* ******be careful: for emmc, move these to platform_init ****** */

#endif
}
static unsigned int tz_uffs(unsigned int x)
{
	unsigned int r = 1;
	if (!x)
		return 0;
	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}
	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}
	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}
	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}
	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}
	return r;
}

static void tz_set_field(volatile u32 *reg, u32 field, u32 val)
{
    u32 tv;
    if (field == 0)
        return;
    tv  = (u32)*reg;
    tv &= ~(field);
    tv |= ((val) << (tz_uffs((unsigned int)field) - 1));
    *reg = tv;
}
#define set_field(r,f,v)                tz_set_field((volatile u32*)r,f,v)
#define TZ_SET_FIELD(reg,field,val)     set_field(reg,field,val)
void platform_init(void)
{
    struct mmc_card *card;
    bool retry_opcond;
    int ret;

    /* power down brom */
    TZ_SET_FIELD(SRAMROM_SEC_CTRL_BASE, 0x1 << 31, 0x1);

#if EMMC_PROJECT
    /* emmc init */
	card = emmc_init_stage1(&retry_opcond);
    if (card) {
        emmc_init_stage2(card, retry_opcond);
    }
#endif


#if EMMC_PROJECT
    
#if !(FPGA_PLATFORM)
        /* check DDR-reserve mode */
    
        mt_mem_init();
#endif
        // dram init end
    
#if WITH_KERNEL_VM
        dcache_enable();
    
        /* add DRAM to mmu_initial_mappings for physical-to-virtual translation */
        mmu_initial_mappings[DRAM_MAPPING_IDX].phys = DRAM_BASE_PHY;
        mmu_initial_mappings[DRAM_MAPPING_IDX].virt = DRAM_BASE_VIRT;
#if FPGA_PLATFORM
        mmu_initial_mappings[DRAM_MAPPING_IDX].size = 0x10000000;
#else
        mmu_initial_mappings[DRAM_MAPPING_IDX].size = get_dram_size();
#endif
        mmu_initial_mappings[DRAM_MAPPING_IDX].flags = 0;
        mmu_initial_mappings[DRAM_MAPPING_IDX].name = "dram";
    
        /* mapping internel sram to cacheable memory */
        arch_mmu_map(SRAM_BASE_VIRT, SRAM_BASE_PHYS,  SRAM_BASE_SIZE >> PAGE_SIZE_SHIFT, 0);
        /* add intrenal sram to mmu_initial_mappings for heap */
        mmu_initial_mappings[SRAM_MAPPING_IDX].phys = SRAM_BASE_PHYS;
        mmu_initial_mappings[SRAM_MAPPING_IDX].virt = SRAM_BASE_VIRT;
        mmu_initial_mappings[SRAM_MAPPING_IDX].size = SRAM_BASE_SIZE;
        mmu_initial_mappings[SRAM_MAPPING_IDX].flags = 0;
        mmu_initial_mappings[SRAM_MAPPING_IDX].name = "sram";
    
        pmm_add_arena(&arena);
#endif
    
#endif
	ntx_led(NTX_ON_LED,0);

	load_hwconfig();

    // Turn on LED
	ntx_led(NTX_ON_LED,1);

    mtk_wdt_transfer_status();

    ret = mempool_init((void *)CACHED_MEMPOOL_ADDR, CACHED_MEMPOOL_SIZE,
                       MEMPOOL_CACHE);
    if (ret != NO_ERROR)
        platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_PANIC);

    ret = mempool_init((void *)UNCACHED_MEMPOOL_ADDR, UNCACHED_MEMPOOL_SIZE,
                       MEMPOOL_UNCACHE);
    if (ret != NO_ERROR)
        platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_PANIC);

    dprintf(ALWAYS, "calling platform_set_boot_args\n");
    platform_set_boot_args();
}

bool check_usb_status(void)
{
#if WITH_PMIC_BD71828
    NTX_HWCONFIG *hwcfg = gethwconfig();
    if(hwcfg->m_val.bPCB == E70T04)
        return false;
    else if(bd71828_dcin_state())
        return true;
#endif
	return false;
}

// E70T04 only
void config_updown_key()
{
    NTX_HWCONFIG *hwcfg = gethwconfig();
    if(hwcfg->m_val.bPCB == E70T04) {
        mt_set_gpio_pull_en(GPIO28, GPIO_PUEN_ENABLE);
        mt_set_gpio_pull_sel(GPIO28, GPIO_PUSEL_UP);   
        mt_set_gpio_pull_en(GPIO29, GPIO_PUEN_ENABLE);
        mt_set_gpio_pull_sel(GPIO29, GPIO_PUSEL_UP);      
        g_boot_arg->upkey_status = mt_get_gpio_in(GPIO28);
        g_boot_arg->downkey_status = mt_get_gpio_in(GPIO29);    
        dprintf(ALWAYS,"upkey_status=%d,downkey_status=%d\n",
            (int)g_boot_arg->upkey_status,(int)g_boot_arg->downkey_status);                
    }    
}

// E70T04 only
void save_updown_key_status()
{
    NTX_HWCONFIG *hwcfg = gethwconfig();
    
    if(hwcfg->m_val.bPCB == E70T04) {
        g_boot_arg->upkey_status = mt_get_gpio_in(GPIO28);
        g_boot_arg->downkey_status = mt_get_gpio_in(GPIO29);
    }
}

// E70T04 only
bool check_updown_key_status()
{
    NTX_HWCONFIG *hwcfg = gethwconfig();
    bool result = false;

    if(hwcfg->m_val.bPCB == E70T04) {
        result = (!g_boot_arg->upkey_status || !g_boot_arg->downkey_status);
    }
    return result;
}

#endif

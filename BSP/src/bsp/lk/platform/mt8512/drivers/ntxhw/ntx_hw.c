

#include <arch.h>
#if ARCH_ARM64
#include <arch/arm64/mmu.h>
#else
#include <arch/arm/mmu.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/ops.h>
#include <assert.h>
#include <debug.h>
#include <dev/timer/arm_generic.h>
#include <dev/uart.h>
#include <err.h>
#include <kernel/vm.h>
#include <platform.h>
#include "platform/ntx_hw.h"
#include <lib/bio.h>
#include <lib/partition.h>
#if WITH_PMIC_BD71828
#include <platform/bd71828.h>
#endif
#include "../drivers/gpio/gpio.h"



volatile NTX_HWCONFIG globalNtxHwCfg, *gptNtxHwCfg=0;


void load_hwconfig(void)
{
	bdev_t *bdev = NULL;
	char *hwconfig;

	dprintf(CRITICAL, "load_hwconfig\n");
	bdev = bio_open_by_label("hwcfg");
	if (!bdev) {
		dprintf(CRITICAL, "label hwcfg isn't exist.\n");
		return;
	}
	else {
		dprintf(CRITICAL, "label hwcfg exist.\n");
		{
			hwconfig = malloc(512);
			bio_read(bdev, hwconfig, 512, 512);
			memcpy(&globalNtxHwCfg, hwconfig, sizeof(NTX_HWCONFIG));
			free(hwconfig);
			gptNtxHwCfg = &globalNtxHwCfg;
			dprintf(CRITICAL, "hwconfig label %s\n", gptNtxHwCfg->m_hdr.cMagicNameA);
		}
		bio_close(bdev);
	}
}

NTX_HWCONFIG *gethwconfig(void)
{
	return (NTX_HWCONFIG *)gptNtxHwCfg;
}

static int _ntx_gpio_led(int onoff)
{
	int iRet;
	if(-1==onoff) {
		// querry led state . 
		iRet = mt_get_gpio_out(GPIO0)==GPIO_OUT_ONE?1:0;
	}
	else {
		mt_set_gpio_dir(GPIO0, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO0, onoff?GPIO_OUT_ONE:GPIO_IN_ZERO);
		iRet = 0 ;
	}
	return iRet;
}
static int _ntx_rohm_led(int onoff)
{
	int iRet;
	if(-1==onoff) {
		// querry led state .
		iRet = (bd71828_led_ctrl(BD71828_LED_ID_GRN,BD71828_LED_STAT_GET)==BD71828_LED_STAT_ON)?1:0; 
	}
	else {
		bd71828_led_ctrl(BD71828_LED_ID_GRN,onoff?BD71828_LED_STAT_OFF:BD71828_LED_STAT_ON);
		iRet = 0 ;
	}
	return iRet;
}

int ntx_led(int id,int onoff)
{
	int iRet ;

	switch(id)
	{
	case NTX_ON_LED:
		if(gptNtxHwCfg) {
			if( (0x6f==gptNtxHwCfg->m_val.bPCB) || (0x71==gptNtxHwCfg->m_val.bPCB) ) 
			{ // E70T04 | E60T04
				//dprintf(ALWAYS, "GPIO LED ON\n");
				iRet = _ntx_gpio_led(onoff);
			}
			else if (gptNtxHwCfg->m_val.bPCB == 0x6c) { // EA0T04 , LED control by ROHM
				iRet = _ntx_rohm_led(onoff);
			}
		}
		else {
			dprintf(ALWAYS, "turns on ON LED for all platform without hwconfig.\n");
			_ntx_gpio_led(onoff);
			_ntx_rohm_led(onoff);
			iRet = -1;
		}
		break;
	default :
		dprintf(CRITICAL,"unkown led id (%d)\n",id);
		iRet = -1 ;
		break;

	}
	return iRet;
}


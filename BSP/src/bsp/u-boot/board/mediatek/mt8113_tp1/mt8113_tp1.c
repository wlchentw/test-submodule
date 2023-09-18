// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc.
 */

//#define DEBUG
#include <common.h>
#include <dm.h>
#include <mapmem.h>

#include "mt8113_tp1_data.h"

DECLARE_GLOBAL_DATA_PTR;

BOOT_ARGUMENT_T *gpt_boot_args ;
void hex_dump(const char *prefix, unsigned char *buf, int len)
{
   int i;

   if (!buf || !len)
		return;

	debug("%s:\n", prefix);
	for (i = 0; i < len; i++) {
		if (i != 0 && !(i % 16))
			debug("\n");
		debug("%02x", *(buf + i));
	}
	debug("\n");
}

unsigned int get_dramsize_from_boot_args(void)
{
	unsigned int dram_size = 0;
	debug("BOOT_ARGUMENT_LOCATION = 0x%x\n", BOOT_ARGUMENT_LOCATION);
	hex_dump("BOOT_ARGUMENT_LOCATION hex:", (unsigned char *)BOOT_ARGUMENT_LOCATION, sizeof(BOOT_ARGUMENT_T));
	BOOT_ARGUMENT_T *g_boot_args = (BOOT_ARGUMENT_T *)BOOT_ARGUMENT_LOCATION;
	if (g_boot_args->magic_number_begin != BOOT_ARGUMENT_MAGIC
	   || g_boot_args->magic_number_end != BOOT_ARGUMENT_MAGIC) {
		printf("%s:boot arg magic error, please check the flow.\n", __func__);
		return 0;
	}

	dram_size = g_boot_args->dram_size;
	debug("g_boot_args->dram_size is 0x%x\n", g_boot_args->dram_size);
	return dram_size;
}
int board_init(void)
{
	unsigned int dram_size;

	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	debug("%s : gd->fdt_blob is %p\n",__FILE__,gd->fdt_blob);

	//gpt_boot_args=(BOOT_ARGUMENT_T *)BOOT_ARGUMENT_LOCATION;
	gpt_boot_args=(BOOT_ARGUMENT_T *)gd->bd->bi_boot_params;
	debug("bootarg @ %p,0x%x\n",gpt_boot_args,BOOT_ARGUMENT_LOCATION);
	
	debug("bootarg magic begin=0x%x\n",gpt_boot_args->magic_number_begin);
	debug("bootarg magic end=0x%x\n",gpt_boot_args->magic_number_end);

	debug("powerkey_status=0x%x\n",gpt_boot_args->powerkey_status);
	debug("usb_status=0x%x\n",gpt_boot_args->usb_status);
	return 0;
}


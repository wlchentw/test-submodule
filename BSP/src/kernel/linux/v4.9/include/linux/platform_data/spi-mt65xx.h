/*
 *  MTK SPI bus driver definitions
 *
 * Copyright (c) 2015 MediaTek Inc.
 * Author: Leilk Liu <leilk.liu@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ____LINUX_PLATFORM_DATA_SPI_MTK_H
#define ____LINUX_PLATFORM_DATA_SPI_MTK_H

/* Board specific platform_data */
struct mtk_chip_config {
	u32 tx_mlsb;
	u32 rx_mlsb;
	u32 cs_pol;
	u32 sample_sel;
	u32 command_cnt;
	u32 dummy_cnt;
	u32 get_tick_dly;
};
#endif

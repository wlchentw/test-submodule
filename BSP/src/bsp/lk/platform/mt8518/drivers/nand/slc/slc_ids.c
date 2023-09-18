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
#include "slc.h"
#include "slc_os.h"

#define NAND_OPTIONS_NONE    0
#define NFI_DEFAULT_ACTIMING 0x10804011

struct mtk_nand_flash_dev nand_flash_devs[] = {
	/* MXIC */
	{
		"MX30LF2G18AC", {0xc2, 0xda, 0x90, 0x95, 0x6, 0, 0, 0},
		5, KB(256), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MX30LF4G18AC", {0xc2, 0xdc, 0x90, 0x95, 0x56, 0, 0, 0},
		5, KB(512), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MX60LF8G18AC", {0xc2, 0xd3, 0xd1, 0x95, 0x5a, 0x00},
		5, KB(1024), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	/* Micron */
	{
		"MT29F2G08ABAEA", {0x2c, 0xda, 0x90, 0x95, 0x06, 0, 0, 0},
		5, KB(256), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MT29F4G08ABAEA", {0x2c, 0xdc, 0x90, 0xa6, 0x54, 0x00},
		5, KB(512), KB(512), 4096, 224, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MT29F8G08ABABA", {0x2C, 0x38, 0x00, 0x26, 0x85, 0x0, 0},
		5, KB(1024), KB(512), 4096, 224, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MT29F8G08ABACA", {0x2c, 0xd3, 0x90, 0xa6, 0x64, 0x00},
		5, KB(256), KB(256), 4096, 224, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"MT29F16G08ADBCA", {0x2c, 0xa5, 0xd1, 0x26, 0x68, 0, 0, 0},
		5, KB(2048), KB(256), 4096, 224, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	/* Toshiba */
	{
		"TC58NYG1S3HBAI6", {0x98, 0xaa, 0x90, 0x15, 0x76, 0x16, 0, 0},
		6, KB(256), KB(128), 2048, 128, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"TC58BVG1S3HTA00", {0x98, 0xda, 0x90, 0x15, 0xf6, 0x00, 0, 0},
		5, KB(256), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	{
		"TC58NVG2S0HTA00", {0x98, 0xdc, 0x90, 0x26, 0x76, 0x16, 0, 0},
		6, KB(512), KB(256), 4096, 256, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 24,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	/* Samsung */
	{
		"K9F2G08U0D", {0xec, 0xda, 0x10, 0x95, 0x46, 0, 0, 0},
		5, KB(256), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 12,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},
	/* ESMT */
	{
		"F59D4G81A-45TG-18V", {0xc8, 0xac, 0x90, 0x15, 0x54, 0, 0, 0},
		5, KB(512), KB(128), 2048, 64, 1, 1, NFI_DEFAULT_ACTIMING, 1024, 4,
		NAND_OPTIONS_NONE, NAND_OPTIONS_NONE
	},

	{NULL}
};


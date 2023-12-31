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
#include "mtk_nand_nal.h"

#define NAND_OPTIONS_NONE	(0)
#define KB(x)	((x) * 1024UL)

struct mtk_nand_flash_dev nand_flash_devs[] = {
	{"K9F1G08U0F", {0xec, 0xf1, 0x00, 0x95, 0x42, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"K9F2G08U0D", {0xec, 0xda, 0x10, 0x95, 0x46, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"H27U2G8F2D", {0xad, 0xda, 0x90, 0x95, 0x46, 0, 0}, 5, KB(256), KB(128), 2048, 128, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MX30LF1G18AC", {0xc2, 0xf1, 0x80, 0x95, 0x02, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10404011, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MX30LF2G18AC", {0xc2, 0xda, 0x90, 0x95, 0x06, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10404011, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MX30LF4G18AC", {0xc2, 0xdc, 0x90, 0x95, 0x56, 0, 0}, 5, KB(512), KB(128), 2048, 64, 8, 1, 0x10404011, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MT29F1G08ABAEA", {0x2c, 0xf1, 0x80, 0x95, 0x04, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	/* {"MT29F2G08ABAEA", {0x2c, 0xda, 0x90, 0x95, 0x06, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10404011, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE}, */
	{"MT29F4G08ABAEA", {0x2C, 0xDC, 0x90, 0xA6, 0x54, 0, 0}, 5, KB(512), KB(256), 4096, 224, 8, 1, 0x10804111, 1024, 24, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MT29F4G08ABAFA", {0x2C, 0xDC, 0x80, 0xA6, 0x62, 0, 0}, 5, KB(512), KB(256), 4096, 256, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MT29F8G08ABABA", {0x2C, 0x38, 0x00, 0x26, 0x85, 0, 0}, 5, KB(1024), KB(256), 4096, 224, 8, 1, 0x10804111, 1024, 24, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MT29F8G08ABACA", {0x2C, 0xD3, 0x90, 0xA6, 0x64, 0, 0}, 5, KB(1024), KB(256), 4096, 224, 8, 1, 0x10804111, 1024, 24, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"MT29F16G08ADBCA", {0x2c, 0xa5, 0xd1, 0x26, 0x68, 0, 0}, 5, KB(2048), KB(256), 4096, 224, 8, 1, 0x10404011, 1024, 24, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"TC58NVG0S3HTA00", {0x98, 0xf1, 0x80, 0x15, 0x72, 0, 0}, 5, KB(128), KB(128), 2048, 128, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"TC58NVG1S3HTA00", {0x98, 0xda, 0x90, 0x15, 0x76, 0, 0}, 5, KB(256), KB(128), 2048, 128, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"TC58BVG1S3HTA00", {0x98, 0xda, 0x90, 0x15, 0xF6, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"TC58NVG2S0HTA00", {0x98, 0xDC, 0x90, 0x26, 0x76, 0, 0}, 5, KB(512), KB(256), 4096, 256, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"TC58NVG3S0HTA00", {0x98, 0xD3, 0x91, 0x26, 0x76, 0, 0}, 5, KB(1024), KB(256), 4096, 256, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"W29N01HVSINA", {0xef, 0xf1, 0x00, 0x95, 0, 0, 0, 0, 0}, 4, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"W29N02GVSIAA", {0xef, 0xda, 0x90, 0x95, 0x04, 0, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"W29N04GVSIAA", {0xef, 0xdc, 0x90, 0x95, 0x54, 0, 0, 0}, 5, KB(512), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"W29N08GVSIAA", {0xef, 0xd3, 0x91, 0x95, 0x58, 0, 0, 0}, 4, KB(1024), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"GD9FU1G8F2AMGF", {0xc8, 0xf1, 0x80, 0x1d, 0x42, 0, 0}, 5, KB(128), KB(128), 2048, 128, 8, 1, 0x10804111, 1024, 32, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"F59L1G81MB-25T", {0xc8, 0xd1, 0x80, 0x95, 0x40, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"F59D4G81A-45TG", {0xc8, 0xac, 0x90, 0x15, 0x54, 0, 0}, 5, KB(512), KB(128), 2048, 64, 8, 1, 0x10804122, 1024, 4, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"PSU1GA30BT", {0xc8, 0xd1, 0x80, 0x95, 0x42, 0, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"PSU2GA30BT", {0xc8, 0xda, 0x90, 0x95, 0x44, 0, 0, 0}, 5, KB(256), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"JS27HU1G08SCN", {0xad, 0xf1, 0x80, 0x1d, 0, 0, 0, 0}, 4, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"PN27G01BBGITG", {0x98, 0xf1, 0x80, 0x15, 0xf2, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"FMND1G08U3D-1A", {0xf8, 0xf1, 0x80, 0x95, 0, 0, 0}, 4, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},
	{"FM29G01C", {0xec, 0xf1, 0x00, 0x95, 0x42, 0, 0}, 5, KB(128), KB(128), 2048, 64, 8, 1, 0x10804111, 1024, 12, NAND_OPTIONS_NONE, NAND_OPTIONS_NONE},

	{NULL}
};


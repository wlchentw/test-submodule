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
#pragma once

#include <platform/mt_reg_base.h>
#include <stdbool.h>
#include <platform/mt8512.h>

#define ENABLE_WDT_MODULE       (1) /* Module switch */
#define LK_WDT_DISABLE          (0)

#define MTK_WDT_BASE            RGU_BASE  

#define MTK_WDT_MODE            (MTK_WDT_BASE+0x0000)
#define MTK_WDT_LENGTH          (MTK_WDT_BASE+0x0004)
#define MTK_WDT_RESTART         (MTK_WDT_BASE+0x0008)
#define MTK_WDT_STATUS          (MTK_WDT_BASE+0x000C)
#define MTK_WDT_INTERVAL        (MTK_WDT_BASE+0x0010)
#define MTK_WDT_SWRST           (MTK_WDT_BASE+0x0014)
#define MTK_WDT_SWSYSRST        (MTK_WDT_BASE+0x0018)
#define MTK_WDT_NONRST_REG      (MTK_WDT_BASE+0x0020)
#define MTK_WDT_NONRST_REG2     (MTK_WDT_BASE+0x0024)
#define MTK_WDT_REQ_MODE        (MTK_WDT_BASE+0x0030)
#define MTK_WDT_REQ_IRQ_EN      (MTK_WDT_BASE+0x0034)
#define MTK_WDT_DRAMC_CTL       (MTK_WDT_BASE+0x0040)
#define MTK_WDT_DEBUG_2_REG		(MTK_WDT_BASE+0x0508)

/*WDT_MODE*/
#define MTK_WDT_MODE_KEYMASK        (0xff00)
#define MTK_WDT_MODE_KEY        (0x22000000)
#define MTK_WDT_MODE_DDR_RESERVE  (0x0080)

#define MTK_WDT_MODE_DUAL_MODE  (0x0040)
#define MTK_WDT_MODE_IN_DIS     (0x0020) /* Reserved */
#define MTK_WDT_MODE_AUTO_RESTART   (0x0010) /* Reserved */
#define MTK_WDT_MODE_IRQ        (0x0008)
#define MTK_WDT_MODE_EXTEN      (0x0004)
#define MTK_WDT_MODE_EXT_POL        (0x0002)
#define MTK_WDT_MODE_ENABLE     (0x0001)

/*WDT_LENGTH*/
#define MTK_WDT_LENGTH_TIME_OUT     (0xffe0)
#define MTK_WDT_LENGTH_KEYMASK      (0x001f)
#define MTK_WDT_LENGTH_KEY      (0x0008)

/*WDT_RESTART*/
#define MTK_WDT_RESTART_KEY     (0x1971)

/*WDT_STATUS*/
#define MTK_WDT_STATUS_HWWDT_RST    (0x80000000)
#define MTK_WDT_STATUS_SWWDT_RST    (0x40000000)
#define MTK_WDT_STATUS_IRQWDT_RST   (0x20000000)
#define MTK_WDT_STATUS_SECURITY_RST (1<<28)
#define MTK_WDT_STATUS_DEBUGWDT_RST (0x00080000)
#define MTK_WDT_STATUS_THERMAL_DIRECT_RST   (1<<18)
#define MTK_WDT_STATUS_SPMWDT_RST          (0x0002)
#define MTK_WDT_STATUS_SPM_THERMAL_RST     (0x0001)

//MTK_WDT_DEBUG_CTL
#define MTK_DEBUG_CTL_KEY           (0x59000000)
#define MTK_RG_DDR_PROTECT_EN       (0x00001)
#define MTK_RG_MCU_LATH_EN          (0x00002)
#define MTK_RG_DRAMC_SREF           (0x00100)
#define MTK_RG_DRAMC_ISO            (0x00200)
#define MTK_RG_CONF_ISO             (0x00400)
#define MTK_DDR_RESERVE_RTA         (0x10000)  //sta
#define MTK_DDR_SREF_STA            (0x20000)  //sta

/*WDT_INTERVAL*/
#define MTK_WDT_INTERVAL_MASK       (0x0fff)

/*WDT_SWRST*/
#define MTK_WDT_SWRST_KEY       (0x1209)

/*WDT_SWSYSRST*/
#define MTK_WDT_SWSYS_RST_PWRAP_SPI_CTL_RST (0x0800)
#define MTK_WDT_SWSYS_RST_APMIXED_RST   (0x0400)
#define MTK_WDT_SWSYS_RST_MD_LITE_RST   (0x0200)
#define MTK_WDT_SWSYS_RST_INFRA_AO_RST  (0x0100)
#define MTK_WDT_SWSYS_RST_MD_RST    (0x0080)
#define MTK_WDT_SWSYS_RST_DDRPHY_RST    (0x0040)
#define MTK_WDT_SWSYS_RST_IMG_RST   (0x0020)
#define MTK_WDT_SWSYS_RST_VDEC_RST  (0x0010)
#define MTK_WDT_SWSYS_RST_VENC_RST  (0x0008)
#define MTK_WDT_SWSYS_RST_MFG_RST   (0x0004)
#define MTK_WDT_SWSYS_RST_DISP_RST  (0x0002)
#define MTK_WDT_SWSYS_RST_INFRA_RST (0x0001)

#define MTK_WDT_SWSYS_RST_KEY       (0x88000000)

typedef enum wd_swsys_reset_type {
    WD_MD_RST,
} WD_SYS_RST_TYPE;

void set_clr_fastboot_mode(bool flag);
void set_clr_recovery_mode(bool flag);
bool check_fastboot_mode(void);
bool check_recovery_mode(void);
void mtk_wdt_init(void);
void mtk_wdt_disable(void);
int rgu_dram_reserved(int enable);
int rgu_is_reserve_ddr_enabled(void);

int rgu_is_dram_slf(void);

void rgu_release_rg_dramc_conf_iso(void);

void rgu_release_rg_dramc_iso(void);

void rgu_release_rg_dramc_sref(void);
int rgu_is_reserve_ddr_mode_success(void);
void mtk_arch_reset(char mode);

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

#include <platform/mt8518.h>

/* I/O mapping */
#define IO_PHYS             PERIPHERAL_BASE_VIRT
#define IO_SIZE             PERIPHERAL_BASE_SIZE

/* IO register definitions */
#define EMI_BASE            (IO_PHYS + 0x00205000)
#define GPIO_BASE           (IO_PHYS + 0x00005000)
#define MCUSYS_CFGREG_BASE  (IO_PHYS + 0x00200000)
#define TRNG_BASE           (IO_PHYS + 0x0020C000)

// gic
#define GIC_DIST_BASE   (MEMORY_BASE_VIRT+0x0C000000)
#define GIC_REDIS_BASE  (MEMORY_BASE_VIRT+0x0C100000)

// APB Module mcucfg
#define MCUCFG_BASE         (IO_PHYS + 0x00200000)

#define UART1_BASE          (IO_PHYS + 0x01005000)
#define UART2_BASE          (IO_PHYS + 0x0001A000)
#define UART3_BASE          (IO_PHYS + 0x01007000)
#define UART4_BASE          (IO_PHYS + 0x01007500)

#define MSDC0_BASE          (IO_PHYS + 0x01120000)
#define MSDC1_BASE          (IO_PHYS + 0x01130000)

#define USB0_BASE           (IO_PHYS + 0x01100000)
#define USBSIF_BASE         (IO_PHYS + 0x01110000)
#define USB_BASE            (USB0_BASE)
#define RGU_BASE            (IO_PHYS + 0x7000)

#define NFI_BASE            (IO_PHYS + 0x1001000)
#define NFIECC_BASE         (IO_PHYS + 0x1002000)

#define SEJ_BASE            (IO_PHYS + 0x000A000)
#define SCP_BASE_SRAM       (IO_PHYS + 0x1800000)
#define SCP_BASE_CFG        (IO_PHYS + 0x1820000)

#define PWM_BASE            (IO_PHYS + 0x00015000)

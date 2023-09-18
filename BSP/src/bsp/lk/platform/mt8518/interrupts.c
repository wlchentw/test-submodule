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
#include <arch/arm64.h>
#include <reg.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform/gic.h>
#include <platform/mt_irq.h>
#include <platform/mt_gic_v3.h>
#include <platform/interrupts.h>
#include <platform/mt_reg_base.h>

/* set for mt_gic */
void mt_irq_set_polarity(unsigned int irq, unsigned int polarity)
{
    unsigned int offset;
    unsigned int reg_index;
    unsigned int value;

    /* peripheral device's IRQ line is using GIC's SPI, and line ID >= GIC_PRIVATE_SIGNALS */
    if (irq < GIC_PRIVATE_SIGNALS) {
        dprintf(SPEW, "The Interrupt ID < 32, please check!\n");
        return;
    }

    offset = (irq - GIC_PRIVATE_SIGNALS) & 0x1F;
    reg_index = (irq - GIC_PRIVATE_SIGNALS) >> 5;
    if (polarity == 0) {
        value = readl(INT_POL_CTL0 + (reg_index * 4));
        value |= (1 << offset); /* always invert the incoming IRQ's polarity */
        writel(value, (INT_POL_CTL0 + (reg_index * 4)));
    } else {
        value = readl(INT_POL_CTL0 + (reg_index * 4));
        value &= ~(0x1 << offset);
        writel(value, INT_POL_CTL0 + (reg_index * 4));
    }
}

/* set for arm gic */
void mt_irq_set_sens(unsigned int irq, unsigned int sens)
{
    unsigned int config;

    if (sens == EDGE_SENSITIVE) {
        config = readl(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config |= (0x2 << (irq % 16) * 2);
        writel(config, GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
    } else {
        config = readl(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        config &= ~(0x2 << (irq % 16) * 2);
        writel(config,  GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
    }
    DSB;
}

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

#ifndef __MT_IRQ_H__
#define __MT_IRQ_H__

#define GIC_CPU_CTRL            0x00
#define GIC_CPU_PRIMASK         0x04
#define GIC_CPU_BINPOINT        0x08
#define GIC_CPU_INTACK          0x0c
#define GIC_CPU_EOI             0x10
#define GIC_CPU_RUNNINGPRI      0x14
#define GIC_CPU_HIGHPRI         0x18

#define GIC_DIST_CTRL           0x000
#define GIC_DIST_CTR            0x004
#define GIC_DIST_ENABLE_SET     0x100
#define GIC_DIST_ENABLE_CLEAR   0x180
#define GIC_DIST_PENDING_SET    0x200
#define GIC_DIST_PENDING_CLEAR  0x280
#define GIC_DIST_ACTIVE_BIT     0x300
#define GIC_DIST_PRI            0x400
#define GIC_DIST_TARGET         0x800
#define GIC_DIST_CONFIG         0xc00
#define GIC_DIST_SOFTINT        0xf00
#define GIC_DIST_ICDISR         0x80
enum {IRQ_MASK_HEADER = 0xF1F1F1F1, IRQ_MASK_FOOTER = 0xF2F2F2F2};

struct mtk_irq_mask {
    unsigned int header;   /* for error checking */
    unsigned int mask0;
    unsigned int mask1;
    unsigned int mask2;
    unsigned int mask3;
    unsigned int mask4;
    unsigned int mask5;
    unsigned int mask6;
    unsigned int mask7;
    unsigned int footer;   /* for error checking */
};


/*
 * Define hadware registers.
 */

/*
 * Define IRQ code.
 */

#define GIC_PRIVATE_SIGNALS (32)

#define GIC_PPI_OFFSET          (27)
#define GIC_PPI_GLOBAL_TIMER    (GIC_PPI_OFFSET + 0)
#define GIC_PPI_LEGACY_FIQ      (GIC_PPI_OFFSET + 1)
#define GIC_PPI_PRIVATE_TIMER   (GIC_PPI_OFFSET + 2)
#define GIC_PPI_WATCHDOG_TIMER  (GIC_PPI_OFFSET + 3)
#define GIC_PPI_LEGACY_IRQ      (GIC_PPI_OFFSET + 4)


#define ARM_GENERIC_TIMER_NONSECURE_PHYSICAL_INT   30

#define MT_GPT_IRQ_ID   117
#define SSUSB_DEV_INT_ID  48
#define MT_MSDC0_IRQ_ID 52
#define MT_MSDC1_IRQ_ID 53

#define MT_NR_PPI   (5)
#define MT_NR_SPI   (221)
#define NR_IRQ_LINE  (GIC_PPI_OFFSET + MT_NR_PPI + MT_NR_SPI)    // 5 PPIs and 224 SPIs

#define EDGE_SENSITIVE 0
#define LEVEL_SENSITIVE 1

#define MT65xx_POLARITY_LOW   0
#define MT65xx_POLARITY_HIGH  1


int mt_irq_mask_all(struct mtk_irq_mask *mask); //(This is ONLY used for the sleep driver)
int mt_irq_mask_restore(struct mtk_irq_mask *mask); //(This is ONLY used for the sleep driver)
void mt_irq_set_sens(unsigned int irq, unsigned int sens);
void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int mt_irq_mask_restore(struct mtk_irq_mask *mask);
void mt_irq_unmask(unsigned int irq);
void mt_irq_ack(unsigned int irq);

void platform_init_interrupts(void);

#endif  /* !__MT_IRQ_H__ */

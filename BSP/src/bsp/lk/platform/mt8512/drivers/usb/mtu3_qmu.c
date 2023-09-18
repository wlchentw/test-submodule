/*
 * mtu3_qmu.c - Queue Management Unit driver for device controller
 *
 * Copyright (C) 2018 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
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
 *
 */

/*
 * Queue Management Unit (QMU) is designed to unload SW effort
 * to serve DMA interrupts.
 * By preparing General Purpose Descriptor (GPD) and Buffer Descriptor (BD),
 * SW links data buffers and triggers QMU to send / receive data to
 * host / from device at a time.
 * And now only GPD is supported.
 *
 * For more detailed information, please refer to QMU Programming Guide
 */

#include <arch/ops.h>
#include <debug.h>
#include <errno.h>
#include <kernel/vm.h>
#include <lib/mempool.h>
#include <platform/reg_utils.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "mtu3.h"
#include "mtu3_hw_regs.h"
#include "mtu3_qmu.h"

#pragma GCC push_options
#pragma GCC optimize("O1")

#define QMU_CHECKSUM_LEN    16

#define GPD_FLAGS_HWO   BIT(0)
#define GPD_FLAGS_BDP   BIT(1)
#define GPD_FLAGS_BPS   BIT(2)
#define GPD_FLAGS_IOC   BIT(7)

#define GPD_EXT_FLAG_ZLP    BIT(5)
#undef INFO
#define INFO 2
#define SPEW 2

#define DBG_C(x...) dprintf(CRITICAL, "[USB][QMU] " x)
#define DBG_I(x...) dprintf(INFO, "[USB][QMU] " x)
#define DBG_S(x...) dprintf(SPEW, "[USB][QMU] " x)

#ifdef SUPPORT_QMU

static paddr_t va_to_pa(void *vaddr)
{
#if WITH_KERNEL_VM
    return kvaddr_to_paddr(vaddr);
#else
    return (paddr_t)vaddr;
#endif
}

static void *pa_to_va(paddr_t paddr)
{
#if WITH_KERNEL_VM
    return paddr_to_kvaddr(paddr);
#else
    return (void *)paddr;
#endif
}

static struct qmu_gpd *gpd_dma_to_virt(struct mtu3_gpd_ring *ring,
                                       paddr_t dma_addr)
{
    paddr_t dma_base = ring->dma;
    struct qmu_gpd *gpd_head = ring->start;
    u32 offset = (dma_addr - dma_base) / sizeof(*gpd_head);

    if (offset >= MAX_GPD_NUM)
        return NULL;

    return gpd_head + offset;
}

static paddr_t gpd_virt_to_dma(struct mtu3_gpd_ring *ring,
                               struct qmu_gpd *gpd)
{
    paddr_t dma_base = ring->dma;
    struct qmu_gpd *gpd_head = ring->start;
    u32 offset;

    offset = gpd - gpd_head;
    if (offset >= MAX_GPD_NUM)
        return 0;

    return dma_base + (offset * sizeof(*gpd));
}

static void gpd_ring_init(struct mtu3_gpd_ring *ring, struct qmu_gpd *gpd)
{
    ring->start = gpd;
    ring->enqueue = gpd;
    ring->dequeue = gpd;
    ring->end = gpd + MAX_GPD_NUM - 1;
}

static void reset_gpd_list(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->start;

    if (gpd) {
        gpd->flag &= ~GPD_FLAGS_HWO;
        gpd_ring_init(ring, gpd);
    }
}

int mtu3_gpd_ring_alloc(struct udc_endpoint *mep)
{
    struct qmu_gpd *gpd;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    u32 size;

    /* software own all gpds as default */
    size = sizeof(struct qmu_gpd) * MAX_GPD_NUM;
    gpd = (struct qmu_gpd *)mempool_alloc(size, MEMPOOL_UNCACHE);
    if (gpd == NULL)
        return -ENOMEM;

    memset(gpd, 0, size);
    ring->dma = va_to_pa(gpd);
    gpd_ring_init(ring, gpd);
    return 0;
}

void mtu3_gpd_ring_free(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;

    mempool_free(ring->start);
    memset(ring, 0, sizeof(*ring));
}

/*
 * calculate check sum of a gpd or bd
 * add "noinline" and "mb" to prevent wrong calculation
 */
static u8 qmu_calc_checksum(u8 *data)
{
    u8 chksum = 0;
    int i;

    data[1] = 0x0;  /* set checksum to 0 */

    mb();   /* ensure the gpd/bd is really up-to-date */
    for (i = 0; i < QMU_CHECKSUM_LEN; i++)
        chksum += data[i];

    /* Default: HWO=1, @flag[bit0] */
    chksum += 1;

    return 0xFF - chksum;
}

void mtu3_qmu_resume(struct udc_endpoint *mep)
{
    int epnum = mep->num;
    paddr_t qcsr;

    qcsr = mep->in ? USB_QMU_TQCSR(epnum) : USB_QMU_RQCSR(epnum);

    writel(QMU_Q_RESUME, qcsr);
    if (!(readl(qcsr) & QMU_Q_ACTIVE))
        writel(QMU_Q_RESUME, qcsr);
}

static struct qmu_gpd *advance_enq_gpd(struct mtu3_gpd_ring *ring)
{
    if (ring->enqueue < ring->end)
        ring->enqueue++;
    else
        ring->enqueue = ring->start;

    return ring->enqueue;
}

static struct qmu_gpd *advance_deq_gpd(struct mtu3_gpd_ring *ring)
{
    if (ring->dequeue < ring->end)
        ring->dequeue++;
    else
        ring->dequeue = ring->start;

    return ring->dequeue;
}

/* check if a ring is emtpy */
static int gpd_ring_empty(struct mtu3_gpd_ring *ring)
{
    struct qmu_gpd *enq = ring->enqueue;
    struct qmu_gpd *next;

    if (ring->enqueue < ring->end)
        next = enq + 1;
    else
        next = ring->start;

    /* one gpd is reserved to simplify gpd preparation */
    return next == ring->dequeue;
}

int mtu3_prepare_transfer(struct udc_endpoint *mep)
{
    return gpd_ring_empty(&mep->gpd_ring);
}

static int mtu3_prepare_tx_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    struct qmu_gpd *enq;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->enqueue;
    struct udc_request *req = &mreq->req;

    /* set all fields to zero as default value */
    memset(gpd, 0, sizeof(*gpd));

    gpd->buffer = (u32)va_to_pa(req->buffer);
    gpd->buf_len = (req->length);
    gpd->flag |= GPD_FLAGS_IOC;

    /* get the next GPD */
    enq = advance_enq_gpd(ring);
    DBG_I("TX %s queue gpd=%p, enq=%p\n", mep->name, gpd, enq);

    enq->flag &= ~GPD_FLAGS_HWO;
    gpd->next_gpd = (u32)gpd_virt_to_dma(ring, enq);

    if (mep->type != USB_EP_XFER_ISO)
        gpd->ext_flag |= GPD_EXT_FLAG_ZLP;

    gpd->chksum = qmu_calc_checksum((u8 *)gpd);
    gpd->flag |= GPD_FLAGS_HWO;

    mreq->gpd = gpd;

    return 0;
}

static int mtu3_prepare_rx_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    struct qmu_gpd *enq;
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->enqueue;
    struct udc_request *req = &mreq->req;

    /* set all fields to zero as default value */
    memset(gpd, 0, sizeof(*gpd));

    gpd->buffer = (u32)va_to_pa(req->buffer);
    gpd->data_buf_len = req->length;
    gpd->flag |= GPD_FLAGS_IOC;

    /* get the next GPD */
    enq = advance_enq_gpd(ring);
    DBG_I("RX %s queue gpd=%p, enq=%p\n", mep->name, gpd, enq);

    enq->flag &= ~GPD_FLAGS_HWO;
    gpd->next_gpd = (u32)gpd_virt_to_dma(ring, enq);
    gpd->chksum = qmu_calc_checksum((u8 *)gpd);
    gpd->flag |= GPD_FLAGS_HWO;

    mreq->gpd = gpd;

    return 0;
}

void mtu3_insert_gpd(struct udc_endpoint *mep, struct mu3d_req *mreq)
{
    if (mep->in)
        mtu3_prepare_tx_gpd(mep, mreq);
    else
        mtu3_prepare_rx_gpd(mep, mreq);
}

int mtu3_qmu_start(struct udc_endpoint *mep)
{
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    u8 epnum = mep->num;

    if (mep->in) {
        /* set QMU start address */
        writel(ring->dma, USB_QMU_TQSAR(mep->num));
        setbits32_r(TX_DMAREQEN, MU3D_EP_TXCR0(mep->num));
        setbits32_r(QMU_TX_CS_EN(epnum), U3D_QCR0);
        /* send zero length packet according to ZLP flag in GPD */
        setbits32_r(QMU_TX_ZLP(epnum), U3D_QCR1);
        writel(QMU_TX_LEN_ERR(epnum) | QMU_TX_CS_ERR(epnum), U3D_TQERRIESR0);

        if (readl(USB_QMU_TQCSR(epnum)) & QMU_Q_ACTIVE) {
            DBG_C("%s Active Now!\n", mep->name);
            return 0;
        }
        writel(QMU_Q_START, USB_QMU_TQCSR(epnum));

    } else {
        writel(ring->dma, USB_QMU_RQSAR(mep->num));
        setbits32_r(RX_DMAREQEN, MU3D_EP_RXCR0(mep->num));
        setbits32_r(QMU_RX_CS_EN(epnum), U3D_QCR0);
        /* don't expect ZLP */
        clrbits32_r(QMU_RX_ZLP(epnum), U3D_QCR3);
        /* move to next GPD when receive ZLP */
        setbits32_r(QMU_RX_COZ(epnum), U3D_QCR3);
        writel(QMU_RX_LEN_ERR(epnum) | QMU_RX_CS_ERR(epnum), U3D_RQERRIESR0);
        writel(QMU_RX_ZLP_ERR(epnum), U3D_RQERRIESR1);

        if (readl(USB_QMU_RQCSR(epnum)) & QMU_Q_ACTIVE) {
            DBG_C("%s Active Now!\n", mep->name);
            return 0;
        }
        writel(QMU_Q_START, USB_QMU_RQCSR(epnum));
    }
    DBG_I("%s's qmu start now!\n", mep->name);

    return 0;
}

/* may called in atomic context */
static void mtu3_qmu_stop(struct udc_endpoint *mep)
{
    int epnum = mep->num;
    paddr_t qcsr;
    int ret;

    qcsr = mep->in ? USB_QMU_TQCSR(epnum) : USB_QMU_RQCSR(epnum);

    if (!(readl(qcsr) & QMU_Q_ACTIVE)) {
        DBG_C("%s's qmu is inactive now!\n", mep->name);
        return;
    }
    writel(QMU_Q_STOP, qcsr);

    ret = wait_for_value(qcsr, QMU_Q_ACTIVE, 0, 10, 100);
    if (ret) {
        DBG_C("stop %s's qmu failed\n", mep->name);
        return;
    }

    DBG_I("%s's qmu stop now!\n", mep->name);
}

void mtu3_qmu_flush(struct udc_endpoint *mep)
{
    DBG_I("%s flush QMU %s\n", __func__, mep->name);

    /*Stop QMU */
    mtu3_qmu_stop(mep);
    reset_gpd_list(mep);
}

static void qmu_done_tx(u8 epnum)
{
    struct udc_endpoint *mep = mtu3_find_ep(epnum, USB_DIR_IN);
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->dequeue;
    struct qmu_gpd *gpd_current = NULL;
    struct udc_request *request = NULL;
    struct mu3d_req *mreq;
    paddr_t gpd_dma;

    gpd_dma = readl(USB_QMU_TQCPR(epnum));
    /*transfer phy address got from QMU register to virtual address */
    gpd_current = gpd_dma_to_virt(ring, gpd_dma);

    DBG_I("%s %s, last=%p, current=%p, enq=%p\n",
          __func__, mep->name, gpd, gpd_current, ring->enqueue);

    while (gpd != gpd_current && !(gpd->flag & GPD_FLAGS_HWO)) {

        request = mep->req;
        mreq = to_mu3d_req(request);
        if (mreq == NULL || mreq->gpd != gpd) {
            DBG_C("no correct TX req is found\n");
            break;
        }

        mreq->actual = gpd->buf_len;
        handle_ept_complete(mep, 0);
        gpd = advance_deq_gpd(ring);
    }

    DBG_I("%s EP%dIN, deq=%p, enq=%p, complete\n",
          __func__, epnum, ring->dequeue, ring->enqueue);
}

static void qmu_done_rx(u8 epnum)
{
    struct udc_endpoint *mep = mtu3_find_ep(epnum, USB_DIR_OUT);
    struct mtu3_gpd_ring *ring = &mep->gpd_ring;
    struct qmu_gpd *gpd = ring->dequeue;
    struct qmu_gpd *gpd_current = NULL;
    struct udc_request *request = NULL;
    struct mu3d_req *mreq;
    paddr_t gpd_dma;

    gpd_dma = readl(USB_QMU_RQCPR(epnum));
    gpd_current = gpd_dma_to_virt(ring, gpd_dma);

    DBG_I("%s %s, last=%p, current=%p, enq=%p\n",
          __func__, mep->name, gpd, gpd_current, ring->enqueue);

    while (gpd != gpd_current && !(gpd->flag & GPD_FLAGS_HWO)) {

        request = mep->req;
        mreq = to_mu3d_req(request);
        if (mreq == NULL || mreq->gpd != gpd) {
            DBG_C("no correct RX req is found\n");
            break;
        }
        mreq->actual = gpd->buf_len;
        handle_ept_complete(mep, 0);
        gpd = advance_deq_gpd(ring);
    }

    DBG_I("%s EP%dOUT, deq=%p, enq=%p, complete\n",
          __func__, epnum, ring->dequeue, ring->enqueue);
}

static void qmu_done_isr(u32 done_status)
{
    int i;

    for (i = 1; i <= (MT_EP_NUM / 2); i++) {
        if (done_status & QMU_RX_DONE_INT(i))
            qmu_done_rx(i);
        if (done_status & QMU_TX_DONE_INT(i))
            qmu_done_tx(i);
    }
}

static void qmu_exception_isr(u32 qmu_status)
{
    u32 errval;
    int i;

    if ((qmu_status & RXQ_CSERR_INT) || (qmu_status & RXQ_LENERR_INT)) {
        errval = readl(U3D_RQERRIR0);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_RX_CS_ERR(i))
                DBG_C("Rx EP%d CS error!\n", i);

            if (errval & QMU_RX_LEN_ERR(i))
                DBG_C("RX EP%d Length error\n", i);
        }
        writel(errval, U3D_RQERRIR0);
    }

    if (qmu_status & RXQ_ZLPERR_INT) {
        errval = readl(U3D_RQERRIR1);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_RX_ZLP_ERR(i))
                DBG_I("RX EP%d Recv ZLP\n", i);
        }
        writel(errval, U3D_RQERRIR1);
    }

    if ((qmu_status & TXQ_CSERR_INT) || (qmu_status & TXQ_LENERR_INT)) {
        errval = readl(U3D_TQERRIR0);
        for (i = 1; i <= (MT_EP_NUM / 2); i++) {
            if (errval & QMU_TX_CS_ERR(i))
                DBG_C("Tx EP%d checksum error!\n", i);

            if (errval & QMU_TX_LEN_ERR(i))
                DBG_I("Tx EP%d send ZLP failed\n", i);
        }
        writel(errval, U3D_TQERRIR0);
    }
}

enum handler_return mtu3_qmu_isr(void)
{
    u32 qmu_status;
    u32 qmu_done_status;

    /* U3D_QISAR1 is read update */
    qmu_status = readl(U3D_QISAR1);
    qmu_status &= readl(U3D_QIER1);

    qmu_done_status = readl(U3D_QISAR0);
    qmu_done_status &= readl(U3D_QIER0);
    writel(qmu_done_status, U3D_QISAR0); /* W1C */
    DBG_I("[INTR] QMUdone[TX=%x, RX=%x] QMUexp[%x]\n",
          (qmu_done_status & 0xFFFF), qmu_done_status >> 16,
          qmu_status);

    if (qmu_done_status)
        qmu_done_isr(qmu_done_status);

    if (qmu_status)
        qmu_exception_isr(qmu_status);

    return INT_RESCHEDULE;
}

int mtu3_qmu_init(void)
{
    if (QMU_GPD_SIZE != 16) {
        DBG_C("QMU_GPD size SHOULD be 16 Bytes");
        return -EFAULT;
    }
    return 0;
}

#else   /* PIO mode */

void mtu3_qmu_flush(struct udc_endpoint *mep)
{}

int mtu3_gpd_ring_alloc(struct udc_endpoint *mep)
{
    return 0;
}

void mtu3_gpd_ring_free(struct udc_endpoint *mep)
{}

enum handler_return mtu3_qmu_isr(void)
{
    return INT_NO_RESCHEDULE;
}

int mtu3_qmu_init(void)
{
    return 0;
}

#endif  /* SUPPORT_QMU */

#pragma GCC pop_options

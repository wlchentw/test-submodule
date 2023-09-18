/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Chunfeng Yun <chunfeng.yun@mediatek.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in
 *  the documentation and/or other materials provided with the
 *  distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <debug.h>
#include <platform/mt_reg_base.h>
#include <platform/reg_utils.h>

#include "usbphy.h"

#pragma GCC push_options
#pragma GCC optimize("O1")

#ifdef DBG_USB_PHY
#define PHY_LOG(x...) dprintf(INFO, "[USB][PHY] " x)
#else
#define PHY_LOG(x...) do{} while(0)
#endif

/* 2712E1 can't set RG_AVALID */
#define U3D_U2PHYDEV_MASK   (E60802_RG_IDDIG | /*E60802_RG_AVALID |*/ \
    E60802_RG_BVALID | E60802_RG_VBUSVALID)

#define U3D_U2PHYFRCDEV_MASK (E60802_FORCE_IDDIG | /*E60802_FORCE_AVALID |*/ \
    E60802_FORCE_BVALID | E60802_FORCE_SESSEND | E60802_FORCE_VBUSVALID)

void mt_usb_phy_poweron(void)
{
    PHY_LOG("%s\n", __func__);

    /* switch to USB function */
    clrbits32_r(E60802_FORCE_UART_EN, U3D_U2PHYDTM0);
    clrbits32_r(E60802_RG_UART_EN, U3D_U2PHYDTM1);
    clrbits32_r(E60802_RG_USB20_GPIO_CTL, U3D_U2PHYACR4);
    clrbits32_r(E60802_USB20_GPIO_MODE, U3D_U2PHYACR4);
    /* DP/DM BC1.1 path Disable */
    clrbits32_r(E60802_RG_USB20_BC11_SW_EN, U3D_USBPHYACR6);
    /* Internal R bias enable */
    setbits32_r(E60802_RG_USB20_INTR_EN, U3D_USBPHYACR0);
    /* 100U from u2 */
    clrbits32_r(E60802_RG_USB20_HS_100U_U3_EN, U3D_USBPHYACR5);
    /* let suspendm=1, enable usb 480MHz pll */
    setbits32_r(E60802_RG_SUSPENDM, U3D_U2PHYDTM0);
    /* force_suspendm=1 */
    setbits32_r(E60802_FORCE_SUSPENDM, U3D_U2PHYDTM0);
    /* wait 2 ms for USBPLL stable */
    spin(2000);
    /* power on device mode */
    clrbits32_r(E60802_RG_SESSEND, U3D_U2PHYDTM1);
    /* NOTE: mt2712E1 can't set RG_AVALID */
    setbits32_r(U3D_U2PHYDEV_MASK, U3D_U2PHYDTM1);
    /* enable force into device mode */
    setbits32_r(U3D_U2PHYFRCDEV_MASK, U3D_U2PHYDTM1);
    /* wait mac ready */
    spin(2000);
    /* apply MAC clock related setting after phy init */
}

void mt_usb_phy_poweroff(void)
{
    /* power down device mode */
    clrbits32_r(E60802_RG_VBUSVALID | E60802_RG_BVALID | E60802_RG_AVALID, U3D_U2PHYDTM1);
    setbits32_r(E60802_RG_IDDIG | E60802_RG_SESSEND, U3D_U2PHYDTM1);

    /* cleaer device force mode */
    clrbits32_r(U3D_U2PHYFRCDEV_MASK, U3D_U2PHYDTM1);

    clrbits32_r(E60802_RG_SUSPENDM, U3D_U2PHYDTM0);
    setbits32_r(E60802_FORCE_SUSPENDM, U3D_U2PHYDTM0);
    spin(2000);
    PHY_LOG("%s\n", __func__);
}

#pragma GCC pop_options

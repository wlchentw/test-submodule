/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifdef CONFIG_MTK_CLKMGR
#include <mach/mt_clkmgr.h>
#else
#include <linux/clk.h>
#endif
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include "mtk_musb.h"
#include "musb_core.h"
#include "usb20.h"

/*#include <mach/mt_gpio.h>*/

#define FRA (48)
#define PARA (28)

#ifdef FPGA_PLATFORM
bool usb_pre_clock(bool enable)
{
	return true;
}

bool usb_enable_clock(bool enable)
{
	return true;
}

void usb_phy_poweron(void)
{
}

void usb_phy_savecurrent(void)
{
}

void usb_phy_recover(void)
{
}

/* BC1.2*/
void Charger_Detect_Init(void)
{
}

void Charger_Detect_Release(void)
{
}

void usb_phy_context_save(void)
{
}

void usb_phy_context_restore(void)
{
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);

	usb_port_mode = USB_PHY_Read_Register8(0x6B);
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) || (usb_port_mode == 0x5E))
		return true;
	else
		return false;
}

void usb_phy_switch_to_usb(void)
{
	usb_enable_clock(true);
	udelay(50);
	/* clear force_uart_en */
	USBPHY_WRITE8(0x6B, 0x00);
	usb_enable_clock(false);
	usb_phy_poweron();
	/* disable the USB clock turned on in usb_phy_poweron() */
	usb_enable_clock(false);
}

void usb_phy_switch_to_uart(void)
{

	if (usb_phy_check_in_uart_mode())
		return;
	DBG(0, "Mask PMIC charger detection in UART mode.\n");
	/*ALPS00775710 */
	/*ALPS00775710 */

	usb_enable_clock(true);
	udelay(50);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* Set RG_SUSPENDM to 1 */
	USBPHY_SET8(0x68, 0x08);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	/* Set ru_uart_mode to 2'b01 */
	USBPHY_SET8(0x6B, 0x5C);

	/* Set RG_UART_EN to 1 */
	USBPHY_SET8(0x6E, 0x07);

	/* Set RG_USB20_DM_100K_EN to 1 */
	USBPHY_SET8(0x22, 0x02);
	usb_enable_clock(false);
}

#endif

#else

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool in_uart_mode;
#endif

static DEFINE_SPINLOCK(musb_reg_clock_lock);
/*#define CONFIG_DEFAULT_DEV_MODE*/
#ifdef CONFIG_DEFAULT_DEV_MODE
static int clkenablecnt;
#endif

bool usb_pre_clock(bool enable)
{
	if (enable) {
#ifndef CONFIG_MTK_CLKMGR
		clk_prepare(usbpll_clk);
		clk_prepare(usbmcu_clk);
		clk_prepare(usb_clk);
#endif
#ifdef CONFIG_DEFAULT_DEV_MODE
		if (clkenablecnt == 0)
			usb_enable_clock(enable);
#endif
	} else {

#ifdef CONFIG_DEFAULT_DEV_MODE
		if (clkenablecnt > 0)
			usb_enable_clock(enable);
#endif
#ifndef CONFIG_MTK_CLKMGR
		clk_unprepare(usb_clk);
		clk_unprepare(usbmcu_clk);
		clk_unprepare(usbpll_clk);
#endif
	}
	return true;
}

bool usb_enable_clock(bool enable)
{
	unsigned long flags;

	spin_lock_irqsave(&musb_reg_clock_lock, flags);
	if (enable) {
#ifdef CONFIG_DEFAULT_DEV_MODE
		clkenablecnt++;
#endif
#ifdef CONFIG_MTK_CLKMGR
		enable_clock(MT_CG_INFRA_USB, "INFRA_USB");
		enable_clock(MT_CG_INFRA_USB_MCU, "INFRA_USB_MCU");
		enable_clock(MT_CG_INFRA_ICUSB, "INFRA_ICUSB");
#else
		clk_enable(usbpll_clk);
		clk_enable(usb_clk);
		clk_enable(usbmcu_clk);
#endif
	} else {
#ifdef CONFIG_DEFAULT_DEV_MODE
		clkenablecnt--;
#endif
#ifdef CONFIG_MTK_CLKMGR
		disable_clock(MT_CG_INFRA_USB_MCU, "INFRA_USB_MCU");
		disable_clock(MT_CG_INFRA_USB, "INFRA_USB");
		disable_clock(MT_CG_INFRA_ICUSB, "INFRA_ICUSB");
#else
		clk_disable(usbpll_clk);
		clk_disable(usb_clk);
		clk_disable(usbmcu_clk);
#endif
	}
	spin_unlock_irqrestore(&musb_reg_clock_lock, flags);

#ifdef CONFIG_DEFAULT_DEV_MODE
	DBG(0, "clkenablecnt [0x%x]\n", clkenablecnt);
#endif

	return 1;
}

static void hs_slew_rate_cal(void)
{
	unsigned long data;
	unsigned long x;
	unsigned char value;
	unsigned long start_time, timeout;
	unsigned int timeout_flag = 0;
	/*4 s1:enable usb ring oscillator. */
	USBPHY_WRITE8(0x15, 0x80);

	/*4 s2:wait 1us. */
	udelay(1);

	/*4 s3:enable free run clock */
	USBPHY_WRITE8(0xf00 - 0x800 + 0x11, 0x01);
	/*4 s4:setting cyclecnt. */
	USBPHY_WRITE8(0xf00 - 0x800 + 0x01, 0x04);
	/*4 s5:enable frequency meter */
	USBPHY_SET8(0xf00 - 0x800 + 0x03, 0x01);

	/*4 s6:wait for frequency valid. */
	start_time = jiffies;
	timeout = jiffies + 3 * HZ;

	while (!(USBPHY_READ8(0xf00 - 0x800 + 0x10) & 0x1)) {
		if (time_after(jiffies, timeout)) {
			timeout_flag = 1;
			break;
		}
	}

	/*4 s7: read result. */
	if (timeout_flag) {
		DBG(0, "[USBPHY] Slew Rate Calibration: Timeout\n");
		value = 0x4;
	} else {
		data = USBPHY_READ32(0xf00 - 0x800 + 0x0c);
		x = ((1024 * FRA * PARA) / data);
		value = (unsigned char)(x / 1000);
		if ((x - value * 1000) / 100 >= 5)
			value += 1;
		DBG(0,
		    "[USBPHY]slew calibration: FM_OUT = %lu, x= %lu, value= %d\n",
		    data, x, value);
	}

	/*4 s8: disable Frequency and run clock. */
	/*disable frequency meter */
	USBPHY_CLR8(0xf00 - 0x800 + 0x03, 0x01);
	/*disable free run clock */
	USBPHY_CLR8(0xf00 - 0x800 + 0x11, 0x01);

	/*4 s9: */
	USBPHY_WRITE8(0x15, value << 4);

	/*4 s10:disable usb ring oscillator. */
	USBPHY_CLR8(0x15, 0x80);
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
bool usb_phy_check_in_uart_mode(void)
{
	UINT8 usb_port_mode;

	usb_enable_clock(true);
	udelay(50);
	usb_port_mode = USBPHY_READ8(0x6B);
	/*usb_port_mode = 1; */
	usb_enable_clock(false);

	if ((usb_port_mode == 0x5C) ||
	    (usb_port_mode == 0x5E) || (usb_port_mode_temp == 1)) {
		usb_port_mode_temp = 1;
		return true;
	} else
		return false;
}

void usb_phy_switch_to_uart(void)
{
	if (usb_phy_check_in_uart_mode())
		return;

	usb_enable_clock(true);
	udelay(50);

	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* Set RG_SUSPENDM to 1 */
	USBPHY_SET8(0x68, 0x08);

	/* force suspendm = 1 */
	USBPHY_SET8(0x6a, 0x04);

	/* Set ru_uart_mode to 2'b01 */
	USBPHY_SET8(0x6B, 0x5C);

	/* Set RG_UART_EN to 1 */
	USBPHY_SET8(0x6E, 0x07);

	/* Set RG_USB20_DM_100K_EN to 1 */
	USBPHY_SET8(0x22, 0x02);
	usb_enable_clock(false);

	/*set uart rx path */
	mtk_uart_usb_rx_sel(1, 1);
	usb_port_mode_temp = 1;
	DBG(0, "usb port value in uart function:%d\n", usb_port_mode_temp);
	/* GPIO Selection */
	/* DRV_WriteReg32(GPIO_BASE + 0x504, 0x10);     //set */
}


void usb_phy_switch_to_usb(void)
{
	/* GPIO Selection */
	/*DRV_WriteReg32(GPIO_BASE + 0x508, 0x10);              //set */
	mtk_uart_usb_rx_sel(1, 0);
	usb_enable_clock(true);
	udelay(50);
	/* clear force_uart_en */
	USBPHY_WRITE8(0x6B, 0x00);
	usb_enable_clock(false);
	usb_phy_poweron();
	/* disable the USB clock turned on in usb_phy_poweron() */
	usb_enable_clock(false);

	usb_port_mode_temp = 0;
	DBG(0, "usb port value in usb function:%d\n", usb_port_mode_temp);
}
#endif

void usb_phy_poweron(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (usb_phy_check_in_uart_mode())
		return;
#endif

	/* enable USB MAC clock. */
	usb_enable_clock(true);

	/* wait 50 usec for PHY3.3v/1.8v stable. */
	udelay(50);

	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	USBPHY_CLR8(0x21, 0x03);
	/*USBPHY_CLR8(0x21, 0x01); */

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);

	/* rg_usb20_dp_100k_mode, 1'b1 */
	USBPHY_SET8(0x22, 0x04);
	USBPHY_CLR8(0x22, 0x03);

	/*OTG enable */
	USBPHY_SET8(0x20, 0x10);
	/* force_suspendm, 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);

	/*6-1. PASS RX sensitivity HQA requirement */
	USBPHY_SET8(0x18, 0x06);

	/* 7 s7: wait for 800 usec. */
	udelay(800);

	/* force enter device mode, from K2, FIXME */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2E);
	USBPHY_SET8(0x6d, 0x3E);

	DBG(0, "usb power on success\n");
}

#ifdef CONFIG_MTK_UART_USB_SWITCH
static bool skipDisableUartMode;
#endif

static void usb_phy_savecurrent_internal(void)
{

	/* 4 1. swtich to USB function.
	 *(system register, force ip into usb mode.
	 */

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* enable USB MAC clock. */
		usb_enable_clock(true);

		/* wait 50 usec for PHY3.3v/1.8v stable. */
		udelay(50);

		/* force_uart_en, 1'b0 */
		USBPHY_CLR8(0x6b, 0x04);
		/* RG_UART_EN, 1'b0 */
		USBPHY_CLR8(0x6e, 0x01);
		/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
		USBPHY_CLR8(0x21, 0x03);

		/*4 2. release force suspendm. */
		/*USBPHY_CLR8(0x6a, 0x04); */
		USBPHY_SET8(0x6a, 0x04);
		/* RG_SUSPENDM, 1'b1 */
		USBPHY_SET8(0x68, 0x08);
		usb_enable_clock(false);
	} else {
		if (skipDisableUartMode)
			skipDisableUartMode = false;
		else
			return;
	}
#else
	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x01);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	USBPHY_CLR8(0x21, 0x03);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	/* USBPHY_CLR8(0x6a, 0x04); */
	USBPHY_SET8(0x6a, 0x04);
	USBPHY_SET8(0x68, 0x08);
#endif

	/* RG_DPPULLDOWN, 1'b1, RG_DMPULLDOWN, 1'b1 */
	USBPHY_SET8(0x68, 0xc0);
	/* RG_XCVRSEL[1:0], 2'b01. */
	USBPHY_CLR8(0x68, 0x30);
	USBPHY_SET8(0x68, 0x10);
	/* RG_TERMSEL, 1'b1 */
	USBPHY_SET8(0x68, 0x04);
	/* RG_DATAIN[3:0], 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/* force_dp_pulldown, 1'b1, force_dm_pulldown, 1'b1,
	 * force_xcversel, 1'b1, force_termsel, 1'b1, force_datain, 1'b1
	 */
	USBPHY_SET8(0x6a, 0xba);

	/*4 8.RG_USB20_BC11_SW_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/*4 9.RG_USB20_OTG_VBUSSCMP_EN 1'b0 */
	USBPHY_CLR8(0x1a, 0x10);
	/*4 10. delay 800us. */
	udelay(800);
	/*4 11. rg_usb20_pll_stable = 1 */
	USBPHY_CLR8(0x68, 0x08);
	/*
	 *  USBPHY_SET8(0x63, 0x02);
	 *
	 * ALPS00427972, implement the analog register formula
	 * ALPS00427972, implement the analog register formula
	 */

	udelay(1);
	/*4 12.  force suspendm = 1. */
	/* USBPHY_SET8(0x6a, 0x04); */
	/*4 13.  wait 1us */
	udelay(1);

	/* force enter device mode, from K2, FIXME */
	/* force enter device mode */
	/*USBPHY_CLR8(0x6c, 0x10); */
	/*USBPHY_SET8(0x6c, 0x2E); */
	/*USBPHY_SET8(0x6d, 0x3E); */
}

void usb_phy_savecurrent(void)
{
	usb_phy_savecurrent_internal();
	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);
	DBG(0, "usb save current success\n");
}

void usb_phy_recover(void)
{

	/* turn on USB reference clock. */
	usb_enable_clock(true);
	/* wait 50 usec. */
	udelay(50);

#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (!usb_phy_check_in_uart_mode()) {
		/* clean PUPD_BIST_EN */
		/* PUPD_BIST_EN = 1'b0 */
		/* PMIC will use it to detect charger type */
		USBPHY_CLR8(0x1d, 0x10);

		/* force_uart_en, 1'b0 */
		USBPHY_CLR8(0x6b, 0x04);
		/* RG_UART_EN, 1'b0 */
		USBPHY_CLR8(0x6e, 0x1);
		/* force_suspendm, 1'b0 */
		USBPHY_CLR8(0x6a, 0x04);
		USBPHY_CLR8(0x22, 0x02);

		skipDisableUartMode = false;
	} else {
		/*if (!skipDisableUartMode) */
		return;
	}
#else
	/* clean PUPD_BIST_EN */
	/* PUPD_BIST_EN = 1'b0 */
	/* PMIC will use it to detect charger type */
	USBPHY_CLR8(0x1d, 0x10);

	/* force_uart_en, 1'b0 */
	USBPHY_CLR8(0x6b, 0x04);
	/* RG_UART_EN, 1'b0 */
	USBPHY_CLR8(0x6e, 0x1);
	/* rg_usb20_gpio_ctl, 1'b0, usb20_gpio_mode, 1'b0 */
	/* force_suspendm, 1'b0 */
	USBPHY_CLR8(0x6a, 0x04);

	USBPHY_CLR8(0x21, 0x03);
#endif

	/* enable VRT internal R architecture */
	/* RG_USB20_INTR_EN = 1'b1 */
	USBPHY_SET8(0x00, 0x20);

	/* RG_DPPULLDOWN, 1'b0, RG_DMPULLDOWN, 1'b0 */
	USBPHY_CLR8(0x68, 0x40);
	/* 4 7. RG_DMPULLDOWN = 1'b0 */
	USBPHY_CLR8(0x68, 0x80);
	/* RG_XCVRSEL[1:0], 2'b00. */
	USBPHY_CLR8(0x68, 0x30);
	/* RG_TERMSEL, 1'b0 */
	USBPHY_CLR8(0x68, 0x04);
	/* RG_DATAIN[3:0], 4'b0000 */
	USBPHY_CLR8(0x69, 0x3c);

	/* force_dp_pulldown, 1'b0, force_dm_pulldown, 1'b0 */
	USBPHY_CLR8(0x6a, 0x10);
	/* 4 12. force_dm_pulldown = 1b'0 */
	USBPHY_CLR8(0x6a, 0x20);
	/* 4 13. force_xcversel = 1b'0 */
	USBPHY_CLR8(0x6a, 0x08);
	/* 4 14. force_termsel = 1b'0 */
	USBPHY_CLR8(0x6a, 0x02);
	/* 4 15. force_datain = 1b'0 */
	USBPHY_CLR8(0x6a, 0x80);

	/* RG_USB20_BC11_SW_EN, 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	/* RG_USB20_OTG_VBUSCMP_EN, 1'b1 */
	USBPHY_SET8(0x1a, 0x10);
	/*18. PASS RX sensitivity HQA requirement */
	USBPHY_CLR8(0x18, 0x08);
	USBPHY_SET8(0x18, 0x06);

	/* wait 800 usec. */
	udelay(800);

	/* force enter device mode, from K2, FIXME */
	USBPHY_CLR8(0x6c, 0x10);
	USBPHY_SET8(0x6c, 0x2E);
	USBPHY_SET8(0x6d, 0x3E);

	/* from K2, FIXME */
#if defined(MTK_HDMI_SUPPORT)
	USBPHY_SET8(0x05, 0x05);
	USBPHY_SET8(0x05, 0x50);
#endif
	hs_slew_rate_cal();

	DBG(0, "usb recovery success\n");
}

/* BC1.2 */
void Charger_Detect_Init(void)
{
	/* turn on USB reference clock. */
	usb_enable_clock(true);
	/* wait 50 usec. */
	udelay(50);
	/* RG_USB20_BC11_SW_EN = 1'b1 */
	USBPHY_SET8(0x1a, 0x80);
	DBG(0, "Charger_Detect_Init\n");
}

void Charger_Detect_Release(void)
{
	/* RG_USB20_BC11_SW_EN = 1'b0 */
	USBPHY_CLR8(0x1a, 0x80);
	udelay(1);
	/* 4 14. turn off internal 48Mhz PLL. */
	usb_enable_clock(false);
	DBG(0, "Charger_Detect_Release\n");
}

void usb_phy_context_save(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	in_uart_mode = usb_phy_check_in_uart_mode();
#endif
}

void usb_phy_context_restore(void)
{
#ifdef CONFIG_MTK_UART_USB_SWITCH
	if (in_uart_mode)
		usb_phy_switch_to_uart();
#endif
	usb_phy_savecurrent_internal();
}

#ifdef CONFIG_USB_PHYCHK_EXTCONN
#define USBPHYACR6				0x018
#define RG_USB20_BC11_SW_EN			(1ul << 23)

#define U2PHYACR4				0x020
#define RG_USB20_DM_100K_EN			(1ul << 17)

#define U2PHYDTM0				0x68
#define FORCE_DM_PULLDOWN			(1ul << 21)
#define FORCE_DP_PULLDOWN			(1ul << 20)

#define RG_DMPULLDOWN				(1ul <<  7)
#define RG_DPPULLDOWN				(1ul <<  6)

#define U2PHYDMON1				0x74
#define B_USB20_LINE_STATE			22
#define USB20_LINE_STATE_MASK			(3ul << 22)

#define MOD "[EXTCON]"
#if 0
#define LS_PRINT(fmt, ...)			pr_info(fmt, ##__VA_ARGS__)
#else
#define LS_PRINT(fmt, ...)
#endif

int mt_usb_phychk_extconn(void)
{
	u32 val = 0;
	u32 line_state = 0;
	int usb_extconn = CHARGER_UNKNOWN;

	static const char * const string_usb_extconn_type[] = {
		 "UNKNOWN LINE TYPE",
		 "STANDARD_HOST",
		 "CHARGING_HOST",
		 "NONSTANDARD_CHARGER",
		 "STANDARD_CHARGER",
		 "INVALID PARAMETER",
	 };

#ifdef CONFIG_MTK_MUSB_PORT0_LOWPOWER_MODE
	mt_usb_clock_prepare();
#endif

	/* enable USB MAC clock. */
	usb_enable_clock(true);

	/* set PHY 0x18[23] = 1'b0 */
	val  = USBPHY_READ32(USBPHYACR6);
	val &= ~(RG_USB20_BC11_SW_EN);
	USBPHY_WRITE32(USBPHYACR6, val);
	LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + USBPHYACR6,
		 val, USBPHY_READ32(USBPHYACR6));

	/* Device side does NOT apply 15K pull-down on DP, and apply 100K
	 * pull up on DM.
	 * 1. For USB Host, 15K pull-down will cause linestate as 2'b00.
	 * 2. For Charger since no pull-down exist on D+/-,
	 * the linesate will be 2'b1x.
	 *
	 * stage 1
	 * set PHY 0x68[21:20] = 2'b11
	 * set PHY 0x68[ 7: 6] = 2'b00
	 * set PHY 0x20[   17] = 1'b1
	 */

	val  =   USBPHY_READ32(U2PHYDTM0);
	val |=  (FORCE_DP_PULLDOWN | FORCE_DM_PULLDOWN);
	val &= ~(RG_DPPULLDOWN | RG_DMPULLDOWN);
	USBPHY_WRITE32(U2PHYDTM0, val);
	LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + U2PHYDTM0,
		 val, USBPHY_READ32(U2PHYDTM0));


	val  = USBPHY_READ32(U2PHYACR4);
	val |= RG_USB20_DM_100K_EN;
	USBPHY_WRITE32(U2PHYACR4, val);
	LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + U2PHYACR4,
		 val, USBPHY_READ32(U2PHYACR4));

	mdelay(10);

	/* Read linestate
	 * Read PHY 0x74[23:22] 2'bxx
	 */
	line_state   = USBPHY_READ32(U2PHYDMON1);
	LS_PRINT("usbphy addr 0x%p = 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + U2PHYDMON1,
		 USBPHY_READ32(U2PHYDMON1));
	line_state  &= USB20_LINE_STATE_MASK;
	line_state >>= B_USB20_LINE_STATE;

	if ((line_state & 0x02) == 0)
		usb_extconn = STANDARD_HOST;
	else if ((line_state & 0x02) != 0) {
		/* Device side does apply 15K pul-down on DP, and apply 100K
		 * pull up on DM.
		 * 1. For standard charger, D+/- are shorted, so the D+ pulling
		 *    down will drive both D+/- to low.
		 *    Therefore linestate as 2'b00.
		 * 2. For non-standard charger, since no pull-down exist on D-,
		 *    the linesate will be 2'b1x.
		 *
		 * stage 2
		 * set PHY 0x68[21:20] = 2'b11
		 * set PHY 0x68[ 7: 6] = 2'b01
		 * set PHY 0x20[   17] = 1'b1
		 */

		val  =   USBPHY_READ32(U2PHYDTM0);
		val |=  (FORCE_DP_PULLDOWN | FORCE_DM_PULLDOWN);
		val &= ~(RG_DPPULLDOWN | RG_DMPULLDOWN);
		val |=   RG_DPPULLDOWN;
		USBPHY_WRITE32(U2PHYDTM0, val);
		LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
			 mtk_musb->xceiv->io_priv + 0x800 + U2PHYDTM0,
			 val, USBPHY_READ32(U2PHYDTM0));

		val  = USBPHY_READ32(U2PHYACR4);
		val |= RG_USB20_DM_100K_EN;
		USBPHY_WRITE32(U2PHYACR4, val);
		LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
			 mtk_musb->xceiv->io_priv + 0x800 + U2PHYACR4,
			 val, USBPHY_READ32(U2PHYACR4));

		mdelay(10);

		/* Read linestate
		 * Read PHY 0x74[23:22] 2'bxx
		 */
		line_state   = USBPHY_READ32(U2PHYDMON1);
		LS_PRINT("usbphy addr 0x%p = 0x%x\n",
			 mtk_musb->xceiv->io_priv + 0x800 + U2PHYDMON1,
			 USBPHY_READ32(U2PHYDMON1));
		line_state  &= USB20_LINE_STATE_MASK;
		line_state >>= B_USB20_LINE_STATE;

		switch (line_state) {
		case 0x00:
			usb_extconn = STANDARD_CHARGER;
			break;
		case 0x02:
		case 0x03:
			usb_extconn = NONSTANDARD_CHARGER;
			break;
		default:
			usb_extconn = CHARGER_UNKNOWN;
			break;
		}
	}

	val  = USBPHY_READ32(U2PHYDTM0);
	val &=  ~(FORCE_DP_PULLDOWN | FORCE_DM_PULLDOWN);
	USBPHY_WRITE32(U2PHYDTM0, val);
	LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + U2PHYDTM0,
		 val, USBPHY_READ32(U2PHYDTM0));

	val  = USBPHY_READ32(U2PHYACR4);
	val &= ~RG_USB20_DM_100K_EN;
	USBPHY_WRITE32(U2PHYACR4, val);
	LS_PRINT("usbphy addr 0x%p = 0x%x but 0x%x\n",
		 mtk_musb->xceiv->io_priv + 0x800 + U2PHYACR4,
		 val, USBPHY_READ32(U2PHYACR4));

	usb_extconn = (usb_extconn > STANDARD_CHARGER)
			? CHARGER_UNKNOWN : usb_extconn;

	usb_enable_clock(false);
#ifdef CONFIG_MTK_MUSB_PORT0_LOWPOWER_MODE
	mt_usb_clock_unprepare();
#endif

	pr_info("\n%s Final USB ext connector type: %s, line state: 0x%x\n",
		MOD, string_usb_extconn_type[usb_extconn], usb_extconn);

	return usb_extconn;
}
#endif

#endif

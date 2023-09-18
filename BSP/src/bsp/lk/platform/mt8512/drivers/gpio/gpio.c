// SPDX-License-Identifier: GPL-2.0
/*
 * MediaTek clock driver for MT8110 SoC
 *
 * Copyright (C) 2019 BayLibre, SAS
 * Author: Jiaguang Zhang <jiaguang.zhang@mediatek.com>
 */

#include <debug.h>
#include <platform/mt_reg_base.h>
#include <reg.h>
#include <string.h>

#include "gpio_cfg.h"
#include "gpio.h"
#define GPIO_BRINGUP

#define DRV_WriteReg32(addr, value)     (*(volatile unsigned int *)(addr) = (value))
#define DRV_Reg32(addr)                 (*(volatile unsigned int *)(addr))


#define GPIO_WR32(addr, data)   DRV_WriteReg32(addr, data)
#define GPIO_RD32(addr)         DRV_Reg32(addr)
#define GPIO_SW_SET_BITS(BIT,REG)   GPIO_WR32(REG,GPIO_RD32(REG) | ((unsigned long)(BIT)))
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT)))
/*---------------------------------------------------------------------------*/
#define TRUE                   1
#define FALSE                  0

#define GPIOTAG                "[GPIO] "
#define GIO_INVALID_OBJ(ptr)   ((ptr) != gpio_obj)


//#define GPIO_DEBUG	1

int mt_set_gpio_drv_chip(u32 pin, u32 drv)
{
	u32 reg;
	u32 val, mask;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (DRV_offset[pin].offset == (s8)-1)
		return GPIO_DRV_UNSUPPORTED;

	mask = ((1 << DRV_width[pin].width) - 1);
	/* check if set value beyond supported width */
	if (drv > mask)
		return -ERINVAL;

	reg = GPIO_RD32(DRV_addr[pin].addr);
	val = reg & ~(mask << DRV_offset[pin].offset);
	val |= (drv << DRV_offset[pin].offset);

	GPIO_WR32(DRV_addr[pin].addr, val);


	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/

int mt_get_gpio_drv_chip(u32 pin)
{
	u32 reg;
	u32 val, mask;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (DRV_offset[pin].offset == (s8)-1)
		return GPIO_DRV_UNSUPPORTED;

	reg = GPIO_RD32(DRV_addr[pin].addr);
	reg = reg >> DRV_offset[pin].offset;
	mask = (1 << DRV_width[pin].width) - 1;
	val = reg & mask;

	return val;
}

static int mt_set_gpio_dir_chip(u32 pin, u32 dir)
{
	u32 bit;
	u32 reg;
	u32 pos;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (dir >= GPIO_DIR_MAX)
		return -ERINVAL;

	bit = DIR_offset[pin].offset;

	reg = GPIO_RD32(DIR_addr[pin].addr);
	if (dir == GPIO_DIR_IN)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);

	GPIO_WR32(DIR_addr[pin].addr, reg);
#ifdef GPIO_DEBUG //[
	dprintf(ALWAYS, "%s(pin=%d) , addr=0x%x,reg=0x%x\n",
			__func__,pin,DIR_addr[pin].addr,reg);
#endif //] GPIO_DEBUG
	return RSUCCESS;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int mt_get_gpio_dir_chip(unsigned long pin)
{
    unsigned long pos;
    unsigned long bit;
    unsigned long reg;

    if (pin >= MAX_GPIO_PIN)
        return -ERINVAL;

    pos = pin / MAX_GPIO_REG_BITS;
    bit = pin % MAX_GPIO_REG_BITS;

    reg = GPIO_RD32(DIR_addr[pin].addr);
    return (((reg & (1L << bit)) != 0)? 1: 0);
}

/*---------------------------------------------------------------------------*/
static int mt_set_gpio_out_chip(u32 pin, u32 output)
{
	u32 bit;
	u32 reg = 0;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (output >= GPIO_OUT_MAX)
		return -ERINVAL;

	bit = DATAOUT_offset[pin].offset;

	reg = GPIO_RD32(DATAOUT_addr[pin].addr);
	if (output == GPIO_OUT_ZERO)
		reg &= (~(1 << bit));
	else
		reg |= (1 << bit);
	GPIO_WR32(DATAOUT_addr[pin].addr, reg);

#ifdef GPIO_DEBUG //[
	dprintf(ALWAYS, "%s(pin=%d) , addr=0x%x,reg=0x%x\n",
			__func__,pin,DATAOUT_addr[pin].addr,reg);
#endif //] GPIO_DEBUG
	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
static int mt_get_gpio_out_chip(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = GPIO_RD32(DATAOUT_addr[pin].addr);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}

/*---------------------------------------------------------------------------*/
static int mt_get_gpio_in_chip(u32 pin)
{
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = DATAIN_offset[pin].offset;
	reg = GPIO_RD32(DATAIN_addr[pin].addr);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
static int mt_set_gpio_mode_chip(u32 pin, u32 mode)
{
	u32 bit;
	u32 reg;


	u32 mask = (1L << GPIO_MODE_BITS) - 1;


	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (mode >= GPIO_MODE_MAX)
		return -ERINVAL;

	bit = MODE_offset[pin].offset;

	mode = mode & 0x7;
	reg = GPIO_RD32(MODE_addr[pin].addr);
	reg &= (~(mask << bit));
	reg |= (mode << bit);
	GPIO_WR32(MODE_addr[pin].addr, reg);

#ifdef GPIO_DEBUG //[
	dprintf(ALWAYS, "%s(pin=%d) , addr=0x%x,reg=0x%x\n",
			__func__,pin,MODE_addr[pin].addr,reg);
#endif //] GPIO_DEBUG


	return RSUCCESS;
}

/*---------------------------------------------------------------------------*/
static int mt_get_gpio_mode_chip(u32 pin)
{
	u32 bit;
	u32 reg;
	u32 mask = (1L << GPIO_MODE_BITS) - 1;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	bit = MODE_offset[pin].offset;
	reg = GPIO_RD32(MODE_addr[pin].addr);
	return ((reg >> bit) & mask);
}

/*---------------------------------------------------------------------------*/
int mt_set_gpio_drv(u32 pin, u32 drv)
{
	return mt_set_gpio_drv_chip(pin,drv);
}
/*---------------------------------------------------------------------------*/
int mt_get_gpio_drv(u32 pin)
{
	return mt_get_gpio_drv_chip(pin);
}

int mt_set_gpio_dir(unsigned long pin, unsigned long dir)
{
    return mt_set_gpio_dir_chip(pin,dir);
}

int mt_get_gpio_dir(unsigned long pin)
{
    return mt_get_gpio_dir_chip(pin);
}

int mt_set_gpio_out(unsigned long pin, unsigned long output)
{
    return mt_set_gpio_out_chip(pin,output);
}

int mt_get_gpio_out(unsigned long pin)
{
    return mt_get_gpio_out_chip(pin);
}

int mt_get_gpio_in(unsigned long pin)
{
    return mt_get_gpio_in_chip(pin);
}

int mt_set_gpio_mode(unsigned long pin, unsigned long mode)
{
    return mt_set_gpio_mode_chip(pin,mode);
}

int mt_get_gpio_mode(unsigned long pin)
{
    return mt_get_gpio_mode_chip(pin);
}

int mt_set_gpio_pull_en(unsigned long pin, unsigned long enable)
{
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (enable >= GPIO_PUEN_MAX)
		return -ERINVAL;		

	bit = PULLEN_offset[pin].offset;

	reg = GPIO_RD32(PULLEN_addr[pin].addr);
	if (enable == GPIO_PUEN_DISABLE) {
		reg &= (~(1 << bit));
	}
	else	
		reg |= (1 << bit);

	GPIO_WR32(PULLEN_addr[pin].addr, reg);

	return RSUCCESS;	
}

int mt_get_gpio_pull_en(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = GPIO_RD32(PULLEN_addr[pin].addr);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}

int mt_set_gpio_pull_sel(unsigned long pin, unsigned long select)
{
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	if (select >= GPIO_PUSEL_MAX)
		return -ERINVAL;		

	bit = PULLSEL_offset[pin].offset;

	reg = GPIO_RD32(PULLSEL_addr[pin].addr);
	if (select == GPIO_PUSEL_DOWN)
		reg &= (~(1 << bit));
	else	
		reg |= (1 << bit);

	GPIO_WR32(PULLSEL_addr[pin].addr, reg);

	return RSUCCESS;	
}

int mt_get_gpio_pull_sel(u32 pin)
{
	u32 pos;
	u32 bit;
	u32 reg;

	if (pin >= MAX_GPIO_PIN)
		return -ERINVAL;

	pos = pin / MAX_GPIO_REG_BITS;
	bit = pin % MAX_GPIO_REG_BITS;

	reg = GPIO_RD32(PULLSEL_addr[pin].addr);
	return (((reg & (1L << bit)) != 0) ? 1 : 0);
}

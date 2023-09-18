/*
 * MD1122 usb2.0 phy board for FPGA
 *
 * Copyright 2016 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */
#include <debug.h>

#include "mtu3_hw_regs.h"
#include "u3phy-i2c.h"

#define MD1122_I2C_ADDR 0x60
#define PHY_VERSION_BANK 0x20
#define PHY_VERSION_ADDR 0xe4

static void *g_ippc_port_addr;

int USB_PHY_Write_Register8(unsigned char data, unsigned char addr)
{
    u3phy_write_reg(g_ippc_port_addr, MD1122_I2C_ADDR, addr, data);

    return 0;
}

unsigned char USB_PHY_Read_Register8(unsigned char addr)
{
    unsigned char data;

    data = u3phy_read_reg(g_ippc_port_addr, MD1122_I2C_ADDR, addr);

    return data;
}

unsigned int get_phy_verison(void)
{
    unsigned int version = 0;

    u3phy_write_reg8(g_ippc_port_addr, MD1122_I2C_ADDR, 0xff, PHY_VERSION_BANK);

    version = u3phy_read_reg32(g_ippc_port_addr, MD1122_I2C_ADDR, PHY_VERSION_ADDR);
    dprintf(ALWAYS, "ssusb phy version: %x %p\n", version, g_ippc_port_addr);

    return version;
}


int md1122_u3phy_init(void *i2c_port_base)
{
    g_ippc_port_addr = i2c_port_base;

    if (get_phy_verison() != 0xa60810a) {
        dprintf(ALWAYS,"get phy version failed\n");
        return -1;
    }

    /* usb phy initial sequence */
    USB_PHY_Write_Register8(0x00, 0xFF);
    dprintf(INFO,"****************before bank 0x00*************************\n");
    dprintf(INFO,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x05, value: %x\n", USB_PHY_Read_Register8(0x05));
    dprintf(INFO,"[U2P]addr: 0x18, value: %x\n", USB_PHY_Read_Register8(0x18));
    dprintf(INFO,"*****************after **********************************\n");
    USB_PHY_Write_Register8(0x55, 0x05);
    USB_PHY_Write_Register8(0x84, 0x18);


    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x05, value: %x\n", USB_PHY_Read_Register8(0x05));
    dprintf(INFO,"[U2P]addr: 0x18, value: %x\n", USB_PHY_Read_Register8(0x18));
    dprintf(INFO,"****************before bank 0x10*************************\n");
    USB_PHY_Write_Register8(0x10, 0xFF);
    dprintf(INFO,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x0A, value: %x\n", USB_PHY_Read_Register8(0x0A));
    dprintf(INFO,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x84, 0x0A);

    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x0A, value: %x\n", USB_PHY_Read_Register8(0x0A));
    dprintf(INFO,"****************before bank 0x40*************************\n");
    USB_PHY_Write_Register8(0x40, 0xFF);
    dprintf(INFO,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x38, value: %x\n", USB_PHY_Read_Register8(0x38));
    dprintf(INFO,"[U2P]addr: 0x42, value: %x\n", USB_PHY_Read_Register8(0x42));
    dprintf(INFO,"[U2P]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
    dprintf(INFO,"[U2P]addr: 0x09, value: %x\n", USB_PHY_Read_Register8(0x09));
    dprintf(INFO,"[U2P]addr: 0x0C, value: %x\n", USB_PHY_Read_Register8(0x0C));
    dprintf(INFO,"[U2P]addr: 0x0E, value: %x\n", USB_PHY_Read_Register8(0x0E));
    dprintf(INFO,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x10));
    dprintf(INFO,"[U2P]addr: 0x14, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(INFO,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x46, 0x38);
    USB_PHY_Write_Register8(0x40, 0x42);
    USB_PHY_Write_Register8(0xAB, 0x08);
    USB_PHY_Write_Register8(0x0C, 0x09);
    USB_PHY_Write_Register8(0x71, 0x0C);
    USB_PHY_Write_Register8(0x4F, 0x0E);
    USB_PHY_Write_Register8(0xE1, 0x10);
    USB_PHY_Write_Register8(0x5F, 0x14);
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x38, value: %x\n", USB_PHY_Read_Register8(0x38));
    dprintf(INFO,"[U2P]addr: 0x42, value: %x\n", USB_PHY_Read_Register8(0x42));
    dprintf(INFO,"[U2P]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
    dprintf(INFO,"[U2P]addr: 0x09, value: %x\n", USB_PHY_Read_Register8(0x09));
    dprintf(INFO,"[U2P]addr: 0x0C, value: %x\n", USB_PHY_Read_Register8(0x0C));
    dprintf(INFO,"[U2P]addr: 0x0E, value: %x\n", USB_PHY_Read_Register8(0x0E));
    dprintf(INFO,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x10));
    dprintf(INFO,"[U2P]addr: 0x14, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(INFO,"****************before bank 0x60*************************\n");
    USB_PHY_Write_Register8(0x60, 0xFF);
    dprintf(INFO,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(INFO,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x03, 0x14);
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(INFO,"****************before bank 0x00*************************\n");
    USB_PHY_Write_Register8(0x00, 0xFF);
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x6A, value: %x\n", USB_PHY_Read_Register8(0x6A));
    dprintf(INFO,"[U2P]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
    dprintf(INFO,"[U2P]addr: 0x6C, value: %x\n", USB_PHY_Read_Register8(0x6C));
    dprintf(INFO,"[U2P]addr: 0x6D, value: %x\n", USB_PHY_Read_Register8(0x6D));
    USB_PHY_Write_Register8(0x04, 0x6A);
    USB_PHY_Write_Register8(0x08, 0x68);
    USB_PHY_Write_Register8(0x26, 0x6C);
    USB_PHY_Write_Register8(0x36, 0x6D);
    dprintf(INFO,"*****************after **********************************\n");
    dprintf(INFO,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(INFO,"[U2P]addr: 0x6A, value: %x\n", USB_PHY_Read_Register8(0x6A));
    dprintf(INFO,"[U2P]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
    dprintf(INFO,"[U2P]addr: 0x6C, value: %x\n", USB_PHY_Read_Register8(0x6C));
    dprintf(INFO,"[U2P]addr: 0x6D, value: %x\n", USB_PHY_Read_Register8(0x6D));

    dprintf(INFO,"[U2P]%s, end\n", __func__);
    return 0;
}

void mt_usb_phy_poweron(void)
{
    md1122_u3phy_init((void *)U3D_SSUSB_FPGA_I2C_OUT_0P);
}

void mt_usb_phy_poweroff(void)
{
    return;
}

/*
 * MD1122 usb2.0 phy board for FPGA
 *
 * Copyright 2016 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */
#include <debug.h>
#include <platform/mt_i2c.h>

#define MD1122_I2C_ADDR 0x60
#define PHY_VERSION_BANK 0x20
#define PHY_VERSION_ADDR 0xe4

int USB_PHY_Write_Register8(unsigned char data, unsigned char addr)
{
    unsigned char buffer[2];

    buffer[0] = addr;
    buffer[1] = data;
    mtk_i2c_write(0, MD1122_I2C_ADDR, 100, buffer, 2);

    return 0;
}

unsigned char USB_PHY_Read_Register8(unsigned char addr)
{
    unsigned char data;

    mtk_i2c_write_read(0, MD1122_I2C_ADDR, 100, &addr, &data, 1, 1);

    return data;
}

unsigned int USB_PHY_Read_Register32(unsigned char addr)
{
    unsigned char data[4];
    unsigned int ret;

    mtk_i2c_write_read(0, MD1122_I2C_ADDR, 100, &addr, data, 1, 4);
    ret = data[0];
    ret |= data[1] << 8;
    ret |= data[2] << 16;
    ret |= data[3] << 24;

    return ret;
}

#if 0
unsigned int get_phy_verison(void)
{
    unsigned int version = 0;

    USB_PHY_Write_Register8(0xff, PHY_VERSION_BANK);

    version = USB_PHY_Read_Register32(PHY_VERSION_ADDR);
    dprintf(ALWAYS, "ssusb phy version: %x\n", version);

    return version;
}
#endif

int md1122_u2phy_init(void)
{
#if 0
    if (get_phy_verison() != 0xa60810a) {
        dprintf(ALWAYS,"get phy version failed\n");
        //return -1;
    }
#endif

    /* usb phy initial sequence */
    USB_PHY_Write_Register8(0x00, 0xFF);
    dprintf(ALWAYS,"****************before bank 0x00*************************\n");
    dprintf(ALWAYS,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x05, value: %x\n", USB_PHY_Read_Register8(0x05));
    dprintf(ALWAYS,"[U2P]addr: 0x18, value: %x\n", USB_PHY_Read_Register8(0x18));
    dprintf(ALWAYS,"*****************after **********************************\n");
    USB_PHY_Write_Register8(0x55, 0x05);
    USB_PHY_Write_Register8(0x84, 0x18);


    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x05, value: %x\n", USB_PHY_Read_Register8(0x05));
    dprintf(ALWAYS,"[U2P]addr: 0x18, value: %x\n", USB_PHY_Read_Register8(0x18));
    dprintf(ALWAYS,"****************before bank 0x10*************************\n");
    USB_PHY_Write_Register8(0x10, 0xFF);
    dprintf(ALWAYS,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x0A, value: %x\n", USB_PHY_Read_Register8(0x0A));
    dprintf(ALWAYS,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x84, 0x0A);

    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x0A, value: %x\n", USB_PHY_Read_Register8(0x0A));
    dprintf(ALWAYS,"****************before bank 0x40*************************\n");
    USB_PHY_Write_Register8(0x40, 0xFF);
    dprintf(ALWAYS,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x38, value: %x\n", USB_PHY_Read_Register8(0x38));
    dprintf(ALWAYS,"[U2P]addr: 0x42, value: %x\n", USB_PHY_Read_Register8(0x42));
    dprintf(ALWAYS,"[U2P]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
    dprintf(ALWAYS,"[U2P]addr: 0x09, value: %x\n", USB_PHY_Read_Register8(0x09));
    dprintf(ALWAYS,"[U2P]addr: 0x0C, value: %x\n", USB_PHY_Read_Register8(0x0C));
    dprintf(ALWAYS,"[U2P]addr: 0x0E, value: %x\n", USB_PHY_Read_Register8(0x0E));
    dprintf(ALWAYS,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x10));
    dprintf(ALWAYS,"[U2P]addr: 0x14, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(ALWAYS,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x46, 0x38);
    USB_PHY_Write_Register8(0x40, 0x42);
    USB_PHY_Write_Register8(0xAB, 0x08);
    USB_PHY_Write_Register8(0x0C, 0x09);
    USB_PHY_Write_Register8(0x71, 0x0C);
    USB_PHY_Write_Register8(0x4F, 0x0E);
    USB_PHY_Write_Register8(0xE1, 0x10);
    USB_PHY_Write_Register8(0x5F, 0x14);
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x38, value: %x\n", USB_PHY_Read_Register8(0x38));
    dprintf(ALWAYS,"[U2P]addr: 0x42, value: %x\n", USB_PHY_Read_Register8(0x42));
    dprintf(ALWAYS,"[U2P]addr: 0x08, value: %x\n", USB_PHY_Read_Register8(0x08));
    dprintf(ALWAYS,"[U2P]addr: 0x09, value: %x\n", USB_PHY_Read_Register8(0x09));
    dprintf(ALWAYS,"[U2P]addr: 0x0C, value: %x\n", USB_PHY_Read_Register8(0x0C));
    dprintf(ALWAYS,"[U2P]addr: 0x0E, value: %x\n", USB_PHY_Read_Register8(0x0E));
    dprintf(ALWAYS,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x10));
    dprintf(ALWAYS,"[U2P]addr: 0x14, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(ALWAYS,"****************before bank 0x60*************************\n");
    USB_PHY_Write_Register8(0x60, 0xFF);
    dprintf(ALWAYS,"0x00~0x0F 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
            USB_PHY_Read_Register8(0x00),USB_PHY_Read_Register8(0x01),USB_PHY_Read_Register8(0x02),USB_PHY_Read_Register8(0x03),
            USB_PHY_Read_Register8(0x04),USB_PHY_Read_Register8(0x05),USB_PHY_Read_Register8(0x06),USB_PHY_Read_Register8(0x07),
            USB_PHY_Read_Register8(0x08),USB_PHY_Read_Register8(0x09),USB_PHY_Read_Register8(0x0A),USB_PHY_Read_Register8(0x0B),
            USB_PHY_Read_Register8(0x0C),USB_PHY_Read_Register8(0x0D),USB_PHY_Read_Register8(0x0E),USB_PHY_Read_Register8(0x0F));
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(ALWAYS,"*****************after **********************************\n");

    USB_PHY_Write_Register8(0x03, 0x14);
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x10, value: %x\n", USB_PHY_Read_Register8(0x14));
    dprintf(ALWAYS,"****************before bank 0x00*************************\n");
    USB_PHY_Write_Register8(0x00, 0xFF);
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x6A, value: %x\n", USB_PHY_Read_Register8(0x6A));
    dprintf(ALWAYS,"[U2P]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
    dprintf(ALWAYS,"[U2P]addr: 0x6C, value: %x\n", USB_PHY_Read_Register8(0x6C));
    dprintf(ALWAYS,"[U2P]addr: 0x6D, value: %x\n", USB_PHY_Read_Register8(0x6D));
    USB_PHY_Write_Register8(0x04, 0x6A);
    USB_PHY_Write_Register8(0x08, 0x68);
    USB_PHY_Write_Register8(0x2E, 0x6C);
    USB_PHY_Write_Register8(0x3E, 0x6D);
    dprintf(ALWAYS,"*****************after **********************************\n");
    dprintf(ALWAYS,"[U2P]addr: 0xFF, value: %x\n", USB_PHY_Read_Register8(0xFF));
    dprintf(ALWAYS,"[U2P]addr: 0x6A, value: %x\n", USB_PHY_Read_Register8(0x6A));
    dprintf(ALWAYS,"[U2P]addr: 0x68, value: %x\n", USB_PHY_Read_Register8(0x68));
    dprintf(ALWAYS,"[U2P]addr: 0x6C, value: %x\n", USB_PHY_Read_Register8(0x6C));
    dprintf(ALWAYS,"[U2P]addr: 0x6D, value: %x\n", USB_PHY_Read_Register8(0x6D));

    dprintf(ALWAYS,"[U2P]%s, end\n", __func__);
    return 0;
}


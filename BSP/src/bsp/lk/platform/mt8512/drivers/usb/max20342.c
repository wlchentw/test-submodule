/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * SPDX-License-Identifier: MIT
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

#include <platform/mt_i2c.h>
#include <platform/max20342.h>


#define MAX20342_REG_REVID				0x0
#define MAX20342_REG_BC_STATUS			0x0a
#define MAX20342_REG_COMMON_STATUS			0x07
#define MAX20342_REG_CC_STATUS1			0x08
#define MAX20342_REG_CC_STATUS2			0x09
#define MAX20342_REG_COMM_CTRL1			0x15
#define MAX20342_REG_COMM_CTRL2			0x16
#define MAX20342_REG_MoistDetPUCfg		0x52
#define MAX20342_REG_MoistDetPDCfg		0x53


/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define I2C_SPEED_400_KHZ    400 /* kHz */

static u8 gbMax20342_i2c_chn = 0;
static u8 gbMax20342_i2c_addr = 0x35;
/**********************************************************
  *
  *   [I2C Function For Read/Write bd71828]
  *
  *********************************************************/
static u32 max20342_write_byte(u8 addr, u8 value)
{
	int ret_code = 0;
	u8 write_data[2];
	u16 len;

	write_data[0]= addr;
	write_data[1] = value;
	len = 2;

	ret_code = mtk_i2c_write(gbMax20342_i2c_chn, gbMax20342_i2c_addr, I2C_SPEED_400_KHZ, write_data, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}

static u32 max20342_read_byte(u8 addr, u8 *dataBuffer)
{
	int ret_code = 0;
	u16 len;
	*dataBuffer = addr;

	len = 1;

	ret_code = mtk_i2c_write_read(gbMax20342_i2c_chn, gbMax20342_i2c_addr, I2C_SPEED_400_KHZ,
								dataBuffer, dataBuffer, len, len);

	if(ret_code == 0)
		return 0; // ok
	else
		return -1; // fail
}


//==============================================================================
// PMIC6398 Init Code
//==============================================================================
void max20342_init(void)
{
	u8 bRegREVId;
	u8 bRegBCStatus;
	u8 bRegCCStatus1,bRegCCStatus2;
	u8 bRegCommonStatus;
	u8 bRegCommCtrl1,bRegCommCtrl2;

	/* initial setting */
	if(0==max20342_read_byte(MAX20342_REG_REVID,&bRegREVId)) {
		u8 bReg;
		max20342_read_byte(MAX20342_REG_BC_STATUS,&bRegBCStatus);
		max20342_read_byte(MAX20342_REG_COMMON_STATUS,&bRegCommonStatus);
		dprintf(CRITICAL, "%s(),REV_ID=0x%x,BCStatus=0x%x,CommStat=0x%x\n",
			__func__,bRegREVId,bRegBCStatus,bRegCommonStatus);

		max20342_read_byte(MAX20342_REG_CC_STATUS1,&bRegCCStatus1);
		max20342_read_byte(MAX20342_REG_CC_STATUS2,&bRegCCStatus2);
		dprintf(CRITICAL, "  CCStatus=0x%x,0x%x\n",bRegCCStatus1,bRegCCStatus2);

		max20342_read_byte(MAX20342_REG_COMM_CTRL1,&bRegCommCtrl1);
		max20342_read_byte(MAX20342_REG_COMM_CTRL2,&bRegCommCtrl2);
		dprintf(CRITICAL, "  CommCtrl 1=0x%x,2=0x%x\n",bRegCommCtrl1,bRegCommCtrl2);

		bReg= bRegCommCtrl1 | 0x08;//VCCINTOnBAT , set VCCIN to avoid system reboot
		max20342_write_byte(MAX20342_REG_COMM_CTRL1,bReg);
		dprintf(CRITICAL, "  CommCtrl1=>0x%x\n",bReg);

		bReg=(bRegCommCtrl2&(~0xc0))|0x80;//USBSW => TDP & TPN .
		max20342_write_byte(MAX20342_REG_COMM_CTRL2,bReg);
		dprintf(CRITICAL, "  CommCtrl2=>0x%x\n",bReg);

	}
	else {
		dprintf(CRITICAL, "%s() not exist !? \n", __func__);
	}
}



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
#include <platform/bd71828.h>
#include <platform/ntx_hw.h>

/**********************************************************
  *   I2C Slave Setting
  *********************************************************/
#define BD71828_I2C_ID       1
#define I2C_SPEED_400_KHZ    400 /* kHz */

//#define DEBUG_I2C_DATA	1

static u8 bd71828_slave_addr = 0x4b;
/**********************************************************
  *   Init Setting
  *********************************************************/
struct pmic_setting {
	unsigned short addr;
	unsigned short val;
	unsigned short mask;
	unsigned char shift;
};

typedef struct tagRTC_Data {
	u8 bYear;
	u8 bMon;
	u8 bDay;
	u8 bHour;
	u8 bMin;
	u8 bSec;
} RTC_Data;

#define RTC_TYPE_REAL		0
#define RTC_TYPE_PWROFF		1

#define BD71828_REG_LED_CTRL		0x4a
#define BD71828_REG_CHG_LED_CTRL	0x83
#define BD71828_REG_CCNTD_0			0xb8
#define BD71828_REG_CCNTD_1			0xb7

#define BD71828_REG_CCNTD_2			0xb6 // 
#define BD71828_REG_CCNTD_3			0xb5 // 

#define BD71828_REG_BATCAP1_TH_L	0xcc // 
#define BD71828_REG_BATCAP1_TH_U	0xcd // 

#define BD71828_REG_INT_STATE3		0xe2

#define BD71828_REG_RTC_SEC		0x4c
#define BD71828_REG_RTC_MIN		0x4d
#define BD71828_REG_RTC_HOUR	0x4e
#define BD71828_REG_RTC_WEEK	0x4f
#define BD71828_REG_RTC_DAY		0x50
#define BD71828_REG_RTC_MON		0x51
#define BD71828_REG_RTC_YEAR	0x52


#define BD71828_REG_PWROFF_SEC		0xf4
#define BD71828_REG_PWROFF_MIN		0xf5
#define BD71828_REG_PWROFF_HOUR		0xf6
#define BD71828_REG_PWROFF_WEEK		0xf7
#define BD71828_REG_PWROFF_DAY		0xf8
#define BD71828_REG_PWROFF_MON		0xf9
#define BD71828_REG_PWROFF_YEAR		0xfa

static struct pmic_setting init_setting[] = {
	/* [7:0]: DCIN_CLPS, input voltage limit 4640 mv */
	{0x71, 0x3A, 0xFF, 0},
	/* [7:7]: WDT_DIS, Battery Charger Watch Dog Timer disable */
	{0x75, 0x0, 0x1, 7},
	/* [1:1]: BTMP_EN, Thermal control enable for charge voltage */
	{0x75, 0x0, 0x1, 1},
	/* [7:0]: CHG_VBAT_1, CV 4200mv */
	{0x7D, 0x30, 0xFF, 0},
	/* [7:0]: CHG_VBAT_2, CV 4200mv */
	{0x7E, 0x30, 0xFF, 0},
	/* [7:0]: CHG_VBAT_3, CV 4200mv */
	{0x7F, 0x30, 0xFF, 0},
	/* [1:1]: BUCK1_IDLE_ON, Vproc 0V */
	{0x8, 0x0, 0x1, 1},
	/* [1:1]: BUCK6_IDLE_ON, Vsram_proc 0V */
	{0x25, 0x0, 0x1, 1},
	/* [1:1]: BUCK2_IDLE_ON, Vcore sw mode */
	{0x12, 0x1, 0x1, 1},
	/* [2:1]: BUCK2_RAMPRATE, 2.5 mv/us */
	{0x14, 0, 0x3, 1},
	/* [1:1]: BUCK2_IDLE_VID, Vcore 600 mv */
	{0x15, 0x10, 0xFF, 0},
	/* [7:0]: BUCK2_SUSP_VID, Vcore 600 mv */
	{0x16, 0x10, 0xFF, 0},
	/* [1:1]: BUCK7_IDLE_ON, Vsram_core sw mode */
	{0x2F, 0x1, 0x1, 1},
	/* [2:1]: BUCK7_RAMPRATE, 2.5 mv/us */
	{0x31, 0, 0x3, 1},
	/* [1:1]: BUCK7_IDLE_VID, Vsram_core 800 mv */
	{0x32, 0x30, 0xFF, 0},
	/* [7:0]: BUCK7_SUSP_VID, Vsram_core 800 mv */
	{0x33, 0x30, 0xFF, 0},
	/* [3:3]: LDO4_RUN_BOOT_ON (EP_VDD) */
	{0x3F, 0x0, 0x1, 3},
};

/**********************************************************
  *
  *   [I2C Function For Read/Write bd71828]
  *
  *********************************************************/
static u32 bd71828_write_byte(u8 addr, u8 value)
{
	int ret_code = 0;
	u8 write_data[2];
	u16 len;

	write_data[0]= addr;
	write_data[1] = value;
	len = 2;

#ifdef DEBUG_I2C_DATA	//[	
	dprintf(CRITICAL,"%s() reg=0x%02x,val=0x%02x\n",__func__,addr,value);
#endif //] DEBUG_I2C_DATA 

	ret_code = mtk_i2c_write(BD71828_I2C_ID, bd71828_slave_addr, I2C_SPEED_400_KHZ, write_data, len);

	if(ret_code == 0) {
		return 0; // ok
	}
	else {
		dprintf(CRITICAL,"%s() reg=0x%02x write failed !!\n",__func__,addr);
		return -1; // fail
	}
}

static u32 bd71828_read_byte(u8 addr, u8 *dataBuffer)
{
	int ret_code = 0;
	u16 len;
	*dataBuffer = addr;

	len = 1;

	ret_code = mtk_i2c_write_read(BD71828_I2C_ID, bd71828_slave_addr, I2C_SPEED_400_KHZ,
								dataBuffer, dataBuffer, len, len);

#ifdef DEBUG_I2C_DATA	//[	
	dprintf(CRITICAL,"%s() reg=0x%02x,val=0x%02x\n",__func__,addr,*dataBuffer);
#endif //] DEBUG_I2C_DATA 
	
	if(ret_code == 0) {
		return 0; // ok
	}
	else {
		dprintf(CRITICAL,"%s() reg=0x%02x read failed !!\n",__func__,addr);
		return -1; // fail
	}
}

/**********************************************************
  *
  *   [Read / Write Function]
  *
  *********************************************************/
u32 pmic_read_interface(u8 RegNum, u8 *val, u8 MASK, u8 SHIFT)
{
	u8 bd71828_reg = 0;
	u32 ret = 0;

	ret = bd71828_read_byte(RegNum, &bd71828_reg);

	bd71828_reg &= (MASK << SHIFT);
	*val = (bd71828_reg >> SHIFT);

	return ret;
}

u32 pmic_config_interface(u8 RegNum, u8 val, u8 MASK, u8 SHIFT)
{
	u8 bd71828_reg = 0;
	u32 ret = 0;

	ret = bd71828_read_byte(RegNum, &bd71828_reg);

	bd71828_reg &= ~(MASK << SHIFT);
	bd71828_reg |= (val << SHIFT);

	ret = bd71828_write_byte(RegNum, bd71828_reg);

	return ret;
}


//#define _DUMP_REGS	1
#if _DUMP_REGS //[
static void bd71828_reg_dump(const char *pszTitle,u8 *pbRegA,int iRegs)
{
	u8 reg;
	int i;

	dprintf(CRITICAL, "[%s] --- %s --- \n", __func__,pszTitle);
				
	for(i=0;i<iRegs;i++)
	{
		bd71828_read_byte(pbRegA[i],&reg);
		dprintf(CRITICAL," reg@[0x%02x]=0x%02x\n",pbRegA[i],reg);
	}
}
#endif //] _DUMP_REGS

static void bd71828_init_setting(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(init_setting); i++)
		pmic_config_interface(
			init_setting[i].addr, init_setting[i].val,
			init_setting[i].mask, init_setting[i].shift);

#if _DUMP_REGS //[
	{
		u8 bDumpRegsAddrA[] = {
			0x3F,
		};
		bd71828_reg_dump("dump registers",bDumpRegsAddrA,sizeof(bDumpRegsAddrA)/sizeof(bDumpRegsAddrA[0]));
	}
#endif //] _DUMP_REGS

}

#if defined(ROHM_DISABLE_PG)
static void bd71828_dis_pg(void)
{
	/* enable hidden slave addr */
	pmic_config_interface(0xFE, 0x8C, 0xFF, 0);
	pmic_config_interface(0xFF, 0x1, 0xFF, 0);

	/* set hidden register */
	bd71828_slave_addr = 0x4d;
	//pmic_config_interface(0x9A, 0x2, 0xFF, 0);
	//pmic_config_interface(0x9E, 0x7F, 0xFF, 0);
	//pmic_config_interface(0x9F, 0x7F, 0xFF, 0);
	pmic_config_interface(0x9A, 0x0, 0xff, 0);
	pmic_config_interface(0x9E, 0x0, 0xff, 0);
	pmic_config_interface(0x9F, 0x0, 0xff, 0);
	bd71828_slave_addr = 0x4b;
}
#endif

/**********************************************************
  *
  *   [External Function]
  *
  *********************************************************/
void bd71828_set_vcore_voltage(u32 target_uv)
{
	u32 reg_val;

	if (target_uv >= BD71828_BUCK2_MAX)
		target_uv = BD71828_BUCK2_MAX;
	else if (target_uv <= BD71828_BUCK2_MIN)
		target_uv = BD71828_BUCK2_MIN;

	reg_val = (target_uv - BD71828_BUCK2_MIN) / BD71828_BUCK2_STEP_VOL;
	pmic_config_interface(BD71828_BUCK2_VOLT_RUN_BOOT, reg_val, BD71828_BUCK2_VOLT_RUN_BOOT_MASK, 0);
}

u32 bd71828_get_vcore_voltage(void)
{
	u32 reg_val, target_uv;

	pmic_read_interface(BD71828_BUCK2_VOLT_RUN_BOOT, &reg_val, BD71828_BUCK2_VOLT_RUN_BOOT_MASK, 0);
	if (reg_val >= 0xF0)
		reg_val = 0xF0;
	target_uv = ((reg_val * BD71828_BUCK2_STEP_VOL) + BD71828_BUCK2_MIN);

	return target_uv;
}

void bd71828_set_vcore_sram_voltage(u32 target_uv)
{
	u32 reg_val;

	if (target_uv >= BD71828_BUCK7_MAX)
		target_uv = BD71828_BUCK7_MAX;
	else if (target_uv <= BD71828_BUCK7_MIN)
		target_uv = BD71828_BUCK7_MIN;

	reg_val = (target_uv - BD71828_BUCK7_MIN) / BD71828_BUCK7_STEP_VOL;
	pmic_config_interface(BD71828_BUCK7_VOLT_RUN_BOOT, reg_val, BD71828_BUCK7_VOLT_RUN_BOOT_MASK, 0);
}

u32 bd71828_get_vcore_sram_voltage(void)
{
	u32 reg_val, target_uv;

	pmic_read_interface(BD71828_BUCK7_VOLT_RUN_BOOT, &reg_val, BD71828_BUCK7_VOLT_RUN_BOOT_MASK, 0);
	if (reg_val >= 0xF0)
		reg_val = 0xF0;
	target_uv = ((reg_val * BD71828_BUCK7_STEP_VOL) + BD71828_BUCK7_MIN);

	return target_uv;
}

void bd71828_set_vmem_voltage(u32 target_uv)
{
	u32 reg_val;

	if (target_uv > BD71828_BUCK4_MAX)
		target_uv = BD71828_BUCK4_MAX;
	else if (target_uv <= BD71828_BUCK4_MIN)
		target_uv = BD71828_BUCK4_MIN;

	reg_val = (target_uv - BD71828_BUCK4_MIN) / BD71828_BUCK4_STEP_VOL;
	pmic_config_interface(BD71828_BUCK4_VOLT, reg_val, BD71828_BUCK4_VOLT_MASK, 0);
}

u32 bd71828_get_vmem_voltage(void)
{
	u32 reg_val, target_uv;

	pmic_read_interface(BD71828_BUCK4_VOLT, &reg_val, BD71828_BUCK4_VOLT_MASK, 0);
	if (reg_val >= 0x20)
		reg_val = 0x20;
	target_uv = ((reg_val * BD71828_BUCK4_STEP_VOL) + BD71828_BUCK4_MIN);

	return target_uv;
}

void bd71828_poweroff(void)
{

#if 0
	u8 bTemp;
	u8 bPwrKeyStatOld ;

	pmic_read_interface (BD71828_REG_INT_STATE3,&bPwrKeyStatOld,0x3c,0);
	dprintf(CRITICAL, "[%s] checking the power key ,pwrkey=0x%0x \n",
		   	__func__,bPwrKeyStatOld);
	do {
		pmic_read_interface (BD71828_REG_INT_STATE3,&bTemp,0x3c,0);
		if(bTemp!=bPwrKeyStatOld) {
			dprintf(CRITICAL, "[%s] pwrkey changed 0x%0x=>0x%0x \n",
				__func__,bPwrKeyStatOld,bTemp);
			bPwrKeyStatOld = bTemp;
		}
		if( (bTemp & 0x3c) &0x10) {
			dprintf(CRITICAL, "[%s] pwrkey short clicked -> powering off \n",
				__func__,bPwrKeyStatOld,bTemp);
			break;
		}
		else if(!((bTemp & 0x3c) &0x20)) {
			dprintf(CRITICAL, "[%s] pwrkey released ? -> powering off \n",
				__func__,bPwrKeyStatOld,bTemp);
			break;
		}
		pmic_config_interface(BD71828_REG_INT_STATE3, 0x00, 0x3c, 0); // clear all power key stat let it can power down device .
		mdelay(100);

	} while (1) ;
#endif


	{
		//keep power off date-time
		u32 value;

		pmic_read_interface(BD71828_REG_RTC_YEAR, &value, 0xff, 0); // year .
		pmic_config_interface(BD71828_REG_PWROFF_YEAR, value, 0xff, 0);

		pmic_read_interface(BD71828_REG_RTC_MON, &value, 0x1f, 0); // month .
		pmic_config_interface(BD71828_REG_PWROFF_MON, value, 0x1f, 0);

		pmic_read_interface(BD71828_REG_RTC_DAY, &value, 0x3f, 0); // day .
		pmic_config_interface(BD71828_REG_PWROFF_DAY, value, 0x3f, 0);

		pmic_read_interface(BD71828_REG_RTC_WEEK, &value, 0x07, 0); // week . 
		pmic_config_interface(BD71828_REG_PWROFF_WEEK, value, 0x07, 0);

		pmic_read_interface(BD71828_REG_RTC_HOUR, &value, 0xbf, 0); // hour .
		pmic_config_interface(BD71828_REG_PWROFF_HOUR, value, 0xbf, 0);

		pmic_read_interface(BD71828_REG_RTC_MIN, &value, 0x7f, 0);// min .
		pmic_config_interface(BD71828_REG_PWROFF_MIN, value, 0x7f, 0);

		pmic_read_interface(BD71828_REG_RTC_SEC, &value, 0x7f, 0);// sec .
		pmic_config_interface(BD71828_REG_PWROFF_SEC, value, 0x7f, 0);
	}
	pmic_config_interface(BD71828_REG_LED_CTRL, 0x80, 0xC0, 0);
	pmic_config_interface(0x03, 0x00, 0xff, 0); // reset source clear . 
	pmic_config_interface(BD71828_REG_INT_STATE3, 0x08, 0xff, 0); // only reserved middle key .
	

	pmic_config_interface(0x04, 0x02, 0x02, 0);// goto HBNT .

	while (1) {
		dprintf(CRITICAL, "[%s] wait for poweroff\n", __func__);
		mdelay(1000);
	}
}

static int _bd71828_get_rtc_data(int iRTC_Type,RTC_Data *O_ptRTC_Data)
{
	u8 regSec,regMin,regHour;
	u8 regDay,regMonth,regYear;


	if(RTC_TYPE_REAL==iRTC_Type) {
		regSec = BD71828_REG_RTC_SEC;
		regMin = BD71828_REG_RTC_MIN;
		regHour = BD71828_REG_RTC_HOUR;
		regDay = BD71828_REG_RTC_DAY;
		regMonth = BD71828_REG_RTC_MON;
		regYear = BD71828_REG_RTC_YEAR;
	}
	else if(RTC_TYPE_PWROFF==iRTC_Type) {
		regSec = BD71828_REG_PWROFF_SEC;
		regMin = BD71828_REG_PWROFF_MIN;
		regHour = BD71828_REG_PWROFF_HOUR;
		regDay = BD71828_REG_PWROFF_DAY;
		regMonth = BD71828_REG_PWROFF_MON;
		regYear = BD71828_REG_PWROFF_YEAR;
	}

	if(O_ptRTC_Data) {	
		pmic_read_interface(regSec, &O_ptRTC_Data->bSec, 0x7f, 0);// sec .
		pmic_read_interface(regMin, &O_ptRTC_Data->bMin, 0x7f, 0);// min .
		pmic_read_interface(regHour, &O_ptRTC_Data->bHour, 0xbf, 0); // hour .
		pmic_read_interface(regDay, &O_ptRTC_Data->bDay, 0x3f, 0); // day .
		pmic_read_interface(regMonth, &O_ptRTC_Data->bMon, 0x1f, 0); // month .
		pmic_read_interface(regYear, &O_ptRTC_Data->bYear, 0xff, 0); // year .
	}

	return 0;
}


int _bd71828_cmp_rtc_data_secs(RTC_Data *I_ptRTC_a,RTC_Data *I_ptRTC_b)
{
	int iRet = 0;
	int iSecs_a , iSecs_b ;

	if( I_ptRTC_a->bYear != I_ptRTC_b->bYear ) {
		iRet += 10000000;
		goto out;
	}

	
	if( I_ptRTC_a->bMon != I_ptRTC_b->bMon ) {
		iRet += 1000000;
		goto out;
	}


	if( I_ptRTC_a->bDay != I_ptRTC_b->bDay ) {
		iRet += 1000000;
		goto out;
	}

	iSecs_a = (I_ptRTC_a->bHour*60*60)+(I_ptRTC_a->bMin*60)+I_ptRTC_a->bSec;
	iSecs_b = (I_ptRTC_b->bHour*60*60)+(I_ptRTC_b->bMin*60)+I_ptRTC_b->bSec;
	
	

	iRet = iSecs_a - iSecs_b;
	dprintf(CRITICAL, "a secs=%d , b secs=%d , diff=%d \n",
	   	iSecs_a , iSecs_b ,iRet);

out:
	return iRet;
}

//==============================================================================
// PMIC6398 Init Code
//==============================================================================
void pmic_init_bd71828(void)
{

	/* initial setting */
	bd71828_init_setting();
	

#if defined(ROHM_DISABLE_PG)
	/* wrokaround solution for 2nd power-on issue*/
	bd71828_dis_pg();
#endif

	

	{
		unsigned char bootsrcReg=0x00,bootresetReg=0x00,bootkeystatusReg=0x00,bRegAddr;

		pmic_read_interface(0x03, &bootresetReg, 0xff, 0);
		dprintf(CRITICAL, "BD71828 RESETSRC [%02x]\n", bootresetReg);
		pmic_read_interface(BD71828_REG_INT_STATE3, &bootkeystatusReg, 0x3c, 0);
		dprintf(CRITICAL, "BD71828 KEYSTATUS [%02x]\n", bootkeystatusReg);
		pmic_read_interface(0x02, &bootsrcReg, 0xff, 0);
		dprintf(CRITICAL, "BD71828 BOOTSRC [%02x]\n", bootsrcReg);

		if(bootsrcReg&0x80) {
			// cold reset . 
			if(bootresetReg&0x04) {
				// 
				dprintf(CRITICAL, "COLD reset from SWRESET, do nothing\n");
			}
			else {
				dprintf(CRITICAL, "COLD reset from others, poweroff\n");
				 if((bootresetReg&0x01) && (bootkeystatusReg&0x04) && !(bootkeystatusReg&0x10)) { //ignore the USB in for system poweroff
					dprintf(CRITICAL, "KEY NONE RELEASE, enter poweroff\n");
					bd71828_poweroff();
				}else{
					pmic_config_interface(BD71828_REG_INT_STATE3, 0x3c, 0xff, 0);
				}
			}
		}
		else {
			RTC_Data tRTC_Real , tRTC_Pwroff;

			_bd71828_get_rtc_data(RTC_TYPE_REAL,&tRTC_Real);
			_bd71828_get_rtc_data(RTC_TYPE_PWROFF,&tRTC_Pwroff);

			if(_bd71828_cmp_rtc_data_secs(&tRTC_Real,&tRTC_Pwroff)<=8) {
				if(!(bootsrcReg&0x02) && (bootresetReg&0x01) && (bootkeystatusReg&0x04) && !(bootkeystatusReg&0x10)) {
					dprintf(CRITICAL, "KEY NONE RELEASE, enter poweroff\n");
					bd71828_poweroff();
				}
			}
		}
	}

#if 0
	// move to platform.c
	NTX_HWCONFIG *hwcfg = gethwconfig();
	if(hwcfg->m_val.bPCB == 0x6F) { // E70T04
		// LED set on platform.c
	}
	else
		bd71828_led_ctrl(BD71828_LED_ID_GRN,BD71828_LED_STAT_ON);
#endif

	dprintf(CRITICAL, "[%s] BD71828 LK End\n", __func__);
}

int bd71828_dcin_state(void)
{
	u8 reg = 0;
	int ret = 0;

	ret =(int) bd71828_read_byte(BD71828_DCIN_STAT, &reg);
	if (ret!=-1) {
		if(reg&0x1) {
			ret = 1;
		}
		else {
			ret = 0;
		}
	}
	else 
	{
		ret = 0;
	}

	return ret ;
}

int bd71828_powerkey_status(void)
{
	int iRet,iChk;
	unsigned char bReg,bRegBootSrc,bRegAddr;
	static int giBD71828_power_key_down = 0;

	// ROHM BD71828
	bRegAddr=0x02;
	iChk = (int)bd71828_read_byte(bRegAddr,&bRegBootSrc);
	if(iChk<0) {
		dprintf(CRITICAL,"%s() reading BootSrc failed!\n",__func__);
	}

	if(!(bRegBootSrc&0x1)) {
		dprintf(CRITICAL,"%s() not poweron key boot !\n",__func__);
		return 0;
	}


	bRegAddr=BD71828_REG_INT_STATE3;
	iChk = (int)bd71828_read_byte(bRegAddr,&bReg);
	if(iChk<0) {
		iRet = iChk;
	}
	else {
		//printf("BD71828 key status 0x%x\n",bReg);
		if (bReg & 0x20) {
			// power key press .
			giBD71828_power_key_down = 1;
		}

		if (bReg & 0x10) {
			// power key short clicked .
			giBD71828_power_key_down = 0;
		}

		if(bReg & 0x08) {
			// power key middle .
			giBD71828_power_key_down = 2;
		}

		bReg &= 0x3C;
		bd71828_write_byte(bRegAddr,bReg);

		iRet = giBD71828_power_key_down ;
	}
	return iRet;
}

int bd71828_is_charge_done(void)
{
	int iRet=0,iChk=0;
	unsigned short wCC , wCC_Thd;
	unsigned char bReg,bReg2;

	iChk |= (int)bd71828_read_byte(BD71828_REG_CCNTD_2,&bReg);
	iChk |= (int)bd71828_read_byte(BD71828_REG_CCNTD_3,&bReg2);
	if(iChk) {
		dprintf(CRITICAL, "[%s] read register CCNTD failed !\n", __func__);
		iRet = -1;
	}
	wCC = bReg2<<8;
	wCC |= bReg;

	iChk |= (int)bd71828_read_byte(BD71828_REG_BATCAP1_TH_L,&bReg);
	iChk |= (int)bd71828_read_byte(BD71828_REG_BATCAP1_TH_U,&bReg2);
	if(iChk) {
		dprintf(CRITICAL, "[%s] read register BATCAP1_TH failed !\n", __func__);
		iRet = -2;
	}
	wCC_Thd = bReg2<<8;
	wCC_Thd |= bReg;


	if(wCC>=wCC_Thd) {
		iRet = 1;
	}
	else {
		iRet = 0;
	}

	return iRet;
}

int bd71828_led_ctrl(int iLED_id,int iLED_state)
{
	int iRet=0,iChk;
	unsigned char bReg,bRegNew,bRegAddr;

	// ROHM BD71828
	bRegAddr=BD71828_REG_LED_CTRL;// LED_CTRL
	iChk = (int)bd71828_read_byte(bRegAddr,&bReg);
	if(iChk<0) {
		iRet = iChk;
	}
	else {

		bRegNew = bReg;

		switch(iLED_id) {
		case BD71828_LED_ID_AUTO:
			if(BD71828_LED_STAT_ON==iLED_state) 
			{
				bRegNew &= ~0xc0;
				iRet = 1;
			}
			break;
		case BD71828_LED_ID_AMB:
			if(BD71828_LED_STAT_ON==iLED_state) {
				bRegNew &= ~0x80;
				bRegNew |= 0x80;
				iRet = 1;
			}
			else if(BD71828_LED_STAT_OFF==iLED_state) {
				bRegNew &= ~0xc0;
				bRegNew |= 0x40;
				iRet = 1;
			}
			else if(BD71828_LED_STAT_GET==iLED_state) {
				if(bReg&0xc0) {
					iRet = (bReg&0x80)?1:0;
				}
				else {
					iRet = 2;// led controlled by rohm .
				}
			}
			break;
		case BD71828_LED_ID_GRN:
			if(BD71828_LED_STAT_ON==iLED_state) {
				bRegNew &= ~0x40;
				bRegNew |= 0x40;
				iRet = 1;
			}
			else if(BD71828_LED_STAT_OFF==iLED_state) {
				bRegNew &= ~0xc0;
				bRegNew |= 0x80;
				iRet = 1;
			}
			else if(BD71828_LED_STAT_GET==iLED_state) {
				if(bReg&0xc0) {
					iRet = (bReg&0x40)?1:0;
				}
				else {
					if(bReg&0x1) {
						iChk = bd71828_is_charge_done();
						// CHG_DONE_LED_EN 1
						if(1==iChk) {
							iRet = 1;
						}
						else if(0==iChk) {
							iRet = 0;
						}
						else {
							iRet = 2;// unknown state controlled by Rohm .
						}
					}
					else {
						// CHG_DONE_LED_EN 0
						iRet = 0;
					}
				}
			}
			break;
		default :
			break;
		}
		
		if(BD71828_LED_STAT_GET!=iLED_state) {
			if(bRegNew!=bReg) {
				bd71828_write_byte(bRegAddr,bRegNew);
			}
		}

	}
	return iRet;

}


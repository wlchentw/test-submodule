#ifndef _rt5748_SW_H_
#define _rt5748_SW_H_

//---------------------- AUTO GEN ---------------------------
/* BUCK Registers */
#define RT5748_REG_VSEL0	(0x00)
#define RT5748_REG_VSEL1	(0x01)
#define RT5748_REG_CTRL1	(0x02)
#define RT5748_REG_ID1		(0x03)
#define RT5748_REG_ID2		(0x04)
#define RT5748_REG_MONITOR	(0x05)
#define RT5748_REG_CTRL2	(0x06)
#define RT5748_REG_CTRL3	(0x07)
#define RT5748_REG_CTRL4	(0x08)

/* PMIC Registers mask and shift,mask is HEX;shift is Integer*/
#define rt5748_vsell_vol_mask	    0xff
#define rt5748_vsell_vol_shift	    0
#define rt5748_vsell_enable_mask	0x01
#define rt5748_vsell_enable_shift	0
#define rt5748_vsell_mode_mask	    0x01
#define rt5748_vsell_mode_shift	    0x00

#define rt5748_vselh_vol_mask	    0xff
#define rt5748_vselh_vol_shift	    0
#define rt5748_vselh_enable_mask    0x02
#define rt5748_vselh_enable_shift   1
#define rt5748_vselh_mode_mask	    0x02
#define rt5748_vselh_mode_shift	    1

#define rt5748_rampup_rate_mask	    0x07
#define rt5748_rampup_rate_shift    4
#define rt5748_rampdown_rate_mask	0x07
#define rt5748_rampdown_rate_shift  5
#define rt5748_softstart_rate_mask	0x03
#define rt5748_softstart_rate_shift 2

#define rt5748_discharge_func_mask	0x01
#define rt5748_discharge_func_shift 7

/*************************** I2C Slave Setting*********************************************/
#define rt5748_SLAVE_ADDR   0x50
#define rt5748_I2C_ID_P1    3
#define rt5748_I2C_ID_M2    2

//---------------------- EXPORT API ---------------------------
extern u32 rt5748_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT, u8 i2c_bus);
extern u32 rt5748_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT, u8 i2c_bus);
extern void rt5748_init(void);

#endif


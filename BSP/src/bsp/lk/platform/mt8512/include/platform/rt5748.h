#ifndef _rt5749_SW_H_
#define _rt5749_SW_H_

//---------------------- AUTO GEN ---------------------------
/* BUCK Registers */
#define RT5749_REG_VSEL0	(0x00)
#define RT5749_REG_VSEL1	(0x01)
#define RT5749_REG_CTRL1	(0x02)
#define RT5749_REG_ID1		(0x03)
#define RT5749_REG_ID2		(0x04)
#define RT5749_REG_MONITOR	(0x05)
#define RT5749_REG_CTRL2	(0x06)
#define RT5749_REG_CTRL3	(0x07)
#define RT5749_REG_CTRL4	(0x08)

/* PMIC Registers mask and shift,mask is HEX;shift is Integer*/
#define rt5749_vsell_vol_mask	    0xff
#define rt5749_vsell_vol_shift	    0
#define rt5749_vsell_enable_mask	0x01
#define rt5749_vsell_enable_shift	0
#define rt5749_vsell_mode_mask	    0x01
#define rt5749_vsell_mode_shift	    0x00

#define rt5749_vselh_vol_mask	    0xff
#define rt5749_vselh_vol_shift	    0
#define rt5749_vselh_enable_mask    0x02
#define rt5749_vselh_enable_shift   1
#define rt5749_vselh_mode_mask	    0x02
#define rt5749_vselh_mode_shift	    1

#define rt5749_rampup_rate_mask	    0x07
#define rt5749_rampup_rate_shift    4
#define rt5749_rampdown_rate_mask	0x07
#define rt5749_rampdown_rate_shift  5
#define rt5749_softstart_rate_mask	0x03
#define rt5749_softstart_rate_shift 2

#define rt5749_discharge_func_mask	0x01
#define rt5749_discharge_func_shift 7

typedef enum {
	VCORE = 0,
    VCORE_SRAM,
    VPROC_SRAM,
} BUCK_USER_TYPE;

/*************************** I2C Slave Setting*********************************************/
#define rt5749_VCORE_SLAVE_ADDR			0x51
#define rt5749_VCORE_I2C_ID				0

#define rt5749_VCORE_SRAM_SLAVE_ADDR	0x52
#define rt5749_VCORE_SRAM_I2C_ID		0

#define rt5749_VPROC_SRAM_SLAVE_ADDR	0x52
#define rt5749_VPROC_SRAM_I2C_ID		1

//---------------------- EXPORT API ---------------------------
extern int rt5749_regulator_get_voltage(BUCK_USER_TYPE type);
extern int rt5749_regulator_set_voltage(BUCK_USER_TYPE type, unsigned int volt);
extern void rt5749_init(void);

#endif


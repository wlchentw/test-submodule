// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
#ifndef __voltage_def_h__
#define __voltage_def_h__
#include <platform.h>
#include <platform_def.h>

#define I2C0_BASE_ADDR 0x11007000
#define I2C1_BASE_ADDR 0x10019000
#define I2C2_BASE_ADDR 0x1001e000
#define I2C_CG_STA	0x100010c8
#define I2C_CG_CLR	0x100010c4
#define I2C_CG_SET	0x100010c0
#define I2C0_CG_SHIFT	24
#define I2C1_CG_SHIFT	25
#define I2C2_CG_SHIFT	26
#define I2C_MUX_CG_PDN_SHIFT	15
/*I2C0/I2C1 setting*/
#define VCORE_SLAVE_ID			0xa2
#define VCORE_SRAM_SLAVE_ID		0xa4
#define VPROC_SLAVE_ID			0xc4
#define VPROC_SRAM_SLAVE_ID		0xa4

#define VCORE_ADDR				0x01
#define VCORE_SRAM_ADDR			0x01
#define VPROC_ADDR				0x08
#define VPROC_SRAM_ADDR			0x06

#define VCORE_VOL				0x3c
#define VCORE_SRAM_VOL			0x64
#define VPROC_OFF_VOL			0x7c
#define VPROC_ON_VOL			0x7e
#define VPROC_SRAM_OFF_VOL		0x61
#define VPROC_SRAM_ON_VOL		0x63


#define I2C_CLK_STA	0x100000a0
#define I2C_CLK_SET	0x100000a4
#define I2C_CLK_CLR	0x100000a8
#define I2C_CLK_MASK	0x7
#define I2C_CLK_SHIFT	8



#define I2C_SLAVE_ID_SHIFT		24
#define I2C_ADDR_SHIFT			8
#define I2C_VALUE_SHIFT			0
#define I2C_SLAVE_ID	(0xFF << I2C_SLAVE_ID_SHIFT)
#define I2C_REG_ADDR	(0xFF << I2C_ADDR_SHIFT)
#define I2C_REG_VALUE	(0xFF << I2C_VALUE_SHIFT)

/*PWM setting for CORE 8512B */
#define REG_32K 0x10000228
#define PWM_CLK_STA	0x100000a0
#define PWM_CLK_SET	0x100000a4
#define PWM_CLK_CLR	0x100000a8
#define PWM_CLK_MASK	0x7
#define PWM_CLK_SHIFT	16
#define PWM_PDN_MASK	0x1
#define PWM_PDN_SHIFT	23

#define GPIO_PWM_CORE_SRAM_CTRL 	0    //for 8512B power solution
#define I2C0_CORE_SRAM_CTRL  		0	 //for 8110 SLT power solution
#define I2C1_PROC_SRAM_MERGE_CTRL	0	 //for 8512B power solution
#define I2C1_PROC_CORE_ALL			1	 //for 8110P1

#define VCORE_LEVEL_ADDR	0x100066B4
#define VCORE_LEVEL_HIGH	0x4

enum {
	I2C_MUX_CG = 0,
	I2C_MUX,
	I2C0_CG,
	I2C1_CG,
	I2C_CLK_BACKUP_NUM,
};

#if I2C0_CORE_SRAM_CTRL
void spm_i2c0_core_sram_voltage(void);
void spm_i2c0_core_clk_setting(void);
#endif

#if GPIO_PWM_CORE_SRAM_CTRL
void spm_pwm_comm_setting(void);
void spm_pwm_core_voltage(void);
#endif

void spm_i2c_comm_setting(void);
void spm_i2c1_proc_voltage(void);
void spm_i2c1_proc_sram_voltage(void);
void spm_i2c1_proc_clk_setting(void);
#if I2C1_PROC_CORE_ALL
#define VCORE_ROHM_SLAVE_ID		0x96
#define VCORE_ROHM_ADDR			0x04
#define VCORE_0P6_VOL			0x8
#define VCORE_0P8_VOL			0x0

void spm_i2c1_proc_core_all(void);
#endif
uint32_t get_i2c_clk_backup(int);
uint32_t get_pwm_clk_backup(void);

#endif

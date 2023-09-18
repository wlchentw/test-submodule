// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
#include "voltage_def.h"
#include <mmio.h>
#include <mt_spm_reg.h>
#include <platform.h>
#include <platform_def.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

extern void mtk_i2c_set_gpio_mode(unsigned char bus_num);
static uint32_t i2c_clk_backup[I2C_CLK_BACKUP_NUM];
static uint32_t pwm_clk_backup;

static inline int set_dvfs_cmd_value(uint32_t tmp, uint32_t slave_id, uint32_t addr, uint32_t i2c_value)
{
	tmp = (tmp & ~I2C_SLAVE_ID) | (slave_id << I2C_SLAVE_ID_SHIFT);
	tmp = (tmp & ~I2C_REG_ADDR) | (addr << I2C_ADDR_SHIFT);
	tmp = (tmp & ~I2C_REG_VALUE)  | (i2c_value << I2C_VALUE_SHIFT);
	return tmp;
}

uint32_t get_i2c_clk_backup(int i){
	return i2c_clk_backup[i];
}

uint32_t get_pwm_clk_backup(void)
{
	return pwm_clk_backup;
}

#if I2C0_CORE_SRAM_CTRL
void spm_i2c0_core_sram_voltage(void)
{
	uint32_t tmp = 0;

	//mtk_i2c_set_gpio_mode(0);
	/*vcore set to 0.6v for 8110 SLT*/
	tmp = mmio_read_32(SPM_DVFS_CMD0);
	mmio_write_32(SPM_DVFS_CMD0, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, VCORE_VOL));

	tmp = mmio_read_32(SPM_DVFS_CMD1);
	mmio_write_32(SPM_DVFS_CMD1, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x46)); //set 0p65

	tmp = mmio_read_32(SPM_DVFS_CMD2);
	mmio_write_32(SPM_DVFS_CMD2, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x50)); //set 0p7

	tmp = mmio_read_32(SPM_DVFS_CMD3);
	mmio_write_32(SPM_DVFS_CMD3, set_dvfs_cmd_value(tmp, VCORE_SLAVE_ID, VCORE_ADDR, 0x64)); //set 0p8

	/*vcore_sram set to 0.8v for 8110 SLT*/
	tmp = mmio_read_32(SPM_DVFS_CMD4);
	mmio_write_32(SPM_DVFS_CMD4, set_dvfs_cmd_value(tmp, VCORE_SRAM_SLAVE_ID, VCORE_SRAM_ADDR , VCORE_SRAM_VOL));

	tmp = mmio_read_32(SPM_DVFS_CMD5);
	mmio_write_32(SPM_DVFS_CMD5, set_dvfs_cmd_value(tmp, VCORE_SRAM_SLAVE_ID, VCORE_SRAM_ADDR , 0x78));	//set 0p9
}
#endif

#if GPIO_PWM_CORE_SRAM_CTRL
void spm_pwm_comm_setting(void)
{
	uint32_t pwm_tmp = 0, pwm_thresh = 0;
	/*we need switch pwm clk to 26M*/
	pwm_tmp = mmio_read_32(PWM_CLK_STA);
	pwm_clk_backup = (pwm_tmp & (PWM_CLK_MASK << PWM_CLK_SHIFT)) >> PWM_CLK_SHIFT;
	mmio_write_32(PWM_CLK_CLR, (PWM_CLK_MASK << PWM_CLK_SHIFT));
	mmio_write_32(PWM_CLK_SET, (0x0 << PWM_CLK_SHIFT));

	mmio_write_32(0x10001084, 0x200000);//PWM clk clr
	mmio_write_32(0x100010A8, 0x1);//pwm clk clr
}
void spm_pwm_core_voltage(void)
{
	uint32_t pwm_thresh = 0;
	pwm_thresh = mmio_read_32(0x1001D180);
	mmio_write_32(SPM_DVFS_CMD0, 0x20); //pwm_thresh
	mmio_write_32(SPM_DVFS_CMD3, pwm_thresh); //pwm backup thresh for wakeup
}
#endif

void spm_i2c_comm_setting(void)
{
	uint32_t i2c_tmp = 0;
	/*we need switch i2c clk to 26M by me*/
	//mmio_write_32(I2C_CLK_CLR, 0x1 << 15);
	i2c_tmp = mmio_read_32(I2C_CLK_STA);
	i2c_clk_backup[I2C_MUX] = (i2c_tmp & (I2C_CLK_MASK << I2C_CLK_SHIFT)) >> I2C_CLK_SHIFT;
	i2c_clk_backup[I2C_MUX_CG] = (i2c_tmp & (0x1 << 15)) >> 15;
	if (i2c_clk_backup[I2C_MUX_CG])
		mmio_write_32(I2C_CLK_CLR, 0x1 << 15);
	mmio_write_32(I2C_CLK_CLR, (I2C_CLK_MASK << I2C_CLK_SHIFT));
	mmio_write_32(I2C_CLK_SET, (0x0 << I2C_CLK_SHIFT));
}

void spm_i2c1_proc_voltage(void)
{
	uint32_t tmp = 0;

	/*vproc set to 0v for 8110 SLT when dpidle*/
	tmp = mmio_read_32(SPM_DVFS_CMD12);
	mmio_write_32(SPM_DVFS_CMD12, set_dvfs_cmd_value(tmp, VPROC_SLAVE_ID, VPROC_ADDR, VPROC_OFF_VOL));

	/*vproc set to normal voltage for 8110 SLT when resume from dpidle*/
	tmp = mmio_read_32(SPM_DVFS_CMD13);
	mmio_write_32(SPM_DVFS_CMD13, set_dvfs_cmd_value(tmp, VPROC_SLAVE_ID, VPROC_ADDR, VPROC_ON_VOL));
}

void spm_i2c1_proc_sram_voltage(void)
{
	uint32_t tmp = 0;

	/*vproc_sram set to 0v for 8110 SLT when dpidle*/
	tmp = mmio_read_32(SPM_DVFS_CMD14);
	mmio_write_32(SPM_DVFS_CMD14, set_dvfs_cmd_value(tmp, VPROC_SRAM_SLAVE_ID, VPROC_SRAM_ADDR, VPROC_SRAM_OFF_VOL));

	/*vproc_sram set to normal voltage for 8110 SLT when resume from dpidle*/
	tmp = mmio_read_32(SPM_DVFS_CMD15);
	mmio_write_32(SPM_DVFS_CMD15, set_dvfs_cmd_value(tmp, VPROC_SRAM_SLAVE_ID, VPROC_SRAM_ADDR, VPROC_SRAM_ON_VOL));
}

void spm_i2c1_proc_clk_setting(void)
{
	i2c_clk_backup[I2C1_CG] = (mmio_read_32(I2C_CG_STA) & (0x1 << I2C1_CG_SHIFT)) >> I2C1_CG_SHIFT;
	if (i2c_clk_backup[I2C1_CG])
		mmio_write_32(I2C_CG_CLR, 0x1 << I2C1_CG_SHIFT);
	mmio_write_32(0x10019050, 0x1);
	mmio_write_32(0x10019040, 0x3);
	mmio_write_32(0x10019054, 0x0);
	mmio_write_32(0x10019020, 0x12a); // speed
}

#if I2C1_PROC_CORE_ALL
void spm_i2c1_proc_core_all(void)
{
	unsigned int tmp = 0;
	/*vcore set to 0.6v for 8110P1*/
	tmp = mmio_read_32(SPM_DVFS_CMD0);
	mmio_write_32(SPM_DVFS_CMD0, set_dvfs_cmd_value(tmp, VCORE_ROHM_SLAVE_ID, VCORE_ROHM_ADDR, VCORE_0P6_VOL));

	tmp = mmio_read_32(SPM_DVFS_CMD3);
	mmio_write_32(SPM_DVFS_CMD3, set_dvfs_cmd_value(tmp, VCORE_ROHM_SLAVE_ID, VCORE_ROHM_ADDR, VCORE_0P8_VOL));
}
#endif

#if I2C0_CORE_SRAM_CTRL
void spm_i2c0_core_clk_setting(void)
{
	i2c_clk_backup[I2C0_CG] = (mmio_read_32(0x100010c8) & (0x1 << 24) >> 24);
	if (i2c_clk_backup[I2C0_CG])
		mmio_write_32(0x100010c4, 0x02000000);
	mmio_write_32(0x100010c4, 0x01000000);
	mmio_write_32(0x11007050, 0x1);
	mmio_write_32(0x11007040, 0x3);
	mmio_write_32(0x11007054, 0x0);
	mmio_write_32(0x11007020, 0x12a); // speed
}
#endif


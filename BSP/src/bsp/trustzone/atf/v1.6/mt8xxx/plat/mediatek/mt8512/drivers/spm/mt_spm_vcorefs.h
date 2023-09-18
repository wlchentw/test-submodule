// SPDX-License-Identifier: MediaTekProprietary AND BSD-3-Clause
#ifndef __MT_SPM_VCOREFS__H__
#define __MT_SPM_VCOREFS__H__

void spm_vcorefs_args(uint64_t x1, uint64_t x2, uint64_t x3);
void spm_request_dvfs_opp(uint64_t id, uint64_t reg);

enum vcorefs_smc_cmd {
	VCOREFS_SMC_CMD_0,
	VCOREFS_SMC_CMD_1,
	VCOREFS_SMC_CMD_2,
	VCOREFS_SMC_CMD_3,
	VCOREFS_SMC_CMD_4,
	NUM_VCOREFS_SMC_CMD,
};

enum dvfsrc_channel {
	DVFSRC_CHANNEL_1 = 1,
	DVFSRC_CHANNEL_2,
	DVFSRC_CHANNEL_3,
	DVFSRC_CHANNEL_4,
	NUM_DVFSRC_CHANNEL,
};

struct reg_config {
	uint32_t offset;
	uint32_t val;
};

enum {
	SPMFW_LP4_3200 = 0,
	SPMFW_LP4X_2400,
	SPMFW_LP4X_3200,
	SPMFW_LP4SIP_2800,
	SPMFW_LP3_1866,
	SPMFW_PSRAM_2133
};

enum {
	VCORE_PWM_VSARM_GPIO =0,
	VCORE_I2C0_VSRAM_I2C0,
	VCORE_I2C1_VSRAM_I2C1,
	VCORE_I2C2_VSRAM_I2C2,
	VCORE_GPIO_VSRAM_GPIO,
	VCORE_PWM,
	VCORE_GPIO,
	PWR_PWRAP,
	PWR_UNDEF
};

#endif /* __MT_SPM_VCOREFS__H__ */

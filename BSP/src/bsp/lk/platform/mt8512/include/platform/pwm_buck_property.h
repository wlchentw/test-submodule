#ifndef _pwm_buck_property_SW_H_
#define _pwm_buck_property_SW_H_

#include <platform/pwm-buck.h>
#include <platform/mt_reg_base.h>

#define PWM_NUM_VCORE	5

#define GPIO_VCORE_BASE (GPIO_BASE+0x230)
#define GPIO_VCORE_PWM_OFFSET 27
#define GPIO_VCORE_PWM_MODE 0x3

/*define pwm period or frequency*/
#define PWM_PERIOD_INIT	650000

PWM_VOLT_MAP vcore_map[] = {
	{600000, 83},
	{650000, 63},
	{675000, 53},
	{700000, 43},
	{750000, 23},
	{800000, 0},
};

#endif


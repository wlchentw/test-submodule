#ifndef _pwm_buck_property_SW_H_
#define _pwm_buck_property_SW_H_

#include <platform/pwm-buck.h>
#include <platform/mt_reg_base.h>

#define PWM_1V8	1

#define PWM_NUM_VCORE	0

#define GPIO_VCORE_BASE (IO_PHYS+0x00005350)
#define GPIO_VCORE_PWM_OFFSET 9
#define GPIO_VCORE_PWM_MODE 0x2

/*define pwm period or frequency*/
#define PWM_PERIOD_INIT	32000

#if defined(PWM_3V3)
PWM_VOLT_MAP vcore_map[] = {
	{700000, 93},
	{800000, 65},
	{900000, 35},
	{1000000, 0},
};
#endif

#if defined(PWM_1V8)
PWM_VOLT_MAP vcore_map[] = {
	{700000, 84},
	{800000, 58},
	{900000, 31},
	{1000000, 0},
};
#endif

#endif


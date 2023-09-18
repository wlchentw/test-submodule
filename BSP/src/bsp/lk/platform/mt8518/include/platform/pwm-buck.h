#ifndef _pwm_buck_SW_H_
#define _pwm_buck_SW_H_

struct pwm_volt_map {
	unsigned int volt;
	unsigned int duty_cycle;
};

#define PWM_VOLT_MAP struct pwm_volt_map

struct pwm_buck {
	unsigned int pwm_period;
	unsigned int n_table;
	struct pwm_volt_map *table;
	unsigned int pwm_gpio;
};

#define BUCK_OK			0
#define BUCK_EINVAL		22

#define BUCKLOG(x...) printf(x)
#define BUCKERR(x...) printf(x)

//---------------------- EXPORT API ---------------------------
extern int regulator_get_voltage(void);
extern int regulator_set_voltage(unsigned int volt);
extern int regulator_is_enabled(void);
extern int regulator_enable(int enable);
extern int pwm_buck_init(void);

#endif


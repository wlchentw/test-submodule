#ifndef _PWM_H_
#define _PWM_H_

/*
  pwm_no: 0/1/2/3
  freq:   eg: 32K: 32000
  returns: 0 means successful, < 0 means failed.
*/
extern int pwm_config_freq(int pwm_no, int freq);

/*
  pwm_no: 0/1/2/3
  returns: >= 0 means successfully, return value is period, < 0 means failed.
*/
extern int pwm_get_period(int pwm_no);

/*
  pwm_no: 0/1/2/3
  returns: >= 0 means successful, return value is duty, < 0 means failed.
*/
extern int pwm_get_duty_cycle(int pwm_no);

/*
  pwm_no: 0/1/2/3
  returns:  0 means successful, < 0 means failed.
*/
extern int pwm_set_duty(int pwm_no, int duty);

/*
  pwm_no: 0/1/2/3
  returns:  0 means successful, < 0 means failed.
*/
extern int pwm_enable(int pwm_no);


/*
  pwm_no: 0/1/2/3
  returns:  0 means successful, < 0 means failed.
*/
extern int pwm_disable(int pwm_no);

/*
  pwm_no: 0/1/2/3
  returns:   >= 0 means successful, return value is the wave numbers already been sent, < 0 means failed.
*/
int pwm_get_send_wavenums(int pwm_no);

void pwm_dump(int pwm_no);
void pwm_dump_all(void);

extern void pwm_init (void);

#endif


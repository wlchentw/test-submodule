#ifndef _m2296_SW_H_
#define _m2296_SW_H_

#define m2296_SLAVE_ADDR   0x12
#define m2296_I2C_ID       1
#define M2296_CHIP_VER     0x10

//---------------------- EXPORT API ---------------------------
extern int m2296_hw_init(void);
extern void mt2296_sw_reset(void);

#endif


#ifndef _mt6395_SW_H_
#define _mt6395_SW_H_

//---------------------- EXPORT API ---------------------------
extern u32 pmic_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT);
extern u32 pmic_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT);
extern void pmic_init_mt6395 (void);
extern int pmic_detect_powerkey(void);

#endif



#ifndef _PL_MT_PMIC_H_
#define _PL_MT_PMIC_H_

//==============================================================================
// PMIC Register Index
//==============================================================================
//register number
#include "upmu_hw.h"

//==============================================================================
// PMIC define
//==============================================================================
typedef enum {
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,          // USB : 450mA
    CHARGING_HOST,
    NONSTANDARD_CHARGER,    // AC : 450mA~1A 
    STANDARD_CHARGER,       // AC : ~1A
    APPLE_2_1A_CHARGER,     // 2.1A apple charger
    APPLE_1_0A_CHARGER,     // 1A apple charger
    APPLE_0_5A_CHARGER,     // 0.5A apple charger
} CHARGER_TYPE;

#define PMIC_CHRDET_EXIST            1
#define PMIC_CHRDET_NOT_EXIST        0

#define E_PWR_INVALID_DATA     22  /* invalid argument */

//==============================================================================
// PMIC Exported Function
//==============================================================================
extern CHARGER_TYPE mt_charger_type_detection(void);
extern void pmic_init (void);
extern u32 pmic_read_interface (u32 RegNum, u32 *val, u32 MASK, u32 SHIFT);
extern u32 pmic_config_interface (u32 RegNum, u32 val, u32 MASK, u32 SHIFT);

#endif 


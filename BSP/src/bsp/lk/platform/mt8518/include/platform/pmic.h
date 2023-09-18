
#ifndef _PL_MT_PMIC_H_
#define _PL_MT_PMIC_H_

//==============================================================================
// PMIC define
//==============================================================================
#define PMIC6391_E1_CID_CODE    0x1091
#define PMIC6391_E2_CID_CODE    0x2091
#define PMIC6391_E3_CID_CODE    0x3091
#define PMIC6397_E1_CID_CODE    0x1097
#define PMIC6397_E2_CID_CODE    0x2097
#define PMIC6397_E3_CID_CODE    0x3097
#define PMIC6397_E4_CID_CODE    0x4097

typedef enum {
    CHARGER_UNKNOWN = 0,
    STANDARD_HOST,
    CHARGING_HOST,
    NONSTANDARD_CHARGER,
    STANDARD_CHARGER,
} CHARGER_TYPE;

//==============================================================================
// PMIC Status Code
//==============================================================================
#define PMIC_TEST_PASS               0x0000
#define PMIC_TEST_FAIL               0xB001
#define PMIC_EXCEED_I2C_FIFO_LENGTH  0xB002
#define PMIC_CHRDET_EXIST            0xB003
#define PMIC_CHRDET_NOT_EXIST        0xB004

//==============================================================================
// PMIC Exported Function
//==============================================================================
extern CHARGER_TYPE mt_charger_type_detection(void);
extern int pmic_detect_powerkey(void);
extern u32 pmic_read_interface (u32 RegNum, u32 *val, u32 MASK, u32 SHIFT);
extern u32 pmic_config_interface (u32 RegNum, u32 val, u32 MASK, u32 SHIFT);
extern u32 pmic_init (void);

#endif // _PL_MT_PMIC_H_


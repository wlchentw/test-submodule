#ifndef _BD71828_SW_H_
#define _BD71828_SW_H_

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

/* regulator info */
#define BD71828_BUCK2_VOLT_RUN_BOOT 0x17
#define BD71828_BUCK2_VOLT_RUN_BOOT_MASK 0xFF
#define BD71828_BUCK2_MIN 500000
#define BD71828_BUCK2_MAX 2000000
#define BD71828_BUCK2_STEP_VOL 6250

#define BD71828_BUCK4_VOLT 0x21
#define BD71828_BUCK4_VOLT_MASK 0x3F
#define BD71828_BUCK4_MIN 1000000
#define BD71828_BUCK4_MAX 1800000
#define BD71828_BUCK4_STEP_VOL 25000

#define BD71828_BUCK7_VOLT_RUN_BOOT 0x34
#define BD71828_BUCK7_VOLT_RUN_BOOT_MASK 0xFF
#define BD71828_BUCK7_MIN 500000
#define BD71828_BUCK7_MAX 2000000
#define BD71828_BUCK7_STEP_VOL 6250

#define BD71828_DCIN_STAT	0x68

extern u32 pmic_read_interface (u8 RegNum, u8 *val, u8 MASK, u8 SHIFT);
extern u32 pmic_config_interface (u8 RegNum, u8 val, u8 MASK, u8 SHIFT);
extern void pmic_init_bd71828 (void);

extern void bd71828_set_vcore_voltage(u32 target_uv);
extern u32 bd71828_get_vcore_voltage(void);
extern void bd71828_set_vcore_sram_voltage(u32 target_uv);
extern u32 bd71828_get_vcore_sram_voltage(void);
extern void bd71828_set_vmem_voltage(u32 target_uv);
extern u32 bd71828_get_vmem_voltage(void);
extern int bd71828_dcin_state(void);
extern int bd71828_powerkey_status(void);

#define BD71828_LED_ID_AUTO		0
#define BD71828_LED_ID_AMB		1
#define BD71828_LED_ID_GRN		2
#define BD71828_LED_STAT_ON		1
#define BD71828_LED_STAT_OFF	0
#define BD71828_LED_STAT_GET	(-1)
extern int bd71828_led_ctrl(int iLED_id,int iLED_state);
int bd71828_is_charge_done(void);
void bd71828_poweroff(void);

#endif

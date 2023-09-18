/*
 * Copyright (C) 2020 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef __LINUX_MFD_M2296_I2C_H__
#define __LINUX_MFD_M2296_I2C_H__

#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/input.h>

/** version **/
#define M2296_CHIP_ID 0x10
/* Regulator IDs */
#define M2296_REG_INTR_A       0x00
#define M2296_REG_INTR_B       0x01
#define M2296_REG_INTR_A_MASK  0x02
#define M2296_REG_INTR_B_MASK  0x03
#define M2296_REG_STATUS_A 0x04
#define M2296_REG_STATUS_B 0x05
#define M2296_REG_START_UP_STATUS  0x06
#define M2296_REG_SYS_CONTROL  0x07
#define M2296_REG_DCDC1_VOLT  0x0A
#define M2296_REG_DCDC1_SLPVOLT 0x0D
#define M2296_REG_DC1_MODE_CONTROL 0x0E
#define M2296_REG_DC2DC3_MODE_CONTROL 0x0F
#define M2296_REG_DC4_MODE_CONTROL 0x11
#define M2296_REG_LDO1LDO2_VOLT  0x12
#define M2296_REG_LDO3LDO4_VOLT  0x13
#define M2296_REG_LDO5LDO6_VOLT  0x14
#define M2296_REG_LDO7LDO8_VOLT  0x15
#define M2296_REG_LDO9_VOLT 0x16
#define M2296_REG_LDO10_RTCLDO_VOLT 0x17
#define M2296_REG_LDO1LDO2_SLPVOLT 0x18
#define M2296_REG_LDO3_SLPVOLT 0x19
#define M2296_REG_DCDC2_VOLT  0x1A
#define M2296_REG_LDO7LDO8_SLPVOLT  0x1B
#define M2296_REG_LDO9_SLPVOLT  0x1C
#define M2296_REG_LDO10_SLPVOLT   0x1D
#define M2296_REG_LDO1_LDO4_MODE_CONTROL 0x1E
#define M2296_REG_LDO5_LDO8_MODE_CONTROL 0x1F
#define M2296_REG_LDO9_LDO10_MODE_CONTROL 0x20
#define M2296_REG_LDO1_LDO8_ONOFF_CONTROL 0x21
#define M2296_REG_LDO9_DC4_ONOFF_CONTROL  0x22
#define M2296_REG_ADC_CODE_VBAT_BIT11_BIT4 0x23
#define M2296_REG_ADC_CODE_VBAT_BIT3_BIT0 0x24
#define M2296_REG_ADC_CODE_ADC1_BIT11_BIT4  0x25
#define M2296_REG_ADC_CODE_ADC1_BIT3_BIT0  0x26
#define M2296_REG_ADC_CODE_ADC2_BIT11_BIT4 0x27
#define M2296_REG_ADC_CODE_ADC2_BIT3_BIT0 0x28
#define M2296_REG_RTC_CONTROL1 0x30
#define M2296_REG_RTC_CONTROL2  0x31
#define M2296_REG_Seconds_BCD 0x32
#define M2296_REG_Minutes_BCD 0x33
#define M2296_REG_Hours_BCD 0x34
#define M2296_REG_Days_BCD 0x35
#define M2296_REG_Weekdays 0x36
#define M2296_REG_Months_Century_BCD 0x37
#define M2296_REG_Years_BCD 0x38
#define M2296_REG_Minutes_Alarm 0x39
#define M2296_REG_Hour_Alarm 0x3A
#define M2296_REG_Day_Alarm_BCD 0x3B
#define M2296_REG_Weekday_Alarm 0x3C
#define M2296_REG_Second_Alarm 0x3D
#define M2296_REG_Timer_Control  0x3E
#define M2296_REG_Timer_Countdown_Value 0x3F
#define M2296_REG_ENDISCH1 0x40
#define M2296_REG_ENDISCH2 0x41
#define M2296_REG_PWRKEY_Control1 0x42
#define M2296_REG_PWRKEY_Control2  0x43
#define M2296_REG_VERSION 0x44
#define M2296_REG_DCDC_Fault_Status 0x45
#define M2296_REG_LDO_Fault_Status 0x46
#define M2296_REG_DCDC_Interrupt_Mask 0x47
#define M2296_REG_LDO_Interrupt_Mask 0x48
#define M2296_REG_User_Reserved 0x50
#define M2296_REG_GMT_Testing 0xF1
#define M2296_MAX_REGISTER  M2296_REG_User_Reserved

/***Interrupt Handler **/
enum m2296_interrupt_type {
	M2296_REG_INT_UNKNOWN = 0,
	/* 0x00 b2 EOC_ADC
	 * EOC_ADC is used to record the 12bit-ADC status.
	 * EOC_ADC is reset to 0 when this byte is read
	 * each time .This bit is also reset to 0 at each VCC plug-in/out.
	 * EOC_ADC <--> ADC status
	 * 0 Converting
	 * 1 End of Convert
	 */
	M2296_REG_INT_EOC_ADC,
	/* 0x00 b3 PWRON_IT
	 * PWRON_IT is used to record the PWRON rising status.
	 * PWRON_IT is reset to 0 when this byte is read each time.
	 * This bit is reset to 0 at each PWRON rising edge or VCC plug-in/out.
	 * PWRON_IT <--> T>TdbPWRONF
	 * 0 NO
	 * 1 YES
	 */
	M2296_REG_INT_PWRON_IT,
	/* 0x00 b4 PWRON_LP
	 * PWRON_LP is used to record the PWRON long press status.
	 * PWRON_LP is reset to 0 when this
	 * byte is read each time.
	 * This bit is also reset to 0 at each PWRON rising edge or
	 * VCC plug-in/out.
	 * PWRON_LP <--> T>TdPWRONLP
	 * 0 NO
	 * 1 YES
	 */
	M2296_REG_INT_PWRON_LP,
	/* 0x00 b5 PWRON PWRON is used to record the
	 *PWRON pin status has changed since last read
	 */
	M2296_REG_INT_PWRON,

	/* 0x01 b0 RTC_ALARM
	 * RTC_ALARM is used to record the RTC alarm status.
	 * RTC_ALARM is reset to 0 when this byte is
	 * read each time .This bit is also reset to 0 at each VCC plug-in/out.
	 * RTC_ALARM <--> RTC alarm Happen
	 * 0 NO
	 * 1 YES
	 */
	M2296_REG_INT_RTC_ALARM,
	/* 0x01 b3 T1MIN
	 * T1MIN is used to record the one minute timer status.
	 * T1MIN is reset to 0 when this byte is read
	 * each time .This bit is also reset to 0 at each VCC plug-in/out.
	 * T1MIN <--> One minute timer reached
	 * 0 NO
	 * 1 YES
	 */
	M2296_REG_INT_T1MIN,
	/* 0x01 b4 T1HOUR
	 * T1HOUR is used to record the one hour timer status.
	 * T1HOUR is reset to 0 when this byte is read
	 * each time .This bit is also reset to 0 at each VCC plug-in/out.
	 * T1HOUR <--> One hour timer reached
	 * 0 NO
	 * 1 YES
	 */
	M2296_REG_INT_T1HOUR,

	/*M2296_REG_INT_PWRON_LPOFF,*/
	/*M2296_REG_INT_BATLOW,*/
};

/**0x00 INTR_A **/
#define M2296_INTR_A_INT BIT(7)/*rw*/
#define M2296_INTR_A_PWRON BIT(5) /*R*/
#define M2296_INTR_A_PWRON_LP BIT(4) /*R*/
#define M2296_INTR_A_PWRON_IT BIT(3) /*R*/
#define M2296_INTR_A_EOC_ADC BIT(2) /*R*/

/* 0x00 b7 INT
 * INT is used to control the output status of /INT pin.
 * When interrupt events happen, /INT pin goes
 * low and this bit is set to 1’b1.
 * After Micro-processor write the bit to be 1’b0,
 * /INT pin goes to high-Z
 * state.
 * INT /INT Pin Status
 * 0 Hi-Z
 * 1 Low
 */
#define INTR_A_INT_LOW  (0x01 << 7)
#define INTR_A_INT_HIZ (INTR_A_INT_LOW & (~INTR_A_INT_LOW))

/* 0x00 b5 PWRON PWRON is used to record the PWRON pin
 * status has changed since last read.
 */
#define INTR_A_PWRON (0x01 << 5)

/* 0x00 b4 PWRON_LP
 * PWRON_LP is used to record the PWRON long press status.
 * PWRON_LP is reset to 0 when this
 * byte is read each time .This bit is also reset to 0 at each PWRON
 * rising edge or VCC plug-in/out.
 * PWRON_LP T>TdPWRONLP
 * 0 NO
 * 1 YES
 */
#define INTR_A_PWRON_LP (0x01 << 4)

/* 0x00 b3 PWRON_IT
 * PWRON_IT is used to record the PWRON rising status.
 * PWRON_IT is reset to 0 when this byte is
 * read each time . This bit is reset to 0 at each PWRON
 * rising edge or VCC plug-in/out.
 * PWRON_IT T>TdbPWRONF
 * 0 NO
 * 1 YES
 */
#define INTR_A_PWRON_LI (0x01 << 3)

/* 0x00 b2 EOC_ADC
 * EOC_ADC is used to record the 12bit-ADC status.
 * EOC_ADC is reset to 0 when this byte is read
 * each time .This bit is also reset to 0 at each VCC plug-in/out.
 * EOC_ADC ADC status
 * 0 Converting
 * 1 End of Convert
 */
#define INTR_A_EOC_ADC (0x01 << 2)


/**0x01 INTR_B **/
#define M2296_INTR_B_T1HOUR BIT(4)
#define M2296_INTR_B_T1MIN BIT(3)
#define M2296_INTR_B_RTC_ALARM BIT(0)

/* 0x01 b4 T1HOUR
 * T1HOUR is used to record the one hour timer status.
 * T1HOUR is reset to 0 when this byte is read
 * each time .This bit is also reset to 0 at each VCC plug-in/out.
 * T1HOUR One hour timer reached
 * 0 NO
 * 1 YES
 */
#define INTR_B_T1HOUR (0x01 << 4)

/* 0x01 b3 T1MIN
 * T1MIN is used to record the one minute timer status.
 * T1MIN is reset to 0 when this byte is read
 * each time .This bit is also reset to 0 at each VCC plug-in/out.
 * T1MIN One minute timer reached
 * 0 NO
 * 1 YES
 */
#define INTR_B_T1MIN (0x01 << 3)

/* 0x01 b0 RTC_ALARM
 * RTC_ALARM is used to record the RTC alarm status.
 * RTC_ALARM is reset to 0 when this byte is
 * read each time .This bit is also reset to 0 at each VCC plug-in/out.
 * RTC_ALARM RTC alarm Happen
 * 0 NO
 * 1 YES
 */
#define INTR_B_RTC_ALARM (0x01 << 0)

/***reg address: regmap_irq_chip.status_base + reg_offset */
#define M2296_REG_INTR_A_OFFSET 0x00
#define M2296_REG_INTR_B_OFFSET 0x01

/** interrupt mask **/
/** 0x02 INTR_A_MASK **/
#define M2296_MASK_EOCADC  BIT(2)/*0x04*/
#define M2296_MASK_IT  BIT(3)/*0x08*/
#define M2296_MASK_LP  BIT(4)/*0x10*/
#define M2296_MASK_PWRON  BIT(5)/*0x20*/
#define M2296_MASK_PWRON_IT_LP (M2296_MASK_PWRON \
				| M2296_MASK_LP | M2296_MASK_IT)

/* 0x02 b4 MASK_LP
 * By writing 1’b1 to MASK_LP to disable asserting /INT low
 * when the event of PWRON is toggled
 * high with duration longer than TdPWRONLP occurs.
 */
#define MASK_LP (0x01 << 4)

/* 0x02 b3 MASK_IT
 * By writing 1’b1 to MASK_IT to disable asserting /INT low
 * when the event of PWRON is toggled
 * high duration longer than TdPWRONLP occurs.
 */
#define MASK_IT (0x01 << 3)

/* 0x02 b2 MASK_EOCADC
 * By writing 1’b1 to MASK_EOCADC to disable asserting
 * /INT low when the event of ADC end of
 * conversion occurs.
 */
#define MASK_EOCADC (0x01 << 2)

/* 0x03 INTR_B_MASK */
#define M2296_MASK_RTC_ALARM BIT(0)/*0x01*/
#define M2296_MASK_T1MIN BIT(3)/*0x08*/
#define M2296_MASK_T1HOUR BIT(4)/*0x10*/
#define M2296_MASK_T1HOUR_T1MIN_RTC_ALARM (M2296_MASK_RTC_ALARM \
			| M2296_MASK_T1MIN | M2296_MASK_T1HOUR)

/* 0x03 b4 MASK_1HOUR
 * By writing 1’b1 to MASK_1HOUR to disable asserting
 * /INT low when the event of T1HOUR occurs.
 */
#define MASK_1HOUR (0x01 << 4)

/* 0x03 b3 MASK_1MIN
 * By writing 1’b1 to MASK_1HOUR to disable asserting
 * /INT low when the event of T1MIN occurs.
 */
#define MASK_1MIN (0x01 << 3)

/* 0x03 b0 MASK_ALARM
 * By writing 1’b1 to MASK_ALARM to disable asserting
 * /INT low when the event of RTC_ALARM occurs
 */
#define MASK_ALARM (0x01 << 0)

/* PMIC Start-up Status--read only
 * 0x06 b3 TLLP_RESTARTUP
 * Record the PMIC start-up procedure is caused by the PWRON
 * long pressed with duration longer
 * than TdPWRONLPOFF when register LPOFF_TO_DO=1’b1
 * 0x06 b2 ONHOLD_STARTUP
 * Record the PMIC start-up procedure is caused by the PWRHOLD1
 * or PWRHOLD2 high-toggled,
 * or SOFTON is written to 1’b1.
 * 0x06 b1 RTCALARM_STARTUP
 * Record the PMIC start-up procedure is caused by the RTC ALARM.
 * 0x06 b0 PWRON_STARTUP
 * Record the PMIC start-up procedure is caused by the
 * PWRON long pressed with duration longer
 * than TdPWRONIT.
 */
#define M2296_TLLP_RESTARTUP BIT(3)
#define M2296_ONHOLD_STARTUP BIT(2)
#define M2296_RTCALARM_STARTUP BIT(1)
#define M2296_PWRON_STARTUP BIT(0)



/* Software PMIC ON/OFF Control and 12-bit ADC Enable Control
 * 0x07 b7 SOFTON
 * Write 0 to SOFTON, and toggle low to PWRHOLDx to start
 * PMU power-off sequence when PMIC is in operation mode.
 * When PMIC is in shutdown mode with PWRON_LPOFF=1’b0,
 * write 1 to SOFTON to enable PMIC power-on sequence.
 * Write 1 to SOFTON to keep PMIC in operation mode
 * after PMIC start-up by PWRON press.
 * 0x07 b5 ADCON
 * Write 1 to enable 12-bit ADC start to convert data.
 * 0x07 b4 EN_ADC1
 * Write 1 to enable ADC converting data by the voltage applied at ADC1 pin.
 * 0x07 b3 EN_ADC2
 * Write 1 to enable ADC converting data by the voltage applied at ADC2 pin.
 */
#define M2296_SOFTON BIT(7)
#define M2296_ADCON BIT(5)
#define M2296_EN_ADC1 BIT(4)
#define M2296_EN_ADC2 BIT(3)

#define SOFTON_KEEP  (0x01 << 7)
#define ADCON_ENABLE  (0x01 << 5)
#define EN_ADC1_ENABLE  (0x01 << 4)
#define EN_ADC2_ENABLE  (0x01 << 3)

/** 0x0E DC1_MODE CONTROL **/
#define M2296_DC1_NRMMODE 0xc0/**[b7,b6] **/
#define M2296_DC1_SLPMODE  0x30/**[b5,b4] **/

/** 0x0F DC2DC3_MODE CONTROL **/
#define M2296_DC3_NRMMODE 0xc0/**[b7,b6] **/
#define M2296_DC3_SLPMODE  0x30/**[b5,b4] **/
#define M2296_DC2_NRMMODE 0x0c/**[b3,b2] **/
#define M2296_DC2_SLPMODE 0x03/**[b1,b0] **/
#define M2296_DC2_DC3_SLPMODE  (M2296_DC2_SLPMODE | M2296_DC3_SLPMODE)

/** 0x11 DC4_MODE CONTROL **/
#define M2296_DC4_NRMMODE 0x0c/**[b3,b2] **/
#define M2296_DC4_SLPMODE  0x03/**[b1,b0] **/

/* DC1_SLPMODE[1:0] [b5,b4],DCDC1 Operating Mode
 * 00 Auto PWM/PSM w/o ECO
 * 01 Auto PWM/PSM with ECO
 * 10 Auto PWM/PSM with ECO
 * 11 shutdown
 */
#define DC1_SLPMODE_WOECO 0x00
#define DC1_SLPMODE_ENABLE 0x10/**slpmode == Auto PWM/PSM with ECO **/
#define DC1_SLPMODE_SHUTDOWN 0x30

/* DCx_SLPMODE[1:0](DC2,DC3,DC4) DCDCx Operating Mode
 * 00 Auto PWM/PSM w/o ECO
 * 01 Auto PWM/PSM with ECO
 * 10 Auto PWM/PSM with ECO
 * 11 shutdown
 */
#define DC2_SLPMODE_WOECO 0x00
#define DC3_SLPMODE_WOECO 0x00
#define DC4_SLPMODE_WOECO 0x00
#define DC2_SLPMODE_ENABLE 0x01/**[b1,b0] **/
#define DC3_SLPMODE_ENABLE 0x10/**[b5,b4] **/
#define DC4_SLPMODE_ENABLE 0x01/**[b1,b0] **/
#define DC2_DC3_SLPMODE_ENABLE (DC2_SLPMODE_ENABLE | DC3_SLPMODE_ENABLE)
#define DC2_SLPMODE_SHUTDOWN 0x03
#define DC3_SLPMODE_SHUTDOWN 0x30
#define DC4_SLPMODE_SHUTDOWN 0x03
#define DC2_DC3_SLPMODE_SHUTDOWN (DC2_SLPMODE_SHUTDOWN | DC3_SLPMODE_SHUTDOWN)
/* DCx_NRMMODE[1:0]
 * Setting the operating mode of DCDC1, DCDC2, DCDC3,
 * and DCDC4 when SLEEP pin is toggled high, Default=2’b00
 * DCx_NRMMODE[1:0] DCDCx Operating Mode
 * 00 Auto PWM/PSM with ECO
 * 01 Auto PWM/PSM with ECO
 * 10 Force PWM
 * 11 Auto PWM/PSM w/o ECO
 */
#define DC1_NRMMODE_ENABLE 0x00
#define DC2_NRMMODE_ENABLE 0x00
#define DC3_NRMMODE_ENABLE 0x00
#define DC4_NRMMODE_ENABLE 0x00


/** 0x1E LDO1_LDO4_MODE CONTROL **/
#define M2296_LDO1_SLPMODE 0x03/**[b1,b0] **/
#define M2296_LDO2_SLPMODE 0xc0/**[b3,b2] **/
#define M2296_LDO3_SLPMODE 0x30/**[b5,b4] **/
#define M2296_LDO4_SLPMODE 0xc0/**[b7,b6] **/

/** 0x1F LDO5_LDO8_MODE CONTROL **/
#define M2296_LDO5_SLPMODE 0x03/**[b1,b0] **/
#define M2296_LDO6_SLPMODE 0xc0/**[b3,b2] **/
#define M2296_LDO7_SLPMODE 0x30/**[b5,b4] **/
#define M2296_LDO8_SLPMODE 0xc0/**[b7,b6] **/

/**0x20 LDO9_LDO10_MODE CONTROL **/
#define M2296_LDO9_SLPMODE 0xc0/**[b3,b2] **/
#define M2296_LDO10_SLPMODE 0x30/**[b5,b4] **/

/* Group A: LDO1, LDO2, LDO3, LDO7, LDO8, LDO9, LDO10
 * LDOx_SLPMODE[1:0] MODE
 * 11 Shutdown X
 * 10 ECO
 * 01 ECO
 * 00 Norma
 */
#define LDO1_SLPMODE_SHUTDOWN  0x03
#define LDO2_SLPMODE_SHUTDOWN  0xc0
#define LDO3_SLPMODE_SHUTDOWN  0x30
#define LDO7_SLPMODE_SHUTDOWN  0x30
#define LDO8_SLPMODE_SHUTDOWN  0xc0
#define LDO9_SLPMODE_SHUTDOWN  0xc0
#define LDO10_SLPMODE_SHUTDOWN  0x30

#define LDO1_SLPMODE_ENABLE (0x01 << 0)
#define LDO2_SLPMODE_ENABLE (0x01 << 2)
#define LDO3_SLPMODE_ENABLE (0x01 << 4)
#define LDO7_SLPMODE_ENABLE (0x01 << 4)
#define LDO8_SLPMODE_ENABLE (0x01 << 6)
#define LDO9_SLPMODE_ENABLE (0x01 << 2)
#define LDO10_SLPMODE_ENABLE (0x01 << 4)

/* Group B: LDO4, LDO5, LDO6
 * LDOx_SLPMODE[1:0] MODE
 * 11 Shutdown
 * 10 Normal
 * 01 ECO
 * 00 Normal
 */
#define LDO4_SLPMODE_SHUTDOWN 0xc0
#define LDO5_SLPMODE_SHUTDOWN 0x03
#define LDO6_SLPMODE_SHUTDOWN 0xc0

#define LDO4_SLPMODE_ENABLE (0x01 << 6)
#define LDO5_SLPMODE_ENABLE (0x01 << 0)
#define LDO6_SLPMODE_ENABLE (0x01 << 2)


/* DCDC1~DCDC4, and LDO1 ~ LDO10 Enable Signal.
 * ONDCx ONLDOx<-->DCDCx, LDOx Status
 * 0 Shutdown
 * 1 Operation
 * 0x21 LDO1_LDO8_ONOFF CONTROL
 */
#define M2296_ONLDO8 BIT(7)
#define M2296_ONLDO7 BIT(6)
#define M2296_ONLDO6 BIT(5)
#define M2296_ONLDO5 BIT(4)
#define M2296_ONLDO4 BIT(3)
#define M2296_ONLDO3 BIT(2)
#define M2296_ONLDO2 BIT(1)
#define M2296_ONLDO1 BIT(0)

#define ONLDO1_ON (0x01 << 0)
#define ONLDO1_OFF (ONLDO1_ON & (~ONLDO1_ON))

#define ONLDO2_ON (0x01 << 1)
#define ONLDO2_OFF (ONLDO2_ON & (~ONLDO2_ON))

#define ONLDO3_ON (0x01 << 2)
#define ONLDO3_OFF (ONLDO3_ON & (~ONLDO3_ON))

#define ONLDO4_ON (0x01 << 3)
#define ONLDO4_OFF (ONLDO4_ON & (~ONLDO4_ON))

#define ONLDO5_ON (0x01 << 4)
#define ONLDO5_OFF (ONLDO5_ON & (~ONLDO5_ON))

#define ONLDO6_ON (0x01 << 5)
#define ONLDO6_OFF (ONLDO6_ON & (~ONLDO6_ON))

#define ONLDO7_ON (0x01 << 6)
#define ONLDO7_OFF (ONLDO7_ON & (~ONLDO7_ON))

#define ONLDO8_ON (0x01 << 7)
#define ONLDO8_OFF (ONLDO7_ON & (~ONLDO7_ON))


/*0x22 LDO9_DC4_ONOFF CONTROL
 */
#define M2296_ONDC4 BIT(7)
#define M2296_ONDC3 BIT(6)
#define M2296_ONDC2 BIT(5)
#define M2296_ONDC1 BIT(4)
#define M2296_ONLDO10 BIT(2)
#define M2296_ONLDO9 BIT(1)

#define ONDC3_ON  (0x01 << 6)
#define ONDC3_OFF  (ONDC3_ON & (~ONDC3_ON))
/* 0x40 ENDISCH1
 * ENDIS_DCx Discharge function
 * 0 Disable
 * 1 Enable
 */
#define M2296_ENDIS_DC1 BIT(6)
#define M2296_ENDIS_DC2 BIT(5)
#define M2296_ENDIS_DC3 BIT(4)
#define M2296_ENDIS_DC4 BIT(3)
#define M2296_ENDIS_L10 BIT(1)
#define M2296_ENDIS_L9 BIT(0)
#define M2296_ENDIS_DC1_DC2_DC3_DC4 (M2296_ENDIS_DC1 | \
		M2296_ENDIS_DC2 | M2296_ENDIS_DC3 | M2296_ENDIS_DC4)
#define M2296_ENDIS_L9_L10 (M2296_ENDIS_L9 | M2296_ENDIS_L10)

/* 0x41 ENDISCH2
 * ENDIS_Lx Discharge function
 * 0 Disable
 * 1 Enable
 */
#define M2296_ENDIS_L8 BIT(7)
#define M2296_ENDIS_L7 BIT(6)
#define M2296_ENDIS_L6 BIT(5)
#define M2296_ENDIS_L5 BIT(4)
#define M2296_ENDIS_L4 BIT(3)
#define M2296_ENDIS_L3 BIT(2)
#define M2296_ENDIS_L2 BIT(1)
#define M2296_ENDIS_L1 BIT(0)
#define M2296_ENDIS_LX_ALL (M2296_ENDIS_L1 | M2296_ENDIS_L2 \
		| M2296_ENDIS_L3 | M2296_ENDIS_L4 | M2296_ENDIS_L5 \
		| M2296_ENDIS_L6 | M2296_ENDIS_L7 | M2296_ENDIS_L8)

/* 0x42 b3 LLP_TO_DO
 * Setting PMIC operating mode after it finishes power-off
 * sequence caused by PWRON long
 * pressed with duration longer than TdPWRONLLP. Default=1’b0
 * LLP_TO_DO <--> PMIC operating mode
 * 0 Remain in shutdown mode
 * 1 Re-startup by sequence
 */
#define M2296_LLP_TO_DO  BIT(3)

/* 0x42 b2 ENLLP
 * Enable PMIC shutdown when PWRON long pressed with duration
 * longer than TdPWRONLLP
 * defined by register TIME_LLP[1:0]. Default=1’b0
 * T>TdPWRONLLP <--> PMIC shutdown
 * 0 NO
 * 1 YES
 */
#define M2296_ENLLP BIT(2)

/* 0x42 b1,b0 TIME_LP
 * TIME_LP[1:0] is used to defined the PWRON
 * long-press delay time TdPWRONLP.
 * When PWRON is toggled high with duration
 * longer than TdPWRONLP,
 * the register PWRON_LP is
 * 1’b1. Default=2’b01
 * TIME_LP[1:0]<-->TdPWRONLP
 * 00 1.0S
 * 01 2.0S
 * 10 3.0S
 * 11 4.0S
 */
#define M2296_TIME_LP_1SEC 0x00
#define M2296_TIME_LP_2SEC 0x01
#define M2296_TIME_LP_3SEC 0x02
#define M2296_TIME_LP_4SEC 0x03

/* 0x47 DCDC Interrupt Mask
 * bit7 Mask_ERRDC4
 * bit6 Mask_ERRDC3
 * bit5 Mask_ERRDC2
 * bit4 Mask_ERRDC1
 * bit3 Mask_ERRLDO6
 * bit2 Mask_ERRLDO8
 */
#define M2296_ERRDC4_MASK BIT(7)
#define M2296_ERRDC3_MASK BIT(6)
#define M2296_ERRDC2_MASK BIT(5)
#define M2296_ERRDC1_MASK BIT(4)
#define M2296_ERRLDO6_MASK BIT(3)
#define M2296_ERRLDO8_MASK BIT(2)


/* 0x48 LDO Interrupt Mask
 * bit7 Mask_ERRLDO1
 * bit6 Mask_ERRLDO2
 * bit5 Mask_ERRLDO3
 * bit4 Mask_ERRLDO4
 * bit3 Mask_ERRLDO5
 * bit2 Mask_ERRLDO7
 * bit1 Mask_ERRLDO9
 * bit0 Mask_ERRLDO10
 */
#define M2296_ERRLDO1_MASK BIT(7)
#define M2296_ERRLDO2_MASK BIT(6)
#define M2296_ERRLDO3_MASK BIT(5)
#define M2296_ERRLDO4_MASK BIT(4)
#define M2296_ERRLDO5_MASK BIT(3)
#define M2296_ERRLDO7_MASK BIT(2)
#define M2296_ERRLDO9_MASK BIT(1)
#define M2296_ERRLDO10_MASK BIT(0)

struct m2296_voltage_config {
	u8 regvalue;
	int voltage;
};

enum m2296_irq_idx {
	M2296_IRQIDX_IRQ0 = 0,
	M2296_IRQIDX_IRQ1,
	M2296_IRQIDX_MAX,
};

//regulator mode
/* Setting the operating mode of DCDC1, DCDC2, DCDC3,
 * and DCDC4 when SLEEP pin is toggled high, Default=2’b00
 * DCx_NRMMODE[1:0] DCDCx Operating Mode
 * 00  Auto PWM/PSM with ECO
 * 01  Force PWM
 * 10  Force PWM
 * 11  Auto PWM/PSM w/o ECO
 */
enum m2296_regulator_mode {
	M2296_REGULATOR_MODE_UNKNOWN = 0,
	M2296_REGULATOR_MODE_AUTO_PWM_PSM_ECO,
	M2296_REGULATOR_MODE_FORCE_PWM,
	M2296_REGULATOR_MODE_AUTO_PWM_PSM_WO_ECO,
	M2296_REGULATOR_MODE_MAX,
};


enum m2296_regulator {
	/*
	 * 0x12 0x18 b3~b0 VOLDO1 [3:0] /VOLDO1_SLP[3:0]
	 * Setting the output voltage of LDO1 in
	 * Normal/Sleep mode, Default=4’b0111
	 * VOLDO1[3:0]] LDO1 VOLDO1[3:0]] LDO1 VOLDO1[3:0]]
	 * LDO1 VOLDO1[3:0]] LDO1
	 * 1111 1.100V 1011 1.000V 0111 0.900V 0011 0.800V
	 * 1110 1.075V 1010 0.975V 0110 0.875V 0010 0.775V
	 * 1101 1.050V 1001 0.950V 0101 0.850V 0001 0.750V
	 * 1100 1.025V 1000 0.925V 0100 0.825V 0000 0.725V
	 */
	M2296_PMIC_VOLDO1,
	M2296_PMIC_VOLDO2,
	M2296_PMIC_VOLDO3,
	M2296_PMIC_VOLDO4,
	M2296_PMIC_VOLDO5,
	M2296_PMIC_VOLDO6,
	M2296_PMIC_VOLDO7,
	M2296_PMIC_VOLDO8,
	M2296_PMIC_VOLDO9,
	M2296_PMIC_VOLDO10,
	/*M2296_PMIC_RTCOUT,*/
	M2296_PMIC_DCDC1,
	M2296_PMIC_DCDC2,
	M2296_PMIC_DCDC3,
	M2296_PMIC_DCDC4,
	M2296_NUM_REGULATORS,
};
////m2296_ldo1
#define m2296_ldo1_id  M2296_PMIC_VOLDO1
#define m2296_ldo1_n_voltages 0x0f/* 0x0000 ~ 0x1111 */
#define m2296_ldo1_linear_min_sel (0)
#define m2296_ldo1_min_uV	(725000)
#define m2296_ldo1_uV_step  (25000)
#define m2296_ldo1_vsel_reg  M2296_REG_LDO1LDO2_VOLT
#define m2296_ldo1_vsel_mask (0x0f)
/**0x21-ONLDO8 ONLDO7 ONLDO6 ONLDO5 ONLDO4 ONLDO3 ONLDO2 ONLDO1**/
#define m2296_ldo1_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo1_enable_mask 0x01

////m2296_ldo2
#define m2296_ldo2_id  M2296_PMIC_VOLDO2
#define m2296_ldo2_n_voltages 0x0f
#define m2296_ldo2_linear_min_sel (0)
#define m2296_ldo2_min_uV	(750000)/**0.75V **/
#define m2296_ldo2_uV_step  (50000)/**0.05v**/
#define m2296_ldo2_vsel_reg  M2296_REG_LDO1LDO2_VOLT
#define m2296_ldo2_vsel_mask (0xf0)
/**ONLDO2 - 10 **/
#define m2296_ldo2_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo2_enable_mask 0x02

////m2296_ldo3
#define m2296_ldo3_id  M2296_PMIC_VOLDO3
#define m2296_ldo3_n_voltages 0x0f
#define m2296_ldo3_linear_min_sel (0)
#define m2296_ldo3_min_uV	(750000)
#define m2296_ldo3_uV_step  (50000)/* 0.05v * 1000 * 1000 */
#define m2296_ldo3_vsel_reg  M2296_REG_LDO3LDO4_VOLT
#define m2296_ldo3_vsel_mask (0x0f)
/**ONLDO3 - 100 **/
#define m2296_ldo3_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo3_enable_mask 0x04

////m2296_ldo4
#define m2296_ldo4_id  M2296_PMIC_VOLDO4
#define m2296_ldo4_n_voltages 0x0f
#define m2296_ldo4_linear_min_sel (0)
#define m2296_ldo4_min_uV	(1400000)/*1.40V*/
#define m2296_ldo4_uV_step  (100000)/*0.1v*/
#define m2296_ldo4_vsel_reg  M2296_REG_LDO3LDO4_VOLT
/** b4 ~ b7 ** 11110000 */
#define m2296_ldo4_vsel_mask (0xf0)
/**ONLDO4 - 1000 **/
#define m2296_ldo4_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo4_enable_mask 0x08

////m2296_ldo5 this only four voltage..
#define m2296_ldo5_id  M2296_PMIC_VOLDO5
#define m2296_ldo5_n_voltages 0x02
#define m2296_ldo5_linear_min_sel (0)
#define m2296_ldo5_min_uV	(1800000)/*1.80V*/
#define m2296_ldo5_uV_step  (500000)
#define m2296_ldo5_vsel_reg  M2296_REG_LDO5LDO6_VOLT
/** b1 ~ b0 ** 11 */
#define m2296_ldo5_vsel_mask (0x03)
/**ONLDO5 - 1 0000 **/
#define m2296_ldo5_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo5_enable_mask 0x10

///m2296_ldo6
#define m2296_ldo6_id  M2296_PMIC_VOLDO6
#define m2296_ldo6_n_voltages 0x0f
#define m2296_ldo6_linear_min_sel (0)
#define m2296_ldo6_min_uV	(1400000)/*1.40V*/
#define m2296_ldo6_uV_step  (100000)/*0.1v*/
#define m2296_ldo6_vsel_reg  M2296_REG_LDO5LDO6_VOLT
/** b4 ~ b7 ** 11110000 */
#define m2296_ldo6_vsel_mask (0xf0)
/**ONLDO6 - 10  0000 **/
#define m2296_ldo6_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo6_enable_mask 0x20

///m2296_ldo7
#define m2296_ldo7_id  M2296_PMIC_VOLDO7
#define m2296_ldo7_n_voltages 0x0f
#define m2296_ldo7_linear_min_sel (0)
#define m2296_ldo7_min_uV	(1400000)/*1.40V*/
#define m2296_ldo7_uV_step  (100000)/*0.1v*/
#define m2296_ldo7_vsel_reg  M2296_REG_LDO7LDO8_VOLT
/** b3~b0 ** 0000 1111 */
#define m2296_ldo7_vsel_mask (0x0f)
/**ONLDO7 - 100  0000 **/
#define m2296_ldo7_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo7_enable_mask 0x40

///m2296_ldo8
#define m2296_ldo8_id  M2296_PMIC_VOLDO8
#define m2296_ldo8_n_voltages 0x0f
#define m2296_ldo8_linear_min_sel (0)
#define m2296_ldo8_min_uV	(1400000)/*1.40V*/
#define m2296_ldo8_uV_step  (100000)/*0.1v*/
#define m2296_ldo8_vsel_reg  M2296_REG_LDO7LDO8_VOLT
/** b7~b4 **  1111 0000 */
#define m2296_ldo8_vsel_mask (0xf0)
/**ONLDO8 - 1000  0000 **/
#define m2296_ldo8_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_ldo8_enable_mask 0x80

///m2296_ldo9
#define m2296_ldo9_id  M2296_PMIC_VOLDO9
#define m2296_ldo9_n_voltages 0x0f
#define m2296_ldo9_linear_min_sel (0)
#define m2296_ldo9_min_uV	(725000)/*0.725VV*/
#define m2296_ldo9_uV_step  (25000)/*0.025v*/
#define m2296_ldo9_vsel_reg  M2296_REG_LDO9_VOLT
/** b7~b4 **  1111 0000 */
#define m2296_ldo9_vsel_mask (0xf0)
/**ONLDO9 - 0000 0010 **/
#define m2296_ldo9_enable_reg  M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_ldo9_enable_mask 0x02

///m2296_ldo10
#define m2296_ldo10_id  M2296_PMIC_VOLDO10
#define m2296_ldo10_n_voltages 0x0f
#define m2296_ldo10_linear_min_sel (0)
#define m2296_ldo10_min_uV	(725000)/*0.725V*/
#define m2296_ldo10_uV_step  (25000)/*0.025v*/
#define m2296_ldo10_vsel_reg  M2296_REG_LDO10_RTCLDO_VOLT
/** b3~b0 **   00001111 */
#define m2296_ldo10_vsel_mask (0x0f)
/**ONLDO10 - 0000  0100 **/
#define m2296_ldo10_enable_reg  M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_ldo10_enable_mask 0x04

//RTCOUT
#define m2296_rtcout_id  M2296_PMIC_RTCOUT
#define m2296_rtcout_n_voltages 0x02
#define m2296_rtcout_linear_min_sel (0)
#define m2296_rtcout_min_uV	(1800000)/*1.80V*/
#define m2296_rtcout_uV_step  (500000)
#define m2296_rtcout_vsel_reg  M2296_REG_LDO10_RTCLDO_VOLT
/** b5~ b4** 11 0000 */
#define m2296_rtcout_vsel_mask (0x30)
/**ONLDO5 - 1 0000 **/
#define m2296_rtcout_enable_reg  M2296_REG_LDO1_LDO8_ONOFF_CONTROL
#define m2296_rtcout_enable_mask 0x10

/*
 * Setting the feedback reference voltage of
 * DCDC1 in Normal/Sleep mode, VO1=VFB1/0.8,
 * Default=6’b010010.
 * VFB1=0.54V + 10mV x Value, VO1=0.675V + 12.5mV x Value,
 * where Value is 6-bit binary code
 * FBDC1[5:0]
 * ≧101111 1.2500V 011111  1.0625V 001111  0.8625V
 * 101110 1.2500V 011110 1.0500V 001110 0.8500V
 * 101101 1.2375V 011101 1.0375V 001101 0.8375V
 * 101100 1.2250V 011100 1.0250V 001100 0.8250V
 * 101011 1.2125V 011011 1.0125V 001011 0.8125V
 * 101010 1.2000V 011010 1.0000V 001010 0.8000V
 * 101001 1.1875V 011001 0.9875V 001001 0.7875V
 * 101000 1.1750V 011000 0.9750V 001000 0.7750V
 * 100111 1.1625V 010111 0.9625V 000111 0.7625V
 * 100110 1.1500V 010110 0.9500V 000110 0.7500V
 * 100101 1.1375V 010101 0.9375V 000101 0.7375V
 * 100100 1.1250V 010100 0.9250V 000100 0.7250V
 * 100011 1.1125V 010011 0.9125V 000011 0.7125V
 * 100010 1.1000V 010010 0.9000V 000010 0.7000V
 * 100001 1.0875V 010001 0.8875V 000001 0.6875V
 * 100000 1.0750V 010000 0.8750V 000000 0.6750V
 */
///m2296_DCDC1--ov1
#define m2296_dcdc1_id  M2296_PMIC_DCDC1
#define m2296_dcdc1_n_voltages 0x3f
#define m2296_dcdc1_linear_min_sel (0)
#define m2296_dcdc1_min_uV	(675000)/*0.6750V */
#define m2296_dcdc1_uV_step  (12500)/*0.0125v*/
#define m2296_dcdc1_vsel_reg  M2296_REG_DCDC1_VOLT
/**b5~b0 ** 0011 1111 */
#define m2296_dcdc1_vsel_mask (0x3f)
/** ONDC1- 0001  0000 **/
#define m2296_dcdc1_enable_reg  M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_dcdc1_enable_mask 0x10

/*
 * Setting the feedback reference voltage of DCDC2, VO2=VFB2 x (5 / 3),
 * Default=5’b01110
 * VFB2=0.51V + 15mV x Value, VO2=0.85V + 25mV x Value,
 * where Value is 5-bit binary code
 * FBDC2[4:0] VO2
 * 11111 1.600V 01111 1.225V
 * 11110 1.600V 01110 1.200V
 * 11101 1.575V 01101 1.175V
 * 11100 1.550V 01100 1.150V
 * 11011 1.525V 01011 1.125V
 * 11010 1.500V 01010 1.100V
 * 11001 1.475V 01001 1.075V
 * 11000 1.450V 01000 1.050V
 * 10111 1.425V 00111 1.025V
 * 10110 1.400V 00110 1.000V
 * 10101 1.375V 00101 0.975V
 * 10100 1.350V 00100 0.950V
 * 10011 1.325V 00011 0.925V
 * 10010 1.300V 00010 0.900V
 * 10001 1.275V 00001 0.875V
 * 10000 1.250V 00000 0.850V
 */
///m2296_DCDC2--ov2
#define m2296_dcdc2_id  M2296_PMIC_DCDC2
#define m2296_dcdc2_n_voltages 0x1f
#define m2296_dcdc2_linear_min_sel (0)
#define m2296_dcdc2_min_uV	(850000)/*0.850V*/
#define m2296_dcdc2_uV_step  (25000)/*0.025v*/
#define m2296_dcdc2_vsel_reg  M2296_REG_DCDC2_VOLT
/*b4~b0 ** 0001 1111 */
#define m2296_dcdc2_vsel_mask (0x1f)
/** ONDC2- 0010  0000 **/
#define m2296_dcdc2_enable_reg  M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_dcdc2_enable_mask 0x20

///m2296_DCDC3
#define m2296_dcdc3_id			M2296_PMIC_DCDC3
#define m2296_dcdc3_fixed_uV		3300000/*3.3v*/
/*ONDC3 0100 0000*/
#define m2296_dcdc3_enable_reg		M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_dcdc3_enable_mask		0x40

///m2296_DCDC4
#define m2296_dcdc4_id			M2296_PMIC_DCDC4
#define m2296_dcdc4_fixed_uV		3300000/*3.3v*/
/*ONDC4 1000 0000*/
#define m2296_dcdc4_enable_reg		M2296_REG_LDO9_DC4_ONOFF_CONTROL
#define m2296_dcdc4_enable_mask		0x80

struct m2296_regulator_drvdata {
	struct regulator_desc rdesc;
};

struct m2296_regulator_data {
	int id;
	struct regulator_init_data *initdata;
	struct device_node *reg_node;
};

struct m2296_chip {
	struct i2c_client *client;
	struct device *dev;
	struct mutex rw_lock;
	u8 dev_rev;
	u8 irq;
	int powkeycode;
	bool bpwron_it_release;
	struct regmap *regmap;
	struct regmap_irq_chip_data *reg_irq_data;
	struct input_dev *input_dev;
	struct delayed_work m2296_regmap_pmic_power_delay_work;
	struct wakeup_source *pwrkey_lock;
};

struct m2296_irq_mapping_table {
	const char *name;
	int (*hdlr)(struct m2296_chip *chip);
	int num;
};

#define M2296_IRQ_MAPPING(_name, _num) \
	{.name = #_name, .hdlr = m2296_##_name##_irq_handler, .num = _num}

int m2296_chip_i2c_read_byte(struct m2296_chip *chip, u8 cmd, u8 *data);
int m2296_chip_i2c_write_byte(struct m2296_chip *chip, u8 cmd, u8 data);
int m2296_update(struct m2296_chip *chip,
			unsigned int reg, unsigned int mask,
			unsigned int data);
int m2296_i2c_chip_close(struct m2296_chip *chip);

#endif /* __LINUX_MFD_M2296_I2C_H__ */

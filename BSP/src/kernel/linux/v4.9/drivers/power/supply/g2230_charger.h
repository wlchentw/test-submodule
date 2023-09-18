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

#ifndef _G2230_CHARGER_H
#define _G2230_CHARGER_H

/** I2C Register Map **/
#define G2230_CHIP_REG_A0 0x00
#define G2230_CHIP_REG_A1 0x01
#define G2230_CHIP_REG_A2 0x02
#define G2230_CHIP_REG_A3 0x03
#define G2230_CHIP_REG_A4 0x04
#define G2230_CHIP_REG_A5 0x05
#define G2230_CHIP_REG_A6 0x06
#define G2230_CHIP_REG_A7 0x07
#define G2230_CHIP_REG_A8 0x08
#define G2230_CHIP_REG_A9 0x09
#define G2230_CHIP_REG_A10_INT1_STATUS 0x0A
#define G2230_CHIP_REG_A11_INT1_MASK 0x0B
#define G2230_CHIP_REG_A12_INT2_STATUS 0x0C
#define G2230_CHIP_REG_A13_INT2_MASK 0x0D
#define G2230_CHIP_REG_A14 0x0E
#define G2230_CHIP_REG_A15 0x0F
#define G2230_CHIP_REG_A16 0x10
#define G2230_CHIP_REG_A17_VER 0x11
#define G2230_CHIP_MAX_REG G2230_CHIP_REG_A17_VER

#define G2230_CHIP_ID  0x06

/** bit0 mask --right shift to bit0 **/
#define BIT0_MASK (0x01)

/*** A0 --bit description **/
#define G2230_A0_CHGRST  BIT(7)
#define G2230_A0_VP_ON     BIT(5)
#define G2230_A0_ENSHIP   BIT(4)
#define G2230_A0_ENOTGI2C BIT(3)
#define G2230_A0_ENCHIN  BIT(2)
#define G2230_A0_ENPWRBNK  BIT(1)
#define G2230_A0_INSUS   BIT(0)

/**A0 b7 CHGRST
 * Reset condition of the registers Group B when BAT goes under BATDET(3.4V).
 **/
#define A0_CHGRST_ENABLE (0x01 << 7)
#define A0_CHGRST_DISABLE (A0_CHGRST_ENABLE & ~(A0_CHGRST_ENABLE))

/**A0 b5 VP_ON
 * VP_ON is used to tell G2230 that PMU is enabled. The bandgap and VP circuits
 * will be turned on and the NTC-R temperature monitor will be activated.
 * VP_ON <--> PMU status
 * 0 OFF
 * 1 ON
 **/
#define A0_VP_ON_PMU_ON (0x01 << 5)
#define A0_VP_ON_PMU_OFF (A0_VP_ON_PMU_ON & (~A0_VP_ON_PMU_ON))

/** A0 b4 ENSHIP
 * ENSHIP is used to enable shipping mode.
 * G2230 can enter shipping mode when DCIN does not exist and ENSHIP=1
 **/
#define A0_ENSHIP_ENABLE (0x01 << 4)

/**A0 b3 ENOTGI2C
 * ENOTGI2C is used to set the switching charger operating in OTG mode,
 * and delivering power to DCIN from BAT.
 * ENOTGI2C would over-ride Charge Enable Function in /ENCHIN.
 * ENOTGI2C <--> OTG Mode
 * 0 NO
 * 1 YES
 **/
#define A0_ENOTGI2C_OTG_ENABLE (0x01 << 3)
#define A0_ENOTGI2C_OTG_DISENABLE	\
			(A0_ENOTGI2C_OTG_ENABLE & (~A0_ENOTGI2C_OTG_ENABLE))

/**A0 b2 /ENCHIN
 * Charge Enable Signal.
 * /ENCHIN <--> Charge Status
 * 0 Enbale
 * 1 Disable
 **/
#define A0_ENCHIN_SHIFT_NUM (0x02)
/** right 2-bit **/
#define A0_ENCHIN_DISABLE (0x01)
#define A0_ENCHIN_ENABLE (A0_ENCHIN_DISABLE & (~A0_ENCHIN_DISABLE))

/**A0 b1 ENPWRBNK
 * ENPWRBNK is used to set the switching charger operating in power bank mode,
 * and delivering power to PMID from BAT. The Q1 MOS (DCIN-PMID) is off.
 * ENPWRBNK <--> Power Bank Mode
 * 0 NO
 * 1 YES
 **/
#define A0_ENPWRBNK_ENABLED (0x01 << 1)
#define A0_ENPWRBNK_DISABLED (A0_ENPWRBNK_ENABLED & (~A0_ENPWRBNK_ENABLED))

/**A0 b0 INSUS
 * DCIN Suspend Control Input
 * INSUS <--> DCIN Status
 * 0 No Suspend
 * 1 Suspend
 **/
#define A0_INSUS_SUSPEND (0x01 << 0)
#define A0_INSUS_NO_SUSPEND (0x00 << 0)

/***01h A1 **/
#define G2230_A1_ENDPM BIT(7)
#define G2230_A1_VINDPM (0x0f << 3)
#define G2230_A1_ISET_DCIN 0x07

/** A1 b7 ENDPM
 * Enable DCIN DPM function
 * ENDPM DCIN DPM function
 * 0 Disable
 * 1 Enable
 **/
#define A1_ENDPM_ENABLE (0x01 << 7)
#define A1_ENDPM_DISABLE (A1_ENDPM_ENABLE & (~A1_ENDPM_ENABLE))

/**A1 b6~b3 VINDPM<3:0>
 * Setting DCIN DPM voltage. VDCIN_DPM = 3.88V + VINDPM<3:0>*80mV
 * 0000 3.88V 0100 4.2V 1000 4.52V 1100 4.84V
 * 0001 3.96V 0101 4.28V 1001 4.6V 1101 4.92V
 * 0010 4.04V 0110 4.36V 1010 4.68V 1110 5.0V
 * 0011 4.12V 0111 4.44V 1011 4.76V 1111 5.08V
 **/
#define A1_VINDPM_MIN_VOL (3880000)
#define A1_VINDPM_STEP_VOL (80000)/*0.08v**/
#define A1_VINDPM_MAX_VOL (5080000)

/***A1 b2~b0 ISET_DCIN<2:0>
 * Setting DCIN Current Limit Threshold.
 * The actual input current limit is the lower of I2C and setting of ILIM pin.
 * ISET_DCIN<2:0> <--> DCIN Current limit
 * 000 150mA 100 1.5A
 * 001 450mA 101 2A
 * 010 850mA 110 2.5A
 **/
#define A1_ISET_DCIN_150mA (0x00)
#define A1_ISET_DCIN_450mA (0x01)
#define A1_ISET_DCIN_850mA (0x02)
#define A1_ISET_DCIN_1500mA (0x04)
#define A1_ISET_DCIN_2000mA (0x05)
#define A1_ISET_DCIN_2500mA (0x06)
#define A1_ISET_DCIN_3000mA (0x07)

/**02h A2 **/
#define G2230_A2_VSYSMIN (0x07 << 5)
#define G2230_A2_VS2B (0x03 << 3)
#define G2230_A2_VBLOW 0x03

/**A2 b7~b5 VSYS_MIN <2:0>
 * Minimum System Voltage Limit
 * VSYS_MIN<2:0> <--> System Voltage limit
 * 000 3.1V 100 3.5V
 * 001 3.2V 101 3.6V
 * 010 3.3V 110 3.7V
 * 011 3.4V 111 3.8V
 **/
#define A2_VSYSMIN_31V (0x00 << 5)
#define A2_VSYSMIN_32V (0x01 << 5)
#define A2_VSYSMIN_33V (0x02 << 5)
#define A2_VSYSMIN_34V (0x03 << 5)
#define A2_VSYSMIN_35V (0x04 << 5)
#define A2_VSYSMIN_36V (0x05 << 5)
#define A2_VSYSMIN_37V (0x06 << 5)
#define A2_VSYSMIN_38V (0x07 << 5)

/**A2 b4~b3 VS2B<1:0>
 * VS2B<1:0> is used to set the SYS to BAT voltage.
 * (VSYS=VBAT+VS2B ) when (VBAT+VS2B >VSYSMIN.)
 * VS2B<1:0> VS2B
 * 00 60mV
 * 01 120mV
 * 10 180mV
 * 11 240mV
 **/
#define A2_VS2B_60mV (0x00 << 3)
#define A2_VS2B_120mV (0x01 << 3)
#define A2_VS2B_180mV (0x02 << 3)
#define A2_VS2B_240mV (0x03 << 3)

/**A2 b1~b0 VBLOW<1:0>
 * Setting the BATLOW threshold.
 * VBLOW<1:0> BATLOW
 * 00 3.2
 * 01 3.3
 * 10 3.4
 * 11 3.5
 **/
#define A2_VBLOW_32V (0x00)
#define A2_VBLOW_33V (0x01)
#define A2_VBLOW_34V (0x02)
#define A2_VBLOW_35V (0x03)

#define G2230_CHIP_VFASTCHG_MIN  (2900000)/**2.9v**/
#define G2230_CHIP_VFASTCHG_MAX	 (3100000)/** 3.1 v**/

/** 03h A3 **/
#define G2230_A3_ISETA (0x3f << 2)
#define G2230_A3_ICHGDONE  (0x01 << 0)

/**A3 b7~b2 ISETA<5:0>
 * SETA<5:0> is used to set the battery charge current level.
 * The termination current limit is 10% of the programmed value.
 * ICHG=512mA+ISETA<5:0>*64mA, (512~4544mA), Default=2048mA (011000)
 * ITERM=10%*ICHG
 **/
#define G2230_ISETA_MASK_ICHG 0xfc
#define G2230_ISETA_SHIFT_ICHG 0x02

/**uA **/
#define G2230_ISETA_ICHG_MIN	(512000)
#define G2230_ISETA_ICHG_MAX	(4544000)
#define G2230_ISETA_ICHG_STEP	(64000)
#define A3_ISETA_MIN G2230_ISETA_ICHG_MIN
#define A3_ISETA_MAX G2230_ISETA_ICHG_MAX
#define A3_ISETA_STEP G2230_ISETA_ICHG_STEP

/**A3 b0 ICHGDONE
 * Setting the termination current ratio.
 * ICHGDONE Termination Current Ratio
 * 0 5%
 * 1 10%
 **/
#define A3_ICHGDONE_5 0x00
#define A3_ICHGDONE_10 0x01

/**04h A4 **/
#define G2230_A4_IPRECHG (0x0f << 4)
#define G2230_A4_VSETA (0x07 << 1)
#define G2230_A4_VSETA_MAX (0x07)
#define G2230_A4_VSETA_SHFT (1)
#define G2230_A4_VBATFC  0x01

/** A4 b7~b4 IPRECHG<3:0>
 * Setting the pre-charge current limit. IPRECHG = 128mA + IPRECHG<3:0>*128mA
 * 0000 128mA 0100 640mA 1000 1152mA 1100 1664mA
 * 0001 256mA 0101 768mA 1001 1280mA 1101 1792mA
 * 0010 384mA 0110 896mA 1010 1408mA 1110 1920mA
 * 0011 512mA 0111 1024mA 1011 1536mA 1111 2048mA
 **/
/**uA **/
#define A4_IPRECHG_MIN (128000)
#define A4_IPRECHG_MAX (2048000)
#define A4_IPRECHG_STEP (128000)

/**A4 b3~b1 VSETA<2:0>
 * VSETA<2:0> is used to set the battery charge voltage level.
 * 000 4.1V 100 4.3V
 * 001 4.15V 101 4.35V
 * 010 4.2V 110 4.4V
 * 011 4.25V 111 4.45V
 **/
#define A4_VSETA_MIN (4100000)
#define A4_VSETA_MAX (4450000)
#define A4_VSETA_STEP (50000)
#define A4_VSETA_4_1V (0x00 << 1)
#define A4_VSETA_4_15V (0x01 << 1)
#define A4_VSETA_4_2V (0x02 << 1)
#define A4_VSETA_4_25V (0x03 << 1)
#define A4_VSETA_4_3V (0x04 << 1)
#define A4_VSETA_4_35V (0x05 << 1)
#define A4_VSETA_4_4V (0x06 << 1)
#define A4_VSETA_4_45V (0x07 << 1)

/**A4 b0 VBATFC
 * Setting the battery voltage threshold from pre-charge to fast charge.
 * BATLOWV <--> Pre-charge to Fast charge threshold
 * 0 2.8V
 * 1 3.0V**/
#define A4_VBATFC_28V (0x00)
#define A4_VBATFC_30V (0x01)

/** 05h A5 **/
#define G2230_A5_EN_TIMER BIT(7)
#define G2230_A5_TIMER (0x03 << 5)
#define G2230_A5_TMR2X_EN BIT(4)
#define G2230_A5_CHGSTEN BIT(3)
#define G2230_A5_CVCOMP (0x07)

/** A5 b7 EN_TIMER
 * Enable timers.
 * EN_TIMER <--> Timers
 * 0 Disable
 * 1 Enable
 **/
#define A5_EN_TIMER_ENABLE (0x01 << 7)
#define A5_EN_TIMER_DISABLE (A5_EN_TIMER_ENABLE & (~A5_EN_TIMER_ENABLE))

/** A5 b6~b5
 * TIMER <1:0>
 * Setting Charger Safety Timer.
 * Fast charge safety time TFASTCHG=TSAFE.
 * Pre-Charge safety time TPRECHG=TSAFE/8.
 * (2X during VINDPM, IINDPM, JEITA, Thermal regulation)
 * TIMER<1:0> <--> TSAFE
 * 00 5 hrs.
 * 01 8 hrs.
 * 10 12 hrs.
 * 11 20 hrs
 **/
#define A5_TIMER_5HRS (0x00 << 5)
#define A5_TIMER_8HRS (0x01 << 5)
#define A5_TIMER_12HRS (0x02 << 5)
#define A5_TIMER_20HRS (0x03 << 5)

/** A5 b4 TMR2X_EN
 * Enable 2X extended safety timer during DPM and thermal regulation.
 * TMR2X_EN <--> 2X TSAFE
 * 0 Disable
 * 1 Enable
 **/
#define A5_TMR2X_EN_ENABLE (0x01 << 4)
#define A5_TMR2X_EN_DISABLE (A5_TMR2X_EN_ENABLE & (~A5_TMR2X_EN_ENABLE))

/**A5 b3 CHGSTEN
 * CHGSTEN is used to set indication method of /CHGLED PIN.
 **/
#define A5_CHGSTEN_HIGH (0x01 << 3)
#define A5_CHGSTEN_LOW (A5_CHGSTEN_HIGH & (~A5_CHGSTEN_HIGH))

/**A5 b2~b0 CVCOMP<2:0>
 * CVCOMP<2:0> is used to set the battery charge voltage compensation level.
 * BATTARGET=BATREG+BATCOMP
 * 000 0mV 100 20mV
 * 001 5mV 101 25mV
 * 010 10mV 110 30mV
 * 011 15mV 111 40mV
 **/
#define A5_CVCOMP_0mV 0x00
#define A5_CVCOMP_5mV 0x01
#define A5_CVCOMP_10mV 0x02
#define A5_CVCOMP_15mV 0x03
#define A5_CVCOMP_20mV 0x04
#define A5_CVCOMP_25mV 0x05
#define A5_CVCOMP_30mV 0x06
#define A5_CVCOMP_40mV 0x07

/**06h A6 **/
#define G2230_A7_OTGV (0x0f << 4)
#define G2230_A7_I_OTG BIT(3)
#define G2230_A7_ENTS_BOOST BIT(2)

/**A6 b7~b4 OTGV<3:0>
 * Setting the OTG voltage. VOTG = 4.55V + OTGV<3:0>*64mV
 * 0000 4.55V 0100 4.806V 1000 5.062V 1100 5.318V
 * 0001 4.614V 0101 4.87V 1001 5.126V 1101 5.382V
 * 0010 4.678V 0110 4.934V 1010 5.19V 1110 5.446V
 * 0011 4.742V 0111 4.998V 1011 5.254V 1111 5.51V
 **/
#define A7_OTGV_MIN (4550000)
#define A7_OTGV_MAX (5510000)
#define A7_OTGV_STEP (640000)

/**A6 b3 I_OTG
 * Setting the output current in OTG mode.
 * I_OTG OTG Output Current
 * 0 1.0A
 * 1 1.5A
 **/
#define A7_I_OTG_1_5A (0x01 << 3)
#define A7_I_OTG_1A (A7_I_OTG_1_5A & (~A7_I_OTG_1_5A))

/**A6 b2 ENTS_BOOST
 * Enable boost mode thermal protection function.
 * ENTS_BOOST Boost mode thermal protection
 * 0 Disable
 * 1 Enbale
 **/
#define A7_ENTS_BOOST_ENABLE (0x01 << 2)
#define A7_ENTS_BOOST_DISABLE (A7_ENTS_BOOST_ENABLE & (~A7_ENTS_BOOST_ENABLE))

/** 07h A7 **/
#define G2230_A7_ENTHR BIT(7)
#define G2230_A7_THRREG (0x03 << 5)/*(BIT(6)|BIT(5))*/
#define G2230_A7_CHG_2DET BIT(4)
#define G2230_A7_CHG_1DET BIT(3)
#define G2230_A7_EN_ILIMADJ BIT(2)
#define G2230_A7_ENJEITA BIT(1)
#define G2230_A7_ICHG50PCT BIT(0)

#define G2230_A7_CHG_1DET_2DET (G2230_A7_CHG_1DET | G2230_A7_CHG_2DET)
/**
 * A7 b7 /ENTHR
 * Enable thermal regulation function.
 * 0 Enable
 * 1 Disable
 **/
#define A7_ENTHR_DISABLE (0x01 << 7)
#define A7_ENTHR_ENABLE (A7_ENTHR_DISABLE & (~A7_ENTHR_DISABLE))

/**A7 b6~b5 THRREG
 * Thermal Regulation Threshold
 * 00 60oC
 * 01 80oC
 * 10 100oC
 * 11 120oC
 **/
#define A7_THRREG_60oC (0x00 << 5)
#define A7_THRREG_80oC (0x01 << 5)
#define A7_THRREG_100oC (0x02 << 5)
#define A7_THRREG_120oC (0x03 << 5)

/**A7 b4 CHG_2DET
 * CHG_2DET is used to enable the secondary detection of charger type.
 * 0 Disable
 * 1 Enable
 **/
 #define A7_CHG_2DET_SHIFT_NUM  (0x04)
#define A7_CHG_2DET_ENABLE (0x01 << A7_CHG_2DET_SHIFT_NUM)
#define A7_CHG_2DET_DISABLE (A7_CHG_2DET_ENABLE & (~A7_CHG_2DET_ENABLE))

/**A7 b3 CHG_1DET
 * CHG_1DET is used to enable the primary detection of charger type.
 * 0 Disable
 * 1 Enable
 **/
#define A7_CHG_1DET_SHIFT_NUM  (0x03)
#define A7_CHG_1DET_ENABLE (0x01 << A7_CHG_1DET_SHIFT_NUM)
#define A7_CHG_1DET_DISABLE (A7_CHG_1DET_ENABLE & (~A7_CHG_1DET_ENABLE))

#define A7_CHG_1DET_2DET_ENABLE (A7_CHG_1DET_ENABLE | A7_CHG_2DET_ENABLE)
#define A7_CHG_1DET_2DET_DISABLE (A7_CHG_1DET_DISABLE | A7_CHG_2DET_DISABLE)

/**A7 b2 EN_ILIMADJ
 * EN_ILIMADJ is used to enable the function which automatically adjusts
 * A1.ISET_DCIN value according to the USB detection result.
 * When EN_ILIMADJ=1’b1, A1. ISET_DCIN will be set to corresponding
 * value after USB detection completed.
 * 0 Keep value.
 * 1 Adjusted to corresponding value automatically.
 **/
#define A7_EN_ILIMADJ_AUTO_ADJUST (0x01 << 2)
#define A7_EN_ILIMADJ_KEEP	\
	(A7_EN_ILIMADJ_AUTO_ADJUST & (~A7_EN_ILIMADJ_AUTO_ADJUST))

/**A7 b1 ENJEITA
 * ENJEITA is used to enable the JEITA function.
 * When ENJEITA=1’b1, A8.TSHT<1:0>, A8.TSWM<1:0>, A8.TSCL<1:0>, A8.TSCD<1:0>
 * are  set to 8’b1110,1001 and cannot be changed by user.
 * ENJEITA Charging condition
 **/
#define A7_ENJEITA_ENABLE (0x01 << 1)
#define A7_ENJEITA_DISABLE (A7_ENJEITA_ENABLE & (~A7_ENJEITA_ENABLE))

/**A7 b0 ICHG50PCT
 * Setting the battery charge current level to 50% of full scale.
 * ICHG50PC <--> T ICHG
 * 0 ICHG=A3<7:2> programmed
 * 1 ICHG=50%*A3<7:2> programmed
 **/
#define A7_ICHG50PCT_HAL_ISETA  (0x01 << 0)
#define A7_ICHG50PCT_FULL_ISETA		\
			(A7_ICHG50PCT_HAL_ISETA & (~A7_ICHG50PCT_HAL_ISETA))

 /***08h A8 **/
#define G2230_A8_TSCD (0x03 << 6)
#define G2230_A8_TSCL (0x03 << 4)
#define G2230_A8_TSWM (0x03 << 2)
#define G2230_A8_TSHT (0x03 << 0)

/**
 * A8 b7~b6 TSCD<1:0>
 * Set TS/VP ratio to monitor battery pack temperature for COLD boundary when
 * A7.ENJEITA=1’b0. When A7.ENJEITA=1’b1, TSCD<1:0> is set to 2’b11. **/
 /** Battery Pack Temp **/
#define A8_TSCD_BATTERY_PACK_TEMP_NEGATIVE_15C (0x00 << 6)
#define A8_TSCD_BATTERY_PACK_TEMP_NEGATIVE_10C (0x01 << 6)
#define A8_TSCD_BATTERY_PACK_TEMP_NEGATIVE_5C (0x02 << 6)
#define A8_TSCD_BATTERY_PACK_TEMP_0C (0x03 << 6)


/**A8 b5~b4 TSCL<1:0>
 * Set TS/VP ratio to monitor battery pack temperature for COOL boundary when
 * A7.ENJEITA=1’b0. When A7.ENJEITA=1’b1, TSCD<1:0> is set to 2’b10.
 **/
#define A8_TSCL_BATTERY_PACK_TEMP_0C (0x00 << 4)
#define A8_TSCL_BATTERY_PACK_TEMP_5C (0x01 << 4)
#define A8_TSCL_BATTERY_PACK_TEMP_10C (0x02 << 4)
#define A8_TSCL_BATTERY_PACK_TEMP_15C (0x03 << 4)


/**A8 b3~b2 TSWM<1:0>
 * Set TS/VP ratio to monitor battery pack temperature for WARM boundary when
 * A7.ENJEITA=1’b0. When A7.ENJEITA=1’b1, TSCD<1:0> is set to 2’b10.
 **/
#define A8_TSWM_BATTERY_PACK_TEMP_35C (0x00 << 2)
#define A8_TSWM_BATTERY_PACK_TEMP_40C (0x01 << 2)
#define A8_TSWM_BATTERY_PACK_TEMP_45C (0x02 << 2)
#define A8_TSWM_BATTERY_PACK_TEMP_50C (0x03 << 2)

/**A8 b1~b0 TSHT<1:0>
 * Set TS/VP ratio to monitor battery pack temperature for HOT boundary when
 * A7.ENJEITA=1’b0. When A7.ENJEITA=1’b1, TSCD<1:0> is set to 2’b01.
 */
#define A8_TSHT_BATTERY_PACK_TEMP_55C (0x00 << 0)
#define A8_TSHT_BATTERY_PACK_TEMP_60C (0x01 << 0)
#define A8_TSHT_BATTERY_PACK_TEMP_65C (0x02 << 0)
#define A8_TSHT_BATTERY_PACK_TEMP_70C (0x03 << 0)

/** 09h A9 **/
#define G2230_A9_NTCDET (0x01 << 7)
#define G2230_A9_TSSEL (0x01 << 6)
#define G2230_A9_TIME_IT (0x03 << 4)
#define G2230_A9_TIME_LP (0x03 << 2)
#define G2230_A9_TDSHIPPING (0x03 << 0)

/** A9 b7 /NTCDET
 * /NTCDET bit is used to disable the auto battery NTC-R type detection.
 * The TS/VP threshold of Battery pack temperature monitor is auto adjusted
 * by the NTC-R type detection result if the /NTCDET bit is 1’b0. Otherwise,
 * the TS/VP threshold is decided by the register bit A9.TSSEL.
 * /NTCDET <--> Auto NTC-R Type Detection
 * 0 Enable
 * 1 Disable
 **/
#define A9_NTCDET_DISABLE (0x01 << 7)
#define A9_NTCDET_ENABLE (A9_NTCDET_DISABLE & (~A9_NTCDET_DISABLE))


/**A9 b6 TSSEL
 * TSSEL is used to choose the set the TS/VP ratio
 * when the register bit A9./NTCDET=1’b1.
 * TSSEL <--> TS/VP ratio
 * 0 10K set
 * 1 100K set
 **/
#define A9_TSSEL_100K (0x01 << 6)
#define A9_TSSEL_10K (A9_TSSEL_100K & (~A9_TSSEL_100K))

/***A9 b5~b4 TIME_IT <1:0>
 * TIME_IT<1:0> is used to defined the PWRON rising-edge de-bouncing delay time.
 * When PWRON is toggled high with duration longer than TdbPWRONF, the register
 * A14.PWRON_IT is 1’b1.
 * TIME_IT<1:0> <--> TdbPWRONF
 * 00 128mS
 * 01 1.0S
 * 10 1.5S
 * 11 2.0S
 **/
#define A9_TIME_IT_128MS (0x00 >> 4)
#define A9_TIME_IT_1S (0x01 >> 4)
#define A9_TIME_IT_1_5S (0x02 >> 4)
#define A9_TIME_IT_2S (0x03 >> 4)

/**A9 b3~b2 TIME_LP <1:0>
 * TIME_LP<1:0> is used to defined the PWRON long-press delay time TdPWRONLP.
 * When PWRON is toggled high with duration > TdPWRONLP,
 * the register A14.PWRON_LP is 1’b1.
 * TIME_LP<1:0> <--> TdPWRONLP
 * 00 1.0S
 * 01 1.5S
 * 10 2.0S
 * 11 2.5S
 **/
#define A9_TIME_IP_1S (0x00 >> 2)
#define A9_TIME_IP_1_5S (0x01 >> 2)
#define A9_TIME_IP_2S (0x02 >> 2)
#define A9_TIME_IP_2_5S (0x03 >> 2)

/**A9 b1~b0 TD_SHIPPING <1:0>
 * TD_SHIPPING<1:0> is used to defined the delay time TdSHIPPING.
 * Enter shipping TdSHIPPING delay after A0.ENSHIP=1’b1 and DCINPG is invalid.
 * TD_SHIPPING<1:0> <--> TdSHIPPING
 * 00 1.0S
 * 01 2.0S
 * 10 4.0S
 * 11 8.0S
 **/
#define A9_TD_SHIPPING_1S (0x00 >> 0)
#define A9_TD_SHIPPING_2S (0x01 >> 0)
#define A9_TD_SHIPPING_4S (0x02 >> 0)
#define A9_TD_SHIPPING_8S (0x03 >> 0)

/**0Ah A10 **/
#define G2230_A10_INT BIT(7)
#define G2230_A10_CHGRUN BIT(6)
#define G2230_A10_INLIMIT BIT(5)
#define G2230_A10_THR BIT(4)
#define G2230_A10_TS_METER (0x07 << 1)
#define G2230_A10_DCDET  BIT(0)

/**A10 b7 INT RW
 * INT is used to control the output status of /INT pin.
 * When interrupt events happen, /INT pin  goes low and the bit A10.INT
 * is set to 1’b1. After Micro-processor write the bit to be 1’b0,
 * /INT pin goes to high-Z state.
 * 0 Hi-Z
 * 1 Low
 **/
#define A10_INT_LOW (0x01 << 7)
#define A10_INT_HIGH (A10_INT_LOW & (~A10_INT_LOW))

/**A10 b6 CHGRUN R
 * CHGRUN is used to record the Auto Power Source Detection Status.
 * CHGRUN <--> APSD Status
 * 0 Not running
 * 1 Running
 **/
#define A10_CHGRUN_RUNNING (0x01 << 6)
#define A10_CHGRUN_NOTRUNNING (A10_CHGRUN_RUNNING & (~A10_CHGRUN_RUNNING))

/**A10 b5 INLIMIT R
 * INLIMIT is used to record DCIN Status.
 * 0 Normal
 * 1 DCIN DPM or DCIN current limit
 **/
#define A10_INLIMIT_DCIN_DPM (0x01 << 5)
#define A10_INLIMIT_NORMAL (A10_INLIMIT_DCIN_DPM & (~A10_INLIMIT_DCIN_DPM))

/**A10 b4 THR R
 * THR show the status of thermal regulation on charger
 * 0 NO
 * 1 YES
 **/
#define A10_THR_ENABLE (0x01 << 4)
#define A10_THR_DISABLE (A10_THR_ENABLE & (~A10_THR_ENABLE))

/**A10 b3~b1 TS_METER <2:0> R
 * Record the battery temperature by detecting the TS pin voltage.
 * TS_METER<2:0> <--> Battery Temp.
 * 110 T ≤ TCOLD
 * 100 TCOLD < T ≤ TCOOL
 * 000 TCOOL < T ≤ TWARM
 * 001 TWARM < T ≤ THOT
 * 011 THOT < T
 * 010 NTC not existed
 * 111 No battery installed
 **/
#define A10_TS_METER_COLD	(0x06 << 1)
#define A10_TS_METER_COOL	(0x04 << 1)
#define A10_TS_METER_NORM	(0x00 << 1)
#define A10_TS_METER_WARM	(0x01 << 1)
#define A10_TS_METER_HOT	(0x03 << 1)
#define A10_TS_METER_NOT_BATTERY (0x07 << 1)
#define A10_TS_METER_NOT_NTC (0x02 << 1)

/**A10 b0 DCDET R
 * DCDET is used to record the power status of DCIN.
 * DCDET <--> DCIN>DCINUV
 * 0 NO
 * 1 YES
 **/
#define A10_DCDET_Y 0x01
#define A10_DCDET_N 0x00

/**0Bh A11 **/
#define G2230_A11_MASK_CHGRUN BIT(6)
#define G2230_A11_MASK_INLIMIT BIT(5)
#define G2230_A11_MASK_THR BIT(4)
#define G2230_A11_MASK_TS BIT(3)
#define G2230_A11_MASK_DCDET BIT(0)

/**A11 b6 MASK_CHGRUN
 * By writing 1’b1 to MASK_CHGRUN to disable asserting /INT low when
 * the Auto Power Source Detection status is changed.
 **/
#define A11_MASK_CHGRUN (0x01 << 6)

/**A11 b5 MASK_INLIMIT
 * By writing 1’b1 to MASK_INLIMIT to disable asserting /INT low when
 * the DCIN Status is changed.
 **/
#define A11_MASK_INLIMIT (0x01 << 5)

/**A11 b4 MASK_THR By writing 1’b1 to MASK_THR to disable asserting /INT low
 * when the event of thermal regulation occurs or not.
 **/
#define A11_MASK_THR (0x01 << 4)

/**A11 b3 MASK_TS By writing 1’b1 to MASK_TS to disable asserting /INT low
 * when battery temp. status is changed.
 **/
#define A11_MASK_TS (0x01 << 3)

/***A11 b0 MASK_DCDET
 * By writing 1’b1 to MASK_DCDET to disable asserting /INT low when
 * the event of DCIN detection crossing occurs or not.
 **/
#define A11_MASK_DCDET (0x01 << 0)


/**0Ch A12 R**/
#define G2230_A12_DCIN_PG BIT(7)
#define G2230_A12_BATLOW BIT(6)
#define G2230_A12_NOBAT BIT(5)
#define G2230_A12_EOC BIT(4)
#define G2230_A12_CHGDONE BIT(3)
#define G2230_A12_SAFE BIT(2)
#define G2230_A12_BST_FAULT BIT(1)

/**A12 b7 DCIN_PG
 * DCIN power status
 * DCIN_PG <--> DCIN Status
 * 0    Not Power Good
 *      DCIN < Max[VDCUV ,(VBAT+ ΔVASDN1)] or DCIN > VDCOV
 * 1    Power Good
 *      Max[VDCUV ,(VBAT+ ΔVASDN1)] < DCIN < VDCOV
 **/
#define A12_DCIN_PG_POWER_GOOD (0x01 << 7)
#define A12_DCIN_PG_NOT_POWER_GOOD		\
			(A12_DCIN_PG_POWER_GOOD & (~A12_DCIN_PG_POWER_GOOD))

/**A12 b6 BATLOW
 * BATLOW is used to record the power status of BAT.
 * BATLOW <--> BAT<BATLOW
 * 0 NO
 * 1 YES
 **/
#define A12_BATLOW (0x01 << 6)

/**A12 b5 NOBAT
 * NOBAT is used to indicate whether the battery is installed.
 * 0 YES
 * 1 NO
 **/
#define A12_NOBAT (0x01 << 5)

/**A12 b4 EOC
 * EOC show the charger status.
 * 0 During Charging
 * 1 Charging Done or Recharging after Termination
 **/
#define A12_EOC (0x01 << 4)

/**A12 b3 CHGDONE
 * CHGDONE show the charger status. CHGDONE is reset by recharging.
 * 0 During Charging or No Battery
 * 1 Charging Done
 **/
#define A12_CHGDONE_SHIFT_NUM (0x03)
#define A12_CHGDONE (0x01 << A12_CHGDONE_SHIFT_NUM)

/**A12 b2 /SAFE
 * /SAFE show the status of Safety Timer.
 * 0 No Expired
 * 1 Expired
 **/
#define A12_SAFE (0x01 << 2)

/**A12 b1 BST_FAULT
 * BST_FAULT is used to indicate any fault conditions that G2230 stops switching
 * 0   Normal
 * 1   Stop switching
 *      (DCIN OVP in OTG mode, BAT<VBOOST_BAT,
 *      or Battery too hot unless A6.ENTS_BOOST set to 0)
 **/
#define A12_BST_FAULT (0x01 << 1)

/**0Dh A13 **/
#define G2230_A13_MASK_DCIN_PG BIT(7)
#define G2230_A13_MASK_BATLOW BIT(6)
#define G2230_A13_MASK_NOBAT BIT(5)
#define G2230_A13_MASK_EOC BIT(4)
#define G2230_A13_MASK_CHGDONE BIT(3)
#define G2230_A13_MASK_SAFE BIT(2)
#define G2230_A13_MASK_BST_FAULT BIT(1)

/**A13 b7 MASK_DCIN_PG
 * By writing 1’b1 to MASK_DCIN_PG to disable asserting /INT low when
 * the event of DCIN power good occurs or not.
 **/
#define A13_MASK_DCIN_PG (0x01 << 7)

/**A13 b6 MASK_BATLOW
 * By writing 1’b1 to MASK_BATLOW to disable asserting /INT low when
 * the event of low battery occurs or not.
 **/
#define A13_MASK_BATLOW  (0x01 << 6)

/**A13 b5 MASK_NOBAT
 * By writing 1’b1 to MASK_NOBAT to disable asserting /INT low when
 * the event of no battery occurs or not.
 **/
#define A13_MASK_NOBAT (0x01 << 5)

/**A13 b4 MASK_EOC By writing 1’b1 to MASK_EOC to disable asserting /INT low
 * when the event of end of charging occurs or not. **/
#define A13_MASK_EOC (0x01 << 4)

/**A13 b3 MASK_CHGDONE
 * By writing 1’b1 to MASK_CHGDONE to disable asserting /INT low when
 * the event of charge done occurs or not.
 **/
#define A13_MASK_CHGDONE (0x01 << 3)

/**A13 b2 MASK_SAFE
 * By writing 1’b1 to MASK_SAFE to disable asserting /INT low when
 * the event of safety timer is expired occurs or not.
 **/
#define A13_MASK_SAFE (0x01 << 2)

/**A13 b1 MASK_BST_FAULT
 * By writing 1’b1 to MASK_BST_FAULT to disable asserting /INT low when
 * any boost function faults occurs or not.
 **/
#define A13_MASK_BST_FAULT (0x01 << 1)

/** 0Eh A14 R **/
#define G2230_A14_OTG_OC BIT(7)
#define G2230_A14_PWRON_IT BIT(6)
#define G2230_A14_PWRON_LP BIT(5)

/**A14 B7 OTG_OC
 * OTG_OC is used to indicate whether OTG is in OC protect (hiccup mode).
 * 0 NO
 * 1 YES
 **/
#define A14_OTG_OC (0x01 << 7)

/**A14 b6 PWRON_IT
 * PWRON_IT is used to record the PWRON rising status.
 * PWRON_IT is reset to 0 when DCIN * plug-in/out or A10.INT is reset to 0.
 * PWRON_IT <--> T>TdbPWRONF
 * 0 NO
 * 1 YES
 **/
#define A14_PWRON_IT (0x01 << 6)

/**A14 b5 PWRON_LP
 * PWRON_LP is used to record the PWRON long press status.
 * PWRON_LP is reset to 0 when DCIN plug-in/out or A10.INT is reset to 0.
 * PWRON_LP <--> T>TdPWRONLP
 * 0 NO
 * 1 YES
 **/
#define A14_PWRON_LP (0x01 << 5)

/** 0Fh A15 **/
#define G2230_A15_MASK_OTG_OC BIT(7)
#define G2230_A15_MASK_PWRON_IT BIT(6)
#define G2230_A15_MASK_PWRON_LP BIT(5)

/* A15 B7 MASK_OTG_OC
 * By writing 1’b1 to MASK_OTG_OC to disable asserting /INT low when
 * OTG enters or leaves hiccup mode.
 */
#define A15_MASK_OTG_OC (0x01 << 7)

/* A15 b6 MASK_PWRON_IT
 * By writing 1’b1 to MASK_PWRON_IT to disable asserting /INT low when
 * the event of PWRON rising with duration longer than TdbPWRONF occurs.
 */
#define A15_MASK_PWRON_IT (0x01 << 6)

/**A15 b5 MASK_PWRON_LP
 * By writing 1’b1 to MASK_PWRON_LP to disable asserting /INT low when
 * the event of PWRON rising with duration longer than TdPWRONLP occurs.
 **/
#define A15_MASK_PWRON_LP (0x01 << 5)

/** 10h A16  R **/
#define G2230_A16_CHGTYPIN (0x07 << 5)
#define G2230_A16_DPM BIT(4)
#define G2230_A16_INCC BIT(3)
#define G2230_A16_DPPM BIT(2)
#define G2230_A16_THSD BIT(1)
#define G2230_A16_NTC100K BIT(0)

/**A16 b7~b5
 * CHGTYPIN <2:0>
 * CHG_TYP<2:0> is used to record the charger type.
 * CHGTYP<2:0> <--> Power source Type
 * 000 Stand Downstream Port (SDP) (0.45A)
 * 001 Reserved
 * 010 Reserved
 * 011 Reserved
 * 100 Reserved
 * 101 Reserved
 * 110 Charging Downstream Port (CDP) (1.5A)
 * 111 Dedicated Charging Port (DCP) (3A)
 **/
#define A16_CHGTYPIN_SHIFT_NUM (0x05)
#define A16_CHGTYPIN_SHIFT_MASK (0x07)
/** right 5-bit **/
#define A16_CHGTYPIN_DCP (0x07)
#define A16_CHGTYPIN_CDP (0x06)
#define A16_CHGTYPIN_SDP (0x00)

/**A16 b4 DPM
 * DPM is used to record the DCIN DPM mode status
 * DPM <--> DPM Status
 * 0 Not in DCIN DPM Mode
 * 1 In DCIN DPM Mode
 **/
#define A16_DPM (0x01 << 4)

/**A16 b3 INCC
 * INCC is used to record the DCIN current limit status
 * INCC <--> DCIN Current Limit Status
 * 0 Not in DCIN current limit
 * 1 In DCIN current limit
 **/
#define A16_INCC  (0x01 << 3)

/**A16 b2 DPPM
 * DPPM is used to record the SYS DPPM mode status
 * DPPM <--> SYS DPPM Status
 * 0 Not in SYS DPPM Mode
 * 1 In SYS DPPM Mode
 **/
#define A16_DPPM_IN_SYS (0x01 << 2)

/**A16 b1 THSD
 * THSD is used to record if the thermal shutdown occurs.
 * THSD <--> Thermal Shutdown occurs
 * 0 NO
 * 1 YES
 **/
#define A16_THSD (0x01 << 1)

/**A16 b0 NTC100K
 * NTC100K is used to record the resistance of the NTC thermistor.
 * NTC100K <--> NTC Thermistor
 * 0 10KΩ
 * 1 100KΩ
 **/
#define A16_NTC10K 0x00
#define A16_NTC100K 0x01

enum g2230_interrupt_type {
	G2230_INTERRUPT_UNKNOWN = 0,
	/** A10 --b0
	 * DCDET is used to record the power status of DCIN.
	 * DCDET  DCIN>DCIN UV
	 * 0  NO
	 * 1  YES
	 **/
	G2230_INTERRUPT_DCDET,
	/** A10 --b3~b1:
	 * Record the battery temperature by detecting the TS pin voltage.
	 * TS_METER<2:0> Battery Temp.
	 * 110  T ≤ T COLD
	 * 100  T COLD < T ≤ T COOL
	 * 000  T COOL < T ≤ T WARM
	 * 001  T WARM < T ≤ T HOT
	 * 011  T HOT < T
	 * 010  NTC not existed
	 * 111  No battery installed
	 **/
	G2230_INTERRUPT_TS_METER,
	/**A10 B4
	 * THR show the status of thermal regulation on charger
	 * THR Thermal Regulation Happen
	 * 0 NO
	 * 1 YES
	 */
	G2230_INTERRUPT_THR,
	/** A10 b5
	 * INLIMIT is used to record DCIN Status.
	 * INLIMIT DCIN Status
	 * 0 Normal
	 * 1 DCIN DPM or DCIN current limit
	 **/
	G2230_INTERRUPT_INLIMIT,
	/** A10 --b6:
	 * CHGRUN is used to record the Auto Power Source Detection Status.
	 * CHGRUN	APSD Status
	 * 0		 Not running
	 * 1		 Running
	 **/
	G2230_INTERRUPT_CHGRUN,
	/**A12 B1
	 * BST_FAULT is used to indicate any fault conditions
	 * BST_FAULT Boost Function Status
	 * 0 Normal
	 * 1Stop switching
	 **/
	G2230_INTERRUPT_BST_FAULT,
	/**A12 B2
	 * SAFE show the status of Safety Timer.
	 * /SAFE Safety Timer Status
	 * 0 No Expired
	 * 1 Expired
	 **/
	G2230_INTERRUPT_SAFE,
	/** A12 B3
	 * CHGDONE show the charger status. CHGDONE is reset by recharging.
	 * CHGDONE Charger Status
	 * 0 During Charging or No Battery
	 * 1 Charging Done
	 **/
	G2230_INTERRUPT_CHGDONE,

	/** A12 b4
	 * EOC show the charger status.
	 * EOC  Charger Status
	 * 0  During Charging
	 * 1  Charging Done or Recharging after Termination
	 **/
	G2230_INTERRUPT_EOC,
	/**A12 B5
	 * NOBAT is used to indicate whether the battery is installed.
	 * NOBAT  Battery installed
	 * 0  YES
	 * 1  NO
	 **/
	G2230_INTERRUPT_NO_BAT,

	/** A12 B6
	 * BATLOW is used to record the power status of BAT.
	 * BATLOW BAT<BATLOW
	 * 0 NO
	 * 1 YES
	 **/
	G2230_INTERRUPT_BATLOW,

	/** A12 B7
	 * DCIN power status
	 * DCIN_PG  DCIN Status
	 * 0   Not Power Good
	 * 1   Power Good
	 **/
	G2230_INTERRUPT_DCIN_PG,
};

//#define G2230_INTERRUPT_DCDET_DCIN_PG
//			(G2230_INTERRUPT_DCIN_PG & G2230_INTERRUPT_DCDET)

enum g2230_chg_status {
	G2230_CHG_STATUS_UNKNOWN,
	G2230_CHG_STATUS_PROGRESS,
	G2230_CHG_STATUS_DONE,
	G2230_CHG_STATUS_MAX,
};

enum g2230_irq_idx {
	G2230_IRQIDX_IRQ0 = 0,
	G2230_IRQIDX_IRQ0_MASK,
	G2230_IRQIDX_IRQ1,
	G2230_IRQIDX_IRQ1_MASK,
	G2230_IRQIDX_MAX,
};

enum g2230_usbsw_state {
	G2230_USBSW_CHG = 0,
	G2230_USBSW_USB,
};

struct g2230_chip {
	struct i2c_client *client;
	struct device *dev;
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	u8 chip_rev;
	int chg_ce_gpio;
	int chg_int_gpio;
	int chg_irq;
	bool isCharging;
	bool online;
	bool bChargingCurHadSet;
	bool bDcdetDisConnect;
	int iDecdetNum;
	int iDecdetIRQnum;
	int iDecdetIRQbit;
	bool bUsbDetect_Init;
	enum charger_type chg_type;
	struct delayed_work g2230_interrupt_delay_work;
	struct mutex io_lock;
	struct mutex ichg_lock;
	struct mutex dcdet_lock;
	atomic_t vbus_good;
#if defined(CONFIG_REGMAP)
	struct regmap *regmap;
#endif
	const char *chg_dev_name;
};


struct g2230_irq_mapping_table {
	const char *name;
	int (*hdlr)(struct g2230_chip *chip);
	int num;
};

//#define G2230_IRQ_DESC(name,irq) { #name,irq,g2230_##name##_irq_handler}
#define G2230_IRQ_MAPPING(_name, _num) \
	{.name = #_name, .hdlr = g2230_##_name##_irq_handler, .num = _num}



static int g2230_get_interrupt_status(struct g2230_chip *chip, u8 *buf);
static int g2230_chip_enable_chgdet(struct g2230_chip *chip, bool en);
static int __g2230_get_ichg(struct g2230_chip *chip, u32 *ichg);

#ifndef CONFIG_TCPC_CLASS
static bool g2230_is_vbusgood(struct g2230_chip *chip);
static int __g2230_dcin_pg_work(struct g2230_chip *chip);
#endif
enum g2230_interrupt_type g2230_get_interrupt_type(struct g2230_chip *chip);
enum g2230_interrupt_type g2230_get_a10_interrupt_type(struct g2230_chip *chip);
enum g2230_interrupt_type g2230_get_a12_interrupt_type(struct g2230_chip *chip);
enum g2230_chg_status g2230_get_charging_cur_status(struct g2230_chip *chip);
#endif

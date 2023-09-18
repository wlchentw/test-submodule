#ifndef _PI_API_H
#define _PI_API_H
#include <string.h>
#define FOR_DV_SIMULATION_USED		0
#define UNDER_PORTING			1

/* To control code size if needed */
/* move to .mk by project
#define SUPPORT_TYPE_LPDDR4		1
#define SUPPORT_TYPE_LPDDR3		0
#define SUPPORT_TYPE_PCDDR4		0
#define SUPPORT_TYPE_PCDDR3		0
#define SUPPORT_PCDDR3_32BIT		0
*/

/***********************************************************************/
/*                  Public Types                                       */
/***********************************************************************/
#define FALSE 0
#define TRUE 1

/***********************************************************************/
/*              Multi Thread Multi Core                                      */
/***********************************************************************/
//#define PARALLEL_CH_CAL
#ifdef PARALLEL_CH_CAL
#define DRAMC_INIT_MULTI_CORE
#define CHANNEL_A_ENABLE 1
#define CHANNEL_B_ENABLE 1
#define CHANNEL_C_ENABLE 1
#define CHANNEL_D_ENABLE 1
#endif

/***********************************************************************/
/*              Constant Define                                        */
/***********************************************************************/
#define chip_name	"[Weber/MT8518]"
#define version	"Version 0.1"

/* cc notes: Used to mark non-exist RG in code porting stage */
#define NEED_REVIEW			1

#define HW_BROADCAST_ON			0

#define DUAL_FREQ_K 0

#define DVFS_EN 0
#define DVFS_STRESS_TEST 0
#if DVFS_EN
#define DVFS_HIGH_FREQ DDR_DDR2400
#define DVFS_LOW_FREQ DDR_DDR1600
#define DVFS_LP4_ADDR_NUM 56
#endif

/* Impedance calculation used Resister values
 * Caculation Equation:
 * OCDP_final = (fcR_PU * OCDP) / fcR_OCD
 * OCDN_final = (fcR_PD * OCDN) / fcR_OCD
 * ODTP_final = (fcR_EXT * OCDP) / fcR_ODT
 * ODTN_final = (fcR_EXT * OCDN) / fcR_ODT
 */
#if SUPPORT_TYPE_LPDDR4
#define fcR_EXT		45
#define fcR_PU		100
#define fcR_PD		45
#define fcR_OCD		45
#define fcR_ODT		60

#define OCDP_DEFAULT	0xe
#define OCDN_DEFAULT	0xd
#define ODTP_DEFAULT	0x0
#define ODTN_DEFAULT	0xa
#elif SUPPORT_TYPE_PCDDR4
#define fcR_EXT		45
#define fcR_PU		45
#define fcR_PD		105
#define fcR_OCD		45
#define fcR_ODT		80

#define OCDP_DEFAULT	0xd
#define OCDN_DEFAULT	0xe
#define ODTP_DEFAULT	0x7
#define ODTN_DEFAULT	0x0
#elif SUPPORT_TYPE_LPDDR3
#define fcR_EXT		55
#define fcR_PU		55
#define fcR_PD		55
#define fcR_OCD		55
#define fcR_ODT		80 /* NOT use, only for compatibility */

#define OCDP_DEFAULT	0xb
#define OCDN_DEFAULT	0xb
#define ODTP_DEFAULT	0x0
#define ODTN_DEFAULT	0x0
#else /* DDR3 */
#if SUPPORT_PCDDR3_32BIT
#define fcR_EXT		45
#define fcR_OCD		45
#define fcR_PU		45
#define fcR_PD		45
#define fcR_OCD		45
#define fcR_ODT		120

#define OCDP_DEFAULT	0xc
#define OCDN_DEFAULT	0xc
#define ODTP_DEFAULT	0x4
#define ODTN_DEFAULT	0x4
#else
#define fcR_EXT		60
#define fcR_PU		60
#define fcR_PD		60
#define fcR_OCD		60
#define fcR_ODT		120

#define OCDP_DEFAULT	0xc
#define OCDN_DEFAULT	0xc
#define ODTP_DEFAULT	0x4
#define ODTN_DEFAULT	0x4
#endif
#endif



#define CALIBRATION_LOG 0
/* #define LK_LEAVE_FREQ_LP4_DDR4266 */

#define FAST_CAL 0

/* Bring Up Selection : Do Not open it when normal operation */
#define FIRST_BRING_UP 1

#if (FOR_DV_SIMULATION_USED == 1)
#define CHANNEL_MAX		2 /* 2 Virtual channel, 1 physical channel */

#define CHANNEL_NUM		2
#else
#define CHANNEL_MAX		2

#define CHANNEL_NUM		2
#endif

/*Feature option*/
#define ENABLE_DUTY_SCAN_V2   1

/* SW option */
#define CBT_K_RANK1_USE_METHOD  1
#if (FOR_DV_SIMULATION_USED == 1)
#define ENABLE_WRITE_DBI 0
#else
#define ENABLE_WRITE_DBI 0 //yifei
#endif

#define DDR4266_FREQ 2133
#define DDR3733_FREQ 1866
#define DDR3200_FREQ 1600
#define DDR2666_FREQ 1333
#define DDR2400_FREQ 1200
#define DDR1866_FREQ 933
#define DDR1600_FREQ 800
#define DDR1066_FREQ 533
#define PERBIT_THRESHOLD_FREQ	(DDR1600_FREQ)
#define DRAM_INITIAL_FREQ	(DDR_DDR1600)
#define LP4_HIGHEST_FREQ	(DDR4266_FREQ)
#define LP4_HIGHEST_FREQSEL	(DDR_DDR2400)
#define DRAM_CALIBRATION_FREQ_1	(DRAM_INITIAL_FREQ)

#define APPLY_LP4_POWER_INIT_SEQUENCE	1 /* cc porting from Azalea */

#define ENABLE_DDRPHY_FREQ_METER 1  /* yifei porting from pll.c */

/* Definitions indicating DRAMC, DDRPHY register shuffle offset */
#define SHU_GRP_DRAMC_OFFSET      0x600
#define SHU_GRP_DDRPHY_OFFSET     0x500

/* init for test engine */
/* pattern0 and base address for test engine when we do calibration */
#define DEFAULT_TEST2_1_CAL 0x55000000
/* pattern1 and offset address for test engine when we  do calibraion */
#if (FOR_DV_SIMULATION_USED == 0)
#define DEFAULT_TEST2_2_CAL 0xaa000400
#else
/* cc notes: Reduce length to save simulation time */
#define DEFAULT_TEST2_2_CAL 0xaa000200
#endif
/* timeout for TE2: (CMP_CPT_POLLING_PERIOD X MAX_CMP_CPT_WAIT_LOOP) */
#define CMP_CPT_POLLING_PERIOD 1
/* max loop */
#define MAX_CMP_CPT_WAIT_LOOP 1000000

/* common */
#define CATRAINING_NUM_LP4		6
#define CATRAINING_NUM_LP3		10
#define CA_GOLDEN_PATTERN		0x55555555
#define DQS_NUMBER			4
#define DQS_BIT_NUM			8
/* define max support bus width in the system (to allocate array size) */
#define DQ_DATA_WIDTH			32
#define DQ_DATA_WIDTH_LP4		16
/* 100us */
#define TIME_OUT_CNT			100
#define HW_REG_SHUFFLE_MAX		4

/* Gating window */
/* 8 for testing 1 */
#define DQS_GW_COARSE_STEP		1
#define DQS_GW_FINE_START		0
#define DQS_GW_FINE_END			32
#define DQS_GW_FINE_STEP		4

/* DATLAT */
#define DATLAT_TAP_NUMBER	32 /* DATLAT[3:0] = {0x80[20:4]} */

/* TX DQ/DQS */
#define MAX_TX_DQDLY_TAPS	31 /* max DQ TAP number */
#define MAX_TX_DQSDLY_TAPS	31 /* max DQS TAP number */
#define TX_DQ_OE_SHIFT_LP4	3
#define TX_K_DQM_WITH_WDBI	(SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)

/* Run time config */
/* DQS strobe calibration enable */
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_LPDDR3)
#define HW_GATING
#define GATING_ADJUST_TXDLY_FOR_TRACKING	1
#else
#define GATING_ADJUST_TXDLY_FOR_TRACKING	0
#endif
#define DUMMY_READ_FOR_TRACKING
#define ENABLE_SW_RUN_TIME_ZQ_WA	1
#if !ENABLE_SW_RUN_TIME_ZQ_WA
#define ZQCS_ENABLE_LP4
#endif
/* After enable rumtime MR4 read, Get DramC SPCMDRESP_REFRESH_RATE. */
#define TEMP_SENSOR_ENABLE
/* Low pwer settings */
#define APPLY_LOWPOWER_GOLDEN_SETTINGS	1
/* #define SPM_LIB_USE */
#ifdef SPM_LIB_USE
#define SPM_CONTROL_AFTERK
#endif
#define IMPEDANCE_TRACKING_ENABLE

#define MR01	1
#define MR02	2
#define MR03	3
#define MR05	5
#define MR06	6
#define MR08	8
#define MR11	11
#define MR12	12
#define MR13	13
#define MR14	14
#define MR18	18
#define MR19	19
#define MR22	22
#define MR23	23
#define MR37	37

#define MR01_FSP0_INIT	0x26
#define MR01_FSP1_INIT	0x56
#define MR02_INIT	0x1a
#define MR02_4266	0x3f
#define MR02_3733	0x36
#define MR02_3200	0x2d
#define MR02_2666	0x24
#define MR02_1600	0x12
#define MR03_INIT	0x31
#define MR11_ODT_DIS	0x44
#define MR11_ODT_60	0x44
#define MR11_ODT_80	0x03
#define MR12_INIT	0x50
#define MR13_RRO 1
#define MR13_FSP0_INIT	0 | (MR13_RRO << 4) | (1 << 3)
#define MR13_FSP1_INIT	0xc0 | (MR13_RRO << 4) | (1 << 3)
#define MR13_FSP1_SET	0x40 | (MR13_RRO << 4) | (1 << 3)
#define MR14_FSP0_INIT	0x4e /*Vref_Level=30, vref_range=1*/
#define MR14_FSP1_INIT	0x4e /*Vref_Level=16, vref_range=1*/
#define MR22_20		0x20
#define MR22_38		0x38
#define MR22_24		0x24
#define MR22_3C		0x3C
#define MR23_INIT	0x3f

#define PA_IMPROVEMENT_FOR_DRAMC_ACTIVE_POWER 0

#define ENABLE_TX_TRACKING	1
#if ENABLE_TX_TRACKING
#define ENABLE_SW_TX_TRACKING	1
#endif
#define ENABLE_RX_TRACKING_LP4	1

#define ENABLE_RODT_TRACKING	1

#define CPU_RW_TEST_AFTER_K	1
#define TA2_RW_TEST_AFTER_K	1

#define PRINT_CALIBRATION_SUMMARY	1

/* cc mark #define XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY */

#define DRAMC_MODEREG_CHECK	1

#define ENABLE_MIOCK_JMETER 1

#define SET_FLD			0x1
#define CLEAR_FLD		0x0

#define TERM			1
#define UNTERM			0

#define PATTERN1	0x55000000
#define PATTERN2	0xaa000000
#define PATTERN3	0xaaaa5555

#define NIBBLE_MAX	0xfU
#define BYTE_MAX	0xffU
#define WORD_MAX	0xffffU
#define LWORD_MAX	0xffffffffU
#define UNCHAR_MAX	0x7fU
#define NUM_4096	4096

#define EDGE_NUMBER	2
/************* FIRST_BRING_UP Init Definition **************/
#if FIRST_BRING_UP

#undef ENABLE_DUTY_SCAN_V2
#define ENABLE_DUTY_SCAN_V2		0

#undef APPLY_LOWPOWER_GOLDEN_SETTINGS
#define APPLY_LOWPOWER_GOLDEN_SETTINGS	0

#undef TX_K_DQM_WITH_WDBI
#define TX_K_DQM_WITH_WDBI		0

#undef PARALLEL_CH_CAL

#undef PA_IMPROVEMENT_FOR_DRAMC_ACTIVE_POWER
#define PA_IMPROVEMENT_FOR_DRAMC_ACTIVE_POWER	0

#undef ENABLE_TX_TRACKING
#undef ENABLE_SW_TX_TRACKING
#define ENABLE_TX_TRACKING		0
#define ENABLE_SW_TX_TRACKING		0
#define ENABLE_SR_TEST		0


#undef CPU_RW_TEST_AFTER_K
#undef TA2_RW_TEST_AFTER_K
#define CPU_RW_TEST_AFTER_K		0
#define TA2_RW_TEST_AFTER_K		1

#undef ENABLE_RX_TRACKING_LP4
#define ENABLE_RX_TRACKING_LP4		0

/* #define ETT */
#define SCRAMBLE_ENABLE 1
/* #define DDR_RESERVE_MODE */

/* #undef ENABLE_MIOCK_JMETER */

/* #undef HW_GATING */
#undef DUMMY_READ_FOR_TRACKING
/* #undef ZQCS_ENABLE_LP4 */
/* #undef SPM_CONTROL_AFTERK */
#undef TEMP_SENSOR_ENABLE
#undef IMPEDANCE_TRACKING_ENABLE
#undef ENABLE_SW_RUN_TIME_ZQ_WA
#define ENABLE_SW_RUN_TIME_ZQ_WA	0

#undef XRTR2R_PERFORM_ENHANCE_DQSG_RX_DLY

#undef CALIBRATION_LOG
#define CALIBRATION_LOG			1

#undef HW_BROADCAST_ON
#define HW_BROADCAST_ON		0

#undef GATING_ADJUST_TXDLY_FOR_TRACKING
#define GATING_ADJUST_TXDLY_FOR_TRACKING  0

#endif /* FIRST_BRING_UP */

/*
 *****************************************************************************
 * for D Sim sumulation used
 *****************************************************************************
 */
#define SIMULATION_LP4_ZQ 1
#define SIMULATION_CBT 1
#define SIMULATION_WRITE_LEVELING  1
#define SIMULATION_GATING 1
#define SIMUILATION_LP4_RDDQC 1
#define SIMULATION_DATLAT 1
#define SIMULATION_SW_IMPED 1
#define SIMULATION_RX_PERBIT    1
#define SIMULATION_TX_PERBIT 1 /*  Please enable with write leveling */

#if (!PRINT_CALIBRATION_SUMMARY)
#define set_calibration_result(x, y, z)
#endif

/***********************************************************************/
/*               Defines                                                */
/***********************************************************************/
#define ENABLE  1
#define DISABLE 0

#define CBT_LOW_FREQ   0
#define CBT_HIGH_FREQ   1

#define PASS_RANGE_NA	0x7fff
#define MAX_CLK_PI_DELAY	35
#define MAX_CS_PI_DELAY		63
#define MAX_RX_DQSDLY_TAPS	127
#define MAX_RX_DQDLY_TAPS	63
/* RX_VREF_DEFAULT */
#define RX_VREF_DEFAULT_ODT	0xe
#define RX_VREF_DEFAULT	0x16
#define RX_VREF_DEFAULT_X_ODT	0xb
#define RX_VREF_DEFAULT_X	0x16
#define RX_VREF_DEFAULT_P	0x10

#if FAST_CAL
/* CBT */
#define CBT_VREF_BEGIN		(13 - 5)
#define CBT_VREF_END		(13 + 5)
#define CBT_VREF_BEGIN_X	(27 - 3)
#define CBT_VREF_END_X		(27 + 7)
#define CBT_VREF_RANGE1_BEGIN	21
#define CBT_VREF_MAX		50
#define MAX_CA_PI_DELAY	63
/* Write Leveling */
#define WL_OFFSET	12
#define WL_STEP		1
#define WL_RANGE	64
/* RxdqsGating */
#define RX_DQS_BEGIN_4266	20 /*2 4 0*/
#define RX_DQS_BEGIN_3733	21 /*2 5 0*/
#define RX_DQS_BEGIN_3200	27 /*3 3 0*/
#define RX_DQS_BEGIN_1600	18 /*2 2 0*/
#define RX_DQS_RANGE	16
/* RX DQ/DQS */
#define RX_VREF_RANGE_BEGIN	6
#define RX_VREF_RANGE_END	31
#define RX_VREF_RANGE_STEP	1
#define RX_VREF_RANGE_BEGIN_ODTOFF	18
#define MAX_RX_DQSDLY_TAPS_2666 10
#define MAX_RX_DQSDLY_TAPS_1600 36
/* TX DQ/DQS */
#define TX_VREF_RANGE_BEGIN	16
#define TX_VREF_RANGE_END	50
#define TX_VREF_RANGE_STEP	2
#define TX_VREF_RANGE_BEGIN1	(13 - 5) /* 300/1100(VDDQ) = 27.2% */
#define TX_VREF_RANGE_END1	(13 + 5)
#define TX_VREF_RANGE_BEGIN2	(30 - 5) /* 290/600(VDDQ)=48.3% */
#define TX_VREF_RANGE_END2	(30 + 5)
#else
/* CBT */
#define CBT_VREF_BEGIN		(13 - 5)
#define CBT_VREF_END		(13 + 5)
#define CBT_VREF_BEGIN_X	(27 - 3)
#define CBT_VREF_END_X		(27 + 7)
#define CBT_VREF_RANGE1_BEGIN	21
#define CBT_VREF_MAX		50
#define MAX_CA_PI_DELAY	63
/* Write Leveling */
#define WL_OFFSET 0
#define WL_STEP		1
#define WL_RANGE	64
/* RxdqsGating */
#define RX_DQS_BEGIN_4266	19
#define RX_DQS_BEGIN_3733	18
#define RX_DQS_BEGIN_2400	33
#define RX_DQS_BEGIN_3200	26
#define RX_DQS_BEGIN_1600	18
#define RX_DQS_RANGE	16
/* RX DQ/DQS */
#define RX_VREF_RANGE_BEGIN	0
#define RX_VREF_RANGE_END	31
#define RX_VREF_RANGE_STEP	1
#define RX_VREF_RANGE_BEGIN_ODTOFF	18
#define MAX_RX_DQSDLY_TAPS_2666 32
#define MAX_RX_DQSDLY_TAPS_1600 48
/* TX DQ/DQS */
#define TX_VREF_RANGE_BEGIN	25
#define TX_VREF_RANGE_END	50
#define TX_VREF_RANGE_STEP	1
#define TX_VREF_RANGE_BEGIN1	(13 - 5) /* 300/1100(VDDQ) = 27.2% */
#define TX_VREF_RANGE_END1	(13 + 5)
#define TX_VREF_RANGE_BEGIN2	(27 - 5) /* 290/600(VDDQ)=48.3% */
#define TX_VREF_RANGE_END2	(27 + 5)
#endif

typedef enum {
	DRAM_OK = 0,
	DRAM_FAIL,
} DRAM_STATUS_T;     /*  DRAM status type */

typedef enum {
	CKE_FIXOFF = 0,
	CKE_FIXON,
	/*
	 * After CKE FIX on/off,
	 * CKE should be returned to dynamic (control by HW)
	 */
	CKE_DYNAMIC
} CKE_FIX_OPTION;

typedef enum {
	/* just need to write CKE FIX register to current channel */
	CKE_WRITE_TO_ONE_CHANNEL = 0,
	/* need to write CKE FIX register to all channel */
	CKE_WRITE_TO_ALL_CHANNEL
} CKE_FIX_CHANNEL;

typedef enum {
	AD_MPLL_208M_CK = 0,
	DA_MPLL_52M_DIV_CK,
	FMEM_CK_BFE_DCM_CH0,
} CLOCK_SRC_T;

typedef enum {
	DLL_MASTER = 0,
	DLL_SLAVE,
} DRAM_DLL_MODE_T;

/* Do not change */
typedef enum {
	DDR_DDR2667 = 0,
	DDR_DDR2400,
	DDR_DDR1866,
	DDR_DDR1600,
	DDR_DDR1066,
	DDR_FREQ_MAX
} DRAM_PLL_FREQ_SEL_T; /*  DRAM DFS type */

typedef enum {
	DRAM_DFS_SHUFFLE_1 = 0,
#if 0 /* cc mark. Magnolia only support one shuffle */
	DRAM_DFS_SHUFFLE_2,
	DRAM_DFS_SHUFFLE_3,
	DRAM_DFS_SHUFFLE_4,
#endif
	DRAM_DFS_SHUFFLE_MAX
} DRAM_DFS_SHUFFLE_TYPE_T;	/*  DRAM SHUFFLE RG type */

typedef struct _DRAM_DFS_FREQUENCY_TABLE_T {
	DRAM_PLL_FREQ_SEL_T freq_sel;
	unsigned short frequency;
	DRAM_DFS_SHUFFLE_TYPE_T shuffleIdx;
} DRAM_DFS_FREQUENCY_TABLE_T;

typedef enum {
	DRAM_CALIBRATION_ZQ = 0,
	DRAM_CALIBRATION_SW_IMPEDANCE,
	DRAM_CALIBRATION_CA_TRAIN,
	DRAM_CALIBRATION_WRITE_LEVEL,
	DRAM_CALIBRATION_GATING,
	DRAM_CALIBRATION_DATLAT,
	DRAM_CALIBRATION_RX_RDDQC,
	DRAM_CALIBRATION_RX_PERBIT,
	DRAM_CALIBRATION_TX_PERBIT,
	DRAM_CALIBRATION_MAX
} DRAM_CALIBRATION_STATUS_T;

typedef enum {
	DDRPHY_CONF_A = 0,
	DDRPHY_CONF_B,
	DDRPHY_CONF_C,
	DDRPHY_CONF_D,
	DDRPHY_CONF_MAX
} DDRPHY_CONF_T;

/* cc notes: Virtual!! */
typedef enum {
	CHANNEL_A = 0,
	CHANNEL_B,
	CHANNEL_C,
	CHANNEL_D,
} DRAM_CHANNEL_T;

typedef enum {
	CHANNEL_SINGLE = 1,
	CHANNEL_DUAL,
	CHANNEL_THIRD,
	CHANNEL_FOURTH
} DRAM_CHANNEL_NUMBER_T;

typedef enum {
	RANK_0 = 0,
	RANK_1,
	RANK_MAX
} DRAM_RANK_T;

typedef enum {
	RANK_SINGLE = 1,
	RANK_DUAL
} DRAM_RANK_NUMBER_T;

/* Do not change */
typedef enum {
	TYPE_PCDDR3 = 0,
	TYPE_PCDDR4,
	TYPE_LPDDR3,
	TYPE_LPDDR4,
	TYPE_MAX,
} DRAM_DRAM_TYPE_T;

/*  For faster switching between term and un-term operation
 * FSP_0: For un-terminated freq.
 * FSP_1: For terminated freq.
 */
typedef enum {
	FSP_0 = 0,
	FSP_1,
	FSP_MAX
} DRAM_FAST_SWITH_POINT_T;

/*
 * Internal CBT mode enum
 * 1. Calibration flow uses get_dram_cbt_mode to
 *    differentiate between mixed vs non-mixed LP4
 * 2. Declared as dram_cbt_mode[RANK_MAX] internally to
 *    store each rank's CBT mode type
 */
typedef enum {
	CBT_NORMAL_MODE = 0,
	CBT_BYTE_MODE1
} DRAM_CBT_MODE_T;

/*
 * External CBT mode enum
 * Due to MDL structure compatibility (single field for dram CBT mode),
 * the below enum is used in preloader to differentiate between dram cbt modes
 */
typedef enum {
	CBT_R0_R1_NORMAL = 0,		/*  Normal mode */
	CBT_R0_R1_BYTE,		/*  Byte mode */
	CBT_R0_NORMAL_R1_BYTE,	/*  Mixed mode R0: Normal R1: Byte */
	CBT_R0_BYTE_R1_NORMAL	/*  Mixed mode R0: Byte R1: Normal */
} DRAM_CBT_MODE_EXTERN_T;

typedef enum {
	ODT_OFF = 0,
	ODT_ON
} DRAM_ODT_MODE_T;

typedef enum {
	DBI_OFF = 0,
	DBI_ON
} DRAM_DBI_MODE_T;

typedef enum {
	DATA_WIDTH_16BIT = 16,
	DATA_WIDTH_32BIT = 32
} DRAM_DATA_WIDTH_T;

typedef enum {
	GET_MDL_USED = 0,
	NORMAL_USED
} DRAM_INIT_USED_T;

typedef enum {
	MODE_1X = 0,
	MODE_2X
} DRAM_DRAM_MODE_T;

typedef enum {
	PACKAGE_SBS = 0,
	PACKAGE_POP
} DRAM_PACKAGE_T;

typedef enum {
	TE_OP_WRITE_READ_CHECK = 0,
	TE_OP_READ_CHECK
} DRAM_TE_OP_T;

typedef enum {
	TEST_ISI_PATTERN = 0,   /* don't change */
	TEST_AUDIO_PATTERN = 1, /* don't change */
	TEST_XTALK_PATTERN = 2, /* don't change */
} DRAM_TEST_PATTERN_T;

typedef enum {
	BL_TYPE_4 = 0,
	BL_TYPE_8
} DRAM_BL_TYPE_T;

typedef enum {
	fcDATLAT_USE_DEFAULT = 0,
	fcDATLAT_USE_RX_SCAN,
} DRAM_DATLAT_CALIBRATION_TYTE_T;

typedef enum {
	TX_DQ_DQS_MOVE_DQ_ONLY = 0,
	TX_DQ_DQS_MOVE_DQM_ONLY,
	TX_DQ_DQS_MOVE_DQ_DQM
} DRAM_TX_PER_BIT_CALIBRATION_TYTE_T;

typedef enum {
	TX_DQM_WINDOW_SPEC_IN = 0xfe,
	TX_DQM_WINDOW_SPEC_OUT = 0xff
} DRAM_TX_PER_BIT_DQM_WINDOW_RESULT_TYPE_T;

typedef enum {
	VREF_RANGE_0 = 0,
	VREF_RANGE_1,
	VREF_RANGE_MAX
} DRAM_VREF_RANGE_T;

typedef struct _REG_TRANSFER {
	unsigned int addr;
	unsigned int fld;
} REG_TRANSFER_T;

/*
 * enum for CKE toggle mode
 * (toggle both ranks
 * 1. at the same time (CKE_RANK_DEPENDENT)
 * 2. individually (CKE_RANK_INDEPENDENT))
 */
typedef enum {
	CKE_RANK_INDEPENDENT = 0,
	CKE_RANK_DEPENDENT
} CKE_CTRL_MODE_T;

typedef enum {
	channel_density_2Gb = 0,
	channel_density_3Gb,
	channel_density_4Gb,
	channel_density_6Gb,
	channel_density_8Gb,
	channel_density_12Gb,
	channel_density_16Gb,
	channel_density_number,
} DRAM_SUPPORT_DENSITY;

/*
 * Definitions to enable specific freq's
 * ACTiming support (To save code size)
 */
#define SUPPORT_LP4_DDR2400_ACTIM 1
#define SUPPORT_LP4_DDR1600_ACTIM 1

#define SUPPORT_LP3_DDR1866_ACTIM 1
#define SUPPORT_LP3_DDR1600_ACTIM 1

#define SUPPORT_PC4_DDR2667_ACTIM 1
#define SUPPORT_PC4_DDR2400_ACTIM 1
#define SUPPORT_PC4_DDR1866_ACTIM 1

#define SUPPORT_PC3_DDR1866_ACTIM 1
#define SUPPORT_PC3_DDR1600_ACTIM 1

/*
 *  Used to keep track the total number of LP4 ACTimings
 *  Since READ_DBI is enable/disabled using preprocessor C define
 * -> Save code size by excluding unneeded ACTimingTable entries
 * Note 1: READ_DBI on/off is for (LP4 data rate >= DDR2667 (FSP1))
 * Must make sure DDR3733 is the 1st entry (DMCATRAIN_INTV is used)
 */
typedef enum {
	AC_TIME_NON_USED = 0,
#if SUPPORT_LP4_DDR2400_ACTIM
	AC_TIME_LP4_NORM_DDR2400_RDBI_OFF,
#endif
#if SUPPORT_LP4_DDR1600_ACTIM
	AC_TIME_LP4_NORM_DDR1600_RDBI_OFF,
#endif
#if SUPPORT_PC4_DDR2667_ACTIM
	AC_TIME_PC4_NORM_DDR2667,
#endif
#if SUPPORT_PC4_DDR2400_ACTIM
	AC_TIME_PC4_NORM_DDR2400,
#endif
#if SUPPORT_PC4_DDR1866_ACTIM
	AC_TIME_PC4_NORM_DDR1866,
#endif
#if SUPPORT_LP3_DDR1866_ACTIM
	AC_TIME_LP3_NORM_DDR2400,
#endif
#if SUPPORT_LP3_DDR1600_ACTIM
	AC_TIME_LP3_NORM_DDR1600,
#endif
#if SUPPORT_PC3_DDR1866_ACTIM
	AC_TIME_PC3_NORM_DDR2400,
#endif
#if SUPPORT_PC3_DDR1600_ACTIM
	AC_TIME_PC3_NORM_DDR1600,
#endif
	AC_TIMING_NUMBER
} AC_TIMING_COUNT_TYPE_T;

typedef enum {
	TA2_RKSEL_XRT = 3,
	TA2_RKSEL_HW = 4,
} TA2_RKSEL_TYPE_T;

typedef enum {
	TA2_PAT_SWITCH_OFF = 0,
	TA2_PAT_SWITCH_ON,
} TA2_PAT_SWITCH_TYPE_T;

typedef enum {
	GW_MODE_NORMAL = 0,
	GW_MODE_7UI,
	GW_MODE_8UI,
} GW_MODE_TYPE_T;

typedef enum {
	PINMUX_CBT = 0,
	PINMUX_MRR,
} MRR_PINMUX;


#define TOTAL_AC_TIMING_NUMBER		AC_TIMING_NUMBER

typedef struct _DRAMC_CTX_T {
	DRAM_CHANNEL_NUMBER_T support_channel_num;
	DRAM_CHANNEL_T channel;
	DRAM_RANK_NUMBER_T support_rank_num;
	DRAM_RANK_T rank;
	DRAM_PLL_FREQ_SEL_T freq_sel;
	DRAM_DFS_SHUFFLE_TYPE_T shu_type;
	DRAM_DRAM_TYPE_T dram_type;
	DRAM_FAST_SWITH_POINT_T dram_fsp;
	DRAM_ODT_MODE_T odt_onoff;
	DRAM_CBT_MODE_T dram_cbt_mode[RANK_MAX];
	DRAM_DBI_MODE_T dbi_r_onoff[FSP_MAX];
	DRAM_DBI_MODE_T dbi_w_onoff[FSP_MAX];
	DRAM_DATA_WIDTH_T data_width;
	unsigned int test2_1;
	unsigned int test2_2;
	DRAM_TEST_PATTERN_T test_pattern;
	unsigned short frequency;
	unsigned short freqGroup;
	unsigned short shuffle_frequency[DRAM_DFS_SHUFFLE_MAX];
	unsigned short vendor_id;
	unsigned short revision_id;
	unsigned short density;
	unsigned long long ranksize[RANK_MAX];
	unsigned char ucnum_dlycell_perT;
	unsigned short delay_cell_timex100;
	unsigned char enable_cbt_scan_vref;
	unsigned char enable_rx_scan_vref;
	unsigned char enable_tx_scan_vref;
	unsigned char ssc_en;
	unsigned char en_4bit_mux;
#if PRINT_CALIBRATION_SUMMARY
	unsigned int cal_result_flag[CHANNEL_MAX][RANK_MAX];
	unsigned int cal_execute_flag[CHANNEL_MAX][RANK_MAX];
#endif
	unsigned char arfg_write_leveling_init_shif[CHANNEL_MAX][RANK_MAX];
	unsigned char fg_tx_perbif_init[CHANNEL_MAX][RANK_MAX];
} DRAMC_CTX_T;

typedef struct _PASS_WIN_DATA_T {
	signed short first_pass;
	signed short last_pass;
	signed short win_center;
	unsigned short win_size;
	unsigned short best_dqdly;
} PASS_WIN_DATA_T;

typedef struct _RX_DELAY_SET_PERBIT_T {
	signed int dqs_dly_perbyte[DQS_NUMBER];
	signed int dqm_dly_perbyte[DQS_NUMBER];
} RX_DELAY_SET_PERBIT_T;


typedef struct _FINAL_WIN_DATA_T {
	unsigned char final_vref;
	signed int final_ca_clk;
	unsigned char final_range;
} FINAL_WIN_DATA_T;

#if SUPPORT_TYPE_LPDDR4
typedef struct _SCAN_WIN_DATA_T {
	unsigned int finish_count;
	unsigned int ca_win_sum;
	signed int ca_center_sum;
	signed int ca_center[CATRAINING_NUM_LP4];
	signed int first_ca_pass[CATRAINING_NUM_LP4];
	signed int last_ca_pass[CATRAINING_NUM_LP4];
} SCAN_WIN_DATA_T;
#else
typedef struct _SCAN_WIN_DATA_T {
	unsigned int finish_count;
	unsigned int ca_win_sum;
	signed int ca_center_sum;
	signed int ca_center[CATRAINING_NUM_LP3];
	signed int first_ca_pass[CATRAINING_NUM_LP3];
	signed int last_ca_pass[CATRAINING_NUM_LP3];
} SCAN_WIN_DATA_T;

#endif
typedef struct _TXDQS_PERBIT_DLY_T {
	signed char first_dqdly_pass;
	signed char last_dqdly_pass;
	signed char first_dqsdly_pass;
	signed char last_dqsdly_pass;
	unsigned char best_dqdly;
	unsigned char best_dqsdly;
} TXDQS_PERBIT_DLY_T;

typedef struct _RXDQS_GATING_TRANS_T {
	unsigned char dqs_lead[DQS_NUMBER];
	unsigned char dqs_lag[DQS_NUMBER];
	unsigned char dqs_high[DQS_NUMBER];
	unsigned char dqs_transition[DQS_NUMBER];
	unsigned char dly_coarse_large_leadLag[DQS_NUMBER];
	unsigned char dly_coarse_0p5t_leadLag[DQS_NUMBER];
	unsigned char dly_fine_tune_leadLag[DQS_NUMBER];
} RXDQS_GATING_TRANS_T;

typedef struct _RXDQS_GATING_WIN_T {
	unsigned char pass_begin[DQS_NUMBER];
	unsigned char pass_count[DQS_NUMBER];
	unsigned char min_coarse_tune2t[DQS_NUMBER];
	unsigned char min_coarse_tune0p5t[DQS_NUMBER];
	unsigned char min_fine_tune[DQS_NUMBER];
	unsigned char pass_count_1[DQS_NUMBER];
	unsigned char min_coarse_tune2t_1[DQS_NUMBER];
	unsigned char min_coarse_tune0p5t_1[DQS_NUMBER];
	unsigned char min_fine_tune_1[DQS_NUMBER];
} RXDQS_GATING_WIN_T;

typedef struct _RXDQS_GATING_BEST_WIN_T {
	unsigned char best_fine_tune[DQS_NUMBER];
	unsigned char best_coarse_tune0p5t[DQS_NUMBER];
	unsigned char best_coarse_tune2t[DQS_NUMBER];
	unsigned char best_fine_tune_p1[DQS_NUMBER];
	unsigned char best_coarse_tune0p5t_p1[DQS_NUMBER];
	unsigned char best_coarse_tune2t_p1[DQS_NUMBER];
} RXDQS_GATING_BEST_WIN_T;

typedef struct _RXDQS_GATING_CAL_T {
	unsigned char dly_coarse_0p5t;
	unsigned char dly_coarse_large;
	unsigned char dly_fine_xt;
	unsigned char dqs_gw_fine_step;
	unsigned char dly_coarse_large_p1;
	unsigned char dly_coarse_0p5t_p1;
} RXDQS_GATING_CAL_T;

typedef struct _TX_DLY_T {
	unsigned char dq_final_pi[DQS_NUMBER];
	unsigned char dq_final_ui_large[DQS_NUMBER];
	unsigned char dq_final_ui_small[DQS_NUMBER];
	unsigned char dq_final_oen_ui_large[DQS_NUMBER];
	unsigned char dq_final_oen_ui_small[DQS_NUMBER];
} TX_DLY_T;

typedef struct _TX_FINAL_DLY_T {
	unsigned char dq_final_dqm_pi[DQS_NUMBER];
	unsigned char dq_final_dqm_ui_large[DQS_NUMBER];
	unsigned char dq_final_dqm_ui_small[DQS_NUMBER];
	unsigned char dq_final_dqm_oen_ui_large[DQS_NUMBER];
	unsigned char dq_final_dqm_oen_ui_small[DQS_NUMBER];
} TX_FINAL_DLY_T;

typedef struct _DRAM_INFO_BY_MRR_T {
	unsigned short mr5_vendor_id;
	unsigned short mr6_vevision_id;
	unsigned long long mr8_density[CHANNEL_NUM][RANK_MAX];
} DRAM_INFO_BY_MRR_T;

typedef struct _VCORE_DELAYCELL_T {
	unsigned short u2Vcore;
	unsigned short u2DelayCell;
} VCORE_DELAYCELL_T;

/* cc notes: Initially defined for LP4.
 * Reused in this project for all types
 */
typedef struct _MR_SET_VALUE_T {
	unsigned short mr00_value[FSP_MAX]; /* cc add for PCDDR3/4 use */
	unsigned short mr01_value[FSP_MAX];
	unsigned short mr02_value[FSP_MAX];
	unsigned short mr03_value[FSP_MAX];
	unsigned short mr04_value[FSP_MAX]; /* cc add for PCDDR4 */
	unsigned short mr05_value[FSP_MAX]; /* cc add for PCDDR4 */
	unsigned short mr06_value[FSP_MAX]; /* cc add for PCDDR4 */
	unsigned short mr10_value[FSP_MAX]; /* cc add for LP3 */
	unsigned short mr12_value[CHANNEL_NUM][RANK_MAX][FSP_MAX];
	unsigned short mr13_value[FSP_MAX];
	unsigned short mr14_value[CHANNEL_NUM][RANK_MAX][FSP_MAX];
	unsigned short mr18_value[CHANNEL_NUM][RANK_MAX];
	unsigned short mr19_value[CHANNEL_NUM][RANK_MAX];
	unsigned short mr23_value[CHANNEL_NUM][RANK_MAX];
	unsigned short mr63_value[FSP_MAX]; /* cc add for LP3 */
} MR_SET_VALUE_T;

/* For new register access */
#define DRAMC_REG_ADDR(offset)    ((p->channel << POS_BANK_NUM)+(offset))
#define SYS_REG_ADDR(offset)    (offset)

/***********************************************************************/
/*               Public Functions                                       */
/***********************************************************************/
/*  basic function */
unsigned char is_lp4_family(DRAM_DRAM_TYPE_T dram_type);
unsigned char is_byte_swapped(DRAMC_CTX_T *p);
/*  Used to support freq's not in ACTimingTable */
void set_freq_group(DRAMC_CTX_T *p);
void dram_scramble(DRAMC_CTX_T *p);
void dramc_init_pre_settings(DRAMC_CTX_T *p);

DRAM_STATUS_T dramc_init(DRAMC_CTX_T *p);

void dramc_setting_init(DRAMC_CTX_T *p);
void auto_refresh_cke_off(DRAMC_CTX_T *p);
void self_refresh_switch(DRAMC_CTX_T *p, unsigned char option);
void dvfs_settings(DRAMC_CTX_T *p);
void set_mr13_vrcg_to_normal_operation(DRAMC_CTX_T *p);
DRAM_STATUS_T ddr_update_ac_timing(DRAMC_CTX_T *p);

void dramc_set_rank_engine2(DRAMC_CTX_T *p, unsigned char u1RankSel);
DRAM_STATUS_T dramc_engine2_init(DRAMC_CTX_T *p, unsigned int test2_1,
	unsigned int test2_2, unsigned char testaudpat,
	unsigned char log2loopcount);
unsigned int dramc_engine2_run(DRAMC_CTX_T *p, DRAM_TE_OP_T wr,
	unsigned char testaudpat);
void dramc_engine2_end(DRAMC_CTX_T *p);
void dramc_run_time_config(DRAMC_CTX_T *p);
void ddr_phy_low_power_enable(DRAMC_CTX_T *p);

#ifdef SPM_LIB_USE
void transfer_to_spm_control(DRAMC_CTX_T *p);
void transfer_pll_to_spm_control(DRAMC_CTX_T *p);
void spm_pinmux_setting(DRAMC_CTX_T *p);
void cbt_dramc_dfs_direct_jump_set(DRAMC_CTX_T *p, unsigned char shu_level,
	unsigned char shu_ack);
#else
#define transfer_to_spm_control(_x_) show_msg((INFO, "SPM_CONTROL: OFF\n"))
#define transfer_pll_to_spm_control(_x_)
#define spm_pinmux_setting(_x_)
#define cbt_dramc_dfs_direct_jump_set(_x_, _y_, _z_)
#endif

void enable_dramc_phy_dcm(DRAMC_CTX_T *p, unsigned char en);

void set_dram_mr_write_leveling_on_off(DRAMC_CTX_T *p, unsigned char on_off);
void move_dramc_tx_dqs(DRAMC_CTX_T *p, unsigned char byte_idx,
	signed char shift_ui);
void cke_fix_on_off(DRAMC_CTX_T *p, CKE_FIX_OPTION option,
	CKE_FIX_CHANNEL write_channel_num);
/*
 * Control CKE toggle mode
 * (toggle both ranks
 * 1. at the same time (CKE_RANK_DEPENDENT)
 * 2. individually (CKE_RANK_INDEPENDENT))
 */
void cke_rank_ctrl(DRAMC_CTX_T *p, CKE_CTRL_MODE_T cke_ctrl_mode);
/*
 * rx_dqs_isi_pulse_cg() -
 * API for "RX DQS ISI pulse CG function" 0: disable, 1: enable
 */
void rx_dqs_isi_pulse_cg(DRAMC_CTX_T *p, unsigned char on_off);
DRAM_STATUS_T DramcRegDump(DRAMC_CTX_T *p);
void dramc_mrr_by_rank(DRAMC_CTX_T *p, unsigned char rank,
	unsigned char mr_idx, unsigned short *value_p);
void dramc_mode_reg_read(DRAMC_CTX_T *p, unsigned char mr_idx,
	unsigned short *value);
void dramc_mode_reg_write(DRAMC_CTX_T *p, unsigned char mr_idx,
	unsigned short value);
void dram_phy_reset(DRAMC_CTX_T *p);
unsigned char get_mr4_refresh_rate(DRAMC_CTX_T *p, DRAM_CHANNEL_T channel);
/*  mandatory calibration function */
DRAM_STATUS_T dramc_dqsosc_auto(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_start_dqsosc(DRAMC_CTX_T *p);
DRAM_STATUS_T DramcStopDQSOSC(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_zq_calibration(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_sw_impedance_cal(DRAMC_CTX_T *p,
	unsigned char term_option);

void apply_config_before_calibration(DRAMC_CTX_T *p);
void apply_config_after_calibration(DRAMC_CTX_T *p);
void dramc_rxdqs_gating_pre_process(DRAMC_CTX_T *p);
void dramc_rxdqs_gating_post_process(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_rx_window_perbit_cal(DRAMC_CTX_T *p,
	unsigned char use_test_engine);
void dramc_rxdatlat_cal(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_tx_window_perbit_cal(DRAMC_CTX_T *p,
	DRAM_TX_PER_BIT_CALIBRATION_TYTE_T cal_type,
	unsigned char vref_scan_enable);

unsigned short dfs_get_highest_freq(DRAMC_CTX_T *p);

#if ENABLE_RX_TRACKING_LP4
void dramc_rx_input_delay_tracking_init_by_freq(DRAMC_CTX_T *p);
void dramc_rx_input_delay_tracking_init_common(DRAMC_CTX_T *p);
void dramc_rx_input_delay_tracking_hw(DRAMC_CTX_T *p);
#endif

void dramc_hw_gating_init(DRAMC_CTX_T *p);
void dramc_hw_gating_on_off(DRAMC_CTX_T *p, unsigned char on_off);
void dramc_print_hw_gating_status(DRAMC_CTX_T *p, unsigned char channel);

/*  reference function */
DRAM_STATUS_T dramc_rank_swap(DRAMC_CTX_T *p, unsigned char rank);

/*  dump all reg for debug */
DRAM_STATUS_T cmd_bus_training(DRAMC_CTX_T *p);

void mpll_init(void);

void set_channel_number(DRAMC_CTX_T *p);
void set_rank_number(DRAMC_CTX_T *p);
void set_phy_2_channel_mapping(DRAMC_CTX_T *p, unsigned char u1Channel);
unsigned char get_phy_2_channel_mapping(DRAMC_CTX_T *p);
void set_rank(DRAMC_CTX_T *p, unsigned char ucRank);
unsigned char get_rank(DRAMC_CTX_T *p);

void dramc_hw_dqsosc(DRAMC_CTX_T *p);

void dramc_write_dbi_on_off(DRAMC_CTX_T *p, unsigned char onoff);
void dramc_read_dbi_on_off(DRAMC_CTX_T *p, unsigned char onoff);
void dramc_drs(DRAMC_CTX_T *p, unsigned char enable);

/*
 * Macro that implements ceil
 * function for unsigned integers (Test before using!)
 */
#define CEILING(n, v) (((n)%(v)) ? (((n)/(v)) + 1) : ((n)/(v)))

/* For Fix Compiler Error*/
void mt_mem_init(void);
void global_option_init(DRAMC_CTX_T *p);
int init_dram(DRAM_DRAM_TYPE_T dram_type,
	DRAM_CBT_MODE_EXTERN_T dram_cbt_mode_extern,
	DRAM_INFO_BY_MRR_T *DramInfo, unsigned char get_mdl_used);
void ddr_phy_freq_sel(DRAMC_CTX_T *p, DRAM_PLL_FREQ_SEL_T sel);
void dramc_update_impedance_term_2un_term(DRAMC_CTX_T *p);
void cbt_switch_freq(DRAMC_CTX_T *p, unsigned char freq);
void dramc_sw_impedance_save_register(DRAMC_CTX_T *p,
	unsigned char ca_term_option, unsigned char dq_term_option,
	unsigned char save_to_where);
DRAM_STATUS_T dramc_dual_rank_rxdatlat_cal(DRAMC_CTX_T *p);
void auto_refresh_switch(DRAMC_CTX_T *p, unsigned char option);
void dramc_mode_reg_write_by_rank(DRAMC_CTX_T *p, unsigned char rank,
	unsigned char mr_idx, unsigned short value);
void set_mrr_pinmux_mapping(DRAMC_CTX_T *p, unsigned char option);
DRAM_STATUS_T dramc_write_leveling(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_rx_dqs_gating_cal(DRAMC_CTX_T *p);
void dramc_ac_timing_optimize(DRAMC_CTX_T *p);
void dramc_dump_debug_info(DRAMC_CTX_T *p);
void set_vcore_by_freq(DRAMC_CTX_T *p);
unsigned int get_dramc_broadcast(void);
void dramc_broadcast_on_off(unsigned int on_off);
void dramc_backup_registers(DRAMC_CTX_T *p, unsigned int *backup_addr,
	unsigned int backup_num);
void dramc_restore_registers(DRAMC_CTX_T *p, unsigned int *restore_addr,
	unsigned int restore_num);
void init_global_variables_by_condition(void);
void set_dram_mode_reg_for_write_dbi_on_off(DRAMC_CTX_T *p,
	unsigned char onoff);
void dramc_write_minus_1mck_for_write_dbi(DRAMC_CTX_T *p,
	signed char shift_ui);
void print_calibration_result(DRAMC_CTX_T *p);
void apply_write_dbi_power_improve(DRAMC_CTX_T *p, unsigned char onoff);
void dramc_cmd_bus_training_post_process(DRAMC_CTX_T *p);
unsigned char get_pre_miock_jmeter_hqa_used_flag(void);
void dramc_miock_jmeter_hqa(DRAMC_CTX_T *p);
DRAM_CBT_MODE_T get_dram_cbt_mode(DRAMC_CTX_T *p);
void dramc_gating_mode(DRAMC_CTX_T *p, GW_MODE_TYPE_T mode);
void move_dramc_tx_dq_oen(DRAMC_CTX_T *p, unsigned char byte_dx,
	signed char shift_ui);
void no_parking_on_clrpll(DRAMC_CTX_T *p);
void dramc_tx_set_vref(DRAMC_CTX_T *p, unsigned char vref_range,
	unsigned char vref_value);
DRAM_STATUS_T dramc_dqsosc_mr23(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_dqsosc_set_mr18_mr19(DRAMC_CTX_T *p);
DRAM_STATUS_T dramc_dqsosc_shu_settings(DRAMC_CTX_T *p);
void dramc_save_to_shuffle_reg(DRAMC_CTX_T *p, DRAM_DFS_SHUFFLE_TYPE_T src_rg,
			   DRAM_DFS_SHUFFLE_TYPE_T dst_rg);
void dfs_init_for_calibration(DRAMC_CTX_T *p);
void cbt_dramc_dfs_direct_jump(DRAMC_CTX_T *p, unsigned char shu_level);
DRAM_STATUS_T dramc_mr_init_lp4(DRAMC_CTX_T *p);
void dramc_mr_init_lp3(DRAMC_CTX_T *p);
void dramc_mr_init_ddr3(DRAMC_CTX_T *p);
void dramc_mr_init_ddr4(DRAMC_CTX_T *p);
void dramc_new_duty_calibration(DRAMC_CTX_T *p);
void dramc_engine2_set_pat(DRAMC_CTX_T *p, unsigned char testaudpat,
	unsigned char log2loopcount,
			unsigned char Use_Len1_Flag);
void dramc_tx_oe_calibration(DRAMC_CTX_T *p);
void ta2_test_run_time_hw(DRAMC_CTX_T *p);
unsigned char *dramc_get_4bitmux_byte_mapping(DRAMC_CTX_T *p);

void o1_path_on_off(DRAMC_CTX_T *p, unsigned char on_off);
void print_calibration_basic_info(DRAMC_CTX_T *p);
void set_calibration_result(DRAMC_CTX_T *p, unsigned char cal_type,
	unsigned char result);
DRAM_STATUS_T execute_move_dramc_delay(DRAMC_CTX_T *p,
	REG_TRANSFER_T regs[], signed char shift_ui);

#if DVFS_EN
void dramc_dvfs_switch(DRAMC_CTX_T *p, unsigned int freq_sel);
void dramc_dvfs_init(DRAMC_CTX_T *p);
void dramc_save_cal_settings(DRAMC_CTX_T *p);
void dramc_restore_cal_settings(DRAMC_CTX_T *p);
void dramc_dump_cal_settings(DRAMC_CTX_T *p);
extern unsigned int dvfs_lp4_cal_setting_addr[DVFS_LP4_ADDR_NUM];
extern unsigned int dvfs_lp4_high_freq_cal_setting[DVFS_LP4_ADDR_NUM];
extern unsigned int dvfs_lp4_low_freq_cal_setting[DVFS_LP4_ADDR_NUM];
#endif


/*  External references */
extern void emi_init(DRAMC_CTX_T *p);
extern void emi_init2(void);
extern void check_ddr_reserve_status(void);

#ifdef ETT
extern unsigned int ett_drv[4]; //OCDP, OCDN, ODTP, ODTN
#if (SUPPORT_TYPE_LPDDR4 || SUPPORT_TYPE_PCDDR4)
unsigned int ett_rx_vref, ett_tx_vref;
#endif

#if SUPPORT_TYPE_LPDDR3
extern unsigned int ett_cbt_win[CATRAINING_NUM_LP3];
#elif SUPPORT_TYPE_LPDDR4
extern unsigned int ett_cbt_win[CATRAINING_NUM_LP4];
#endif
extern unsigned int ett_rx_min, ett_tx_min;
#if (SUPPORT_PCDDR3_32BIT || SUPPORT_TYPE_LPDDR3)
extern unsigned int ett_rx_win[DATA_WIDTH_32BIT], ett_rx_first[DATA_WIDTH_32BIT], ett_rx_last[DATA_WIDTH_32BIT];
extern unsigned int ett_tx_win[DATA_WIDTH_32BIT], ett_tx_first[DATA_WIDTH_32BIT], ett_tx_last[DATA_WIDTH_32BIT];
#else
extern unsigned int ett_rx_win[DATA_WIDTH_16BIT], ett_rx_first[DATA_WIDTH_16BIT], ett_rx_last[DATA_WIDTH_16BIT];
extern unsigned int ett_tx_win[DATA_WIDTH_16BIT], ett_tx_first[DATA_WIDTH_16BIT], ett_tx_last[DATA_WIDTH_16BIT];
#endif
#endif

#if ENABLE_DDRPHY_FREQ_METER
extern unsigned int DDRPhyFreqMeter(unsigned int clksrc);
#endif
extern MR_SET_VALUE_T dram_mr;
extern DRAM_DFS_FREQUENCY_TABLE_T freq_tbl[DRAM_DFS_SHUFFLE_MAX];
extern unsigned char print_mode_reg_write;

extern signed int ca_train_clk_delay[CHANNEL_NUM][RANK_MAX];
extern unsigned int ca_train_cs_delay[CHANNEL_NUM][RANK_MAX];
extern signed int ca_train_cmd_delay[CHANNEL_NUM][RANK_MAX];
/*  LPDDR DQ -> PHY DQ mapping */
#if SUPPORT_TYPE_LPDDR4
extern unsigned char lpddr4_phy_mapping_pop[CHANNEL_NUM][16];
extern unsigned char lpddr4_4bitmux_byte_mapping[DATA_WIDTH_16BIT];
#endif

#if SUPPORT_TYPE_LPDDR3
extern unsigned char lpddr3_phy_mapping_pop[CHANNEL_NUM][32];
extern unsigned char lpddr3_4bitmux_byte_mapping[DATA_WIDTH_32BIT];
#endif
extern signed int wl_final_delay[CHANNEL_NUM][DQS_NUMBER];
extern short rx_dqs_duty_offset[CHANNEL_NUM][DQS_NUMBER][EDGE_NUMBER];
#if SIMULATION_WRITE_LEVELING
extern unsigned char wrlevel_done[CHANNEL_NUM];
#endif
#if GATING_ADJUST_TXDLY_FOR_TRACKING
extern unsigned char tx_dly_cal_min[CHANNEL_NUM];
extern unsigned char tx_dly_cal_max[CHANNEL_NUM];
#endif
#if DRAMC_MODEREG_CHECK
void dramc_mode_reg_check(DRAMC_CTX_T *p);
#endif
#endif /* _PI_API_H */

/*--------------------------------------------------------------*/
/*		DRAMC_COMMON.H				*/
/*--------------------------------------------------------------*/

#ifndef _DRAMC_COMMON_H_
#define _DRAMC_COMMON_H_

#include <platform/typedefs.h>
#include <platform/dramc_api.h>
#include <platform/dramc_register.h>

#if (FOR_DV_SIMULATION_USED == 0)
#include <debug.h>
#include <kernel/thread.h>
#include <lk/init.h>
#include <platform.h>
#include <reg.h>
#include <platform/mtk_timer.h>
#include <platform/emi.h>
#endif

/*------------------------------------------------------------*/
/*                  macros, defines, typedefs, enums          */
/*------------------------------------------------------------*/
/************************** Common Macro *********************/
#if (FOR_DV_SIMULATION_USED == 1) /* TBA used API */
#define dsb()
#define dprintf(level, fmt, args...)		\
	do { \
		if ((level) > 0) \
			printf(fmt, ## args); \
	} while (0)

#define show_msg_with_timestamp(_x_) \
	do { \
		dprintf _x_; \
		show_current_time(); \
	} while (0)

#define INFO			0
#define CRITICAL		0

#define delay_ms(x)		delay_us(1)
#else /* REAL chip API */
//#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")

#define delay_us(x)       udelay(x)
#define delay_ms(x)       mdelay(x)

#undef INFO
#define INFO			0

#define show_current_time()
#define show_msg_with_timestamp()
/* #define dprintf */
#endif /* end of FOR_DV_SIMULATION_USED */

/**********************************************/
/* Priority of debug log				*/
/*--------------------------------------------*/
/* show_msg: High                       */
/* show_msg2: Medium High               */
/* show_msg3: Medium Low                */
/**********************************************/
#define show_log(_x_)   dprintf(INFO, _x_)
#define show_msg(_x_)   dprintf _x_ /* Calibration Result */
#if CALIBRATION_LOG
#define show_msg2(_x_)  dprintf _x_ /* Calibration Sequence */
#else
#define show_msg2(_x_) /* Calibration Sequence */
#endif

#ifdef ETT
#define show_msg3(_x_)  dprintf _x_   /* Calibration detail "ooxx" */
#else
#define show_msg3(_x_)
#endif

#define show_err(_x_)   dprintf(CRITICAL, _x_)
#define show_err2(_x_, _y_)   dprintf(CRITICAL, _x_, _y_)
#define show_err3(_x_, _y_, _z_)   dprintf(CRITICAL, _x_, _y_, _z_)

#define USE_PMIC_CHIP_MT6355	0

#endif


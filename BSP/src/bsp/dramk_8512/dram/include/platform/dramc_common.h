//
//  DRAMC_COMMON.H
//

#ifndef _DRAMC_COMMON_H_
#define _DRAMC_COMMON_H_

#include "dramc_register.h"
#include "DRAMC_CH0_Regs.h"
#include "DRAMC_CH0_NAO_Regs.h"
#include "DDRPHY_NAO_Regs.h"
#include "DDRPHY_Regs.h"
#include "dramc_api.h"
#ifndef __ETT__
#if (FOR_DV_SIMULATION_USED==0)
#include <platform/mtk_timer.h>
#include <debug.h>
#endif
#endif

#if __ETT__
    #include <common.h>
#endif

#if PSRAM_SPEC
#include "PSRAMC.h"
#include "PSRAMC_NAO.h"
#endif

/***********************************************************************/
/*                  Public Types                                       */
/***********************************************************************/
#if SW_CHANGE_FOR_SIMULATION || __FLASH_TOOL_DA__
typedef signed char     INT8;
typedef signed short    INT16;
typedef signed int      INT32;
typedef signed char     S8;
typedef signed short    S16;
typedef signed int      S32;
typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned char       BOOL;
#define FALSE   0
#define TRUE    1
#endif
/*------------------------------------------------------------*/
/*                  macros, defines, typedefs, enums          */
/*------------------------------------------------------------*/
/************************** Common Macro *********************/
#if __ETT__
#if ((!defined(_WIN32)) && (FOR_DV_SIMULATION_USED==0) && (SW_CHANGE_FOR_SIMULATION==0))
#else
//#undef dsb
//#define dsb()
#endif
#else
#if (FOR_DV_SIMULATION_USED==0)
#define dsb()
#endif
#endif

// K2?? : The following needs to be porting.
// choose a proper mcDELAY
#if __ETT__
#if defined(DUMP_INIT_RG_LOG_TO_DE)
#define mcDELAY_US(x)       if (gDUMP_INIT_RG_LOG_TO_DE_RG_log_flag) mcSHOW_DUMP_INIT_RG_MSG("DELAY_US(%d);\n",x); GPT_Delay_us((U32) (x))
#define mcDELAY_MS(x)       GPT_Delay_ms((U32) (x))
#elif (FOR_DV_SIMULATION_USED==0)
#define mcDELAY_US(x)       GPT_Delay_us((U32) (x))
#define mcDELAY_MS(x)       GPT_Delay_ms((U32) (x))
#else
#define mcDELAY_US(x)       GPT_Delay_us((U32) (1))
#define mcDELAY_MS(x)       GPT_Delay_ms((U32) (1))
#endif
#else
//====
  #if __FLASH_TOOL_DA__
    #define mcDELAY_US(x)       gpt4_busy_wait_us(x)
    #define mcDELAY_MS(x)       gpt4_busy_wait_us(x*1000)
  #else
        #if (FOR_DV_SIMULATION_USED==1)
            #define mcDELAY_US(x)       delay_us(1)
            #define mcDELAY_MS(x)       delay_us(1)
        #elif (SW_CHANGE_FOR_SIMULATION==1)
            #define mcDELAY_US(x)       *MDM_TM_WAIT_US = x; while (*MDM_TM_WAIT_US>0);
            #define mcDELAY_MS(x)       *MDM_TM_WAIT_US = x; while (*MDM_TM_WAIT_US>0);
        #else
            #define mcDELAY_US(x)       udelay(x)
            #define mcDELAY_MS(x)       mdelay(x)
        #endif
#endif
#endif

/**********************************************/
/* Priority of debug log                      */
/*--------------------------------------------*/
/* mcSHOW_DBG_MSG: High                       */
/* mcSHOW_DBG_MSG2: Medium High               */
/* mcSHOW_DBG_MSG3: Medium Low                */
/* mcSHOW_DBG_MSG4: Low                       */
/**********************************************/
#if __FLASH_TOOL_DA__
  #define printf LOGD
  #define print LOGD
#endif

#undef  DIAG
#define DIAG
//#undef  ENABLE_FULL_CALIB_LOG
//#define ENABLE_FULL_CALIB_LOG

#if defined(ENABLE_FULL_CALIB_LOG)
  #define LOG_LEVEL3
#elif defined(DIAG)
  #define LOG_LEVEL2
#else
  #define LOG_LEVEL1
#endif

#if __ETT__
    #if defined(DDR_INIT_TIME_PROFILING)
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)   {opt_print _x_;}
    #define mcSHOW_ERR_MSG(_x_)
    #elif defined(RELEASE)
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)  //{opt_print _x_;}
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)    {opt_print _x_;}
    #define mcSHOW_EYESCAN_MSG(_x_) {opt_print _x_;}   //mcSHOW_JV_LOG_MSG(_x_) is for vendor JV
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)
    #elif VENDER_JV_LOG
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)    {opt_print _x_;}   //mcSHOW_JV_LOG_MSG(_x_) is for vendor JV
    #define mcSHOW_EYESCAN_MSG(_x_) {opt_print _x_;}   //mcSHOW_JV_LOG_MSG(_x_) is for vendor JV
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)
    #elif FOR_DV_SIMULATION_USED
    #define mcSHOW_DBG_MSG0(_x_)  {printf _x_;}
    #define mcSHOW_DIAG_MSG(_x_)  {printf _x_;}
    #define mcSHOW_DBG_MSG(_x_)   {printf _x_;}
    #define mcSHOW_DBG_MSG2(_x_)  {printf _x_;}
    #define mcSHOW_DBG_MSG3(_x_)  {printf _x_;}
    //#define mcSHOW_DBG_MSG4(_x_)  if (RXPERBIT_LOG_PRINT) {printf _x_;}
    #define mcSHOW_DBG_MSG4(_x_)  {printf _x_;}
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)   {printf _x_;}
    #elif SW_CHANGE_FOR_SIMULATION
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)
    #elif defined(DUMP_INIT_RG_LOG_TO_DE)
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_DUMP_INIT_RG_MSG(_x_) opt_print _x_;
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)
    #elif MRW_CHECK_ONLY
    #define mcSHOW_DBG_MSG0(_x_)
    #define mcSHOW_DIAG_MSG(_x_)
    #define mcSHOW_DBG_MSG(_x_)
    #define mcSHOW_DBG_MSG2(_x_)
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_MRW_MSG(_x_)    {printf _x_;}
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)
    #else   // ETT real chip
    #define mcSHOW_DBG_MSG0(_x_, ...)   {mcDELAY_US(10);opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_DIAG_MSG(_x_, ...)   {mcDELAY_US(10);opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_DBG_MSG(_x_, ...)   {mcDELAY_US(10);opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_DBG_MSG2(_x_, ...)  {mcDELAY_US(10); opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_DBG_MSG3(_x_, ...)  {mcDELAY_US(10); opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_DBG_MSG4(_x_, ...)  {mcDELAY_US(10); opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_JV_LOG_MSG(_x_, ...)
    #define mcSHOW_EYESCAN_MSG(_x_, ...) {if (gEye_Scan_color_flag) {mcDELAY_US(200);}; opt_print(_x_, ##__VA_ARGS__);}
    #define mcSHOW_TIME_MSG(_x_, ...)
    #define mcSHOW_ERR_MSG(_x_, ...)   {printf("\033[1;31m%s[%d]: ", __FUNCTION__,__LINE__); opt_print(_x_, ##__VA_ARGS__); printf("\033[0m");}
    #endif

#else  // preloader
    #if defined(DDR_INIT_TIME_PROFILING)
    #define mcSHOW_DBG_MSG0(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_DIAG_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_DBG_MSG(_x_,...)			//dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_DBG_MSG2(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_DBG_MSG3(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_DBG_MSG4(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_JV_LOG_MSG(_x_,...)      //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_EYESCAN_MSG(_x_,...)
    #define mcSHOW_TIME_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_INFO_MSG(_x_,...)		dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #define mcSHOW_ERR_MSG(_x_,...)			dprintf(ALWAYS, _x_, ##__VA_ARGS__)
    #elif(TARGET_BUILD_VARIANT_ENG) //&& !defined(MTK_EFUSE_WRITER_SUPPORT) && !CFG_TEE_SUPPORT && !MTK_EMMC_SUPPORT
    #define mcSHOW_DBG_MSG0(_x_) {printf _x_;}
    #define mcSHOW_DIAG_MSG(_x_) {printf _x_;}
    #define mcSHOW_DBG_MSG(_x_)   {print _x_;}
    #define mcSHOW_DBG_MSG2(_x_)  {print _x_;}
    #define mcSHOW_DBG_MSG3(_x_)
    #define mcSHOW_DBG_MSG4(_x_)
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)   {print _x_;}
    #elif FOR_DV_SIMULATION_USED
    #define mcSHOW_DBG_MSG0(_x_) {printf _x_;}
    #define mcSHOW_DIAG_MSG(_x_) {printf _x_;}
    #define mcSHOW_DBG_MSG(_x_)   {printf _x_;}
    #define mcSHOW_DBG_MSG2(_x_)  {printf _x_;}
    #define mcSHOW_DBG_MSG3(_x_)  {printf _x_;}
    #define mcSHOW_DBG_MSG4(_x_)  {printf _x_;}
    #define mcSHOW_JV_LOG_MSG(_x_)
    #define mcSHOW_EYESCAN_MSG(_x_)
    #define mcSHOW_TIME_MSG(_x_)
    #define mcSHOW_ERR_MSG(_x_)   {printf("\033[1;31m%s[%d]: ", __FUNCTION__,__LINE__); printf _x_; printf("\033[0m");}
    #else
      #if defined(LOG_LEVEL1)
        #define mcSHOW_DBG_MSG0(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DIAG_MSG(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_MRW_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG(_x_,...)         //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG2(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG3(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG4(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_JV_LOG_MSG(_x_,...)      //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_EYESCAN_MSG(_x_,...)     //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_TIME_MSG(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_INFO_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_ERR_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
      #elif defined(LOG_LEVEL2)
        #define mcSHOW_DBG_MSG0(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DIAG_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_MRW_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG(_x_,...)         //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG2(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG3(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG4(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_JV_LOG_MSG(_x_,...)      //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_EYESCAN_MSG(_x_,...)     //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_TIME_MSG(_x_,...)        //dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_INFO_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_ERR_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
      #elif defined(LOG_LEVEL3)
        #define mcSHOW_DBG_MSG0(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DIAG_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_MRW_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG2(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG3(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_DBG_MSG4(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_JV_LOG_MSG(_x_,...)      dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_EYESCAN_MSG(_x_,...)     dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_TIME_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_INFO_MSG(_x_,...)        dprintf(ALWAYS, _x_, ##__VA_ARGS__)
        #define mcSHOW_ERR_MSG(_x_,...)         dprintf(ALWAYS, _x_, ##__VA_ARGS__)
      #endif
    #endif
#endif

#define mcFPRINTF(_x_, fmt, ...)
//#define mcFPRINTF(_x_)          fprintf _x_;

#define USE_PMIC_CHIP_MT6358	1

#ifndef __ETT__
#undef LOG_E
#define LOG_E	dprintf(ALWAYS, "[%s:%s@%d] +\n", __FILE__, __func__, __LINE__);

#undef LOG_X
#define LOG_X	dprintf(ALWAYS, "[%s:%s@%d] -\n", __FILE__, __func__, __LINE__);

#undef LOG_HERE
#define LOG_HERE	dprintf(ALWAYS, "[%s:%s@%d]\n", __FILE__, __func__, __LINE__);

#undef LOG_H
#define LOG_H 		LOG_HERE
#endif

#define SAFE_POINTER(p) \
{       \
    if ((p) == NULL)    \
        (p) = "";   \
}

extern int dump_log;
#endif   // _DRAMC_COMMON_H_

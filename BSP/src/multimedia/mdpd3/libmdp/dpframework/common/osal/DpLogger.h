#ifndef __DP_LOGGER_H__
#define __DP_LOGGER_H__

#include "DpConfig.h"

#if CONFIG_FOR_OS_WINDOWS
    #include "./windows/logger/DpLogger_Win32.h"
#elif CONFIG_FOR_OS_ANDROID
    #if CONFIG_FOR_ALOG_SUPPORT
    	#include "./android/logger/DpLogger_Android.h"
    #else
        #include "../../../if/DpLogger_Yocto.h"
    #endif
#else
    #error "Unknown operation system!\n"
#endif

#endif  // __DP_LOGGER_H__

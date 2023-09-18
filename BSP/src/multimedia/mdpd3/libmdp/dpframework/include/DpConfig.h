#ifndef __DP_CONFIG_H__
#define __DP_CONFIG_H__

#define CONFIG_FOR_OS_WINDOWS       0

#define CONFIG_FOR_OS_ANDROID       1

#define CONFIG_FOR_ALOG_SUPPORT     0    // must set 0 for yocto project
#define CONFIG_FOR_FENCE_SUPPORT    0    // must set 0 for yocto project
#define CONFIG_FOR_ATRACE_SUPPORT   0    // must set 0 for yocto project
#define CONFIG_FOR_PROPERTY_SUPPORT 0    // must set 0 for yocto project

#if !(CONFIG_FOR_OS_WINDOWS ^ CONFIG_FOR_OS_ANDROID)
    #error "Please specify the correct platform"
#endif

#define CONFIG_FOR_TPIPE_FINFO      0

#define CONFIG_FOR_PROFILE_INFO     0

#define CONFIG_FOR_DUMP_COMMAND     0

#define CONFIG_FOR_FLUSH_RANGE      0

#define CONFIG_FOR_VERIFY_FPGA      0

#define CONFIG_FOR_SYSTRACE         0

#endif  // __DP_CONFIG_H__

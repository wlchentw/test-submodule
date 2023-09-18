#ifndef __PQ_LOGGER_H__
#define __PQ_LOGGER_H__

#include "DpLogger.h"

#undef LOG_TAG
#define LOG_TAG "PQ"

#if CONFIG_FOR_ALOG_SUPPORT
#define PQ_LOGD(fmt, arg...) ALOGD("[PQ]" fmt, ##arg)
#define PQ_LOGE(fmt, arg...) ALOGE("[PQ]" fmt, ##arg)
#ifdef ENABLE_PQ_DEBUG_LOG
#define PQ_LOGI(fmt, arg...) \
    do { \
       ALOGI_IF (__android_log_is_loggable(ANDROID_LOG_VERBOSE, "PQ", ANDROID_LOG_DEBUG), "[PQ]" fmt, ##arg); \
    }while(0)
#else
#define PQ_LOGI(fmt, arg...) do {} while(0)
#endif
#endif

#endif  // __PQ_LOGGER_H__

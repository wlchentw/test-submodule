#define LOG_TAG "PQ"
#define MTK_LOG_ENABLE 1
#include "DpConfig.h"
#if CONFIG_FOR_PROPERTY_SUPPORT
#include <cutils/properties.h>
#endif
#if CONFIG_FOR_ALOG_SUPPORT
#include <cutils/log.h>
#else
#include "PQLogger.h"
#endif
#include <fcntl.h>

#include <PQCommon.h>
#include <PQAlgorithmAdaptor.h>

PQAlgorithmAdaptor::PQAlgorithmAdaptor(ProxyTuningBuffer swreg, ProxyTuningBuffer input, ProxyTuningBuffer output)
{
    PQ_LOGD("[PQAlgorithmAdaptor] PQAlgorithmAdaptor()... ");
    m_swRegBuffer = new PQTuningBuffer(swreg);
    m_inputBuffer = new PQTuningBuffer(input);
    m_outputBuffer = new PQTuningBuffer(output);
};

PQAlgorithmAdaptor::~PQAlgorithmAdaptor()
{
    PQ_LOGD("[PQAlgorithmAdaptor] ~PQAlgorithmAdaptor()... ");
    delete m_swRegBuffer;
    delete m_inputBuffer;
    delete m_outputBuffer;
};


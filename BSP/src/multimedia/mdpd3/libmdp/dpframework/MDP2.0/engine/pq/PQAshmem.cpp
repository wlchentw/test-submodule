#define LOG_TAG "PQ"
#include "PQAshmem.h"
#include <stdlib.h>
#ifndef NO_PQ_SERVICE
#include <hidlmemory/mapping.h>

using vendor::mediatek::hardware::pq::V2_0::Result;
using ::android::hardware::hidl_memory;
#else
#include "PQServiceCommon.h"

#define ASHMEM_MALLOC_SIZE (1024 * 1024)    // need to confirm ashmem size
#endif

PQAshmem* PQAshmem::s_pInstance = NULL;
PQMutex   PQAshmem::s_ALMutex;

PQAshmem* PQAshmem::getInstance()
{
    AutoMutex lock(s_ALMutex);

    if(NULL == s_pInstance)
    {
        s_pInstance = new PQAshmem();
        atexit(PQAshmem::destroyInstance);
    }

    return s_pInstance;
}

void PQAshmem::destroyInstance()
{
    AutoMutex lock(s_ALMutex);

    if (NULL != s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = NULL;
    }
}

PQAshmem::PQAshmem()
    :m_is_ashmem_init(false),
    m_ashmem_base(NULL)
{
    m_ashmem_max_offset = static_cast<uint32_t>(PQAshmemProxy::getAshmemMaxOffset());
}

PQAshmem::~PQAshmem()
{
}

#ifndef NO_PQ_SERVICE
bool PQAshmem::initAshmem(sp<IPictureQuality> service)
#else
bool PQAshmem::initAshmem(void)
#endif
{
    AutoMutex lock(s_ALMutex);

    do {
        if (m_is_ashmem_init == true) {
            break;
        }

#ifndef NO_PQ_SERVICE
        android::hardware::Return<void> ret = service->getAshmem(
            [&] (Result retval, const hidl_memory& memory) {
            if (retval == Result::OK) {
                m_mapAshmem = mapMemory(memory);
                if (m_mapAshmem != NULL) {
                    m_ashmem_base = static_cast<unsigned int*>(static_cast<void*>(m_mapAshmem->getPointer()));
                    m_is_ashmem_init = true;
                }
            }
        });
        if (!ret.isOk()){
            PQ_LOGE("Transaction error in IPictureQuality::getAshmem");
		}
#else
        m_ashmem_base = (unsigned int *)malloc(ASHMEM_MALLOC_SIZE);
        PQ_LOGD("%s[%d] malloc size=0x%x addr=%p\n", __func__, __LINE__, ASHMEM_MALLOC_SIZE, m_ashmem_base);
        m_is_ashmem_init = true;

        *(m_ashmem_base + SHP_ENABLE) = 0; //SHP_ENABLE
        *(m_ashmem_base + DSHP_ENABLE) = 0; //DSHP_ENABLE
        *(m_ashmem_base + DC_ENABLE) = 0; //DC_ENABLE
        *(m_ashmem_base + MDP_DRE_ENABLE) = 1; //MDP_DRE_ENABLE
        PQ_LOGD("%s[%d] init SHP_ENABLE / DSHP_ENABLE / DC_ENABLE / MDP_DRE_ENABLE\n", __func__, __LINE__);
#endif
    } while (0);

    return m_is_ashmem_init;
}

unsigned int *PQAshmem::getAshmemBase(void)
{
    if (m_is_ashmem_init == true)
    {
        return m_ashmem_base;
    }
    else
    {
        return NULL;
    }
}

int32_t PQAshmem::setAshmemValueByOffset(uint32_t offset, int32_t value)
{
    AutoMutex lock(s_ALMutex);

    if (m_is_ashmem_init == false) {
        PQ_LOGD("[PQAshmem] setAshmemValueByOffset : Ashmem is not ready\n");
        return -1;
    }

    if (offset > m_ashmem_max_offset) {
        PQ_LOGD("[PQAshmem] setAshmemValueByOffset : offset (0x%08x) is invalid\n", (int)(offset*sizeof(uint32_t)));
        return -1;
    }

#ifndef NO_PQ_SERVICE
    m_mapAshmem->update();
    *(m_ashmem_base + offset) = value;
    m_mapAshmem->commit();
#else
    *(m_ashmem_base + offset) = value;
#endif
    PQ_LOGI("[PQAshmem] setAshmemValueByOffset : set offset 0x%08x = 0x%08x\n", (int)(offset*sizeof(uint32_t)), value);

    return 0;
}

int32_t PQAshmem::getAshmemValueByOffset(uint32_t offset, int32_t *value)
{
    AutoMutex lock(s_ALMutex);

    if (m_is_ashmem_init == false) {
        PQ_LOGD("[PQAshmem] getAshmemValueByOffset : Ashmem is not ready\n");
        return -1;
    }

    if (offset > m_ashmem_max_offset) {
        PQ_LOGD("[PQAshmem] getAshmemValueByOffset : offset (0x%08x) is invalid\n", (int)(offset*sizeof(uint32_t)));
        return -1;
    }

    // get field value
#ifndef NO_PQ_SERVICE
    m_mapAshmem->read();
    *value = *(m_ashmem_base + offset);
    m_mapAshmem->commit();
#else
    *value = *(m_ashmem_base + offset);
#endif
    PQ_LOGI("[PQAshmem] getAshmemValueByOffset : get offset 0x%08x = 0x%08x\n", (int)(offset*sizeof(uint32_t)), *value);

    return 0;
}

int32_t PQAshmem::setAshmemValueByAddr(unsigned int *addr, int32_t value)
{
    AutoMutex lock(s_ALMutex);

    if (m_is_ashmem_init == false) {
        PQ_LOGD("[PQAshmem] setAshmemValueByAddr : Ashmem is not ready\n");
        return -1;
    }

    if (addr > (m_ashmem_base + m_ashmem_max_offset) || addr < m_ashmem_base) {
        PQ_LOGD("[PQAshmem] setAshmemValueByAddr : addr (%p) is invalid; m_ashmem_base(%p); m_ashmem_max_offset(%d)\n", addr, m_ashmem_base, m_ashmem_max_offset);
        return -1;
    }

#ifndef NO_PQ_SERVICE
    m_mapAshmem->update();
    *(addr) = value;
    m_mapAshmem->commit();
#else
    *(addr) = value;
#endif
    PQ_LOGI("[PQAshmem] setAshmemValueByAddr : set addr %p = 0x%08x\n", addr, value);

    return 0;
}

int32_t PQAshmem::getAshmemValueByAddr(unsigned int *addr, int32_t *value)
{
    AutoMutex lock(s_ALMutex);

    if (m_is_ashmem_init == false) {
        PQ_LOGD("[PQAshmem] getAshmemValueByAddr : Ashmem is not ready\n");
        return -1;
    }

    if (addr > (m_ashmem_base + m_ashmem_max_offset) || addr < m_ashmem_base) {
        PQ_LOGD("[PQAshmem] getAshmemValueByAddr : addr (%p) is invalid;m_ashmem_base(%p); m_ashmem_max_offset(%d)\n", addr, m_ashmem_base, m_ashmem_max_offset);
        return -1;
    }

    // get field value
#ifndef NO_PQ_SERVICE
    m_mapAshmem->read();
    *value = *(addr);
    m_mapAshmem->commit();
#else
    *value = *(addr);
#endif
    PQ_LOGI("[PQAshmem] getAshmemValueByAddr : get addr %p = 0x%08x\n", addr, *value);

    return 0;
}

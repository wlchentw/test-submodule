#include "DpMmuHandler.h"
#include "DpEngineType.h"
#include "DpTimer.h"
#include "m4u_port_map.h"

#if defined(MTK_M4U_SUPPORT)
MTKM4UDrv* DpMmuHandler::s_pM4uDrv = NULL;
#else
#include <sys/ioctl.h>
#include <ion/ion.h>
#include <linux/ion.h>
#include <linux/ion_mtk.h>
int32_t DpMmuHandler::s_IONHandle = -1;
#endif //MTK_M4U_SUPPORT

DpMutex    DpMmuHandler::s_instMutex;

DpMmuHandler::DpMmuHandler(void     *pBase,
                           uint32_t size)
    : m_selfAlloc(false),
      m_selfMapMVA(true),
      m_memorySize(size),
      m_memoryVA(pBase),
      m_memoryPA(0),
      m_engineType(tNone),
      //m_outputPort(-1),
      m_curPlane(-1)
{
    DPLOGI("DpMmuHandler: create MMU handler\n");

    assert(0 != size);

#if defined(MTK_M4U_SUPPORT)
    if (s_pM4uDrv == NULL)
    {
        AutoMutex lock(s_instMutex);
        if (s_pM4uDrv == NULL)
        {
            s_pM4uDrv = new MTKM4UDrv();
            assert(NULL != s_pM4uDrv);
            atexit(destroyInstance);
        }
    }
#else
    DpTimeValue begin;
    DpTimeValue end;
    int32_t     diff;

    if (s_IONHandle < 0)
    {
        AutoMutex lock(s_instMutex);
        if (s_IONHandle < 0)
        {
            DP_TIMER_GET_CURRENT_TIME(begin);

            s_IONHandle = ion_open();

            DP_TIMER_GET_CURRENT_TIME(end);
            DP_TIMER_GET_DURATION_IN_MS(begin,
                                        end,
                                        diff);

            if (diff > 10)
            {
                DPLOGW("DpMmuHandler: open ION handle %d ms\n", diff);
            }

            DPLOGI("DpMmuHandler: Created ION file handle 0x%08x\n", s_IONHandle);
            atexit(destroyInstance);
        }
    }
#endif //MTK_M4U_SUPPORT
}


DpMmuHandler::DpMmuHandler(uint32_t MVABase,
                           uint32_t size)
    : m_selfAlloc(false),
      m_selfMapMVA(false),
      m_memorySize(size),
      m_memoryVA(NULL),
      m_memoryPA(MVABase),
      m_engineType(tNone),
      //m_outputPort(-1),
      m_curPlane(-1)
{
    DPLOGI("DpMmuHandler: Create MMU handler(MVA): 0x%08x\n", MVABase);

    assert(0 != size);

#if defined(MTK_M4U_SUPPORT)
    if (s_pM4uDrv == NULL)
    {
        AutoMutex lock(s_instMutex);
        if (s_pM4uDrv == NULL)
        {
            s_pM4uDrv = new MTKM4UDrv();
            assert(NULL != s_pM4uDrv);
            atexit(destroyInstance);
        }
    }
#else
    DpTimeValue begin;
    DpTimeValue end;
    int32_t     diff;

    if (s_IONHandle < 0)
    {
        AutoMutex lock(s_instMutex);
        if (s_IONHandle < 0)
        {
            DP_TIMER_GET_CURRENT_TIME(begin);

            s_IONHandle = ion_open();

            DP_TIMER_GET_CURRENT_TIME(end);
            DP_TIMER_GET_DURATION_IN_MS(begin,
                                        end,
                                        diff);

            if (diff > 10)
            {
                DPLOGW("DpMmuHandler: open ION handle %d ms\n", diff);
            }

            DPLOGI("DpMmuHandler: Created ION file handle 0x%08x\n", s_IONHandle);
            atexit(destroyInstance);
        }
    }
#endif
}


DpMmuHandler::DpMmuHandler(void     *pVABase,
                           uint32_t MVABase,
                           uint32_t size)
    : m_selfAlloc(false),
      m_selfMapMVA(false),
      m_memorySize(size),
      m_memoryVA(pVABase),
      m_memoryPA(MVABase),
      m_engineType(tNone),
      //m_outputPort(-1),
      m_curPlane(-1)
{
    DPLOGI("DpMmuHandler: Create MMU handler(VA + MVA): VA = 0x%p, MVA = 0x%08x\n", pVABase, MVABase);

    assert(0 != size);

#if defined(MTK_M4U_SUPPORT)
    if (s_pM4uDrv == NULL)
    {
        AutoMutex lock(s_instMutex);
        if (s_pM4uDrv == NULL)
        {
            s_pM4uDrv = new MTKM4UDrv();
            assert(NULL != s_pM4uDrv);
            atexit(destroyInstance);
        }
    }
#else
    DpTimeValue begin;
    DpTimeValue end;
    int32_t     diff;

    if (s_IONHandle < 0)
    {
        AutoMutex lock(s_instMutex);
        if (s_IONHandle < 0)
        {
            DP_TIMER_GET_CURRENT_TIME(begin);

            s_IONHandle = ion_open();

            DP_TIMER_GET_CURRENT_TIME(end);
            DP_TIMER_GET_DURATION_IN_MS(begin,
                                        end,
                                        diff);

            if (diff > 10)
            {
                DPLOGW("DpMmuHandler: open ION handle %d ms\n", diff);
            }

            DPLOGI("DpMmuHandler: Created ION file handle 0x%08x\n", s_IONHandle);
            atexit(destroyInstance);
        }
    }
#endif
}


DpMmuHandler::~DpMmuHandler()
{
    M4U_MODULE_ID_ENUM module;

    DPLOGI("DpMmuHandler: destroy MMU handler\n");

    if(0 != m_memoryPA && (true == m_selfMapMVA))
    {
        module = convertType(m_engineType, m_curPlane);

        DPLOGI("DpMmuHandler: invalid M4U TLB range, engine = %d, plane = %d, MVAddr = %x, size = %x\n",
               m_engineType, m_curPlane, m_memoryPA, m_memorySize);

#if defined(MTK_M4U_SUPPORT)
    #if 0
        s_pM4uDrv->m4u_invalid_tlb_range(module,
                                         m_memoryPA,
                                         m_memoryPA + m_memorySize - 1);
    #endif

        s_pM4uDrv->m4u_dealloc_mva(module,
                                   (unsigned long)m_memoryVA,
                                   m_memorySize,
                                   m_memoryPA);
#endif //MTK_M4U_SUPPORT

        m_memoryPA = 0;
    }

    DPLOGI("DpMmuHandler: delete M4U driver object\n");

    if (true == m_selfAlloc)
    {
        // Free the allocated buffer
        free(m_memoryVA);
        m_memoryVA = NULL;
    }
}


void DpMmuHandler::destroyInstance()
{
    AutoMutex lock(s_instMutex);

#if defined(MTK_M4U_SUPPORT)
    delete s_pM4uDrv;
    s_pM4uDrv = NULL;
#else
    DpTimeValue        begin;
    DpTimeValue        end;
    int32_t            diff;

    if (s_IONHandle >= 0)
    {
        DP_TIMER_GET_CURRENT_TIME(begin);

        ion_close(s_IONHandle);

        DP_TIMER_GET_CURRENT_TIME(end);
        DP_TIMER_GET_DURATION_IN_MS(begin,
                                    end,
                                    diff);

        if (diff > 10)
        {
            DPLOGW("DpMmuHandler: close ION handle %d ms\n", diff);
        }

        DPLOGI("DpMmuHandler: Destroyed ION file handle 0x%08x\n", s_IONHandle);
        s_IONHandle = -1;
    }
#endif //MTK_M4U_SUPPORT
}


uint32_t DpMmuHandler::queryFeature(void)
{
    return (FEATURE_FLUSH_WHOLE_CACHE |
            FEATURE_FLUSH_BUFFER_RANGE);
}


uint32_t DpMmuHandler::mapHWAddress(DpEngineType type,
                                    uint32_t     plane)
{
    M4U_MODULE_ID_ENUM module;
    //M4U_PORT_ID_ENUM   port;
    DpTimeValue    begin;
    DpTimeValue    end;
    int32_t        diff;

    if (NULL == m_memoryVA && (true == m_selfMapMVA))
    {
        DPLOGI("DpMmuHandler: allocate memory buffer\n");

        m_memoryVA = malloc(sizeof(uint8_t) * m_memorySize);
        m_selfAlloc = true;
    }

    DPLOGI("DpMmuHandler: try mapping physical address: %d, %d\n", type, plane);

    if ((m_engineType != type) ||
        (m_curPlane   != plane))
    {
        if ((0 != m_memoryPA) &&
            (m_engineType != type) &&
            (true == m_selfMapMVA))

        {
            DPLOGI("DpMmuHandler: engine type changed\n");

            module = convertType(m_engineType, m_curPlane);

            DPLOGI("DpMmuHandler: invalid M4U TLB range, engine = %d, plane = %d, MVAddr = %x, size = %x\n",
                   m_engineType, m_curPlane, m_memoryPA, m_memorySize);

#if defined(MTK_M4U_SUPPORT)
        #if 0
            s_pM4uDrv->m4u_invalid_tlb_range(module,
                                             m_memoryPA,
                                             m_memoryPA + m_memorySize - 1);
        #endif

            s_pM4uDrv->m4u_dealloc_mva(module,
                                       (unsigned long)m_memoryVA,
                                       m_memorySize,
                                       m_memoryPA);
#else
            DPLOGE("DpMmuHandler: mapHWAddress m4u_dealloc_mva not support\n");
#endif //MTK_M4U_SUPPORT

            // Reset HW address to zero
            m_memoryPA = 0;
        }

        DPLOGI("DpMmuHandler: M4U enable function\n");
        module = convertType(type, plane);

        DP_TIMER_GET_CURRENT_TIME(begin);
        if (true == m_selfMapMVA)
        {
            DPLOGI("DpMmuHandler: M4U - get MVA param: Module = %d, VAddr = %p, Size = %d\n", module, m_memoryVA, m_memorySize);

#if defined(MTK_M4U_SUPPORT)
            uint32_t rwflag = M4U_PROT_READ|M4U_PROT_WRITE;

            if(M4U_STATUS_OK != s_pM4uDrv->m4u_alloc_mva(module,
                                                         (unsigned long)m_memoryVA,
                                                         m_memorySize,
                                                         rwflag,
                                                         /*M4U_FLAGS_SEQ_ACCESS*/ 0,
                                                         &m_memoryPA))
            {
                DPLOGE("DpMmuHandler: can't allocate mva\n");
                m_memoryPA = 0;
                return 0;
            }
#else
            DPLOGE("DpMmuHandler: mapHWAddress m4u_alloc_mva not support\n");
#endif //MTK_M4U_SUPPORT
        }

        DP_TIMER_GET_CURRENT_TIME(end);
        DP_TIMER_GET_DURATION_IN_MS(begin, end, diff);
        if (25 < diff)
        {
            DPLOGD("DpMmuHandler::mapHWAddress: m4u_alloc_mva time %d ms\n", diff);
        }

        DPLOGI("M: MVA:0x%08x, S:%d\n", m_memoryPA, m_memorySize);

    #if 0
        port = convertPort(type, plane);
        if (m_outputPort != port)
        {
            m_outputPort = (int32_t)port;

            M4U_PORT_STRUCT m4uPort;
            m4uPort.ePortID = port;
            m4uPort.Virtuality = 1;
            m4uPort.Security = 0;
            m4uPort.domain = 3;
            m4uPort.Distance = 1;
            m4uPort.Direction = 0;
            if(M4U_STATUS_OK != s_pM4uDrv->m4u_config_port(&m4uPort))
            {
                DPLOGE("DpMmuHandler: can't config m4u port\n");
                m_memoryPA = 0;
                return 0;
            }
        }
    #endif

        DPLOGI("DpMmuHandler: M4U - get MVA = %x (ori: %p)\n", m_memoryPA, m_memoryVA);

    #if 0
        if (true == m_selfMapMVA)
        {
            if(M4U_STATUS_OK != s_pM4uDrv->m4u_insert_tlb_range(module,
                                                                m_memoryPA,
                                                                m_memoryPA + m_memorySize - 1,
                                                                RT_RANGE_HIGH_PRIORITY,
                                                                1))
            {
                s_pM4uDrv->m4u_dealloc_mva(module,
                                           (uint32_t)m_memoryVA,
                                           m_memorySize,
                                           m_memoryPA);

                DPLOGE("DpMmuHandler: can't insert tlb range\n");
                m_memoryPA = 0;
                return NULL;
            }
        }
    #endif

        m_engineType = type;
        m_curPlane   = plane;
    }

    return m_memoryPA;
}


void* DpMmuHandler::mapSWAddress()
{
    if (NULL == m_memoryVA && (true == m_selfMapMVA))
    {
        DPLOGI("DpMmuHandler: allocate memory buffer\n");

        m_memoryVA = malloc(sizeof(uint8_t) * m_memorySize);
        m_selfAlloc = true;
        DPLOGI("DpMmuHandler: Allocated VA address: %p\n", m_memoryVA);
    }

    return m_memoryVA;
}

bool DpMmuHandler::flushMemory(FLUSH_TYPE_ENUM type,
                               int32_t,
                               int32_t)
{
    M4U_MODULE_ID_ENUM module;
    DpTimeValue        begin;
    DpTimeValue        end;
    int32_t            diff;

    DP_TIMER_GET_CURRENT_TIME(begin);

#if defined(MTK_M4U_SUPPORT)

    module = convertType(m_engineType, m_curPlane);

    switch (type)
    {
        case FLUSH_BEFORE_HW_READ_WHOLE_CACHE:
            DPLOGI("DpMmuHandler: flush with M4U_CACHE_FLUSH_ALL\n");
            //s_pM4uDrv->m4u_cache_sync(module,
            //                          M4U_CACHE_FLUSH_ALL,
            //                          (uint32_t)m_memoryVA,
            //                          m_memorySize, m_memoryPA);
            s_pM4uDrv->m4u_cache_flush_all(module);
            break;
        case FLUSH_BEFORE_HW_READ_BUFFER_RANGE:
            DPLOGI("DpMmuHandler: flush with M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM\n");
            if (NULL == m_memoryVA)
            {
                DPLOGE("DpMmuHandler: flush memory failed: VA is NULL\n");
                return false;
            }
            #ifdef MDP_VERSION_USE_TABLET_M4U
                s_pM4uDrv->m4u_cache_sync(module,
                              M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM,
                              (unsigned long)m_memoryVA,
                              m_memorySize);
            #else
                s_pM4uDrv->m4u_dma_map_area(module,
                                            M4U_DMA_TO_DEVICE,
                                            (unsigned long)m_memoryVA,
                                            m_memorySize,
                                            m_memoryPA);
            #endif
            break;
        case FLUSH_AFTER_HW_WRITE_BUFFER_RANGE:
            DPLOGI("DpMmuHandler: flush with M4U_CACHE_INVALID_AFTER_HW_WRITE_MEM\n");
            if (NULL == m_memoryVA)
            {
                DPLOGE("DpMmuHandler: flush memory failed: VA is NULL\n");
                return false;
            }
            #ifdef MDP_VERSION_USE_TABLET_M4U
                s_pM4uDrv->m4u_cache_sync(module,
                                          M4U_CACHE_INVALID_AFTER_HW_WRITE_MEM,
                                          (unsigned long)m_memoryVA,
                                          m_memorySize);
            #else
                s_pM4uDrv->m4u_dma_unmap_area(module,
                                              M4U_DMA_FROM_DEVICE,
                                              (unsigned long)m_memoryVA,
                                              m_memorySize,
                                              m_memoryPA);
            #endif
            break;
        default:
            return false;
    }
#else
    DP_UNUSED(type);
    int ret;
	struct ion_sys_data sys_data;

	sys_data.sys_cmd = ION_SYS_DMA_OP;
	sys_data.dma_param.dma_type = ION_DMA_CACHE_FLUSH_ALL;
	ret = ion_custom_ioctl(s_IONHandle, ION_CMD_SYSTEM, &sys_data);
    if (0 != ret)
    {
        DPLOGW("DpMmuHandler: flush ION memory failed");
    }
#endif //MTK_M4U_SUPPORT

    DP_TIMER_GET_CURRENT_TIME(end);
    DP_TIMER_GET_DURATION_IN_MS(begin,
                                end,
                                diff);
    if (diff > 10)
    {
        DPLOGW("DpMmuHandler: flush addr %d ms\n", diff);
    }

    return true;
}

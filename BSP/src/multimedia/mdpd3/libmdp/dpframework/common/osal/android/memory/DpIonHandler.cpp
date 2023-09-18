#include "DpIonHandler.h"
#include "DpEngineType.h"
#include "DpTimer.h"
#include "m4u_port_map.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <ion/ion.h>
#include <linux/ion.h>
#include <linux/ion_mtk.h>
#include <sys/mman.h>

int32_t    DpIonHandler::s_IONHandle = -1;
DpMutex    DpIonHandler::s_instMutex;

DpIonHandler::DpIonHandler(void     *pBase,
                           uint32_t size)
    : m_cacheAble(true),
      m_selfAlloc(false),
      m_selfMapMVA(true),
      m_allocHandle(0),
      m_shareFD(-1),
      m_memorySize(size),
      m_memoryVA(pBase),
      m_memoryPA(0),
      m_engineType(tNone),
      //m_outputPort(-1),
      m_curPlane(-1)
{
    DpTimeValue begin;
    DpTimeValue end;
    int32_t     diff;

    DPLOGI("DpIonHandler: create ION handler begin\n");

    assert(0 != size);

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
                DPLOGW("DpIonHandler: open ION handle %d ms\n", diff);
            }

            DPLOGI("DpIonHandler: Created ION file handle 0x%08x\n", s_IONHandle);
            atexit(destroyInstance);
        }
    }

    if (s_IONHandle < 0)
    {
        DPLOGE("DpIonHandler: open ION device failed\n");
    }
}

DpIonHandler::DpIonHandler(int32_t  FD,
                           uint32_t size,
                           bool     cache,
                           int32_t  *pInternalFD)
    : m_cacheAble(cache),
      m_selfAlloc(false),
      m_selfMapMVA(false),
      m_allocHandle(0),
      m_shareFD(-1),
      m_memorySize(size),
      m_memoryVA(0),
      m_memoryPA(0),
      m_engineType(tNone),
      //m_outputPort(-1),
      m_curPlane(-1)
{
    bool        status;
    DpTimeValue begin;
    DpTimeValue end;
    int32_t     diff;

    DPLOGI("DpIonHandler: create ION handler begin\n");

    assert(0 != size);

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
                DPLOGW("DpIonHandler: open ION handle %d ms\n", diff);
            }

            DPLOGI("DpIonHandler: Created ION file handle 0x%08x\n", s_IONHandle);
            atexit(destroyInstance);
        }
    }

    if (s_IONHandle < 0)
    {
        DPLOGE("DpIonHandler: open ION device failed\n");
    }

    if (-1 == FD)
    {
        DPLOGI("DpIonHandler: allocate ION buffer\n");

        status = alloc_internal(size);
        assert(true == status);

        m_selfAlloc = true;
    }
    else
    {
        DPLOGI("DpIonHandler: import ION buffer\n");

        status = import_internal(FD);
        assert(true == status);
    }

    if (NULL != pInternalFD)
    {
        *pInternalFD = m_shareFD;
    }

    DPLOGI("DpIonHandler: shareFD: %d, size: %d\n", m_shareFD, m_memorySize);
}


DpIonHandler::~DpIonHandler()
{
    DPLOGI("DpIonHandler: destroy ION handler begin\n");

#if 0
    if(m_memoryPA != 0)
    {
        M4U_MODULE_ID_ENUM module = convertType(m_engineType, m_curPlane);

        DPLOGI("DpIonHandler: invalidate M4U TLB range, engine = %d, plane = %d, MVAddr = %p, size = %p\n",
               m_engineType, m_curPlane, m_memoryPA, m_memorySize);

        m_pM4uDrv->m4u_invalid_tlb_range(module,
                                         m_memoryPA,
                                         m_memoryPA + m_memorySize - 1);
    }
#endif

    if(m_memoryVA != NULL && m_selfMapMVA == false)
    {
        DPLOGI("DpIonHandler: unmap ION virtual address\n");
        munmap(m_memoryVA, m_memorySize);
    }

    DPLOGI("DpIonHandler: free ION allocated buffer\n");
    free_internal();

    DPLOGI("DpIonHandler: destroy ION handler end\n");
}


void DpIonHandler::destroyInstance()
{
    DpTimeValue        begin;
    DpTimeValue        end;
    int32_t            diff;

    AutoMutex lock(s_instMutex);

    DPLOGI("DpIonHandler: close ION handle 0x%08x\n", s_IONHandle);
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
            DPLOGW("DpIonHandler: close ION handle %d ms\n", diff);
        }

        DPLOGI("DpIonHandler: Destroyed ION file handle 0x%08x\n", s_IONHandle);
        s_IONHandle = -1;
    }
    DPLOGI("DpIonHandler: close ION handler end\n");
}


uint32_t DpIonHandler::queryFeature(void)
{
    return (FEATURE_FLUSH_WHOLE_CACHE |
            FEATURE_FLUSH_BUFFER_RANGE);
}


bool DpIonHandler::alloc_internal(uint32_t size)
{
    int ion_prot_flags = m_cacheAble ? ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC : 0;
    if (ion_alloc(s_IONHandle, size, 0x40, ION_HEAP_MULTIMEDIA_MASK, ion_prot_flags, &m_allocHandle)) //For M4U cache alignment
    {
        DPLOGE("DpIonHandler: alloc memory buffer failed\n");
        return false;
    }

    if (ion_share(s_IONHandle, m_allocHandle, &m_shareFD))
    {
        DPLOGE("DpIonHandler: map file descriptor failed\n");
    }

    return true;
}


bool DpIonHandler::import_internal(int32_t FD)
{
    if (ion_import(s_IONHandle, FD, &m_allocHandle))
    {
        DPLOGE("DpIonHandler: import memory by fd(%d) failed\n", FD);
        return false;
    }

    if (ion_share(s_IONHandle, m_allocHandle, &m_shareFD))
    {
        DPLOGE("DpIonHandler: map file descriptor failed\n");
    }

    return true;
}


void DpIonHandler::free_internal()
{
    DPLOGI("DpIonHandler: close ION share FD\n");

    if (s_IONHandle >= 0 && m_shareFD >= 0)
    {
        if (close(m_shareFD))
        {
            DPLOGE("DpIonHandler: close share FD failed\n");
        }
    }

    DPLOGI("DpIonHandler: free allocate handle\n");

    if (s_IONHandle >= 0 && m_allocHandle > 0)
    {
        if (ion_free(s_IONHandle, m_allocHandle))
        {
            DPLOGE("DpIonHandler: free memory failed\n");
        }
    }
}


uint32_t DpIonHandler::mapHWAddress(DpEngineType type,
                                    uint32_t     plane)
{
    M4U_MODULE_ID_ENUM module;
    //M4U_PORT_ID_ENUM   port;
    DpTimeValue        begin;
    DpTimeValue        end;
    int32_t            diff;

    DPLOGI("DpIonHandler: try mapping physical address: %d, %d\n", type, plane);

    DP_TIMER_GET_CURRENT_TIME(begin);

    if ((m_engineType != type) ||
        (m_curPlane   != plane))
    {
        if((m_memoryPA   != 0) &&
           (m_engineType != type))
        {
        #if 0
            module = convertType(m_engineType, m_curPlane);

            DPLOGI("DpIonHandler: invalidate M4U TLB range, engine = %d, plane = %d, MVAddr = %p, size = %p\n",
                   m_engineType, m_curPlane, m_memoryPA, m_memorySize);

            m_pM4uDrv->m4u_invalid_tlb_range(module,
                                             m_memoryPA,
                                             m_memoryPA + m_memorySize - 1);

            DPLOGI("DpIonHandler: reset mva and release M4U TLB\n");
            //pM4uDrv->m4u_reset_mva_release_tlb(module);
        #endif

            if (true == m_selfMapMVA)
            {
                if( s_IONHandle >= 0 && m_allocHandle > 0)
                {
                    if (ion_free(s_IONHandle, m_allocHandle))
                    {
                        DPLOGE("DpIonHandler: free memory failed\n");
                    }
                }
                m_allocHandle = 0;
            }

            m_memoryPA = 0;
        }

        if (0 == m_memoryPA)
        {
            if (true == m_selfMapMVA)
            {
                if (ion_alloc(s_IONHandle, m_memorySize, (unsigned long)m_memoryVA, ION_HEAP_MULTIMEDIA_MAP_MVA_MASK, 3, &m_allocHandle))
                {
                    DPLOGE("DpIonHandler: mapHWAddress ion_alloc failed, virtual address: %p\n", m_memoryVA);
                    return 0;
                }
            }

            module = convertType(type, plane);

            //pM4uDrv->m4u_enable_m4u_func(module);

            // Buffer must be configured before querying physical address
            struct ion_mm_data mm_data;
            int32_t            status;
            mm_data.mm_cmd = ION_MM_CONFIG_BUFFER;
            mm_data.config_buffer_param.handle = m_allocHandle;
            mm_data.config_buffer_param.security = 0;
            mm_data.config_buffer_param.coherent = 0;
            DPLOGI("DpIonHandler: configs: Handler = %d, module = %d\n", m_allocHandle, module);

            status = ion_custom_ioctl(s_IONHandle, ION_CMD_MULTIMEDIA, &mm_data);
            if((status > 0) && (status != ION_ERROR_CONFIG_LOCKED))
            {
                DPLOGE("DpIonHandler: config memory buffer failed\n");
                return 0;
            }

            // Get physical address
            struct ion_sys_data sys_data;
            sys_data.sys_cmd = ION_SYS_GET_PHYS;
            sys_data.get_phys_param.handle = m_allocHandle;
            if(ion_custom_ioctl(s_IONHandle, ION_CMD_SYSTEM, &sys_data))
            {
                DPLOGE("DpIonHandler: get physical address failed, virtual address: %p\n", m_memoryVA);
                return 0;
            }

            m_memoryPA = sys_data.get_phys_param.phy_addr;
            DPLOGI("I: MVA:0x%08x, S:%d\n", m_memoryPA, m_memorySize);

        #if 0
            port = convertPort(type, plane);
            if (m_outputPort != port)
            {
                m_outputPort= (int32_t)port;

                M4U_PORT_STRUCT m4uPort;
                m4uPort.ePortID = port;
                m4uPort.Virtuality = 1;
                m4uPort.Security = 0;
                m4uPort.Distance = 1;
                m4uPort.Direction = 0;
                //m4uPort.Domain = 3;
                if(M4U_STATUS_OK != m_pM4uDrv->m4u_config_port(&m4uPort))
                {
                    // Reset to zero
                    m_memoryPA = 0;

                    DPLOGE("DpIonHandler: config the M4U port failed\n");
                    return 0;
                }
            }
        #endif

        #if 0
            if(M4U_STATUS_OK != m_pM4uDrv->m4u_insert_tlb_range(module,
                                                                m_memoryPA,
                                                                m_memoryPA + m_memorySize - 1,
                                                                RT_RANGE_HIGH_PRIORITY,
                                                                1))
            {
                // Reset to zero
                m_memoryPA = 0;

                DPLOGE("DpIonHandler: can't insert buffer to M4U TLB\n");
                return NULL;
            }
        #endif // 0
        }
        else
        {
        #if 0
            module = convertType(type, plane);

            //pM4uDrv->m4u_enable_m4u_func(module);

            DPLOGI("DpIonHandler: config MVA %p for plane %d %p\n", m_memoryPA, plane);

            port = convertPort(type, plane);
            if (m_outputPort != port)
            {
                m_outputPort = (int32_t)port;

                M4U_PORT_STRUCT m4uPort;
                m4uPort.ePortID = convertPort(type, plane);
                m4uPort.Virtuality = 1;
                m4uPort.Security = 0;
                m4uPort.Distance = 1;
                m4uPort.Direction = 0;
                //m4uPort.Domain = 3;
                if(M4U_STATUS_OK != m_pM4uDrv->m4u_config_port(&m4uPort))
                {
                    // Reset to zero
                    m_memoryPA = 0;

                    DPLOGE("DpIonHandler: config the M4U port failed\n");
                    return 0;
                }
            }
        #endif
        }

        m_engineType = type;
        m_curPlane   = plane;
    }

    DP_TIMER_GET_CURRENT_TIME(end);

    DP_TIMER_GET_DURATION_IN_MS(begin,
                                end,
                                diff);

    if (diff > 10)
    {
        DPLOGW("DpIonHandler: map HW addr %d ms\n", diff);
    }

    return m_memoryPA;
}


void* DpIonHandler::mapSWAddress()
{
    uint32_t flag;

    if (0 == m_memoryVA)
    {
        flag = PROT_READ | PROT_WRITE;

        // Get virtual address
        m_memoryVA = mmap(NULL, m_memorySize, flag, MAP_SHARED, m_shareFD, 0);
        assert(0 != m_memoryVA);
    }

    DPLOGI("DpIonHandler: mapped virtual address: %p\n", m_memoryVA);

    return m_memoryVA;
}


bool DpIonHandler::flushMemory(FLUSH_TYPE_ENUM type,
                               int32_t,
                               int32_t)
{
    DpTimeValue         begin;
    DpTimeValue         end;
    int32_t             diff;
    int                 ret;
	struct ion_sys_data sys_data;

    if (true == m_cacheAble)
    {
        DP_TIMER_GET_CURRENT_TIME(begin);

        switch (type)
        {
            case FLUSH_BEFORE_HW_READ_WHOLE_CACHE:
                DPLOGI("DpIonHandler: flush read buffer all\n");
				sys_data.sys_cmd = ION_SYS_DMA_OP;

				sys_data.dma_param.dma_type = ION_DMA_CACHE_FLUSH_ALL;
				ret = ion_custom_ioctl(s_IONHandle, ION_CMD_SYSTEM, &sys_data);
                break;
            case FLUSH_BEFORE_HW_READ_BUFFER_RANGE:
                DPLOGI("DpIonHandler: flush read buffer by range\n");
				sys_data.sys_cmd = ION_SYS_DMA_OP;
				sys_data.dma_param.handle = m_allocHandle;

				sys_data.dma_param.dma_type = ION_DMA_MAP_AREA;
				sys_data.dma_param.dma_dir = ION_DMA_TO_DEVICE;
				ret = ion_custom_ioctl(s_IONHandle, ION_CMD_SYSTEM, &sys_data);
                break;
            case FLUSH_AFTER_HW_WRITE_BUFFER_RANGE:
                DPLOGI("DpIonHandler: flush write buffer by invalid by range\n");
				sys_data.sys_cmd = ION_SYS_DMA_OP;
				sys_data.dma_param.handle = m_allocHandle;

				sys_data.dma_param.dma_type = ION_DMA_UNMAP_AREA;
				sys_data.dma_param.dma_dir = ION_DMA_FROM_DEVICE;
				ret = ion_custom_ioctl(s_IONHandle, ION_CMD_SYSTEM, &sys_data);
                break;
            default:
                return false;
        }

        if (0 != ret)
        {
            DPLOGE("DpIonHandler: flush ION memory failed");
        }

        DP_TIMER_GET_CURRENT_TIME(end);
        DP_TIMER_GET_DURATION_IN_MS(begin,
                                    end,
                                    diff);
        if (diff > 10)
        {
            DPLOGW("DpIonHandler: flush addr %d ms\n", diff);
        }
    }

    return true;
}

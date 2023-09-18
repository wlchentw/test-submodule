#ifndef __M4U_PORT_MAP_H__
#define __M4U_PORT_MAP_H__

#include "DpLogger.h"
#if defined(MTK_M4U_SUPPORT)
#include <m4u_lib.h>
#else
#include "mt_iommu_port.h"
#endif //MTK_M4U_SUPPORT

static M4U_MODULE_ID_ENUM convertType(DpEngineType type,
                                      int32_t)
{
    switch (type)
    {
        case tRDMA0:
            return M4U_PORT_MDP_RDMA0;

        case tWROT0:
            return M4U_PORT_MDP_WROT0;

        default:
            DPLOGE("DpMmuHandler: unknown engine type: %d\n", type);
            assert(0);
            break;
    }

    return M4U_PORT_UNKNOWN;
}

static M4U_PORT_ID_ENUM convertPort(DpEngineType type,
                                    int32_t)
{
    switch (type)
    {
        case tRDMA0:
            return M4U_PORT_MDP_RDMA0;


        case tWROT0:
            return M4U_PORT_MDP_WROT0;

        default:
            DPLOGE("DpMmuHandler: unknown engine type: %d\n", type);
            assert(0);
            break;
    }

    return M4U_PORT_UNKNOWN;
}

#endif  // __M4U_PORT_MAP_H__

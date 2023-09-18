#include "DpPathTopology.h"

const DpEngineType DpPathTopology::s_topology[tTotal] =
{
    tRDMA0, tGAMMA, tDTH, tSCL0, tTDSHP0, tWROT0, tNone
};

const int32_t DpPathTopology::s_engOrder[tTotal] =
{
    0, 1, 2, 3, 4, 5, -1
};

const bool DpPathTopology::s_adjency[tTotal][tTotal] =
{
/*            RDMA0 GAMMA  DTH  RSZ0  TDSHP0 WROT0 */
/*0 RDMA0 */ {  0,    1,    1,    1,    1,     1 },
/*1 GAMMA */ {  0,    0,    1,    0,    0,     1 },
/*2 DTH   */ {  0,    0,    0,    0,    0,     1 },
/*3 RSZ0  */ {  0,    0,    0,    0,    1,     1 },
/*4 TDSHP0*/ {  0,    0,    0,    0,    0,     1 },
};

DpPathTopology::DpPathTopology()
{
}


DpPathTopology::~DpPathTopology()
{
}


bool DpPathTopology::sortPathInfo(DpEngineType source,
                                  DpEngineType target,
                                  PathInfo     &info,
                                  uint32_t     length)
{
    uint32_t     index;
    int32_t      inner;
    DpEngineType temp;
    int32_t      order;

    for (index = 1; index <= (length - 1); index++)
    {
        temp  = info[index];
        order = s_engOrder[temp];
        inner = index - 1;

        while ((inner >= 0) && (order < s_engOrder[info[inner]]))
        {
            info[inner + 1] = info[inner];
            inner = inner - 1;
        }

        info[inner + 1] = temp;
    }

    if ((source != info[0]) ||
        (target != info[length - 1]))
    {
        return false;
    }

    return true;
}


DP_STATUS_ENUM DpPathTopology::getEngUsages()
{
    DP_STATUS_ENUM status;
    char           *module_name;
    int32_t        i;

    status = DpDriver::getInstance()->queryEngUsages(m_engUsages);

    assert(DP_STATUS_RETURN_SUCCESS == status);

    for (i = tRDMA0; i < tTotal; i++)
    {
        DP_GET_ENGINE_NAME(i, module_name);
        DPLOGI("MDP modules( %s) usage: %d\n", module_name, m_engUsages[i]);
    }

    return DP_STATUS_RETURN_SUCCESS;
}

bool DpPathTopology::connectEdge(DpEngineType startPoint,
                                 DpEngineType endPoint,
                                 PathInfo     &dataPath)
{
    int32_t         current;
    DpEngineType    nodeA;
    DpEngineType    nodeB;
    int32_t         source;
    int32_t         target;
    uint32_t        weight;

    target = s_engOrder[endPoint];

    for (source = s_engOrder[startPoint]; source <= target; source++)
    {
        m_distance[topology(source)] = 0;
    }

    for (source = s_engOrder[startPoint]; source <= target; source++)
    {
        for (current = (source + 1); current <= target; ++current)
        {
            nodeA = topology(source),
            nodeB = topology(current);

            if (isConnect(nodeA, nodeB))
            {
                weight = LOAD_BALANCE_BY_ENG_USAGE ? getWeight(nodeA, nodeB) : 1;
                if ((0 == m_distance[nodeB])
                    || (((m_distance[nodeA] + weight) < m_distance[nodeB]) && (dataPath[nodeA] != tNone)))
                {
                    m_distance[nodeB] = m_distance[nodeA] + weight;
                    dataPath[nodeB] = nodeA;
                }
            }
        }
    }

    return true;
}

uint32_t DpPathTopology::getWeight(DpEngineType source,
                                   DpEngineType target)
{
    if ((target < tRDMA0) ||
        (target >= tTotal) ||
        (target >= CMDQ_MAX_ENGINE_COUNT))
    {
        DPLOGE("DpPathTopology: connect engine path failed\n");
        return DP_STATUS_INVALID_PARAX;
    }
    return m_engUsages[target] + 1;
}


bool DpPathTopology::connectPath(PathInfo &engInfo,
                                 int32_t  length,
                                 uint32_t &engFlag,
                                 PathInfo &dataPath)
{
    bool    status;
    int32_t index;

    dataPath[engInfo[0]] = engInfo[0];

    for (index = 0; index < (length - 1); index++)
    {
        // Record the engine is required
        engFlag |= (1 << engInfo[index]);

        status = connectEdge(engInfo[index + 0],
                             engInfo[index + 1],
                             dataPath);
        if (false == status)
        {
            return status;
        }
    }

    engFlag |= (1 << engInfo[length - 1]);

    return true;
}


static bool needResizer(uint32_t    numOutputPort,
                        int32_t     sourceCropWidth,
                        int32_t     sourceCropWidthSubpixel,
                        int32_t     sourceCropHeight,
                        int32_t     sourceCropHeightSubpixel,
                        int32_t     sourceCropSubpixX,
                        int32_t     sourceCropSubpixY,
                        int32_t     targetWidth,
                        int32_t     targetHeight)
{
    bool needResizer = true;

    DPLOGI("DpPathTopology: Width  %d %d %d\n", sourceCropWidth, sourceCropWidthSubpixel, targetWidth);
    DPLOGI("DpPathTopology: Height %d %d %d\n", sourceCropHeight, sourceCropHeightSubpixel, targetHeight);
    DPLOGI("DpPathTopology: CropSubpix %d %d\n", sourceCropSubpixX, sourceCropSubpixY);

    if ((sourceCropWidth == targetWidth) &&
        (sourceCropHeight == targetHeight) &&
        (sourceCropWidthSubpixel == 0) &&
        (sourceCropHeightSubpixel == 0) &&
        (sourceCropSubpixX == 0) &&
        (sourceCropSubpixY == 0))
    {
        needResizer = false;
    }

    DPLOGI("DpPathTopology: needResizer %d\n", needResizer);

    return needResizer;
}

DP_STATUS_ENUM DpPathTopology::getPathInfo(STREAM_TYPE_ENUM scenario,
                                           DpPortAdapt      &sourcePort,
                                           DpPortAdapt      &targetPort,
                                           uint32_t         &engFlag,
                                           PathInfo         &pathInfo,
                                           DpEngineType     &sourceEng,
                                           DpEngineType     &targetEng,
                                           uint32_t         numOutputPort,
                                           uint32_t)
{
    int32_t        rotation;
    bool           flipStatus;
    PORT_TYPE_ENUM portType;
    PathInfo       engInfo;
    uint32_t       engCount;
    DP_STATUS_ENUM status;
    DpColorFormat  sourceFormat;
    int32_t        sourceCropXOffset;
    int32_t        sourceCropYOffset;
    int32_t        sourceCropWidth;
    int32_t        sourceCropWidthSubpixel;
    int32_t        sourceCropHeight;
    int32_t        sourceCropHeightSubpixel;
    int32_t        sourceCropSubpixX;
    int32_t        sourceCropSubpixY;
    bool           sourceDitherStatus;
    DpColorFormat  targetFormat;
    int32_t        targetWidth;
    int32_t        targetHeight;
    int32_t        dummy;
    bool needRsz = false;

    status     = targetPort.getPortInfo(&targetFormat,
                                        &targetWidth,
                                        &targetHeight,
                                        &dummy,
                                        &dummy,
                                        0);
    if (DP_STATUS_RETURN_SUCCESS != status)
    {
        DPLOGE("DpPathTopology: query output port format failed\n");
        return status;
    }

    rotation   = targetPort.getRotation();
    if (rotation == 90 || rotation == 270)
    {
        int32_t temp = targetWidth;
        targetWidth = targetHeight;
        targetHeight = temp;
    }

    status = sourcePort.getSourceCrop(&sourceCropXOffset,
                                      &sourceCropSubpixX,
                                      &sourceCropYOffset,
                                      &sourceCropSubpixY,
                                      &sourceCropWidth,
                                      &sourceCropWidthSubpixel,
                                      &sourceCropHeight,
                                      &sourceCropHeightSubpixel);
    if (DP_STATUS_RETURN_SUCCESS != status)
    {
        DPLOGE("DpPathTopology: query input port crop info failed\n");
        return status;
    }

    needRsz = needResizer(numOutputPort, sourceCropWidth, sourceCropWidthSubpixel, sourceCropHeight, sourceCropHeightSubpixel, sourceCropSubpixX, sourceCropSubpixY, targetWidth, targetHeight);

    if ((needRsz || targetPort.getTDSHPStatus()) &&
		(targetPort.getGammaStatus() || targetPort.getInvertStatus() ||
	 		targetPort.getDthStatus()))
    {
	    DPLOGE("DpPathTopology: RSZ(%s)/TDSHP(%s) path conflict with GAMMA(%s)/DTH(%s) path!\n",
		needRsz ? "true" : "false",
		targetPort.getTDSHPStatus() ? "true" : "false",
		(targetPort.getGammaStatus() | targetPort.getInvertStatus()) ? "true" : "false",
		targetPort.getDthStatus() ? "true" : "false");
	    return DP_STATUS_INVALID_PATH;
    }

    DPLOGI("DpPathTopology: RSZ(%s)/TDSHP(%s)/GAMMA(%s)/DTH(%s) path!\n",
	needRsz ? "true" : "false",
	targetPort.getTDSHPStatus() ? "true" : "false",
	(targetPort.getGammaStatus() | targetPort.getInvertStatus()) ? "true" : "false",
	targetPort.getDthStatus() ? "true" : "false");

    engCount  = 0;

    sourceEng = tRDMA0;
    targetEng = tWROT0;

    engInfo[engCount] = sourceEng;
    engCount++;

    if ((tNone == sourceEng) ||
        (tNone == targetEng))
    {
        DPLOGE("DpPathTopology: unknown source or target engine\n");
        return DP_STATUS_INVALID_ENGINE;
    }

    if (needRsz == true)
    {
        engInfo[engCount] = tSCL0;
        engCount++;
    }

    if (targetPort.getTDSHPStatus())
    {
	//workaround: add rsz if src crop and tdshp are enabled
	if (needRsz == false && (sourceCropXOffset != 0 || sourceCropYOffset != 0))
	{
		engInfo[engCount] = tSCL0;
		engCount++;
	}
        engInfo[engCount] = tTDSHP0;
        engCount++;
    }

    if (targetPort.getGammaStatus() || targetPort.getInvertStatus())
    {
        engInfo[engCount] = tGAMMA;
        engCount++;
    }

    if (targetPort.getDthStatus())
    {
        engInfo[engCount] = tDTH;
        engCount++;
    }

    if (needRsz == false && targetPort.getTDSHPStatus() == 0 &&
	targetPort.getGammaStatus() == 0 && targetPort.getInvertStatus() == 0 &&
	targetPort.getDthStatus() == 0 &&
	(sourceCropXOffset != 0 || sourceCropYOffset != 0))
    {
        engInfo[engCount] = tSCL0;
        engCount++;
    }

    engInfo[engCount] = targetEng;
    engCount++;

    if (false == sortPathInfo(sourceEng,
                              targetEng,
                              engInfo,
                              engCount))
    {
        DPLOGE("DpPathTopology: invalid engine path info\n");
        assert(0);
        return DP_STATUS_INVALID_PATH;
    }

    if (false == connectPath(engInfo,
                             engCount,
                             engFlag,
                             pathInfo))
    {
        DPLOGE("DpPathTopology: connect engine path failed\n");
        assert(0);
        return DP_STATUS_INVALID_PATH;
    }

    return DP_STATUS_RETURN_SUCCESS;
}

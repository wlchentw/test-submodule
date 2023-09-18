#include "DpTileEngine.h"
#include "DpEngineType.h"
#include "mdp_reg_tdshp.h"
#if CONFIG_FOR_VERIFY_FPGA
#include "ConfigInfo.h"
#endif

#if CONFIG_FOR_OS_ANDROID
#if CONFIG_FOR_PROPERTY_SUPPORT
#include <cutils/properties.h>
#endif
#include "DpLogger.h"
#include "cust_color.h"
#include "cust_tdshp.h"

#include "PQSessionManager.h"
#include <PQAlgorithmFactory.h>

#define _PQ_TDSHP_DEBUG_
//#define _PQ_ADL_DEBUG_
#define _PQ_DSHP_DEBUG_
//#define _PQ_TIME_DEBUG_

// define to print log for performance issue.
#define TDSHP_CONFIG_FRAME_THRES    ( 2)  // ms
//#define TDSHP_CALC_ADL_THRES        (  1)  // ms
//#define TDSHP_IOCTL_THRES           (  2)  // ms
//#define TDSHP_GET_PROP_THRES        (  1)  // ms

#endif // CONFIG_FOR_OS_ANDROID

//--------------------------------------------------------
// TDSHP driver engine
//--------------------------------------------------------
class DpEngine_TDSHP: public DpTileEngine
{
public:
    DpEngine_TDSHP(uint32_t identifier)
        : DpTileEngine(identifier),
          m_outHistXLeft(0),
          m_outHistYTop(0),
          m_lastOutVertical(0),
          m_lastOutHorizontal(0),
          m_prevPABufferIndex(0)
    {
        m_enableLog = DpDriver::getInstance()->getEnableLog();
        memset(m_regLabel, -1, sizeof(m_regLabel));
    }

    ~DpEngine_TDSHP()
    {
    }

private:
    int32_t        m_outHistXLeft;
    int32_t        m_outHistYTop;
    int32_t        m_lastOutVertical;
    int32_t        m_lastOutHorizontal;

    int32_t        m_regLabel[MAX_NUM_READBACK_PA_BUFFER];

    uint32_t       m_prevPABufferIndex;

    DP_STATUS_ENUM onInitEngine(DpCommand&);

    DP_STATUS_ENUM onDeInitEngine(DpCommand&);

    DP_STATUS_ENUM onConfigFrame(DpCommand&,
                                 DpConfig&);

    DP_STATUS_ENUM onReconfigFrame(DpCommand&,
                                   DpConfig&);

    DP_STATUS_ENUM onConfigTile(DpCommand&);

    DP_STATUS_ENUM onPostProc(DpCommand &command);

    DP_STATUS_ENUM onAdvanceTile(DpCommand&);

    int64_t onQueryFeature()
    {
        return eTDSHP;
    }

#if CONFIG_FOR_OS_ANDROID
    // TDSHP
    void resetSWReg(void);

    int32_t         m_enableLog;
#else

#endif // CONFIG_FOR_OS_ANDROID

    // ADL
    void ADL_resetLumaHist(DpCommand &command);

    DP_STATUS_ENUM onReconfigTiles(DpCommand &command);
};

// Register factory function
static DpEngineBase* TDSHP0Factory(DpEngineType type)
{
    if (tTDSHP0 == type)
    {
        return new DpEngine_TDSHP(0);
    }
    return NULL;
};

// Register factory function
EngineReg TDSHP0Reg(TDSHP0Factory);

DP_STATUS_ENUM DpEngine_TDSHP::onInitEngine(DpCommand &command)
{
    // Enable TDSHP
    //MM_REG_WRITE(command, MDP_TDSHP_00, 0x80000000, 0x80000000); // TDS_EN will be enabled by following steps.
    MM_REG_WRITE(command, MDP_TDSHP_CTRL, 0x00000001, 0x00000001);
    command.addMetLog("MDP_TDSHP__MDP_TDSHP_CTRL", 0x00000001);

    // Enable fifo
    MM_REG_WRITE(command, MDP_TDSHP_CFG, 0x00000002, 0x00000002);

    // reset LumaHist
    ADL_resetLumaHist(command);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_TDSHP::onDeInitEngine(DpCommand &command)
{
    // Disable fifo
    MM_REG_WRITE(command, MDP_TDSHP_CFG, 0x00000002, 0x00000002);

    // Disable TDSHP
    //MM_REG_WRITE(command, MDP_TDSHP_00, 0x00000000, 0x80000000);
    MM_REG_WRITE(command, MDP_TDSHP_CTRL, 0x00000000, 0x00000001);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_TDSHP::onConfigFrame(DpCommand &command, DpConfig &config)
{
#if CONFIG_FOR_OS_ANDROID
    bool shp_enable = false;
    bool adl_enable = false;
    DpTimeValue    begin;
    DpTimeValue    end;
    int32_t        diff;

    DP_TIMER_GET_CURRENT_TIME(begin);

    DPLOGI("DpEngine_TDSHP: pqSessionId = %lux \n", config.pqSessionId);

    PQSession* pPQSession = PQSessionManager::getInstance()->getPQSession(config.pqSessionId);

    if (pPQSession != NULL)
    {
        PQAlgorithmFactory* pPQAlgorithmFactory = PQAlgorithmFactory::getInstance();

        PQDCAdaptor* pPQDCAdaptor = pPQAlgorithmFactory->getDynamicContrast(m_identifier);
        adl_enable = pPQDCAdaptor->calRegs(pPQSession, command, config, m_frameConfigLabel);
        DPLOGI("DpEngine_TDSHP: configFrame() adl_enable = %d\n",adl_enable);

        PQDSAdaptor* pPQDSAdaptor = pPQAlgorithmFactory->getDynamicSharpness(m_identifier);
        shp_enable = pPQDSAdaptor->calRegs(pPQSession, command, config);
        DPLOGI("DpEngine_TDSHP: configFrame() shp_enable = %d\n",shp_enable);

        if (shp_enable == false &&
            adl_enable == false)
        {
            m_bypassEngine = true;
            MM_REG_WRITE(command, MDP_TDSHP_CFG, 0x00000001, 0x00000001); // set relay mode
        }
        else
        {
            m_bypassEngine = false;
            MM_REG_WRITE(command, MDP_TDSHP_CFG, 0x00000000, 0x00000001); // disable relay mode
        }
    }

    DP_TIMER_GET_CURRENT_TIME(end);
    DP_TIMER_GET_DURATION_IN_MS(begin, end, diff);

    if (diff > TDSHP_CONFIG_FRAME_THRES)
    {
        DPLOGD("DpEngine_TDSHP: configFrame() time %d ms\n", diff);
    }

#endif // CONFIG_FOR_OS_ANDROID

    m_outHistXLeft      = 0;
    m_outHistYTop       = 0;
    m_lastOutVertical   = 0;
    m_lastOutHorizontal = 0;
    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM DpEngine_TDSHP::onReconfigFrame(DpCommand &command, DpConfig &config)
{
    bool adl_enable = false;

    DPLOGI("DpEngine_TDSHP: onReconfigFrame");
    DPLOGI("DpEngine_TDSHP: pqSessionId = %lux \n", config.pqSessionId);

    PQSession* pPQSession = PQSessionManager::getInstance()->getPQSession(config.pqSessionId);

    if (pPQSession != NULL)
    {
        PQAlgorithmFactory* pPQAlgorithmFactory = PQAlgorithmFactory::getInstance();

        PQDCAdaptor* pPQDCAdaptor = pPQAlgorithmFactory->getDynamicContrast(m_identifier);
        adl_enable = pPQDCAdaptor->calRegs(pPQSession, command, config, m_frameConfigLabel, true);
        DPLOGI("DpEngine_TDSHP: configFrame() adl_enable = %d\n",adl_enable);
    }

    m_outHistXLeft      = 0;
    m_outHistYTop       = 0;
    m_lastOutVertical   = 0;
    m_lastOutHorizontal = 0;
    return DP_STATUS_RETURN_SUCCESS;
}

void DpEngine_TDSHP::ADL_resetLumaHist(DpCommand &command)
{
    // reset LUMA HIST
    //DPLOGI("DpEngine_TDSHP: ADL_resetLumaHist()\n");
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_00, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_01, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_02, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_03, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_04, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_05, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_06, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_07, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_08, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_09, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_10, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_11, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_12, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_13, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_14, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_15, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_HIST_INIT_16, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_LUMA_SUM_INIT, 0, 0xFFFFFFFF);
#ifdef TDSHP_1_1
#ifdef MT8512_PQ_SUPPORT
#else
    MM_REG_WRITE(command, MDP_DC_TWO_D_W1_RESULT_INIT, 0, 0xFFFFFFFF);
#endif
#endif

#if DYN_CONTRAST_VERSION == 2
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_00, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_01, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_02, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_03, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_04, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_05, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_06, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_07, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_08, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_09, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_10, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_11, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_12, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_13, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_14, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_15, 0, 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_CONTOUR_HIST_INIT_16, 0, 0xFFFFFFFF);
#endif
}

#if CONFIG_FOR_OS_ANDROID

#endif // CONFIG_FOR_OS_ANDROID

DP_STATUS_ENUM DpEngine_TDSHP::onConfigTile(DpCommand &command)
{
    uint32_t TDS_in_hsize;
    uint32_t TDS_in_vsize;
    uint32_t TDS_out_hoffset;
    uint32_t TDS_out_voffset;
    uint32_t TDS_out_hsize;
    uint32_t TDS_out_vsize;
    uint32_t TDS_hist_left;
    uint32_t TDS_hist_top;

    // Set source size
    TDS_in_hsize    = m_inTileXRight  - m_inTileXLeft + 1;
    TDS_in_vsize    = m_inTileYBottom - m_inTileYTop + 1;
    MM_REG_WRITE(command, MDP_TDSHP_INPUT_SIZE , (TDS_in_hsize << 16) +
                                          (TDS_in_vsize <<  0), 0x1FFF1FFF);

    // Set crop offset
    TDS_out_hoffset = m_outTileXLeft - m_inTileXLeft;
    TDS_out_voffset = m_outTileYTop  - m_inTileYTop;
    MM_REG_WRITE(command, MDP_TDSHP_OUTPUT_OFFSET, (TDS_out_hoffset << 16) +
                                            (TDS_out_voffset <<  0), 0x00FF00FF);

    // Set target size
    TDS_out_hsize   = m_outTileXRight - m_outTileXLeft + 1;
    TDS_out_vsize   = m_outTileYBottom - m_outTileYTop + 1;
    MM_REG_WRITE(command, MDP_TDSHP_OUTPUT_SIZE, (TDS_out_hsize << 16) +
                                            (TDS_out_vsize <<  0), 0x1FFF1FFF);

    // Set histogram window
    TDS_hist_left = (m_outTileXLeft > m_outHistXLeft) ? m_outTileXLeft : m_outHistXLeft;
    TDS_hist_top  = (m_outTileYTop  > m_outHistYTop)  ? m_outTileYTop  : m_outHistYTop;
    MM_REG_WRITE(command, MDP_HIST_CFG_00, ((m_outTileXRight - m_inTileXLeft) << 16) +
                                            ((TDS_hist_left   - m_inTileXLeft) <<  0), 0xFFFFFFFF);
    MM_REG_WRITE(command, MDP_HIST_CFG_01, ((m_outTileYBottom - m_inTileYTop) << 16) +
                                            ((TDS_hist_top     - m_inTileYTop) <<  0), 0xFFFFFFFF);

#if 0
    {
        uint32_t* pTmp;
        pTmp = (uint32_t*)(0x83700060);
        MM_REG_WRITE_FROM_MEM_BEGIN(command);
        for(i = 0x200; i<= 0x244; i += 4)
        {
            MM_REG_WRITE_FROM_MEM(command,MDP_TDSHP_00+i,pTmp,0xFFFFFFFF);
            pTmp++;
        }
        MM_REG_WRITE_FROM_MEM_END(command);
    }
#endif

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_TDSHP::onPostProc(DpCommand &command)
{
    if (command.getSyncMode()) //old version pq readback must remove
    {
        DPLOGI("DpEngine_TDSHP::onPostProc : SyncMode do nothing\n");
        return DP_STATUS_RETURN_SUCCESS;
    }
    uint32_t index;
    bool pq_readback;
    bool hdr_readback;
    int32_t dre_readback;
    uint32_t engineFlag;
    uint32_t VEncFlag;
    uint32_t counter = 0;
    uint32_t* readbackPABuffer = NULL;
    uint32_t readbackPABufferIndex = 0;

    command.getReadbackStatus(pq_readback, hdr_readback, dre_readback, engineFlag, VEncFlag);

    if ((((engineFlag >> tVENC) & 0x1) && VEncFlag) || !(pq_readback || hdr_readback))
    {
        DPLOGI("DpEngine_TDSHP::onPostProc : VENC and no readback do nothing\n");
        return DP_STATUS_RETURN_SUCCESS;
    }
    readbackPABuffer = command.getReadbackPABuffer(readbackPABufferIndex);

    if (readbackPABuffer == NULL)
    {
        DPLOGW("DpEngine_TDSHP::onPostProc : readbackPABuffer has been destroyed readbackPABuffer = %p, readbackPABufferIndex = %d\n", readbackPABuffer, readbackPABufferIndex);
        return DP_STATUS_RETURN_SUCCESS;
    }
    MM_REG_READ_BEGIN(command);

    if (pq_readback)
    {
        uint32_t iTDSHPBase = /*((engineFlag >> tTDSHP0) & 0x1) ?*/ MDP_TDSHP0_BASE /*: MDP_TDSHP1_BASE*/;

        for (index = 0x6C; index <= 0xB4; index += 4)
        {
            if (index == 0x88)
                continue;

            MM_REG_READ(command, iTDSHPBase + index, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], &m_regLabel[counter]);
            counter++;
        }

        //For shit usage to read color info by Algo's requests
        #ifdef CMD_GPR_R32
	    MM_REG_READ(command, CMD_GPR_R32(CMDQ_DATA_REG_PQ_COLOR), readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], &m_regLabel[counter]);
            counter++;
    	#else
            MM_REG_READ(command, iTDSHPBase + 0x264, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], &m_regLabel[counter]);
            counter++;
        #endif

#if DYN_CONTRAST_VERSION == 2
        for (index = 0x3DC; index <= 0x41C; index += 4)
        {
            MM_REG_READ(command, iTDSHPBase + index, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], &m_regLabel[counter]);
            counter++;
        }
#endif
    }

    MM_REG_READ_END(command);

    command.setNumReadbackPABuffer(counter);

    m_prevPABufferIndex = readbackPABufferIndex;

    DPLOGI("DpEngine_TDSHP::onPostProc: counter:%d\n", counter);

#if 0
    uint32_t    i;
    uint32_t*   pTmp;
    pTmp = (uint32_t*)(0x83700060);

    MM_REG_READ_BEGIN(command);
    for(i = 0x6C; i < 0xB8; i += 4)
    {
        if(0x88 == i)
            continue;

        MM_REG_READ(command,MDP_TDSHP_00+i,pTmp);
        pTmp++;
    }
    MM_REG_READ_END(command);
#endif
    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM DpEngine_TDSHP::onAdvanceTile(DpCommand &command)
{
    // FIXME: check tile order
    if (m_lastOutHorizontal != m_outHorizontal)
    {
        m_outHistXLeft      = m_outHorizontal ? (m_outTileXRight + 1) : 0;
        m_lastOutHorizontal = m_outHorizontal;
    }
    if (m_lastOutVertical != m_outVertical)
    {
        m_outHistYTop       = m_outVertical ? (m_outTileYBottom + 1) : 0;
        m_lastOutVertical   = m_outVertical;
    }

#ifdef MDP_HW_TILE_SW_RESET
    MM_REG_WRITE_FROM_REG_BEGIN(command);

    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_00, MDP_LUMA_HIST_00, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_01, MDP_LUMA_HIST_01, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_02, MDP_LUMA_HIST_02, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_03, MDP_LUMA_HIST_03, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_04, MDP_LUMA_HIST_04, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_05, MDP_LUMA_HIST_05, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_06, MDP_LUMA_HIST_06, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_07, MDP_LUMA_HIST_07, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_08, MDP_LUMA_HIST_08, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_09, MDP_LUMA_HIST_09, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_10, MDP_LUMA_HIST_10, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_11, MDP_LUMA_HIST_11, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_12, MDP_LUMA_HIST_12, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_13, MDP_LUMA_HIST_13, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_14, MDP_LUMA_HIST_14, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_15, MDP_LUMA_HIST_15, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_HIST_INIT_16, MDP_LUMA_HIST_16, 0xFFFFFFFF);
    MM_REG_WRITE_FROM_REG(command, MDP_LUMA_SUM_INIT, MDP_LUMA_SUM, 0xFFFFFFFF);

#ifdef MT8512_PQ_SUPPORT
#else
    MM_REG_WRITE_FROM_REG(command, MDP_DC_TWO_D_W1_RESULT_INIT, MDP_DC_TWO_D_W1_RESULT, 0xFFFFFFFF); // for color
#endif

    MM_REG_WRITE_FROM_REG_END(command);
#else
    DP_UNUSED(command);
#endif

    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM DpEngine_TDSHP::onReconfigTiles(DpCommand &command)
{
    if (command.getSyncMode()) //old version pq readback must remove
    {
        DPLOGI("DpEngine_TDSHP::onReconfigTiles : SyncMode do nothing\n");
        return DP_STATUS_RETURN_SUCCESS;
    }

    uint32_t index;
    bool pq_readback;
    bool hdr_readback;
    int32_t dre_readback;
    uint32_t engineFlag;
    uint32_t VEncFlag;
    uint32_t counter = 0;
    uint32_t* readbackPABuffer = NULL;
    uint32_t readbackPABufferIndex = 0;

    command.getReadbackStatus(pq_readback, hdr_readback, dre_readback, engineFlag, VEncFlag);

    if ((((engineFlag >> tVENC) & 0x1) && VEncFlag) || !(pq_readback || hdr_readback))
    {
        DPLOGI("DpEngine_TDSHP::onReconfigTiles : VENC and no readback do nothing\n");
        return DP_STATUS_RETURN_SUCCESS;
    }

    readbackPABuffer = command.getReadbackPABuffer(readbackPABufferIndex);
    if (readbackPABufferIndex == m_prevPABufferIndex)
    {
        DPLOGI("DpEngine_TDSHP::onReconfigTiles : PABufferIndex no change do nothing\n");
        return DP_STATUS_RETURN_SUCCESS;
    }

    if (readbackPABuffer == NULL)
    {
        DPLOGW("DpEngine_TDSHP::onReconfigTiles : readbackPABuffer has been destroyed readbackPABuffer = %p, readbackPABufferIndex = %d\n", readbackPABuffer, readbackPABufferIndex);
        return DP_STATUS_RETURN_SUCCESS;
    }

    if (pq_readback)
    {
        uint32_t iTDSHPBase = /*((engineFlag >> tTDSHP0) & 0x1) ?*/ MDP_TDSHP0_BASE /*: MDP_TDSHP1_BASE*/;

        for (index = 0x6C; index <= 0xB4; index += 4)
        {
            if (index == 0x88)
                continue;

            MM_REG_READ(command, iTDSHPBase + index, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], NULL, m_regLabel[counter]);
            counter++;
        }

        //For shit usage to read color info by Algo's requests
            #ifdef CMD_GPR_R32
                MM_REG_READ(command, CMD_GPR_R32(CMDQ_DATA_REG_PQ_COLOR), readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], NULL, m_regLabel[counter]);
                counter++;
            #else
                MM_REG_READ(command, iTDSHPBase + 0x264, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], NULL, m_regLabel[counter]);
                counter++;
            #endif

#if DYN_CONTRAST_VERSION == 2
        for (index = 0x3DC; index <= 0x41C; index += 4)
        {
            MM_REG_READ(command, iTDSHPBase + index, readbackPABuffer[((readbackPABufferIndex + counter) & (MAX_NUM_READBACK_PA_BUFFER - 1))], NULL, m_regLabel[counter]);
            counter++;
        }
#endif
    }

    command.setNumReadbackPABuffer(counter);
    m_prevPABufferIndex = readbackPABufferIndex;

    return DP_STATUS_RETURN_SUCCESS;
}

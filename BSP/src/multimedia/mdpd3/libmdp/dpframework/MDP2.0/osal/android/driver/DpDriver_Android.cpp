#include "DpDriver.h"
#include "DpLogger.h"
#include "DpTimer.h"
#include "DpProfiler.h"
#include "DpDataType.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef CMDQ_V3
#include "cmdq_v3_driver.h"
#else
#include "cmdq_driver.h"
#endif
#include "DpEngineType.h"
#include "DpEngineBase.h"
#if PMQOS_SETTING
#include "cmdq_mdp_pmqos.h"
#endif
#include "ddp_drv.h"
#include "DpProperty.h"
#include "DpPlatform.h"

#define MDP_PAGE_SIZE 1024

DpDriver* DpDriver::s_pInstance = 0;

// Mutex object
DpMutex   DpDriver::s_instMutex;

DpDriver* DpDriver::getInstance()
{
    if (0 == s_pInstance)
    {
        AutoMutex lock(s_instMutex);
        if (0 == s_pInstance)
        {
            s_pInstance = new DpDriver();
            DPLOGI("DpDriver: create driver instance(0x%p)\n", s_pInstance);

            atexit(destroyInstance);
        }
    }

    return s_pInstance;
}


void DpDriver::destroyInstance()
{
    AutoMutex lock(s_instMutex);

    delete s_pInstance;
    DPLOGI("DpDriver: driver instance(0x%p) is deleted\n", s_pInstance);

    s_pInstance = NULL;
}


DpDriver::DpDriver()
    : m_driverID(-1),
      m_enableLog(0),
      m_enableSystrace(0),
      m_enableDumpBuffer(0),
      m_enableCheckDumpReg(0),
      m_enableDumpRegister(0),
      m_enableCheckMet(0),
      m_enableMet(0),
      m_reduceConfigDisable(0),
      m_pq_support(0),
      m_mdpColor(0),
      m_refCntRDMA0(0),
      m_refCntRDMA1(0),
      m_supportGlobalPQ(0),
      m_cmdqDts({})
{
    DPLOGI("DpDriver: create DpDriver instance\n");

    m_driverID = open("/dev/mtk_cmdq", O_RDONLY, 0);
    if (-1 == m_driverID)
    {
        DPLOGE("Open disp driver failed\n!");
        //yuesheng for debug assert(0);
    }
    else
    {
        queryDeviceTreeInfo();
    }

    getProperty("vendor.dp.log.enable", &m_enableLog);
    getProperty("vendor.dp.systrace.enable", &m_enableSystrace);
    getProperty("vendor.dp.dumpbuffer.enable", &m_enableDumpBuffer);
    getProperty("vendor.dp.dumpbuffer.folder", m_dumpBufferFolder);
    getProperty("persist.vendor.dp.dumpreg.check", &m_enableCheckDumpReg);
    getProperty("vendor.dp.dumpreg.enable", &m_enableDumpRegister);
    getProperty("persist.vendor.dp.met.check", &m_enableCheckMet);
    getProperty("vendor.dp.met.enable", &m_enableMet);
#ifdef MDP_REDUCE_CONFIG
    getProperty("vendor.dp.reduceconfig.disable", &m_reduceConfigDisable);
#else
    m_reduceConfigDisable = 1;
#endif
    getProperty("ro.vendor.mtk_pq_support", &m_pq_support);
    getProperty("ro.vendor.globalpq.support", &m_supportGlobalPQ);


    int32_t pqColorMode = 0;
    getProperty("ro.vendor.mtk_pq_color_mode", &pqColorMode);

    if (pqColorMode == 2 || pqColorMode == 3)
    {
        m_mdpColor = 1;
    }
    else
    {
        m_mdpColor = 0;
    }

    if (strcmp(m_dumpBufferFolder, "") == 0)
    {
        sprintf(m_dumpBufferFolder, "/sdcard/mdp/");
    }
}


DpDriver::~DpDriver()
{
    DPLOGI("DpDriver: destroy DpDriver instance\n");

    if(-1 != m_driverID)
    {
        close(m_driverID);
        m_driverID = -1;
    }
}


DP_STATUS_ENUM DpDriver::checkHandle()
{
    AutoMutex lock(m_instMutex);

    if (-1 == m_driverID)
    {
        m_driverID = open("/dev/mtk_cmdq", O_RDONLY, 0);
    }

    if (-1 == m_driverID)
    {
        DPLOGE("DpDriver: can't open display driver\n");
        assert(0);

        return DP_STATUS_OPERATION_FAILED;
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::getTDSHPGain(DISPLAY_TDSHP_T *pSharpness,
                                      uint32_t        *pCurLevel)
{
    int32_t       status;
    DISP_PQ_PARAM pqparam;
    int           drvID = -1;

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    drvID = open("/proc/mtk_mira", O_RDONLY, 0);
    if (-1 == drvID)
    {
        DPLOGE("DpDriver: can't open display driver\n");
        assert(0);

        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(drvID, DISP_IOCTL_GET_TDSHPINDEX, pSharpness);
    if (0 != status)
    {
        DPLOGE("DpDriver: get sharpness value failed(%d)\n", status);
        close(drvID);
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(drvID, DISP_IOCTL_GET_PQPARAM, &pqparam);
    if (0 != status)
    {
        DPLOGE("DpDriver: get sharpness level failed(%d)\n", status);
        close(drvID);
        return DP_STATUS_OPERATION_FAILED;
    }

    *pCurLevel = pqparam.u4SHPGain;
    close(drvID);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::requireMutex(int32_t *pMutex)
{
    int32_t status;

    DPLOGI("DpDriver: require the mutex with pointer %p\n", pMutex);

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(m_driverID, CMDQ_IOCTL_LOCK_MUTEX, pMutex);
    if (0 != status)
    {
        DPLOGE("DpDriver: require mutex failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::releaseMutex(int32_t mutex)
{
    int32_t status;

    DPLOGI("DpDriver: relase the specified mutex: %d\n", mutex);

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(m_driverID, CMDQ_IOCTL_UNLOCK_MUTEX, &mutex);
    if (0 != status)
    {
        DPLOGE("DpDriver: release mutex failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::queryEngUsages(EngUsages &engUsages)
{
    int32_t  status;
    cmdqUsageInfoStruct usageInfo;

    DPLOGI("DpDriver: exec CMDQ_IOCTL_QUERY_USAGE\n");

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(m_driverID, CMDQ_IOCTL_QUERY_USAGE, &usageInfo);
    if (0 != status)
    {
        DPLOGI("DpDriver: exec CMDQ_IOCTL_QUERY_USAGE failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    memcpy(engUsages, usageInfo.count, sizeof(engUsages));
    return DP_STATUS_RETURN_SUCCESS;
}


void DpDriver::addRefCnt(DpEngineType &sourceEng)
{
    AutoMutex lock(m_instMutex);

    if (tRDMA0 == sourceEng)
    {
        if (m_refCntRDMA0 > m_refCntRDMA1)
        {
            DPLOGI("DpDriver: select source engine RDMA1 due to ref. count\n");
            sourceEng = tRDMA1;
            m_refCntRDMA1++;
        }
        else
        {
            m_refCntRDMA0++;
        }
    }
    else if (tRDMA1 == sourceEng)
    {
        if (m_refCntRDMA1 > m_refCntRDMA0)
        {
            DPLOGI("DpDriver: select source engine RDMA0 due to ref. count\n");
            sourceEng = tRDMA0;
            m_refCntRDMA0++;
        }
        else
        {
            m_refCntRDMA1++;
        }
    }
}


void DpDriver::removeRefCnt(uint64_t pathFlags)
{
    AutoMutex lock(m_instMutex);

    if (pathFlags & (1 << tRDMA0))
    {
        if (m_refCntRDMA0 != 0)
        {
            m_refCntRDMA0--;
        }
    }
#if tRDMA1 != tNone
    else if (pathFlags & (1 << tRDMA1))
    {
        if (m_refCntRDMA1 != 0)
        {
            m_refCntRDMA1--;
        }
    }
#endif
}


DP_STATUS_ENUM DpDriver::submitCommand(DpCommandBlock &block, DpJobID* pRet, uint32_t extRecorderFlag, char**)
{
    DP_TRACE_CALL();
    int32_t           status;
    cmdqCommandStruct param;
    cmdqJobStruct     submitJob;
    //DpFrameInfo       frameInfo;
    //int32_t           portNum;
    //char              *frameInfoToCMDQ;

#if 0
    frameInfo = block.getFrameInfo();
    portNum = block.getRegDstNum();
    frameInfoToCMDQ = block.getFrameInfoToCMDQ();
    setFrameInfo(frameInfo, portNum, frameInfoToCMDQ);

    *pFrameInfo = block.getFrameInfoToCMDQ();
#endif

    memset(&param, 0, sizeof(param));

#if 0
    param.userDebugStr = (unsigned long)frameInfoToCMDQ;
    param.userDebugStrLen = strlen(frameInfoToCMDQ);
#endif

#if PMQOS_SETTING
    param.prop_addr = (unsigned long)block.getMdpPmqos();
    param.prop_size = sizeof(mdp_pmqos);
#endif

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    if (0 != block.getSecureAddrCount())
    {
        param.secData.isSecure = true;
        param.secData.addrMetadataCount = block.getSecureAddrCount();
        param.secData.addrMetadatas = (unsigned long)block.getSecureAddrMD();
        param.secData.enginesNeedDAPC = block.getEngineFlag() | block.getSecurePortFlag();
        param.secData.enginesNeedPortSecurity = block.getSecurePortFlag();

        DPLOGI("secure meta data engine flag = %x !!!!\n",block.getSecurePortFlag());

#ifdef CMDQ_V3
        if (block.hasIspSecMeta())
        {
            DPLOGI("DpDriver: Set ISP secure meta\n");
            //param.secData.ispMeta = block.getSecIspMeta();
        }
#endif
        DPLOGI("DpDriver: Secure meta data Ready !!!!\n");
    }

    param.scenario     = CMDQ_SCENARIO_USER_MDP;
    param.priority     = block.getPriority();

    if (VENC_ENABLE_FLAG == (VENC_ENABLE_FLAG & extRecorderFlag))
    {
        param.engineFlag = (1 << tVENC);
    }
    else
    {
        param.engineFlag = block.getEngineFlag() & (~(1 << tVENC));
    }

    param.pVABase      = (unsigned long)block.getBlockBaseSW();
    param.blockSize    = block.getBlockSize();

    param.debugRegDump = block.getISPDebugDumpRegs();

    submitJob.command = param;
    submitJob.hJob = 0;

    DPLOGI("DpDriver: submit command block: start 0x%p, end 0x%p, engine flag 0x%lx\n",
        (void *)param.pVABase, (void *)(param.pVABase + param.blockSize), param.engineFlag);
    DPLOGI("DpDriver: submit command block: debugRegDump: %u\n", param.debugRegDump);

    status = ioctl(m_driverID, CMDQ_IOCTL_ASYNC_JOB_EXEC, &submitJob);
    if (0 != status)
    {
        DPLOGE("DpDriver: submit command block failed(%d), JobID: %llx\n", status, submitJob.hJob);
        return DP_STATUS_OPERATION_FAILED;
    }

    *pRet = submitJob.hJob;

    if ((STREAM_BITBLT == block.getScenario()) || (STREAM_GPU_BITBLT == block.getScenario()))
    {
        removeRefCnt(param.engineFlag);
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::waitFramedone(DpJobID pFrame, DpReadbackRegs &readBackRegs)
{
    DP_TRACE_CALL();
    int32_t             status;
    cmdqJobResultStruct param;
    uint32_t            index;
    DpTimeValue    begin;
    DpTimeValue    end;
    int32_t        diff;

    memset(&param, 0, sizeof(param));
    param.hJob = pFrame;

    if (((readBackRegs.m_engineFlag >> tVENC) & 0x1) ||
#ifdef MT8512_PQ_SUPPORT
#else
        ((readBackRegs.m_engineFlag >> tCAMIN) & 0x1) ||
#endif
        ((readBackRegs.m_engineFlag >> tTDSHP0) & 0x1)
#ifdef SUPPORT_DRE
        || ((readBackRegs.m_engineFlag >> tAAL0) & 0x1)
#endif // SUPPORT_DRE
       )
    {
        param.readAddress.count = readBackRegs.m_num;
        param.readAddress.dmaAddresses = (unsigned long)readBackRegs.m_regs;
        param.readAddress.values = (unsigned long)readBackRegs.m_values;

        DPLOGI("DpDriver: count:%d\n", readBackRegs.m_num);
        for (index = 0; index < readBackRegs.m_num; index++)
        {
            DPLOGI("DpDriver: %d, addr:%#010x\n", index, readBackRegs.m_regs[index]);
        }

        if (DP_STATUS_RETURN_SUCCESS != checkHandle())
        {
            DPLOGE("DpDriver: invalid display driver handle\n");
            return DP_STATUS_OPERATION_FAILED;
        }

        DP_TIMER_GET_CURRENT_TIME(begin);
        status = ioctl(m_driverID, CMDQ_IOCTL_ASYNC_JOB_WAIT_AND_CLOSE, &param);
        if (0 != status)
        {
            DPLOGE("DpDriver: waitFramedone failed(%d), JobID: %llx\n", status, pFrame);
            return DP_STATUS_OPERATION_FAILED;
        }

        DP_TIMER_GET_CURRENT_TIME(end);
        DP_TIMER_GET_DURATION_IN_MS(begin,
                                end,
                                diff);
        DPLOGI("DpDriver: count:%d\n", readBackRegs.m_num);
        for (index = 0; index < readBackRegs.m_num; index++)
        {
            DPLOGI("DpDriver: %d, addr:%#010x, value:%#010x\n", index, readBackRegs.m_regs[index], readBackRegs.m_values[index]);
        }
    }
    else // for ISP part
    {
        param.regValue.regValues = (unsigned long)readBackRegs.m_values;
        param.regValue.count = MAX_NUM_READBACK_REGS;

        if (DP_STATUS_RETURN_SUCCESS != checkHandle())
        {
            DPLOGE("DpDriver: invalid display driver handle\n");
            return DP_STATUS_OPERATION_FAILED;
        }

        DP_TIMER_GET_CURRENT_TIME(begin);
        status = ioctl(m_driverID, CMDQ_IOCTL_ASYNC_JOB_WAIT_AND_CLOSE, &param);
        if (0 != status)
        {
            DPLOGE("DpDriver: waitFramedone failed(%d), JobID: %llx\n", status, pFrame);
            return DP_STATUS_OPERATION_FAILED;
        }
        DP_TIMER_GET_CURRENT_TIME(end);
        DP_TIMER_GET_DURATION_IN_MS(begin,
                                end,
                                diff);

        readBackRegs.m_num = param.regValue.count;
        readBackRegs.m_engineFlag = param.engineFlag;
    }

#if 0
    //JPEGEnc is enabled
    if ((readBackRegs.m_engineFlag >> tJPEGENC) & 0x1)
    {
        readBackRegs.m_jpegEnc_filesize = (readBackRegs.m_values[0] + 0x200) -
            (readBackRegs.m_values[1] + readBackRegs.m_values[2]);
        DPLOGI("JPEGEnc is enabled and JPEGEnc regs read back[0..2] %X %X %X \n",
               readBackRegs.m_values[0], readBackRegs.m_values[1], readBackRegs.m_values[2]);
        DPLOGI("waitComplete: JPEGEnc file size is %d\n", readBackRegs.m_jpegEnc_filesize);
    }
#endif
    if (diff > 34)
    {
        DPLOGW("DpDriver::waitFramedone takes %dms", diff);
    }
    else
    {
        DPLOGI("DpDriver::waitFramedone takes %dms", diff);
    }
    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::execCommand(DpCommandBlock &block)
{
    DP_TRACE_CALL();
    int32_t           status;
    cmdqCommandStruct param;
    DpTimeValue       begin;
    DpTimeValue       end;
    int32_t           diff;
    //DpFrameInfo       frameInfo;
    //int32_t           portNum;
    //char              *frameInfoToCMDQ;

#if 0
    frameInfo = block.getFrameInfo();
    portNum = block.getRegDstNum();
    frameInfoToCMDQ = block.getFrameInfoToCMDQ();
    setFrameInfo(frameInfo, portNum, frameInfoToCMDQ);
#endif

    memset(&param, 0, sizeof(param));

#if 0
    param.userDebugStr = (unsigned long)frameInfoToCMDQ;
    param.userDebugStrLen = strlen(frameInfoToCMDQ);
#endif

#if PMQOS_SETTING
    param.prop_addr = (unsigned long)block.getMdpPmqos();
    param.prop_size = sizeof(mdp_pmqos);
#endif

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    if (0 != block.getSecureAddrCount())
    {
        param.secData.isSecure = true;
        param.secData.addrMetadataCount = block.getSecureAddrCount();
        param.secData.addrMetadatas = (unsigned long)block.getSecureAddrMD();
        param.secData.enginesNeedDAPC = block.getEngineFlag() | block.getSecurePortFlag();
        param.secData.enginesNeedPortSecurity = block.getSecurePortFlag();

        DPLOGI("secure meta data engine flag = %x !!!!\n",block.getSecurePortFlag());
        DPLOGI("DpDriver: Secure meta data Ready !!!!\n");
    }

    #ifdef CONFIG_FOR_SOURCE_PQ
    if (STREAM_COLOR_BITBLT == block.getScenario())
    {
        param.scenario = CMDQ_SCENARIO_USER_DISP_COLOR;
    }
    else
    #endif
    {
        param.scenario = CMDQ_SCENARIO_USER_MDP;
    }

    param.priority     = block.getPriority();
    param.engineFlag   = block.getEngineFlag();
    param.pVABase      = (unsigned long)block.getBlockBaseSW();
    param.blockSize    = block.getBlockSize();

    param.debugRegDump = block.getISPDebugDumpRegs();

    param.regRequest.regAddresses = (unsigned long)block.getReadbackRegs(param.regRequest.count);
    param.regValue.regValues = (unsigned long)block.getReadbackValues(param.regValue.count);

    DPLOGI("DpDriver: exec command block: regAddresses: %p, regValues: %p, counter: %u\n",
        (void *)param.regRequest.regAddresses, (void *)param.regValue.regValues, param.regRequest.count);
    DPLOGI("DpDriver: exec command block: start %p, end %p, engine flag 0x%lx\n",
        (void *)param.pVABase, (void *)(param.pVABase + param.blockSize), param.engineFlag);
    DPLOGI("DpDriver: exec command block: debugRegDump: %u\n", param.debugRegDump);

    DP_TIMER_GET_CURRENT_TIME(begin);

    status = ioctl(m_driverID, CMDQ_IOCTL_EXEC_COMMAND, &param);
    if (0 != status)
    {
        DPLOGE("DpDriver: exec command block failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    DP_TIMER_GET_CURRENT_TIME(end);
    DP_TIMER_GET_DURATION_IN_MS(begin,
                                end,
                                diff);
    if (diff > 30)
    {
        DPLOGW("DpDriver::execCommand takes %d ms\n", diff);
    }

    if ((STREAM_BITBLT == block.getScenario()) || (STREAM_GPU_BITBLT == block.getScenario()))
    {
        removeRefCnt(param.engineFlag);
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::allocatePABuffer(uint32_t numPABuffer, uint32_t *pPABuffer)
{
    DP_TRACE_CALL();
    int32_t status;
    uint32_t index;
    cmdqWriteAddressStruct wa;

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    for (index = 0; index < numPABuffer; index++)
    {
        if((index & (MDP_PAGE_SIZE - 1)) == 0)
        {
            memset(&wa, 0, sizeof(wa));
            wa.count = MDP_PAGE_SIZE;
            status = ioctl(m_driverID, CMDQ_IOCTL_ALLOC_WRITE_ADDRESS, &wa);
            if (0 != status)
            {
                DPLOGE("DpDriver: allocate PA Buffer failed(%d)\n", status);
                return DP_STATUS_OPERATION_FAILED;
            }
        }
        pPABuffer[index] = wa.startPA + ((index & (MDP_PAGE_SIZE - 1)) << 2);
        DPLOGI("DpDriver allocatePABuffer(): PABuffer[%d] = %x\n", index, pPABuffer[index]);
    }

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::releasePABuffer(uint32_t numPABuffer, uint32_t *pPABuffer)
{
    DP_TRACE_CALL();
    int32_t status;
    uint32_t index;
    cmdqWriteAddressStruct wa;

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    for (index = 0 ; index < numPABuffer ; index += MDP_PAGE_SIZE)
    {
        memset(&wa, 0, sizeof(wa));
        wa.count = MDP_PAGE_SIZE;
        wa.startPA = pPABuffer[index];
        status = ioctl(m_driverID, CMDQ_IOCTL_FREE_WRITE_ADDRESS, &wa);
        if (0 != status)
        {
            DPLOGE("DpDriver: allocate PA Buffer failed(%d)\n", status);
            return DP_STATUS_OPERATION_FAILED;
        }
    }

    memset(pPABuffer, 0, sizeof(uint32_t) * numPABuffer);

    return DP_STATUS_RETURN_SUCCESS;
}


int32_t DpDriver::getEnableLog()
{
    return m_enableLog;
}


int32_t DpDriver::getEnableSystrace()
{
    return m_enableSystrace;
}

int32_t DpDriver::getEnableMet()
{
    if (m_enableCheckMet)
    {
        getProperty("vendor.dp.met.enable", &m_enableMet);
    }
    return m_enableMet;
}

int32_t DpDriver::getEnableDumpBuffer()
{
    return m_enableDumpBuffer;
}

char* DpDriver::getdumpBufferFolder()
{
    return m_dumpBufferFolder;
}

int32_t DpDriver::getEnableDumpRegister()
{
    if (m_enableCheckDumpReg)
    {
        getProperty("vendor.dp.dumpreg.enable", &m_enableDumpRegister);
    }
    return m_enableDumpRegister;
}

int32_t DpDriver::getDisableReduceConfig()
{
    return m_reduceConfigDisable;
}

int32_t DpDriver::getPQSupport()
{
#ifdef MT8512_PQ_SUPPORT
	m_pq_support = 1;
#endif
    return m_pq_support;
}

int32_t DpDriver::getGlobalPQSupport()
{
    return m_supportGlobalPQ;
}

int32_t DpDriver::getMdpColor()
{
    return m_mdpColor;
}

DP_STATUS_ENUM DpDriver::notifyEngineWROT()
{
    int status;
    uint32_t engineFlag = (1 << tWROT0)
#if tWROT1 != tNone
        | (1 << tWROT1)
#endif
        ;

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(m_driverID, CMDQ_IOCTL_NOTIFY_ENGINE, engineFlag);
    if (0 != status)
    {
        DPLOGE("DpDriver: notify WROT failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    DPLOGD("notify WROT success\n");
    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpDriver::queryDeviceTreeInfo()
{
    int32_t status;

    if (DP_STATUS_RETURN_SUCCESS != checkHandle())
    {
        DPLOGE("DpDriver: invalid display driver handle\n");
        return DP_STATUS_OPERATION_FAILED;
    }

    status = ioctl(m_driverID, CMDQ_IOCTL_QUERY_DTS, &m_cmdqDts);
    if (0 != status)
    {
        DPLOGE("DpDriver: exec CMDQ_IOCTL_QUERY_DTS failed(%d)\n", status);
        return DP_STATUS_OPERATION_FAILED;
    }

    if (0 == m_cmdqDts.MDPBaseAddress[CMDQ_MDP_PA_BASE_MM_MUTEX])
    {
        DPLOGD("DpDriver: Disp_mutex value is not written in device tree.\n");
    }

    return DP_STATUS_RETURN_SUCCESS;
}


int32_t DpDriver::getEventValue(int32_t event)
{
    if (event < 0 || event >= CMDQ_SYNC_TOKEN_MAX)
    {
        DPLOGE("DpDriver: event value %d is not written in device tree.\n", event);
        return DP_STATUS_OPERATION_FAILED;
    }

#ifdef CMDQ_DVENT_FROM_DTS
    return m_cmdqDts.eventTable[event];
#else
    return event;
#endif
}


uint32_t DpDriver::getMMSysMutexBase()
{
    return m_cmdqDts.MDPBaseAddress[CMDQ_MDP_PA_BASE_MM_MUTEX];
}

DP_STATUS_ENUM DpDriver::setFrameInfo(DpFrameInfo frameInfo, int32_t portNum, char *frameInfoToCMDQ)
{
    char frameInfoOutput[200] = {0};
    uint32_t frameInfoOutputLength = 0;

    sprintf(frameInfoToCMDQ,"[MDP] Input: (%d, %d, %d, %d, C%d%s%s%s%s) sec%d\n[MDP] Buffer info:bufMVA={0x%08x,0x%08x,0x%08x} bufSize={0x%08x,0x%08x,0x%08x}\n",
        frameInfo.m_srcWidth,
        frameInfo.m_srcHeight,
        frameInfo.m_srcYPitch,
        frameInfo.m_srcUVPitch,
        DP_COLOR_GET_UNIQUE_ID(frameInfo.m_srcFormat),
        DP_COLOR_GET_SWAP_ENABLE(frameInfo.m_srcFormat) ? "s" : "",
        DP_COLOR_GET_BLOCK_MODE(frameInfo.m_srcFormat) ? "b" : "",
        DP_COLOR_GET_INTERLACED_MODE(frameInfo.m_srcFormat) ? "i" : "",
        DP_COLOR_GET_UFP_ENABLE(frameInfo.m_srcFormat) ? "u" : "",
        frameInfo.m_srcSecMode,
        frameInfo.m_srcMemAddr[0],
        frameInfo.m_srcMemAddr[1],
        frameInfo.m_srcMemAddr[2],
        frameInfo.m_srcMemSize[0],
        frameInfo.m_srcMemSize[1],
        frameInfo.m_srcMemSize[2]);

    for(int portIndex = 0 ; portIndex < portNum ; portIndex++){
        frameInfoOutputLength = sprintf(frameInfoOutput,"[MDP] Output: port%d (%d, %d, %d, %d, C%d%s%s%s%s) sec%d\n[MDP] Buffer info:bufMVA={0x%08x,0x%08x,0x%08x} bufSize={0x%08x,0x%08x,0x%08x}\n",
            portIndex,
            frameInfo.m_dstWidth[portIndex],
            frameInfo.m_dstHeight[portIndex],
            frameInfo.m_dstYPitch[portIndex],
            frameInfo.m_dstUVPitch[portIndex],
            DP_COLOR_GET_UNIQUE_ID(frameInfo.m_dstFormat[portIndex]),
            DP_COLOR_GET_SWAP_ENABLE(frameInfo.m_dstFormat[portIndex]) ? "s" : "",
            DP_COLOR_GET_BLOCK_MODE(frameInfo.m_dstFormat[portIndex]) ? "b" : "",
            DP_COLOR_GET_INTERLACED_MODE(frameInfo.m_dstFormat[portIndex]) ? "i" : "",
            DP_COLOR_GET_UFP_ENABLE(frameInfo.m_dstFormat[portIndex]) ? "u" : "",
            frameInfo.m_dstSecMode[portIndex],
            frameInfo.m_dstMemAddr[portIndex][0],
            frameInfo.m_dstMemAddr[portIndex][1],
            frameInfo.m_dstMemAddr[portIndex][2],
            frameInfo.m_dstMemSize[portIndex][0],
            frameInfo.m_dstMemSize[portIndex][1],
            frameInfo.m_dstMemSize[portIndex][2]);

        strncat(frameInfoToCMDQ, frameInfoOutput, frameInfoOutputLength);
    }
    return DP_STATUS_RETURN_SUCCESS;
}

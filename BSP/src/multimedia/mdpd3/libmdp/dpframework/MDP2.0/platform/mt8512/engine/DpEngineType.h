#ifndef __DP_ENGINE_TYPE_H__
#define __DP_ENGINE_TYPE_H__

#include "DpConfig.h"
#include "DpDataType.h"

#include "cmdq_v3_def.h"

#define DP_GET_ENGINE_NAME_CASE(_value, _name)          \
    case (_value):                                      \
    _name = (char *)#_value;                                    \
    break;
#define DP_GET_ENGINE_NAME(value, name)                 \
{                                                       \
    switch(value)                                       \
    {                                                   \
        DP_GET_ENGINE_NAME_CASE(tRDMA0, name)           \
        DP_GET_ENGINE_NAME_CASE(tGAMMA, name)           \
        DP_GET_ENGINE_NAME_CASE(tDTH, name)           \
        DP_GET_ENGINE_NAME_CASE(tSCL0, name)            \
        DP_GET_ENGINE_NAME_CASE(tTDSHP0, name)            \
        DP_GET_ENGINE_NAME_CASE(tWROT0, name)          \
        default:                                        \
        name = (char*)"tNone";                          \
        assert(0);                                      \
        break;                                          \
    }                                                   \
}

// Invalid engine type
#define tNone      -1 //for 8512 build pass -1
//CAM
#define tWPEI         tNone //CMDQ_ENG_WPEI
#define tWPEO         tNone //CMDQ_ENG_WPEO
#define tWPEI2        tNone //CMDQ_ENG_WPEI2
#define tWPEO2        tNone //CMDQ_ENG_WPEO2
#define tIMGI         tNone //CMDQ_ENG_ISP_IMGI
#define tIMGO         tNone //CMDQ_ENG_ISP_IMGO
#define tIMG2O        tNone //CMDQ_ENG_ISP_IMG2O
//IPU
#define tIPUI         tNone //CMDQ_ENG_IPUI
#define tIPUO         tNone //CMDQ_ENG_IPUO
//MDP
#define tCAMIN        tNone //CMDQ_ENG_MDP_CAMIN
#define tCAMIN2       tNone //CMDQ_ENG_MDP_CAMIN2
#define tRDMA0        CMDQ_ENG_MDP_RDMA0
#define tRDMA1        tNone
#define tAAL0         tNone //CMDQ_ENG_MDP_AAL0
#define tCCORR0       tNone //CMDQ_ENG_MDP_CCORR0
#define tSCL0         CMDQ_ENG_MDP_RSZ0
#define tSCL1         tNone //CMDQ_ENG_MDP_RSZ1
#define tSCL2         tNone
#define tTDSHP0       CMDQ_ENG_MDP_TDSHP0
#define tCOLOR0       tNone //CMDQ_ENG_MDP_COLOR0
#define tWROT0        CMDQ_ENG_MDP_WROT0
#define tWROT1        tNone
#define tWDMA         tNone //CMDQ_ENG_MDP_WDMA
#define tPATH0_SOUT   tNone //CMDQ_ENG_MDP_PATH0_SOUT
#define tPATH1_SOUT   tNone //CMDQ_ENG_MDP_PATH1_SOUT
#define tGAMMA        CMDQ_ENG_MDP_GAMMA
#define tDTH          CMDQ_ENG_MDP_DTH
//DISP
#define tTO_WROT      tNone
#define tTO_WROT_SOUT tNone
#define tTO_DISP0     tNone
#define tTO_DISP1     tNone
#define tTO_WDMA      tNone
#define tTO_RSZ       tNone
//JPEG
#define tJPEGENC      tNone //CMDQ_ENG_JPEG_ENC
#define tVENC         CMDQ_MAX_ENGINE_COUNT//tNone //CMDQ_ENG_VIDEO_ENC
#define tTotal        (tVENC+1)
#define tJPEGDEC      tNone //CMDQ_ENG_JPEG_DEC
#define tJPEGREMDC    tNone //CMDQ_ENG_JPEG_REMDC
//PQ
#define tCOLOR_EX     tNone //CMDQ_ENG_DISP_COLOR0
#define tOVL0_EX      tNone //CMDQ_ENG_DISP_OVL0
#define tWDMA_EX      tNone //CMDQ_ENG_DISP_WDMA0

#endif  // __DP_ENGINE_TYPE_H__

#include "DpPathConnection.h"
#include "DpPathBase.h"
#include "mmsys_config.h"
#include "tile_driver.h"
#include "tile_param.h"
//#include "DpWrapper_WPE.h"
//#include "DpWrapper_ISP.h"

const DpPathConnection::mout_t DpPathConnection::s_moutMap[MOUT_MAP_SIZE] =
{
    {  tRDMA0, {      tGAMMA,        tDTH,       tSCL0,  tTDSHP0,  tWROT0}, MDP_RDMA0_MOUT_EN},
    {  tGAMMA, {      tWROT0,      tDTH,       tNone,    tNone,   tNone}, MDP_GAMMA_MOUT_EN},
    {   tSCL0, {     tWROT0,      tTDSHP0,       tNone,    tNone,   tNone}, MDP_PRZ0_MOUT_EN},
};

const DpPathConnection::sel_t DpPathConnection::s_selInMap[SEL_IN_SIZE] =
{
    {        tDTH, {      tGAMMA,        tRDMA0,   tNone,    tNone,   tNone,   tNone,   tNone, tNone},  MDP_DTH_SEL_IN},
    {     tTDSHP0, {      tSCL0,         tRDMA0,   tNone,    tNone,   tNone,   tNone,   tNone, tNone},  MDP_TDSHP_SEL_IN},
    {      tWROT0, { 	  tDTH,        tRDMA0,    tTDSHP0,    tSCL0, tGAMMA,   tNone,   tNone, tNone},  MDP_WROT0_SEL_IN},
};

const DpPathConnection::sout_t DpPathConnection::s_selOutMap[SOUT_MAP_SIZE] =
{
    //{  tTO_WROT_SOUT, {  tWROT0,    tWROT1}, DISP_TO_WROT_SOUT_SEL},
};


DP_STATUS_ENUM DpPathConnection::initTilePath(struct TILE_PARAM_STRUCT *p_tile_param)
{
    DpPathBase::iterator  iterator;
    DpEngineType          curType;
    int32_t               index;

    /* tile core property */
    TILE_REG_MAP_STRUCT *ptr_tile_reg_map = p_tile_param->ptr_tile_reg_map;

    if (false == m_connected)
    {
        if (false == queryMuxInfo())
        {
            return DP_STATUS_INVALID_PATH;
        }
    }

    for (iterator = m_pPath->begin(); iterator != m_pPath->end(); iterator++)
    {
        if (true == iterator->isOutputDisable())
        {
            continue;
        }

        curType = iterator->getEngineType();
        switch (curType)
        {
        //MDP
        case tRDMA0:         ptr_tile_reg_map->RDMA0_EN       = 1; break;
        case tGAMMA:         ptr_tile_reg_map->GAMMA_EN       = 1; break;
        case tDTH:           ptr_tile_reg_map->DTH_EN         = 1; break;
        case tSCL0:          ptr_tile_reg_map->PRZ0_EN        = 1; break;
        case tTDSHP0:        ptr_tile_reg_map->TDSHP0_EN      = 1; break;
        case tWROT0:         ptr_tile_reg_map->WROT0_EN       = 1; break;
        default:        break;
        }
    }

    for (index = 0; index < MOUT_NUM; index++)
    {
        switch (s_moutMap[index].id)
        {
        case tRDMA0:    ptr_tile_reg_map->RDMA0_OUT     = m_mOutInfo[index]; break;
        case tGAMMA:    ptr_tile_reg_map->GAMMA_OUT     = m_mOutInfo[index]; break;
        case tSCL0:     ptr_tile_reg_map->PRZ0_OUT      = m_mOutInfo[index]; break;
        default:        assert(0);
        }
    }

    for (index = 0; index < SEL_IN_NUM; index++)
    {
        switch (s_selInMap[index].id)
        {
        case tDTH:        ptr_tile_reg_map->DTH_SEL        = m_sInInfo[index]; break;
        case tTDSHP0:     ptr_tile_reg_map->TDSHP0_SEL     = m_sInInfo[index]; break;
        case tWROT0:      ptr_tile_reg_map->WROT0_SEL      = m_sInInfo[index]; break;
        default:        assert(0);
        }
    }

    for (index = 0; index < SEL_OUT_NUM; index++)
    {
        switch (s_selOutMap[index].id)
        {
        //case tRDMA0:    ptr_tile_reg_map->RDMA0_SOUT     = m_sOutInfo[index]; break;
        //case tGAMMA:    ptr_tile_reg_map->GAMMA_SOUT     = m_sOutInfo[index]; break;
        //case tSCL0:     ptr_tile_reg_map->PRZ0_SOUT      = m_sOutInfo[index]; break;
        default:        assert(0);
        }
    }

    return DP_STATUS_RETURN_SUCCESS;
}


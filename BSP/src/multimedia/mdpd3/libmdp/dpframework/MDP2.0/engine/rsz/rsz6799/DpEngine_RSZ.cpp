#include "DpTileEngine.h"
#include "DpEngineType.h"
#include "DpTileScaler.h"
#include "mdp_reg_rsz.h"
#include "tile_mdp_reg.h"
#if CONFIG_FOR_VERIFY_FPGA
#include "ConfigInfo.h"
#endif

#ifndef BASIC_PACKAGE
#define ENABLE_PQ_RSZ (1)
#else
#define ENABLE_PQ_RSZ (0)
#endif // BASIC_PACKAGE

#if ENABLE_PQ_RSZ
#include "PQRszImpl.h"
#if CONFIG_FOR_OS_ANDROID
#include "cust_tdshp.h"

#include "PQSessionManager.h"
#include "PQAlgorithmFactory.h"
#if CONFIG_FOR_PROPERTY_SUPPORT
#include <cutils/properties.h>
#endif
#define ISP_DEBUG "persist.vendor.sys.isp.rsz.debug"
#endif // CONFIG_FOR_OS_ANDROID
#endif // ENABLE_PQ_RSZ


//--------------------------------------------------------
// Scaler driver engine
//--------------------------------------------------------
class DpEngine_SCL: public DpTileEngine
{
public:
    DpEngine_SCL(uint32_t identifier)
        : DpTileEngine(identifier),
          m_use121filter(m_data.m_use121filter),
          m_coeffStepX(m_data.m_coeffStepX),
          m_coeffStepY(m_data.m_coeffStepY),
          m_precisionX(m_data.m_precisionX),
          m_precisionY(m_data.m_precisionY),
          m_cropOffsetX(m_data.m_cropOffsetX),
          m_cropSubpixX(m_data.m_cropSubpixX),
          m_cropWidth(m_data.m_cropWidth),
          m_cropOffsetY(m_data.m_cropOffsetY),
          m_cropSubpixY(m_data.m_cropSubpixY),
          m_cropHeight(m_data.m_cropHeight),
          m_cropWidthSubpixel(0),
          m_cropHeightSubpixel(0),
          m_outRoiWidth(0),
          m_outRoiHeight(0),
          m_horDirScale(m_data.m_horDirScale),
          m_horAlgorithm(m_data.m_horAlgorithm),
          m_verDirScale(m_data.m_verDirScale),
          m_verAlgorithm(m_data.m_verAlgorithm),
          m_verticalFirst(m_data.m_verticalFirst),
          m_verCubicTrunc(m_data.m_verCubicTrunc)
    {
        m_pData = &m_data;
#if ENABLE_PQ_RSZ
#if CONFIG_FOR_PROPERTY_SUPPORT
        char c_isPqDebug[PROPERTY_VALUE_MAX];
        property_get(ISP_DEBUG, c_isPqDebug, "0");
        m_ispPqDebug = atoi(c_isPqDebug);
#endif
#endif
    }

    ~DpEngine_SCL()
    {
    }

private:
    MDP_PRZ_DATA                m_data;

    bool                        &m_use121filter;
    uint32_t                    &m_coeffStepX;
    uint32_t                    &m_coeffStepY;
    uint32_t                    &m_precisionX;
    uint32_t                    &m_precisionY;

    int32_t                     &m_cropOffsetX;
    int32_t                     &m_cropSubpixX;
    int32_t                     &m_cropWidth;
    int32_t                     &m_cropOffsetY;
    int32_t                     &m_cropSubpixY;
    int32_t                     &m_cropHeight;
    int32_t                     m_cropWidthSubpixel;
    int32_t                     m_cropHeightSubpixel;
    int32_t                     m_outRoiWidth;
    int32_t                     m_outRoiHeight;

    bool                        &m_horDirScale;
    DP_TILE_SCALER_ALGO_ENUM    &m_horAlgorithm;
    bool                        &m_verDirScale;
    DP_TILE_SCALER_ALGO_ENUM    &m_verAlgorithm;
    uint32_t                    &m_verticalFirst;
    uint32_t                    &m_verCubicTrunc;
#if ENABLE_PQ_RSZ
#if CONFIG_FOR_OS_ANDROID
    uint32_t                    m_ispPqDebug;
#endif
#endif
    DP_STATUS_ENUM onInitEngine(DpCommand&);

    DP_STATUS_ENUM onDeInitEngine(DpCommand&);

    DP_STATUS_ENUM onConfigFrame(DpCommand&,
                                 DpConfig&);

    DP_STATUS_ENUM onConfigTile(DpCommand&);

#ifdef MDP_RSZ_DISABLE_DCM_SMALL_TILE
    DP_STATUS_ENUM onAdvanceTile(DpCommand&);
#endif

    int32_t getRsz6TapTableIndex(int32_t stepSize);

    int64_t onQueryFeature()
    {
        return eSCL;
    }

#if ENABLE_PQ_RSZ
    void onCalcRsz(DpCommand &command, PQSession *pPQSession);
    void initInParam(RszInput *inParam, DpPqParam *pqParam);
#endif // ENABLE_PQ_RSZ
};


// Register factory function
static DpEngineBase* SCL0Factory(DpEngineType type)
{
    if (tSCL0 == type)
    {
        return new DpEngine_SCL(0);
    }
    return NULL;
};

// Register factory function
static DpEngineBase* SCL1Factory(DpEngineType type)
{
    if (tSCL1 == type)
    {
        return new DpEngine_SCL(1);
    }
    return NULL;
};

// Register factory function
static DpEngineBase* SCL2Factory(DpEngineType type)
{
    if (tSCL2 == type)
    {
        return new DpEngine_SCL(2);
    }
    return NULL;
};

// Register factory function
EngineReg SCL0Reg(SCL0Factory);
EngineReg SCL1Reg(SCL1Factory);
EngineReg SCL2Reg(SCL2Factory);


DP_STATUS_ENUM DpEngine_SCL::onInitEngine(DpCommand &command)
{
    // Reset engine
    MM_REG_WRITE(command, PRZ_ENABLE, 0x00010000, 0x00010000);
    MM_REG_WRITE(command, PRZ_ENABLE, 0x00000000, 0x00010000);

    // Enable engine
    MM_REG_WRITE(command, PRZ_ENABLE, 0x00000001, 0x00000001);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_SCL::onDeInitEngine(DpCommand &command)
{
    // Disable engine
    MM_REG_WRITE(command, PRZ_ENABLE, 0x00000000, 0x00000001);

    return DP_STATUS_RETURN_SUCCESS;
}


int32_t DpEngine_SCL::getRsz6TapTableIndex(int32_t stepSize)
{
    int32_t table_idx = 0;

    if (stepSize <= 32768) // resize ratio >=1
    {
        table_idx = 27;
    }
    else if (stepSize < 36409)  // resize ratio >= 0.9
    {
        table_idx = 20;
    }
    else if (stepSize < 40961) // resize ratio >= 0.8
    {
        table_idx = 21;
    }
    else if (stepSize < 46812) // resize ratio >= 0.7
    {
        table_idx = 22;
    }
    else if (stepSize < 54614) // resize ratio >= 0.6
    {
        table_idx = 23;
    }
    else if (stepSize < 59579) // resize ratio >= 0.55
    {
        table_idx = 24;
    }
    else if (stepSize < 65537) // resize ratio >= 0.5
    {
        table_idx = 25;
    }
    else                       // resize ratio < 0.5
    {
        table_idx = 26;
    }

    return table_idx;
}

DP_STATUS_ENUM DpEngine_SCL::onConfigFrame(DpCommand &command,
                                           DpConfig  &config)
{
    //DP_STATUS_ENUM status;
    uint32_t controlVal;
    int32_t  horTable;
    int32_t  verTable;

    int32_t  cropWidth;
    int32_t  outWidth;
    int32_t  cropHeight;
    int32_t  outHeight;

    m_cropOffsetX = config.inXOffset;
    m_cropSubpixX = config.inXSubpixel;
    m_cropOffsetY = config.inYOffset;
    m_cropSubpixY = config.inYSubpixel;
    m_cropWidth   = config.inCropWidth;
    m_cropWidthSubpixel   = config.inCropWidthSubpixel;
    m_cropHeight  = config.inCropHeight;
    m_cropHeightSubpixel  = config.inCropHeightSubpixel;

    m_outRoiWidth = config.outRoiWidth;
    m_outRoiHeight = config.outRoiHeight;

    horTable = config.rszHorTable;
    verTable = config.rszVerTable;

    if (0 == m_cropWidth)
    {
        m_cropWidth = m_inFrameWidth;
    }

    if (0 == m_cropHeight)
    {
        m_cropHeight = m_inFrameHeight;
    }

#if ENABLE_PQ_RSZ
    command.setPQSessionID(config.pqSessionId);
#endif // ENABLE_PQ_RSZ

    if ((config.rootAndLeaf == 2) && // 1-in + 1-out = 2
        (m_cropWidth     == m_inFrameWidth) &&
        (m_inFrameWidth  == m_outFrameWidth) &&
        (m_cropHeight    == m_inFrameHeight) &&
        (m_inFrameHeight == m_outFrameHeight) &&
        (m_cropSubpixX   == 0) &&
        (m_cropSubpixY   == 0))
    {
        m_bypassEngine = true;

        MM_REG_WRITE(command, PRZ_ENABLE, 0x00000000, 0x00000001);

        return DP_STATUS_RETURN_SUCCESS;
    }
    else
    {
        m_bypassEngine = false;
    }

#ifdef HW_SUPPORT_10BIT_PATH
    // 10 bit format
    if ((DP_COLOR_GET_10BIT_PACKED(config.inFormat) || DP_COLOR_GET_10BIT_LOOSE(config.inFormat)) &&
        (DP_COLOR_GET_10BIT_PACKED(config.outFormat) || DP_COLOR_GET_10BIT_LOOSE(config.outFormat)))
    {
        MM_REG_WRITE(command, PRZ_CONTROL_2, 0 << 9, 0x200);
    }
    else
    {
        MM_REG_WRITE(command, PRZ_CONTROL_2, 1 << 9, 0x200);
    }
#endif // HW_SUPPORT_10BIT_PATH

    // C42 conversion: drop if source is YUV422 or YUV420; ISP is YUV422
    m_use121filter = !DP_COLOR_GET_H_SUBSAMPLE(config.inFormat) && !config.enISP;

#if ENABLE_PQ_RSZ
    PQSession* pPQSession = PQSessionManager::getInstance()->getPQSession(config.pqSessionId);
    onCalcRsz(command, pPQSession);

    return DP_STATUS_RETURN_SUCCESS;
#endif // ENABLE_PQ_RSZ
#if CONFIG_FOR_VERIFY_FPGA
    prz_config &prz = *(prz_config*)config.pEngine_cfg;

    if (1 == prz.config_en)
    {
        MM_REG_WRITE(command, PRZ_ENABLE, prz.rsz_en << 0, 0x00000001);
        command.addMetLog("MDP_RSZ__RSZ_ENABLE", prz.rsz_en);

        MM_REG_WRITE(command, PRZ_CONTROL_1, prz.rsz_c42_interp_en           << 26 |
                                             prz.rsz_vertical_table_select   << 21 |
                                             prz.rsz_horizontal_table_select << 16 |
                                             prz.rsz_truncation_bit_v        << 13 |
                                             prz.rsz_truncation_bit_h        << 10 |
                                             prz.rsz_lbcson_mode             <<  9 |
                                             prz.rsz_vertical_algorithm      <<  7 |
                                             prz.rsz_horizontal_algorithm    <<  5 |
                                             prz.rsz_vertical_first          <<  4 |
                                             prz.rsz_vertical_en             <<  1 |
                                             prz.rsz_horizontal_en           <<  0 , 0x07fffff3);

        command.addMetLog("MDP_RSZ__RSZ_CONTROL_1",   prz.rsz_c42_interp_en           << 26 |
                                             prz.rsz_vertical_table_select   << 21 |
                                             prz.rsz_horizontal_table_select << 16 |
                                             prz.rsz_truncation_bit_v        << 13 |
                                             prz.rsz_truncation_bit_h        << 10 |
                                             prz.rsz_lbcson_mode             <<  9 |
                                             prz.rsz_vertical_algorithm      <<  7 |
                                             prz.rsz_horizontal_algorithm    <<  5 |
                                             prz.rsz_vertical_first          <<  4 |
                                             prz.rsz_vertical_en             <<  1 |
                                             prz.rsz_horizontal_en           <<  0);
        command.addMetLog("MDP_RSZ__RSZ_INPUT_IMAGE", (m_inFrameHeight   << 16) +
                                             (m_inFrameWidth    <<  0));
        command.addMetLog("MDP_RSZ__RSZ_OUTPUT_IMAGE", (m_outFrameHeight << 16) +
                                              (m_outFrameWidth  <<  0));

        MM_REG_WRITE(command, PRZ_CONTROL_2, prz.rsz_ver_cubic_trunc_en         << 27 |
                                             prz.rsz_ver_luma_cubic_trunc_bit   << 24 |
                                             prz.rsz_ver_chroma_cubic_trunc_bit << 21 |
                                             prz.rsz_hor_cubic_trunc_en         << 20 |
                                             prz.rsz_hor_luma_cubic_trunc_bit   << 17 |
                                             prz.rsz_hor_chroma_cubic_trunc_bit << 14 |
                                             prz.rsz_power_saving               <<  9 |
                                             prz.rsz_tap_adapt_en               <<  7 |
                                             prz.rsz_ibse_ylevel_en             <<  6 |
                                             prz.rsz_ibse_clip_en               <<  5 |
                                             prz.rsz_ibse_en                    <<  4 |
                                             prz.rsz_demo_mode                  <<  2 |
                                             prz.rsz_demo_swap                  <<  1 |
                                             prz.rsz_demo_en                    <<  0 , 0x0fffc2ff);

        MM_REG_WRITE(command, PRZ_HORIZONTAL_COEFF_STEP, prz.rsz_horizontal_coeff_step, 0x07fffff);
        MM_REG_WRITE(command, PRZ_VERTICAL_COEFF_STEP,   prz.rsz_vertical_coeff_step,   0x07fffff);

        MM_REG_WRITE(command, PRZ_TAP_ADAPT, prz.rsz_tap_adapt_edge_thr       << 20 |
                                             prz.rsz_tap_adapt_dc_coring      << 15 |
                                             prz.rsz_tap_adapt_var_coring     << 10 |
                                             prz.rsz_tap_adapt_fallback_ratio <<  4 |
                                             prz.rsz_tap_adapt_slope          <<  0 , 0x03ffffff);

        MM_REG_WRITE(command, PRZ_IBSE_SOFTCLIP, prz.rsz_ibse_clip_ratio   << 15 |
                                                 prz.rsz_ibse_clip_thr     <<  7 |
                                                 prz.rsz_ibse_gain_mid     <<  2 |
                                                 prz.rsz_ibse_tbl_idx_mid  <<  0 , 0x0fffff);
        MM_REG_WRITE(command, PRZ_IBSE_YLEVEL_1, prz.rsz_ibse_ylevel_p0    << 24 |
                                                 prz.rsz_ibse_ylevel_p16   << 16 |
                                                 prz.rsz_ibse_ylevel_p32   <<  8 |
                                                 prz.rsz_ibse_ylevel_p48   <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_IBSE_YLEVEL_2, prz.rsz_ibse_ylevel_p64   << 24 |
                                                 prz.rsz_ibse_ylevel_p80   << 16 |
                                                 prz.rsz_ibse_ylevel_p96   <<  8 |
                                                 prz.rsz_ibse_ylevel_p112  <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_IBSE_YLEVEL_3, prz.rsz_ibse_ylevel_p128  << 24 |
                                                 prz.rsz_ibse_ylevel_p144  << 16 |
                                                 prz.rsz_ibse_ylevel_p160  <<  8 |
                                                 prz.rsz_ibse_ylevel_p176  <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_IBSE_YLEVEL_4, prz.rsz_ibse_ylevel_p192  << 24 |
                                                 prz.rsz_ibse_ylevel_p208  << 16 |
                                                 prz.rsz_ibse_ylevel_p224  <<  8 |
                                                 prz.rsz_ibse_ylevel_p240  <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_IBSE_YLEVEL_5, prz.rsz_ibse_ylevel_p256  <<  8 |
                                                 prz.rsz_ibse_ylevel_alpha <<  0 , 0x0ff3f);

        MM_REG_WRITE(command, PRZ_IBSE_GAINCONTROL_1, prz.rsz_ibse_gaincontrol_coring_zero     << 24 |
                                                      prz.rsz_ibse_gaincontrol_coring_thr      << 16 |
                                                      prz.rsz_ibse_gaincontrol_coring_value    <<  8 |
                                                      prz.rsz_ibse_gaincontrol_gain            <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_IBSE_GAINCONTROL_2, prz.rsz_ibse_gaincontrol_softcoring_gain << 24 |
                                                      prz.rsz_ibse_gaincontrol_limit           << 16 |
                                                      prz.rsz_ibse_gaincontrol_bound           <<  8 |
                                                      prz.rsz_ibse_gaincontrol_softlimit_ratio <<  0 , 0x0fffff0f);

        MM_REG_WRITE(command, PRZ_DEMO_IN_HMASK,  prz.rsz_demo_in_mask_hstart  << 16 |
                                                  prz.rsz_demo_in_mask_hend    <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_DEMO_IN_VMASK,  prz.rsz_demo_in_mask_vstart  << 16 |
                                                  prz.rsz_demo_in_mask_vend    <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_DEMO_OUT_HMASK, prz.rsz_demo_out_mask_hstart << 16 |
                                                  prz.rsz_demo_out_mask_hend   <<  0 , 0xffffffff);
        MM_REG_WRITE(command, PRZ_DEMO_OUT_VMASK, prz.rsz_demo_out_mask_vstart << 16 |
                                                  prz.rsz_demo_out_mask_vend   <<  0 , 0xffffffff);

        MM_REG_WRITE(command, PRZ_ATPG, prz.RSZ_ATPG_CT << 1 |
                                        prz.RSZ_ATPG_OB << 0 , 0x03);

        MM_REG_WRITE(command, PRZ_PAT1_GEN_SET,      prz.reg1_pat_type   << 16 |
                                                     prz.reg1_grid_size  <<  4 |
                                                     prz.reg1_grid_show  <<  2 |
                                                     prz.reg1_pat_gen_en <<  0 , 0x0ff00fd);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_FRM_SIZE, prz.reg1_frm_size_v << 16 |
                                                     prz.reg1_frm_size_h <<  0 , 0x1fff1fff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_COLOR0,   prz.reg1_color_u    << 16 |
                                                     prz.reg1_color_y    <<  0 , 0x0ff00ff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_COLOR1,   prz.reg1_bg_color_y << 16 |
                                                     prz.reg1_color_v    <<  0 , 0x0ff00ff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_COLOR2,   prz.reg1_bg_color_v << 16 |
                                                     prz.reg1_bg_color_u <<  0 , 0x0ff00ff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_POS,      prz.reg1_pos_y << 16 |
                                                     prz.reg1_pos_x <<  0 , 0x1fff1fff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_TILE_POS, prz.reg1_tile_pos_y << 16 |
                                                     prz.reg1_tile_pos_x <<  0 , 0x1fff1fff);
        MM_REG_WRITE(command, PRZ_PAT1_GEN_TILE_OV,  prz.reg1_tile_ov_y  <<  8 |
                                                     prz.reg1_tile_ov_x  <<  0 , 0x0ffff);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_SET,      prz.reg2_pat_type << 16 |
                                                     prz.reg2_cursor_show <<  1 |
                                                     prz.reg2_pat_gen_en  <<  0 , 0x0ff0003);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_COLOR0,   prz.reg2_color_u    << 16 |
                                                     prz.reg2_color_y    <<  0 , 0x0ff00ff);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_COLOR1,   prz.reg2_color_v    <<  0 , 0x0ff);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_POS,      prz.reg2_pos_y << 16 |
                                                     prz.reg2_pos_x <<  0 , 0x1fff1fff);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_TILE_POS, prz.reg2_tile_pos_y << 16 |
                                                     prz.reg2_tile_pos_x <<  0 , 0x1fff1fff);
        MM_REG_WRITE(command, PRZ_PAT2_GEN_TILE_OV,  prz.reg2_tile_ov_y  <<  8 |
                                                     prz.reg2_tile_ov_x  <<  0 , 0x0ffff);

        m_bypassEngine = !prz.rsz_en;
        m_use121filter = prz.rsz_c42_interp_en;

        m_horAlgorithm = (DP_TILE_SCALER_ALGO_ENUM)prz.rsz_horizontal_algorithm;
        m_horDirScale = prz.rsz_horizontal_en;
        m_precisionX = (m_horAlgorithm == DP_TILE_SCALER_ALG0_6_TAPS) ? (1 << 15) : (1 << 20);
        m_coeffStepX = prz.rsz_horizontal_coeff_step;

        m_verAlgorithm = (DP_TILE_SCALER_ALGO_ENUM)prz.rsz_vertical_algorithm;
        m_verDirScale = prz.rsz_vertical_en;
        m_precisionY = (m_verAlgorithm == DP_TILE_SCALER_ALG0_6_TAPS) ? (1 << 15) : (1 << 20);
        m_coeffStepY = prz.rsz_vertical_coeff_step;

        m_verticalFirst = prz.rsz_vertical_first;
        m_verCubicTrunc = prz.rsz_ver_cubic_trunc_en;

        return DP_STATUS_RETURN_SUCCESS;
    }
#endif // CONFIG_FOR_VERIFY_FPGA

    controlVal = 0;

    if (m_outFrameWidth == m_cropWidth)
    {
        m_horAlgorithm  = DP_TILE_SCALER_ALG0_6_TAPS;
        m_horDirScale   = true;

        // Setup coeffstep and precision
        m_precisionX = (1 << 15);
        m_coeffStepX = m_precisionX;
        horTable        = 27;
    }
    else
    {
        if (m_cropWidth <= 1)
        {
            cropWidth = 2;
        }
        else
        {
            cropWidth = m_cropWidth;
        }

        if (m_outFrameWidth <= 1)
        {
            outWidth = 2;
        }
        else
        {
            outWidth = m_outFrameWidth;
        }

        if ((m_cropWidth - 1) < (m_outFrameWidth - 1))      // ratio > (1)
        {
            m_horAlgorithm  = DP_TILE_SCALER_ALG0_6_TAPS;
            m_horDirScale   = true;

            // Setup coeffstep and precision
            m_precisionX = (1 << 15);
            m_coeffStepX = (int32_t)((float)(cropWidth - 1) * m_precisionX / (outWidth - 1) + 0.5);
            horTable        = getRsz6TapTableIndex(m_coeffStepX);
        }
        else if ((m_cropWidth - 1) < 24 * (m_outFrameWidth - 1))    // (1/24) < ratio < (1)
        {
            m_horAlgorithm  = DP_TILE_SCALER_ALGO_CUB_ACC;
            m_horDirScale   = true;

            // Setup coeffstep and precision
            m_precisionX = (1 << 20);
            m_coeffStepX = (int32_t)((float)(outWidth - 1) * m_precisionX / (cropWidth - 1) + 1);
            horTable        = 17; // table can be selected between 0~19 by request
        }
        else    // (1/24) > ratio
        {
            m_horAlgorithm  = DP_TILE_SCALER_ALG0_SRC_ACC;
            m_horDirScale   = true;

            // Setup coeffstep and precision
            m_precisionX = (1 << 20);
            m_coeffStepX = (int32_t)((float)(outWidth - 1) * m_precisionX / (cropWidth - 1) + 1);
            horTable        = 0;
        }
    }

    MM_REG_WRITE(command, PRZ_HORIZONTAL_COEFF_STEP, m_coeffStepX, 0x007FFFFF);

    if (m_outFrameHeight == m_cropHeight)
    {
        m_verAlgorithm  = DP_TILE_SCALER_ALG0_6_TAPS;
        m_verDirScale   = true;

        // Setup coeffstep and precision
        m_precisionY = (1 << 15);
        m_coeffStepY = m_precisionY;
        verTable        = 27;
    }
    else
    {
        if (m_cropHeight <= 1)
        {
            cropHeight = 2;
        }
        else
        {
            cropHeight = m_cropHeight;
        }

        if (m_outFrameHeight <= 1)
        {
            outHeight = 2;
        }
        else
        {
            outHeight = m_outFrameHeight;
        }

        if ((m_cropHeight - 1) < (m_outFrameHeight - 1))    // ratio > (1)
        {
            m_verAlgorithm  = DP_TILE_SCALER_ALG0_6_TAPS;
            m_verDirScale   = true;

            // Setup coeffstep and precision
            m_precisionY = (1 << 15);
            m_coeffStepY = (int32_t)((float)(cropHeight - 1) * m_precisionY / (outHeight - 1) + 0.5);
            verTable        = getRsz6TapTableIndex(m_coeffStepY);
        }
        else if ((m_cropHeight - 1) < 24 * (m_outFrameHeight - 1))  // (1/24) < ratio < (1)
        {
            m_verAlgorithm  = DP_TILE_SCALER_ALGO_CUB_ACC;
            m_verDirScale   = true;

            // Setup coeffstep and precision
            m_precisionY = (1 << 20);
            m_coeffStepY = (int32_t)((float)(outHeight - 1) * m_precisionY / (cropHeight - 1) + 1);
            verTable        = 17; // table can be selected between 0~19 by request
        }
        else    // (1/24) > ratio
        {
            m_verAlgorithm  = DP_TILE_SCALER_ALG0_SRC_ACC;
            m_verDirScale   = true;

            // Setup coeffstep and precision
            m_precisionY = (1 << 20);
            m_coeffStepY = (int32_t)((float)(outHeight - 1) * m_precisionY / (cropHeight - 1) + 1);
            verTable        = 0;
        }
    }

    MM_REG_WRITE(command, PRZ_VERTICAL_COEFF_STEP, m_coeffStepY, 0x007FFFFF);

    // Vertical first
    controlVal |= (m_outFrameWidth > m_cropWidth)? (1 << 4): 0;

    controlVal |= (m_horAlgorithm << 5);
    controlVal |= (horTable << 16);

    controlVal |= (m_verAlgorithm << 7);
    controlVal |= (verTable << 21);

    controlVal |= (true == m_horDirScale)? (1 << 0): 0;
    controlVal |= (true == m_verDirScale)? (1 << 1): 0;
    controlVal |= (true == m_use121filter)? (1 << 26): 0;

    MM_REG_WRITE(command, PRZ_CONTROL_1, controlVal, 0x07FF01F3);

    DPLOGI("DpEngine_RSZ: H CoeffStep %d Table %d\n", m_coeffStepX, horTable);
    DPLOGI("DpEngine_RSZ: V CoeffStep %d Table %d\n", m_coeffStepY, verTable);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_SCL::onConfigTile(DpCommand &command)
{
    uint32_t PRZ_drs_padding_dis;
    uint32_t PRZ_drs_lclip_en;
    uint32_t PRZ_input_image_w;
    uint32_t PRZ_input_image_h;
    uint32_t PRZ_luma_hor_int_ofst;
    uint32_t PRZ_luma_hor_subpix_ofst;
    uint32_t PRZ_luma_ver_int_ofst;
    uint32_t PRZ_luma_ver_subpix_ofst;
    uint32_t PRZ_chroma_hor_int_ofst;
    uint32_t PRZ_chroma_hor_subpix_ofst;
    uint32_t PRZ_output_image_w;
    uint32_t PRZ_output_image_h;
    uint32_t PRZ_urs_clip_en;

    // YUV444->YUV422 downsampler
    if (0 == (m_inTileXRight & 0x1))
    {
        // Odd coordinate, should pad 1 column
        PRZ_drs_padding_dis = 0;
    }
    else
    {
        // Even coordinate, no padding required
        PRZ_drs_padding_dis = 1;
    }

    if ((true == m_use121filter) && (m_inTileXLeft > 0))
    {
        PRZ_drs_lclip_en = 1;
    }
    else
    {
        PRZ_drs_lclip_en = 0;
    }

    PRZ_input_image_w = m_inTileXRight - m_inTileXLeft + 1;
    PRZ_input_image_h = m_inTileYBottom - m_inTileYTop + 1;

    PRZ_luma_hor_int_ofst      = m_lumaXOffset;
    PRZ_luma_hor_subpix_ofst   = m_lumaXSubpixel;
    PRZ_luma_ver_int_ofst      = m_lumaYOffset;
    PRZ_luma_ver_subpix_ofst   = m_lumaYSubpixel;
    PRZ_chroma_hor_int_ofst    = m_chromaXOffset;
    PRZ_chroma_hor_subpix_ofst = m_chromaXSubpixel;

    PRZ_output_image_w = m_outTileXRight - m_outTileXLeft + 1;
    PRZ_output_image_h = m_outTileYBottom - m_outTileYTop + 1;

    // YUV422->YUV444 upsampler
    if (m_outTileXRight >= (m_outFrameWidth - 1))
    {
        PRZ_urs_clip_en = 0;
    }
    else
    {
        PRZ_urs_clip_en = 1;
    }

    MM_REG_WRITE(command, PRZ_CONTROL_2, (PRZ_drs_lclip_en    << 11) +
                                         (PRZ_drs_padding_dis << 12) +
                                         (PRZ_urs_clip_en     << 13), 0x00003800);

    MM_REG_WRITE(command, PRZ_INPUT_IMAGE, (PRZ_input_image_h   << 16) +
                                           (PRZ_input_image_w   <<  0), 0xFFFFFFFF);

#ifdef MDP_RSZ_DISABLE_DCM_SMALL_TILE
    if (PRZ_input_image_w <= 16)
    {
        MM_REG_WRITE(command, PRZ_CONTROL_1, 1 << 27, 1 << 27); //rsz_dcm_dis
    }
#endif

    MM_REG_WRITE(command, PRZ_LUMA_HORIZONTAL_INTEGER_OFFSET, PRZ_luma_hor_int_ofst, 0x0000FFFF);
    MM_REG_WRITE(command, PRZ_LUMA_HORIZONTAL_SUBPIXEL_OFFSET, PRZ_luma_hor_subpix_ofst, 0x001FFFFF);
    MM_REG_WRITE(command, PRZ_LUMA_VERTICAL_INTEGER_OFFSET, PRZ_luma_ver_int_ofst, 0x0000FFFF);
    MM_REG_WRITE(command, PRZ_LUMA_VERTICAL_SUBPIXEL_OFFSET, PRZ_luma_ver_subpix_ofst, 0x001FFFFF);
    MM_REG_WRITE(command, PRZ_CHROMA_HORIZONTAL_INTEGER_OFFSET, PRZ_chroma_hor_int_ofst, 0x0000FFFF);
    MM_REG_WRITE(command, PRZ_CHROMA_HORIZONTAL_SUBPIXEL_OFFSET, PRZ_chroma_hor_subpix_ofst, 0x001FFFFF);

    MM_REG_WRITE(command, PRZ_OUTPUT_IMAGE, (PRZ_output_image_h << 16) +
                                            (PRZ_output_image_w << 0), 0xFFFFFFFF);

    return DP_STATUS_RETURN_SUCCESS;
}

#ifdef MDP_RSZ_DISABLE_DCM_SMALL_TILE
DP_STATUS_ENUM DpEngine_SCL::onAdvanceTile(DpCommand &command)
{
    uint32_t PRZ_input_image_w;

    PRZ_input_image_w = m_inTileXRight - m_inTileXLeft + 1;

    if (PRZ_input_image_w <= 16)
    {
        MM_REG_WRITE(command, PRZ_CONTROL_1, 0 << 27, 1 << 27); //rsz_dcm_dis
    }

    return DP_STATUS_RETURN_SUCCESS;
}
#endif

#if ENABLE_PQ_RSZ
void DpEngine_SCL::onCalcRsz(DpCommand &command, PQSession *pPQSession)
{
    int32_t tmp, q = 0;
    RszInput inParam;
    RszOutput outParam;

    memset(&inParam, 0, sizeof(RszInput));
    memset(&outParam, 0, sizeof(RszOutput));

    DpPqParam pqParam;

    if (pPQSession != NULL)
    {
        pPQSession->getPQParam(&pqParam);
    }
    else
    {
        pqParam.u.isp.isIspScenario = 0;
        pqParam.u.isp.ispScenario = 0;
    }

    initInParam(&inParam, &pqParam);

    PQRSZAdaptor* pPQRSZAdaptor = PQAlgorithmFactory::getInstance()->getRSZ(m_identifier);

    pPQRSZAdaptor->calRegs(pPQSession, command, &inParam, &outParam);

    tmp = m_cropSubpixX + m_cropWidthSubpixel;
    q = tmp/0x00100000 + (tmp % 0x00100000 != 0);
    if(m_cropOffsetX + m_cropWidth + q <= m_inFrameWidth )
    {
        m_cropWidth = m_cropWidth + q;
    }
    else
    {
        m_cropWidth = m_inFrameWidth - m_cropOffsetX;
    }
    tmp = m_cropSubpixY + m_cropHeightSubpixel;
    q = tmp/0x00100000 + (tmp % 0x00100000 != 0);
    if(m_cropOffsetY + m_cropHeight + q <= m_inFrameHeight )
    {
        m_cropHeight = m_cropHeight + q;
    }
    else
    {
        m_cropHeight = m_inFrameHeight - m_cropOffsetY;
    }

    if (true == m_use121filter)
    {
        MM_REG_WRITE(command, PRZ_CONTROL_1, 1 << 26, 0x04000000);
    }

    // Need for tile calculation
    m_horAlgorithm  = (DP_TILE_SCALER_ALGO_ENUM)outParam.horAlgo;
    m_horDirScale   = outParam.horEnable;
    m_precisionX = outParam.precX;
    m_coeffStepX = outParam.coeffStepX;

    m_verAlgorithm  = (DP_TILE_SCALER_ALGO_ENUM)outParam.verAlgo;
    m_verDirScale   = outParam.verEnable;
    m_precisionY = outParam.precY;
    m_coeffStepY = outParam.coeffStepY;

    m_verticalFirst = outParam.verticalFirst;
    m_verCubicTrunc = outParam.verCubicTruncEn;

    /*
     ****** 6-tap (zoom in) ******
     * in source domain
     * coeff = floor(src/tar*prec) => src*prec = tar*coeff
     *
     *        | Input    | FW (as HW)             | SW (as input)                | Tile (for calculation) |
     * -------|----------|------------------------|------------------------------|------------------------|
     * offset | src      |             = src      |                   = src      | *prec       = src*prec |
     * subpix | src*2^20 | *prec/2^20  = src*prec | floor(*2^20/prec) = src*2^20 | *prec/2^20  = src*prec |
     * output | tar      |             = tar      |                   = tar      | *coeff      = src*prec |
     *
     ****** 4n-/n-tap (zoom out) ******
     * in target domain
     * coeff = ceil(tar/src*prec)  => src*coeff = tar*prec
     *                             => [src*2^20] = ceil((tar_off*prec + tar_sub) *2^20 /coeff)
     *                             => [src*2^20] = int((tar_off*prec + tar_sub) *2^20 + coeff-1) /coeff
     * src_off = [src*2^20] /2^20
     * src_sub = [src*2^20] - src_off *2^20
     *
     *        | Input    | FW (as HW)             | SW (as input)                | Tile (for calculation) |
     * -------|----------|------------------------|------------------------------|------------------------|
     * offset | src      | *coeff/prec = tar      | src_off           ~ src      | *coeff      = tar*prec |
     * subpix | src*2^20 | *coeff/2^20 = tar*prec | src_sub           ~ src*2^20 | *coeff/2^20 = tar*prec |
     * output | tar      |             = tar      |                   = tar      | *prec       = tar*prec |
     */

    if (DP_TILE_SCALER_ALG0_6_TAPS == m_horAlgorithm)
    {
        m_cropOffsetX = outParam.horLumaIntOffset;
        m_cropSubpixX = ((int64_t)outParam.horLumaSubOffset << TILE_SCALER_SUBPIXEL_SHIFT) / outParam.precX;
    }
    else // (1 << TILE_SCALER_SUBPIXEL_SHIFT) == outParam.precX
    {
        int64_t subpixX;
        subpixX = ((int64_t)outParam.horLumaIntOffset << TILE_SCALER_SUBPIXEL_SHIFT) + outParam.horLumaSubOffset;
        subpixX = ((subpixX << TILE_SCALER_SUBPIXEL_SHIFT) + outParam.coeffStepX - 1) / outParam.coeffStepX;

        m_cropOffsetX = subpixX >> TILE_SCALER_SUBPIXEL_SHIFT;
        m_cropSubpixX = subpixX - ((int64_t)m_cropOffsetX << TILE_SCALER_SUBPIXEL_SHIFT);
    }

    if (DP_TILE_SCALER_ALG0_6_TAPS == m_verAlgorithm)
    {
        m_cropOffsetY = outParam.verLumaIntOffset;
        m_cropSubpixY = ((int64_t)outParam.verLumaSubOffset << TILE_SCALER_SUBPIXEL_SHIFT) / outParam.precY;
    }
    else // (1 << TILE_SCALER_SUBPIXEL_SHIFT) == outParam.precY
    {
        int64_t subpixY;
        subpixY = ((int64_t)outParam.verLumaIntOffset << TILE_SCALER_SUBPIXEL_SHIFT) + outParam.verLumaSubOffset;
        subpixY = ((subpixY << TILE_SCALER_SUBPIXEL_SHIFT) + outParam.coeffStepY - 1) / outParam.coeffStepY;

        m_cropOffsetY = subpixY >> TILE_SCALER_SUBPIXEL_SHIFT;
        m_cropSubpixY = subpixY - ((int64_t)m_cropOffsetY << TILE_SCALER_SUBPIXEL_SHIFT);
    }
}

void DpEngine_SCL::initInParam(RszInput *inParam, DpPqParam *pqParam)
{
    // Prepare RszInput parameter
    inParam->srcWidth = m_inFrameWidth;
    inParam->srcHeight = m_inFrameHeight;
    inParam->dstWidth = m_outRoiWidth;
    inParam->dstHeight = m_outRoiHeight;
    inParam->cropOffsetX = m_cropOffsetX;
    inParam->cropSubpixX = m_cropSubpixX;
    inParam->cropOffsetY = m_cropOffsetY;
    inParam->cropSubpixY = m_cropSubpixY;
    inParam->cropWidth = m_cropWidth;
    inParam->cropSubpixWidth = m_cropWidthSubpixel;
    inParam->cropHeight = m_cropHeight;
    inParam->cropSubpixHeight = m_cropHeightSubpixel;

    inParam->enable = 1; //from comment
    inParam->yuv422Tyuv444 = 0; //from comment
    inParam->demoEnable = 0; //from comment
    inParam->powerSavingMode = 1;
    inParam->is_ispScenario = pqParam->u.isp.isIspScenario;
    inParam->ispScenario = pqParam->u.isp.ispScenario;
    inParam->inISOSpeed = pqParam->u.isp.iso;
#if CONFIG_FOR_OS_ANDROID
    inParam->DebugFlagsRSZ = m_ispPqDebug;

    if (m_ispPqDebug != 0)
    {
        DPLOGD("DpEngine_RSZ: inParam->is_ispScenario = [%d], inParam->ispScenario = [%d]", inParam->is_ispScenario, inParam->ispScenario);
    }
#else
    inParam->DebugFlags = 0;
#endif
}
#endif // ENABLE_PQ_RSZ

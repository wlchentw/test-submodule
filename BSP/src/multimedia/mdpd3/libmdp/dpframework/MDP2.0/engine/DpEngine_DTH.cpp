#include "DpTileEngine.h"
#include "DpEngineType.h"
#include "mdp_reg_dth.h"
#include "mmsys_config.h"
#if CONFIG_FOR_VERIFY_FPGA
#include "ConfigInfo.h"
#endif

#if CONFIG_FOR_OS_ANDROID
#include "PQSessionManager.h"
#endif

//------------------------------------------------------------
// Dummy DTH driver engine
//-------------------------------------------------------------
class DpEngine_DTH: public DpTileEngine
{
public:
    DpEngine_DTH(uint32_t identifier)
        : DpTileEngine(identifier)
    {
    }

    ~DpEngine_DTH()
    {
    }

private:
    DP_STATUS_ENUM onInitEngine(DpCommand&);

    DP_STATUS_ENUM onDeInitEngine(DpCommand&);

    DP_STATUS_ENUM onConfigFrame(DpCommand&,
                                 DpConfig&);

    DP_STATUS_ENUM onConfigTile(DpCommand&);

    int64_t onQueryFeature()
    {
        return eDTH;
    }

};

// register factory function
static DpEngineBase* DTHFactory(DpEngineType type)
{
    if (tDTH == type)
    {
        return new DpEngine_DTH(0);
    }
    return NULL;
};

// register factory function
EngineReg DTHReg(DTHFactory);

DP_STATUS_ENUM DpEngine_DTH::onInitEngine(DpCommand &command)
{
    //DTH enable
    MM_REG_WRITE(command, MDP_DTH_EN, 0x1, 0x1);
    command.addMetLog("MDP_DTH_EN", 1);

    //Relay mode
    MM_REG_WRITE(command, MDP_DTH_CFG, 0x1, 0x1);
    command.addMetLog("MDP_DTH_CFG", 1);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_DTH::onDeInitEngine(DpCommand &command)
{
    // Disable DTH

    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM DpEngine_DTH::onConfigFrame(DpCommand &command, DpConfig &config)
{
    PQSession* pPQSession = PQSessionManager::getInstance()->getPQSession(config.pqSessionId);
    if (pPQSession != NULL)
    {
	uint32_t cfg_val = 0;
	uint32_t in_bit; //0:8bit 1:4bit
	uint32_t out_bit; //1:4bit 2:2bit 3:1bit
	uint32_t algo; //0:Quantization 1:Bayer 2:Sierra lite
    	DpPqParam  PqParam;
    	pPQSession->getPQParam(&PqParam);

#if 0 //do invert in gamma
	if (PqParam.invert != 0)
	{
		cfg_val |= (0x1 << 1);
		cfg_val |= (0x1 << 10);
		MM_REG_WRITE(command, MDP_DTH_EN, 0x2, 0x3);
	}
#endif
	DPLOGI("DTH: dth_enable=%d dth_algo=%d\n", PqParam.dth_enable, PqParam.dth_algo);
	if (PqParam.dth_enable != 0)
	{
		in_bit = (PqParam.dth_algo & 0x00010000) >> 16;
		out_bit = (PqParam.dth_algo & 0x00000300) >> 8;
		algo = PqParam.dth_algo & 0x00000003;

		cfg_val =
			(0 << 12) | //y4_in_mode
			(1 << 11) | //Sierra_err_mode
			(0 << 10) | //inverse out
			(out_bit << 8) | //out bits,reg_drmode
			(in_bit << 7) | //in bits,dth_4b_switch
			(algo << 5) | //algo,dth_mode
			(1 << 1) | //enable dth
			0; //Relay mode disable

		DPLOGI("DTH: %d Enabled IN:%dbits OUT:%dbits Algo:%s\n",
				cfg_val,
				(in_bit ? 4 : 8),
				(out_bit == 1 ? 4 : (out_bit == 2 ? 2 : 1)),
				algo == 2 ? "Sierra lite" : (algo ? "Bayer" : "Quantization")
				);

	} else {
		cfg_val = 1;
		DPLOGI("DTH: Disabled\n");
	}
	MM_REG_WRITE(command, MDP_DTH_CFG, cfg_val, 0x1FE3);

    }
    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_DTH::onConfigTile(DpCommand &command)
{
    uint32_t DTH_hsize;
    uint32_t DTH_vsize;

    DTH_hsize      = m_inTileXRight   - m_inTileXLeft + 1;
    DTH_vsize      = m_inTileYBottom  - m_inTileYTop  + 1;

    MM_REG_WRITE(command, MDP_DTH_SIZE, (DTH_hsize << 16) +
                                          (DTH_vsize <<  0), 0x1FFF1FFF);

    return DP_STATUS_RETURN_SUCCESS;
}

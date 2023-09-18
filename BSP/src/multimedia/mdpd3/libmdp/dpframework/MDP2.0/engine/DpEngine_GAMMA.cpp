#include "DpTileEngine.h"
#include "DpEngineType.h"
#include "mdp_reg_gamma.h"
#include "mmsys_config.h"
#if CONFIG_FOR_VERIFY_FPGA
#include "ConfigInfo.h"
#endif

#if CONFIG_FOR_OS_ANDROID
#include "PQSessionManager.h"
#endif

//------------------------------------------------------------
// Dummy GAMMAdriver engine
//-------------------------------------------------------------
class DpEngine_GAMMA: public DpTileEngine
{
public:
    DpEngine_GAMMA(uint32_t identifier)
        : DpTileEngine(identifier)
    {
    }

    ~DpEngine_GAMMA()
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
        return eGAMMA;
    }

};

// register factory function
static DpEngineBase* GAMMAFactory(DpEngineType type)
{
    if (tGAMMA == type)
    {
        return new DpEngine_GAMMA(0);
    }
    return NULL;
};

// register factory function
EngineReg GAMMAReg(GAMMAFactory);

DP_STATUS_ENUM DpEngine_GAMMA::onInitEngine(DpCommand &command)
{
    //GAMMA enable
    MM_REG_WRITE(command, MDP_GAMMA_EN, 0x1, 0x1);
    command.addMetLog("MDP_GAMMA_EN", 1);

    //Relay mode
    MM_REG_WRITE(command, MDP_GAMMA_CFG, 0x1, 0x1);
    command.addMetLog("MDP_GAMMA_CFG", 1);

    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_GAMMA::onDeInitEngine(DpCommand &command)
{
    // Disable GAMMA

    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM DpEngine_GAMMA::onConfigFrame(DpCommand &command, DpConfig &config)
{
    PQSession* pPQSession = PQSessionManager::getInstance()->getPQSession(config.pqSessionId);
    if (pPQSession != NULL)
    {
	uint32_t cfg_val = 0;
    	DpPqParam  PqParam;
	uint32_t gamma_table_size;
	int i;

    	pPQSession->getPQParam(&PqParam);

	DPLOGI("GAMMA: gamma_enable=%d gamma_type=%d invert=%d\n",
			PqParam.gamma_enable, PqParam.gamma_enable == 0 ? 0 : PqParam.gamma_type, PqParam.invert);

	if (PqParam.invert == 0 && PqParam.gamma_enable == 0)
	{
		DPLOGI("GAMMA: Disabled\n");
		cfg_val = 1;
	}
	else
	{
		if (PqParam.invert != 0)
		{
			cfg_val |= (0x1 << 2);//invert en
			DPLOGI("GAMMA: Invert enabled. cfg_val=%d\n", cfg_val);
			if (PqParam.gamma_enable == 0)
			{
				PqParam.gamma_enable = 1;
				PqParam.gamma_type = 0;
				for (i = 0; i < 256; i++)
					PqParam.gamma_table[i] = i;
			}
		}

		if (PqParam.gamma_enable != 0)
		{
			cfg_val |= (0x1 << 1);//gamma lut en
			if (PqParam.gamma_type == 0)
			{
				cfg_val |= 0 << 3;//0:8bit lut 1:4bit lut
				cfg_val |= 0 << 4;//lut out lshift 4b
				gamma_table_size = 256;
			}
			else
			{
				cfg_val |= 1 << 3;//0:8bit lut 1:4bit lut
				cfg_val |= 1 << 4;//lut out lshift 4b
				gamma_table_size = 16;
			}

			DPLOGI("GAMMA: Enabled, gamma_table_size:%d\n", gamma_table_size);

			for (i = 0; i < gamma_table_size; i++)
			{
				MM_REG_WRITE_MASK(command, MDP_GAMMA_LUT + 4 * i, PqParam.gamma_table[i], 0xFF);
			}

			int step;
			if (PqParam.gamma_type == 0)
				step = 16;
			else
				step = 1;
			DPLOGI("GAMMA: Table[0,1,2...15]/[0,16,32,...240]: %4d %4d %4d %4d, %4d %4d %4d %4d, %4d %4d %4d %4d, %4d %4d %4d %4d\n",
				PqParam.gamma_table[0 * step], PqParam.gamma_table[1 * step], PqParam.gamma_table[2 * step], PqParam.gamma_table[3 * step],
				PqParam.gamma_table[4 * step], PqParam.gamma_table[5 * step], PqParam.gamma_table[6 * step], PqParam.gamma_table[7 * step],
				PqParam.gamma_table[8 * step], PqParam.gamma_table[9 * step], PqParam.gamma_table[10 * step], PqParam.gamma_table[11 * step],
				PqParam.gamma_table[12 * step], PqParam.gamma_table[13 * step], PqParam.gamma_table[14 * step], PqParam.gamma_table[15 * step]
						);
		}
	}
    	MM_REG_WRITE(command, MDP_GAMMA_CFG, cfg_val, 0x1F);
    }
    return DP_STATUS_RETURN_SUCCESS;
}


DP_STATUS_ENUM DpEngine_GAMMA::onConfigTile(DpCommand &command)
{
    uint32_t GAMMA_hsize;
    uint32_t GAMMA_vsize;

    GAMMA_hsize      = m_inTileXRight   - m_inTileXLeft + 1;
    GAMMA_vsize      = m_inTileYBottom  - m_inTileYTop  + 1;

    MM_REG_WRITE(command, MDP_GAMMA_SIZE, (GAMMA_hsize << 16) +
                                          (GAMMA_vsize <<  0), 0x1FFF1FFF);

    return DP_STATUS_RETURN_SUCCESS;
}

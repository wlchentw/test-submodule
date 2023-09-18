
#include <platform/mt8518.h>
#include <reg.h>
#include <platform/pmic_wrap_init.h>
#include <platform/mtk_timer.h>
#include <platform/pmic.h>

//flag to indicate ca15 related power is ready
volatile int g_ca15_ready __attribute__ ((section (".data"))) = 0;

extern CHARGER_TYPE hw_charger_type_detection(void);

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC access API
//////////////////////////////////////////////////////////////////////////////////////////
u32 pmic_read_interface (u32 RegNum, u32 *val, u32 MASK, u32 SHIFT)
{
    u32 return_value = 0;
    u32 pmic_reg = 0;
    u32 rdata;

    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic_reg=rdata;
    if(return_value!=0)
    {
        printf("[pmic_read_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    printf("[pmic_read_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

    pmic_reg &= (MASK << SHIFT);
    *val = (pmic_reg >> SHIFT);
    printf("[pmic_read_interface] val=0x%x\n", *val);

    return return_value;
}

u32 pmic_config_interface (u32 RegNum, u32 val, u32 MASK, u32 SHIFT)
{
    u32 return_value = 0;
    u32 pmic_reg = 0;
    u32 rdata;

    //1. mt_read_byte(RegNum, &pmic_reg);
    return_value= pwrap_wacs2(0, (RegNum), 0, &rdata);
    pmic_reg=rdata;
    if(return_value!=0)
    {
        printf("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    printf("[pmic_config_interface] Reg[%x]=0x%x\n", RegNum, pmic_reg);

    pmic_reg &= ~(MASK << SHIFT);
    pmic_reg |= (val << SHIFT);

    //2. mt_write_byte(RegNum, pmic_reg);
    return_value= pwrap_wacs2(1, (RegNum), pmic_reg, &rdata);
    if(return_value!=0)
    {
        printf("[pmic_config_interface] Reg[%x]= pmic_wrap read data fail\n", RegNum);
        return return_value;
    }
    printf("[pmic_config_interface] write Reg[%x]=0x%x\n", RegNum, pmic_reg);

    return return_value;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PMIC-Charger Type Detection
//////////////////////////////////////////////////////////////////////////////////////////
CHARGER_TYPE g_ret = CHARGER_UNKNOWN;
int g_charger_in_flag = 0;
int g_first_check=0;

CHARGER_TYPE mt_charger_type_detection(void)
{
    if( g_first_check == 0 )
    {
        g_first_check = 1;
        g_ret = hw_charger_type_detection();
    }
    else
    {
        printf("[mt_charger_type_detection] Got data !!, %d, %d\r\n", g_charger_in_flag, g_first_check);
    }

    printf("[mt_charger_type_detection] chr_type: %d.\r\n", g_ret);
    return g_ret;
}

//==============================================================================
// PMIC Usage APIs
//==============================================================================
u32 get_pmic6397_chip_version (void)
{
    u32 eco_version = 0;

    pmic_read_interface(0x0100, &eco_version, 0xFFFF, 0);

    return eco_version;
}

u32 pmic_IsUsbCableIn (void)
{
    u32 val=0;

    pmic_read_interface(0x0000, &val, 0x1, 5);

    if(val)
        return PMIC_CHRDET_EXIST;
    else
        return PMIC_CHRDET_NOT_EXIST;
}

int pmic_detect_powerkey(void)
{
    u32 val=0;

    pmic_read_interface(0x0144, &val, 0x1, 3);
    if (val==1){
        printf("[pmic_detect_powerkey_PL] Release\n");
        return 0;
    }else{
        printf("[pmic_detect_powerkey_PL] Press\n");
        return 1;
    }
}

int pmic_detect_homekey(void)
{
    u32 val=0;

    pmic_read_interface(0x014A, &val, 0x1, 4);
    return val;
}

//==============================================================================
// PMIC Init Code
//==============================================================================
void PMIC_INIT_SETTING_V1(void)
{
	u32 chip_version = 0;
    /* put init setting from DE/SA */

	pmic_config_interface(0x0, 0x0, 0x1, 0);	/* [0:0]: RG_VCDT_HV_EN; Disable HV. Only compare LV threshold. */

	chip_version = get_pmic6397_chip_version();
	switch (chip_version) {
	/* [7:4]: RG_VCDT_HV_VTH; 7V OVP */
	case PMIC6391_E1_CID_CODE:
	case PMIC6391_E2_CID_CODE:
	case PMIC6391_E3_CID_CODE:
		pmic_config_interface(0x2, 0xC, 0xF, 4);
		break;
	case PMIC6397_E2_CID_CODE:
	case PMIC6397_E3_CID_CODE:
	case PMIC6397_E4_CID_CODE:
		pmic_config_interface(0x2, 0xB, 0xF, 4);
		break;
	default:
		printf("[Power/PMIC] Error chip ID %d\r\n", chip_version);
		pmic_config_interface(0x2, 0xB, 0xF, 4);
		break;
	}
}

static void pmic_default_buck_voltage(void)
{
	u32 reg_val=0;
	u32 buck_val=0;
	pmic_read_interface(0x01F2, &reg_val, 0xFFFF, 0);
	if ((reg_val &0x01) == 0x01) {
		printf("[EFUSE_DOUT_288_303] FUSE 288=0x%x\n", reg_val);

		/* VCORE */
		pmic_read_interface(0x01EE, &reg_val, 0xF, 12);
		pmic_read_interface(0x0278, &buck_val, 0x7F, 0);
		buck_val = (buck_val&0x07)|(reg_val<<3);
		pmic_config_interface(0x0278, buck_val, 0x7F, 0);
		pmic_config_interface(0x027A, buck_val, 0x7F, 0);

		pmic_read_interface(0x01F0, &reg_val, 0xFFFF, 0);
		/* VCA15 */
		buck_val = 0;
		pmic_read_interface(0x0226, &buck_val, 0x7F, 0);
		buck_val = (buck_val&0x07)|((reg_val&0x0F)<<3);
		pmic_config_interface(0x0226, buck_val, 0x7F, 0);
		pmic_config_interface(0x0228, buck_val, 0x7F, 0);

		/* VSAMRCA15 */
		buck_val = 0;
		pmic_read_interface(0x024C, &buck_val, 0x7F, 0);
		buck_val = (buck_val&0x07)|((reg_val&0xF0)>>1);
		pmic_config_interface(0x024C, buck_val, 0x7F, 0);
		pmic_config_interface(0x024E, buck_val, 0x7F, 0);

		/* VCA7 */
		buck_val = 0;
		pmic_read_interface(0x0338, &buck_val, 0x7F, 0);
		buck_val = (buck_val&0x07)|((reg_val&0xF00)>>5);
		pmic_config_interface(0x0338, buck_val, 0x7F, 0);
		pmic_config_interface(0x033A, buck_val, 0x7F, 0);

		/* VSAMRCA7 */
		buck_val = 0;
		pmic_read_interface(0x035E, &buck_val, 0x7F, 0);
		buck_val = (buck_val&0x07)|((reg_val&0xF000)>>9);
		pmic_config_interface(0x035E, buck_val, 0x7F, 0);
		pmic_config_interface(0x0360, buck_val, 0x7F, 0);

		//set the power control by register(use original)
		pmic_config_interface(0x0206, 0x1, 0x1, 12);
	}
}

u32 pmic_init (void)
{
    u32 ret_code = PMIC_TEST_PASS;
    u32 reg_val=0;

    printf("[pmic_init] Start..................\n");

    /* Adjust default BUCK voltage */
    pmic_default_buck_voltage();

    pmic_config_interface(0x0126,  0x1, 0x1, 1);
    pmic_config_interface(0x0126,  0x0, 0x1, 2);
    pmic_config_interface(0x0126,  0x1, 0x1, 4);
    pmic_config_interface(0x0126,  0x0, 0x1, 6);
    pmic_read_interface(0x0126, &reg_val, 0xFFFF, 0);
    printf("[pmic_init] Enable PMIC RST function (depends on main chip RST function) Reg[0x%x]=0x126\n", reg_val);

    //Enable CA15 by default for different PMIC behavior
    pmic_config_interface(0x0222, 0x1, 0x1, 0);
    pmic_config_interface(0x0248, 0x1, 0x1, 0);
	/*Disable CA7/SRCMA7/VDRM by default for no user*/
    pmic_config_interface(0x0334, 0x0, 0x1, 0);
    pmic_config_interface(0x035A, 0x0, 0x1, 0);
    pmic_config_interface(0x0386, 0x0, 0x1, 0);
    gpt_busy_wait_us(200);
    g_ca15_ready = 1;

    pmic_read_interface(0x0222, &reg_val, 0xFFFF, 0);
    printf("Reg[0x%x]=0x0222\n", reg_val);
    pmic_read_interface(0x0248, &reg_val, 0xFFFF, 0);
    printf("Reg[0x%x]=0x0248\n", reg_val);

    //pmic initial setting
    PMIC_INIT_SETTING_V1();
    printf("[PMIC_INIT_SETTING_V1] Done\n");

    //26M clock amplitute adjust
    pmic_config_interface(0x084A, 0x0, 0x3, 2);
    pmic_config_interface(0x084A, 0x1, 0x3, 11);

    pmic_read_interface(0x01F4, &reg_val, 0xFFFF, 0);
    if ((reg_val & 0x8000) == 0)
    {
        pmic_config_interface(0x039E, 0x0041, 0xFFFF, 0);
        pmic_config_interface(0x039E, 0x0040, 0xFFFF, 0);
        pmic_config_interface(0x039E, 0x0050, 0xFFFF, 0);
    }

    //BC11_RST=1
    pmic_config_interface(0x0024, 0x1, 0x1, 1);
    //RG_BC11_BB_CTRL=1
    pmic_config_interface(0x0024, 0x1, 0x1, 0);
    pmic_config_interface(0x001A, 0x0, 0x1, 4);

    printf("[pmic_init] Done...................\n");

    return ret_code;
}


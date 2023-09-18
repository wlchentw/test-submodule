/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Accelerometer Sensor Driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *****************************************************************************/


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/mfd/core.h>
#include <asm/mach-types.h>
//#include "hwtcon_def.h"
#include "fiti_core.h"
//#include "hwtcon_hal.h"
//#include "hwtcon_reg.h"
#include <linux/interrupt.h>
#include <linux/of_irq.h>

#include <linux/mfd/sy7636.h>
#include <linux/mfd/jd9930.h>
#include <linux/input.h>

#include <linux/epd_pmic.h>

#include "hwtcon_fb.h"
#include "hwtcon_core.h"


#define GDEBUG	0
#include <linux/gallen_dbg.h>

#include "../../../../../arch/arm/mach-mediatek/ntx_hwconfig.h"


//#define EPD_OFF_DISABLED	1

// 0 : wait night mode xon off 
// 1 : wait night mode xon on 
// undef : no wait 
#define NM_WAIT_XON		0

#define DEFAULT_FITI_TEMP	25

#define FITI_ERR(string, args...) \
	pr_notice("[FITI ERR]"string" @%s,%u\n", ##args, __func__, __LINE__)
#define FITI_LOG(string, args...) \
	pr_debug("[FITI LOG]"string" @%s,%u\n", ##args, __func__, __LINE__)

#define EINK_Tdx_delay_us	600

extern volatile NTX_HWCONFIG *gptHWCFG;

static struct fiti_context g_fiti;
static wait_queue_head_t g_fiti_power_OK_wait_queue;

static unsigned long gdwJiffiesToUpdateTemperature;

static struct regulator *gptRegulator_display;
static struct regulator *gptRegulator_vcom;
static struct regulator *gptRegulator_tmst;


struct sy7636 *gptSy7636;
struct jd9930 *gptJd9930;

static void pmic_power_good_callback(int iPowerGoodEvt);

static void epd_regulators_init(void) 
{

	if(gptRegulator_display) {
		FITI_LOG("regulators ready !");
		return ;
	}

	if(!gptSy7636) {
		gptSy7636 = sy7636_get();
	}
	if(gptSy7636) {
		sy7636_int_callback_setup(gptSy7636,pmic_power_good_callback);
		if(!gptRegulator_display) {
			gptRegulator_display = sy7636_get_display_regulator();
		}
		if(!gptRegulator_vcom) {
			gptRegulator_vcom = sy7636_get_vcom_regulator();
		}
		if(!gptRegulator_tmst) {
			gptRegulator_tmst = sy7636_get_tmst_regulator();
		}
	}

	if(!gptJd9930) {
		gptJd9930 = jd9930_get();
	}
	if(gptJd9930) {
		jd9930_int_callback_setup(gptJd9930,pmic_power_good_callback);
		if(!gptRegulator_display) {
			gptRegulator_display = jd9930_get_display_regulator();
		}
		if(!gptRegulator_vcom) {
			gptRegulator_vcom = jd9930_get_vcom_regulator();
		}
		if(!gptRegulator_tmst) {
			gptRegulator_tmst = jd9930_get_tmst_regulator();
		}
	}

	gdwJiffiesToUpdateTemperature = jiffies;

	if(gptRegulator_vcom) {
		unsigned short wVCOM_10mV = 0 ;
		short sVCOM_10mV;
		int iVCOM_uV;

		wVCOM_10mV = gptHWCFG->m_val.bVCOM_10mV_HiByte<<8|gptHWCFG->m_val.bVCOM_10mV_LoByte;
		sVCOM_10mV = (short) wVCOM_10mV;
		iVCOM_uV = (int)sVCOM_10mV*10000;

		if(0!=iVCOM_uV) {
			printk("%s:VCOM from HWCONFIG 0x%x=>%d 10mV,%d uV\n",
				__func__,wVCOM_10mV,(int)sVCOM_10mV,iVCOM_uV);
			regulator_set_voltage(gptRegulator_vcom, iVCOM_uV, iVCOM_uV);
		}
		else {
			// Actually ,we don't need to set VCOM if iVCOM_uV=0
		}
	}

}


#define FITI_ENABLE_TIMEOUT_MS (3000)
#define FITI_OFF_TIMEOUT_MS (500)

int fiti_i2c_write(unsigned char reg, unsigned char writedata)
{
	DBG_MSG("%s(%d),reg 0x%02x,writedata=0x%02x\n",__func__,__LINE__,reg,writedata);
	return 0;
}

int fiti_i2c_read(unsigned char reg, unsigned char *rd_buf)
{
	FITI_LOG("reg 0x%02x",reg);
	return -1;
}

void edp_vdd_enable(void)
{
	int iChk = 0;

	if (g_fiti.reg_edp == NULL) {
		FITI_ERR("reg edp is NULL");
		g_fiti.reg_edp = regulator_get(NULL, "ldo_dvcc_epd");
		//g_fiti.reg_edp = sy7636_get_vdd_regulator();
	}


	if (!regulator_is_enabled(g_fiti.reg_edp)) {
		FITI_LOG("enable vdd");
		iChk = regulator_set_voltage(g_fiti.reg_edp, 1800000, 1800000);
		if(iChk<0) {
			FITI_ERR("reg edp set voltage failed !");
		}

		iChk = regulator_enable(g_fiti.reg_edp);
		if(iChk<0) {
			FITI_ERR("reg edp enable failed !");
		}
		udelay(EINK_Tdx_delay_us);
	}
	// MTK hwton calling when resuming .
}
EXPORT_SYMBOL(edp_vdd_enable);

void edp_vdd_disable(void)
{
	int iChk = 0;

	//FITI_LOG("%s(%d)\n",__func__,__LINE__);
	// MTK hwtcon calling when suspending .

	if (g_fiti.reg_edp == NULL) {
		FITI_ERR("reg edp is NULL");
		//g_fiti.reg_edp = sy7636_get_vdd_regulator();
		g_fiti.reg_edp = regulator_get(NULL,"ldo_dvcc_epd");
	}

	if (regulator_is_enabled(g_fiti.reg_edp)) {
		FITI_LOG("disable vdd");
		iChk = regulator_disable(g_fiti.reg_edp);
		if(iChk<0) {
			FITI_ERR("reg edp disable failed !");
		}
	}

}
EXPORT_SYMBOL(edp_vdd_disable);

bool fiti_pmic_judge_power_on_going(void)
{
	FITI_LOG("%d",g_fiti.current_state);
	return (g_fiti.current_state == POWER_ON_GOING)?1:0;
}
bool fiti_pmic_judge_power_off(void)
{
	FITI_LOG("%d",g_fiti.current_state);
	return (g_fiti.current_state == POWER_OFF)?1:0;
}
bool fiti_pmic_judge_power_on(void)
{
	FITI_LOG("%d",g_fiti.current_state);
	return (g_fiti.current_state == POWER_ON)?1:0;
}

int fiti_pmic_get_current_state(void)
{
	FITI_LOG("%d",g_fiti.current_state);
	return g_fiti.current_state;
}
int fiti_pmic_pwrwork_pending(void)
{
	FITI_LOG("%d",g_fiti.current_state);
	return delayed_work_pending(&g_fiti.power_work);
}

int fiti_read_temperature(void)
{
	int iTemp;

	epd_regulators_init();

	if(gptRegulator_tmst) {
		iTemp = regulator_get_voltage(gptRegulator_tmst);
		FITI_LOG("temp=%d",iTemp);
	}
	else {
		ERR_MSG("%s(): handle error !\n",__func__);
		iTemp = DEFAULT_FITI_TEMP;
	}

	return iTemp;	
}

int fiti_read_vcom(void)
{
	int iVCOMmV = -2500;

	epd_regulators_init();


	if(gptRegulator_vcom) {
		iVCOMmV = regulator_get_voltage(gptRegulator_vcom)/1000;
		FITI_LOG("vcom=%d",iVCOMmV);
	}
	else {
		FITI_ERR("handle error return default vcom=%d !",iVCOMmV);
	}

	return iVCOMmV;
}

static void pmic_power_good_callback(int iPowerGoodEvt)
{
	unsigned long flags;
	DBG_MSG("%s(%d)\n",__func__,__LINE__);
	g_fiti.power_good_status = (MSC_RAW_EPD_POWERGOOD_YES==iPowerGoodEvt)?1:0;

	spin_lock_irqsave(&g_fiti.power_state_lock, flags);
	g_fiti.current_state = g_fiti.power_good_status ? POWER_ON : POWER_OFF;
	spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);
}

void fiti_wait_power_good(void)
{
#ifndef FPGA_EARLY_PORTING
	{
		int status = 0;	
		unsigned long flags;
		int iWaitPG_TimeoutMS=FITI_ENABLE_TIMEOUT_MS;

		

		FITI_LOG("timeout=%dms",iWaitPG_TimeoutMS);
		/* wait for pmic enable OK */
		status = wait_event_timeout(
			g_fiti_power_OK_wait_queue,
			g_fiti.power_good_status,
			msecs_to_jiffies(iWaitPG_TimeoutMS));
		/* wait timeout */
		if (status == 0) {
			FITI_ERR("enable fiti:%d timeout currnt:%d target:%d",
				g_fiti.fiti_id,
				g_fiti.current_state,
				g_fiti.target_state);

			g_fiti.power_good_status = !g_fiti.power_good_status;
			FITI_LOG("[fiti] power_good_irq status force :%d", g_fiti.power_good_status);

			spin_lock_irqsave(&g_fiti.power_state_lock, flags);
			g_fiti.current_state = g_fiti.power_good_status ? POWER_ON : POWER_OFF;
			spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);

			WARN_ON(1);
		}
	}
#else //][!FPGA_EARLY_PORTING
	DBG_MSG("%s(%d)\n",__func__,__LINE__);
#endif //] 

}
int fiti_write_vcom(int iVCOMmV)
{
	int iVCOMuV ;

	epd_regulators_init();
	

	if(gptRegulator_vcom) {
		iVCOMuV = regulator_set_voltage(gptRegulator_vcom,iVCOMmV*1000,iVCOMmV*1000);
		FITI_LOG("vcom=%d mV",iVCOMuV/1000);
	}
	else {
		FITI_ERR("handle error return !");
	}
	return 0;
}

void fiti_setting_get_from_waveform(char *waveform_addr)
{
	FITI_LOG("waveform_addr=%s",waveform_addr);
	g_fiti.pmic_setting.VPOS = ((*(waveform_addr+0x62)<<8)+
		*(waveform_addr+0x63))*25/2;
	g_fiti.pmic_setting.VNEG = ((*(waveform_addr+0x64)<<8)+
		*(waveform_addr+0x65))*25/2;

}
void fiti_power_enable(bool enable,int iDelayms)
{
	cancel_delayed_work_sync(&g_fiti.power_work);
	FITI_LOG("en=%d,after %d ms",enable?1:0,iDelayms);
#ifndef FPGA_EARLY_PORTING
	{
		unsigned long flags;

		spin_lock_irqsave(&g_fiti.power_state_lock, flags);
		if(enable) {
			//g_fiti.current_state = POWER_ON_GOING;
			g_fiti.target_state = POWER_ON ;
		}
		else {
			//g_fiti.current_state = POWER_OFF_GOING;
			g_fiti.target_state = POWER_OFF ;
		}
		spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);

		//if(iDelayms>0) {
		//	dump_stack();
		//}
		/* fiti_pmic_config_pmic */
		//queue_work(g_fiti.power_workqueue, &g_fiti.power_work);
		queue_delayed_work_on(0,g_fiti.power_workqueue, &g_fiti.power_work,iDelayms);
	}
#endif
}





static void fiti_pmic_config_pmic(struct work_struct *work_item)
{
	enum FITI_POWER_STATE_ENUM target_state;
	enum FITI_POWER_STATE_ENUM current_state;
	unsigned long flags;
	int iLastNM;

	//int iIsNightMode = hwtcon_core_use_night_mode();
	


	spin_lock_irqsave(&g_fiti.power_state_lock, flags);
	target_state = g_fiti.target_state;
	current_state = g_fiti.current_state;
	FITI_LOG("state cur(%d),target(%d)",current_state,target_state);
	spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);

	if (target_state == POWER_ON) {
		switch (current_state) {
		case POWER_ON:
			/* Do Nothing*/
			FITI_LOG("on->on : do nothing!");
			return;
		case POWER_ON_GOING:
		case POWER_OFF:
			/* power on */

			spin_lock_irqsave(&g_fiti.power_state_lock, flags);
			g_fiti.current_state = POWER_ON_GOING;
			spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);

			epd_regulators_init();
			edp_vdd_enable();

			{
				struct hwtcon_task *task = hwtcon_core_get_current_update_task();


				iLastNM = g_fiti.iIsNightMode;
				if(task) {
					FITI_LOG("updating night mode = %d",task->night_mode);
					g_fiti.iIsNightMode = task->night_mode ? 1 : 0;
				}
				else {
					g_fiti.iIsNightMode = hwtcon_core_use_night_mode();
					FITI_LOG("global night mode = %d",g_fiti.iIsNightMode);
				}

			}

			if(!g_fiti.iIsNightMode) {
				
				if(gptRegulator_display)
					regulator_setflags(gptRegulator_display,0);

				hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_1_TMR,0);
				hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_0_TMR,0);
				hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,0);
			}
			else {
				if(gptRegulator_display)
					regulator_setflags(gptRegulator_display,EPD_PMIC_FLAGS_NIGHTMODE);
#ifdef NM_WAIT_XON //[
				if(!hwtcon_fb_info()->nm_xon_on_with_vcom) {
#if (NM_WAIT_XON==1) //[
					int xon_ctl_mode=XON_CTRLMODE_1_TMR_IS_PENDING;
#else //][!(NM_WAIT_XON==1)
					int xon_ctl_mode=XON_CTRLMODE_0_TMR_IS_PENDING;
#endif //] (NM_WAIT_XON==1)
					
					if( iLastNM && hwtcon_fb_xon_ctrl(xon_ctl_mode,0) )
					{
						unsigned long dwJiffies_timeout=jiffies+msecs_to_jiffies(6000);
;
						FITI_LOG("wait xon %d ...",NM_WAIT_XON);
						while( (NM_WAIT_XON==hwtcon_fb_xon_ctrl(XON_CTRLMODE_GET_CUR_STAT,0)) ||
								hwtcon_fb_xon_ctrl(xon_ctl_mode,0) ) 
						{
							if(time_is_before_jiffies(dwJiffies_timeout)) {
								FITI_ERR("wait xon %d timeout !",NM_WAIT_XON);
								break;
							}
							msleep(10);
						}
					}
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,0);
				}
#else // ][! NM_WAIT_XON
				hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_1_TMR,0);
				hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_0_TMR,0);
#endif //] NM_WAIT_XON
			}


			if(gptRegulator_display) {

				if(regulator_enable(gptRegulator_display)) {
					FITI_ERR("display power on failed !");
					g_fiti.power_good_status = 0;
				}
				else {

					if(time_is_before_jiffies(gdwJiffiesToUpdateTemperature)) {
						hwtcon_fb_set_temperature(fiti_read_temperature());
						FITI_LOG("temperature updated");
						gdwJiffiesToUpdateTemperature = jiffies + msecs_to_jiffies(60000);
					}

					if(gptRegulator_vcom) {
						if(regulator_enable(gptRegulator_vcom)) {
							FITI_ERR("vcom on failed !");
						}
						FITI_LOG("vcom enabled");
						udelay(1700); // Tvd of Panel sepc must be >=100us .
					}
					if(g_fiti.iIsNightMode) {
						
#ifdef NM_WAIT_XON //[
						if(hwtcon_fb_info()->nm_xon_on_with_vcom) {
#if (NM_WAIT_XON==1) //[
							int xon_ctl_mode=XON_CTRLMODE_1_TMR_IS_PENDING;
#else //][!(NM_WAIT_XON==1)
							int xon_ctl_mode=XON_CTRLMODE_0_TMR_IS_PENDING;
#endif //] (NM_WAIT_XON==1)
							
							if(iLastNM && hwtcon_fb_xon_ctrl(xon_ctl_mode,0))
							{
								unsigned long dwJiffies_timeout=jiffies+msecs_to_jiffies(6000);
		;
								FITI_LOG("wait xon %d ...",NM_WAIT_XON);
								while( (NM_WAIT_XON==hwtcon_fb_xon_ctrl(XON_CTRLMODE_GET_CUR_STAT,0)) ||
										hwtcon_fb_xon_ctrl(xon_ctl_mode,0) ) 
								{
									if(time_is_before_jiffies(dwJiffies_timeout)) {
										FITI_ERR("wait xon %d timeout !",NM_WAIT_XON);
										break;
									}
									msleep(10);
								}
							}
							hwtcon_fb_xon_ctrl(XON_CTRLMODE_1,0);
						}
#else // ][! NM_WAIT_XON
						hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_1_TMR,0);
						hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_0_TMR,0);
#endif //] NM_WAIT_XON
					}
					g_fiti.power_good_status = 1;
				}
				
				FITI_LOG("[fiti] power_good_irq status:%d", g_fiti.power_good_status);

				spin_lock_irqsave(&g_fiti.power_state_lock, flags);
				g_fiti.current_state = g_fiti.power_good_status ? POWER_ON : POWER_OFF;
				spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);
				/* fiti_pmic_config_pmic */
				//queue_work(g_fiti.power_workqueue, &g_fiti.power_work);
				queue_delayed_work_on(0,g_fiti.power_workqueue, &g_fiti.power_work,0);
				wake_up(&g_fiti_power_OK_wait_queue);
			}

			break;
		default:
			FITI_ERR("invalid current state:%d", current_state);
			break;
		}
	} else if (target_state == POWER_OFF) {
		switch (current_state) {
		case POWER_OFF_GOING:
		case POWER_ON:
			/* power off */

			g_fiti.current_state = POWER_OFF_GOING;

			epd_regulators_init();

			if(gptRegulator_display) {

#ifndef EPD_OFF_DISABLED//[

				if(regulator_disable(gptRegulator_vcom)) {
					FITI_ERR("display power off failed !");
				}
				else {
					FITI_LOG("vcom disabled");
					g_fiti.power_good_status = 0;
				}
				if(g_fiti.iIsNightMode) {
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_1_TMR,0);
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_0_TMR,0);
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_OFF_DEF_0_TMR,0);
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_OFF_DEF_1_TMR,0);
				}
				else {
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_1_TMR,0);
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_CANCEL_0_TMR,0);
					hwtcon_fb_xon_ctrl(XON_CTRLMODE_OFF_DEF_0_TMR_DAY,0);
				}
			
				if(regulator_disable(gptRegulator_display)) {
					FITI_ERR("display power off failed !");
				}
				else {
				}
#endif //] EPD_OFF_DISABLED
				FITI_LOG("[fiti] power_good_irq status:%d", g_fiti.power_good_status);
				
				spin_lock_irqsave(&g_fiti.power_state_lock, flags);
				g_fiti.current_state = g_fiti.power_good_status ? POWER_ON : POWER_OFF;
				spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);
				/* fiti_pmic_config_pmic */
				//queue_work(g_fiti.power_workqueue, &g_fiti.power_work);
				queue_delayed_work_on(0,g_fiti.power_workqueue, &g_fiti.power_work,0);
				wake_up(&g_fiti_power_OK_wait_queue);
			}


			//edp_vdd_disble();
			
			break;
		case POWER_ON_GOING: /* power good irq will queue this work again */
		case POWER_OFF:
			/* Do nothing */
			FITI_LOG("off/on going->off : do nothing!");
			break;
		default:
			FITI_ERR("invalid current state:%d", current_state);
			break;
		}
	} else {
		FITI_ERR("invalid target state:%d", target_state);
	}
}


static int __init fiti_init(void)
{
	unsigned long flags;
	spin_lock_init(&g_fiti.power_state_lock);
	spin_lock_irqsave(&g_fiti.power_state_lock, flags);
	g_fiti.current_state = POWER_OFF;
	g_fiti.target_state = POWER_OFF;
	spin_unlock_irqrestore(&g_fiti.power_state_lock, flags);

	g_fiti.power_workqueue =
		create_highpri_singlethread_workqueue("power_fiti");
	//INIT_WORK(&g_fiti.power_work, fiti_pmic_config_pmic);
	INIT_DELAYED_WORK(&g_fiti.power_work, fiti_pmic_config_pmic);

	g_fiti.version = 0;

	g_fiti.power_off_status = true;


	init_waitqueue_head(&g_fiti_power_OK_wait_queue);
	printk("PMIC for eInk display\n");
	
	return 0;
}

static void __exit fiti_exit(void)
{
}

/*
 * Module entry points
 */
late_initcall(fiti_init);
module_exit(fiti_exit);
MODULE_DESCRIPTION("epd pmic driver");
MODULE_LICENSE("GPL");


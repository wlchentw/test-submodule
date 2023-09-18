#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_net.h>
#include <linux/slab.h>
#include <asm/setup.h>

#include "ntx_hwconfig.h"

#include "ntx_firmware_parser.h"

#define LM3630FL_NTX_FIRMWARE	1
#ifdef LM3630FL_NTX_FIRMWARE//[
#include "../../../drivers/video/backlight/lm3630a_bl_tables.h"
#endif //]LM3630FL_NTX_FIRMWARE

#include "ntx_firmware_simulation.h"


#define TLC5947FL_NTX_FIRMWARE	1
#define AW99703FL_NTX_FIRMWARE	1

#define _MYINIT_DATA	
#define _MYINIT_TEXT	
volatile static unsigned char _MYINIT_DATA *gpbHWCFG_paddr;
volatile unsigned long _MYINIT_DATA gdwHWCFG_size;
volatile int _MYINIT_DATA giBootPort;
volatile NTX_HWCONFIG *gptHWCFG;
volatile int giSuspendingState ;

EXPORT_SYMBOL(gptHWCFG);

static void * _MemoryRequest(void *addr, u32 len, const char * name)
{
    void * mem = NULL;
   
    do {
        printk(KERN_DEBUG "***%s:%d: request memory region! addr=%p, len=%hd***\n",
                __FUNCTION__, __LINE__, addr, len);
	
	if (!request_mem_region((u32)addr, len, name)) {
            printk(KERN_CRIT "%s():  request memory region failed! addr=%p, len %hd\n",__FUNCTION__, addr, len);
            break;
        }
        mem = (void *) ioremap_nocache((u32)addr, len);
        if (!mem) {
            printk(KERN_CRIT "***%s:%d: could not ioremap %s***\n", __FUNCTION__, __LINE__, name);
            release_mem_region((u32)addr, len);
            break;
        }
    } while (0);
    
    return mem;
}

int gIsCustomerUi;

static int _MYINIT_TEXT hwcfg_p_setup(char *str)
{
	gpbHWCFG_paddr = (unsigned char *)simple_strtoul(str,NULL,0);
	if(NULL==gptHWCFG) {
		gptHWCFG = (NTX_HWCONFIG *)_MemoryRequest((void *)gpbHWCFG_paddr, gdwHWCFG_size, "hwcfg_p");
		if(!gptHWCFG) {
			return 0;
		}
	}
	
	printk("%s() hwcfg_p=%p,vaddr=%p,size=%d,pcb=0x%x,lvl=%x,rev=%x\n",__FUNCTION__,
		gpbHWCFG_paddr,gptHWCFG,(int)gdwHWCFG_size,
		gptHWCFG->m_val.bPCB,gptHWCFG->m_val.bPCB_LVL,gptHWCFG->m_val.bPCB_REV);
	gIsCustomerUi = (int)gptHWCFG->m_val.bUIStyle;
	
	return 1;
}
static int _MYINIT_TEXT hwcfg_size_setup(char *str)
{
	gdwHWCFG_size = (unsigned long)simple_strtoul(str,NULL,0);
	printk("%s() hwcfg_szie=%d\n",__FUNCTION__,(int)gdwHWCFG_size);
	return 1;
}

static int _MYINIT_TEXT boot_port_setup(char *str)
{
#if 0   // Disable(Ray)        
	extern int of_alias_id_exchange(const char *stem,int id1,int id2);

	giBootPort = (int)simple_strtoul(str,NULL,0);
	printk("%s() boot_port=%d\n",__FUNCTION__,giBootPort);

#if 1
	{
		if(2==gptHWCFG->m_val.bIFlash) // Internal flash is eMMC
		{
			if(1==giBootPort) {
				if(0==of_alias_id_exchange("mmc",0,1)) {
					printk("%s():mmc alias exchanged 0 <==> 1 !\n",__FUNCTION__);
				}
			}
		}
		else if(0==gptHWCFG->m_val.bIFlash) { // Internal flash is SD .
			if(0==giBootPort) {
				if(0==of_alias_id_exchange("mmc",0,1)) {
					printk("%s():mmc alias exchanged 0 <==> 1 !\n",__FUNCTION__);
				}
			}
		}
	}
#endif
	

	return 1;
#else
        return 0;
#endif
}


volatile static unsigned char _MYINIT_DATA *gpbNTXFW_paddr;
volatile unsigned long _MYINIT_DATA gdwNTXFW_size;
volatile NTX_FIRMWARE_HDR *gptNTXFW;

#ifdef LM3630FL_NTX_FIRMWARE//[
NTX_FW_LM3630FL_RGBW_current_tab_hdr *gptLm3630fl_RGBW_curr_tab_hdr = 0;
NTX_FW_LM3630FL_dualcolor_hdr *gptLm3630fl_dualcolor_tab_hdr = 0;
NTX_FW_LM3630FL_dualcolor_hdr *gptLm3630fl_dualcolor_x2_tab_hdr = 0;
NTX_FW_LM3630FL_dualcolor_percent_tab *gptLm3630fl_dualcolor_percent_tab = 0;
NTX_FW_LM3630FL_dualcolor_percent_tab *gptLm3630fl_dualcolor_set1_percent_tab = 0;
NTX_FW_LM3630FL_dualcolor_percent_tab *gptLm3630fl_dualcolor_set2_percent_tab = 0;
#endif //]LM3630FL_NTX_FIRMWARE

#ifdef TLC5947FL_NTX_FIRMWARE//[
NTX_FW_TLC5947FL_dualcolor_hdr *gptTlc5947fl_dualcolor_tab_hdr = 0;
NTX_FW_TLC5947FL_dualcolor_percent_tab *gptTlc5947fl_dualcolor_percent_tab = 0;
#endif //]TLC5947FL_NTX_FIRMWARE

NTX_FW_LM36922FL_dualcolor_hdr *gptLm36922fl_dualcolor_tab_hdr = 0;
NTX_FW_LM36922FL_DUALFL_percent_tab *gptLm36922fl_dualcolor_percent_tab = 0;

#ifdef AW99703FL_NTX_FIRMWARE//[
NTX_FW_AW99703FL_dualcolor_hdr *gptAw99703fl_dualcolor_tab_hdr = 0;
NTX_FW_AW99703FL_DUALFL_percent_tab *gptAw99703fl_dualcolor_percent_tab = 0;
#endif


static int ntxfw_item_proc(
		NTX_FIRMWARE_HDR *I_ptFWHdr,
		NTX_FIRMWARE_ITEM_HDR *I_ptFWItemHdr,
		void *I_pvFWItemBin,int I_iItemIdx)
{
	int iRet = 0;
#ifdef CONFIG_BACKLIGHT_LM3630A
	if(!gptHWCFG) {
		printk(KERN_ERR"[%s_%d] gptHWCFG NULL\n",__FUNCTION__,__LINE__);
		return -1;
	}

	if(I_ptFWItemHdr->dw12345678!=0x12345678) {
		printk(KERN_WARNING"ntxfw format error !\n");
		return -2;
	}

	printk("ntxfw[%d],\"%s\",type=0x%x,sz=%d\n",I_iItemIdx,
			I_ptFWItemHdr->szFirmwareName,
			I_ptFWItemHdr->wFirmwareType,
			(int)I_ptFWItemHdr->dwFirmwareSize);

	if( 2==gptHWCFG->m_val.bFL_PWM||4==gptHWCFG->m_val.bFL_PWM||
			5==gptHWCFG->m_val.bFL_PWM||6==gptHWCFG->m_val.bFL_PWM||
			7==gptHWCFG->m_val.bFL_PWM) 
	{

		if(NTX_FW_TYPE_FLPERCETCURTABLE==I_ptFWItemHdr->wFirmwareType) {
			extern void ntx_percent_curr_tab_set(void *pvTable);
			ntx_percent_curr_tab_set(I_pvFWItemBin);
		}
#ifdef LM3630FL_NTX_FIRMWARE//[
		// models with lm3630 .
		else if(NTX_FW_TYPE_LM3630_FLPERCTTAB==I_ptFWItemHdr->wFirmwareType) {
			NTX_FW_LM3630FL_percent_tab *ptLm3630fl_percent_tab = I_pvFWItemBin;
			printk("lm3630fl percent table,color=%d\n",ptLm3630fl_percent_tab->bColor);

			if(1==gptHWCFG->m_val.bFrontLight) {
				// TABLE0 .
				lm3630a_set_FL_W_duty_table(gptHWCFG->m_val.bFrontLight,
					100,ptLm3630fl_percent_tab->bPercentBrightnessA);
				lm3630a_set_default_power_by_table(gptHWCFG->m_val.bFrontLight,
					ptLm3630fl_percent_tab->bDefaultCurrent);
			}
		}
		else if(NTX_FW_TYPE_LM3630_FLCURTABLE==I_ptFWItemHdr->wFirmwareType) {
			NTX_FW_LM3630FL_current_tab *ptLm3630fl_ricohcurr_tab = I_pvFWItemBin;
			printk("lm3630fl ricoh current table,color=%d\n",ptLm3630fl_ricohcurr_tab->bColor);

			if(5==gptHWCFG->m_val.bFL_PWM) {
				// RGBW FL .
				if(NTX_FW_FL_COLOR_WHITE==ptLm3630fl_ricohcurr_tab->bColor) {
					lm3630a_set_FL_RicohCurrTab(1,1,ptLm3630fl_ricohcurr_tab->dwCurrentA,255);
				}
				else 
				if(NTX_FW_FL_COLOR_RED==ptLm3630fl_ricohcurr_tab->bColor) {
					lm3630a_set_FL_RicohCurrTab(1,0,ptLm3630fl_ricohcurr_tab->dwCurrentA,255);
				}
				else 
				if(NTX_FW_FL_COLOR_GREEN==ptLm3630fl_ricohcurr_tab->bColor) {
					lm3630a_set_FL_RicohCurrTab(0,1,ptLm3630fl_ricohcurr_tab->dwCurrentA,255);
				}
				else 
				if(NTX_FW_FL_COLOR_BLUE==ptLm3630fl_ricohcurr_tab->bColor) {
					lm3630a_set_FL_RicohCurrTab(0,0,ptLm3630fl_ricohcurr_tab->dwCurrentA,255);
				}
			}
			
		}
		else if(NTX_FW_TYPE_LM3630_RGBW_CURTAB_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptLm3630fl_RGBW_curr_tab_hdr = I_pvFWItemBin;
			printk("lm3630fl RGBW curr table:%d items\n",(int)gptLm3630fl_RGBW_curr_tab_hdr->dwTotalItems);
		}
		else if(NTX_FW_TYPE_LM3630_RGBW_CURTAB==I_ptFWItemHdr->wFirmwareType) {
			NTX_FW_LM3630FL_RGBW_current_item *L_ptLm3630fl_RGBW_cur_tab = I_pvFWItemBin;
			if(gptLm3630fl_RGBW_curr_tab_hdr) {
				lm3630a_set_FL_RGBW_RicohCurrTab(gptLm3630fl_RGBW_curr_tab_hdr->dwTotalItems,L_ptLm3630fl_RGBW_cur_tab);
			}
			else {
				printk(KERN_ERR"[Warning] LM3630FL RGBW curr table header not exist !!\n");
			}
		}
		else if(NTX_FW_TYPE_LM3630_DUALFL_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptLm3630fl_dualcolor_tab_hdr = I_pvFWItemBin;
			printk("lm3630fl dual color table : %d temperatures ,c1_pwr=0x%x,c2_pwr=0x%x\n",
				(int)gptLm3630fl_dualcolor_tab_hdr->dwTotalColors,
				gptLm3630fl_dualcolor_tab_hdr->bDefaultC1_Current,
				gptLm3630fl_dualcolor_tab_hdr->bDefaultC2_Current);
		}
		else if(NTX_FW_TYPE_LM3630_DUALFL_PERCENTTAB==I_ptFWItemHdr->wFirmwareType) {
			if(gptLm3630fl_dualcolor_tab_hdr) {
				gptLm3630fl_dualcolor_percent_tab = I_pvFWItemBin;
			}
			else {
				printk(KERN_ERR"[Warning] LM3630FL dualcolor table header not exist !!\n");
			}
		}
		else if( NTX_FW_TYPE_LM3630_DUALFL_X2_PERCENTTAB == I_ptFWItemHdr->wFirmwareType ){
			if(gptLm3630fl_dualcolor_x2_tab_hdr){
				
				if(gptLm3630fl_dualcolor_set1_percent_tab==NULL){
					gptLm3630fl_dualcolor_set1_percent_tab = I_pvFWItemBin;
				}
				else if(gptLm3630fl_dualcolor_set2_percent_tab==NULL){
					gptLm3630fl_dualcolor_set2_percent_tab = I_pvFWItemBin;
				}
			}
		}
		else if(NTX_FW_TYPE_LM3630_MIX2COLOR11_CURTAB==I_ptFWItemHdr->wFirmwareType) {
			NTX_FW_LM3630FL_MIX2COLOR11_current_tab *L_ptLm3630fl_Mix2Color11_curr_tab = I_pvFWItemBin;
			lm3630a_set_FL_Mix2color11_RicohCurrTab(L_ptLm3630fl_Mix2Color11_curr_tab);
		}
		else if(NTX_FW_TYPE_LM3630_DUALFL_X2_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptLm3630fl_dualcolor_x2_tab_hdr = I_pvFWItemBin ;
		}
#endif //]LM3630FL_NTX_FIRMWARE
		else {
		}
	}
#ifdef TLC5947FL_NTX_FIRMWARE
	else if( 8==gptHWCFG->m_val.bFL_PWM) 
	{
		if(NTX_FW_TYPE_TLC5947_DUALFL_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptTlc5947fl_dualcolor_tab_hdr = I_pvFWItemBin;
			printk(KERN_INFO"TLC5947 TotalColors:%d , items:%d \n",gptTlc5947fl_dualcolor_tab_hdr->dwTotalColors);
		}
		else if(NTX_FW_TYPE_TLC5947_DUALFL_PERCENTTAB==I_ptFWItemHdr->wFirmwareType) {
			if(gptTlc5947fl_dualcolor_tab_hdr) {
				gptTlc5947fl_dualcolor_percent_tab = I_pvFWItemBin;
			}
			else {
				printk(KERN_INFO"[Warning] TLC5947 dualcolor table header not exist !!\n");
			}
		}
	}
#endif // TLC5947FL_NTX_FIRMWARE
	else if( 0x0a==gptHWCFG->m_val.bFL_PWM) 
	{
		if(NTX_FW_TYPE_LM36922_DUALFL_X2_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptLm36922fl_dualcolor_tab_hdr = I_pvFWItemBin;
			printk(KERN_ERR"LM36922 TotalColors:%d , items:%d \n",gptLm36922fl_dualcolor_tab_hdr->dwTotalColors);
		}
		else if(NTX_FW_TYPE_LM36922_DUALFL_X2_PERCENTTAB==I_ptFWItemHdr->wFirmwareType) {
			if(gptLm36922fl_dualcolor_tab_hdr) {
				gptLm36922fl_dualcolor_percent_tab = I_pvFWItemBin;
			}
			else {
				printk(KERN_ERR"[Warning] LM36922 dualcolor table header not exist !!\n");
			}
		}
	}
#endif

#ifdef AW99703FL_NTX_FIRMWARE
	if (0x0c == gptHWCFG->m_val.bFL_PWM)
	{
		if(NTX_FW_TYPE_AW99703_DUALFL_X2_HDR==I_ptFWItemHdr->wFirmwareType) {
			gptAw99703fl_dualcolor_tab_hdr = I_pvFWItemBin;
			printk(KERN_ERR"AW99703 TotalColors:%d , items:%d \n",gptAw99703fl_dualcolor_tab_hdr->dwTotalColors);
		}
		else if(NTX_FW_TYPE_AW99703_DUALFL_X2_PERCENTTAB==I_ptFWItemHdr->wFirmwareType) {
			if(gptAw99703fl_dualcolor_tab_hdr) {
				gptAw99703fl_dualcolor_percent_tab = I_pvFWItemBin;
			}
			else {
				printk(KERN_ERR"[Warning] AW99703 dualcolor table header not exist !!\n");
			}
		}
	}
#endif // AW99703FL_NTX_FIRMWARE
	
	return iRet;
}

static int _ntxfw_parse(void)
{
	if(ntx_firmware_parse_fw_buf(gptNTXFW,gdwNTXFW_size,ntxfw_item_proc)<0) {
		printk(KERN_ERR"[NTX][%s_%d] ntxfw parse failed !!\n",__FUNCTION__,__LINE__);
		gpbNTXFW_paddr = 0;
		gptNTXFW = 0;
		gdwNTXFW_size = 0;
		return 0;
	}
	else {
		printk("%s() ntxfw_p=%p,vaddr=%p,size=%d,name=\"%s\",ver=%d.%d.%d.%d,items=%d\n",__FUNCTION__,
			gpbNTXFW_paddr,gptNTXFW,gdwNTXFW_size,gptNTXFW->szFirmwareName,
			gptNTXFW->bVersionMajor,gptNTXFW->bVersionSub,
			gptNTXFW->bVersionMinor,gptNTXFW->bVersionMini,
			(int)gptNTXFW->wFirmwareItems);
		return 1;
	}
}

static int _MYINIT_TEXT ntxfw_p_setup(char *str)
{
	gpbNTXFW_paddr = (unsigned char *)simple_strtoul(str,NULL,0);
	gptNTXFW = (NTX_FIRMWARE_HDR *)_MemoryRequest((void *)gpbNTXFW_paddr, gdwNTXFW_size, "ntxfw_p");
	return _ntxfw_parse();
}

static int _MYINIT_TEXT ntxfw_size_setup(char *str)
{
	gdwNTXFW_size = (unsigned long)simple_strtoul(str,NULL,0);
	printk("%s() ntxfw_size=%d\n",__FUNCTION__,(int)gdwNTXFW_size);
	return 1;
}

char ntx_cmd_line[COMMAND_LINE_SIZE];
void ntx_parse_cmdline(void)
{
	static int iParseCnt = 0;
	char *pcPatternStart,*pcPatternVal,*pcPatternValEnd,cTempStore;
	unsigned long ulPatternLen;

	char *szParsePatternA[]={"hwcfg_sz=","hwcfg_p=","boot_port=","ntxfw_sz=","ntxfw_p="};
	int ((*pfnDispatchA[])(char *str))={hwcfg_size_setup,hwcfg_p_setup,boot_port_setup,ntxfw_size_setup,ntxfw_p_setup};
	int i;
	char *pszCmdLineBuf;

	if(iParseCnt++>0) {
		printk("%s : cmdline parse already done .\n",__FUNCTION__);
		return ;
	}
	//printk("%s():cmdline(%d)=%s\n",__FUNCTION__,strlen(saved_command_line),saved_command_line);

	pszCmdLineBuf = ntx_cmd_line;//kmalloc(strlen(saved_command_line)+1,GFP_KERNEL);
	//ASSERT(pszCmdLineBuf);
	strcpy(pszCmdLineBuf,saved_command_line);
	printk("%s():cp cmdline=%s\n",__FUNCTION__,pszCmdLineBuf);

	for(i=0;i<sizeof(szParsePatternA)/sizeof(szParsePatternA[0]);i++) {
		ulPatternLen = strlen(szParsePatternA[i]);
		pcPatternStart = strstr(pszCmdLineBuf,szParsePatternA[i]);
		if(pcPatternStart) {
			pcPatternVal=pcPatternStart + ulPatternLen ;
			pcPatternValEnd = strchr(pcPatternVal,' ');
			cTempStore='\0';
			if(pcPatternValEnd) {
				cTempStore = *pcPatternValEnd;
				*pcPatternValEnd = '\0';
			}
			//printk("%s():pattern \"%s\" ,val=\"%s\"\n",__FUNCTION__,szParsePatternA[i],pcPatternVal);
			pfnDispatchA[i](pcPatternVal);
			if(pcPatternValEnd) {
				*pcPatternValEnd = cTempStore;
			}
		}
		else 
			printk ("[%s-%d] %s not found !!!\n",__func__,__LINE__,szParsePatternA[i]);
	}
}

void ntx_init(void)
{
	ntx_parse_cmdline();

#if 0
#ifdef HWCONFIG_SIMULATION //[
	if(NULL == gptHWCFG) { 
		printk("%s(%d) enabled hwconfig simulation .\n",__func__,__LINE__);
		gptHWCFG = (NTX_HWCONFIG *)gcHWCFG_EA0T04A;
		gdwHWCFG_size = sizeof(gcHWCFG_EA0T04A);
	}
#endif //] HWCONFIG_SIMULATION

#ifdef NTXFW_SIMULATION //[
	if(NULL ==gptNTXFW) {
		printk("%s(%d) enabled ntxfw simulation .\n",__func__,__LINE__);
		gptNTXFW = (NTX_FIRMWARE_HDR *)gcNTXFW_EA0T04A;
		gdwNTXFW_size = sizeof(gcNTXFW_EA0T04A);
		_ntxfw_parse();
	}
#endif //] NTXFW_SIMULATION
#endif
}



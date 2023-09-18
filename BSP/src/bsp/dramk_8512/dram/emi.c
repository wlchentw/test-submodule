// SPDX-License-Identifier: MediaTekProprietary

//=============================================================================
//  Include Files
//=============================================================================
#include <stdlib.h>
//#include <common.h>
//#include <ett_common.h>
//#include <test_case_controller.h>
//#include <api.h>
//#include "gpio.h"
//#include "ett_cust.h"
//#include "emi_setting.h"
//#include "pll.h"
//#include "dramc_pi_api.h"
#include <platform/dramc_common.h>
#include <platform/dramc_api.h>
#include <platform/dramc_register.h>
#include <platform/mtk_wdt.h>
#if WITH_PMIC_MT6398
#include <platform/pmic_6398.h>
#endif
#include <lib/bio.h>

#define OLD_CODE    0

unsigned int mt_get_dram_type_from_hw_trap(void);

#if !__ETT__
#if (FOR_DV_SIMULATION_USED==0)
//#include "dram_buffer.h"
#endif
#else
#include <barriers.h>
#endif

#if !__FLASH_TOOL_DA__ && !__ETT__
#if (FOR_DV_SIMULATION_USED==0)
   #include "platform.h"
//   #include "emi_hw.h"
#if defined(DEF_LAST_DRAMC)
//   #include "plat_dbg_info.h"
#endif
#endif
#endif

#include <platform/emi.h>

#if !__FLASH_TOOL_DA__ && !__ETT__
#include <platform/custom_emi.h>   // fix build error: emi_settings
#endif

#if CFG_BOOT_ARGUMENT
#define bootarg g_dram_buf->bootarg
#endif

#if (FOR_DV_SIMULATION_USED==0)
//#include <pmic.h>
#include <platform/rt5748.h>
/* now we can use definition MTK_PMIC_CHIP_MT6357 */
//#define MTK_PMIC_CHIP_MT6357
#endif

#if !__ETT__
void mt_reg_sync_write(unsigned int addr, unsigned int value)
{
  *((UINT32P)(addr))= value;
  dsb();
}
//#define mt_reg_sync_write(x,y) mt_reg_sync_writel(y,x)
#endif

#ifdef MTK_PMIC_CHIP_MT6357
//#include <regulator/mtk_regulator.h>
#endif

#if !__ETT__
#define CQ_DMA_BASE (IO_PHYS+0x0212000)
#endif

//=============================================================================
//  Definition
//=============================================================================

static unsigned int get_dramc_addr(dram_addr_t *dram_addr, unsigned int offset);

//=============================================================================
//  Global Variables
//=============================================================================
int emi_setting_index = -1;
int num_of_emi_records = 1;
EMI_SETTINGS emi_settings[] =
{

};

static DRAM_INFO_BY_MRR_T DramInfo;
static int enable_combo_dis = 0;
static unsigned int rank_swap;
extern DRAMC_CTX_T *psCurrDramCtx;
extern EMI_SETTINGS default_emi_setting;
#ifdef MTK_PMIC_CHIP_MT6357
static struct mtk_regulator reg_vdram, reg_vcore, reg_vio18;
#endif

#ifdef LAST_DRAMC
static LAST_DRAMC_INFO_T* last_dramc_info_ptr;
#endif

#ifdef LAST_EMI
static LAST_EMI_INFO_T* last_emi_info_ptr;
#endif

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
extern u64 get_part_addr(const char *name);
static int read_offline_dram_mdl_data(DRAM_INFO_BY_MRR_T *DramInfo);
static int write_offline_dram_mdl_data(DRAM_INFO_BY_MRR_T *DramInfo);
static u64 part_dram_data_addr = 0;
static unsigned int dram_offline_data_flags = 0;
#endif
//=============================================================================
//  External references
//=============================================================================
extern DRAMC_CTX_T *psCurrDramCtx;
extern char* opt_dle_value;

void print_DBG_info(DRAMC_CTX_T *p);
void Dump_EMIRegisters(DRAMC_CTX_T *p);


#define EMI_CHANNEL_APB_BASE 0x1022D000
#define INFRA_DRAMC_REG_CONFIG  0x100010b4

#if OLD_CODE
#define EMI_APB_BASE    0x10219000
#if !__ETT__
#define IO_PHYS             (0x10000000)
#define CHN0_EMI_BASE       (IO_PHYS + 0x0022D000)
#define CHN1_EMI_BASE       (IO_PHYS + 0x00235000)

#define EMI_CONA               (EMI_APB_BASE + 0x000)
#define EMI_CONF               (EMI_APB_BASE + 0x028)
#define EMI_CONH               (EMI_APB_BASE + 0x038)
#endif

#define CHN_EMI_CONA(base)	(base + 0x000)
#define CHN_EMI_CONC(base)	(base + 0x010)
#endif

void EMI_ESL_Setting1(void)
{
	dsb();
	DramcBroadcastOnOff(DRAMC_BROADCAST_OFF);
	dsb();

	*((volatile unsigned *)(EMI_CONA)) = 0xf050f054;
	*((volatile unsigned *)(EMI_CONB)) = 0x17283544;
	*((volatile unsigned *)(EMI_CONC)) = 0x0a1a0b1a;
	*((volatile unsigned *)(EMI_COND)) = 0x3657587a;
	*((volatile unsigned *)(EMI_CONE)) = 0x0000c042;
	*((volatile unsigned *)(EMI_CONG)) = 0x2b2b2a38;
	*((volatile unsigned *)(EMI_CONH)) = 0x44440003;
	*((volatile unsigned *)(EMI_CONM)) = 0x007811ff;
	*((volatile unsigned *)(EMI_CONN)) = 0x00000000;
	*((volatile unsigned *)(EMI_MDCT)) = 0x11120c1f;
	*((volatile unsigned *)(EMI_SHF0)) = 0x11120c1f;
	*((volatile unsigned *)(EMI_MDCT_2ND)) = 0x00001123;
	*((volatile unsigned *)(EMI_SHF1)) = 0x00001123;
	*((volatile unsigned *)(EMI_IOCL)) = 0xa8a8a8a8;
	*((volatile unsigned *)(EMI_IOCL_2ND)) = 0x25252525;
	*((volatile unsigned *)(EMI_IOCM)) = 0xa8a8a8a8;
	*((volatile unsigned *)(EMI_IOCM_2ND)) = 0x25252525;
	*((volatile unsigned *)(EMI_TESTB)) = 0x00060037;
	*((volatile unsigned *)(EMI_TESTC)) = 0x38460014;
	*((volatile unsigned *)(EMI_TESTD)) = 0xa0000000;
	*((volatile unsigned *)(EMI_ARBI)) = 0x00007108;
	*((volatile unsigned *)(EMI_ARBI_2ND)) = 0x00007108;
	*((volatile unsigned *)(EMI_ARBK)) = 0x00000000;
	*((volatile unsigned *)(EMI_SLCT)) = 0xff01ff00;
	*((volatile unsigned *)(EMI_BMEN)) = 0x00ff0000;
	*((volatile unsigned *)(EMI_CLUA)) = 0x00000000;
	*((volatile unsigned *)(EMI_ARBA)) = 0x08047050;
	*((volatile unsigned *)(EMI_ARBB)) = 0x10107050;
	*((volatile unsigned *)(EMI_ARBC)) = 0x0a0a50df;
	*((volatile unsigned *)(EMI_ARBD)) = 0x0000f0d0;
	*((volatile unsigned *)(EMI_ARBE)) = 0x08086050;
	*((volatile unsigned *)(EMI_ARBE_2ND)) = 0x00000034;
	*((volatile unsigned *)(EMI_ARBF)) = 0x0a0a50df;
	*((volatile unsigned *)(EMI_ARBG)) = 0x20207048;
	*((volatile unsigned *)(EMI_ARBH)) = 0x20207048;


	*((volatile unsigned *)CHN_EMI_CONA(CHN0_EMI_BASE)) =
		0x0444f050;
	*((volatile unsigned *)CHN_EMI_CONB(CHN0_EMI_BASE)) =
		0xffff5048;
	*((volatile unsigned *)CHN_EMI_CONC(CHN0_EMI_BASE)) =
		0x00000005;
	*((volatile unsigned *)CHN_EMI_COND(CHN0_EMI_BASE)) =
		0x00000000;
	*((volatile unsigned *)CHN_EMI_MDCT(CHN0_EMI_BASE)) =
		0x11f08c03;
	*((volatile unsigned *)CHN_EMI_SHF0(CHN0_EMI_BASE)) =
		0x12508C17;
	*((volatile unsigned *)CHN_EMI_TESTB(CHN0_EMI_BASE)) =
		0x00038037;
	*((volatile unsigned *)CHN_EMI_TESTC(CHN0_EMI_BASE)) =
		0x38460002;
	*((volatile unsigned *)CHN_EMI_TESTD(CHN0_EMI_BASE)) =
		0x00000000;
	*((volatile unsigned *)CHN_EMI_MD_PRE_MASK(CHN0_EMI_BASE)) =
		0xaa0148ff;
	*((volatile unsigned *)CHN_EMI_MD_PRE_MASK_SHF(CHN0_EMI_BASE)) =
		0xaa516cff;
	*((volatile unsigned *)CHN_EMI_AP_ERALY_CKE(CHN0_EMI_BASE)) =
		0x000002ff;
	*((volatile unsigned *)CHN_EMI_DQFR(CHN0_EMI_BASE)) =
		0x00003101;
	*((volatile unsigned *)CHN_EMI_ARBI(CHN0_EMI_BASE)) =
		0x20407188;
	*((volatile unsigned *)CHN_EMI_ARBI_2ND(CHN0_EMI_BASE)) =
		0x20407188;
	*((volatile unsigned *)CHN_EMI_ARBJ(CHN0_EMI_BASE)) =
		0x0719595e;
	*((volatile unsigned *)CHN_EMI_ARBJ_2ND(CHN0_EMI_BASE)) =
		0x0719595e;
	*((volatile unsigned *)CHN_EMI_ARBK(CHN0_EMI_BASE)) =
		0x64f3fc79;
	*((volatile unsigned *)CHN_EMI_ARBK_2ND(CHN0_EMI_BASE)) =
		0x64f3fc79;
	*((volatile unsigned *)CHN_EMI_SLCT(CHN0_EMI_BASE)) =
		0x00080868;
	*((volatile unsigned *)CHN_EMI_ARB_REF(CHN0_EMI_BASE)) =
		0x82410222;
	*((volatile unsigned *)CHN_EMI_DRS_MON0(CHN0_EMI_BASE)) =
		0x0000f801;
	*((volatile unsigned *)CHN_EMI_DRS_MON1(CHN0_EMI_BASE)) =
		0x40000000;
	*((volatile unsigned *)CHN_EMI_RKARB0(CHN0_EMI_BASE)) =
		0x00060020;
	*((volatile unsigned *)CHN_EMI_RKARB1(CHN0_EMI_BASE)) =
		0x01010101;
	*((volatile unsigned *)CHN_EMI_RKARB2(CHN0_EMI_BASE)) =
		0x20201840;
	*((volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)) =
		0xfffffff9;
	dsb();



#if OLD_CODE
	//from emi_config_lpddr3_1ch.c v11_30
mt_reg_sync_write(INFRA_DRAMC_REG_CONFIG, 0x0000001f); // (ch0,ch1 same setting ;  ch2,ch3 same setting)

//C:-------------------BEGIN))= EMI Setting--------------------
//))= Row = 15-bit
#ifndef ONE_CH
  #ifdef RANK_512MB  // = > 2channel = dual rank = total= 2G
    mt_reg_sync_write(EMI_APB_BASE+0x00000000, 0xa053a154); //10:Row= 15bits= 11:Row= 16bits
  #else  //RANK_1G  = > 2channel = dual rank = total= 4G
    mt_reg_sync_write(EMI_APB_BASE+0x00000000, 0xf053f154); //2CH
  #endif
#else
  #ifdef RANK_512MB
    mt_reg_sync_write(EMI_APB_BASE+0x00000000, 0xa053a054); //1CH
  #else
    mt_reg_sync_write(EMI_APB_BASE+0x00000000, 0xf053f054); //1CH
  #endif
#endif
//))= Row = 14-bit;
mt_reg_sync_write(EMI_APB_BASE+0x00000008, 0x17283544);
mt_reg_sync_write(EMI_APB_BASE+0x00000010, 0x0a1a0b1a);
mt_reg_sync_write(EMI_APB_BASE+0x00000018, 0x3657587a);
mt_reg_sync_write(EMI_APB_BASE+0x00000020, 0x80400148);
mt_reg_sync_write(EMI_APB_BASE+0x00000028, 0x00000000);
mt_reg_sync_write(EMI_APB_BASE+0x00000030, 0x2b2b2a38);
mt_reg_sync_write(EMI_APB_BASE+0x00000038, 0x00000000);
#ifdef BANK_INTERLEAVE_ON
mt_reg_sync_write(EMI_APB_BASE+0x00000038, 0x00000004); // enable bank interleaving
#endif
mt_reg_sync_write(EMI_APB_BASE+0x00000040, 0x00008803);

mt_reg_sync_write(EMI_APB_BASE+0x00000060, 0x000001ff); // reserved buffer to normal read/write no limit
mt_reg_sync_write(EMI_APB_BASE+0x00000068, 0x00000000);
mt_reg_sync_write(EMI_APB_BASE+0x00000078, 0x11338c17); //reserve buffer to ap_w[30:28]= ap_r[26:24]= u_w[22:20]= u_r[28:16]
mt_reg_sync_write(EMI_APB_BASE+0x0000007c, 0x00001112); //reserve buffer to hi_w[15:12]= hi_r[10:8]= md_w[6:4]= md_r[2:0]
mt_reg_sync_write(EMI_APB_BASE+0x000000d0, 0xa8a8a8a8);
mt_reg_sync_write(EMI_APB_BASE+0x000000d4, 0x25252525);
mt_reg_sync_write(EMI_APB_BASE+0x000000d8, 0xa8a8a8a8);
mt_reg_sync_write(EMI_APB_BASE+0x000000dc, 0x25252525);
mt_reg_sync_write(EMI_APB_BASE+0x000000e8, 0x00060037); // initial starvation counter div2= [4]= 1
mt_reg_sync_write(EMI_APB_BASE+0x000000f0, 0x38460000);
mt_reg_sync_write(EMI_APB_BASE+0x000000f8, 0x00000000);
/////  EMI  bandwidth limit  ////////// default UI
////////////////////////////////////////////////////
#ifdef SCN_VPWFD
mt_reg_sync_write(EMI_APB_BASE+0x00000100, 0x4020524f);
mt_reg_sync_write(EMI_APB_BASE+0x00000108, 0x4020504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000110, 0xa0a050d8);
mt_reg_sync_write(EMI_APB_BASE+0x00000118, 0x000070cc);
mt_reg_sync_write(EMI_APB_BASE+0x00000120, 0x40406046);
mt_reg_sync_write(EMI_APB_BASE+0x00000128, 0xa0a070d7);
mt_reg_sync_write(EMI_APB_BASE+0x00000130, 0xa0a0504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000138, 0xa0a0504f);
#elif  SCN_VR4K
mt_reg_sync_write(EMI_APB_BASE+0x00000100, 0x4020524f);
mt_reg_sync_write(EMI_APB_BASE+0x00000108, 0x4020504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000110, 0xa0a050da);
mt_reg_sync_write(EMI_APB_BASE+0x00000118, 0x000070cc);
mt_reg_sync_write(EMI_APB_BASE+0x00000120, 0x40406046);
mt_reg_sync_write(EMI_APB_BASE+0x00000128, 0xa0a070d3);
mt_reg_sync_write(EMI_APB_BASE+0x00000130, 0xa0a0504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000138, 0xa0a0504f);
#elif  SCN_ICFP
mt_reg_sync_write(EMI_APB_BASE+0x00000100, 0x4020524f);
mt_reg_sync_write(EMI_APB_BASE+0x00000108, 0x4020504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000110, 0xa0a050cb);
mt_reg_sync_write(EMI_APB_BASE+0x00000118, 0x000070cc);
mt_reg_sync_write(EMI_APB_BASE+0x00000120, 0x40406046);
mt_reg_sync_write(EMI_APB_BASE+0x00000128, 0xa0a070d6);
mt_reg_sync_write(EMI_APB_BASE+0x00000130, 0xa0a0504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000138, 0xa0a0504f);
#else//SCN_UI
mt_reg_sync_write(EMI_APB_BASE+0x00000100, 0x4020524f);
mt_reg_sync_write(EMI_APB_BASE+0x00000108, 0x4020504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000110, 0xa0a050c6);
mt_reg_sync_write(EMI_APB_BASE+0x00000118, 0x000070cc);
mt_reg_sync_write(EMI_APB_BASE+0x00000120, 0x40406046);
mt_reg_sync_write(EMI_APB_BASE+0x00000128, 0xa0a070d5);
mt_reg_sync_write(EMI_APB_BASE+0x00000130, 0xa0a0504f);
mt_reg_sync_write(EMI_APB_BASE+0x00000138, 0xa0a0504f);
#endif

mt_reg_sync_write(EMI_APB_BASE+0x00000140, 0x00007108);
mt_reg_sync_write(EMI_APB_BASE+0x00000144, 0x00007108);
mt_reg_sync_write(EMI_APB_BASE+0x00000158, 0x0000ff00);

mt_reg_sync_write(EMI_APB_BASE+0x00000820, 0x2a2a0101);
mt_reg_sync_write(EMI_APB_BASE+0x00000824, 0x01012a2a);
mt_reg_sync_write(EMI_APB_BASE+0x00000828, 0x58580101);
mt_reg_sync_write(EMI_APB_BASE+0x0000082c, 0x01015858);
mt_reg_sync_write(EMI_APB_BASE+0x00000830, 0x0fc39a60);
mt_reg_sync_write(EMI_APB_BASE+0x00000834, 0x05050003);
mt_reg_sync_write(EMI_APB_BASE+0x00000838, 0x254d3fff);
mt_reg_sync_write(EMI_APB_BASE+0x0000083c, 0xa0a0a0a0);

//C:-------------------END))= EMI Setting--------------------;
//C:-------------------BEGIN))= CHANNEL EMI Setting--------------------;
#ifdef RANK_512MB  // = > 2channel = dual rank = total= 2G
  mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000000, 0x0400a051);
#else  //RANK_1G  = > 2channel = dual rank = total= 4G
  mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000000, 0x0400f051);
#endif
#ifdef BANK_INTERLEAVE_ON
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000000, 0x2400f051); // enable bank interleaving
#endif
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000008, 0x00ff2048); // over BW limit= starvation slow down= [13:12]= 2
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000010, 0x00000000);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000018, 0x88008817);

#ifdef SCN_VPWFD
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000048, 0x00030027); //RD_INORDER_THR[20:16]= 3
#elif  SCN_VR4K
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000048, 0x00030027); //RD_INORDER_THR[20:16]= 3
#elif  SCN_ICFP
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000048, 0x00030027); //RD_INORDER_THR[20:16]= 3
#else//SCN_UI
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000048, 0x00030027); //RD_INORDER_THR[20:16]= 3
#endif

mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000050, 0x38460002); // [1] : MD_RD_AFT_WR_EN
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000058, 0x00000000);
#ifdef SCN_VPWFD
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000080, 0x00000f00);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000088, 0x00000b00);
#elif  SCN_VR4K
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000080, 0x00000b00);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000088, 0x00000b00);
#elif  SCN_ICFP
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000080, 0x00000b00);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000088, 0x00000b00);
#else//SCN_UI
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000080, 0x00000f00);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000088, 0x00000b00);
#endif

//#endif
#ifdef SCN_ICFP
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000140, 0x20406188);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000144, 0x20406188);
#elif SCN_VPWFD
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000140, 0x20406188);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000144, 0x20406188);
#elif SCN_VR4K
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000140, 0x20406188);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000144, 0x20406188);
#else// SCN_UI
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000140, 0x20406188);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000144, 0x20406188);
#endif

mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000148, 0x3719595e);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x0000014c, 0x3719595e);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000150, 0x64f3fc79);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000154, 0x64f3fc79);

#ifdef SCN_VPWFD
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000158, 0x00080888);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x0000015c, 0x82410222); //STRICT_MSK_ULTRA_EN [5]= 1
#elif  SCN_VR4K
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000158, 0x00080888);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x0000015c, 0x82410222); //STRICT_MSK_ULTRA_EN [5]= 1
#elif  SCN_ICFP
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000158, 0x00080868);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x0000015c, 0x88410222); //STRICT_MSK_ULTRA_EN [5]= 1
#else//SCN_UI
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000158, 0x00080888);
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x0000015c, 0x82410222); //STRICT_MSK_ULTRA_EN [5]= 1
#endif

mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x000001b0, 0x0006002f); // Rank-Aware arbitration
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x000001b4, 0x01010101); // Rank-Aware arbitration
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x000001b8, 0x10100820); // Rank-Aware arbitration
mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x000003fc, 0x00000000); // Write M17_toggle_mask =0!!!!

mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000710, 0x8a228c17); // [24:20] = 0x2 : bank throttling (default= 0x01f00000)

//C:-------------------END))= CHANNEL EMI Setting--------------------;
mt_reg_sync_write(INFRA_DRAMC_REG_CONFIG, 0x00000000); // disable broadcast
#endif
}

void EMI_ESL_Setting2(void)
{
#if OLD_CODE
mt_reg_sync_write(INFRA_DRAMC_REG_CONFIG, 0x0000001f); // (ch0,ch1 same setting ;  ch2,ch3 same setting)

mt_reg_sync_write(EMI_CHANNEL_APB_BASE+0x00000010, 0x00000001); // [0] EMI enable

mt_reg_sync_write(EMI_APB_BASE+0x00000060, 0x000005ff); // [26] Disable read data DCM, [10] EMI enable

mt_reg_sync_write(INFRA_DRAMC_REG_CONFIG, 0x00000000); // disable broadcast
#endif
}


void EMI_Patch(void)
{
	EMI_SETTINGS *emi_set;
	unsigned temp;

	emi_set = get_emi_setting();

	// unit is 406.35MB/s
#if LP4_HIGHEST_DDR3200
	if((emi_set->type)==TYPE_LPDDR4)
	{
		// total bw for LP4
		// from 1200 to 2400, 0x5 = 406.35 * 5 = 2031.746MB/s
		// from 2400 to 3200, 0x9 = 406.35 * 9 = 3657.143MB/s
		*((volatile unsigned *)(EMI_BWCT0)) = 0x09000515;
		*((volatile unsigned *)(EMI_BWCT0_3RD)) = 0x0;
	}
#else
	if((emi_set->type)==TYPE_LPDDR4)
	{
		// total bw for LP4
		// from 1200 to 1600, 0x5 = 406.35 * 5 = 2031.746MB/s
		// from 1600 to 2400/2667, 0x6 = 406.35 * 6 = 2438.095MB/s
		*((volatile unsigned *)(EMI_BWCT0)) = 0x06000515;
		*((volatile unsigned *)(EMI_BWCT0_3RD)) = 0x0;

	#if EMI_CPU_ONLY_MON
		// set dvfs threshold of cpu only bandwidth
		*((volatile unsigned *)(EMI_BWCT0_6TH)) = 0x04010315;
	#endif
	}
#endif

#if OLD_CODE
	//The following is EMI patch
#if !__ETT__
#ifdef FIX_BUILD_FAIL
	// Enable MPU violation interrupt to MD for D1 and D7
	*((volatile unsigned int *)EMI_MPU_CTRL_D(1)) |= 0x10;
	*((volatile unsigned int *)EMI_MPU_CTRL_D(7)) |= 0x10;

        // DVFS threshold
        if (u1IsLP4Family(mt_get_dram_type_from_hw_trap())){
	/*LP4*/
                *((volatile unsigned int *)EMI_BWCT0) = 0x0a000705;
                *((volatile unsigned int *)EMI_BWCT0_3RD) = 0x0;
        } else {
                *((volatile unsigned int *)EMI_BWCT0) = 0x07000505; //total BW setting for VcoreDVFS
                *((volatile unsigned int *)EMI_BWCT0_3RD) = 0x0;
        }

	// EMI QoS0.5
	*((volatile unsigned int *) EMI_BWCT0_2ND) = 0x00030023; // monitor AP
	*((volatile unsigned int *) EMI_BWCT0_4TH) = 0x00C00023; // monitor GPU
	*((volatile unsigned int *) EMI_BWCT0_5TH) = 0x00240023; // monitor MM

#ifdef LAST_EMI
	last_emi_info_ptr = (LAST_EMI_INFO_T *) get_dbg_info_base(KEY_LAST_EMI);
	last_emi_info_ptr->decs_magic = LAST_EMI_MAGIC_PATTERN;
#if CFG_LAST_EMI_BW_DUMP
	last_emi_info_ptr->decs_ctrl = 0xDECDDECD;
#else
	last_emi_info_ptr->decs_ctrl = 0xDEC8DEC8;
#endif
	last_emi_info_ptr->decs_dram_type = 0;
	last_emi_info_ptr->decs_diff_us = 0;
#endif
#endif
#endif
#endif
}

#if !__ETT__
#ifdef FIX_BUILD_FAIL
void reserve_emi_mbw_buf(void)
{
	unsigned long long rsv_start;
	dram_addr_t dram_addr;

	dram_addr.ch = 0;
	dram_addr.rk = 0;
	get_dramc_addr(&dram_addr, 0x0);

	if (dram_addr.full_sys_addr > 0xFFFFFFFF)
		rsv_start = mblock_reserve_ext(&bootarg.mblock_info, 0x800000, 0x800000, 0x100000000, 0, "emi_mbw_buf");
	else
		rsv_start = mblock_reserve_ext(&bootarg.mblock_info, 0x800000, 0x800000, dram_addr.full_sys_addr, 0, "emi_mbw_buf");

#ifdef LAST_EMI
	last_emi_info_ptr->mbw_buf_l = (unsigned int) (rsv_start & 0xFFFFFFFF);
	last_emi_info_ptr->mbw_buf_h = (unsigned int) (rsv_start >> 32);
#endif
}
#endif
#endif

static void EMI_rank_swap_emi_setting(EMI_SETTINGS *emi_set)
{
	static unsigned int temp;

	if (emi_set->EMI_CONA_VAL & 0x20000) {
		temp = emi_set->EMI_CONA_VAL;
		emi_set->EMI_CONA_VAL &= ~(0xF3F0F0F0);
		emi_set->EMI_CONA_VAL |= (temp & 0xC0C0C0C0) >> 2;
		emi_set->EMI_CONA_VAL |= (temp & 0x30303030) << 2;
		emi_set->EMI_CONA_VAL |= (temp & 0x02000000) >> 1;
		emi_set->EMI_CONA_VAL |= (temp & 0x01000000) << 1;

		temp = emi_set->EMI_CONH_VAL;
		emi_set->EMI_CONH_VAL &= ~(0xFFFF0030);
		emi_set->EMI_CONH_VAL |= (temp & 0xF0F00000) >> 4;
		emi_set->EMI_CONH_VAL |= (temp & 0x0F0F0000) << 4;
		emi_set->EMI_CONH_VAL |= (temp & 0x00000020) >> 1;
		emi_set->EMI_CONH_VAL |= (temp & 0x00000010) << 1;

		temp = emi_set->CHN0_EMI_CONA_VAL;
		emi_set->CHN0_EMI_CONA_VAL &= ~(0x00FFF0FC);
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x00F00000) >> 4;
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x000F0000) << 4;
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x0000C0C0) >> 2;
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x00003030) << 2;
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x00000008) >> 1;
		emi_set->CHN0_EMI_CONA_VAL |= (temp & 0x00000004) << 1;

		temp = emi_set->CHN1_EMI_CONA_VAL;
		emi_set->CHN1_EMI_CONA_VAL &= ~(0x00FFF0FC);
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x00F00000) >> 4;
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x000F0000) << 4;
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x0000C0C0) >> 2;
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x00003030) << 2;
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x00000008) >> 1;
		emi_set->CHN1_EMI_CONA_VAL |= (temp & 0x00000004) << 1;

		emi_set->CHN0_EMI_CONA_VAL |= 0x80000000;
		emi_set->CHN1_EMI_CONA_VAL |= 0x80000000;

		temp = emi_set->DRAM_RANK_SIZE[0];
		emi_set->DRAM_RANK_SIZE[0] = emi_set->DRAM_RANK_SIZE[1];
		emi_set->DRAM_RANK_SIZE[1] = temp;

		if (emi_set->dram_cbt_mode_extern == CBT_R0_NORMAL_R1_BYTE)
			emi_set->dram_cbt_mode_extern = CBT_R0_BYTE_R1_NORMAL;
		else if (emi_set->dram_cbt_mode_extern == CBT_R0_BYTE_R1_NORMAL)
			emi_set->dram_cbt_mode_extern = CBT_R0_NORMAL_R1_BYTE;
	}
}

static void EMI_rank_swap_handle(void)
{
	static unsigned int handled = 0;
	int i;

	if (!handled) {
		EMI_rank_swap_emi_setting(&default_emi_setting);

		for (i = 0 ; i < num_of_emi_records; i++)
			EMI_rank_swap_emi_setting(&emi_settings[i]);

		handled = 1;
	}
}

void EMI_set_rank_swap(unsigned int enable)
{
	rank_swap = (enable) ? 1 : 0;
}

void EMI_Init(DRAMC_CTX_T *p)
{
	EMI_SETTINGS *emi_set;
	unsigned temp;

	emi_set = get_emi_setting();
	EMI_ESL_Setting1();

	DramcBroadcastOnOff(DRAMC_BROADCAST_OFF);

	emi_set->EMI_CONA_VAL &= ~(3UL<<8); //EMI_CONA[9:8] = 0: single channel, 1 dual channel, 2 quad channel
	// to check
	emi_set->EMI_CONF_VAL = 0x00421000;

	*(volatile unsigned *)EMI_CONA = emi_set->EMI_CONA_VAL;
	*(volatile unsigned *)EMI_CONF = emi_set->EMI_CONF_VAL;
	*(volatile unsigned *)EMI_CONH = emi_set->EMI_CONH_VAL;

	if ((emi_set->type)== TYPE_LPDDR3) {
	#ifdef LP3_128MB_SUPPORT
		//128M byte
	#else
		// 64M byte
		*((volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)) = 0xffffeaf9;
	#endif
		//if (dramc_get_efuse_dram_size() == DRAM_SIZE_512Mb) {
		//	*((volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)) = 0xffffeaf9;
		//} else if (dramc_get_efuse_dram_size() == DRAM_SIZE_256Mb){
		//	*((volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)) = 0xffffd0f9;
		//}
	}

	*(volatile unsigned *)CHN_EMI_CONA(CHN0_EMI_BASE) = emi_set->CHN0_EMI_CONA_VAL;

	//CMD_EN
	temp = *((volatile unsigned *)(EMI_CONM));
	temp |= (0x1<<10);
	*((volatile unsigned *)( EMI_CONM))=temp;


	// bit0: emi_enable, set bit0 to 1 after emi initialization
	*(volatile unsigned *)CHN_EMI_CONC(CHN0_EMI_BASE) = 0x5;
	dsb();

	p->vendor_id = emi_set->iLPDDRX_MODE_REG_5;


#if OLD_CODE
	EMI_SETTINGS *emi_set;

	emi_set = get_emi_setting();

	if (rank_swap)
		EMI_rank_swap_handle();

	EMI_ESL_Setting1(); //Copy Paste from DE

	//The following is MDL settings
	*(volatile unsigned *)EMI_CONA = emi_set->EMI_CONA_VAL;
	*(volatile unsigned *)EMI_CONH = emi_set->EMI_CONH_VAL;

	// CHNA and CHNB uses the same CH0 setting
	*(volatile unsigned *)CHN_EMI_CONA(CHN0_EMI_BASE) = emi_set->CHN0_EMI_CONA_VAL;
	*(volatile unsigned *)CHN_EMI_CONA(CHN1_EMI_BASE) = emi_set->CHN1_EMI_CONA_VAL;

//#if CHANNEL_NUM == 1
//	*(volatile unsigned *)EMI_CONF = 0x0;
//#else
	*(volatile unsigned *)EMI_CONF = emi_set->EMI_CONF_VAL;
	if (u1IsLP4Family(p->dram_type))
	{
		*(volatile unsigned *)CHN_EMI_CONC(CHN0_EMI_BASE) = 0x0;
		*(volatile unsigned *)CHN_EMI_CONC(CHN1_EMI_BASE) = 0x0;
	} else {
		*(volatile unsigned *)CHN_EMI_CONC(CHN0_EMI_BASE) = emi_set->EMI_CONF_VAL;
		*(volatile unsigned *)CHN_EMI_CONC(CHN1_EMI_BASE) = emi_set->EMI_CONF_VAL;
	}
//#endif
#endif

#if 0
#if __ETT__ && !defined(__LPAE__)
	/*
	 * special emi mapping w/o LPAE support
	 * RANK0_MAX_SIZE_ETT = 0x40000000 --> RANK0 @0x4000_0000~0x7fff_ffff
	 * RANK1_MAX_SIZE_ETT = 0x40000000 --> RANK1 @0x8000_0000~0xbfff_ffff
	 */
	/* set all the rank size of all the channels to the minimum value */
	*(volatile unsigned int*)EMI_CONH = ((*(volatile unsigned int*)EMI_CONH & 0x0000ffff) | (0x11110000));

	/* set all the rank size of all channel-0 to the minimum value */
	*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE)
		= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE) & 0xff00ffff ) | (0x00110000));

	/* set all the rank size of all channel-1 to the minimum value */
	*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE)
		= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE) & 0xff00ffff ) | (0x00110000));

#endif
#endif

	dsb();
}

void EMI_Init2(void)
{
	EMI_ESL_Setting2(); //Copy Paste from DE

	EMI_Patch(); //Please add the EMI patch here
}

static unsigned int emi_conh = 0;
static unsigned int chn_emi_cona[2] = {0, 0};

/* return the start address of rank1 */
unsigned int set_emi_before_rank1_mem_test(void)
{
	unsigned int emi_cona;

	emi_cona = *(volatile unsigned int*)EMI_CONA;
	emi_conh = *(volatile unsigned int*)EMI_CONH;
	chn_emi_cona[0] = *(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE);
	chn_emi_cona[1] = *(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE);

	if (emi_cona & 0x100) {
		/* set all the rank size of all the channels to the minimum value */
		*(volatile unsigned int*)EMI_CONH = ((*(volatile unsigned int*)EMI_CONH & 0x0000ffff) | (0x22220000));

		/* set all the rank size of all channel-0 to the minimum value */
		*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE)
			= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE) & 0xff00ffff ) | (0x00220000));

		/* set all the rank size of all channel-1 to the minimum value */
		*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE)
			= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE) & 0xff00ffff ) | (0x00220000));
	} else {
		/* set all the rank size of all the channels to the minimum value */
		*(volatile unsigned int*)EMI_CONH = ((*(volatile unsigned int*)EMI_CONH & 0x0000ffff) | (0x44440000));

		/* set all the rank size of all channel-0 to the minimum value */
		*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE)
			= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE) & 0xff00ffff ) | (0x00440000));

		/* set all the rank size of all channel-1 to the minimum value */
		*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE)
			= ((*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE) & 0xff00ffff ) | (0x00440000));
	}

	dsb();
	return 0x40000000;
}

void restore_emi_after_rank1_mem_test(void)
{
	*(volatile unsigned int*)EMI_CONH = emi_conh;
	*(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE) = chn_emi_cona[0];
	*(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE) = chn_emi_cona[1];
	dsb();
}

unsigned int check_gating_error(void)
{
	unsigned int ret = 0, i, phy_base, err_code = 0;
	unsigned int misc_stberr_rk0_r, misc_stberr_rk0_f, misc_stberr_rk1_r, misc_stberr_rk1_f;

	phy_base = Channel_A_PHY_AO_BASE_ADDRESS;

	for (i = 0; i < CHANNEL_NUM; ++i, phy_base += 0x8000, err_code = 0) {
		misc_stberr_rk0_r = *(volatile unsigned int*)(phy_base + 0x510);
		misc_stberr_rk0_f = *(volatile unsigned int*)(phy_base + 0x514);
		misc_stberr_rk1_r = *(volatile unsigned int*)(phy_base + 0x518);
		misc_stberr_rk1_f = *(volatile unsigned int*)(phy_base + 0x51c);
		if (misc_stberr_rk0_r & (1 << 16)) {
			ret |= (1 << i);
#ifdef LAST_DRAMC
			if ((misc_stberr_rk0_r & 0xffff) != 0) {
				err_code |= ERR_DRAM_GATING_RK0_R;
			}
			if ((misc_stberr_rk0_f & 0xffff) != 0) {
				err_code |= ERR_DRAM_GATING_RK0_F;
			}
			if ((misc_stberr_rk1_r & 0xffff) != 0) {
				err_code |= ERR_DRAM_GATING_RK1_R;
			}
			if ((misc_stberr_rk1_f & 0xffff) != 0) {
				err_code |= ERR_DRAM_GATING_RK1_F;
			}
			dram_fatal_set_gating_err(i, err_code);
			dram_fatal_set_stberr(i, 0, (misc_stberr_rk0_r & 0xffff) | ((misc_stberr_rk0_f & 0xffff) << 16));
			dram_fatal_set_stberr(i, 1, (misc_stberr_rk1_r & 0xffff) | ((misc_stberr_rk1_f & 0xffff) << 16));
		} else {
			dram_fatal_set_gating_err(i, 0);
			dram_fatal_set_stberr(i, 0, 0);
			dram_fatal_set_stberr(i, 1, 0);
#endif
		}
	}

	return ret;
}

#if 0
void CHA_HWGW_Print(DRAMC_CTX_T *p)
{
    U8 u1RefreshRate;
    U32 backup_channel, chIdx;

    backup_channel = p->channel;

#if (FOR_DV_SIMULATION_USED==0)
    // Read HW gating tracking
#ifdef HW_GATING
    for(chIdx=0; chIdx<p->support_channel_num; chIdx++)
    {
        DramcPrintHWGatingStatus(p, chIdx);
    }
#endif

#if ENABLE_RX_TRACKING_LP4
    for(chIdx=0; chIdx<p->support_channel_num; chIdx++)
    {
        DramcPrintRXDQDQSStatus(p, chIdx);
    }
#endif

#ifdef IMPEDANCE_TRACKING_ENABLE
        if(u1IsLP4Family(p->dram_type))
        {
            DramcPrintIMPTrackingStatus(p, CHANNEL_A);
//CH_B HW Fail, can't use            DramcPrintIMPTrackingStatus(p, CHANNEL_B);
        }
#endif

#ifdef TEMP_SENSOR_ENABLE
        for(chIdx=0; chIdx<p->support_channel_num; chIdx++)
        {
            u1RefreshRate = u1GetMR4RefreshRate(p, chIdx);
            mcSHOW_ERR_MSG("[CH%d] MRR(MR4) [10:8]=%x\n", chIdx, u1RefreshRate);
        }
#endif
#endif

    vSetPHY2ChannelMapping(p, backup_channel);
}
#endif

void Dump_EMIRegisters(DRAMC_CTX_T *p)
{
#ifndef OLYMPUS_TO_BE_PORTING

  U8 ucstatus = 0;
  U32 uiAddr;
  U32 u4value;

  for (uiAddr=0; uiAddr<0x160; uiAddr+=4)
  {
    mcSHOW_DBG_MSG("EMI offset:%x, value:%x\n", uiAddr, *(volatile unsigned *)(EMI_APB_BASE+uiAddr));
  }
#endif
}

void print_DBG_info(DRAMC_CTX_T *p)
{
#ifndef OLYMPUS_TO_BE_PORTING

    unsigned int addr = 0x0;
    U32 u4value;

#ifdef DDR_INIT_TIME_PROFILING
    return;
#endif

    mcSHOW_DBG_MSG("EMI_CONA=%x\n",*(volatile unsigned *)(EMI_APB_BASE+0x00000000));
    mcSHOW_DBG_MSG("EMI_CONH=%x\n",*(volatile unsigned *)(EMI_APB_BASE+0x00000038));

    //RISCReadAll();
#endif
}

int mt_get_dram_type(void)
{
#if (fcFOR_CHIP_ID == fcSchubert)
    return (*((volatile unsigned *)(Channel_A_DRAMC_AO_BASE_ADDRESS+0x10)) >> 10) & 0x7;
#else
    #error No defined mt_get_dram_type for your chip !!!
#endif
}

int mt_get_freq_setting(DRAMC_CTX_T *p)
{
    return p->frequency;
}

#ifdef DDR_RESERVE_MODE
u32 g_ddr_reserve_enable;
u32 g_ddr_reserve_success;
#define TIMEOUT 3
extern void before_Dramc_DDR_Reserved_Mode_setting(void);
#endif

#ifdef DDR_RESERVE_MODE

#define	CHAN_DRAMC_NAO_MISC_STATUSA(base)	(base + 0x80)
#define SREF_STATE				(1 << 16)

static unsigned int is_dramc_exit_slf(void)
{
	unsigned int ret;
	U32 u4DramType = 0;

	u4DramType = *(volatile unsigned int *)(IO_PHYS + 0x0022C000 + 0x10) >> 10 & 0x7;

	if (u4DramType == 5)
	{
		ret = (*((volatile unsigned int *)(IO_PHYS + 0x00234080)) >> 16 & 0x1);
		if (ret != 0) {
			dramc_crit("PSRAM is in self-refresh (MISC_STATUSA = 0x%x)\n", ret);
			return 0;
		} else {
			dramc_crit("PSRAM is not in self-refresh\n");
			return 1;
		}
	}

	ret = *(volatile unsigned *)CHAN_DRAMC_NAO_MISC_STATUSA(Channel_A_DRAMC_NAO_BASE_ADDRESS);
	if ((ret & SREF_STATE) != 0) {
		dramc_crit("DRAM CHAN-A is in self-refresh (MISC_STATUSA = 0x%x)\n", ret);
		return 0;
	}
	dramc_crit("ALL DRAM CHAN is not in self-refresh\n");
	return 1;
}

#endif

unsigned int dramc_set_vcore_voltage(unsigned int vcore)
{
    mcSHOW_INFO_MSG("[%s] vol:%d\n", __func__, vcore);
#if ENABLE_LP3_SW
    #if WITH_VCORE_PWM_BUCK
		return regulator_set_voltage(vcore);
    #endif
#elif SUPPORT_TYPE_LPDDR4
    #if WITH_PMIC_BD71828
        bd71828_set_vcore_voltage(vcore);
    #elif WITH_VCORE_PWM_BUCK
        return regulator_set_voltage(vcore);
    #elif WITH_VCORE_I2C_BUCK
        rt5749_regulator_set_voltage(VCORE, vcore);
        return 0;
    #endif
#endif
	return 0;
}

unsigned int dramc_get_vcore_voltage(void)
{
#if ENABLE_LP3_SW
    #if WITH_VCORE_PWM_BUCK
		return regulator_get_voltage();
    #endif
#elif SUPPORT_TYPE_LPDDR4
    #if WITH_PMIC_BD71828
        return bd71828_get_vcore_voltage();
    #elif WITH_VCORE_PWM_BUCK
	    return regulator_get_voltage();
    #elif WITH_VCORE_I2C_BUCK
        rt5749_regulator_get_voltage(VCORE);
        return 0;
    #endif
#endif
	return 0;
}

#if LP4_MAX_2400
unsigned int dramc_set_vcore_sram_voltage(unsigned int vcore_sram)
{
    mcSHOW_INFO_MSG("[%s] vol:%d\n", __func__, vcore_sram);
#if ENABLE_LP3_SW
    // do nothing
#elif SUPPORT_TYPE_LPDDR4
    #if WITH_PMIC_BD71828
        bd71828_set_vcore_sram_voltage(vcore_sram);
    #endif
#endif
	return 0;
}

unsigned int dramc_get_vcore_sram_voltage(void)
{
#if ENABLE_LP3_SW
    // do nothing
#elif SUPPORT_TYPE_LPDDR4
    #if WITH_PMIC_BD71828
        return bd71828_get_vcore_sram_voltage();
    #endif
#endif
	return 0;
}
#endif

unsigned int dramc_set_vdd1_voltage(unsigned int ddr_type, unsigned int vdd1)
{
	unsigned int vio18_vocal;
	unsigned int vio18_votrim;

#if !__ETT__
	if (u1IsLP4Family(ddr_type))
		return 0;
#endif

	if (vdd1 > 1980000)
		vdd1 = 188;
	else if (vdd1 < 1730000)
		vdd1 = 173;
	else
		vdd1 = vdd1 / 10000;

	if (vdd1 > 190) {
		vio18_vocal = 0xa;
		vio18_votrim = (198 - vdd1);
		vio18_votrim |= 0x8;
	} else if (vdd1 < 180) {
		vio18_vocal = 0x0;
		vio18_votrim = (180 - vdd1);
	} else {
		vio18_vocal = (vdd1 - 180);
		vio18_votrim = 0x0;
	}

#if MTK_PMIC_CHIP_MT6357
	pmic_config_interface(PMIC_RG_VIO18_VOCAL_ADDR, vio18_vocal, PMIC_RG_VIO18_VOCAL_MASK, PMIC_RG_VIO18_VOCAL_SHIFT);
	pmic_config_interface(PMIC_RG_VIO18_VOTRIM_ADDR, vio18_votrim, PMIC_RG_VIO18_VOTRIM_MASK, PMIC_RG_VIO18_VOTRIM_SHIFT);
#endif

	return 0;
}

unsigned int dramc_get_vdd1_voltage(void)
{
	return 1800000; /*  pmic no api to get, return 1.8V here */
}

static unsigned int lp3_vdram_base[] = { 1100000, 1200000 };
static unsigned int lp3_vdram_ofs[] = { 0, 0 } ;//20000, 40000 };

unsigned int dramc_set_vdd2_voltage(unsigned int ddr_type, unsigned int vdd2)
{
	int ret;

	mcSHOW_INFO_MSG("[%s] vol:%d\n", __func__, vdd2);
	if (u1IsLP4Family(ddr_type)) {
        #if WITH_PMIC_BD71828
            bd71828_set_vmem_voltage(vdd2);
        #elif WITH_PMIC_MT6398
            if (vdd2 > 1100000) { /* rough calculate: if > 1.2V, set +5%, if < 1.2V, set -%5*/
                ret = pmic_config_interface(0x20, 0x2, 0x3, 0);   /* pmic fixed, can only increase 5% */
                if (ret != 0)
                    mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
            } else if (vdd2 < 1100000) {
                ret = pmic_config_interface(0x20, 0x1, 0x3, 0);   /* pmic fixed, can only decrease 5% */
                if (ret != 0)
                    mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
            } else {
                ret = pmic_config_interface(0x20, 0x0, 0x3, 0);
                if (ret != 0)
                    mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
            }
        #endif
	} else if(u1IsLP3Family(ddr_type)) {
		/* LPDDR3 */
		#if WITH_PMIC_MT6398
        if (vdd2 > 1200000) { /* rough calculate: if > 1.2V, set +5%, if < 1.2V, set -%5*/
            ret = pmic_config_interface(0x20, 0x2, 0x3, 0);   /* pmic fixed, can only increase 5% */
            if (ret != 0)
                mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
        } else if (vdd2 < 1200000) {
            ret = pmic_config_interface(0x20, 0x1, 0x3, 0);   /* pmic fixed, can only decrease 5% */
            if (ret != 0)
                mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
        } else {
            ret = pmic_config_interface(0x20, 0x0, 0x3, 0);
            if (ret != 0)
                mcSHOW_ERR_MSG("set vdd2:%d failed!\n", vdd2);
		}
#endif
	}
	return 0;
}

unsigned int dramc_get_vdd2_voltage(unsigned int ddr_type)
{
	int ret = 0;
    U8 val = 0;

#if SUPPORT_TYPE_LPDDR4
    int vdd2_vol[] = {1100000, 1045000, 1155000};	// {1.1V, 1.1V - 1.1*5%, 1.1V + 1.1 * 5%}
#elif ENABLE_LP3_SW
    int vdd2_vol[] = {1200000, 1140000, 1260000};	// {1.2V, 1.2V - 1.2*5%, 1.2V + 1.2 * 5%}
#else
	int vdd2_vol[] = {1200000, 1140000, 1260000};	// {1.2V, 1.2V - 1.2*5%, 1.2V + 1.2 * 5%}
#endif

	if (u1IsLP4Family(ddr_type) || u1IsLP3Family(ddr_type)) {
        #if WITH_PMIC_BD71828
            return bd71828_get_vmem_voltage();
		#elif WITH_PMIC_MT6398
            ret = pmic_read_interface(0x20, &val, 0x3, 0);
            if (ret == 0) {
                if (val > sizeof(vdd2_vol) / sizeof(vdd2_vol[0]))
                    return 0;
                return vdd2_vol[val];
            }
            else
                return 0;
		#endif
	} else {
		/* LPDDR3 */
#ifdef MTK_PMIC_CHIP_MT6357
		unsigned int sum = 0;

		sum = mtk_regulator_get_voltage(&reg_vdram);

		for (ret = 0;(unsigned int)ret < sizeof(lp3_vdram_base) / sizeof(*lp3_vdram_base);ret++) {
			if (sum == lp3_vdram_base[ret] + lp3_vdram_ofs[ret]) {
				break;
			}
		}

		if (ret == sizeof(lp3_vdram_base) / sizeof(*lp3_vdram_base))
			ret -= 1;

		pmic_read_interface(PMIC_RG_VDRAM_VOCAL_ADDR, &sum, PMIC_RG_VDRAM_VOCAL_MASK, PMIC_RG_VDRAM_VOCAL_SHIFT);

		sum = lp3_vdram_base[ret] + (sum * 10000);

		return sum;
#endif
	}

	return 0;
}

unsigned int dramc_set_vddq_voltage(unsigned int ddr_type, unsigned int vddq)
{
    int ret;

    mcSHOW_INFO_MSG("[%s] vol:%d\n", __func__, vddq);
    if (u1IsLP4Family(ddr_type)) {
        /* vddq and vdd2 are connected together */
        dramc_set_vdd2_voltage(ddr_type, vddq);
    }
	return 0;
}

unsigned int dramc_get_vddq_voltage(unsigned int ddr_type)
{
//    int ret;
//    int val = 0;

    if (u1IsLP4Family(ddr_type) || u1IsLP3Family(ddr_type)) {
        return dramc_get_vdd2_voltage(ddr_type);
	}

    return 0;
}

void dramc_set_vdram_voltage(unsigned int ddr_type, unsigned int vol)
{
    dramc_set_vdd2_voltage(ddr_type, vol); //1.1V, VDDQ use the same power with VDD2
}

unsigned int mt_get_dram_type_from_hw_trap(void)
{
	static unsigned int ddr_type_detected = 0;
	static unsigned int pmic_trap_ddr_type = TYPE_LPDDR3;

	if (!ddr_type_detected) {
#ifdef MTK_PMIC_CHIP_MT6357
		unsigned int hw_trap;
		hw_trap = get_dram_type();	//for build pass

		switch (hw_trap) {
			case 0:
				pmic_trap_ddr_type = TYPE_LPDDR3;
				break;
			case 1:
				pmic_trap_ddr_type = TYPE_LPDDR4;
				break;
			case 2:
				pmic_trap_ddr_type = TYPE_LPDDR4X;
				break;
			case 3:
				pmic_trap_ddr_type = TYPE_LPDDR4P;
				break;
			default:
				dramc_crit("[dramc] Wrong HW TRAP\n");
				ASSERT(0);
				break;
		}
		dramc_debug("PMIC TRAP GET DDR TYPE: 0x%x\n", pmic_trap_ddr_type);
#endif
		ddr_type_detected = 1;
	}

	return pmic_trap_ddr_type;
}

void setup_dramc_voltage_by_pmic(void)
{
	unsigned int ddr_type = mt_get_dram_type_from_hw_trap();
	int ret;

#ifdef MTK_PMIC_CHIP_MT6357
	ret = mtk_regulator_get("vcore", &reg_vcore);
	if (ret)
		dramc_crit("mtk_regulator_get vcore fail\n");

	ret = mtk_regulator_get("vdram", &reg_vdram);
	if (ret)
		dramc_crit("mtk_regulator_get vdram fail\n");

	mtk_regulator_set_mode(&reg_vcore, 0x1);
#endif

	if (u1IsLP4Family(ddr_type)) {
#ifdef MTK_PMIC_CHIP_MT6357
		/* LPDDR4 */
		ret = mtk_regulator_enable(&reg_vdram, 0);
		if (ret)
			dramc_crit("disable reg_vdram failed\n");
#endif
#if MTK_PMIC_CHIP_MT6357
		ret = rt5738_enable(RT5738_VDD2, 1);

		if (ret == 0) {
			dramc_crit("enable RT5738_VDD2 fail (ret = %d)\n", ret);
			return;
		}

		ret = rt5738_enable(RT5738_VDDQ, 1);

		if (ret == 0) {
			dramc_crit("enable RT5738_VDDQ fail (ret = %d)\n", ret);
			return;
		}

		rt5738_set_mode(RT5738_VDD2, 0x1);
		rt5738_set_mode(RT5738_VDDQ, 0x1);
#endif
#ifdef VCORE_BIN
		dramc_set_vcore_voltage(get_vcore_uv_table(0));
#else
		dramc_set_vcore_voltage(SEL_PREFIX_VCORE(LP4, KOPP0));
#endif

		dramc_set_vdd2_voltage(ddr_type, SEL_PREFIX_VDRAM(LP4));
		dramc_set_vddq_voltage(ddr_type, SEL_PREFIX_VDDQ);
	}
#if ENABLE_LP3_SW
	else {
		/* LPDDR3 */
#ifdef VCORE_BIN
		dramc_set_vcore_voltage(get_vcore_uv_table(0));
#else
		dramc_set_vcore_voltage(SEL_PREFIX_VCORE(LP3, KOPP0));
#endif
		dramc_set_vdd2_voltage(ddr_type, SEL_PREFIX_VDRAM(LP3));
	}
#endif
	dramc_crit("Vcore = %d\n", dramc_get_vcore_voltage());
	dramc_crit("Vdram = %d\n", dramc_get_vdd2_voltage(ddr_type));

	if (u1IsLP4Family(ddr_type))
		dramc_crit("Vddq = %d\n", dramc_get_vddq_voltage(ddr_type));
}

static void restore_vcore_setting(void)
{
#ifdef MTK_PMIC_CHIP_MT6357
	int ret;

	ret = mtk_regulator_get("vcore", &reg_vcore);
	if (ret)
		dramc_crit("mtk_regulator_get vcore fail\n");
#endif

	if (u1IsLP4Family(mt_get_dram_type_from_hw_trap()))
#ifdef VCORE_BIN
		dramc_set_vcore_voltage(get_vcore_uv_table(0));
#else
		dramc_set_vcore_voltage(SEL_PREFIX_VCORE(LP4, KOPP0));
#endif
	else
#ifdef VCORE_BIN
		dramc_set_vcore_voltage(get_vcore_uv_table(0));
#else
		dramc_set_vcore_voltage(SEL_PREFIX_VCORE(LP3, KOPP0));
#endif

	dramc_crit("Vcore = %d\n", dramc_get_vcore_voltage());
}

static void restore_pmic_setting(void)
{
	unsigned int ddr_type = mt_get_dram_type_from_hw_trap();
	int ret;

	restore_vcore_setting();

#ifdef MTK_PMIC_CHIP_MT6357
	ret = mtk_regulator_get("vdram", &reg_vdram);
	if (ret) {
		dramc_crit("mtk_regulator_get vdram fail\n");
		return;
	}
#endif

	if (u1IsLP4Family(ddr_type)) {
		/* LPDDR4 */
#ifdef MTK_PMIC_CHIP_MT6357
		ret = mtk_regulator_enable(&reg_vdram, 0);
		if (ret)
			dramc_crit("disable reg_vdram failed\n");
#endif
		dramc_set_vdd2_voltage(ddr_type, SEL_PREFIX_VDRAM(LP4));
		dramc_set_vddq_voltage(ddr_type, SEL_PREFIX_VDDQ);
	} else
		dramc_set_vdd2_voltage(ddr_type, SEL_PREFIX_VDRAM(LP3));

	dramc_crit("Vdram = %d\n", dramc_get_vdd2_voltage(ddr_type));
	if (u1IsLP4Family(ddr_type))
		dramc_crit("Vddq = %d\n", dramc_get_vddq_voltage(ddr_type));
}

void switch_dramc_voltage_to_auto_mode(void)
{
#ifdef MTK_PMIC_CHIP_MT6357
	mtk_regulator_set_mode(&reg_vcore, 0x0);
#endif
#if MTK_PMIC_CHIP_MT6357
	if (u1IsLP4Family(mt_get_dram_type_from_hw_trap())) {
		rt5738_set_mode(RT5738_VDD2, 0x0);
		rt5738_set_mode(RT5738_VDDQ, 0x0);
	}
#endif
}

void release_dram(void)
{
#ifdef DDR_RESERVE_MODE
    int i;
    int counter = TIMEOUT;
#if PSRAM_SPEC
	U32 dramtype;
#endif

    // scy: restore pmic setting (VCORE, VDRAM, VSRAM, VDDQ)
    //restore_pmic_setting();
    rgu_release_rg_dramc_conf_iso();//Release DRAMC/PHY conf ISO
#if PSRAM_SPEC
	dramtype = (*((volatile unsigned int *)(IO_PHYS + 0x0022C010)) >> 10 & 0x7);
    if (dramtype == 5)
		Psram_DDR_Reserved_Mode_setting();
	else
#endif
    Dramc_DDR_Reserved_Mode_setting();
    rgu_release_rg_dramc_iso();//Release PHY IO ISO
    rgu_release_rg_dramc_sref();//Let DRAM Leave SR

    // setup for EMI: touch center EMI and channel EMI to enable CLK
    dramc_crit("[DDR reserve] EMI CONA: %x\n", *(volatile unsigned int*)EMI_CONA);
    dramc_crit("[DDR reserve] EMI CHA CONA: %x\n", *(volatile unsigned int*)CHN_EMI_CONA(CHN0_EMI_BASE));
    dramc_crit("[DDR reserve] EMI CHB CONA: %x\n", *(volatile unsigned int*)CHN_EMI_CONA(CHN1_EMI_BASE));
#if PSRAM_SPEC
	dramc_crit("[DDR reserve] Psram 0x10232400: %x\n", *((volatile unsigned int *)(IO_PHYS + 0x00232400)));
#endif
    for (i=0;i<10;i++);

    while(counter)
    {
        if(is_dramc_exit_slf() == 1) /* expect to exit dram-self-refresh */
            break;
        counter--;
    }

    if(counter == 0)
    {
        if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1)
        {
            dramc_crit("[DDR Reserve] release dram from self-refresh FAIL!\n");
            g_ddr_reserve_success = 0;
        }
    }
    else
    {
         dramc_crit("[DDR Reserve] release dram from self-refresh PASS!\n");
#if SUPPORT_TYPE_LPDDR3
	#ifdef TRIGGER_WDT_RESERVE_MODE_BY_DDR_SELF
		 cpu_mem_test_vfy(0x00, get_dram_size());
	#endif
#endif
    }
#if PSRAM_SPEC
	if (dramtype == 5)
		Psram_DDR_Reserved_Mode_AfterSR();
	else
#endif
    Dramc_DDR_Reserved_Mode_AfterSR();
    //Expect to Use LPDDR3200 and PHYPLL as output, so no need to handle
    //shuffle status since the status will be reset by system reset
    //There is an PLLL_SHU_GP in SPM which will reset by system reset
    return;
#endif
}

void check_ddr_reserve_status(void)
{
	/* get status of DCS and DVFSRC */
    int dcs_success/* = rgu_is_emi_dcs_success()*/, dvfsrc_success/* = rgu_is_dvfsrc_success()*/;
    int dcs_en/* = rgu_is_emi_dcs_enable()*/, dvfsrc_en/* = rgu_is_dvfsrc_enable()*/;

    /* EMI SPM workaround for Bianco only: toggle mask */
    *(volatile unsigned int*) (CHN0_EMI_BASE + 0x3FC) &= ~0x1;

	dprintf(ALWAYS, "WDT_MODE:0x%x, WDT_DRAMC_CTL:0x%x\n", readl(MTK_WDT_MODE), readl(MTK_WDT_DRAMC_CTL));
	
#ifdef DDR_RESERVE_MODE
    int counter = TIMEOUT;
    if(rgu_is_reserve_ddr_enabled())
    {
      g_ddr_reserve_enable = 1;
#ifdef LAST_DRAMC
      dram_fatal_set_ddr_rsv_mode_flow();
#endif

      if(rgu_is_reserve_ddr_mode_success())
      {
        while(counter)
        {
          if(rgu_is_dram_slf())
          {
            g_ddr_reserve_success = 1;
            break;
          }
          counter--;
        }
        if(counter == 0)
        {
          mcSHOW_DBG_MSG("[DDR Reserve] ddr reserve mode success but DRAM not in self-refresh!\n");
          g_ddr_reserve_success = 0;
#ifdef LAST_DRAMC
	  dram_fatal_set_ddr_rsv_mode_err();
#endif
        }
      }
      else
      {
        mcSHOW_ERR_MSG("[DDR Reserve] ddr reserve mode FAIL!\n");
        g_ddr_reserve_success = 0;
#ifdef LAST_DRAMC
	  dram_fatal_set_ddr_rsv_mode_err();
#endif
      }
	/* Disable DDR-reserve mode in pre-loader stage then enable it again in kernel stage */
	//rgu_dram_reserved(0);

	/* overwrite g_ddr_reserve_success if some of dcs/dvfsrc/drs failed */
	/* TODO: check DRS status */
	if ((dcs_en == 1 && dcs_success == 0) || (dvfsrc_en == 1 && dvfsrc_success == 0)) {
		mcSHOW_ERR_MSG("[DDR Reserve] DRAM content might be corrupted -> clear g_ddr_reserve_success\n");
		g_ddr_reserve_success = 0;

		if (dvfsrc_en == 1 && dvfsrc_success == 0) {
			mcSHOW_ERR_MSG("[DDR Reserve] DVFSRC fail!\n");
#if 0//def LAST_DRAMC
			dram_fatal_set_dvfsrc_err();
#endif
		}

		if (dcs_en == 1 && dcs_success == 0) {
			mcSHOW_ERR_MSG("[DDR Reserve] DCS fail!\n");
#if 0 //def LAST_DRAMC
			dram_fatal_set_emi_dcs_err();
#endif
		}
	} else {
		mcSHOW_DBG_MSG("[DDR Reserve] DCS/DVFSRC success! (dcs_en=%d, dvfsrc_en=%d)\n", dcs_en, dvfsrc_en);
	}
	/* release dram, no matter success or failed */
	release_dram();
    }
    else
    {
      mcSHOW_ERR_MSG("[DDR Reserve] ddr reserve mode not be enabled yet\n");
      g_ddr_reserve_enable = 0;
    }
#endif
}

unsigned int DRAM_MRR(int MRR_num)
{
    u16 MRR_value = 0x0;
    DRAMC_CTX_T *p = psCurrDramCtx;

    DramcModeRegRead(p, MRR_num, &MRR_value);
    return MRR_value;
}

static int mt_get_dram_type_for_dis(void)
{
    return mt_get_dram_type_from_hw_trap();
}

#ifdef COMBO_MCP
static char id[22];
static int emmc_nand_id_len=16;
static int fw_id_len;
static int mt_get_mdl_number(void)
{
    static int found = 0;
    static int mdl_number = -1;
    int i;
    int j;
    int has_emmc_nand = 0;
    int discrete_dram_num = 0;
    int mcp_dram_num = 0;
    u64 rank0_size=0, rank1_size=0;

    unsigned int dram_type;
    DRAM_INFO_BY_MRR_T DramInfo;
    DRAM_DRAM_TYPE_T Dis_DramType;
    DRAM_CBT_MODE_EXTERN_T DramMode;

    if (!(found)) {
        int result=0;

        for (i = 0 ; i < num_of_emi_records; i++) {
            if ((emi_settings[i].type & 0x0F00) == 0x0000)
                discrete_dram_num ++;
            else
                mcp_dram_num ++;
        }

        /*If the number >=2  &&
         * one of them is discrete DRAM
         * enable combo discrete dram parse flow
         * */
        if ((discrete_dram_num > 0) && (num_of_emi_records >= 2))
            enable_combo_dis = 1;

        dramc_crit("[EMI] mcp_dram_num:%d,discrete_dram_num:%d,enable_combo_dis:%d\r\n",mcp_dram_num,discrete_dram_num,enable_combo_dis);

#if DUAL_FREQ_K || __FLASH_TOOL_DA__
        Dis_DramType = mt_get_dram_type_for_dis();
#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        if (!u1IsLP4Family(Dis_DramType) || read_offline_dram_mdl_data(&DramInfo)<0) {
#endif
        if (u1IsLP4Family(Dis_DramType))
            DramMode = CBT_BYTE_MODE1;
        else
            DramMode = CBT_NORMAL_MODE;

        Init_DRAM(Dis_DramType, DramMode, &DramInfo, GET_MDL_USED);
        DramInfo.u2MR5VendorID &= 0xFF;

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION
        if (u1IsLP4Family(Dis_DramType))
            write_offline_dram_mdl_data(&DramInfo);
        }
#endif

        rank0_size = (u64) DramInfo.u8MR8Density[0][0]; //now only K CHA to save time
        rank1_size = (u64) DramInfo.u8MR8Density[0][1]; //now only K CHA to save time
#endif
        /*
         *
         * 0. if there is only one discrete dram, use index=0 emi setting and boot it.
         * */
        if ((0 == mcp_dram_num) && (1 == discrete_dram_num)) {
            mdl_number = 0;
            found = 1;

            //K first frequency (1066MHz)

            return mdl_number;
        }

        /* 1.
         * if there is MCP dram in the list, we try to find emi setting by emmc ID
         * */
        if (mcp_dram_num > 0) {
//            result = platform_get_mcp_id(id, emmc_nand_id_len,&fw_id_len);	//for build pass
            for (i = 0; i < num_of_emi_records; i++) {
                if (emi_settings[i].type != 0) {
                    if ((emi_settings[i].type & 0xF00) != 0x000) {
                        /* valid ID */
                        if (result == 0
#if DUAL_FREQ_K || __FLASH_TOOL_DA__
                                && (emi_settings[i].DRAM_RANK_SIZE[0] == rank0_size) &&
                                (emi_settings[i].DRAM_RANK_SIZE[1] == rank1_size)
#endif
                           ) {
                            if ((emi_settings[i].type & 0xF00) == 0x200) {
                                /* eMMC */
                                if (memcmp(id, emi_settings[i].ID, emi_settings[i].id_length) == 0) {
                                    mdl_number = i;
                                    found = 1;
                                    break;
                                } else {
                                    dramc_crit("[MDL] index(%d) emmc id match failed\n",i);
                                }
                            }
                            else if ((emi_settings[i].type & 0xF00) == 0x300)
                            {
                                /* UFS */
                                if (memcmp(id, emi_settings[i].ID, emi_settings[i].id_length) == 0){
                                    mdl_number = i;
                                    found = 1;
                                    break;
                                } else {
                                    dramc_crit("[MDL] index(%d) ufs id match failed\n",i);
                                }
                            }
                        }
                    }
                }
            }
        }

        /* 2. find emi setting by MODE register 5
         * */
        // if we have found the index from by eMMC ID checking, we can boot android by the setting
        // if not, we try by vendor ID
        if ((0 == found) && (1 == enable_combo_dis))
        {
            EMI_SETTINGS *emi_set;

            // try to find discrete dram by MR5 (vendor ID)
            for (i = 0; i < num_of_emi_records; i++) {
                dramc_crit("[MDL]index:%d, MR5:%x, type:%x, vender_id:%x\n", i, emi_settings[i].iLPDDRX_MODE_REG_5, emi_settings[i].type, DramInfo.u2MR5VendorID);
                // only check discrete dram type
                if ((emi_settings[i].type & 0xF) == Dis_DramType && (emi_settings[i].type & 0x0F00) == 0x0000) {
                    // support for combo discrete dram
                    if (emi_settings[i].iLPDDRX_MODE_REG_5 == DramInfo.u2MR5VendorID) {
                        dramc_crit("[MDL] index:%d, rank0_size:0x%llx(0x%llx), rank1_size:0x%llx(0x%llx)\n", i,
                            rank0_size, emi_settings[i].DRAM_RANK_SIZE[0],
                            rank1_size, emi_settings[i].DRAM_RANK_SIZE[1]);
                        if((emi_settings[i].DRAM_RANK_SIZE[0] == rank0_size) && (emi_settings[i].DRAM_RANK_SIZE[1] == rank1_size)) {
                            mdl_number = i;
                            found = 1;
                            break;
                        }
                    }
                }
            }
        }

        dramc_crit("found:%d,i:%d\n",found,i);
    }

    return mdl_number;
}
#endif

int get_dram_channel_nr(void)
{
    int channel_nr;

    channel_nr = (*((volatile unsigned int*)(EMI_CONA)) >> 8) & 0x3;

    return (0x1 << channel_nr);
}

int get_dram_rank_nr(void)
{

    int index;
    int emi_cona;

#ifdef COMBO_MCP
  #ifdef DDR_RESERVE_MODE
    if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1)
    {
      emi_cona = *(volatile unsigned int*)(EMI_CONA);
    }
    else
  #endif
    {
      index = mt_get_mdl_number();
      if (index < 0 || index >=  num_of_emi_records)
      {
          return -1;
      }

      emi_cona = emi_settings[index].EMI_CONA_VAL;
    }
#else
    emi_cona = default_emi_setting.EMI_CONA_VAL;
#if CFG_FPGA_PLATFORM
    return 1;
#endif
#endif

    if ((emi_cona & (1 << 17)) != 0 || //for channel 0
        (emi_cona & (1 << 16)) != 0 )  //for channel 1
        return 2;
    else
        return 1;
}

void get_dram_rank_size_by_EMI_CONA (u64 dram_rank_size[])
{
    unsigned col_bit, row_bit;
    u64 ch0_rank0_size, ch0_rank1_size, ch1_rank0_size, ch1_rank1_size;
#ifndef COMBO_MCP
    unsigned emi_cona = default_emi_setting.EMI_CONA_VAL;
    unsigned emi_conh = default_emi_setting.EMI_CONH_VAL;
#else
    unsigned emi_cona = *(volatile unsigned int*)(EMI_CONA);
    unsigned emi_conh = *(volatile unsigned int*)(EMI_CONH);
#endif
    unsigned nr_chan_enabled = 1;
    u64 per_chan_rank0_size = 0, per_chan_rank1_size = 0;
    unsigned shift_for_16bit = 1;	// data width = 2 bytes

    if (emi_cona & 0x2)
        shift_for_16bit = 0;		// data width = 4 bytes

    dram_rank_size[0] = 0;
    dram_rank_size[1] = 0;

    ch0_rank0_size = (emi_conh >> 16) & 0xf;
    ch0_rank1_size = (emi_conh >> 20) & 0xf;
    ch1_rank0_size = (emi_conh >> 24) & 0xf;
    ch1_rank1_size = (emi_conh >> 28) & 0xf;

    switch ((emi_cona >> 8) & 0x3) {
	    case 0:
		    nr_chan_enabled = 1;
		    break;
	    case 1:
		    nr_chan_enabled = 2;
		    break;
	    case 2:
		    nr_chan_enabled = 4;
		    break;
	    case 3:
	    default:
		    dramc_crit("invalid CHN_EN field in EMI_CONA (0x%x)\n", emi_cona);
		    // assume 4 channel by default
		    nr_chan_enabled = 2;
		    break;
    }

    // CH0 EMI
    {
        if(ch0_rank0_size == 0)
        {
            //rank 0 setting
            col_bit = ((emi_cona >> 4) & 0x03) + 9;
            row_bit = ((((emi_cona >> 24) & 0x01) << 2) + ((emi_cona >> 12) & 0x03)) + 13;
            per_chan_rank0_size = ((u64)(1 << (row_bit + col_bit))) * ((u64)(4 >> shift_for_16bit) * 8); // data width (bytes) * 8 banks
        }
        else
        {
            per_chan_rank0_size = (ch0_rank0_size * 256 << 20);
        }

        if (0 != (emi_cona &  (1 << 17)))   //rank 1 exist
        {
            if(ch0_rank1_size == 0)
            {
                col_bit = ((emi_cona >> 6) & 0x03) + 9;
                row_bit = ((((emi_cona >> 25) & 0x01) << 2) + ((emi_cona >> 14) & 0x03)) + 13;
                per_chan_rank1_size = ((u64)(1 << (row_bit + col_bit))) * ((u64)(4 >> shift_for_16bit) * 8); // data width (bytes) * 8 banks
            }
            else
            {
                per_chan_rank1_size = (ch0_rank1_size * 256 << 20);
            }
        }

	if (nr_chan_enabled > 2) {
		// CH0 EMI have CHA+CHB
		dram_rank_size[0] = per_chan_rank0_size * 2;
		dram_rank_size[1] = per_chan_rank1_size * 2;
	} else {
		// CH0 EMI is CHA
		dram_rank_size[0] = per_chan_rank0_size;
		dram_rank_size[1] = per_chan_rank1_size;
	}
    }

    // CH1 EMI
    if(nr_chan_enabled >= 2)
    {
        if(ch1_rank0_size == 0)
        {
            //rank0 setting
            col_bit = ((emi_cona >> 20) & 0x03) + 9;
            row_bit = ((((emi_conh >> 4) & 0x01) << 2) + ((emi_cona >> 28) & 0x03)) + 13;
            per_chan_rank0_size = ((u64)(1 << (row_bit + col_bit))) * ((u64)(4 >> shift_for_16bit) * 8); // data width (bytes) * 8 banks
        }
        else
        {
            per_chan_rank0_size = (ch1_rank0_size * 256 << 20);
        }

        if (0 != (emi_cona &  (1 << 16)))   //rank 1 exist
        {
            if(ch1_rank1_size == 0)
            {
                col_bit = ((emi_cona >> 22) & 0x03) + 9;
                row_bit = ((((emi_conh >> 5) & 0x01) << 2) + ((emi_cona >> 30) & 0x03)) + 13;
                per_chan_rank1_size = ((u64)(1 << (row_bit + col_bit))) * ((u64)(4 >> shift_for_16bit) * 8); // data width (bytes) * 8 banks
            }
            else
            {
                per_chan_rank1_size = (ch1_rank1_size * 256 << 20);
            }
        }
	if (nr_chan_enabled > 2) {
		// CH1 EMI have CHC+CHD
		dram_rank_size[0] += per_chan_rank0_size * 2;
		dram_rank_size[1] += per_chan_rank1_size * 2;
	} else {
		// CH1 EMI is CHB
		dram_rank_size[0] += per_chan_rank0_size;
		dram_rank_size[1] += per_chan_rank1_size;
	}
    }

    //dramc_debug("DRAM rank0 size:0x%llx,\nDRAM rank1 size=0x%llx\n", dram_rank_size[0], dram_rank_size[1]);
}

const char *dram_size_to_str(unsigned long long size)
{
    char *buf;
    static char res[64];

    // first, clear the buffer
    memset(res, '\0', sizeof(res) / sizeof(res[0]));

    if (size >= 0x40000000) {
        buf = int_div_to_double(size, 0x40000000, 1);
        strncpy(res, buf, 32);

        strcat(res, "GB");
        return res;
    }
    else if (size >= 0x100000) {
        buf = int_div_to_double(size, 0x100000, 1);
        strncpy(res, buf, 32);

        strcat(res, "MB");
        return res;
    }
    else {
        buf = int_div_to_double(size, 0x400, 1);
        strncpy(res, buf, 32);

        strcat(res, "KB");
        return res;
    }
}

#if (FOR_DV_SIMULATION_USED==0)
#if !__FLASH_TOOL_DA__ && !__ETT__
/*
 * setup block
 */
#ifdef FIX_BUILD_FAIL
void get_orig_dram_rank_info(dram_info_t *orig_dram_info)
{
	int i, j;
	u64 base = DRAM_BASE;
	u64 rank_size[4];

	i = get_dram_rank_nr();

	orig_dram_info->rank_num = (i > 0) ? i : 0;
	get_dram_rank_size(rank_size);

	orig_dram_info->rank_info[0].start = base;
	for (i = 0; i < orig_dram_info->rank_num; i++) {

		orig_dram_info->rank_info[i].size = (u64)rank_size[i];

		if (i > 0) {
			orig_dram_info->rank_info[i].start =
				orig_dram_info->rank_info[i - 1].start +
				orig_dram_info->rank_info[i - 1].size;
		}
		dramc_debug("orig_dram_info[%d] start: 0x%llx, size: 0x%llx\n",
				i, orig_dram_info->rank_info[i].start,
				orig_dram_info->rank_info[i].size);
	}

	for(j=i; j<4; j++)
	{
	  		orig_dram_info->rank_info[j].start = 0;
	  		orig_dram_info->rank_info[j].size = 0;
	}
#ifdef CUSTOM_CONFIG_MAX_DRAM_SIZE
	for (i = 1; i < orig_dram_info->rank_num; i++) {
		if (orig_dram_info->rank_info[i].start >= (CUSTOM_CONFIG_MAX_DRAM_SIZE + DRAM_BASE)) {
			orig_dram_info->rank_num = i;
			dramc_crit("[dramc] reduce rank size = %u\n", (unsigned int) orig_dram_info->rank_num);
			break;
		}
	}
#endif
}
#endif

#ifdef DRAM_SIMULATION_SIZE  

unsigned long long get_dram_size(void)
{
	return DRAM_SIMULATION_SIZE;
}

#elif SUPPORT_TYPE_LPDDR4
/* return dram size: unit: Bytes */
unsigned long long get_dram_size(void)
{
#define DEFAULT_DRAM_SIZE   0x10000000ULL
    /* only one channel, one rank */
    unsigned long long size;

    size = DramInfo.u8MR8Density[CHANNEL_A][RANK_0];
    if (size == 0) {
        dprintf(ALWAYS, "MR8 density: 0, return default dram size:%llx\n", DEFAULT_DRAM_SIZE);
        size = DEFAULT_DRAM_SIZE;
    }
    else if (size % 0x800000 != 0)
        dprintf(ALWAYS, "MR8 density: %llx not aligned to 8MB, seems error!\n", size);

    return size;
}
#elif ENABLE_LP3_SW
unsigned long long get_dram_size(void)
{
	U64 rank_size = 0;

	if(0xffffeaf9 == *(volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)) {
		rank_size = 0x4000000;
	} else if (0xffffffff == *(volatile unsigned *)CHN_EMI_DUMMY_2(CHN0_EMI_BASE)){
		rank_size = 0x8000000;
	}

	// dramc_crit("emi get dram size: 0x%llx(%s)\n", rank_size, dram_size_to_str(rank_size));

	return rank_size;
}
#else
/* return dram size: unit: Bytes */
unsigned long long get_dram_size(void)
{
    /* only one channel, one rank */
	return 0x800000;
}
#endif

void get_dram_rank_size (u64 dram_rank_size[])
{
#ifdef COMBO_MCP
    int index, rank_nr, i;

  #ifdef DDR_RESERVE_MODE
    if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==1)
    {
        get_dram_rank_size_by_EMI_CONA(dram_rank_size);
    }
    else
  #endif
    {
        index = mt_get_mdl_number();

        if (index < 0 || index >= num_of_emi_records)
        {
            return;
        }

        rank_nr = get_dram_rank_nr();

        for(i = 0; i < rank_nr; i++){
            dram_rank_size[i] = emi_settings[index].DRAM_RANK_SIZE[i];

            dramc_debug("%d:dram_rank_size:%llx\n",i,dram_rank_size[i]);
        }
    }
    return;
#else
    get_dram_rank_size_by_EMI_CONA(dram_rank_size);
    return;
#endif
}

void assertion_failed(char *file, int line, char *expr)
{
    printf("<ASSERT> %s:line %d %s\n", file, line, expr);
	while(1);
}

#define ASSERT(expr) \
    do{ if(!(expr)){assertion_failed(__FILE__, __LINE__, #expr);} }while(0)

#if 0
void reserve_dramc_dummy_read(void)
{
	unsigned long long reserve_start = 0;
	char *reserve_name[4] = {"dramc-rk0", "dramc-rk1", "dramc-rk2", "dramc-rk3"};
	unsigned int i;
	int rank_nr;
	dram_addr_t dram_addr;

	dram_addr.ch = 0;

	rank_nr = get_dram_rank_nr();
	if (rank_nr <= 0) {
		dramc_crit("[DRAMC] reserve dummy read fail\n");
		ASSERT(0);
	}

	for (i = 0; i < (unsigned int)rank_nr; i++) {
		dram_addr.rk = i;
		get_dramc_addr(&dram_addr, 0x0);
#ifdef CUSTOM_CONFIG_MAX_DRAM_SIZE
		if (dram_addr.full_sys_addr > (unsigned long long)CUSTOM_CONFIG_MAX_DRAM_SIZE)
			break;
#endif

#if 0
		reserve_start = mblock_reserve_ext(&bootarg.mblock_info, 0x1000, 0x1000, dram_addr.full_sys_addr, 0, reserve_name[i]);	//for build pass
		if (reserve_start != (dram_addr.full_sys_addr - 0x1000)) {
			dramc_crit("[DRAMC] dummy read fail (0x%llx)\n", reserve_start);
			ASSERT(0);
		}
#endif
	}
}
#endif
#endif //#if !__FLASH_TOOL_DA__ && !__ETT__
#endif


#if (FOR_DV_SIMULATION_USED==0)
#if !__ETT__
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5

static int simple_mem_test(unsigned long start, unsigned int len)
{
	unsigned int *MEM32_BASE = (unsigned int *) start;
	unsigned int i, orig_val, new_val;

	for (i = 0; i < (len >> 2); ++i) {
		orig_val = MEM32_BASE[i];
		dsb();
		MEM32_BASE[i] = PATTERN1;
		dsb();
		new_val = MEM32_BASE[i];
		if (new_val != PATTERN1)
			return -1;
		dsb();
		MEM32_BASE[i] = orig_val;
	}

	return 0;
}

int complex_mem_test (unsigned long start, unsigned int len)
{
    unsigned char *MEM8_BASE = (unsigned char *) start;
    unsigned short *MEM16_BASE = (unsigned short *) start;
    unsigned int *MEM32_BASE = (unsigned int *) start;
    unsigned int *MEM_BASE = (unsigned int *) start;
    unsigned char pattern8;
    unsigned short pattern16;
    unsigned int i, j, size, pattern32;
    unsigned int value;

    size = len >> 2;

    /* === Verify the tied bits (tied high) === */
    for (i = 0; i < size; i++)
    {
        MEM32_BASE[i] = 0;
    }

    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0)
        {
            return -1;
        }
        else
        {
            MEM32_BASE[i] = 0xffffffff;
        }
    }

    /* === Verify the tied bits (tied low) === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -2;
        }
        else
            MEM32_BASE[i] = 0x00;
    }

    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
        MEM8_BASE[i] = pattern8++;
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
    {
        if (MEM8_BASE[i] != pattern8++)
        {
            return -3;
        }
    }

    /* === Verify pattern 2 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = j = 0; i < len; i += 2, j++)
    {
        if (MEM8_BASE[i] == pattern8)
            MEM16_BASE[j] = pattern8;
        if (MEM16_BASE[j] != pattern8)
        {
            return -4;
        }
        pattern8 += 2;
    }

    /* === Verify pattern 3 (0x00~0xffff) === */
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
        MEM16_BASE[i] = pattern16++;
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
    {
        if (MEM16_BASE[i] != pattern16++)
        {
            return -5;
        }
    }

    /* === Verify pattern 4 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
        MEM32_BASE[i] = pattern32++;
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
    {
        if (MEM32_BASE[i] != pattern32++)
        {
            return -6;
        }
    }

    /* === Pattern 5: Filling memory range with 0x44332211 === */
    for (i = 0; i < size; i++)
        MEM32_BASE[i] = 0x44332211;

    /* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x44332211)
        {
            return -7;
        }
        else
        {
            MEM32_BASE[i] = 0xa5a5a5a5;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a5a5)
        {
            return -8;
        }
        else
        {
            MEM8_BASE[i * 4] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a500)
        {
            return -9;
        }
        else
        {
            MEM8_BASE[i * 4 + 2] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa500a500)
        {
            return -10;
        }
        else
        {
            MEM8_BASE[i * 4 + 1] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5000000)
        {
            return -11;
        }
        else
        {
            MEM8_BASE[i * 4 + 3] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x00000000)
        {
            return -12;
        }
        else
        {
            MEM16_BASE[i * 2 + 1] = 0xffff;
        }
    }


    /* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffff0000)
        {
            return -13;
        }
        else
        {
            MEM16_BASE[i * 2] = 0xffff;
        }
    }


    /*===  Read Check === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -14;
        }
    }


    /************************************************
    * Additional verification
    ************************************************/
    /* === stage 1 => write 0 === */

    for (i = 0; i < size; i++)
    {
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 2 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];

        if (value != PATTERN1)
        {
            return -15;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 3 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -16;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 4 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -17;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 5 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -18;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 6 => read 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -19;
        }
    }


    /* === 1/2/4-byte combination test === */
    i = (unsigned int) MEM_BASE;

    while (i < (unsigned int) MEM_BASE + (size << 2))
    {
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
    }
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != 0x12345678)
        {
            return -20;
        }
    }


    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    MEM8_BASE[0] = pattern8;
    for (i = 0; i < size * 4; i++)
    {
        unsigned char waddr8, raddr8;
        waddr8 = i + 1;
        raddr8 = i;
        if (i < size * 4 - 1)
            MEM8_BASE[waddr8] = pattern8 + 1;
        if (MEM8_BASE[raddr8] != pattern8)
        {
            return -21;
        }
        pattern8++;
    }


    /* === Verify pattern 2 (0x00~0xffff) === */
    pattern16 = 0x00;
    MEM16_BASE[0] = pattern16;
    for (i = 0; i < size * 2; i++)
    {
        if (i < size * 2 - 1)
            MEM16_BASE[i + 1] = pattern16 + 1;
        if (MEM16_BASE[i] != pattern16)
        {
            return -22;
        }
        pattern16++;
    }


    /* === Verify pattern 3 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    MEM32_BASE[0] = pattern32;
    for (i = 0; i < size; i++)
    {
        if (i < size - 1)
            MEM32_BASE[i + 1] = pattern32 + 1;
        if (MEM32_BASE[i] != pattern32)
        {
            return -23;
        }
        pattern32++;
    }

    return 0;
}

static int dram_check_reserve_mode(void)
{
#ifdef DDR_RESERVE_MODE
	unsigned int emi_cona;

	mcSHOW_DBG_MSG("g_ddr_reserve_enable:%d, g_ddr_reserve_success:%d\n", g_ddr_reserve_enable, g_ddr_reserve_success);
	if((g_ddr_reserve_enable==1) && (g_ddr_reserve_success==1)) {
		/* EMI register dummy read: give clock to EMI APB register to avoid DRAM access hang */
		emi_cona = *(volatile unsigned int *)(EMI_CONA);
		mcSHOW_DBG_MSG("[DDR Reserve mode] EMI dummy read CONA = 0x%x\n", emi_cona);

#if SUPPORT_TYPE_PSRAM
		/* PEMI register dummy read: give clock to PEMI APB register to avoid DRAM access hang */
		mcSHOW_DBG_MSG("[DDR Reserve mode](PEMI_CONA)=%x\n", *((volatile unsigned *)((IO_PHYS + 0x0023F000) + 0x00)));
#endif

		/* disable transaction mask */
		*(volatile unsigned int *) (CHN0_EMI_BASE + 0x3FC) &= 0xFFFFFFFE;
	} else /* normal boot */
#endif
	{
		/* force clear RGU control for DRAMC before calibration */
		rgu_release_rg_dramc_conf_iso();//Release DRAMC/PHY conf ISO
		rgu_release_rg_dramc_iso();//Release PHY IO ISO
		rgu_release_rg_dramc_sref();//Let DRAM Leave SR

		return 0;
	}

#ifdef DDR_RESERVE_MODE
	unsigned int rank1_start_address, i;

	if((g_ddr_reserve_enable==1) && (g_ddr_reserve_success==1)) {
#if (SUPPORT_TYPE_PSRAM != 1)
		i = simple_mem_test(DRAM_BASE_VIRT, 0x2000);
		if (i == 0) {
			mcSHOW_DBG_MSG("simple R/W mem test pass (start addr:0x%lx, @Rank0)\n", (unsigned long)DRAM_BASE_VIRT);
		} else {
			mcSHOW_ERR_MSG("simple R/W mem test fail :%x (start addr:0x%lx, @Rank0)\n", i, (unsigned long)DRAM_BASE_VIRT);
		}
#endif
	}
#endif

	return 1;

}

void mt_mem_init(void)
{
    int index;
    /*unsigned int SW_CTRL_VC, HW_CTRL_VC;*/
    EMI_SETTINGS *emi_set;

	if (dram_check_reserve_mode() == 1) {
	#ifdef DDR_RESERVE_MODE
		rgu_dram_reserved(1);
	#endif
		return;
	}

#ifdef DDR_RESERVE_MODE
	rgu_dram_reserved(1);
#endif

	// set voltage and hw trapping before mdl
    setup_dramc_voltage_by_pmic();

#ifdef VCORE_BIN
    dvfsrc_opp_level_mapping();
#endif

#ifdef COMBO_MCP
    index = mt_get_mdl_number();
    dramc_crit("[EMI] MDL number = %d\r\n", index);
    if (index < 0 || index >=  num_of_emi_records)
    {
        dramc_crit("[EMI] setting failed 0x%x\r\n", index);
        ASSERT(0);
    }
    else
    {
        emi_setting_index = index;
        emi_set = &emi_settings[emi_setting_index];
    }
#else
	dramc_crit("[EMI] ComboMCP not ready, using default setting\n");
	emi_setting_index = -1;
	emi_set = &default_emi_setting;
#endif

#ifdef DDR_RESERVE_MODE
    if(g_ddr_reserve_enable==1 && g_ddr_reserve_success==0)
        Before_Init_DRAM_While_Reserve_Mode_fail(emi_set->type & 0xF);
#endif

    Init_DRAM((emi_set->type & 0xF), emi_set->dram_cbt_mode_extern, &DramInfo, NORMAL_USED);
    switch_dramc_voltage_to_auto_mode();
    //restore_vcore_setting();

#if 0
	{
		DRAMC_CTX_T * p = psCurrDramCtx;
		DramcRegDump(p);
	}
#endif
}
#endif
#endif

#define DRAMC_ADDR_SHIFT_CHN(addr, channel) (addr + (channel * 0x10000))

void phy_addr_to_dram_addr(dram_addr_t *dram_addr, unsigned long long phy_addr)
{
	unsigned int emi_cona, emi_conf;
	unsigned long long rank_size[4];
	unsigned int channel_num, rank_num;
	unsigned int bit_scramble, bit_xor, bit_shift, channel_pos, channel_width;
	unsigned int temp;
	unsigned int index;
	unsigned int ddr_type = mt_get_dram_type_from_hw_trap();

	emi_cona = *((volatile unsigned int *)EMI_CONA);
	emi_conf = *((volatile unsigned int *)EMI_CONF) >> 8;
	get_dram_rank_size_by_EMI_CONA(rank_size);
	rank_num = (unsigned int) get_dram_rank_nr();
	channel_num = (unsigned int) get_dram_channel_nr();

	if (rank_num >= sizeof(rank_size) / sizeof(*rank_size)) {
		dramc_crit("[Dramc] Wrong rank_num: %u\n", rank_num);
		ASSERT(0);
	}

	phy_addr -= 0x40000000;
	for (index = 0; index < rank_num; index++) {
		if (phy_addr >= rank_size[index])
			phy_addr -= rank_size[index];
		else
			break;
	}

	for (bit_scramble = 11; bit_scramble < 17; bit_scramble++) {
		bit_xor = (emi_conf >> (4 * (bit_scramble - 11))) & 0xf;
		bit_xor &= phy_addr >> 16;
		for (bit_shift = 0; bit_shift < 4; bit_shift++)
			phy_addr ^= ((bit_xor>>bit_shift)&0x1) << bit_scramble;
	}

	if (channel_num > 1) {
		channel_pos = ((emi_cona >> 2) & 0x3) + 7;

		for (channel_width = bit_shift = 0; bit_shift < 4; bit_shift++) {
			if ((unsigned int)(1 << bit_shift) >= channel_num)
				break;
			channel_width++;
		}

		switch (channel_width) {
			case 2:
				dram_addr->addr = ((phy_addr & ~(((0x1 << 2) << channel_pos) - 1)) >> 2);
				break;
			default:
				dram_addr->addr = ((phy_addr & ~(((0x1 << 1) << channel_pos) - 1)) >> 1);
				break;
		}
		dram_addr->addr |= (phy_addr & ((0x1 << channel_pos) - 1));
	} else {
		dram_addr->addr = phy_addr;
	}

	if (u1IsLP4Family(ddr_type))
		dram_addr->addr >>= 1;
	else
		dram_addr->addr >>= 2;

	temp = dram_addr->addr;
	switch ((emi_cona >> 4) & 0x3) {
		case 0:
			dram_addr->col = temp & 0x1FF;
			temp = temp >> 9;
			break;
		case 1:
			dram_addr->col = temp & 0x3FF;
			temp = temp >> 10;
			break;
		case 2:
		default:
			dram_addr->col = temp & 0x7FF;
			temp = temp >> 11;
			break;
	}
	dram_addr->bk = temp & 0x7;
	temp = temp >> 3;

	dram_addr->row = temp;

	//mcSHOW_DBG_MSG("ch%d, rk%d, dram addr: %x\n", dram_addr->ch, dram_addr->rk, dram_addr->addr);
	//mcSHOW_DBG_MSG("bk%x, row%x, col%x\n", dram_addr->bk, dram_addr->row, dram_addr->col);
}

void put_dummy_read_pattern(unsigned long long dst_pa, unsigned int src_pa, unsigned int len)
{
	*((volatile unsigned int *)(CQ_DMA_BASE + 0x018)) = 7 << 16;

	*((volatile unsigned int *)(CQ_DMA_BASE + 0x01c)) = src_pa;
	*((volatile unsigned int *)(CQ_DMA_BASE + 0x060)) = 0;

	*((volatile unsigned int *)(CQ_DMA_BASE + 0x020)) = dst_pa & 0xffffffff;
	*((volatile unsigned int *)(CQ_DMA_BASE + 0x064)) = dst_pa >> 32;

	*((volatile unsigned int *)(CQ_DMA_BASE + 0x024)) = len;
	dsb();
	*((volatile unsigned int *)(CQ_DMA_BASE + 0x008)) = 0x1;

	while(*((volatile unsigned int *)(CQ_DMA_BASE + 0x008)));

	*((volatile unsigned int *)(CQ_DMA_BASE + 0x064)) = 0;
}

static unsigned int get_dramc_addr(dram_addr_t *dram_addr, unsigned int offset)
{
	unsigned int channel_num, rank_num;
	unsigned long long dummy_read_addr;
	unsigned long long rank_size[4];
	unsigned int index;
	unsigned long long *src_vir_addr;
	unsigned int *src_pa_addr;

	channel_num = (unsigned int) get_dram_channel_nr();
	rank_num = (unsigned int) get_dram_rank_nr();
	get_dram_rank_size_by_EMI_CONA(rank_size);
	dummy_read_addr = DRAM_BASE_VIRT;
	src_vir_addr = (unsigned long long*) DRAM_BASE_VIRT;
	src_pa_addr = (unsigned int *) 0x40000000;

	if (dram_addr->ch >= channel_num) {
		mcSHOW_DBG_MSG("[DRAMC] invalid channel: %d\n", dram_addr->ch);
		return 0;
	}

	if (dram_addr->rk >= rank_num) {
		mcSHOW_DBG_MSG("[DRAMC] invalid rank: %d\n", dram_addr->rk);
		return 0;
	}

	for (index = 0; index <= dram_addr->rk; index++)
		dummy_read_addr += rank_size[index];
	dummy_read_addr -= offset;
	if (dram_addr->ch == 0)
		dummy_read_addr &= ~(0x100);

	if (offset == 0x20) {
		for (index = 0; index < 4; index++)
			*(src_vir_addr + index) = 0xAAAA5555;
		put_dummy_read_pattern(dummy_read_addr, (unsigned int) src_pa_addr, 16);
	}

	dram_addr->full_sys_addr = dummy_read_addr;
	phy_addr_to_dram_addr(dram_addr, dummy_read_addr);

	return dram_addr->addr;
}

unsigned int get_dummy_read_addr(dram_addr_t *dram_addr)
{
	return get_dramc_addr(dram_addr, 0x20); // 32-byte align for dummy RW pattern
}

unsigned int get_ta2_addr(dram_addr_t *dram_addr)
{
	unsigned int addr = get_dramc_addr(dram_addr, 0x1000);

	if (!u1IsLP4Family(mt_get_dram_type_from_hw_trap()))
		addr <<= 2;

	return addr & 0xFFFFFFF0;
}

void init_ta2_single_channel(unsigned int channel)
{
	unsigned int temp;
	unsigned int matype[2];
	unsigned int col_shf[2] = {0, 0};
	dram_addr_t dram_addr;
	DRAMC_CTX_T *p = psCurrDramCtx;

	// mt6771: CHN0_EMI for CHN-A;  CHN1_EMI for CHN-B
	if(channel < 2) {
		matype[0] = (*(volatile unsigned *)EMI_CONA >> 4) & 0x3;
		matype[1] = (*(volatile unsigned *)EMI_CONA >> 6) & 0x3;
	} else {
		matype[0] = (*(volatile unsigned *)EMI_CONA >> 20) & 0x3;
		matype[1] = (*(volatile unsigned *)EMI_CONA >> 22) & 0x3;
	}

	if (matype[0] < matype[1]) {
		col_shf[0] = matype[1] - matype[0];
		matype[0] = matype[1];
	} else if (matype[1] < matype[0])
		col_shf[1] = matype[0] - matype[1];

	matype[0] = (matype[0] + 1) << 30;

	// disable self test engine1 and self test engine2
	temp = u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_3, channel)) & 0x1FFFFFFF;
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_3, channel), temp);

	// set MATYPE
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU_CONF0, channel)) & 0x7FFFFFFF) | matype[0];
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU_CONF0, channel), temp);
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU2_CONF0, channel)) & 0x7FFFFFFF) | matype[0];
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU2_CONF0, channel), temp);
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU3_CONF0, channel)) & 0x7FFFFFFF) | matype[0];
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_SHU3_CONF0, channel), temp);

	// set rank address for test agent to auto
	temp = u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_4, channel)) & 0x8FFFFFFF;
	temp |= (0x4 << 28);
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_4, channel), temp);

	// set test for both rank0 and rank1
	temp = u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_3, channel)) & 0xFFFFFFF0;
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_3, channel), temp | 0x1);

	// set base address for test agent to reserved space
	dram_addr.ch = channel;
	dram_addr.rk = 0;
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_1, channel)) & 0x0000000F);
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_1, channel), temp | (get_ta2_addr(&dram_addr) << col_shf[0]));
	dram_addr.rk = 1;
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_5, channel)) & 0x0000000F);
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_5, channel), temp | (get_ta2_addr(&dram_addr) << col_shf[1]));

	// set test length (offset) to 0x20
	temp = (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_2, channel)) & 0x0000000F) | (0x20 << 4);
	vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_TEST2_2, channel), temp);

	return;
}

#ifdef LAST_DRAMC

static unsigned int is_last_dramc_initialized(void)
{
	if(last_dramc_info_ptr->ta2_result_magic != LAST_DRAMC_MAGIC_PATTERN) {
		return 0;
	} else {
		return 1;
	}
}

void update_last_dramc_info(void)
{
	unsigned int chn;
	unsigned int latch_result = 0;
	unsigned int temp;
	unsigned int *curr;
	DRAMC_CTX_T *p = psCurrDramCtx;

	// init checksum and magic pattern
	if(last_dramc_info_ptr->ta2_result_magic != LAST_DRAMC_MAGIC_PATTERN) {
		last_dramc_info_ptr->ta2_result_magic = LAST_DRAMC_MAGIC_PATTERN;
		last_dramc_info_ptr->ta2_result_last = 0;
		last_dramc_info_ptr->ta2_result_past = 0;
		last_dramc_info_ptr->ta2_result_checksum = LAST_DRAMC_MAGIC_PATTERN;
		last_dramc_info_ptr->reboot_count = 0;
	} else {
		last_dramc_info_ptr->ta2_result_checksum ^= last_dramc_info_ptr->reboot_count;
		last_dramc_info_ptr->reboot_count++;
		last_dramc_info_ptr->ta2_result_checksum ^= last_dramc_info_ptr->reboot_count;
	}

	// TODO: check DCS status

	// read data from latch register and reset
	for (chn = 0; chn < CHANNEL_NUM; ++chn) {
		//dramc_crit("[LastDRAMC] latch result before RST: %x\n", u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_WDT_DBG_SIGNAL, chn)));
		latch_result = (latch_result << 16) | u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_WDT_DBG_SIGNAL, chn)) & 0xFFFF;
		temp = u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_DDRCONF0, chn));
		vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_DDRCONF0, chn), temp | 0x00000004);
		vIO32Write4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_DDRCONF0, chn), temp & 0xFFFFFFFB);
		//dramc_crit("[LastDRAMC] latch result after RST: %x\n", u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_WDT_DBG_SIGNAL, chn)));
	}

	last_dramc_info_ptr->ta2_result_checksum ^= last_dramc_info_ptr->ta2_result_past ^ latch_result;
	last_dramc_info_ptr->ta2_result_past = last_dramc_info_ptr->ta2_result_last;
	last_dramc_info_ptr->ta2_result_last = latch_result;
	for (temp = 0; temp < sizeof(LAST_DRAMC_INFO_T) / sizeof(temp); temp++) {
		curr = (unsigned int *)last_dramc_info_ptr + temp;
		dramc_crit("[LastDRAMC] 0x%x: 0x%x\n", curr, *curr);
	}

	return;
}

void update_last_dramc_k_voltage(DRAMC_CTX_T *p, unsigned int voltage)
{
	unsigned int shu_type;

	shu_type = get_shuffleIndex_by_Freq(p);

	last_dramc_info_ptr->k_voltage[shu_type] = voltage;
}

void init_ta2_all_channel(void)
{
	unsigned int chn;

	update_last_dramc_info();

	// TODO: consider DCS
	for (chn = 0; chn < CHANNEL_NUM; ++chn)
		init_ta2_single_channel(chn);
}


unsigned int check_gating_err_in_dramc_latch(void)
{
	unsigned int chn, ret = 0;
	DRAMC_CTX_T *p = psCurrDramCtx;
#ifdef FIX_BUILD_FAIL
	if ((g_boot_reason == BR_POWER_KEY) || (g_boot_reason == BR_USB)
			|| mtk_wdt_is_pmic_full_reset() || (is_last_dramc_initialized() == 0)){
		dramc_crit("for cold boot, always return 0\n");
		return 0;
	}
#endif
	for (chn = 0; chn <= 3; ++chn) {
		if (u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_WDT_DBG_SIGNAL, chn)) & 0x80) {
			dramc_crit("[dramc] found gating error in channel %d (0x%x)\n",
					chn, u4IO32Read4B(DRAMC_ADDR_SHIFT_CHN(DRAMC_REG_WDT_DBG_SIGNAL, chn)));
			ret |= (1 << chn);
		}
	}

	return ret;
}

void dram_fatal_exception_detection_start(void)
{
#ifdef FIX_BUILD_FAIL
	last_dramc_info_ptr = (LAST_DRAMC_INFO_T *) get_dbg_info_base(KEY_LAST_DRAMC);
#endif
#if 0 // SUPPORT_SAVE_TIME_FOR_CALIBRATION
	part_dram_data_addr = get_part_addr("boot_para"); // addr = 0x8000
	if (part_dram_data_addr != 0x0)
		dramc_crit("[dramc] init partition address is 0x%llx\n", part_dram_data_addr);
	else {
		dramc_crit("[dramc] init partition address is incorrect !!!\n");
	}

    part_dram_data_addr = 0x1;  // TODO: it's used in preloader, lk don't use the api, just set 0x1 now
#endif

	if (
#ifdef FIX_BUILD_FAIL
		(g_boot_reason == BR_POWER_KEY) || (g_boot_reason == BR_USB)
			|| mtk_wdt_is_pmic_full_reset() ||
#endif
		(is_last_dramc_initialized() == 0)){
		/* cold boot: initialize last_dram_fatal_err_flag and dram_fatal_err_flag */
		dramc_crit("[dramc] init SRAM region for DRAM exception detection\n");
		last_dramc_info_ptr->last_fatal_err_flag = 0x0;
		last_dramc_info_ptr->storage_api_err_flag = 0x0;
		dram_fatal_init_stberr();
	} else {
		last_dramc_info_ptr->last_fatal_err_flag = last_dramc_info_ptr->fatal_err_flag;
		last_dramc_info_ptr->storage_api_err_flag = 0x0;
		dram_fatal_backup_stberr();
		dram_fatal_init_stberr();
	}

	last_dramc_info_ptr->fatal_err_flag = 1 << OFFSET_DRAM_FATAL_ERR;
	dsb();
}

void dram_fatal_exception_detection_end(void)
{
	last_dramc_info_ptr->fatal_err_flag = 0x0;
	dsb();
}

unsigned int check_dram_fatal_exception(void)
{
	dramc_crit("[dramc] DRAM_FATAL_ERR_FLAG = 0x%x\n", last_dramc_info_ptr->fatal_err_flag);

	return ((last_dramc_info_ptr->fatal_err_flag & ~((1 << OFFSET_DRAM_FATAL_ERR)|DDR_RSV_MODE_ERR_MASK)) != 0x0) ? 1 : 0;
}

unsigned int check_last_dram_fatal_exception(void)
{
	dramc_crit("[dramc] LAST_DRAM_FATAL_ERR_FLAG = 0x%x\n", last_dramc_info_ptr->last_fatal_err_flag);

	return ((last_dramc_info_ptr->last_fatal_err_flag & ~(DDR_RSV_MODE_ERR_MASK)) != 0x0) ? 1 : 0;
}

void dram_fatal_set_ta2_err(unsigned int chn, unsigned int err_code)
{
	unsigned int shift = OFFSET_DRAM_TA2_ERR + 2 * chn, ret;

	if (chn > 3)
		return;

	ret = last_dramc_info_ptr->fatal_err_flag & ~(0x7 << shift);
	last_dramc_info_ptr->fatal_err_flag = ret | ((err_code & 0x7) << shift);
	dsb();
}

void dram_fatal_set_gating_err(unsigned int chn, unsigned int err_code)
{
	unsigned int shift = OFFSET_DRAM_GATING_ERR + 4 * chn, ret;

	if (chn > 3)
		return;

	ret = last_dramc_info_ptr->fatal_err_flag & ~(0xf << shift);
	last_dramc_info_ptr->fatal_err_flag = ret | ((err_code & 0xf) << shift);
	dsb();
}

void dram_fatal_init_stberr(void)
{
	last_dramc_info_ptr->gating_err[0][0] = 0x0;
	last_dramc_info_ptr->gating_err[0][1] = 0x0;
	last_dramc_info_ptr->gating_err[1][0] = 0x0;
	last_dramc_info_ptr->gating_err[1][1] = 0x0;

	dsb();
}

void dram_fatal_backup_stberr(void)
{
	last_dramc_info_ptr->last_gating_err[0][0] = last_dramc_info_ptr->gating_err[0][0];
	last_dramc_info_ptr->last_gating_err[0][1] = last_dramc_info_ptr->gating_err[0][1];
	last_dramc_info_ptr->last_gating_err[1][0] = last_dramc_info_ptr->gating_err[1][0];
	last_dramc_info_ptr->last_gating_err[1][1] = last_dramc_info_ptr->gating_err[1][1];

	dsb();
}

void dram_fatal_set_stberr(unsigned int chn, unsigned int rk, unsigned int err_code)
{
	if ((chn > 1) || (rk > 1))
		return;

	last_dramc_info_ptr->gating_err[chn][rk] = err_code;

	dsb();
}

void dram_fatal_set_err(unsigned int err_code, unsigned int mask, unsigned int offset)
{
	unsigned int ret;

	ret = last_dramc_info_ptr->fatal_err_flag & ~(mask << offset);
	last_dramc_info_ptr->fatal_err_flag = ret | ((err_code & mask) << offset);
	dsb();
}

#endif

#if SUPPORT_SAVE_TIME_FOR_CALIBRATION

#if !__ETT__
#include <platform/msdc.h>
#include <platform/mmc_core.h>
#include <platform/mmc_ioctl.h>
#include <lib/bio.h>
#include <lib/heap.h>
#include <lib/partition.h>
#else
#include "ett_common.h"
#include "emi.h"
#endif

u32 g_dram_storage_api_err_code;

static u16 crc16(const u8* data, u32 length){
	u8 x;
	u16 crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((u8)(x << 12)) ^ ((u8)(x <<5)) ^ ((u8)x);
	}
	return crc;
}

static void assign_checksum_for_dram_data(DRAM_CALIBRATION_SHU_DATA_T *shu_data)
{
	/* need to initialize checksum to 0 before calculation */
	shu_data->checksum = 0;
	shu_data->checksum = crc16((u8*)shu_data, sizeof(*shu_data));
}

static int check_checksum_for_dram_data(DRAM_CALIBRATION_SHU_DATA_T *shu_data)
{
	u16 checksum_in_storage = shu_data->checksum;

	assign_checksum_for_dram_data(shu_data);

	return (shu_data->checksum == checksum_in_storage) ? 1 : 0;
}

#ifdef COMBO_MCP
static void assign_checksum_for_mdl_data(DRAM_CALIBRATION_MRR_DATA_T *mrr_info)
{
	/* need to initialize checksum to 0 before calculation */
	mrr_info->checksum = 0;
	mrr_info->checksum = crc16((u8*)mrr_info, sizeof(*mrr_info));
}

static int check_checksum_for_mdl_data(DRAM_CALIBRATION_MRR_DATA_T *mrr_info)
{
	u16 checksum_in_storage = mrr_info->checksum;

	assign_checksum_for_mdl_data(mrr_info);

	return (mrr_info->checksum == checksum_in_storage) ? 1 : 0;
}
#endif

#if !__ETT__
#ifdef COMBO_MCP
static int read_offline_dram_mdl_data(DRAM_INFO_BY_MRR_T *DramInfo)
{
	int i, ret;
	u16 emi_checksum;
	blkdev_t *bootdev = NULL;
	DRAM_CALIBRATION_HEADER_T hdr;
	DRAM_CALIBRATION_MRR_DATA_T mrr_info;
	DRAM_CALIBRATION_SHU_DATA_T shu_data;
	DRAM_CALIBRATION_DATA_T *datap = NULL;

	if (dram_offline_data_flags)
		goto exit;

	if (DramInfo == NULL) {
		dramc_crit("[dramc] DramInfo == NULL, skip\n");
		dram_offline_data_flags = ERR_NULL_POINTER;
		goto exit;
	}

	bootdev = blkdev_get(CFG_BOOT_DEV);
	if (bootdev == NULL) {
		dramc_crit("[dramc] can't find boot device(%d)\n", CFG_BOOT_DEV);
		dram_offline_data_flags = ERR_BLKDEV_NOT_FOUND;
		goto exit;
	}

	if (!part_dram_data_addr) {
		dram_offline_data_flags = ERR_BLKDEV_NO_PART;
		goto exit;
	}

	ret = blkdev_read(bootdev, part_dram_data_addr, sizeof(hdr), (u8*)&hdr, PART_ID_DRAM_DATA);
	if (ret != 0) {
		dramc_crit("[dramc] blkdev_read %s failed\n", "hdr");
		dram_offline_data_flags = ERR_BLKDEV_READ_FAIL;
		goto exit;
	}

	if (hdr.pl_version != FAST_K_LK_VERSION) {
		/* current preloader version does not match the calibration hdr in storage -> erase the partition */
		dramc_crit("[dramc] FAST_K_LK_VERSION is updated, erase the DRAM shu_data\n");

		shu_data.checksum = 0;

		/* clear each shuffle */
		for (i = 0; i < DRAM_DFS_SHUFFLE_MAX; i++) {
			ret = blkdev_write(bootdev, part_dram_data_addr + ((unsigned long) &datap->data[i]), sizeof(shu_data), (u8*)&shu_data, PART_ID_DRAM_DATA);
			if (ret != 0) {
				dramc_crit("[dramc] blkdev_write failed\n");
				dram_offline_data_flags = ERR_BLKDEV_WRITE_FAIL;
				goto exit;
			}
		}

		dram_offline_data_flags = ERR_PL_UPDATED;
		goto exit;
	}

	/* check magic number */
	if (hdr.magic_number != DRAM_CALIBRATION_DATA_MAGIC) {
		dramc_crit("[dramc] magic number mismatch\n");
		dram_offline_data_flags = ERR_MAGIC_NUMBER;
		goto exit;
	}

	ret = blkdev_read(bootdev, part_dram_data_addr + ((unsigned long) &datap->mrr_info), sizeof(mrr_info), (u8*)&mrr_info, PART_ID_DRAM_DATA);
	if (ret != 0) {
		dramc_crit("[dramc] blkdev_read %s failed\n", "data");
		dram_offline_data_flags = ERR_BLKDEV_READ_FAIL;
		goto exit;
	}

	/* check checksum */
	if (check_checksum_for_mdl_data(&mrr_info) != 1) {
		dramc_crit("[dramc] checksum failed\n");
		dram_offline_data_flags = ERR_CHECKSUM;

		goto exit;
	}

	emi_checksum = crc16((u8*)emi_settings, sizeof(emi_settings));

	if (emi_checksum != mrr_info.emi_checksum) {
		dramc_crit("[dramc] emi checksum failed\n");
		dram_offline_data_flags = ERR_CHECKSUM;

		goto exit;
	}

	/* copy the data stored in storage to the data structure for calibration */
	memcpy(DramInfo, &(mrr_info.DramInfo), sizeof(*DramInfo));

exit:
	if (dram_offline_data_flags)
		SET_DRAM_STORAGE_API_ERR(dram_offline_data_flags, DRAM_STORAGE_API_READ);

	return 0 - dram_offline_data_flags;
}

static int write_offline_dram_mdl_data(DRAM_INFO_BY_MRR_T *DramInfo)
{
	int ret;
	blkdev_t *bootdev = NULL;
	DRAM_CALIBRATION_HEADER_T hdr;
	DRAM_CALIBRATION_MRR_DATA_T mrr_info;
	DRAM_CALIBRATION_DATA_T *datap = NULL;

	if (DramInfo == NULL) {
		dramc_crit("[dramc] DramInfo == NULL, skip\n");
		SET_DRAM_STORAGE_API_ERR(ERR_NULL_POINTER, DRAM_STORAGE_API_WRITE);
		return -ERR_NULL_POINTER;
	}

	bootdev = blkdev_get(CFG_BOOT_DEV);
	if (bootdev == NULL) {
		dramc_crit("[dramc] can't find boot device(%d)\n", CFG_BOOT_DEV);
		SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_NOT_FOUND, DRAM_STORAGE_API_WRITE);
		return -ERR_BLKDEV_NOT_FOUND;
	}

	if (!part_dram_data_addr) {
		return -ERR_BLKDEV_NO_PART;
	}

	memcpy(&(mrr_info.DramInfo), DramInfo, sizeof(*DramInfo));

#if 0
    /* assign PL version */
    hdr.pl_version = FAST_K_LK_VERSION;

    /* assign magic number */
    hdr.magic_number = DRAM_CALIBRATION_DATA_MAGIC;

    /* assign api error code */
    hdr.calib_err_code = g_dram_storage_api_err_code;

    ret = blkdev_write(bootdev, part_dram_data_addr, sizeof(hdr), (u8*)&hdr, PART_ID_DRAM_DATA);
    if (ret != 0) {
        dramc_crit("[dramc] blkdev_write failed\n");
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_WRITE_FAIL, DRAM_STORAGE_API_WRITE);
        return -ERR_BLKDEV_WRITE_FAIL;
    }
#endif

    /* calculate and assign checksum */
	mrr_info.emi_checksum = crc16((u8*)emi_settings, sizeof(emi_settings));
    assign_checksum_for_mdl_data(&mrr_info);

    ret = blkdev_write(bootdev, part_dram_data_addr + ((unsigned long) &datap->mrr_info), sizeof(mrr_info), (u8*)&mrr_info, PART_ID_DRAM_DATA);
    if (ret != 0) {
        dramc_crit("[dramc] blkdev_write failed\n");
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_WRITE_FAIL, DRAM_STORAGE_API_WRITE);
        return -ERR_BLKDEV_WRITE_FAIL;
    }

    return 0;
}
#endif

#if EMMC_READY

void dump_dram_cali_header(DRAM_CALIBRATION_HEADER_T *header)
{
    mcSHOW_INFO_MSG("%s header: version:0x%x, magic:0x%x, cali_code:0x%d\n",
        FAST_K, header->pl_version, header->magic_number, header->calib_err_code);   
}

void dump_dram_cali_delay_cell(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s delay cell:%d, delay_cellx100:%d\n", FAST_K, prefix, cali->ucnum_dlycell_perT, cali->u2DelayCellTimex100);

}

void dump_dram_cali_clk_dqs_duty(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    // CLK/DQS Duty
    mcSHOW_INFO_MSG("%s %s CLK Duty:%d, DQS0 Duty:%d, DQS1 Duty:%d, CLK reverse bit:%d DQS reverse bit:%d\n", 
        FAST_K, prefix,
        cali->s1ClockDuty_clk_delay_cell[CHANNEL_A][RANK_0],
        cali->s1DQSDuty_clk_delay_cell[CHANNEL_A][0], cali->s1DQSDuty_clk_delay_cell[CHANNEL_A][1],
        cali->u1clk_use_rev_bit, cali->u1dqs_use_rev_bit);
}

void dump_dram_cali_cbt(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s CBT: CLK_dly:%d, CMD_dly:%d, CS_dly:%d, Vref:%d, CA delay cell:%d %d %d %d %d %d\n",
        FAST_K, prefix,
        cali->u1CBTClkDelay_Save[CHANNEL_A][RANK_0],
        cali->u1CBTCmdDelay_Save[CHANNEL_A][RANK_0],
        cali->u1CBTCsDelay_Save[CHANNEL_A][RANK_0],
        cali->u1CBTVref_Save[CHANNEL_A][RANK_0],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][0],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][1],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][2],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][3],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][4],
        cali->u1CBTCA_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][5]);
}

void dump_dram_cali_write_leveling(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s WL: DQS0 dly:%d, DQS1 dly:%d\n",
        FAST_K, prefix,
        cali->u1WriteLeveling_bypass_Save[CHANNEL_A][RANK_0][0],
        cali->u1WriteLeveling_bypass_Save[CHANNEL_A][RANK_0][1]);
}

void dump_dram_cali_gating(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s RX Gating: DQS0 Large UI dly:%d, DQS0 UI dly:%d, delay cell:%d, bypass_cnt:%d\n",
        FAST_K, prefix,
        cali->u1Gating2T_Save[CHANNEL_A][RANK_0][0],
        cali->u1Gating05T_Save[CHANNEL_A][RANK_0][0],
        cali->u1Gatingfine_tune_Save[CHANNEL_A][RANK_0][0],
        cali->u1Gatingucpass_count_Save[CHANNEL_A][RANK_0][0]);
    mcSHOW_INFO_MSG("%s %s RX Gating: DQS1 Large UI dly:%d, DQS1 UI dly:%d, delay cell:%d, bypass_cnt:%d\n",
        FAST_K, prefix,
        cali->u1Gating2T_Save[CHANNEL_A][RANK_0][1],
        cali->u1Gating05T_Save[CHANNEL_A][RANK_0][1],
        cali->u1Gatingfine_tune_Save[CHANNEL_A][RANK_0][1],
        cali->u1Gatingucpass_count_Save[CHANNEL_A][RANK_0][1]);
}

void dump_dram_cali_tx(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    int i;
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s TX: Vref:%d, Center Min:DQS0:%d, DQS1:%d, Center Max:DQS0:%d, DQS1:%d\n",
        FAST_K, prefix,
        cali->u1TxWindowPerbitVref_Save[CHANNEL_A][RANK_0],
        cali->u1TxCenter_min_Save[CHANNEL_A][RANK_0][0],
        cali->u1TxCenter_min_Save[CHANNEL_A][RANK_0][1],
        cali->u1TxCenter_max_Save[CHANNEL_A][RANK_0][0],
        cali->u1TxCenter_max_Save[CHANNEL_A][RANK_0][1]);
    
    mcSHOW_INFO_MSG("%s %s TX Win Center: ", FAST_K, prefix);
    for(i = 0; i < DQ_DATA_WIDTH_LP4; ++i)
        mcSHOW_INFO_MSG("%d ", cali->u1Txwin_center_Save[CHANNEL_A][RANK_0][i]);
    mcSHOW_INFO_MSG("\n");
    
    mcSHOW_INFO_MSG("%s %s TX per bit delay cell: ", FAST_K, prefix);
    for(i = 0; i < DQ_DATA_WIDTH_LP4; ++i)
        mcSHOW_INFO_MSG("%d ", cali->u1TX_PerBit_DelayLine_Save[CHANNEL_A][RANK_0][i]);
    mcSHOW_INFO_MSG("\n");
}

void dump_dram_cali_rx(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    int i;
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s RX: Vref:%d, DQS0:%d, DQS1:%d, DQM0:%d, DQM1:%d\n",
        FAST_K, prefix,
        cali->u1RxWinPerbitVref_Save[CHANNEL_A],
        cali->u1RxWinPerbit_DQS[CHANNEL_A][RANK_0][0],
        cali->u1RxWinPerbit_DQS[CHANNEL_A][RANK_0][1],
        cali->u1RxWinPerbit_DQM[CHANNEL_A][RANK_0][0],
        cali->u1RxWinPerbit_DQM[CHANNEL_A][RANK_0][1]);
    
    mcSHOW_INFO_MSG("%s %s RX Win Per Bit: ", FAST_K, prefix);
    for(i = 0; i < DQ_DATA_WIDTH_LP4; ++i)
        mcSHOW_INFO_MSG("%d ", cali->u1RxWinPerbit_DQ[CHANNEL_A][RANK_0][i]);
    mcSHOW_INFO_MSG("\n");
}

void dump_dram_cali_data_lat(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s DATLAT: best step:%d\n",
        FAST_K, prefix,
        cali->u1RxDatlat_Save[CHANNEL_A][RANK_0]);

}

void dump_dram_cali_tx_oe(SAVE_TIME_FOR_CALIBRATION_T *cali, const char *prefix)
{
    int i;
    SAFE_POINTER(prefix);
    mcSHOW_INFO_MSG("%s %s TX OE: DQ MCK: DQS0:%d, DQS1:%d, DQ UI: DQS0:%d, DQS1:%d\n",
        FAST_K, prefix,
        cali->u1RxWinPerbitVref_Save[CHANNEL_A],
        cali->u1TX_OE_DQ_MCK[CHANNEL_A][RANK_0][0],
        cali->u1TX_OE_DQ_MCK[CHANNEL_A][RANK_0][1],
        cali->u1TX_OE_DQ_UI[CHANNEL_A][RANK_0][0],
        cali->u1TX_OE_DQ_UI[CHANNEL_A][RANK_0][1]);
}

void dump_dram_cali_save_data(SAVE_TIME_FOR_CALIBRATION_T *cali)
{
    mcSHOW_INFO_MSG("%s magic:0x%x, magic_end:0x%x\n", FAST_K, cali->magic, cali->magic_end);

    dump_dram_cali_delay_cell(cali, NULL);
    dump_dram_cali_clk_dqs_duty(cali, NULL);
    dump_dram_cali_cbt(cali, NULL);
    dump_dram_cali_write_leveling(cali, NULL);
    dump_dram_cali_gating(cali, NULL);
    dump_dram_cali_tx(cali, NULL);
    dump_dram_cali_rx(cali, NULL);
    dump_dram_cali_data_lat(cali, NULL);
    dump_dram_cali_tx_oe(cali, NULL);
}


void dump_dram_cali_shuffle_data(DRAM_CALIBRATION_SHU_DATA_T *shu_data, DRAM_DFS_SHUFFLE_TYPE_T shuffle)
{
    DRAM_CALIBRATION_SHU_DATA_T *data = shu_data + shuffle;
    SAVE_TIME_FOR_CALIBRATION_T *cali;

    mcSHOW_INFO_MSG("%s shuffle %d: checksum:%d\n", FAST_K, shuffle, data->checksum);

    cali = &shu_data->calibration_data;
    mcSHOW_INFO_MSG("%s magic:0x%x, magic_end:0x%x\n", FAST_K, cali->magic, cali->magic_end);
    dump_dram_cali_save_data(cali);
}


void dump_dram_cali_data(DRAM_CALIBRATION_DATA_T *data)
{
    int shuffle;

    dump_dram_cali_header(&data->header);

    for(shuffle = 0; shuffle < DRAM_DFS_SHUFFLE_MAX; ++shuffle)
        dump_dram_cali_shuffle_data(data->data, shuffle);
}

int read_offline_dram_calibration_data(DRAM_DFS_SHUFFLE_TYPE_T shuffle, SAVE_TIME_FOR_CALIBRATION_T *offLine_SaveData)
{
#if 1   
	int i;
	bdev_t *bootdev = NULL;
	unsigned char *ch;
    ssize_t ret;

	DRAM_CALIBRATION_HEADER_T hdr;
	DRAM_CALIBRATION_SHU_DATA_T shu_data;
    DRAM_CALIBRATION_DATA_T data;
	DRAM_CALIBRATION_DATA_T *datap = NULL;

	if (dram_offline_data_flags)
		goto exit;

	if (offLine_SaveData == NULL) {
		mcSHOW_ERR_MSG("%s offLine_SaveData == NULL, skip\n", FAST_K);
		dram_offline_data_flags = ERR_NULL_POINTER;
		goto exit;
	}

    bootdev = bio_open("mmc0");
	if (!bootdev) {
		mcSHOW_ERR_MSG("%s unable to open device:%s\n", FAST_K, "mmc0");
		return -1;
	}

#if 1
	ret = bio_read(bootdev, (u8*)&hdr, BOOT_PARA_OFFSET, sizeof(hdr));
	if (ret != sizeof(hdr)) {
		mcSHOW_ERR_MSG("%s bio_read %s failed\n", FAST_K, "hdr");
		dram_offline_data_flags = ERR_BLKDEV_READ_FAIL;
		goto exit;
	}

    memset(&data, 0x0, sizeof(data));
    memset(&shu_data, 0x0, sizeof(shu_data));
    
    /* check preloader version */
	if (hdr.pl_version != FAST_K_LK_VERSION) {
		/* current preloader version does not match the calibration hdr in storage -> erase the partition */
		mcSHOW_INFO_MSG("%s FAST_K_LK_VERSION is updated, erase the DRAM shu_data\n", FAST_K);

		ret = bio_write(bootdev, (u8*)&data, BOOT_PARA_OFFSET, sizeof(data));
		if (ret != sizeof(data)) {
			mcSHOW_ERR_MSG("%s clear dram para failed:%d, expect:%d\n", FAST_K, ret, sizeof(data));
			dram_offline_data_flags = ERR_BLKDEV_WRITE_FAIL;
			goto exit;
		}

		dram_offline_data_flags = ERR_PL_UPDATED;
		goto exit;
	}

	/* check magic number */
	if (hdr.magic_number != DRAM_CALIBRATION_DATA_MAGIC) {
		mcSHOW_ERR_MSG("%s magic number mismatch:actual:%d, expect:%d\n", FAST_K, hdr.magic_number, DRAM_CALIBRATION_DATA_MAGIC);
		dram_offline_data_flags = ERR_MAGIC_NUMBER;
		goto exit;
	}

	ret = bio_read(bootdev, (u8*)&shu_data, BOOT_PARA_OFFSET + ((unsigned long) &datap->data[shuffle]), sizeof(shu_data));
	if (ret != sizeof(shu_data)) {
		mcSHOW_ERR_MSG("%s blkdev_read shuffle data failed:%u\n", FAST_K, ret);
		dram_offline_data_flags = ERR_BLKDEV_READ_FAIL;
		goto exit;
	}

	/* check checksum */
	if (check_checksum_for_dram_data(&shu_data) != 1) {
		mcSHOW_ERR_MSG("%s checksum failed\n", FAST_K);
		dram_offline_data_flags = ERR_CHECKSUM;
		goto exit;
	}

	/* copy the data stored in storage to the data structure for calibration */
	memcpy(offLine_SaveData, &(shu_data.calibration_data), sizeof(*offLine_SaveData));

#endif

exit:
	if (dram_offline_data_flags)
		SET_DRAM_STORAGE_API_ERR(dram_offline_data_flags, DRAM_STORAGE_API_READ);

    if (bootdev)
        bio_close(bootdev);
        
    return 0 - dram_offline_data_flags;
#else
        return 0;
#endif

}

int write_offline_dram_calibration_data(DRAM_DFS_SHUFFLE_TYPE_T shuffle, SAVE_TIME_FOR_CALIBRATION_T *offLine_SaveData)
{
#if 1    
    int ret;
    bdev_t *bootdev = NULL;
    DRAM_CALIBRATION_HEADER_T hdr;
    DRAM_CALIBRATION_SHU_DATA_T shu_data;
    DRAM_CALIBRATION_DATA_T data;
    DRAM_CALIBRATION_DATA_T *datap = NULL;

    if (offLine_SaveData == NULL) {
        mcSHOW_ERR_MSG("%s offLine_SaveData == NULL, skip\n", FAST_K);
        SET_DRAM_STORAGE_API_ERR(ERR_NULL_POINTER, DRAM_STORAGE_API_WRITE);
        return -ERR_NULL_POINTER;
    }

    bootdev = bio_open("mmc0");
    if (bootdev == NULL) {
        mcSHOW_ERR_MSG("%s can't find boot partition(%s)\n", FAST_K, "mmc0");
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_NOT_FOUND, DRAM_STORAGE_API_WRITE);
        return -ERR_BLKDEV_NOT_FOUND;
    }

    memcpy(&(shu_data.calibration_data), offLine_SaveData, sizeof(*offLine_SaveData));
    
    /* assign PL version */
    hdr.pl_version = FAST_K_LK_VERSION;

    /* assign magic number */
    hdr.magic_number = DRAM_CALIBRATION_DATA_MAGIC;

    /* assign api error code */
    hdr.calib_err_code = g_dram_storage_api_err_code;

    ret = bio_write(bootdev, (u8*)&hdr, BOOT_PARA_OFFSET, sizeof(hdr));
    if (ret != sizeof(hdr)) {
        mcSHOW_INFO_MSG("%s blkdev_write hdr failed\n", FAST_K);
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_WRITE_FAIL, DRAM_STORAGE_API_WRITE);
        bio_close(bootdev);
        return -ERR_BLKDEV_WRITE_FAIL;
    }

    // check write is ok?
    ret = bio_read(bootdev, (u8*)&hdr, BOOT_PARA_OFFSET, sizeof(hdr));
    if (ret != sizeof(hdr)) {
        mcSHOW_ERR_MSG("%s bio_read %s failed\n", FAST_K, "hdr");
    } else 
        dump_dram_cali_header(&hdr);

    /* calculate and assign checksum */
    assign_checksum_for_dram_data(&shu_data);

    ret = bio_write(bootdev, (u8*)&shu_data, BOOT_PARA_OFFSET + ((unsigned long) &datap->data[shuffle]), sizeof(shu_data));
    if (ret != sizeof(shu_data)) {
        mcSHOW_ERR_MSG("%s blkdev_write dram para failed:%d\n", FAST_K, ret);
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_WRITE_FAIL, DRAM_STORAGE_API_WRITE);
        bio_close(bootdev);
        return -ERR_BLKDEV_WRITE_FAIL;
    }

    bio_close(bootdev);

#endif

    return 0;

}

int compare_dram_cali_data(DRAM_DFS_SHUFFLE_TYPE_T shuffle, SAVE_TIME_FOR_CALIBRATION_T *offLine_SaveData)
{
    bdev_t *bootdev = NULL;
    ssize_t ret = 0;

    DRAM_CALIBRATION_SHU_DATA_T shu_data;
    DRAM_CALIBRATION_DATA_T data;
    DRAM_CALIBRATION_DATA_T *datap = NULL;
    SAVE_TIME_FOR_CALIBRATION_T *save_cali_data;

    bootdev = bio_open("mmc0");
    if (!bootdev) {
        mcSHOW_ERR_MSG("%s compare_dram_cali_data: unable to open device:%s\n", FAST_K, "mmc0");
        return -1;
    }

    ret = bio_read(bootdev, (u8*)&shu_data, BOOT_PARA_OFFSET + ((unsigned long) &datap->data[shuffle]), sizeof(shu_data));
	if (ret != sizeof(shu_data)) {
		mcSHOW_ERR_MSG("%s blkdev_read %s failed:%u\n", FAST_K, ret);
		ret = ERR_BLKDEV_READ_FAIL;
		goto error;
	}

    save_cali_data = &shu_data.calibration_data;

    // dump memory 
    dramc_dump_memory(offLine_SaveData, sizeof(*offLine_SaveData));
    dramc_dump_memory(save_cali_data, sizeof(*save_cali_data));

    ret = memcmp(save_cali_data, offLine_SaveData, sizeof(*offLine_SaveData));
    if (ret == 0){
        mcSHOW_INFO_MSG("%s [%s] pass!\n", FAST_K, __func__);
    }else{
        mcSHOW_ERR_MSG("%s [%s] failed!\n", FAST_K, __func__);
    }
    bio_close(bootdev);
    return 0;

error:
    if (bootdev)
        bio_close(bootdev);

    return ret;
}

int clean_dram_calibration_data(void)
{
    bdev_t *bootdev = NULL;
    ssize_t ret;

    DRAM_CALIBRATION_DATA_T data;

    bootdev = bio_open("mmc0");
    if (!bootdev) {
        mcSHOW_ERR_MSG("%s unable to open device:%s\n", FAST_K, "mmc0");
        return -1;
    }

    memset(&data, 0x0, sizeof(data));
    ret = bio_write(bootdev, (u8*)&data, BOOT_PARA_OFFSET, sizeof(data));
    if (ret != sizeof(data)) {
        mcSHOW_ERR_MSG("%s clear dram para failed:%d, expect:%d\n", FAST_K, ret, sizeof(data));
        SET_DRAM_STORAGE_API_ERR(ERR_BLKDEV_WRITE_FAIL, DRAM_STORAGE_API_WRITE);
        return -ERR_BLKDEV_WRITE_FAIL;
    }

    bio_close(bootdev);

    return 0;
}

#endif
#else

DRAM_CALIBRATION_DATA_T dram_data; // using global variable to avoid stack overflow

int read_offline_dram_calibration_data(DRAM_DFS_SHUFFLE_TYPE_T shuffle, SAVE_TIME_FOR_CALIBRATION_T *offLine_SaveData)
{
	return 0;
}

int write_offline_dram_calibration_data(DRAM_DFS_SHUFFLE_TYPE_T shuffle, SAVE_TIME_FOR_CALIBRATION_T *offLine_SaveData)
{
	return 0;
}

int clean_dram_calibration_data(void)
{
	return;
}
#endif


void set_err_code_for_storage_api(void)
{
#ifdef LAST_DRAMC
	last_dramc_info_ptr->storage_api_err_flag = g_dram_storage_api_err_code;
	dsb();
#endif
}

#endif

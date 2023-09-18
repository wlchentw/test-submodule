#include <debug.h>
#include <platform/mtk_wdt.h>
#include <reg.h>

#if ENABLE_WDT_MODULE

static bool mtk_wd_CheckNonResetReg2(unsigned int offset)
{
    u32 tmp;
    tmp = readl(MTK_WDT_NONRST_REG2);
    if (tmp & (1U << offset))
        return true;
    else
        return false;
}

static void mtk_wd_SetNonResetReg2(unsigned int offset, bool value)
{
    u32 reg;

    reg = readl(MTK_WDT_NONRST_REG2);
    if (value)
        reg |= (1U << offset);
    else
        reg &= ~(1U << offset);

    writel(reg, MTK_WDT_NONRST_REG2);
}

void set_clr_fastboot_mode(bool flag)
{
    if (flag == true)
        mtk_wd_SetNonResetReg2(0x2, 1);
    else if (flag == false)
        mtk_wd_SetNonResetReg2(0x2, 0);

    dprintf(INFO, "set_clr_fastboot_mode\n");
}

void set_clr_recovery_mode(bool flag)
{
    if (flag == true)
        mtk_wd_SetNonResetReg2(0x1, 1);
    else if (flag == false)
        mtk_wd_SetNonResetReg2(0x1, 0);

    dprintf(INFO, "set_clr_recovery_mode\n");
}

bool check_fastboot_mode(void)
{
#if !(CFG_FPGA_PLATFORM)
    return mtk_wd_CheckNonResetReg2(0x2);
#else
	return false;
#endif
}

bool check_recovery_mode(void)
{
    return mtk_wd_CheckNonResetReg2(0x1);
}

void mtk_wdt_disable(void)
{
    u32 tmp;

    tmp = readl(MTK_WDT_MODE);
    tmp &= ~MTK_WDT_MODE_ENABLE;       /* disable watchdog */
    tmp |= (MTK_WDT_MODE_KEY);         /* need key then write is allowed */
    writel(tmp, MTK_WDT_MODE);
}

static void mtk_wdt_reset(char mode)
{
    /* Watchdog Rest */
    unsigned int wdt_mode_val;
    writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);

    wdt_mode_val = readl(MTK_WDT_MODE);
    /* clear autorestart bit: autoretart: 1, bypass power key, 0: not bypass power key */
    wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
    /* make sure WDT mode is hw reboot mode, can not config isr mode  */
    wdt_mode_val &= ~(MTK_WDT_MODE_IRQ | MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE);

    wdt_mode_val |= (MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN);

    if (mode)  /* mode != 0 means by pass power key reboot, We using auto_restart bit as by pass power key flag */
        wdt_mode_val |= MTK_WDT_MODE_AUTO_RESTART;

    writel(wdt_mode_val, MTK_WDT_MODE);

    spin(100);
    writel(MTK_WDT_SWRST_KEY, MTK_WDT_SWRST);
}

static unsigned int mtk_wdt_check_status(void)
{
    static unsigned int status = 0;

    /*
     * Because WDT_STA register will be cleared after writing WDT_MODE,
     * we use a static variable to store WDT_STA.
     * After reset, static varialbe will always be clear to 0,
     * so only read WDT_STA when static variable is 0 is OK
     */
    if (0 == status)
        status = readl(MTK_WDT_STATUS);

    return status;
}

static void mtk_wdt_mode_config(bool dual_mode_en,
                                bool irq,
                                bool ext_en,
                                bool ext_pol,
                                bool wdt_en)
{
    unsigned int tmp;

    tmp = readl(MTK_WDT_MODE);
    tmp |= MTK_WDT_MODE_KEY;

    // Bit 0 : Whether enable watchdog or not
    if (wdt_en == true)
        tmp |= MTK_WDT_MODE_ENABLE;
    else
        tmp &= ~MTK_WDT_MODE_ENABLE;

    // Bit 1 : Configure extern reset signal polarity.
    if (ext_pol == true)
        tmp |= MTK_WDT_MODE_EXT_POL;
    else
        tmp &= ~MTK_WDT_MODE_EXT_POL;

    // Bit 2 : Whether enable external reset signal
    if (ext_en == true)
        tmp |= MTK_WDT_MODE_EXTEN;
    else
        tmp &= ~MTK_WDT_MODE_EXTEN;

    // Bit 3 : Whether generating interrupt instead of reset signal
    if (irq == true)
        tmp |= MTK_WDT_MODE_IRQ;
    else
        tmp &= ~MTK_WDT_MODE_IRQ;

    // Bit 6 : Whether enable debug module reset
    if (dual_mode_en == true)
        tmp |= MTK_WDT_MODE_DUAL_MODE;
    else
        tmp &= ~MTK_WDT_MODE_DUAL_MODE;

    // Bit 4: WDT_Auto_restart, this is a reserved bit, we use it as bypass powerkey flag.
    //      Because HW reboot always need reboot to kernel, we set it always.
    tmp |= MTK_WDT_MODE_AUTO_RESTART;

    writel(tmp, MTK_WDT_MODE);
    //dual_mode(1); //always dual mode
    //mdelay(100);
    dprintf(INFO,"mtk_wdt_mode_config LK mode value=%x", readl(MTK_WDT_MODE));
}

static void mtk_wdt_set_time_out_value(uint32_t value)
{
    static unsigned int timeout;

    /*
    * TimeOut = BitField 15:5
    * Key      = BitField  4:0 = 0x08
    */

    // sec * 32768 / 512 = sec * 64 = sec * 1 << 6
    timeout = (unsigned int)(value * ( 1 << 6) );
    timeout = timeout << 5;
    writel((timeout | MTK_WDT_LENGTH_KEY), MTK_WDT_LENGTH);
}

static void mtk_wdt_restart(void)
{
    // Reset WatchDogTimer's counting value to time out value
    // ie., keepalive()
    writel(MTK_WDT_RESTART_KEY, MTK_WDT_RESTART);
}

static void mtk_wdt_sw_reset(void)
{
    printf ("UB WDT SW RESET\n");
    mtk_wdt_reset(1); /* NOTE here, this reset will cause by pass power key */

    while (1) {
        printf ("UB SW reset fail ... \n");
    }
}

static void mtk_wdt_hw_reset(void)
{
    dprintf(INFO,"UB WDT_HW_Reset\n");

    // 1. set WDT timeout 1 secs, 1*64*512/32768 = 1sec
    mtk_wdt_set_time_out_value(1);

    // 2. enable WDT debug reset enable, generating irq disable, ext reset disable
    //    ext reset signal low, wdt enalbe
    mtk_wdt_mode_config(true, false, false, false, true);

    // 3. reset the watch dog timer to the value set in WDT_LENGTH register
    mtk_wdt_restart();

    // 4. system will reset
    while (1);
}

void mtk_wdt_init(void)
{
    /* This function will store the reset reason: Time out/ SW trigger */
    dprintf(ALWAYS, "RGU STA: %x\n", mtk_wdt_check_status());

    mtk_wdt_mode_config(false, false, false, false, false);

#if (!LK_WDT_DISABLE)
    mtk_wdt_set_time_out_value(10);
    mtk_wdt_mode_config(true, true, true, false, true);
    mtk_wdt_restart();
#endif
}

static bool mtk_is_rgu_trigger_reset(void)
{
    if (mtk_wdt_check_status())
        return true;
    return false;
}

void mtk_arch_reset(char mode)
{
    dprintf(INFO,"UB mtk_arch_reset\n");

    mtk_wdt_reset(mode);

    while (1);
}

static void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)
{
    if (WD_MD_RST == reset_type) {
        unsigned int wdt_dbg_ctrl;
        wdt_dbg_ctrl = readl(MTK_WDT_SWSYSRST);
        wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
        wdt_dbg_ctrl |= 0x80;// 1<<7
        writel(wdt_dbg_ctrl, MTK_WDT_SWSYSRST);
        spin(1000);
        wdt_dbg_ctrl = readl(MTK_WDT_SWSYSRST);
        wdt_dbg_ctrl |= MTK_WDT_SWSYS_RST_KEY;
        wdt_dbg_ctrl &= (~0x80);// ~(1<<7)
        writel(wdt_dbg_ctrl, MTK_WDT_SWSYSRST);
        dprintf(INFO,"rgu pl md reset\n");
    }
}

int rgu_dram_reserved(int enable)
{
    volatile unsigned int tmp, ret = 0;
    if(1 == enable)
    {
        /* enable ddr reserved mode */
        tmp = readl(MTK_WDT_MODE);
        tmp |= (MTK_WDT_MODE_DDR_RESERVE|MTK_WDT_MODE_KEY);
        writel(tmp, MTK_WDT_MODE);

    } else if(0 == enable)
    {
        /* disable ddr reserved mode, set reset mode,
               disable watchdog output reset signal */
        tmp = readl(MTK_WDT_MODE);
        tmp &= (~MTK_WDT_MODE_DDR_RESERVE);
        tmp |= MTK_WDT_MODE_KEY;
        writel(tmp, MTK_WDT_MODE);
    } else
    {
        dprintf(CRITICAL,"Wrong input %d, should be 1(enable) or 0(disable) in %s\n", enable, __func__);
        ret = -1;
    }
	dprintf(CRITICAL,"RGU %s:MTK_WDT_MODE(%x)\n", __func__,tmp);
    return ret;
}

int rgu_is_reserve_ddr_enabled(void)
{
  unsigned int wdt_mode;
  wdt_mode = readl(MTK_WDT_MODE);
  if(wdt_mode & MTK_WDT_MODE_DDR_RESERVE)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int rgu_is_dram_slf(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = readl(MTK_WDT_DRAMC_CTL);
  dprintf(CRITICAL,"DDR is in self-refresh. %x\n", wdt_dbg_ctrl);
  if(wdt_dbg_ctrl & MTK_DDR_SREF_STA)
  {
    //dprintf(CRITICAL,"DDR is in self-refresh. %x\n", wdt_dbg_ctrl);
    return 1;
  }
  else
  {
    //dprintf(CRITICAL,"DDR is not in self-refresh. %x\n", wdt_dbg_ctrl);
    return 0;
  }
}

void rgu_release_rg_dramc_conf_iso(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = readl(MTK_WDT_DRAMC_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_CONF_ISO);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  writel(wdt_dbg_ctrl, MTK_WDT_DRAMC_CTL);
  dprintf(CRITICAL,"RGU %s:MTK_WDT_DRAMC_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}

void rgu_release_rg_dramc_iso(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = readl(MTK_WDT_DRAMC_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_DRAMC_ISO);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  writel(wdt_dbg_ctrl, MTK_WDT_DRAMC_CTL);
  dprintf(CRITICAL,"RGU %s:MTK_WDT_DRAMC_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}

void rgu_release_rg_dramc_sref(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = readl(MTK_WDT_DRAMC_CTL);
  wdt_dbg_ctrl &= (~MTK_RG_DRAMC_SREF);
  wdt_dbg_ctrl |= MTK_DEBUG_CTL_KEY;
  writel(wdt_dbg_ctrl, MTK_WDT_DRAMC_CTL);
  dprintf(CRITICAL,"RGU %s:MTK_WDT_DRAMC_CTL(%x)\n", __func__,wdt_dbg_ctrl);
}
int rgu_is_reserve_ddr_mode_success(void)
{
  unsigned int wdt_dbg_ctrl;
  wdt_dbg_ctrl = readl(MTK_WDT_DRAMC_CTL);
  if(wdt_dbg_ctrl & MTK_DDR_RESERVE_RTA)
  {
    dprintf(CRITICAL,"WDT DDR reserve mode success! %x\n",wdt_dbg_ctrl);
    return 1;
  }
  else
  {
    dprintf(CRITICAL,"WDT DDR reserve mode FAIL! %x\n",wdt_dbg_ctrl);
    return 0;
  }
}

#else

void mtk_wdt_init(void)
{
    dprintf(INFO,"UB WDT Dummy init called\n");
}

static bool mtk_is_rgu_trigger_reset()
{
    dprintf(INFO,"UB Dummy mtk_is_rgu_trigger_reset called\n");
    return FALSE;
}

void mtk_arch_reset(char mode)
{
    dprintf(INFO,"UB WDT Dummy arch reset called\n");
}

int mtk_wdt_boot_check(void)
{
    dprintf(INFO,"UB WDT Dummy mtk_wdt_boot_check called\n");
    return WDT_NOT_WDT_REBOOT;
}

void mtk_wdt_disable(void)
{
    dprintf(INFO,"UB WDT Dummy mtk_wdt_disable called\n");
}

static void mtk_wdt_restart(void)
{
    dprintf(INFO,"UB WDT Dummy mtk_wdt_restart called\n");
}
static void mtk_wdt_sw_reset(void)
{
    dprintf(INFO,"UB WDT Dummy mtk_wdt_sw_reset called\n");
}

static void mtk_wdt_hw_reset(void)
{
    dprintf(INFO,"UB WDT Dummy mtk_wdt_hw_reset called\n");
}

static void rgu_swsys_reset(WD_SYS_RST_TYPE reset_type)
{
    dprintf(INFO,"UB WDT Dummy rgu_swsys_reset called\n");
}
#endif

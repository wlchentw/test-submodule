#ifndef SOC_MEDIATEK_MT8518_PMIC_WRAP_H
#define SOC_MEDIATEK_MT8518_PMIC_WRAP_H

#include <platform/pll.h>
#include <platform/mt8518.h>
#include <platform/mt_reg_base.h>

/* external API */
s32 pwrap_read(u32 adr, u32 *rdata);
s32 pwrap_write(u32 adr, u32 wdata);
s32 pwrap_wacs2(u32 write, u32 adr, u32 wdata, u32 *rdata);
s32 pwrap_init_preloader(void);

#define read32(addr)		readl(addr)
#define write32(addr,val)	writel((val), (addr))

#define MUDI

#define MODULE_SW_CG_1_SET	(CKSYS_BASE+0x054)
#define MODULE_SW_CG_1_CLR	(CKSYS_BASE+0x084)

#define MODULE_SW_WDT_RST	(INFRACFG_AO_BASE+0x02C)

#define PWRAP_CG_TMR	(1 << 27)
#define PWRAP_CG_SPI	(1 << 28)
#define PWRAP_CG_SYS	(1 << 29)

#define PMIC_B_DCM_EN	(1 << 1)
#define PMIC_SPI_DCM_EN	(1 << 2)

#define CLK_CFG_5_CLR		(CKSYS_BASE+0x098)
#define CLK_SPI_CK_26M		0x1

#define PWRAP_BASE          (IO_PHYS + 0xF000)
#define PMIC_WRAP_BASE		(PWRAP_BASE)//0x1000F000
static struct MT8518_pwrap_regs * const mt8518_pwrap = (void *)PMIC_WRAP_BASE;

enum {
	WACS2 = 1 << 3
};

/* timeout setting */
enum {
	TIMEOUT_READ_US        = 255,
	TIMEOUT_WAIT_IDLE_US   = 255
};

/* PMIC_WRAP registers */
struct MT8518_pwrap_regs {
	u32 mux_sel;
	u32 wrap_en;
	u32 dio_en;
	u32 sidly;
	u32 rddmy;
	u32 si_ck_con;
	u32 cshext_write;
	u32 cshext_read;
	u32 cslext_start;
	u32 cslext_end;
	u32 staupd_prd;
	u32 staupd_grpen;
	u32 reserved[4];
	u32 staupd_man_trig;
	u32 staupd_sta;
	u32 wrap_sta;
	u32 harb_init;
	u32 harb_hprio;
	u32 hiprio_arb_en;
	u32 harb_sta0;
	u32 harb_sta1;
	u32 man_en;
	u32 man_cmd;
	u32 man_rdata;
	u32 man_vldclr;
	u32 wacs0_en;
	u32 init_done0;
	u32 wacs0_cmd;
	u32 wacs0_rdata;
	u32 wacs0_vldclr;
	u32 wacs1_en;
	u32 init_done1;
	u32 wacs1_cmd;
	u32 wacs1_rdata;
	u32 wacs1_vldclr;
	u32 wacs2_en;
	u32 init_done2;
	u32 wacs2_cmd;
	u32 wacs2_rdata;
	u32 wacs2_vldclr;
	u32 int_en;
	u32 int_flg_raw;
	u32 int_flg;
	u32 int_clr;
	u32 sig_adr;
	u32 sig_mode;
	u32 sig_value;
	u32 sig_errval;
	u32 crc_en;
	u32 timer_en;
	u32 timer_sta;
	u32 wdt_unit;
	u32 wdt_src_en;
	u32 wdt_flg;
	u32 debug_int_sel;
	u32 dvfs_adr0;
	u32 dvfs_wdata0;
	u32 dvfs_adr1;
	u32 dvfs_wdata1;
	u32 dvfs_adr2;
	u32 dvfs_wdata2;
	u32 dvfs_adr3;
	u32 dvfs_wdata3;
	u32 dvfs_adr4;
	u32 dvfs_wdata4;
	u32 dvfs_adr5;
	u32 dvfs_wdata5;
	u32 dvfs_adr6;
	u32 dvfs_wdata6;
	u32 dvfs_adr7;
	u32 dvfs_wdata7;
	u32 spminf_sta;
	u32 cipher_key_sel;
	u32 cipher_iv_sel;
	u32 cipher_en;
	u32 cipher_rdy;
	u32 cipher_mode;
	u32 cipher_swrst;
	u32 dcm_en;
	u32 dcm_dbc_prd;
	u32 ext_ck;
	u32 adc_cmd_addr;
	u32 adc_cmd;
	u32 adc_rdy_addr;
	u32 adc_rdata_addr1;
	u32 adc_rdata_addr2;
	u32 gps_sta;
	u32 swrst;
	u32 op_type;
	u32 msb_first;
};

enum {
	RDATA_WACS_RDATA_SHIFT = 0,
	RDATA_WACS_FSM_SHIFT   = 16,
	RDATA_WACS_REQ_SHIFT   = 19,
	RDATA_SYNC_IDLE_SHIFT,
	RDATA_INIT_DONE_SHIFT,
	RDATA_SYS_IDLE_SHIFT,
};

enum {
	RDATA_WACS_RDATA_MASK = 0xffff,
	RDATA_WACS_FSM_MASK   = 0x7,
	RDATA_WACS_REQ_MASK   = 0x1,
	RDATA_SYNC_IDLE_MASK  = 0x1,
	RDATA_INIT_DONE_MASK  = 0x1,
	RDATA_SYS_IDLE_MASK   = 0x1,
};

/* WACS_FSM */
enum {
	WACS_FSM_IDLE     = 0x00,
	WACS_FSM_REQ      = 0x02,
	WACS_FSM_WFDLE    = 0x04, /* wait for dle, wait for read data done */
	WACS_FSM_WFVLDCLR = 0x06, /* finish read data, wait for valid flag
				   * clearing */
	WACS_INIT_DONE    = 0x01,
	WACS_SYNC_IDLE    = 0x01,
	WACS_SYNC_BUSY    = 0x00
};

#define DEW_BASE  (0xBC00)
//-------macro for pmic register--------------------------------
#define PMIC_BASE (0x0000)
#define PMIC_WRP_CKPDN            (PMIC_BASE+0x011A) //0x0056
#define PMIC_WRP_RST_CON          (PMIC_BASE+0x0120)//0x005C
#define PMIC_TOP_CKCON2           (PMIC_BASE+0x012A)
#define PMIC_TOP_CKCON3           (PMIC_BASE+0x01D4)
//-----macro for dewrapper regsister--------------------------------------------------------
#define DEW_EVENT_OUT_EN   (DEW_BASE+0x0)
#define DEW_DIO_EN         (DEW_BASE+0x2)
#define DEW_EVENT_SRC_EN   (DEW_BASE+0x4)
#define DEW_EVENT_SRC      (DEW_BASE+0x6)
#define DEW_EVENT_FLAG     (DEW_BASE+0x8)
#define DEW_READ_TEST      (DEW_BASE+0xA)
#define DEW_WRITE_TEST     (DEW_BASE+0xC)
#define DEW_CRC_EN         (DEW_BASE+0xE)
#define DEW_CRC_VAL        (DEW_BASE+0x10)
#define DEW_MON_GRP_SEL    (DEW_BASE+0x12)
#define DEW_MON_FLAG_SEL   (DEW_BASE+0x14)
#define DEW_EVENT_TEST     (DEW_BASE+0x16)
#define DEW_CIPHER_KEY_SEL (DEW_BASE+0x18)
#define DEW_CIPHER_IV_SEL  (DEW_BASE+0x1A)
#define DEW_CIPHER_LOAD    (DEW_BASE+0x1C)
#define DEW_CIPHER_START   (DEW_BASE+0x1E)
#define DEW_CIPHER_RDY     (DEW_BASE+0x20)
#define DEW_CIPHER_MODE    (DEW_BASE+0x22)
#define DEW_CIPHER_SWRST   (DEW_BASE+0x24)
#define DEW_CIPHER_IV0     (DEW_BASE+0x26)
#define DEW_CIPHER_IV1     (DEW_BASE+0x28)
#define DEW_CIPHER_IV2     (DEW_BASE+0x2A)
#define DEW_CIPHER_IV3     (DEW_BASE+0x2C)
#define DEW_CIPHER_IV4     (DEW_BASE+0x2E)
#define DEW_CIPHER_IV5     (DEW_BASE+0x30)

/* dewrapper defaule value */
enum {
	DEFAULT_VALUE_READ_TEST  = 0x5aa5,
	WRITE_TEST_VALUE         = 0xa55a
};

enum pmic_regck {
	REG_CLOCK_SAFE_MODE,
	REG_CLOCK_12MHZ,
	REG_CLOCK_24MHZ
};

/* manual commnd */
enum {
	OP_WR    = 0x1,
	OP_CSH   = 0x0,
	OP_CSL   = 0x1,
	OP_OUTS  = 0x8,
	OP_OUTD  = 0x9,
	OP_INS   = 0xC,
	OP_IND   = 0xE
};

/* error information flag */
enum {
	E_PWR_INVALID_ARG             = 1,
	E_PWR_INVALID_RW              = 2,
	E_PWR_INVALID_ADDR            = 3,
	E_PWR_INVALID_WDAT            = 4,
	E_PWR_INVALID_OP_MANUAL       = 5,
	E_PWR_NOT_IDLE_STATE          = 6,
	E_PWR_NOT_INIT_DONE           = 7,
	E_PWR_NOT_INIT_DONE_READ      = 8,
	E_PWR_WAIT_IDLE_TIMEOUT       = 9,
	E_PWR_WAIT_IDLE_TIMEOUT_READ  = 10,
	E_PWR_INIT_SIDLY_FAIL         = 11,
	E_PWR_RESET_TIMEOUT           = 12,
	E_PWR_TIMEOUT                 = 13,
	E_PWR_INIT_RESET_SPI          = 20,
	E_PWR_INIT_SIDLY              = 21,
	E_PWR_INIT_REG_CLOCK          = 22,
	E_PWR_INIT_ENABLE_PMIC        = 23,
	E_PWR_INIT_DIO                = 24,
	E_PWR_INIT_CIPHER             = 25,
	E_PWR_INIT_WRITE_TEST         = 26,
	E_PWR_INIT_ENABLE_CRC         = 27,
	E_PWR_INIT_ENABLE_DEWRAP      = 28,
	E_PWR_INIT_ENABLE_EVENT       = 29,
	E_PWR_READ_TEST_FAIL          = 30,
	E_PWR_WRITE_TEST_FAIL         = 31,
	E_PWR_SWITCH_DIO              = 32,
	E_PWR_INVALID_DATA            = 33
};

/* external API */
extern s32 pwrap_read(u32 adr, u32 *rdata);
extern s32 pwrap_write(u32 adr, u32 wdata);
extern s32 pwrap_wacs2(u32 write, u32 adr, u32 wdata, u32 *rdata);
extern s32 pwrap_init(void);

#endif /* SOC_MEDIATEK_MT8518_PMIC_WRAP_H */

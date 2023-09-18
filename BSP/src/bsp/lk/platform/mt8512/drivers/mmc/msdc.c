/*
 * Copyright (c) 2016 MediaTek Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#define MSDC_DEBUG_KICKOFF

#include <platform/msdc.h>
#include <platform/mmc_core.h>
#include <kernel/event.h>
#include <kernel/vm.h>
#include <platform/interrupts.h>
#include <platform/mt_irq.h>
#include <string.h>
#include <assert.h>

#define CMD_RETRIES        (5)
#define CMD_TIMEOUT        (100) /* 100ms */

#define PERI_MSDC_SRCSEL   (0xc100000c)

/* Tuning Parameter */
#define DEFAULT_DEBOUNCE   (8)  /* 8 cycles */
#define DEFAULT_DTOC       (40) /* data timeout counter. 65536x40 sclk. */
#define DEFAULT_WDOD       (0)  /* write data output delay. no delay. */
#define DEFAULT_BSYDLY     (8)  /* card busy delay. 8 extend sclk */

/* Declarations */
static int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd);
static int msdc_wait_cmd_done(struct mmc_host *host, struct mmc_command *cmd);
static int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd);

typedef struct {
    int    autocmd;
    int    rdsmpl;
    int    wdsmpl;
    int    rsmpl;
    int    start_bit;
} msdc_priv_t;

static int msdc_rsp[] = {
    0,  /* RESP_NONE */
    1,  /* RESP_R1 */
    2,  /* RESP_R2 */
    3,  /* RESP_R3 */
    4,  /* RESP_R4 */
    1,  /* RESP_R5 */
    1,  /* RESP_R6 */
    1,  /* RESP_R7 */
    7,  /* RESP_R1b */
};

struct msdc_cust {
    unsigned char  clk_src;           /* host clock source             */
    unsigned char  hclk_src;           /* host clock source             */
    unsigned char  cmd_edge;          /* command latch edge            */
    unsigned char  data_edge;         /* data latch edge               */
#define MSDC_SMPL_RISING        (0)
#define MSDC_SMPL_FALLING       (1)
#define MSDC_SMPL_SEPERATE      (2)
    unsigned char  clk_drv;           /* clock pad driving             */
    unsigned char  cmd_drv;           /* command pad driving           */
    unsigned char  dat_drv;           /* data pad driving              */
    unsigned char  rst_drv;           /* reset pin pad driving         */
    unsigned char  ds_drv;            /* ds pad driving                */
    unsigned char  data_pins;         /* data pins                     */
    unsigned int   data_offset;       /* data address offset           */
    unsigned int   flags;             /* hardware capability flags     */
#define MSDC_CD_PIN_EN      (1 << 0)  /* card detection pin is wired   */
#define MSDC_WP_PIN_EN      (1 << 1)  /* write protection pin is wired */
#define MSDC_RST_PIN_EN     (1 << 2)  /* emmc reset pin is wired       */
#define MSDC_SDIO_IRQ       (1 << 3)  /* use internal sdio irq (bus)   */
#define MSDC_EXT_SDIO_IRQ   (1 << 4)  /* use external sdio irq         */
#define MSDC_REMOVABLE      (1 << 5)  /* removable slot                */
#define MSDC_SYS_SUSPEND    (1 << 6)  /* suspended by system           */
#define MSDC_HIGHSPEED      (1 << 7)  /* high-speed mode support       */
#define MSDC_UHS1           (1 << 8)  /* uhs-1 mode support            */
#define MSDC_DDR            (1 << 9)  /* ddr mode support              */
#define MSDC_HS200          (1 << 10) /* hs200 mode support(eMMC4.5)   */
#define MSDC_HS400          (1 << 11) /* hs200 mode support(eMMC5.0)   */
} msdc_cap[2] = {
    {
        MSDC50_CLKSRC_DEFAULT, /* host clock source          */
        MSDC50_CLKSRC4HCLK_273MHZ, /* host clock source          */
        MSDC_SMPL_RISING,   /* command latch edge            */
        MSDC_SMPL_RISING,   /* data latch edge               */
        MSDC_DRVN_GEAR2,    /* clock pad driving             */
        MSDC_DRVN_GEAR2,    /* command pad driving           */
        MSDC_DRVN_GEAR2,    /* data pad driving              */
        MSDC_DRVN_GEAR2,    /* rst pad driving               */
        MSDC_DRVN_GEAR2,    /* ds pad driving                */
        8,                  /* data pins                     */
        0,                  /* data address offset           */
        MSDC_HIGHSPEED
    },

    {
        MSDC50_CLKSRC_DEFAULT, /* host clock source          */
        MSDC50_CLKSRC4HCLK_273MHZ, /* host clock source          */
        MSDC_SMPL_RISING,   /* command latch edge            */
        MSDC_SMPL_RISING,   /* data latch edge               */
        MSDC_DRVN_GEAR2,    /* clock pad driving             */
        MSDC_DRVN_GEAR2,    /* command pad driving           */
        MSDC_DRVN_GEAR2,    /* data pad driving              */
        MSDC_DRVN_GEAR2,    /* rst pad driving               */
        MSDC_DRVN_GEAR2,    /* ds pad driving                */
        4,                  /* data pins                     */
        0,                  /* data address offset           */
        MSDC_HIGHSPEED
    },
};

static event_t msdc_int_event;
static u32 g_int_status = 0;
static msdc_priv_t msdc_priv;

#ifndef FPGA_PLATFORM
static u32 hclks_msdc30[] = { 26000000, 208000000, 200000000, 156000000,
                              182000000, 156000000, 178280000
                            };
/* add function for MSDC_PAD_CTL handle */
static void msdc_set_smt(struct mmc_host *host, int smt)
{
	return;
}

/* pull up means that host driver the line to HIGH
 * pull down means that host driver the line to LOW */
static void msdc_pin_set(struct mmc_host *host)
{
    /* driver CLK/DAT pin */
    ASSERT(host);

    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 22, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 21, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 20, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 19, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 18, 0);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 17, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 16, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 15, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 14, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 13, 1);
    MSDC_SET_FIELD(GPIO_PULLSEL2, 0x1 << 12, 1);
}

/* host can modify from 0-7 */
static void msdc_set_driving(struct mmc_host *host, struct msdc_cust *msdc_cap)
{
    ASSERT(host);
    ASSERT(msdc_cap);

    if (host && msdc_cap) {
	/*
	 * 0x10005740[19:16] --> PAD_MSDC0_DAT
	 * 0x10005750[7:4] --> PAD_MSDC0_CMD
	 * 0x10005750[11:8] --> PAD_MSDC0_CLK
	 *
	 */
	MSDC_SET_FIELD(DRV4_CFG_ADDR, (0x7 << 8), msdc_cap->clk_drv);  //CLK[11:8]
	MSDC_SET_FIELD(DRV4_CFG_ADDR, (0x7 << 4), msdc_cap->cmd_drv); //CMD[7:4]
	MSDC_SET_FIELD(DRV3_CFG_ADDR, (0x7 << 16), msdc_cap->dat_drv);  //DAT[19:16]
    }
}

static void msdc_set_pin_mode(struct mmc_host *host)
{
	ASSERT(host);
	/*
	 * GPIO86---0x260[20:18]=1---MSDC0_DAT0
	 * GPIO85---0x260[17:15]=1---MSDC0_DAT1
	 * GPIO84---0x260[14:12]=1---MSDC0_DAT2
	 * GPIO83---0x260[11:9]=1---MSDC0_DAT3
	 * GPIO82---0x260[8:6]=1---MSDC0_CLK
	 * GPIO81---0x260[5:3]=1---MSDC0_CMD
	 * GPIO80---0x260[2:0]=1---MSDC0_RSTB
	 * GPIO79---0x250[29:27]=1---MSDC0_DAT4
	 * GPIO78---0x250[26:24]=1---MSDC0_DAT5
	 * GPIO77---0x250[23:21]=1---MSDC0_DAT6
	 * GPIO76---0x250[20:18]=1---MSDC0_DAT7
	 */
	/* gpio register default value is ok, but We still set it */

	MSDC_SET_FIELD(GPIO_MODE7_ADDR, (0x7 << 27), 1);
	MSDC_SET_FIELD(GPIO_MODE7_ADDR, (0x7 << 24), 1);
	MSDC_SET_FIELD(GPIO_MODE7_ADDR, (0x7 << 21), 1);
	MSDC_SET_FIELD(GPIO_MODE7_ADDR, (0x7 << 18), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 18), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 15), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 12), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 9), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 6), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 3), 1);
	MSDC_SET_FIELD(GPIO_MODE8_ADDR, (0x7 << 0), 1);
}

static void msdc_gpio_and_pad_init(struct mmc_host *host)
{
	/*set smt enable*/
	msdc_set_smt(host, 1);

	/*set pupd enable*/
	msdc_pin_set(host);

	/* set gpio to msdc mode*/
	msdc_set_pin_mode(host);

	/*set driving*/
	msdc_set_driving(host, &msdc_cap[host->host_id]);

}
#endif

#ifndef FPGA_PLATFORM

static int pmic_config_interface(int a, int b, int c, int d)
{
    return 0;
}

static void msdc_set_card_pwr(struct mmc_host *host, int on)
{
    unsigned int ret;

    ret = pmic_config_interface(0xAB,0x7,0x7,4); /* VMCH=3.3V */

    if (ret == 0) {
        if (on) {
            ret = pmic_config_interface(0xAB,0x1,0x1,0); /* VMCH_EN=1 */
        } else {
            ret = pmic_config_interface(0xAB,0x0,0x1,0); /* VMCH_EN=0 */
        }
    }

    if (ret != 0) {
        dprintf(CRITICAL, "PMIC: Set MSDC Host Power Fail\n");
    } else {
        spin(3000);
    }
}

static void msdc_set_host_pwr(struct mmc_host *host, int on)
{
    unsigned int ret;

    ret = pmic_config_interface(0xA7,0x7,0x7,4); /* VMC=3.3V */

    if (ret == 0) {
        if (on) {
            ret = pmic_config_interface(0xA7,0x1,0x1,0); /* VMC_EN=1 */
        } else {
            ret = pmic_config_interface(0xA7,0x0,0x1,0); /* VMC_EN=0 */
        }
    }

    if (ret != 0) {
        dprintf(CRITICAL, "PMIC: Set MSDC Card Power Fail\n");
    }
}
#else
#define PWR_GPIO            (0x10001E84)
#define PWR_GPIO_EO         (0x10001E88)

#define PWR_MASK_EN         (0x1 << 8)
#define PWR_MASK_VOL_18     (0x1 << 9)
#define PWR_MASK_VOL_33     (0x1 << 10)
#define PWR_MASK_L4         (0x1 << 11)
#define PWR_MSDC_DIR        (PWR_MASK_EN | PWR_MASK_VOL_18 | PWR_MASK_VOL_33 | PWR_MASK_L4)

#define MSDC0_PWR_MASK_EN         (0x1 << 12)
#define MSDC0_PWR_MASK_VOL_18     (0x1 << 13)
#define MSDC0_PWR_MASK_VOL_33     (0x1 << 14)
#define MSDC0_PWR_MASK_L4         (0x1 << 15)
#define MSDC0_PWR_MSDC_DIR        (MSDC0_PWR_MASK_EN | MSDC0_PWR_MASK_VOL_18 | MSDC0_PWR_MASK_VOL_33 | MSDC0_PWR_MASK_L4)

static void msdc_clr_gpio(u32 bits)
{
    switch (bits) {
        case MSDC0_PWR_MASK_EN:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_EN),0);
            MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_EN),0);
            break;
        case MSDC0_PWR_MASK_VOL_18:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            break;
        case MSDC0_PWR_MASK_VOL_33:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 0);
            break;
        case MSDC0_PWR_MASK_L4:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_L4, 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 0);
            break;
        case PWR_MASK_EN:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_EN,0);
            MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_EN,0);
            break;
        case PWR_MASK_VOL_18:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            break;
        case PWR_MASK_VOL_33:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 0);
            break;
        case PWR_MASK_L4:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_L4, 0);
            MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_L4, 0);
            break;
        default:
            dprintf(CRITICAL, "[%s:%s]invalid value: 0x%x\n", __FILE__, __func__, bits);
            break;
    }
}

static void msdc_set_gpio(u32 bits)
{
    switch (bits) {
        case MSDC0_PWR_MASK_EN:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_EN,1);
            MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_EN,1);
            break;
        case MSDC0_PWR_MASK_VOL_18:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
            MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 1);
            break;
        case MSDC0_PWR_MASK_VOL_33:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
            MSDC_SET_FIELD(PWR_GPIO_EO, (MSDC0_PWR_MASK_VOL_18|MSDC0_PWR_MASK_VOL_33), 2);
            break;
        case MSDC0_PWR_MASK_L4:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    MSDC0_PWR_MASK_L4, 1);
            MSDC_SET_FIELD(PWR_GPIO_EO, MSDC0_PWR_MASK_L4, 1);
            break;
        case PWR_MASK_EN:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_EN,1);
            MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_EN,1);
            break;
        case PWR_MASK_VOL_18:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
            MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 1);
            break;
        case PWR_MASK_VOL_33:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
            MSDC_SET_FIELD(PWR_GPIO_EO, (PWR_MASK_VOL_18|PWR_MASK_VOL_33), 2);
            break;
        case PWR_MASK_L4:
            /* check for set before */
            MSDC_SET_FIELD(PWR_GPIO,    PWR_MASK_L4, 1);
            MSDC_SET_FIELD(PWR_GPIO_EO, PWR_MASK_L4, 1);
            break;
        default:
            dprintf(CRITICAL, "[%s:%s]invalid value: 0x%x\n", __FILE__, __func__, bits);
            break;
    }
}

static void msdc_set_card_pwr(struct mmc_host *host, int on)
{
    if (on) {
        msdc_set_gpio(MSDC0_PWR_MASK_VOL_18);
        msdc_set_gpio(MSDC0_PWR_MASK_L4);
        msdc_set_gpio(MSDC0_PWR_MASK_EN);
    } else {
        msdc_clr_gpio(MSDC0_PWR_MASK_EN);
        msdc_clr_gpio(MSDC0_PWR_MASK_VOL_18);
        msdc_clr_gpio(MSDC0_PWR_MASK_L4);
    }
    spin(10000);
}

static void msdc_set_host_level_pwr(struct mmc_host *host, u32 level)
{
    msdc_clr_gpio(PWR_MASK_VOL_18);
    msdc_clr_gpio(PWR_MASK_VOL_33);

    if (level) {
        msdc_set_gpio(PWR_MASK_VOL_18);
    } else {
        msdc_set_gpio(PWR_MASK_VOL_33);
    }
    msdc_set_gpio(PWR_MASK_L4);
}

static void msdc_set_host_pwr(struct mmc_host *host, int on)
{
    msdc_set_host_level_pwr(host, 0);
}
#endif

static void msdc_set_startbit(struct mmc_host *host, u8 start_bit)
{
    addr_t base = host->base;
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;

    /* set start bit */
    MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_START_BIT, start_bit);
    priv->start_bit = start_bit;
    dprintf(INFO, "start bit = %d, MSDC_CFG[0x%x]\n", start_bit, MSDC_READ32(MSDC_CFG));
}

#define TYPE_CMD_RESP_EDGE      (0)
#define TYPE_WRITE_CRC_EDGE     (1)
#define TYPE_READ_DATA_EDGE     (2)
#define TYPE_WRITE_DATA_EDGE    (3)

static void msdc_set_smpl(struct mmc_host *host, u8 HS400, u8 mode, u8 type)
{
    addr_t base = host->base;
    int i=0;
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;
    static u8 read_data_edge[8] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING,
                                   MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING
                                  };
    static u8 write_data_edge[4] = {MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING, MSDC_SMPL_RISING};

    switch (type) {
        case TYPE_CMD_RESP_EDGE:
            if (HS400) {
                // eMMC5.0 only output resp at CLK pin, so no need to select DS pin
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_PADCMD_LATCHCK, 0); //latch cmd resp at CLK pin
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CMD_RESP_SEL, 0);//latch cmd resp at CLK pin
            }

            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, mode);
                priv->rsmpl = mode;
            } else {
                dprintf(CRITICAL, "[%s]: invalid resp parameter: HS400=%d, type=%d, mode=%d\n", __func__, HS400, type, mode);
            }
            break;

        case TYPE_WRITE_CRC_EDGE:
            if (HS400) {
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 1);//latch write crc status at DS pin
            } else {
                MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin
            }

            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
                if (HS400) {
                    MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_EDGE, mode);
                } else {
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
                }
                priv->wdsmpl = mode;
            } else if (mode == MSDC_SMPL_SEPERATE && !HS400) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D0SPL, write_data_edge[0]); //only dat0 is for write crc status.
                priv->wdsmpl = mode;
            } else {
                dprintf(CRITICAL, "[%s]: invalid crc parameter: HS400=%d, type=%d, mode=%d\n", __func__, HS400, type, mode);
            }
            break;

        case TYPE_READ_DATA_EDGE:
            if (HS400) {
                msdc_set_startbit(host, START_AT_RISING_AND_FALLING); //for HS400, start bit is output both on rising and falling edge
                priv->start_bit = START_AT_RISING_AND_FALLING;
            } else {
                msdc_set_startbit(host, START_AT_RISING); //for the other mode, start bit is only output on rising edge. but DDR50 can try falling edge if error casued by pad delay
                priv->start_bit = START_AT_RISING;
            }
            if (mode == MSDC_SMPL_RISING || mode == MSDC_SMPL_FALLING) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 0);
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, mode);
                priv->rdsmpl = mode;
            } else if (mode == MSDC_SMPL_SEPERATE) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL_SEL, 1);
                for (i=0; i<8; i++) {
                    MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_R_D0SPL << i), read_data_edge[i]);
                }
                priv->rdsmpl = mode;
            } else {
                dprintf(CRITICAL, "[%s]: invalid read parameter: HS400=%d, type=%d, mode=%d\n", __func__, HS400, type, mode);
            }
            break;

        case TYPE_WRITE_DATA_EDGE:
            MSDC_SET_FIELD(EMMC50_CFG0, MSDC_EMMC50_CFG_CRC_STS_SEL, 0);//latch write crc status at CLK pin

            if (mode == MSDC_SMPL_RISING|| mode == MSDC_SMPL_FALLING) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 0);
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL, mode);
                priv->wdsmpl = mode;
            } else if (mode == MSDC_SMPL_SEPERATE) {
                MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_W_D_SMPL_SEL, 1);
                for (i=0; i<4; i++) {
                    MSDC_SET_FIELD(MSDC_IOCON, (MSDC_IOCON_W_D0SPL << i), write_data_edge[i]);//dat0~4 is for SDIO card.
                }
                priv->wdsmpl = mode;
            } else {
                dprintf(CRITICAL, "[%s]: invalid write parameter: HS400=%d, type=%d, mode=%d\n", __func__, HS400, type, mode);
            }
            break;

        default:
            dprintf(CRITICAL, "[%s]: invalid parameter: HS400=%d, type=%d, mode=%d\n", __func__, HS400, type, mode);
            break;
    }
}

void msdc_set_timeout(struct mmc_host *host, u32 ns, u32 clks)
{
    addr_t base = host->base;
    u32 timeout, clk_ns;
    u32 mode = 0;

    if (host->cur_bus_clk == 0) {
        timeout = 0;
    } else {
        clk_ns  = 1000000000UL / host->cur_bus_clk;
        timeout = (ns + clk_ns - 1) / clk_ns + clks;
        timeout = (timeout + (1 << 20) - 1) >> 20; /* in 1048576 sclk cycle unit */
        MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, mode);
        timeout = mode >= 2 ? timeout * 2 : timeout; //DDR mode will double the clk cycles for data timeout
        timeout = timeout > 1 ? timeout - 1 : 0;
        timeout = timeout > 255 ? 255 : timeout;
    }
    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_DTOC, timeout);
    dprintf(INFO, "[MSDC] Set read data timeout: %dns %dclks -> %d (65536 sclk cycles)\n",
            ns, clks, timeout + 1);
}

void msdc_set_autocmd(struct mmc_host *host, int cmd)
{
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;

    priv->autocmd = cmd;
}

int msdc_get_autocmd(struct mmc_host *host)
{
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;

    return priv->autocmd;
}

static void msdc_abort(struct mmc_host *host)
{
    addr_t base = host->base;

    dprintf(CRITICAL, "[MSDC] Abort: MSDC_FIFOCS=%xh MSDC_PS=%xh SDC_STS=%xh\n",
            MSDC_READ32(MSDC_FIFOCS), MSDC_READ32(MSDC_PS), MSDC_READ32(SDC_STS));
    /* reset controller */
    MSDC_RESET();

    /* clear fifo */
    MSDC_CLR_FIFO();

    /* make sure txfifo and rxfifo are empty */
    if (MSDC_TXFIFOCNT() != 0 || MSDC_RXFIFOCNT() != 0) {
        dprintf(INFO, "[MSDC] Abort: TXFIFO(%d), RXFIFO(%d) != 0\n",
                MSDC_TXFIFOCNT(), MSDC_RXFIFOCNT());
    }

    /* clear all interrupts */
    MSDC_CLR_INT();
}

static int msdc_get_card_status(struct mmc_host *host, u32 *status)
{
    int err;
    struct mmc_command cmd;

    cmd.opcode  = MMC_CMD_SEND_STATUS;
    cmd.arg     = host->card->rca << 16;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = CMD_RETRIES;
    cmd.timeout = CMD_TIMEOUT;

    err = msdc_send_cmd(host, &cmd);
    if (!err)
        err = msdc_wait_cmd_done(host, &cmd);

    if (err == MMC_ERR_NONE)
        *status = cmd.resp[0];

    return err;
}

int msdc_abort_handler(struct mmc_host *host, int abort_card)
{
    struct mmc_command stop;
    u32 status = 0;
    u32 state = 0;

    while (state != 4) { // until status to "tran"
        msdc_abort(host);
        if (msdc_get_card_status(host, &status)) {
            dprintf(CRITICAL, "Get card status failed\n");
            return 1;
        }
        state = R1_CURRENT_STATE(status);
        dprintf(INFO, "check card state<%d>\n", state);
        if (state == 5 || state == 6) {
            dprintf(INFO, "state<%d> need cmd12 to stop\n", state);
            if (abort_card) {
                stop.opcode  = MMC_CMD_STOP_TRANSMISSION;
                stop.rsptyp  = RESP_R1B;
                stop.arg     = 0;
                stop.retries = CMD_RETRIES;
                stop.timeout = CMD_TIMEOUT;
                msdc_send_cmd(host, &stop);
                msdc_wait_cmd_done(host, &stop); // don't tuning
            } else if (state == 7) {  // busy in programing
                dprintf(INFO, "state<%d> card is busy\n", state);
                spin(100000);
            } else if (state != 4) {
                dprintf(INFO, "state<%d> ??? \n", state);
                return 1;
            }
        }
    }
    msdc_abort(host);
    return 0;
}

static u32 msdc_intr_wait(struct mmc_host *host, u32 intrs)
{
    u32 sts = 0;
    u32 tmo = UINT_MAX;
    int ret = 0;

    /* warning that interrupts are not enabled */
    ret = event_wait_timeout(&msdc_int_event, tmo);
    if (ret != 0) {
        addr_t base = host->base;
        dprintf(CRITICAL, "[%s]: failed to get event INT=0x%x\n",
                __func__, MSDC_READ32(MSDC_INT));
        g_int_status = 0;
        return 0;
    }

    sts = g_int_status;
    g_int_status = 0;

    if (~intrs & sts)
        dprintf(CRITICAL, "msdc_intr_wait Unexpected INT(0x%x)\n", ~intrs & sts);

    return sts;
}

static enum handler_return msdc_interrupt_handler(void *arg)
{
    struct mmc_host *host = arg;
    addr_t base = host->base;

    /* Save & Clear the interrupt */
    g_int_status = MSDC_READ32(MSDC_INT);
    MSDC_WRITE32(MSDC_INT, g_int_status);
    MSDC_WRITE32(MSDC_INTEN, 0);
    host->intr_mask = 0;

    /* MUST BE *false*! otherwise, schedule in interrupt */
    event_signal(&msdc_int_event, false);

    return INT_RESCHEDULE;
}

static int msdc_send_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;
    addr_t base   = host->base;
    u32 opcode = cmd->opcode;
    u32 rsptyp = cmd->rsptyp;
    u32 rawcmd;
    u32 error = MMC_ERR_NONE;

    /* rawcmd :
     * vol_swt << 30 | auto_cmd << 28 | blklen << 16 | go_irq << 15 |
     * stop << 14 | rw << 13 | dtype << 11 | rsptyp << 7 | brk << 6 | opcode
     */
    rawcmd = (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)) |
             msdc_rsp[rsptyp] << 7 | host->blklen << 16;

    if (opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK) {
        rawcmd |= ((2 << 11) | (1 << 13));
        if (priv->autocmd & MSDC_AUTOCMD12) {
            rawcmd |= (1 << 28);
        } else if (priv->autocmd & MSDC_AUTOCMD23) {
            rawcmd |= (2 << 28);
        }
    } else if (opcode == MMC_CMD_WRITE_BLOCK || opcode == MMC_CMD50) {
        rawcmd |= ((1 << 11) | (1 << 13));
    } else if (opcode == MMC_CMD_READ_MULTIPLE_BLOCK) {
        rawcmd |= (2 << 11);
        if (priv->autocmd & MSDC_AUTOCMD12) {
            rawcmd |= (1 << 28);
        } else if (priv->autocmd & MSDC_AUTOCMD23) {
            rawcmd |= (2 << 28);
        }
    } else if (opcode == MMC_CMD_READ_SINGLE_BLOCK ||
               opcode == SD_ACMD_SEND_SCR ||
               opcode == SD_CMD_SWITCH ||
               opcode == MMC_CMD_SEND_EXT_CSD ||
               opcode == MMC_CMD_SEND_WRITE_PROT ||
               opcode == MMC_CMD_SEND_WRITE_PROT_TYPE ||
               opcode == MMC_CMD21) {
        rawcmd |= (1 << 11);
    } else if (opcode == MMC_CMD_STOP_TRANSMISSION) {
        rawcmd |= (1 << 14);
        rawcmd &= ~(0x0FFF << 16);
    } else if (opcode == SD_IO_RW_EXTENDED) {
        if (cmd->arg & 0x80000000)  /* R/W flag */
            rawcmd |= (1 << 13);
        if ((cmd->arg & 0x08000000) && ((cmd->arg & 0x1FF) > 1))
            rawcmd |= (2 << 11); /* multiple block mode */
        else
            rawcmd |= (1 << 11);
    } else if (opcode == SD_IO_RW_DIRECT) {
        if ((cmd->arg & 0x80000000) && ((cmd->arg >> 9) & 0x1FFFF))/* I/O abt */
            rawcmd |= (1 << 14);
    } else if (opcode == SD_CMD_VOL_SWITCH) {
        rawcmd |= (1 << 30);
    } else if (opcode == SD_CMD_SEND_TUNING_BLOCK) {
        rawcmd |= (1 << 11); /* CHECKME */
        if (priv->autocmd & MSDC_AUTOCMD19)
            rawcmd |= (3 << 28);
    } else if (opcode == MMC_CMD_GO_IRQ_STATE) {
        rawcmd |= (1 << 15);
    } else if (opcode == MMC_CMD_WRITE_DAT_UNTIL_STOP) {
        rawcmd |= ((1<< 13) | (3 << 11));
    } else if (opcode == MMC_CMD_READ_DAT_UNTIL_STOP) {
        rawcmd |= (3 << 11);
    }

    dprintf(INFO, "+[MSDC%d] CMD(%d): ARG(0x%x), RAW(0x%x), BLK_NUM(0x%x) RSP(%d)\n",host->host_id,
            (opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT)), cmd->arg, rawcmd,
            MSDC_READ32(SDC_BLK_NUM), rsptyp);

    while (SDC_IS_CMD_BUSY());
    if ((rsptyp == RESP_R1B) || (opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK) ||
            opcode == MMC_CMD_WRITE_BLOCK || opcode == MMC_CMD_READ_MULTIPLE_BLOCK ||
            opcode == MMC_CMD_READ_SINGLE_BLOCK)
        while (SDC_IS_BUSY());

    SDC_SEND_CMD(rawcmd, cmd->arg);

end:
    cmd->error = error;

    return error;
}

static int msdc_wait_cmd_done(struct mmc_host *host, struct mmc_command *cmd)
{
    addr_t base   = host->base;
    u32 rsptyp = cmd->rsptyp;
    u32 status;
    u32 opcode = (cmd->opcode & ~(SD_CMD_BIT | SD_CMD_APP_BIT));
    u32 error = MMC_ERR_NONE;
    u32 wints = MSDC_INT_CMDTMO | MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR |
                MSDC_INT_ACMDRDY | MSDC_INT_ACMDCRCERR | MSDC_INT_ACMDTMO;
    u32 *resp = &cmd->resp[0];
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;

    while (1) {
        /* Wait for interrupt coming */
        while (((status = MSDC_READ32(MSDC_INT)) & wints) == 0);
        MSDC_WRITE32(MSDC_INT, (status & wints));
        if (~wints & status)
            dprintf(CRITICAL, "msdc_wait_cmd_done Unexpected INT(0x%x)\n",
                    ~wints & status);

        if (status & MSDC_INT_CMDRDY)
            break;
        else if (status & MSDC_INT_RSPCRCERR) {
            if (opcode != MMC_CMD21)
                dprintf(CRITICAL, "[MSDC] cmd%d CRCERR! (0x%x)\n", opcode, status);
            error = MMC_ERR_BADCRC;
            goto err;
        } else if (status & MSDC_INT_CMDTMO) {
            dprintf(CRITICAL, "[MSDC] cmd%d TMO! (0x%x)\n", opcode, status);
            error = MMC_ERR_TIMEOUT;
            goto err;
        } else if (priv->autocmd & MSDC_AUTOCMD23) {
            if (status & MSDC_INT_ACMDRDY)
                /* Autocmd rdy is set prior to cmd rdy */
                continue;
            else if (status & MSDC_INT_ACMDCRCERR) {
                dprintf(CRITICAL, "[MSDC] autocmd23 CRCERR! (0x%x)\n", status);
                error = MMC_ERR_ACMD_RSPCRC;
                goto err;
            } else if (status & MSDC_INT_ACMDTMO) {
                dprintf(CRITICAL, "[MSDC] autocmd23 TMO! (0x%x)\n", status);
                error = MMC_ERR_ACMD_TIMEOUT;
                goto err;
            }
        } else {
            dprintf(CRITICAL, "[MSDC] cmd%d UNEXPECT status! (0x%x)\n",
                    opcode, status);
            error = MMC_ERR_UNEXPECT;
            goto err;
        }
    }

    switch (rsptyp) {
        case RESP_NONE:
            dprintf(INFO, "-[MSDC] CMD(%d): RSP(%d)\n",
                    opcode, rsptyp);
            break;
        case RESP_R2:
            *resp++ = MSDC_READ32(SDC_RESP3);
            *resp++ = MSDC_READ32(SDC_RESP2);
            *resp++ = MSDC_READ32(SDC_RESP1);
            *resp++ = MSDC_READ32(SDC_RESP0);
            dprintf(INFO, "-[MSDC] CMD(%d): RSP(%d) = 0x%x 0x%x 0x%x 0x%x\n",
                    opcode, cmd->rsptyp, cmd->resp[0], cmd->resp[1],
                    cmd->resp[2], cmd->resp[3]);
            break;
        default: /* Response types 1, 3, 4, 5, 6, 7(1b) */
            cmd->resp[0] = MSDC_READ32(SDC_RESP0);
            dprintf(INFO, "-[MSDC] CMD(%d): RSP(%d) = 0x%x\n",
                    opcode, cmd->rsptyp, cmd->resp[0]);
            break;
    }

err:
    if (rsptyp == RESP_R1B)
        while ((MSDC_READ32(MSDC_PS) & MSDC_PS_DAT0) != MSDC_PS_DAT0);

    cmd->error = error;

    return error;
}

int msdc_cmd(struct mmc_host *host, struct mmc_command *cmd)
{
    int err;

    err = msdc_send_cmd(host, cmd);
    if (err != MMC_ERR_NONE)
        return err;

    err = msdc_wait_cmd_done(host, cmd);

    if (err && cmd->opcode != MMC_CMD21) {
        addr_t base = host->base;
        u32 tmp = MSDC_READ32(SDC_CMD);

        /* check if data is used by the command or not */
        if (tmp & SDC_CMD_DTYP) {
            if (msdc_abort_handler(host, 1)) {
                dprintf(CRITICAL, "[MSDC] abort failed\n");
            }
        }

        if (cmd->opcode == MMC_CMD_APP_CMD ||
                cmd->opcode == SD_CMD_SEND_IF_COND) {
            if (err ==  MMC_ERR_TIMEOUT)
                return err;
        }

        err = msdc_tune_cmdrsp(host, cmd);
    }

    return err;
}

#ifdef MSDC_USE_DMA_MODE
static void msdc_flush_membuf(void *buf, u32 len)
{
    arch_clean_invalidate_cache_range((addr_t)buf,len);
}

static int msdc_dma_wait_done(struct mmc_host *host, struct mmc_command *cmd)
{
    addr_t base = host->base;
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;
    u32 status;
    u32 error = MMC_ERR_NONE;
    u32 wints = MSDC_INT_XFER_COMPL | MSDC_INT_DATTMO | MSDC_INT_DATCRCERR |
                MSDC_INT_DXFER_DONE | MSDC_INT_DMAQ_EMPTY |
                MSDC_INT_ACMDRDY | MSDC_INT_ACMDTMO | MSDC_INT_ACMDCRCERR |
                MSDC_INT_CMDRDY | MSDC_INT_CMDTMO | MSDC_INT_RSPCRCERR;

    /* Deliver it to irq handler */
    host->intr_mask = wints;

    do {
        status = msdc_intr_wait(host, wints);

        if (status & MSDC_INT_XFER_COMPL) {
            if (mmc_op_multi(cmd->opcode) && (priv->autocmd & MSDC_AUTOCMD12)) {
                /* acmd rdy should be checked after xfer_done been held */
                if (status & MSDC_INT_ACMDRDY) {
                    break;
                } else if (status & MSDC_INT_ACMDTMO) {
                    dprintf(CRITICAL, "[MSDC] ACMD12 timeout(%xh)\n", status);
                    error = MMC_ERR_ACMD_TIMEOUT;
                    goto end;
                } else if (status & MSDC_INT_ACMDCRCERR) {
                    dprintf(CRITICAL, "[MSDC] ACMD12 CRC error(%xh)\n", status);
                    error = MMC_ERR_ACMD_RSPCRC;
                    goto end;
                }
            } else
                break;
        }

        if (status == 0 || status & MSDC_INT_DATTMO) {
            dprintf(CRITICAL, "[MSDC] DMA DAT timeout(%xh)\n", status);
            error = MMC_ERR_TIMEOUT;
            goto end;
        } else if (status & MSDC_INT_DATCRCERR) {
            dprintf(CRITICAL, "[MSDC] DMA DAT CRC error(%xh)\n", status);
            error = MMC_ERR_BADCRC;
            goto end;
        } else {
            dprintf(CRITICAL, "[MSDC] Unexpect status(0x%x)\n", status);
            error = MMC_ERR_UNEXPECT;
            goto end;
        }
    } while (1);

end:
    if (error)
        MSDC_RESET();

    return error;
}

int msdc_dma_transfer(struct mmc_host *host, struct mmc_data *data)
{
    addr_t base = host->base;
    int err;
    paddr_t pa;

    /* Set dma timeout */
    msdc_set_timeout(host, data->timeout * 1000000, 0);
    /* DRAM address */
#if WITH_KERNEL_VM
    pa = kvaddr_to_paddr(data->buf);
#else
    pa = (paddr_t)(data->buf);
#endif
    if (sizeof(pa) > 4)
        dprintf(INFO, "[MSDC] WARN: 64bit physical address!\n");
    MSDC_WRITE32(MSDC_DMA_SA, (u32)pa);
    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_BURSTSZ, MSDC_DMA_BURST_64B);
    /* BASIC_DMA mode */
    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_MODE, 0);
    /* This is the last buffer */
    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_LASTBUF, 1);
    /* Total transfer size */
    MSDC_WRITE32(MSDC_DMA_LEN, data->blks * host->blklen);
    /* Set interrupts bit */
    MSDC_SET_BIT32(MSDC_INTEN,
                   MSDC_INT_XFER_COMPL | MSDC_INT_DATTMO | MSDC_INT_DATCRCERR);
    /* Clean & Invalidate cache */
    msdc_flush_membuf(data->buf, data->blks * host->blklen);
    /* Trigger DMA start */
    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_START, 1);
    /* wait DMA transferring done */
    err = msdc_dma_wait_done(host, data->cmd);
    msdc_flush_membuf(data->buf, data->blks * host->blklen);
    if (err) {
        dprintf(CRITICAL, "[MSDC] DMA failed! err(%d)\n", err);
        if (msdc_abort_handler(host, 1)) {
            dprintf(CRITICAL, "[MSDC] eMMC cannot back to TRANS mode!\n");
            return MMC_ERR_FAILED;
        }
    }

    /* Check DMA status and stop DMA transfer */
    MSDC_SET_FIELD(MSDC_DMA_CTRL, MSDC_DMA_CTRL_STOP, 1);
    while (MSDC_READ32(MSDC_DMA_CFG) & MSDC_DMA_CFG_STS);

    return err;
}

static int msdc_dma_rw(struct mmc_host *host, u8 *buf, u32 blkaddr, u32 nblks, bool rd)
{
    int multi, err;
    struct mmc_command cmd;
    struct mmc_data data;
    addr_t base = host->base;

    ASSERT(nblks <= host->max_phys_segs);

    dprintf(INFO, "[MSDC] %s data %d blks %s 0x%x\n",
            rd ? "Read" : "Write", nblks, rd ? "from" : "to", blkaddr);

    multi = nblks > 1 ? 1 : 0;
    /* DMA and block number _MUST_BE_ set prior to issuing command */
    MSDC_DMA_ON;
    MSDC_WRITE32(SDC_BLK_NUM, nblks);

    /* send read command */
    if (rd)
        cmd.opcode =
            multi ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK;
    else
        cmd.opcode = multi ? MMC_CMD_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK;
    cmd.arg = blkaddr;
    cmd.rsptyp  = RESP_R1;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;

    err = msdc_cmd(host, &cmd);
    if (err != MMC_ERR_NONE)
        return err;

    data.cmd = &cmd;
    data.blks = nblks;
    data.buf = buf;
    if (rd)
        data.timeout = 100;
    else
        data.timeout = 250;

    err = msdc_dma_transfer(host, &data);
    MSDC_DMA_OFF;

    return err;
}

static int msdc_dma_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks)
{
    return msdc_dma_rw(host, dst, src, nblks, true);
}

static int msdc_dma_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks)
{
    return msdc_dma_rw(host, src, dst, nblks, false);
}
#else
static int msdc_pio_read_word(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = MMC_ERR_NONE;
    addr_t base = host->base;
    u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
    //u32 timeout = 100000;
    u32 status;
    u32 totalsz = size;
    u8  done = 0;
    u8 *u8ptr;
    u32 dcrc=0;

    while (1) {
        status = MSDC_READ32(MSDC_INT);
        MSDC_WRITE32(MSDC_INT, status);
        if (status & ~ints)
            dprintf(CRITICAL, "msdc_pio_read_word Unexpected INT(0x%x)\n", status);
        if (status & MSDC_INT_DATCRCERR) {
            MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
            dprintf(CRITICAL, "[MSDC] DAT CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
                    status, size, totalsz, MSDC_RXFIFOCNT(),dcrc);
            err = MMC_ERR_BADCRC;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            dprintf(CRITICAL, "[MSDC] DAT TMO error (0x%x), Left: %d/%d bytes, RXFIFO:%d\n",
                    status, size, totalsz, MSDC_RXFIFOCNT());
            err = MMC_ERR_TIMEOUT;
            break;
        } else if (status & MSDC_INT_ACMDCRCERR) {
            MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
            dprintf(CRITICAL, "[MSDC] AUTOCMD CRC error (0x%x), Left:%d/%d bytes, RXFIFO:%d,dcrc:0x%x\n",
                    status, size, totalsz, MSDC_RXFIFOCNT(),dcrc);
            err = MMC_ERR_ACMD_RSPCRC;
            break;
        } else if (status & MSDC_INT_XFER_COMPL) {
            done = 1;
        }

        if (size == 0 && done)
            break;

        /* Note. RXFIFO count would be aligned to 4-bytes alignment size */
        if ((size >=  MSDC_FIFO_THD) && (MSDC_RXFIFOCNT() >= MSDC_FIFO_THD)) {
            int left = MSDC_FIFO_THD >> 2;
            do {
                *ptr++ = MSDC_FIFO_READ32();
            } while (--left);
            size -= MSDC_FIFO_THD;
            dprintf(INFO, "[MSDC] Read %d bytes, RXFIFOCNT: %d,  Left: %d/%d\n",
                    MSDC_FIFO_THD, MSDC_RXFIFOCNT(), size, totalsz);
        } else if ((size < MSDC_FIFO_THD) && MSDC_RXFIFOCNT() >= size) {
            while (size) {
                if (size > 3) {
                    *ptr++ = MSDC_FIFO_READ32();
                } else {
                    u8ptr = (u8 *)ptr;
                    while (size --)
                        *u8ptr++ = MSDC_FIFO_READ8();
                }
            }
            dprintf(INFO, "[MSDC] Read left bytes, RXFIFOCNT: %d, Left: %d/%d\n",
                    MSDC_RXFIFOCNT(), size, totalsz);
        }
    }

    return err;
}

static int msdc_pio_read(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = msdc_pio_read_word(host, (u32 *)ptr, size);

    if (err != MMC_ERR_NONE) {
        msdc_abort(host); /* reset internal fifo and state machine */
        dprintf(CRITICAL, "[MSDC] %d-bit PIO Read Error (%d)\n", 32, err);
    }

    return err;
}

static int msdc_pio_write_word(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = MMC_ERR_NONE;
    addr_t base = host->base;
    u32 ints = MSDC_INT_DATCRCERR | MSDC_INT_DATTMO | MSDC_INT_XFER_COMPL;
    //u32 timeout = 250000;
    u32 status;
    u8 *u8ptr;
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;

    while (1) {
        status = MSDC_READ32(MSDC_INT);
        MSDC_WRITE32(MSDC_INT, status);
        if (status & ~ints) {
            dprintf(CRITICAL, "msdc_pio_write_word Unexpected INT(0x%x)\n", status);
        }
        if (status & MSDC_INT_DATCRCERR) {
            dprintf(CRITICAL, "[MSDC] DAT CRC error (0x%x), Left DAT: %d bytes\n",
                    status, size);
            err = MMC_ERR_BADCRC;
            break;
        } else if (status & MSDC_INT_DATTMO) {
            dprintf(CRITICAL, "[MSDC] DAT TMO error (0x%x), Left DAT: %d bytes, MSDC_FIFOCS=%xh\n",
                    status, size, MSDC_READ32(MSDC_FIFOCS));
            err = MMC_ERR_TIMEOUT;
            break;
        } else if (status & MSDC_INT_ACMDCRCERR) {
            dprintf(CRITICAL, "[MSDC] AUTO CMD CRC error (0x%x), Left DAT: %d bytes\n",
                    status, size);
            err = MMC_ERR_ACMD_RSPCRC;
            break;
        } else if (status & MSDC_INT_XFER_COMPL) {
            if (size == 0) {
                dprintf(INFO, "[MSDC] all data flushed to card\n");
                break;
            } else
                dprintf(INFO, "[MSDC]<CHECKME> XFER_COMPL before all data written\n");
        }

        if (size == 0)
            continue;

        if (size >= MSDC_FIFO_SZ) {
            if (MSDC_TXFIFOCNT() == 0) {
                int left = MSDC_FIFO_SZ >> 2;
                do {
                    MSDC_FIFO_WRITE32(*ptr);
                    ptr++;
                } while (--left);
                size -= MSDC_FIFO_SZ;
            }
        } else if (size < MSDC_FIFO_SZ && MSDC_TXFIFOCNT() == 0) {
            while (size ) {
                if (size > 3) {
                    MSDC_FIFO_WRITE32(*ptr);
                    ptr++;
                    size -= 4;
                } else {
                    u8ptr = (u8 *)ptr;
                    while (size --) {
                        MSDC_FIFO_WRITE8(*u8ptr);
                        u8ptr++;
                    }
                }
            }
        }
    }

    return err;
}

static int msdc_pio_write(struct mmc_host *host, u32 *ptr, u32 size)
{
    int err = msdc_pio_write_word(host, (u32 *)ptr, size);

    if (err != MMC_ERR_NONE) {
        msdc_abort(host); /* reset internal fifo and state machine */
        dprintf(CRITICAL, "[MSDC] PIO Write Error (%d)\n", err);
    }

    return err;
}

static int msdc_pio_bread(struct mmc_host *host, u8 *dst, u32 src, u32 nblks)
{
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;
    addr_t base = host->base;
    u32 blksz = host->blklen;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    int multi;
    struct mmc_command cmd;
    struct mmc_command stop;
    u32 *ptr = (u32 *)dst;

    dprintf(INFO, "[MSDC] Read data %d bytes from 0x%x\n", nblks * blksz, src);

    multi = nblks > 1 ? 1 : 0;

    MSDC_WRITE32(SDC_BLK_NUM, nblks);
    msdc_set_timeout(host, 100000000, 0);

    /* send read command */
    cmd.opcode  = multi ? MMC_CMD_READ_MULTIPLE_BLOCK : MMC_CMD_READ_SINGLE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = src;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_read(host, (u32 *)ptr, nblks * blksz);

done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
            dprintf(CRITICAL, "[MSDC] Read data error (%d)\n", derr);
            if (msdc_abort_handler(host, 1))
                dprintf(CRITICAL, "[MSDC] abort failed\n");
        } else {
            dprintf(CRITICAL, "[MSDC] Read error (%d)\n", err);
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}

static int msdc_pio_bwrite(struct mmc_host *host, u32 dst, u8 *src, u32 nblks)
{
    msdc_priv_t *priv = (msdc_priv_t *)host->priv;
    addr_t base = host->base;
    int err = MMC_ERR_NONE, derr = MMC_ERR_NONE;
    int multi;
    u32 blksz = host->blklen;
    struct mmc_command cmd;
    struct mmc_command stop;
    u32 *ptr = (u32 *)src;

    dprintf(CRITICAL, "[MSDC] Write data %d bytes to 0x%x\n", nblks * blksz, dst);

    multi = nblks > 1 ? 1 : 0;

    MSDC_WRITE32(SDC_BLK_NUM, nblks);

    /* No need since MSDC always waits 8 cycles for write data timeout */

    /* send write command */
    cmd.opcode  = multi ? MMC_CMD_WRITE_MULTIPLE_BLOCK : MMC_CMD_WRITE_BLOCK;
    cmd.rsptyp  = RESP_R1;
    cmd.arg     = dst;
    cmd.retries = 0;
    cmd.timeout = CMD_TIMEOUT;
    err = msdc_cmd(host, &cmd);

    if (err != MMC_ERR_NONE)
        goto done;

    err = derr = msdc_pio_write(host, (u32 *)ptr, nblks * blksz);

done:
    if (err != MMC_ERR_NONE) {
        if (derr != MMC_ERR_NONE) {
            dprintf(CRITICAL, "[MSDC] Write data error (%d)\n", derr);
            if (msdc_abort_handler(host, 1))
                dprintf(CRITICAL, "[MSDC] abort failed\n");
        } else {
            dprintf(CRITICAL, "[MSDC] Write error (%d)\n", err);
        }
    }
    return (derr == MMC_ERR_NONE) ? err : derr;
}
#endif


static void msdc_config_clksrc(struct mmc_host *host, u32 clksrc, u32 hclksrc)
{
    // modify the clock
    ASSERT(host);
    /*
     * For MT2712, MSDC0 use 400Mhz(MSDCPLL) source clock
     */
    host->clksrc  = clksrc;
    host->hclksrc = hclksrc;
#ifndef FPGA_PLATFORM
    if (host->host_id == 0)
        host->clk     = 400 * 1000 * 1000;
    else
        host->clk     = 200 * 1000 * 1000;
#else
    host->clk = MSDC_OP_SCLK;
#endif

    /* Chaotian, may need update this part of code */
    dprintf(INFO, "[info][%s] pll_clk %u (%uMHz), pll_hclk %u\n",
            __func__, host->clksrc, host->clk/1000000, host->hclksrc);
}

void msdc_config_clock(struct mmc_host *host, int state, u32 hz)
{
    addr_t base = host->base;
    u32 mode = 0;
    u32 div;
    u32 sclk;
    u32 u4buswidth=0;

    if (hz >= host->f_max) {
        hz = host->f_max;
    } else if (hz < host->f_min) {
        hz = host->f_min;
    }

    msdc_config_clksrc(host, host->clksrc, host->hclksrc);
    MSDC_CLR_BIT32(MSDC_CFG, MSDC_CFG_CKMOD_HS400);
    MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);

    if (state & MMC_STATE_HS400) {
        mode = 0x3;
        div = 0; /* we let hs400 mode fixed at 200Mhz */
        sclk = host->clk >> 1;
        MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKMOD_HS400);
        MSDC_CLR_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
    } else if (state&MMC_STATE_DDR) {
        mode = 0x2; /* ddr mode and use divisor */
        if (hz >= (host->clk >> 2)) {
            div  = 0;              /* mean div = 1/2 */
            sclk = host->clk >> 2; /* sclk = clk/div/2. 2: internal divisor */
        } else {
            div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->clk >> 2) / div;
            div  = (div >> 1);     /* since there is 1/2 internal divisor */
        }
    } else if (hz >= host->clk) {
        mode = 0x1; /* no divisor and divisor is ignored */
        div  = 0;
        sclk = host->clk;
    } else {
        mode = 0x0; /* use divisor */
        if (hz >= (host->clk >> 1)) {
            div  = 0;              /* mean div = 1/2 */
            sclk = host->clk >> 1; /* sclk = clk / 2 */
        } else {
            div  = (host->clk + ((hz << 2) - 1)) / (hz << 2);
            sclk = (host->clk >> 2) / div;
        }
    }
    host->cur_bus_clk = sclk;

    /* set clock mode and divisor */
    MSDC_SET_FIELD(MSDC_CFG, (MSDC_CFG_CKMOD |MSDC_CFG_CKDIV),\
                   (mode << 12) | div);
    /* wait clock stable */
    while (!(MSDC_READ32(MSDC_CFG) & MSDC_CFG_CKSTB));

    MSDC_GET_FIELD(SDC_CFG,SDC_CFG_BUSWIDTH,u4buswidth);

    dprintf(INFO,
            "[MSDC] SET_CLK(%dkHz): SCLK(%dkHz) MODE(%d) DIV(%d) DS(%d) RS(%d) buswidth(%s)\n",
            hz/1000, sclk/1000, mode, div, msdc_cap[host->host_id].data_edge,
            msdc_cap[host->host_id].cmd_edge,
            (u4buswidth == 0) ?
            "1-bit" : (u4buswidth == 1) ?
            "4-bits" : (u4buswidth == 2) ?
            "8-bits" : "undefined");
}

void msdc_config_bus(struct mmc_host *host, u32 width)
{
    u32 val,mode, div;
    addr_t base = host->base;

    val = (width == HOST_BUS_WIDTH_8) ? 2 :
          (width == HOST_BUS_WIDTH_4) ? 1 : 0;

    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_BUSWIDTH, val);
    MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKMOD,mode);
    MSDC_GET_FIELD(MSDC_CFG,MSDC_CFG_CKDIV,div);

    dprintf(INFO, "CLK (%dMHz), SCLK(%dkHz) MODE(%d) DIV(%d) buswidth(%u-bits)\n",
            host->clk/1000000, host->cur_bus_clk/1000, mode, div, width);
}

void msdc_clock(struct mmc_host *host, int on)
{
    /* Chaotian, may need update this part of code */
    dprintf(INFO, "[MSDC] Turn %s %s clock \n", on ? "on" : "off", "host");
}

static void msdc_host_power(struct mmc_host *host, int on)
{
    dprintf(INFO, "[MSDC] Turn %s %s power \n", on ? "on" : "off", "host");
}

static void msdc_card_power(struct mmc_host *host, int on)
{
    dprintf(INFO, "[MSDC] Turn %s %s power \n", on ? "on" : "off", "card");
    return; /* power always on, return directly */

    if (on) {
        msdc_set_card_pwr(host, 1);
    } else {
        msdc_set_card_pwr(host, 0);
    }
}

void msdc_power(struct mmc_host *host, u8 mode)
{
    if (mode == MMC_POWER_ON || mode == MMC_POWER_UP) {
        msdc_host_power(host, 1);
        msdc_card_power(host, 1);
    } else {
        msdc_card_power(host, 0);
        msdc_host_power(host, 0);
    }
}

void msdc_reset_tune_counter(struct mmc_host *host)
{
    host->time_read = 0;
}

#if defined(FEATURE_MMC_CM_TUNING)
#ifndef FPGA_PLATFORM
int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd)
{
	u32 base = host->base;
	u32 rsmpl,cur_rsmpl, orig_rsmpl;
	u32 rdly,cur_rdly, orig_rdly;
	u8 hs400 = 0, orig_clkmode;
	int result = MMC_ERR_CMDTUNEFAIL;

	MSDC_SET_FIELD(EMMC_TOP_CMD, PAD_CMD_RD_RXDLY_SEL, 1);
	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(EMMC_TOP_CMD, PAD_CMD_RXDLY, orig_rdly);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);

	hs400 = (orig_clkmode == 3) ? 1 : 0;
	rdly = 0;

	do {
		for (rsmpl = 0; rsmpl < 2; rsmpl++) {
			cur_rsmpl = (orig_rsmpl + rsmpl) % 2;
			msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE);
			if (host->cur_bus_clk <= 400000){
				MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
			}
			if (cmd->opcode != MMC_CMD_STOP_TRANSMISSION) {
				result = msdc_send_cmd(host, cmd);
				if(result == MMC_ERR_TIMEOUT)
					rsmpl--;
				if (result != MMC_ERR_NONE && cmd->opcode != MMC_CMD_STOP_TRANSMISSION){
					if(cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK ||
					   cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK ||
					   cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||
					   cmd->opcode == MMC_CMD_WRITE_BLOCK)
						msdc_abort_handler(host,1);
					continue;
				}
				result = msdc_wait_cmd_done(host, cmd);
			} else if (cmd->opcode == MMC_CMD_STOP_TRANSMISSION){
				result = MMC_ERR_NONE;
				goto done;
			}
			else
				result = MMC_ERR_BADCRC;

			if (result == MMC_ERR_NONE) {
				goto done;
			}

			if(cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK ||
			   cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK ||
			   cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||
			   cmd->opcode == MMC_CMD_WRITE_BLOCK)
				msdc_abort_handler(host,1);
		}
		cur_rdly = (orig_rdly + rdly + 1) % 32;
		MSDC_SET_FIELD(EMMC_TOP_CMD, PAD_CMD_RXDLY, cur_rdly);
	} while (++rdly < 32);

done:
    dprintf(INFO,("[SD%d] <TUNE_CMD%d><%s>@msdc_tune_cmdrsp\n",
            host->host_id, (cmd->opcode & (~(SD_CMD_BIT | SD_CMD_APP_BIT))),\
            (result == MMC_ERR_NONE) ? "PASS" : "FAIL"));	
    return result;
}
#else
int msdc_tune_cmdrsp(struct mmc_host *host, struct mmc_command *cmd)
{
	u32 base = host->base;
	u32 rsmpl,cur_rsmpl, orig_rsmpl;
	u8 hs400 = 0, orig_clkmode;
	int result = MMC_ERR_CMDTUNEFAIL;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, orig_rsmpl);
	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);

	hs400 = (orig_clkmode == 3) ? 1 : 0;

	for (rsmpl = 0; rsmpl < 2; rsmpl++) {
		cur_rsmpl = (orig_rsmpl + rsmpl) % 2;
		msdc_set_smpl(host, hs400, cur_rsmpl, TYPE_CMD_RESP_EDGE);
		if (host->cur_bus_clk <= 400000){
			MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, 0);
		}
		if (cmd->opcode != MMC_CMD_STOP_TRANSMISSION) {
			result = msdc_send_cmd(host, cmd);
			if(result == MMC_ERR_TIMEOUT)
				rsmpl--;
			if (result != MMC_ERR_NONE && cmd->opcode != MMC_CMD_STOP_TRANSMISSION){
				if(cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK ||
				   cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK ||
				   cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||
				   cmd->opcode == MMC_CMD_WRITE_BLOCK)
					msdc_abort_handler(host,1);
				continue;
			}
			result = msdc_wait_cmd_done(host, cmd);
		} else if (cmd->opcode == MMC_CMD_STOP_TRANSMISSION){
			result = MMC_ERR_NONE;
			goto done;
		}
		else
			result = MMC_ERR_BADCRC;

		if (result == MMC_ERR_NONE) {
			goto done;
		}

		if(cmd->opcode == MMC_CMD_READ_MULTIPLE_BLOCK ||
		   cmd->opcode == MMC_CMD_WRITE_MULTIPLE_BLOCK ||
		   cmd->opcode == MMC_CMD_READ_SINGLE_BLOCK ||
		   cmd->opcode == MMC_CMD_WRITE_BLOCK)
			msdc_abort_handler(host,1);
	}

done:
	dprintf(INFO,("[SD%d] <TUNE_CMD%d><%s>@msdc_tune_cmdrsp\n",
				host->host_id, (cmd->opcode & (~(SD_CMD_BIT | SD_CMD_APP_BIT))),\
				(result == MMC_ERR_NONE) ? "PASS" : "FAIL"));	
	return result;
}
#endif
#endif

#if defined(FEATURE_MMC_RD_TUNING)
#ifndef FPGA_PLATFORM
int msdc_tune_bread(struct mmc_host *host, uchar *dst, u32 src, u32 nblks)
{
	u32 base = host->base;
	u32 cur_rxdly0, org_rxdly0;
	u32 rxdly = 0;
	u32 rdsmpl, cur_rdsmpl, orig_rdsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_READTUNEFAIL;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_rdsmpl);
	MSDC_SET_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
	MSDC_GET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, org_rxdly0);

	do {
		for (rdsmpl = 0; rdsmpl < 2; rdsmpl++) {
			cur_rdsmpl = (orig_rdsmpl + rdsmpl) % 2;
			msdc_set_smpl(host, hs400, cur_rdsmpl, TYPE_READ_DATA_EDGE);

			result = host->blk_read(host, dst, src, nblks);
			if (result == MMC_ERR_CMDTUNEFAIL ||
			    result == MMC_ERR_CMD_RSPCRC ||
			    result == MMC_ERR_ACMD_RSPCRC)
				goto done;

			MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
			if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

			/* no crc error in this data line */
			if (result == MMC_ERR_NONE && dcrc == 0) {
				goto done;
			} else {
				result = MMC_ERR_BADCRC;
			}
		}

		cur_rxdly0 = (org_rxdly0 + rxdly + 1) % 32;
		MSDC_SET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, cur_rxdly0);

	} while (++rxdly < 32);

done:
	dprintf(INFO,("[SD%d] <msdc_tune_bread<%s><cmd%d>@msdc_tune_bread\n",
		       host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", (nblks == 1 ? 17 : 18)));

	return result;
}
#else
int msdc_tune_bread(struct mmc_host *host, uchar *dst, u32 src, u32 nblks)
{
	u32 base = host->base;
	u32 rxdly = 0;
	u32 rdsmpl, cur_rdsmpl, orig_rdsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_READTUNEFAIL;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_rdsmpl);

	for (rdsmpl = 0; rdsmpl < 2; rdsmpl++) {
		cur_rdsmpl = (orig_rdsmpl + rdsmpl) % 2;
		msdc_set_smpl(host, hs400, cur_rdsmpl, TYPE_READ_DATA_EDGE);

		result = host->blk_read(host, dst, src, nblks);
		if (result == MMC_ERR_CMDTUNEFAIL ||
		    result == MMC_ERR_CMD_RSPCRC ||
		    result == MMC_ERR_ACMD_RSPCRC)
			goto done;

		MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
		if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

		/* no crc error in this data line */
		if (result == MMC_ERR_NONE && dcrc == 0) {
			goto done;
		} else {
			result = MMC_ERR_BADCRC;
		}
	}

done:
	dprintf(INFO,("[SD%d] <msdc_tune_bread<%s><cmd%d>@msdc_tune_bread\n",
			host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", (nblks == 1 ? 17 : 18)));

	return result;
}
#endif

#define READ_TUNING_MAX_HS (2 * 32)
#define READ_TUNING_MAX_UHS (2 * 32)
#define READ_TUNING_MAX_UHS_CLKMOD1 (2 * 32)

#ifndef FPGA_PLATFORM
int msdc_tune_read(struct mmc_host *host)
{
	u32 base = host->base;
	u32 cur_rxdly0, org_rxdly0;
	u32 rxdly = 0;
	u32 rdsmpl, cur_dsmpl, orig_dsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_NONE;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);
	MSDC_SET_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
	MSDC_GET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, org_rxdly0);

	cur_dsmpl = (orig_dsmpl + 1) ;
	msdc_set_smpl(host, hs400, (cur_dsmpl % 2), TYPE_READ_DATA_EDGE);
	++(host->time_read);
	if (cur_dsmpl >= 2){
		cur_rxdly0++;
		MSDC_SET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, cur_rxdly0 % 32);
		if (cur_rxdly0 > 32)
			result = MMC_ERR_READTUNEFAIL;
	}

	dprintf(INFO,("[SD%d] <msdc_tune_read <%s> @msdc_tune_bread\n",
				host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL"));	
	return result;
}
#else
int msdc_tune_read(struct mmc_host *host)
{
	u32 base = host->base;
	u32 rdsmpl, cur_dsmpl, orig_dsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_NONE;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_dsmpl);

	cur_dsmpl = (orig_dsmpl + 1) ;
	msdc_set_smpl(host, hs400, (cur_dsmpl % 2), TYPE_READ_DATA_EDGE);
	++(host->time_read);
	if (cur_dsmpl >= 2){
		result = MMC_ERR_READTUNEFAIL;
	}

	dprintf(INFO,("[SD%d] <msdc_tune_read <%s> @msdc_tune_bread\n",
				host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL"));	
	return result;
}
#endif
#endif  /* end of FEATURE_MMC_RD_TUNING */

#if defined(FEATURE_MMC_WR_TUNING)
#ifndef FPGA_PLATFORM
/* Chaotian, make read/write tune flow the same */
int msdc_tune_bwrite(struct mmc_host *host, u32 dst, uchar *src, u32 nblks)
{
	u32 base = host->base;
	u32 cur_rxdly0, org_rxdly0;
	u32 rxdly = 0;
	u32 rdsmpl, cur_rdsmpl, orig_rdsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_READTUNEFAIL;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_rdsmpl);
	MSDC_SET_BIT32(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY_SEL);
	MSDC_GET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, org_rxdly0);

	do {
		for (rdsmpl = 0; rdsmpl < 2; rdsmpl++) {
			cur_rdsmpl = (orig_rdsmpl + rdsmpl) % 2;
			msdc_set_smpl(host, hs400, cur_rdsmpl, TYPE_READ_DATA_EDGE);

			result = host->blk_write(host, dst, src, nblks);
			if (result == MMC_ERR_CMDTUNEFAIL ||
			    result == MMC_ERR_CMD_RSPCRC ||
			    result == MMC_ERR_ACMD_RSPCRC)
				goto done;

			MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
			if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

			/* no crc error in this data line */
			if (result == MMC_ERR_NONE && dcrc == 0) {
				goto done;
			} else {
				result = MMC_ERR_BADCRC;
			}
		}

		cur_rxdly0 = (org_rxdly0 + rxdly + 1) % 32;
		MSDC_SET_FIELD(EMMC_TOP_CONTROL, PAD_DAT_RD_RXDLY, cur_rxdly0);

	} while (++rxdly < 32);

done:
	dprintf(INFO,("[SD%d] <msdc_tune_bread<%s><cmd%d>@msdc_tune_bread\n",
		       host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", (nblks == 1 ? 17 : 18)));

	return result;
}
#else
int msdc_tune_bwrite(struct mmc_host *host, u32 dst, uchar *src, u32 nblks)
{
	u32 base = host->base;
	u32 rdsmpl, cur_rdsmpl, orig_rdsmpl;
	u32 dcrc, ddr = 0;
	u8 hs400 = 0;
	u32 orig_clkmode;
	int result = MMC_ERR_READTUNEFAIL;

	MSDC_GET_FIELD(MSDC_CFG, MSDC_CFG_CKMOD, orig_clkmode);
	ddr = (orig_clkmode == 2) ? 1 : 0;
	hs400 = (orig_clkmode == 3) ? 1 : 0;

	MSDC_GET_FIELD(MSDC_IOCON, MSDC_IOCON_R_D_SMPL, orig_rdsmpl);

	for (rdsmpl = 0; rdsmpl < 2; rdsmpl++) {
		cur_rdsmpl = (orig_rdsmpl + rdsmpl) % 2;
		msdc_set_smpl(host, hs400, cur_rdsmpl, TYPE_READ_DATA_EDGE);

		result = host->blk_write(host, dst, src, nblks);
		if (result == MMC_ERR_CMDTUNEFAIL ||
		    result == MMC_ERR_CMD_RSPCRC ||
		    result == MMC_ERR_ACMD_RSPCRC)
			goto done;

		MSDC_GET_FIELD(SDC_DCRC_STS, SDC_DCRC_STS_POS|SDC_DCRC_STS_NEG, dcrc);
		if (!ddr) dcrc &= ~SDC_DCRC_STS_NEG;

		/* no crc error in this data line */
		if (result == MMC_ERR_NONE && dcrc == 0) {
			goto done;
		} else {
			result = MMC_ERR_BADCRC;
		}
	}

done:
	dprintf(INFO,("[SD%d] <msdc_tune_bwrite<%s><cmd%d>@msdc_tune_bwrite\n",
			host->host_id, (result == MMC_ERR_NONE && dcrc == 0) ?"PASS" : "FAIL", (nblks == 1 ? 17 : 18)));

	return result;
}
#endif
#endif /* end of FEATURE_MMC_WR_TUNING */

void msdc_emmc_boot_stop(struct mmc_host *host)
{
    addr_t base = host->base;
    u32 count = 0;

    /* Step5. stop the boot mode */
    MSDC_WRITE32(SDC_ARG, 0x00000000);
    MSDC_WRITE32(SDC_CMD, 0x00001000);

    MSDC_SET_FIELD(EMMC_CFG0, EMMC_CFG0_BOOTWDLY, 2);
    MSDC_SET_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSTOP);
    while (MSDC_READ32(EMMC_STS) & EMMC_STS_BOOTUPSTATE) {
        spin(1000);
        count++;
        if (count >= 1000) {
            dprintf(ALWAYS, "Timeout to wait EMMC to leave boot state!\n");
            break;
        }
    }

    /* Step6. */
    MSDC_CLR_BIT32(EMMC_CFG0, EMMC_CFG0_BOOTSUPP);

    /* Step7. clear EMMC_STS bits */
    MSDC_WRITE32(EMMC_STS, MSDC_READ32(EMMC_STS));
}

int msdc_init(struct mmc_host *host)
{
    addr_t base = host->host_id ? MSDC1_BASE: MSDC0_BASE; /* only support MSDC0, MSDC1 */
    addr_t top_baddr = host->host_id? MSDC1_TOP_BASE :MSDC0_TOP_BASE;
    msdc_priv_t *priv;

    dprintf(INFO, "[%s]: Host controller intialization start \n", __func__);

    priv = &msdc_priv;
    memset(priv, 0, sizeof(msdc_priv_t));

    host->base   = base;
    host->top_base = top_baddr;
    host->clksrc = msdc_cap[host->host_id].clk_src;
    host->hclksrc= msdc_cap[host->host_id].hclk_src;
#ifndef FPGA_PLATFORM
    host->f_max  = hclks_msdc30[host->clksrc];
#else
    host->f_max  = MSDC_MAX_SCLK;
#endif

    host->f_min  = MSDC_MIN_SCLK;
    host->blklen = 0;
    host->priv   = (void *)priv;
    host->caps   = MMC_CAP_MULTIWRITE;

    if (msdc_cap[host->host_id].flags & MSDC_HIGHSPEED)
        host->caps |= (MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED);
    if (msdc_cap[host->host_id].flags & MSDC_DDR)
        host->caps |= MMC_CAP_DDR;
    if (msdc_cap[host->host_id].data_pins == 4)
        host->caps |= MMC_CAP_4_BIT_DATA;
    if (msdc_cap[host->host_id].data_pins == 8)
        host->caps |= MMC_CAP_8_BIT_DATA | MMC_CAP_4_BIT_DATA;
    if (msdc_cap[host->host_id].flags & MSDC_HS200)
        host->caps |= MMC_CAP_EMMC_HS200;
    if (msdc_cap[host->host_id].flags & MSDC_HS400)
        host->caps |= MMC_CAP_EMMC_HS400;

    host->ocr_avail = MMC_VDD_32_33;  /* TODO: To be customized */

    /* Configure BASIC_DMA + AUTOCMD12 for better R/W performance
     * NOTE: ACMD23 only support transferring size of up to 32M */
    priv->autocmd = MSDC_AUTOCMD12;
    if (priv->autocmd == MSDC_AUTOCMD23)
        /* The maximal transferring size is size of *[15:0] number of blocks* */
        host->max_phys_segs = 0xffff;
    else
        /* The maximal transferring size is size of DMA_LENGTH */
        host->max_phys_segs = (UINT_MAX & ~511) >> MMC_BLOCK_BITS_SHFT;

    priv->rdsmpl       = msdc_cap[host->host_id].data_edge;
    priv->wdsmpl       = msdc_cap[host->host_id].data_edge;
    priv->rsmpl       = msdc_cap[host->host_id].cmd_edge;

#ifdef MSDC_USE_DMA_MODE
    host->blk_read  = msdc_dma_bread;
    host->blk_write = msdc_dma_bwrite;
    dprintf(INFO, "Transfer method: DMA\n");
#else
    host->blk_read  = msdc_pio_bread;
    host->blk_write = msdc_pio_bwrite;
    dprintf(INFO, "Transfer method: PIO\n");
#endif

    priv->rdsmpl       = msdc_cap[host->host_id].data_edge;
    priv->rsmpl       = msdc_cap[host->host_id].cmd_edge;

    /* disable EMMC boot mode */
    msdc_emmc_boot_stop(host);

    msdc_power(host, MMC_POWER_OFF);
    msdc_power(host, MMC_POWER_ON);

    /* set to SD/MMC mode */
    MSDC_SET_FIELD(MSDC_CFG, MSDC_CFG_MODE, MSDC_SDMMC);
    MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_PIO);
    MSDC_SET_BIT32(MSDC_CFG, MSDC_CFG_CKPDN);

    MSDC_RESET();
    MSDC_CLR_FIFO();
    MSDC_CLR_INT();

    /* reset tuning parameter */
    //MSDC_WRITE32(MSDC_PAD_CTL0, 0x0098000);
    //MSDC_WRITE32(MSDC_PAD_CTL1, 0x00A0000);
    //MSDC_WRITE32(MSDC_PAD_CTL2, 0x00A0000);
    MSDC_WRITE32(MSDC_PAD_TUNE, 0x0000000);
    MSDC_WRITE32(MSDC_DAT_RDDLY0, 0x00000000);
    MSDC_WRITE32(MSDC_DAT_RDDLY1, 0x00000000);
    MSDC_WRITE32(MSDC_IOCON, 0x00000000);

    MSDC_SET_BIT32(MSDC_PAD_TUNE, MSDC_PAD_TUNE_DATRRDLYSEL);
    MSDC_SET_BIT32(MSDC_PAD_TUNE, MSDC_PAD_TUNE_CMDRDLYSEL);
    MSDC_WRITE32(MSDC_PATCH_BIT0, 0x403c0046);
    MSDC_WRITE32(MSDC_PATCH_BIT1, 0xFFFF4309);//High 16 bit = 0 mean Power KPI is on, enable ECO for write timeout issue
    MSDC_SET_BIT32(EMMC50_CFG0, MSDC_EMMC50_CFG_CRCSTS_SEL);
    MSDC_CLR_BIT32(SDC_FIFO_CFG, SDC_FIFO_CFG_WRVALIDSEL);
    MSDC_CLR_BIT32(SDC_FIFO_CFG, SDC_FIFO_CFG_RDVALIDSEL);
    MSDC_SET_BIT32(SDC_AVG_CFG0, SDC_RX_ENHANCE_EN);
    MSDC_SET_BIT32(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS);
    MSDC_CLR_BIT32(MSDC_PATCH_BIT1, MSDC_BUSY_CHECK_SEL); /* disable busy check */
    //MSDC_PATCH_BIT1YD:WRDAT_CRCS_TA_CNTR need fix to 3'001 by default,(<50MHz) (>=50MHz set 3'001 as initial value is OK for tunning)
    //YD:CMD_RSP_TA_CNTR need fix to 3'001 by default(<50MHz)(>=50MHz set 3'001as initial value is OK for tunning)
    /* 2012-01-07 using internal clock instead of feedback clock */
    //MSDC_SET_BIT32(MSDC_PATCH_BIT0, MSDC_CKGEN_MSDC_CK_SEL);

#ifdef MSDC_USE_PATCH_BIT2_TURNING_WITH_ASYNC
    MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,1);
    MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,0);
#else
    MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGCRCSTS,0);
    MSDC_SET_FIELD(MSDC_PATCH_BIT2, MSDC_PB2_CFGRESP,1);
#endif

    /* enable wake up events */
    //MSDC_SET_BIT32(SDC_CFG, SDC_CFG_INSWKUP);

#ifndef FPGA_PLATFORM
    msdc_gpio_and_pad_init(host);
#endif
    /* set sampling edge */
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_RSPL, msdc_cap[host->host_id].cmd_edge);
    MSDC_SET_FIELD(MSDC_IOCON, MSDC_IOCON_DSPL, msdc_cap[host->host_id].data_edge);

    /* write crc timeout detection */
    MSDC_SET_FIELD(MSDC_PATCH_BIT0, MSDC_PB0_DETWR_CRCTMO, 1);

    msdc_set_startbit(host, START_AT_RISING);

    msdc_config_bus(host, HOST_BUS_WIDTH_1);
    msdc_config_clock(host, 0, MSDC_MIN_SCLK);
    msdc_set_timeout(host, 100000000, 0);

    /* disable SDIO func */
    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_SDIO, 0);
    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_SDIOIDE, 0);
    MSDC_SET_FIELD(SDC_CFG, SDC_CFG_INSWKUP, 0);

    /* Clear all interrupts first */
    MSDC_CLR_INT();
    MSDC_WRITE32(MSDC_INTEN, 0);

#ifdef MSDC_USE_DMA_MODE
    /* Register msdc irq */
    mt_irq_set_sens(MT_MSDC0_IRQ_ID + host->host_id, LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_MSDC0_IRQ_ID + host->host_id, MT65xx_POLARITY_LOW);
    event_init(&msdc_int_event, false, EVENT_FLAG_AUTOUNSIGNAL);
    register_int_handler(MT_MSDC0_IRQ_ID + host->host_id, msdc_interrupt_handler, host);
    unmask_interrupt(MT_MSDC0_IRQ_ID + host->host_id);
#endif

    dprintf(INFO, "[%s]: Host controller intialization done\n", __func__);
    return 0;
}


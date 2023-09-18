#include <reg.h>
#include <platform/mt8518.h>
#include <platform/spm.h>
#include <platform/spm_mtcmos.h>
#include <platform/pll.h>

#define USE_SPIN    0

#define spm_mtcmos_noncpu_lock(x)   (*(&x) = 0)
#define spm_mtcmos_noncpu_unlock(x) (*(&x) = 0)

#if USE_SPIN

#define udelay(x)       spin(x)
#define mdelay(x)       udelay((x) * 1000)

#else /* !USE_SPIN */

#define my_delay(count) \
    do { \
        volatile unsigned int i = count * 26; \
        for (; i != 0; i--); \
    } while (0)

#define udelay(x)       my_delay(x)
#define mdelay(x)       udelay((x) * 1000)

#endif              /* USE_SPIN */

/**************************************
 * for non-CPU MTCMOS
 **************************************/
#define DIS_PWR_STA_MASK    (0x1 << 3)
#define CM4_PWR_STA_MASK   (0x1 << 16)
#define AUDAFE_PWR_STA_MASK    (0x1 << 21)
#define AUDSRC_PWR_STA_MASK    (0x1 << 20)

#define DIS_SRAM_PDN        (0xf << 8)
#define CM4_SRAM_PDN        (0x1 << 8)
#define AUDAFE_SRAM_PDN        (0xf << 8)
#define AUDSRC_SRAM_PDN        (0x1 << 8)

#define DIS_SRAM_ACK        (0x1 << 12)
#define CM4_SRAM_ACK        (0x1 << 9)
#define AUDAFE_SRAM_ACK        (0xf << 12)
#define AUDSRC_SRAM_ACK        (0x1 << 9)

#define DISP_PROT_MASK       ((0x1 << 1))   /* bit 1 */
#define CM4_PROT_MASK       ((0x1 << 7) | (0x1 << 8)| (0x1 << 9))   /* bit 7, 8 ,9 */
#define AUDAFE_PROT_MASK       ((0x1 << 25) | (0x1 << 26)| (0x1 << 28))   /* bit 25, 26 ,28 */
#define AUDSRC_PROT_MASK       ((0x1 << 27))   /* bit 27 */

#define PWR_CLK_DIS             (1U << 4)
#define PWR_ON_2ND              (1U << 3)
#define PWR_ON                  (1U << 2)
#define PWR_ISO                 (1U << 1)
#define PWR_RST_B               (1U << 0)
#define SRAM_CKISO              (1U << 5)
#define SRAM_ISOINT_B           (1U << 6)

int spm_mtcmos_ctrl_disp(int state)
{
    int err = 0;
    unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) | DISP_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) !=
                DISP_PROT_MASK) {
        }

        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) | DIS_SRAM_PDN);

        while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK) !=
                DIS_SRAM_ACK) {
        }

        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_DIS_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_DIS_PWR_CON, val);

        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

        while ((spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
        }
    } else {    /* STA_POWER_ON */
        spm_write(SPM_DIS_PWR_CON, spm_read(SPM_DIS_PWR_CON) | PWR_ON);
        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) | PWR_ON_2ND);

        while (!(spm_read(SPM_PWR_STATUS) & DIS_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_2ND) & DIS_PWR_STA_MASK)) {
        }

        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) | PWR_RST_B);

        spm_write(SPM_DIS_PWR_CON,
                  spm_read(SPM_DIS_PWR_CON) & ~DIS_SRAM_PDN);

        while ((spm_read(SPM_DIS_PWR_CON) & DIS_SRAM_ACK)) {
        }

        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) & ~DISP_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & DISP_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_cm4(int state)
{
    int err = 0;
    unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) | CM4_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & CM4_PROT_MASK) !=
                CM4_PROT_MASK) {
        }

        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) | CM4_SRAM_PDN);

        while ((spm_read(SPM_CM4_PWR_CON) & CM4_SRAM_ACK) !=
                CM4_SRAM_ACK) {
        }

        spm_write(SPM_CM4_PWR_CON, spm_read(SPM_CM4_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_CM4_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_CM4_PWR_CON, val);

        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

        while ((spm_read(SPM_PWR_STATUS) & CM4_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_2ND) & CM4_PWR_STA_MASK)) {
        }
    } else {    /* STA_POWER_ON */
        spm_write(SPM_CM4_PWR_CON, spm_read(SPM_CM4_PWR_CON) | PWR_ON);
        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) | PWR_ON_2ND);

        while (!(spm_read(SPM_PWR_STATUS) & CM4_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_2ND) & CM4_PWR_STA_MASK)) {
        }

        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) | PWR_RST_B);

        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) & ~CM4_SRAM_PDN);

        while ((spm_read(SPM_CM4_PWR_CON) & CM4_SRAM_ACK)) {
        }

        spm_write(SPM_CM4_PWR_CON,
                  spm_read(SPM_CM4_PWR_CON) | SRAM_ISOINT_B);

        udelay(20);

        spm_write(SPM_CM4_PWR_CON,
                 spm_read(SPM_CM4_PWR_CON) & ~SRAM_CKISO);

        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) & ~CM4_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & CM4_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_audafe(int state)
{
    int err = 0;
    unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) | AUDAFE_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & AUDAFE_PROT_MASK) !=
                AUDAFE_PROT_MASK) {
        }

        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) | AUDAFE_SRAM_PDN);

        while ((spm_read(SPM_AUDAFE_PWR_CON) & AUDAFE_SRAM_ACK) !=
                AUDAFE_SRAM_ACK) {
        }

        spm_write(SPM_AUDAFE_PWR_CON, spm_read(SPM_AUDAFE_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_AUDAFE_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_AUDAFE_PWR_CON, val);

        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

        while ((spm_read(SPM_PWR_STATUS) & AUDAFE_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_2ND) & AUDAFE_PWR_STA_MASK)) {
        }
    } else {    /* STA_POWER_ON */
        spm_write(SPM_AUDAFE_PWR_CON, spm_read(SPM_AUDAFE_PWR_CON) | PWR_ON);
        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) | PWR_ON_2ND);

        while (!(spm_read(SPM_PWR_STATUS) & AUDAFE_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_2ND) & AUDAFE_PWR_STA_MASK)) {
        }

        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) | PWR_RST_B);

        spm_write(SPM_AUDAFE_PWR_CON,
                  spm_read(SPM_AUDAFE_PWR_CON) & ~AUDAFE_SRAM_PDN);

        while ((spm_read(SPM_AUDAFE_PWR_CON) & AUDAFE_SRAM_ACK)) {
        }

        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) & ~AUDAFE_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & AUDAFE_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

int spm_mtcmos_ctrl_audsrc(int state)
{
    int err = 0;
    unsigned int val;
    unsigned long flags;

    spm_mtcmos_noncpu_lock(flags);

    if (state == STA_POWER_DOWN) {
        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) | AUDSRC_PROT_MASK);
        while ((spm_read(TOPAXI_PROT_STA1) & AUDSRC_PROT_MASK) !=
                AUDSRC_PROT_MASK) {
        }

        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) | AUDSRC_SRAM_PDN);

        while ((spm_read(SPM_AUDSRC_PWR_CON) & AUDSRC_SRAM_ACK) !=
                AUDSRC_SRAM_ACK) {
        }

        spm_write(SPM_AUDSRC_PWR_CON, spm_read(SPM_AUDSRC_PWR_CON) | PWR_ISO);

        val = spm_read(SPM_AUDSRC_PWR_CON);
        val = (val & ~PWR_RST_B) | PWR_CLK_DIS;
        spm_write(SPM_AUDSRC_PWR_CON, val);

        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) & ~(PWR_ON | PWR_ON_2ND));

        while ((spm_read(SPM_PWR_STATUS) & AUDSRC_PWR_STA_MASK)
                || (spm_read(SPM_PWR_STATUS_2ND) & AUDSRC_PWR_STA_MASK)) {
        }
    } else {    /* STA_POWER_ON */
        spm_write(SPM_AUDSRC_PWR_CON, spm_read(SPM_AUDSRC_PWR_CON) | PWR_ON);
        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) | PWR_ON_2ND);

        while (!(spm_read(SPM_PWR_STATUS) & AUDSRC_PWR_STA_MASK)
                || !(spm_read(SPM_PWR_STATUS_2ND) & AUDSRC_PWR_STA_MASK)) {
        }

        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) & ~PWR_CLK_DIS);
        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) & ~PWR_ISO);
        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) | PWR_RST_B);

        spm_write(SPM_AUDSRC_PWR_CON,
                  spm_read(SPM_AUDSRC_PWR_CON) & ~AUDSRC_SRAM_PDN);

        while ((spm_read(SPM_AUDSRC_PWR_CON) & AUDSRC_SRAM_ACK)) {
        }

        spm_write(TOPAXI_PROT_EN,
                  spm_read(TOPAXI_PROT_EN) & ~AUDSRC_PROT_MASK);
        while (spm_read(TOPAXI_PROT_STA1) & AUDSRC_PROT_MASK) {
        }
    }

    spm_mtcmos_noncpu_unlock(flags);

    return err;
}

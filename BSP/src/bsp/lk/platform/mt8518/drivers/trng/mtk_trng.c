#include <debug.h>
#include <reg.h>
#include <platform/mt_reg_base.h>
//#include <platform/pll.h>
#include <string.h>

#define TRNG_CTRL_REG            (TRNG_BASE+0x00)
#define TRNG_DATA_REG            (TRNG_BASE+0x08)
#define TRNG_CONF_REG            (TRNG_BASE+0x0C)

/* #define TRNG_PDN_VALUE          0x1 */

/* TRNG_CTRL_REG */
#define TRNG_RDY         (0x80000000)
#define TRNG_START       (0x00000001)

/* Assume clock setting for trng is on */
s32 trng_drv_get_random_data(u8 *buf, u32 len)
{
    s32 retval = 0;

    if (0 == len)
        return 0;

    if (NULL == buf) {
        dprintf(CRITICAL, "[TRNG] Error: input buffer is NULL\n");
        return -1;
    }

    /*if (readl(TRNG_PDN_STATUS) & TRNG_PDN_VALUE) //TRNG clock is off
        writel(TRNG_PDN_VALUE, TRNG_PDN_CLR);  //ungate TRNG clock*/

    if (TRNG_START != (readl(TRNG_CTRL_REG) & TRNG_START)) {
        writel(TRNG_START, TRNG_CTRL_REG); //start TRNG
        if (TRNG_START != (readl(TRNG_CTRL_REG) & TRNG_START)) {
            dprintf(CRITICAL, "[TRNG] Error: fail to start TRNG because clock is disabled\n");
            return -2;
        }
    }

    /* clear output buffer */
    memset(buf, 0, len);

    /* generate random data with default rings */
    while (len >= sizeof(u32)) {
        if(TRNG_RDY != (readl(TRNG_CTRL_REG) & TRNG_RDY)) {
            spin(1);
            continue;
        }

        *(u32 *)buf = readl(TRNG_DATA_REG);
        retval += sizeof(u32);
        buf += sizeof(u32);
        len -= sizeof(u32);
    }

    writel(0x0, TRNG_CTRL_REG);     //stop TRNG
    /*writel(TRNG_PDN_VALUE, TRNG_PDN_SET); //gate TRNG clock*/

    return retval;
}

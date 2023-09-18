#include <string.h>
#include <sha256.h>
#include <hmac.h>
#include <reg.h>
#include <lk/init.h>
#include <debug.h>
#include "seclib.h"
#include "platform/mtk_serial_key.h"
#include "platform/mmc_rpmb.h"

/******************************************************************************
 * CONSTANT DEFINITIONS
 ******************************************************************************/
#define MOD                             "RPMB"

static hmac_state hmac;
static struct sha256_context sha256_state;
static hash_param sha256_hash = {
    .hash_state = &sha256_state,
    .init = (int (*)(void *))sha256_start,
    .process = (int (*)(void *, const unsigned char *, unsigned int))sha256_process,
    .done = (int (*)(void *, unsigned char *))sha256_end,
    .hashsize = 32,
    .blocksize = 64
};

/* rpmb_auth_key */
static int rak_inited = 0;
static unsigned char rak[32];

static int rpmb_key_init(unsigned char *data, unsigned int size)
{
    if (size != 32)
        return -1;

    memcpy(rak, data, 32);
    rak_inited = 1;
    return 0;
}

int rpmb_hmac_init(unsigned char *buf, unsigned int size)
{
    if (rak_inited == 0) {
        return -1;
        }
    hmac_init(&hmac, &sha256_hash, rak, sizeof(rak));
    hmac_process(&hmac, buf, size);

    return 0;
}

int rpmb_hmac_process(unsigned char *buf, unsigned int size)
{
    return hmac_process(&hmac, buf, size);
}

int rpmb_hmac_done(unsigned char *outmac, unsigned int *size)
{
    unsigned long out_len;

    if (size == NULL || outmac == NULL) return -1;

    out_len = *size;
    hmac_done(&hmac, outmac, &out_len);
    *size = out_len;

    return 0;
}

int rpmb_set_key(int (*set_key_func)(u8 *))
{
    if (rak_inited == 0) {
        return -1;
        }
    return set_key_func(rak);
}

void rpmb_init(void)
{
    u32 id[4];
    u32 rpmb_key[8];

    id[0] = readl(SERIAL_KEY_LO);
    id[1] = readl(SERIAL_KEY_HI);
    id[2] = readl(SERIAL_KEY_2_LO);
    id[3] = readl(SERIAL_KEY_2_HI);
    seclib_get_msg_auth_key((void *)id, 16, (void *)rpmb_key, 32);
#if 0
    dprintf(CRITICAL, "id: %08x %08x %08x %08x\n",
        id[0], id[1], id[2], id[3]);
    dprintf(CRITICAL, "rpmbkey: %08x %08x %08x %08x %08x %08x %08x %08x\n",
        rpmb_key[0], rpmb_key[1], rpmb_key[2], rpmb_key[3],
        rpmb_key[4], rpmb_key[5], rpmb_key[6], rpmb_key[7]);
#endif

    rpmb_key_init((unsigned char *)rpmb_key, sizeof(rpmb_key));
    rpmb_set_key(mmc_rpmb_set_key);
}

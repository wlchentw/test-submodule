#include <stdlib.h>
#include <string.h>
#include <hmac.h>
/**
   Initialize an HMAC context.
   @param hmac     The HMAC state
   @param hash     The hash for HMAC
   @param key      The secret key
   @param keylen   The length of the secret key
   @return 0 if successful
*/
int hmac_init(hmac_state *hmac, hash_param *hash, const unsigned char *key,
          unsigned long keylen)
{
    unsigned int i;
    unsigned char *buf;

    hmac->hash = hash;

    hmac->key = malloc(hmac->hash->blocksize);
    buf = malloc(hmac->hash->blocksize);

    /* assume keylen <= hmac blocksize */
    memcpy(hmac->key, key, keylen);
    memset(hmac->key + keylen, 0, hmac->hash->blocksize - keylen);

    for (i = 0; i < hmac->hash->blocksize; i++)
        buf[i] = hmac->key[i] ^ 0x36;

    hmac->hash->init(hmac->hash->hash_state);
    hmac->hash->process(hmac->hash->hash_state, buf, hmac->hash->blocksize);

    free(buf);

    return 0;
}

/**
  Process data through HMAC
  @param hmac    The hmac state
  @param in      The data to send through HMAC
  @param inlen   The length of the data to HMAC
  @return 0 if successful
*/
int hmac_process(hmac_state *hmac, const unsigned char *in, unsigned long inlen)
{
    return hmac->hash->process(hmac->hash->hash_state, in, inlen);
}

/**
   Terminate an LTC_HMAC session
   @param hmac    The LTC_HMAC state
   @param out     [out] The destination of the LTC_HMAC authentication tag
   @param outlen  [in/out]  The max size and resulting size of the LTC_HMAC authentication tag
   @return CRYPT_OK if successful
*/
int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen)
{
    unsigned int i;
    unsigned char *buf, *isha;

    buf = malloc(hmac->hash->blocksize);
    isha = malloc(hmac->hash->hashsize);

    hmac->hash->done(hmac->hash->hash_state, isha);

    for (i = 0; i < hmac->hash->blocksize; i++)
        buf[i] = hmac->key[i] ^ 0x5c;

    hmac->hash->init(hmac->hash->hash_state);
    hmac->hash->process(hmac->hash->hash_state, buf, hmac->hash->blocksize);
    hmac->hash->process(hmac->hash->hash_state, isha, hmac->hash->hashsize);
    hmac->hash->done(hmac->hash->hash_state, buf);

    for (i = 0; i < hmac->hash->hashsize && i < (unsigned int)*outlen; i++)
        out[i] = buf[i];

    *outlen = i;

    free(isha);
    free(buf);
    return 0;
}

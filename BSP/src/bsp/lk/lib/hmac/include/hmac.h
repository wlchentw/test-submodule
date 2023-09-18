#ifndef HMAC_H
#define HMAC_H

typedef struct Hash_param {
    void *hash_state;
    int (*init)(void *hash_state);
    int (*process)(void *hash_state, const unsigned char *in, unsigned int inlen);
    int (*done)(void *hash_state, unsigned char *out);
    unsigned int hashsize;
    unsigned int blocksize;
} hash_param;

typedef struct Hmac_state {
    hash_param     *hash;
    unsigned char  *key;
} hmac_state;

int hmac_init(hmac_state *hmac, hash_param *hash, const unsigned char *key,
              unsigned long keylen);
int hmac_process(hmac_state *hmac, const unsigned char *in,
                 unsigned long inlen);
int hmac_done(hmac_state *hmac, unsigned char *out, unsigned long *outlen);

#endif /* HMAC_H */

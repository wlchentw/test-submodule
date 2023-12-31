// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2001-2007, Tom St Denis
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
#include "tomcrypt.h"

/**
  @file rsa_sign_hash.c
  RSA PKCS #1 v1.5 and v2 PSS sign hash, Tom St Denis and Andreas Lange
*/

#ifdef LTC_MRSA

/**
  PKCS #1 pad then sign
  @param in        The hash to sign
  @param inlen     The length of the hash to sign (octets)
  @param out       [out] The signature
  @param outlen    [in/out] The max size and resulting size of the signature
  @param padding   Type of padding (LTC_PKCS_1_PSS, LTC_PKCS_1_V1_5 or LTC_PKCS_1_V1_5_NA1)
  @param prng      An active PRNG state
  @param prng_idx  The index of the PRNG desired
  @param hash_idx  The index of the hash desired
  @param saltlen   The length of the salt desired (octets)
  @param key       The private RSA key to use
  @return CRYPT_OK if successful
*/
int rsa_sign_hash_ex(const unsigned char *in,       unsigned long  inlen,
                           unsigned char *out,      unsigned long *outlen,
                           int            padding,
                           prng_state    *prng,     int            prng_idx,
                           int            hash_idx, unsigned long  saltlen,
                           rsa_key *key)
{
   unsigned long modulus_bitlen, modulus_bytelen, x, y;
   int           err;

   LTC_ARGCHK(in       != NULL);
   LTC_ARGCHK(out      != NULL);
   LTC_ARGCHK(outlen   != NULL);
   LTC_ARGCHK(key      != NULL);

   /* valid padding? */
   if ((padding != LTC_PKCS_1_V1_5) &&
       (padding != LTC_PKCS_1_PSS) &&
       (padding != LTC_PKCS_1_V1_5_NA1)) {
     return CRYPT_PK_INVALID_PADDING;
   }

   if (padding == LTC_PKCS_1_PSS) {
     /* valid prng ? */
     if ((err = prng_is_valid(prng_idx)) != CRYPT_OK) {
        return err;
     }
   }

   if (padding != LTC_PKCS_1_V1_5_NA1) {
     /* valid hash ? */
     if ((err = hash_is_valid(hash_idx)) != CRYPT_OK) {
        return err;
     }
   }

   /* get modulus len in bits */
   modulus_bitlen = mp_count_bits((key->N));

  /* outlen must be at least the size of the modulus */
  modulus_bytelen = mp_unsigned_bin_size((key->N));
  if (modulus_bytelen > *outlen) {
     *outlen = modulus_bytelen;
     return CRYPT_BUFFER_OVERFLOW;
  }

  if (padding == LTC_PKCS_1_PSS) {
    /* PSS pad the key */
    x = *outlen;
    if ((err = pkcs_1_pss_encode(in, inlen, saltlen, prng, prng_idx,
                                 hash_idx, modulus_bitlen, out, &x)) != CRYPT_OK) {
       return err;
    }
  } else {
    /* PKCS #1 v1.5 pad the hash */
    unsigned char *tmpin;

    if (padding == LTC_PKCS_1_V1_5) {
      ltc_asn1_list digestinfo[2], siginfo[2];
      /* not all hashes have OIDs... so sad */
      if (hash_descriptor[hash_idx]->OIDlen == 0) {
         return CRYPT_INVALID_ARG;
      }

    /* construct the SEQUENCE
        SEQUENCE {
           SEQUENCE {hashoid OID
                     blah    NULL
           }
         hash    OCTET STRING
        }
     */
      LTC_SET_ASN1(digestinfo, 0, LTC_ASN1_OBJECT_IDENTIFIER, hash_descriptor[hash_idx]->OID, hash_descriptor[hash_idx]->OIDlen);
      LTC_SET_ASN1(digestinfo, 1, LTC_ASN1_NULL,              NULL,                          0);
      LTC_SET_ASN1(siginfo,    0, LTC_ASN1_SEQUENCE,          digestinfo,                    2);
      LTC_SET_ASN1(siginfo,    1, LTC_ASN1_OCTET_STRING,      in,                            inlen);

      /* allocate memory for the encoding */
      y = mp_unsigned_bin_size(key->N);
      tmpin = XMALLOC(y);
      if (tmpin == NULL) {
         return CRYPT_MEM;
      }

      if ((err = der_encode_sequence(siginfo, 2, tmpin, &y)) != CRYPT_OK) {
         XFREE(tmpin);
         return err;
      }
    } else {
      /* set the pointer and data-length to the input values */
      tmpin = (unsigned char *)in;
      y = inlen;
    }

    x = *outlen;
    err = pkcs_1_v1_5_encode(tmpin, y, LTC_PKCS_1_EMSA, modulus_bitlen, NULL, 0, out, &x);

    if (padding == LTC_PKCS_1_V1_5) {
      XFREE(tmpin);
    }

    if (err != CRYPT_OK) {
      return err;
    }
  }

  /* RSA encode it */
  return ltc_mp.rsa_me(out, x, out, outlen, PK_PRIVATE, key);
}

#endif /* LTC_MRSA */

#define RSA_PKCS1_PADDING_SIZE 11
/**
  Just For Audio Cast
  PKCS #1 pad then sign
  @param in        The hash to sign
  @param inlen     The length of the hash to sign (octets)
  @param out       [out] The signature
  @param outlen    [in/out] The max size and resulting size of the signature
  @param padding   Type of padding LTC_PKCS_1_V1_5
  @param key       The private RSA key to use
  @return CRYPT_OK if successful
*/
int rsa_sign_hash_ex_cast(const unsigned char *in,  unsigned long  inlen,
                          unsigned char *out,      unsigned long  *outlen,
                          int            padding,
                          rsa_key *key)
{
    LTC_ARGCHK(in       != NULL);
    LTC_ARGCHK(out      != NULL);
    LTC_ARGCHK(outlen   != NULL);
    LTC_ARGCHK(key      != NULL);
    int err = CRYPT_OK;

    /* valid padding? */
    if (padding != LTC_PKCS_1_V1_5)
    {
        return CRYPT_PK_INVALID_PADDING;
    }

    /* outlen must be at least the size of the modulus */
    unsigned long modulus_bytelen = mp_unsigned_bin_size((key->N));

    if (modulus_bytelen > *outlen)
    {
        *outlen = modulus_bytelen;
        return CRYPT_BUFFER_OVERFLOW;
    }

    /*11 == RSA PCSK1 PADDING SIZE*/
    if (inlen > modulus_bytelen - RSA_PKCS1_PADDING_SIZE)
    {
        return CRYPT_PK_INVALID_SIZE;
    }
    /* PKCS #1 v1.5 pad the hash */
    unsigned char *encoded_block = (unsigned char *)XMALLOC(modulus_bytelen);

    /*encoding LTC_PKCS_1_V1_5 padding without sha1*/
    encoded_block[0] = 0;
    encoded_block[1] = 1;
    unsigned long i = modulus_bytelen - 1 - inlen;
    encoded_block[i] = 0;

    XMEMSET(encoded_block + 2,0xff, i - 2);
    XMEMCPY(encoded_block + modulus_bytelen-inlen, in , inlen);

    /* RSA encode it */
    err = ltc_mp.rsa_me(encoded_block, modulus_bytelen, out, outlen, PK_PRIVATE, key);

    if (NULL != encoded_block)
    {
        XFREE(encoded_block);
        encoded_block = NULL;
    }

    return err;
}

/* $Source: /cvs/libtom/libtomcrypt/src/pk/rsa/rsa_sign_hash.c,v $ */
/* $Revision: 1.11 $ */
/* $Date: 2007/05/12 14:32:35 $ */

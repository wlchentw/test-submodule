#ifndef __MTK_CRYPTO_API_USER_H__
#define __MTK_CRYPTO_API_USER_H__


int aes_ecb_hw_key_encrypt(
    const unsigned char *psrc,         /* [in]  source data buffer */
    unsigned char       *pdes,         /* [out] destination data buffer, memory prepared by caller */
    unsigned int       dat_len       /* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);


int aes_ecb_hw_key_decrypt(
const unsigned char *psrc,			/* [in]  source data buffer */
unsigned char       *pdes,			/* [out] destination data buffer, memory prepared by caller */
unsigned int       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

int aes_cbc_hw_key_encrypt(
const unsigned char *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
const unsigned char *psrc,			/* [in]  source data buffer */
unsigned char       *pdes,			/* [out] destination data buffer, memory prepared by caller */
unsigned int       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);


int aes_cbc_hw_key_decrypt(
const unsigned char *piv,			/* [in]  initial vector or initial counter value buffer, 16 bytes */
const unsigned char *psrc,			/* [in]  source data buffer */
unsigned char       *pdes,			/* [out] destination data buffer, memory prepared by caller */
unsigned int       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

#define VENDOR_KEY_MODE_1	1
#define VENDOR_KEY_MODE_2	2
int aes_vendor_key_encrypt(
int mode,						   /* 1-->bounding to vendor key only 2-->bounding to vendor key + sbc key */
const unsigned char *psrc,         /* [in]  source data buffer */
unsigned char       *pdes,         /* [out] destination data buffer, memory prepared by caller */
unsigned int       dat_len       /* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);


int aes_vendor_key_decrypt(
int mode,						   /* 1-->bounding to vendor key only 2-->bounding to vendor key + sbc key */
const unsigned char *psrc,			/* [in]  source data buffer */
unsigned char       *pdes,			/* [out] destination data buffer, memory prepared by caller */
unsigned int       dat_len		/* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

#endif

/*
 * Copyright (c) 2017-2018, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <string.h>

#include "ck_helpers.h"
#include "local_utils.h"

/*
 * SKS TA returns Cryptoki like information structure.
 * These routine convert the SKS format structure and bit flags
 * from/into Cryptoki format structures and bit flags.
 */
#define MEMCPY_FIELD(_dst, _src, _f) \
	do { \
		memcpy((_dst)->_f, (_src)->_f, sizeof((_dst)->_f)); \
		if (sizeof((_dst)->_f) != sizeof((_src)->_f)) \
			return CKR_GENERAL_ERROR; \
	} while (0)

#define MEMCPY_VERSION(_dst, _src, _f) \
	do { \
		memcpy(&(_dst)->_f, (_src)->_f, sizeof(CK_VERSION)); \
		if (sizeof(CK_VERSION) != sizeof((_src)->_f)) \
			return CKR_GENERAL_ERROR; \
	} while (0)

static CK_RV sks2ck_all_slot_flags(CK_SLOT_INFO_PTR ck_info,
				   struct sks_slot_info *sks_info)
{
	CK_FLAGS ck_flag;
	uint32_t sks_mask;

	ck_info->flags = 0;
	for (sks_mask = 1; sks_mask; sks_mask <<= 1) {

		/* Skip sks token flags without a CK equilavent */
		if (sks2ck_slot_flag(&ck_flag, sks_mask))
			continue;

		if (sks_info->flags & sks_mask)
			ck_info->flags |= ck_flag;
	}

	return CKR_OK;
}

CK_RV sks2ck_slot_info(CK_SLOT_INFO_PTR ck_info,
			struct sks_slot_info *sks_info)
{
	CK_RV rv;

	MEMCPY_FIELD(ck_info, sks_info, slotDescription);
	MEMCPY_FIELD(ck_info, sks_info, manufacturerID);

	rv = sks2ck_all_slot_flags(ck_info, sks_info);
	if (rv)
		return rv;

	MEMCPY_VERSION(ck_info, sks_info, hardwareVersion);
	MEMCPY_VERSION(ck_info, sks_info, firmwareVersion);

	return CKR_OK;
}

static CK_RV sks2ck_all_token_flags(CK_TOKEN_INFO_PTR ck_info,
				struct sks_token_info *sks_info)
{
	CK_FLAGS ck_flag;
	uint32_t sks_mask;

	ck_info->flags = 0;
	for (sks_mask = 1; sks_mask; sks_mask <<= 1) {

		/* Skip sks token flags without a CK equilavent */
		if (sks2ck_token_flag(&ck_flag, sks_mask))
			continue;

		if (sks_info->flags & sks_mask)
			ck_info->flags |= ck_flag;
	}

	return CKR_OK;
}

CK_RV sks2ck_token_info(CK_TOKEN_INFO_PTR ck_info,
			struct sks_token_info *sks_info)
{
	CK_RV rv;

	MEMCPY_FIELD(ck_info, sks_info, label);
	MEMCPY_FIELD(ck_info, sks_info, manufacturerID);
	MEMCPY_FIELD(ck_info, sks_info, model);
	MEMCPY_FIELD(ck_info, sks_info, serialNumber);

	rv = sks2ck_all_token_flags(ck_info, sks_info);
	if (rv)
		return rv;

	ck_info->ulMaxSessionCount = sks_info->ulMaxSessionCount;
	ck_info->ulSessionCount = sks_info->ulSessionCount;
	ck_info->ulMaxRwSessionCount = sks_info->ulMaxRwSessionCount;
	ck_info->ulRwSessionCount = sks_info->ulRwSessionCount;
	ck_info->ulMaxPinLen = sks_info->ulMaxPinLen;
	ck_info->ulMinPinLen = sks_info->ulMinPinLen;
	ck_info->ulTotalPublicMemory = sks_info->ulTotalPublicMemory;
	ck_info->ulFreePublicMemory = sks_info->ulFreePublicMemory;
	ck_info->ulTotalPrivateMemory = sks_info->ulTotalPrivateMemory;
	ck_info->ulFreePrivateMemory = sks_info->ulFreePrivateMemory;
	MEMCPY_VERSION(ck_info, sks_info, hardwareVersion);
	MEMCPY_VERSION(ck_info, sks_info, firmwareVersion);
	MEMCPY_FIELD(ck_info, sks_info, utcTime);

	return CKR_OK;
}

CK_RV sks2ck_session_info(CK_SESSION_INFO_PTR ck_info,
			   struct sks_session_info *sks_info)
{
	ck_info->slotID = sks_info->slot_id;
	ck_info->state = sks_info->state;
	ck_info->flags = sks_info->flags;
	ck_info->ulDeviceError = sks_info->error_code;

	return CKR_OK;
}

/*
 * Helpers for CK/SKS conversions: tables of identifiers
 *
 * Define conversion tables between Cryptoki IDs and SKS 32bit IDs.
 * By convention, Cryptoki variable types CK_<XYZ> (i.e CK_ATTRIBUTE_TYPE)
 * are registered through DECLARE_CK2SKS_FUNCTIONS(<xyz>); in ck_helpers.h
 * and locally through DEFINE_CK2SKS_FUNCTIONS(<xyz>) in this source file.
 *
 * In the above description, <xyz> is the lower case equivalent of <XYZ>
 * in Cryptoki variable type definition label. I.e, for type CK_ATTRIBUTE_TYPE:
 *
 * In header file:
 *	DECLARE_CK2SKS_FUNCTIONS(attribute_type);
 * In source file:
 *	static const struct ck2sks attribute_type[] = {
 *		CK2SKS_ID_BRACE(CKA_CLASS, SKS_CKA_CLASS),
 *		CK2SKS_ID_BRACE(CKA_TOKEN, SKS_CKA_TOKEN),
 *		...
 *	};
 *	DEFINE_CK2SKS_FUNCTIONS(attribute_type, CK_ATTRIBUTE_TYPE)
 *
 * The above code snipet declares and defines functions ck2sks_attribute_type()
 * and sks2ck_attribute_type() using ID conversion array attribute type
 * defines in the source file.
 *
 * Some Cryptoki variables types have mutliple ID enumerations that would
 * conflict if merged into a single ID valid list. For exmaple the flag type
 * CK_FLAGS is used by Cryptoki to enumerate mechanism flags, token flags and
 * more. This implementation defines specific tables per ID scope.
 * I.e:
 *	mechanism_flags for CKF_<FOO> related to mechanism flags.
 *	token_flags for  CKF_<FOO> related to token flags.
 */
struct ck2sks {
	CK_ULONG ck;
	uint32_t sks;
	// TODO: string for both IDs
};

/*
 * Macros to define the SKS identifier relate to a Cryptoki identifier.
 * Use CK2SKS_ID() when SKS identifier label is SKS_<CK-label>.
 * Use CK2SKS_BRACE() when specific SKS identifier regarding Cryptoki CK label.
 */
#define CK2SKS_ID(ck_id)		{ .ck = ck_id, .sks = SKS_ ## ck_id }
#define CK2SKS_ID_BRACE(ck_id, sks_id)	{ .ck = ck_id, .sks = sks_id }

#define SKS2CK(out, in, conv)		sks2ck(out, in, conv, ARRAY_SIZE(conv))
#define CK2SKS(out, in, conv)		ck2sks(out, in, conv, ARRAY_SIZE(conv))

#define DEFINE_CK2SKS_FUNCTIONS(_conv_table, _ck_typeof)	\
	uint32_t ck2sks_ ## _conv_table(_ck_typeof ck)		\
	{							\
		uint32_t id;					\
								\
		if (CK2SKS(&id, ck, _conv_table))		\
			return SKS_UNDEFINED_ID;		\
								\
		return id;					\
	}							\
	CK_RV sks2ck_ ## _conv_table(_ck_typeof *ck, uint32_t sks)	\
	{							\
		if (SKS2CK(ck, sks, _conv_table))		\
			return CKR_GENERAL_ERROR;		\
								\
		return CKR_OK;					\
	}

static int sks2ck(CK_ULONG *out, uint32_t id,
		  const struct ck2sks *conv, size_t count)
{
	size_t n;

	for (n = 0; n < count; n++) {
		if (id == conv[n].sks) {
			*out = conv[n].ck;
			return 0;
		}
	}

	return -1;
}

static int ck2sks(uint32_t *out, CK_ULONG id,
		  const struct ck2sks *conv, size_t count)
{
	size_t n;

	for (n = 0; n < count; n++) {
		if (id == conv[n].ck) {
			*out = conv[n].sks;
			return 0;
		}
	}

	return -1;
}

/*
 * Identifiers conversion tables and related functions definitions.
 * Generic way goes:
 *
 * static const struct ck2sks <foo>[] = {
 *		CK2SKS_ID_BRACE(CK[<X>]_<Y>),
 *		CK2SKS_ID_BRACE(CK[<X>]_<Y>, SKS_<Z>),
 * };
 *
 * DEFINE_CK2SKS_FUNCTIONS(<foo>, CK_<related-type-label>)
 */
static const struct ck2sks slot_flag[] = {
	CK2SKS_ID_BRACE(CKF_TOKEN_PRESENT, SKS_CKFS_TOKEN_PRESENT),
	CK2SKS_ID_BRACE(CKF_REMOVABLE_DEVICE, SKS_CKFS_REMOVABLE_DEVICE),
	CK2SKS_ID_BRACE(CKF_HW_SLOT, SKS_CKFS_HW_SLOT),
};

DEFINE_CK2SKS_FUNCTIONS(slot_flag, CK_FLAGS)

static const struct ck2sks token_flag[] = {
	CK2SKS_ID_BRACE(CKF_RNG,
			SKS_CKFT_RNG),
	CK2SKS_ID_BRACE(CKF_WRITE_PROTECTED,
			SKS_CKFT_WRITE_PROTECTED),
	CK2SKS_ID_BRACE(CKF_LOGIN_REQUIRED,
			SKS_CKFT_LOGIN_REQUIRED),
	CK2SKS_ID_BRACE(CKF_USER_PIN_INITIALIZED,
			SKS_CKFT_USER_PIN_INITIALIZED),
	CK2SKS_ID_BRACE(CKF_RESTORE_KEY_NOT_NEEDED,
			SKS_CKFT_RESTORE_KEY_NOT_NEEDED),
	CK2SKS_ID_BRACE(CKF_CLOCK_ON_TOKEN,
			SKS_CKFT_CLOCK_ON_TOKEN),
	CK2SKS_ID_BRACE(CKF_PROTECTED_AUTHENTICATION_PATH,
			SKS_CKFT_PROTECTED_AUTHENTICATION_PATH),
	CK2SKS_ID_BRACE(CKF_DUAL_CRYPTO_OPERATIONS,
			SKS_CKFT_DUAL_CRYPTO_OPERATIONS),
	CK2SKS_ID_BRACE(CKF_TOKEN_INITIALIZED,
			SKS_CKFT_TOKEN_INITIALIZED),
	CK2SKS_ID_BRACE(CKF_USER_PIN_COUNT_LOW,
			SKS_CKFT_USER_PIN_COUNT_LOW),
	CK2SKS_ID_BRACE(CKF_USER_PIN_FINAL_TRY,
			SKS_CKFT_USER_PIN_FINAL_TRY),
	CK2SKS_ID_BRACE(CKF_USER_PIN_LOCKED,
			SKS_CKFT_USER_PIN_LOCKED),
	CK2SKS_ID_BRACE(CKF_USER_PIN_TO_BE_CHANGED,
			SKS_CKFT_USER_PIN_TO_BE_CHANGED),
	CK2SKS_ID_BRACE(CKF_SO_PIN_COUNT_LOW,
			SKS_CKFT_SO_PIN_COUNT_LOW),
	CK2SKS_ID_BRACE(CKF_SO_PIN_FINAL_TRY,
			SKS_CKFT_SO_PIN_FINAL_TRY),
	CK2SKS_ID_BRACE(CKF_SO_PIN_LOCKED,
			SKS_CKFT_SO_PIN_LOCKED),
	CK2SKS_ID_BRACE(CKF_SO_PIN_TO_BE_CHANGED,
			SKS_CKFT_SO_PIN_TO_BE_CHANGED),
	CK2SKS_ID_BRACE(CKF_ERROR_STATE,
			SKS_CKFT_ERROR_STATE),
};

DEFINE_CK2SKS_FUNCTIONS(token_flag, CK_FLAGS)

static const struct ck2sks attribute_type[] = {
	CK2SKS_ID(CKA_CLASS),
	CK2SKS_ID(CKA_KEY_TYPE),
	CK2SKS_ID(CKA_VALUE),
	CK2SKS_ID(CKA_VALUE_LEN),
	CK2SKS_ID(CKA_LABEL),
	CK2SKS_ID(CKA_WRAP_TEMPLATE),
	CK2SKS_ID(CKA_UNWRAP_TEMPLATE),
	CK2SKS_ID(CKA_DERIVE_TEMPLATE),
	CK2SKS_ID(CKA_START_DATE),
	CK2SKS_ID(CKA_END_DATE),
	CK2SKS_ID(CKA_OBJECT_ID),
	CK2SKS_ID(CKA_APPLICATION),
	CK2SKS_ID(CKA_MECHANISM_TYPE),
	CK2SKS_ID(CKA_ID),
	CK2SKS_ID(CKA_ALLOWED_MECHANISMS),
	CK2SKS_ID(CKA_EC_PARAMS),
	CK2SKS_ID(CKA_EC_POINT),
	CK2SKS_ID(CKA_MODULUS),
	CK2SKS_ID(CKA_MODULUS_BITS),
	CK2SKS_ID(CKA_PUBLIC_EXPONENT),
	CK2SKS_ID(CKA_PRIVATE_EXPONENT),
	CK2SKS_ID(CKA_PRIME_1),
	CK2SKS_ID(CKA_PRIME_2),
	CK2SKS_ID(CKA_EXPONENT_1),
	CK2SKS_ID(CKA_EXPONENT_2),
	CK2SKS_ID(CKA_COEFFICIENT),
	CK2SKS_ID(CKA_SUBJECT),
	CK2SKS_ID(CKA_PUBLIC_KEY_INFO),
	CK2SKS_ID(CKA_CERTIFICATE_TYPE),
	CK2SKS_ID(CKA_CERTIFICATE_CATEGORY),
	CK2SKS_ID(CKA_ISSUER),
	CK2SKS_ID(CKA_SERIAL_NUMBER),
	CK2SKS_ID(CKA_URL),
	CK2SKS_ID(CKA_HASH_OF_SUBJECT_PUBLIC_KEY),
	CK2SKS_ID(CKA_HASH_OF_ISSUER_PUBLIC_KEY),
	CK2SKS_ID(CKA_NAME_HASH_ALGORITHM),
	CK2SKS_ID(CKA_KEY_GEN_MECHANISM),
	/* Below are boolean attributes */
	CK2SKS_ID(CKA_TOKEN),
	CK2SKS_ID(CKA_PRIVATE),
	CK2SKS_ID(CKA_TRUSTED),
	CK2SKS_ID(CKA_SENSITIVE),
	CK2SKS_ID(CKA_ENCRYPT),
	CK2SKS_ID(CKA_DECRYPT),
	CK2SKS_ID(CKA_WRAP),
	CK2SKS_ID(CKA_UNWRAP),
	CK2SKS_ID(CKA_SIGN),
	CK2SKS_ID(CKA_SIGN_RECOVER),
	CK2SKS_ID(CKA_VERIFY),
	CK2SKS_ID(CKA_VERIFY_RECOVER),
	CK2SKS_ID(CKA_DERIVE),
	CK2SKS_ID(CKA_EXTRACTABLE),
	CK2SKS_ID(CKA_LOCAL),
	CK2SKS_ID(CKA_NEVER_EXTRACTABLE),
	CK2SKS_ID(CKA_ALWAYS_SENSITIVE),
	CK2SKS_ID(CKA_MODIFIABLE),
	CK2SKS_ID(CKA_COPYABLE),
	CK2SKS_ID(CKA_DESTROYABLE),
	CK2SKS_ID(CKA_ALWAYS_AUTHENTICATE),
	CK2SKS_ID(CKA_WRAP_WITH_TRUSTED),
	/* Specifc SKS attribute IDs */
	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

DEFINE_CK2SKS_FUNCTIONS(attribute_type, CK_ATTRIBUTE_TYPE)

static const struct ck2sks mechanism_type[] = {
	CK2SKS_ID(CKM_MD5),
	CK2SKS_ID(CKM_SHA_1),
	CK2SKS_ID(CKM_SHA224),
	CK2SKS_ID(CKM_SHA256),
	CK2SKS_ID(CKM_SHA384),
	CK2SKS_ID(CKM_SHA512),

	CK2SKS_ID(CKM_AES_ECB),
	CK2SKS_ID(CKM_AES_CBC),
	CK2SKS_ID(CKM_AES_CBC_PAD),
	CK2SKS_ID(CKM_AES_CTR),
	CK2SKS_ID(CKM_AES_GCM),
	CK2SKS_ID(CKM_AES_CCM),
	CK2SKS_ID(CKM_AES_CTS),
	CK2SKS_ID(CKM_AES_GMAC),
	CK2SKS_ID(CKM_AES_CMAC),
	CK2SKS_ID(CKM_AES_CMAC_GENERAL),
	CK2SKS_ID(CKM_AES_ECB_ENCRYPT_DATA),
	CK2SKS_ID(CKM_AES_CBC_ENCRYPT_DATA),
	CK2SKS_ID(CKM_AES_KEY_GEN),
	CK2SKS_ID(CKM_AES_XCBC_MAC),

	CK2SKS_ID(CKM_GENERIC_SECRET_KEY_GEN),

	CK2SKS_ID(CKM_MD5_HMAC),
	CK2SKS_ID(CKM_SHA_1_HMAC),
	CK2SKS_ID(CKM_SHA224_HMAC),
	CK2SKS_ID(CKM_SHA256_HMAC),
	CK2SKS_ID(CKM_SHA384_HMAC),
	CK2SKS_ID(CKM_SHA512_HMAC),

	CK2SKS_ID(CKM_EC_KEY_PAIR_GEN),
	CK2SKS_ID(CKM_ECDSA),
	CK2SKS_ID(CKM_ECDSA_SHA1),
	CK2SKS_ID(CKM_ECDSA_SHA224),
	CK2SKS_ID(CKM_ECDSA_SHA256),
	CK2SKS_ID(CKM_ECDSA_SHA384),
	CK2SKS_ID(CKM_ECDSA_SHA512),
	CK2SKS_ID(CKM_ECDH1_DERIVE),
	CK2SKS_ID(CKM_ECDH1_COFACTOR_DERIVE),
	CK2SKS_ID(CKM_ECMQV_DERIVE),
	CK2SKS_ID(CKM_ECDH_AES_KEY_WRAP),

	CK2SKS_ID(CKM_RSA_PKCS_KEY_PAIR_GEN),
	CK2SKS_ID(CKM_RSA_PKCS),
	CK2SKS_ID(CKM_RSA_9796),
	CK2SKS_ID(CKM_RSA_X_509),
	CK2SKS_ID(CKM_SHA1_RSA_PKCS),
	CK2SKS_ID(CKM_RSA_PKCS_OAEP),
	CK2SKS_ID(CKM_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_SHA1_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_SHA256_RSA_PKCS),
	CK2SKS_ID(CKM_SHA384_RSA_PKCS),
	CK2SKS_ID(CKM_SHA512_RSA_PKCS),
	CK2SKS_ID(CKM_SHA256_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_SHA384_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_SHA512_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_SHA224_RSA_PKCS),
	CK2SKS_ID(CKM_SHA224_RSA_PKCS_PSS),
	CK2SKS_ID(CKM_RSA_AES_KEY_WRAP),

	CK2SKS_ID(CKM_DES_KEY_GEN),
	CK2SKS_ID(CKM_DES_ECB),
	CK2SKS_ID(CKM_DES_CBC),
	CK2SKS_ID(CKM_DES_MAC),
	CK2SKS_ID(CKM_DES_MAC_GENERAL),
	CK2SKS_ID(CKM_DES_CBC_PAD),

	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

DEFINE_CK2SKS_FUNCTIONS(mechanism_type, CK_MECHANISM_TYPE)

static const struct ck2sks mechanism_flag[] = {
	CK2SKS_ID_BRACE(CKF_HW, SKS_CKFM_HW),
	CK2SKS_ID_BRACE(CKF_ENCRYPT, SKS_CKFM_ENCRYPT),
	CK2SKS_ID_BRACE(CKF_DECRYPT, SKS_CKFM_DECRYPT),
	CK2SKS_ID_BRACE(CKF_DIGEST, SKS_CKFM_DIGEST),
	CK2SKS_ID_BRACE(CKF_SIGN, SKS_CKFM_SIGN),
	CK2SKS_ID_BRACE(CKF_SIGN_RECOVER, SKS_CKFM_SIGN_RECOVER),
	CK2SKS_ID_BRACE(CKF_VERIFY, SKS_CKFM_VERIFY),
	CK2SKS_ID_BRACE(CKF_VERIFY_RECOVER, SKS_CKFM_VERIFY_RECOVER),
	CK2SKS_ID_BRACE(CKF_GENERATE, SKS_CKFM_GENERATE),
	CK2SKS_ID_BRACE(CKF_GENERATE_KEY_PAIR, SKS_CKFM_GENERATE_PAIR),
	CK2SKS_ID_BRACE(CKF_WRAP, SKS_CKFM_WRAP),
	CK2SKS_ID_BRACE(CKF_UNWRAP, SKS_CKFM_UNWRAP),
	CK2SKS_ID_BRACE(CKF_DERIVE, SKS_CKFM_DERIVE),
	CK2SKS_ID_BRACE(CKF_EC_F_P, SKS_CKFM_EC_F_P),
	CK2SKS_ID_BRACE(CKF_EC_F_2M, SKS_CKFM_EC_F_2M),
	CK2SKS_ID_BRACE(CKF_EC_ECPARAMETERS, SKS_CKFM_EC_ECPARAMETERS),
	CK2SKS_ID_BRACE(CKF_EC_NAMEDCURVE, SKS_CKFM_EC_NAMEDCURVE),
	CK2SKS_ID_BRACE(CKF_EC_UNCOMPRESS, SKS_CKFM_EC_UNCOMPRESS),
	CK2SKS_ID_BRACE(CKF_EC_COMPRESS, SKS_CKFM_EC_COMPRESS),
};

DEFINE_CK2SKS_FUNCTIONS(mechanism_flag, CK_FLAGS)

static const struct ck2sks object_class[] = {
	CK2SKS_ID(CKO_SECRET_KEY),
	CK2SKS_ID(CKO_PUBLIC_KEY),
	CK2SKS_ID(CKO_PRIVATE_KEY),
	CK2SKS_ID(CKO_OTP_KEY),
	CK2SKS_ID(CKO_CERTIFICATE),
	CK2SKS_ID(CKO_DATA),
	CK2SKS_ID(CKO_DOMAIN_PARAMETERS),
	CK2SKS_ID(CKO_HW_FEATURE),
	CK2SKS_ID(CKO_MECHANISM),
	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

DEFINE_CK2SKS_FUNCTIONS(object_class, CK_OBJECT_CLASS)

static const struct ck2sks key_type[] = {
	CK2SKS_ID(CKK_AES),
	CK2SKS_ID(CKK_GENERIC_SECRET),
	CK2SKS_ID(CKK_MD5_HMAC),
	CK2SKS_ID(CKK_SHA_1_HMAC),
	CK2SKS_ID(CKK_SHA224_HMAC),
	CK2SKS_ID(CKK_SHA256_HMAC),
	CK2SKS_ID(CKK_SHA384_HMAC),
	CK2SKS_ID(CKK_SHA512_HMAC),
	CK2SKS_ID(CKK_RSA),
	CK2SKS_ID(CKK_EC),
	CK2SKS_ID(CKK_DSA),
	CK2SKS_ID(CKK_DH),
	CK2SKS_ID(CKK_DES),
	CK2SKS_ID(CKK_DES2),
	CK2SKS_ID(CKK_DES3),
	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

DEFINE_CK2SKS_FUNCTIONS(key_type, CK_KEY_TYPE)

static const struct ck2sks certificate_type[] = {
	CK2SKS_ID(CKC_X_509),
	CK2SKS_ID(CKC_X_509_ATTR_CER),
	CK2SKS_ID(CKC_WTLS),
	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

DEFINE_CK2SKS_FUNCTIONS(certificate_type, CK_CERTIFICATE_TYPE)

static const struct ck2sks ec_kdf_type[] = {
	CK2SKS_ID(CKD_NULL),
	CK2SKS_ID(CKD_SHA1_KDF),
	CK2SKS_ID(CKD_SHA1_KDF_ASN1),
	CK2SKS_ID(CKD_SHA1_KDF_CONCATENATE),
	CK2SKS_ID(CKD_SHA224_KDF),
	CK2SKS_ID(CKD_SHA256_KDF),
	CK2SKS_ID(CKD_SHA384_KDF),
	CK2SKS_ID(CKD_SHA512_KDF),
	CK2SKS_ID(CKD_CPDIVERSIFY_KDF),
};

DEFINE_CK2SKS_FUNCTIONS(ec_kdf_type, CK_EC_KDF_TYPE)

static const struct ck2sks rsa_pkcs_mgf_type[] = {
	CK2SKS_ID(CKG_MGF1_SHA1),
	CK2SKS_ID(CKG_MGF1_SHA224),
	CK2SKS_ID(CKG_MGF1_SHA256),
	CK2SKS_ID(CKG_MGF1_SHA384),
	CK2SKS_ID(CKG_MGF1_SHA512),
};

DEFINE_CK2SKS_FUNCTIONS(rsa_pkcs_mgf_type, CK_RSA_PKCS_MGF_TYPE)

static const struct ck2sks rsa_pkcs_oaep_source_type[] = {
	CK2SKS_ID(CKZ_DATA_SPECIFIED),
};

DEFINE_CK2SKS_FUNCTIONS(rsa_pkcs_oaep_source_type, CK_RSA_PKCS_OAEP_SOURCE_TYPE)

static const struct ck2sks user_type[] = {
	CK2SKS_ID(CKU_SO),
	CK2SKS_ID(CKU_USER),
	CK2SKS_ID(CKU_CONTEXT_SPECIFIC),
};

DEFINE_CK2SKS_FUNCTIONS(user_type, CK_USER_TYPE)

static const struct ck2sks error_code[] = {
	CK2SKS_ID(CKR_OK),
	CK2SKS_ID(CKR_GENERAL_ERROR),
	CK2SKS_ID(CKR_DEVICE_MEMORY),
	CK2SKS_ID(CKR_ARGUMENTS_BAD),
	CK2SKS_ID(CKR_BUFFER_TOO_SMALL),
	CK2SKS_ID(CKR_FUNCTION_FAILED),
	CK2SKS_ID(CKR_ATTRIBUTE_TYPE_INVALID),
	CK2SKS_ID(CKR_ATTRIBUTE_VALUE_INVALID),
	CK2SKS_ID(CKR_OBJECT_HANDLE_INVALID),
	CK2SKS_ID(CKR_KEY_HANDLE_INVALID),
	CK2SKS_ID(CKR_MECHANISM_INVALID),
	CK2SKS_ID(CKR_SLOT_ID_INVALID),
	CK2SKS_ID(CKR_SESSION_HANDLE_INVALID),
	CK2SKS_ID(CKR_PIN_INCORRECT),
	CK2SKS_ID(CKR_PIN_LOCKED),
	CK2SKS_ID(CKR_PIN_EXPIRED),
	CK2SKS_ID(CKR_PIN_INVALID),
	CK2SKS_ID(CKR_OPERATION_ACTIVE),
	CK2SKS_ID(CKR_KEY_FUNCTION_NOT_PERMITTED),
	CK2SKS_ID(CKR_KEY_TYPE_INCONSISTENT),
	CK2SKS_ID(CKR_OPERATION_NOT_INITIALIZED),
	CK2SKS_ID(CKR_SESSION_READ_ONLY),
	CK2SKS_ID(CKR_MECHANISM_PARAM_INVALID),
	CK2SKS_ID(CKR_TOKEN_WRITE_PROTECTED),
	CK2SKS_ID(CKR_TOKEN_NOT_PRESENT),
	CK2SKS_ID(CKR_TOKEN_NOT_RECOGNIZED),
	CK2SKS_ID(CKR_ACTION_PROHIBITED),
	CK2SKS_ID(CKR_ATTRIBUTE_READ_ONLY),
	CK2SKS_ID(CKR_PIN_TOO_WEAK),
	CK2SKS_ID(CKR_CURVE_NOT_SUPPORTED),
	CK2SKS_ID(CKR_DOMAIN_PARAMS_INVALID),
	CK2SKS_ID(CKR_USER_ALREADY_LOGGED_IN),
	CK2SKS_ID(CKR_USER_ANOTHER_ALREADY_LOGGED_IN),
	CK2SKS_ID(CKR_USER_NOT_LOGGED_IN),
	CK2SKS_ID(CKR_USER_PIN_NOT_INITIALIZED),
	CK2SKS_ID(CKR_USER_TOO_MANY_TYPES),
	CK2SKS_ID(CKR_USER_TYPE_INVALID),
	CK2SKS_ID(CKR_SESSION_READ_ONLY_EXISTS),
	CK2SKS_ID(CKR_SESSION_READ_WRITE_SO_EXISTS),
	CK2SKS_ID(CKR_SESSION_EXISTS),
	CK2SKS_ID(CKR_TEMPLATE_INCOMPLETE),
	CK2SKS_ID(CKR_TEMPLATE_INCONSISTENT),
	CK2SKS_ID(CKR_SIGNATURE_INVALID),
	CK2SKS_ID(CKR_ATTRIBUTE_SENSITIVE),
	CK2SKS_ID(CKR_SIGNATURE_LEN_RANGE),
	CK2SKS_ID(CKR_DATA_LEN_RANGE),
	CK2SKS_ID(CKR_ENCRYPTED_DATA_LEN_RANGE),
	CK2SKS_ID_BRACE(CK_VENDOR_INVALID_ID, SKS_UNDEFINED_ID),
};

CK_RV sks2ck_rv(uint32_t sks)
{
	CK_ULONG rv;

	if (SKS2CK(&rv, sks, error_code))
		return CKR_GENERAL_ERROR;

	return (CK_RV)rv;
}

CK_RV teec2ck_rv(TEEC_Result res)
{
	switch (res) {
	case TEEC_SUCCESS:
		return CKR_OK;
	case TEEC_ERROR_OUT_OF_MEMORY:
		return CKR_DEVICE_MEMORY;
	case TEEC_ERROR_BAD_PARAMETERS:
		return CKR_ARGUMENTS_BAD;
	case TEEC_ERROR_SHORT_BUFFER:
		return CKR_BUFFER_TOO_SMALL;
	default:
		return CKR_FUNCTION_FAILED;
	}
}
/* Convert a array of mechanism type from sks into CK_MECHANIMS_TYPE */
CK_RV sks2ck_mechanism_type_list(CK_MECHANISM_TYPE *dst,
				 void *src, size_t count)
{
	CK_MECHANISM_TYPE *ck = dst;
	char *sks = src;
	size_t n;
	uint32_t proc;

	for (n = 0; n < count; n++, sks += sizeof(uint32_t), ck++) {
		memcpy(&proc, sks, sizeof(proc));
		if (sks2ck_mechanism_type(ck, proc))
			return CKR_MECHANISM_INVALID;
	}

	return CKR_OK;
}

/* Convert structure CK_MECHANIMS_INFO from sks to ck (3 ulong fields) */
CK_RV sks2ck_mechanism_info(CK_MECHANISM_INFO *info, void *src)
{
	struct sks_mechanism_info sks;
	CK_FLAGS ck_flag;
	uint32_t mask;
	CK_RV rv;

	memcpy(&sks, src, sizeof(sks));

	info->ulMinKeySize = sks.min_key_size;
	info->ulMaxKeySize = sks.max_key_size;

	info->flags = 0;
	for (mask = 1; mask; mask <<= 1) {
		if (!(sks.flags & mask))
			continue;

		rv = sks2ck_mechanism_flag(&ck_flag, mask);
		if (rv)
			return rv;

		info->flags |= ck_flag;
	}

	return CKR_OK;
}

/*
 * Helper functions to analyse CK fields
 */
size_t ck_attr_is_class(uint32_t id)
{
	if (id == CKA_CLASS)
		return sizeof(CK_ULONG);
	else
		return 0;
}

size_t ck_attr_is_type(uint32_t id)
{
	switch (id) {
	case CKA_CERTIFICATE_TYPE:
	case CKA_KEY_TYPE:
	case CKA_HW_FEATURE_TYPE:
	case CKA_MECHANISM_TYPE:
	case CKA_KEY_GEN_MECHANISM:
		return sizeof(CK_ULONG);
	default:
		return 0;
	}
}
int sks_object_has_boolprop(uint32_t class)
{
	switch (class) {
	case SKS_CKO_DATA:
	case SKS_CKO_CERTIFICATE:
	case SKS_CKO_PUBLIC_KEY:
	case SKS_CKO_PRIVATE_KEY:
	case SKS_CKO_SECRET_KEY:
	case SKS_CKO_DOMAIN_PARAMETERS:
		return 1;
	default:
		return 0;
	}
}
int sks_class_has_type(uint32_t class)
{
	switch (class) {
	case SKS_CKO_CERTIFICATE:
	case SKS_CKO_PUBLIC_KEY:
	case SKS_CKO_PRIVATE_KEY:
	case SKS_CKO_SECRET_KEY:
	case SKS_CKO_MECHANISM:
	case SKS_CKO_HW_FEATURE:
		return 1;
	default:
		return 0;
	}
}

uint32_t ck2sks_type_in_class(CK_ULONG ck, CK_ULONG class)
{
	switch (class) {
	case CKO_DATA:
		return 0;
	case CKO_SECRET_KEY:
	case CKO_PUBLIC_KEY:
	case CKO_PRIVATE_KEY:
	case CKO_OTP_KEY:
		return ck2sks_key_type(ck);
	case CKO_MECHANISM:
		return ck2sks_mechanism_type(ck);
	case CKO_CERTIFICATE:
		return ck2sks_certificate_type(ck);
	default:
		return SKS_UNDEFINED_ID;
	}
}

CK_RV sks2ck_type_in_class(CK_ULONG *ck, uint32_t sks, CK_ULONG class)
{
	switch (class) {
	case SKS_CKO_DATA:
		return CKR_NO_EVENT;
	case SKS_CKO_SECRET_KEY:
	case SKS_CKO_PUBLIC_KEY:
	case SKS_CKO_PRIVATE_KEY:
	case SKS_CKO_OTP_KEY:
		return sks2ck_key_type(ck, sks);
	case SKS_CKO_MECHANISM:
		return sks2ck_mechanism_type(ck, sks);
	case SKS_CKO_CERTIFICATE:
		return sks2ck_certificate_type(ck, sks);
	default:
		return CKR_GENERAL_ERROR;
	}
}

CK_RV ck_add_object_class(CK_ATTRIBUTE_PTR attrs, CK_ULONG_PTR count,
			CK_ATTRIBUTE_PTR *attrs_new_p,CK_OBJECT_CLASS cka_class)
{
	size_t n;
	CK_ULONG count_new = *count;
	CK_ATTRIBUTE_PTR attrs_new = NULL;
	CK_OBJECT_CLASS_PTR object_class_p = NULL;
	int object_class_present = 0;
	CK_RV rv;

	for (n = 0; n < *count; n++) {
		if (attrs[n].type == CKA_CLASS) {
			object_class_present = 1;
			break;
		}
	}
	if (!object_class_present)
		count_new++;

	attrs_new = malloc(count_new * sizeof(CK_ATTRIBUTE));
	if (attrs_new == NULL)
		return CKR_HOST_MEMORY;

	memcpy(attrs_new, attrs, (*count) * sizeof(CK_ATTRIBUTE));

	if (object_class_present) {
		rv = CKR_OK;
		goto bail;
	}

	object_class_p = malloc(sizeof(CK_OBJECT_CLASS));
	if (object_class_p == NULL) {
		rv = CKR_HOST_MEMORY;
		goto bail;
	}

	*object_class_p = cka_class;
	attrs_new[count_new - 1].type = CKA_CLASS;
	attrs_new[count_new - 1].pValue = object_class_p;
	attrs_new[count_new - 1].ulValueLen = sizeof(CK_OBJECT_CLASS);
	rv = CKR_OK;

bail:
	if (rv == CKR_OK) {
		if(*attrs_new_p == attrs)
			free(attrs);
		*attrs_new_p = attrs_new;
		*count = count_new;
	} else {
		if (attrs_new)
			free(attrs_new);
		if (object_class_p)
			free(object_class_p);
	}

	return rv;
}

CK_RV ck_guess_key_type(CK_MECHANISM_PTR mecha,
			CK_ATTRIBUTE_PTR attrs, CK_ULONG_PTR count,
			CK_ATTRIBUTE_PTR *attrs_new_p)
{
	size_t n;
	CK_ULONG count_new = *count;
	CK_ATTRIBUTE_PTR attrs_new = NULL;
	CK_KEY_TYPE_PTR key_type_p = NULL;
	int key_type_present = 0;
	CK_RV rv;

	for (n = 0; n < *count; n++) {
		if (attrs[n].type == CKA_KEY_TYPE) {
			key_type_present = 1;
			break;
		}
	}
	if (!key_type_present)
		count_new++;

	attrs_new = malloc(count_new * sizeof(CK_ATTRIBUTE));
	if (attrs_new == NULL)
		return CKR_HOST_MEMORY;

	memcpy(attrs_new, attrs, (*count) * sizeof(CK_ATTRIBUTE));

	if (key_type_present) {
		rv = CKR_OK;
		goto bail;
	}

	key_type_p = malloc(sizeof(CK_KEY_TYPE));
	if (key_type_p == NULL) {
		rv = CKR_HOST_MEMORY;
		goto bail;
	}

	switch (mecha->mechanism) {
	case CKM_RSA_PKCS_KEY_PAIR_GEN:
		*key_type_p = CKK_RSA;
		attrs_new[count_new - 1].type = CKA_KEY_TYPE;
		attrs_new[count_new - 1].pValue = key_type_p;
		attrs_new[count_new - 1].ulValueLen = sizeof(CK_KEY_TYPE);
		rv = CKR_OK;
		break;
	case CKM_EC_KEY_PAIR_GEN:
		*key_type_p = CKK_EC;
		attrs_new[count_new - 1].type = CKA_KEY_TYPE;
		attrs_new[count_new - 1].pValue = key_type_p;
		attrs_new[count_new - 1].ulValueLen = sizeof(CK_KEY_TYPE);
		rv = CKR_OK;
		break;
	default:
		rv = CKR_TEMPLATE_INCOMPLETE;
		break;
	}

bail:
	if (rv == CKR_OK) {
		*attrs_new_p = attrs_new;
		*count = count_new;
	} else {
		if (attrs_new)
			free(attrs_new);
		if (key_type_p)
			free(key_type_p);
	}

	return rv;
}

/*
 * Copyright (c) 2017-2018, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __HELPERS_CK_H
#define __HELPERS_CK_H

#include <pkcs11.h>
#include <stdint.h>
#include <stddef.h>
#include <sks_ta.h>
#include <tee_client_api.h>

/*
 * SKS reserves vendor ID 0xffffffff to represent an invalid ID
 * (attribute class, type, ...)
 */
#define CK_VENDOR_INVALID_ID		0xffffffffUL
#define SKS_CK_VENDOR_INVALID_ID	0xffffffffUL

/* Helper for sks2ck_xxx() and ck2sks_xxx() helper declaration */
#define DECLARE_CK2SKS_FUNCTIONS(_label, _ck_typeof)		\
	uint32_t ck2sks_ ## _label(_ck_typeof ck);	\
	CK_RV sks2ck_ ## _label(_ck_typeof *ck, uint32_t sks)

DECLARE_CK2SKS_FUNCTIONS(slot_flag, CK_FLAGS);
DECLARE_CK2SKS_FUNCTIONS(token_flag, CK_FLAGS);
DECLARE_CK2SKS_FUNCTIONS(user_type, CK_USER_TYPE);
DECLARE_CK2SKS_FUNCTIONS(attribute_type, CK_ATTRIBUTE_TYPE);
DECLARE_CK2SKS_FUNCTIONS(mechanism_type, CK_MECHANISM_TYPE);
DECLARE_CK2SKS_FUNCTIONS(mechanism_flag, CK_FLAGS);
DECLARE_CK2SKS_FUNCTIONS(object_class, CK_OBJECT_CLASS);
DECLARE_CK2SKS_FUNCTIONS(key_type, CK_KEY_TYPE);
DECLARE_CK2SKS_FUNCTIONS(ec_kdf_type, CK_EC_KDF_TYPE);
DECLARE_CK2SKS_FUNCTIONS(rsa_pkcs_mgf_type, CK_RSA_PKCS_MGF_TYPE);
DECLARE_CK2SKS_FUNCTIONS(rsa_pkcs_oaep_source_type,
			 CK_RSA_PKCS_OAEP_SOURCE_TYPE);
DECLARE_CK2SKS_FUNCTIONS(certificate_type, CK_CERTIFICATE_TYPE);

/*
 * Convert structure struct sks_token_info retreived from TA into a
 * cryptoki API compliant CK_TOKEN_INFO structure.
 *
 * struct sks_token_info is defined in the SKS TA API.
 */
CK_RV sks2ck_token_info(CK_TOKEN_INFO_PTR ck_info,
			struct sks_token_info *sks_info);
CK_RV sks2ck_slot_info(CK_SLOT_INFO_PTR ck_info,
			struct sks_slot_info *sks_info);
CK_RV sks2ck_session_info(CK_SESSION_INFO_PTR ck_info,
			struct sks_session_info *sks_info);

/* Backward compat on deprecated functions */
static inline CK_RV sks2ck_attribute_id(CK_ATTRIBUTE_TYPE *ck, uint32_t sks)
{
	return sks2ck_attribute_type(ck, sks);
}

static inline uint32_t ck2sks_attribute_id(CK_ATTRIBUTE_TYPE ck)
{
	return ck2sks_attribute_type(ck);
}

static inline CK_RV sks2ck_class(CK_OBJECT_CLASS *ck, uint32_t sks)
{
	return sks2ck_object_class(ck, sks);
}

static inline uint32_t ck2sks_class(CK_OBJECT_CLASS ck)
{
	return ck2sks_object_class(ck);
}

CK_RV sks2ck_mechanism_type_list(CK_MECHANISM_TYPE *dst, void *sks,
				 size_t count);
CK_RV sks2ck_mechanism_info(CK_MECHANISM_INFO *info, void *sks);

uint32_t ck2sks_type_in_class(CK_ULONG ck, CK_ULONG class);
CK_RV sks2ck_type_in_class(CK_ULONG *ck, uint32_t sks, CK_ULONG class);

int sks_attr2boolprop_shift(CK_ULONG attr);

CK_RV sks2ck_rv(uint32_t sks);
CK_RV teec2ck_rv(TEEC_Result res);

/*
 * Helper functions to analyse CK fields
 */
size_t ck_attr_is_class(uint32_t attribute_id);
size_t ck_attr_is_type(uint32_t attribute_id);
int ck_attr2boolprop_shift(CK_ULONG attr);

int sks_object_has_boolprop(uint32_t class);
int sks_class_has_type(uint32_t class);

/*
 * Try to guess key type if mechanism is key generation
 *
 * @mech: Referecn to mechanism
 * @attrs; Reference to input object attributes
 * @count: number of attributes for the object, output value may be incremented
 * @attrs_new_p: Referenece to output attributes, always defines the key type
 *
 * This may be needed because some tools (e.g.: pkcs11-tool) may not specify
 * some attributes as key type when these can be assumed from the mechanism
 * type.
 *
 * The function allocates memory for a copy of the attributes since it could
 * be increased when adding the missing attribute. Caller is responsible from
 * freeing the output attribute references.
 */
CK_RV ck_guess_key_type(CK_MECHANISM_PTR mecha,
		       CK_ATTRIBUTE_PTR attrs, CK_ULONG_PTR count,
		       CK_ATTRIBUTE_PTR *attrs_new_p);

CK_RV ck_add_object_class(CK_ATTRIBUTE_PTR attrs, CK_ULONG_PTR count,
			CK_ATTRIBUTE_PTR *attrs_new_p,CK_OBJECT_CLASS cka_class);

#endif /*__HELPERS_CK_H*/

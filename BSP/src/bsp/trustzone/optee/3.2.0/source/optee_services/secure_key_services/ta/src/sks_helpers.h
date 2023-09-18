/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017-2018, Linaro Limited
 */

#ifndef __SKS_HELPERS_H
#define __SKS_HELPERS_H

#include <sks_ta.h>
#include <stdint.h>
#include <stddef.h>
#include <tee_internal_api.h>

/* Short aliases for return code */
#define SKS_OK				SKS_CKR_OK
#define SKS_ERROR			SKS_CKR_GENERAL_ERROR
#define SKS_MEMORY			SKS_CKR_DEVICE_MEMORY
#define SKS_BAD_PARAM			SKS_CKR_ARGUMENTS_BAD
#define SKS_SHORT_BUFFER		SKS_CKR_BUFFER_TOO_SMALL
#define SKS_FAILED			SKS_CKR_FUNCTION_FAILED

struct sks_object;

/*
 * Helper functions to analyse CK fields
 */
bool valid_sks_attribute_id(uint32_t id, uint32_t size);

size_t sks_attr_is_class(uint32_t attribute_id);
size_t sks_attr_is_type(uint32_t attribute_id);
bool sks_class_has_boolprop(uint32_t class);
bool sks_class_has_type(uint32_t class);
bool sks_attr_class_is_key(uint32_t class);
int sks_attr2boolprop_shift(uint32_t attr);

bool key_type_is_symm_key(uint32_t id);
bool key_type_is_asymm_key(uint32_t id);
bool certificate_is_valid(uint32_t id);
bool mechanism_is_valid(uint32_t id);
size_t get_supported_mechanisms(uint32_t *array, size_t array_count);
bool mechanism_is_supported(uint32_t id);

void sks2tee_mode(uint32_t *tee_id, uint32_t function);
bool sks2tee_load_attr(TEE_Attribute *tee_ref, uint32_t tee_id,
			struct sks_object *obj, uint32_t sks_id);

/*
 * Convert SKS return code into a GPD TEE result ID when matching.
 * If not, return a TEE success (_noerr) or generic error (_error).
 */
TEE_Result sks2tee_noerr(uint32_t rv);
TEE_Result sks2tee_error(uint32_t rv);
uint32_t tee2sks_error(TEE_Result res);

/* Id-to-string conversions when CFG_TEE_TA_LOG_LEVEL > 0 */
const char *sks2str_attr(uint32_t id);
const char *sks2str_class(uint32_t id);
const char *sks2str_type(uint32_t id, uint32_t class);
const char *sks2str_key_type(uint32_t id);
const char *sks2str_certificate_type(uint32_t id);
const char *sks2str_boolprop(uint32_t id);
const char *sks2str_proc(uint32_t id);
const char *sks2str_proc_flag(uint32_t id);
const char *sks2str_slot_flag(uint32_t id);
const char *sks2str_token_flag(uint32_t id);
const char *sks2str_rc(uint32_t id);
const char *sks2str_skscmd(uint32_t id);
const char *sks2str_attr_value(uint32_t id, size_t size, void *value);
const char *sks2str_function(uint32_t id);

#endif /*__CK_HELPERS_H*/

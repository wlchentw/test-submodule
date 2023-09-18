// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2018, Linaro Limited
 */

#include <assert.h>
#include <pkcs11.h>
#include <stdlib.h>

#include "invoke_ta.h"
#include "local_utils.h"
#include "pkcs11_processing.h"
#include "pkcs11_token.h"
#include "ck_helpers.h"

static int lib_inited;

#define REGISTER_CK_FUNCTION(_function)		._function = _function
#define DO_NOT_REGISTER_CK_FUNCTION(_function)	._function = NULL

static const CK_FUNCTION_LIST libsks_function_list = {
	.version = {
		.major = 2,
		.minor = 40,
	},
	REGISTER_CK_FUNCTION(C_Initialize),
	REGISTER_CK_FUNCTION(C_Finalize),
	REGISTER_CK_FUNCTION(C_GetInfo),
	REGISTER_CK_FUNCTION(C_GetFunctionList),
	REGISTER_CK_FUNCTION(C_GetSlotList),
	REGISTER_CK_FUNCTION(C_GetSlotInfo),
	REGISTER_CK_FUNCTION(C_GetTokenInfo),
	REGISTER_CK_FUNCTION(C_GetMechanismList),
	REGISTER_CK_FUNCTION(C_GetMechanismInfo),
	REGISTER_CK_FUNCTION(C_InitToken),
	REGISTER_CK_FUNCTION(C_InitPIN),
	REGISTER_CK_FUNCTION(C_SetPIN),
	REGISTER_CK_FUNCTION(C_OpenSession),
	REGISTER_CK_FUNCTION(C_CloseSession),
	REGISTER_CK_FUNCTION(C_CloseAllSessions),
	REGISTER_CK_FUNCTION(C_GetSessionInfo),
	REGISTER_CK_FUNCTION(C_GetOperationState),
	REGISTER_CK_FUNCTION(C_SetOperationState),
	REGISTER_CK_FUNCTION(C_Login),
	REGISTER_CK_FUNCTION(C_Logout),
	REGISTER_CK_FUNCTION(C_CreateObject),
	REGISTER_CK_FUNCTION(C_CopyObject),
	REGISTER_CK_FUNCTION(C_DestroyObject),
	REGISTER_CK_FUNCTION(C_GetObjectSize),
	REGISTER_CK_FUNCTION(C_GetAttributeValue),
	REGISTER_CK_FUNCTION(C_SetAttributeValue),
	REGISTER_CK_FUNCTION(C_FindObjectsInit),
	REGISTER_CK_FUNCTION(C_FindObjects),
	REGISTER_CK_FUNCTION(C_FindObjectsFinal),
	REGISTER_CK_FUNCTION(C_EncryptInit),
	REGISTER_CK_FUNCTION(C_Encrypt),
	REGISTER_CK_FUNCTION(C_EncryptUpdate),
	REGISTER_CK_FUNCTION(C_EncryptFinal),
	REGISTER_CK_FUNCTION(C_DecryptInit),
	REGISTER_CK_FUNCTION(C_Decrypt),
	REGISTER_CK_FUNCTION(C_DecryptUpdate),
	REGISTER_CK_FUNCTION(C_DecryptFinal),
	REGISTER_CK_FUNCTION(C_DigestInit),
	REGISTER_CK_FUNCTION(C_Digest),
	REGISTER_CK_FUNCTION(C_DigestUpdate),
	REGISTER_CK_FUNCTION(C_DigestKey),
	REGISTER_CK_FUNCTION(C_DigestFinal),
	REGISTER_CK_FUNCTION(C_SignInit),
	REGISTER_CK_FUNCTION(C_Sign),
	REGISTER_CK_FUNCTION(C_SignUpdate),
	REGISTER_CK_FUNCTION(C_SignFinal),
	REGISTER_CK_FUNCTION(C_SignRecoverInit),
	REGISTER_CK_FUNCTION(C_SignRecover),
	REGISTER_CK_FUNCTION(C_VerifyInit),
	REGISTER_CK_FUNCTION(C_Verify),
	REGISTER_CK_FUNCTION(C_VerifyUpdate),
	REGISTER_CK_FUNCTION(C_VerifyFinal),
	REGISTER_CK_FUNCTION(C_VerifyRecoverInit),
	REGISTER_CK_FUNCTION(C_VerifyRecover),
	REGISTER_CK_FUNCTION(C_DigestEncryptUpdate),
	REGISTER_CK_FUNCTION(C_DecryptDigestUpdate),
	REGISTER_CK_FUNCTION(C_SignEncryptUpdate),
	REGISTER_CK_FUNCTION(C_DecryptVerifyUpdate),
	REGISTER_CK_FUNCTION(C_GenerateKey),
	REGISTER_CK_FUNCTION(C_GenerateKeyPair),
	REGISTER_CK_FUNCTION(C_WrapKey),
	REGISTER_CK_FUNCTION(C_UnwrapKey),
	REGISTER_CK_FUNCTION(C_DeriveKey),
	REGISTER_CK_FUNCTION(C_SeedRandom),
	REGISTER_CK_FUNCTION(C_GenerateRandom),
	REGISTER_CK_FUNCTION(C_GetFunctionStatus),
	REGISTER_CK_FUNCTION(C_CancelFunction),
	REGISTER_CK_FUNCTION(C_WaitForSlotEvent),
};

/*
 * List of all PKCS#11 cryptoki API functions implemented
 */

CK_RV C_Initialize(CK_VOID_PTR init_args)
{
	(void)init_args;
	CK_C_INITIALIZE_ARGS_PTR args = NULL;

	if (init_args) {
		args = (CK_C_INITIALIZE_ARGS_PTR)init_args;
		/* Reserved must be set to NULL in this version of PKCS#11 */
		if (args->reserved)
			return CKR_ARGUMENTS_BAD;
	}

	if (lib_inited)
		return CKR_CRYPTOKI_ALREADY_INITIALIZED;

	lib_inited = 1;
	return CKR_OK;
}

CK_RV C_Finalize(CK_VOID_PTR res)
{
	(void)res;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	/* Reserved must be set to NULL in this version of PKCS#11 */
	if (res)
		return CKR_ARGUMENTS_BAD;

	sks_invoke_terminate();

	lib_inited = 0;

	return CKR_OK;
}

CK_RV C_GetInfo(CK_INFO_PTR info)
{
	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (!info)
		return CKR_ARGUMENTS_BAD;

	return sks_ck_get_info(info);
}

CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR_PTR ppFunctionList)
{
	if (!ppFunctionList)
		return CKR_ARGUMENTS_BAD;

	/* Discard the const attribute when exporting the list address */
	*ppFunctionList = (void *)&libsks_function_list;

	return CKR_OK;
}

CK_RV C_GetSlotList(CK_BBOOL token_present,
		    CK_SLOT_ID_PTR slots,
		    CK_ULONG_PTR count)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_slot_get_list(token_present, slots, count);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetSlotInfo(CK_SLOT_ID slot,
		    CK_SLOT_INFO_PTR info)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_slot_get_info(slot, info);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SLOT_ID_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_InitToken(CK_SLOT_ID slot,
		  CK_UTF8CHAR_PTR pin,
		  CK_ULONG pin_len,
		  CK_UTF8CHAR_PTR label)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_init_token(slot, pin, pin_len, label);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_PIN_INCORRECT:
	case CKR_PIN_LOCKED:
	case CKR_SESSION_EXISTS:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
	case CKR_TOKEN_NOT_RECOGNIZED:
	case CKR_TOKEN_WRITE_PROTECTED:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetTokenInfo(CK_SLOT_ID slot,
		     CK_TOKEN_INFO_PTR info)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_token_get_info(slot, info);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
	case CKR_TOKEN_NOT_RECOGNIZED:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetMechanismList(CK_SLOT_ID slot,
			 CK_MECHANISM_TYPE_PTR mechanisms,
			 CK_ULONG_PTR count)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_token_mechanism_ids(slot, mechanisms, count);

	switch (rv) {
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
	case CKR_TOKEN_NOT_RECOGNIZED:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetMechanismInfo(CK_SLOT_ID slot,
			 CK_MECHANISM_TYPE type,
			 CK_MECHANISM_INFO_PTR info)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_token_mechanism_info(slot, type, info);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_MECHANISM_INVALID:
	case CKR_OK:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
	case CKR_TOKEN_NOT_RECOGNIZED:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_OpenSession(CK_SLOT_ID slot,
		    CK_FLAGS flags,
		    CK_VOID_PTR cookie,
		    CK_NOTIFY callback,
		    CK_SESSION_HANDLE_PTR session)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (flags & ~(CKF_RW_SESSION | CKF_SERIAL_SESSION))
		return CKR_ARGUMENTS_BAD;

	/* Specific mandated flag */
	if (!(flags & CKF_SERIAL_SESSION))
		return CKR_SESSION_PARALLEL_NOT_SUPPORTED;

	rv = sks_ck_open_session(slot, flags, cookie, callback, session);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SESSION_COUNT:
	case CKR_SESSION_PARALLEL_NOT_SUPPORTED:
	case CKR_SESSION_READ_WRITE_SO_EXISTS:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
	case CKR_TOKEN_NOT_RECOGNIZED:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_CloseSession(CK_SESSION_HANDLE session)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_close_session(session);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_CloseAllSessions(CK_SLOT_ID slot)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_close_all_sessions(slot);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SLOT_ID_INVALID:
	case CKR_TOKEN_NOT_PRESENT:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetSessionInfo(CK_SESSION_HANDLE session,
		       CK_SESSION_INFO_PTR info)
{
	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return sks_ck_get_session_info(session, info);
}

CK_RV C_InitPIN(CK_SESSION_HANDLE session,
		CK_UTF8CHAR_PTR pin,
		CK_ULONG pin_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_init_pin(session, pin, pin_len);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_PIN_INVALID:
	case CKR_PIN_LEN_RANGE:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_READ_ONLY:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_USER_NOT_LOGGED_IN:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SetPIN(CK_SESSION_HANDLE session,
	       CK_UTF8CHAR_PTR old,
	       CK_ULONG old_len,
	       CK_UTF8CHAR_PTR new,
	       CK_ULONG new_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_set_pin(session, old, old_len, new, new_len);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_MECHANISM_INVALID:
	case CKR_OK:
	case CKR_PIN_INCORRECT:
	case CKR_PIN_INVALID:
	case CKR_PIN_LEN_RANGE:
	case CKR_PIN_LOCKED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_ARGUMENTS_BAD:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Login(CK_SESSION_HANDLE session,
	      CK_USER_TYPE user_type,
	      CK_UTF8CHAR_PTR pin,
	      CK_ULONG pin_len)

{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_login(session, user_type, pin, pin_len);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_PIN_INCORRECT:
	case CKR_PIN_LOCKED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY_EXISTS:
	case CKR_USER_ALREADY_LOGGED_IN:
	case CKR_USER_ANOTHER_ALREADY_LOGGED_IN:
	case CKR_USER_PIN_NOT_INITIALIZED:
	case CKR_USER_TOO_MANY_TYPES:
	case CKR_USER_TYPE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Logout(CK_SESSION_HANDLE session)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = sks_ck_logout(session);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetOperationState(CK_SESSION_HANDLE session,
			  CK_BYTE_PTR state,
			  CK_ULONG_PTR state_len)
{
	(void)session;
	(void)state;
	(void)state_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_SetOperationState(CK_SESSION_HANDLE session,
			  CK_BYTE_PTR state,
			  CK_ULONG state_len,
			  CK_OBJECT_HANDLE ciph_key,
			  CK_OBJECT_HANDLE auth_key)
{
	(void)session;
	(void)state;
	(void)state_len;
	(void)ciph_key;
	(void)auth_key;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_CreateObject(CK_SESSION_HANDLE session,
		     CK_ATTRIBUTE_PTR attribs,
		     CK_ULONG count,
		     CK_OBJECT_HANDLE_PTR phObject)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_create_object(session, attribs, count, phObject);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_READ_ONLY:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_CURVE_NOT_SUPPORTED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_DOMAIN_PARAMS_INVALID:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TEMPLATE_INCOMPLETE:
	case CKR_TEMPLATE_INCONSISTENT:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_CopyObject(CK_SESSION_HANDLE session,
		   CK_OBJECT_HANDLE obj,
		   CK_ATTRIBUTE_PTR attribs,
		   CK_ULONG count,
		   CK_OBJECT_HANDLE_PTR new_obj)
{
	(void)session;
	(void)obj;
	(void)attribs;
	(void)count;
	(void)new_obj;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DestroyObject(CK_SESSION_HANDLE session,
		      CK_OBJECT_HANDLE obj)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_destroy_object(session, obj);

	switch (rv) {
	case CKR_ACTION_PROHIBITED:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OBJECT_HANDLE_INVALID:
	case CKR_OK:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TOKEN_WRITE_PROTECTED:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetObjectSize(CK_SESSION_HANDLE session,
		      CK_OBJECT_HANDLE obj,
		      CK_ULONG_PTR out_size)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_get_object_size(session, obj, out_size);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_INFORMATION_SENSITIVE:
	case CKR_OBJECT_HANDLE_INVALID:
	case CKR_OK:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_GetAttributeValue(CK_SESSION_HANDLE session,
			  CK_OBJECT_HANDLE obj,
			  CK_ATTRIBUTE_PTR attribs,
			  CK_ULONG count)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_get_attribute_value(session, obj, attribs, count);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_SENSITIVE:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_OBJECT_HANDLE_INVALID:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SetAttributeValue(CK_SESSION_HANDLE session,
			  CK_OBJECT_HANDLE obj,
			  CK_ATTRIBUTE_PTR attribs,
			  CK_ULONG count)
{
	(void)session;
	(void)obj;
	(void)attribs;
	(void)count;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_FindObjectsInit(CK_SESSION_HANDLE session,
			CK_ATTRIBUTE_PTR attribs,
			CK_ULONG count)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_find_objects_init(session, attribs, count);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_FindObjects(CK_SESSION_HANDLE session,
		    CK_OBJECT_HANDLE_PTR obj,
		    CK_ULONG max_count,
		    CK_ULONG_PTR count)

{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_find_objects(session, obj, max_count, count);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE session)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_find_objects_final(session);

	switch (rv) {
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_EncryptInit(CK_SESSION_HANDLE session,
		    CK_MECHANISM_PTR mechanism,
		    CK_OBJECT_HANDLE key)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_init(session, mechanism, key, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
	case CKR_KEY_HANDLE_INVALID:
	case CKR_KEY_SIZE_RANGE:
	case CKR_KEY_TYPE_INCONSISTENT:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Encrypt(CK_SESSION_HANDLE session,
		CK_BYTE_PTR in,
		CK_ULONG in_len,
		CK_BYTE_PTR out,
		CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_oneshot(session, in, in_len, out, out_len, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_EncryptUpdate(CK_SESSION_HANDLE session,
		      CK_BYTE_PTR in,
		      CK_ULONG in_len,
		      CK_BYTE_PTR out,
		      CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_update(session, in, in_len, out, out_len, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_EncryptFinal(CK_SESSION_HANDLE session,
		     CK_BYTE_PTR out,
		     CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_final(session, out, out_len, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_DecryptInit(CK_SESSION_HANDLE session,
		    CK_MECHANISM_PTR  mechanism,
		    CK_OBJECT_HANDLE  key)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_init(session, mechanism, key, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
	case CKR_KEY_HANDLE_INVALID:
	case CKR_KEY_SIZE_RANGE:
	case CKR_KEY_TYPE_INCONSISTENT:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Decrypt(CK_SESSION_HANDLE session,
		CK_BYTE_PTR in,
		CK_ULONG in_len,
		CK_BYTE_PTR out,
		CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_oneshot(session, in, in_len, out, out_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_ENCRYPTED_DATA_INVALID:
	case CKR_ENCRYPTED_DATA_LEN_RANGE:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_DecryptUpdate(CK_SESSION_HANDLE session,
		      CK_BYTE_PTR in,
		      CK_ULONG in_len,
		      CK_BYTE_PTR out,
		      CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_update(session, in, in_len, out, out_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_ENCRYPTED_DATA_INVALID:
	case CKR_ENCRYPTED_DATA_LEN_RANGE:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_DecryptFinal(CK_SESSION_HANDLE session,
		     CK_BYTE_PTR out,
		     CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_encdecrypt_final(session, out, out_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_ENCRYPTED_DATA_INVALID:
	case CKR_ENCRYPTED_DATA_LEN_RANGE:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}


CK_RV C_DigestInit(CK_SESSION_HANDLE session,
		   CK_MECHANISM_PTR  mechanism)
{
	(void)session;
	(void)mechanism;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_Digest(CK_SESSION_HANDLE session,
	       CK_BYTE_PTR in,
	       CK_ULONG in_len,
	       CK_BYTE_PTR out,
	       CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DigestUpdate(CK_SESSION_HANDLE session,
		     CK_BYTE_PTR in,
		     CK_ULONG in_len)
{
	(void)session;
	(void)in;
	(void)in_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DigestKey(CK_SESSION_HANDLE session,
		  CK_OBJECT_HANDLE  key)
{
	(void)session;
	(void)key;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DigestFinal(CK_SESSION_HANDLE session,
		    CK_BYTE_PTR digest,
		    CK_ULONG_PTR len)
{
	(void)session;
	(void)digest;
	(void)len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_SignInit(CK_SESSION_HANDLE session,
		 CK_MECHANISM_PTR mechanism,
		 CK_OBJECT_HANDLE key)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_init(session, mechanism, key, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
	case CKR_KEY_HANDLE_INVALID:
	case CKR_KEY_SIZE_RANGE:
	case CKR_KEY_TYPE_INCONSISTENT:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Sign(CK_SESSION_HANDLE session,
	     CK_BYTE_PTR       in,
	     CK_ULONG          in_len,
	     CK_BYTE_PTR       out,
	     CK_ULONG_PTR      out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_oneshot(session, in, in_len, out, out_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_INVALID:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
	case CKR_FUNCTION_REJECTED:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SignUpdate(CK_SESSION_HANDLE session,
		   CK_BYTE_PTR in,
		   CK_ULONG in_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_update(session, in, in_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SignFinal(CK_SESSION_HANDLE session,
		  CK_BYTE_PTR out,
		  CK_ULONG_PTR out_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_final(session, out, out_len, CK_TRUE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_BUFFER_TOO_SMALL:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
	case CKR_FUNCTION_REJECTED:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SignRecoverInit(CK_SESSION_HANDLE session,
			CK_MECHANISM_PTR  mechanism,
			CK_OBJECT_HANDLE  key)
{
	(void)session;
	(void)mechanism;
	(void)key;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_SignRecover(CK_SESSION_HANDLE session,
		    CK_BYTE_PTR in,
		    CK_ULONG in_len,
		    CK_BYTE_PTR out,
		    CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_VerifyInit(CK_SESSION_HANDLE session,
		   CK_MECHANISM_PTR  mechanism,
		   CK_OBJECT_HANDLE  key)
{
	CK_RV rv;

	rv = ck_signverify_init(session, mechanism, key, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_KEY_FUNCTION_NOT_PERMITTED:
	case CKR_KEY_HANDLE_INVALID:
	case CKR_KEY_SIZE_RANGE:
	case CKR_KEY_TYPE_INCONSISTENT:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_Verify(CK_SESSION_HANDLE session,
	       CK_BYTE_PTR in,
	       CK_ULONG in_len,
	       CK_BYTE_PTR out,
	       CK_ULONG out_len)
{
	CK_RV rv;
	CK_ULONG out_size = out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_oneshot(session, in, in_len, out, &out_size,
				   CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_INVALID:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SIGNATURE_INVALID:
	case CKR_SIGNATURE_LEN_RANGE:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_VerifyUpdate(CK_SESSION_HANDLE session,
		     CK_BYTE_PTR in,
		     CK_ULONG in_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_update(session, in, in_len, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_VerifyFinal(CK_SESSION_HANDLE session,
		    CK_BYTE_PTR sign,
		    CK_ULONG sign_len)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_signverify_final(session, sign, &sign_len, CK_FALSE);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_DATA_LEN_RANGE:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_OK:
	case CKR_OPERATION_NOT_INITIALIZED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SIGNATURE_INVALID:
	case CKR_SIGNATURE_LEN_RANGE:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_VerifyRecoverInit(CK_SESSION_HANDLE session,
			  CK_MECHANISM_PTR mechanism,
			  CK_OBJECT_HANDLE key)
{
	(void)session;
	(void)mechanism;
	(void)key;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_VerifyRecover(CK_SESSION_HANDLE session,
		      CK_BYTE_PTR in,
		      CK_ULONG in_len,
		      CK_BYTE_PTR out,
		      CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DigestEncryptUpdate(CK_SESSION_HANDLE session,
			    CK_BYTE_PTR in,
			    CK_ULONG in_len,
			    CK_BYTE_PTR out,
			    CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DecryptDigestUpdate(CK_SESSION_HANDLE session,
			    CK_BYTE_PTR in,
			    CK_ULONG in_len,
			    CK_BYTE_PTR out,
			    CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}


CK_RV C_SignEncryptUpdate(CK_SESSION_HANDLE session,
			  CK_BYTE_PTR in,
			  CK_ULONG in_len,
			  CK_BYTE_PTR out,
			  CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DecryptVerifyUpdate(CK_SESSION_HANDLE session,
			    CK_BYTE_PTR in,
			    CK_ULONG in_len,
			    CK_BYTE_PTR out,
			    CK_ULONG_PTR out_len)
{
	(void)session;
	(void)in;
	(void)in_len;
	(void)out;
	(void)out_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_GenerateKey(CK_SESSION_HANDLE session,
		    CK_MECHANISM_PTR mechanism,
		    CK_ATTRIBUTE_PTR attribs,
		    CK_ULONG count,
		    CK_OBJECT_HANDLE_PTR handle)
{
	CK_RV rv;
	CK_ATTRIBUTE_PTR attribs_n = NULL;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_add_object_class(attribs, &count,
			&attribs_n, CKO_SECRET_KEY);
	if (rv != CKR_OK) {
		if (attribs_n)
			free(attribs_n);
		return rv;
	}

	rv = ck_generate_key(session, mechanism, attribs, count, handle);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_READ_ONLY:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_CURVE_NOT_SUPPORTED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TEMPLATE_INCOMPLETE:
	case CKR_TEMPLATE_INCONSISTENT:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	if (attribs_n)
		free(attribs_n);
	return rv;
}

CK_RV C_GenerateKeyPair(CK_SESSION_HANDLE session,
			CK_MECHANISM_PTR mechanism,
			CK_ATTRIBUTE_PTR pub_attribs,
			CK_ULONG pub_count,
			CK_ATTRIBUTE_PTR priv_attribs,
			CK_ULONG priv_count,
			CK_OBJECT_HANDLE_PTR pub_key,
			CK_OBJECT_HANDLE_PTR priv_key)
{
	CK_RV rv;
	CK_ATTRIBUTE_PTR pub_attribs_n = NULL;
	CK_ATTRIBUTE_PTR priv_attribs_n = NULL;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	if (!mechanism || !pub_key || !priv_key)
		return CKR_ARGUMENTS_BAD;

	if (pub_count && !pub_attribs)
		return CKR_ARGUMENTS_BAD;

	if (priv_count && !priv_attribs)
		return CKR_ARGUMENTS_BAD;

	rv = ck_guess_key_type(mechanism, pub_attribs, &pub_count,
			       &pub_attribs_n);
	if (rv != CKR_OK)
		goto bail;

	rv = ck_add_object_class(pub_attribs_n, &pub_count,
				   &pub_attribs_n, CKO_PUBLIC_KEY);
	if (rv != CKR_OK)
		goto bail;

	rv = ck_guess_key_type(mechanism, priv_attribs, &priv_count,
				   &priv_attribs_n);
	if (rv != CKR_OK)
		goto bail;

	rv = ck_add_object_class(priv_attribs_n, &priv_count,
				   &priv_attribs_n, CKO_PRIVATE_KEY);
	if (rv != CKR_OK)
		goto bail;

	rv = ck_generate_key_pair(session, mechanism, pub_attribs_n, pub_count,
				  priv_attribs_n, priv_count, pub_key, priv_key);

bail:
	if (pub_attribs_n)
		free(pub_attribs_n);
	if (priv_attribs_n)
		free(priv_attribs_n);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_READ_ONLY:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_CURVE_NOT_SUPPORTED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_DOMAIN_PARAMS_INVALID:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TEMPLATE_INCOMPLETE:
	case CKR_TEMPLATE_INCONSISTENT:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_WrapKey(CK_SESSION_HANDLE session,
		CK_MECHANISM_PTR  mechanism,
		CK_OBJECT_HANDLE wrap_key,
		CK_OBJECT_HANDLE key,
		CK_BYTE_PTR wrapped_key,
		CK_ULONG_PTR wrapped_key_len)
{
	(void)session;
	(void)mechanism;
	(void)wrap_key;
	(void)key;
	(void)wrapped_key;
	(void)wrapped_key_len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_UnwrapKey(CK_SESSION_HANDLE session,
		  CK_MECHANISM_PTR mechanism,
		  CK_OBJECT_HANDLE unwrap_key,
		  CK_BYTE_PTR wrapped_key,
		  CK_ULONG wrapped_key_len,
		  CK_ATTRIBUTE_PTR attribs,
		  CK_ULONG count,
		  CK_OBJECT_HANDLE_PTR new_key)
{
	(void)session;
	(void)mechanism;
	(void)unwrap_key;
	(void)wrapped_key;
	(void)wrapped_key_len;
	(void)attribs;
	(void)count;
	(void)new_key;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_DeriveKey(CK_SESSION_HANDLE session,
		  CK_MECHANISM_PTR mechanism,
		  CK_OBJECT_HANDLE derived_key,
		  CK_ATTRIBUTE_PTR attribs,
		  CK_ULONG count,
		  CK_OBJECT_HANDLE_PTR key_handle)
{
	CK_RV rv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	rv = ck_derive_key(session, mechanism, derived_key, attribs, count,
			   key_handle);

	switch (rv) {
	case CKR_ARGUMENTS_BAD:
	case CKR_ATTRIBUTE_READ_ONLY:
	case CKR_ATTRIBUTE_TYPE_INVALID:
	case CKR_ATTRIBUTE_VALUE_INVALID:
	case CKR_CRYPTOKI_NOT_INITIALIZED:
	case CKR_CURVE_NOT_SUPPORTED:
	case CKR_DEVICE_ERROR:
	case CKR_DEVICE_MEMORY:
	case CKR_DEVICE_REMOVED:
	case CKR_DOMAIN_PARAMS_INVALID:
	case CKR_FUNCTION_CANCELED:
	case CKR_FUNCTION_FAILED:
	case CKR_GENERAL_ERROR:
	case CKR_HOST_MEMORY:
	case CKR_KEY_HANDLE_INVALID:
	case CKR_KEY_SIZE_RANGE:
	case CKR_KEY_TYPE_INCONSISTENT:
	case CKR_MECHANISM_INVALID:
	case CKR_MECHANISM_PARAM_INVALID:
	case CKR_OK:
	case CKR_OPERATION_ACTIVE:
	case CKR_PIN_EXPIRED:
	case CKR_SESSION_CLOSED:
	case CKR_SESSION_HANDLE_INVALID:
	case CKR_SESSION_READ_ONLY:
	case CKR_TEMPLATE_INCOMPLETE:
	case CKR_TEMPLATE_INCONSISTENT:
	case CKR_TOKEN_WRITE_PROTECTED:
	case CKR_USER_NOT_LOGGED_IN:
		break;
	default:
		assert(!rv);
	}

	return rv;
}

CK_RV C_SeedRandom(CK_SESSION_HANDLE session,
		   CK_BYTE_PTR seed,
		   CK_ULONG len)
{
	(void)session;
	(void)seed;
	(void)len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_GenerateRandom(CK_SESSION_HANDLE session,
		       CK_BYTE_PTR out,
		       CK_ULONG len)
{
	(void)session;
	(void)out;
	(void)len;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

CK_RV C_GetFunctionStatus(CK_SESSION_HANDLE session)
{
	(void)session;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_PARALLEL;
}

CK_RV C_CancelFunction(CK_SESSION_HANDLE session)
{
	(void)session;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_PARALLEL;
}


CK_RV C_WaitForSlotEvent(CK_FLAGS flags,
			 CK_SLOT_ID_PTR slot,
			 CK_VOID_PTR rsv)
{
	(void)flags;
	(void)slot;
	(void)rsv;

	if (!lib_inited)
		return CKR_CRYPTOKI_NOT_INITIALIZED;

	return CKR_FUNCTION_NOT_SUPPORTED;
}

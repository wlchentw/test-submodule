/*
 * Copyright (c) 2017, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <pkcs11.h>
#include <sks_ck_debug.h>
#include <sks_ta.h>
#include <stdlib.h>
#include <string.h>

#include "ck_helpers.h"
#include "invoke_ta.h"
#include "local_utils.h"
#include "pkcs11_token.h"

#define SKS_CRYPTOKI_SLOT_MANUFACTURER		"Linaro"

#define PADDED_STRING_COPY(_dst, _src) \
	do { \
		memset((char *)_dst, ' ', sizeof(_dst)); \
		strncpy((char *)_dst, _src, strlen(_src)); \
		memset((char *)_dst, '\0', sizeof(_src));   \
	} while (0)

/**
 * sks_ck_get_info - implementation of C_GetInfo
 */
int sks_ck_get_info(CK_INFO_PTR info)
{
	const CK_VERSION ck_version = { 2, 40 };
	const char manuf_id[] = SKS_CRYPTOKI_SLOT_MANUFACTURER; // TODO slot?
	const CK_FLAGS flags = 0;	/* must be zero per the PKCS#11 2.40 */
	const char lib_description[] = "OP-TEE SKS Cryptoki library";
	const CK_VERSION lib_version = { 0, 0 };

	info->cryptokiVersion = ck_version;
	PADDED_STRING_COPY(info->manufacturerID, manuf_id);
	info->flags = flags;
	PADDED_STRING_COPY(info->libraryDescription, lib_description);
	info->libraryVersion = lib_version;

	return CKR_OK;
}

/**
 * slot_get_info - implementation of C_GetSlotList
 */
CK_RV sks_ck_slot_get_list(CK_BBOOL present,
			   CK_SLOT_ID_PTR slots, CK_ULONG_PTR count)
{
	TEEC_SharedMemory *shm;
	size_t size = 0;
	CK_RV rv = CKR_GENERAL_ERROR;
	unsigned int n;

	/* Discard present: all are present */
	(void)present;

	if (!count)
		return CKR_ARGUMENTS_BAD;

	if (ck_invoke_ta_in_out(NULL, SKS_CMD_CK_SLOT_LIST, NULL, 0,
				NULL, 0, NULL, &size) != CKR_BUFFER_TOO_SMALL)
		return CKR_DEVICE_ERROR;

	if (!slots || *count < (size / sizeof(uint32_t))) {
		*count = size / sizeof(uint32_t);
		if (!slots)
			return CKR_OK;

		return CKR_BUFFER_TOO_SMALL;
	}

	shm = sks_alloc_shm_out(NULL, size);
	if (!shm)
		return CKR_HOST_MEMORY;

	if (ck_invoke_ta_in_out(NULL, SKS_CMD_CK_SLOT_LIST, NULL, 0,
				NULL, 0, shm, NULL) != CKR_OK) {
		rv = CKR_DEVICE_ERROR;
		goto bail;
	}

	for (n = 0; n < (size / sizeof(uint32_t)); n++)
		slots[n] = *((uint32_t *)shm->buffer + n);

	*count = size / sizeof(uint32_t);
	rv = CKR_OK;
bail:
	sks_free_shm(shm);
	return rv;

}

/**
 * slot_get_info - implementation of C_GetSlotInfo
 */
int sks_ck_slot_get_info(CK_SLOT_ID slot, CK_SLOT_INFO_PTR info)
{
	uint32_t ctrl[1] = { slot };
	CK_SLOT_INFO *ck_info = info;
	struct sks_slot_info sks_info;
	size_t out_size = sizeof(sks_info);
	CK_RV rv = CKR_GENERAL_ERROR;

	if (!info)
		return CKR_ARGUMENTS_BAD;

	rv = ck_invoke_ta_in_out(NULL, SKS_CMD_CK_SLOT_INFO, &ctrl,
			sizeof(ctrl), NULL, 0, &sks_info, &out_size);
	if (rv)
		return rv;

	if (sks2ck_slot_info(ck_info, &sks_info)) {
		LOG_ERROR("unexpected bad token info structure\n");
		return CKR_DEVICE_ERROR;
	}

	return CKR_OK;
}

/**
 * slot_get_info - implementation of C_GetTokenInfo
 */
CK_RV sks_ck_token_get_info(CK_SLOT_ID slot, CK_TOKEN_INFO_PTR info)
{
	uint32_t ctrl[1] = { slot };
	CK_TOKEN_INFO *ck_info = info;
	TEEC_SharedMemory *shm;
	size_t size;
	CK_RV rv = CKR_GENERAL_ERROR;

	if (!info)
		return CKR_ARGUMENTS_BAD;

	ctrl[0] = (uint32_t)slot;
	size = 0;
	if (ck_invoke_ta_in_out(NULL, SKS_CMD_CK_TOKEN_INFO, ctrl, sizeof(ctrl),
				NULL, 0, NULL, &size) != CKR_BUFFER_TOO_SMALL)
		return CKR_DEVICE_ERROR;

	shm = sks_alloc_shm_out(NULL, size);
	if (!shm)
		return CKR_HOST_MEMORY;

	ctrl[0] = (uint32_t)slot;
	rv = ck_invoke_ta_in_out(NULL, SKS_CMD_CK_TOKEN_INFO,
				 ctrl, sizeof(ctrl), NULL, 0, shm, NULL);
	if (rv)
		goto bail;

	if (shm->size < sizeof(struct sks_token_info)) {
		LOG_ERROR("unexpected bad token info size\n");
		rv = CKR_DEVICE_ERROR;
		goto bail;
	}

	rv = sks2ck_token_info(ck_info, shm->buffer);

bail:
	sks_free_shm(shm);

	return rv;
}

/**
 * sks_ck_init_token - implementation of C_InitToken
 */
CK_RV sks_ck_init_token(CK_SLOT_ID slot,
			CK_UTF8CHAR_PTR pin,
			CK_ULONG pin_len,
			CK_UTF8CHAR_PTR label)
{
	uint32_t sks_slot = slot;
	uint32_t sks_pin_len = pin_len;
	size_t ctrl_size = 2 * sizeof(uint32_t) + sks_pin_len +
			   32 * sizeof(uint8_t);
	char *ctrl;
	size_t offset;

	if (!pin || !label)
		return CKR_ARGUMENTS_BAD;

	ctrl = malloc(ctrl_size);
	if (!ctrl)
		return CKR_HOST_MEMORY;

	memcpy(ctrl, &sks_slot, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	memcpy(ctrl + offset, &sks_pin_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(ctrl + offset, pin, sks_pin_len);
	offset += sks_pin_len;

	memcpy(ctrl + offset, label, 32 * sizeof(uint8_t));

	return ck_invoke_ta(NULL, SKS_CMD_CK_INIT_TOKEN, ctrl, ctrl_size);
}

/**
 * sks_ck_token_mechanism_ids - implementation of C_GetMechanismList
 */
CK_RV sks_ck_token_mechanism_ids(CK_SLOT_ID slot,
				 CK_MECHANISM_TYPE_PTR mechanisms,
				 CK_ULONG_PTR count)
{
	uint32_t ctrl[1] = { slot };
	size_t outsize = 0;
	void *outbuf = NULL;
	CK_RV rv;

	if (!count)
		return CKR_ARGUMENTS_BAD;

	if (!mechanisms)
		return CKR_ARGUMENTS_BAD;

	if (mechanisms) {
		outsize = *count * sizeof(uint32_t);
		outbuf = malloc(outsize);
		if (!outbuf)
			return CKR_HOST_MEMORY;
	}

	rv = ck_invoke_ta_in_out(NULL, SKS_CMD_CK_MECHANISM_IDS,
				 &ctrl, sizeof(ctrl),
				 NULL, 0, outbuf, &outsize);

	if (rv == CKR_OK || rv == CKR_BUFFER_TOO_SMALL) {
		*count = outsize / sizeof(uint32_t);
	}
	if (rv == CKR_BUFFER_TOO_SMALL) {
		rv = CKR_OK;
		goto bail;
	}
	if (rv) {
		goto bail;
	}

	if (sks2ck_mechanism_type_list(mechanisms, outbuf, *count)) {
		LOG_ERROR("unexpected bad mechanism_type list\n");
		rv = CKR_DEVICE_ERROR;
	}

bail:
	free(outbuf);

	return rv;
}

/**
 * sks_ck_token_mechanism_info - implementation of C_GetMechanismInfo
 */
CK_RV sks_ck_token_mechanism_info(CK_SLOT_ID slot,
				  CK_MECHANISM_TYPE type,
				  CK_MECHANISM_INFO_PTR info)
{
	CK_RV rv;
	uint32_t ctrl[2];
	struct sks_mechanism_info outbuf;
	size_t outsize = sizeof(outbuf);

	if (!info)
		return CKR_ARGUMENTS_BAD;

	ctrl[0] = (uint32_t)slot;
	ctrl[1] = ck2sks_mechanism_type(type);
	if (ctrl[1] == SKS_UNDEFINED_ID) {
		LOG_DEBUG("mechanism is not support by this library\n");
		return CKR_MECHANISM_INVALID;
	}

	/* info is large enought, for sure */
	rv = ck_invoke_ta_in_out(NULL, SKS_CMD_CK_MECHANISM_INFO,
				 &ctrl, sizeof(ctrl),
				 NULL, 0, &outbuf, &outsize);
	if (rv)
		return rv;

	if (sks2ck_mechanism_info(info, &outbuf)) {
		LOG_ERROR("unexpected bad mechanism info structure\n");
		rv = CKR_DEVICE_ERROR;
	}
	return rv;
}

/**
 * sks_ck_open_session - implementation of C_OpenSession
 */
CK_RV sks_ck_open_session(CK_SLOT_ID slot,
		          CK_FLAGS flags,
		          CK_VOID_PTR cookie,
		          CK_NOTIFY callback,
		          CK_SESSION_HANDLE_PTR session)
{
	uint32_t ctrl[1] = { slot };
	unsigned long cmd;
	uint32_t handle;
	size_t out_sz = sizeof(handle);
	CK_RV rv;

	if (!session)
		return CKR_ARGUMENTS_BAD;

	if (cookie || callback) {
		LOG_ERROR("C_OpenSession does not handle callback yet\n");
		return CKR_FUNCTION_NOT_SUPPORTED;
	}

	if (flags & CKF_RW_SESSION)
		cmd = SKS_CMD_CK_OPEN_RW_SESSION;
	else
		cmd = SKS_CMD_CK_OPEN_RO_SESSION;

	rv = ck_invoke_ta_in_out(NULL, cmd, &ctrl, sizeof(ctrl),
				 NULL, 0, &handle, &out_sz);
	if (rv)
		return rv;

	*session = handle;

	return CKR_OK;
}

CK_RV sks_ck_close_session(CK_SESSION_HANDLE session)
{
	uint32_t ctrl[1] = { (uint32_t)session };

	return ck_invoke_ta(NULL, SKS_CMD_CK_CLOSE_SESSION,
			    &ctrl, sizeof(ctrl));
}

/**
 * sks_ck_close_all_sessions - implementation of C_CloseAllSessions
 */
CK_RV sks_ck_close_all_sessions(CK_SLOT_ID slot)
{
	uint32_t ctrl[1] = { (uint32_t)slot };

	return ck_invoke_ta(NULL, SKS_CMD_CK_CLOSE_ALL_SESSIONS,
			    &ctrl, sizeof(ctrl));
}

/**
 * sks_ck_get_session_info - implementation of C_GetSessionInfo
 */
CK_RV sks_ck_get_session_info(CK_SESSION_HANDLE session,
			      CK_SESSION_INFO_PTR info)
{
	uint32_t ctrl[1] = { (uint32_t)session };
	CK_SESSION_INFO *s_info = info;
	TEEC_SharedMemory *shm = NULL;
	CK_RV rv = CKR_GENERAL_ERROR;
	size_t info_size = sizeof(struct sks_session_info);

	if (!info)
		return CKR_ARGUMENTS_BAD;

	shm = sks_alloc_shm_out(NULL, info_size);
	if (!shm)
		return CKR_HOST_MEMORY;

	rv = ck_invoke_ta_in_out(NULL, SKS_CMD_CK_SESSION_INFO,
				   &ctrl, sizeof(ctrl),
				   NULL, 0, shm, NULL);
	if (rv)
		goto bail;

	if (shm->size < info_size) {
		LOG_ERROR("unexpected bad session info size\n");
		rv = CKR_DEVICE_ERROR;
		goto bail;
	}

	rv = sks2ck_session_info(s_info, shm->buffer);

bail:
	sks_free_shm(shm);

	return rv;
}

/**
 * sks_ck_init_pin - implementation of C_InitPIN
 */
CK_RV sks_ck_init_pin(CK_SESSION_HANDLE session,
		      CK_UTF8CHAR_PTR pin, CK_ULONG pin_len)
{
	uint32_t sks_session = session;
	uint32_t sks_pin_len = pin_len;
	size_t ctrl_size = 2 * sizeof(uint32_t) + sks_pin_len;
	char *ctrl;

	ctrl = malloc(ctrl_size);
	if (!ctrl)
		return CKR_HOST_MEMORY;

	memcpy(ctrl, &sks_session, sizeof(uint32_t));
	memcpy(ctrl + sizeof(uint32_t), &sks_pin_len, sizeof(uint32_t));
	memcpy(ctrl + 2 * sizeof(uint32_t), pin, sks_pin_len);

	return ck_invoke_ta(NULL, SKS_CMD_INIT_PIN, ctrl, ctrl_size);
}

/**
 * sks_ck_set_pin - implementation of C_SetPIN
 */
CK_RV sks_ck_set_pin(CK_SESSION_HANDLE session,
		     CK_UTF8CHAR_PTR old, CK_ULONG old_len,
		     CK_UTF8CHAR_PTR new, CK_ULONG new_len)
{
	uint32_t sks_session = session;
	uint32_t sks_old_len = old_len;
	uint32_t sks_new_len = new_len;
	size_t ctrl_size = 3 * sizeof(uint32_t) + sks_old_len + sks_new_len;
	char *ctrl;
	size_t offset;

	ctrl = malloc(ctrl_size);
	if (!ctrl)
		return CKR_HOST_MEMORY;

	memcpy(ctrl, &sks_session, sizeof(uint32_t));
	offset = sizeof(uint32_t);

	memcpy(ctrl + offset, &sks_old_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(ctrl + offset, old, sks_old_len);
	offset += sks_old_len;

	memcpy(ctrl + offset, &sks_new_len, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(ctrl + offset, new, sks_new_len);

	return ck_invoke_ta(NULL, SKS_CMD_SET_PIN, ctrl, ctrl_size);
}

/**
 * sks_ck_login - implementation of C_Login
 */
CK_RV sks_ck_login(CK_SESSION_HANDLE session, CK_USER_TYPE user_type,
		   CK_UTF8CHAR_PTR pin, CK_ULONG pin_len)

{
	uint32_t sks_session = session;
	uint32_t sks_user = ck2sks_user_type(user_type);
	uint32_t sks_pin_len = pin_len;
	size_t ctrl_size = 3 * sizeof(uint32_t) + sks_pin_len;
	char *ctrl;

	ctrl = malloc(ctrl_size);
	if (!ctrl)
		return CKR_HOST_MEMORY;

	memcpy(ctrl, &sks_session, sizeof(uint32_t));
	memcpy(ctrl + sizeof(uint32_t), &sks_user, sizeof(uint32_t));
	memcpy(ctrl + 2 * sizeof(uint32_t), &sks_pin_len, sizeof(uint32_t));
	memcpy(ctrl + 3 * sizeof(uint32_t), pin, sks_pin_len);

	return ck_invoke_ta(NULL, SKS_CMD_LOGIN, ctrl, ctrl_size);
}

/**
 * sks_ck_logout - implementation of C_Logout
 */
CK_RV sks_ck_logout(CK_SESSION_HANDLE session)
{
	uint32_t sks_session = session;
	size_t ctrl_size = sizeof(uint32_t);
	char *ctrl;

	ctrl = malloc(ctrl_size);
	if (!ctrl)
		return CKR_HOST_MEMORY;

	memcpy(ctrl, &sks_session, sizeof(uint32_t));

	return ck_invoke_ta(NULL, SKS_CMD_LOGOUT, ctrl, ctrl_size);
}

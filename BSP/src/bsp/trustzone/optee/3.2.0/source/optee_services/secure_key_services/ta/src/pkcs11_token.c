// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2017-2018, Linaro Limited
 */

#include <assert.h>
#include <sks_ta.h>
#include <string.h>
#include <string_ext.h>
#include <sys/queue.h>
#include <tee_internal_api_extensions.h>
#include <util.h>

#include "attributes.h"
#include "handle.h"
#include "pkcs11_token.h"
#include "pkcs11_attributes.h"
#include "processing.h"
#include "serializer.h"
#include "sks_helpers.h"

/* ID is token index */
#define TOKEN_COUNT	CFG_SKS_TA_TOKEN_COUNT

/* Static allocation of tokens runtime instances (reset to 0 at load) */
struct ck_token ck_token[TOKEN_COUNT];

static struct client_list pkcs11_client_list;

static void close_ck_session(struct pkcs11_session *session);

/* Static allocation of tokens runtime instances */
struct ck_token *get_token(unsigned int token_id)
{
	if (token_id > TOKEN_COUNT)
		return NULL;

	return &ck_token[token_id];
}

unsigned int get_token_id(struct ck_token *token)
{
	assert(token >= ck_token && token < &ck_token[TOKEN_COUNT]);

	return token - ck_token;
}

/* Client */
struct pkcs11_client *tee_session2client(uintptr_t tee_session)
{
	struct pkcs11_client *client;

	TAILQ_FOREACH(client, &pkcs11_client_list, link) {
		if (client == (void *)tee_session)
			return client;
	}

	return NULL;
}

uintptr_t register_client(void)
{
	struct pkcs11_client *client = NULL;

	client = TEE_Malloc(sizeof(*client), TEE_MALLOC_FILL_ZERO);
	if (!client)
		return 0;

	TAILQ_INSERT_HEAD(&pkcs11_client_list, client, link);
	TAILQ_INIT(&client->session_list);
	handle_db_init(&client->session_handle_db);

	return (uintptr_t)(void *)client;
}

void unregister_client(uintptr_t tee_session)
{
	struct pkcs11_client *client = tee_session2client(tee_session);
	struct pkcs11_session *session = NULL;
	struct pkcs11_session *next = NULL;

	if (!client) {
		EMSG("Unexpected invalid TEE session handle");
		return;
	}

	TAILQ_FOREACH_SAFE(session, &client->session_list, link, next) {
		close_ck_session(session);
	}

	TAILQ_REMOVE(&pkcs11_client_list, client, link);
	handle_db_destroy(&client->session_handle_db);
	TEE_Free(client);
}

static int pkcs11_token_init(unsigned int id)
{
	struct ck_token *token = init_token_db(id);

	if (!token)
		return 1;

	if (token->state != PKCS11_TOKEN_RESET) {
		/* Token is already in a valid state */
		return 0;
	}

	/* Initialize the token runtime state */
	token->state = PKCS11_TOKEN_READ_WRITE;
	token->session_count = 0;
	token->rw_session_count = 0;

	return 0;
}

int pkcs11_init(void)
{
	unsigned int id = 0;

	for (id = 0; id < TOKEN_COUNT; id++)
		if (pkcs11_token_init(id))
			return 1;

	TAILQ_INIT(&pkcs11_client_list);

	return 0;
}

void pkcs11_deinit(void)
{
	unsigned int id = 0;

	for (id = 0; id < TOKEN_COUNT; id++)
		close_persistent_db(get_token(id));
}

bool pkcs11_session_is_read_write(struct pkcs11_session *session)
{
	switch (session->state) {
	case PKCS11_SESSION_PUBLIC_READ_WRITE:
	case PKCS11_SESSION_USER_READ_WRITE:
	case PKCS11_SESSION_SO_READ_WRITE:
		return true;
	default:
		return false;
	}
}

bool pkcs11_session_is_security_officer(struct pkcs11_session *session)
{
	return session->state == PKCS11_SESSION_SO_READ_WRITE;
}

bool pkcs11_session_is_user(struct pkcs11_session *session)
{
	return session->state == PKCS11_SESSION_USER_READ_WRITE ||
		session->state == PKCS11_SESSION_USER_READ_ONLY;
}

bool pkcs11_session_is_public(struct pkcs11_session *session)
{
	return session->state == PKCS11_SESSION_PUBLIC_READ_WRITE ||
		session->state == PKCS11_SESSION_PUBLIC_READ_ONLY;
}

struct pkcs11_session *sks_handle2session(uint32_t handle,
					  uintptr_t tee_session)
{
	struct pkcs11_client *client = tee_session2client(tee_session);

	return handle_lookup(&client->session_handle_db, handle);
}

/*
 * Currently not support dual operations.
 */
int set_processing_state(struct pkcs11_session *session,
			 enum processing_func function,
			 struct sks_object *obj1, struct sks_object *obj2)
{
	enum pkcs11_proc_state state;
	struct active_processing *proc = NULL;

	TEE_MemFill(&state, 0, sizeof(state));

	if (session->processing)
		return SKS_CKR_OPERATION_ACTIVE;

	switch (function) {
	case SKS_FUNCTION_ENCRYPT:
		state = PKCS11_SESSION_ENCRYPTING;
		break;
	case SKS_FUNCTION_DECRYPT:
		state = PKCS11_SESSION_DECRYPTING;
		break;
	case SKS_FUNCTION_SIGN:
		state = PKCS11_SESSION_SIGNING;
		break;
	case SKS_FUNCTION_VERIFY:
		state = PKCS11_SESSION_VERIFYING;
		break;
	case SKS_FUNCTION_DIGEST:
		state = PKCS11_SESSION_DIGESTING;
		break;
	case SKS_FUNCTION_DERIVE:
		state = PKCS11_SESSION_READY;
		break;
	default:
		TEE_Panic(function);
		return -1;
	}

	proc = TEE_Malloc(sizeof(*proc), TEE_MALLOC_FILL_ZERO);
	if (!proc)
		return SKS_MEMORY;

	/* Boolean are default to false and pointers to NULL */
	proc->state = state;
	proc->tee_op_handle = TEE_HANDLE_NULL;

	if (obj1 && get_bool(obj1->attributes, SKS_CKA_ALWAYS_AUTHENTICATE))
		proc->always_authen = true;

	if (obj2 && get_bool(obj2->attributes, SKS_CKA_ALWAYS_AUTHENTICATE))
		proc->always_authen = true;

	session->processing = proc;

	return SKS_OK;
}

static void cipher_pin(TEE_ObjectHandle key_handle, uint8_t *buf, size_t len)
{
	uint8_t iv[16] = { 0 };
	uint32_t size = len;
	TEE_OperationHandle tee_op_handle = TEE_HANDLE_NULL;
	TEE_Result res = TEE_ERROR_GENERIC;

	res = TEE_AllocateOperation(&tee_op_handle,
				    TEE_ALG_AES_CBC_NOPAD,
				    TEE_MODE_ENCRYPT, 128);
	if (res)
		TEE_Panic(0);

	res = TEE_SetOperationKey(tee_op_handle, key_handle);
	if (res)
		TEE_Panic(0);

	TEE_CipherInit(tee_op_handle, iv, sizeof(iv));

	res = TEE_CipherDoFinal(tee_op_handle, buf, len, buf, &size);
	if (res || size != SKS_TOKEN_PIN_SIZE)
		TEE_Panic(0);

	TEE_FreeOperation(tee_op_handle);
}

/* ctrl=[slot-id][pin-size][pin][label], in=unused, out=unused */
uint32_t entry_ck_token_initialize(TEE_Param *ctrl,
				   TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	uint32_t pin_size = 0;
	void *pin = NULL;
	char label[SKS_TOKEN_LABEL_SIZE + 1] = { 0 };
	struct ck_token *token;
	uint8_t *cpin = NULL;
	int pin_rc = 0;
	struct pkcs11_client *client;
	struct sks_object *obj = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	rv = serialargs_get(&ctrlargs, &pin_size, sizeof(uint32_t));
	if (rv)
		return rv;

	if (pin_size < 8 || pin_size > SKS_TOKEN_PIN_SIZE)
		return SKS_CKR_PIN_LEN_RANGE;

	rv = serialargs_get_ptr(&ctrlargs, &pin, pin_size);
	if (rv)
		return rv;

	rv = serialargs_get(&ctrlargs, &label, SKS_TOKEN_LABEL_SIZE);
	if (rv)
		return rv;

	if (token->db_main->flags & SKS_CKFT_SO_PIN_LOCKED) {
		IMSG("SKSt%u: SO PIN locked", token_id);
		return SKS_CKR_PIN_LOCKED;
	}

	TAILQ_FOREACH(client, &pkcs11_client_list, link) {
		if (!TAILQ_EMPTY(&client->session_list)) {
			return SKS_CKR_SESSION_EXISTS;
		}
	}

	cpin = TEE_Malloc(SKS_TOKEN_PIN_SIZE, TEE_MALLOC_FILL_ZERO);
	if (!cpin) {
		return SKS_MEMORY;
	}

	TEE_MemMove(cpin, pin, pin_size);
	cipher_pin(token->pin_hdl[0], cpin, SKS_TOKEN_PIN_SIZE);

	if (!token->db_main->so_pin_size) {
		TEE_MemMove(token->db_main->so_pin, cpin, SKS_TOKEN_PIN_SIZE);
		token->db_main->so_pin_size = pin_size;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      so_pin),
				     sizeof(token->db_main->so_pin));
		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      so_pin_size),
				     sizeof(token->db_main->so_pin_size));

		goto inited;
	}

	pin_rc = 0;
	if (token->db_main->so_pin_size != pin_size)
		pin_rc = 1;
	if (buf_compare_ct(token->db_main->so_pin, cpin, SKS_TOKEN_PIN_SIZE))
		pin_rc = 1;

	if (pin_rc) {
		token->db_main->flags |= SKS_CKFT_SO_PIN_COUNT_LOW;
		token->db_main->so_pin_count++;

		if (token->db_main->so_pin_count == 6)
			token->db_main->flags |= SKS_CKFT_SO_PIN_FINAL_TRY;
		if (token->db_main->so_pin_count == 7)
			token->db_main->flags |= SKS_CKFT_SO_PIN_LOCKED;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      flags),
				     sizeof(token->db_main->flags));

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      so_pin_count),
				     sizeof(token->db_main->so_pin_count));

		TEE_Free(cpin);
		return SKS_CKR_PIN_INCORRECT;
	}

	token->db_main->flags &= ~(SKS_CKFT_SO_PIN_COUNT_LOW |
				   SKS_CKFT_SO_PIN_FINAL_TRY);
	token->db_main->so_pin_count = 0;

inited:
	TEE_MemMove(token->db_main->label, label, SKS_TOKEN_LABEL_SIZE);
	token->db_main->flags |= SKS_CKFT_TOKEN_INITIALIZED;
	/* Reset user PIN */
	token->db_main->user_pin_size = 0;
	token->db_main->flags &= ~(SKS_CKFT_USER_PIN_INITIALIZED |
				   SKS_CKFT_USER_PIN_COUNT_LOW |
				   SKS_CKFT_USER_PIN_FINAL_TRY |
				   SKS_CKFT_USER_PIN_LOCKED |
				   SKS_CKFT_USER_PIN_TO_BE_CHANGED);

	update_persistent_db(token, 0, sizeof(*token->db_main));

	/* Remove all persistent objects */
	if (token->db_objs && token->db_objs->count > 0) {
		while (!LIST_EMPTY(&token->object_list)) {
			obj = LIST_FIRST(&token->object_list);
#ifdef DEBUG
			MSG_RAW("[destroy] obj uuid %pUl", (void *)obj->uuid);
#endif
			unregister_persistent_object(token, obj->uuid);
			cleanup_persistent_object(obj, token);
		}
	}

	label[SKS_TOKEN_LABEL_SIZE] = '\0';
	IMSG("SKSt%" PRIu32 ": initialized \"%s\"", token_id, label);

	TEE_Free(cpin);

	return SKS_OK;
}

uint32_t entry_ck_slot_list(TEE_Param *ctrl, TEE_Param *in, TEE_Param *out)
{
	const size_t out_size = sizeof(uint32_t) * TOKEN_COUNT;
	uint32_t *id = NULL;
	unsigned int n = 0;

	if (ctrl || in || !out)
		return SKS_BAD_PARAM;

	if (out->memref.size < out_size) {
		out->memref.size = out_size;
		return SKS_SHORT_BUFFER;
	}

	/* FIXME: we could support unaligment buffers */
	if ((uintptr_t)out->memref.buffer & 0x03UL)
		return SKS_BAD_PARAM;

	for (id = out->memref.buffer, n = 0; n < TOKEN_COUNT; n++, id++)
		*id = (uint32_t)n;

	out->memref.size = out_size;

	return SKS_OK;
}

uint32_t entry_ck_slot_info(TEE_Param *ctrl, TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	struct ck_token *token = NULL;
	const char desc[] = SKS_CRYPTOKI_SLOT_DESCRIPTION;
	const char manuf[] = SKS_CRYPTOKI_SLOT_MANUFACTURER;
	const char hwver[2] = SKS_CRYPTOKI_SLOT_HW_VERSION;
	const char fwver[2] = SKS_CRYPTOKI_SLOT_FW_VERSION;
	struct sks_slot_info info;
	char dev_uuid[37]; /* UUID as string */
	TEE_UUID dev_id;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));
	TEE_MemFill(&info, 0, sizeof(info));

	if (!ctrl || in || !out)
		return SKS_BAD_PARAM;

	if (out->memref.size < sizeof(struct sks_slot_info)) {
		out->memref.size = sizeof(struct sks_slot_info);
		return SKS_SHORT_BUFFER;
	}

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	TEE_MemFill(&info, 0, sizeof(info));

	/* Set slot description to the device UUID if available */
	if (TEE_GetPropertyAsUUID(TEE_PROPSET_TEE_IMPLEMENTATION,
				"gpd.tee.deviceID", &dev_id) == TEE_SUCCESS) {
		snprintf(dev_uuid, sizeof(dev_uuid),
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			dev_id.timeLow, dev_id.timeMid,
			dev_id.timeHiAndVersion,
			dev_id.clockSeqAndNode[0], dev_id.clockSeqAndNode[1],
			dev_id.clockSeqAndNode[2], dev_id.clockSeqAndNode[3],
			dev_id.clockSeqAndNode[4], dev_id.clockSeqAndNode[5],
			dev_id.clockSeqAndNode[6], dev_id.clockSeqAndNode[7]);
		PADDED_STRING_COPY(info.slotDescription, dev_uuid);
	} else {
		PADDED_STRING_COPY(info.slotDescription, desc);
	}
	PADDED_STRING_COPY(info.manufacturerID, manuf);

	info.flags |= SKS_CKFS_TOKEN_PRESENT;
	info.flags |= SKS_CKFS_REMOVABLE_DEVICE;
	info.flags &= ~SKS_CKFS_HW_SLOT;

	TEE_MemMove(&info.hardwareVersion, &hwver, sizeof(hwver));
	TEE_MemMove(&info.firmwareVersion, &fwver, sizeof(fwver));

	out->memref.size = sizeof(struct sks_slot_info);
	TEE_MemMove(out->memref.buffer, &info, out->memref.size);

	return SKS_OK;
}

uint32_t entry_ck_token_info(TEE_Param *ctrl, TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	struct ck_token *token = NULL;
	const char manuf[] = SKS_CRYPTOKI_TOKEN_MANUFACTURER;
	const char model[] = SKS_CRYPTOKI_TOKEN_MODEL;
	const char hwver[] = SKS_CRYPTOKI_TOKEN_HW_VERSION;
	const char fwver[] = SKS_CRYPTOKI_TOKEN_FW_VERSION;
	char sernu[] = SKS_CRYPTOKI_TOKEN_SERIAL_NUMBER;
	struct sks_token_info info;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));
	TEE_MemFill(&info, 0, sizeof(info));

	if (!ctrl || in || !out)
		return SKS_BAD_PARAM;

	if (out->memref.size < sizeof(struct sks_token_info)) {
		out->memref.size = sizeof(struct sks_token_info);
		return SKS_SHORT_BUFFER;
	}

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	if (snprintf(sernu + sizeof(sernu) - 2, 2, "%1d", token_id) >= 2)
		TEE_Panic(0);

	TEE_MemFill(&info, 0, sizeof(info));

	PADDED_STRING_COPY(info.label, token->db_main->label);
	PADDED_STRING_COPY(info.manufacturerID, manuf);
	PADDED_STRING_COPY(info.model, model);
	PADDED_STRING_COPY(info.serialNumber, sernu);

	info.flags = token->db_main->flags;

	/* TODO */
	info.ulMaxSessionCount = ~0;
	info.ulSessionCount = token->session_count;
	info.ulMaxRwSessionCount = ~0;
	info.ulRwSessionCount = token->rw_session_count;
	/* TODO */
	info.ulMaxPinLen = 128;
	info.ulMinPinLen = 10;
	/* TODO */
	info.ulTotalPublicMemory = ~0;
	info.ulFreePublicMemory = ~0;
	info.ulTotalPrivateMemory = ~0;
	info.ulFreePrivateMemory = ~0;

	TEE_MemMove(&info.hardwareVersion, &hwver, sizeof(hwver));
	TEE_MemMove(&info.firmwareVersion, &fwver, sizeof(hwver));

	// TODO: get time and convert from reference into YYYYMMDDhhmmss/UTC
	TEE_MemFill(info.utcTime, 0, sizeof(info.utcTime));

	/* Return to caller with data */
	out->memref.size = sizeof(struct sks_token_info);
	TEE_MemMove(out->memref.buffer, &info, out->memref.size);

	return SKS_OK;
}

uint32_t entry_ck_token_mecha_ids(TEE_Param *ctrl,
				  TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	struct ck_token *token = NULL;
	uint32_t mechanisms_count = (uint32_t)get_supported_mechanisms(NULL, 0);
	size_t __maybe_unused count = 0;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || !out)
		return SKS_BAD_PARAM;

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	if (out->memref.size < mechanisms_count * sizeof(uint32_t)) {
		out->memref.size = mechanisms_count * sizeof(uint32_t);
		return SKS_SHORT_BUFFER;
	}

	out->memref.size = sizeof(uint32_t) *
		get_supported_mechanisms(out->memref.buffer, mechanisms_count);

	assert(out->memref.size == mechanisms_count * sizeof(uint32_t));

#ifdef DEBUG
	for (count = 0; count < mechanisms_count; count++) {
		IMSG("SKSt%" PRIu32 ": mechanism 0x%04" PRIx32 ": %s",
			token_id, ((uint32_t *)out->memref.buffer)[count],
			sks2str_proc(((uint32_t *)out->memref.buffer)[count]));
	}
#endif

	return SKS_OK;
}

static uint32_t supported_mechanism_info_flag(uint32_t proc_id)
{
	uint32_t flags = 0;

	switch (proc_id) {
	case SKS_CKM_GENERIC_SECRET_KEY_GEN:
	case SKS_CKM_AES_KEY_GEN:
		flags = SKS_CKFM_GENERATE;
		break;
	case SKS_CKM_AES_ECB:
	case SKS_CKM_AES_CBC:
	case SKS_CKM_AES_CBC_PAD:
	case SKS_CKM_AES_CTR:
	case SKS_CKM_AES_CTS:
	case SKS_CKM_AES_GCM:
	case SKS_CKM_AES_CCM:
		flags = SKS_CKFM_ENCRYPT | SKS_CKFM_DECRYPT |
			SKS_CKFM_WRAP | SKS_CKFM_UNWRAP;
		break;
	case SKS_CKM_AES_GMAC:
		flags = SKS_CKFM_SIGN | SKS_CKFM_VERIFY | SKS_CKFM_DERIVE;
		break;
	case SKS_CKM_AES_CMAC:
	case SKS_CKM_AES_CMAC_GENERAL:
	case SKS_CKM_MD5_HMAC:
	case SKS_CKM_SHA_1_HMAC:
	case SKS_CKM_SHA224_HMAC:
	case SKS_CKM_SHA256_HMAC:
	case SKS_CKM_SHA384_HMAC:
	case SKS_CKM_SHA512_HMAC:
	case SKS_CKM_AES_XCBC_MAC:
		flags = SKS_CKFM_SIGN | SKS_CKFM_VERIFY;
		break;
	case SKS_CKM_AES_ECB_ENCRYPT_DATA:
	case SKS_CKM_AES_CBC_ENCRYPT_DATA:
		flags = SKS_CKFM_DERIVE;
		break;
	case SKS_CKM_EC_KEY_PAIR_GEN:
	case SKS_CKM_RSA_PKCS_KEY_PAIR_GEN:
		flags = SKS_CKFM_GENERATE_PAIR;
		break;
	case SKS_CKM_ECDSA:
	case SKS_CKM_ECDSA_SHA1:
	case SKS_CKM_ECDSA_SHA224:
	case SKS_CKM_ECDSA_SHA256:
	case SKS_CKM_ECDSA_SHA384:
	case SKS_CKM_ECDSA_SHA512:
		flags = SKS_CKFM_SIGN | SKS_CKFM_VERIFY;
		break;
	case SKS_CKM_ECDH1_DERIVE:
	case SKS_CKM_ECDH1_COFACTOR_DERIVE:
	case SKS_CKM_ECMQV_DERIVE:
		flags = SKS_CKFM_DERIVE;
		break;
	case SKS_CKM_ECDH_AES_KEY_WRAP:
		flags = SKS_CKFM_WRAP | SKS_CKFM_UNWRAP;
		break;
	case SKS_CKM_RSA_PKCS:
	case SKS_CKM_RSA_X_509:
		flags = SKS_CKFM_ENCRYPT | SKS_CKFM_DECRYPT |
			SKS_CKFM_SIGN | SKS_CKFM_VERIFY |
			SKS_CKFM_SIGN_RECOVER | SKS_CKFM_VERIFY_RECOVER |
			SKS_CKFM_WRAP | SKS_CKFM_UNWRAP;
		break;
	case SKS_CKM_RSA_9796:
		flags = SKS_CKFM_SIGN | SKS_CKFM_VERIFY |
			SKS_CKFM_SIGN_RECOVER | SKS_CKFM_VERIFY_RECOVER;
		break;

	case SKS_CKM_RSA_PKCS_OAEP:
		flags = SKS_CKFM_ENCRYPT | SKS_CKFM_DECRYPT |
			SKS_CKFM_WRAP | SKS_CKFM_UNWRAP;
		break;
	case SKS_CKM_RSA_PKCS_PSS:
	case SKS_CKM_SHA1_RSA_PKCS:
	case SKS_CKM_SHA224_RSA_PKCS:
	case SKS_CKM_SHA256_RSA_PKCS:
	case SKS_CKM_SHA384_RSA_PKCS:
	case SKS_CKM_SHA512_RSA_PKCS:
	case SKS_CKM_SHA1_RSA_PKCS_PSS:
	case SKS_CKM_SHA224_RSA_PKCS_PSS:
	case SKS_CKM_SHA256_RSA_PKCS_PSS:
	case SKS_CKM_SHA384_RSA_PKCS_PSS:
	case SKS_CKM_SHA512_RSA_PKCS_PSS:
		flags = SKS_CKFM_SIGN | SKS_CKFM_VERIFY;
		break;
	case SKS_CKM_RSA_AES_KEY_WRAP:
		flags = SKS_CKFM_WRAP | SKS_CKFM_UNWRAP;
		break;
	default:
		TEE_Panic(proc_id);
		break;
	}

	assert(check_pkcs11_mechanism_flags(proc_id, flags) == 0);

	return flags;
}

static void supported_mechanism_key_size(uint32_t proc_id,
					 uint32_t *min_key_size,
					 uint32_t *max_key_size,
					 bool bit_size_only)
{
	uint32_t mult = bit_size_only ? 8 : 1;

	switch (proc_id) {
	case SKS_CKM_GENERIC_SECRET_KEY_GEN:
		*min_key_size = 1;		/* in bits */
		*max_key_size = 4096;		/* in bits */
		break;
	case SKS_CKM_MD5_HMAC:
		*min_key_size = 16 * mult;
		*max_key_size = 16 * mult;
		break;
	case SKS_CKM_SHA_1_HMAC:
		*min_key_size = 20 * mult;
		*max_key_size = 20 * mult;
		break;
	case SKS_CKM_SHA224_HMAC:
		*min_key_size = 28 * mult;
		*max_key_size = 28 * mult;
		break;
	case SKS_CKM_SHA256_HMAC:
		*min_key_size = 32 * mult;
		*max_key_size = 32 * mult;
		break;
	case SKS_CKM_SHA384_HMAC:
		*min_key_size = 48 * mult;
		*max_key_size = 48 * mult;
		break;
	case SKS_CKM_SHA512_HMAC:
		*min_key_size = 64 * mult;
		*max_key_size = 64 * mult;
		break;
	case SKS_CKM_AES_XCBC_MAC:
		*min_key_size = 28 * mult;
		*max_key_size = 28 * mult;
		break;
	case SKS_CKM_AES_KEY_GEN:
	case SKS_CKM_AES_ECB:
	case SKS_CKM_AES_CBC:
	case SKS_CKM_AES_CBC_PAD:
	case SKS_CKM_AES_CTR:
	case SKS_CKM_AES_CTS:
	case SKS_CKM_AES_GCM:
	case SKS_CKM_AES_CCM:
	case SKS_CKM_AES_GMAC:
	case SKS_CKM_AES_CMAC:
	case SKS_CKM_AES_CMAC_GENERAL:
		*min_key_size = 16 * mult;
		*max_key_size = 32 * mult;
		break;
	case SKS_CKM_EC_KEY_PAIR_GEN:
	case SKS_CKM_ECDSA:
	case SKS_CKM_ECDSA_SHA1:
	case SKS_CKM_ECDSA_SHA224:
	case SKS_CKM_ECDSA_SHA256:
	case SKS_CKM_ECDSA_SHA384:
	case SKS_CKM_ECDSA_SHA512:
	case SKS_CKM_ECDH1_DERIVE:
	case SKS_CKM_ECDH1_COFACTOR_DERIVE:
	case SKS_CKM_ECMQV_DERIVE:
	case SKS_CKM_ECDH_AES_KEY_WRAP:
		*min_key_size = 160;	/* in bits */
		*max_key_size = 521;	/* in bits */
		break;
	case SKS_CKM_RSA_PKCS_KEY_PAIR_GEN:
	case SKS_CKM_RSA_PKCS:
	case SKS_CKM_RSA_9796:
	case SKS_CKM_RSA_X_509:
	case SKS_CKM_SHA1_RSA_PKCS:
	case SKS_CKM_RSA_PKCS_OAEP:
	case SKS_CKM_SHA1_RSA_PKCS_PSS:
	case SKS_CKM_SHA256_RSA_PKCS:
	case SKS_CKM_SHA384_RSA_PKCS:
	case SKS_CKM_SHA512_RSA_PKCS:
	case SKS_CKM_SHA256_RSA_PKCS_PSS:
	case SKS_CKM_SHA384_RSA_PKCS_PSS:
	case SKS_CKM_SHA512_RSA_PKCS_PSS:
	case SKS_CKM_SHA224_RSA_PKCS:
	case SKS_CKM_SHA224_RSA_PKCS_PSS:
		*min_key_size = 256;	/* in bits */
		*max_key_size = 4096;	/* in bits */
		break;
	default:
		*min_key_size = 0;
		*max_key_size = 0;
		break;
	}
}

uint32_t entry_ck_token_mecha_info(TEE_Param *ctrl,
				   TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	uint32_t type = 0;
	struct ck_token *token = NULL;
	struct sks_mechanism_info *info = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || !out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get(&ctrlargs, &type, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	if (!mechanism_is_supported(type))
		return SKS_CKR_MECHANISM_INVALID;

	if (out->memref.size < sizeof(info)) {
		out->memref.size = sizeof(info);
		return SKS_SHORT_BUFFER;
	}

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	info = (struct sks_mechanism_info *)out->memref.buffer;

	info->flags = supported_mechanism_info_flag(type);

	supported_mechanism_key_size(type, &info->min_key_size,
					&info->max_key_size, false);

	out->memref.size = sizeof(struct sks_mechanism_info);

	IMSG("SKSt%" PRIu32 ": mechanism 0x%" PRIx32 " info", token_id, type);

	return SKS_OK;
}

/* Select the read-only/read-write state for session login state */
static void set_session_state(struct pkcs11_client *client,
			      struct pkcs11_session *session, bool readonly)
{
	struct pkcs11_session *sess = NULL;
	enum pkcs11_session_state state = PKCS11_SESSION_RESET;

	/*
	 * No need to check all client session, only the first session on
	 * target token gives client login configuration.
	 */
	TAILQ_FOREACH(sess, &client->session_list, link) {
		assert(sess != session);

		if (sess->token != session->token)
			continue;

		switch (sess->state) {
		case PKCS11_SESSION_PUBLIC_READ_WRITE:
		case PKCS11_SESSION_PUBLIC_READ_ONLY:
			state = PKCS11_SESSION_PUBLIC_READ_WRITE;
			break;
		case PKCS11_SESSION_USER_READ_WRITE:
		case PKCS11_SESSION_USER_READ_ONLY:
			state = PKCS11_SESSION_USER_READ_WRITE;
			break;
		case PKCS11_SESSION_SO_READ_WRITE:
			state = PKCS11_SESSION_SO_READ_WRITE;
			break;
		default:
			TEE_Panic(0);
		}
		break;
	 }

	switch (state) {
	case PKCS11_SESSION_USER_READ_WRITE:
		session->state = readonly ? PKCS11_SESSION_USER_READ_ONLY :
					  PKCS11_SESSION_USER_READ_WRITE;
		break;
	case PKCS11_SESSION_SO_READ_WRITE:
		/* SO cannot open read-only sessions */
		if (readonly)
			TEE_Panic(0);

		session->state = PKCS11_SESSION_PUBLIC_READ_ONLY;
		break;
	default:
		session->state = readonly ? PKCS11_SESSION_PUBLIC_READ_ONLY :
					  PKCS11_SESSION_PUBLIC_READ_WRITE;
		break;
	}
}

static void session_login_user(struct pkcs11_session *session)
{
	struct pkcs11_client *client = tee_session2client(session->tee_session);
	struct pkcs11_session *sess = NULL;

	TAILQ_FOREACH(sess, &client->session_list, link) {
		if (sess->token != session->token)
			continue;

		if (pkcs11_session_is_read_write(sess))
			sess->state = PKCS11_SESSION_USER_READ_WRITE;
		else
			sess->state = PKCS11_SESSION_USER_READ_ONLY;
	}
}

static void session_login_so(struct pkcs11_session *session)
{
	struct pkcs11_client *client = tee_session2client(session->tee_session);
	struct pkcs11_session *sess = NULL;

	TAILQ_FOREACH(sess, &client->session_list, link) {
		if (sess->token != session->token)
			continue;

		if (pkcs11_session_is_read_write(sess))
			sess->state = PKCS11_SESSION_SO_READ_WRITE;
		else
			TEE_Panic(0);
	}
}

static void session_logout(struct pkcs11_session *session)
{
	struct pkcs11_client *client = tee_session2client(session->tee_session);
	struct pkcs11_session *sess = NULL;
	struct sks_object *obj = NULL;

	TAILQ_FOREACH(sess, &client->session_list, link) {
		if (sess->token != session->token)
			continue;

		LIST_FOREACH(obj, &sess->object_list, link) {
			if (!object_is_private(obj->attributes))
				continue;

			destroy_object(sess, obj, true);
			handle_put(&sess->object_handle_db,
				   sks_object2handle(obj, sess));
		}

		if (pkcs11_session_is_read_write(sess))
			sess->state = PKCS11_SESSION_PUBLIC_READ_WRITE;
		else
			sess->state = PKCS11_SESSION_PUBLIC_READ_ONLY;
	}
}

/* ctrl=[slot-id], in=unused, out=[session-handle] */
static uint32_t open_ck_session(uintptr_t tee_session, TEE_Param *ctrl,
				TEE_Param *in, TEE_Param *out, bool readonly)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	struct ck_token *token = NULL;
	struct pkcs11_session *session = NULL;
	struct pkcs11_client *client = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || !out)
		return SKS_BAD_PARAM;

	if (out->memref.size < sizeof(uint32_t)) {
		out->memref.size = sizeof(uint32_t);
		return SKS_SHORT_BUFFER;
	}

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	if (!readonly && token->state == PKCS11_TOKEN_READ_ONLY) {
		return SKS_CKR_TOKEN_WRITE_PROTECTED;
	}

	client = tee_session2client(tee_session);
	if (!client) {
		EMSG("Unexpected invalid TEE session handle");
		return SKS_FAILED;
	}

	if (readonly) {
		TAILQ_FOREACH(session, &client->session_list, link) {
			if (session->state == PKCS11_SESSION_SO_READ_WRITE) {
				return SKS_CKR_SESSION_READ_WRITE_SO_EXISTS;
			}
		}
	}

	session = TEE_Malloc(sizeof(*session), TEE_MALLOC_FILL_ZERO);
	if (!session)
		return SKS_MEMORY;

	session->handle = handle_get(&client->session_handle_db, session);
	if (!session->handle) {
		TEE_Free(session);
		return SKS_MEMORY;
	}

	session->tee_session = tee_session;
	session->token = token;
	session->client = client;

	LIST_INIT(&session->object_list);
	handle_db_init(&session->object_handle_db);

	set_session_state(client, session, readonly);

	TAILQ_INSERT_HEAD(&client->session_list, session, link);

	session->token->session_count++;
	if (!readonly)
		session->token->rw_session_count++;

	*(uint32_t *)out->memref.buffer = session->handle;
	out->memref.size = sizeof(uint32_t);

	IMSG("SKSs%" PRIu32 ": open", session->handle);

	return SKS_OK;
}

/* ctrl=[slot-id], in=unused, out=[session-handle] */
uint32_t entry_ck_token_ro_session(uintptr_t tee_session, TEE_Param *ctrl,
				   TEE_Param *in, TEE_Param *out)
{
	return open_ck_session(tee_session, ctrl, in, out, true);
}

/* ctrl=[slot-id], in=unused, out=[session-handle] */
uint32_t entry_ck_token_rw_session(uintptr_t tee_session, TEE_Param *ctrl,
				   TEE_Param *in, TEE_Param *out)
{
	return open_ck_session(tee_session, ctrl, in, out, false);
}

static void close_ck_session(struct pkcs11_session *session)
{
	release_active_processing(session);

	/* No need to put object handles, the whole database is destroyed */
	while (!LIST_EMPTY(&session->object_list)) {
		destroy_object(session, LIST_FIRST(&session->object_list),
				true);
	}

	release_session_find_obj_context(session);

	TAILQ_REMOVE(&session->client->session_list, session, link);
	handle_put(&session->client->session_handle_db, session->handle);
	handle_db_destroy(&session->object_handle_db);

	// If no more session, next opened one will simply be Public login

	session->token->session_count--;
	if (pkcs11_session_is_read_write(session))
		session->token->rw_session_count--;

	TEE_Free(session);

	IMSG("SKSs%" PRIu32 ": close", session->handle);
}

/* ctrl=[session-handle], in=unused, out=unused */
uint32_t entry_ck_token_close_session(uintptr_t tee_session, TEE_Param *ctrl,
				      TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out || ctrl->memref.size < sizeof(uint32_t))
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	close_ck_session(session);

	return SKS_OK;
}

uint32_t entry_ck_token_close_all(uintptr_t tee_session, TEE_Param *ctrl,
				  TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t token_id = 0;
	struct ck_token *token = NULL;
	struct pkcs11_session *session = NULL;
	struct pkcs11_session *next = NULL;
	struct pkcs11_client *client = tee_session2client(tee_session);

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &token_id, sizeof(uint32_t));
	if (rv)
		return rv;

	token = get_token(token_id);
	if (!token)
		return SKS_CKR_SLOT_ID_INVALID;

	IMSG("SKSt%" PRIu32 ": close sessions", token_id);

	TAILQ_FOREACH_SAFE(session, &client->session_list, link, next) {
		if (session->token == token)
			close_ck_session(session);
	}

	return SKS_OK;
}

/* ctrl=[session-handle], in=unused, out=[session-info] */
uint32_t entry_ck_token_session_info(uintptr_t tee_session, TEE_Param *ctrl,
				  TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;
	struct sks_session_info info;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));
	TEE_MemFill(&info, 0, sizeof(info));

	if (!ctrl || in || !out || ctrl->memref.size < sizeof(uint32_t))
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	if (out->memref.size < sizeof(struct sks_session_info)) {
		out->memref.size = sizeof(struct sks_session_info);
		return SKS_SHORT_BUFFER;
	}

	if ((uintptr_t)out->memref.buffer & 0x3UL)
		return SKS_BAD_PARAM;

	info.slot_id = get_token_id(session->token);
	switch (session->state) {
	case PKCS11_SESSION_PUBLIC_READ_WRITE:
		info.state = SKS_CKSS_RW_PUBLIC_SESSION;
		break;
	case PKCS11_SESSION_PUBLIC_READ_ONLY:
		info.state = SKS_CKSS_RO_PUBLIC_SESSION;
		break;
	case PKCS11_SESSION_USER_READ_WRITE:
		info.state = SKS_CKSS_RW_USER_FUNCTIONS;
		break;
	case PKCS11_SESSION_USER_READ_ONLY:
		info.state = SKS_CKSS_RO_USER_FUNCTIONS;
		break;
	case PKCS11_SESSION_SO_READ_WRITE:
		info.state = SKS_CKSS_RW_SO_FUNCTIONS;
		break;
	default:
		TEE_Panic(0);
	}
	info.flags = SKS_CKFS_SERIAL_SESSION;
	if (session->state == PKCS11_SESSION_USER_READ_WRITE ||
			session->state == PKCS11_SESSION_PUBLIC_READ_WRITE)
		info.flags |= SKS_CKFS_RW_SESSION;
	info.error_code = 0;

	/* Return to caller with data */
	TEE_MemMove(out->memref.buffer, &info, sizeof(info));

	return SKS_OK;
}

static uint32_t set_pin(struct pkcs11_session *session,
			uint8_t *new_pin, size_t new_pin_size,
			uint32_t user_type)
{
	uint8_t *cpin = NULL;
	uint32_t *pin_count = NULL;
	uint32_t *pin_size = NULL;
	uint8_t *pin = NULL;
	TEE_ObjectHandle pin_key_hdl;
	uint32_t flag_mask = 0;

	TEE_MemFill(&pin_key_hdl, 0, sizeof(pin_key_hdl));

	if (session->token->db_main->flags & SKS_CKFT_WRITE_PROTECTED)
		return SKS_CKR_TOKEN_WRITE_PROTECTED;

	if (!pkcs11_session_is_read_write(session))
		return SKS_CKR_SESSION_READ_ONLY;

	if (new_pin_size < 8 || new_pin_size > SKS_TOKEN_PIN_SIZE)
		return SKS_CKR_PIN_LEN_RANGE;

	switch (user_type) {
	case SKS_CKU_SO:
		pin = session->token->db_main->so_pin;
		pin_size = &session->token->db_main->so_pin_size;
		pin_count = &session->token->db_main->so_pin_count;
		pin_key_hdl = session->token->pin_hdl[0];
		flag_mask = SKS_CKFT_SO_PIN_COUNT_LOW |
				SKS_CKFT_SO_PIN_FINAL_TRY |
				SKS_CKFT_SO_PIN_LOCKED |
				SKS_CKFT_SO_PIN_TO_BE_CHANGED;
		break;
	case SKS_CKU_USER:
		pin = session->token->db_main->user_pin;
		pin_size = &session->token->db_main->user_pin_size;
		pin_count = &session->token->db_main->user_pin_count;
		pin_key_hdl = session->token->pin_hdl[1];
		flag_mask = SKS_CKFT_USER_PIN_COUNT_LOW |
				SKS_CKFT_USER_PIN_FINAL_TRY |
				SKS_CKFT_USER_PIN_LOCKED |
				SKS_CKFT_USER_PIN_TO_BE_CHANGED;
		break;
	default:
		return SKS_FAILED;
	}

	cpin = TEE_Malloc(SKS_TOKEN_PIN_SIZE, TEE_MALLOC_FILL_ZERO);
	if (!cpin)
		return SKS_MEMORY;

	TEE_MemMove(cpin, new_pin, new_pin_size);

	cipher_pin(pin_key_hdl, cpin, SKS_TOKEN_PIN_SIZE);

	TEE_MemMove(pin, cpin, SKS_TOKEN_PIN_SIZE);
	*pin_size = new_pin_size;
	*pin_count = 0;

	session->token->db_main->flags &= ~flag_mask;

	if (user_type == SKS_CKU_USER)
		session->token->db_main->flags |= SKS_CKFT_USER_PIN_INITIALIZED;

	// Paranoia: Check unmodified old content is still valid
	update_persistent_db(session->token,
			     0, sizeof(*session->token->db_main));

	TEE_Free(cpin);

	return SKS_OK;
}

/* ctrl=[session-handle][pin-size]{pin-arrays], in=unused, out=unused */
uint32_t entry_init_pin(uintptr_t tee_session, TEE_Param *ctrl,
			TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;
	uint32_t pin_size = 0;
	void *pin = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	if (!pkcs11_session_is_security_officer(session))
		return SKS_CKR_USER_NOT_LOGGED_IN;

	rv = serialargs_get(&ctrlargs, &pin_size, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get_ptr(&ctrlargs, &pin, pin_size);
	if (rv)
		return rv;

	assert(session->token->db_main->flags & SKS_CKFT_TOKEN_INITIALIZED);

	IMSG("SKSs%" PRIu32 ": init PIN", session_handle);

	return set_pin(session, pin, pin_size, SKS_CKU_USER);
}

static uint32_t check_so_pin(struct pkcs11_session *session,
			     uint8_t *pin, size_t pin_size)
{
	struct ck_token *token = session->token;
	uint8_t *cpin = NULL;
	int pin_rc = 0;

	/* Note: intentional return code USER_PIN_NOT_INITIALIZED */
	if (!token->db_main->so_pin_size ||
	    !(token->db_main->flags & SKS_CKFT_TOKEN_INITIALIZED))
		return SKS_CKR_USER_PIN_NOT_INITIALIZED;

	if (token->db_main->flags & SKS_CKFT_SO_PIN_LOCKED)
		return SKS_CKR_PIN_LOCKED;

	cpin = TEE_Malloc(SKS_TOKEN_PIN_SIZE, TEE_MALLOC_FILL_ZERO);
	if (!cpin)
		return SKS_MEMORY;

	if (pin_size > SKS_TOKEN_PIN_SIZE) {
		TEE_Free(cpin);
		return SKS_BAD_PARAM;
	}

	TEE_MemMove(cpin, pin, pin_size);
	cipher_pin(token->pin_hdl[0], cpin, SKS_TOKEN_PIN_SIZE);

	pin_rc = 0;

	if (token->db_main->so_pin_size != pin_size)
		pin_rc = 1;

	if (buf_compare_ct(token->db_main->so_pin, cpin, SKS_TOKEN_PIN_SIZE))
		pin_rc = 1;

	TEE_Free(cpin);

	if (pin_rc) {
		token->db_main->flags |= SKS_CKFT_SO_PIN_COUNT_LOW;
		token->db_main->so_pin_count++;

		if (token->db_main->so_pin_count == 6)
			token->db_main->flags |= SKS_CKFT_SO_PIN_FINAL_TRY;
		if (token->db_main->so_pin_count == 7)
			token->db_main->flags |= SKS_CKFT_SO_PIN_LOCKED;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      flags),
				     sizeof(token->db_main->flags));

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      so_pin_count),
				     sizeof(token->db_main->so_pin_count));

		if (token->db_main->flags & SKS_CKFT_SO_PIN_LOCKED)
			return SKS_CKR_PIN_LOCKED;

		return SKS_CKR_PIN_INCORRECT;
	}

	if (token->db_main->so_pin_count) {
		token->db_main->so_pin_count = 0;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      so_pin_count),
				     sizeof(token->db_main->so_pin_count));
	}

	if (token->db_main->flags & (SKS_CKFT_SO_PIN_COUNT_LOW |
					SKS_CKFT_SO_PIN_FINAL_TRY)) {
		token->db_main->flags &= ~(SKS_CKFT_SO_PIN_COUNT_LOW |
					   SKS_CKFT_SO_PIN_FINAL_TRY);

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      flags),
				     sizeof(token->db_main->flags));
	}

	return SKS_OK;
}

static uint32_t check_user_pin(struct pkcs11_session *session,
				uint8_t *pin, size_t pin_size)
{
	struct ck_token *token = session->token;
	uint8_t *cpin = NULL;
	int pin_rc = 0;

	if (!token->db_main->user_pin_size ||
	    !(token->db_main->flags & SKS_CKFT_USER_PIN_INITIALIZED))
		return SKS_CKR_USER_PIN_NOT_INITIALIZED;

	if (token->db_main->flags & SKS_CKFT_USER_PIN_LOCKED)
		return SKS_CKR_PIN_LOCKED;

	cpin = TEE_Malloc(SKS_TOKEN_PIN_SIZE, TEE_MALLOC_FILL_ZERO);
	if (!cpin)
		return SKS_MEMORY;

	if (pin_size > SKS_TOKEN_PIN_SIZE) {
		TEE_Free(cpin);
		return SKS_BAD_PARAM;
	}

	TEE_MemMove(cpin, pin, pin_size);
	cipher_pin(token->pin_hdl[1], cpin, SKS_TOKEN_PIN_SIZE);

	pin_rc = 0;

	if (token->db_main->user_pin_size != pin_size)
		pin_rc = 1;

	if (buf_compare_ct(token->db_main->user_pin, cpin, SKS_TOKEN_PIN_SIZE))
		pin_rc = 1;

	TEE_Free(cpin);

	if (pin_rc) {
		token->db_main->flags |= SKS_CKFT_USER_PIN_COUNT_LOW;
		token->db_main->user_pin_count++;

		if (token->db_main->user_pin_count == 6)
			token->db_main->flags |= SKS_CKFT_USER_PIN_FINAL_TRY;
		if (token->db_main->user_pin_count == 7)
			token->db_main->flags |= SKS_CKFT_USER_PIN_LOCKED;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      flags),
				     sizeof(token->db_main->flags));

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      user_pin_count),
				     sizeof(token->db_main->user_pin_count));

		if (token->db_main->flags & SKS_CKFT_USER_PIN_LOCKED)
			return SKS_CKR_PIN_LOCKED;

		return SKS_CKR_PIN_INCORRECT;
	}

	if (token->db_main->user_pin_count) {
		token->db_main->user_pin_count = 0;

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      user_pin_count),
				     sizeof(token->db_main->user_pin_count));
	}

	if (token->db_main->flags & (SKS_CKFT_USER_PIN_COUNT_LOW |
					SKS_CKFT_USER_PIN_FINAL_TRY)) {
		token->db_main->flags &= ~(SKS_CKFT_USER_PIN_COUNT_LOW |
				   SKS_CKFT_USER_PIN_FINAL_TRY);

		update_persistent_db(token,
				     offsetof(struct token_persistent_main,
					      flags),
				     sizeof(token->db_main->flags));
	}

	return SKS_OK;
}

/* ctrl=[session][old-size]{old-pin][pin-size]{pin], in=unused, out=unused */
uint32_t entry_set_pin(uintptr_t tee_session, TEE_Param *ctrl,
			TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;
	uint32_t old_pin_size = 0;
	uint32_t pin_size = 0;
	void *old_pin = NULL;
	void *pin = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get(&ctrlargs, &old_pin_size, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get_ptr(&ctrlargs, &old_pin, old_pin_size);
	if (rv)
		return rv;

	rv = serialargs_get(&ctrlargs, &pin_size, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get_ptr(&ctrlargs, &pin, pin_size);
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	if (!pkcs11_session_is_read_write(session))
		return SKS_CKR_SESSION_READ_ONLY;

	if (pkcs11_session_is_security_officer(session)) {
		if (!(session->token->db_main->flags &
		      SKS_CKFT_TOKEN_INITIALIZED))
			return SKS_ERROR;

		rv = check_so_pin(session, old_pin, old_pin_size);
		if (rv)
			return rv;

		return set_pin(session, pin, pin_size, SKS_CKU_SO);
	}

	if (!(session->token->db_main->flags & SKS_CKFT_USER_PIN_INITIALIZED))
		return SKS_ERROR;

	rv = check_user_pin(session, old_pin, old_pin_size);
	if (rv)
		return rv;

	IMSG("SKSs%" PRIu32 ": set PIN", session_handle);

	return set_pin(session, pin, pin_size, SKS_CKU_USER);
}

/* ctrl=[session][user_type][pin-size]{pin], in=unused, out=unused */
uint32_t entry_login(uintptr_t tee_session, TEE_Param *ctrl,
		     TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;
	struct pkcs11_session *sess = NULL;
	struct pkcs11_client *client = NULL;
	uint32_t user_type = 0;
	uint32_t pin_size = 0;
	void *pin = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	rv = serialargs_get(&ctrlargs, &user_type, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get(&ctrlargs, &pin_size, sizeof(uint32_t));
	if (rv)
		return rv;

	rv = serialargs_get_ptr(&ctrlargs, &pin, pin_size);
	if (rv)
		return rv;

	client = tee_session2client(tee_session);

	switch (user_type) {
	case SKS_CKU_SO:
		if (pkcs11_session_is_security_officer(session))
			return SKS_CKR_USER_ALREADY_LOGGED_IN;

		if (pkcs11_session_is_user(session))
			return SKS_CKR_USER_ANOTHER_ALREADY_LOGGED_IN;

		TAILQ_FOREACH(sess, &client->session_list, link)
			if (sess->token == session->token &&
			    !pkcs11_session_is_read_write(sess))
				return SKS_CKR_SESSION_READ_ONLY_EXISTS;

		TAILQ_FOREACH(client, &pkcs11_client_list, link) {
			TAILQ_FOREACH(sess, &client->session_list, link) {
				if (sess->token == session->token &&
				    !pkcs11_session_is_public(sess))
					return SKS_CKR_USER_TOO_MANY_TYPES;
			}
		}

		rv = check_so_pin(session, pin, pin_size);
		if (rv == SKS_OK)
			session_login_so(session);

		break;

	case SKS_CKU_USER:
		if (pkcs11_session_is_security_officer(session))
			return SKS_CKR_USER_ANOTHER_ALREADY_LOGGED_IN;

		if (pkcs11_session_is_user(session))
			return SKS_CKR_USER_ALREADY_LOGGED_IN;

		// TODO: check all client: if SO or user logged, we can return
		// CKR_USER_TOO_MANY_TYPES.

		rv = check_user_pin(session, pin, pin_size);
		if (rv == SKS_OK)
			session_login_user(session);

		break;

	case SKS_CKU_CONTEXT_SPECIFIC:
		if (!session_is_active(session) ||
		    !session->processing->always_authen)
			return SKS_CKR_OPERATION_NOT_INITIALIZED;

		if (pkcs11_session_is_public(session))
			return SKS_CKR_FUNCTION_FAILED;

		assert(pkcs11_session_is_user(session) ||
			pkcs11_session_is_security_officer(session));

		if (pkcs11_session_is_security_officer(session))
			rv = check_so_pin(session, pin, pin_size);
		else
			rv = check_user_pin(session, pin, pin_size);

		session->processing->relogged = (rv == SKS_OK);

		if (rv == SKS_CKR_PIN_LOCKED)
			session_logout(session);

		break;

	default:
		return SKS_CKR_USER_TYPE_INVALID;
	}

	if (!rv)
		IMSG("SKSs%" PRIu32 ": login", session_handle);

	return rv;
}

/* ctrl=[session], in=unused, out=unused */
uint32_t entry_logout(uintptr_t tee_session, TEE_Param *ctrl,
		      TEE_Param *in, TEE_Param *out)
{
	uint32_t rv = 0;
	struct serialargs ctrlargs;
	uint32_t session_handle = 0;
	struct pkcs11_session *session = NULL;

	TEE_MemFill(&ctrlargs, 0, sizeof(ctrlargs));

	if (!ctrl || in || out)
		return SKS_BAD_PARAM;

	serialargs_init(&ctrlargs, ctrl->memref.buffer, ctrl->memref.size);

	rv = serialargs_get(&ctrlargs, &session_handle, sizeof(uint32_t));
	if (rv)
		return rv;

	session = sks_handle2session(session_handle, tee_session);
	if (!session)
		return SKS_CKR_SESSION_HANDLE_INVALID;

	if (pkcs11_session_is_public(session))
		return SKS_CKR_USER_NOT_LOGGED_IN;

	session_logout(session);

	IMSG("SKSs%" PRIu32 ": logout", session_handle);

	return SKS_OK;
}


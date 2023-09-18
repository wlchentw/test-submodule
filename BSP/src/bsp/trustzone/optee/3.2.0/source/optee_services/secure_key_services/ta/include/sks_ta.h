/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017-2018, Linaro Limited
 */

#ifndef __SKS_TA_H__
#define __SKS_TA_H__

#include <sys/types.h>
#include <stdint.h>

#define TA_SKS_UUID { 0xfd02c9da, 0x306c, 0x48c7, \
			{ 0xa4, 0x9c, 0xbb, 0xd8, 0x27, 0xae, 0x86, 0xee } }

/* SKS trusted application version information */
#define SKS_VERSION_ID0		0
#define SKS_VERSION_ID1		0

/*
 * SKS_CMD_PING		Acknowledge TA presence and return TA version info
 *
 * Optional invocation parameter:
 *
 * [out]        memref[2] = [
 *                      32bit version0 value,
 *                      32bit version1 value
 *              ]
 */
#define SKS_CMD_PING			0x00000000

/*
 * SKS_CMD_CK_SLOT_LIST - Get the table of the valid slot IDs
 *
 * [out]        memref[2] = 32bit array slot_ids[slot counts]
 *
 * The TA instance may represent several PKCS#11 slots and associated tokens.
 * This command relates the PKCS#11 API function C_GetSlotList and return the
 * valid IDs recognized by the trusted application.
 */
#define SKS_CMD_CK_SLOT_LIST		0x00000001

/*
 * SKS_CMD_CK_SLOT_INFO - Get cryptoki structured slot information
 *
 * [in]		memref[0] = 32bit slot ID
 * [out]	memref[0] = 32bit fine grain return code
 * [out]        memref[2] = (struct sks_ck_slot_info)info
 *
 * The TA instance may represent several PKCS#11 slots and associated tokens.
 * This command relates the PKCS#11 API function C_GetSlotInfo and return the
 * information about the target slot.
 */
#define SKS_CMD_CK_SLOT_INFO		0x00000002

#define SKS_SLOT_DESC_SIZE		64
#define SKS_SLOT_MANUFACTURER_SIZE	32
#define SKS_SLOT_VERSION_SIZE		2

struct sks_slot_info {
	uint8_t slotDescription[SKS_SLOT_DESC_SIZE];
	uint8_t manufacturerID[SKS_SLOT_MANUFACTURER_SIZE];
	uint32_t flags;
	uint8_t hardwareVersion[SKS_SLOT_VERSION_SIZE];
	uint8_t firmwareVersion[SKS_SLOT_VERSION_SIZE];
};

/*
 * Values for sks_token_info::flags.
 * SKS_CKFS_<x> corresponds to cryptoki flag CKF_<x> related to slot flags.
 */
#define SKS_CKFS_TOKEN_PRESENT		(1U << 0)
#define SKS_CKFS_REMOVABLE_DEVICE	(1U << 1)
#define SKS_CKFS_HW_SLOT		(1U << 2)

/*
 * SKS_CMD_CK_TOKEN_INFO - Get cryptoki structured token information
 *
 * [in]		memref[0] = 32bit slot ID
 * [out]	memref[0] = 32bit fine grain return code
 * [out]        memref[2] = (struct sks_ck_token_info)info
 *
 * The TA instance may represent several PKCS#11 slots and associated tokens.
 * This command relates the PKCS#11 API function C_GetTokenInfo and return the
 * information about the target represented token.
 */
#define SKS_CMD_CK_TOKEN_INFO		0x00000003

#define SKS_TOKEN_LABEL_SIZE		32
#define SKS_TOKEN_MANUFACTURER_SIZE	32
#define SKS_TOKEN_MODEL_SIZE		16
#define SKS_TOKEN_SERIALNUM_SIZE	16

struct sks_token_info {
	uint8_t label[SKS_TOKEN_LABEL_SIZE];
	uint8_t manufacturerID[SKS_TOKEN_MANUFACTURER_SIZE];
	uint8_t model[SKS_TOKEN_MODEL_SIZE];
	uint8_t serialNumber[SKS_TOKEN_SERIALNUM_SIZE];
	uint32_t flags;
	uint32_t ulMaxSessionCount;
	uint32_t ulSessionCount;
	uint32_t ulMaxRwSessionCount;
	uint32_t ulRwSessionCount;
	uint32_t ulMaxPinLen;
	uint32_t ulMinPinLen;
	uint32_t ulTotalPublicMemory;
	uint32_t ulFreePublicMemory;
	uint32_t ulTotalPrivateMemory;
	uint32_t ulFreePrivateMemory;
	uint8_t hardwareVersion[2];
	uint8_t firmwareVersion[2];
	uint8_t utcTime[16];
};

/*
 * Values for sks_token_info::flags.
 * SKS_CKFT_<x> corresponds to cryptoki CKF_<x> related to token flags.
 */
#define SKS_CKFT_RNG					(1U << 0)
#define SKS_CKFT_WRITE_PROTECTED			(1U << 1)
#define SKS_CKFT_LOGIN_REQUIRED				(1U << 2)
#define SKS_CKFT_USER_PIN_INITIALIZED			(1U << 3)
#define SKS_CKFT_RESTORE_KEY_NOT_NEEDED			(1U << 4)
#define SKS_CKFT_CLOCK_ON_TOKEN				(1U << 5)
#define SKS_CKFT_PROTECTED_AUTHENTICATION_PATH		(1U << 6)
#define SKS_CKFT_DUAL_CRYPTO_OPERATIONS			(1U << 7)
#define SKS_CKFT_TOKEN_INITIALIZED			(1U << 8)
#define SKS_CKFT_USER_PIN_COUNT_LOW			(1U << 9)
#define SKS_CKFT_USER_PIN_FINAL_TRY			(1U << 10)
#define SKS_CKFT_USER_PIN_LOCKED			(1U << 11)
#define SKS_CKFT_USER_PIN_TO_BE_CHANGED			(1U << 12)
#define SKS_CKFT_SO_PIN_COUNT_LOW			(1U << 13)
#define SKS_CKFT_SO_PIN_FINAL_TRY			(1U << 14)
#define SKS_CKFT_SO_PIN_LOCKED				(1U << 15)
#define SKS_CKFT_SO_PIN_TO_BE_CHANGED			(1U << 16)
#define SKS_CKFT_ERROR_STATE				(1U << 17)

/*
 * SKS_CMD_CK_MECHANISM_IDS - Get list of the supported mechanisms
 *
 * [in]		memref[0] = 32bit slot ID
 * [out]	memref[0] = 32bit fine grain return code
 * [out]        memref[2] = 32bit array mechanism IDs
 *
 * This commands relates to the PKCS#11 API function C_GetMechanismList.
 */
#define SKS_CMD_CK_MECHANISM_IDS	0x00000004

/*
 * SKS_CMD_CK_MECHANISM_INFO - Get information on a specific mechanism
 *
 * [in]		memref[0] = [
 *			32bit slot ID,
 *			32bit mechanism ID
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]        memref[2] = (struct sks_mecha_info)info
 *
 * This commands relates to the PKCS#11 API function C_GetMechanismInfo.
 */
#define SKS_CMD_CK_MECHANISM_INFO	0x00000005

struct sks_mechanism_info {
	uint32_t min_key_size;
	uint32_t max_key_size;
	uint32_t flags;
};

/*
 * Values for sks_mechanism_info::flags.
 * SKS_CKFM_<x> strictly matches cryptoki CKF_<x> related to mechanism flags.
 */
#define SKS_CKFM_HW			(1U << 0)
#define SKS_CKFM_ENCRYPT		(1U << 8)
#define SKS_CKFM_DECRYPT		(1U << 9)
#define SKS_CKFM_DIGEST			(1U << 10)
#define SKS_CKFM_SIGN			(1U << 11)
#define SKS_CKFM_SIGN_RECOVER		(1U << 12)
#define SKS_CKFM_VERIFY			(1U << 13)
#define SKS_CKFM_VERIFY_RECOVER		(1U << 14)
#define SKS_CKFM_GENERATE		(1U << 15)
#define SKS_CKFM_GENERATE_PAIR		(1U << 16)
#define SKS_CKFM_WRAP			(1U << 17)
#define SKS_CKFM_UNWRAP			(1U << 18)
#define SKS_CKFM_DERIVE			(1U << 19)
#define SKS_CKFM_EC_F_P			(1U << 20)
#define SKS_CKFM_EC_F_2M		(1U << 21)
#define SKS_CKFM_EC_ECPARAMETERS	(1U << 22)
#define SKS_CKFM_EC_NAMEDCURVE		(1U << 23)
#define SKS_CKFM_EC_UNCOMPRESS		(1U << 24)
#define SKS_CKFM_EC_COMPRESS		(1U << 25)

/*
 * SKS_CMD_CK_INIT_TOKEN - Initialize PKCS#11 token
 *
 * [in]		memref[0] = [
 *			32bit slot ID,
 *			32bit PIN length,
 *			8bit array PIN[PIN length],
 *			8bit array label[32]
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_InitToken().
 */
#define SKS_CMD_CK_INIT_TOKEN		0x00000006

/*
 * SKS_CMD_CK_INIT_PIN - Initialize PKCS#11 token PIN
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit PIN length,
 *			8bit array PIN[PIN length]
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_InitPIN().
 */
#define SKS_CMD_CK_INIT_PIN		0x00000007

/*
 * SKS_CMD_CK_SET_PIN - Set PKCS#11 token PIN
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit old_pin_length,
 *			8bit array old_pin[old_pin_length],
 *			32bit new_pin_length,
 *			8bit array new_pin[new_pin_length]
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_SetPIN()
 */
#define SKS_CMD_CK_SET_PIN		0x00000008

/*
 * SKS_CMD_CK_OPEN_RO_SESSION - Open read-only session
 *
 * [in]		memref[0] = 32bit slot ID
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[0] = 32bit session handle
 *
 * This commands relates to the PKCS#11 API function C_OpenSession() for a
 * read-only session.
 */
#define SKS_CMD_CK_OPEN_RO_SESSION	0x00000009

/*
 * SKS_CMD_CK_OPEN_RW_SESSION - Open read/write session
 *
 * [in]		memref[0] = 32bit slot
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[0] = 32bit session handle
 *
 * This commands relates to the PKCS#11 API function C_OpenSession() for a
 * read/write session.
 */
#define SKS_CMD_CK_OPEN_RW_SESSION	0x0000000a

/*
 * SKS_CMD_CK_CLOSE_SESSION - Close an opened session
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_CloseSession().
 */
#define SKS_CMD_CK_CLOSE_SESSION	0x0000000b

/*
 * SKS_CMD_CK_SESSION_INFO - Get Cryptoki information on a session
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 * [out]        memref[2] = (struct sks_ck_session_info)info
 *
 * This commands relates to the PKCS#11 API function C_GetSessionInfo().
 */
#define SKS_CMD_CK_SESSION_INFO		0x0000000c

/*
 * Values for sks_session_info::state.
 * SKS_CKSS_<x> strictly matches cryptoki CKS_<x> related to session state.
 */
#define SKS_CKSS_RO_PUBLIC_SESSION	0
#define SKS_CKSS_RO_USER_FUNCTIONS	1
#define SKS_CKSS_RW_PUBLIC_SESSION	2
#define SKS_CKSS_RW_USER_FUNCTIONS	3
#define SKS_CKSS_RW_SO_FUNCTIONS	4

struct sks_session_info {
	uint32_t slot_id;
	uint32_t state;
	uint32_t flags;
	uint32_t error_code;
};

/*
 * Values for sks_session_info::flags.
 * SKS_CKFS_<x> strictly matches cryptoki CKF_<x> related to session flags.
 */
#define SKS_CKFS_RW_SESSION			(1U << 1)
#define SKS_CKFS_SERIAL_SESSION		(1U << 2)

/*
 * SKS_CMD_CK_CLOSE_ALL_SESSIONS - Close all client sessions on slot/token
 *
 * [in]		memref[0] = 32bit slot
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_CloseAllSessions().
 */
#define SKS_CMD_CK_CLOSE_ALL_SESSIONS	0x0000000d

/*
 * SKS_CMD_IMPORT_OBJECT - Import a raw object in the session or token
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = 32bit object handle
 *
 * This commands relates to the PKCS#11 API function C_CreateObject().
 */
#define SKS_CMD_IMPORT_OBJECT		0x0000000e

/**
 * Serialization of object attributes
 */

/*
 * sks_object_head - Header of object whose data are serialized in memory
 *
 * An object in made of several attributes. Attributes are store one next to
 * the other with byte alignment as serialized byte arrays. Appended
 * attributes byte arrays are prepend with this header structure that
 * defines the number of attribute items and the overall byte size of the
 * attrs byte array.
 *
 * @attrs_size - byte size of whole byte array attrs[]
 * @attrs_count - number of attribute items stored in attrs[]
 * @attrs - then starts the attributes data
 */
struct sks_object_head {
	uint32_t attrs_size;
	uint32_t attrs_count;
	uint8_t attrs[];
};

/*
 * Attribute reference in the TA ABI. Each attribute start with the header
 * structure followed by the attribute value, its byte size being defined
 * in the attribute header.
 *
 * @id - the 32bit identifier of the attribute, see SKS_CKA_<x>
 * @size - the 32bit value attribute byte size
 * @data - then starts the attribute value
 */
struct sks_attribute_head {
	uint32_t id;
	uint32_t size;
	uint8_t data[];
};

/*
 * SKS_CMD_DESTROY_OBJECT - Destroy an object
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit object handle
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This commands relates to the PKCS#11 API function C_DestroyObject().
 */
#define SKS_CMD_DESTROY_OBJECT		0x0000000f

/*
 * SKS_CMD_ENCRYPT_INIT - Initialize encryption processing
 * SKS_CMD_DECRYPT_INIT - Initialize decryption processing
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_attribute_head)mechanism + mecha parameters
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * These commands relate to the PKCS#11 API functions C_EncryptInit() and
 * C_DecryptInit.
 */
#define SKS_CMD_ENCRYPT_INIT		0x00000010
#define SKS_CMD_DECRYPT_INIT		0x00000011

/*
 * SKS_CMD_ENCRYPT_UPDATE - Update encryption processing
 * SKS_CMD_DECRYPT_UPDATE - Update decryption processing
 *
 * [in]		memref[0] = 32bit session handle
 * [in]		memref[1] = input data to be processed
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = output processed data
 *
 * These commands relate to the PKCS#11 API functions C_EncryptUpdate() and
 * C_DecryptUpdate.
 */
#define SKS_CMD_ENCRYPT_UPDATE		0x00000012
#define SKS_CMD_DECRYPT_UPDATE		0x00000013

/*
 * SKS_CMD_ENCRYPT_FINAL - Finalize encryption processing
 * SKS_CMD_DECRYPT_FINAL - Finalize decryption processing
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = output processed data
 *
 * These commands relate to the PKCS#11 API functions C_EncryptFinal() and
 * C_DecryptFinal.
 */
#define SKS_CMD_ENCRYPT_FINAL		0x00000014
#define SKS_CMD_DECRYPT_FINAL		0x00000015

/*
 * SKS_CMD_GENERATE_SYMM_KEY - Generate a symmetric key
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_attribute_head)mechanism + mecha parameters,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = 32bit key handle
 *
 * This command relates to the PKCS#11 API functions C_GenerateKey().
 */
#define SKS_CMD_GENERATE_SYMM_KEY	0x00000016

/*
 * SKS_CMD_SIGN_INIT - Initialize a signature computation processing
 * SKS_CMD_VERIFY_INIT - Initialize a signature verification processing
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit key handle,
 *			(struct sks_attribute_head)mechanism + mecha parameters,
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * These commands relate to the PKCS#11 API functions C_SignInit() and
 * C_VerifyInit.
 */
#define SKS_CMD_SIGN_INIT		0x00000017
#define SKS_CMD_VERIFY_INIT		0x00000018

/*
 * SKS_CMD_SIGN_UPDATE - Update a signature computation processing
 * SKS_CMD_VERIFY_UPDATE - Update a signature verification processing
 *
 * [in]		memref[0] = 32bit session handle
 * [in]		memref[1] = input data to be processed
 * [out]	memref[0] = 32bit fine grain return code
 *
 * These commands relate to the PKCS#11 API functions C_SignUpdate() and
 * C_VerifyUpdate.
 */
#define SKS_CMD_SIGN_UPDATE		0x00000019
#define SKS_CMD_VERIFY_UPDATE		0x0000001a

/*
 * SKS_CMD_SIGN_FINAL - Finalize a signature computation processing
 * SKS_CMD_VERIFY_FINAL - Finalize a signature verification processing
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = output processed data
 *
 * These commands relate to the PKCS#11 API functions C_SignFinal() and
 * C_VerifyFinal.
 */
#define SKS_CMD_SIGN_FINAL		0x0000001b
#define SKS_CMD_VERIFY_FINAL		0x0000001c

/*
 * SKS_CMD_FIND_OBJECTS_INIT - Initialize a objects search
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This command relates to the PKCS#11 API function C_FindOjectsInit().
 */
#define SKS_CMD_FIND_OBJECTS_INIT	0x0000001d

/*
 * SKS_CMD_FIND_OBJECTS - Get handles of matching objects
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = 32bit array object_handle_array[N]
 *
 * This command relates to the PKCS#11 API function C_FindOjects().
 * The size of object_handle_array depends output buffer size
 * provided by the client.
 */
#define SKS_CMD_FIND_OBJECTS		0x0000001e

/*
 * SKS_CMD_FIND_OBJECTS_FINAL - Finalize current objects search
 *
 * [in]		memref[0] = 32bit session handle
 * [out]	memref[0] = 32bit fine grain return code
 *
 * This command relates to the PKCS#11 API function C_FindOjectsFinal().
 */
#define SKS_CMD_FIND_OBJECTS_FINAL	0x0000001f

/*
 * SKS_CMD_GET_OBJECT_SIZE - Get size used by object in the TEE
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit key handle
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = 32bit object_byte_size
 */
#define SKS_CMD_GET_OBJECT_SIZE		0x00000020

/*
 * SKS_CMD_GET_ATTRIBUTE_VALUE - Get the value of object attribute(s)
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit object handle,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = (struct sks_object_head)attribs + attributes data
 */
#define SKS_CMD_GET_ATTRIBUTE_VALUE	0x00000021

/*
 * SKS_CMD_SET_ATTRIBUTE_VALUE - Set the value for object attribute(s)
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit object handle,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = (struct sks_object_head)attribs + attributes data
 */
#define SKS_CMD_SET_ATTRIBUTE_VALUE	0x00000022

/*
 * SKS_CMD_DERIVE_KEY - Derive a key from already provisioned parent key
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_attribute_head)mechanism + mecha parameters,
 *			32bit key handle,
 *			(struct sks_object_head)attribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = 32bit object handle
 */
#define SKS_CMD_DERIVE_KEY		0x00000023

/*
 * SKS_CMD_INIT_PIN - Initialize user PIN
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit PIN byte size,
 *			byte arrays: PIN data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 */
#define SKS_CMD_INIT_PIN		0x00000024

/*
 * SKS_CMD_SET_PIN - Change user PIN
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit old PIN byte size,
 *			byte arrays: PIN data
 *			32bit new PIN byte size,
 *			byte arrays: new PIN data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 */
#define SKS_CMD_SET_PIN			0x00000025

/*
 * SKS_CMD_LOGIN - Initialize user PIN
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit user identifier,
 *			32bit PIN byte size,
 *			byte arrays: PIN data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 */
#define SKS_CMD_LOGIN			0x00000026

/*
 * Values for user identifier parameter in SKS_CMD_LOGIN
 */
#define SKS_CKU_SO			0x000
#define SKS_CKU_USER			0x001
#define SKS_CKU_CONTEXT_SPECIFIC	0x002

/*
 * SKS_CMD_LOGOUT - Log out from token
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			32bit PIN byte size,
 *			byte array: PIN data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 */
#define SKS_CMD_LOGOUT			0x00000027

/*
 * SKS_CMD_GENERATE_KEY_PAIR - Generate an asymmetric key pair
 *
 * [in]		memref[0] = [
 *			32bit session handle,
 *			(struct sks_attribute_head)mechanism + mecha parameters,
 *			(struct sks_object_head)pubkey_attribs + attributes data
 *			(struct sks_object_head)privkeyattribs + attributes data
 *		]
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = [
 *			32bit public key handle,
 *			32bit prive key handle
 *		]
 *
 * This command relates to the PKCS#11 API functions C_GenerateKeyPair().
 */
#define SKS_CMD_GENERATE_KEY_PAIR	0x00000028

/*
 * SKS_CMD_ENCRYPT_ONESHOT - Update and finalize encryption processing
 * SKS_CMD_DECRYPT_ONESHOT - Update and finalize decryption processing
 *
 * [in]		memref[0] = 32bit session handle
 * [in]		memref[1] = input data to be processed
 * [out]	memref[0] = 32bit fine grain return code
 * [out]	memref[2] = output processed data
 *
 * These commands relate to the PKCS#11 API functions C_EncryptUpdate() and
 * C_DecryptUpdate.
 */
#define SKS_CMD_ENCRYPT_ONESHOT		0x00000029
#define SKS_CMD_DECRYPT_ONESHOT		0x0000002a

/*
 * SKS_CMD_SIGN_ONESHOT - Update and finalize a signature computation
 * SKS_CMD_VERIFY_ONESHOT - Update and finalize a signature verification
 *
 * [in]		memref[0] = 32bit session handle
 * [in]		memref[1] = input data to be processed
 * [out]	memref[0] = 32bit fine grain return code
 *
 * These commands relate to the PKCS#11 API functions C_SignUpdate() and
 * C_VerifyUpdate.
 */
#define SKS_CMD_SIGN_ONESHOT		0x0000002b
#define SKS_CMD_VERIFY_ONESHOT		0x0000002c

/*
 * Command return codes
 * SKS_CKR_<x> relates cryptoki CKR_<x> in meaning if not in value.
 */
#define SKS_CKR_OK				0x00000000
#define SKS_CKR_GENERAL_ERROR			0x00000001
#define SKS_CKR_DEVICE_MEMORY			0x00000002
#define SKS_CKR_ARGUMENTS_BAD			0x00000003
#define SKS_CKR_BUFFER_TOO_SMALL		0x00000004
#define SKS_CKR_FUNCTION_FAILED			0x00000005
#define SKS_CKR_SIGNATURE_INVALID		0x00000007
#define SKS_CKR_ATTRIBUTE_TYPE_INVALID		0x00000008
#define SKS_CKR_ATTRIBUTE_VALUE_INVALID		0x00000009
#define SKS_CKR_OBJECT_HANDLE_INVALID		0x0000000a
#define SKS_CKR_KEY_HANDLE_INVALID		0x0000000b
#define SKS_CKR_MECHANISM_INVALID		0x0000000c
#define SKS_CKR_SESSION_HANDLE_INVALID		0x0000000d
#define SKS_CKR_SLOT_ID_INVALID			0x0000000e
#define SKS_CKR_MECHANISM_PARAM_INVALID		0x0000000f
#define SKS_CKR_TEMPLATE_INCONSISTENT		0x00000010
#define SKS_CKR_TEMPLATE_INCOMPLETE		0x00000011
#define SKS_CKR_PIN_INCORRECT			0x00000012
#define SKS_CKR_PIN_LOCKED			0x00000013
#define SKS_CKR_PIN_EXPIRED			0x00000014
#define SKS_CKR_PIN_INVALID			0x00000015
#define SKS_CKR_PIN_LEN_RANGE			0x00000016
#define SKS_CKR_SESSION_EXISTS			0x00000017
#define SKS_CKR_SESSION_READ_ONLY		0x00000018
#define SKS_CKR_SESSION_READ_WRITE_SO_EXISTS	0x00000019
#define SKS_CKR_OPERATION_ACTIVE		0x0000001a
#define SKS_CKR_KEY_FUNCTION_NOT_PERMITTED	0x0000001b
#define SKS_CKR_OPERATION_NOT_INITIALIZED	0x0000001c
#define SKS_CKR_TOKEN_WRITE_PROTECTED		0x0000001d
#define SKS_CKR_TOKEN_NOT_PRESENT		0x0000001e
#define SKS_CKR_TOKEN_NOT_RECOGNIZED		0x0000001f
#define SKS_CKR_ACTION_PROHIBITED		0x00000020
#define SKS_CKR_ATTRIBUTE_READ_ONLY		0x00000021
#define SKS_CKR_PIN_TOO_WEAK			0x00000022
#define SKS_CKR_CURVE_NOT_SUPPORTED		0x00000023
#define SKS_CKR_DOMAIN_PARAMS_INVALID		0x00000024
#define SKS_CKR_USER_ALREADY_LOGGED_IN		0x00000025
#define SKS_CKR_USER_ANOTHER_ALREADY_LOGGED_IN	0x00000026
#define SKS_CKR_USER_NOT_LOGGED_IN		0x00000027
#define SKS_CKR_USER_PIN_NOT_INITIALIZED	0x00000028
#define SKS_CKR_USER_TOO_MANY_TYPES		0x00000029
#define SKS_CKR_USER_TYPE_INVALID		0x0000002a
#define SKS_CKR_SESSION_READ_ONLY_EXISTS	0x0000002b
#define SKS_CKR_KEY_SIZE_RANGE			0x0000002c
#define SKS_CKR_ATTRIBUTE_SENSITIVE		0x0000002d
#define SKS_CKR_SIGNATURE_LEN_RANGE		0x0000002e
#define SKS_CKR_KEY_TYPE_INCONSISTENT		0x0000002f
#define SKS_CKR_DATA_LEN_RANGE			0x00000030
#define SKS_CKR_ENCRYPTED_DATA_LEN_RANGE	0x00000031

/* Status without strict equivalence in Cryptoki API */
#define SKS_NOT_FOUND				0x00001000
#define SKS_NOT_IMPLEMENTED			0x00001001

/* Attribute specific values */
#define SKS_CK_UNAVAILABLE_INFORMATION		((uint32_t)0xFFFFFFFF)
#define SKS_UNDEFINED_ID			SKS_CK_UNAVAILABLE_INFORMATION
#define SKS_FALSE				0
#define SKS_TRUE				1

/*
 * Attribute identifiers
 * Valid values for struct sks_attribute_head::id
 *
 * SKS_ATTR_<x> corresponds to cryptoki CKA_<x>.
 * Value range [0 63] is reserved to boolean value attributes.
 */
#define SKS_BOOLPROPS_BASE			0x00000000
#define SKS_CKA_TOKEN				0x00000000
#define SKS_CKA_PRIVATE				0x00000001
#define SKS_CKA_TRUSTED				0x00000002
#define SKS_CKA_SENSITIVE			0x00000003
#define SKS_CKA_ENCRYPT				0x00000004
#define SKS_CKA_DECRYPT				0x00000005
#define SKS_CKA_WRAP				0x00000006
#define SKS_CKA_UNWRAP				0x00000007
#define SKS_CKA_SIGN				0x00000008
#define SKS_CKA_SIGN_RECOVER			0x00000009
#define SKS_CKA_VERIFY				0x0000000a
#define SKS_CKA_VERIFY_RECOVER			0x0000000b
#define SKS_CKA_DERIVE				0x0000000c
#define SKS_CKA_EXTRACTABLE			0x0000000d
#define SKS_CKA_LOCAL				0x0000000e
#define SKS_CKA_NEVER_EXTRACTABLE		0x0000000f
#define SKS_CKA_ALWAYS_SENSITIVE		0x00000010
#define SKS_CKA_MODIFIABLE			0x00000011
#define SKS_CKA_COPYABLE			0x00000012
#define SKS_CKA_DESTROYABLE			0x00000013
#define SKS_CKA_ALWAYS_AUTHENTICATE		0x00000014
#define SKS_CKA_WRAP_WITH_TRUSTED		0x00000015
/* Last boolean property ID (value is 63) is reserved */
#define SKS_BOOLPROPS_LAST			SKS_CKA_WRAP_WITH_TRUSTED
#define SKS_BOOLPROPS_END			0x0000003F
#define SKS_BOOLPROPH_FLAG			BIT(31)

#define SKS_CKA_LABEL				0x00000040
#define SKS_CKA_VALUE				0x00000041
#define SKS_CKA_VALUE_LEN			0x00000042
#define SKS_CKA_WRAP_TEMPLATE			0x00000043
#define SKS_CKA_UNWRAP_TEMPLATE			0x00000044
#define SKS_CKA_DERIVE_TEMPLATE			0x00000045
#define SKS_CKA_START_DATE			0x00000046
#define SKS_CKA_END_DATE			0x00000047
#define SKS_CKA_OBJECT_ID			0x00000048
#define SKS_CKA_APPLICATION			0x00000049
#define SKS_CKA_MECHANISM_TYPE			0x0000004a
#define SKS_CKA_ID				0x0000004b
#define SKS_CKA_ALLOWED_MECHANISMS		0x0000004c
#define SKS_CKA_CLASS				0x0000004d
#define SKS_CKA_KEY_TYPE			0x0000004e
#define SKS_CKA_EC_POINT			0x0000004f
#define SKS_CKA_EC_PARAMS			0x00000050
#define SKS_CKA_MODULUS				0x00000051
#define SKS_CKA_MODULUS_BITS			0x00000052
#define SKS_CKA_PUBLIC_EXPONENT			0x00000053
#define SKS_CKA_PRIVATE_EXPONENT		0x00000054
#define SKS_CKA_PRIME_1				0x00000055
#define SKS_CKA_PRIME_2				0x00000056
#define SKS_CKA_EXPONENT_1			0x00000057
#define SKS_CKA_EXPONENT_2			0x00000058
#define SKS_CKA_COEFFICIENT			0x00000059
#define SKS_CKA_SUBJECT				0x0000005a
#define SKS_CKA_PUBLIC_KEY_INFO			0x0000005b
#define SKS_CKA_CERTIFICATE_TYPE		0x0000005c
#define SKS_CKA_CERTIFICATE_CATEGORY		0x0000005d
#define SKS_CKA_ISSUER				0x0000005e
#define SKS_CKA_SERIAL_NUMBER			0x0000005f
#define SKS_CKA_URL				0x00000060
#define SKS_CKA_HASH_OF_SUBJECT_PUBLIC_KEY	0x00000061
#define SKS_CKA_HASH_OF_ISSUER_PUBLIC_KEY	0x00000062
#define SKS_CKA_NAME_HASH_ALGORITHM		0x00000063
#define SKS_CKA_KEY_GEN_MECHANISM		0x00000064

/*
 * Valid values for attribute SKS_CKA_CLASS
 * SKS_CKO_<x> corresponds to cryptoki CKO_<x>.
 */
#define SKS_CKO_SECRET_KEY			0x000
#define SKS_CKO_PUBLIC_KEY			0x001
#define SKS_CKO_PRIVATE_KEY			0x002
#define SKS_CKO_OTP_KEY				0x003
#define SKS_CKO_CERTIFICATE			0x004
#define SKS_CKO_DATA				0x005
#define SKS_CKO_DOMAIN_PARAMETERS		0x006
#define SKS_CKO_HW_FEATURE			0x007
#define SKS_CKO_MECHANISM			0x008

/*
 * Valid values for attribute SKS_CKA_KEY_TYPE
 * SKS_CKK_<x> corresponds to cryptoki CKK_<x> related to symmetric keys
 */
#define SKS_CKK_AES				0x000
#define SKS_CKK_GENERIC_SECRET			0x001
#define SKS_CKK_MD5_HMAC			0x002
#define SKS_CKK_SHA_1_HMAC			0x003
#define SKS_CKK_SHA224_HMAC			0x004
#define SKS_CKK_SHA256_HMAC			0x005
#define SKS_CKK_SHA384_HMAC			0x006
#define SKS_CKK_SHA512_HMAC			0x007
#define SKS_CKK_EC				0x008
#define SKS_CKK_RSA				0x009
#define SKS_CKK_DSA				0x00a
#define SKS_CKK_DH				0x00b
#define SKS_CKK_DES				0x00c
#define SKS_CKK_DES2				0x00d
#define SKS_CKK_DES3				0x00e

/*
 * Valid values for attribute SKS_CKA_CERTIFICATE_TYPE
 * SKS_CKC_<x> corresponds to cryptoki CKC_<x>.
 */
#define SKS_CKC_X_509			0x000
#define SKS_CKC_X_509_ATTR_CER		0x001
#define SKS_CKC_WTLS			0x002

/*
 * Valid values for attribute SKS_CKA_MECHANISM_TYPE
 * SKS_CKM_<x> corresponds to cryptoki CKM_<x>.
 */
#define SKS_CKM_AES_ECB				0x000
#define SKS_CKM_AES_CBC				0x001
#define SKS_CKM_AES_CBC_PAD			0x002
#define SKS_CKM_AES_CTS				0x003
#define SKS_CKM_AES_CTR				0x004
#define SKS_CKM_AES_GCM				0x005
#define SKS_CKM_AES_CCM				0x006
#define SKS_CKM_AES_GMAC			0x007
#define SKS_CKM_AES_CMAC			0x008
#define SKS_CKM_AES_CMAC_GENERAL		0x009
#define SKS_CKM_AES_ECB_ENCRYPT_DATA		0x00a
#define SKS_CKM_AES_CBC_ENCRYPT_DATA		0x00b
#define SKS_CKM_AES_KEY_GEN			0x00c
#define SKS_CKM_GENERIC_SECRET_KEY_GEN		0x00d
#define SKS_CKM_MD5_HMAC			0x00e
#define SKS_CKM_SHA_1_HMAC			0x00f
#define SKS_CKM_SHA224_HMAC			0x010
#define SKS_CKM_SHA256_HMAC			0x011
#define SKS_CKM_SHA384_HMAC			0x012
#define SKS_CKM_SHA512_HMAC			0x013
#define SKS_CKM_AES_XCBC_MAC			0x014
#define SKS_CKM_EC_KEY_PAIR_GEN			0x015
#define SKS_CKM_ECDSA				0x016
#define SKS_CKM_ECDSA_SHA1			0x017
#define SKS_CKM_ECDSA_SHA224			0x018	/* /!\ CK !PKCS#11 */
#define SKS_CKM_ECDSA_SHA256			0x019	/* /!\ CK !PKCS#11 */
#define SKS_CKM_ECDSA_SHA384			0x01a	/* /!\ CK !PKCS#11 */
#define SKS_CKM_ECDSA_SHA512			0x01b	/* /!\ CK !PKCS#11 */
#define SKS_CKM_ECDH1_DERIVE			0x01c
#define SKS_CKM_ECDH1_COFACTOR_DERIVE		0x01d
#define SKS_CKM_ECMQV_DERIVE			0x01e
#define SKS_CKM_ECDH_AES_KEY_WRAP		0x01f
#define SKS_CKM_RSA_PKCS_KEY_PAIR_GEN		0x020
#define SKS_CKM_RSA_PKCS			0x021
#define SKS_CKM_RSA_9796			0x022
#define SKS_CKM_RSA_X_509			0x023
#define SKS_CKM_SHA1_RSA_PKCS			0x024
#define SKS_CKM_RSA_PKCS_OAEP			0x025
#define SKS_CKM_SHA1_RSA_PKCS_PSS		0x026
#define SKS_CKM_SHA256_RSA_PKCS			0x027
#define SKS_CKM_SHA384_RSA_PKCS			0x028
#define SKS_CKM_SHA512_RSA_PKCS			0x029
#define SKS_CKM_SHA256_RSA_PKCS_PSS		0x02a
#define SKS_CKM_SHA384_RSA_PKCS_PSS		0x02b
#define SKS_CKM_SHA512_RSA_PKCS_PSS		0x02c
#define SKS_CKM_SHA224_RSA_PKCS			0x02d
#define SKS_CKM_SHA224_RSA_PKCS_PSS		0x02e
#define SKS_CKM_RSA_AES_KEY_WRAP		0x02f
#define SKS_CKM_RSA_PKCS_PSS			0x030
#define SKS_CKM_MD5				0x031
#define SKS_CKM_SHA_1				0x032
#define SKS_CKM_SHA224				0x033
#define SKS_CKM_SHA256				0x034
#define SKS_CKM_SHA384				0x035
#define SKS_CKM_SHA512				0x036
#define SKS_CKM_DH_PKCS_DERIVE			0x037
#define SKS_CKM_DES_KEY_GEN			0x038
#define SKS_CKM_DES_ECB			0x039
#define SKS_CKM_DES_CBC			0x040
#define SKS_CKM_DES_MAC			0x041
#define SKS_CKM_DES_MAC_GENERAL		0x042
#define SKS_CKM_DES_CBC_PAD			0x043

/* SKS added IDs for operation without cryptoki mechanism ID defined */
#define SKS_PROCESSING_IMPORT			0x1000
#define SKS_PROCESSING_COPY			0x1001

/*
 * Valid values key differentiation function identifiers
 * SKS_CKD_<x> reltaes to cryptoki CKD_<x>.
 */
#define SKS_CKD_NULL				0x0000UL
#define SKS_CKD_SHA1_KDF			0x0001UL
#define SKS_CKD_SHA1_KDF_ASN1			0x0002UL
#define SKS_CKD_SHA1_KDF_CONCATENATE		0x0003UL
#define SKS_CKD_SHA224_KDF			0x0004UL
#define SKS_CKD_SHA256_KDF			0x0005UL
#define SKS_CKD_SHA384_KDF			0x0006UL
#define SKS_CKD_SHA512_KDF			0x0007UL
#define SKS_CKD_CPDIVERSIFY_KDF			0x0008UL

/*
 * Valid values MG function identifiers
 * SKS_CKG_<x> reltaes to cryptoki CKG_<x>.
 */
#define SKS_CKG_MGF1_SHA1			0x0001UL
#define SKS_CKG_MGF1_SHA224			0x0005UL
#define SKS_CKG_MGF1_SHA256			0x0002UL
#define SKS_CKG_MGF1_SHA384			0x0003UL
#define SKS_CKG_MGF1_SHA512			0x0004UL

/*
 * Valid values for RSA PKCS/OAEP source type identifier
 * SKS_CKZ_<x> reltaes to cryptoki CKZ_<x>.
 */
#define SKS_CKZ_DATA_SPECIFIED			0x0001UL

/*
 * Processing parameters
 *
 * These can hardly be described by ANSI-C structures since the byte size of
 * some fields of the structure are specified by a previous field in the
 * structure. Therefore the format of the parameter binary data for each
 * supported processing is defined here from this comment rather than using
 * C structures.
 *
 * Processing parameters are used as argument the C_EncryptInit and friends
 * using the struct sks_attribute_head format where field 'type' is the SKS
 * processing ID and field 'size' is the parameter byte size. Below is shown
 * the head structure struct sks_attribute_head fields and the trailling data
 * that are the effective parameters binary blob for the target
 * processing/mechanism.
 *
 * AES ECB
 *   head:	32bit: type = SKS_CKM_AES_ECB
 *		32bit: params byte size = 0
 *
 * AES CBC, CBC_NOPAD and CTS
 *   head:	32bit: type = SKS_CKM_AES_CBC
 *			  or SKS_CKM_AES_CBC_PAD
 *			  or SKS_CKM_AES_CTS
 *		32bit: params byte size = 16
 *  params:	16byte: IV
 *
 * AES CTR, params relates to struct CK_AES_CTR_PARAMS.
 *   head:	32bit: type = SKS_CKM_AES_CTR
 *		32bit: params byte size = 20
 *  params:	32bit: counter bit increment
 *		16byte: IV
 *
 * AES GCM, params relates to struct CK_AES_GCM_PARAMS.
 *   head:	32bit: type = SKS_CKM_AES_GCM
 *		32bit: params byte size
 *  params:	32bit: IV_byte_size
 *		byte array: IV (IV_byte_size bytes)
 *		32bit: AAD_byte_size
 *		byte array: AAD data (AAD_byte_size bytes)
 *		32bit: tag bit size
 *
 * AES CCM, params relates to struct CK_AES_CCM_PARAMS.
 *   head:	32bit: type = SKS_CKM_AES_CCM
 *		32bit: params byte size
 *  params:	32bit: data_byte_size
 *		32bit: nonce_byte_size
 *		byte array: nonce data (nonce_byte_size bytes)
 *		32bit: AAD_byte_size
 *		byte array: AAD data (AAD_byte_size bytes)
 *		32bit: MAC byte size
 *
 * AES GMAC
 *   head:	32bit: type = SKS_CKM_AES_GMAC
 *		32bit: params byte size = 12
 *  params:	12byte: IV
 *
 * AES CMAC with general length, params relates to struct CK_MAC_GENERAL_PARAMS.
 *   head:	32bit: type = SKS_CKM_AES_CMAC_GENERAL
 *		32bit: params byte size = 12
 *  params:	32bit: byte size of the output CMAC data
 *
 * AES CMAC fixed size (16byte CMAC)
 *   head:	32bit: type = SKS_CKM_AES_CMAC_GENERAL
 *		32bit: size = 0
 *
 * AES derive by ECB, params relates to struct CK_KEY_DERIVATION_STRING_DATA.
 *   head:	32bit: type = SKS_CKM_AES_ECB_ENCRYPT_DATA
 *		32bit: params byte size
 *  params:	32bit: byte size of the data to encrypt
 *		byte array: data to encrypt
 *
 * AES derive by CBC, params relates to struct CK_AES_CBC_ENCRYPT_DATA_PARAMS.
 *   head:	32bit: type = SKS_CKM_AES_CBC_ENCRYPT_DATA
 *		32bit: params byte size
 *  params:	16byte: IV
 *		32bit: byte size of the data to encrypt
 *		byte array: data to encrypt
 *
 * AES and generic secret generation
 *   head:	32bit: type = SKS_CKM_AES_KEY_GEN
 *			   or SKS_CKM_GENERIC_SECRET_KEY_GEN
 *		32bit: size = 0
 *
 * ECDH, params relates to struct CK_ECDH1_DERIVE_PARAMS.
 *   head:	32bit: type = SKS_CKM_ECDH1_DERIVE
 *			   or SKS_CKM_ECDH1_COFACTOR_DERIVE
 *		32bit: params byte size
 *  params:	32bit: key derivation function (SKS_CKD_xxx)
 *		32bit: byte size of the shared data
 *		byte array: shared data
 *		32bit: byte: size of the public data
 *		byte array: public data
 *
 * AES key wrap by ECDH, params relates to struct CK_ECDH_AES_KEY_WRAP_PARAMS.
 *   head:	32bit: type = SKS_CKM_ECDH_AES_KEY_WRAP
 *		32bit: params byte size
 *  params:	32bit: bit size of the AES key
 *		32bit: key derivation function (SKS_CKD_xxx)
 *		32bit: byte size of the shared data
 *		byte array: shared data
 *
 * RSA_PKCS (pre-hashed payload)
 *   head:	32bit: type = SKS_CKM_RSA_PKCS
 *		32bit: size = 0
 *
 * RSA PKCS OAEP, params relates to struct CK_RSA_PKCS_OAEP_PARAMS.
 *   head:	32bit: type = SKS_CKM_RSA_PKCS_OAEP
 *		32bit: params byte size
 *  params:	32bit: hash algorithm identifier (SKS_CK_M_xxx)
 *		32bit: CK_RSA_PKCS_MGF_TYPE
 *		32bit: CK_RSA_PKCS_OAEP_SOURCE_TYPE
 *		32bit: byte size of the source data
 *		byte array: source data
 *
 * RSA PKCS PSS, params relates to struct CK_RSA_PKCS_PSS_PARAMS.
 *   head:	32bit: type = SKS_CKM_RSA_PKCS_PSS
 *			   or SKS_CKM_SHA256_RSA_PKCS_PSS
 *			   or SKS_CKM_SHA384_RSA_PKCS_PSS
 *			   or SKS_CKM_SHA512_RSA_PKCS_PSS
 *		32bit: params byte size
 *  params:	32bit: hash algorithm identifier (SKS_CK_M_xxx)
 *		32bit: CK_RSA_PKCS_MGF_TYPE
 *		32bit: byte size of the salt in the PSS encoding
 *
 * AES key wrapping by RSA, params relates to struct CK_RSA_AES_KEY_WRAP_PARAMS.
 *   head:	32bit: type = CKM_RSA_AES_KEY_WRAP
 *		32bit: params byte size
 *  params:	32bit: bit size of the AES key
 *		32bit: hash algorithm identifier (SKS_CK_M_xxx)
 *		32bit: CK_RSA_PKCS_MGF_TYPE
 *		32bit: CK_RSA_PKCS_OAEP_SOURCE_TYPE
 *		32bit: byte size of the source data
 *		byte array: source data
 */

#endif /*__SKS_TA_H__*/

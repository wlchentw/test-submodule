/*
 * Copyright (c) 2018, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <compiler.h>
#include <util.h>
#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>

#include "pkcs11_token.h"
#include "processing.h"
#include "serializer.h"
#include "sks_helpers.h"

uint32_t tee_init_ctr_operation(struct active_processing *processing,
				    void *proc_params, size_t params_size)
{
	struct serialargs args;
	uint32_t rv = 0;
	/* CTR parameters */
	uint32_t incr_counter = 0;
	void *counter_bits = NULL;

	TEE_MemFill(&args, 0, sizeof(args));

	if (!proc_params)
		return SKS_BAD_PARAM;

	serialargs_init(&args, proc_params, params_size);

	rv = serialargs_get(&args, &incr_counter, sizeof(uint32_t));
	if (rv)
		goto bail;

	rv = serialargs_get_ptr(&args, &counter_bits, 16);
	if (rv)
		goto bail;

	if (incr_counter != 1) {
		DMSG("Supports only 1 bit increment counter: %d",
						incr_counter);
		rv = SKS_CKR_MECHANISM_PARAM_INVALID;
		goto bail;
	}

	TEE_CipherInit(processing->tee_op_handle, counter_bits, 16);

	rv = SKS_OK;

bail:
	return rv;
}

void tee_release_ctr_operation(struct active_processing *processing __unused)
{
}

/*
 * Authenticated ciphering: (CCM / GCM)
 *
 * As per PKCS#11, CCM/GCM decrypt shall not revealed the data until the
 * decryption is completed and the mac verified. The SKS TA must retain the
 * ciphered data until the CCM finalization. To do so, arrays of decrypted
 * data are allocated during AE update processing and copied into client
 * buffer at AE finalization.
 *
 * As per PKCS#11, CCM/GCM decrypt expect the tag/mac data to be provided
 * inside the input data for DecryptUpdate() and friends. But the DecryptFinal
 * API does not provide input data reference hence we do not know which is the
 * last call to DecryptUpdate() where last bytes are not ciphered data but the
 * requested tag/mac byte. To handle this, the TA saves the last input data
 * bytes (length is defined by the tag byte size) in the AE context and
 * waits the DecryptFinal() to either treat these as data bytes or tag/mac
 * bytes. Refer to pending_tag and pending_size in struct ae_aes_context.
 */

/*
 * @size - byte size of the allocated buffer
 * @data - pointer to allocated data
 */
struct out_data_ref {
	size_t size;
	void *data;
};

/*
 * @tag_byte_len - tag size in byte
 * @pending_tag - Input data that could be the appended tag
 * @pending_size - Size of pending input data that could be the tag
 * @out_data - Pointer to an array of output data references.
 * @out_count - Number of buffer references in out_data
 */
struct ae_aes_context {
	size_t tag_byte_len;
	char *pending_tag;
	size_t pending_size;
	struct out_data_ref *out_data;
	size_t out_count;
};

static void release_ae_aes_context(struct ae_aes_context *ctx)
{
	size_t n = 0;

	for (n = 0; n < ctx->out_count; n++) {
		TEE_Free(ctx->out_data[n].data);
	}

	TEE_Free(ctx->out_data);
	ctx->out_data = NULL;
	ctx->out_count = 0;

	TEE_Free(ctx->pending_tag);
	ctx->pending_tag = NULL;
}

uint32_t tee_ae_decrypt_update(struct active_processing *processing,
			       void *in, size_t in_size)
{
	struct ae_aes_context *ctx = processing->extra_ctx;
	size_t data_len = 0;
	uint32_t size = 0;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t rv = 0;
	char *ct = NULL;
	uint32_t ct_size = 0;
	void *ptr = NULL;

	if (!in_size)
		return SKS_OK;

	if (!in)
		return SKS_BAD_PARAM;

	/*
	 * Save the last input bytes in case they are the tag
	 * bytes and not ciphered data bytes to be decrypted.
	 */

	if (ctx->pending_size + in_size <= ctx->tag_byte_len) {
		/*
		 * Data bytes are all potential tag bytes.
		 * We only need to update the pending_tag buffer,
		 * and cannot treat any byte as data byte.
		 */
		TEE_MemMove(ctx->pending_tag + ctx->pending_size, in, in_size);

		ctx->pending_size += in_size;

		return SKS_OK;
	}

	/* Size of data that are not potential tag in pending and input data */
	data_len = in_size + ctx->pending_size - ctx->tag_byte_len;

	if (ctx->pending_size &&
	    (ctx->pending_size + in_size) >= ctx->tag_byte_len) {
		/* Process pending tag bytes that are effective data byte */
		uint32_t len = MIN(data_len, ctx->pending_size);

		res = TEE_AEUpdate(processing->tee_op_handle,
				   ctx->pending_tag, len, NULL, &ct_size);

		// TODO: explain this
		if (res != TEE_ERROR_SHORT_BUFFER &&
		    (res != TEE_SUCCESS || ct_size)) {
			rv = SKS_ERROR;
			goto bail;
		}

		/*
		 * If output data to store (not revealed yet), redo with
		 * an allocated temporary reference.
		 */
		if (ct_size) {
			ct = TEE_Malloc(ct_size, TEE_USER_MEM_HINT_NO_FILL_ZERO);
			if (!ct) {
				rv = SKS_MEMORY;
				goto bail;
			}

			res = TEE_AEUpdate(processing->tee_op_handle,
					   ctx->pending_tag, len, ct, &ct_size);
			if (res) {
				rv = tee2sks_error(res);
				goto bail;
			}

			/* Finally, no out data? Release temp buffer. */
			if (!ct_size) {
				TEE_Free(ct);
				ct = NULL;
				DMSG_RAW("\nWe expected some data!\n\n");
			}
		}

		/* Save potential tag bytes for later */
		TEE_MemMove(ctx->pending_tag, ctx->pending_tag + len,
			    ctx->pending_size - len);

		ctx->pending_size -= len;
		data_len -= len;
	}

	if (data_len) {
		/* Process input data that are not potential tag bytes */
		size = 0;
		res = TEE_AEUpdate(processing->tee_op_handle,
				   in, data_len, NULL, &size);

		if (res != TEE_ERROR_SHORT_BUFFER &&
		    (res != TEE_SUCCESS || size)) {
			rv = SKS_ERROR;
			goto bail;
		}

		if (size) {
			ptr = TEE_Realloc(ct, ct_size + size);
			if (!ptr) {
				rv = SKS_MEMORY;
				goto bail;
			}
			ct = ptr;

			res = TEE_AEUpdate(processing->tee_op_handle,
					   in, data_len, ct + ct_size, &size);
			if (res) {
				rv = tee2sks_error(res);
				goto bail;
			}

			ct_size += size;
		}
	}

	/* Update pending tag in context if any */
	data_len = in_size - data_len;
	if (data_len > (ctx->tag_byte_len - ctx->pending_size)) {
		/* This could be asserted */
		rv = SKS_ERROR;
		goto bail;
	}

	if (data_len) {
		TEE_MemMove(ctx->pending_tag + ctx->pending_size,
			    (char *)in + in_size - data_len, data_len);

		ctx->pending_size += data_len;
	}

	/* Save output data reference in the context */
	if (ct_size) {
		ptr = TEE_Realloc(ctx->out_data, (ctx->out_count + 1) *
				  sizeof(struct out_data_ref));
		if (!ptr) {
			rv = SKS_MEMORY;
			goto bail;
		}
		ctx->out_data = ptr;
		ctx->out_data[ctx->out_count].size = ct_size;
		ctx->out_data[ctx->out_count].data = ct;
		ctx->out_count++;
	}

	rv = SKS_OK;

bail:
	if (rv) {
		TEE_Free(ct);
	}

	return rv;
}

static uint32_t reveale_ae_data(struct ae_aes_context *ctx,
				void *out, uint32_t *out_size)
{
	size_t n = 0;
	uint32_t req_size = 0;
	char *out_ptr = out;

	for (req_size = 0, n = 0; n < ctx->out_count; n++)
		req_size += ctx->out_data[n].size;

	if (*out_size < req_size) {
		*out_size = req_size;
		return SKS_SHORT_BUFFER;
	}

	if (!out_ptr)
		return SKS_BAD_PARAM;

	for (n = 0; n < ctx->out_count; n++) {
		TEE_MemMove(out_ptr,
			    ctx->out_data[n].data, ctx->out_data[n].size);

		TEE_Free(ctx->out_data[n].data);
		out_ptr += ctx->out_data[n].size;
	}

	TEE_Free(ctx->out_data);
	ctx->out_data = NULL;
	ctx->out_count = 0;

	*out_size = req_size;

	return SKS_OK;
}

uint32_t tee_ae_decrypt_final(struct active_processing *processing,
			      void *out, uint32_t *out_size)
{
	struct ae_aes_context *ctx = processing->extra_ctx;
	uint32_t rv = 0;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t data_size = 0;
	void *data_ptr = NULL;

	if (!out_size) {
		DMSG("Expect at least a buffer for the output data");
		return SKS_BAD_PARAM;
	}

	/* Final is already completed, only need to output the data */
	if (!ctx->pending_tag)
		return reveale_ae_data(ctx, out, out_size);

	if (ctx->pending_size != ctx->tag_byte_len) {
		DMSG("Not enough samples: %zu/%zu",
			ctx->pending_size, ctx->tag_byte_len);
		return SKS_FAILED;	// FIXME: CKR_ENCRYPTED_DATA_LEN_RANGE
	}

	data_size = 0;
	res = TEE_AEDecryptFinal(processing->tee_op_handle,
				 NULL, 0, NULL, &data_size,
				 ctx->pending_tag, ctx->tag_byte_len);

	if (res == TEE_ERROR_SHORT_BUFFER) {
		data_ptr = TEE_Malloc(data_size,
				      TEE_USER_MEM_HINT_NO_FILL_ZERO);
		if (!data_ptr) {
			rv = SKS_MEMORY;
			goto bail;
		}

		res = TEE_AEDecryptFinal(processing->tee_op_handle,
					 NULL, 0, data_ptr, &data_size,
					 ctx->pending_tag, ctx->tag_byte_len);

		if (!data_size) {
			TEE_Free(data_ptr);
			data_ptr = NULL;
			DMSG_RAW("\nIs this expected from the Core API?\n\n");
		}
	}

	/* AE decryption is completed */
	TEE_Free(ctx->pending_tag);
	ctx->pending_tag = NULL;

	rv = tee2sks_error(res);
	if (rv)
		goto bail;

	if (data_ptr) {
		void *tmp_ptr = NULL;

		tmp_ptr = TEE_Realloc(ctx->out_data,
					(ctx->out_count + 1) *
					sizeof(struct out_data_ref));
		if (!tmp_ptr) {
			rv = SKS_MEMORY;
			goto bail;
		}
		ctx->out_data = tmp_ptr;
		ctx->out_data[ctx->out_count].size = data_size;
		ctx->out_data[ctx->out_count].data = data_ptr;
		ctx->out_count++;

		data_ptr = NULL;
	}

	rv = reveale_ae_data(ctx, out, out_size);

bail:
	TEE_Free(data_ptr);

	return rv;
}

uint32_t tee_ae_encrypt_final(struct active_processing *processing,
			      void *out, uint32_t *out_size)
{
	struct ae_aes_context *ctx = processing->extra_ctx;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint8_t *tag = NULL;
	uint32_t tag_len = 0;
	uint32_t size = 0;

	if (!out || !out_size)
		return SKS_BAD_PARAM;

	/* Check the required sizes (warning: 2 output len: data + tag) */
	res = TEE_AEEncryptFinal(processing->tee_op_handle,
				 NULL, 0, NULL, &size,
				 &tag, &tag_len);

	if (tag_len != ctx->tag_byte_len ||
	    (res != TEE_SUCCESS && res != TEE_ERROR_SHORT_BUFFER)) {
		EMSG("Unexpected tag length %u/%zu or rc 0x%" PRIx32,
			tag_len, ctx->tag_byte_len, res);
		return SKS_ERROR;
	}

	if (*out_size < size + tag_len) {
		*out_size = size + tag_len;
		return SKS_SHORT_BUFFER;
	}

	/* Process data and tag input the client output buffer */
	tag = (uint8_t *)out + size;

	res = TEE_AEEncryptFinal(processing->tee_op_handle,
				 NULL, 0, out, &size, tag, &tag_len);

	if (tag_len != ctx->tag_byte_len) {
		EMSG("Unexpected tag length");
		return SKS_ERROR;
	}

	if (!res)
		*out_size = size + tag_len;

	return tee2sks_error(res);
}

uint32_t tee_init_ccm_operation(struct active_processing *processing,
				void *proc_params, size_t params_size)
{
	uint32_t rv = 0;
	struct ae_aes_context *params = NULL;
	struct serialargs args;
	/* CCM parameters */
	uint32_t data_len = 0;
	uint32_t nonce_len = 0;
	void *nonce = NULL;
	uint32_t aad_len = 0;
	void *aad = NULL;
	uint32_t mac_len = 0;

	TEE_MemFill(&args, 0, sizeof(args));

	if (!proc_params)
		return SKS_BAD_PARAM;

	serialargs_init(&args, proc_params, params_size);

	rv = serialargs_get(&args, &data_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	rv = serialargs_get(&args, &nonce_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	// TODO: no need to copy nonce into secure world
	rv = serialargs_alloc_and_get(&args, &nonce, nonce_len);
	if (rv)
		goto bail;

	rv = serialargs_get(&args, &aad_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	// TODO: no need to copy aad into secure world
	rv = serialargs_alloc_and_get(&args, &aad, aad_len);
	if (rv)
		goto bail;

	rv = serialargs_get(&args, &mac_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	/* As per pkcs#11 mechanism specification */
	if (data_len > 28 ||
	    !nonce_len || nonce_len > 15 ||
	    aad_len > 256 ||
	    mac_len < 4 || mac_len > 16 || mac_len & 1) {
		DMSG("Invalid parameters: data_len %" PRIu32
			", nonce_len %" PRIu32 ", aad_len %" PRIu32
			", mac_len %" PRIu32, data_len, nonce_len,
			aad_len, mac_len);
		rv = SKS_CKR_MECHANISM_PARAM_INVALID;
		goto bail;
	}

	params = TEE_Malloc(sizeof(struct ae_aes_context),
			    TEE_USER_MEM_HINT_NO_FILL_ZERO);
	if (!params) {
		rv = SKS_MEMORY;
		goto bail;
	}

	params->tag_byte_len = mac_len;
	params->out_count = 0;
	params->pending_size = 0;
	params->out_data = TEE_Malloc(sizeof(struct out_data_ref),
				      TEE_MALLOC_FILL_ZERO);
	params->pending_tag = TEE_Malloc(mac_len,
					 TEE_USER_MEM_HINT_NO_FILL_ZERO);
	if (!params->out_data || !params->pending_tag) {
		rv = SKS_MEMORY;
		goto bail;
	}

	TEE_AEInit(processing->tee_op_handle, nonce, nonce_len, mac_len * 8,
					   aad_len, data_len);
	if (aad_len)
		TEE_AEUpdateAAD(processing->tee_op_handle, aad, aad_len);

	/* Session processing owns the active processing params */
	assert(!processing->extra_ctx);
	processing->extra_ctx = params;

	rv = SKS_OK;

bail:
	TEE_Free(nonce);
	TEE_Free(aad);
	if (rv && params) {
		TEE_Free(params->out_data);
		TEE_Free(params->pending_tag);
		TEE_Free(params);
	}
	return rv;
}

void tee_release_ccm_operation(struct active_processing *processing)
{
	struct ae_aes_context *ctx = processing->extra_ctx;

	release_ae_aes_context(ctx);
	TEE_Free(processing->extra_ctx);
	processing->extra_ctx = NULL;
}

/*
 * GCM
 */
uint32_t tee_init_gcm_operation(struct active_processing *processing,
				    void *proc_params, size_t params_size)
{
	struct serialargs args;
	uint32_t rv = 0;
	uint32_t tag_len = 0;
	struct ae_aes_context *params = NULL;
	/* GCM parameters */
	uint32_t iv_len = 0;
	void *iv = NULL;
	uint32_t aad_len = 0;
	void *aad = NULL;
	uint32_t tag_bitlen = 0;

	TEE_MemFill(&args, 0, sizeof(args));

	if (!proc_params)
		return SKS_BAD_PARAM;

	serialargs_init(&args, proc_params, params_size);

	rv = serialargs_get(&args, &iv_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	// TODO: no need to copy iv into secure world
	rv = serialargs_alloc_and_get(&args, &iv, iv_len);
	if (rv)
		goto bail;

	rv = serialargs_get(&args, &aad_len, sizeof(uint32_t));
	if (rv)
		goto bail;

	// TODO: no need to copy aad into secure world
	rv = serialargs_alloc_and_get(&args, &aad, aad_len);
	if (rv)
		goto bail;

	rv = serialargs_get(&args, &tag_bitlen, sizeof(uint32_t));
	if (rv)
		goto bail;

	tag_len = ROUNDUP(tag_bitlen, 8) / 8;

	/* As per pkcs#11 mechanism specification */
	if (tag_bitlen > 128 ||
	    !iv_len || iv_len > 256) {
		DMSG("Invalid parameters: tag_bit_len %" PRIu32
			", iv_len %" PRIu32, tag_bitlen, iv_len);
		rv = SKS_CKR_MECHANISM_PARAM_INVALID;
		goto bail;
	}

	params = TEE_Malloc(sizeof(struct ae_aes_context),
			    TEE_USER_MEM_HINT_NO_FILL_ZERO);
	if (!params) {
		rv = SKS_MEMORY;
		goto bail;
	}

	/* Store the byte round up byte length for the tag */
	params->tag_byte_len = tag_len;
	params->out_count = 0;
	params->pending_size = 0;
	params->out_data = TEE_Malloc(sizeof(struct out_data_ref),
				      TEE_MALLOC_FILL_ZERO);
	params->pending_tag = TEE_Malloc(tag_len,
					 TEE_USER_MEM_HINT_NO_FILL_ZERO);

	if (!params->out_data || !params->pending_tag) {
		rv = SKS_MEMORY;
		goto bail;
	}

	/* Session processing owns the active processing params */
	assert(!processing->extra_ctx);
	processing->extra_ctx = params;

	TEE_AEInit(processing->tee_op_handle, iv, iv_len, tag_bitlen, 0, 0);

	if (aad_len)
		TEE_AEUpdateAAD(processing->tee_op_handle, aad, aad_len);

	rv = SKS_OK;

bail:
	TEE_Free(iv);
	TEE_Free(aad);
	if (rv && params) {
		TEE_Free(params->out_data);
		TEE_Free(params->pending_tag);
		TEE_Free(params);
	}

	return rv;
}

void tee_release_gcm_operation(struct active_processing *processing)
{
	struct ae_aes_context *ctx = processing->extra_ctx;

	release_ae_aes_context(ctx);
	TEE_Free(processing->extra_ctx);
	processing->extra_ctx = NULL;
}

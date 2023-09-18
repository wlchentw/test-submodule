/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <asm/byteorder.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include "nfsb.h"
#include "rsa.h"

static const char MAGIC[] = { 'N', 'F', 'S', 'B' };

static const ssize_t BYTES_PER_SECTOR = 512;
static const int SHA256_SIZE = 32;

unsigned char nfsb_hash_type(const struct nfsb_header *h)
{
	return h->hash_type;
}

ssize_t nfsb_fs_offset(const struct nfsb_header *h)
{
	return ntohl(h->fs_offset) * BYTES_PER_SECTOR;
}

ssize_t nfsb_fs_size(const struct nfsb_header *h)
{
	return ntohl(h->fs_size) * BYTES_PER_SECTOR;
}

ssize_t nfsb_hash_size(const struct nfsb_header *h)
{
	return ntohl(h->hash_size) * BYTES_PER_SECTOR;
}

unsigned int nfsb_data_blockcount(const struct nfsb_header *h)
{
	return ntohl(h->data_blocks);
}

unsigned int nfsb_data_blocksize(const struct nfsb_header *h)
{
	return ntohl(h->data_blocksize);
}

const char *nfsb_hash_algo(const struct nfsb_header *h)
{
	return h->hash_algo;
}

unsigned int nfsb_hash_blocksize(const struct nfsb_header *h)
{
	return ntohl(h->hash_blocksize);
}

const char *nfsb_verity_salt(const struct nfsb_header *h)
{
	return h->verity_salt;
}

const char *nfsb_verity_hash(const struct nfsb_header *h)
{
	return h->verity_hash;
}

int nfsb_verify(const struct nfsb_header *h, const unsigned char *key, const unsigned char *n)
{
	unsigned char *digest = NULL;
	unsigned char *msg = NULL;
	int ret = -1;

	/* SHA-256 the header, minus the signature memory. */

	digest = malloc(SHA256_SIZE);
	if (!digest)
		goto failure;

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, h, sizeof(struct nfsb_header) - sizeof(h->rsa_sig));
	SHA256_Final(digest, &sha256);

	/* Reconstruct the SHA-256 from the header RSA-2048 signature. */
	msg = rsa_encryptdecrypt(h->rsa_sig, key, n);
	if (!msg) {
		printf("rsa_encryptdecrypt failure\n");
		goto failure;
	}

	if (!memcmp(&msg[RSA_LEN - SHA256_LEN], digest, SHA256_LEN))
		ret = 0;
	else
		ret = -1;

failure:
	free(msg);
	free(digest);
	return ret;
}

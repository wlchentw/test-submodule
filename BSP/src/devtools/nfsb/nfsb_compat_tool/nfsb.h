/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#ifndef _NFSB_H
#define _NFSB_H

#include <stdint.h>

/**
 * A Netflix Secure Boot Header (NFSB) contains the dm-verity
 * information for a file system image. The header contents can be
 * used to establish trust of the file system data and is signed by an
 * RSA-2048 private key trusted by the kernel.
 */
#pragma pack(push)
#pragma pack(1)
struct nfsb_header {
	char magic[4];
	uint8_t version;
	uint8_t hash_type;	/* verity hash type */
	uint32_t fs_offset;	/* file system image offset in 512B sectors (big-endian) */
	uint32_t fs_size;	/* file system image size in 512B sectors (big-endian) */
	uint32_t hash_size;	/* hash image size in 512B sectors (big-endian) */
	uint32_t data_blocks;	/* file system image block count (big-endian) */
	uint32_t data_blocksize;	/* file system image block size in bytes (big-endian) */
	char hash_algo[16];	/* verity hash algorithm */
	uint32_t hash_blocksize;	/* hash image block size in bytes (big-endian) */
	char verity_salt[65];	/* SHA-256 salt in hexadecimal + NULL */
	char verity_hash[65];	/* SHA-256 hash in hexadecimal + NULL */
	uint8_t rsa_sig[256];	/* RSA-2048/1024 signature */
};
#pragma pack(pop)

#endif

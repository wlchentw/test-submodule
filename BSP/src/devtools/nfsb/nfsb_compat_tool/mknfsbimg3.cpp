#include <inttypes.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <endian.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include "nfsb.h"

uint32_t getfilesize(char* path)
{
	int result;
	struct stat buf;
	result = stat(path,&buf);
	if(result == 0)
		return buf.st_size;
	else
		return 0;
}

static const char MAGIC[] = {'N','F','S','B'};

#define PAGE_SIZE (4096)
#define BYTES_SECTOR (512)
#define SHA256_LEN (32)
#define RSA_LEN (256)

static ssize_t nearest_page(ssize_t sz)
{
	return ((sz + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
}

void sha256(char* buffer, int size, char* hash)
{
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, buffer, size);
	SHA256_Final((unsigned char*)hash,&sha256);
}

int nfsb_header_sign(struct nfsb_header *h, const char *key, const char *n)
{
	char hash[RSA_LEN];

	memset(hash, 0, RSA_LEN);
	sha256((char*)h, sizeof(struct nfsb_header) - sizeof(h->rsa_sig), &hash[RSA_LEN - SHA256_LEN] );

	RSA* rsa = RSA_new();

	char e[RSA_LEN] = {0};
	e[RSA_LEN - 3 ] = 0x01;
	e[RSA_LEN - 1 ] = 0x01;

	BIGNUM* bn_n = BN_bin2bn((const unsigned char*)n, RSA_LEN, NULL);
	BIGNUM* bn_e = BN_bin2bn((const unsigned char*)e, RSA_LEN, NULL);
	BIGNUM* bn_d = BN_bin2bn((const unsigned char*)key,RSA_LEN, NULL);

	rsa->n = bn_n;
	rsa->e = bn_e;
	rsa->d = bn_d;

	if (RSA_private_encrypt(RSA_LEN, (const unsigned char*)hash, h->rsa_sig, rsa, RSA_NO_PADDING) == -1) {
		printf("RSA_private_encrypt fail\n");
		exit(1);
	}
	
	return 0;
}

int write_content_with_padding(int fd, char* content, int size, int padding_size)
{
	int bytes_write = write(fd, content, size);
	if (bytes_write != size) {
		printf("write content fail\n");
		exit(1);
	}

	if (size % padding_size != 0) {
		int padding_need_size = padding_size - (size % padding_size);
		char *padding_content = (char *)malloc(padding_need_size);
		memset(padding_content, 0, padding_need_size);
		bytes_write = write(fd, padding_content, padding_need_size);
		if (bytes_write != padding_need_size) {
			printf("write content padding fail\n");
			exit(1);
		}
		free(padding_content);
	}

	return 0;
}

int main(int argc, char **argv)
{
	if(argc != 7) {
		printf("Invaild input\n");
		printf("%s fs_image hash_image nfsb_image table_file modulus_file privkey_file\n", argv[0]);
		exit(1);
	}

	char* fs_image_path = argv[1];
	char* hash_image_path = argv[2];
	char* nfsb_image_path = argv[3];
	char* table_file_path = argv[4];
	char* modulus_file_path = argv[5];
	char* privkey_file_path = argv[6];

	uint32_t fs_image_size = getfilesize(fs_image_path);
	if (fs_image_size == 0) {
		printf("Error: read %s error!\n", fs_image_path);
		exit(1);
	}

	uint32_t hash_image_size = getfilesize(hash_image_path);
	if (fs_image_size == 0) {
		printf("Error: read %s error!\n", fs_image_path);
		exit(1);
	}

	uint32_t table_file_size = getfilesize(table_file_path);
	if (table_file_size == 0) {
		printf("Error: read %s error!\n", table_file_path);
		exit(1);
	}

	uint32_t modulus_file_size = getfilesize(modulus_file_path);
	if (modulus_file_size == 0) {
		printf("Error: read %s error!\n", modulus_file_path);
		exit(1);
	}

	uint32_t privkey_file_size = getfilesize(privkey_file_path);
	if (privkey_file_size == 0) {
		printf("Error: read %s error!\n", privkey_file_path);
		exit(1);
	}

	/* Construct and sign nfsb header */
	struct nfsb_header *header = (struct nfsb_header *)malloc(sizeof(struct nfsb_header));
	memset(header, 0, sizeof(struct nfsb_header));

	int rootfs_table_fd = open(table_file_path, O_RDONLY);
	char* rootfs_table_content = (char *)malloc(table_file_size);
	memset(rootfs_table_content, 0, table_file_size);
	int bytes_read = read(rootfs_table_fd, rootfs_table_content, table_file_size);
	if (bytes_read != table_file_size) {
		printf("Error: read %s error!\n", table_file_path);
		exit(1);
	}
	close(rootfs_table_fd);

	uint8_t hash_type = 0;
	uint32_t data_blockcount = 0, data_blocksize = 0, hash_blocksize = 0;
	char *hash_algo = NULL, *verity_salt = NULL, *verity_hash = NULL;
	int i = 0;

	i = sscanf(rootfs_table_content, "%*[^:]: "
			   "%*[^:]: %" SCNu8 " "
			   "%*[^:]: %u "
			   "%*[^:]: %u "
			   "%*[^:]: %u "
			   "%*[^:]: %ms "
			   "%*[^:]: %ms "
			   "%*[^:]: %ms ",
			   &hash_type, &data_blockcount, &data_blocksize, &hash_blocksize,
			   &hash_algo, &verity_salt, &verity_hash);

	if (i != 7) {
		printf("Error: parse %s error!\n", table_file_path);
		exit(1);
	}

	memcpy(header->magic, "NFSB", strlen("NFSB"));
	header->version = 3;
	header->hash_type = hash_type;
	header->fs_offset = htonl(nearest_page(sizeof(struct nfsb_header)) / BYTES_SECTOR);
	header->fs_size = htonl(nearest_page(fs_image_size) / BYTES_SECTOR);
	header->hash_size = nearest_page(hash_image_size) / BYTES_SECTOR;
	header->data_blocks = htonl(data_blockcount);
	header->data_blocksize = htonl(data_blocksize);
	header->hash_blocksize = htonl(hash_blocksize);
	strncpy(header->hash_algo, hash_algo, sizeof(header->hash_algo));
	strncpy(header->verity_salt, verity_salt, sizeof(header->verity_salt));
	strncpy(header->verity_hash, verity_hash, sizeof(header->verity_hash));

	int modulus_file_fd = open(modulus_file_path, O_RDONLY);
	char* modulus_file_content = (char *)malloc(modulus_file_size);
	memset(modulus_file_content, 0, modulus_file_size);
	bytes_read = read(modulus_file_fd, modulus_file_content, modulus_file_size);
	if (bytes_read != modulus_file_size) {
		printf("Error: read %s error!\n", modulus_file_path);
		exit(1);
	}
	close(modulus_file_fd);

	int privkey_file_fd = open(privkey_file_path, O_RDONLY);
	char* privkey_file_content = (char *)malloc(privkey_file_size);
	memset(privkey_file_content, 0, privkey_file_size);
	bytes_read = read(privkey_file_fd, privkey_file_content, privkey_file_size);
	if (bytes_read != privkey_file_size) {
		printf("Error: read %s error!\n", privkey_file_path);
		exit(1);
	}
	close(privkey_file_fd);

	if(nfsb_header_sign(header, privkey_file_content, modulus_file_content)) {
		printf("Error: sign nfsb header error!\n");
		exit(1);
	}

	int nfsb_image_fd = open(nfsb_image_path, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	printf("Writing nfsb header...\n");
	write_content_with_padding(nfsb_image_fd, (char*)header,
		sizeof(struct nfsb_header), ntohl(header->fs_offset) * BYTES_SECTOR);

	printf("Writing fs image...\n");
	int fs_image_fd = open(fs_image_path, O_RDONLY);
	char* fs_image_content = (char *)malloc(fs_image_size);
	memset(fs_image_content, 0, fs_image_size);
	bytes_read = read(fs_image_fd, fs_image_content, fs_image_size);
	if (bytes_read != fs_image_size) {
		printf("Error: read %s error!\n", fs_image_path);
		exit(1);
	}
	close(fs_image_fd);

	write_content_with_padding(nfsb_image_fd, fs_image_content,
		fs_image_size, ntohl(header->fs_size) * BYTES_SECTOR);

	printf("Writing hash image...\n");
	int hash_image_fd = open(hash_image_path, O_RDONLY);
	char* hash_image_content = (char *)malloc(hash_image_size);
	memset(hash_image_content, 0, hash_image_size);
	bytes_read = read(hash_image_fd, hash_image_content, hash_image_size);
	if (bytes_read != hash_image_size) {
		printf("Error: read %s error!\n", hash_image_path);
		exit(1);
	}
	close(hash_image_fd);

	write_content_with_padding(nfsb_image_fd, hash_image_content,
		hash_image_size, hash_image_size);

	close(nfsb_image_fd);
	printf("NFSB disk image written to %s.\n", nfsb_image_path);
}

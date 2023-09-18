/*
  * Crypt_header is placed in the first sector of crypt image
  * It include dm crypt parameters and will be encrypted in the first bootup by per-device encryption key
*/
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <malloc.h>
#include <endian.h>

#define SECTOR_SIZE (512)
#define PAGE_SIZE (4096)

#define MAX_CONST_LEN (16)
#define MAX_KEY_LEN	  (64)

#define DM_PROTECT_KEY_LEN (16)
#define DM_PROTECT_IV_LEN	(16)

#define MAGIC_STR "CRYPT_HEADER_V1"

struct crypt_header
{
	char magic_1[MAX_CONST_LEN];
	/* encryption begin */
	char magic_2[MAX_CONST_LEN];
	char parttag[MAX_CONST_LEN];
	char cipher[MAX_CONST_LEN];
	char chainmode[MAX_CONST_LEN];
	char ivmode[MAX_CONST_LEN];
	char ivopts[MAX_CONST_LEN];
	int sector_start;
	int sector_size;
	int key_size;
	int iv_offset;
	char encrypted_key[MAX_KEY_LEN]; /*vendor key encryption key*/
	/* encryption end */
};

static unsigned char hex_to_bin(char hex)
{
	if( hex >= '0' && hex <= '9')
		return hex - '0';
	if( hex >= 'a' && hex <= 'f')
		return hex - 'a' + 10;
	if( hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	fprintf(stderr,"Invalid hex value 0x%x\n",hex);
	exit(1);
}

static char bin_to_hex(unsigned char bin)
{
	if ( bin >=0 && bin <=9)
	{
		return bin+'0';
	}
	if ( bin >= 0xa && bin <=0xf)
	{
		return bin-0xa+'a';
	}
	fprintf(stderr,"Invalid bin value 0x%x\n",bin);
	exit(1);
}

static void hexstr_to_array(const char* hex_str,int len, char* out)
{
	int i = 0;
	for(i=0; i<len; i+=2)
	{
		out[i/2] = (hex_to_bin(hex_str[i])<<4) | (hex_to_bin(hex_str[i+1]));
	}
}

static void array_to_hexstr(const char* bin_array, int len, char* out)
{
	int i=0;
	for(i=0;i<len;i++)
	{
		out[2*i] = bin_to_hex(bin_array[i]>>4);
		out[2*i+1] = bin_to_hex(bin_array[i]%16);
	}
}

static const unsigned char initial_iv[DM_PROTECT_IV_LEN] = {0x7e, 0x61, 0x4b, 0xd3, 
											0xe2, 0xc8, 0xc3, 0x09, 
											0xed, 0x57, 0x02, 0x04, 
											0x8f, 0xac, 0x0b, 0xc5};

extern "C" int aes_cbc_decrypt(
	const unsigned char *piv,          /* [in]  initial vector or initial counter value buffer, 16 bytes */
	const unsigned char *pkey,         /* [in]  key buffer */
	unsigned int       key_len,      /* [in]  key length (byte unit), 16 (128 bits), 24(192 bits), 32(256 bits) */
	const unsigned char *psrc,         /* [in]  source data buffer */
	unsigned char       *pdes,         /* [out] destination data buffer, memory prepared by caller */
	unsigned int       dat_len       /* [in]  data buffer length for source/destination (byte unit), multiple of 16 */
);

extern const char* get_dm_protect_key_str(void);
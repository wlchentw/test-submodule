/* Copyright Statement:                                                        
 *                                                                             
 * This software/firmware and related documentation ("MediaTek Software") are  
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without 
 * the prior written permission of MediaTek inc. and/or its licensors, any     
 * reproduction, modification, use or disclosure of MediaTek Software, and     
 * information contained herein, in whole or in part, shall be strictly        
 * prohibited.                                                                 
 *                                                                             
 * MediaTek Inc. (C) 2014. All rights reserved.                                
 *                                                                             
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES 
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")     
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER  
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL          
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED    
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR          
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH 
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,            
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.   
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK       
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE  
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR     
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S 
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE       
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE  
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE  
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.    
 *                                                                             
 * The following software/firmware and/or related documentation ("MediaTek     
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc. 
 */
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <sys/mman.h>
#include "crypt_tool.h"


uint64_t getfilesize(char* path)
{
	int result;
	struct stat buf;
	result = stat(path,&buf);
	if(result == 0)
		return buf.st_size;
	else
		return 0;
}

static void usage(const char* name)
{
	fprintf(stderr,"Usage:\n"
		"%s <cipher> <key> <iv_offset> <offset> [input image] [output image] [parttag] [dm protect key]\n"
		"Options:\n"
		"cipher: Encryption cipher and an optional IV generation mode.(In format cipher[:keycount]-chainmode-ivmode[:ivopts]).\n"
		"key: Key used for encryption. It is encoded as a hexadecimal number.\n"
		"iv_offset: The IV offset is a sector count that is added to the sector number before creating the IV.\n"
		"offset: Starting sector within the device where the encrypted data begins.\n"
		"input image: path of input ext4 image\n"
		"output image: path of output dm-crypt image\n"
		"parttag: dm device name at runtime\n"
		"dm protect key: protect crypt header before first bootup\n",name);
}

void dump_hex(const char* name,uint8_t buff[],int len)
{
	int i=0;
	printf("%s content is:",name);
	for(;i<len;i++)
	{
		if(i%16 == 0) printf("\n");
		printf("0x%x ",buff[i]);
	}
	printf("\n");
}

typedef enum
{
	FS_TYPE_EXT4,
	FS_TYPE_UNKNOWN
}fs_type;

fs_type getfstype(char* buf)
{
	fs_type type = FS_TYPE_UNKNOWN;
	
	if(*(unsigned short*)(buf+0x400+0x38)==0xEF53)
		type = FS_TYPE_EXT4;

	return type;
}

struct crypt_config
{
	struct crypt_header header;
		
	unsigned char* key;
	int iv_size;
	unsigned char* iv;
	const EVP_CIPHER* evp_cipher;
};

struct crypt_config* build_crypt_config(char* cipher_in, char* key, char* encrypted_key, int iv_offset, int offset, char* parttag)
{
	struct crypt_config* cc = (struct crypt_config*)malloc(sizeof(struct crypt_config));
	char* cipher_string = strdup(cipher_in);
	char* keycount = strsep(&cipher_string,"-");
	char* cipher = strsep(&keycount,":");

	memset(cc,0,sizeof(crypt_config));
	
	if(keycount)
	{
		fprintf(stderr,"keycount is not supported!\n");
		exit(1);
	}	
	
	char* chainmode = strsep(&cipher_string, "-");
	char* ivopts = strsep(&cipher_string, "-");
	char* ivmode = strsep(&ivopts, ":");
	
	if (strcmp(cipher, "aes")) {
		fprintf(stderr, "Only support aes cipher now\n");
		exit(1);
	}
	
	if (!chainmode || strcmp(chainmode, "cbc")) {
		fprintf(stderr, "Only support cbc chainmode now\n");
		exit(1);
	}
	
	if (!ivmode || strcmp(ivmode, "plain")) {
		fprintf(stderr, "Only support plain ivopts now\n");
		exit(1);
	}
	
	if (ivopts) {
		fprintf(stderr, "Not support ivopts now\n");
		exit(1);
	}

	strcpy(cc->header.magic_1,MAGIC_STR);
	strcpy(cc->header.magic_2,MAGIC_STR);
	strcpy(cc->header.parttag,parttag);
	strcpy(cc->header.cipher,cipher);
	strcpy(cc->header.chainmode,chainmode);
	strcpy(cc->header.ivmode,ivmode);
	strcpy(cc->header.ivopts,"");
	cc->header.sector_start = offset;
	cc->header.sector_size = SECTOR_SIZE;
	cc->header.iv_offset = iv_offset;
	cc->header.key_size = (strlen(key)>>1);
	hexstr_to_array(encrypted_key,strlen(encrypted_key),cc->header.encrypted_key);
	strcpy(cc->header.encrypted_key,encrypted_key);
	
	cc->key = (unsigned char*)malloc(cc->header.key_size);
	hexstr_to_array(key,strlen(key),(char*)cc->key);
	char final_cipher_name[64] = {0};
	snprintf(final_cipher_name, 64, "%s-%d-%s",cc->header.cipher,cc->header.key_size*8,cc->header.chainmode);
	cc->evp_cipher = EVP_get_cipherbyname(final_cipher_name);
	if(!cc->evp_cipher) {
		fprintf(stderr, "Alloc evp cipher fail:%s\n",final_cipher_name);
		exit(1);
	}
	cc->iv_size = EVP_CIPHER_iv_length(cc->evp_cipher);
	cc->iv = (unsigned char*)malloc(cc->iv_size);
}

static void dump_data(uint8_t *buff,int len)
{
	uint i=0;
	
    for (i=0; i<len; i++){
	if ((i%16)==0)
		printf("\n");
	printf("0x%02x,",buff[i]);
    }
    printf("\n");
}

int main(int argc,char **argv)
{
	struct stat in_img_sb,out_img_sb;
	uint32_t block_size;
	int in_image_fd=-1,out_image_fd=-1;
	uint64_t in_image_size,block_count;
	uint8_t *buf;
	uint8_t tmpbuf[SECTOR_SIZE];
	int i;
	fs_type img_fs_type;

	char *in_image, *out_image;
	char *cipher, *key;
	char *parttag;
	char *dm_protect_key_str;
	int iv_offset, offset;
	unsigned char dm_protect_key[DM_PROTECT_KEY_LEN];
	
	if(argc != 9)
	{
		usage(argv[0]);
		return -1;
	}

	OpenSSL_add_all_algorithms();
	ERR_load_crypto_strings();
	ERR_clear_error();
	
	cipher = argv[1];
	key = argv[2];
	iv_offset = atoi(argv[3]);
	offset = atoi(argv[4]);
	in_image = argv[5];
	out_image = argv[6];
	parttag = argv[7];
	dm_protect_key_str = argv[8];

	if(strlen(dm_protect_key_str) != DM_PROTECT_KEY_LEN*2) {
		printf("dm_protec_key len is not correct, len:0x%x\n",strlen(dm_protect_key_str));
		return -1;
	}

	if(stat(in_image,&in_img_sb)!=0)
	{
		printf("Could not stat input image %s\n",in_image);
		return -1;
	}

	if(offset <= 1)
	{
		offset = 1; //  SECTOR 1  store crypt_header
	}
	in_image_size = getfilesize(in_image);
	block_size = SECTOR_SIZE; //dm-crypt only support 512 sector size
	block_count = (in_image_size + SECTOR_SIZE -1)/SECTOR_SIZE;

	in_image_fd = open(in_image,O_RDONLY);
	buf = (uint8_t *)mmap(NULL,block_size*block_count,PROT_READ,MAP_SHARED,in_image_fd,0);
	out_image_fd = open(out_image, O_CREAT|O_TRUNC|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	struct crypt_config* cc=build_crypt_config(cipher,key,key,iv_offset,offset,parttag);
	hexstr_to_array(dm_protect_key_str,DM_PROTECT_KEY_LEN*2,(char*)dm_protect_key);
	AES_KEY dm_key;
	unsigned char dm_iv[DM_PROTECT_IV_LEN] = {0};
	memcpy(dm_iv,initial_iv,DM_PROTECT_IV_LEN);
	AES_set_encrypt_key(dm_protect_key,DM_PROTECT_KEY_LEN*8,&dm_key);
	memset(tmpbuf,0,SECTOR_SIZE);
	memcpy(tmpbuf,&cc->header,sizeof(cc->header));
	AES_cbc_encrypt(tmpbuf,tmpbuf,SECTOR_SIZE,&dm_key,dm_iv,AES_ENCRYPT);
	write(out_image_fd,tmpbuf,SECTOR_SIZE);

	off64_t _offset = cc->header.sector_start * cc->header.sector_size;
	lseek64(out_image_fd,_offset,SEEK_SET);
	
	for(uint64_t block_index = 0 ; block_index<block_count; block_index++) {
		EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
		int outlen,tmplen;
		//plain iv mode
		memset(cc->iv,0,cc->iv_size);
		*(unsigned int*)cc->iv = ((block_index+cc->header.iv_offset) & 0xffffffff);
		
		//printf("key is :");
		//dump_data(cc->key,16);
		//printf("iv is :");
		//dump_data(cc->iv,16);
		EVP_EncryptInit_ex(ctx,cc->evp_cipher,NULL,cc->key,cc->iv);
		EVP_CIPHER_CTX_set_padding(ctx,0);
		EVP_EncryptUpdate(ctx,tmpbuf,&outlen,&buf[block_index*block_size],block_size);
		EVP_EncryptFinal_ex(ctx,tmpbuf+outlen,&tmplen);
		EVP_CIPHER_CTX_cleanup(ctx);

		write(out_image_fd,tmpbuf,SECTOR_SIZE);
	}

	_offset = lseek64(out_image_fd,0,SEEK_CUR);

	if(_offset%PAGE_SIZE != 0) {
		char tmp_[PAGE_SIZE] = {0};
		write(out_image_fd,tmp_,PAGE_SIZE - _offset%PAGE_SIZE);
	}

	close(out_image_fd);
	close(in_image_fd);

	return 0;
}


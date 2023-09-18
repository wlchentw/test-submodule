#include <sys/ioctl.h>
#include <linux/fs.h>
#include "crypt_tool.h"

extern "C" {
#include "mtk_crypto_api_user.h"
}

extern "C" unsigned char* iv_generator(int iv_len);

static void usage_mount(const char* name)
{
	fprintf(stderr,"Usage:\n"
		"%s [parttag]\n",name);
}

static void usage_enckey(const char* name)
{
	fprintf(stderr,"Usage:\n"
		"%s enckey [mode] [dm key str]\n",name);
}

/*
dmsetup create [dm dev name] --table "0 [block count] crypt [cipher]-[chainmode]-[ivmode]:[ivopts] [iv_offset] [blkdev_path] [offset]
*/

char dm_setup_param[] = "dmsetup create %s --table "
					   "\"0 %lld crypt %s-%s-%s %s "
					   "%d %s %d\"";

#define MAX_CMD_LINE_LEN 1024
char dm_setup_cmd[MAX_CMD_LINE_LEN];
unsigned char dm_protect_key[DM_PROTECT_KEY_LEN];
unsigned char dm_protect_dec_key_mode_1[DM_PROTECT_KEY_LEN];
unsigned char dm_protect_dec_key_mode_2[DM_PROTECT_KEY_LEN];


#define PART_ID_START (1)
#define PART_ID_END (32)
#define PART_NAME_MAX_LEN (64)

int main(int argc,char **argv)
{
	int blkdev_fd;
	long long blkdev_sz;
	unsigned char tmpbuf[SECTOR_SIZE];
	struct crypt_header* crypt_header;
	int is_encrypted = 0;
	char blkdev_path[PART_NAME_MAX_LEN] = {0};
	unsigned char* iv;
	char *parttag;
	int i;
	const char* dm_protect_key_str = NULL;
	int dm_protect_exist = 0;

	if(argc != 2 && argc != 4) {
		usage_mount(argv[0]);
		usage_enckey(argv[0]);
		return -1;
	}

	if(strcmp(argv[1],"enckey")==0) {
		int mode = atoi(argv[2]);
		unsigned char dm_protect_key[DM_PROTECT_KEY_LEN] = {0};
		char* dm_protect_key_str = argv[3];
		unsigned char dm_protect_enc_key[DM_PROTECT_KEY_LEN] = {0};
		unsigned char dm_protect_enc_key_str[DM_PROTECT_KEY_LEN*2+1] = {0};

		if(strlen(dm_protect_key_str)!=DM_PROTECT_KEY_LEN*2) {
			printf("dm_protec_key len is not correct, len:0x%x\n",strlen(dm_protect_key_str));
			return -1;
		}

		if(mode == VENDOR_KEY_MODE_1 || mode == VENDOR_KEY_MODE_2) {
			int ret;
			hexstr_to_array(dm_protect_key_str,DM_PROTECT_KEY_LEN*2,(char*)dm_protect_key);
			ret = aes_vendor_key_encrypt(mode,dm_protect_key,dm_protect_enc_key,DM_PROTECT_KEY_LEN);
			if(ret != 0) {
				printf("enckey mode %d fail, ret is 0x%x\n",mode,ret);
				return -1;
			}
			array_to_hexstr((const char*)dm_protect_enc_key,DM_PROTECT_KEY_LEN,(char*)dm_protect_enc_key_str);
			printf("the mode %d dm_protect_enc_key is:\n",mode);
			printf("%s\n",dm_protect_enc_key_str);
			return 0;
		}

		printf("command unknown error\n");
		return 0;
	}

	if(strcmp(argv[1],"sk")==0) {
		printf("%s\n",DM_PROTECT_KEY);
		return 0;
	}

	parttag = argv[1];

	if(strcmp(DM_PROTECT_KEY,"not_embedded") == 0) {
		dm_protect_key_str = get_dm_protect_key_str();
	} else {
		dm_protect_key_str = strdup(DM_PROTECT_KEY);
	}
	if(strlen(dm_protect_key_str) != DM_PROTECT_KEY_LEN*2) {
		fprintf(stderr,"dm protect key len error!\n");
		return -2;
	}

	hexstr_to_array(dm_protect_key_str, DM_PROTECT_KEY_LEN*2, (char*)dm_protect_key);

	aes_vendor_key_decrypt(VENDOR_KEY_MODE_1,dm_protect_key,dm_protect_dec_key_mode_1,DM_PROTECT_KEY_LEN);

	aes_vendor_key_decrypt(VENDOR_KEY_MODE_2,dm_protect_key,dm_protect_dec_key_mode_2,DM_PROTECT_KEY_LEN);

	for(i = PART_ID_START; i < PART_ID_END; i++)
	{
		int tmp_blkdev_fd;
		
		memset(blkdev_path,0,PART_NAME_MAX_LEN);
		snprintf(blkdev_path,PART_NAME_MAX_LEN,"/dev/mmcblk0p%d",i);
		tmp_blkdev_fd = open(blkdev_path,O_RDONLY);
		if(tmp_blkdev_fd == -1)
			continue;
		read(tmp_blkdev_fd,tmpbuf,SECTOR_SIZE);
		close(tmp_blkdev_fd);
		crypt_header = (struct crypt_header*)tmpbuf;
		dm_protect_exist = 0;
		if(strcmp(crypt_header->magic_1,MAGIC_STR)!=0) {
			unsigned char dec_tmpbuf[SECTOR_SIZE];
			/* try mode 0 (plain key mode) */
			aes_cbc_decrypt(initial_iv,dm_protect_key,DM_PROTECT_KEY_LEN,tmpbuf,dec_tmpbuf,SECTOR_SIZE);
			if(memcmp(dec_tmpbuf,MAGIC_STR,strlen(MAGIC_STR))!=0) {
				/* try mode 1 (vendor key mode) */
				aes_cbc_decrypt(initial_iv,dm_protect_dec_key_mode_1,DM_PROTECT_KEY_LEN,tmpbuf,dec_tmpbuf,SECTOR_SIZE);
				if(memcmp(dec_tmpbuf,MAGIC_STR,strlen(MAGIC_STR))!=0) {
					/* try mode 2 (vendor key + customer iv mode)*/
					aes_cbc_decrypt(initial_iv,dm_protect_dec_key_mode_2,DM_PROTECT_KEY_LEN,tmpbuf,dec_tmpbuf,SECTOR_SIZE);
					if(memcmp(dec_tmpbuf,MAGIC_STR,strlen(MAGIC_STR))!=0) {
						continue;
					} else {
						memcpy(dm_protect_key,dm_protect_dec_key_mode_2,DM_PROTECT_KEY_LEN);
						memset(dm_protect_dec_key_mode_2,0,DM_PROTECT_KEY_LEN);
					}
				} else {
					memcpy(dm_protect_key,dm_protect_dec_key_mode_1,DM_PROTECT_KEY_LEN);
					memset(dm_protect_dec_key_mode_1,0,DM_PROTECT_KEY_LEN);
				}
			}
			memcpy(tmpbuf,dec_tmpbuf,SECTOR_SIZE);
			dm_protect_exist = 1;
		}
		if(memcmp(crypt_header->magic_1,crypt_header->magic_2,MAX_CONST_LEN)!=0) {
			aes_ecb_hw_key_decrypt((unsigned char*)&crypt_header->magic_2,
							   (unsigned char*)&crypt_header->magic_2,
							   sizeof(struct crypt_header)-MAX_CONST_LEN);
			if(strcmp(crypt_header->magic_1,crypt_header->magic_2)!=0)
				continue;
		}
		if(strcmp(crypt_header->parttag,parttag)!=0)
			continue;
		break;
	}

	if(i == PART_ID_END) {
		fprintf(stderr,"Failed to found dm crypt image!\n");
		return -1;
	}
retry:
	
	blkdev_fd = open(blkdev_path,O_RDWR);
	if(blkdev_fd == -1) {
		fprintf(stderr,"Failed to open %s, program exit!\n",blkdev_path);
		return -2;
	}

	if (ioctl(blkdev_fd, BLKGETSIZE64, &blkdev_sz) == 0) {
		blkdev_sz = (blkdev_sz >> 9) - 1; //convert to 512 block size, exclude 1st sector for crypt_header
		if (blkdev_sz != (uint32_t) blkdev_sz) {
		  fprintf(stderr,"device has more than 2^32 sectors, can't use all of them\n");
		  return -3;
		}
	}	

	read(blkdev_fd,tmpbuf,SECTOR_SIZE);
	crypt_header = (struct crypt_header*)tmpbuf;
	close(blkdev_fd);

	if(dm_protect_exist) {
		aes_cbc_decrypt(initial_iv,dm_protect_key,DM_PROTECT_KEY_LEN,tmpbuf,tmpbuf,SECTOR_SIZE);
		memset(dm_protect_key,0,DM_PROTECT_KEY_LEN);
		dm_protect_exist = 0;
	}

	if(strcmp(crypt_header->magic_1,MAGIC_STR)!=0)
	{
		fprintf(stderr,"Can not find magic str in header of %s, exit\n",MAGIC_STR);
		return -4;
	}
	
	if(memcmp(crypt_header->magic_1,crypt_header->magic_2,MAX_CONST_LEN)==0) {
		is_encrypted = 0;
	}
	else {
		is_encrypted = 1;
	}

	if(is_encrypted)
	{
		aes_ecb_hw_key_decrypt((unsigned char*)&crypt_header->magic_2,
							   (unsigned char*)&crypt_header->magic_2,
							   sizeof(struct crypt_header)-MAX_CONST_LEN);
	} else {
		aes_ecb_hw_key_encrypt((unsigned char*)&crypt_header->magic_2,
							   (unsigned char*)&crypt_header->magic_2,
							   sizeof(struct crypt_header)-MAX_CONST_LEN);
		blkdev_fd = open(blkdev_path,O_RDWR);
		write(blkdev_fd,crypt_header,SECTOR_SIZE); //because crypt_header is point to the header of tmpbuf
		close(blkdev_fd);
		goto retry;
	}

	if(strcmp(crypt_header->magic_1,crypt_header->magic_2)!=0)
	{
		fprintf(stderr,"Error: maigc number in header is not same, exit\n");
		return -4;
	}

	snprintf(dm_setup_cmd,MAX_CMD_LINE_LEN,dm_setup_param,
			parttag,blkdev_sz,crypt_header->cipher,crypt_header->chainmode,
			crypt_header->ivmode,crypt_header->encrypted_key,crypt_header->iv_offset,blkdev_path,crypt_header->sector_start);

	printf("dmsetup cmd is:%s\n",dm_setup_cmd);

	system(dm_setup_cmd);
	
	return 0;
}


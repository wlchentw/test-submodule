#include <sys/ioctl.h>
#include <linux/fs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
extern "C" {
#include "nfsb.h"
}
#include "nfsb_key_modulus.h"
#include "nfsb_key_public.h"

#define MAX_CMDLINE_LEN 4096
#define MAX_DEVICE_NAME_LEN 64
#define SECTOR_SHIFT 9

char* linear_cmd_tmp = "dmsetup create %s -r --table \"%ld %ld linear %s %ld\"";
char* verity_cmd_tmp = "dmsetup create %s -r --table \"%ld %ld verity %u /dev/mapper/%s /dev/mapper/%s %u %u %u %ld %s %s %s\"";

static inline unsigned long to_sector(unsigned long n)
{
	return (n >> SECTOR_SHIFT);
}

int main(int argc,char **argv)
{
	int argc_num = argc;
	char* device_path = argv[argc_num - 2];
	int device_fd = 0;
	struct nfsb_header header;
	char linear_setup_cmd[MAX_CMDLINE_LEN] = {0};
	char verity_setup_cmd[MAX_CMDLINE_LEN] = {0};
	char mount_cmd[MAX_CMDLINE_LEN] = {0};
	char linear_device_name[MAX_DEVICE_NAME_LEN] = {0};
	char verity_device_name[MAX_DEVICE_NAME_LEN] = {0};
	unsigned long linear_size = 0;

	memset(&header, 0 , sizeof(struct nfsb_header));
	device_fd = open(device_path,O_RDONLY);
	if(device_fd == -1) {
		fprintf(stderr,"Fail to open device %s\n", device_path);
		return -1;
	}
	if (ioctl(device_fd, BLKGETSIZE64, &linear_size) == 0) {			
		if (linear_size != (unsigned int) linear_size) {		  
			fprintf(stderr,"device has more than 2^32 sectors, can't use all of them\n");		  
			return -3;		
		}	
	}
	int i = read(device_fd, &header, sizeof(nfsb_header));
	close(device_fd);

	if(nfsb_verify(&header, rsa_pubkey, rsa_modulus)) {
		fprintf(stderr,"nfsb verify fail, exit!\n");
		return -2;
	}

	printf("nfsb verify pass\n");

	snprintf(linear_device_name, MAX_DEVICE_NAME_LEN, "%s_linear", basename(device_path));
	snprintf(verity_device_name, MAX_DEVICE_NAME_LEN, "%s_verity", basename(device_path));

	snprintf(linear_setup_cmd, MAX_CMDLINE_LEN, linear_cmd_tmp, linear_device_name,
			 0, to_sector(linear_size) - to_sector(nfsb_fs_offset(&header)),
			 device_path, to_sector(nfsb_fs_offset(&header)));

	snprintf(verity_setup_cmd, MAX_CMDLINE_LEN, verity_cmd_tmp, verity_device_name,
			0, to_sector(nfsb_fs_size(&header)), nfsb_hash_type(&header), linear_device_name, linear_device_name,
		 	nfsb_data_blocksize(&header), nfsb_hash_blocksize(&header), nfsb_data_blockcount(&header),
		 	nfsb_fs_size(&header) / nfsb_hash_blocksize(&header), nfsb_hash_algo(&header),
		 	nfsb_verity_hash(&header), nfsb_verity_salt(&header));

	printf("%s\n",linear_setup_cmd);
	printf("%s\n",verity_setup_cmd);

	int idx = 1;

	strcat(mount_cmd, "mount ");
	for(idx = 1; idx < argc-2; idx++) {
		strcat(mount_cmd, argv[idx]);
		strcat(mount_cmd, " ");
	}

	strcat(mount_cmd, "/dev/mapper/");
	strcat(mount_cmd, verity_device_name);
	strcat(mount_cmd, " ");

	strcat(mount_cmd, argv[argc-1]);

	printf("%s\n", mount_cmd);

	if(system(linear_setup_cmd)!=0) {
		fprintf(stderr,"nfsb create linear device fail, exit!\n");
		return -4;
	}
	if(system(verity_setup_cmd)!=0) {
		fprintf(stderr,"nfsb create verity device fail, exit!\n");
		return -5;
	}
	if(system(mount_cmd)!=0) {
		fprintf(stderr,"mount filesystem, exit!\n");
		return -6;
	}
	return 0;
}


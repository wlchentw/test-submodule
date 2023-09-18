#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>
#include "keyinstall.h"

#ifndef ALOGE
#define ALOGE printf
#endif

#define KEYBLOCK_F_PATH "/dev/mtd13"//this is for mt8518
#define SINGLE_KEY_MAX_SIZE (50*1024)//single key must be smaller than 50KB
#define KEYBLOCK_SIZE (512*1024)//total key size;fixed to 512KB
#define KEYTYPEMAX (8)

#ifdef PLATFORM_MT8532
#define KB_PARTITION_PATH "/dev/block/platform/soc/by-name/kb" //8532-platform is diffrent
#else
#define KB_PARTITION_PATH "/dev/block/platform/11120000.mmc/by-name/kb" //kb partiton for read/write
#endif

typedef struct
{
    unsigned char magic[4];//'HEAD'
    unsigned int keyinstalled[KEYTYPEMAX];//maximun keytypes
    //unsigned int keycount; //keycount-unused now
}KBHeader_T;

typedef struct
{
    unsigned int kb_size;//equal to mtd size;the size of kb
    unsigned int erase_size;//equal to erase size;per block size of kb
    unsigned int total_num;//total blocks of kb;
    unsigned int good_block_num;//total good blocks of kb
    unsigned int first_good_block;//first good block of kb
    unsigned int write_block_num;//write block of DRMKEY BLOCK FILE
    int         isGoodBlockDetected;//is good block detected; 1--Yes,0--NO;
}KB_INFO_T;

KB_INFO_T  kb_info;

int freekeyblock(unsigned char* keyblock){
    if(keyblock)
        free(keyblock);

    return 0;
}

int get_info_for_kb(char* KBPath)
{
    int dev_fd;
    mtd_info_t mtd;
    loff_t bpos;
    dev_fd = open(KBPath,O_SYNC|O_RDWR);
    if(dev_fd < 0)
    {
        ALOGE("open %s failed\n",KBPath);
        return -1;
    }
    if(ioctl(dev_fd,MEMGETINFO,&mtd)<0)
    {
        ALOGE("%s:MTD getinfo failed\n",KBPath);
        close(dev_fd);
        return -2;
    }

    //set kb info
    memset(&kb_info,0,sizeof(KB_INFO_T));
    kb_info.kb_size = mtd.size;
    kb_info.erase_size = mtd.erasesize;
    kb_info.total_num = (mtd.size)/(mtd.erasesize);
    kb_info.write_block_num = KEYBLOCK_SIZE/(mtd.erasesize);
    kb_info.good_block_num = kb_info.total_num;
    kb_info.first_good_block = 0;
    kb_info.isGoodBlockDetected = 0;
    int i = 0;
    for(; i < kb_info.total_num; i++){
        bpos = i*kb_info.erase_size;
        if (ioctl(dev_fd, MEMGETBADBLOCK, &bpos) > 0) {
            ALOGE("Bad block detected.index = %d.\n",i);
            kb_info.good_block_num--;
        }else if(kb_info.isGoodBlockDetected == 0){
            kb_info.first_good_block = i;
            kb_info.isGoodBlockDetected = 1;
        }
    }
#if 0
    //logger out kb info
    ALOGE("kb_info.kb_size: %d.\n",kb_info.kb_size);
    ALOGE("kb_info.erase_size: %d.\n",kb_info.erase_size);
    ALOGE("kb_info.total_num: %d.\n",kb_info.total_num);
    ALOGE("kb_info.write_block_num: %d.\n",kb_info.write_block_num);
    ALOGE("kb_info.good_block_num: %d.\n",kb_info.good_block_num);
    ALOGE("kb_info.first_good_block: %d.\n",kb_info.first_good_block);
    ALOGE("kb_info.isGoodBlockDetected: %d.\n",kb_info.isGoodBlockDetected);
#endif
    if(kb_info.good_block_num < kb_info.write_block_num)
    {
        ALOGE("Not enough blocks for drmkey.Good block:%d,Should reserve:%d.\n",
            kb_info.good_block_num,kb_info.write_block_num);
            close(dev_fd);
            return -3;
    }

    close(dev_fd);
    return 0;
}

int earse_keyblock_local_nand(int dev_fd,int start)
{
    erase_info_t erase;

    //try to erase keyblock,if failed,return for set bad block
    erase.start = start;
    erase.length = kb_info.erase_size;

    if(ioctl(dev_fd,MEMERASE,&erase)<0)
    {
        ALOGE("MTD erase failed.\n");
        return -1;
    }

    return 0;
}

static int is_mtd_device(void)
{
    int is_mtd = 0;
    struct stat sb;
    is_mtd = stat("/dev/mtd1",&sb);
    //ALOGE("%s",is_mtd?"EMMC DEVICE":"NAND DEVICE");
    return (is_mtd == 0);
}

int write_keyblock_to_emmc(unsigned char* keyblock){
    int len = KEYBLOCK_SIZE;
    int fd = 0, n=0;

    if(!keyblock){
        ALOGE("invalid pointer for keyblock.\n");
        return -1;
    }

//write
    fd = open(KB_PARTITION_PATH, O_RDWR);
    if(fd <0)
    {
        ALOGE("open EMMC fail: %d\n",fd);
        return -2;
    }

    lseek(fd,0,SEEK_SET);
    n=write(fd,keyblock,len);
    if(n!=len){
        ALOGE("ERROR write EMMC fail.");
        close(fd);
        return -3;
    }
    close(fd);

    return 0;
}

#include <errno.h>
extern int errno;
int read_keyblock_from_emmc(unsigned char* keyblock){

    int len = KEYBLOCK_SIZE;
    int fd = 0, n=0;

    if(!keyblock){
        ALOGE("invalid pointer for keyblock.\n");
        return -1;
    }

//read
    fd = open(KB_PARTITION_PATH, O_RDWR);
    if(fd <0)
    {
        ALOGE("open EMMC fail: %d.path:%s.error=%d.\n",fd,KB_PARTITION_PATH,errno);
        return -2;
    }

    lseek(fd,0,SEEK_SET);
    n=read(fd,keyblock,len);
    if(n!=len){
        ALOGE("ERROR read EMMC fail.");
        close(fd);
        return -3;
    }
    close(fd);

    return 0;
}

int read_keyblock_from_nand(unsigned char* keyblock){
    int dev_fd;
    int iResult = 0;
    int pos = 0;

    if(!keyblock)
    {
        ALOGE("NULL input pointer.\n");
        return -1;
    }

    //get kb_info
    if(get_info_for_kb(KEYBLOCK_F_PATH) < 0){
        ALOGE("kb info is corruppted.\n");
        return -2;
    }

    dev_fd=open(KEYBLOCK_F_PATH,O_RDWR|O_SYNC);
    if(dev_fd<0) {
        ALOGE("%s:MTD open failed\n",KEYBLOCK_F_PATH);
        return -3;
    }

    //try to read keyblock from nand;read blocks confirmed
    int i = 0;
    for(; i < kb_info.write_block_num;i++){
        pos = (kb_info.first_good_block + i) * kb_info.erase_size;
        if (ioctl(dev_fd, MEMGETBADBLOCK, &pos) > 0) {
            ALOGE("Bad block detected.index = %d.try to escape.\n",i);
        }else{
            lseek(dev_fd,pos,SEEK_SET);
            iResult = read(dev_fd,keyblock+i*kb_info.erase_size,kb_info.erase_size);
            if(iResult != kb_info.erase_size) {
                ALOGE("Read failed.ret=%d.\n",iResult);
                close(dev_fd);
                return -3;
            }
        }
    }

    close(dev_fd);
    return 0;
}

int write_keyblock_to_nand(unsigned char* keyblock){
    int dev_fd;
    int iResult = 0;
    int pos = 0;

    if(!keyblock)
    {
        ALOGE("NULL input pointer.\n");
        return -1;
    }

    //get kb_info
    if(get_info_for_kb(KEYBLOCK_F_PATH) < 0){
        ALOGE("kb info is corruppted.\n");
        return -2;
    }

    dev_fd=open(KEYBLOCK_F_PATH,O_RDWR|O_SYNC);
    if(dev_fd<0) {
        ALOGE("%s:MTD erase failed\n",KEYBLOCK_F_PATH);
        return -3;
    }

    //write keyblock to nand,if failed,set bad block
    int write_block = 0;
    int i = 0;
    for(i = kb_info.first_good_block; i < kb_info.total_num;i++){
        pos = i * kb_info.erase_size;
        if (ioctl(dev_fd, MEMGETBADBLOCK, &pos) > 0) {
            ALOGE("Bad block detected.index = %d.try to escape.\n",i);
        }else{
            //erase first
            if(earse_keyblock_local_nand(dev_fd,pos) < 0){
                ALOGE("Erase failed.try to set bad block.\n");
                ioctl(dev_fd, MEMSETBADBLOCK, &pos);
                continue;
            }

            lseek(dev_fd,pos,SEEK_SET);
            iResult = write(dev_fd,keyblock + write_block*kb_info.erase_size ,kb_info.erase_size);
            if(iResult != kb_info.erase_size) {
                ALOGE("write failed.ret=%d.\n",iResult);
                ioctl(dev_fd, MEMSETBADBLOCK, &pos);
            }else{
                write_block++;
            }
        }

        if(write_block == kb_info.write_block_num)
            break;
    }
    if(write_block < kb_info.write_block_num){
        ALOGE("Not enough blocks for write.");
        return -4;
    }

    close(dev_fd);
    return 0;
}

int write_key_to_keyblock(int drmKeyId,char* KBPath,unsigned char* kb,int len)
{
    KBHeader_T header;
    int device_type = 0;
    int ret = 0;

    if(len <= 0 || len > SINGLE_KEY_MAX_SIZE)
    {
        ALOGE("ERROR invalid input keylength. Please Adjust it.\n");
        return -1;
    }

    if(!kb)
    {
        ALOGE("ERROR input key is null. Please Adjust it.\n");
        return -2;
    }

    if(drmKeyId < 0 || drmKeyId >= KEYTYPEMAX)
    {
        ALOGE("ERROR invalid input drmKeyId. Please Adjust it.\n");
        return -3;
    }

    unsigned char* dummy_kb=(unsigned char*)malloc(KEYBLOCK_SIZE);
    if(!dummy_kb)
    {
        ALOGE("ERROR allocate dummy_kb.\n");
        return -4;
    }
    memset(dummy_kb,0,KEYBLOCK_SIZE);
    device_type = is_mtd_device();
    if(device_type){
        ret = read_keyblock_from_nand(dummy_kb);
    }else{
        ret = read_keyblock_from_emmc(dummy_kb);
    }
    if(ret != 0){
        ALOGE("read keyblock failed.\n");
        freekeyblock(dummy_kb);
        return -5;
    }

    memset(&header,0,sizeof(KBHeader_T));
    memcpy(&header,dummy_kb,sizeof(KBHeader_T));
    //if no key istalled,initalize header
    if(header.magic[0] != 'H' || header.magic[1] != 'E')
    {
        memset(&header,0,sizeof(KBHeader_T));
        header.magic[0] = 'H';
        header.magic[1] = 'E';
        header.magic[2] = 'A';
        header.magic[3] = 'D';
    }
    header.keyinstalled[drmKeyId] = len;
    memcpy(dummy_kb,&header,sizeof(KBHeader_T));
    size_t offsize = sizeof(KBHeader_T) + 4 + drmKeyId * SINGLE_KEY_MAX_SIZE;
    memcpy(dummy_kb + offsize,kb,len);
    if(device_type){
        ret = write_keyblock_to_nand(dummy_kb);
    }else{
        ret = write_keyblock_to_emmc(dummy_kb);
    }
    if(ret != 0){
        ALOGE("write keyblock failed.\n");
        freekeyblock(dummy_kb);
        return -6;
    }

    freekeyblock(dummy_kb);
    return 0;
}

int read_key_from_keyblock(int drmKeyId,char* KBPath,unsigned char** kb,int* len)
{
    KBHeader_T header;
    int device_type = 0;
    int ret = 0;

    if(!len || !kb)
    {
        ALOGE("invalid input.\n");
        return -1;
    }

    if(drmKeyId < 0 || drmKeyId >= KEYTYPEMAX)
    {
        ALOGE("ERROR invalid input drmKeyId. Please Adjust it.\n");
        return -2;
    }

    unsigned char* keyblock = (unsigned char*)malloc(KEYBLOCK_SIZE);
    if(!keyblock){
        ALOGE("malloc keyblock failed.\n");
        return -3;
    }

    device_type = is_mtd_device();
    if(device_type){
        ret = read_keyblock_from_nand(keyblock);
    }else{
        ret = read_keyblock_from_emmc(keyblock);
    }

    if(ret != 0){
        ALOGE("read keyblock failed.\n");
        freekeyblock(keyblock);
        return -4;
    }

    //check header
    memset(&header,0,sizeof(KBHeader_T));
    memcpy(&header,keyblock,sizeof(KBHeader_T));
    //empty_file
    if(header.magic[0] != 'H' || header.magic[1] != 'E')
    {
        ALOGE("No key has been installed.\n");
        freekeyblock(keyblock);
        return -5;
    }

    //key not installed
    if(header.keyinstalled[drmKeyId] == 0)
    {
        ALOGE("KeyID:%d is not installed.\n",drmKeyId);
        freekeyblock(keyblock);
        return -6;
    }

    unsigned int key_size = header.keyinstalled[drmKeyId];
    if(key_size > SINGLE_KEY_MAX_SIZE)
    {
        ALOGE("keyblock file is corrupted.\n");
        freekeyblock(keyblock);
        return -7;
    }
    unsigned char* dummy_kb=(unsigned char*)malloc(key_size);
    if(!dummy_kb)
    {
        ALOGE("ERROR allocate dummy_kb.\n");
        freekeyblock(keyblock);
        return -8;
    }

    memset(dummy_kb,0,key_size);
    size_t offsize = sizeof(KBHeader_T) + 4 + drmKeyId * SINGLE_KEY_MAX_SIZE;
    memcpy(dummy_kb,keyblock+offsize,key_size);

    freekeyblock(keyblock);
    *len = key_size;
    *kb  = dummy_kb;

    return 0;
}

int write_key_to_device_API(int drmKeyId,unsigned char* kb,int len)
{
    return write_key_to_keyblock(drmKeyId,KEYBLOCK_F_PATH, kb, len);
}

int read_key_from_device_API(int drmKeyId,unsigned char** kb,int* len)
{
    return read_key_from_keyblock(drmKeyId,KEYBLOCK_F_PATH, kb, len);
}

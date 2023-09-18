/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2017
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <sys/mount.h>
#include "libfile_op.h"
#include "libnvram_log.h"
#include "libnvram.h"
#include "CFG_file_public.h"
#include <mtd/mtd-abi.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>

#define INVALID_HANDLE_VALUE    -1

typedef struct {
    unsigned int ulCheckSum;
    unsigned int iCommonFileNum;
    unsigned int iCustomFileNum;
} BackupFileInfo;
typedef struct {
    char	cFileName[128];
    unsigned int iLID;
} FileName;

static const char *g_pcNVM_AllFile    = "/data/nvram/AllFile";
static const char *g_pcNVM_AllMap     = "/data/nvram/AllMap";
static const char *g_pcNVM_AllFile_Check    = "/data/nvram/AllFileCheck";
static const char *g_pcNVM_AllMap_Check     = "/data/nvram/AllMapCheck";
static const char *g_pcNVM_Flag       = "/data/nvram/RestoreFlag";//The File Will be Created after restore

static const char *g_pcNVM_APCalFile  = "/data/nvram/APCFG/APRDCL";
static const char *g_pcNVM_APRdebFile = "/data/nvram/APCFG/APRDEB";
unsigned int gFileStartAddr = 0;
pthread_mutex_t gFileStartAddrlock = PTHREAD_MUTEX_INITIALIZER;

extern int nvram_platform_log_block;
extern int nvram_platform_resv_block;
#define MIN(a,b) ((a) <= (b) ? (a) : (b))
extern FileName aBackupToBinRegion[];
extern unsigned int g_i4CFG_File_Count;
extern const unsigned int g_Backup_File_Count;
extern unsigned int g_i4CFG_File_Custom_Count;
extern int nvram_read_back_feature;

pthread_mutex_t dirlock = PTHREAD_MUTEX_INITIALIZER;
extern int nvram_gpt_flag;
extern bool nvram_emmc_support();
bool FileOp_BackupDataToFiles(int * iFileMask,bool bWorkForBinRegion);
bool NVM_GetDeviceInfo(const char *path, struct mtd_info_user *device_info);
bool FileOp_RestoreFromBinRegion_ToFile();
bool FileOp_CheckBackUpResult();
bool FileOp_RestoreData_All(void);
bool FileOp_RestoreFromFiles(int eBackupType);

static int open_file_with_dirs(const char *fn, mode_t mode)
{
    char tmp[PATH_MAX];
    int i = 0;
    int filedesc = 0;
    struct stat info;
    int val=0;

    umask(007);
    pthread_mutex_lock(&dirlock);
    while(*fn) {
        tmp[i] = *fn;

        if(*fn == '/' && i) {
            tmp[i] = '\0';
            if(access(tmp, F_OK) != 0) {
                if(mkdir(tmp, 0770) == -1) {
                    NVRAM_LOG("mkdir error! %s\n",(char*)strerror(errno));
                    if(errno != EEXIST) {
                        pthread_mutex_unlock(&dirlock);
                        return -1;
                    }
                }
                val = stat(tmp,&info);
                if ((val == 0)) {
                    if (strstr(tmp, "BT_Addr")) {
                        if (-1 == chown(tmp, 1001/*bluetooth*/, 100/*users*/))
                            NVRAM_LOG("change BT_Addr owner failed(media):%s\n", (char*)strerror(errno));
                    } else if (strstr(tmp, "WIFI")) {
                        if (-1 == chown(tmp, 1008/*wifi*/, 100/*users*/))
                           NVRAM_LOG("change WIFI owner failed(media):%s\n", (char*)strerror(errno));
                    } else {
                        if (-1 == chown(tmp, 1000/*system*/, 100/*users*/))
                           NVRAM_LOG("change %s owner failed(media):%s\n", tmp, (char*)strerror(errno));
                    }
                }
                if ((val == 0) && !S_ISDIR(info.st_mode)) {
                    if(-1 == chmod(tmp, 0664))
                        NVRAM_LOG("chmod file failed: %s\n", (char*)strerror(errno));
                }
            }
            tmp[i] = '/';
        }
        i++;
        fn++;
    }
    tmp[i] = '\0';
    filedesc = open(tmp, O_TRUNC |O_CREAT|O_WRONLY|O_SYNC, mode);
    if(-1 != filedesc) {
        val = stat(tmp,&info);
        if ((val == 0)) {
            if (strstr(tmp, "BT_Addr")) {
                if (-1 == chown(tmp, 1001/*bluetooth*/, 100/*users*/))
                    NVRAM_LOG("change BT_Addr owner failed(media):%s\n", (char*)strerror(errno));
            } else if (strstr(tmp, "WIFI")) {
                if (-1 == chown(tmp, 1008/*wifi*/, 100/*users*/))
                    NVRAM_LOG("change WIFI owner failed(media):%s\n", (char*)strerror(errno));
            } else {
                if (-1 == chown(tmp, 1000/*system*/, 100/*users*/))
                   NVRAM_LOG("change %s owner failed(media):%s\n", tmp, (char*)strerror(errno));
            }
        }
        if ((val == 0) && !S_ISDIR(info.st_mode)) {
            if(-1 == chmod(tmp, mode))
                NVRAM_LOG("chmod file failed: %s\n", (char*)strerror(errno));
        }
    }
    pthread_mutex_unlock(&dirlock);
    return filedesc;

}

#define BitmapFlag 0xAABBCCDD
int  NvRamBlockNum = 0;
int BinRegionBlockTotalNum=0;
static char *bBadBlockBitMap = NULL;

bool FileOp_CreateBinRegionBadBlockBitMap()
{
    int fd,i;
    char cMtdDevName[64] = {0};
    int iWriteSize,iBlockSize,iPartitionSize,iBlockNum;
    struct mtd_info_user info;
    loff_t bpos;

    memset(cMtdDevName, 0, sizeof cMtdDevName);
    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE);
    //NVBAK_LOG("FileOp_CreateBinRegionBadBlockBitMap:%s\n",cMtdDevName);


    if(!NVM_GetDeviceInfo(cMtdDevName,&info)) {
        NVBAK_LOG("FileOp_CreateBinRegionBadBlockBitMap get device info error\r\n");
        return false;
    }

    fd=open(cMtdDevName,O_RDWR|O_SYNC);
    if(fd<0) {
        NVBAK_LOG("mtd open error\r\n");
        return false;
    }

    iWriteSize=info.writesize;
    iBlockSize=info.erasesize;
    iPartitionSize=info.size;

    NVBAK_LOG("writeSize 0x%x blockSize 0x%x partitionSize 0x%x\n",
              iWriteSize, iBlockSize, iPartitionSize);

    BinRegionBlockTotalNum = info.size/iBlockSize - nvram_platform_log_block - nvram_platform_resv_block;

    NvRamBlockNum=BinRegionBlockTotalNum;
    bBadBlockBitMap = NULL;
    bBadBlockBitMap=(char*)malloc(NvRamBlockNum);
    if(bBadBlockBitMap==NULL) {
        NVBAK_LOG("malloc bBadBlockBitMap error!!\r\n");
        close(fd);
        return false;
    }
    memset(bBadBlockBitMap,0,(NvRamBlockNum*sizeof(char)));
    iBlockNum=NvRamBlockNum;

    for(i=0; i<iBlockNum; i++) {
        bpos = i*iBlockSize;
        if(!nvram_emmc_support()) {
            if (ioctl(fd, MEMGETBADBLOCK, &bpos) > 0) {
                NVBAK_LOG("mtd MEMGETBADBLOCK bad block id:%d\r\n",i);
                bBadBlockBitMap[i]=1;
            } else {
                bBadBlockBitMap[i]=0;
            }
        }
    }
    close(fd);
    //for(i=0; i<iBlockNum; i++) {
    //    NVBAK_LOG("[NVRAM_Bitmap]:Block%d,%d\n",i,bBadBlockBitMap[i]);
    //}
    return true;

}

static int FileOp_Erase_Device(struct mtd_info_user *mtd_info, int fd, int start, int len)
{
    struct erase_info_user einfo;

    if(nvram_emmc_support()) {
        return 0;
    }

    /* NVBAK_LOG("start %s start_add=%d \n", __FUNCTION__, start); */
    /* set the erase block size */
    einfo.length = mtd_info->erasesize;
    if (len < mtd_info->erasesize)
        NVBAK_LOG("Warning: erase length not erasesize aligned, %d, just erase the whole block(%d)\n",
                  len, mtd_info->erasesize);
    /* NVBAK_LOG("start %s erasesize=%d \n", __FUNCTION__,einfo.length); */
    /* erase */
    for (einfo.start = start; einfo.start < start + len; einfo.start += einfo.length)
        ioctl(fd, MEMERASE, &einfo);

    /* NVBAK_LOG("end %s  end_add=%d \n", __FUNCTION__, start + len); */
    return 0;
}

bool FileOp_BackupToBinRegion_All( )
{
    bool bRet = true;
    struct stat st;
    char *tempBuffer=NULL;
    char *BitMapBuffer=NULL;
    struct mtd_info_user info;
    unsigned int iMapFileSize,iDatFileSize,iWriteSize,iBlockSize,iPartitionSize;
    unsigned int iMemSize;
    unsigned int iBitmapFlag=BitmapFlag;
    int iFileDesc_file, iFileDesc_map, fd, iResult,i,j,iWriteTime,pos=0,iFreeBlockNum=0;
    char cMtdDevName[64] = {0};
    NVBAK_LOG("Enter BackupToBinRegion_all\n");

    NVM_Init();
    memset(cMtdDevName, 0, sizeof cMtdDevName);
    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE);

    bool bWorkForBinRegion=true;
    int iFileMask[ALL];
    iFileMask[APBOOT] = iFileMask[APCLN] = 0;

    bRet = FileOp_BackupDataToFiles(iFileMask,bWorkForBinRegion);
    if(bRet == false) {
        NVBAK_LOGE("Crete the map file and the data file fail\n");
        return false;
    }
    if(stat(g_pcNVM_AllMap,&st)<0) {
        NVBAK_LOGE("Error MapFile stat \n");
        return false;
    }
    iMapFileSize=st.st_size;
    if(stat(g_pcNVM_AllFile,&st)<0) {
        NVBAK_LOGE("Error DatFile stat \n");
        return false;
    }
    iDatFileSize=st.st_size;
    NVBAK_LOG("info:mapFileSize:%d,datFileSize:%d\n",iMapFileSize,iDatFileSize);

    if(!NVM_GetDeviceInfo(cMtdDevName,&info)) {
        NVBAK_LOGE("FileOp_BackupToBinRegion_All get device info error\r\n");
        free(bBadBlockBitMap);
        return false;
    }

    NVBAK_LOG("mtdDevName:%s \n",cMtdDevName);
    fd=open(cMtdDevName,O_RDWR|O_SYNC);
    if(fd<0) {
        NVBAK_LOGE("mtd open error\r\n");
        free(bBadBlockBitMap);
        return false;
    }

    if(!FileOp_CreateBinRegionBadBlockBitMap()) {
        NVBAK_LOGE("Error create Badblock Bitmap \n");
        return false;
    }

    iWriteSize=info.writesize;
    iBlockSize=info.erasesize;
    iPartitionSize=info.size;
	
    iFileDesc_file = open(g_pcNVM_AllFile , O_RDWR|O_SYNC);
    iFileDesc_map = open(g_pcNVM_AllMap, O_RDWR|O_SYNC);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        NVBAK_LOGE("cannot open file data\n");
        if(iFileDesc_map != INVALID_HANDLE_VALUE)
            close(iFileDesc_map);
        close(fd);
        free(bBadBlockBitMap);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        NVBAK_LOGE("cannot open map data\n");
        close(iFileDesc_file);
        close(fd);
        free(bBadBlockBitMap);
        return false;
    }

    if((iMapFileSize+3*sizeof(unsigned int)) % iWriteSize != 0)
        iMemSize=(((iMapFileSize+3*sizeof(unsigned int))/iWriteSize)+1)*iWriteSize;//mapfile size, datfile size, cleanboot flag
    else
        iMemSize=iMapFileSize+3*sizeof(unsigned int);//mapfile size, datfile size, cleanboot flag
    //iMemSize=iBlockSize;
    if(iMemSize > iBlockSize) {
        NVBAK_LOGE("MapFile size is biger than a Block Size\r\n");
        close(fd);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    tempBuffer=(char*)malloc(iMemSize);
    if(tempBuffer==NULL) {
        NVBAK_LOGE("memory malloc error\r\n");
        close(fd);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    memset(tempBuffer,0xFF,iMemSize);
    memcpy(tempBuffer,&iMapFileSize,sizeof(unsigned int));
    memcpy(tempBuffer+sizeof(unsigned int),&iDatFileSize,sizeof(unsigned int));

    iResult = read(iFileDesc_map,tempBuffer+3*sizeof(unsigned int),iMapFileSize);
    if(iResult != (int)iMapFileSize) {
        NVBAK_LOGE("map file read error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    for(i=0; i<NvRamBlockNum; i++) {
        if(bBadBlockBitMap[i]==0) {
            pos=i;
            break;
        }
    }
    NVBAK_LOG("pos for mapfile:%d,i:%d\n",pos,i);
    if(i==NvRamBlockNum) {
        NVBAK_LOGE("there are not enough good blocks for backup nvram map file\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    FileOp_Erase_Device(&info, fd, pos * iBlockSize, iBlockSize);
    lseek(fd,pos*iBlockSize,SEEK_SET);
    iResult = write(fd,tempBuffer,iMemSize);
    NVBAK_LOG("[NVRAM Backup]:map file write :%d\n",iResult);
    if(iResult != (int)iMemSize) {
        NVBAK_LOGE("map file write error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    //check the iResult of write
    if(lseek(fd,pos*iBlockSize,SEEK_SET) < 0) {
        NVBAK_LOG("[NVRAM Backup]:lseek error :%d\n",errno);
        //return false;
    }

    i=0;
    iResult = read(fd,&i,sizeof(unsigned int));
    if(iResult < 0||(i != (int)iMapFileSize)) {
        NVBAK_LOGE("check map file write error:%x,iMapfileSize:%d,iResult:%d\n",i,iMapFileSize,iResult);
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }

    for(j=(NvRamBlockNum-1); j>pos; j--) {
        if(bBadBlockBitMap[j]==0) {
            break;
        }
    }
    NVBAK_LOG("pos for bitmap:%d\n",j);
    if(j==pos) {
        NVBAK_LOGE("there is no space for bitmap in nand\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    //write the map file and bitmap file into the last available block of nand
    BitMapBuffer=(char*)malloc(iBlockSize);
    if(BitMapBuffer == NULL) {
        NVBAK_LOGE("malloc memory BitMapBuffer error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    memset(BitMapBuffer,0xFF,iBlockSize);
    memcpy(BitMapBuffer,bBadBlockBitMap,NvRamBlockNum*sizeof(char));
    memcpy(BitMapBuffer+NvRamBlockNum*sizeof(char),bBadBlockBitMap,NvRamBlockNum*sizeof(char));
    memcpy(BitMapBuffer+2*NvRamBlockNum*sizeof(char),bBadBlockBitMap,NvRamBlockNum*sizeof(char));
    memcpy(BitMapBuffer+3*NvRamBlockNum*sizeof(char),&iBitmapFlag,sizeof(unsigned int));
    memcpy(BitMapBuffer+3*NvRamBlockNum*sizeof(char)+sizeof(unsigned int),tempBuffer,iMemSize);

    FileOp_Erase_Device(&info, fd, j * iBlockSize, iBlockSize);
    lseek(fd,j*iBlockSize,SEEK_SET);
    iResult = write(fd,BitMapBuffer,iBlockSize);
    if(iResult != (int)iBlockSize) {
        NVBAK_LOGE("bimap file write error\r\n");
        close(fd);
        free(tempBuffer);
        free(BitMapBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    //check the iResult of write the bitmap
    char *tempBitmap1 = NULL;
    char *tempBitmap2 = NULL;
    tempBitmap1=(char *)malloc(NvRamBlockNum);
    if(tempBitmap1 == NULL) {
        NVBAK_LOGE("malloc tempBitmap1 Fail!!\r\n");
        close(fd);
        free(tempBuffer);
        free(BitMapBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    tempBitmap2=(char *)malloc(NvRamBlockNum);
    if(tempBitmap2 == NULL) {
        NVBAK_LOGE("malloc tempBitmap2 Fail!!\r\n");
        close(fd);
        free(tempBuffer);
        free(BitMapBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        free(tempBitmap1);
        return false;
    }
    lseek(fd,j*iBlockSize,SEEK_SET);
    iResult = read(fd,tempBitmap1,NvRamBlockNum*sizeof(char));
    iResult = read(fd,tempBitmap2,NvRamBlockNum*sizeof(char));
    for(i=0; i<NvRamBlockNum; i++) {
        if(tempBitmap1[i]!=tempBitmap2[i]) {
            NVBAK_LOG("check bimap file write error\r\n");
            close(fd);
            free(tempBuffer);
            free(BitMapBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(bBadBlockBitMap);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
    }
    free(BitMapBuffer);
    free(tempBuffer);
    free(tempBitmap1);
    free(tempBitmap2);
    tempBuffer=NULL;

    if(iDatFileSize%iBlockSize != 0)
        iMemSize=((iDatFileSize/iBlockSize)+1)*iBlockSize;
    else
        iMemSize=iDatFileSize;
    tempBuffer=(char*)malloc(iMemSize);
    if(tempBuffer==NULL) {
        NVBAK_LOGE("memory malloc error\r\n");
        close(fd);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    memset(tempBuffer,0xFF,iMemSize);
    iResult = read(iFileDesc_file,tempBuffer,iDatFileSize);
    if(iResult != (int)iDatFileSize) {
        NVBAK_LOGE("dat file read error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    }
    for(i=pos+1; i<j; i++) {
        if(bBadBlockBitMap[i]==0) {
            pos=i;
            break;
        }
    }
    if(i==j) {
        NVBAK_LOGE("there are not enough good blocks for backup nvram data file\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(bBadBlockBitMap);
        return false;
    } else {
        for(i; i<j; i++) {
            if(bBadBlockBitMap[i]==0)
                iFreeBlockNum++;
        }
        if((iFreeBlockNum*iBlockSize)<(iMemSize)) {
            NVBAK_LOGE("there are not enough good blocks for backup nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(bBadBlockBitMap);
            return false;
        }
    }
    NVBAK_LOG("pos for data file:%d\n",pos);
    iWriteTime=iMemSize/iBlockSize;
    int iAlreadyWrite=0;
    int bitmappos=j;
    NVBAK_LOG("writeTime:%d\n",iWriteTime);

    for(i=0; i<iWriteTime; i++) {
        FileOp_Erase_Device(&info, fd, pos * iBlockSize, iBlockSize);
        //NVBAK_LOG("FileOp_Erase_Device:%d,iBlockSize %d, pos:%d\n",pos * iBlockSize,iBlockSize,pos);
        iResult = lseek(fd,pos*iBlockSize,SEEK_SET);
        if(iResult != (int)(pos*iBlockSize)) {
            NVBAK_LOGE("dat file lseek error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(bBadBlockBitMap);
            return false;
        }
        iResult = write(fd,tempBuffer+i*iBlockSize,iBlockSize);
        if(iResult != (int)iBlockSize) {
            NVBAK_LOGE("dat file write error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(bBadBlockBitMap);
            return false;
        }
        iAlreadyWrite++;
        if(iAlreadyWrite==iWriteTime)
            break;
        for(j=pos+1; j<bitmappos; j++) {
            if(bBadBlockBitMap[j]==0) {
                pos=j;
                break;
            }
        }
        if(j>=bitmappos) {
            NVBAK_LOGE("there are not enough good blocks for write nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(bBadBlockBitMap);
            return false;
        }
    }
    NVBAK_LOG("end:%d\n",pos);
    free(bBadBlockBitMap);
    close(fd);
    free(tempBuffer);
    close(iFileDesc_file);
    close(iFileDesc_map);

    if(-1 == chown(g_pcNVM_AllFile , 1000/*system*/, 100/*users*/)) {
        NVBAK_LOG("chown for AllFile fail: %s\n", (char*)strerror(errno));
        //  return false;
    }

    if(-1 == chown(g_pcNVM_AllMap, 1000/*system*/, 100/*users*/)) {
        NVBAK_LOG("chown for AllMap fail: %s\n", (char*)strerror(errno));
        //   return false;
    }

    NVBAK_LOG("Leave BackupToBinRegion_all\n");

    sync();

    if (nvram_read_back_feature) {
        NVBAK_LOG("Enter Check Backup\n");
        if(!FileOp_RestoreFromBinRegion_ToFile()) {
            NVBAK_LOGE(" FileOp_RestoreFromBinRegion_ToFile fail\n");
            return false;
        }
        if(!FileOp_CheckBackUpResult()) {
            NVBAK_LOGE(" FileOp_CheckBackUpResult fail\n");
            return false;
        }
        NVBAK_LOG("Leave Check Backup\n");
    }
    return true;
}

bool FileOp_RestoreFromBinRegion(bool bCleanBoot)
{
    int iFileDesc_file, iFileDesc_map, fd, iResult,iBlockNum;
    unsigned int iMapFileSize,iDatFileSize,iBlockSize;
    bool bRet = true;
    char cMtdDevName[64] = {0};
    char *tempBuffer=NULL;
    char *tempBitmap1 = NULL;
    char *tempBitmap2 = NULL;
    int i,j,pos=0,flag=0;
    bool bSuccessFound = false;
    struct mtd_info_user info;

    NVBAK_LOG("FileOp_RestoreFromBinRegion\r\n");
    NVM_Init();

    memset(cMtdDevName, 0, sizeof cMtdDevName);
    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE);

    NVBAK_LOG("mtdDevName:%s\n",cMtdDevName);

    if(!NVM_GetDeviceInfo(cMtdDevName,&info)) {
        NVBAK_LOG("FileOp_CreateBinRegionBadBlockBitMap get device info error\r\n");
        return false;
    }
    fd=open(cMtdDevName,O_RDWR|O_SYNC);
    if(fd<0) {
        NVBAK_LOG("mtd open error\r\n");
        return false;
    }

    iBlockSize=info.erasesize;
    BinRegionBlockTotalNum = info.size/iBlockSize - nvram_platform_log_block - nvram_platform_resv_block;
    NvRamBlockNum=BinRegionBlockTotalNum;
    NVRAM_LOG("BinRegionBlockTotalNum %d , NvRamBlockNum=%d\r\n",BinRegionBlockTotalNum,NvRamBlockNum);

    tempBuffer=(char*)malloc(iBlockSize);
    if(tempBuffer==NULL) {
        NVBAK_LOG("memory malloc error\r\n");
        close(fd);
        return false;
    }
    iBlockNum=NvRamBlockNum;
    tempBitmap1=(char *)malloc(NvRamBlockNum);
    if(tempBitmap1 == NULL) {
        NVBAK_LOG("malloc tempBitmap1 Fail!!\r\n");
        free(tempBuffer);
        close(fd);
        return false;
    }
    tempBitmap2=(char *)malloc(NvRamBlockNum);
    if(tempBitmap2 == NULL) {
        NVBAK_LOG("malloc tempBitmap2 Fail!!\r\n");
        free(tempBuffer);
        close(fd);
        free(tempBitmap1);
        return false;
    }

    int iBitmapFlag=0;
    while(iBlockNum>0) {
        flag=0;
        iBlockNum--;
        NVBAK_LOG("blockNum:%d\n",iBlockNum);
        lseek(fd,iBlockNum*iBlockSize,SEEK_SET);
        iResult = read(fd,tempBuffer,iBlockSize);
        //NVBAK_LOG("read:%d\n",iResult);
        if(iResult <= 0) {
            NVBAK_LOG("read size error\r\n");
            close(fd);
            free(tempBuffer);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
        memcpy(tempBitmap1,tempBuffer,NvRamBlockNum*sizeof(char));
        memcpy(tempBitmap2,tempBuffer+NvRamBlockNum*sizeof(char),NvRamBlockNum*sizeof(char));
        for(i=0; i<NvRamBlockNum; i++) {
            if(tempBitmap1[i]!=tempBitmap2[i]) {
                NVBAK_LOG("1i:%d,1:%d,2:%d\n",i,tempBitmap1[i],tempBitmap2[i]);
                flag = 1;
                break;
            }
        }
        if(flag)
            continue;
        memcpy(tempBitmap2,tempBuffer+2*NvRamBlockNum*sizeof(char),NvRamBlockNum*sizeof(char));
        for(i=0; i<NvRamBlockNum; i++) {
            if(tempBitmap1[i]!=tempBitmap2[i]) {
                NVBAK_LOG("2i:%d,1:%d,2:%d\n",i,tempBitmap1[i],tempBitmap2[i]);
                flag = 1;
                break;
            }
        }
        if(flag)
            continue;
        memcpy(&iBitmapFlag,tempBuffer+3*NvRamBlockNum*sizeof(char),sizeof(unsigned int));
        if(iBitmapFlag!=(int)BitmapFlag) {
            NVBAK_LOG("bitMapFlag:0x%x:0x%x\n",iBitmapFlag,BitmapFlag);
            continue;
        }
        bSuccessFound=true;
        break;
    }
    if(!bSuccessFound) {
        NVBAK_LOG("can not find block bit map\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    //for(i=0; i<NvRamBlockNum; i++) {
    //    NVBAK_LOG("[NVRAM_Bitmap]:Block%d,%d\n",i,tempBitmap1[i]);
    //}
    for(i=0; i<iBlockNum; i++) {
        if(tempBitmap1[i]==0) {
            pos=i;
            break;
        }
    }
    if(i==iBlockNum) {
        NVBAK_LOG("can not find map file\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    lseek(fd,pos*iBlockSize,SEEK_SET);
    iResult = read(fd,tempBuffer,iBlockSize);
    NVBAK_LOG("map file size:%d\n",iResult);
    if(iResult <= 0) {
        NVBAK_LOG("read size error\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    iMapFileSize=*((unsigned int*)tempBuffer);
    iDatFileSize=*((unsigned int*)(tempBuffer+4));

    NVBAK_LOG("map file:%d,dat file:%d\n",iMapFileSize,iDatFileSize);
restorefiles:
    iFileDesc_file = open(g_pcNVM_AllFile , O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    iFileDesc_map = open(g_pcNVM_AllMap, O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        NVBAK_LOG(" cannot open file data\n");
        if(iFileDesc_map != INVALID_HANDLE_VALUE)
            close(iFileDesc_map);
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        NVBAK_LOG("cannot open map data\n");
        close(iFileDesc_file);
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    iResult = write(iFileDesc_map,tempBuffer+3*sizeof(unsigned int),iMapFileSize);
    if(iResult != (int)iMapFileSize) {
        NVBAK_LOG("map file write error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    free(tempBuffer);
    tempBuffer=NULL;
    if(iDatFileSize%iBlockSize != 0)
        tempBuffer=(char*)malloc((iDatFileSize/iBlockSize+1)*iBlockSize);
    else
        tempBuffer=(char*)malloc(iDatFileSize);
    if(tempBuffer==NULL) {
        NVBAK_LOG("memory malloc error\r\n");
        close(fd);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    int iFreeBlockNum=0;
    for(i=pos+1; i<iBlockNum; i++) {
        if(tempBitmap1[i]==0) {
            pos=i;
            break;
        }
    }
    if(i==iBlockNum) {
        NVBAK_LOG("there are not enough good blocks for read nvram data file\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    } else {
        for(i; i<iBlockNum; i++) {
            if(tempBitmap1[i]==0)
                iFreeBlockNum++;
        }
        if((iFreeBlockNum*iBlockSize)<(iDatFileSize)) {
            NVBAK_LOG("there are not enough good blocks for read  nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
    }
    int iReadTime=0;
    if(iDatFileSize%iBlockSize != 0)
        iReadTime=iDatFileSize/iBlockSize+1;
    else
        iReadTime=iDatFileSize/iBlockSize;
    int iAlreadyRead=0;
    NVBAK_LOG("dat file read begin:%d\n",pos);
    for(i=0; i<iReadTime; i++) {
        iResult = lseek(fd,pos*iBlockSize,SEEK_SET);
        if(iResult != pos*(int)iBlockSize) {
            NVBAK_LOG("binregion lseek error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }

        iResult = read(fd,tempBuffer+i*iBlockSize,iBlockSize);
        NVBAK_LOG("dat file read size:%d\n",iResult);
        if(iResult !=(int)iBlockSize) {
            NVBAK_LOG("bin region read error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
        iAlreadyRead++;
        if(iAlreadyRead==iReadTime)
            break;
        for(j=pos+1; j<iBlockNum; j++) {
            if(tempBitmap1[j]==0) {
                pos=j;
                break;
            }
        }
        if(j>=iBlockNum) {
            NVBAK_LOG("there are not enough good blocks to read nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
    }
    NVBAK_LOG("dat file read end:%d\n",pos);
    iResult = write(iFileDesc_file,tempBuffer,iDatFileSize);
    if(iResult != (int)iDatFileSize) {
        NVBAK_LOG("dat file write error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    close(fd);
    free(tempBuffer);
    close(iFileDesc_file);
    close(iFileDesc_map);
    free(tempBitmap1);
    free(tempBitmap2);

    if(bCleanBoot) {
        bRet = FileOp_RestoreData_All();
    } else {
        bRet = FileOp_RestoreFromFiles(APBOOT);
    }
    return bRet;

}

static void getAllFileAndAllMapSize(unsigned int *mapFileSize,unsigned int *datFileSize)
{
    char cMtdDevName[64] = {0};

    if(mapFileSize == NULL || datFileSize == NULL) {
        return;
    }

    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE);

    int fd=open(cMtdDevName,O_RDONLY);
    if(fd<0) {
        NVBAK_LOG("mtd open error\r\n");
        return;
    }

    if(sizeof(unsigned int) != read(fd,mapFileSize,sizeof(unsigned int))) {
        NVBAK_LOG("mtd read error\r\n");
        return;
    }

    if(sizeof(unsigned int) != read(fd,datFileSize,sizeof(unsigned int))) {
        NVBAK_LOG("mtd read error\r\n");
        return;
    }
}

bool FileOp_RecoveryData()
{
    int iResult;
    int iResultAllFile;
    int iResultAllMap;
    bool bCleanBoot;
    int iFileDesc_Flag;
    struct stat statbuf;
    struct stat statbufAllFile;
    struct stat statbufAllMap;
    unsigned int iMapFileSizeFromBinRegion = 0;
    unsigned int iDatFileSizeFromBinRegion = 0;
    unsigned int iCompleteFlag;

    NVM_Init();

    iResult = stat(g_pcNVM_Flag,&statbuf);
    iResultAllFile = stat(g_pcNVM_AllFile,&statbufAllFile);
    iResultAllMap = stat(g_pcNVM_AllMap,&statbufAllMap);

    if(0 == iResult) {

        iFileDesc_Flag = open(g_pcNVM_Flag, O_RDONLY);
        if(-1 == iFileDesc_Flag) {
            NVBAK_LOG("Open Restore Flag file failed,go to restore!!!");
            goto Recovery;
        }
        iResult = read(iFileDesc_Flag,&iCompleteFlag,sizeof(unsigned int));
        if(iResult != sizeof(unsigned int)) {
            NVBAK_LOG("Read retore Flag file failed,go to restore!!!");
            close(iFileDesc_Flag);
            goto Recovery;
        }
        if(0x12345678 != iCompleteFlag) {
            NVBAK_LOG("Restore Flag is error, go to retore!!!");
            close(iFileDesc_Flag);
            goto Recovery;
        }
        close(iFileDesc_Flag);
    }

    if(iResultAllFile == 0 || iResultAllMap == 0) {
        getAllFileAndAllMapSize(&iMapFileSizeFromBinRegion,&iDatFileSizeFromBinRegion);
        NVBAK_LOG("enter clean boot %u %u\n",iMapFileSizeFromBinRegion,iDatFileSizeFromBinRegion);
    }

    if(iResult == -1 || iResultAllFile == -1 || iResultAllMap == -1 ||
       (iResultAllFile == 0 && statbufAllFile.st_size == 0) ||
       (iResultAllMap == 0 && statbufAllMap.st_size == 0)   ||
       (iResultAllMap == 0 && statbufAllMap.st_size != iMapFileSizeFromBinRegion) ||
       (iResultAllFile == 0 && statbufAllFile.st_size != iDatFileSizeFromBinRegion)) {
Recovery:
        bCleanBoot = true;
        NVBAK_LOG("enter clean boot +++\n");

        if(!FileOp_RestoreFromBinRegion(bCleanBoot)) {
            NVBAK_LOGE("FileOp_RecoveryData:recovery data fail\n");
            return false;
        }

        iFileDesc_Flag = open(g_pcNVM_Flag, O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
        if(iFileDesc_Flag==-1) {
            NVBAK_LOGE("FileOp_RecoveryData:set clean file fail\n");
            return false;
        }
        unsigned int iCompleteFlag=0x12345678;
        write(iFileDesc_Flag,&iCompleteFlag,sizeof(unsigned int));
        close(iFileDesc_Flag);
    }
    return true;
}

static bool FileOp_MakeFile(const char* src, unsigned int* iFileNum, int* FileSize)
{
    unsigned char acBuffer[1024];
    int iSrc, iFileSize, iWriteSize, iSize; //FileSize;
    int iFileDesc_file, iFileDesc_map;
    File_Title *FileInfo = NULL;
    bool bRet = false;
    struct stat statbuf;
    off_t iResult = 0;

    NVBAK_LOG("FileOp_MakeFile: Handle the file (%s)\n", src);
    if(iFileNum == NULL) {
        NVBAK_LOG("FileOp_MakeFile:Input iFileNum Null pointer!\n");
        return false;
    }
    if(FileSize == NULL) {
        NVBAK_LOG("FileOp_MakeFile:Input FileSize Null pointer!\n");
        return false;
    }

    iFileDesc_file = open(g_pcNVM_AllFile , O_RDWR|O_SYNC);
    iFileDesc_map = open(g_pcNVM_AllMap, O_RDWR|O_SYNC);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open/create file data\n");
        if(iFileDesc_map != INVALID_HANDLE_VALUE)
            close(iFileDesc_map);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open/create map data\n");
        close(iFileDesc_file);
        return false;
    }

    /* Seek to the file end */
    iResult = lseek(iFileDesc_file, 0, SEEK_END);
    if (iResult == (off_t) -1) {
        NVBAK_LOG("iFileDesc_file seek error !\n");
        close(iFileDesc_file);
        close(iFileDesc_map);
        return false;
    }

    iResult = lseek(iFileDesc_map, 0, SEEK_END);

    if (iResult == (off_t) -1) {
        NVBAK_LOG("iFileDesc_map seek error !\n");
        close(iFileDesc_file);
        close(iFileDesc_map);
        return false;
    }

    iSrc = open(src, O_RDONLY);

    if (-1 == iSrc) {
        NVBAK_LOGE("open %s, err %d", src, errno);
        goto EXIT;
    }

    fstat(iSrc, &statbuf);
    *FileSize = iFileSize = (int)statbuf.st_size;

    NVBAK_LOG("File size is (%d)\n", iFileSize);

    while(iFileSize > 0) {
        iWriteSize = MIN(iFileSize, (int)sizeof(acBuffer));
        iSize = (int)read(iSrc, acBuffer, iWriteSize);
        if (iSize != iWriteSize) {
            goto EXIT;
        }
        iSize = (int)write(iFileDesc_file, acBuffer, iWriteSize);
        if (iSize != iWriteSize) {
            goto EXIT;
        }
        iFileSize -= iWriteSize;
    }
    /* malloc the buffer of title buf */
    FileInfo = (File_Title *)malloc(sizeof(File_Title));
    if(FileInfo == NULL) {
        NVBAK_LOG("FileOp_MakeFile malloc memory for FileInfo failed!!\n");
        close(iFileDesc_file);
        close(iFileDesc_map);
        return false;
    }
    memset(FileInfo, 0, sizeof(File_Title));

    /* Koshi: write map file */
    FileInfo->Filesize = *FileSize;
    FileInfo->NameSize = 7;
    *iFileNum+=1;
    pthread_mutex_lock(&gFileStartAddrlock);
    FileInfo->FielStartAddr = gFileStartAddr;
    memcpy(FileInfo->cFileName, src, MAX_NAMESIZE);
    write(iFileDesc_map, FileInfo, sizeof(File_Title));
    NVBAK_LOG("FileInfo: Filenum %u (addr - %d / size - %d) \n", *iFileNum, FileInfo->FielStartAddr, FileInfo->Filesize);
    gFileStartAddr += FileInfo->Filesize;
    pthread_mutex_unlock(&gFileStartAddrlock);

    bRet = true;

EXIT:
    if (iSrc != -1) {
        close(iSrc);
    }
    if (iFileDesc_file != -1) {
        close(iFileDesc_file);
    }
    if (iFileDesc_map != -1) {
        close(iFileDesc_map);
    }
    free(FileInfo);
    return bRet;
}

bool FileOp_RestoreFromFiles(int eBackupType)
{
    int iFileDesc_file, iFileDesc_map, iFileDesc, iSize;
    int iFileTitleOffset = 0;
    short int iFileNum=0;
    char *buf;  /* content  */
    File_Title *FileInfo = NULL;
    off_t iResult;
    File_Title_Header FileTitleInfo;

    int fhs = sizeof(unsigned int)+2*sizeof(unsigned int)+sizeof(File_Title_Header);

    int fis = sizeof(File_Title);

    /* malloc the buffer of title buf */
    FileInfo = (File_Title *)malloc(sizeof(File_Title));
    if(FileInfo == NULL) {
        NVBAK_LOG("FileOp_MakeFile malloc mermory for FileInfo failed\n");
        return false;
    }
    memset(FileInfo, 0, sizeof(File_Title));

    iFileDesc_file = open(g_pcNVM_AllFile , O_RDWR|O_SYNC);
    iFileDesc_map = open(g_pcNVM_AllMap, O_RDWR|O_SYNC);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open file data\n");
        if(iFileDesc_map != INVALID_HANDLE_VALUE)
            close(iFileDesc_map);
        free(FileInfo);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open map data\n");
        close(iFileDesc_file);
        free(FileInfo);
        return false;
    }

    lseek(iFileDesc_map, sizeof(unsigned int)+2*sizeof(unsigned int), SEEK_SET);
    iSize = (int)read(iFileDesc_map, &FileTitleInfo, sizeof(File_Title_Header));

    switch (eBackupType) {
    case APBOOT:
        iFileNum = FileTitleInfo.iApBootNum;
        iFileTitleOffset = fhs;
        break;

    case APCLN:
        iFileNum = FileTitleInfo.iApCleanNum;
        iFileTitleOffset = fhs + (FileTitleInfo.iApBootNum)* fis;
        break;

    case ALL:
        break;
    default:
        close(iFileDesc_map);
        close(iFileDesc_file);
        free(FileInfo);
        return false;
        //break;
    }

    iResult = lseek(iFileDesc_map, iFileTitleOffset, SEEK_SET);

    while(iFileNum>0) {
        memset(FileInfo, 0, sizeof(File_Title));
        iSize = (int)read(iFileDesc_map, FileInfo, sizeof(File_Title));
        NVBAK_LOG("FileInfo: %s\n", FileInfo->cFileName);
        NVBAK_LOG("FileInfo: startaddr(%x) size(%x) .\n", FileInfo->FielStartAddr, FileInfo->Filesize);

        iFileDesc = open_file_with_dirs(FileInfo->cFileName, 0660);

        if(INVALID_HANDLE_VALUE == iFileDesc) {
            /* Error handling */
            NVBAK_LOG("FileOp_RestoreFromFiles cannot create %s\n", FileInfo->cFileName);

            //added
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }

        buf = (char *)malloc(FileInfo->Filesize);
        if(buf == NULL) {
            /* Error handling */
            NVBAK_LOG("FileOp_RestoreFromFiles malloc memory for buff faild\n");

            //added
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }

        //read the data and write to the file
        iResult = lseek(iFileDesc_file, FileInfo->FielStartAddr, SEEK_SET);
        if(iResult == -1) {
            NVBAK_LOG("lseek fail !");
            close(iFileDesc);
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }
        iSize = (int)read(iFileDesc_file, buf, FileInfo->Filesize);
        if(iSize!=FileInfo->Filesize) {
            NVBAK_LOG("read fail !");
            close(iFileDesc);
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }
        iSize = (int)write(iFileDesc, buf, FileInfo->Filesize);
        if(iSize!=FileInfo->Filesize) {
            NVBAK_LOG("write fail !");
            close(iFileDesc);
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }
        close(iFileDesc);
        free(buf);
        --iFileNum;
    }
    //while (--iFileNum);

    close(iFileDesc_map);
    close(iFileDesc_file);
    free(FileInfo);
    return true;
}

static unsigned int FileOp_ComputeCheckSum(void)
{
    int iFileDesc_file;
    unsigned int iFileSize;
    unsigned int ulCheckSum=0;
    int looptime;
    struct stat st;
    int i,flag;
    int iResult;
    int iLength=sizeof(unsigned int);
    unsigned int tempNum;

    if(stat(g_pcNVM_AllFile,&st)<0) {
        NVBAK_LOG("Error FileOp_ComputeCheckSum stat \n");
        return 0;
    }

    iFileSize=st.st_size;
    looptime=iFileSize/(sizeof(unsigned int));

    iFileDesc_file = open(g_pcNVM_AllFile , O_RDWR|O_SYNC);
    if(iFileDesc_file<0) {
        NVBAK_LOG("FileOp_ComputeCheckSum cannot open data file\n");
        return 0;
    }
    flag=1;
    for(i=0; i<looptime; i++) {
        iResult=read(iFileDesc_file, &tempNum, iLength);
        if(iResult!= iLength) {
            NVBAK_LOG("FileOp_ComputeCheckSum cannot read checksum data\n");
            close(iFileDesc_file);
            return 0;
        }
        if(flag) {
            ulCheckSum^=tempNum;
            flag=0;
        } else {
            ulCheckSum+=tempNum;
            flag=1;
        }
    }
    tempNum=0;
    iLength=iFileSize%(sizeof(unsigned int));
    iResult=read(iFileDesc_file, &tempNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_ComputeCheckSum cannot read last checksum data\n");
        close(iFileDesc_file);
        return 0;
    }
    ulCheckSum+=tempNum;
    //ulCheckSum^=gFileStartAddr;
    close(iFileDesc_file);
    return ulCheckSum;
}

static unsigned int FileOp_ComputeReadBackCheckSum(void)
{
    int iFileDesc_file;
    unsigned int iFileSize;
    unsigned int ulCheckSum=0;
    int looptime;
    struct stat st;
    int i,flag;
    int iResult;
    int iLength=sizeof(unsigned int);
    unsigned int tempNum;

    if(stat(g_pcNVM_AllFile_Check,&st)<0) {
        NVBAK_LOG("Error FileOp_ComputeCheckSum stat \n");
        return 0;
    }

    iFileSize=st.st_size;

    looptime=iFileSize/(sizeof(unsigned int));

    iFileDesc_file = open(g_pcNVM_AllFile_Check , O_RDWR|O_SYNC);
    if(iFileDesc_file<0) {
        NVBAK_LOG("FileOp_ComputeCheckSum cannot open data file\n");
        return 0;
    }
    flag=1;
    for(i=0; i<looptime; i++) {
        iResult=read(iFileDesc_file, &tempNum, iLength);
        if(iResult!= iLength) {
            NVBAK_LOG("FileOp_ComputeReadBackCheckSum cannot read checksum data\n");
            close(iFileDesc_file);
            return 0;
        }
        if(flag) {
            ulCheckSum^=tempNum;
            flag=0;
        } else {
            ulCheckSum+=tempNum;
            flag=1;
        }
    }
    tempNum=0;
    iLength=iFileSize%(sizeof(unsigned int));
    iResult=read(iFileDesc_file, &tempNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_ComputeReadBackCheckSum cannot read last checksum data\n");
        close(iFileDesc_file);
        return 0;
    }
    ulCheckSum+=tempNum;
    //ulCheckSum^=gFileStartAddr;
    close(iFileDesc_file);
    return ulCheckSum;
}

static BackupFileInfo stBackupFileInfo;
static bool FileOp_GetCheckSum(void)
{
    int iFileDesc_map;
    int iResult;
    int iLength=sizeof(unsigned int);

    iFileDesc_map = open(g_pcNVM_AllMap,O_RDONLY);
    if(iFileDesc_map<0) {
        NVBAK_LOG("FileOp_GetCheckSum cannot open/create map data\n");
        return false;
    }
    iResult=read(iFileDesc_map, &stBackupFileInfo.ulCheckSum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    iLength=sizeof(unsigned int);
    iResult=read(iFileDesc_map, &stBackupFileInfo.iCommonFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    iResult=read(iFileDesc_map, &stBackupFileInfo.iCustomFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    close(iFileDesc_map);

    return true;
}

static bool FileOp_SetCheckSum(unsigned int ulCheckSum)
{
    int iFileDesc_map=0;
    int iResult;
    int iLength=sizeof(unsigned int);
    unsigned int iOldCommonFileNum=g_i4CFG_File_Count;
    unsigned int iOldCustomFileNum=g_i4CFG_File_Custom_Count;

    pthread_mutex_lock(&gFileStartAddrlock);
    ulCheckSum^=gFileStartAddr;
    gFileStartAddr=0;
    pthread_mutex_unlock(&gFileStartAddrlock);
    iFileDesc_map = open(g_pcNVM_AllMap, O_WRONLY|O_SYNC);
    if(iFileDesc_map<0) {
        NVBAK_LOG("FileOp_SetCheckSum cannot open/create map data\n");
        return false;
    }
    iResult=write(iFileDesc_map, &ulCheckSum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_SetCheckSum cannot write checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    ///NVBAK_LOG("common file num:%d,custom file num:%d\n",iOldCommonFileNum,iOldCustomFileNum);
    iLength=sizeof(unsigned int);
    iResult=write(iFileDesc_map, &iOldCommonFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_SetCheckSum cannot write common file num data\n");
        close(iFileDesc_map);
        return false;
    }
    iResult=write(iFileDesc_map, &iOldCustomFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("FileOp_SetCheckSum cannot write custom file num data\n");
        close(iFileDesc_map);
        return false;
    }
    close(iFileDesc_map);
    return true;
}

static bool FileOp_CompareCheckSum(unsigned int ulCheckSum1,unsigned int ulCheckSum2)
{
    if(ulCheckSum1!=ulCheckSum2)
        return false;
    return true;
}

bool FileOp_BackupDataToFiles(int * iFileMask,bool bWorkForBinRegion)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    const char* lpSrcDirName = NULL;
    char acSrcPathName[MAX_NAMESIZE];
    unsigned int iFileNum = 0;
    int iFileDesc_map, FileSize=0, eBackupType,iFileDesc_file;
    bool bRet = true;
    unsigned int i=0;
    unsigned int ulCheckSum=0;
    unsigned int iOldCommonFileNum=0;
    unsigned int iOldCustomFileNum=0;
    bool bMask=true;
    int iMask[ALL];

    File_Title_Header FileTitleInfo;
    memset(&FileTitleInfo, 0, sizeof(File_Title_Header));

    if(iFileMask!=NULL)
        memcpy(iMask,iFileMask,(ALL*sizeof(int)));
    else {
        NVBAK_LOG("iFileMask is NULL\n");
        return false;
    }

    /* Create the map file */
    iFileDesc_map = open(g_pcNVM_AllMap, O_TRUNC|O_CREAT|O_WRONLY|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open/create map data\n");
        return false;
    }

    write(iFileDesc_map, &ulCheckSum, sizeof(unsigned int));
    write(iFileDesc_map, &iOldCommonFileNum,sizeof(unsigned int));
    write(iFileDesc_map, &iOldCustomFileNum, sizeof(unsigned int));
    /* Reserve the FileTitleInfo space */
    write(iFileDesc_map, &FileTitleInfo, sizeof(FileTitleInfo));

    close(iFileDesc_map);
    /* Create the data file */
    iFileDesc_file = open(g_pcNVM_AllFile , O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open/create data file\n");
        return false;
    }
    close(iFileDesc_file);

    for (eBackupType =APBOOT; eBackupType<ALL; eBackupType++) {
        if(iMask[eBackupType]==1)
            continue;

        switch (eBackupType) {
        case APBOOT:
            //NVBAK_LOG("FileOp_BackupData_All APBOOT start !\n");
            lpSrcDirName = g_pcNVM_APRdebFile;
            break;

        case APCLN:
            //NVBAK_LOG("FileOp_BackupData_All APCLN start !\n");
            lpSrcDirName = g_pcNVM_APCalFile;
            break;

        case ALL:
        default:
            return false;
            break;
        }

        /* Check the folder in NvRam is exists */
        NVBAK_LOG("open dir %s !\n", lpSrcDirName);

        dir = opendir(lpSrcDirName);

        if (dir == NULL) {
            NVBAK_LOG("FileOp_BackupData open dir: %s ,this dir not exist!\n", lpSrcDirName);
            continue;
            //return false;
        }

        while(1) {
            entry = readdir(dir);

            if (entry == NULL) {
                break;
            }

            lstat(entry->d_name, &statbuf);

            if (!strcmp(entry->d_name, ".")|| !strcmp(entry->d_name, "..")) {
                continue;
            }

            sprintf(acSrcPathName, "%s/%s", lpSrcDirName, entry->d_name);
            //NVBAK_LOG("FileOp_BackupData: (%s)\n", acSrcPathName);
            if(bWorkForBinRegion) {
                if(eBackupType ==APBOOT||eBackupType ==APCLN) {
                    bMask=true;
                    for(i=0; i<g_Backup_File_Count; i++) {
                        if(0==strcmp(entry->d_name,aBackupToBinRegion[i].cFileName) ) {
                            bMask=false;
                            break;
                        }
                    }

                    if(bMask==true)
                        continue;
                }
            }

            if (!FileOp_MakeFile(acSrcPathName, &iFileNum, &FileSize)) {
                NVBAK_LOG("ERROR FileOp_BackupData (%s)\n", acSrcPathName);
                bRet = false;
                continue;
            }
        }
        closedir(dir);
        //NVBAK_LOG("FileSize is (%d) !\n", FileSize);
        //NVBAK_LOG("FileNum is (%u) !\n", iFileNum);
        FileTitleInfo.iFileBufLen += FileSize;
        //NVBAK_LOG("iFileBufLen is (%d) !\n", FileTitleInfo.iFileBufLen);

        switch (eBackupType) {
        case APBOOT:
            FileTitleInfo.iApBootNum = iFileNum;
            //NVBAK_LOG("APBOOT num (%d) !\n", FileTitleInfo.iApBootNum);
            break;

        case APCLN:
            FileTitleInfo.iApCleanNum = iFileNum;
            //NVBAK_LOG("APCLN num (%d) !\n", FileTitleInfo.iApCleanNum);
            break;

        case ALL:
        default:
            return false;
            break;
        }
        iFileNum = 0;
        FileSize=0;
    }

    iFileDesc_map = open(g_pcNVM_AllMap, O_WRONLY|O_SYNC);

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        /* Error handling */
        NVBAK_LOG("FileOp_MakeFile cannot open/create map data\n");
        return false;
    }
    write(iFileDesc_map, &ulCheckSum, sizeof(unsigned int));
    write(iFileDesc_map, &iOldCommonFileNum, sizeof(unsigned int));
    write(iFileDesc_map, &iOldCustomFileNum, sizeof(unsigned int));
    write(iFileDesc_map, &FileTitleInfo, sizeof(FileTitleInfo));

    close(iFileDesc_map);
    iFileDesc_map = open(g_pcNVM_AllMap, O_RDONLY);

    memset(&FileTitleInfo, 0, sizeof(File_Title_Header));
    lseek(iFileDesc_map, sizeof(unsigned int)+2*sizeof(unsigned int), SEEK_SET);
    read(iFileDesc_map, &FileTitleInfo, sizeof(FileTitleInfo));
    close(iFileDesc_map);

    ulCheckSum=FileOp_ComputeCheckSum();
    //NVBAK_LOG("FileOp_ComputeCheckSum:%x\n",ulCheckSum);
    if(!FileOp_SetCheckSum(ulCheckSum)) {
        NVBAK_LOG("FileOp_SetCheckSum Fail !\n");
        return false;
    }
    NVBAK_LOG("FileOp_BackupData_All end !\n");
    return bRet;
}

bool FileOp_RestoreData_All(void)
{
    bool bRet = true;
    int eBackupType;
    unsigned int ulSavedCheckSum;
    unsigned int ulCheckSum;
    struct stat st;
    unsigned int iFileSize;
    int error_flag=0;

    NVBAK_LOG("[FileOp_RestoreData_All] start !\n");

    if(!FileOp_GetCheckSum()) {
        NVBAK_LOG("[FileOp_RestoreData_All] GetCheckSum Fail !\n");
        return false;
    }
    ulSavedCheckSum=stBackupFileInfo.ulCheckSum;
    ulCheckSum=FileOp_ComputeCheckSum();
    NVBAK_LOG("checkSum:%x\n",ulCheckSum);
    if(stat(g_pcNVM_AllFile,&st)<0) {
        NVBAK_LOG("Error FileOp_RestoreData_All stat\n");
        return false;
    }
    iFileSize=st.st_size;
    NVBAK_LOG("fileSize:%d\n",iFileSize);
    ulCheckSum^=iFileSize;

    NVBAK_LOG("FileOp_CheckSum:%x,%x\n",ulSavedCheckSum,ulCheckSum);
    if(!FileOp_CompareCheckSum(ulSavedCheckSum,ulCheckSum)) {
        NVBAK_LOG("check sum not match!");
        return false;
    }
    for (eBackupType =APBOOT; eBackupType<ALL; eBackupType++) {
        bRet = FileOp_RestoreFromFiles(eBackupType);
        if(bRet == false) {
            NVBAK_LOG("RestoreFromFiles Error!! The No.%d file\n",eBackupType);
            error_flag=1;
        }
    }
    if(error_flag == 0) {
        NVBAK_LOG("[FileOp_RestoreData_All] end !\n");
        return true;
    } else {
        NVBAK_LOG("[FileOp_RestoreData_All] some file restore failed !\n");
        return false;
    }

}

bool FileOp_RestoreFromBinRegion_ToFile()
{
    int iFileDesc_file, iFileDesc_map, fd, iResult,iBlockNum;
    unsigned int iMapFileSize,iDatFileSize,iBlockSize,iPartitionSize;
    bool bRet=true;
    char cMtdDevName[64] = {0};
    char *tempBuffer=NULL;
    char *tempBitmap1 = NULL;
    char *tempBitmap2 = NULL;
    int i,j,pos=0,flag=0;
    bool bSuccessFound = false;
    struct mtd_info_user info;
    NVM_Init();
    NVBAK_LOG("Enter FileOp_RestoreFromBinRegion_ToFile\n");

    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName,NVRAM_BACKUP_DEVICE);

    NVBAK_LOG("mtdDevName:%s\n",cMtdDevName);
    bRet = NVM_GetDeviceInfo(cMtdDevName,&info);
    if(bRet == false) {
        NVBAK_LOG("FileOp_RestoreFromBinRegion_ToFile get device info error\r\n");
        return false;
    }
    fd = open(cMtdDevName,O_RDWR|O_SYNC);
    if(fd < 0) {
        NVBAK_LOG("mtd open error\r\n");
        return false;
    }

    iBlockSize=info.erasesize;
    iPartitionSize=info.size;
    BinRegionBlockTotalNum=info.size/iBlockSize - nvram_platform_log_block - nvram_platform_resv_block;

    NvRamBlockNum=BinRegionBlockTotalNum;
    tempBuffer=(char*)malloc(iBlockSize);
    if(tempBuffer==NULL) {
        NVBAK_LOG("memory malloc error\r\n");
        close(fd);
        return false;
    }
    iBlockNum=NvRamBlockNum;
    NVBAK_LOG("blockSize:%d\n",iBlockSize);
    NVBAK_LOG("partitionSize:%d\n",iPartitionSize);
    NVBAK_LOG("BinRegionBlockTotalNum:%d\n",BinRegionBlockTotalNum);
    NVBAK_LOG("NvRamBlockNum:%d\n",NvRamBlockNum);
    NVBAK_LOG("blockNum:%d\n",iBlockNum);

    tempBitmap1=(char *)malloc(NvRamBlockNum);
    if(tempBitmap1 == NULL) {
        NVBAK_LOG("malloc tempBitmap1 Fail!!\r\n");
        free(tempBuffer);
        close(fd);
        return false;
    }
    tempBitmap2=(char *)malloc(NvRamBlockNum);
    if(tempBitmap2 == NULL) {
        NVBAK_LOG("malloc tempBitmap2 Fail!!\r\n");
        free(tempBuffer);
        close(fd);
        free(tempBitmap1);
        return false;
    }

    int iBitmapFlag=0;
    while(iBlockNum>0) {
        flag=0;
        iBlockNum--;
        NVBAK_LOG("iBlockNum:%d\n",iBlockNum);
        lseek(fd,iBlockNum*iBlockSize,SEEK_SET);
        iResult = read(fd,tempBuffer,iBlockSize);
        NVBAK_LOG("read:%d\n",iResult);
        if(iResult <= 0) {
            NVBAK_LOG("read size error\r\n");
            close(fd);
            free(tempBuffer);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
        memcpy(tempBitmap1,tempBuffer,NvRamBlockNum*sizeof(char));
        memcpy(tempBitmap2,tempBuffer+NvRamBlockNum*sizeof(char),NvRamBlockNum*sizeof(char));
        for(i=0; i<NvRamBlockNum; i++) {
            if(tempBitmap1[i]!=tempBitmap2[i]) {
                NVBAK_LOG("1i:%d,1:%d,2:%d\n",i,tempBitmap1[i],tempBitmap2[i]);
                flag = 1;
                break;
            }
        }
        if(flag)
            continue;
        memcpy(tempBitmap2,tempBuffer+2*NvRamBlockNum*sizeof(char),NvRamBlockNum*sizeof(char));
        for(i=0; i<NvRamBlockNum; i++) {
            if(tempBitmap1[i]!=tempBitmap2[i]) {
                NVBAK_LOG("2i:%d,1:%d,2:%d\n",i,tempBitmap1[i],tempBitmap2[i]);
                flag = 1;
                break;
            }
        }
        if(flag)
            continue;
        memcpy(&iBitmapFlag,tempBuffer+3*NvRamBlockNum*sizeof(char),sizeof(unsigned int));
        if(iBitmapFlag!=(int)BitmapFlag) {
            NVBAK_LOG("iBitMapFlag:%d,BitMapFlag:%d\n",iBitmapFlag,BitmapFlag);
            continue;
        }
        bSuccessFound=true;
        break;
    }
    if(!bSuccessFound) {
        NVBAK_LOG("can not find bad block bit map\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    for(i=0; i<iBlockNum; i++) {
        if(tempBitmap1[i]==0) {
            pos=i;
            break;
        }
    }
    if(i==iBlockNum) {
        NVBAK_LOG("can not find map file\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    lseek(fd,pos*iBlockSize,SEEK_SET);
    iResult=read(fd,tempBuffer,iBlockSize);
    NVBAK_LOG("map file read size:%d\n",iResult);
    if(iResult<=0) {
        NVBAK_LOG("read size error\r\n");
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    iMapFileSize=*((unsigned int*)tempBuffer);
    iDatFileSize=*((unsigned int*)(tempBuffer+4));

    NVBAK_LOG(" map file:%d,dat file:%d\n",iMapFileSize,iDatFileSize);

    iFileDesc_file = open(g_pcNVM_AllFile_Check, O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    iFileDesc_map = open(g_pcNVM_AllMap_Check,  O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        NVBAK_LOG(" cannot open file data\n");
        if(iFileDesc_map != INVALID_HANDLE_VALUE)
            close(iFileDesc_map);
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        NVBAK_LOG(" cannot open map data\n");
        close(iFileDesc_file);
        close(fd);
        free(tempBuffer);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    iResult=write(iFileDesc_map,tempBuffer+3*sizeof(unsigned int),iMapFileSize);
    if(iResult!=(int)iMapFileSize) {
        NVBAK_LOG("map file write error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }

    free(tempBuffer);
    tempBuffer=NULL;
    if(iDatFileSize%iBlockSize != 0)
        tempBuffer=(char*)malloc((iDatFileSize/iBlockSize+1)*iBlockSize);
    else
        tempBuffer=(char*)malloc(iDatFileSize);
    if(tempBuffer==NULL) {
        NVBAK_LOG("memory malloc error\r\n");
        close(fd);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    int iFreeBlockNum=0;
    for(i=pos+1; i<iBlockNum; i++) {
        if(tempBitmap1[i]==0) {
            pos=i;
            break;
        }
    }
    if(i==iBlockNum) {
        NVBAK_LOG("there are not enough good blocks for read nvram data file\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    } else {
        for(i; i<iBlockNum; i++) {
            if(tempBitmap1[i]==0)
                iFreeBlockNum++;
        }
        if((iFreeBlockNum*iBlockSize)<(iDatFileSize)) {
            NVBAK_LOG("there are not enough good blocks for read  nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
    }
    int iReadTime=0;
    if(iDatFileSize%iBlockSize != 0)
        iReadTime=iDatFileSize/iBlockSize+1;
    else
        iReadTime=iDatFileSize/iBlockSize;
    int iAlreadyRead=0;
    NVBAK_LOG("dat file read begin:%d\n",pos);
    for(i=0; i<iReadTime; i++) {
        iResult=lseek(fd,pos*iBlockSize,SEEK_SET);
        if(iResult!=pos*(int)iBlockSize) {
            NVBAK_LOG("binregion lseek error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }

        iResult=read(fd,tempBuffer+i*iBlockSize,iBlockSize);
        NVBAK_LOG("dat file read size:%d\n",iResult);
        if(iResult!=(int)iBlockSize) {
            NVBAK_LOG("bin region read error\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
        iAlreadyRead++;
        if(iAlreadyRead==iReadTime)
            break;
        for(j=pos+1; j<iBlockNum; j++) {
            if(tempBitmap1[j]==0) {
                pos=j;
                break;
            }
        }
        if(j>=iBlockNum) {
            NVBAK_LOG("there are not enough good blocks to read nvram data file\r\n");
            close(fd);
            free(tempBuffer);
            close(iFileDesc_file);
            close(iFileDesc_map);
            free(tempBitmap1);
            free(tempBitmap2);
            return false;
        }
    }
    NVBAK_LOG("dat file read end:%d\n",pos);
    iResult=write(iFileDesc_file,tempBuffer,iDatFileSize);
    if(iResult!=(int)iDatFileSize) {
        NVBAK_LOG("dat file write error\r\n");
        close(fd);
        free(tempBuffer);
        close(iFileDesc_file);
        close(iFileDesc_map);
        free(tempBitmap1);
        free(tempBitmap2);
        return false;
    }
    close(fd);
    free(tempBuffer);
    close(iFileDesc_file);
    close(iFileDesc_map);
    free(tempBitmap1);
    free(tempBitmap2);
    NVBAK_LOG("Leave FileOp_RestoreFromBinRegion_ToFile\n");
    return true;
}

bool FileOp_CheckBackUpResult()
{
    unsigned int ulSavedCheckSum;
    unsigned int ulCheckSum;
    struct stat st;
    unsigned int iFileSize;

    NVBAK_LOG(" Enter FileOp_CheckBackUpResult\n");

    if(!FileOp_GetCheckSum()) {
        NVBAK_LOG("[FileOp_GetCheckSum] GetCheckSum Fail !");
        return false;
    }
    ulSavedCheckSum=stBackupFileInfo.ulCheckSum;
    ulCheckSum=FileOp_ComputeReadBackCheckSum();
    NVBAK_LOG("ulSavedCheckSum:%d\n",ulSavedCheckSum);
    NVBAK_LOG("ulCheckSun:%d\n",ulCheckSum);
    if(stat(g_pcNVM_AllFile_Check,&st)<0) {
        NVBAK_LOG("Error FileOp_CheckBackUpResult stat \n");
        return false;
    }
    iFileSize=st.st_size;
    NVBAK_LOG("iFileSize:%d\n",iFileSize);
    ulCheckSum^=iFileSize;

    NVBAK_LOG("FileOp_CheckSum:%x,%x",ulSavedCheckSum,ulCheckSum);
    if(!FileOp_CompareCheckSum(ulSavedCheckSum,ulCheckSum)) {
        NVBAK_LOG("check sum not match!");
        return false;
    }
    NVBAK_LOG(" Leave FileOp_CheckBackUpResult\n");
    return true;
}//end of FileOp_CheckBackUpResult

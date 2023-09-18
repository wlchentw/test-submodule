/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2008
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
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/mount.h>
#include "libnvram_log.h"
#include "libnvram.h"
#include "libfile_op.h"
#include <mtd/mtd-abi.h>
#include <pthread.h>
#include <sys/file.h>
#define INVALID_HANDLE_VALUE    -1

typedef struct {
    unsigned int ulCheckSum;
    unsigned int iCommonFileNum;
    unsigned int iCustomFileNum;
} BackupFileInfo;

extern bool nvram_new_partition_support();
extern bool nvram_emmc_support();
bool NVM_CheckFile(const char * filepath);
int NVM_CheckFileSize(int iRealFileLid,const TCFG_FILE *pCfgFielTable);
bool NVM_ResetFileToDefault(int file_lid);
bool NVM_InSpecialLidList(int file_lid, int *index);

extern int iCustomBeginLID;
extern int iFileVerInfoLID;

static const char *g_pcNVM_AllFile    = "/data/nvram/AllFile";
static const char *g_pcNVM_AllMap     = "/data/nvram/AllMap";

//The File Will be Created after restore
static const char *g_pcNVM_Flag       = "/data/nvram/RestoreFlag";
unsigned int gFileStartAddr = 0;
pthread_mutex_t gFileStartAddrlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t generateFileVerlock = PTHREAD_MUTEX_INITIALIZER;
#define BitmapFlag 0xAABBCCDD

typedef struct {
    char	cFileName[128];
    unsigned int iLID;
} FileName;

extern const TCFG_FILE g_akCFG_File[];
extern const int g_i4CFG_File_Count;
extern const TCFG_FILE g_akCFG_File_Custom[];
extern const int g_i4CFG_File_Custom_Count;
extern int iCustomBeginLID;
extern int iFileVerInfoLID;
extern int iNvRamFileMaxLID;
volatile int g_i4MaxNvRamLid = 0;
extern FileName aBackupToBinRegion[];
extern const unsigned int g_Backup_File_Count;
static char *strVerInfo="NVRAM_VER_INFO";
pthread_mutex_t dirlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t backuplock = PTHREAD_MUTEX_INITIALIZER;
int  NvRamBlockNum = 0;

int nvram_platform_log_block = 0;
int nvram_platform_resv_block = 0;
int nvram_init_flag = 0;
int nvram_gpt_flag = 0;
int BinRegionBlockTotalNum=0;

extern const char *nvram_new_partition_name;
extern const TABLE_FOR_SPECIAL_LID g_new_nvram_lid[];
extern const unsigned int g_new_nvram_lid_count;

extern FileName aPerformance[];
extern const unsigned int g_Performance_File_Count;

int open_file_with_dirs(const char *fn, int flag,mode_t mode)
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
    filedesc = open(tmp,flag,mode);
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

bool NVM_GetDeviceInfo(const char *path, struct mtd_info_user *device_info)
{
    int fd;
    int iRet;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        NVRAM_LOGE("NVM_GetDeviceInfo : open  %s fail!!! %s\n",path,(char*)strerror(errno));
        return false;
    }
    if (nvram_gpt_flag == 1) {
        device_info->type = MTD_NANDFLASH;
        device_info->flags = MTD_WRITEABLE;
        device_info->size = lseek(fd, 0, SEEK_END);
        device_info->erasesize = 128*1024;
        device_info->writesize = 512;
        device_info->oobsize = 0;

    } else {
        iRet = ioctl(fd, MEMGETINFO, device_info);
        if (iRet < 0) {
            NVRAM_LOGE("NVM_GetDeviceInfo : dumchar ioctl fail %s\n",(char*)strerror(errno));
            close(fd);
            return false;
        }
    }
    close(fd);

    return true;
}

bool NVM_EraseDeviceBlock(const char *path, struct erase_info_user erase_info)
{
    int fd;
    int iRet;

    fd = open(path, O_RDWR|O_SYNC);
    if (fd < 0) {
        NVRAM_LOGE("NVM_EraseDeviceBlock: open fail!!! %s\n",(char*)strerror(errno));
        return false;
    } else {
        iRet = ioctl(fd, MEMERASE, &erase_info);
        if (iRet < 0) {
            NVRAM_LOGE("NVM_EraseDeviceBlock: erase fail! %s\n",(char*)strerror(errno));
            close(fd);
            return false;
        }
    }
    close(fd);

    return true;
}

/********************************************************************************
//FUNCTION:
//		NVM_Init
//DESCRIPTION:
//		this function is called to call nvram lib and get max lid.
//
//PARAMETERS:
//		None
//
//RETURN VALUE:
//		the max Lid number.
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/

FileName *gFileNameArray = NULL;

int NVM_Init(void)
{
    int fd=0;

    if (nvram_init_flag) {
        return (g_i4MaxNvRamLid);
    }

    g_i4MaxNvRamLid = g_i4CFG_File_Count + g_i4CFG_File_Custom_Count;
	
    fd = open(NVRAM_BACKUP_DEVICE_GPT,O_RDWR);
    if(fd < 0) {
        fd = open(NVRAM_BACKUP_DEVICE,O_RDWR);
        if(fd < 0) {
            return (g_i4MaxNvRamLid);
        } else {
            nvram_gpt_flag = 0;
        }
    } else {
        nvram_gpt_flag = 1;
    }
    close(fd);
    nvram_init_flag = 1;

	NVRAM_LOG("NVM_Init gpt %d\n",nvram_gpt_flag);

    return (g_i4MaxNvRamLid);
}
int NVM_GetLIDByName(char* filename)
{
    int i = 0;
    int Lid = - 1;

    NVRAM_LOG("NVM_GetLIDByName %s \n",filename);
    NVM_Init();

    if (gFileNameArray == NULL) {

        gFileNameArray = (FileName*)malloc(g_i4MaxNvRamLid*sizeof(FileName));
        if (gFileNameArray == NULL) {
            NVRAM_LOG("Filename array malloc fail \n");
            free(gFileNameArray);
            return -1;
        }


        for ( i=0; i< g_i4CFG_File_Count; i++) {

            strcpy(gFileNameArray[i].cFileName, g_akCFG_File[i].cFileName);
            gFileNameArray[i].iLID = i;
        }
        for ( i=g_i4CFG_File_Count ; i< g_i4CFG_File_Count+g_i4CFG_File_Custom_Count; i++) {

            strcpy(gFileNameArray[i].cFileName, g_akCFG_File_Custom[i-iCustomBeginLID].cFileName);
            gFileNameArray[i].iLID = i;

            NVRAM_LOG("deal with cfgcustfile =%s,%d\n",gFileNameArray[i].cFileName,gFileNameArray[i].iLID);
        }

    }

    if (!filename || (strstr(filename,"Reserved")!=NULL) || (strlen(filename)>FILENAMELENGTH)  || (strlen(filename)==0)) {
        NVRAM_LOG("NVRAM: Invalide argument for find LID name array! \n");
        return -1;
    }

    for (i=0 ; i < g_i4MaxNvRamLid; i++) {

        if (strstr(gFileNameArray[i].cFileName,filename)) {
            Lid = gFileNameArray[i].iLID;

            break;
        }
    }
    NVRAM_LOG("NVRAM: NVM_GetLIDByName Lid =%d \n",Lid);
    return Lid;

}

/********************************************************************************
//FUNCTION:
//		NVM_GetCfgFileTable
//DESCRIPTION:
//		this function is called to the array table of nvram file information .
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//
//RETURN VALUE:
//		refers to the definition of "TCFG_FILE"
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
const TCFG_FILE* NVM_GetCfgFileTable(int file_lid)
{
    if (file_lid >= (g_i4CFG_File_Count + g_i4CFG_File_Custom_Count)) {
        NVRAM_LOGE("NVM_GetCfgFileTable file_lid is over than maximum %d\n", (g_i4CFG_File_Count + g_i4CFG_File_Custom_Count));
        return NULL;
    }

    if(file_lid<iCustomBeginLID) {
        return g_akCFG_File;
    }

    return g_akCFG_File_Custom;
}

/*******************************************************************************
//FUNCTION:
//		NVM_GenerateFileVer
//DESCRIPTION:
//		this function is called to generate the version file in backup or nvram partition.
//
//PARAMETERS:
//		CPY_File_To_NVM: [IN] true is generate version file in NVM partition
//
//RETURN VALUE:
//		true is success, otherwise is fail
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
******************************************************************************/
bool NVM_GenerateFileVer(bool CPY_File_To_NVM)
{
    int iFileDesc;
    int iFilNO =0;
    char tempstr[FILENAMELENGTH];
    int iStrLen=0;
    memset(tempstr,0,FILENAMELENGTH);
    iStrLen=strlen(strVerInfo);

    NVRAM_LOG("NVM_GenerateFileVer: CPY_File_To_NVM=%d ++\n", CPY_File_To_NVM);

    if (!CPY_File_To_NVM) {
        return true;
    }

    pthread_mutex_lock(&generateFileVerlock);
    iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName, O_CREAT | O_RDWR|O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);
    NVRAM_LOG("Create/Open the file of %s \n", g_akCFG_File[iFileVerInfoLID].cFileName);
    if (iFileDesc == -1) {
        NVRAM_LOG("Create the dir path of %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
        iFileDesc = open_file_with_dirs(g_akCFG_File[iFileVerInfoLID].cFileName, O_CREAT | O_RDWR|O_SYNC,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);
    }

    if (iFileDesc == -1) {
        NVRAM_LOG("Fail to open %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
        pthread_mutex_unlock(&generateFileVerlock);
        return false;
    }
    memcpy(tempstr,strVerInfo,iStrLen+1);
    write(iFileDesc, tempstr, FILENAMELENGTH);
    write(iFileDesc, g_akCFG_File[iFileVerInfoLID].cFileVer, 4);
    memset(tempstr,0,FILENAMELENGTH);
    for (iFilNO = iFileVerInfoLID; iFilNO < iCustomBeginLID; iFilNO++) {
        int i;
        iStrLen=strlen(g_akCFG_File[iFilNO].cFileName);
        for(i=iStrLen; i>=0; i--) {
            if(g_akCFG_File[iFilNO].cFileName[i]=='/') {
                strcpy(tempstr,g_akCFG_File[iFilNO].cFileName+i+1);
                break;
            }
        }
        write(iFileDesc, tempstr, FILENAMELENGTH);
        write(iFileDesc, g_akCFG_File[iFilNO].cFileVer, 4);
        memset(tempstr,0,FILENAMELENGTH);
    }

    for (iFilNO=iCustomBeginLID; iFilNO < (g_i4CFG_File_Count + g_i4CFG_File_Custom_Count); iFilNO++) {
        int i;
        iStrLen=strlen(g_akCFG_File_Custom[iFilNO-iCustomBeginLID].cFileName);
        for(i=iStrLen; i>=0; i--) {
            if(g_akCFG_File_Custom[iFilNO-iCustomBeginLID].cFileName[i]=='/') {
                strcpy(tempstr,g_akCFG_File_Custom[iFilNO-iCustomBeginLID].cFileName+i+1);
                break;
            }
        }
        write(iFileDesc, tempstr, FILENAMELENGTH);
        write(iFileDesc, g_akCFG_File_Custom[iFilNO-iCustomBeginLID].cFileVer, 4);
        memset(tempstr,0,FILENAMELENGTH);
    }

    close(iFileDesc);
    pthread_mutex_unlock(&generateFileVerlock);

    return true;
}



/********************************************************************************
//FUNCTION:
//		NVM_CheckVerFile
//DESCRIPTION:
//		this function is called to check the exist of versiono file information
//      in NVM partition or default version.
//
//PARAMETERS:
//		Is_NVM: [IN] true is to check NVM partition, otherwise check default version
//
//RETURN VALUE:
//		true is exist, otherwise version is not exist.
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_CheckVerFile(bool In_NVM)
{
    struct stat statbuf;
    NVRAM_LOG("NVM_CheckVerFile: %d\n", In_NVM);

    if (In_NVM) {
        if (stat(g_akCFG_File[iFileVerInfoLID].cFileName, &statbuf) == -1
            || !S_ISREG(statbuf.st_mode)) {
            return false;
        }
    }
    return true;
}

/********************************************************************************
//FUNCTION:
//		NVM_UpdateFileVerNo
//DESCRIPTION:
//		this function is called to reset a NvRam to default value.
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//
//RETURN VALUE:
//		true is success, otherwise is fail
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_UpdateFileVerNo(int file_lid, VerInfoUpdateFlag UpdateFlag)
{
    int iFileDesc;
    int iFileSize;
    int iRealFileLid = 0;
    struct stat st;
    char * buffer1=NULL;
    char * buffer2=NULL;
    int iResult,iStrLen;
    bool bIsNewVerInfo;
    char tempstr[FILENAMELENGTH];
    char tempstr1[FILENAMELENGTH];
    const TCFG_FILE *pCfgFileTable = NULL;

    struct stat statbuf;
    int ilooptime, iIndex,ipos = 0;
    memset(tempstr,0,FILENAMELENGTH);
    NVRAM_LOG("NVM_UpdateFileVerNo: %d ++\n", file_lid);
    if(UpdateFlag == VerUpdate) {
        pCfgFileTable = NVM_GetCfgFileTable(file_lid);
        NVRAM_LOG("NVM_GetCfgFileTable done\n");
        if ( file_lid < iCustomBeginLID ) {
            NVRAM_LOG("This File LID is belong to common lid!\n");
            iRealFileLid = file_lid;
        } else {
            NVRAM_LOG("This File LID is belong to custom lid\n");
            iRealFileLid = file_lid - iCustomBeginLID;
        }

        if (pCfgFileTable == NULL) {
            NVRAM_LOG("NVM_GetCfgFileTable Fail!!! \n");
            return false;
        }

        memset(tempstr1,0,FILENAMELENGTH);
        iStrLen = strlen(pCfgFileTable[iRealFileLid].cFileName);
        for(iIndex = iStrLen; iIndex >= 0; iIndex--) {
            if( pCfgFileTable[iRealFileLid].cFileName[iIndex]=='/') {
                strcpy( tempstr1, pCfgFileTable[iRealFileLid].cFileName + iIndex + 1 );//get the target name
                break;
            }
        }
        NVRAM_LOG("DEBUG searching LID = %d,%s\n", file_lid,tempstr1);
        iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName,O_RDONLY);

        fstat(iFileDesc, &statbuf);

        iFileSize = (int)statbuf.st_size;

        ilooptime = iFileSize/(FILEVERLENGTH+FILENAMELENGTH);

        for(iIndex = 0; iIndex < ilooptime; iIndex++) {
            ipos = lseek(iFileDesc, iIndex* (FILEVERLENGTH+FILENAMELENGTH), SEEK_SET);
            if(-1 == ipos) {
                NVRAM_LOG("NVM_UpdateFileVerNo Error lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
                close(iFileDesc);
                return false;
            }

            read(iFileDesc, tempstr, FILENAMELENGTH);
            if(0 == strcmp(tempstr,tempstr1)) {
                break;
            }
        }
        if(iIndex == ilooptime) {
            NVRAM_LOG("NVM_UpdateFileVerNo find target file fail: %d\n", file_lid);
            UpdateFlag = VerAdd;

        }

        close(iFileDesc);

    }
    memset(tempstr,0,FILENAMELENGTH);
    //open file
    pthread_mutex_lock(&generateFileVerlock);
    iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName,O_RDWR|O_SYNC);

    if (iFileDesc == -1) {
        NVRAM_LOG("Error NVM_UpdateFileVerNo Open1 version file fail \n");
        pthread_mutex_unlock(&generateFileVerlock);
        return false;
    }
    iResult = read(iFileDesc,tempstr,FILENAMELENGTH);
    if(0==strcmp(tempstr,strVerInfo))
        bIsNewVerInfo=true;
    else
        bIsNewVerInfo=false;

    memset(tempstr,0,FILENAMELENGTH);
    if(UpdateFlag==VerUpdate) { //Ver Info has existed , or added at the end of Version Info File

        // move to the address in the version file.
        // becuase we just use cFileVer to save the version,
        // so the offset address is file_lid* sizeof(cFileVer)
        if(bIsNewVerInfo) {
            if(-1==lseek(iFileDesc, ipos , SEEK_SET)) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo lseek version file fail \n");
                close(iFileDesc);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
        } else {
            if(-1==lseek(iFileDesc, (file_lid) * (FILEVERLENGTH), SEEK_SET)) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo lseek version file fail \n");
                close(iFileDesc);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
        }
        if(file_lid < iCustomBeginLID) {
            if(bIsNewVerInfo) {
                int i;
                iStrLen=strlen(g_akCFG_File[file_lid].cFileName);
                for(i=iStrLen; i>=0; i--) {
                    if(g_akCFG_File[file_lid].cFileName[i]=='/') {
                        strcpy(tempstr,g_akCFG_File[file_lid].cFileName+i+1);
                        break;
                    }
                }
                write(iFileDesc, tempstr, FILENAMELENGTH);
                memset(tempstr,0,FILENAMELENGTH);
            }
            write(iFileDesc, g_akCFG_File[file_lid].cFileVer, FILEVERLENGTH);
        } else {
            if(bIsNewVerInfo) {
                int i;
                iStrLen=strlen(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName);
                for(i=iStrLen; i>=0; i--) {
                    if(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName[i]=='/') {
                        strcpy(tempstr,g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName+i+1);
                        break;
                    }
                }
                write(iFileDesc, tempstr, FILENAMELENGTH);
                memset(tempstr,0,FILENAMELENGTH);

            }
            write(iFileDesc, g_akCFG_File_Custom[file_lid - iCustomBeginLID].cFileVer, FILEVERLENGTH);
        }

        //close(iFileDesc);
    } else if(UpdateFlag==VerDel) {
        if(stat(g_akCFG_File[iFileVerInfoLID].cFileName,&st)<0) {
            NVRAM_LOG("Error NVM_UpdateFileVerNo stat \n");
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }

        if(-1==lseek(iFileDesc, 0, SEEK_SET)) {
            NVRAM_LOG("Error NVM_UpdateFileVerNo lseek version file fail \n");
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }

        iFileSize=st.st_size;

        buffer1=(char*)malloc(iFileSize);
        if(bIsNewVerInfo)
            buffer2=(char*)malloc(iFileSize-FILEVERLENGTH-FILENAMELENGTH);
        else
            buffer2=(char*)malloc(iFileSize-FILEVERLENGTH);
        if(buffer1==NULL||buffer2==NULL) {
            NVRAM_LOG("Error NVM_UpdateFileVerNo stat \n");
            if(buffer1 != NULL)
                free(buffer1);
            if(buffer2 != NULL)
                free(buffer2);
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }
        iResult = read(iFileDesc,buffer1,iFileSize);
        if(iResult != st.st_size) {
            NVRAM_LOGE("iResult = %d, st.st_size = %zd\n",iResult, st.st_size);
            free(buffer1);
            free(buffer2);
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }
        close(iFileDesc);
        pthread_mutex_unlock(&generateFileVerlock);
        if(!bIsNewVerInfo) {
            memcpy(buffer2,buffer1,file_lid * FILEVERLENGTH);
            memcpy(buffer2+file_lid * FILEVERLENGTH,buffer1+(file_lid+1)*FILEVERLENGTH,
                   iFileSize-(file_lid+1)*FILEVERLENGTH);
        } else {
            memcpy(buffer2,buffer1,(file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH));
            memcpy(buffer2+(file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH),buffer1+(file_lid+1+1)*(FILEVERLENGTH+FILENAMELENGTH),
                   iFileSize-(file_lid+1+1)*(FILEVERLENGTH+FILENAMELENGTH));
        }
        pthread_mutex_lock(&generateFileVerlock);
        iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName,
                         O_TRUNC|O_RDWR|O_SYNC);
        if (iFileDesc == -1) {
            NVRAM_LOG("Error NVM_UpdateFileVerNo Open2 version file fail \n");
            free(buffer1);
            free(buffer2);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }
        if(!bIsNewVerInfo) {
            iResult = write(iFileDesc,buffer2,iFileSize-FILEVERLENGTH);
            if(iResult != (iFileSize-FILEVERLENGTH)) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo write version file fail \n");
                close(iFileDesc);
                free(buffer1);
                free(buffer2);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
        } else {
            iResult = write(iFileDesc,buffer2,iFileSize-FILEVERLENGTH-FILENAMELENGTH);
            if(iResult != (iFileSize-FILEVERLENGTH-FILENAMELENGTH)) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo write version file fail \n");
                close(iFileDesc);
                free(buffer1);
                free(buffer2);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
        }
        free(buffer1);
        free(buffer2);

    } else if(UpdateFlag==VerAdd) {
        if(stat(g_akCFG_File[iFileVerInfoLID].cFileName,&st)<0) {
            NVRAM_LOG("Error NVM_UpdateFileVerNo stat \n");
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            return false;
        }
        iFileSize=st.st_size;
        bool bAddedAtend;
        if(bIsNewVerInfo) {
            if((file_lid+1)*(FILEVERLENGTH+FILENAMELENGTH)>=iFileSize)
                bAddedAtend=true;
            else
                bAddedAtend=false;
        } else {
            if(file_lid*FILEVERLENGTH>=iFileSize)
                bAddedAtend=true;
            else
                bAddedAtend=false;
        }
        if(bAddedAtend) {
            NVRAM_LOG("Added at the end of the file \n");
            if(bIsNewVerInfo) {
                if(-1==lseek(iFileDesc, (file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH), SEEK_SET)) {
                    NVRAM_LOG("Error NVM_UpdateFileVerNo lseek version file fail \n");
                    close(iFileDesc);
                    pthread_mutex_unlock(&generateFileVerlock);
                    return false;
                }
            } else {
                if(-1==lseek(iFileDesc, file_lid * FILEVERLENGTH, SEEK_SET)) {
                    NVRAM_LOG("Error NVM_UpdateFileVerNo lseek version file fail \n");
                    close(iFileDesc);
                    pthread_mutex_unlock(&generateFileVerlock);
                    return false;
                }
            }
            if(file_lid < iCustomBeginLID) {
                if(bIsNewVerInfo) {
                    int i;
                    iStrLen=strlen(g_akCFG_File[file_lid].cFileName);
                    for(i=iStrLen; i>=0; i--) {
                        if(g_akCFG_File[file_lid].cFileName[i]=='/') {
                            strcpy(tempstr,g_akCFG_File[file_lid].cFileName+i+1);
                            break;
                        }
                    }
                    write(iFileDesc, tempstr, FILENAMELENGTH);
                    memset(tempstr,0,FILENAMELENGTH);
                }
                write(iFileDesc, g_akCFG_File[file_lid].cFileVer, FILEVERLENGTH);
            } else {
                if(bIsNewVerInfo) {
                    int i;
                    iStrLen=strlen(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName);
                    for(i=iStrLen; i>=0; i--) {
                        if(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName[i]=='/') {
                            strcpy(tempstr,g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName+i+1);
                            break;
                        }
                    }
                    write(iFileDesc, tempstr, FILENAMELENGTH);
                    memset(tempstr,0,FILENAMELENGTH);
                }
                write(iFileDesc, g_akCFG_File_Custom[file_lid - iCustomBeginLID].cFileVer, FILEVERLENGTH);
            }
        } else {
            NVRAM_LOG("Added at the middle part of the file \n");
            buffer1=(char*)malloc(iFileSize);
            if(bIsNewVerInfo) {
                buffer2=(char*)malloc(iFileSize+FILEVERLENGTH+FILENAMELENGTH);
            } else {
                buffer2=(char*)malloc(iFileSize+FILEVERLENGTH);
            }
            if(buffer1==NULL||buffer2==NULL) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo stat \n");
                if(buffer1 != NULL)
                    free(buffer1);
                if(buffer2 != NULL)
                    free(buffer2);
                close(iFileDesc);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            pthread_mutex_lock(&generateFileVerlock);
            iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName,O_RDWR|O_SYNC);
            if (iFileDesc == -1) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo Open1 version file fail \n");
                pthread_mutex_unlock(&generateFileVerlock);
                free(buffer1);
                free(buffer2);
                return false;
            }
            iResult = read(iFileDesc,buffer1,iFileSize);
            if(iResult != iFileSize) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo read add \n");
                free(buffer1);
                free(buffer2);
                close(iFileDesc);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
            close(iFileDesc);
            pthread_mutex_unlock(&generateFileVerlock);
            NVRAM_LOG("cpy1\n");
            if(bIsNewVerInfo) {
                memcpy(buffer2,buffer1,(file_lid+1)*(FILEVERLENGTH+FILENAMELENGTH));
            } else {
                memcpy(buffer2,buffer1,file_lid*FILEVERLENGTH);
            }
            if(file_lid < iCustomBeginLID) {
                if(bIsNewVerInfo) {
                    int i;
                    iStrLen=strlen(g_akCFG_File[file_lid].cFileName);
                    for(i=iStrLen; i>=0; i--) {
                        if(g_akCFG_File[file_lid].cFileName[i]=='/') {
                            strcpy(tempstr,g_akCFG_File[file_lid].cFileName+i+1);
                            break;
                        }
                    }
                    memcpy(buffer2+(file_lid+1)*(FILENAMELENGTH+FILEVERLENGTH),tempstr,FILENAMELENGTH);
                    memset(tempstr,0,FILENAMELENGTH);
                    memcpy(buffer2+(file_lid+1)*(FILENAMELENGTH+FILEVERLENGTH)+FILENAMELENGTH,g_akCFG_File[file_lid].cFileVer,FILEVERLENGTH);
                } else {
                    memcpy(buffer2+file_lid*FILEVERLENGTH,g_akCFG_File[file_lid].cFileVer,FILEVERLENGTH);
                }
            } else {
                if(bIsNewVerInfo) {
                    int i;
                    iStrLen=strlen(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName);
                    for(i=iStrLen; i>=0; i--) {
                        if(g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName[i]=='/') {
                            strcpy(tempstr,g_akCFG_File_Custom[file_lid-iCustomBeginLID].cFileName+i+1);
                            break;
                        }
                    }
                    memcpy(buffer2+(file_lid+1)*(FILENAMELENGTH+FILEVERLENGTH),tempstr,FILENAMELENGTH);
                    memset(tempstr,0,FILENAMELENGTH);
                    memcpy(buffer2+(file_lid+1)*(FILENAMELENGTH+FILEVERLENGTH)+FILENAMELENGTH,g_akCFG_File_Custom[file_lid - iCustomBeginLID].cFileVer,FILEVERLENGTH);

                } else {
                    memcpy(buffer2+file_lid*FILEVERLENGTH,g_akCFG_File_Custom[file_lid - iCustomBeginLID].cFileVer,FILEVERLENGTH);
                }

            }
            NVRAM_LOG("cpy2\n");
            if(bIsNewVerInfo) {
                memcpy(buffer2+(file_lid+1+1)*(FILEVERLENGTH+FILENAMELENGTH),buffer1+(file_lid+1)*(FILEVERLENGTH+FILENAMELENGTH),
                       iFileSize-(file_lid+1)*(FILEVERLENGTH+FILENAMELENGTH));
            } else {
                memcpy(buffer2+(file_lid+1)*FILEVERLENGTH,buffer1+file_lid*FILEVERLENGTH,
                       iFileSize-file_lid*FILEVERLENGTH);
            }
            NVRAM_LOG("cpy3\n");
            pthread_mutex_lock(&generateFileVerlock);
            iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName,
                             O_TRUNC|O_RDWR|O_SYNC);
            if (iFileDesc == -1) {
                NVRAM_LOG("Error NVM_UpdateFileVerNo Open2 version file fail \n");
                free(buffer1);
                free(buffer2);
                pthread_mutex_unlock(&generateFileVerlock);
                return false;
            }
            if(bIsNewVerInfo) {
                iResult = write(iFileDesc,buffer2,iFileSize+FILEVERLENGTH+FILENAMELENGTH);
                if(iResult != (iFileSize+FILEVERLENGTH+FILENAMELENGTH)) {
                    NVRAM_LOG("Error NVM_UpdateFileVerNo write version file fail \n");
                    close(iFileDesc);
                    free(buffer1);
                    free(buffer2);
                    pthread_mutex_unlock(&generateFileVerlock);
                    return false;
                }
            } else {
                iResult = write(iFileDesc,buffer2,iFileSize+FILEVERLENGTH);
                if(iResult!=(iFileSize+FILEVERLENGTH)) {
                    NVRAM_LOG("Error NVM_UpdateFileVerNo write version file fail \n");
                    close(iFileDesc);
                    free(buffer1);
                    free(buffer2);
                    pthread_mutex_unlock(&generateFileVerlock);
                    return false;
                }
            }


            free(buffer1);
            free(buffer2);
        }
    } else {
        pthread_mutex_unlock(&generateFileVerlock);
        return false;
    }
    close(iFileDesc);
    pthread_mutex_unlock(&generateFileVerlock);
    NVRAM_LOG("UpdateFileVerNo: %d --\n", file_lid);
    return true;

}
static unsigned int NVM_ComputeCheckSum(void)
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
        NVBAK_LOG("Error NVM_ComputeCheckSum stat \n");
        return 0;
    }

    iFileSize=st.st_size;
    looptime=iFileSize/(sizeof(unsigned int));

    iFileDesc_file = open(g_pcNVM_AllFile , O_RDONLY);
    if(iFileDesc_file<0) {
        NVBAK_LOG("NVM_ComputeCheckSum cannot open data file\n");
        return 0;
    }
    flag=1;
    for(i=0; i<looptime; i++) {
        iResult=read(iFileDesc_file, &tempNum, iLength);
        if(iResult!= iLength) {
            NVBAK_LOG("NVM_GetCheckSum cannot read checksum data\n");
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
        NVBAK_LOG("NVM_GetCheckSum cannot read last checksum data\n");
        close(iFileDesc_file);
        return 0;
    }
    ulCheckSum+=tempNum;
    //ulCheckSum^=gFileStartAddr;
    close(iFileDesc_file);
    return ulCheckSum;
}

static BackupFileInfo stBackupFileInfo;
static bool NVM_GetCheckSum(void)
{
    int iFileDesc_map;
    int iResult;
    int iLength=sizeof(unsigned int);

    iFileDesc_map = open(g_pcNVM_AllMap,O_RDONLY);
    if(iFileDesc_map<0) {
        NVBAK_LOG("NVM_GetCheckSum cannot open/create map data\n");
        return false;
    }
    iResult=read(iFileDesc_map, &stBackupFileInfo.ulCheckSum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("NVM_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    iLength=sizeof(unsigned int);
    iResult=read(iFileDesc_map, &stBackupFileInfo.iCommonFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("NVM_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    iResult=read(iFileDesc_map, &stBackupFileInfo.iCustomFileNum, iLength);
    if(iResult!= iLength) {
        NVBAK_LOG("NVM_GetCheckSum cannot read checksum data\n");
        close(iFileDesc_map);
        return false;
    }
    close(iFileDesc_map);

    return true;
}

static bool NVM_CompareCheckSum(unsigned int ulCheckSum1,unsigned int ulCheckSum2)
{
    if(ulCheckSum1!=ulCheckSum2)
        return false;
    return true;
}

bool NVM_RestoreFromFiles_OneFile(int eBackupType,int file_lid,const char* filename,bool* find_flag)
{
    int iFileDesc_file, iFileDesc_map, iFileDesc, iSize;
    int iFileTitleOffset = 0;
    short int iFileNum=0;
    char *buf;  /* content  */
    File_Title *FileInfo = NULL;
    bool bRet = false;
    off_t iRet;

    File_Title_Header FileTitleInfo;
    int fhs = sizeof(unsigned int)+2*sizeof(unsigned int)+sizeof(File_Title_Header);

    int fis = sizeof(File_Title);

    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    int filesizeintable = 0;
    if((filename == NULL) && (file_lid >=0)) {
        NVRAM_LOG("NVM_RestoreFromFiles_OneFile : %d ++\n", file_lid);

        if(!NVM_CheckVerFile(true)) {
            NVM_GenerateFileVer(true);
        }

        //get the file informatin table.
        pCfgFielTable = NVM_GetCfgFileTable(file_lid);
        if (pCfgFielTable == NULL) {
            NVRAM_LOG("NVM_RestoreFromFiles_OneFile: NVM_GetCfgFileTable Fail!!!\n");
            return 0;
        }


        if (file_lid == iFileVerInfoLID) {
            if (!NVM_GenerateFileVer(true)) {
                return 0;
            }
            NVRAM_LOG("NVM_RestoreFromFiles_OneFile:Wrong file_lid Fail!!!\n");
            return 0;
        }

        if (file_lid >= iCustomBeginLID) {
            iRealFileLid = file_lid - iCustomBeginLID;
        } else {
            iRealFileLid = file_lid;
        }

    }
    /* malloc the buffer of title buf */
    FileInfo = (File_Title *)malloc(sizeof(File_Title));
    memset(FileInfo, 0, sizeof(File_Title));

    iFileDesc_file = open(g_pcNVM_AllFile , O_RDWR);
    iFileDesc_map = open(g_pcNVM_AllMap, O_RDWR);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        /* Error handling */
        NVBAK_LOG("NVM_RestoreFromFiles_OneFile cannot open file data\n");
        free(FileInfo);
        return false;
    }

    if(INVALID_HANDLE_VALUE == iFileDesc_map) {
        /* Error handling */
        NVBAK_LOG("NVM_RestoreFromFiles_OneFile cannot open map data\n");
        close(iFileDesc_file);
        free(FileInfo);
        return false;
    }

    lseek(iFileDesc_map, sizeof(unsigned int)+2*sizeof(unsigned int), SEEK_SET);
    iSize = (int)read(iFileDesc_map, &FileTitleInfo, sizeof(File_Title_Header));

    switch (eBackupType) {
    case APBOOT:
        NVBAK_LOG("NVM_RestoreFromFiles_OneFile APBOOT start !");
        iFileNum = FileTitleInfo.iApBootNum;
        iFileTitleOffset = fhs;
        break;

    case APCLN:
        NVBAK_LOG("NVM_RestoreFromFiles_OneFile APCLN start !");
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

    iRet = lseek(iFileDesc_map, iFileTitleOffset, SEEK_SET);

    while(iFileNum>0) {
        iSize = (int)read(iFileDesc_map, FileInfo, sizeof(File_Title));
        if((filename == NULL) && (file_lid >=0)) {
            if(strcmp(FileInfo->cFileName,pCfgFielTable[iRealFileLid].cFileName) != 0) {
                --iFileNum;
                continue;
            }
        } else {
            if(strcmp(FileInfo->cFileName,filename) != 0) {
                --iFileNum;
                continue;
            }
        }
        *find_flag=1;
        iFileNum=0;
        NVBAK_LOG("FileInfo: %s\n", FileInfo->cFileName);
        if((filename == NULL) && (file_lid >=0)) {
            filesizeintable = pCfgFielTable[iRealFileLid].i4RecNum * pCfgFielTable[iRealFileLid].i4RecSize;
            NVBAK_LOG("FileInfo: startaddr(0x%x) size(0x%x) size in table(0x%x).\n", FileInfo->FielStartAddr, FileInfo->Filesize,filesizeintable);
            if(FileInfo->Filesize!=filesizeintable+2) {
                NVBAK_LOG("Restored file size error !");
                close(iFileDesc_map);
                close(iFileDesc_file);
                free(FileInfo);
                return false;
            }
        }
        buf = (char *)malloc(FileInfo->Filesize);

        //read the data and write to the file
        iRet = lseek(iFileDesc_file, FileInfo->FielStartAddr, SEEK_SET);
        if(iRet == -1) {
            NVBAK_LOG("lseek fail !");
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }
        iSize = (int)read(iFileDesc_file, buf, FileInfo->Filesize);
        if(iSize!=FileInfo->Filesize) {
            NVBAK_LOG("read fail !iSize=%d,FileInfo->Filesize=%d\n",iSize,FileInfo->Filesize);
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }

        iFileDesc = open_file_with_dirs(FileInfo->cFileName, O_CREAT | O_TRUNC | O_RDWR|O_SYNC,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP );

        if(INVALID_HANDLE_VALUE == iFileDesc) {
            /* Error handling */
            NVBAK_LOG("NVM_RestoreFromFiles_OneFile cannot create %s\n", FileInfo->cFileName);

            //added
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(buf);
            free(FileInfo);
            return false;
        }
        iSize = (int)write(iFileDesc, buf, FileInfo->Filesize);
        if(iSize!=FileInfo->Filesize) {
            NVBAK_LOG("write fail !iSize=%d,FileInfo->Filesize=%d\n",iSize,FileInfo->Filesize);
            close(iFileDesc);
            free(buf);
            close(iFileDesc_map);
            close(iFileDesc_file);
            free(FileInfo);
            return false;
        }
        close(iFileDesc);
        free(buf);
        if((filename == NULL) && (file_lid >= 0)) {
#if 0
            if(NVM_ProtectDataFile(file_lid,true) == 1) {
                NVRAM_LOG("NVM_RestoreFromFiles_OneFile ProtectDataFile Success!!\n");
            } else {
                NVRAM_LOG("NVM_RestoreFromFiles_OneFile ProtectDataFile SET Fail!!\n");
                return false;
            }
#endif
            if(NVM_CheckFile(FileInfo->cFileName) && (NVM_CheckFileSize(iRealFileLid, pCfgFielTable) != -1)) {
                NVRAM_LOG("NVM_RestoreFromFiles_OneFile ProtectDataFile Success!!\n");
            } else {
                NVRAM_LOG("NVM_RestoreFromFiles_OneFile ProtectDataFile Fail!!\n");
                close(iFileDesc_map);
                close(iFileDesc_file);
                free(FileInfo);
                return false;
            }
        }
        bRet = true;
        break;
    }

    close(iFileDesc_map);
    close(iFileDesc_file);
    free(FileInfo);
    return bRet;
}

bool NVM_CheckData_OneFile(int file_lid,const char * filename)
{
    bool bRet = false;
    int eBackupType;
    unsigned int ulSavedCheckSum;
    unsigned int ulCheckSum;
    struct stat st;
    unsigned int iFileSize;
    bool find_flag = 0;

    NVBAK_LOG("[NVM_CheckData_OneFile] start !");
    if(!NVM_GetCheckSum()) {
        NVBAK_LOG("[NVM_CheckData_OneFile] GetCheckSum Fail !");
        return false;
    }
    ulSavedCheckSum=stBackupFileInfo.ulCheckSum;
    ulCheckSum=NVM_ComputeCheckSum();
    NVBAK_LOG("ulCheckSun:%d\n",ulCheckSum);
    if(stat(g_pcNVM_AllFile,&st)<0) {
        NVBAK_LOG("Error NVM_CheckData_OneFile stat \n");
        return false;
    }
    iFileSize=st.st_size;
    NVBAK_LOG("iFileSize:%d\n",iFileSize);
    ulCheckSum^=iFileSize;

    NVBAK_LOG("NVM_CheckData_OneFile:%x,%x",ulSavedCheckSum,ulCheckSum);
    if(!NVM_CompareCheckSum(ulSavedCheckSum,ulCheckSum)) {
        NVBAK_LOG("check sum not match!");
        return false;
    }

    for (eBackupType =APBOOT; eBackupType < ALL; eBackupType++) {
        if(find_flag == 0)
            bRet = NVM_RestoreFromFiles_OneFile(eBackupType,file_lid,filename,&find_flag);
        else
            break;
    }

    NVBAK_LOG("[NVM_CheckData_OneFile] end !");
    return bRet;
}

bool NVM_RestoreFromBinRegion_OneFile(int file_lid,const char * filename)
{
    int iFileDesc_file, iFileDesc_map, fd, iResult,iBlockNum;
    unsigned int iMapFileSize,iDatFileSize,iBlockSize;
    bool bRet = true;
    char cMtdDevName[64] = {0};
    char *tempBuffer=NULL;
    char *tempBitmap1 = NULL;
    char *tempBitmap2 = NULL;
    int i,j,pos=0,flag=0;
    bool bSuccessFound;
    struct mtd_info_user info;

    NVM_Init();

    if((file_lid < 0) && (filename == NULL)) {
        NVBAK_LOGE("NVM_RestoreFromBinRegion_OneFile bad arg\n");
        return false;
    }
    memset(cMtdDevName, 0, sizeof cMtdDevName);
    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName, NVRAM_BACKUP_DEVICE_GPT);
    else
        sprintf(cMtdDevName, NVRAM_BACKUP_DEVICE);

    if(!NVM_GetDeviceInfo(cMtdDevName, &info)) {
        NVBAK_LOG("NVM_RestoreFromBinRegion_OneFile get device info fail!!!\n");
    }

    fd=open(cMtdDevName,O_RDWR);
    if(fd<0) {
        NVBAK_LOG("mtd open error %s\r\n",(char*)strerror(errno));
        return false;
    }
    iBlockSize=info.erasesize;

    BinRegionBlockTotalNum=info.size/iBlockSize - nvram_platform_log_block - nvram_platform_resv_block;
    NvRamBlockNum=BinRegionBlockTotalNum;

    tempBuffer=(char*)malloc(iBlockSize);
    if(tempBuffer==NULL) {
        NVBAK_LOG("memory malloc error\r\n");
        close(fd);
        return false;
    }
    iBlockNum=NvRamBlockNum;
    NVBAK_LOG("iBlockNum:%d\n",iBlockNum);

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
    for(i=0; i<NvRamBlockNum; i++) {
        NVBAK_LOG("[NVRAM_Bitmap]:Block%d,%d\n",i,tempBitmap1[i]);
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
    iResult = read(fd,tempBuffer,iBlockSize);
    NVBAK_LOG("map file read size:%d\n",iResult);
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

    iFileDesc_file = open(g_pcNVM_AllFile , O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR| S_IRGRP |S_IWGRP);
    iFileDesc_map = open(g_pcNVM_AllMap, O_TRUNC|O_CREAT|O_RDWR|O_SYNC, S_IRUSR|S_IWUSR| S_IRGRP |S_IWGRP);

    if(INVALID_HANDLE_VALUE == iFileDesc_file) {
        NVBAK_LOG("cannot open file data\n");
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
        if(iResult != (int)iBlockSize) {
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


    bRet = NVM_CheckData_OneFile(file_lid,filename);
    return bRet;

}

/*******************************************************************************
//FUNCTION:
//NVM_ComputeCheckNo
//DESCRIPTION:
//		this function is called to compute CheckNo of file in NvRam
//PARAMETERS:
//		filepath:[IN] the path name of the file
//		ContentSize:[IN] the content size of the file
//		CheckNo_flag:[IN] the flag of CheckNo in the file
//RETURN VALUE:
//		CheckNo
//DEPENDENCY:
//		NVM_CheckFileSize must be called
//GLOBALS AFFECTED:
//		None
********************************************************************************/
char NVM_ComputeCheckNo(const char * filepath,char *pCheckNo_flag,bool IS_OLD_FILE)
{
    int iFileDesc=0;
    unsigned int i=0,ilooptimes=0;
    bool flag=0;
    char buf,cCheckNo;
    struct stat st;
    if(stat(filepath,&st)<0) {
        NVRAM_LOG("Error NVM_ComputeCheckNo stat!\n");
        *pCheckNo_flag=0xFF;
        return 0;
    }
    iFileDesc=open(filepath,O_RDONLY);
    if (iFileDesc == -1) { //if file doesn't exist
        NVRAM_LOG("NVM_ComputeCheckNo:Open file failed!");
        *pCheckNo_flag=0xFF;
        return 0;
    }
    memset(&buf,0,sizeof(char));
    memset(&cCheckNo,0,sizeof(char));
    if(IS_OLD_FILE)
        ilooptimes=(st.st_size-2*sizeof(char))/sizeof(char);
    else
        ilooptimes=st.st_size/sizeof(char);
    for(i=0; i<ilooptimes; ++i) {
        if(sizeof(char)==read(iFileDesc,&buf,sizeof(char))) {
            if(flag) {
                cCheckNo^=buf;
                flag=false;
            } else {
                cCheckNo+=buf;
                flag=true;
            }
        } else {
            NVRAM_LOG("NVM_ComputeCheckNo:Read file failed!");
            *pCheckNo_flag=0xFF;
            close(iFileDesc);
            return 0;
        }

    }
    close(iFileDesc);
    return cCheckNo;
}
/********************************************************************************
//FUNCTION:
//		NVM_CheckFileSize
//DESCRIPTION:
//		this function is called to Check the size of the file in NvRam contain the check bytes.
//
//PARAMETERS:
//		file_lid:[IN] the lid of the file
//
//RETURN VALUE:
//		true is exist, otherwise is fail
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
int NVM_CheckFileSize(int iRealFileLid,const TCFG_FILE *pCfgFielTable)
{
    struct stat st;
    if(stat(pCfgFielTable[iRealFileLid].cFileName,&st)<0) {
        NVRAM_LOG("NVM_CheckFileSize:stat Fail!!!\n");
        return -1;
    }

    NVRAM_LOG("NVM_CheckFileSize:stat_size:%lld,size in table:%d\n",st.st_size,pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecSize);
    switch (pCfgFielTable[iRealFileLid].stDefualType) {
    case DEFAULT_ZERO:
    case DEFAULT_FF:		// when the type of default value is DEFAULT_ZERO/DEFAULT_FF
    case SIGNLE_DEFUALT_REC:
        if(pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecSize == st.st_size)
            return 0;
        else if(pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecSize == st.st_size-2*sizeof(char))
            return 1;
        else {
            NVRAM_LOG("NVM_CheckFileSize:File size not match!!!\n");
            return -1;
        }
        break;

    case MULTIPLE_DEFUALT_REC:	// when the type of default value is MULTIPLE_DEFUALT_REC, we use the defined default value of one record to generate file
        if(pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecSize == st.st_size)
            return 0;
        else if(pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecNum*pCfgFielTable[iRealFileLid].i4RecSize == st.st_size-2*sizeof(char))
            return 1;
        else {
            NVRAM_LOG("NVM_CheckFileSize:File size not match!!!\n");
            return -1;
        }
        break;
    default:
        break;

    }
    return -1;
}
/********************************************************************************
//FUNCTION:
//		NVM_SetCheckNo
//DESCRIPTION:
//		this function is called to set the CheckNo of the file in NvRam.
//
//PARAMETERS:
//		filepath:[IN] the path name of the file
//
//RETURN VALUE:
//		true is success, otherwise is fail
//
//DEPENDENCY:
//		NVM_CheckFileSize must be called
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_SetCheckNo(const char *filepath,bool isOldFile)
{
    char cCheckNo=0;
    char cCheckFlag=0;
    int iFileDesc;
    char cFlagValue=0xAA;

    cCheckNo=NVM_ComputeCheckNo(filepath,&cCheckFlag,isOldFile);
    NVRAM_LOG("NVM_SetCheckNo: CheckNo:%x,CheckFlag:%x\n",cCheckNo,cCheckFlag);
    if(cCheckFlag == 0xFF) {
        NVRAM_LOG("NVM_SetCheckNo: ComputeCheckNoFail!!\n");
        return 0;
    }
    iFileDesc=open(filepath,O_RDWR |O_SYNC);
    if (iFileDesc == -1) { //if file doesn't exist
        NVRAM_LOG("NVM_SetCheckNo:Open file failed!");
        return 0;
    }
    if(isOldFile) {
        if(lseek(iFileDesc,-2*(int)sizeof(char),SEEK_END)<0) {
            NVRAM_LOG("NVM_SetCheckNo: (OldFile) lseek Fail!!\n");
            close(iFileDesc);
            return 0;
        }
    } else {
        if(lseek(iFileDesc,0,SEEK_END)<0) {
            NVRAM_LOG("NVM_SetCheckNo: (Newfile) lseek Fail!!\n");
            close(iFileDesc);
            return 0;
        }
    }
    if(sizeof(char) != write(iFileDesc,&cFlagValue,sizeof(char))) {
        NVRAM_LOG("NVM_SetCheckNo: write Check flag Fail!!\n");
        close(iFileDesc);
        return 0;
    }
    if(sizeof(char) == write(iFileDesc,&cCheckNo,sizeof(char))) {
        //NVRAM_LOG("NVM_SetCheckNo: CheckNo generate successfully!\n");
        close(iFileDesc);
        return 1;
    } else {
        NVRAM_LOGE("NVM_SetCheckNo: CheckNo generate Fail!\n");
        close(iFileDesc);
        return 0;
    }
}

/********************************************************************************
//FUNCTION:
//		NVM_CheckFile
//DESCRIPTION:
//		this function is called to Check file in NvRam correct.
//
//PARAMETERS:
//		filepath:[IN] the path name of the file
//
//RETURN VALUE:
//		true is correct (1), otherwise is fail(0)
//
//DEPENDENCY:
//		NVM_CheckFileSize must be called
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_CheckFile(const char * filepath)
{
    char cCheckNo=0;
    char cCheckNo_file[2];
    char cCheckFlag=0;
    int iFileDesc;
    char cFlagValue=0xAA;
    bool ISOLDFILE=true;
    memset(cCheckNo_file,0,2*sizeof(char));

    cCheckNo=NVM_ComputeCheckNo(filepath,&cCheckFlag,ISOLDFILE);
    if(cCheckFlag == 0xFF) {
        NVRAM_LOG("NVM_CheckFile: ComputeCheckNo Fail!!\n");
        return 0;
    }
    iFileDesc=open(filepath,O_RDONLY);
    if (iFileDesc == -1) { //if file doesn't exist
        NVRAM_LOG("NVM_CheckFile:Open file failed!");
        return 0;
    }
    if(lseek(iFileDesc,-2*sizeof(char),SEEK_END)<0) {
        NVRAM_LOG("NVM_CheckFile: lseek Fail!!\n");
        close(iFileDesc);
        return 0;
    } else {
        if(2*sizeof(char) == read(iFileDesc,cCheckNo_file,2*sizeof(char)))
            if(cCheckNo_file[0] == cFlagValue) {
                if(cCheckNo_file[1] == cCheckNo) {
                    close(iFileDesc);
                    return 1;

                } else {
                    NVRAM_LOG("NVM_CheckFile: File has been modified!!\n");
                    close(iFileDesc);
                    return 0;
                }
            } else {
                NVRAM_LOG("NVM_CheckFile: Check flag in File has been destroyed!!\n");
                close(iFileDesc);
                return 0;
            }
        else {
            NVRAM_LOG("NVM_CheckFile: Read file Fail!!\n");
            close(iFileDesc);
            return 0;
        }

    }

}
/********************************************************************************
//FUNCTION:
//		NVM_ProtectUserData
//DESCRIPTION:
//		this function is called to Protect User's Data.
//
//PARAMETERS:
//		filepath:[IN] the path name of the file
//
//RETURN VALUE:
//		true is correct (1), otherwise is fail(0)
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
//int loop_Reset_Protect = 0;
pthread_mutex_t recoverlock = PTHREAD_MUTEX_INITIALIZER;

int NVM_ProtectDataFile(int file_lid,bool Setflag)
{
    int CheckFileSize_flag=0;
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    bool restore_flag=false;
    int i=0,j=0;
    char filename[MAX_NAMESIZE];
    memset(filename,0,MAX_NAMESIZE);

    if(!NVM_CheckVerFile(true)) {
        NVM_GenerateFileVer(true);
    }

    //get the file informatin table.
    pCfgFielTable = NVM_GetCfgFileTable(file_lid);
    if (pCfgFielTable == NULL) {
        NVRAM_LOG("NVM_CheckFileSize: NVM_GetCfgFileTable Fail!!!\n");
        return -1;
    }

    if (file_lid == iFileVerInfoLID) {
        if (!NVM_GenerateFileVer(true)) {
            return -1;
        }
        NVRAM_LOG("NVM_CheckFileSize:Wrong file_lid Fail!!!\n");
        return -1;
    }

    if (file_lid >= iCustomBeginLID) {
        iRealFileLid = file_lid - iCustomBeginLID;
    } else {
        iRealFileLid = file_lid;
    }

    CheckFileSize_flag = NVM_CheckFileSize(iRealFileLid,pCfgFielTable);
    if(CheckFileSize_flag == -1) {
        NVRAM_LOG("NVM_ProtectUserData:File Size Error!!!\n");
        goto restore;
    }
    if(Setflag || CheckFileSize_flag == 0) {
        if(!NVM_SetCheckNo(pCfgFielTable[iRealFileLid].cFileName,CheckFileSize_flag)) {
            NVRAM_LOGE("NVM_ProtectUserData:Set Check Num Fail!!!\n");
            return 0;
        } else {
            //NVRAM_LOG("NVM_ProtectUserData:Set Check Num Success\n");
            return 1;
        }

    }
    if(!Setflag) {
        if(!NVM_CheckFile(pCfgFielTable[iRealFileLid].cFileName)) {
            NVRAM_LOG("NVM_ProtectUserData:Check Failed!!!\n");
            goto restore;
        } else {
            NVRAM_LOG("NVM_ProtectUserData:Check Success\n");
            return 1;
        }
    }

restore:
    for(j=strlen(pCfgFielTable[iRealFileLid].cFileName)-1; j>=0; j--)
        if(pCfgFielTable[iRealFileLid].cFileName[j] == '/') {
            strcpy(filename,pCfgFielTable[iRealFileLid].cFileName+j+1);
            NVRAM_LOG("filename:%s\n",filename);
            break;
        }
    for(i=0; i<(int)g_Backup_File_Count; i++) {
        if(0==strcmp(filename,aBackupToBinRegion[i].cFileName) && (unsigned int)file_lid == aBackupToBinRegion[i].iLID) {
            restore_flag=true;
            break;
        }
    }
    NVRAM_LOG("NVM_ProtectUserData: Restore or Reset!\n");
    pthread_mutex_lock(&recoverlock);
    if(!restore_flag || (restore_flag && !NVM_RestoreFromBinRegion_OneFile(file_lid,NULL))) {
        pthread_mutex_unlock(&recoverlock);
        if(restore_flag)
            NVRAM_LOG("NVM_ProtectUserData Restore Fail! Reset!!\n");
        else
            NVRAM_LOG("NVM_ProtectUserData Reset\n");
        if(!NVM_ResetFileToDefault(file_lid)) {
            NVRAM_LOG("NVM_ProtectUserData Reset Fail!!\n");
            return 0;
        } else {
            NVRAM_LOG("NVM_ProtectUserData Reset Success\n");
            return 1;
        }
    } else {
        pthread_mutex_unlock(&recoverlock);
        NVRAM_LOG("NVM_ProtectUserData Restore Success\n");
        return 1;
    }
}
/********************************************************************************
//FUNCTION:
//		NVM_ResetFileToDefault
//DESCRIPTION:
//		this function is called to reset a NvRam to default value.
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//
//RETURN VALUE:
//		true is success, otherwise is fail
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_ResetFileToDefault(int file_lid)
{
    int iFileDesc;
    char *databuf = NULL;
    int i=0;
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    int i4RecNum, i4RecSize;

    NVRAM_LOG("NVM_ResetFileToDefault : %d ++\n", file_lid);

    if(!NVM_CheckVerFile(true)) {
        if(false == NVM_GenerateFileVer(true)) {
            NVRAM_LOG("GenerateFileVer Fail!\n");
            return false;
        }
    }

    //get the file informatin table.
    pCfgFielTable = NVM_GetCfgFileTable(file_lid);
    if (pCfgFielTable == NULL) {
        NVRAM_LOG("NVM_GetCfgFileTable Fail!!!\n");
        return false;
    }

    // if the file lid is version file,
    // just generate it directly by call NVM_GenerateFileVer(...)
    //if (file_lid == AP_CFG_FILE_VER_INFO_LID)
    if (file_lid == iFileVerInfoLID) {
        if (!NVM_GenerateFileVer(true)) {
            return false;
        }
        return true;
    }

    if (file_lid >= iCustomBeginLID) {
        iRealFileLid = file_lid - iCustomBeginLID;
    } else {
        iRealFileLid = file_lid;
    }

    switch (pCfgFielTable[iRealFileLid].stDefualType) {
    case DEFAULT_ZERO:
    case DEFAULT_FF:
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName,	O_CREAT | O_TRUNC | O_RDWR|O_SYNC, S_IRUSR | S_IWUSR| S_IRGRP |S_IWGRP);

        if (iFileDesc == -1) {
            iFileDesc = open_file_with_dirs(pCfgFielTable[iRealFileLid].cFileName, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);
            NVRAM_LOG("Create the dir path of %s\n", pCfgFielTable[iRealFileLid].cFileName);
        }

        if (iFileDesc == -1) {
            NVRAM_LOG("Error NVM_ResetFileToDefault can't open file %s\n", pCfgFielTable[iRealFileLid].cFileName);
            return false;
        }

        i4RecNum = pCfgFielTable[iRealFileLid].i4RecNum;
        i4RecSize = pCfgFielTable[iRealFileLid].i4RecSize;

        databuf = (char*)malloc(i4RecSize * i4RecNum);
        if(databuf == NULL) {
            NVRAM_LOG("malloc databuf failed!\n");
            close(iFileDesc);
            return false;
        }

        if (pCfgFielTable[iRealFileLid].stDefualType == DEFAULT_ZERO) {
            memset(databuf, 0, i4RecSize * i4RecNum);
        } else {
            memset(databuf, 0xff, i4RecSize * i4RecNum);
        }
        write(iFileDesc, databuf, i4RecSize * i4RecNum);

        if (databuf != NULL) {
            free(databuf);
        }

        close(iFileDesc);
        break;

    case SIGNLE_DEFUALT_REC:
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName,	O_CREAT | O_TRUNC | O_RDWR|O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);

        if (iFileDesc == -1) {
            iFileDesc = open_file_with_dirs(pCfgFielTable[iRealFileLid].cFileName, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);
            NVRAM_LOG("Create the dir path of %s\n", pCfgFielTable[iRealFileLid].cFileName);
        }

        if (iFileDesc == -1) {
            NVRAM_LOG("Error NVM_ResetFileToDefault can't open file %s\n", pCfgFielTable[iRealFileLid].cFileName);
            return false;
        }

        i4RecNum = pCfgFielTable[iRealFileLid].i4RecNum;
        i4RecSize = pCfgFielTable[iRealFileLid].i4RecSize;

        write(iFileDesc, pCfgFielTable[iRealFileLid].pDefualtVaule, i4RecSize * i4RecNum);
        close(iFileDesc);
        break;

    case MULTIPLE_DEFUALT_REC:
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName,	O_CREAT| O_TRUNC | O_RDWR|O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);

        if (iFileDesc == -1) {
            iFileDesc = open_file_with_dirs(pCfgFielTable[iRealFileLid].cFileName, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);
            NVRAM_LOG("Create the dir path of %s\n", pCfgFielTable[iRealFileLid].cFileName);
        }

        if (iFileDesc == -1) {
            NVRAM_LOG("Error NVM_ResetFileToDefault can't open file %s\n", pCfgFielTable[iRealFileLid].cFileName);
            return false;
        }

        if ((pCfgFielTable[iRealFileLid].pDefualtVaule==NULL)||(pCfgFielTable[iRealFileLid].i4RecNum < 2)) {

            NVRAM_LOG("NVM_ResetFileToDefault Mulitple para is error \n");
            close(iFileDesc);
            return false;
        }

        i4RecNum = pCfgFielTable[iRealFileLid].i4RecNum;
        i4RecSize = pCfgFielTable[iRealFileLid].i4RecSize;

        //use one record to generate all record value
        for (i = 0; i < i4RecNum; i++) {
            write(iFileDesc, pCfgFielTable[iRealFileLid].pDefualtVaule, i4RecSize * i4RecNum);
        }

        close(iFileDesc);
        break;

    default:
        break;

    }

    NVM_UpdateFileVerNo(file_lid,VerUpdate);
    unsigned int index = 0;
    bool maskflag = 0;
    for (index = 0; index < g_Performance_File_Count; index++) {
        if ((unsigned int)file_lid == aPerformance[index].iLID) {
            maskflag = 1;
            break;
        }
    }
    if (maskflag == 0) {
        if(!NVM_SetCheckNo(pCfgFielTable[iRealFileLid].cFileName,false)) {
            NVRAM_LOGE("NVM_ResetFileToDefault :Set Check Num Fail!!!\n");
            return false;
        } else {
            //NVRAM_LOG("NVM_ResetFileToDefault :Set Check Num Success\n");
        }
    }
    NVRAM_LOG("NVM_ResetFileToDefault Success!!\n");

    return true;
}

/********************************************************************************
//FUNCTION:
//		NVM_CmpFileVerNo
//DESCRIPTION:
//		this function is called to compare file version between FAT2 and default version.
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//
//RETURN VALUE:
//		true is same, otherwise is not same
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_CmpFileVerNo(int file_lid)
{
    int iFileDesc;
    char cFbyte[4]= {0,0,0,0};
    int i=0,iResult;
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    char tempstr[FILENAMELENGTH];
    char tempstr1[FILENAMELENGTH];
    bool bIsNewVerInfo;

    NVRAM_LOG("NVM_CmpFileVerNo %d  \n" , file_lid);
    //get the file informatin table. if the file lid is version file, just generate it directly by call GenerateFileVer
    pCfgFielTable = NVM_GetCfgFileTable(file_lid);
    if (pCfgFielTable == NULL) {
        NVRAM_LOG("NVM_GetCfgFileTable Fail!!!\n");
        return false;
    }

    //if (file_lid >= AP_CFG_CUSTOM_BEGIN_LID) {
    if (file_lid >= iCustomBeginLID) {
        //iRealFileLid = file_lid - AP_CFG_CUSTOM_BEGIN_LID;
        iRealFileLid = file_lid - iCustomBeginLID;
    } else {
        iRealFileLid = file_lid;
    }
    int iStrLen;
    iStrLen=strlen(pCfgFielTable[iRealFileLid].cFileName);
    for(i=iStrLen; i>=0; i--) {
        if(pCfgFielTable[iRealFileLid].cFileName[i]=='/') {
            strcpy(tempstr1,pCfgFielTable[iRealFileLid].cFileName+i+1);//get the target name
            break;
        }
    }
    if(i<0) {
        NVRAM_LOG("Path parse Fail!!!\n");
        return false;
    }

#if 1
    //NVRAM_LOG("Check if FILE_VER exists before openning it!\n");
    if(!NVM_CheckVerFile(true)) {
        if(false == NVM_GenerateFileVer(true)) {
            NVRAM_LOG("GenerateFileVer Fail!\n");
            return false;
        }
    }
#endif

    //compare the file version
    iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName, O_RDONLY);
    if (iFileDesc == -1) {
        NVRAM_LOG("Error META_CmpFileVerNo Open2 version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
        return false;
    }
    iResult = read(iFileDesc,tempstr,FILENAMELENGTH);
    if(iResult < 0) {
        close(iFileDesc);
        return false;
    }
    if(0==strcmp(tempstr,strVerInfo)) {
        NVRAM_LOG("New version info file\n");
        bIsNewVerInfo=true;
    } else {
        NVRAM_LOG("Old version info file\n");
        bIsNewVerInfo=false;
    }
    if(bIsNewVerInfo) {
        memset(tempstr,0,FILENAMELENGTH);
        if(-1==lseek(iFileDesc, (file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH), SEEK_SET)) {
            NVRAM_LOG("Error META_CmpFileVerNo lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
            close(iFileDesc);
            return false;
        }
        read(iFileDesc, tempstr, FILENAMELENGTH);
        if(0==strcmp(tempstr,tempstr1)) {
            if(-1==lseek(iFileDesc, (file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH)+FILENAMELENGTH, SEEK_SET)) {
                NVRAM_LOG("Error META_CmpFileVerNo lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
                close(iFileDesc);
                return false;
            }
            read(iFileDesc, cFbyte, FILEVERLENGTH);
        } else {
            //search the version info from the file
            struct stat statbuf;
            int iFileSize,ilooptime;
            fstat(iFileDesc, &statbuf);
            iFileSize = (int)statbuf.st_size;
            ilooptime=iFileSize/(FILEVERLENGTH+FILENAMELENGTH);
            for(i=0; i<ilooptime; i++) {
                if(-1==lseek(iFileDesc, i* (FILEVERLENGTH+FILENAMELENGTH), SEEK_SET)) {
                    NVRAM_LOG("Error META_CmpFileVerNo lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
                    close(iFileDesc);
                    return false;
                }
                read(iFileDesc, tempstr, FILENAMELENGTH);
                if(0==strcmp(tempstr,tempstr1)) {
                    memset(tempstr,0,FILENAMELENGTH);
                    read(iFileDesc, cFbyte, FILEVERLENGTH);
                    break;
                } else {
                    memset(tempstr,0,FILENAMELENGTH);
                    continue;
                }
            }
            if(i==ilooptime) {
                NVRAM_LOG("Error META_CmpFileVerNo find target file fail: %d\n", file_lid);
                close(iFileDesc);
                return false;
            }
        }
    } else {
        if(-1==lseek(iFileDesc, file_lid * FILEVERLENGTH, SEEK_SET)) {
            NVRAM_LOG("Error META_CmpFileVerNo lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
            close(iFileDesc);
            return false;
        }
        read(iFileDesc, cFbyte, FILEVERLENGTH);
    }

    close(iFileDesc);
    NVRAM_LOG("Load File Version: %s, NvRam File Version: %s\n", pCfgFielTable[iRealFileLid].cFileVer, cFbyte);
    // compare the version one by one char
    for (i = 0; i<4; i++) {
        if (pCfgFielTable[iRealFileLid].cFileVer[i]!= cFbyte[i]) {
            NVRAM_LOG("Error META_CmpFileVerNo is not same %d, %d ,%d\n",
                      iRealFileLid, pCfgFielTable[iRealFileLid].cFileVer[i], cFbyte[i]);
            return false;
        }
    }

    return true;
}

/********************************************************************************
//FUNCTION:
//		NVM_DataVerConvert
//DESCRIPTION:
//		this function is called to convert data acccording to the version info .
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//RETURN VALUE:
//		convert successfully?
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
int NVM_DataVerConvert(int file_lid)
{
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    int iFileDesc;
    int iCurrentFileVerNO;
    int iNewFileVerNO;
    int iResult=1;
    char pFileVerInfo[4];
    int rec_size = 0;
    int rec_num = 0;
    char* pSrcMem=NULL;
    char* pDstMem=NULL;
    struct stat st;
    int iFileSize;
    char tempstr[FILENAMELENGTH];
    bool bIsNewVerInfo;
    bool setflag = true;

    NVRAM_LOG("NVM_DataVerConvert %d  \n" , file_lid);
    iFileDesc = open(g_akCFG_File[iFileVerInfoLID].cFileName, O_RDONLY);
    if (iFileDesc == -1) {
        NVRAM_LOG("Error META_ConvertData Open2 version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
        return 0;
    }

    iResult = read(iFileDesc,tempstr,FILENAMELENGTH);
    if(0==strcmp(tempstr,strVerInfo)) {
        NVRAM_LOG("New version info file\n");
        bIsNewVerInfo=true;
    } else {
        NVRAM_LOG("Old version info file\n");
        bIsNewVerInfo=false;
    }

    if(stat(g_akCFG_File[iFileVerInfoLID].cFileName,&st)<0) {
        NVRAM_LOG("Error NVM_DataVerConvert stat \n");
        close(iFileDesc);
        return 0;
    }
    //if the file lid is greater than the max file lid
    if(bIsNewVerInfo) {
        if(((file_lid+1+1) * (FILEVERLENGTH+FILENAMELENGTH))>st.st_size) {
            NVRAM_LOG("NVM_DataVerConvert:the file lid is greater than the max file lid, reset it\n");
            if(!NVM_ResetFileToDefault(file_lid)) {
                NVRAM_LOG("reset the file fail\n");
                close(iFileDesc);
                return 0;
            }
            close(iFileDesc);
            return 1;
        }
    } else {
        if(((file_lid+1) * FILEVERLENGTH)>st.st_size) {
            NVRAM_LOG("NVM_DataVerConvert:the file lid is greater than the max file lid, reset it\n");
            if(!NVM_ResetFileToDefault(file_lid)) {
                NVRAM_LOG("reset the file fail\n");
                close(iFileDesc);
                return 0;
            }
            close(iFileDesc);
            return 1;
        }

    }

    pCfgFielTable = NVM_GetCfgFileTable(file_lid);
    if (pCfgFielTable == NULL) {
        NVRAM_LOG("NVM_GetCfgFileTable Fail!!!\n");
        close(iFileDesc);
        return 0;
    }

    if (file_lid >= iCustomBeginLID) {
        iRealFileLid = file_lid - iCustomBeginLID;
    } else {
        iRealFileLid = file_lid;
    }

    if(bIsNewVerInfo) {
        if(-1==lseek(iFileDesc, (file_lid+1) * (FILEVERLENGTH+FILENAMELENGTH)+FILENAMELENGTH, SEEK_SET)) {
            NVRAM_LOG("Error NVM_DataVerConvert lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
            close(iFileDesc);
            return 0;
        }
    } else {
        if(-1==lseek(iFileDesc, file_lid * FILEVERLENGTH, SEEK_SET)) {
            NVRAM_LOG("Error NVM_DataVerConvert lseek version file fail: %s\n", g_akCFG_File[iFileVerInfoLID].cFileName);
            close(iFileDesc);
            return 0;
        }
    }

    read(iFileDesc, pFileVerInfo, FILEVERLENGTH);
    close(iFileDesc);

    iNewFileVerNO=atoi(pCfgFielTable[iRealFileLid].cFileVer);
    iCurrentFileVerNO=atoi(pFileVerInfo);

    if(stat(pCfgFielTable[iRealFileLid].cFileName,&st)<0) {
        NVRAM_LOG("Error NVM_DataVerConvert stat \n");
        return 0;
    }

    iFileSize=st.st_size;
    iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_RDWR);
    if (iFileDesc == -1) {
        NVRAM_LOG("Error NVM_DataVerConvert Open2  file fail: %s\n", pCfgFielTable[iRealFileLid].cFileName);
        return 0;
    }
    pSrcMem=(char*)malloc(iFileSize);
    if(pSrcMem==NULL) {
        NVRAM_LOG("Error NVM_DataVerConvert malloc \n");
        close(iFileDesc);
        return 0;
    }
    memset(pSrcMem,0,iFileSize);
    if(read(iFileDesc, pSrcMem , iFileSize) < 0) {
        NVRAM_LOG("Read NVRAM fails %d\n", errno);
        close(iFileDesc);
        free(pSrcMem);
        return 0;
    }

    rec_size=pCfgFielTable[iRealFileLid].i4RecSize;
    rec_num=pCfgFielTable[iRealFileLid].i4RecNum;
    pDstMem=(char*)malloc(rec_size*rec_num);
    if(pDstMem==NULL) {
        NVRAM_LOG("Error NVM_DataVerConvert malloc2 \n");
        free(pSrcMem);
        close(iFileDesc);
        return 0;
    }
    memset(pDstMem,0,rec_size*rec_num);

    if(pCfgFielTable[iRealFileLid].NVM_DataConvertFunc!=NULL) {
        iResult=pCfgFielTable[iRealFileLid].NVM_DataConvertFunc(iCurrentFileVerNO,iNewFileVerNO,pSrcMem,pDstMem);
        if(iResult!=1) {
            NVRAM_LOG("Error NVM_DataVerConvert fail \n");
            free(pSrcMem);
            free(pDstMem);
            close(iFileDesc);
            return 0;
        }
    } else {
        NVRAM_LOG("Error META_ConverDataFunction doesn't exist \n");
        free(pSrcMem);
        free(pDstMem);
        close(iFileDesc);
        return 0;
    }
    close(iFileDesc);
    //Clear the old content of the file
    iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_TRUNC| O_RDWR|O_SYNC);
    if (iFileDesc == -1) {
        NVRAM_LOG("Error NVM_DataVerConvert Open2  file fail: %s\n", pCfgFielTable[iRealFileLid].cFileName);
        free(pSrcMem);
        free(pDstMem);
        return 0;
    }
    //NVRAM_LOG("rec_num:%d,%d\n",rec_num,rec_size);
    if (write(iFileDesc,pDstMem , rec_num*rec_size) < 0) {
        printf("WriteFile bt nvram failed:%d\r\n", errno);
        free(pSrcMem);
        free(pDstMem);
        close(iFileDesc);
        return 0;
    }
    free(pSrcMem);
    free(pDstMem);
    close(iFileDesc);
    if(iResult==1) {
        //if Data convert successfully, the data version info in ver file should also be changed.
        NVM_UpdateFileVerNo(file_lid, VerUpdate);
    }

    if(NVM_ProtectDataFile(file_lid,setflag) == 1) {
        NVRAM_LOG("NVM_ResetFileToDefault ProtectDataFile Success!!\n");
    } else {
        NVRAM_LOG("NVM_ResetFileToDefault ProtectDataFile SET Fail!!\n");
        return false;
    }

    return iResult;

}
static bool NVM_CheckFileNum(unsigned int iOldCommonFileNum,unsigned int iOldCustomFileNum)
{
    unsigned int iLid=0;
    int iNewCustomFileNum=iNvRamFileMaxLID-iCustomBeginLID;
    if((iOldCommonFileNum==0)||(iOldCustomFileNum==0)) {
        NVRAM_LOG("File Num is zero\n");
        return true;
    }
    if((iOldCommonFileNum==(unsigned int)iCustomBeginLID)&&(iOldCustomFileNum==(unsigned int)iNewCustomFileNum)) {
        NVRAM_LOG("File Num matches\n");
        return true;
    } else {
        if(iOldCommonFileNum<(unsigned int)iCustomBeginLID&&iOldCommonFileNum>0) {
            for(iLid=iOldCommonFileNum; iLid<(unsigned int)iCustomBeginLID; iLid++) {
                if(!NVM_UpdateFileVerNo(iLid,VerAdd)) {
                    NVRAM_LOG("NVM_CheckFileNum: Update File Fail:%d\n",iLid);
                    return false;
                }
            }
        }

        if(iOldCustomFileNum<(unsigned int)iNewCustomFileNum&&iOldCustomFileNum>0) {
            for(iLid=(iCustomBeginLID+iOldCustomFileNum); iLid<(unsigned int)(iCustomBeginLID+iNewCustomFileNum); iLid++) {
                if(!NVM_UpdateFileVerNo(iLid,VerAdd)) {
                    NVRAM_LOG("NVM_CheckFileNum: Update File Fail:%d\n",iLid);
                    return false;
                }
            }
        }

        if(iOldCommonFileNum>(unsigned int)iCustomBeginLID) {
            for(iLid=(unsigned int)iCustomBeginLID; iLid<iOldCommonFileNum; iLid++) {
                if(!NVM_UpdateFileVerNo(iLid,VerDel)) {
                    NVRAM_LOG("NVM_CheckFileNum: Update File Version Fail:%d\n",iLid);
                    return false;
                }
            }
        }

        if(iOldCustomFileNum>(unsigned int)iNewCustomFileNum) {
            for(iLid=(iOldCommonFileNum+iNewCustomFileNum); iLid<(iOldCommonFileNum+iOldCustomFileNum); iLid++) {
                if(!NVM_UpdateFileVerNo(iLid,VerDel)) {
                    NVRAM_LOG("NVM_CheckFileNum: Update File Version Fail:%d\n",iLid);
                    return false;
                }
            }
        }

    }

    return true;
}
/********************************************************************************
//FUNCTION:
//		NVM_DataVerConvertAll
//DESCRIPTION:
//		this function is called to convert all data acccording to the version info .
//
//PARAMETERS:
//		file_lid: [IN] the lid of nvram file
//RETURN VALUE:
//		convert successfully?
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_DataVerConvertAll(unsigned int iOldCommonFileNum,unsigned int iOldCustomFileNum)
{
    int iLID=1;
    int iFileDesc=0;
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid;
    bool bWrongFileVer=false;
    NVRAM_LOG("Enter DataConvert All\n");
    if(iOldCommonFileNum==0 || iOldCustomFileNum==0) {
        NVRAM_LOG("File Num is zero\n");
        return true;
    }
    if (!NVM_CheckVerFile(true)) {
        NVRAM_LOG("fatal error: cna't find version info file\n");
        return false;
    }
    if (!NVM_CheckFileNum(iOldCommonFileNum,iOldCustomFileNum)) {
        NVRAM_LOG("NVM_CheckFileNum fail\n");
        return false;
    }
    for(iLID=1; iLID<iNvRamFileMaxLID; iLID++) {
        bool bFileExist=true;
        struct stat st;
        memset(&st,0,sizeof(st));
        pCfgFielTable = NVM_GetCfgFileTable(iLID);
        if (pCfgFielTable == NULL) {
            NVRAM_LOG("NVM_GetCfgFileTable Fail!!!\n");
            return false;
        }

        if (iLID >= iCustomBeginLID) {
            iRealFileLid = iLID - iCustomBeginLID;
        } else {
            iRealFileLid = iLID;
        }
        if(stat(pCfgFielTable[iRealFileLid].cFileName,&st)<0) {
            NVRAM_LOG("this file doesn't exist\n");
            bFileExist=false;
        }

        if(bFileExist) {
            if(!NVM_CmpFileVerNo(iLID)) {
                NVRAM_LOG("DataConvert Begin\n");
                if(0==NVM_DataVerConvert(iLID)) {
                    //Clear the old content of the file
                    iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_TRUNC|O_RDWR);
                    if(iFileDesc==-1) {
                        NVRAM_LOG("NVM_Clear File Content Fail\n");
                        return false;
                    }
                    close(iFileDesc);

                    NVRAM_LOG("Data Convert Fail,reset the file\n");
                    if(!NVM_ResetFileToDefault(iLID)) {
                        NVRAM_LOG("reset the file fail\n");
                        return false;
                    }

                }
                NVRAM_LOG("DataConvert End\n");
            } else {
                //if version info is the same ,then output the size info
                NVRAM_LOG("Compare the size of same vesion file\n");

                int iLoadFileSize=(pCfgFielTable[iRealFileLid].i4RecSize)*(pCfgFielTable[iRealFileLid].i4RecNum);
                NVRAM_LOG("Compare the size,the load file size:%d, the nvram file size:%lld\n",iLoadFileSize, st.st_size);
                if(iLoadFileSize!=st.st_size && iLoadFileSize+2*(int)sizeof(char)!=st.st_size) {
                    NVRAM_LOG("NvRam data size can't match between version \n");
                    bWrongFileVer=true;
                }

            }

        }
    }
    NVRAM_LOG("Leave DataConvert All\n");
    if(bWrongFileVer==true)
        return false;
    return true;
}

/********************************************************************************
//FUNCTION:
//		NVM_GetFileDesc
//DESCRIPTION:
//		this function is called to the desc of nvram file and the information
//      of record size and number.
//
//PARAMETERS:
//		file_lid: 	[IN]	the lid of nvram file
//		pRecSize: 	[OUT] 	the record size
//		pRecNum: 	[OUT] 	the rocord number
//		IsRead: 	[IN]	true is read, otherwise is write
//
//RETURN VALUE:
//		the file file desc
//
//DEPENDENCY:
//		None
//
//GLOBALS AFFECTED
//		None
********************************************************************************/

F_ID NVM_GetFileDesc(int file_lid, int *pRecSize, int *pRecNum, bool IsRead)
{
    int iFileDesc;
    int iResult;
    const TCFG_FILE *pCfgFielTable = NULL;
    int iRealFileLid = 0;
    bool Checkflag = 0;
    F_ID FileID;
    FileID.iFileDesc=-1;
    FileID.ifile_lid=file_lid;
    FileID.bIsRead = IsRead;

    unsigned int index = 0;
    bool maskflag = 0;
    bool bRet = false;
    int index_in_list = -1;
    struct mtd_info_user info;
    int i = 0;
    char cMtdDevName[64] = {0};
    struct erase_info_user erase_info;

    NVRAM_LOG("NVM_GetFileDesc lid:%d\n", file_lid);
    for (index = 0; index < g_Performance_File_Count; index++) {
        if ((unsigned int)file_lid == aPerformance[index].iLID) {
            maskflag = 1;
            break;
        }
    }
    NVM_Init();
    pCfgFielTable = NVM_GetCfgFileTable(file_lid);
    //if (file_lid < AP_CFG_CUSTOM_BEGIN_LID) {
    if (file_lid < iCustomBeginLID) {
        iRealFileLid = file_lid;
    } else {
        //iRealFileLid = file_lid - AP_CFG_CUSTOM_BEGIN_LID;
        iRealFileLid = file_lid - iCustomBeginLID;
    }

    if (pCfgFielTable == NULL) {
        NVRAM_LOGE("NVM_GetCfgFileTable Fail!!! \n");
        return FileID;
    }

    if(nvram_new_partition_support()) {
        if(NVM_InSpecialLidList(file_lid, &index_in_list)) {
            if(pRecSize!=NULL&&pRecNum!=NULL) {
                *pRecSize = pCfgFielTable[iRealFileLid].i4RecSize;
                *pRecNum  = pCfgFielTable[iRealFileLid].i4RecNum;
            }
            goto Solve_Special_Lid;
        }
    }

    NVRAM_LOG("Open %s,LID:%d\n",pCfgFielTable[iRealFileLid].cFileName,file_lid);
    //Check if the lid is reserved.
    if (strcmp(pCfgFielTable[iRealFileLid].cFileName,"Reserved") == 0) {
        NVRAM_LOG("The LID is Reserved, please check lid you used!!!");
        return FileID;
    }
    if (IsRead) {
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_RDONLY);
    } else {
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_RDWR);
    }
    //get file RecSize and RecNum information
    if(pRecSize!=NULL&&pRecNum!=NULL) {
        *pRecSize = pCfgFielTable[iRealFileLid].i4RecSize;
        *pRecNum  = pCfgFielTable[iRealFileLid].i4RecNum;
    }
    if (iFileDesc == -1) { //if file doesn't exist
        if(access(g_pcNVM_Flag, F_OK) == 0) {
            NVRAM_LOG("File is not exist, try to restore from binregion!!!");
            for(i=0; i<(int)g_Backup_File_Count; i++) {
                if((unsigned int)file_lid == aBackupToBinRegion[i].iLID) {
                    if(NVM_RestoreFromBinRegion_OneFile(file_lid, NULL)) {
                        NVRAM_LOG("successfully restore.\n");
                        goto ProtectData;
                    }
                    break;
                }
            }
        }
        NVRAM_LOG("Create the dir path of %s\n", pCfgFielTable[iRealFileLid].cFileName);
        iFileDesc = open_file_with_dirs(pCfgFielTable[iRealFileLid].cFileName, O_CREAT | O_RDWR,S_IRUSR | S_IWUSR | S_IRGRP |S_IWGRP);//create the file
        if (iFileDesc == -1) {
            NVRAM_LOG("Error Num %s\n",(char*)strerror(errno));
            NVRAM_LOG("Error NVM_GetFileDesc open_file_with_dirs file fail: %s\n", pCfgFielTable[iRealFileLid].cFileName);
        }
        close(iFileDesc);
        if (!NVM_ResetFileToDefault(file_lid)) {
            NVRAM_LOG("ResetFileToDefault Failed\n");
            return FileID;
        }
    } else {
        close(iFileDesc);//avoid the bug of re-open file
        if (!NVM_CmpFileVerNo(file_lid)) {
            // if file version is not same, convert it.
            if(pCfgFielTable[iRealFileLid].bDataProcessingType==DataReset) { //Reset Data
                NVRAM_LOG("NVM_CmpFileVerNo Fail: Reset!!! \n");
                iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_TRUNC);
                if(iFileDesc==-1) {
                    NVRAM_LOG("NVM_Clear File Content Fail_Get File Desc \n");
                    return FileID;
                }
                close(iFileDesc);
                if (!NVM_ResetFileToDefault(file_lid))
                    return FileID;
            } else {
                NVRAM_LOG("NVM_CmpFileVerNo Fail: Convert!!! \n");
                //only for test
                iResult=NVM_DataVerConvert(file_lid);
                if(iResult==0) {
                    iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_TRUNC);
                    if(iFileDesc==-1) {
                        NVRAM_LOG("NVM_Clear File Content Fail_Get File Desc \n");
                        return FileID;
                    }
                    close(iFileDesc);
                    if (!NVM_ResetFileToDefault(file_lid))
                        return FileID;
                }
            }
        }

    }
ProtectData:
    if (maskflag == 0) {
        if(NVM_ProtectDataFile(file_lid,Checkflag) == 1) {
            //NVRAM_LOG("NVM_GetFileDesc ProtectDataFile Check Success!!\n");
        } else {
            NVRAM_LOGE("NVM_GetFileDesc ProtectDataFile Fail!!\n");
            return FileID;
        }
    }
    if (IsRead) {
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_RDONLY);
        if(flock(iFileDesc, LOCK_SH) < 0) {
            NVRAM_LOG("SH lock on %d err: %d\n", file_lid, errno);
        } else {
            //NVRAM_LOG("SH lock on %d\n", file_lid);
        }
    } else {
        iFileDesc = open(pCfgFielTable[iRealFileLid].cFileName, O_RDWR|O_SYNC);
        if(flock(iFileDesc, LOCK_EX) < 0) {
            NVRAM_LOG("EX lock on %d err: %d\n", file_lid, errno);
        } else {
            //NVRAM_LOG("EX lock on %d\n", file_lid);
        }
    }
    NVRAM_LOG("NVM_GetFileDesc --\n");
    FileID.iFileDesc = iFileDesc;
    return FileID;


Solve_Special_Lid:
    memset(cMtdDevName, 0, sizeof cMtdDevName);
    if (nvram_gpt_flag == 1)
        sprintf(cMtdDevName, NVRAM_PROINFO_DEVICE_GPT);
    else
        sprintf(cMtdDevName, NVRAM_PROINFO_DEVICE);
    NVRAM_LOG("New NVRAM partition name is %s.\n", cMtdDevName);
    bRet = NVM_GetDeviceInfo(cMtdDevName, &info);
    if (false == bRet) {
        NVRAM_LOG("NVM_GetFileDesc: get device info fail!!!\n");
        return FileID;
    }

    if(g_new_nvram_lid[index_in_list].start_address % info.erasesize != 0 || g_new_nvram_lid[index_in_list].size % info.erasesize != 0 ||g_new_nvram_lid[index_in_list].start_address + g_new_nvram_lid[index_in_list].size> info.size) {
        NVRAM_LOG("Lid info in special info is error!!!\n");
        return FileID;
    }

    if(!nvram_emmc_support()) {
        if(*pRecSize < info.writesize) {
            NVRAM_LOG("*pRecSize %d < info.writesize %d\n",*pRecSize,info.writesize);
            *pRecSize = info.writesize;
        }
    }

    if(*pRecSize % info.writesize != 0) {
        NVRAM_LOG("Please make sure size for special lid in new nvram partition should alignment %d\n", info.writesize);
        return FileID;
    }

    if(IsRead) {
        iFileDesc = open(cMtdDevName, O_RDONLY);
    } else {
        if(!nvram_emmc_support()) {
            erase_info.start = g_new_nvram_lid[index_in_list].start_address;
            erase_info.length = g_new_nvram_lid[index_in_list].size;
            bRet = NVM_EraseDeviceBlock(cMtdDevName, erase_info);
            if( false == bRet) {
                NVRAM_LOG("NVM_GetFileDesc: erase device failed");
                return FileID;
            }
        }

        iFileDesc = open(cMtdDevName, O_RDWR|O_SYNC);
        if(iFileDesc < 0) {
            NVRAM_LOG("Open new nvram partition fail!!!\n");
            return FileID;
        }
    }

    if(lseek(iFileDesc, g_new_nvram_lid[index_in_list].start_address, SEEK_SET) < 0) {
        NVRAM_LOG("seek for lid %d fail!!!\n", file_lid);
        return FileID;
    }
    FileID.iFileDesc = iFileDesc;
    return FileID;

}


/********************************************************************************
//FUNCTION:
//		NVM_CloseFileDesc
//DESCRIPTION:
//		this function is called to close the file desc which is open by NVM_GetFileDesc.
//
//PARAMETERS:
//		hFile: 	[IN] the file desc
//
//RETURN VALUE:
//		true is success, otherwise is fail
//
//DEPENDENCY:
//		GetFileDesc must have been called
//
//GLOBALS AFFECTED
//		None
********************************************************************************/
bool NVM_CloseFileDesc(F_ID FileID)
{
    int index_in_list = -1;
    bool Setflag=1;
    NVRAM_LOG("NVM_CloseFileDesc ++\n");
    if(nvram_new_partition_support()) {
        if(NVM_InSpecialLidList(FileID.ifile_lid, &index_in_list)) {
            //the handle of pro info shoud be closed
            if (FileID.iFileDesc == -1) {
                return false;
            }

            if(0 != close(FileID.iFileDesc)) {
                NVRAM_LOG("NVM_CloseFileDesc Close file error!!\n");
                return false;
            }
            return true;
        }
    }

    if (FileID.iFileDesc== -1) {
        return false;
    }

    if (!FileID.bIsRead) {
        if(0 != fsync(FileID.iFileDesc)) {
            NVRAM_LOGE("NVM_CloseFileDesc fsync file error!!\n");
            return false;
        }
    }

    if(0 != close(FileID.iFileDesc)) {
        NVRAM_LOGE("NVM_CloseFileDesc Close file error!!\n");
        return false;
    }

    bool maskflag = 0;
    unsigned int index = 0;
    for (index = 0; index < g_Performance_File_Count; index++) {
        if ((unsigned int)FileID.ifile_lid == aPerformance[index].iLID) {
            maskflag = 1;
            break;
        }
    }

    if (maskflag)
        return true;

    if(true == FileID.bIsRead) {
        NVRAM_LOG("NVM_CloseFileDesc: Open by Readonly, no need to check when close\n");
        return true;
    }
    if(NVM_ProtectDataFile(FileID.ifile_lid,Setflag) == 1) {
        NVRAM_LOG("NVM_CloseFileDesc ProtectDataFile Success!!\n");
        return true;
    } else {
        NVRAM_LOGE("NVM_CloseFileDesc ProtectDataFile SET Fail!!\n");
        return false;
    }
}

bool NVM_InSpecialLidList(int file_lid, int *index)
{
    int i;
    if(g_new_nvram_lid_count == 0) {
        NVRAM_LOG("The spcial lid is empty!!!\n");
        return false;
    }

    for(i = 0; i < g_new_nvram_lid_count; i++) {
        if(file_lid == g_new_nvram_lid[i].lid) {
            *index = i;
            break;
        }
    }

    if(i == g_new_nvram_lid_count)
        return false;
    else {
        NVRAM_LOG("%d is in new nvram partition!!!\n", file_lid);
        return true;
    }
}

bool Check_FileVerinFirstBoot(void)
{
    int ret;
    int max_lid_num = NVM_Init();
    struct stat statbuf;
    NVRAM_LOG("Check FILE_VER in first boot\n");
    ret=stat(g_pcNVM_Flag,&statbuf);
    if(-1 == ret) {
        NVRAM_LOG("No RestoreFlag\n");
        if (stat(g_akCFG_File[iFileVerInfoLID].cFileName, &statbuf) == -1
            || !S_ISREG(statbuf.st_mode)
            || statbuf.st_size != ((FILENAMELENGTH + FILEVERLENGTH) * (max_lid_num + 1))) {
            NVRAM_LOG("FILE_VER is invalid, generate it\n");
            return NVM_GenerateFileVer(true);
        }
    }
    return true;
}

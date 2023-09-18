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
#ifndef __NVRAM_LIB_H
#define __NVRAM_LIB_H

#include "CFG_file_public.h"
#include <stdbool.h>
#include <time.h>
#include <mtd/mtd-abi.h>
#include <sys/syscall.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef bool
#define bool int
#define false 0
#define true 1
#endif

extern char* __progname;

#define NVRAM_BACKUP_DEVICE "/dev/mtd1"
#define NVRAM_BACKUP_DEVICE_GPT  "/dev/disk/by-partlabel/nvram"
#define NVRAM_PROINFO_DEVICE "/dev/pro_info"
#define NVRAM_PROINFO_DEVICE_GPT "/dev/block/platform/mtk-msdc.0/by-name/proinfo"

typedef struct {
    char 	cFileVer[FILEVERLENGTH];
    char 	cFileName[FILENAMELENGTH];
    int 	i4RecSize;
    int 	i4RecNum;
    int 	i4MaxFileLid;
} F_INFO;

typedef struct {
    int iFileDesc;
    int ifile_lid;
    bool bIsRead;
} F_ID;

typedef enum {
    VerUpdate,
    VerDel,
    VerAdd,
} VerInfoUpdateFlag;

#define ISREAD      1
#define ISWRITE     0

#ifndef NDEBUG
#define NVRAM_LOG(format,...) ({printf("%d %d %s NVRAM: " format, getpid(), syscall(SYS_gettid),__progname, ##__VA_ARGS__);})
#else
#define NVRAM_LOG(...) ((void)0)
#endif

//error log
#define NVRAM_LOGE(format,...) ({fprintf(stderr, "%d %d %s NVRAM: " format, getpid(), syscall(SYS_gettid), __progname, ##__VA_ARGS__);})

int NVM_Init(void);
F_ID NVM_GetFileDesc(int file_lid, int *pRecSize, int *pRecNum, bool IsRead);
bool NVM_CloseFileDesc(F_ID iFileID);
int NVM_GetLIDByName(char* filename);
bool Check_FileVerinFirstBoot(void);

#ifdef __cplusplus
}
#endif

#endif //__NVRAM_LIB_H

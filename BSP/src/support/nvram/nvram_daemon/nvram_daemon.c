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
 * MediaTek Inc. (C) 2010. All rights reserved.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "libnvram.h"
#include "nvram_bt.h"
#include "nvram_wifi.h"
#include "nvram_productinfo.h"
#include "nvram_ethernet.h"
#include "libfile_op.h"

#if defined(VA_SUPPORT_GVA_SDK_SUPPORT) || defined(AUDIO_SUPPORT_C4A_SDK_SUPPORT)
#include <fcntl.h>
#include "libnvram.h"
#include "CFG_file_lid.h"
#include "Custom_NvRam_LID.h"
#include "CFG_Wifi_File.h"
#include "CFG_BT_File.h"
#include "CFG_PRODUCT_INFO_File.h"
#include "libfile_op.h"

#define SN_LEN (23)
#define PROPERTY_ISERIAL_FOLDER "/data/factory"
#define PROPERTY_ISERIAL_FILE "/data/factory/serial.txt"
#endif

#define NU_MAX 4
typedef struct _NVRAM_USER_DATA {
    const char*	pcName;
    pthread_t	kThread;
    void* (*start_routine)(void*);
} NVRAM_USER_DATA;

static NVRAM_USER_DATA g_akNUData[NU_MAX] = {
    {"Bluetooth", 0, NVRAM_BT},
    {"WIFI", 0, NVRAM_WIFI},
    {"ETHERNET", 0, NVRAM_ETHERNET},
    {"None", 0, NULL}
};

bool Init_AllThreads_In_Nvram(void)
{
    int i, ret;
    void* thread_result;
    for (i = 0; i < NU_MAX; ++i) {
        if (g_akNUData[i].start_routine) {
            ret = pthread_create(&g_akNUData[i].kThread, NULL, g_akNUData[i].start_routine, NULL);
            if (ret != 0) {
                return false;
            }
        }
    }

    for (i = NU_MAX - 1; i >= 0 ; --i) {
        if (g_akNUData[i].start_routine) {
            ret = pthread_join(g_akNUData[i].kThread, &thread_result);
            if (ret != 0) {
                return false;
            }
        }
    }
    return true;
}

#if defined(VA_SUPPORT_GVA_SDK_SUPPORT) || defined(AUDIO_SUPPORT_C4A_SDK_SUPPORT)
static bool r_SN(char* sn) 
{
	F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, true /*read*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }

    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);

    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }

    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
	free(pBuf);
        return false;
    }

    NVRAM_LOG("r_SN pBuf = %s.\n", pBuf);
    memcpy(sn, pBuf, strlen(pBuf));
    NVRAM_LOG("r_SN sn = %s.\n", sn);

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
	free(pBuf);
    return true;
}

static bool w_SN(char* sn) {
    F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, false /*write*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }
    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);
    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }
    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
	free(pBuf);
        return false;
    }
    /// to-do
    memcpy(pBuf, sn, strlen(sn));

    if(lseek(fid.iFileDesc, 0, SEEK_SET) < 0) {
        fprintf(stderr,"Seek PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
	free(pBuf);
        return false;
    }

    if (write(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Write PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
	free(pBuf);
        return false;
    }

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
    free(pBuf);
    return true;
}

static void writeToFile(char *buf)
{
	char buf_prop[SN_LEN] = {0};

	NVRAM_LOG("write  %s to %s.\n", buf, PROPERTY_ISERIAL_FILE);
	if (access(PROPERTY_ISERIAL_FOLDER, F_OK) != 0)
	{
		NVRAM_LOG("no /data/factory, should create it.\n");
		mkdir(PROPERTY_ISERIAL_FOLDER, 0777);
	}
	int fd = open(PROPERTY_ISERIAL_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		NVRAM_LOG("writeToFile fail.%d\n", fd);
		return -1;
	}
	int length = (strlen(buf) > SN_LEN)? SN_LEN : strlen(buf);
	write(fd, buf, length);
	NVRAM_LOG("write to %s successfully.\n", PROPERTY_ISERIAL_FILE);
	close(fd);
}
#endif


int main(void)
{
    int iMaxLidNum = NVM_Init();

    NVRAM_LOG("Total nvram Lid number is %d\n", iMaxLidNum);
    umask(007);
    if (iMaxLidNum == 0) {
        NVRAM_LOG("Total nvram Lid number is 0\n");
        exit(EXIT_FAILURE);
    }

    mkdir("/data/nvram", 0660);
    if(!FileOp_RecoveryData()) {
        NVRAM_LOG("Bin Region Restore to 'data/nvram/' Fail\n");
    }

    if(!Check_FileVerinFirstBoot()) {
        NVRAM_LOG("Check FILE_VER\n");
    }

    if (!Init_AllThreads_In_Nvram())
        NVRAM_LOG("init all threads fail\n");
#if defined(VA_SUPPORT_GVA_SDK_SUPPORT) || defined(AUDIO_SUPPORT_C4A_SDK_SUPPORT)
	char SN[SN_LEN + 1];
	memset(SN, 0, SN_LEN + 1);

	if (r_SN(SN) == true)
	{
		NVRAM_LOG("wangxy r_SN(SN) is successfully.strlen(SN) = %d\n", strlen(SN));
		if (0 == strlen(SN))
		{
			NVRAM_LOG("SN is NULL.\n");

			if(w_SN("123456789ABCDEF")) 
			{
				NVRAM_LOG(" write SN successfully.\n");
			}
			memset(SN, 0, SN_LEN + 1);
			r_SN(SN);
			NVRAM_LOG("the sn is: %s\n", SN);
			writeToFile(SN);
		}
		else
		{
			NVRAM_LOG("the serial: %s\n", SN);
			writeToFile(SN);
		}
	}
#endif
    NVRAM_LOG("NVRAM daemon sync start !\n");
    sync();
    NVRAM_LOG("NVRAM daemon sync end !\n");
    exit(EXIT_SUCCESS);
}

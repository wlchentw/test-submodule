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
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "libnvram.h"
#include "CFG_Wifi_File.h"

extern int iFileWIFILID;
extern int iFileCustomWIFILID;


/* Just tell NVRAM to create wifi default record */
void* NVRAM_WIFI(void* arg)
{
    F_ID wifi_nvram_fd;
    int rec_size = 0;
    int rec_num = 0;
    WIFI_CFG_PARAM_STRUCT wifi_nvram;

    wifi_nvram_fd = NVM_GetFileDesc(iFileWIFILID, &rec_size, &rec_num, true);
    NVRAM_LOG("[wifi] wifi FD %d rec_size %d rec_num %d\n", wifi_nvram_fd.iFileDesc, rec_size, rec_num);
    if(wifi_nvram_fd.iFileDesc > 0 ) {
        if(read(wifi_nvram_fd.iFileDesc, &wifi_nvram, rec_num*rec_size) < 0) {
            NVRAM_LOG("Read NVRAM fails %d\n", errno);
            NVM_CloseFileDesc(wifi_nvram_fd);
            return NULL;
        }
        NVRAM_LOG("Read NVRAM: [WiFi mac %02x-%02x-%02x-%02x-%02x-%02x]\n",
            wifi_nvram.aucMacAddress[0],
            wifi_nvram.aucMacAddress[1],
            wifi_nvram.aucMacAddress[2],
            wifi_nvram.aucMacAddress[3],
            wifi_nvram.aucMacAddress[4],
            wifi_nvram.aucMacAddress[5]);
        NVM_CloseFileDesc(wifi_nvram_fd);
    }

    return NULL;
}

/**
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2016. All rights reserved.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <fcntl.h>

#define WIFI_FW_LOG_NODE  "/dev/fw_log_wifi"
#define WIFI_FW_LOG_PATH  "/data/misc/stp_dump"
#define DUMP_WIFI_FW_NAME_EXT ".wifi"
#define DUMP_WIFI_FW_NAME_PREFIX "dump_"
#define WIFI_FW_LOG_SIZE  (1 * 1024 * 1024)
#define WIFI_FW_LOG_BUF_SIZE  (1944)    // Cover old WIFI driver return size
#define LOG_VERSION 0x100

#define TRUE    (1)
#define FALSE   (0)

static const unsigned long BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
static unsigned long timestamp = 0;
static unsigned int dump_file_seq_num = 0;
static int file_size_remain_to_switch = 0;
static uint8_t buffer[WIFI_FW_LOG_BUF_SIZE] = {0};
static int log_file_num = 6;
static int logger_on = 1;

static pthread_t wifi_fw_logger_thread = 0;
static int wifi_fw_log_fd = 0;

static void remove_old_log_files(int all, int index)
{
    /* check already exist file under log_path */
    char temp_wifi_fw_filename[36] = {0};
    char wifi_fw_fullname[256] = {0};

    DIR *p_dir = opendir(WIFI_FW_LOG_PATH);
    if (p_dir != NULL)
    {
        struct dirent *p_file;
        while ((p_file = readdir(p_dir)) != NULL)
        {
            /* ignore . and .. directory */
            if (strncmp(p_file->d_name, "..", 2) == 0
                || strncmp(p_file->d_name, ".", 1) == 0)
            {
                continue;
            }
            memset(temp_wifi_fw_filename, 0, sizeof(temp_wifi_fw_filename));
            memset(wifi_fw_fullname, 0, sizeof(wifi_fw_fullname));
            if (strstr(p_file->d_name, DUMP_WIFI_FW_NAME_EXT) != NULL)
            {
                if (all)    //remove all old log files
                {
                    snprintf(wifi_fw_fullname, sizeof(wifi_fw_fullname), "%s/%s", WIFI_FW_LOG_PATH, p_file->d_name);
                    if (remove(wifi_fw_fullname)) {
                        printf("[wifi_fw_logger]The old log:%s can't remove, errno: %d\n", p_file->d_name, errno);
                    }
                    else {
                        printf("[wifi_fw_logger]The old log: %s is removed\n", p_file->d_name);
                    }
                }
                else    //remove a specific log file
                {
                    snprintf(temp_wifi_fw_filename, sizeof(temp_wifi_fw_filename), "_%d.wifi", index);
                    if (strstr(p_file->d_name, temp_wifi_fw_filename) != NULL) {
                        snprintf(wifi_fw_fullname, sizeof(wifi_fw_fullname), "%s/%s", WIFI_FW_LOG_PATH, p_file->d_name);
                        if (remove(wifi_fw_fullname)) {
                        printf("[wifi_fw_logger]The old log: %s can't remove, errno: %d\n", p_file->d_name, errno);
                        } else {
                            printf("[wifi_fw_logger]The old log: %s is removed\n", p_file->d_name);
                        }
                    }
                }
            }
        }
        closedir(p_dir);
    }
    else
    {
        printf("[wifi_fw_logger]readdir fail, errno: %d\n", errno);
    }
}

static unsigned long btsnoop_timestamp(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // Timestamp is in microseconds.
    timestamp = tv.tv_sec * 1000000ULL;
    timestamp += tv.tv_usec;
    timestamp += BTSNOOP_EPOCH_DELTA;
    return timestamp;
}

static void fillheader(unsigned char *header, int headerlen,
        unsigned short int dump_file_seq_num)
{
    int copy_header_len = 0;
    unsigned int logversion = htobe32(LOG_VERSION);
    memset(header, 0, headerlen);
    memcpy(header, &logversion, sizeof(logversion));
    copy_header_len += 4;   /** 4 byte for logversion */
    copy_header_len += 4;   /** 4 byte for chip id, not implement yet */
    dump_file_seq_num = htobe16(dump_file_seq_num);
    memcpy(header + copy_header_len, &dump_file_seq_num, sizeof(dump_file_seq_num));
    copy_header_len += 2;   /** 2 byte for sequence number */
    copy_header_len += 6;   /** first hci log length(2), zero(4) */
    btsnoop_timestamp();
    timestamp = htobe64(timestamp);
    memcpy(header + copy_header_len, &timestamp, sizeof(timestamp));
}

static FILE * create_new_log_file(int index)
{
    time_t local_timestamp;
    char timestamp_buffer[24];
    char dump_file_name[128] = {0};
    unsigned char header[24] = {0};
    unsigned char padding[8] = {0};
    FILE *fp = NULL;

    /* get current timestamp */
    time(&local_timestamp);
    strftime(timestamp_buffer, 24, "%Y%m%d%H%M%S", localtime(&local_timestamp));
    snprintf(dump_file_name, sizeof(dump_file_name), "%s/" DUMP_WIFI_FW_NAME_PREFIX "%s_%d" DUMP_WIFI_FW_NAME_EXT, WIFI_FW_LOG_PATH, timestamp_buffer, index);

    /* dump file for picus log */
    if ((fp = fopen(dump_file_name, "wb")) == NULL) {
        printf("[wifi_fw_logger]create log file %s fail [%s] errno %d\n", dump_file_name, strerror(errno), errno);
        return NULL;
    } else {
        printf("[wifi_fw_logger]log file %s is created, dumping...\n", dump_file_name);
    }

    fillheader(header, sizeof(header), index);
    fwrite(header, 1, sizeof(header), fp);
    fwrite(padding, 1, sizeof(padding), fp);
    file_size_remain_to_switch = WIFI_FW_LOG_SIZE;

    return fp;
}

static void *wifi_fw_logger_main(void *arg)
{
    int nRead = 0;
    int nWritten = 0;
    fd_set rset;    /** For select */
    struct timeval tv;
    FILE *current_fp = NULL;
    int current_index = 0;

    printf("[wifi_fw_logger]%s, thread_id: 0x%x\n", __func__, pthread_self());

    current_fp = create_new_log_file(0);
    if (NULL == current_fp)
    {
        printf("[wifi_fw_logger]fatal error: create new log file fail\n");
        return NULL;
    }

    do
    {
        FD_ZERO(&rset);
        FD_SET(wifi_fw_log_fd, &rset);
        tv.tv_sec = 10; /* timeout is 10s for select method */
        tv.tv_usec = 0;
        if (select(wifi_fw_log_fd + 1, &rset, NULL, NULL, &tv) == 0) {
            printf("[wifi_fw_logger]Read data timeout(10s) from fw_log_wifi\n");
            continue;
        }

        if (!FD_ISSET(wifi_fw_log_fd, &rset))
            continue;
        /* Read all packet from driver fwlog queue */
        nRead = read(wifi_fw_log_fd, buffer, sizeof(buffer));
        if (nRead > 0)
        {
            nWritten = fwrite(buffer, 1, nRead, current_fp);
            if (nWritten != nRead)
            {
                printf("[wifi_fw_logger]write may fail, nRead(%d) != nWritten(%d)\n", nRead, nWritten);
            }
            file_size_remain_to_switch -= nWritten;
        }
        else if (nRead < 0)
        {
            printf("[wifi_fw_logger]read fail, errno=%d\n", errno);
            continue;
        }
        /* switch file name if file size is over file_size */
        if (file_size_remain_to_switch <= 0) {
            file_size_remain_to_switch = WIFI_FW_LOG_SIZE;
            fclose(current_fp);
            if (log_file_num - 1 > current_index) {
                current_index++;
            } else {
                current_index = 0;
            }
            /* remove the file before creating */
            remove_old_log_files(FALSE, current_index);
            current_fp = create_new_log_file(current_index);
            if (NULL == current_fp)
            {
                printf("[wifi_fw_logger]create new log file fail\n");
                //TODO
            }
        }
        fflush(current_fp);
    } while (logger_on);


    printf("[wifi_fw_logger]%s exit, thread_id: 0x%x\n", __func__, pthread_self());

    return NULL;
}


void wifi_fw_logger_start(void)
{
    printf("[wifi_fw_logger]%s\n", __func__);
    if (wifi_fw_log_fd > 0)
    {
        printf("%s, device is already opened\n", __func__);
        return;
    }
    //1. open device
    wifi_fw_log_fd = open(WIFI_FW_LOG_NODE, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (wifi_fw_log_fd <= 0) {
        printf("[wifi_fw_logger]Can't open device node %s, errno: %d\n", WIFI_FW_LOG_NODE, errno);
        return;
    } else {
        printf("[wifi_fw_logger]Open device node successfully wifi_fw_log_fd = %d\n", wifi_fw_log_fd);
    }

    remove_old_log_files(TRUE, 0);

    //2. create logger thread
    if (wifi_fw_logger_thread == 0)
    {
        pthread_attr_t thread_attr;

        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&wifi_fw_logger_thread, &thread_attr, wifi_fw_logger_main, NULL))
        {
            printf("[wifi_fw_logger]create logger thread fail\n",__func__);
            wifi_fw_logger_thread = 0;
            return;
        }
        pthread_attr_destroy(&thread_attr);
    }
}

void wifi_fw_logger_stop(void)
{
    //disable fw log first
    //system("echo raw-hex, 01 5d fc 04 02 00 01 00 > /dev/fw_log_bt");

    if (wifi_fw_log_fd > 0)
    {
        close(wifi_fw_log_fd);
    }

    //thread exit
    logger_on = 0;
    wifi_fw_logger_thread = 0;
    printf("[wifi_fw_logger]%s\n", __func__);

}
//---------------------------------------------------------------------------

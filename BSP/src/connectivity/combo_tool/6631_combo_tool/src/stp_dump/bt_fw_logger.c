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

#define BT_FW_LOG_CTRL_FIFO "/tmp/bt_fwlog_ctrl"
static int fw_log_ctrl_fd = 0;
#define FIFO_BUF_SIZE   (128)

#define BT_FW_LOG_NODE  "/dev/fw_log_bt"
#define BT_FW_LOG_DEFAULT_PATH  "/data/misc/stp_dump"

#define LOG_PATH_LENGTH   (128)
static char fw_log_path[LOG_PATH_LENGTH] = {0};
#define DUMP_PICUS_NAME_EXT ".picus"
#define DUMP_PICUS_NAME_PREFIX "dump_"
#define BT_FW_LOG_DEFAULT_SIZE  (10 * 1024 * 1024)
static uint32_t log_file_size = BT_FW_LOG_DEFAULT_SIZE;
#define BT_FW_LOG_BUF_SIZE  (1024*32)   //(1944)    // Cover old BT driver return size
#define LOG_VERSION 0x100

#define TRUE    (1)
#define FALSE   (0)

static const unsigned long BTSNOOP_EPOCH_DELTA = 0x00dcddb30f2f8000ULL;
static unsigned long timestamp = 0;
static unsigned int dump_file_seq_num = 0;
static int file_size_remain_to_switch = 0;
static uint8_t buffer[BT_FW_LOG_BUF_SIZE] = {0};
static int log_file_num = 6;
static int logger_on = 1;

static pthread_t bt_fw_logger_thread = 0;
static int fw_log_fd = 0;

#define BT_STACK_CONF_FILE "/data/misc/bluedroid/bt_stack.conf"
enum
{
    FW_LOG_LEVEL_OFF,
    FW_LOG_LEVEL_LOW_POWER,
    FW_LOG_LEVEL_SQC,
    FW_LOG_LEVEL_FULL,  //All log
    FW_LOG_LEVEL_MAX,   //invalid
};

const char *fw_log_cmd[FW_LOG_LEVEL_MAX] = {
    "echo raw-hex, 01 5d fc 04 02 00 01 00 > /dev/fw_log_bt",
    "echo raw-hex, 01 5d fc 04 02 00 01 01 > /dev/fw_log_bt",
    "echo raw-hex, 01 5d fc 04 02 00 01 02 > /dev/fw_log_bt",
    "echo raw-hex, 01 5d fc 04 02 00 01 03 > /dev/fw_log_bt"
};

static char *trim(char *str) {
  while (isspace(*str))
    ++str;

  if (!*str)
    return str;

  char *end_str = str + strlen(str) - 1;
  while (end_str > str && isspace(*end_str))
    --end_str;

  end_str[1] = '\0';
  return str;
}

static void parse_fwlog_config(void)
{
    int line_num = 0;
    char line[1024];

    printf("[bt_fw_logger]%s\n", __func__);

    FILE *fp = fopen(BT_STACK_CONF_FILE, "rt");
    if (!fp)
    {
        printf("[bt_fw_logger]%s unable to open file '%s': %s\n", __func__, BT_STACK_CONF_FILE, strerror(errno));
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        char *line_ptr = trim(line);
        ++line_num;

        // Skip blank and comment lines.
        if (*line_ptr == '\0' || *line_ptr == '#')
            continue;

        if (line_ptr)
        {
            char *split = strchr(line_ptr, '=');
            if (!split)
            {
                printf("[bt_fw_logger]%s no key/value separator found on line %d\n", __func__, line_num);
                continue;
            }
            *split = '\0';
            if (strcmp(trim(line_ptr), "mt66xx_fwlog_level") == 0)  //log level
            {
                char *level_str = trim(split + 1);
                if (level_str)
                {
                    uint32_t level = atoi(&level_str[0]);
                    if (level < FW_LOG_LEVEL_MAX)
                    {
                        printf("[bt_fw_logger]conf file set fwlog level: %d, set fwlog level to driver!!!\n", level);
                        system(fw_log_cmd[level]);
                    }
                    else
                        printf("[bt_fw_logger]log level in conf file is invalid: %d\n", level);
                }
            }
            else if (strcmp(trim(line_ptr), "mt66xx_fwlog_path") == 0)//log location
            {
                char *path_str = trim(split + 1);
                if (path_str)
                {
                    struct stat st_buf;
                    stat(path_str, &st_buf);
                    if (S_ISDIR(st_buf.st_mode))
                    {
                        snprintf(fw_log_path, LOG_PATH_LENGTH, "%s", path_str);
                        printf("[bt_fw_logger]conf file set fwlog path: %s\n", fw_log_path);
                    }
                    else
                    {
                        printf("[bt_fw_logger]invalid file path %s or permission denied, errno:%d\n", path_str, errno);
                    }
                }
            }
            else if (strcmp(trim(line_ptr), "mt66xx_fwlog_size") == 0)//single log file size
            {
                char *size_str = trim(split + 1);
                if (size_str)
                {
                    char *ptr = NULL;
                    uint32_t size = (uint32_t)strtoul(size_str, &ptr, 10);
                    if ((size >= 10240) && (size <= (1024*1024*20)))
                    {
                        log_file_size = size;
                        printf("[bt_fw_logger]conf file set fwlog size: %d\n", log_file_size);
                    }
                }
            }
            else if (strcmp(trim(line_ptr), "mt66xx_fwlog_count") == 0)//log file number
            {
                char *count_str = trim(split + 1);
                if (count_str)
                {
                    char *ptr = NULL;
                    uint32_t num = (uint32_t)strtoul(count_str, &ptr, 10);
                    if ((num > 0) && (num <= 100))
                    {
                        log_file_num = num;
                        printf("[bt_fw_logger]conf file set fwlog count: %d\n", log_file_num);
                    }
                }
            }
        }
    }

    printf("[bt_fw_logger]%s end\n", __func__);
}

static void mv_last_log_files(void)
{
    char cmd[256] = {0};
    char old_picus_log_folder[LOG_PATH_LENGTH] = {0};

    snprintf(old_picus_log_folder, LOG_PATH_LENGTH, "%s/%s", fw_log_path, "picus_last");

    if (0 != mkdir(old_picus_log_folder, 0777)) {
        if (errno != EEXIST) {
            printf("[bt_fw_logger]create old picus log folder fail errno %d\n", errno);
            return;
        }
    }
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "rm -rf %s/*.picus", old_picus_log_folder);
    if (0 != system(cmd)) {
        printf("[bt_fw_logger]delete old old files fail errno %d\n", errno);
        return;
    }
    printf("[bt_fw_logger]delete picus files in %s\n", old_picus_log_folder);

    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "mv %s/*.picus %s/", fw_log_path, old_picus_log_folder);
    if (0 != system(cmd)) {
        printf("[bt_fw_logger]move old files fail errno %d\n", errno);
        return;
    }
    system("sync");
    printf("[bt_fw_logger]move last picus files to %s\n", old_picus_log_folder);
}

static void remove_old_log_files(char *log_path, int all, int index)
{
    /* check already exist file under log_path */
    char temp_picus_filename[36] = {0};
    char picus_fullname[256] = {0};

    DIR *p_dir = opendir(log_path);
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
            memset(temp_picus_filename, 0, sizeof(temp_picus_filename));
            memset(picus_fullname, 0, sizeof(picus_fullname));
            if (strstr(p_file->d_name, DUMP_PICUS_NAME_EXT) != NULL)
            {
                if (all)    //remove all old log files
                {
                    snprintf(picus_fullname, sizeof(picus_fullname), "%s/%s", log_path, p_file->d_name);
                    if (remove(picus_fullname)) {
                        printf("[bt_fw_logger]The old log:%s can't remove, errno: %d\n", p_file->d_name, errno);
                    }
                    else {
                        printf("[bt_fw_logger]The old log: %s is removed\n", p_file->d_name);
                    }
                }
                else    //remove a specific log file
                {
                    snprintf(temp_picus_filename, sizeof(temp_picus_filename), "_%d.picus", index);
                    if (strstr(p_file->d_name, temp_picus_filename) != NULL) {
                        snprintf(picus_fullname, sizeof(picus_fullname), "%s/%s", log_path, p_file->d_name);
                        if (remove(picus_fullname)) {
                        printf("[bt_fw_logger]The old log: %s can't remove, errno: %d\n", p_file->d_name, errno);
                        } else {
                            printf("[bt_fw_logger]The old log: %s is removed\n", p_file->d_name);
                        }
                    }
                }
            }
        }
        closedir(p_dir);
    }
    else
    {
        printf("[bt_fw_logger]readdir fail, errno: %d\n", errno);
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
    snprintf(dump_file_name, sizeof(dump_file_name), "%s/" DUMP_PICUS_NAME_PREFIX "%s_%d" DUMP_PICUS_NAME_EXT, fw_log_path, timestamp_buffer, index);

    /* dump file for picus log */
    if ((fp = fopen(dump_file_name, "wb")) == NULL) {
        printf("[bt_fw_logger]create log file %s fail [%s] errno %d\n", dump_file_name, strerror(errno), errno);
        return NULL;
    } else {
        printf("[bt_fw_logger]log file %s is created, dumping...\n", dump_file_name);
    }

    fillheader(header, sizeof(header), index);
    fwrite(header, 1, sizeof(header), fp);
    fwrite(padding, 1, sizeof(padding), fp);
    file_size_remain_to_switch = log_file_size;

    return fp;
}

static void *bt_fw_logger_main(void *arg)
{
    int nRead = 0;
    int nWritten = 0;
    fd_set rset;    /** For select */
    struct timeval tv;
    FILE *current_fp = NULL;
    int current_index = 0;
    int i_ret = 0;

    printf("[bt_fw_logger]%s, thread_id: 0x%x\n", __func__, pthread_self());

    //create log ctrl fifo
    i_ret = access(BT_FW_LOG_CTRL_FIFO, F_OK);
    if (i_ret != 0)
    {
        i_ret = mkfifo(BT_FW_LOG_CTRL_FIFO, 0777);
        if (i_ret < 0)
            printf("mkfifo %s fail, errno: %d\n", BT_FW_LOG_CTRL_FIFO, errno);
        else
            printf("mkfifo %s success\n", BT_FW_LOG_CTRL_FIFO);
    }

    parse_fwlog_config();

    do
    {
        int max_fd = fw_log_fd;

        if ((fw_log_ctrl_fd = open(BT_FW_LOG_CTRL_FIFO, O_RDWR|O_NONBLOCK)) < 0)
        {
            printf("open fifo %s fail, errno: %d\n", BT_FW_LOG_CTRL_FIFO, errno);
        }
        FD_ZERO(&rset);
        FD_SET(fw_log_fd, &rset);
        if (fw_log_ctrl_fd > 0)
        {
            FD_SET(fw_log_ctrl_fd, &rset);
            max_fd = (fw_log_ctrl_fd > fw_log_fd) ? fw_log_ctrl_fd : fw_log_fd;
        }
        tv.tv_sec = 10; /* timeout is 10s for select method */
        tv.tv_usec = 0;
        if (select(max_fd + 1, &rset, NULL, NULL, NULL) == 0) {
            printf("[bt_fw_logger]Read data timeout(10s) from fw_log_bt\n");
            if (fw_log_ctrl_fd > 0) close(fw_log_ctrl_fd);
            continue;
        }

        if (fw_log_ctrl_fd > 0)
        {
            if (FD_ISSET(fw_log_ctrl_fd, &rset)) //parse log ctrl event
            {
                int res = 0;
                char fifo_buf[FIFO_BUF_SIZE] = {0};

                memset(fifo_buf, 0, FIFO_BUF_SIZE);
                res = read(fw_log_ctrl_fd, fifo_buf, FIFO_BUF_SIZE);
                printf("read %d byte from fw_log_ctrl_fd, fifo_buf: %s\n", res, fifo_buf);
                if (NULL != strstr(fifo_buf, "log_path"))   //change log location
                {
                    struct stat st_buf;
                    fifo_buf[res-1] = '\0'; //replace the \n to \0
                    stat(fifo_buf+9, &st_buf);
                    if (S_ISDIR(st_buf.st_mode))
                    {
                        snprintf(fw_log_path, LOG_PATH_LENGTH, "%s", fifo_buf+9);
                        printf("[bt_fw_logger]change log path to %s\n", fw_log_path);
                        //close current log file in the current location, and create log file with index 0 in new location
                        if (current_fp > 0)
                        {
                            fflush(current_fp);
                            fclose(current_fp);
                            current_fp = 0;
                        }
                    }
                    else
                    {
                        printf("[bt_fw_logger]invalid file path %s or permission denied, errno:%d\n", fifo_buf+9, errno);
                    }
                }
                else if (NULL != strstr(fifo_buf, "log_size"))  //change single log file size
                {
                    char *ptr = NULL;
                    uint32_t size = (uint32_t)strtoul(fifo_buf+9, &ptr, 10);
                    if ((size >= 10240) && (size <= (1024*1024*20)))
                    {
                        if (size > log_file_size)
                            file_size_remain_to_switch += (size - log_file_size);
                        else
                            file_size_remain_to_switch = 0;
                        log_file_size = size;
                        printf("[bt_fw_logger]change log_file_size to %d\n", log_file_size);
                    }
                }
                else if (NULL != strstr(fifo_buf, "log_count")) //change log file count
                {
                    char *ptr = NULL;
                    uint32_t num = (uint32_t)strtoul(fifo_buf+10, &ptr, 10);
                    if ((num > 0) && (num <= 100))
                    {
                        log_file_num = num;
                        printf("[bt_fw_logger]change log_file_num to %d\n", log_file_num);
                    }
                }
            }
        }

        if (fw_log_ctrl_fd > 0) close(fw_log_ctrl_fd);

        if (!FD_ISSET(fw_log_fd, &rset))
            continue;

        if (current_fp == 0)
        {
            mv_last_log_files(); // move old log to a folder
            current_fp = create_new_log_file(0);
            if (NULL == current_fp)
            {
                printf("[bt_fw_logger]fatal error: create new log file fail\n");
                return NULL;
            }
        }
        /* Read all packet from driver fwlog queue */
        nRead = read(fw_log_fd, buffer, sizeof(buffer));
        if (nRead > 0)
        {
            nWritten = fwrite(buffer, 1, nRead, current_fp);
            if (nWritten != nRead)
            {
                printf("[bt_fw_logger]write may fail, nRead(%d) != nWritten(%d)\n", nRead, nWritten);
            }
            file_size_remain_to_switch -= nWritten;
        }
        else if (nRead < 0)
        {
            printf("[bt_fw_logger]read fail, errno=%d\n", errno);
            continue;
        }
        /* switch file name if file size is over file_size */
        if (file_size_remain_to_switch <= 0) {
            file_size_remain_to_switch = log_file_size;
            fclose(current_fp);
            if (log_file_num - 1 > current_index) {
                current_index++;
            } else {
                current_index = 0;
            }
            if (current_index == 1) {   // if the old picus log is not pulled, it will be lost
                char old_picus_log_folder[LOG_PATH_LENGTH] = {0};
                snprintf(old_picus_log_folder, LOG_PATH_LENGTH, "%s/%s", fw_log_path, "picus_last");
                remove_old_log_files(old_picus_log_folder, TRUE, 0);
            }
            /* remove the file before creating */
            remove_old_log_files(fw_log_path, FALSE, current_index);
            current_fp = create_new_log_file(current_index);
            if (NULL == current_fp)
            {
                printf("[bt_fw_logger]create new log file fail\n");
                //TODO
                return NULL;
            }
        }
        fflush(current_fp);
        usleep(50000);
    } while (logger_on);


    printf("[bt_fw_logger]%s exit, thread_id: 0x%x\n", __func__, pthread_self());

    return NULL;
}


void bt_fw_logger_start(void)
{
    uint16_t index = 0;
    printf("[bt_fw_logger]%s\n", __func__);
    if (fw_log_fd > 0)
    {
        printf("%s, device is already opened\n", __func__);
        return;
    }
    //1. open device
    do
    {
        fw_log_fd = open(BT_FW_LOG_NODE, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fw_log_fd <= 0) {
            printf("[bt_fw_logger]Can't open device node %s, errno: %d\n", BT_FW_LOG_NODE, errno);
            sleep(1);
        } else {
            printf("[bt_fw_logger]Open device node successfully fw_log_fd = %d, index=%d\n", fw_log_fd, index);
            break;
        }
        if(index == 200)
        {
            printf("[bt_fw_logger]Can't open device node with 200 times retry\n");
            return;
        }
    }while (index++ < 200);

    //set default file path
    snprintf(fw_log_path, LOG_PATH_LENGTH, "%s", BT_FW_LOG_DEFAULT_PATH);

    //2. create logger thread
    if (bt_fw_logger_thread == 0)
    {
        pthread_attr_t thread_attr;

        pthread_attr_init(&thread_attr);
        pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&bt_fw_logger_thread, &thread_attr, bt_fw_logger_main, NULL))
        {
            printf("[bt_fw_logger]create logger thread fail\n",__func__);
            bt_fw_logger_thread = 0;
            return;
        }
        pthread_attr_destroy(&thread_attr);
    }
}

void bt_fw_logger_stop(void)
{
    //disable fw log first
    system("echo raw-hex, 01 5d fc 04 02 00 01 00 > /dev/fw_log_bt");

    if (fw_log_fd > 0)
    {
        close(fw_log_fd);
    }

    //thread exit
    logger_on = 0;
    bt_fw_logger_thread = 0;
    printf("[bt_fw_logger]%s\n", __func__);

}
//---------------------------------------------------------------------------

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
 * MediaTek Inc. (C) 2014. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <error.h>
#ifdef MTK_BT_C4A
#include <cutils/properties.h>
#endif
#include "libnvram.h"

/* use nvram */
#include "CFG_BT_File.h"
#include "CFG_BT_Default.h"
#include "CFG_file_lid.h"

#include "bt_kal.h"
//#include "cust_bt.h"
#include "bt_drv.h"

#define BT_DATA_LOGGER 0    //save BT tx/rx raw data or not

#ifdef MTK_IVT_ONLY
#define LOG_ERR(f, ...)       printf("%s: " f, __FUNCTION__, ##__VA_ARGS__)
#define LOG_WAN(f, ...)       printf("%s: " f, __FUNCTION__, ##__VA_ARGS__)

#define LOG_DBG(f, ...)       printf("%s: " f,  __FUNCTION__, ##__VA_ARGS__)
#define LOG_TRC(f)            printf("%s #%d", __FUNCTION__, __LINE__)
#endif

#ifndef MTK_BT_NVRAM
static ap_nvram_btradio_struct stBtDefault_6631 =
{
    {0x00, 0x00, 0x46, 0x66, 0x31, 0x01},
    {0x60, 0x00}, //not used
#if defined(__MTK_MERGE_INTERFACE_SUPPORT__)
    {0x63, 0x10, 0x00, 0x00},
#elif defined(__MTK_BT_I2S_SUPPORT__)
    {0x03, 0x10, 0x00, 0x02},
#else
    {0x23, 0x10, 0x00, 0x00},
#endif
    {0x07, 0x80, 0x00, 0x06, 0x05, 0x07},       // Align 6631 others product
    {0x03, 0x40, 0x1F, 0x40, 0x1F, 0x00, 0x04},
    {0x80, 0x00}, //not used
    {0xFF, 0xFF, 0xFF},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00}, // not used
    {0x00, 0x00, 0x00, 0x00}, // not used
    ///////////// Reserved /////////////
    {0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};
#endif

/* Audio interface & Codec information Mapping */
struct audio_t audio_conf_map[] = {
#if defined(__MTK_MERGE_INTERFACE_SUPPORT__)
    { 0x6628,    { MERGE_INTERFACE,  SYNC_8K,  SHORT_FRAME,  0 } },
#else
    { 0x6628,    { PCM,              SYNC_8K,  SHORT_FRAME,  0 } },
#endif
#if defined(__MTK_MERGE_INTERFACE_SUPPORT__)
    { 0x6630,    { MERGE_INTERFACE,  SYNC_8K,  SHORT_FRAME,  0 } },
#elif defined(__MTK_BT_I2S_SUPPORT__)
    { 0x6630,    { I2S,              SYNC_8K,  SHORT_FRAME,  0 } },
#else
    { 0x6630,    { PCM,              SYNC_8K,  SHORT_FRAME,  0 } },
#endif
#if defined(__MTK_MERGE_INTERFACE_SUPPORT__)
    { 0x6632,    { MERGE_INTERFACE,  SYNC_8K,  SHORT_FRAME,  0 } },
#elif defined(__MTK_BT_I2S_SUPPORT__)
    { 0x6632,    { I2S,              SYNC_8K,  SHORT_FRAME,  0 } },
#else
    { 0x6632,    { PCM,              SYNC_8K,  SHORT_FRAME,  0 } },
#endif
    { 0x8163,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x8127,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x8167,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6582,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6592,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6752,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x0321,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x0335,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x0337,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6580,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6755,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6797,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0x6757,    { CVSD_REMOVAL,     SYNC_8K,  SHORT_FRAME,  0 } },
    { 0,         { 0 } }
};


/**************************************************************************
 *              F U N C T I O N   D E C L A R A T I O N S                 *
***************************************************************************/

extern BOOL BT_InitDevice(
    INT32   comPort,
    UINT32  chipId,
    PUCHAR  pucNvRamData,
    UINT32  u4Baud,
    UINT32  u4HostBaud,
    UINT32  u4FlowControl,
    SETUP_UART_PARAM_T setup_uart_param
);

extern BOOL BT_DeinitDevice(INT32 comPort);

/**************************************************************************
 *                          F U N C T I O N S                             *
***************************************************************************/

static BOOL is_memzero(unsigned char *buf, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        if (*(buf+i) != 0) return FALSE;
    }
    return TRUE;
}

#if BT_DATA_LOGGER
static FILE *autobt_bt_tx_fp = NULL;
#define AUTOBT_BT_TX_FILE  "/tmp/autobt_bt_tx.log"
static FILE *autobt_bt_rx_fp = NULL;
#define AUTOBT_BT_RX_FILE  "/tmp/autobt_bt_rx.log"
static void create_bt_log_file(void)
{
    if (autobt_bt_tx_fp == NULL)
    {
        if ((autobt_bt_tx_fp = fopen(AUTOBT_BT_TX_FILE, "wb")) == NULL) {
            printf("[autobt]create AUTOBT_BT_TX_FILE fail [%s] errno %d\n", strerror(errno), errno);
            return;
        } else {
            printf("[autobt]log file %s is created, fp=0x%x\n", AUTOBT_BT_TX_FILE, autobt_bt_tx_fp);
        }
    }

    if (autobt_bt_rx_fp == NULL)
    {
        if ((autobt_bt_rx_fp = fopen(AUTOBT_BT_RX_FILE, "wb")) == NULL) {
            printf("[autobt]create AUTOBT_BT_RX_FILE fail [%s] errno %d\n", strerror(errno), errno);
            return;
        } else {
            printf("[autobt]log file %s is created, fp=0x%x\n", AUTOBT_BT_RX_FILE, autobt_bt_rx_fp);
        }
    }
}

static void bt_log_write(FILE *fp, unsigned char *buf, unsigned int len)
{
    if ((fp != NULL) && (buf != NULL))
    {
        int nWritten = fwrite(buf, 1, len, fp);
        if (nWritten != len)
        {
            printf("[autobt]write may fail, u4PktLen(%d) != nWritten(%d), fp=0x%x\n", len, nWritten, fp);
        }
        fflush(fp);
    }
}
static void bt_log_exit(void)
{
    if (autobt_bt_rx_fp != NULL)
    {
        fclose(autobt_bt_rx_fp);
        autobt_bt_rx_fp = NULL;
    }
    if (autobt_bt_tx_fp != NULL)
    {
        fclose(autobt_bt_tx_fp);
        autobt_bt_tx_fp = NULL;
    }
}
#endif

/* Initialize UART driver */
static int init_uart(char *dev)
{
    int fd;

    LOG_TRC();

    fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        LOG_ERR("Can't open %s (%s), errno[%d]\n", dev, strerror(errno), errno);
        return -1;
    }

    return fd;
}

EXPORT_SYMBOL int bt_get_combo_id(unsigned int *pChipId)
{
    int  chipId_ready_retry = 0;
#ifdef MTK_BT_C4A
    char chipId_val[PROPERTY_VALUE_MAX];

    do {
        if (property_get("persist.mtk.wcn.combo.chipid", chipId_val, NULL) &&
            0 != strcmp(chipId_val, "-1")){
            *pChipId = (unsigned int)strtoul(chipId_val, NULL, 16);
            break;
        }
        else {
            chipId_ready_retry ++;
            usleep(500000);
        }
    } while(chipId_ready_retry < 10);
#else
#if defined(MTK_MT8167)
    *pChipId = 0x8167;
#elif defined(MTK_MT6630)
    *pChipId = 0x6630;
#elif defined(MTK_MT6631)
    *pChipId = 0x6631;
#endif
#endif

    LOG_DBG("Get combo chip id retry %d\n", chipId_ready_retry);
    if (chipId_ready_retry >= 10) {
        LOG_DBG("Invalid combo chip id!\n");
        return -1;
    }
    else {
        LOG_DBG("Combo chip id %x\n", *pChipId);
        return 0;
    }
}

static int bt_read_nvram(unsigned char *pucNvRamData)
{
    F_ID bt_nvram_fd = {0};
    int rec_size = 0;
    int rec_num = 0;
    ap_nvram_btradio_struct bt_nvram;

    int nvram_ready_retry = 0;
//    char nvram_init_val[PROPERTY_VALUE_MAX];

    LOG_TRC();

#ifdef MTK_BT_C4A
    /* Sync with Nvram daemon ready */
    do {
        if (property_get("service.nvram_init", nvram_init_val, NULL) &&
            0 == strcmp(nvram_init_val, "Ready"))
            break;
        else {
            nvram_ready_retry ++;
            usleep(500000);
        }
    } while(nvram_ready_retry < 10);

    LOG_DBG("Get NVRAM ready retry %d\n", nvram_ready_retry);
    if (nvram_ready_retry >= 10) {
        LOG_ERR("Get NVRAM restore ready fails!\n");
        return -1;
    }
#endif
#ifdef MTK_BT_NVRAM
    bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISREAD);
    if (bt_nvram_fd.iFileDesc < 0) {
        LOG_WAN("Open BT NVRAM fails errno %d\n", errno);
        return -1;
    }

    if (rec_num != 1) {
        LOG_ERR("Unexpected record num %d", rec_num);
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    if (rec_size != sizeof(ap_nvram_btradio_struct)) {
        LOG_ERR("Unexpected record size %d ap_nvram_btradio_struct %d",
                rec_size, (int)sizeof(ap_nvram_btradio_struct));
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    if (read(bt_nvram_fd.iFileDesc, &bt_nvram, rec_num*rec_size) < 0) {
        LOG_ERR("Read NVRAM fails errno %d\n", errno);
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    NVM_CloseFileDesc(bt_nvram_fd);
    memcpy(pucNvRamData, &bt_nvram, sizeof(ap_nvram_btradio_struct));
#else
    return -1;
#endif
    return 0;
}

static int bt_write_nvram(unsigned char *pucNvRamData)
{
    F_ID bt_nvram_fd = {0};
    int rec_size = 0;
    int rec_num = 0;
    ap_nvram_btradio_struct bt_nvram;

    int nvram_ready_retry = 0;
//    char nvram_init_val[PROPERTY_VALUE_MAX];

    LOG_TRC();

#ifdef MTK_BT_C4A
    /* Sync with Nvram daemon ready */
    do {
        if (property_get("service.nvram_init", nvram_init_val, NULL) &&
            0 == strcmp(nvram_init_val, "Ready"))
            break;
        else {
            nvram_ready_retry ++;
            usleep(500000);
        }
    } while(nvram_ready_retry < 10);

    LOG_DBG("Get NVRAM ready retry %d\n", nvram_ready_retry);
    if (nvram_ready_retry >= 10) {
        LOG_ERR("Get NVRAM restore ready fails!\n");
        return -1;
    }
#endif
#ifdef MTK_BT_NVRAM
    bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISWRITE);
    if (bt_nvram_fd.iFileDesc < 0) {
        LOG_WAN("Open BT NVRAM fails errno %d\n", errno);
        return -1;
    }

    if (rec_num != 1) {
        LOG_ERR("Unexpected record num %d", rec_num);
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    if (rec_size != sizeof(ap_nvram_btradio_struct)) {
        LOG_ERR("Unexpected record size %d ap_nvram_btradio_struct %d",
                rec_size, (int)sizeof(ap_nvram_btradio_struct));
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    if (write(bt_nvram_fd.iFileDesc, pucNvRamData, rec_num*rec_size) < 0) {
        LOG_ERR("Read NVRAM fails errno %d\n", errno);
        NVM_CloseFileDesc(bt_nvram_fd);
        return -1;
    }

    NVM_CloseFileDesc(bt_nvram_fd);
    if (false == FileOp_BackupToBinRegion_All())
    {
        LOG_ERR("backup to nvram binregion failed!\n");
    }
#else
#ifdef MTK_MT6631
    /* Use MT6631 default value */
    memcpy(&stBtDefault_6631, pucNvRamData, sizeof(ap_nvram_btradio_struct));
#endif
#ifdef MTK_MT6630
    memcpy(&stBtDefault_6630, pucNvRamData, sizeof(ap_nvram_btradio_struct));
#endif
#endif
    return 0;
}

#define CUST_BT_SERIAL_PORT "/dev/stpbt"

#define PURE_BTSNOOP 0
#if PURE_BTSNOOP
#include <dlfcn.h>
#include <pthread.h>
#include <signal.h>

#define BT_CONFIG_PATH "/data/misc/bluedroid/bt_stack.conf"
#define BTSNOOP_CFG_KEY "BtSnoopLogPure"
#define BTSNOOP_PATH_KEY "BtSnoopPureName"
#define BTSNOOP_FILE_NAME "/data/misc/bluetooth/logs/btsnoop_pure.log"

typedef void (*INIT)(bool value, const char *log_path);
typedef void (*CAPTURE)(const char *buffer, bool is_received);

static void *glib_handle = NULL;
static INIT    pure_btsnoop_enable = NULL;
static CAPTURE pure_btsnoop_capture = NULL;

static pthread_t btsnoop_Thread;

#define BTSNOOP_BUF_LEN 1024
static unsigned char btsnoop_buf[BTSNOOP_BUF_LEN];
static int write_p, read_p;
static int pure_fd = -1;
static bool pure_btsnoop = false;
static bool pure_relay = false;
int read_com_port(int fd, unsigned char *buf, unsigned int len);

static int btsnoop_push_data_to_buffer(unsigned char *data, int len)
{
	int remainLen = 0;

	if (write_p >= read_p)
		remainLen = write_p - read_p;
	else
		remainLen = BTSNOOP_BUF_LEN -
			(read_p - write_p);

	if ((remainLen + len) >= BTSNOOP_BUF_LEN) {
		LOG_ERR("%s copy copyLen %d > BTSNOOP_BUF_LEN(%d), push failed\n",
			__func__,
			(remainLen + len),
			BTSNOOP_BUF_LEN);
		return -1;
	}

	if (write_p >= BTSNOOP_BUF_LEN)
		write_p = 0;

	if (write_p + len <= BTSNOOP_BUF_LEN)
		memcpy(&btsnoop_buf[write_p], data, len);
	else {
		memcpy(&btsnoop_buf[write_p],
			data,
			BTSNOOP_BUF_LEN - write_p);
		memcpy(btsnoop_buf,
			&data[BTSNOOP_BUF_LEN - write_p],
			len - (BTSNOOP_BUF_LEN - write_p));
	}

	write_p += len;
	if (write_p >= BTSNOOP_BUF_LEN)
		write_p -= BTSNOOP_BUF_LEN;

	return 0;
}

static int btsnoop_pull_data_from_buffer(
						unsigned char *data,
						int len)
{
	int copyLen = 0;
	unsigned long ret = 0;

	if (write_p >= read_p)
		copyLen = write_p - read_p;
	else
		copyLen = BTSNOOP_BUF_LEN -
			(read_p - write_p);

	if (copyLen > len)
		copyLen = len;

	if (read_p + copyLen <= BTSNOOP_BUF_LEN)
		memcpy(data, &btsnoop_buf[read_p], copyLen);
	else {
		memcpy(data, &btsnoop_buf[read_p],
				BTSNOOP_BUF_LEN - read_p);
		memcpy(&data[BTSNOOP_BUF_LEN - read_p],
				btsnoop_buf,
				copyLen - (BTSNOOP_BUF_LEN-read_p));
	}

	read_p += copyLen;
	if (read_p >= BTSNOOP_BUF_LEN)
		read_p -= BTSNOOP_BUF_LEN;

	return copyLen;
}

int btsnoop_receive_data(int fd, unsigned char *buf, unsigned int len)
{
    int bytesRead = 0;
    unsigned int bytesToRead = len;

    int ret = 0;
    struct timeval tv;
    fd_set readfd;

    tv.tv_sec = 5; /* SECOND */
    tv.tv_usec = 0; /* USECOND */
    FD_ZERO(&readfd);

    /* Try to receive len bytes */
    while (bytesToRead > 0) {

        FD_SET(fd, &readfd);
        ret = select(fd + 1, &readfd, NULL, NULL, &tv);

        if (ret > 0) {
            bytesRead = read_com_port(fd, buf, bytesToRead);
            if (bytesRead < 0) {
                return -1;
            }
            else {
                bytesToRead -= bytesRead;
                buf += bytesRead;
            }
        }
        else if (ret == 0) {
            LOG_DBG("Read com port timeout 5000ms!\n");
            return -1;
        }
        else if ((ret == -1) && (errno == EINTR)) {
            LOG_ERR("select error EINTR\n");
        }
        else {
            LOG_ERR("select error %s(%d)!\n", strerror(errno), errno);
            return -1;
        }
    }
    return 0;
}

int btsnoop_read_thread(void *ptr)
{
    UINT8 ucHeader = 0;
    UINT32 u4Len = 0, pkt_len = 0;
    UINT32 i;
    char buf[BTSNOOP_BUF_LEN];

    if (pure_fd < 0) {
        LOG_ERR("bt driver fd is invalid!\n");
        return FALSE;
    }

    while (1) {
        if (btsnoop_receive_data(pure_fd, &ucHeader, sizeof(ucHeader)) < 0) {
            continue;
        }
        memset(buf, 0, BTSNOOP_BUF_LEN);
        u4Len = 0;

        buf[0] = ucHeader;
        u4Len ++;

        switch (ucHeader) {
          case 0x04:
            LOG_DBG("Receive HCI event\n");
            if (btsnoop_receive_data(pure_fd, &buf[1], 2) < 0) {
                LOG_ERR("Read event header fails\n");
                continue;
            }

            u4Len += 2;
            pkt_len = (UINT32)buf[2];
            if ((u4Len + pkt_len) > BTSNOOP_BUF_LEN) {
                LOG_ERR("Read buffer overflow! packet len %d\n", u4Len + pkt_len);
                continue;
            }

            if (btsnoop_receive_data(pure_fd, &buf[3], pkt_len) < 0) {
                LOG_ERR("Read event param fails\n");
                continue;
            }

            u4Len += pkt_len;
            break;

          case 0x02:
            LOG_DBG("Receive ACL data\n");
            if (btsnoop_receive_data(pure_fd, &btsnoop_buf[1], 4) < 0) {
                LOG_ERR("Read ACL header fails\n");
                continue;
            }

            u4Len += 4;
            pkt_len = (((UINT32)btsnoop_buf[4]) << 8);
            pkt_len += (UINT32)btsnoop_buf[3]; /*little endian*/
            if ((u4Len + pkt_len) > BTSNOOP_BUF_LEN) {
                LOG_ERR("Read buffer overflow! packet len %d\n", u4Len + pkt_len);
                continue;
            }

            if (btsnoop_receive_data(pure_fd, &btsnoop_buf[5], pkt_len) < 0) {
                LOG_ERR("Read ACL data fails\n");
                continue;
            }

            u4Len += pkt_len;
            break;

          case 0x03:
            LOG_DBG("Receive SCO data\n");
            if (btsnoop_receive_data(pure_fd, &btsnoop_buf[1], 3) < 0) {
                LOG_ERR("Read SCO header fails\n");
                continue;
            }

            u4Len += 3;
            pkt_len = (UINT32)btsnoop_buf[3];
            if ((u4Len + pkt_len) > BTSNOOP_BUF_LEN) {
                LOG_ERR("Read buffer overflow! packet len %d\n", u4Len + pkt_len);
                continue;
            }

            if (btsnoop_receive_data(pure_fd, &btsnoop_buf[4], pkt_len) < 0) {
                LOG_ERR("Read SCO data fails\n");
                continue;
            }

            u4Len += pkt_len;
            break;
          default:
            LOG_ERR("Unexpected BT packet header %02x\n", ucHeader);
            continue;
        }

        /* Dump rx packet */
        LOG_DBG("read:\n");
        for (i = 0; i < u4Len; i++) {
            LOG_DBG("%02x\n", buf[i]);
        }

        if (pure_btsnoop == true)
            pure_btsnoop_capture(buf, false);

        /* If debug event, drop and retry */
        if ((buf[0] == 0x04) && (buf[1] == 0xE0)) {
            memset(buf, 0, u4Len);
            u4Len = 0;
            continue;
        }

        if (!(pure_relay == false && (buf[0] == 0x04 && buf[1] == 0xFF && (buf[3] == 0x50 || buf[3] == 0x51)))) {
            btsnoop_push_data_to_buffer(buf, u4Len);
        }
    }
    return;
}

int btsnoop_get_cfg(char *path)
{
    int ret = -1, path_get = 0;
    FILE *fp = NULL;
    char *start = NULL, *end = NULL;
    char line[1024];

    fp = fopen(BT_CONFIG_PATH, "rt");
    if (!fp) {
        printf("open cfg file failed\n");
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (!ret && path_get)
            break;
        if (line[0] == '#')
            continue;
        if (strstr(line, BTSNOOP_CFG_KEY) && strstr(line, "true"))
            ret = 0;
        if (strstr(line, BTSNOOP_PATH_KEY)) {
            start = strstr(line, "=");
            end = strstr(start, ".log");
            if (start && end) {
                memcpy(path, start + 1, (end + strlen(".log") - (start + 1)));
                path[end + strlen(".log") - (start + 1)] = '\0';
                path_get = 1;
            }
        }
    }

    fclose(fp);

    if (!ret && !path_get)
        strcpy(path, BTSNOOP_FILE_NAME);

    return ret;
}

void btsnoop_init(int bt_fd)
{
    char path[128];
    const char *errstr;

    if (btsnoop_get_cfg(path)) {
        LOG_DBG("These is no btsnoop cfg\n");
    } else {
        glib_handle = dlopen("libbluetooth_mtk_pure_btsnoop.so", RTLD_LAZY);
        if (!glib_handle) {
            LOG_ERR("dlopen snoop %s\n", dlerror());
        } else {
            dlerror(); /* Clear any existing error */

            pure_btsnoop_enable = dlsym(glib_handle, "pure_btsnoop_enable");
            pure_btsnoop_capture = dlsym(glib_handle, "pure_btsnoop_capture");

            if ((errstr = dlerror()) != NULL)
                LOG_ERR("Can't find function symbols %s\n", errstr);
            else {
                pure_btsnoop_enable(true, path);
                pure_btsnoop = true;
            }
        }
    }
    pure_fd = bt_fd;

    write_p = 0;
    read_p = 0;

    pthread_create(&btsnoop_Thread, NULL, btsnoop_read_thread, (void*)NULL);

    return;
}

void btsnoop_deinit()
{
    pthread_cancel(btsnoop_Thread);
    pthread_join(btsnoop_Thread, NULL);

    pure_fd = -1;
    write_p = 0;
    read_p = 0;

    if (glib_handle) {
        pure_btsnoop_enable(false, NULL);
        pure_btsnoop = false;

        dlclose(glib_handle);
        glib_handle = NULL;
    }
    return;
}
#endif

/* MTK specific chip initialize process */
EXPORT_SYMBOL int bt_init(void)
{
    int fd = -1;
    unsigned int chipId = 0;
    unsigned char ucNvRamData[sizeof(ap_nvram_btradio_struct)] = {0};
    unsigned int speed = 0;
    unsigned int flow_control = 0;
    SETUP_UART_PARAM_T uart_setup_callback = NULL;

    LOG_TRC();

    fd = init_uart(CUST_BT_SERIAL_PORT);
    if (fd < 0){
        LOG_ERR("Can't initialize" CUST_BT_SERIAL_PORT "\n");
        return -1;
    }

#if PURE_BTSNOOP
    btsnoop_init(fd);
#endif

#if BT_DATA_LOGGER
    create_bt_log_file();
#endif

    /* Get combo chip id */
    if (bt_get_combo_id(&chipId) < 0) {
        LOG_ERR("Get combo chip id fails\n");
        goto error;
    }

    /* Read NVRAM data */
    if ((bt_read_nvram(ucNvRamData) < 0) ||
          is_memzero(ucNvRamData, sizeof(ap_nvram_btradio_struct))) {
        LOG_ERR("Read NVRAM data fails or NVRAM data all zero!!\n");
        LOG_WAN("Use %x default value\n", chipId);
        switch (chipId) {
#ifdef MTK_MT8167
            case 0x8167:
                /* Use MT8167 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif
#ifdef MTK_MT6631
            case 0x6631:
                /* Use MT6631 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif
#ifdef MTK_MT6630
            case 0x6630:
                /* Use MT6630 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif

            default:
                LOG_ERR("Unknown combo chip id: %04x\n", chipId);
                goto error;
        }
    }

    LOG_DBG("[BDAddr %02x-%02x-%02x-%02x-%02x-%02x][Voice %02x %02x][Codec %02x %02x %02x %02x] \
            [Radio %02x %02x %02x %02x %02x %02x][Sleep %02x %02x %02x %02x %02x %02x %02x][BtFTR %02x %02x] \
            [TxPWOffset %02x %02x %02x][CoexAdjust %02x %02x %02x %02x %02x %02x] \
            [Radio_ext %02x %02x][TxPWOffset_ext %02x %02x %02x]\n",
            ucNvRamData[0], ucNvRamData[1], ucNvRamData[2], ucNvRamData[3], ucNvRamData[4], ucNvRamData[5],
            ucNvRamData[6], ucNvRamData[7],
            ucNvRamData[8], ucNvRamData[9], ucNvRamData[10], ucNvRamData[11],
            ucNvRamData[12], ucNvRamData[13], ucNvRamData[14], ucNvRamData[15], ucNvRamData[16], ucNvRamData[17],
            ucNvRamData[18], ucNvRamData[19], ucNvRamData[20], ucNvRamData[21], ucNvRamData[22], ucNvRamData[23], ucNvRamData[24],
            ucNvRamData[25], ucNvRamData[26],
            ucNvRamData[27], ucNvRamData[28], ucNvRamData[29],
            ucNvRamData[30], ucNvRamData[31], ucNvRamData[32], ucNvRamData[33], ucNvRamData[34], ucNvRamData[35],
            ucNvRamData[36], ucNvRamData[37],
            ucNvRamData[38], ucNvRamData[39], ucNvRamData[40]);

    if (BT_InitDevice(
          fd,
          chipId,
          ucNvRamData,
          speed,
          speed,
          flow_control,
          uart_setup_callback) == FALSE) {

        LOG_ERR("Initialize BT device fails\n");
        goto error;
    }

    LOG_DBG("bt_init success\n");
    return fd;

error:
    if (fd >= 0)
        close(fd);
    return -1;
}

/* MTK specific deinitialize process */
EXPORT_SYMBOL int bt_restore(int fd)
{
    LOG_TRC();
#if PURE_BTSNOOP
    btsnoop_deinit(fd);
#endif
    BT_DeinitDevice(fd);
    close(fd);
    return 0;
}

int write_com_port(int fd, unsigned char *buf, unsigned int len)
{
    int nWritten = 0;
    unsigned int bytesToWrite = len;

    if (fd < 0) {
        LOG_ERR("No available com port\n");
        return -EIO;
    }

    while (bytesToWrite > 0) {
        #if BT_DATA_LOGGER
        bt_log_write(autobt_bt_tx_fp, buf, bytesToWrite);
        #endif
        nWritten = write(fd, buf, bytesToWrite);
        if (nWritten < 0) {
            if (errno == EINTR || errno == EAGAIN)
                break;
            else
                return -errno; /* errno used for whole chip reset */
        }
        bytesToWrite -= nWritten;
        buf += nWritten;
    }

    return (len - bytesToWrite);
}

int read_com_port(int fd, unsigned char *buf, unsigned int len)
{
    int nRead = 0;
    unsigned int bytesToRead = len;

    if (fd < 0) {
        LOG_ERR("No available com port\n");
        return -EIO;
    }

    nRead = read(fd, buf, bytesToRead);
    #if BT_DATA_LOGGER
    bt_log_write(autobt_bt_rx_fp, buf, nRead);
    #endif
    if (nRead < 0) {
        if(errno == EINTR || errno == EAGAIN)
            return 0;
        else
            return -errno; /* errno used for whole chip reset */
    }

    return nRead;
}

EXPORT_SYMBOL int bt_send_data(int fd, unsigned char *buf, unsigned int len)
{
    int bytesWritten = 0;
    unsigned int bytesToWrite = len;

#if PURE_BTSNOOP
    if (pure_btsnoop == true)
        pure_btsnoop_capture(buf, true);
    if (buf[0] == 0x01 && buf[1] == 0xbe && buf[2] == 0xfc)
        pure_relay = true;
#endif

    /* Try to send len bytes data in buffer */
    while (bytesToWrite > 0) {
        bytesWritten = write_com_port(fd, buf, bytesToWrite);
        if (bytesWritten < 0) {
            return -1;
        }
        bytesToWrite -= bytesWritten;
        buf += bytesWritten;
    }

    return 0;
}

EXPORT_SYMBOL int bt_receive_data(int fd, unsigned char *buf, unsigned int len)
{
#if PURE_BTSNOOP
    int delay = 500;
    for (; delay > 0; delay--) {
        if ((write_p > read_p) && (write_p >= len + read_p))
            break;
        if ((write_p < read_p) && (write_p + BTSNOOP_BUF_LEN >= len + read_p))
            break;

        usleep(10 * 1000);
    }

    if (delay == 0)
        return -1;

    btsnoop_pull_data_from_buffer(buf, len);
    return 0;
#else
    int bytesRead = 0;
    unsigned int bytesToRead = len;

    int ret = 0;
    struct timeval tv;
    fd_set readfd;

    tv.tv_sec = 5; /* SECOND */
    tv.tv_usec = 0; /* USECOND */
    FD_ZERO(&readfd);

    /* Try to receive len bytes */
    while (bytesToRead > 0) {

        FD_SET(fd, &readfd);
        ret = select(fd + 1, &readfd, NULL, NULL, &tv);

        if (ret > 0) {
            bytesRead = read_com_port(fd, buf, bytesToRead);
            if (bytesRead < 0) {
                return -1;
            }
            else {
                bytesToRead -= bytesRead;
                buf += bytesRead;
            }
        }
        else if (ret == 0) {
            LOG_DBG("Read com port timeout 5000ms!\n");
            return -1;
        }
        else if ((ret == -1) && (errno == EINTR)) {
            LOG_ERR("select error EINTR\n");
        }
        else {
            LOG_ERR("select error %s(%d)!\n", strerror(errno), errno);
            return -1;
        }
    }
    return 0;
#endif
}

EXPORT_SYMBOL int bt_set_relay(bool enable)
{
#if PURE_BTSNOOP
    pure_relay = enable;
    return 0;
#else
    return -1;
#endif
}

static int bt_get_audio_configuration(BT_INFO *pBTInfo)
{
    unsigned int chipId = 0;
    int i;

    LOG_DBG("BT_MTK_OP_AUDIO_GET_CONFIG\n");

    /* Get combo chip id */
    if (bt_get_combo_id(&chipId) < 0) {
        LOG_ERR("Get combo chip id fails\n");
        return -2;
    }

    /* Return the specific audio config on current chip */
    for (i = 0; audio_conf_map[i].chip_id; i++) {
        if (audio_conf_map[i].chip_id == chipId) {
            LOG_DBG("Find chip %x\n", chipId);
            memcpy(&(pBTInfo->audio_conf), &(audio_conf_map[i].audio_conf), sizeof(AUDIO_CONFIG));
            return 0;
        }
    }

    LOG_ERR("Current chip is not included in audio_conf_map\n");
    return -3;
}

int mtk_bt_op(bt_mtk_opcode_t opcode, void *param)
{
    int ret = -1;
    switch (opcode) {
        case BT_MTK_OP_AUDIO_GET_CONFIG: {
            BT_INFO *pBTInfo = (BT_INFO*)param;
            if (pBTInfo != NULL) {
                ret = bt_get_audio_configuration(pBTInfo);
            }
            else {
                LOG_ERR("BT_MTK_OP_AUDIO_GET_CONFIG have NULL as parameter\n");
            }
            break;
        }
    default:
        LOG_ERR("Unknown operation %d\n", opcode);
        break;
    }
    return ret;
}

EXPORT_SYMBOL int bt_op_tx_power_offset(int wr_flag, char *group_offset, char *group_flag)
{
    unsigned char ucNvRamData[sizeof(ap_nvram_btradio_struct)] = {0};
    unsigned int chipId = 0;
    ap_nvram_btradio_struct *p_nvm = ucNvRamData;
    int i = 0;

    if (bt_get_combo_id(&chipId) < 0) {
        LOG_ERR("Get combo chip id fails\n");
        return -1;
    }

    /* Read NVRAM data */
    if ((bt_read_nvram(ucNvRamData) < 0) ||
          is_memzero(ucNvRamData, sizeof(ap_nvram_btradio_struct))) {
        LOG_ERR("Read NVRAM data fails or NVRAM data all zero!!\n");
        LOG_WAN("Use %x default value\n", chipId);
        switch (chipId) {
#ifdef MTK_MT6630
            case 0x6630:
                /* Use MT6630 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif
#ifdef MTK_MT8167
            case 0x8167:
                /* Use MT8167 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif
#ifdef MTK_MT6631
            case 0x6631:
                /* Use MT6631 default value */
                memcpy(ucNvRamData, &stBtDefault, sizeof(ap_nvram_btradio_struct));
                break;
#endif
            default:
                LOG_ERR("Unknown combo chip id: %04x\n", chipId);
                return -1;
        }
    }

    LOG_WAN("[BDAddr %02x-%02x-%02x-%02x-%02x-%02x][Voice %02x %02x][Codec %02x %02x %02x %02x] \
            [Radio %02x %02x %02x %02x %02x %02x][Sleep %02x %02x %02x %02x %02x %02x %02x][BtFTR %02x %02x] \
            [TxPWOffset %02x %02x %02x][CoexAdjust %02x %02x %02x %02x %02x %02x] \
            [Radio_ext %02x %02x][TxPWOffset_ext %02x %02x %02x]\n",
            ucNvRamData[0], ucNvRamData[1], ucNvRamData[2], ucNvRamData[3], ucNvRamData[4], ucNvRamData[5],
            ucNvRamData[6], ucNvRamData[7],
            ucNvRamData[8], ucNvRamData[9], ucNvRamData[10], ucNvRamData[11],
            ucNvRamData[12], ucNvRamData[13], ucNvRamData[14], ucNvRamData[15], ucNvRamData[16], ucNvRamData[17],
            ucNvRamData[18], ucNvRamData[19], ucNvRamData[20], ucNvRamData[21], ucNvRamData[22], ucNvRamData[23], ucNvRamData[24],
            ucNvRamData[25], ucNvRamData[26],
            ucNvRamData[27], ucNvRamData[28], ucNvRamData[29],
            ucNvRamData[30], ucNvRamData[31], ucNvRamData[32], ucNvRamData[33], ucNvRamData[34], ucNvRamData[35],
            ucNvRamData[36], ucNvRamData[37],
            ucNvRamData[38], ucNvRamData[39], ucNvRamData[40]);

    if (1 == wr_flag) {
        for (i = 0; i < 6; i++) {
            if (i%2 == 0) {
                group_offset[i] = p_nvm->TxPWOffset[i/2] & 0xF;
            }
            else {
                group_offset[i] = (p_nvm->TxPWOffset[i/2] & 0xF0) >> 4;
            }
        }
    }

    if (2 == wr_flag) {
        for (i = 0; i < 6; i++) {
            if (1 == group_flag[i]) {
                if (i%2 == 0) {
                    p_nvm->TxPWOffset[i/2] = (p_nvm->TxPWOffset[i/2] & 0xF0) | (group_offset[i] & 0xF);
                }
                else {
                    p_nvm->TxPWOffset[i/2] = (p_nvm->TxPWOffset[i/2] & 0xF) | ((group_offset[i] & 0xF) << 4);
                }
            }
        }
        bt_write_nvram(&ucNvRamData);
    }

    return 0;
}


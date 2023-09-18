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
#include <sys/stat.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/if.h>

#include "libnvram.h"
#include "Custom_NvRam_LID.h"

#define ETH_DEV "eth0"
#define ETH_HW_ADDR_LEN 6


#define DATA_LENGTH sizeof(char)
static char *default_zero_mac = "00:00:00:00:00:00";
char *default_eth_mac = "7A:24:B3:D1:8B:67";

#define LOG(...) printf(__VA_ARGS__)

static int set_macAddr_to_eth(unsigned char *macaddr)
{
	struct ifreq ifr;
	int skfd;

	memset(&ifr, 0, sizeof(ifr));
	/* Open a basic socket. */
	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		LOG("[eth_set_mac_addr]: create socket failed: %s",strerror(errno));
		return -1;
	}
	else
	{
		LOG("[eth_set_mac_addr]: socket is %d",  skfd);
	}
	strncpy(ifr.ifr_name, ETH_DEV, strlen(ETH_DEV));
	memcpy(ifr.ifr_hwaddr.sa_data, macaddr, ETH_HW_ADDR_LEN);
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(skfd, SIOCSIFHWADDR, &ifr) < 0)
	{
		LOG("[eth_set_mac_addr]:set %s's MAC addr error: %s",  ETH_DEV, strerror(errno));
		close(skfd);
		return -1;
	}
	close(skfd);
	return 0;
}

static void string_del_colon(char* pStr, char* outString)
{
	int j,k;
	char del_x = ':';

	char str[20] = {0}, str_tmp[20] = {0};
	strncpy(str, pStr, strlen(pStr));
	for (j=k=0; str[j]!='\0'; j++)
	{
		if (str[j] != del_x)
		{
			str_tmp[k] = str[j];
			k++;
		}
	}
	str_tmp[k] = '\0';
	strncpy(outString, str_tmp, strlen(str_tmp));
}

static int insert_char(char *src, char chr, int nPos)
{
	if (src == NULL || nPos >  strlen(src))
	{
		return -1;
	}
	char *p = src + strlen(src) + 1;
	while((p - src) >= nPos)
	{
		*p = *(p - 1);
		p--;
	}
	*p = chr;
	return 0;
}

void convertMac(char *string, char *output)
{
	unsigned char index_str1[20] = {0};
	unsigned char temp_Str[20] = {0};
	unsigned char temp[20] = {0};
	unsigned char temp_Str_1[20] = {0};
	unsigned char remove_colon[20] = {0};
	char ch = ':';
	char ch1 = 'A';
	int i = 0, j = 0;

	string_del_colon(string, remove_colon);
	strcpy(temp_Str, string);
	string_del_colon(temp_Str,temp_Str_1);
	srand(time(NULL));
	int index = rand()%100000000;
	printf("rand()=%d\n", index);
	sprintf(index_str1,"%d", index);
	j = strlen(index_str1);
	for (i = j; i >= 0; i--)
	{
		if (i != 1)
		{
			temp_Str_1[i] = index_str1[i];
		}
	}
	strncpy(temp, remove_colon + 7, strlen(remove_colon) - 8);
	strcat(temp_Str_1, temp);
	for (i = 1; i < 6; i++)
	{
		insert_char(temp_Str_1, ch, 3*i);
	}
	printf("new mac address = %s\n", temp_Str_1);
	if (strlen(temp_Str_1) == 16)
	{
		srand(time(NULL));
		index = rand()%100;
		printf("rand()=%d\n", index);
		j = strlen(temp_Str_1);
		temp_Str_1[j] = ch1;
		temp_Str_1[j+1] = '\0';
	}
	if (strlen(temp_Str_1) == 18)
	{
		srand(time(NULL));
		index = rand()%100;
		printf("rand()=%d\n", index);
		j = strlen(temp_Str_1);
		temp_Str_1[j-1] = '\0';
	}
	strncpy(output, temp_Str_1, strlen(temp_Str_1));
}


/* Just tell NVRAM to create Product info default record */
void* NVRAM_ETHERNET(void* arg)
{
    F_ID pin_fid;
    int rec_size = 0;
    int rec_num = 0;
	int wr_offset = 0;
	int size = 0;
	unsigned char mac[6] = {0};
	int iFD = 0;
	F_ID eth_nvram_fd;
	unsigned char t_mac_addr[6] = {0};
	unsigned char dest_mac[24] = {0};
	char eth_mac[24] = {0};

    NVRAM_LOG("[NVRAM Daemon]eth info lid %d ++\n",AP_CFG_RDED_ETHERNET_CUSTOM_LID);
    pin_fid = NVM_GetFileDesc(AP_CFG_RDED_ETHERNET_CUSTOM_LID, &rec_size, &rec_num, true);
	iFD = pin_fid.iFileDesc;
    NVRAM_LOG("[NVRAM Daemon] eth info FID %d rec_size %d rec_num %d\n", pin_fid.iFileDesc, rec_size, rec_num);

	for (wr_offset = 0; wr_offset < 6; wr_offset++)
    {
        if ((size = read(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
        {
            LOG("[NVRAM Daemon] read eth offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            break;
        }

        if (wr_offset != 5)
        {
            sprintf(eth_mac + 3 * wr_offset, "%02X:", mac[wr_offset]);
        }
        else
        {
            sprintf(eth_mac + 3 * wr_offset, "%02X", mac[wr_offset]);
        }
    }
	LOG("[NVRAM Daemon] read eth mac is %s\n", eth_mac);
	memset(dest_mac,0,strlen(dest_mac));
	if (strcmp(default_zero_mac, eth_mac) == 0)
	{
		LOG("[NVRAM Daemon] read eth mac from nvram is NULL. set a radom mac address:%s \n",default_eth_mac);
		convertMac(default_eth_mac,dest_mac);
	}
	else
	{
		strncpy(dest_mac,eth_mac, strlen(eth_mac));
		LOG("[NVRAM Daemon] read eth mac from nvram is not NULL. dest_mac:%s \n",dest_mac);
	}
	LOG("[NVRAM Daemon] before write to eth driver , the mac is %s\n", dest_mac);
	sscanf(dest_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
	&(t_mac_addr[0]),
	&(t_mac_addr[1]),
	&(t_mac_addr[2]),
	&(t_mac_addr[3]),
	&(t_mac_addr[4]),
	&(t_mac_addr[5]));
	if (set_macAddr_to_eth(t_mac_addr)== 0)
	{
		LOG("[NVRAM Daemon] set eth mac to eth driver successfully\n");
	}
	if(!NVM_CloseFileDesc(pin_fid))
	{
		 NVRAM_LOG("[NVRAM Daemon] close eth info error");
	}
    return NULL;
}

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
#include <sys/types.h>
#include "libnvram.h"
#include "wfbt_wr.h"
#include "Custom_NvRam_LID.h"
#include "libfile_op.h"
#include "CFG_BT_File.h"
#include "CFG_file_lid.h"

#define BT_NVRAM_FILE_PER               (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)
#define LOG(...) printf(__VA_ARGS__)

extern int iFileWIFILID;
extern int iFileETHLID;

extern int iFileCustomWIFILID;

#define WIFI_LENGTH 80
#define ETH_LENGTH 80
#define DATA_LENGTH sizeof(char)
int wifi_test = 0;
int eth_test = 0;
#define ETH_DEV "eth0"
#define ETH_HW_ADDR_LEN 6

unsigned char eth_mac_addr[6] = {0};

EPPROM_2_NVRAM_WIFI Epp2NvWfTable[WIFI_LENGTH] =
{
    {80, 0x04, "WLAN MAC Address0"},
    {81, 0x05, "WLAN MAC Address1"},
    {82, 0x06, "WLAN MAC Address2"},
    {83, 0x07, "WLAN MAC Address3"},
    {84, 0x08, "WLAN MAC Address4"},
    {85, 0x09, "WLAN MAC Address5"},
    {86, 0x0C, "cTxPwr2G4Cck"},
    {87, 0x0D, "cTxPwr2G4Dsss"},
    {88, 0x0E, "acReserved"},
    {89, 0x10, "cTxPwr2G4OFDM_6M_9M"},
    {90, 0x11, "cTxPwr2G4OFDM_12M_18M"},
    {91, 0x12, "cTxPwr2G4OFDM_24M_36M"},
    {92, 0x13, "cTxPwr2G4OFDM_Reserved"},
    {93, 0x14, "cTxPwr2G4OFDM_48Mbps"},
    {94, 0x15, "cTxPwr2G4OFDM_54Mbps"},
    {95, 0x16, "cTxPwr2G4HT20_MCS0"},
    {96, 0x17, "cTxPwr2G4HT20_MCS1_MCS2"},
    {97, 0x18, "cTxPwr2G4HT20_MCS3_MCS4"},
    {98, 0x19, "cTxPwr2G4HT20_MCS5"},
    {99, 0x1a, "cTxPwr2G4HT20_MCS6"},
    {100, 0x1b, "cTxPwr2G4HT20_MCS7"},
    {101, 0x1c, "cTxPwr2G4HT40_MCS0"},
    {102, 0x1d, "cTxPwr2G4HT40_MCS1_MCS2"},
    {103, 0x1e, "cTxPwr2G4HT40_MCS3_MCS4"},
    {104, 0x1f, "cTxPwr2G4HT40_MCS5"},
    {105, 0x20, "cTxPwr2G4HT40_MCS6"},
    {106, 0x21, "cTxPwr2G4HT40_MCS7"},
    {107, 0x22, "cTxPwr5GOFDM_6M_9M"},
    {108, 0x23, "cTxPwr5GOFDM_12M_18M"},
    {109, 0x24, "cTxPwr5GOFDM_24M_36M"},
    {110, 0x25, "cTxPwr5GOFDM_Reserved"},
    {111, 0x26, "cTxPwr5GOFDM_48Mbps"},
    {112, 0x27, "cTxPwr5GOFDM_54Mbps"},
    {113, 0x28, "cTxPwr5GHT20_MCS0"},
    {114, 0x29, "cTxPwr5GHT20_MCS1_MCS2"},
    {115, 0x2a, "cTxPwr5GHT20_MCS3_MCS4"},
    {116, 0x2b, "cTxPwr5GHT20_MCS5"},
    {117, 0x2c, "cTxPwr5GHT20_MCS6"},
    {118, 0x2d, "cTxPwr5GHT20_MCS7"},
    {119, 0x2e, "cTxPwr5GHT40_MCS0"},
    {120, 0x2f, "cTxPwr5GHT40_MCS1_MCS2"},
    {121, 0x30, "cTxPwr5GHT40_MCS3_MCS4"},
    {122, 0x31, "cTxPwr5GHT40_MCS5"},
    {123, 0x32, "cTxPwr5GHT40_MCS6"},
    {124, 0x33, "cTxPwr5GHT40_MCS7"},
    {125, 0x50, "aucChOffset0"},
    {126, 0x51, "aucChOffset1"},
    {127, 0x52, "aucChOffset2"},
    {128, 0x60, "cTxPwr5GOffset0"},
    {129, 0x61, "cTxPwr5GOffset1"},
    {130, 0x62, "cTxPwr5GOffset2"},
    {131, 0x63, "cTxPwr5GOffset3"},
    {132, 0x64, "cTxPwr5GOffset4"},
    {133, 0x65, "cTxPwr5GOffset5"},
    {134, 0x66, "cTxPwr5GOffset6"},
    {135, 0x67, "cTxPwr5GOffset7"},
    {136, 0x110, "cTxPwr5GOffset8"},
    {137, 0x111, "cTxPwr5GOffset9"},
    {138, 0x112, "cTxPwr5GOffset10"},
    {139, 0x113, "cTxPwr5GOffset11"},
    {140, 0x114, "cTxPwr5GOffset12"},
    {141, 0x115, "cTxPwr5GOffset13"},
    {142, 0x116, "cTxPwr5GOffset14"},
    {143, 0x117, "cTxPwr5GOffset15"},
    {144, 0x118, "cTxPwr5GOffset16"},
    {145, 0x119, "cTxPwr5GOffset17"},
    {146, 0x11a, "cTxPwr5GOffset18"},
    {147, 0x11b, "cTxPwr5GOffset19"},
    {148, 0x11c, "cTxPwr5GOffset20"},
    {149, 0x11d, "Frequence offset (XTAL calibration)"},
    {150, 0x11e, "WF2G4_HT40_Disable"},
    {151, 0x11f, "Country_List_Index"},
    {152, 0x120, "Max_Power_Table_Index"},
    {153, 0x121, "WIFI Reserved"},
    {154, 0x122, "WIFI Reserved"},
    {155, 0x123, "WIFI Reserved"},
    {156, 0x124, "WIFI Reserved"},
    {157, 0x125, "WIFI Reserved"},
    {158, 0x126, "WIFI Reserved"},
    {159, 0x127, "WIFI Reserved"}
};

#define WIFI6630_LENGTH 80
EPPROM_2_NVRAM_WIFI Epp2NvWfTable_6630[WIFI6630_LENGTH] =
{
    {80, 0x04, "WLAN MAC Address0"},
    {81, 0x05, "WLAN MAC Address1"},
    {82, 0x06, "WLAN MAC Address2"},
    {83, 0x07, "WLAN MAC Address3"},
    {84, 0x08, "WLAN MAC Address4"},
    {85, 0x09, "WLAN MAC Address5"},
    {86, 0x0C, "cTxPwr2G4Cck"},
    {87, 0x0D, "cTxPwr2G4Dsss"},
    {88, 0x0E, "acReserved"},
    {89, 0x10, "cTxPwr2G4OFDM_6M_9M"},
    {90, 0x11, "cTxPwr2G4OFDM_12M_18M"},
    {91, 0x12, "cTxPwr2G4OFDM_24M_36M"},
    {92, 0x13, "cTxPwr2G4OFDM_Reserved"},
    {93, 0x14, "cTxPwr2G4OFDM_48Mbps"},
    {94, 0x15, "cTxPwr2G4OFDM_54Mbps"},
    {95, 0x16, "cTxPwr2G4HT20_MCS0"},
    {96, 0x17, "cTxPwr2G4HT20_MCS1_MCS2"},
    {97, 0x18, "cTxPwr2G4HT20_MCS3_MCS4"},
    {98, 0x19, "cTxPwr2G4HT20_MCS5"},
    {99, 0x1a, "cTxPwr2G4HT20_MCS6"},
    {100, 0x1b, "cTxPwr2G4HT20_MCS7"},
    {101, 0x1c, "cTxPwr2G4HT40_MCS0"},
    {102, 0x1d, "cTxPwr2G4HT40_MCS1_MCS2"},
    {103, 0x1e, "cTxPwr2G4HT40_MCS3_MCS4"},
    {104, 0x1f, "cTxPwr2G4HT40_MCS5"},
    {105, 0x20, "cTxPwr2G4HT40_MCS6"},
    {106, 0x21, "cTxPwr2G4HT40_MCS7"},
    {107, 0x22, "cTxPwr5GOFDM_6M_9M"},
    {108, 0x23, "cTxPwr5GOFDM_12M_18M"},
    {109, 0x24, "cTxPwr5GOFDM_24M_36M"},
    {110, 0x25, "cTxPwr5GOFDM_Reserved"},
    {111, 0x26, "cTxPwr5GOFDM_48Mbps"},
    {112, 0x27, "cTxPwr5GOFDM_54Mbps"},
    {113, 0x28, "cTxPwr5GHT20_MCS0"},
    {114, 0x29, "cTxPwr5GHT20_MCS1_MCS2"},
    {115, 0x2a, "cTxPwr5GHT20_MCS3_MCS4"},
    {116, 0x2b, "cTxPwr5GHT20_MCS5"},
    {117, 0x2c, "cTxPwr5GHT20_MCS6"},
    {118, 0x2d, "cTxPwr5GHT20_MCS7"},
    {119, 0x2e, "cTxPwr5GHT40_MCS0"},
    {120, 0x2f, "cTxPwr5GHT40_MCS1_MCS2"},
    {121, 0x30, "cTxPwr5GHT40_MCS3_MCS4"},
    {122, 0x31, "cTxPwr5GHT40_MCS5"},
    {123, 0x32, "cTxPwr5GHT40_MCS6"},
    {124, 0x33, "cTxPwr5GHT40_MCS7"},
    {125, 0x50, "aucChOffset0"},
    {126, 0x51, "aucChOffset1"},
    {127, 0x52, "aucChOffset2"},
    {128, 0x60, "cTxPwr5GOffset0"},
    {129, 0x61, "cTxPwr5GOffset1"},
    {130, 0x62, "cTxPwr5GOffset2"},
    {131, 0x63, "cTxPwr5GOffset3"},
    {132, 0x64, "cTxPwr5GOffset4"},
    {133, 0x65, "cTxPwr5GOffset5"},
    {134, 0x66, "cTxPwr5GOffset6"},
    {135, 0x67, "cTxPwr5GOffset7"},
    {136, 0x110, "cTxPwr5GOffset8"},
    {137, 0x111, "cTxPwr5GOffset9"},
    {138, 0x112, "cTxPwr5GOffset10"},
    {139, 0x113, "cTxPwr5GOffset11"},
    {140, 0x114, "cTxPwr5GOffset12"},
    {141, 0x115, "cTxPwr5GOffset13"},
    {142, 0x116, "cTxPwr5GOffset14"},
    {143, 0x117, "cTxPwr5GOffset15"},
    {144, 0x118, "cTxPwr5GOffset16"},
    {145, 0x119, "cTxPwr5GOffset17"},
    {146, 0x11a, "cTxPwr5GOffset18"},
    {147, 0x11b, "cTxPwr5GOffset19"},
    {148, 0x11c, "cTxPwr5GOffset20"},
    {149, 0x11d, "Frequence offset (XTAL calibration)"},
    {150, 0x11e, "WF2G4_HT40_Disable"},
    {151, 0x11f, "Country_List_Index"},
    {152, 0x120, "Max_Power_Table_Index"},
    {153, 0x121, "WIFI Reserved"},
    {154, 0x122, "WIFI Reserved"},
    {155, 0x123, "WIFI Reserved"},
    {156, 0x124, "WIFI Reserved"},
    {157, 0x125, "WIFI Reserved"},
    {158, 0x126, "WIFI Reserved"},
    {159, 0x127, "WIFI Reserved"}
    };

#define BT_LENGTH 36
EPPROM_2_NVRAM_BT Epp2NvBTTable[BT_LENGTH] =
{
    {16, 0, "BT MAC Address[0]"},
    {17, 1, "BT MAC Address[1]"},
    {18, 2, "BT MAC Address[2]"},
    {19, 3, "BT MAC Address[3]"},
    {20, 4, "BT MAC Address[4]"},
    {21, 5, "BT MAC Address[5]"},
    {22, 6, "BT Voice Setting[0]"},
    {23, 7, "BT Voice Setting[1]"},
    {24, 8, "BT PCM Setting[0]"},
    {25, 9, "BT PCM Setting[1]"},
    {26, 10, "BT PCM Setting[2]"},
    {27, 11, "BT PCM Setting[3]"},
    {28, 12, "BT RF Setting[0]"},
    {29, 13, "BT RF Setting[1]"},
    {30, 14, "BT RF Setting[2]"},
    {31, 15, "BT RF Setting[3]"},
    {32, 16, "BT RF Setting[4]"},
    {33, 17, "BT RF Setting[5]"},
    {34, 18, "BT Sleep Mode[0]"},
    {35, 19, "BT Sleep Mode[1]"},
    {36, 20, "BT Sleep Mode[2]"},
    {37, 21, "BT Sleep Mode[3]"},
    {38, 22, "BT Sleep Mode[4]"},
    {39, 23, "BT Sleep Mode[5]"},
    {40, 24, "BT Sleep Mode[6]"},
    {41, 25, "BT Other Feature[0]"},
    {42, 26, "BT Other Feature[1]"},
    {43, 27, "BT Tx Power Offset[0]"},
    {44, 28, "BT Tx Power Offset[1]"},
    {45, 29, "BT Tx Power Offset[2]"},
    {46, 30, "BT Coexistence[0]"},
    {47, 31, "BT Coexistence[1]"},
    {48, 32, "BT Coexistence[2]"},
    {49, 33, "BT Coexistence[3]"},
    {50, 34, "BT Coexistence[4]"},
    {51, 35, "BT Coexistence[5]"}
};

#define BT6630_LENGTH 36
EPPROM_2_NVRAM_BT Epp2NvBTTable_6630[BT6630_LENGTH] =
{
    {16, 0, "BT MAC Address[0]"},
    {17, 1, "BT MAC Address[1]"},
    {18, 2, "BT MAC Address[2]"},
    {19, 3, "BT MAC Address[3]"},
    {20, 4, "BT MAC Address[4]"},
    {21, 5, "BT MAC Address[5]"},
    {22, 6, "BT Voice Setting[0]"},
    {23, 7, "BT Voice Setting[1]"},
    {24, 8, "BT PCM Setting[0]"},
    {25, 9, "BT PCM Setting[1]"},
    {26, 10, "BT PCM Setting[2]"},
    {27, 11, "BT PCM Setting[3]"},
    {28, 12, "BT RF Setting[0]"},
    {29, 13, "BT RF Setting[1]"},
    {30, 14, "BT RF Setting[2]"},
    {31, 15, "BT RF Setting[3]"},
    {32, 16, "BT RF Setting[4]"},
    {33, 17, "BT RF Setting[5]"},
    {34, 18, "BT Sleep Mode[0]"},
    {35, 19, "BT Sleep Mode[1]"},
    {36, 20, "BT Sleep Mode[2]"},
    {37, 21, "BT Sleep Mode[3]"},
    {38, 22, "BT Sleep Mode[4]"},
    {39, 23, "BT Sleep Mode[5]"},
    {40, 24, "BT Sleep Mode[6]"},
    {41, 25, "BT Other Feature[0]"},
    {42, 26, "BT Other Feature[1]"},
    {43, 27, "BT Tx Power Offset[0]"},
    {44, 28, "BT Tx Power Offset[1]"},
    {45, 29, "BT Tx Power Offset[2]"},
    {46, 30, "BT Coexistence[0]"},
    {47, 31, "BT Coexistence[1]"},
    {48, 32, "BT Coexistence[2]"},
    {49, 33, "BT Coexistence[3]"},
    {50, 34, "BT Coexistence[4]"},
    {51, 35, "BT Coexistence[5]"}
};

int parse_cmd(const char *cmd1, const char *cmd2)
{
    if ((0 == strcmp("wifi", cmd1)) || (0 == strcmp("WIFI", cmd1)))
    {
        if ((0 == strcmp("r", cmd2)) || (0 == strcmp("R", cmd2)))
        {
            return WIFI_R;
        }
        else if ((0 == strcmp("w", cmd2)) || (0 == strcmp("W", cmd2)))
        {
            return WIFI_W;
        }
#ifdef WFBTETH_WR_MAC
        else if ((0 == strcmp("r_mac", cmd2)) || (0 == strcmp("R_MAC", cmd2)))
        {
            return WIFI_R_MAC;
        }
        else if ((0 == strcmp("w_mac", cmd2)) || (0 == strcmp("W_MAC", cmd2)))
        {
            return WIFI_W_MAC;
        }
#endif
        else
        {
            LOG("[wfbteth_wr] error: WIFI cmd line is not correct!\n");
            return -1;
        }
    }
	else if ((0 == strcmp("eth", cmd1)) || (0 == strcmp("ETH", cmd1)))
    {
        if ((0 == strcmp("r", cmd2)) || (0 == strcmp("R", cmd2)))
        {
            return ETH_R;
        }
        else if ((0 == strcmp("w", cmd2)) || (0 == strcmp("W", cmd2)))
        {
            return ETH_W;
        }
#ifdef WFBTETH_WR_MAC
        else if ((0 == strcmp("r_mac", cmd2)) || (0 == strcmp("R_MAC", cmd2)))
        {
            return ETH_R_MAC;
        }
        else if ((0 == strcmp("w_mac", cmd2)) || (0 == strcmp("W_MAC", cmd2)))
        {
            return ETH_W_MAC;
        }
		else if ((0 == strcmp("mac_to_eth", cmd2)) || (0 == strcmp("MAC_TO_ETH", cmd2)))
        {
            return MAC_W_TO_ETH;
        }
		else if ((0 == strcmp("mac_to_nvram", cmd2)) || (0 == strcmp("MAC_TO_NVRAM", cmd2)))
        {
            return MAC_W_TO_NVRAM;
        }
#endif
        else
        {
            LOG("[wfbteth_wr] error: WIFI cmd line is not correct!\n");
            return -1;
        }
    }
    else if ((0 == strcmp("bt", cmd1)) || (0 == strcmp("BT", cmd1)))
    {
        if ((0 == strcmp("r", cmd2)) || (0 == strcmp("R", cmd2)))
        {
            return BT_R;
        }
        else if ((0 == strcmp("w", cmd2)) || (0 == strcmp("W", cmd2)))
        {
            return BT_W;
        }

#ifdef WFBTETH_WR_MAC
        else if ((0 == strcmp("r_mac", cmd2)) || (0 == strcmp("R_MAC", cmd2)))
        {
            return BT_R_MAC;
        }
        else if ((0 == strcmp("w_mac", cmd2)) || (0 == strcmp("W_MAC", cmd2)))
        {
            return BT_W_MAC;
        }
#endif
        else
        {
            LOG("[wfbteth_wr] error: BT cmd line is not correct!\n");
            return -1;
        }
    }
    else if ((0 == strcmp("wifi6630", cmd1)) || (0 == strcmp("WIFI6630", cmd1)))
    {
        if ((0 == strcmp("r", cmd2)) || (0 == strcmp("R", cmd2)))
        {
            return WIFI6630_R;
        }
        else if ((0 == strcmp("w", cmd2)) || (0 == strcmp("W", cmd2)))
        {
            return WIFI6630_W;
        }
        else
        {
            LOG("[wfbteth_wr] error: WIFI6630 cmd line is not correct!\n");
            return -1;
        }
    }
    else if ((0 == strcmp("bt6630", cmd1)) || (0 == strcmp("BT6630", cmd1)))
    {
        if ((0 == strcmp("r", cmd2)) || (0 == strcmp("R", cmd2)))
        {
            return BT6630_R;
        }
        else if ((0 == strcmp("w", cmd2)) || (0 == strcmp("W", cmd2)))
        {
            return BT6630_W;
        }
        else
        {
            LOG("[wfbteth_wr] error: BT6630 cmd line is not correct!\n");
            return -1;
        }
    }
    else
    {
        LOG("[wfbteth_wr] error: cmd line is not correct!\n");
        return -1;
    }
}

void print_wifiinfo(int wr, int offset, unsigned char data)
{
    int i = 0, flag = 0;
    if (wifi_test == WIFI_TEST)
    {
        if (WIFI_R == wr)
        {
            LOG("[wfbteth_wr] read wifi6625 offset(%d) = 0x%02X\n", offset, data);
        }
        else if (WIFI_W == wr)
        {
            LOG("[wfbteth_wr] write wifi6625 offset(%d) = 0x%02X successed!\n", offset, data);
        }
    }
    for (i = 0; i < WIFI_LENGTH; i++)
    {
        if (offset == Epp2NvWfTable[i].NvramOffset)
        {
            flag = i;
            break;
        }
    }
    if (WIFI_LENGTH == i)
    {
        LOG("[wfbteth_wr]wifi offset is invalid!\n");
        return;
    }
    if (WIFI_R == wr)
    {
        LOG("[wfbteth_wr] read wifi offset(%d) %s = 0x%02X\n", offset, Epp2NvWfTable[flag].Description, data);
    }
    else if (WIFI_W == wr)
    {
        LOG("[wfbteth_wr] write wifi offset(%d) %s = 0x%02X successed!\n", offset, Epp2NvWfTable[flag].Description, data);
    }
    return;
}


void print_ethinfo(int wr, int offset, unsigned char data)
{
    int i = 0, flag = 0;
    if (eth_test == ETH_TEST)
    {
        if (ETH_R == wr)
        {
            LOG("[wfbteh_wr] read eth offset(%d) = 0x%02X\n", offset, data);
        }
        else if (ETH_W == wr)
        {
            LOG("[wfbteh_wr] write eth offset(%d) = 0x%02X successed!\n", offset, data);
        }
    }
    for (i = 0; i < ETH_LENGTH; i++)
    {
        if (offset == Epp2NvWfTable[i].NvramOffset)
        {
            flag = i;
            break;
        }
    }
    if (ETH_LENGTH == i)
    {
        LOG("[wfbteth_wr]eth offset is invalid!\n");
        return;
    }
    if (ETH_R == wr)
    {
        LOG("[wfbteth_wr] read ETH offset(%d) %s = 0x%02X\n", offset, Epp2NvWfTable[flag].Description, data);
    }
    else if (ETH_W == wr)
    {
        LOG("[wfbteth_wr] write ETH offset(%d) %s = 0x%02X successed!\n", offset, Epp2NvWfTable[flag].Description, data);
    }
    return;
}


void print_wifi6630info(int wr, int offset, unsigned char data)
{
    int i = 0;
    int flag = 0;

    if (wifi_test == WIFI_TEST)
    {
        if (WIFI6630_R == wr)
        {
            LOG("[wfbteth_wr] read wifi6630 offset(%d) = 0x%02X\n", offset, data);
        }
        else if (WIFI6630_W == wr)
        {
            LOG("[wfbteth_wr] write wifi6630 offset(%d) = 0x%02X successed!\n", offset, data);
        }
    }
    for (i = 0; i < WIFI6630_LENGTH; i++)
    {
        if (offset == Epp2NvWfTable_6630[i].NvramOffset)
        {
            flag = i;
            break;
        }
    }
    if (WIFI6630_LENGTH == i)
    {
        LOG("[wfbteth_wr]wifi6630 offset is invalid!\n");
        return;
    }
    if (WIFI6630_R == wr)
    {
        LOG("[wfbteth_wr] read wifi6630 offset(%d) %s = 0x%02X\n", offset, Epp2NvWfTable_6630[flag].Description, data);
    }
    else if (WIFI6630_W == wr)
    {
        LOG("[wfbteth_wr] write wifi6630 offset(%d) %s = 0x%02X successed!\n", offset, Epp2NvWfTable_6630[flag].Description, data);
    }
    return;
}

void print_btinfo(int wr, int offset, unsigned char data)
{
    int i = 0;
    int flag = 0;

    for (i = 0; i < BT_LENGTH; i++)
    {
        if (offset == Epp2NvBTTable[i].NvramOffset)
        {
            flag = i;
            break;
        }
    }
    if (BT_LENGTH == i)
    {
        LOG("[wfbteth_wr]bt offset is invalid!\n");
        return;
    }
    if (BT_R == wr)
    {
        LOG("[wfbteth_wr] read bt offset(%d) %s = 0x%02X\n", offset, Epp2NvBTTable[flag].Description, data);
    }
    else if (BT_W == wr)
    {
        LOG("[wfbteth_wr] write bt offset(%d) %s = 0x%02X successed!\n", offset, Epp2NvBTTable[flag].Description, data);
    }
    return;
}

void print_bt6630info(int wr, int offset, unsigned char data)
{
    int i = 0;
    int flag = 0;

    for (i=0; i < BT6630_LENGTH; i++)
    {
        if (offset == Epp2NvBTTable_6630[i].NvramOffset)
        {
            flag = i;
            break;
        }
    }
    if (BT6630_LENGTH == i)
    {
        LOG("[wfbteth_wr]bt6630 offset is invalid!\n");
        return;
    }
    if (BT6630_R == wr)
    {
        LOG("[wfbteth_wr] read bt6630 offset(%d) %s = 0x%02X\n", offset, Epp2NvBTTable_6630[flag].Description, data);
    }
    else if (BT6630_W == wr)
    {
        LOG("[wfbteth_wr] write bt6630 offset(%d) %s = 0x%02X successed!\n", offset, Epp2NvBTTable_6630[flag].Description, data);
    }
    return;
}

static void convert_string(char *string)
{
	int i = 0;
	int index = 0;
	int high_low = 0;
	int data = 0;
	char *p = string;
	for (i = 0; i < strlen(string); i++, p++)
	{
		if ((*p >= '0') && (*p <= '9'))
		{
			data = *p - '0';
		}
		else if ((*p >= 'a') && (*p <= 'f'))
		{
			data = *p - 'a' + 0xA;
		}
		else if ((*p >= 'A') && (*p <= 'F'))
		{
			data = *p - 'A' + 0xA;
		}
		else if (*p == ':')
		{
			high_low = 2;
		}
		else
		{
			LOG("Input MAC error! \n");
			return;
		}

		if (high_low == 0)
		{
			eth_mac_addr[index] = data * 16;
			high_low = 1;
		}
		else if (high_low == 1)
		{
			eth_mac_addr[index] += data;
		}
		else
		{
			high_low = 0;
			index++;
		}
	}
}
int main(int argc,char *argv[])
{
    int cmd = C_END;
    unsigned char argbuffer = 0, databuffer = 0;
    int wr_offset = 0;
    int iFD = 0;
    int size = 0;
    int rec_size = 0, rec_num = 0;
    F_ID wifi_nvram_fd;
	F_ID eth_nvram_fd;
    F_ID bt_nvram_fd = {0};
#ifdef WFBTETH_WR_MAC
    unsigned char mac[6] = {0};
    char s_mac[24] = {0};
	unsigned char dest_mac[24] = {0};
	unsigned char t_mac_addr[6] = {0};
	unsigned char eth_mac[6] = {0};
#endif

    if (3 > argc)
    {
        LOG("[wfbteth_wr] error: argument is less then 4!\n");
        return -1;
    }
    cmd = parse_cmd(argv[1], argv[2]);
    if (NULL != argv[3])
    {
        wr_offset = atoi(argv[3]);
		strcpy(eth_mac, argv[3]);
    }

    if ((-1 != cmd) && ((WIFI_W == cmd) || (ETH_W == cmd) || (BT_W == cmd)))
    {
        if (NULL != argv[4])
        {
            argbuffer = (unsigned char)strtoul(argv[4], NULL, 16);
            if (NULL != argv[5])
            {
                if (0 == strcmp("test", argv[5]))
                {
                    wifi_test = WIFI_TEST;
					eth_test  = ETH_TEST;
                }
            }
        }
        else
        {
            LOG("[wfbteth_wr] error: no setting argument!\n");
            return -1;
        }
    }
    else if ((WIFI_R == cmd) || (ETH_R == cmd) ||  (WIFI6630_R == cmd) || (BT_R == cmd))
    {
        if (NULL != argv[4])
        {
            if (0 == strcmp("test", argv[4]))
            {
                wifi_test = WIFI_TEST;
            }
        }
    }
#ifdef WFBTETH_WR_MAC
    else if ((WIFI_W_MAC == cmd) || (BT_W_MAC == cmd) || (ETH_W_MAC == cmd) )
    {
        int i = 0;
        int index = 0;
        int high_low = 0;
        int data = 0;
        char *p = argv[3];
        for (i = 0; i < strlen(argv[3]); i++, p++)
        {
            if ((*p >= '0') && (*p <= '9'))
            {
                data = *p - '0';
            }
            else if ((*p >= 'a') && (*p <= 'f'))
            {
                data = *p - 'a' + 0xA;
            }
            else if ((*p >= 'A') && (*p <= 'F'))
            {
                data = *p - 'A' + 0xA;
            }
            else if (*p == ':')
            {
                high_low = 2;
            }
            else
            {
                LOG("Input MAC error! \n");
                return -1;
            }

            if (high_low == 0)
            {
                mac[index] = data * 16;
                high_low = 1;
            }
            else if (high_low == 1)
            {
                mac[index] += data;
            }
            else
            {
                high_low = 0;
                index++;
            }
        }
    }
#endif

    switch (cmd)
    {
#ifdef WFBTETH_WR_MAC
    case ETH_R_MAC:
		LOG("[wfbteth_wr] read eth mac start.\n");
        eth_nvram_fd = NVM_GetFileDesc(iFileETHLID, &rec_size, &rec_num, ISREAD);
        iFD = eth_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open eth file fail! error:%s\n", strerror(errno));
            break;
        }

        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset, SEEK_SET);
            if ((size = read(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] read eth offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }

            if (wr_offset != 5)
            {
                sprintf(s_mac + 3 * wr_offset, "%02X:", mac[wr_offset]);
            }
            else
            {
                sprintf(s_mac + 3 * wr_offset, "%02X", mac[wr_offset]);
            }
        }
        LOG("[wfbteth_wr] read s mac is %s\n", s_mac);
        break;
	case WIFI_R_MAC:
        wifi_nvram_fd = NVM_GetFileDesc(iFileWIFILID, &rec_size, &rec_num, ISREAD);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi file fail! error:%s\n", strerror(errno));
            break;
        }

        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset + 4, SEEK_SET);
            if ((size = read(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] read wifi offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }
            if (wr_offset != 5)
            {
                sprintf(s_mac + 3 * wr_offset, "%02X:", mac[wr_offset]);
            }
            else
            {
                sprintf(s_mac + 3 * wr_offset, "%02X", mac[wr_offset]);
            }
        }
        LOG("[wfbteth_wr] read wifi mac is %s\n", s_mac);
        break;
    case WIFI_W_MAC:
        wifi_nvram_fd = NVM_GetFileDesc(iFileWIFILID, &rec_size, &rec_num, ISWRITE);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi file fail! error:%s\n", strerror(errno));
            break;
        }
        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset + 4, SEEK_SET);
            if ((size = write(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] write wifi offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }
        }
        break;
	case ETH_W_MAC:
		LOG("[wfbteth_wr] write eth mac start.\n");
        eth_nvram_fd = NVM_GetFileDesc(iFileETHLID, &rec_size, &rec_num, ISWRITE);
        iFD = eth_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open eth file fail! error:%s\n", strerror(errno));
            break;
        }
        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset, SEEK_SET);
            if ((size = write(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] write eth offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }
        }
		LOG("[wfbt_wr] write eth mac successfully.\n");
        break;
    case BT_R_MAC:
        bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISREAD);
        if (rec_num != 1)
        {
            LOG("[wfbteth_wr]Unexpected record num %d", rec_num);
            NVM_CloseFileDesc(bt_nvram_fd);
            break;
        }
        iFD = bt_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open bt file fail! error:%s\n", strerror(errno));
            break;
        }

        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset, SEEK_SET);
            if ((size = read(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] read bt offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }

            if (wr_offset != 5)
            {
                sprintf(s_mac + 3 * wr_offset, "%02X:", mac[wr_offset]);
            }
            else
            {
                sprintf(s_mac + 3 * wr_offset, "%02X", mac[wr_offset]);
            }
        }
        LOG("[wfbteth_wr] read bt mac is %s\n", s_mac);
        break;
    case BT_W_MAC:
        bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISWRITE);
        if (rec_num != 1)
        {
            LOG("[wfbteth_wr]Unexpected record num %d", rec_num);
            NVM_CloseFileDesc(bt_nvram_fd);
            break;
        }
        iFD = bt_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open bt file fail! error:%s\n", strerror(errno));
            break;
        }

        for (wr_offset = 0; wr_offset < 6; wr_offset++)
        {
            lseek(iFD, wr_offset, SEEK_SET);
            if ((size = write(iFD, &mac[wr_offset], DATA_LENGTH)) < 0)
            {
                LOG("[wfbteth_wr] write bt offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
                break;
            }
        }
        break;
#endif
    case WIFI_R:
        wifi_nvram_fd = NVM_GetFileDesc(iFileWIFILID, &rec_size, &rec_num, ISREAD);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = read(iFD, &databuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] read wifi offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_wifiinfo(WIFI_R, wr_offset, databuffer);
        break;
	case ETH_R:
        eth_nvram_fd = NVM_GetFileDesc(iFileETHLID, &rec_size, &rec_num, ISREAD);
        iFD = eth_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open eth file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = read(iFD, &databuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] read eth offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_ethinfo(ETH_R, wr_offset, databuffer);
        break;
    case WIFI_W:
        wifi_nvram_fd = NVM_GetFileDesc(iFileWIFILID, &rec_size, &rec_num, ISWRITE);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = write(iFD, &argbuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbt_wr] write wifi offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_wifiinfo(WIFI_W, wr_offset, argbuffer);
        sync();
        break;
	case ETH_W:
        eth_nvram_fd = NVM_GetFileDesc(iFileETHLID, &rec_size, &rec_num, ISWRITE);
        iFD = eth_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open eth file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = write(iFD, &argbuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] write eth offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_ethinfo(ETH_W, wr_offset, argbuffer);
        sync();
        break;
    case BT_R:
        bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISREAD);
        if (rec_num != 1)
        {
            LOG("[wfbteth_wr]Unexpected record num %d", rec_num);
            NVM_CloseFileDesc(bt_nvram_fd);
            return -1;
        }
        iFD = bt_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open bt file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = read(iFD, &databuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] read bt offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_btinfo(BT_R, wr_offset, databuffer);
        break;
    case BT_W:
        bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, ISWRITE);
        if (rec_num != 1)
        {
            LOG("[wfbteth_wr]Unexpected record num %d", rec_num);
            NVM_CloseFileDesc(bt_nvram_fd);
            return -1;
        }
        iFD = bt_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open bt file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = write(iFD, &argbuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] write bt offset(%d) data fail! error:%s\n",wr_offset,strerror(errno));
            return -1;
        }
        print_btinfo(BT_W, wr_offset, argbuffer);
        break;
    case WIFI6630_R:
        wifi_nvram_fd = NVM_GetFileDesc(iFileCustomWIFILID, &rec_size, &rec_num, ISREAD);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi6630 file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = read(iFD, &databuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] read wifi6630 offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_wifi6630info(WIFI6630_R, wr_offset, databuffer);
        break;
    case WIFI6630_W:
        wifi_nvram_fd = NVM_GetFileDesc(iFileCustomWIFILID, &rec_size, &rec_num, ISWRITE);
        iFD = wifi_nvram_fd.iFileDesc;
        if (0 > iFD)
        {
            LOG("[wfbteth_wr] open wifi6630 file fail! error:%s\n", strerror(errno));
            return -1;
        }
        lseek(iFD, wr_offset, SEEK_SET);
        if ((size = write(iFD, &argbuffer, DATA_LENGTH)) < 0)
        {
            LOG("[wfbteth_wr] write wifi6630 offset(%d) data fail! error:%s\n", wr_offset, strerror(errno));
            return -1;
        }
        print_wifi6630info(WIFI6630_W, wr_offset, argbuffer);
        sync();
        break;
    default:
        LOG("[wfbteth_wr] error: not support!\n");
        break;
    }

    if ((WIFI_W == cmd) || (WIFI_R == cmd) || (WIFI6630_W == cmd) || (WIFI6630_R == cmd))
    {
        NVM_CloseFileDesc(wifi_nvram_fd);
    }
    else if ((BT_W == cmd) || (BT_R == cmd) ||(BT6630_W == cmd) || (BT6630_R == cmd))
    {
        NVM_CloseFileDesc(bt_nvram_fd);
    }
	else if ((ETH_W == cmd) || (ETH_R == cmd) )
    {
        NVM_CloseFileDesc(eth_nvram_fd);
    }
#ifdef WFBTETH_WR_MAC
    else if ((WIFI_W_MAC == cmd) || (WIFI_R_MAC == cmd))
    {
        NVM_CloseFileDesc(wifi_nvram_fd);
    }
    else if ((BT_W_MAC == cmd) || (BT_R_MAC == cmd))
    {
        NVM_CloseFileDesc(bt_nvram_fd);
    }
	else if ((ETH_W_MAC == cmd) || (ETH_R_MAC == cmd))
    {
        NVM_CloseFileDesc(eth_nvram_fd);
    }
	else if ((MAC_TO_ETH == cmd) || (MAC_TO_NVRAM == cmd))
    {
        NVM_CloseFileDesc(eth_nvram_fd);
    }
#endif

    if ((WIFI_W == cmd) || (BT_W == cmd) || (WIFI6630_W == cmd) ||(BT6630_W == cmd) || (ETH_W == cmd)
#ifdef WFBTETH_WR_MAC
        || (WIFI_W_MAC == cmd) || (BT_W_MAC == cmd) || (ETH_W_MAC == cmd)
#endif
    )
    {
        if (false == FileOp_BackupToBinRegion_All())
        {
            LOG("[wfbteth_wr] backup to nvram binregion failed!\n");
        }
		LOG("[wfbteth_wr] backup to nvram binregion successfully.!\n");
    }
    return 0;
}

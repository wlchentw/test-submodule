/****************************************************************************
(c) Copyright 2014 Wi-Fi Alliance.  All Rights Reserved

Permission to use, copy, modify, and/or distribute this software for any purpose with or
without fee is hereby granted, provided that the above copyright notice and this permission
notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************************/

/*
 *   File: wfa_cs.c -- configuration and setup
 *   This file contains all implementation for the dut setup and control
 *   functions, such as network interfaces, ip address and wireless specific
 *   setup with its supplicant.
 *
 *   The current implementation is to show how these functions
 *   should be defined in order to support the Agent Control/Test Manager
 *   control commands. To simplify the current work and avoid any GPL licenses,
 *   the functions mostly invoke shell commands by calling linux system call,
 *   system("<commands>").
 *
 *   It depends on the differnt device and platform, vendors can choice their
 *   own ways to interact its systems, supplicants and process these commands
 *   such as using the native APIs.
 *
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <poll.h>

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_ver.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_ca.h"
#include "wfa_tlv.h"
#include "wfa_sock.h"
#include "wfa_tg.h"
#include "wfa_cmds.h"
#include "wfa_rsp.h"
#include "wfa_utils.h"
#ifdef WFA_WMM_PS_EXT
#include "wfa_wmmps.h"
#endif

#ifdef MTK_CFG80211_SIGMA
#include "wfa_wpa.h"
#endif


#ifdef MTK_P2P_SIGMA
extern wpa_private_t *p2p_priv_p;
#include "wfa_ralink.h"

#define MAX_RTSP_RETRY 2
#define DEFAULT_WFD_RTSP_PORT       7236

#define MTK_DERAULT_GO_INTENT       7
#define MTK_ENABLE_TDLS             1
#define WFD_SLEEP_TIME_BEFORE_CONN_ATTEMP   20

typedef struct wfd_init_0_argv
{
	unsigned char dev_addr[WFA_P2P_DEVID_LEN];
	unsigned char intent;
	unsigned int freq;
} wfdInit0Argv_t;


int wfdConnPeerIsInitiator = 0;
int wfdReinvokePeerIsInitiator = 0;
int wfdConnIamAutoGo = 0;
char p2pIsListen = 0;
int wfdTdlsEnable = 0;
char tdls_peer_ip[24] = "";
#ifndef MTK_BDP_SIGMA
char wlanIsFixIp = 0;
#endif
char concurrencyEnable = 0;
WORD p2pLisCh = 1;
char staAssocSsid[WFA_SSID_NAME_LEN] = "";
wfdInit0Argv_t argv_in;
#endif

#ifdef MTK_BDP_SIGMA
char wlanIsFixIp = 0;
#endif

static unsigned int fgIsDhcpOn = 0;

#define CERTIFICATES_PATH    "/data/misc/wifi"

#define DEFAULT_WFD_DEV_NAME    "Mediatek-Sink"

/* Some device may only support UDP ECHO, activate this line */
//#define WFA_PING_UDP_ECHO_ONLY 1
#define printf(x...)    fprintf(stderr, x)

#define WFA_ENABLED 1

extern unsigned short wfa_defined_debug;
int wfaExecuteCLI(char *CLI);

/* Since the two definitions are used all over the CA function */
char gCmdStr[WFA_CMD_STR_SZ];
#ifdef MTK_11N_AP_SIGMA
char gSSID[WFA_BUFF_512];
char gWpaBuff[WFA_BUF_STR_SZ];
char gPsk[WFA_BUF_STR_SZ];
char gApModeBuff[WFA_BUF_STR_SZ];
#endif

dutCmdResponse_t gGenericResp;
int wfaTGSetPrio(int sockfd, int tgClass);
void create_apts_msg(int msg, unsigned int txbuf[],int id);

int sret = 0;


#ifdef MTK_WFD_SIGMA
extern int wfaMtkWfdGetClientIp_Android(FILE *fp, char *mac, char *return_ip, int return_buf_len);
extern int wfaMtkWfdStartDhcpClient(char *return_ip, int return_buf_len, char intf_idx);
extern int wfaMtkWfdStartDhcpServer(char *client_mac, char *return_ip, int return_buf_len);
extern void wfaMtkWfdKillProcess(char * process_name);
extern int wfdMtkWfdCheckP2pConnResult(unsigned char *retcode, unsigned char *conn_mac, unsigned int size_of_mac);
extern int wfdMtkWfdCheckTdlsConnResult(unsigned char *retcode, unsigned char *dev_addr);
extern int wfdMtkWfdCheckP2pAndTdlsConnResult(unsigned char *retcode, unsigned char *dev_addr);
#endif /* MTK_WFD_SIGMA */

#define MTK_HS20_CMD(supp_cmd_fmt, ... ) \
    sprintf(gCmdStr, "wpa_cli -p/tmp/wpa_supplicant " supp_cmd_fmt, ##__VA_ARGS__); \
    puts("\n\t"); \
    puts(gCmdStr); \
    if (system(gCmdStr)) printf("\t\tSYSYTEM MTK_HS20_CMD FAIL: \n\t\t\t%s \n", gCmdStr);

#define MTK_WPA3_CMD MTK_HS20_CMD

extern char ctrl_if[];
extern char e2eResults[];

FILE *e2efp = NULL;
char gNonPrefChanStr[WFA_BUFF_64];

int tdls_delay_start = 0;

int channel2Freq(char channel)
{
    if (channel > 0 && channel <= 14)
        return channel * 5 + 2407;
    if (channel > 36 && channel <= 161)
        return 5000 + 5* channel;
    return 0;
}

int chk_ret_status()
{
    char *ret = getenv(WFA_RET_ENV);

    if(*ret == '1')
        return WFA_SUCCESS;
    else
        return WFA_FAILURE;
}

char *remove_ext (char* mystr, char dot) {
    char *retstr, *lastdot;
    // Error checks and allocate string.
    if (mystr == NULL)
        return NULL;
    if ((retstr = malloc(strlen (mystr) + 1)) == NULL)
        return NULL;
    // Make a copy and find the relevant characters.
    strcpy(retstr, mystr);
    lastdot = strrchr(retstr, dot);    // If it has an extension.
    if (lastdot != NULL) {
        *lastdot = '\0';
    }
    return retstr;
}
/*
 * agtCmdProcGetVersion(): response "ca_get_version" command to controller
 *  input:  cmd --- not used
 *          valLen -- not used
 *  output: parms -- a buffer to store the version info response.
 */
int agtCmdProcGetVersion(int len, BYTE *parms, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *getverResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "entering agtCmdProcGetVersion ...\n");

    getverResp->status = STATUS_COMPLETE;
    wSTRNCPY(getverResp->cmdru.version, WFA_SYSTEM_VER, WFA_VERNAM_LEN);

    wfaEncodeTLV(WFA_GET_VERSION_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)getverResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
#ifdef MTK_BDP_SIGMA
    /* If you run P2P-5.1.12 and then P2P-5.1.13, for SCC, DUT may can not establish P2P connection if UCC specific operation channel*
    *due to DUT didn't disconnect to AP (CH36)  in P2P-5.1.12*/
    DPRINT_INFO(WFA_OUT, "%s: disconnect AP to avoid interference "
                         "between test cases\n", __FUNCTION__);
    char cmdStr[128] = "\0";
    sprintf(cmdStr, "wpa_cli -i %s %s disconnect", WFA_STAUT_IF , ctrl_if);
    printf("  system: %s \n",cmdStr);
    system(cmdStr);
#endif
    return WFA_SUCCESS;
}

/*
 * wfaStaAssociate():
 *    The function is to force the station wireless I/F to re/associate
 *    with the AP.
 */
int wfaStaAssociate(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *assoc = (dutCommand_t *)caCmdBuf;
    char *ifname = assoc->intf;
    dutCmdResponse_t *staAssocResp = &gGenericResp;
    int freq = 0;

    DPRINT_INFO(WFA_OUT, "entering wfaStaAssociate ...\n");
    /*
     * if bssid appears, station should associate with the specific
     * BSSID AP at its initial association.
     * If it is different to the current associating AP, it will be forced to
     * roam the new AP
     */


#if 1
    FILE *fd;
    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;
    char idx[16];
    int  idx_i, i;
    char *tmpfilename3 = "./conn_status.txt";
    FILE *tmpfile3 = NULL;
    char conn_bssid[64];
    fd = fopen(filename, "r+");
    if ((NULL == fd) || (NULL == fgets(idx, 16, fd))) {
        printf("fail: %s \n", filename);
        staAssocResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_ASSOCIATE_RESP_TLV, 4, (BYTE *)staAssocResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;
    } else {
        idx_i = atoi(idx);
        DPRINT_INFO(WFA_OUT, "[%s]The network index is [%d]", __func__, idx_i);
    }
#endif
#ifdef MTK_CFG80211_SIGMA
    if (assoc->cmdsu.assoc.bssid[0] != '\0') {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 bssid %s",
                WFA_STAUT_IF, ctrl_if, assoc->cmdsu.assoc.bssid);
        system(gCmdStr);
    }
    if (assoc->cmdsu.assoc.channel) {
        freq = channel2Freq(assoc->cmdsu.assoc.channel);
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 scan_freq %d",
                WFA_STAUT_IF, ctrl_if, freq);
        system(gCmdStr);
    }
#if 1
	/* In MBO 5.2.3.3,
	     testAP will generate several SSID with same value, DUT may connect to wrong bssid. */
	/* Set correct bssid to corresponding network id setting */
	sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d bssid %s", WFA_STAUT_IF, ctrl_if,
				idx_i, assoc->cmdsu.assoc.bssid);
	sret = system(gCmdStr);
	printf("********** [DUT should connect to BSSID] ==> %s **********\n", assoc->cmdsu.assoc.bssid);

    sprintf(gCmdStr, "wpa_cli -i %s %s select_network %d", WFA_STAUT_IF, ctrl_if, idx_i);
#endif
    system(gCmdStr);
    printf("system: %s\n",gCmdStr);
    //make sure connection
    for (i = 0; i < 20; i++){
        MTK_HS20_CMD("-i %s status | busybox grep ^wpa_state= | cut -f2- -d= > %s \n",
                        ifname, tmpfilename3);

        tmpfile3 = fopen(tmpfilename3, "r+");
        if (fscanf(tmpfile3, "%s", conn_bssid) != EOF)
        {
            if (strncmp(conn_bssid, "COMPLETED", 9) == 0)
            {
                break;
            } else {
                MTK_HS20_CMD("-i %s status", ifname);
            }
        }
        else {
            DPRINT_INFO(WFA_OUT, "association times %d ...\n", i);
        }
        buzz_time(1000000);
    }

#else
    if(assoc->cmdsu.assoc.bssid[0] != '\0')
    {
        /* if (the first association) */
        /* just do initial association to the BSSID */

        /* Add bssid to network id 0. TODO: find correct network id? */
        /* All other set_network command use network id 0*/
        sprintf(gCmdStr, "wpa_cli -i%s set_network 0 bssid %s", ifname, assoc->cmdsu.assoc.bssid);
        sret = system(gCmdStr);

        /* else (station already associate to an AP) */
        /* Do forced roaming */

    }
    else
    {
        /* use 'ifconfig' command to bring down the interface (linux specific) */
        sprintf(gCmdStr, "ifconfig %s down", ifname);
        system(gCmdStr);

        /* use 'ifconfig' command to bring up the interface (linux specific) */
        sprintf(gCmdStr, "ifconfig %s up", ifname);
        system(gCmdStr);

        /* 
         *  use 'wpa_cli' command to force a 802.11 re/associate 
         *  (wpa_supplicant specific) 
         */
        sprintf(gCmdStr, "wpa_cli -i%s %s reassociate", ifname, ctrl_if);
        system(gCmdStr);
    }
#endif

    /*
     * Then report back to control PC for completion.
     * This does not have failed/error status. The result only tells
     * a completion.
     */
    staAssocResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_ASSOCIATE_RESP_TLV, 4, (BYTE *)staAssocResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaReAssociate():
 *    The function is to force the station wireless I/F to re/associate
 *    with the AP.
 */
int wfaStaReAssociate(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *reassoc = (dutCommand_t *)caCmdBuf;
    char *ifname = reassoc->intf;
    dutCmdResponse_t *staAssocResp = &gGenericResp;
#if 1
    int i;

    FILE *tmpfile1 = NULL;
    FILE *tmpfile2 = NULL;
    FILE *tmpfile3 = NULL;
    char ssid_id[8];
    char ssid_str[WFA_SSID_NAME_LEN];
    char conn_bssid[64];
    char *tmpfilename1 = "/tmp/bssid2ssid.txt";
    char *tmpfilename2 = "/tmp/ssid2id.txt";
    char *tmpfilename3 = "/tmp/conn_status.txt";

    MTK_HS20_CMD("-i %s ap_scan 1 ", ifname);
    MTK_HS20_CMD("-i %s ap_scan 1 ", ifname);
    MTK_HS20_CMD("-i %s ap_scan 1 ", ifname);
    MTK_HS20_CMD("-i %s reassociate \n", ifname);

    /* make sure connection */
    for(i=0; i<20; i++) {
        MTK_HS20_CMD("-i %s status | grep ^wpa_state= | cut -f2- -d= > %s \n",
                        ifname,
                        tmpfilename3);
        tmpfile3 = fopen(tmpfilename3, "r+");

        if(fscanf(tmpfile3, "%s", conn_bssid) != EOF) {
            if(/* conn_bssid != NULL && */ strncmp(conn_bssid, "COMPLETED", 9) == 0) {
                MTK_HS20_CMD("-i %s status | grep bssid | cut -f 2 -d = > %s \n",
                                ifname,
                                tmpfilename3);
                tmpfile3 = fopen(tmpfilename3, "r+");

                if(fscanf(tmpfile3, "%s", conn_bssid) != EOF) {
                    if(/* conn_bssid != NULL && */ strncmp(conn_bssid, reassoc->cmdsu.assoc.bssid, 17) == 0) {
                        DPRINT_INFO(WFA_OUT, "Reassociation to %s is connected ...\n", conn_bssid);
                        break;
                    } else {
                        DPRINT_INFO(WFA_OUT, "Reassociation to other target(tgt: %s)(ori: %s) ...\n",
                                                conn_bssid, reassoc->cmdsu.assoc.bssid);
                    }
                }
                break;
            }
        } else {
            DPRINT_INFO(WFA_OUT, "Reassociation times %d ...\n", i);
        }
        buzz_time(1000000);
    }
    fclose(tmpfile3);
    buzz_time(2000000);

    if(fgIsDhcpOn) {
        printf("Requesting DHCP IP!\n");

#if 0 /* MTK_WPA3_SIGMA */
        if(fgIsIpv6Dhcp)
            sprintf(gCmdStr,"dhclient -6 %s", ifname);
        else
            sprintf(gCmdStr,"dhclient %s", ifname);

        printf("%s[%u] %s", __FUNCTION__, __LINE__, gCmdStr);
        system(gCmdStr);
#else
        system("mtk_dhcp_client.sh wlan0");
#endif
        printf("DHCP DONE!\n");
    }

#else
    DPRINT_INFO(WFA_OUT, "entering wfaStaAssociate ...\n");
    /*
     * if bssid appears, station should associate with the specific
     * BSSID AP at its initial association.
     * If it is different to the current associating AP, it will be forced to
     * roam the new AP
     */
    if(reassoc->cmdsu.assoc.bssid[0] != '\0')
    {
        /* if (the first association) */
        /* just do initial association to the BSSID */


        /* else (station already associate to an AP) */
        /* Do forced roaming */

    }
    else
    {
        /* use 'ifconfig' command to bring down the interface (linux specific) */
        sprintf(gCmdStr, "ifconfig %s down", ifname);
        sret = system(gCmdStr);

        /* use 'ifconfig' command to bring up the interface (linux specific) */
        sprintf(gCmdStr, "ifconfig %s up", ifname);

        /*
         *  use 'wpa_cli' command to force a 802.11 re/associate
         *  (wpa_supplicant specific)
         */
        sprintf(gCmdStr, "wpa_cli -i%s %s reassociate", ifname, ctrl_if);
        sret = system(gCmdStr);
    }

#endif /* End of MTK_HS20_SIGMA */
    /*
     * Then report back to control PC for completion.
     * This does not have failed/error status. The result only tells
     * a completion.
     */
    staAssocResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_ASSOCIATE_RESP_TLV, 4, (BYTE *)staAssocResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaIsConnected():
 *    The function is to check whether the station's wireless I/F has
 *    already connected to an AP.
 */
int wfaStaIsConnected(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *connStat = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *staConnectResp = &gGenericResp;
    char *ifname = connStat->intf;
    FILE *tmpfile = NULL;
    char result[32];
    unsigned int connected = 0;



    DPRINT_INFO(WFA_OUT, "Entering isConnected ...\n");

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_chkconnect %s\n", ifname);
    sret = system(gCmdStr);

    if(chk_ret_status() == WFA_SUCCESS)
        staConnectResp->cmdru.connected = 1;
    else
        staConnectResp->cmdru.connected = 0;
#else

#ifdef MTK_BDP_SIGMA
    if (wlanIsFixIp == 0)
    {
        /* char peer_ip[24] = ""; */
        char strcmd[1024] = {0};
        printf("%s: start dhcp client\n", __FUNCTION__);
        /* wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 1); */
        sprintf(strcmd, "mtk_dhcp_client.sh %s", WFA_STAUT_IF);
        system(strcmd);
        system("sleep 2");
    }
#endif
    /*
     * use 'wpa_cli' command to check the interface status
     * none, scanning or complete (wpa_supplicant specific)
     */

    sprintf(gCmdStr, "wpa_cli -i%s %s status | busybox grep ^wpa_state= | busybox cut -f2- -d= > ./sigma_isConnected.txt", WFA_STAUT_IF, ctrl_if);

    sret = system(gCmdStr);
    DPRINT_INFO(WFA_OUT, "%s: cmd=[%s]\n", __FUNCTION__, gCmdStr);

    /*
     * the status is saved in a file.  Open the file and check it.
     */
    tmpfile = fopen("./sigma_isConnected.txt", "r+");
    if(tmpfile == NULL)
    {
        staConnectResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_IS_CONNECTED_RESP_TLV, 4, (BYTE *)staConnectResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    sret = fscanf(tmpfile, "%s", (char *)result);

    if(strncmp(result, "COMPLETED", 9) == 0)
        staConnectResp->cmdru.connected = 1;
    else
        staConnectResp->cmdru.connected = 0;

    if (tmpfile)
        fclose(tmpfile);
#endif

    /*
     * Report back the status: Complete or Failed.
     */
    staConnectResp->status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_STA_IS_CONNECTED_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)staConnectResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetIpConfig():
 * This function is to retriev the ip info including
 *     1. dhcp enable
 *     2. ip address
 *     3. mask
 *     4. primary-dns
 *     5. secondary-dns
 *
 *     The current implementation is to use a script to find these information
 *     and store them in a file.
 */
int wfaStaGetIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    int slen, ret, i = 0;
    dutCommand_t *getIpConf = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *ipconfigResp = &gGenericResp;
    char *ifname = getIpConf->intf;
    caStaGetIpConfigResp_t *ifinfo = &ipconfigResp->cmdru.getIfconfig;

    FILE *tmpfd;
    char string[512];
    char *str;

#ifdef MTK_CFG80211_SIGMA
    ifname = WFA_STAUT_IF;
    /* get ip addr */
    sprintf(gCmdStr, "busybox ifconfig %s |busybox grep \"inet addr\" |busybox cut -d ':' -f 2"
                     " | busybox cut -d ' ' -f 1 > /tmp/.ipAddr", ifname);
    DPRINT_INFO(WFA_OUT, "%s: cmd=[%s]\n", __FUNCTION__, gCmdStr);
    sret = system(gCmdStr);
    tmpfd = fopen("/tmp/.ipAddr", "r+");
    if(tmpfd)
    {
        memset(string, 0, sizeof(string));
        ifinfo->isDhcp = 0;
        sret = fscanf(tmpfd, "%s", (char *)string);
        if (string[0]!=0x00)
        {
            memset(ifinfo->ipaddr, 0, sizeof(ifinfo->ipaddr));
            strncpy(ifinfo->ipaddr, string, sizeof(ifinfo->ipaddr)-1);
            DPRINT_INFO(WFA_OUT, "%s: ipaddr=[%s]\n", __FUNCTION__, ifinfo->ipaddr);
        }
    }
    if (tmpfd)
        fclose(tmpfd);
    /* get ip mask */
    sprintf(gCmdStr, "busybox ifconfig %s |busybox grep \"inet addr\" |busybox cut -d ':' -f 4"
                     " > /tmp/.ipMask", ifname);
    DPRINT_INFO(WFA_OUT, "%s: cmd=[%s]\n", __FUNCTION__, gCmdStr);
    sret = system(gCmdStr);
    tmpfd = fopen("/tmp/.ipMask", "r+");
    if(tmpfd)
    {
        memset(string, 0, sizeof(string));
        sret = fscanf(tmpfd, "%s", (char *)string);
        if (string[0]!=0x00)
        {
            memset(ifinfo->mask, 0, sizeof(ifinfo->mask));
            strncpy(ifinfo->mask, string, sizeof(ifinfo->mask)-1);
            DPRINT_INFO(WFA_OUT, "%s: mask=[%s]\n", __FUNCTION__, ifinfo->mask);
        }
    }
    if (tmpfd)
        fclose(tmpfd);
    /* get mac addr */
    sprintf(gCmdStr, "busybox ifconfig %s |busybox grep HWaddr | busybox cut -d ' ' -f 10"
                     " > /tmp/.mac", ifname);
    DPRINT_INFO(WFA_OUT, "%s: cmd=[%s]\n", __FUNCTION__, gCmdStr);
    sret = system(gCmdStr);
    tmpfd = fopen("/tmp/.mac", "r+");
    if(tmpfd)
    {
        memset(string, 0, sizeof(string));
        sret = fscanf(tmpfd, "%s", (char *)string);
        if (string[0]!=0x00)
        {
            memset(ifinfo->mac, 0, sizeof(ifinfo->mac));
            strncpy(ifinfo->mac, string, sizeof(ifinfo->mac)-1);
            DPRINT_INFO(WFA_OUT, "%s: mac=[%s]\n", __FUNCTION__, ifinfo->mac);
        }
    }
    if (tmpfd)
        fclose(tmpfd);
    tmpfd = NULL;
    strcpy(ifinfo->dns[0], "0");
    strcpy(ifinfo->dns[1], "0");
#ifdef MTK_BDP_SIGMA
    ifinfo->isDhcp = wlanIsFixIp == 0 ? 1 : 0;
#endif
#else /* !MTK_CFG80211_SIGMA */

    /*
     * check a script file (the current implementation specific)
     */
    ret = access("/usr/local/sbin/getipconfig.sh", F_OK);
    if(ret == -1)
    {
        printf("%s - 1\n", __func__);
        ipconfigResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_GET_IP_CONFIG_RESP_TLV, 4, (BYTE *)ipconfigResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file not exist\n");
        return WFA_FAILURE;

    }

    strcpy(ifinfo->dns[0], "0");
    strcpy(ifinfo->dns[1], "0");

    /*
     * Run the script file "getipconfig.sh" to check the ip status
     * (current implementation  specific).
     * note: "getipconfig.sh" is only defined for the current implementation
     */
    sprintf(gCmdStr, "getipconfig.sh /tmp/sigma_ipconfig.txt %s\n", ifname); 

    sret = system(gCmdStr);

    /* open the output result and scan/retrieve the info */
    tmpfd = fopen("/tmp/sigma_ipconfig.txt", "r+");

    if(tmpfd == NULL)
    {
        printf("%s - 2\n", __func__);
        ipconfigResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_GET_IP_CONFIG_RESP_TLV, 4, (BYTE *)ipconfigResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    for(;;)
    {
        if(fgets(string, sizeof(string), tmpfd) == NULL)
            break; 

        /* check dhcp enabled */
        if(strncmp(string, "dhcpcli", 7) ==0)
        {
            str = strtok(string, "=");
            str = strtok(NULL, "=");
            if(str != NULL)
                ifinfo->isDhcp = 1;
            else
                ifinfo->isDhcp = 0;
        }

        /* find out the ip address */
        if(strncmp(string, "ipaddr", 6) == 0)
        {
            str = strtok(string, "=");
            str = strtok(NULL, " ");
            if(str != NULL)
            {
                wSTRNCPY(ifinfo->ipaddr, str, 15);

                ifinfo->ipaddr[15]='\0';
            }
            else
                wSTRNCPY(ifinfo->ipaddr, "none", 15);
        }

        /* check the mask */
        if(strncmp(string, "mask", 4) == 0)
        {
            char ttstr[16];
            char *ttp = ttstr;

            str = strtok_r(string, "=", &ttp);
            if(*ttp != '\0')
            {
                strcpy(ifinfo->mask, ttp);
                slen = strlen(ifinfo->mask);
                ifinfo->mask[slen-1] = '\0';
            }
            else
                strcpy(ifinfo->mask, "none");
        }

        /* find out the dns server ip address */
        if(strncmp(string, "nameserv", 8) == 0)
        {
            char ttstr[16];
            char *ttp = ttstr;

            str = strtok_r(string, " ", &ttp);
            if(str != NULL && i < 2)
            {
                strcpy(ifinfo->dns[i], ttp);
                slen = strlen(ifinfo->dns[i]);
                ifinfo->dns[i][slen-1] = '\0';
            }
            else
                strcpy(ifinfo->dns[i], "none");

            i++;
        }
    }

#endif /* MTK_CFG80211_SIGMA */
    /*
     * Report back the results
     */
    printf("%s - 3\n", __func__);
    ipconfigResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_IP_CONFIG_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)ipconfigResp, respBuf);   

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

	printf("%i %i %s %s %s %s %i\n", ipconfigResp->status, 
	   ifinfo->isDhcp, ifinfo->ipaddr, ifinfo->mask, 
		   ifinfo->dns[0], ifinfo->dns[1], *respLen);

    if (tdls_delay_start)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s tdls_start", WFA_STAUT_IF, ctrl_if);
	printf("%s", gCmdStr);
        printf("after IP got, do TDLS start\n");
        system(gCmdStr);
    }

    if (tmpfd)
    fclose(tmpfd);
    return WFA_SUCCESS;
}

/*
 * wfaStaSetIpConfig():
 *   The function is to set the ip configuration to a wireless I/F.
 *   1. IP address
 *   2. Mac address
 *   3. default gateway
 *   4. dns nameserver (pri and sec).
 */
int wfaStaSetIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *setIpConf = (dutCommand_t *)caCmdBuf;
    caStaSetIpConfig_t *ipconfig = &setIpConf->cmdsu.ipconfig;
    dutCmdResponse_t *staSetIpResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "entering wfaStaSetIpConfig isDhcp=%d\n", ipconfig->isDhcp);

#if 1
    if (fgIsDhcpOn) {
        /* TODO */
        sprintf(gCmdStr, "mtk_dhcp_client.sh %s", WFA_STAUT_IF);
        sret = system(gCmdStr);
        printf("system: %s\n",gCmdStr);
        staSetIpResp->status = STATUS_COMPLETE;
        wfaEncodeTLV(WFA_STA_SET_IP_CONFIG_RESP_TLV, 4, (BYTE *)staSetIpResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_SUCCESS;
    }
#endif
#ifdef MTK_BDP_SIGMA
    if (ipconfig->isDhcp == 1)
    {
        wlanIsFixIp = 0;
        sprintf(gCmdStr, "mtk_dhcp_client.sh %s", WFA_STAUT_IF);
        sret = system(gCmdStr);
        printf("system: %s\n",gCmdStr);
    }
    else
    {	
        wlanIsFixIp = 1;
#ifdef MTK_WFD_SIGMA
        sprintf(gCmdStr, "iwpriv %s set WfdLocalIp=%s", WFA_STAUT_IF, ipconfig->ipaddr);
        sret = system(gCmdStr);
        printf("system: %s\n",gCmdStr);
#endif
    }
#else
#ifdef MTK_P2P_SIGMA
    if (ipconfig->isDhcp == 1)
    {
        wlanIsFixIp = 0;
        sprintf(gCmdStr, "mtk_dhcp_client.sh %s", WFA_STAUT_IF);
        sret = system(gCmdStr);
        printf("system: %s\n", gCmdStr);
        printf("Wait 3 seconds to get IP\n");
        sleep(3);
    }
    else
    {
        wlanIsFixIp = 1;
        sprintf(gCmdStr, "iwpriv %s set WfdLocalIp=%s", WFA_STAUT_IF, ipconfig->ipaddr);
        sret = system(gCmdStr);
        printf("system: %s\n",gCmdStr);
    }
#endif
#endif /* MTK_BDP_SIGMA */


    /*
     * Use command 'ifconfig' to configure the interface ip address, mask.
     * (Linux specific).
     */
    if (ipconfig->isDhcp == 0) {
#ifdef MTK_CFG80211_SIGMA
    sprintf(gCmdStr, "busybox ifconfig %s %s netmask %s > /dev/null 2>&1 ", WFA_STAUT_IF, ipconfig->ipaddr, ipconfig->mask);
#endif
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    /* use command 'route add' to set set gatewway (linux specific) */ 
    if(ipconfig->defGateway[0] != '\0')
    {
#ifdef MTK_CFG80211_SIGMA	   
        sprintf(gCmdStr, "busybox route add default gw %s > /dev/null 2>&1", ipconfig->defGateway);
#else
        sprintf(gCmdStr, "/sbin/route add default gw %s > /dev/null 2>&1", ipconfig->defGateway);
#endif
        sret = system(gCmdStr);
    }
    printf("system: %s\n",gCmdStr);

    /* set dns (linux specific) */
    sprintf(gCmdStr, "cp /etc/resolv.conf /tmp/resolv.conf.bk");
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "echo nameserv %s > /etc/resolv.conf", ipconfig->pri_dns);
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "echo nameserv %s >> /etc/resolv.conf", ipconfig->sec_dns);
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);
    }
    /*
     * report status
     */
    staSetIpResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_IP_CONFIG_RESP_TLV, 4, (BYTE *)staSetIpResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaVerifyIpConnection():
 * The function is to verify if the station has IP connection with an AP by
 * send ICMP/pings to the AP.
 */
int wfaStaVerifyIpConnection(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *verip = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *verifyIpResp = &gGenericResp;

#ifndef WFA_PING_UDP_ECHO_ONLY
    char strout[64], *pcnt;
    FILE *tmpfile;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaVerifyIpConnection ...\n");

    /* set timeout value in case not set */
    if(verip->cmdsu.verifyIp.timeout <= 0)
    {
        verip->cmdsu.verifyIp.timeout = 10;
    }
   
    /* execute the ping command  and pipe the result to a tmp file */
#ifdef MTK_CFG80211_SIGMA
    sprintf(gCmdStr, "ping %s -c 3 -W %u | grep loss | busybox cut -f3 -d, 1>& /tmp/sigma_pingout.txt", verip->cmdsu.verifyIp.dipaddr, verip->cmdsu.verifyIp.timeout);
#else
    sprintf(gCmdStr, "ping %s -c3 -W%u | grep loss | cut -f3 -d, 1>& /tmp/sigma_pingout.txt", verip->cmdsu.verifyIp.dipaddr, verip->cmdsu.verifyIp.timeout);
#endif /* MTK_CFG80211_SIGMA */
    sret = system(gCmdStr); 

    /* scan/check the output */
    tmpfile = fopen("/tmp/sigma_pingout.txt", "r+");
    if(tmpfile == NULL)
    {
        verifyIpResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, 4, (BYTE *)verifyIpResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    verifyIpResp->status = STATUS_COMPLETE;
    if(fscanf(tmpfile, "%s", strout) == EOF)
        verifyIpResp->cmdru.connected = 0;
    else
    {
        pcnt = strtok(strout, "%");

        /* if the loss rate is 100%, not able to connect */
        if(atoi(pcnt) == 100)
            verifyIpResp->cmdru.connected = 0;
        else
            verifyIpResp->cmdru.connected = 1;
    }

    fclose(tmpfile);
#else
    int btSockfd;
    struct pollfd fds[2];
    int timeout = 2000;
    char anyBuf[64];
    struct sockaddr_in toAddr;
    int done = 1, cnt = 0, ret, nbytes;

    verifyIpResp->status = STATUS_COMPLETE;
    verifyIpResp->cmdru.connected = 0;

    btSockfd = wfaCreateUDPSock("127.0.0.1", WFA_UDP_ECHO_PORT);

    if(btSockfd == -1)
    {
        verifyIpResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, 4, (BYTE *)verifyIpResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;;
    }

    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = inet_addr(verip->cmdsu.verifyIp.dipaddr);
    toAddr.sin_port = htons(WFA_UDP_ECHO_PORT);

    while(done)
    {
        wfaTrafficSendTo(btSockfd, (char *)anyBuf, 64, (struct sockaddr *)&toAddr);
        cnt++;

        fds[0].fd = btSockfd;
        fds[0].events = POLLIN | POLLOUT;

        ret = poll(fds, 1, timeout);
        switch(ret)
        {
        case 0:
            /* it is time out, count a packet lost*/
            break;
        case -1:
        /* it is an error */
        default:
        {
            switch(fds[0].revents)
            {
            case POLLIN:
            case POLLPRI:
            case POLLOUT:
                nbytes = wfaTrafficRecv(btSockfd, (char *)anyBuf, (struct sockaddr *)&toAddr);
                if(nbytes != 0)
                    verifyIpResp->cmdru.connected = 1;
                done = 0;
                break;
            default:
                /* errors but not care */
                ;
            }
        }
        }
        if(cnt == 3)
        {
            done = 0;
        }
    }

#endif

    wfaEncodeTLV(WFA_STA_VERIFY_IP_CONNECTION_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)verifyIpResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

/*
 * wfaStaGetMacAddress()
 *    This function is to retrieve the MAC address of a wireless I/F.
 */
int wfaStaGetMacAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *getMac = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *getmacResp = &gGenericResp;
    char *str;
    char *ifname = getMac->intf;

    FILE *tmpfd;
    char string[257];

    DPRINT_INFO(WFA_OUT, "Entering wfaStaGetMacAddress ...\n");
    /*
     * run the script "getipconfig.sh" to find out the mac
     */
    //sprintf(gCmdStr, "getipconfig.sh /tmp/ipconfig.txt %s", ifname); 
#ifdef MTK_CFG80211_SIGMA
    sprintf(gCmdStr, "busybox ifconfig %s > /tmp/sigma_ipconfig.txt ", ifname); 
#endif
    sret = system(gCmdStr);

    tmpfd = fopen("/tmp/sigma_ipconfig.txt", "r+");
    if(tmpfd == NULL)
    {
        getmacResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_GET_MAC_ADDRESS_RESP_TLV, 4, (BYTE *)getmacResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    if(fgets((char *)&string[0], 256, tmpfd) == NULL)
    {
        getmacResp->status = STATUS_ERROR;
    }

    str = strtok(string, " ");
    while(str && ((strcmp(str,"HWaddr")) != 0))
    {
        str = strtok(NULL, " ");
    }

    /* get mac */
    if(str)
    {
        str = strtok(NULL, " ");
        strcpy(getmacResp->cmdru.mac, str);
        getmacResp->status = STATUS_COMPLETE;
    }

    wfaEncodeTLV(WFA_STA_GET_MAC_ADDRESS_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)getmacResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    fclose(tmpfd);
    return WFA_SUCCESS;
}

/*
 * wfaStaGetStats():
 * The function is to retrieve the statistics of the I/F's layer 2 txFrames,
 * rxFrames, txMulticast, rxMulticast, fcsErrors/crc, and txRetries.
 * Currently there is not definition how to use these info.
 */
int wfaStaGetStats(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *statsResp = &gGenericResp;

    /* this is never used, you can skip this call */

    statsResp->status = STATUS_ERROR;
    wfaEncodeTLV(WFA_STA_GET_STATS_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)statsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);


    return WFA_SUCCESS;
}

/*
 * wfaSetEncryption():
 *   The function is to set the wireless interface with WEP or none.
 *
 *   Since WEP is optional test, current function is only used for
 *   resetting the Security to NONE/Plaintext (OPEN). To test WEP,
 *   this function should be replaced by the next one (wfaSetEncryption1())
 *
 *   Input parameters:
 *     1. I/F
 *     2. ssid
 *     3. encpType - wep or none
 *     Optional:
 *     4. key1
 *     5. key2
 *     6. key3
 *     7. key4
 *     8. activeKey Index
 */

int wfaSetEncryption1(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEncryption_t *setEncryp = (caStaSetEncryption_t *)caCmdBuf;
    dutCmdResponse_t *setEncrypResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "entering wfaSetEncryption1 ...\n");
    /*
     * disable the network first
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", setEncryp->intf, ctrl_if);
    sret = system(gCmdStr);

    /*
     * set SSID
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", setEncryp->intf, ctrl_if, setEncryp->ssid);
    sret = system(gCmdStr);

    /*
     * Tell the supplicant for infrastructure mode (1)
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 mode 0", setEncryp->intf, ctrl_if);
    sret = system(gCmdStr);

    /*
     * set Key management to NONE (NO WPA) for plaintext or WEP
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt NONE", setEncryp->intf, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", setEncryp->intf, ctrl_if);
    sret = system(gCmdStr);

    setEncrypResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_ENCRYPTION_RESP_TLV, 4, (BYTE *)setEncrypResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 *  Since WEP is optional, this function could be used to replace
 *  wfaSetEncryption() if necessary.
 */
int wfaSetEncryption(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEncryption_t *setEncryp = (caStaSetEncryption_t *)caCmdBuf;
    dutCmdResponse_t *setEncrypResp = &gGenericResp;
    int i;

    DPRINT_INFO(WFA_OUT, "entering wfaSetEncryption ...\n");

    memset(setEncryp->intf, 0, sizeof(setEncryp->intf));
    strncpy(setEncryp->intf, WFA_STAUT_IF, strlen(WFA_STAUT_IF));
    memset(gCmdStr,0,sizeof(gCmdStr));

    /*
     * remove the network first
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s remove_network all", setEncryp->intf, ctrl_if);
    printf("%s", gCmdStr);
    sret = system(gCmdStr);

    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;

    /*
     * add_network first (create entry 0)
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s add_network > %s", setEncryp->intf, ctrl_if, filename);
    printf("%s", gCmdStr);
    sret = system(gCmdStr);

    /*
     * set SSID
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", setEncryp->intf, ctrl_if, setEncryp->ssid);
    printf("%s", gCmdStr);
    sret = system(gCmdStr);

    /*
     * Tell the supplicant for infrastructure mode (1)
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 mode 0", setEncryp->intf, ctrl_if);
    printf("%s", gCmdStr);
    sret = system(gCmdStr);

    /*
     * set Key management to NONE (NO WPA) for plaintext or WEP
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt NONE", setEncryp->intf, ctrl_if);
    printf("%s", gCmdStr);
    sret = system(gCmdStr);

    /* set keys */
    if(setEncryp->encpType == 1)
    {
        for(i=0; i<4; i++)
        {
            if(setEncryp->keys[i][0] != '\0')
            {
                sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 wep_key%i %s",
                        setEncryp->intf, ctrl_if, i, setEncryp->keys[i]);
                sret = system(gCmdStr);
               printf("%s", gCmdStr);
            }
        }

        /* set active key */
        i = setEncryp->activeKeyIdx;
        if(setEncryp->keys[i][0] != '\0')
        {
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 wep_tx_keyidx %i",
            setEncryp->intf, ctrl_if, setEncryp->activeKeyIdx);
            sret = system(gCmdStr);
			printf("%s", gCmdStr);
        }
    }
    else /* clearly remove the keys -- reported by p.schwann */
    {

        for(i = 0; i < 4; i++)
        {
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 wep_key%i \"\"", setEncryp->intf, ctrl_if, i);
            sret = system(gCmdStr);
			printf("%s", gCmdStr);
        }

        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 auth_alg OPEN", setEncryp->intf, ctrl_if);\
        printf("%s", gCmdStr);
        sret = system(gCmdStr);

    }

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", setEncryp->intf, ctrl_if);
    sret = system(gCmdStr);
    printf("%s", gCmdStr);

    setEncrypResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_ENCRYPTION_RESP_TLV, 4, (BYTE *)setEncrypResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetSecurity(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    int ret = WFA_SUCCESS;
#if 1
    caStaSetSecurity_t *setSecurity = (caStaSetSecurity_t *)caCmdBuf;
    dutCmdResponse_t *setSecurityResp = &gGenericResp;
    FILE *fd;
    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;
    char wpa_parameter[WFA_PARIWISE_CIPHER_LEN];
    char idx[16];
    int  idx_i;
    unsigned int fgIsOWE = FALSE;
    unsigned int fgIsSAE = FALSE;
    unsigned int fgIsReconfig = FALSE;
    unsigned int u4Pmf = 0;

    DPRINT_INFO(WFA_OUT, "Entering %s ...\n", __func__);

    /* wpa_cli -i wlan0 add_network */
    MTK_WPA3_CMD("-i %s add_network > %s", setSecurity->intf, filename);

    /* get net network index */
    fd = fopen(filename, "r+");
    if ((NULL == fd) || (NULL == fgets(idx, 16, fd))) {
        printf("fail: %s \n", filename);
        setSecurityResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setSecurityResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;
    } else {
        idx_i = atoi(idx);
        DPRINT_INFO(WFA_OUT, "[%s]The network index is [%d]", __func__, idx_i);
        /* maintained_network_idx = idx_i; */
    }

    /* wpa_cli -i wlan0 disable_network 0 */
    MTK_WPA3_CMD("-i %s disable_network %d", setSecurity->intf, idx_i);

    /* wpa_cli -i wlan0 set_network 0 ssid "ssud" */
    MTK_WPA3_CMD("-i %s set_network %d ssid '\"%s\"'", setSecurity->intf, idx_i, setSecurity->ssid);

    /* wpa_cli -i wlan0 set_network 0 pairwise xxxx */  /* union of pariwiseCipher and encpType */
    memset(wpa_parameter, 0, WFA_PARIWISE_CIPHER_LEN);
    if (setSecurity->type == SEC_TYPE_PSK)
        goto SET_PSK;

    if (strlen(setSecurity->pairwiseCipher)) {
        /* If there are multiple ciphers, space separated list will be provided. */
        if (strcasecmp(setSecurity->pairwiseCipher, "AES-GCMP-256")==0)
            strcat(wpa_parameter, "GCMP-256 ");  /* wpa_parameter has the same length to pairwiseCipher. No overflow concern here. */
        else if (strcasecmp(setSecurity->pairwiseCipher, "AES-CCMP-256")==0)
            strcat(wpa_parameter, "CCMP-256 ");
        else if (strcasecmp(setSecurity->pairwiseCipher, "AES-GCMP-128")==0)
            strcat(wpa_parameter, "GCMP ");
        else if (strcasecmp(setSecurity->pairwiseCipher, "AES-CCMP-128")==0)
            strcat(wpa_parameter, "CCMP ");
        else
            DPRINT_INFO(WFA_WNG, "Unknow pairwiseCipher %s", setSecurity->pairwiseCipher);
    } else if (strlen(setSecurity->encpType)) {
        if (strcasecmp(setSecurity->encpType, "TKIP")==0)
            strcat(wpa_parameter, "TKIP ");
        else if (strcasecmp(setSecurity->encpType, "AES-CCMP")==0)
            strcat(wpa_parameter, "CCMP ");
        else if (strcasecmp(setSecurity->encpType, "AES-GCMP")==0)
            strcat(wpa_parameter, "GCMP ");
        else
            DPRINT_INFO(WFA_WNG, "Unknow encpType %s", setSecurity->encpType);
    }
    if(strlen(wpa_parameter))
        MTK_WPA3_CMD("-i %s set_network %d pairwise %s", setSecurity->intf, idx_i, wpa_parameter);

    /* wpa_cli -i wlan0 set_network 0 group xxxx */  /* union of groupCipher and encpType */
    memset(wpa_parameter, 0, WFA_PARIWISE_CIPHER_LEN);
    if (strlen(setSecurity->groupCipher)) {
        /* If there are multiple ciphers, space separated list will be provided. */
        if (strcasecmp(setSecurity->groupCipher, "AES-GCMP-256")==0)
            strcat(wpa_parameter, "GCMP-256 ");  /* wpa_parameter has the same length to pairwiseCipher. No overflow concern here. */
        else if (strcasecmp(setSecurity->groupCipher, "AES-CCMP-256")==0)
            strcat(wpa_parameter, "CCMP-256 ");
        else if (strcasecmp(setSecurity->groupCipher, "AES-GCMP-128")==0)
            strcat(wpa_parameter, "GCMP ");
        else if (strcasecmp(setSecurity->groupCipher, "AES-CCMP-128")==0)
            strcat(wpa_parameter, "CCMP ");
        else
            DPRINT_INFO(WFA_WNG, "Unknow groupCipher %s", setSecurity->groupCipher);
    } else if(strlen(setSecurity->encpType)) {
        if (strcasecmp(setSecurity->encpType, "TKIP")==0)
            strcat(wpa_parameter, "TKIP ");
        else if (strcasecmp(setSecurity->encpType, "AES-CCMP")==0)
            strcat(wpa_parameter, "CCMP ");
        else if (strcasecmp(setSecurity->encpType, "AES-GCMP")==0)
            strcat(wpa_parameter, "GCMP ");
        else
            DPRINT_INFO(WFA_WNG, "Unknow encpType %s", setSecurity->encpType);
    }
    if(strlen(wpa_parameter))
        MTK_WPA3_CMD("-i %s set_network %d group %s", setSecurity->intf, idx_i, wpa_parameter);

    /* wpa_cli -i wlan0 set_network 0 group_mgmt xxxx */
    memset(wpa_parameter, 0, WFA_PARIWISE_CIPHER_LEN);
    if (strlen(setSecurity->groupMgmtCipher)) {
        /* If there are multiple ciphers, space separated list will be provided. */
        if (strcasecmp(setSecurity->groupMgmtCipher, "BIP-GMAC-256")==0)
            strcat(wpa_parameter, "BIP-GMAC-256 ");  /* wpa_parameter has the same length to pairwiseCipher. No overflow concern here. */
        else if (strcasecmp(setSecurity->groupMgmtCipher, "BIP-CMAC-256")==0)
            strcat(wpa_parameter, "BIP-CMAC-256 ");
        else if (strcasecmp(setSecurity->groupMgmtCipher, "BIP-GMAC-128")==0)
            strcat(wpa_parameter, "BIP-GMAC-128 ");
        else if (strcasecmp(setSecurity->groupMgmtCipher, "BIP-CMAC-128")==0)
            strcat(wpa_parameter, "AES-128-CMAC ");
        else
            DPRINT_INFO(WFA_WNG, "Unknow groupMgmtCipher %s", setSecurity->groupMgmtCipher);
    }
    if(strlen(wpa_parameter))
        MTK_WPA3_CMD("-i %s set_network %d group_mgmt %s", setSecurity->intf, idx_i, wpa_parameter);

    /* wpa_cli -i wlan0 set_network 0 key_mgmt xxxx */
    u4Pmf = setSecurity->pmf;
    memset(wpa_parameter, 0, WFA_PARIWISE_CIPHER_LEN);
    if(strcasecmp(setSecurity->keyMgmtType, "SuiteB")==0)
    {
        strcat(wpa_parameter, "WPA-EAP-SUITE-B-192");
        u4Pmf = 2; /* WPA3 UCC script may not send the mandatory PMF paramter .... */
    }
    else if (strcasecmp(setSecurity->keyMgmtType, "OWE")==0 || setSecurity->type == SEC_TYPE_OWE)
        /* Follow both defination in WiFi_TestSuite_Control_API_Specification_v10.2.0_security_v0.9 and UCC script cmds. */
    {
        fgIsOWE = TRUE;
        strcat(wpa_parameter, "OWE");
        u4Pmf = 2; /* WPA3 UCC script may not send the mandatory PMF paramter .... */
    }
    else if(setSecurity->type == SEC_TYPE_SAE)
    {
        fgIsSAE = TRUE;
        strcat(wpa_parameter, "SAE");
        u4Pmf = 2; /* WPA3 UCC script may not send the mandatory PMF paramter .... */
    }
    else if(setSecurity->type == SEC_TYPE_PSK_SAE) /* Transition compatibility mode that PSK and SAE are enabled */
    {
        fgIsSAE = TRUE;
        strcat(wpa_parameter, "SAE WPA-PSK");
        u4Pmf = 2; /* WPA3 UCC script may not send the mandatory PMF paramter .... */
    }
    else if(setSecurity->type == SEC_TYPE_PSK) {
SET_PSK:
        strcat(wpa_parameter, "WPA-PSK");
        goto SET_PSK2;
    }
    else if(setSecurity->type == SEC_TYPE_EAPTLS  ||
            setSecurity->type == SEC_TYPE_EAPTTLS ||
            setSecurity->type == SEC_TYPE_EAPPEAP ||
            setSecurity->type == SEC_TYPE_EAPSIM  ||
            setSecurity->type == SEC_TYPE_EAPFAST ||
            setSecurity->type == SEC_TYPE_EAPAKA
           )
        strcat(wpa_parameter, "WPA-EAP");
    else
        strcat(wpa_parameter, "NONE");

    if (strlen(wpa_parameter))
SET_PSK2:
        MTK_WPA3_CMD("-i %s set_network %d key_mgmt %s", setSecurity->intf, idx_i, wpa_parameter);

    if(setSecurity->type == SEC_TYPE_PSK)
        goto SET_PSK3;
    /* wpa_cli -i wlan0 set_network 0 ieee80211w x */
    MTK_WPA3_CMD("-i %s set_network %d ieee80211w %d", setSecurity->intf, idx_i, u4Pmf);

SET_PSK3:
    /* wpa_cli -i wlan0 set_network 0 psk "xxxxxxxx" */
    if(setSecurity->type == SEC_TYPE_PSK || setSecurity->type == SEC_TYPE_SAE ||setSecurity->type == SEC_TYPE_PSK_SAE)
        MTK_WPA3_CMD("-i %s set_network %d psk '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.passphrase);

    switch(setSecurity->type)
    {
        case SEC_TYPE_EAPTLS:
        /* Default Setting in sigma program located at "Wi-FiTestSuite\radius_agent\radiator and hostapd certs.zip" */
            MTK_WPA3_CMD("-i %s set_network %d eap TLS", setSecurity->intf, idx_i);
            if (wSTRNCMP(setSecurity->keyMgmtType, "wpa2", 4) == 0) {
                MTK_WPA3_CMD("-i %s set_network %d proto WPA2", setSecurity->intf, idx_i);
            } else {
                MTK_WPA3_CMD("-i %s set_network %d proto WPA", setSecurity->intf, idx_i);
            }

            MTK_WPA3_CMD("-i %s set_network %d ieee80211w 1", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d ca_cert '\"%s/%s.pem\"'", setSecurity->intf, idx_i, CERTIFICATES_PATH, setSecurity->secu.tls.trustedRootCA);
            MTK_WPA3_CMD("-i %s set_network %d client_cert '\"%s/%s.pem\"'", setSecurity->intf, idx_i, CERTIFICATES_PATH, setSecurity->secu.tls.clientCertificate);
            MTK_WPA3_CMD("-i %s set_network %d identity '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.tls.username);
            MTK_WPA3_CMD("-i %s set_network %d private_key '\"%s/%s.key\"'", setSecurity->intf, idx_i, CERTIFICATES_PATH, setSecurity->secu.tls.clientCertificate);
            MTK_WPA3_CMD("-i %s set_network %d private_key_passwd '\"wifi\"'", setSecurity->intf, idx_i);
            //MTK_WPA3_CMD("-i %s set_network %d proactive_key_caching 1", setSecurity->intf, idx_i);
            if(strcasecmp(setSecurity->keyMgmtType, "SuiteB")==0)
            {
                /* UCC does not set group_mgmt so that we hard code for that */
                MTK_WPA3_CMD("-i %s set_network %d group_mgmt BIP-GMAC-256", setSecurity->intf, idx_i);
            }
            break;

        case SEC_TYPE_EAPTTLS:
            MTK_WPA3_CMD("-i %s set_network %d eap TTLS", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d ieee80211w 1", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d priority %d", setSecurity->intf, idx_i, (setSecurity->secu.ttls.prefer==1?1:0));
            MTK_WPA3_CMD("-i %s set_network %d phase2 '\"auth=MSCHAPV2\"'", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d identity '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.ttls.username);
            MTK_WPA3_CMD("-i %s set_network %d password '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.ttls.passwd);
            MTK_WPA3_CMD("-i %s set_network %d client_cert %s", setSecurity->intf, idx_i, setSecurity->secu.ttls.clientCertificate);
            MTK_WPA3_CMD("-i %s set_network %d private_key NULL", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d ca_cert '\"%s/%s.pem\"'", setSecurity->intf, idx_i, CERTIFICATES_PATH, setSecurity->secu.ttls.trustedRootCA);
            MTK_WPA3_CMD("-i %s set_network %d imsi '\"none\"'", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d sim_slot '\"-1\"'", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d pcsc '\"none\"'", setSecurity->intf, idx_i);
            /* MTK_WPA3_CMD("-i %s set_network %d private_key_passwd '\"wifi\"'", setSecurity->intf, idx_i); */
            if(strcasecmp(setSecurity->keyMgmtType, "SuiteB")==0)
            {
                /* UCC does not set group_mgmt so that we hard code for that */
                MTK_WPA3_CMD("-i %s set_network %d group_mgmt BIP-GMAC-256", setSecurity->intf, idx_i);
            }
            break;

        case SEC_TYPE_EAPPEAP:
            MTK_WPA3_CMD("-i %s set_network %d eap PEAP", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d anonymous_identity '\"anonymous\"'", setSecurity->intf, idx_i);
            MTK_WPA3_CMD("-i %s set_network %d identity '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.peap.username);
            MTK_WPA3_CMD("-i %s set_network %d password '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.peap.passwd);
            MTK_WPA3_CMD("-i %s set_network %d ca_cert '\"%s\"'", setSecurity->intf, idx_i, setSecurity->secu.peap.trustedRootCA);
            MTK_WPA3_CMD("-i %s set_network %d phase1 '\"peaplabel=%i\"'", setSecurity->intf, idx_i, setSecurity->secu.peap.peapVersion);
            MTK_WPA3_CMD("-i %s set_network %d phase2 '\"auth=%s\"'", setSecurity->intf, idx_i, setSecurity->secu.peap.innerEAP);
            /* MTK_WPA3_CMD("-i %s set_network %d private_key_passwd '\"wifi\"'", setSecurity->intf, idx_i); */
            if(strcasecmp(setSecurity->keyMgmtType, "SuiteB")==0)
            {
                /* UCC does not set group_mgmt so that we hard code for that */
                MTK_WPA3_CMD("-i %s set_network %d group_mgmt BIP-GMAC-256", setSecurity->intf, idx_i);
            }
            break;

        case SEC_TYPE_EAPSIM:
            DPRINT_INFO(WFA_WNG, "[TODO] SEC_TYPE_EAPSIM\n");
            break;

        case SEC_TYPE_EAPFAST:
            DPRINT_INFO(WFA_WNG, "[TODO] SEC_TYPE_EAPFAST\n");
            break;

        case SEC_TYPE_EAPAKA:
            DPRINT_INFO(WFA_WNG, "[TODO] SEC_TYPE_EAPAKA\n");
            break;

        default:
            break;
    }

    if (strlen(setSecurity->certType)) {
        DPRINT_INFO(WFA_WNG, "[TODO] certType %s\n", setSecurity->certType);  /* should do anything about it? */
    }

    if (strlen(setSecurity->TLSCipher)) {
        MTK_WPA3_CMD("-i %s openssl_ciphers %s", setSecurity->intf, setSecurity->TLSCipher);
    }

    if (strlen(setSecurity->SAEInvalidElement)) {
       DPRINT_INFO(WFA_WNG, "[TODO] SAEInvalidElement %s\n", setSecurity->SAEInvalidElement);
    }

    if (strlen(setSecurity->ecGroupID)) {
        if (fgIsOWE == TRUE) {
            MTK_WPA3_CMD("-i %s set_network %d owe_group %s", setSecurity->intf, idx_i, setSecurity->ecGroupID);
        } else if (fgIsSAE == TRUE) {
            DPRINT_INFO(WFA_WNG, "Set SAE ecGroupID %s\n", setSecurity->ecGroupID);  /* your supplicant needs patch CL404517 at DaVanci server for SAE_GROUPS. */
            MTK_WPA3_CMD("-i %s set sae_groups %s", setSecurity->intf, setSecurity->ecGroupID);
        }
    }

    /* wpa_cli -i wlan0 enable_network 0 */
    MTK_WPA3_CMD("-i %s enable_network %d", setSecurity->intf, idx_i);

    /* wpa_cli -i wlan0 save config */
    MTK_WPA3_CMD("-i %s save_config", setSecurity->intf);

    /* wpa_cli -i wlan0 reconfigure */
    if(fgIsReconfig == TRUE) {
        MTK_WPA3_CMD("-i %s reconfigure", setSecurity->intf);
    }

    setSecurityResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setSecurityResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
#endif
    return ret;
}


/*
 * wfaStaSetEapTLS():
 *   This is to set
 *   1. ssid
 *   2. encrypType - tkip or aes-ccmp
 *   3. keyManagementType - wpa or wpa2
 *   4. trustedRootCA
 *   5. clientCertificate
 */
int wfaStaSetEapTLS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapTLS_t *setTLS = (caStaSetEapTLS_t *)caCmdBuf;
    char *ifname = setTLS->intf;
    int  idx_i;
    char idx[16];
    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;
    FILE *fd;
    char *trustedRootCAStr;
    char *clientCertificateStr;
    dutCmdResponse_t *setEapTlsResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaSetEapTLS ...\n");
    MTK_WPA3_CMD("-i %s remove_network all", ifname);

    /*
     * need to store the trustedROOTCA and clientCertificate into a file first.
     */
    MTK_WPA3_CMD("-i %s add_network > %s", ifname, filename);

    fd = fopen(filename, "r+");
    if ((NULL == fd) || (NULL == fgets(idx, 16, fd))) {
        printf("fail: %s \n", filename);
        setEapTlsResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setEapTlsResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;
    } else {
        idx_i = atoi(idx);
        DPRINT_INFO(WFA_OUT, "[%s]The network index is [%d]", __func__, idx_i);
    }
    trustedRootCAStr = remove_ext(setTLS->trustedRootCA, '.');
    clientCertificateStr = remove_ext(setTLS->clientCertificate, '.');

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eaptls -i %s %s %s %s", ifname, setTLS->ssid, setTLS->trustedRootCA, setTLS->clientCertificate);
    sret = system(gCmdStr);
#else

    MTK_WPA3_CMD("-i %s %s disable_network %d", ifname, ctrl_if, idx_i);

    /* ssid */
    MTK_WPA3_CMD("-i %s %s set_network %d ssid '\"%s\"'", ifname, ctrl_if, idx_i, setTLS->ssid);

    /* key management */
    if(strcasecmp(setTLS->keyMgmtType, "wpa2-sha256") == 0)
    {
    }
    else if(strcasecmp(setTLS->keyMgmtType, "wpa2-eap") == 0)
    {
    }
    else if(strcasecmp(setTLS->keyMgmtType, "wpa2-ft") == 0)
    {

    }
    else if((strcasecmp(setTLS->keyMgmtType, "wpa") == 0) ||
            (strcasecmp(setTLS->keyMgmtType, "wpa2") == 0))
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d key_mgmt WPA-EAP", ifname, ctrl_if, idx_i);
    }
    else
    {
        printf("%s: unknown key management type\n", __func__);
    }
    printf("%s\n", gCmdStr);
    sret = system(gCmdStr);

    MTK_WPA3_CMD("-i %s %s set_network %d ieee80211w %d", ifname, ctrl_if, idx_i, setTLS->pmf);
    /* protocol WPA */
    if (strcasecmp(setTLS->keyMgmtType, "wpa2") == 0) {
        MTK_WPA3_CMD("-i  %s set_network %d proto WPA2", ifname, idx_i);
    } else {
        MTK_WPA3_CMD("-i %s set_network %d proto WPA", ifname, idx_i);
    }

    MTK_WPA3_CMD("-i %s %s set_network %d eap TLS", ifname, ctrl_if, idx_i);

    MTK_WPA3_CMD("-i %s %s set_network %d ca_cert '\"%s/%s.pem\"'", ifname, ctrl_if, idx_i, CERTIFICATES_PATH, trustedRootCAStr);

    MTK_WPA3_CMD("-i %s %s set_network %d client_cert '\"%s/%s.pem\"'", ifname, ctrl_if, idx_i, CERTIFICATES_PATH, clientCertificateStr);

    if (wSTRNCMP(setTLS->username, "", 1) == 0) {
        MTK_WPA3_CMD("-i %s %s set_network %d identity '\"wifi-user@wifilabs.local\"'", ifname, ctrl_if, idx_i);
    } else {
        MTK_WPA3_CMD("-i %s %s set_network %d identity '\"%s\"'", ifname, ctrl_if, idx_i, setTLS->username);
    }
    MTK_WPA3_CMD("-i %s %s set_network %d private_key '\"%s/%s.key\"'", ifname, ctrl_if, idx_i, CERTIFICATES_PATH, clientCertificateStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d private_key_passwd '\"wifi\"'", ifname, ctrl_if, idx_i);
    printf("%s\n", gCmdStr);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network %d", ifname, ctrl_if, idx_i);
    printf("%s\n", gCmdStr);
    sret = system(gCmdStr);
#endif

    setEapTlsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPTLS_RESP_TLV, 4, (BYTE *)setEapTlsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * The function is to set
 *   1. ssid
 *   2. passPhrase
 *   3. keyMangementType - wpa/wpa2
 *   4. encrypType - tkip or aes-ccmp
 */
int wfaStaSetPSK(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;
    /*Incompleted function*/
    dutCmdResponse_t *setPskResp = &gGenericResp;

#ifndef WFA_PC_CONSOLE
    caStaSetPSK_t *setPSK = (caStaSetPSK_t *)caCmdBuf;
    DPRINT_INFO(WFA_OUT, "entering wfaSetPSK ...:%s\n", setPSK->keyMgmtType);

    memset(setPSK->intf, 0, sizeof(setPSK->intf));
    strncpy(setPSK->intf, WFA_STAUT_IF, strlen(WFA_STAUT_IF));
#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_psk %s %s %s", setPSK->intf, setPSK->ssid, setPSK->passphrase);
    sret = system(gCmdStr);
#else

    memset(gCmdStr,0,sizeof(gCmdStr));

    sprintf(gCmdStr, "wpa_cli -i %s %s remove_network all", setPSK->intf, ctrl_if); 
    system(gCmdStr);	
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s add_network > %s", setPSK->intf, ctrl_if, filename);
    system(gCmdStr);	
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", setPSK->intf, ctrl_if, setPSK->ssid); 
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 auth_alg OPEN", setPSK->intf, ctrl_if); 
    system(gCmdStr);	
    printf("system: %s\n",gCmdStr);


    if(strcasecmp(setPSK->keyMgmtType, "wpa2-sha256") == 0)
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA2-SHA256", setPSK->intf, ctrl_if); 
    else if((strcasecmp(setPSK->keyMgmtType, "wpa2-psk") == 0) ||
            (strcasecmp(setPSK->keyMgmtType, "wpa2") == 0) ||
		    (strcasecmp(setPSK->keyMgmtType, "wpa2-wpa-psk") == 0))
    {
#ifdef PMF_SIGMA
        if (setPSK->pmf)
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-PSK WPA-PSK-SHA256", setPSK->intf, ctrl_if);
        else
#endif /* PMF_SIGMA */
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-PSK", setPSK->intf, ctrl_if);
    }
    else if(strcasecmp(setPSK->keyMgmtType, "wpa2-ft") == 0)
    {

    }
    else
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-PSK", setPSK->intf, ctrl_if); 

    DPRINT_INFO(WFA_OUT, "Key_Mgmt Cmd: %s\n", gCmdStr);	
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

#ifdef PMF_SIGMA
    if (setPSK->pmf)
    {
        /* security */
        if ((strcasecmp(setPSK->keyMgmtType, "wpa2-psk") == 0) ||
            (strcasecmp(setPSK->keyMgmtType, "wpa2") == 0))
        {
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 proto RSN", setPSK->intf, ctrl_if);
            sret = system(gCmdStr);
        }

        if (setPSK->encpType == ENCRYPT_AESCCMP)
        {
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 pairwise CCMP", setPSK->intf, ctrl_if);		  
            DPRINT_INFO(WFA_OUT, "encpType cmd: %s\n", gCmdStr); 
            sret = system(gCmdStr);
        }
    }

    /* if PMF enable */
    if(setPSK->pmf == WFA_ENABLED || setPSK->pmf == WFA_OPTIONAL)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ieee80211w 1", setPSK->intf, ctrl_if);
        DPRINT_INFO(WFA_OUT, "pmf cmd: %s\n", gCmdStr);
        sret = system(gCmdStr);
    }
    else if(setPSK->pmf == WFA_REQUIRED)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ieee80211w 2", setPSK->intf, ctrl_if);
        DPRINT_INFO(WFA_OUT, "pmf cmd: %s\n", gCmdStr);
        sret = system(gCmdStr);
    }
    else if(setPSK->pmf == WFA_F_REQUIRED)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ieee80211w 2", setPSK->intf, ctrl_if);
        DPRINT_INFO(WFA_OUT, "pmf cmd: %s\n", gCmdStr);
        sret = system(gCmdStr);
    }
    else if(setPSK->pmf == WFA_F_DISABLED)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ieee80211w 0", setPSK->intf, ctrl_if);
        DPRINT_INFO(WFA_OUT, "pmf cmd: %s\n", gCmdStr);
        sret = system(gCmdStr);
    }
    else
    {
        /* Disable PMF */
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ieee80211w 0", setPSK->intf, ctrl_if);
        DPRINT_INFO(WFA_OUT, "pmf cmd: %s\n", gCmdStr);
        sret = system(gCmdStr);
    }

#else
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 pairwise CCMP TKIP", setPSK->intf, ctrl_if); 
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);
#endif /* PMF_SIGMA */

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 psk '\"%s\"'", setPSK->intf, ctrl_if, setPSK->passphrase); 
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", setPSK->intf, ctrl_if, setPSK->ssid); 
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);

    /*************************************************/
    sprintf(gCmdStr, "wpa_cli -i %s %s p2p_stop_find", WFA_STAUT_IF_P2P, ctrl_if);
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);


    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", WFA_STAUT_IF, ctrl_if);
    sret = system(gCmdStr);
    printf("system: %s\n",gCmdStr);
#endif

#endif
    setPskResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_PSK_RESP_TLV, 4, (BYTE *)setPskResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaGetInfo():
 * Get vendor specific information in name/value pair by a wireless I/F.
 */
int wfaStaGetInfo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    dutCommand_t *getInfo = (dutCommand_t *)caCmdBuf;

    /*
     * Normally this is called to retrieve the vendor information
     * from a interface, no implement yet
     */
    sprintf(infoResp.cmdru.info, "interface,%s,vendor,Mediatek,cardtype,802.11a/b/g", getInfo->intf);

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_INFO_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetEapTTLS():
 *   This is to set
 *   1. ssid
 *   2. username
 *   3. passwd
 *   4. encrypType - tkip or aes-ccmp
 *   5. keyManagementType - wpa or wpa2
 *   6. trustedRootCA
 */
int wfaStaSetEapTTLS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapTTLS_t *setTTLS = (caStaSetEapTTLS_t *)caCmdBuf;
    char *ifname = setTTLS->intf;
    int  idx_i;
    char idx[16];
    char *filename = MTK_WPA_CLI_OUTPUT_BUFFER_PATH;
    FILE *fd;
    dutCmdResponse_t *setEapTtlsResp = &gGenericResp;
    char *trustedRootCAStr;

    fd = fopen(filename, "r+");
    if ((NULL == fd) || (NULL == fgets(idx, 16, fd))) {
        printf("fail: %s \n", filename);
        setEapTtlsResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_SET_SECURITY_RESP_TLV, 4, (BYTE *)setEapTtlsResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
        return WFA_FAILURE;
    } else {
        idx_i = atoi(idx);
        DPRINT_INFO(WFA_OUT, "[%s]The network index is [%d]", __func__, idx_i);
        /* maintained_network_idx = idx_i; */
    }
    trustedRootCAStr = remove_ext(setTTLS->trustedRootCA, '.');
#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eapttls %s %s %s %s %s", ifname, setTTLS->ssid, setTTLS->username, setTTLS->passwd, setTTLS->trustedRootCA);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", ifname, ctrl_if, setTTLS->ssid);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 identity '\"%s\"'", ifname, ctrl_if, setTTLS->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 password '\"%s\"'", ifname, ctrl_if, setTTLS->passwd);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    sret = system(gCmdStr);

    /* This may not need to set. if it is not set, default to take all */
//   sprintf(cmdStr, "wpa_cli -i %s set_network 0 pairwise '\"%s\"", ifname, setTTLS->encrptype);
    if(strcasecmp(setTTLS->keyMgmtType, "wpa2-sha256") == 0)
    {
    }
    else if(strcasecmp(setTTLS->keyMgmtType, "wpa2-eap") == 0)
    {
    }
    else if(strcasecmp(setTTLS->keyMgmtType, "wpa2-ft") == 0)
    {

    }
    else if(strcasecmp(setTTLS->keyMgmtType, "wpa") == 0)
    {

    }
    else if(strcasecmp(setTTLS->keyMgmtType, "wpa2") == 0)
    {
        // to take all and device to pick one it supported
    }
    else
    {
        // ??
    }
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d eap TTLS", ifname, ctrl_if, idx_i);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d ca_cert '\"%s/%s\"'", ifname, ctrl_if, idx_i, CERTIFICATES_PATH, trustedRootCAStr);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d proto WPA", ifname, ctrl_if, idx_i);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network %d phase2 '\"auth=MSCHAPV2\"'", ifname, ctrl_if, idx_i);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network %d", ifname, ctrl_if, idx_i);
    sret = system(gCmdStr);
#endif

    setEapTtlsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPTTLS_RESP_TLV, 4, (BYTE *)setEapTtlsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetEapSIM():
 *   This is to set
 *   1. ssid
 *   2. user name
 *   3. passwd
 *   4. encrypType - tkip or aes-ccmp
 *   5. keyMangementType - wpa or wpa2
 */
int wfaStaSetEapSIM(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapSIM_t *setSIM = (caStaSetEapSIM_t *)caCmdBuf;
    char *ifname = setSIM->intf;
    dutCmdResponse_t *setEapSimResp = &gGenericResp;

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eapsim %s %s %s %s", ifname, setSIM->ssid, setSIM->username, setSIM->encrptype);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", ifname, ctrl_if, setSIM->ssid);
    sret = system(gCmdStr);


    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 identity '\"%s\"'", ifname, ctrl_if, setSIM->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 pairwise '\"%s\"'", ifname, ctrl_if, setSIM->encrptype);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 eap SIM", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 proto WPA", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    if(strcasecmp(setSIM->keyMgmtType, "wpa2-sha256") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-SHA256", ifname, ctrl_if);
    }
    else if(strcasecmp(setSIM->keyMgmtType, "wpa2-eap") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setSIM->keyMgmtType, "wpa2-ft") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-FT", ifname, ctrl_if);
    }
    else if(strcasecmp(setSIM->keyMgmtType, "wpa") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setSIM->keyMgmtType, "wpa2") == 0)
    {
        // take all and device to pick one which is supported.
    }
    else
    {
        // ??
    }
    sret = system(gCmdStr);

#endif

    setEapSimResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPSIM_RESP_TLV, 4, (BYTE *)setEapSimResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetPEAP()
 *   This is to set
 *   1. ssid
 *   2. user name
 *   3. passwd
 *   4. encryType - tkip or aes-ccmp
 *   5. keyMgmtType - wpa or wpa2
 *   6. trustedRootCA
 *   7. innerEAP
 *   8. peapVersion
 */
int wfaStaSetPEAP(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapPEAP_t *setPEAP = (caStaSetEapPEAP_t *)caCmdBuf;
    char *ifname = setPEAP->intf;
    dutCmdResponse_t *setPeapResp = &gGenericResp;

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_peap %s %s %s %s %s %s %i %s", ifname, setPEAP->ssid, setPEAP->username,
            setPEAP->passwd, setPEAP->trustedRootCA,
            setPEAP->encrptype, setPEAP->peapVersion,
            setPEAP->innerEAP);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", ifname, setPEAP->ssid, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 eap PEAP", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 anonymous_identity '\"anonymous\"' ", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 identity '\"%s\"'", ifname, ctrl_if, setPEAP->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 password '\"%s\"'", ifname, ctrl_if, setPEAP->passwd);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ca_cert '\"%s/%s\"'", ifname, ctrl_if, CERTIFICATES_PATH, setPEAP->trustedRootCA);
    sret = system(gCmdStr);

    if(strcasecmp(setPEAP->keyMgmtType, "wpa2-sha256") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-SHA256", ifname, ctrl_if);
    }
    else if(strcasecmp(setPEAP->keyMgmtType, "wpa2-eap") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setPEAP->keyMgmtType, "wpa2-ft") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-FT", ifname, ctrl_if);
    }
    else if(strcasecmp(setPEAP->keyMgmtType, "wpa") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setPEAP->keyMgmtType, "wpa2") == 0)
    {
        // take all and device to pick one which is supported.
    }
    else
    {
        // ??
    }
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 phase1 '\"peaplabel=%i\"'", ifname, ctrl_if, setPEAP->peapVersion);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 phase2 '\"auth=%s\"'", ifname, ctrl_if, setPEAP->innerEAP);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);
#endif

    setPeapResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_PEAP_RESP_TLV, 4, (BYTE *)setPeapResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetUAPSD()
 *    This is to set
 *    1. acBE
 *    2. acBK
 *    3. acVI
 *    4. acVO
 */
int wfaStaSetUAPSD(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *setUAPSDResp = &gGenericResp;
#if 1 /* used for only one specific device, need to update to reflect yours */
    caStaSetUAPSD_t *setUAPSD = (caStaSetUAPSD_t *)caCmdBuf;
    char *ifname = setUAPSD->intf;
    /* char tmpStr[10]; */
    char line[100];
    char *pathl="/etc/Wireless/RT61STA";
    char acTriggertype = 0;
    BYTE acBE=1;
    BYTE acBK=1;
    BYTE acVO=1;
    BYTE acVI=1;
    BYTE APSDCapable;
    FILE *pipe;

    /*
     * A series of setting need to be done before doing WMM-PS
     * Additional steps of configuration may be needed.
     */
#if 0
    /*
     * bring down the interface
     */
    sprintf(gCmdStr, "ifconfig %s down",ifname);
    sret = system(gCmdStr);
    /*
     * Unload the Driver
     */
    sprintf(gCmdStr, "rmmod rt61");
    sret = system(gCmdStr);
#endif
#ifndef WFA_WMM_AC
    if(setUAPSD->acBE != 1)
        acBE=setUAPSD->acBE = 0;
    if(setUAPSD->acBK != 1)
        acBK=setUAPSD->acBK = 0;
    if(setUAPSD->acVO != 1)
        acVO=setUAPSD->acVO = 0;
    if(setUAPSD->acVI != 1)
        acVI=setUAPSD->acVI = 0;
#else
    acBE=setUAPSD->acBE;
    acBK=setUAPSD->acBK;
    acVO=setUAPSD->acVO;
    acVI=setUAPSD->acVI;
#endif
    if(acBE == 1) {
        acTriggertype |= BIT(0);
    }
    if(acBK == 1) {
        acTriggertype |= BIT(1);
    }
    if(acVI == 1) {
        acTriggertype |= BIT(2);
    }
    if(acVO == 1) {
        acTriggertype |= BIT(3);
    }

    APSDCapable = acBE||acBK||acVO||acVI;
    /*
     * set other AC parameters
     */

    MTK_IWPRIV_CMD("%s set_sw_ctrl 0x10010003 0x%x",ifname,acTriggertype);
#if 0
    sprintf(tmpStr,"%d;%d;%d;%d",setUAPSD->acBE,setUAPSD->acBK,setUAPSD->acVI,setUAPSD->acVO);
    sprintf(gCmdStr, "sed -e \"s/APSDCapable=.*/APSDCapable=%d/g\" -e \"s/APSDAC=.*/APSDAC=%s/g\" %s/rt61sta.dat >/tmp/wfa_tmp",APSDCapable,tmpStr,pathl);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "mv /tmp/wfa_tmp %s/rt61sta.dat",pathl);
    sret = system(gCmdStr);
    pipe = popen("uname -r", "r");
    /* Read into line the output of uname*/
    fscanf(pipe,"%s",line);
    pclose(pipe);

    /*
     * load the Driver
     */
    sprintf(gCmdStr, "insmod /lib/modules/%s/extra/rt61.ko",line);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "ifconfig %s up",ifname);
    sret = system(gCmdStr);
#endif
#endif

    setUAPSDResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_UAPSD_RESP_TLV, 4, (BYTE *)setUAPSDResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaDeviceGetInfo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *dutCmd = (dutCommand_t *)caCmdBuf;
    caDevInfo_t *devInfo = &dutCmd->cmdsu.dev;
    dutCmdResponse_t *infoResp = &gGenericResp;
    /*a vendor can fill in the proper info or anything non-disclosure */
#ifdef MTK_WFD_SIGMA
    caDeviceGetInfoResp_t dinfo = {"Mediatek", "WFD", MTK_WIRELESS_VER};
#else
    caDeviceGetInfoResp_t dinfo = {WFA_VENDOR, WFA_MODEL, WFA_SYSTEM_VER, WFA_FIRMWARE};
#endif

    DPRINT_INFO(WFA_OUT, "Entering wfaDeviceGetInfo ...\n");

    if(devInfo->fw == 0)
        memcpy(&infoResp->cmdru.devInfo, &dinfo, sizeof(caDeviceGetInfoResp_t));
    else
    {
        // Call internal API to pull the version ID */
        memcpy(&infoResp->cmdru.devInfo, &dinfo, sizeof(caDeviceGetInfoResp_t));
    }

    infoResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_DEVICE_GET_INFO_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;

}

/*
 * This funciton is to retrieve a list of interfaces and return
 * the list back to Agent control.
 * ********************************************************************
 * Note: We intend to make this WLAN interface name as a hardcode name.
 * Therefore, for a particular device, you should know and change the name
 * for that device while doing porting. The MACRO "WFA_STAUT_IF" is defined in
 * the file "inc/wfa_ca.h". If the device OS is not linux-like, this most
 * likely is hardcoded just for CAPI command responses.
 * *******************************************************************
 *
 */
int wfaDeviceListIF(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *infoResp = &gGenericResp;
    dutCommand_t *ifList = (dutCommand_t *)caCmdBuf;
    caDeviceListIFResp_t *ifListResp = &infoResp->cmdru.ifList;

    DPRINT_INFO(WFA_OUT, "Entering wfaDeviceListIF ...\n");
   
#ifdef MTK_WFD_SIGMA
    {
        extern int wfdMtkWfdQuitConnectCheck(void);
        wfdMtkWfdQuitConnectCheck();
    }
#endif
    switch(ifList->cmdsu.iftype)
    {
        case IF_80211:
            infoResp->status = STATUS_COMPLETE;
            ifListResp->iftype = IF_80211; 
#ifdef MTK_P2P_SIGMA
            strcpy(ifListResp->ifs[0], WFA_STAUT_IF_P2P);
#else
            strcpy(ifListResp->ifs[0], WFA_STAUT_IF);
#endif
            strcpy(ifListResp->ifs[1], WFA_STAUT_IF);
            strcpy(ifListResp->ifs[2], "NULL");
            break;
        case IF_ETH:
            infoResp->status = STATUS_COMPLETE;
            ifListResp->iftype = IF_ETH; 
            strcpy(ifListResp->ifs[0], "eth0");
            strcpy(ifListResp->ifs[1], "NULL");
            strcpy(ifListResp->ifs[2], "NULL");
            break;
        default:
        {
            infoResp->status = STATUS_ERROR;
            wfaEncodeTLV(WFA_DEVICE_LIST_IF_RESP_TLV, 4, (BYTE *)infoResp, respBuf);   
            *respLen = WFA_TLV_HDR_LEN + 4;

            return WFA_SUCCESS; 
        }
    }
   
    wfaEncodeTLV(WFA_DEVICE_LIST_IF_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)infoResp, respBuf);   
   *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    return WFA_SUCCESS;
}

int wfaStaDebugSet(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *debugResp = &gGenericResp;
    dutCommand_t *debugSet = (dutCommand_t *)caCmdBuf;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaDebugSet ...\n");

    if(debugSet->cmdsu.dbg.state == 1) /* enable */
        wfa_defined_debug |= debugSet->cmdsu.dbg.level;
    else
        wfa_defined_debug = (~debugSet->cmdsu.dbg.level & wfa_defined_debug);

    debugResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_INFO_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)debugResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);


    return WFA_SUCCESS;
}


/*
 *   wfaStaGetBSSID():
 *     This function is to retrieve BSSID of a specific wireless I/F.
 */
int wfaStaGetBSSID(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    char string[64]; 
    char *str;
    FILE *tmpfd;
    dutCmdResponse_t *bssidResp = &gGenericResp;
    dutCommand_t *getBssid = (dutCommand_t *)caCmdBuf;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaGetBSSID ...\n");
    /* retrieve the BSSID */
    sprintf(gCmdStr, "wpa_cli -i%s %s status > /tmp/sigma_bssid.txt", getBssid->intf, ctrl_if);

    sret = system(gCmdStr);

    tmpfd = fopen("/tmp/sigma_bssid.txt", "r+");
    if(tmpfd == NULL)
    {
        bssidResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_GET_BSSID_RESP_TLV, 4, (BYTE *)bssidResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + 4;

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    for(;;)
    {
        if(fscanf(tmpfd, "%s", string) == EOF)
        {
            bssidResp->status = STATUS_COMPLETE;
            strcpy(bssidResp->cmdru.bssid, "00:00:00:00:00:00");
            break;
        }

        if(strncmp(string, "bssid", 5) == 0)
        {
            str = strtok(string, "=");
            str = strtok(NULL, "=");
            if(str != NULL)
            {
                strcpy(bssidResp->cmdru.bssid, str);
                bssidResp->status = STATUS_COMPLETE;
                break;
            }
        }
    }

    wfaEncodeTLV(WFA_STA_GET_BSSID_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)bssidResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);

    fclose(tmpfd);
    return WFA_SUCCESS;
}

/*
 * wfaStaSetIBSS()
 *    This is to set
 *    1. ssid
 *    2. channel
 *    3. encrypType - none or wep
 *    optional
 *    4. key1
 *    5. key2
 *    6. key3
 *    7. key4
 *    8. activeIndex - 1, 2, 3, or 4
 */
int wfaStaSetIBSS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetIBSS_t *setIBSS = (caStaSetIBSS_t *)caCmdBuf;
    dutCmdResponse_t *setIbssResp = &gGenericResp;
    int i;

    /*
     * disable the network first
     */ 
    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", setIBSS->intf, ctrl_if); 
    sret = system(gCmdStr);

    /*
     * set SSID
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", setIBSS->intf, ctrl_if, setIBSS->ssid); 
    sret = system(gCmdStr);

    /*
     * Set channel for IBSS
     */
    sprintf(gCmdStr, "iwconfig %s channel %i", setIBSS->intf, setIBSS->channel);
    sret = system(gCmdStr);

    /*
     * Tell the supplicant for IBSS mode (1)
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 mode 1", setIBSS->intf, ctrl_if);
    sret = system(gCmdStr);

    /*
     * set Key management to NONE (NO WPA) for plaintext or WEP
     */
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt NONE", setIBSS->intf, ctrl_if);
    sret = system(gCmdStr);

    if(setIBSS->encpType == 1)
    {
        for(i=0; i<4; i++)
        {
            if(strlen(setIBSS->keys[i]) ==5 || strlen(setIBSS->keys[i]) == 13)
            {
                sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 wep_key%i \"%s\"", 
                setIBSS->intf, ctrl_if, i, setIBSS->keys[i]);
                sret = system(gCmdStr);
            }
        } 

        i = setIBSS->activeKeyIdx;
        if(strlen(setIBSS->keys[i]) ==5 || strlen(setIBSS->keys[i]) == 13)
        {
            sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 wep_tx_keyidx %i", 
            setIBSS->intf, ctrl_if, setIBSS->activeKeyIdx);
            sret = system(gCmdStr);
        }
    }

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", setIBSS->intf, ctrl_if);
    sret = system(gCmdStr);

    setIbssResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_IBSS_RESP_TLV, 4, (BYTE *)setIbssResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 *  wfaSetMode():
 *  The function is to set the wireless interface with a given mode (possible
 *  adhoc)
 *  Input parameters:
 *    1. I/F
 *    2. ssid
 *    3. mode adhoc or managed
 *    4. encType
 *    5. channel
 *    6. key(s)
 *    7. active  key
 */
int wfaStaSetMode(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetMode_t *setmode = (caStaSetMode_t *)caCmdBuf;
    dutCmdResponse_t *SetModeResp = &gGenericResp;
    int i;

    /*
     * bring down the interface
     */
    sprintf(gCmdStr, "ifconfig %s down",setmode->intf);
    sret = system(gCmdStr);

    /*
     * distroy the interface
     */
    sprintf(gCmdStr, "wlanconfig %s destroy",setmode->intf);
    sret = system(gCmdStr);


    /*
     * re-create the interface with the given mode
     */
    if(setmode->mode == 1)
        sprintf(gCmdStr, "wlanconfig %s create wlandev wifi0 wlanmode adhoc",setmode->intf);
    else
        sprintf(gCmdStr, "wlanconfig %s create wlandev wifi0 wlanmode managed",setmode->intf);

    sret = system(gCmdStr);
    if(setmode->encpType == ENCRYPT_WEP)
    {
        int j = setmode->activeKeyIdx;
        for(i=0; i<4; i++)
        {
            if(setmode->keys[i][0] != '\0')
            {
                sprintf(gCmdStr, "iwconfig  %s key  s:%s",
                        setmode->intf, setmode->keys[i]);
                sret = system(gCmdStr);
            }
            /* set active key */
            if(setmode->keys[j][0] != '\0')
                sprintf(gCmdStr, "iwconfig  %s key  s:%s",
                        setmode->intf, setmode->keys[j]);
            sret = system(gCmdStr);
        }

    }
    /*
     * Set channel for IBSS
     */
    if(setmode->channel)
    {
        sprintf(gCmdStr, "iwconfig %s channel %i", setmode->intf, setmode->channel);
        sret = system(gCmdStr);
    }


    /*
     * set SSID
     */
    sprintf(gCmdStr, "iwconfig %s essid %s", setmode->intf, setmode->ssid);
    sret = system(gCmdStr);

    /*
     * bring up the interface
     */
    sprintf(gCmdStr, "ifconfig %s up",setmode->intf);
    sret = system(gCmdStr);

    SetModeResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_MODE_RESP_TLV, 4, (BYTE *)SetModeResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetPwrSave(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetPwrSave_t *setps = (caStaSetPwrSave_t *)caCmdBuf;
    dutCmdResponse_t *SetPSResp = &gGenericResp;

    sprintf(gCmdStr, "iwconfig %s power %s", setps->intf, setps->mode);
    sret = system(gCmdStr);

    if(strcasecmp("off", setps->mode)==0)
    {
        MTK_IWPRIV_CMD("%s set_power_mode 0", setps->intf);
    }
    else if (strcasecmp("on", setps->mode)==0)
    {
        MTK_IWPRIV_CMD("%s set_power_mode 1", setps->intf);
    }
    else
    {
        printf("wfaStaSetPwrSave: %s unknown power save mode: %s", setps->intf, setps->mode);
    }

    SetPSResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_PWRSAVE_RESP_TLV, 4, (BYTE *)SetPSResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaUpload(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaUpload_t *upload = &((dutCommand_t *)caCmdBuf)->cmdsu.upload;
    dutCmdResponse_t *upLoadResp = &gGenericResp;
    caStaUploadResp_t *upld = &upLoadResp->cmdru.uld;

    if(upload->type == WFA_UPLOAD_VHSO_RPT)
    {
        int rbytes;
        /*
         * if asked for the first packet, always to open the file
         */
        if(upload->next == 1)
        {
            if(e2efp != NULL)
            {
                fclose(e2efp);
                e2efp = NULL;
            }

            e2efp = fopen(e2eResults, "r");
        }

        if(e2efp == NULL)
        {
            upLoadResp->status = STATUS_ERROR;
            wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, 4, (BYTE *)upLoadResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + 4;
            return WFA_FAILURE;
        }

        rbytes = fread(upld->bytes, 1, 256, e2efp);

        if(rbytes < 256)
        {
            /*
             * this means no more bytes after this read
             */
            upld->seqnum = 0;
            fclose(e2efp);
            e2efp=NULL;
        }
        else
        {
            upld->seqnum = upload->next;
        }

        upld->nbytes = rbytes;

        upLoadResp->status = STATUS_COMPLETE;
        wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)upLoadResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
    }
    else
    {
        upLoadResp->status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_UPLOAD_RESP_TLV, 4, (BYTE *)upLoadResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + 4;
    }

    return WFA_SUCCESS;
}
/*
 * wfaStaSetWMM()
 *  TO be ported on a specific plaform for the DUT
 *  This is to set the WMM related parameters at the DUT.
 *  Currently the function is used for GROUPS WMM-AC and WMM general configuration for setting RTS Threshhold, Fragmentation threshold and wmm (ON/OFF)
 *  It is expected that this function will set all the WMM related parametrs for a particular GROUP .
 */
int wfaStaSetWMM(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
#ifdef WFA_WMM_AC
    caStaSetWMM_t *setwmm = (caStaSetWMM_t *)caCmdBuf;
    char *ifname = setwmm->intf;
    dutCmdResponse_t *setwmmResp = &gGenericResp;
    char cmdstr[1024];

    switch(setwmm->group)
    {
    case GROUP_WMMAC:
        if (setwmm->send_trig)
        {
            int Sockfd;
            struct sockaddr_in psToAddr;
            unsigned int TxMsg[512];

            Sockfd = wfaCreateUDPSock(setwmm->dipaddr, 12346);
            memset(&psToAddr, 0, sizeof(psToAddr));
            psToAddr.sin_family = AF_INET;
            psToAddr.sin_addr.s_addr = inet_addr(setwmm->dipaddr);
            psToAddr.sin_port = htons(12346);


            switch (setwmm->trig_ac)
            {
            case WMMAC_AC_VO:
                wfaTGSetPrio(Sockfd, 7);
                create_apts_msg(APTS_CK_VO, TxMsg, 0);
                printf("\r\nSending AC_VO trigger packet\n");
                break;

            case WMMAC_AC_VI:
                wfaTGSetPrio(Sockfd, 5);
                create_apts_msg(APTS_CK_VI, TxMsg, 0);
                printf("\r\nSending AC_VI trigger packet\n");
                break;

            case WMMAC_AC_BK:
                wfaTGSetPrio(Sockfd, 2);
                create_apts_msg(APTS_CK_BK, TxMsg, 0);
                printf("\r\nSending AC_BK trigger packet\n");
                break;

            default:
            case WMMAC_AC_BE:
                wfaTGSetPrio(Sockfd, 0);
                create_apts_msg(APTS_CK_BE, TxMsg, 0);
                printf("\r\nSending AC_BE trigger packet\n");
                break;
            }

            sendto(Sockfd, TxMsg, 256, 0, (struct sockaddr *)&psToAddr,
                   sizeof(struct sockaddr));
            close(Sockfd);
            usleep(1000000);
        }
        else if (setwmm->action == WMMAC_ADDTS)
        {
            printf("ADDTS AC PARAMS: dialog id: %d, TID: %d, "
                   "DIRECTION: %d, PSB: %d, UP: %d, INFOACK: %d BURST SIZE DEF: %d"
                   "Fixed %d, MSDU Size: %d, Max MSDU Size %d, "
                   "MIN SERVICE INTERVAL: %d, MAX SERVICE INTERVAL: %d, "
                   "INACTIVITY: %d, SUSPENSION %d, SERVICE START TIME: %d, "
                   "MIN DATARATE: %d, MEAN DATA RATE: %d, PEAK DATA RATE: %d, "
                   "BURSTSIZE or MSDU Aggreg: %d, DELAY BOUND: %d, PHYRATE: %d, SPLUSBW: %f, "
                   "MEDIUM TIME: %d, ACCESSCAT: %d\n",
                   setwmm->actions.addts.dialog_token,
                   setwmm->actions.addts.tspec.tsinfo.TID,
                   setwmm->actions.addts.tspec.tsinfo.direction,
                   setwmm->actions.addts.tspec.tsinfo.PSB,
                   setwmm->actions.addts.tspec.tsinfo.UP,
                   setwmm->actions.addts.tspec.tsinfo.infoAck,
                   setwmm->actions.addts.tspec.tsinfo.bstSzDef,
                   setwmm->actions.addts.tspec.Fixed,
                   setwmm->actions.addts.tspec.size,
                   setwmm->actions.addts.tspec.maxsize,
                   setwmm->actions.addts.tspec.min_srvc,
                   setwmm->actions.addts.tspec.max_srvc,
                   setwmm->actions.addts.tspec.inactivity,
                   setwmm->actions.addts.tspec.suspension,
                   setwmm->actions.addts.tspec.srvc_strt_tim,
                   setwmm->actions.addts.tspec.mindatarate,
                   setwmm->actions.addts.tspec.meandatarate,
                   setwmm->actions.addts.tspec.peakdatarate,
                   setwmm->actions.addts.tspec.burstsize,
                   setwmm->actions.addts.tspec.delaybound,
                   setwmm->actions.addts.tspec.PHYrate,
                   setwmm->actions.addts.tspec.sba,
                   setwmm->actions.addts.tspec.medium_time,
                   setwmm->actions.addts.accesscat);

            //tspec should be set here.
            wmmtspec_t *addts = &setwmm->actions.addts.tspec;

            if (setwmm->actions.addts.psb_flag == 1) {
                sprintf(cmdstr, "addts token %d,"
                    "tid %d,dir %d,psb %d,up %d,fixed %d,size %d,maxsize %d,maxsrvint %d,"
                    "minsrvint %d,inact %d,suspension %d,srvstarttime %d,minrate %d,meanrate %d,"
                    "peakrate %d,burst %d,delaybound %d,phyrate %d,sba %f,mediumtime %d",
                    setwmm->actions.addts.dialog_token, addts->tsinfo.TID, addts->tsinfo.direction,
                    addts->tsinfo.PSB, addts->tsinfo.UP, addts->Fixed, addts->size, addts->maxsize,
                    addts->max_srvc, addts->min_srvc, addts->inactivity, addts->suspension,
                    addts->srvc_strt_tim, addts->mindatarate, addts->meandatarate, addts->peakdatarate,
                    addts->burstsize, addts->delaybound, addts->PHYrate, addts->sba, addts->medium_time);
            } else {
                sprintf(cmdstr, "addts token %d,"
                    "tid %d,dir %d,up %d,fixed %d,size %d,maxsize %d,maxsrvint %d,"
                    "minsrvint %d,inact %d,suspension %d,srvstarttime %d,minrate %d,meanrate %d,"
                    "peakrate %d,burst %d,delaybound %d,phyrate %d,sba %f,mediumtime %d",
                    setwmm->actions.addts.dialog_token, addts->tsinfo.TID, addts->tsinfo.direction,
                    addts->tsinfo.UP, addts->Fixed, addts->size, addts->maxsize,
                    addts->max_srvc, addts->min_srvc, addts->inactivity, addts->suspension,
                    addts->srvc_strt_tim, addts->mindatarate, addts->meandatarate, addts->peakdatarate,
                    addts->burstsize, addts->delaybound, addts->PHYrate, addts->sba, addts->medium_time);
            }
            MTK_IWPRIV_CMD("%s driver \"%s\"", setwmm->intf, cmdstr);
        }
        else if (setwmm->action == WMMAC_DELTS)
        {
            // send del tspec
            sprintf(cmdstr, "delts tid %d", setwmm->actions.delts);
            MTK_IWPRIV_CMD("%s driver \"%s\"", setwmm->intf, cmdstr);
        }

        setwmmResp->status = STATUS_COMPLETE;
        break;

    case GROUP_WMMCONF:
        sprintf(gCmdStr, "iwconfig %s rts %d",
                ifname,setwmm->actions.config.rts_thr);

        sret = system(gCmdStr);
        sprintf(gCmdStr, "iwconfig %s frag %d",
                ifname,setwmm->actions.config.frag_thr);

        sret = system(gCmdStr);
        sprintf(gCmdStr, "iwpriv %s wmmcfg %d",
                ifname, setwmm->actions.config.wmm);

        sret = system(gCmdStr);
        setwmmResp->status = STATUS_COMPLETE;
        break;

    default:
        DPRINT_ERR(WFA_ERR, "The group %d is not supported\n",setwmm->group);
        setwmmResp->status = STATUS_ERROR;
        break;

    }

    wfaEncodeTLV(WFA_STA_SET_WMM_RESP_TLV, 4, (BYTE *)setwmmResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
#endif

    return WFA_SUCCESS;
}

int wfaStaSendNeigReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *sendNeigReqResp = &gGenericResp;

    /*
     *  run your device to send NEIGREQ
     */

    sendNeigReqResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SEND_NEIGREQ_RESP_TLV, 4, (BYTE *)sendNeigReqResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetEapFAST(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapFAST_t *setFAST= (caStaSetEapFAST_t *)caCmdBuf;
    char *ifname = setFAST->intf;
    dutCmdResponse_t *setEapFastResp = &gGenericResp;

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eapfast %s %s %s %s %s %s", ifname, setFAST->ssid, setFAST->username,
            setFAST->passwd, setFAST->pacFileName,
            setFAST->innerEAP);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", ifname, ctrl_if, setFAST->ssid);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 identity '\"%s\"'", ifname, ctrl_if, setFAST->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 password '\"%s\"'", ifname, ctrl_if, setFAST->passwd);
    sret = system(gCmdStr);

    if(strcasecmp(setFAST->keyMgmtType, "wpa2-sha256") == 0)
    {
    }
    else if(strcasecmp(setFAST->keyMgmtType, "wpa2-eap") == 0)
    {
    }
    else if(strcasecmp(setFAST->keyMgmtType, "wpa2-ft") == 0)
    {

    }
    else if(strcasecmp(setFAST->keyMgmtType, "wpa") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setFAST->keyMgmtType, "wpa2") == 0)
    {
        // take all and device to pick one which is supported.
    }
    else
    {
        // ??
    }
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 eap FAST", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 pac_file '\"%s/%s\"'", ifname, ctrl_if, CERTIFICATES_PATH,     setFAST->pacFileName);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 anonymous_identity '\"anonymous\"'", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 phase1 '\"fast_provisioning=1\"'", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 phase2 '\"auth=%s\"'", ifname, ctrl_if,setFAST->innerEAP);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);
#endif

    setEapFastResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPFAST_RESP_TLV, 4, (BYTE *)setEapFastResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSetEapAKA(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapAKA_t *setAKA= (caStaSetEapAKA_t *)caCmdBuf;
    char *ifname = setAKA->intf;
    dutCmdResponse_t *setEapAkaResp = &gGenericResp;

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eapaka %s %s %s %s", ifname, setAKA->ssid, setAKA->username, setAKA->passwd);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i %s %s disable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 ssid '\"%s\"'", ifname, ctrl_if, setAKA->ssid);
    sret = system(gCmdStr);

    if(strcasecmp(setAKA->keyMgmtType, "wpa2-sha256") == 0)
    {
    }
    else if(strcasecmp(setAKA->keyMgmtType, "wpa2-eap") == 0)
    {
    }
    else if(strcasecmp(setAKA->keyMgmtType, "wpa2-ft") == 0)
    {

    }
    else if(strcasecmp(setAKA->keyMgmtType, "wpa") == 0)
    {
        sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 key_mgmt WPA-EAP", ifname, ctrl_if);
    }
    else if(strcasecmp(setAKA->keyMgmtType, "wpa2") == 0)
    {
        // take all and device to pick one which is supported.
    }
    else
    {
        // ??
    }
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 proto WPA2", ifname, ctrl_if);
    sret = system(gCmdStr);
    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 proto CCMP", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 eap AKA", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 phase1 \"result_ind=1\"", ifname, ctrl_if);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 identity '\"%s\"'", ifname, ctrl_if, setAKA->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s set_network 0 password '\"%s\"'", ifname, ctrl_if, setAKA->passwd);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s enable_network 0", ifname, ctrl_if);
    sret = system(gCmdStr);
#endif

    setEapAkaResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPAKA_RESP_TLV, 4, (BYTE *)setEapAkaResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/* EAP-AKA' */
int wfaStaSetEapAKAPrime(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetEapAKAPrime_t *setAKAPrime = (caStaSetEapAKAPrime_t *)caCmdBuf;
    char *ifname = setAKAPrime->intf;
    dutCmdResponse_t *setEapAkaPrimeResp = &gGenericResp;

#ifdef WFA_NEW_CLI_FORMAT
    sprintf(gCmdStr, "wfa_set_eapakaprime %s %s %s %s", ifname, setAKAPrime->ssid, setAKAPrime->username, setAKAPrime->passwd);
    sret = system(gCmdStr);
#else

    sprintf(gCmdStr, "wpa_cli -i%s remove_network 0", ifname);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s add_network", ifname);
    sret = system(gCmdStr);

    if(strcasecmp(setAKAPrime->keyMgmtType, "wpa2-sha256") == 0)
    {
    }
    else if(strcasecmp(setAKAPrime->keyMgmtType, "wpa2-eap") == 0)
    {
    }
    else if(strcasecmp(setAKAPrime->keyMgmtType, "wpa2-ft") == 0)
    {
 
    }
    else if(strcasecmp(setAKAPrime->keyMgmtType, "wpa") == 0)
    {
       sprintf(gCmdStr, "wpa_cli -i%s set_network 0 key_mgmt WPA-EAP", ifname);
    }
    else if(strcasecmp(setAKAPrime->keyMgmtType, "wpa2") == 0)
    {
      // take all and device to pick one which is supported.
      sprintf(gCmdStr, "wpa_cli -i%s set_network 0 key_mgmt WPA-EAP", ifname);
    }
    else
    {
       // ??
       setEapAkaPrimeResp->status = STATUS_INVALID;
       strcpy(setEapAkaPrimeResp->cmdru.info, "invalid key management parameter");
       wfaEncodeTLV(WFA_STA_SET_EAPAKAPRIME_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)setEapAkaPrimeResp, respBuf);
       *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
       return WFA_FAILURE;
    }
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 pairwise CCMP", ifname);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 proto WPA2", ifname);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 eap AKA\\'", ifname);
    printf("eap command=%s\n", gCmdStr);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 phase1 '\"result_ind=1\"'", ifname);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 identity '\"%s\"'", ifname, setAKAPrime->username);
    printf("user name=%s\n", setAKAPrime->username);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 password '\"%s\"'", ifname, setAKAPrime->passwd);
    printf("password=%s\n", setAKAPrime->passwd);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s set_network 0 ssid '\"%s\"'", ifname, setAKAPrime->ssid);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i%s enable_network 0", ifname);
    sret = system(gCmdStr);
#endif

    setEapAkaPrimeResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_EAPAKAPRIME_RESP_TLV, 4, (BYTE *)setEapAkaPrimeResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}



int wfaStaSetSystime(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaSetSystime_t *systime = (caStaSetSystime_t *)caCmdBuf;
    dutCmdResponse_t *setSystimeResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaSetSystime ...\n");

    sprintf(gCmdStr, "date %d-%d-%d",systime->month,systime->date,systime->year);
    sret = system(gCmdStr);

    sprintf(gCmdStr, "time %d:%d:%d", systime->hours,systime->minutes,systime->seconds);
    sret = system(gCmdStr);

    setSystimeResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_SYSTIME_RESP_TLV, 4, (BYTE *)setSystimeResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

#ifdef WFA_STA_TB
int wfaStaPresetParams(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *PresetParamsResp = &gGenericResp;
    caStaPresetParameters_t *presetParams = (caStaPresetParameters_t *)caCmdBuf;
    BYTE presetDone = 1;
    int st = 0;
   char cmdStr[128];
   char string[256];
   FILE *tmpfd = NULL;
   long val;
   char *endptr;
   char nonPrefChanStr[32];

    DPRINT_INFO(WFA_OUT, "Inside wfaStaPresetParameters function ...\n");

    if(presetParams->program == PROG_TYPE_MBO)
    {
        if (presetParams->chans.chPrefNum)
        {
            sprintf(nonPrefChanStr, " %d:%d:%d:%d",
                presetParams->chans.chOpClass, presetParams->chans.chPrefNum, presetParams->chans.chPref, presetParams->chans.chReasonCode);
            strcat(gNonPrefChanStr, nonPrefChanStr);
            sprintf(gCmdStr, "wpa_cli -i %s SET non_pref_chan %s", presetParams->intf, gNonPrefChanStr);
            sret = system(gCmdStr);
        }

        if (presetParams->cellularDataCap)
        {
            sprintf(gCmdStr, "wpa_cli -i %s SET mbo_cell_capa %d", presetParams->intf, presetParams->cellularDataCap);
            sret = system(gCmdStr);
        }
    }

   if (presetParams->supplicant == eWpaSupplicant)
   {
#if 0
	st = access("/tmp/processid.txt", F_OK);
	if (st != -1)
	{
	    st = remove("/tmp/processid.txt");
	}
	
	sprintf(cmdStr, "/usr/local/sbin/findprocess.sh %s /tmp/processid.txt\n", "wpa_supplicant");
	st = system(cmdStr);
	
	tmpfd = fopen("/tmp/processid.txt", "r+");
	if (tmpfd == NULL)
	{
	    DPRINT_ERR(WFA_ERR, "process id file not exist\n");
	    return WFA_FAILURE;
	}
	
	for (;;)
	{
	    if (fgets(string, 256, tmpfd) == NULL)
	        break;

	    errno = 0;
	    val = strtol(string, &endptr, 10);
	    if (errno != 0 && val == 0)
	    {
		DPRINT_ERR(WFA_ERR, "strtol error\n");
		return WFA_FAILURE;
	    }
	
	    if (endptr == string)
	    {
		DPRINT_ERR(WFA_ERR, "No wpa_supplicant instance was found\n");
	    }

	    presetDone = 1;
	}
#else
	presetDone = 1;
#endif
   }

    if(presetParams->wmmFlag)
    {
        st = wfaExecuteCLI(gCmdStr);
        switch(st)
        {
        case 0:
            presetDone = 1;
            break;
        case 1:
            presetDone = 0;
            break;
        case 2:
            presetDone = 0;
            break;
        }
    }

    if(presetParams->modeFlag != 0)
    {
        switch(presetParams->wirelessMode)
        {
        default:
            printf("other mode does not need to support\n");
        }

        st = wfaExecuteCLI(gCmdStr);
        switch(st)
        {
        case 0:
            presetDone = 1;
            break;
        case 1:
            presetDone = 0;
        case 2:
            presetDone = 0;
            break;
        }
    }


    if(presetParams->psFlag)
    {
        printf("%s\n", gCmdStr);
        sret = system(gCmdStr);
    }
#ifdef MTK_WFD_SIGMA  
    if(presetParams->tdlsFlag)
    {
#if 0
        // enable / disable tdls based on tdls
        if (presetParams->tdls == eEnable)
        {
#if (MTK_ENABLE_TDLS)
            wfdTdlsEnable = 1;
		    tdls_peer_ip[0] = '0';
		    printf("\n tdls enable = %d\n", presetParams->tdls);
		    sprintf(cmdStr, "iwpriv %s set TdlsCapable=1", WFA_STAUT_IF);
		    system(cmdStr);
#endif
	    }
        else
        {      
            wfdTdlsEnable = 0;
            tdls_peer_ip[0] = '0';
            printf("\n Disable TDLS\n");
            sprintf(cmdStr, "iwpriv %s set TdlsCapable=0", WFA_STAUT_IF);
            system(cmdStr);
        }
#else
        // enable / disable tdls based on tdls
        if (presetParams->tdls == eEnable)
        {
            wfdTdlsEnable = 1;
            tdls_peer_ip[0] = '0';
            printf("\n tdls enable = %d, do tdls_start\n", presetParams->tdls);
            tdls_delay_start = 1;
            printf("\n tdls enable until IP get, set tdls_delay_start = 1\n");
            sprintf(cmdStr, "wpa_cli -i %s %s tdls_start", WFA_STAUT_IF, ctrl_if);
            system(cmdStr);
        }
        else
        {      
            wfdTdlsEnable = 0;
            tdls_peer_ip[0] = '0';
            printf("\n Disable TDLS, do tdls_stop\n");
            sprintf(cmdStr, "wpa_cli -i %s %s tdls_stop", WFA_STAUT_IF, ctrl_if);
            system(cmdStr);
        }
#endif
    }
#endif
    /************the followings are used for Voice Enterprise **************/
    if(presetParams->program == PROG_TYPE_VENT)
    {
        if(presetParams->ftoa == eEnable)
        {
            // enable Fast BSS Transition Over the Air
        }
        else
        {
	        // disable Fast BSS Transition Over the Air
        }

        if(presetParams->ftds == eEnable)
        {
            // enable Fast BSS Transition Over the DS
        }
        else
        {
            // disable Fast BSS Transition Over the DS
        }

        if(presetParams->activescan == eEnable)
        {
            // Enable Active Scan on STA
        }
        else
        {
            // disable Active Scan on STA
        }
    }

    /************the followings are used for Wi-Fi Display *************/
    if(presetParams->program == PROG_TYPE_WFD)
    {
        printf("%s - WFD prog.\n", __func__);
        if(presetParams->tdlsFlag)
        {
            // enable / disable tdls based on tdls
            if (presetParams->tdls == eEnable)
            {   
#if (MTK_ENABLE_TDLS)
                printf("\n tdls enable... \n");
                //sprintf(cmdStr, "iwpriv %s set TdlsCapable=1", WFA_STAUT_IF);
                wfdTdlsEnable = 1;
                sprintf(cmdStr, "wpa_cli -i %s %s tdls_start", WFA_STAUT_IF, ctrl_if);
                tdls_delay_start = 1;
                printf("\n tdls enable until IP get, set tdls_delay_start = 1\n");
                system(cmdStr);
#else
                printf("\n TDLS is disabled by default!\n");
#endif
            }
        }
        if(presetParams->wfdDevTypeFlag)
        {
#ifdef MTK_WFD_SIGMA        
            extern int wfaMtkWfdCmd_rtspSetWfdDevType(int wfdDevType);
            // set WFD device type to source/sink/dual based on wfdDevType 
            if (presetParams->wfdDevType == ePSink)
            {
                fprintf(stderr, "Setting wfdDevType to P-Sink\n");
                wfaMtkWfdCmd_rtspSetWfdDevType(ePSink);
            }
            else if (presetParams->wfdDevType == eSSink)
            {
                fprintf(stderr, "Setting wfdDevType to S-Sink\n");
                wfaMtkWfdCmd_rtspSetWfdDevType(eSSink);
            }
#endif
        }
        if(presetParams->wfdUibcGenFlag)
        {
            // enable / disable the feature
#ifdef MTK_WFD_SIGMA
            if (presetParams->wfdUibcGen == eEnable)
            {
                extern int wfaMtkWfdCmd_rtspUibcCapEnable(int cap_type);
                fprintf(stderr, "%s: UIBC Generic Eanbled\n", __FUNCTION__);
                wfaMtkWfdCmd_rtspUibcCapEnable(eUibcGen);
            }
#endif
        }
        if(presetParams->wfdUibcHidFlag)
        {
            // enable / disable feature
#ifdef MTK_WFD_SIGMA
            if (presetParams->wfdUibcHid == eEnable)
            {
                extern int wfaMtkWfdCmd_rtspUibcCapEnable(int cap_type);
                fprintf(stderr, "%s: UIBC HIDC Eanbled\n", __FUNCTION__);
                wfaMtkWfdCmd_rtspUibcCapEnable(eUibcHid);
            }
#endif
        }
        if(presetParams->wfdUiInputFlag)
        {
            // set the UI input as mentioned
        }
        if(presetParams->wfdHdcpFlag)
        {
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspEnableHDCP2X(int enable);
            // enable / disable feature
            if (presetParams->wfdHdcp == eEnable)
                wfaMtkWfdCmd_rtspEnableHDCP2X(1);
            else
                wfaMtkWfdCmd_rtspEnableHDCP2X(0);
#endif
        }
        if(presetParams->wfdFrameSkipFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdAvChangeFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdStandByFlag)
        {
            // enable / disable feature
#ifdef MTK_WFD_SIGMA
            if (presetParams->wfdStandBy == eEnable)
            {
                extern int wfaMtkWfdCmd_rtspEnableStandby(int enable);
                fprintf(stderr, "Set standby feature as enabled\n");
                wfaMtkWfdCmd_rtspEnableStandby(1);
            }
#endif /* MTK_WFD_SIGMA */
        }
        if(presetParams->wfdInVideoFlag)
        {
            // select the input vide as protecteed or non-protetcted or protected audio
            // or unprotected audio etc.
        }

        if(presetParams->wfdVideoFmatFlag)
        {
            // set the video format as requested

            //switch(presetParams->wfdVideoFmt )
            //{
            // case e640x480p60:
            //   ;
            // default:
            // set the mandatory
            // }
#ifdef MTK_WFD_SIGMA
            {    
                extern int wfaMtkWfdCmd_rtspSetVideoFormat(unsigned char *index_array, int array_size);
                fprintf(stderr, "%s: wfdVideoFmatFlag is set\n", __FUNCTION__);
                wfaMtkWfdCmd_rtspSetVideoFormat(presetParams->wfdVideoFmt, presetParams->wfdInputVideoFmats);
            }
#endif
        }
        if(presetParams->wfdAudioFmatFlag)
        {
            // set the Audio format as requested

            //switch(presetParams->wfdAudioFmt )
            //{
            // case eMandatoryAudioMode:
            //	;
            // case eDefaultAudioMode:
            //  ;

            // default:
            // set the mandatory
            // }
        }

        if(presetParams->wfdI2cFlag)
        {
            // enable / disable feature
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspEnableI2c(int enable);
            if (presetParams->wfdI2c == eEnable)
            {
                fprintf(stderr, "Enabling I2C...\n");
                wfaMtkWfdCmd_rtspEnableI2c(1);
            }
            else
            {
                fprintf(stderr, "Disabling I2C...\n");
                wfaMtkWfdCmd_rtspEnableI2c(0);
            }
#endif
        }
        if(presetParams->wfdVideoRecoveryFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdPrefDisplayFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdServiceDiscoveryFlag)
        {
#ifdef MTK_P2P_SIGMA
   	        BYTE bSerDisc;
	  
	        if (presetParams->wfdServiceDiscovery == eEnable)
	            bSerDisc = 1;
	        else
	  	        bSerDisc = 0;
            // enable / disable feature
	        printf("Service Discovery = %d.\n", bSerDisc);
	        sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=%d", presetParams->intf, bSerDisc);
	        system(cmdStr);
#endif
        }
        if(presetParams->wfd3dVideoFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdMultiTxStreamFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdTimeSyncFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdEDIDFlag)
        {
            // enable / disable feature
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspEnableEdid(int enable);
            if (presetParams->wfdEDID == eEnable)
            {
                fprintf(stderr, "Enabling Edid...\n");
                wfaMtkWfdCmd_rtspEnableEdid(1);
            }
            else
            {
                fprintf(stderr, "Disabling Edid...\n");
                wfaMtkWfdCmd_rtspEnableEdid(0);
            }
#endif /* MTK_WFD_SIGMA */
        }
        if(presetParams->wfdUIBCPrepareFlag)
        {
            // Provdes information to start valid WFD session to check UIBC operation.
        }
        if(presetParams->wfdCoupledCapFlag)
        {
            // enable / disable feature
        }
        if(presetParams->wfdOptionalFeatureFlag)
        {
            // disable all program specific optional features
#ifdef MTK_WFD_SIGMA
            fprintf(stderr, "Disable All Optional Features\n");
            extern int wfaMtkWfdCmd_sigmaDisableAll(void);
            wfaMtkWfdCmd_sigmaDisableAll();
#endif /* MTK_WFD_SIGMA */
        }
        if(presetParams->wfdSessionAvailFlag)
        {
#ifdef MTK_WFD_SIGMA
            BYTE bSessAvail;

            if (presetParams->wfdSessionAvail == 0)
                bSessAvail = 0; // Not Available
            else
                bSessAvail = 1; // Available
            // enable / disable session available bit
            sprintf(cmdStr, "iwpriv %s set WfdSessionAvail=%d", presetParams->intf, bSessAvail);
            fprintf(stderr, "Session Availability cmd = [%s]\n", cmdStr);
            system(cmdStr);
#endif
        }
        if(presetParams->wfdDeviceDiscoverabilityFlag)
        {
#ifdef MTK_P2P_SIGMA
            BYTE bDevDisc;
	   
            if (presetParams->wfdDeviceDiscoverability == eEnable)
                bDevDisc = 1;
            else
                bDevDisc = 0;
            // enable / disable feature
            printf("Device Discovery = %d.\n", bDevDisc);
            sprintf(cmdStr, "iwpriv %s set p2pDevDiscEnable=%d", presetParams->intf, bDevDisc);
            system(cmdStr);
#endif
        }
        if(presetParams->tdlsMode == eWeakSec)
        {
            printf("%s: TDLS mode set to WeakSecurity\n", __FUNCTION__);
			printf("@@@@@@@@@@@@@@@[what for?? do nothing]@@@@@@@@@@@@@@@@@\n");
        }
    }

   if(presetParams->program == PROG_TYPE_WFDS)
   {

	   if(presetParams->wfdsType == eAcceptPD)
	   {
	      // preset to accept PD request
		 if (presetParams->wfdsConnectionCapabilityFlag == 1) 
		 {
		 	// use  presetParams->wfdsConnectionCapability and set role accordingly
		 }

	   }
	   if(presetParams->wfdsType == eRejectPD)
	   {
	      // preset to Reject PD request
	   }
	   if(presetParams->wfdsType == eIgnorePD)
	   {
	      // preset to Ignore PD request
	   }
	   if(presetParams->wfdsType == eRejectSession)
	   {
	      // preset to reject Session request
	   }
	   
   }
   
   if (presetDone)
   {
      PresetParamsResp->status = STATUS_COMPLETE;
   }
   else
   {
      PresetParamsResp->status = STATUS_INVALID;
   }

    wfaEncodeTLV(WFA_STA_PRESET_PARAMETERS_RESP_TLV, 4, (BYTE *)PresetParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSet11n(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *v11nParamsResp = &gGenericResp;

    v11nParamsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, 4, (BYTE *)v11nParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}
int wfaStaSetWireless(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staWirelessResp = &gGenericResp;

    staWirelessResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_WIRELESS_RESP_TLV, 4, (BYTE *)staWirelessResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaStaSendADDBA(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSendADDBAResp = &gGenericResp;

    staSendADDBAResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_SEND_ADDBA_RESP_TLV, 4, (BYTE *)staSendADDBAResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}

int wfaStaSetRIFS(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSetRIFSResp = &gGenericResp;

    staSetRIFSResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_RIFS_TEST_RESP_TLV, 4, (BYTE *)staSetRIFSResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

int wfaStaSendCoExistMGMT(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staSendMGMTResp = &gGenericResp;

    staSendMGMTResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SEND_COEXIST_MGMT_RESP_TLV, 4, (BYTE *)staSendMGMTResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;

}

int wfaStaResetDefault(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caStaResetDefault_t *reset = (caStaResetDefault_t *)caCmdBuf;
    dutCmdResponse_t *ResetResp = &gGenericResp;

#ifdef MTK_P2P_SIGMA
    // need to make your own command available for this, here is only an example
    char cmdStr[256];
    char wfdDevName[128] = {0x00};
    char tmpmac[32]={0x00}, tmpip[32]={0x00}, tmpmask[32]={0x00};
    char *tmpptr=NULL;
    printf("\n Entry wfaStaResetDefault... \n");

    wfdConnPeerIsInitiator = 0;
    wfdReinvokePeerIsInitiator = 0;
    wfdConnIamAutoGo = 0;
    concurrencyEnable = 0;
    wlanIsFixIp = 0;
    wfdTdlsEnable = 0;
    p2pIsListen = 0;

    char tmp_if_name[IFNAMSIZ] = {0};	

#ifdef MTK_WFD_SIGMA
    extern int wfaMtkWfdCmd_rtspTeardown(void);
    extern int wfdMtkWfdSigmaStaResetDefault(void);

    if(wfaMtkWfdCmd_rtspTeardown() != 0)
    {
        DPRINT_ERR(WFA_ERR, "Fail to send RTSP Teardown\n");
    }
    DPRINT_ERR(WFA_ERR, "Send RTSP Teardown [DONE] \n");
    wfdMtkWfdSigmaStaResetDefault();


    extern int wfdMtkWfdQuitConnectCheck(void);
    wfdMtkWfdQuitConnectCheck();
#endif

    sprintf(cmdStr, "wpa_cli -i %s %s disconnect", WFA_STAUT_IF, ctrl_if);
    printf("  system: %s \n",cmdStr);
    system(cmdStr);
	
    wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 
    printf(" Interface : %s",tmp_if_name);
    sprintf(cmdStr, "wpa_cli -i %s %s disconnect", tmp_if_name, ctrl_if);
    printf("  system: %s \n",cmdStr);
    system(cmdStr);
    sprintf(cmdStr, "wpa_cli -i %s %s p2p_group_remove %s", tmp_if_name, ctrl_if, tmp_if_name);
    printf("  system: %s \n",cmdStr);
    system(cmdStr);

#ifdef MTK_BDP_SIGMA
    sprintf(cmdStr, "ifconfig %s 0.0.0.0", WFA_STAUT_IF_P2P);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
    sleep(5);
#endif

    // Disable Supplicant Per-Station-Key Feature
    sprintf(cmdStr, "wpa_cli -i %s %s p2p_set per_sta_psk 0", WFA_STAUT_IF_P2P, ctrl_if);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);

    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_devType 1", WFA_STAUT_IF, ctrl_if);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 1", WFA_STAUT_IF, ctrl_if);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_maxThroughput 300", WFA_STAUT_IF, ctrl_if);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
    printf("  ==== Enable WFD function ====\n");
    sprintf(cmdStr, "wpa_cli -i %s %s set wifi_display 1", WFA_STAUT_IF, ctrl_if);
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
#ifdef MTK_P2P_SIGMA
    printf("  ==== Kill DHCP server ====\n");
    sprintf(cmdStr, "mtk_dhcp_reset.sh");
    system(cmdStr);
    printf("  system: %s \n",cmdStr);
#endif
    sprintf(gCmdStr, "wpa_cli -i %s %s disconnect", WFA_STAUT_IF_P2P, ctrl_if);
    system(gCmdStr);
    printf("  system: %s \n",gCmdStr);

#else

    sprintf(gCmdStr, "wpa_cli -i %s %s remove_network all", WFA_STAUT_IF, ctrl_if);
    system(gCmdStr);
    printf("  system: %s \n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s add_network", WFA_STAUT_IF, ctrl_if);
    system(gCmdStr);
    printf("  system: %s \n",gCmdStr);

    sprintf(gCmdStr, "wpa_cli -i %s %s disconnect", WFA_STAUT_IF, ctrl_if);
    system(gCmdStr);
    printf("  system: %s \n",gCmdStr);

#endif /* MTK_WFD_SIGMA */

    if(strcasecmp(reset->prog, "MBO") == 0)
    {
        memset(gNonPrefChanStr, 0, sizeof(gNonPrefChanStr));
    }

    else if (strncmp(reset->prog, "PMF", 3) == 0) {
        sprintf(gCmdStr, "wpa_cli -i %s %s sta_autoconnect 0", WFA_STAUT_IF, ctrl_if);
        system(gCmdStr);
        printf("system: %s\n",gCmdStr);
    }
    ResetResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_RESET_DEFAULT_RESP_TLV, 4, (BYTE *)ResetResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

#else

int wfaStaTestBedCmd(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t *staCmdResp = &gGenericResp;

    staCmdResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_DISCONNECT_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
#endif

/*
 * This is used to send a frame or action frame
 */
int wfaStaDevSendFrame(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *cmd = (dutCommand_t *)caCmdBuf;
    /* uncomment it if needed */
    char *ifname = cmd->intf;
    dutCmdResponse_t *devSendResp = &gGenericResp;
    caStaDevSendFrame_t *sf = &cmd->cmdsu.sf;
    unsigned char strCmd[128];

    DPRINT_INFO(WFA_OUT, "Inside wfaStaDevSendFrame function ...\n");
    /* processing the frame */

    switch(sf->program)
    {
    case PROG_TYPE_PMF:
    {
        pmfFrame_t *pmf = &sf->frameType.pmf;
        switch(pmf->eFrameName)
        {
        case PMF_TYPE_DISASSOC:
        {
            /* use the protected to set what type of key to send */

        }
        break;
        case PMF_TYPE_DEAUTH:
        {

        }
        break;
        case PMF_TYPE_SAQUERY:
        {

        }
        break;
        case PMF_TYPE_AUTH:
        {
        }
        break;
        case PMF_TYPE_ASSOCREQ:
        {
        }
        break;
        case PMF_TYPE_REASSOCREQ:
        {
        }
        break;
        }
    }
    break;
    case PROG_TYPE_TDLS:
    {
        tdlsFrame_t *tdls = &sf->frameType.tdls;
        switch(tdls->eFrameName)
        {
        case TDLS_TYPE_DISCOVERY:
            /* use the peer mac address to send the frame */
            printf("@@@@ TDLS_DISCOVER: tdls->peer : %s\n", tdls->peer);
            sprintf(gCmdStr, "wpa_cli -i %s %s TDLS_DISCOVER %s", WFA_STAUT_IF, ctrl_if, tdls->peer);
            system(gCmdStr);
            break;
        case TDLS_TYPE_SETUP:
            if (tdls->timeout != 0)
            {
                sprintf(gCmdStr, "iwpriv %s set TdlsTPKLifeTime=%d", WFA_STAUT_IF, tdls->timeout);
                system(gCmdStr);    
            }
            if (tdls->status != 0)
            {
                sprintf(gCmdStr, "iwpriv %s set TdlsSetFailOnSetupConf=1", WFA_STAUT_IF, tdls->peer);
                system(gCmdStr);
            }
            else
            {
                sprintf(gCmdStr, "iwpriv %s set TdlsSetFailOnSetupConf=0", WFA_STAUT_IF, tdls->peer);
                system(gCmdStr);
            }
            printf("@@@@ TDLS_SETUP: tdls->peer : %s\n", tdls->peer);
            sprintf(gCmdStr, "wpa_cli -i %s %s TDLS_SETUP %s", WFA_STAUT_IF, ctrl_if, tdls->peer);
            system(gCmdStr);
            break;
        case TDLS_TYPE_TEARDOWN:
            printf("@@@@ TDLS_TEARDOWN: tdls->peer : %s\n", tdls->peer);
            sprintf(gCmdStr, "wpa_cli -i %s %s TDLS_TEARDOWN %s", WFA_STAUT_IF, ctrl_if, tdls->peer);
            system(gCmdStr);
            break;
        case TDLS_TYPE_CHANNELSWITCH:
            break;
        case TDLS_TYPE_NULLFRAME:
            break;
        }
    }
    break;
    case PROG_TYPE_VENT:
    {
        ventFrame_t *vent = &sf->frameType.vent;
        switch(vent->type)
        {
            case VENT_TYPE_NEIGREQ:
                printf("@@@@ VENT_SETUP: vent->ssid : %s\n", vent->ssid);
                sprintf(gCmdStr, "iwpriv wlan0 driver \"NEIGHBOR-REQUEST SSID=%s\"", vent->ssid);
                system(gCmdStr);
                break;
            case VENT_TYPE_TRANSMGMT:
                printf("@@@@ VENT_SETUP: BSS-TRANSITION-QUERY reason=6\n");
                sprintf(gCmdStr, "iwpriv wlan0 driver \"BSS-TRANSITION-QUERY reason=6\"");
                system(gCmdStr);
                break;
        }
    }
    break;
    case PROG_TYPE_WFD:
    {
        wfdFrame_t *wfd = &sf->frameType.wfd;
        switch(wfd->eframe)
        {
        case WFD_FRAME_PRBREQ:
        {
            /* send probe req */
        }
        break;

        case WFD_FRAME_PRBREQ_TDLS_REQ:
        {
            /* send tunneled tdls probe req  */
        }
        break;

        case WFD_FRAME_11V_TIMING_MSR_REQ:
        {
            /* send 11v timing mearurement request */
        }
        break;

        case WFD_FRAME_RTSP:
        {
            /* send WFD RTSP messages*/
            // fetch the type of RTSP message and send it.
            switch(wfd->eRtspMsgType)
            {
            case WFD_RTSP_PAUSE:
#ifdef MTK_WFD_SIGMA
                //send RTSP AUSE 
                {
                    extern int wfaMtkWfdCmd_rtspPause(void);
                    if (wfaMtkWfdCmd_rtspPause() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP PAUSE\n");
                }
#endif
                break;
            case WFD_RTSP_PLAY:
                //send RTSP PLAY
#ifdef MTK_WFD_SIGMA
                {
                    extern int wfaMtkWfdCmd_rtspPlay(void);
                    if (wfaMtkWfdCmd_rtspPlay() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP PLAY\n");
                }
#endif 
                break;
            case WFD_RTSP_TEARDOWN:
                //send RTSP TEARDOWN
#ifdef MTK_WFD_SIGMA
                {
                    extern int wfaMtkWfdCmd_rtspTeardown(void);
                    if (wfaMtkWfdCmd_rtspTeardown() != 0)
                        DPRINT_ERR(WFA_ERR, "Fail to send RTSP Teardown\n");
                }
#endif
                break;
            case WFD_RTSP_TRIG_PAUSE:
                //send RTSP TRIGGER PAUSE
                break;
            case WFD_RTSP_TRIG_PLAY:
                //send RTSP TRIGGER PLAY
                break;
            case WFD_RTSP_TRIG_TEARDOWN:
                //send RTSP TRIGGER TEARDOWN
                break;
            case WFD_RTSP_SET_PARAMETER:
                //send RTSP SET PARAMETER
                if (wfd->eSetParams == WFD_CAP_UIBC_KEYBOARD)
                {
                    //send RTSP SET PARAMETER message for UIBC keyboard
#ifdef MTK_WFD_SIGMA
                    extern int wfaMtkWfdCmd_rtspUibcCapUpdate(int type);
                    fprintf(stderr, "%s: UibcCapUpdate = Keyboard\n", __FUNCTION__);
                    if (wfaMtkWfdCmd_rtspUibcCapUpdate(WFD_CAP_UIBC_KEYBOARD) != 0)
                        fprintf(stderr, "Fail to update RTSP UIBC Cap to Keyboard\n");
#endif 
                }
                if (wfd->eSetParams == WFD_CAP_UIBC_MOUSE)
                {
                    //send RTSP SET PARAMETER message for UIBC Mouse
#ifdef MTK_WFD_SIGMA                        
                    extern int wfaMtkWfdCmd_rtspUibcCapUpdate(int type);
                    fprintf(stderr, "%s: UibcCapUpdate = Mouse\n", __FUNCTION__);
                    if (wfaMtkWfdCmd_rtspUibcCapUpdate(WFD_CAP_UIBC_MOUSE) != 0)
                        fprintf(stderr, "Fail to update RTSP UIBC Cap to Mouse\n");
#endif
                }
                else if (wfd->eSetParams == WFD_CAP_RE_NEGO)
                {
                    //send RTSP SET PARAMETER message Capability re-negotiation
                }
                else if (wfd->eSetParams == WFD_STANDBY)
                {
                    //send RTSP SET PARAMETER message for standby
#ifdef MTK_WFD_SIGMA
                    extern int wfaMtkWfdCmd_rtspEnterStandby(void);
                    if (wfaMtkWfdCmd_rtspEnterStandby() != 0)
                    {
                        fprintf(stderr, "%s:%d enterStandy mode failed!\n", __FUNCTION__, __LINE__);
                    }
                    else
                        fprintf(stderr, "%s:%d enterStandy mode success!\n", __FUNCTION__, __LINE__);
#endif
                }
                else if (wfd->eSetParams == WFD_UIBC_SETTINGS_ENABLE)
                {
                    //send RTSP SET PARAMETER message for UIBC settings enable
                }
                else if (wfd->eSetParams == WFD_UIBC_SETTINGS_DISABLE)
                {
                    //send RTSP SET PARAMETER message for UIBC settings disable
                }
                else if (wfd->eSetParams == WFD_ROUTE_AUDIO)
                {
                    //send RTSP SET PARAMETER message for route audio
                }
                else if (wfd->eSetParams == WFD_3D_VIDEOPARAM)
                {
                    //send RTSP SET PARAMETER message for 3D video parameters
                }
                else if (wfd->eSetParams == WFD_2D_VIDEOPARAM)
                {
                    //send RTSP SET PARAMETER message for 2D video parameters
                }
                break;
            }
        }
        break;
#ifdef MTK_WFD_SIGMA
        case WFD_FRAME_SERVDISC_REQ:
            sprintf(strCmd, "iwpriv %s set p2pSendSerDiscInit=%s", WFA_STAUT_IF_P2P, wfd->da);
            system(strCmd);
            break;
#endif
        }
    }
    break;
    /* not need to support HS2 release 1, due to very short time period  */
    case PROG_TYPE_HS2_R2:
    {
        /* type of frames */
        hs2Frame_t *hs2 = &sf->frameType.hs2_r2;
        switch(hs2->eframe)
        {
        case HS2_FRAME_ANQPQuery:
        {

        }
        break;
        case HS2_FRAME_DLSRequest:
        {

        }
        break;
        case HS2_FRAME_GARPReq:
        {

        }
        break;
        case HS2_FRAME_GARPRes:
        {
        }
        break;
        case HS2_FRAME_NeighAdv:
        {
        }
        case HS2_FRAME_ARPProbe:
        {
        }
        case HS2_FRAME_ARPAnnounce:
        {

        }
        break;
        case HS2_FRAME_NeighSolicitReq:
        {

        }
        break;
        case HS2_FRAME_ARPReply:
        {

        }
        break;
        }

        }/*  PROG_TYPE_HS2-R2  */
    case PROG_TYPE_GEN:
    {
        /* General frames */
    }
    case PROG_TYPE_MBO:
    {
        mboFrame_t *mbo = &sf->frameType.mbo;
        switch(mbo->eframe)
        {
            case MBO_FRAME_BTMQuery:
            {
                sprintf(gCmdStr, "wpa_cli -i %s WNM_BSS_QUERY %d", ifname, mbo->eBTMQueryReasonCode);
                sret = system(gCmdStr);
            }
            break;
        }
    }
    break;

    }
    devSendResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_DEV_SEND_FRAME_RESP_TLV, 4, (BYTE *)devSendResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * This is used to set a temporary MAC address of an interface
 */
int wfaStaSetMacAddr(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    // Uncomment it if needed
    //dutCommand_t *cmd = (dutCommand_t *)caCmdBuf;
    // char *ifname = cmd->intf;
    dutCmdResponse_t *staCmdResp = &gGenericResp;
    // Uncomment it if needed
    //char *macaddr = &cmd->cmdsu.macaddr[0];

    wfaEncodeTLV(WFA_STA_SET_MAC_ADDRESS_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}


int wfaStaDisconnect(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

    dutCommand_t *disc = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *staDiscResp = &gGenericResp;

    DPRINT_INFO(WFA_OUT, "Entering wfaStaDisconnect ...\n");

    // stop the supplicant
 	sprintf(gCmdStr, "wpa_cli -i %s %s disconnect", disc->intf, ctrl_if);
	sret = system(gCmdStr);

    staDiscResp->status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_STA_DISCONNECT_RESP_TLV, 4, (BYTE *)staDiscResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/* Execute CLI, read the status from Environment variable */
int wfaExecuteCLI(char *CLI)
{
    char *retstr;

    sret = system(CLI);

    retstr = getenv("WFA_CLI_STATUS");
    printf("cli status %s\n", retstr);
    if (retstr)
        return atoi(retstr);
    else
        return 0;
}

/* Supporting Functions */

void wfaSendPing(tgPingStart_t *staPing, float *interval, int streamid)
{
    int totalpkts, tos=-1;
    char cmdStr[2048];
    //char *addr = staPing->dipaddr;
#ifdef WFA_PC_CONSOLE
    char addr[40];
    char bflag[] = "-b";
    char *tmpstr;
    int inum=0;
#else
    char bflag[] = "  ";
#endif

    totalpkts = (int)(staPing->duration * staPing->frameRate);

#ifdef WFA_PC_CONSOLE

    printf("\nwfa_cs.c wfaSendPing CS : The Stream ID is %d\n",streamid);
    strcpy(addr,staPing->dipaddr);
	printf("\nCS :the addr is %s \n",addr);
    printf("\nCS :Inside the WFA_PC_CONSLE BLOCK");
    printf("\nCS :the addr is %s ",addr);
    if (staPing->iptype == 2)
    {
        memset(bflag, 0, strlen(bflag));
    }
    else
    {
        tmpstr = strtok(addr, ".");
        inum = atoi(tmpstr);

        printf("interval %f\n", *interval);

        if(inum >= 224 && inum <= 239) // multicast
        {
        }
        else // if not MC, check if it is BC address
        {
            printf("\nCS :Inside the BC address BLOCK");
            printf("\nCS :the inum %d",inum);
            strtok(NULL, ".");
            //strtok(NULL, ".");
            tmpstr = strtok(NULL, ".");
            printf("tmpstr %s\n", tmpstr);
            inum = atoi(tmpstr);
            printf("\nCS : The string is %s",tmpstr);
            if(inum != 255)
                memset(bflag, 0, strlen(bflag));
        }
    }
#endif
    if ( staPing->dscp >= 0)
    {
        tos= convertDscpToTos(staPing->dscp);
        if (tos < 0)
            printf("\nwfaSendPing invalid tos converted, dscp=%d",  staPing->dscp);
    }
    printf("\nwfa_cs.c wfaSendPing : The Stream ID=%d IPtype=%i\n",streamid, staPing->iptype);
    printf("IPtype : %i  tos=%d\n",staPing->iptype, tos);

    if (staPing->iptype == 2)
    {
        if ( tos>0)
            sprintf(cmdStr, "echo streamid=%i > /tmp/spout_%d.txt;wfaping6.sh %s -i %f -c %i -Q %d -s %i -q %s>> /tmp/spout_%d.txt 2>/dev/null",
                    streamid,streamid,bflag, *interval, totalpkts, tos,  staPing->frameSize, staPing->dipaddr, streamid);
        else
            sprintf(cmdStr, "echo streamid=%i > /tmp/spout_%d.txt;wfaping6.sh %s -i %f -c %i -s %i -q %s>> /tmp/spout_%d.txt 2>/dev/null",
                    streamid,streamid,bflag, *interval, totalpkts, staPing->frameSize, staPing->dipaddr, streamid);
        sret = system(cmdStr);
        printf("\nCS : The command string is [%s]\n",cmdStr);
    }
    else
    {
#ifdef MTK_P2P_SIGMA
        if (p2p_priv_p->concurrencyOn == 1)
            sprintf(cmdStr, "echo streamid=%i > /tmp/spout_%d.txt;wfaping.sh %s -I %s -i %f -c %i -s %i -S 111616 -q %s>> /tmp/spout_%d.txt 2>/dev/null",
                    streamid,streamid,bflag, p2p_priv_p->wlan_if_name, *interval, totalpkts, staPing->frameSize,staPing->dipaddr, streamid);
        else
#endif
        if (tos > 0)
            sprintf(cmdStr, "echo streamid=%i > /tmp/spout_%d.txt;wfaping.sh %s -i %f -c %i  -Q %d -s %i -q %s>> /tmp/spout_%d.txt 2>/dev/null",
                    streamid,streamid,bflag, *interval, totalpkts, tos, staPing->frameSize, staPing->dipaddr, streamid);
        else
            sprintf(cmdStr, "echo streamid=%i > /tmp/spout_%d.txt;./scripts/wfaping.sh %s -i %f -c %i -s %i %s>> /tmp/spout_%d.txt 2>/dev/null",
                    streamid,streamid,bflag, *interval, totalpkts, staPing->frameSize, staPing->dipaddr, streamid);
        sret = system(cmdStr);
        printf("\nCS : The command string is [%s]\n",cmdStr);
    }
    sprintf(cmdStr, "./scripts/updatepid.sh /tmp/spout_%d.txt",streamid);
    sret = system(cmdStr);
    printf("\nCS : The command string is [%s]\n",cmdStr);

}

int wfaStopPing(dutCmdResponse_t *stpResp, int streamid)
{
    char strout[256];
    FILE *tmpfile = NULL;
    char cmdStr[128];
    printf("\nwfa_cs.c wfaStopPing:: stream id=%d\n", streamid);
    sprintf(cmdStr, "./scripts/getpid.sh /tmp/spout_%d.txt /tmp/pid.txt",streamid);
    sret = system(cmdStr);

    printf("\nCS : The command string is [%s]\n",cmdStr);

    sret = system("./scripts/stoping.sh /tmp/pid.txt ; sleep 2");

    sprintf(cmdStr, "./scripts/getpstats.sh /tmp/spout_%d.txt",streamid);
    sret = system(cmdStr);

    printf("\nCS : The command string is [%s]\n",cmdStr);

    tmpfile = fopen("/tmp/stpsta.txt", "r+");

    if(tmpfile == NULL)
    {
        return WFA_FAILURE;
    }

    if(fscanf(tmpfile, "%s", strout) != EOF)
    {
        if(*strout == '\0')
        {
            stpResp->cmdru.pingStp.sendCnt = 0;
        }

        else
            stpResp->cmdru.pingStp.sendCnt = atoi(strout);
    }

    printf("\nwfaStopPing after scan sent count %i\n", stpResp->cmdru.pingStp.sendCnt);


    if(fscanf(tmpfile, "%s", strout) != EOF)
    {
        if(*strout == '\0')
        {
            stpResp->cmdru.pingStp.repliedCnt = 0;
        }
        else
            stpResp->cmdru.pingStp.repliedCnt = atoi(strout);
    }
    printf("wfaStopPing after scan replied count %i\n", stpResp->cmdru.pingStp.repliedCnt);

    fclose(tmpfile);

    //FOR CONCURRENT
	char buf[WPS_BUF_LENGTH];
	size_t buflen;
	char cmd[WPS_CMD_LENGTH];
	buflen = sizeof(buf) - 1;

	printf("%s: WORKAROUND FOR CON \n", __FUNCTION__);
#ifdef MTK_P2P_SIGMA
	if ((p2p_priv_p->concurrencyOn == 1) &&
	   (p2p_priv_p->role == DEVICE))
	{
			snprintf(cmd, sizeof(cmd), "P2P_LISTEN");
			wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
	}
#endif

    return WFA_SUCCESS;
}

/*
 * wfaStaGetP2pDevAddress():
 */
int wfaStaGetP2pDevAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* dutCommand_t *getInfo = (dutCommand_t *)caCmdBuf; */
#ifdef MTK_P2P_SIGMA
    dutCommand_t *getInfo = (dutCommand_t *)caCmdBuf;
	unsigned char addr[6] = {0}, strout[128] = {0};
	FILE *tmpfile = NULL;

    printf("\n Entry wfaStaGetP2pDevAddress... ");

    // Fetch the device ID and store into infoResp->cmdru.devid
    //strcpy(infoResp->cmdru.devid, str);
    //strcpy(&infoResp.cmdru.devid[0], "ABCDEFGH");

    if (checksum_ok(p2p_priv_p))
    {
        printf("%s: p2p_priv_p->if_name=%s, p2p_if_name=%s, role=[%d]\n",__FUNCTION__, 
                p2p_priv_p->if_name, p2p_priv_p->p2p_if_name, p2p_priv_p->role);

        if(!wpa_get_mac(p2p_priv_p,&addr))
        {
            printf(MAC_FMT,MAC_ARG(addr));
            sprintf(&infoResp.cmdru.devid[0],MAC_FMT, MAC_ARG(addr) );	
        }
        else
        {
            printf("AA:BB:CC:DD:EE:FF");	
            strcpy(&infoResp.cmdru.devid[0], "AA:BB:CC:DD:EE:FF");		
        }
    }

#endif
   
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_GET_DEV_ADDRESS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}



/*
 * wfaStaSetP2p():
 */
int wfaStaSetP2p(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
#ifdef MTK_P2P_SIGMA
    caStaSetP2p_t *getStaSetP2p = (caStaSetP2p_t *)caCmdBuf;
    char cmdStr[128];
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    buflen = sizeof(buf) - 1;	

    printf("\n Entry wfaStaSetP2p... ");

    if (!checksum_ok(p2p_priv_p))
    {
        infoResp.status = STATUS_ERROR;
        goto done;
    }

    if (!strncmp(getStaSetP2p->intf, WFA_STAUT_IF_P2P, WFA_STAUT_IF_NAME_P2P_LEN))
    {
        printf("  Cmd.inf = %s.\n", getStaSetP2p->intf);
        if (getStaSetP2p->listen_chn_flag)
        {
            char listen_ch = getStaSetP2p->listen_chn_flag;
            printf("  Listen Channel = %d.\n", getStaSetP2p->listen_chn);
            snprintf(cmd, sizeof(cmd), "P2P_SET listen_channel %d",getStaSetP2p->listen_chn);			
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }
        if (getStaSetP2p->p2p_mode_flag)
        {   
            char scan = 1;
            printf("  Mode = %s.\n", getStaSetP2p->p2p_mode);
            if (!strcasecmp(getStaSetP2p->p2p_mode, "Idle"))
            {
                snprintf(cmd, sizeof(cmd), "P2P_SET disabled 0");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
            }
            else if (!strcasecmp(getStaSetP2p->p2p_mode, "discover"))
            {
                snprintf(cmd, sizeof(cmd), "P2P_SET disabled 0");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                snprintf(cmd, sizeof(cmd), "P2P_FLUSH");
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                snprintf(cmd, sizeof(cmd), "P2P_FIND");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                if ((getStaSetP2p->discover_type_flag) && 
                    ((getStaSetP2p->discoverType == 3) || 
                            ((getStaSetP2p->discoverType == 1) && wfdTdlsEnable))
                   )
                {
                    sprintf(gCmdStr, "wpa_cli -i %s %s TDLS_SCAN %s", WFA_STAUT_IF, ctrl_if, "FF:FF:FF:FF:FF:FF");
                    system(gCmdStr);
                }
            }
            else if (!strcasecmp(getStaSetP2p->p2p_mode, "listen"))
            {
                snprintf(cmd, sizeof(cmd), "P2P_SET disabled 0");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                snprintf(cmd, sizeof(cmd), "P2P_FLUSH");
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                snprintf(cmd, sizeof(cmd), "P2P_LISTEN");			
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
            }
        }
        if (getStaSetP2p->presistent_flag)
        {		
            printf("  Presistent = %d.\n", getStaSetP2p->presistent);
            p2p_priv_p->presistent_oper = getStaSetP2p->presistent;
            //YF: persistent_reconnect means auto response inviteReq without user
            snprintf(cmd, sizeof(cmd), "SET persistent_reconnect %d", p2p_priv_p->presistent_oper);
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }
        if (getStaSetP2p->intra_bss_flag)
            printf("  IntraBSS = %d.\n", getStaSetP2p->intra_bss);
        if (getStaSetP2p->noa_duration_flag)
            printf("  NoA Duration= %d.\n", getStaSetP2p->noa_duration);
        if (getStaSetP2p->noa_interval_flag)
            printf("  NoA Interval = %d.\n", getStaSetP2p->noa_interval);
        if (getStaSetP2p->noa_count_flag)
            printf("  NoA Count = %d.\n", getStaSetP2p->noa_count);
        if (getStaSetP2p->concurrency_flag)
        {
            printf("  Concurrency = %d.\n", getStaSetP2p->concurrency);
            p2p_priv_p->concurrencyOn = 1;
            // to prevent the infra side lost connection
            snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }
        if (getStaSetP2p->p2p_invitation_flag)
            printf("  Invitation = %d.\n", getStaSetP2p->p2p_invitation);
        if (getStaSetP2p->bcn_int_flag)
            printf("  Beacon Interval = %d.\n", getStaSetP2p->bcn_int);
        if (getStaSetP2p->ext_listen_time_int_flag)
            printf("  ExtListenTimeIntetval = %d.\n", getStaSetP2p->ext_listen_time_int);
        if (getStaSetP2p->ext_listen_time_period_flag)
            printf("  ExtListenTimePeriod = %d.\n", getStaSetP2p->ext_listen_time_period);
        if (getStaSetP2p->discoverability_flag)
        {
            printf("  Discoverability = %d.\n", getStaSetP2p->discoverability);
        }
        if (getStaSetP2p->service_discovry_flag)
        {
            printf("Service Discovery = %d.\n", getStaSetP2p->service_discovery);
            sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=%d", getStaSetP2p->intf, getStaSetP2p->discoverability);
            system(cmdStr);
        }
        if (getStaSetP2p->crossconnection_flag)
            printf("  CrossConnection = %d.\n", getStaSetP2p->crossconnection);
        if (getStaSetP2p->p2pmanaged_flag)
            printf("  Managed = %d.\n", getStaSetP2p->p2pmanaged);
        if (getStaSetP2p->go_apsd_flag)
        {
            printf("  GO APSD = %d.\n", getStaSetP2p->go_apsd);
            snprintf(cmd, sizeof(cmd), "P2P_SET go_apsd 1");
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }

        infoResp.status = STATUS_COMPLETE;
    }
    else
    {
        infoResp.status = STATUS_ERROR;
    }

done: 
#endif

    wfaEncodeTLV(WFA_STA_P2P_SETP2P_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}
/*
 * wfaStaP2pConnect():
 */
int wfaStaP2pConnect(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

#ifdef MTK_P2P_SIGMA
    caStaP2pConnect_t *getStaP2pConnect = (caStaP2pConnect_t *)caCmdBuf;
    unsigned char dev_addr[WFA_P2P_DEVID_LEN];
    unsigned char strCmd[256];
    unsigned char bfound_peer = 0;
    int polling_cnt = 0;
    int finding_cnt = 0;	   
    int conn_cnt = 0;
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    char tmp_if_name[ IFNAMSIZ ];	
    buflen = sizeof(buf) - 1;
#endif

    printf("\n Entry wfaStaP2pConnect... ");

    // Implement the function and does not return anything.

#ifdef MTK_P2P_SIGMA    
    printf("  P2P Connect ::  Intf[%s].    GroupId[%s].    DevId[%s].\n", getStaP2pConnect->intf, getStaP2pConnect->grpid, getStaP2pConnect->devId);

    if (!checksum_ok(p2p_priv_p))
    {
        goto fail;
    }

    strcpy(&dev_addr, getStaP2pConnect->devId);

poll:
    //polling for peers
    do {
        sprintf(strCmd, "sleep 1");
        system(strCmd);
        snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        printf("%s===> %s\n", __FUNCTION__, buf);
        if(strncmp(buf, "FAIL", 4) == 0)
            printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, polling_cnt);
        else
        {
            bfound_peer = 1;
            printf("Peer discovered %s\n",dev_addr);
            break;
        }
        polling_cnt++;
    } while(polling_cnt <= 20 );

    if ((!bfound_peer) && finding_cnt < 1) //not found peers, try one more find
    {
        snprintf(cmd, sizeof(cmd), "P2P_FIND");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        polling_cnt =0;

        sprintf(strCmd, "sleep 2");
        system(strCmd);     
        goto poll;
    }

    if (bfound_peer)
    {

        /*
           try to connect and wait for event :
           P2P_CONNECT 02:0c:43:35:a2:78 pbc join
           P2P_CONNECT 02:0c:43:35:a2:78 12345670 keypad join          
           P2P_CONNECT 02:0c:43:35:a2:78 12345670 display join

           case 1: GO
           P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-L5" freq=2462 passphrase="EnIEEE6B" go_dev_addr=02:1f:e2:c5:3d:26

           case 2: GC
           P2P-GROUP-STARTED p2p0 client ssid="DIRECT-ia" freq=0 psk=2ff59eff2217c760f1616cc26a2bb5775dedc50d4e146c4603bde83b9d218d5a go_dev_addr=02:1f:e2:c5:3d:26'
         */      

        if (p2p_priv_p->confMethod == PBC)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc join", dev_addr);
        }
        else if (p2p_priv_p->confMethod == KEY_PAD)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad join", dev_addr, p2p_priv_p->keypad);  
        }
        else if (p2p_priv_p->confMethod == DIPLAY_PIN)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display join", dev_addr);   
        }
        else
        {
            DPRINT_ERR(WFA_ERR,"p2p_priv_p->confMethod=%d\n",p2p_priv_p->confMethod);   
            goto fail;
        }

        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);
            conn_cnt++;
            if (conn_cnt >= 60)
            {
                printf("wfaStaP2pConnect...  fail timeout!\n");         
                goto fail;
            }

            printf("wfaStaP2pConnect... conn_cnt=%d\n",conn_cnt);
        } while(p2p_priv_p->bNegoDone == 0);

        printf("wfaStaP2pConnect...  role=%d\n", p2p_priv_p->role);
        memset(tmp_if_name, '\0', IFNAMSIZ);
        wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 
        if (p2p_priv_p->role == GO)
        {

            /* Run dhcp server and set IP to p2p interface*/
            printf("%s :  setting IP %s for %s\n", __FUNCTION__, DHCPD_DEF_STATIC_IP_ADDR, tmp_if_name);
            sprintf(strCmd, "ifconfig %s %s",tmp_if_name, DHCPD_DEF_STATIC_IP_ADDR);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);
            printf("%s :  enable dhcp server\n", __FUNCTION__);
            snprintf(strCmd, sizeof(strCmd),
                "mtk_dhcp_server.sh %s", tmp_if_name);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);
            infoResp.status = STATUS_COMPLETE;
            strcpy(infoResp.cmdru.grpFormInfo.result, "GO");
            strcpy(infoResp.cmdru.grpFormInfo.grpId, p2p_priv_p->group_id);
            goto ok;
        }
        else if (p2p_priv_p->role == GC)
        {
            /* Run dhcp client*/
	    sprintf(strCmd, "mtk_dhcp_client.sh %s",tmp_if_name);
            system(strCmd);
            sprintf(strCmd, "sleep 5");
            system(strCmd);
            infoResp.status = STATUS_COMPLETE;
            strcpy(infoResp.cmdru.grpFormInfo.result, "CLIENT");
            strcpy(infoResp.cmdru.grpFormInfo.grpId, p2p_priv_p->group_id);
            goto ok;
        }
    }

fail:
    infoResp.status = STATUS_ERROR;
    strcpy(infoResp.cmdru.grpFormInfo.result,"FALSE");
    strcpy(infoResp.cmdru.grpFormInfo.grpId, " ");

ok:
    wfaEncodeTLV(WFA_STA_P2P_CONNECT_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
#endif  
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

#ifdef MTK_WFD_SIGMA
static void* wfaStaWaitingWfdConnection_Nego(void * argv)
{
    unsigned char  bNegotiate = 0;
    unsigned char group_id[WFA_P2P_GRP_ID_LEN] = {0};
    unsigned short rtsp_port = DEFAULT_WFD_RTSP_PORT;
    char session_id[32] = "";
    int conn_cnt =0 ;
    FILE * fp;
    char peer_ip[24] = {0};
    unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char strCmd[256] = {0};
    char if_name[IFNAMSIZ] = {0};
    char sbuf[256] = {0};
    unsigned char tmp_ping_result[8] = {0};
    char cmdStr[128];
    
    wfdInit0Argv_t argv_in;

    printf("[SIGMA]  %s Enter ....  \n",__func__);


    memset(&argv_in, 0 ,sizeof(wfdInit0Argv_t));
    memmove(&argv_in,(wfdInit0Argv_t *)argv, sizeof(wfdInit0Argv_t));
    
    printf("[SIGMA]  dev_addr   : %s\n",argv_in.dev_addr);
    printf("[SIGMA]  freq       : %d \n",argv_in.freq);
    printf("[SIGMA]  intent     : %d \n",argv_in.intent );

    if(argv_in.dev_addr[0] == 0 || argv_in.dev_addr[0] == '\0')
    {
        printf("[SIGMA] Invalid  dev_addr   \n");
        return NULL;
    }

    memcpy(dev_addr,argv_in.dev_addr,strlen(argv_in.dev_addr));

    char buf[WPS_BUF_LENGTH] = {0};
    size_t buflen = 0;
    char cmd[WPS_CMD_LENGTH] = {0};
    unsigned char bfound_peer = 0;
    unsigned char session_avail = 0;
    unsigned char dev_info = 0;

    buflen = sizeof(buf) - 1;
    do {
        sprintf(strCmd, "sleep 1");
        system(strCmd);
        snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        if(strncmp(buf, "FAIL", 4) == 0)
            printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, conn_cnt);
        else
        {
            bfound_peer = 1;
            printf("Peer discovered %s\n",dev_addr);
            break;
        }
        conn_cnt++;
    } while(conn_cnt <= WFD_SLEEP_TIME_BEFORE_CONN_ATTEMP );

    if(bfound_peer)
    {
        char tmp_rtspport[128] = {0};
        char tmp_string[128] = {0};
        char tmp_dev_info[128] = {0};                                           

        //mtk94097 Need Driver to get port from WFD_IE
        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
        system(strCmd);                     
        printf("system: %s\n",strCmd);

        FILE *tmpfd;
        tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
            rtsp_port = DEFAULT_WFD_RTSP_PORT;
        }
        else
        {
            fgets(tmp_string, sizeof(tmp_string), tmpfd);
            //sample code: 00 0006 0111 ^022a^ 0064
            memcpy(tmp_dev_info,tmp_string+6,4);
            memcpy(tmp_rtspport,tmp_string+10,4);
            printf("tmp_rtspport: %s  tmp_dev_info: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_dev_info,tmp_string);
            rtsp_port = strtol(tmp_rtspport, NULL , 16);                                                        
            dev_info = strtol(tmp_dev_info, NULL , 16);
            session_avail = (dev_info & WFD_DEV_INFO_BITMASK_SESS_AVAIL) > 1;
            fclose(tmpfd);                          
        }
    }
    else
    {
        printf(" [SIGMA] Device %s lost\n",dev_addr);
        return NULL;
    }

    //refer to wfaStaP2pStartGrpFormation()

    snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");            
    wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

    int persistent_flag = 0;                
    FILE *tmpfd;
    char tmp_pers_string[8] = {0};

    if (bfound_peer && session_avail==1) {
        if (p2p_priv_p->confMethod == PBC) {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc go_intent=%d freq=%d ", dev_addr, argv_in.intent , argv_in.freq);
        } else if (p2p_priv_p->confMethod == KEY_PAD) {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad go_intent=%d freq=%d", dev_addr, p2p_priv_p->keypad , argv_in.intent , argv_in.freq);
        } else if (p2p_priv_p->confMethod == DIPLAY_PIN) {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display go_intent=%d freq=%d", dev_addr ,argv_in.intent , argv_in.freq );
        } else {
            printf("[CFG80211] p2p_priv_p->confMethod=%d \n",p2p_priv_p->confMethod);
            return NULL;
        }

        if (p2p_priv_p->presistent_oper == 1) {
            strcat(cmd, " persistent");
            printf("[%s] with persistent!\n", __func__);
        } else {
            printf("[%s] without persistent!\n", __func__);
        }
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
    }

#ifdef MTK_WFD_SIGMA           
    if(session_avail==1)
    {
        wfdMtkWfdCheckP2pConnResult(&bNegotiate, dev_addr, sizeof(dev_addr));
    }
    else
    {
        bNegotiate = WFD_SESSION_UNAVALIBLE;
    }
#endif /* MTK_WFD_SIGMA */


    if (bNegotiate == P2P_DEVICE)
    {
        printf("I'm P2P Device!\n");
        return NULL;
    }
    else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
    {
        /* session not available */
        fprintf(stderr, "------------------------\n");
        fprintf(stderr, " WFD Connection Failed (Session Not Available)\n");
        fprintf(stderr, "------------------------\n");
        return NULL;
    }
    else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
    {
        char Rule[10] = {0};

        if (bNegotiate == P2P_GO)
            sprintf(&Rule, "%s", "GO");
        else
            sprintf(&Rule, "%s", "CLIENT");
        printf("I'm P2P %s!\n", Rule);

        if(p2p_priv_p->group_id[0] != '\0')
        {
            printf("[SIGMA]: group_id: %s\n",p2p_priv_p->group_id);
            strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
        }

        char tmp_rtspport[128] = {0};
        char tmp_string[128] = {0};

        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep intended_addr | busybox cut -d'=' -f2 > /tmp/sigma_if_addr_wfd.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
        system(strCmd);                     
        printf("system: %s\n",strCmd);

        FILE *tmpfd;
        tmpfd = NULL;
        tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("[SIGMA] %s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
            memcpy(if_addr, dev_addr, sizeof(dev_addr));
            printf("[SIGMA] device mac: %s  \n",if_addr);
        }
        else
        {
            fgets(if_addr, sizeof(if_addr), tmpfd);
            printf("[SIGMA] interface mac: %s  \n",if_addr);
            fclose(tmpfd);
        }

#ifdef MTK_WFD_SIGMA
        {
            char peer_ip[24] = "";
            int result = 0;
            char port_str[16] = "";
            int rtsp_retry = 0;

            extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
            if (rtsp_port == 0)
            {
                fprintf(stderr, "[SIGMA] ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                rtsp_port = DEFAULT_WFD_RTSP_PORT;
            }

            sprintf(port_str, "%d", rtsp_port);
            if (bNegotiate == P2P_GO)
            {
                fprintf(stderr, "[SIGMA] Starting DHCP server...\n");
                if (wfaMtkWfdStartDhcpServer(if_addr, peer_ip, sizeof(peer_ip)) != 0)
                {
                    fprintf(stderr, "Fail to get DHCP Client IP address.\n");
                    result = -1;
                }
            }
            else if (bNegotiate == P2P_CLIENT)
            {
                fprintf(stderr, "[SIGMA] Starting DHCP client...\n");
                if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
                {
                    fprintf(stderr, "Fail to get DHCP Server IP address.\n");
                    result = -1;
                }
            }

            //Add workaround solution for Marvell TestBed Getting IP TOOOOO late
            printf("[SIGMA] workaround solution for Marvell TestBed Getting IP TOOOOO late\n");
            conn_cnt = 0;
            while(conn_cnt < 60 )
            {
                sprintf(strCmd, "busybox ping %s -c 4|grep transmitted|busybox cut -d',' -f2|busybox cut -d' ' -f2 >/tmp/sigma_ping_result_wfd.txt",peer_ip);
                system(strCmd);                     
                printf("system: %s\n",strCmd);

                //check ping result
                tmpfd = NULL;
                tmpfd = fopen("/tmp/sigma_ping_result_wfd.txt", "r+");
                if(tmpfd == NULL)
                {
                    printf("[SIGMA] %s - fail to open /tmp/sigma_ping_result_wfd.txt\n", __func__);
                    return NULL;
                }
                else
                {
                    fgets(tmp_ping_result, sizeof(tmp_ping_result), tmpfd);
                    if(tmp_ping_result[0] == 0 || tmp_ping_result[0] == '0')
                    {
                        printf("[SIGMA]conn_cnt: %d  \n",conn_cnt);
                        conn_cnt++;
                        fclose(tmpfd);
                        sleep(1);                        
                        continue;
                    }
                    else
                    {
                        printf("[SIGMA]tmp_ping_result: %s  \n",tmp_ping_result);
                        fclose(tmpfd);
                        break;
                    }
                }
            }

            if (result == 0)
            {
                /* starting rtsp */
                while (rtsp_retry < MAX_RTSP_RETRY)
                {
                    sleep(10);
                    fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                    if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                    {
                        /* successful */
                        fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                        result = 0;
                        break;
                    }
                    else
                    {
                        /* failed */
                        fprintf(stderr, "RTSP negotiation is failed\n");
                        result = -1;
                    }
                    rtsp_retry ++;
                    if (rtsp_retry < MAX_RTSP_RETRY)
                        fprintf(stderr, "Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                }
            }


            if (result != 0)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, "WFD Connection Failed!!!\n");
                fprintf(stderr, "------------------------\n");
            }
            else
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " WFD Connection Success\n");
                fprintf(stderr, "------------------------\n");
                if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)                               
                {
                    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
                    system(cmdStr);
                }
            }
        }
#else
        fprintf(stderr, "[SIGMA] Just return without Doing anything ...  \n");
        return;
#endif 
    }
    return NULL;
}


static void* wfaStaWaitingWfdConnection_AutoGO(void * argv)
{
    unsigned char  bNegotiate = 0;
    unsigned char group_id[WFA_P2P_GRP_ID_LEN] = {0};
    unsigned short rtsp_port = DEFAULT_WFD_RTSP_PORT;
    char session_id[32] = "";
    int loop_cnt =0 ;
    FILE * fp;
    char peer_ip[24] = {0};
    unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char strCmd[256] = {0};
    char if_name[IFNAMSIZ] = {0};
    char sbuf[256] = {0};
    unsigned char tmp_ping_result[8] = {0};
    char cmdStr[256];

    fprintf(stderr, "[SIGMA] Enter %s, tdls=%d\n", __FUNCTION__, wfdTdlsEnable);

    if (wfdTdlsEnable == 0)
    {
        fprintf(stderr, "Checking WFD-P2P connections...\n");

        char Rule[10] = {0};
        int ret = -1;

        memset(sbuf, 0, 256);

        sprintf(&Rule, "%s", "GO");
        printf("[SIGMA] - I'm P2P %s!\n", Rule);

        while(loop_cnt < 30)
        {
            printf("[SIGMA] Checking for client Mac(%d)..\n",loop_cnt);
            wpa_get_if_by_role(p2p_priv_p, &if_name); 
            //printf("[SIGMA] Interface : %s\n",if_name);
            sprintf(strCmd,"wpa_cli -i%s %s all_sta > /tmp/sigma_wfd_autoGO_polling.txt;sync;sync",if_name, ctrl_if);
            system(strCmd);
            printf("[SIGMA]system : %s\n" , strCmd);

            fp = fopen("/tmp/sigma_wfd_autoGO_polling.txt", "r+");
            if(!fp)
                continue;
            if(fgets(sbuf,256,fp) == NULL)
            {
                fclose(fp);
                sleep(6);                       
                //printf("[SIGMA] Polling count : %d\n",loop_cnt);
                loop_cnt++;
                continue;
            }
            else
            {
                if(sbuf[0] == '\0')
                {
                    printf("[SIGMA] Invalid Interface MAC(%d)\n",loop_cnt);
                    loop_cnt++;
                }
                else
                {
                    //printf("[SIGMA] sbuf: %s\n",sbuf);
                    if(sbuf[strlen(sbuf)-1] != '\0')
                        sbuf[strlen(sbuf)-1] = '\0';
                    memcpy(if_addr,sbuf,strlen(sbuf));
                    printf("[SIGMA] Interface Mac: %s(%d)\n",if_addr,strlen(if_addr));

                    if(fp)
                        fclose(fp);

                    sprintf(strCmd,"wpa_cli -i%s %s all_sta | grep p2p_device_addr | busybox cut -d'=' -f2 > /tmp/sigma_wfd_autoGO_peerDevMac.txt",if_name, ctrl_if);
                    system(strCmd);
                    printf("[SIGMA]system : %s\n" , strCmd);

                    fp = fopen("/tmp/sigma_wfd_autoGO_peerDevMac.txt", "r+");
                    if(!fp)
                        continue;
                    else
                    {
                        fgets(dev_addr,WFA_P2P_DEVID_LEN,fp);
                        if(dev_addr[0] == '\0')
                        {
                            printf("[SIGMA] INVALID device MAC \n");
                            loop_cnt++;
                            continue;
                        }
                    }
                    if(fp)
                        fclose(fp);

                    printf("[SIGMA] Device Mac: %s\n",dev_addr);
                    break;
                }

                if(fp)
                    fclose(fp);
            }
        }               

        loop_cnt =0;
        printf("[SIGMA] Checking for client IP..\n");
        while(loop_cnt < 60)
        {
            loop_cnt ++;
            sleep(1);

            fp = fopen(DHCPD_LEASE_FILE_PATH, "r");
            printf("[SIGMA] fopen(%s)..\n",DHCPD_LEASE_FILE_PATH);
            if (!fp)
                continue;

#ifdef MTK_ANDROID_SIGMA
            if ((ret = wfaMtkWfdGetClientIp_Android(fp, if_addr, peer_ip, sizeof(peer_ip))) == 0)
                break;
#else                   
            if ((ret = wfaMtkWfdGetClientIp(fp, NULL, peer_ip, sizeof(peer_ip))) == 0)
                break;
#endif

            if (fp)
                fclose(fp);
        }

        if (loop_cnt >= 60 && ret != 0)
            printf("[SIGMA] %s: Time out waiting for getting client's IP address\n", __FUNCTION__);
        else
            printf("[SIGMA] Checking client Ip complete (ret = %d)..\n", ret);

        if(p2p_priv_p->group_id[0] != '\0')
            strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
        char tmp_rtspport[128] = {0};
        char tmp_string[128] = {0};

        //mtk94097 Need Driver to get port from WFD_IE
        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
        system(strCmd);                     
        printf("system: %s\n",strCmd);

        /*      
        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep intended_addr | busybox cut -d'=' -f2 > /tmp/sigma_if_addr_wfd.txt", WFA_STAUT_IF_P2P, dev_addr, ctrl_if);
        system(strCmd);                     
        printf("system: %s\n",strCmd);
         */

        FILE *tmpfd;
        tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
            rtsp_port = DEFAULT_WFD_RTSP_PORT;
        }
        else
        {
            fgets(tmp_string, sizeof(tmp_string), tmpfd);
            //sample code: 00 0006 0111 ^022a^ 0064
            memcpy(tmp_rtspport,tmp_string+10,4);
            printf("tmp_rtspport: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_string);
            rtsp_port = strtol(tmp_rtspport, NULL , 16);

            fclose(tmpfd);
        }

        /*
           tmpfd = NULL;
           tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
           if(tmpfd == NULL)
           {
           printf("[mtk_sigma]%s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
           memcpy(if_addr, dev_addr, sizeof(dev_addr));
           printf("[mtk_sigma]device mac: %s  \n",if_addr);
           }
           else
           {
           fgets(if_addr, sizeof(if_addr), tmpfd);
           printf("[mtk_sigma]interface mac: %s  \n",if_addr);
           fclose(tmpfd);
           }   
         */


        //sprintf(group_id, "%s %s", pdrv_ralink_cfg->addr, pdrv_ralink_cfg->ssid);
        printf("GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);

        //Add workaround solution for Marvell TestBed Getting IP TOOOOO late
        printf("[mtk_sigma]workaround solution for Marvell TestBed Getting IP TOOOOO late\n");
        loop_cnt= 0;
        while(loop_cnt < 60 )
        {
            sprintf(strCmd, "busybox ping %s -c 4 | grep transmitted | busybox cut -d',' -f2 | busybox cut -d' ' -f2 > /tmp/sigma_ping_result_wfd.txt",peer_ip);
            system(strCmd);                     
            printf("system: %s\n",strCmd);

            //check ping result

            tmpfd = NULL;
            tmpfd = fopen("/tmp/sigma_ping_result_wfd.txt", "r+");
            if(tmpfd == NULL)
            {
                printf("[mtk_sigma]%s - fail to open /tmp/sigma_ping_before_wfd.txt\n", __func__);
                return NULL;
            }
            else
            {
                fgets(tmp_ping_result, sizeof(tmp_ping_result), tmpfd);
                if(tmp_ping_result[0] == 0 || tmp_ping_result[0] == '0' || tmp_ping_result[0] == '\0')
                {
                    printf("[mtk_sigma]conn_cnt: %d  \n",loop_cnt);
                    loop_cnt++;
                    fclose(tmpfd);
                    sleep(1);                       
                    continue;
                }
                else
                {
                    printf("[mtk_sigma]tmp_ping_result: %s  \n",tmp_ping_result);
                    fclose(tmpfd);
                    break;
                }
            }
        }


        {
            int result = 0;
            char port_str[16] = "";
            int rtsp_retry = 0;
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
#endif 

            if (rtsp_port == 0)
            {
                fprintf(stderr, "ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                rtsp_port = DEFAULT_WFD_RTSP_PORT;
            }
            sprintf(port_str, "%d", rtsp_port);

            {
                /* starting rtsp */
                while (rtsp_retry < MAX_RTSP_RETRY)
                {                        
                    sleep(10);
                    fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                    if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                    {
                        /* successful */
                        fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                        result = 0;
                        break;
                    }
                    else
                    {
                        /* failed */
                        fprintf(stderr, "RTSP negotiation is failed\n");
                        result = -1;
                    }
                    rtsp_retry ++;
                    if (rtsp_retry < MAX_RTSP_RETRY)
                        fprintf(stderr, "Retrying RTSP (retry=%d)\n", rtsp_retry);
                }
            }

            if (result != 0)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, "WFD Connection Failed!!!\n");
                fprintf(stderr, "------------------------\n");
            }
            else
            {
                //                  sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=0", staStartWfdConn->intf);
                //                  system(cmdStr);
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " WFD Connection Success\n");
                fprintf(stderr, "------------------------\n");
                if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)
                    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
                system(cmdStr);
            }
        }
    }
    else /* wfd connect by tdls */
    {
        char peer_ip[24] = "";
        int result = 0;
        char port_str[16] = "";
        int rtsp_retry = 0;
        char peer_ip_addr[5] = {0};
        FILE *tmpfile = NULL;
        char tmpBuf[16];
        char tmp_rtspport[8];

#ifdef MTK_WFD_SIGMA
        extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
#endif 

        fprintf(stderr, "Checking WFD P2P&TDLS connections...\n");
        wfdMtkWfdCheckP2pAndTdlsConnResult(&bNegotiate, NULL);

        fprintf(stderr, "bNegotiate =%d\n", bNegotiate);
        if (bNegotiate == TDLS_LINKED)
        {
            sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_peer_ip= | busybox cut -f2- -d= > /tmp/tdlsPeerIP", WFA_STAUT_IF, ctrl_if);
            system(cmdStr);
            tmpfile = fopen("/tmp/tdlsPeerIP", "r+");
            if(tmpfile == NULL)
            {
                /* hope not happen */
                DPRINT_ERR(WFA_ERR, "file open failed\n");
                //return WFA_FAILURE;
                return NULL;
            }
            fscanf(tmpfile, "%s", tdls_peer_ip);

            sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_dev_info= | busybox cut -f2- -d= > /tmp/tdlsDevInfo", WFA_STAUT_IF, ctrl_if);
            system(cmdStr);
            tmpfile = fopen("/tmp/tdlsDevInfo", "r+");
            if(tmpfile == NULL)
            {
                /* hope not happen */
                DPRINT_ERR(WFA_ERR, "file open failed\n");
                //return WFA_FAILURE;
                return NULL;
            }
            fscanf(tmpfile, "%s", tmpBuf);
            memcpy(tmp_rtspport, tmpBuf + 4, 4);
            tmp_rtspport[4] = '\0';
            printf("tmp_rtspport: %s  <tmpBuf: %s>\n", tmp_rtspport, tmpBuf);
            rtsp_port = strtol(tmp_rtspport, NULL , 16);

#ifdef MTK_WFD_SIGMA
            /* starting rtsp */
            while (rtsp_retry < MAX_RTSP_RETRY)
            {
                sleep(15);
                fprintf(stderr, "Starting RTSP to [%s:%s]...\n", tdls_peer_ip, port_str);
                if (wfaMtkWfdCmd_rtspStart(tdls_peer_ip, port_str, session_id) == 0)
                {
                    /* successful */
                    fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                    result = 0;
                    break;
                }
                else
                {
                    /* failed */
                    fprintf(stderr, "RTSP negotiation is failed\n");
                    result = -1;
                }
                rtsp_retry ++;
                if (rtsp_retry < MAX_RTSP_RETRY)
                    fprintf(stderr, "Retrying RTSP (retry=%d)...\n", rtsp_retry);
            }
#endif 

            if (result != 0)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, "WFD Connection Failed!!!\n");
                fprintf(stderr, "------------------------\n");
            }
            else
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " WFD Connection Success\n");
                fprintf(stderr, "------------------------\n");
            }
        }

        else if (bNegotiate == P2P_DEVICE)
        {
            printf("P2P/TDLS Connection Failed!\n");
        }
        else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
        {
            /* session not available */
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, " WFD Connection Failed (Session Not Available)\n");
            fprintf(stderr, "------------------------\n");
        }
        else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
        {
            char Rule[10] = {0};

            if (bNegotiate == P2P_GO)
                sprintf(&Rule, "%s", "GO");
            else
                sprintf(&Rule, "%s", "CLIENT");
            printf("I'm P2P %s!\n", Rule);
            //wfa_driver_ralink_get_oid(WFA_STAUT_IF_P2P, OID_802_11_P2P_PEER_GROUP_ID, &group_id, sizeof(group_id));
            //wfa_driver_ralink_get_oid(WFA_STAUT_IF_P2P, OID_802_11_WFD_PEER_RTSP_PORT, &rtsp_port, sizeof(rtsp_port));
            //sprintf(group_id, "%s %s", pdrv_ralink_cfg->addr, pdrv_ralink_cfg->ssid);
            fprintf(stderr, "GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);

#ifdef MTK_WFD_SIGMA
            /* starting dhcp server and getting peer's ip */
            {
                char peer_ip[24] = "";
                int result = 0;
                char port_str[16] = "";
                int rtsp_retry = 0;
#ifdef MTK_WFD_SIGMA
                extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
#endif
                if (rtsp_port == 0)
                {
                    fprintf(stderr, "ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                    rtsp_port = DEFAULT_WFD_RTSP_PORT;
                }
                sprintf(port_str, "%d", rtsp_port);
                if (bNegotiate == P2P_GO)
                {
                    fprintf(stderr, "Starting DHCP server...\n");
#ifdef MTK_WFD_SIGMA
                    if (wfaMtkWfdStartDhcpServer(NULL, peer_ip, sizeof(peer_ip)) != 0)
                    {
                        fprintf(stderr, "Fail to get DHCP Client IP address.\n");
                        result = -1;
                    }
#endif 
                }
                else if (bNegotiate == P2P_CLIENT)
                {
                    fprintf(stderr, "Starting DHCP client...\n");
#ifdef MTK_WFD_SIGMA
                    if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
                    {
                        fprintf(stderr, "Fail to get DHCP Server IP address.\n");
                        result = -1;
                    }
#endif 
                }

#ifdef MTK_WFD_SIGMA
                if (result == 0)
                {
                    /* starting rtsp */
                    while (rtsp_retry < MAX_RTSP_RETRY)
                    {
                        sleep(10);
                        fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                        if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                        {
                            /* successful */
                            fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                            result = 0;
                            break;
                        }
                        else
                        {
                            /* failed */
                            fprintf(stderr, "RTSP negotiation is failed\n");
                            result = -1;
                        }
                        rtsp_retry ++;
                        if (rtsp_retry < MAX_RTSP_RETRY)
                            fprintf(stderr, "Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                    }
                }
#endif
                if (result != 0)
                {
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, "WFD Connection Failed!!!\n");
                    fprintf(stderr, "------------------------\n");
                }
                else
                {
                    //                      sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=0", staStartWfdConn->intf);
                    //                      system(cmdStr);
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, " WFD Connection Success\n");
                    fprintf(stderr, "------------------------\n");
                    if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)
                    {
                        sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);                             
                        system(cmdStr);
                    }
                }
            }
#endif /* MTK_WFD_SIGMA */
        }
    }

    return NULL;
}
#endif /* MTK_WFD_SIGMA */


/*
 * wfaStaStartAutoGo():
 */
int wfaStaStartAutoGo(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaStartAutoGo_t *getStaStartAutoGo = (caStaStartAutoGo_t *)caCmdBuf;

    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    int op_freq = 0;
    int polling_cnt = 0;
    unsigned char strCmd[256]; 
    char tmp_if_name[ IFNAMSIZ ];	

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaStartAutoGo... \n");
#ifdef MTK_P2P_SIGMA

    // Fetch the group ID and store into 	infoResp->cmdru.grpid 
    printf("  AutoGO.intf = %s.    AutoGO.opCh = %d\n", getStaStartAutoGo->intf, getStaStartAutoGo->oper_chn);
    //chia-chi
    printf("[chia-chi]:  p2p_priv_p->group_id=%s\n", p2p_priv_p->group_id);

    if (checksum_ok(p2p_priv_p))
    {	
        if (getStaStartAutoGo->ssid_flag)
        {
            printf("  AutoGO.ssid = %s\n", getStaStartAutoGo->ssid);

            buflen = sizeof(buf) - 1;	
            snprintf(cmd, sizeof(cmd), "P2P_SET ssid_postfix %s",getStaStartAutoGo->ssid);			
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }

        snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        op_freq = wpa_channel_to_freq(getStaStartAutoGo->oper_chn);
        snprintf(cmd, sizeof(cmd), "P2P_GROUP_ADD freq=%d",op_freq);
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        do {
            sprintf(cmd, "sleep 1");
            system(cmd);

            polling_cnt++;
            if (polling_cnt >= 60)
            {
                infoResp.status = STATUS_ERROR;
                goto done;
            }
        } while( p2p_priv_p->bNegoDone ==0 );

        if ( p2p_priv_p->role ==GO)
        {
            memset(tmp_if_name, '\0', IFNAMSIZ);
            wpa_get_if_by_role(p2p_priv_p, &tmp_if_name);
            /* Run dhcp server and set IP to p2p interface*/
            // to do: run dhcp server, set IP address
            printf("%s :  setting IP %s for %s\n", __FUNCTION__, DHCPD_DEF_STATIC_IP_ADDR, tmp_if_name);
            sprintf(strCmd, "ifconfig %s %s",tmp_if_name, DHCPD_DEF_STATIC_IP_ADDR);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);

            printf("%s :  enable dhcp server\n", __FUNCTION__);
            sprintf(strCmd, "mtk_dhcp_server.sh %s", tmp_if_name);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);

#ifdef MTK_WFD_SIGMA
            int res = -1;
            pthread_t t_pid;
            unsigned char bAutoGO = 1;

            printf("[SIGMA]  before  pthread_create() \n");
            res = pthread_create(&t_pid, NULL, (void *)&wfaStaWaitingWfdConnection_AutoGO, NULL);
            if (res < 0)
            {
                printf("[SIGMA]pthread_create(wfaStaCheckWfdConnection) error\n");
                return -1;
            }
            printf("[SIGMA]  after  pthread_create() \n");

#endif	
        }
        else
        {
            //printf("%s: %s\n", __FUNCTION__, infoResp.cmdru.grpid);
            strcpy(&infoResp.cmdru.grpid[0], "ABCDEFGH");
        }

        infoResp.status = STATUS_COMPLETE;
        printf("%s :  p2p_priv_p->group_id=%s\n", __func__,
                p2p_priv_p->group_id);
        strcpy(&infoResp.cmdru.grpid[0], p2p_priv_p->group_id);
    }
    else
    {
        infoResp.status = STATUS_ERROR;
    }

done:
#endif
    wfaEncodeTLV(WFA_STA_P2P_START_AUTO_GO_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaP2pStartGrpFormation(): 
 */
int wfaStaP2pStartGrpFormation(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaP2pStartGrpForm_t *getStaP2pStartGrpForm = (caStaP2pStartGrpForm_t *)caCmdBuf;
    unsigned char dev_addr[WFA_P2P_DEVID_LEN];
    unsigned char strCmd[256];   
    unsigned char bfound_peer=0, intent;
    int op_chn = 0;
    int op_freq = 0;   
    int polling_cnt = 0;
    int conn_cnt = 0;
    char tmp_if_name[ IFNAMSIZ ];	

    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    buflen = sizeof(buf) - 1;

#ifdef MTK_P2P_SIGMA

    printf("\n Entry wfaStaP2pStartGrpFormation... \n");
    printf("  Start Group Formation ::  DevAddr[%s].    GoIntent[%d].    Init Nego[%d]. IsPersistent[%d]\n", 
        getStaP2pStartGrpForm->devId, getStaP2pStartGrpForm->intent_val, 
        getStaP2pStartGrpForm->init_go_neg, p2p_priv_p->presistent_oper);

    if (getStaP2pStartGrpForm->oper_chn_flag)
    {
        op_chn = getStaP2pStartGrpForm->oper_chn;
        op_freq = wpa_channel_to_freq(op_chn);
        printf("                                     OpChannel[%d]., Freq=%d\n", getStaP2pStartGrpForm->oper_chn,op_freq);
        if (!op_freq)
        {
            goto fail;
        }

    }
    if (getStaP2pStartGrpForm->ssid_flag)
        printf("                                     SSID[%s].\n", getStaP2pStartGrpForm->ssid);




    if (!checksum_ok(p2p_priv_p))
    {
        goto fail;
    }


    intent = getStaP2pStartGrpForm->intent_val;

    if (getStaP2pStartGrpForm->init_go_neg)
    {
        int bFind = 0;
        strcpy(dev_addr, getStaP2pStartGrpForm->devId);

        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);


            memset(buf, '\0', sizeof(buf));
            memset(cmd, '\0', sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
            //YF
            if(strstr(buf, "[REPORTED]") == NULL)
            {
                printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, polling_cnt);
            }
            else
            {
                bfound_peer = 1;
                break;
            }

            polling_cnt++;
            if (polling_cnt >= 60)
            {
                goto fail;
            }

            if ((!bfound_peer) && (bFind == 0))
            {
                memset(buf, '\0', sizeof(buf));
                memset(cmd, '\0', sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "P2P_FIND");
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                system(strCmd);
                bFind=1;
            } 
        } while(bfound_peer ==0 );

        printf("Peer discovered %s\n",dev_addr);

        /*
           try to connect and wait for event :
           P2P_CONNECT 02:0c:43:35:a2:78 pbc go_intent=7 freq=2462
           P2P_CONNECT 02:0c:43:35:a2:78 12345670 keypad go_intent=7 freq=2462		   
           P2P_CONNECT 02:0c:43:35:a2:78 12345670 display go_intent=7 freq=2462

           case 1: GO
           P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-L5" freq=2462 passphrase="EnIEEE6B" go_dev_addr=02:1f:e2:c5:3d:26

           case 2: GC
           P2P-GROUP-STARTED p2p0 client ssid="DIRECT-ia" freq=0 psk=2ff59eff2217c760f1616cc26a2bb5775dedc50d4e146c4603bde83b9d218d5a go_dev_addr=02:1f:e2:c5:3d:26'
         */		

        if (p2p_priv_p->confMethod == PBC)
        {
            if (getStaP2pStartGrpForm->oper_chn_flag)
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc go_intent=%d freq=%d", dev_addr, intent, op_freq);
            }
            else
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc go_intent=%d", dev_addr, intent);
            }
        }
        else if (p2p_priv_p->confMethod == KEY_PAD)
        {
            if (getStaP2pStartGrpForm->oper_chn_flag)
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad go_intent=%d freq=%d", dev_addr, p2p_priv_p->keypad, intent, op_freq);
            }
            else
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad go_intent=%d", dev_addr, p2p_priv_p->keypad, intent);	
            }
        }
        else if (p2p_priv_p->confMethod == DIPLAY_PIN)
        {
            if (getStaP2pStartGrpForm->oper_chn_flag)
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display go_intent=%d freq=%d", dev_addr, intent, op_freq);
            }
            else
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display go_intent=%d", dev_addr, intent);	
            }
        }
        else
        {
            DPRINT_ERR(WFA_ERR,"p2p_priv_p->confMethod=%d\n",p2p_priv_p->confMethod);	
            goto fail;
        }

        //YF persistent
        if (p2p_priv_p->presistent_oper == 1)
            strcat(cmd, " persistent");

        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);
            conn_cnt++;
            if (conn_cnt >= 60)
            {
                printf("wfaStaP2pStartGrpFormation... formation fail timeout!\n");			
                goto fail;
            }

            printf("wfaStaP2pStartGrpFormation... conn_cnt=%d\n",conn_cnt);
        } while(p2p_priv_p->bNegoDone == 0);
        sleep(5);
        printf("wfaStaP2pStartGrpFormation... formation to role=%d\n", p2p_priv_p->role);
        memset(tmp_if_name, '\0', IFNAMSIZ);
        wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 
        if (p2p_priv_p->role == GO)
        {
            /* Run dhcp server and set IP to p2p interface*/
            printf("%s :  setting IP %s for %s\n", __FUNCTION__, DHCPD_DEF_STATIC_IP_ADDR, tmp_if_name);
            sprintf(strCmd, "ifconfig %s %s",tmp_if_name, DHCPD_DEF_STATIC_IP_ADDR);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);
            printf("%s :  enable dhcp server\n", __FUNCTION__);
            sprintf(strCmd, "mtk_dhcp_server.sh %s", tmp_if_name);
            system(strCmd);
            printf("[SIGMA] system: %s",strCmd);
            infoResp.status = STATUS_COMPLETE;
            strcpy(infoResp.cmdru.grpFormInfo.result, "GO");
            strcpy(infoResp.cmdru.grpFormInfo.grpId, p2p_priv_p->group_id);
            goto ok;
        }
        else if (p2p_priv_p->role == GC)
        {
            /* Run dhcp client*/
            sprintf(strCmd, "mtk_dhcp_client.sh %s",tmp_if_name);
            system(strCmd);
            sprintf(strCmd, "sleep 5");
            system(strCmd);
            infoResp.status = STATUS_COMPLETE;
            strcpy(infoResp.cmdru.grpFormInfo.result, "CLIENT");
            strcpy(infoResp.cmdru.grpFormInfo.grpId, p2p_priv_p->group_id);
            goto ok;
        }
        else //NeGo Fail case
        {
            infoResp.status = STATUS_COMPLETE;
            strcpy(infoResp.cmdru.grpFormInfo.result, " ");
            strcpy(infoResp.cmdru.grpFormInfo.grpId, " ");
            goto ok;
        }
    }
#if 1 //yiwei add for 5.1.15
    else
    {
        strcpy(&dev_addr, getStaP2pStartGrpForm->devId);

        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);

            snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
            if(!wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1))
            {
                bfound_peer = 1;
            }

            polling_cnt++;
            if (polling_cnt >= 60)
            {
                goto fail;
            }
        } while(bfound_peer ==0 );

        printf("Peer discovered %s\n",dev_addr);

        /*
           try to connect and wait for event :
           P2P_CONNECT 02:0c:43:35:a2:78 pbc auth go_intent=7 freq=2462
           P2P_CONNECT 02:0c:43:35:a2:78 12345670  keypad auth go_intent=7 freq=2462		   
           P2P_CONNECT 02:0c:43:35:a2:78 12345670  display auth go_intent=7 freq=2462

           case 1: GO
           P2P-GROUP-STARTED p2p0 GO ssid="DIRECT-L5" freq=2462 passphrase="EnIEEE6B" go_dev_addr=02:1f:e2:c5:3d:26

           case 2: GC
           P2P-GROUP-STARTED p2p0 client ssid="DIRECT-ia" freq=0 psk=2ff59eff2217c760f1616cc26a2bb5775dedc50d4e146c4603bde83b9d218d5a go_dev_addr=02:1f:e2:c5:3d:26'
         */		

#if 0
        if (p2p_priv_p->confMethod == PBC)
        {
            if (getStaP2pStartGrpForm->oper_chn_flag)
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc auth go_intent=%d freq=%d", dev_addr, intent, op_freq);
            }
            else
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc auth go_intent=%d", dev_addr, intent);
            }
        }
        else if (p2p_priv_p->confMethod == KEY_PAD)
        {
            if (getStaP2pStartGrpForm->oper_chn_flag)
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad auth go_intent=%d freq=%d", dev_addr, p2p_priv_p->keypad, intent, op_freq);
            }
            else
            {
                snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad auth go_intent=%d", dev_addr, p2p_priv_p->keypad, intent);	
            }
        }
        else 
#endif
            if (p2p_priv_p->confMethod == DIPLAY_PIN)
            {
                if (getStaP2pStartGrpForm->oper_chn_flag)
                {
                    snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display auth go_intent=%d freq=%d", dev_addr, intent, op_freq);
                }
                else
                {
                    snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display auth go_intent=%d", dev_addr, intent);	
                }
            }
            else
            {
                DPRINT_ERR(WFA_ERR,"p2p_priv_p->confMethod=%d\n",p2p_priv_p->confMethod);	
                goto fail;
            }

        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        infoResp.status = STATUS_COMPLETE;
        strcpy(infoResp.cmdru.grpFormInfo.result, " ");
        strcpy(infoResp.cmdru.grpFormInfo.grpId, " ");
        goto ok;
    }
#endif

fail:
    infoResp.status = STATUS_ERROR;
    strcpy(infoResp.cmdru.grpFormInfo.result,"FALSE");
    strcpy(infoResp.cmdru.grpFormInfo.grpId, " ");
    //		strcpy(infoResp.cmdru.grpFormInfo.result, "CLIENT");
    //		strcpy(infoResp.cmdru.grpFormInfo.grpId, "AA:BB:CC:DD:EE:FF_DIRECT-SSID");
ok:
#endif
    wfaEncodeTLV(WFA_STA_P2P_START_GRP_FORMATION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


/*
 * wfaStaP2pDissolve():
 */
int wfaStaP2pDissolve(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaP2pDissolve_t *getStap2pDissolve= (caStaP2pDissolve_t *)caCmdBuf;
#ifdef MTK_P2P_SIGMA
    char tmp_if_name[IFNAMSIZ] = {0};	
    char buf[WPS_BUF_LENGTH] = {0};
    size_t buflen = 0;
    char cmd[WPS_CMD_LENGTH] = {0};

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaP2pDissolve... \n");

    // Implement the function and this does not return any thing back.
#ifdef MTK_WFD_SIGMA
    extern int wfaMtkWfdCmd_rtspTeardown(void);
    wfaMtkWfdCmd_rtspTeardown();
    sleep(1);
    /* disconnect rtsp first */
#endif
    wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 

    snprintf(cmd, sizeof(cmd), "P2P_GROUP_REMOVE %s",tmp_if_name);
    wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
    printf(" [SIGMA] %s===> %s\n", __FUNCTION__, buf);
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_DISSOLVE_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSendP2pInvReq():
 */
int wfaStaSendP2pInvReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaSendP2pInvReq_t *getStaP2pInvReq= (caStaSendP2pInvReq_t *)caCmdBuf;    
#ifdef MTK_P2P_SIGMA
    char strcmd[256]={0}, buf[1024] = {0};
    int buflen = 0, reinvoke_role = 2, polling_cnt = 0, bfound_peer = 0;
    int netId = 0;

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaSendP2pInvReq... ");
    printf("\n      PEER devId (%s) ",getStaP2pInvReq->devId);
    printf("\n      grpId      (%s) ",getStaP2pInvReq->grpId);
    printf("\n      reinvoke   (%d) \n",getStaP2pInvReq->reinvoke);


    p2p_priv_p->recvInviteRspStatus = 0;
    p2p_priv_p->isRecvInviteRsp = 0;

    if (getStaP2pInvReq->reinvoke == 1)
    {
        p2p_priv_p->isInviteReinvoke = 1;

        for(netId = 0; netId < 10; netId++) {
            memset(strcmd, 0 , sizeof(strcmd));
            memset(buf, 0 , sizeof(buf));
            snprintf(strcmd, sizeof(strcmd), "GET_NETWORK %d disabled", netId);
            buflen = sizeof(buf) - 1;
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);
            if (buf[0] < '0' || buf[0] > '9')
                continue;
            else
                break;
        }
        printf("%s: reinvoke role = %d\n", __FUNCTION__, atoi(buf));
        reinvoke_role = atoi(buf);


        if (reinvoke_role == 2)
        {
            //Check the p2p peer is in table 
            snprintf(strcmd, sizeof(strcmd), "P2P_FIND");
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);

            polling_cnt = 0;
            do {
                sprintf(strcmd, "sleep 1");
                system(strcmd);

                memset(buf, '\0', sizeof(buf));
                memset(strcmd, '\0', sizeof(strcmd));

                snprintf(strcmd, sizeof(strcmd), "P2P_PEER %s", getStaP2pInvReq->devId);
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);

                if(strstr(buf, "[REPORTED]") == NULL)
                {
                    printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__,
                            getStaP2pInvReq->devId, polling_cnt);
                }
                else
                {
                    bfound_peer = 1;
                    break;
                }

                polling_cnt++;
                if (polling_cnt >= 60)
                {
                    printf("\n wfaStaSendP2pInvReq...can't find peer & TIMEOUT\n ");
                    infoResp.status = STATUS_ERROR;
                    goto done;
                }
            } while(1);

            memset(buf, '\0', sizeof(buf));
            memset(strcmd, '\0', sizeof(strcmd));
            snprintf(strcmd, sizeof(strcmd), "P2P_INVITE persistent=%d peer=%s",
                    netId, getStaP2pInvReq->devId);

            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);

            //CmdRsp FAIL
            if(strncmp(buf, "FAIL", 4) == 0)
            {
                infoResp.status = STATUS_ERROR;
                goto done;
            }
        }
        else
        {
            infoResp.status = STATUS_ERROR;
            goto done;
        }
    }
    else
        if(p2p_priv_p->role == GO)
        {
            snprintf(strcmd, sizeof(strcmd), "P2P_INVITE group=%s peer=%s", 
                    p2p_priv_p->p2p_if_name, getStaP2pInvReq->devId);
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);
        }
        else if(p2p_priv_p->role == GC)
        {
            snprintf(strcmd, sizeof(strcmd), "P2P_INVITE group=%s peer=%s", 
                    p2p_priv_p->p2p_if_name, getStaP2pInvReq->devId);

            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, strcmd, buf, buflen, 1);
        }
        else
        {
            infoResp.status = STATUS_ERROR;
            goto done;
        }

    //wait "P2P-INVITATION-RESULT status=0 ff:ff:ff:ff:ff:ff" 
    polling_cnt = 0;
    do {
        sprintf(strcmd, "sleep 1");
        system(strcmd);

        polling_cnt++;
        if (polling_cnt >= 60)
        {
            printf("\n wfaStaSendP2pInvReq...TIMEOUT\n ");
            infoResp.status = STATUS_ERROR;
            goto done;
        }
    } while( p2p_priv_p->isRecvInviteRsp == 0 );   

    if (p2p_priv_p->recvInviteRspStatus != 0)
    {
        printf("\n wfaStaSendP2pInvReq...status:%d ==>fail\n ", p2p_priv_p->recvInviteRspStatus);
        /* 5.1.13 Negative Scenario
         * Do not respond with STATUS_ERROR, it makes UCC stop
         */
    }
#endif

    infoResp.status = STATUS_COMPLETE;

done:
    wfaEncodeTLV(WFA_STA_P2P_SEND_INV_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


/*
 * wfaStaAcceptP2pInvReq():
 */
int wfaStaAcceptP2pInvReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaAcceptP2pInvReq_t *getStaP2pInvReq= (caStaAcceptP2pInvReq_t *)caCmdBuf;   
#ifdef MTK_P2P_SIGMA
    char strcmd[512]={0};
    char tmp_if_name[ IFNAMSIZ ];
    wpa_get_if_by_role(p2p_priv_p, &tmp_if_name);

    printf("\n Entry wfaStaAcceptP2pInvReq... ");
    printf("\n      PEER devId (%s) ",getStaP2pInvReq->devId);
    printf("\n      grpId      (%s) ",getStaP2pInvReq->grpId);
    printf("\n      reinvoke   (%d) ",getStaP2pInvReq->reinvoke);

    if (getStaP2pInvReq->reinvoke == 1)
        p2p_priv_p->isInviteReinvoke = 1; 
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_ACCEPT_INV_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


/*
 * wfaStaSendP2pProvDisReq():
 */
int wfaStaSendP2pProvDisReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaSendP2pProvDisReq_t *getStaP2pProvDisReq= (caStaSendP2pProvDisReq_t *)caCmdBuf;

    printf("\n Entry wfaStaSendP2pProvDisReq... ");

    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SEND_PROV_DIS_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetWpsPbc():
 */
int wfaStaSetWpsPbc(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    buflen = sizeof(buf) - 1;

    printf("\n Entry wfaStaSetWpsPbc... ");

    // Implement the function and this does not return any thing back.
#ifdef MTK_P2P_SIGMA
    // ex : "P2P_CONNECT 02:1f:e2:c5:3d:26 pbc"
    // ex : DUT's GO ==> WPS_PBC

    if (!checksum_ok(p2p_priv_p))
    {
        infoResp.status = STATUS_ERROR;
    }
    else
    {
        if (p2p_priv_p ->role == GO)
        {
            snprintf(cmd, sizeof(cmd), "WPS_PBC");
            printf("wfaStaSetWpsPbc... %s\n",cmd);
            if (p2p_priv_p->p2p_cmd_sock <= 0)
                wpa_p2p_cmd_connect(p2p_priv_p, p2p_priv_p->p2p_if_name);

            wpa_start_cmd(p2p_priv_p->p2p_if_name, p2p_priv_p->p2p_cmd_sock, cmd, buf, buflen, 1);
        }

        p2p_priv_p->confMethod = PBC;
        infoResp.status = STATUS_COMPLETE;
    }
#endif
      
    wfaEncodeTLV(WFA_STA_WPS_SETWPS_PBC_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaWpsReadPin():
 */
int wfaStaWpsReadPin(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    buflen = sizeof(buf) - 1;

    printf("\n Entry wfaStaWpsReadPin... \n");
#ifdef MTK_P2P_SIGMA

    // Fetch the device PIN and put in 	infoResp->cmdru.wpsPin 
    //strcpy(infoResp->cmdru.wpsPin, "12345678");
    //always reply "12345670"
    // ex : "P2P_CONNECT 02:1f:e2:c5:3d:26 12345670 display"
    // ex : DUT's GO ==> WPS_PIN any 12345670
    if (!checksum_ok(p2p_priv_p))
    {
        infoResp.status = STATUS_ERROR;
	    strcpy(&infoResp.cmdru.wpsPin[0], "error!!");		 
    }
    else
    {
    	if(p2p_priv_p->role == GO)
	    {
            snprintf(cmd, sizeof(cmd), "WPS_PIN any 12345670");
            printf("wfaStaWpsReadPin... %s\n",cmd);
            //YF: CHANGE THE CMD SOCK from p2p-p2p0-0
            if (p2p_priv_p->p2p_cmd_sock <= 0)
                wpa_p2p_cmd_connect(p2p_priv_p, p2p_priv_p->p2p_if_name);
            wpa_start_cmd(p2p_priv_p->p2p_if_name, p2p_priv_p->p2p_cmd_sock, cmd, buf, buflen, 1);
        }
        if ((p2p_priv_p->concurrencyOn == 1) &&
                (p2p_priv_p->role == DEVICE))
        {
            snprintf(cmd, sizeof(cmd), "P2P_LISTEN");
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }

        p2p_priv_p->confMethod = DIPLAY_PIN;
        infoResp.status = STATUS_COMPLETE;
        strcpy(&infoResp.cmdru.wpsPin[0], "12345670");
    }
#endif
    wfaEncodeTLV(WFA_STA_WPS_READ_PIN_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}



/*
 * wfaStaWpsReadLabel():
 */
int wfaStaWpsReadLabel(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

    printf("\n Entry wfaStaWpsReadLabel... ");

    // Fetch the device Label and put in	infoResp->cmdru.wpsPin
    //strcpy(infoResp->cmdru.wpsPin, "12345678");
    strcpy(&infoResp.cmdru.wpsPin[0], "1234456");


    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_WPS_READ_PIN_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


/*
 * wfaStaWpsEnterPin():
 */
int wfaStaWpsEnterPin(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaWpsEnterPin_t *getStaWpsEnterPin= (caStaWpsEnterPin_t *)caCmdBuf;
    char buf[WPS_BUF_LENGTH];
    size_t buflen=0;
    char cmd[WPS_CMD_LENGTH];

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaWpsEnterPin... \n");

#ifdef MTK_P2P_SIGMA

    // Implement the function and this does not return any thing back.

    if (!checksum_ok(p2p_priv_p))
    {
        infoResp.status = STATUS_ERROR;
    }
    else
    {
        // ex for : "P2P_CONNECT 02:0c:43:35:a2:78 12345670 keypad go_intent=15 freq=2462"
        p2p_priv_p->confMethod = KEY_PAD;
        sprintf(p2p_priv_p->keypad, "%s", getStaWpsEnterPin->wpsPin);	
        printf("p2p_priv_p->keypad=%s\n",p2p_priv_p->keypad);

        if (p2p_priv_p->role == GO)
        {
            //YF
            if (p2p_priv_p->p2p_cmd_sock <= 0)
                wpa_p2p_cmd_connect(p2p_priv_p, p2p_priv_p->p2p_if_name);
            /*WPS_PIN any  51549961 (peer's PIN)*/
            snprintf(cmd, sizeof(cmd), "WPS_PIN any %s", p2p_priv_p->keypad);
            wpa_start_cmd(p2p_priv_p->p2p_if_name, p2p_priv_p->p2p_cmd_sock, cmd, buf, buflen, 1);
        }
        else if (p2p_priv_p->role == GC)
        {
            printf("%s: GC TBD\n", __FUNCTION__);
        }
        else
        {
            printf("%s: P2P DEVICE TBD\n", __FUNCTION__);
            if ((p2p_priv_p->concurrencyOn == 1) &&
                    (p2p_priv_p->role == DEVICE))
            {
                snprintf(cmd, sizeof(cmd), "P2P_LISTEN");
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
            }
        }

        infoResp.status = STATUS_COMPLETE;
    }
#endif

    wfaEncodeTLV(WFA_STA_WPS_ENTER_PIN_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


/*
 * wfaStaGetPsk():
 */
int wfaStaGetPsk(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
#ifdef MTK_P2P_SIGMA
    caStaGetPsk_t *getStaGetPsk= (caStaGetPsk_t *)caCmdBuf;
    char buf[WPS_BUF_LENGTH]={0};
    size_t buflen=0;
    char cmd[WPS_CMD_LENGTH]={0};
    int op_freq = 0;
    int polling_cnt = 0;

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaGetPsk... \n");

    if (checksum_ok(p2p_priv_p))
    {
        snprintf(cmd, sizeof(cmd), "P2P_GET_PASSPHRASE");
        if (p2p_priv_p->p2p_cmd_sock <= 0)
            wpa_p2p_cmd_connect(p2p_priv_p, p2p_priv_p->p2p_if_name);
        wpa_start_cmd(p2p_priv_p->p2p_if_name, p2p_priv_p->p2p_cmd_sock, cmd, buf, buflen, 1);
        printf("%s : PSK=%s\n", __FUNCTION__,buf);
        printf("%s : SSID=%s\n", __FUNCTION__,p2p_priv_p->ssid);

        strcpy(&infoResp.cmdru.pskInfo.passPhrase[0], buf);
        strcpy(&infoResp.cmdru.pskInfo.ssid[0],p2p_priv_p->ssid);		
    }
    else
    {
        // Fetch the device PP and SSID  and put in 	infoResp->cmdru.pskInfo 
        //strcpy(infoResp->cmdru.wpsPin, "12345678");
        strcpy(&infoResp.cmdru.pskInfo.passPhrase[0], "12345678");
        strcpy(&infoResp.cmdru.pskInfo.ssid[0], "AAAAAAAA");
    }
#endif
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_GET_PSK_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaP2pReset():
 */
int wfaStaP2pReset(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];

    printf("\n Entry wfaStaP2pReset... ");
    // Implement the function and this does not return any thing back.

#ifdef MTK_P2P_SIGMA

    if (checksum_ok(p2p_priv_p))
    {
        p2p_priv_p->confMethod= UN_SET;
        memset(p2p_priv_p->keypad, 0x00, sizeof(p2p_priv_p->keypad));
        p2p_priv_p->go_intent = 7; //default to 7
        p2p_priv_p->bNegoDone = 0; //default to 0 , not yet!
        p2p_priv_p->role = DEVICE; //default to DEVICE
        memset(p2p_priv_p->ssid, 0x00, sizeof(p2p_priv_p->ssid));
        memset(p2p_priv_p->group_id, 0x00, sizeof(p2p_priv_p->group_id));
        p2p_priv_p->presistent_oper = 0;
        p2p_priv_p->isOnInviteSession = 0;
        memset(p2p_priv_p->inviteDevAddr, 0x00, sizeof(p2p_priv_p->inviteDevAddr));
        p2p_priv_p->concurrencyOn = 0;
        p2p_priv_p->isInviteReinvoke = 0;
        p2p_priv_p->freq = 0;
        p2p_priv_p->isRecvInviteRsp = 0;
        p2p_priv_p->recvInviteRspStatus = 0;

        buflen = sizeof(buf) - 1;
        snprintf(cmd, sizeof(cmd), "P2P_GROUP_REMOVE *");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

#if 1 //yiwei reset to no ps, only 7.1.3 need  set power save!
        snprintf(cmd, sizeof(cmd), "P2P_SET ps 0"); //let P2P GC enter power save mode!
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
#endif

        snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_FLUSH");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_SERVICE_FLUSH");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_SET ssid_postfix ");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_EXT_LISTEN");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_SET client_apsd disable");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "P2P_SET go_apsd disable");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        
        snprintf(cmd, sizeof(cmd), "REMOVE_NETWORK all");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        snprintf(cmd, sizeof(cmd), "SAVE_CONFIG");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        printf("  ==== Kill DHCP server ====\n");
        sprintf(cmd, "busybox killall dhcpd");
        system(cmd);
	sprintf(cmd, "mtk_dhcp_reset.sh");
	system(cmd);
        printf("  system: %s \n",cmd);

        printf("  ==== Flush %s IP ====\n", WFA_STAUT_IF_P2P);
        sprintf(cmd, "ifconfig %s 0.0.0.0", WFA_STAUT_IF_P2P);
        system(cmd);
        printf("  system: %s \n",cmd);
    }
#endif
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_RESET_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}



/*
 * wfaStaGetP2pIpConfig():
 */
int wfaStaGetP2pIpConfig(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaGetP2pIpConfig_t *staGetP2pIpConfig= (caStaGetP2pIpConfig_t *)caCmdBuf;
#ifdef MTK_P2P_SIGMA
    char cmdStr[256], strout[128];
    FILE *tmpfile = NULL;

    uint8_t buf[ETH_ALEN] = {0};
    char macStr[32];
    char tmp_if_name[ IFNAMSIZ ];	


    caStaGetIpConfigResp_t *ifinfo = &(infoResp.cmdru.getIfconfig);

    printf("\n Entry wfaStaGetP2pIpConfig... ");

    // Fetch the device IP config  and put in 	infoResp->cmdru 
    //strcpy(infoResp->cmdru.wpsPin, "12345678");
#ifdef MTK_WFD_SIGMA
    {
        extern int wfaMtkWfdP2pGetIfConfig(char *macbuf, char *ipbuf, char *maskbuf);

        memset(ifinfo->mac, 0, WFA_MAC_ADDR_STR_LEN);
        memset(ifinfo->ipaddr, 0, WFA_IP_ADDR_STR_LEN);
        memset(ifinfo->mask, 0, WFA_IP_MASK_STR_LEN);
        if (wfaMtkWfdP2pGetIfConfig(ifinfo->mac, ifinfo->ipaddr, ifinfo->mask) == 0)
        {
            ifinfo->isDhcp = 1;
            strncpy(&(ifinfo->dns[0][0]), ifinfo->ipaddr, WFA_IP_ADDR_STR_LEN);
            fprintf(stderr, "%s: Get P2P IP config success!\n", __FUNCTION__);
        }
        else
        {
            /* set response as static ip */
            fprintf(stderr, "%s: Fail to get P2P IP config, using static config!\n", __FUNCTION__);
            ifinfo->isDhcp =0;
            if (!strlen(ifinfo->ipaddr))
                strcpy(&(ifinfo->ipaddr[0]), "192.165.100.111");
            if (!strlen(ifinfo->mask))
                strcpy(&(ifinfo->mask[0]), "255.255.255.0");
            if (!strlen(ifinfo->dns))
                strcpy(&(ifinfo->dns[0][0]), "192.165.100.1"); 
            if (!strlen(ifinfo->mac))
                strcpy(&(ifinfo->mac[0]), "ba:ba:ba:ba:ba:ba");
        }

        infoResp.status = STATUS_COMPLETE;
        wfaEncodeTLV(WFA_STA_P2P_GET_IP_CONFIG_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

        return WFA_SUCCESS;
    }
#endif

    if (checksum_ok(p2p_priv_p))
    {
        printf("%s: p2p_priv_p->if_name=%s, p2p_if_name=%s, role=[%d]\n",__FUNCTION__,
                p2p_priv_p->if_name, p2p_priv_p->p2p_if_name, p2p_priv_p->role);
        memset(tmp_if_name, '\0', IFNAMSIZ);
        wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 
        /* get MAC */
        sprintf(cmdStr, "busybox ifconfig %s | grep HWaddr | busybox awk '{print $5}' > /tmp/sigma_mac.txt", tmp_if_name);
        printf("get mac cmd: %s\n", cmdStr);
        system(cmdStr);
        /* get IP */		
        sprintf(cmdStr, "busybox ifconfig %s | grep \"inet addr\" | busybox cut -d ' ' -f 12 | busybox cut -c 6- > /tmp/sigma_ip.txt", tmp_if_name);
        printf("get ip cmd: %s\n", cmdStr);
        system(cmdStr);
    }

    ifinfo->isDhcp =1;
    /* get mask and dns ?*/
    strcpy(&(ifinfo->mask[0]), "255.255.255.0");
    strcpy(&(ifinfo->dns[0][0]), "192.165.100.1");

    /* scan/check the output */
    /* MAC Address */
    tmpfile = fopen("/tmp/sigma_mac.txt", "r+");
    if(tmpfile == NULL)
    {
        infoResp.status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_P2P_GET_IP_CONFIG_RESP_TLV, 4, (BYTE *)&infoResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return FALSE;
    }

    if(fscanf(tmpfile, "%s", strout) == EOF)
        strcpy(&(ifinfo->mac[0]), "ba:ba:ba:ba:ba:ba");
    else
    {
        strcpy(&(ifinfo->mac[0]), strout);
    }

    fclose(tmpfile);

    /* IP Address */
    tmpfile = fopen("/tmp/sigma_ip.txt", "r+");
    if(tmpfile == NULL)
    {
        infoResp.status = STATUS_ERROR;
        wfaEncodeTLV(WFA_STA_P2P_GET_IP_CONFIG_RESP_TLV, 4, (BYTE *)&infoResp, respBuf);   
        *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return FALSE;
    }

    if(fscanf(tmpfile, "%s", strout) == EOF)
        strcpy(&(ifinfo->ipaddr[0]), "192.165.100.111");
    else
    {
        strcpy(&(ifinfo->ipaddr[0]), strout);
    }

    fclose(tmpfile);
    /* get mask and dns ?*/
    strcpy(&(ifinfo->mask[0]), "255.255.255.0");
    strcpy(&(ifinfo->dns[0][0]), ifinfo->ipaddr);
#endif
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_GET_IP_CONFIG_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}




/*
 * wfaStaSendServiceDiscoveryReq():
 */
int wfaStaSendServiceDiscoveryReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

#ifdef MTK_P2P_SIGMA
    caStaSendServiceDiscoveryReq_t *staSendServiceDiscoveryReq= (caStaSendServiceDiscoveryReq_t *)caCmdBuf;
    char cmdStr[128], buf[MAX_CMD_BUFF];
    int buflen = 0;
    int polling_cnt = 0, bfound_peer = 0;
    buflen = sizeof(buf) - 1;
#endif

    printf("\n Entry wfaStaSendServiceDiscoveryReq... ");
    // Implement the function and this does not return any thing back.

#ifdef MTK_P2P_SIGMA
    if (checksum_ok(p2p_priv_p))
    {
        do {
            memset(buf, '\0', sizeof(buf));
            memset(cmdStr, '\0', sizeof(cmdStr));
            snprintf(cmdStr, sizeof(cmdStr), "P2P_PEER %s",staSendServiceDiscoveryReq->devId);
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmdStr, buf, buflen, 1);

            if(strstr(buf, "[REPORTED]") == NULL)
            {
                printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, staSendServiceDiscoveryReq->devId, polling_cnt);
            }
            else
            {
                bfound_peer = 1;
                break;
            }

            polling_cnt++;
            if (polling_cnt >= 60)
            {
                break;
            }

            if ((!bfound_peer))
            {
                memset(buf, '\0', sizeof(buf));
                memset(cmdStr, '\0', sizeof(cmdStr));
                snprintf(cmdStr, sizeof(cmdStr), "P2P_FIND");
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmdStr, buf, buflen, 1);
                system("sleep 2");
            } 
        } while(bfound_peer ==0 );

        DPRINT_INFO(WFA_OUT,"Peer discovered %s\n",staSendServiceDiscoveryReq->devId);
        snprintf(cmdStr, sizeof(cmdStr), "P2P_SERV_DISC_REQ %s 02000001", staSendServiceDiscoveryReq->devId);
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmdStr, buf, buflen, 1);
    }
    printf("%s: %s -> %s\n", __FUNCTION__, cmdStr, buf);
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SEND_SERVICE_DISCOVERY_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}



/*
 * wfaStaSendP2pPresenceReq():
 */
int wfaStaSendP2pPresenceReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaSendP2pPresenceReq_t *staSendP2pPresenceReq= (caStaSendP2pPresenceReq_t *)caCmdBuf;

    printf("\n Entry wfaStaSendP2pPresenceReq... ");
    // Implement the function and this does not return any thing back.
    printf("\n The long long Duration: %lld... ",staSendP2pPresenceReq->duration);
    printf("\n The long long interval : %lld.. ",staSendP2pPresenceReq->interval);


    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SEND_PRESENCE_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetSleepReq():
 */
int wfaStaSetSleepReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
#ifdef MTK_P2P_SIGMA
    char buf[WPS_BUF_LENGTH];
    size_t buflen;
    char cmd[WPS_CMD_LENGTH];
    buflen = sizeof(buf) - 1;
#endif 

    dutCmdResponse_t infoResp;
    /* caStaSetSleep_t *staSetSleepReq= (caStaSetSleep_t *)caCmdBuf; */

    printf("\n Entry wfaStaSetSleepReq... ");
    // Implement the function and this does not return any thing back.

#ifdef MTK_P2P_SIGMA
    if (checksum_ok(p2p_priv_p))
    {
        snprintf(cmd, sizeof(cmd), "P2P_SET ps 1"); //let P2P GC enter power save mode!
        if (p2p_priv_p->role == DEVICE)
        {
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        }
        else
        {
            if (p2p_priv_p->p2p_cmd_sock <= 0)
                wpa_p2p_cmd_connect(p2p_priv_p, p2p_priv_p->p2p_if_name);
            wpa_start_cmd(p2p_priv_p->p2p_if_name, p2p_priv_p->p2p_cmd_sock, cmd, buf, buflen, 1);
        }
    }
#endif

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SET_SLEEP_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN +4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetOpportunisticPsReq():
 */
int wfaStaSetOpportunisticPsReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;

    printf("\n Entry wfaStaSetOpportunisticPsReq... ");
    // Implement the function and this does not return any thing back.


    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_SET_OPPORTUNISTIC_PS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
#ifndef WFA_STA_TB
/*
 * wfaStaPresetParams():
 */

int wfaStaPresetParams(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaPresetParameters_t *presetParams = (caStaPresetParameters_t *)caCmdBuf;
    char nonPrefChanStr[32];

    printf( "Inside wfaStaPresetParameters function ...\n");

    if(presetParams->program == PROG_TYPE_MBO)
    {
        if (presetParams->chans.chPrefNum)
        {
            sprintf(nonPrefChanStr, " %d:%d:%d:%d",
                presetParams->chans.chOpClass, presetParams->chans.chPrefNum, presetParams->chans.chPref, presetParams->chans.chReasonCode);
            strcat(gNonPrefChanStr, nonPrefChanStr);
            sprintf(gCmdStr, "wpa_cli -i %s SET non_pref_chan %s", presetParams->intf, gNonPrefChanStr);
            sret = system(gCmdStr);
        }

        if (presetParams->cellularDataCap)
        {
            sprintf(gCmdStr, "wpa_cli -i %s SET mbo_cell_capa %d", presetParams->intf, presetParams->cellularDataCap);
            sret = system(gCmdStr);
        }
    }

    // Implement the function and its sub commands
    infoResp.status = STATUS_COMPLETE;

    wfaEncodeTLV(WFA_STA_PRESET_PARAMETERS_RESP_TLV, 4, (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}
int wfaStaSet11n(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	caSta11n_t * v11nParams = (caSta11n_t *)caCmdBuf;
    dutCmdResponse_t infoResp;
    dutCmdResponse_t *v11nParamsResp = &infoResp;

    int st =0; // SUCCESS
	
	printf( "Inside wfaStaSet11n function....\n"); 

    if(v11nParams->addba_reject != 0xFF && v11nParams->addba_reject < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_addba_reject failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->ampdu != 0xFF && v11nParams->ampdu < 2)
    {
        // implement the funciton

        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_ampdu failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->amsdu != 0xFF && v11nParams->amsdu < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_amsdu failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->greenfield != 0xFF && v11nParams->greenfield < 2)
    {
        // implement the funciton
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "_set_greenfield failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->mcs32!= 0xFF && v11nParams->mcs32 < 2 && v11nParams->mcs_fixedrate[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }
    else if (v11nParams->mcs32!= 0xFF && v11nParams->mcs32 < 2 && v11nParams->mcs_fixedrate[0] == '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs32 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }
    else if (v11nParams->mcs32 == 0xFF && v11nParams->mcs_fixedrate[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_mcs32 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->rifs_test != 0xFF && v11nParams->rifs_test < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_rifs_test failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->sgi20 != 0xFF && v11nParams->sgi20 < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_sgi20 failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->smps != 0xFFFF)
    {
        if(v11nParams->smps == 0)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
        }
        else if(v11nParams->smps == 1)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
            ;
        }
        else if(v11nParams->smps == 2)
        {
            // implement the funciton
            //st = wfaExecuteCLI(gCmdStr);
            ;
        }
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_smps failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->stbc_rx != 0xFFFF)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_stbc_rx failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->width[0] != '\0')
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_11n_channel_width failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->_40_intolerant != 0xFF && v11nParams->_40_intolerant < 2)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_40_intolerant failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    if(v11nParams->txsp_stream != 0 && v11nParams->txsp_stream <4)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_txsp_stream failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }

    }

    if(v11nParams->rxsp_stream != 0 && v11nParams->rxsp_stream < 4)
    {
        // implement the funciton
        //st = wfaExecuteCLI(gCmdStr);
        if(st != 0)
        {
            v11nParamsResp->status = STATUS_ERROR;
            strcpy(v11nParamsResp->cmdru.info, "set_rxsp_stream failed");
            wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, sizeof(dutCmdResponse_t), (BYTE *)v11nParamsResp, respBuf);
            *respLen = WFA_TLV_HDR_LEN + sizeof(dutCmdResponse_t);
            return FALSE;
        }
    }

    v11nParamsResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_11N_RESP_TLV, 4, (BYTE *)v11nParamsResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;
    return WFA_SUCCESS;
}
#endif
/*
 * wfaStaAddArpTableEntry():
 */
int wfaStaAddArpTableEntry(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* caStaAddARPTableEntry_t *staAddARPTableEntry= (caStaAddARPTableEntry_t *)caCmdBuf; uncomment and use it */

    printf("\n Entry wfastaAddARPTableEntry... ");
    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_ADD_ARP_TABLE_ENTRY_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaBlockICMPResponse():
 */
int wfaStaBlockICMPResponse(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    /* caStaBlockICMPResponse_t *staAddARPTableEntry= (caStaBlockICMPResponse_t *)caCmdBuf; uncomment and use it */

    printf("\n Entry wfaStaBlockICMPResponse... ");
    // Implement the function and this does not return any thing back.

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_P2P_BLOCK_ICMP_RESPONSE_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaSetRadio():
 */

int wfaStaSetRadio(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *setRadio = (dutCommand_t *)caCmdBuf;
    dutCmdResponse_t *staCmdResp = &gGenericResp;
    char *str;
    caStaSetRadio_t *sr = &setRadio->cmdsu.sr;

    if(sr->mode == WFA_OFF)
    {
        // turn radio off
    }
    else
    {
        // always turn the radio on
    }

    wfaEncodeTLV(WFA_STA_SET_RADIO_RESP_TLV, 4, (BYTE *)staCmdResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

/*
 * wfaStaSetRFeature():
 */

int wfaStaSetRFeature(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCommand_t *dutCmd = (dutCommand_t *)caCmdBuf;
    caStaRFeat_t *rfeat = &dutCmd->cmdsu.rfeat;
    dutCmdResponse_t *caResp = &gGenericResp;
    char *intf = dutCmd->intf;
    short secchoffset = 0;
    char nonPrefChanStr[32];

    DPRINT_INFO(WFA_OUT, "Entering %s ...\n", __func__);

    DPRINT_INFO(WFA_OUT, "rfeat->prog=%s\n", rfeat->prog);
    DPRINT_INFO(WFA_OUT, "rfeat->chswitchmode=%s\n", rfeat->chswitchmode);
    DPRINT_INFO(WFA_OUT, "rfeat->offchnum=%d\n", rfeat->offchnum);
    DPRINT_INFO(WFA_OUT, "rfeat->secchoffset=%s\n", rfeat->secchoffset);
    DPRINT_INFO(WFA_OUT, "rfeat->uapsd=%d\n", rfeat->uapsd);

    if (strcasecmp(rfeat->prog, "tdls") == 0) {
        if (rfeat->uapsd == eEnable) {
            /* 7668 */
            sprintf(gCmdStr, "iwpriv %s set_power_mode 2", WFA_STAUT_IF);
            printf("wfaStaSetRFeature: %s\n", gCmdStr);
            system(gCmdStr);
            /* 7662 */
            sprintf(gCmdStr, "iwpriv %s set PSMode=Legacy_PSP", WFA_STAUT_IF);
            system(gCmdStr);
        } else {
            /* 7668 */
            sprintf(gCmdStr, "iwpriv %s set_power_mode 0", WFA_STAUT_IF);
            printf("wfaStaSetRFeature: %s\n", gCmdStr);
            system(gCmdStr);
            /* 7662 */
            sprintf(gCmdStr, "iwpriv %s set PSMode=Disable", WFA_STAUT_IF);
            system(gCmdStr);
        }

        if (rfeat->tpktimer == eEnable)	{
            sprintf(gCmdStr, "iwpriv %s set TdlsDisableKeyTimeout=0", WFA_STAUT_IF);
            system(gCmdStr);
        } else {
            sprintf(gCmdStr, "iwpriv %s set TdlsDisableKeyTimeout=1", WFA_STAUT_IF);
            system(gCmdStr);
        }

        /* 7668 */
        if ((strcasecmp(rfeat->chswitchmode, "enable") == 0) || (strcasecmp(rfeat->chswitchmode, "initiate") == 0)) {
            if(strcasecmp(rfeat->secchoffset, "20") == 0)
                secchoffset = 0;
            else if (strcasecmp(rfeat->secchoffset, "40above") == 0)
                secchoffset = 1;
            else if (strcasecmp(rfeat->secchoffset, "40below") == 0)
                secchoffset = 3;

            sprintf(gCmdStr, "iwpriv %s driver \"set_chip tdls 1  %s  %d 0 %d  0 0 \" ", WFA_STAUT_IF, rfeat->peer, rfeat->offchnum, secchoffset);
            printf("wfaStaSetRFeature: %s\n", gCmdStr);
            system(gCmdStr);
        } else if ((strcasecmp(rfeat->chswitchmode, "disable") == 0) || (strcasecmp(rfeat->chswitchmode, "passive") == 0)) {
            sprintf(gCmdStr, "iwpriv %s driver \"set_chip tdls 0  %s  %d 0 %d  0 0 \" ", WFA_STAUT_IF, rfeat->peer, rfeat->offchnum, secchoffset);
            printf("wfaStaSetRFeature: %s\n", gCmdStr);
            system(gCmdStr);
        }

        /* 7662 */
        if ((strcasecmp(rfeat->chswitchmode, "enable") == 0) || (strcasecmp(rfeat->chswitchmode, "initiate") == 0)) {
            if(strcasecmp(rfeat->secchoffset, "20") == 0)
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=0", WFA_STAUT_IF);
            else if (strcasecmp(rfeat->secchoffset, "40") == 0) {
                if ((rfeat->offchnum == 36) || (rfeat->offchnum == 44) || (rfeat->offchnum == 52) ||
                        (rfeat->offchnum == 60) || (rfeat->offchnum == 100) || (rfeat->offchnum == 108) ||
                        (rfeat->offchnum == 116) || (rfeat->offchnum == 124) || (rfeat->offchnum == 132) ||
                        (rfeat->offchnum == 149) || (rfeat->offchnum == 157))
                    sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=1", WFA_STAUT_IF);
                else if ((rfeat->offchnum == 40) || (rfeat->offchnum == 48) || (rfeat->offchnum == 56) || 
                        (rfeat->offchnum == 64) || (rfeat->offchnum == 104) || (rfeat->offchnum == 112) ||
                        (rfeat->offchnum == 120) || (rfeat->offchnum == 128) || (rfeat->offchnum == 136) ||
                        (rfeat->offchnum == 153) || (rfeat->offchnum == 161))
                    sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=3", WFA_STAUT_IF);
                else
                    sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=0", WFA_STAUT_IF);
            } else if (strcasecmp(rfeat->secchoffset, "40above") == 0) {
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=1",WFA_STAUT_IF);
            } else if (strcasecmp(rfeat->secchoffset, "40below") == 0) {
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=3",WFA_STAUT_IF);
            } else {
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=0",WFA_STAUT_IF);
            }

            system(gCmdStr);

            sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitch=%s-%d", WFA_STAUT_IF, rfeat->peer, rfeat->offchnum);
            system(gCmdStr);
        } else if ((strcasecmp(rfeat->chswitchmode, "disable") == 0) || (strcasecmp(rfeat->chswitchmode, "passive") == 0)) {
            sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchReject=0", WFA_STAUT_IF);
            system(gCmdStr);
            sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchDisable=%s", WFA_STAUT_IF, rfeat->peer);
            system(gCmdStr);
        } else if(strcasecmp(rfeat->chswitchmode, "RejReq") == 0) {
            sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchReject=1", WFA_STAUT_IF);
            system(gCmdStr);
        } else if(strcasecmp(rfeat->chswitchmode, "UnSolResp") == 0) {
            if(strcasecmp(rfeat->secchoffset, "20") == 0)
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=0",WFA_STAUT_IF);
            else if (strcasecmp(rfeat->secchoffset, "40above") == 0)
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=1",WFA_STAUT_IF);
            else if (strcasecmp(rfeat->secchoffset, "40below") == 0)
                sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchBW=3",WFA_STAUT_IF);
            system(gCmdStr);

            sprintf(gCmdStr, "iwpriv %s set TdlsChannelSwitchUnSolRsp=%s-%d", WFA_STAUT_IF, rfeat->peer, rfeat->offchnum);
            system(gCmdStr);
        }
    }
    if(strcasecmp(rfeat->prog, "mbo") == 0)
    {
        if (rfeat->chPrefClear)
        {
            memset(gNonPrefChanStr, 0, sizeof(gNonPrefChanStr));
            sprintf(gCmdStr, "wpa_cli -i %s SET non_pref_chan 0", dutCmd->intf);
            sret = system(gCmdStr);
            printf("\n gCmdStr:%s\n ", gCmdStr);
        }

        if (rfeat->chans.chPrefNum)
        {
            sprintf(nonPrefChanStr, " %d:%d:%d:%d",
                rfeat->chans.chOpClass, rfeat->chans.chPrefNum, rfeat->chans.chPref, rfeat->chans.chReasonCode);
            strcat(gNonPrefChanStr, nonPrefChanStr);
            sprintf(gCmdStr, "wpa_cli -i %s SET non_pref_chan %s", dutCmd->intf, gNonPrefChanStr);
            sret = system(gCmdStr);
            printf("\n gCmdStr:%s\n ", gCmdStr);
        }

        if (rfeat->cellularDataCap)
        {
            sprintf(gCmdStr, "wpa_cli -i %s SET mbo_cell_capa %d", dutCmd->intf, rfeat->cellularDataCap);
            sret = system(gCmdStr);
        }
    }

    caResp->status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_SET_RFEATURE_RESP_TLV, 4, (BYTE *)caResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + 4;

    return WFA_SUCCESS;
}

void wfa_cfg80211_tdls_setup(unsigned char *dev_addr)
{
    unsigned char strCmd[128];

    printf("@@@@ TDLS_SETUP: tdls->peer : %s\n", dev_addr);
    sprintf(strCmd, "wpa_cli -i %s %s TDLS_SETUP %s", WFA_STAUT_IF, ctrl_if, dev_addr);
    system(strCmd);

    return;
}

/*
 * wfaStaStartWfdConnection():
 */	
int wfaStaStartWfdConnection(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaStartWfdConn_t *staStartWfdConn= (caStaStartWfdConn_t *)caCmdBuf; //uncomment and use it
    unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0}, group_id[WFA_P2P_GRP_ID_LEN] = {0};
    unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char tmp_ping_result[8] = {0};
    unsigned char  bNegotiate = 0, intent, op_chn;
    unsigned char strCmd[256] = {0};
    int conn_cnt = 0;
    unsigned short rtsp_port = 0;
    unsigned int freq = 0;
    char cmdStr[256];

#ifdef MTK_WFD_SIGMA
    char session_id[32] = "";
    memset(dev_addr, 0, sizeof(dev_addr));

    char buf[WPS_BUF_LENGTH] = {0};
    size_t buflen = 0;
    char cmd[WPS_CMD_LENGTH] = {0};
    unsigned char bfound_peer = 0;
    unsigned char session_avail = 0;
    unsigned char dev_info = 0;

    buflen = sizeof(buf) - 1;
    if (!checksum_ok(p2p_priv_p))
    {
        printf("[CFG80211] checksum_ok()  Failed!\n");
        infoResp.status = STATUS_ERROR;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        return WFA_SUCCESS;
    }

#ifdef MTK_BDP_SIGMA
        snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");
        buflen = sizeof(buf) - 1;
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
        sleep(1);
#endif
    snprintf(cmd, sizeof(cmd), "P2P_FIND");
    wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
    sleep(5);


    printf("Enter wfaStaStartWfdConnection... \n");
    printf("Connection info: inf[%s],peer[0]=[%s],peer[1]=[%s],GoIntent[%d],Init WFD[%d]\n", 
            staStartWfdConn->intf, 
            staStartWfdConn->peer[0],
            (strlen(staStartWfdConn->peer[1])?staStartWfdConn->peer[1]:"NULL"),
            staStartWfdConn->intent_val, 
            staStartWfdConn->init_wfd);

    if (staStartWfdConn->oper_chn_flag)
    {
        op_chn = staStartWfdConn->oper_chn;
        /* channels 1..13 */ /* reg_class : 81 */
        if (op_chn > 13)
            freq = 5180 + 5 * (op_chn-36);
        else
            freq = 2407 + 5 * op_chn;

        printf(" Set Channel(%d) with Freq(%d) \n",op_chn, freq);			
        sprintf(strCmd, "wpa_cli -i %s %s set p2p_oper_freq=%d", staStartWfdConn->intf, ctrl_if, freq);
        p2p_priv_p->freq = freq;
        system(strCmd);
        printf(" System : %s\n",strCmd);
        printf("%s - OpChannel[%d].\n", __func__, op_chn);
    }

    intent = staStartWfdConn->intent_val;
    if (intent == 16)
        intent = MTK_DERAULT_GO_INTENT;

    sprintf(strCmd, "wpa_cli -i %s %s set p2p_go_intent %d",staStartWfdConn->intf, ctrl_if, intent);
    system(strCmd);
    printf(" System : %s\n",strCmd);

    strcpy(&dev_addr, staStartWfdConn->peer[0]);
    if (staStartWfdConn->init_wfd)
    {
        wfdConnPeerIsInitiator = 0;
        wfdConnIamAutoGo = 0;
        {
            do {
                sprintf(strCmd, "sleep 1");
                system(strCmd);
                snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

                printf("%s===> %s\n", __FUNCTION__, buf);
                if(strncmp(buf, "FAIL", 4) == 0)
                    printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, conn_cnt);
                else
                {
                    bfound_peer = 1;
                    printf("Peer discovered %s\n",dev_addr);
                    break;
                }
                conn_cnt++;
            } while(conn_cnt <= WFD_SLEEP_TIME_BEFORE_CONN_ATTEMP );

            char tmp_rtspport[128] = {0};
            char tmp_string[128] = {0};
            char tmp_dev_info[128] = {0};											

            //mtk94097 Need Driver to get port from WFD_IE
            sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
            system(strCmd);						
            printf("system: %s\n",strCmd);

            FILE *tmpfd;
            tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
            if(tmpfd == NULL)
            {
                printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
                rtsp_port = DEFAULT_WFD_RTSP_PORT;
            }
            else
            {
                fgets(tmp_string, sizeof(tmp_string), tmpfd);
                //sample code: 00 0006 0111 ^022a^ 0064
                memcpy(tmp_dev_info,tmp_string+6,4);
                memcpy(tmp_rtspport,tmp_string+10,4);
                printf("tmp_rtspport: %s  tmp_dev_info: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_dev_info,tmp_string);
                rtsp_port = strtol(tmp_rtspport, NULL , 16);														
                dev_info = strtol(tmp_dev_info, NULL , 16);
                session_avail = (dev_info & WFD_DEV_INFO_BITMASK_SESS_AVAIL) > 1;
                fclose(tmpfd);							
            }
        }

        if (wfdTdlsEnable == 0)
        {
p2p_connection:
            printf("[SIGMA] Starting P2P connection to peer[0]=[%s]\n", dev_addr);

            //refer to wfaStaP2pStartGrpFormation()

            snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");			
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

            if (bfound_peer && session_avail == 1) {
                if (p2p_priv_p->confMethod == PBC) {
                    snprintf(cmd, sizeof(cmd),
                            "P2P_CONNECT %s pbc go_intent=%d freq=%d ",
                            dev_addr, intent, freq);
                } else if (p2p_priv_p->confMethod == KEY_PAD) {
                    snprintf(cmd, sizeof(cmd),
                            "P2P_CONNECT %s %s keypad go_intent=%d freq=%d",
                            dev_addr, p2p_priv_p->keypad, intent, freq);
                } else if (p2p_priv_p->confMethod == DIPLAY_PIN) {
                    snprintf(cmd, sizeof(cmd),
                            "P2P_CONNECT %s 12345670 display go_intent=%d freq=%d",
                            dev_addr, intent, freq);
                } else {
                    printf("[CFG80211] p2p_priv_p->confMethod=%d \n",
                            p2p_priv_p->confMethod);
                    infoResp.status = STATUS_ERROR;
                    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
                    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                    strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                    return WFA_SUCCESS;
                }

                if (p2p_priv_p->presistent_oper == 1) {
                    strcat(cmd, " persistent");
                    printf("[%s] with persistent!\n", __func__);
                } else {
                    printf("[%s] without persistent!\n", __func__);
                }

                wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
            }

            if(session_avail==1)
            {
                wfdMtkWfdCheckP2pConnResult(&bNegotiate, dev_addr, sizeof(dev_addr));
            }
            else
            {
                bNegotiate = WFD_SESSION_UNAVALIBLE;
            }

            if (bNegotiate == P2P_DEVICE)
            {
                printf("I'm P2P Device!\n");
                infoResp.status = STATUS_COMPLETE;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            }
            else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
            {
                /* session not available */
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " WFD Connection Failed (Session Not Available)\n");
                fprintf(stderr, "------------------------\n");
                infoResp.status = STATUS_COMPLETE;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            }
            else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
            {
                char Rule[10] = {0};

                if (bNegotiate == P2P_GO)
                    sprintf(&Rule, "%s", "GO");
                else
                    sprintf(&Rule, "%s", "CLIENT");
                printf("I'm P2P %s!\n", Rule);

                if(p2p_priv_p->group_id[0] != '\0')
                {
                    printf("[SIGMA]: group_id: %s\n",p2p_priv_p->group_id);
                    strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
                }
                char tmp_rtspport[128] = {0};
                char tmp_string[128] = {0};

                //mtk94097 Need Driver to get port from WFD_IE
                sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
                system(strCmd);						
                printf("system: %s\n",strCmd);

                sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep intended_addr | busybox cut -d'=' -f2 > /tmp/sigma_if_addr_wfd.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
                system(strCmd);						
                printf("system: %s\n",strCmd);


                FILE *tmpfd;
                tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
                if(tmpfd == NULL)
                {
                    printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
                    rtsp_port = DEFAULT_WFD_RTSP_PORT;
                }
                else
                {
                    fgets(tmp_string, sizeof(tmp_string), tmpfd);
                    //sample code: 00 0006 0111 ^022a^ 0064
                    memcpy(tmp_rtspport,tmp_string+10,4);
                    printf("tmp_rtspport: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_string);
                    rtsp_port = strtol(tmp_rtspport, NULL , 16);

                    fclose(tmpfd);
                }

                tmpfd = NULL;
                tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
                if(tmpfd == NULL)
                {
                    printf("[SIGMA] %s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
                    memcpy(if_addr, dev_addr, sizeof(dev_addr));
                    printf("[SIGMA] device mac: %s  \n",if_addr);
                }
                else
                {
                    fgets(if_addr, sizeof(if_addr), tmpfd);
                    printf("[SIGMA] interface mac: %s  \n",if_addr);
                    fclose(tmpfd);
                }


                //sprintf(group_id, "%s %s", pdrv_ralink_cfg->addr, pdrv_ralink_cfg->ssid);

#ifdef MTK_WFD_SIGMA
                {
                    char peer_ip[24] = "";
                    int result = 0;
                    char port_str[16] = "";
                    int rtsp_retry = 0;

                    extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);

                    if (rtsp_port == 0)
                    {
                        fprintf(stderr, "[SIGMA] ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                        rtsp_port = DEFAULT_WFD_RTSP_PORT;
                    }

                    sprintf(port_str, "%d", rtsp_port);
                    if (bNegotiate == P2P_GO)
                    {
                        fprintf(stderr, "[SIGMA] Starting DHCP server...\n");

                        if (wfaMtkWfdStartDhcpServer(if_addr, peer_ip, sizeof(peer_ip)) != 0)
                        {
                            fprintf(stderr, "Fail to get DHCP Client IP address.\n");
                            result = -1;
                        }
                    }
                    else if (bNegotiate == P2P_CLIENT)
                    {
                        fprintf(stderr, "[SIGMA] Starting DHCP client...\n");
                        if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
                        {
                            fprintf(stderr, "Fail to get DHCP Server IP address.\n");
                            result = -1;
                        }
                    }


                    //Add workaround solution for Marvell TestBed Getting IP TOOOOO late
                    printf("[SIGMA] workaround solution for Marvell TestBed Getting IP TOOOOO late\n");
                    conn_cnt = 0;
                    while(conn_cnt < 60 )
                    {
                        sprintf(strCmd, "busybox ping %s -c 4|grep transmitted|busybox cut -d',' -f2|busybox cut -d' ' -f2 >/tmp/sigma_ping_result_wfd.txt",peer_ip);
                        system(strCmd);						
                        printf("system: %s\n",strCmd);

                        //check ping result

                        tmpfd = NULL;
                        tmpfd = fopen("/tmp/sigma_ping_result_wfd.txt", "r+");
                        if(tmpfd == NULL)
                        {
                            printf("[SIGMA] %s - fail to open /tmp/sigma_ping_result_wfd.txt\n", __func__);
                            return WFA_FAILURE;
                        }
                        else
                        {
                            fgets(tmp_ping_result, sizeof(tmp_ping_result), tmpfd);
                            if(tmp_ping_result[0] == 0 || tmp_ping_result[0] == '0'
                               || tmp_ping_result[0] == '\0')
                            {
                                printf("[SIGMA]conn_cnt: %d  \n",conn_cnt);
                                conn_cnt++;
                                fclose(tmpfd);
                                sleep(1);                                    
                                continue;
                            }
                            else
                            {
                                printf("[SIGMA]tmp_ping_result: %s  \n",tmp_ping_result);
                                fclose(tmpfd);
                                break;
                            }
                        }
                    }

                    if (result == 0)
                    {
                        /* starting rtsp */
                        while (rtsp_retry < MAX_RTSP_RETRY)
                        {
                            sleep(10);
                            fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                            if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                            {
                                /* successful */
                                fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                                result = 0;
                                break;
                            }
                            else
                            {
                                /* failed */
                                fprintf(stderr, "RTSP negotiation is failed\n");
                                result = -1;
                            }
                            rtsp_retry ++;
                            if (rtsp_retry < MAX_RTSP_RETRY)
                                fprintf(stderr, "Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                        }
                    }


                    if (result != 0)
                    {
                        fprintf(stderr, "------------------------\n");
                        fprintf(stderr, "WFD Connection Failed!!!\n");
                        fprintf(stderr, "------------------------\n");
                        infoResp.status = STATUS_COMPLETE;
                        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                    }
                    else
                    {
                        fprintf(stderr, "------------------------\n");
                        fprintf(stderr, " WFD Connection Success\n");
                        fprintf(stderr, "------------------------\n");
                        infoResp.status = STATUS_COMPLETE;
                        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], Rule);
                        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], group_id);
                        if (strlen(session_id))
                            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], session_id);
                        else
                            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
                        if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)								
                        {
                            sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);								
                            system(cmdStr);
                        }
                    }
                }
#else
                infoResp.status = STATUS_COMPLETE;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], &Rule[0]);
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], group_id);
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
#endif 
            }
        }
#ifdef MTK_WFD_SIGMA
        else /* wfd connect by tdls */
        {
            char peer_ip[24] = "";
            int result = 0;
            char port_str[16] = "";
            int rtsp_retry = 0;
            char peer_ip_addr[5] = {0};
            unsigned char *target_addr = NULL;
            extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);

            sprintf(cmdStr, "wpa_cli -i %s %s p2p_listen", WFA_STAUT_IF_P2P, ctrl_if);
            system(cmdStr);
            system("sleep 1");

            /* Check which addr should we connect to. 
               It is supposed to use peer[1] if it is supplied */

            memset(dev_addr, 0, sizeof(dev_addr));
            if (strlen(staStartWfdConn->peer[1]))
                strncpy(dev_addr, staStartWfdConn->peer[1], sizeof(dev_addr));
            else
                strncpy(dev_addr, staStartWfdConn->peer[0], sizeof(dev_addr));

            printf("Starting TDLS connection, dev_addr=[%s]\n", dev_addr);

            strcpy(&dev_addr, staStartWfdConn->peer[0]);

            wfa_cfg80211_tdls_setup(&dev_addr);

            wfdMtkWfdCheckTdlsConnResult(&bNegotiate, dev_addr);

            if (bNegotiate == TDLS_LINKED)
            {
                FILE *tmpfile = NULL;
                char tmpBuf[16];
                char tmp_rtspport[8];

                sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_peer_ip= | busybox cut -f2- -d= > /tmp/tdlsPeerIP", WFA_STAUT_IF, ctrl_if);
                system(cmdStr);
                tmpfile = fopen("/tmp/tdlsPeerIP", "r+");
                if(tmpfile == NULL)
                {
                    /* hope not happen */
                    DPRINT_ERR(WFA_ERR, "file open failed\n");
                    return WFA_FAILURE;
                }
                fscanf(tmpfile, "%s", tdls_peer_ip);

                sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_dev_info= | busybox cut -f2- -d= > /tmp/tdlsDevInfo", WFA_STAUT_IF, ctrl_if);
                system(cmdStr);
                tmpfile = fopen("/tmp/tdlsDevInfo", "r+");
                if(tmpfile == NULL)
                {
                    /* hope not happen */
                    DPRINT_ERR(WFA_ERR, "file open failed\n");
                    return WFA_FAILURE;
                }
                fscanf(tmpfile, "%s", tmpBuf);
                memcpy(tmp_rtspport, tmpBuf + 4, 4);
                tmp_rtspport[4] = '\0';
                printf("tmp_rtspport: %s  <tmpBuf: %s>\n", tmp_rtspport, tmpBuf);
                rtsp_port = strtol(tmp_rtspport, NULL , 16);

                if (rtsp_port == 0)
                {
                    fprintf(stderr, "ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                    rtsp_port = DEFAULT_WFD_RTSP_PORT;
                }
                sprintf(port_str, "%d", rtsp_port);

                /* starting rtsp */
                while (rtsp_retry < MAX_RTSP_RETRY)
                {
                    sleep(15);
                    fprintf(stderr, "Starting RTSP to [%s:%s]...\n", tdls_peer_ip, port_str);
                    if (wfaMtkWfdCmd_rtspStart(tdls_peer_ip, port_str, session_id) == 0)
                    {
                        /* successful */
                        fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                        result = 0;
                        break;
                    }
                    else
                    {
                        /* failed */
                        fprintf(stderr, "RTSP negotiation is failed\n");
                        result = -1;
                    }
                    rtsp_retry ++;
                    if (rtsp_retry < MAX_RTSP_RETRY)
                        fprintf(stderr, "Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                }

                if (result != 0)
                {
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, "WFD Connection Failed!!!\n");
                    fprintf(stderr, "------------------------\n");
                    infoResp.status = STATUS_COMPLETE;
                    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                    strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                }
                else
                {
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, " WFD Connection Success\n");
                    fprintf(stderr, "------------------------\n");
                    infoResp.status = STATUS_COMPLETE;
                    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "SUCCESS");
                    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], group_id);
                    if (strlen(session_id))
                        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], session_id);
                    else
                        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
                }
            }
            else if (bNegotiate == WFD_PEER_TDLS_WEAK_SECURITY)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " Peer WFD TDLS Week Security!\n");
                fprintf(stderr, "------------------------\n");
                /* modify for test case 6.1.22 P-SnUT does not use TDLS when connected to a non-secure BSS */
                goto p2p_connection;
            }
            else if (bNegotiate == WFD_PEER_PC_P2P)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " Peer WFD TDLS PC-bit is OFF!\n");
                fprintf(stderr, "------------------------\n");
                /* As determined by WFA staff, testbed shall not fallback t P2P mode,
                   but just return -1 */
                infoResp.status = STATUS_COMPLETE;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            }
            else
            {
                printf("TDLS link fail!bNegotiate=%d\n", bNegotiate);
                infoResp.status = STATUS_COMPLETE;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            }	
        }
#endif /* MTK_WFD_SIGMA */
    }
    else
    {
        /* Stop scan and lock at Listen Channel to response peer P2P scanning */	
        snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        int res = -1;
        pthread_t t_pid;
        memset(&argv_in, 0 ,sizeof(wfdInit0Argv_t));
        strcpy(argv_in.dev_addr,dev_addr);
        argv_in.freq = freq;
        argv_in.intent = intent;
        printf("[SIGMA]  dev_addr	: %s\n",argv_in.dev_addr);
        printf("[SIGMA]  freq		: %d \n",argv_in.freq);
        printf("[SIGMA]  intent		: %d \n",argv_in.intent );


        res = pthread_create(&t_pid, NULL, (void *)&wfaStaWaitingWfdConnection_Nego, (void *)&argv_in);
        if (res < 0)
        {
            printf("[SIGMA]pthread_create(wfaStaCheckWfdConnection) error\n");
            return -1;
        }
        printf("[SIGMA]  after	pthread_create() \n");


        wfdConnPeerIsInitiator = 1;
        wfdConnIamAutoGo = 0;
        fprintf(stderr, "Init GO = 0\n");	

        infoResp.status = STATUS_COMPLETE/*STATUS_RUNNING*/;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], " ");
    }
#endif

    //infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_START_WFD_CONNECTION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);	
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}
/*
 * wfaStaCliCommand(): 
 */

int wfaStaCliCommand(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    char cmdName[32];
    char *pcmdStr=NULL, *str;
    int  st = 1;
    char CmdStr[WFA_CMD_STR_SZ];
    FILE *wfaCliFd;
    char wfaCliBuff[64];
    char retstr[256];
    int CmdReturnFlag =0;
    char tmp[256];
    FILE * sh_pipe;
    caStaCliCmdResp_t infoResp;

    printf("\nEntry wfaStaCliCommand; command Received: %s\n",caCmdBuf);
    memcpy(cmdName, strtok_r((char *)caCmdBuf, ",", (char **)&pcmdStr), 32);
    sprintf(CmdStr, "%s",cmdName);

    for(;;)
    {
        // construct CLI standard cmd string
        str = strtok_r(NULL, ",", &pcmdStr);
        if(str == NULL || str[0] == '\0')
            break;
        else
        {
            sprintf(CmdStr, "%s /%s",CmdStr,str);
            str = strtok_r(NULL, ",", &pcmdStr);
            sprintf(CmdStr, "%s %s",CmdStr,str);
        }
    }
    // check the return process
    wfaCliFd=fopen("/etc/WfaEndpoint/wfa_cli.txt","r");
    if(wfaCliFd!= NULL)
    {
        while(fgets(wfaCliBuff, 64, wfaCliFd) != NULL)
        {
            //printf("\nLine read from CLI file : %s",wfaCliBuff);
            if(ferror(wfaCliFd))
                break;

            str=strtok(wfaCliBuff,"-");
            if(strcmp(str,cmdName) == 0)
            {
                str=strtok(NULL,",");
                if (str != NULL)
                {
                    if(strcmp(str,"TRUE") == 0)
                        CmdReturnFlag =1;
                }
                else
                    printf("ERR wfa_cli.txt, inside line format not end with , or missing TRUE/FALSE\n");
                break;
            }
        }
        fclose(wfaCliFd);
    }
    else
    {
        printf("/etc/WfaEndpoint/wfa_cli.txt is not exist\n");
        goto cleanup;
    }

    //printf("\n Command Return Flag : %d",CmdReturnFlag);
    memset(&retstr[0],'\0',255);
    memset(&tmp[0],'\0',255);
    sprintf(gCmdStr, "%s",  CmdStr);
    printf("\nCLI Command -- %s\n", gCmdStr);

    sh_pipe = popen(gCmdStr,"r");
    if(!sh_pipe)
    {
        printf ("Error in opening pipe\n");
        goto cleanup;
    }

    sleep(5);
    //tmp_val=getdelim(&retstr,255,"\n",sh_pipe);
    if (fgets(&retstr[0], 255, sh_pipe) == NULL)
    {
        printf("Getting NULL string in popen return\n");
        goto cleanup;
    }
    else
        printf("popen return str=%s\n",retstr);

    sleep(2);
    if(pclose(sh_pipe) == -1)
    {
        printf("Error in closing shell cmd pipe\n");
        goto cleanup;
    }
    sleep(2);

    // find status first in output
    str = strtok_r((char *)retstr, "-", (char **)&pcmdStr);
    if (str != NULL)
    {
        memset(tmp, 0, 10);
        memcpy(tmp, str,  2);
        printf("cli status=%s\n",tmp);
        if(strlen(tmp) > 0)
            st = atoi(tmp);
        else printf("Missing status code\n");
    }
    else
    {
        printf("wfaStaCliCommand no return code found\n");
    }
    infoResp.resFlag=CmdReturnFlag;

cleanup:

    switch(st)
    {
    case 0:
        infoResp.status = STATUS_COMPLETE;
        if (CmdReturnFlag)
        {
            if((pcmdStr != NULL) && (strlen(pcmdStr) > 0) )
            {
                memset(&(infoResp.result[0]),'\0',WFA_CLI_CMD_RESP_LEN-1);
                strncpy(&infoResp.result[0], pcmdStr ,(strlen(pcmdStr) < WFA_CLI_CMD_RESP_LEN ) ? strlen(pcmdStr) : (WFA_CLI_CMD_RESP_LEN-2) );
                printf("Return CLI result string to CA=%s\n", &(infoResp.result[0]));
            }
            else
            {
                strcpy(&infoResp.result[0], "No return string found\n");
            }
        }
        break;
    case 1:
        infoResp.status = STATUS_ERROR;
        break;
    case 2:
        infoResp.status = STATUS_INVALID;
        break;
    }

    wfaEncodeTLV(WFA_STA_CLI_CMD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    printf("Exit from wfaStaCliCommand\n");
    return TRUE;

}
/*
 * wfaStaConnectGoStartWfd():
 */

int wfaStaConnectGoStartWfd(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaConnectGoStartWfd_t *staConnecGoStartWfd= (caStaStartWfdConn_t *)caCmdBuf; //uncomment and use it
    unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char group_id[WFA_P2P_GRP_ID_LEN] ={0};
    unsigned char  bNegotiate = 0, intent, op_chn;
    unsigned char strCmd[256] = {0};
    int conn_cnt = 0;
    unsigned short rtsp_port = 0;
#ifdef MTK_WFD_SIGMA
    char session_id[64] = {0} ;
#endif

    printf("\n Entry wfaStaConnectGoStartWfd... \n");

#ifdef MTK_WFD_SIGMA 

    char cmd[WPS_CMD_LENGTH] = {0};
    char buf[WPS_BUF_LENGTH] = {0};
    size_t buflen;
    unsigned char bfound_peer = 0;
    FILE *tmpfd;
    char tmp_string[64] = {0};
    char tmp_rtspport[64] = {0};
    char peer_ip[24] = {0};
    unsigned char tmp_ping_result[8] = {0};
    char port_str[16] = {0};
    int result = -1;
    char cmdStr[128];

    buflen = sizeof(buf) - 1;
    printf("[SIGMA]  P2P Connect ::  Intf[%s].    GroupId[%s].    DevId[%s].\n", staConnecGoStartWfd->intf, staConnecGoStartWfd->grpid, staConnecGoStartWfd->devId);

    if (!checksum_ok(p2p_priv_p))
    {
        printf("[SIGMA] p2p_priv_p is INVALID \n");
        infoResp.status = STATUS_ERROR;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        return WFA_SUCCESS;
    }

    strcpy(&dev_addr, staConnecGoStartWfd->devId);	

    //polling for peers
    do {
        sprintf(strCmd, "sleep 1");
        system(strCmd);

        snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

        printf("%s===> %s\n", __FUNCTION__, buf);
        if(strncmp(buf, "FAIL", 4) == 0)
            printf("%s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, conn_cnt);
        else
        {
            bfound_peer = 1;
            printf("Peer discovered %s\n",dev_addr);
            break;
        }
        conn_cnt++;
    } while(conn_cnt <= 20 );
    conn_cnt =0 ;


    snprintf(cmd, sizeof(cmd), "P2P_STOP_FIND");			
    wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);


    //Try to join the group
    if (bfound_peer)
    {	
        if (p2p_priv_p->confMethod == PBC)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s pbc join", dev_addr);
        }
        else if (p2p_priv_p->confMethod == KEY_PAD)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s %s keypad join", dev_addr, p2p_priv_p->keypad);	
        }
        else if (p2p_priv_p->confMethod == DIPLAY_PIN)
        {
            snprintf(cmd, sizeof(cmd), "P2P_CONNECT %s 12345670 display join", dev_addr);	
        }
        else
        {
            DPRINT_ERR(WFA_ERR,"p2p_priv_p->confMethod=%d\n",p2p_priv_p->confMethod);	
            infoResp.status = STATUS_ERROR;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            return WFA_SUCCESS;
        }		
        wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);


        //determine the ROLE
        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);
            conn_cnt++;
            if (conn_cnt >= 60)
            {
                printf("%s...  fail timeout!\n", __func__);

                infoResp.status = STATUS_ERROR;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                return WFA_SUCCESS;
            }

            printf("%s... conn_cnt=%d\n", __func__, conn_cnt);
        } while(p2p_priv_p->bNegoDone == 0);
        conn_cnt =0 ;

        printf("%s...  role=%d\n", __func__, p2p_priv_p->role);
        //		memset(tmp_if_name, '\0', IFNAMSIZ);
        //		wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 

        //Get group ID
        if(p2p_priv_p->group_id[0] != '\0')
        {
            printf("[SIGMA]: group_id: %s\n",p2p_priv_p->group_id);
            strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
        }

        //mtk94097 Need Driver to get port from WFD_IE
        //Get Rtsp Port
        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
        system(strCmd); 					
        printf("system: %s\n",strCmd);

        sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep intended_addr | busybox cut -d'=' -f2 > /tmp/sigma_if_addr_wfd.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
        system(strCmd); 					
        printf("system: %s\n",strCmd);


        tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
            rtsp_port = DEFAULT_WFD_RTSP_PORT;
        }
        else
        {
            fgets(tmp_string, sizeof(tmp_string), tmpfd);
            //sample code: 00 0006 0111 ^022a^ 0064
            memcpy(tmp_rtspport,tmp_string+10,4);
            printf("tmp_rtspport: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_string);
            rtsp_port = strtol(tmp_rtspport, NULL , 16);			
            fclose(tmpfd);
        }
        sprintf(port_str, "%d", rtsp_port);

        //Get Interface Mac Address
        tmpfd = NULL;
        tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("[mtk_sigma]%s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
            memcpy(if_addr, dev_addr, sizeof(dev_addr));
            printf("[mtk_sigma]device mac: %s  \n",if_addr);
        }
        else
        {
            fgets(if_addr, sizeof(if_addr), tmpfd);
            printf("[mtk_sigma]interface mac: %s  \n",if_addr);
            fclose(tmpfd);
        }
        fprintf(stderr, "GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);


        extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
        if (p2p_priv_p->role == GO)
        {
            printf("[SIGMA] ERROR: we must be GC \n");

            infoResp.status = STATUS_ERROR;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
            return WFA_SUCCESS;
        }
        else
        {		
            fprintf(stderr, "[SIGMA] Starting DHCP client for role %d...\n", p2p_priv_p->role);
            if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
            {
                fprintf(stderr, "Fail to get DHCP Server IP address.\n");

                infoResp.status = STATUS_ERROR;
                strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "FALSE");
                strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                return WFA_SUCCESS;
            }
        }


        //Add workaround solution for Marvell TestBed Getting IP TOOOOO late
        printf("[SIGMA] workaround solution for Marvell TestBed Getting IP TOOOOO late\n");
        conn_cnt = 0;
        while(conn_cnt < 60 )
        {
            sprintf(strCmd, "busybox ping %s -c 4 | grep transmitted | busybox cut -d',' -f2 | busybox cut -d' ' -f2 > /tmp/sigma_ping_result_wfd.txt",peer_ip);
            system(strCmd); 					
            printf("system: %s\n",strCmd);

            //check ping result

            tmpfd = NULL;
            tmpfd = fopen("/tmp/sigma_ping_result_wfd.txt", "r+");
            if(tmpfd == NULL)
            {
                printf("[SIGMA] %s - fail to open /tmp/sigma_ping_before_wfd.txt\n", __func__);
                conn_cnt++;
                continue;
            }
            else
            {
                fgets(tmp_ping_result, sizeof(tmp_ping_result), tmpfd);
                if(tmp_ping_result[0] == 0 || tmp_ping_result[0] == '0' || tmp_ping_result[0] == '\0')
                {
                    printf("[SIGMA] conn_cnt: %d  \n",conn_cnt);
                    conn_cnt++;
                    fclose(tmpfd);
                    sleep(1);                    
                    continue;
                }
                else
                {
                    printf("[SIGMA] tmp_ping_result: %s	\n",tmp_ping_result);
                    fclose(tmpfd);
                    break;
                }
            }
        }

        //Get valid IP address, and ping peer side OK
        if(conn_cnt <5)
        {
            /* starting rtsp */
            conn_cnt = 0;
            while (conn_cnt < MAX_RTSP_RETRY)
            {
                sleep(10);
                fprintf(stderr, "[SIGMA] Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                {
                    /* successful */
                    fprintf(stderr, "[SIGMA] RTSP completed, session_id=[%s]\n", session_id);
                    result = 0;
                    break;
                }
                else
                {
                    /* failed */
                    fprintf(stderr, "[SIGMA] RTSP negotiation is failed\n");
                    result = -1;
                }
                conn_cnt++;
                if (conn_cnt < MAX_RTSP_RETRY)
                    fprintf(stderr, "[SIGMA] Retrying RTSP (retry=%d, max=%d)...\n", conn_cnt, MAX_RTSP_RETRY);
            }		
        }

        if (result != 0)
        {
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, "WFD Connection Failed!!!\n");
            fprintf(stderr, "------------------------\n");
            infoResp.status = STATUS_COMPLETE;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        }
        else
        {
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, " WFD Connection Success\n");
            fprintf(stderr, "------------------------\n");
            infoResp.status = STATUS_COMPLETE;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "CLIENT");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], group_id);
            if (strlen(session_id))
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], session_id);
            else
                strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
            sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
            system(cmdStr);
        }

    }
#endif

    wfaEncodeTLV(WFA_STA_CONNECT_GO_START_WFD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);	
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

/*
 * wfaStaGenerateEvent():
 */

int wfaStaGenerateEvent(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaGenEvent_t *staGenerateEvent= (caStaGenEvent_t *)caCmdBuf; //uncomment and use it
    caWfdStaGenEvent_t *wfdGenEvent;

    printf("\n Entry wfaStaGenerateEvent... ");

    fprintf(stderr, "\n Entry wfaStaGenerateEvent...\n");

    // Geneate the specified action and return with complete/error.
    //sarick note: do not fix to "program,WFD", to be more compitable to UCC */
    if (1)
    //if(staGenerateEvent->program == PROG_TYPE_WFD)
    {
        wfdGenEvent = &staGenerateEvent->wfdEvent;
        if(wfdGenEvent ->type == eUibcGen)
        {
#ifdef MTK_WFD_SIGMA
#define SEND_EVENT_COUNT    3
            extern int wfaMtkWfdCmd_rtspUibcGenEvent(int evtType);
            int evtType = 0;
            //int count = 0;
            fprintf(stderr, "%s: UibcGen, evt type = %d\n", __FUNCTION__, wfdGenEvent->wfdUibcEventType);

            if (wfdGenEvent->wfdUibcEventType == eSingleTouchEvent || 
                    wfdGenEvent->wfdUibcEventType == eMouseEvent ||
                    wfdGenEvent->wfdUibcEventType == eMultiTouchEvent ||
                    wfdGenEvent->wfdUibcEventType == eKeyBoardEvent)
            {
                evtType = wfdGenEvent->wfdUibcEventType;
            }
            else
            {
                fprintf(stderr, "%s: Unknown uibc event type(%d), treat as single touch event!\n", 
                        __FUNCTION__, wfdGenEvent->wfdUibcEventType);
                wfdGenEvent->wfdUibcEventType = eSingleTouchEvent;
            }

            //for (count = 0; count < SEND_EVENT_COUNT; count ++)
            //{
            if (evtType == 0)
                fprintf(stderr, "evtType: 0\n");

            if (wfaMtkWfdCmd_rtspUibcGenEvent(evtType) != 0)
                fprintf(stderr, "UibcGenEvent Failed!\n");
            else
                fprintf(stderr, "UibcGenEvent Success!\n");
            //    usleep(200000);
            //}
#endif
        } 
        else if(wfdGenEvent ->type == eUibcHid)
        {
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspUibcHidcEvent(int hidType);
            int hid_type;

            if (wfdGenEvent->wfdUibcEventType == eKeyBoardEvent)
                hid_type = eKeyBoardEvent;
            else if (wfdGenEvent->wfdUibcEventType == eMouseEvent)
                hid_type = eMouseEvent;
            else
            {
                fprintf(stderr, "%s: !!Unsupported uibc event type(%d), treat as HID Keyboard event!\n", 
                        __FUNCTION__, wfdGenEvent->wfdUibcEventType);
                hid_type = eKeyBoardEvent;
            }

            if (wfaMtkWfdCmd_rtspUibcHidcEvent(hid_type) != 0)
                fprintf(stderr, "UibcHidcEvent Failed!\n");
            else
                fprintf(stderr, "UibcHidcEvent Success!\n");
#endif		
        }		
        else if(wfdGenEvent ->type == eFrameSkip)
        {

        }
        else if(wfdGenEvent ->type == eI2cRead)
        {
        }
        else if(wfdGenEvent ->type == eI2cWrite)
        {
        }
        else if(wfdGenEvent ->type == eInputContent)
        {
        }
        else if(wfdGenEvent ->type == eIdrReq)
        {
#ifdef MTK_WFD_SIGMA
            extern int wfaMtkWfdCmd_rtspSendIdrReq(void);
            fprintf(stderr, "%s: GenerateEvent = SendIdrReq\n", __FUNCTION__);

            if (wfaMtkWfdCmd_rtspSendIdrReq() != 0)
                fprintf(stderr, "SendIdrReq Failed!\n");
#endif
        }		
    }

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GENERATE_EVENT_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}




/*
 * wfaStaReinvokeWfdSession():
 */

int wfaStaReinvokeWfdSession(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaReinvokeWfdSession_t *staReinvokeSession= (caStaReinvokeWfdSession_t *)caCmdBuf; //uncomment and use it

#ifdef MTK_CFG80211_SIGMA
    char buf[WPS_BUF_LENGTH] = {0};
    size_t buflen = 0;
    char cmd[WPS_CMD_LENGTH] = {0};
    char cmdStr[128];
#endif

    buflen = sizeof(buf) - 1;
    printf("\n Entry wfaStaReinvokeWfdSession... \n");
    // Reinvoke the WFD session by accepting the p2p invitation   or sending p2p invitation
#ifdef MTK_WFD_SIGMA
    if (staReinvokeSession->wfdInvitationAction == eInvitationSend)
    {
        unsigned char bNegotiate = 0;
        unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0}, group_id[WFA_P2P_GRP_ID_LEN] = {0};
        unsigned char strCmd[256] = {0};
        unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
        unsigned short rtsp_port = DEFAULT_WFD_RTSP_PORT;
        char session_id[64] = "";

        // connect the specified GO and then establish the wfd session	
        sprintf(cmdStr, "wpa_cli -i %s %s p2p_find", WFA_STAUT_IF_P2P, ctrl_if);
        system(cmdStr);
        system("sleep 10");

        printf("Send Invitation\n");
        strcpy(&dev_addr, staReinvokeSession->peer);
        strcpy(&group_id, staReinvokeSession->grpid);

        /* send invitation here */
        int polling_cnt = 0 ;
        int found_device = 0;
        do {
            sprintf(strCmd, "sleep 1");
            system(strCmd);
            snprintf(cmd, sizeof(cmd), "P2P_PEER %s",dev_addr);
            wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);

            printf("[SIGMA] %s===> %s\n", __FUNCTION__, buf);

            if(strncmp(buf, "FAIL", 4) == 0)
                printf("[SIGMA] %s===> wait the p2p_dev %s on %ds\n", __FUNCTION__, dev_addr, polling_cnt);
            else
            {
                found_device = 1;
                printf("[SIGMA] Peer discovered %s\n",dev_addr);
                break;
            }
            polling_cnt++;
        } while(polling_cnt <= 20 );

        if(found_device == 0)
            printf("[SIGMA] Deivce %s LOST ...\n",dev_addr);
        else
        {
            //get the NetWork ID first
            if(strlen(group_id) > 9)
            {
                int group_id_len = strlen(group_id);
                char *tmp_sub_string;
                char sub_ssid_string[16] = {0};
                int network_id = 0;

                tmp_sub_string = strstr(group_id,"DIRECT-");
                if(tmp_sub_string)
                {
                    printf(" [SIGMA] ori ===> %s\n",tmp_sub_string);			
                    memcpy(sub_ssid_string,tmp_sub_string,9);
                    printf(" [SIGMA] sub ===> %s\n",sub_ssid_string);	

                    sprintf(strCmd, "wpa_cli -i %s %s LIST_NETWORK | grep %s | busybox cut -f1 > /tmp/sigma_network_id.txt", WFA_STAUT_IF_P2P, ctrl_if, sub_ssid_string);
                    system(strCmd); 					
                    printf("system: %s\n",strCmd);


                    FILE *tmpfd;
                    tmpfd = fopen("/tmp/sigma_network_id.txt", "r+");
                    if(tmpfd == NULL)
                    {
                        printf("%s - fail to open /tmp/sigma_network_id.txt\n", __func__);
                    }
                    else
                    {
                        char tmp_string[64] = {0};
                        fgets(tmp_string, sizeof(tmp_string), tmpfd);
                        network_id = strtol(tmp_string, NULL , 10);														
                        printf("[SIGMA] network_ID :%d\n",network_id);
                        fclose(tmpfd);							
                    }
                    if (p2p_priv_p->freq)
                        snprintf(cmd, sizeof(cmd), "P2P_INVITE persistent=%d peer=%s freq=%d",network_id, dev_addr, p2p_priv_p->freq);
                    else
                        snprintf(cmd, sizeof(cmd), "P2P_INVITE persistent=%d peer=%s",network_id, dev_addr);
                    wpa_start_cmd(p2p_priv_p->if_name, p2p_priv_p->cmd_sock, cmd, buf, buflen, 1);
                    printf(" [SIGMA] %s===> %s\n", __FUNCTION__, buf);			
                }
                else
                {
                    printf("tmp_sub_string  INVALID \n");
                }
            }
        }

        wfdMtkWfdCheckP2pConnResult(&bNegotiate, dev_addr, sizeof(dev_addr));

        if (bNegotiate == P2P_DEVICE)
        {
            printf("[SIGMA] I'm P2P Device!\n");
            infoResp.status = STATUS_COMPLETE;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        }
        else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
        {
            /* session not available */
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, " WFD Connection Failed (Session Not Available)\n");
            fprintf(stderr, "------------------------\n");
            infoResp.status = STATUS_COMPLETE;
            strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
            strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
            strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
        }
        else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
        {
            char Rule[10] = {0};

            if (bNegotiate == P2P_GO)
                sprintf(&Rule, "%s", "GO");
            else
                sprintf(&Rule, "%s", "CLIENT");
            printf("I'm P2P %s!\n", Rule);



            if(p2p_priv_p->group_id[0] != '\0')
            {
                printf("[SIGMA]: group_id: %s\n",p2p_priv_p->group_id);
                strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
            }
            char tmp_rtspport[128] = {0};
            char tmp_string[128] = {0};

            //mtk94097 Need Driver to get port from WFD_IE
            sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep wfd_subelems | busybox cut -d'=' -f2 > /tmp/sigma_rtsp_port.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
            system(strCmd); 					
            printf("system: %s\n",strCmd);

            sprintf(strCmd, "wpa_cli -i %s %s p2p_peer %s | grep intended_addr | busybox cut -d'=' -f2 > /tmp/sigma_if_addr_wfd.txt", WFA_STAUT_IF_P2P, ctrl_if, dev_addr);
            system(strCmd); 					
            printf("system: %s\n",strCmd);


            FILE *tmpfd;
            tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
            if(tmpfd == NULL)
            {
                printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
                rtsp_port = DEFAULT_WFD_RTSP_PORT;
            }
            else
            {
                fgets(tmp_string, sizeof(tmp_string), tmpfd);
                //sample code: 00 0006 0111 ^022a^ 0064
                memcpy(tmp_rtspport,tmp_string+10,4);
                printf("tmp_rtspport: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_string);
                rtsp_port = strtol(tmp_rtspport, NULL , 16);

                fclose(tmpfd);
            }

            tmpfd = NULL;
            tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
            if(tmpfd == NULL)
            {
                printf("[SIGMA]%s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
                memcpy(if_addr, dev_addr, sizeof(dev_addr));
                printf("[SIGMA]device mac: %s  \n",if_addr);
            }
            else
            {
                fgets(if_addr, sizeof(if_addr), tmpfd);
                printf("[SIGMA]interface mac: %s  \n",if_addr);
                if (!strncmp(if_addr, "00:00:00:00:00:00", WFA_P2P_DEVID_LEN - 1))
                    memcpy(if_addr, dev_addr, sizeof(dev_addr));
                printf("[SIGMA]interface mac: %s  \n",if_addr);
                fclose(tmpfd);
            }


            fprintf(stderr, "[SIGMA] GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);
            /* starting dhcp server and getting peer's ip */
            {
                char peer_ip[24] = "";
                int result = 0;
                char port_str[16] = "";
                int rtsp_retry = 0;
#ifdef MTK_WFD_SIGMA
                extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
#endif
                if (rtsp_port == 0)
                {
                    fprintf(stderr, "[SIGMA] ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                    rtsp_port = DEFAULT_WFD_RTSP_PORT;
                }
                sprintf(port_str, "%d", rtsp_port);
                if (bNegotiate == P2P_GO)
                {
                    fprintf(stderr, "[SIGMA] Starting DHCP server...\n");
#ifdef MTK_WFD_SIGMA
                    if (wfaMtkWfdStartDhcpServer(if_addr, peer_ip, sizeof(peer_ip)) != 0)
                    {
                        fprintf(stderr, "[SIGMA] Fail to get DHCP Client IP address.\n");
                        result = -1;
                    }
#endif 
                }
                else if (bNegotiate == P2P_CLIENT)
                {
                    fprintf(stderr, "[SIGMA] Starting DHCP client...\n");
#ifdef MTK_WFD_SIGMA
                    if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
                    {
                        fprintf(stderr, "[SIGMA] Fail to get DHCP Server IP address.\n");
                        result = -1;
                    }
#endif 
                }
#ifdef MTK_WFD_SIGMA
                if (result == 0)
                {
                    /* starting rtsp */
                    while (rtsp_retry < MAX_RTSP_RETRY)
                    {
                        sleep(10);
                        fprintf(stderr, "[SIGMA] Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                        if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                        {
                            /* successful */
                            fprintf(stderr, "[SIGMA] RTSP completed, session_id=[%s]\n", session_id);
                            result = 0;
                            break;
                        }
                        else
                        {
                            /* failed */
                            fprintf(stderr, "[SIGMA] RTSP negotiation is failed\n");
                            result = -1;
                        }
                        rtsp_retry ++;
                        if (rtsp_retry < MAX_RTSP_RETRY)
                            fprintf(stderr, "[SIGMA] Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                    }
                }
#endif
                if (result != 0)
                {
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, "WFD Connection Failed!!!\n");
                    fprintf(stderr, "------------------------\n");
                    infoResp.status = STATUS_COMPLETE;
                    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], "NULL");
                    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
                    strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "-1");
                }
                else
                {
                    //sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=0", staStartWfdConn->intf);
                    //system(cmdStr);
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, " WFD Connection Success\n");
                    fprintf(stderr, "------------------------\n");
                    infoResp.status = STATUS_COMPLETE;
                    strcpy(&infoResp.cmdru.wfdConnInfo.result[0], Rule);
                    strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], group_id);
                    if (strlen(session_id))
                        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], session_id);
                    else
                        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], "1234567890");
                    if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)
                    {
                        sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
                        system(cmdStr);
                    }
                }
            }
        }
    }
    else
    {
        printf("Accept Invitation\n");
        wfdReinvokePeerIsInitiator = 1;

        //try to add Join
        unsigned char strCmd[128] = {0};
        sprintf(strCmd, "wpa_cli -i %s %s p2p_find", WFA_STAUT_IF_P2P, ctrl_if);
        system(strCmd);
        printf(" System : %s \n",strCmd);


        infoResp.status = STATUS_COMPLETE/*STATUS_RUNNING*/;
        strcpy(&infoResp.cmdru.wfdConnInfo.result[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.p2pGrpId[0], " ");
        strcpy(&infoResp.cmdru.wfdConnInfo.wfdSessionId[0], " ");
    }
#else /* !MTK_WFD_SIGMA */
    infoResp.status = STATUS_COMPLETE;
#endif /* MTK_WFD_SIGMA */
    wfaEncodeTLV(WFA_STA_REINVOKE_WFD_SESSION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);	
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


int wfaStaGetParameter(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaGetParameter_t *staGetParam= (caStaGetParameter_t *)caCmdBuf; //uncomment and use it
    char DevList[256];

    caStaGetParameterResp_t *paramList = &infoResp.cmdru.getParamValue;

    printf("\n Entry wfaStaGetParameter... ");

    if(staGetParam->getParamValue == eDiscoveredDevList )
    {
        // Get the discovered devices, make space seperated list and return, check list is not bigger than 128 bytes.
        paramList->getParamType = eDiscoveredDevList;
        unsigned char strCmd[128] = {0};
        sprintf(strCmd,"wpa_cli -i %s %s p2p_peers > /tmp/sigma_dev_list.txt", WFA_STAUT_IF_P2P, ctrl_if);
        system(strCmd);
        printf("system: %s \n",strCmd);
        FILE *tmpfd;
        tmpfd = fopen("/tmp/sigma_dev_list.txt", "r+");
        if(tmpfd == NULL)
        {
            printf("%s - fail to open /tmp/sigma_dev_list.txt\n", __func__);
            memcpy(DevList,"AA:BB:CC:DD:EE:FF",18);
        }
        else
        {
            char tmp_string[128] = {0};
            memset(DevList, 0 ,sizeof(DevList));
            for(;;)
            {
                memset(tmp_string, sizeof(tmp_string),0);
                if(fgets(tmp_string, sizeof(tmp_string), tmpfd) == NULL)
                    break;
                tmp_string[strlen(tmp_string)-1] = ' ';
                strcat(DevList,tmp_string);				
                printf(" [sigma] ....... <%s> \n",tmp_string);				
            }
        }
    printf(" [sigma] DevList: %s \n",DevList);	

    if (tmpfd)
        fclose(tmpfd);
    //strcpy(&paramList->devList, "11:22:33:44:55:66 22:33:44:55:66:77 33:44:55:66:77:88");
    strcpy(&paramList->devList, DevList);		
    }
	
    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_GET_PARAMETER_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);	
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


int wfaStaNfcAction(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	caStaNfcAction_t *getStaNfcAction = (caStaNfcAction_t *)caCmdBuf;  //uncomment and use it
	
    printf("\n Entry wfaStaNfcAction... ");

	if(getStaNfcAction->nfcOperation == eNfcHandOver)
	{
		printf("\n NfcAction - HandOver... ");
	
	}
	else if(getStaNfcAction->nfcOperation == eNfcReadTag)
	{
		printf("\n NfcAction - Read Tag... ");

	}
	else if(getStaNfcAction->nfcOperation == eNfcWriteSelect)
	{
		printf("\n NfcAction - Write Select... ");
	
	}
	else if(getStaNfcAction->nfcOperation == eNfcWriteConfig)
	{
		printf("\n NfcAction - Write Config... ");
	
	}
	else if(getStaNfcAction->nfcOperation == eNfcWritePasswd)
	{
		printf("\n NfcAction - Write Password... ");
	
	}
	else if(getStaNfcAction->nfcOperation == eNfcWpsHandOver)
	{
		printf("\n NfcAction - WPS Handover... ");
	
	}
	
	 // Fetch the device mode and put in	 infoResp->cmdru.p2presult 
	 //strcpy(infoResp->cmdru.p2presult, "GO");
	
	 // Fetch the device grp id and put in	 infoResp->cmdru.grpid 
	 //strcpy(infoResp->cmdru.grpid, "AA:BB:CC:DD:EE:FF_DIRECT-SSID");
	 
	 strcpy(infoResp.cmdru.staNfcAction.result, "CLIENT");
	 strcpy(infoResp.cmdru.staNfcAction.grpId, "AA:BB:CC:DD:EE:FF_DIRECT-SSID");
	 infoResp.cmdru.staNfcAction.peerRole = 1;
	
	


	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_NFC_ACTION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}

int wfaStaExecAction(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	caStaExecAction_t *staExecAction = (caStaExecAction_t *)caCmdBuf;  //comment if not used
	
	 printf("\n Entry wfaStaExecAction... ");

	if(staExecAction->prog == PROG_TYPE_NAN)
	{
		// Perform necessary configurations and actions
		// return the MAC address conditionally as per CAPI specification
	}
	
	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_EXEC_ACTION_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}

int wfaStaInvokeCommand(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	caStaInvokeCmd_t *staInvokeCmd = (caStaInvokeCmd_t *)caCmdBuf;  //uncomment and use it
	
	 printf("\n Entry wfaStaInvokeCommand... ");


	 // based on the command type , invoke API or complete the required procedures
	 // return the  defined parameters based on the command that is received ( example response below)

	if(staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeAdvt )
	{
		 infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeAdvt;
		 infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.numServInfo = 1;
		 strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].servName,"org.wi-fi.wfds.send.rx");
		 infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].advtID = 0x0000f;
		 strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.advRsp.servAdvInfo[0].serviceMac,"ab:cd:ef:gh:ij:kl");
	}
	else if (staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeSeek)
	{
		infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeSeek;
		infoResp.cmdru.staInvokeCmd.invokeCmdResp.seekRsp.searchID = 0x000ff;	
	}
	else if (staInvokeCmd->cmdType == ePrimitiveCmdType && staInvokeCmd->InvokeCmds.primtiveType.PrimType == eCmdPrimTypeConnSession)
	{
		infoResp.cmdru.staInvokeCmd.invokeCmdRspType = eCmdPrimTypeConnSession;
		infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.sessionID = 0x000ff;  
		strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.result,"GO");
		strcpy(infoResp.cmdru.staInvokeCmd.invokeCmdResp.connSessResp.grpId,"DIRECT-AB WFADUT");
	
	}	
	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_INVOKE_CMD_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}


int wfaStaManageService(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	//caStaMngServ_t *staMngServ = (caStaMngServ_t *)caCmdBuf;  //uncomment and use it
	
	 printf("\n Entry wfaStaManageService... ");

	// based on the manage service type , invoke API's or complete the required procedures
	// return the  defined parameters based on the command that is received ( example response below)
	strcpy(infoResp.cmdru.staManageServ.result, "CLIENT");
	strcpy(infoResp.cmdru.staManageServ.grpId, "AA:BB:CC:DD:EE:FF_DIRECT-SSID");
    infoResp.cmdru.staManageServ.sessionID = 0x000ff;

	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_MANAGE_SERVICE_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}


	
int wfaStaGetEvents(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	caStaGetEvents_t *staGetEvents = (caStaGetEvents_t *)caCmdBuf;  //uncomment and use it
	
	 printf("\n Entry wfaStaGetEvents... ");
	 
	 if(staGetEvents->program == PROG_TYPE_NAN)
	{ 
		// Get all the events from the Log file or stored events
		// return the  received/recorded event details - eventName, remoteInstanceID, localInstanceID, mac
	}

	// Get all the event from the Log file or stored events
	// return the  received/recorded events as space seperated list   ( example response below)
	strcpy(infoResp.cmdru.staGetEvents.result, "SearchResult SearchTerminated AdvertiseStatus SessionRequest ConnectStatus SessionStatus PortStatus");
	
	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_GET_EVENTS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}

int wfaStaGetEventDetails(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{

	dutCmdResponse_t infoResp;
	caStaGetEventDetails_t *getStaGetEventDetails = (caStaMngServ_t *)caCmdBuf;  //uncomment and use it
	
	 printf("\n Entry wfaStaGetEventDetails... ");


	 // based on the Requested Event type
	 // return the latest corresponding evnet detailed parameters  ( example response below)

	if(getStaGetEventDetails->eventId== eSearchResult )
	{
		// fetch from log file or event history for the search result event and return the parameters
		infoResp.cmdru.staGetEventDetails.eventID= eSearchResult;

		infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.searchID = 0x00abcd;
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceMac,"ab:cd:ef:gh:ij:kl");
		infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.advID = 0x00dcba;
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceName,"org.wi-fi.wfds.send.rx");

		infoResp.cmdru.staGetEventDetails.getEventDetails.searchResult.serviceStatus = eServiceAvilable;
	}
	else if (getStaGetEventDetails->eventId == eSearchTerminated)
	{		// fetch from log file or event history for the search terminated event and return the parameters
		infoResp.cmdru.staGetEventDetails.eventID= eSearchTerminated;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.searchTerminated.searchID = 0x00abcd;
	}
	else if (getStaGetEventDetails->eventId == eAdvertiseStatus)
	{// fetch from log file or event history for the Advertise Status event and return the parameters
		infoResp.cmdru.staGetEventDetails.eventID= eAdvertiseStatus;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.advStatus.advID = 0x00dcba;

		infoResp.cmdru.staGetEventDetails.getEventDetails.advStatus.status = eAdvertised;	
	}	
	else if (getStaGetEventDetails->eventId == eSessionRequest)
	{// fetch from log file or event history for the session request event and return the parameters
		infoResp.cmdru.staGetEventDetails.eventID= eSessionRequest;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.advID = 0x00dcba;
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.sessionMac,"ab:cd:ef:gh:ij:kl");
		infoResp.cmdru.staGetEventDetails.getEventDetails.sessionReq.sessionID = 0x00baba;	
	}	
	else if (getStaGetEventDetails->eventId ==eSessionStatus )
	{// fetch from log file or event history for the session status event and return the parameters
		infoResp.cmdru.staGetEventDetails.eventID= eSessionStatus;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.sessionID = 0x00baba;	
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
		infoResp.cmdru.staGetEventDetails.getEventDetails.sessionStatus.state = eSessionStateOpen;	
	}	
	else if (getStaGetEventDetails->eventId == eConnectStatus)
	{
		infoResp.cmdru.staGetEventDetails.eventID= eConnectStatus;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.sessionID = 0x00baba;	
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
		infoResp.cmdru.staGetEventDetails.getEventDetails.connStatus.status = eGroupFormationComplete;	
	
	}	
	else if (getStaGetEventDetails->eventId == ePortStatus)
	{
		infoResp.cmdru.staGetEventDetails.eventID= ePortStatus;	
		infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.sessionID = 0x00baba;	
		strcpy(infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.sessionMac,"ab:cd:ef:gh:ij:kl");
		infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.port = 1009;
		infoResp.cmdru.staGetEventDetails.getEventDetails.portStatus.status = eLocalPortAllowed;	
	}	



	infoResp.status = STATUS_COMPLETE;
	wfaEncodeTLV(WFA_STA_GET_EVENT_DETAILS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);
	
   return WFA_SUCCESS;
}
#ifdef MTK_11N_AP_SIGMA
/*
* use popen() to do shell cmd
* with fread() get the desired msg
*/
int mtk_run_shell_cmd(char *cmd, char *reply,
			size_t buflen,
			size_t *reply_len,
			void *ret,
			void (*cb)(char *msg, size_t len, void *ret))
{
	FILE *pp;
	printf("%s running cmd: %s\n", __func__, cmd);
	pp = popen(cmd, "r"); /* read is enough */
	if (!pp) {
		printf("%s cmd: %s failed\n", __func__, cmd);
		return -1;
	}
	if (reply && reply_len) {
		memset(reply, 0, buflen);
		*reply_len = fread(reply, sizeof(char), buflen, pp);
		printf("popen reply: %s\n", reply);
	}
	if (cb)
		cb(reply, *reply_len, ret);

	pclose(pp);
	return 0;
}

/* tokenize according to the new line
* restore whole line to the argv[i]
* return:
* number of lines
*/
static int tokenize_line(char *cmd, char *argv[], int len)
{
	char *pos;
	char *start;
	int argc = 0;

	start = pos = cmd;
	for (;;) {
		argv[argc] = pos;
		argc++;
		while (*pos != '\n') {
			pos++;
			if (pos - start >= len)
				break;
		}
		if (*pos == '\n') {
			*pos++ = '\0';
			if (pos - start >= len)
				break;
		}
	}

	return argc;
}

int _wfaApPrintRes(char *msg, size_t len, void *ret)
{
    int i = 0;
    if (!msg || !len)
    {
        printf("msg:%p, len:%d", msg, len);
        i = -1;
    } else {
        printf("Response msg:%s\n", msg);
        i = 0;
    }
    if (ret)
        *(int *)ret = i;
    return i;
}

int _wfaApHandleWmm(char *msg, size_t len, void *ret)
{
    int i = 0;
    if (!msg || !len)
    {
        printf("msg:%p, len:%d\n", msg, len);
        printf("result:wmm disabled\n");
        i = 0;
    } else {
        printf("Response msg:%s\n", msg);
        printf("result:wmm enabled\n");
        i = 1;
    }
    if (ret)
        *(int *)ret = i;
    return i;
}

int _wfaApHandleWmmPS(char *msg, size_t len, void *ret)
{
    int i = 0;
    if (!msg || !len)
    {
        printf("msg:%p, len:%d\n", msg, len);
        printf("result:wmmps disabled\n");
        i = 0;
    } else {
        printf("result:%s, wmmps enabled\n", msg);
        i = 1;
    }
    if (ret)
        *(int *)ret = i;
    return i;
}

int wfaApResetDefault(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caApCommonResp_t infoResp;
    printf("Entering wfaApResetDefault ...\n");

    printf("  ==== Kill hostapd process ====\n");
    sprintf(gCmdStr, "busybox killall hostapd");
    system(gCmdStr);
    printf("  system: %s \n",gCmdStr);

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_AP_RESET_DEFAULT_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

int wfaApSetWireless(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caApCommonResp_t infoResp;
    printf("Entering wfaApSetWireless ...\n");
    caApSetWireless_t *apSetWireless = (caApSetWireless_t *) caCmdBuf;

    do {
        if (apSetWireless->ssid[0] == INVALID_STR) {
            printf("ssid invalid\n");
        } else {
            /*Config ssid*/
	    sprintf(gCmdStr, "busybox sed -i /^ssid/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
	    sprintf(gCmdStr, "busybox sed -i \"$ a ssid=%s\" %s", apSetWireless->ssid, WFA_AP_CONF_PATH);
            system(gCmdStr);
	    printf(gCmdStr);

            memset(gSSID, 0, sizeof (gSSID));
            sprintf(gSSID, "%s", apSetWireless->ssid);
        }
        if (apSetWireless->dutName[0] == INVALID_STR) {
            printf("dut name invalid\n");
            infoResp.status = STATUS_INVALID;
            break;
        }

        /*Config device_name*/
	sprintf(gCmdStr, "busybox sed -i /^device_name/d %s", WFA_AP_CONF_PATH);
	system(gCmdStr);
	printf(gCmdStr);
        sprintf(gCmdStr, "busybox sed -i \"$ a device_name=%s\" %s", apSetWireless->dutName, WFA_AP_CONF_PATH);
	system(gCmdStr);
	printf(gCmdStr);

        if (apSetWireless->channel != INVALID_VALUE) {
            /*config channel*/
            printf("channel:%d\n", apSetWireless->channel);
	    sprintf(gCmdStr, "busybox sed -i /^channel/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
            sprintf(gCmdStr, "busybox sed -i \"$ a channel=%d\" %s", apSetWireless->channel, WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
        }

        if (apSetWireless->width != INVALID_VALUE) {
            /*bandwidth*/
            printf("width:%d\n", apSetWireless->width);
        }

        if (apSetWireless->mode[0] != INVALID_STR) {
            /*hw mode 11g, 11ng, 11a, 11b, 11na, 11ac*/
            printf("mode:%s\n", apSetWireless->mode);
	    sprintf(gCmdStr, "busybox sed -i /^hw_mode/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
	    sprintf(gCmdStr, "busybox sed -i /^ieee80211n/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
	    sprintf(gCmdStr, "busybox sed -i /^ieee80211ac/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
	    sprintf(gCmdStr, "busybox sed -i /^vht_oper_chwidth/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
            if (0 == strcmp(apSetWireless->mode, "11ac")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "a", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

		sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211ac=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

		sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_chwidth=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else if (0 == strcmp(apSetWireless->mode, "11a")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "a", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "0", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else if (0 == strcmp(apSetWireless->mode, "11na")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "a", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else if (0 == strcmp(apSetWireless->mode, "11b")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "b", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "0", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else if (0 == strcmp(apSetWireless->mode, "11g")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "g", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "0", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else if (0 == strcmp(apSetWireless->mode, "11ng")) {
                sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "g", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            } else {
                /*apply 11na setting by default*/
		sprintf(gCmdStr, "busybox sed -i \"$ a hw_mode=%s\" %s", "a", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);

                sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211n=%s\" %s", "1", WFA_AP_CONF_PATH);
                system(gCmdStr);
                printf(gCmdStr);
            }
        }

	if ((0 == strcmp(apSetWireless->mode, "11ac")) && (apSetWireless->channel != INVALID_VALUE)) {
		sprintf(gCmdStr, "busybox sed -i /^vht_oper_centr_freq_seg0_idx/d %s", WFA_AP_CONF_PATH);
		system(gCmdStr);
		printf(gCmdStr);
		sprintf(gCmdStr, "busybox sed -i /^ht_capab/d %s", WFA_AP_CONF_PATH);
		system(gCmdStr);
		printf(gCmdStr);
		if (apSetWireless->channel >= 36 || apSetWireless->channel <= 48) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "42", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if (apSetWireless->channel >= 52 || apSetWireless->channel <= 64) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "58", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if (apSetWireless->channel >= 100 || apSetWireless->channel <= 112) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "106", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if (apSetWireless->channel >= 116 || apSetWireless->channel <= 128) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "122", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if (apSetWireless->channel >= 132 || apSetWireless->channel <= 144) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "138", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if (apSetWireless->channel >= 149 || apSetWireless->channel <= 161) {
			sprintf(gCmdStr, "busybox sed -i \"$ a vht_oper_centr_freq_seg0_idx=%s\" %s", "155", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		}
		if ((apSetWireless->channel >= 36) || (apSetWireless->channel <= 144)) {
			/* ht_capab=[HT40-] (apSetWireless->channel%8) == 0
			 * channel: 40/48/56/64/104/112/120/128/136/144
			 * ht_capab=[HT40+] (apSetWireless->channel%8) == 4
			 * channel: 36/44/52/60/100/108/116/124/132/140
			 */
			if ((apSetWireless->channel%8) == 0) {
				sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=%s\" %s", "[HT40-]", WFA_AP_CONF_PATH);
				system(gCmdStr);
				printf(gCmdStr);
			} else if ((apSetWireless->channel%8) == 4) {
				sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=%s\" %s", "[HT40+]", WFA_AP_CONF_PATH);
				system(gCmdStr);
				printf(gCmdStr);
			}
		} else if ((apSetWireless->channel == 153) || (apSetWireless->channel == 161)) {
			sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=%s\" %s", "[HT40-]", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		} else if ((apSetWireless->channel == 149) || (apSetWireless->channel == 157)) {
			sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=%s\" %s", "[HT40+]", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);
		}
	}
        if (apSetWireless->rts != INVALID_VALUE) {
            /**/
            printf("rts:%d\n", apSetWireless->rts);
            sprintf(gCmdStr, "busybox sed -i /^rts_threshold/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            sprintf(gCmdStr, "busybox sed -i \"$ a rts_threshold=%d\" %s", apSetWireless->rts, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

        if (apSetWireless->fragement != INVALID_VALUE) {
            /**/
            printf("fragement:%d\n", apSetWireless->fragement);
	    sprintf(gCmdStr, "busybox sed -i /^fragm_threshold/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            sprintf(gCmdStr, "busybox sed -i \"$ a fragm_threshold=%d\" %s", apSetWireless->fragement, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

        if (apSetWireless->beaconInterval != INVALID_VALUE) {
            /**/
            printf("beaconInterval:%d\n", apSetWireless->beaconInterval);
	    sprintf(gCmdStr, "busybox sed -i /^beacon_int/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            sprintf(gCmdStr, "busybox sed -i \"$ a beacon_int=%d\" %s", apSetWireless->beaconInterval, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

        if (apSetWireless->ampdu != INVALID_VALUE) {
            /**/
            printf("ampdu:%d\n", apSetWireless->ampdu);
        }

        if (apSetWireless->amsdu != INVALID_VALUE) {
            /**/
            printf("amsdu:%d\n", apSetWireless->amsdu);
        }

        if (apSetWireless->spatialTxStream[0] != INVALID_STR) {
            /**/
            printf("spatialTxStream:%s\n", apSetWireless->spatialTxStream);
        }

        if (apSetWireless->spatialRxStream[0] != INVALID_STR) {
            /**/
            printf("spatialRxStream:%s\n", apSetWireless->spatialRxStream);
        }

        if (apSetWireless->greenfield != INVALID_VALUE) {
            /**/
            printf("greenfield:%d\n", apSetWireless->greenfield);
	    sprintf(gCmdStr, "busybox sed -i \"/^ht_capab\\\=\\\[GF]/d\" %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            if (apSetWireless->greenfield) {
            	sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=[GF]\" %s", WFA_AP_CONF_PATH);
            	system(gCmdStr);
            	printf(gCmdStr);
            }
        }

        if (apSetWireless->mcs32 != INVALID_VALUE) {
            /**/
            printf("mcs32:%d\n", apSetWireless->mcs32);
        }

        if (apSetWireless->mcsMandatory != INVALID_VALUE) {
            /**/
            printf("mcsMandatory:%d\n", apSetWireless->mcsMandatory);
        }

        if (apSetWireless->mcsSupported != INVALID_VALUE) {
            /**/
            printf("mcsSupported:%d\n", apSetWireless->mcsSupported);
        }

        if (apSetWireless->stbcTx != INVALID_VALUE) {
            /**/
            printf("stbcTx:%d\n", apSetWireless->stbcTx);
        }

        if (apSetWireless->dtim != INVALID_VALUE) {
            /**/
            printf("dtim:%d\n", apSetWireless->dtim);
	    sprintf(gCmdStr, "busybox sed -i /^dtim_period/d  %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

	    sprintf(gCmdStr, "busybox sed -i \"$ a dtim_period=%d\" %s", apSetWireless->dtim, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

        if (apSetWireless->sgi20 != INVALID_VALUE) {
            printf("sgi20:%d\n", apSetWireless->sgi20);
	    sprintf(gCmdStr, "busybox sed -i \"/^ht_capab\\\=\\\[SHORT-GI-20]/d\"  %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            if (apSetWireless->sgi20) {
	    	sprintf(gCmdStr, "busybox sed -i \"$ a ht_capab=[SHORT\-GI\-20]\"  %s", WFA_AP_CONF_PATH);
	    	system(gCmdStr);
	    	printf(gCmdStr);
            }
        }

	sprintf(gCmdStr, "busybox sed -i /^wmm_enabled/d %s", WFA_AP_CONF_PATH);
        system(gCmdStr);
        printf(gCmdStr);
        if (apSetWireless->wme != 0) {
            printf("wmm:%d\n", apSetWireless->wme);
	    sprintf(gCmdStr, "busybox sed -i \"$ a wmm_enabled=%d\" %s", apSetWireless->wme, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

	sprintf(gCmdStr, "busybox sed -i /^uapsd_advertisement_enabled/d %s", WFA_AP_CONF_PATH);
        system(gCmdStr);
        printf(gCmdStr);
        if (apSetWireless->wmmps != 0) {
            printf("wmmps:%d\n", apSetWireless->wmmps);
	    sprintf(gCmdStr, "busybox sed -i \"$ a uapsd_advertisement_enabled=%d\" %s", apSetWireless->wmmps, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        }

        infoResp.status = STATUS_COMPLETE;
    } while (0);

    wfaEncodeTLV(WFA_AP_SET_WIRELESS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

int _wfaApParsePsk (char *msg, size_t len, void *ret)
{
    int iRet = 0;
    int i = 0;
    int j = 0;
    int argc = 0;
    char* buf = msg;
    char *argv[5];
    printf("Entering _wfaApParsePsk ...\n");
    do {
        if (msg == NULL) {
            printf("msg is null\n");
            iRet = -1;
            break;
        }
        iRet = -2;
        memset(gPsk, 0, sizeof(gPsk));
        printf("msg:%s\n", msg);
        argc = tokenize_line(buf, argv, len);

        for (i = 0; i < argc; i++) {
            for (j = 0; j < strlen(argv[i]); j++) {
                argv[i]++;
                if (argv[i] == ' ')
                    continue;
                if (0 == strncmp (argv[i], "psk=", strlen("psk="))) {
                    strcpy (gPsk, "wpa_psk=");
                    strcpy (gPsk + strlen("wpa_psk="), argv[i] + strlen("psk="));
                    printf("psk found:%s\n", gPsk);
                    iRet = 0;
                }
                break;
            }
            if (gPsk[0] != 0)
                break;
        }
    } while (0);

    if (ret)
        *(int *)ret = iRet;
    return iRet;
}

int wfaApSetSecurity(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caApCommonResp_t infoResp;
    caApSetSecurity_t *apSetSecurity = (caApSetSecurity_t *) caCmdBuf;
    int i;
    printf("Entering wfaApSetSecurity ...\n");

    do {
        if (apSetSecurity->dutName[0] == INVALID_STR) {
            printf("dut name invalid\n");
            infoResp.status = STATUS_INVALID;
            break;
        }

        if (apSetSecurity->keyMgmtType[0] == INVALID_STR && apSetSecurity->encpType[0] == INVALID_STR) {
            printf("key mgmt type invalid\n");
            infoResp.status = STATUS_INVALID;
            break;
        }

        if (0 == strncmp(apSetSecurity->keyMgmtType, "NONE", strlen ("NONE"))) {
            /*remove wpa*/
	    sprintf(gCmdStr, "busybox sed -i /wpa/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove rsn_pairwise*/
	    sprintf(gCmdStr, "busybox sed -i /rsn_pairwise/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wpa_psk*/
	    sprintf(gCmdStr, "busybox sed -i /wpa_psk/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_default_key*/
            sprintf(gCmdStr, "busybox sed -i /wep_default_key/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_key0*/
            sprintf(gCmdStr, "busybox sed -i /wep_key0/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        } else if ((0 == strncmp(apSetSecurity->keyMgmtType, "WPA2-PSK", strlen ("WPA2-PSK"))) ||
                (0 == strncmp(apSetSecurity->keyMgmtType, "WPA2-PSK-Mixed", strlen ("WPA2-PSK-Mixed")))) {
            if (apSetSecurity->secu.passphrase[0] == INVALID_STR) {
                printf("key mgmt is wpa2-psk, but passphase is empty\n");
                infoResp.status = STATUS_INVALID;
                break;
            }
            /*remove wpa*/
	    sprintf(gCmdStr, "busybox sed -i /wpa/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove rsn_pairwise*/
            sprintf(gCmdStr, "busybox sed -i /rsn_pairwise/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wpa_psk*/
	    sprintf(gCmdStr, "busybox sed -i /^wpa_psk=/d %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_default_key*/
            sprintf(gCmdStr, "busybox sed -i /wep_default_key/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_key0*/
            sprintf(gCmdStr, "busybox sed -i /wep_key0/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            /*config wpa*/
	    sprintf(gCmdStr, "busybox sed -i \"$ a wpa=2\" %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*config rsn_pairwise*/
	    sprintf(gCmdStr, "busybox sed -i \"$ a rsn_pairwise=CCMP\" %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*config wpa_psk*/
            /*mtk_run_shell_cmd("sed -i \"$ a wpa_psk=7d7d60d387163b64c18cb17cf6522755b51ae28f0fd5c0f7fb5b7a22c4815c51\" /data/misc/wifi/hostapd.conf", NULL, 0, NULL, NULL, NULL);*/
	    sprintf(gCmdStr, "busybox sed -i \"$ a rsn_pairwise=CCMP\" %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            sprintf(gCmdStr, "wpa_passphrase \"%s\" \"%s\"", gSSID, apSetSecurity->secu.passphrase);
	    mtk_run_shell_cmd(gCmdStr, gWpaBuff, sizeof(gWpaBuff), &i, NULL, _wfaApParsePsk);
            printf("gPsk:%s\n", gPsk);
	    sprintf(gCmdStr, "busybox sed -i \"$ a %s\" %s", gPsk, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        } else if (0 == strncmp(apSetSecurity->encpType, "WEP", strlen ("WEP"))) {
            /*remove wpa*/
            sprintf(gCmdStr, "busybox sed -i /wpa/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove rsn_pairwise*/
            sprintf(gCmdStr, "busybox sed -i /rsn_pairwise/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wpa_psk*/
            sprintf(gCmdStr, "busybox sed -i /wpa_psk/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_default_key*/
            sprintf(gCmdStr, "busybox sed -i /wep_default_key/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
            /*remove wep_key0*/
            sprintf(gCmdStr, "busybox sed -i /wep_key0/d %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            /*config wep_default_key*/
            sprintf(gCmdStr, "busybox sed -i \"$ a wep_default_key=0\" %s", WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);

            /*config wep_key0*/
            sprintf(gCmdStr, "busybox sed -i \"$ a wep_key0=%s\" %s", apSetSecurity->secu.wepkey, WFA_AP_CONF_PATH);
            system(gCmdStr);
            printf(gCmdStr);
        } else if (0 == strncmp(apSetSecurity->keyMgmtType, "WPA2-Ent", strlen ("WPA2-Ent"))) {
            infoResp.status = STATUS_INVALID;
            break;

        } else {
            printf("invalid key mgmt:%s\n", apSetSecurity->keyMgmtType);
            infoResp.status = STATUS_INVALID;
            break;
        }

        if (apSetSecurity->encpType[0] == INVALID_STR) {
            printf("encryption type invalid\n");

        }

        if (apSetSecurity->pmf == INVALID_VALUE) {
            printf("pmf invalid\n");

        }

        infoResp.status = STATUS_COMPLETE;
    } while (0);
    wfaEncodeTLV(WFA_AP_SET_SECURITY_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);   
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


int wfaApSetRadius(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   DPRINT_INFO(WFA_OUT, "Entering wfaApSetRadius ...\n");
   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SET_RADIUS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);  
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApConfigCommit(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caApCommonResp_t infoResp;
    caApCommit_t *apCommit = (caApCommit_t *) caCmdBuf;
    size_t reply_len = 0;
    int ret = 0;
    int wmm_enabled = 0;
    int wmmps_enabled = 0;

    DPRINT_INFO(WFA_OUT, "Entering wfaApConfigCommit ...\n");
    do {
        if (apCommit->dutName[0] == INVALID_STR) {
            printf("dut name invalid\n");
            infoResp.status = STATUS_INVALID;
            break;
        }

	sprintf(gCmdStr, "busybox chmod 777 %s", WFA_AP_CONF_PATH);
        system(gCmdStr);
	printf(gCmdStr);

        /* check WMM and WMMPS settings */
	sprintf(gCmdStr, "busybox sed -i /ApUapsd/d %s", WFA_AP_CONF_PATH);
        system(gCmdStr);
	printf(gCmdStr);

	sprintf(gCmdStr, "busybox grep \"wmm_enabled=1\" %s", WFA_AP_CONF_PATH);
        mtk_run_shell_cmd(gCmdStr, gApModeBuff, WFA_BUF_STR_SZ, &reply_len, &wmm_enabled, _wfaApHandleWmm);
	sprintf(gCmdStr, "busybox grep \"uapsd_advertisement_enabled=1\" %s", WFA_AP_CONF_PATH);
        mtk_run_shell_cmd(gCmdStr, gApModeBuff, WFA_BUF_STR_SZ, &reply_len, &wmmps_enabled, _wfaApHandleWmmPS);
        /* Static settings for AP Uapsd */
        if (wmm_enabled && wmmps_enabled) {
	    sprintf(gCmdStr, "busybox echo \"ApUapsd 1\" >> %s", WFA_AP_CONF_PATH);
	    system(gCmdStr);
	    printf(gCmdStr);
        }

	printf("  ==== Kill hostapd process ====\n");
	sprintf(gCmdStr, "busybox killall hostapd");
	system(gCmdStr);
	printf("  system: %s \n",gCmdStr);
	sleep(5);

	sprintf(gCmdStr, "busybox ifconfig %s up", WFA_STAUT_IF);
        system(gCmdStr);
        printf("  system: %s \n",gCmdStr);

        /*start hostapd*/
	printf("  ==== Start hostapd  ====\n");
	sprintf(gCmdStr, "%s -ddd %s &", WFA_AP_BIN_PATH, WFA_AP_CONF_PATH);
        system(gCmdStr);
        printf("  system: %s \n",gCmdStr);

        /*check earlier settings*/
	sprintf(gCmdStr, "busybox ifconfig %s", WFA_STAUT_IF_AP);
        mtk_run_shell_cmd(gCmdStr, gApModeBuff, WFA_BUF_STR_SZ, &reply_len, NULL, _wfaApPrintRes);
        /*reset ap0 and rndis0's ip setting*/
	sprintf(gCmdStr, "busybox ifconfig %s default", WFA_STAUT_IF_AP);
        mtk_run_shell_cmd(gCmdStr, gApModeBuff, WFA_BUF_STR_SZ, &reply_len, NULL, _wfaApPrintRes);

        infoResp.status = STATUS_COMPLETE;
    } while (0);
    wfaEncodeTLV(WFA_AP_CONFIG_COMMIT_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);  
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}

int wfaApSet11d(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   caApSet11d_t *apSet11d_t = (caApSet11d_t *) caCmdBuf;

   DPRINT_INFO(WFA_OUT, "Entering wfaApSet11d ...\n");

   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SET_11D_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApSet11h(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   caApSet11d_t *apSet11d_t = (caApSet11d_t *) caCmdBuf;

   DPRINT_INFO(WFA_OUT, "Entering wfaApSet11h ...\n");

   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SET_11H_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf); 
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApSetApQos(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   caApSetApQos_t *apSetApQos_t = (caApSetApQos_t *) caCmdBuf;

   DPRINT_INFO(WFA_OUT, "Entering wfaApSetApQos ...\n");

   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SET_APQOS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApSetStaQos(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   caApSetApQos_t *apSetStaQos_t = (caApSetApQos_t *) caCmdBuf;

   DPRINT_INFO(WFA_OUT, "Entering wfaApSetStaQos ...\n");

   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SET_STAQOS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApSendAddbaReq(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
   caApCommonResp_t infoResp;
   caApSendAddbaReq_t *apSendAddbaReq_t = (caApSendAddbaReq_t *) caCmdBuf;

   DPRINT_INFO(WFA_OUT, "Entering wfaApSendAddbaReq ...\n");

   infoResp.status = STATUS_COMPLETE;
   wfaEncodeTLV(WFA_AP_SEND_ADDBA_REQ_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
   *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApSet11nWireless(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
	caApCommonResp_t infoResp;
	caApSet11nWireless_t *apSet11nWireless = (caApSet11nWireless_t *) caCmdBuf;

	DPRINT_INFO(WFA_OUT, "Entering wfaApSet11nWireless ...\n");
	printf("In 11nWireless: apSet11nWireless->dutName = %s\n",apSet11nWireless->dutName);
	printf("In 11nWireless: apSet11nWireless->spatialTxStream = %s\n",apSet11nWireless->spatialTxStream);
	printf("In 11nWireless: apSet11nWireless->spatialRxStream = %s\n",apSet11nWireless->spatialRxStream);
	printf("In 11nWireless: apSet11nWireless->sgi20 = %s\n",apSet11nWireless->sgi20);

   	infoResp.status = STATUS_COMPLETE;
   	wfaEncodeTLV(WFA_AP_SET_11N_WIRELESS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
   	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   	return WFA_SUCCESS;
}

int wfaApSetPMF(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
	caApCommonResp_t infoResp;
	caApSetPMF_t *apSetPMF = (caApSetPMF_t *) caCmdBuf;

	DPRINT_INFO(WFA_OUT, "Entering wfaApSetPMF ...\n");
	do {
		if (apSetPMF->dutName[0] == INVALID_STR) {
            		printf("dut name invalid\n");
            		infoResp.status = STATUS_INVALID;
            		break;
        	}
		else {
			/*Remove existing ieee80211w flag in hostapd.conf*/
			sprintf(gCmdStr, "busybox sed -i /^ieee80211w/d %s", WFA_AP_CONF_PATH);
			system(gCmdStr);
			printf(gCmdStr);

			/*Config if needed (1:optional or 2:required)*/
			if (apSetPMF->pmf > 0) { /* 0:disabled 1:optional 2:required*/
				sprintf(gCmdStr, "busybox sed -i \"$ a ieee80211w=%d\" %s",apSetPMF->pmf, WFA_AP_CONF_PATH);
				printf("wfaApSetPMF 11w_pmf = %d\n",apSetPMF->pmf);

				system(gCmdStr);
				printf(gCmdStr);
			}
		}
	} while (0);
   	infoResp.status = STATUS_COMPLETE;
   	wfaEncodeTLV(WFA_AP_SET_PMF_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
   	*respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

   return WFA_SUCCESS;
}

int wfaApGetMacAddress(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    caApGetMacAddress_t *apGetMacAddress = (caApGetMacAddress_t *) caCmdBuf;
    caApCommonResp_t infoResp;
    char *str;

    FILE *tmpfd;
    char string[257];

    DPRINT_INFO(WFA_OUT, "Entering wfaApGetMacAddress ...\n");
    sprintf(gCmdStr, "busybox ifconfig ap0 > /tmp/ipconfig.txt");

    sret = system(gCmdStr);

    tmpfd = fopen("/tmp/ipconfig.txt", "r+");
    if(tmpfd == NULL)
    {
        infoResp.status = STATUS_ERROR;
        wfaEncodeTLV(WFA_AP_GET_MAC_ADDRESS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
        *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

        DPRINT_ERR(WFA_ERR, "file open failed\n");
        return WFA_FAILURE;
    }

    if(fgets((char *)&string[0], 256, tmpfd) == NULL)
    {
        infoResp.status = STATUS_ERROR;
    }

    str = strtok(string, " ");
    while(str && ((strcmp(str,"HWaddr")) != 0))
    {
        str = strtok(NULL, " ");
    }

    /* get mac */
    if(str)
    {
        str = strtok(NULL, " ");
        strcpy(infoResp.mac, str);
	printf("infoResp.mac = %s\n",infoResp.mac);
        infoResp.status = STATUS_COMPLETE;
    }

    wfaEncodeTLV(WFA_AP_GET_MAC_ADDRESS_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);

    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    fclose(tmpfd);
    return WFA_SUCCESS;
}
#endif

#ifdef MTK_WFD_SIGMA
int wfaStaCheckWfdConnection(unsigned char bAutoGO)
{
    unsigned char  bNegotiate = 0;
    unsigned char group_id[WFA_P2P_GRP_ID_LEN] = {0};
    unsigned short rtsp_port = DEFAULT_WFD_RTSP_PORT;
    char session_id[32] = "";
    unsigned char dev_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char if_addr[WFA_P2P_DEVID_LEN] = {0};
    unsigned char strCmd[256] = {0};
    char cmdStr[256];
	
    fprintf(stderr, "Enter %s, tdls=%d\n", __FUNCTION__, wfdTdlsEnable);
	if (wfdTdlsEnable == 0)
	{
        fprintf(stderr, "Checking WFD-P2P connections...\n");
        wfdMtkWfdCheckP2pConnResult(&bNegotiate, NULL, 0);

	    if (bNegotiate == P2P_DEVICE)
	    {
		        printf("%s - I'm P2P Device!\n", __func__);
	    }
        else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
        {
            /* session not available */
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, " WFD Connection Failed (Session Not Available:%s)\n", __FUNCTION__);
            fprintf(stderr, "------------------------\n");
        }
	    else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
	    {
	        char Rule[10] = {0};

	        if (bNegotiate == P2P_GO)
	            sprintf(&Rule, "%s", "GO");
	        else
	            sprintf(&Rule, "%s", "CLIENT");
        
	        printf("%s - I'm P2P %s!\n", __func__, Rule);

				if(p2p_priv_p->group_id[0] != '\0')
				{
					printf("[SIGMA]: group_id: %s\n",p2p_priv_p->group_id);
					strncpy(group_id, p2p_priv_p->group_id, strlen(p2p_priv_p->group_id));
				}
				char tmp_rtspport[128] = {0};
				char tmp_string[128] = {0};
				
				// we must connected first		
				//so all those file should be exist
				
				FILE *tmpfd;
				tmpfd = fopen("/tmp/sigma_rtsp_port.txt", "r+");
				if(tmpfd == NULL)
				{
					printf("%s - fail to open /tmp/sigma_rtsp_port.txt\n", __func__);
					rtsp_port = DEFAULT_WFD_RTSP_PORT;
				}
				else
				{
					fgets(tmp_string, sizeof(tmp_string), tmpfd);
					//sample code: 00 0006 0111 ^022a^ 0064
					memcpy(tmp_rtspport,tmp_string+10,4);
					printf("tmp_rtspport: %s  <tmp_string: %s>\n",tmp_rtspport,tmp_string);
					rtsp_port = strtol(tmp_rtspport, NULL , 16);					
					fclose(tmpfd);
				}
			
				tmpfd = NULL;
				tmpfd = fopen("/tmp/sigma_if_addr_wfd.txt", "r+");
				if(tmpfd == NULL)
				{
					printf("[SIGMA] %s - fail to open /tmp/sigma_if_addr_wfd.txt\n", __func__);
					//memcpy(if_addr, dev_addr, sizeof(dev_addr));
					printf("[SIGMA] device mac: %s	\n",if_addr);
				}
				else
				{
					fgets(if_addr, sizeof(if_addr), tmpfd);
					printf("[SIGMA] interface mac: %s  \n",if_addr);
					fclose(tmpfd);
				}	

			printf("GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);

	        /* starting dhcp server and getting peer's ip */
	            char peer_ip[24] = "";
	            int result = 0;
	            char port_str[16] = "";
	            int rtsp_retry = 0;
			#ifdef MTK_WFD_SIGMA
	            extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
			#endif 

            if (rtsp_port == 0)
            {
                fprintf(stderr, "ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                rtsp_port = DEFAULT_WFD_RTSP_PORT;
            }
	            sprintf(port_str, "%d", rtsp_port);
	            if (bNegotiate == P2P_GO)
	            {
	                fprintf(stderr, "Starting DHCP server...\n");
				#ifdef MTK_WFD_SIGMA
	                if (wfaMtkWfdStartDhcpServer(NULL, peer_ip, sizeof(peer_ip)) != 0)
	                {
	                    fprintf(stderr, "Fail to get DHCP Client IP address.\n");
	                    result = -1;
	                }
				#endif
	            }
	            else if (bNegotiate == P2P_CLIENT)
	            {
	                fprintf(stderr, "Starting DHCP client...\n");
					#ifdef MTK_WFD_SIGMA
	                if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
	                {
	                    fprintf(stderr, "Fail to get DHCP Server IP address.\n");
	                    result = -1;
	                }
					#endif
	            }

#ifdef MTK_WFD_SIGMA
	            if (result == 0)
	            {
	                /* starting rtsp */
	                while (rtsp_retry < MAX_RTSP_RETRY)
	                {                        
	                    sleep(10);
	                    fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
	                    if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
	                    {
	                        /* successful */
	                        fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
	                        result = 0;
	                        break;
	                    }
	                    else
	                    {
	                        /* failed */
	                        fprintf(stderr, "RTSP negotiation is failed\n");
	                        result = -1;
	                    }
	                    rtsp_retry ++;
	                    if (rtsp_retry < MAX_RTSP_RETRY)
	                        fprintf(stderr, "Retrying RTSP (retry=%d)\n", rtsp_retry);
	                }
	            }
#endif
	            if (result != 0)
	            {
	                fprintf(stderr, "------------------------\n");
	                fprintf(stderr, "WFD Connection Failed!!!\n");
	                fprintf(stderr, "------------------------\n");
	            }
	            else
	            {
//	                sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=0", staStartWfdConn->intf);
//	                system(cmdStr);
	                fprintf(stderr, "------------------------\n");
	                fprintf(stderr, " WFD Connection Success\n");
	                fprintf(stderr, "------------------------\n");
	                if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)								
	                {
	                    sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
	                    system(cmdStr);
	                }
	            }
    	}
	}
	else /* wfd connect by tdls */
	{
        char peer_ip[24] = "";
        int result = 0;
        char port_str[16] = "";
        int rtsp_retry = 0;
        char peer_ip_addr[5] = {0};
		FILE *tmpfile = NULL;
		char tmpBuf[16];
		char tmp_rtspport[8];
#ifdef MTK_WFD_SIGMA
        extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
#endif 

        fprintf(stderr, "Checking WFD P2P&TDLS connections...\n");
        wfdMtkWfdCheckP2pAndTdlsConnResult(&bNegotiate, NULL);

        fprintf(stderr, "bNegotiate =%d\n", bNegotiate);
        if (bNegotiate == TDLS_LINKED)
        {
			sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_peer_ip= | busybox cut -f2- -d= > /tmp/tdlsPeerIP", WFA_STAUT_IF, ctrl_if);
			system(cmdStr);
			tmpfile = fopen("/tmp/tdlsPeerIP", "r+");
			if(tmpfile == NULL)
			{
				/* hope not happen */
				DPRINT_ERR(WFA_ERR, "file open failed\n");
				return WFA_FAILURE;
			}
			fscanf(tmpfile, "%s", tdls_peer_ip);

			sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_dev_info= | busybox cut -f2- -d= > /tmp/tdlsDevInfo", WFA_STAUT_IF, ctrl_if);
			system(cmdStr);
			tmpfile = fopen("/tmp/tdlsDevInfo", "r+");
			if(tmpfile == NULL)
			{
				/* hope not happen */
				DPRINT_ERR(WFA_ERR, "file open failed\n");
				return WFA_FAILURE;
			}
			fscanf(tmpfile, "%s", tmpBuf);
			memcpy(tmp_rtspport, tmpBuf + 4, 4);
			tmp_rtspport[4] = '\0';
			printf("tmp_rtspport: %s  <tmpBuf: %s>\n", tmp_rtspport, tmpBuf);
			rtsp_port = strtol(tmp_rtspport, NULL , 16);

#ifdef MTK_WFD_SIGMA
            /* starting rtsp */
            while (rtsp_retry < MAX_RTSP_RETRY)
            {
                sleep(15);
                fprintf(stderr, "Starting RTSP to [%s:%s]...\n", tdls_peer_ip, port_str);
                if (wfaMtkWfdCmd_rtspStart(tdls_peer_ip, port_str, session_id) == 0)
                {
                    /* successful */
                    fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                    result = 0;
                    break;
                }
                else
                {
                    /* failed */
                    fprintf(stderr, "RTSP negotiation is failed\n");
                    result = -1;
                }
                rtsp_retry ++;
                if (rtsp_retry < MAX_RTSP_RETRY)
                fprintf(stderr, "Retrying RTSP (retry=%d)...\n", rtsp_retry);
            }
#endif 

            if (result != 0)
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, "WFD Connection Failed!!!\n");
                fprintf(stderr, "------------------------\n");
            }
            else
            {
                fprintf(stderr, "------------------------\n");
                fprintf(stderr, " WFD Connection Success\n");
                fprintf(stderr, "------------------------\n");
            }
        }

        else if (bNegotiate == P2P_DEVICE)
        {
            printf("P2P/TDLS Connection Failed!\n");
        }
        else if (bNegotiate == WFD_SESSION_UNAVALIBLE)
        {
            /* session not available */
            fprintf(stderr, "------------------------\n");
            fprintf(stderr, " WFD Connection Failed (Session Not Available)\n");
            fprintf(stderr, "------------------------\n");
        }
        else if ((bNegotiate == P2P_GO) || (bNegotiate == P2P_CLIENT))
        {
            char Rule[10] = {0};

            if (bNegotiate == P2P_GO)
                sprintf(&Rule, "%s", "GO");
            else
                sprintf(&Rule, "%s", "CLIENT");
            printf("I'm P2P %s!\n", Rule);
//            wfa_driver_ralink_get_oid(WFA_STAUT_IF_P2P, OID_802_11_P2P_PEER_GROUP_ID, &group_id, sizeof(group_id));
//            wfa_driver_ralink_get_oid(WFA_STAUT_IF_P2P, OID_802_11_WFD_PEER_RTSP_PORT, &rtsp_port, sizeof(rtsp_port));
            //sprintf(group_id, "%s %s", pdrv_ralink_cfg->addr, pdrv_ralink_cfg->ssid);
            fprintf(stderr, "GROUP_ID = %s.  rtsp_port = %d\n", group_id, rtsp_port);

#ifdef MTK_WFD_SIGMA
            /* starting dhcp server and getting peer's ip */
            {
                char peer_ip[24] = "";
                int result = 0;
                char port_str[16] = "";
                int rtsp_retry = 0;

                extern int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
                if (rtsp_port == 0)
                {
                    fprintf(stderr, "ERROR! RTSP port is 0, default to %d\n", DEFAULT_WFD_RTSP_PORT);
                    rtsp_port = DEFAULT_WFD_RTSP_PORT;
                }
                sprintf(port_str, "%d", rtsp_port);
                if (bNegotiate == P2P_GO)
                {
                    fprintf(stderr, "Starting DHCP server...\n");
                    if (wfaMtkWfdStartDhcpServer(NULL, peer_ip, sizeof(peer_ip)) != 0)
                    {
                        fprintf(stderr, "Fail to get DHCP Client IP address.\n");
                        result = -1;
                    }
                }
                else if (bNegotiate == P2P_CLIENT)
                {
                    fprintf(stderr, "Starting DHCP client...\n");
                    if (wfaMtkWfdStartDhcpClient(peer_ip, sizeof(peer_ip), 0) != 0)
                    {
                        fprintf(stderr, "Fail to get DHCP Server IP address.\n");
                        result = -1;
                    }
                }

                if (result == 0)
                {
                    /* starting rtsp */
                    while (rtsp_retry < MAX_RTSP_RETRY)
                    {
                        sleep(10);
                        fprintf(stderr, "Starting RTSP to [%s:%s]...\n", peer_ip, port_str);
                        if (wfaMtkWfdCmd_rtspStart(peer_ip, port_str, session_id) == 0)
                        {
                            /* successful */
                            fprintf(stderr, "RTSP completed, session_id=[%s]\n", session_id);
                            result = 0;
                            break;
                        }
                        else
                        {
                            /* failed */
                            fprintf(stderr, "RTSP negotiation is failed\n");
                            result = -1;
                        }
                        rtsp_retry ++;
                        if (rtsp_retry < MAX_RTSP_RETRY)
                            fprintf(stderr, "Retrying RTSP (retry=%d, max=%d)...\n", rtsp_retry, MAX_RTSP_RETRY);
                    }
                }
#endif
                if (result != 0)
                {
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, "WFD Connection Failed!!!\n");
                    fprintf(stderr, "------------------------\n");
                }
                else
                {
//                      sprintf(cmdStr, "iwpriv %s set p2pSerDiscEnable=0", staStartWfdConn->intf);
//                      system(cmdStr);
                    fprintf(stderr, "------------------------\n");
                    fprintf(stderr, " WFD Connection Success\n");
                    fprintf(stderr, "------------------------\n");
                    if (bNegotiate == P2P_GO || bNegotiate == P2P_CLIENT)								
                    {
                    	sprintf(cmdStr, "wpa_cli -i %s %s set wfd_sessionAvail 0", WFA_STAUT_IF, ctrl_if);
                    	system(cmdStr);
                    }
                }
            }
        }

    }

    return 0;
}
#endif /* MTK_WFD_SIGMA */

int wfaStaHs20StaScan(int len, BYTE *caCmdBuf, int *respLen, BYTE *respBuf)
{
    dutCmdResponse_t infoResp;
    caStaScan_t *staScan = (caStaScan_t *)caCmdBuf;
    int j = 0;
    char hessid_t[WFA_HS20_LEN_64] = " -h";
    char accs_net_type_t[WFA_HS20_LEN_64] = " -a";

    DPRINT_INFO(WFA_OUT, "Entering %s ...\n", __func__);

    MTK_HS20_CMD("-i %s ap_scan 0 ", staScan->intf);

    if((staScan->hessid[0] != '\0') || (staScan->accs_net_type[0] != '\0')) {
        MTK_HS20_CMD("-i %s sta_add_scan_info%s%s\n",
            staScan->intf,
            (staScan->hessid[0] == '\0')?"":strcat(hessid_t,staScan->hessid),
            (staScan->accs_net_type[0] == '\0')?"":strcat(accs_net_type_t,staScan->accs_net_type)
            );
    }

    printf("Setting Scan Info");
    for(j = 1; j <= 5; j++) {
        printf(".", j);
        buzz_time(1000000);
    }
    printf("\n");

    MTK_HS20_CMD("-i %s ap_scan 1 ", staScan->intf);
    MTK_HS20_CMD("-i %s scan", staScan->intf);

    infoResp.status = STATUS_COMPLETE;
    wfaEncodeTLV(WFA_STA_HS20_STA_SCAN_RESP_TLV, sizeof(infoResp), (BYTE *)&infoResp, respBuf);
    *respLen = WFA_TLV_HDR_LEN + sizeof(infoResp);

    return WFA_SUCCESS;
}


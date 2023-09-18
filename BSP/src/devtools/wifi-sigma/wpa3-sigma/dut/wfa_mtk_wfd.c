#ifdef MTK_P2P_SIGMA
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/wireless.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "wfa_portall.h"
#include "wfa_debug.h"
#include "wfa_main.h"
#include "wfa_types.h"
#include "wfa_dut.h"
#include "wfa_sock.h"
#include "wfa_tg.h"
#include "wfa_stdincs.h"
#include "wfa_ralink.h"
#include "wfa_cmds.h"
#include "mtk_ini.h"

int wfaMtkWfdSendCmd(int *sockfd, char *cmd, char *rsp, int *rlen);
int wfaMtkWfdInitSock(void);
int wfaMtkWfdCmd_init(void);
int wfaMtkWfdCmd_rtspPlay(void);
int wfaMtkWfdCmd_rtspPause(void);
int wfaMtkWfdCmd_rtspTeardown(void);
int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId);
int wfaMtkWfdCmdRspProcess(int curr_cmd, char *rsp, int rsp_len, char *ret_buf);
int wfaMtkWfdGetClientIp(FILE *fp, char *mac, char *return_ip, int return_buf_len);
int wfaMtkWfdStartDhcpServer(char *client_mac, char *return_ip, int return_buf_len);
int wfaMtkWfdGetServerIp(FILE *fp, char *return_ip);
int wfaMtkWfdStartDhcpClient(char *return_ip, int return_buf_len, char intf_idx);

extern int wfa_driver_ralink_get_oid(unsigned char *intf, int oid, unsigned char *data, int len);
extern int wfa_driver_ralink_set_oid(int oid, char *data, int len);
extern void wfa_driver_ralink_tdls_setup(unsigned char *dev_addr);
extern void wfa_cfg80211_tdls_setup(unsigned char *dev_addr);

static void wfaMtkWfdDump(char *buf, int len);



#define MTKSIGDBG(x...) \
    do{\
        fprintf(stderr, "[mtk-sigma]" x);\
    } while(0)

#define MTK_WFD_PORT            2472
#define MTK_WFD_SND_BUF_SIZE    1024
#define MTK_WFD_RCV_BUF_SIZE    1024

#define WFD_SIGMA_CMD_SIZE      sizeof(struct wfd_sigma_cmd_hdr)

#ifdef MTK_P2P_SIGMA
#include "wfa_wpa.h"
extern wpa_private_t *p2p_priv_p;
#endif

#ifdef MTK_ANDROID_SIGMA
int wfaMtkWfdGetClientIp_Android(FILE *fp, char *mac, char *return_ip, int return_buf_len);
int wfaMtkWfdGetServerIp_Android(char *return_ip,char intf_idx);

//#define ANDROID_SIGMA_PATH 				"/storage/sdcard0/sigma"
#define ANDROID_SIGMA_PATH				"/data/sigma/wfd_sigma"
#endif

#define P2P_SERVER_IP_FILE_PATH     "/3rd/wpa_supplicant/p2p_s_ip"
#define WFD_P2P_IF_NAME             "p2p0"
#define MAX_DHCP_WAIT_CNT           30
#define WFD_MAX_STR_BUF             256

#define DHCPC_SERVER_IP_FILE_PATH   "/tmp/p2p_s_ip"
#define WFD_P2P_GET_IFCONFIG_PATH   "/tmp/sigma_ifconfig.txt"

#define MAX_P2P_CONN_RESULT_CHECK_COUNT     80
#define MAX_TDLS_CONN_RESULT_CHECK_COUNT    20
#define WFD_RETRY_CONNECT_INTERVAL          20
#define WFD_P2P_CONN_CHECK_INTERVAL_SEC     1 /* sec */
#define WFD_TDLS_CONN_CHECK_INTERVAL_SEC    3

/* cmd list */
enum 
{
    MTK_WFD_SIGMA_CMD_INIT = 0,
    MTK_WFD_SIGMA_CMD_RTSP_START, /* Start RTSP */
    MTK_WFD_SIGMA_CMD_RTSP_PLAY, /* M7 */
    MTK_WFD_SIGMA_CMD_RTSP_PAUSE, /* M10 */
    MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN, /* M9 */
    MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ, /* 5 */
    MTK_WFD_SIGMA_CMD_RSP_OK, /* response OK */
    MTK_WFD_SIGMA_CMD_RSP_ERROR_UNKNOWN, /* response Unknown error */
    MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI, /* 10 */
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC,
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_HIDC, /* 15 */
    MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD,
    MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_HDCP2X,
    MTK_WFD_SIGMA_CMD_RTSP_SET_VIDEO_FORMAT,
    MTK_WFD_SIGMA_CMD_RTSP_SET_AUDIO_FORMAT,
    MTK_WFD_SIGMA_CMD_RTSP_CONF_RESET,
    MTK_WFD_SIGMA_CMD_SET_SESSION_AVAIL,
    MTK_WFD_SIGMA_CMD_DISABLE_ALL,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_STANDBY,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_I2C,
    MTK_WFD_SIGMA_CMD_RTSP_ENABLE_EDID,
    MTK_WFD_SIGMA_CMD_MAX
};

enum
{
    WFD_VIDEO_FORMAT_TYPE_CEA = 0,
    WFD_VIDEO_FORMAT_TYPE_VESA,
    WFD_VIDEO_FORMAT_TYPE_HH
};


struct wfd_sigma_cmd_hdr
{
    int len; /* length of the whole cmd message */
    int type; /* type of the message */
};

struct wfd_sigma_cmd
{
    struct wfd_sigma_cmd_hdr hdr;
    char data[32];
};


static int mtkWfdSockFd = -1;
extern char ctrl_if[];

int wfaMtkWfdInit(void)
{
    int ret = 0;

    DPRINT_ERR(WFA_ERR, "%s\n", __FUNCTION__);

    if(mtkWfdSockFd == -1)
    {
        mtkWfdSockFd = wfaMtkWfdInitSock();
        if (mtkWfdSockFd < 0)
        {
            DPRINT_ERR(WFA_ERR, "fail to connect to peer\n");
            goto mtkwfd_out_err;
        }
    }
    
    DPRINT_ERR(WFA_ERR, "connection to MTK WFD Ok\n");

    //test code
    /*
    {
        char ip[24]="";
        char mac[24]= "";
        char mask[24] = "";
        wfaMtkWfdP2pGetIfConfig(mac, ip, mask);
        
        fprintf(stderr, "P2P IP get, [mac=%s, ip=%s, mask=%s]\n", mac, ip, mask);
    }
    */
    
    /*
    DPRINT_ERR(WFA_ERR, "Wating for 5 sec to send cmd\n");
    sleep (5);
    wfaMtkWfdCmd_init();
    sleep(3);
    wfaMtkWfdCmd_rtspStart("192.168.100.300", "554");
    */
    
    return ret;

mtkwfd_out_err:
    if (mtkWfdSockFd != -1)
        close(mtkWfdSockFd);
    mtkWfdSockFd = -1;
    exit(1);
    
}

int wfaMtkWfdCmd_init(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_INIT;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    //wfaMtkWfdDump(rbuf, rlen);
    
    return 0;

}


int wfaMtkWfdCmd_rtspConfReset(void)
{
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_CONF_RESET;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    
    return 0;

}

int wfaMtkWfdCmd_rtspStart(char *ipaddr, char *port,  char *sessionId)
{
    /* sending init cmd to mtk wfd */
    char buf[128];
    char rbuf[128];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    if (!ipaddr || !port || !sessionId)
    {
        DPRINT_ERR(WFA_ERR, "Error input [ipaddr] or [port] or [sessionId buffer not provided] \n");
        return -1;
    }
    memset(rbuf, 0, sizeof(rbuf));
    /* prepare the data of string "ip:port:" */
    sprintf(rbuf, "%s:%s:", ipaddr, port);
    
    memset(buf, 0, sizeof(buf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_START;
    /* start cmd has data which carries "ip:port:" string */
    strncpy(cmdP->data, rbuf, strlen(rbuf));
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(rbuf) + 1 ;
    memset(rbuf, 0, sizeof(rbuf));
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
        if (wfaMtkWfdCmdRspProcess(cmdP->hdr.type, rbuf, rlen, sessionId) != 0)
            return -1;
    }
    
    return 0;

}

int wfaMtkWfdCmd_rtspPlay(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_PLAY;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    
    return 0;
}


int wfaMtkWfdCmd_rtspPause(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_PAUSE;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Pause\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    
    return 0;
}

int wfaMtkWfdCmd_rtspTeardown(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

	MTKSIGDBG("%s in .....\n",__FUNCTION__);


    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: teardown\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    
    return 0;
}

int wfaMtkWfdCmd_rtspUibcCapUpdate(int type)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    switch (type)
    {
        case WFD_CAP_UIBC_MOUSE:
        {
            strcpy(cmdP->data, "Mouse");
            break;
        }

        case WFD_CAP_UIBC_KEYBOARD:
        {
            strcpy(cmdP->data, "Keyboard");
            break;
        }

        default:
        {
            return -1;
        }

    }
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data) +1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: MTK_WFD_SIGMA_CMD_RTSP_UIBC_CAP_UPDATE\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;

}


int wfaMtkWfdCmd_rtspSendIdrReq(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspUibcGenEvent(int evtType)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    
    if (evtType == eSingleTouchEvent || evtType == eMouseEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE;
    else if (evtType == eMultiTouchEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI;
    else if (evtType == eKeyBoardEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD;
    else
        /* default to single touch */
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE;
    
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}


int wfaMtkWfdCmd_rtspUibcHidcEvent(int hidType)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[254];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (hidType != eKeyBoardEvent)
    {
        fprintf(stderr, "%s:Warning, hidType != Keyboard\n", __FUNCTION__);
    }

    if (hidType == eKeyBoardEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD;
    else if (hidType == eMouseEvent)
        cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspUibcCapEnable(int cap_type)
{
    /* sending init cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    switch (cap_type)
    {
        case eUibcGen:
        {
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC;
            break;
        }
        case eUibcHid:
        {
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_HIDC;
            break;
        }
        default:
        {
            MTKSIGDBG("%s: Unknown cap_type, force use Generic\n", __FUNCTION__);
            cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_UIBC_ENABLE_GENERIC;
            break;
        }
    }
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}

int wfaMtkWfdCmd_rtspEnterStandby(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: Init\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    return 0;
}


int wfaMtkWfdCmd_rtspSetWfdDevType(int wfdDevType)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    switch (wfdDevType)
    {
        case ePSink:
        {
            strcpy(cmdP->data, "P-Sink");
            break;
        }

        case eSSink:
        {
            strcpy(cmdP->data, "S-Sink");
            break;
        }

        default:
        {
            strcpy(cmdP->data, "P-Sink");
            break;
        }

    }
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: MTK_WFD_SIGMA_CMD_RTSP_SET_WFD_DEV_TYPE\n");
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;

}

int wfaMtkWfdCmd_rtspEnableHDCP2X(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_HDCP2X;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableSessionAvail(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_SET_SESSION_AVAIL;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}



int wfaMtkWfdCmd_rtspSetVideoFormat(unsigned char *index_array, int array_size)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;
    
    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;


    /* video format bitmask */
    if (!index_array || !array_size)
    {
        MTKSIGDBG("%s: Error video format array\n", __FUNCTION__);
        return -1;
    }    
    cmdP->data[0] = (char)array_size;
    memcpy(&cmdP->data[1], index_array, array_size);
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_SET_VIDEO_FORMAT;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 1 + array_size;
    
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableStandby(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_STANDBY;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableI2c(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_I2C;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}

int wfaMtkWfdCmd_rtspEnableEdid(int enable)
{
    /* sending cmd to mtk wfd */
    char buf[254];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;

    if (enable)
        strcpy(cmdP->data, "enable");
    else
        strcpy(cmdP->data, "disable");
    
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_RTSP_ENABLE_EDID;
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + strlen(cmdP->data)+1;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }

    return 0;
}


int wfaMtkWfdCmd_sigmaDisableAll(void)
{
    /* sending init cmd to mtk wfd */
    char buf[64];
    char rbuf[64];
    int rlen;
    struct wfd_sigma_cmd *cmdP = NULL;

    memset(buf, 0, sizeof(buf));
    memset(rbuf, 0, sizeof(rbuf));
    cmdP = (struct wfd_sigma_cmd *)&buf;
    cmdP->hdr.type = MTK_WFD_SIGMA_CMD_DISABLE_ALL;
    /* init cmd does not have data */
    cmdP->hdr.len = WFD_SIGMA_CMD_SIZE + 4;
    rlen = sizeof(rbuf);
    if (wfaMtkWfdSendCmd(&mtkWfdSockFd, buf, rbuf, &rlen) < 0)
    {
        DPRINT_ERR(WFA_ERR, "fail to send cmd: %d\n", cmdP->hdr.type);
        return -1;
    }
    if (rlen > 0)
    {
        MTKSIGDBG("received response from mtk wfd\n");
        //wfaMtkWfdDump(rbuf, rlen);
    }
    
    return 0;
}


int wfaMtkWfdInitSock(void)
{
    int ret = 0;
    int sockfd = -1;
    struct sockaddr_in servAddr;
    
    if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "socket() failed: %i\n", errno);
        ret = -1;
    }

    memset(&servAddr, 0, sizeof(servAddr)); 
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servAddr.sin_port        = htons(MTK_WFD_PORT);
    
    DPRINT_ERR(WFA_ERR, "connecting to MTK WFD\n");

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "connect() failed\n");
        close(sockfd);
        ret = -1;
    }

    if (ret < 0)
        return ret;
    else
        return sockfd;
}

static void wfaMtkWfdDump(char *buf, int len)
{
    int pos = 0;
    if (!buf)
        return;

    fprintf(stderr, "Dumping buf (len=%d):\n---------", len);
    while(pos < len)
    {
        if (pos%8 == 0)
        {
            fprintf(stderr,"\n%04d| ", pos);
        }
        fprintf(stderr, "%02x ", buf[pos]);
        pos ++;
    }
    fprintf(stderr, "\n---------\n");
}


void wfaMtkWfdKillProcess(char * process_name)
{
	fprintf(stderr,"%s  in ..... \n",__FUNCTION__); 

	char killCmd[64] = {0};	
	char sBuf[512] = {0};	
	char * tokenBuf = NULL;	
	FILE *fp = NULL;		

	memset(killCmd,0,sizeof(killCmd));	
	memset(sBuf,0,sizeof(sBuf));	

	if(process_name == NULL || process_name[0] == '\0')
	{
		fprintf(stderr,"%s:%d  Process Name Invalid \n",__FUNCTION__,__LINE__);	
		return;	
	}
	if(strlen(process_name)>1)
		sprintf(killCmd,"ps | grep [%c]%s > /tmp/sigma_kill_%s 2>&1;sync;sync",process_name[0],process_name+1,process_name);	
	else
		sprintf(killCmd,"ps | grep [%c] > /tmp/sigma_kill_%s 2>&1;sync;sync",process_name[0],process_name);	
	
	system(killCmd);	
	fprintf(stderr," 1. >>>> %s \n",killCmd);	

	memset(killCmd,0,sizeof(killCmd));	
	sprintf(killCmd,"cat /tmp/sigma_kill_%s",process_name);	
	system(killCmd);	
	//fprintf(stderr," 1. >>>> %s \n",killCmd);	

	memset(killCmd,0,sizeof(killCmd));	
	sprintf(killCmd,"/tmp/sigma_kill_%s",process_name);	
	fp = fopen(killCmd,"r");	
	if(!fp)	
	{		
		fprintf(stderr,"%s open Error \n",killCmd);
		fclose(fp);
		return;	
	}	

	while(fgets(sBuf,sizeof(sBuf),fp))
	{		
		tokenBuf = strtok(sBuf," ");		
		if(tokenBuf != NULL)		
		{			
			fprintf(stderr,"<PID>%s \n",tokenBuf);
			sprintf(killCmd,"kill -9 %s",tokenBuf);	
			system(killCmd);	
			fprintf(stderr,"system: %s \n",killCmd);	
		}		
		memset(sBuf,0,sizeof(sBuf));	
	}	
	fclose(fp);

	if(strlen(process_name)>1)
		sprintf(killCmd,"ps aux| grep [%c]%s > /tmp/sigma_kill_%s_aux 2>&1;sync;sync",process_name[0],process_name+1,process_name);	
	else
		sprintf(killCmd,"ps aux| grep [%c] > /tmp/sigma_kill_%s_aux 2>&1;sync;sync",process_name[0],process_name);	
	system(killCmd);	
	fprintf(stderr,"  2. >>>> %s \n",killCmd);	

	memset(killCmd,0,sizeof(killCmd));	
	sprintf(killCmd,"cat /tmp/sigma_kill_%s_aux",process_name);	
	system(killCmd);	
	//fprintf(stderr,"  2. >>>> %s \n",killCmd);	

	memset(killCmd,0,sizeof(killCmd));	
	sprintf(killCmd,"/tmp/sigma_kill_%s_aux",process_name);	
	fp = fopen(killCmd,"r");	
	if(!fp)	
	{		
		fprintf(stderr,"%s open Error \n",killCmd);
		fclose(fp);		
		return;	
	}	

	while(fgets(sBuf,sizeof(sBuf),fp))
	{
		tokenBuf = strtok(sBuf," ");
		if(tokenBuf != NULL)
		{	
			fprintf(stderr,"2.<-Owner>: %s \n",tokenBuf);	
			tokenBuf = strtok(NULL," ");
			if(tokenBuf != NULL)		
			{				
				fprintf(stderr,"2.<PID>: %s \n",tokenBuf);	
				sprintf(killCmd,"kill -9 %s",tokenBuf);	
				system(killCmd);	
				fprintf(stderr,"system: %s \n",killCmd);
			}		
		}		
		memset(sBuf,0,sizeof(sBuf));
	}	
	fclose(fp);		
	return;
}

int wfaMtkWfdSendCmd(int *sockfd, char *cmd, char *rsp, int *rlen)
{
    struct wfd_sigma_cmd *cmdP = NULL;
    int sndLen = 0, retLen = 0;
    fd_set sockSet;
    int maxfdn1 = -1;
    int nfds = 0;
    int ret = 0;

	DPRINT_ERR(WFA_ERR, "in ....\n", sndLen, retLen);

    if (*sockfd == -1)
    {
        DPRINT_ERR(WFA_ERR, "socket is not ready, trying to connect.\n");
        *sockfd = wfaMtkWfdInitSock();
        if (*sockfd < 0)
        {
            DPRINT_ERR(WFA_ERR, "fail to connect to peer\n");
            exit (1);
        }
        else
            DPRINT_ERR(WFA_ERR, "socket connected.\n");
        
    }

    cmdP = (struct wfd_sigma_cmd *)cmd;
    sndLen = cmdP->hdr.len;
    retLen = send(*sockfd, cmd, sndLen, 0);
    if (retLen != sndLen)
    {
        DPRINT_ERR(WFA_ERR, "sending len mistach (requested=%d, sent=%d)\n", sndLen, retLen);
        ret = -1;
        goto wfdsend_exit;
    }

    /* prepare to receive response */
    FD_ZERO(&sockSet);
    FD_SET(*sockfd, &sockSet);
    maxfdn1 = (*sockfd) + 1;
    
    DPRINT_ERR(WFA_ERR, "Waiting for response from MTK WFD\n");
    if((nfds = select(maxfdn1, &sockSet, NULL, NULL, NULL)) < 0)
    {
        DPRINT_ERR(WFA_ERR, "select error!\n");
        ret = -1;
        goto wfdsend_exit;
    }
    else
    {
        if ((retLen = recv(*sockfd, rsp, *rlen, 0)) <= 0)
        {
            DPRINT_ERR(WFA_ERR, "recv() failed or connection closed prematurely");
            ret = -1;
            goto wfdsend_exit;
        }
        *rlen = retLen;
        DPRINT_ERR(WFA_ERR, "Received response from MTK WFD (len=%d)\n", retLen);
    }
    ret = 0;

wfdsend_exit:
	
	DPRINT_ERR(WFA_ERR, "ret=%d\n", ret);
    return ret;
}

int wfaMtkWfdCmdRspProcess(int curr_cmd, char *rsp, int rsp_len, char *ret_buf)
{
    struct wfd_sigma_cmd *cmdRspP = (struct wfd_sigma_cmd *)rsp;
    char *ptr = NULL;

    if (!rsp || !rsp_len)
    {
        MTKSIGDBG("No response message. Ignoring...\n");
        return -1;
    }
    switch(curr_cmd)
    {
        case (MTK_WFD_SIGMA_CMD_RTSP_START):
        {
            if (cmdRspP->hdr.type != MTK_WFD_SIGMA_CMD_RSP_OK)
            {
                MTKSIGDBG("Cmd Response from MTK WFD is NG (err = %d)...\n", cmdRspP->hdr.type);
                return -1;
            }
            if (!ret_buf)
            {
                MTKSIGDBG("%s: return buffer not supplied\n", __FUNCTION__);
                return -1;
            }
            /* parse to get the sessionId */
            /* the cmd format is
                "[session_id]:"
                where session_id is in hex or digits 
            */
            ptr = strchr(cmdRspP->data, ':');
            if (!ptr)
            {
                MTKSIGDBG("%s: session id not found int the response\n", __FUNCTION__);
                return -1;
            }
            strncpy(ret_buf, cmdRspP->data, ptr-cmdRspP->data);
            ret_buf[ptr-cmdRspP->data] = 0x00;
            return 0;
        }
        
        case (MTK_WFD_SIGMA_CMD_RTSP_PLAY):
        case (MTK_WFD_SIGMA_CMD_RTSP_PAUSE):
        case (MTK_WFD_SIGMA_CMD_RTSP_TEARDOWN):
        case (MTK_WFD_SIGMA_CMD_RTSP_ENTER_STANDBY):
        case (MTK_WFD_SIGMA_CMD_RTSP_SEND_IDR_REQ):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_SINGLE):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_KEYBOARD):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_KEYBOARD):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_HIDC_EVENT_MOUSE):
        case (MTK_WFD_SIGMA_CMD_RTSP_UIBC_GEN_EVENT_MULTI):
        {
            break;            
            if (cmdRspP->hdr.type != MTK_WFD_SIGMA_CMD_RSP_OK)
            {
                MTKSIGDBG("Cmd Response from MTK WFD is NG (err = %d)...\n", cmdRspP->hdr.type);
                return -1;
            }
            if (strlen(ret_buf))
                MTKSIGDBG("Cmd Response =[%s]\n", ret_buf);
        }
       
        default:

            break;

    }
    return 0;

}

int wfaMtkWfdStartDhcpServer(char *client_mac, char *return_ip, int return_buf_len)
{
#ifdef MTK_ANDROID_SIGMA
    char strcmd[256];	 
    int loop_cnt = 0;
    int ret = -1;
    FILE* fp = NULL;
    char tmp_if_name[IFNAMSIZ] = {0};	 


    wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 

    memset(strcmd, 0, sizeof(strcmd));
    sprintf(strcmd, "mtk_dhcp_reset.sh");
    MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
    system(strcmd);


	//set local server IP
	memset(strcmd, 0, sizeof(strcmd));
	sprintf(strcmd, "busybox ifconfig %s %s", tmp_if_name, DHCPD_DEF_STATIC_IP_ADDR);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);

	memset(strcmd, 0, sizeof(strcmd));
	sprintf(strcmd, "mtk_dhcp_server.sh %s", tmp_if_name);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);
	
	sleep(1);


	MTKSIGDBG("Checking for client IP..\n");
	while(loop_cnt < MAX_DHCP_WAIT_CNT)
	{
		loop_cnt ++;
		sleep(1);
		if (!fp)
		{
			fp = fopen(DHCPD_LEASE_FILE_PATH, "r");
			if (!fp)
				continue;
		}

		if ((ret = wfaMtkWfdGetClientIp_Android(fp, client_mac, return_ip, return_buf_len)) == 0)
			break;
	}

	if (loop_cnt >= MAX_DHCP_WAIT_CNT && ret != 0)
		MTKSIGDBG("%s: Time out waiting for getting client's IP address\n", __FUNCTION__);
	else
		MTKSIGDBG("Checking client Ip complete (ret = %d)..\n", ret);

	if (fp)
		fclose(fp);

	return ret; 

#else
	char strcmd[256];	 
	FILE* fp = NULL;
	int loop_cnt = 0;
	int ret = -1;
	//struct stat fstat; 

	memset(strcmd, 0, sizeof(strcmd));
	wfaMtkWfdKillProcess("dhcpd");
	sprintf(strcmd, "echo \"\"> %s;sync;sync",DHCPD_LEASE_FILE_PATH);
	system(strcmd); 

	wfaMtkWfdKillProcess("udhcpc");
	sprintf(strcmd, "echo \"\"> %s;sync;sync", DHCPC_SERVER_IP_FILE_PATH);
	system(strcmd); 
	/* Starting DHCP server */
	fp = fopen(DHCPD_CONFIG_FILE_PATH, "r");
	if (!fp)
	{
		MTKSIGDBG("%s: file open failed\n", __FUNCTION__ );
		sprintf(strcmd, "ifconfig p2p0 %s ", DHCPD_DEF_STATIC_IP_ADDR);
	}
	else
	{
		char *sbuf = NULL;
		char *p_lease = NULL, *p_hwaddr = NULL;
		char ipstr[24];
		int found = 0, idx = 0;

		MTKSIGDBG("Parse the dhcpd.conf file ....\n");
		sbuf = malloc(WFD_MAX_STR_BUF);
		if (!sbuf)
		{
			MTKSIGDBG("buf allocate failed\n");
			return -1;
		}
		memset(sbuf, 0, WFD_MAX_STR_BUF);
		
		/* seek to the beginning of the file */
		if (fseek(fp, 0, SEEK_SET) < 0)
		{
			MTKSIGDBG("Failed seek to header\n");
			return -1;
		}
		
		while (fgets(sbuf, WFD_MAX_STR_BUF, fp))
		{
			if (sbuf[0] == '#') /* ignore commented line */
				continue;
			if ((p_lease = strstr(sbuf, "option routers ")) != NULL)
			{
				memset(ipstr, 0, sizeof(ipstr));
				found = 0;
				idx = 0;
				while (*p_lease)
				{
					if ((((*p_lease)>='0') &&((*p_lease)<='9')) || ((*p_lease)=='.'))
					{
						found = 1;
						ipstr[idx++] = *p_lease;
					}
					else
					{
						if (found)
							break;
					}
					p_lease++;
				} /* while (*p_lease) */
			}		 
			memset(sbuf, 0, WFD_MAX_STR_BUF);
		} /* while (fgets) */
		
		sprintf(strcmd, "ifconfig %s %s", WFA_STAUT_IF_P2P, ipstr);
		
		if (sbuf)
			free(sbuf);
	}
	if (fp)
		fclose(fp);
	fp = NULL ; 

	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);

	//sleep(1);
	sprintf(strcmd, "mtk_dhcp_server.sh %s", WFD_P2P_IF_NAME);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);
	
#ifdef MTK_WFD_SIGMA
		/* getting released ip address */
		MTKSIGDBG("Checking for client IP..\n");
		while(loop_cnt < MAX_DHCP_WAIT_CNT)
		{
			loop_cnt ++;
			sleep(1);
			if (!fp)
			{
				fp = fopen(DHCPD_LEASE_FILE_PATH, "r");
				if (!fp)
					continue;
			}

			if ((ret = wfaMtkWfdGetClientIp(fp, client_mac, return_ip, return_buf_len)) == 0)
				break;
		}
		if (loop_cnt >= MAX_DHCP_WAIT_CNT && ret != 0)
		{
			MTKSIGDBG("%s: Time out waiting for getting client's IP address\n", __FUNCTION__);
		}
		else
			MTKSIGDBG("Checking client Ip complete (ret = %d)..\n", ret);
			
		
		
		if (fp)
			fclose(fp);
#else
		ret = 1;
#endif
	return ret; 	   
#endif
}

#ifdef MTK_ANDROID_SIGMA
//@mac : interface mac address; should never be NULL 
int wfaMtkWfdGetClientIp_Android(FILE *fp, char *mac, char *return_ip, int return_buf_len)
{
    char sbuf[256] = {0};
    int ret = -1;
    char *p_lease = NULL;
    char ipstr[24];
    int idx = 0;
    int ip_got = 0;
	char * tmp_token = NULL;

    MTKSIGDBG("Entering %s ......\n", __FUNCTION__);

	if(mac == NULL)
	{
		MTKSIGDBG("MAC is null \n");
		return -1;
	}

    if (!fp || !return_ip)
        return -1;
	
    memset(sbuf, 0, 256);
	
	MTKSIGDBG("start to open the file\n");
	
    if (fseek(fp, 0, SEEK_SET) < 0)
    {
        MTKSIGDBG("Failed seek to header\n");
        return -1;
    }
	while (fgets(sbuf, 256, fp)!= NULL)
	{
		if (sbuf[0] == '#') /* ignore commented line */
			continue;
		MTKSIGDBG(">> <%s>%d \n",sbuf,strlen(sbuf));
		//MTKSIGDBG(">>>>> >%s<%d \n",mac,strlen(mac));
		
		if ((p_lease = strstr(sbuf, mac)) != NULL)
		{
			memset(ipstr, 0, sizeof(ipstr));
			idx = 0;
			tmp_token = strtok(p_lease, " ");
			MTKSIGDBG("Mac Address : %s\n",tmp_token);
			tmp_token = strtok(NULL, " ");			
			MTKSIGDBG("IP Address : %s\n",tmp_token);
			//tmp_token = strtok(NULL, " ");
			//MTKSIGDBG("Device name  : %s\n",tmp_token);
			ip_got = 1 ;

			while (tmp_token[idx])
			{
				if (((((tmp_token[idx])<'0')||((tmp_token[idx])>'9'))) && ((tmp_token[idx]) != '.'))
				{
					ip_got = 0 ;
					MTKSIGDBG("Invalid ip @(%d)!!!\n",idx);
					break;
				}
				else
				{
					idx++;
				}
			} 

			if(ip_got == 1)
			{
				strncpy(return_ip, tmp_token, strlen(tmp_token));
				MTKSIGDBG("%s: Found client IP = [%s]\n", __FUNCTION__, return_ip);
				ret = 0;
				break;
			}
		}
		memset(sbuf, 0, 256);
		ret = -1;
	}
	
	return ret;
}
#endif



int wfaMtkWfdGetClientIp(FILE *fp, char *mac, char *return_ip, int return_buf_len)
{
    char *sbuf = NULL;
    int ret = -1;
    char *p_lease = NULL, *p_hwaddr = NULL;
    char ipstr[24];
    int found = 0, idx = 0;
    int ip_got = 0;

    MTKSIGDBG("Entering %s, mac=%s\n", __FUNCTION__, mac);

    if (!fp || !return_ip)
        return -1;
    sbuf = malloc(WFD_MAX_STR_BUF);
    if (!sbuf)
    {
        MTKSIGDBG("buf allocate failed\n");
        return -1;
    }
    memset(sbuf, 0, WFD_MAX_STR_BUF);
    /* read from file */
    /* The lease file content looks like this:
    
              # All times in this file are in UTC (GMT), not your local timezone.   This is
              # not a bug, so please don't ask about it.   There is no portable way to
              # store leases in the local timezone, so please don't request this as a
              # feature.   If this is inconvenient or confusing to you, we sincerely
              # apologize.   Seriously, though - don't ask.
              # The format of this file is documented in the dhcpd.leases(5) manual page.
              # This lease file was written by isc-dhcp-V3.0.5
              lease 192.168.5.200 {
              starts 4 1970/01/01 00:06:04;
              ends 4 1970/01/01 06:06:04;
              binding state active;
              next binding state free;
              hardware ethernet 00:0c:43:22:66:72;
              uid "\001\000\014C\"fr";
              client-hostname "NB10110004";
            }
    */
    /* seek to the beginning of the file */
    if (fseek(fp, 0, SEEK_SET) < 0)
    {
        MTKSIGDBG("Failed seek to header\n");
        return -1;
    }

    while (fgets(sbuf, WFD_MAX_STR_BUF, fp))
    {
        if (sbuf[0] == '#') /* ignore commented line */
            continue;
        if ((p_lease = strstr(sbuf, "lease ")) != NULL)
        {
            memset(ipstr, 0, sizeof(ipstr));
            found = 0;
            idx = 0;
            while (*p_lease)
            {
                if ((((*p_lease)>='0') &&
((*p_lease)<='9')) || ((*p_lease)=='.'))
                {
                    found = 1;
                    ipstr[idx++] = *p_lease;
                    if (idx == return_buf_len-1)
                    {
                        MTKSIGDBG("Buf is insufficient!!!\n");
                        break;
                    }
                }
                else
                {
                    if (found)
                        break;
                }
                p_lease++;
            } /* while (*p_lease) */
#ifndef MTK_BDP_SIGMA
            if (!mac)
            {
                //strncpy(return_ip, ipstr, sizeof(ipstr));
                //MTKSIGDBG("%s: Found client IP = [%s]\n", __FUNCTION__, return_ip);
                //ret = 0;
                //break;
                ip_got = 1;
            }
#endif
        }
        if ((p_hwaddr = strstr(sbuf, "hardware ethernet"))!= NULL)
        {
#ifdef MTK_BDP_SIGMA
            if (!mac)
            {
                ip_got = 1;
                /* hw mac has provided, this is the real IP we delivered */
                strncpy(return_ip, ipstr, sizeof(ipstr));
                MTKSIGDBG("%s: Found client IP = [%s]\n", __FUNCTION__, return_ip);
                ret = 0;
                break;
            }
            else if (mac && strlen(ipstr))
            {
                /* if mac is provided, we need to match mac with this ip lease */
                if ((p_lease = strstr(sbuf, mac)) != NULL)
                {
                    /* found this mac, and this ip is correct */
                    strncpy(return_ip, ipstr, sizeof(ipstr));
                    MTKSIGDBG("%s: Found client IP = [%s] with mac=[%s]\n", __FUNCTION__, return_ip, mac);
                    ret = 0;
                    break;
                }
            }
            else
                MTKSIGDBG("%s: Warning IP check has some problems?\n", __FUNCTION__);
#else
            if (ip_got)
            {
                /* hw mac has provided, this is the real IP we delivered */
                strncpy(return_ip, ipstr, sizeof(ipstr));
                MTKSIGDBG("%s: Found client IP = [%s]\n", __FUNCTION__, return_ip);
                ret = 0;
                break;
            }
            else
                MTKSIGDBG("%s: Warning IP check has some problems?\n", __FUNCTION__);
#endif
        }
        if (strstr(sbuf, "}") != NULL)
        {
            ip_got = 0;
        }
        memset(sbuf, 0, WFD_MAX_STR_BUF);
        ret = -1;
    } /* while (fgets) */

    if (sbuf)
        free(sbuf);

    return ret;
}


int wfaMtkWfdStartDhcpClient(char *return_ip, int return_buf_len, char intf_idx)
{
#ifdef MTK_ANDROID_SIGMA
	char strcmd[256];	 
	int loop_cnt = 0;
	int ret = -1;
	memset(strcmd, 0, sizeof(strcmd));	
	char tmp_if_name[IFNAMSIZ] = {0};	 

	sprintf(strcmd, "mtk_dhcp_reset.sh");
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);
	sleep(3);	
	

	wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 


	if (intf_idx == 0)
	{
		sprintf(strcmd, "mtk_dhcp_client.sh %s",tmp_if_name);
		//sprintf(strcmd, "/system/bin/dhcpcd -A -BK -dd %s",tmp_if_name);
		MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
		system(strcmd);
	}
	else
	{
		sprintf(strcmd, "mtk_dhcp_client.sh %s",WFA_STAUT_IF);
		//sprintf(strcmd, "/system/bin/dhcpcd -A -BK -dd %s",WFA_STAUT_IF);
		MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
		system(strcmd);
	}
	MTKSIGDBG("Checking for server IP..\n");

	while(loop_cnt < MAX_DHCP_WAIT_CNT)
	{
		loop_cnt ++;
		sleep(1);
		if ((ret = wfaMtkWfdGetServerIp_Android(return_ip,intf_idx)) == 0)
			break;
		else
			continue;
	}
	if (loop_cnt >= MAX_DHCP_WAIT_CNT && ret != 0)
	{
		MTKSIGDBG("%s: Time out waiting for getting server's IP address\n", __FUNCTION__);
	}
	return ret;  

#else
	char strcmd[256];	 
	FILE* fp = NULL;
	int loop_cnt = 0;
	int ret = -1;
	//struct stat fstat; 
    char tmp_if_name[IFNAMSIZ] = {0};

	memset(strcmd, 0, sizeof(strcmd));
	wfaMtkWfdKillProcess("dhcpd");
	sprintf(strcmd, "echo \"\"> %s;sync;sync", DHCPD_LEASE_FILE_PATH);
	system(strcmd); 

	wfaMtkWfdKillProcess("udhcpc");
	sprintf(strcmd, "echo \"\"> %s;sync;sync", DHCPC_SERVER_IP_FILE_PATH);
	system(strcmd);

    wpa_get_if_by_role(p2p_priv_p, &tmp_if_name);
	/* Starting DHCP client */	  
	if (intf_idx == 0)
	{
		sprintf(strcmd, "mtk_dhcp_client.sh %s", tmp_if_name);
		MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
		system(strcmd);
	}
	else
	{
		sprintf(strcmd, "mtk_dhcp_client.sh %s", "wlan0");
		MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
		system(strcmd);
	}
	MTKSIGDBG("Checking for server IP..\n");

	/* getting released ip address */
	while(loop_cnt < MAX_DHCP_WAIT_CNT)
	{
		loop_cnt ++;
		sleep(1);
		if (!fp)
		{
			fp = fopen(DHCPC_SERVER_IP_FILE_PATH, "r");
			if (!fp)
			{
				MTKSIGDBG("%s: file open failed\n", __FUNCTION__);
				continue;
			}
		}

		if ((ret = wfaMtkWfdGetServerIp(fp, return_ip)) == 0)
			break;
	}
	if (loop_cnt >= MAX_DHCP_WAIT_CNT && ret != 0)
	{
		MTKSIGDBG("%s: Time out waiting for getting server's IP address\n", __FUNCTION__);
	}
	if (fp)
		fclose(fp);

	return ret;    
#endif	
}

#ifdef MTK_ANDROID_SIGMA
int wfaMtkWfdGetServerIp_Android(char *return_ip,char intf_idx)
{
    int ret,i = 0;
	char strcmd[256] = {0};	
    FILE *tmpfd;
    char string[256] = {0};
    char *str;
	char config_path[256] = {0};
	char tmp_if_name[IFNAMSIZ] = {0};	

	MTKSIGDBG("enter wfaMtkWfdGetServerIp_Android ... \n");
	wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 
	
    if (!return_ip)
        return -1;
	
	sprintf(config_path,"mtk_p2p_getipconfig.sh");


    ret = access(config_path, F_OK);
    if(ret == -1)
    {
    	MTKSIGDBG("config_path:%s NOT exist\n",config_path);
		return -1;
    }

	sprintf(strcmd, "%s /tmp/sigma_ipconfig.txt %s\n",config_path,tmp_if_name);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
	system(strcmd);

	tmpfd = fopen("/tmp/sigma_ipconfig.txt", "r+");
    if(tmpfd == NULL)
    {
    	MTKSIGDBG("/tmp/sigma_ipconfig.txt open FAILED\n");
		return -1;
    }

	for(;;)
	{
        if(fgets(string, 256, tmpfd) == NULL)
            {
                fclose(tmpfd);
                MTKSIGDBG("fgets NULL, /tmp/sigma_ipconfig.txt:  %s \n", string);
                return -1;
            }
        MTKSIGDBG("/tmp/sigma_ipconfig.txt:  %s \n", string);    
        /* find out the dns server ip address */
        if(strncmp(string, "dhcpserver", 10) == 0)
        {
            str = strtok(string, "=");
            str = strtok(NULL, " ");
            if(str != NULL && str[0] != '\n' && str[0] != '\0' && (strlen(str)>5))
            {
				int str_len = strlen(str);
				str[str_len-1] ='\0';
				MTKSIGDBG("return_ip: %s(%d)\n",str,str_len);
				//memcpy(return_ip, str, strlen(str));	  
				while(i < str_len )
				{
					return_ip[i] = str[i];
					i++;
				}
				break;
			}
			else
			{
				MTKSIGDBG("return_ip is NULL\n");
                                   fclose(tmpfd);
				return -1;
			}
		}
	 }
	if (tmpfd)
		fclose(tmpfd);

	return 0;
}
#endif


int wfaMtkWfdGetServerIp(FILE *fp, char *return_ip)
{
    char *sbuf = NULL;
    int ret = -1;
    char *ptr = NULL;
    int found = 0, idx = 0;

    MTKSIGDBG("Entering %s\n", __FUNCTION__);

    if (!fp || !return_ip)
        return -1;
    sbuf = malloc(WFD_MAX_STR_BUF);
    if (!sbuf)
    {
        MTKSIGDBG("buf allocate failed\n");
        return -1;
    }
    memset(sbuf, 0, WFD_MAX_STR_BUF);
    /* read from file */
    /* The server ip file content looks like this:

        192.168.123.1
    */
    /* seek to the beginning of the file */
    if (fseek(fp, 0, SEEK_SET) < 0)
    {
        MTKSIGDBG("Failed seek to header\n");
        ret = -1;
        goto out;
    }
    while (fgets(sbuf, WFD_MAX_STR_BUF, fp))
    {
        idx = 0;
        ptr = sbuf;        
        while (ptr && *ptr)
        {
            if ((((*ptr)>='0') &&
((*ptr)<='9')) || ((*ptr)=='.'))
            {
                return_ip[idx++] = *ptr;
                found = 1;
            }
            else
            {
                if (found)
                {
                    return_ip[idx] = 0x00;
                    MTKSIGDBG("%s: Found server IP = [%s]\n", __FUNCTION__, return_ip);
                    ret = 0;
                    goto out;
                }
            }
            ptr++;
        }
    }

out:
    if (sbuf)
        free(sbuf);
    return ret;
}


int wfaMtkWfdP2pGetIfConfig(char *macbuf, char *ipbuf, char *maskbuf)
{
    char cmdStr[256];
    FILE* fp = NULL;
    char *ptr = NULL;
    int mac_found = 0, ip_found = 0, mask_found = 0;
    int idx = 0;
	char tmp_ifname[128] = {0};
     
    if (!macbuf || !ipbuf || !maskbuf)
        return -1;

    memset(cmdStr, 0, sizeof(cmdStr));
    wpa_get_if_by_role(p2p_priv_p, tmp_ifname);
    sprintf(cmdStr, "busybox ifconfig %s > %s;sync;sync;", tmp_ifname, WFD_P2P_GET_IFCONFIG_PATH);
    system(cmdStr);

    memset(cmdStr, 0, sizeof(cmdStr));
    fp = fopen(WFD_P2P_GET_IFCONFIG_PATH, "r");
    if (!fp)
    {
        MTKSIGDBG("%s: file open failed\n", __FUNCTION__);
        return -1;
    }
    while (fgets(cmdStr, sizeof(cmdStr), fp))
    {
        if ((ptr = strstr(cmdStr, "HWaddr ")) != NULL)
        {
            /* found mac */
            ptr += strlen("HWaddr ");
            strncpy(macbuf, ptr, 17);
            mac_found = 1;
        }
        if ((ptr = strstr(cmdStr, "inet addr:")) != NULL)
        {
            /* found ip */
            ptr += strlen("inet addr:");
            idx = 0;
            while ((((*ptr)>='0') && ((*ptr)<='9')) || ((*ptr)=='.'))
            {
                ipbuf[idx++] = *ptr;
                ptr ++;
            }
            ip_found = 1;
        }
        if ((ptr = strstr(cmdStr, "Mask:")) != NULL)
        {
            /* found mask */
            ptr += strlen("Mask:");
            idx = 0;
            while ((((*ptr)>='0') && ((*ptr)<='9')) || ((*ptr)=='.'))
            {
                maskbuf[idx++] = *ptr;
                ptr ++;
            }
            mask_found = 1;
        }
        memset(cmdStr, 0, sizeof(cmdStr));
    }

    if (fp)
        fclose(fp);
    if (mac_found && ip_found && mask_found)
        return 0;
    else
        return -1;
    
}

int wfdMtkWfdKillDhcp(void)
{
    char strcmd[256]="";
    memset(strcmd, 0, sizeof(strcmd));

#ifdef MTK_ANDROID_SIGMA
    /* killing previous dhcp processes */
      sprintf(strcmd, "mtk_dhcp_reset.sh");

    system(strcmd);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);

    sprintf(strcmd, "mtk_dhcp_reset.sh");
    system(strcmd);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
#else
	sprintf(strcmd, "mtk_dhcp_reset.sh");
	system(strcmd);
	sprintf(strcmd, "echo \"\"> %s;sync;sync", DHCPD_LEASE_FILE_PATH);
	system(strcmd);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);

	sprintf(strcmd, "mtk_dhcp_reset.sh");
	system(strcmd);
	sprintf(strcmd, "echo \"\"> %s;sync;sync", DHCPC_SERVER_IP_FILE_PATH);
	system(strcmd);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
#endif

    return 0;
}

int wfdMtkWfdKillWpaSupplicant(void)
{
    char strcmd[256]="";
    memset(strcmd, 0, sizeof(strcmd));
#ifdef MTK_ANDROID_SIGMA	
    sprintf(strcmd, "busybox killall wpa_supplicant");
    system(strcmd);
	MTKSIGDBG("Executing cmd: [%s]\n", strcmd);
#else
	wfaMtkWfdKillProcess("wpa_supplicant");
#endif
    return 0;
}

int wfdMtkWfdSigmaStaGetPingResult(char *fpath, int *reqCnt, int *replyCnt)
{
    FILE* fp = NULL;
    char sbuf[512];
    char number_str[64];
    char *ptr = NULL, *ptr2 = NULL;
    
    fprintf(stderr, "Enter %s...\n", __FUNCTION__);
    if (!fpath|| !reqCnt || !replyCnt)
    {
        return -1;
    }
    fp = fopen(fpath, "r");
    if (!fp)
        return -2;
    memset(sbuf, 0, sizeof(sbuf));
    /* Pattern as below: 
        10 packets transmitted, 10 received, 0% packet loss, time 4507ms
     */
    #define PING_REQ_PATTERN_STRING     " packets transmitted"
    #define PING_REPLY_PATTERN_STRING   " received"
    while (fgets(sbuf, sizeof(sbuf), fp))
    {
        if ((ptr = strstr(sbuf, PING_REQ_PATTERN_STRING)) != NULL)
        {
            /* find requset sent number */
            memset(number_str, 0, sizeof(number_str));
            memcpy(number_str, sbuf, (unsigned int)(ptr - sbuf));
            *reqCnt = atoi(number_str);
            fprintf(stderr, "%s: reqCnt = %d\n", __FUNCTION__, *reqCnt);
            /* find reply sent number */
            ptr += strlen(PING_REQ_PATTERN_STRING);
            ptr += 2; /* skip ", " */
            if ((ptr2 = strstr(ptr, PING_REPLY_PATTERN_STRING)) != NULL)
            {
                memset(number_str, 0, sizeof(number_str));
                memcpy(number_str, sbuf, (unsigned int)(ptr2 - ptr));
                *replyCnt = atoi(number_str);
                fprintf(stderr, "%s: replyCnt = %d\n", __FUNCTION__, *replyCnt);
                break;
            }
        }
        memset(sbuf, 0, sizeof(sbuf));
    }

    fclose(fp);
    return 0;
}


int wfdMtkWfdIfUp(void)
{
    char strcmd[256]="";
	
    memset(strcmd, 0, sizeof(strcmd));

    return 0;
}

int wfdMtkWfdSigmaStaResetDefault(void)
{
    fprintf(stderr, "enter wfdMtkWfdSigmaStaResetDefault ...\n");

	char strcmd[256]={0};
	char tmp_if_name[IFNAMSIZ] = {0};	 

	
	wpa_get_if_by_role(p2p_priv_p, &tmp_if_name); 	

    memset(strcmd, 0, sizeof(strcmd));
#ifdef MTK_CFG80211_SIGMA
	sprintf(strcmd, "wpa_cli -i %s %s p2p_stop_find", WFA_STAUT_IF_P2P, ctrl_if);
	system(strcmd);
    fprintf(stderr, "system: %s\n",strcmd);

	sprintf(strcmd, "wpa_cli -i %s %s p2p_flush", WFA_STAUT_IF_P2P, ctrl_if);
	system(strcmd);
    fprintf(stderr, "system: %s\n",strcmd);

	
#else
	sprintf(strcmd, "iwpriv %s set p2pCleanTable=1", WFA_STAUT_IF_P2P);
	system(strcmd);
    fprintf(stderr, "system: %s\n",strcmd);
#endif
	sleep(1);

	sprintf(strcmd, "rm -fr /tmp/sigma_*");
	system(strcmd);
	fprintf(stderr, "system: %s\n",strcmd);

#ifdef MTK_CFG80211_SIGMA
	;
#else
	wfdMtkWfdKillWpaSupplicant();
#endif
	wfdMtkWfdKillDhcp();
    sleep(2);
    wfdMtkWfdIfUp();
    wfaMtkWfdCmd_rtspConfReset();
	
    return 0;
}

int p2pMtkP2pdSigmaStaResetDefault(void)
{
    fprintf(stderr, "Sta Resetting default for P2P\n");

	char strcmd[256]={0};
	char tmp_if_name[IFNAMSIZ] = {0};	 

	
	wpa_get_if_by_role(p2p_priv_p, &tmp_if_name);	

    memset(strcmd, 0, sizeof(strcmd));
#ifdef MTK_CFG80211_SIGMA
	sprintf(strcmd, "wpa_cli -i %s %s p2p_flush", WFA_STAUT_IF_P2P, ctrl_if);
	system(strcmd);
	fprintf(stderr, "system: %s\n",strcmd);
#else
	sprintf(strcmd, "iwpriv %s set p2pCleanTable=1", WFA_STAUT_IF_P2P);
	system(strcmd);
	fprintf(stderr, "system: %s\n",strcmd);
#endif
	sleep(1);

    wfdMtkWfdKillWpaSupplicant();
    wfdMtkWfdKillDhcp();
    sleep(2);
    wfdMtkWfdIfUp();
	
    return 0;
}


#ifdef MTK_WFD_SIGMA
static int quitConnCheck = 0;

int wfaMtkWfdQuitConnCheckClear(void)
{
    char cmds[128];

    /* There are two ways controlling Quit connCheck
        1. quitConnCheck flag (used by wfa_dut internal thread)
        2. MTK_DTV_QUICK_CONN_CHECK_FILENAME, used by wfa_ca, in such case, wfa_dut may be busying in a loop,
           so will not be able to clear the quitConnCheck flag.
     */
    quitConnCheck = 0;    
    if (remove(MTK_DTV_QUICK_CONN_CHECK_FILENAME) != 0)
        fprintf(stderr, "Remove %s failed\n", MTK_DTV_QUICK_CONN_CHECK_FILENAME);
    return 0;
}

int wfaMtkWfdQuitConnCheck(void)
{
    /* return 0: don't quit
              1: quit
     */
    if (quitConnCheck || (access(MTK_DTV_QUICK_CONN_CHECK_FILENAME, F_OK ) != -1))
    {
        /* exists */
        return 1;
    }
    else
    {
        /* not exist */
        return 0;
    }
}


int wfdMtkWfdCheckP2pConnResult(unsigned char *retcode, unsigned char *conn_mac, unsigned int size_of_mac)
{
    int conn_cnt = 0;
    unsigned char res = 0;

    fprintf(stderr, "Entering P2P Connection Result Check...\n");

    wfaMtkWfdQuitConnCheckClear();
    do {
        if (wfaMtkWfdQuitConnCheck())
        {
            wfaMtkWfdQuitConnCheckClear();
            fprintf(stderr, "staResetDefault, quit checking P2P connection...\n");
            break;
        }
        sleep(WFD_P2P_CONN_CHECK_INTERVAL_SEC);

        if (conn_mac && size_of_mac && ((conn_cnt%WFD_RETRY_CONNECT_INTERVAL) ==(WFD_RETRY_CONNECT_INTERVAL-1)))
        {
			#ifdef MTK_CFG80211_SIGMA
				fprintf(stderr ,"[CFG80211] go through this part (%d)... \n",conn_cnt);
			#else
	            fprintf(stderr ,"Reissuing connect OID command (mac=%s)"
                    "(retry since=%d sec)\n", (char *)conn_mac, conn_cnt);            
				wfa_driver_ralink_set_oid(OID_802_11_P2P_CONNECT_ADDR, (char *)conn_mac, (int)size_of_mac);
			#endif
        }

			if(p2p_priv_p->bNegoDone != 0)
			{
				if(p2p_priv_p->role == GO)
					res =P2P_GO;
				else if(p2p_priv_p->role == GC)
					res =P2P_CLIENT;
			}
		
        fprintf(stderr, "Conn-P2P Check Result=%d\n", res);
        conn_cnt++;
        if (conn_cnt >= MAX_P2P_CONN_RESULT_CHECK_COUNT)
            res = P2P_DEVICE;
    } while(res == 0);
    *retcode = res;
    fprintf(stderr, "P2P Connection Result Check finished, result=%d(%s)\n", 
        res, (res==P2P_DEVICE||res==WFD_SESSION_UNAVALIBLE)?"Not Connected":"Connected");
   
    return 0;
}


int wfdMtkWfdCheckTdlsConnResult(unsigned char *retcode, unsigned char *dev_addr)
{
    int conn_cnt = 0;
	FILE *tmpfile = NULL;
	char result[32];
	char cmdStr[128];

    fprintf(stderr, "Entering TDLS Connection Result Check...\n");

    wfaMtkWfdQuitConnCheckClear();   
    do {
        sleep(WFD_TDLS_CONN_CHECK_INTERVAL_SEC);
        
        if (wfaMtkWfdQuitConnCheck())
        {
            wfaMtkWfdQuitConnCheckClear();
            fprintf(stderr, "staResetDefault, quit checking TDLS connection...\n");
            break;
        }
    
		#ifdef MTK_CFG80211_SIGMA
		{
			sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_link_state= | busybox cut -f2- -d= > /tmp/tdlsLinkState", WFA_STAUT_IF, ctrl_if);
			system(cmdStr);
			tmpfile = fopen("/tmp/tdlsLinkState", "r+");
			if(tmpfile == NULL)
			{
				/* hope not happen */
				DPRINT_ERR(WFA_ERR, "file open failed\n");
				return WFA_FAILURE;
			}
			fscanf(tmpfile, "%s", result);
			*retcode = atoi(result);
		}
		#else
        	wfa_driver_ralink_get_oid((unsigned char *)WFA_STAUT_IF, OID_802_11_QUERY_WFD_TDLS_CONNECT_STATUS, retcode, sizeof(retcode));
		#endif
		
        fprintf(stderr, "Conn-TDLS Check Result=%d (retry=%d)\n", *retcode, conn_cnt);
        if (((*retcode) == TDLS_LINKED) ||
			((*retcode) == WFD_PEER_PC_P2P) ||
			((*retcode) == WFD_PEER_TDLS_WEAK_SECURITY))
			break;
        else
        {
            if (dev_addr)
        	{
				#ifdef MTK_CFG80211_SIGMA
					wfa_cfg80211_tdls_setup(dev_addr);
				#else
	        		wfa_driver_ralink_tdls_setup(dev_addr);
				#endif
            }
        }
        conn_cnt++;
        if (conn_cnt >= MAX_TDLS_CONN_RESULT_CHECK_COUNT)
            (*retcode) = P2P_DEVICE;
    } while((*retcode) == 0);

    return 0;

}


int wfdMtkWfdCheckP2pAndTdlsConnResult(unsigned char *retcode, unsigned char *dev_addr)
{
    int conn_cnt = 0;
	FILE *tmpfile = NULL;
	char result[32];
	char cmdStr[128];

    fprintf(stderr, "Entering P2P & TDLS Connection Result Check...\n");

    wfaMtkWfdQuitConnCheckClear();   
    do {
        sleep(WFD_TDLS_CONN_CHECK_INTERVAL_SEC);
        
        if (wfaMtkWfdQuitConnCheck())
        {
            wfaMtkWfdQuitConnCheckClear();
            fprintf(stderr, "staResetDefault, quit checking P2P & TDLS connection...\n");
            break;
        }

        /* check TDLS here */
		#ifdef MTK_CFG80211_SIGMA
		{
			sprintf(cmdStr, "wpa_cli -i%s %s tdls_status | grep ^tdls_link_state= | busybox cut -f2- -d= > /tmp/tdlsLinkState", WFA_STAUT_IF, ctrl_if);
			system(cmdStr);
			tmpfile = fopen("/tmp/tdlsLinkState", "r+");
			if(tmpfile == NULL)
			{
				/* hope not happen */
				DPRINT_ERR(WFA_ERR, "file open failed\n");
				return WFA_FAILURE;
			}
			fscanf(tmpfile, "%s", result);
			*retcode = atoi(result);
		}
		#else
			wfa_driver_ralink_get_oid((unsigned char *)WFA_STAUT_IF, OID_802_11_QUERY_WFD_TDLS_CONNECT_STATUS, retcode, sizeof(retcode));
		#endif
		
        fprintf(stderr, "Conn-TDLS Check Result=%d (retry=%d)\n", *retcode, conn_cnt);
        if (((*retcode) == TDLS_LINKED) ||
			((*retcode) == WFD_PEER_PC_P2P) ||
			((*retcode) == WFD_PEER_TDLS_WEAK_SECURITY))
			break;
        else
        {
            if (dev_addr)
            {
				#ifdef MTK_CFG80211_SIGMA
					wfa_cfg80211_tdls_setup(dev_addr);
				#else
	        		wfa_driver_ralink_tdls_setup(dev_addr);
				#endif
            }
        }

        /* Check P2P here */
		#ifdef MTK_CFG80211_SIGMA
			;
		#else
        	wfa_driver_ralink_get_oid((unsigned char *)WFA_STAUT_IF_P2P, OID_802_11_P2P_CONNECT_STATUS, retcode, sizeof(retcode));
		#endif
		
        fprintf(stderr, "Conn-P2P Check Result=%d (retry=%d)\n", *retcode, conn_cnt);
        if ((*retcode) != 0)
            break;

        conn_cnt++;
        if (conn_cnt >= MAX_TDLS_CONN_RESULT_CHECK_COUNT)
        {
            (*retcode) = P2P_DEVICE;
        }
    } while((*retcode) == 0);

    return 0;

}


int wfdMtkWfdQuitConnectCheck(void)
{
    quitConnCheck = 1;
    
    return 0;
}
#endif /* MTK_WFD_SIGMA */
#endif

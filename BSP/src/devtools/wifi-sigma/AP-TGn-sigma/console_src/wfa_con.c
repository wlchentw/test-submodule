/****************************************************************************
*
* Copyright (c) 2016 Wi-Fi Alliance
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
* RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
* NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
* USE OR PERFORMANCE OF THIS SOFTWARE.
*
*****************************************************************************/
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
//#include <rpc/rpc.h>
#include <linux/ip.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "wfa_con.h"
#include <linux/types.h>
#include <linux/socket.h>
#include <arpa/inet.h>
#include "wfa_debug.h"
#include "wfa_utils.h"
#include "mpx.h"

unsigned int rmsg[512];         // rx msg buffer
unsigned int txmsg[512];        // tx msg buffer

/* Debug message flags */
unsigned short wfa_defined_debug = WFA_DEBUG_ERR | WFA_DEBUG_WARNING | WFA_DEBUG_INFO;

/*
* printf time stamp like android log
*/
void build_timestamp(char *time_buf, int buf_len)
{
	char timebuf[32];
	struct timeval timeval;
	struct tm *tm;

	memset(timebuf, 0, 32);
	memset(time_buf, 0, 128);

	gettimeofday(&timeval, NULL);

	tm = localtime(&timeval.tv_sec);
	strftime(timebuf, sizeof(timebuf), "%m-%d %H:%M:%S", tm);
	snprintf(time_buf, 128, "%s.%03ld", timebuf, timeval.tv_usec / 1000);
}

void mtk_printf(const char *fmt, ...)
{
	char msg[4096];
	char time_str[128];
	va_list params;
	va_start(params, fmt);
	vsnprintf(msg, sizeof(msg), fmt, params);
	build_timestamp(time_str, sizeof(time_str));
	printf("%s %s", time_str, msg);
	va_end(params);
}
void mtk_fprintf(FILE *fp, const char *fmt, ...)
{
	char msg[4096];
	char time_str[128];
	va_list params;
	va_start(params, fmt);
	vsnprintf(msg, sizeof(msg), fmt, params);
	build_timestamp(time_str, sizeof(time_str));
	fprintf(fp, "%s %s", time_str, msg);
	va_end(params);
}

#define MAXRETRY    3           // max retry count
#define SLEEP_PERIOD 10         //time to exit itself
#define INTERSTEP_PERIOD 5      //time allowed within steps
int reset        = 0;
int tout         = 0;
int ap_reset_recd   = 0;
int ap_num_retry    = 0;
int tos_vo, tos_vi, tos_be, tos_bk;
int timeron = SLEEP_PERIOD;
int exitflag = 0;
pthread_t time_thr;

struct apts_msg ap_apts_msgs[] =
{
    {0, -1},
    {"B.D", B_D},
    {"B.H", B_H},
    {"B.B", B_B},
    {"B.M", B_M},
    {"M.D", M_D},
    {"B.Z", B_Z},
    {"M.Y", M_Y},
    {"L.1", L_1},
    {"A.Y", A_Y},
    {"B.W", B_W},
    {"A.J", A_J},
    {"M.V", M_V},
    {"M.U", M_U},
    {"A.U", A_U},
    {"M.L", M_L},
    {"B.K", B_K},
    {"M.B", M_B},
    {"M.K", M_K},
    {"M.W", M_W},
    {"APTS TX         ", APTS_DEFAULT },
    {"APTS Hello      ", APTS_HELLO },
    {"APTS Broadcast  ", APTS_BCST },
    {"APTS Confirm    ", APTS_CONFIRM},
    {"APTS STOP       ", APTS_STOP},
    {"APTS CK BE      ", APTS_CK_BE },
    {"APTS CK BK      ", APTS_CK_BK },
    {"APTS CK VI      ", APTS_CK_VI },
    {"APTS CK VO      ", APTS_CK_VO },
    {"APTS RESET      ", APTS_RESET },
    {"APTS RESET RESP ", APTS_RESET_RESP },
    {"APTS RESET STOP ", APTS_RESET_STOP },
    {0, 0 }     // APTS_LAST
};

struct station;
int WfaConRcvHello(struct station *,unsigned int *,int );
int WfaConRcvConf(struct station *,unsigned int *,int );
int WfaConRcvVOSnd(struct station *,unsigned int *,int );
int WfaConRcvVOSndCyclic(struct station *,unsigned int *,int );
int WfaConRcvVOSndE(struct station *,unsigned int *,int );
int WfaConRcvVOE(struct station *,unsigned int *,int );
int WfaConWaitStop(struct station *,unsigned int *,int );
int WfaConRcvVOSnd2VO(struct station *,unsigned int *,int );
int WfaConRcvVO(struct station *,unsigned int *,int );
int WfaConRcvVI(struct station *,unsigned int *,int );
int WfaConRcvBE(struct station *,unsigned int *,int );
int WfaConRcvBKE(struct station *,unsigned int *,int );
int WfaConRcvVIE(struct station *,unsigned int *,int );
int WfaConRcvVISndBE(struct station *,unsigned int *,int );
int WfaConRcvVISndBK(struct station *,unsigned int *,int );
int WfaConRcvVISnd(struct station *,unsigned int *,int );
int WfaConRcvVISndE(struct station *,unsigned int *,int );
int WfaConRcvVISndVOE(struct station *,unsigned int *,int );
int WfaConRcvConfSndVI(struct station *,unsigned int *,int );
int WfaConRcvVIOnlySndBcastE(struct station *,unsigned int *,int );
int WfaConRcvVOSndBcastE(struct station *,unsigned int *,int );
int WfaConRcvBESnd(struct station *,unsigned int *,int );
int WfaConRcvBESndBcastE(struct station *,unsigned int *,int );
int WfaConRcvVISndBcast (struct station *,unsigned int *,int );
int WfaConRcvBESndBcast (struct station *,unsigned int *,int );
int WfaConRcvVISndBcastE (struct station *,unsigned int *,int );
int WfaConRcvVOSndAllE(struct station *,unsigned int *,int );
int WfaConRcvVISndVIE(struct station *,unsigned int *,int );
int WfaConRcvVOSndVOE(struct station *,unsigned int *,int );
int WfaConRcvVOSndVO(struct station *,unsigned int *,int );
int WfaConRcvBESndE(struct station *,unsigned int *,int );

char traceflag=1;   // enable debug packet tracing

consoleProcStatetbl_t consoleProcStatetbl[LAST_TEST + 1][10] =
{
    /* Ini*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVOSnd,WfaConWaitStop,0,0,0,0,0,0},
    /* B.D*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVOSnd,WfaConRcvVOE,WfaConWaitStop,WfaConWaitStop ,0,0,0,0},
    /* B.H*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVOSnd2VO,WfaConRcvVOE,WfaConWaitStop      ,WfaConWaitStop      ,0,0,0,0},
    /* B.B*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVO   ,WfaConRcvVI               ,WfaConRcvBE         ,WfaConRcvBKE        ,WfaConWaitStop,0,0,0},
    /* B.M*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVIE   ,WfaConWaitStop            ,0,0,0,0,0,0},

    /* M.D */
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISndBE    ,WfaConRcvVISndBK ,WfaConRcvVISnd      ,WfaConRcvVISndVOE    ,WfaConWaitStop      ,0,0,0},
    /* B.Z*/
    {WfaConRcvHello,WfaConRcvConfSndVI,WfaConRcvVOSndBcastE,WfaConWaitStop      ,0,0,0,0},
    /* M.Y*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvVO,WfaConRcvBESnd,WfaConRcvBESndBcastE,WfaConWaitStop,0,0,0},
    /* L.1*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVOSndCyclic,  WfaConWaitStop      ,0,0,0,0,0,0},
    /* A.Y*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvVO                 ,WfaConRcvBESnd        ,WfaConRcvBE         ,WfaConRcvBESndBcastE,WfaConWaitStop,0,0},
    /* B.W*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISndBcast,   WfaConRcvVISndBcast       ,WfaConRcvVIE,WfaConWaitStop      ,0,0,0,0},
    /* A.J*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVO,           WfaConRcvVOSndAllE    ,WfaConWaitStop      ,0,0,0,0,0},

    /* M.V*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvBESnd  ,WfaConRcvVISndE   ,WfaConWaitStop      ,0,0,0,0},
    /* M.U*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvBESnd  ,WfaConRcvVOSnd    ,WfaConRcvVOSndE   ,WfaConWaitStop      ,0,0,0},
    /* A.U*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvBE    ,WfaConRcvBESnd    ,WfaConRcvBE         ,WfaConRcvVOSnd    ,WfaConRcvVOE,WfaConWaitStop,0},
    /* M.L*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvBESndE,       WfaConWaitStop ,0,0,0,0,0,0},

    /* B.K*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvBESnd   ,WfaConRcvVIE    ,WfaConWaitStop      ,0,0,0,0},
    /* M.B*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVO,           WfaConRcvVI     ,WfaConRcvBE         ,WfaConRcvBKE        ,WfaConWaitStop      ,0,0,0},
    /* M.K*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISnd,        WfaConRcvBESnd ,WfaConRcvVIE   ,WfaConWaitStop      ,0,0,0,0},
    /* M.W*/
    {WfaConRcvHello,WfaConRcvConf,WfaConRcvVISndBcast,   WfaConRcvBESndBcast,WfaConRcvVIE,WfaConWaitStop      ,0,0,0,0}
};

unsigned int fromlen;           // sizeof socket struct
unsigned char dscp, ldscp;      // new/last dscp output values
struct sockaddr dst;            // sock declarations
struct sockaddr_in target;
struct sockaddr_in from;
struct sockaddr_in local;
int sockflags;                  // socket call flag
int nsta=0;                     // Number of stations
struct station stations[NSTA];
char *procname;                 // dst system name or ip address
int sd,rd;                      // socket descriptor


void WfaConResetAll();

char *src_ip;
char *dest_ip;
int src_port;
int dest_port;
char *net_mask;
char *inf_name;

extern void mpx(char *m, void *buf_v, int len);

void IAmDead() {
    DPRINT_INFO(WFA_OUT, "Time to Die...\n");
    exit(-10);
}

void WfaConResetAll() {
    int r;

    DPRINT_INFO(WFA_OUT, "Entering WfaConResetAll:: \n");
    reset = 1;
    alarm(0);
    ap_num_retry++;
    if (ap_num_retry > MAXRETRY)
        IAmDead();

    if (ap_reset_recd) {
        ap_reset_recd = 0;
        set_dscp(sd, tos_be);
        ap_create_apts_msg(APTS_RESET_RESP, txmsg, 0);
        txmsg[1] = tos_be;
        r = sendto(sd, txmsg, 190, sockflags, (struct sockaddr *) &from, sizeof(from));
        if (traceflag)
            mpx("CMD send\n", txmsg, 64);
        DPRINT_INFO(WFA_OUT, "sent RESET RESP\n");
    } else {
        int resp_recd = 0;
        ap_create_apts_msg(APTS_RESET, txmsg, 0);
        txmsg[1] = tos_be;
        r = sendto(sd, txmsg, 190, sockflags, (struct sockaddr *) &from, sizeof(from));
        if (traceflag)
            mpx("CMD send\n", txmsg, 64);
        DPRINT_INFO(WFA_OUT, "sent RESET \n");
        while (!resp_recd) {
            r = recvfrom(sd, rmsg, sizeof(rmsg), 0, (struct sockaddr *) &from, &fromlen);
            if (r < 0) {
                DPRINT_ERR(WFA_ERR, "rcv error:%d\n", r);
                exit(1);
            }
            if (rmsg[10] != APTS_RESET_RESP)
                continue;
            if (traceflag)
                mpx("CMD recd\n", rmsg, 64);
            alarm(0);
            resp_recd = 1;
        }/* while  */
    }
}

void resettimer(int period) {
    timeron = period;
}

void* timerthread(void* period) {
    int* per = (int*) period;
    exitflag = 1;
    while (1) {
        sleep(1);
        timeron--;
        if (!timeron && exitflag) {
            DPRINT_INFO(WFA_OUT, "time out\n");
            exit(1);
        }
    }
}

main(int argc, char **argv) {
    int r, flags = 0, n, i, id, base = 10, bKeepFrom = 0;
    struct apts_msg *testcase;
    struct station *sta = NULL;
    char IP[INET_ADDRSTRLEN], string[128];
    char *str, *endptr;
    FILE *tmpfile;
    consoleProcStatetbl_t func;
    fd_set rfds;
    struct timeval tv;
    int dscp;

    if (argv[1][0] == '-' && argv[1][1] == 't') {
        timeron = atoi(argv[2]);
        pthread_create(&time_thr, NULL, timerthread, NULL);
    }

    procname = argv[3]; // gather non-option args here
    if ((strncmp(procname, "L.1AP", 4) == 0)) {
        procname[3] = '\0';
        bKeepFrom = 1;
    } else {
        bKeepFrom = 0;
    }

    src_ip = argv[4];
    dest_ip = argv[5];
    src_port = atoi(argv[6]);
    dest_port = atoi(argv[7]);
    net_mask = argv[8];
    dscp = atoi(argv[9]);
    inf_name = argv[10];

    DPRINT_INFO(WFA_OUT, "src_ip: %s, dest_ip: %s, src_port: %d, dest_port: %d, net_mask: %s, traffic_class: %d, inf_name: %s\n",
            src_ip,
            dest_ip,
            src_port,
            dest_port,
            net_mask,
            dscp,
            inf_name);

    setup_socket(inf_name, dest_port, dscp);
    DPRINT_INFO(WFA_OUT, "sd: %d, rd: %d\n", sd, rd);

    testcase = (struct apts_msg *) apts_lookup(procname);
    DPRINT_INFO(WFA_OUT, "testcase name: %s, cmd: %d\n", testcase->name, testcase->cmd);
    sockflags = MSG_DONTWAIT;
    tos_vo = 0xD0;
    tos_vi = 0xA0;
    tos_be = 0x00;
    tos_bk = 0x20;
    tmpfile = fopen(TEMP_OUTPUT_PATH "tos.txt", "r+");
    if (tmpfile == NULL) {
        DPRINT_INFO(WFA_OUT, "Can not find the tos file,proceeding with default values\n");
    } else {
        while (fgets(string, 128, tmpfile) != NULL) {
            if (strstr(string, "#"))
                continue;
            if (strstr(string, "0x")) {
                base = 16;
            }
            str = strtok(string, ",");
            tos_vo = strtol(str, &endptr, base);
            str = strtok(NULL, ",");
            tos_vi = strtol(str, &endptr, base);
            str = strtok(NULL, ",");
            tos_be = strtol(str, &endptr, base);
            str = strtok(NULL, ",");
            tos_bk = strtol(str, &endptr, base);
        }
    }
    DPRINT_INFO(WFA_OUT, "Using TOS: VO=0x%x, VI=0x%x, BE=0x%x, BK=0x%x\n", tos_vo, tos_vi, tos_be, tos_bk);

    traceflag = 1;
    while (1) {
        FD_ZERO(&rfds);
        FD_SET(sd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 300000;
        r = select(sd + 1, &rfds, NULL, NULL, &tv);
        if (r < 0) {
            DPRINT_ERR(WFA_ERR, "select error.\n");
            continue;
        }

        if (nsta) {
            DPRINT_INFO(WFA_OUT, "Waiting in state %d\n", sta->state);
        }
        if ((sta == NULL) || (bKeepFrom == 1))
            r = recvfrom(sd, rmsg, sizeof(rmsg), flags, (struct sockaddr *) &from, &fromlen);
        else
            r = recv(sd, rmsg, sizeof(rmsg), flags);

        resettimer(INTERSTEP_PERIOD * 10);
        if (r < 0) {
            DPRINT_ERR(WFA_ERR, "rcv error:");
            exit(1);
        }
        alarm(0);
        tout = 0;

        /* check some cases  */
        if (traceflag && strcmp(procname, "L.1")) {
            DPRINT_INFO(WFA_OUT, "APTS Received #    length:%d\n", r);
            mpx("APTS RX", rmsg, 64);
        }
        // Do not process unless from remote
        if (from.sin_addr.s_addr == 0 || from.sin_addr.s_addr == local.sin_addr.s_addr) {
            DPRINT_INFO(WFA_OUT, "Received 0 / local\n");
            continue;
        }
        if (from.sin_addr.s_addr == target.sin_addr.s_addr) {
            DPRINT_INFO(WFA_OUT, "Received BROADCAST\n");
            continue;
        }
        if (rmsg[10] == APTS_BCST) {
            DPRINT_INFO(WFA_OUT, "Received BROADCAST, skipping\n");
            continue;
        }
        /* check some cases  */

        DPRINT_INFO(WFA_OUT, "cmd is %d", rmsg[11]);

        if (rmsg[10] == APTS_HELLO) {
            if ((id = get_sta_id(from.sin_addr.s_addr)) >= 0) {
                if (!reset)
                    continue;
                DPRINT_INFO(WFA_OUT, "HELLO after reset");

            } else if ((id = assign_sta_id(from.sin_addr.s_addr)) < 0) {
                DPRINT_INFO(WFA_OUT, "Can not assign id,sta list full");
                continue;
            }

            sta = &stations[id];
            bzero(sta->ipaddress, 20);
            inet_ntop(AF_INET, &(from.sin_addr), IP, INET_ADDRSTRLEN);
            DPRINT_INFO(WFA_OUT, "ip is %s\n", IP);
            strcpy(&(sta->ipaddress[0]), IP);
            sta->cmd = testcase->cmd;
            sta->cookie = 0;
            sta->nsent = 0;
            sta->nerr = 0;
            sta->msgno = 0;
            sta->myid = id;
            sta->alreadyCleared = 0;
            DPRINT_INFO(WFA_OUT, "new_station: size(%d) id=%02d IP address:%s\n", r, id, sta->ipaddress);
            if (reset)
                reset = 0;
            else
                nsta++;
            DPRINT_INFO(WFA_OUT, "New STA = %d\n", nsta);
            sta->state = 0;
            sta->statefunc = consoleProcStatetbl[sta->cmd];
        }/*  if (rmsg[10]==APTS_HELLO)  */
        else {
            if (reset)
                continue;
            if ((id = get_sta_id(from.sin_addr.s_addr)) < 0) {
                inet_ntop(AF_INET, &(from.sin_addr), IP, INET_ADDRSTRLEN);
                DPRINT_INFO(WFA_OUT, "\r\n Unexpected message rcd from ip s_addr=%s sta id=%d", IP, id);
                continue;
            }
            sta = &stations[id];
        }/* else  */

        if (rmsg[10] == APTS_RESET) {
            ap_reset_recd = 1;
            DPRINT_INFO(WFA_OUT, "Rcv RESET from STA, sent RESET back to STA, exit\n");
            WfaConResetAll();
            exit(0);
        }
        if (rmsg[10] == APTS_RESET_STOP) {
            DPRINT_INFO(WFA_OUT, "Recd Kill from Sta\n");
            exit(0);
        }

        func = (sta->statefunc)[sta->state];
        if (!sta->alreadyCleared) {
            func.statefunc(sta, rmsg, r);
        }

    }/* while loop  */
}/*  main */


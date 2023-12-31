
/******************************************************************************
*                         C O M P I L E R   F L A G S
*******************************************************************************
*/

/******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
*******************************************************************************
*/
#include "wmt_ioctl.h"
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
/*#include <syslog.h>*/
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/types.h>

// For directory operation
#include <dirent.h>
#include <sys/uio.h>
#include <linux/serial.h> /* struct serial_struct  */

/******************************************************************************
*                              C O N S T A N T S
*******************************************************************************
*/
/* !defined(ANDROID) */
#ifndef ALOGI
#define ALOGI printf
#endif
#ifndef ALOGE
#define ALOGE printf
#endif
#ifndef ALOGD
#define ALOGD printf
#endif
#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX (128)
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "6620_launcher"

#ifndef N_MTKSTP
#define N_MTKSTP    (15 + 1)  /* MediaTek WCN Serial Transport Protocol */
#endif

#define HCIUARTSETPROTO        _IOW('U', 200, int)

#define CUST_COMBO_WMT_DEV "/dev/stpwmt"
#define CUST_COMBO_STP_DEV "/dev/ttyMT2"  //-- for ALPS
#define CUST_COMBO_PATCH_PATH "/etc/firmware"  //-- for ALPS

#define CUST_BAUDRATE_DFT (115200)

#define CUST_MULTI_PATCH (1)
#define MTK_WCN_ENABLE_COREDUMP_BY_PROPERTY 0

typedef enum {
    STP_MIN = 0x0,
    STP_UART_FULL = 0x1,
    STP_UART_MAND = 0x2,
    STP_BTIF_FULL = 0x3,
    STP_SDIO = 0x4,
    STP_MAX = 0x5,
}STP_MODE;

#define MAX_CMD_LEN (NAME_MAX+1)

typedef enum {
    UART_DISABLE_FC = 0, /*NO flow control*/
    UART_MTK_SW_FC = 1,  /*MTK SW Flow Control, differs from Linux Flow Control*/
    UART_LINUX_FC = 2,   /*Linux SW Flow Control*/
    UART_HW_FC = 3,      /*HW Flow Control*/
} STP_UART_FC;

typedef struct {
    const char *key;
    const char *defValue;
    char value[PROPERTY_VALUE_MAX];
} SYS_PROPERTY;

typedef struct {
    STP_UART_FC fc;
    int parity;
    int stop_bit;
} STP_UART_CONFIG;

typedef struct {
    STP_MODE eStpMode;
    char *pPatchPath;
    char *pPatchName;
    char *gStpDev;
    int iBaudrate;
    STP_UART_CONFIG sUartConfig;
}STP_PARAMS_CONFIG, *P_STP_PARAMS_CONFIG;


#if CUST_MULTI_PATCH
typedef struct {
    int dowloadSeq;
    char addRess[4];
    char patchName[256];
} STP_PATCH_INFO, *P_STP_PATCH_INFO;
#endif

typedef struct {
    const char *pCfgItem;
    char cfgItemValue[NAME_MAX + 1];
} CHIP_ANT_MODE_INFO, *P_CHIP_ANT_MODE_INFO;


typedef struct {
    int chipId;
    STP_MODE stpMode;
    CHIP_ANT_MODE_INFO antMode;
}CHIP_MODE_INFO, *P_CHIP_MODE_INFO;
#ifndef WMT_PLAT_APEX
CHIP_MODE_INFO gChipModeInfo[] = {
    {0x6620, STP_UART_FULL, {"mt6620.defAnt", "mt6620_ant_m3.cfg"}},
    {0x6628, STP_UART_FULL, {"mt6628.defAnt", "mt6628_ant_m1.cfg"}},
    {0x6630, STP_UART_FULL, {"mt6630.defAnt", "mt6630_ant_m1.cfg"}},
};
#else
CHIP_MODE_INFO gChipModeInfo[] = {
    {0x6620, STP_UART_FULL, {"mt6620.defAnt", "WMT.cfg"}},
    {0x6628, STP_UART_FULL, {"mt6628.defAnt", "WMT.cfg"}},
    {0x6630, STP_UART_FULL, {"mt6630.defAnt", "WMT.cfg"}},
};
#endif
/******************************************************************************
*                             D A T A   T Y P E S
*******************************************************************************
*/
struct cmd_hdr{
    char *pCmd;
    int (*hdr_func)(P_STP_PARAMS_CONFIG pStpParamsConfig);
};

struct speed_map {
    unsigned int baud;
    speed_t      speed;
};

/******************************************************************************
*                                 M A C R O S
*******************************************************************************
*/
#define INIT_CMD(c, e, s) {.cmd = c, .cmd_sz = sizeof(c), .evt = e, .evt_sz = sizeof(e), .str = s}

/******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
*******************************************************************************
*/
static int set_speed(int fd, struct termios *ti, int speed);
int setup_uart_param(int hComPort, int iBaudrate, STP_UART_CONFIG *stp_uart);

int cmd_hdr_baud_115k(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_921k(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_2kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_2_5kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_3kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_3_2kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_3_25kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_3_5kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_baud_4kk(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_stp_open(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_stp_close(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_stp_rst(P_STP_PARAMS_CONFIG pStpParamsConfig);
int cmd_hdr_sch_patch(P_STP_PARAMS_CONFIG pStpParamsConfig);
static int check_chip_id(void);
static int setHifInfo(int chipId, char *cfgFilePath);
static int wmt_cfg_item_parser(char *pItem);
static int get_wmt_cfg(int chipId);
static speed_t get_speed(int baudrate);


/******************************************************************************
*                            P U B L I C   D A T A
*******************************************************************************
*/

/******************************************************************************
*                           P R I V A T E   D A T A
*******************************************************************************
*/
static struct speed_map speeds[] = {
    {115200,    B115200},
    {921600,    B921600},
    {1000000,    B1000000},
    {1152000,    B1152000},
    {2000000,    B2000000},
    {2500000,    B2500000},
    {3000000,    B3000000},
    {3500000,    B3500000},
    {4000000,    B4000000},
};

static STP_UART_CONFIG g_stp_uart_config;

struct cmd_hdr cmd_hdr_table[] = {
    { "baud_115200_0", cmd_hdr_baud_115k},
    { "baud_921600_0", cmd_hdr_baud_921k},
    { "baud_2000000_0", cmd_hdr_baud_2kk},
    { "baud_2500000_0", cmd_hdr_baud_2_5kk},
    { "baud_3000000_0", cmd_hdr_baud_3kk},
    //{ "baud_3200000_0", cmd_hdr_baud_3_2kk},
    //{ "baud_3250000_0", cmd_hdr_baud_3_25kk},
    { "baud_3500000_0", cmd_hdr_baud_3_5kk},
    { "baud_4000000_0", cmd_hdr_baud_4kk},
    { "open_stp", cmd_hdr_stp_open},
    { "close_stp", cmd_hdr_stp_close},
    { "rst_stp", cmd_hdr_stp_rst},
    { "srh_patch", cmd_hdr_sch_patch},
};

static volatile sig_atomic_t __io_canceled = 0;
static char gPatchName[NAME_MAX+1]= {0};
static char gPatchFolder[NAME_MAX+1]= {0};
static char gStpDev[NAME_MAX+1]= {0};
static int gStpMode = -1;
static char gWmtCfgName[NAME_MAX+1] = {0};
static int gWmtFd = -1;
static int gTtyFd = -1;
static char gCmdStr[MAX_CMD_LEN]= {0};
static char gRespStr[MAX_CMD_LEN]= {0};
static int gFmMode = 2; /* 1: i2c, 2: comm I/F */
static const char *gUartName = NULL;

#if CUST_MULTI_PATCH
static unsigned int gPatchNum = 0;
static unsigned int gDwonSeq = 0;
static P_STP_PATCH_INFO pStpPatchInfo = NULL;
static STP_PATCH_INFO gStpPatchInfo;
#endif

pthread_t thread_handle = -1;
/******************************************************************************
*                              F U N C T I O N S
*******************************************************************************
*/

/* Used as host uart param setup callback */
int setup_uart_param(
    int hComPort,
    int iBaudrate,
    STP_UART_CONFIG *stpUartConfig) {
    struct termios ti;
    int  fd;

    if (!stpUartConfig) {
        ALOGE("Invalid stpUartConfig");
        return -2;
    }

    ALOGI("setup_uart_param %d %d\n", iBaudrate, stpUartConfig->fc);

    fd = hComPort;
    if (fd < 0) {
        ALOGE("Invalid serial port");
        return -2;
    }

    tcflush(fd, TCIOFLUSH);

    if (tcgetattr(fd, &ti) < 0) {
        ALOGE("Can't get port settings");
        return -3;
    }

    cfmakeraw(&ti);

    ALOGI("ti.c_cflag = 0x%08x\n", ti.c_cflag);
    ti.c_cflag |= CLOCAL;
    ALOGI("CLOCAL = 0x%x\n", CLOCAL);
    ALOGI("(ori)ti.c_iflag = 0x%08x\n", ti.c_iflag);
    ALOGI("(ori)ti.c_cflag = 0x%08x\n", ti.c_cflag);
    ALOGI("stpUartConfig->fc= %d (0:none,sw,hw,linux)\n", stpUartConfig->fc);

    if (stpUartConfig->fc == UART_DISABLE_FC) {
        ti.c_cflag &= ~CRTSCTS;
        ti.c_iflag &= ~(0x80000000);
    } else if (stpUartConfig->fc == UART_MTK_SW_FC) {
        ti.c_cflag &= ~CRTSCTS;
        ti.c_iflag |= 0x80000000; /*MTK Software FC*/
    } else if (stpUartConfig->fc == UART_HW_FC) {
        ti.c_cflag |= CRTSCTS;      /*RTS, CTS Enable*/
        ti.c_iflag &= ~(0x80000000);
    } else if (stpUartConfig->fc == UART_LINUX_FC) {
        ti.c_iflag |= (IXON | IXOFF | IXANY); /*Linux Software FC*/
        ti.c_cflag &= ~CRTSCTS;
        ti.c_iflag &= ~(0x80000000);
    } else {
        ti.c_cflag &= ~CRTSCTS;
        ti.c_iflag &= ~(0x80000000);
    }

    ALOGI("c_c CRTSCTS = 0x%16x\n", CRTSCTS);
    ALOGI("c_i IXON = 0x%08x\n", IXON);
    ALOGI("c_i IXOFF = 0x%08x\n", IXOFF);
    ALOGI("c_i IXANY = 0x%08x\n", IXANY);
    ALOGI("(aft)ti.c_iflag = 0x%08x\n", ti.c_iflag);
    ALOGI("(aft)ti.c_cflag = 0x%08x\n\n", ti.c_cflag);

    if (tcsetattr(fd, TCSANOW, &ti) < 0) {
        ALOGE("Can't set port settings");
        return -4;
    }

    /* Set baudrate */
    if (set_speed(fd, &ti, iBaudrate) < 0) {
        ALOGE("Can't set initial baud rate");
        return -5;
    }

    tcflush(fd, TCIOFLUSH);

    return 0;
}

static void sig_hup(int sig) {
    fprintf(stderr, "sig_hup...\n");
}

static void sig_term(int sig) {
    fprintf(stderr, "sig_term...\n");
    __io_canceled = 1;
    ioctl(gWmtFd, WMT_IOCTL_SET_LAUNCHER_KILL, 1);
}


static speed_t get_speed(int baudrate) {
    unsigned int idx;
    for (idx = 0; idx < sizeof(speeds)/sizeof(speeds[0]); idx++) {
        if (baudrate == (int)speeds[idx].baud) {
            return speeds[idx].speed;
        }
    }
    return CBAUDEX;
}

int set_speed(int fd, struct termios *ti, int speed) {
    struct serial_struct ss;
    int baudenum = get_speed(speed);

    if (speed != CBAUDEX) {
        // printf("%s: standard baudrate: %d -> 0x%08x\n", __FUNCTION__, speed, baudenum);
        if ((ioctl(fd, TIOCGSERIAL, &ss)) < 0) {
            ALOGI("%s: BAUD: error to get the serial_struct info:%s\n", __FUNCTION__, strerror(errno));
            return -1;
        }
        ss.flags &= ~ASYNC_SPD_CUST;
#if defined(SERIAL_STRUCT_EXT) /*modified in serial_struct.h*/
        memset(ss.reserved, 0x00, sizeof(ss.reserved));
#endif
        ss.flags |= (1 << 13);    /*set UPFLOWLATENCY flat to tty, or serial_core will reset tty->low_latency to 0*/
        /*set standard buadrate setting*/
        if ((ioctl(fd, TIOCSSERIAL, &ss)) < 0) {
            ALOGI("%s: BAUD: error to set serial_struct:%s\n", __FUNCTION__, strerror(errno));
            return -2;
        }
        cfsetospeed(ti, baudenum);
        cfsetispeed(ti, baudenum);
        return tcsetattr(fd, TCSANOW, ti);
    }
    else {
        ALOGI("%s: unsupported non-standard baudrate: %d -> 0x%08x\n", __FUNCTION__, speed, baudenum);
        return -3;
    }
}

int cmd_hdr_baud_115k(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 115200, gStpUartConfig) : -1;
}

int cmd_hdr_baud_921k(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 921600, gStpUartConfig) : -1;
}

int cmd_hdr_baud_2kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 2000000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_2_5kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 2500000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_3kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 3000000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_3_2kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 3200000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_3_25kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 3250000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_3_5kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 3500000, gStpUartConfig) : -1;
}

int cmd_hdr_baud_4kk(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    STP_UART_CONFIG *gStpUartConfig = &pStpParamsConfig->sUartConfig;
    return (gTtyFd != -1) ? setup_uart_param(gTtyFd, 4000000, gStpUartConfig) : -1;
}

int cmd_hdr_stp_open(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    int ld;
    if ((STP_UART_FULL == gStpMode) && (-1 == gTtyFd)) {
        gTtyFd = open(gStpDev, O_RDWR | O_NOCTTY);
        if (gTtyFd < 0) {
            fprintf(stderr, "Can't open serial port %s\n", gStpDev);
            return -2;
        }
        ALOGI("real_tty(%s) opened(%d)\n", gStpDev, gTtyFd);

        /* Set TTY to N_MTKSTP line discipline */
        ld = N_MTKSTP;
        if (ioctl(gTtyFd, TIOCSETD, &ld) < 0) {
            fprintf(stderr, "Can't set ldisc to N_MTKSTP\n");
            return -3;
        }

         // printf("Set tty->low_latency\n");
         if (ioctl(gTtyFd, HCIUARTSETPROTO, 0) < 0) {
                ALOGE("Can't set HCIUARTSETPROTO\n");
                return -4;
        }
        return 0;
    }
    else {
        fprintf(stderr, "stp_open fail: stp_mode(%d) real_tty_fd(%d) \n", gStpMode, gTtyFd);
        return -1;
    }
}

int cmd_hdr_stp_close(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    int ld;

    if ((STP_UART_FULL == gStpMode) && (0 <= gTtyFd)) {
        /* Restore TTY line discipline */
        ld = N_TTY;
        if (ioctl(gTtyFd, TIOCSETD, &ld) < 0) {
            ALOGE("Can't restore line discipline");
            return -2;
        }

        close(gTtyFd);
        gTtyFd = -1;
        return 0;
    } else if (gTtyFd == -1) {
        return 0;
    } else {
        fprintf(stderr, "stp_close fail: stp_mode(%d) real_tty_fd(%d) \n", gStpMode, gTtyFd);
        return -1;
    }
}

int cmd_hdr_stp_rst(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    int ret = 0;
    /*this step fail?*/
    ret = cmd_hdr_stp_close(pStpParamsConfig);
    /*here, launcher is close state*/
    ret = cmd_hdr_stp_open(pStpParamsConfig);
    return ret;
}

#if CUST_MULTI_PATCH
int cmd_hdr_sch_patch(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    // #define PATCH_PATH "/system/etc/firmware"
    int chipId = 0;
    int hwVersion = 0;
    int fwVersion = 0;
    char chipName[16] = {0};
    char patchFullName[256] = {0};
#if (MTK_COMBO_USING_PATCH_NAME==1)
    char patchName[128] = {0};
#endif /* MTK_COMBO_USING_PATCH_NAME */
    unsigned int patchVer = 0;
    DIR *pDir = NULL;
    int patchFd = -1;
    int iRet = 0;
    int bytes;
    unsigned int patchNum = 0;
    char patchInfo[8] = {0};
    unsigned int isFirst = 1;
    P_STP_PATCH_INFO pstPaInfo = NULL;
    struct dirent* pDirent = NULL;

    if (gWmtFd > 0) {
        /*1. ioctl to get CHIP ID*/
        chipId = ioctl(gWmtFd, WMT_IOCTL_GET_CHIP_INFO, 0);
        if ((0x0321 == chipId) || (0x0335 == chipId) || (0x0337 == chipId)) {
                chipId = 0x6735;
                ALOGI("for denali chipid convert\n");
        }
        if (0x0326 == chipId) {
          chipId = 0x6755;
          ALOGI("for jade chipid convert\n");
        }
        if (0x0279 == chipId) {
          chipId = 0x6797;
          ALOGI("for everest chipid convert\n");
        }
        strncpy(chipName, "mt", strlen("mt"));
        sprintf(chipName + strlen("mt"), "%04x", chipId);

        if ((!strcmp(chipName, "mt6620")) || (!strcmp(chipName, "mt6628")) || (!strcmp(chipName, "mt6630")) ||
            (!strcmp(chipName, "mt6572")) || (!strcmp(chipName, "mt6582")) || (!strcmp(chipName, "mt6592")) ||
            (!strcmp(chipName, "mt8127"))|| (!strcmp(chipName, "mt7623"))  || (!strcmp(chipName, "mt6571")) ||
            (!strcmp(chipName, "mt6752")) ||
            (!strcmp(chipName, "mt8163")) || (!strcmp(chipName, "mt6580")) ||
            (!strcmp(chipName, "mt6735")) || (!strcmp(chipName, "mt6755")) || (!strcmp(chipName, "mt6797")) ||
			(!strcmp(chipName, "mt8167"))) {
            if ((!strcmp(chipName, "mt6572")) || (!strcmp(chipName, "mt6582")) || (!strcmp(chipName, "mt6592"))) {
                strncpy(chipName, "ROMv1", strlen("ROMv1"));
                chipName[5] = '\0';
            } else if (!strcmp(chipName, "mt8127") || !strcmp(chipName, "mt6571")) {
                                strncpy(chipName, "ROMv2", strlen("ROMv2"));
                                chipName[5] = '\0';
            } else if (!strcmp(chipName, "mt6755") ||!strcmp(chipName, "mt6752") || !strcmp(chipName, "mt6735")
                        || !strcmp(chipName, "mt8163") || !strcmp(chipName, "mt6580") || !strcmp(chipName, "mt7623")
                        || !strcmp(chipName, "mt8167")) {
                        strncpy(chipName, "ROMv2_lm", strlen("ROMv2_lm"));
            } else if (!strcmp(chipName, "mt6797")) {
                strncpy(chipName, "ROMv3", strlen("ROMv3"));
                chipName[5] = '\0';
            }
            strcat(chipName, "_patch");
            ALOGI("patch name pre-fix:%s\n", chipName);
            /*2. ioctl to get FIRMWARE VERSION*/
            fwVersion = ioctl(gWmtFd, WMT_IOCTL_GET_CHIP_INFO, 2);
            ALOGI("fwVersion:0x%04x\n", fwVersion);
            /*3. open directory patch located*/
            if (NULL == pStpParamsConfig->pPatchPath) {
                pStpParamsConfig->pPatchPath = CUST_COMBO_PATCH_PATH;
            }

            {
                pDir = opendir(pStpParamsConfig->pPatchPath);
                if (NULL == pDir) {
                    ALOGE("patch path cannot be opened");
                    iRet = -1;
                    return iRet;
                }
                while (NULL != (pDirent = readdir(pDir))) {
                    patchVer = 0;

                    if (0 == (strncmp(pDirent->d_name, chipName, strlen(chipName)))) {
                        /*4.1. search patch name begined with chipName*/
                        strcpy(patchFullName, pStpParamsConfig->pPatchPath);
                        strcat(patchFullName, "/");
                        // robust, if input patch is /etc/firmwre/ no issue should be happened.
                        strcat(patchFullName, pDirent->d_name);

                        ALOGI("%s\n", patchFullName);
#if (MTK_COMBO_USING_PATCH_NAME==1)
                        strcpy (patchName, pDirent->d_name);
#endif /* MTK_COMBO_USING_PATCH_NAME */
                        /*4.1. search patch name mt[CHIP ID]xxx.bin*/
                        if (0 <= (patchFd = (open(patchFullName, O_RDONLY)))) {
                           /*4.2. read patch header and check if metch with 
                             MAJOR+MINOR number in fw version */
                            if (-1 != lseek(patchFd, 22, SEEK_SET)) {
                                memset(&gStpPatchInfo, 0, sizeof(gStpPatchInfo));
                                memset(patchInfo, 0, sizeof(patchInfo));

                                bytes = read(patchFd, ((char *)&patchVer) + 1, 1);
                                if (-1 == bytes) {
                                    ALOGI("read patchVer1 failed!\n");
                                    goto readfailed;
                                }
                                bytes = read(patchFd, ((char *)&patchVer), 1);
                                if (-1 == bytes) {
                                    ALOGI("read patchVer failed!\n");
                                    goto readfailed;
                                }
                                  /*print hardware version information in patch*/
                                ALOGI("fw Ver in patch: 0x%04x\n", patchVer);
                                if (0 == ((patchVer ^ fwVersion) & 0x00ff)) {
                                    bytes = read(patchFd, patchInfo, 4);
                                    if (-1 == bytes) {
                                        ALOGI("read patchInfo failed!\n");
                                        goto readfailed;
                                     }
                                    patchInfo[4] = '\0';
                                    ALOGI("read patch info:0x%02x,0x%02x,0x%02x,0x%02x\n",
                                    patchInfo[0], patchInfo[1], patchInfo[2], patchInfo[3]);
                                    if (1 == isFirst) {
                                        gPatchNum = (patchInfo[0] & 0xF0) >> 4;
                                        ALOGI("gpatchnum = [%d]\n", gPatchNum);
                                        ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_NUM, gPatchNum);

                                        gDwonSeq = (patchInfo[0] & 0x0F);
                                        ALOGI("gdwonseq = [%d]\n", gDwonSeq);
                                        gStpPatchInfo.dowloadSeq = gDwonSeq;
                                        memcpy(gStpPatchInfo.addRess, patchInfo, sizeof(gStpPatchInfo.addRess));
                                        gStpPatchInfo.addRess[0] = 0x00;
#if (MTK_COMBO_USING_PATCH_NAME==1)
                                        strncpy(gStpPatchInfo.patchName, patchName,
                                            sizeof(gStpPatchInfo.patchName) - 1);
#else
                                        strncpy(gStpPatchInfo.patchName, patchFullName,
                                            sizeof(gStpPatchInfo.patchName) - 1);
#endif /* MTK_COMBO_USING_PATCH_NAME */
                                        gStpPatchInfo.patchName[sizeof(gStpPatchInfo.patchName) - 1] = '\0';
                                        ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_INFO, &gStpPatchInfo);
                                        isFirst++;
                                     } else {
                                        gDwonSeq = (patchInfo[0] & 0x0F);
                                        ALOGI("gdwonseq = [%d]\n", gDwonSeq);
                                        gStpPatchInfo.dowloadSeq = gDwonSeq;
                                        memcpy(gStpPatchInfo.addRess, patchInfo, sizeof(gStpPatchInfo.addRess));
                                        gStpPatchInfo.addRess[0] = 0x00;
#if (MTK_COMBO_USING_PATCH_NAME==1)
                                        strncpy(gStpPatchInfo.patchName, patchName,
                                            sizeof(gStpPatchInfo.patchName) - 1);
#else
                                        strncpy(gStpPatchInfo.patchName, patchFullName,
                                            sizeof(gStpPatchInfo.patchName) - 1);
#endif /* MTK_COMBO_USING_PATCH_NAME */
                                        gStpPatchInfo.patchName[sizeof(gStpPatchInfo.patchName) - 1] = '\0';
                                        ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_INFO, &gStpPatchInfo);
                                    }
                                }
                            }
                            else {
                                ALOGE("seek failed\n");
                            }
readfailed:
                            close(patchFd);
                            patchFd = -1;
                        }
                        else {
                            ALOGI("open patch file(%s) failed\n", patchFullName);
                            // ALOGE(patchFullName);
                        }
                    }
                }
                /*5. return value*/
                closedir(pDir);
                pDir = NULL;
            }
        }
    }
    else {
        ALOGE("file descriptor is not valid\n");
        iRet = -2;
    }
    return iRet;
}
#else
int cmd_hdr_sch_patch(P_STP_PARAMS_CONFIG pStpParamsConfig) {
    // #define PATCH_PATH "/system/etc/firmware"
    int chipId = 0;
    int hwVersion = 0;
    int fwVersion = 0;
    char chipName[16] = {0};
    char patchFullName[256] = {0};
    unsigned int patchVer = 0;
    DIR *pDir = NULL;
    int patchFd = -1;
    struct dirent* pDirent = NULL;
    int iRet = -1;

    if (gWmtFd > 0) {
    /*1. ioctl to get CHIP ID*/
        chipId = ioctl(gWmtFd, WMT_IOCTL_GET_CHIP_INFO, 0);
        strcpy(chipName, "mt");
        sprintf(chipName + strlen(chipName), "%04x", chipId);
        strcat(chipName, "_patch");
        ALOGI("patch name pre-fix:%s\n", chipName);
        fwVersion = ioctl(gWmtFd, WMT_IOCTL_GET_CHIP_INFO, 2);
        ALOGI("fwVersion:0x%04x\n", fwVersion);
    /*3. open directory patch located*/
        if (NULL == pStpParamsConfig->pPatchPath) {
            pStpParamsConfig->pPatchPath = CUST_COMBO_PATCH_PATH;
        }
        pDir = opendir(pStpParamsConfig->pPatchPath);
        if (NULL == pDir) {
            ALOGE("patch path cannot be opened");
            iRet = -2;
            return iRet;
        }
        while (NULL != (pDirent = readdir(pDir))) {
            patchVer = 0;

            if (0 == (strncmp(pDirent->d_name, chipName, strlen(chipName)))) {
                /*4.1. search patch name begined with chipName*/
                strcpy(patchFullName, pStpParamsConfig->pPatchPath);
                strcat(patchFullName, "/");  // robust, if input patch is /etc/firmwre/ no issue should be happened.
                strcat(patchFullName, pDirent->d_name);

                ALOGI("%s\n", patchFullName);
                /*4.1. search patch name mt[CHIP ID]xxx.bin*/
                if (0 < (patchFd = (open(patchFullName, O_RDONLY)))) {
                  /*4.2. read patch header and check if metch with MAJOR+MINOR number in fw version */
                    if (-1 != lseek(patchFd, 22, SEEK_SET)) {
                        read(patchFd, ((char *)&patchVer) + 1, 1);
                        read(patchFd, ((char *)&patchVer), 1);
                        /*print firmware version information in patch*/
                          ALOGI("fw Ver in patch: 0x%04x\n", patchVer);
                          if (0 == ((patchVer ^ fwVersion) & 0x00ff)) {
                                ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_NAME, patchFullName);
                                ALOGI("fw Ver in patch matches with firmware version\n");
                                iRet = 0;
                                close(patchFd);
                                break;
                            }
                    }
                    else {
                        ALOGE("seek failed\n");
                    }
                    close(patchFd);
                    patchFd = -1;
                }
                else {
                    // printf("open patch file(%s) failed\n", patchFullName);
                    ALOGE(patchFullName);
                }
          }
        }

        /*5. return value*/
        closedir(pDir);
        pDir = NULL;
    }
    else {
        ALOGE("file descriptor is not valid\n");
        iRet = -1;
    }
    return iRet;
}
#endif
/*
ret 0: success
ret 1: cmd not found
ret -x: handler return value
*/
int handle_cmd(P_STP_PARAMS_CONFIG pStpParamsConfig, char *cmd, int len) {
    int ret = 1;
    int i;
    int cmd_len;

    for (i = 0; i < (int)(sizeof(cmd_hdr_table)/sizeof(cmd_hdr_table[0])); ++i) {
        cmd_len = (int)strlen(cmd_hdr_table[i].pCmd);
        if (!strncmp(cmd_hdr_table[i].pCmd, cmd, (len < cmd_len) ? len : cmd_len)) {
            ret = (*cmd_hdr_table[i].hdr_func)(pStpParamsConfig);
        }
    }

    return ret;
}

void display_usage(int chipid) {
    unsigned int index = 0;
    char * usage1[] = {
        "MTK WCN combo tool set, version 1.0-release",
        "Usage: consys_launcher -m mode -p patchfolderpath",
        "    -m (BT/GPS/FM common interface mode selection)",
        "        -1: UART mode (common interface: UART)",
        "        -3: BTIF mode (common interface: BTIF)",
        "        -4: SDIO mode (common interface: SDIO)",
        "    -p (MTK WCN soc conssy chip firmware patch location)",
        "        -e.g. /etc/firmware",
        "e.g. consys_launcher -m 3 -p /etc/firmware/",
    };
        char * usage2[] = {
        "MTK WCN combo tool set, version 1.0-release",
        "Usage: 6620_launcher -m mode -p patchfolderpath [-d uartdevicenode] [-b baudrate] [-c uartflowcontrol]",
//        "    -m (BT/GPS/FM common interface mode selection)",
//        "        -1: UART mode (common interface: UART)",
//        "        -1: UART full mode (common interface UART)",
//        "        -2: UART mandetary mode (common interface UART)",
        "        -4: UART mode (common interface SDIO)",
        "    -p (MTK WCN Combo chip firmware patch location)",
        "        -e.g. /etc/firmware",
        "    -b (Baudrate set when BT/GPS/FM runs under UART mode, no needed under SDIO mode)",
        "        -115200/921600/2000000/2500000/3000000/3500000/4000000",
        "    -d (UART device node, when under UART mode, no needed under SDIO mode)",
        "        -e.g. /dev/ttyMT1, /dev/ttyMT2, /dev/ttyHS2, etc.",
        "    -c (UART flowcontrol set)",
        "        -0, no flowcontrol default value, please donot modify this parameter",
        "e.g. 6620_launcher 4 /etc/firmware/mt6628_patch_hdr.bin",
        "e.g. 6620_launcher -m 1 -p /etc/firmware/",
        "e.g. 6620_launcher -m 1 -n /etc/firmware/mt6628_patch_hdr.bin",
        "e.g. 6620_launcher -m 4 -d /dev/ttyMT2 -b 4000000 -n /etc/firmware/mt6628_patch_hdr.bin",
    };
    if (0x6582 == chipid || 0x8127 == chipid) {
           for (index = 0; index < sizeof(usage1)/sizeof(usage1[0]); index++) {
               ALOGI("%s\n", usage1[index]);
           }
    } else {
            for (index = 0; index < sizeof(usage2)/sizeof(usage2[0]); index++) {
                ALOGI("%s\n", usage2[index]);
           }
    }
    exit(EXIT_FAILURE);
}

int para_valid_check(P_STP_PARAMS_CONFIG pStpParamConfig) {
    if ((NULL != pStpParamConfig->pPatchPath) || (NULL != pStpParamConfig->pPatchName)) {
        if (NULL != pStpParamConfig->pPatchPath) {
            ALOGI("MCU patch folder path: %s\n", pStpParamConfig->pPatchPath);
        }
        if (NULL != pStpParamConfig->pPatchName) {
            ALOGI("MCU patch full path: %s\n", pStpParamConfig->pPatchName);
        }
    }
    else {
        puts("MCU patch name or patch not found, exit.");
        return -1;
    }
    if (pStpParamConfig->eStpMode != STP_SDIO && pStpParamConfig->eStpMode != STP_UART_MAND &&
        pStpParamConfig->eStpMode != STP_UART_FULL) {
        puts("Stp Mode is not set, common interface use default: SDIO Mode");
        pStpParamConfig->eStpMode = STP_SDIO;
        return 0;
    }
    // SDIO mode: eStpMode = STP_SDIO && (pPachName != NULL || pPatchPath != NULL)
    if (pStpParamConfig->eStpMode == STP_SDIO) {
        ALOGI("Common Interface: SDIO mode\n");
    }
    else if (pStpParamConfig->eStpMode == STP_UART_MAND || pStpParamConfig->eStpMode == STP_UART_FULL) {
        ALOGI("Common Interface: UART mode\n");
        if (NULL == pStpParamConfig->gStpDev) {
            pStpParamConfig->gStpDev = CUST_COMBO_STP_DEV;
            ALOGI("no uart device input, use default: %s\n", pStpParamConfig->gStpDev);
        }
        if (pStpParamConfig->iBaudrate < 0) {
            // FixMe:Chaozhong, add baudrate validation check
            pStpParamConfig->iBaudrate = 4000000;
            ALOGI("no baudrate input, use default: %d\n", pStpParamConfig->iBaudrate);
        }
    }
    return 0;
}

static int wmt_cfg_item_parser(char *pItem) {
    int maxIndex  = sizeof (gChipModeInfo) / sizeof (gChipModeInfo[0]);
    int index = 0;
    int length = 0;
    char *str = NULL;
    char *keyStr = NULL;
    char *valueStr = NULL;
    if (NULL == pItem) {
        ALOGI("Warning:pItem is NULL\n");
        return -1;
    }
    /*all item must be start with mt66xx*/
    str = strstr(pItem, "m");
    if (NULL == str) {
        ALOGI("Warning:no string start with 'm' found in %s\n", pItem);
        return -2;
    }

    for (index = 0; index < maxIndex; index++) {
        keyStr = (char*)gChipModeInfo[index].antMode.pCfgItem;
        if (0 == strncasecmp(str, keyStr, strlen(keyStr))) {
            str = strstr(str, "=");
            if (NULL == str) {
                ALOGI("Warning:no '=' found in %s\n", str);
                return -3;
            }
            str = strstr(str, "m");
            if (NULL == str) {
                ALOGI("Warning:no 'm' found in %s\n", str);
                return -4;
            }
            while (((*str) == ' ') || ((*str) == '\t') || ((*str) == '\n')) {
                if (str >= pItem + strlen(pItem)) {
                    break;
                }
                str++;
            }
            valueStr = str;
            while (((*str) != ' ') && ((*str) != '\t') && ((*str) != '\0') &&
                ((*str) != '\n') && ((*str) != '\r')) {
                if (str >= pItem + strlen(pItem)) {
                    ALOGI("break\n");
                    break;
                }
                str++;
            }
            *str = '\0';
            length = sizeof(gChipModeInfo[index].antMode.cfgItemValue);
            strncpy(gChipModeInfo[index].antMode.cfgItemValue, valueStr, length - 1);
            gChipModeInfo[index].antMode.cfgItemValue[length - 1] = '\0';
            ALOGI("Info:key:%s value:%s\n", keyStr, gChipModeInfo[index].antMode.cfgItemValue);
            break;
        }
    }
    return 0;
}

static int get_coredump_flag(void) {
    return 2; /* default disable */
}

static void set_coredump_flag(void) {
#define COREDUMP_CTRL_FILE "/data/coredump"
    int coredumpEnableFlag = 0;
    int iRet = -1;
    int coredumpFd = -1;
    if (gWmtFd < 0) {
        ALOGI("%s:invalid wmt fd\n", __func__);
        return;
    }
    /* simplify flag in linux and android property & buildtype cases */
    coredumpEnableFlag = get_coredump_flag();
    ioctl(gWmtFd, WMT_IOCTL_WMT_COREDUMP_CTRL, coredumpEnableFlag);
    return;
}

static int get_wmt_cfg(int chipId) {
#define WMTCFGFILEPATH "/system/etc/firmware/WMT.cfg"
#define OPENMODE "r"
#define MAXLINELEN 512
    FILE * file = NULL;
    int iRet = -1;
    char *pStr = NULL;
    char line[MAXLINELEN];

    file = fopen(WMTCFGFILEPATH, OPENMODE);
    if (NULL == file) {
        ALOGI("%s cannot be opened, errno:%d\n", WMTCFGFILEPATH, errno);
        return -2;
    }
    iRet = 0;
    do {
        pStr = fgets(line, MAXLINELEN, file);
        if (NULL == pStr) {
            ALOGI("NULL is returned, eighter EOF or error maybe found\n");
            break;
        }

        wmt_cfg_item_parser(line);
        memset(line, 0, MAXLINELEN);
    } while (pStr != NULL);

    if (NULL != file) {
        if (0 == fclose(file)) {
            ALOGI("close %s succeed\n", WMTCFGFILEPATH);
        }
        else {
            ALOGI("close %s failed, errno:%d\n", WMTCFGFILEPATH, errno);
        }
    }
    return iRet;
}


static int get_chip_info_index(int chipId) {
    int i = 0;
    int index = -1;
    int left = 0;
    int middle = 0;
    int right = sizeof(gChipModeInfo) / sizeof(gChipModeInfo[0]) - 1;
    if ((chipId < gChipModeInfo[left].chipId) || (chipId > gChipModeInfo[right].chipId))
        return index;

    middle = (left + right) / 2;
    while (left <= right) {
        if (chipId > gChipModeInfo[middle].chipId) {
            left = middle + 1;
        } else if (chipId < gChipModeInfo[middle].chipId) {
            right = middle - 1;
        } else {
            index = middle;
            break;
        }
        middle = (left + right) / 2;
    }

    if (0 > index)
        ALOGI("no supported chipid found\n");
    else
        ALOGI("index:%d, chipId:0x%x\n", index, gChipModeInfo[index].chipId);

    return index;
}

/* TBD in platform-specific way */
static int get_persist_chip_id(void) { return -1; }
static int set_persist_chip_id(const int id) { return 0; }

/* return 0 for not ready, otherwise ready */
static int get_drv_rdy(void) {
    /* GeK: no property to communicate with wmt_loader, and we need another way
     * to check driver ready. Maybe query ioctl(COMBO_IOCTL_DO_MODULE_INIT) to
     * WCN_COMBO_LOADER_DEV is a possible way for both android and non-android.
     */
    ALOGI("WARNING!!! Need a correct way to check driver ready!!!");
    return 1;
}

static int query_chip_id(void) {
    int chipId;
    int chk_id;
    int iRet = -1;

    chipId = get_persist_chip_id();

    if (get_chip_info_index(chipId) >= 0)
        return chipId;

    // no persistent information found
    // get chip id
    chipId = ioctl(gWmtFd, WMT_IOCTL_WMT_QUERY_CHIPID, NULL);
    if (chipId <= 0)
        return -1;

    ALOGI("chip id is 0x%x\n", chipId);
    iRet = set_persist_chip_id(chipId);

    if (iRet) {
        ALOGI("set persistent fail:%d\n", iRet);
        return -1;
    }
    return chipId;
}

static int check_chip_id(void) {
#define COMBO_IOC_MAGIC        'h'
#define COMBO_IOCTL_GET_CHIP_ID  _IOR(COMBO_IOC_MAGIC, 0, int)
#define COMBO_IOCTL_SET_CHIP_ID  _IOW(COMBO_IOC_MAGIC, 1, int)
    int chipId = -1;
    int fdHifsdio = -1;

    // read from system property
    chipId = get_persist_chip_id();
    // open HIF-SDIO
    fdHifsdio = open("/dev/hifsdiod", O_RDWR | O_NOCTTY);
    if (fdHifsdio < 0) {
        ALOGI("open hifsdiod fail\n");
        return -1;
    }

    // read from config file
    if (0 > get_chip_info_index(chipId)) {
        int ret;
        // no wcn.combo.chipid property information found
        // get chip id
        chipId = ioctl(fdHifsdio, COMBO_IOCTL_GET_CHIP_ID, NULL);
        ALOGI("chip id is 0x%x\n", chipId);
        // assume we get 0x6628 here
        ALOGI("chiId:0x%x, setting to persistent\n", chipId);
        ret = set_persist_chip_id(chipId);
        if (ret)
            chipId = -1;
    } else {
        ioctl(fdHifsdio, COMBO_IOCTL_SET_CHIP_ID, chipId);
        ALOGI("set chipId(0x%x) to HIF-SDIO module\n", chipId);
    }
    // close HIF-SDIO
    close(fdHifsdio);
    fdHifsdio = -1;
    return chipId;
}

static int setHifInfo(int chipId, char *cfgFilePath) {
    int index = -1;
    index = get_chip_info_index(chipId);
    if ((gStpMode <= STP_MIN) || (STP_SDIO < gStpMode)) {
        ALOGI("STP Mode is not set, fetching default mode...\n");
        if (0 <= index) {
            gStpMode = gChipModeInfo[index].stpMode;
        } else {
            // gStpMode = STP_UART_FULL;
            gStpMode = -1;
        }
    }

    if ((0 <= index) && (NULL != cfgFilePath)) {
#if (MTK_COMBO_USING_PATCH_NAME==1)
        memset(gWmtCfgName, 0, sizeof(gWmtCfgName));
        strncpy(gWmtCfgName, gChipModeInfo[index].antMode.cfgItemValue, sizeof(gWmtCfgName) - 1);
        gWmtCfgName[strlen(gChipModeInfo[index].antMode.cfgItemValue)] = '\0';
#else
        memset(gWmtCfgName, 0, sizeof(gWmtCfgName));
        strncpy (gWmtCfgName, cfgFilePath, sizeof(gWmtCfgName) - 1);
        gWmtCfgName[sizeof(gWmtCfgName) - 1] = '\0';
        strcat(gWmtCfgName, "/");
        strcat(gWmtCfgName, gChipModeInfo[index].antMode.cfgItemValue);
        gWmtCfgName[strlen(cfgFilePath) + strlen("/") + strlen(gChipModeInfo[index].antMode.cfgItemValue)] = '\0';
#endif /* MTK_COMBO_USING_PATCH_NAME */
    } else {
        memset(gWmtCfgName, 0, sizeof(gWmtCfgName));
    }
    ALOGI("chipId(0x%04x), default Mode(%d), strlen(gWmtCfgName)(%u), wmtCfgFile(%s)\n",
        chipId, gStpMode, strlen(gWmtCfgName), gWmtCfgName);
    return gStpMode;
}

#if (MTK_COMBO_ENABLE_WIFI_IN_LAUNCHER==1)
#define CUST_COMBO_WIFI_DEV "/dev/wmtWifi"
#define POWER_ON_WIFI "1"
#define POWER_ON_WIFI_LEN 1
static int gWifiFd = -1;

static int enable_wifi(void) {
    int retryCounter = 0;
    int i_ret = -1;

    gWifiFd = open(CUST_COMBO_WIFI_DEV, O_RDWR | O_NOCTTY);
    do {
        if (gWifiFd < 0) {
            ALOGI("Can't open device node(%s) error:%d \n", CUST_COMBO_WIFI_DEV, gWifiFd);
            usleep(300000);
            gWifiFd = open(CUST_COMBO_WIFI_DEV, O_RDWR | O_NOCTTY);
        } else {
           break;
        }
        retryCounter++;
    } while (retryCounter < 20);

    if (gWifiFd > 0) {
        do {
            i_ret = write(gWifiFd, POWER_ON_WIFI, POWER_ON_WIFI_LEN);
            if (i_ret == 1) {
                    ALOGI("Power on device node(%s) gWifiFd:%d succeed !!\n", CUST_COMBO_WIFI_DEV, gWifiFd);
                    break;
            } else {
                   ALOGI("Power on device node(%s) gWifiFd:%d Fail (%d) and retry\n", CUST_COMBO_WIFI_DEV, gWifiFd, i_ret);
            }
            retryCounter++;
            usleep(300000);
        } while (retryCounter < 20);
    }
    close(gWifiFd);

    return 0;
}
#endif /* MTK_COMBO_ENABLE_WIFI_IN_LAUNCHER */

static void* launcher_pwr_on_chip(void * arg) {
        int retryCounter = 0;
        int i_ret = -1;
        int chipid = *(int*) arg;
        char readyStr[PROPERTY_VALUE_MAX] = {0};
        int iRet = -1;

        pthread_setname_np(pthread_self(), "pwr_on_conn");

        ALOGI("enter power on connsys flow");
        do {
                i_ret = ioctl(gWmtFd, WMT_IOCTL_LPBK_POWER_CTRL, 1);
                if (0 == i_ret) {
                        break;
                } else {
                        ioctl(gWmtFd, WMT_IOCTL_LPBK_POWER_CTRL, 0);
                        ALOGI("power on %x failed, retrying, retry counter:%d\n", chipid, retryCounter);
                        usleep(1000000);
                }
                retryCounter++;
        } while (retryCounter < 20);

#if (MTK_COMBO_ENABLE_WIFI_IN_LAUNCHER==1)
        ALOGI("enter enable wifi");
        enable_wifi();
#endif /* MTK_COMBO_ENABLE_WIFI_IN_LAUNCHER */

        pthread_detach(thread_handle);
        thread_handle = -1;

        return NULL;
}
static void* launcher_set_fwdbg_flag(void * arg) {
    int i_ret = -1;
    int flag = *(int*) arg;

    pthread_setname_np(pthread_self(), "dump_fwemi_log");
    ALOGI("dump firmware dbg log from emi buffer ");
    i_ret = ioctl(gWmtFd, WMT_IOCTL_FW_DBGLOG_CTRL, flag);
    if (i_ret < 0) {
        ALOGI("ioctl error: err msg: %s\n", strerror(errno));
        pthread_detach(thread_handle);
        thread_handle = -1;
    }
    return NULL;
}

/*
 * -m: mode (SDIO/UART)
 * -d: uart device node
 * -b: baudrate
 * -c: enable SW FC or not
 * -p: patch folder path
 * -n: patch file name (fullpath) 
 *
 */
int main(int argc, char *argv[]) {
    static const char *opString = "m:d:b:c:p:n:?";
    struct uart_t *u = NULL;
    int opt, ld, err;
    int baud = 0;
    struct sigaction sa;
    struct pollfd fds[2];
    int fd_num = 0;
    int len = 0;
    int uartFcCtrl = 0;
    int argCount = 0;
    int chipId = -1;
    int i_ret = 1;
    int retry = 0;
    int polling_flag = 0;
    int fwdbgEnableFlag = 0;
    char readyStr[PROPERTY_VALUE_MAX] = {0};
    STP_PARAMS_CONFIG sStpParaConfig;
    char fwStateStr0[PROPERTY_VALUE_MAX] = {0};
    char fwStateStr[PROPERTY_VALUE_MAX] = {0};

    do {
        i_ret = get_drv_rdy();
        if (i_ret)
            break;
        usleep(300000);
    } while (1);

    do {
            gWmtFd = open(CUST_COMBO_WMT_DEV, O_RDWR | O_NOCTTY);
            if (gWmtFd < 0) {
                ALOGI("Can't open device node(%s) error:%d \n", CUST_COMBO_WMT_DEV, gWmtFd);
                usleep(300000);
            } else
                break;
    } while (1);
    ALOGE("open device node succeed.(Node:%s, fd:%d) \n", CUST_COMBO_WMT_DEV, gWmtFd);
    do {
            chipId = query_chip_id();
            if (0 > chipId) {
                usleep(300000);
                chipId = ioctl(gWmtFd, WMT_IOCTL_WMT_QUERY_CHIPID, NULL);
                ALOGI("chiId from kernel by ioctl:0x%04x\n", chipId);
                if (-1 != chipId)
                    break;
                } else
                    break;
    } while (1);
    ALOGI("chiId:0x%04x\n", chipId);

    if ((0x0321 == chipId) || (0x0335 == chipId) || (0x0337 == chipId)) {
        chipId = 0x6735;
        ALOGI("for denali chipid convert\n");
    }
    if (0x0326 == chipId) {
            chipId = 0x6755;
            ALOGI("for jade chipid convert\n");
    }
    if ((0x6735 == chipId) || (0x6752 == chipId) || (0x6582 == chipId) || (0x6592 == chipId)
              || (0x6572 == chipId) || (0x6571 == chipId) || (0x8127 == chipId)
              || (0x8163 == chipId) || (0x6580 == chipId) || (0x6755 == chipId)
              || (0x6797 == chipId) || (0x7623 == chipId) || (0x8167 == chipId)) {
                ALOGI("run SOC chip flow\n");
                gStpMode = STP_BTIF_FULL;
                memset(gPatchFolder, 0, sizeof(gPatchFolder));
                opt = getopt(argc, argv, opString);
                while (opt != -1) {
                    switch (opt) {
                        case 'm':
                            gStpMode = atoi(optarg);
                            sStpParaConfig.eStpMode  = gStpMode;
                            ALOGI("stpmode[%d]\n", gStpMode);
                            break;
                        case 'p':
                            // gPatchFolder = optarg;
                            strcpy(gPatchFolder, optarg);
                            sStpParaConfig.pPatchPath = gPatchFolder;
                            break;
                        case '?':
                        default:
                            display_usage(chipId);
                            break;
                    }
                    opt = getopt(argc, argv, opString);
                }
        /* send default patch file name path to driver */
        ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_NAME, gPatchName);
        /* set fm mode & stp mode*/
        ioctl(gWmtFd, WMT_IOCTL_SET_STP_MODE, ((gFmMode & 0x0F) << 4)  |(gStpMode & 0x0F));
#ifndef WMT_PLAT_APEX
       set_coredump_flag();
#endif
       } else {
           ALOGI("run combo chip flow\n");
           sStpParaConfig.pPatchPath = NULL;
           sStpParaConfig.pPatchName = NULL;
           sStpParaConfig.gStpDev = NULL;
           sStpParaConfig.eStpMode  = -1;
           sStpParaConfig.iBaudrate  = -1;
           sStpParaConfig.sUartConfig.fc = UART_DISABLE_FC;
           sStpParaConfig.sUartConfig.parity = 0;
           sStpParaConfig.sUartConfig.stop_bit = 0;
           /*Default parameters starts*/
           baud = 4000000;
           gStpMode = -1;
           uartFcCtrl = UART_DISABLE_FC;
           strncpy(gStpDev, CUST_COMBO_STP_DEV, sizeof(gStpDev) - 1);
           gStpDev[sizeof(gStpDev) - 1] = '\0';
           memset(gPatchFolder, 0, sizeof(gPatchFolder));
           memset(gPatchName, 0, sizeof(gPatchName));
           /*Default parameters ends*/
           opt = getopt(argc, argv, opString);
           while (opt != -1) {
               switch (opt) {
                   case 'm':
                        gStpMode = atoi(optarg);
                        sStpParaConfig.eStpMode  = gStpMode;
                        break;
                   case 'd':
                        strncpy(gStpDev, optarg, sizeof(gStpDev) - 1);
                        gStpDev[sizeof(gStpDev) - 1] = '\0';
                        sStpParaConfig.gStpDev = gStpDev;
                        break;
                   case 'b':
                        baud = atoi(optarg);
                        sStpParaConfig.iBaudrate = baud;
                        break;
                   case 'c':
                        uartFcCtrl = atoi(optarg);
                        sStpParaConfig.sUartConfig.fc = uartFcCtrl;
                        ALOGI("c found\n");
                        break;
                   case 'p':
                       // gPatchFolder = optarg;
                       strcpy(gPatchFolder, optarg);
                       sStpParaConfig.pPatchPath = gPatchFolder;
                       break;
                   case 'n':
                       // gPatchName = optarg;
                       strcpy(gPatchName, optarg);
                       sStpParaConfig.pPatchName = gPatchName;
                       break;
                   case '?':
                   default:
                       display_usage(chipId);
                       break;
              }
             opt = getopt(argc, argv, opString);
         }
         if (0 > get_chip_info_index(chipId)) {
             ALOGI("invalid chip, check again\n");
             chipId = query_chip_id();
             ALOGI("chiId:0x%04x\n", chipId);
         }
         ioctl(gWmtFd, WMT_IOCTL_WMT_TELL_CHIPID, chipId);
         ALOGI("set chipId(0x%x) to HIF-SDIO module\n", chipId);

         get_wmt_cfg(chipId);

         setHifInfo(chipId, sStpParaConfig.pPatchPath);
         ALOGI("HifConfig:0x%04x, wmtCfgFile:%s\n", sStpParaConfig.eStpMode, gWmtCfgName);
#ifndef WMT_PLAT_APEX
        set_coredump_flag();
#endif
        if (0 != para_valid_check(&sStpParaConfig)) {
            // Try to use custom method to check parameters
            if (argc > optind) {
            // For this competible usage , we only left STP mode set and firmware patch input, omit flowcontrol set
                // First baud for STP UART mode, otherwise, SDIO mode
                baud = atoi(argv[optind]);
                if (baud >=  CUST_BAUDRATE_DFT) {
                    ALOGI("get baud rate(%d) for UART mode\n", baud);
                    gStpMode = STP_UART_FULL;
                    sStpParaConfig.iBaudrate = baud;
                }
                else if (baud == 1) {
                    ALOGI("Definitively use SDIO mode\n");
                    gStpMode = STP_SDIO;
                } else {
                    ALOGI("invalid baud rate(%d) for UART, use SDIO mode\n", baud);
                    gStpMode = STP_SDIO;
                }
                sStpParaConfig.eStpMode  = gStpMode;
                // Firmare patch analysis
                optind++;
                memset(gPatchName, 0, sizeof(gPatchName));
                if (argc > optind) {
                   strncat(gPatchName, argv[optind], sizeof(gPatchName)-1);
                   sStpParaConfig.pPatchName = gPatchName;
                   ALOGI("PatchFile:%s\n", sStpParaConfig.pPatchName);
                } else {
                   sStpParaConfig.pPatchName = NULL;
                   ALOGI("no patch file \n");
                }
                // Flow Control analysis
                optind++;
                if (argc > optind) {
                    uartFcCtrl = atoi(argv[optind]);
                    sStpParaConfig.sUartConfig.fc = uartFcCtrl;
                    ALOGI("flowcontrol flag: %d\n", sStpParaConfig.sUartConfig.fc);
                } else {
                    ALOGI("no flow control flat set\n");
                }
            }
        }
        if (0 != para_valid_check(&sStpParaConfig)) {
            display_usage(chipId);
        }
        /* send default patch file name path to driver */
        ioctl(gWmtFd, WMT_IOCTL_SET_PATCH_NAME, gPatchName);
        /* send uart name to driver*/
        if (sStpParaConfig.gStpDev) {
            gUartName = strstr(sStpParaConfig.gStpDev, "tty");
            if (!gUartName) {
                ALOGI("no uart name found in %s\n", sStpParaConfig.gStpDev);
            } else {
                ALOGI("uart name %s\n", gUartName);
            }
        }

        if (!gUartName) {
            gUartName = "ttyMT2";
            ALOGI("use default uart %s\n", gUartName);
        }

        ioctl(gWmtFd, WMT_IOCTL_PORT_NAME, gUartName);

        /* send hardware interface configuration to driver */
        ioctl(gWmtFd, WMT_IOCTL_SET_STP_MODE, ((baud & 0xFFFFFF) << 8) | ((gFmMode & 0x0F) << 4)  |(gStpMode & 0x0F));

        /* send WMT config name configuration to driver */
        ioctl(gWmtFd, WMT_IOCTL_WMT_CFG_NAME, gWmtCfgName);
    }

    ioctl(gWmtFd, WMT_IOCTL_SET_LAUNCHER_KILL, 0);

    i_ret = ioctl(gWmtFd, WMT_IOCTL_GET_APO_FLAG, NULL);
    if (i_ret != 0) {
        if (pthread_create(&thread_handle, NULL, launcher_pwr_on_chip, &chipId)) {
            ALOGE("create pwr on thread fail\n");
        } else {
            ALOGI("create pwr on thread ok\n");
        }
    } else {
        ALOGI("no supported always power on\n");
    }
    /*set signal handler*/
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags   = SA_NOCLDSTOP;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = sig_term;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);

    sa.sa_handler = sig_hup;
    sigaction(SIGHUP, &sa, NULL);

    fds[0].fd = gWmtFd; /* stp_wmt fd */
    fds[0].events = POLLIN | POLLRDNORM; /* wait read events */
    ++fd_num;

    while (!__io_canceled) {
        fds[0].revents = 0;
        err = poll(fds, fd_num, 2000);  // 5 seconds
        if (err < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                ALOGI("poll error:%d errno:%d, %s\n", err, errno, strerror(errno));
                break;
            }
        }
        else if (!err) {
            continue;
        }
        if (fds[0].revents & POLLIN) {
            memset(gCmdStr, 0, sizeof(gCmdStr));
            len = read(gWmtFd, gCmdStr, sizeof(gCmdStr)-1);
            if (len > 0 && len < (int)sizeof(gCmdStr)) {
                // printf ("POLLIN(%d) and read(%d)\n", gWmtFd, len);
            }
            else {
                ALOGI("POLLIN(%d) but read fail:%d\n", gWmtFd, len);
                continue;
            }
            gCmdStr[len] = '\0';
            // ALOGI("rx_cmd_str:%s\n", gCmdStr);
            err = handle_cmd(&sStpParaConfig, gCmdStr, len);
            if (!err) {
                // ALOGI("handle_cmd(%s), respond ok \n", gCmdStr);
                snprintf(gRespStr, sizeof(gRespStr), "ok");
            }
            else {
                if (err == 1) {
                    snprintf(gRespStr, sizeof(gRespStr), "cmd not found");
                }
                else {
                    snprintf(gRespStr, sizeof(gRespStr), "resp_%d", err);
                }
            }
            ALOGI("cmd(%s) resp(%s)\n", gCmdStr, gRespStr);
            len = write(gWmtFd, gRespStr, strlen(gRespStr));
            if (len != (int)strlen(gRespStr)) {
                fprintf(stderr, "write resp(%d) fail: len(%d), errno(%d, %s)\n",
                    gWmtFd, len, errno, (len == -1) ? strerror(errno) : "");
            }
        }
    }
clean_up:

    if (gWmtFd >= 0) {
        close(gWmtFd);
        gWmtFd = -1;
    }

    if (gStpMode == STP_UART_FULL && gTtyFd >= 0) {
        /* Restore TTY line discipline */
        ld = N_TTY;
        if (ioctl(gTtyFd, TIOCSETD, &ld) < 0) {
            ALOGE("Can't restore line discipline");
            exit(1);
        }

        close(gTtyFd);
        gTtyFd = -1;
    }

    return 0;
}


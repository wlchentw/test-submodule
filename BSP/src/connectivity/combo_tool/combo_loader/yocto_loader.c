#include "loader_pwr.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
// For directory operation
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "wmt_loader"

#define WCN_COMBO_LOADER_CHIP_ID_PROP    "persist.mtk.wcn.combo.chipid"
#ifndef ALOGI
#define ALOGI printf
#endif
#ifndef PROPERTY_VALUE_MAX
#define PROPERTY_VALUE_MAX (128)
#endif

#define WCN_COMBO_LOADER_DEV    "/dev/wmtdetect"
#define WCN_COMBO_DEF_CHIPID    "0x6582"
#define WMT_MODULES_PRE         "/system/lib/modules/"
#define WMT_KO_PATH_PREFIX      "/lib/modules/"
#define WMT_WIFI_MODULE_PATH    "/kernel/drivers/misc/mediatek/connectivity/wlan/wmt_chrdev_wifi.ko"
#define WMT_WLAN_MODULE_PATH    "/kernel/drivers/misc/mediatek/connectivity/wlan/gen3/wlan_gen3.ko"
/* connectivity modules are built out of kernel tree */
#define WMT_OUT_KO_PATH_PREFIX          "/lib/modules/mt66xx/"
#define WMT_OUT_WMT_WIFI_MODULE_PATH    "wmt_chrdev_wifi.ko"
#define WMT_OUT_WLAN_MODULE_PATH        "wlan_drv_gen3.ko"

#define WMT_MODULES_SUFF        ".ko"
#define WMT_IOC_MAGIC           'w'
#define COMBO_IOCTL_GET_CHIP_ID       _IOR(WMT_IOC_MAGIC, 0, int)
#define COMBO_IOCTL_SET_CHIP_ID       _IOW(WMT_IOC_MAGIC, 1, int)
#define COMBO_IOCTL_EXT_CHIP_DETECT   _IOR(WMT_IOC_MAGIC, 2, int)
#define COMBO_IOCTL_GET_SOC_CHIP_ID   _IOR(WMT_IOC_MAGIC, 3, int)
#define COMBO_IOCTL_DO_MODULE_INIT    _IOR(WMT_IOC_MAGIC, 4, int)
#define COMBO_IOCTL_MODULE_CLEANUP    _IOR(WMT_IOC_MAGIC, 5, int)
#define COMBO_IOCTL_EXT_CHIP_PWR_ON   _IOR(WMT_IOC_MAGIC, 6, int)
#define COMBO_IOCTL_EXT_CHIP_PWR_OFF  _IOR(WMT_IOC_MAGIC, 7, int)
#define COMBO_IOCTL_DO_SDIO_AUDOK     _IOR(WMT_IOC_MAGIC, 8, int)



#define STP_WMT_MODULE_PRE_FIX "mtk_stp_wmt"
#define STP_BT_MODULE_PRE_FIX "mtk_stp_bt"
#define STP_GPS_MODULE_PRE_FIX "mtk_stp_gps"
#define HIF_SDIO_MODULE_PRE_FIX "mtk_hif_sdio"
#define STP_SDIO_MODULE_PRE_FIX "mtk_stp_sdio"
#define STP_UART_MODULE_PRE_FIX "mtk_stp_uart"



static int gLoaderFd = -1;

static char DRIVER_MODULE_PATH[64]  = {0};
static char DRIVER_MODULE_ARG[8] = "";
static int chipid_array[] = {
0x6620,
0x6628,
0x6630,
0x6572,
0x6582,
0x6592,
0x8127,
0x6571,
0x6752,
0x6735,
0x0321,
0x0335,
0x0337,
0x8163,
0x6580,
0x6755,
0x0326,
0x6797,
0x0279
};
static char chip_version[PROPERTY_VALUE_MAX] = {0};

static int g_remove_ko_flag = 1;


extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);
extern int load_fm_module(int chip_id);
//extern int load_wifi_module(int chip_id);
extern int load_ant_module(int chip_id);
// insmod
static int is_chipId_vaild(int chipid) {
        int iret;
        unsigned char i;
        iret = -1;

        for (i = 0; i < sizeof(chipid_array)/sizeof(0x6630); i++) {
                 if (chipid == chipid_array[i]) {
                        ALOGI("is_chipId_vaild: %d :0x%x!\n", i, chipid);
                        iret = 0;
                        break;
                 }
        }
        return iret;
}

int do_kernel_module_init(int gLoaderFd, int chipId) {
        int iRet = 0;
        if (gLoaderFd < 0) {
                ALOGI("invalid gLoaderFd: %d\n", gLoaderFd);
                return -1;
        }

        iRet = ioctl(gLoaderFd, COMBO_IOCTL_MODULE_CLEANUP, chipId);
        if (iRet) {
                ALOGI("do WMT-DETECT module cleanup failed: %d\n", iRet);
                return -2;
        }
        iRet = ioctl(gLoaderFd, COMBO_IOCTL_DO_MODULE_INIT, chipId);
        if (iRet) {
                ALOGI("do kernel module init failed: %d\n", iRet);
                return -3;
        }
        ALOGI("do kernel module init succeed: %d\n", iRet);
        return 0;
}

void *load_file(const char *fn, unsigned *_sz)
{
	char *data;
	int sz;
	int fd;
	data = 0;

	fd = open(fn, O_RDONLY);
	if(fd < 0) return 0;

	sz = lseek(fd, 0, SEEK_END);
	if(sz < 0)
		goto oops;

	if(lseek(fd, 0, SEEK_SET) != 0)
		goto oops;

	data = (char*) malloc(sz + 1);
	if(data == 0)
		goto oops;

	if(read(fd, data, sz) != sz)
		goto oops;

	close(fd);    data[sz] = 0;

	if(_sz)
		*_sz = sz;

	return data;

oops:
	close(fd);
	if(data != 0)
		free(data);

	return 0;
}

//insmod
static int insmod(const char *filename, const char *args)
{
	void *module;
	unsigned int size;
	int ret;

	module = load_file(filename, &size);
	if (!module)
		return -1;

	ret = init_module(module, size, args);

	free(module);

	return ret;
}


int load_wifi_module(int chip_id)
{
	int ret = -1;
	struct utsname utsname;
	char wifi_module_path[100] = "";
	char wlan_module_path[100] = "";

	if (!(uname(&utsname))) {
		/* Use modules built within kernel tree */
		sprintf(wifi_module_path, "%s%s%s", WMT_KO_PATH_PREFIX, utsname.release, WMT_WIFI_MODULE_PATH);
		sprintf(wlan_module_path, "%s%s%s", WMT_KO_PATH_PREFIX, utsname.release, WMT_WLAN_MODULE_PATH);
	} else
		return -1;

	if ((access(wifi_module_path,0) < 0) && (access(wlan_module_path,0) < 0))
	{
		/* Use modules built out of kernel tree */
		printf("Use modules built out of kernel tree\n");
		memset(wifi_module_path, 0, sizeof(wifi_module_path));
		memset(wlan_module_path, 0, sizeof(wlan_module_path));
		sprintf(wifi_module_path, "%s%s", WMT_OUT_KO_PATH_PREFIX, WMT_OUT_WMT_WIFI_MODULE_PATH);
		sprintf(wlan_module_path, "%s%s", WMT_OUT_KO_PATH_PREFIX, WMT_OUT_WLAN_MODULE_PATH);
	}

	if (chip_id == 0x6630) {
		//insert 6630 driver
		if(0 == insmod(wifi_module_path, DRIVER_MODULE_ARG)){
			ret = 0;
			printf("Success to insmod wmt wifi module\n");
		} else
			printf("Fail to insmod wmt wifi module %s\n", wifi_module_path);

		if(0 == insmod(wlan_module_path, DRIVER_MODULE_ARG)){
			ret = 0;
			printf("Success to insmod wlan module\n");
		} else
			printf("Fail to insmod wlan module %s\n", wlan_module_path);
    }

	return ret;
}


/* TBD in platform-specific way */
static int get_persist_chip_id(char *str, size_t len) { return -1; }
static int set_persist_chip_id(int id) { return 0; }
static void set_proc_owner(void) { }
static void update_driver_ready(void) { }

int main(int argc, char *argv[]) {
        int iRet = -1;
        int noextChip = -1;
        int chipId = -1;
        int count = 0;
        char chipidStr[PROPERTY_VALUE_MAX] = {0};
        char readyStr[PROPERTY_VALUE_MAX] = {0};
        int loadFmResult = -1;
        int loadAntResult = -1;
        int loadWlanResult = -1;
        int retryCounter = 1;
        int autokRet = 0;
        do {
                gLoaderFd = open(WCN_COMBO_LOADER_DEV, O_RDWR | O_NOCTTY);
                if (gLoaderFd < 0) {
                        count++;
                        ALOGI("Can't open device node(%s) count(%d)\n", WCN_COMBO_LOADER_DEV, count);
                        usleep(300000);
                } else
                        break;
        } while (1);

        // read from system property
        chipId = get_persist_chip_id(chipidStr, sizeof(chipidStr));

        if (-1 != is_chipId_vaild(chipId)) {
                /*valid chipid detected*/
                ALOGI("key:(%s)-value:(%s),chipId:0x%04x,iRet(%d)\n",
                        WCN_COMBO_LOADER_CHIP_ID_PROP, chipidStr, chipId, iRet);
                if (0x6630 == chipId) {
                        retryCounter = 10;
                        /*trigger autok process, incase last autok process is 
                          interrupted by abnormal power off or battery down*/
                        do {
                                /*power on combo chip*/
                                iRet = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_PWR_ON);
                                if (0 != iRet) {
                                        ALOGI("external combo chip power on failed\n");
                                        noextChip = 1;
                                } else {
                                        /*detect is there is an external combo chip*/
                                        noextChip = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_DETECT, NULL);
                                }

                                if (noextChip) {
                                         // do nothing
                                         ALOGI("no external combo chip detected\n");
                                } else {
                                         ALOGI("external combo chip detected\n");
                                         chipId = ioctl(gLoaderFd, COMBO_IOCTL_GET_CHIP_ID, NULL);
                                         ALOGI("chipid (0x%x) detected\n", chipId);
                               }

                                if (0 == noextChip) {
                                        autokRet = ioctl(gLoaderFd, COMBO_IOCTL_DO_SDIO_AUDOK, chipId);
                                        if (0 != autokRet) {
                                                ALOGI("do SDIO3.0 autok failed\n");
                                        } else {
                                                ALOGI("do SDIO3.0 autok succeed\n");
                                        }
                                }
                                iRet = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_PWR_OFF);
                                if (0 != iRet) {
                                        ALOGI("external combo chip power off failed\n");
                                } else {
                                        ALOGI("external combo chip power off succeed\n");
                                }
                                if ((0 == noextChip) && (-1 == chipId)) {
                                        /*extenral chip detected, but no valid chipId detected, retry*/
                                        retryCounter--;
                                        ALOGI("chipId detect failed, retrying, left retryCounter:%d\n", retryCounter);
                                        usleep(500000);
                                } else
                                        break;
                        }while (0 < retryCounter);
                        chipId = 0x6630;
                 }
        } else {
                /*trigger external combo chip detect and chip identification process*/
                do {
                        /*power on combo chip*/
                        iRet = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_PWR_ON);
                        if (0 != iRet) {
                                ALOGI("external combo chip power on failed\n");
                                noextChip = 1;
                        } else {
                                /*detect is there is an external combo chip*/
                                noextChip = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_DETECT, NULL);
                        }

                        if (noextChip) {   // use soc itself
                                ALOGI("no external combo chip detected, get current soc chipid\n");
                                chipId = ioctl(gLoaderFd, COMBO_IOCTL_GET_SOC_CHIP_ID, NULL);
                                ALOGI("soc chipid (0x%x) detected\n", chipId);
                        } else {
                                ALOGI("external combo chip detected\n");
                                chipId = ioctl(gLoaderFd, COMBO_IOCTL_GET_CHIP_ID, NULL);
                                ALOGI("chipid (0x%x) detected\n", chipId);
                       }

                        sprintf(chipidStr, "0x%04x", chipId);
                        iRet = set_persist_chip_id(chipId);
                        if (0 != iRet) {
                                ALOGI("set property(%s) to %s failed,iRet:%d, errno:%d\n",
                                    WCN_COMBO_LOADER_CHIP_ID_PROP, chipidStr, iRet, errno);
                        } else {
                               ALOGI("set property(%s) to %s succeed.\n", WCN_COMBO_LOADER_CHIP_ID_PROP, chipidStr);
                        }
                        if (0 == noextChip) {
                                autokRet = ioctl(gLoaderFd, COMBO_IOCTL_DO_SDIO_AUDOK, chipId);
                                if (0 != autokRet) {
                                        ALOGI("do SDIO3.0 autok failed\n");
                                } else {
                                        ALOGI("do SDIO3.0 autok succeed\n");
                                }
                        }
                        iRet = ioctl(gLoaderFd, COMBO_IOCTL_EXT_CHIP_PWR_OFF);
                        if (0 != iRet) {
                                ALOGI("external combo chip power off failed\n");
                        } else {
                                ALOGI("external combo chip power off succeed\n");
                        }
                        if ((0 == noextChip) && (-1 == chipId)) {
                            /*extenral chip detected, but no valid chipId detected, retry*/
                            retryCounter--;
                            usleep(500000);
                            ALOGI("chipId detect failed, retrying, left retryCounter:%d\n", retryCounter);
                        } else
                              break;
                }while (0 < retryCounter);
        }

        /*set chipid to kernel*/
        ioctl(gLoaderFd, COMBO_IOCTL_SET_CHIP_ID, chipId);

        if (g_remove_ko_flag) {
                if ((0x0321 == chipId) || (0x0335 == chipId) || (0x0337 == chipId)) {
                        chipId = 0x6735;
                }
                if (0x0326 == chipId) {
                        chipId = 0x6755;
                }
                if (0x0279 == chipId) {
                        chipId = 0x6797;
                }
                do_kernel_module_init(gLoaderFd, chipId);
                if (gLoaderFd >= 0) {
                        close(gLoaderFd);
                        gLoaderFd = -1;
               }

        } else {
#if 0
                if (gLoaderFd >= 0) {
                        close(gLoaderFd);
                        gLoaderFd = -1;
                }
                ALOGI("rmmod mtk_wmt_detect\n");
                rmmod("mtk_wmt_detect");
                /*INSERT TARGET MODULE TO KERNEL*/
                iRet = insert_wmt_modules(chipId, 0, -1);
                /*this process should never fail*/
                if (iRet) {
                        ALOGI("insert wmt modules fail(%d):(%d)\n", iRet, __LINE__);
                        /*goto done;*/
                }

                loadFmResult = load_fm_module(chipId);
                if (loadFmResult) {
                        ALOGI("load FM modules fail(%d):(%d)\n", iRet, __LINE__);
                        /*continue, we cannot let this process interrupted by subsystem module load fail*/
                        /*goto done;*/
                }

                loadAntResult = load_ant_module(chipId);
                if (loadAntResult) {
                        ALOGI("load ANT modules fail(%d):(%d)\n", iRet, __LINE__);
                        /*continue, we cannot let this process interrupted by subsystem module load fail*/
                        /*goto done;*/
                }

                loadWlanResult = load_wifi_module(chipId);
                if (loadWlanResult) {
                        ALOGI("load WIFI modules fail(%d):(%d)\n", iRet, __LINE__);
                        /*continue, we cannot let this process interrupted by subsystem module load fail*/
                        /*goto done;*/
                }
#endif
        }

        // chown to set proc owner
        set_proc_owner();
        if (0/*0x6630 == chipId*/) {
                retryCounter = 0;
                int i_ret = -1;
                do {
                        i_ret = loader_wmt_pwr_ctrl(1);
                        if (0 == i_ret)
                                break;
                        else {
                                loader_wmt_pwr_ctrl(0);
                                ALOGI("power on %x failed, retrying, retry counter:%d\n", chipId, retryCounter);
                                usleep(1000000);
                       }
                       retryCounter++;
                } while (retryCounter < 20);
        }

	//insmod wmt_chrdev_wifi.ko & wlan_gen3.ko
	loadWlanResult = load_wifi_module(chipId);
	if (loadWlanResult) {
		ALOGI("load WIFI modules fail(%d):(%d)\n", iRet, __LINE__);
	}

        // update wmt driver ready
        update_driver_ready();

        return iRet;
}




#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "NvRAMUtils.h"

#define USE_MTK_NVRAM 1

#if USE_MTK_NVRAM
#include "libnvram.h"
#include "CFG_file_lid.h"
#include "Custom_NvRam_LID.h"
#include "CFG_Wifi_File.h"
#include "CFG_BT_File.h"
#include "CFG_PRODUCT_INFO_File.h"
#include "libfile_op.h"

bool r_WiFi_CFG(struct WIFI_CFG_STRUCT* param)
{
    int rec_size = 0;
    int rec_num = 0;
    F_ID wifi_nvram_fd;
    WIFI_CFG_PARAM_STRUCT wifi_nvram;

    if(param == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    wifi_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_WIFI_LID, &rec_size, &rec_num, true/*read flag*/);
    if(wifi_nvram_fd.iFileDesc < 0) {
        fprintf(stderr, "NVM_GetFileDesc fail\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr, "Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if (rec_size != sizeof(WIFI_CFG_PARAM_STRUCT)) {
        fprintf(stderr, "Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                rec_size, sizeof(WIFI_CFG_PARAM_STRUCT));
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if (read(wifi_nvram_fd.iFileDesc, &wifi_nvram, rec_num*rec_size) < 0) {
        fprintf(stderr, "Read NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(wifi_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc warnning\n");
    }

    memcpy(param, &wifi_nvram.aucMacAddress, 6);

	return true;
}

bool w_WiFi_CFG(struct WIFI_CFG_STRUCT* param)
{
    int rec_size = 0;
    int rec_num = 0;
    F_ID wifi_nvram_fd;
    WIFI_CFG_PARAM_STRUCT wifi_nvram;

    if(param == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    wifi_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_WIFI_LID, &rec_size, &rec_num, true/*read flag*/);
    if(wifi_nvram_fd.iFileDesc < 0) {
        fprintf(stderr, "NVM_GetFileDesc fail\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr, "Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if (rec_size != sizeof(WIFI_CFG_PARAM_STRUCT)) {
        fprintf(stderr, "Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                rec_size, sizeof(WIFI_CFG_PARAM_STRUCT));
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if (read(wifi_nvram_fd.iFileDesc, &wifi_nvram, rec_num*rec_size) < 0) {
        fprintf(stderr, "Read NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(wifi_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc warnning\n");
    }

    memcpy(&wifi_nvram.aucMacAddress, param, 6);

    wifi_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_WIFI_LID, &rec_size, &rec_num, false/*write flag*/);
    if(wifi_nvram_fd.iFileDesc < 0) {
        fprintf(stderr, "NVM_GetFileDesc fail\n");
        return false;
    }
        if (rec_num != 1) {
            fprintf(stderr,"Unexpected record num %d\n", rec_num);
            NVM_CloseFileDesc(wifi_nvram_fd);
            return false;
        }

        if (rec_size != sizeof(WIFI_CFG_PARAM_STRUCT)) {
            fprintf(stderr,"Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                    rec_size, sizeof(WIFI_CFG_PARAM_STRUCT));
            NVM_CloseFileDesc(wifi_nvram_fd);
            return false;
        }

    if (write(wifi_nvram_fd.iFileDesc, &wifi_nvram, rec_num*rec_size) != rec_num*rec_size) {
        fprintf(stderr,"Write NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(wifi_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(wifi_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc error\n");
    }

    return true;
}

bool r_BT_CFG(struct BT_CFG_STRUCT* param)
{
    int rec_size = 0;
    int rec_num = 0;
    F_ID bt_nvram_fd;
    ap_nvram_btradio_struct bt_nvram;

    if(param == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, true/*read flag*/);
    if(bt_nvram_fd.iFileDesc < 0) {
        fprintf(stderr, "NVM_GetFileDesc fail\n");
        return false;
    }
    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (rec_size != sizeof(ap_nvram_btradio_struct)) {
        fprintf(stderr,"Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                rec_size, sizeof(ap_nvram_btradio_struct));
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (read(bt_nvram_fd.iFileDesc, &bt_nvram, rec_num*rec_size) < 0) {
        fprintf(stderr,"Read NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(bt_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc warnning\n");
    }

    memcpy(param, &bt_nvram.addr, 6);

    return true;
}


bool w_BT_CFG(struct BT_CFG_STRUCT* param)
{
    int rec_size = 0;
    int rec_num = 0;
    F_ID bt_nvram_fd;
    ap_nvram_btradio_struct bt_nvram;

    if(param == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, true);
    if(bt_nvram_fd.iFileDesc < 0) {
        fprintf(stderr, "NVM_GetFileDesc fail\n");
        return false;
    }
    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (rec_size != sizeof(ap_nvram_btradio_struct)) {
        fprintf(stderr,"Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                rec_size, sizeof(ap_nvram_btradio_struct));
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (read(bt_nvram_fd.iFileDesc, &bt_nvram, rec_num*rec_size) < 0) {
        fprintf(stderr,"Read NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(bt_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc warnning\n");
    }

    memcpy(&bt_nvram.addr, param, 6);

    bt_nvram_fd = NVM_GetFileDesc(AP_CFG_RDEB_FILE_BT_ADDR_LID, &rec_size, &rec_num, false/*write flag*/);
    if(bt_nvram_fd.iFileDesc <0 ) {

    }
    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (rec_size != sizeof(ap_nvram_btradio_struct)) {
        fprintf(stderr,"Unexpected record size %d WIFI_CFG_PARAM_STRUCT %d\n",
                rec_size, sizeof(ap_nvram_btradio_struct));
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if (write(bt_nvram_fd.iFileDesc, &bt_nvram, rec_num*rec_size) != rec_num*rec_size) {
        fprintf(stderr,"Write NVRAM fails %d\n", errno);
        NVM_CloseFileDesc(bt_nvram_fd);
        return false;
    }

    if(!NVM_CloseFileDesc(bt_nvram_fd)) {
        fprintf(stderr, "NVM_CloseFileDesc error\n");
    }

    return true;
}


bool b_CFG()
{
    return FileOp_BackupToBinRegion_All();
}

bool r_XOCAP() {
	int rec_size = 0;
    int rec_num = 0;
    F_ID xocap_nvram_fd;
 
    xocap_nvram_fd = NVM_GetFileDesc(AP_CFG_CUSTOM_FILE_XOCAP_LID, &rec_size, &rec_num, true);
    if(xocap_nvram_fd.iFileDesc != -1) {
        if (rec_num != 1) {
            fprintf(stderr,"Unexpected record num %d\n", rec_num);
            NVM_CloseFileDesc(xocap_nvram_fd);
            return false;
        }

       
        NVM_CloseFileDesc(xocap_nvram_fd);
    }
	return true;
}


bool r_SN(char* sn) {
	F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, true /*read*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }

    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);

    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }

    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    memcpy(sn, pBuf, SN_LEN);

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
	free(pBuf);
	
    return true;
}

bool w_SN(char* sn) {
	F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, false /*write*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }

    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);

    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }

    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    /// to-do
    memcpy(pBuf, sn, strlen(sn));

    if(lseek(fid.iFileDesc, 0, SEEK_SET) < 0) {
        fprintf(stderr,"Seek PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    if (write(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Write PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
	free(pBuf);
    return true;
}

bool r_PCBSN(char* sn) {
	F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, true /*read*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }

    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);

    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }

    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    memcpy(sn, pBuf + 32, PCBSN_LEN);

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
	free(pBuf);
	
    return true;
}

bool w_PCBSN(char* sn) {
	F_ID fid;
    int rec_size = 0;
    int rec_num = 0;
    char* pBuf = NULL;

    if(sn == NULL) {
        fprintf(stderr,"%s param is null\n", __FUNCTION__);
        return false;
    }

    fid = NVM_GetFileDesc(AP_CFG_REEB_PRODUCT_INFO_LID, &rec_size, &rec_num, false /*write*/);

    if(fid.iFileDesc == -1) {
        fprintf(stderr,"Open PRODUCT_INFO fail!\n");
        return false;
    }

    if (rec_num != 1) {
        fprintf(stderr,"Unexpected record num %d\n", rec_num);
        NVM_CloseFileDesc(fid);
        return false;
    }

    fprintf(stderr,"PRODUCT_INFO rec_num %d, rec_size %d\n", rec_num, rec_size);

    pBuf = malloc(rec_num * rec_size);
    if(pBuf == NULL) {
        fprintf(stderr,"malloc fali %d\n", rec_num * rec_size);
        return false;
    }

    if (read(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Read PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    /// to-do
    memcpy(pBuf + 32, sn, PCBSN_LEN);

    if(lseek(fid.iFileDesc, 0, SEEK_SET) < 0) {
        fprintf(stderr,"Seek PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    if (write(fid.iFileDesc, pBuf, rec_num * rec_size) < 0) {
        fprintf(stderr,"Write PRODUCT_INFO fails %d\n", errno);
        NVM_CloseFileDesc(fid);
		free(pBuf);
        return false;
    }

    if(!NVM_CloseFileDesc(fid)) {
        fprintf(stderr,"Close product info error!");
    }
	free(pBuf);
    return true;
}

#endif /* USE_MTK_NVRAM */

#if USE_DT
bool r_WiFi_CFG(struct WIFI_CFG_STRUCT* param)
{
    return false;
}

bool w_WiFi_CFG(struct WIFI_CFG_STRUCT* param)
{
    return false;
}

bool r_BT_CFG(struct BT_CFG_STRUCT* param)
{
    return false;
}
bool w_BT_CFG(struct BT_CFG_STRUCT* param)
{
    return false;
}

bool b_CFG() {
	return false;
}
#endif /* USE_DT */

/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/

#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


//temp dir for install key
#define DRMKEY_LINUX
#ifdef DRMKEY_ANDROID
#define KB_MIX_PATH         "/data/vendor/key_provisioning/KB_MIX"
#define KB_INSTALL_LOG_PATH         "/data/vendor/key_provisioning/drm_key_install.log"
#elif defined(DRMKEY_LINUX)
#define KB_MIX_PATH         "/data/vendor/key_provisioning/KB_MIX"
#define KB_INSTALL_LOG_PATH         "/data/vendor/key_provisioning/drm_key_install.log"
#endif

//install DRM key
extern int install_KB_MIX(void);

//query DRM key
extern int query_drmkey(unsigned int *count, unsigned int **keytype);


static char DRMKeyNames[][64] =
{
"WIDEVINE",
"MARLIN",
"HDCP_1X_TX",
"HDCP_2X_V1_TX",
"HDCP_2X_V1_RX",
"HDCP_2X_V2_TX",
"HDCP_2X_V2_RX",
"PLAYREADY_BGROUPCERT",
"PLAYREADY_ZGPRIV",
"PLAYREADY_KEYFILE",
"KEYMASTER_ATTEST",
"DEVICE_RSA_KEYPAIR",
"LEK",
"GOOGLE_VOUCHER",
"DAP",
"PLAYREADY_BUNITCERT_ID",
"PLAYREADY_ZUPRIV_ID"
};

extern int access(const char* pathname,int mode);

int file_exist(const char* pathname)
{
    return (access(pathname,0)==0);
}

void Usage(void)
{
    printf("usage:\n");
    printf("drmkeydebug q\n");
    printf("  query installed drm key\n");
    printf("drmkeydebug d {key_id}  {pc_path}\n");
    printf("dump encrypted drmkey for keyID\n");
    printf("drmkeydebug e ${part_path}\n");
    printf("  erase kbf or kbo\n");
    printf("drmkeydebug i [l log_path]\n");
    printf("  install keyblock files in /data/key_provisioning/\n");
    printf("drmkeydebug i KB_MIX [l log_path]\n");
    printf("  install "KB_MIX_PATH"\n");
}

#define BUILD_TIME "2015-01-22"

int main(int argc,char* argv[])
{
    unsigned int  keycount;
    unsigned int*  keytype = NULL;
    int ret;
    unsigned int i;

    if(argc >1 && strcmp(argv[1],"q")==0)
    {
        ret = query_drmkey(&keycount, &keytype);
        if (ret == 0)
        {
            if(keycount > 0)
            {
                for (i=0; i<keycount; i++)
                {
                    if(keytype[i] == 64){
                        printf("keyname=HDCP_1_4_RX,keytype=%10d\n",keytype[i]);
                    }else{
                        printf("keyname=[%-20s],keytype=%10d\n",DRMKeyNames[keytype[i]],keytype[i]);
                    }
                }
            }
            else if(keycount == 0)
            {
                printf("No key installed\n");
            }
            if(keytype)
                free(keytype);
        }

        return 0;
    }
    else if(argc >1 && strcmp(argv[1],"i")==0)
    {
        if(argc == 2 || argc == 4)
        {
            if (file_exist(KB_MIX_PATH))
            {
                printf("Found KB_MIX in %s,installing...!\n",KB_MIX_PATH);
                if( install_KB_MIX()!= 0)
                {
                    printf("install"KB_MIX_PATH" fail\n");
                    return -7;
                }
                printf("install"KB_MIX_PATH" finished.\n");
            }

            return 0;
        }
        if (argc > 2 && strcmp(argv[2], "KB_MIX") == 0)
        {
            if( install_KB_MIX()!= 0)
            {
                printf("install"KB_MIX_PATH" fail\n");
                return -8;
            }
        }

        return 0;
    }

    Usage();

    return 0;
}

/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tee_client_api.h>

#include "efuse.h"

static TEEC_Context teec_ctx;
static TEEC_Session session;

static const char *efuse_err_strings[] = {
    "eFuse is ok.",
    "eFuse is busy.",
    "Input parameters is invalid.",
    "Incorrect length of eFuse value",
    "Can't write a zero efuse value.",
    "The eFuse can't be written.",
    "\"sensitive write\" is not allowed.",
    "Incorrect eFuse access type",
    "The eFuse has been blown.",
    "The eFuse feature is not implemented.",
    "Check eFuse read failed after eFuse is written.",
    "Token's eFuse value and lenth is not match with that defined in function \"efuse_write\".",
    "Incorrect magic number in token",
    "Incorrect hardware random id in token",
    "eFuse is locking.",
    "The eFuse command is invalid.",
    "The eFuse setting is invalid.",
    "Resource problem duing efuse operation",
    "Input efuse value is out of sepcification.",
    "failed to verify public key",
    "failed to verify signature"
};

#define efuse_err_string_num  (sizeof(efuse_err_strings)/sizeof(efuse_err_strings[0]))


static int open_ta(void)
{
    TEEC_Result rc;
    uint32_t err_origin;
    static TEEC_UUID TA_EFUSE_UUID = EFUSE_UUID;

    /* 0: open context */
    rc = TEEC_InitializeContext(NULL, &teec_ctx);

    if (rc != TEEC_SUCCESS) {
        printf("TEEC_InitializeContext failed with code 0x%x", rc);
        goto out;
    }

    /* 1: open session */
    rc =  TEEC_OpenSession(&teec_ctx, &session, &TA_EFUSE_UUID, TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);

    if (rc != TEEC_SUCCESS) {
        printf("open_session failed err %d", rc);
        goto close_context;
    }

    goto out;
close_context:
    /* 2: close context */
    TEEC_FinalizeContext(&teec_ctx);
out:
    return rc;
}


int tee_fuse_read(u32 fuse, u8 *data, size_t len)
{
    TEEC_Result ret = 0, rc = 0;
    uint32_t err_origin;
    TEEC_Operation op;

    /* 0: open context */
    /* 1: open session */
    rc =  open_ta();
    if (rc != TEEC_SUCCESS)
        goto out;

    op.started = 1;
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_OUTPUT, TEEC_VALUE_INPUT, TEEC_NONE);

    op.params[0].value.a = fuse;
    op.params[1].tmpref.size = len;
    op.params[1].tmpref.buffer = data;
    op.params[2].value.a = len;

    rc = TEEC_InvokeCommand(&session, TZCMD_EFUSE_READ, &op, &err_origin);
    if (rc) {
        printf("%s(): rc = %d\n", __func__, rc);
        ret = -1;
    }

    /* 2: close session */
    TEEC_CloseSession(&session);
    /* 3: close context */
    TEEC_FinalizeContext(&teec_ctx);
out:
    return ret;
}


int tee_fuse_write(u32 fuse, const u8 *data, size_t len,
                   const UnitSpecificWriteToken *token)
{
    TEEC_Result ret = TEEC_SUCCESS, rc = TEEC_SUCCESS;
    uint32_t err_origin;
    TEEC_Operation op;
    UnitSpecificWriteToken new_token;

    /* 0: open context */
    /* 1: open session */
    rc =  open_ta();
    if (rc != TEEC_SUCCESS)
        goto out;

    op.started = 1;
    op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_MEMREF_TEMP_INPUT, TEEC_NONE);

    op.params[0].value.a = fuse;
    op.params[1].tmpref.size = len;
    op.params[1].tmpref.buffer = (void *)data;
    op.params[2].tmpref.size = sizeof(UnitSpecificWriteToken);
    op.params[2].tmpref.buffer = (void *)(token?token:&new_token);

    rc = TEEC_InvokeCommand(&session, TZCMD_EFUSE_WRITE, &op, &err_origin);
    if (rc) {
        printf("%s(): rc = %d\n", __func__, rc);
        ret = -1;
    }

    /* 2: close session */
    TEEC_CloseSession(&session);
    /* 3: close context */
    TEEC_FinalizeContext(&teec_ctx);
out:
    return ret;
}

int tee_fuse_write_start()
{
    return TEEC_SUCCESS;
}

int tee_fuse_write_end()
{
    return TEEC_SUCCESS;
}

int tee_fuse_no_more_sensitive_writes()
{
    return TEEC_SUCCESS;
}

const char *tee_fuse_error(int result)
{
    if (result == 0)
        return efuse_err_strings[0];

    if ((-result) >= efuse_err_string_num) {
        printf("tee result = 0x%x\n", result);
        return "tee error";
    }

    return efuse_err_strings[(-result)];
}
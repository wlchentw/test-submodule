/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2011
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

#ifndef TRUSTZONE_H
#define TRUSTZONE_H

#include "typedefs.h"

#define BL31        0x43001000UL
#define BL33        0x43a00000UL
#define BL31_BASE   0x43000000UL
#define BL31_SIZE   0x00030000UL  /* default is 192K Bytes */

#define ATF_BOOT_ARG_ADDR (0x40000000)
#define TEE_BOOT_ARG_ADDR (0x40001000)
#define ATF_BOOTCFG_MAGIC (0x4D415446) // String MATF in little-endian

#define DEVINFO_SIZE 4

/* bootarg for ATF */
typedef struct {
    u64 bootarg_loc;
    u64 bootarg_size;
    u64 bl33_start_addr;
    u64 tee_info_addr;
} mtk_bl_param_t;

typedef struct {
    u32 atf_magic;
    u32 tee_support;
    u32 tee_entry;
    u32 tee_boot_arg_addr;
    u32 hwuid[4];     // HW Unique id for t-base used
    u32 atf_hrid_size;
    u32 HRID[8];      // HW random id for t-base used
    u32 atf_log_port;
    u32 atf_log_baudrate;
    u32 atf_log_buf_start;
    u32 atf_log_buf_size;
    u32 atf_irq_num;
    u32 devinfo[DEVINFO_SIZE];
    u32 atf_aee_debug_buf_start;
    u32 atf_aee_debug_buf_size;
#if CFG_TEE_SUPPORT
    u32 tee_rpmb_size;
#endif
} atf_arg_t, *atf_arg_t_ptr;

#endif /* TRUSTZONE_H */


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
 * MediaTek Inc. (C) 2016. All rights reserved.
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
 
#ifndef DRAM_BUFFER_H 
#define DRAM_BUFFER_H

#include "partition.h"

#define BMT_BUFFER_SIZE     0x10000
#define PART_HDR_BUF_SIZE 512
#define GPT_BUFFER_SIZE    (0x4000)
#define STORAGE_BUFFER_SIZE 0x10000
#define IMG_HDR_BUF_SIZE 512
#define LOG_BUFFER_MAX_SIZE             (0x10000)
#define DRAM_SEC_SECRO_BUFFER_LENGTH     (0x3000)
#define DRAM_SEC_WORKING_BUFFER_LENGTH   0x2000  
#define DRAM_SEC_UTIL_BUFFER_LENGTH      0x1000   
#define DRAM_SEC_LIB_HEAP_LENGTH         0x4000   
#define DRAM_SEC_IMG_BUFFER_LENGTH       0x3000    
#define DRAM_SEC_CHUNK_BUFFER_LENGTH     0x100000 
#define CFG_DRAM_ADDR                   (0x00240000)
#define MAX_MAIN_SIZE                (0x1000)
#define MAX_SPAR_SIZE                (0x80)
#define BMT_DAT_BUFFER_SIZE         (MAX_MAIN_SIZE + MAX_SPAR_SIZE) 
#define PMT_DAT_BUFFER_SIZE         (MAX_MAIN_SIZE + MAX_SPAR_SIZE)
#define PMT_READ_BUFFER_SIZE        (MAX_MAIN_SIZE)
#define NAND_NFI_BUFFER_SIZE          0x1000
#define PART_MAX_NUM   20

#if CFG_BYPASS_EMI
typedef struct {
	u8 bmt_buf[0x1000];
	u8 bmt_dat_buf[BMT_DAT_BUFFER_SIZE];
	u8 nand_nfi_buf[0x1000];
	part_hdr_t part_hdr_buf[1];
	u32 crc32_table[256];
	u8 pgpt_header_buf[512];
	u8 sgpt_header_buf[512];
	u8 pgpt_entries_buf[GPT_BUFFER_SIZE];
	u8 sgpt_entries_buf[GPT_BUFFER_SIZE];
	unsigned char storage_buffer[16];
	u8 img_hdr_buf[IMG_HDR_BUF_SIZE];
	unsigned int part_num;
	part_hdr_t   part_info[2];
	part_t  partition_info[2];
#ifdef MTK_EMMC_SUPPORT
	struct part_meta_info meta_info[1];
#endif
	u32 bootarg;
	u8 log_dram_buf[0x1000];
	u8  sec_secro_buf[DRAM_SEC_SECRO_BUFFER_LENGTH];
	u8  sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];/*This dram Buffer not used for security concern*/
	u8  sec_util_buf[DRAM_SEC_UTIL_BUFFER_LENGTH];
	u8  sec_lib_heap_buf[DRAM_SEC_LIB_HEAP_LENGTH];
	u8  sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];        /*This dram Buffer not used for security concern*/
	u8  sec_chunk_buf[0x4000];
	u32 *boottag; 
} dram_buf_t;
#else
typedef struct {
	/*bmt.c*/
	u8 bmt_buf[BMT_BUFFER_SIZE];
	u8 bmt_dat_buf[BMT_DAT_BUFFER_SIZE];
	/*nand.c*/
	u8 nand_nfi_buf[NAND_NFI_BUFFER_SIZE];
	
	/*download.c*/
	part_hdr_t part_hdr_buf[PART_HDR_BUF_SIZE];  
	/*efi.c*/
	u32 crc32_table[256];
	u8 pgpt_header_buf[512];
	u8 sgpt_header_buf[512];
	u8 pgpt_entries_buf[GPT_BUFFER_SIZE];
	u8 sgpt_entries_buf[GPT_BUFFER_SIZE];
	/*mmc_common_inter.c*/
	unsigned char storage_buffer[STORAGE_BUFFER_SIZE];
	/*partition.c*/
	u8 img_hdr_buf[IMG_HDR_BUF_SIZE];
	unsigned int part_num;
	part_hdr_t   part_info[PART_MAX_NUM];
	part_t  partition_info[128];
	
#ifdef MTK_EMMC_SUPPORT
	struct part_meta_info meta_info[128];
#endif
	u32 bootarg;
	u8 log_dram_buf[LOG_BUFFER_MAX_SIZE];
	u8  sec_secro_buf[DRAM_SEC_SECRO_BUFFER_LENGTH];
	u8  sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];/*This dram Buffer not used for security concern*/
	u8  sec_util_buf[DRAM_SEC_UTIL_BUFFER_LENGTH];
	u8  sec_lib_heap_buf[DRAM_SEC_LIB_HEAP_LENGTH];
	u8  sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];        /*This dram Buffer not used for security concern*/
	u8  sec_chunk_buf[DRAM_SEC_CHUNK_BUFFER_LENGTH]; 
	u32 *boottag;
} dram_buf_t;
#endif

typedef struct {
	u8 sram_sec_working_buf[DRAM_SEC_WORKING_BUFFER_LENGTH];
	u8 sram_sec_img_buf[DRAM_SEC_IMG_BUFFER_LENGTH];
} sec_buf_t;

void init_dram_buffer();
u64 platform_memory_size(void);
dram_buf_t *g_dram_buf;
sec_buf_t  g_sec_buf;

#endif /*DRAM_BUFFER_H*/

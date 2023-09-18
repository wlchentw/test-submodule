/* Copyright Statement:
*
* This software/firmware and related documentation ("MediaTek Software") are
* protected under relevant copyright laws. The information contained herein
* is confidential and proprietary to MediaTek Inc. and/or its licensors.
* Without the prior written permission of MediaTek inc. and/or its licensors,
* any reproduction, modification, use or disclosure of MediaTek Software,
* and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2015. All rights reserved.
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
*/
#include <sys/types.h>
#include <string.h>
#include <lib/bio.h>
#include <platform/mmc_rpmb.h>
#include <dl_commands.h>
/* VERIFIED BOOT 2 */
#include "libavb_ab/libavb_ab.h"
#include "libavb/libavb.h"

static AvbIOResult mt_read_from_partition(AvbOps* ops,
                                   const char* partition,
                                   int64_t offset,
                                   size_t num_bytes,
                                   void* buffer,
                                   size_t* out_num_read)
{
    bdev_t *bdev;
    size_t part_size;
    size_t read_bytes;

    bdev = bio_open_by_label(partition);
    if (!bdev) {
        dprintf(CRITICAL, "Partition [%s] is not exist.\n", partition);
        return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
    }

    part_size = bdev->total_size;

    if (offset < 0) {
        if (-offset > part_size)
        {
            bio_close(bdev);
            return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;
        }

        offset += part_size;
    }

    if (offset+num_bytes > part_size)
    {
        bio_close(bdev);
        return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;
    }

    read_bytes = bio_read(bdev, buffer, offset, num_bytes);

    if (out_num_read != NULL)
        *out_num_read = read_bytes;

    bio_close(bdev);
    return AVB_IO_RESULT_OK;
}

static AvbIOResult mt_write_to_partition(AvbOps* ops,
                                  const char* partition,
                                  int64_t offset,
                                  size_t num_bytes,
                                  const void* buffer)
{
    bdev_t *bdev;
    size_t part_size;
    size_t write_bytes;

    bdev = bio_open_by_label(partition);
    if (!bdev) {
        dprintf(CRITICAL, "Partition [%s] is not exist.\n", partition);
        return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
    }

    part_size = bdev->total_size;

    if (offset < 0) {
        if (-offset > part_size)
        {
            bio_close(bdev);
            return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;
        }

        offset += part_size;
    }

    if (offset+num_bytes > part_size)
    {
        bio_close(bdev);
        return AVB_IO_RESULT_ERROR_RANGE_OUTSIDE_PARTITION;
    }

    write_bytes = bio_write(bdev, buffer,offset, num_bytes);

    if (write_bytes != num_bytes)
    {
        bio_close(bdev);
        return AVB_IO_RESULT_ERROR_IO;
    }

    bio_close(bdev);
    return AVB_IO_RESULT_OK;
}

#ifdef AVB_ENABLE_ANTIROLLBACK
typedef struct {
    uint64_t rollback_indexes[AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS];
} rollback_indexes_t;
#define VERIDX_IN_PRMB 0
/*antirollback index is stored in rpmb region block 1*/
static AvbIOResult mt_read_rollback_index(AvbOps* ops,
                                   size_t rollback_index_location,
                                   uint64_t* out_rollback_index)
{
    int ret=0;
    unsigned char blk[256] = {0};
    ret = mmc_rpmb_block_read(VERIDX_IN_PRMB,blk);
    if(ret != 0 )
    {
        *out_rollback_index = 0;
        return AVB_IO_RESULT_OK;
    }
    rollback_indexes_t* indexes = (rollback_indexes_t*)blk;
    *out_rollback_index=indexes->rollback_indexes[rollback_index_location];
#if 0
    dprintf(CRITICAL, "indexes read: %08llx %08llx %08llx %08llx\n",
            indexes->rollback_indexes[0], indexes->rollback_indexes[1], 
            indexes->rollback_indexes[2], indexes->rollback_indexes[3]);
#endif
    return AVB_IO_RESULT_OK;
}

static AvbIOResult mt_write_rollback_index(AvbOps* ops,
                                    size_t rollback_index_location,
                                    uint64_t rollback_index)
{
    int ret=0;
    unsigned char blk[256] = {0};
    ret = mmc_rpmb_block_read(VERIDX_IN_PRMB,blk);
    if(ret != 0 )
    {
        return AVB_IO_RESULT_ERROR_IO;
    }
    rollback_indexes_t* indexes = (rollback_indexes_t*)blk;
    indexes->rollback_indexes[rollback_index_location] = rollback_index;
#if 0
    dprintf(CRITICAL, "indexes write: %08llx %08llx %08llx %08llx\n",
            indexes->rollback_indexes[0], indexes->rollback_indexes[1], 
            indexes->rollback_indexes[2], indexes->rollback_indexes[3]);
#endif
    ret = mmc_rpmb_block_write(VERIDX_IN_PRMB,(unsigned char*)indexes);
    if(ret != 0 )
    {
        return AVB_IO_RESULT_ERROR_IO;
    }
    return AVB_IO_RESULT_OK;
}
#else
static AvbIOResult mt_read_rollback_index(AvbOps* ops,
                                   size_t rollback_index_location,
                                   uint64_t* out_rollback_index)
{
    *out_rollback_index = 0;
    return AVB_IO_RESULT_OK;
}

static AvbIOResult mt_write_rollback_index(AvbOps* ops,
                                    size_t rollback_index_location,
                                    uint64_t rollback_index)
{
    return AVB_IO_RESULT_OK;
}
#endif

#define INDEX_RPMB_UNLOCK 1
#define AVB_DEVICE_UNLOCK 0x5a
static AvbIOResult mt_read_is_device_unlocked(AvbOps* ops, bool* out_is_unlocked)
{
    if (out_is_unlocked != NULL) {
        *out_is_unlocked = false;
    }

#if defined(AVB_ENABLE_ANTIROLLBACK) || defined(AVB_ENABLE_DEVICE_STATE_CHANGE)
    unsigned char blk[256]={0};
    int ret = -1;

    ret=mmc_rpmb_block_read(INDEX_RPMB_UNLOCK,&blk[0]);
    if(ret != 0){
        dprintf(CRITICAL, "mmc_rpmb_block_read fail %d.\n", ret);
        return AVB_IO_RESULT_ERROR_IO;
    }

    if(blk[0] == AVB_DEVICE_UNLOCK)
        *out_is_unlocked = true;
#endif

    dprintf(CRITICAL, "mt_read_is_device_unlocked return: %d.\n", *out_is_unlocked);

    return AVB_IO_RESULT_OK;
}

static AvbIOResult mt_get_unique_guid_for_partition(AvbOps* ops,
                                             const char* partition,
                                             char* guid_buf,
                                             size_t guid_buf_size)
{
    bdev_t *bdev;

    bdev = bio_open_by_label(partition);
    if (!bdev) {
        dprintf(CRITICAL, "Partition [%s] is not exist.\n", partition);
        return AVB_IO_RESULT_ERROR_NO_SUCH_PARTITION;
    }

    strlcpy(guid_buf, bdev->unique_uuid, guid_buf_size);

    bio_close(bdev);
    return AVB_IO_RESULT_OK;
}

#include <libfdt.h>
#include <image.h>
extern const unsigned char blob[];
static const unsigned char* get_pubkey_in_blob(void)
{
    int sig_node;
    int noffset;
    const void *sig_blob=&blob[0];
    sig_node = fdt_subnode_offset(sig_blob, 0, FDT_SIG_NODE);
    for (noffset = fdt_first_subnode(sig_blob, sig_node);
        noffset >= 0;
        noffset = fdt_next_subnode(sig_blob, noffset))
    {
        return (const unsigned char*)fdt_getprop(sig_blob, noffset, BLOB_MOD_NODE, NULL);
    }
    return NULL;
}

static AvbIOResult mt_validate_vbmeta_public_key(AvbOps* ops,
                                            const uint8_t* public_key_data,
                                            size_t public_key_length,
                                            const uint8_t* public_key_metadata,
                                            size_t public_key_metadata_length,
                                            bool* out_is_trusted)
{
    unsigned char* pubkey_in_blob;
    unsigned char* pubkey_in_footer;
    AvbRSAPublicKeyHeader* header;

    header = (AvbRSAPublicKeyHeader*)public_key_data;
    pubkey_in_blob = (unsigned char*)get_pubkey_in_blob();
    pubkey_in_footer = (unsigned char*)(public_key_data+sizeof(AvbRSAPublicKeyHeader));
    if(memcmp(pubkey_in_blob,pubkey_in_footer,avb_be32toh(header->key_num_bits)/32)==0)
    {
        *out_is_trusted = true;
    }
    else
    {
        *out_is_trusted = false;
    }

    return AVB_IO_RESULT_OK;
}

static AvbOps avbops;

static AvbABOps avbabops = {
    .ops = &avbops,

    .read_ab_metadata = avb_ab_data_read,
    .write_ab_metadata = avb_ab_data_write,
};


static AvbOps avbops = {
    .ab_ops = &avbabops,

    .read_from_partition = mt_read_from_partition,
    .write_to_partition = mt_write_to_partition,
    .validate_vbmeta_public_key = mt_validate_vbmeta_public_key,
    .read_rollback_index = mt_read_rollback_index,
    .write_rollback_index = mt_write_rollback_index,
    .read_is_device_unlocked = mt_read_is_device_unlocked,
    .get_unique_guid_for_partition = mt_get_unique_guid_for_partition,
};

bool is_device_unlocked(void)
{
    AvbIOResult io_ret;
    bool is_device_unlocked;
    io_ret = avbops.read_is_device_unlocked(&avbops, &is_device_unlocked);
    if (io_ret != AVB_IO_RESULT_OK) {
        //any error treat as locked
        return false;
    }
    return is_device_unlocked;
}

void* get_partition_data(char* part_name,AvbSlotVerifyData* verifyData)
{
    int i=0;
    for(;i<verifyData->num_loaded_partitions;i++)
    {
        if(avb_strcmp(part_name,verifyData->loaded_partitions[i].partition_name)==0)
        {
            return verifyData->loaded_partitions[i].data;
        }
    }
    return NULL;
}

AvbSlotVerifyResult avb_update_rollback_indexes(AvbOps* ops,AvbSlotVerifyData* verifyData)
{
    size_t n = 0;
    AvbIOResult io_ret;
    AvbSlotVerifyResult ret = AVB_SLOT_VERIFY_RESULT_OK;
    /* Update stored rollback index such that the stored rollback index
    * is the largest value supporting all currently bootable slots. Do
    * this for every rollback index location.
    */
    for (n = 0; n < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; n++) {
      uint64_t rollback_index_value = 0;

      rollback_index_value = verifyData->rollback_indexes[n];
      if (rollback_index_value != 0) {
        uint64_t current_rollback_index_value;
        io_ret = ops->read_rollback_index(ops, n, &current_rollback_index_value);
        if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
          goto out;
        } else if (io_ret != AVB_IO_RESULT_OK) {
          avb_error("Error getting rollback index for slot.\n");
          ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
          goto out;
        }
        if (current_rollback_index_value != rollback_index_value) {
          io_ret = ops->write_rollback_index(ops, n, rollback_index_value);
          if (io_ret == AVB_IO_RESULT_ERROR_OOM) {
            ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
            goto out;
          } else if (io_ret != AVB_IO_RESULT_OK) {
            avb_error("Error setting stored rollback index.\n");
            ret = AVB_SLOT_VERIFY_RESULT_ERROR_OOM;
            goto out;
          }
        }
      }
    }
out:
    return ret;
}

#ifndef AB_OTA_UPDATER
AvbSlotVerifyResult android_verified_boot_2_0(AvbSlotVerifyData** verifyData)
{
    AvbSlotVerifyResult verify_result;
    const char* requested_partitions[] = {BOOT_PART_NAME,NULL};
    verify_result = avb_slot_verify(&avbops,requested_partitions,"",is_device_unlocked(),verifyData);
    dprintf(CRITICAL, "avb boot verification result is %s\n",avb_slot_verify_result_to_string(verify_result));
    if(verify_result == AVB_SLOT_VERIFY_RESULT_OK)
    {
        verify_result = avb_update_rollback_indexes(&avbops,*verifyData);
        dprintf(CRITICAL, "avb boot rollback indexes result is %s\n",avb_slot_verify_result_to_string(verify_result));
    }
    return verify_result;
}
#else
//static AvbSlotVerifyData *slot_data;
AvbSlotVerifyResult android_verified_boot_2_0(AvbSlotVerifyData** verifyData)
{
    AvbABFlowResult ab_result;
    const char* requested_partitions[] = {"bootimg", NULL};
    /* ab flow */
    ab_result = avb_ab_flow(&avbabops, requested_partitions, false, verifyData);
    dprintf(CRITICAL, "ab_result: %s\n", avb_ab_flow_result_to_string(ab_result));

    return ab_result;
}
#endif

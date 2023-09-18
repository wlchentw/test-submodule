/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
*/
/* MediaTek Inc. (C) 2016. All rights reserved.
 * Author: Rick Chang <rick.chang@mediatek.com>
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

#ifndef _UT_V4L2_H_
#define _UT_V4L2_H_

#include <stdint.h>
#include "videodev2.h"

#define MAX_PATH_SIZE 128
#define MAX_NAME_SIZE 128
#define TIMEOUT 5000

#define OUT V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
#define CAP V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define ALIGN(a, b) (((a) + (b) - 1) & (~((b) - 1)))
#define DIV_ALIGN(a, b) (((a) + (b) - 1) / (b))
#define DMA_ALIGN 64

struct ut_ctx;

enum ut_ret_type {
	UT_RET_OK = 0,
	UT_RET_FAIL = -1,
	UT_RET_NO_FILE = -2,
	UT_RET_NO_FREE_BUF = -3,
	UT_RET_EOS = -4,
	UT_RET_RES_CHANGE = -5,
	UT_RET_SRC_NOT_SUPPORT = -6,
	UT_RET_WAIT_SEQ_HEADER = -7,
};

enum Ut_Chip_Name_Type {
	UT_CHIP_NAME_MT2701 = 0,
	UT_CHIP_NAME_MT2712,
};

struct ut_buf {
	int idx;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	int n_plane;
	unsigned char *va[VIDEO_MAX_PLANES];
	struct ut_buf *next_free;
};

enum Ut_Chip_Name_Type ut_chip_name;

struct ut_v4l2_fmt {
	unsigned int	pixelformat;
	unsigned int	num_planes;
	unsigned int	depth[VIDEO_MAX_PLANES];
	unsigned int	row_depth[VIDEO_MAX_PLANES];
};

typedef int (*ut_buf_handle) (void *user_data, struct ut_buf *buf);
typedef int (*ut_hash_handle) (void *user_data, struct ut_buf *buf);

struct ut_buf_ctx {
	struct v4l2_format fmt;
	struct v4l2_crop crop;
	struct v4l2_streamparm sparm;
	struct v4l2_selection sel;
	struct v4l2_fmtdesc *fmtdesc;
	int fmtdesc_num;

	enum v4l2_memory mem_type;

	struct ut_buf *buf_list;
	struct ut_buf *free_list;
	int buf_num;

	ut_buf_handle buf_handle;
	void *user_data;

	const struct ut_v4l2_fmt *ut_fmt;
	struct v4l2_plane *planes;
	int n_plane;
};

struct ut_ctx {
	int dev_fd;
	char dev_path[MAX_PATH_SIZE];
	char *dev_name;

	pthread_t thread;
	bool start_thread;

	char in_file_path[MAX_PATH_SIZE];
	char out_file_path[MAX_PATH_SIZE];
	bool out_file;

	bool check_hash;
	char check_hash_path[MAX_PATH_SIZE];
	void *check_udata;
	ut_hash_handle check_handle;

	bool gen_hash;
	char gen_hash_path[MAX_PATH_SIZE];
	void *gen_udata;
	ut_hash_handle gen_handle;

	int in_data_num;
	int out_data_num;
	int attr;
	int fps;
	long long time_start;
	long long time_end;
	long long start_play_time;
	long long times_minus;
	int cur_tid;

	struct v4l2_control ctrl;
	struct v4l2_ext_controls ext_controls;
	struct v4l2_capability cap;

	struct ut_buf_ctx out_ctx;
	struct ut_buf_ctx cap_ctx;
};

/* util */
struct ut_ctx *ut_create_ctx(void);
void ut_destroy_ctx(struct ut_ctx *ctx);
/*
int ut_start_thread(struct ut_ctx *ctx, void *(*task) (void *), void *user_data);
long ut_stop_thread(struct ut_ctx *ctx);
*/
/* set */
int ut_set_buf_num(struct ut_ctx *ctx, int num, __u32 buf_type);
int ut_set_mem_type(struct ut_ctx *ctx, enum v4l2_memory mem_type, __u32 buf_type);
int ut_set_dev_path(struct ut_ctx *ctx, const char *path);
int ut_set_dev_name(struct ut_ctx *ctx, char *name);
int ut_set_in_file_path(struct ut_ctx *ctx, const char *path);
int ut_set_out_file_path(struct ut_ctx *ctx, const char *path);
int ut_set_in_md5_path(struct ut_ctx *ctx, const char *path);
int ut_set_out_md5_path(struct ut_ctx *ctx, const char *path);
int ut_set_debug_level(struct ut_ctx *ctx, int level);
int ut_set_buf_handle(struct ut_ctx *ctx, void *user_data, ut_buf_handle handle,
		__u32 buf_type);
int ut_set_check_hash_handle(struct ut_ctx *ctx, void *user_data, ut_buf_handle handle);
int ut_set_gen_hash_handle(struct ut_ctx *ctx, void *user_data, ut_buf_handle handle);

/* get */
int ut_get_out_data_num(struct ut_ctx *ctx);
int ut_get_in_data_num(struct ut_ctx *ctx);
int ut_get_buf_num(struct ut_ctx *ctx, __u32 buf_type);
int ut_get_fmtdesc_num(struct ut_ctx *ctx, __u32 buf_type);
int ut_get_debug_level(struct ut_ctx *ctx);
const char *ut_get_out_md5_path(struct ut_ctx *ctx);
const char *ut_get_in_md5_path(struct ut_ctx *ctx);
const char *ut_get_out_file_path(struct ut_ctx *ctx);
const char *ut_get_in_file_path(struct ut_ctx *ctx);
const char *ut_get_dev_path(struct ut_ctx *ctx);
const char *ut_get_dev_name(struct ut_ctx *ctx);
enum v4l2_memory ut_get_mem_type(struct ut_ctx *ctx, __u32 buf_type);
struct v4l2_format *ut_get_fmt(struct ut_ctx *ctx, __u32 buf_type);
struct v4l2_crop *ut_get_crop(struct ut_ctx *ctx, __u32 buf_type);
struct v4l2_streamparm *ut_get_sparm(struct ut_ctx *ctx, __u32 buf_type);
struct v4l2_control *ut_get_ctrl(struct ut_ctx *ctx);
struct v4l2_ext_controls *ut_get_ext_ctrls(struct ut_ctx *ctx);
struct v4l2_selection *ut_get_selection(struct ut_ctx *ctx, __u32 buf_type);
struct v4l2_fmtdesc *ut_get_fmtdesc(struct ut_ctx *ctx, int idx, __u32 buf_type);

/* v4l2 */
int ut_v4l2_open_dev(struct ut_ctx *ctx);
void ut_v4l2_close_dev(struct ut_ctx *ctx);
int ut_v4l2_poll(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_enum_fmt(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_enum_check(struct ut_ctx *ctx, __u32 pixfmt, __u32 buf_type);
int ut_v4l2_set_fmt(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_get_fmt(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_try_fmt(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_set_param(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_set_crop(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_querycap(struct ut_ctx *ctx);
int ut_v4l2_set_ctrl(struct ut_ctx *ctx);
int ut_v4l2_set_ext_ctrls(struct ut_ctx *ctx);
int ut_v4l2_sub_event(struct ut_ctx *ctx);
int ut_v4l2_get_ctrl(struct ut_ctx *ctx);
int ut_v4l2_get_crop(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_get_selection(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_q_buf(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_alloc_buf(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_q_all_buf(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_dq_buf(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_dq_all_buf(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_stream_on(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_stream_off(struct ut_ctx *ctx, __u32 buf_type);
int ut_v4l2_process_cap_task(struct ut_ctx *ctx);
int ut_v4l2_process_out_task(struct ut_ctx *ctx);
void ut_v4l2_free_buf(struct ut_ctx *ctx, __u32 buf_type);
extern void ut_v4l2_init_time(struct ut_ctx *ctx);
extern int64_t get_time_ms(void);
/* debug */
struct ut_buf *ut_v4l2_get_buf(struct ut_ctx *ctx, int idx, __u32 buf_type);
void ut_get_chipname(void);

/* helper */
const unsigned int *ut_v4l2_fmt_bpp(__u32 fmt);
const unsigned int *ut_v4l2_fmt_bpl(__u32 fmt);
int ut_v4l2_fmt_planes_num(__u32 fmt);

#endif

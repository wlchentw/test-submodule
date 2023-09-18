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
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/syscall.h>



#include "ut_v4l2.h"
#include "md5.h"

enum ut_attr_type {
	UT_ATTR_Q_EMPTY = 1
};

static unsigned int debug = 0;

#define UT_LOG(level, fmt, arg...)		\
do {						\
	if (debug >= (level))			\
		printf(fmt , ## arg);		\
} while (0)

#define UT_ERR(fmt, arg...)					\
do {								\
	printf("(%s:%d)" fmt, __func__, __LINE__, ##arg);	\
} while (0)

#define UT_WARN(fmt, arg...)					\
do {								\
	printf("*** WARNING: " fmt, ##arg);	\
} while (0)

#define RET(x)					\
do {						\
	if ((x) != 0) {				\
		UT_ERR("\'%s\' fail.\n", #x);	\
		goto err;			\
	}					\
} while (0)

void ut_get_chipname(void)
{
	FILE *stream;
	char buf[1024] = { 0 };

	stream = popen("cat /proc/device-tree/model", "r");

	if (NULL == stream) {
		perror("popen fail\n");
		exit(1);
	}

	fread(buf, sizeof(buf) - 1, sizeof(char), stream);
	pclose(stream);

	if (strstr(buf, "2701")) {
		ut_chip_name = UT_CHIP_NAME_MT2701;
	} else if (strstr(buf, "2712")) {
		ut_chip_name = UT_CHIP_NAME_MT2712;
	} else {
		UT_LOG(0,"Use default chip name MT2712.\n");
		ut_chip_name = UT_CHIP_NAME_MT2712;
	}
}

int64_t get_time_ms(void)
{
	int64_t time = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);
	time = (int64_t)(tv.tv_sec * 1000LL + tv.tv_usec / 1000);
	return time;
}

static void ut_fourcc2str(__u32 fmt, char *str)
{
	char a = fmt & 0xFF;
	char b = (fmt >> 8) & 0xFF;
	char c = (fmt >> 16) & 0xFF;
	char d = (fmt >> 24) & 0xFF;

	sprintf(str, "%c%c%c%c", a, b, c, d);
}

static const struct ut_v4l2_fmt ut_v4l2_formats[] = {
	{
		.pixelformat	= V4L2_PIX_FMT_PNG,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
	}, {
		.pixelformat	= V4L2_PIX_FMT_JPEG,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
	}, {
		.pixelformat	= V4L2_PIX_FMT_GREY,
		.depth		= { 8 },
		.row_depth	= { 8 },
		.num_planes	= 1,
	}, {
		.pixelformat	= V4L2_PIX_FMT_ARGB32,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
	}, {
		.pixelformat	= V4L2_PIX_FMT_AYUV,
		.depth		= { 32 },
		.row_depth	= { 32 },
		.num_planes	= 1,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV420M,
		.depth		= { 8, 2, 2 },
		.row_depth	= { 8, 4, 4 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVU420M,
		.depth		= { 8, 2, 2 },
		.row_depth	= { 8, 4, 4 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV422M,
		.depth		= { 8, 4, 4 },
		.row_depth	= { 8, 4, 4 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVU422M,
		.depth		= { 8, 4, 4 },
		.row_depth	= { 8, 4, 4 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YUV444M,
		.depth		= { 8, 8, 8 },
		.row_depth	= { 8, 8, 8 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_YVU444M,
		.depth		= { 8, 8, 8 },
		.row_depth	= { 8, 8, 8 },
		.num_planes	= 3,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV12M,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV21M,
		.depth		= { 8, 4 },
		.row_depth	= { 8, 4 },
		.num_planes	= 2,
	}, {
		.pixelformat	= V4L2_PIX_FMT_NV16M,
		.depth		= { 8, 8 },
		.row_depth	= { 8, 8 },
		.num_planes	= 2,
	}, {
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.depth		= { 16 },
		.row_depth	= { 16 },
		.num_planes	= 1,
	}
};

const unsigned int *ut_v4l2_fmt_bpp(__u32 fmt)
{
	int i = 0;
	char name[5] = {0};

	for (i = 0; i < ARRAY_SIZE(ut_v4l2_formats); i++) {
		if (ut_v4l2_formats[i].pixelformat == fmt)
			return ut_v4l2_formats[i].depth;
	}

	ut_fourcc2str(fmt, name);
	UT_LOG(0, "%s - unsupported fmt:%s\n", __func__, name);
	return NULL;
}

const unsigned int *ut_v4l2_fmt_bpl(__u32 fmt)
{
	int i = 0;
	char name[5] = {0};

	for (i = 0; i < ARRAY_SIZE(ut_v4l2_formats); i++) {
		if (ut_v4l2_formats[i].pixelformat == fmt)
			return ut_v4l2_formats[i].row_depth;
	}

	ut_fourcc2str(fmt, name);
	UT_LOG(0, "%s - unsupported fmt:%s\n", __func__, name);
	return NULL;
}

int ut_v4l2_fmt_planes_num(__u32 fmt)
{
	int i = 0;
	char name[5] = {0};

	for (i = 0; i < ARRAY_SIZE(ut_v4l2_formats); i++) {
		if (ut_v4l2_formats[i].pixelformat == fmt)
			return ut_v4l2_formats[i].num_planes;
	}

	ut_fourcc2str(fmt, name);
	UT_LOG(0, "%s - unsupported fmt:%s\n", __func__, name);
	return NULL;
}

static void ut_show_buf_planes(struct ut_buf *buf)
{
	struct v4l2_plane *planes = buf->planes;
	int i;

	for (i = 0; i < buf->n_plane; i++) {
		UT_LOG(3, "Buf%d-%d length %d userptr:0x%lx offset 0x%x used %d\n",
		       buf->idx, i, (int)planes[i].length, planes[i].m.userptr,
		       (int)planes[i].m.mem_offset, (int)planes[i].bytesused);
	}
}

static void ut_show_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_format *fmt = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.fmt : &ctx->cap_ctx.fmt;
	char name[128];

	ut_fourcc2str(fmt->fmt.pix_mp.pixelformat, name);
	UT_LOG(1, "(%d) wxh: %dx%d, format: %s\n",
	       buf_type, fmt->fmt.pix_mp.width, fmt->fmt.pix_mp.height, name);
}

struct ut_ctx *ut_create_ctx(void)
{
	struct ut_ctx *ctx;
	ctx = calloc(1, sizeof(*ctx));
	if (!ctx) {
		UT_ERR("Out of memory.\n");
		return NULL;
	}
	ctx->dev_fd = -1;
	return ctx;
}

void ut_destroy_ctx(struct ut_ctx *ctx)
{
	if (ctx) {
		if (ctx->out_ctx.fmtdesc)
			free(ctx->out_ctx.fmtdesc);
		if (ctx->cap_ctx.fmtdesc)
			free(ctx->cap_ctx.fmtdesc);
		free(ctx);
	}
}

int ut_get_in_data_num(struct ut_ctx *ctx)
{
	return ctx->in_data_num;
}

int ut_get_out_data_num(struct ut_ctx *ctx)
{
	return ctx->out_data_num;
}

int ut_set_buf_num(struct ut_ctx *ctx, int num, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	buf_ctx->buf_num = num;
	return 0;
}

int ut_get_buf_num(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	return buf_ctx->buf_num;
}

int ut_get_fmtdesc_num(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	return buf_ctx->fmtdesc_num;
}

int ut_set_mem_type(struct ut_ctx *ctx, enum v4l2_memory mem_type, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	buf_ctx->mem_type = mem_type;
	return 0;
}

enum v4l2_memory ut_get_mem_type(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	return buf_ctx->mem_type;
}

int ut_set_dev_path(struct ut_ctx *ctx, const char *path)
{
	strncpy(ctx->dev_path, path, MAX_PATH_SIZE);
	return 0;
}

int ut_set_dev_name(struct ut_ctx *ctx, char *name)
{
	ctx->dev_name = name;
	return 0;
}

const char *ut_get_dev_path(struct ut_ctx *ctx)
{
	return ctx->dev_path;
}

const char *ut_get_dev_name(struct ut_ctx *ctx)
{
	return ctx->dev_name;
}

int ut_set_in_file_path(struct ut_ctx *ctx, const char *path)
{
	strncpy(ctx->in_file_path, path, MAX_PATH_SIZE);
	return 0;
}

const char *ut_get_in_file_path(struct ut_ctx *ctx)
{
	return ctx->in_file_path;
}

int ut_set_out_file_path(struct ut_ctx *ctx, const char *path)
{
	strncpy(ctx->out_file_path, path, MAX_PATH_SIZE);
	ctx->out_file = true;
	return 0;
}

const char *ut_get_out_file_path(struct ut_ctx *ctx)
{
	return ctx->out_file_path;
}

int ut_set_in_md5_path(struct ut_ctx *ctx, const char *path)
{
	strncpy(ctx->check_hash_path, path, MAX_PATH_SIZE);
	ctx->check_hash = true;
	return 0;
}

const char *ut_get_in_md5_path(struct ut_ctx *ctx)
{
	return ctx->check_hash_path;
}

int ut_set_out_md5_path(struct ut_ctx *ctx, const char *path)
{
	strncpy(ctx->gen_hash_path, path, MAX_PATH_SIZE);
	ctx->gen_hash = true;
	return 0;
}

const char *ut_get_out_md5_path(struct ut_ctx *ctx)
{
	return ctx->gen_hash_path;
}

int ut_set_debug_level(struct ut_ctx *ctx, int level)
{
	debug = level;
	return 0;
}

int ut_get_debug_level(struct ut_ctx *ctx)
{
	return debug;
}

int ut_set_buf_handle(struct ut_ctx *ctx, void *user_data, ut_buf_handle handle,
		__u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;

	buf_ctx->user_data = user_data;
	buf_ctx->buf_handle = handle;
	return 0;
}

int ut_set_check_hash_handle(struct ut_ctx *ctx, void *user_data, ut_hash_handle handle)
{
	ctx->check_udata = user_data;
	ctx->check_handle = handle;
	return 0;
}

int ut_set_gen_hash_handle(struct ut_ctx *ctx, void *user_data, ut_hash_handle handle)
{
	ctx->gen_udata = user_data;
	ctx->gen_handle = handle;
	return 0;
}

static int ut_ioctl(struct ut_ctx *ctx, unsigned long req, void *data)
{
	int ret;

	ret = ioctl(ctx->dev_fd, req, data);
	if (ret == -1) {
		if (errno == EAGAIN)
			return -errno;
		UT_ERR("ioctl fail. (%d)(%s)\n", errno, strerror(errno));
		return -1;
	}
	return 0;
}

static int ut_check_src_change(struct ut_ctx *ctx)
{
	struct v4l2_event e = { 0 };
	__u32 change;
	enum ut_ret_type ret = UT_RET_OK;
	RET(ut_ioctl(ctx, VIDIOC_DQEVENT, &e));

	switch (e.type) {
		case V4L2_EVENT_SOURCE_CHANGE:
			change = e.u.src_change.changes;
			if (change & V4L2_EVENT_SRC_CH_RESOLUTION) {
				UT_LOG(1, "Got resolution change.\n");
				ret = UT_RET_RES_CHANGE;
			}
			break;
		case V4L2_EVENT_EOS:
			UT_LOG(0, "Vdec not support the source, stop playing it.\n");
			ret = UT_RET_SRC_NOT_SUPPORT;
			break;
		default:
			UT_ERR("Unknown event type: %d.\n", e.type);
			goto err;
	}

	return ret;
err:
	return -1;
}

static void ut_show_revent(short revent, __u32 buf_type)
{
	switch (revent) {
	case POLLIN:
		UT_LOG(3, "(%d)POLLIN\n", buf_type);
		break;
	case POLLOUT:
		UT_LOG(3, "(%d)POLLOUT\n", buf_type);
		break;
	case POLLERR:
		UT_LOG(3, "(%d)POLLERR\n", buf_type);
		break;
	case POLLPRI:
		UT_LOG(3, "(%d)POLLPRI\n", buf_type);
		break;
	default:
		UT_LOG(3, "(%d)UNKNOWN\n", buf_type);
		break;
	}
}

int ut_v4l2_poll(struct ut_ctx *ctx, __u32 buf_type)
{
	struct pollfd pfd;
	short wait_event = V4L2_TYPE_IS_OUTPUT(buf_type) ? POLLOUT : POLLIN;
	int ret;

	pfd.fd = ctx->dev_fd;
	pfd.events = POLLERR;
	pfd.events |= wait_event;

	if (!V4L2_TYPE_IS_OUTPUT(buf_type))
		pfd.events |= POLLPRI;

again:
	ret = poll(&pfd, 1, TIMEOUT);
	if (ret == -1) {
		UT_ERR("poll fail. (%s).\n", strerror(errno));
		return -1;
	}
	if (ret == 0) {
		UT_ERR("poll fail. (TIMEOUT).\n");
		return -1;
	}
	ut_show_revent(pfd.revents, buf_type);
	if (pfd.revents & POLLERR)
		return -1;

	if (pfd.revents & POLLPRI) {
		ret = ut_check_src_change(ctx);
		if (ret != 0)
			return ret;
	}

	if ((pfd.revents & wait_event) == wait_event)
		return 0;
	goto again;
}

struct v4l2_format *ut_get_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	return V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.fmt : &ctx->cap_ctx.fmt;
}

struct v4l2_crop *ut_get_crop(struct ut_ctx *ctx, __u32 buf_type)
{
	return V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.crop : &ctx->cap_ctx.crop;
}

struct v4l2_selection *ut_get_selection(struct ut_ctx *ctx, __u32 buf_type)
{
	return V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.sel : &ctx->cap_ctx.sel;
}

struct v4l2_fmtdesc *ut_get_fmtdesc(struct ut_ctx *ctx, int idx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx : &ctx->cap_ctx;

	if (idx >= buf_ctx->fmtdesc_num)
		return NULL;
	return &buf_ctx->fmtdesc[idx];
}

struct v4l2_streamparm *ut_get_sparm(struct ut_ctx *ctx, __u32 buf_type)
{
	return V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.sparm : &ctx->cap_ctx.sparm;
}

int ut_v4l2_enum_check(struct ut_ctx *ctx, __u32 pixfmt, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type) ?
		&ctx->out_ctx : &ctx->cap_ctx;
	char name[128];
	int i;

	if (!buf_ctx->fmtdesc)
		RET(ut_v4l2_enum_fmt(ctx, buf_type));

	for (i = 0; i < buf_ctx->fmtdesc_num; i++) {
		struct v4l2_fmtdesc *f = &buf_ctx->fmtdesc[i];

		if (f->pixelformat == pixfmt)
			return 0;
	}

	ut_fourcc2str(pixfmt, name);
	UT_WARN("(%d) %s not support!\n", buf_type, name);
err:
	return -1;
}

int ut_v4l2_enum_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type) ?
		&ctx->out_ctx : &ctx->cap_ctx;
	int i;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	if (buf_ctx->fmtdesc) {
		free(buf_ctx->fmtdesc);
		buf_ctx->fmtdesc = NULL;
	}

	for (i = 0;; i++) {
		struct v4l2_fmtdesc fmt;
		struct v4l2_fmtdesc *new;
		char name[128];

		fmt.index = i;
		fmt.type = buf_type;
		if (ut_ioctl(ctx, VIDIOC_ENUM_FMT, &fmt))
			break;

		UT_LOG(2, "index: %u, ", fmt.index);
		UT_LOG(2, "type: %d, ", fmt.type);
		ut_fourcc2str(fmt.pixelformat, name);
		UT_LOG(2, "pixelformat: %s\n", name);

		new = realloc(buf_ctx->fmtdesc, (i + 1) * sizeof(fmt));
		RET(new == NULL);
		new[i] = fmt;
		buf_ctx->fmtdesc = new;
	}
	buf_ctx->fmtdesc_num = i;

	return 0;
err:
	return -1;
}

int ut_v4l2_set_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_format *fmt = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.fmt : &ctx->cap_ctx.fmt;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	ut_show_fmt(ctx, buf_type);

	fmt->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_S_FMT, fmt));

	return 0;
err:
	return -1;
}

int ut_v4l2_get_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_format *fmt = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.fmt : &ctx->cap_ctx.fmt;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	fmt->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_G_FMT, fmt));

	ut_show_fmt(ctx, buf_type);

	return 0;
err:
	return -1;
}

int ut_v4l2_try_fmt(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_format *fmt = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.fmt : &ctx->cap_ctx.fmt;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	fmt->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_TRY_FMT, fmt));

	ut_show_fmt(ctx, buf_type);

	return 0;
err:
	return -1;
}

int ut_v4l2_set_param(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_streamparm *sparm = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.sparm : &ctx->cap_ctx.sparm;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	sparm->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_S_PARM, sparm));

	return 0;
err:
	return -1;
}

int ut_v4l2_set_crop(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_crop *crop = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.crop : &ctx->cap_ctx.crop;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	crop->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_S_CROP, crop));

	return 0;
err:
	return -1;
}

int ut_v4l2_set_selection(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_selection *sel = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.sel : &ctx->cap_ctx.sel;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	sel->type = V4L2_TYPE_IS_OUTPUT(buf_type) ? V4L2_BUF_TYPE_VIDEO_OUTPUT
						 : V4L2_BUF_TYPE_VIDEO_CAPTURE;
	RET(ut_ioctl(ctx, VIDIOC_S_SELECTION, sel));

	return 0;
err:
	return -1;
}

int ut_v4l2_sub_event(struct ut_ctx *ctx)
{
	struct v4l2_event_subscription sub = { 0 };

	UT_LOG(2, "%s\n", __func__);

	sub.type = V4L2_EVENT_SOURCE_CHANGE;
	RET(ut_ioctl(ctx, VIDIOC_SUBSCRIBE_EVENT, &sub));

	sub.type = V4L2_EVENT_EOS;
	RET(ut_ioctl(ctx, VIDIOC_SUBSCRIBE_EVENT, &sub));

	return 0;
err:
	return -1;
}

int ut_v4l2_querycap(struct ut_ctx *ctx)
{
	struct v4l2_capability *cap = &ctx->cap;

	UT_LOG(2, "-> %s()\n", __func__);

	memset(cap, 0, sizeof(*cap));

	RET(ut_ioctl(ctx, VIDIOC_QUERYCAP, cap));

	UT_LOG(2, "\tdev name   : %s\n", ctx->dev_name);
	UT_LOG(2, "Driver Info:\n");
	UT_LOG(2, "\tDriver name   : %s\n", cap->driver);
	UT_LOG(2, "\tCard type     : %s\n", cap->card);
	UT_LOG(2, "\tBus info      : %s\n", cap->bus_info);
	UT_LOG(2, "\tDriver version: %d.%d.%d\n",
			cap->version >> 16,
			(cap->version >> 8) & 0xff,
			cap->version & 0xff);
	UT_LOG(2, "\tCapabilities  : 0x%08X\n", cap->capabilities);

	return 0;
err:
	return -1;
}

int ut_v4l2_set_ctrl(struct ut_ctx *ctx)
{
	struct v4l2_control *ctrl = &ctx->ctrl;

	UT_LOG(2, "-> %s()\n", __func__);

	RET(ut_ioctl(ctx, VIDIOC_S_CTRL, ctrl));

	return 0;
err:
	return -1;
}

int ut_v4l2_set_ext_ctrls(struct ut_ctx *ctx)
{
	struct v4l2_ext_controls *ext_controls = &ctx->ext_controls;

	UT_LOG(2, "-> %s()\n", __func__);

	RET(ut_ioctl(ctx, VIDIOC_S_EXT_CTRLS, ext_controls));

	return 0;
err:
	return -1;
}

int ut_v4l2_get_ctrl(struct ut_ctx *ctx)
{
	struct v4l2_control *ctrl = &ctx->ctrl;

	UT_LOG(2, "%s\n", __func__);

	RET(ut_ioctl(ctx, VIDIOC_G_CTRL, ctrl));

	return 0;
err:
	return -1;
}

struct v4l2_control *ut_get_ctrl(struct ut_ctx *ctx)
{
	return &ctx->ctrl;
}

struct v4l2_ext_controls *ut_get_ext_ctrls(struct ut_ctx *ctx)
{
	return &ctx->ext_controls;
}

int ut_v4l2_get_crop(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_crop *crop = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.crop : &ctx->cap_ctx.crop;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	crop->type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_G_CROP, crop));

	return 0;
err:
	return -1;
}

int ut_v4l2_get_selection(struct ut_ctx *ctx, __u32 buf_type)
{
	struct v4l2_selection *sel = V4L2_TYPE_IS_OUTPUT(buf_type) ?
	    &ctx->out_ctx.sel : &ctx->cap_ctx.sel;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	sel->type = V4L2_TYPE_IS_OUTPUT(buf_type) ? V4L2_BUF_TYPE_VIDEO_OUTPUT
						 : V4L2_BUF_TYPE_VIDEO_CAPTURE;
	RET(ut_ioctl(ctx, VIDIOC_G_SELECTION, sel));

	return 0;
err:
	return -1;
}

int ut_v4l2_find_and_open_dev(struct ut_ctx *ctx)
{
	int i;
	int ret;
	char path[MAX_PATH_SIZE];

	for (i = 0;; i++) {
		ret = snprintf(path, MAX_PATH_SIZE, "/dev/video%d", i);
		RET(ret < 0 || ret >= MAX_PATH_SIZE);

		ctx->dev_fd = open(path, O_RDWR | O_CLOEXEC);
		RET(ctx->dev_fd == -1);

		ret = ut_v4l2_querycap(ctx);
		if (!ret && !strncmp((const char*)ctx->cap.driver,
				     ctx->dev_name,
				     sizeof(ctx->cap.driver) - 1)) {
			UT_LOG(0, "Find \'%s\' in \'%s\'\n", ctx->cap.driver,
							     path);
			return 0;
		}
		close(ctx->dev_fd);
	}
err:
	return -1;
}

pid_t gettid() {
	return syscall(SYS_gettid);
}

void ut_v4l2_init_time(struct ut_ctx *ctx)
{
	ctx->start_play_time = get_time_ms();
	ctx->time_start = 0;
	ctx->time_end = 0;
}


int ut_v4l2_open_dev(struct ut_ctx *ctx)
{
	int dev_fd = -1;

	UT_LOG(2, "%s\n", __func__);

	ctx->cur_tid = gettid();

	if (ctx->dev_path[0] == 0)
		return ut_v4l2_find_and_open_dev(ctx);

	dev_fd = open(ctx->dev_path, O_RDWR | O_CLOEXEC);
	if (dev_fd == -1) {
		UT_ERR("Open device \'%s\' fail.\n", ctx->dev_path);
		return -1;
	}

	ctx->dev_fd = dev_fd;
	return 0;
}

void ut_v4l2_close_dev(struct ut_ctx *ctx)
{
	if (ctx->dev_fd == -1)
		return;

	close(ctx->dev_fd);
}

static int ut_mmap_buf(int fd, struct ut_buf *buf)
{
	struct v4l2_plane *planes = buf->planes;
	int i;

	ut_show_buf_planes(buf);
	for (i = 0; i < buf->n_plane; i++) {
		buf->va[i] = (unsigned char *)mmap(NULL,
						   planes[i].length,
						   PROT_READ | PROT_WRITE,
						   MAP_SHARED, fd,
						   planes[i].m.mem_offset);
		RET(buf->va[i] == MAP_FAILED);
	}
	return 0;
err:
	return -1;
}

static int ut_munmap_buf(struct ut_buf *buf)
{
	int i;

	for (i = 0; i < buf->n_plane; i++) {
		if (buf->va[i]) {
			RET(munmap(buf->va[i], buf->planes[i].length));
			buf->va[i] = NULL;
		}
	}
	return 0;
err:
	return -1;
}

static int ut_get_buf_mmap(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	struct v4l2_buffer v4l2_buf = { 0 };
	int n_plane = buf_ctx->fmt.fmt.pix_mp.num_planes;
	int i;

	for (i = 0; i < buf_ctx->buf_num; i++) {
		struct ut_buf *buf = &buf_ctx->buf_list[i];

		buf->n_plane = n_plane;
		v4l2_buf.index = i;
		v4l2_buf.type = buf_type;
		v4l2_buf.memory = V4L2_MEMORY_MMAP;
		v4l2_buf.m.planes = buf->planes;
		v4l2_buf.length = n_plane;
		RET(ut_ioctl(ctx, VIDIOC_QUERYBUF, &v4l2_buf));
		RET(ut_mmap_buf(ctx->dev_fd, buf));
	}
	return 0;
err:
	return -1;
}

static int ut_put_buf_mmap(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	int i;

	for (i = 0; i < buf_ctx->buf_num; i++)
		RET(ut_munmap_buf(&buf_ctx->buf_list[i]));
	return 0;
err:
	return -1;
}

static int ut_get_buf_mem(struct ut_ctx *ctx, enum v4l2_memory mem_type,
			  __u32 buf_type)
{
	switch (mem_type) {
	case V4L2_MEMORY_MMAP:
		RET(ut_get_buf_mmap(ctx, buf_type));
		break;
	case V4L2_MEMORY_USERPTR:
		break;
	case V4L2_MEMORY_OVERLAY:
	case V4L2_MEMORY_DMABUF:
	default:
		UT_ERR("Unknown memory type.\n");
		goto err;
	}
	return 0;
err:
	return -1;
}

static int ut_put_buf_mem(struct ut_ctx *ctx, enum v4l2_memory mem_type,
			  __u32 buf_type)
{
	switch (mem_type) {
	case V4L2_MEMORY_MMAP:
		RET(ut_put_buf_mmap(ctx, buf_type));
		break;
	case V4L2_MEMORY_USERPTR:
		break;
	case V4L2_MEMORY_OVERLAY:
	case V4L2_MEMORY_DMABUF:
	default:
		UT_ERR("Unknown memory type.\n");
		goto err;
	}
	return 0;
err:
	return -1;
}

void ut_v4l2_free_buf_queue(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx =
	    V4L2_TYPE_IS_OUTPUT(buf_type) ? &ctx->out_ctx : &ctx->cap_ctx;
	struct v4l2_requestbuffers reqbuf = { 0 };

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	if (!buf_ctx->buf_list)
		return;

	ut_put_buf_mem(ctx, buf_ctx->mem_type, buf_type);
	free(buf_ctx->buf_list);
	buf_ctx->buf_list = NULL;
	buf_ctx->free_list = NULL;

	reqbuf.count = 0;
	reqbuf.memory = buf_ctx->mem_type;
	reqbuf.type = buf_type;
	ut_ioctl(ctx, VIDIOC_REQBUFS, &reqbuf);
}

static struct ut_buf *ut_get_free_buf(struct ut_buf_ctx *buf_ctx)
{
	return buf_ctx->free_list;
}

static void ut_push_free_buf(struct ut_buf_ctx *buf_ctx, struct ut_buf *buf)
{
	buf->next_free = buf_ctx->free_list;
	buf_ctx->free_list = buf;
}

static void ut_pop_free_buf(struct ut_buf_ctx *buf_ctx)
{
	buf_ctx->free_list = buf_ctx->free_list->next_free;
}

static int ut_init_buf_list(struct ut_buf_ctx *buf_ctx)
{
	struct ut_buf *buf_list = buf_ctx->buf_list;
	int i;

	RET(buf_list == NULL);

	for (i = 0; i < buf_ctx->buf_num; i++) {
		struct ut_buf *buf = &buf_list[i];
		buf->idx = i;
		ut_push_free_buf(buf_ctx, buf);
	}
	return 0;
err:
	return -1;
}

int ut_v4l2_alloc_buf_queue(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	struct ut_buf *buf_list = NULL;
	struct v4l2_requestbuffers reqbuf = { 0 };
	long time_s, time_e;

	UT_LOG(2, "(%d) %s (mem_type=%d count=%d)\n", buf_type, __func__,
		buf_ctx->mem_type, buf_ctx->buf_num);

	time_s = get_time_ms();

	reqbuf.count = buf_ctx->buf_num;
	reqbuf.memory = buf_ctx->mem_type;
	reqbuf.type = buf_type;
	RET(ut_ioctl(ctx, VIDIOC_REQBUFS, &reqbuf));

	buf_list = calloc(buf_ctx->buf_num, sizeof(*buf_list));
	if (!buf_list) {
		UT_ERR("Out of memory.\n");
		return -1;
	}
	buf_ctx->buf_list = buf_list;
	RET(ut_init_buf_list(buf_ctx));
	RET(ut_get_buf_mem(ctx, buf_ctx->mem_type, buf_type));

	time_e = get_time_ms();
	UT_LOG(1, "tid %d alloc %d %s buffer time %ld ms\n",
		ctx->cur_tid,
		reqbuf.count,
		(buf_type == OUT) ? "output" : "capture",
		time_e - time_s);

	return 0;
err:
	free(buf_list);
	if (!buf_ctx->buf_list)
		return -1;
	reqbuf.count = 0;
	ut_ioctl(ctx, VIDIOC_REQBUFS, &reqbuf);
	buf_ctx->buf_list = NULL;
	buf_ctx->free_list = NULL;
	return -1;
}

static int ut_check_md5(struct ut_ctx *ctx, struct ut_buf *buf)
{
	FILE *f = fopen(ctx->check_hash_path, "rb");
	int md5_offset = buf->n_plane * ctx->out_data_num;
	int i;

	if (!f) {
		UT_ERR("open \'%s\' fail\n", ctx->check_hash_path);
		return -1;
	}

	if (fseek(f, md5_offset * 16, SEEK_SET)) {
		UT_ERR("seek fail\n");
		goto err;
	}

	/* We gen and check md5 for each plane */
	for (i = 0; i < buf->n_plane; i++) {
		unsigned char digest[16];
		unsigned char golden[16];
		MD5_CTX md5;

		RET(fread(golden, 16, 1, f) <= 0);
		RET(buf->va[i] == NULL);
		MD5_Init(&md5);
		MD5_Update(&md5, buf->va[i], buf->planes[i].bytesused);
		MD5_Final(digest, &md5);

		if (memcmp(digest, golden, 16)) {
			UT_ERR("Check frame %d-%d fail: src(",
			       ctx->out_data_num, i);
			for (i = 0; i < 16; i++)
				printf("%02x", digest[i]);
			printf(") golden(");
			for (i = 0; i < 16; i++)
				printf("%02x", golden[i]);
			printf(")\n");
			goto err;
		}
	}
	fclose(f);
	return 0;
err:
	fclose(f);
	return -1;
}

static int ut_gen_md5(struct ut_ctx *ctx, struct ut_buf *buf)
{
	FILE *f = fopen(ctx->gen_hash_path, "ab");
	int md5_offset = buf->n_plane * ctx->out_data_num;
	int i;

	if (!f) {
		UT_ERR("open \'%s\' fail\n", ctx->gen_hash_path);
		return -1;
	}

	if (ftell(f) / 16 != md5_offset) {
		UT_ERR("\'%s\' exitst! (Remove it first)\n",
		       ctx->gen_hash_path);
		goto err;
	}

	/* We gen and check md5 for each plane */
	for (i = 0; i < buf->n_plane; i++) {
		unsigned char digest[16];
		MD5_CTX md5;

		RET(buf->va[i] == NULL);
		MD5_Init(&md5);
		MD5_Update(&md5, buf->va[i], buf->planes[i].bytesused);
		MD5_Final(digest, &md5);
		RET(fwrite(digest, 16, 1, f) <= 0);
	}
	fclose(f);
	return 0;
err:
	fclose(f);
	return -1;
}

static int ut_file_to_buf(struct ut_ctx *ctx, struct ut_buf *buf)
{
	FILE *f;
	char path[MAX_PATH_SIZE];
	struct stat st;
	int ret, i;
	char *ptr = NULL;

	/* handle single file input*/
	if (ctx->attr & UT_ATTR_Q_EMPTY)
		return UT_RET_NO_FILE;
	if (ctx->in_file_path[0] == 0)
		return 0;
	if (ctx->in_data_num == 1 && !strchr(ctx->in_file_path, '%'))
		goto eos;

	ret = snprintf(path, MAX_PATH_SIZE, ctx->in_file_path,
		       ctx->in_data_num);
	if (ret < 0 || ret >= MAX_PATH_SIZE) {
		UT_ERR("Path too long.");
		return -1;
	}

	for (i = 0; i < buf->n_plane; i++) {
		if (buf->n_plane == 2) {
			if (i == 1) {
				ptr = strstr(path, ".y.bin");
				*(ptr + 1) = 'c';
			}
		} else if (buf->n_plane == 3) {
			if (i == 1) {
				ptr = strstr(path, ".y.bin");
				snprintf(ptr, MAX_PATH_SIZE, ".cb.bin");
			} else if (i == 2) {
				ptr = strstr(path, ".cb.bin");
				snprintf(ptr, MAX_PATH_SIZE, ".cr.bin");
			}
		}

		f = fopen(path, "rb");
		if (!f) {
			UT_ERR("open %s fail\n", path);
			if (ctx->in_data_num == 0)
				return -1;
			goto eos;
		}
		RET(stat(path, &st) == -1);
		if (ctx->out_ctx.mem_type == V4L2_MEMORY_USERPTR) {
			buf->planes[i].length = ALIGN(st.st_size + 1024, DMA_ALIGN);
			buf->planes[i].bytesused = st.st_size + 1024;
			buf->planes[i].m.userptr = (unsigned long)
				aligned_alloc(DMA_ALIGN, buf->planes[i].length);
			buf->va[i] = (unsigned char *)buf->planes[i].m.userptr;
		}

		RET(buf->planes[i].length < st.st_size);
		RET(buf->va[i] == NULL);
		ret = fread(buf->va[i], 1, st.st_size, f);
		if (ret <= 0) {
			UT_ERR("fread fail. (%d)", ret);
			goto err;
		}

		fclose(f);
	}
	return 0;
eos:
	/* queue empty buf */
	buf->planes[0].bytesused = 0;
	ctx->attr |= UT_ATTR_Q_EMPTY;
	return 0;
err:
	fclose(f);
	return -1;

}

static int ut_fill_buf(struct ut_ctx *ctx, struct ut_buf *buf)
{
	struct ut_buf_ctx *buf_ctx = &ctx->out_ctx;

	UT_LOG(2, "%s (%d)\n", __func__, ctx->in_data_num);

	if (buf_ctx->buf_handle)
		return buf_ctx->buf_handle(buf_ctx->user_data, buf);
	else
		return ut_file_to_buf(ctx, buf);
}

static int ut_buf_to_file(struct ut_ctx *ctx, struct ut_buf *buf)
{
	FILE *f;
	char path[MAX_PATH_SIZE];
	int i, ret;
	char *ptr =NULL;

	ret = snprintf(path, MAX_PATH_SIZE, ctx->out_file_path,
		       ctx->out_data_num);
	if (ret < 0 || ret >= MAX_PATH_SIZE) {
		UT_ERR("Path too long.");
		return -1;
	}

	for (i = 0; i < buf->n_plane; i++) {

		if (buf->n_plane == 2) {
			if (i == 1) {
				ptr = strstr(path, ".y.bin.out");
				*(ptr + 1) = 'c';
			}
		}

		if (!strchr(ctx->out_file_path, '%')) {
			if (ctx->out_data_num == 0)
				remove(path);
			f = fopen(path, "ab");
		} else {
			remove(path);
			f = fopen(path, "wb");
		}

		if (!f) {
			UT_ERR("Open file \'%s\' fail", path);
			return -1;
		}

		RET(buf->va[i] == NULL);
		ret = fwrite(buf->va[i], 1, buf->planes[i].bytesused, f);
		if (ret <= 0) {
			UT_ERR("fwrite fail. (%d)", ret);
			goto err;
		}
		fclose(f);
	}

	return 0;
err:
	fclose(f);
	return -1;
}

static int ut_output_buf(struct ut_ctx *ctx, struct ut_buf *buf)
{
	struct ut_buf_ctx *buf_ctx = &ctx->cap_ctx;

	UT_LOG(1, "%s (%d)\n", __func__, ctx->out_data_num);

	if (buf_ctx->buf_handle)
		return buf_ctx->buf_handle(buf_ctx->user_data, buf);
	else
		return ut_buf_to_file(ctx, buf);
}

static int ut_check_hash(struct ut_ctx *ctx, struct ut_buf *buf)
{
	UT_LOG(1, "Check hash (frame %d)\n", ctx->out_data_num);

	if (ctx->check_handle)
		return ctx->check_handle(ctx->check_udata, buf);
	else
		return ut_check_md5(ctx, buf);
}

static int ut_gen_hash(struct ut_ctx *ctx, struct ut_buf *buf)
{
	UT_LOG(1, "Gen hash (frame %d)\n", ctx->out_data_num);

	if (ctx->gen_handle)
		return ctx->gen_handle(ctx->gen_udata, buf);
	else
		return ut_gen_md5(ctx, buf);
}

static int ut_process_buf(struct ut_ctx *ctx, struct ut_buf *buf)
{
	if (ctx->out_file)
		RET(ut_output_buf(ctx, buf));
	if (ctx->check_hash)
		RET(ut_check_hash(ctx, buf));
	if (ctx->gen_hash)
		RET(ut_gen_hash(ctx, buf));
	return 0;
err:
	return -1;
}

struct ut_buf *ut_v4l2_get_buf(struct ut_ctx *ctx, int idx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	struct ut_buf *buf = NULL;

	RET(idx >= buf_ctx->buf_num);
	buf = &buf_ctx->buf_list[idx];
err:
	return buf;
}

int ut_v4l2_q_buf(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx = V4L2_TYPE_IS_OUTPUT(buf_type)
	    ? &ctx->out_ctx : &ctx->cap_ctx;
	struct v4l2_buffer v4l2_buf = {0};
	struct ut_buf *buf;
	int ret = 0, i;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	buf = ut_get_free_buf(buf_ctx);
	if (buf == NULL)
		return UT_RET_NO_FREE_BUF;

	if (buf_ctx->mem_type == V4L2_MEMORY_USERPTR) {
		buf->n_plane = buf_ctx->n_plane;
		for (i = 0; i < buf->n_plane; i++) {
			buf->planes[i] = buf_ctx->planes[i];
			buf->va[i] = (unsigned char *)buf->planes[i].m.userptr;
		}
	}

	if (V4L2_TYPE_IS_OUTPUT(buf_type)) {
		ret = ut_fill_buf(ctx, buf);
		if (ret == UT_RET_NO_FILE)
			goto end;
		RET(ret == -1);
		ctx->in_data_num++;
		ut_v4l2_init_time(ctx);
	} else {
		int i;
		for (i = 0; i < buf->n_plane; i++)
			buf->planes[i].bytesused = 0;
	}

	v4l2_buf.index = buf->idx;
	v4l2_buf.type = buf_type;
	v4l2_buf.m.planes = buf->planes;
	v4l2_buf.memory = buf_ctx->mem_type;
	v4l2_buf.length = buf->n_plane;
	ut_show_buf_planes(buf);
	RET(ut_ioctl(ctx, VIDIOC_QBUF, &v4l2_buf));
end:
	ut_pop_free_buf(buf_ctx);
	return ret;
err:
	return -1;
}

int ut_v4l2_q_all_buf(struct ut_ctx *ctx, __u32 buf_type)
{
	int ret;
	do {
		ret = ut_v4l2_q_buf(ctx, buf_type);
		if (ret == UT_RET_NO_FREE_BUF)
			return 0;
		if (ret == UT_RET_NO_FILE)
			return ret;
		RET(ret);
	} while (1);
err:
	return -1;
}

int ut_v4l2_dq_buf(struct ut_ctx *ctx, __u32 buf_type)
{
	struct ut_buf_ctx *buf_ctx =
	    V4L2_TYPE_IS_OUTPUT(buf_type) ? &ctx->out_ctx : &ctx->cap_ctx;
	struct v4l2_buffer v4l2_buf = { 0 };
	struct ut_buf *buf;
	struct v4l2_plane planes[VIDEO_MAX_PLANES] = { 0 };
	int i, ret = 0;

	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	v4l2_buf.type = buf_type;
	v4l2_buf.m.planes = planes;
	v4l2_buf.memory = buf_ctx->mem_type;
	v4l2_buf.length = buf_ctx->fmt.fmt.pix_mp.num_planes;
	ret = ut_ioctl(ctx, VIDIOC_DQBUF, &v4l2_buf);
	RET(v4l2_buf.flags & V4L2_BUF_FLAG_ERROR);
	if (ret == -EAGAIN)
		return ret;
	RET(ret);

	buf = &buf_ctx->buf_list[v4l2_buf.index];
	for (i = 0; i < buf->n_plane; i++)
		buf->planes[i].bytesused = planes[i].bytesused;
	ut_show_buf_planes(buf);

	if (V4L2_TYPE_IS_OUTPUT(buf_type))
		goto end;

	if (planes[0].bytesused == 0) {
		UT_LOG(2, "Got EOS.\n");
		ret = UT_RET_EOS;
		goto end;
	}

	if (buf_type == CAP) {
		ctx->fps ++;
		if (ctx->time_start == 0) {
			ctx->fps = 1;
			ctx->time_start = get_time_ms();
			UT_LOG(1, "tid %d 1st pic output delay = %lld ms\n",
				ctx->cur_tid, ctx->time_start - ctx->start_play_time);
		} else {
			ctx->time_end = get_time_ms();
			if (ctx->time_end - ctx->time_start >= 1000LL) {
				ctx->time_start = ctx->time_end;
				UT_LOG(1, "dev_name: %s, tid:%d fps = %d\n", ctx->dev_name, ctx->cur_tid, ctx->fps);
				ctx->fps = 0;
			}
		}
	}

	RET(ut_process_buf(ctx, buf));
	ctx->out_data_num++;

end:
	ut_push_free_buf(buf_ctx, buf);
	return ret;
err:
	return -1;
}

int ut_v4l2_dq_all_buf(struct ut_ctx *ctx, __u32 buf_type)
{
	int ret;
	do {
		ret = ut_v4l2_dq_buf(ctx, buf_type);
		if (ret == -EAGAIN)
			return 0;
		if (ret == UT_RET_EOS)
			return ret;
		RET(ret);
	} while (1);
err:
	return -1;
}

int ut_v4l2_stream_on(struct ut_ctx *ctx, __u32 buf_type)
{
	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	RET(ut_ioctl(ctx, VIDIOC_STREAMON, &buf_type));
	return 0;
err:
	return -1;
}

int ut_v4l2_stream_off(struct ut_ctx *ctx, __u32 buf_type)
{
	UT_LOG(2, "(%d) %s\n", buf_type, __func__);

	RET(ut_ioctl(ctx, VIDIOC_STREAMOFF, &buf_type));
	return 0;
err:
	return -1;
}

int ut_v4l2_process_cap_task(struct ut_ctx *ctx)
{
	int ret;

	UT_LOG(2, "%s\n", __func__);

	do {
		RET(ut_v4l2_q_all_buf(ctx, CAP));
		ret = ut_v4l2_poll(ctx, CAP);
		if (ret == UT_RET_RES_CHANGE) {
			RET(ut_v4l2_dq_all_buf(ctx, CAP));
			RET(ut_v4l2_stream_off(ctx, CAP));
			ut_v4l2_free_buf_queue(ctx, CAP);
			RET(ut_v4l2_get_fmt(ctx, CAP));
			RET(ut_v4l2_alloc_buf_queue(ctx, CAP));
			RET(ut_v4l2_stream_on(ctx, CAP));
			continue;
		}
		if (ret == UT_RET_SRC_NOT_SUPPORT) {
			return 0;
		}

		RET(ret);
		ret = ut_v4l2_dq_all_buf(ctx, CAP);
		if (ret == UT_RET_EOS) {
			return 0;
		}
		RET(ret);
	} while (1);
err:
	return -1;
}

int ut_v4l2_process_out_task(struct ut_ctx *ctx)
{
	int ret;

	UT_LOG(2, "%s\n", __func__);

	do {
		ret = ut_v4l2_q_all_buf(ctx, OUT);
		if (ret == UT_RET_NO_FILE)
			return 0;
		RET(ret);
		RET(ut_v4l2_poll(ctx, OUT));

		RET(ut_v4l2_dq_all_buf(ctx, OUT));
	} while (1);
err:
	return -1;
}

/*
int ut_start_thread(struct ut_ctx *ctx, void *(*task) (void *), void *user_data)
{
	int ret = -1;
	pthread_attr_t attr;

	UT_LOG(2, "%s\n", __func__);

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	RET(pthread_create(&ctx->thread, &attr, task, user_data));

	ctx->start_thread = true;
	ret = 0;
err:
	pthread_attr_destroy(&attr);
	return ret;
}

long ut_stop_thread(struct ut_ctx *ctx)
{
	void *ret;

	if (!ctx->start_thread)
		return 0;
	UT_LOG(2, "+%s\n", __func__);

	pthread_join(ctx->thread, &ret);
	ctx->start_thread = false;

	UT_LOG(2, "-%s\n", __func__);

	return (long)ret;
}
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <linux/errno.h>
#include "img_hal.h"

int mtk_imghal_s_dst_fmt(struct mtk_imghal_param *param)
{
	int i;
	const unsigned int *bpp, *bpl;

	/* user may need modify dst_fmt here*/
	if (param->dst_fmt.width == 0)
		param->dst_fmt.width = param->dec_fmt.width;
	if (param->dst_fmt.height == 0)
		param->dst_fmt.height = param->dec_fmt.height;
	if (param->dst_fmt.pixelformat == 0) {
		param->dst_fmt.pixelformat = param->dec_fmt.pixelformat;
		param->dst_fmt.num_planes = param->dec_fmt.num_planes;
	} else
		param->dst_fmt.num_planes = ut_v4l2_fmt_planes_num(param->dst_fmt.pixelformat);

	bpp = ut_v4l2_fmt_bpp(param->dst_fmt.pixelformat);
	bpl = ut_v4l2_fmt_bpl(param->dst_fmt.pixelformat);

	for (i = 0; i < param->dst_fmt.num_planes; i++) {
		struct v4l2_plane *plane = &param->dst_buf.planes[i];
		param->dst_fmt.plane_fmt[i].bytesperline = ALIGN(DIV_ALIGN(param->dst_fmt.width * bpl[i], 8), 16);
		plane->bytesused = param->dst_fmt.plane_fmt[i].bytesperline * param->dst_fmt.height / (bpl[i] / bpp[i]);
		plane->length = ALIGN(param->dst_buf.planes[i].bytesused, DMA_ALIGN);
		param->dst_fmt.plane_fmt[i].sizeimage = plane->length;
		plane->m.userptr = (unsigned long)
			aligned_alloc(DMA_ALIGN, plane->length);
	}
	snprintf(param->dst_buf.file_path, MAX_PATH_SIZE, "%s.out", param->src_buf.file_path);
	return 0;
err:
	return -1;
}

int main(int argc, char **argv)
{
	int i, j;
	struct mtk_imghal_param *param;

	for (i = 0; i < argc; i++)
		printf("param[%u]: %s\n", i, argv[i]);
	i = 1;
	param = calloc(1, sizeof(*param));
	mtk_imghal_init_param(param);
	if (strstr(argv[i], ".png")) {
		param->img_type = MTK_IMG_PNG;
		param->src_fmt.num_planes = 1;
		if (argc < 5)
			return -1;
	}
	else if (strstr(argv[i], ".jpg") || strstr(argv[i], ".jpeg")) {
		param->img_type = MTK_IMG_JPEG;
		param->src_fmt.num_planes = 1;
		if (argc < 5)
			return -1;
	}
	else if (strstr(argv[i], ".bin")) {
		param->img_type = MTK_IMG_UNCOMPRESS;
		if (argc < 6)
			return -1;
	}
	else
		return -1;

	strncpy(param->src_buf.file_path, argv[i], MAX_PATH_SIZE);
	i++;

	/* dst fmt */
	if (!strcmp(argv[i], "ARGB8888")) {
		param->dst_fmt.pixelformat = V4L2_PIX_FMT_ARGB32;
		param->dst_fmt.num_planes = 1;
	}
	else if (!strcmp(argv[i], "AYUV")) {
		param->dst_fmt.pixelformat = V4L2_PIX_FMT_AYUV;
		param->dst_fmt.num_planes = 1;
	}
	else if (!strcmp(argv[i], "Y8")) {
		param->dst_fmt.pixelformat = V4L2_PIX_FMT_GREY;
		param->dst_fmt.num_planes = 1;
	}
	else if (!strcmp(argv[i], "NV12")) {
		param->dst_fmt.pixelformat = V4L2_PIX_FMT_NV12;
		param->dst_fmt.num_planes = 2;
	}
	else if (!strcmp(argv[i], "NV16")) {
		param->dst_fmt.pixelformat = V4L2_PIX_FMT_NV16;
		param->dst_fmt.num_planes = 2;
	}
	else if (!strcmp(argv[i], "0"))
		printf("unsupported format:%s\n", argv[i]);
	i++;
	param->dst_fmt.width = atoi(argv[i]);
	i++;
	param->dst_fmt.height = atoi(argv[i]);
	i++;
	for (j = 0; j < param->dst_fmt.num_planes; j++) {
		param->dst_fmt.plane_fmt[j].bytesperline = IMG_DEF_PITCH;
		param->dst_fmt.plane_fmt[j].sizeimage = DMA_ALIGN;
	}

	/* src fmt only for imgrz */
	if (param->img_type == MTK_IMG_UNCOMPRESS) {
		if (!strcmp(argv[i], "NV12")) {
			param->src_fmt.pixelformat = V4L2_PIX_FMT_NV12;
			param->src_fmt.num_planes = 2;
		}
		else if (!strcmp(argv[i], "AYUV")) {
			param->src_fmt.pixelformat = V4L2_PIX_FMT_AYUV;
			param->src_fmt.num_planes = 1;
		}
		else if (!strcmp(argv[i], "YV12")) {
			param->src_fmt.pixelformat = V4L2_PIX_FMT_YUV420M;
			param->src_fmt.num_planes = 3;
		}
		else if (!strcmp(argv[i], "YV16")) {
			param->src_fmt.pixelformat = V4L2_PIX_FMT_YUV422M;
			param->src_fmt.num_planes = 3;
		}
		else if (!strcmp(argv[i], "YV24")) {
			param->src_fmt.pixelformat = V4L2_PIX_FMT_YUV444M;
			param->src_fmt.num_planes = 3;
		}
		else
			return -1;
		i++;
		param->src_fmt.width = atoi(argv[i]);
		i++;
		param->src_fmt.height = atoi(argv[i]);
		i++;
		for (j = 0; j < param->src_fmt.num_planes; j++) {
			param->src_fmt.plane_fmt[j].bytesperline = IMG_DEF_PITCH;
			param->src_fmt.plane_fmt[j].sizeimage = DMA_ALIGN;
		}
	}
	param->set_dst_fmt = mtk_imghal_s_dst_fmt;
	mtk_imghal_dec(param);
}

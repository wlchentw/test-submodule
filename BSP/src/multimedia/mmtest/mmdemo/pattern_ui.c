#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"

#if 0
#define WINDOWS_PATH "/data/windows.png"
#define BUTTON_PATH  "/data/button.png"
#define JPG1_PATH	"/data/00.jpg"
#endif

int draw_ui(Pattern_parm *ptPatternParm)
{
	char file_path[MAX_PATH_LENGTH];
	enum mtk_img_type rImgType = 0;
	int status = 0;

	memcpy(file_path, ptPatternParm->input_buf, sizeof(ptPatternParm->input_buf));

	rImgType = getFileType(file_path);
	if (rImgType < MTK_IMG_JPEG || rImgType > MTK_IMG_UNCOMPRESS)
		return -1;

	printf("<UI>rImgType %d, file_path %s\n", rImgType, file_path);


	//decode api
	//in->file path, rImgType
	//out-> out_w, out_h, out_fmt, out_buf(data buffer)

	mtk_imghal_init_param(ptPatternParm->ptImgparam);

	ptPatternParm->ptImgparam->img_type = rImgType;
	ptPatternParm->ptImgparam->src_fmt.num_planes = 1;
	int i;
	const unsigned int *bpp;


	/* user may need modify dst_fmt here*/
	ptPatternParm->ptImgparam->dst_fmt.width = ptPatternParm->rDisplayRegion.width;
	ptPatternParm->ptImgparam->dst_fmt.height = ptPatternParm->rDisplayRegion.height;
	if (MTK_IMG_JPEG == rImgType)
	{
		ptPatternParm->ptImgparam->dst_fmt.pixelformat = V4L2_PIX_FMT_GREY;
		ptPatternParm->v4l2_src_color_fmt = V4L2_PIX_FMT_Y8;
	}
	else
	{
		ptPatternParm->ptImgparam->dst_fmt.pixelformat = V4L2_PIX_FMT_ARGB32;
		ptPatternParm->v4l2_src_color_fmt = V4L2_PIX_FMT_ARGB32;
	}
	//ptPatternParm->v4l2_dst_color_fmt = V4L2_PIX_FMT_Y4_M0;
	ptPatternParm->ptImgparam->dst_fmt.num_planes = 1;

	bpp = ut_v4l2_fmt_bpp(ptPatternParm->ptImgparam->dst_fmt.pixelformat);
	for (i = 0; i < ptPatternParm->ptImgparam->dst_fmt.num_planes; i++) {
		struct v4l2_plane *plane = &ptPatternParm->ptImgparam->dst_buf.planes[i];
		plane->bytesused = ALIGN(DIV_ALIGN(ptPatternParm->ptImgparam->dst_fmt.width * bpp[i], 8), 16) * ptPatternParm->ptImgparam->dst_fmt.height;
		plane->length = ALIGN(ptPatternParm->ptImgparam->dst_buf.planes[i].bytesused, DMA_ALIGN);
		plane->m.userptr = (unsigned long)
			aligned_alloc(DMA_ALIGN, plane->length);
		ptPatternParm->ptImgparam->dst_fmt.plane_fmt[i].bytesperline = ALIGN(DIV_ALIGN(ptPatternParm->ptImgparam->dst_fmt.width * bpp[i], 8), 16);
		ptPatternParm->ptImgparam->dst_fmt.plane_fmt[i].sizeimage = plane->length;
		 //sprintf(ptPatternParm->ptImgparam->dst_buf.file_path, "%s.out", file_path);
	}

	strncpy(ptPatternParm->ptImgparam->src_buf.file_path, file_path, MAX_PATH_SIZE);
	status = mtk_imghal_dec(ptPatternParm->ptImgparam);
	if (status != 0) {
	printf("mtk_imghal_dec return fail:%d\n", status);
	return status;
	}
	ptPatternParm->src_addr[0] = (unsigned char*)ptPatternParm->ptImgparam->dst_buf.planes[0].m.userptr;
	ptPatternParm->pixelSize = ptPatternParm->ptImgparam->dst_buf.planes[0].length;

	ptPatternParm->src_width = ptPatternParm->ptImgparam->dst_fmt.width;
	ptPatternParm->src_height = ptPatternParm->ptImgparam->dst_fmt.height;
	if (0 == ptPatternParm->iCropType)
	{
		ptPatternParm->rCropRegion.width = ptPatternParm->src_width;
		ptPatternParm->rCropRegion.height = ptPatternParm->src_height;
	}
	return status;
}


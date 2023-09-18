#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"

int draw_picture(Pattern_parm *ptPatternParm)
{
	char file_path[50];
	enum mtk_img_type rImgType = 0;

	printf("<PICTURE> input_buf %s\n", ptPatternParm->input_buf);
	rImgType = getFileType(ptPatternParm->input_buf);
	printf("<PICTURE>rImgType %d\n", rImgType);


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
	ptPatternParm->ptImgparam->dst_fmt.pixelformat = V4L2_PIX_FMT_ARGB32;
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
	}

	strncpy(ptPatternParm->ptImgparam->src_buf.file_path, ptPatternParm->input_buf, MAX_PATH_SIZE);
	mtk_imghal_dec(ptPatternParm->ptImgparam);
	ptPatternParm->src_addr[0] = (unsigned char*)ptPatternParm->ptImgparam->dst_buf.planes[0].m.userptr;
	ptPatternParm->pixelSize = ptPatternParm->ptImgparam->dst_buf.planes[0].length;

	ptPatternParm->src_width = ptPatternParm->ptImgparam->dst_fmt.width;
	ptPatternParm->src_height = ptPatternParm->ptImgparam->dst_fmt.height;
	if (0 == ptPatternParm->iCropType)
	{
		ptPatternParm->rCropRegion.width = ptPatternParm->src_width;
		ptPatternParm->rCropRegion.height = ptPatternParm->src_height;
	}
	return 0;
}


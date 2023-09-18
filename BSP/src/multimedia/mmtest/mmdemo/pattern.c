#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"


static const int LP_PIC_JPEG_HEADER[] = {0XFF, 0XD8};

static const int LP_PIC_PNG_HEADER[] = {0X89, 0X50, 0X4E, 0X47, 0X0D, 0X0A, 0X1A, 0X0A};
static const int LP_PIC_PNG_IHDR_FLAG[] = {'I', 'H', 'D', 'R'};

//to get picture file type
enum mtk_img_type getFileType(char * file_path)
{
	FILE *fdes = NULL;
	unsigned char *rbuf = NULL;
	int fsize = 0;
	int rsize = 0;
	enum mtk_img_type imgType = 0;
	printf("<Pattern> file_path %s\n", file_path);
	fdes = fopen(file_path, "rb");
	if(!fdes)
	{
		printf("<Pattern>open file error!\n");
		return 0;
	}
	if(fseek(fdes, 0, SEEK_END) < 0)
	{
		printf("<Pattern>seek file error!\n");
		if (fdes)
    	{
    		fclose(fdes);
    	}
		return 0;
	}
	fsize = ftell(fdes);
	if (fsize <= 0)
	{
		printf("<Pattern>ftell file error!\n");
		if (fdes)
    	{
    		fclose(fdes);
    	}
		return 0;
	}
	printf("<Pattern>ui file size is = %d!\n", fsize);
	if (fseek(fdes, 0, SEEK_SET) < 0)
	{
		printf("<Pattern>fseek(SEEK_SET) error!\n");
		if (fdes)
    	{
    		fclose(fdes);
    	}
		return 0;
	}
	rbuf = (unsigned char*)malloc(10);
	if (!rbuf)
	{
		printf("<Pattern>malloc(%d) error!", 10);
		if (fdes)
    	{
    		fclose(fdes);
    	}
		return 0;
	}
	memset(rbuf, 0, 10);
	rsize = fread(rbuf, 1, 10, fdes);
	printf("<Pattern>fread header rbuf[0] %x, rbuf[1] %x, rbuf[2] %x, rbuf[3] %x \n", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
	if ((LP_PIC_JPEG_HEADER[0] == rbuf[0])
		 && (LP_PIC_JPEG_HEADER[1] == rbuf[1]))
	{
		printf("<Pattern>JPG file \n");
		imgType = MTK_IMG_JPEG;
	}
	else if (LP_PIC_PNG_HEADER[0] == rbuf[0]
			 && LP_PIC_PNG_HEADER[1] == rbuf[1]
			 && LP_PIC_PNG_HEADER[2] == rbuf[2]
			 && LP_PIC_PNG_HEADER[3] == rbuf[3]
			 && LP_PIC_PNG_HEADER[4] == rbuf[4]
			 && LP_PIC_PNG_HEADER[5] == rbuf[5]
			 && LP_PIC_PNG_HEADER[6] == rbuf[6]
			 && LP_PIC_PNG_HEADER[7] == rbuf[7])
	{
		printf("<Pattern>Png file \n");
		imgType = MTK_IMG_PNG;
	}

	if (rbuf)
	{
		free(rbuf);
		rbuf = NULL;
	}

	if (fdes)
	{
		fclose(fdes);
	}
	return imgType;
}

int mtk_imghal_s_dst_fmt(struct mtk_imghal_param *param)
{
	int i;
	const unsigned int *bpp;

	/* user may need modify dst_fmt here*/
	param->dst_fmt = param->dec_fmt;

	bpp = ut_v4l2_fmt_bpp(param->dst_fmt.pixelformat);
	for (i = 0; i < param->dst_fmt.num_planes; i++) {
		struct v4l2_plane *plane = &param->dst_buf.planes[i];
		plane->bytesused = ALIGN(DIV_ALIGN(param->dst_fmt.width * bpp[i], 8), 16) * param->dst_fmt.height;
		plane->length = ALIGN(param->dst_buf.planes[i].bytesused, DMA_ALIGN);
		plane->m.userptr = (unsigned long)
			aligned_alloc(DMA_ALIGN, plane->length);
	}
	#if 0
	snprintf(param->dst_buf.file_path, MAX_PATH_SIZE, "%s.out", param->src_buf.file_path);
	#endif
	return 0;
err:
	return -1;
}


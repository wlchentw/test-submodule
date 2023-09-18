#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mdp_api.h"
/* V4L2 header files */
//#include "videodev2.h" /* <linux/videodev2.h> is not latest. */

int g_log_level = 4;

#define IOCTL_OR_ERROR_RETURN_VALUE(fd, type, arg, value)	\
	do {	\
		int r; \
		if (r = video_ioctl(fd, type, arg) != 0) {	\
			if (g_log_level >= 1) { \
				printf("[%s] %s()%d: ioctl failed: return %d  (%x)%s\n", LOG_TAG, __func__, __LINE__, r, (unsigned int)(type), #type);\
			} \
			return value;		\
		}	\
	} while (0)

#define IOCTL_OR_ERROR_RETURN_FALSE(fd, type, arg) \
	IOCTL_OR_ERROR_RETURN_VALUE(fd, type, arg, 0)

#define VIDEO_MAX_PLANES               8

enum
{
	MDP_STATE_UNKNOWN,
	MDP_STATE_INIT,
	MDP_STATE_CONFIG_SRC,
	MDP_STATE_CONFIG_DST,
	MDP_STATE_CONFIG_PQ,
	MDP_STATE_STREAM_ON,
	MDP_STATE_STREAM_RUN,
	MDP_STATE_STREAM_OFF,
	MDP_STATE_DEINIT
};

int write_bmp(char *fn, struct BITMAPFILEHEADER bfh, struct BITMAPINFOHEADER bih, unsigned char *bufaddr, unsigned int bufsize, unsigned char format)
{
    FILE *fp = 0;

    fp = fopen(fn, "wb");
    if (fp==0) {
	V4L2_LOG("Faild to create file: %s", fn);
        return -2;
    }

    unsigned char byte;
    unsigned int bytes;
    unsigned int s_bytes;
    byte = 'B';
    fwrite(&byte, 1, 1, fp);
    byte = 'M';
    fwrite(&byte, 1, 1, fp);
    bfh.file_size += 256 * 4;
    if (bih.width % 4 != 0)
    {
	    bfh.file_size += (4 - (bih.width % 4)) * bih.height;
    }
    bfh.file_size +=
    byte = bfh.file_size & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = (bfh.file_size & 0xff00) >> 8;
    fwrite(&byte, 1, 1, fp);
    byte = (bfh.file_size & 0xff0000) >> 16;
    fwrite(&byte, 1, 1, fp);
    byte = (bfh.file_size & 0xff000000) >> 24;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    bfh.data_offset += 256 * 4;
    byte = bfh.data_offset & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = (bfh.data_offset >> 8) & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 40;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = bih.width & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.width & 0xff00) >> 8;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.width & 0xff0000) >> 16;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.width & 0xff000000) >> 24;
    fwrite(&byte, 1, 1, fp);
    byte = bih.height & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.height & 0xff00) >> 8;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.height & 0xff0000) >> 16;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.height & 0xff000000) >> 24;
    fwrite(&byte, 1, 1, fp);
    byte = 1;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 8;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    byte = 0;
    fwrite(&byte, 1, 1, fp);
    if (bih.width % 4 != 0)
    {
	    bih.data_size += (4 - (bih.width % 4)) * bih.height;
    }
    byte = bih.data_size & 0xff;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.data_size & 0xff00) >> 8;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.data_size & 0xff0000) >> 16;
    fwrite(&byte, 1, 1, fp);
    byte = (bih.data_size & 0xff000000) >> 24;
    fwrite(&byte, 1, 1, fp);

    bytes = 0x00000ec4;
    fwrite(&bytes, 1, 4, fp);
    bytes = 0x0;
    bytes = 0x00000ec4;
    fwrite(&bytes, 1, 4, fp);
    {
	    bytes = 0x00000100;
	    fwrite(&bytes, 1, 4, fp);
	    bytes = 0x00000100;
	    fwrite(&bytes, 1, 4, fp);
	    int i;
	    for (i = 0; i < 256; i++)
	    {
	    unsigned int bytes;
	    bytes = i | (i << 8) | (i << 16) | (0xff << 24);
	    fwrite(&bytes, 1, 4, fp);
	    }
    }

    if (bufaddr && bufsize)
    {
	int i, j;
	for (i = 0; i < bih.height; i++)
	{
        	fwrite(bufaddr + (bih.width * (bih.height - i - 1)), 1, bih.width, fp);
		for (j = 0; j <  4 - (bih.width % 4) && (bih.width % 4) != 0; j++)
		{
	    		byte = 0;
	    		fwrite(&byte, 1, 1, fp);
		}
	}
    }
    V4L2_LOG("Output file: %s", fn);

	fclose(fp);

	return 0;
}


int write_buffer_to_file(const char *fn, unsigned char *bufaddr, unsigned int bufsize)
{
    FILE *fp = 0;

    fp = fopen(fn, "wb");
    if (fp==0) {
	V4L2_LOG("Faild to create file: %s", fn);
        return -2;
    }
#if 1
    int writed = 0, w;
    while (1)
    {
	if (writed < bufsize)
	{
		w = bufsize - writed > 1024 * 10 ? 1024 * 10 : bufsize - writed;
        	writed += fwrite(bufaddr + writed, 1, w, fp);
	}
	else
		break;
    }
#else

    fwrite(bufaddr, 1, bufsize, fp);
#endif
    fclose(fp); fp = 0;
    V4L2_LOG("Output file: %s", fn);

    return 0;
}

int getfilesize( FILE *fp )
{
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}

unsigned int read_file_to_buffer( const char *fn, unsigned char *buf)
{
	FILE *fp = 0;
	unsigned int read_size = 0;
	unsigned int file_size = 0;

	fp = fopen( fn, "r" );
	if( !fp ){
		V4L2_LOG( "Failed to open file %s", fn );
		goto END;
	}

	file_size = getfilesize( fp );
	int ret = 0;
	do {
		read_size += ret;
		ret = fread( buf+read_size, 1,  1024 * 1024, fp );
	} while (read_size < file_size && ret > 0);
	if( read_size != file_size ){
		V4L2_LOG( "Failed to read file: read %d, expected %d", read_size, file_size );
	}

END:
	if( fp ) fclose(fp);

	return read_size;
}

int video_ioctl(int fd, int cmd, void *p)
{
	int ret;
	struct timeval start, stop, diff;

	if (/* ioctl_performance */ 0)
		gettimeofday(&start, NULL);

	ret = ioctl(fd, cmd, p);

	if (/* ioctl_performance */ 0) {
		gettimeofday(&stop, NULL);
		/* Print time... */
		timersub(&stop, &start, &diff);
		MS_ERR("ioctl: %s(%x), time: %lu.%03lu ms, ret=%d",
			cmd_to_str(cmd), cmd,
			diff.tv_sec*1000 + diff.tv_usec/1000,
			diff.tv_usec%1000,
			ret);
	}

//	V4L2_LOG("v4l2_cmd: 0x%08x return 0x%08x(%d)", cmd, ret, ret);

	return ret;
}

static int Format2PlaneFmt(unsigned int fmt, unsigned int w, unsigned int h,
		struct v4l2_plane_pix_format *plane_fmt,
		unsigned int *nplanes, unsigned int pitch)
{
	switch (fmt)
	{
		case V4L2_PIX_FMT_ARGB32:
			*nplanes = 1;
			if (pitch == w) pitch = pitch * 4;
			plane_fmt[0].bytesperline = pitch;//w * 4;
			plane_fmt[0].sizeimage = plane_fmt[0].bytesperline * h;
			if (plane_fmt[0].sizeimage % 64 != 0)
				plane_fmt[0].sizeimage = (plane_fmt[0].sizeimage & (~63)) + 64;
			break;
		case V4L2_PIX_FMT_RGB24:
			*nplanes = 1;
			if (pitch == w) pitch = pitch * 3;
			plane_fmt[0].bytesperline = pitch;//w * 3;
			plane_fmt[0].sizeimage = plane_fmt[0].bytesperline * h;
			break;
		case V4L2_PIX_FMT_Y4_M2:
		case V4L2_PIX_FMT_Y2_M3:
		case V4L2_PIX_FMT_Y1_M3:
			*nplanes = 1;
			plane_fmt[0].bytesperline = pitch;//w / 2;
			plane_fmt[0].sizeimage = plane_fmt[0].bytesperline * h;
			plane_fmt[0].sizeimage = (plane_fmt[0].bytesperline * h + 4095) & 0xFFFFFF00;
			break;
		case V4L2_PIX_FMT_GREY:
		case V4L2_PIX_FMT_Y8:
		case V4L2_PIX_FMT_Y4_M0:
		case V4L2_PIX_FMT_Y4_M1:
		case V4L2_PIX_FMT_Y2_M0:
		case V4L2_PIX_FMT_Y2_M1:
		case V4L2_PIX_FMT_Y2_M2:
		case V4L2_PIX_FMT_Y1_M0:
		case V4L2_PIX_FMT_Y1_M1:
		case V4L2_PIX_FMT_Y1_M2:
		default:
			*nplanes = 1;
			plane_fmt[0].bytesperline = pitch;//w;
			plane_fmt[0].sizeimage = plane_fmt[0].bytesperline * h;
			break;
	}

	return 0;
}


int mdp_open_device(void)
{
#define VIDEO_DEV_NAME		"/dev/video"
#define MTK_MDP_MODULE_NAME	"mtk-mdp"

	int fd = -1;
	char dev_node[64];
	int i = 0;
	int ret;
	struct v4l2_capability c;
	char driver[16];

	strncpy(driver, MTK_MDP_MODULE_NAME, sizeof(MTK_MDP_MODULE_NAME));

	for (i = 0; i < 64; i++) {
		sprintf(dev_node, "%s%d", VIDEO_DEV_NAME, i);
		fd = open(dev_node, O_RDWR | O_NONBLOCK | O_CLOEXEC, 0);
		if (-1 == fd) {
			continue;
		}

		memset(&c, 0, sizeof(c));
		ioctl(fd, VIDIOC_QUERYCAP, &c);
		V4L2_LOG("v4l2 driver: %s", c.driver);
		if (strcmp(c.driver, driver) != 0) {
			close(fd);
			continue;
		}

		break;
	}

	if (-1 != fd)
		V4L2_LOG("open %s success! video:%d", dev_node, fd);
	else
		V4L2_LOG("open %s failed!!!", MTK_MDP_MODULE_NAME);

	return fd;
}

int mdp_close_device(int *fd)
{
	if (-1 < *fd) {
		close(*fd);
		*fd = -1;
	}

	return 0;
}

void video_output_poll(int fd)
{
	int ret;
	struct pollfd pfd;
	struct timeval start, stop, diff;

	if (/* ioctl_performance */ 0)
		gettimeofday(&start, NULL);

	pfd.fd = fd;
	pfd.events = POLLOUT | POLLERR;
	pfd.revents = 0;

	ret = poll(&pfd, 1, -1);

	if (/* ioctl_performance*/ 0) {
		gettimeofday(&stop, NULL);
		/* Print time... */
		timersub(&stop, &start, &diff);
		V4L2_LOG("poll: output(%d), time: %lu.%03lu ms, ret=%d",
			0,
			diff.tv_sec*1000 + diff.tv_usec/1000,
			diff.tv_usec%1000,
			ret);
	}

	//V4L2_LOG("v4l2 output poll: revents=%x(%d)", pfd.revents, pfd.revents);
}

void video_capture_poll(int fd)
{
	int ret;
	struct pollfd pfd;
	struct timeval start, stop, diff;

	if (/* ioctl_performance */ 0)
		gettimeofday(&start, NULL);

	pfd.fd = fd;
	pfd.events = POLLIN | POLLERR;
	pfd.revents = 0;

	ret = poll(&pfd, 1, -1);

	if (/* ioctl_performance */ 0) {
		gettimeofday(&stop, NULL);
		/* Print time... */
		timersub(&stop, &start, &diff);
		V4L2_LOG("poll: capture(%d), time: %lu.%03lu ms, ret=%d",
			0,
			diff.tv_sec*1000 + diff.tv_usec/1000,
			diff.tv_usec%1000,
			ret);
	}

	//V4L2_LOG("v4l2 capture poll: revents=%x(%d)", pfd.revents, pfd.revents);
}


int BlitStreamInit(struct BlitStreamParam *blit)
{
	int ret;

//	blit = malloc(sizeof(struct BlitStreamParam));
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}

	memset(blit, 0, sizeof(struct BlitStreamParam));
	blit->state = MDP_STATE_UNKNOWN;

	ret = mdp_open_device();
	if (ret > 0)
	{
		blit->fd = ret;
		blit->state = MDP_STATE_INIT;
		return 0;
	}
	else
		return ret;
}

void BlitStreamDeinit(struct BlitStreamParam *blit)
{
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return;
	}

	mdp_close_device(&(blit->fd));

	blit->state = MDP_STATE_DEINIT;
	memset(blit, 0, sizeof(struct BlitStreamParam));
	//free(blit);
}

int BlitStreamConfigSrc(
		struct BlitStreamParam *blit,
		unsigned int w, unsigned int h,
		unsigned int pitch, unsigned int fmt,
		unsigned char *(addr[3]), unsigned int mem_type,	
		unsigned int crop_x, unsigned int crop_y, 
		unsigned int crop_w, unsigned int crop_h
		)
{
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	V4L2_LOG("Src: WxH(%dx%d) Crop(%d,%d,%d,%d)", w, h, crop_x, crop_y, crop_w, crop_h);

	blit->src_w = w;
	blit->src_h = h;
	blit->src_fmt = fmt;
	Format2PlaneFmt(blit->src_fmt, blit->src_w, blit->src_h,
			blit->src_plane_fmt, &(blit->src_plane_num), pitch);
	pitch = blit->src_plane_fmt[0].bytesperline;
	blit->src_pitch = pitch;
	blit->src_mem_type = mem_type;
	blit->src_crop_x = crop_x;
	blit->src_crop_y = crop_y;
	blit->src_crop_w = crop_w;
	blit->src_crop_h = crop_h;
	blit->src_size = h * pitch;
	blit->src_addr[0] = addr[0];
	blit->src_addr[1] = addr[1];
	blit->src_addr[2] = addr[2];
	blit->src_buf_type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	blit->state =  MDP_STATE_CONFIG_SRC;

	return 0;
}

int BlitStreamConfigDst(
		struct BlitStreamParam *blit,
		unsigned int w, unsigned int h,
		unsigned int pitch, unsigned int fmt,
		unsigned char *(addr[3]), unsigned int mem_type,	
		unsigned int crop_x, unsigned int crop_y, 
		unsigned int crop_w, unsigned int crop_h,
		unsigned int rotate, unsigned int flip
		)
{
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	V4L2_LOG("Dst: WxH(%dx%d) Crop(%d,%d,%d,%d)", w, h, crop_x, crop_y, crop_w, crop_h);

	blit->dst_w = w;
	blit->dst_h = h;
	blit->dst_fmt = fmt;
	Format2PlaneFmt(blit->dst_fmt, blit->dst_w, blit->dst_h,
			blit->dst_plane_fmt, &(blit->dst_plane_num), pitch);
	pitch = blit->dst_plane_fmt[0].bytesperline;
	blit->dst_pitch = pitch;
	blit->dst_mem_type = mem_type;
	blit->dst_crop_x = crop_x;
	blit->dst_crop_y = crop_y;
	blit->dst_crop_w = crop_w;
	blit->dst_crop_h = crop_h;
	blit->dst_size = h * pitch;
	blit->dst_addr[0] = addr[0];
	blit->dst_addr[1] = addr[1];
	blit->dst_addr[2] = addr[2];
	blit->rotate = rotate;
	blit->flip = flip;
	blit->dst_buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	blit->state = MDP_STATE_CONFIG_DST;

	return 0;
}

int BlitStreamConfigPQ(
		struct BlitStreamParam *blit,
		struct SharpParam sharp,
		struct GammaParam gamma,
		struct DthParam dth,
		struct RszParam rsz,
		unsigned int invert
		)
{
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	blit->gamma = gamma;
	blit->sharp = sharp;
	blit->dth = dth;
	blit->rsz = rsz;
	blit->invert = invert;

#if 0
#define VIDIOC_S_EXT_CTRLS	_IOWR('V', 72, struct v4l2_ext_controls)
/*
 *	C O N T R O L S
 */
struct v4l2_control {
	__u32		     id;
	__s32		     value;
};

struct v4l2_ext_control {
	__u32 id;
	__u32 size;
	__u32 reserved2[1];
	union {
		__s32 value;
		__s64 value64;
		char *string;
		__u8 *p_u8;
		__u16 *p_u16;
		__u32 *p_u32;
		void *ptr;
	};
} __attribute__ ((packed));

struct v4l2_ext_controls {
	union {
#ifndef __KERNEL__
		__u32 ctrl_class;
#endif
		__u32 which;
	};
	__u32 count;
	__u32 error_idx;
	__u32 reserved[2];
	struct v4l2_ext_control *controls;
};
#endif
	struct mtk_mdp_ext_con_param mtk_ext_param;
	struct v4l2_ext_controls ext_cons;
	struct v4l2_ext_control ext_con;

	memset(&ext_cons, 0, sizeof(ext_cons));
	memset(&ext_con, 0, sizeof(ext_con));
	ext_cons.controls = &ext_con;
	ext_cons.count = 1;
	ext_cons.error_idx = 1;
	mtk_ext_param.cmd = MTK_MDP_EXT_CON_SET_SHARPNESS;
	memcpy(&mtk_ext_param.data.sharp, &sharp, sizeof(sharp));
	ext_cons.controls->ptr = &mtk_ext_param;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_EXT_CTRLS, &ext_cons);

	memset(&ext_cons, 0, sizeof(ext_cons));
	memset(&ext_con, 0, sizeof(ext_con));
	ext_cons.controls = &ext_con;
	ext_cons.count = 1;
	ext_cons.error_idx = 1;
	mtk_ext_param.cmd = MTK_MDP_EXT_CON_SET_GAMMA;
	memcpy(&mtk_ext_param.data.gamma, &gamma, sizeof(gamma));
	ext_cons.controls->ptr = &(mtk_ext_param);
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_EXT_CTRLS, &ext_cons);

	memset(&ext_cons, 0, sizeof(ext_cons));
	memset(&ext_con, 0, sizeof(ext_con));
	ext_cons.controls = &ext_con;
	ext_cons.count = 1;
	ext_cons.error_idx = 1;
	mtk_ext_param.cmd = MTK_MDP_EXT_CON_SET_DTH;
	memcpy(&mtk_ext_param.data.dth, &dth, sizeof(dth));
	ext_cons.controls->ptr = &(mtk_ext_param);
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_EXT_CTRLS, &ext_cons);

	memset(&ext_cons, 0, sizeof(ext_cons));
	memset(&ext_con, 0, sizeof(ext_con));
	ext_cons.controls = &ext_con;
	ext_cons.count = 1;
	ext_cons.error_idx = 1;
	mtk_ext_param.cmd = MTK_MDP_EXT_CON_SET_RSZ;
	memcpy(&mtk_ext_param.data.rsz, &rsz, sizeof(rsz));
	ext_cons.controls->ptr = &(mtk_ext_param);
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_EXT_CTRLS, &ext_cons);

	memset(&ext_cons, 0, sizeof(ext_cons));
	memset(&ext_con, 0, sizeof(ext_con));
	ext_cons.controls = &ext_con;
	ext_cons.count = 1;
	ext_cons.error_idx = 1;
	mtk_ext_param.cmd = MTK_MDP_EXT_CON_SET_INVERT;
	mtk_ext_param.data.invert = invert;
	ext_cons.controls->ptr = &(mtk_ext_param);
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_EXT_CTRLS, &ext_cons);

	blit->state = MDP_STATE_CONFIG_PQ;
	return 0;
}

int BlitStreamOn(struct BlitStreamParam *blit)
{
	int i;
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	/*
	 * Source
 	 * VIDIOC_S_FMT
	 * VIDIOC_S_SELECTION
	 * VIDIOC_REQBUFS
	 */
	memset(&(blit->src_format), 0, sizeof(blit->src_format));

	blit->src_format.type = blit->src_buf_type;
	blit->src_format.fmt.pix_mp.width = blit->src_w;
	blit->src_format.fmt.pix_mp.height = blit->src_h;
	blit->src_format.fmt.pix_mp.pixelformat = blit->src_fmt;
	blit->src_format.fmt.pix_mp.num_planes = blit->src_plane_num;
	for (i = 0; i < blit->src_plane_num; ++i) {
		blit->src_format.fmt.pix_mp.plane_fmt[i].bytesperline = blit->src_plane_fmt[i].bytesperline;
	}
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_FMT, &(blit->src_format));

	memset(&(blit->src_sel), 0, sizeof(blit->src_sel));
	blit->src_sel.type = V4L2_BUF_TYPE_VIDEO_OUTPUT; /* buffer type (do not use *_MPLANE types) */
	blit->src_sel.target = V4L2_SEL_TGT_CROP; //V4L2_SEL_TGT_COMPOSE for capture
	blit->src_sel.r.left = blit->src_crop_x;
	blit->src_sel.r.top = blit->src_crop_y;
	blit->src_sel.r.width = blit->src_crop_w;
	blit->src_sel.r.height = blit->src_crop_h;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_SELECTION, &(blit->src_sel));

	memset(&(blit->src_reqbufs), 0, sizeof(blit->src_reqbufs));
	blit->src_reqbufs.count = 1;//64
	blit->src_reqbufs.type = blit->src_buf_type;
	blit->src_reqbufs.memory = blit->src_mem_type;//V4L2_MEMORY_USERPTR V4L2_MEMORY_DMABUF V4L2_MEMORY_MMAP
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_REQBUFS, &(blit->src_reqbufs));

	if (blit->src_mem_type == V4L2_MEMORY_MMAP)
	{
		struct v4l2_buffer buffer;
		struct v4l2_plane planes[VIDEO_MAX_PLANES];

		memset(&buffer, 0, sizeof(buffer));
		memset(planes, 0, sizeof(planes));

		buffer.index = 0;
		buffer.type = blit->src_buf_type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.m.planes = planes;
		buffer.length = 1;
		IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_QUERYBUF, &buffer);
		blit->src_addr[0] = mmap(NULL, planes[0].length, PROT_READ | PROT_WRITE,
				MAP_SHARED, blit->fd, planes[0].m.mem_offset);
	}

	/*
	 * Destination
 	 * VIDIOC_S_FMT
	 * VIDIOC_S_SELECTION
	 * VIDIOC_S_CTRL
	 * VIDIOC_REQBUFS
	 */
#if 1
	struct v4l2_control control;

	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_ROTATE;
	control.value = blit->rotate;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_CTRL, &control);

	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_HFLIP;
	control.value = (blit->flip & 0x2) >> 1;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_CTRL, &control);

	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_VFLIP;
	control.value = blit->flip & 0x1;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_CTRL, &control);

	memset(&control, 0, sizeof(control));
	control.id = V4L2_CID_ALPHA_COMPONENT;
	control.value = 255;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_CTRL, &control);
#endif
	memset(&(blit->dst_format), 0, sizeof(blit->dst_format));

	blit->dst_format.type = blit->dst_buf_type;
	blit->dst_format.fmt.pix_mp.width = blit->dst_w;
	blit->dst_format.fmt.pix_mp.height = blit->dst_h;
	blit->dst_format.fmt.pix_mp.pixelformat = blit->dst_fmt;
	blit->dst_format.fmt.pix_mp.num_planes = blit->dst_plane_num;
	for (i = 0; i < blit->dst_plane_num; ++i) {
		blit->dst_format.fmt.pix_mp.plane_fmt[i].bytesperline = blit->dst_plane_fmt[i].bytesperline;
	}
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_FMT, &(blit->dst_format));

	memset(&(blit->dst_sel), 0, sizeof(blit->dst_sel));
	blit->dst_sel.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /* buffer type (do not use *_MPLANE types) */
	blit->dst_sel.target = V4L2_SEL_TGT_COMPOSE; //V4L2_SEL_TGT_CROP for output
	blit->dst_sel.r.left = blit->dst_crop_x;
	blit->dst_sel.r.top = blit->dst_crop_y;
	blit->dst_sel.r.width = blit->dst_crop_w;
	blit->dst_sel.r.height = blit->dst_crop_h;
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_S_SELECTION, &(blit->dst_sel));

	memset(&(blit->dst_reqbufs), 0, sizeof(blit->dst_reqbufs));
	blit->dst_reqbufs.count = 1;//64
	blit->dst_reqbufs.type = blit->dst_buf_type;
	blit->dst_reqbufs.memory = blit->dst_mem_type;//V4L2_MEMORY_USERPTR V4L2_MEMORY_DMABUF V4L2_MEMORY_MMAP
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_REQBUFS, &(blit->dst_reqbufs));

	if (blit->dst_mem_type == V4L2_MEMORY_MMAP)
	{
		struct v4l2_buffer buffer;
		struct v4l2_plane planes[VIDEO_MAX_PLANES];

		memset(&buffer, 0, sizeof(buffer));
		memset(planes, 0, sizeof(planes));

		buffer.index = 0;
		buffer.type = blit->dst_buf_type;
		buffer.memory = V4L2_MEMORY_MMAP;
		buffer.m.planes = planes;
		buffer.length = 1;
		IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_QUERYBUF, &buffer);
		blit->dst_addr[0] = mmap(NULL, planes[0].length, PROT_READ | PROT_WRITE,
				MAP_SHARED, blit->fd, planes[0].m.mem_offset);
	}

	/*
	 * VIDIOC_STREAMON
	 */
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_STREAMON, &(blit->src_buf_type));
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_STREAMON, &(blit->dst_buf_type));

	blit->state = MDP_STATE_STREAM_ON;
	return 0;
}


int BlitStreamRun(struct BlitStreamParam *blit)
{
	struct v4l2_buffer input_qbuf;
	struct v4l2_plane input_qbuf_planes[VIDEO_MAX_PLANES];
	int i;
	struct timeval start, end;
	long long total_time;
	gettimeofday(&start, NULL);

	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	/*
	 * VIDIOC_QBUF
	 * input
	 */
	memset(&blit->src_qbuffer, 0, sizeof(blit->src_qbuffer));
	memset(blit->src_qplanes, 0, sizeof(blit->src_qplanes));
	if (blit->src_mem_type == V4L2_MEMORY_USERPTR) {
		for (i = 0; i < blit->src_plane_num; i++) {
			blit->src_planes_byteused[i] = blit->src_plane_fmt[i].sizeimage;
			blit->src_planes_length[i] = (blit->src_plane_fmt[i].sizeimage + 127) & ~127;
			blit->src_qplanes[i].bytesused = blit->src_planes_byteused[i];
			blit->src_qplanes[i].length = blit->src_planes_length[i];
			blit->src_qplanes[i].m.userptr = (unsigned long)blit->src_addr[i];
			blit->src_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue input: p[%d]:addr=%p, size=0x%x/0x%x",
				i,
				(void *)blit->src_qplanes[i].m.userptr,
				blit->src_qplanes[i].bytesused,
				blit->src_qplanes[i].length);
		}
	}
	else if (blit->src_mem_type == V4L2_MEMORY_MMAP) {
		for (i = 0; i < blit->src_plane_num; i++) {
			blit->src_qplanes[i].bytesused = blit->src_planes_byteused[i];
			blit->src_qplanes[i].length = blit->src_planes_length[i];
			//TO-DO
			//blit->src_planes[i].m.mem_offset = g_output_buf.buf[index].mem_offset[i];
			blit->src_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue input: p[%d]:mem_offset=%u, size=0x%x/0x%x",
				i,
				blit->src_qplanes[i].m.mem_offset,
				blit->src_qplanes[i].bytesused,
				blit->src_qplanes[i].length);
		}
	}
	else if (blit->src_mem_type == V4L2_MEMORY_DMABUF) {
		for (i = 0; i < blit->src_plane_num; i++) {
			blit->src_qplanes[i].bytesused = blit->src_planes_byteused[i];
			blit->src_qplanes[i].length = blit->src_planes_length[i];
			//TO-DO
			//blit->src_planes[i].m.fd = g_output_buf.buf[index].dmafd[i];
			blit->src_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue input: p[%d]:fd=%d, size=0x%x/0x%x",
				i,
				blit->src_qplanes[i].m.fd,
				blit->src_qplanes[i].bytesused,
				blit->src_qplanes[i].length);
		}
	}

	blit->src_qbuffer.index = 0;
	blit->src_qbuffer.type = blit->src_buf_type;
	blit->src_qbuffer.memory = blit->src_mem_type;
	blit->src_qbuffer.m.planes = blit->src_qplanes;
	blit->src_qbuffer.length = blit->src_plane_num;

	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_QBUF, &(blit->src_qbuffer));

	/*
	 * VIDIOC_QBUF
	 * output
	 */
	memset(&blit->dst_qbuffer, 0, sizeof(blit->dst_qbuffer));
	memset(blit->dst_qplanes, 0, sizeof(blit->dst_qplanes));
	if (blit->dst_mem_type == V4L2_MEMORY_USERPTR) {
		for (i = 0; i < blit->dst_plane_num; i++) {
			int offset;
			offset = blit->dst_crop_y * blit->dst_pitch + blit->dst_crop_x;
			blit->dst_planes_byteused[i] = blit->dst_plane_fmt[i].sizeimage + offset;
			blit->dst_planes_length[i] = (blit->dst_plane_fmt[i].sizeimage + offset + 127) & ~127;
			blit->dst_qplanes[i].bytesused = blit->dst_planes_byteused[i];
			blit->dst_qplanes[i].length = blit->dst_planes_length[i];
			blit->dst_qplanes[i].m.userptr = (unsigned long)blit->dst_addr[i];
			blit->dst_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue output: p[%d]:addr=%p, size=0x%x/0x%x",
				i,
				(void *)blit->dst_qplanes[i].m.userptr,
				blit->dst_qplanes[i].bytesused,
				blit->dst_qplanes[i].length);
		}
	}
	else if (blit->dst_mem_type == V4L2_MEMORY_MMAP) {
		for (i = 0; i < blit->dst_plane_num; i++) {
			blit->dst_qplanes[i].bytesused = blit->dst_planes_byteused[i];
			blit->dst_qplanes[i].length = blit->dst_planes_length[i];
			//TO-DO
			//blit->dst_planes[i].m.mem_offset = g_output_buf.buf[index].mem_offset[i];
			blit->dst_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue output: p[%d]:mem_offset=%u, size=0x%x/0x%x",
				i,
				blit->dst_qplanes[i].m.mem_offset,
				blit->dst_qplanes[i].bytesused,
				blit->dst_qplanes[i].length);
		}
	}
	else if (blit->dst_mem_type == V4L2_MEMORY_DMABUF) {
		for (i = 0; i < blit->dst_plane_num; i++) {
			blit->dst_qplanes[i].bytesused = blit->dst_planes_byteused[i];
			blit->dst_qplanes[i].length = blit->dst_planes_length[i];
			//TO-DO
			//blit->dst_planes[i].m.fd = g_output_buf.buf[index].dmafd[i];
			blit->dst_qplanes[i].data_offset = 0;

			V4L2_LOG("enqueue output: p[%d]:fd=%d, size=0x%x/0x%x",
				i,
				blit->dst_qplanes[i].m.fd,
				blit->dst_qplanes[i].bytesused,
				blit->dst_qplanes[i].length);
		}
	}

	blit->dst_qbuffer.index = 0;
	blit->dst_qbuffer.type = blit->dst_buf_type;
	blit->dst_qbuffer.memory = blit->dst_mem_type;
	blit->dst_qbuffer.m.planes = blit->dst_qplanes;
	blit->dst_qbuffer.length = blit->dst_plane_num;

	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_QBUF, &(blit->dst_qbuffer));

	gettimeofday(&end, NULL);
	total_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	V4L2_LOG("Queue Time is %lld us / %lld ms", total_time, total_time / 1000);

	gettimeofday(&start, NULL);
	/*
	 * VIDIOC_DQBUF
	 * input
	 */
	memset(&(blit->src_dqbuffer), 0, sizeof(blit->src_dqbuffer));
	memset(&(blit->src_dqplanes), 0, sizeof(blit->src_dqplanes));
	blit->src_dqbuffer.type = blit->src_buf_type;
	blit->src_dqbuffer.memory = blit->src_mem_type;
	blit->src_dqbuffer.m.planes = blit->src_dqplanes;
	blit->src_dqbuffer.length = blit->src_plane_num;
	video_output_poll(blit->fd);
	if (video_ioctl(blit->fd, VIDIOC_DQBUF, &(blit->src_dqbuffer)) != 0) {
		V4L2_LOG("failed: VIDIOC_DQBUF input buf");
		return -1;
	} else {
		//set_input_buf_state(dqbuf.index, BUF_DEQUEUED);
	}

	V4L2_LOG("dqbuf input vbbuffer idx=%d, length=%d, used=%d, %d, %d, flags=0x%08x",
		blit->src_dqbuffer.index, blit->src_dqbuffer.length, blit->src_dqbuffer.m.planes[0].bytesused,
		blit->src_dqbuffer.m.planes[1].bytesused, blit->src_dqbuffer.m.planes[2].bytesused,
		blit->src_dqbuffer.flags);


	/*
	 * VIDIOC_DQBUF
	 * output
	 */
	memset(&(blit->dst_dqbuffer), 0, sizeof(blit->dst_dqbuffer));
	memset(&(blit->dst_dqplanes), 0, sizeof(blit->dst_dqplanes));
	blit->dst_dqbuffer.type = blit->dst_buf_type;
	blit->dst_dqbuffer.memory = blit->dst_mem_type;
	blit->dst_dqbuffer.m.planes = blit->dst_dqplanes;
	blit->dst_dqbuffer.length = blit->dst_plane_num;
	video_capture_poll(blit->fd);
	if (video_ioctl(blit->fd, VIDIOC_DQBUF, &(blit->dst_dqbuffer)) != 0) {
		V4L2_LOG("failed: VIDIOC_DQBUF output buf");
		return -1;;
	} else {
		//set_input_buf_state(dqbuf.index, BUF_DEQUEUED);
	}

	V4L2_LOG("dqbuf output vbbuffer idx=%d, length=%d, used=%d, %d, %d, flags=0x%08x",
		blit->dst_dqbuffer.index, blit->dst_dqbuffer.length, blit->dst_dqbuffer.m.planes[0].bytesused,
		blit->dst_dqbuffer.m.planes[1].bytesused, blit->dst_dqbuffer.m.planes[2].bytesused,
		blit->dst_dqbuffer.flags);

	gettimeofday(&end, NULL);
	total_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	V4L2_LOG("Dequeue time is %lld us / %lld ms", total_time, total_time / 1000);

	blit->state = MDP_STATE_STREAM_RUN;
	return 0;
}

int BlitStreamOff(struct BlitStreamParam *blit)
{
	if (blit == NULL)
	{
		V4L2_LOG("BlitStreamParam is NULL");
		return -1;
	}
	if (blit->fd < 0)
	{
		V4L2_LOG("Invalid fd");
		return -2;
	}

	/*
	 * VIDIOC_STREAMOFF
	 */
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_STREAMOFF, &(blit->src_buf_type));
	IOCTL_OR_ERROR_RETURN_FALSE(blit->fd, VIDIOC_STREAMOFF, &(blit->dst_buf_type));

	blit->state = MDP_STATE_STREAM_OFF;

	return 0;
}

unsigned int convert2fmt(int id)
{
	switch (id)
	{
		case 0:
			return V4L2_PIX_FMT_ARGB32;
		case 1:
			return V4L2_PIX_FMT_Y8;
		case 2:
			return V4L2_PIX_FMT_Y4_M0;
		case 3:
			return V4L2_PIX_FMT_Y4_M1;
		case 4:
			return V4L2_PIX_FMT_Y4_M2;
		case 5:
			return V4L2_PIX_FMT_Y2_M0;
		case 6:
			return V4L2_PIX_FMT_Y2_M1;
		case 7:
			return V4L2_PIX_FMT_Y2_M2;
		case 8:
			return V4L2_PIX_FMT_Y2_M3;
		case 9:
			return V4L2_PIX_FMT_Y1_M0;
		case 10:
			return V4L2_PIX_FMT_Y1_M1;
		case 11:
			return V4L2_PIX_FMT_Y1_M2;
		case 12:
			return V4L2_PIX_FMT_Y1_M3;
		default:
			V4L2_LOG("Not support id\n");
			return V4L2_PIX_FMT_Y8;
	}
}


#ifndef __MDP_API__
#define __MDP_API__

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

#include "videodev2.h" /* <linux/videodev2.h> is not latest. */
#define VIDEO_MAX_PLANES               8

struct BlitStreamParam
{
	int fd;
	unsigned int src_w;
	unsigned int src_h;
	unsigned int src_fmt;
	unsigned int src_plane_num;
	unsigned int src_pitch;
	unsigned char *(src_addr[3]);
	unsigned int src_mem_type;
	unsigned int src_buf_type;
	unsigned int src_crop_x;
	unsigned int src_crop_y;
	unsigned int src_crop_w;
	unsigned int src_crop_h;
	unsigned int src_size;
	unsigned int src_planes_byteused[3];
	unsigned int src_planes_length[3];
	struct v4l2_format src_format;
	struct v4l2_selection src_sel;
	struct v4l2_requestbuffers src_reqbufs;
	struct v4l2_buffer src_qbuffer;
	struct v4l2_plane src_qplanes[VIDEO_MAX_PLANES];
	struct v4l2_plane_pix_format src_plane_fmt[VIDEO_MAX_PLANES];
	struct v4l2_plane src_dqplanes[VIDEO_MAX_PLANES];
	struct v4l2_buffer src_dqbuffer;
	unsigned int dst_w;
	unsigned int dst_h;
	unsigned int dst_fmt;
	unsigned int dst_plane_num;
	unsigned int dst_pitch;
	unsigned char *(dst_addr[3]);
	unsigned int dst_mem_type;
	unsigned int dst_buf_type;
	unsigned int dst_crop_x;
	unsigned int dst_crop_y;
	unsigned int dst_crop_w;
	unsigned int dst_crop_h;
	unsigned int dst_size;
	unsigned int dst_planes_byteused[3];
	unsigned int dst_planes_length[3];
	struct v4l2_format dst_format;
	struct v4l2_selection dst_sel;
	struct v4l2_requestbuffers dst_reqbufs;
	struct v4l2_buffer dst_qbuffer;
	struct v4l2_plane dst_qplanes[VIDEO_MAX_PLANES];
	struct v4l2_plane_pix_format dst_plane_fmt[VIDEO_MAX_PLANES];
	struct v4l2_plane dst_dqplanes[VIDEO_MAX_PLANES];
	struct v4l2_buffer dst_dqbuffer;
	unsigned int rotate;
	unsigned int flip;
	int state;
	struct SharpParam sharp;
	struct GammaParam gamma;
	struct DthParam dth;
	struct RszParam rsz;
	int invert;
};

int BlitStreamInit(struct BlitStreamParam *blit);
void BlitStreamDeinit(struct BlitStreamParam *blit);
int BlitStreamConfigSrc(
		struct BlitStreamParam *blit,
		unsigned int w, unsigned int h,
		unsigned int pitch, unsigned int fmt,
		unsigned char *(addr[3]), unsigned int mem_type,	
		unsigned int crop_x, unsigned int crop_y, 
		unsigned int crop_w, unsigned int crop_h
		);
int BlitStreamConfigDst(
		struct BlitStreamParam *blit,
		unsigned int w, unsigned int h,
		unsigned int pitch, unsigned int fmt,
		unsigned char *(addr[3]), unsigned int mem_type,	
		unsigned int crop_x, unsigned int crop_y, 
		unsigned int crop_w, unsigned int crop_h,
		unsigned int rotate, unsigned int flip
		);
int BlitStreamConfigPQ(
		struct BlitStreamParam *blit,
		struct SharpParam sharp,
		struct GammaParam gamma,
		struct DthParam dth,
		struct RszParam rsz,
		unsigned int invert
		);
int BlitStreamOn(struct BlitStreamParam *blit);
int BlitStreamRun(struct BlitStreamParam *blit);
int BlitStreamOff(struct BlitStreamParam *blit);


extern int g_log_level;
#define LOG_TAG "MDP"

#define V4L2_LOG(fmt, ...) \
			do { \
					if (g_log_level >= 3) { \
						printf("[%s] " fmt" @%d\n", LOG_TAG, ##__VA_ARGS__, __LINE__); \
					} \
			} while (0)

#define V4L2_LOG_E(fmt, ...) \
			do { \
						printf("[%s] " fmt" @%d\n", LOG_TAG, ##__VA_ARGS__, __LINE__); \
			} while (0)

struct BITMAPFILEHEADER {
  unsigned short identity; 
  unsigned int file_size; 
  unsigned short reserved1; 
  unsigned short reserved2; 
  unsigned int data_offset; 
};

struct BITMAPINFOHEADER {
  unsigned int header_size; 
  int width; 
  int height; 
  unsigned short planes; 
  unsigned short bit_per_pixel; 
  unsigned int compression; 
  unsigned int data_size; 
  int hresolution; 
  int vresolution; 
  unsigned int used_colors; 
  unsigned int important_colors; 
};

int write_bmp(char *fn, struct BITMAPFILEHEADER bfh, struct BITMAPINFOHEADER bih, unsigned char *buf_addr, unsigned int buf_size, unsigned char format);

#endif

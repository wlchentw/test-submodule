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

int main(int argc, char *argv[])
{
	if (argc < 9)
	{
		V4L2_LOG_E("v4l2mdp s_w s_h s_p s_f d_w d_h d_p d_f");
		return 0;
	}
	struct BlitStreamParam *blit =  malloc(sizeof(struct BlitStreamParam));
	unsigned int s_w = atoi(argv[1]);
	unsigned int s_h = atoi(argv[2]);
	unsigned int s_pitch = atoi(argv[3]);
	unsigned int s_fmt = atoi(argv[4]);
	unsigned char *(s_addr[3]);
	unsigned int s_mem_type = V4L2_MEMORY_USERPTR;
	unsigned int s_crop_x = 0;
	unsigned int s_crop_y = 0;
	unsigned int s_crop_w = s_w;
	unsigned int s_crop_h = s_h;
	unsigned int d_w = atoi(argv[5]);
	unsigned int d_h = atoi(argv[6]);
	unsigned int d_pitch = atoi(argv[7]);
	unsigned int d_fmt = atoi(argv[8]);
	unsigned char *(d_addr[3]);
	unsigned int d_mem_type = V4L2_MEMORY_USERPTR;
	unsigned int d_crop_x = 0;
	unsigned int d_crop_y = 0;
	unsigned int d_crop_w = d_w;
	unsigned int d_crop_h = d_h;
	unsigned int rotate = 0;
	unsigned int flip = 0;

	unsigned char *data_src;
	unsigned char *data_dst;

	s_fmt = convert2fmt(atoi(argv[4]));
	d_fmt = convert2fmt(atoi(argv[8]));

	struct SharpParam sharp;
	struct GammaParam gamma;
	struct DthParam dth;
	struct RszParam rsz;
	unsigned int invert;

	memset(&sharp, 0, sizeof(sharp));
	memset(&gamma, 0, sizeof(gamma));
	memset(&dth, 0, sizeof(dth));
	memset(&rsz, 0, sizeof(rsz));
	invert = 0;

	char src_file[100] = {0};
	char dst_file[100] = {0};

	int loop = 1;
	int loop_index = 1;

	int print_out = 0;

	if (argc > 9)
	{
		int index = 9;
		do {
			switch (argv[index++][0])
			{
				case 'R':
					rotate = atoi(argv[index++]);
					break;
				case 'F':
					flip = atoi(argv[index++]);
					break;
				case 'c':
					{
					int crop_type = atoi(argv[index++]);
					if (crop_type == 0)
					{
						s_crop_x = atoi(argv[index++]);
						s_crop_y = atoi(argv[index++]);
						s_crop_w = atoi(argv[index++]);
						s_crop_h = atoi(argv[index++]);
					}
					else
					{
						d_crop_x = atoi(argv[index++]);
						d_crop_y = atoi(argv[index++]);
						d_crop_w = atoi(argv[index++]);
						d_crop_h = atoi(argv[index++]);
					}
					}
					break;
				case 's':
					sharp.en = atoi(argv[index++]);
					sharp.level = atoi(argv[index++]);
					break;
				case 'g':
					gamma.en = atoi(argv[index++]);
					gamma.type = atoi(argv[index++]);
					{
						int lut_size = gamma.type == 0 ? 256 : 16;
						int i;
						for (i = 0; i < lut_size; i++)
							gamma.lut[i] = lut_size - i - 1;
					}
					break;
				case 'd':
					dth.en = atoi(argv[index++]);
					dth.algo = atoi(argv[index++]);
					break;
				case 'i':
					invert = atoi(argv[index++]);
					break;
				case 'r':
					rsz.algo = atoi(argv[index++]);
					break;
				case 'S':
					memcpy(src_file,argv[index], strlen(argv[index++]));
					break;
				case 'D':
					memcpy(dst_file,argv[index], strlen(argv[index++]));
					break;
				case 'l':
					loop = atoi(argv[index++]);
					g_log_level = 0;
					break;
				case 'p':
					print_out = atoi(argv[index++]);
					break;
				case 'm':
					s_mem_type = atoi(argv[index++]);
					d_mem_type = atoi(argv[index++]);
					if (s_mem_type == 0)
						s_mem_type = V4L2_MEMORY_USERPTR;
					else if (s_mem_type == 1)
						s_mem_type = V4L2_MEMORY_MMAP;
					else if (s_mem_type == 2)
						s_mem_type = V4L2_MEMORY_DMABUF;
					
					if (d_mem_type == 0)
						d_mem_type = V4L2_MEMORY_USERPTR;
					else if (d_mem_type == 1)
						d_mem_type = V4L2_MEMORY_MMAP;
					else if (d_mem_type == 2)
						d_mem_type = V4L2_MEMORY_DMABUF;
					break;
				default:
					V4L2_LOG_E("Unsupport parameter!");
					return;
			}
		} while (index < argc);
	}

	if (s_mem_type == V4L2_MEMORY_USERPTR)
	{
		data_src = malloc(s_pitch * s_h + 4096);
		s_addr[0] = (unsigned char *)(((unsigned long)data_src + 4095) & (~4095));
	}
	else
		s_addr[0] = NULL;

	if (d_mem_type == V4L2_MEMORY_USERPTR)
	{
		data_dst = malloc(d_pitch * d_h + 4096);
		d_addr[0] = (unsigned char *)(((unsigned long)data_dst + 4095) & (~4095));
	}
	else
		d_addr[0] = NULL;
	s_addr[1] = NULL;
	d_addr[1] = NULL;
	s_addr[2] = NULL;
	d_addr[2] = NULL;

	V4L2_LOG("MDP will start ...");

	BlitStreamInit(blit);
	V4L2_LOG("MDP BlitStreamInit done");
	BlitStreamConfigSrc(blit, s_w, s_h, s_pitch, s_fmt, s_addr, s_mem_type, s_crop_x, s_crop_y, s_crop_w, s_crop_h);
	V4L2_LOG("MDP BlitStreamConfigSrc done");
	BlitStreamConfigDst(blit, d_w, d_h, d_pitch, d_fmt, d_addr, d_mem_type, d_crop_x, d_crop_y, d_crop_w, d_crop_h, rotate, flip);
	V4L2_LOG("MDP BlitStreamConfigDst done");
	
	unsigned int s_size, d_size;
	s_size = blit->src_size;
	d_size = blit->dst_size;

	BlitStreamConfigPQ(blit, sharp, gamma, dth, rsz, invert);
	V4L2_LOG("MDP BlitStreamConfigPQ done");

	BlitStreamOn(blit);
	V4L2_LOG("MDP BlitStreamOn done");

	V4L2_LOG("Start draw buffer...");
	if (src_file[0] != 0)
	{
		V4L2_LOG("Read file to buffer start.");
		read_file_to_buffer(src_file, blit->src_addr[0]);
		V4L2_LOG("Read file to buffer done.");
	}
	else
	{
		int i, j;
		for (i = 0; i < s_h; i++)
			for (j = 0; j < s_w; j++)
			{
				if (s_fmt == V4L2_PIX_FMT_ARGB32)
				{
						blit->src_addr[0][4 * (i * s_w + j)] = j % 256;
						blit->src_addr[0][4 * (i * s_w + j) + 1] = j % 256;
						blit->src_addr[0][4 * (i * s_w + j) + 2] = j % 256;
						blit->src_addr[0][4 * (i * s_w + j) + 3] = 0xff;
				}
				else //Y
					blit->src_addr[0][i * s_w + j] = j % 256;
			}
	}
	memset(blit->dst_addr[0], 0, d_pitch * d_h);
	V4L2_LOG("Draw buffer done.");

	struct timeval start, end;
	gettimeofday(&start, NULL);
LOOP:
	BlitStreamRun(blit);

	if (loop_index++ < loop)
		goto LOOP;

	gettimeofday(&end, NULL);
	long long total_time = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
	V4L2_LOG_E("----------------------------------------------------------------------");
	V4L2_LOG_E("Total time is %lld us / %.2f ms", total_time, total_time * 1.0 / 1000);
	V4L2_LOG_E("Tiems:%d, average time is %lld us / %.2f ms", loop, total_time / loop, total_time * 1.0/ 1000 / loop);
	V4L2_LOG_E("----------------------------------------------------------------------");

	V4L2_LOG("MDP BlitStreamRun done");

	if (dst_file[0] != 0)
	{
		V4L2_LOG("Writr buffer to file start.");
		char bmp_file_name[100];
		sprintf(bmp_file_name, "%s.bmp", dst_file);
		struct BITMAPFILEHEADER bfh;
		struct BITMAPINFOHEADER bih;
		bfh.identity = 0x4d42;
		bfh.file_size = blit->dst_h * blit->dst_pitch + 54;
		bfh.reserved1 = 0;
		bfh.reserved2 = 0;
		bfh.data_offset = 54;
		bih.header_size = 40;
		bih.width = blit->dst_w;
		bih.height = blit->dst_h;
		bih.planes = 1;
		bih.bit_per_pixel = 8;
		bih.compression = 0;
		bih.data_size = blit->dst_h * blit->dst_pitch;
		bih.hresolution = 0;
		bih.vresolution = 0;
		bih.used_colors = 0;
		bih.important_colors = 0;

		write_bmp(bmp_file_name, bfh, bih, blit->dst_addr[0], bih.data_size, blit->dst_fmt);
		write_buffer_to_file(dst_file, blit->dst_addr[0], blit->dst_h * blit->dst_pitch);
		V4L2_LOG("Writr buffer to file done.");
	}

	if (print_out){
		int i;
		for (i = 0; i < 128; i++)
		{
			if (i % 32 == 0) printf("\n");
			printf("%x-%x ", blit->src_addr[0][i], blit->dst_addr[0][i]);
		}
		printf("\n----------------------------------------------------------------------\n");
		for (i = 128; i > 0; i--)
		{
			if (i % 32 == 0) printf("\n");
			printf("%x-%x ", blit->src_addr[0][s_size - i], blit->dst_addr[0][d_size - i]);
		}
		printf("\n----------------------------------------------------------------------\n");
	}

	BlitStreamOff(blit);
	V4L2_LOG("MDP BlitStreamOff done");

	if (s_mem_type == V4L2_MEMORY_USERPTR)
		free(data_src);
	if (d_mem_type == V4L2_MEMORY_USERPTR)
		free(data_dst);

	BlitStreamDeinit(blit);
	V4L2_LOG("MDP BlitStreamDeinit done");

	free(blit);
	V4L2_LOG("MDP End");

	return 0;
}

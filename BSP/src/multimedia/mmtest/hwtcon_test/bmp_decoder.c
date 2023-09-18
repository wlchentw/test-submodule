// SPDX-License-Identifier: MediaTekProprietary
#include <stdio.h>
#include <stdlib.h>
#include "bmp_decoder.h"

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
        uint16 bfType;
        uint32 bfSize;
        uint16 bfReserved1;
        uint16 bfReserved2;
        uint32 bfOffBits;
} BITMAPFILEHEADER;
#pragma pack()

#pragma pack(2)
typedef struct tagBITMAPINFOHEADER{
        uint32 biSize;
        uint32 biWidth;
        uint32 biHeight;
        uint16 biPlanes;
        uint16 biBitCount;
        uint32 biCompression;
        uint32 biSizeImage;
        uint32 biXPelsPerMeter;
        uint32 biYPelsPerMeter;
        uint32 biClrUsed;
		uint32 biClrImportant;
} BITMAPINFOHEADER;
#pragma pack()

enum clr_fmt_id {
	COLOR_RGB,
	COLOR_BGR,
	COLOR_ARGB,
	COLOR_ABGR,
	COLOR_RGBA,
	COLOR_BGRA,
};

char *get_fmt_name(enum clr_fmt_id fmt_id)
{
	switch (fmt_id){
	case COLOR_RGB:
		return "RGB";
	case COLOR_BGR:
		return "BGR";
	case COLOR_ARGB:
		return "ARGB";
	case COLOR_ABGR:
		return "ABGR";
	case COLOR_RGBA:
		return "RGBA";
	case COLOR_BGRA:
		return "BGRA";
	}
}

void dump_bmp_file_header(BITMAPFILEHEADER *bitmapfileheader)
{
	printf("bitmapfileheader:\n");
	printf("bfType %c%c\n", (bitmapfileheader->bfType>>8), bitmapfileheader->bfType);
	printf("bfSize %d\n", bitmapfileheader->bfSize);
	printf("bfReserved1 %d\n", bitmapfileheader->bfReserved1);
	printf("bfReserved2 %d\n", bitmapfileheader->bfReserved2);
	printf("bfOffBits %d\n", bitmapfileheader->bfOffBits);
}

void dump_bmp_info_header(BITMAPINFOHEADER *bitmapinfoheader)
{
	printf("bitmapinfoheader:\n");

	printf("sizeof uint8 %zu:\n", sizeof(uint8));
	printf("sizeof uint16 %zu:\n", sizeof(uint16));
	printf("sizeof uint32 %zu:\n", sizeof(uint32));
	printf("sizeof uint64 %zu:\n", sizeof(uint64));

	printf("biSize %d\n", bitmapinfoheader->biSize);
	printf("biWidth %d\n", bitmapinfoheader->biWidth);
	printf("biHeight %d\n", bitmapinfoheader->biHeight);
	printf("biPlanes %d\n", bitmapinfoheader->biPlanes);
	printf("biBitCount %d\n", bitmapinfoheader->biBitCount);
	printf("biCompression %d\n", bitmapinfoheader->biCompression);

	printf("biSizeImage %d\n", bitmapinfoheader->biSizeImage);
	printf("biClrUsed %d\n", bitmapinfoheader->biClrUsed);

	printf("biClrImportant %d\n", bitmapinfoheader->biClrImportant);
}

void WriteBMP(const char *fileName,
	unsigned char *pInputR,
	unsigned char *pInputG,
	unsigned char *pInputB,
	unsigned long PicWidth,
	unsigned long PicHeight)
{
	BITMAPFILEHEADER bitmapfileheader = {0};
	BITMAPINFOHEADER bitmapinfoheader = {0};

	FILE *fp;

	unsigned long x,y;
	unsigned long appendCnt;

	// Initialize bit map file header
	bitmapfileheader.bfType = 'MB';
	bitmapfileheader.bfSize = sizeof(bitmapfileheader) + sizeof(bitmapinfoheader) + PicWidth*PicHeight*3;
	bitmapfileheader.bfReserved1 = 0;
	bitmapfileheader.bfReserved2 = 0;
	bitmapfileheader.bfOffBits = sizeof(bitmapfileheader) + sizeof(bitmapinfoheader);

	// Initialize bit map info header
	bitmapinfoheader.biSize = sizeof(bitmapinfoheader);
	bitmapinfoheader.biWidth = PicWidth;
	bitmapinfoheader.biHeight = PicHeight;
	bitmapinfoheader.biPlanes = 1;
	bitmapinfoheader.biBitCount = 24;
	bitmapinfoheader.biCompression = 0;
	bitmapinfoheader.biSizeImage = PicWidth*PicHeight*3;
	bitmapinfoheader.biClrUsed = 0;
	bitmapinfoheader.biClrImportant = 0;


	printf("sizeof(bitmapfileheader) = %zu\n", sizeof(bitmapfileheader));

	printf("sizeof(bitmapinfoheader) = %zu\n",sizeof(bitmapinfoheader));

	if((fp = fopen(fileName,"wb")) != NULL) {
		fwrite(&bitmapfileheader,sizeof(bitmapfileheader),1,fp);
		fwrite(&bitmapinfoheader,sizeof(bitmapinfoheader),1,fp);

		for(y=0; y<PicHeight; y++) {
			for(x=0; x<PicWidth; x++) {
				fputc(pInputB[(PicHeight - 1 - y)*PicWidth + x],fp);
				fputc(pInputG[(PicHeight - 1 - y)*PicWidth + x],fp);
				fputc(pInputR[(PicHeight - 1 - y)*PicWidth + x],fp);
			}

			// One line should align with 16byte
			if(((PicWidth*3) % 4) != 0) {
				appendCnt = 4 - ((PicWidth*3) % 4);
				for(x=0; x<appendCnt; x++)
				fputc(0,fp);
			}
		}

		fclose(fp);
	} else {
		printf("Open file BMP fail.\n");
	}
}

int Read_BMP_Resolution(const char *fileName, int *PicWidth, int *PicHeight)
{
	BITMAPFILEHEADER bitmapfileheader;
	BITMAPINFOHEADER bitmapinfoheader;
	int read_count = 0;

	FILE *fp = fopen(fileName,"rb");

	if (fp == NULL) {
		printf("open file %s fail\n", fileName);
		return -1;
	}

	if (fread(&bitmapfileheader,sizeof(bitmapfileheader),1,fp) != 1) {
		printf("read file header fail\n");
		fclose(fp);
		return -1;
	}
	if (fread(&bitmapinfoheader,sizeof(bitmapinfoheader),1,fp) != 1) {
		printf("read file info fail\n");
		fclose(fp);
		return -1;
	}

	*PicWidth = bitmapinfoheader.biWidth;
	*PicHeight = bitmapinfoheader.biHeight;
	fclose(fp);
	return 0;
}

int ReadBMP_Y(const char *fileName, unsigned char *pInputY)
{
	BITMAPFILEHEADER bitmapfileheader;
	BITMAPINFOHEADER bitmapinfoheader;
	FILE *fp;

	unsigned long x,y;
	int i = 0;
	unsigned long appendCnt;
	int width;
	int height;
	int tmp_B, tmp_G, tmp_R, tmp_A, tmp_allign;

	printf("sizeof(bitmapfileheader) = %zu\n",sizeof(bitmapfileheader));
	printf("sizeof(bitmapinfoheader) = %zu\n", sizeof(bitmapinfoheader));

	if((fp = fopen(fileName,"rb")) == NULL) {
		printf("Open file BMP fail.\n");
		return -1;
	}
	fread(&bitmapfileheader,sizeof(bitmapfileheader),1,fp);
	fread(&bitmapinfoheader,sizeof(bitmapinfoheader),1,fp);

	// dump_bmp_file_header(&bitmapfileheader);
	// dump_bmp_info_header(&bitmapinfoheader);

	width = bitmapinfoheader.biWidth;
	height = bitmapinfoheader.biHeight;

	printf("bit counter:%d\n", bitmapinfoheader.biBitCount);

	fseek(fp, bitmapfileheader.bfOffBits, SEEK_SET);
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			switch (bitmapinfoheader.biBitCount) {
			case 8:
				tmp_R = fgetc(fp);
				pInputY[(height - 1 - y)*width + x] = tmp_R;
				// One line should align with 16byte
				if((width % 4) != 0) {
					appendCnt = 4 - (width % 4);

					for(i = 0; i < appendCnt; i++)
						tmp_allign = fgetc(fp);
				}
				break;
			case 24:
				tmp_B = fgetc(fp);
				tmp_G = fgetc(fp);
				tmp_R = fgetc(fp);

				if (tmp_B == 0 &&
					tmp_G == 0 &&
					tmp_R == 0)
					pInputY[(height - 1 - y)*width + x] = 0;
				else
					pInputY[(height - 1 - y)*width + x] = 0.2568 * tmp_R + 0.504 * tmp_G + 0.1237 * tmp_B + 16;
				// One line should align with 16byte
				if(((width * 3) % 4) != 0) {
					appendCnt = 4 - ((width * 3) % 4);

					for(i = 0; i < appendCnt; i++)
						tmp_allign = fgetc(fp);
				}
				break;
			case 32:
				tmp_B = fgetc(fp);
				tmp_G = fgetc(fp);
				tmp_R = fgetc(fp);
				tmp_A = fgetc(fp);

				pInputY[(height - 1 - y)*width + x] = 0.2568 * tmp_R + 0.504 * tmp_G + 0.1237 * tmp_B + 16;
				break;
			default:
				printf("invalid bitCount:%d\n", bitmapinfoheader.biBitCount);
				fclose(fp);
				return -1;
			}
		}
	}

	fclose(fp);
	return 0;
}
int ReadBMP_RGB(const char *fileName,
               unsigned char *pInputR,
               unsigned char *pInputG,
               unsigned char *pInputB)
{
	BITMAPFILEHEADER bitmapfileheader;
	BITMAPINFOHEADER bitmapinfoheader;
	FILE *fp;

	unsigned long x,y;
	int i = 0;
	unsigned long appendCnt;
	int width;
	int height;
	int tmp, tmp_B, tmp_G, tmp_R, tmp_A, tmp_allign;

	if((fp = fopen(fileName,"rb")) == NULL) {
		printf("Open file BMP %s fail.\n", fileName);
		return -1;
	}
	fread(&bitmapfileheader,sizeof(bitmapfileheader),1,fp);
	fread(&bitmapinfoheader,sizeof(bitmapinfoheader),1,fp);

#if 0
	printf("sizeof(bitmapfileheader) = %zu\n",sizeof(bitmapfileheader));
	printf("sizeof(bitmapinfoheader) = %zu\n", sizeof(bitmapinfoheader));
	dump_bmp_file_header(&bitmapfileheader);
	dump_bmp_info_header(&bitmapinfoheader);
#endif

	width = bitmapinfoheader.biWidth;
	height = bitmapinfoheader.biHeight;

	fseek(fp, bitmapfileheader.bfOffBits, SEEK_SET);
	for(y=0; y<height; y++) {
		for(x=0; x<width; x++) {
			switch (bitmapinfoheader.biBitCount) {
			case 8:
				tmp = fgetc(fp);
				pInputR[(height - 1 - y)*width + x] = tmp;
				pInputG[(height - 1 - y)*width + x] = tmp;
				pInputB[(height - 1 - y)*width + x] = tmp;
				// One line should align with 16byte
				if((width % 4) != 0) {
					appendCnt = 4 - (width % 4);

					for(i = 0; i < appendCnt; i++)
						tmp_allign = fgetc(fp);
				}
				break;
			case 24:
				tmp_B = fgetc(fp);
				tmp_G = fgetc(fp);
				tmp_R = fgetc(fp);

				pInputR[(height - 1 - y)*width + x] = tmp_R;
				pInputG[(height - 1 - y)*width + x] = tmp_G;
				pInputB[(height - 1 - y)*width + x] = tmp_B;
				// One line should align with 16byte
				if(((width * 3) % 4) != 0) {
					appendCnt = 4 - ((width * 3) % 4);

					for(i = 0; i < appendCnt; i++)
						tmp_allign = fgetc(fp);
				}
				break;
			case 32:
				tmp_B = fgetc(fp);
				tmp_G = fgetc(fp);
				tmp_R = fgetc(fp);
				tmp_A = fgetc(fp);

				pInputR[(height - 1 - y)*width + x] = tmp_R;
				pInputG[(height - 1 - y)*width + x] = tmp_G;
				pInputB[(height - 1 - y)*width + x] = tmp_B;
				break;
			default:
				printf("invalid bitCount:%d\n", bitmapinfoheader.biBitCount);
				fclose(fp);
				return -1;
			}
		}
	}

	fclose(fp);
	return 0;
}


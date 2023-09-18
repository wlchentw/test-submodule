#ifndef __BMP_DECODER_H__
#define __BMP_DECODER_H__
enum COLOR_FORMAT {
    COLOR_FORMAT_YUV420_BLOCK = 0,
    COLOR_FORMAT_YUV420_10BIT_BLOCK = 1,

    COLOR_FORMAT_YUV422_YUYV = 10,

    COLOR_FORMAT_YUV444 = 20,

    COLOR_FORMAT_RGB888_RGB = 30,

};

void WriteBMP(const char *fileName,
               unsigned char *pInputR,
               unsigned char *pInputG,
               unsigned char *pInputB,
               unsigned long PicWidth,
               unsigned long PicHeight);
int ReadBMP_RGB(const char *fileName,
               unsigned char *pInputR,
               unsigned char *pInputG,
               unsigned char *pInputB);
int ReadBMP_Y(const char *fileName, unsigned char *pInputY);
int Read_BMP_Resolution(const char *fileName, int *PicWidth, int *PicHeight);

#endif // __BMP_DECODER_H__

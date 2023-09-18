/***************************************************
 * EPD frame buffer device content manager (4 bits).
 * 
 * Author : Gallen Lin 
 * Data : 2011/01/20 
 * Revision : 1.0
 * File Name : epdfb_dc.c 
 * 
****************************************************/

#include "epdfb_dc.h"




#ifdef __KERNEL__//[
	#include <linux/module.h>
	#include <linux/kernel.h>
	#include <linux/errno.h>
	#include <linux/string.h>
	#include <linux/delay.h>
	#include <linux/interrupt.h>
	#include <linux/fb.h>
	#include <linux/byteorder/generic.h>
	
	#define GDEBUG 1
	#include <linux/gallen_dbg.h>
	#define my_malloc(sz)	kmalloc(sz,GFP_KERNEL)
	#define my_free(p)		kfree(p)
	#define my_memcpy(dest,src,size)	memcpy(dest,(const unsigned char *)src,size)

	#define _be32_to_cpu be32_to_cpu
	#define _cpu_to_be32 cpu_to_be32
#else //][!__KERNEL__
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <endian.h>
	#define GDEBUG 0
	#include "gallen_dbg.h"
	#define my_malloc(sz)	(void *)malloc(sz)
	#define my_free(p)		free(p)
	#define my_memcpy(dest,src,size)	memcpy(dest,(const void *)src,size)
	#define _be32_to_cpu be32toh
	#define _cpu_to_be32 htobe32
#endif//] __KERNEL__

#define MAX(a,b)	((a>b)?a:b)
#define MIN(a,b)	((a<b)?a:b)

#define EPDFB_MAGIC 	0x15a815a8

//#define debug_pts 12 // debug pixels information 
#define MIRROR_FROM_SRC	1

#define CHK_EPDFB_DC(pEPD_DC)	\
(pEPD_DC->dwMagicPrivateBegin==EPDFB_MAGIC)&&(pEPD_DC->dwMagicPrivateEnd==EPDFB_MAGIC)

static const int giDither8[16][16] = {
  {   1,235, 59,219, 15,231, 55,215,  2,232, 56,216, 12,228, 52,212},
  { 129, 65,187,123,143, 79,183,119,130, 66,184,120,140, 76,180,116},
  {  33,193, 17,251, 47,207, 31,247, 34,194, 18,248, 44,204, 28,244},
  { 161, 97,145, 81,175,111,159, 95,162, 98,146, 82,172,108,156, 92},
  {   9,225, 49,209,  5,239, 63,223, 10,226, 50,210,  6,236, 60,220},
  { 137, 73,177,113,133, 69,191,127,138, 74,178,114,134, 70,188,124},
  {  41,201, 25,241, 37,197, 21,255, 42,202, 26,242, 38,198, 22,252},
  { 169,105,153, 89,165,101,149, 85,170,106,154, 90,166,102,150, 86},
  {   3,233, 57,217, 13,229, 53,213,  0,234, 58,218, 14,230, 54,214},
  { 131, 67,185,121,141, 77,181,117,128, 64,186,122,142, 78,182,118},
  {  35,195, 19,249, 45,205, 29,245, 32,192, 16,250, 46,206, 30,246},
  { 163, 99,147, 83,173,109,157, 93,160, 96,144, 80,174,110,158, 94},
  {  11,227, 51,211,  7,237, 61,221,  8,224, 48,208,  4,238, 62,222},
  { 139, 75,179,115,135, 71,189,125,136, 72,176,112,132, 68,190,126},
  {  43,203, 27,243, 39,199, 23,253, 40,200, 24,240, 36,196, 20,254},
  { 171,107,155, 91,167,103,151, 87,168,104,152, 88,164,100,148, 84} 
};


#define EINK_CFA_GEN4_W		3
#define EINK_CFA_GEN4_H		3
#define RGBA_R	0
#define RGBA_G	1
#define RGBA_B	2
static const char gcCFA_gen4_rgba_maskA[EINK_CFA_GEN4_H][EINK_CFA_GEN4_W] = {
	{RGBA_R,RGBA_B,RGBA_G},
	{RGBA_G,RGBA_R,RGBA_B},
	{RGBA_B,RGBA_G,RGBA_R},
};
static const char gcCFA_gen4_2_rgba_maskA[EINK_CFA_GEN4_H][EINK_CFA_GEN4_W] = {
	{RGBA_B,RGBA_R,RGBA_G},
	{RGBA_R,RGBA_G,RGBA_B},
	{RGBA_G,RGBA_B,RGBA_R},
};

#define ARGB_R	1
#define ARGB_G	2
#define ARGB_B	3
static const char gcCFA_gen4_argb_maskA[EINK_CFA_GEN4_H][EINK_CFA_GEN4_W] = {
	{ARGB_R,ARGB_B,ARGB_G},
	{ARGB_G,ARGB_R,ARGB_B},
	{ARGB_R,ARGB_G,ARGB_R},
};


static void _fb_gray_8to4(unsigned char *data, DWORD len)
{
	unsigned char *pb = (unsigned char *)data ;
	DWORD dwByteRdIdx,dwByteWrIdx;
	unsigned char bTemp;
	DWORD dwLenNew = len;

	DBG_MSG("%s(%d):data=%p,len=%d\n",\
			__FUNCTION__,__LINE__,data,len);

	for(dwByteRdIdx=0,dwByteWrIdx=0;1;)
	{
		bTemp = (pb[dwByteRdIdx]>>4)&0x0f;
		bTemp = bTemp<<4;
		if(++dwByteRdIdx>=len){
			break;
		}
		bTemp |= (pb[dwByteRdIdx]>>4)&0x0f;
		dwLenNew-=1;
		pb[++dwByteWrIdx] = bTemp;

		if(++dwByteRdIdx>=len) {
			break;
		}
	}

}

// get blue value to gray value .
#if 1
#define _RGB565_to_Gray8(_wRGB_565)	(((unsigned char)(_wRGB_565)&0x001f)<<3)
#define _RGB565_to_Gray4(_wRGB_565)	(((unsigned char)(_wRGB_565)&0x001f)>>1)
#else
#define _RGB565_to_Gray8(_wRGB_565)	\
	(unsigned char)(( \
		(((DWORD)((unsigned char)(_wRGB_565)&0xf800)>>11)*299) + \
	  (((DWORD)((unsigned char)(_wRGB_565)&0x07e0)>>5)*587) + \
	   ((DWORD)((unsigned char)(_wRGB_565)&0x001f)*114) ) >> 8 )


#define _RGB565_to_Gray4(_wRGB_565)	\
	_RGB565_to_Gray8(_wRGB_565)>>4
#endif
#define _RGB565_to_RGB888(_wRGB_565)({\
	DWORD dwR,dwG,dwB;\
	DWORD dwRGB888;\
	dwB=((_wRGB_565)&0x1f);\
	dwG=((_wRGB_565>>6)&0x3f);\
	dwR=((_wRGB_565>>11)&0x1f);\
	dwRGB888=dwR<<16|dwG<<8|dwB;\
	dwRGB888;\
})
	
#define _RGB565_to_RGBW8888(_wRGB_565)	({\
	DWORD dwR,dwG,dwB,dwT;\
	DWORD dwRGB8888;\
	dwB=((_wRGB_565)&0x1f);\
	dwG=((_wRGB_565>>6)&0x3f);\
	dwR=((_wRGB_565>>11)&0x1f);\
	dwT=0;\
	dwRGB8888=dwR<<24|dwG<<16|dwB<<8|dwT;\
	dwRGB8888;\
})

#define _RGB565(_r,_g,_b)	(unsigned short)((_r)<<11|(_g)<<6|(_b))
#define _RGB888_to_RGB565(pbRGB_888)({\
	unsigned short wR,wG,wB;\
	unsigned short wRGB565;\
	wB=(unsigned short)((*(pbRGB_888+0))>>3);\
	wG=(unsigned short)((*(pbRGB_888+1))>>2);\
	wR=(unsigned short)((*(pbRGB_888+2))>>3);\
	wRGB565=wR<<11|wG<<5|wB;\
	wRGB565;\
})

static const unsigned short gwGray4toRGB565_TableA[] = {
	_RGB565(0x00,0x00,0x00),
	_RGB565(0x02,0x02,0x02),
	_RGB565(0x04,0x04,0x04),
	_RGB565(0x06,0x06,0x06),
	_RGB565(0x08,0x08,0x08),
	_RGB565(0x0a,0x0a,0x0a),
	_RGB565(0x0c,0x0c,0x0c),
	_RGB565(0x0e,0x0e,0x0e),
	_RGB565(0x10,0x10,0x10),
	_RGB565(0x12,0x12,0x12),
	_RGB565(0x14,0x14,0x14),
	_RGB565(0x16,0x16,0x16),
	_RGB565(0x18,0x18,0x18),
	_RGB565(0x1a,0x1a,0x1a),
	_RGB565(0x1c,0x1c,0x1c),
	_RGB565(0x1e,0x1e,0x1e),
};
static const unsigned short gwGray2toRGB565_TableA[] = {
	_RGB565(0x00,0x00,0x00),
	_RGB565(0x0a,0x0a,0x0a),
	_RGB565(0x14,0x14,0x14),
	_RGB565(0x1e,0x1e,0x1e),
};

static const unsigned char gbGray4toGray8_TableA[] = {
	0x00,
	0x10,
	0x20,
	0x30,
	0x40,
	0x50,
	0x60,
	0x70,
	0x80,
	0x90,
	0xa0,
	0xb0,
	0xc0,
	0xd0,
	0xe0,
	0xf0,
};

static void _fb_Gray4toRGB565(
	unsigned char *IO_pbRGB565Buf,DWORD I_dwRGB565BufSize,
	unsigned char *I_pb4BitsSrc,DWORD I_dw4BitsBufSize,int iIsPixelSwap)
{
	DWORD dwRd;
	unsigned short *pwWrBuf = (unsigned short *)IO_pbRGB565Buf;
	unsigned char *pbRdSrc = I_pb4BitsSrc;
	
	int iIdxDot1,iIdxDot2;
	
	
	for(dwRd=0;dwRd<I_dw4BitsBufSize;dwRd++)
	{
		
		if(iIsPixelSwap) {
			iIdxDot1 = (pbRdSrc[dwRd]>>4)&0xf;
			iIdxDot2 = (pbRdSrc[dwRd])&0xf;
		}
		else {
			iIdxDot1 = (pbRdSrc[dwRd])&0xf;
			iIdxDot2 = (pbRdSrc[dwRd]>>4)&0xf;
		}
		
		*pwWrBuf++ = gwGray4toRGB565_TableA[iIdxDot1];
		*pwWrBuf++ = gwGray4toRGB565_TableA[iIdxDot2];
	}
}

static void _fb_Gray8toRGB565(
	unsigned char *IO_pbRGB565Buf,DWORD I_dwRGB565BufSize,
	unsigned char *I_pb8BitsSrc,DWORD I_dw8BitsBufSize)
{
	DWORD dwRd,dwWr=0;
	unsigned short *pwWrBuf = (unsigned short *)IO_pbRGB565Buf;
	unsigned char *pbRdSrc = I_pb8BitsSrc;
	
	
	
	for(dwRd=0;dwRd<I_dw8BitsBufSize;dwRd++)
	{
		*pwWrBuf++ = (*pbRdSrc++)<<8;
		if(++dwWr>=I_dwRGB565BufSize) {
			ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
			break;
		}
	}
}

static void _fb_RGB565toGray4(
	unsigned char *IO_pbGray4Buf,DWORD I_dwGray4BufSize,
	unsigned char *I_pbRGB565Buf,DWORD I_dwRGB565BufSize,int iIsPixelSwap)
{
	DWORD dwRd,dwWr = 0;
	unsigned char *pbWrBuf = (unsigned char *)IO_pbGray4Buf;
	unsigned short *pwRdSrc = (unsigned short *)I_pbRGB565Buf;
	
	unsigned char bDot1,bDot2;
	
	ASSERT(!(I_dwRGB565BufSize&1));
	for(dwRd=0;dwRd<I_dwRGB565BufSize;dwRd+=2)
	{
		bDot1 = (*pwRdSrc) >> 12 ;
		pwRdSrc ++ ;
		bDot2 = (*pwRdSrc) >> 12 ;
		pwRdSrc ++ ;
		
		if(iIsPixelSwap) {
			*pbWrBuf = bDot2&(bDot1<<4) ;
		}
		else {
			*pbWrBuf = (bDot2<<4)&bDot1 ;
		}
		pbWrBuf++;
		if(++dwWr >=  I_dwGray4BufSize) {
			ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
			break;
		}
	}
}


static void _fb_gray_4to8(
	unsigned char *IO_pb8BitsBuf,DWORD I_dw8BitsBufSize,
	unsigned char *I_pb4BitsSrc,DWORD I_dw4BitsBufSize,int iIsPixelSwap)
{
	DWORD dwWr,dwRd;
	
	for(dwWr=0,dwRd=0;dwRd<I_dw4BitsBufSize;dwRd++)
	{
		if(iIsPixelSwap) {
			if(dwWr>=I_dw8BitsBufSize) {
				ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
				break;
			}
			IO_pb8BitsBuf[dwWr++] = (I_pb4BitsSrc[dwRd]<<4)&0xf0;
			
			if(dwWr>=I_dw8BitsBufSize) {
				ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
				break;
			}
			IO_pb8BitsBuf[dwWr++] = I_pb4BitsSrc[dwRd]&0xf0;
		}
		else {
			if(dwWr>=I_dw8BitsBufSize) {
				ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
				break;
			}
			IO_pb8BitsBuf[dwWr++] = I_pb4BitsSrc[dwRd]&0xf0;
			
			if(dwWr>=I_dw8BitsBufSize) {
				ERR_MSG("%s(%d):write buffer too small \n",__FUNCTION__,__LINE__);
				break;
			}
			IO_pb8BitsBuf[dwWr++] = (I_pb4BitsSrc[dwRd]<<4)&0xf0;
		}
	}
}

EPDFB_DC *epdfbdc_create_ex2(DWORD dwFBW,DWORD dwFBH,\
	DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf,DWORD dwCreateFlag)
{
	EPDFB_DC *pDC = 0;
	DWORD dwFBBits = (dwFBW*dwFBH*bPixelBits);
	DWORD dwFBWBits = (dwFBW*bPixelBits);
	DWORD dwDCSize ,dwDCWidthBytes;

	dwDCSize=dwFBBits>>3;
	dwDCWidthBytes=dwFBWBits>>3;
	
	if(dwFBWBits&0x7) {
		dwDCWidthBytes+=1;
		dwDCSize+=dwFBH;
	}
	
	DBG_MSG("%s(%d):w=%d,h=%d,bits=%d,bufp=%p\n",__FUNCTION__,__LINE__,\
		dwW,dwH,bPixelBits,pbDCbuf);
	
	pDC = (EPDFB_DC *)my_malloc(sizeof(EPDFB_DC));
	if(0==pDC) {
		return 0;
	}

	
	if(pbDCbuf) {
		pDC->dwBufSize = 0;
		pDC->pbDCbuf = pbDCbuf;
		
		DBG_MSG("%s(%d):pDC malloc=%p buf assign @ %p \n",
			__FUNCTION__,__LINE__,pDC,pbDCbuf);
	}
	else {
		pbDCbuf = (unsigned char *)my_malloc(dwDCSize);
		if(0==pbDCbuf) {
			my_free(pDC);
			return 0;
		}
		DBG_MSG("%s(%d):pDC malloc=%p,pbDCbuf=%p,sz=%u\n",
			__FUNCTION__,__LINE__,pDC,pbDCbuf,dwDCSize);
		memset(pbDCbuf,0,dwDCSize);
		pDC->pbDCbuf = pbDCbuf;
		pDC->dwBufSize = dwDCSize;
	}
	ASSERT(dwFBW>=dwW);
	pDC->dwFBWExtra = dwFBW-dwW ;
	ASSERT(dwFBH>=dwH);
	pDC->dwFBHExtra = dwFBH-dwH ;
	pDC->dwWidth = dwW;
	pDC->dwHeight = dwH;
	pDC->dwFBXOffset = 0;
	pDC->dwFBYOffset = 0;

	{
		int maxval = 255;
		int row,col;

		switch(bPixelBits) {
		case 1:
			maxval = 1;
			break;
		case 2:
			maxval = 3;
			break;
		case 4:
			maxval = 15;
			break;
		case 8:
			maxval = 255;
			break;
		case 16:
			maxval = 31;
			break;
		case 32:
			maxval = 255;
			break;
		}
		//printk("D8 Array[][] = {");
		for ( row = 0; row < 16; ++row ) {
			//printk("{");
			for ( col = 0; col < 16; ++col ) {
				pDC->dither8[row][col] = giDither8[row][col] * ( maxval + 1 ) / 256;
				//printk("%d,",pDC->dither8[row][col]);
			}
			//printk("}\n");
		}
		//printk("\n");
	}
	

	pDC->bPixelBits = bPixelBits;
	pDC->dwDCSize = dwDCSize;
	
	pDC->dwDCWidthBytes = dwDCWidthBytes;
	
	pDC->dwDirtyOffsetStart = 0;
	pDC->dwDirtyOffsetEnd = dwDCSize;
	
	pDC->dwMagicPrivateBegin = EPDFB_MAGIC;
	pDC->dwMagicPrivateEnd = EPDFB_MAGIC;
	pDC->dwFlags = dwCreateFlag;
	
	pDC->pfnGetWaveformBpp = 0;
	pDC->pfnVcomEnable = 0;
	pDC->pfnSetPartialUpdate = 0;
	pDC->pfnGetRealFrameBuf = 0;
	pDC->pfnGetRealFrameBufEx = 0;
	pDC->pfnDispStart = 0;
	pDC->pfnGetWaveformMode = 0;
	pDC->pfnSetWaveformMode = 0;
	pDC->pfnIsUpdating = 0;
	pDC->pfnWaitUpdateComplete = 0;
	pDC->pfnSetUpdateRect = 0;
	pDC->pfnPwrAutoOffIntervalMax = 0;
	pDC->pfnAutoOffEnable = 0;
	pDC->pfnPutImg = 0;
	pDC->pfnSetVCOM = 0;
	pDC->pfnSetVCOMToFlash = 0;
	pDC->pfnGetVCOM = 0;
	
	pDC->iIsForceWaitUpdateFinished = 1;
	
	pDC->pfnPwrOnOff = 0;
	
	pDC->iWFMode = -1;
	pDC->iLastWFMode = -1;
	
	return pDC;
}

EPDFB_DC *epdfbdc_create_ex(DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf,DWORD dwCreateFlag)
{
	return epdfbdc_create_ex2(dwW,dwH,dwW,dwH,bPixelBits,pbDCbuf,dwCreateFlag);
}



EPDFB_DC *epdfbdc_create(DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf)
{
	return epdfbdc_create_ex(dwW,dwH,bPixelBits,pbDCbuf,EPDFB_DC_FLAG_DEFAUT);
}

	
EPDFB_DC_RET epdfbdc_delete(EPDFB_DC *pEPD_dc)
{
	DBG_MSG("%s\n",__FUNCTION__);
	if(!CHK_EPDFB_DC(pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(pEPD_dc->dwBufSize>0) {
		my_free((void *)pEPD_dc->pbDCbuf);
		pEPD_dc->pbDCbuf = 0;
	}
	my_free(pEPD_dc);
	return EPDFB_DC_SUCCESS;
}

/*
 * 
 * 
 * 
 */
EPDFB_DC_RET epdfbdc_fbimg_normallize(EPDFB_DC *I_pEPD_dc,\
	EPDFB_IMG *IO_pEPD_img)
{
	EPDFB_DC_RET tRet=EPDFB_DC_PIXELBITSNOTSUPPORT;
	unsigned char *pbBuf = 0;
	DWORD dwBufSize ;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		return EPDFB_DC_OBJECTERR;
	}
	
	if(IO_pEPD_img->bPixelBits!=I_pEPD_dc->bPixelBits) {
		switch(IO_pEPD_img->bPixelBits) { // source image 
		case 16:
			if(4==I_pEPD_dc->bPixelBits) {
				pbBuf = IO_pEPD_img->pbImgBuf;
				dwBufSize = IO_pEPD_img->dwW*IO_pEPD_img->dwH*IO_pEPD_img->bPixelBits/8;
				_fb_RGB565toGray4(IO_pEPD_img->pbImgBuf,dwBufSize,
					pbBuf,dwBufSize,0);
				tRet=EPDFB_DC_SUCCESS;
			}
			else if(8==I_pEPD_dc->bPixelBits) {
			}
			else if (16==I_pEPD_dc->bPixelBits){
				tRet=EPDFB_DC_SUCCESS;
			}
			break;
		case 8:
			if(4==I_pEPD_dc->bPixelBits) {
				_fb_gray_8to4(IO_pEPD_img->pbImgBuf,\
					IO_pEPD_img->dwW*IO_pEPD_img->dwH );
				IO_pEPD_img->bPixelBits = 4;
				tRet=EPDFB_DC_SUCCESS;
			}
			else if(16==I_pEPD_dc->bPixelBits) {
				dwBufSize = IO_pEPD_img->dwW*IO_pEPD_img->dwH*I_pEPD_dc->bPixelBits/8;
				pbBuf = (unsigned char *)my_malloc(dwBufSize);
				if(!pbBuf) {
					return EPDFB_DC_MEMMALLOCFAIL;
				}
				memcpy(pbBuf,IO_pEPD_img->pbImgBuf,IO_pEPD_img->dwW*IO_pEPD_img->dwH);
				_fb_Gray8toRGB565(IO_pEPD_img->pbImgBuf,\
					IO_pEPD_img->dwW*IO_pEPD_img->dwH,pbBuf,dwBufSize);
				my_free(pbBuf);pbBuf=0;
				
			}
			else if(8==I_pEPD_dc->bPixelBits) {
				tRet=EPDFB_DC_SUCCESS;
			}
			break;
		case 4:
			dwBufSize = IO_pEPD_img->dwW*IO_pEPD_img->dwH*I_pEPD_dc->bPixelBits/8;
			pbBuf = (unsigned char *)my_malloc(dwBufSize);
			if(!pbBuf) {
				return EPDFB_DC_MEMMALLOCFAIL;
			}
			if(8==I_pEPD_dc->bPixelBits) {
				memcpy(pbBuf,IO_pEPD_img->pbImgBuf, \
					IO_pEPD_img->dwW*IO_pEPD_img->dwH*IO_pEPD_img->bPixelBits/8);
				_fb_gray_4to8(IO_pEPD_img->pbImgBuf,IO_pEPD_img->dwW*IO_pEPD_img->dwH,
					pbBuf,dwBufSize,0);
				tRet=EPDFB_DC_SUCCESS;
			}
			else if(16==I_pEPD_dc->bPixelBits) {
				memcpy(pbBuf,IO_pEPD_img->pbImgBuf, \
					IO_pEPD_img->dwW*IO_pEPD_img->dwH*IO_pEPD_img->bPixelBits/8);
				_fb_Gray4toRGB565(IO_pEPD_img->pbImgBuf,
					IO_pEPD_img->dwW*IO_pEPD_img->dwH*IO_pEPD_img->bPixelBits/8,
					pbBuf,dwBufSize,0);
				tRet=EPDFB_DC_SUCCESS;				
			}
			else if(4==I_pEPD_dc->bPixelBits) {
				tRet=EPDFB_DC_SUCCESS;
			}
			my_free(pbBuf);pbBuf=0;
			break;
		default :
			//tRet = EPDFB_DC_PIXELBITSNOTSUPPORT;
			break;
		}
	}
	return tRet;
}

/* Dither Tresshold for Red Channel */
static const unsigned char dither_tresshold_r[64] = {
  1, 7, 3, 5, 0, 8, 2, 6,
  7, 1, 5, 3, 8, 0, 6, 2,
  3, 5, 0, 8, 2, 6, 1, 7,
  5, 3, 8, 0, 6, 2, 7, 1,

  0, 8, 2, 6, 1, 7, 3, 5,
  8, 0, 6, 2, 7, 1, 5, 3,
  2, 6, 1, 7, 3, 5, 0, 8,
  6, 2, 7, 1, 5, 3, 8, 0
};

/* Dither Tresshold for Green Channel */
static const unsigned char dither_tresshold_g[64] = {
  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4,

  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4
};

/* Dither Tresshold for Blue Channel */
static const unsigned char dither_tresshold_b[64] = {
  5, 3, 8, 0, 6, 2, 7, 1,
  3, 5, 0, 8, 2, 6, 1, 7,
  8, 0, 6, 2, 7, 1, 5, 3,
  0, 8, 2, 6, 1, 7, 3, 5,

  6, 2, 7, 1, 5, 3, 8, 0,
  2, 6, 1, 7, 3, 5, 0, 8,
  7, 1, 5, 3, 8, 0, 6, 2,
  1, 7, 3, 5, 0, 8, 2, 6
};

/* Get 16bit closest color */
unsigned char closest_rb(unsigned char c) {
  return (c >> 3 << 3); /* red & blue */
}
unsigned char closest_g(unsigned char c) {
  return (c >> 2 << 2); /* green */
}

/* RGB565 */
static inline unsigned short RGB16BIT(unsigned char r, unsigned char g, unsigned char b) {
  return ((unsigned short)((r>>3)<<11)|((g>>2)<<5)|(b>>3));
}
static inline unsigned short RGB24BIT(unsigned char r, unsigned char g, unsigned char b) {
  return ((unsigned short)((r)<<16)|((g)<<8)|((b)));
}
static inline unsigned short RGB32BIT(unsigned char r, unsigned char g, unsigned char b) {
  return ((unsigned short)((r)<<24)|((g)<<16)|((b<<8)));
}

/* Dithering by individual subpixel */
DWORD ordered_dither_rgb888_xy(
  int x,
  int y,
  unsigned char r,
  unsigned char g,
  unsigned char b,
	int iBitsPixel
)
{
  /* Get Tresshold Index */
  unsigned char tresshold_id = ((y & 7) << 3) + (x & 7);

  r = closest_rb(
          MIN(r + dither_tresshold_r[tresshold_id], 0xff)
       );
  g = closest_g(
          MIN(g + dither_tresshold_g[tresshold_id], 0xff)
       );
  b = closest_rb(
          MIN(b + dither_tresshold_b[tresshold_id], 0xff)
       );
	if(16==iBitsPixel) {
 	 return (DWORD)RGB16BIT(r, g, b);
	}
	else 
	if(24==iBitsPixel) {
 	 return (DWORD)RGB24BIT(r, g, b);
	}
	else {
 	 return (DWORD)RGB32BIT(r, g, b);
	}
}

static inline DWORD _epdfbdc_pixel_dither(EPDFB_DC *pEPD_dc,unsigned char bSrcBitsPerPixel,
		DWORD dwSrcPxlVal,DWORD I_dwSrcPxlX,DWORD I_dwSrcPxlY)
{
	DWORD dwPValDitherRet=dwSrcPxlVal;

	if(pEPD_dc->dwFlags & EPDFB_DC_FLAG_DITHER8) {
		DWORD dwPVal;
		switch (bSrcBitsPerPixel) {
		case 2:
			dwPVal = dwSrcPxlVal & 0x3;
			break;
		case 4:
			dwPVal = dwSrcPxlVal & 0xf;
			break;
		case 8:
			dwPVal = dwSrcPxlVal & 0xff;
			break;
		case 16:
			dwPVal = dwSrcPxlVal & 0x1f;
			break;
		case 24:
			dwPVal = dwSrcPxlVal & 0xff;
			break;
		case 32:
			dwPVal = dwSrcPxlVal & 0xff;
			break;
		default :
			dwPVal = 0;
			break;
		}
		if((int)(dwPVal)>=pEPD_dc->dither8[I_dwSrcPxlY%16][I_dwSrcPxlX%16]) {
			dwPValDitherRet = 0xffffffff;
		}
		else {
			dwPValDitherRet = 0;
		}
	}
	else if(pEPD_dc->dwFlags & EPDFB_DC_FLAG_COLORDITHER_ORDERED) {
		if(24==bSrcBitsPerPixel) {
			unsigned char bR,bG,bB;
			bB = (unsigned char)(dwSrcPxlVal & 0xff);
			bG = (unsigned char)((dwSrcPxlVal>>8)&0xff) ;
			bR = (unsigned char)((dwSrcPxlVal>>16)&0xff) ;
			dwPValDitherRet = ordered_dither_rgb888_xy((int)I_dwSrcPxlX,(int)I_dwSrcPxlY,bR,bG,bB,pEPD_dc->bPixelBits);
		}
	}
	return dwPValDitherRet;
}

static inline DWORD _pixel_value_convert(EPDFB_DC *pEPD_dc,DWORD dwPixelVal,
	unsigned char bSrcBitsPerPixel,unsigned char bDestBitsPerPixel,DWORD dwSrcFlags)
{
	DWORD dwRetPixelVal;
	
	switch(bSrcBitsPerPixel) 
	{
	case 1:
		dwRetPixelVal = dwPixelVal?0x0:0xffffffff;
		break;

	case 2:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal<<2;
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal<<6;
		}
		else if(16==bDestBitsPerPixel) {
			dwRetPixelVal = (DWORD)(gwGray2toRGB565_TableA[(dwPixelVal&0x3)]);
			//printk("%x->%x.",dwPixelVal,dwRetPixelVal);
		}
		else if(32==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal<<6|dwPixelVal<<14|dwPixelVal<<22|dwPixelVal<<30;
			//printk("%x->%x.",dwPixelVal,dwRetPixelVal);
		}
		else if(24==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal<<6|dwPixelVal<<14|dwPixelVal<<22;
			//printk("%x->%x.",dwPixelVal,dwRetPixelVal);
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>1;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;

	case 4:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal<<4;
			//dwRetPixelVal = (DWORD)gbGray4toGray8_TableA[(dwPixelVal&0xf)];
		}
		else if(16==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<12;
			dwRetPixelVal = (DWORD)(gwGray4toRGB565_TableA[(dwPixelVal&0xf)]);
		}
		else if(32==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<8;
			dwRetPixelVal = dwPixelVal<<4|dwPixelVal<<12|dwPixelVal<<20|dwPixelVal<<28;
		}
		else if(24==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<8;
			dwRetPixelVal = dwPixelVal<<4|dwPixelVal<<12|dwPixelVal<<20;
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>3;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>2;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;

	case 8:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>4;
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else if(16==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<8;
			dwRetPixelVal = (DWORD)(gwGray4toRGB565_TableA[(dwPixelVal>>4)]);
		}
		else if(32==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<8;
			dwRetPixelVal = dwPixelVal|dwPixelVal<<8|dwPixelVal<<16|dwPixelVal<<24;
		}
		else if(24==bDestBitsPerPixel) {
			//dwRetPixelVal = dwPixelVal<<8;
			dwRetPixelVal = dwPixelVal|dwPixelVal<<8|dwPixelVal<<16;
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>7;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>6;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;

	case 16:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = (DWORD)_RGB565_to_Gray4(dwPixelVal);
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = (DWORD)_RGB565_to_Gray8(dwPixelVal);
		}
		else if(16==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else if(32==bDestBitsPerPixel) {
			dwRetPixelVal =_RGB565_to_RGBW8888(dwPixelVal);
		}
		else if(24==bDestBitsPerPixel) {
			dwRetPixelVal = _RGB565_to_RGB888(dwPixelVal);
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>31;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>30;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;

	case 24:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>20;
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>18;
		}
		else if(16==bDestBitsPerPixel) {
			unsigned char *pbVal=(unsigned char *)(&dwPixelVal);
			unsigned short wTemp = (unsigned short)_RGB888_to_RGB565(pbVal);
			
			dwRetPixelVal = (DWORD)(wTemp);
		}
		else if(32==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else if(24==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal&0xffffff;
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>23;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>22;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;

	case 32:
		if(4==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>20;
		}
		else if(8==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal;
		}
		else if(16==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>8;
		}
		else if(32==bDestBitsPerPixel) {
			unsigned char bR,bG,bB;
			if(dwSrcFlags & EPDFB_DC_FLAG_32BITS_ARGB) {
				bR = (dwPixelVal>>16)&0xff;
				bG = (dwPixelVal>>8)&0xff;
				bB = (dwPixelVal)&0xff;
			}
			else {
				bR = (dwPixelVal>>24)&0xff;
				bG = (dwPixelVal>>16)&0xff;
				bB = (dwPixelVal>>8)&0xff;
			}
			if(pEPD_dc->dwFlags & EPDFB_DC_FLAG_32BITS_ARGB) {
				// ARGB
				dwRetPixelVal = bR<<16|bG<<8|bB;
			}
			else {
				// RGBA
				dwRetPixelVal = bR<<24|bG<<16|bB<<8;
			}
		}
		else if(1==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>23;
		}
		else if(2==bDestBitsPerPixel) {
			dwRetPixelVal = dwPixelVal>>22;
		}
		else {
			dwRetPixelVal = dwPixelVal;
		}
		break;
		
	default:
		WARNING_MSG("convert %d -> %d not supported \n",bSrcBitsPerPixel,bDestBitsPerPixel);
		dwRetPixelVal = dwPixelVal;
		break;
	}
	//DBG_MSG("0x%x->0x%x.",dwPixelVal,dwRetPixelVal);
	
	return dwRetPixelVal;
}



static inline unsigned char *_epdfbdc_get_dcimg_ptr(
	EPDFB_DC *I_pEPD_imgdc,	DWORD I_dwImgX,DWORD I_dwImgY)
{
	unsigned char *pbRet ;
	unsigned char bBitsPerPixel = I_pEPD_imgdc->bPixelBits;

	if(4==bBitsPerPixel) {
		pbRet = (unsigned char *)(I_pEPD_imgdc->pbDCbuf + 
			(I_dwImgX>>1)+(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(8==bBitsPerPixel) {
		pbRet = (unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(16==bBitsPerPixel) {
		pbRet = (unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX>>1)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(24==bBitsPerPixel) {
		pbRet = (unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX/3)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(32==bBitsPerPixel) {
		pbRet = (unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX<<2)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(1==bBitsPerPixel) {
		pbRet =(unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX>>3)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else if(2==bBitsPerPixel) {
		pbRet =(unsigned char *)(I_pEPD_imgdc->pbDCbuf+(I_dwImgX>>2)+\
			(I_dwImgY*I_pEPD_imgdc->dwDCWidthBytes));
	}
	else {
		pbRet = 0;
	}
	return pbRet;
}

static inline DWORD _epdfbdc_get_img_pixelvalue_from_ptr(EPDFB_DC *I_pEPD_imgdc,
	unsigned char **IO_ppbImg,DWORD I_dwX)
{
	DWORD dwRet;
	unsigned char bBitsPerPixel=I_pEPD_imgdc->bPixelBits;
	
	ASSERT(IO_ppbImg);
	ASSERT(*IO_ppbImg);
	ASSERT(*IO_ppbImg>=I_pEPD_imgdc->pbDCbuf);
	
	if((*IO_ppbImg)>I_pEPD_imgdc->pbDCbuf+I_pEPD_imgdc->dwDCSize) {
		ERR_MSG("[Warning]*IO_ppbImg=%p>pbDCbuf=%p+dwDCSize=%d\n",(*IO_ppbImg),
			I_pEPD_imgdc->pbDCbuf,I_pEPD_imgdc->dwDCSize);
		return 0;
		ASSERT(0);
	}
	
	switch(bBitsPerPixel) {
	case 4:
	{
		unsigned char b;
		if(I_dwX&1) {
			if(I_pEPD_imgdc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA) {
				b = (**IO_ppbImg)&0xf;
			}
			else {
				b = ((**IO_ppbImg)>>4)&0xf;
			}
			(*IO_ppbImg) += 1;
		}
		else {
			if(I_pEPD_imgdc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA) {
				b = ((**IO_ppbImg)>>4)&0xf;
			}
			else {
				b = (**IO_ppbImg)&0xf;
			}
		}
		dwRet = (DWORD)b;
		
		break;
	}
	case 1:
	{
		unsigned char b=(I_dwX&0x7);
		unsigned char bBitMask=0x80>>b;
		
		dwRet = (DWORD)((**IO_ppbImg)&bBitMask);
		if(7==b) {
			(*IO_ppbImg) += 1;
		}
		break;
	}
	case 2:
	{
		unsigned char b=(I_dwX&0x3);
		unsigned char bShift=6-(b<<1);
		
		/*
		if(I_pEPD_imgdc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA) {

		}
		else {
			switch(bShift) {
			case 0:
				bShift = 6;
				break;
			case 2:
				bShift = 2;
				break;
			case 4:
				bShift = 4;
				break;
			case 6:
				bShift = 0;
				break;
			default :
				break;
			}		
		}
		*/
		
		dwRet = (DWORD)(((**IO_ppbImg)>>bShift)&0x03);
		
		if(3==b) {
			(*IO_ppbImg) += 1;
		}
		break;
	}
	case 8:
	{
		dwRet = (DWORD)(**((unsigned char **)IO_ppbImg));
		(*IO_ppbImg) += 1;
		break;
	}
	case 16:
	{
		dwRet = (DWORD)(**((unsigned short **)IO_ppbImg));
		(*IO_ppbImg) += 2;
		break;
	}
	case 24:
	{
		dwRet = (DWORD)(**((DWORD **)IO_ppbImg));
		(*IO_ppbImg) += 3;
		break;
	}
	case 32:
	{
		dwRet = (DWORD)(**((unsigned short **)IO_ppbImg));
		(*IO_ppbImg) += 4;
		break;
	}
	default:
		ERR_MSG("%d Bits/Pixel not supported !!\n",bBitsPerPixel);
		dwRet=0;
		break;
	}
	return dwRet;
}
#if 0
static inline void _epdfbdc_set_dcpixel_at_ptr(EPDFB_DC *I_pEPD_dc,
	unsigned char **IO_ppbDCDest,DWORD I_dwSrcPixelVal,unsigned char I_bSrcBitsPerPixel,
	unsigned short I_dwDestX)
{
	DWORD L_dwPixelVal;
	unsigned char bDestBitsPerPixel;
	
	ASSERT(IO_ppbDCDest);
	ASSERT(*IO_ppbDCDest);
	
	ASSERT(*IO_ppbDCDest>=I_pEPD_dc->pbDCbuf);
	ASSERT(*IO_ppbDCDest<I_pEPD_dc->pbDCbuf+I_pEPD_dc->dwDCSize);
	
	bDestBitsPerPixel = I_pEPD_dc->bPixelBits;
	L_dwPixelVal = _pixel_value_convert(I_dwSrcPixelVal,I_bSrcBitsPerPixel,bDestBitsPerPixel);
	
	switch(bDestBitsPerPixel) {
	case 4:
		{
		unsigned char bTemp,bPixelVal;
		
		bPixelVal=(unsigned char)(L_dwPixelVal&0xf);
		bTemp = **IO_ppbDCDest;
		
		if(I_pEPD_dc->dwFlags&EPDFB_DC_FLAG_REVERSEDRVDATA) {
			if(I_dwDestX&0x1) {
				bTemp &= 0x0f;
				bTemp |= (bPixelVal<<4)&0xf0 ;
				**IO_ppbDCDest = bTemp;
				(*IO_ppbDCDest) += 1;
			}
			else {
				bTemp &= 0xf0;
				bTemp |= bPixelVal;
				**IO_ppbDCDest = bTemp;
			}
		}	
		else {
			if(I_dwDestX&0x1) {
				bTemp &= 0xf0;
				bTemp |= bPixelVal;
				**IO_ppbDCDest = bTemp;
				(*IO_ppbDCDest) += 1;
			}
			else {
				bTemp &= 0x0f;
				bTemp |= (bPixelVal<<4)&0xf0 ;
				**IO_ppbDCDest = bTemp;
			}
		}
			
		
		break;
		}
	case 1:
		{
		unsigned char bTemp;
		unsigned char b=(I_dwDestX&0x7);
		
		bTemp = **IO_ppbDCDest;
		if(L_dwPixelVal) {
			bTemp |= 0x01<<b;
		}
		else {
			bTemp &= ~(0x01<<b);
		}
		**IO_ppbDCDest = bTemp;	
		if(7==b) {
			(*IO_ppbDCDest) += 1;
		}		
		break;
		}
	case 2:
		{
		unsigned char bTemp;
		unsigned char b=(I_dwDestX&0x3);
		
		bTemp = **IO_ppbDCDest;
		bTemp &= ~(0x3<<(b<<1));
		bTemp |= L_dwPixelVal<<(b<<1);
		**IO_ppbDCDest = bTemp;	
		if(3==b) {
			(*IO_ppbDCDest) += 1;
		}		
		break;
		}		
	case 8:
		{
		**((unsigned char **)IO_ppbDCDest) = (unsigned char)(L_dwPixelVal&0x000000ff);
		(*IO_ppbDCDest) += 1;
		break;
		}
	case 16:
		{
		**((unsigned short **)IO_ppbDCDest) = (unsigned short)(L_dwPixelVal&0x0000ffff);
		(*IO_ppbDCDest) += 2;
		break;
		}
	case 24:
		{
		**((DWORD **)IO_ppbDCDest) = (DWORD)(L_dwPixelVal&0x00ffffff);
		(*IO_ppbDCDest) += 3;
		break;
		}
	case 32:
		{
		**((DWORD **)IO_ppbDCDest) = (DWORD)(L_dwPixelVal&0xffffffff);
		(*IO_ppbDCDest) += 4;
		break;
		}
		
	default:
		break;
	
	}
}
#endif


static inline DWORD _epdfbdc_get_img_pixelvalue(EPDFB_DC *I_pEPD_dc,
	EPDFB_IMG *I_pEPD_img,DWORD dwX,DWORD dwY,DWORD dwImgWidthBytes)
{
	DWORD dwRet = 0;
	unsigned char bBitsPerPixel = I_pEPD_img->bPixelBits;
	//DWORD dwImgWidthBytes ;
	
	if(4==bBitsPerPixel) {
		unsigned char *pb;
		unsigned char b;
		
		pb = I_pEPD_img->pbImgBuf+
			(dwX>>1)+(dwY*dwImgWidthBytes);
			
		if(dwX&1) {
			if(I_pEPD_dc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA) {
				b = (*pb)&0xf;
			}
			else {
				b = ((*pb)>>4)&0xf;
			}
		}
		else {
			if(I_pEPD_dc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA) {
				b = ((*pb)>>4)&0xf;
			}
			else {
				b = (*pb)&0xf;
			}
		}
		dwRet = (DWORD)b;
	}
	else if(2==bBitsPerPixel) {
		unsigned char *pb =
		((unsigned char *)(I_pEPD_img->pbImgBuf+(dwX>>2)+(dwY*dwImgWidthBytes)));
		unsigned char bShiftBits = (dwX%4)<<1;
		dwRet = (DWORD)((*pb)>>bShiftBits);
		dwRet &= 0x3;
	}
	else if(1==bBitsPerPixel) {
		unsigned char *pb =
		((unsigned char *)(I_pEPD_img->pbImgBuf+(dwX>>3)+(dwY*dwImgWidthBytes)));
		unsigned char bBitMask=0x80>>(dwX&0x7);
		dwRet = (DWORD)((*pb)&bBitMask);
	}
	else if(8==bBitsPerPixel) {
		dwRet = (DWORD)
		*((unsigned char *)(I_pEPD_img->pbImgBuf+(dwX)+(dwY*dwImgWidthBytes)));
		dwRet &= 0xff;
	}
	else if(16==bBitsPerPixel) {
		dwRet = (DWORD)
		*((unsigned short *)(I_pEPD_img->pbImgBuf+(dwX<<1)+(dwY*dwImgWidthBytes)));
		dwRet &= 0xffff;
	}
	else if(24==bBitsPerPixel) {
//#ifdef _X86 //[
		unsigned char *pb = 
		(unsigned char *)(I_pEPD_img->pbImgBuf+(dwX*3)+(dwY*dwImgWidthBytes));
		//dwRet = (((*pb)<<16)|((*(pb+1))<<8)|((*(pb+2))))&0xffffff;
		dwRet = _be32_to_cpu(*((DWORD *)(pb)));
#ifdef debug_pts //[
		if(dwX<debug_pts && dwY<debug_pts) {
			DBG_MSG("getsrcpixel (%d,%d) @0x%p val={0x%02x,0x%02x,0x%02x,0x%02x} = 0x%x\n",
			(int)dwX,(int)dwY,pb,pb[0],pb[1],pb[2],pb[3],dwRet);
		}
#endif //] debug_pts

//#else //][ !_X86
//		dwRet = (DWORD)
//		*((DWORD *)(I_pEPD_img->pbImgBuf+(dwX*3)+(dwY*dwImgWidthBytes)));
//		dwRet &= 0xffffff;
//#endif //] _X86
#if 0
		{
			unsigned char *pbRet = &dwRet;
			printf("%s(%d,%d)=%02x %02x %02x %02x,0x%08x\n",__FUNCTION__,
					dwX,dwY,pbRet[0],pbRet[1],pbRet[2],pbRet[3],dwRet);
		}
#endif
	}
	else if(32==bBitsPerPixel) {
		unsigned char *pb = I_pEPD_img->pbImgBuf+(dwX<<2)+(dwY*dwImgWidthBytes);
		dwRet = _be32_to_cpu(*((DWORD *)(pb)));
#ifdef debug_pts //[
		if(dwX<debug_pts && dwY<debug_pts) {
			DBG_MSG("getsrcpixel (%d,%d) @0x%p val={0x%02x,0x%02x,0x%02x,0x%02x} = 0x%x\n",
			(int)dwX,(int)dwY,pb,pb[0],pb[1],pb[2],pb[3],dwRet);
		}
#endif //] debug_pts
	}
	else {
		ERR_MSG("%s : pixel bits %d not supported !\n",__FUNCTION__,bBitsPerPixel);
	}
	
	return dwRet;
}

static inline void _epdfbdc_set_pixel(EPDFB_DC *pEPD_dc,
	DWORD dwX,DWORD dwY,
	DWORD dwPixelVal,unsigned char bSrcBitsPerPixel,
	DWORD dwSrcFlags)
{
	DWORD L_dwPixelVal;
	unsigned char bDestBitsPerPixel;
	
	L_dwPixelVal = _pixel_value_convert(pEPD_dc,dwPixelVal,bSrcBitsPerPixel,pEPD_dc->bPixelBits,dwSrcFlags);
	bDestBitsPerPixel = pEPD_dc->bPixelBits;
	

#if 0
		{
			unsigned char *pb = &L_dwPixelVal;
			DBG_MSG("%s(%d,%d)=%02x %02x %02x %02x,0x%08x\n",__FUNCTION__,
					dwX,dwY,pb[0],pb[1],pb[2],pb[3],L_dwPixelVal);
		}
#endif
	//DBG_MSG("set (%d,%d)=0x%x\n",dwX,dwY,L_dwPixelVal);
	
	if( 4 == bDestBitsPerPixel ) {
		volatile unsigned char *pbDCDest;
		unsigned char bTemp,bPixelVal;
		
		
		//DBG_MSG("%s(),x=0x%x,y=0x%x,pixel=0x%x\n",__FUNCTION__,dwX,dwY,dwPixelVal);
		
		pbDCDest = pEPD_dc->pbDCbuf + \
					(dwX>>1)+(dwY*pEPD_dc->dwDCWidthBytes);
		
		bPixelVal=(unsigned char)(L_dwPixelVal&0xf);
		bTemp = *pbDCDest;
		
		if(pEPD_dc->dwFlags&EPDFB_DC_FLAG_REVERSEDRVDATA) {
			if(dwX&0x1) {
				bTemp &= 0x0f;
				bTemp |= (bPixelVal<<4)&0xf0 ;
			}
			else {
				bTemp &= 0xf0;
				bTemp |= bPixelVal;
			}
		}	
		else {
			if(dwX&0x1) {
				bTemp &= 0xf0;
				bTemp |= bPixelVal;
			}
			else {
				bTemp &= 0x0f;
				bTemp |= (bPixelVal<<4)&0xf0 ;
			}
		}
		
		*pbDCDest = bTemp;
	}
	else if( 16 == bDestBitsPerPixel ) {
		//volatile unsigned short *pwDCDest;
		volatile unsigned char *pbDCDest = (unsigned char *)(pEPD_dc->pbDCbuf + \
					(dwX<<1)+(dwY*pEPD_dc->dwDCWidthBytes));
		
		if(pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_BE) {
			*pbDCDest++ = (L_dwPixelVal>>8)&0xff;
			*pbDCDest++ = (L_dwPixelVal)&0xff;
		}
		else
		{
			*pbDCDest++ = (L_dwPixelVal)&0xff;
			*pbDCDest++ = (L_dwPixelVal>>8)&0xff;
		}
	}
	else if( 24 == bDestBitsPerPixel ) {
		volatile unsigned char *pbDCDest;
		volatile unsigned char *pbSrcPixVal = (unsigned char *)&L_dwPixelVal;
	
		if(pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_G4) {
			pbDCDest = (unsigned char *)(pEPD_dc->pbDCbuf + \
					(dwX*3)+(dwY*pEPD_dc->dwDCWidthBytes));
			switch(dwY%3) {
			case 0:
				pbDCDest[0] = *pbSrcPixVal++;
				pbDCDest[1] = *pbSrcPixVal++;
				pbDCDest[2] = *pbSrcPixVal++;
				break;
			case 1:
				pbDCDest[2] = *pbSrcPixVal++;
				pbDCDest[0] = *pbSrcPixVal++;
				pbDCDest[1] = *pbSrcPixVal++;
				break;
			case 2:
				pbDCDest[1] = *pbSrcPixVal++;
				pbDCDest[2] = *pbSrcPixVal++;
				pbDCDest[0] = *pbSrcPixVal++;
				break;
			}
		}
		else {
			pbDCDest = (unsigned char *)(pEPD_dc->pbDCbuf + \
					(dwX*3)+(dwY*pEPD_dc->dwDCWidthBytes));
			*pbDCDest++ = *pbSrcPixVal++;
			*pbDCDest++ = *pbSrcPixVal++;
			*pbDCDest++ = *pbSrcPixVal++;
		}
	}
	else if( 32 == bDestBitsPerPixel ) {
		volatile unsigned char *pbDCDest;
		unsigned char *pb;

		pbDCDest = (unsigned char *)(pEPD_dc->pbDCbuf + \
			(dwX<<2)+(dwY*pEPD_dc->dwDCWidthBytes));

		L_dwPixelVal = _cpu_to_be32(L_dwPixelVal);
		pb=(unsigned char *)&L_dwPixelVal;

#ifdef debug_pts //[
		if(dwX<debug_pts && dwY<debug_pts) {
			DBG_MSG("(%d,%d) SrcPixVal(%d)=0x%x {0x%02x,0x%02x,0x%02x,0x%02x}\n",
				(int)dwX,(int)dwY,(int)bSrcBitsPerPixel,(DWORD)L_dwPixelVal,pb[0],pb[1],pb[2],pb[3]);
		}
#endif

		if(8==bSrcBitsPerPixel) {
			*pbDCDest++ = pb[3];
			*pbDCDest++ = pb[3];
			*pbDCDest++ = pb[3];
			*pbDCDest++ = pb[3];
		}
		else {
			if(pEPD_dc->dwFlags & EPDFB_DC_FLAG_32BITS_ARGB) {
				pbDCDest++;
			}

			*pbDCDest++ = pb[0];
			*pbDCDest++ = pb[1];
			*pbDCDest++ = pb[2];
		}

	}
	else if( 8 == bDestBitsPerPixel ) {
		unsigned char *pbDCDest;
		unsigned char *pb;
		int idx;

		pbDCDest = (unsigned char *)(pEPD_dc->pbDCbuf + \
			(dwX)+(dwY*pEPD_dc->dwDCWidthBytes));
	

		L_dwPixelVal = _cpu_to_be32(L_dwPixelVal);
		pb=(unsigned char *)&L_dwPixelVal;

#ifdef debug_pts //[
		if(dwX<debug_pts && dwY<debug_pts) {
			DBG_MSG("(%d,%d) @%p<=0x%x{0x%02x,0x%02x,0x%02x,0x%02x}",
				(int)dwX,(int)dwY,pbDCDest,(DWORD)L_dwPixelVal,pb[0],pb[1],pb[2],pb[3]);
		}
#endif

		if(32==bSrcBitsPerPixel) 
		{
			if(pEPD_dc->dwFlags & EPDFB_DC_FLAG_32BITS_ARGB) {
				// ARGB
				if(pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_G4) {
					idx = gcCFA_gen4_argb_maskA[dwY%EINK_CFA_GEN4_H][dwX%EINK_CFA_GEN4_H];
					*pbDCDest = pb[idx];
#ifdef debug_pts //[
					if(dwX<debug_pts && dwY<debug_pts) {
						DBG_MSG(" ARGB,G4,idx=%d ",idx);
					}
#endif //] debug_pts
				}
				else {
					*pbDCDest = (pb[1]+pb[2]+pb[3])/3;
				}
			}
			else {
				// RGBA
				if((pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_MASK)==EPDFB_DC_FLAG_OFB_CFA_G4) {
					
					if((pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_RMASK)==EPDFB_DC_FLAG_OFB_CFA_R270) {
						idx = gcCFA_gen4_rgba_maskA[dwX%EINK_CFA_GEN4_H][(pEPD_dc->dwHeight-dwY-1)%EINK_CFA_GEN4_W];
						*pbDCDest = pb[idx];
#ifdef debug_pts //[
						if(dwX<debug_pts && dwY<debug_pts) {
							DBG_MSG(" RGBA,G4,r270,idx=%d",idx);
						}
#endif //] debug_pts
					}
					else 
					{
						idx = gcCFA_gen4_rgba_maskA[dwY%EINK_CFA_GEN4_H][dwX%EINK_CFA_GEN4_W];
						*pbDCDest = pb[idx];
#ifdef debug_pts //[
						if(dwX<debug_pts && dwY<debug_pts) {
							DBG_MSG(" RGBA,G4,r0,idx=%d",idx);
						}
#endif //] debug_pts
					}
				}
				else if((pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_MASK)==EPDFB_DC_FLAG_OFB_CFA_G4_2) {
					if((pEPD_dc->dwFlags&EPDFB_DC_FLAG_OFB_CFA_RMASK)==EPDFB_DC_FLAG_OFB_CFA_R270) {
						idx = gcCFA_gen4_2_rgba_maskA[dwX%EINK_CFA_GEN4_H][(pEPD_dc->dwHeight-dwY-1)%EINK_CFA_GEN4_W];
						*pbDCDest = pb[idx];
#ifdef debug_pts //[
						if(dwX<debug_pts && dwY<debug_pts) {
							DBG_MSG(" RGBA,G4_2,r270,idx=%d",idx);
						}
#endif //] debug_pts
					}
					else {
						idx = gcCFA_gen4_2_rgba_maskA[dwY%EINK_CFA_GEN4_H][dwX%EINK_CFA_GEN4_W];
						*pbDCDest = pb[idx];
#ifdef debug_pts //[
						if(dwX<debug_pts && dwY<debug_pts) {
							DBG_MSG(" RGBA,G4_2,r0,idx=%d",idx);
						}
#endif //] debug_pts
					}
				}
				else {
					*pbDCDest = (pb[0]+pb[1]+pb[2])/3;
				}
			}
		}
		else if(8==bSrcBitsPerPixel) {
			*pbDCDest = pb[3];
		}
#ifdef debug_pts //[
		if(dwX<debug_pts && dwY<debug_pts) {
			DBG_MSG("\n");
		}
#endif //] debug_pts
	}
	else {
	}
}


//

EPDFB_DC_RET epdfbdc_put_dcimg(EPDFB_DC *pEPD_dc,
	EPDFB_DC *pEPD_dcimg,EPDFB_ROTATE_T tRotateDegree,
	DWORD I_dwDCimgX,DWORD I_dwDCimgY,
	DWORD I_dwDCimgW,DWORD I_dwDCimgH,
	DWORD I_dwDCPutX,DWORD I_dwDCPutY)
{
	EPDFB_DC_RET tRet=EPDFB_DC_SUCCESS;
	
	DWORD dwDCH,dwDCW;
	
	//DWORD dwImgW,dwImgH;
	//DWORD dwImgX,dwImgY;
	
	unsigned char bImgPixelBits;
	DWORD dwDCWidthBytes;
	DWORD dwX,dwY,dwEPD_dc_flags;
	DWORD dwSrcX,dwSrcY;

	DWORD h,w;// image offset of x,y .
	
	DWORD dwWHidden,dwHHidden;
	
	//unsigned char *pbDCRow;
	//unsigned char *pbImgRow;
	
	//int tick=jiffies;
	DWORD dwImgWidthBytes;
	
	
	EPDFB_IMG tEPD_img,*pEPD_img=&tEPD_img;
	pEPD_img->dwX = I_dwDCimgX;
	pEPD_img->dwY = I_dwDCimgY;
	pEPD_img->dwW = pEPD_dcimg->dwWidth+pEPD_dcimg->dwFBWExtra;
	pEPD_img->dwH = pEPD_dcimg->dwHeight+pEPD_dcimg->dwFBHExtra;
	pEPD_img->bPixelBits = pEPD_dcimg->bPixelBits;
	pEPD_img->pbImgBuf = (unsigned char *)pEPD_dcimg->pbDCbuf;
	

	dwImgWidthBytes = pEPD_dcimg->dwDCWidthBytes;//pEPD_dc->bPixelBits/8
	GALLEN_DBGLOCAL_BEGIN();

	if(!CHK_EPDFB_DC(pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		GALLEN_DBGLOCAL_RUNLOG(0);
		GALLEN_DBGLOCAL_ESC();
		return EPDFB_DC_OBJECTERR;
	}
	
	//I_dwDCPutX += pEPD_dc->dwFBXOffset;
	//I_dwDCPutY += pEPD_dc->dwFBYOffset;
	
	
	dwWHidden = pEPD_dc->dwFBWExtra;
	dwHHidden = pEPD_dc->dwFBHExtra;
	dwDCH=pEPD_dc->dwHeight + dwHHidden;
	dwDCW=pEPD_dc->dwWidth + dwWHidden;
	dwEPD_dc_flags = pEPD_dc->dwFlags;
	
	bImgPixelBits=pEPD_dcimg->bPixelBits;
	
	//if(pEPD_dc->bPixelBits!=bImgPixelBits) {
	//	GALLEN_DBGLOCAL_ESC();
	//	return EPDFB_DC_PIXELBITSNOTSUPPORT;
	//}
	
	//ASSERT(4==pEPD_dc->bPixelBits);
	//ASSERT(4==bImgPixelBits);
	//ASSERT(0==(I_dwDCimgW&0x1)); // image width must be even .
	//ASSERT(0==(I_dwDCimgH&0x1)); // image heigh must be even .
	
	
	dwDCWidthBytes = pEPD_dc->dwDCWidthBytes;
	

	DBG_MSG("dc_flag=0x%x,imgdc.w=%u,imgdc.h=%u,img.w=%u,img.h=%u,img pixbits=%d,dc pixbits=%d\n",
		dwEPD_dc_flags,pEPD_dcimg->dwWidth,pEPD_dcimg->dwHeight,I_dwDCimgW,I_dwDCimgH,pEPD_dcimg->bPixelBits,pEPD_dc->bPixelBits);
	DBG_MSG("IMG_Wbytes=%u,DC_Wbytes=%u,rot=%d\n",dwImgWidthBytes,dwDCWidthBytes,tRotateDegree);

	switch(tRotateDegree) {
	case EPDFB_R_0:GALLEN_DBGLOCAL_RUNLOG(2);
		// left->right,up->down .
#if 0 //[ performance test only  . 
		{
			//DWORD dwPixelVal;
			unsigned char *pbImgRow;
			unsigned char *pbDst;
			int idx;
			
			
			pEPD_dc->dwDirtyOffsetStart = I_dwDCimgY*dwDCWidthBytes;
			pEPD_dc->dwDirtyOffsetEnd = (I_dwDCimgY+I_dwDCimgH)*dwDCWidthBytes;
			
			//dwImgPixelIdx = 0;
			for(h=0;h<I_dwDCimgH;h+=1) {
				
				dwSrcY = I_dwDCimgY+h;
				dwY = I_dwDCPutY+h+pEPD_dc->dwFBYOffset;
				if(dwY>=dwDCH) {
					continue ;
				}

				// only for rgba8888 to cfa .
				pbImgRow = _epdfbdc_get_dcimg_ptr(pEPD_dcimg,I_dwDCimgX,dwSrcY);
				pbDst=_epdfbdc_get_dcimg_ptr(pEPD_dc,I_dwDCimgX,dwY);
				

				for(w=0;w<I_dwDCimgW;w+=1) {
					dwX = I_dwDCPutX+w+pEPD_dc->dwFBXOffset;


					idx = gcCFA_gen4_rgba_maskA[dwY%3][dwX%3];
					*pbDst = pbImgRow[idx];
					pbDst++;
#ifdef debug_pts //[
					if(dwX<debug_pts && dwY<debug_pts) {
						printk("(%d,%d) {0x%02x,0x%02x,0x%02x,0x%02x},idx=%d => 0x%02x\n",
							(int)dwX,(int)dwY,
							pbImgRow[0],pbImgRow[1],pbImgRow[2],pbImgRow[3],idx,pbImgRow[idx]);
					}
#endif //] debug_pts
				}
			}
		}		
#else //][!
		{
			DWORD dwPixelVal;
			
			
			pEPD_dc->dwDirtyOffsetStart = I_dwDCimgY*dwDCWidthBytes;
			pEPD_dc->dwDirtyOffsetEnd = (I_dwDCimgY+I_dwDCimgH)*dwDCWidthBytes;
			
			//dwImgPixelIdx = 0;
			for(h=0;h<I_dwDCimgH;h+=1) {
				
				dwSrcY = I_dwDCimgY+h;
				dwY = I_dwDCPutY+h+pEPD_dc->dwFBYOffset;
				if(dwY>=dwDCH) {
					continue ;
				}

				//GALLEN_DBGLOCAL_PRINTMSG("y=%d,x=",h);
				
				//pbImgRow = _epdfbdc_get_dcimg_ptr(pEPD_dcimg,I_dwDCimgX,dwSrcY);
				
				for(w=0;w<I_dwDCimgW;w+=1) {
					
					dwSrcX = I_dwDCimgX+w;
					dwX = I_dwDCPutX+w+pEPD_dc->dwFBXOffset;
					if(dwX>=dwDCW) {
						continue ;
					}
					
#ifdef MIRROR_FROM_SRC //[
					if(dwEPD_dc_flags&EPDFB_DC_FLAG_MIRROR_SRC) {
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwDCW-dwSrcX-1,dwSrcY,dwImgWidthBytes);
					}
					else 
#endif //]MIRROR_FROM_SRC
					{
						//GALLEN_DBGLOCAL_PRINTMSG("%d,",w,h);
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwSrcX,dwSrcY,dwImgWidthBytes);
					}
					//DBG_MSG("(%d,%d)=0x%x\n",(int)dwSrcX,(int)dwSrcY,dwPixelVal);
					
					dwPixelVal = _epdfbdc_pixel_dither(pEPD_dc,bImgPixelBits,dwPixelVal,dwSrcX,dwSrcY);

					if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPLEFTPIXEL && 0==w) {
						// skip left pixel output.
					}
					else if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPRIGHTPIXEL && I_dwDCimgW==w+1) {
						// skip right pixel output.
					}
					else {
						_epdfbdc_set_pixel(pEPD_dc,dwX,dwY,dwPixelVal,bImgPixelBits,pEPD_dcimg->dwFlags);
					}
				}
				//GALLEN_DBGLOCAL_PRINTMSG("\n,",w,h);
			}
		}		
#endif //] 
		break;
	case EPDFB_R_90:GALLEN_DBGLOCAL_RUNLOG(3);
		// up->down,right->left .
		{
			DWORD dwPixelVal;

			pEPD_dc->dwDirtyOffsetStart = I_dwDCimgX*dwDCWidthBytes;
			pEPD_dc->dwDirtyOffsetEnd = (I_dwDCimgX+I_dwDCimgW)*dwDCWidthBytes;

			
			//dwImgPixelIdx = 0;

			for(h=0;h<I_dwDCimgH;h+=1) {
				dwSrcY = I_dwDCimgY+h;
				dwX = dwDCW-h-1-I_dwDCPutY-dwWHidden+pEPD_dc->dwFBXOffset;
				if(dwX>=dwDCW) {
					continue ;
				}
				
				//pbImgRow = _epdfbdc_get_dcimg_ptr(pEPD_dcimg,I_dwDCimgX,dwSrcY);

				for(w=0;w<I_dwDCimgW;w+=1) {

					dwSrcX = I_dwDCimgX+w;
					dwY = I_dwDCPutX+w+pEPD_dc->dwFBYOffset;
					if(dwY>=dwDCH) {
						continue ;
					}

#ifdef MIRROR_FROM_SRC //[
					if(dwEPD_dc_flags&EPDFB_DC_FLAG_MIRROR_SRC) {
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwDCW-1-dwSrcX,dwSrcY,dwImgWidthBytes);
					}
					else 
#endif //] MIRROR_FROM_SRC
					{
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwSrcX,dwSrcY,dwImgWidthBytes);
					}

					dwPixelVal = _epdfbdc_pixel_dither(pEPD_dc,bImgPixelBits,dwPixelVal,dwSrcX,dwSrcY);

					if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPLEFTPIXEL && 0==w) {
						// skip left pixel output.
						//DBG_MSG("[[%d]]",w);
					}
					else if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPRIGHTPIXEL && I_dwDCimgW==w+1) {
						// skip right pixel output.
						//DBG_MSG("{{%d}}",w);
					}
					else {
						_epdfbdc_set_pixel(pEPD_dc,dwX,dwY,dwPixelVal,bImgPixelBits,pEPD_dcimg->dwFlags);
					}
				}
			}// image height loop .
		}
		break;
	case EPDFB_R_180:GALLEN_DBGLOCAL_RUNLOG(4);
		// right->left,down->up .
		{
			volatile DWORD dwPixelVal;
			
			pEPD_dc->dwDirtyOffsetStart = (dwDCH-I_dwDCimgY-I_dwDCimgH)*dwDCWidthBytes;
			pEPD_dc->dwDirtyOffsetEnd = (dwDCH-I_dwDCimgY)*dwDCWidthBytes;
			
			//dwImgPixelIdx = 0;
			for(h=0;h<I_dwDCimgH;h+=1) {

				dwSrcY = I_dwDCimgY+h;
				dwY = dwDCH-h-1-I_dwDCPutY-dwHHidden+pEPD_dc->dwFBYOffset;
				if(dwY>=dwDCH) {
					continue ;
				}					

				//pbImgRow = _epdfbdc_get_dcimg_ptr(pEPD_dcimg,I_dwDCimgX,dwSrcY);

				//DBG_MSG("y=%d->",I_dwDCimgH-h-1);
				for(w=0;w<I_dwDCimgW;w+=1) {
					dwSrcX = I_dwDCimgX+w;
					dwX = dwDCW-w-1-I_dwDCPutX-dwWHidden+pEPD_dc->dwFBXOffset;
				//DBG_MSG("y=%d->",dwImgH-h-1);
					if(dwX>=dwDCW) {
						continue ;
					}
				
#ifdef MIRROR_FROM_SRC //[
					if(dwEPD_dc_flags&EPDFB_DC_FLAG_MIRROR_SRC) {
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwDCW-1-dwSrcX,dwSrcY,dwImgWidthBytes);
					}
					else 
#endif //] MIRROR_FROM_SRC
					{	
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwSrcX,dwSrcY,dwImgWidthBytes);
					}

					dwPixelVal = _epdfbdc_pixel_dither(pEPD_dc,bImgPixelBits,dwPixelVal,dwSrcX,dwSrcY);

					if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPLEFTPIXEL && 0==w) {
						// skip left pixel output.
					}
					else if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPRIGHTPIXEL && I_dwDCimgW==w+1) {
						// skip right pixel output.
					}
					else {
						_epdfbdc_set_pixel(pEPD_dc,dwX,dwY,dwPixelVal,bImgPixelBits,pEPD_dcimg->dwFlags);
					}
				}
			}
		}		
		break;
	case EPDFB_R_270:GALLEN_DBGLOCAL_RUNLOG(5);
		// down->up,left->right .
		{
			DWORD dwPixelVal;
			
			pEPD_dc->dwDirtyOffsetStart = (dwDCH-I_dwDCimgX-I_dwDCimgW)*dwDCWidthBytes;
			pEPD_dc->dwDirtyOffsetEnd = (dwDCH-I_dwDCimgX)*dwDCWidthBytes;
			
			//dwImgPixelIdx = 0;
			for(h=0;h<I_dwDCimgH;h+=1) {

				dwSrcY = I_dwDCimgY+h;
				dwX = h+I_dwDCPutY+pEPD_dc->dwFBXOffset;
				if(dwX>=dwDCW) {
					continue ;
				}

				//pbImgRow = _epdfbdc_get_dcimg_ptr(pEPD_dcimg,I_dwDCimgX,dwSrcY);

				for(w=0;w<I_dwDCimgW;w+=1) {

					dwSrcX = I_dwDCimgX+w;
					dwY = dwDCH-w-1-I_dwDCPutX-dwHHidden+pEPD_dc->dwFBYOffset;
					if(dwY>=dwDCH) {
						continue ;
					}					
					
#ifdef MIRROR_FROM_SRC //[
					if(dwEPD_dc_flags&EPDFB_DC_FLAG_MIRROR_SRC) {
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwDCW-1-dwSrcX,dwSrcY,dwImgWidthBytes);
					}
					else 
#endif //] MIRROR_FROM_SRC
					{
						dwPixelVal = _epdfbdc_get_img_pixelvalue(pEPD_dc,pEPD_img,dwSrcX,dwSrcY,dwImgWidthBytes);
					}

					dwPixelVal = _epdfbdc_pixel_dither(pEPD_dc,bImgPixelBits,dwPixelVal,dwSrcX,dwSrcY);

					if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPLEFTPIXEL && 0==w) {
						// skip left pixel output.
					}
					else if(dwEPD_dc_flags&EPDFB_DC_FLAG_SKIPRIGHTPIXEL && I_dwDCimgW==w+1) {
						// skip right pixel output.
					}
					else {
						_epdfbdc_set_pixel(pEPD_dc,dwX,dwY,dwPixelVal,bImgPixelBits,pEPD_dcimg->dwFlags);
					}
				}
			}
		}
		break;
	}

//printk ("[%s-%d] %d\n",__func__,__LINE__,jiffies-tick);	
	
	GALLEN_DBGLOCAL_END();
	return tRet;
}

EPDFB_DC_RET epdfbdc_put_fbimg(EPDFB_DC *pEPD_dc,
	EPDFB_IMG *pEPD_img,EPDFB_ROTATE_T tRotateDegree)
{
	EPDFB_DC *ptEPD_dcimg ;
	EPDFB_DC_RET tRet;
	
	ptEPD_dcimg = epdfbdc_create_ex(pEPD_img->dwW,pEPD_img->dwH,\
		pEPD_img->bPixelBits,pEPD_img->pbImgBuf,pEPD_dc->dwFlags);
		
	tRet = epdfbdc_put_dcimg(pEPD_dc,ptEPD_dcimg,tRotateDegree,
		0,0,pEPD_img->dwW,pEPD_img->dwH,pEPD_img->dwX,pEPD_img->dwY);

	epdfbdc_delete(ptEPD_dcimg);
	
	return tRet;
}


EPDFB_DC_RET epdfbdc_get_rotate_active(EPDFB_DC *I_pEPD_dc,
	DWORD *IO_pdwX,DWORD *IO_pdwY,
	DWORD *IO_pdwW,DWORD *IO_pdwH,
	EPDFB_ROTATE_T I_tRotate)
{
	EPDFB_DC_RET tRet=EPDFB_DC_SUCCESS;
	DWORD dwX,dwY,dwH,dwW;
	DWORD dwDCW,dwDCH;
	unsigned dwWHidden,dwHHidden;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(!(IO_pdwX&&IO_pdwY&&IO_pdwW&&IO_pdwH)) {
		return EPDFB_DC_PARAMERR;
	}
	
	dwWHidden = I_pEPD_dc->dwFBWExtra;
	dwHHidden = I_pEPD_dc->dwFBHExtra;
	dwDCH=I_pEPD_dc->dwHeight + dwHHidden;
	dwDCW=I_pEPD_dc->dwWidth + dwWHidden;
	
	switch(I_tRotate)
	{
	default :
	case EPDFB_R_0:
		dwX = (*IO_pdwX);
		dwY = (*IO_pdwY);
		dwW = (*IO_pdwW);
		dwH = (*IO_pdwH);
		return tRet;
	case EPDFB_R_90:
		dwX = dwDCW-(*IO_pdwY)-(*IO_pdwH)-dwWHidden;
		dwY = (*IO_pdwX);
		dwW = (*IO_pdwH);
		dwH = (*IO_pdwW);
		break;
	case EPDFB_R_180:
		dwX = dwDCW-(*IO_pdwX)-(*IO_pdwW)-dwWHidden;
		dwY = dwDCH-(*IO_pdwY)-(*IO_pdwH)-dwHHidden;
		dwW = (*IO_pdwW);
		dwH = (*IO_pdwH);
		break;
	case EPDFB_R_270:
		dwX = (*IO_pdwY);
		dwY = dwDCH-(*IO_pdwW)-(*IO_pdwX)-dwHHidden;
		dwW = (*IO_pdwH);
		dwH = (*IO_pdwW);
		break;
	}
	*IO_pdwX = dwX;
	*IO_pdwY = dwY;
	*IO_pdwW = dwW;
	*IO_pdwH = dwH;
	return tRet;
}


EPDFB_DC_RET epdfbdc_set_pixel(EPDFB_DC *I_pEPD_dc,\
	DWORD I_dwX,DWORD I_dwY,DWORD I_dwPVal)
{
	EPDFB_DC_RET tRet=EPDFB_DC_SUCCESS;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	_epdfbdc_set_pixel(I_pEPD_dc,I_dwX,I_dwY,I_dwPVal,4,I_pEPD_dc->dwFlags);
	
	return tRet;
}

EPDFB_DC_RET epdfbdc_dcbuf_to_RGB565(EPDFB_DC *I_pEPD_dc,\
	unsigned char *O_pbRGB565Buf,DWORD I_dwRGB565BufSize)
{
	EPDFB_DC_RET tRet=EPDFB_DC_SUCCESS;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		return EPDFB_DC_OBJECTERR;
	}
	
	_fb_Gray4toRGB565(O_pbRGB565Buf,I_dwRGB565BufSize,\
		(unsigned char *)I_pEPD_dc->pbDCbuf,I_pEPD_dc->dwDCSize,I_pEPD_dc->dwFlags&EPDFB_DC_FLAG_REVERSEINPDATA);
	
	return tRet;
}

EPDFB_DC_RET epdfbdc_get_dirty_region(EPDFB_DC *I_pEPD_dc,\
	DWORD *O_pdwDirtyOffsetStart,DWORD *O_pdwDirtyOffsetEnd)
{
	EPDFB_DC_RET tRet=EPDFB_DC_SUCCESS;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		return EPDFB_DC_OBJECTERR;
	}
	
	if(O_pdwDirtyOffsetStart) {
		*O_pdwDirtyOffsetStart = I_pEPD_dc->dwDirtyOffsetStart ;
	}
	I_pEPD_dc->dwDirtyOffsetStart = 0;
	
	if(O_pdwDirtyOffsetEnd) {
		*O_pdwDirtyOffsetEnd = I_pEPD_dc->dwDirtyOffsetEnd ;
	}
	I_pEPD_dc->dwDirtyOffsetEnd = I_pEPD_dc->dwDCSize;
	
	return tRet;
}


EPDFB_DC_RET epdfbdc_set_width_height(EPDFB_DC *I_pEPD_dc,
		DWORD dwFBW,DWORD dwFBH,
		DWORD dwW,DWORD dwH)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;
	unsigned char bPixelBits;
	DWORD dwFBBits ;
	DWORD dwFBWBits;
	DWORD dwDCSize ,dwDCWidthBytes;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}

	if(dwFBW<dwW) {
		ERR_MSG("%s(%d): FBW(%d)<W(%d) !\n",__FUNCTION__,__LINE__,dwFBW,dwW);
		return EPDFB_DC_PARAMERR;
	}

	if(dwFBH<dwH) {
		ERR_MSG("%s(%d): FBH(%d)<H(%d) !\n",__FUNCTION__,__LINE__,dwFBH,dwH);
		return EPDFB_DC_PARAMERR;
	}

	bPixelBits = I_pEPD_dc->bPixelBits;
	dwFBBits = (dwFBW*dwFBH*bPixelBits);
	dwFBWBits = (dwFBW*bPixelBits);
	dwDCSize=dwFBBits>>3;
	dwDCWidthBytes=dwFBWBits>>3;
	
	if(dwFBWBits&0x7) {
		dwDCWidthBytes+=1;
		dwDCSize+=dwFBH;
	}

	if( I_pEPD_dc->dwBufSize>0 && dwDCSize>I_pEPD_dc->dwDCSize) {
		ERR_MSG("%s(%d): new DCSize(%d)>original DCSize(%d) !\n",__FUNCTION__,__LINE__,
				dwDCSize,I_pEPD_dc->dwDCSize);
		return EPDFB_DC_PARAMERR;
	}

	I_pEPD_dc->dwDCWidthBytes = dwDCWidthBytes;
	I_pEPD_dc->dwDirtyOffsetEnd = dwDCSize;
	I_pEPD_dc->dwDCSize = dwDCSize;

	I_pEPD_dc->dwFBWExtra = dwFBW-dwW ;
	I_pEPD_dc->dwFBHExtra = dwFBH-dwH ;
	I_pEPD_dc->dwWidth = dwW;
	I_pEPD_dc->dwHeight = dwH;
	
	return tRet;
}
EPDFB_DC_RET epdfbdc_set_fbxyoffset(EPDFB_DC *I_pEPD_dc,
		DWORD dwFBXOffset,DWORD dwFBYOffset)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;
	DWORD dwFBW,dwFBH;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	dwFBW = I_pEPD_dc->dwWidth+I_pEPD_dc->dwFBWExtra;
	dwFBH = I_pEPD_dc->dwHeight+I_pEPD_dc->dwFBHExtra;

	if(dwFBW<dwFBXOffset) {
		ERR_MSG("%s(%d): FBW(%d)<X(%d) !\n",__FUNCTION__,__LINE__,dwFBW,dwFBXOffset);
		return EPDFB_DC_PARAMERR;
	}

	if(dwFBH<dwFBYOffset) {
		ERR_MSG("%s(%d): FBH(%d)<Y(%d) !\n",__FUNCTION__,__LINE__,dwFBH,dwFBYOffset);
		return EPDFB_DC_PARAMERR;
	}

	I_pEPD_dc->dwFBXOffset = dwFBXOffset;
	I_pEPD_dc->dwFBYOffset = dwFBYOffset;
	
	return tRet;	
}

EPDFB_DC_RET epdfbdc_rotate(EPDFB_DC *I_pEPD_dc,EPDFB_ROTATE_T I_tRotate)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;
	unsigned char *pbWorkBuf;
	EPDFB_IMG tEPD_img;
	
	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(I_tRotate == EPDFB_R_0) {
		// nothing have to do .
		return EPDFB_DC_SUCCESS;
	}
	
	pbWorkBuf = (unsigned char *)my_malloc(I_pEPD_dc->dwDCSize);
	if(pbWorkBuf) {
		
		tEPD_img.dwX = 0;
		tEPD_img.dwY = 0;
		tEPD_img.dwW = I_pEPD_dc->dwWidth;
		tEPD_img.dwH = I_pEPD_dc->dwHeight;
		
		tEPD_img.bPixelBits = I_pEPD_dc->bPixelBits;
		tEPD_img.pbImgBuf = pbWorkBuf;
		
		my_memcpy(pbWorkBuf,I_pEPD_dc->pbDCbuf,I_pEPD_dc->dwDCSize);
		
		tRet = epdfbdc_put_fbimg(I_pEPD_dc,&tEPD_img,I_tRotate);
		my_free(pbWorkBuf);pbWorkBuf=0;
	}
	else {
		ERR_MSG("%s(%d): memory not enough !\n",__FUNCTION__,__LINE__);
		tRet = EPDFB_DC_MEMMALLOCFAIL;
	}
	
	return tRet;
}

EPDFB_DC_RET epdfbdc_set_host_dataswap(EPDFB_DC *I_pEPD_dc,int iIsSet)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(iIsSet) {
		I_pEPD_dc->dwFlags |= EPDFB_DC_FLAG_REVERSEINPDATA;
	}
	else {
		I_pEPD_dc->dwFlags &= ~EPDFB_DC_FLAG_REVERSEINPDATA;
	}

	return tRet;
}

EPDFB_DC_RET epdfbdc_set_drive_dataswap(EPDFB_DC *I_pEPD_dc,int iIsSet)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(iIsSet) {
		I_pEPD_dc->dwFlags |= EPDFB_DC_FLAG_SKIPLEFTPIXEL;
	}
	else {
		I_pEPD_dc->dwFlags &= ~EPDFB_DC_FLAG_SKIPLEFTPIXEL;
	}

	return tRet;
}

EPDFB_DC_RET epdfbdc_set_skip_pixel(EPDFB_DC *I_pEPD_dc,int iIsSet,int iIsSkipRight)
{
	EPDFB_DC_RET tRet = EPDFB_DC_SUCCESS;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}
	
	if(iIsSkipRight) {
		if(iIsSet) {
			I_pEPD_dc->dwFlags |= EPDFB_DC_FLAG_SKIPRIGHTPIXEL;
		}
		else {
			I_pEPD_dc->dwFlags &= ~EPDFB_DC_FLAG_SKIPRIGHTPIXEL;
		}
	}
	else {
		if(iIsSet) {
			I_pEPD_dc->dwFlags |= EPDFB_DC_FLAG_SKIPLEFTPIXEL;
		}
		else {
			I_pEPD_dc->dwFlags &= ~EPDFB_DC_FLAG_SKIPLEFTPIXEL;
		}
	}

	return tRet;
}

EPDFB_DC_RET epdfbdc_set_flags(EPDFB_DC *I_pEPD_dc,DWORD *pIO_dwFlags)
{
	unsigned dwOldFlags;

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}

	if(!pIO_dwFlags) {
		return EPDFB_DC_PARAMERR; 
	}
	
	dwOldFlags = I_pEPD_dc->dwFlags ;
	I_pEPD_dc->dwFlags = *pIO_dwFlags;
	*pIO_dwFlags = dwOldFlags;
	return EPDFB_DC_SUCCESS;
}

EPDFB_DC_RET epdfbdc_get_flags(EPDFB_DC *I_pEPD_dc,DWORD *pO_dwFlags)
{

	if(!CHK_EPDFB_DC(I_pEPD_dc)) {
		ERR_MSG("%s(%d): object handle error !\n",__FUNCTION__,__LINE__);
		return EPDFB_DC_OBJECTERR;
	}

	if(!pO_dwFlags) {
		return EPDFB_DC_PARAMERR; 
	}
	
	*pO_dwFlags = I_pEPD_dc->dwFlags;
	return EPDFB_DC_SUCCESS;
}




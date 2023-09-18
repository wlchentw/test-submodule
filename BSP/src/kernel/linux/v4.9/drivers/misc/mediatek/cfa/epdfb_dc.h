/***************************************************
 * EPD frame buffer device content manager (4 bits).
 * 
 * Author : Gallen Lin 
 * Data : 2011/01/20 
 * Revision : 1.0
 * File Name : epdfb_dc.h 
 * 
****************************************************/

#ifndef __epdfb_dc_h//[
#define __epdfb_dc_h


typedef unsigned int DWORD ;


typedef enum {
	EPDFB_DC_PXLFMT_DEFAULT = 0,
	EPDFB_DC_PXLFMT_EINKCFA = 1,
} EPDFB_DC_PXLFMTTYPE;

typedef enum {
	EPDFB_DC_SUCCESS = 0,
	EPDFB_DC_PARAMERR = -1,
	EPDFB_DC_MEMMALLOCFAIL = -2,
	EPDFB_DC_OBJECTERR = -3, // dc object content error .
	EPDFB_DC_PIXELBITSNOTSUPPORT = -4,
} EPDFB_DC_RET;

typedef enum {
	EPDFB_R_0 = 0,
	EPDFB_R_90 = 1,
	EPDFB_R_180 = 2,
	EPDFB_R_270 = 3,
} EPDFB_ROTATE_T;

typedef struct tagEPDFB_IMG {
	DWORD dwX,dwY;
	DWORD dwW,dwH;
	unsigned char bPixelBits,bReserve;
	unsigned char *pbImgBuf;
} EPDFB_IMG;



typedef void (*fnVcomEnable)(int iIsEnable);
typedef void (*fnPwrOnOff)(int iIsOn);
typedef void (*fnPwrAutoOffIntervalMax)(int iIsEnable);
typedef void (*fnAutoOffEnable)(int iIsEnable);
typedef int (*fnGetWaveformBpp)(void);
typedef int (*fnSetPartialUpdate)(int iIsPartial);
typedef unsigned char *(*fnGetRealFrameBuf)(void);
typedef unsigned char *(*fnGetRealFrameBufEx)(DWORD *O_pdwFBSize);
typedef void (*fnDispStart)(int iIsStart);
typedef int (*fnGetWaveformMode)(void);
typedef void (*fnSetWaveformMode)(int iWaveformMode);
typedef int (*fnIsUpdating)(void);
typedef int (*fnWaitUpdateComplete)(void);
typedef int (*fnSetUpdateRect)(unsigned short wX,unsigned short wY,
	unsigned short wW,unsigned short wH);
typedef int (*fnPutImg)(EPDFB_IMG *I_ptPutImage,EPDFB_ROTATE_T I_tRotate);
typedef int (*fnSetVCOM)(int iVCOM_set_mV);
typedef int (*fnSetVCOMToFlash)(int iVCOM_set_mV);
typedef int (*fnGetVCOM)(int *O_piVCOM_get_mV);


// epd framebuffer device content ...
typedef struct tagEPDFB_DC {
	// public :
	DWORD dwWidth;// visible DC width .
	DWORD dwHeight;// visible DC height .
	DWORD dwFBWExtra; // frame buffer width extra width .
	DWORD dwFBHExtra; // frame buffer height extra height .
	DWORD dwFBXOffset; // frame buffer X offset .
	DWORD dwFBYOffset; // frame buffer Y offset .
	unsigned char bPixelBits;
	unsigned char bReservedA[1];
	volatile unsigned char *pbDCbuf;// DC buffer .
	
	fnGetWaveformBpp pfnGetWaveformBpp;
	fnVcomEnable pfnVcomEnable;
	fnSetPartialUpdate pfnSetPartialUpdate;
	fnGetRealFrameBuf pfnGetRealFrameBuf;
	fnGetRealFrameBufEx pfnGetRealFrameBufEx;
	fnDispStart pfnDispStart;
	fnGetWaveformMode pfnGetWaveformMode;
	fnSetWaveformMode pfnSetWaveformMode;
	fnIsUpdating pfnIsUpdating;
	fnWaitUpdateComplete pfnWaitUpdateComplete;
	fnSetUpdateRect pfnSetUpdateRect;
	fnPwrAutoOffIntervalMax pfnPwrAutoOffIntervalMax;
	fnPwrOnOff pfnPwrOnOff;
	fnAutoOffEnable pfnAutoOffEnable;
	fnPutImg pfnPutImg;
	fnSetVCOM pfnSetVCOM;
	fnSetVCOMToFlash pfnSetVCOMToFlash;
	fnGetVCOM pfnGetVCOM;
	
	// private : do not modify these member var .
	//  only for epdfbdc manager .
	DWORD dwMagicPrivateBegin;
	
	DWORD dwDCSize; // visible dc size .
	DWORD dwBufSize; // dc buffer real size .
	
	DWORD dwDCWidthBytes;
	DWORD dwFlags;
	int iWFMode,iLastWFMode;
	int iIsForceWaitUpdateFinished;
	DWORD dwDirtyOffsetStart,dwDirtyOffsetEnd;
	DWORD dwMagicPrivateEnd;
	int dither8[16][16];
	EPDFB_DC_PXLFMTTYPE iPxlFmtType;
} EPDFB_DC;


#define epdfbdc_create_e60mt2()	epdfbdc_create(800,600,4,0)

EPDFB_DC *epdfbdc_create(DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf);
	
#define EPDFB_DC_FLAG_DEFAUT	(0x0)

#define EPDFB_DC_FLAG_REVERSEDRVDATA		0x00000001 // reverse drive data -> pixel 3,pixel 4,pixel 1,pixel 2  .
#define EPDFB_DC_FLAG_REVERSEINPDATA		0x00000002 // reverse input data -> pixel 3,pixel 4,pixel 1,pixel 2  .

#define EPDFB_DC_FLAG_SKIPLEFTPIXEL			0x00000004 // shift input image data (skip 1 pixel in image left side).
#define EPDFB_DC_FLAG_SKIPRIGHTPIXEL		0x00000008 // shift input image data (skip 1 pixel in image right side).

#define EPDFB_DC_FLAG_MIRROR						0x00000010 // mirror DC .
#define EPDFB_DC_FLAG_MIRROR_SRC				0x00000020 // mirror source image .

#define EPDFB_DC_FLAG_DITHER8									0x00000100 // 1 bit dithering method .
#define EPDFB_DC_FLAG_COLORDITHER_ORDERED			0x00000200 // RGB888 to dithering method .

#define EPDFB_DC_FLAG_OFB_RGB565				0x00010000 // output framebuffer data format is rgb565 .
#define EPDFB_DC_FLAG_FLASHDIRTY				0x00020000 // only flash dirty image .

#define EPDFB_DC_FLAG_32BITS_RGBWRECT		0x00040000 // output framebuffer data format is 32 bits RGBW retangle physical layout on panel 
#define EPDFB_DC_FLAG_32BITS_ARGB				0x00080000 // output framebuffer data format is 32 bits ARGB format
#define EPDFB_DC_FLAG_OFB_BE						0x00100000 // output framebuffer write with big endian .
#define EPDFB_DC_FLAG_OFB_LE						0x00200000 // output framebuffer write with big endian .
#define EPDFB_DC_FLAG_OFB_CFA_MASK		0x00c00000

#define EPDFB_DC_FLAG_OFB_CFA_G4		0x00400000 // output framebuffer data format is EINK_CFA source x3  
#define EPDFB_DC_FLAG_OFB_CFA_G4_2		0x00800000 // output framebuffer data format is EINK_CFA source x3  

#define EPDFB_DC_FLAG_OFB_CFA_RMASK		0x03000000 //   
#define EPDFB_DC_FLAG_OFB_CFA_R90		0x01000000 // CFA layer rotate 90  
#define EPDFB_DC_FLAG_OFB_CFA_R180		0x02000000 // CFA layer rotate 180
#define EPDFB_DC_FLAG_OFB_CFA_R270		0x03000000 // CFA layer rotate 270

EPDFB_DC *epdfbdc_create_ex(DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf,DWORD dwCreateFlag);
	
EPDFB_DC *epdfbdc_create_ex2(DWORD dwFBW,DWORD dwFBH,\
	DWORD dwW,DWORD dwH,\
	unsigned char bPixelBits,unsigned char *pbDCbuf,DWORD dwCreateFlag);


EPDFB_DC_RET epdfbdc_delete(EPDFB_DC *I_pEPD_dc);




EPDFB_DC_RET epdfbdc_fbimg_normallize(EPDFB_DC *I_pEPD_dc,\
	EPDFB_IMG *IO_pEPD_img);

EPDFB_DC_RET epdfbdc_put_fbimg(EPDFB_DC *I_pEPD_dc,\
	EPDFB_IMG *I_pEPD_img,EPDFB_ROTATE_T I_tRotateDegree);

EPDFB_DC_RET epdfbdc_put_dcimg(EPDFB_DC *pEPD_dc,
	EPDFB_DC *pEPD_dcimg,EPDFB_ROTATE_T tRotateDegree,
	DWORD I_dwDCimgX,DWORD I_dwDCimgY,
	DWORD I_dwDCimgW,DWORD I_dwDCimgH,
	DWORD I_dwDCPutX,DWORD I_dwDCPutY);

EPDFB_DC_RET epdfbdc_get_dirty_region(EPDFB_DC *I_pEPD_dc,\
	DWORD *O_pdwDirtyOffsetStart,DWORD *O_pdwDirtyOffsetEnd);
	
EPDFB_DC_RET epdfbdc_set_pixel(EPDFB_DC *I_pEPD_dc,\
	DWORD I_dwX,DWORD I_dwY,DWORD I_dwPVal);

EPDFB_DC_RET epdfbdc_dcbuf_to_RGB565(EPDFB_DC *I_pEPD_dc,\
	unsigned char *O_pbRGB565Buf,DWORD I_dwRGB565BufSize);

EPDFB_DC_RET epdfbdc_rotate(EPDFB_DC *I_pEPD_dc,EPDFB_ROTATE_T I_tRotate);
EPDFB_DC_RET epdfbdc_set_host_dataswap(EPDFB_DC *I_pEPD_dc,int I_isSet);
EPDFB_DC_RET epdfbdc_set_drive_dataswap(EPDFB_DC *I_pEPD_dc,int I_isSet);
EPDFB_DC_RET epdfbdc_set_skip_pixel(EPDFB_DC *I_pEPD_dc,int iIsSet,int iIsSkipRight);

EPDFB_DC_RET epdfbdc_get_rotate_active(EPDFB_DC *I_pEPD_dc,
	DWORD *IO_pdwX,DWORD *IO_pdwY,
	DWORD *IO_pdwW,DWORD *IO_pdwH,
	EPDFB_ROTATE_T I_tRotate);
	
EPDFB_DC_RET epdfbdc_set_width_height(EPDFB_DC *I_pEPD_dc,
		DWORD dwFBW,DWORD dwFBH,
		DWORD dwW,DWORD dwH);
		
EPDFB_DC_RET epdfbdc_set_fbxyoffset(EPDFB_DC *I_pEPD_dc,
		DWORD dwFBXOffset,DWORD dwFBYOffset);

EPDFB_DC_RET epdfbdc_set_flags(EPDFB_DC *I_pEPD_dc,DWORD *pIO_dwFlags);

EPDFB_DC_RET epdfbdc_get_flags(EPDFB_DC *I_pEPD_dc,DWORD *pO_dwFlags);

#endif //]__epdfb_dc_h


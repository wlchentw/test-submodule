#ifndef _PATTERN_H_
#define _PATTERN_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "mdp_api.h"
#include "hwtcon_display.h"
#include "img_hal.h"

#define DEBUG_WRITE_BUFFER 0
#define MAX_PATH_LENGTH 100
#if 0
typedef struct pattern_info_t
{
	int		 row_bytes,
				size,
				fd;
	char		*buffer;
};

typedef enum{
	MTK_IMG_JPEG = 1,
	MTK_IMG_PNG = 2,
}mtk_img_type;
#endif

typedef enum{
	MTK_IMG_Y8	   = 1,
	MTK_IMG_Y4	   = 2,
	MTK_IMG_ARGB8888 = 3,
	MTK_IMG_RGB888   = 4,
}mtk_img_format;

typedef struct{
	int x_point;
	int y_point;
	int width;
	int height;
}Region;

typedef struct{
	int iClearScreen;				//whether need clear last show picture
	//int iUiSerialNo;				 //show which ui picture
	int iCropType;				   //src crop, dst crop
	int iRotate;					 //90, 180, 270
	int iFlip;					   //top-bottom, left-right
	Region rCropRegion;			  //crop region
	Region rDisplayRegion;		   //display region
	char input_buf[MAX_PATH_LENGTH]; //picture->file path, text->string
	//int iTextResizeScale;			//text will enlarge to iTextResizeScale * src_w
	mtk_img_format eImgFormat;	   //For graphic grey bits per pixel  
	int iRowDirection;			   //For graphic row or column direction graphic grey  
	int iCollisionCase;			   //For Collision Case 0:test1;1:test2
	int iAutoWaveformCase;         //0:2 case; 1:4 case; 2: 8 case; 3:16 case
	int src_width;
	int src_height;
	unsigned char * src_addr[3];
	unsigned char * dst_addr[3];
	int pixelSize;
	unsigned int v4l2_src_color_fmt;
	unsigned int v4l2_dst_color_fmt;
	int update_mode;                //for display, 1:full mode, 0:partial mode
	int waveform_mode;              //for display, 1:full mode, 0:partial mode
	int temperature;                //for display
	int panel_width;
	int panel_height;
	struct mtk_imghal_param *ptImgparam;
}Pattern_parm;

enum mtk_img_type getFileType(char * file_path); 

int draw_ui(Pattern_parm *ptPatternParm);
int draw_text(Pattern_parm *ptPatternParm);
int draw_graphic(Pattern_parm *ptPatternParm);
int draw_graphicType(Pattern_parm *ptPatternParm, int graphic_type);
int draw_picture(Pattern_parm *ptPatternParm);

#endif

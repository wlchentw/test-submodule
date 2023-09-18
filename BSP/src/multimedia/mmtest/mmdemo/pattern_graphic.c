#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"

int image_format_map_bits_per_pixel(mtk_img_format eImgFormat)
{
	printf("<GRAPHIC> eImgFormat %d\n", eImgFormat);
	switch(eImgFormat)
	{
		case MTK_IMG_Y8:
			 return 8;
		case MTK_IMG_Y4:
			 return 8;
		case MTK_IMG_ARGB8888:
			 return 32;
		case MTK_IMG_RGB888:
			 return 24;	 
	}
	
}

int draw_graphicType(Pattern_parm *ptPatternParm, int graphic_type)
{
    int x, y;
    int xres = ptPatternParm->rDisplayRegion.width;
    int yres = ptPatternParm->rDisplayRegion.height;
    unsigned char which_level = 0xF0;
    int row_bytes = ptPatternParm->rDisplayRegion.width;
    int icount = 0;
    int icountIndex = 0;
    if (0 == graphic_type)//2 jie
    {
        int row_count = yres / 2;
    	//printf("2 jie width:%d height:%d\n", xres, yres);
        //row direction
    	for (y = 0; y < yres; ++y) {
		    for (x = 0; x < xres; ++x) {
		       ptPatternParm->dst_addr[0][(row_bytes * y) + x] = which_level;
		    }
		    icount ++;
    		if (icount >= row_count)
    		{
    			icount = 0;
    			which_level = 0x00;
    		}
		
	    }
    }
    else if (1 == graphic_type)//4 jie
    {
        int row_count = yres / 4;
    	//printf("4 jie width:%d height:%d\n", xres, yres);
    	//row direction
    	for (y = 0; y < yres; ++y) {
		    for (x = 0; x < xres; ++x) {
		       ptPatternParm->dst_addr[0][(row_bytes * y) + x] = which_level;
		    }
		    icount ++;
		    if (icount >= row_count)
		    {
		        icount = 0;
		        if (which_level < 0x10)
		        {
		            which_level = 0x00;
		        }
		        else
		        {
                    which_level = which_level - 0x50; 
                }
		        //printf("4 while_level 0x%x\n", which_level);
		    }
	    }
    }
    else if (2 == graphic_type)//8 jie
    {
        int row_count = yres / 8;
        icountIndex = 1;
    	//printf("8 jie width:%d height:%d\n", xres, yres);
    	//row direction
    	for (y = 0; y < yres; ++y) {
		    for (x = 0; x < xres; ++x) {
		       ptPatternParm->dst_addr[0][(row_bytes * y) + x] = which_level;
		    }
		    icount ++;
		    if (icount >= row_count)
		    {
		        icount = 0;
		        switch (icountIndex)
		        {
		            case 1:
		            case 3:
		            case 5:
		            case 7:
		                which_level = which_level - 0x20; 
		                break;
		            case 2:
		            case 6:
		                which_level = which_level - 0x30; 
		                break;
		            case 4:
		                which_level = which_level - 0x10; 
		                break;
                    default:
		                break;
		            
		                
		        }
		        
		        icountIndex++;
		        //printf("8 icountIndex %d while_level 0x%x\n", icountIndex, which_level);
		    }
	    }
    }
    else if (3 == graphic_type)//16 jie
    {
        int row_count = yres / 16;
    	//printf("16 jie width:%d height:%d\n", xres, yres);
    	//row direction
    	for (y = 0; y < yres; ++y) {
		    for (x = 0; x < xres; ++x) {
		       ptPatternParm->dst_addr[0][(row_bytes * y) + x] = which_level;
		    }
		    icount ++;
		    if (icount >= row_count)
		    {
		        icount = 0;
		        if (which_level < 0x10)
		        {
		            which_level = 0x00;
		        }
		        else
		        {
		            which_level = which_level - 0x10; 
		        }
		        //printf("16 while_level 0x%x\n", which_level);
		    }
	    }
    }
    return 0;
}

int draw_graphic(Pattern_parm *ptPatternParm)
{
	int size = 0;
	printf("<GRAPHIC> iRowDirection %d\n", ptPatternParm->iRowDirection);

	int bits_per_pixel = image_format_map_bits_per_pixel(ptPatternParm->eImgFormat);
	
	ptPatternParm->pixelSize = ptPatternParm->rDisplayRegion.width * ptPatternParm->rDisplayRegion.height * bits_per_pixel / 8;
   
	#if 0
	char *buffer = (char*)malloc(ptPatternParm->rDisplayRegion.width * ptPatternParm->rDisplayRegion.height * bits_per_pixel / 8);
	if (NULL == buffer)
	{
		printf("<GRAPHIC> malloc buffer fail\n");
		return -1;
	}

	FILE *fdes = NULL;
	fdes = fopen("text.bin", "w");
	if(!fdes)
	{
		printf("<GRAPHIC>open file error!\n");
		return 0;
	}
#endif
	int x, y;
	int xres = ptPatternParm->rDisplayRegion.width * bits_per_pixel / 8;
	int yres = ptPatternParm->rDisplayRegion.height;
	unsigned char which_level = 0xFF;
	int row_bytes = ptPatternParm->rDisplayRegion.width * bits_per_pixel / 8;
	int row_count = yres / 256;
	int column_count = xres / 256;  
	int icount = 0;
	
#if 1
	printf("Image buffer Addr: %p width:%d height:%d\n",
		ptPatternParm->src_addr[0],
		xres,
		yres);
	if (ptPatternParm->iRowDirection)
	{
		//row direction
		for (y = 0; y < yres; ++y) {
			for (x = 0; x < xres; ++x) {
			   ptPatternParm->src_addr[0][(row_bytes * y) + x] = which_level;
			}
			icount ++;
			if (icount >= row_count)
			{
				icount = 0;
				which_level --;
			}
			
		}
	}
	else
	{
		//column direction
		for (x = 0; x < xres; ++x) {
			for (y = 0; y < yres; ++y) {
				ptPatternParm->src_addr[0][(row_bytes * y) + x] = which_level;
			}
			icount ++;
			if (icount >= column_count)
			{
				icount = 0;
				which_level --;
			}
		}
	}
#endif
	ptPatternParm->src_width = xres;
		ptPatternParm->src_height = yres;
		if (0 == ptPatternParm->iCropType)
		{
	 		ptPatternParm->rCropRegion.width = ptPatternParm->src_width;
	 		ptPatternParm->rCropRegion.height = ptPatternParm->src_height;
		}

	#if 0
	fwrite(ptPatternParm->src_addr[0], sizeof(int), size, fdes);
	
	fclose(fdes);
	#endif

	
	return 0;
}


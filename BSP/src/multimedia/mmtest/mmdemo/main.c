/**
main
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pattern.h"
#include "hwtcon_display.h"

unsigned int convert2colorfmt(unsigned int id)
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
			printf("Not support id\n");
			return V4L2_PIX_FMT_Y4_M0;
	}
}

extern int hwtcon_get_panel_info(int *panel_width, int *panel_height);
void pass_mdp(Pattern_parm *ptPatternParm)
{
	int length = sizeof(struct BlitStreamParam);
	struct BlitStreamParam *blit = malloc(length);
	if (NULL == blit)
	{
        printf("blit malloc error\n");
        return;
	}
	int dst_pitch = 0;
	//unsigned char * addr[3] = {NULL, NULL, NULL};
	//addr[0] = malloc(ptPatternParm->rDisplayRegion.width * ptPatternParm->rDisplayRegion.height);
	BlitStreamInit(blit);
	printf("MDP BlitStreamInit done\n");
	BlitStreamConfigSrc(blit, 
						ptPatternParm->src_width, 
						ptPatternParm->src_height, 
						ptPatternParm->src_width, 
						ptPatternParm->v4l2_src_color_fmt, 
						&ptPatternParm->src_addr[0], 
						V4L2_MEMORY_USERPTR, 
						ptPatternParm->rCropRegion.x_point,
						ptPatternParm->rCropRegion.y_point, 
						ptPatternParm->rCropRegion.width, 
						ptPatternParm->rCropRegion.height);
	printf("MDP BlitStreamConfigSrc:width:%d height:%d pitch:%d region[%d %d %d %d]\n",
		ptPatternParm->src_width,
		ptPatternParm->src_height,
		ptPatternParm->src_width,
		ptPatternParm->rCropRegion.x_point,
		ptPatternParm->rCropRegion.y_point,
		 ptPatternParm->rCropRegion.width,
		 ptPatternParm->rCropRegion.height
		);
	hwtcon_get_panel_info(&dst_pitch, NULL);
	printf("MDP BlitStreamConfigDst width:%d height:%d pitch:%d region[%d %d %d %d] rotate:%d flip:%d\n",
				ptPatternParm->rDisplayRegion.width, 
						ptPatternParm->rDisplayRegion.height, 
						dst_pitch,
						ptPatternParm->rDisplayRegion.x_point, 
						ptPatternParm->rDisplayRegion.y_point, 
						ptPatternParm->rDisplayRegion.width,
						ptPatternParm->rDisplayRegion.height,
						ptPatternParm->iRotate,
						ptPatternParm->iFlip);
	//if BlitStreamConfigSrc and BlitStreamConfigDst out_w,out_h not the same, mdp will resize.
	BlitStreamConfigDst(blit, 
						ptPatternParm->rDisplayRegion.width, 
						ptPatternParm->rDisplayRegion.height, 
						dst_pitch,
						ptPatternParm->v4l2_dst_color_fmt, 
						&ptPatternParm->dst_addr[0],
						V4L2_MEMORY_USERPTR,
						#if 1
						ptPatternParm->rDisplayRegion.x_point, 
						ptPatternParm->rDisplayRegion.y_point, 
						#else
           			    0,
           			    0,
           			    #endif
						ptPatternParm->rDisplayRegion.width,
						ptPatternParm->rDisplayRegion.height,
						ptPatternParm->iRotate,
						ptPatternParm->iFlip);

	BlitStreamOn(blit);
	printf("MDP BlitStreamOn done\n");
	//BlitStreamConfigPQ(blit);
	//printf("MDP BlitStreamConfigPQ done\n");
	BlitStreamRun(blit);
	printf("MDP BlitStreamRun done\n");
	BlitStreamOff(blit);
	printf("MDP BlitStreamOff done\n");
	BlitStreamDeinit(blit);
	printf("MDP BlitStreamDeinit done\n");
	
	free(blit);
	printf("MDP End\n");
	
}


void pass_hwtcon_display(Pattern_parm *ptPatternParm)
{
	//Display api	
	//in->dst_addr(MDP out buffer), display x,y,w,h	
	int status = hwtcon_open_device();
	if (status != 0) {
		return;
	}	
	struct rect region = {0};	
	region.x = ptPatternParm->rDisplayRegion.x_point;	
	region.y = ptPatternParm->rDisplayRegion.y_point;
	region.width = ptPatternParm->rDisplayRegion.width;
	region.height = ptPatternParm->rDisplayRegion.height;

	/* show the picture. */	
	#if 0
	status = hwtcon_display(ptPatternParm->dst_addr[0],
		ptPatternParm->rDisplayRegion.width,
		ptPatternParm->rDisplayRegion.height,
		ptPatternParm->rDisplayRegion.width,
		region,
		ptPatternParm->update_mode,
		ptPatternParm->waveform_mode,
		ptPatternParm->temperature
		);
	#else
	status = hwtcon_display_no_copy(ptPatternParm->dst_addr[0],
			ptPatternParm->panel_width,
			ptPatternParm->panel_height,//ptPatternParm->rDisplayRegion.height,
			ptPatternParm->panel_width,//ptPatternParm->rDisplayRegion.width,
			region,
			ptPatternParm->update_mode,
			ptPatternParm->waveform_mode,
			ptPatternParm->temperature
			);
	#endif
	if (status != 0)
		return;
	hwtcon_close_device();
}

int map_grey_level(int index)
{
    switch(index)
    {
        case 0:
            return 2;
            break;
        case 1:
            return 4;
            break;
        case 2:
            return 8;
            break;
        case 3:
            return 16;
            break;
        default:
            return 0;
            break;
    }
}

char * map_wave_mode(int index)
{
    if (2 == index)
    {
        return "gc16";
    }
    else if (index >= 15)
    {
        return "auto";
    }
    
    return NULL;
}

extern char *hwtcon_display_get_img_buffer(void);

#define HW_ENGINE_MDP 1 << 0
#define HW_ENGINE_TCON 1 << 1
int main(int argc, char *argv[])
{
	int argi = 1;
	int ret = 0;
	char *a;
	int option = 0;
	int i = 0;
	int j = 0;
	int k = 0;
	int h = 0;
	int wavemode1 = 0;
	int wavemode2 = 0;
	int use_hw_engine = HW_ENGINE_MDP | HW_ENGINE_TCON;

	Pattern_parm *ptPatParm = malloc(sizeof(Pattern_parm));
	memset(ptPatParm, 0, sizeof(Pattern_parm));
	ptPatParm->src_addr[0] = malloc(2560*2560);
	//ptPatParm->dst_addr[0] = malloc(1920*1080);

	ptPatParm->src_addr[0] = (unsigned char *)((((unsigned long)ptPatParm->src_addr[0] + 4095) >> 12) << 12);
	//ptPatParm->dst_addr[0] = (unsigned char *)((((unsigned long)ptPatParm->dst_addr[0] + 4095) >> 12) << 12);
	ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();

	ptPatParm->ptImgparam = malloc(sizeof(struct mtk_imghal_param));
	
    hwtcon_get_panel_info(&ptPatParm->panel_width, &ptPatParm->panel_height);
    printf("hxp, panel w:%d, panel h:%d\n", ptPatParm->panel_width, ptPatParm->panel_height);
    
	#if DEBUG_WRITE_BUFFER
	FILE *fsrcdes = NULL;
	fsrcdes = fopen("/data/src_buffer.bin", "wb+");
	if(!fsrcdes) {
		printf("<GRAPHIC>open src file error!\n");
		return 0;
	}
	FILE *fdstdes = NULL;
	fdstdes = fopen("/data/dst_buffer.bin", "wb+");
	if(!fdstdes) {
		printf("<GRAPHIC>open dst file error!\n");
		return 0;
	}
	#endif
	char **argv_copy = argv;
	if (argc > 1 ) {
 		while ((option = getopt(argc, argv_copy, "u:t:g:p:ca:b:")) != -1) {
			//u->show ui, t->show text, g->show graph, p->show picture, c->clear screen
 			switch (option) {
				case 'u':
    			{
    				if(strlen(optarg)) {
    					ptPatParm->v4l2_dst_color_fmt = atoi(optarg);
    					snprintf(ptPatParm->input_buf, sizeof(ptPatParm->input_buf), "%s", argv[optind]); 
#if 0
    					ptPatParm->iCropType = atoi(argv[optind + 1]);
    					ptPatParm->rCropRegion.x_point = atoi(argv[optind + 2]);
    					ptPatParm->rCropRegion.y_point = atoi(argv[optind + 3]);
    					ptPatParm->rCropRegion.width = atoi(argv[optind + 4]);
    					ptPatParm->rCropRegion.height = atoi(argv[optind + 5]);
    					ptPatParm->iRotate = atoi(argv[optind + 6]);
    					ptPatParm->iFlip = atoi(argv[optind + 7]);
    					ptPatParm->rDisplayRegion.x_point = atoi(argv[optind + 8]);
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind + 9]);
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 10]);
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 11]);
#else
    					ptPatParm->rDisplayRegion.x_point = atoi(argv[optind + 1]);
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind + 2]);
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 3]);
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 4]);
    					ptPatParm->update_mode = atoi(argv[optind + 5]);
    					ptPatParm->waveform_mode = atoi(argv[optind + 6]);
    					ptPatParm->temperature = atoi(argv[optind + 7]);
#endif
    				}
    				argi++;
    				printf("<UI>input_buf:%s, v4l2_dst_color_fmt:%d, update_mode:%d, waveform_mode:%d, temperature:%d, rCropRegion:%d,%d,%d,%d\n", 
    					ptPatParm->input_buf,
    					ptPatParm->v4l2_dst_color_fmt,
    					ptPatParm->update_mode,
    					ptPatParm->waveform_mode,
    					ptPatParm->temperature,
    					ptPatParm->rDisplayRegion.x_point,
    					ptPatParm->rDisplayRegion.y_point,
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);
                    if ((ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width) ||
                        (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height))
                    {
                        printf("WANING:x + w > panel_w or y + h > panel_h\n");
                        goto finish;
                    }

    				ret = draw_ui(ptPatParm);
    				if (ret < 0) {
    					printf("draw_ui return error ret : %d\n", ret);
    					return -1;
    				}

    				//use_hw_engine = HW_ENGINE_MDP;
    				break;
    			}
		   case 't':
		        {
    				if(strlen(optarg)) {
    					ptPatParm->v4l2_dst_color_fmt = atoi(optarg);
    					snprintf(ptPatParm->input_buf, sizeof(ptPatParm->input_buf), "%s", argv[optind]); 
    					ptPatParm->rDisplayRegion.x_point = atoi(argv[optind + 1]); 
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind + 2]); 
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 3]); 
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 4]); 
                        ptPatParm->update_mode = atoi(argv[optind + 5]);
                        ptPatParm->waveform_mode = atoi(argv[optind + 6]);
    					ptPatParm->temperature = atoi(argv[optind + 7]);
    				}
    				argi++;
    				printf("<TEXT>input_buf:%s, v4l2_dst_color_fmt:%d, update_mode:%d, waveform_mode:%d, temperature:%d, rDisplayRegion:%d,%d,%d,%d\n",
    				ptPatParm->input_buf,
    				ptPatParm->v4l2_dst_color_fmt,
    				ptPatParm->update_mode,
    				ptPatParm->waveform_mode,
    				ptPatParm->temperature,
    				ptPatParm->rDisplayRegion.x_point,
    				ptPatParm->rDisplayRegion.y_point,
    				ptPatParm->rDisplayRegion.width,
    				ptPatParm->rDisplayRegion.height);
                    if ((ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width) ||
                        (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height))
                    {
                        printf("WANING:x + w > panel_w or y + h > panel_h\n");
                        goto finish;
                    }

    				ret = draw_text(ptPatParm);
    				if (ret < 0) {
    					printf("<TEXT>draw_text return error ret : %d\n", ret);
    					return -1;
    				}
    				ptPatParm->v4l2_src_color_fmt = V4L2_PIX_FMT_Y8;
    				//ptPatParm->v4l2_dst_color_fmt = V4L2_PIX_FMT_Y4_M0;
    				break;
				}
			case 'g':
			    {
    				if(strlen(optarg)) {
    					ptPatParm->iRowDirection = atoi(optarg);
    					ptPatParm->eImgFormat = atoi(argv[optind]);
    					ptPatParm->rDisplayRegion.x_point = atoi(argv[optind + 1]); 
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind + 2]); 
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 3]); 
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 4]);
                        ptPatParm->v4l2_dst_color_fmt = atoi(argv[optind + 5]);
                        ptPatParm->update_mode = atoi(argv[optind + 6]);
                        ptPatParm->waveform_mode = atoi(argv[optind + 7]);
    					ptPatParm->temperature = atoi(argv[optind + 8]);
    				}
    				argi++;
    				printf("<GRAPHIC>eImgFormat:%d, iRowDirection:%d, update_mode:%d, waveform_mode:%d, temperature:%d, dstfmt:%d, rDisplayRegion:%d,%d,%d,%d\n",
    					ptPatParm->eImgFormat,
    					ptPatParm->iRowDirection,
    					ptPatParm->update_mode,
    					ptPatParm->waveform_mode,
                        ptPatParm->temperature,
    					ptPatParm->v4l2_dst_color_fmt,
    					ptPatParm->rDisplayRegion.x_point,
    					ptPatParm->rDisplayRegion.y_point,
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);
                    if ((ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width) ||
                        (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height))
                    {
                        printf("WANING:x + w > panel_w or y + h > panel_h\n");
                        goto finish;
                    }

    				ret = draw_graphic(ptPatParm);
    				if (ret < 0) {
    					printf("<GRAPHIC>draw_graphic return error ret : %d\n", ret);
    					return -1;
    				}
    				ptPatParm->v4l2_src_color_fmt = V4L2_PIX_FMT_Y8;
    				//ptPatParm->v4l2_dst_color_fmt = V4L2_PIX_FMT_Y4_M0;
    				break;
				}
		    case 'p':
			    {
    				if(strlen(optarg)) {
    					ptPatParm->iCollisionCase = atoi(optarg);
    					ptPatParm->rDisplayRegion.x_point = atoi(argv[optind]);
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind + 1]); 
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 2]); 
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 3]);
                        ptPatParm->update_mode = atoi(argv[optind + 4]);
                        ptPatParm->waveform_mode = atoi(argv[optind + 5]);
    					ptPatParm->temperature = atoi(argv[optind + 6]);
    				}
    				argi++;
    				printf("<Collision>iCollisionCase:%d, update_mode:%d, waveform_mode:%d, temperature:%d, rDisplayRegion:%d,%d,%d,%d\n",
    					ptPatParm->iCollisionCase,
    					ptPatParm->update_mode,
    					ptPatParm->waveform_mode,
                        ptPatParm->temperature,
    					ptPatParm->rDisplayRegion.x_point,
    					ptPatParm->rDisplayRegion.y_point,
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);
    			    if ((ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width) ||
    			        (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height))
    			    {
    			        printf("WANING:x + w > panel_w or y + h > panel_h\n");
    			        goto finish;
    			    }
                    if (0 == ptPatParm->iCollisionCase)
    				{
         				for (i = 0; i < ptPatParm->panel_width; i++){
    				        for (j = 0; j< ptPatParm->panel_height; j++){
    				           if(i >= ptPatParm->rDisplayRegion.x_point && 
    				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
    				              j >= ptPatParm->rDisplayRegion.y_point &&
    				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
    				           {
         					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0xAA;
         					   }
         				   }
         				}
         				pass_hwtcon_display(ptPatParm);
         				
                        ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
                        printf("sleep 10ms before\n");
                        
                        usleep(10000);//delay 10ms
                        
                        printf("sleep 10ms after\n");
                         
         				ptPatParm->rDisplayRegion.x_point = ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width / 2;
         				ptPatParm->rDisplayRegion.y_point = ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height / 2;
         				for (i = 0; i < ptPatParm->panel_width; i++){
    				        for (j = 0; j< ptPatParm->panel_height; j++){
    				           if(i >= ptPatParm->rDisplayRegion.x_point && 
    				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
    				              j >= ptPatParm->rDisplayRegion.y_point &&
    				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
    				           {
         					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0x00;
         					   }
         				   }
         				}
         				printf("<Collision>x_point:%d, y_point:%d\n",
         					ptPatParm->rDisplayRegion.x_point,
         					ptPatParm->rDisplayRegion.y_point);

         				if (ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width)
         				{
         				   ptPatParm->rDisplayRegion.width = ptPatParm->panel_width - ptPatParm->rDisplayRegion.x_point;
         				   printf("x_point + display width > panel width\n");
         				}
         				if (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height)
         				{
         				   ptPatParm->rDisplayRegion.height = ptPatParm->panel_height - ptPatParm->rDisplayRegion.y_point;
         				   printf("y_point + display height > panel height\n");
         				}
         				pass_hwtcon_display(ptPatParm);
         				goto finish;
    				} 
    				else if (1 == ptPatParm->iCollisionCase)
    				{
         				for (i = 0; i < ptPatParm->panel_width; i++){
    				        for (j = 0; j< ptPatParm->panel_height; j++){
    				           if(i >= ptPatParm->rDisplayRegion.x_point && 
    				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
    				              j >= ptPatParm->rDisplayRegion.y_point &&
    				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
    				           {
         					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0xAA;
         					   }
         				   }
         				}
         				pass_hwtcon_display(ptPatParm);
         				
                        ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
                        printf("sleep 10ms before\n");
                        
                        usleep(10000);//delay 10ms
                        
                        printf("sleep 10ms after\n");
                         
         				ptPatParm->rDisplayRegion.x_point = ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width + 10;
         				ptPatParm->rDisplayRegion.y_point = ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height + 10;
         				for (i = 0; i < ptPatParm->panel_width; i++){
    				        for (j = 0; j< ptPatParm->panel_height; j++){
    				           if(i >= ptPatParm->rDisplayRegion.x_point && 
    				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
    				              j >= ptPatParm->rDisplayRegion.y_point &&
    				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
    				           {
         					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0xAA;
         					   }
         				   }
         				}
         				printf("<B>x_point:%d, y_point:%d\n",
         					ptPatParm->rDisplayRegion.x_point,
         					ptPatParm->rDisplayRegion.y_point);
         				if (ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width)
         				{
         				   ptPatParm->rDisplayRegion.width = ptPatParm->panel_width - ptPatParm->rDisplayRegion.x_point;
         				   printf("x_point + display width > panel width\n");
         				}
         				if (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height)
         				{
         				   ptPatParm->rDisplayRegion.height = ptPatParm->panel_height - ptPatParm->rDisplayRegion.y_point;
         				   printf("y_point + display height > panel height\n");
         				}
         				pass_hwtcon_display(ptPatParm);

         				ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
                        printf("sleep 250ms before\n");
                        
                        usleep(250000);//delay 250ms
                        
                        printf("sleep 250ms after\n");
                         
         				ptPatParm->rDisplayRegion.x_point = ptPatParm->rDisplayRegion.x_point - 10 - ptPatParm->rDisplayRegion.width / 2;
         				ptPatParm->rDisplayRegion.y_point = ptPatParm->rDisplayRegion.y_point - 10 - ptPatParm->rDisplayRegion.height / 2;
         				for (i = 0; i < ptPatParm->panel_width; i++){
    				        for (j = 0; j< ptPatParm->panel_height; j++){
    				           if(i >= ptPatParm->rDisplayRegion.x_point && 
    				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
    				              j >= ptPatParm->rDisplayRegion.y_point &&
    				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
    				           {
         					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0x00;
         					   }
         				   }
         				}
         				if (ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width)
         				{
         				   ptPatParm->rDisplayRegion.width = ptPatParm->panel_width - ptPatParm->rDisplayRegion.x_point;
         				   printf("x_point + display width > panel width\n");
         				}
         				if (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height)
         				{
         				   ptPatParm->rDisplayRegion.height = ptPatParm->panel_height - ptPatParm->rDisplayRegion.y_point;
         				   printf("y_point + display height > panel height\n");
         				}
         				printf("<C>x_point:%d, y_point:%d\n",
         					ptPatParm->rDisplayRegion.x_point,
         					ptPatParm->rDisplayRegion.y_point);
         				
         				pass_hwtcon_display(ptPatParm);
         				goto finish;
    				}
                    goto finish;
				}
		    case 'a':
			    {
			        if(strlen(optarg)) {
    					ptPatParm->iAutoWaveformCase = atoi(optarg);
                        ptPatParm->update_mode = atoi(argv[optind]);
                        ptPatParm->waveform_mode = atoi(argv[optind + 1]);
    					ptPatParm->temperature = atoi(argv[optind + 2]);
    				}
    				argi++;
    				ptPatParm->rDisplayRegion.x_point = 0;
    				ptPatParm->rDisplayRegion.y_point = 0;
    				ptPatParm->rDisplayRegion.width = ptPatParm->panel_width;
    				ptPatParm->rDisplayRegion.height = ptPatParm->panel_height;
    				
    				printf("<AutoWaveformCase>iAutoWaveformCase:%d, update_mode:%d, waveform_mode:%d, temperature:%d, rDisplayRegion:%d,%d,%d,%d\n",
    					ptPatParm->iAutoWaveformCase,
    					ptPatParm->update_mode,
    					ptPatParm->waveform_mode,
                        ptPatParm->temperature,
    					ptPatParm->rDisplayRegion.x_point,
    					ptPatParm->rDisplayRegion.y_point,
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);

    			    
    			    //0 Auto waveform Matrix test
    				if (0 == ptPatParm->iAutoWaveformCase) //(4 > ptPatParm->iAutoWaveformCase)//
    				{
    				    for (i = 0; i < 4; i++)
    				    {
    				        for (j = 0; j < 4; j++)
    				        {
             				    ptPatParm->waveform_mode = 2; //use gc16 wave mode
        				        draw_graphicType(ptPatParm, i);
                                wavemode1 = ptPatParm->waveform_mode;
             				    pass_hwtcon_display(ptPatParm);
             				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
             				    getchar();
                                //usleep(1000000);//delay 500ms
             				    draw_graphicType(ptPatParm, j);
             				    ptPatParm->waveform_mode = 15; //use Auto wave mode
             				    pass_hwtcon_display(ptPatParm);
             				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
             				    printf("grey_level %d:%d, waveform_mode %s:%s\n", 
             				              map_grey_level(i), map_grey_level(j), 
             				              map_wave_mode(wavemode1), map_wave_mode(ptPatParm->waveform_mode));
             				    getchar();
                                //usleep(1000000);//delay 500ms
         				    }
         				}
         				goto finish;
    				}
    				//1 Auto waveform repeating test
    				else if (1 == ptPatParm->iAutoWaveformCase)
    				{
    				    for (i = 0; i < 4; i++)
    				    {
    				        for (j = 0; j < 4; j++)
    				        {
    				            ptPatParm->waveform_mode = 2; //use gc16 wave mode
    				            wavemode1 = ptPatParm->waveform_mode;
        				        draw_graphicType(ptPatParm, i);
             				    printf("grey_level %d, waveform_mode %s\n", 
             				              map_grey_level(i), map_wave_mode(ptPatParm->waveform_mode));
             				    pass_hwtcon_display(ptPatParm);
             				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
             				    draw_graphicType(ptPatParm, j);
             				    ptPatParm->waveform_mode = 15; //use Auto wave mode
             				    wavemode2 = ptPatParm->waveform_mode;
             				    pass_hwtcon_display(ptPatParm);
             				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
             				    for (k = 0; k < 4; k++)
            				    {
            				        for (h = 0; h < 4; h++)
            				        {
                     				    ptPatParm->waveform_mode = 15; //use Auto wave mode
                				        draw_graphicType(ptPatParm, k);
                     				    pass_hwtcon_display(ptPatParm);
                     				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
                     				    draw_graphicType(ptPatParm, h);
                     				    pass_hwtcon_display(ptPatParm);
                     				    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();
                     				    printf("grey_level %d:%d, sub grey_level: %d:%d, waveform_mode %s:%s:%s:%s\n", 
             				              map_grey_level(i), map_grey_level(j), 
             				              map_grey_level(k), map_grey_level(h),
             				              map_wave_mode(wavemode1), map_wave_mode(wavemode2),
             				              map_wave_mode(ptPatParm->waveform_mode),
             				              map_wave_mode(ptPatParm->waveform_mode));
                 				    }
                 				}
             				    
         				    }
         				}
         				goto finish;
    				}
    				
                    goto finish;
				}
		    case 'b':
			    {
    				if(strlen(optarg)) {
    					//ptPatParm->iAutoWaveformCase = atoi(optarg);
    					ptPatParm->rDisplayRegion.x_point = atoi(optarg);
    					ptPatParm->rDisplayRegion.y_point = atoi(argv[optind]); 
    					ptPatParm->rDisplayRegion.width = atoi(argv[optind + 1]); 
    					ptPatParm->rDisplayRegion.height = atoi(argv[optind + 2]);
                        ptPatParm->update_mode = atoi(argv[optind + 3]);
                        ptPatParm->waveform_mode = atoi(argv[optind + 4]);
    					ptPatParm->temperature = atoi(argv[optind + 5]);
    				}
    				argi++;
    				printf("<AutoWaveformCase>update_mode:%d, waveform_mode:%d, temperature:%d, rDisplayRegion:%d,%d,%d,%d\n",
    					ptPatParm->update_mode,
    					ptPatParm->waveform_mode,
                        ptPatParm->temperature,
    					ptPatParm->rDisplayRegion.x_point,
    					ptPatParm->rDisplayRegion.y_point,
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);

    			    if ((ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width > ptPatParm->panel_width) ||
    			        (ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height > ptPatParm->panel_height))
    			    {
    			        printf("WANING:x + w > panel_w or y + h > panel_h\n");
    			        goto finish;
    			    }
				    //waveform_mode to set Auto ok
				    for (i = 0; i < ptPatParm->panel_width; i++){
				        for (j = 0; j< ptPatParm->panel_height; j++){
				           if(i >= ptPatParm->rDisplayRegion.x_point && 
				              i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width &&
				              j >= ptPatParm->rDisplayRegion.y_point &&
				              j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height)
				           {
     					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0xAA;
     					   }
     				   }
     				}
     				pass_hwtcon_display(ptPatParm);
     				
                    ptPatParm->dst_addr[0] = hwtcon_display_get_img_buffer();                         
                    
                    for (i = 0; i < ptPatParm->panel_width; i++){
				        for (j = 0; j< ptPatParm->panel_height; j++){
				           if((i >= ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width / 4) && 
				              (i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width * 3 / 4) &&
				              (j >= ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height / 4) &&
				              (j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height * 3 / 4))
				           {
     					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0x00;
     					   }
     					   else
     					   if((i >= ptPatParm->rDisplayRegion.x_point) && 
				              (i < ptPatParm->rDisplayRegion.x_point + ptPatParm->rDisplayRegion.width) &&
				              (j >= ptPatParm->rDisplayRegion.y_point) &&
				              (j < ptPatParm->rDisplayRegion.y_point + ptPatParm->rDisplayRegion.height))
				           {
     					       ptPatParm->dst_addr[0][j*ptPatParm->panel_width + i] = 0xAA;
     					   }
     				   }
     				}
     				
     				printf("<AutoWaveformCase 4>x_point:%d, y_point:%d, w: %d, h: %d\n",
     					ptPatParm->rDisplayRegion.x_point,
     					ptPatParm->rDisplayRegion.y_point,
     					ptPatParm->rDisplayRegion.width,
     					ptPatParm->rDisplayRegion.height);
     				getchar();

     				pass_hwtcon_display(ptPatParm);
     				goto finish;
				}
			case 'c':
			    {
    				//if(strlen(optarg)) {
    					//ptPatParm->rDisplayRegion.width = atoi(optarg); 
    					//ptPatParm->rDisplayRegion.height = atoi(argv[optind]); 
    				//}
    				argi++;
    				ptPatParm->rDisplayRegion.width = ptPatParm->panel_width;
    				ptPatParm->rDisplayRegion.height = ptPatParm->panel_height;
    				printf("<Clear>hxp clear width:%d, height:%d\n",
    					ptPatParm->rDisplayRegion.width,
    					ptPatParm->rDisplayRegion.height);

    				//clear Screen
    				for (i = 0; i < ptPatParm->rDisplayRegion.width * ptPatParm->rDisplayRegion.height; i++) {
    					ptPatParm->dst_addr[0][i] = 0xF0;
    				}
    				use_hw_engine = HW_ENGINE_TCON;
    				break;
    			}
			default:
				printf("invalid param:\n");
				use_hw_engine = 0;
				break;
		}
		ptPatParm->v4l2_dst_color_fmt = convert2colorfmt(ptPatParm->v4l2_dst_color_fmt);
        printf("after convert v4l2_dst_color_fmt : %d\n", ptPatParm->v4l2_dst_color_fmt);
        
		if (use_hw_engine) {
			//printf("To write to text.bin\n");
			printf("Image buffer Addr: before %p after %p region[%d %d %d %d]\n",
				ptPatParm->src_addr[0],
				ptPatParm->dst_addr[0],
				ptPatParm->rDisplayRegion.x_point,
				ptPatParm->rDisplayRegion.y_point,
				ptPatParm->rDisplayRegion.width,
				ptPatParm->rDisplayRegion.height);
			if (use_hw_engine & HW_ENGINE_MDP)
				pass_mdp(ptPatParm);
				
            #if DEBUG_WRITE_BUFFER
			fwrite(ptPatParm->src_addr[0], sizeof(int), ptPatParm->pixelSize, fsrcdes);
			fwrite(ptPatParm->dst_addr[0], sizeof(int), ptPatParm->rDisplayRegion.width * ptPatParm->rDisplayRegion.height, fdstdes);
			#endif
			if (use_hw_engine & HW_ENGINE_TCON)
				pass_hwtcon_display(ptPatParm);
		}
		
        #if DEBUG_WRITE_BUFFER
		fclose(fsrcdes);
		fclose(fdstdes);
        #endif
 		}
	}
	//while(1);
finish:
	return 0;
}



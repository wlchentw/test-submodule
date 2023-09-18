

#include "hwtcon_epd.h"

//#define debug_pts 12 // debug pixels information
//#define debug_error_pts // debug error pixels


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


int ntx_rgba_to_cfa(
	unsigned char *color_buffer,unsigned char *gray_buffer, 
	unsigned int width,	unsigned int height, unsigned int enhance,
	unsigned start_x,unsigned start_y,int iRot,
	int iPanelW,int iPanelH,int iFB_W,int iFB_H ,
	int iCFA_Rotate, int iCFA_type)
{
	unsigned char *pbSrc ;
	unsigned char *pbDest ;
	int x,y;
	int iColorIdx;
	int end_y;
	int end_x;

	switch(iRot) 
	{
	case 0: //0
		if(3==iCFA_Rotate) {
			end_y = start_y+height;
			end_x = start_x+width;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+(y*iPanelW+start_x);
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[x%EINK_CFA_GEN4_H][(iPanelH-y-1)%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
					pbSrc += 4;
					pbDest += 1;
				}
			}
		}
		else {
			end_y = start_y+height;
			end_x = start_x+width;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+(y*iPanelW+start_x);
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[y%EINK_CFA_GEN4_H][x%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
					pbSrc += 4;
					pbDest += 1;
				}
			}
		}
		break;
	case 2: //180
		if(3==iCFA_Rotate) {
			int iCFAx,iTemp;

			end_y = start_y+height;
			end_x = start_x+width;
			iTemp = iPanelW*iPanelH;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+iTemp-(y*iPanelW)-start_x-1;
				iCFAx = y%EINK_CFA_GEN4_W;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[(iPanelW-x-1)%EINK_CFA_GEN4_H][iCFAx];
#ifdef debug_error_pts//[
					if( (pbDest<gray_buffer)|| (pbSrc<color_buffer)) {
						printk(KERN_ERR"ERROR PTR (%d)NTX_SF 180 (%d,%d) @0x%p,i=%d  => %p \n",
							iCFA_Rotate,x,y,pbSrc,iColorIdx,pbDest);
					}
#endif //] debug_error_pts
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 180 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest -= 1;
				}
			}
		}
		else {
			int iCFAy,iTemp;

			end_y = start_y+height;
			end_x = start_x+width;
			iTemp = iPanelW*iPanelH;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+iTemp-(y*iPanelW)-start_x-1;
				iCFAy = (iPanelH-y-1)%EINK_CFA_GEN4_H;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[iCFAy][(iPanelW-x-1)%EINK_CFA_GEN4_W];
#ifdef debug_error_pts//[
					if( (pbDest<gray_buffer)|| (pbSrc<color_buffer)) {
						printk(KERN_ERR"ERROR PTR (%d)NTX_SF 180 (%d,%d) @0x%p,i=%d  => %p \n",
							iCFA_Rotate,x,y,pbSrc,iColorIdx,pbDest);
					}
#endif //] debug_error_pts
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 180 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest -= 1;
				}
			}
		}
		break;	
	case 1: //90
		if(3==iCFA_Rotate) {
			int iCFAy,iTemp;
			
			end_y = start_y+height;
			end_x = start_x+width;
			iTemp = start_x*iPanelW;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+(iPanelW-y+iTemp-1);
				iCFAy=(iPanelW-y-1)%EINK_CFA_GEN4_H;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[iCFAy][(iPanelH-x-1)%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 90 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d => %p \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx,pbDest);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest += iPanelW;
				}
			}
		}
		else {
			int iCFAx,iTemp;

			end_y = start_y+height;
			end_x = start_x+width;

//#define TEST_PORTRAIT	1 // 長邊先做
//#define TEST_PORTRAIT	2 // 長邊先做+寫入指標+
#define TEST_PORTRAIT	0 // 
                     	  
#if (TEST_PORTRAIT==1)//[
			// 長邊先做.
			iTemp = start_y*iPanelW;
			for(x=start_x;x<end_x;x++) {
				pbSrc = color_buffer+(start_y*(iFB_W<<2)+(x<<2));
				pbDest = gray_buffer+iPanelW-start_y+x*iPanelW-1;
				iCFAx = x%EINK_CFA_GEN4_H;
				for(y=start_y;y<end_y;y++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[iCFAx][(iPanelW-y-1)%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
					pbSrc += iFB_W<<2;
					pbDest -= 1;
				}
			}
#elif (TEST_PORTRAIT==2)//[
			// 長邊先做.
			iTemp = start_y*iPanelW;
			for(x=start_x;x<end_x;x++) {
				pbSrc = color_buffer+(iFB_H-start_y-1)*(iFB_W<<2)+(x<<2);
				pbDest = gray_buffer+start_y+x*iPanelW-1;
				iCFAx = x%EINK_CFA_GEN4_H;
				for(y=end_y-1;y>=start_y;y--) {
					iColorIdx = gcCFA_gen4_rgba_maskA[iCFAx][(iPanelW-y-1)%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
					pbSrc -= iFB_W<<2;
					pbDest += 1;
				}
			}
#else//][!
			iTemp = start_x*iPanelW;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+(iPanelW-y+iTemp-1);
				iCFAx = (iPanelW-y-1)%EINK_CFA_GEN4_W;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[x%EINK_CFA_GEN4_H][iCFAx];
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 90 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d  => %p \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx,pbDest);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest += iPanelW;
				}
			}
#endif //]
		}
		break;	
	case 3: //270
		if(3==iCFA_Rotate) {
			int iCFAy,iTemp,iTemp2;

			end_y = start_y+height;
			end_x = start_x+width;
			iTemp = iPanelW*iPanelH;
			iTemp2 = start_x*iPanelW;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+iTemp-iPanelW+y-iTemp2;
				iCFAy = y%EINK_CFA_GEN4_H;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[iCFAy][x%EINK_CFA_GEN4_W];
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 270 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d  => %p  \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx,pbDest);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest -= iPanelW;
				}
			}
		}
		else {
			int iCFAx,iTemp,iTemp2;

			end_y = start_y+height;
			end_x = start_x+width;
			iTemp = iPanelW*iPanelH;
			iTemp2 = start_x*iPanelW;
			for(y=start_y;y<end_y;y++) {
				pbSrc = color_buffer+(y*(iFB_W<<2)+(start_x<<2));
				pbDest = gray_buffer+iTemp-iPanelW+y-iTemp2;
				iCFAx = y%EINK_CFA_GEN4_W;
				for(x=start_x;x<end_x;x++) {
					iColorIdx = gcCFA_gen4_rgba_maskA[(iPanelH-x-1)%EINK_CFA_GEN4_H][iCFAx];
#ifdef debug_error_pts//[
					if(pbDest<gray_buffer) {
						printk("ERROR PTR (%d)NTX_SF 270 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d  => %p \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx,pbDest);
					}
#endif //] debug_error_pts
					*pbDest = pbSrc[iColorIdx];
#ifdef debug_pts //[
					if(x<debug_pts && y<debug_pts) {
						printk("(%d)NTX_SF 270 (%d,%d) @0x%p={0x%02x,0x%02x,0x%02x,0x%02x},i=%d  => %p \n",
							iCFA_Rotate,x,y,pbSrc,pbSrc[0],pbSrc[1],pbSrc[2],pbSrc[3],iColorIdx,pbDest);
					}
#endif //] debug_pts
					pbSrc += 4;
					pbDest -= iPanelW;
				}
			}
		}
		break;	
	}
	return 0;	
}




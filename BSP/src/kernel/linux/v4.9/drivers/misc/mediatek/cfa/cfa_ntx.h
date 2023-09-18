#ifndef _cfa_ntx_h //[
#define _cfa_ntx_h



int ntx_rgba_to_cfa(
	unsigned char *color_buffer,unsigned char *gray_buffer, 
	unsigned int width,	unsigned int height, unsigned int enhance,
	unsigned start_x,unsigned start_y,int iRot,
	int iPanelW,int iPanelH,int iFB_W,int iFB_H ,
	int iCFA_Rotate, int iCFA_type);



#endif //] _cfa_ntx_h


/*****************************************************************************
 * This program is practice project,
 * Do not distribute publicly.
 *****************************************************************************/
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_platform.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/cpu.h>

#include "libEink_Kaleido.h"
#include "cfa_driver.h"

#include "hwtcon_core.h"
#include "hwtcon_fb.h"
#include "hwtcon_file.h"
#include "hwtcon_driver.h"
#include "hwtcon_pipeline_config.h"
#include "hwtcon_rect.h"
#include "hwtcon_epd.h"
#include "epdfb_dc.h"
#include "cfa_ntx.h"

#ifdef HWTCON_CFA_MODE_EINK_AIE_S4 //[
	#define EINK_AIE_CFA_S4	1
#endif //] HWTCON_CFA_MODE_EINK_AIE_S4
	   
#ifdef HWTCON_CFA_MODE_EINK_AIE_S7
	#define EINK_AIE_CFA_S7	1
#endif //] HWTCON_CFA_MODE_EINK_AIE_S7

#ifdef HWTCON_CFA_MODE_EINK_AIE_S9
#define EINK_AIE_CFA_S9	1
#endif //] HWTCON_CFA_MODE_EINK_AIE_S9
	  
#ifdef EINK_AIE_CFA_S4//[
#include "cfa_aie_s4_bin.h"
#endif //]EINK_AIE_CFA_S4
#ifdef EINK_AIE_CFA_S7//[
#include "cfa_aie_s7_bin.h"
#endif //]EINK_AIE_CFA_S7
#ifdef EINK_AIE_CFA_S9//[
#include "cfa_aie_s9_bin.h"
#endif //]EINK_AIE_CFA_S9

#define CFA_TIME unsigned long long
#define CFA_TASK_WAIT_TIMEOUT_MS 10000
#define CFA_DATA_WAIT_TIMEOUT_MS 1000

#define CFA_LOG_LEVEL 0



//#define CFA_HANDLE_THREAD 2

#if (CFA_LOG_LEVEL>=1) //[
	#define CFA_LOG(string, args...) \
		pr_notice("[CFA LOG]"string"\n", ##args); 
#else //][!
	#define CFA_LOG(string, args...) \
		pr_debug("[CFA LOG]"string"\n", ##args); 
#endif //]

#define CFA_ERR(string, args...) \
		pr_notice("[CFA ERR]"string" @%s,%u\n", ##args, __func__, __LINE__)


typedef struct tagCFA_MODE{
	int mode;
	const char *name;
}CFA_MODE;


#define CFA_MODE_ITEM(name)	{HWTCON_CFA_MODE_##name,#name}

static CFA_MODE gtCFA_modesA[] = {
	CFA_MODE_ITEM(NONE),
	CFA_MODE_ITEM(EINK_NORMAL),
	CFA_MODE_ITEM(EINK_G1),
	CFA_MODE_ITEM(EINK_G2),
	CFA_MODE_ITEM(EINK_G0),
	CFA_MODE_ITEM(EINK_AIE_S4),
	CFA_MODE_ITEM(EINK_AIE_S7),
	CFA_MODE_ITEM(EINK_AIE_S9),
	CFA_MODE_ITEM(NTX),
	CFA_MODE_ITEM(NTX_SF),
};
#define CFA_MODES_TOTAL	(sizeof(gtCFA_modesA)/sizeof(gtCFA_modesA[0]))

char * cfa_get_mode_name_by_id(int cfa_mode_id)
{
	int i;
	char *pszRet = 0;
	for(i=0;i<CFA_MODES_TOTAL;i++) 
	{	
		if(gtCFA_modesA[i].mode==cfa_mode_id) {
			pszRet = (char *)gtCFA_modesA[i].name;
			break;
		}
	}
	return pszRet;
}

int cfa_get_id_by_mode_name(const char *cfa_mode_name)
{
	int i;
	int iRet = -1;
	for(i=0;i<CFA_MODES_TOTAL;i++) 
	{	
		if(0==strcmp(gtCFA_modesA[i].name,cfa_mode_name)) {
			iRet = gtCFA_modesA[i].mode;
			break;
		}
	}
	return iRet;
}

char * cfa_get_mode_name(int iIsFirst,int *O_piModeId)
{
	static int giModeNameIdx = 0;
	char *pszRet=0;

	if(iIsFirst) {
		giModeNameIdx = 0;
	}
	
	if( giModeNameIdx < CFA_MODES_TOTAL ) {
		pszRet = (char *)gtCFA_modesA[giModeNameIdx].name;
	}

	if(O_piModeId) {
		*O_piModeId = gtCFA_modesA[giModeNameIdx].mode;
	}

	giModeNameIdx++;

	return pszRet;
}

extern struct semaphore color_sem1, color_sem2;

static struct semaphore color_upper, color_lower;

static int cfa_major = 0;
static int cfa_minor = 0;
struct device *cfa_dev = NULL;

static struct cdev  *cfa_cdev;
static struct class  *cfa_class;

static struct rect  cfa_color_rec1 = {0};
static struct rect  cfa_color_rec2 = {0};
static char *cfa_color_input2 = NULL;
static char *cfa_color_output2 = NULL;

//static struct thread_data cfa_data;
static u8 *cfa_lut_va = 0;
static size_t cfa_lut_size;
static size_t cfa_lut_buf_size;


static int cfa_get_time_in_ms(CFA_TIME start, CFA_TIME end)
{
	CFA_TIME duration = end - start;
	return duration;
}

static CFA_TIME time_to_ms(void)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	return ((CFA_TIME) tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


void cfa_color_handle(unsigned char *color_buffer,unsigned char *gray_buffer, unsigned int width,
						unsigned int height, unsigned int enhance,unsigned int thread_id,
						unsigned start_x,unsigned start_y)
{
	int cfa_conv_chk = 0;
	int cfa_mode,task_cfa_mode;

	struct hwtcon_task *task = hwtcon_core_get_current_mdp_task();
	int iRot = hwtcon_fb_get_rotation();
	int iCFA_Rotate =(int) hw_tcon_get_cfa_panel_rotate();
	int iCFA_type =(int) hw_tcon_get_epd_type();
	int iPanelW = hw_tcon_get_edp_width();
	int iPanelH = hw_tcon_get_edp_height();
	int iFB_H = hwtcon_fb_get_height();
	int iFB_W = hwtcon_fb_get_width();

	CFA_LOG("thread_id=%d",thread_id);
	CFA_LOG("color_buffer@%p",color_buffer);
	CFA_LOG("gray_buffer@%p",gray_buffer);
	CFA_LOG("width=%d",width);
	CFA_LOG("height=%d",height);
	CFA_LOG("enhance=%d",enhance);
	CFA_LOG("rotate=%d",iRot);
	CFA_LOG("CFA rotate=%d",iCFA_Rotate);
	CFA_LOG("CFA type=%d",iCFA_type);
	CFA_LOG("PanelW=%d",iPanelW);
	CFA_LOG("PanelH=%d",iPanelH);
	CFA_LOG("image (x,y)=(%d,%d)",start_x,start_y);



	

	task_cfa_mode = HWTCON_FLAG_GET_CFA_MODE(task->update_data.flags);
	if(task_cfa_mode) {
		cfa_mode = task_cfa_mode;
	}
	else {
		cfa_mode = (hwtcon_fb_info()->cfa_mode<=0)?0:hwtcon_fb_info()->cfa_mode;
	}




	switch (cfa_mode)
	{
#ifdef HWTCON_CFA_MODE_NTX_SF //[
	case HWTCON_CFA_MODE_NTX_SF:
	{
		ntx_rgba_to_cfa(color_buffer,gray_buffer, width, height, enhance,
			start_x,start_y,iRot,iPanelW,iPanelH,iFB_W, iFB_H ,
			iCFA_Rotate,iCFA_type);
	}
	break;
#endif //] HWTCON_CFA_MODE_NTX_SF
#ifdef HWTCON_CFA_MODE_NTX //[
	case HWTCON_CFA_MODE_NTX:
	{
		// 
		EPDFB_DC_RET tRet;
		EPDFB_DC *ptDC_fb; // src
		EPDFB_DC *ptDC_img; // dest .
		u32 dwFlags;

		if(1==iCFA_type) {
			dwFlags = EPDFB_DC_FLAG_OFB_CFA_G4;
		}
		else if(2==iCFA_type) {
			dwFlags = EPDFB_DC_FLAG_OFB_CFA_G4_2;
		}
		else {
			dwFlags = EPDFB_DC_FLAG_OFB_CFA_G4;
		}
		
		if(1==iCFA_Rotate) {
			dwFlags |= EPDFB_DC_FLAG_OFB_CFA_R90;
		}
		else if(2==iCFA_Rotate) {
			dwFlags |= EPDFB_DC_FLAG_OFB_CFA_R180;
		}
		else if(3==iCFA_Rotate) {
			dwFlags |= EPDFB_DC_FLAG_OFB_CFA_R270;
		}
		

		ptDC_fb = epdfbdc_create_ex2(
			iFB_W,iFB_H,
			iFB_W,iFB_H,
			32,color_buffer,0) ;
		if(!ptDC_fb) {
			CFA_ERR("allocate fb dc fail w:%d h:%d",
				width,height);
		}
		ptDC_img = epdfbdc_create_ex2(
			iPanelW , iPanelH ,
			iPanelW , iPanelH ,
			8,gray_buffer,dwFlags);
		if(!ptDC_img) {
			CFA_ERR("allocate panel dc fail w:%d h:%d",
				iPanelW,iPanelH);
		}


		tRet = epdfbdc_put_dcimg(ptDC_img,ptDC_fb,
					iRot,start_x,start_y,width,height,
					start_x,start_y);

		

		if(EPDFB_DC_SUCCESS!=tRet) {
			CFA_ERR("epdfbdc_put_dcimg failed ! ret=%d\n ",tRet);
		}

		if(ptDC_fb) {
			epdfbdc_delete(ptDC_fb);
			ptDC_fb = 0 ;
		}
		if(ptDC_img) {
			epdfbdc_delete(ptDC_img);
			ptDC_img = 0 ;
		}

	}
	break;
#endif //] HWTCON_CFA_MODE_NTX_CFA

	default:


#if defined(HWTCON_CFA_MODE_EINK_NORMAL) || defined(HWTCON_CFA_MODE_EINK_G1) //[
	case HWTCON_CFA_MODE_EINK_G1:
		CFA_LOG("width=%d,height=%d,gain=1,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_Gain(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,1,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] defined(HWTCON_CFA_MODE_EINK_NORMAL) || defined(HWTCON_CFA_MODE_EINK_G1)
#if defined(HWTCON_CFA_MODE_EINK_G2) //[
	case HWTCON_CFA_MODE_EINK_G2:
		CFA_LOG("width=%d,height=%d,gain=2,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_Gain(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,2,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] defined(HWTCON_CFA_MODE_EINK_NORMAL) || defined(HWTCON_CFA_MODE_EINK_G1)
#if defined(HWTCON_CFA_MODE_EINK_G0) //[
	case HWTCON_CFA_MODE_EINK_G0:
		CFA_LOG("width=%d,height=%d,gain=0,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_Gain(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,0,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] defined(HWTCON_CFA_MODE_EINK_NORMAL) || defined(HWTCON_CFA_MODE_EINK_G1)



#ifdef HWTCON_CFA_MODE_EINK_AIE_S4 //[
	case HWTCON_CFA_MODE_EINK_AIE_S4:
		CFA_LOG("width=%d,height=%d,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_AIE(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,cfa_aie_s4_bin,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] HWTCON_CFA_MODE_EINK_AIE_S4
#ifdef HWTCON_CFA_MODE_EINK_AIE_S7 //[
	case HWTCON_CFA_MODE_EINK_AIE_S7:
		CFA_LOG("width=%d,height=%d,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_AIE(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,cfa_aie_s7_bin,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] HWTCON_CFA_MODE_EINK_AIE_S7
#ifdef HWTCON_CFA_MODE_EINK_AIE_S9 //[
	case HWTCON_CFA_MODE_EINK_AIE_S9:
		CFA_LOG("width=%d,height=%d,left=%d,top=%d,right=%d,bottom=%d,rotate=%d",
			iFB_W,height,start_x,start_y,start_x+width-1,start_y+height,iRot);
		cfa_conv_chk = eink_color_mapping_AIE(color_buffer,RGBA8888,gray_buffer,
				iFB_W,iFB_H,cfa_aie_s9_bin,start_x,start_y,start_x+width-1,start_y+height,iRot);
		break;
#endif //] HWTCON_CFA_MODE_EINK_AIE_S9

	}

	if (cfa_conv_chk)
		CFA_ERR("cfa convert thread %d fail", thread_id);
	else
		CFA_LOG("cfa convert thread %d success,cfa_mode=%d", 
			thread_id,cfa_mode);

}
EXPORT_SYMBOL(cfa_color_handle);

static int cfa_drv_open(struct inode *node, struct file *flip)
{
	CFA_ERR("open cfa device");
	return 0;
}

static int cfa_drv_close(struct inode *node, struct file *flip)
{
	CFA_ERR("close cfa device");
	return 0;
}

static ssize_t  cfa_drv_read(struct file *file, char __user * buf, size_t len, loff_t * ppos)
{
	CFA_ERR("read from cfa device");
	return 0;
}

static struct file_operations  cfa_ops = {
	.owner = THIS_MODULE,
	.open = cfa_drv_open,
	.release = cfa_drv_close,
	.read = cfa_drv_read,
};

static int cfa_handle_color_render_lower(void *ignore)
{
	int res = 0;

	while (1) {
COLOR_UPPER:
		res = down_timeout(&color_upper, msecs_to_jiffies(CFA_TASK_WAIT_TIMEOUT_MS));
		if (res)
			goto COLOR_UPPER;

#if 1
		cfa_color_handle(cfa_color_input2, cfa_color_output2, 
			cfa_color_rec2.width,cfa_color_rec2.height, 1,1,
			cfa_color_rec2.x,cfa_color_rec2.y);
#endif 
		up(&color_lower);
		CFA_LOG("threads cfa color handle 2/2");
	}
	return 0;
}

static void cfa_calculate_color_region(struct rect input_rect, struct rect *output_rect1,
	struct rect *output_rect2, char **input2,  char **output2)
{
	unsigned int buf_offset = 0;
/*
	int rotate = hwtcon_fb_get_rotation();


	if( (1==rotate) || (3==rotate) ) {
		output_rect1->width = (input_rect.height >> 1);
		output_rect1->height = input_rect.width;
		output_rect1->y = input_rect.x;
		output_rect1->x = input_rect.y;

		output_rect2->width = input_rect.height - output_rect1->width;
		output_rect2->height = input_rect.width;
		output_rect2->y = 0;
		output_rect2->x = input_rect.y;

		buf_offset = output_rect1->width + output_rect1->x;

		*input2 = hwtcon_fb_info()->fb_buffer_va + hwtcon_fb_get_width() * buf_offset * 4;
		*output2 = hwtcon_fb_info()->color_buffer_va + hwtcon_fb_get_height() * buf_offset;
	}
	else 
*/
	{
		output_rect1->height = (input_rect.height >> 1)
			- (((input_rect.height >> 1) + input_rect.y) % 3);
		output_rect1->width = input_rect.width;
		output_rect1->x = input_rect.x;
		output_rect1->y = input_rect.y;

		output_rect2->height = input_rect.height - output_rect1->height;
		output_rect2->width = input_rect.width;
		output_rect2->x = input_rect.x;
		output_rect2->y = output_rect1->y+output_rect1->height;

		buf_offset = output_rect1->height + output_rect1->y;

	//	*input2 = hwtcon_fb_info()->fb_buffer_va + hwtcon_fb_get_width() * buf_offset * 4;
	//	*output2 = hwtcon_fb_info()->color_buffer_va + hwtcon_fb_get_width() * buf_offset;
		*input2 = hwtcon_fb_info()->fb_buffer_va ;
		*output2 = hwtcon_fb_info()->color_buffer_va ;
	}


	CFA_LOG("output_rect1 x:%d y:%d w:%d h:%d", output_rect1->x, output_rect1->y,
		output_rect1->width, output_rect1->height);
	CFA_LOG("output_rect2 x:%d y:%d w:%d h:%d", output_rect2->x, output_rect2->y,
		output_rect2->width, output_rect2->height);

	return;
}

static int cfa_handle_color_mapping(void *ignore)
{
	CFA_TIME T1, T2;
	int res = 0;
	//struct rect  cfa_panel_region = {0};
	struct rect  cfa_fb_region = {0};
	while (1) {

COLOR_WAIT:
		res = down_timeout(&color_sem2, msecs_to_jiffies(CFA_TASK_WAIT_TIMEOUT_MS));
		if (res)
			goto COLOR_WAIT;

		CFA_LOG("cfa handle color mapping start ,threads=%d , online_cpus=%d",
				hwtcon_fb_info()->cfa_convert_threads,num_online_cpus());

		
		cfa_fb_region = hwtcon_core_get_task_user_region(hwtcon_core_get_current_mdp_task());
		CFA_LOG("cfa fb region x:%d y:%d w:%d h:%d", cfa_fb_region.x, cfa_fb_region.y,
			cfa_fb_region.width, cfa_fb_region.height);

#if 0
		cfa_panel_region = hwtcon_core_get_cfa_color_region();
		CFA_LOG("cfa panel region x:%d y:%d w:%d h:%d", cfa_panel_region.x, cfa_panel_region.y,
			cfa_panel_region.width, cfa_panel_region.height);
#endif

		T1 = time_to_ms();
		//if (CFA_HANDLE_THREAD == 1) 
		if( 1==hwtcon_fb_info()->cfa_convert_threads || 
			1==num_online_cpus() || 
			cfa_fb_region.height < 100)
		{
			cfa_color_handle(hwtcon_fb_info()->fb_buffer_va, 
				hwtcon_fb_info()->color_buffer_va,
				cfa_fb_region.width, cfa_fb_region.height, 1,0 ,cfa_fb_region.x,cfa_fb_region.y);
		} else {

			cfa_calculate_color_region(cfa_fb_region, &cfa_color_rec1, &cfa_color_rec2,
				&cfa_color_input2,  &cfa_color_output2);
			CFA_LOG("CFA input2[%p ] out put2[%p ]",
				cfa_color_input2, cfa_color_output2);

			up(&color_upper);

			cfa_color_handle(hwtcon_fb_info()->fb_buffer_va, 
				hwtcon_fb_info()->color_buffer_va,
				cfa_color_rec1.width, cfa_color_rec1.height, 1,0, cfa_color_rec1.x,cfa_color_rec1.y);
			CFA_LOG(" 2 threads cfa color handle  1/2");
			res = down_timeout(&color_lower, msecs_to_jiffies(CFA_DATA_WAIT_TIMEOUT_MS));

		}
		T2 = time_to_ms();
		up(&color_sem1);
		CFA_LOG("cfa color handle takes:%d ms", cfa_get_time_in_ms(T1, T2));
	}
	return 0;
}

static struct task_struct *pThread_color_upper;
static struct task_struct *pThread_color_lower;

static int cfa_kthread_create(void)
{
	struct sched_param param = {2};

	if (pThread_color_upper == NULL) {
		pThread_color_upper = kthread_create(cfa_handle_color_mapping, NULL,
			"cfa_color_mapping_upper");
		if (IS_ERR(pThread_color_upper)) {
			CFA_ERR("create thread cfa_color_mapping upper failed");
			return -1;
		}
		kthread_bind(pThread_color_upper, 1);
		wake_up_process(pThread_color_upper);
		/* adjust thread priority. */
		sched_setscheduler(pThread_color_upper, SCHED_RR, &param);
	}

	if (pThread_color_lower == NULL) {
		pThread_color_lower = kthread_create(cfa_handle_color_render_lower, NULL,
			"cfa_color_mapping_lower");
		if (IS_ERR(pThread_color_lower)) {
			CFA_ERR("create thread cfa_color_mapping lower failed");
			return -1;
		}
		kthread_bind(pThread_color_lower, 0);
		wake_up_process(pThread_color_lower);
		/* adjust thread priority. */
		sched_setscheduler(pThread_color_lower, SCHED_RR, &param);
	}

	return 0;
}


static int __init cfa_init(void)
{
	int err;
	dev_t dev;

	dev = MKDEV(cfa_major, cfa_minor);
	err = alloc_chrdev_region(&dev, 0, 1,"cfa");
	if (err) {
		CFA_ERR("alloc cfa chrdev region fail");
		return -1;
	}

	printk(KERN_ERR"Eink Kaleido Datecode : %d\n",Eink_get_DATE());

	cfa_major = MAJOR(dev);
	cfa_minor = MINOR(dev);

	cfa_cdev = cdev_alloc();
	if (cfa_cdev == NULL)
		CFA_ERR("cfa cdev alloc fail");

	cdev_init(cfa_cdev, &cfa_ops);
	cfa_cdev->owner = THIS_MODULE;
	err = cdev_add(cfa_cdev, dev, 1);
	if (err) {
		cfa_cdev = NULL;
		CFA_ERR("cfa cdev_add fail");
		return -1;
	}

	cfa_class = class_create(THIS_MODULE, "cfa_class");
	if (IS_ERR(cfa_class)) {
		cdev_del(cfa_cdev);
		CFA_ERR("cfa creat class fail");
		return -1;
	}

	cfa_dev = device_create(cfa_class, NULL, dev, NULL, "cfa"); /* /dev/cfa */
	if (IS_ERR(cfa_dev)) {
		class_destroy(cfa_class);
		cdev_del(cfa_cdev);
		CFA_ERR("cfa creat class fail");
		return -1;
	}

	CFA_ERR("cfa init successful");

	return 0;
}

static void __exit cfa_exit(void)
{
	if(cfa_lut_va) {
		vfree(cfa_lut_va);
		cfa_lut_va=0;
	}
	cfa_lut_buf_size = 0;
	cfa_lut_size = 0;

	class_destroy(cfa_class);
	cdev_del(cfa_cdev);

	pr_notice("cfa_exit");
}

int cfa_load_einklut(char *pszCFA_LUT_filename)
{

	int iPanelW,iPanelH;
	int iChkEinkInit;


	if(cfa_lut_va) {
		CFA_ERR("reinit skipped !!");	
	}

	iPanelW=(int)hw_tcon_get_edp_width();
	iPanelH=(int)hw_tcon_get_edp_height();

	
#if 1
	if(hw_tcon_get_epd_type())
	{

		// CFA panel .
		
		int cfa_lut_file_size = 0;


		/* load cfa lut data only exec once */
		cfa_lut_file_size = hwtcon_file_get_size(pszCFA_LUT_filename);
		if (cfa_lut_file_size == 0) {
			TCON_ERR("read file:%s fail", pszCFA_LUT_filename);
			return -1;
		}

		cfa_lut_buf_size = (iPanelW*iPanelH*6)+512;
		cfa_lut_va = vmalloc(cfa_lut_buf_size);
		if(!cfa_lut_va) {
			TCON_ERR("allocate cfa lut buffer %d bytes fail",cfa_lut_buf_size);
			return -1 ;
		}



		if (hwtcon_core_string_ends_with_gz(pszCFA_LUT_filename)) 
		{
			/* cfa_lut.gz file, need to decompress */
			char *zip_buffer = vmalloc(cfa_lut_file_size);

			if (zip_buffer == NULL) {
				TCON_ERR("allocate unzip buffer fail");
				return -1;
			}


			hwtcon_file_read_buffer(pszCFA_LUT_filename,
				zip_buffer,
				cfa_lut_file_size);

			hwtcon_file_unzip_buffer(zip_buffer,cfa_lut_va,
				cfa_lut_file_size,
				cfa_lut_buf_size,
				(unsigned long *)&cfa_lut_size);

			printk("%s: cfa lut depressed %d bytes ok\n",
				__func__,cfa_lut_size);

			vfree(zip_buffer);
		} else {
			TCON_LOG("read cfa_lut.bin size:%d total buffer size:%d",
					cfa_lut_file_size,
					cfa_lut_buf_size);
			hwtcon_file_read_buffer(pszCFA_LUT_filename,
				cfa_lut_va,
				cfa_lut_file_size);
		}
	}
#endif

	if(cfa_lut_va) {
		iChkEinkInit = EInk_Init(cfa_lut_va,cfa_lut_size,iPanelW,iPanelH);
		if(0!=iChkEinkInit) {
			CFA_ERR("Eink_Init failed (error=%d) !",iChkEinkInit);
		}
	}

	return 0;
}

int cfa_threads_init(void)
{
	int res;

	sema_init(&color_upper, 0);
	sema_init(&color_lower, 0);

	res = cfa_kthread_create();
	if (res)
			CFA_ERR("cfa creat kthreads fail");
	else
		CFA_LOG("cfa creat kthreads successfully");




	return 0;
}

int cfa_threads_deinit(void)
{


	if (pThread_color_upper) {
		kthread_stop(pThread_color_upper);
		pThread_color_upper = 0;
	}

	if (pThread_color_lower) {
		kthread_stop(pThread_color_lower);
		pThread_color_lower = 0;
	}




	return 0;
}



MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Software cfa");
late_initcall_sync(cfa_init);
module_exit(cfa_exit);


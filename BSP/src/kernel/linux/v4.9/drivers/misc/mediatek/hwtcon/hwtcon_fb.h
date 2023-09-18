/*****************************************************************************
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * Accelerometer Sensor Driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 *****************************************************************************/

#ifndef __HWTCON_FB_H__
#define __HWTCON_FB_H__
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/pm_wakeup.h>
#include <linux/pm_qos.h>


#include <linux/hwtcon_ioctl_cmd.h>
#include "hwtcon_def.h"
#include "hwtcon_rect.h"
#include "hwtcon_pipeline_config.h"

#define ALIGN_PANEL 16
#define FB_ALIGN(X)	 ALIGN(X, ALIGN_PANEL)

/* read temperature from sensor every 1 mins */
#define SENSOR_READ_INTERVAL_MS 60000

#define MAX_DEBUG_IMAGE_BUFFER_COUNT 1
#define MAX_FILE_NAME_LEN 100

enum LUT_RELEASE_ENUM {
	/* reserve for LUT 0 ~ 63 */
	LUT_RELEASE_TIME_START = 64,
	LUT_RELEASE_TIME_END = 65,
	LUT_RELEASE_MAX,
};

struct hwtcon_task_list {
	spinlock_t lock;
	struct list_head list;
};

struct wf_lut_info {
	bool busy;
	struct rect region;
	int priority;
	enum WAVEFORM_MODE_ENUM waveform_mode;
	struct hwtcon_task *task;
};

struct hwtcon_cache_buf {
	struct device *dev;
	size_t size;
	char *alloc_va;
	dma_addr_t dma_handle;
	struct sg_table *sgt;
	unsigned int n_pages;
	struct page **pages;
};


struct fb_private_info {
	struct regulator *regulator_vcore;
	struct pm_qos_request *vcore_req;

	/* frame buffer info: MDP's input */
	struct hwtcon_cache_buf *fb_cache_buf;	
	dma_addr_t fb_buffer_pa;
	char *fb_buffer_va;
	size_t fb_buffer_size;

	/* tmp buffer info*/
	dma_addr_t tmp_buffer_pa;
	char *tmp_buffer_va;
	size_t tmp_buffer_size;

	/* color buffer info: color mapping's output, MDP's input */
	struct hwtcon_cache_buf *color_cache_buf;
	dma_addr_t color_buffer_pa;
	char *color_buffer_va;
	size_t color_buffer_size;

	/* color info buffer info: color mapping's output, color regal's input */
	struct hwtcon_cache_buf *cinfo_cache_buf;
	dma_addr_t cinfo_buffer_pa;
	char *cinfo_buffer_va;
	size_t cinfo_buffer_size;

	/* img buffer info: MDP's output, PIPELINE's input */
	struct hwtcon_cache_buf *img_cache_buf;
	dma_addr_t img_buffer_pa;
	char *img_buffer_va;
	size_t img_buffer_size;

	/* temp image buffer info: regal buffer */
	struct hwtcon_cache_buf *temp_img_cache_buf;	
	dma_addr_t temp_img_buffer_pa;
	char *temp_img_buffer_va;
	size_t temp_img_buffer_size;

	/* debug image buffer info:
	 * reserve 10 image buffers for buffer save.
	 * only for debug issue.
	 */
	dma_addr_t debug_img_buffer_pa[MAX_DEBUG_IMAGE_BUFFER_COUNT];
	char *debug_img_buffer_va[MAX_DEBUG_IMAGE_BUFFER_COUNT];
	size_t debug_img_buffer_size[MAX_DEBUG_IMAGE_BUFFER_COUNT];
	char *debug_img_buffer_name[MAX_DEBUG_IMAGE_BUFFER_COUNT];
	int debug_img_buffer_counter;
	char debug_img_dump_path[MAX_FILE_NAME_LEN];

	/* working buffer info */
	dma_addr_t wb_buffer_pa;
	char *wb_buffer_va;
	size_t wb_buffer_size;

	/* waveform buffer info */
	dma_addr_t waveform_pa;
	char *waveform_va;
	size_t waveform_size;



	/* LUT management */
	spinlock_t lut_free_lock;
	u64 lut_free;
	spinlock_t lut_active_lock;
	u64 lut_active;

	/* marker management: record every marker data in hwtcon. */
	struct hwtcon_task_list fb_global_marker_list;
	struct completion wb_frame_done_completion;
	struct completion wf_lut_release_completion;

	/* update sequence */
	spinlock_t g_update_order_lock;
	u32 g_update_order;

	/* wake lock for pm_ops */
	struct wakeup_source wake_lock;

	/* eink TEMPERATURE */
	int temperature;
	//struct delayed_work read_sensor_work;

	/* first ioctl call */
	bool hwtcon_first_call;

	/*record whether power enabled */
	struct mutex mmsys_power_enable_lock;
	bool mmsys_power_enable;

	/* record whether clock enabled */
	spinlock_t hwtcon_clk_enable_lock;
	bool hwtcon_clk_enable;

	/* add a lock for wf_lut debug reg */
	spinlock_t hwtcon_tcon_reg_lock;

	/* for mmsys power close. */
	struct timer_list mmsys_power_timer;
	/* power down delay time.
	 * time elapse need to power down after HW frame done.
	 */
	int power_down_delay_ms;

	/* current temperature zone */
	int current_temp_zone;

	/* pipeline hw busy */
	bool pipeline_busy;

	/* ignore update request. */
	bool ignore_request;

	/* night mode or not */
	bool enable_night_mode;
	bool enable_night_mode_by_wfm;

	/* invert frame buffer or not */
	bool invert_fb;

	/* night mode waveform mode mapping */
	int night_mode_wfm_mapping;

	/* current night mode */
	int current_night_mode;

	/* add timer for each LUT update for error detect and recovery*/
	struct timer_list timer_lut_release[MAX_LUT_REGION_COUNT];
	/* slot for GCE to backup register */
	cmdqBackupSlotHandle lut_release_slot;

	/* all task list in hwtcon driver. */
	/* free task list */
	struct hwtcon_task_list free_task_list;
	/* acquire task done, wait for mdp process  */
	struct hwtcon_task_list wait_for_mdp_task_list;
	/* mdp process done */
	struct hwtcon_task_list mdp_done_task_list;

	/* pipeilne is processing */
	struct hwtcon_task_list pipeline_processing_task_list;
	/* pipeline process done */
	struct hwtcon_task_list pipeline_done_task_list;
	/* collision task list */
	struct hwtcon_task_list collision_task_list;

	/* wait queue for power state change */
	wait_queue_head_t power_state_change_wait_queue;

	/* wait queue for wait wf_lut release */
	wait_queue_head_t wf_lut_release_wait_queue;

	/* wait queue for trigger MDP & collision detect */
	wait_queue_head_t mdp_trigger_wait_queue;
	wait_queue_head_t pipeline_trigger_wait_queue;

	/* wait for task state change to spefic state. */
	wait_queue_head_t task_state_wait_queue;

	/* work for power down mmsys */
	struct work_struct wk_power_down_mmsys;
	/* thread for power down mmsys domain */
	struct workqueue_struct *wq_power_down_mmsys;

	/* thread for handle pipeline work done */
	struct workqueue_struct *wq_pipeline_written_done;
	/* thread for handle wf_lut display done. */
	struct workqueue_struct *wq_wf_lut_display_done;

	struct mutex update_queue_mutex;
	struct mutex image_buffer_access_mutex;

	struct device *dev;

	u32 mdp_src_format;

	unsigned gpio_xon;
	struct gpio_desc *gpio_xon_desc;
	struct hrtimer hrt_xon_on_ctrl;
	struct hrtimer hrt_xon_off_ctrl;
	u32 off_xon_1_delay_us; // pull high delay time when night mode power off . 
	u32 off_xon_0_delay_us; // pull low delay time when night mode power off .
	u32 off_xon_0_day_delay_us; // pull low delay time when day mode power off .
	u32 nm_xon_on_with_vcom; // night mode xon turn on with vcom . 
	u32 jiffies_xon_onoff;

	int cfa_mode;
	int cfa_convert_threads;

	HWTCON_TIME time_last_event_sync;
};

int hwtcon_fb_register_fb(struct platform_device *pdev);
int hwtcon_fb_unregister_fb(struct platform_device *pdev);
int hwtcon_fb_suspend(struct device *dev);
int hwtcon_fb_resume(struct device *dev);
int hwtcon_fb_poweroff(struct device *dev);

struct fb_private_info *hwtcon_fb_info(void);
u32 hwtcon_fb_get_rotation(void);
void hwtcon_fb_get_resolution(u32 *width, u32 *height);
u32 hwtcon_fb_get_grayscale(void);
void hwtcon_fb_flush_update(void);
int hwtcon_fb_check_update_data_invalid(
	struct hwtcon_update_data *update_data);
int hwtcon_fb_get_virtual_width(void);
int hwtcon_fb_get_width(void);
int hwtcon_fb_get_height(void);
bool hwtcon_core_use_night_mode(void);
bool hwtcon_core_invert_fb(void);

int hwtcon_fb_ioctl_set_night_mode(bool night_mode ,bool invert_fb,int nm_wfm_mapping);

#define XON_CTRLMODE_GET_CUR_STAT			7
#define XON_CTRLMODE_1_TMR_IS_PENDING		6
#define XON_CTRLMODE_0_TMR_IS_PENDING		5
#define XON_CTRLMODE_OFF_DEF_0_TMR_DAY		4
#define XON_CTRLMODE_OFF_DEF_0_TMR		3
#define XON_CTRLMODE_OFF_DEF_1_TMR		2
#define XON_CTRLMODE_1		1
#define XON_CTRLMODE_0	0
#define XON_CTRLMODE_CANCEL_1_TMR	(-1)
#define XON_CTRLMODE_CANCEL_0_TMR	(-2)
int hwtcon_fb_xon_ctrl(int iCtrlMode,unsigned long dwDelayus);

int hwtcon_fb_set_temperature(int iTemp);
struct hwtcon_cache_buf *hwtcon_fb_alloc_cached_mva_buf(struct device *dev,
	size_t size,
	const gfp_t flag);
void hwtcon_fb_free_cached_mva_buf(struct hwtcon_cache_buf *buf);
void hwtcon_fb_invalid_cache(struct hwtcon_cache_buf *buf);
void hwtcon_fb_clean_cache(struct hwtcon_cache_buf *buf);
u32 hwtcon_fb_get_cur_color_format(void);

#define HWTCON_CFA_MODE_NONE			0
#define HWTCON_CFA_MODE_EINK_NORMAL		1 // eink normal mode = HWTCON_CFA_MODE_EINK_G1
#define HWTCON_CFA_MODE_EINK_G1			1 // eink color enhance gain=1
#define HWTCON_CFA_MODE_EINK_AIE_S4		2 // eink AIE algorithm S4 .
#define HWTCON_CFA_MODE_EINK_AIE_S7		3 // eink AIE algorithm S7 .
#define HWTCON_CFA_MODE_EINK_AIE_S9		4 // eink AIE algorithm S9 .
#define HWTCON_CFA_MODE_EINK_G2			5 // eink color enhance gain=2
#define HWTCON_CFA_MODE_EINK_G0			6 // eink color enhance gain=0
#define HWTCON_CFA_MODE_NTX				10 // color mapping by epdfbdc .
#define HWTCON_CFA_MODE_NTX_SF			11 // simple fast color mapping .
//#define HWTCON_CFA_MODE_DEFAULT		HWTCON_CFA_MODE_EINK_CFA_NORMAL
#define HWTCON_CFA_MODE_DEFAULT		HWTCON_CFA_MODE_EINK_NORMAL
int hwtcon_fb_set_cfa_mode(int cfa_mode);

#define HWTCON_FLAG_GET_CFA_MODE(hwtcon_flags)	\
	(((hwtcon_flags)&HWTCON_FLAG_CFA_FLDS_MASK)>>8)
#define HWTCON_FLAG_SET_CFA_MODE(hwtcon_flags,cfa_mode)	\
	(hwtcon_flags)&=~HWTCON_FLAG_CFA_FLDS_MASK;\
	(hwtcon_flags)|=cfa_mode<<8


void hwtcon_fb_set_last_event_sync(const char *pszEvtMsg);
HWTCON_TIME hwtcon_fb_get_last_event_sync(void);

#endif /* __HWTCON_FB_H__ */

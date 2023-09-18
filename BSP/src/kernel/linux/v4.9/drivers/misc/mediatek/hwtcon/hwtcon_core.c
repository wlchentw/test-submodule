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

#include <linux/delay.h>
#include <linux/videodev2.h>
#include "hwtcon_core.h"
#include "hwtcon_def.h"
#include "hwtcon_fb.h"
#include "hwtcon_debug.h"
#include "hwtcon_file.h"
#include "hwtcon_driver.h"
#include "hwtcon_hal.h"
#include "fiti_core.h"
#include "hwtcon_epd.h"
#include "hwtcon_mdp.h"
#include "hwtcon_pipeline_config.h"
#include "hwtcon_wf_lut_config.h"
#include "hwtcon_dpi_config.h"
#include "hwtcon_rect.h"
#include "mtk_imgrz_ext.h"
#include "../../cfa/cfa_driver.h"
#include <linux/semaphore.h>

extern struct semaphore color_sem1, color_sem2;

static struct rect cfa_color_region = {0};

struct rect  hwtcon_core_get_cfa_color_region(void)
{
	return cfa_color_region;
}
EXPORT_SYMBOL(hwtcon_core_get_cfa_color_region);


void hwtcon_core_fiti_power_enable(bool enable)
{
	TCON_LOG("%s(%d)",__func__,enable);
	if (enable) {
		fiti_power_enable(true,0);
		return;
	}

	/* close fiti power
	 * check still remain task in task list.
	 * -> Yes: return directly
	 * -> No: close fiti power
	 */
	if (hwtcon_core_check_hwtcon_idle()) 
	{
		if(EINK_NO_POWER_DOWN==hwtcon_fb_info()->power_down_delay_ms) {
			TCON_WARN("PMIC no power down .");
			return ;
		}
		//fiti_power_enable(false,hwtcon_fb_info()->power_down_delay_ms);
		fiti_power_enable(false,1);
	}
}

static struct hwtcon_task_list *hwtcon_core_get_task_list_from_state(
	enum HWTCON_TASK_STATE state)
{
	switch (state) {
	case TASK_STATE_FREE:
		return &hwtcon_fb_info()->free_task_list;
	case TASK_STATE_WAIT_MDP_HANDLE:
		return &hwtcon_fb_info()->wait_for_mdp_task_list;
	case TASK_STAT_MDP_DONE:
		return &hwtcon_fb_info()->mdp_done_task_list;
	case TASK_STATE_PIPELINE_DONE:
		return &hwtcon_fb_info()->pipeline_done_task_list;
	case TASK_STATE_PIPELINE_PROCESS:
		return &hwtcon_fb_info()->pipeline_processing_task_list;
	case TASK_STATE_COLLISION:
		return &hwtcon_fb_info()->collision_task_list;
	default:
		TCON_ERR("invalid task state:%d", state);
		return NULL;
	}

	return NULL;
}

bool hwtcon_core_string_ends_with_gz(char *file_name)
{
	int str_len = strlen(file_name);

	if ((file_name[str_len - 3] == '.') &&
		(file_name[str_len - 2] == 'g') &&
		(file_name[str_len - 1] == 'z'))
		return true;
	return false;
}

static bool hwtcon_core_verify_unzip_buffer(void)
{
	/* this code is debug code, used to check decompress pass
	 * compare decompress buffer with golden buffer.
	 */
	char *file_name = hwtcon_debug_get_info()->golden_file_name;
	char *golden_buffer = NULL;
	int file_size = hwtcon_file_get_size(file_name);
	int i = 0;

	if (file_size == 0) {
		TCON_ERR("read file %s fail", file_name);
		return false;
	}
	golden_buffer = vmalloc(file_size);
	if (golden_buffer == NULL) {
		TCON_ERR("allocate golden buffer fail");
		return false;
	}

	hwtcon_file_read_buffer(file_name, golden_buffer, file_size);
	for (i = 0; i < file_size; i++)
		if (golden_buffer[i] != hwtcon_fb_info()->waveform_va[i])
			break;
	if (i == file_size)
		TCON_ERR("compare pass");
	else
		TCON_ERR("compare fail index:%d 0x%x - 0x%x",
			i,
			hwtcon_fb_info()->waveform_va[i],
			golden_buffer[i]);

	vfree(golden_buffer);

	return (i == file_size);
}

int hwtcon_core_load_init_setting_from_file(void)
{
	int wf_lut_file_size = 0;

	if (!hwtcon_fb_info()->hwtcon_first_call)
		return -1;

	/* load data only exec once */
	wf_lut_file_size = hwtcon_file_get_size(
		hwtcon_driver_get_wf_file_path());
	if (wf_lut_file_size == 0) {
		TCON_ERR("read file:%s fail", hwtcon_driver_get_wf_file_path());
		return -1;
	}
	if (hwtcon_core_string_ends_with_gz(
			hwtcon_driver_get_wf_file_path())) {
		/* wf_lut.gz file, need to decompress */
		char *zip_buffer = vmalloc(wf_lut_file_size);

		if (zip_buffer == NULL) {
			TCON_ERR("allocate unzip buffer fail");
			return -1;
		}

		hwtcon_file_read_buffer(hwtcon_driver_get_wf_file_path(),
			zip_buffer,
			wf_lut_file_size);

		hwtcon_file_unzip_buffer(zip_buffer,
			hwtcon_fb_info()->waveform_va,
			wf_lut_file_size,
			hwtcon_fb_info()->waveform_size,0);

		if (strlen(hwtcon_debug_get_info()->golden_file_name) != 0) {
			if (hwtcon_core_verify_unzip_buffer())
				TCON_ERR("compare golden pass");
			else
				TCON_ERR("compare golden fail");
		}
		vfree(zip_buffer);
	} else {
		TCON_LOG("read wf_lut.bin size:%d total buffer size:%d",
				wf_lut_file_size,
				hwtcon_fb_info()->waveform_size);
		hwtcon_file_read_buffer(hwtcon_driver_get_wf_file_path(),
			hwtcon_fb_info()->waveform_va,
			wf_lut_file_size);
	}

	hwtcon_fb_info()->hwtcon_first_call = false;
	wf_lut_parse_wf_file(hwtcon_fb_info()->waveform_va);
	if (!fiti_pmic_judge_power_on_going())
		fiti_setting_get_from_waveform(hwtcon_fb_info()->waveform_va);
	else {
		TCON_ERR("Can't access fiti when fiti in POWER_ON_GOING");
		hwtcon_fb_info()->hwtcon_first_call = false;
	}

	cfa_load_einklut(hwtcon_driver_get_cfa_file_path());

	return 0;
}

int hwtcon_core_get_task_count(struct list_head *header)
{
	struct hwtcon_task *task, *tmp;
	int count = 0;

	list_for_each_entry_safe(task, tmp,
		header, list) {
		count++;
	}
	return count;
}

void hwtcon_core_dump_task_list(struct list_head *header)
{

	struct hwtcon_task *task, *tmp;
	int count = 0;

	list_for_each_entry_safe(task, tmp,
		header, list) {
		TCON_ERR("index:%d task:0x%llx", count++, task->unique_id);
	}
}


static void hwtcon_core_change_task_state(struct hwtcon_task *task,
	enum HWTCON_TASK_STATE dst_state,
	bool src_lock,
	bool dst_lock,
	enum INSERT_LIST_ENUM insert_position)
{
	enum HWTCON_TASK_STATE src_state = task->state;

	/* which task_list is current task in ? */
	struct hwtcon_task_list *src_task_list =
		hwtcon_core_get_task_list_from_state(src_state);
	struct hwtcon_task_list *dst_task_list =
		hwtcon_core_get_task_list_from_state(dst_state);

	if (dst_state == TASK_STATE_FREE) {
		hwtcon_core_put_task_callback(task);
		wake_up(&hwtcon_fb_info()->task_state_wait_queue);
		return;
	}

	if (!src_task_list || !dst_task_list) {
		TCON_ERR("change task:0x%llx id:0x%x state[%d]->[%d] fail",
			task->unique_id,
			task->update_data.update_marker,
			src_state,
			dst_state);
		dump_stack();
		return;
	}

	if (src_task_list != dst_task_list) {
		unsigned long flags;

		/* remove task from former task list. */
		if (src_lock)
			spin_lock_irqsave(&src_task_list->lock, flags);
		list_del_init(&task->list);
		if (src_lock)
			spin_unlock_irqrestore(&src_task_list->lock, flags);

		/* add task to current task list. */
		if (dst_lock)
			spin_lock_irqsave(&dst_task_list->lock, flags);
		if (insert_position == INSERT_TO_HEAD)
			list_add(&task->list, &dst_task_list->list);
		else
			list_add_tail(&task->list, &dst_task_list->list);

		/* modify task state */
		task->state = dst_state;
		if (dst_lock)
			spin_unlock_irqrestore(&dst_task_list->lock, flags);
	}

	#if 0
	TCON_ERR("change task:0x%llx from:%d to %d",
		task->unique_id,
		task->state, dst_state);
	#endif

	wake_up(&hwtcon_fb_info()->task_state_wait_queue);
}

void hwtcon_core_put_task_with_lut_release(struct hwtcon_task *task)
{
	/* hwtcon_core_put_task */
	queue_work(hwtcon_fb_info()->wq_wf_lut_display_done,
		&task->work_display_done);
}

void hwtcon_core_put_task_callback(struct hwtcon_task *task)
{
	task->assign_lut = -1;
	/* hwtcon_core_put_task */
	queue_work(hwtcon_fb_info()->wq_wf_lut_display_done,
		&task->work_display_done);
}

void hwtcon_core_put_task(struct hwtcon_task *task)
{
	struct update_marker_struct *update_marker, *tmp;

	wake_up(&hwtcon_fb_info()->task_state_wait_queue);
	hwtcon_debug_record_printf(
		"task:0x%llx [%d %d %d %d]->[%d %d %d %d]",
		task->unique_id,
		hwtcon_core_get_task_user_region(task).x,
		hwtcon_core_get_task_user_region(task).y,
		hwtcon_core_get_task_user_region(task).width,
		hwtcon_core_get_task_user_region(task).height,
		hwtcon_core_get_task_region(task).x,
		hwtcon_core_get_task_region(task).y,
		hwtcon_core_get_task_region(task).width,
		hwtcon_core_get_task_region(task).height);
	hwtcon_debug_record_printf(
		" regal:%s mode:%02d->%s wf_cnt:%d ",
		(task->regal_status ==
			REGAL_STATUS_NON_REGAL) ? "False" : "True",
		task->update_data.waveform_mode,
		hwtcon_core_get_wf_mode_name(task->update_data.waveform_mode),
		wf_lut_get_waveform_len(hwtcon_core_read_temp_zone(),
			task->update_data.waveform_mode));
	hwtcon_debug_record_printf(
		"event %03d submit %03d trigger_mdp %03d mdp_done %03d ",
		(task->time_last_event_sync == 0 || task->time_submit == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
				task->time_last_event_sync, task->time_submit),
		(task->time_submit == 0 || task->time_trigger_mdp == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
				task->time_submit, task->time_trigger_mdp),
		(task->time_trigger_mdp == 0 || task->time_mdp_done == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
				task->time_trigger_mdp, task->time_mdp_done),
		(task->time_mdp_done == 0 ||
			task->time_trigger_pipeline == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_mdp_done, task->time_trigger_pipeline));
	hwtcon_debug_record_printf(
		"trigger_pipeline %03d pipeline_done %03d wait_power_good %03d ",
		(task->time_trigger_pipeline == 0 ||
			task->time_pipeline_done == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_trigger_pipeline, task->time_pipeline_done),
		(task->time_pipeline_done == 0 ||
			task->time_wait_fiti_power_good == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_pipeline_done,
			task->time_wait_fiti_power_good),
		(task->time_wait_fiti_power_good == 0 ||
			task->time_trigger_wf_lut == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_wait_fiti_power_good,
			task->time_trigger_wf_lut));

	hwtcon_debug_record_printf(
		"trigger_wf_lut %03d wf_lut_done total:%03d marker:%d",
		(task->time_trigger_wf_lut == 0 ||
			task->time_wf_lut_done == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_trigger_wf_lut, task->time_wf_lut_done),
		(task->time_submit == 0 || task->time_wf_lut_done == 0) ? -1 :
			hwtcon_hal_get_time_in_ms(
			task->time_submit, task->time_wf_lut_done),
		task->update_data.update_marker);

	hwtcon_debug_record_printf("\n");

	#ifndef MARKER_V2_ENABLE
	/* signal update_marker */
	list_for_each_entry_safe(update_marker, tmp,
		&task->marker_info_list.list, task_list) {
		unsigned long flags;
		bool release_marker = false;

		spin_lock_irqsave(
			&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
		list_del_init(&update_marker->global_list);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->fb_global_marker_list.lock, flags);

		complete(&update_marker->submit_completion);
		complete(&update_marker->update_completion);

		spin_lock_irqsave(&update_marker->marker_lock, flags);
		if (update_marker->has_waiters) {
			/* have waiters, free this completion on waiter thread*/
			update_marker->need_release = true;
		} else {
			/* no waiters, free here */
			release_marker = true;
		}
		spin_unlock_irqrestore(&update_marker->marker_lock, flags);

		TCON_LOG("[MARKER] update_completion: %d",
			update_marker->update_marker);

		if (release_marker)
			vfree(update_marker);
	}
	#else
	/* signal update_marker */
	list_for_each_entry_safe(update_marker, tmp,
		&hwtcon_fb_info()->fb_global_marker_list.list, global_list) {
		unsigned long flags;
		bool release_marker = false;
		bool signal_marker = false;

		spin_lock_irqsave(&update_marker->marker_lock, flags);
		if (task->assign_lut < MAX_LUT_REGION_COUNT)
			update_marker->lut_mask &= ~(1LL << task->assign_lut);
		signal_marker = ((update_marker->lut_mask == 0LL) &&
			(update_marker->marker_state ==
				MARKER_STATE_LUT_ASSIGNED));
		spin_unlock_irqrestore(&update_marker->marker_lock, flags);

		if (signal_marker) {
			HWTCON_TIME end_time = timeofday_ms();

			spin_lock_irqsave(
				&hwtcon_fb_info()->fb_global_marker_list.lock,
				flags);
			list_del_init(&update_marker->global_list);
			spin_unlock_irqrestore(
				&hwtcon_fb_info()->fb_global_marker_list.lock,
				flags);

			spin_lock_irqsave(&update_marker->marker_lock, flags);
			if (update_marker->has_waiters) {
				/* have waiters,
				 * free this completion on waiter thread
				 */
				update_marker->need_release = true;
			} else {
				/* no waiters, free here */
				release_marker = true;
			}
			spin_unlock_irqrestore(
				&update_marker->marker_lock, flags);

			TCON_LOG("[MARKER] update_completion: %d",
				update_marker->update_marker);
			TCON_LOG("[%d] update end marker=%d,",
				update_marker->update_marker,
				update_marker->update_marker);
			TCON_LOG("end time=%lld, time taken=%d ms",
				end_time,
				hwtcon_hal_get_time_in_ms(
					update_marker->start_time,
					end_time));

			complete(&update_marker->submit_completion);
			complete(&update_marker->update_completion);
		}

		if (release_marker)
			vfree(update_marker);
	}
	#endif

	if (task) {
		del_timer(&task->mdp_debug_timer);
		vfree(task);
	}

}

static void hwtcon_core_handle_task_display_done(struct work_struct *work_item)
{
	struct hwtcon_task *task = container_of(work_item,
		struct hwtcon_task,
		work_display_done);

	/* remove task to free task list. */
	hwtcon_core_put_task(task);
}

void hwtcon_core_handle_mdp_debug_timer_cb(unsigned long param)
{
	wake_up(&hwtcon_fb_info()->pipeline_trigger_wait_queue);
}

void hwtcon_core_handle_lut_release_timeout_cb(unsigned long param)
{
	struct hwtcon_task *task = NULL;
	struct hwtcon_task *tmp = NULL;
	int lut_id = param;
	unsigned long flags;
	int release_task_found = false;

	/* search the release task */
	spin_lock_irqsave(&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);
	list_for_each_entry_safe(task, tmp,
		&hwtcon_fb_info()->pipeline_done_task_list.list, list) {
		if (task->assign_lut == lut_id) {
			release_task_found = true;
			break;
		}
	}
	spin_unlock_irqrestore(&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);


	if (!release_task_found)
		TCON_ERR("LUT[%d] release timeout", lut_id);
	else
		TCON_ERR("LUT[%d] timeout on task marker[%d] region[%d %d %d %d]",
			task->assign_lut,
			task->update_data.update_marker,
			hwtcon_core_get_task_region(task).x,
			hwtcon_core_get_task_region(task).y,
			hwtcon_core_get_task_region(task).width,
			hwtcon_core_get_task_region(task).height);
	hwtcon_driver_handle_released_lut_flag(1LL << lut_id);
}

static void print_task_top_10(char *task_name,
			struct hwtcon_task_list *task_list)
{
	struct hwtcon_task *task = NULL;
	struct hwtcon_task *tmp = NULL;
	int count = 0;
	unsigned long flags = 0;

	TCON_ERR("dump top 10 tasks in list:%s begin", task_name);

	spin_lock_irqsave(&task_list->lock, flags);
	list_for_each_entry_safe(task, tmp,
		&task_list->list, list) {
		if (count > 10)
			break;
		TCON_ERR("task[%d] region[%d %d %d %d]",
			count++,
			task->update_data.update_region.left,
			task->update_data.update_region.top,
			task->update_data.update_region.width,
			task->update_data.update_region.height);
		TCON_ERR("update_mode[%d] regal[%d %d]  wf[%d]",
			task->update_data.update_mode,
			task->regal_status,
			task->regal_mode,
			task->update_data.waveform_mode);
	}
	spin_unlock_irqrestore(&task_list->lock, flags);

	TCON_ERR("dump top 10 tasks in list:%s end", task_name);

}

static struct hwtcon_task *hwtcon_core_get_task(
	struct hwtcon_update_data *update_data,
	bool create_marker)
{
	struct hwtcon_task *task = NULL;
	struct update_marker_struct *marker = NULL;
	unsigned long flags;

	task = vzalloc(sizeof(struct hwtcon_task));
	if (task == NULL) {
		static int count;

		TCON_ERR("vmalloc task fail %d", count);
		if (count == 0) {
			count++;
			TCON_ERR("lut free 0x%016llx active 0x%016llx:",
				hwtcon_fb_info()->lut_free,
				hwtcon_fb_info()->lut_active);
			TCON_ERR("release 0x%016llx",
				hwtcon_core_get_released_lut());
			TCON_ERR("hardware active:0x%08x 0x%08x",
				pp_read(WF_LUT_EN_STA1_VA),
				pp_read(WF_LUT_EN_STA0_VA));
			TCON_ERR("pipeline_processing_task_list count:%d",
				hwtcon_core_get_task_count(
				&hwtcon_fb_info()->
					pipeline_processing_task_list.list));
			TCON_ERR("pipeline_done_task_list count:%d",
				hwtcon_core_get_task_count(
				&hwtcon_fb_info()->
					pipeline_done_task_list.list));
			TCON_ERR("wait_for_mdp_task_list count:%d",
				hwtcon_core_get_task_count(
				&hwtcon_fb_info()->
					wait_for_mdp_task_list.list));
			TCON_ERR("mdp_done_task_list count:%d",
				hwtcon_core_get_task_count(
				&hwtcon_fb_info()->mdp_done_task_list.list));
			TCON_ERR("collision_task_list count:%d",
				hwtcon_core_get_task_count(
				&hwtcon_fb_info()->collision_task_list.list));

			print_task_top_10("pipeline_processing_task_list",
			&hwtcon_fb_info()->pipeline_processing_task_list);
			print_task_top_10("pipeline_done_task_list",
				&hwtcon_fb_info()->pipeline_done_task_list);
			print_task_top_10("wait_for_mdp_task_list",
				&hwtcon_fb_info()->wait_for_mdp_task_list);
			print_task_top_10("mdp_done_task_list",
				&hwtcon_fb_info()->mdp_done_task_list);
			print_task_top_10("collision_task_list",
				&hwtcon_fb_info()->collision_task_list);
		}
		return NULL;
	}

	task->update_data = *update_data;

	/* assign task's update order */
	spin_lock_irqsave(&hwtcon_fb_info()->g_update_order_lock, flags);
	task->update_order = hwtcon_fb_info()->g_update_order++;
	spin_unlock_irqrestore(&hwtcon_fb_info()->g_update_order_lock, flags);

	/* regal setting */
	if (hwtcon_core_use_regal(update_data, &task->regal_mode))
		task->regal_status = REGAL_STATUS_REGAL;
	else
		task->regal_status = REGAL_STATUS_NON_REGAL;

	task->night_mode = (u32) hwtcon_fb_info()->enable_night_mode;

	memset(&task->pipeline_info, 0, sizeof(task->pipeline_info));
	task->state = TASK_STATE_FREE;
	task->lut_dependency = 0LL;
	task->assign_lut = -1;
	INIT_LIST_HEAD(&task->list);
	spin_lock_init(&task->marker_info_list.lock);
	INIT_LIST_HEAD(&task->marker_info_list.list);

	if (create_marker) {
		/* allocate marker info */
		marker = hwtcon_core_alloc_update_marker();
		if (marker == NULL) {
			/* free task resource */
			vfree(task);
			return NULL;
		}

		marker->update_marker = update_data->update_marker;
		list_add_tail(&marker->task_list,
			&task->marker_info_list.list);
		spin_lock_irqsave(
			&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
		list_add_tail(&marker->global_list,
			&hwtcon_fb_info()->fb_global_marker_list.list);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
	}

	/* init task info */
	task->unique_id = sched_clock();
	task->time_last_event_sync = hwtcon_fb_get_last_event_sync();
	task->time_submit = timeofday_ms();


	INIT_WORK(&task->work_written_done,
		hwtcon_core_handle_task_written_done);

	INIT_WORK(&task->work_display_done,
		hwtcon_core_handle_task_display_done);

	/* start timer for mdp merge simulate debug. */
	setup_timer(&task->mdp_debug_timer,
		hwtcon_core_handle_mdp_debug_timer_cb,
		0L);

	return task;
}

/* check pipeline & wf_lut idle */
bool hwtcon_core_check_hwtcon_idle(void)
{
	if (hwtcon_fb_info()->lut_active != 0LL)
		return false;

	/* check if all task list Empty */
	if (!list_empty(&hwtcon_fb_info()->wait_for_mdp_task_list.list) ||
		!list_empty(&hwtcon_fb_info()->mdp_done_task_list.list) ||
		!list_empty(
		&hwtcon_fb_info()->pipeline_processing_task_list.list) ||
		!list_empty(&hwtcon_fb_info()->pipeline_done_task_list.list) ||
		!list_empty(&hwtcon_fb_info()->collision_task_list.list))
		return false;

	return true;
}

void hwtcon_core_handle_clock_disable(void)
{
	int iIsNightMode;

	struct hwtcon_task *task = hwtcon_core_get_current_update_task();

	if(task) {
		iIsNightMode = task->night_mode ? 1 : 0;
	}
	else {
		iIsNightMode = hwtcon_core_use_night_mode();
	}

	
	if (hwtcon_core_check_hwtcon_idle() == false)
		return;

	//if (hwtcon_fb_info()->power_down_delay_ms == EINK_NO_POWER_DOWN)
		//return;


	if( (0==hwtcon_fb_info()->power_down_delay_ms) || iIsNightMode) 
	{
		/* hwtcon_core_handle_mmsys_power_down */
		queue_work(hwtcon_fb_info()->wq_power_down_mmsys,
			&hwtcon_fb_info()->wk_power_down_mmsys);
	}
	else {
		TCON_LOG("start timer %d ms to power down mmsys",
			hwtcon_fb_info()->power_down_delay_ms);

		mod_timer(&hwtcon_fb_info()->mmsys_power_timer,
			jiffies + msecs_to_jiffies(
				hwtcon_fb_info()->power_down_delay_ms));
	}

}

void hwtcon_core_handle_mmsys_power_down(
	struct work_struct *work_item)
{
	hwtcon_driver_enable_mmsys_power(false);
}

void hwtcon_core_handle_mmsys_power_down_cb(unsigned long param)
{
	/* hwtcon_core_handle_mmsys_power_down */
	queue_work(hwtcon_fb_info()->wq_power_down_mmsys,
		&hwtcon_fb_info()->wk_power_down_mmsys);
}

int hwtcon_core_wait_for_task_triggered(u32 update_marker)
{
#if 1
	struct update_marker_struct *marker, *tmp;
	bool found = false;
	unsigned long flags;

	/* search global marker list */
	spin_lock_irqsave(&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
	list_for_each_entry_safe(marker, tmp,
		&hwtcon_fb_info()->fb_global_marker_list.list, global_list) {
		if (marker->update_marker == update_marker) {
			found = true;
			marker->has_waiters = true;
			break;
		}
	}
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->fb_global_marker_list.lock, flags);

	if (found) {
		bool release_marker = false;

		if (wait_for_completion_timeout(
				&marker->submit_completion,
				msecs_to_jiffies(
				HWTCON_TASK_WAIT_MARKER_TIMEOUT_MS)) == 0) {
			TCON_ERR("wait marker[%d] submit timeout:",
				marker->update_marker);
			TCON_ERR("mask[0x%016llx] state[%d]",
				marker->lut_mask,
				marker->marker_state);
		}

		spin_lock_irqsave(&marker->marker_lock, flags);
		marker->has_waiters = false;
		release_marker = marker->need_release;
		spin_unlock_irqrestore(&marker->marker_lock, flags);

		if (release_marker)
			vfree(marker);

	}
#endif
	return 0;
};


int hwtcon_core_wait_for_task_displayed(u32 update_marker)
{
#if 1
	struct update_marker_struct *marker, *tmp;
	bool found = false;
	unsigned long flags;

	TCON_LOG("wait marker[%d]",update_marker);
	/* search global marker list */
	spin_lock_irqsave(&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
	list_for_each_entry_safe(marker, tmp,
		&hwtcon_fb_info()->fb_global_marker_list.list, global_list) {
		if (marker->update_marker == update_marker) {
			found = true;
			marker->has_waiters = true;
			break;
		}
	}
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->fb_global_marker_list.lock, flags);

	if (found) {
		bool release_marker = false;

		TCON_LOG("waiting for marker[%d]",marker->update_marker);
		if (wait_for_completion_timeout(
				&marker->update_completion,
				msecs_to_jiffies(
				HWTCON_TASK_WAIT_MARKER_TIMEOUT_MS)) == 0) {
			TCON_ERR("wait marker[%d] update timeout:",
				marker->update_marker);
			TCON_ERR("mask[0x%016llx] state[%d]",
				marker->lut_mask,
				marker->marker_state);
		}
		TCON_LOG("waiting for marker[%d] done",marker->update_marker);

		spin_lock_irqsave(&marker->marker_lock, flags);
		marker->has_waiters = false;
		release_marker = marker->need_release;
		spin_unlock_irqrestore(&marker->marker_lock, flags);

		if (release_marker)
			vfree(marker);
	}
#endif
	return 0;
}

int hwtcon_core_convert_temperature(int temp)
{
	return wf_lut_waveform_get_temperature_index(temp);
}

int hwtcon_core_read_temperature(void)
{
	if (hwtcon_debug_get_info()->fixed_temperature == TEMP_USE_SENSOR)
		return hwtcon_fb_info()->temperature;

	/* used fixed temperature set by command */
	return hwtcon_debug_get_info()->fixed_temperature;
}

int hwtcon_core_read_temp_zone(void)
{
	hwtcon_core_load_init_setting_from_file();
	return hwtcon_core_convert_temperature(
		hwtcon_core_read_temperature());
}


int hwtcon_core_submit_task(struct hwtcon_update_data *update_data)
{
	struct hwtcon_task *task = NULL;

	hwtcon_core_load_init_setting_from_file();

	mutex_lock(&hwtcon_fb_info()->update_queue_mutex);

	TCON_LOG("wfm=%d,set nightmode by wfm=%d",
			update_data->waveform_mode,
			hwtcon_fb_info()->enable_night_mode_by_wfm);

	if( hwtcon_fb_info()->enable_night_mode_by_wfm ) {
		if( WAVEFORM_MODE_GCK16==update_data->waveform_mode || 
			WAVEFORM_MODE_GLKW16==update_data->waveform_mode) {
			hwtcon_fb_ioctl_set_night_mode(1,0,0);
		}
		else {
			hwtcon_fb_ioctl_set_night_mode(0,0,0);
		}
	}


	task = hwtcon_core_get_task(update_data, true);
	if (task == NULL) {
		TCON_ERR("hwtcon_core_get_task fail");
		mutex_unlock(&hwtcon_fb_info()->update_queue_mutex);
		return HWTCON_STATUS_GET_TASK_FAIL;
	}

	TCON_LOG("SUBMIT:task:0x%llx marker:%d wf_mode:%d(%s)",
		task->unique_id,
		task->update_data.update_marker,
		task->update_data.waveform_mode,
		hwtcon_core_get_wf_mode_name(update_data->waveform_mode));
	TCON_LOG("[%d] update start marker=%d, start time=%lld",
				task->update_data.update_marker,
				task->update_data.update_marker,
				task->time_submit);

	hwtcon_core_change_task_state(task,
		TASK_STATE_WAIT_MDP_HANDLE, false, true, INSERT_TO_TAIL);

	if (timer_pending(&hwtcon_fb_info()->mmsys_power_timer))
		del_timer(&hwtcon_fb_info()->mmsys_power_timer);

	/* enable fiti power */
	hwtcon_core_fiti_power_enable(true);

	wake_up(&hwtcon_fb_info()->mdp_trigger_wait_queue);

	mutex_unlock(&hwtcon_fb_info()->update_queue_mutex);

	return 0;
}



/*
 * 1. remove task from trigger_task_list.
 * 2. add task to wf_lut_task_list.
 * 3. modify task state.
 */
void hwtcon_core_handle_task_written_done(
	struct work_struct *work_item)
{
	struct hwtcon_task *task = container_of(work_item,
			struct hwtcon_task,
				work_written_done);

	/* dump working buffer */
	if (hwtcon_debug_get_info()->enable_dump_next_buffer>0) {
		if(hwtcon_fb_info()->debug_img_buffer_counter>0) {
			snprintf(hwtcon_fb_info()->debug_img_buffer_name[0],
				MAX_FILE_NAME_LEN,
			"%s/next_wb_%d_m%04d_%dx%dy%dw%dh.bin",
				hwtcon_fb_info()->debug_img_dump_path,
				hwtcon_fb_info()->debug_img_buffer_counter,
				task->update_data.update_marker,
				hwtcon_core_get_task_region(task).x,
				hwtcon_core_get_task_region(task).y,
				hwtcon_core_get_task_region(task).width,
				hwtcon_core_get_task_region(task).height);
			hwtcon_file_save_buffer(hwtcon_fb_info()->wb_buffer_va,
				hwtcon_fb_info()->wb_buffer_size,
				hwtcon_fb_info()->debug_img_buffer_name[0]);
		}
		else {
			hwtcon_debug_get_info()->enable_dump_next_buffer = 0;
		}
	}
	else {
		hwtcon_file_save_buffer(hwtcon_fb_info()->wb_buffer_va,
			hwtcon_fb_info()->wb_buffer_size, "/tmp/next_wb.bin");
		hwtcon_debug_get_info()->enable_dump_next_buffer = 0;
	}

	if (hwtcon_debug_get_info()->collision_debug) {
		enum WF_SLOT_ENUM slot = wf_lut_get_waveform_mode_slot(
				task->update_data.waveform_mode,
				task->night_mode);


		TCON_ERR("simulate collision scenario, force sleep %d ms",
				hwtcon_debug_get_info()->collision_debug);
		msleep(hwtcon_debug_get_info()->collision_debug);
		/* wait fiti power good */
		fiti_wait_power_good();
		/* trigger WF_LUT */
		TS_WF_LUT_set_lut_info(NULL,
			hwtcon_core_get_task_region(task).x,
			hwtcon_core_get_task_region(task).y,
			hwtcon_core_get_task_region(task).width,
			hwtcon_core_get_task_region(task).height,
			task->assign_lut,
			slot);
		lut_frame_count[task->assign_lut] = 0;
		lut_crc_all_zero[task->assign_lut] = true;
	}
}

void hwtcon_core_set_task_region(struct hwtcon_task *task, struct rect region)
{
	struct rect virtual_region = {0};

	if (hw_tcon_get_edp_fliph())
		region.x = hw_tcon_get_edp_width() - region.x - region.width;
	switch (hwtcon_fb_get_rotation()) {
	case HWTCON_ROTATE_0:
		virtual_region.x = region.x;
		virtual_region.y = region.y;
		virtual_region.width = region.width;
		virtual_region.height = region.height;
		break;
	case HWTCON_ROTATE_90:
		virtual_region.x = region.y;
		virtual_region.y = hw_tcon_get_edp_width() -
				region.x -
				region.width;
		virtual_region.width = region.height;
		virtual_region.height = region.width;
		break;
	case HWTCON_ROTATE_180:
		virtual_region.x =  hw_tcon_get_edp_width() -
					region.width - region.x;
		virtual_region.y = hw_tcon_get_edp_height() -
					region.height - region.y;
		virtual_region.width = region.width;
		virtual_region.height = region.height;
		break;
	case HWTCON_ROTATE_270:
		virtual_region.x = hw_tcon_get_edp_height() -
					region.y - region.height;
		virtual_region.y = region.x;
		virtual_region.width = region.height;
		virtual_region.height = region.width;
		break;
	default:
		TCON_ERR("invalid rotation:%d", hwtcon_fb_get_rotation());
		return;
	}

	task->update_data.update_region.left = virtual_region.x;
	task->update_data.update_region.top = virtual_region.y;
	task->update_data.update_region.width = virtual_region.width;
	task->update_data.update_region.height = virtual_region.height;

}

struct rect hwtcon_core_get_task_user_region(
	const struct hwtcon_task *task)
{
	struct rect region = {0};
		region.y = task->update_data.update_region.top;
		region.x = task->update_data.update_region.left;
		region.width = task->update_data.update_region.width;
		region.height = task->update_data.update_region.height;
	return region;
}

struct rect hwtcon_core_rotate_region(
	const struct hwtcon_rect *src_region, u32 rotation)
{
	struct rect region = {0};

	switch (rotation) {
	case HWTCON_ROTATE_0:
		region.x = src_region->left;
		region.y = src_region->top;
		region.width = src_region->width;
		region.height = src_region->height;
		break;
	case HWTCON_ROTATE_270:
		region.x = src_region->top;
		region.y = hw_tcon_get_edp_height() -
			src_region->left - src_region->width;
		region.width = src_region->height;
		region.height = src_region->width;
		break;
	case HWTCON_ROTATE_180:
		region.x = hw_tcon_get_edp_width() -
			src_region->width - src_region->left;
		region.y = hw_tcon_get_edp_height() -
			src_region->height - src_region->top;
		region.width = src_region->width;
		region.height = src_region->height;
		break;
	case HWTCON_ROTATE_90:
		region.x = hw_tcon_get_edp_width() -
				src_region->top -
				src_region->height;
		region.y = src_region->left;
		region.width = src_region->height;
		region.height = src_region->width;
		break;
	default:
		TCON_ERR("invalid rotation:%d", rotation);
		WARN(1, "invalid rotation:%d", rotation);
		break;
	}
	if (hw_tcon_get_edp_fliph())
		region.x = hw_tcon_get_edp_width() - region.x - region.width;
	return region;
}

struct rect hwtcon_core_get_update_data_region(
	const struct hwtcon_update_data *update_data)
{
	struct rect region = {0};
	struct hwtcon_rect buffer_region = {0};

	buffer_region.top = update_data->update_region.top;
	buffer_region.left = update_data->update_region.left;
	buffer_region.width = update_data->update_region.width;
	buffer_region.height = update_data->update_region.height;

	region = hwtcon_core_rotate_region(
			&buffer_region, hwtcon_fb_get_rotation());

	if (region.x < 0 ||
		region.y < 0 ||
		region.width <= 0 ||
		region.height <= 0) {
		TCON_ERR("invalid buffer region[%d %d %d %d]",
			buffer_region.left,
			buffer_region.top,
			buffer_region.width,
			buffer_region.height);
		TCON_ERR("rotation:%d region[%d %d %d %d]",
			hwtcon_fb_get_rotation(),
			region.x,
			region.y,
			region.width,
			region.height);
		TCON_ERR("panel[%d %d] flag:0x%08x",
			hw_tcon_get_edp_width(),
			hw_tcon_get_edp_height(),
			update_data->flags);
		dump_stack();
	}

	return region;
}

struct rect hwtcon_core_get_task_region(
	const struct hwtcon_task *task)
{
	return hwtcon_core_get_update_data_region(&task->update_data);
}

struct rect hwtcon_core_get_mdp_region(
	const struct hwtcon_task *task)
{
	struct rect region = {0};

	region.x = task->update_data.update_region.left;
	region.y = task->update_data.update_region.top;
	region.width = task->update_data.update_region.width;
	region.height = task->update_data.update_region.height;

	return region;
}

void hwtcon_core_get_task_buffer_info(
	const struct hwtcon_task *task,
	u32 *buffer_pa, u32 *buffer_width, u32 *buffer_height)
{
	if (buffer_pa)
		*buffer_pa = hwtcon_fb_info()->img_buffer_pa;
	if (buffer_width)
		*buffer_width = hw_tcon_get_edp_width();
	if (buffer_height)
		*buffer_height = hw_tcon_get_edp_height();
}

void hwtcon_core_get_mdp_input_buffer_info(
	const struct hwtcon_task *task,
	u32 *buffer_pa, u32 *buffer_width, u32 *buffer_height)
{

	if (hwtcon_mdp_need_cfa(task)) {
		if (buffer_pa)
			*buffer_pa = hwtcon_fb_info()->color_buffer_pa;
		if (buffer_width)
			*buffer_width = hwtcon_fb_get_width();
		if (buffer_height)
			*buffer_height = hwtcon_fb_get_height();
			TCON_LOG("MDP INPUT CFA");
	}  else {
		if (buffer_pa)
			*buffer_pa = hwtcon_fb_info()->fb_buffer_pa;
		if (buffer_width)
			*buffer_width = hwtcon_fb_get_width();
		if (buffer_height)
			*buffer_height = hwtcon_fb_get_height();
	}

}

int hwtcon_core_wait_all_task_done(void)
{
	int status = 0;

	status = wait_event_timeout(
			hwtcon_fb_info()->task_state_wait_queue,
			(hwtcon_core_check_hwtcon_idle() == true),
			msecs_to_jiffies(HWTCON_TASK_WAIT_MARKER_TIMEOUT_MS));
	/* wait timeout */
	if (status == 0) {
		TCON_ERR("wait all task done timeout count[%d %d %d %d %d]",
			hwtcon_core_get_task_count(
				&hwtcon_fb_info()->wait_for_mdp_task_list.list),
			hwtcon_core_get_task_count(
				&hwtcon_fb_info()->mdp_done_task_list.list),
			hwtcon_core_get_task_count(
			&hwtcon_fb_info()->pipeline_processing_task_list.list),
			hwtcon_core_get_task_count(
			&hwtcon_fb_info()->pipeline_done_task_list.list),
			hwtcon_core_get_task_count(
				&hwtcon_fb_info()->collision_task_list.list));
		TCON_ERR("pipeline status:0x%016llx",
			hwtcon_fb_info()->lut_active);
		return -1;
	}
	return 0;

}


bool hwtcon_core_can_merge_trigger_task_region(const struct rect *rect1,
	const struct rect *rect2,
	struct rect *merge_region)
{
	if (hwtcon_rect_check_relationship(rect1, rect2, merge_region) ==
			RECT_RELATION_CONTAIN)
		return true;

	return false;
}

bool hwtcon_core_can_merge_collision_task_region(const struct rect *rect1,
	const struct rect *rect2,
	struct rect *merge_region)
{

	if (hwtcon_rect_check_relationship(rect1, rect2, merge_region) ==
		RECT_RELATION_INTERSECT ||
		hwtcon_rect_check_relationship(rect1, rect2, merge_region) ==
		RECT_RELATION_CONTAIN)
		return true;

	return false;
}

void hwtcon_core_insert_task_to_collision_task_list(
				struct hwtcon_task *insert_task)
{
	unsigned long flags;
	bool find_a_merge = true;
	struct hwtcon_task *collision_task, *tmp = NULL;

	spin_lock_irqsave(&hwtcon_fb_info()->collision_task_list.lock, flags);
	/* merge */
	while (find_a_merge) {
		find_a_merge = false;
		list_for_each_entry_safe(collision_task, tmp,
			&hwtcon_fb_info()->collision_task_list.list, list) {
			struct rect task_region =
				hwtcon_core_get_task_region(insert_task);
			struct rect collision_task_region =
				hwtcon_core_get_task_region(collision_task);
			struct rect merge_region = {0};

			if (hwtcon_core_can_merge_collision_task_region(
			&task_region, &collision_task_region, &merge_region) &&
				(collision_task->update_data.update_mode ==
					insert_task->update_data.update_mode) &&
				(collision_task->regal_status ==
					insert_task->regal_status) &&
				(collision_task->regal_mode ==
					insert_task->regal_mode)) {
				find_a_merge = true;
				/* merge task & collision_task to task */
				hwtcon_core_set_task_region(
					insert_task, merge_region);

				insert_task->update_order =
					MIN(insert_task->update_order,
					collision_task->update_order);

				if (insert_task->update_data.waveform_mode !=
				collision_task->update_data.waveform_mode) {

					if(insert_task->night_mode) {
						if(UPDATE_MODE_PARTIAL==insert_task->update_data.update_mode) {
							insert_task->update_data.waveform_mode =
								WAVEFORM_MODE_GCK16;
						}
						else {
							insert_task->update_data.waveform_mode =
								WAVEFORM_MODE_GLKW16;
						}
					}
					else {
						if(UPDATE_MODE_PARTIAL==insert_task->update_data.update_mode) {
							insert_task->update_data.waveform_mode =
								WAVEFORM_MODE_GL16;
						}
						else {
							insert_task->update_data.waveform_mode =
								WAVEFORM_MODE_GC16;
						}
					}
				}

				insert_task->lut_dependency |=
					collision_task->lut_dependency;

				/* marker info: move collision_task marker info
				 * to insert_task
				 */
				list_splice_init(
					&collision_task->marker_info_list.list,
					&insert_task->marker_info_list.list);

				TCON_LOG("merge collision region[%d %d %d %d]",
					hwtcon_core_get_task_region(
						collision_task).x,
					hwtcon_core_get_task_region(
						collision_task).y,
					hwtcon_core_get_task_region(
						collision_task).width,
					hwtcon_core_get_task_region(
						collision_task).height);

				list_del_init(&collision_task->list);
				hwtcon_core_put_task_callback(collision_task);
			}
		}
	}
	TCON_LOG("insert task: 0x%llx [%d %d %d %d]",
		insert_task->unique_id,
		hwtcon_core_get_task_region(insert_task).x,
		hwtcon_core_get_task_region(insert_task).y,
		hwtcon_core_get_task_region(insert_task).width,
		hwtcon_core_get_task_region(insert_task).height);
	TCON_LOG("to collision list lut_dependency:0x%016llx",
		insert_task->lut_dependency);
	/* add insert_task to collsion task list*/
	hwtcon_core_change_task_state(insert_task, TASK_STATE_COLLISION,
		false, false, INSERT_TO_TAIL);

	spin_unlock_irqrestore(
		&hwtcon_fb_info()->collision_task_list.lock, flags);

}

void hwtcon_core_insert_normal_task_to_mdp_done_task_list(
	struct hwtcon_task *insert_task)
{
	unsigned long flags;
	struct hwtcon_task *task, *tmp = NULL;

	spin_lock_irqsave(&hwtcon_fb_info()->mdp_done_task_list.lock, flags);

	/* merge */
	list_for_each_entry_safe_reverse(task, tmp,
		&hwtcon_fb_info()->mdp_done_task_list.list, list) {
		struct rect task_region = hwtcon_core_get_task_region(task);
		struct rect insert_task_region =
			hwtcon_core_get_task_region(insert_task);
		struct rect merge_region = {0};

		/* merge condition:
		 * 1. same update mode
		 * 2. same regal setting (regal status & regal mode)
		 * 3. same waveform mode
		 */
		if (hwtcon_core_can_merge_trigger_task_region(
			&task_region, &insert_task_region, &merge_region) &&
			(task->update_data.update_mode ==
				insert_task->update_data.update_mode) &&
			(task->regal_status == insert_task->regal_status) &&
			(task->update_data.waveform_mode ==
				insert_task->update_data.waveform_mode) &&
			(task->regal_mode == insert_task->regal_mode)) {

			TCON_LOG("user insert scenario merge");
			TCON_LOG("task in mdp_done_task list");
			TCON_LOG("merge region [%d %d %d %d] ||",
				insert_task_region.x,
				insert_task_region.y,
				insert_task_region.width,
				insert_task_region.height);
			TCON_LOG("[%d %d %d %d] = [%d %d %d %d]",
				task_region.x,
				task_region.y,
				task_region.width,
				task_region.height,
				merge_region.x,
				merge_region.y,
				merge_region.width,
				merge_region.height);

			TCON_LOG("dump merge marker begin");
			do {
				struct update_marker_struct *update_marker,
					*tmp_marker;

				list_for_each_entry_safe(
					update_marker, tmp_marker,
					&task->marker_info_list.list,
						task_list) {
					TCON_LOG("merge marker %d",
						update_marker->update_marker);
				}

				list_for_each_entry_safe(
					update_marker, tmp_marker,
					&insert_task->marker_info_list.list,
					task_list) {
					TCON_LOG("merge marker %d",
						update_marker->update_marker);
				}
			} while (0);
			TCON_LOG("dump merge marker end");

			/* merge task & collision_task to task */
			hwtcon_core_set_task_region(insert_task, merge_region);

			insert_task->update_order =
				MIN(insert_task->update_order,
				task->update_order);

			insert_task->lut_dependency = 0LL;

			/* marker info: move task marker info to insert_task */
			list_splice_init(&task->marker_info_list.list,
				&insert_task->marker_info_list.list);

			list_del_init(&task->list);
			hwtcon_core_put_task_callback(task);
		} else
			break;
	}
	/* insert trigger task to mdp_done_task_list */
	hwtcon_core_change_task_state(insert_task, TASK_STAT_MDP_DONE,
			false, false, INSERT_TO_TAIL);
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->mdp_done_task_list.lock, flags);
}


void hwtcon_core_insert_collision_task_to_mdp_done_task_list(
	struct hwtcon_task *insert_task)
{
	unsigned long flags;
	struct hwtcon_task *task, *tmp = NULL;
	bool inserted = false;

	spin_lock_irqsave(&hwtcon_fb_info()->mdp_done_task_list.lock, flags);

	/* merge */
	list_for_each_entry_safe(task, tmp,
		&hwtcon_fb_info()->mdp_done_task_list.list, list) {
		struct rect task_region = hwtcon_core_get_task_region(task);
		struct rect insert_task_region =
			hwtcon_core_get_task_region(insert_task);
		struct rect merge_region = {0};

		if (hwtcon_core_can_merge_trigger_task_region(
			&task_region, &insert_task_region, &merge_region) &&
			(task->update_data.update_mode ==
				insert_task->update_data.update_mode) &&
			(task->regal_status == insert_task->regal_status) &&
			(task->regal_mode == insert_task->regal_mode)) {

			TCON_LOG("collsion scenario merge");

			TCON_LOG("task in mdp_done_task list");

			TCON_LOG("merge region [%d %d %d %d]",
				insert_task_region.x,
				insert_task_region.y,
				insert_task_region.width,
				insert_task_region.height);

			TCON_LOG("|| [%d %d %d %d] = [%d %d %d %d]",
				task_region.x,
				task_region.y,
				task_region.width,
				task_region.height,
				merge_region.x,
				merge_region.y,
				merge_region.width,
				merge_region.height);


			TCON_LOG("dump merge marker begin");
			do {
				struct update_marker_struct *update_marker,
					*tmp_marker;

				list_for_each_entry_safe(
					update_marker, tmp_marker,
					&task->marker_info_list.list,
					task_list) {
					TCON_LOG("merge marker %d",
						update_marker->update_marker);
				}

				list_for_each_entry_safe(
					update_marker, tmp_marker,
					&insert_task->marker_info_list.list,
					task_list) {
					TCON_LOG("merge marker %d",
						update_marker->update_marker);
				}
			} while (0);
			TCON_LOG("dump merge marker end");

			/* merge task & collision_task to task */
			hwtcon_core_set_task_region(insert_task, merge_region);

			insert_task->update_order =
				MIN(insert_task->update_order,
				task->update_order);

			if (insert_task->update_data.waveform_mode !=
				task->update_data.waveform_mode) {
				if(insert_task->night_mode) {
					if(UPDATE_MODE_PARTIAL==insert_task->update_data.update_mode) {
						insert_task->update_data.waveform_mode =
							WAVEFORM_MODE_GCK16;
					}
					else {
						insert_task->update_data.waveform_mode =
							WAVEFORM_MODE_GLKW16;
					}
				}
				else {
					if(UPDATE_MODE_PARTIAL==insert_task->update_data.update_mode) {
						insert_task->update_data.waveform_mode =
							WAVEFORM_MODE_GL16;
					}
					else {
						insert_task->update_data.waveform_mode =
							WAVEFORM_MODE_GC16;
					}
				}
			}
			insert_task->lut_dependency = 0LL;

			/* marker info: move task marker info to insert_task */
			list_splice_init(&task->marker_info_list.list,
				&insert_task->marker_info_list.list);

			list_del_init(&task->list);
			hwtcon_core_put_task_callback(task);
		} else if (insert_task->update_order > task->update_order) {
			/* can't merge &
			 * insert_task update_order > current task update order
			 * need to insert insert_task behind current task
			 */
			continue;
		} else {
			/* insert insert_task before current task */
			list_add_tail(&insert_task->list, &task->list);
			inserted = true;
			break;
		}
	}

	if (inserted == false) {
		/* mdp_done_task_list is empty.
		 * or all task in mdp_done_task_list's update_order
		 * is smaller than insert_task.
		 * add insert_task to the tail of mdp_done_task_list.
		 */
		hwtcon_core_change_task_state(insert_task,
			TASK_STAT_MDP_DONE, false, false, INSERT_TO_TAIL);
	}
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->mdp_done_task_list.lock, flags);
}

void hwtcon_core_create_collision_task(struct hwtcon_task *task)
{
	struct hwtcon_task *collision_task = NULL;

	/* no collision */
	if (task->pipeline_info.collision_lut_0 == 0 &&
		task->pipeline_info.collision_lut_1 == 0)
		return;
	/* have collision
	 * create a new collision task
	 * 1. force new task waveform mode to partial
	 * 2. new task region is collision region
	 * 3. copy new task marker from task, delete current task marker info
	 * 4. new task->lut = -1
	 * 5. new task->lut_dependency
	 */
	collision_task = hwtcon_core_get_task(&task->update_data, false);
	if (collision_task == NULL) {
		TCON_ERR("create new task fail");
		dump_stack();
		return;
	}

	collision_task->update_order = task->update_order;

	collision_task->update_data.update_mode = UPDATE_MODE_PARTIAL;
	//collision_task->update_data.update_mode = UPDATE_MODE_FULL;
	collision_task->update_data.flags = 0;

	hwtcon_core_set_task_region(collision_task,
			task->pipeline_info.collision_region);
	
	if(collision_task->night_mode) {
		if(UPDATE_MODE_PARTIAL==collision_task->update_data.update_mode) {
			collision_task->update_data.waveform_mode = WAVEFORM_MODE_GCK16;
		}
		else {
			collision_task->update_data.waveform_mode = WAVEFORM_MODE_GLKW16;
		}
	}
	else {
		if(UPDATE_MODE_PARTIAL==collision_task->update_data.update_mode) {
			collision_task->update_data.waveform_mode = WAVEFORM_MODE_GL16;
		}
		else {
			collision_task->update_data.waveform_mode = WAVEFORM_MODE_GC16;
		}
	}

	/* move task's marker to collision task */
	list_splice_init(&task->marker_info_list.list,
			&collision_task->marker_info_list.list);

	collision_task->assign_lut = -1;
	collision_task->lut_dependency =
		(u64)task->pipeline_info.collision_lut_1 << 32 |
		(u64)task->pipeline_info.collision_lut_0;

	TCON_LOG("create collision task[new] region[%d %d %d %d]",
		collision_task->update_data.update_region.left,
		collision_task->update_data.update_region.top,
		collision_task->update_data.update_region.width,
		collision_task->update_data.update_region.height);
	TCON_LOG("lut_dependency:0x%016llx wf_mode:%s upd_mode=%d nm=%d",
		collision_task->lut_dependency,
		hwtcon_core_get_wf_mode_name(
			collision_task->update_data.waveform_mode),
			collision_task->update_data.update_mode,
			collision_task->night_mode);
	hwtcon_core_insert_task_to_collision_task_list(collision_task);
	return;
}


//#define PENCOLOR_DEBUG		1
static int hwtcon_a2_modify_wb(struct rect *region,int isWhite)
{
	int i, j;
	unsigned int  pitch = hw_tcon_get_edp_width();
	unsigned short *pixel_wb = NULL ;
	unsigned short *wb_base = (unsigned short *)hwtcon_fb_info()->wb_buffer_va;
	unsigned char *pixel_img = NULL ;
	unsigned char *img_base = (unsigned char *)hwtcon_fb_info()->img_buffer_va;
#ifdef PENCOLOR_DEBUG //[
	unsigned long pixel_last_val=0xffffffff;
#endif //] PENCOLOR_DEBUG

	for (i = region->x; i < region->x + region->width; i++)
		for (j = region->y; j < region->y + region->height; j++) {
			pixel_wb = wb_base+pitch * j + i;

			pixel_img = img_base + (j * i);

#ifdef PENCOLOR_DEBUG //[
			if( pixel_last_val != (unsigned long)(*pixel_img) ) {
				TCON_LOG("(%d,%d) wb=0x%x,img=0x%x",i,j,*pixel_wb,*pixel_img);
			}
			pixel_last_val = (unsigned long)(*pixel_img);
#endif //] PENCOLOR_DEBUG

			if(1==isWhite) {
				// white pen . 
				if ( ((*pixel_wb & 0x07c0) != 0x7c0) )
					*(pixel_wb) = *pixel_wb & ~0x07c0;
			}
			else if(0==isWhite) {
				// black pen . 
				if ( ((*pixel_wb & 0x07c0) != 0) )
					*(pixel_wb) = *pixel_wb | 0x0780;
			}
			else {
				// auto detect by img buffer . 
				if(*pixel_img<0xf0) 
				{
					// white pen .
					if ( ((*pixel_wb & 0x07c0) != 0x7c0) )
						*(pixel_wb) = *pixel_wb & ~0x07c0;
				}
				else {
					// black pen . 
					if ( ((*pixel_wb & 0x07c0) != 0) )
						*(pixel_wb) = *pixel_wb | 0x0780;
				}
			}
			
	}
	return 0;
}


void hwtcon_core_handle_a2_handwrite_task(struct hwtcon_task *task)
{
	struct rect region = {0};

	region.x = hwtcon_core_get_task_region(task).x;
	region.y = hwtcon_core_get_task_region(task).y;
	region.width = hwtcon_core_get_task_region(task).width;
	region.height = hwtcon_core_get_task_region(task).height;
	TCON_LOG("force a2 handle");
	if (task->update_data.flags & HWTCON_FLAG_FORCE_A2_OUTPUT) {

		if (task->update_data.flags & HWTCON_FLAG_FORCE_A2_OUTPUT_WHITE) {
			hwtcon_a2_modify_wb(&region,1);
		}
		else if(task->update_data.flags & HWTCON_FLAG_FORCE_A2_OUTPUT_BLACK) {
			hwtcon_a2_modify_wb(&region,0);
		}
		else {
			hwtcon_a2_modify_wb(&region,-1);
		}
	}
	return;
}

int hwtcon_core_wait_all_wf_lut_release(void)
{
	int status = 0;

	/* wait all wf_lut release */
	status = wait_event_timeout(
			hwtcon_fb_info()->wf_lut_release_wait_queue,
			(hwtcon_fb_info()->lut_active == 0LL),
			msecs_to_jiffies(HWTCON_WAIT_WF_LUT_RELEASE_TIMEOUT));
	/* wait timeout */
	if (status == 0) {
		TCON_ERR("wait timeout, lut status: 0x%016llx",
			hwtcon_fb_info()->lut_free);
		TCON_ERR("0x%016llx 0x%016llx",
			hwtcon_core_get_released_lut(),
			hwtcon_fb_info()->lut_active);
		return -1;
	}
	return 0;
}

int hwtcon_core_wait_power_down(void)
{
	int status = 0;
	int timeout_ms = HWTCON_TASK_WAIT_MARKER_TIMEOUT_MS +
		hwtcon_fb_info()->power_down_delay_ms;

	if(hwtcon_fb_info()->power_down_delay_ms==EINK_NO_POWER_DOWN) {
		return 0;
	}

	status = wait_event_timeout(
			hwtcon_fb_info()->power_state_change_wait_queue,
			(hwtcon_fb_info()->mmsys_power_enable == false),
			msecs_to_jiffies(timeout_ms));
	if (status == 0) {
		TCON_ERR("wait power down timeout:%d timer:%d",
			hwtcon_fb_info()->mmsys_power_enable,
			timeout_ms);
		hwtcon_driver_enable_mmsys_power(false);
		return -1;
	}

	return 0;
}


bool hwtcon_core_use_regal(struct hwtcon_update_data *update_data,
	u32 *regal_mode)
{
	switch (update_data->waveform_mode) {
	case WAVEFORM_MODE_REAGL:
		*regal_mode = REGAL_MODE_REGAL;
		return true;
	case WAVEFORM_MODE_GLKW16:
		*regal_mode = REGAL_MODE_DRAK;
		return true;
	default:
		return false;
	}
	return false;
}

void hwtcon_core_change_waveform_slot(struct hwtcon_task *task)
{
	int temp_zone = 0;
	int night_mode = 0;

	temp_zone = hwtcon_core_read_temp_zone();
	night_mode = task->night_mode;

	if ((hwtcon_fb_info()->current_temp_zone != temp_zone)||
		(hwtcon_fb_info()->current_night_mode != night_mode)) {
		TCON_LOG("change waveform slot temp_zone:%d night_mode:%d",
			temp_zone, night_mode);
		/* wait all wf_lut release */
		if (hwtcon_core_wait_all_wf_lut_release() != 0)
			return;
		wf_lut_waveform_slot_association(NULL, night_mode, temp_zone);
		if (hwtcon_fb_info()->current_night_mode != night_mode) {
			/* PMIC night mode enable */
			fiti_wait_power_good();
			//fiti_set_night_mode(night_mode);
		}

		hwtcon_fb_info()->current_temp_zone = temp_zone;
		hwtcon_fb_info()->current_night_mode = night_mode;

	}
}

int hwtcon_core_trigger_pipeline(struct hwtcon_task *task)
{
	struct pipeline_info info = {0};
	int status = 0;
	unsigned long flags;
	struct update_marker_struct *update_marker, *tmp;

	hwtcon_driver_enable_mmsys_power(true);

	/* update waveform slot */
	hwtcon_core_change_waveform_slot(task);

	task->time_trigger_pipeline = timeofday_ms();
	TCON_LOG("[%d] waveform=0x%x (%s) mode=0x%x update region top=%d,",
		task->update_data.update_marker,
		task->update_data.waveform_mode,
		hwtcon_core_get_wf_mode_name(task->update_data.waveform_mode),
		task->update_data.update_mode,
		hwtcon_core_get_task_user_region(task).y);
	TCON_LOG("left=%d, width=%d, height=%d flags=0x%x rotation=%d",
			hwtcon_core_get_task_user_region(task).x,
			hwtcon_core_get_task_user_region(task).width,
			hwtcon_core_get_task_user_region(task).height,
			task->update_data.flags, hwtcon_fb_get_rotation());

	TCON_LOG("TRIGGER:task:0x%llx marker:%d time:%lld region[%d %d %d %d]",
		task->unique_id,
		task->update_data.update_marker,
		task->time_trigger_pipeline,
		hwtcon_core_get_task_region(task).x,
		hwtcon_core_get_task_region(task).y,
		hwtcon_core_get_task_region(task).width,
		hwtcon_core_get_task_region(task).height);
	/* trigger lut to pipeline. */
	TCON_LOG("%s update:%d->%s wf_mode:%d-%s temperature:%d-%d",
		task->night_mode ? "Night Mode" : "Day Mode",
		task->update_data.update_mode,
		(task->update_data.update_mode == UPDATE_MODE_FULL) ?
			"FULL" : "PARTIAL",
		task->update_data.waveform_mode,
		hwtcon_core_get_wf_mode_name(task->update_data.waveform_mode),
		hwtcon_core_read_temperature(),
		hwtcon_core_read_temp_zone());


	// should check pmic power ready ...
	
	if(!fiti_pmic_judge_power_on()) {
		TCON_LOG("waiting for pmic on ...");
		fiti_wait_power_good();
	}


	status = pipeline_handle_normal_update(task, &info);

	task->time_pipeline_done = timeofday_ms();

	if (status != 0) {
		TCON_ERR("trigger pipeline fail[%d]", status);
		spin_lock_irqsave(
			&hwtcon_fb_info()->pipeline_processing_task_list.lock,
			flags);
		list_del_init(&task->list);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->pipeline_processing_task_list.lock,
			flags);

		#ifdef MARKER_V2_ENABLE
		list_for_each_entry_safe(update_marker, tmp,
			&task->marker_info_list.list, task_list) {
			complete(&update_marker->submit_completion);
			TCON_LOG("[MARKER] submit_completion: %d",
				update_marker->update_marker);
			spin_lock_irqsave(&update_marker->marker_lock, flags);
			list_del_init(&update_marker->task_list);
			update_marker->lut_mask |= 0LL;
			update_marker->marker_state = MARKER_STATE_LUT_ASSIGNED;
			spin_unlock_irqrestore(
				&update_marker->marker_lock, flags);
		}
		#endif

		/* close mmsys & fiti power */
		hwtcon_core_handle_clock_disable();
		hwtcon_core_put_task_callback(task);
		return status;
	}

	memcpy(&task->pipeline_info, &info, sizeof(info));

	if (info.update_void) {
		/* update_void == true: pipeline doesn't modify working buffer*/
		if (info.collision_lut_0 == 0 &&
			info.collision_lut_1 == 0) {
			/* no collision
			 * move task to free task list
			 */
			HWTCON_TIME end_time = timeofday_ms();

			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				TCON_LOG("[%d] Sending update.",
					update_marker->update_marker);
				TCON_LOG("VOID update region top=%d,left=%d",
					hwtcon_core_get_task_user_region(
						task).y,
					hwtcon_core_get_task_user_region(
						task).x);
				TCON_LOG("width=%d, height=%d",
					hwtcon_core_get_task_user_region(
						task).width,
					hwtcon_core_get_task_user_region(
						task).height);
				TCON_LOG("temp index: %d rotation=%d",
					hwtcon_core_read_temp_zone(),
					hwtcon_fb_get_rotation());
				TCON_LOG("task:0x%llx marker:%d update",
					task->unique_id,
					update_marker->update_marker);
				TCON_LOG("void with no collision");
				TCON_LOG("[%d] update end marker=%d,",
					update_marker->update_marker,
					update_marker->update_marker);
				TCON_LOG("end time=%lld, time taken=%d ms",
					end_time,
					hwtcon_hal_get_time_in_ms(
						update_marker->start_time,
						end_time));
			}
			spin_lock_irqsave(
			&hwtcon_fb_info()->pipeline_processing_task_list.lock,
			flags);
			list_del_init(&task->list);
			spin_unlock_irqrestore(
			&hwtcon_fb_info()->pipeline_processing_task_list.lock,
			flags);

			#ifdef MARKER_V2_ENABLE
			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				complete(&update_marker->submit_completion);
				TCON_LOG("[MARKER] submit_completion: %d",
				update_marker->update_marker);
				spin_lock_irqsave(
					&update_marker->marker_lock, flags);
				list_del_init(&update_marker->task_list);
				update_marker->lut_mask |= 0LL;
				update_marker->marker_state =
					MARKER_STATE_LUT_ASSIGNED;
				spin_unlock_irqrestore(
					&update_marker->marker_lock, flags);
			}
			#endif

			/* close mmsys & fiti power */
			hwtcon_core_handle_clock_disable();
			hwtcon_core_put_task_callback(task);
		} else {
			/* have collision
			 * move task to collision task list
			 * 1. force task waveform mode to partial
			 * 2. task region is collision region
			 * 3. task->lut = -1
			 * 4. task->lut_dependency
			 * 5. move to collision task list
			 */
			task->update_data.update_mode = UPDATE_MODE_PARTIAL;

			if(task->night_mode) {
				if(UPDATE_MODE_PARTIAL==task->update_data.update_mode) {
					task->update_data.waveform_mode = WAVEFORM_MODE_GCK16;
				}
				else {
					task->update_data.waveform_mode = WAVEFORM_MODE_GLKW16;
				}
			}
			else {
				if(UPDATE_MODE_PARTIAL==task->update_data.update_mode) {
					task->update_data.waveform_mode = WAVEFORM_MODE_GL16;
				}
				else {
					task->update_data.waveform_mode = WAVEFORM_MODE_GC16;
				}
			}
			task->update_data.flags = 0;
			hwtcon_core_set_task_region(
				task, info.collision_region);
			task->assign_lut = -1;
			task->lut_dependency = (u64)info.collision_lut_1 <<
				32 | (u64)info.collision_lut_0;

			#ifdef MARKER_V2_ENABLE
			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				spin_lock_irqsave(
					&update_marker->marker_lock, flags);
				update_marker->lut_mask |= 0LL;
				update_marker->marker_state =
					MARKER_STATE_COLLISION;
				spin_unlock_irqrestore(
					&update_marker->marker_lock, flags);
			}
			#endif

			TCON_LOG("create collision task[replace]");
			TCON_LOG("region[%d %d %d %d]",
				task->update_data.update_region.left,
				task->update_data.update_region.top,
				task->update_data.update_region.width,
				task->update_data.update_region.height);
			TCON_LOG("lut_dependency:0x%016llx wf_mode:%s",
				task->lut_dependency,
				hwtcon_core_get_wf_mode_name(
					task->update_data.waveform_mode));

			spin_lock_irqsave(
			&hwtcon_fb_info()->
				pipeline_processing_task_list.lock, flags);
			list_del_init(&task->list);
			spin_unlock_irqrestore(
			&hwtcon_fb_info()->pipeline_processing_task_list.lock,
			flags);
			hwtcon_core_insert_task_to_collision_task_list(task);

		}

	} else {
		/* update_void == false pipeline modify working buffer */
		WARN_ON(task->assign_lut > MAX_LUT_REGION_COUNT);
		if (info.collision_lut_0 == 0 &&
			info.collision_lut_1 == 0) {
			/* no collision
			 * signal update_marker
			 */
			#ifdef MARKER_V2_ENABLE
			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				complete(&update_marker->submit_completion);
				TCON_LOG("[MARKER] submit_completion: %d",
					update_marker->update_marker);
				spin_lock_irqsave(&update_marker->marker_lock,
					flags);
				list_del_init(&update_marker->task_list);
				update_marker->lut_mask |=
					(1LL << task->assign_lut);
				update_marker->marker_state =
					MARKER_STATE_LUT_ASSIGNED;
				spin_unlock_irqrestore(
					&update_marker->marker_lock, flags);
			}
			#else
			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				complete(&update_marker->submit_completion);
				TCON_LOG("[MARKER] submit_completion: %d",
					update_marker->update_marker);
			}
			#endif
		} else {
			/* have collision
			 * create a new collision task
			 */
			#ifdef MARKER_V2_ENABLE
			list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
				spin_lock_irqsave(
					&update_marker->marker_lock, flags);
				update_marker->lut_mask |=
					(1LL << task->assign_lut);
				update_marker->marker_state =
					MARKER_STATE_COLLISION;
				spin_unlock_irqrestore(
					&update_marker->marker_lock, flags);
			}
			#endif
			hwtcon_core_create_collision_task(task);
		}


		if(WAVEFORM_MODE_A2==task->update_data.waveform_mode) {
#if 1 //[
			if (task->update_data.flags & HWTCON_FLAG_FORCE_A2_OUTPUT)
				hwtcon_core_handle_a2_handwrite_task(task);
#else //][!
			if (task->update_data.flags & HWTCON_FLAG_FORCE_A2_OUTPUT)
				task->update_data.waveform_mode = WAVEFORM_MODE_DU;
#endif //]
		}


		/* pipeline update working buffer
		 * move task to pipeline_done_task_list
		 * trigger WF_LUT to show update.
		 */
		hwtcon_core_change_task_state(task, TASK_STATE_PIPELINE_DONE,
				true, true, INSERT_TO_TAIL);

		/* auto waveform handle */
		hwtcon_core_handle_auto_waveform(task);

		list_for_each_entry_safe(update_marker, tmp,
				&task->marker_info_list.list, task_list) {
			TCON_LOG("[%d] Sending update. waveform:%d (%s)",
				update_marker->update_marker,
				task->update_data.waveform_mode,
			hwtcon_core_get_wf_mode_name(
					task->update_data.waveform_mode));
			TCON_LOG("mode:%d update region top=%d,left=%d,width",
				task->update_data.update_mode,
				hwtcon_core_get_task_user_region(task).y,
				hwtcon_core_get_task_user_region(task).x);
			TCON_LOG("=%d, height=%d temp index: %d rotation=%d",
				hwtcon_core_get_task_user_region(task).width,
				hwtcon_core_get_task_user_region(task).height,
				hwtcon_core_read_temp_zone(),
				hwtcon_fb_get_rotation());

			TCON_LOG("[%d] Sending update in LUT: %d",
					update_marker->update_marker,
					task->assign_lut);
		}

		TCON_LOG("trigger WF_LUT with lut:%d region[%d %d %d %d]",
			task->assign_lut,
			hwtcon_core_get_task_region(task).x,
			hwtcon_core_get_task_region(task).y,
			hwtcon_core_get_task_region(task).width,
			hwtcon_core_get_task_region(task).height);
		TCON_LOG("wf_mode:%d lut status[0x%016llx 0x%016llx 0x%016llx]",
			task->update_data.waveform_mode,
			hwtcon_fb_info()->lut_free,
			hwtcon_core_get_released_lut(),
			hwtcon_fb_info()->lut_active);

		if (hwtcon_debug_get_info()->enable_dump_next_buffer>0 ||
			hwtcon_debug_get_info()->collision_debug) {
			/* hwtcon_core_handle_task_written_done */
			queue_work(hwtcon_fb_info()->wq_pipeline_written_done,
				&task->work_written_done);
			if (hwtcon_debug_get_info()->enable_dump_next_buffer)
				msleep(500);
		}

		if (!hwtcon_debug_get_info()->collision_debug) {
			enum WF_SLOT_ENUM slot = wf_lut_get_waveform_mode_slot(
				task->update_data.waveform_mode,
				task->night_mode);


			task->time_wait_fiti_power_good = timeofday_ms();
			/* wait fiti power good */
			fiti_wait_power_good();
			task->time_trigger_wf_lut = timeofday_ms();
			/* trigger WF_LUT */
			TS_WF_LUT_set_lut_info(NULL,
					hwtcon_core_get_task_region(task).x,
					hwtcon_core_get_task_region(task).y,
					hwtcon_core_get_task_region(task).width,
					hwtcon_core_get_task_region(
						task).height,
					task->assign_lut,
					slot);
			lut_frame_count[task->assign_lut] = 0;
			lut_crc_all_zero[task->assign_lut] = true;
		}

	}

	return 0;
}


bool hwtcon_core_check_task_block(struct hwtcon_task *task,
		struct hwtcon_task *search_task)
{

	if (task->update_data.flags != search_task->update_data.flags) {
		struct rect task_region = hwtcon_core_get_task_region(task);
		struct rect sreach_task_region =
			hwtcon_core_get_task_region(search_task);

		if (hwtcon_rect_check_relationship(
			&task_region, &sreach_task_region, NULL) ==
			RECT_RELATION_CONTAIN)
			return true;
	}
	return false;
}

static struct hwtcon_task *gptHWTCON_current_mdp_task;
struct hwtcon_task *hwtcon_core_get_current_mdp_task(void)
{
	return gptHWTCON_current_mdp_task;
}

int hwtcon_core_use_cfa_color_mapping(struct hwtcon_task *task)
{
	if (hw_tcon_get_epd_type() == 0)
		return 0;
	if (hwtcon_fb_get_cur_color_format() == V4L2_PIX_FMT_Y8)
		return 0;
	else if (task->update_data.flags&HWTCON_FLAG_CFA_SKIP)
		return 0;
	else if (HWTCON_FLAG_GET_CFA_MODE(task->update_data.flags))
		return 1;
	else if (hwtcon_fb_info()->cfa_mode<=0) {
		return 0;
	}
	return 1;
}

int hwtcon_core_dispatch_mdp(void *ignore)
{
	struct hwtcon_task *task = NULL;
	unsigned long flags;
	int ret = 0;

	while (1) {

		gptHWTCON_current_mdp_task = 0;
		wait_event(hwtcon_fb_info()->mdp_trigger_wait_queue,
			!list_empty(
			&hwtcon_fb_info()->wait_for_mdp_task_list.list));
		spin_lock_irqsave(
			&hwtcon_fb_info()->wait_for_mdp_task_list.lock, flags);
		task = list_first_entry_or_null(
			&hwtcon_fb_info()->wait_for_mdp_task_list.list,
			struct hwtcon_task, list);
		gptHWTCON_current_mdp_task = task;
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->wait_for_mdp_task_list.lock,
			flags);

		if (task == NULL)
			continue;

		TCON_LOG("wait for trigger Task");
		TCON_LOG("[MARKER]:%d flags:0x%08x to MDP",
			task->update_data.update_marker,
			task->update_data.flags);

		task->time_trigger_mdp = timeofday_ms();
		task->mdp_src_format = hwtcon_fb_info()->mdp_src_format;

		if (hwtcon_debug_get_info()->enable_dump_next_buffer>0) {
			if(hwtcon_fb_info()->debug_img_buffer_counter>0) {
				snprintf(hwtcon_fb_info()->debug_img_buffer_name[0],
					MAX_FILE_NAME_LEN,
				"%s/next_fb_%d_m%04d_%dx%dy%dw%dh.bin",
					hwtcon_fb_info()->debug_img_dump_path,
					hwtcon_fb_info()->debug_img_buffer_counter,
					task->update_data.update_marker,
					hwtcon_core_get_task_region(task).x,
					hwtcon_core_get_task_region(task).y,
					hwtcon_core_get_task_region(task).width,
					hwtcon_core_get_task_region(task).height);
				hwtcon_file_save_buffer(hwtcon_fb_info()->fb_buffer_va,
					hwtcon_fb_info()->fb_buffer_size,
					hwtcon_fb_info()->debug_img_buffer_name[0]);
			}
			else {
				hwtcon_file_save_buffer(hwtcon_fb_info()->fb_buffer_va,
					hwtcon_fb_info()->fb_buffer_size,
					"/tmp/next_fb.bin");
			}
		}
		/* call MDP */
		mutex_lock(&hwtcon_fb_info()->image_buffer_access_mutex);

		if (hwtcon_core_use_cfa_color_mapping(task)) {
			HWTCON_TIME T1, T2;

			cfa_color_region = hwtcon_core_get_mdp_region(task);
			T1 = timeofday_ms();
			up(&color_sem2);
			ret = down_timeout(&color_sem1, msecs_to_jiffies(HWTCON_TASK_TIMEOUT_MS));
			T2 = timeofday_ms();
			if (ret)
				TCON_ERR("trigger cfa color rendering fail");
			else
				TCON_LOG("trigger cfa color rendering begin, cfa_mode=%d, task cfa_mode=%d",
					hwtcon_fb_info()->cfa_mode,HWTCON_FLAG_GET_CFA_MODE(task->update_data.flags));
			hwtcon_debug_record_printf("cfa color mapping (%d),%d cores,%d threads, takes:%d ms\n",
				hwtcon_fb_info()->cfa_mode,num_online_cpus(),hwtcon_fb_info()->cfa_convert_threads,
				hwtcon_hal_get_time_in_ms(T1, T2));
		}

		#if 1
		hwtcon_mdp_convert(task);
		#else
		do {
			struct rect src_region = {0, 0, hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height()};
			struct rect dst_region = {0, 0, hw_tcon_get_edp_width(),
				hw_tcon_get_edp_height()};

			hwtcon_mdp_copy_buffer_with_region(
				hwtcon_fb_info()->img_buffer_va,
				hw_tcon_get_edp_width(),
				&dst_region,
				hwtcon_fb_info()->fb_buffer_va,
				hw_tcon_get_edp_width(),
				&src_region);
		} while (0);
		#endif
		if (hwtcon_debug_get_info()->enable_dump_next_buffer>0) {
			if(hwtcon_fb_info()->debug_img_buffer_counter>0) {
				snprintf(hwtcon_fb_info()->debug_img_buffer_name[0],
					MAX_FILE_NAME_LEN,
				"%s/next_%d_m%04d_%dx%dy%dw%dh.bin",
					hwtcon_fb_info()->debug_img_dump_path,
					hwtcon_fb_info()->debug_img_buffer_counter,
					task->update_data.update_marker,
					hwtcon_core_get_task_region(task).x,
					hwtcon_core_get_task_region(task).y,
					hwtcon_core_get_task_region(task).width,
					hwtcon_core_get_task_region(task).height);
				hwtcon_file_save_buffer(hwtcon_fb_info()->img_buffer_va,
					hwtcon_fb_info()->img_buffer_size,
					hwtcon_fb_info()->debug_img_buffer_name[0]);

				TCON_LOG("write imgbuf -> \"%s\"",hwtcon_fb_info()->debug_img_buffer_name[0]);

				hwtcon_fb_info()->debug_img_buffer_counter--;
			}
			else {
				hwtcon_file_save_buffer(hwtcon_fb_info()->img_buffer_va,
					hwtcon_fb_info()->img_buffer_size,
				"/tmp/next_img.bin");
			}
		}

		if (hwtcon_debug_get_info()->enable_dump_image_buffer) {
			int index =
				hwtcon_fb_info()->debug_img_buffer_counter++;
			HWTCON_TIME start = timeofday_ms();
			HWTCON_TIME end = 0;

			hwtcon_fb_info()->debug_img_buffer_counter %=
				MAX_DEBUG_IMAGE_BUFFER_COUNT;

			snprintf(hwtcon_fb_info()->debug_img_buffer_name[index],
				MAX_FILE_NAME_LEN,
			"/mnt/us/documents/dump/img_%04d_%03d_%03d_%03d_%03d.bin",
				task->update_data.update_marker,
				hwtcon_core_get_task_region(task).x,
				hwtcon_core_get_task_region(task).y,
				hwtcon_core_get_task_region(task).width,
				hwtcon_core_get_task_region(task).height);
			#if 0
			memcpy(hwtcon_fb_info()->debug_img_buffer_va[index],
				hwtcon_fb_info()->img_buffer_va,
				hwtcon_fb_info()->img_buffer_size);
			#else
			hwtcon_mdp_memcpy(
				hwtcon_fb_info()->debug_img_buffer_pa[index],
				hwtcon_fb_info()->img_buffer_pa);
			#endif

			end = timeofday_ms();
			TCON_ERR("dump buffer time:%d ms",
				hwtcon_hal_get_time_in_ms(start, end));
		}

		mutex_unlock(&hwtcon_fb_info()->image_buffer_access_mutex);

		task->time_mdp_done = timeofday_ms();
		spin_lock_irqsave(
			&hwtcon_fb_info()->wait_for_mdp_task_list.lock, flags);
		list_del_init(&task->list);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->wait_for_mdp_task_list.lock,
			flags);
		hwtcon_core_insert_normal_task_to_mdp_done_task_list(task);
		if (hwtcon_debug_get_info()->mdp_merge_debug) {
			/* start a timer for delay wakeup
			 * dispatch_pipeline thread
			 * hwtcon_core_handle_mdp_debug_timer_cb
			 */
			TCON_ERR("delay %d ms for simulate",
				hwtcon_debug_get_info()->mdp_merge_debug);
			TCON_ERR("task merge in mdp_task_done list");
			mod_timer(&task->mdp_debug_timer,
				jiffies + msecs_to_jiffies(
				    hwtcon_debug_get_info()->mdp_merge_debug));
		} else
			wake_up(&hwtcon_fb_info()->pipeline_trigger_wait_queue);

	}

	return 0;
}



static struct hwtcon_task *gptHWTCON_current_update_task;
struct hwtcon_task *hwtcon_core_get_current_update_task(void)
{
	return gptHWTCON_current_update_task;
}

int hwtcon_core_dispatch_pipeline(void *ignore)
{
	struct hwtcon_task *task = NULL;
	unsigned long flags;

	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	add_wait_queue(&hwtcon_fb_info()->pipeline_trigger_wait_queue, &wait);
	while (1) {
		/* find the first task */
		spin_lock_irqsave(
			&hwtcon_fb_info()->mdp_done_task_list.lock, flags);
		task = list_first_entry_or_null(
			&hwtcon_fb_info()->mdp_done_task_list.list,
			struct hwtcon_task, list);
		gptHWTCON_current_update_task = task;
		if (task == NULL) {
			spin_unlock_irqrestore(
				&hwtcon_fb_info()->mdp_done_task_list.lock,
				flags);
			wait_woken(&wait,
				TASK_INTERRUPTIBLE,
				MAX_SCHEDULE_TIMEOUT);
			continue;
		}

		hwtcon_core_change_task_state(task,
			TASK_STATE_PIPELINE_PROCESS,
			false,
			true,
			INSERT_TO_TAIL);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->mdp_done_task_list.lock, flags);

		/* trigger task to pipeline. */
		mutex_lock(&hwtcon_fb_info()->image_buffer_access_mutex);
		hwtcon_core_trigger_pipeline(task);
		mutex_unlock(&hwtcon_fb_info()->image_buffer_access_mutex);

	}

	remove_wait_queue(
		&hwtcon_fb_info()->pipeline_trigger_wait_queue,
		&wait);
	return 0;
}

void hwtcon_core_config_timing(struct cmdqRecStruct *pkt)
{
	/* config smi setting */
	rdma_config_smi_setting(NULL);

	wf_lut_config_context_init_for_pipeline();

	/* confit wf_lut */
	//wf_lut_dpi_enable(pkt);

	//hwtcon_edp_pinmux_active();
}

void hwtcon_core_dump_task_info(struct hwtcon_task *task)
{
	if (task == NULL)
		return;
	TCON_LOG("dump task:0x%llx begin", task->unique_id);

	TCON_LOG("state:%d region[%d %d %d %d]",
		task->state,
		hwtcon_core_get_task_region(task).x,
		hwtcon_core_get_task_region(task).y,
		hwtcon_core_get_task_region(task).width,
		hwtcon_core_get_task_region(task).height);
	TCON_LOG("dump task:0x%llx end", task->unique_id);
}

void hwtcon_core_handle_release_lut(int lut_id)
{
	struct hwtcon_task *task = NULL;
	struct hwtcon_task *tmp;
	#ifndef MARKER_V2_ENABLE
	struct update_marker_struct *update_marker = NULL;
	struct update_marker_struct *tmp_marker;
	#endif

	unsigned long flags;
	bool release_task_found = false;

	if (lut_id < 0 || lut_id >= MAX_LUT_REGION_COUNT) {
		WARN(1, "invalid lut_id:%d", lut_id);
		return;
	}

	/* cancel task lut release timer */
	del_timer(&hwtcon_fb_info()->timer_lut_release[lut_id]);

	/* search the release task */
	spin_lock_irqsave(
		&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);
	list_for_each_entry_safe(task, tmp,
		&hwtcon_fb_info()->pipeline_done_task_list.list, list) {
		if (task->assign_lut == lut_id) {
			release_task_found = true;
			break;
		}
	}
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);

	if (release_task_found == false)
		return;

	if (lut_crc_all_zero[lut_id]) {
		TCON_ERR("debug drop frame: lut[%d] crc", lut_id);
		TCON_ERR("all zero frame_count[%d] waveform[%d->%s]",
			lut_frame_count[lut_id],
			task->update_data.waveform_mode,
			hwtcon_core_get_wf_mode_name(
				task->update_data.waveform_mode));
		}
	lut_crc_all_zero[lut_id] = true;
	lut_frame_count[lut_id] = 0;

	hwtcon_core_dump_task_info(task);

	/* release task */
	task->time_wf_lut_done = timeofday_ms();

	#ifndef MARKER_V2_ENABLE
	list_for_each_entry_safe(update_marker, tmp_marker,
		&task->marker_info_list.list, task_list) {
		TCON_LOG("[%d] update end marker=%d",
			update_marker->update_marker,
			update_marker->update_marker);
		TCON_LOG("end time=%lld, time taken=%d ms",
			task->time_wf_lut_done,
			hwtcon_hal_get_time_in_ms(
			task->time_trigger_pipeline,
			task->time_wf_lut_done));
	}
	#endif

	TCON_LOG("DONE:task:0x%llx marker[%d] time:%lld cost:%d ms",
		task->unique_id,
		task->update_data.update_marker,
		task->time_wf_lut_done,
		hwtcon_hal_get_time_in_ms(
			task->time_trigger_pipeline,
			task->time_wf_lut_done));
	spin_lock_irqsave(
		&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);
	list_del_init(&task->list);
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->pipeline_done_task_list.lock, flags);
	hwtcon_core_put_task_with_lut_release(task);
}

void hwtcon_core_update_collision_list_on_release_lut(u64 released_lut)
{
	unsigned long flags;
	struct hwtcon_task *task, *tmp;

	spin_lock_irqsave(&hwtcon_fb_info()->collision_task_list.lock, flags);
	list_for_each_entry_safe(task, tmp,
		&hwtcon_fb_info()->collision_task_list.list, list) {

		task->lut_dependency &= ~released_lut;
		if (task->lut_dependency == 0LL) {
			TCON_LOG("retrigger collision task:0x%llx",
				task->unique_id);
			TCON_LOG("[%d %d %d %d] wf:%s upd_mode=%d to mdp_done_list",
				hwtcon_core_get_task_region(task).x,
				hwtcon_core_get_task_region(task).y,
				hwtcon_core_get_task_region(task).width,
				hwtcon_core_get_task_region(task).height,
				hwtcon_core_get_wf_mode_name(task->update_data.waveform_mode),
				task->update_data.update_mode);
			list_del_init(&task->list);
			hwtcon_core_insert_collision_task_to_mdp_done_task_list(
									task);
		}
	}

	spin_unlock_irqrestore(&hwtcon_fb_info()->collision_task_list.lock,
		flags);
	wake_up(&hwtcon_fb_info()->pipeline_trigger_wait_queue);
}

int hwtcon_core_convert_bit_count_2_grey_level(u32 histogram)
{
	if ((histogram & ~HISTOGRAM_GREY_LEVEL_Y2) == 0)
		return 0;
	if ((histogram & ~HISTOGRAM_GREY_LEVEL_Y4) == 0)
		return 1;
	if ((histogram & ~HISTOGRAM_GREY_LEVEL_Y8) == 0)
		return 2;
	if ((histogram & ~HISTOGRAM_GREY_LEVEL_Y16) == 0)
		return 3;

	return 4;
}

char *hwtcon_core_get_wf_mode_name(enum WAVEFORM_MODE_ENUM mode)
{
	switch (mode) {
	case WAVEFORM_MODE_INIT:
		return "init";
	case WAVEFORM_MODE_DU:
		return "du";
	case WAVEFORM_MODE_GC16:
		return "gc16";
	case WAVEFORM_MODE_GL16:
		return "gl16";
	case WAVEFORM_MODE_GLR16:
		return "glr16 (reagl)";
	case WAVEFORM_MODE_A2:
		return "a2";
	case WAVEFORM_MODE_AUTO:
		return "auto";
	case WAVEFORM_MODE_GCK16:
		return "gck16";
	case WAVEFORM_MODE_GLKW16:
		return "glkw16";
	case WAVEFORM_MODE_GCC16:
		return "gcc16";
	default:
		return "unknown_mode";
	}

	return "unknown_mode";
}

struct update_marker_struct *hwtcon_core_alloc_update_marker(void)
{
	struct update_marker_struct *marker = NULL;

	marker = vzalloc(sizeof(struct update_marker_struct));
	if (marker == NULL) {
		TCON_ERR("vmalloc update marker fail");
		return NULL;
	}

	/* init marker member	*/
	INIT_LIST_HEAD(&marker->global_list);
	INIT_LIST_HEAD(&marker->task_list);
	marker->update_marker = -1;
	init_completion(&marker->update_completion);
	init_completion(&marker->submit_completion);
	spin_lock_init(&marker->marker_lock);
	marker->lut_mask = 0LL;
	marker->marker_state = MARKER_STATE_LUT_NOT_ASSIGN;
	marker->start_time = timeofday_ms();
	marker->has_waiters = false;
	marker->need_release = false;

	return marker;
}

void hwtcon_core_handle_auto_waveform(struct hwtcon_task *task)
{
	u32 next_grey = 0;
	u32 current_grey = 0;
	enum WAVEFORM_MODE_ENUM wf_mode;
	static const enum WAVEFORM_MODE_ENUM day_mode_wf_mode_table[5][5] = {
		{1, 2, 2, 2, 2},
		{1, 2, 2, 2, 2},
		{1, 2, 2, 2, 2},
		{1, 2, 2, 2, 2},
		{3, 3, 3, 3, 3},
	};

    static const enum WAVEFORM_MODE_ENUM night_mode_wf_mode_table[5][5] = {
        {1, 8, 8, 8, 8},
        {1, 8, 8, 8, 8},
        {1, 8, 8, 8, 8},
        {1, 8, 8, 8, 8},
        {9, 9, 9, 9, 9},
    };


	if (task->update_data.waveform_mode != WAVEFORM_MODE_AUTO) {
		TCON_LOG("[%d]chist=0x%x[%d] nhist=0x%x[%d]",
			task->update_data.update_marker,
			task->pipeline_info.current_histogram,
			current_grey,
			task->pipeline_info.next_histogram,
			next_grey);
		return;
	}

	next_grey = hwtcon_core_convert_bit_count_2_grey_level(
					task->pipeline_info.next_histogram);
	current_grey = hwtcon_core_convert_bit_count_2_grey_level(
					task->pipeline_info.current_histogram);

    if (task->night_mode)
        wf_mode = night_mode_wf_mode_table[current_grey][next_grey];
    else
        wf_mode = day_mode_wf_mode_table[current_grey][next_grey];

	task->update_data.waveform_mode = wf_mode;

	TCON_LOG("[%d] current_hist_stat = 0x%x[%d] next_hist_stat",
		task->update_data.update_marker,
		task->pipeline_info.current_histogram,
		current_grey);
	TCON_LOG("= 0x%x[%d] new waveform = 0x%x (%s)",
		task->pipeline_info.next_histogram,
		next_grey,
		wf_mode,
		hwtcon_core_get_wf_mode_name(wf_mode));
}

u64 hwtcon_core_get_released_lut(void)
{
	u64 released = 0LL;

	released = ~(hwtcon_fb_info()->lut_free |
	hwtcon_fb_info()->lut_active) &
	LUT_BIT_ALL_SET;
	return released;
}

/* Note: this function only can call in dispatch pipeline thread. */
enum GET_LUT_STATUS_ENUM hwtcon_core_get_free_lut(bool *need_do_clear,
	int *acquired_id)
{
	unsigned long flags;
	u64 free_lut = 0LL;
	u64 active_lut = 0LL;

	spin_lock_irqsave(&hwtcon_fb_info()->lut_free_lock, flags);
	free_lut = hwtcon_fb_info()->lut_free;
	spin_unlock_irqrestore(&hwtcon_fb_info()->lut_free_lock, flags);

	spin_lock_irqsave(&hwtcon_fb_info()->lut_active_lock, flags);
	active_lut = hwtcon_fb_info()->lut_active;
	spin_unlock_irqrestore(&hwtcon_fb_info()->lut_active_lock, flags);

	if (free_lut != 0LL) {
		/* free lut available */
		if (need_do_clear) {
			if (hwtcon_hal_bit_set_cnt(
				hwtcon_core_get_released_lut()) >
				MAX_RELEASED_LUT_COUNT)
				*need_do_clear = true;
			else
				*need_do_clear = false;
		}

		*acquired_id = hwtcon_hal_ffs(free_lut);
		if (*acquired_id < 0) {
			/* should not go here */
			TCON_ERR("calc ffs error: free_lut:0x%016llx id:%d",
				free_lut,
				*acquired_id);
			WARN(1, "calc ffs error");
			goto GETFERR;
		}

		spin_lock_irqsave(&hwtcon_fb_info()->lut_free_lock, flags);
		hwtcon_fb_info()->lut_free &= ~(1LL << *acquired_id);
		spin_unlock_irqrestore(&hwtcon_fb_info()->lut_free_lock, flags);

		spin_lock_irqsave(&hwtcon_fb_info()->lut_active_lock, flags);
		hwtcon_fb_info()->lut_active |= (1LL << *acquired_id);
		spin_unlock_irqrestore(
			&hwtcon_fb_info()->lut_active_lock, flags);

		TCON_LOG("alloc free lut:%d lut status: 0x%016llx",
			*acquired_id,
			hwtcon_fb_info()->lut_free);
		TCON_LOG("0x%016llx 0x%016llx",
			hwtcon_core_get_released_lut(),
			hwtcon_fb_info()->lut_active);

		goto GETOK;
	} else {
		/* free lut not available */
		if (active_lut == LUT_BIT_ALL_SET) {
			/* active lut all busy, need wait WF_LUT release lut*/
			int status = 0;

			status = wait_event_timeout(
				hwtcon_fb_info()->wf_lut_release_wait_queue,
				(hwtcon_fb_info()->lut_active !=
							LUT_BIT_ALL_SET),
				msecs_to_jiffies(
					HWTCON_WAIT_WF_LUT_RELEASE_TIMEOUT));

			if (status == 0) {
				TCON_ERR("wait wf_lut release lut timeout");
				goto GETOUT;
			}
		}
		/* not all busy */
		if (pipeline_init_working_buffer() != 0)
			goto GETFERR;

		/* free lut not avilable need to retrigger get_free_lut */
		goto GETBUSSY;
	}

GETOK:		return GET_LUT_OK;
GETFERR:	return GET_LUT_ERR;
GETBUSSY:	return GET_LUT_BUSY;
GETOUT:	return GET_LUT_TIMEOUT;

}

void hwtcon_core_reset_pipeline(void)
{
	/* write 0 then 1 to reset
	 * 0: pipeline
	 * 1: image buffer rdma
	 * 2: working buffer rdma
	 * 3. working buffer wdma
	 * 4: wf_lut
	 * 5: dpi
	 * 6: tcon
	 * 7: main reset
	 */
	pp_write(NULL, MMSYS_SW1_RST_B, 0xF0);
	pp_write(NULL, MMSYS_SW1_RST_B, 0xFF);
}

void hwtcon_core_reset_mmsys(void)
{
	/* write 0 then 1 to reset
	 * 0: pipeline
	 * 1: image buffer rdma
	 * 2: working buffer rdma
	 * 3. working buffer wdma
	 * 4: wf_lut
	 * 5: dpi
	 * 6: tcon
	 * 7: main reset
	 */
	pp_write(NULL, MMSYS_SW1_RST_B, 0x00);
	pp_write(NULL, MMSYS_SW1_RST_B, 0xFF);
}

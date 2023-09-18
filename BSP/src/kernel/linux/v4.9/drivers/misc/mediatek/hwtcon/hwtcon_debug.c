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

#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <../fs/proc/internal.h>
#include "hwtcon_debug.h"
#include "hwtcon_def.h"
#include "hwtcon_file.h"
#include "hwtcon_core.h"
#include <linux/hwtcon_ioctl_cmd.h>
#include "hwtcon_fb.h"
#include "hwtcon_wf_lut_config.h"
#include "fiti_core.h"
#include "hwtcon_hal.h"
#include "hwtcon_driver.h"
#include "hwtcon_epd.h"
#include "hwtcon_pipeline_config.h"
#include "hwtcon_dpi_config.h"
#include "hwtcon_tcon_config.h"
#include "../cfa/cfa_driver.h"
#include "include/mt-plat/mtk_devinfo.h"

#define MAX_WORD_COUNT 10
#define MAX_CHAR_COUNT 256

typedef void (*proc_write_cmd_func)(int argc, char *argv[]);
typedef int (*print_seq_func)(struct seq_file *m, void *v);

struct hwtcon_debug_info g_debug_info = {
	.log_level = false,
	.fixed_temperature = TEMP_USE_SENSOR,
	.debug = {0},
	.enable_wf_lut_dpi_checksum = 0,
	.enable_wf_lut_checksum = 0,
	.enable_dump_next_buffer = false,
	.enable_dump_image_buffer = false,
	.fiti_power_always_on = false,
	.golden_file_name = {0},
	.debug_va = NULL,
	.collision_debug = 0,
	.mdp_merge_debug = 0,
};

struct proc_write_info {
	char *item_name;
	proc_write_cmd_func item_callback_func;
};

struct proc_read_info {
	struct proc_dir_entry *proc_node;
	char *proc_node_name;
	print_seq_func func_ptr;
};

struct print_buffer {
	bool enable;	/* enable the print buffer to memory function. */
	char *buffer;
	u32 used_size;
	u32 buffer_size;
};

/* for error file print memory */
static struct print_buffer g_error_buffer = {
	.enable = true,
};

static struct print_buffer g_fiti_buffer = {
	.enable = true,
};

static struct print_buffer g_lut_buffer = {
	.enable = false,
};

static struct print_buffer g_temp_buffer = {
	.enable = true,
};

static struct print_buffer g_wf_file_name_buffer = {
	.enable = true,
};

static struct print_buffer g_reserved_buffer_info = {
	.enable = true,
};

static struct print_buffer g_record_buffer = {
	.enable = false,
};


bool is_char(char p)
{
	if (p >= '0' && p <= '9')
		return true;
	if (p >= 'a' && p <= 'z')
		return true;
	if (p >= 'A' && p <= 'Z')
		return true;
	if (p == '_' || p == '-' || p == '=')
		return true;
	if (p == '/' || p == '.')
		return true;

	return false;
}

int hwtcon_debug_force_clock_on(bool enable)
{
	if (enable) {
		hwtcon_driver_force_enable_mmsys_domain(true);
		hwtcon_driver_prepare_clk();
		hwtcon_driver_enable_pipeline_clk(true);
		hwtcon_driver_enable_dpi_clk(true);
	} else {
		hwtcon_driver_enable_pipeline_clk(false);
		hwtcon_driver_enable_dpi_clk(false);
		hwtcon_driver_unprepare_clk();
		hwtcon_driver_force_enable_mmsys_domain(false);
	}

	return 0;
}

/* write proc callback funciton */
void debug_write_log_level(int argc, char *argv[])
{
	int log_level = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &log_level) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_ERR("set loglevel to %d", log_level);
	g_debug_info.log_level = log_level ? true : false;
}


void debug_unzip_wf_lut_test(int argc, char *argv[])
{
	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	snprintf(hwtcon_debug_get_info()->golden_file_name,
		sizeof(hwtcon_debug_get_info()->golden_file_name),
		"/data/%s", argv[0]);
	TCON_ERR("set golden file name:%s",
		hwtcon_debug_get_info()->golden_file_name);
}

void debug_test_unzip_func(int argc, char *argv[])
{
	char zip_file_name[100] = {0};
	char golden_file_name[100] = {0};
	char *zip_buffer = NULL;
	char *unzip_buffer = NULL;
	char *golden_buffer = NULL;
	int zip_file_size = 0;
	int golden_file_size = 0;
	int i = 0;

	snprintf(zip_file_name,
		sizeof(zip_file_name), "/data/wf_lut.gz");
	snprintf(golden_file_name,
		sizeof(golden_file_name),
		"/data/wf_lut.bin");
	zip_file_size = hwtcon_file_get_size(zip_file_name);
	golden_file_size = hwtcon_file_get_size(golden_file_name);

	/* allocate buffer */
	zip_buffer = vmalloc(zip_file_size);
	unzip_buffer = vmalloc(golden_file_size);
	golden_buffer = vmalloc(golden_file_size);

	/* fill buffer content */
	hwtcon_file_read_buffer(zip_file_name, zip_buffer, zip_file_size);
	hwtcon_file_read_buffer(golden_file_name,
		golden_buffer, golden_file_size);
	hwtcon_file_unzip_buffer(zip_buffer, unzip_buffer,
		zip_file_size, golden_file_size,0);

	/* compare unzip buffer with golden buffer */
	for (i = 0; i < golden_file_size; i++)
		if (unzip_buffer[i] != golden_buffer[i])
			break;
	if (i < golden_file_size)
		TCON_ERR("compare fail, index:%d 0x%x - 0x%x",
			i, unzip_buffer[i], golden_buffer[i]);
	else
		TCON_ERR("compare pass");

	vfree(golden_buffer);
	vfree(unzip_buffer);
	vfree(zip_buffer);
}

void debug_write_test(int argc, char *argv[])
{
	/*TCON_ERR("delay counter:0x%08x", hw_tcon_get_edp_dpi_cnt_off());*/
	//schedule_delayed_work(&hwtcon_fb_info()->read_sensor_work,
			//msecs_to_jiffies(SENSOR_READ_INTERVAL_MS));
	int file_size = 0;

	static char my_path_1[60] = "/data/init_bin/wf_lut.gz";
	static char my_path_2[60] = "/data/test/abc.txt";

	file_size = hwtcon_file_get_size(my_path_2);
	if (file_size == 0)
		TCON_ERR("read file:%s fail", my_path_2);

	TCON_ERR("read file:%s ok, size %d", my_path_2, file_size);
	file_size = hwtcon_file_get_size(my_path_1);
	if (file_size == 0)
		TCON_ERR("read file:%s fail", my_path_1);

	TCON_ERR("read file:%s ok,size %d", my_path_1, file_size);

}

cmdqBackupSlotHandle h_backup_slot;

void debug_cmdq_slot_test(int argc, char *argv[])
{
}

void debug_cpu0_hang_simulate(int argc, char *argv[])
{
	spinlock_t lock;
	unsigned long flags;
	HWTCON_TIME start, end;
	int i,j,k,m,n = 0;
	unsigned long sum = 12;
	int count = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &count) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	spin_lock_init(&lock);

	start = timeofday_ms();
	spin_lock_irqsave(&lock, flags);

	for (i = 0; i < 1000; i++)
		for (j = 0; j < 10; j++)
			for (m = 0; m < 10; m++)
				for (n = 0; n < 10; n++)
					for (k = 0; k < count; k++)
						sum *= sum;

	spin_unlock_irqrestore(&lock, flags);
	end = timeofday_ms();

	TCON_ERR("disable CPU0 count:%d irq handler for %d ms value[%ld]",
		count,
		hwtcon_hal_get_time_in_ms(start, end), sum);
}

void debug_cmdq_lut_release_debug(int argc, char *argv[])
{
	CMDQ_VARIABLE reg0 = 0;
	CMDQ_VARIABLE reg1 = 0;
	CMDQ_VARIABLE index = 0;
	CMDQ_VARIABLE result = 0;
	CMDQ_VARIABLE bit_mask = 0;
	CMDQ_VARIABLE dram_pa = 0;
	int i = 0;
	u32 value[LUT_RELEASE_MAX] = {0};

	struct cmdqRecStruct *handle = NULL;
	
	u32 write_value = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtouint(argv[0], 0, &write_value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	hwtcon_debug_force_clock_on(true);

	pp_write(NULL, 0x14004200, write_value);

	TCON_ERR("read 0x14004200 [0x%08x]", pp_read_pa(0x14004200));
	
	for(i = 0; i < LUT_RELEASE_MAX; i++) {
		cmdqBackupWriteSlot(hwtcon_fb_info()->lut_release_slot, i, 0);
	}

	cmdqRecCreate(CMDQ_SCENARIO_HWTCON, &handle);
	cmdqRecReset(handle);

	cmdqRecBackupRegisterToSlot(handle, hwtcon_fb_info()->lut_release_slot, LUT_RELEASE_TIME_START, 0x10008068);

	cmdq_op_read_reg(handle, 0x14004200, &reg0, 0xFFFFFFFF);
	cmdq_op_assign(handle, &index, 0);
	cmdq_op_assign(handle, &bit_mask, 0);
	cmdq_op_assign(handle, &dram_pa, hwtcon_fb_info()->lut_release_slot);

	cmdq_op_while(handle, index, CMDQ_LESS_THAN, 32);
		cmdq_op_assign(handle, &bit_mask, 1);
		cmdq_op_left_shift(handle, &bit_mask, bit_mask, index);
		cmdq_op_and(handle, &result, reg0, bit_mask);
		cmdq_op_if(handle, result, CMDQ_NOT_EQUAL, 0);
			/* write value to slot slot + index * 4 */
			cmdq_op_left_shift(handle, &result, index, 2);
			cmdq_op_add(handle, &result, dram_pa, result);
			cmdq_op_write_mem_with_cpr(handle, result, 1);
		cmdq_op_end_if(handle);

		cmdq_op_add(handle, &index, index, 1);
	cmdq_op_end_while(handle);

	/* reg1 */
	cmdq_op_read_reg(handle, 0x14004200, &reg1, 0xFFFFFFFF);
	cmdq_op_assign(handle, &index, 0);
	cmdq_op_assign(handle, &bit_mask, 0);
	cmdq_op_assign(handle, &dram_pa, hwtcon_fb_info()->lut_release_slot);

	cmdq_op_while(handle, index, CMDQ_LESS_THAN, 32);
		cmdq_op_assign(handle, &bit_mask, 1);
		cmdq_op_left_shift(handle, &bit_mask, bit_mask, index);
		cmdq_op_and(handle, &result, reg0, bit_mask);
		cmdq_op_if(handle, result, CMDQ_NOT_EQUAL, 0);
			/* write value to slot slot + (index + 32) * 4 */
			cmdq_op_add(handle, &result, index, 32);
			cmdq_op_left_shift(handle, &result, result, 2);
			cmdq_op_add(handle, &result, dram_pa, result);
			cmdq_op_write_mem_with_cpr(handle, result, 1);
		cmdq_op_end_if(handle);

		cmdq_op_add(handle, &index, index, 1);
	cmdq_op_end_while(handle);

	cmdqRecBackupRegisterToSlot(handle, hwtcon_fb_info()->lut_release_slot, LUT_RELEASE_TIME_END, 0x10008068);

	cmdqRecFlush(handle);
	cmdqRecDestroy(handle);

	for(i = 0; i < LUT_RELEASE_MAX; i++) {
		cmdqBackupReadSlot(hwtcon_fb_info()->lut_release_slot, i, &value[i]);
		TCON_ERR("read slot[%d] value[0x%08x]", i, value[i]);
	}
	TCON_ERR("total time:%d unit", hwtcon_hal_get_gpt_time_in_unit(value[LUT_RELEASE_TIME_START], value[LUT_RELEASE_TIME_END]));
}

void debug_cmdq_slot_read(int argc, char *argv[])
{
	u32 value[10] = {0};

	cmdqBackupReadSlot(h_backup_slot, 0, &value[0]);
	cmdqBackupReadSlot(h_backup_slot, 1, &value[1]);
	cmdqBackupReadSlot(h_backup_slot, 2, &value[2]);
	cmdqBackupReadSlot(h_backup_slot, 3, &value[3]);

	TCON_ERR("readback value[0x%08x 0x%08x 0x%08x 0x%08x]",
		value[0], value[1], value[2], value[3]);
}

void debug_reload_waveform_file(int argc, char *argv[])
{
	char old_wf_file_name[NAME_MAX];
	int size;

	if (argc < 1) {
		TCON_LOG("usage: echo reload");
		TCON_LOG("[waveform_path] > /proc/hwtcon/cmd");
		}

	size = strlen(hwtcon_driver_get_wf_file_path()) + 1;
	if (size > NAME_MAX)
		size = NAME_MAX;
	memcpy(old_wf_file_name, hwtcon_driver_get_wf_file_path(), size);

	if (argc >= 1)
		hwtcon_driver_set_wf_file_path(argv[0]);

	TCON_ERR("reloading waveform file:%s",
		hwtcon_driver_get_wf_file_path());
	hwtcon_fb_info()->hwtcon_first_call = true;
	hwtcon_fb_info()->ignore_request = true;
	hwtcon_fb_flush_update();
	if (hwtcon_core_load_init_setting_from_file() != 0) {
		TCON_ERR("reload fail, restore to %s", old_wf_file_name);
		hwtcon_driver_set_wf_file_path(old_wf_file_name);
	} else {
		TCON_ERR("reload waveform file %s done",
			hwtcon_driver_get_wf_file_path());
	}
	hwtcon_fb_info()->ignore_request = false;
}

void debug_dump_power_status(int argc, char *argv[])
{
	#ifdef MARKER_V2_ENABLE
	unsigned long flags;
	struct update_marker_struct *update_marker, *tmp;
	#endif

	int i = 0;
	u32 value[LUT_RELEASE_MAX] = {0};

	for(i = 0; i < LUT_RELEASE_MAX; i++) {
		cmdqBackupReadSlot(hwtcon_fb_info()->lut_release_slot, i, &value[i]);
		TCON_ERR("slot[%d] value[0x%x]", i, value[i]);
	}

	TCON_ERR("lut free 0x%016llx active 0x%016llx release 0x%016llx",
		hwtcon_fb_info()->lut_free,
		hwtcon_fb_info()->lut_active,
		hwtcon_core_get_released_lut());
	TCON_ERR("hardware active:0x%08x 0x%08x",
		pp_read(WF_LUT_EN_STA1_VA),
		pp_read(WF_LUT_EN_STA0_VA));
	TCON_ERR("pipeline_processing_task_list count:%d",
		hwtcon_core_get_task_count(
			&hwtcon_fb_info()->pipeline_processing_task_list.list));
	TCON_ERR("pipeline_done_task_list count:%d",
		hwtcon_core_get_task_count(
			&hwtcon_fb_info()->pipeline_done_task_list.list));
	TCON_ERR("wait_for_mdp_task_list count:%d",
		hwtcon_core_get_task_count(
			&hwtcon_fb_info()->wait_for_mdp_task_list.list));
	TCON_ERR("mdp_done_task_list count:%d",
		hwtcon_core_get_task_count(
			&hwtcon_fb_info()->mdp_done_task_list.list));
	TCON_ERR("collision_task_list count:%d",
		hwtcon_core_get_task_count(
			&hwtcon_fb_info()->collision_task_list.list));

	if (hwtcon_core_get_task_count(
		&hwtcon_fb_info()->collision_task_list.list) != 0) {
		struct hwtcon_task *task, *tmp;

		list_for_each_entry_safe(task, tmp,
			&hwtcon_fb_info()->collision_task_list.list, list) {
			TCON_ERR("region:[%d %d %d %d]",
				task->update_data.update_region.left,
				task->update_data.update_region.top,
				task->update_data.update_region.width,
				task->update_data.update_region.height);
			TCON_ERR("lut_dependency:0x%016llx",
				task->lut_dependency);
		}
	}

	#ifdef MARKER_V2_ENABLE
	/* dump all marker not released */
	TCON_ERR("dump all marker in global_marker_list");
	spin_lock_irqsave(&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
	list_for_each_entry_safe(update_marker, tmp,
		&hwtcon_fb_info()->fb_global_marker_list.list, global_list) {
		TCON_ERR("marker[%d] state[%d] lut_mask[0x%016llx]",
			update_marker->update_marker,
			update_marker->marker_state,
			update_marker->lut_mask);
	}
	spin_unlock_irqrestore(
		&hwtcon_fb_info()->fb_global_marker_list.lock, flags);
	#endif
}



/* enable dpi checksum. */
void debug_write_enable_wf_lut_dpi_checksum(int argc, char *argv[])
{
	int enable_checksum = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &enable_checksum) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_ERR("enable_wf_lut_dpi_checksum %d",
		enable_checksum);
	g_debug_info.enable_wf_lut_dpi_checksum = enable_checksum;
}

/* enable wf_lut checksum. */
void debug_write_enable_wf_lut_checksum(int argc, char *argv[])
{
	int enable_checksum = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &enable_checksum) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_ERR("enable_wf_lut_checksum %d",
		enable_checksum);
	g_debug_info.enable_wf_lut_checksum = enable_checksum;
}

void debug_read_temperature(int argc, char *argv[])
{
#ifndef FPGA_EARLY_PORTING
	TCON_ERR("read temp:%d->%d sensor:temp:%d->%d",
		hwtcon_core_read_temperature(),
		hwtcon_core_read_temp_zone(),
		fiti_read_temperature(),
		hwtcon_core_convert_temperature(
			fiti_read_temperature()));
#endif
}

void debug_write_temperature(int argc, char *argv[])
{
	int temp = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &temp) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_ERR("set temperature to %d", temp);
	g_debug_info.fixed_temperature = temp;
}


void debug_write_config_param(int argc, char *argv[])
{
	int debug = 0;
	int i = 0;

	if (argc <= 0 || argc >= ARRAY_SIZE(g_debug_info.debug)) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	for (i = 0; i < argc; i++) {
		if (kstrtoint(argv[i], 0, &debug) != 0) {
			TCON_ERR("invalid argv[%d]:%s", i, argv[i]);
			return;
		}
		g_debug_info.debug[i] = debug;
		TCON_ERR("set debug[%d]:%d", i, g_debug_info.debug[i]);
	}
}

void debug_write_fs_test(int argc, char *argv[])
{
	int i = 0;
	char *file_name = "/data/debug.txt";
	char read_buffer[100] = {0};

	hwtcon_file_printf(file_name, "begin to write file, argc:%d\n",
		argc);
	for (i = 0; i < argc; i++)
		hwtcon_file_printf(file_name, "argv[%d] = %s\n",
			i, argv[i]);
	hwtcon_file_printf(file_name, "write end\n");

	TCON_ERR("begin to test read file");
	hwtcon_file_read_buffer(file_name, read_buffer, sizeof(read_buffer));
	TCON_ERR("read file:%s", read_buffer);
}

void debug_get_file_size(int argc, char *argv[])
{
	int file_size = 0;
	char file_name[100] = {0};

	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}

	snprintf(file_name, sizeof(file_name), "/data/%s", argv[0]);
	TCON_ERR("get file size test:%s", file_name);

	file_size = hwtcon_file_get_size(file_name);
	TCON_ERR("file size:%d %d %d",
		file_size, file_size / 256, file_size % 256);

}

void debug_write_hwtcon_unit_test(int argc, char *argv[])
{
	struct hwtcon_update_data update_data;
	u32 update_marker = 0;
	int status = 0;

	memset(&update_data, 0, sizeof(update_data));
	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &update_marker) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_LOG("unitTest begin");

	update_data.update_region.top = 0;
	update_data.update_region.left = 0;
	update_data.update_region.width = hw_tcon_get_edp_width();
	update_data.update_region.height = hw_tcon_get_edp_height();

	update_data.update_marker = update_marker;

	status = hwtcon_core_submit_task(&update_data);

	TCON_LOG("unitTest end:%d", status);

}

void debug_write_cmdq_test(int argc, char *argv[])
{
	int test_id = 0;
	struct cmdqRecStruct *pkt = NULL;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &test_id) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}
#if 0
	cmdq_unit_test(test_id);
#else
	hwtcon_debug_force_clock_on(true);
	cmdqRecCreate(CMDQ_SCENARIO_PIPELINE, &pkt);
	cmdqRecReset(pkt);

	pp_write(pkt, 0x1400D000, 0xFFFFFFFF);

	cmdqRecFlush(pkt);

	TCON_ERR("read val:0x%08x", pp_read_pa(0x1400D000));
	hwtcon_debug_force_clock_on(false);
#endif

}

void debug_swtcon_source_dpi_test(int argc, char *argv[])
{

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}


#if 0
	cmdq_unit_test(test_id);
#endif


	swtcon_config_context(NULL);
}

void debug_swtcon_data_tcon_test(int argc, char *argv[])
{

#ifndef FPGA_EARLY_PORTING
	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	hwtcon_core_fiti_power_enable(true);

	swdata_hwtcon_config_context(NULL);
#endif
}

void debug_wf_wf_lut_tcon_test(int argc, char *argv[])
{
}


void debug_clock_test(int argc, char *argv[])
{
	int enable = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &enable) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	enable = !!enable;
	TCON_ERR("clock enable:%d", enable);
	hwtcon_debug_force_clock_on(enable);
}

void debug_read_clock_rate(int argc, char *argv[])
{
	/* use command to measure mmsys & dpi clock
	 * clkdbg() {echo $@>/proc/clkdbg; cat /proc/clkdbg;}
	 * clkdbg fmeter
	 * search keyword: mm_dpi_tmp0 & mm_sel
	 */
#if 0
	unsigned long mmsys_rate, dpi_rate;

	if ((IS_ERR(hwtcon_device_info()->clock_info.pipeline0)) ||
		(IS_ERR(hwtcon_device_info()->clock_info.dpi_tmp0))) {
		TCON_ERR("parse pipeline & dpi clock fail");
		return;
	}

	mmsys_rate = clk_get_rate(hwtcon_device_info()->clock_info.pipeline0);
	dpi_rate = clk_get_rate(hwtcon_device_info()->clock_info.dpi_tmp0);

	TCON_ERR("MMSYS clock rate[%ld] DPI clock rate[%ld]",
		mmsys_rate,
		dpi_rate);
#endif
	TCON_ERR("MMSYS clock rate[%ld] DPI clock rate[%ld]",
		hwtcon_driver_get_mmsys_clock_rate(),
		hwtcon_driver_get_dpi_clock_rate());

}


// dump_next [en] [dump_count] [dump_path]
//  [en] 
//   empty : will generate /tmp/next_img.bin
//   0 : disable . 
//  [dump_count]
//   dump image count .
//  [dump_path]
//   dump file output path , eg, "/tmp"
void debug_dump_next_buffer(int argc, char *argv[])
{
	hwtcon_fb_info()->debug_img_buffer_counter = 0;
	hwtcon_fb_info()->debug_img_dump_path[0] = '\0';
	g_debug_info.enable_dump_next_buffer = 0;


	if(0==argc) {
		g_debug_info.enable_dump_next_buffer = 1;
		strcpy(hwtcon_fb_info()->debug_img_dump_path,"/tmp");
	}
	else {
		int iTemp = 0;

		if (kstrtoint(argv[0], 0, &iTemp) != 0) {
			TCON_ERR("invalid argv[0]:%s", argv[0]);
			return;
		}
		g_debug_info.enable_dump_next_buffer = iTemp;

		if(argc>1) {

			// [dump_count]  
			if (kstrtoint(argv[1], 0, &iTemp) != 0) {
				TCON_ERR("invalid argv[1]:%s", argv[1]);
				return;
			}

			hwtcon_fb_info()->debug_img_buffer_counter = iTemp;
			hwtcon_debug_get_info()->enable_dump_image_buffer = false;
		}

		if(argc>2) {
			strcpy(hwtcon_fb_info()->debug_img_dump_path,argv[2]);
		}
		else {
			strcpy(hwtcon_fb_info()->debug_img_dump_path,"/tmp");
		}

	}

	TCON_LOG("enable dump next buffer en=%d cnt=%d path=%s",
			g_debug_info.enable_dump_next_buffer,
			hwtcon_fb_info()->debug_img_buffer_counter,
			hwtcon_fb_info()->debug_img_dump_path);
}

void debug_dump_image_buffer(int argc, char *argv[])
{
	int enable = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &enable) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	g_debug_info.enable_dump_image_buffer = enable;

	TCON_LOG("%s dump image buffer", enable ? "enable" : "disable");
}

void debug_save_image_buffer_to_file(int argc, char *argv[])
{
	int i = 0;

	for (i = 0; i < MAX_DEBUG_IMAGE_BUFFER_COUNT; i++) {
		if (hwtcon_fb_info()->debug_img_buffer_name[i][0] != 0)
			hwtcon_file_save_buffer(
				hwtcon_fb_info()->debug_img_buffer_va[i],
				hwtcon_fb_info()->debug_img_buffer_size[i],
				hwtcon_fb_info()->debug_img_buffer_name[i]);
	}
}

void debug_clear_image_buffer(int argc, char *argv[])
{
	int i = 0;

	for (i = 0; i < MAX_DEBUG_IMAGE_BUFFER_COUNT; i++) {
		memset(hwtcon_fb_info()->debug_img_buffer_name[i], 0,
			MAX_FILE_NAME_LEN);
		memset(hwtcon_fb_info()->debug_img_buffer_va[i], 0,
			hwtcon_fb_info()->debug_img_buffer_size[i]);
	}
}

void debug_dump_last_buffer(int argc, char *argv[])
{
	TCON_LOG("dump previous buffer begin");
	if (hw_tcon_get_epd_type()) {
		hwtcon_file_save_buffer(hwtcon_fb_info()->color_buffer_va,
			hwtcon_fb_info()->color_buffer_size, "/tmp/last_color.bin");
		hwtcon_file_save_buffer(hwtcon_fb_info()->cinfo_buffer_va,
			hwtcon_fb_info()->color_buffer_size, "/tmp/last_cinfo.bin");
	}	
	hwtcon_file_save_buffer(hwtcon_fb_info()->fb_buffer_va,
		hwtcon_fb_info()->fb_buffer_size, "/tmp/last_fb.bin");
	hwtcon_file_save_buffer(hwtcon_fb_info()->img_buffer_va,
		hwtcon_fb_info()->img_buffer_size, "/tmp/last_image.bin");
	hwtcon_file_save_buffer(hwtcon_fb_info()->temp_img_buffer_va,
		hwtcon_fb_info()->temp_img_buffer_size, "/tmp/last_regal.bin");
	hwtcon_file_save_buffer(hwtcon_fb_info()->wb_buffer_va,
		hwtcon_fb_info()->wb_buffer_size, "/tmp/last_wb.bin");
	TCON_LOG("dump previous buffer end");
}

void debug_cfa_mode(int argc, char *argv[])
{
	int cfa_mode = 0;
	int cfa_convert_threads = -1;
	char *pszCFA_mode;
	int iCFA_mode;

	//TCON_LOG("\"%s\" argc=%d,argv[0]=%s,argv[1]=%s",__func__,argv[0], argv[1]);

	if (argc < 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return ;
	}

	pszCFA_mode = argv[0];
	cfa_mode = cfa_get_id_by_mode_name(pszCFA_mode);
	if(-1==cfa_mode) {
		if (kstrtoint(argv[0], 0, &cfa_mode) != 0) {
			
			TCON_ERR("invalid argv[0] (cfa_mode) : %s", argv[0]);

			printk("avalibile modes : \n");
			pszCFA_mode = cfa_get_mode_name(1,&iCFA_mode);
			while(pszCFA_mode) {
				printk(" %s=%d\n",pszCFA_mode,iCFA_mode);
				pszCFA_mode = cfa_get_mode_name(0,&iCFA_mode);
			}
			return;
		}
	}

	if(argc > 1) {
		if (kstrtoint(argv[1], 0, &cfa_convert_threads) != 0) {
			TCON_ERR("invalid argv[1] (threads) : %s", argv[1]);
			return;
		}
	}


	TCON_ERR("set cfa_mode:%d(%s)", cfa_mode,cfa_get_mode_name_by_id(cfa_mode));
	hwtcon_fb_set_cfa_mode(cfa_mode);
	TCON_ERR("  cfa_convert_threads:%d", cfa_convert_threads);
	hwtcon_fb_info()->cfa_convert_threads = cfa_convert_threads;

}


void debug_set_mmsys_power_down_time(int argc, char *argv[])
{
	int time_ms = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &time_ms) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}
	TCON_ERR("set power_down_time:%d ms", time_ms);
	hwtcon_fb_info()->power_down_delay_ms = time_ms;
}

void debug_fiti_reg_write(int argc, char *argv[])
{
	int addr = 0x00;
	int value = 0x00;

	if (argc != 2) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &addr) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (kstrtoint(argv[1], 0, &value) != 0) {
		TCON_ERR("invalid argv[1]:%s", argv[1]);
		return;
	}

	TCON_LOG("set debug_fiti_reg_write addr:0x%x, value:0x%x\n",
		addr, value);

	fiti_i2c_write((unsigned char)addr, (unsigned char)value);

}

void debug_fiti_reg_read(int argc, char *argv[])
{
	int addr = 0x00;
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &addr) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	fiti_i2c_read((unsigned char)addr, (unsigned char *)&value);

	TCON_LOG("set debug_fiti_reg_read addr:0x%x, value:0x%x\n",
		addr, value);
	hwtcon_debug_fiti_printf("fiti_reg_read addr:0x%x, value:0x%x\n",
		addr, value);

}


void debug_fiti_power_control(int argc, char *argv[])
{
#ifndef FPGA_EARLY_PORTING
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_LOG("pmic power set %d",value);

	if (value) {
		const int iMinDelayms=500;
		int iPwrdwn_delay_ms = hwtcon_fb_info()->power_down_delay_ms<iMinDelayms?
			iMinDelayms:hwtcon_fb_info()->power_down_delay_ms;
		//g_debug_info.fiti_power_always_on = true;
		hwtcon_core_fiti_power_enable(true);
		mod_timer(&hwtcon_fb_info()->mmsys_power_timer,
			jiffies + msecs_to_jiffies(iPwrdwn_delay_ms+3000));
		
	} else {
		g_debug_info.fiti_power_always_on = false;
		hwtcon_core_fiti_power_enable(false);
	}
#endif
}

void debug_fiti_vcom_control(int argc, char *argv[])
{
#ifndef FPGA_EARLY_PORTING
	int read_write = 0x00;
	int value = 0x00;

	if (argc != 2) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &read_write) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (kstrtoint(argv[1], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[1]);
		return;
	}

	if (read_write) {
		if (value < 0)
			value = -value;
		fiti_write_vcom(value);
	} else
		TCON_LOG("read vcom value:%d mv", fiti_read_vcom());
#endif
}

void debug_fiti_read_ts(int argc, char *argv[])
{
	int value = 0x00;

	value = wf_lut_waveform_get_temperature_index(
		hwtcon_core_read_temperature());
	hwtcon_debug_fiti_printf("current ts index:%d\n",
		value);
}

void debug_enable_record_printf(int argc, char *argv[])
{
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (value)
		g_record_buffer.enable = true;
	else {
		g_record_buffer.enable = false;
		vfree(g_record_buffer.buffer);
		g_record_buffer.buffer = NULL;
		g_record_buffer.used_size = 0;
		g_record_buffer.buffer_size = 0;
	}
	TCON_ERR("set fs_record:%d", value);
}

void debug_enable_err_printf(int argc, char *argv[])
{
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (value)
		g_error_buffer.enable = true;
	else {
		g_error_buffer.enable = false;
		vfree(g_error_buffer.buffer);
		g_error_buffer.buffer = NULL;
		g_error_buffer.used_size = 0;
		g_error_buffer.buffer_size = 0;
	}

	TCON_ERR("set fs_err:%d", value);

}

void debug_enable_lut_info_printf(int argc, char *argv[])
{
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	g_lut_buffer.enable = value;
	TCON_ERR("set fs_lut_info:%d", value);
}

void debug_setting_fiti_version_number(int argc, char *argv[])
{
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	//fiti_set_version(value);
	TCON_ERR("fiti pmic set version:%d", value);
}

void debug_dump_test(int argc, char *argv[])
{
	TCON_ERR("fb buf[%p 0x%08x] img buf[%p 0x%08x]",
		hwtcon_fb_info()->fb_buffer_va,
		hwtcon_fb_info()->fb_buffer_size,
		hwtcon_fb_info()->img_buffer_va,
		hwtcon_fb_info()->img_buffer_size);

	TCON_ERR("temp img buf[%p 0x%08x] wb buf[%p 0x%08x] wf_file[%p 0x%08x]",
		hwtcon_fb_info()->temp_img_buffer_va,
		hwtcon_fb_info()->temp_img_buffer_size,
		hwtcon_fb_info()->wb_buffer_va,
		hwtcon_fb_info()->wb_buffer_size,
		hwtcon_fb_info()->waveform_va,
		hwtcon_fb_info()->waveform_size);

	TCON_ERR("dump wf_file info ready[%d] file_name[%s]",
		wf_lut_get_wf_info()->wf_file_ready,
		wf_lut_get_wf_info()->wf_file_name);
	TCON_ERR("mode_version[0x%x] wf_type[0x%x] wfm_rev[%d]",
		wf_lut_get_wf_info()->mode_version,
		wf_lut_get_wf_info()->wf_type,
		wf_lut_get_wf_info()->wfm_rev);
}

void debug_collision_scenario(int argc, char *argv[])
{
	int value = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0 ||
		value < 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (value) {
		TCON_ERR("enable force sleep %d ms for collision debug", value);
		g_debug_info.collision_debug = value;
	} else {
		TCON_ERR("disable force collision debug");
		g_debug_info.collision_debug = 0;
	}
}

void debug_collision_merge_in_mdp(int argc, char *argv[])
{
	int value = 0;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0 ||
		value < 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	if (value) {
		TCON_ERR("enable force sleep %d ms for mdp merge debug", value);
		g_debug_info.mdp_merge_debug = value;
	} else {
		TCON_ERR("disable force mdp merge debug");
		g_debug_info.mdp_merge_debug = 0;
	}
}


void debug_clear_active_lut(int argc, char *argv[])
{
	unsigned long flags;

	/* update active lut */
	spin_lock_irqsave(&hwtcon_fb_info()->lut_active_lock, flags);

	hwtcon_fb_info()->lut_active = 0;

	spin_unlock_irqrestore(&hwtcon_fb_info()->lut_active_lock, flags);

}

void debug_read_hardware_version(int argc, char *argv[])
{
	TCON_ERR("read hw version:0x%08x", get_devinfo_with_index(65));
}

void debug_enable_nightmode(int argc, char *argv[])
{
	int nightmode;
	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}
	if (kstrtoint(argv[0], 0, &nightmode) != 0) {
		TCON_ERR("invalid argv[0]:%s not int !", argv[0]);
		return;
	}
	if(2==nightmode) {
		hwtcon_fb_info()->enable_night_mode_by_wfm = true;
	}
	else if(1==nightmode) {
		hwtcon_fb_info()->enable_night_mode_by_wfm = false;
		hwtcon_fb_ioctl_set_night_mode(1,1,1);
	}
	else if(3==nightmode) {
		hwtcon_fb_info()->enable_night_mode_by_wfm = false;
		hwtcon_fb_ioctl_set_night_mode(1,0,1);
	}
	else if(0==nightmode) {
		hwtcon_fb_info()->enable_night_mode_by_wfm = false;
		hwtcon_fb_ioctl_set_night_mode(0,0,0);
	}
	else if(4==nightmode) {
		hwtcon_fb_info()->enable_night_mode_by_wfm = false;
		hwtcon_fb_ioctl_set_night_mode(0,1,0);
	}
	else {
		TCON_ERR("please echo [0|1|2|3]");
		TCON_ERR(" 0:disable nightmode .");
		TCON_ERR(" 1:force enable night mode with inverted fb and nm wfm mapping.");
		TCON_ERR(" 2:enable night mode by waveform mode .");
		TCON_ERR(" 3:force enable night mode w/o inverted fb and nm wfm mapping.");
		TCON_ERR(" 4:just use inverted fb .");
	}
}

void debug_ep_vdd_ctrl(int argc, char *argv[])
{
	int vdd_uV;
	if (argc != 1) {
		TCON_ERR("invalid argc:%d", argc);
		return;
	}
	if (kstrtoint(argv[0], 0, &vdd_uV) != 0) {
		TCON_ERR("invalid argv[0]:%s not int !", argv[0]);
		return;
	}

	if(vdd_uV>0) {
		edp_vdd_enable();
	}
	else if(0==vdd_uV){
		edp_vdd_disable();
	}
	else {
		TCON_ERR("invalid parameters %s ! please try 1|0 ", argv[0]);
	}
}


void debug_epd_pinmux_control(int argc, char *argv[])
{
	int value = 0x00;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &value) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	TCON_LOG("%s() en=%d",__func__,value);

	if (value)
		hwtcon_edp_pinmux_active();
	else
		hwtcon_edp_pinmux_inactive();
}


void debug_read_register_val(int argc, char *argv[])
{
	u32 *va = NULL;
	u32 pa = 0;
	u32 base_pa = 0;
	int i = 0;

	if (argc != 1) {
		TCON_ERR("invalid usage");
		return;
	}

	if (kstrtoint(argv[0], 0, &pa) != 0) {
		TCON_ERR("invalid input:%s", argv[0]);
		return;
	}

	base_pa = pa & 0xFFFFF000;

	#if 0
	va = ioremap(pa, sizeof(u32));
	TCON_ERR("read pa:0x%08x val:0x%08x", pa, pp_read(va));
	#else
	va = ioremap(base_pa, 0x1000);

	hwtcon_debug_force_clock_on(true);
	for (i = 0; i < 0x1000 / 4; i = i + 4) {
		pr_notice("0x%08x: 0x%08x\t 0x%08x\t 0x%08x\t 0x%08x\n",
			base_pa + i * 4 + 0,
			pp_read(va + i + 0),
			pp_read(va + i + 1),
			pp_read(va + i + 2),
			pp_read(va + i + 3));
	}
	hwtcon_debug_force_clock_on(false);

	#endif
	iounmap(va);
}

void debug_dump_all_register(int argc, char *argv[])
{
	u32 register_pa = 0x14000000;
	u32 dump_size = 0xE000;
	u32 *va = ioremap(register_pa, dump_size);
	int i = 0;
	char print_buffer[200];
	struct fs_struct file;
	int status = 0;

	init_fs_struct(&file);
	status = file.fs_create(&file, "/tmp/dump_reg.txt", FS_MODE_RW);
	if (status != 0) {
		TCON_ERR("open /tmp/dump_reg.txt fail:%d", status);
		iounmap(va);
		return;
	}

	hwtcon_debug_force_clock_on(true);
	#if 0
	for (i = 0; i < dump_size / 4; i = i + 4) {
		snprintf(print_buffer, sizeof(print_buffer),
			"0x%08x: 0x%08x\t 0x%08x\t 0x%08x\t 0x%08x\n",
			register_pa + i * 4 + 0,
			pp_read(va + i + 0),
			pp_read(va + i + 1),
			pp_read(va + i + 2),
			pp_read(va + i + 3));
		file.fs_write(&file, print_buffer,
			strlen(print_buffer));
	}
	#else
	for (i = 0; i < dump_size / 4; i = i + 4) {
		snprintf(print_buffer, sizeof(print_buffer),
			"0x%016X %08X\n",
			register_pa + (i + 0) * 4,
			pp_read(va + i + 0));
		file.fs_write(&file, print_buffer,
			strlen(print_buffer));
		snprintf(print_buffer, sizeof(print_buffer),
			"0x%016X %08X\n",
			register_pa + (i + 1) * 4,
			pp_read(va + i + 1));
		file.fs_write(&file, print_buffer,
			strlen(print_buffer));
		snprintf(print_buffer, sizeof(print_buffer),
			"0x%016X %08X\n",
			register_pa + (i + 2) * 4,
			pp_read(va + i + 2));
		file.fs_write(&file, print_buffer,
			strlen(print_buffer));
		snprintf(print_buffer, sizeof(print_buffer),
			"0x%016X %08X\n",
			register_pa + (i + 3) * 4,
			pp_read(va + i + 3));
		file.fs_write(&file, print_buffer,
			strlen(print_buffer));
	}
	#endif
	hwtcon_debug_force_clock_on(false);
	file.fs_close(&file);
	iounmap(va);
}

void debug_write_register_val(int argc, char *argv[])
{
	u32 pa = 0;
	u32 value = 0;

	if (argc != 2) {
		TCON_ERR("invalid usage");
		return;
	}

	if ((kstrtoint(argv[0], 0, &pa) != 0) ||
		(kstrtouint(argv[1], 0, &value) != 0)) {
		TCON_ERR("invalid input:%s %s",
			argv[0],
			argv[1]);
		return;
	}
	TCON_ERR("write pa:0x%08x with value:0x%08x", pa, value);
	hwtcon_debug_force_clock_on(true);
	pp_write(NULL, pa, value);
	hwtcon_debug_force_clock_on(false);
}

void debug_mmap_test(int argc, char *argv[])
{
	int pa_addr = 0x00;
	u32 mmap_size = 0x10000;

	if (argc != 1) {
		TCON_ERR("invalid argc:%d\n", argc);
		return;
	}

	if (kstrtoint(argv[0], 0, &pa_addr) != 0) {
		TCON_ERR("invalid argv[0]:%s", argv[0]);
		return;
	}

	g_debug_info.debug_va = ioremap(pa_addr, mmap_size);
	TCON_ERR("mmap pa:0x%08x size:%d va:%p",
		pa_addr, mmap_size,
		g_debug_info.debug_va);
}


void debug_get_waveform_mode(int argc, char *argv[])
{
	wf_lut_print_current_loaded_wavefrom_info();
}

static struct proc_write_info g_proc_write[] = {
		{"read", debug_read_register_val},
		{"reload", debug_reload_waveform_file},
		{"dump_reg", debug_dump_all_register},
		{"write", debug_write_register_val},
		{"mmap", debug_mmap_test},
		{"log", debug_write_log_level},
		{"zip", debug_test_unzip_func},
		{"zip_lut", debug_unzip_wf_lut_test},
		{"test", debug_write_test},
		{"hang", debug_cpu0_hang_simulate},
		{"slot", debug_cmdq_slot_test},
		{"slot_read", debug_cmdq_slot_read},
		{"debug", debug_write_config_param},
		{"fs_test", debug_write_fs_test},
		{"fs_size", debug_get_file_size},
		{"hwtcon_test", debug_write_hwtcon_unit_test},
		{"dpi_checksum", debug_write_enable_wf_lut_dpi_checksum},
		{"wf_lut_checksum", debug_write_enable_wf_lut_checksum},
		{"read_temp", debug_read_temperature},
		{"write_temp", debug_write_temperature},
		{"cmdq", debug_write_cmdq_test},
		{"swdata_dpi", debug_swtcon_source_dpi_test},
		{"swdata_tcon", debug_swtcon_data_tcon_test},
		{"wf_lut_tcon", debug_wf_wf_lut_tcon_test},
		{"clock", debug_clock_test},
		{"clock_rate", debug_read_clock_rate},

		{"dump_image_en", debug_dump_image_buffer},
		{"save_image", debug_save_image_buffer_to_file},
		{"clear_image", debug_clear_image_buffer},

		{"dump_last", debug_dump_last_buffer},
		{"dump_next", debug_dump_next_buffer},

		{"power_down_time", debug_set_mmsys_power_down_time},
		{"status", debug_dump_power_status},
		{"fiti_write", debug_fiti_reg_write},
		{"fiti_read", debug_fiti_reg_read},
		{"fiti_power", debug_fiti_power_control},
		{"pinmux", debug_epd_pinmux_control},
		{"get_waveform", debug_get_waveform_mode},
		{"fiti_ts", debug_fiti_read_ts},
		{"fiti_vcom", debug_fiti_vcom_control},
		{"fs_record", debug_enable_record_printf},
		{"fs_err", debug_enable_err_printf},
		{"fs_lut_info", debug_enable_lut_info_printf},
		{"fiti_version", debug_setting_fiti_version_number},
		{"dump", debug_dump_test},
		{"col_debug", debug_collision_scenario},
		{"mdp_debug", debug_collision_merge_in_mdp},
		{"clear", debug_clear_active_lut},
		{"hw_version", debug_read_hardware_version},
		{"night_mode", debug_enable_nightmode},
		{"ep_vdd", debug_ep_vdd_ctrl},
		{"cfa_mode", debug_cfa_mode},
};

int hwtcon_debug_process_string(char *str)
{
	int word_count = 0;
	int find_word = 0;
	int i = 0;
	char *pbeg = str;
	char *pend = str;
	char *buffer[MAX_WORD_COUNT] = {NULL};

	for (i = 0; i < ARRAY_SIZE(buffer); i++) {
		buffer[i] = vmalloc(MAX_CHAR_COUNT);
		memset(buffer[i], 0, MAX_CHAR_COUNT);
	}

	/* split string to word , store in buffer[] */
	while (*pend != '\0') {
		if (find_word == 0 && is_char(*pend)) {
			/*find a word begin */
			find_word = 1;
			pbeg = pend;
		} else if (find_word == 1 && !is_char(*pend)) {
			/* find a word end */
			find_word = 0;
			/* copy pbeg ~ pend to buffer */
			memcpy(buffer[word_count++], pbeg,
				(pend - pbeg) > (MAX_CHAR_COUNT - 1) ?
				(MAX_CHAR_COUNT - 1) : (pend - pbeg));
			if (word_count >= MAX_WORD_COUNT)
				break;
		}
		pend++;
	}
	if (find_word == 1)
		memcpy(buffer[word_count++], pbeg,
			(pend - pbeg) > (MAX_CHAR_COUNT - 1) ?
			(MAX_CHAR_COUNT - 1) : (pend - pbeg));

	/* search item, call item related callback function.
	 * item name store in buffer[0]
	 * item param store in buffer[1] ~ buffer[word_count -1]
	 */

	for (i = 0; i < word_count; i++)
		TCON_LOG("buffer[%d]:%s,len=%d", i, buffer[i],strlen(buffer[i]));

	for (i = 0; i < ARRAY_SIZE(g_proc_write); i++) {
		if ((strlen(g_proc_write[i].item_name) == strlen(buffer[0])) &&
			strncmp(g_proc_write[i].item_name,buffer[0],
				strlen(g_proc_write[i].item_name)) == 0) 
		{
			g_proc_write[i].item_callback_func(word_count - 1,&buffer[1]);
			break;
		}
	}

	if (i == ARRAY_SIZE(g_proc_write))
		TCON_ERR("invalid cmd:%s", str);

	for (i = 0; i < ARRAY_SIZE(buffer); i++)
		vfree(buffer[i]);

	return 0;

}

ssize_t hwtcon_debug_write_cmd(struct file *f,
	const char __user *user_buffer,
	size_t size, loff_t *offset)
{
	size_t copied_size = 0;
	char *buffer = vmalloc(size + 1);

	memset(buffer, 0, size + 1);
	do {
		if (copy_from_user(buffer, user_buffer, size) != 0) {
			TCON_ERR("copy write cmd from user fail");
			copied_size = 0;
			break;
		}
		copied_size = size;
		buffer[size] = '\0';

		TCON_LOG("cmd: %s", buffer);
		hwtcon_debug_process_string(buffer);
	} while (0);
	vfree(buffer);
	return copied_size;
}



/* read proc callback funciton */
int record_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_record_buffer.buffer);
	return 0;
}

int error_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_error_buffer.buffer);
	return 0;
}

int fiti_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_fiti_buffer.buffer);
	return 0;
}

int lut_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_lut_buffer.buffer);
	return 0;
}

int temperature_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_temp_buffer.buffer);
	return 0;
}

int wf_file_name_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_wf_file_name_buffer.buffer);
	return 0;
}

int reserved_buffer_info(struct seq_file *m, void *v)
{
	seq_printf(m, "%s\n", g_reserved_buffer_info.buffer);
	return 0;
}


static void hwtcon_debug_printf(struct print_buffer *buffer_info,
	const char *msg, va_list args)
{
	char print_msg[512];
	/* print buffer function enable ?
	 * not allow to write to memory if not enable.
	 */
	if (!buffer_info->enable)
		return;

	/* convert msg to print_msg */
	vsnprintf(print_msg, sizeof(print_msg), msg, args);

	/* allocate buffer if not exist. */
	if (buffer_info->buffer == NULL) {
		buffer_info->buffer = vmalloc(PAGE_SIZE);
		if (buffer_info->buffer == NULL) {
			TCON_ERR("vmalloc buffer fail, size:%lu",
				PAGE_SIZE);
			return;
		}
		buffer_info->buffer_size = PAGE_SIZE;
		buffer_info->used_size = 0;
	}

	/* check if buffer sitll enough. */
	if (buffer_info->buffer_size - buffer_info->used_size <
		strlen(print_msg) + 1) {
		/* allocate a larger one. */
		char *new_buffer = vmalloc(
			PAGE_SIZE + buffer_info->buffer_size);

		if (new_buffer == NULL) {
			TCON_ERR("allocate buffer fail, size:%lu",
				PAGE_SIZE + buffer_info->buffer_size);
			return;
		}

		/* copy the old buffer to new buffer */
		memcpy(new_buffer, buffer_info->buffer,
			buffer_info->used_size);

		/* free old buffer */
		vfree(buffer_info->buffer);

		/* update the g_error_buffer info */
		buffer_info->buffer = new_buffer;
		buffer_info->buffer_size =
			PAGE_SIZE + buffer_info->buffer_size;
	}

	/* write print_buffer to g_error_buffer.buffer */
	buffer_info->used_size += snprintf(
		buffer_info->buffer + buffer_info->used_size,
		buffer_info->buffer_size - buffer_info->used_size - 1,
		"%s", print_msg);
}

/* print log to /proc/hwtcon/error file */
void hwtcon_debug_err_printf(const char *print_msg, ...)
{
	va_list args;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_error_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_record_printf(const char *print_msg, ...)
{
	va_list args;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_record_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_fiti_printf(const char *print_msg, ...)
{
	va_list args;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_fiti_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_lut_info_printf(const char *print_msg, ...)
{
	va_list args;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_lut_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_temp_info_printf(const char *print_msg, ...)
{
	va_list args;

	/* clear g_temp_buffer */
	if (g_temp_buffer.buffer)
		g_temp_buffer.used_size = 0;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_temp_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_wf_file_name_info_printf(const char *print_msg, ...)
{
	va_list args;

	/* clear g_wf_file_name_buffer */
	if (g_wf_file_name_buffer.buffer)
		g_wf_file_name_buffer.used_size = 0;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_wf_file_name_buffer, print_msg, args);
	va_end(args);
}

void hwtcon_debug_reserved_buffer_info_printf(const char *print_msg, ...)
{
	va_list args;

	/* clear g_reserved_buffer_info */
	if (g_reserved_buffer_info.buffer)
		g_reserved_buffer_info.used_size = 0;

	va_start(args, print_msg);
	hwtcon_debug_printf(&g_reserved_buffer_info, print_msg, args);
	va_end(args);
}




#undef DECLARE_ITEM
#define DECLARE_ITEM(name, func) \
	{NULL, name, func},

static struct proc_read_info g_proc_read[] = {
	#include "hwtcon_debug_read.h"
};

int hwtcon_debug_read(struct inode *inode, struct file *file)
{
	int i = 0;
	struct proc_inode *node = container_of(inode,
		struct proc_inode, vfs_inode);

	for (i = 0; i < ARRAY_SIZE(g_proc_read); i++) {
		if (g_proc_read[i].proc_node == node->pde) {
			TCON_ERR("cat %s", g_proc_read[i].proc_node_name);
			break;
		}
	}

	/* proc entry not found. */
	if (i == ARRAY_SIZE(g_proc_read))
		return -EFAULT;

	if (strncmp(g_proc_read[i].proc_node_name, "temp",
			strlen("temp")) == 0) {
		/* read temperature */
		hwtcon_debug_temp_info_printf("temp:%d temp_zone:%d",
			hwtcon_core_read_temperature(),
			hwtcon_core_read_temp_zone());
	} else if (strncmp(g_proc_read[i].proc_node_name, "waveform_version",
			strlen("waveform_version")) == 0) {
		/* read waveform file name */
		hwtcon_debug_wf_file_name_info_printf("%s",
			wf_lut_get_wf_info()->wf_file_name);
	} else if (strncmp(g_proc_read[i].proc_node_name, "reserved_buf",
			strlen("reserved_buf")) == 0) {
		if (hwtcon_device_info()->reserved_buf_ready)
			hwtcon_debug_reserved_buffer_info_printf(
				"0x%08x 0x%08x",
				hwtcon_device_info()->reserved_buf_pa,
				hwtcon_device_info()->reserved_buf_pa +
					WAVEFORM_SIZE);
	}

	return single_open(file, g_proc_read[i].func_ptr, inode->i_private);
}

#undef DECLARE_ITEM
#define DECLARE_ITEM(name, func) \
	{ \
		.owner = THIS_MODULE, \
		.open = hwtcon_debug_read, \
		.read = seq_read, \
		.llseek = seq_lseek, \
		.release = single_release, \
	}, \

/* fops for read proc */
static const struct file_operations fops[] = {
	#include "hwtcon_debug_read.h"
};

/* fops for write proc */
static const struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.open = NULL,
	.read = NULL,
	.write = hwtcon_debug_write_cmd,
};

ssize_t wb_show(struct file *file, struct kobject *obj,
	struct bin_attribute *attr,
	char *buffer, loff_t offset, size_t count)
{
	if (offset + count > hwtcon_fb_info()->wb_buffer_size)
		count = hwtcon_fb_info()->wb_buffer_size - offset;

	memcpy(buffer, hwtcon_fb_info()->wb_buffer_va + offset,
		count);

	return count;
}

struct bin_attribute wb_bin_attr = {

	.attr = {
		.name = "wb",
		.mode = 0444,
	},
	.read = wb_show,
};


static struct proc_dir_entry *folder_ptr;
static struct proc_dir_entry *eink_folder_ptr;
static struct proc_dir_entry *waveform_folder_ptr;
static struct proc_dir_entry *file_ptr;

int hwtcon_debug_create_procfs(void)
{
	int i = 0;
	int status = 0;

	do {
		wb_bin_attr.size = hwtcon_fb_info()->wb_buffer_size;
		if (sysfs_create_bin_file(
			&hwtcon_fb_info()->dev->kobj, &wb_bin_attr) != 0) {
			TCON_ERR("create wb sysfs fail");
			status = HWTCON_STATUS_CREATE_FS_FAIL;
			break;
		}

		folder_ptr = proc_mkdir("hwtcon", NULL);
		if (folder_ptr == NULL) {
			TCON_ERR("create /proc/hwtcon folder fail");
			status = HWTCON_STATUS_CREATE_FS_FAIL;
			break;
		}

		eink_folder_ptr = proc_mkdir("eink", NULL);
		if (eink_folder_ptr == NULL) {
			TCON_ERR("create /proc/eink/ folder fail");
			status = HWTCON_STATUS_CREATE_FS_FAIL;
			break;
		}

		waveform_folder_ptr = proc_mkdir("waveform", eink_folder_ptr);
		if (waveform_folder_ptr == NULL) {
			TCON_ERR("create /proc/eink/waveform folder fail");
			status = HWTCON_STATUS_CREATE_FS_FAIL;
			break;
		}
		/* create file nodes in /proc/hwtcon/ */
		for (i = 0; i < ARRAY_SIZE(g_proc_read); i++) {
			if (strncmp(g_proc_read[i].proc_node_name,
				"waveform_version",
				strlen("waveform_version")) == 0)
				g_proc_read[i].proc_node = proc_create(
					g_proc_read[i].proc_node_name,
					0660, waveform_folder_ptr,
					&fops[i]);
			else
				g_proc_read[i].proc_node = proc_create(
					g_proc_read[i].proc_node_name,
					0660, folder_ptr,
					&fops[i]);

			if (g_proc_read[i].proc_node == NULL) {
				TCON_ERR("create /proc/hwtcon/%s fail",
					g_proc_read[i].proc_node_name);
				status = HWTCON_STATUS_CREATE_FS_FAIL;
				break;
			}
		}
		if (i != ARRAY_SIZE(g_proc_read))
			break;

		/* create file node in /proc/hwtcon/cmd */
		file_ptr = proc_create("cmd", 0660, folder_ptr, &write_fops);
		if (file_ptr == NULL) {
			TCON_ERR("create /proc/hwtcon/cmd fail");
			status = HWTCON_STATUS_CREATE_FS_FAIL;
			break;
		}
	} while (0);


	return status;
}

int hwtcon_debug_destroy_procfs(void)
{
	int i = 0;

	sysfs_remove_bin_file(&hwtcon_fb_info()->dev->kobj, &wb_bin_attr);

	/* /proc/hwtcon/cmd */
	if (file_ptr != NULL) {
		proc_remove(file_ptr);
		file_ptr = NULL;
	}

	/* /proc/hwtcon */
	for (i = 0; i < ARRAY_SIZE(g_proc_read); i++)
		if (g_proc_read[i].proc_node != NULL) {
			proc_remove(g_proc_read[i].proc_node);
			g_proc_read[i].proc_node = NULL;
		}

	/* /proc/hwtcon/ folder */
	if (folder_ptr != NULL) {
		proc_remove(folder_ptr);
		folder_ptr = NULL;
	}

	/* /proc/eink/waveform/ folder */
	if (waveform_folder_ptr != NULL) {
		proc_remove(waveform_folder_ptr);
		waveform_folder_ptr = NULL;
	}

	/* /proc/eink/ folder */
	if (eink_folder_ptr != NULL) {
		proc_remove(eink_folder_ptr);
		eink_folder_ptr = NULL;
	}

	return 0;
}

struct hwtcon_debug_info *hwtcon_debug_get_info(void)
{
	return &g_debug_info;
}


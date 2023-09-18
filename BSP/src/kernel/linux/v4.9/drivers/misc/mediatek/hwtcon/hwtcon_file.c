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
#include <linux/zlib.h>
#include <linux/slab.h>

#include "hwtcon_file.h"
#include "hwtcon_def.h"
#include "hwtcon_fb.h"
#include "decompress_inflate.h"

/*internal use*/
static int fs_create(struct fs_struct *file, const char *fileName,
	enum FS_OPEN_MODE_ENUM open_mode);
static int fs_read(struct fs_struct *file, char *buffer, u32 size);
static void fs_write(struct fs_struct *file, char *buffer, u32 size);
static void fs_close(struct fs_struct *file);
static int fs_get_size(struct fs_struct *file, const char *fileName);

void init_fs_struct(struct fs_struct *file)
{
	file->fs = 0;
	file->fp = NULL;
	file->fs_create = fs_create;
	file->fs_read = fs_read;
	file->fs_write = fs_write;
	file->fs_close = fs_close;
	file->fs_get_size = fs_get_size;
}

static int fs_create(struct fs_struct *file, const char *fileName,
	enum FS_OPEN_MODE_ENUM open_mode)
{
	if (fileName == NULL || *fileName == '\0') {
		TCON_ERR("illegal fileName");
		return HWTCON_STATUS_INVALID_PARAM;
	}
	file->fs = get_fs();
	set_fs(KERNEL_DS);

	if (open_mode == FS_MODE_READ_ONLY)
		file->fp = filp_open(fileName, open_mode, 0444);
	else
		file->fp = filp_open(fileName, open_mode, 0644);

	if (IS_ERR(file->fp)) {
		TCON_ERR("create file[%s] error, fp[%p]", fileName, file->fp);
		set_fs(file->fs);
		return HWTCON_STATUS_OPEN_FILE_FAIL;
	}
	return 0;
}

static void fs_write(struct fs_struct *file, char *buffer, u32 size)
{
	#if 0
	/* NOTE: in FPGA env, the file->fp->f_op->write is NULL. */
	file->fp->f_op->write(file->fp, buffer, size, &file->fp->f_pos);
	#else
	vfs_write(file->fp, buffer, size, &file->fp->f_pos);
	#endif
}

static int fs_read(struct fs_struct *file, char *buffer, u32 size)
{
	#if 0
	return file->fp->f_op->read(file->fp, buffer, size, &file->fp->f_pos);
	#else
	return vfs_read(file->fp, buffer, size, &file->fp->f_pos);
	#endif
}

static void fs_close(struct fs_struct *file)
{
	if (file->fp)
		filp_close(file->fp, NULL);

	set_fs(file->fs);
}

static int fs_get_size(struct fs_struct *file, const char *fileName)
{
	struct kstat stat = {0};

	vfs_stat(fileName, &stat);
	return stat.size;
}

int hwtcon_file_save_buffer(char *buffer, u32 size, const char *file_name)
{
	struct fs_struct file;
	int status = 0;

	TCON_LOG("dump file:%s with size:%d", file_name, size);
	init_fs_struct(&file);
	status = fs_create(&file, file_name, FS_MODE_RW);
	if (status != 0)
		return status;
	fs_write(&file, buffer, size);
	fs_close(&file);
	return 0;
}

int hwtcon_file_save_buffer_append(char *buffer, u32 size,
		const char *file_name)
{
	struct fs_struct file;
	int status = 0;

	init_fs_struct(&file);
	status = fs_create(&file, file_name, FS_MODE_RW_APPEND);
	if (status != 0)
		return status;
	fs_write(&file, buffer, size);
	fs_close(&file);
	return 0;
}


int hwtcon_file_printf(const char *file_name, char *fmt, ...)
{
	char buffer[512];
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	return hwtcon_file_save_buffer(buffer, strlen(buffer), file_name);
}

int hwtcon_file_read_buffer(char *file_name, char *buffer, int buffer_size)
{
	struct fs_struct file;
	int status = 0;
	int read_size = 0;

	init_fs_struct(&file);
	status = fs_create(&file, file_name, FS_MODE_READ_ONLY);
	if (status != 0)
		return status;
	read_size = fs_read(&file, buffer, buffer_size);
	if (read_size != buffer_size) {
		TCON_ERR("read size:%d not match buffer size:%d",
			read_size,
			buffer_size);
	}
	fs_close(&file);

	return 0;

}

int hwtcon_file_get_size(char *file_name)
{
	struct fs_struct file;
	int status = 0;
	int file_size = 0;

	init_fs_struct(&file);
	status = fs_create(&file, file_name, FS_MODE_READ_ONLY);
	if (status != 0)
		return 0;

	/* get file size */
	file_size = fs_get_size(&file, file_name);

	fs_close(&file);
	return file_size;
}

/* unzip data */
int hwtcon_file_unzip_buffer(char *zip_buffer,
	char *unzip_buffer,
	int zip_length,
	int unzip_max_length,
	unsigned long *unzip_bytes)
{
	int iRet;
	long pos;

	unsigned long ulbytes;

	iRet = hwtcon_gunzip(zip_buffer, zip_length,
		NULL, NULL, unzip_buffer, unzip_max_length, &pos, NULL , &ulbytes);

	if(unzip_bytes) {
		*unzip_bytes = ulbytes;
	}

	if(ulbytes>=unzip_max_length) {
		TCON_WARN("unzip buffer might not big enough ! current unzip buffer size=%d",
			unzip_max_length);
	}

	return iRet;
}

// SPDX-License-Identifier: MediaTekProprietary
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "hwtcon_display.h"
#include "hwtcon_ioctl_cmd.h"
#include "bmp_decoder.h"
#include "unistd.h"

int hwtcon_test_hand_write(int argc, char *argv[])
{

	int status = 0, marker = 0;
	unsigned int i = 0;
	struct rect display_region = {0};
	unsigned int panel_width = 0;
	unsigned int panel_height = 0;
	unsigned int buffer_pitch = 0;
	int update_mode = UPDATE_MODE_FULL;
	int waveform_mode = 2;
	int temperature = 20;
	int color = 8;
	unsigned char *ptr = NULL;	
	unsigned int dither_mode = 0;	
	unsigned char *buffer = NULL;
	struct hwtcon_update_data update_data;

 	struct hwtcon_rect regions[] = {
		{ 373,	311,	4,	  4},
		{ 372,	310,	5,	  5},
		{ 370,	308,	6,	  7},
		{ 366,	306,	9,	  8},
		{ 363,	304,	8,	  7},
		{ 359,	301,	9,	  9},
		{ 356,	298,	9,	  9},
		{ 352,	294,	9,	  9},
		{ 349,	291,	9,	  9},
		{ 347,	288,	8,	  8},
		{ 342,	283,   11,	 10},
		{ 339,	279,   10,	 10},
		{ 335,	274,   11,	 12},
		{ 330,	268,   12,	 12},
		{ 327,	263,   10,	 11},
		{ 322,	257,   11,	 12},
		{ 316,	252,   12,	 12},
		{ 312,	248,   11,	 11},
		{ 306,	243,   12,	 12},
		{ 300,	237,   12,	 12},
		{ 296,	233,   11,	 11},
		{ 290,	228,   12,	 12},
		{ 284,	223,   12,	 11},
		{ 279,	219,   12,	 11},
		{ 271,	213,   14,	 12},
		{ 263,	208,   14,	 12},
		{ 258,	203,   12,	 12},
		{ 251,	198,   13,	 12},
		{ 245,	193,   12,	 11},
		{ 238,	188,   14,	 12},
		{ 232,	185,   12,	 10},
		{ 223,	180,   15,	 12},
		{ 215,	176,   14,	 11},
		{ 208,	172,   13,	 11},
		{ 200,	168,   14,	 11},
		{ 191,	164,   15,	 11},
		{ 183,	159,   14,	 12},
		{ 173,	154,   16,	 12},
		{ 164,	147,   15,	 14},
		{ 158,	142,   12,	 12},
		{ 150,	136,   14,	 13},
		{ 143,	130,   13,	 12},
		{ 136,	125,   13,	 11},
		{ 128,	117,   15,	 14},
		{ 120,	110,   15,	 14},
		{ 113,	105,   14,	 13},
		{ 104,	 96,   17,	 16},
		{  96,	 91,   15,	 13},
		{  90,	 86,   13,	 12},
		{  86,	 82,   11,	 11},
		{  80,	 78,   13,	 11},
		{  78,	 76,	9,	 10},
		{  77,	 74,	8,	  8},
		{  77,	 73,	5,	  6},
		};

	if (argc == 5) {
		color = atoi(argv[1]);
		update_mode = atoi(argv[2]);
		waveform_mode = atoi(argv[3]);
		temperature = atoi(argv[4]);
	} else if(argc == 6) {
		color = atoi(argv[1]);
		update_mode = atoi(argv[2]);
		waveform_mode = atoi(argv[3]);
		temperature = atoi(argv[4]);		
		dither_mode = strtol(argv[5], &ptr, 16);	
	} else {
		TCON_ERR("invalid usage: ./display hand color_value 1 2 29 168");
		TCON_ERR("color: 0 -> 15");
		return -1;
	}
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}


	buffer_pitch = panel_width;
	
	buffer = (unsigned char *)calloc(1, buffer_pitch * panel_height);

	for( i = 0; i < sizeof(regions) / sizeof(struct hwtcon_rect); i++) {
		display_region.x = regions[i].top;
		display_region.y = regions[i].left;
		display_region.width = regions[i].width;
		display_region.height = regions[i].height;
		update_data.update_marker = marker;
		marker++;
		if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
			update_mode, waveform_mode, temperature) != 0) {
			TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				display_region.x,
				display_region.y,
				display_region.width,
				display_region.height,
				panel_width,
				panel_height,
				update_mode,
				waveform_mode,
				temperature);
			return -1;
		}
		if (color > 15 || color < 0)
			TCON_ERR("color will be: 0 -> 15");
		
		hwtcon_fill_buffer_with_color(buffer,
			panel_width,
			panel_height,
			buffer_pitch,
			color,
			display_region);
		
		hwtcon_display_set_update_data(&update_data,
					display_region, update_mode, waveform_mode);
		if (dither_mode) {
			update_data.flags |= HWTCON_FLAG_USE_DITHERING_Y1 | HWTCON_FLAG_USE_DITHERING_Y4 | HWTCON_FLAG_FORCE_A2_OUTPUT;
			update_data.dither_mode = dither_mode;
		}		
		/* show the picture. */
		status = hwtcon_display_region(buffer,
			panel_width,
			panel_height,
			buffer_pitch,
			&update_data,
			temperature, true, false);
		if(status != 0)
			TCON_ERR("end status:%d", status);

	}	
	free(buffer);
	return status;

}

int read_file_to_buffer(char *buffer, int size, char *fileName)
{
	FILE *fp = fopen(fileName, "rb");
	size_t read_size = 0;
	if (fp == NULL) {
		TCON_ERR("open file:%s fail:%s", fileName, strerror(errno));
		return TCON_STATUS_OPEN_FILE_FAIL;
	}

	read_size = fread(buffer, 1, size, fp);
	if (read_size != size) {
		TCON_ERR("read file fail: size[%d] not match with read size[%d]",
			size, read_size);
		fclose(fp);
		return TCON_STATUS_READ_FILE_FAIL;
	}
	fclose(fp);
	return 0;
}

int hwtcon_test_check_input_valid(struct rect display_region, int panel_width, int panel_height,
	int update_mode, int waveform_mode, int temperature)
{
	if (display_region.x < 0 ||
		display_region.y < 0 ||
		display_region.width < 0 ||
		display_region.height < 0)
		return -1;
	if (display_region.x + display_region.width > panel_width)
		return -1;
	if (display_region.y + display_region.height > panel_height)
		return -1;
	if (update_mode != 0 && update_mode != 1)
		return -1;
	if (temperature < -20 || temperature > 60)
		return -1;
	return 0;
}

int display_clear(int argc, char *argv[])
{
	int status = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	char *buffer = NULL;
	int update_mode = UPDATE_MODE_FULL;
	int waveform_mode = 2;
	int temperature = 20;
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;

	buffer = (char *)calloc(1, buffer_pitch * panel_height);

	/* set the display region */
	display_region.x = 0;
	display_region.y = 0;
	display_region.width = panel_width;
	display_region.height = panel_height;

	/* fill buffer */
	memset(buffer, 0xF0, buffer_pitch * panel_height);

	hwtcon_display_set_update_data(&update_data,
			display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		&update_data,
		temperature, false, false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(buffer);
	return status;
}

int display_black(int argc, char *argv[])
{
	int status = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	char *buffer = NULL;
	int update_mode = UPDATE_MODE_FULL;
	int waveform_mode = 2;
	int temperature = 20;
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;

	buffer = (char *)calloc(1, buffer_pitch * panel_height);

	/* set the display region */
	display_region.x = 0;
	display_region.y = 0;
	display_region.width = panel_width;
	display_region.height = panel_height;

	hwtcon_fill_buffer_with_color(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		0,
		display_region);

	hwtcon_display_set_update_data(&update_data,
			display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		&update_data,
		temperature, false, false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(buffer);
	return status;
}


int display_color(int argc, char *argv[])
{
	int status = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	char *buffer = NULL;
	int update_mode = UPDATE_MODE_FULL;
	int waveform_mode = 2;
	int temperature = 20;
	int color = 8;
	unsigned char *ptr = NULL;
	unsigned int dither_mode = 0;	
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	if (argc == 9) {
		color = atoi(argv[1]);
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		display_region.width = atoi(argv[4]);
		display_region.height = atoi(argv[5]);
		update_mode = atoi(argv[6]);
		waveform_mode = atoi(argv[7]);
		temperature = atoi(argv[8]);
	}
	else if (argc == 2) {

		color = atoi(argv[1]);
		/* set the display region */
		display_region.x = 0;
		display_region.y = 0;
		display_region.width = panel_width;
		display_region.height = panel_height;
		update_mode = 1;
		waveform_mode = 2;
		temperature = 20;

	}
	else if (argc == 6) {
		color = atoi(argv[1]);
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		display_region.width = atoi(argv[4]);
		display_region.height = atoi(argv[5]);
		update_mode = 1;
		waveform_mode = 2;
		temperature = 20;
	}
	else if (argc == 10) {
		color = atoi(argv[1]);
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		display_region.width = atoi(argv[4]);
		display_region.height = atoi(argv[5]);
		update_mode = atoi(argv[6]);
		waveform_mode = atoi(argv[7]);
		temperature = atoi(argv[8]);
		dither_mode = strtol(argv[9], &ptr, 16);		
	}	
	else {
		TCON_ERR("invalid usage: ./display color color_value x y w h 1 2 29");
		TCON_ERR("invalid usage: ./display color color_value ");
		TCON_ERR("invalid usage: ./display color color_value x y w h");
		TCON_ERR("color: 0 -> 15");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;

	buffer = (char *)calloc(1, buffer_pitch * panel_height);
	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}
	if (color > 15 || color < 0)
		TCON_ERR("color will be: 0 -> 15");

	hwtcon_fill_buffer_with_color(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		color,
		display_region);
	hwtcon_display_set_update_data(&update_data,
			display_region, update_mode, waveform_mode);
	if (dither_mode) {
		update_data.flags |= HWTCON_FLAG_USE_DITHERING_Y1 | HWTCON_FLAG_USE_DITHERING_Y4 | HWTCON_FLAG_FORCE_A2_OUTPUT;
		update_data.dither_mode = dither_mode;
	}				
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		&update_data,
		temperature, false, false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(buffer);
	return status;
}

int hwtcon_test_performance(int argc, char *argv[])
{
	int i = 0;
	int param = 0;
	int status = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	char *buffer = NULL;
	char fileName[50] = {};
	int update_mode = 0;
	int waveform_mode = 0;
	int temperature = 0;
	int golden_frame_count = 0;
	struct hwtcon_update_data update_data;

	if (argc != 10) {
		TCON_ERR("invalid usage: update fileName x y widht height partial/full wf_mode temp golden_frame_count");
		return -1;
	}

	snprintf(fileName, sizeof(fileName), "%s", argv[1]);
	/* set the display region */
	display_region.x = atoi(argv[2]);
	display_region.y = atoi(argv[3]);
	display_region.width = atoi(argv[4]);
	display_region.height = atoi(argv[5]);
	update_mode = atoi(argv[6]);
	waveform_mode = atoi(argv[7]);
	temperature = atoi(argv[8]);
	golden_frame_count = atoi(argv[9]);

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}
	buffer = (char *)calloc(1, display_region.width * display_region.height);

	/* read file to buffer */
	status = read_file_to_buffer(buffer, display_region.width * display_region.height, fileName);
	if (status != 0)
		goto EXIT;

	hwtcon_display_get_eink_info()->record_time = true;
	hwtcon_display_get_eink_info()->target_time = golden_frame_count;

	hwtcon_display_set_update_data(&update_data,
		display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		display_region.width,
		display_region.height,
		display_region.width,
		&update_data,
		temperature, false, false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	hwtcon_display_get_eink_info()->record_time = false;
EXIT:
	free(buffer);
	return 0;
}


int hwtcon_test_print_resolution(int argc, char *argv[])
{
	int panel_width = 0;
	int panel_height = 0;
	int status = 0;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	TCON_ERR("panel res: %d %d", panel_width, panel_height);
	return 0;
}

int hwtcon_test_show_bin_file(int argc, char *argv[])
{
	int i = 0;
	int param = 0;
	int status = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	char *buffer = NULL;
	char fileName[50] = {};
	int update_mode = 0;
	int waveform_mode = 0;
	int temperature = 0;
	struct hwtcon_update_data update_data;

	if (argc < 9) {
		TCON_ERR("invalid usage: update fileName x y widht height partial/full wf_mode temp");
		return -1;
	}

	snprintf(fileName, sizeof(fileName), "%s", argv[1]);
	/* set the display region */
	display_region.x = atoi(argv[2]);
	display_region.y = atoi(argv[3]);
	display_region.width = atoi(argv[4]);
	display_region.height = atoi(argv[5]);
	update_mode = atoi(argv[6]);
	waveform_mode = atoi(argv[7]);
	temperature = atoi(argv[8]);

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;
	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}

	buffer = (char *)calloc(1, buffer_pitch * panel_height);

	/* read file to buffer */
	status = read_file_to_buffer(buffer, buffer_pitch * panel_height, fileName);
	if (status != 0)
		goto EXIT;

	hwtcon_display_set_update_data(&update_data,
		display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		panel_width,
		panel_height,
		buffer_pitch,
		&update_data,
		temperature,
		true,
		false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
EXIT:
	free(buffer);
	return status;
}

int hwtcon_test_show_bmp_file(int argc, char *argv[])
{
	struct rect display_region = {0};
	char *file_name = NULL;
	int pic_width = 0;
	int pic_height = 0;
	int panel_width = 0;
	int panel_height = 0;
	int status = 0;
	int update_mode = 0;
	int waveform_mode = 0;
	int temperature = 0;
	unsigned char *pYChannel = NULL;
	struct hwtcon_update_data update_data;

	if (argc == 5) {
		file_name = argv[1];
		display_region.x = 0;
		display_region.y = 0;
		update_mode = atoi(argv[2]);
		waveform_mode = atoi(argv[3]);
		temperature = atoi(argv[4]);
	} else if (argc == 7) {
		file_name = argv[1];
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		update_mode = atoi(argv[4]);
		waveform_mode = atoi(argv[5]);
		temperature = atoi(argv[6]);
	} else {
		TCON_ERR("invalid usage: ./display bmp file_name [x y] 1 2 29");
		return -1;
	}

	if (Read_BMP_Resolution(file_name, &pic_width, &pic_height) != 0) {
		TCON_ERR("read bmp file:%s fail", file_name);
		return -1;
	}
	display_region.width = pic_width;
	display_region.height = pic_height;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}

	TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				display_region.x,
				display_region.y,
				display_region.width,
				display_region.height,
				panel_width,
				panel_height,
				update_mode,
				waveform_mode,
				temperature);

	pYChannel =(unsigned char * )calloc(1, pic_width * pic_height);
	status = ReadBMP_Y(file_name, pYChannel);
	if(status != 0)
		TCON_ERR("ReadBMP_Y not right");
	hwtcon_display_set_update_data(&update_data,
			display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(pYChannel,
		pic_width,
		pic_height,
		pic_width,
		&update_data,
		temperature, false, false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(pYChannel);
	return 0;
}

int hwtcon_test_show_bmp_file_with_wait(int argc, char *argv[])
{
	struct rect display_region = {0};
	char *file_name = NULL;
	int pic_width = 0;
	int pic_height = 0;
	int panel_width = 0;
	int panel_height = 0;
	int status = 0;
	int update_mode = 0;
	int waveform_mode = 0;
	int temperature = 0;
	unsigned char *pYChannel = NULL;
	struct hwtcon_update_data update_data;

	if (argc == 9) {
		file_name = argv[1];
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		display_region.width = atoi(argv[4]);
		display_region.height = atoi(argv[5]);
		update_mode = atoi(argv[6]);
		waveform_mode = atoi(argv[7]);
		temperature = atoi(argv[8]);
	} else {
		TCON_ERR("invalid usage: ./display bmp file_name x y w h 1 2 29");
		return -1;
	}

	if (Read_BMP_Resolution(file_name, &pic_width, &pic_height) != 0) {
		TCON_ERR("read bmp file:%s fail", file_name);
		return -1;
	}

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}

	pYChannel = (unsigned char *)calloc(1, pic_width * pic_height);
	status = ReadBMP_Y(file_name, pYChannel);
	if(status != 0)
		TCON_ERR("ReadBMP_Y not right");

	hwtcon_display_set_update_data(&update_data,
		display_region, update_mode, waveform_mode);

	/* show the picture. */
	status = hwtcon_display_region(pYChannel,
		pic_width,
		pic_height,
		pic_width,
		&update_data,
		temperature,
		true,
		true);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(pYChannel);
	return 0;
}


int ReadBin_Y(const char *fileName, unsigned char *pInputY, int size)
{
	FILE *fp = fopen(fileName, "rb");
	if (fp == NULL) {
		TCON_ERR("open fileName:%s fail", fileName);
		return -1;
	}

	if (fread(pInputY, 1, size, fp)  != size) {
		TCON_ERR("read fileName:%s fail", fileName);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

int hwtcon_test_show_bmp_file_region(int argc, char *argv[])
{
	struct rect display_region = {0};
	char *file_name = NULL;
	int pic_width = 0;
	int pic_height = 0;
	int panel_width = 0;
	int panel_height = 0;
	int status = 0;
	int update_mode = 0;
	int waveform_mode = 0;
	int temperature = 0;
	unsigned char *pYChannel = NULL;
	struct hwtcon_update_data update_data;

	if (argc == 9) {
		file_name = argv[1];
		display_region.x = atoi(argv[2]);
		display_region.y = atoi(argv[3]);
		display_region.width = atoi(argv[4]);
		display_region.height = atoi(argv[5]);
		update_mode = atoi(argv[6]);
		waveform_mode = atoi(argv[7]);
		temperature = atoi(argv[8]);
	} else {
		TCON_ERR("invalid usage: ./display bmp file_name x y width height 1 2 29");
		return -1;
	}

	if (Read_BMP_Resolution(file_name, &pic_width, &pic_height) != 0) {
		TCON_ERR("read bmp file:%s fail", file_name);
		return -1;
	}
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	if (panel_width != pic_width ||
		panel_height != pic_height) {
		TCON_ERR("pic resolution[%d %d] not match panel[%d %d]",
			pic_width, pic_height, panel_width, panel_height);
		return -1;
	}


	if(hwtcon_test_check_input_valid(display_region, panel_width, panel_height,
		update_mode, waveform_mode, temperature) != 0) {
		TCON_ERR("invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			panel_width,
			panel_height,
			update_mode,
			waveform_mode,
			temperature);
		return -1;
	}

	TCON_ERR("region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				display_region.x,
				display_region.y,
				display_region.width,
				display_region.height,
				panel_width,
				panel_height,
				update_mode,
				waveform_mode,
				temperature);

	pYChannel = (unsigned char *)calloc(1, pic_width * pic_height);
	status = ReadBMP_Y(file_name, pYChannel);
	if(status != 0)
		TCON_ERR("ReadBMP_Y not right");
	hwtcon_display_set_update_data(&update_data,
		display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(pYChannel,
		pic_width,
		pic_height,
		pic_width,
		&update_data,
		temperature,
		false,
		false);
	if(status != 0)
		TCON_ERR("end status:%d", status);
	free(pYChannel);
	return status;
}


struct item_info {
	struct rect update_region;
	char *fileName;
	int update_mode;
	int waveform_mode;
	int temperature;
};

struct color_item_info {
	struct rect update_region;
	int update_mode;
	int waveform_mode;
	int temperature;
	int color;
};


int hwtcon_test_show_bmp_files(struct item_info *test_items, int count,
	bool wait_submission,
	bool wait_complete)
{
	int i = 0;
	int status = 0;
	unsigned char *readBuffer[100] = {NULL};
	int panel_width = 0;
	int panel_height = 0;
	int pic_width = 0;
	int pic_height = 0;
	unsigned int dither_mode = 0;	
	struct hwtcon_update_data update_data;

	/* acquire panel info */
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	/* prepare */
	for (i = 0; i < count; i++) {
		/* test picture resolution */
		if (Read_BMP_Resolution(test_items[i].fileName, &pic_width, &pic_height) != 0) {
			TCON_ERR("read bmp file:%s fail", test_items[i].fileName);
			goto FREE;
		}
		if (pic_width != panel_width ||
			pic_height != panel_height) {
			TCON_ERR("pic resolution[%d %d] not match panel[%d %d]",
				pic_width, pic_height, panel_width, panel_height);
			goto FREE;
		}

		if(hwtcon_test_check_input_valid(test_items[i].update_region,
			panel_width, panel_height,
			test_items[i].update_mode,
			test_items[i].waveform_mode,
			test_items[i].temperature) != 0) {
			TCON_ERR("testcase:%d invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				i,
				test_items[i].update_region.x,
				test_items[i].update_region.y,
				test_items[i].update_region.width,
				test_items[i].update_region.height,
				panel_width,
				panel_height,
				test_items[i].update_mode,
				test_items[i].waveform_mode,
				test_items[i].temperature);
			goto FREE;
		}

		/* malloc buffer to store test picture */
		readBuffer[i] = (unsigned char*)calloc(1, panel_width * panel_height);
		if (readBuffer[i] == NULL) {
			TCON_ERR("malloc buffer fail");
			goto FREE;
		}
		if (ReadBMP_Y(test_items[i].fileName, readBuffer[i]) != 0) {
			TCON_ERR("read bmp fail");
			goto FREE;
		}
	}

	/* show the picture. */
	for (i = 0; i < count; i++) {
		TCON_ERR("harry jiaguang trigger %d task begin", i);
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		if (dither_mode) {
			update_data.flags |= HWTCON_FLAG_USE_DITHERING_Y1 | HWTCON_FLAG_USE_DITHERING_Y4 | HWTCON_FLAG_FORCE_A2_OUTPUT;
			update_data.dither_mode = dither_mode;
		}
 
		status = hwtcon_display_region(readBuffer[i],
			pic_width,
			pic_height,
			pic_width,
			&update_data,
			test_items[i].temperature,
			wait_submission,
			wait_complete);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}else{
			TCON_ERR("harry jiaguang trigger %d task end", i);
		}
	}

FREE:
	for (i = 0; i < count; i++)
		if (readBuffer[i] != NULL)
			free(readBuffer[i]);
	return 0;
}

int hwtcon_test_show_bins(struct color_item_info *test_items, int count,
	bool wait_submission,
	bool wait_complete)
{
	int i = 0;
	int status = 0;
	unsigned char *readBuffer[100] = {NULL};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	struct hwtcon_update_data update_data;

	/* acquire panel info */
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;

	/* prepare */
	for (i = 0; i < count; i++) {
		if(hwtcon_test_check_input_valid(test_items[i].update_region,
			panel_width, panel_height,
			test_items[i].update_mode,
			test_items[i].waveform_mode,
			test_items[i].temperature) != 0) {
			TCON_ERR("testcase:%d invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				i,
				test_items[i].update_region.x,
				test_items[i].update_region.y,
				test_items[i].update_region.width,
				test_items[i].update_region.height,
				panel_width,
				panel_height,
				test_items[i].update_mode,
				test_items[i].waveform_mode,
				test_items[i].temperature);
			goto FREE;
		}
		/* malloc buffer to store test picture */
		readBuffer[i] = (unsigned char*)calloc(1, buffer_pitch * panel_height);
		if (readBuffer[i] == NULL) {
			TCON_ERR("malloc buffer fail");
			goto FREE;
		}
		hwtcon_fill_buffer_with_color(readBuffer[i],
			panel_width,
			panel_height,
			buffer_pitch,
			test_items[i].color,
			test_items[i].update_region);
	}

	/* show the picture. */
	for (i = 0; i < count; i++) {
		TCON_ERR("harry jiaguang trigger %d task begin", i);
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);

		status = hwtcon_display_region(readBuffer[i],
			panel_width,
			panel_height,
			buffer_pitch,
			&update_data,
			test_items[i].temperature,
			true,
			false);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}else{
		TCON_ERR("harry jiaguang trigger %d task end", i);
		}
	}

FREE:
	for (i = 0; i < count; i++)
		if (readBuffer[i] != NULL)
			free(readBuffer[i]);
	return 0;
}


int hwtcon_test_show_collision(struct color_item_info *test_items, int count,
	bool wait_submission,
	bool wait_complete)
{
	int i = 0;
	int status = 0;
	unsigned char *readBuffer[100] = {NULL};
	int panel_width = 0;
	int panel_height = 0;
	int buffer_pitch = 0;
	struct hwtcon_update_data update_data;

	/* acquire panel info */
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	buffer_pitch = (panel_width + 15) >> 4 << 4;

	/* prepare */
	for (i = 0; i < count; i++) {
		if(hwtcon_test_check_input_valid(test_items[i].update_region,
			panel_width, panel_height,
			test_items[i].update_mode,
			test_items[i].waveform_mode,
			test_items[i].temperature) != 0) {
			TCON_ERR("testcase:%d invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				i,
				test_items[i].update_region.x,
				test_items[i].update_region.y,
				test_items[i].update_region.width,
				test_items[i].update_region.height,
				panel_width,
				panel_height,
				test_items[i].update_mode,
				test_items[i].waveform_mode,
				test_items[i].temperature);
			goto FREE;
		}
		/* malloc buffer to store test picture */
		readBuffer[i] = (unsigned char*)calloc(1, buffer_pitch * panel_height);
		if (readBuffer[i] == NULL) {
			TCON_ERR("malloc buffer fail");
			goto FREE;
		}
		hwtcon_fill_buffer_with_color(readBuffer[i],
			panel_width,
			panel_height,
			buffer_pitch,
			test_items[i].color,
			test_items[i].update_region);
	}

	/* show the picture. */
	for (i = 0; i < count; i++) {
		TCON_ERR("harry jiaguang trigger %d task begin", i);
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);

		status = hwtcon_display_region(readBuffer[i],
			panel_width,
			panel_height,
			buffer_pitch,
			&update_data,
			test_items[i].temperature,
			true,
			false);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}else{
		TCON_ERR("harry jiaguang trigger %d task end", i);
		}
	}

FREE:
	for (i = 0; i < count; i++)
		if (readBuffer[i] != NULL)
			free(readBuffer[i]);
	return 0;
}

int hwtcon_test_color_collision(int argc, char *argv[])
{
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	int centrol_x = 0;
	int centrol_y = 0;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	centrol_x = panel_width >> 1;
	centrol_y = panel_height >> 1;

	status = display_clear(NULL,NULL);
	sleep(1);
	struct color_item_info test_items1[] = {
		{{centrol_x - 200, centrol_y - 200, 200, 200}, UPDATE_MODE_PARTIAL, 2, 28, 0},
		{{centrol_x - 60, centrol_y - 80, 200, 200}, UPDATE_MODE_PARTIAL, 2, 28, 6},
		{{centrol_x - 150, centrol_y - 150, 200, 200}, UPDATE_MODE_PARTIAL, 2, 28, 10},
	};
	status = hwtcon_test_show_collision(test_items1, sizeof(test_items1) / sizeof(test_items1[0]), true, false);
	sleep(1);
	status = display_clear(NULL,NULL);
	sleep(1);
	status = hwtcon_test_show_collision(test_items1, sizeof(test_items1) / sizeof(test_items1[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_back_to_back(int argc, char *argv[])
{
	int status = 0;
	unsigned int panel_width = 0;
	unsigned int panel_height = 0;
	unsigned int centrol_x = 0;
	unsigned int centrol_y = 0;
	unsigned int times = 0;
	unsigned int count = 100;
	unsigned int i = 0;
	struct color_item_info test_items1[12] = {0};
	struct color_item_info test_items2[12] = {0};


	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}
	TCON_ERR("w %d h %d", panel_width, panel_height);

	centrol_x = panel_width / 2;
	centrol_y = panel_height / 2;

	if (argc == 2) {
			count = atoi(argv[1]);

	}

	for(i = 0; i < 12; i = i + 1) {
		TCON_ERR("11, i : %d", i);
		test_items1[i].update_region.x = centrol_x - 270 + 50 * i;
		test_items1[i].update_region.y = centrol_y;
		test_items1[i].update_region.width = 40;
		test_items1[i].update_region.height = 20;
		test_items1[i].update_mode = UPDATE_MODE_PARTIAL;
		test_items1[i].color = 0;
		test_items1[i].waveform_mode = 2;
		test_items1[i].temperature = 25;

		test_items2[i].update_region.x = centrol_x + 280 - 50 * i;
		test_items2[i].update_region.y = centrol_y;
		test_items2[i].update_region.width = 40;
		test_items2[i].update_region.height = 20;
		test_items1[i].update_mode = UPDATE_MODE_PARTIAL;
		test_items2[i].color = 15;
		test_items2[i].waveform_mode = 2;
		test_items2[i].temperature = 25;
	}

	status = display_clear(NULL,NULL);
	sleep(1);

	for(times = 0; times < count; times++) {
		status = hwtcon_test_show_bins(test_items1, sizeof(test_items1) / sizeof(test_items1[0]), true, true);
		sleep(1);
		status = hwtcon_test_show_bins(test_items2, sizeof(test_items2) / sizeof(test_items2[0]), true, true);
		sleep(1);
	}
	#if 0
	status = ioctl(eink.fd, MXCFB_WAIT_FOR_ANY_UPDATE_COMPLETE, &update_marker);
	if (status != 0) {
		TCON_ERR("ioctl wait for marker:%d fail:%d", update_marker, status);
		return status;
	}
	#endif
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_quick_click(int argc, char *argv[])
{
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	int centrol_x = 0;
	int centrol_y = 0;
	int times = 0;
	int count = 3;
	int i = 0;
	unsigned char *readBuffer[3] = {NULL};
	int buffer_pitch = 0;
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);

	centrol_x = panel_width >> 1;
	centrol_y = panel_height >> 1;
	buffer_pitch = (panel_width + 15) >> 4 << 4;

	TCON_ERR("W:%d H:%d pith:%d", panel_width, panel_height, buffer_pitch);

	struct color_item_info test_items1[] = {
		{{centrol_x - 200, centrol_y - 200, 400, 400}, UPDATE_MODE_PARTIAL, 2, 28, 0},
		{{centrol_x - 150, centrol_y - 150, 300, 300}, UPDATE_MODE_PARTIAL, 2, 28, 15},
		{{centrol_x - 70, centrol_y - 70, 140, 140}, UPDATE_MODE_PARTIAL, 2, 28, 8},
	};

	status = display_clear(NULL,NULL);
	sleep(2);
			/* malloc buffer to store test picture */
	readBuffer[0] = (unsigned char*)calloc(1, buffer_pitch * panel_height);
	if (readBuffer[0] == NULL) {
		TCON_ERR("malloc buffer fail");
		goto FREE;
	}
	hwtcon_fill_buffer_with_color(readBuffer[0],
		panel_width,
		panel_height,
		buffer_pitch,
		test_items1[0].color,
		test_items1[0].update_region);

	readBuffer[1] = (unsigned char*)calloc(1, buffer_pitch * panel_height);
	if (readBuffer[1] == NULL) {
		TCON_ERR("malloc buffer fail");
		goto FREE;
	}
	hwtcon_fill_buffer_with_color(readBuffer[1],
		panel_width,
		panel_height,
		buffer_pitch,
		test_items1[1].color,
		test_items1[1].update_region);
	hwtcon_fill_buffer_with_color(readBuffer[1],
		panel_width,
		panel_height,
		buffer_pitch,
		test_items1[2].color,
		test_items1[2].update_region);

	/* show the picture. */
	for (i = 0; i < 2; i++) {
		TCON_ERR("harry jiaguang trigger %d task begin", i);
		hwtcon_display_set_update_data(&update_data,
			test_items1[i].update_region,
			test_items1[i].update_mode,
			test_items1[i].waveform_mode);
		status = hwtcon_display_region(readBuffer[i],
			panel_width,
			panel_height,
			buffer_pitch,
			&update_data,
			test_items1[i].temperature,
			true,
			false);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}else{
		TCON_ERR("harry jiaguang trigger %d task end", i);
		}
	}

FREE:
	for (i = 0; i < 3; i++)
		if (readBuffer[i] != NULL)
			free(readBuffer[i]);
	return 0;

}


int hwtcon_test_show_bin_files(struct item_info *test_items, int count)
{
	int i = 0;
	int status = 0;
	unsigned char *readBuffer[100] = {NULL};
	int panel_width = 0;
	int panel_height = 0;
	struct hwtcon_update_data update_data;

	/* acquire panel info */
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	/* prepare */
	for (i = 0; i < count; i++) {
		if(hwtcon_test_check_input_valid(test_items[i].update_region,
			panel_width, panel_height,
			test_items[i].update_mode,
			test_items[i].waveform_mode,
			test_items[i].temperature) != 0) {
			TCON_ERR("testcase:%d invalid param: region[%d %d %d %d] panel[%d %d] update_mode:%d, wf_mode:%d, temp:%d\n",
				i,
				test_items[i].update_region.x,
				test_items[i].update_region.y,
				test_items[i].update_region.width,
				test_items[i].update_region.height,
				panel_width,
				panel_height,
				test_items[i].update_mode,
				test_items[i].waveform_mode,
				test_items[i].temperature);
			goto FREE;
		}

		/* malloc buffer to store test picture */
		readBuffer[i] = (unsigned char*)calloc(1, panel_width * panel_height);
		if (readBuffer[i] == NULL) {
			TCON_ERR("malloc buffer fail");
			goto FREE;
		}

		if (ReadBin_Y(test_items[i].fileName, readBuffer[i], panel_width * panel_height) != 0)
			goto FREE;

	}

	/* show the picture. */
	for (i = 0; i < count; i++) {
		TCON_ERR("harry jiaguang trigger %d task begin", i);
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		status = hwtcon_display_region(readBuffer[i],
			panel_width,
			panel_height,
			panel_width,
			&update_data,
			test_items[i].temperature,
			false,
			false);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}else{
		TCON_ERR("harry jiaguang trigger %d task end", i);
		}
	}

FREE:
	for (i = 0; i < count; i++)
		if (readBuffer[i] != NULL)
			free(readBuffer[i]);
	return 0;
}

int hwtcon_test_full_partial_update_same_time(int argc, char * argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 200, 200}, "/data/normal_update/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{200, 200, 200, 200}, "/data/normal_update/02.bmp", UPDATE_MODE_PARTIAL, 2, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;

}

int hwtcon_test_bmp_collision(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 600, 400}, "/data/02.bmp", UPDATE_MODE_FULL, 2, 28},
		{{100, 100, 200, 200}, "/data/04.bmp", UPDATE_MODE_FULL, 2, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;

}

int hwtcon_test_mdp_merge(int argc, char *argv[])
{
	int status = 0;
		struct item_info test_items[] = {
		{{0, 0, 600, 400}, "/data/normal_update/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{0, 0, 300, 300}, "/data/normal_update/02.bmp", UPDATE_MODE_FULL, 2, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;

}

int hwtcon_test_col_merge(int argc, char *argv[])
{
int status = 0;
		struct item_info test_items[] = {
		{{0, 0, 600, 400}, "/data/normal_update/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{0, 0, 300, 300}, "/data/normal_update/02.bmp", UPDATE_MODE_FULL, 2, 28},
		{{200, 200, 200, 200}, "/data/normal_update/03.bmp", UPDATE_MODE_FULL, 3, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;

}


int hwtcon_test_update_order(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 1264, 1680}, "/data/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{0, 0, 500, 500}, "/data/02.bmp", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 500, 500}, "/data/03.bmp", UPDATE_MODE_FULL, 3, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_collision(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 800, 800}, "/tmp/collision/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 500, 500}, "/tmp/collision/02.bmp", UPDATE_MODE_FULL, 2, 28},
	};

	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;

}

void fill_buffer(char *buffer, int buffer_width, int buffer_height,
	int x, int y, int width, int height, int value)
{
	int i, j;

	if ((x + width > buffer_width) ||
		(y + height > buffer_height)) {
		TCON_ERR("invalid region: [%d %d %d %d]", x, y, width, height);
		return;
	}

	for (i = x; i < x + width; i++)
		for (j = y; j < y + height; j++)
			buffer[i + j * buffer_width] = value;
}

int hwtcon_test_full_collision_with_full_update(int argc, char *argv[])
{
	int i;
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	bool wait_submission = true;
	bool wait_complete = false;
	struct item_info test_items[] = {
		{{0, 0, 800, 800}, "/no_use", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 300, 300}, "/no_use", UPDATE_MODE_FULL, 2, 28},
	};
	const int item_count = sizeof(test_items) / sizeof(struct item_info);
	char *display_buffer[item_count];
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	for (i = 0; i < item_count; i++) {
		display_buffer[i] = (char *)calloc(1, panel_width * panel_height);
		memset(display_buffer[i], 0xF0, panel_width * panel_height);
	}

	fill_buffer(display_buffer[0], panel_width, panel_height, 0, 0, 800, 800, 0x60);
	fill_buffer(display_buffer[1], panel_width, panel_height, 500, 500, 300, 300, 0x30);

	/* display */
	for (i = 0; i < item_count; i++) {
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		status = hwtcon_display_region(display_buffer[i],
				panel_width,
				panel_height,
				panel_width,
				&update_data,
				test_items[i].temperature,
				wait_submission,
				wait_complete);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}
	}
	return 0;
}

int hwtcon_test_full_collision_with_partial_update(int argc, char *argv[])
{
	int i;
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	bool wait_submission = true;
	bool wait_complete = false;
	struct item_info test_items[] = {
		{{0, 0, 800, 800}, "/no_use", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 300, 300}, "/no_use", UPDATE_MODE_PARTIAL, 2, 28},
	};
	const int item_count = sizeof(test_items) / sizeof(struct item_info);
	char *display_buffer[item_count];
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	for (i = 0; i < item_count; i++) {
		display_buffer[i] = (char *)calloc(1, panel_width * panel_height);
		memset(display_buffer[i], 0xF0, panel_width * panel_height);
	}

	fill_buffer(display_buffer[0], panel_width, panel_height, 0, 0, 800, 800, 0x60);
	fill_buffer(display_buffer[1], panel_width, panel_height, 500, 500, 300, 300, 0x30);

	/* display */
	for (i = 0; i < item_count; i++) {
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		status = hwtcon_display_region(display_buffer[i],
				panel_width,
				panel_height,
				panel_width,
				&update_data,
				test_items[i].temperature,
				wait_submission,
				wait_complete);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}
	}
	return 0;
}

int hwtcon_test_no_collision_with_partial_update(int argc, char *argv[])
{
	int i;
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	bool wait_submission = true;
	bool wait_complete = false;
	struct item_info test_items[] = {
		{{0, 0, 800, 800}, "/no_use", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 300, 300}, "/no_use", UPDATE_MODE_PARTIAL, 2, 28},
	};
	const int item_count = sizeof(test_items) / sizeof(struct item_info);
	char *display_buffer[item_count];
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}

	for (i = 0; i < item_count; i++) {
		display_buffer[i] = (char *)calloc(1, panel_width * panel_height);
		memset(display_buffer[i], 0xF0, panel_width * panel_height);
	}

	fill_buffer(display_buffer[0], panel_width, panel_height, 0, 0, 800, 800, 0x60);
	fill_buffer(display_buffer[1], panel_width, panel_height, 500, 500, 300, 300, 0x60);

	/* display */
	for (i = 0; i < item_count; i++) {
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		status = hwtcon_display_region(display_buffer[i],
				panel_width,
				panel_height,
				panel_width,
				&update_data,
				test_items[i].temperature,
				wait_submission,
				wait_complete);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}
	}
	return 0;
}

int hwtcon_test_collision1(int argc, char *argv[])
{
	int i;
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	bool wait_submission = true;
	bool wait_complete = false;
	struct item_info test_items[] = {
		{{0, 0, 800, 800}, "/no_use", UPDATE_MODE_FULL, 2, 28},
		{{500, 500, 800, 500}, "/no_use", UPDATE_MODE_PARTIAL, 2, 28},
	};
	const int item_count = sizeof(test_items) / sizeof(struct item_info);
	char *display_buffer[item_count];
	struct hwtcon_update_data update_data;
	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}
	for (i = 0; i < item_count; i++) {
		display_buffer[i] = (char *)calloc(1, panel_width * panel_height);
		memset(display_buffer[i], 0xF0, panel_width * panel_height);
	}

	fill_buffer(display_buffer[0], panel_width, panel_height, 0, 0, 800, 800, 0x60);
	fill_buffer(display_buffer[1], panel_width, panel_height, 500, 500, 800, 500, 0x30);
	fill_buffer(display_buffer[1], panel_width, panel_height, 500, 500, 300, 300, 0x60);

	/* display */
	for (i = 0; i < item_count; i++) {
		hwtcon_display_set_update_data(&update_data,
			test_items[i].update_region,
			test_items[i].update_mode,
			test_items[i].waveform_mode);
		status = hwtcon_display_region(display_buffer[i],
				panel_width,
				panel_height,
				panel_width,
				&update_data,
				test_items[i].temperature,
				wait_submission,
				wait_complete);
		if(status != 0){
			TCON_ERR("display region err");
			return -1;
		}
	}
	return status;
}


int hwtcon_test_bin_auto_waveform(int argc, char *argv[])
{	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 240, 160}, "/data/auto_test/img1.bin", UPDATE_MODE_FULL, 20, 28},
		{{0, 0, 360, 240}, "/data/auto_test/img2.bin", UPDATE_MODE_FULL, 20, 28},
		{{0, 0, 360, 240}, "/data/auto_test/img3.bin", UPDATE_MODE_FULL, 20, 28},
		{{0, 0, 480, 320}, "/data/auto_test/img4.bin", UPDATE_MODE_FULL, 20, 28},
		{{0, 0, 600, 400}, "/data/auto_test/img5.bin", UPDATE_MODE_FULL, 20, 28},
	};
	status = hwtcon_test_show_bin_files(test_items, sizeof(test_items) / sizeof(test_items[0]));
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_bin_collision_1(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 360, 240}, "/tmp/collision/01.bmp", UPDATE_MODE_PARTIAL, 2, 28},
		{{120, 80, 360, 240}, "/tmp/collision/02.bmp", UPDATE_MODE_PARTIAL, 2, 28},
	};
	status = hwtcon_test_show_bmp_files(test_items, sizeof(test_items) / sizeof(test_items[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_bin_collision_2(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 360, 240}, "/data/collision_2/img1.bin", UPDATE_MODE_PARTIAL, 2, 28},
		{{120, 80, 360, 240}, "/data/collision_2/img2.bin", UPDATE_MODE_FULL, 2, 28},
	};

	status = hwtcon_test_show_bin_files(test_items, sizeof(test_items) / sizeof(test_items[0]));
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_bin_collision_3(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 360, 240}, "/data/collision_3/img1.bin", UPDATE_MODE_PARTIAL, 2, 28},
		{{120, 80, 240, 160}, "/data/collision_3/img2.bin", UPDATE_MODE_PARTIAL, 2, 28},
	};

	status = hwtcon_test_show_bin_files(test_items, sizeof(test_items) / sizeof(test_items[0]));
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_bin_collision_4(int argc, char *argv[])
{
	int status = 0;
	int panel_width = 0;
	int panel_height = 0;
	int centrol_x = 0;
	int centrol_y = 0;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	centrol_x = panel_width >> 1;
	centrol_y = panel_height >> 1;

	status = display_clear(NULL,NULL);
	sleep(1);
	struct item_info test_items1[] = {
		{{centrol_x - 200, centrol_y - 200, 200, 200}, "/tmp/collision/01.bmp", UPDATE_MODE_PARTIAL, 2, 28},
		{{centrol_x - 60, centrol_y - 80, 200, 200}, "/tmp/collision/02.bmp", UPDATE_MODE_PARTIAL, 2, 28},
		{{centrol_x - 150, centrol_y - 150, 200, 200}, "/tmp/collision/03.bmp", UPDATE_MODE_PARTIAL, 2, 28},
	};
	status = hwtcon_test_show_bmp_files(test_items1, sizeof(test_items1) / sizeof(test_items1[0]), true, false);
	sleep(2);
	status = display_clear(NULL,NULL);
	sleep(1);
	struct item_info test_items2[] = {
		{{centrol_x - 200, centrol_y - 200, 200, 200}, "/tmp/collision/01.bmp", UPDATE_MODE_FULL, 2, 28},
		{{centrol_x - 60, centrol_y - 80, 200, 200}, "/tmp/collision/02.bmp", UPDATE_MODE_FULL, 2, 28},
		{{centrol_x - 150, centrol_y - 150, 200, 200}, "/tmp/collision/03.bmp", UPDATE_MODE_FULL, 2, 28},
	};
	status = hwtcon_test_show_bmp_files(test_items2, sizeof(test_items2) / sizeof(test_items2[0]), true, false);
	TCON_ERR("end status:%d", status);
	return status;
}

int hwtcon_test_bin_collision_5(int argc, char *argv[])
{
	int status = 0;
	struct item_info test_items[] = {
		{{0, 0, 240, 160}, "/data/collision_5/img1.bin", UPDATE_MODE_PARTIAL, 2, 28},
		{{240, 160, 240, 80}, "/data/collision_5/img2.bin", UPDATE_MODE_PARTIAL, 2, 28},
		{{240, 240, 120, 160}, "/data/collision_5/img3.bin", UPDATE_MODE_PARTIAL, 2, 28},
		{{120, 80, 360, 240}, "/data/collision_5/img4.bin", UPDATE_MODE_FULL, 2, 28},
	};

	status = hwtcon_test_show_bin_files(test_items, sizeof(test_items) / sizeof(test_items[0]));
	TCON_ERR("end status:%d", status);
	return status;
}

#if 0
#include "cmap.h"
int hwtcon_test_cmap(int argc, char *argv[])
{
	int status = ioctl(hwtcon_display_get_eink_info()->fd, FBIOPUTCMAP, &einkfb_8bpp_cmap);

	TCON_ERR("set cmap %d", status);

	return 0;
}
#endif

int hwtcon_test_rotate(int argc, char *argv[])
{
	int status = 0;
	int rotate = 0;

	if (argc != 2) {
		TCON_ERR("invalid usage: display rotate 1");
		return 0;
	}
	rotate = atoi(argv[1]);
	status = hwtcon_display_set_rotation(rotate);
	TCON_ERR("end status:%d", status);
	return status;
}


int display_graylevel(int argc, char *argv[])
{
	int i = 0;
	int param = 0;
	int status = 0;
	int grayblocknum = 0;
	struct rect display_region = {0};
	int panel_width = 0;
	int panel_height = 0;
	char *buffer = NULL;
	char fileName[50] = {};
	int update_mode = UPDATE_MODE_FULL;
	int waveform_mode = 0;
	int temperature = 0;
	struct hwtcon_update_data update_data;

	status = hwtcon_get_panel_info(&panel_width, &panel_height);
	if(status != 0){
		TCON_ERR("get panel info err");
		return -1;
	}
	buffer = (char *)calloc(1, panel_width * panel_height);
	/* set the display region */
	display_region.x = 0;
	display_region.y = 0;
	display_region.width = panel_width;
	display_region.height = panel_height;
	update_mode = UPDATE_MODE_FULL;
	waveform_mode = 2;
	temperature = 20;
	grayblocknum = panel_width * panel_height / 16;
	/* fill buffer */
	for(i = 0; i < 16; i++) {
		memset(buffer+grayblocknum * i, 16 * i, grayblocknum);
	}

	hwtcon_display_set_update_data(&update_data,
		display_region, update_mode, waveform_mode);
	/* show the picture. */
	status = hwtcon_display_region(buffer,
		panel_width,
		panel_height,
		panel_width,
		&update_data,
		temperature, false, false);
	if(status != 0)
			TCON_ERR("end status:%d", status);
	free(buffer);
	return status;
}

int hwtcon_test_night_mode(int argc, char *argv[])
{
	int status = 0;
	int night = 0;

	if (argc != 2) {
		TCON_ERR("invalid usage: display night 1");
		return 0;
	}
	night = atoi(argv[1]);
	status = hwtcon_display_set_night_mode(night);
	TCON_ERR("end status:%d", status);
	return status;
}

typedef int (*ITEM_CALLBACK)(int argc, char *argv[]);
struct item_callback {
	char *name;
	ITEM_CALLBACK func;
} items[] = {
		{"perf", hwtcon_test_performance},
		{"info", hwtcon_test_print_resolution},
		{"bin", hwtcon_test_show_bin_file},
		{"bmp1", hwtcon_test_show_bmp_file},
		{"bmp", hwtcon_test_show_bmp_file_region},
		{"full_partial_update", hwtcon_test_full_partial_update_same_time},
		{"bmp_col", hwtcon_test_bmp_collision},
		{"bin_col_1", hwtcon_test_bin_collision_1},
		{"bin_col_2", hwtcon_test_bin_collision_2},
		{"bin_col_3", hwtcon_test_bin_collision_3},
		{"bmp_collision", hwtcon_test_bin_collision_4},
		{"bin_col_5", hwtcon_test_bin_collision_5},
		{"marker_test", hwtcon_test_show_bmp_file_with_wait},
		{"auto_test", hwtcon_test_bin_auto_waveform},
		{"mdp_merge", hwtcon_test_mdp_merge},
		{"col_merge", hwtcon_test_col_merge},
		{"update_order", hwtcon_test_update_order},
		{"col", hwtcon_test_collision},
		{"col1", hwtcon_test_collision1},
		{"full_col_full_update", hwtcon_test_full_collision_with_full_update},
		{"full_col_partial_update", hwtcon_test_full_collision_with_partial_update},
		{"col_partial", hwtcon_test_no_collision_with_partial_update},
		{"rotate", hwtcon_test_rotate},
		{"clear", display_clear},
		{"gray", display_graylevel},
		{"black", display_black},
		{"color", display_color},
		{"collision", hwtcon_test_color_collision},
		{"slide_round", hwtcon_test_back_to_back},
		{"quick_click", hwtcon_test_quick_click},
		{"night", hwtcon_test_night_mode},
		{"hand", hwtcon_test_hand_write},
};

int main(int argc, char *argv[])
{
	int i = 0;
	if (argc <= 1) {
		TCON_ERR("invalid usage: ./display item_name [params]");
		return -1;
	}
	if(hwtcon_open_device() != 0) {
		TCON_ERR("open device fail");
		return -1;
	}

	for (i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
		if ((strncmp(argv[1], items[i].name, strlen(items[i].name)) == 0) &&
			(strlen(items[i].name) == strlen(argv[1]))) {
			/* find the execute item */
			items[i].func(argc - 1, &argv[1]);
			break;
		}
	}
	if (i == sizeof(items) / sizeof(items[0]))
		TCON_ERR("invalid item:%s", argv[1]);
	hwtcon_close_device();
	return 0;
}
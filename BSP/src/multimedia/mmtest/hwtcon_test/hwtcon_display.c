// SPDX-License-Identifier: MediaTekProprietary
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "hwtcon_display.h"
#include "hwtcon_ioctl_cmd.h"

#define WAIT_IMG_BUF_IDLE_COUNT 10
#define EINK_FRAME_BUFFER "/dev/fb0"

struct eink_t eink;
struct fb_var_screeninfo vsInfo;

HWTCON_TIME hwtcon_display_get_time(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (HWTCON_TIME)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

HWTCON_TIME hwtcon_display_get_time_duration_ms(HWTCON_TIME start,
	HWTCON_TIME end)
{
	return end - start;
}

struct eink_t *hwtcon_display_get_eink_info(void)
{
	return &eink;
}

int hwtcon_display_set_rotation(int rotate)
{
	int status = 0;

	/* for rotate feature */
	printf("harry set rotate %d\n", rotate);
	vsInfo.rotate = rotate;
	status = ioctl(eink.fd, FBIOPUT_VSCREENINFO, &vsInfo);
	if (status != 0) {
		TCON_ERR("ioctl FBIOPUT_VSCREENINFO fail", status);
		return status;
	}
	return 0;
}


int hwtcon_open_device(void)
{
	int result = 0;
	int status = 0;

	/* already opend */
	if (eink.fd > 0)
		return 0;

	eink.fd = open(EINK_FRAME_BUFFER, O_RDWR);
	if (eink.fd < 0) {
		TCON_ERR("cannot open framebuffer device \"" EINK_FRAME_BUFFER "\" err:%s",
			strerror(errno));
		hwtcon_close_device();
		return TCON_STATUS_OPEN_FD_FAIL;
	}

	if (ioctl(eink.fd, FBIOGET_VSCREENINFO, &vsInfo) != 0) {
		TCON_ERR("cannot query framebuffer geometry");
		hwtcon_close_device();
		return TCON_STATUS_QUERY_FB_FAIL;
	}

#if 0
	/* for rotate feature */
	printf("harry set rotate 111\n");
	vsInfo.rotate = 3;
	status = ioctl(eink.fd, FBIOPUT_VSCREENINFO, &vsInfo);
	if (status != 0) {
		TCON_ERR("ioctl FBIOPUT_VSCREENINFO fail", status);
		return status;
	}
#endif


	// calculate the framebuffer size
	eink.size = vsInfo.xres_virtual * vsInfo.yres* vsInfo.bits_per_pixel / 8;

	eink.size_buf_kernel = vsInfo.xres_virtual * vsInfo.yres_virtual* vsInfo.bits_per_pixel / 8;

	// map the framebuffer into user space
	eink.buffer = mmap(NULL, eink.size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, eink.fd, 0);
	eink.buf_kernel = mmap(NULL, eink.size_buf_kernel, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, eink.fd, 0);

	if (eink.buffer == NULL || eink.buf_kernel == NULL) {
		TCON_ERR("cannot map framebuffer, buffer[%p] buf_kernel[%p]",
			eink.buffer,
			eink.buf_kernel);
		hwtcon_close_device();
		return TCON_STATUS_MAP_KERNEL_BUF_FAIL;
	}

	// calculate the number of bytes per row
	eink.row_bytes = vsInfo.xres_virtual * vsInfo.bits_per_pixel / 8;

	// create the screen image
	eink.screen_image.xres  = vsInfo.xres;
	eink.screen_image.xlen  = vsInfo.xres_virtual;
	eink.screen_image.yres  = vsInfo.yres;
	eink.screen_image.bpp   = vsInfo.bits_per_pixel;
	eink.screen_image.start = eink.buffer;

	#if 0
	TCON_LOG("eink info -> row_bytes[%d] fd[%d] buffer[%p] size[%d] buf_kernel[%p] kernel_size[%d]",
		eink.row_bytes,
		eink.fd,
		eink.buffer,
		eink.size,
		eink.buf_kernel,
		eink.size_buf_kernel);
	TCON_LOG("eink image info -> xres[%d] xlen[%d] yes[%d] bpp[%p] start[%p]",
		eink.screen_image.xres,
		eink.screen_image.xlen,
		eink.screen_image.yres,
		eink.screen_image.bpp,
		eink.screen_image.start);
	#endif

	return 0;
}

void hwtcon_close_device(void)
{
	if (eink.fd > 0) {
		close(eink.fd);
		eink.fd = -1;
		munmap(eink.buffer, eink.size);
		munmap(eink.buf_kernel, eink.size_buf_kernel);
		eink.buffer = NULL;
		eink.buf_kernel = NULL;
		}
}

int hwtcon_get_panel_info(int *panel_width, int *panel_height)
{
	hwtcon_open_device();
	if (eink.fd <= 0) {
		TCON_ERR("open hwtcon device fail");
		return -1;
	}

	if (panel_width)
		*panel_width = eink.screen_image.xres;
	if (panel_height)
		*panel_height = eink.screen_image.yres;

	TCON_ERR("xres_virtual:%d,yres_virtual:%d ", vsInfo.xres_virtual, vsInfo.yres_virtual);


	return 0;
}

/* copy whole buffer to image buffer. */
static int hwtcon_copy_buffer_to_img_buffer(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct rect display_region)
{
	int wait_count = 0;
	int i, j;

	/* check display_region */
	if (display_region.x + display_region.width > eink.screen_image.xres ||
		display_region.y + display_region.height > eink.screen_image.yres) {
		TCON_ERR("invalid display_region:[%d %d %d %d] screen[w:%d h:%d]",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			eink.screen_image.xres,
			eink.screen_image.yres);
		return TCON_STATUS_INVALID_DISP_REGION;
	}

	/* copy buffer to img buffer:eink.buffer */
	for (i = display_region.x; i < display_region.x + display_region.width; i++)
		for (j = display_region.y; j < display_region.y + display_region.height; j++)
			eink.buffer[eink.screen_image.xres * j + i] = 
				buffer[buffer_pitch * (j - display_region.y) + (i - display_region.x)];

	return 0;
}

/* copy buffer to image buffer. */
int hwtcon_fill_buffer_with_color(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	int buffer_color,
	struct rect display_region)
{
	int i, j;

	/* check display_region */
	if (display_region.x + display_region.width > eink.screen_image.xres ||
		display_region.y + display_region.height > eink.screen_image.yres ||
		buffer_color < 0 || buffer_color > 15
	) {
		TCON_ERR("invalid display_region:[%d %d %d %d] screen[w:%d h:%d] color[%d]",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			eink.screen_image.xres,
			eink.screen_image.yres,
			buffer_color);
		return TCON_STATUS_INVALID_DISP_REGION;
	}

	/* copy buffer to img buffer:eink.buffer */
	for (i = display_region.x; i < display_region.x + display_region.width; i++)
		for (j = display_region.y; j < display_region.y + display_region.height; j++) {
			buffer[buffer_pitch * j + i] =
				buffer_color << 4;
		}


	return 0;
}

/* copy buffer to image buffer. */
static int hwtcon_copy_screen_buffer_to_alt_img_buffer(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct rect display_region)
{
	int wait_count = 0;
	int i, j;
	FILE *fp2;
	/* check display_region */
	if (display_region.x + display_region.width > eink.screen_image.xres ||
		display_region.y + display_region.height > eink.screen_image.yres) {
		TCON_ERR("invalid display_region:[%d %d %d %d] screen[w:%d h:%d]",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			eink.screen_image.xres,
			eink.screen_image.yres);
		return TCON_STATUS_INVALID_DISP_REGION;
	}
	/* copy buffer to img buffer:eink.buffer */
	for (i = display_region.x; i < display_region.x + display_region.width; i++)
		for (j = display_region.y; j < display_region.y + display_region.height; j++) {
			eink.buffer[vsInfo.xres_virtual * j + i + vsInfo.xres_virtual * vsInfo.yres] =
				buffer[buffer_pitch * j + i];
		}
	if((fp2 = fopen("/tmp/color_in.bin","wb")) == NULL) {
		printf("Open file color_in fail.\n");
		return -1;
	}
	fwrite(eink.buffer + buffer_pitch * buffer_height, buffer_pitch * buffer_height, 1, fp2);
	return 0;
}


/* copy buffer to image buffer. */
static int hwtcon_copy_screen_buffer_to_img_buffer(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct rect display_region)
{
	int wait_count = 0;
	int i, j;

	/* check display_region */
	if (display_region.x + display_region.width > eink.screen_image.xres ||
		display_region.y + display_region.height > eink.screen_image.yres) {
		TCON_ERR("invalid display_region:[%d %d %d %d] screen[w:%d h:%d]",
			display_region.x,
			display_region.y,
			display_region.width,
			display_region.height,
			eink.screen_image.xres,
			eink.screen_image.yres);
		return TCON_STATUS_INVALID_DISP_REGION;
	}

	/* copy buffer to img buffer:eink.buffer */
	for (i = display_region.x; i < display_region.x + display_region.width; i++)
		for (j = display_region.y; j < display_region.y + display_region.height; j++) {
			eink.buffer[vsInfo.xres_virtual * j + i] = 
				buffer[buffer_pitch * j + i];
		}
	return 0;
}

static unsigned int update_flag;

void hwtcon_display_set_flag(unsigned int flag)
{
	update_flag = flag;
	TCON_ERR("set flag to 0x%08x", update_flag);
}

static int hwtcon_display_to_eink(
	struct hwtcon_update_data *update_data,
	int temperature,
	u32 *update_marker)
{
	int i, j = 0;
	int status = 0;
	static u32 unique_id;
	HWTCON_TIME time_start = 0LL;
	HWTCON_TIME time_end = 0LL;
	HWTCON_TIME time_duration = 0LL;

	status = ioctl(eink.fd, HWTCON_SET_TEMPERATURE, &temperature);
	if (status != 0) {
		TCON_ERR("ioctl set temperature fail", status);
		return status;
	}

	if (update_data->waveform_mode < 0 || update_data->waveform_mode >= 12)
		update_data->waveform_mode = 257;	/* auto waveform mode */
	update_data->update_marker = unique_id++;
	*update_marker = update_data->update_marker;


	TCON_ERR("update  region[%d %d %d %d] %s wf_mode[%d] flags[0x%08x]",
		update_data->update_region.left,
		update_data->update_region.top,
		update_data->update_region.width,
		update_data->update_region.height,
		(update_data->update_mode == UPDATE_MODE_FULL) ?
			"full update" : "partial update",
		update_data->waveform_mode,
		update_data->flags);

	#if 0
	if (eink.record_time)
		time_start = hwtcon_display_get_time();
	#endif

	status = ioctl(eink.fd, HWTCON_SEND_UPDATE, update_data);

	if (status != 0) {
		TCON_ERR("ioctl send update data fail", status);
		return status;
	}

	#if 0
	if (eink.record_time) {
		struct hwtcon_update_marker_data marker;

		marker.update_marker = upd_data.update_marker;
		status = ioctl(eink.fd, HWTCON_WAIT_FOR_UPDATE_COMPLETE, &marker);
		if (status != 0) {
			TCON_ERR("wait frame done error");
			return status;
		}
		time_end = hwtcon_display_get_time();
		time_duration = hwtcon_display_get_time_duration_ms(time_start, time_end);
		if (fabs(time_duration - eink.target_time * 11.67) < 30) {
			TCON_ERR("compare pass:  duration:%lld target:%f",
				time_duration, eink.target_time * 11.67);
		} else {
			TCON_ERR("compare fail: duration:%lld target:%f",
				time_duration, eink.target_time * 11.67);
		}
	}
	#endif

	return 0;
}

char *hwtcon_display_get_img_buffer(void)
{
	int status = hwtcon_open_device();
	if (status != 0) {
		TCON_ERR("open device fail:%d", status);
		return NULL;
	}

	TCON_ERR("read eink.buffer:0x%p", eink.buffer);

	return eink.buffer;
}

void hwtcon_display_set_update_data(struct hwtcon_update_data *update_data,
	struct rect region,
	int update_mode,
	int waveform_mode)
{
	memset(update_data, 0, sizeof(struct hwtcon_update_data));

	update_data->update_region.left = region.x;
	update_data->update_region.top = region.y;
	update_data->update_region.width = region.width;
	update_data->update_region.height = region.height;
	update_data->update_mode = update_mode;
	update_data->waveform_mode = waveform_mode;
}


int hwtcon_display_no_copy(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct rect display_region,
	int update_mode,
	int waveform_mode,
	int temperature)
{
	int status = 0;
	u32 update_marker = 0;
	struct hwtcon_update_data update_data;

	memset(&update_data, 0, sizeof(update_data));

	do {
		hwtcon_display_set_update_data(&update_data,
			display_region,
			update_mode, waveform_mode);

		/* trigger pipeline work. */
		status = hwtcon_display_to_eink(&update_data,
				temperature,
				&update_marker);
		if (status != 0)
			break;

		/* wait for pipeline display done. */
		status = ioctl(eink.fd, HWTCON_WAIT_FOR_UPDATE_SUBMISSION, &update_marker);
		if (status != 0) {
			TCON_ERR("ioctl wait for marker:%d fail:%d", update_marker, status);
			return status;
		}

	} while (0);

	if (status != 0)
		TCON_ERR("hwtcon_display fail:%d", status);
	else
		TCON_LOG("hwtcon_display OK");

	return status;
}

int hwtcon_display_set_night_mode(int enable)
{
	int status = 0;

	/* for rotate feature */
	TCON_ERR("set night mode %d\n", enable);
	status = ioctl(eink.fd, HWTCON_SET_NIGHTMODE, &enable);
	if (status != 0) {
		TCON_ERR("ioctl HWTCON_SET_NIGHTMODE fail", status);
		return status;
	}
	return 0;
}

int hwtcon_display_region(char *buffer,
	int buffer_width,
	int buffer_height,
	int buffer_pitch,
	struct hwtcon_update_data *update_data,
	int temperature,
	bool wait_submission,
	bool wait_complete)
{
	int status = 0;
	u32 update_marker = 0;
	struct rect display_region = {0};
		display_region.x = update_data->update_region.left; 
		display_region.y = update_data->update_region.top;
		display_region.width = update_data->update_region.width;	
		display_region.height = update_data->update_region.height;
	do {
		/* copy userspace buffer to frame buffer as img buffer. */
		status = hwtcon_copy_screen_buffer_to_img_buffer(buffer,
			buffer_width,
			buffer_height,
			buffer_pitch,
			display_region);

		if (status != 0)
			break;

		/* trigger pipeline work. */
		status = hwtcon_display_to_eink(update_data,
				temperature,
				&update_marker);
		if (status != 0)
			break;
		#if 1
		if (wait_submission) {
			TCON_ERR("wait marker %d submission", update_marker);
			/* wait for pipeline display done. */
			status = ioctl(eink.fd, HWTCON_WAIT_FOR_UPDATE_SUBMISSION, &update_marker);
			if (status != 0) {
				TCON_ERR("ioctl wait for marker:%d fail:%d", update_marker, status);
				return status;
			}
		}

		if (wait_complete) {
			struct hwtcon_update_marker_data data = {update_marker, 0};

			TCON_ERR("wait marker %d complete", update_marker);
			/* wait for pipeline display done. */
			status = ioctl(eink.fd, HWTCON_WAIT_FOR_UPDATE_COMPLETE, &data);
			if (status != 0) {
				TCON_ERR("ioctl wait for marker:%d fail:%d", update_marker, status);
				return status;
			}
		}
		#endif
	} while (0);

	if (status != 0)
		TCON_ERR("hwtcon_display fail:%d", status);
	else
		TCON_LOG("hwtcon_display OK");

	return status;
}


#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <hardware/overlay.h>

//#define LOG_NDEBUG 0
#define LOG_TAG "FB_OVERLAY"
#include <cutils/log.h>

//#include <dmw96osdm_common.h>

#include <osdm_utils.h>
#include "fb_overlay.h"

#include <dspg_buffer.h>

// open the frame-buffer device and create the overlay.
// return the frame-buffer device file descriptor
int fb_overlay_create(int index, int width, int height, int format)
{
	int fd, val;
	char device_name[1024];

	sprintf(device_name, "/dev/graphics/overlay%d", index);

	LOGV("openning device '%s'", device_name);

	if ((fd = open(device_name, O_RDWR)) < 0) {
		// cannot open overlay frame-buffer device
		LOGE("cannot open frame-buffer device '%s'", device_name);
		return -1;
	}

	LOGV("creating new plane");
	if (osdm_plane_create(fd, 0, 0, width, height, format, 0) != 0) {
		LOGE("error creating osdm plane on device '%s'", device_name);
		close(fd);
		return -1;
	}
	// overlay should be still hiden here

	return fd;
}

// destroy the overlay and close the frame-buffer device.
int fb_overlay_destroy(int fd)
{
	fb_overlay_disable(fd);

	// close this overlay
	close(fd);

	return 0;
}

// commit this overlay parameters
// currently ignoring flip_h and flip_v
int fb_overlay_commit_parameters(int fd, int x, int y, int width, int height, 
								 int format, int flip_h, int flip_v, int rotation)
{

	if (osdm_plane_set_color_format(fd, format) != 0) {
		LOGE("error setting color format");
		return -1;
	}

	if (osdm_plane_set_size_and_rotation(fd, width, height, rotation) != 0) {
		LOGE("error setting size and rotation");
		return -1;
	}

	if (osdm_plane_set_position(fd, x, y) != 0) {
		LOGE("error setting position");
		return -1;
	}

	return 0;
}

// get the current position and size of this overlay
int fb_overlay_get_position(int fd, int *x, int *y, uint32_t *w, uint32_t *h)
{
	if (osdm_plane_get_size(fd, w, h) != 0) {
		LOGE("error getting overlay size");
		return -1;
	}

	if (osdm_plane_get_position(fd, x, y) != 0) {
		LOGE("error getting overlay position");
		return -1;
	}

	return 0;
}

// initialize this buffer (set the mmap address, size and status)
int fb_overlay_init_double_buffer(int fd, struct private_overlay_double_buffer_t *buffer)
{
	unsigned int width, height;
	struct fb_fix_screeninfo finfo;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) != 0) return -1;

	if (osdm_plane_get_size(fd, &width, &height) != 0) return -1;

	LOGV("fb_overlay_get_size returned %dx%d",width, height);

	buffer->phys_address = (void*)finfo.smem_start;
	buffer->mmap_size = finfo.smem_len;
	buffer->mmap_address = mmap(NULL, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (!buffer->mmap_address) {
		LOGE("error mapping buffer address");
		// error mapping the buffer
		return -1;
	}

	buffer->active_buffer_index = 0;
	buffer->buffers[0].index = 0;
	buffer->buffers[0].bus_address = 0;
	buffer->buffers[0].virt_address = 0;
	buffer->buffers[1].index = 1;
	buffer->buffers[1].bus_address = 0;
	buffer->buffers[1].virt_address = 0;

	return 0;
}

// destroy this buffer (unmap the memory)
int fb_overlay_deinit_double_buffer(int fd, struct private_overlay_double_buffer_t *buffer)
{
	munmap(buffer->mmap_address, buffer->mmap_size);

	return 0;
}

// pan the buffer
int fb_overlay_pan_buffer(int fd, int yoffset)
{
	return osdm_plane_pan_buffer(fd, yoffset);
}

int fb_overlay_pan_buffer_to_address(int fd, unsigned int virt_address)
{
	return osdm_plane_pan_buffer_to_address(fd, virt_address);
}

int fb_overlay_mix(int fd)
{
	int val=1;

	LOGW("********************** [ fb_overlay_mix ] ***********************");

	//if (ioctl(fd, FBIOSET_OSDM_DISPLAY_MIX, &val) != 0) {
	//	LOGE("error: failed to perform mix");
	//	return -1;
	//}

	return 0;
}

int fb_overlay_enable(int fd)
{
	int val;

	/* set per plane alpha 255 */
	if (osdm_plane_set_per_plane_alpha(fd, 255) != 0) return -1;

	/* enable the plane */
	if (osdm_plane_enable(fd) != 0) return -1;

	return 0;
}

int fb_overlay_disable(int fd)
{
	return osdm_plane_disable(fd);
}



#if 0
/* For internal use */

int fb_overlay_set_size_and_rotation(int fd, int width, int height, int rotation)
{
	struct fb_var_screeninfo vinfo;
	memset(&vinfo, 0, sizeof(struct fb_var_screeninfo));

	// get previous overlay params
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
		LOGE("error getting overlay params");
		return -1;
	}

	vinfo.xres = width;
	vinfo.yres = height;
	vinfo.yres_virtual = height*2;
	vinfo.rotate = rotation;
	vinfo.activate = FB_ACTIVATE_NOW;

	// set overlay dimensions
	if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) != 0) {
		LOGE("error setting overlay dimensions");
		return -1;
	}

	return 0;
}

int fb_overlay_get_size(int fd, unsigned int *width, unsigned int *height)
{
	struct fb_var_screeninfo vinfo;

	// get overlay dimensions
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
		LOGE("error getting overlay dimensions");
		return -1;
	}

	*width = (unsigned int)vinfo.xres;
	*height = (unsigned int)vinfo.yres;

	return 0;
}

int fb_overlay_set_position(int fd, int x, int y)
{
	struct osdm_position pos;

	pos.xpos = x;
	pos.ypos = y;

	if (ioctl(fd, FBIOSET_OSDM_POS, &pos) != 0) {
		return -1;
	}

	return 0;
}

int fb_overlay_set_color_format(int fd, int format)
{
	int val;

	if ((val = get_osdm_color_format(format)) < 0) {
		LOGE("cannot find the correct osdm color format");
		// cannot find the correct osdm color format
		return -1;
	}

	LOGV("setting color format = 0x%x", val);
	// set overlay color format
	if (ioctl(fd, FBIOSET_OSDM_CONFIG, &val) != 0) {
		LOGE("error setting osdm color format");
		// error setting osdm color format
		return -1;
	}

	if (fb_overlay_set_yuv_defaults(fd) != 0) {
		LOGE("error setting yuv defaults");
		return -1;
	}

	return 0;
}

int fb_overlay_set_yuv_defaults(int fd)
{
	struct osdm_video_limits yuv_limits;
	struct osdm_video_color_control color_ctr;

	/**********************************************************************************************************/
	/*									Color control setting												  */
	/**********************************************************************************************************/
	color_ctr.brightness = 0;
	color_ctr.contrast = 64;
	color_ctr.hue = 0;
	color_ctr.saturation = 64;
	if (ioctl(fd, FBIOSET_OSDM_VIDEO_COLOR_CONTROL, &color_ctr) < 0) {
		LOGE("error: ioctl FBIOSET_OSDM_VIDEO_COLOR_CONTROL failed");
		return -1;
	}

	/**********************************************************************************************************/
	/*									Plane YUV limits setting											  */
	/**********************************************************************************************************/
	yuv_limits.chroma_max = 240;
	yuv_limits.chroma_min = 16;
	yuv_limits.luma_max = 235;
	yuv_limits.luma_min = 16;
	if (ioctl(fd, FBIOSET_OSDM_VIDEO_LIMIT, &yuv_limits) < 0) {
		LOGE("error: ioctl FBIOSET_OSDM_VIDEO_LIMIT failed");
		return -1;
	}

	return 0;
}



int fb_overlay_get_yuv_address(int fd, unsigned int addr, int overlay_color_format, unsigned int *y, unsigned int *u, unsigned int *v)
{
	struct fb_var_screeninfo vinfo;
	int osdm_color_format;

	// get overlay dimensions
	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
		LOGE("error getting overlay dimensions");
		return -1;
	}

	osdm_color_format = get_osdm_color_format(overlay_color_format);

	dmw96osdm_plane_get_address(&vinfo, addr , osdm_color_format, y, u, v);

	return 0;
}


unsigned int get_bits_per_pixel(int format)
{
	switch (format) {
		case OVERLAY_FORMAT_RGBA_8888:
		case OVERLAY_FORMAT_BGRA_8888:
			return 32;
		case OVERLAY_FORMAT_RGB_565:
		case OVERLAY_FORMAT_YCbCr_422_SP:
		case OVERLAY_FORMAT_YCbCr_422_PLANAR:
			return 16;
		case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
		case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
		case OVERLAY_FORMAT_YCbCr_420_PLANAR:
			return 12;
	}
	return 0;
}

int get_osdm_color_format(int overlay_color_format)
{
	switch(overlay_color_format) {
		case OVERLAY_FORMAT_RGBA_8888:
			return OSDM_RGB32_8888_RGBA;
		case OVERLAY_FORMAT_BGRA_8888:
			return OSDM_RGB32_8888_BGRA;
		case OVERLAY_FORMAT_RGB_565:
			return OSDM_RGB16_565_RGB;
		case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
			return OSDM_YUV420_MPEG1_JPEG_SEMI_PLANAR_V1_U1_V0_U0;
		case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
			return OSDM_YUV420_MPEG2_SEMI_PLANAR_V1_U1_V0_U0;
		case OVERLAY_FORMAT_YCbCr_422_SP:
			return OSDM_YUV422_SEMI_PLANAR_V1_U1_V0_U0;
		case OVERLAY_FORMAT_YCbCr_420_PLANAR:
			return OSDM_YUV420_MPEG1_JPEG_PLANAR;
		case OVERLAY_FORMAT_YCbCr_422_PLANAR:
			return OSDM_YUV422_PLANAR;
	}
	
	// should not get here
	return -1;
}
#endif


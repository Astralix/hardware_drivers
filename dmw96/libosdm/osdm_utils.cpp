#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#define LOG_TAG "libosdm"
#include <cutils/log.h>

#include <hardware/overlay.h>

#include <dmw96osdm_common.h>

#include "osdm_utils.h"

int osdm_plane_create(int fd, int x, int y, int width, int height, int color_format, int rotation)
{
	if (osdm_plane_set_color_format(fd, color_format) < 0) {
		LOGE("cannot set color format (%d)\n", color_format);
		close(fd);
		return -1;
	}

	if (osdm_plane_set_size_and_rotation(fd, width, height, rotation) < 0) {
		LOGE("cannot set size and rotation\n");
		close(fd);
		return -1;
	}

	if (osdm_plane_set_position(fd, x, y) < 0) {
		LOGE("cannot set position\n");
		close(fd);
		return -1;
	}

	return 0;
}

int osdm_get_varinfo(int fd, struct fb_var_screeninfo *vinfo)
{
	if (ioctl(fd, FBIOGET_OSDM_VSCREENINFO, vinfo) != 0) {
		LOGE("error getting overlay parameters (FBIOGET_OSDM_VSCREENINFO failed)");
		return -1;
	}

	return 0;
}

int osdm_plane_set_size_and_rotation(int fd, int width, int height, int rotation)
{
	struct fb_var_screeninfo vinfo;

	// get overlay params
	if (osdm_get_varinfo(fd,&vinfo) != 0) return -1;

	vinfo.xres = width;
	vinfo.yres = height;
	vinfo.xres_virtual = width;
	vinfo.yres_virtual = height*2;
	vinfo.rotate = rotation;
	vinfo.activate = FB_ACTIVATE_NOW;

	// set overlay dimensions
	if (ioctl(fd, FBIOPUT_OSDM_VSCREENINFO, &vinfo) != 0) {
		LOGE("error setting overlay dimensions (FBIOPUT_OSDM_VSCREENINFO failed)");
		return -1;
	}

	return 0;
}


int osdm_plane_set_transparent_pos_and_dimension(int fd, int x, int y, int width, int height)
{
	struct osdm_position pos;

	pos.xpos = x;
	pos.ypos = y;

	LOGV("Setting transparency - x=%d, y=%d, width=%d, height=%d ", x, y, width, height);

	if (ioctl(fd, FBIOSET_OSDM_TRANSPARENCY_POS, &pos) != 0) {
		LOGE("error setting transparency position (FBIOSET_OSDM_TRANSPARENCY_POS failed)");
		return -1;
	}

	struct fb_var_screeninfo vinfo;

	// get overlay params
	if (osdm_get_varinfo(fd,&vinfo) != 0) return -1;

	vinfo.xres = width;
	vinfo.yres = height;
	vinfo.xres_virtual = width;
	vinfo.yres_virtual = height*2;

	// set overlay dimensions
	if (ioctl(fd, FBIOSET_OSDM_TRANSPARENCY_DIM, &vinfo) != 0) {
		LOGE("error setting transparency dimensions (FBIOSET_OSDM_TRANSPARENCY_DIM failed)");
		return -1;
	}

	return 0;
}

int osdm_plane_get_size(int fd, unsigned int *width, unsigned int *height)
{
	struct fb_var_screeninfo vinfo;

	// get overlay dimensions
	if (osdm_get_varinfo(fd,&vinfo) != 0) return -1;

	*width = (unsigned int)vinfo.xres;
	*height = (unsigned int)vinfo.yres;

	return 0;
}

int osdm_plane_set_position(int fd, int x, int y)
{
	struct osdm_position pos;

	pos.xpos = x;
	pos.ypos = y;

	if (ioctl(fd, FBIOSET_OSDM_POS, &pos) != 0) {
		return -1;
	}

	return 0;
}

int osdm_plane_get_position(int fd, int *x, int *y)
{
	struct osdm_position pos;

	if (ioctl(fd, FBIOGET_OSDM_POS, &pos) != 0) {
		return -1;
	}

	*x = pos.xpos;
	*y = pos.ypos;

	return 0;
}

int osdm_plane_set_rotation(int fd, int rotation)
{
	struct fb_var_screeninfo vinfo;

	// get overlay params
	if (osdm_get_varinfo(fd,&vinfo) != 0) return -1;

	vinfo.rotate = rotation;

	// set overlay dimensions
	if (ioctl(fd, FBIOPUT_OSDM_VSCREENINFO, &vinfo) != 0) {
		LOGE("error setting overlay rotation (FBIOPUT_OSDM_VSCREENINFO failed)");
		return -1;
	}

	return 0;
}

int osdm_plane_get_rotation(int fd, int *rotation)
{
	struct fb_var_screeninfo vinfo;

	// get overlay params
	if (osdm_get_varinfo(fd,&vinfo) != 0) return -1;

	*rotation = vinfo.rotate;

	return 0;
}


int osdm_plane_set_color_format(int fd, int format)
{
	int val;

	if ((val = get_osdm_color_format(format)) < 0) {
		LOGE("cannot find the correct osdm color format for overlay format %d",format);
		// cannot find the correct osdm color format
		return -1;
	}

	// set overlay color format
	if (ioctl(fd, FBIOSET_OSDM_CONFIG, &val) != 0) {
		LOGE("error setting osdm color format");
		// error setting osdm color format
		return -1;
	}

	return 0;
}

int osdm_plane_enable(int fd)
{
	int val;

	/* set per plane alpha 255 */
	val = 255;
	if (ioctl(fd, FBIOSET_OSDM_PER_PLANE_ALPHA, &val) < 0) {
		LOGE("error: cannot set per plane alpha\n");
		return -1;
	}

	val = 1;
	if (ioctl(fd, FBIOSET_OSDM_PLANE_ENB, &val) != 0) {
		LOGE("error: enable overlay failed");
		return -1;
	}

	return 0;
}

int osdm_plane_per_pixel_alpha_enable(int fd)
{
	int val = 1;

	if (ioctl(fd, FBIOSET_OSDM_PER_PIXEL_ALPHA_ENB, &val) < 0) {
		LOGE("error: FBIOSET_OSDM_PER_PIXEL_ALPHA_ENB failed\n");
		return -1;
	}

	return 0;
}

int osdm_plane_set_per_plane_alpha(int fd, int alpha)
{
	if (ioctl(fd, FBIOSET_OSDM_PER_PLANE_ALPHA, &alpha) < 0) {
		LOGE("error: FBIOSET_OSDM_PER_PLANE_ALPHA failed\n");
		return -1;
	}

	return 0;
}


int osdm_plane_disable(int fd)
{
	int val = 0;

    if (ioctl(fd, FBIOSET_OSDM_PLANE_ENB, &val) != 0) {
		LOGE("error: disable overlay failed");
		return -1;
	}

    // this is needed to commit the plane disable
    osdm_plane_pan_buffer(fd, 0);

	return 0;
}

int osdm_plane_pan_buffer(int fd, int yoffset)
{
	struct fb_var_screeninfo vinfo;

//	if (osdm_get_varinfo(fd, &vinfo) != 0) return -1;

	vinfo.yoffset = yoffset;
	vinfo.reserved[0] = 0;

	if (ioctl(fd, FBIOPAN_OSDM_DISPLAY, &vinfo) != 0) {
		LOGE("error: overlay pan buffer failed (yoffset = %d)", yoffset);
		return -1;
	}

	return 0;
}

int osdm_plane_pan_buffer_to_address(int fd, unsigned int virt_address)
{
	struct fb_var_screeninfo vinfo;

//	if (osdm_get_varinfo(fd, &vinfo) != 0) return -1;

	/* mark this pan as zero copy pan with reserved[0] = PAN_OSDM_ZERO_COPY_MAGIC */
	vinfo.yoffset = 0;
	vinfo.reserved[0] = PAN_OSDM_ZERO_COPY_MAGIC;
	vinfo.reserved[1] = virt_address;

	if (ioctl(fd, FBIOPAN_OSDM_DISPLAY, &vinfo) != 0) {
		LOGE("error: overlay pan buffer address failed (virt_address = %u)", virt_address);
		return -1;
	}

	return 0;
}

int osdm_get_screen_size(int *width, int *height)
{
	char fb_file[32] = "/dev/graphics/fb0";
	int fd;
	struct fb_var_screeninfo info;

	fd = open(fb_file, O_RDONLY);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1) {
		return -1;
	}

	*width = info.xres;
	*height = info.yres;

	close(fd);

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
			return 16;
		case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
		case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
			return 12;
	}
	return 0;
}

int get_osdm_color_format(int overlay_color_format)
{
	switch(overlay_color_format) {
		case OVERLAY_FORMAT_RGBA_8888:
			/* This is an ugly hack, Android paints the colors in the opposite order */
			return OSDM_RGB32_8888_ABGR;
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
	}
	
	// should not get here
	return -1;
}


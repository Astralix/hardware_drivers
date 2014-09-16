#include <dspg_buffer.h>

struct private_overlay_double_buffer_t {
	void *mmap_address;
	void *phys_address;
	unsigned long mmap_size;
	int active_buffer_index;
	dspg_buffer_t buffers[2];
};

#define MAX_OVERLAYS 4

// open the frame-buffer device and create the overlay.
// return the frame-buffer device file descriptor
int fb_overlay_create(int index, int width, int height, int format);

// destroy the overlay and close the frame-buffer device.
int fb_overlay_destroy(int fd);

// commit this overlay parameters
int fb_overlay_commit_parameters(int fd, int x, int y, int width, int height, 
								 int format, int flip_h, int flip_v,int rotation);

// get the current position and size of this overlay
int fb_overlay_get_position(int fd, int *x, int *y, uint32_t *w, uint32_t *h);

// initialize this buffer (set the mmap address, size and status)
int fb_overlay_init_double_buffer(int fd, struct private_overlay_double_buffer_t *buffer);

// destroy this buffer (do munmap on the memory)
int fb_overlay_deinit_double_buffer(int fd, struct private_overlay_double_buffer_t *buffer);

// pan the buffer
int fb_overlay_pan_buffer(int fd, int yoffset);

// pan the buffer to the desired bus address
int fb_overlay_pan_buffer_to_address(int fd, unsigned int virt_address);

int fb_overlay_enable(int fd);
int fb_overlay_disable(int fd);

int fb_overlay_mix(int fd);

int fb_overlay_set_yuv_defaults(int fd);



int fb_overlay_get_yuv_address(int fd, unsigned int addr, int overlay_color_format, unsigned int *y, unsigned int *u, unsigned int *v);

/* For internal use */
/*
unsigned int get_bits_per_pixel(int format);
int get_osdm_color_format(int overlay_color_format);

int fb_overlay_get_size(int fd, unsigned int *width, unsigned int *height);
int fb_overlay_set_size_and_rotation(int fd, int width, int height, int rotation);
int fb_overlay_set_position(int fd, int x, int y);
int fb_overlay_set_color_format(int fd, int format);
*/

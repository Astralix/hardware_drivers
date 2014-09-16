

#define OSDM_PLANE_MAIN_GUI "/dev/graphics/main_osd"
#define OSDM_PLANE_OVERLAY_0 "/dev/graphics/overlay0"
#define OSDM_PLANE_OVERLAY_1 "/dev/graphics/overlay1"
#define OSDM_PLANE_OVERLAY_2 "/dev/graphics/overlay2"
#define OSDM_PLANE_OVERLAY_3 "/dev/graphics/overlay3"

int osdm_plane_create(int fd, int x, int y, int width, int height, int color_format, int rotation);

int osdm_plane_set_size_and_rotation(int fd, int width, int height, int rotation);
int osdm_plane_get_size(int fd, unsigned int *width, unsigned int *height);
int osdm_plane_set_transparent_pos_and_dimension(int fd, int x, int y, int width, int height);
int osdm_plane_set_position(int fd, int x, int y);
int osdm_plane_get_position(int fd, int *x, int *y);
int osdm_plane_set_rotation(int fd, int rotation);
int osdm_plane_get_rotation(int fd, int *rotation);
int osdm_plane_set_color_format(int fd, int format);
int osdm_plane_enable(int fd);
int osdm_plane_disable(int fd);
int osdm_plane_per_pixel_alpha_enable(int fd);
int osdm_plane_set_per_plane_alpha(int fd, int alpha);

int osdm_plane_pan_buffer(int fd, int yoffset);
int osdm_plane_pan_buffer_to_address(int fd, unsigned int virt_address);

int osdm_get_screen_size(int *width, int *height);
int osdm_get_varinfo(int fd, struct fb_var_screeninfo *vinfo);

unsigned int get_bits_per_pixel(int format);
int get_osdm_color_format(int overlay_color_format);


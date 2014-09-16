
//#define LOG_NDEBUG 0
#define LOG_TAG "Cursor"
#include <utils/Log.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/ioctl.h>
#include <linux/fb.h>

#include <Cursor.h>

#include "dwfb_common.h"


/* default cursor bitmap */
unsigned char cursor_default_32[] = {
    1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,2,2,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,2,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,2,1,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,2,1,1,3,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,1,2,2,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,1,3,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* alternative cursor bitmap */
unsigned char cursor_alternative_32[] = {
    1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,2,2,2,2,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,2,2,2,3,3,3,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,2,3,2,2,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,3,1,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,3,1,1,3,2,2,1,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,1,3,2,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,1,3,2,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,3,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


Cursor::Cursor(const char *devName)
{
    isInitialized = true;
    isVisible = false;

    fd = open(devName, O_RDWR);
    if (fd < 1) {
        LOGE("error openning mouse cursor device '%s'", devName);
        isInitialized = false;
        return;
    }

    /* check if this is really our cursor device */
    struct dwfb_location location;
    int ret = ioctl(fd, FBIOGET_LCDC_CURSOR_LOCATION, &location);
    if (ret<0) {
        LOGE("Cursor constructor: FBIOGET_LCDC_CURSOR_LOCATION ioctl failed (%d), probably not a cursor device", ret);
        isInitialized = false;
        return;
    }
}

Cursor::~Cursor()
{
    if (isInitialized) {
        setVisible(false);
        close(fd);
    }
}

int Cursor::setPosition(int x, int y)
{
    if (!isInitialized) {
        LOGW("setPosition: cursor not initialized");
        return -1;
    }

    struct dwfb_location location;

    location.x_start = x;
    location.y_start = y;

    int ret = ioctl(fd, FBIOSET_LCDC_CURSOR_LOCATION, &location);
    if (ret<0) {
        LOGE("setPosition: ioctl failed (%d)", ret);
        return -1;
    }

    return 0;
}

int Cursor::setVisible(bool visible)
{
    if (!isInitialized) {
        LOGW("setVisible: cursor not initialized");
        return -1;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
        LOGE("setVisible: FBIOGET_VSCREENINFO ioctl failed");
        return -1;
    }

    vinfo.nonstd = (int)visible;
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) != 0) {
        LOGE("setVisible: FBIOPUT_VSCREENINFO ioctl failed");
        return -1;
    }

    return 0;
}

int Cursor::setDefaultBitmap()
{
    return setBitmap(cursor_default_32, sizeof(cursor_default_32)/8);
}

int Cursor::setAlternativeBitmap()
{
    return setBitmap(cursor_alternative_32, sizeof(cursor_alternative_32)/8);
}

int Cursor::setBitmap(unsigned char *cursor, unsigned int size)
{
    if (!isInitialized) {
        LOGW("setBitmap: cursor not initialized");
        return -1;
    }

    int ret;
    unsigned int i, counter;
    struct cursor_bitmap bitmap;

    memset(bitmap.bitmap_array, 0, LCDC_CURSOR_BITMAP_ARRAY_SIZE);

    for (i=0 ; i<size ; i++) {
        unsigned short pixels = 0;
        for (counter = 0; counter<8;counter++) {
            pixels>>=2;
            pixels |= (cursor[i*8 + counter])<<14;
        }
        bitmap.bitmap_array[i] = pixels;
    }
    bitmap.bitmap_size = LCDC_CURSOR_32x32;

    ret = ioctl(fd, FBIOSET_LCDC_CURSOR_BITMAP, &bitmap);
    if (ret<0) {
        LOGE("setBitmap: ioctl failed (%d)", ret);
    }

    return ret;
}

int Cursor::setDefaultPallete()
{
    if (!isInitialized) {
        LOGW("setDefaultPallete: cursor not initialized");
        return -1;
    }

    struct fb_fix_screeninfo finfo;

    struct fb_cmap map;
    int ret;

    map.start = 0;
    map.len = 3;
    map.red = (unsigned short*)malloc(sizeof(unsigned short)*3);
    map.green = (unsigned short*)malloc(sizeof(unsigned short)*3);
    map.blue = (unsigned short*)malloc(sizeof(unsigned short)*3);
    map.transp = NULL;

    // first is black
    map.red[0] = 0;
    map.green[0] = 0;
    map.blue[0] = 0;

    // second is white
    map.red[1] = 0x1f;
    map.green[1] = 0x3f;
    map.blue[1] = 0x1f;

    // third is gray
    map.red[2] = 0x10;
    map.green[2] = 0x20;
    map.blue[2] = 0x10;

    ret = ioctl(fd, FBIOPUTCMAP, &map);

    free(map.red);
    free(map.green);
    free(map.blue);

    if (ret < 0) {
        LOGE("setDefaultPallete ioctl error (%d)", ret);
        return -1;
    }

    return 0;
}


/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "Overlay"
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/overlay.h>

#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <cutils/ashmem.h>
#include <cutils/atomic.h>

#include <osdm_utils.h>
#include <dspg_buffer.h>
#include "fb_overlay.h"

#include "PostProcessor.h"

//#define LOG_FUNCTIONS
#ifdef LOG_FUNCTIONS
  #define LOG_FUNCTION_START	LOGV("%s: start", __func__);
  #define LOG_FUNCTION_END		LOGV("%s: end ok", __func__);
  #define LOG_FUNCTION_END_ERROR(x...)	LOGV("%s: end error(%s)", __func__, x);
#else
  #define LOG_FUNCTION_START	do{}while(0);
  #define LOG_FUNCTION_END		do{}while(0);
  #define LOG_FUNCTION_END_ERROR(x...)	do{}while(0);
#endif

#define VIDEO_MAIN_LAYER 0
#define VIDEO_SUB_LAYER 1

/*****************************************************************************/
enum {
    OVERLAY_TYPE_DECODER = 1,
    OVERLAY_TYPE_SCALING_DECODER,
    OVERLAY_TYPE_CAMERA,
};

typedef struct
{
    int x;
    int y;
    uint32_t width;
    uint32_t height;
    uint32_t rotation;
    uint32_t flip_h;
    uint32_t flip_v;
} overlay_ctrl_t;


/* Data that needs to be shared between control and data modules */
typedef struct 
{
    int overlayType;
    int allow_display;
    int do_enable_on_next_queue;
    unsigned int screenWidth;
    unsigned int screenHeight;
    unsigned int inputWidth;
    unsigned int inputHeight;
    unsigned int outputWidth;
    unsigned int outputHeight;
    unsigned int flip_h;
    unsigned int flip_v;
    unsigned int frameSize;
    sem_t mutex;

    int is_alive;

} shared_data_t;

/* returns the frame size in bytes */
static unsigned int get_frame_size(int format, int width, int height)
{
    int bpp = get_bits_per_pixel(format);

    return width*height*bpp/8;
}

static int create_shared_data()
{
    int fd;

    if ((fd = ashmem_create_region("overlay_shared_data", sizeof(shared_data_t))) < 0) {
        LOGE("create_shared_mem: error creating shared memory (%d)", fd);
        return -1;
    }

    return fd;
}

static int release_shared_data(shared_data_t *data)
{
    if (munmap(data, sizeof(shared_data_t)) != 0) {
        LOGE("release_shared_data: munmap error");
        return -1;
    }

    return 0;
}

static shared_data_t* get_shared_data(int fd)
{
    shared_data_t *data;

    data = (shared_data_t*)mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data < 0) {
        LOGE("get_shared_data: mmap error (%p)", data);
        return NULL;
    }

    return data;
}

/*****************************************************************************/

struct overlay_control_context_t {
    struct overlay_control_device_t device;
    /* our private state goes below here */
    overlay_t *overlay[4];
    sem_t create_lock;
};

struct overlay_data_context_t {
    struct overlay_data_device_t device;
    /* our private state goes below here */
    // same as the handle_t inside overlay_object
    int fd;
    int shared_fd;
    int format;
    uint32_t width;
    uint32_t height;
    unsigned int frame_size;
    int x;
    int y;
    int rotation;
    int flip_h;
    int flip_v;
    int visible;
    shared_data_t *shared_data;
    PostProcessor *pp;
    struct private_overlay_double_buffer_t double_buffer;
};

static int overlay_device_open(const struct hw_module_t* module, const char* name,
                               struct hw_device_t** device);

static struct hw_module_methods_t overlay_module_methods = {
    open: overlay_device_open
};

struct overlay_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: OVERLAY_HARDWARE_MODULE_ID,
        name: "DSPG Overlay module",
        author: "Gal Sasson",
        methods: &overlay_module_methods,
    }
};

/*****************************************************************************/

/*
 * This is the overlay_t object, it is returned to the user and represents
 * an overlay.
 * This handles will be passed across processes and possibly given to other
 * HAL modules (for instance video decode modules).
 */
struct handle_t : public native_handle {
    /* add the data fields we need here, for instance: */
    int fd;
    int shared_fd;
    int index;
    int format;
    unsigned int input_width;
    unsigned int input_height;
};

static int handle_get_fd(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->fd;
}

static int handle_get_shared_fd(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->shared_fd;
}

static int handle_get_format(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->format;
}

//static int handle_get_allow_display(const overlay_handle_t overlay) {
//    return static_cast<const struct handle_t *>(overlay)->allow_display;
//}

class overlay_object : public overlay_t 
{ 
    handle_t mHandle;

    overlay_ctrl_t mControl;
    overlay_ctrl_t mStageControl;

    static overlay_handle_t getHandleRef(struct overlay_t* overlay) {
        /* returns a reference to the handle, caller doesn't take ownership */
        return &(static_cast<overlay_object *>(overlay)->mHandle);
    }
    
public:
    int mposX;
    int mposY;
    unsigned int mWidth;
    unsigned int mHeight;

    int mTransparent_posX;
    int mTransparent_posY;
    unsigned int mTransparent_Width;
    unsigned int mTransparent_Height;

    shared_data_t *mSharedData;
    int pending_scale_request;

    overlay_object(int fd, int shared_fd, int index, int w, int h, int format) {
        this->overlay_t::getHandleRef = getHandleRef;
        mHandle.version = sizeof(native_handle);
        mHandle.numFds = 2;
        mHandle.numInts = 4; // extra ints we have in our handle
        mHandle.fd = fd;
        mHandle.shared_fd = shared_fd;
        mHandle.index = index;
        mHandle.format = format;
        mHandle.input_width = w;
        mHandle.input_height = h;
        this->w = w;
        this->h = h;
        this->format = format;

        memset(&mControl, 0, sizeof(overlay_ctrl_t));
        memset(&mStageControl, 0, sizeof(overlay_ctrl_t));

        mControl.width = w;
        mControl.height = h;
    }

    int get_fd() { return mHandle.fd; }
    void set_fd(int fd) { mHandle.fd = fd; }
    int get_index() { return mHandle.index; }
    int get_format() { return mHandle.format; }
    void set_format(int f) { mHandle.format = f; }
    unsigned int get_input_width() { return mHandle.input_width; }
    unsigned int get_input_height() { return mHandle.input_height; }
    overlay_ctrl_t* control() { return &mControl; }
    overlay_ctrl_t* stage_control() { return &mStageControl; }
};

int overlay_commit_parameters(overlay_object *overlay)
{
    overlay_ctrl_t *ctrl = overlay->control();

    return fb_overlay_commit_parameters(overlay->get_fd(),
                                        ctrl->x,
                                        ctrl->y,
                                        ctrl->width,
                                        ctrl->height,
                                        overlay->get_format(),
                                        ctrl->flip_h,
                                        ctrl->flip_v,
                                        ctrl->rotation);
}


// ****************************************************************************
// Control module
// ****************************************************************************

static int overlay_get(struct overlay_control_device_t *dev, int name) 
{
    int result = -1;
    switch (name) {
        case OVERLAY_MINIFICATION_LIMIT:
            result = 1; // 0 = no limit
            break;
        case OVERLAY_MAGNIFICATION_LIMIT:
            result = 1; // 0 = no limit
            break;
        case OVERLAY_SCALING_FRAC_BITS:
            result = 1; // 0 = infinite
            break;
	case OVERLAY_ROTATION_STEP_DEG:
            result = 90;
            break;
        case OVERLAY_HORIZONTAL_ALIGNMENT:
            result = 1; 
            break;
        case OVERLAY_VERTICAL_ALIGNMENT:
            result = 1; 
            break;
        case OVERLAY_WIDTH_ALIGNMENT:
            result = 64; 
            break;
        case OVERLAY_HEIGHT_ALIGNMENT:
            result = 64; 
            break;
    }

    return result;
}

static overlay_t* overlay_createOverlay(struct overlay_control_device_t *dev,
                                        uint32_t w, uint32_t h, int32_t format) 
{
    LOG_FUNCTION_START
    int fd;
    //int type = OVERLAY_TYPE_SCALING_DECODER;
    int type = OVERLAY_TYPE_DECODER;
    int shared_fd;
    overlay_object *overlay = NULL;

    /* check the input params, reject if not supported or invalid */
    switch (format) 
    {
        case OVERLAY_FORMAT_DEFAULT:
            format = OVERLAY_FORMAT_YCbCr_420_SP_MPEG2;
            /* This is an ugly hack, camera service opens the overlay with this color format, so we know this is the camera */
            type = OVERLAY_TYPE_CAMERA;
            break;
        case OVERLAY_FORMAT_RGBA_8888:
        case OVERLAY_FORMAT_RGB_565:
        case OVERLAY_FORMAT_BGRA_8888:
            // add supported format here (especially YUV formats)
        case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
        case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
        case OVERLAY_FORMAT_YCbCr_422_SP:
        case OVERLAY_FORMAT_YCbCr_420_PLANAR:
        case OVERLAY_FORMAT_YCbCr_422_PLANAR:
            break;
        default:
            return NULL;
    }

    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
    sem_wait(&ctx->create_lock);

    // find the next available overlay device
    int overlay_index = -1;
    for (int i=0 ; i<4 ; i++) {
        if (ctx->overlay[i] == NULL) {
            overlay_index = i;
            break;
        }
    }

    LOGV("overlay_createOverlay: overlay_index = %d (size = %dx%d)", overlay_index, w, h);

    if (overlay_index == -1) goto create_error;

    /* Create overlay object. Talk to the h/w here and adjust to what it can
     * do. the overlay_t returned can  be a C++ object, subclassing overlay_t
     * if needed.
     * 
     * we probably want to keep a list of the overlay_t created so they can
     * all be cleaned up in overlay_close(). 
     */

    // open the framebuffer device with index 'overlay_index'
    if ((fd = fb_overlay_create(overlay_index, w, h, format)) < 0) {
        LOGE("error creating overlay (fb_overlay_create failed)");
        goto create_error;
    }

    /* disable this overlay */
    fb_overlay_disable(fd);

    /* create the shared data for the overlay */
    shared_fd = create_shared_data();
    if (shared_fd < 0) {
        LOGE("error create shared data");
        goto create_error;
    }

    overlay = new overlay_object(fd, shared_fd, overlay_index, w, h, format);

    overlay->mSharedData = get_shared_data(shared_fd);
    if (overlay->mSharedData == NULL) {
        LOGE("error getting shared data");
        goto create_error;
    }

    // this flag is needed in case the overlay is destroyed before the overlay data module
    overlay->mSharedData->is_alive = 1;

    /* init the semaphore on the shared memory */
    if (sem_init(&overlay->mSharedData->mutex, 1, 1) != 0) {
        LOGE("error creating semaphore");
        goto create_error;
    }

    // dont allow display before the first commit
    overlay->mSharedData->allow_display = 0;
    overlay->mSharedData->overlayType = type;
    overlay->mSharedData->inputWidth = w;
    overlay->mSharedData->inputHeight = h;
    overlay->mSharedData->outputWidth = w;
    overlay->mSharedData->outputHeight = h;
    overlay->mSharedData->frameSize = get_frame_size(format, w, h);
    /* get the screen size */
    osdm_get_screen_size((int*)&overlay->mSharedData->screenWidth, (int*)&overlay->mSharedData->screenHeight);

    overlay->pending_scale_request = 0;

    ctx->overlay[overlay_index] = overlay;

    overlay->mTransparent_Width = 0;
    overlay->mTransparent_Height = 0;
    overlay->mTransparent_posX = 0;
    overlay->mTransparent_posY = 0;

    // TODO : validate boundary conditions and planes
    if(overlay_index == VIDEO_SUB_LAYER && ctx->overlay[VIDEO_MAIN_LAYER] != NULL) {
        // Set the transparency params for sub video window.
        // This will committed together with setposition()
        overlay_object *overlay_main = static_cast<overlay_object *>(ctx->overlay[VIDEO_MAIN_LAYER]);
        overlay->mTransparent_Width = overlay_main->mWidth;
        overlay->mTransparent_Height = overlay_main->mHeight;
        overlay->mTransparent_posX = overlay_main->mposX;
        overlay->mTransparent_posY = overlay_main->mposY;
    }

    LOG_FUNCTION_END
    sem_post(&ctx->create_lock);
    return overlay;

create_error:
    sem_post(&ctx->create_lock);
    return NULL;
}

static void overlay_destroyOverlay(struct overlay_control_device_t *dev,
         overlay_t* overlay) 
{
    LOG_FUNCTION_START

    /* free resources associated with this overlay_t */
    overlay_control_context_t *ctx = (overlay_control_context_t*)dev;
    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);

    sem_wait(&ctx->create_lock);
	
    /* release the shared data */
    if (overlay_obj->mSharedData) {
        overlay_obj->mSharedData->is_alive = 0;
		
        sem_destroy(&overlay_obj->mSharedData->mutex);
        release_shared_data(overlay_obj->mSharedData);
    }

    // tell the hw to destroy the overlay and close the device
    fb_overlay_destroy(overlay_obj->get_fd());

    // set NULL in the control context
    ctx->overlay[overlay_obj->get_index()] = NULL;

    sem_post(&ctx->create_lock);

    LOG_FUNCTION_END
    delete overlay;
}

static int overlay_get_64aligned_scale(uint32_t width, uint32_t height, uint32_t *aligned_width, uint32_t *aligned_height)
{
    uint32_t w = width;
    uint32_t h = height;
	 
    if (w<64) w = 64;
    if (h<64) h = 64;

    if (w>1920) w = 1920;
    if (h>1080) h = 1080;

    *aligned_width = w&0xffffffc0;
    *aligned_height = h&0xffffffc0;

    return 0;
}

static int overlay_sendScaleRequest(uint32_t width, uint32_t height)
{
    int scaler_fd;
    char buff[16];

    LOGV("overlay_sendScaleRequest: size = %dx%d", width, height);

    scaler_fd = open("/tmp/decoder-scaler", O_WRONLY);
    if (scaler_fd < 0) return -1;

    sprintf(buff, "%dx%d", width, height);
    if (write(scaler_fd, buff, strlen(buff)+1) < 0) {
        return -2;
    }

    close(scaler_fd);
    return 0;
}

static int overlay_setPosition(struct overlay_control_device_t *dev,
                               overlay_t* overlay, 
                               int x, int y, uint32_t w, uint32_t h) 
{
    LOG_FUNCTION_START
    int result = 0;

    LOGV("overlay_setPosition: (pos = %dx%d, size = %dx%d)", x, y, w, h);

    /* set this overlay's position (talk to the h/w) */
    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *stage = overlay_obj->stage_control();

    /* On a transition between states android gives us negative position and some realy big sizes
       Make sure that the window is inside the screen. If not, try to correct this */
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (w > overlay_obj->mSharedData->screenWidth) w = overlay_obj->mSharedData->screenWidth;
    if (h > overlay_obj->mSharedData->screenHeight) h = overlay_obj->mSharedData->screenHeight;
    if (w+x > overlay_obj->mSharedData->screenWidth) w = overlay_obj->mSharedData->screenWidth - x;
    if (h+y > overlay_obj->mSharedData->screenHeight) h = overlay_obj->mSharedData->screenHeight - y;

    if (w < 16 || h < 16) {
        LOGE("overlay_setPosition: error: width or height < 16");
        return -1;
    }

    stage->x = x;
    stage->y = y;
    stage->width = w;
    stage->height = h;

    overlay_obj->mposX = x;
    overlay_obj->mposY = y;
    overlay_obj->mWidth = w;
    overlay_obj->mHeight = h;

    LOG_FUNCTION_END
    return result;
}

static int overlay_getPosition(struct overlay_control_device_t *dev,
                               overlay_t* overlay, 
                               int* x, int* y, uint32_t* w, uint32_t* h) 
{
    LOG_FUNCTION_START

    /* get this overlay's position */
    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);

    // gals: the current position of the overlay as the driver sees it, not the offline data
    if (fb_overlay_get_position(overlay_obj->get_fd(), x, y, w, h) != 0) {
        LOG_FUNCTION_END_ERROR("fb_overlay_get_position failed");
        return -EINVAL;
    }

    LOG_FUNCTION_END
    return 0;
}

static int overlay_setParameter(struct overlay_control_device_t *dev,
                                overlay_t* overlay, int param, int value) 
{
    LOG_FUNCTION_START
    
    int rotation, result = 0;

    /* set this overlay's parameter (talk to the h/w) */
    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *stage = overlay_obj->stage_control();

    switch (param) {
        case OVERLAY_ROTATION_DEG:
            /* if only 90 rotations are supported, the call fails
             * for other values */
            LOGV("overlay_setParameter: OVERLAY_ROTATION_DEG %d",value);
            stage->rotation = value;
            break;
        case OVERLAY_DITHER:
            break;
        case OVERLAY_TRANSFORM:
            LOGV("overlay_setParameter: OVERLAY_TRANSFORM: %d", value);
            stage->rotation = 0;
            stage->flip_h = 0;
            stage->flip_v = 0;

            if (value == OVERLAY_TRANSFORM_ROT_270) {
                stage->rotation = 270;
            }
            else if (value == OVERLAY_TRANSFORM_ROT_180) {
                stage->rotation = 180;
            }
            else {
                if (value & OVERLAY_TRANSFORM_ROT_90) {
                    stage->rotation = 90;
                }

                if (value & OVERLAY_TRANSFORM_FLIP_H) {
                    stage->flip_h = 1;
                }
                else if (value & OVERLAY_TRANSFORM_FLIP_V) {
                    stage->flip_v = 1;
                }
            }
            break;
        default:
            LOGW("overlay_setParameter: unknown param %d", param);
            return -EINVAL;
    }

    LOG_FUNCTION_END
    return result;
}

static int overlay_stage(struct overlay_control_device_t *dev,
                          overlay_t* overlay)
{
    LOG_FUNCTION_START

    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);

    LOG_FUNCTION_END
    return 0;
}

int overlay_setTransparency (struct overlay_control_device_t *dev, overlay_object *overlay_sub, 
                            overlay_object *overlay_main)
{
    LOG_FUNCTION_START

    int result = 0;
    /* In overlay module, layer 0 corresponds to VIDEO_MAIN_LAYER and layer 1 corresponds to
       VIDEO_SUB_LAYER. */
    overlay_control_context_t   *ctx = (overlay_control_context_t *)dev;
    overlay_ctrl_t              *stage_sub = overlay_sub->stage_control();	
    overlay_ctrl_t              *stage_main = overlay_main->stage_control();

    if( (overlay_main->mposX >= overlay_sub->mposX) && (overlay_main->mposY >= overlay_sub->mposY) ) 
    {
        if (stage_sub->rotation == 90 || stage_sub->rotation == 270) {
            //Reverse height & width
            int mainHeight = overlay_main->mWidth;
            int mainWidth = overlay_main->mHeight;
                			
            /* Adjust transparent region height */
            /* if main is bottom-aligned to sub */
            if ((overlay_main->mposX+mainHeight) > (stage_sub->x+stage_sub->height)) {
                LOGV("%d: Changing TH from %d to %d", __LINE__, overlay_sub->mTransparent_Height, 
                     (overlay_sub->mTransparent_Height - ((overlay_main->mposX + mainHeight) - (stage_sub->x+stage_sub->height))));
                           
                overlay_sub->mTransparent_Height -= ((overlay_main->mposX + mainHeight) - (stage_sub->x+stage_sub->height));                        
            }
            /* if main is top-aligned to sub */
            else if (overlay_main->mposX < stage_sub->x){
                LOGV("%d: Changing TH from %d to %d", __LINE__, overlay_sub->mTransparent_Height, 
                     (overlay_sub->mTransparent_Height - (stage_sub->x - overlay_main->mposX)));

                overlay_sub->mTransparent_Height -= (stage_sub->x - overlay_main->mposX);                
            }
			
            /* Adjust transparent region width */
            /* if main is right-aligned to sub */
            if ((overlay_main->mposY+mainWidth) > (stage_sub->y+stage_sub->width)) {
                LOGV("%d: Changing TW from %d to %d", __LINE__, overlay_sub->mTransparent_Width, 
                     (overlay_sub->mTransparent_Width - ((overlay_main->mposY+mainWidth) - (stage_sub->y+stage_sub->width))));
           
                overlay_sub->mTransparent_Width -= ((overlay_main->mposY+mainWidth) - (stage_sub->y+stage_sub->width));                        
            }
            /* if main is left-aligned to sub */
            else if (overlay_main->mposY < stage_sub->y){
                LOGV("%d: Changing TW from %d to %d", __LINE__, overlay_sub->mTransparent_Width, 
                     (overlay_sub->mTransparent_Width - (stage_sub->y - overlay_main->mposX)));
			   
                overlay_sub->mTransparent_Width -= (stage_sub->y - overlay_main->mposY);                
            }

            result = osdm_plane_set_transparent_pos_and_dimension(overlay_sub->get_fd(), overlay_sub->mTransparent_posX,
                                                                  overlay_sub->mTransparent_posY, overlay_sub->mTransparent_Height,
                                                                  overlay_sub->mTransparent_Width
                                                                  );
        }
        else {
            /* Adjust transparancy width */
            /* If main-overlay is right-aligned to sub-overlay */
            if( (overlay_main->mposX+overlay_main->mWidth) > (stage_sub->x+stage_sub->width)) {
                LOGV("%d: Changing TW from %d to %d", __LINE__, overlay_sub->mTransparent_Width, 
                     (overlay_sub->mTransparent_Width - ((overlay_main->mposX + overlay_main->mWidth) - (stage_sub->x+stage_sub->width))));
                
                overlay_sub->mTransparent_Width -= ((overlay_main->mposX + overlay_main->mWidth) - (stage_sub->x+stage_sub->width));                
            }
            /* If main-overlay is left-aligned to sub-overlay */
            else if (overlay_main->mposX < stage_sub->x){
                LOGV("%d: Changing TW from %d to %d", __LINE__, overlay_sub->mTransparent_Width, 
                     (overlay_sub->mTransparent_Width - stage_sub->x - overlay_main->mposX));

                overlay_sub->mTransparent_Width -= (stage_sub->x - overlay_main->mposX);                
            }
			
            /* Adjust transparancy height */
            /* If main overlay is bottom-algned to sub-overlay */
            if( (overlay_main->mposY+overlay_main->mHeight) > (stage_sub->y+stage_sub->height)) {
                LOGV("%d: Changing TH from %d to %d", __LINE__, overlay_sub->mTransparent_Height, 
                     (overlay_sub->mTransparent_Height - ((overlay_main->mposY + overlay_main->mHeight) - (stage_sub->y+stage_sub->height))));
                
                overlay_sub->mTransparent_Height -= ((overlay_main->mposY + overlay_main->mHeight) - (stage_sub->y+stage_sub->height));                
            }
            /* If main-overlay is top-aligned to sub-overlay */
            else if (overlay_main->mposY < stage_sub->y){
                LOGV("%d: Changing TH from %d to %d", __LINE__, overlay_sub->mTransparent_Height, 
                     (overlay_sub->mTransparent_Height - (stage_sub->y - overlay_main->mposY)));

                overlay_sub->mTransparent_Height -= (stage_sub->y - overlay_main->mposY);                
            }
            
            result = osdm_plane_set_transparent_pos_and_dimension(overlay_sub->get_fd(), overlay_sub->mTransparent_posX,
                                                                  overlay_sub->mTransparent_posY, overlay_sub->mTransparent_Width,
                                                                  overlay_sub->mTransparent_Height);
        }
        LOGV("Trans_after loc=%dx%d size=%dx%d", overlay_sub->mTransparent_posX, overlay_sub->mTransparent_posY,
             overlay_sub->mTransparent_Width, overlay_sub->mTransparent_Height);

    }
    // Reset the transparency values once set.
    overlay_sub->mTransparent_posX = 0;
    overlay_sub->mTransparent_posY = 0;
    overlay_sub->mTransparent_Width = 0;
    overlay_sub->mTransparent_Height = 0;
    return result ;
}

/*         
        ys1      sub_plane
        ------------------------
     xs1|                      |xs1
        |  ym1   main_plane    |
        |    ---------------   |
        | xm1|             |xm2|
        |    |             |   |
        |    |             |   |
        |    |             |   |
        |    |             |   |
        |    ---------------   |
        | ym2                  |
        ------------------------
        ys2
*/
bool overlay_IsMainPlaneOverlapping (overlay_object *overlay_main, overlay_object *overlay_sub, uint32_t rotation)
{
    unsigned int xs1, xs2, xm1, xm2, ys1, ys2, ym1, ym2, mainHeight, mainWidth, subHeight, subWidth;
    
    if (rotation == 90 || rotation == 270) {
        //Reverse height & width
        mainHeight = overlay_main->mWidth;
        mainWidth = overlay_main->mHeight;

        subHeight = overlay_sub->mWidth;
        subWidth = overlay_sub->mHeight;
    } else {
        mainHeight = overlay_main->mHeight;
        mainWidth = overlay_main->mWidth;

        subHeight = overlay_sub->mHeight;
        subWidth = overlay_sub->mWidth;
    }

    xs1 = overlay_sub->mposX;
    xs2 = overlay_sub->mposX+subWidth;
    xm1 = overlay_main->mposX;
    xm2 = overlay_main->mposX+mainWidth;

    ys1 = overlay_sub->mposY;
    ys2 = overlay_sub->mposY+subHeight;
    ym1 = overlay_main->mposY;
    ym2 = overlay_main->mposY+mainHeight;
    
    /* Checking if main plane is not overlapping with the sub plane */
    /* Two planes are not overlapping if the following condition is satisfied:
       If xs1 and xs2 of main overlay is completely out of xs1 and xs2 of sub overlay OR
          xy1 and xs2 of main overlay is completely out of xy1 and xy2 of sub overlay
       */
    if ((xm1 > xs2 || xm2 < xs1) ||
        (ym1 > ys2 || ym2 < ys1))
        return false;
    else
        return true;
}

static int overlay_commit(struct overlay_control_device_t *dev,
                          overlay_t* overlay) 
{
    LOG_FUNCTION_START

    int result = 0;
    int dirty = 0;
    bool bIsTransparencySet = false ;

    overlay_object *overlay_obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *control = overlay_obj->control();
    overlay_ctrl_t *stage = overlay_obj->stage_control();

    LOGV("overlay_commit: pos = %dx%d, size = %dx%d, rotation = %d",stage->x, stage->y, stage->width, stage->height, stage->rotation);

    /* If we rotate 90 or 270 flip the width and height because the osdm expects the width and height before the rotation */
    if (stage->rotation == 90 || stage->rotation == 270) {
        int tmp = stage->width;
        stage->width = stage->height;
        stage->height = tmp;
    }

    //set transparency if already set
    if(overlay_obj->mTransparent_Width > 0) {
        // validate the transparency params.
        overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
        // In overlay module, layer 0 corresponds to VIDEO_MAIN_LAYER and layer 1 corresponds to
        // VIDEO_SUB_LAYER.
        overlay_object *overlay_main = static_cast<overlay_object *>(ctx->overlay[VIDEO_MAIN_LAYER]);
        bIsTransparencySet = overlay_IsMainPlaneOverlapping(overlay_main, overlay_obj, stage->rotation);
        if (bIsTransparencySet == true) {
            // TODO: handle rotation during transparency setting
            // Make transparency position absolute within plane.
            // if main layer is lying below sub layer start position, make sub layer as transparent.
            overlay_obj->mTransparent_posX -= stage->x;
            overlay_obj->mTransparent_posY -= stage->y;
            if (stage->rotation == 90 || stage->rotation == 270) {
                int tmp = overlay_obj->mTransparent_Width;
                overlay_obj->mTransparent_Width = overlay_obj->mTransparent_Height;
                overlay_obj->mTransparent_Height = tmp;
            }
        } else {
            overlay_obj->mTransparent_posX = 0;
            overlay_obj->mTransparent_posY = 0;
            overlay_obj->mTransparent_Width = 0;
            overlay_obj->mTransparent_Height = 0;
        }
    }

    /* Use decoder and scaler in pipe-line mode if this is an overlay of type "scaling decoder" */

    if (overlay_obj->mSharedData->overlayType == OVERLAY_TYPE_SCALING_DECODER) {
        /* If there is a pending scale request don't change the overlay position */
        if (overlay_obj->pending_scale_request) {
            LOGV("overlay_commit: pending scale request, abort");
            return 0;
        }

        /* set x and y and take the rotation into account */
        if (stage->rotation == 90 || stage->rotation == 270) {
            stage->x = stage->x + ((stage->height - overlay_obj->mSharedData->outputHeight) / 2);
            stage->y = stage->y + ((stage->width - overlay_obj->mSharedData->outputWidth) / 2);
        }
        else {
            stage->x = stage->x + ((stage->width - overlay_obj->mSharedData->outputWidth) / 2);
            stage->y = stage->y + ((stage->height - overlay_obj->mSharedData->outputHeight) / 2);
        }

        /* find the required scaled size */
        uint32_t scaled_width, scaled_height;
        overlay_get_64aligned_scale(stage->width, stage->height, &scaled_width, &scaled_height);
        LOGV("scaled_size = %dx%d, input_size = %dx%d",scaled_width, scaled_height, overlay_obj->get_input_width(), overlay_obj->get_input_height());
        if (scaled_width != overlay_obj->get_input_width() &&
            scaled_height != overlay_obj->get_input_height()) 
        {
            //LOGV("overlay_commit: sending scale request for %dx%d", scaled_width, scaled_height);
            if (overlay_sendScaleRequest(scaled_width, scaled_height) < 0) {
                LOGE("overlay_commit: error sending scale request");
                return -1;
            }
            overlay_obj->pending_scale_request = 1;
            return 0;
        }

        dirty = (control->x != stage->x ||
                 control->y != stage->y || 
                 control->rotation != stage->rotation ||
                 control->flip_h != stage->flip_h ||
                 control->flip_v != stage->flip_v);
    }
    else {
        // We need to handle the scaling (by using the post-processor stand-alone mode)

        unsigned int requested_width = stage->width;
        unsigned int requested_height = stage->height;

        // by default always align to 8 pixels
        unsigned int pix_align = 8;

        // set 64 pixels alignment when using the gdma rotator
        if (stage->rotation > 0) {
            pix_align = 64;
        }

        /* the post-processor allows up scaling of up to 3 times per dimension ((d*3 - 2) for the height) */
        if (stage->width > overlay_obj->mSharedData->inputWidth*3) {
            stage->width = overlay_obj->mSharedData->inputWidth*3;
        }
        if (stage->height > overlay_obj->mSharedData->inputHeight*3-2) {
            stage->height = overlay_obj->mSharedData->inputHeight*3-2;
        }

        // align width and height to pix_align
        stage->width &= 0xffffffff-pix_align+1;
        stage->height &= 0xffffffff-pix_align+1;

        /* do not allow upscale of one dimension and downscale of the other */
        if (stage->width > overlay_obj->mSharedData->inputWidth && stage->height < overlay_obj->mSharedData->inputHeight) {
            stage->width = overlay_obj->mSharedData->inputWidth&(0xffffffff-pix_align+1);
        }
        else if (stage->width < overlay_obj->mSharedData->inputWidth && stage->height > overlay_obj->mSharedData->inputHeight) {
            stage->height = overlay_obj->mSharedData->inputHeight&(0xffffffff-pix_align+1);
        }

        /* set x and y and take the rotation into account */
        if (stage->rotation == 90 || stage->rotation == 270) {
            if(bIsTransparencySet == true) {
                int diffx, diffy;
                diffx = (requested_height - (stage->height&(0xffffffff-pix_align+1)))/2;
                diffy = (requested_width - (stage->width&(0xffffffff-pix_align+1)))/2;
                stage->x += diffx;
                stage->y += diffy;

                if (overlay_obj->mTransparent_posX >= diffx)
                    overlay_obj->mTransparent_posX -= diffx;

                if (overlay_obj->mTransparent_posY >= diffy)
                    overlay_obj->mTransparent_posY -= diffy;
                LOGV("trans newx=%d", overlay_obj->mTransparent_posX);
            } else {
                // we need 64 pixels alignment when we use the gdma rotator
                stage->x += (requested_height - (stage->height&(0xffffffff-pix_align+1)))/2;
                stage->y += (requested_width - (stage->width&(0xffffffff-pix_align+1)))/2;
            }
        } else {
            if(bIsTransparencySet == true) {
                int diffx, diffy;
                diffx = (requested_width - (stage->width&(0xffffffff-pix_align+1)))/2;
                diffy = (requested_height - (stage->height&(0xffffffff-pix_align+1)))/2;
                stage->x += diffx;
                stage->y += diffy;
                if (overlay_obj->mTransparent_posX >= diffx)
                    overlay_obj->mTransparent_posX -= diffx;
                
                if (overlay_obj->mTransparent_posY >= diffy)
                    overlay_obj->mTransparent_posY -= diffy;
                LOGV("trans newx=%d", overlay_obj->mTransparent_posX);
            } else{
                stage->x += (requested_width - (stage->width&(0xffffffff-pix_align+1)))/2;
                stage->y += (requested_height - (stage->height&(0xffffffff-pix_align+1)))/2;
            }
        }

        dirty = (control->x != stage->x ||
                 control->y != stage->y ||
                 control->rotation != stage->rotation ||
                 control->width != stage->width ||
                 control->height != stage->height ||
                 control->flip_h != stage->flip_h ||
                 control->flip_v != stage->flip_v);
        LOGV("Transparency after scale stagesize=%dx%d location=%dx%d", stage->width, stage->height,stage->x,stage->y);
    }


    //set transparency if already set
    if(bIsTransparencySet == true) {
        overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
        if (overlay_setTransparency (dev, overlay_obj, static_cast<overlay_object *>(ctx->overlay[VIDEO_MAIN_LAYER])) != 0)
        {
            LOGE("Failed set transparency !!!");
        }
    }

    /* set the real parameters below (only if something has changed): */
    if (dirty) 
    {
        sem_wait(&overlay_obj->mSharedData->mutex);

        control->x = stage->x;
        control->y = stage->y;
        control->rotation = stage->rotation;
        control->flip_h = stage->flip_h;
        control->flip_v = stage->flip_v;

        /* in case of an overlay of type scaling_decoder, we never set the width and height,
    		   this was set on the creation of the overlay and we change it only when creating a new overlay */

        if (overlay_obj->mSharedData->overlayType != OVERLAY_TYPE_SCALING_DECODER) {
            // we make the scaling so update the overlay
            control->width = stage->width;
            control->height = stage->height;
            LOGV("doing post-processor scaling to size %dx%d",control->width, control->height);
        }

        /* disable this overlay before calling commit (enable when queueing a new frame) */
        fb_overlay_disable(overlay_obj->get_fd());

        /* commit all the parameters to the overlay plane */
        result = overlay_commit_parameters(overlay_obj);

        if (overlay_obj->mSharedData->overlayType != OVERLAY_TYPE_SCALING_DECODER) {
            // update the data module about the new output size
            overlay_obj->mSharedData->outputWidth = control->width;
            overlay_obj->mSharedData->outputHeight = control->height;
            overlay_obj->mSharedData->frameSize = get_frame_size(overlay_obj->format, control->width, control->height);
        }

        // flip is done by the post-processor on frame queue so update shared data struct 
        overlay_obj->mSharedData->flip_h = control->flip_h;
        overlay_obj->mSharedData->flip_v = control->flip_v;

        overlay_obj->mSharedData->allow_display = 1;
        overlay_obj->mSharedData->do_enable_on_next_queue = 1;

        sem_post(&overlay_obj->mSharedData->mutex);
    } else {
        if (!overlay_obj->mSharedData->allow_display) { 
            overlay_obj->mSharedData->allow_display = 1;
            overlay_obj->mSharedData->do_enable_on_next_queue = 1;
        }
        LOGV("overlay_commit: nothing to be done");
    }

    LOG_FUNCTION_END
    return result;
}

static int overlay_control_close(struct hw_device_t *dev) 
{
    LOG_FUNCTION_START

    struct overlay_control_context_t* ctx = (struct overlay_control_context_t*)dev;
    if (ctx) {
        /* free all resources associated with this device here
         * in particular the overlay_handle_t, outstanding overlay_t, etc...
         */
        for (int i=0 ; i<4 ;i++) {
            if (ctx->overlay[i]) {
                overlay_destroyOverlay((overlay_control_device_t*)dev, ctx->overlay[i]);
                delete ctx->overlay[i];
            }
            ctx->overlay[i] = NULL;
        }
        free(ctx);
    }

    LOG_FUNCTION_END
    return 0;
}

// toggleVideoOverlay method is used to toggle two video overlays namely VIDEO_SUB_LAYER
// and VIDEO_MAIN_LAYER.
static int overlay_toggleVideoOverlay(struct overlay_control_device_t *dev)
{
    LOG_FUNCTION_START

    unsigned int tmpX, tmpY, tmpW, tmpH;
    int result = 0;

    LOGV("overlay_toggleVideoOverlay.");

    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;

    if (ctx->overlay[VIDEO_MAIN_LAYER] == NULL || ctx->overlay[VIDEO_SUB_LAYER] == NULL) {
        LOGE("No two overlays to toggle!!");
        return -1;
    }

    // In overlay module, layer 0 corresponds to VIDEO_MAIN_LAYER and layer 1 corresponds to
    // VIDEO_SUB_LAYER.
    overlay_object *overlay_main = static_cast<overlay_object *>(ctx->overlay[VIDEO_MAIN_LAYER]);
    overlay_object *overlay_sub = static_cast<overlay_object *>(ctx->overlay[VIDEO_SUB_LAYER]);

    //swap position and width params of main and sub video layers.
    tmpX = overlay_sub->mposX;
    tmpY = overlay_sub->mposY;
    tmpW = overlay_sub->mWidth;
    tmpH = overlay_sub->mHeight;
    overlay_sub->mposX = overlay_main->mposX;
    overlay_sub->mposY = overlay_main->mposY;
    overlay_sub->mWidth = overlay_main->mWidth;
    overlay_sub->mHeight = overlay_main->mHeight;
    overlay_main->mposX = tmpX;
    overlay_main->mposY = tmpY;
    overlay_main->mWidth = tmpW;
    overlay_main->mHeight = tmpH;


    overlay_sub->mTransparent_posX = 0;
    overlay_sub->mTransparent_posY = 0;
    overlay_sub->mTransparent_Width = 0;
    overlay_sub->mTransparent_Height = 0;

    overlay_ctrl_t *stage = overlay_sub->stage_control();

    //if main layer is layer is lying below and within sub layer, make sub layer as transparent.
    if (overlay_IsMainPlaneOverlapping(overlay_main, overlay_sub, stage->rotation) == true){
        overlay_sub->mTransparent_posX = overlay_main->mposX;
        overlay_sub->mTransparent_posY = overlay_main->mposY;
        overlay_sub->mTransparent_Width = overlay_main->mWidth;
        overlay_sub->mTransparent_Height = overlay_main->mHeight;
    }
    else {
        // any missing conditions
    }

    // Set the size and rotation for main and sub video layers
    if (osdm_plane_set_size_and_rotation(overlay_main->get_fd(), overlay_main->mWidth, overlay_main->mHeight, 0) < 0) {
        LOGE("toggle - cannot set size and rotation\n");
        close(overlay_main->get_fd());
        return -1;
    }

    if (osdm_plane_set_position(overlay_main->get_fd(), overlay_main->mposX, overlay_main->mposY) < 0) {
        LOGE("toggle - cannot set position\n");
        close(overlay_main->get_fd() );
        return -1;
    }

    if (osdm_plane_set_size_and_rotation(overlay_sub->get_fd(), overlay_sub->mWidth, overlay_sub->mHeight, 0) < 0) {
        LOGE("toggle - cannot set size and rotation\n");
        close(overlay_main->get_fd() );
        return -1;
    }

    if (osdm_plane_set_position(overlay_sub->get_fd(), overlay_sub->mposX, overlay_sub->mposY) < 0) {
        LOGE("toggle - cannot set position\n");
        close(overlay_main->get_fd() );
        return -1;
    }

    // Now set the position to commit
    if(overlay_setPosition(dev, overlay_main, overlay_main->mposX, overlay_main->mposY, overlay_main->mWidth, overlay_main->mHeight) < 0) {
        LOGE("toggleVideoOverlay failed for MAIN VIDEO LAYER");
        return -1;
    }

    if(overlay_setPosition(dev, overlay_sub, overlay_sub->mposX, overlay_sub->mposY, overlay_sub->mWidth, overlay_sub->mHeight) < 0) {
        LOGE("toggleVideoOverlay failed for SUB VIDEO LAYER");
        return -1;
    }

    // Commit the changes for main and sub video layers
    overlay_commit(dev, overlay_main);
    overlay_commit(dev, overlay_sub);

    LOG_FUNCTION_END
    return result;
}

 
// ****************************************************************************
// Data module
// ****************************************************************************

int overlay_initialize(struct overlay_data_device_t *dev,
        overlay_handle_t handle)
{
    LOG_FUNCTION_START
    /* 
     * overlay_handle_t should contain all the information to "inflate" this
     * overlay. Typically it'll have a file descriptor, informations about
     * how many buffers are there, etc...
     * It is also the place to mmap all buffers associated with this overlay
     * (see getBufferAddress).
     * 
     * NOTE: this function doesn't take ownership of overlay_handle_t
	 *   
     */

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;

    ctx->fd = handle_get_fd(handle);
    ctx->shared_fd = handle_get_shared_fd(handle);
    ctx->format = handle_get_format(handle);
    ctx->visible = 0;

    ctx->shared_data = get_shared_data(ctx->shared_fd);
    if (ctx->shared_data == NULL) {
        LOGE("overlay_initialize: cannot get shared data");
        return -1;
    }

    LOGV("overlay_initialize: format = %d, input = %dx%d, output = %dx%d, allow_display = %d", 
         ctx->format, 
         ctx->shared_data->inputWidth, ctx->shared_data->inputHeight,
         ctx->shared_data->outputWidth, ctx->shared_data->outputHeight, 
         ctx->shared_data->allow_display);

    if (fb_overlay_init_double_buffer(ctx->fd, &(ctx->double_buffer)) != 0) 
        return -1;

    /* setup post processor */
    if (ctx->shared_data->overlayType != OVERLAY_TYPE_SCALING_DECODER) 
    {
        ctx->pp = new PostProcessor();

        if (ctx->pp->init() != 0) {
            LOGE("pp->init() failed");
            return -1;
        }

        // this will allocate the input frame
        if (ctx->pp->allocInputFrame(ctx->format, ctx->shared_data->inputWidth, ctx->shared_data->inputHeight) != 0) {
            LOGE("pp->setInputFrame() failed");
            return -1;
        }
    }

    LOG_FUNCTION_END
    return 0;
}

// 'buffer' is of type dspg_buffer_t
int overlay_dequeueBuffer(struct overlay_data_device_t *dev,
			  overlay_buffer_t* buffer) 
{
    LOG_FUNCTION_START

    /* blocks until a buffer is available and return an opaque structure
    * representing this buffer.
    */
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;

    if (!ctx->shared_data->is_alive) return -1;

    sem_wait(&ctx->shared_data->mutex);

    int index = !ctx->double_buffer.active_buffer_index;

    ctx->double_buffer.buffers[index].bus_address = 0;
    ctx->double_buffer.buffers[index].virt_address = 0;
    ctx->double_buffer.buffers[index].allow_zero_copy = 1;

    *buffer = &ctx->double_buffer.buffers[index];

    sem_post(&ctx->shared_data->mutex);
    LOG_FUNCTION_END
    return 0;
}

// 'buffer' is of type dspg_buffer_t
int overlay_queueBuffer(struct overlay_data_device_t *dev,
                        overlay_buffer_t buffer)
{
    LOG_FUNCTION_START

    /* Mark this buffer for posting and recycle or free overlay_buffer_t. */
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    struct dspg_buffer_t *dspg_buffer = (struct dspg_buffer_t*)buffer;

    if (!ctx->shared_data->is_alive) return -1;

    sem_wait(&ctx->shared_data->mutex);

    int yoffset = dspg_buffer->index * ctx->shared_data->outputHeight;

    if (ctx->shared_data->allow_display) 
    {
        if (ctx->shared_data->inputWidth != ctx->shared_data->outputWidth ||
            ctx->shared_data->inputHeight != ctx->shared_data->outputHeight ||
            ctx->shared_data->flip_h || ctx->shared_data->flip_v) 
        {
            /* we need to make the scaling here 
      	     * input address: ctx->shared_data->dataInputBuffer
    	     * output address: osdm input buffer (osdm_input_phys_address) */
            if (dspg_buffer->bus_address != 0) 
            {
                // we got the bus address of the buffer
                LOGV("zero_copy: sending physical address 0x%x to the post-processor", dspg_buffer->bus_address);
                if (ctx->pp->setInputFrame(dspg_buffer->bus_address, 
                                           ctx->format, 
                                           ctx->shared_data->inputWidth,
                                           ctx->shared_data->inputHeight) != 0) 
                {
                    LOGE("overlay_queueBuffer: error with pp->setInputFrame");
                    sem_post(&ctx->shared_data->mutex);
                    return -1;
                }
            }

            void *osdm_input_phys_address = (void*)((unsigned int)ctx->double_buffer.phys_address + dspg_buffer->index * ctx->shared_data->frameSize);
            if (ctx->pp->setOutputFrame((unsigned int)osdm_input_phys_address, 
                                        ctx->format, 
                                        ctx->shared_data->outputWidth, 
                                        ctx->shared_data->outputHeight) != 0)
            {
                LOGE("overlay_queueBuffer: error with pp->setOutputFrame");
                sem_post(&ctx->shared_data->mutex);
                return -1;
            }

            if (ctx->pp->setHorizontalFlip(ctx->shared_data->flip_h) != 0)
            {
                LOGE("overlay_queueBuffer: error with pp->setHorizontalFlip");
                sem_post(&ctx->shared_data->mutex);
                return -1;
            }

            if (ctx->pp->setVerticalFlip(ctx->shared_data->flip_v) != 0)
            {
                LOGE("overlay_queueBuffer: error with pp->setVerticalFlip");
                sem_post(&ctx->shared_data->mutex);
                return -1;
            }
	
            if (ctx->pp->commitSettings() != 0)
            {
                LOGE("overlay_queueBuffer: error with pp->commitSettings");
                sem_post(&ctx->shared_data->mutex);
                return -1;
            }
	
            if (ctx->pp->execute() != 0)
            {
                LOGE("overlay_queueBuffer: error with pp->execute");
                sem_post(&ctx->shared_data->mutex);
                return -1;
            } 

            // pan the buffer
            fb_overlay_pan_buffer(ctx->fd, yoffset);
        }
        else {
            /* no scaling */
            if (dspg_buffer->virt_address != 0) {
                /* use zero copy: the osdm will use this address as the input address */
                LOGV("zero_copy: sending virtual address 0x%x to the osdm", dspg_buffer->virt_address);
                fb_overlay_pan_buffer_to_address(ctx->fd, dspg_buffer->virt_address);
            }
            else {
                fb_overlay_pan_buffer(ctx->fd, yoffset);
            }
        }

        /* Enable overlay if required */
        if (ctx->shared_data->do_enable_on_next_queue) {
            LOGV("-------------> doing overlay enable <-----------------");
            if (fb_overlay_enable(ctx->fd) != 0) return -1;
            ctx->shared_data->do_enable_on_next_queue = 0;
        }

        ctx->double_buffer.active_buffer_index = dspg_buffer->index;
    }
    else {
        LOGV("overlay_queueBuffer: allow_display = 0");
    }

    sem_post(&ctx->shared_data->mutex);
    LOG_FUNCTION_END
    return 0;
}

int overlay_getBufferCount(struct overlay_data_device_t *dev)
{
    LOG_FUNCTION_START
    LOG_FUNCTION_END

    return 2;
}

int overlay_setCrop(struct overlay_data_device_t *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    LOGW("%s called. Ignoring", __func__);
    return 0;
}

int overlay_getCrop(struct overlay_data_device_t *dev, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
    LOGW("%s called. Ignoring", __func__);
    return 0;
}

int overlay_getOutputSize(struct overlay_data_device_t *dev, uint32_t* w, uint32_t* h)
{
    LOG_FUNCTION_START

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (!ctx->shared_data->is_alive) return -1;
    sem_wait(&ctx->shared_data->mutex);

    *w = ctx->shared_data->outputWidth;
    *h = ctx->shared_data->outputHeight;

    sem_post(&ctx->shared_data->mutex);
    return 0;
}

int overlay_resizeInput(struct overlay_data_device_t *dev, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_START

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (!ctx->shared_data->is_alive) return -1;
    sem_wait(&ctx->shared_data->mutex);

    /* verify new input size against post processor constrains */
    if (w & 0xf || 
        h & 0xf ||
        ctx->shared_data->outputWidth > w*3 ||
        ctx->shared_data->outputWidth < w/70 || 
        ctx->shared_data->outputHeight > h*3-2 ||
        ctx->shared_data->outputHeight < h/70 ||
        (ctx->shared_data->outputWidth > w && ctx->shared_data->outputHeight < h) ||
        (ctx->shared_data->outputWidth < w && ctx->shared_data->outputHeight > h)) 
    {
        LOGE("%s error: input size failed constrains check (new input = %dx%d, output = %dx%d)", __func__, 
             w, h, ctx->shared_data->outputWidth, ctx->shared_data->outputHeight);
        sem_post(&ctx->shared_data->mutex);
        return -1;
    }

    /* set new values */
    ctx->shared_data->inputWidth = w;
    ctx->shared_data->inputHeight = h;

    if (ctx->pp->allocInputFrame(ctx->format, ctx->shared_data->inputWidth, ctx->shared_data->inputHeight) != 0) {
        LOGE("%s error: pp->setInputFrame() failed", __func__);
        sem_post(&ctx->shared_data->mutex);
        return -1;
    }

    sem_post(&ctx->shared_data->mutex);
    return 0;
}

// 'buffer' is of type dspg_buffer_t
void *overlay_getBufferAddress(struct overlay_data_device_t *dev,
                               overlay_buffer_t buffer)
{
    LOG_FUNCTION_START
	
    void* address;
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    struct dspg_buffer_t *dspg_buffer = (struct dspg_buffer_t*)buffer;

    if (!ctx->shared_data->is_alive) return NULL;

    sem_wait(&ctx->shared_data->mutex);

    if (ctx->shared_data->inputWidth != ctx->shared_data->outputWidth ||
        ctx->shared_data->inputHeight != ctx->shared_data->outputHeight) 
    {
        // we need to do scaling, so give the buffer of the post processor to the application
        address = ctx->pp->getInputFrameVirtAddr();
    }
    else {
        // give the address of the osdm input buffer:
        // we have a double buffer so calculate the offset of this buffer according to this overlay format
        address = (void*)((unsigned int)ctx->double_buffer.mmap_address + dspg_buffer->index * ctx->shared_data->frameSize);
    }
	
    sem_post(&ctx->shared_data->mutex);

    LOG_FUNCTION_END
    return address;
}

static int overlay_data_close(struct hw_device_t *dev) 
{
    LOG_FUNCTION_START

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (ctx) {
        /* free all resources associated with this device here
         * in particular all pending overlay_buffer_t if needed.
	 * 
	 * NOTE: overlay_handle_t passed in initialize() is NOT freed and
	 * its file descriptors are not closed (this is the responsibility
	 * of the caller).
	 */
		
        if (ctx->shared_data) {
            release_shared_data(ctx->shared_data);
        }
        if (ctx->pp) {
            delete ctx->pp;
        }
        fb_overlay_deinit_double_buffer(ctx->fd, &(ctx->double_buffer));
        fb_overlay_disable(ctx->fd);

        free(ctx);
    }

    LOG_FUNCTION_END
    return 0;
}

int overlay_setParameter(struct overlay_data_device_t *dev, int param, int value)
{
    LOGW("[%s] called with param = %d, value = %d. Ignoring.", __func__, param, value);

    return 0;
}


/*****************************************************************************/

static int overlay_device_open(const struct hw_module_t* module, const char* name,
                               struct hw_device_t** device)
{
    LOG_FUNCTION_START

    int status = -EINVAL;
    if (!strcmp(name, OVERLAY_HARDWARE_CONTROL)) {
        struct overlay_control_context_t *dev;
        dev = (overlay_control_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        // create the overlay creation mutex
        if (sem_init(&dev->create_lock, 0, 1) != 0) {
            LOGE("error creating semaphore");
            return -1;
        }

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_control_close;
        
        dev->device.get = overlay_get;
        dev->device.createOverlay = overlay_createOverlay;
        dev->device.destroyOverlay = overlay_destroyOverlay;
        dev->device.setPosition = overlay_setPosition;
        dev->device.getPosition = overlay_getPosition;
        dev->device.setParameter = overlay_setParameter;
        dev->device.commit = overlay_commit;
        dev->device.stage = overlay_stage;
        dev->device.toggleVideoOverlay = overlay_toggleVideoOverlay;

        *device = &dev->device.common;
        status = 0;
    } else if (!strcmp(name, OVERLAY_HARDWARE_DATA)) {
        struct overlay_data_context_t *dev;
        dev = (overlay_data_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_data_close;
        
        dev->device.initialize = overlay_initialize;
        dev->device.dequeueBuffer = overlay_dequeueBuffer;
        dev->device.queueBuffer = overlay_queueBuffer;
        dev->device.getBufferAddress = overlay_getBufferAddress;
        dev->device.getBufferCount = overlay_getBufferCount;
        dev->device.setCrop = overlay_setCrop;
        dev->device.getCrop = overlay_getCrop;
        dev->device.resizeInput = overlay_resizeInput;
        dev->device.setParameter = overlay_setParameter;

        dev->device.getOutputSize = overlay_getOutputSize;
        
        *device = &dev->device.common;
        status = 0;
    }

    LOG_FUNCTION_END
    return status;
}


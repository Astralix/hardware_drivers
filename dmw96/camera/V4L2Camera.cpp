/*
**
** Copyright (C) 2009 0xlab.org - http://0xlab.org/
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
//#define LOG_NDEBUG 0
#define LOG_TAG "DSPG_V4L2Camera"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/time.h>

#include <linux/videodev.h>
#include "cutils/properties.h"

#include "V4L2Camera.h"

#include "JpegEncoder.h"

#include "include/linux/dmw96ciu_common.h" 

namespace android {

V4L2Camera::V4L2Camera ()
    : fd(-1),nQueued(0), nDequeued(0),
            m_gps_latitude(-1),
            m_gps_longitude(-1),
            m_gps_altitude(-1),
            m_gps_timestamp(-1),
            m_exif_orientation(-1),
            m_jpeg_thumbnail_width (0),
            m_jpeg_thumbnail_height(0),
            m_jpeg_quality(100),
            m_frame_counter(0)
{
    LOGV("%s :", __func__);
    videoIn = (struct vdIn *) calloc (1, sizeof (struct vdIn));
	memset(videoIn,0,sizeof (struct vdIn));

    videoIn->width = DEFAULT_PREVIEW_WIDTH;
    videoIn->height = DEFAULT_PREVIEW_HEIGHT;
    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 3 / 2);
    videoIn->formatIn = V4L2_PIX_FMT_NV12;
	videoIn->framerate = OV34_30_FPS;
	videoIn->isStreaming = false;
}

V4L2Camera::~V4L2Camera()
{
    LOGV("%s :", __func__);
    free(videoIn);
}

int V4L2Camera::Open(int CameraId)
{
    int ret;
    char *pCameraFileName;

    LOGV("%s :", __func__);

	if( fd != -1 )
	{
        LOGW("%s : Camera already opened", __func__);
		return NO_ERROR;
	}

    if ( CameraId == 0 ){
    	pCameraFileName = (char *)DSPG_FIRST_CAMERA_DEVICE;
    } else if ( CameraId == 1 ) {
    	pCameraFileName = (char *)DSPG_SECOND_CAMERA_DEVICE;
    } else {
        LOGE("%s : Unknown camera ID", __func__);
        return UNKNOWN_ERROR;
    }

    // Make the camera device open as non-blocking. as in ciu-test
    if ((fd = open(pCameraFileName, O_RDWR | O_NONBLOCK)) == -1)
    {
        LOGE("%s : ERROR opening V4L interface %s: %s", __func__,pCameraFileName,strerror(errno));
        return UNKNOWN_ERROR;
    }

	mCameraId = CameraId;
	return NO_ERROR;
}

int64_t V4L2Camera::getNowms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)(tv.tv_usec/1000) + tv.tv_sec * 1000ll;
}

int V4L2Camera::SetPreviewSize(int Width,int Height,int PixelFormat,int Framerate)
{

    videoIn->width = Width;
    videoIn->height = Height;
    videoIn->formatIn = PixelFormat;

	LOGV("Setting width=%d,height=%d,Framreate=%d",Width, Height, Framerate);
	if( videoIn->formatIn == V4L2_PIX_FMT_NV12)
	    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 3 / 2);
	else if( videoIn->formatIn == V4L2_PIX_FMT_NV16)
	    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 2);

	videoIn->framerate = Framerate;
 	//BS:: above XGA we support 15 and 7.5 fps
	if ( videoIn->height > 720 ) 
	{
		videoIn->framerate = 15;
	}
	return NO_ERROR;
}

int V4L2Camera::StartPreview ()
{
	int ret;

	if ( fd < 0 )
	{
        LOGE("%s: camera device is not opened",__func__);
        return UNKNOWN_ERROR;
	}

    if (videoIn->isStreaming) 
	{
        LOGE("%s: preview already started",__func__);
		return ALREADY_EXISTS;
	}

    ret = ioctl (fd, VIDIOC_QUERYCAP, &videoIn->cap);
    if (ret < 0)
    {
        LOGE("%s: unable to query device.",__func__);
        return UNKNOWN_ERROR;
    }

    if ((videoIn->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
        LOGE("%s: video capture not supported.",__func__);
        return UNKNOWN_ERROR;
    }

    if (!(videoIn->cap.capabilities & V4L2_CAP_STREAMING)) {
        LOGE("%s: Capture device does not support streaming i/o",__func__);
        return UNKNOWN_ERROR;
    }

    videoIn->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoIn->format.fmt.pix.width = videoIn->width;
    videoIn->format.fmt.pix.height = videoIn->height;
    videoIn->format.fmt.pix.pixelformat = videoIn->formatIn;
    videoIn->format.fmt.pix.field = V4L2_FIELD_INTERLACED;

    LOGV("Requested : Width = %d, Height = %d framesizeIn=%d",videoIn->format.fmt.pix.width,
																		videoIn->format.fmt.pix.height,
																		videoIn->framesizeIn);

    ret = ioctl(fd, VIDIOC_S_FMT, &videoIn->format);
    if (ret < 0) {
        LOGE("%s: VIDIOC_S_FMT Failed: %s",__func__,strerror(errno));
        return ret;
    }

    videoIn->width = videoIn->format.fmt.pix.width;
    videoIn->height = videoIn->format.fmt.pix.height;

    LOGV("Configured : Width = %d, Height = %d framesizeIn=%d",videoIn->width,
																videoIn->height,
																videoIn->framesizeIn);
    ret = InitFps();
    if (ret < 0) {
        LOGE("%s: InitFps Failed: %s",__func__, strerror(errno));
        return ret;
    }

	ret = InitMmap();

	if( ret != NO_ERROR)
	{
		LOGE("%s: InitMmap failed",__func__);
        return ret;
    }

	//start stream
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl (fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
		LOGE("StartStreaming: Unable to start capture: %s", strerror(errno));
        return ret;
    }

	LOGV("%s : Start Streaming", __func__);

    videoIn->isStreaming = true;

    m_frame_counter = 0;

    return ret;
}

void V4L2Camera::Close ()
{
    LOGV("%s :", __func__);
	if (fd != -1)
	{
		close(fd);
		fd = -1;
	}
}


//  Init mmap
int V4L2Camera::InitMmap()
{
    int ret;

    LOGI("%s :", __func__);

    /* Check if camera can handle NB_BUFFER buffers */
    videoIn->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoIn->rb.memory = V4L2_MEMORY_MMAP;
    videoIn->rb.count = NB_BUFFER;

    ret = ioctl(fd, VIDIOC_REQBUFS, &videoIn->rb);
    if (ret < 0)
    {
        LOGE("Init: VIDIOC_REQBUFS failed: %s", strerror(errno));
        return ret;
    }

	mPreviewHeap = new sp<MemoryHeapBase>[videoIn->rb.count];

    for (uint32_t i = 0; i < videoIn->rb.count; i++) {

        memset (&videoIn->buf, 0, sizeof (struct v4l2_buffer));

        videoIn->buf.index = i;
        videoIn->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoIn->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (fd, VIDIOC_QUERYBUF, &videoIn->buf);
        if (ret < 0) {
            LOGE("Init: Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

		mPreviewHeap[i] = new MemoryHeapBase(fd, videoIn->buf.length, 0, videoIn->buf.m.offset);
		videoIn->mem[i] = mPreviewHeap[i]->getBase();
#if 0
        videoIn->mem[i] = mmap (0,
               videoIn->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               videoIn->buf.m.offset);
#endif
        if (videoIn->mem[i] == MAP_FAILED) {
            LOGE("Init: Unable to map buffer #%d (%s)", i,strerror(errno));
            return UNKNOWN_ERROR;
        }

		/* get the physical address of this buffer */
		ret = ioctl(fd, VIDIOC_G_PHYSICAL, &videoIn->buf);
		if (ret) {
		        LOGE("Init: VIDIOC_G_PHYSICAL failed");
		        return UNKNOWN_ERROR;
		}
        videoIn->physical_mem[i] = videoIn->buf.reserved;

        ret = ioctl(fd, VIDIOC_QBUF, &videoIn->buf);
        if (ret < 0) {
            LOGE("Init: VIDIOC_QBUF Failed");
            return UNKNOWN_ERROR;
        }
		
		//BS:: put physical address to be used in other layers for ZERO_COPY
		if (videoIn->buf.length < GetFrameSize() + 16) {
			LOGE("Frame size doesn't include reserved memory for physical address! GetFrameSize()=%d, videoIn->buf.length=%d",GetFrameSize(),videoIn->buf.length);		
            return UNKNOWN_ERROR;
		}

		*(uint32_t*)((uint8_t *)videoIn->mem[i] + GetFrameSize()) = GetPhysicalAddress(i);
		*(uint32_t*)((uint8_t *)videoIn->mem[i] + GetFrameSize() + sizeof(unsigned int)) = i;

		LOGV("PhysicalAddress=0x%x, VirtAddress=0x%x, GetFrameSize()=%d, VirtAddrEnd=0x%x",
			GetPhysicalAddress(i), videoIn->mem[i], GetFrameSize(), videoIn->mem[i] + GetFrameSize());

        nQueued++;
    }

    return NO_ERROR;
}

int V4L2Camera::InitFps()
{
    int ret = 0;
    int framerate;
    struct v4l2_streamparm streamparm;
	struct v4l2_frmivalenum frmi; 

	frmi.pixel_format = videoIn->format.fmt.pix.pixelformat;

    LOGI("%s videoIn->framerate=%d", __func__,videoIn->framerate);
			
	frmi.width = videoIn->format.fmt.pix.width;
	frmi.height = videoIn->format.fmt.pix.height;
	frmi.index = 0;	
	while( ret == 0 )
	{
		ret = ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmi);
		if (ret != 0){
		    LOGE("VIDIOC_ENUM_FRAMEINTERVALS fail....");
		    break;
		}

		if(frmi.discrete.denominator/frmi.discrete.numerator == videoIn->framerate)
			break;

		frmi.index++;
	}

	if (ret == 0)
	{
		streamparm.parm.capture.timeperframe.numerator = frmi.discrete.numerator;
		streamparm.parm.capture.timeperframe.denominator = frmi.discrete.denominator;
		streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		ret = ioctl(fd, VIDIOC_S_PARM, &streamparm);
		if(ret != 0) {
		    LOGE("VIDIOC_S_PARM  Fail....");
		    return -1;
		}

		videoIn->framerate = streamparm.parm.capture.timeperframe.denominator;
	}
	else
	{
		LOGE("Failed to set frame reate ....");
	    return -1;
	}

    return NO_ERROR;
}

void V4L2Camera::Uninit()
{
    int ret;

    LOGV("%s :", __func__);
    videoIn->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoIn->buf.memory = V4L2_MEMORY_MMAP;

    /* Dequeue everything */
/*    int DQcount = nQueued - nDequeued;

    for (int i = 0; i < DQcount-1; i++) {
        ret = ioctl(fd, VIDIOC_DQBUF, &videoIn->buf);
        if (ret < 0)
            LOGE("Uninit: VIDIOC_DQBUF Failed");
    }
    nQueued = 0;
    nDequeued = 0;
*/
    nQueued = 0;
    nDequeued = 0;
    /* Unmap buffers */
    for (unsigned int i = 0; i < videoIn->rb.count; i++){
			mPreviewHeap[i]->dispose();
			mPreviewHeap[i].clear();
	}
	delete[] mPreviewHeap;

}

int V4L2Camera::StopPreview ()
{
    enum v4l2_buf_type type;
    int ret;

    LOGV("%s:%d", __func__,__LINE__);

    m_frame_counter = 0;

    if (videoIn->isStreaming) {
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ret = ioctl (fd, VIDIOC_STREAMOFF, &type);
        if (ret < 0) {
            LOGE("StopStreaming: Unable to stop capture: %s", strerror(errno));
            return ret;
        }

		//unmmap
		Uninit();
		//close device due to hardware limitaiton we always close device in stop preview
		//Close();

        videoIn->isStreaming = false;
	}
    return NO_ERROR;
}


/*BS:: Instead of copying buffer, pass it to the upper layers, the problem here is that we can run out of
 * available buffers if they are not released by camera service*/
int V4L2Camera::GrabPreviewFrame (unsigned char **pPreviewBuffer)
{
    int ret = -1;

    struct v4l2_buffer captureBuf;

#ifdef ENABLE_PROFILING
    m_frame_counter++;

    LOGD("VEncode: -> CamGrab, Num: %d, time=%lld", m_frame_counter, getNowms());
#endif

    // Poll for data on fd. And then go for ioctl DQBUF
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO (&fds);
    FD_SET (fd, &fds);

    /* Timeout. */
    tv.tv_sec = 0;
    tv.tv_usec = 200000;

    r = select (fd + 1, &fds, NULL, NULL, &tv);

    if (-1 == r) {
        LOGE("FAILURE - select\n");
#ifdef ENABLE_PROFILING
        m_frame_counter--;
#endif
        return ret;
    }

    if (0 == r) {
        LOGE("FAILURE - select timeout\n");
#ifdef ENABLE_PROFILING
        m_frame_counter--;
#endif
        return ret;
    }

	captureBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    captureBuf.memory = V4L2_MEMORY_MMAP;

    /* DQueue frame */
    ret = ioctl(fd, VIDIOC_DQBUF, &captureBuf);
    if (ret < 0) {
        LOGE("GrabPreviewFrame: VIDIOC_DQBUF Failed nDequeued=%d nQueued=%d",nDequeued,nQueued);
#ifdef ENABLE_PROFILING
        m_frame_counter--;
#endif
        return ret;
    }
    *pPreviewBuffer = (unsigned char *)videoIn->mem[captureBuf.index];

	nDequeued++;

    return captureBuf.index;
}

unsigned int V4L2Camera::GetPhysicalAddress(unsigned int buffer_index)
{
    if (buffer_index >= NB_BUFFER) return 0;

    return videoIn->physical_mem[buffer_index];
}

/*BS:: Release preview frame by index*/
void V4L2Camera::ReleasePreviewFrame ( int Index )
{
	int32_t ret;

    struct v4l2_buffer releaseBuf;

    releaseBuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    releaseBuf.memory = V4L2_MEMORY_MMAP;
    releaseBuf.index = Index;
    releaseBuf.flags = 0;

    /* Queue frame */
    ret = ioctl(fd, VIDIOC_QBUF, &releaseBuf);
    if (ret < 0) {
        LOGE("ReleasePreviewFrame: VIDIOC_QBUF Failed for index=%d nDequeued=%d nQueued=%d",Index,nDequeued,nQueued);
        return;
    }
	nQueued++;
}

/**
 * Converts semi-planar YUV420 as generated for camera preview into RGB565
 * format for use as an OpenGL ES texture. It assumes that both the input
 * and output data are contiguous and start at zero.
 * 
 * @param yuvs the array of YUV420 semi-planar data
 * @param rgbs an array into which the RGB565 data will be written
 * @param width the number of pixels horizontally
 * @param height the number of pixels vertically
 */
#if 0
#define PACK_SHORT_565(r,g,b)  ((((r)<<8)&0xf800)|(((g)<<3)&0x7C0)|((b)>>3))
void V4L2Camera::Convert(unsigned char * yuvs, unsigned char * rgbs ) 
{
    //the end of the luminance data
    int lumEnd = videoIn->width * videoIn->height;
	int buff[32];
	int counter = 0;
	int color;
	unsigned short *rgb_short = (unsigned short*)rgbs;

	struct timeval tv1, tv2;
	gettimeofday(&tv1, NULL);

	if ((int)yuvs&0x3 != 0) LOGW("yuvs is not aligned (%p)", yuvs);

	for (counter=0;counter<lumEnd;counter++)
	{
		if (!(counter%128)) {
			memcpy(buff, yuvs, (lumEnd-counter>128)?128:lumEnd-counter);
			yuvs+=128;
		}videoIn->rb
		color = ((unsigned char*)buff)[counter%128];
		rgb_short[counter] = PACK_SHORT_565(color,color,color);
	}

	gettimeofday(&tv2, NULL);
	LOGW("convert time = %ld", (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000); 
}
#endif

//we tackle the conversion two pixels at a time for greater speed
void V4L2Camera::Convert(unsigned char * yuvs, unsigned char * rgbs ) 
{

    //the end of the luminance data
    int lumEnd = videoIn->width * videoIn->height;
    //points to the next luminance value pair
    int lumPtr = 0;
    //points to the next chromiance value pair
    int chrPtr = lumEnd;
    //points to the next byte output pair of RGB565 value
    int outPtr = 0;
    //the end of the current luminance scanline
    int lineEnd = videoIn->width;

    while (true) {

        //skip back to the start of the chromiance values when necessary
        if (lumPtr == lineEnd) {
            if (lumPtr == lumEnd) break; //we've reached the end
            //division here is a bit expensive, but's only done once per scanline
            chrPtr = lumEnd + ((lumPtr  >> 1) / videoIn->width) * videoIn->width;
            lineEnd += videoIn->width;
        }

        //read the luminance and chromiance values
        int Y1 = yuvs[lumPtr++] & 0xff; 
        int Y2 = yuvs[lumPtr++] & 0xff; 
        int Cb = (yuvs[chrPtr++] & 0xff) - 128; 
        int Cr = (yuvs[chrPtr++] & 0xff) - 128;
        int R, G, B;

        //generate first RGB components
        B = Y1 + ((454 * Cb) >> 8);
        if(B < 0) B = 0; else if(B > 255) B = 255; 
        G = Y1 - ((88 * Cb + 183 * Cr) >> 8); 
        if(G < 0) G = 0; else if(G > 255) G = 255; 
        R = Y1 + ((359 * Cr) >> 8); 
        if(R < 0) R = 0; else if(R > 255) R = 255; 
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));

        //generate second RGB components
        B = Y2 + ((454 * Cb) >> 8);
        if(B < 0) B = 0; else if(B > 255) B = 255; 
        G = Y2 - ((88 * Cb + 183 * Cr) >> 8); 
        if(G < 0) G = 0; else if(G > 255) G = 255; 
        R = Y2 + ((359 * Cr) >> 8); 
        if(R < 0) R = 0; else if(R > 255) R = 255; 
        //NOTE: this assume little-endian encoding
        rgbs[outPtr++]  = (unsigned char) (((G & 0x3c) << 3) | (B >> 3));
        rgbs[outPtr++]  = (unsigned char) ((R & 0xf8) | (G >> 5));
    }
}

void V4L2Camera::setExifFixedAttribute()
{
    char property[PROPERTY_VALUE_MAX];

    //2 0th IFD TIFF Tags
    //3 Maker
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';
    //3 Model
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';
    //3 Software
    property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    //3 YCbCr Positioning
    mExifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    //2 0th IFD Exif Private Tags
    //3 F Number
    mExifInfo.fnumber.num = EXIF_DEF_FNUMBER_NUM;
    mExifInfo.fnumber.den = EXIF_DEF_FNUMBER_DEN;
    //3 Exposure Program
    mExifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    //3 Exif Version
    memcpy(mExifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(mExifInfo.exif_version));
    //3 Aperture
    uint32_t av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = av*EXIF_DEF_APEX_DEN;
    mExifInfo.aperture.den = EXIF_DEF_APEX_DEN;
    //3 Maximum lens aperture
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;
    //3 Lens Focal Length

	//BS::TBD
	if (mCameraId == CAMERA_ID_BACK)
	    mExifInfo.focal_length.num = DEFAULT_FOCAL_LENGTH;
	else
	    mExifInfo.focal_length.num = DEFAULT_FOCAL_LENGTH;

    mExifInfo.focal_length.den = EXIF_DEF_FOCAL_LEN_DEN;
    //3 User Comments
    strcpy((char *)mExifInfo.user_comment, EXIF_DEF_USERCOMMENTS);
    //3 Color Space information
    mExifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    //3 Exposure Mode
    mExifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    //2 0th IFD GPS Info Tags
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));

    //2 1th IFD TIFF Tags
    mExifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    mExifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

void V4L2Camera::setExifChangedAttribute()
{
    //2 0th IFD TIFF Tags
    //3 Width
    mExifInfo.width = videoIn->width;
    //3 Height
    mExifInfo.height = videoIn->height;
    //3 Orientation

    switch (m_exif_orientation) {
    case 0:
        mExifInfo.orientation = EXIF_ORIENTATION_UP;
        break;
    case 90:
        mExifInfo.orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        mExifInfo.orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        mExifInfo.orientation = EXIF_ORIENTATION_270;
        break;
    default:
        mExifInfo.orientation = EXIF_ORIENTATION_UP;
        break;
    }
    //3 Date time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)mExifInfo.date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);

    //2 0th IFD Exif Private Tags
    //3 Exposure Time
#if 0
    int shutterSpeed = fimc_v4l2_g_ctrl(m_cam_fd,
                                            V4L2_CID_CAMERA_GET_SHT_TIME);
    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (shutterSpeed < 0) {
        LOGE("%s: error %d getting shutterSpeed, camera_id = %d, using 100",
             __func__, shutterSpeed, m_camera_id);
        shutterSpeed = 100;
    }
#else
    int shutterSpeed = 100;
#endif
    mExifInfo.exposure_time.num = 1;
    // x us -> 1/x s */
    mExifInfo.exposure_time.den = (uint32_t)(1000000 / shutterSpeed);

    //3 ISO Speed Rating
#if 0
    int iso = fimc_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_GET_ISO);
    /* TBD - front camera needs to be fixed to support this g_ctrl,
       it current returns a negative err value, so avoid putting
       odd value into exif for now */
    if (iso < 0) {
        LOGE("%s: error %d getting iso, camera_id = %d, using 100",
             __func__, iso, m_camera_id);
        iso = ISO_100;
    }
    switch(iso) {
        case ISO_50:
            mExifInfo.iso_speed_rating = 50;
            break;
        case ISO_100:
            mExifInfo.iso_speed_rating = 100;
            break;
        case ISO_200:
            mExifInfo.iso_speed_rating = 200;
            break;
        case ISO_400:
            mExifInfo.iso_speed_rating = 400;
            break;
        case ISO_800:
            mExifInfo.iso_speed_rating = 800;
            break;
        case ISO_1600:
            mExifInfo.iso_speed_rating = 1600;
            break;
        default:
            mExifInfo.iso_speed_rating = 100;
            break;
    }
#else
	mExifInfo.iso_speed_rating = 100;
#endif
    uint32_t av, tv, bv, sv, ev;
    av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num / mExifInfo.fnumber.den);
    tv = APEX_EXPOSURE_TO_SHUTTER((double)mExifInfo.exposure_time.num / mExifInfo.exposure_time.den);
    sv = APEX_ISO_TO_FILMSENSITIVITY(mExifInfo.iso_speed_rating);
    bv = av + tv - sv;
    ev = av + tv;
    LOGV("Shutter speed=%d us, iso=%d\n", shutterSpeed, mExifInfo.iso_speed_rating);
    LOGV("AV=%d, TV=%d, SV=%d\n", av, tv, sv);

    //3 Shutter Speed
    mExifInfo.shutter_speed.num = tv*EXIF_DEF_APEX_DEN;
    mExifInfo.shutter_speed.den = EXIF_DEF_APEX_DEN;
    //3 Brightness
    mExifInfo.brightness.num = bv*EXIF_DEF_APEX_DEN;
    mExifInfo.brightness.den = EXIF_DEF_APEX_DEN;
    //3 Exposure Bias
#if 0
    if (m_params->scene_mode == SCENE_MODE_BEACH_SNOW) {
        mExifInfo.exposure_bias.num = EXIF_DEF_APEX_DEN;
        mExifInfo.exposure_bias.den = EXIF_DEF_APEX_DEN;
    } else {
        mExifInfo.exposure_bias.num = 0;
        mExifInfo.exposure_bias.den = 0;
    }
#else
    mExifInfo.exposure_bias.num = 0;
    mExifInfo.exposure_bias.den = 0;
#endif
    //3 Metering Mode
#if 0
    switch (m_params->metering) {
    case METERING_SPOT:
        mExifInfo.metering_mode = EXIF_METERING_SPOT;
        break;
    case METERING_MATRIX:
        mExifInfo.metering_mode = EXIF_METERING_AVERAGE;
        break;
    case METERING_CENTER:
        mExifInfo.metering_mode = EXIF_METERING_CENTER;
        break;
    default :
        mExifInfo.metering_mode = EXIF_METERING_AVERAGE;
        break;
    }
#else
	mExifInfo.metering_mode = EXIF_METERING_AVERAGE;
#endif
    //3 Flash
#if 0
    int flash = fimc_v4l2_g_ctrl(m_cam_fd, V4L2_CID_CAMERA_GET_FLASH_ONOFF);
    if (flash < 0)
        mExifInfo.flash = EXIF_DEF_FLASH;
    else
        mExifInfo.flash = flash;
#else
	mExifInfo.flash = EXIF_DEF_FLASH;
#endif
#if 0
    //3 White Balance
    if (m_params->white_balance == WHITE_BALANCE_AUTO)
        mExifInfo.white_balance = EXIF_WB_AUTO;
    else
        mExifInfo.white_balance = EXIF_WB_MANUAL;
#else
	mExifInfo.white_balance = EXIF_WB_AUTO;
#endif

#if 0	
    //3 Scene Capture Type
    switch (m_params->scene_mode) {
    case SCENE_MODE_PORTRAIT:
        mExifInfo.scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        mExifInfo.scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHTSHOT:
        mExifInfo.scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        mExifInfo.scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }
#else
	mExifInfo.scene_capture_type = EXIF_SCENE_STANDARD;
#endif

    //2 0th IFD GPS Info Tags
    if (m_gps_latitude != 0 && m_gps_longitude != 0) {
        if (m_gps_latitude > 0)
            strcpy((char *)mExifInfo.gps_latitude_ref, "N");
        else
            strcpy((char *)mExifInfo.gps_latitude_ref, "S");

        if (m_gps_longitude > 0)
            strcpy((char *)mExifInfo.gps_longitude_ref, "E");
        else
            strcpy((char *)mExifInfo.gps_longitude_ref, "W");

        if (m_gps_altitude > 0)
            mExifInfo.gps_altitude_ref = 0;
        else
            mExifInfo.gps_altitude_ref = 1;

        double latitude = fabs(m_gps_latitude / 10000.0);
        double longitude = fabs(m_gps_longitude / 10000.0);
        double altitude = fabs(m_gps_altitude / 100.0);

        mExifInfo.gps_latitude[0].num = (uint32_t)latitude;
        mExifInfo.gps_latitude[0].den = 1;
        mExifInfo.gps_latitude[1].num = (uint32_t)((latitude - mExifInfo.gps_latitude[0].num) * 60);
        mExifInfo.gps_latitude[1].den = 1;
        mExifInfo.gps_latitude[2].num = (uint32_t)((((latitude - mExifInfo.gps_latitude[0].num) * 60)
                                        - mExifInfo.gps_latitude[1].num) * 60);
        mExifInfo.gps_latitude[2].den = 1;

        mExifInfo.gps_longitude[0].num = (uint32_t)longitude;
        mExifInfo.gps_longitude[0].den = 1;
        mExifInfo.gps_longitude[1].num = (uint32_t)((longitude - mExifInfo.gps_longitude[0].num) * 60);
        mExifInfo.gps_longitude[1].den = 1;
        mExifInfo.gps_longitude[2].num = (uint32_t)((((longitude - mExifInfo.gps_longitude[0].num) * 60)
                                        - mExifInfo.gps_longitude[1].num) * 60);
        mExifInfo.gps_longitude[2].den = 1;

        mExifInfo.gps_altitude.num = (uint32_t)altitude;
        mExifInfo.gps_altitude.den = 1;

        struct tm tm_data;
        gmtime_r(&m_gps_timestamp, &tm_data);
        mExifInfo.gps_timestamp[0].num = tm_data.tm_hour;
        mExifInfo.gps_timestamp[0].den = 1;
        mExifInfo.gps_timestamp[1].num = tm_data.tm_min;
        mExifInfo.gps_timestamp[1].den = 1;
        mExifInfo.gps_timestamp[2].num = tm_data.tm_sec;
        mExifInfo.gps_timestamp[2].den = 1;
        snprintf((char*)mExifInfo.gps_datestamp, sizeof(mExifInfo.gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        mExifInfo.enableGps = true;
    } else {
        mExifInfo.enableGps = false;
    }
    //2 1th IFD TIFF Tags
    mExifInfo.widthThumb = m_jpeg_thumbnail_width;
    mExifInfo.heightThumb = m_jpeg_thumbnail_height;
}

int V4L2Camera::getExif(unsigned char *pExifDst, unsigned char *pThumbSrc,int thumbSize)
{
	int exifSize = -1;

    if (thumbSize <= 0) {
		mExifInfo.enableThumb = false;
	} else {
		mExifInfo.enableThumb = true;
	}
	setExifChangedAttribute();

	LOGV("%s: calling jpgEnc.makeExif, mExifInfo.width set to %d, height to %d\n",
	     __func__, mExifInfo.width, mExifInfo.height);

	int res = mJpgEnc.makeExif(pExifDst, &mExifInfo, (unsigned int *)&exifSize, pThumbSrc,thumbSize);

	if ( res != NO_ERROR ){
		LOGE("Failed to create EXIF");
		exifSize = -1;
	}
	LOGV("%s:%d	exifSize = %d",__func__,__LINE__,exifSize);

    return exifSize;

}

//======================================================================

int V4L2Camera::setGPSLatitude(const char *gps_latitude)
{
    double conveted_latitude = 0;
    LOGV("%s(gps_latitude(%s))", __func__, gps_latitude);
    if (gps_latitude == NULL)
        m_gps_latitude = 0;
    else {
        conveted_latitude = atof(gps_latitude);
        m_gps_latitude = (long)(conveted_latitude * 10000 / 1);
    }

    LOGV("%s(m_gps_latitude(%ld))", __func__, m_gps_latitude);
    return 0;
}

int V4L2Camera::setGPSLongitude(const char *gps_longitude)
{
    double conveted_longitude = 0;
    LOGV("%s(gps_longitude(%s))", __func__, gps_longitude);
    if (gps_longitude == NULL)
        m_gps_longitude = 0;
    else {
        conveted_longitude = atof(gps_longitude);
        m_gps_longitude = (long)(conveted_longitude * 10000 / 1);
    }

    LOGV("%s(m_gps_longitude(%ld))", __func__, m_gps_longitude);
    return 0;
}

int V4L2Camera::setGPSAltitude(const char *gps_altitude)
{
    double conveted_altitude = 0;
    LOGV("%s(gps_altitude(%s))", __func__, gps_altitude);
    if (gps_altitude == NULL)
        m_gps_altitude = 0;
    else {
        conveted_altitude = atof(gps_altitude);
        m_gps_altitude = (long)(conveted_altitude * 100 / 1);
    }

    LOGV("%s(m_gps_altitude(%ld))", __func__, m_gps_altitude);
    return 0;
}

int V4L2Camera::setGPSTimeStamp(const char *gps_timestamp)
{
    LOGV("%s(gps_timestamp(%s))", __func__, gps_timestamp);
    if (gps_timestamp == NULL)
        m_gps_timestamp = 0;
    else
        m_gps_timestamp = atol(gps_timestamp);

    LOGV("%s(m_gps_timestamp(%ld))", __func__, m_gps_timestamp);
    return 0;
}

int V4L2Camera::setGPSProcessingMethod(const char *gps_processing_method)
{
    LOGV("%s(gps_processing_method(%s))", __func__, gps_processing_method);
    memset(mExifInfo.gps_processing_method, 0, sizeof(mExifInfo.gps_processing_method));
    if (gps_processing_method != NULL) {
        size_t len = strlen(gps_processing_method);
        if (len > sizeof(mExifInfo.gps_processing_method)) {
            len = sizeof(mExifInfo.gps_processing_method);
        }
        memcpy(mExifInfo.gps_processing_method, gps_processing_method, len);
    }
    return 0;
}

int V4L2Camera::setJpegThumbnailSize(int width, int height)
{
    LOGV("%s(width(%d), height(%d))", __func__, width, height);

    m_jpeg_thumbnail_width  = width;
    m_jpeg_thumbnail_height = height;

    return 0;
}

int V4L2Camera::getJpegThumbnailSize(int *width, int  *height)
{
    if (width)
        *width   = m_jpeg_thumbnail_width;
    if (height)
        *height  = m_jpeg_thumbnail_height;

    return 0;
}

void V4L2Camera::getThumbnailConfig(int *width, int *height, int *size)
{
#if 0
    if (mCameraId == CAMERA_ID_BACK) {
        *width  = BACK_CAMERA_THUMBNAIL_WIDTH;
        *height = BACK_CAMERA_THUMBNAIL_HEIGHT;
        *size   = BACK_CAMERA_THUMBNAIL_WIDTH * BACK_CAMERA_THUMBNAIL_HEIGHT
                    * BACK_CAMERA_THUMBNAIL_BPP / 8;
    } else {
        *width  = FRONT_CAMERA_THUMBNAIL_WIDTH;
        *height = FRONT_CAMERA_THUMBNAIL_HEIGHT;
        *size   = FRONT_CAMERA_THUMBNAIL_WIDTH * FRONT_CAMERA_THUMBNAIL_HEIGHT
                    * FRONT_CAMERA_THUMBNAIL_BPP / 8;
    }
#endif
	*width  = m_jpeg_thumbnail_width;
    *height = m_jpeg_thumbnail_height;
    *size   = m_jpeg_thumbnail_width * m_jpeg_thumbnail_height * 3 / 2;

}

int V4L2Camera::setJpegQuality(int jpeg_quality)
{
    LOGV("%s(jpeg_quality (%d))", __func__, jpeg_quality);

    if (jpeg_quality < JPEG_QUALITY_MIN || JPEG_QUALITY_MAX < jpeg_quality) {
        LOGE("ERR(%s):Invalid jpeg_quality (%d)", __func__, jpeg_quality);
        return -1;
    }

    if (m_jpeg_quality != jpeg_quality) {
        m_jpeg_quality = jpeg_quality;
    }

    return 0;
}

int V4L2Camera::getJpegQuality(void)
{
    return m_jpeg_quality;
}

void V4L2Camera::getPostViewConfig(int *width, int *height, int *size)
{
	*width = MAX_PREVIEW_WIDTH;
    *height = MAX_PREVIEW_HEIGHT;
    *size = MAX_PREVIEW_WIDTH * MAX_PREVIEW_HEIGHT * 3 / 2;
}

//======================================================================

int V4L2Camera::setExifOrientationInfo(int orientationInfo)
{
     LOGV("%s(orientationInfo(%d))", __func__, orientationInfo);

     if (orientationInfo < 0) {
         LOGE("ERR(%s):Invalid orientationInfo (%d)", __func__, orientationInfo);
         return -1;
     }
     m_exif_orientation = orientationInfo;

     return 0;
}



}; // namespace android

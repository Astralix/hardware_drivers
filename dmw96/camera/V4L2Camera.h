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

#ifndef _V4L2CAMERA_H
#define _V4L2CAMERA_H
#define NB_BUFFER 4
#define DEFAULT_FRAME_RATE 1

#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <linux/videodev.h>
//camera 

#include "Exif.h"
#include "JpegEncoder.h"

#ifndef DSPG_FIRST_CAMERA_DEVICE
#define DSPG_FRONT_CAMERA_DEVICE	"/dev/video4"
#endif

#ifndef DSPG_SECOND_CAMERA_DEVICE
#define DSPG_SECOND_CAMERA_DEVICE	"/dev/video"
#endif

//#define DEFAULT_PREVIEW_WIDTH           320
//#define DEFAULT_PREVIEW_HEIGHT          240

#define DEFAULT_PREVIEW_WIDTH        640
#define DEFAULT_PREVIEW_HEIGHT       480
//TBD:!!!!!!
#define MAX_PREVIEW_WIDTH           640
#define MAX_PREVIEW_HEIGHT      	480

#define MAX_JPG_WIDTH                   1280
#define MAX_JPG_HEIGHT                  768

#define DEFAULT_FOCAL_LENGTH			343

#define JPG_STREAM_BUF_SIZE			MAX_JPG_WIDTH*MAX_JPG_HEIGHT
#define JPEG_QUALITY_MIN	0
#define JPEG_QUALITY_MAX	100
#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR       "0.10,1.20,Infinity"

#define MAX_BUFFERS     11
#define DSPG_CAMERA_FRAME_EXTRA_SIZE	64

namespace android {

enum ov3640_frameintervals_type {
	OV34_15_FPS,
	OV34_30_FPS,
};

struct vdIn {
    struct v4l2_capability cap;
    struct v4l2_format format;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers rb;
    void *mem[NB_BUFFER];
    unsigned int physical_mem[NB_BUFFER];
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
	int framerate;
};

class V4L2Camera {

    struct gps_info_latiude {
        unsigned int    north_south;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoLatitude;
    struct gps_info_longitude {
        unsigned int    east_west;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoLongitude;
    struct gps_info_altitude {
        unsigned int    plus_minus;
        unsigned int    dgree;
        unsigned int    minute;
        unsigned int    second;
    } gpsInfoAltitude;

    enum CAMERA_ID {
        CAMERA_ID_BACK  = 0,
        CAMERA_ID_FRONT = 1,
    };
public:
    V4L2Camera();
    ~V4L2Camera();

    int Open (int CameraId);
    int Init (int Width,int Height,int PixelFormat,int Framerate);

    void Uninit ();
    void Close ();

    int StartPreview ();
    int StopPreview ();


    int GrabPreviewFrame ( unsigned char **pPreviewBuffer );
    void ReleasePreviewFrame ( int Index );

    unsigned int GetPhysicalAddress(unsigned int buffer_index);

    //BS:: not in use
    //int savePicture(unsigned char *inputBuffer, const char * filename);

    //BS:: ONLY IN USE WHEN NO OVERLAY PRESENT
    void Convert(unsigned char *buf, unsigned char *rgb);

	//Convert NV12 SP CbCr to NV12CrCbPlanarm this is how it used by Andorid 
	//look at hardware/libhardware/include/hardware/hardware. as described

    //BS:: we do not copy frames
    //void GrabRawFrame(void *previewBuffer);

	int GetWidth() { return videoIn->width; }
	int GetHeight() { return videoIn->height; }
	int GetFrameSize() { return videoIn->framesizeIn;}
	int GetFps() { return videoIn->framerate;}
	int SetPreviewSize(int Width,int Height,int PixelFormat,int Framerate);
    void            getPostViewConfig(int*, int*, int*);

	//Exif
	void setExifChangedAttribute();
	void setExifFixedAttribute();
	int getExif(unsigned char *pExifDst, unsigned char *pThumbSrc,int thumbSize);
	int64_t getNowms();

	int mCameraId;

    int             setGPSLatitude(const char *gps_latitude);
    int             setGPSLongitude(const char *gps_longitude);
    int             setGPSAltitude(const char *gps_altitude);
    int             setGPSTimeStamp(const char *gps_timestamp);
    int             setGPSProcessingMethod(const char *gps_timestamp);

    int             setJpegThumbnailSize(int width, int height);
    int             getJpegThumbnailSize(int *width, int *height);
    void            getThumbnailConfig(int *width, int *height, int *size);

    int             setJpegQuality(int jpeg_qality);
    int             getJpegQuality(void);
	int 			setExifOrientationInfo(int orientationInfo);

	sp<MemoryHeapBase>	*mPreviewHeap;

	// camera frame counter
	int             m_frame_counter;

private:
    struct vdIn *videoIn;
    int fd;

    int nQueued;
    int nDequeued;

    int InitMmap ();
    int InitFps( );

	exif_attribute_t mExifInfo;
	JpegEncoder	mJpgEnc;

    long            m_gps_latitude;
    long            m_gps_longitude;
    long            m_gps_altitude;
    long            m_gps_timestamp;

    int             m_jpeg_thumbnail_width;
    int             m_jpeg_thumbnail_height;
    int             m_jpeg_quality;

	int 			m_exif_orientation;
};

}; // namespace android

#endif

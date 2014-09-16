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

#ifndef DSPG_FRONT_CAMERA_FILENAME
#define DSPG_FRONT_CAMERA_FILENAME	"/dev/video4"
#endif

#ifndef DSPG_BACK_CAMERA_FILENAME
#define DSPG_BACK_CAMERA_FILENAME	"/dev/video"
#endif

#define TEMP_JPEG_FILENAME	"/tmp/tmp.jpg"
#define DEFAULT_PREVIEW_WIDTH           320
#define DEFAULT_PREVIEW_HEIGHT          240
//#define DEFAULT_PREVIEW_WIDTH           640
//#define DEFAULT_PREVIEW_HEIGHT          480

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
    bool isStreaming;
    int width;
    int height;
    int formatIn;
    int framesizeIn;
	int framerate;
};

class V4L2Camera {
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


	void drawSquare(unsigned char *buffer, int x, int y);
	//void V4L2Camera::drawResolution(unsigned char *buffer);

    int GrabPreviewFrame ( unsigned char **pPreviewBuffer );
    void ReleasePreviewFrame ( int Index );

    //BS:: not in use
    //int savePicture(unsigned char *inputBuffer, const char * filename);

    //BS:: ONLY IN USE WHEN NO OVERLAY PRESENT
	int SaveYUVNV12toJPEG (unsigned char *pYUV, int quality);
    void Convert(unsigned char *buf, unsigned char *rgb);
	int SaveYuvToFile(unsigned char *yuv);

	//Convert NV12 SP CbCr to NV12CrCbPlanarm this is how it used by Andorid 
	//look at hardware/libhardware/include/hardware/hardware. as described

    //BS:: we do not copy frames
    //void GrabRawFrame(void *previewBuffer);

	int GetWidth() { return videoIn->width; }
	int GetHeight() { return videoIn->height; }
	int GetFrameSize() { return videoIn->framesizeIn;}
	int GetFps() { return videoIn->framerate;}
	void SetPreviewSize(int Width,int Height,int PixelFormat);

private:
    struct vdIn *videoIn;
	unsigned char *buffer;
	unsigned char *cbcrBuffer;
	int squareX;
	int squareDirection;
	int counter;
    int fd;

    int nQueued;
    int nDequeued;

    int InitMmap ();
    int InitFps( );

	void deinterleave(uint8_t* vuPlanar, uint8_t* uRows,
        uint8_t* vRows, int rowIndex, int width);
};

}; // namespace android

#endif

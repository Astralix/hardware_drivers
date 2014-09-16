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

#define LOG_NDEBUG 0
#define LOG_TAG "DSPG_V4L2Camera_dummy"
#include <utils/Log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/select.h>

#include "V4L2Camera_dummy.h"

extern "C" {
    #include "jpeglib.h"
    #include "jerror.h"
}

namespace android {

V4L2Camera::V4L2Camera ()
    : fd(-1),nQueued(0), nDequeued(0)
{
    LOGV("%s :", __func__);
    videoIn = (struct vdIn *) calloc (1, sizeof (struct vdIn));
	memset(videoIn,0,sizeof (struct vdIn));

    videoIn->width = DEFAULT_PREVIEW_WIDTH;
    videoIn->height = DEFAULT_PREVIEW_HEIGHT;
    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 3 / 2);
    videoIn->formatIn = V4L2_PIX_FMT_NV12;

	videoIn->isStreaming = false;

	buffer = NULL;
	cbcrBuffer = NULL;
}

V4L2Camera::~V4L2Camera()
{
    LOGV("%s :", __func__);
    free(videoIn);

	if (buffer) free(buffer);
	if (cbcrBuffer) free(cbcrBuffer);
}

int V4L2Camera::Open(int CameraId)
{
	LOGV("%s :", __func__);
#ifdef REAL_CAMERA
    int ret;
    char *pCameraFileName;

    LOGD("%s :", __func__);

	if( fd != -1 )
	{
        LOGW("%s : Camera already opened", __func__);
		return NO_ERROR;
	}

	//TBD::: FOR NOW ONLY 1 CAMERA IS ACTIVE,FRONT but Android requires the back camera
	//TBD::: Fake back camera as front
    if ( CameraId == CAMERA_ID_BACK ){
//    	pCameraFileName = (char *)DSPG_BACK_CAMERA_FILENAME;
    	pCameraFileName = (char *)DSPG_FRONT_CAMERA_FILENAME;
    } else if ( CameraId == CAMERA_ID_FRONT ) {
    	pCameraFileName = (char *)DSPG_FRONT_CAMERA_FILENAME;
    } else {
        LOGE("%s : Unknown camera ID", __func__);
        return UNKNOWN_ERROR;
    }

    if ((fd = open(pCameraFileName, O_RDWR)) == -1)
    {
        LOGE("%s : ERROR opening V4L interface %s: %s", __func__,pCameraFileName,strerror(errno));
        return UNKNOWN_ERROR;
    }
#else
#endif
	return NO_ERROR;
}

int V4L2Camera::StartPreview ()
{
	LOGV("%s :", __func__);
#ifdef REAL_CAMERA
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

	if( videoIn->formatIn == V4L2_PIX_FMT_NV12)
	    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 3 / 2);
	else if( videoIn->formatIn == V4L2_PIX_FMT_NV16)
	    videoIn->framesizeIn = ( videoIn->width * videoIn->height * 2);
	else
	{
        LOGE("%s: Format not supported",__func__);
		return UNKNOWN_ERROR;
	}

    LOGD("Requested : Width = %d, Height = %d framesizeIn=%d",videoIn->width,
																videoIn->height,
																videoIn->framesizeIn);

    ret = ioctl(fd, VIDIOC_S_FMT, &videoIn->format);
    if (ret < 0) {
        LOGE("%s: VIDIOC_S_FMT Failed: %s",__func__,strerror(errno));
        return ret;
    }

    ret = InitFps();
    if (ret < 0) {
        LOGE("%s: InitFps Failed: %s",__func__, strerror(errno));
        return ret;
    }

    videoIn->width = videoIn->format.fmt.pix.width;
    videoIn->height = videoIn->format.fmt.pix.height;

    LOGD("Configured : Width = %d, Height = %d framesizeIn=%d",videoIn->width,
																videoIn->height,
																videoIn->framesizeIn);
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

	LOGI("%s : Start Streaming", __func__);

    videoIn->isStreaming = true;


    return ret;
#else
	if (buffer) free(buffer);
	if (cbcrBuffer) free(cbcrBuffer);

	buffer = (unsigned char*)malloc(videoIn->framesizeIn);
	cbcrBuffer = (unsigned char*)malloc(videoIn->width* videoIn->height / 2);

	counter = 0;
	squareX = 0;
	squareDirection = 1;
	unsigned char cbColor = rand() % 210 + 45;
	unsigned char crColor = rand() % 210 + 45;

	for (int i=0 ; i<videoIn->width*videoIn->height/2; i+=2) {
		cbcrBuffer[i] = cbColor;
		cbcrBuffer[i+1] = crColor;
	}

	return NO_ERROR;
#endif
}

void V4L2Camera::SetPreviewSize(int Width,int Height,int PixelFormat)
{
	LOGV("%s :", __func__);

	videoIn->width = Width;
    videoIn->height = Height;
    videoIn->formatIn = PixelFormat;
	videoIn->framesizeIn = ( videoIn->width * videoIn->height * 3 / 2);
}

void V4L2Camera::Close ()
{
#ifdef REAL_CAMERA
    LOGD("%s :", __func__);
	if (fd != -1)
	{
		close(fd);
		fd = -1;
	}
#else
#endif
}


//  Init mmap
int V4L2Camera::InitMmap()
{
	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
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

    for (int i = 0; i < videoIn->rb.count; i++) {

        memset (&videoIn->buf, 0, sizeof (struct v4l2_buffer));

        videoIn->buf.index = i;
        videoIn->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoIn->buf.memory = V4L2_MEMORY_MMAP;

        ret = ioctl (fd, VIDIOC_QUERYBUF, &videoIn->buf);
        if (ret < 0) {
            LOGE("Init: Unable to query buffer (%s)", strerror(errno));
            return ret;
        }

        videoIn->mem[i] = mmap (0,
               videoIn->buf.length,
               PROT_READ | PROT_WRITE,
               MAP_SHARED,
               fd,
               videoIn->buf.m.offset);

        if (videoIn->mem[i] == MAP_FAILED) {
            LOGE("Init: Unable to map buffer #%d (%s)", i,strerror(errno));
            return UNKNOWN_ERROR;
        }

        ret = ioctl(fd, VIDIOC_QBUF, &videoIn->buf);
        if (ret < 0) {
            LOGE("Init: VIDIOC_QBUF Failed");
            return UNKNOWN_ERROR;
        }

        nQueued++;
    }
#else
#endif
    return NO_ERROR;
}

int V4L2Camera::InitFps()
{
	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
    int ret;
    int framerate;
    struct v4l2_streamparm streamparm;
	struct v4l2_frmivalenum frmi; 

    LOGI("%s :", __func__);

	frmi.pixel_format = videoIn->format.fmt.pix.pixelformat;
			
	if (videoIn->format.fmt.pix.width == 320 && videoIn->format.fmt.pix.height == 240)
		//for qvga it is 7.5/15 fps
		frmi.index = OV34_15_FPS;
	else if (videoIn->format.fmt.pix.width == 640 && videoIn->format.fmt.pix.height == 480)
		//for vga it is 15/30 fps
		frmi.index = OV34_15_FPS;
	else
		//other sizes not supported
		return UNKNOWN_ERROR;
	
	frmi.width = videoIn->format.fmt.pix.width;
	frmi.height = videoIn->format.fmt.pix.height;
		
	ret = ioctl (fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmi);
	if (ret != 0){
        LOGE("VIDIOC_ENUM_FRAMEINTERVALS fail....");
        return ret;
	}

    streamparm.parm.capture.timeperframe.numerator = frmi.discrete.numerator;
    streamparm.parm.capture.timeperframe.denominator = frmi.discrete.denominator;
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_S_PARM, &streamparm);
    if(ret != 0) {
        LOGE("VIDIOC_S_PARM  Fail....");
        return -1;
    }

	videoIn->framerate = streamparm.parm.capture.timeperframe.denominator;
#else
#endif
    return NO_ERROR;
}

void V4L2Camera::Uninit()
{
	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
    int ret;

    LOGD("%s :", __func__);
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
    /* Unmap buffers */
    for (int i = 0; i < NB_BUFFER; i++)
        if (munmap(videoIn->mem[i], videoIn->buf.length) < 0)
            LOGE("Uninit: Unmap failed");
#else
#endif
}

int V4L2Camera::StopPreview ()
{
	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
    enum v4l2_buf_type type;
    int ret;

    LOGD("%s :", __func__);
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
		Close();

        videoIn->isStreaming = false;
	}
#else
#endif
    return NO_ERROR;
}

void V4L2Camera::drawSquare(unsigned char *buffer, int xStart, int yStart)
{
	for (int y = 0; y<70 ; y++) {
		for (int x = 0; x<25 ; x++) {
			buffer[(y + yStart)*videoIn->width + (x+xStart)] = 0x0;
		}
	}
}

/*BS:: Instead of copying buffer, pass it to the upper layers, the problem here is that we can run out of
 * available buffers if they are not released by camera service*/
int V4L2Camera::GrabPreviewFrame (unsigned char **pPreviewBuffer)
{
//	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
    int ret;

    videoIn->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoIn->buf.memory = V4L2_MEMORY_MMAP;

    /* DQueue frame */
    ret = ioctl(fd, VIDIOC_DQBUF, &videoIn->buf);
    if (ret < 0) {
        LOGE("GrabPreviewFrame: VIDIOC_DQBUF Failed");
        return ret;
    }
    *pPreviewBuffer = (unsigned char *)videoIn->mem[videoIn->buf.index];

    return videoIn->buf.index;
#else
	/* here we need to copy the frame to the buffer */
	squareX += squareDirection*5;
	if (squareX > videoIn->width-30) {
		squareX = videoIn->width - 30;
		squareDirection = -1;
	}
	else if (squareX < 0) {
		squareX = 0;
		squareDirection = 1;
	}

	memset(buffer, 0xff, videoIn->width * videoIn->height);
	memcpy(buffer + videoIn->width*videoIn->height, cbcrBuffer, videoIn->width*videoIn->height/2);

	drawSquare(buffer, squareX, 10);

	usleep(66000);
	*pPreviewBuffer = buffer;

	return 0;
#endif
}

/*BS:: Release preview frame by index*/
void V4L2Camera::ReleasePreviewFrame ( int Index )
{
//	LOGV("%s :", __func__);

#ifdef REAL_CAMERA
	int32_t ret;

    videoIn->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoIn->buf.memory = V4L2_MEMORY_MMAP;
    videoIn->buf.index = Index;

    /* Queue frame */
    ret = ioctl(fd, VIDIOC_QBUF, &videoIn->buf);
    if (ret < 0) {
        LOGE("ReleasePreviewFrame: VIDIOC_QBUF Failed for index=%d",Index);
        return;
    }
#else
#endif
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

int V4L2Camera::SaveYuvToFile(unsigned char *yuv)
{
	FILE *fptr = fopen("/tmp/out.yuv","wb");

	if (fptr != NULL ){
		unsigned int offset = 0;
		unsigned int i=0,chroma_size = videoIn->width*videoIn->height/2;
//		dmw96ciu_get_luma_address(videoIn->width , videoIn->height , 0 ,&offset);

		//luma
		fwrite(videoIn->mem[videoIn->buf.index],1,videoIn->width*videoIn->height,fptr);
		//chroma

		while( i < chroma_size ){

			fwrite((unsigned char *)videoIn->mem[videoIn->buf.index] + offset + i,1,1,fptr);
			i+=2;
		}

		i = 0;			
		while( i < chroma_size ){

				fwrite((unsigned char *)videoIn->mem[videoIn->buf.index] + offset + i + 1,1,1,fptr);
			i+=2;
		}
		
		LOGD("YUV file is ready");
		fclose(fptr);
	} else {
		LOGE("Cannot open file /tmp/out.yuv");
	}
		
	return 0;
}

int V4L2Camera::SaveYUVNV12toJPEG(unsigned char* src, int quality) 
{
	int width = videoIn->width;
	int height = videoIn->height;
	int fileSize;

	char* filename = TEMP_JPEG_FILENAME;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile = NULL;
    int ret = NO_ERROR;
    if ((outfile = fopen(filename, "wb")) == NULL) 
    {  
        return UNKNOWN_ERROR;
    }    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

	//setup compression defaults
    cinfo.image_width = width; 
    cinfo.image_height = height;
    cinfo.input_components = 3;        
    cinfo.in_color_space = JCS_YCbCr;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.raw_data_in = TRUE;  
    
    {
        JSAMPARRAY pp[3];
        JSAMPROW *rpY = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        JSAMPROW *rpU = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        JSAMPROW *rpV = (JSAMPROW*)malloc(sizeof(JSAMPROW) * height);
        int k;
        cinfo.comp_info[0].h_samp_factor =
        cinfo.comp_info[0].v_samp_factor = 2;
        cinfo.comp_info[1].h_samp_factor =
        cinfo.comp_info[1].v_samp_factor =
        cinfo.comp_info[2].h_samp_factor =
        cinfo.comp_info[2].v_samp_factor = 1;
        jpeg_start_compress(&cinfo, TRUE);

        pp[0] = rpY;
        pp[1] = rpU;
        pp[2] = rpV;

		uint8_t* uRows = new uint8_t [8 * (width >> 1)];
		uint8_t* vRows = new uint8_t [8 * (width >> 1)];



		unsigned char *pUV = src + width*height;

		for (int line = 0; line < height; line += 2*DCTSIZE) {

			deinterleave(pUV, uRows, vRows, line, width);

	        // setup pointers in the JSAMPIMAGE array
            for (int i = 0; i < 2*DCTSIZE; i++) {
	            rpY[i] = src + width*(i + line);
		        // construct u row and v row
		        if ((i & 1) == 0) {
		            // height and width are both halved because of downsampling
		            int offset = (i >> 1) * (width >> 1);
		            rpU[i/2] = uRows + offset;
		            rpV[i/2] = vRows + offset;
		        }
            }
            jpeg_write_raw_data(&cinfo, pp,2*DCTSIZE);
        }
        jpeg_finish_compress(&cinfo);
        free(rpY);
        free(rpU);
        free(rpV);

		delete [] uRows;
		delete [] vRows;

    }
	fileSize = ftell(outfile);
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
    return fileSize;
}

//Deinterleive UV samples to planar mode
void V4L2Camera::deinterleave(uint8_t* vuPlanar, uint8_t* uRows,
        uint8_t* vRows, int rowIndex, int width) 
{
    for (int row = 0; row < 8; ++row) {
        int offset = ((rowIndex >> 1) + row) * width;
        uint8_t* uv = vuPlanar + offset;
        for (int i = 0; i < (width >> 1); ++i) {
            int index = row * (width >> 1) + i;
            uRows[index] = *uv++;
            vRows[index] = *uv++;
        }
    }
}


}; // namespace android

/*
**
** Copyright (C) 2009 DSPG
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
#define LOG_TAG "DSPG_CameraHardware"
#include <utils/Log.h>

#include "CameraHardware.h"
#include <fcntl.h>
#include <sys/mman.h>

#include <hardware/overlay.h>
#include <ui/Overlay.h>
#include <dspg_buffer.h>

#include <cutils/properties.h>

static int mDebugFps = 0;

namespace android {

struct addrs {
    unsigned int phys_addr;
    unsigned int index;
    unsigned int reserved;
    unsigned int reserved2;
};


wp<CameraHardwareInterface> CameraHardware::singleton;

CameraHardware::CameraHardware(int cameraId) : mPreviewFrameSize(0),
					mRecordRunning(false),
					mMsgEnabled(0),
					mCaptureInProgress(false),
					mOverlay(NULL),
					mUseOverlay(false)
{
    LOGI("%s :", __func__);

    mCameraId = cameraId;

    // clear mBuffersOut
    for (int i=0 ; i<MAX_BUFFERS ; i++) {
        mBuffersOut[i] = 0;
    }

    /* whether prop "debug.camera.showfps" is enabled or not */
    char value[PROPERTY_VALUE_MAX];
    property_get("debug.camera.showfps", value, "0");
    mDebugFps = atoi(value);
    LOGD_IF(mDebugFps, "showfps enabled");

    mExitPreviewThread = false;
    /* whether the PreviewThread is active in preview or stopped.  we
     * create the thread but it is initially in stopped state.
     */
    mPreviewRunning = false;


    if (mCamera.Open(mCameraId) != NO_ERROR)
    {
    	LOGE("Open failed: cannot open device.");
    }
	else
	{
		//set what camera driver returns
	    initDefaultParameters();
	}

	mCamera.setExifFixedAttribute();

    mRecordHeap = new MemoryHeapBase(sizeof(struct addrs) * NB_BUFFER);

    mPreviewThread = new PreviewThread(this);
    mPictureThread = new PictureThread(this);

}

void CameraHardware::initDefaultParameters()
{
    CameraParameters p;
    String8 parameterString;

    p.setPreviewSize(mCamera.GetWidth(),mCamera.GetHeight());
    p.setPictureSize(mCamera.GetWidth(),mCamera.GetHeight());
    p.setPreviewFrameRate(mCamera.GetFps());
    p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP); //??
//    p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV422SP);

	p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
//              "720x480,640x480,352x288,176x144");
              "1280x720,720x576,720x480,704x576,704x480,640x480,352x288,352x240,320x240,176x144,160x120");
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
//              "2048x1536,1024x768,640x480");
              "1280x720,640x480,320x240");		
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,
              "11,15,20,25,30");
    p.set(CameraParameters::KEY_PREVIEW_FRAME_RATE,"30");	

    p.getSupportedPreviewSizes(mSupportedPreviewSizes);

	//BS:: we do not have autofocus for both front camera
    parameterString = CameraParameters::FOCUS_MODE_FIXED;
    
	p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
              parameterString.string());

    p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_FIXED);

    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,
          CameraParameters::PIXEL_FORMAT_YUV420SP);

	p.set(CameraParameters::KEY_ROTATION, 0);

	p.set(CameraParameters::KEY_JPEG_QUALITY, "100"); // maximum quality

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(11000,30000)");
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, "11000,30000");

	p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.43");

    p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
          CameraParameters::PIXEL_FORMAT_YUV420SP);
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
          CameraParameters::PIXEL_FORMAT_JPEG);
    p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,
          CameraParameters::PIXEL_FORMAT_YUV420SP);

    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
              "176x144,0x0");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, "176");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, "144");

    p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "51.2");
    p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "39.4");

    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "4");
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-4");
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0.5");
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "100");

    if (setParameters(p) != NO_ERROR) {
        LOGE("Failed to set default parameters?!");
    }

	mParameters = p;
}

CameraHardware::~CameraHardware()
{
    singleton.clear();
}

sp<IMemoryHeap> CameraHardware::getPreviewHeap() const
{
    LOGI("%s :", __func__);
    return NULL;
}

sp<IMemoryHeap> CameraHardware::getRawHeap() const
{
    LOGI("%s :", __func__);
    return NULL;
}

// ---------------------------------------------------------------------------
// CameraHardwareInterface callbacks

void CameraHardware::setCallbacks(notify_callback notify_cb,
                                      data_callback data_cb,
                                      data_callback_timestamp data_cb_timestamp,
                                      void *user)
{
    mNotifyCb = notify_cb;
    mDataCb = data_cb;
    mDataCbTimestamp = data_cb_timestamp;
    mCallbackCookie = user;
}

void CameraHardware::enableMsgType(int32_t msgType)
{
    LOGV("%s : msgType = 0x%x, mMsgEnabled before = 0x%x",
         __func__, msgType, mMsgEnabled);
    mMsgEnabled |= msgType;
}

void CameraHardware::disableMsgType(int32_t msgType)
{
    LOGV("%s : msgType = 0x%x, mMsgEnabled before = 0x%x",
         __func__, msgType, mMsgEnabled);
    mMsgEnabled &= ~msgType;
}

bool CameraHardware::msgTypeEnabled(int32_t msgType)
{
    return (mMsgEnabled & msgType);
}
// ---------------------------------------------------------------------------
static void showFPS(const char *tag)
{
    static int mFrameCount = 0;
    static int mLastFrameCount = 0;
    static nsecs_t mLastFpsTime = 0;
    static float mFps = 0;
    mFrameCount++;
    if (!(mFrameCount & 0x1F)) {
        nsecs_t now = systemTime();
        nsecs_t diff = now - mLastFpsTime;
        mFps =  ((mFrameCount - mLastFrameCount) * float(s2ns(1))) / diff;
        mLastFpsTime = now;
        mLastFrameCount = mFrameCount;
        LOGD("[%s] %d Frames, %f FPS", tag, mFrameCount, mFps);
    }
}

bool CameraHardware::isSupportedPreviewSize(const int Width,
                                               const int Height) const
{
    unsigned int i;

    for (i = 0; i < mSupportedPreviewSizes.size(); i++) {
        if (mSupportedPreviewSizes[i].width == Width &&
                mSupportedPreviewSizes[i].height == Height)
            return true;
    }

    return false;
}

status_t CameraHardware::setOverlay(const sp<Overlay> &overlay)
{
    LOGV("%s :", __func__);

    int overlayWidth  = 0;
    int overlayHeight = 0;
    int overlayFrameSize = 0;

    if (overlay == NULL) {
        LOGD("%s : overlay == NULL", __func__);
        goto setOverlayFail;
    }
    LOGD("%s : overlay = %p", __func__, overlay->getHandleRef());

    if (overlay->getHandleRef()== NULL) {
        if (mOverlay != 0) {
            LOGV("%s:%d",__func__,__LINE__);
            mOverlay->destroy();
        }

        mOverlay = NULL;
        mUseOverlay = false;

        return NO_ERROR;
    }

    if (overlay->getStatus() != NO_ERROR) {
        LOGE("ERR(%s):overlay->getStatus() fail", __func__);
        goto setOverlayFail;
    }

   //BS:: TBD!!!!!!!!!
   if (overlay->setCrop(0, 0, overlayWidth, overlayHeight) != NO_ERROR) {
        LOGE("ERR(%s)::(mOverlay->setCrop(0, 0, %d, %d) fail", __func__, overlayWidth, overlayHeight);
        goto setOverlayFail;
    }

    mOverlay = overlay;
    mUseOverlay = true;

    return NO_ERROR;

setOverlayFail :
    if (mOverlay != 0)
        mOverlay->destroy();
    mOverlay = 0;

    mUseOverlay = false;

    return UNKNOWN_ERROR;
}

int CameraHardware::OverlayPostFrame(unsigned char *pPreviewFrame, int index)
{
    int ret;
    void *pOverlayBuffer;
    overlay_buffer_t overlay_buffer;
	unsigned int previewFramePhysAddr;
	unsigned char *previewFrameVirtAddr;
	unsigned int previewFrameSize;

	uint32_t outWidth, outHeight;

	struct dspg_frame_footer *frameFooter;

	previewFramePhysAddr = mCamera.GetPhysicalAddress(index);
	previewFrameVirtAddr = pPreviewFrame;
	previewFrameSize = mCamera.GetFrameSize();

	// take small frame dimensions from the frame footer located right after the full image
	frameFooter = (struct dspg_frame_footer*)(pPreviewFrame+mCamera.GetFrameSize());

	/* If displaying preview for 720p, give small picture to the overlay (right after the full-frame in the memory) */
	if (mCamera.GetWidth() == 1280) {
		mOverlay->getOutputSize(&outWidth, &outHeight);
	
		if (outWidth <= 320 && outHeight <= 240) {
			if (!mUseSmallPreviewFrame) {
				if (mOverlay->resizeInput(frameFooter->smallWidth, frameFooter->smallHeight) == 0)
					mUseSmallPreviewFrame = true;
				else
					LOGE("error: cannot resize overlay input to %dx%d", frameFooter->smallWidth, frameFooter->smallHeight);
			}
		}
		else {
			if (mUseSmallPreviewFrame) {
				if (mOverlay->resizeInput(mCamera.GetWidth(), mCamera.GetHeight()) == 0)
					mUseSmallPreviewFrame = false;
				else
					LOGE("error: cannot resize overlay input to %dx%d", mCamera.GetWidth(), mCamera.GetHeight());
			}
		}
	}

	if (mUseSmallPreviewFrame) {
		// skip the full-frame (put address at the begining of the small frame)
		previewFramePhysAddr += mCamera.GetFrameSize() + sizeof(struct dspg_frame_footer);
		previewFrameVirtAddr += mCamera.GetFrameSize() + sizeof(struct dspg_frame_footer);
		previewFrameSize = frameFooter->smallWidth * frameFooter->smallHeight * 1.5;
	}
	

    ret = mOverlay->dequeueBuffer(&overlay_buffer);
    if (ret != NO_ERROR )
    {
    	LOGE("ERR(%s):Fail on mOverlay->dequeueBuffer()", __func__);
    	return UNKNOWN_ERROR;
    }

    struct dspg_buffer_t *dspg_buffer = (struct dspg_buffer_t*)overlay_buffer;

    if (dspg_buffer->allow_zero_copy) {
        // give physical address to the post-processor
        dspg_buffer->bus_address = previewFramePhysAddr;

        // give virtual address to the osdm
        dspg_buffer->virt_address = (unsigned int)previewFrameVirtAddr;
    }
    else
    {
        pOverlayBuffer = mOverlay->getBufferAddress(overlay_buffer);
		if( pOverlayBuffer == NULL)
		{
		        LOGE("ERR(%s):Fail on mOverlay->getBufferAddress()", __func__);
		        return UNKNOWN_ERROR;
		}

		//BS:: copy frame to Overlay
		//TBD:!!!!! USE DMA COPY?
		memcpy((unsigned char *)pOverlayBuffer, previewFrameVirtAddr, previewFrameSize);
	}

    ret = mOverlay->queueBuffer(overlay_buffer);
    if (ret != NO_ERROR )
    {
    	LOGE("ERR(%s):overlay queueBuffer fail", __func__);
    	return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

int CameraHardware::previewThread()
{
    int index,ret;
    nsecs_t timestamp;
    unsigned char *pRawFrame;
    int width, height;
	int offset;
	sp<MemoryBase> buffer;
	int bufIndex;

    //get preview frame size
    index = mCamera.GrabPreviewFrame(&pRawFrame);
    timestamp = systemTime();
    if (index < 0) {
        LOGE("ERR(%s):Fail on mCamera.GrabPreviewFrame()", __func__);
        return UNKNOWN_ERROR;
    }

#ifdef ENABLE_PROFILING
    LOGD("VEncode: Grab->Preview, Num: %d, time=%lld", mCamera.m_frame_counter, mCamera.getNowms());
#endif

	// Post frame to overlay here only if we are not sending the frame to the encoder, 
	// else we do it after sending the frame to the encoder
	if (!mRecordRunning || !(mMsgEnabled & CAMERA_MSG_VIDEO_FRAME)) {
		if (mOverlay != NULL ) {
			ret = OverlayPostFrame(pRawFrame, index);
			if ( ret != NO_ERROR ) {
				LOGE("ERR(%s):Failed to preview frame in Overlay", __func__);
			}
		}
	}

	if (mMsgEnabled & (CAMERA_MSG_PREVIEW_FRAME)) {
	    sp<MemoryBase> buffer = new MemoryBase(mCamera.mPreviewHeap[index], 0, mCamera.GetFrameSize());
		//swap UV, Android's yuv420SP = nv21, we are working in nv12 format
		unsigned int *dstPtr = (unsigned int*)((unsigned char *)mCamera.mPreviewHeap[index]->getBase() + mCamera.GetWidth()*mCamera.GetHeight());
		unsigned int tmp;

		unsigned int uv_num_words = mCamera.GetWidth()*mCamera.GetHeight() / 8;

		for (unsigned int i = 0; i < uv_num_words; i++)
		{
			tmp = *dstPtr;
			*dstPtr =	((tmp & 0x000000ff) << 8) | ((tmp & 0x0000ff00) >> 8) |
						((tmp & 0x00ff0000) << 8) | ((tmp & 0xff000000) >> 8);
			dstPtr++;
		}
      	mDataCb(CAMERA_MSG_PREVIEW_FRAME, buffer, mCallbackCookie);
    }

#ifdef ENABLE_PROFILING
    LOGD("VEncode: Preview->CamSource, Num: %d, time=%lld", mCamera.m_frame_counter, mCamera.getNowms());
#endif

    Mutex::Autolock lock(mRecordLock);
	if (mRecordRunning == true) {
	    // Notify the client of a new frame for recording
		// Call is Async, so we can release the buffer only after we are notified that it is not used any more (releaseRecordingFrame)
        if (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME) {
		    struct addrs *addrs;

		    addrs = (struct addrs *)mRecordHeap->base();
		    sp<MemoryBase> buffer = new MemoryBase(mRecordHeap, index * sizeof(struct addrs), sizeof(struct addrs));
		    addrs[index].phys_addr = mCamera.GetPhysicalAddress(index);
		    addrs[index].index = index;

            // application holds this buffer now
            mBuffersOut[index] = 1;

            mDataCbTimestamp(timestamp, CAMERA_MSG_VIDEO_FRAME, buffer, mCallbackCookie);

			// If we record video, send the frame to the overlay here, 
			// after sending the callback to the encoder
			if (mOverlay != NULL ) {
				ret = OverlayPostFrame(pRawFrame, index);
				if ( ret != NO_ERROR ) {
					LOGE("ERR(%s):Failed to preview frame in Overlay", __func__);
				}
			}

        } else
		    mCamera.ReleasePreviewFrame(index);
    } else {
	    mCamera.ReleasePreviewFrame(index);
	}

#ifdef ENABLE_PROFILING
    LOGD("VEncode: CamSource->Done, Num: %d, time=%lld", mCamera.m_frame_counter, mCamera.getNowms());
#endif

    return NO_ERROR;
}

int CameraHardware::previewThreadWrapper()
{
    LOGI("%s: starting", __func__);

    while (1) {
        mPreviewLock.lock();
        while (!mPreviewRunning) {

            /* Wait for all buffers to by released */
            waitForBuffersRelease();

            LOGI("%s: calling mCamera->stopPreview() and waiting", __func__);
            //if we want to take picrute, do not stop preview
            if( mCaptureInProgress == false ) 
                mCamera.StopPreview();

            /* signal that we're stopping */
            LOGI("%s: signaling for stop condition", __func__);
            mPreviewStoppedCondition.signal();
            mPreviewCondition.wait(mPreviewLock);
        }
        mPreviewLock.unlock();

        if (mExitPreviewThread) {
            LOGI("%s: exiting", __func__);
            mCamera.StopPreview();
            return 0;
        }
        previewThread();
    }
}


status_t CameraHardware::startPreview()
{
    int width, height, pixelformat;
	int ret = NO_ERROR;
	int fps;
    int new_preview_width  = 0;
    int new_preview_height = 0;
	const char *new_str_preview_format = NULL;

    LOGV("%s :", __func__);

    Mutex::Autolock lock(mStateLock);
    /*if (mCaptureInProgress) {
        LOGE("%s : capture in progress, not allowed", __func__);
        return INVALID_OPERATION;
    }*/
	mUseSmallPreviewFrame = false;

    mPreviewLock.lock();
    if (mPreviewRunning) {
        // already running
        LOGE("%s : preview thread already running", __func__);
		ret = UNKNOWN_ERROR;
		goto out;
    }

    mParameters.getPreviewSize(&new_preview_width, &new_preview_height);
    new_str_preview_format = mParameters.getPreviewFormat();	
	fps = mParameters.getPreviewFrameRate();

	mCamera.SetPreviewSize(new_preview_width,new_preview_height,V4L2_PIX_FMT_NV12,fps);

	LOGV("preview width=%d, height=%d size=%d",new_preview_width,new_preview_height,mCamera.GetFrameSize());

    ret = mCamera.StartPreview();

	if ( ret != NO_ERROR)
	{
		LOGE("%s: start preview failed",__func__);
		ret = UNKNOWN_ERROR;
		goto out;
	}

    mPreviewRunning = true;
    mPreviewCondition.signal();

out:
    mPreviewLock.unlock();
    return ret;
}


void CameraHardware::stopPreview()
{
    LOGV("%s: begin", __func__);
    /* request that the preview thread stop. */
    mPreviewLock.lock();
    if (mPreviewRunning) {
        mPreviewRunning = false;
        mPreviewCondition.signal();

        /* wait until preview thread is stopped */
        mPreviewStoppedCondition.wait(mPreviewLock);
    } else {
        LOGV("%s : preview not running, doing nothing", __func__);
    }
    mPreviewLock.unlock();

	LOGV("%s: end", __func__);
}

bool CameraHardware::previewEnabled()
{
    LOGV("%s :", __func__);
    return mPreviewRunning;
}

// ---------------------------------------------------------------------------
// Recording

status_t CameraHardware::startRecording()
{
    LOGV("%s:%d", __func__,__LINE__);

    Mutex::Autolock lock(mRecordLock);

    if (mRecordRunning == false) {
        mRecordRunning = true;
    }
    return NO_ERROR;

}

void CameraHardware::stopRecording()
{
    LOGV("%s:%d", __func__,__LINE__);

    Mutex::Autolock lock(mRecordLock);

    if (mRecordRunning == true) {
        mRecordRunning = false;
    }
    LOGV("%s:%d", __func__,__LINE__);

}

void CameraHardware::releaseRecordingFrame(const sp<IMemory>& mem)
{
    ssize_t offset;
    sp<IMemoryHeap> heap = mem->getMemory(&offset, NULL);
    struct addrs *addrs = (struct addrs *)((uint8_t *)heap->base() + offset);

    //get preview frame size
    mCamera.ReleasePreviewFrame(addrs->index);
    mBuffersOut[addrs->index] = 0;
}


// ---------------------------------------------------------------------------

void CameraHardware::waitForBuffersRelease()
{
    int timeout = 500000;		// 500 ms
    int forceRelease = 1;

    while(timeout>0) {
        if (areThereBuffersOut() == 0) {
            forceRelease = 0;
            break;
        }
        LOGV("waiting for all buffers to return...");
        usleep(10000);
        timeout-=10000;
    }
    if (forceRelease) {
        // force release all buffer that are held by the application
        for (int i=0 ; i<MAX_BUFFERS ; i++) {
            if (mBuffersOut[i] == 1) {
                mCamera.ReleasePreviewFrame(i);
                mBuffersOut[i] = 0;
            }
        }
    }
}

int CameraHardware::areThereBuffersOut()
{
    for (int i=0 ; i<MAX_BUFFERS ; i++) {
        if (mBuffersOut[i] == 1) return 1;
    }

    return 0;
}

status_t CameraHardware::autoFocus()
{

    if (mMsgEnabled & CAMERA_MSG_FOCUS){
	    mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
	}
    //BS::NOT SUPPORTED IN OUR CAMERA
    return NO_ERROR;
}

status_t CameraHardware::cancelAutoFocus()
{
    LOGV("%s :", __func__);

    //BS::NOT SUPPORTED IN OUR CAMERA
    return NO_ERROR;
}


status_t CameraHardware::takePicture()
{
    LOGV("%s :", __func__);
    stopPreview();

    Mutex::Autolock lock(mStateLock);
    if (mCaptureInProgress) {
        LOGE("%s : capture already in progress", __func__);
        return INVALID_OPERATION;
    }

    mCaptureInProgress = true;

    if (mPictureThread->run("CameraPictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
        LOGE("%s : couldn't run picture thread", __func__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

int CameraHardware::getSnapshotImage(unsigned char **pPreviewFrame)
{
    int index;
	int counter = 30;

	while( counter != 0)
	{
	
	    index = mCamera.GrabPreviewFrame(pPreviewFrame);
		if ( index < 0 )
		{
			LOGW("mCamera.GrabPreviewFrame failed");
		}
		else
		{
		    mCamera.ReleasePreviewFrame(index);
		}
		counter--;
	}

	return index;
}

status_t CameraHardware::pictureThread()
{
    LOGV("%s:%d", __func__,__LINE__);
    int ret = NO_ERROR,index;

    unsigned char *pSrcVirtAddr;
    int new_preview_width  = 0;
    int new_preview_height = 0;
    mParameters.getPictureSize(&new_preview_width, &new_preview_height);
    const char *new_str_preview_format = mParameters.getPreviewFormat();
	int fps = mParameters.getPreviewFrameRate();
	int srcPhysAddr;

	mCamera.SetPreviewSize(new_preview_width,new_preview_height,V4L2_PIX_FMT_NV12,fps);

    ret = mCamera.StartPreview();
	if ( ret != NO_ERROR)
	{
		if (ret != ALREADY_EXISTS) {
			LOGE("%s: start preview failed",__func__);
			goto F_EXIT;
		}
	}
	//wait a few frames before capturing
	index = getSnapshotImage(&pSrcVirtAddr);

	//now take real snapshot
	index = mCamera.GrabPreviewFrame(&pSrcVirtAddr);

    if ( index < 0 )
    {
        LOGE("ERR(%s):Fail to grab and post frame", __func__);
		mCamera.StopPreview();
		ret = UNKNOWN_ERROR;
		goto F_EXIT;
    }
	srcPhysAddr = mCamera.GetPhysicalAddress(index);

    if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
	    sp<MemoryBase> buffer = new MemoryBase(mCamera.mPreviewHeap[index], 0, mCamera.GetFrameSize());
	    mDataCb(CAMERA_MSG_RAW_IMAGE, buffer, mCallbackCookie);

		if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
			mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
		}
    }

	//create thumbnail picture by scaling down the raw captured picture
    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
		JpegEncoder	JpegEncoder;

		ret = getJpeg(&JpegEncoder,srcPhysAddr);
		mCamera.ReleasePreviewFrame(index);
		mCamera.StopPreview();

		if (ret == NO_ERROR)
		{
			int32_t thumbSize;
			int32_t imgJpegSize;
			unsigned char *pJpgSrc;

			JpegEncoder.getJpegSizes(&thumbSize,&imgJpegSize);

			pJpgSrc = JpegEncoder.getOutputVirtAddr();

			LOGV("ThumbSize=%d ImgSize=%d",thumbSize,imgJpegSize);

			//BS:: create exif data, allocate enough space for both exif thumb and main image
			sp<MemoryHeapBase> ExifHeap = new MemoryHeapBase(EXIF_FILE_SIZE + thumbSize + imgJpegSize);

			//BS:: flush APP0 stuff and set pointer for thumbnail
			int JpegExifSize = mCamera.getExif((unsigned char *)ExifHeap->base() + JPEG_MARKER_SIZE,
												pJpgSrc + JPEG_APP0_SIZE,
												thumbSize - JPEG_APP0_SIZE);

			if (JpegExifSize >= 0) {
				//BS:: address where jpeg image is placed, manipulate JPEG Markers to include both EXIF+JPEG
				unsigned char *pJpegImageDst = (unsigned char *)ExifHeap->base() + JpegExifSize + JPEG_MARKER_SIZE;
				memcpy((unsigned char *)ExifHeap->base(), pJpgSrc + thumbSize, JPEG_MARKER_SIZE);
				memcpy(pJpegImageDst, pJpgSrc + thumbSize + JPEG_MARKER_SIZE, imgJpegSize - JPEG_MARKER_SIZE);

				sp<MemoryBase> mem = new MemoryBase(ExifHeap, 0, imgJpegSize + thumbSize + JpegExifSize);
				mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mem, mCallbackCookie);
			}
			else
			{
			    ret = UNKNOWN_ERROR;
			}
		}
	}

F_EXIT:
	mStateLock.lock();
	mCaptureInProgress = false;
	mStateLock.unlock();

	/*Stop camera*/		
    return ret;
}

int CameraHardware::getJpeg(JpegEncoder	*pJpegEncoder,int imgPhysAddr)
{
	//setup jpeg encoder
	PostProcessor pp;
    int thumbWidth,thumbHeight,thumbSize;
	int thumbPhysAddr = 0;

	LOGV("%s:%d",__func__,__LINE__);
	/*Initialize PostProcessor, even if not used later*/
	/* We hold instance of PP in case thumbnail will be created*/
    if (pp.init() != 0) {
	    LOGE("pp->init() failed");
        return UNKNOWN_ERROR;
    }

	mCamera.getThumbnailConfig(&thumbWidth,&thumbHeight,&thumbSize);

	if (thumbWidth > 0 && thumbHeight > 0) {

		/*Scale source image to the thumbnail size using post processor*/
		bool bRes = scaleDownYuv420(&pp,imgPhysAddr,mCamera.GetWidth(),mCamera.GetHeight(),thumbWidth,thumbHeight);

		if (bRes == true){
		
			/*Configure Encoder for thumbnail */
			if (pJpegEncoder->setConfig(JPEG_SET_THUMBNAIL_WIDTH, thumbWidth) != NO_ERROR)
				return UNKNOWN_ERROR;

			if (pJpegEncoder->setConfig(JPEG_SET_THUMBNAIL_HEIGHT, thumbHeight) != NO_ERROR)
				return UNKNOWN_ERROR;

			if (pJpegEncoder->setConfig(JPEG_SET_THUMBNAIL_SIZE, thumbSize) != NO_ERROR)
				return UNKNOWN_ERROR;

			thumbPhysAddr = pp.getOutputFrameBusAddr();
		} else {
			LOGE("scaleDownYuv420 failed");
			return UNKNOWN_ERROR;
		}
	}

	int new_jpeg_quality = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
	int jpegSize = mCamera.GetFrameSize();

	if (pJpegEncoder->setConfig(JPEG_SET_OUTPUT_BUF_SIZE,jpegSize) != NO_ERROR)
	    return UNKNOWN_ERROR;

    if (pJpegEncoder->setConfig(JPEG_SET_ENCODE_QUALITY, new_jpeg_quality) != NO_ERROR)
        return UNKNOWN_ERROR;

    if (pJpegEncoder->setConfig(JPEG_SET_ENCODE_WIDTH, mCamera.GetWidth()) != NO_ERROR)
    	return UNKNOWN_ERROR;

    if (pJpegEncoder->setConfig(JPEG_SET_ENCODE_HEIGHT, mCamera.GetHeight()) != NO_ERROR)
        return UNKNOWN_ERROR;

	uint32_t res = pJpegEncoder->encode(imgPhysAddr,thumbPhysAddr);

	if (res == UNKNOWN_ERROR)
	{
		LOGE("%s: failed to encode jpeg picture",__FUNCTION__);
		return UNKNOWN_ERROR;
	}		

	LOGV("%s:%d",__func__,__LINE__);
	return NO_ERROR;
}

bool CameraHardware::scaleDownYuv420(PostProcessor *pp,uint32_t srcPhysAddr, int32_t srcWidth, int32_t srcHeight,
                                     int32_t dstWidth, int32_t dstHeight)
{
    if (srcWidth % 16 != 0 || srcHeight % 16 != 0){
        LOGE("%s: invalid width or height for scaling",__func__);
        return false;
    }

	if (pp->setInputFrame(srcPhysAddr, OVERLAY_FORMAT_YCbCr_420_SP_MPEG2,srcWidth, srcHeight) != NO_ERROR ){
    	LOGE("pp->setInputFrame() failed");
        return false;
    }

    if (pp->allocOutputFrame(OVERLAY_FORMAT_YCbCr_420_SP_MPEG2,dstWidth,dstHeight) != NO_ERROR) {
    	LOGE("pp->setOutputFrame() failed");
        return false;
    }

	if (pp->commitSettings() != NO_ERROR)
	{
		LOGE("%s: error with pp->commitSettings",__func__);
		return false;
	}

	if (pp->execute() != NO_ERROR)
	{
		LOGE("%s: error with pp->execute",__func__);
		return false;
	} 

    return true;

}

status_t CameraHardware::cancelPicture()
{
    LOGV("%s :", __func__);
    mPictureThread->requestExitAndWait();

    return NO_ERROR;
}

status_t CameraHardware::dump(int fd, const Vector<String16>& args) const
{
    LOGV("%s :", __func__);
    return NO_ERROR;
}

status_t CameraHardware::setParameters(const CameraParameters& params)
{
	int ret = NO_ERROR;
    LOGV("%s :", __func__);
    mStateLock.lock();
    if (mCaptureInProgress) {
        mStateLock.unlock();
        LOGE("%s : capture in progress, not allowed", __func__);
        return UNKNOWN_ERROR;
    }
    mStateLock.unlock();

	params.dump();

    // fps range
    int new_min_fps = 0;
    int new_max_fps = 0;
    int current_min_fps, current_max_fps;
    params.getPreviewFpsRange(&new_min_fps, &new_max_fps);
    mParameters.getPreviewFpsRange(&current_min_fps, &current_max_fps);

    /* Check basic validation if scene mode is different */
     if ((new_min_fps > new_max_fps) ||
            (new_min_fps < 0) || (new_max_fps < 0))
        ret = UNKNOWN_ERROR;

	// frame rate
	int new_frame_rate = params.getPreviewFrameRate();
	/* ignore fps requests with unsupported values
	 */

	// Validate frame rate against new min and max fps.
	if (new_frame_rate < (new_min_fps/1000) || new_frame_rate > (new_max_fps/1000) ) {
		//Do not return error if fps is not supported, will fail the CTS test
		LOGW("WARN(%s): request for preview frame rate %d not allowed \n",
		     __func__, new_frame_rate);
	} else {
		mParameters.setPreviewFrameRate(new_frame_rate);
	}

    // rotation
    int new_rotation = params.getInt(CameraParameters::KEY_ROTATION);
    LOGV("%s : new_rotation %d", __func__, new_rotation);
    if (0 <= new_rotation) {
        LOGV("%s : set orientation:%d\n", __func__, new_rotation);
        if (mCamera.setExifOrientationInfo(new_rotation) < 0) {
            LOGE("ERR(%s):Fail on mSecCamera->setExifOrientationInfo(%d)", __func__, new_rotation);
            ret = UNKNOWN_ERROR;
        } else {
            mParameters.set(CameraParameters::KEY_ROTATION, new_rotation);
        }
    }

    // preview size
    int new_preview_width  = 0;
    int new_preview_height = 0;
    params.getPreviewSize(&new_preview_width, &new_preview_height);
    const char *new_str_preview_format = params.getPreviewFormat();
    LOGV("%s : new_preview_width x new_preview_height =%dX%d, format = %s",
         __func__, new_preview_width, new_preview_height, new_str_preview_format);

    if (0 < new_preview_width && 0 < new_preview_height &&
            new_str_preview_format != NULL &&
            isSupportedPreviewSize(new_preview_width, new_preview_height)) {

        int new_preview_format = 0;

        if (!strcmp(new_str_preview_format,
                         CameraParameters::PIXEL_FORMAT_YUV420SP)){
            new_preview_format = V4L2_PIX_FMT_NV12;
		} else if (!strcmp(new_str_preview_format, CameraParameters::PIXEL_FORMAT_YUV422SP)) {
            new_preview_format = V4L2_PIX_FMT_NV16;
		} else {
	        LOGE("%s : %s format not supported", __func__,new_str_preview_format);
	        ret = UNKNOWN_ERROR;
		}
        mParameters.setPreviewSize(new_preview_width, new_preview_height);
        mParameters.setPreviewFormat(new_str_preview_format);

	} else {
        LOGE("%s: Invalid preview size(%dx%d)",
                __func__, new_preview_width, new_preview_height);
        ret = UNKNOWN_ERROR;
	}
	
	//picrute size
    int new_picture_width  = 0;
    int new_picture_height = 0;

    params.getPictureSize(&new_picture_width, &new_picture_height);
    LOGV("%s : new_picture_width x new_picture_height = %dx%d", __func__, new_picture_width, new_picture_height);
    if (0 < new_picture_width && 0 < new_picture_height) {
	    mParameters.setPictureSize(new_picture_width, new_picture_height);
    } else {
        LOGE("%s: Invalid picture size(%dx%d)",
                __func__, new_preview_width, new_preview_height);
        ret = UNKNOWN_ERROR;
	}

    //JPEG image quality
    int new_jpeg_quality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    LOGV("%s : new_jpeg_quality %d", __func__, new_jpeg_quality);
    /* we ignore bad values */
    if (new_jpeg_quality >=1 && new_jpeg_quality <= 100) {
        if (mCamera.setJpegQuality(new_jpeg_quality) < 0) {
            LOGE("ERR(%s):Fail on mSecCamera->setJpegQuality(quality(%d))", __func__, new_jpeg_quality);
            ret = UNKNOWN_ERROR;
        } else {
            mParameters.set(CameraParameters::KEY_JPEG_QUALITY, new_jpeg_quality);
        }
    }

    // JPEG thumbnail size
    int new_jpeg_thumbnail_width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int new_jpeg_thumbnail_height= params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);
    if (0 <= new_jpeg_thumbnail_width && 0 <= new_jpeg_thumbnail_height) {
        if (mCamera.setJpegThumbnailSize(new_jpeg_thumbnail_width, new_jpeg_thumbnail_height) < 0) {
            LOGE("ERR(%s):Fail on mCamera.setJpegThumbnailSize(width(%d), height(%d))", __func__, new_jpeg_thumbnail_width, new_jpeg_thumbnail_height);
            ret = UNKNOWN_ERROR;
        } else {
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, new_jpeg_thumbnail_width);
            mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, new_jpeg_thumbnail_height);
        }
    }

    // gps latitude
    const char *new_gps_latitude_str = params.get(CameraParameters::KEY_GPS_LATITUDE);
    if (mCamera.setGPSLatitude(new_gps_latitude_str) < 0) {
        LOGE("%s::mSecCamera->setGPSLatitude(%s) fail", __func__, new_gps_latitude_str);
        ret = UNKNOWN_ERROR;
    } else {
        if (new_gps_latitude_str) {
            mParameters.set(CameraParameters::KEY_GPS_LATITUDE, new_gps_latitude_str);
        } else {
            mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
        }
    }

    // gps longitude
    const char *new_gps_longitude_str = params.get(CameraParameters::KEY_GPS_LONGITUDE);

    if (mCamera.setGPSLongitude(new_gps_longitude_str) < 0) {
        LOGE("%s::mSecCamera->setGPSLongitude(%s) fail", __func__, new_gps_longitude_str);
        ret = UNKNOWN_ERROR;
    } else {
        if (new_gps_longitude_str) {
            mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, new_gps_longitude_str);
        } else {
            mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
        }
    }

    // gps altitude
    const char *new_gps_altitude_str = params.get(CameraParameters::KEY_GPS_ALTITUDE);

    if (mCamera.setGPSAltitude(new_gps_altitude_str) < 0) {
        LOGE("%s::mSecCamera->setGPSAltitude(%s) fail", __func__, new_gps_altitude_str);
        ret = UNKNOWN_ERROR;
    } else {
        if (new_gps_altitude_str) {
            mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, new_gps_altitude_str);
        } else {
            mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
        }
    }

    // gps timestamp
    const char *new_gps_timestamp_str = params.get(CameraParameters::KEY_GPS_TIMESTAMP);

    if (mCamera.setGPSTimeStamp(new_gps_timestamp_str) < 0) {
        LOGE("%s::mSecCamera->setGPSTimeStamp(%s) fail", __func__, new_gps_timestamp_str);
        ret = UNKNOWN_ERROR;
    } else {
        if (new_gps_timestamp_str) {
            mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, new_gps_timestamp_str);
        } else {
            mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
        }
    }

    // gps processing method
    const char *new_gps_processing_method_str = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);

    if (mCamera.setGPSProcessingMethod(new_gps_processing_method_str) < 0) {
        LOGE("%s::mSecCamera->setGPSProcessingMethod(%s) fail", __func__, new_gps_processing_method_str);
        ret = UNKNOWN_ERROR;
    } else {
        if (new_gps_processing_method_str) {
            mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, new_gps_processing_method_str);
        } else {
            mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
        }
    }



    return ret;
}

CameraParameters CameraHardware::getParameters() const
{
    LOGV("%s :", __func__);
    return mParameters;
}

void CameraHardware::release()
{
    LOGI("%s:%d", __func__,__LINE__);

    /* shut down any threads we have that might be running.  do it here
     * instead of the destructor.  we're guaranteed to be on another thread
     * than the ones below.  if we used the destructor, since the threads
     * have a reference to this object, we could wind up trying to wait
     * for ourself to exit, which is a deadlock.
     */
    if (mPreviewThread != NULL) {
        /* this thread is normally already in it's     int new_preview_width  = 0;
    int new_preview_height = 0;
threadLoop but blocked
         * on the condition variable or running.  signal it so it wakes
         * up and can exit.
         */
        mPreviewThread->requestExit();
        mExitPreviewThread = true;
        mPreviewRunning = true; /* let it run so it can exit */
        mPreviewCondition.signal();
        mPreviewThread->requestExitAndWait();
        mPreviewThread.clear();
    }
    if (mPictureThread != NULL) {
        mPictureThread->requestExitAndWait();
        mPictureThread.clear();
    }

    if (mRecordHeap != NULL)
        mRecordHeap.clear();

    if (mUseOverlay) {
		LOGV("%s:%d",__func__,__LINE__);
        mOverlay->destroy();
        mUseOverlay = false;
        mOverlay = NULL;
    }

	mCamera.Close();
}

status_t CameraHardware::sendCommand(int32_t command, int32_t arg1, int32_t arg2)
{
    return BAD_VALUE;
}



sp<CameraHardwareInterface> CameraHardware::createInstance(int CameraId)
{
    LOGV("%s :", __func__);
    //TBD::: User camera ID
    if (singleton != 0) {
        sp<CameraHardwareInterface> hardware = singleton.promote();
        if (hardware != 0) {
            return hardware;
        }
    }
    sp<CameraHardwareInterface> hardware(new CameraHardware(CameraId));
    singleton = hardware;
    return hardware;
}

static CameraInfo sCameraInfo[] = {
#ifdef ENABLE_FIRST_CAMERA
    {
        DSPG_FIRST_CAMERA_FACING,
        DSPG_FIRST_CAMERA_ROTATE_ANGLE,
    },
#endif

#ifdef ENABLE_SECOND_CAMERA
    {
        DSPG_SECOND_CAMERA_FACING,
        DSPG_SECOND_CAMERA_ROTATE_ANGLE,
    },
#endif
};

extern "C" int HAL_getNumberOfCameras()
{
    return sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
}

extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo *cameraInfo)
{
    LOGV("%s:%d", __func__,__LINE__);
    memcpy(cameraInfo, &sCameraInfo[cameraId], sizeof(CameraInfo));
}

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
{
    return CameraHardware::createInstance(cameraId);
}

}; // namespace android

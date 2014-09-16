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

#ifndef ANDROID_HARDWARE_CAMERA_HARDWARE_H
#define ANDROID_HARDWARE_CAMERA_HARDWARE_H

#include <utils/threads.h>
#include <camera/CameraHardwareInterface.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>

#include "V4L2Camera.h"
#include "Exif.h"

#ifdef SUPPORT_ZERO_COPY
#include "PostProcessor.h"
#endif

namespace android {

/* the footer is located right after every frame received from the ciu (very proprietary..) */
struct dspg_frame_footer {
    uint32_t smallWidth;
    uint32_t smallHeight;
    uint32_t reserved[14];
};

class CameraHardware : public CameraHardwareInterface {
public:
    virtual sp<IMemoryHeap> getPreviewHeap() const;
    virtual sp<IMemoryHeap> getRawHeap() const;

    virtual void        setCallbacks(notify_callback notify_cb,
                                     data_callback data_cb,
                                     data_callback_timestamp data_cb_timestamp,
                                     void *user);

    virtual void        enableMsgType(int32_t msgType);
    virtual void        disableMsgType(int32_t msgType);
    virtual bool        msgTypeEnabled(int32_t msgType);


    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled();

    virtual bool        useOverlay() { return true;}
//   virtual bool        useOverlay() { return false;}
    virtual status_t    setOverlay(const sp<Overlay> &overlay);

    virtual status_t    startRecording();
    virtual void        stopRecording();
//    virtual bool        recordingEnabled() { return true;}
//recording is asking qcif which is not supported
    virtual bool        recordingEnabled() { return mRecordRunning;}
    virtual void        releaseRecordingFrame(const sp<IMemory>& mem);

    //
    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();

    virtual status_t    takePicture();
    virtual status_t    cancelPicture();

    virtual status_t    dump(int fd, const Vector<String16> &args) const;
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters  getParameters() const;
    virtual status_t    sendCommand(int32_t command, int32_t arg1,
                                    int32_t arg2);
    virtual void release();

    static sp<CameraHardwareInterface> createInstance(int CameraId);

private:
                        CameraHardware(int cameraId);
    virtual             ~CameraHardware();

    static wp<CameraHardwareInterface> singleton;

   class PreviewThread : public Thread {
        CameraHardware *mHardware;
    public:
        PreviewThread(CameraHardware *hw):
        Thread(false),
        mHardware(hw) { }
        virtual void onFirstRef() {
            run("CameraPreviewThread", PRIORITY_URGENT_DISPLAY);
        }
        virtual bool threadLoop() {
            mHardware->previewThreadWrapper();
            return false;
        }
    };

    class PictureThread : public Thread {
        CameraHardware *mHardware;
    public:
        PictureThread(CameraHardware *hw):
        Thread(false),
        mHardware(hw) { }
        virtual bool threadLoop() {
            mHardware->pictureThread();
            return false;
        }
    };

	bool isSupportedPreviewSize(const int Width,const int Height) const;
    int previewThreadWrapper();
    void initDefaultParameters();
    int previewThread();
    int pictureThread();

    static int beginAutoFocusThread(void *cookie);

    int OverlayPostFrame(unsigned char *pPreviewFrame, int index);

	int getSnapshotImage(unsigned char **pPreviewFrame);

	//BS::ZeroCopy
	bool scaleDownYuv420(PostProcessor *pp,uint32_t srcPhysAddr, int32_t srcWidth, int32_t srcHeight,
		                 int32_t dstWidth, int32_t dstHeight);
	int getJpeg(JpegEncoder	*pJpegEncoder,int srcPhysAddr);

    void waitForBuffersRelease();
    int areThereBuffersOut();

    /* used by preview thread to block until it's told to run */
    mutable Mutex       mPreviewLock;
    mutable Condition   mPreviewCondition;
    mutable Condition   mPreviewStoppedCondition;
    bool        mPreviewRunning;
    bool        mExitPreviewThread;

    int                 mPreviewFrameSize;
	int 				mRawFrameSize;

    /* used for record */
    mutable Mutex       mLock;
    mutable Mutex       mRecordLock;
    bool 	mRecordRunning;
    Mutex	mRecordingLock;

    CameraParameters    mParameters;

	/* Callbacks */
    notify_callback     	mNotifyCb;
    data_callback       	mDataCb;
    data_callback_timestamp mDataCbTimestamp;
    void        			*mCallbackCookie;
    int32_t     mMsgEnabled;

    sp<PreviewThread>   mPreviewThread;

    sp<PictureThread>   mPictureThread;
    bool        mCaptureInProgress;

    /* used to guard threading state */
    mutable Mutex       mStateLock;

	/* memory heap*/
    sp<MemoryHeapBase>  mRecordHeap;

    // Holds buffers status: 0 - camera driver holds the buffer, 1 - application holds the buffer.
    int mBuffersOut[MAX_BUFFERS];

    //------------------------------------------------
    // Overlay
    sp<Overlay> mOverlay;
    bool        mUseOverlay;
	bool		mUseSmallPreviewFrame;

    //------------------------------------------------
    V4L2Camera		mCamera;
	int mCameraId;

    Vector<Size> mSupportedPreviewSizes;
};

}; // namespace android

#endif

/*
 * Copyright (C) 2009 The Android Open Source Project
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
#define LOG_TAG "DSPGHardwareRenderer"
#include <utils/Log.h>

#include "DSPGHardwareRenderer.h"

#include <media/stagefright/MediaDebug.h>
#include <surfaceflinger/ISurface.h>
#include <ui/Overlay.h>

#include <dspg_buffer.h>

namespace android {

int printOmxColorName(OMX_COLOR_FORMATTYPE colorFormat, char *name);

////////////////////////////////////////////////////////////////////////////////

DSPGHardwareRenderer::DSPGHardwareRenderer(
        const sp<ISurface> &surface,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight,
        OMX_COLOR_FORMATTYPE omxColorFormat, int32_t rotation)
    : mISurface(surface),
      mDisplayWidth(displayWidth),
      mDisplayHeight(displayHeight),
      mDecodedWidth(decodedWidth),
      mDecodedHeight(decodedHeight),
      mColorFormat(omxColorFormat),
      mInitCheck(NO_INIT),
      mFrameSize(mDecodedWidth * mDecodedHeight * 2),
      mIsFirstFrame(true),
      mIndex(0) 
{
//    CHECK(mISurface.get() != NULL);
//    CHECK(mDecodedWidth > 0);
//    CHECK(mDecodedHeight > 0);

	LOGV("DSPGHardwareRenderer::constructor begin");
	LOGV("decoded = %dx%d, display = %dx%d, color = %d, rotation = %d", decodedWidth, decodedHeight, displayWidth, displayHeight, omxColorFormat, rotation);

	int overlayColorFormat = -1;

#if !LOG_NDEBUG
	/* print color format name */
	char name[1024];
	printOmxColorName(omxColorFormat, name);
	LOGV("--> color format = %s", name);
#endif

	switch (omxColorFormat) {
	case OMX_COLOR_FormatYUV420SemiPlanar:
	case OMX_COLOR_FormatYUV420PackedSemiPlanar:
		overlayColorFormat = OVERLAY_FORMAT_YCbCr_420_SP_MPEG2;
		break;
	case OMX_COLOR_Format16bitRGB565:
		overlayColorFormat = OVERLAY_FORMAT_RGB_565;
		break;
	default:
		LOGE("unsupported omx color format %d", omxColorFormat);
		mInitCheck = -1;
		return;
	}

	switch(rotation) {
	case 0:
		break;
	case 90:
		rotation = OVERLAY_TRANSFORM_ROT_90;
		break;
	case 180:
		rotation = OVERLAY_TRANSFORM_ROT_180;
		break;
	case 270:
		rotation = OVERLAY_TRANSFORM_ROT_270;
		break;
	default:
		mInitCheck = -1;
		LOGE("cannot handle rotation parameter %d", rotation);
		return;
	}

	sp<OverlayRef> ref = mISurface->createOverlay(
			mDecodedWidth, mDecodedHeight,  overlayColorFormat , rotation);

    if (ref.get() == NULL) {
        LOGE("Unable to create the overlay!");
        return;
    }
	LOGV("Overlay created!!!");

    mOverlay = new Overlay(ref);

	LOGV("created overlay, buffercount = %d", mOverlay->getBufferCount());

	mInitCheck = OK;
}

DSPGHardwareRenderer::~DSPGHardwareRenderer() 
{
	LOGV("DSPGHardwareRenderer::destructor begin");

	if (mOverlay.get() != NULL) {
        mOverlay->destroy();
        mOverlay.clear();

        // XXX apparently destroying an overlay is an asynchronous process...
        sleep(1);
    }
}

void DSPGHardwareRenderer::render(const void *data, size_t size, void *platformPrivate) 
{
//	LOGV("[DSPGHardwareRenderer::render] begin");
//	struct timeval tv1, tv2;
//	gettimeofday(&tv1, NULL);

	overlay_buffer_t buffer;

	mOverlay->dequeueBuffer(&buffer);

#ifdef SUPPORT_ZERO_COPY
	struct dspg_buffer_t *dspg_buffer = (struct dspg_buffer_t*)buffer;

	if (dspg_buffer->allow_zero_copy) {

		// All buffers allocated by DWL ('data' in this function) have their bus address (physical address) in: addr - 8bytes.
		// give physical address to the post-processor
		unsigned int *bus_address = (unsigned int*)(data)-16;
		dspg_buffer->bus_address = *bus_address;

		// give virtual address to the osdm
		dspg_buffer->virt_address = (unsigned int)data;

//		LOGV("zero_copy: sending frame (bus_address = 0x%x, virt_address = 0x%x", dspg_buffer->bus_address, dspg_buffer->virt_address);
	}
	else
#endif
	{
		/* Use memcpy to pass the data */
		void *addr = mOverlay->getBufferAddress(buffer);
	
		if (addr == NULL) {
			LOGE("mOverlay->getBufferAddress returned NULL");
			return;
		}
	
		memcpy(addr, data, size);
	}

	mOverlay->queueBuffer(buffer);

//	gettimeofday(&tv2, NULL);
//	LOGV("render time: %d.%d",tv2.tv_sec - tv1.tv_sec, tv2.tv_usec - tv1.tv_usec);
}

int printOmxColorName(OMX_COLOR_FORMATTYPE colorFormat, char *name)
{
	switch (colorFormat) {
	case OMX_COLOR_Format8bitRGB332:
		strcpy(name, "OMX_COLOR_Format8bitRGB332");
		return 0;
	case OMX_COLOR_Format12bitRGB444:
		strcpy(name, "OMX_COLOR_Format12bitRGB444");
		return 0;
	case OMX_COLOR_Format16bitARGB4444:
		strcpy(name, "OMX_COLOR_Format16bitARGB4444");
		return 0;
	case OMX_COLOR_Format16bitARGB1555:
		strcpy(name, "OMX_COLOR_Format16bitARGB1555");
		return 0;
	case OMX_COLOR_Format16bitRGB565:
		strcpy(name, "OMX_COLOR_Format16bitRGB565");
		return 0;
	case OMX_COLOR_Format16bitBGR565:
		strcpy(name, "OMX_COLOR_Format16bitBGR565");
		return 0;
	case OMX_COLOR_Format18bitRGB666:
		strcpy(name, "OMX_COLOR_Format18bitRGB666");
		return 0;
	case OMX_COLOR_Format18bitARGB1665:
		strcpy(name, "OMX_COLOR_Format18bitARGB1665");
		return 0;
	case OMX_COLOR_Format19bitARGB1666:
		strcpy(name, "OMX_COLOR_Format19bitARGB1666");
		return 0;
	case OMX_COLOR_Format24bitRGB888:
		strcpy(name, "OMX_COLOR_Format24bitRGB888");
		return 0;
	case OMX_COLOR_Format24bitBGR888:
		strcpy(name, "OMX_COLOR_Format24bitBGR888");
		return 0;
	case OMX_COLOR_Format24bitARGB1887:
		strcpy(name, "OMX_COLOR_Format24bitARGB1887");
		return 0;
	case OMX_COLOR_Format25bitARGB1888:
		strcpy(name, "OMX_COLOR_Format25bitARGB1888");
		return 0;
	case OMX_COLOR_Format32bitBGRA8888:
		strcpy(name, "OMX_COLOR_Format32bitBGRA8888");
		return 0;
	case OMX_COLOR_Format32bitARGB8888:
		strcpy(name, "OMX_COLOR_Format32bitARGB8888");
		return 0;
	case OMX_COLOR_FormatYUV411Planar:
		strcpy(name, "OMX_COLOR_FormatYUV411Planar");
		return 0;
	case OMX_COLOR_FormatYUV411PackedPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV411PackedPlanar");
		return 0;
	case OMX_COLOR_FormatYUV420Planar:
		strcpy(name, "OMX_COLOR_FormatYUV420Planar");
		return 0;
	case OMX_COLOR_FormatYUV420PackedPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV420PackedPlanar");
		return 0;
	case OMX_COLOR_FormatYUV420SemiPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV420SemiPlanar");
		return 0;
	case OMX_COLOR_FormatYUV422Planar:
		strcpy(name, "OMX_COLOR_FormatYUV422Planar");
		return 0;
	case OMX_COLOR_FormatYUV422PackedPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV422PackedPlanar");
		return 0;
	case OMX_COLOR_FormatYUV422SemiPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV422SemiPlanar");
		return 0;
	case OMX_COLOR_FormatYUV420PackedSemiPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV420PackedSemiPlanar");
		return 0;
	case OMX_COLOR_FormatYUV422PackedSemiPlanar:
		strcpy(name, "OMX_COLOR_FormatYUV422PackedSemiPlanar");
		return 0;
	case OMX_COLOR_Format18BitBGR666:
		strcpy(name, "OMX_COLOR_Format18BitBGR666");
		return 0;
	case OMX_COLOR_Format24BitARGB6666:
		strcpy(name, "OMX_COLOR_Format24BitARGB6666");
		return 0;
	case OMX_COLOR_Format24BitABGR6666:
		strcpy(name, "OMX_COLOR_Format24BitABGR6666");
		return 0;
	default:
		strcpy(name, "Unknown");
		return 0;
	}

	return 0;
}

}  // namespace android



//#define LOG_NDEBUG 0
#define LOG_TAG "PostProcessor"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <utils/Timers.h>

#include <ppapi.h>
#include <hardware/overlay.h>

#include "PostProcessor.h"

PostProcessor::PostProcessor()
{
	LOGW("constructor");
	ppInst = NULL;
	hasSettings = false;
}

PostProcessor::~PostProcessor()
{
	LOGW("destructor");

	if (inputBuffer.size > 0) {
		freeBuffer(&inputBuffer);
	}

	if (outputBuffer.size > 0) {
		freeBuffer(&outputBuffer);
	}

	if (ppInst) {
		LOGW("destructor: calling PPRelease");
		PPRelease(ppInst);
	}
}

int PostProcessor::init()
{
	LOGW("init");
	PPResult res;

	res = PPInit(&ppInst);
	if (res != PP_OK) {
		LOGE("init: PPInit returned error %d",res);
		return -1;
	}

	memset(&ppConf, 0, sizeof(PPConfig));
	memset(&inputBuffer, 0, sizeof(DWLLinearMem_t));
	memset(&outputBuffer, 0, sizeof(DWLLinearMem_t));
	return 0;
}

int PostProcessor::allocBuffer(DWLLinearMem_t *buffer, unsigned int size)
{
	return PPMallocLinear(ppInst, size, buffer);
}

int PostProcessor::freeBuffer(DWLLinearMem_t *buffer)
{
	PPFreeLinear(ppInst, buffer);
	buffer->size = 0;

	return 0;
}

int PostProcessor::allocInputFrame(unsigned int format, unsigned int width, unsigned int height)
{
	int res;

	if (ppInst == NULL) {
		LOGE("allocInputFrame: ppInst not initialized, call init before");
		return -2;
	}

	// TODO: calculate the required size
	unsigned int requiredSize = width * height * 2;

	// If we already allocated an input buffer but its size is too small
	if (inputBuffer.size > 0 && inputBuffer.size < requiredSize) {
		freeBuffer(&inputBuffer);
	}

	if (inputBuffer.size == 0) {
		res = allocBuffer(&inputBuffer, requiredSize);
		if (res != 0) {
			LOGE("allocInputFrame: error allocating input frame (%d)",res);
			return -1;
		}
	}

	return setInputFrame(inputBuffer.busAddress, format, width, height);
}

int PostProcessor::setInputFrame(unsigned int inputFrameBusAddr, unsigned int format, unsigned int width, unsigned int height)
{
	LOGV("setInputFrame(0x%x, %d, %d, %d)", inputFrameBusAddr, format, width, height);
	unsigned int ppFormat;

	switch (format) {
	case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
	case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
		ppFormat = PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
		break;
	default:
		return -1;
	}

    /* Width and height of input picture to PP has to be always multiple of 16 pixels, 
    so aligning the height to 16-pixels which will fix issues resolutions like QQVGA(160x120) */
    /* !!To be fixed: Since we are not aligning width, resolutions like WVGA(854x480) will not work still */
    unsigned int alignedHeight = height & (0xffffffff-16+1);
    if (alignedHeight != height) {
        LOGD("Actual input height is modified to:%d", alignedHeight);
    }

	ppConf.ppInImg.bufferBusAddr = inputFrameBusAddr;
	ppConf.ppInImg.bufferCbBusAddr = inputFrameBusAddr + (width * height);
	ppConf.ppInImg.pixFormat = ppFormat;
	ppConf.ppInImg.width = width;
	ppConf.ppInImg.height = alignedHeight /*height*/;

	return 0;
}

int PostProcessor::allocOutputFrame(unsigned int format, unsigned int width, unsigned int height)
{
	int res;

	if (ppInst == NULL) {
		LOGE("allocOutputFrame: ppInst not initialized, call init before");
		return -2;
	}

	// TODO: calculate the required size
	unsigned int requiredSize = width * height * 2;

	// If we already allocated an output buffer but its size is too small
	if (outputBuffer.size > 0 && outputBuffer.size < requiredSize) {
		freeBuffer(&outputBuffer);
	}

	if (outputBuffer.size == 0) {
		res = allocBuffer(&outputBuffer, requiredSize);
		if (res != 0) {
			LOGE("allocOutputFrame: error allocating output frame (%d)",res);
			return -1;
		}
	}

	return setOutputFrame(outputBuffer.busAddress, format, width, height);
}

int PostProcessor::setOutputFrame(unsigned int outputFrameBusAddr, unsigned int format, unsigned int width, unsigned int height)
{
	unsigned int ppFormat;
	LOGV("setOutputFrame(0x%x, %d, %d, %d)", outputFrameBusAddr, format, width, height);

	switch (format) {
	case OVERLAY_FORMAT_YCbCr_420_SP_MPEG2:
	case OVERLAY_FORMAT_YCbCr_420_SP_JPEG_MPEG1:
		ppFormat = PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
		break;
	default:
		return -1;
	}
	
	ppConf.ppOutImg.bufferBusAddr = outputFrameBusAddr;
	ppConf.ppOutImg.bufferChromaBusAddr = outputFrameBusAddr + (width*height);
	ppConf.ppOutImg.pixFormat = ppFormat;
	ppConf.ppOutImg.width = width;
	ppConf.ppOutImg.height = height;
	
	return 0;	
}

int PostProcessor::setHorizontalFlip(int doFlip)
{
    if (ppInst == NULL) {
        LOGE("setHorizontalFlip: ppInst not initialized, call init before");
        return -2;
    }

    if (doFlip) 
        ppConf.ppInRotation.rotation |= PP_ROTATION_HOR_FLIP;
    else
        ppConf.ppInRotation.rotation &= ~PP_ROTATION_HOR_FLIP;

    return 0;
}

int PostProcessor::setVerticalFlip(int doFlip)
{
    if (ppInst == NULL) {
        LOGE("setVerticalFlip: ppInst not initialized, call init before");
        return -2;
    }

    if (doFlip) 
        ppConf.ppInRotation.rotation |= PP_ROTATION_VER_FLIP;
    else
        ppConf.ppInRotation.rotation &= ~PP_ROTATION_VER_FLIP;

    return 0;
}

void* PostProcessor::getInputFrameVirtAddr()
{
	LOGV("getInputFrameVirtAddr: %p", inputBuffer.virtualAddress);
	return (void*)inputBuffer.virtualAddress;
}

void* PostProcessor::getOutputFrameVirtAddr()
{
	LOGV("getOutputFrameVirtAddr: %p", outputBuffer.virtualAddress);
	return (void*)outputBuffer.virtualAddress;
}

uint32_t PostProcessor::getOutputFrameBusAddr()
{
	return (uint32_t)outputBuffer.busAddress;
}

int PostProcessor::commitSettings()
{
	LOGV("commitSettings()");
	PPResult res;

	if (ppInst == NULL) {
		LOGE("commitSettings: ppInst not initialized, call init before");
		return -2;
	}


#ifdef MAINTAIN_ASPECT_RATIO
	// use FILL algorithm to maintain input picture aspect ratio and crop the input picture according to output dimensions
	int ret = setInputCropParamsForFill();
	if (ret == 0) {
		// enable input crop
		ppConf.ppInCrop.enable = 1;
	}
	else {
		ppConf.ppInCrop.enable = 0;
	}
#endif

	res = PPSetConfig(ppInst, &ppConf);
	if (res != PP_OK) {
		LOGE("commitSettings: PPSetConfig returned error %d", res);
		LOGE("commitSettings: Post-Processor config:");
		LOGE("input(%dx%d)\noutput(%dx%d)\nflip(%dx%d)\ncrop(active=%d, origin=%dx%d, size=%dx%d)", 
			 ppConf.ppInImg.width, ppConf.ppInImg.height,
			 ppConf.ppOutImg.width, ppConf.ppOutImg.height,
			 ppConf.ppInRotation.rotation&PP_ROTATION_HOR_FLIP, ppConf.ppInRotation.rotation&PP_ROTATION_VER_FLIP,
			 ppConf.ppInCrop.enable, ppConf.ppInCrop.originX, ppConf.ppInCrop.originY, ppConf.ppInCrop.width, ppConf.ppInCrop.height);
		return -1;
	}

	hasSettings = true;
	return 0;
}

int PostProcessor::execute()
{
	LOGV("execute()");
	PPResult res;

	if (!hasSettings) {
		LOGE("execute: no settings have been set. use commitSettings before this call");
		return -1;
	}

#ifdef TIME_PP
	nsecs_t t1,t2;
	t1 = systemTime();
#endif

	res = PPGetResult(ppInst);
	if (res != PP_OK) {
		LOGE("execute: PPGetResult returned error: %d", res);
		return -1;
	}

#ifdef TIME_PP
	t2 = systemTime();
	LOGV("PostProcessor::execute input(%dx%d) output(%dx%d) flip(%dx%d) crop(active=%d, origin=%dx%d, size=%dx%d) = %llu us", 
		 ppConf.ppInImg.width, ppConf.ppInImg.height,
		 ppConf.ppOutImg.width, ppConf.ppOutImg.height,
		 ppConf.ppInRotation.rotation&PP_ROTATION_HOR_FLIP, ppConf.ppInRotation.rotation&PP_ROTATION_VER_FLIP,
		 ppConf.ppInCrop.enable, ppConf.ppInCrop.originX, ppConf.ppInCrop.originY, ppConf.ppInCrop.width, ppConf.ppInCrop.height,
		 (t2-t1) / 1000);
#endif

	return 0;
}

int PostProcessor::setInputCropParamsForFill()
{
	float wFactor = (float)ppConf.ppInImg.width/ppConf.ppOutImg.width;
	float hFactor = (float)ppConf.ppInImg.height/ppConf.ppOutImg.height;
	if (wFactor < hFactor) {
		// crop according to width (calculate crop height)
		ppConf.ppInCrop.width = ppConf.ppInImg.width & (0xffffffff - ALIGN_CROP_SIZE + 1);
		ppConf.ppInCrop.originX = 0;

		ppConf.ppInCrop.height = (int)(wFactor*ppConf.ppOutImg.height) & (0xffffffff - ALIGN_CROP_SIZE + 1);
		int startY = (ppConf.ppInImg.height - ppConf.ppInCrop.height)>>1;
		ppConf.ppInCrop.originY = (startY & (0xffffffff - ALIGN_CROP_START + 1)) + (((startY%ALIGN_CROP_START) & (0xffffffff - (ALIGN_CROP_START>>1) + 1)) << 1);
	}
	else {
		// crop according to height (calculate crop width)
		ppConf.ppInCrop.height = ppConf.ppInImg.height & (0xffffffff - ALIGN_CROP_SIZE + 1);
		ppConf.ppInCrop.originY = 0;

		ppConf.ppInCrop.width = (int)(ppConf.ppOutImg.width*hFactor) & (0xffffffff - ALIGN_CROP_SIZE + 1);
		LOGV("crop width = %d (hFactor = %f)", ppConf.ppInCrop.width, hFactor);
		int startX = (ppConf.ppInImg.width - ppConf.ppInCrop.width)>>1;
		ppConf.ppInCrop.originX = (startX & (0xffffffff - ALIGN_CROP_START + 1)) + (((startX%ALIGN_CROP_START) & (0xffffffff - (ALIGN_CROP_START>>1) + 1)) << 1);
	}

	/* check that we are ok with the scaler upscaling limitations (width=*3, height=*3-2)
	   if not, dont use crop */
	if (ppConf.ppOutImg.width > ppConf.ppInCrop.width * 3 ||
		ppConf.ppOutImg.height > ppConf.ppInCrop.height * 3 - 2) {
		LOGV("warning: cannot use crop to maintain aspect ratio");
		return -1;
	}

	return 0;
}

/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  Implementation for H.263
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_encoder_h263"
#include <utils/Log.h>


#include "encoder_h263.h"
#include "util.h" // Q16_FLOAT
#include "mp4encapi.h"
#include "OSAL.h"
#include <assert.h>
#include <string.h>
#include <math.h>

#if !defined (ENC6280) && !defined (ENC7280)
#error "SPECIFY AN ENCODER PRODUCT (ENC6280 or ENC7280) IN COMPILER DEFINES!"
#endif

typedef struct ENCODER_H263
{
    ENCODER_PROTOTYPE base;

    MP4EncInst instance;
    MP4EncIn encIn;
    OMX_U32 origWidth;
    OMX_U32 origHeight;
    OMX_U32 nPFrames;
    OMX_U32 nIFrameCounter;
    OMX_U32 nTickIncrement;
    OMX_U32 nAllowedPictureTypes;
    OMX_U32 nTotalFrames;
    OMX_BOOL stabilize;
} ENCODER_H263;

// destroy codec instance
static void encoder_destroy_h263(ENCODER_PROTOTYPE* arg)
{
    ENCODER_H263* this = (ENCODER_H263*)arg;
    if ( this)
    {
        this->base.stream_start = 0;
        this->base.stream_end = 0;
        this->base.encode = 0;
        this->base.destroy = 0;

        if (this->instance)
        {
            MP4EncRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free( this);
    }
}

static CODEC_STATE encoder_stream_start_h263(ENCODER_PROTOTYPE* arg,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);
    ENCODER_H263* this = (ENCODER_H263*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufBusAddress = stream->bus_address;
    this->encIn.outBufSize = stream->buf_max_size;
    this->nTotalFrames = 0;

    MP4EncOut encOut;
    MP4EncRet ret = MP4EncStrmStart(this->instance, &this->encIn, &encOut);

    switch (ret)
    {
    case ENC_OK:
    {
        stream->streamlen = encOut.strmSize;
        stat = CODEC_OK;
    }
        break;
    case ENC_NULL_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INSTANCE_ERROR:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    case ENC_INVALID_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INVALID_STATUS:
    {
        stat = CODEC_ERROR_INVALID_STATE;
    }
    break;
    case ENC_OUTPUT_BUFFER_OVERFLOW:
    {
        stat = CODEC_ERROR_BUFFER_OVERFLOW;
    }
    break;
    default:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    }

    return stat;
}

static CODEC_STATE encoder_stream_end_h263(ENCODER_PROTOTYPE* arg,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);
    ENCODER_H263* this = (ENCODER_H263*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufSize = stream->buf_max_size;
    this->encIn.outBufBusAddress = stream->bus_address;

    MP4EncOut encOut;
    MP4EncRet ret = MP4EncStrmEnd(this->instance, &this->encIn, &encOut);

    switch (ret)
    {
    case ENC_OK:
    {
        stream->streamlen = encOut.strmSize;
        stat = CODEC_OK;
    }
    break;
    case ENC_NULL_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INSTANCE_ERROR:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    case ENC_INVALID_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INVALID_STATUS:
    {
        stat = CODEC_ERROR_INVALID_STATE;
    }
    break;
    default:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    }

    return stat;
}

// NOTE This needs to support changing coding control settings during runtime.
// NOTE Otherwise GOV headers can't be inserted into stream.
static CODEC_STATE encoder_encode_h263(ENCODER_PROTOTYPE* arg, FRAME* frame,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(frame);
    assert(stream);
    ENCODER_H263* this = (ENCODER_H263*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    MP4EncRet ret;

    // Planar YCbCr 4:2:0
    this->encIn.busLuma = frame->fb_bus_address;
    this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth * this->origHeight);
    this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth * this->origHeight / 4);
    // Semiplanar YCbCr 4:2:0
    // Interleaved YCbCr 4:2:2

    // Optimal size for output buffer size is the size of the VBV buffer (as specified by standard)
    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufBusAddress = stream->bus_address;
    this->encIn.outBufSize = stream->buf_max_size;

    if (frame->frame_type == INTRA_FRAME)
    {
        this->encIn.vopType = INTRA_VOP;
    }
    else if (frame->frame_type == PREDICTED_FRAME)
    {
        if (this->nPFrames > 0)
        {
            if ((this->nIFrameCounter % this->nPFrames) == 0)
            {
                this->encIn.vopType = INTRA_VOP;
                this->nIFrameCounter = 0;
            }
            else
            {
                this->encIn.vopType = PREDICTED_VOP;
            }
            this->nIFrameCounter++;
        }
        else
        {
            this->encIn.vopType = PREDICTED_VOP;
        }
    }
    else
    {
        assert( 0);
    }

    // The time stamp of the frame relative to the last encoded frame. This is given in
    // ticks. A zero value is usually used for the very first frame.
    // Valid value range: [0, 127*time resolution]
    this->encIn.timeIncr = this->nTickIncrement;

    // Pointer to a buffer where the individual video package’s sizes will be returned. Set
    // to NULL if the information is not needed. This buffer has to have sizeof(u32)
    // bytes for each macroblock of the encoded picture. A zero value is written after the
    // last relevant VP size (when buffer big enough). Maximum size of 14404 bytes for
    // HD 720p resolution (when sizeof(u32) = 4).
    this->encIn.pVpBuf = NULL;

    // This is the size in bytes of the video package size buffer described above.
    this->encIn.vpBufSize = 0;

    // The bus address of the luminance component buffer of the picture to be stabilized.
    // Used only when video stabilization is enabled.
    if ((this->stabilize == OMX_TRUE) && (frame->fb_bufferSize == (2 * frame->fb_frameSize)))
    {
        // This should point to next frame in buffer
        this->encIn.busLumaStab = frame->fb_bus_address + frame->fb_frameSize;
    }
    else
    {
        this->encIn.busLumaStab = 0;
    }

    MP4EncOut encOut;

    if (this->nTotalFrames == 0)
    {
        this->encIn.timeIncr = 0;
    }

    MP4EncRateCtrl rate_ctrl;
    ret = MP4EncGetRateCtrl(this->instance, &rate_ctrl);

    if (ret == ENC_OK && (rate_ctrl.bitPerSecond != frame->bitrate))
    {
        rate_ctrl.bitPerSecond = frame->bitrate;
        ret = MP4EncSetRateCtrl(this->instance, &rate_ctrl);
    }

    if (ret == ENC_OK)
        ret = MP4EncStrmEncode(this->instance, &this->encIn, &encOut);


    switch (ret)
    {
    case ENC_VOP_READY:
    {
        this->nTotalFrames++;
        stream->streamlen = encOut.strmSize;

        if (encOut.vopType == INTRA_VOP)
        {
            stat = CODEC_CODED_INTRA;
        }
        else if (encOut.vopType == PREDICTED_VOP)
        {
            stat = CODEC_CODED_PREDICTED;
        }
        else
        {
            stat = CODEC_OK;
        }
    }
    break;
    case ENC_NULL_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INSTANCE_ERROR:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    case ENC_INVALID_ARGUMENT:
    {
        stat = CODEC_ERROR_INVALID_ARGUMENT;
    }
    break;
    case ENC_INVALID_STATUS:
    {
        stat = CODEC_ERROR_INVALID_STATE;
    }
    break;
    case ENC_OUTPUT_BUFFER_OVERFLOW:
    {
        stat = CODEC_ERROR_BUFFER_OVERFLOW;
    }
    break;
    case ENC_HW_TIMEOUT:
    {
        stat = CODEC_ERROR_HW_TIMEOUT;
    }
    break;
    case ENC_HW_BUS_ERROR:
    {
        stat = CODEC_ERROR_HW_BUS_ERROR;
    }
    break;
    case ENC_HW_RESET:
    {
        stat = CODEC_ERROR_HW_RESET;
    }
    break;
    case ENC_SYSTEM_ERROR:
    {
        stat = CODEC_ERROR_SYSTEM;
    }
    break;
    case ENC_HW_RESERVED:
    {
        stat = CODEC_ERROR_RESERVED;
    }
    break;
    default:
    {
        stat = CODEC_ERROR_UNSPECIFIED;
    }
    break;
    }

    return stat;
}

// create codec instance and initialize it
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_h263(const H263_CONFIG* params)
{
    MP4EncCfg cfg;

    cfg.strmType = H263_STRM;

    // Only baseline profile is supported.
    if (params->h263_config.eProfile != OMX_VIDEO_H263ProfileBaseline)
    {
        return NULL;
    }
    /*
    OMX_VIDEO_H263ProfileBaseline
    OMX_VIDEO_H263ProfileH320Coding
    OMX_VIDEO_H263ProfileBackwardCompatible
    OMX_VIDEO_H263ProfileISWV2
    OMX_VIDEO_H263ProfileISWV3
    OMX_VIDEO_H263ProfileHighCompression
    OMX_VIDEO_H263ProfileInternet
    OMX_VIDEO_H263ProfileInterlace
    OMX_VIDEO_H263ProfileHighLatency
    */

    // define H263 level
    switch (params->h263_config.eLevel) {
    case OMX_VIDEO_H263Level10:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_10;
        break;
    case OMX_VIDEO_H263Level20:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_20;
        break;
    case OMX_VIDEO_H263Level30:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_30;
        break;
    case OMX_VIDEO_H263Level40:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_40;
        break;
    case OMX_VIDEO_H263Level50:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_50;
        break;
    case OMX_VIDEO_H263Level60:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_60;
        break;
    case OMX_VIDEO_H263Level70:
        cfg.profileAndLevel = H263_PROFILE_0_LEVEL_70;
        break;
    default:
        return NULL;
    }

    cfg.width = params->common_config.nOutputWidth;
    cfg.height = params->common_config.nOutputHeight;


    // In H.263 streams the frame rate is relative to the (30 000)/1001 frame rate. So, to avoid
    // rounding errors please use ‘30 000’ for the time resolution (numerator) and multiples of
    // 1001 for the frame rate denominator.
    cfg.frmRateNum = 30000;

    double intPart;
    double frac = modf((30 / Q16_FLOAT(params->common_config.nInputFramerate)), &intPart);
    if (frac >= 0.5)
    {
        intPart++;
    }
    cfg.frmRateDenom = intPart * 1001;

    ENCODER_H263* this = OSAL_Malloc(sizeof(ENCODER_H263));

    this->instance = 0;
    memset( &this->encIn, 0, sizeof(MP4EncIn));
    this->origWidth = params->pp_config.origWidth;
    this->origHeight = params->pp_config.origHeight;
    this->base.stream_start = encoder_stream_start_h263;
    this->base.stream_end = encoder_stream_end_h263;
    this->base.encode = encoder_encode_h263;
    this->base.destroy = encoder_destroy_h263;

    this->nIFrameCounter = 0;

    this->nPFrames = params->h263_config.nPFrames;

    // Allowed picture types
    // OMX_VIDEO_PictureTypeI   = 0x01
    // OMX_VIDEO_PictureTypeP   = 0x02
    // OMX_VIDEO_PictureTypeB   = 0x04 (not supported by hantro)
    // OMX_VIDEO_PictureTypeS   = 0x14 (not supported by hantro)

    this->nTickIncrement = cfg.frmRateDenom;

    MP4EncRet ret = MP4EncInit(&cfg, &this->instance);

    // Setup coding control
    if (ret == ENC_OK)
    {
        MP4EncCodingCtrl coding_ctrl;
        ret = MP4EncGetCodingCtrl(this->instance, &coding_ctrl);

        if (ret == ENC_OK)
        {
            // Header extension codes enable/disable
            if (params->error_ctrl_config.bEnableHEC)
            {
                coding_ctrl.insHEC = 1;
            }
            else
            {
                coding_ctrl.insHEC = 0;
            }

            // Video package (VP) size
            if (params->error_ctrl_config.nResynchMarkerSpacing > 0)
            {
                coding_ctrl.vpSize = params->error_ctrl_config.nResynchMarkerSpacing;
            }
            // else use default valuee

            ret = MP4EncSetCodingCtrl(this->instance, &coding_ctrl);
        }
    }

    // Setup rate control
    if (ret == ENC_OK)
    {
        MP4EncRateCtrl rate_ctrl;
        ret = MP4EncGetRateCtrl(this->instance, &rate_ctrl);

        if (ret == ENC_OK)
        {
            rate_ctrl.gopLen = params->h263_config.nPFrames;
            rate_ctrl.vopRc = params->rate_config.nPictureRcEnabled;
            rate_ctrl.mbRc = params->rate_config.nMbRcEnabled;

            rate_ctrl.qpHdr = params->rate_config.nQpDefault;
            rate_ctrl.qpMin = params->rate_config.nQpMin;
            rate_ctrl.qpMax = params->rate_config.nQpMax;

            if (rate_ctrl.qpHdr < rate_ctrl.qpMin)
            {
                rate_ctrl.qpHdr = rate_ctrl.qpMin;
            }
            else if (rate_ctrl.qpHdr > rate_ctrl.qpMax)
            {
                rate_ctrl.qpHdr = rate_ctrl.qpMax;
            }

            rate_ctrl.bitPerSecond = params->rate_config.nTargetBitrate;
            rate_ctrl.vbv = params->rate_config.nVbvEnabled;

            switch (params->rate_config.eRateControl)
            {
            case OMX_Video_ControlRateDisable:
                rate_ctrl.vopSkip = 0;
                break;
            case OMX_Video_ControlRateVariable:
                rate_ctrl.vopSkip = 0;
                break;
            case OMX_Video_ControlRateConstant:
                rate_ctrl.vopSkip = 0;
                break;
            case OMX_Video_ControlRateVariableSkipFrames:
                rate_ctrl.vopSkip = 1;
                break;
            case OMX_Video_ControlRateConstantSkipFrames:
                rate_ctrl.vopSkip = 1;
                break;
            case OMX_Video_ControlRateMax:
                rate_ctrl.vopSkip = 0;
                break;
            default:
                break;
            }
            ret = MP4EncSetRateCtrl(this->instance, &rate_ctrl);
        }
    }

    // Setup preprocessing
    if (ret == ENC_OK)
    {
        MP4EncPreProcessingCfg pp_config;

        ret = MP4EncGetPreProcessing(this->instance, &pp_config);

        // input image size
        pp_config.origWidth = params->pp_config.origWidth;
        pp_config.origHeight = params->pp_config.origHeight;

        // cropping offset
        pp_config.xOffset = params->pp_config.xOffset;
        pp_config.yOffset = params->pp_config.yOffset;

        switch (params->pp_config.formatType)
        {
        case OMX_COLOR_FormatYUV420PackedPlanar:
        case OMX_COLOR_FormatYUV420Planar:
#ifdef ENC6280
            pp_config.yuvType = ENC_YUV420_PLANAR;
#endif
#ifdef ENC7280
            pp_config.inputType = ENC_YUV420_PLANAR;
#endif
            break;
        case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
#ifdef ENC6280
            pp_config.yuvType = ENC_YUV420_SEMIPLANAR;
#endif
#ifdef ENC7280
            pp_config.inputType = ENC_YUV420_SEMIPLANAR;
#endif
            break;
        case OMX_COLOR_FormatYCbYCr:
#ifdef ENC6280
            pp_config.yuvType = ENC_YUV422_INTERLEAVED_YUYV;
#endif
#ifdef ENC7280
            pp_config.inputType = ENC_YUV422_INTERLEAVED_YUYV;
#endif
            break;
        case OMX_COLOR_FormatCbYCrY:
#ifdef ENC6280
            pp_config.yuvType = ENC_YUV422_INTERLEAVED_UYVY;
#endif
#ifdef ENC7280
            pp_config.inputType = ENC_YUV422_INTERLEAVED_UYVY;
#endif
            break;
        default:
            ret = ENC_INVALID_ARGUMENT;
            break;
        }

        switch (params->pp_config.angle)
        {
        case 0:
            pp_config.rotation = ENC_ROTATE_0;
            break;
        case 90:
            pp_config.rotation = ENC_ROTATE_90R;
            break;
        case 270:
            pp_config.rotation = ENC_ROTATE_90L;
            break;
        default:
            ret = ENC_INVALID_ARGUMENT;
            break;
        }

        // Enables or disables the video stabilization function. Set to a non-zero value will
        // enable the stabilization. The input image’s dimensions (origWidth, origHeight)
        // have to be at least 8 pixels bigger than the final encoded image’s. Also when
        // enabled the cropping offset (xOffset, yOffset) values are ignored.
        this->stabilize = params->pp_config.frameStabilization;
        pp_config.videoStabilization = params->pp_config.frameStabilization;

        if (ret == ENC_OK)
        {
            ret = MP4EncSetPreProcessing(this->instance, &pp_config);
        }
    }

    if (ret != ENC_OK)
    {
        OSAL_Free(this);
        return NULL;
    }

    return (ENCODER_PROTOTYPE*) this;
}

CODEC_STATE HantroHwEncOmx_encoder_frame_rate_h263(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate)
{
    ENCODER_H263* this = (ENCODER_H263*)arg;

    double intPart;
    double frac = modf((30 / Q16_FLOAT(xFramerate)), &intPart);
    if (frac >= 0.5)
    {
        intPart++;
    }

    this->nTickIncrement = intPart * 1001;

    return CODEC_OK;
}

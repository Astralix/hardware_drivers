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
--  Description : Implementation for H264
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "encoder_h264"
#include <utils/Log.h>

#include "encoder_h264.h"
#include "util.h" // Q16_FLOAT
#include "h264encapi.h"
#include "OSAL.h"
#include <assert.h>
#include <string.h>


#if !defined (ENC6280) && !defined (ENC7280) && !defined (ENC8270) && !defined (ENC8290) && !defined (ENCH1)
#error "SPECIFY AN ENCODER PRODUCT (ENC6280, ENC7280, ENC8270, ENC8290 OR ENCH1) IN COMPILER DEFINES!"
#endif

typedef struct ENCODER_H264
{
    ENCODER_PROTOTYPE base;

    H264EncInst instance;
    H264EncIn encIn;
    OMX_U32 origWidth;
    OMX_U32 origHeight;
    OMX_U32 nEstTimeInc;
    OMX_U32 bStabilize;
    OMX_U32 nIFrameCounter;
    OMX_U32 nPFrames;
    OMX_U32 nTotalFrames;
} ENCODER_H264;

#if defined (ENC8290) || defined (ENCH1)
i32 testParam;
#endif

// destroy codec instance
static void encoder_destroy_h264(ENCODER_PROTOTYPE* arg)
{
    ENCODER_H264* this = (ENCODER_H264*)arg;
    if ( this)
    {
        this->base.stream_start = 0;
        this->base.stream_end = 0;
        this->base.encode = 0;
        this->base.destroy = 0;

        if (this->instance)
        {
            H264EncRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free( this);
    }
}

static CODEC_STATE encoder_stream_start_h264(ENCODER_PROTOTYPE* arg,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);
    ENCODER_H264* this = (ENCODER_H264*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufSize = stream->buf_max_size;
    this->encIn.busOutBuf = stream->bus_address;
    this->nTotalFrames = 0;

    H264EncOut encOut;
    H264EncRet ret = H264EncStrmStart(this->instance, &this->encIn, &encOut);

    switch (ret)
    {
        case H264ENC_OK:
        {
            stream->streamlen = encOut.streamSize;
            stat = CODEC_OK;
        }
        break;
        case H264ENC_NULL_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INSTANCE_ERROR:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
        case H264ENC_INVALID_ARGUMENT:
        {

            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INVALID_STATUS:
        {
            stat = CODEC_ERROR_INVALID_STATE;
        }
        break;
        case H264ENC_OUTPUT_BUFFER_OVERFLOW:
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

static CODEC_STATE encoder_stream_end_h264(ENCODER_PROTOTYPE* arg,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(stream);
    ENCODER_H264* this = (ENCODER_H264*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufSize = stream->buf_max_size;
    this->encIn.busOutBuf = stream->bus_address;

    H264EncOut encOut;
    H264EncRet ret = H264EncStrmEnd(this->instance, &this->encIn, &encOut);

    switch (ret)
    {
        case H264ENC_OK:
        {
            stream->streamlen = encOut.streamSize;
            stat = CODEC_OK;
        }
        break;
        case H264ENC_NULL_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INSTANCE_ERROR:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
        case H264ENC_INVALID_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INVALID_STATUS:
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

static CODEC_STATE encoder_encode_h264(ENCODER_PROTOTYPE* arg, FRAME* frame,
        STREAM_BUFFER* stream)
{
    assert(arg);
    assert(frame);
    assert(stream);
    ENCODER_H264* this = (ENCODER_H264*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->encIn.busLuma = frame->fb_bus_address;
    this->encIn.busChromaU = frame->fb_bus_address + (this->origWidth
            * this->origHeight);
    this->encIn.busChromaV = this->encIn.busChromaU + (this->origWidth
            * this->origHeight / 4);

    this->encIn.timeIncrement = this->nEstTimeInc;

    this->encIn.pOutBuf = (u32 *) stream->bus_data;
    this->encIn.outBufSize = stream->buf_max_size;
    this->encIn.busOutBuf = stream->bus_address;
#if !defined (ENC8290) && !defined (ENCH1)
    this->encIn.pNaluSizeBuf = NULL;
    this->encIn.naluSizeBufSize = 0;
#endif
    // The bus address of the luminance component buffer of the picture to be stabilized.
    // Used only when video stabilization is enabled.
    if (this->bStabilize && (frame->fb_bufferSize == (2 * frame->fb_frameSize))) {
        this->encIn.busLumaStab = frame->fb_bus_address + frame->fb_frameSize;
    }
    else
    {
        this->encIn.busLumaStab = 0;
    }

    if (frame->frame_type == INTRA_FRAME)
    {
        this->encIn.codingType = H264ENC_INTRA_FRAME;
    }
    else if (frame->frame_type == PREDICTED_FRAME)
    {
        if (this->nPFrames > 0)
        {
            if ((this->nIFrameCounter % this->nPFrames) == 0)
            {
                this->encIn.codingType = H264ENC_INTRA_FRAME;
                this->nIFrameCounter = 0;
            }
            else
            {
                this->encIn.codingType = H264ENC_PREDICTED_FRAME;
            }
            this->nIFrameCounter++;
        }
        else
        {
            this->encIn.codingType = H264ENC_INTRA_FRAME;
        }
    }
    else
    {
        assert( 0);
    }

    if (this->nTotalFrames == 0)
    {
        this->encIn.timeIncrement = 0;
    }

    H264EncRet ret;
    H264EncOut encOut;
    H264EncRateCtrl rate_ctrl;

    ret = H264EncGetRateCtrl(this->instance, &rate_ctrl);

    if (ret == H264ENC_OK && (rate_ctrl.bitPerSecond != frame->bitrate))
    {
        rate_ctrl.bitPerSecond = frame->bitrate;
        ret = H264EncSetRateCtrl(this->instance, &rate_ctrl);
    }

    if (ret == H264ENC_OK)
#if !defined (ENC8290) && !defined (ENCH1)
        ret = H264EncStrmEncode(this->instance, &this->encIn, &encOut);
#else
        ret = H264EncStrmEncode(this->instance, &this->encIn, &encOut, NULL, NULL);
#endif
    switch (ret)
    {
        case H264ENC_FRAME_READY:
        {
            this->nTotalFrames++;

            stream->streamlen = encOut.streamSize;

            if (encOut.codingType == H264ENC_INTRA_FRAME)
            {
                stat = CODEC_CODED_INTRA;
            }
            else if (encOut.codingType == H264ENC_PREDICTED_FRAME)
            {
                stat = CODEC_CODED_PREDICTED;
            }
            else
            {
                stat = CODEC_OK;
            }
        }
        break;
        case H264ENC_NULL_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INSTANCE_ERROR:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
        case H264ENC_INVALID_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INVALID_STATUS:
        {
            stat = CODEC_ERROR_INVALID_STATE;
        }
        break;
        case H264ENC_OUTPUT_BUFFER_OVERFLOW:
        {
            stat = CODEC_ERROR_BUFFER_OVERFLOW;
        }
        break;
        case H264ENC_HW_TIMEOUT:
        {
            stat = CODEC_ERROR_HW_TIMEOUT;
        }
        break;
        case H264ENC_HW_BUS_ERROR:
        {
            stat = CODEC_ERROR_HW_BUS_ERROR;
        }
        break;
        case H264ENC_HW_RESET:
        {
            stat = CODEC_ERROR_HW_RESET;
        }
        break;
        case H264ENC_SYSTEM_ERROR:
        {
            stat = CODEC_ERROR_SYSTEM;
        }
        break;
        case H264ENC_HW_RESERVED:
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
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_h264(const H264_CONFIG* params)
{
    H264EncConfig cfg;

    memset(&cfg,0,sizeof(H264EncConfig));

#if defined (ENC8290) || defined (ENCH1)
    if ( (params->h264_config.eProfile != OMX_VIDEO_AVCProfileBaseline) &&
        (params->h264_config.eProfile != OMX_VIDEO_AVCProfileMain) &&
        (params->h264_config.eProfile != OMX_VIDEO_AVCProfileHigh) )
    {
        return NULL;
    }
#else
    if (params->h264_config.eProfile != OMX_VIDEO_AVCProfileBaseline)
    {
		LOGE("[%s] only baseline profile is supported (requested profile: %d)", __FUNCTION__, params->h264_config.eProfile);
        return NULL;
    }
#endif

    switch (params->h264_config.eLevel)
    {
#if defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
        case OMX_VIDEO_AVCLevel1:
            cfg.level = H264ENC_LEVEL_1;
            break;
        case OMX_VIDEO_AVCLevel1b:
            cfg.level = H264ENC_LEVEL_1_b;
            break;
        case OMX_VIDEO_AVCLevel11:
            cfg.level = H264ENC_LEVEL_1_1;
            break;
        case OMX_VIDEO_AVCLevel12:
            cfg.level = H264ENC_LEVEL_1_2;
            break;
        case OMX_VIDEO_AVCLevel13:
            cfg.level = H264ENC_LEVEL_1_3;
            break;
        case OMX_VIDEO_AVCLevel2:
            cfg.level = H264ENC_LEVEL_2;
            break;
        case OMX_VIDEO_AVCLevel21:
            cfg.level = H264ENC_LEVEL_2_1;
            break;
        case OMX_VIDEO_AVCLevel22:
            cfg.level = H264ENC_LEVEL_2_2;
            break;
        case OMX_VIDEO_AVCLevel3:
            cfg.level = H264ENC_LEVEL_3;
            break;
        case OMX_VIDEO_AVCLevel31:
            cfg.level = H264ENC_LEVEL_3_1;
            break;
        case OMX_VIDEO_AVCLevel32:
            cfg.level = H264ENC_LEVEL_3_2;
            break;
        case OMX_VIDEO_AVCLevel4:
#ifdef ENC8270
            cfg.level = H264ENC_LEVEL_4_0;
#else
            cfg.level = H264ENC_LEVEL_4;
#endif
            break;

#if defined (ENC8290) || defined (ENCH1)
        case OMX_VIDEO_AVCLevel41:
            cfg.level = H264ENC_LEVEL_4_1;
            break;
        case OMX_VIDEO_AVCLevel42:
            cfg.level = H264ENC_LEVEL_4_2;
            break;
        case OMX_VIDEO_AVCLevel5:
            cfg.level = H264ENC_LEVEL_5;
            break;
        case OMX_VIDEO_AVCLevel51:
        case OMX_VIDEO_AVCLevelMax:
            cfg.level = H264ENC_LEVEL_5_1;
            break;
#else

        case OMX_VIDEO_AVCLevel41:
        case OMX_VIDEO_AVCLevelMax:
            cfg.level = H264ENC_LEVEL_4_1;
            break;
#endif

#else
        case OMX_VIDEO_AVCLevel1:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_1;
            break;
        case OMX_VIDEO_AVCLevel1b:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_1_b;
            break;
        case OMX_VIDEO_AVCLevel11:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_1_1;
            break;
        case OMX_VIDEO_AVCLevel12:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_1_2;
            break;
        case OMX_VIDEO_AVCLevel13:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_1_3;
            break;
        case OMX_VIDEO_AVCLevel2:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_2;
            break;
        case OMX_VIDEO_AVCLevel21:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_2_1;
            break;
        case OMX_VIDEO_AVCLevel22:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_2_2;
            break;
        case OMX_VIDEO_AVCLevel3:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_3;
            break;
        case OMX_VIDEO_AVCLevel31:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_3_1;
            break;
        case OMX_VIDEO_AVCLevel32:
        case OMX_VIDEO_AVCLevelMax:
            cfg.profileAndLevel = H264ENC_BASELINE_LEVEL_3_2;
            break;
#endif
        default:
			LOGE("[%s] unsupported encoding level %d", __FUNCTION__, params->h264_config.eLevel);
            return NULL;
            break;
    }


    cfg.streamType = H264ENC_BYTE_STREAM;
    cfg.width = params->common_config.nOutputWidth;
    cfg.height = params->common_config.nOutputHeight;

    //cfg.frameRateDenom = 1;
    //cfg.frameRateNum = cfg.frameRateDenom * Q16_FLOAT(params->common_config.nInputFramerate);

    // Initialise the frameratenum properly to 90000.
    // eg: for frNum = 90000, and nInputFramerate = 15, frDen shd be 6000.
    cfg.frameRateNum = TIME_RESOLUTION;
    cfg.frameRateDenom = cfg.frameRateNum / Q16_FLOAT(params->common_config.nInputFramerate);

#if !defined (ENC8290) && !defined (ENCH1)
    cfg.complexityLevel = H264ENC_COMPLEXITY_1; // Only supported option
#endif

    ENCODER_H264* this = OSAL_Malloc(sizeof(ENCODER_H264));

    this->instance = 0;
    memset( &this->encIn, 0, sizeof(H264EncIn));
    this->origWidth = params->pp_config.origWidth;
    this->origHeight = params->pp_config.origHeight;
    this->base.stream_start = encoder_stream_start_h264;
    this->base.stream_end = encoder_stream_end_h264;
    this->base.encode = encoder_encode_h264;
    this->base.destroy = encoder_destroy_h264;

    this->bStabilize = params->pp_config.frameStabilization;
    this->nPFrames = params->nPFrames;
    this->nIFrameCounter = 0;
    this->nEstTimeInc = cfg.frameRateDenom;

    H264EncRet ret = H264EncInit(&cfg, &this->instance);

    // Setup coding control
    if (ret == H264ENC_OK)
    {
        H264EncCodingCtrl coding_ctrl;
        ret = H264EncGetCodingCtrl(this->instance, &coding_ctrl);

        if (ret == H264ENC_OK)
        {
            if (params->nSliceHeight > 0)
            {
                // slice height in macroblock rows (each row is 16 pixels)
                coding_ctrl.sliceSize = params->nSliceHeight / 16;
            }

            if (params->bDisableDeblocking)
            {
                coding_ctrl.disableDeblockingFilter = 1;
            }
            else
            {
                coding_ctrl.disableDeblockingFilter = 0;
            }

            // SEI information
            if (params->bSeiMessages)
            {
                coding_ctrl.seiMessages = 1;
            }
            else
            {
                coding_ctrl.seiMessages = 0;
            }

            ret = H264EncSetCodingCtrl(this->instance, &coding_ctrl);
        }
    }
    else {
        LOGE("[%s] H264EncInit failed! (%d)", __func__, ret);
        OSAL_Free(this);
        return NULL;
    }

    // Setup rate control
    if (ret == H264ENC_OK)
    {
        H264EncRateCtrl rate_ctrl;
        ret = H264EncGetRateCtrl(this->instance, &rate_ctrl);

        if (ret == H264ENC_OK)
        {

            if(params->nPFrames)
                rate_ctrl.gopLen = params->nPFrames;

            // optional. Set to -1 to use default
            if (params->rate_config.nTargetBitrate > 0)
            {
                rate_ctrl.bitPerSecond = params->rate_config.nTargetBitrate;
            }

            // optional. Set to -1 to use default
            if (params->rate_config.nQpDefault > 0)
            {
                rate_ctrl.qpHdr = params->rate_config.nQpDefault;
            }

            // optional. Set to -1 to use default
            if (params->rate_config.nQpMin >= 0)
            {
                rate_ctrl.qpMin = params->rate_config.nQpMin;
                if(rate_ctrl.qpHdr != -1 && rate_ctrl.qpHdr < rate_ctrl.qpMin)
                {
                    rate_ctrl.qpHdr = rate_ctrl.qpMin;
                }
            }

            // optional. Set to -1 to use default
            if (params->rate_config.nQpMax > 0)
            {
                rate_ctrl.qpMax = params->rate_config.nQpMax;
                if(rate_ctrl.qpHdr > rate_ctrl.qpMax)
                {
                    rate_ctrl.qpHdr = rate_ctrl.qpMax;
                }
            }

            if (params->rate_config.nPictureRcEnabled >= 0)
            {
                rate_ctrl.pictureRc = params->rate_config.nPictureRcEnabled;
            }

            if (params->rate_config.nMbRcEnabled >= 0)
            {
                rate_ctrl.mbRc = params->rate_config.nMbRcEnabled;
            }

            if (params->rate_config.nHrdEnabled >= 0)
            {
                rate_ctrl.hrd = params->rate_config.nHrdEnabled;
            }

            switch (params->rate_config.eRateControl)
            {
                case OMX_Video_ControlRateDisable:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateVariable:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateConstant:
                    rate_ctrl.pictureSkip = 0;
                    break;
                case OMX_Video_ControlRateVariableSkipFrames:
                    rate_ctrl.pictureSkip = 1;
                    break;
                case OMX_Video_ControlRateConstantSkipFrames:
                    rate_ctrl.pictureSkip = 1;
                    break;
                case OMX_Video_ControlRateMax:
                    rate_ctrl.pictureSkip = 0;
                    break;
                default:
                    break;
            }
            ret = H264EncSetRateCtrl(this->instance, &rate_ctrl);
        }
        else {
            LOGE("[%s] H264EncGetRateCtrl failed! (%d)", __func__, ret);
            OSAL_Free(this);
            return NULL;
        }
    }

    // Setup preprocessing
    if (ret == H264ENC_OK)
    {
        H264EncPreProcessingCfg pp_config;

        ret = H264EncGetPreProcessing(this->instance, &pp_config);

        pp_config.origWidth = params->pp_config.origWidth;
        pp_config.origHeight = params->pp_config.origHeight;

        pp_config.xOffset = params->pp_config.xOffset;
        pp_config.yOffset = params->pp_config.yOffset;

        switch (params->pp_config.formatType)
        {
            case OMX_COLOR_FormatYUV420PackedPlanar:
            case OMX_COLOR_FormatYUV420Planar:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_YUV420_PLANAR;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_YUV420_PLANAR;
#endif
                break;
            case OMX_COLOR_FormatYUV420PackedSemiPlanar:
            case OMX_COLOR_FormatYUV420SemiPlanar:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_YUV420_SEMIPLANAR;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_YUV420_SEMIPLANAR;
#endif
                break;
            case OMX_COLOR_FormatYCbYCr:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_YUV422_INTERLEAVED_YUYV;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_YUV422_INTERLEAVED_YUYV;
#endif
                break;
            case OMX_COLOR_FormatCbYCrY:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_YUV422_INTERLEAVED_UYVY;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_YUV422_INTERLEAVED_UYVY;
#endif
                break;

            case OMX_COLOR_Format16bitRGB565:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_RGB565;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_RGB565;
#endif
                break;

            case OMX_COLOR_Format16bitBGR565:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_BGR565;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_BGR565;
#endif
                break;

            case OMX_COLOR_Format16bitARGB4444:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_RGB444;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_RGB444;
#endif
                break;

            case OMX_COLOR_Format16bitARGB1555:
#ifdef ENC6280
                pp_config.yuvType = H264ENC_RGB555;
#endif
#if defined (ENC7280) || defined (ENC8270) || defined (ENC8290) || defined (ENCH1)
                pp_config.inputType = H264ENC_RGB555;
#endif
                break;
#ifdef ENCH1
            case OMX_COLOR_Format12bitRGB444:
                pp_config.inputType = H264ENC_RGB444;
            case OMX_COLOR_Format24bitRGB888:
            case OMX_COLOR_Format25bitARGB1888:
            case OMX_COLOR_Format32bitARGB8888:
                pp_config.inputType = H264ENC_RGB888;
                break;
            case OMX_COLOR_Format24bitBGR888:
                pp_config.inputType = H264ENC_BGR888;
#endif
            default:
                ret = H264ENC_INVALID_ARGUMENT;
                break;
        }

        switch (params->pp_config.angle)
        {
            case 0:
                pp_config.rotation = H264ENC_ROTATE_0;
                break;
            case 90:
                pp_config.rotation = H264ENC_ROTATE_90R;
                break;
            case 270:
                pp_config.rotation = H264ENC_ROTATE_90L;
                break;
            default:
                ret = H264ENC_INVALID_ARGUMENT;
                break;
        }

        pp_config.videoStabilization = params->pp_config.frameStabilization;

        if (ret == H264ENC_OK)
        {
            ret = H264EncSetPreProcessing(this->instance, &pp_config);
        }
    }
    else {
        LOGE("[%s] H264EncSetRateCtrl failed! (%d)", __func__, ret);
        OSAL_Free(this);
        return NULL;
    }


    if (ret != H264ENC_OK)
    {
        LOGE("[%s] ret != H264ENC_OK (%d)", __FUNCTION__, ret);
        OSAL_Free(this);
        return NULL;
    }

    return (ENCODER_PROTOTYPE*) this;
}

CODEC_STATE HantroHwEncOmx_encoder_intra_period_h264(ENCODER_PROTOTYPE* arg, OMX_U32 nPFrames)
{

    ENCODER_H264* this = (ENCODER_H264*)arg;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    this->nPFrames = nPFrames;

    H264EncRateCtrl rate_ctrl;
    memset(&rate_ctrl,0,sizeof(H264EncRateCtrl));

    H264EncRet ret = H264EncGetRateCtrl(this->instance, &rate_ctrl);

    if (ret == H264ENC_OK)
    {
        rate_ctrl.gopLen = nPFrames;

        ret = H264EncSetRateCtrl(this->instance, &rate_ctrl);
    }

    switch (ret)
    {
        case H264ENC_OK:
        {
            stat = CODEC_OK;
        }
        case H264ENC_NULL_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INSTANCE_ERROR:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
        case H264ENC_INVALID_ARGUMENT:
        {
            stat = CODEC_ERROR_INVALID_ARGUMENT;
        }
        break;
        case H264ENC_INVALID_STATUS:
        {
            stat = CODEC_ERROR_INVALID_STATE;
        }
        default:
        {
            stat = CODEC_ERROR_UNSPECIFIED;
        }
        break;
    }

    return stat;
}

CODEC_STATE HantroHwEncOmx_encoder_frame_rate_h264(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate)
{
    ENCODER_H264* this = (ENCODER_H264*)arg;

    this->nEstTimeInc = (OMX_U32) (TIME_RESOLUTION / Q16_FLOAT(xFramerate));

    return CODEC_OK;
}

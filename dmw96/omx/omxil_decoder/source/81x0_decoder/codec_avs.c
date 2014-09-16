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
--  Description :  Implementation for AVS codec
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_avs"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_avs.h"
#include "post_processor.h"
#include <avsdecapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

/* number of frame buffers decoder should allocate 0=AUTO */
//#define FRAME_BUFFERS 3
#define FRAME_BUFFERS 0

#define SHOW4(p) (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p+=4;
//#define ALIGN(offset, align)   ((((unsigned int)offset) + align-1) & ~(align-1))
#define ALIGN(offset, align)    offset

typedef struct CODEC_AVS
{
    CODEC_PROTOTYPE base;
    AvsDecInst instance;
    PP_STATE pp_state;
    PPInst pp_instance;
    PPConfig pp_config;
    OMX_U32 framesize;
    PP_TRANSFORMS transforms;
    OMX_BOOL update_pp_out;
    OMX_U32 picId;

} CODEC_AVS;

// destroy codec instance
static void decoder_destroy_avs(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_AVS *this = (CODEC_AVS *) arg;

    if(this)
    {
        this->base.decode = 0;
        this->base.getframe = 0;
        this->base.getinfo = 0;
        this->base.destroy = 0;
        this->base.scanframe = 0;
        this->base.setppargs = 0;

        if(this->pp_instance)
        {
            if(this->pp_state == PP_PIPELINE)
            {
                HantroHwDecOmx_pp_pipeline_disable(this->pp_instance,
                                                   this->instance);
                this->pp_state = PP_DISABLED;
            }
            HantroHwDecOmx_pp_destroy(&this->pp_instance);
        }

        if(this->instance)
        {
            AvsDecRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free(this);
    }

}


// try to consume stream data
static CODEC_STATE decoder_decode_avs(CODEC_PROTOTYPE * arg,
                                      STREAM_BUFFER * buf, OMX_U32 * consumed,
                                      FRAME * frame)
{
    CALLSTACK;

    CODEC_AVS *this = (CODEC_AVS *) arg;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

    AvsDecInput input;
    AvsDecOutput output;

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    memset(&input, 0, sizeof(AvsDecInput));
    memset(&output, 0, sizeof(AvsDecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    input.picId = this->picId;
    TRACE_PRINT( "Pic id %d, stream lenght %d \n", input.picId, input.dataLen);

    frame->size = 0;
    *consumed = 0;

    if (this->pp_config.ppOutImg.bufferBusAddr != output.strmCurrBusAddress)
        {
            this->update_pp_out = OMX_TRUE;
        }

    if(this->pp_state == PP_PIPELINE && this->update_pp_out == OMX_TRUE)
    {
        HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        PPResult res = HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
        this->update_pp_out = OMX_FALSE;
        if(res != PP_OK)
        {
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
    }

    AvsDecRet ret = AvsDecDecode(this->instance, &input, &output);

    switch (ret)
    {
        case AVSDEC_STRM_PROCESSED:
            //Stream buffer processed, but new frame was not ready.
            stat = CODEC_NEED_MORE;
            break;
        case AVSDEC_PIC_DECODED:
            //All the data in the stream buffer was processed. Stream buffer must be updated before calling AvsDecDecode again.
            this->picId++;
            stat = CODEC_HAS_FRAME;
            break;
        case AVSDEC_HDRS_RDY:
            //Headers decoded. Stream header information is now readable with the function AvsDecGetInfo.
            if(this->pp_state == PP_PIPELINE)
            {
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
            }
            stat = CODEC_HAS_INFO;
            break;
        case AVSDEC_PARAM_ERROR:
            //Error in parameter structures. Stream decoding is not started. Possibly a NULL pointer or stream length is 0. Check structures.
            stat = CODEC_ERROR_INVALID_ARGUMENT;
            break;
        case AVSDEC_STRM_ERROR:
            //Error in stream decoding. Can not recover before new headers decoded.
            stat = CODEC_ERROR_STREAM;
            break;
        case AVSDEC_NOT_INITIALIZED:
            //Decoder instance is not initialized. Stream decoding is not started. AvsDecInit must be called before decoding can be started.
            stat = CODEC_ERROR_NOT_INITIALIZED;
            break;
        case AVSDEC_HW_BUS_ERROR:
            //A bus error occurred in the hardware operation of the decoder. The validity of each bus address should be checked. The decoding can not continue. The decoder instance has to be released.
            stat = CODEC_ERROR_HW_BUS_ERROR;
            break;
        case AVSDEC_HW_TIMEOUT:
            //Error, the wait for a hardware finish has timed out. The current frame is lost. New frame decoding has to be started.
            stat = CODEC_ERROR_HW_TIMEOUT;
            break;
        case AVSDEC_SYSTEM_ERROR:
            //Error, a fatal system error was caught. The decoding can not continue. The decoder instance has to be released.
            stat = CODEC_ERROR_SYS;
            break;
        case AVSDEC_HW_RESERVED:
            //Failed to reserve HW for current decoder instance. The current frame is lost. New frame decoding has to be started.
            stat = CODEC_ERROR_HW_RESERVED;
            break;
        case AVSDEC_MEMFAIL:
            //The decoder was not able to allocate memory.
            stat = CODEC_ERROR_MEMFAIL;
            break;
        default:
            assert(!"unhandled AvsDecRet");
            break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {
#ifdef HANTRO_TESTBENCH
        *consumed = input.dataLen - output.dataLeft;
        TRACE_PRINT( "decoder data left %d\n", output.dataLeft);
#else
        if(stat == CODEC_HAS_INFO)
            *consumed = 0;
        else
            *consumed = input.dataLen;
#endif
    }

    return stat;
}

// get stream info
static CODEC_STATE decoder_getinfo_avs(CODEC_PROTOTYPE * arg, STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_AVS *this = (CODEC_AVS *) arg;
    AvsDecInfo info;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    memset(&info, 0, sizeof(AvsDecInfo));

    AvsDecRet ret = AvsDecGetInfo(this->instance, &info);
    if (ret == AVSDEC_HDRS_NOT_RDY)
    {
        return CODEC_ERROR_STREAM;
    }
    else if (ret == AVSDEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if (ret == AVSDEC_OK)
    {
        pkg->width = info.frameWidth;
        pkg->height = info.frameHeight;
        pkg->stride = info.codedWidth;
        pkg->sliceheight = info.codedHeight;
        pkg->interlaced = info.interlacedSequence;
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar; //info.outputFormat;

        /* UNUSED parameters */
        /*
        info.profileAndLevelIndication;
        info.displayAspectRatio;
        info.streamFormat;
        info.videoFormat;
        info.videoRange;
        */

        if(this->pp_state != PP_DISABLED)
        {
            if(this->transforms == PPTR_DEINTERLACE)
            {
                this->update_pp_out = OMX_TRUE;
            }
            HantroHwDecOmx_pp_set_info(&this->pp_config, pkg, this->transforms);
            PPResult res =
                HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
            if(res != PP_OK)
            {
                return CODEC_ERROR_INVALID_ARGUMENT;
            }
        }
        this->framesize = pkg->framesize;

        return CODEC_OK;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get decoded frame
static CODEC_STATE decoder_getframe_avs(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    AvsDecPicture picture;

    CODEC_AVS *this = (CODEC_AVS *)arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(frame);

    if(eos && this->pp_state == PP_PIPELINE)
    {
        TRACE_PRINT("change pp output, EOS\n");
        HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        PPResult res =
            HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
        this->update_pp_out = OMX_FALSE;
        if(res != PP_OK)
        {
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
    }

    /* stream ended but we know there is picture to be outputted. We
     * have to loop one more time without "eos" and on next round
     * NextPicture is called with eos "force last output"
     */
    CALLSTACK;

    memset(&picture, 0, sizeof(AvsDecPicture));

    u32 endofStream = eos == OMX_TRUE ? 1 : 0;

    AvsDecRet ret = AvsDecNextPicture(this->instance, &picture, endofStream);

    TRACE_PRINT(" end of stream %d\n", endofStream);
    TRACE_PRINT("err mbs %d \n", picture.numberOfErrMBs);

    if(ret == AVSDEC_PIC_RDY)
    {
        assert(this->framesize);

        if(this->pp_state != PP_DISABLED)
        {
            this->update_pp_out = OMX_TRUE;
            PPResult res = HantroHwDecOmx_pp_execute(this->pp_instance);
            if(res != PP_OK)
            {
                return CODEC_ERROR_UNSPECIFIED;
            }
        }
        else
        {
            // gots to copy the frame from the buffer reserved by the codec
            // into to the specified output buffer
            // there's no indication of the framesize in the output picture structure
            // so we have to use the framesize from before
            memcpy(frame->fb_bus_data, picture.pOutputPicture, this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.numberOfErrMBs;

        return CODEC_HAS_FRAME;
    }
    else if(ret == AVSDEC_OK)
    {
        return CODEC_OK;
    }
    else if(ret == AVSDEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if(ret == AVSDEC_NOT_INITIALIZED)
    {
        return CODEC_ERROR_SYS;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_avs(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;
    CODEC_AVS *this = (CODEC_AVS *) arg;

    assert(this);

    *first = 0;
    *last = buf->streamlen;
    return 1;
}

static CODEC_STATE decoder_setppargs_avs(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;
    CODEC_AVS *this = (CODEC_AVS *) codec;

    assert(this);
    assert(args);

    PPResult ret;

    ret = HantroHwDecOmx_pp_init(&this->pp_instance, &this->pp_config);

    if(ret == PP_OK)
    {
        this->transforms =
            HantroHwDecOmx_pp_set_output(&this->pp_config, args, OMX_TRUE);
        if(this->transforms == PPTR_ARG_ERROR)
            return CODEC_ERROR_INVALID_ARGUMENT;
#ifdef ENABLE_PP
        if(this->transforms == PPTR_NONE)
        {
            TRACE_PRINT("PP_DISABLED\n");
            this->pp_state = PP_DISABLED;
        }
        else
        {
            TRACE_PRINT("PP_COMBINED_MODE\n");
            PPResult res;

            res =
                HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                  this->instance,
                                                  PP_PIPELINED_DEC_TYPE_AVS);
            if(res != PP_OK)
            {
                return CODEC_ERROR_UNSPECIFIED;
            }
            this->pp_state = PP_PIPELINE;

        }

        return CODEC_OK;
#else
    this->pp_state = PP_DISABLED;
    return CODEC_OK;
#endif

    }
    return CODEC_ERROR_UNSPECIFIED;
}

#ifdef DYNAMIC_SCALING
static CODEC_STATE decoder_setscaling_avs(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_AVS *this = (CODEC_AVS *) codec;

    assert(this);

    this->pp_config.ppOutImg.width = width;
    this->pp_config.ppOutImg.height = height;

    PPResult res = HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
    if(res != PP_OK)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }

    return CODEC_OK;
}
#endif

// create codec instance and initialize it
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_avs()
{
    CALLSTACK;

    CODEC_AVS *this = OSAL_Malloc(sizeof(CODEC_AVS));

    memset(this, 0, sizeof(CODEC_AVS));

    this->base.destroy = decoder_destroy_avs;
    this->base.decode = decoder_decode_avs;
    this->base.getinfo = decoder_getinfo_avs;
    this->base.getframe = decoder_getframe_avs;
    this->base.scanframe = decoder_scanframe_avs;
    this->base.setppargs = decoder_setppargs_avs;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_avs;
#endif
    this->instance = 0;
    this->picId = 0;

#ifdef IS_G1_DECODER
    AvsDecRet ret = AvsDecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT,
                        FRAME_BUFFERS, DEC_REF_FRM_RASTER_SCAN);
#else
    AvsDecRet ret = AvsDecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT,
                        FRAME_BUFFERS);
#endif

    if(ret != AVSDEC_OK)
    {
        decoder_destroy_avs((CODEC_PROTOTYPE *) this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

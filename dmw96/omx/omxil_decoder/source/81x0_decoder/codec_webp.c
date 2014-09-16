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
--  Description :  Implementation for WEBP codec
--
------------------------------------------------------------------------------*/
//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_webp"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_webp.h"
#include "post_processor.h"
#include <vp8decapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

#define SHOW4(p) (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p+=4;
//#define ALIGN(offset, align)   ((((unsigned int)offset) + align-1) & ~(align-1))
#define ALIGN(offset, align)    offset

typedef struct CODEC_WEBP
{
    CODEC_PROTOTYPE base;
    VP8DecInst instance;
    PP_STATE pp_state;
    PPInst pp_instance;
    PPConfig pp_config;
    OMX_U32 framesize;
    PP_TRANSFORMS transforms;
    OMX_BOOL update_pp_out;
    OMX_U32 picId;
    OMX_U32 first_buffer_size;
    OMX_BOOL headersDecoded;
    OMX_U32 sliceHeight;
} CODEC_WEBP;

// destroy codec instance
static void decoder_destroy_webp(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_WEBP *this = (CODEC_WEBP *) arg;

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
            VP8DecRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free(this);
    }
}


// try to consume stream data
static CODEC_STATE decoder_decode_webp(CODEC_PROTOTYPE * arg,
                                      STREAM_BUFFER * buf, OMX_U32 * consumed,
                                      FRAME * frame)
{
    CALLSTACK;

    CODEC_WEBP *this = (CODEC_WEBP *) arg;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

    VP8DecInput input;
    VP8DecOutput output;

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    memset(&input, 0, sizeof(VP8DecInput));
    memset(&output, 0, sizeof(VP8DecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    input.sliceHeight = 0;

    frame->size = 0;
    *consumed = 0;

    //Do not update PP before headers are decoded
    if (this->pp_state == PP_PIPELINE && this->headersDecoded == OMX_TRUE)
        this->update_pp_out = OMX_TRUE;

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

    VP8DecRet ret = VP8DecDecode(this->instance, &input, &output);

    switch (ret)
    {
        case VP8DEC_PIC_DECODED:
            this->picId++;
            stat = CODEC_HAS_FRAME;
            break;
        case VP8DEC_HDRS_RDY:
            if(this->pp_state == PP_PIPELINE)
            {
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
                this->headersDecoded = OMX_TRUE;
            }
            this->first_buffer_size = buf->streamlen;
            stat = CODEC_HAS_INFO;
            break;
        case VP8DEC_PARAM_ERROR:
            stat = CODEC_ERROR_INVALID_ARGUMENT;
            break;
        case VP8DEC_STRM_ERROR:
            stat = CODEC_ERROR_STREAM;
            break;
        case VP8DEC_NOT_INITIALIZED:
            stat = CODEC_ERROR_NOT_INITIALIZED;
            break;
        case VP8DEC_HW_BUS_ERROR:
            stat = CODEC_ERROR_HW_BUS_ERROR;
            break;
        case VP8DEC_HW_TIMEOUT:
            stat = CODEC_ERROR_HW_TIMEOUT;
            break;
        case VP8DEC_SYSTEM_ERROR:
            stat = CODEC_ERROR_SYS;
            break;
        case VP8DEC_HW_RESERVED:
            stat = CODEC_ERROR_HW_RESERVED;
            break;
        case VP8DEC_MEMFAIL:
            stat = CODEC_ERROR_MEMFAIL;
            break;
        case VP8DEC_STREAM_NOT_SUPPORTED:
            stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
            break;
        default:
            assert(!"unhandled VP8DecRet");
            break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {
        if(stat == CODEC_HAS_INFO)
            *consumed = 0;
        else
            *consumed = input.dataLen;

        TRACE_PRINT( "Consumed %d\n", (int) *consumed);
    }

    return stat;
}

// get stream info
static CODEC_STATE decoder_getinfo_webp(CODEC_PROTOTYPE * arg, STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_WEBP *this = (CODEC_WEBP *) arg;
    VP8DecInfo info;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    memset(&info, 0, sizeof(VP8DecInfo));

    VP8DecRet ret = VP8DecGetInfo(this->instance, &info);
    
    //printf("WEBP get info ret %d\n", ret);
    if (ret == VP8DEC_NOT_INITIALIZED)
    {
        return CODEC_ERROR_SYS;
    }
    else if (ret == VP8DEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if (ret == VP8DEC_OK)
    {
        pkg->width = info.frameWidth;
        pkg->height = info.frameHeight;
        pkg->stride = info.frameWidth;
        pkg->sliceheight = info.frameHeight;
        pkg->interlaced = 0;
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
        pkg->imageSize = pkg->width * pkg->height * 3 / 2;
        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar; //info.outputFormat;

        if(this->pp_state != PP_DISABLED)
        {
            if(this->transforms == PPTR_DEINTERLACE)
            {
                this->update_pp_out = OMX_TRUE;
            }

            HantroHwDecOmx_pp_set_info(&this->pp_config, pkg, this->transforms);
            
            PPResult res =
                HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
                
            //printf("pp set res %d\n", res);
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
static CODEC_STATE decoder_getframe_webp(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    VP8DecPicture picture;

    CODEC_WEBP *this = (CODEC_WEBP *)arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(frame);

    eos = OMX_TRUE; // Force EOS since only one frame is decoded

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

    memset(&picture, 0, sizeof(VP8DecPicture));

    u32 endofStream = eos == OMX_TRUE ? 1 : 0;

    VP8DecRet ret = VP8DecNextPicture(this->instance, &picture, endofStream);

    TRACE_PRINT(" end of stream %d\n", endofStream);
    TRACE_PRINT("err mbs %d \n", picture.nbrOfErrMBs);

    if(ret == VP8DEC_PIC_RDY)
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
            memcpy(frame->fb_bus_data, picture.pOutputFrame, this->framesize);
            //printf("this->framesize %d\n", this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.nbrOfErrMBs;

        return CODEC_HAS_FRAME;
    }
    else if(ret == VP8DEC_OK)
    {
        return CODEC_OK;
    }
    else if(ret == VP8DEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if(ret == VP8DEC_NOT_INITIALIZED)
    {
        return CODEC_ERROR_SYS;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_webp(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;
    CODEC_WEBP *this = (CODEC_WEBP *) arg;

    assert(this);

    *first = 0;
    *last = buf->streamlen;
    return 1;
}

static CODEC_STATE decoder_setppargs_webp(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;
    CODEC_WEBP *this = (CODEC_WEBP *) codec;

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
                                                  PP_PIPELINED_DEC_TYPE_WEBP);
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

// create codec instance and initialize it
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_webp()
{
    CALLSTACK;

    CODEC_WEBP *this = OSAL_Malloc(sizeof(CODEC_WEBP));

    memset(this, 0, sizeof(CODEC_WEBP));

    this->base.destroy = decoder_destroy_webp;
    this->base.decode = decoder_decode_webp;
    this->base.getinfo = decoder_getinfo_webp;
    this->base.getframe = decoder_getframe_webp;
    this->base.scanframe = decoder_scanframe_webp;
    this->base.setppargs = decoder_setppargs_webp;
    this->instance = 0;
    this->picId = 0;
    this->headersDecoded = OMX_FALSE;

    VP8DecRet ret = VP8DecInit(&this->instance, VP8DEC_WEBP, 0, 0, DEC_REF_FRM_RASTER_SCAN);
    if(ret != VP8DEC_OK)
    {
        decoder_destroy_webp((CODEC_PROTOTYPE *) this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

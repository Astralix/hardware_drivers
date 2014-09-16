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
--  Description :  Implementation for VP8 codec
--
------------------------------------------------------------------------------*/
//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_vp8"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_vp8.h"
#include "post_processor.h"
#include <vp8decapi.h>
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

typedef struct CODEC_VP8
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
} CODEC_VP8;

// destroy codec instance
static void decoder_destroy_vp8(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_VP8 *this = (CODEC_VP8 *) arg;

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
static CODEC_STATE decoder_decode_vp8(CODEC_PROTOTYPE * arg,
                                      STREAM_BUFFER * buf, OMX_U32 * consumed,
                                      FRAME * frame)
{
    CALLSTACK;

    CODEC_VP8 *this = (CODEC_VP8 *) arg;

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
    input.dataLen = buf->streamlen + 256;
    //input.picId = this->picId;
    TRACE_PRINT( "Pic id %d, stream length %d, buffer length = %d\n", (int) this->picId, (int) input.dataLen, (int) buf->streamlen);

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
            LOGE("error in line %d: HantroHwDecOmx_pp_set returned %d", __LINE__, res);
            LOGE("pp config: input %dx%d, output %dx%d", this->pp_config.ppInImg.width, this->pp_config.ppInImg.height,
                                                         this->pp_config.ppOutImg.width, this->pp_config.ppOutImg.height);
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
            LOGE("error in line %d: VP8DecDecode returned %d", __LINE__, ret);
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
        else if((this->picId == 1) && (this->first_buffer_size < buf->streamlen))
            *consumed = this->first_buffer_size; // otherwise we will lose the 2nd frame
        else
            *consumed = buf->streamlen;
    }

    return stat;
}

// get stream info
static CODEC_STATE decoder_getinfo_vp8(CODEC_PROTOTYPE * arg, STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_VP8 *this = (CODEC_VP8 *) arg;
    VP8DecInfo info;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    memset(&info, 0, sizeof(VP8DecInfo));

    VP8DecRet ret = VP8DecGetInfo(this->instance, &info);
    if (ret == VP8DEC_NOT_INITIALIZED)
    {
        return CODEC_ERROR_SYS;
    }
    else if (ret == VP8DEC_PARAM_ERROR)
    {
        LOGE("error in line %d: VP8DecGetInfo returned %d", __LINE__, ret);
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
            if(res != PP_OK)
            {
                LOGE("error in line %d: HantroHwDecOmx_pp_set returned %d", __LINE__, res);
                return CODEC_ERROR_INVALID_ARGUMENT;
            }
        }
        this->framesize = pkg->framesize;

        return CODEC_OK;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get decoded frame
static CODEC_STATE decoder_getframe_vp8(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    VP8DecPicture picture;

    CODEC_VP8 *this = (CODEC_VP8 *)arg;

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
            LOGE("error in line %d: HantroHwDecOmx_pp_set returned %d", __LINE__, res);
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
    
    if(ret == VP8DEC_PIC_RDY || ret == VP8DEC_PIC_DECODED)
    {
        frame->isIntra = picture.isIntraFrame;
        frame->isGoldenOrAlternate = picture.isGoldenFrame;
    }

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
        LOGE("error in line %d: VP8DecNextPicture returned %d", __LINE__, ret);
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if(ret == VP8DEC_NOT_INITIALIZED)
    {
        return CODEC_ERROR_SYS;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_vp8(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;
    CODEC_VP8 *this = (CODEC_VP8 *) arg;

    assert(this);

    *first = 0;
    *last = buf->streamlen;
    return 1;
}

static CODEC_STATE decoder_setppargs_vp8(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;
    CODEC_VP8 *this = (CODEC_VP8 *) codec;

    assert(this);
    assert(args);

    PPResult ret;

    ret = HantroHwDecOmx_pp_init(&this->pp_instance, &this->pp_config);

    if(ret == PP_OK)
    {
        this->transforms =
            HantroHwDecOmx_pp_set_output(&this->pp_config, args, OMX_TRUE);
        if(this->transforms == PPTR_ARG_ERROR)
        {
            LOGE("error in line %d: HantroHwDecOmx_pp_set_output returned PPTR_ARG_ERROR", __LINE__);	
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
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
                                                  PP_PIPELINED_DEC_TYPE_VP8);
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
static CODEC_STATE decoder_setscaling_vp8(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_VP8 *this = (CODEC_VP8 *) codec;

    assert(this);

    this->pp_config.ppOutImg.width = width;
    this->pp_config.ppOutImg.height = height;

    PPResult res = HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
    if(res != PP_OK)
    {
        LOGE("error in line %d: HantroHwDecOmx_pp_set returned %d", __LINE__, res);
        return CODEC_ERROR_INVALID_ARGUMENT;
    }

    return CODEC_OK;
}
#endif

// create codec instance and initialize it
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_vp8()
{
    CALLSTACK;

    CODEC_VP8 *this = OSAL_Malloc(sizeof(CODEC_VP8));

    memset(this, 0, sizeof(CODEC_VP8));

    this->base.destroy = decoder_destroy_vp8;
    this->base.decode = decoder_decode_vp8;
    this->base.getinfo = decoder_getinfo_vp8;
    this->base.getframe = decoder_getframe_vp8;
    this->base.scanframe = decoder_scanframe_vp8;
    this->base.setppargs = decoder_setppargs_vp8;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_vp8;
#endif
    this->instance = 0;
    this->picId = 0;
    this->headersDecoded = OMX_FALSE;

#ifdef IS_G1_DECODER
    VP8DecRet ret = VP8DecInit(&this->instance, VP8DEC_VP8,
                        USE_VIDEO_FREEZE_CONCEALMENT,
                            FRAME_BUFFERS, DEC_REF_FRM_RASTER_SCAN);
#else
    VP8DecRet ret = VP8DecInit(&this->instance, VP8DEC_VP8,
                        USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS);
#endif

    if(ret != VP8DEC_OK)
    {
        decoder_destroy_vp8((CODEC_PROTOTYPE *) this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

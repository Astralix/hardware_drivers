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
--  Description :  Implementation for VP-6 decoder
--
------------------------------------------------------------------------------*/
//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_vp6"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_vp6.h"
#include "post_processor.h"
#include <vp6decapi.h>
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

OMX_U32 picId;

typedef struct CODEC_VP6
{
    CODEC_PROTOTYPE base;

    VP6DecInst instance;
    OMX_U32 framesize;
    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;
    OMX_BOOL pipeline_enabled;
    OMX_BOOL update_pp_out;
    //OMX_BOOL extraEosLoopDone;
} CODEC_VP6;

// destroy codec instance
static void decoder_destroy_vp6(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_VP6 *this = (CODEC_VP6 *) arg;

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
            if(this->pipeline_enabled != OMX_FALSE)
            {
                HantroHwDecOmx_pp_pipeline_disable(this->pp_instance,
                                                   this->instance);
                this->pp_state = PP_DISABLED;
            }
            HantroHwDecOmx_pp_destroy(&this->pp_instance);
        }

        if(this->instance)
        {
            VP6DecRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_vp6(CODEC_PROTOTYPE * arg,
                                       STREAM_BUFFER * buf,
                                       OMX_U32 * consumed,
                                       FRAME * frame)
{
    VP6DecInput input;

    VP6DecOutput output;

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    CODEC_VP6 *this = (CODEC_VP6 *) arg;

    CALLSTACK;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

    memset(&input, 0, sizeof(VP6DecInput));
    memset(&output, 0, sizeof(VP6DecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
/*    input.picId = this->picId;*/
    TRACE_PRINT(stdout, "Pic id %d, stream lenght %d \n", (int) picId,
               (int) input.dataLen);
    *consumed = 0;
    frame->size = 0;

    if(this->pp_state == PP_PIPELINE && this->update_pp_out == OMX_TRUE)
    {
        HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        PPResult res =
            HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
        this->update_pp_out = OMX_FALSE;
        if(res!= PP_OK)
        {
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
    }

    VP6DecRet ret = VP6DecDecode(this->instance, &input, &output);

    switch (ret)
    {
    case VP6DEC_PIC_DECODED:
        picId++;
        stat = CODEC_HAS_FRAME;
        break;
    case VP6DEC_HDRS_RDY:
        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        }
        stat = CODEC_HAS_INFO;
        break;
    case VP6DEC_ADVANCED_TOOLS:
        stat = CODEC_NEED_MORE;
        break;
    case VP6DEC_STRM_PROCESSED:
        stat = CODEC_NEED_MORE;
        break;
    case VP6DEC_PARAM_ERROR:
        stat = CODEC_ERROR_INVALID_ARGUMENT;
        break;
    case VP6DEC_STRM_ERROR:
        stat = CODEC_ERROR_STREAM;
        break;
    case VP6DEC_NOT_INITIALIZED:
        stat = CODEC_ERROR_NOT_INITIALIZED;
        break;
    case VP6DEC_HW_BUS_ERROR:
        stat = CODEC_ERROR_HW_BUS_ERROR;
        break;
    case VP6DEC_HW_TIMEOUT:
        stat = CODEC_ERROR_HW_TIMEOUT;
        break;
    case VP6DEC_SYSTEM_ERROR:
        stat = CODEC_ERROR_SYS;
        break;
    case VP6DEC_HW_RESERVED:
        stat = CODEC_ERROR_HW_RESERVED;
        break;
    case VP6DEC_MEMFAIL:
        stat = CODEC_ERROR_MEMFAIL;
        break;
    case VP6DEC_STREAM_NOT_SUPPORTED:
        stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
        break;
    default:
        assert(!"unhandled VP6DecRet");
        break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {

        if(stat == CODEC_HAS_INFO)
            *consumed = 0;
        else
            *consumed = input.dataLen;
    }

    return stat;
}


    // get stream info
static CODEC_STATE decoder_getinfo_vp6(CODEC_PROTOTYPE * arg,
                                        STREAM_INFO * pkg)
{
    VP6DecInfo decinfo;

    CODEC_VP6 *this = (CODEC_VP6 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    CALLSTACK;

    memset(&decinfo, 0, sizeof(VP6DecInfo));

    // the headers are ready, get stream information
    VP6DecRet ret = VP6DecGetInfo(this->instance, &decinfo);

    this = (CODEC_VP6 *) arg;
    if(ret == VP6DEC_OK)
    {
        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;

        //pkg->width = decinfo.scaledWidth;
        //pkg->height = decinfo.scaledHeight;
        pkg->width = decinfo.frameWidth;
        pkg->height = decinfo.frameHeight;
        pkg->sliceheight = decinfo.frameHeight;
        pkg->stride = decinfo.frameWidth;
        pkg->interlaced = 0;
        pkg->framesize = (pkg->sliceheight * pkg->stride) * 3 / 2;

        if(this->pp_state != PP_DISABLED)
        {
            this->update_pp_out = OMX_TRUE;
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
    else if(ret == VP6DEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if(ret == VP6DEC_HDRS_NOT_RDY)
    {
        return CODEC_ERROR_STREAM;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get decoded frame
static CODEC_STATE decoder_getframe_vp6(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    CALLSTACK;

    CODEC_VP6 *this = (CODEC_VP6 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(frame);

    if(eos && this->pp_state == PP_PIPELINE)
    {
        TRACE("change pp output, EOS\n");
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

    /*if(eos && this->extraEosLoopDone == OMX_FALSE)
    {
        this->extraEosLoopDone = OMX_TRUE;
        eos = OMX_FALSE;
    }*/

    VP6DecPicture picture;

    memset(&picture, 0, sizeof(VP6DecPicture));

    VP6DecRet ret = VP6DecNextPicture(this->instance, &picture, eos);

    TRACE("VP6DecNextPicture returned: %d\n", ret);

    switch (ret)
    {
    case VP6DEC_OK:
        return CODEC_OK;
    case VP6DEC_PIC_RDY:

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
            memcpy(frame->fb_bus_data, picture.pOutputFrame, this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.nbrOfErrMBs;

        return CODEC_HAS_FRAME;
    case VP6DEC_PARAM_ERROR:
        return CODEC_ERROR_INVALID_ARGUMENT;
    case VP6DEC_NOT_INITIALIZED:
        return CODEC_ERROR_SYS;
    default:
        assert(!"unhandled VP6DecRet");
        break;
    }

    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_vp6(CODEC_PROTOTYPE * arg,
                                 STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;
    *first = 0;
    *last = buf->streamlen;
    return 1;
}

static CODEC_STATE decoder_setppargs_vp6(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;

    CODEC_VP6 *this = (CODEC_VP6 *) codec;

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
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
#ifdef ENABLE_PP
        if(this->transforms == PPTR_NONE)
        {
            TRACE("PP_DISABLED\n");
            this->pp_state = PP_DISABLED;
        }
        else
        {
            if( /*this->transforms == PPTR_SCALE ||
                 * this->transforms == PPTR_FORMAT ||
                 * this->transforms == PPTR_DEINTERLACE ||
                 * this->transforms == (PPTR_SCALE | PPTR_FORMAT) ||
                 * this->transforms == (PPTR_SCALE | PPTR_DEINTERLACE) ||
                 * this->transforms == (PPTR_FORMAT | PPTR_DEINTERLACE) ||
                 * this->transforms == (PPTR_SCALE | PPTR_FORMAT | PPTR_DEINTERLACE) */
                  1)
            {
                PPResult res;

                res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_VP6);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }


                this->pp_state = PP_PIPELINE;
            }
            else
            {
                TRACE("PP_STANDALONE\n");
                this->pp_state = PP_STANDALONE;
            }
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
static CODEC_STATE decoder_setscaling_vp6(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_VP6 *this = (CODEC_VP6 *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_vp6()
{
    CALLSTACK;

    CODEC_VP6 *this = OSAL_Malloc(sizeof(CODEC_VP6));

    memset(this, 0, sizeof(CODEC_VP6));

    picId = 0;
    this->base.destroy = decoder_destroy_vp6;
    this->base.decode = decoder_decode_vp6;
    this->base.getinfo = decoder_getinfo_vp6;
    this->base.getframe = decoder_getframe_vp6;
    this->base.scanframe = decoder_scanframe_vp6;
    this->base.setppargs = decoder_setppargs_vp6;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_vp6;
#endif
    this->instance = 0;
    this->pipeline_enabled = OMX_FALSE;

#ifdef IS_G1_DECODER
    VP6DecRet ret = VP6DecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT,
                        FRAME_BUFFERS, DEC_REF_FRM_RASTER_SCAN);
#else
    VP6DecRet ret = VP6DecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT);
#endif

    if(ret != VP6DEC_OK)
    {
        decoder_destroy_vp6((CODEC_PROTOTYPE *) this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

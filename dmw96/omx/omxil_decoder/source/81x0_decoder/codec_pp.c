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
--  Description :  Implementation for post-processor codec.
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_pp"
#include <utils/Log.h>

#include "codec_pp.h"
#include "post_processor.h"
#include <assert.h>
#include <string.h>
#include <OSAL.h>

typedef struct CODEC_PP
{
    CODEC_PROTOTYPE base;
    CODEC_STATE decode_state;
    OMX_U32 input_frame_size;
    OMX_U32 input_width;
    OMX_U32 input_height;
    OMX_COLOR_FORMATTYPE input_format;
    OMX_COLOR_FORMATTYPE output_format;
    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;

} CODEC_PP;

static void codec_pp_destroy(CODEC_PROTOTYPE * codec)
{
    CODEC_PP *pp = (CODEC_PP *) codec;

    assert(pp);

    if(pp)
    {
        pp->base.decode = 0;
        pp->base.getframe = 0;
        pp->base.getinfo = 0;
        pp->base.destroy = 0;
        pp->base.scanframe = 0;
        pp->base.setppargs = 0;

        if(pp->pp_instance)
            HantroHwDecOmx_pp_destroy(&pp->pp_instance);
        OSAL_Free(pp);
    }
}

static
    CODEC_STATE codec_pp_decode(CODEC_PROTOTYPE * codec, STREAM_BUFFER * buf,
                                OMX_U32 * consumed, FRAME * frame)
{
    CODEC_PP *this = (CODEC_PP *) codec;

    assert(this);
    assert(this->pp_instance);
    assert(this->input_frame_size);

    if(this->decode_state == CODEC_HAS_INFO)
    {
        // if decode is being called for the first time we need to
        // pass the stream information upwards. Typically this
        // information would come from the stream after the stream headers
        // are parsed. But since we have just raw data here it must be faked.
        this->decode_state = CODEC_OK;
        return CODEC_HAS_INFO;
    }

    frame->size = HantroHwDecOmx_pp_get_framesize(&this->pp_config);
    frame->MB_err_count = 0;

    HantroHwDecOmx_pp_set_input_buffer(&this->pp_config, buf->bus_address);
    HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
    PPResult res = HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);

    if(res != PP_OK)
        return CODEC_ERROR_UNSPECIFIED;

    res = HantroHwDecOmx_pp_execute(this->pp_instance);
    if(res != PP_OK)
        return CODEC_ERROR_UNSPECIFIED;

    *consumed = this->input_frame_size;
    return CODEC_HAS_FRAME;
}

static CODEC_STATE codec_pp_getinfo(CODEC_PROTOTYPE * codec, STREAM_INFO * info)
{
    CODEC_PP *this = (CODEC_PP *) codec;

    assert(this);
    assert(info);
    info->format = this->input_format;
    info->width = this->input_width;
    info->height = this->input_height;
    info->stride = this->input_width;
    info->sliceheight = this->input_height;

    // apply info from the config
    HantroHwDecOmx_pp_set_info(&this->pp_config, info, this->transforms);

    info->format = this->output_format;
    return CODEC_OK;
}

static
    CODEC_STATE codec_pp_getframe(CODEC_PROTOTYPE * codec, FRAME * frame,
                                  OMX_BOOL eos)
{
    // nothing to do here, the output frames are ready in the call to decode
    return CODEC_OK;
}

static
    int codec_pp_scanframe(CODEC_PROTOTYPE * codec, STREAM_BUFFER * buf,
                           OMX_U32 * first, OMX_U32 * last)
{
    CODEC_PP *this = (CODEC_PP *) codec;

    assert(this);

    if(buf->streamlen < this->input_frame_size)
        return -1;

    assert(first);
    assert(last);
    *first = 0;
    *last = this->input_frame_size;
    return 1;
}

static CODEC_STATE codec_pp_setppargs(CODEC_PROTOTYPE * codec, PP_ARGS * args)
{
    assert(codec);
    assert(args);
    CODEC_PP *this = (CODEC_PP *) codec;

    if(args->scale.width == 0 && args->scale.height == 0)
    {
        // always should have size specified because
        // there is no video stream to decode and for getting the resolution from 
        args->scale.width = this->input_width;
        args->scale.height = this->input_height;
    }

    PPResult ret = HantroHwDecOmx_pp_init(&this->pp_instance, &this->pp_config);

    if(ret == PP_OK)
    {
        this->transforms =
            HantroHwDecOmx_pp_set_output(&this->pp_config, args, OMX_FALSE);
        if(this->transforms == PPTR_ARG_ERROR)
            return CODEC_ERROR_INVALID_ARGUMENT;

        this->output_format = args->format;
        return CODEC_OK;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_pp(OMX_U32 input_width,
                                                  OMX_U32 input_height,
                                                  OMX_COLOR_FORMATTYPE
                                                  input_format)
{
    CODEC_PP *pp = OSAL_Malloc(sizeof(CODEC_PP));

    if(pp)
    {
        memset(pp, 0, sizeof(CODEC_PP));
        pp->base.destroy = codec_pp_destroy;
        pp->base.decode = codec_pp_decode;
        pp->base.getinfo = codec_pp_getinfo;
        pp->base.getframe = codec_pp_getframe;
        pp->base.scanframe = codec_pp_scanframe;
        pp->base.setppargs = codec_pp_setppargs;
        pp->input_width = input_width;
        pp->input_height = input_height;
        pp->input_format = input_format;
        pp->input_frame_size = input_height * input_width;
        switch (input_format)
        {
        case OMX_COLOR_FormatYUV420PackedPlanar:
        case OMX_COLOR_FormatYUV420Planar:
		case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
            pp->input_frame_size = pp->input_frame_size * 3 / 2;
            break;
        case OMX_COLOR_FormatYCbYCr:   // 422 interleaved
            pp->input_frame_size *= 2;
            break;
        case OMX_COLOR_FormatYCrYCb:
            pp->input_frame_size *= 2;
            break;
        case OMX_COLOR_FormatCbYCrY:
            pp->input_frame_size *= 2;
            break;
        case OMX_COLOR_FormatCrYCbY:
            pp->input_frame_size *= 2;
            break;

        default:
            assert(0);
            OSAL_Free(pp);
            return NULL;
        }
        pp->decode_state = CODEC_HAS_INFO;
    }
    return (CODEC_PROTOTYPE *) pp;
}

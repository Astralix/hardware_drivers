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
--  Description :  Implementation for MPEG2 codec.
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_mpeg2"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_mpeg2.h"
#include "post_processor.h"
#include <mpeg2decapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

/* number of frame buffers decoder should allocate 0=AUTO */
//#define FRAME_BUFFERS 3
#define FRAME_BUFFERS 0

#define ALIGN(offset, align)    offset

//#define MPEG2_DECODE_STATISTICS

typedef struct CODEC_MPEG2
{
    CODEC_PROTOTYPE base;
    OMX_U32 framesize;
    OMX_BOOL enableDeblock;  // Needs PP pipeline mode
    Mpeg2DecInst instance;

    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;
    OMX_BOOL update_pp_out;
    OMX_U32 picId;
    OMX_BOOL extraEosLoopDone;
    OMX_BOOL interlaced;

#ifdef MPEG2_DECODE_STATISTICS
    FILE *stat_file;
#endif

} CODEC_MPEG2;

// destroy codec instance
static void decoder_destroy_mpeg2(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_MPEG2 *this = (CODEC_MPEG2 *) arg;

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
            Mpeg2DecRelease(this->instance);
            this->instance = 0;
        }

#ifdef MPEG2_DECODE_STATISTICS
        if (this->stat_file > 0) 
            fclose(this->stat_file);
#endif

        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_mpeg2(CODEC_PROTOTYPE * arg,
                                        STREAM_BUFFER * buf, OMX_U32 * consumed,
                                        FRAME * frame)
{
    CALLSTACK;

    CODEC_MPEG2 *this = (CODEC_MPEG2 *) arg;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    Mpeg2DecInput input;

    Mpeg2DecOutput output;

    memset(&input, 0, sizeof(Mpeg2DecInput));
    memset(&output, 0, sizeof(Mpeg2DecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    frame->size = 0;
    input.picId = this->picId;

#ifdef MPEG2_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    if(this->pp_state == PP_PIPELINE && this->update_pp_out == OMX_TRUE)
    {
        HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        PPResult res =
            HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
        this->update_pp_out = OMX_FALSE;
        if(res != PP_OK)
        {
            return CODEC_ERROR_INVALID_ARGUMENT;
        }
    }

#ifdef MPEG2_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    Mpeg2DecRet ret = Mpeg2DecDecode(this->instance, &input, &output);

#ifdef MPEG2_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %lu: size = %d bytes (%d bps), time to decode = %lu us\n", this->picId, input.dataLen, input.dataLen*30*8, ts);
    }
#endif

    TRACE_PRINT("Decoding ID %d\nMpeg2DecDecode returned: %d\n", (int)this->picId, (int)ret);

    switch (ret)
    {
    case MPEG2DEC_STRM_PROCESSED:
        stat = CODEC_NEED_MORE;
        break;
    case MPEG2DEC_PIC_DECODED:
        this->picId++;
        stat = CODEC_HAS_FRAME;
        break;
    case MPEG2DEC_HDRS_RDY:
        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        }
        stat = CODEC_HAS_INFO;
        break;
    case MPEG2DEC_PARAM_ERROR:
        stat = CODEC_ERROR_INVALID_ARGUMENT;
        break;
    case MPEG2DEC_STRM_ERROR:
        stat = CODEC_ERROR_STREAM;
        break;
    case MPEG2DEC_NOT_INITIALIZED:
        stat = CODEC_ERROR_NOT_INITIALIZED;
        break;
    case MPEG2DEC_HW_BUS_ERROR:
        stat = CODEC_ERROR_HW_BUS_ERROR;
        break;
    case MPEG2DEC_HW_TIMEOUT:
        stat = CODEC_ERROR_HW_TIMEOUT;
        break;
    case MPEG2DEC_SYSTEM_ERROR:
        stat = CODEC_ERROR_SYS;
        break;
    case MPEG2DEC_HW_RESERVED:
        stat = CODEC_ERROR_HW_RESERVED;
        break;
    case MPEG2DEC_MEMFAIL:
        stat = CODEC_ERROR_MEMFAIL;
        break;
    case MPEG2DEC_STREAM_NOT_SUPPORTED:
        stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
        break;
    default:
        assert(!"unhandled Mpeg2DecRet");
        break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {
        *consumed = input.dataLen - output.dataLeft;
    }

    return stat;
}

    // get stream info
static CODEC_STATE decoder_getinfo_mpeg2(CODEC_PROTOTYPE * arg,
                                         STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_MPEG2 *this = (CODEC_MPEG2 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    Mpeg2DecInfo info;

    memset(&info, 0, sizeof(Mpeg2DecInfo));
    Mpeg2DecRet ret = Mpeg2DecGetInfo(this->instance, &info);

    switch (ret)
    {
    case MPEG2DEC_OK:

        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;
        pkg->width = ALIGN(info.frameWidth, 16);
        pkg->height = ALIGN(info.frameHeight, 16);
        pkg->stride = ALIGN(info.codedWidth, 16);
        pkg->sliceheight = ALIGN(info.codedHeight, 16);
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
        pkg->interlaced = info.interlacedSequence;
        this->interlaced = info.interlacedSequence;

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
    case MPEG2DEC_PARAM_ERROR:
        return CODEC_ERROR_INVALID_ARGUMENT;
    case MPEG2DEC_HDRS_NOT_RDY:
        return CODEC_ERROR_STREAM;
    default:
        assert(!"unhandled Mpeg2DecRet");
        break;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get decoded frame
static CODEC_STATE decoder_getframe_mpeg2(CODEC_PROTOTYPE * arg, FRAME * frame,
                                          OMX_BOOL eos)
{
    CALLSTACK;

#ifdef MPEG2_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    CODEC_MPEG2 *this = (CODEC_MPEG2 *) arg;

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

    if(eos && this->extraEosLoopDone == OMX_FALSE && this->interlaced)
    {
        this->extraEosLoopDone = OMX_TRUE;
        eos = OMX_FALSE;
    }

    Mpeg2DecPicture picture;

#ifdef MPEG2_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    memset(&picture, 0, sizeof(Mpeg2DecPicture));
    Mpeg2DecRet ret = Mpeg2DecNextPicture(this->instance, &picture, eos);

#ifdef MPEG2_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %d: time of Mpeg2DecNextPicture = %lu us\n", picture.picId, ts);
    }
#endif

    switch (ret)
    {
    case MPEG2DEC_PIC_RDY:

#ifdef MPEG2_DECODE_STATISTICS
        gettimeofday(&tv1, NULL);
#endif

        if(this->pp_state != PP_DISABLED)
        {
            this->update_pp_out = OMX_TRUE;
            PPResult res = HantroHwDecOmx_pp_execute(this->pp_instance);

            if(res != PP_OK)
            {
                return CODEC_ERROR_UNSPECIFIED;
            }

            TRACE_PRINT("PP result: %d\n", res);
            if(res != PP_OK)
            {
                return CODEC_ERROR_UNSPECIFIED;
            }

        }
        else
        {
            memcpy(frame->fb_bus_data, picture.pOutputPicture, this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.numberOfErrMBs;

#ifdef MPEG2_DECODE_STATISTICS
        gettimeofday(&tv2, NULL);
        if (this->stat_file>0) {
            unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
            fprintf(this->stat_file, "frame %d: time of HantroHwDecOmx_pp_execute or memcpy = %lu us\n", picture.picId, ts);
        }
#endif

        TRACE_PRINT("BUFFERING: getframe MPEG2 NEXTPICTURE , ID %d\n", picture.picId);
        return CODEC_HAS_FRAME;
    case MPEG2DEC_OK:
        return CODEC_OK;
    default:
        break;
    }

    return CODEC_ERROR_UNSPECIFIED;
}

/* Find first and last startcode of stream. places masked by "first" and "last" */

static int decoder_scanframe_mpeg2(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                   OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;

    *first = 0;
    *last = 0;

#ifndef SKIP_SCANFRAME_MPEG2
    // scan for start code
    int i = 0;

    int z = 0;

    for(; (OMX_U32)i < buf->streamlen; ++i)
    {
        if(!buf->bus_data[i])
            ++z;
        /* match to frame or sequence level start code */
        else if((buf->bus_data[i] == 0x01) && (z >= 2) &&
                ((buf->bus_data[i + 1] > 0xAF) || (buf->bus_data[i + 1] == 0)))
        {
            *first = i - z;
            break;
        }
        else
            z = 0;
    }

    for(i = buf->streamlen - 3; i >= 0; --i)
    {
        // frame start code
        if((buf->bus_data[i] == 0x00) && (buf->bus_data[i + 1] == 0x00) &&
           (buf->bus_data[i + 2] == 0x01) && (buf->bus_data[i + 3] == 0x00))
        {
            *last = i;
            return 1;
        }
    }
    return -1;
#else
    *first = 0;
    *last = buf->streamlen;
    return 1;  
#endif
}

static CODEC_STATE decoder_setppargs_mpeg2(CODEC_PROTOTYPE * codec,
                                           PP_ARGS * args)
{
    CALLSTACK;

    CODEC_MPEG2 *this = (CODEC_MPEG2 *) codec;

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
            TRACE_PRINT("PP_PIPELINE\n");
            PPResult res;

            res =
                HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                  this->instance,
                                                  PP_PIPELINED_DEC_TYPE_MPEG2);
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
static CODEC_STATE decoder_setscaling_mpeg2(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_MPEG2 *this = (CODEC_MPEG2 *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_mpeg2(void)
{
    CALLSTACK;

    CODEC_MPEG2 *this = OSAL_Malloc(sizeof(CODEC_MPEG2));

    memset(this, 0, sizeof(CODEC_MPEG2));

    this->base.destroy = decoder_destroy_mpeg2;
    this->base.decode = decoder_decode_mpeg2;
    this->base.getinfo = decoder_getinfo_mpeg2;
    this->base.getframe = decoder_getframe_mpeg2;
    this->base.scanframe = decoder_scanframe_mpeg2;
    this->base.setppargs = decoder_setppargs_mpeg2;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_mpeg2;
#endif
    this->instance = 0;
    this->update_pp_out = OMX_FALSE;
    this->picId = 0;
    //this->extraEosLoopDone = OMX_FALSE;

#ifdef IS_G1_DECODER
    Mpeg2DecRet ret = Mpeg2DecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT,
                        FRAME_BUFFERS, DEC_REF_FRM_RASTER_SCAN);
#else
    Mpeg2DecRet ret = Mpeg2DecInit(&this->instance, USE_VIDEO_FREEZE_CONCEALMENT,
                        FRAME_BUFFERS);
#endif

    if(ret != MPEG2DEC_OK)
    {
        decoder_destroy_mpeg2((CODEC_PROTOTYPE *) this);
        return NULL;
    }

#ifdef MPEG2_DECODE_STATISTICS
    this->stat_file = fopen("/tmp/mpeg2_stat.txt", "w");
#endif

    return (CODEC_PROTOTYPE *) this;
}

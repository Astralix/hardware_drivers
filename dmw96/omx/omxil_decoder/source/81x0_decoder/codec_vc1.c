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
--  Description :  Implementation for VC1 codec
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_vc1"
#include <utils/Log.h>

#include "codec_vc1.h"
#include "post_processor.h"
#include <vc1decapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

/* number of frame buffers decoder should allocate 0=AUTO */
//#define FRAME_BUFFERS 3
#define FRAME_BUFFERS 0

#define SHOW4(p) (p[0]) | (p[1]<<8) | (p[2]<<16) | (p[3]<<24); p+=4;
#define ALIGN(offset, align)   ((((unsigned int)offset) + align-1) & ~(align-1))
//#define ALIGN(offset, align)    offset

//#define VC1_DECODE_STATISTICS

typedef enum VC1_CODEC_STATE
{
    VC1_PARSE_METADATA,
    VC1_DECODE
} VC1_CODEC_STATE;

typedef struct CODEC_VC1
{
    CODEC_PROTOTYPE base;

    VC1DecInst instance;
    VC1DecMetaData metadata;
    VC1_CODEC_STATE state;
    OMX_U32 framesize;
    // needed post-proc params
    // multiRes
    // rangeRed

    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;
    OMX_BOOL pipeline_enabled;
    OMX_BOOL update_pp_out;
    //OMX_BOOL extraEosLoopDone;

#ifdef VC1_DECODE_STATISTICS
    FILE *stat_file;
#endif
} CODEC_VC1;

// destroy codec instance
static void decoder_destroy_vc1(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) arg;

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
            VC1DecRelease(this->instance);
            this->instance = 0;
        }

#ifdef VC1_DECODE_STATISTICS
        if (this->stat_file > 0) 
            fclose(this->stat_file);
#endif

        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_vc1(CODEC_PROTOTYPE * arg,
                                      STREAM_BUFFER * buf, OMX_U32 * consumed,
                                      FRAME * frame)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) arg;

    assert(this);
    assert(buf);
    assert(consumed);

#ifdef VC1_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    frame->size = 0;
    VC1DecRet ret;

    if(this->state == VC1_PARSE_METADATA)
    {
        if((buf->bus_data[0] == 0x00) && (buf->bus_data[1] == 0x00) &&
           (buf->bus_data[2] == 0x01))
        {
            TRACE("ADVANCED PROFILE\n");
            // is advanced profile
            this->metadata.profile = 8;

#ifdef IS_G1_DECODER
            ret = VC1DecInit(&this->instance, &this->metadata,
                    USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS,
                    DEC_REF_FRM_RASTER_SCAN);
#else
            ret = VC1DecInit(&this->instance, &this->metadata,
                    USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS);
#endif
            this->state = VC1_DECODE;

            if(this->pp_state == PP_PIPELINE)
            {
                PPResult res;

                res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_VC1);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                TRACE("PP_COMBINED_MODE\n");
                this->pipeline_enabled = OMX_TRUE;
            }
            *consumed = 0;
            return CODEC_NEED_MORE;
        }
        else
        {
            TRACE("MAIN OR SIMPLE PROFILE\n");
            OMX_U8 *p = buf->bus_data + 12;

            this->metadata.maxCodedHeight = SHOW4(p);
            this->metadata.maxCodedWidth = SHOW4(p);
            ret = VC1DecUnpackMetaData(buf->bus_data + 8, 4, &this->metadata);

            if(ret == VC1DEC_PARAM_ERROR)
                return CODEC_ERROR_INVALID_ARGUMENT;
            else if(ret == VC1DEC_METADATA_FAIL)
                return CODEC_ERROR_STREAM;
        }
        if(ret == VC1DEC_OK)
        {
#ifdef IS_G1_DECODER
            ret = VC1DecInit(&this->instance, &this->metadata,
                    USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS,
                    DEC_REF_FRM_RASTER_SCAN);
#else
            ret = VC1DecInit(&this->instance, &this->metadata,
                    USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS);
#endif
            this->state = VC1_DECODE;

            if(this->pp_state == PP_PIPELINE)
            {
                PPResult res;

                res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_VC1);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                TRACE("PP_COMBINED_MODE\n");
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
                this->pipeline_enabled = OMX_TRUE;
            }

            *consumed = buf->streamlen;
            return CODEC_HAS_INFO;
        }
    }
    else if(this->state == VC1_DECODE)
    {
        VC1DecInput input;

        memset(&input, 0, sizeof(VC1DecInput));

        input.pStream = buf->bus_data;
        input.streamBusAddress = buf->bus_address;
        input.streamSize = buf->streamlen;
        input.picId = 0;

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

        VC1DecOutput output;

#ifdef VC1_DECODE_STATISTICS
        gettimeofday(&tv1, NULL);
#endif

        ret = VC1DecDecode(this->instance, &input, &output);

#ifdef VC1_DECODE_STATISTICS
        gettimeofday(&tv2, NULL);
        if (this->stat_file>0) {
            unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
            fprintf(this->stat_file, "frame %lu: size = %d bytes, time of VC1DecDecode = %lu us\n", input.picId, input.streamSize, ts);
        }
#endif

        if(this->metadata.profile == 8)
        {
            *consumed = input.streamSize - output.dataLeft;
        }

        TRACE("VC1DecDecode returned: %d\n", ret);
        switch (ret)
        {
        case VC1DEC_PIC_DECODED:
            if(this->metadata.profile != 8)
            {
                *consumed = buf->streamlen;
            }
            return CODEC_HAS_FRAME;
        case VC1DEC_STRM_ERROR:
            return CODEC_ERROR_STREAM;
        case VC1DEC_MEMFAIL:
            return CODEC_ERROR_MEMFAIL;
        case VC1DEC_END_OF_SEQ:
            return CODEC_NEED_MORE; //??
        case VC1DEC_HDRS_RDY:
            if(this->pp_state == PP_PIPELINE)
            {
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
            }
            return CODEC_HAS_INFO;
        case VC1DEC_RESOLUTION_CHANGED:
            return CODEC_HAS_INFO;  //??
        case VC1DEC_STRM_PROCESSED:
            return CODEC_NEED_MORE;
        case VC1DEC_PARAM_ERROR:
            return CODEC_ERROR_INVALID_ARGUMENT;
        case VC1DEC_NOT_INITIALIZED:
            return CODEC_ERROR_NOT_INITIALIZED;
        case VC1DEC_HW_BUS_ERROR:
            return CODEC_ERROR_HW_BUS_ERROR;
        case VC1DEC_HW_TIMEOUT:
            return CODEC_ERROR_HW_TIMEOUT;
        case VC1DEC_SYSTEM_ERROR:
            return CODEC_ERROR_SYS;
        case VC1DEC_HW_RESERVED:
            return CODEC_ERROR_HW_RESERVED;
        default:
            assert(!"unhandled VC1DecRet");
            break;
        }

    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get stream info
static CODEC_STATE decoder_getinfo_vc1(CODEC_PROTOTYPE * arg, STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;
    pkg->isVc1Stream = OMX_TRUE;

    if(this->metadata.profile == 8) //advanced
    {
        VC1DecInfo info;

        memset(&info, 0, sizeof(VC1DecInfo));
        VC1DecGetInfo(this->instance, &info);

        if(info.interlacedSequence)
        {
            TRACE("INTERLACED SEQUENCE\n");
        }

        pkg->width = ALIGN(info.maxCodedWidth, 8);
        pkg->height = ALIGN(info.maxCodedHeight, 8);
        pkg->stride = ALIGN(info.maxCodedWidth, 8);
        pkg->sliceheight = ALIGN(info.maxCodedHeight, 8);
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
        pkg->interlaced = info.interlacedSequence;
    }
    else
    {
        pkg->width = ALIGN(this->metadata.maxCodedWidth, 8);
        pkg->height = ALIGN(this->metadata.maxCodedHeight, 8);
        pkg->stride = ALIGN(this->metadata.maxCodedWidth, 8);
        pkg->sliceheight = ALIGN(this->metadata.maxCodedHeight, 8);
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
    }

    if(this->pp_state != PP_DISABLED)
    {
        if(this->transforms == PPTR_DEINTERLACE)
        {
            this->update_pp_out = OMX_TRUE;
        }

        TRACE("this->pp_state != PP_DISABLED\n");
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

// get decoded frame
static CODEC_STATE decoder_getframe_vc1(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) arg;

#ifdef VC1_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

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

    VC1DecPicture picture;

    memset(&picture, 0, sizeof(VC1DecPicture));

#ifdef VC1_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    VC1DecRet ret = VC1DecNextPicture(this->instance, &picture, eos);

#ifdef VC1_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %d: time of VC1DecNextPicture = %lu us\n", picture.picId, ts);
    }
#endif

    TRACE("VC1DecNextPicture returned: %d\n", ret);

    switch (ret)
    {
    case VC1DEC_OK:
        return CODEC_OK;
    case VC1DEC_PIC_RDY:
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
            memcpy(frame->fb_bus_data, picture.pOutputPicture, this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.numberOfErrMBs;
        return CODEC_HAS_FRAME;
    case VC1DEC_PARAM_ERROR:
    case VC1DEC_NOT_INITIALIZED:
        return CODEC_ERROR_INVALID_ARGUMENT;
    default:
        assert(!"unhandled VC1DecRet");
        break;
    }

    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_vc1(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) arg;

    assert(this);

    if(this->metadata.profile == 8) //advanced
    {
        *first = 0;
        *last = 0;
#ifndef SKIP_SCANFRAME_VC1
        // scan for start code
        int i = 0;

        int z = 0;

        for(; (OMX_U32)i < buf->streamlen; ++i)
        {
            if(!buf->bus_data[i])
                ++z;
            else if(buf->bus_data[i] == 0x01 && z >= 2)
            {
                *first = i - z;
                TRACE("first: %02x, %02x, %02x, %02x\n", buf->bus_data[i - 2],
                      buf->bus_data[i - 1], buf->bus_data[i],
                      buf->bus_data[i + 1]);
                break;
            }
            else
                z = 0;
        }

        for(i = buf->streamlen - 3; i >= 0; --i)
        {
            if((buf->bus_data[i] == 0x00) &&
               (buf->bus_data[i + 1] == 0x00) && (buf->bus_data[i + 2] == 0x01))
            {
                *last = i;
                TRACE(" last: %02x, %02x, %02x, %02x\n", buf->bus_data[i],
                      buf->bus_data[i + 1], buf->bus_data[i + 2],
                      buf->bus_data[i + 3]);
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
    else
    {
        *first = 0;
        *last = buf->streamlen;
        return 1;
    }
}

static CODEC_STATE decoder_setppargs_vc1(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;

    CODEC_VC1 *this = (CODEC_VC1 *) codec;

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
static CODEC_STATE decoder_setscaling_vc1(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_VC1 *this = (CODEC_VC1 *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_vc1()
{
    CALLSTACK;

    CODEC_VC1 *this = OSAL_Malloc(sizeof(CODEC_VC1));

    memset(this, 0, sizeof(CODEC_VC1));

    this->base.destroy = decoder_destroy_vc1;
    this->base.decode = decoder_decode_vc1;
    this->base.getinfo = decoder_getinfo_vc1;
    this->base.getframe = decoder_getframe_vc1;
    this->base.scanframe = decoder_scanframe_vc1;
    this->base.setppargs = decoder_setppargs_vc1;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_vc1;
#endif
    this->instance = 0;
    this->pipeline_enabled = OMX_FALSE;

    this->state = VC1_PARSE_METADATA;

#ifdef VC1_DECODE_STATISTICS
    this->stat_file = fopen("/tmp/vc1_stat.txt", "w");
#endif

    return (CODEC_PROTOTYPE *) this;
}

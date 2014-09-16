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
--  Description :  Implementation for MPEG4/H.263 codec.
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_mpeg4"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_mpeg4.h"
#include "post_processor.h"
#include <mp4decapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

/* number of frame buffers decoder should allocate 0=AUTO */
//#define FRAME_BUFFERS 3
#define FRAME_BUFFERS 0

#define ALIGN(offset, align)    offset

//#define MPEG4_DECODE_STATISTICS

typedef enum MPEG4_CODEC_STATE
{
    MPEG4_DECODE,
    MPEG4_PARSE_METADATA
} MPEG4_CODEC_STATE;

typedef struct CODEC_MPEG4
{
    CODEC_PROTOTYPE base;
    OMX_U32 framesize;
    OMX_BOOL enableDeblock;  // Needs PP pipeline mode
    MP4DecInst instance;

    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;
    OMX_BOOL update_pp_out;
    OMX_U32 picId;
    OMX_BOOL extraEosLoopDone;
    MPEG4_FORMAT format;
    MPEG4_CODEC_STATE state;
    OMX_S32 custom1Width;
    OMX_S32 custom1Height;
    OMX_BOOL dispatchOnlyFrame;
    OMX_BOOL interlaced;

#ifdef MPEG4_DECODE_STATISTICS
    FILE *stat_file;
#endif
} CODEC_MPEG4;

// destroy codec instance
static void decoder_destroy_mpeg4(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) arg;

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
            MP4DecRelease(this->instance);
            this->instance = 0;
        }

#ifdef MPEG4_DECODE_STATISTICS
        if (this->stat_file > 0) 
            fclose(this->stat_file);
#endif

        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_mpeg4(CODEC_PROTOTYPE * arg,
                                        STREAM_BUFFER * buf, OMX_U32 * consumed,
                                        FRAME * frame)
{
    CALLSTACK;

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) arg;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    if(this->state == MPEG4_PARSE_METADATA)
    {
        TRACE_PRINT("PARSE_METADATA\n");
        this->state = MPEG4_DECODE;
        this->custom1Width =
            (buf->bus_data[0]) | (buf->bus_data[1] << 8) |
            (buf->bus_data[2] << 16) | (buf->bus_data[3] << 24);
        this->custom1Height =
            (buf->bus_data[4]) | (buf->bus_data[5] << 8) |
            (buf->bus_data[6] << 16) | (buf->bus_data[7] << 24);
        MP4DecSetInfo(this->instance, this->custom1Width, this->custom1Height);
        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        }
        //*consumed = buf->streamlen;
        *consumed = 8;
        return CODEC_HAS_INFO;
    }

    MP4DecInput input;

    MP4DecOutput output;

    memset(&input, 0, sizeof(MP4DecInput));
    memset(&output, 0, sizeof(MP4DecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    input.enableDeblock = this->enableDeblock;
    frame->size = 0;
    input.picId = this->picId;

#ifdef MPEG4_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    if (this->pp_config.ppOutImg.bufferBusAddr != output.strmCurrBusAddress)
    {
        this->update_pp_out = OMX_TRUE;
    }

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
    TRACE_PRINT("MP4DecDecode\n input.pStream=0x%08x\n input.streamBusAddress=0x%08x\n",(unsigned int) input.pStream, input.streamBusAddress);
#if 0
    {
        u8 *poi = input.pStream;
        u8 cou = 0;
        for(cou = 0; cou < 40; cou += 4)
            TRACE_PRINT( "data %x %x %x %x\n",
                    *(poi + cou + 0), *(poi + cou + 1), *(poi + cou + 2),
                    *(poi + cou + 3));
    }
#endif

#ifdef MPEG4_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif
    MP4DecRet ret = MP4DecDecode(this->instance, &input, &output);

#ifdef MPEG4_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %lu: size = %d bytes (%d bps), time to decode = %lu us\n", this->picId, input.dataLen, input.dataLen*30*8, ts);
    }
#endif

    TRACE_PRINT("Decoding ID %d\nMP4DecDecode returned: %d\n", (int) this->picId,ret);
    TRACE_PRINT("  dataLen = %d\n  dataLeft=%d\n",input.dataLen,output.dataLeft);

    switch (ret)
    {

    case MP4DEC_PIC_DECODED:
        TRACE_PRINT("MP4DecDecode: PIC_DECODED\n");
        this->picId++;
        stat = CODEC_HAS_FRAME;
        break;
    case MP4DEC_STRM_NOT_SUPPORTED:
        LOGE("MP4DecDecode: STRM_NOT_SUPPORTED\n");
        stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
        break;
    case MP4DEC_STRM_PROCESSED:
        TRACE_PRINT("MP4DecDecode: STRM_PROCESSED\n");
        stat = CODEC_NEED_MORE;
        break;
    case MP4DEC_VOS_END:   //NOTE: ??
        TRACE_PRINT("MP4DecDecode: VOS_END\n");
        stat = CODEC_HAS_FRAME;
        this->dispatchOnlyFrame = OMX_TRUE;
        break;
    case MP4DEC_HDRS_RDY:
        TRACE_PRINT("MP4DecDecode: HDRS_RDY\n");
    case MP4DEC_DP_HDRS_RDY:
        TRACE_PRINT("MP4DecDecode: DP_HDRS_RDY\n");
        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        }
        stat = CODEC_HAS_INFO;
        break;
    case MP4DEC_PARAM_ERROR:
        LOGE("MP4DecDecode: PARAM_ERROR\n");
        stat = CODEC_ERROR_INVALID_ARGUMENT;
        break;
    case MP4DEC_STRM_ERROR:
        LOGE("MP4DecDecode: STRM_ERROR\n");
        stat = CODEC_ERROR_STREAM;
        break;
    case MP4DEC_NOT_INITIALIZED:
        LOGE("MP4DecDecode: NOT_INITIALIZED\n");
        stat = CODEC_ERROR_NOT_INITIALIZED;
        break;
    case MP4DEC_HW_BUS_ERROR:
        LOGE("MP4DecDecode: HW_BUS_ERROR\n");
        stat = CODEC_ERROR_HW_BUS_ERROR;
        break;
    case MP4DEC_HW_TIMEOUT:
        LOGW("MP4DecDecode: HW_TIMEOUT\n");
        stat = CODEC_ERROR_HW_TIMEOUT;
        break;
    case MP4DEC_SYSTEM_ERROR:
        LOGE("MP4DecDecode: SYSTEM_ERROR\n");
        stat = CODEC_ERROR_SYS;
        break;
    case MP4DEC_HW_RESERVED:
        LOGE("MP4DecDecode: HW_RESERVED\n");
        stat = CODEC_ERROR_HW_RESERVED;
        break;
    case MP4DEC_FORMAT_NOT_SUPPORTED:
        LOGE("MP4DecDecode: NOT_SUPPORTED\n");
        stat = CODEC_ERROR_FORMAT_NOT_SUPPORTED;
        break;
    case MP4DEC_MEMFAIL:
        LOGE("MP4DecDecode: MEMFAIL\n");
        stat = CODEC_ERROR_MEMFAIL;
        break;
    default:
        LOGE("MP4DecDecode: unhandled MP4DecRet\n");
        assert(!"unhandled MP4DecRet");
        break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {
        *consumed = input.dataLen - output.dataLeft;
    }
    return stat;
}

    // get stream info
static CODEC_STATE decoder_getinfo_mpeg4(CODEC_PROTOTYPE * arg,
                                         STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    if(this->format == MPEG4FORMAT_CUSTOM_1_3)
    {
        pkg->width = ALIGN(this->custom1Width, 16);
        pkg->height = ALIGN(this->custom1Height, 16);
        pkg->stride = ALIGN(this->custom1Width, 16);
        pkg->sliceheight = ALIGN(this->custom1Height, 16);
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
    }
    else
    {
        MP4DecInfo info;

        memset(&info, 0, sizeof(MP4DecInfo));
        MP4DecRet ret = MP4DecGetInfo(this->instance, &info);

        switch (ret)
        {
        case MP4DEC_OK:
            break;
        case MP4DEC_PARAM_ERROR:
            return CODEC_ERROR_INVALID_ARGUMENT;
        case MP4DEC_HDRS_NOT_RDY:
            return CODEC_ERROR_STREAM;
        default:
            assert(!"unhandled MP4DecRet");
            return CODEC_ERROR_UNSPECIFIED;
        }
        pkg->width = ALIGN(info.frameWidth, 16);
        pkg->height = ALIGN(info.frameHeight, 16);
        pkg->stride = ALIGN(info.codedWidth, 16);
        pkg->sliceheight = ALIGN(info.codedHeight, 16);
        pkg->interlaced = info.interlacedSequence;
        this->interlaced = info.interlacedSequence;
        pkg->framesize = pkg->width * pkg->height * 3 / 2;
    }

    pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;

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

// get decoded frame
static CODEC_STATE decoder_getframe_mpeg4(CODEC_PROTOTYPE * arg, FRAME * frame,
                                          OMX_BOOL eos)
{
    CALLSTACK;

#ifdef MPEG4_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) arg;

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

    if(eos && (this->extraEosLoopDone == OMX_FALSE) && this->interlaced)
    {
        this->extraEosLoopDone = OMX_TRUE;
        eos = OMX_FALSE;
    }

    MP4DecPicture picture;

    memset(&picture, 0, sizeof(MP4DecPicture));

    if (this->dispatchOnlyFrame == OMX_TRUE)
    {
        eos = OMX_TRUE;
    }

#ifdef MPEG4_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    MP4DecRet ret = MP4DecNextPicture(this->instance, &picture, eos);

#ifdef MPEG4_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %d: time of MP4DecNextPicture = %lu us\n", picture.picId, ts);
    }
#endif

    switch (ret)
    {
    case MP4DEC_PIC_RDY:
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
#ifdef MPEG4_DECODE_STATISTICS
        if (this->stat_file) {
            fprintf(this->stat_file, "[%s] MP4DEC_PIC_RDY - memcpy\n", __func__);
        }
#endif
            memcpy(frame->fb_bus_data, picture.pOutputPicture, this->framesize);
        }
        frame->size = this->framesize;
        frame->MB_err_count = picture.nbrOfErrMBs;
        TRACE_PRINT("BUFFERING: getframe MPEG4 NEXTPICTURE , ID %d\n", picture.picId);
        return CODEC_HAS_FRAME;
    case MP4DEC_OK:
#ifdef MPEG4_DECODE_STATISTICS
        if (this->stat_file) {
            fprintf(this->stat_file, "[%s] MP4DEC_OK\n", __func__);
        }
#endif
        TRACE_PRINT("getframe MPEG4 API NEXTPICTURE  OK\n");
        return CODEC_OK;
    default:
        break;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_mpeg4(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                   OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) arg;

    assert(this);
#ifdef HANTRO_TESTBENCH
    if(this->format != MPEG4FORMAT_CUSTOM_1_3 &&
        this->format != MPEG4FORMAT_CUSTOM_1 &&
        this->format != MPEG4FORMAT_SORENSON)
    {
        *first = 0;
        *last = 0;
        // scan for start code
        int i = 0;

        int z = 0;

        for(; i < buf->streamlen; ++i)
        {
            if(!buf->bus_data[i])
                ++z;
            else if(buf->bus_data[i] == 0x01 && z >= 2)
            {
                *first = i - z;
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
                return 1;
            }
        }
        return -1;
    }
    else
    {
        *first = 0;
        *last = buf->streamlen;
        return 1;
    }
#else
    *first = 0;
    *last = buf->streamlen;
    return 1;
#endif
}

static CODEC_STATE decoder_setppargs_mpeg4(CODEC_PROTOTYPE * codec,
                                           PP_ARGS * args)
{
    CALLSTACK;

    CODEC_MPEG4 *this = (CODEC_MPEG4 *) codec;

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
                                                  PP_PIPELINED_DEC_TYPE_MPEG4);
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
static CODEC_STATE decoder_setscaling_mpeg4(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_MPEG4 *this = (CODEC_MPEG4 *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_mpeg4(OMX_BOOL enable_deblocking,
                                                     MPEG4_FORMAT format)
{
    CALLSTACK;

    MP4DecStrmFmt strmFormat;

    CODEC_MPEG4 *this = OSAL_Malloc(sizeof(CODEC_MPEG4));

    memset(this, 0, sizeof(CODEC_MPEG4));

    this->base.destroy = decoder_destroy_mpeg4;
    this->base.decode = decoder_decode_mpeg4;
    this->base.getinfo = decoder_getinfo_mpeg4;
    this->base.getframe = decoder_getframe_mpeg4;
    this->base.scanframe = decoder_scanframe_mpeg4;
    this->base.setppargs = decoder_setppargs_mpeg4;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_mpeg4;
#endif
    this->instance = 0;
    this->update_pp_out = OMX_FALSE;
    this->picId = 0;
    this->extraEosLoopDone = OMX_FALSE;
    this->format = format;
    this->custom1Width = 0;
    this->custom1Height = 0;
    this->state = MPEG4_DECODE;
    this->dispatchOnlyFrame = OMX_FALSE;

    if(enable_deblocking)
    {
        this->enableDeblock = 1;
    }

    switch (format)
    {
    case MPEG4FORMAT_MPEG4:
        strmFormat = MP4DEC_MPEG4;
        break;
    case MPEG4FORMAT_SORENSON:
        strmFormat = MP4DEC_SORENSON;
        break;
    case MPEG4FORMAT_CUSTOM_1:
        strmFormat = MP4DEC_CUSTOM_1;
        break;
    case MPEG4FORMAT_CUSTOM_1_3:
        strmFormat = MP4DEC_CUSTOM_1;
        this->state = MPEG4_PARSE_METADATA;
        break;
    default:
        strmFormat = MP4DEC_MPEG4;
        break;
    }

#ifdef IS_G1_DECODER
    MP4DecRet ret = MP4DecInit(&this->instance, strmFormat,
                        USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS,
                        DEC_REF_FRM_RASTER_SCAN);
#else
    MP4DecRet ret = MP4DecInit(&this->instance, strmFormat,
                        USE_VIDEO_FREEZE_CONCEALMENT, FRAME_BUFFERS);
#endif

    if(ret != MP4DEC_OK)
    {
        decoder_destroy_mpeg4((CODEC_PROTOTYPE *) this);
        return NULL;
    }

#ifdef MPEG4_DECODE_STATISTICS
    this->stat_file = fopen("/tmp/mpeg4_stat.txt", "w");
#endif

    return (CODEC_PROTOTYPE *) this;
}

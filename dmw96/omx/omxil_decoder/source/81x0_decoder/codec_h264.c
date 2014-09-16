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
--  Description :  Implementation for H.264 codec
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_h264"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "codec_h264.h"
#include "post_processor.h"
#include "h264decapi.h"
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset, memcpy

#define DISABLE_OUTPUT_REORDER 0
#define USE_DISPLAY_SMOOTHING 1

#ifndef OMX_DECODER_TRACE
static int fprintf_dummy(const char *f, ...)
{
    return 0;
}
#endif

//#define H264_DECODE_STATISTICS

typedef struct CODEC_H264
{
    CODEC_PROTOTYPE base;
    OMX_U32 framesize;
    H264DecInst instance;
    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;
    OMX_U32 picId;
    //OMX_BOOL extraEosLoopDone;
    OMX_BOOL update_pp_out;

#ifdef H264_DECODE_STATISTICS
    FILE *stat_file;
#endif
} CODEC_H264;

// destroy codec instance
static void decoder_destroy_h264(CODEC_PROTOTYPE * arg)
{
    CODEC_H264 *this = (CODEC_H264 *) arg;

    CALLSTACK;
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
            H264DecRelease(this->instance);
            this->instance = 0;
        }

#ifdef H264_DECODE_STATISTICS
        if (this->stat_file > 0) 
            fclose(this->stat_file);
#endif

        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_h264(CODEC_PROTOTYPE * arg,
                                       STREAM_BUFFER * buf, OMX_U32 * consumed,
                                       FRAME * frame)
{
    H264DecInput input;

    H264DecOutput output;

    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    CODEC_H264 *this = (CODEC_H264 *) arg;

    CALLSTACK;

    assert(this);
    assert(this->instance);
    assert(buf);
    assert(consumed);

#ifdef H264_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

    memset(&input, 0, sizeof(H264DecInput));
    memset(&output, 0, sizeof(H264DecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    input.picId = this->picId;
    TRACE_PRINT( "Pic id %d, stream lenght %d \n", input.picId,
                input.dataLen);
    *consumed = 0;
    frame->size = 0;

    if (this->pp_config.ppOutImg.bufferBusAddr != output.strmCurrBusAddress)
    {
        this->update_pp_out = OMX_TRUE;
    }

    if (this->pp_state == PP_PIPELINE && this->update_pp_out == OMX_TRUE)
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

#ifdef H264_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    H264DecRet ret = H264DecDecode(this->instance, &input, &output);

#ifdef H264_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %lu: size = %d bytes (%d bps), time of H264DecDecode = %lu us\n", this->picId, input.dataLen, input.dataLen*30*8, ts);
    }
#endif

    TRACE_PRINT( "from decoder: dataleft %d used %d\n", output.dataLeft,
                input.dataLen - output.dataLeft);
    switch (ret)
    {
    case H264DEC_PENDING_FLUSH:
        this->picId++;
        stat = CODEC_HAS_FRAME;
        break;
    case H264DEC_PIC_DECODED:
        this->picId++;
        stat = CODEC_HAS_FRAME;
#if 0
        {
            OMX_U32 jii;
            for(jii = 0; jii<640; jii++)
            {
                *(frame->fb_bus_data + jii) = 0x0;

                *(frame->fb_bus_data + 4*640 + jii) = 0x0;
                *(frame->fb_bus_data + 100*640 + jii/2) = 0x0;
                *(frame->fb_bus_data + 32*640 + jii*2) = 0x0;
                *(frame->fb_bus_data + 240*640 + jii*2) = 0x0;

            }

            memset(frame->fb_bus_data+640*480, 0, 640*480/2 );
        // TODO
        }
#endif
        break;
    case H264DEC_HDRS_RDY:
        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
        }
        stat = CODEC_HAS_INFO;
        break;
    case H264DEC_ADVANCED_TOOLS:
        stat = CODEC_NEED_MORE;
        break;
    case H264DEC_STRM_PROCESSED:
        stat = CODEC_NEED_MORE;
        break;
    case H264DEC_PARAM_ERROR:
        stat = CODEC_ERROR_INVALID_ARGUMENT;
        break;
    case H264DEC_STRM_ERROR:
        stat = CODEC_ERROR_STREAM;
        break;
    case H264DEC_NOT_INITIALIZED:
        stat = CODEC_ERROR_NOT_INITIALIZED;
        break;
    case H264DEC_HW_BUS_ERROR:
        stat = CODEC_ERROR_HW_BUS_ERROR;
        break;
    case H264DEC_HW_TIMEOUT:
        stat = CODEC_ERROR_HW_TIMEOUT;
        break;
    case H264DEC_SYSTEM_ERROR:
        stat = CODEC_ERROR_SYS;
        break;
    case H264DEC_HW_RESERVED:
        stat = CODEC_ERROR_HW_RESERVED;
        break;
    case H264DEC_MEMFAIL:
        stat = CODEC_ERROR_MEMFAIL;
        break;
    case H264DEC_STREAM_NOT_SUPPORTED:
        stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
        break;
    default:
        assert(!"unhandled H264DecRet");
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
static CODEC_STATE decoder_getinfo_h264(CODEC_PROTOTYPE * arg,
                                        STREAM_INFO * pkg)
{
    H264DecInfo decinfo;

    CODEC_H264 *this = (CODEC_H264 *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    CALLSTACK;

    memset(&decinfo, 0, sizeof(H264DecInfo));

    // the headers are ready, get stream information
    H264DecRet ret = H264DecGetInfo(this->instance, &decinfo);

    this = (CODEC_H264 *) arg;
    if(ret == H264DEC_OK)
    {
        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;

        pkg->width = decinfo.picWidth;//decinfo.cropParams.cropOutWidth;
        pkg->height = decinfo.picHeight;//decinfo.cropParams.cropOutHeight;
        pkg->sliceheight = decinfo.picHeight;
        pkg->stride = decinfo.picWidth;
#if defined (IS_8190) || defined (IS_G1_DECODER)
        pkg->interlaced = decinfo.interlacedSequence;
#else
        pkg->interlaced = 0;
#endif
        pkg->framesize = (pkg->sliceheight * pkg->stride) * 3 / 2;

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
    else if(ret == H264DEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    else if(ret == H264DEC_HDRS_NOT_RDY)
    {
        return CODEC_ERROR_STREAM;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get decoded frame
static CODEC_STATE decoder_getframe_h264(CODEC_PROTOTYPE * arg, FRAME * frame,
                                         OMX_BOOL eos)
{
    H264DecPicture picture;

    CODEC_H264 *this = (CODEC_H264 *) arg;

#ifdef H264_DECODE_STATISTICS
    struct timeval tv1, tv2;
#endif

	assert(this != 0);
    assert(this->instance != 0);
    assert(frame);

    if( (this->pp_state == PP_PIPELINE) && 
        (eos || (this->pp_config.ppOutImg.bufferBusAddr != frame->fb_bus_address)) )
    {
        TRACE_PRINT("Change pp output\n");
        if (eos == OMX_TRUE)
            TRACE_PRINT("EOS\n");
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

    memset(&picture, 0, sizeof(H264DecPicture));

    u32 endofStream = eos == OMX_TRUE ? 1 : 0;

#ifdef H264_DECODE_STATISTICS
    gettimeofday(&tv1, NULL);
#endif

    H264DecRet ret = H264DecNextPicture(this->instance, &picture, endofStream);

#ifdef H264_DECODE_STATISTICS
    gettimeofday(&tv2, NULL);
    if (this->stat_file>0) {
        unsigned long ts = (tv2.tv_sec == tv1.tv_sec)? (tv2.tv_usec - tv1.tv_usec) : (1000000 - tv1.tv_usec + tv2.tv_usec);
        fprintf(this->stat_file, "frame %d: time of H264DecNextPicture = %lu us\n", picture.picId, ts);
    }
#endif

	this = (CODEC_H264 *) arg;

    if(ret == H264DEC_PIC_RDY)
    {
        assert(this->framesize);
        
        LOGV("PIC ID %d\n", picture.picId);
        TRACE_PRINT( "err mbs %d \n", picture.nbrOfErrMBs);

        if(this->pp_state != PP_DISABLED)
        {
            this->update_pp_out = OMX_TRUE;
            PPResult res = HantroHwDecOmx_pp_execute(this->pp_instance);

            if(res != PP_OK)
            {
                return CODEC_ERROR_UNSPECIFIED;
            }
/*            if(this->pp_state == PP_STANDALONE)
            {
                HantroHwDecOmx_pp_set_input_buffer(&this->pp_config, picture.outputPictureBusAddress);
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
                PPResult res = HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
                if(res!=PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                res = HantroHwDecOmx_pp_execute(this->pp_instance);
                if(res!=PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
           }*/
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
        frame->MB_err_count = picture.nbrOfErrMBs;

        return CODEC_HAS_FRAME;
    }
    else if(ret == H264DEC_OK)
    {
        return CODEC_OK;
    }
    else if(ret == H264DEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_h264(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                  OMX_U32 * first, OMX_U32 * last)
{
    // find the first and last start code offsets.
    // returns 1 if start codes are found otherwise -1
    // this doesnt find anything if there's only a single NAL unit in the buffer?
    *first = 0;
    *last = 0;
#ifdef HANTRO_TESTBENCH
    // scan for a NAL
    int i = 0;

    int z = 0;

    CALLSTACK;

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
        if(buf->bus_data[i] == 0 && buf->bus_data[i + 1] == 0
           && buf->bus_data[i + 2] == 1)
        {
            /* Check for leading zeros */
            while(i > 0)
            {
                if(buf->bus_data[i])
                {
                    *last = i + 1;
                    break;
                }
                --i;
            }
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

static CODEC_STATE decoder_setppargs_h264(CODEC_PROTOTYPE * codec,
                                          PP_ARGS * args)
{
    PPResult ret;

    CODEC_H264 *this = (CODEC_H264 *) codec;

    assert(this);
    assert(args);

    CALLSTACK;

    ret = HantroHwDecOmx_pp_init(&this->pp_instance, &this->pp_config);

    if(ret == PP_OK)
    {
        this->transforms =
            HantroHwDecOmx_pp_set_output(&this->pp_config, args, OMX_TRUE);
        if(this->transforms == PPTR_ARG_ERROR)
            return CODEC_ERROR_INVALID_ARGUMENT;

#ifdef ANDROID_MOD
		TRACE_PRINT( "this->transforms = 0x%x\n", this->transforms);
#endif

#ifdef ENABLE_PP
        if(this->transforms == PPTR_NONE)
        {
            this->pp_state = PP_DISABLED;
        }
        else
        {
            if( /*this->transforms == PPTR_SCALE  ||
                 * this->transforms == PPTR_FORMAT ||
                 * this->transforms == (PPTR_SCALE | PPTR_FORMAT) */ 1)
            {
                PPResult res;

                res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_H264);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                this->pp_state = PP_PIPELINE;
            }
            else
            {
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
static CODEC_STATE decoder_setscaling_h264(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_H264 *this = (CODEC_H264 *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_h264(OMX_BOOL conceal_errors)
{
    CODEC_H264 *this = OSAL_Malloc(sizeof(CODEC_H264));

    memset(this, 0, sizeof(CODEC_H264));

    CALLSTACK;

    this->base.destroy = decoder_destroy_h264;
    this->base.decode = decoder_decode_h264;
    this->base.getinfo = decoder_getinfo_h264;
    this->base.getframe = decoder_getframe_h264;
    this->base.scanframe = decoder_scanframe_h264;
    this->base.setppargs = decoder_setppargs_h264;
#ifdef DYNAMIC_SCALING
    this->base.setscaling = decoder_setscaling_h264;
#endif
    this->instance = 0;
    this->picId++;

#ifdef IS_G1_DECODER
    H264DecRet ret = H264DecInit(&this->instance, DISABLE_OUTPUT_REORDER,
                        USE_VIDEO_FREEZE_CONCEALMENT, USE_DISPLAY_SMOOTHING,
                        DEC_REF_FRM_RASTER_SCAN);
#ifdef MVC_SUPPORT
    if(ret == H264DEC_OK)
        ret = H264DecSetMvc(this->instance);
#endif
#endif

#ifdef IS_8190
    H264DecRet ret = H264DecInit(&this->instance, DISABLE_OUTPUT_REORDER,
                        USE_VIDEO_FREEZE_CONCEALMENT, USE_DISPLAY_SMOOTHING);
#endif

#if !defined (IS_8190) && !defined (IS_G1_DECODER)
    H264DecRet ret = H264DecInit(&this->instance, DISABLE_OUTPUT_REORDER);
#endif

    if(ret != H264DEC_OK)
    {
        decoder_destroy_h264((CODEC_PROTOTYPE *) this);
        return NULL;
    }

#ifdef H264_DECODE_STATISTICS
    this->stat_file = fopen("/tmp/h264_stat.txt", "w");
#endif

    return (CODEC_PROTOTYPE *) this;
}

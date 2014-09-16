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
--  Description :  Implementation for JPEG codec.
--
------------------------------------------------------------------------------*/

#include "codec_jpeg.h"
#include "post_processor.h"
#include <jpegdecapi.h>
#include <ppapi.h>
#include <OSAL.h>
#include <assert.h>
#include <string.h> // memset

#define MCU_SIZE 16
#ifdef IS_G1_DECODER
#define MCU_MAX_COUNT (8100)
#else
#define MCU_MAX_COUNT (4096)
#endif

typedef enum JPEG_DEC_STATE
{
    JPEG_PARSE_HEADERS,
    JPEG_DECODE
} JPEG_DEC_STATE;

typedef struct CODEC_JPEG
{
    CODEC_PROTOTYPE base;
    OSAL_ALLOCATOR alloc;
    JpegDecInst instance;
    JpegDecImageInfo info;
    JpegDecInput input;
    JPEG_DEC_STATE state;
    OMX_U32 sliceWidth;
    OMX_U32 sliceHeight;
    OMX_U32 scanLineSize;
    OMX_U32 scanLinesLeft;

    PPConfig pp_config;
    PPInst pp_instance;
    PP_TRANSFORMS transforms;
    PP_STATE pp_state;

    struct buffer
    {
        OSAL_BUS_WIDTH bus_address;
        OMX_U8 *bus_data;
        OMX_U32 capacity;
        OMX_U32 size;
    } Y, CbCr;

    OMX_BOOL mjpeg;
    OMX_BOOL forcedSlice;
    OMX_U32 imageSize;
    OMX_BOOL ppInfoSet;

} CODEC_JPEG;

// ratio for calculating chroma offset for slice data
static OMX_U32 decoder_offset_ratio(OMX_U32 sampling)
{

    OMX_U32 ratio = 1;

    switch (sampling)
    {
    case JPEGDEC_YCbCr400:
        ratio = 1;
        break;
    case JPEGDEC_YCbCr420_SEMIPLANAR:
    case JPEGDEC_YCbCr411_SEMIPLANAR:
        ratio = 2;
        break;
    case JPEGDEC_YCbCr422_SEMIPLANAR:
        ratio = 1;
        break;
    case JPEGDEC_YCbCr444_SEMIPLANAR:
        ratio = 1;
        break;

    default:
        ratio = 1;

    }

    return ratio;

}

// destroy codec instance
static void decoder_destroy_jpeg(CODEC_PROTOTYPE * arg)
{
    CODEC_JPEG *this = (CODEC_JPEG *) arg;

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
            JpegDecRelease(this->instance);
            this->instance = 0;
        }
        if(this->Y.bus_address)
            OSAL_AllocatorFreeMem(&this->alloc, this->Y.capacity,
                                  this->Y.bus_data, this->Y.bus_address);
        if(this->CbCr.bus_address)
            OSAL_AllocatorFreeMem(&this->alloc, this->CbCr.capacity,
                                  this->CbCr.bus_data, this->CbCr.bus_address);

        OSAL_AllocatorDestroy(&this->alloc);
        OSAL_Free(this);
    }
}

// try to consume stream data
static CODEC_STATE decoder_decode_jpeg(CODEC_PROTOTYPE * arg,
                                       STREAM_BUFFER * buf, OMX_U32 * consumed,
                                       FRAME * frame)
{
    CODEC_JPEG *this = (CODEC_JPEG *) arg;

    assert(arg);
    assert(buf);
    assert(consumed);

    this->input.streamBuffer.pVirtualAddress = (u32 *) buf->bus_data;
    this->input.streamBuffer.busAddress = buf->bus_address;
    this->input.streamLength = buf->streamlen;
    this->input.bufferSize = 0; // note: use this if streamlen > 2097144
    this->input.decImageType = JPEGDEC_IMAGE;   // Only fullres supported

    if(this->state == JPEG_PARSE_HEADERS)
    {
        JpegDecRet ret =
            JpegDecGetImageInfo(this->instance, &this->input, &this->info);
        *consumed = 0;
        switch (ret)
        {
        case JPEGDEC_OK:
            if (this->pp_state != PP_PIPELINE && 
                (this->pp_config.ppOutImg.pixFormat != 
                 this->info.outputFormat))
            {
                PPResult res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_JPEG);
                if(res != PP_OK)
                {
                    return JPEGDEC_ERROR;
                }
                this->pp_state = PP_PIPELINE;
            }
            return CODEC_HAS_INFO;
        case JPEGDEC_PARAM_ERROR:
            return CODEC_ERROR_INVALID_ARGUMENT;
        case JPEGDEC_ERROR:
        case JPEGDEC_STRM_ERROR:
            return CODEC_ERROR_STREAM;
        case JPEGDEC_INVALID_INPUT_BUFFER_SIZE:
        case JPEGDEC_INVALID_STREAM_LENGTH:
        case JPEGDEC_INCREASE_INPUT_BUFFER:
            return CODEC_ERROR_INVALID_ARGUMENT;
        case JPEGDEC_UNSUPPORTED:
            return CODEC_ERROR_STREAM_NOT_SUPPORTED;
        default:
            assert(!"unhandled JpegDecRet");
            break;
        }
    }
    else
    {
        // decode
        JpegDecOutput output;

        memset(&output, 0, sizeof(JpegDecOutput));
        OMX_U32 sliceOffset = 0;

        OMX_U32 offset = this->sliceWidth;

        offset *= this->info.outputHeight;

        if(this->pp_state == PP_DISABLED)
        {
            // Offset created by the slices decoded so far
            sliceOffset =
                this->sliceWidth * (this->info.outputHeight -
                                    this->scanLinesLeft);
        }

        if(this->pp_state != PP_STANDALONE)
        {

            OMX_U32 ratio = decoder_offset_ratio(this->info.outputFormat);

            this->input.pictureBufferY.pVirtualAddress =
                (u32 *) frame->fb_bus_data + sliceOffset;
            this->input.pictureBufferY.busAddress =
                frame->fb_bus_address + sliceOffset;

            this->input.pictureBufferCbCr.pVirtualAddress =
                (u32 *) (frame->fb_bus_data + offset + sliceOffset / ratio);
            this->input.pictureBufferCbCr.busAddress =
                frame->fb_bus_address + offset + sliceOffset / ratio;
        }
        else
        {
            // Let the jpeg decoder allocate output buffer
            this->input.pictureBufferY.pVirtualAddress = (u32 *) NULL;
            this->input.pictureBufferY.busAddress = 0;

            this->input.pictureBufferCbCr.pVirtualAddress = (u32 *) NULL;
            this->input.pictureBufferCbCr.busAddress = 0;
        }

        if(this->pp_state == PP_PIPELINE)
        {
            HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
            PPResult res =
                HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
            if(res != PP_OK)
            {
                return CODEC_ERROR_INVALID_ARGUMENT;
            }
        }

        JpegDecRet ret = JpegDecDecode(this->instance, &this->input, &output);

        frame->size = 0;
        switch (ret)
        {
        case JPEGDEC_FRAME_READY:

            if(this->Y.bus_address)
            {
                assert(this->pp_state == PP_STANDALONE);
                OMX_U32 luma = this->info.outputWidth * this->scanLinesLeft;

                OMX_U32 chroma = 0;

                switch (this->info.outputFormat)
                {
                case JPEGDEC_YCbCr420_SEMIPLANAR:
                    chroma = (luma * 3 / 2) - luma;
                    break;
                case JPEGDEC_YCbCr422_SEMIPLANAR:
                    chroma = luma;
                    break;
                }
                
                memcpy(this->Y.bus_data + this->Y.size,
                       output.outputPictureY.pVirtualAddress, luma);
                this->Y.size += luma;
                memcpy(this->CbCr.bus_data + this->CbCr.size,
                       output.outputPictureCbCr.pVirtualAddress, chroma);
                this->CbCr.size += chroma;

                HantroHwDecOmx_pp_set_input_buffer_planes(&this->pp_config,
                                                          this->Y.bus_address,
                                                          this->CbCr.
                                                          bus_address);
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
                PPResult res =
                    HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                res = HantroHwDecOmx_pp_execute(this->pp_instance);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                OSAL_AllocatorFreeMem(&this->alloc, this->Y.capacity,
                                      this->Y.bus_data, this->Y.bus_address);
                if(this->CbCr.bus_data)
                    OSAL_AllocatorFreeMem(&this->alloc, this->CbCr.capacity,
                                          this->CbCr.bus_data,
                                          this->CbCr.bus_address);

                memset(&this->Y, 0, sizeof(this->Y));
                memset(&this->CbCr, 0, sizeof(this->CbCr));

                frame->size = HantroHwDecOmx_pp_get_framesize(&this->pp_config);
                this->scanLinesLeft -= this->scanLinesLeft;
            }
            else
            {
                if(this->pp_state == PP_STANDALONE)
                {
                    HantroHwDecOmx_pp_set_input_buffer(&this->pp_config,
                                                       output.outputPictureY.
                                                       busAddress);
                    HantroHwDecOmx_pp_set_output_buffer(&this->pp_config,
                                                        frame);
                    PPResult res = HantroHwDecOmx_pp_set(this->pp_instance,
                                                         &this->pp_config);

                    if(res != PP_OK)
                    {
                        return CODEC_ERROR_UNSPECIFIED;
                    }
                    res = HantroHwDecOmx_pp_execute(this->pp_instance);
                    if(res != PP_OK)
                    {
                        return CODEC_ERROR_UNSPECIFIED;
                    }
                }
                frame->size = this->scanLineSize * this->scanLinesLeft;
                this->scanLinesLeft -= this->scanLinesLeft;
                frame->size = HantroHwDecOmx_pp_get_framesize(&this->pp_config);
                this->scanLinesLeft -= this->scanLinesLeft;
            }
#ifdef ENABLE_CODEC_MJPEG
            if(this->input.motionJpeg)
            {
                if(this->pp_state == PP_DISABLED)
                    frame->size = this->imageSize;
                // next frame starts with headers
                this->state = JPEG_PARSE_HEADERS;
            }
#endif
            *consumed = buf->streamlen;
            return CODEC_NEED_MORE;

        case JPEGDEC_SLICE_READY:
            {
                if(!this->forcedSlice)
                    frame->size = this->scanLineSize * this->sliceHeight;
                this->scanLinesLeft -= this->sliceHeight;

                OMX_U32 luma = this->info.outputWidth * this->sliceHeight;

                OMX_U32 chroma = 0;

                switch (this->info.outputFormat)
                {
                case JPEGDEC_YCbCr420_SEMIPLANAR:
                    chroma = (luma * 3 / 2) - luma;
                    break;
                case JPEGDEC_YCbCr422_SEMIPLANAR:
                    chroma = luma;
                    break;
                }

                if(this->pp_state == PP_STANDALONE)
                {
                    if(this->Y.bus_address == 0)
                    {
                        this->Y.capacity =
                            this->info.outputWidth * this->info.outputHeight;
                        if(OSAL_AllocatorAllocMem
                           (&this->alloc, &this->Y.capacity, &this->Y.bus_data,
                            &this->Y.bus_address) != OMX_ErrorNone)
                            return CODEC_ERROR_UNSPECIFIED;
                    }
                    // append frame luma in the temp buffer
                    memcpy(this->Y.bus_data + this->Y.size,
                           output.outputPictureY.pVirtualAddress, luma);
                    this->Y.size += luma;

                    if(this->info.outputFormat != JPEGDEC_YCbCr400)
                    {
                        if(this->CbCr.bus_address == 0)
                        {
                            OMX_U32 lumasize =
                                this->info.outputWidth *
                                this->info.outputHeight;
                            OMX_U32 capacity = 0;

                            switch (this->info.outputFormat)
                            {
                            case JPEGDEC_YCbCr420_SEMIPLANAR:
                                capacity = lumasize * 3 / 2;
                                break;
                            case JPEGDEC_YCbCr422_SEMIPLANAR:
                                capacity = lumasize * 2;
                                break;
                            }
                            this->CbCr.capacity = capacity - lumasize;
                            if(OSAL_AllocatorAllocMem(&this->alloc,
                                                      &this->CbCr.capacity,
                                                      &this->CbCr.bus_data,
                                                      &this->CbCr.
                                                      bus_address) !=
                               OMX_ErrorNone)
                                return CODEC_ERROR_UNSPECIFIED;

                        }
                        // append frame CbCr in the temp buffer
                        memcpy(this->CbCr.bus_data + this->CbCr.size,
                               output.outputPictureCbCr.pVirtualAddress,
                               chroma);
                        this->CbCr.size += chroma;
                    }
                    // because the image is being assembled from slices and then fed to the post proc
                    // we tell the client that no frames were available.
                    frame->size = 0;
                }
            }
            return CODEC_NEED_MORE;

        case JPEGDEC_STRM_PROCESSED:
            return CODEC_NEED_MORE;
        case JPEGDEC_HW_RESERVED:
            return CODEC_ERROR_HW_TIMEOUT;
        case JPEGDEC_PARAM_ERROR:
        case JPEGDEC_INVALID_STREAM_LENGTH:
        case JPEGDEC_INVALID_INPUT_BUFFER_SIZE:
            return CODEC_ERROR_INVALID_ARGUMENT;
        case JPEGDEC_UNSUPPORTED:
            return CODEC_ERROR_STREAM_NOT_SUPPORTED;
        case JPEGDEC_DWL_HW_TIMEOUT:
            return CODEC_ERROR_HW_TIMEOUT;
        case JPEGDEC_SYSTEM_ERROR:
            return CODEC_ERROR_SYS;
        case JPEGDEC_HW_BUS_ERROR:
            return CODEC_ERROR_HW_BUS_ERROR;
        case JPEGDEC_ERROR:
        case JPEGDEC_STRM_ERROR:
            return CODEC_ERROR_STREAM;
        default:
            assert(!"unhandled JpegDecRet");
            break;
        }

    }
    return CODEC_ERROR_UNSPECIFIED;
}

// get stream info
static CODEC_STATE decoder_getinfo_jpeg(CODEC_PROTOTYPE * arg,
                                        STREAM_INFO * pkg)
{
    CODEC_JPEG *this = (CODEC_JPEG *) arg;

    assert(this != 0);
    assert(this->instance != 0);
    assert(pkg);

    this->sliceWidth = this->info.outputWidth;
    this->scanLinesLeft = this->info.outputHeight;
    // Calculate slice size
    if(this->info.outputWidth / MCU_SIZE * this->info.outputHeight / MCU_SIZE
       > MCU_MAX_COUNT)
    {
        this->input.sliceMbSet =
            MCU_MAX_COUNT / (this->info.outputWidth / MCU_SIZE);
        if(this->pp_state == PP_DISABLED || this->pp_state == PP_STANDALONE)
        {
            this->sliceHeight = this->input.sliceMbSet * MCU_SIZE;
            this->forcedSlice=OMX_TRUE;
        }
        else
        {
            this->sliceHeight = this->info.outputHeight;
        }
    }
    else
    {
        this->input.sliceMbSet = 0;
        this->sliceHeight = this->info.outputHeight;
    }

    switch (this->info.outputFormat)
    {
    case JPEGDEC_YCbCr400:
        this->input.sliceMbSet /= 2;
        this->sliceHeight = this->input.sliceMbSet * MCU_SIZE;
        pkg->format = OMX_COLOR_FormatL8;
        this->scanLineSize = this->info.outputWidth;
        break;
    case JPEGDEC_YCbCr420_SEMIPLANAR:
        pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;
        this->scanLineSize = (this->info.outputWidth * 3) / 2;
        break;
    case JPEGDEC_YCbCr422_SEMIPLANAR:
        pkg->format = OMX_COLOR_FormatYUV422PackedSemiPlanar;
        this->scanLineSize = this->info.outputWidth * 2;
        break;
    case JPEGDEC_YCbCr411_SEMIPLANAR:
        pkg->format = PP_PIX_FMT_YCBCR_4_1_1_SEMIPLANAR;
        this->scanLineSize = (this->info.outputWidth * 3) / 2;
        break;
    case JPEGDEC_YCbCr444_SEMIPLANAR:
        pkg->format = PP_PIX_FMT_YCBCR_4_4_4_SEMIPLANAR;
        this->scanLineSize = (this->info.outputWidth * 3) / 2;
        break;
    default:
        assert(!"unhandled JpegDecRet");
        break;
    }
    // calculate minimum frame size
    pkg->framesize = this->scanLineSize * this->sliceHeight;
    {
        OMX_U32 samplingFactor = 4;

        switch (this->info.outputFormat)
        {
        case JPEGDEC_YCbCr420_SEMIPLANAR:
        case JPEGDEC_YCbCr411_SEMIPLANAR:
            samplingFactor = 3;
            break;
        case JPEGDEC_YCbCr422_SEMIPLANAR:
            samplingFactor = 4;
            break;
        case JPEGDEC_YCbCr400:
            samplingFactor = 2;
            break;
        case JPEGDEC_YCbCr444_SEMIPLANAR:
            samplingFactor = 6;
            break;
        default:
            break;
        }

        if(this->pp_state == PP_DISABLED)
        {
            this->imageSize = pkg->imageSize = this->info.outputWidth *
                this->info.outputHeight * samplingFactor / 2;
        }
    }

    pkg->width = this->info.outputWidth;
    pkg->height = this->info.outputHeight;
    pkg->stride = this->info.outputWidth;
    pkg->sliceheight = this->sliceHeight;
    if(this->pp_state == PP_STANDALONE)
        pkg->sliceheight = this->info.outputHeight;
    this->state = JPEG_DECODE;

    if(this->pp_state != PP_DISABLED)
    {
        // prevent multiple updates when decoding mjpeg
        if(this->ppInfoSet == OMX_FALSE)
            HantroHwDecOmx_pp_set_info(&this->pp_config, pkg, this->transforms);

        this->ppInfoSet = OMX_TRUE;
        pkg->imageSize = HantroHwDecOmx_pp_get_framesize(&this->pp_config);
    }

    return CODEC_OK;
}

// get decoded frame
static CODEC_STATE decoder_getframe_jpeg(CODEC_PROTOTYPE * arg, FRAME * frame,
                                         OMX_BOOL eos)
{
    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_jpeg(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                  OMX_U32 * first, OMX_U32 * last)
{
#ifdef ENABLE_CODEC_MJPEG
    CODEC_JPEG *this = (CODEC_JPEG *) arg;
#ifdef HANTRO_TESTBENCH
    if (this->input.motionJpeg)
    {
        OMX_U32 i;
        *first = 0;
        *last = 0;

        for(i = 0; i < buf->streamlen; ++i)
        {
            if(0xFF == buf->bus_data[i])
            {
                if(((i + 1) < buf->streamlen) && 0xD9 == buf->bus_data[i + 1])
                {
                    *last = i+2; /* add 16 bits to get the new start code */
                    return 1;
                }
            }
        }
    }
    else
        return -1;
#else
    if (this->input.motionJpeg)
    {
        *first = 0;
        *last = buf->streamlen;
        return 1;
    }
    else
        return -1;
#endif /* HANTRO_TESTBENCH */
#endif /* ENABLE_CODEC_MJPEG */
    return -1;
}

static CODEC_STATE decoder_setppargs_jpeg(CODEC_PROTOTYPE * codec,
                                          PP_ARGS * args)
{
    CODEC_JPEG *this = (CODEC_JPEG *) codec;

    assert(this);
    assert(args);

    PPResult ret;

    ret = HantroHwDecOmx_pp_init(&this->pp_instance, &this->pp_config);

    if(ret == PP_OK)
    {
        this->transforms =
            HantroHwDecOmx_pp_set_output(&this->pp_config, args, OMX_FALSE);
        if(this->transforms == PPTR_ARG_ERROR)
            return CODEC_ERROR_INVALID_ARGUMENT;
#ifdef ENABLE_PP
        if(this->transforms == PPTR_NONE)
        {
            this->pp_state = PP_DISABLED;
        }
        else
        {
            if(this->transforms & PPTR_ROTATE || this->transforms & PPTR_CROP)
            {
                this->pp_state = PP_STANDALONE;
            }
            else
            {
                PPResult res;

                res =
                    HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                      this->instance,
                                                      PP_PIPELINED_DEC_TYPE_JPEG);
                if(res != PP_OK)
                {
                    return CODEC_ERROR_UNSPECIFIED;
                }
                this->pp_state = PP_PIPELINE;

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

// create codec instance and initialize it
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_jpeg(OMX_BOOL motion_jpeg)
{
    CODEC_JPEG *this = OSAL_Malloc(sizeof(CODEC_JPEG));

    memset(this, 0, sizeof(CODEC_JPEG));

    this->base.destroy = decoder_destroy_jpeg;
    this->base.decode = decoder_decode_jpeg;
    this->base.getinfo = decoder_getinfo_jpeg;
    this->base.getframe = decoder_getframe_jpeg;
    this->base.scanframe = decoder_scanframe_jpeg;
    this->base.setppargs = decoder_setppargs_jpeg;

    this->instance = 0;
    this->state = JPEG_PARSE_HEADERS;
    JpegDecRet ret = JpegDecInit(&this->instance);
#ifdef ENABLE_CODEC_MJPEG
    this->input.motionJpeg = motion_jpeg;
#endif
    this->forcedSlice = OMX_FALSE;
    this->ppInfoSet = OMX_FALSE;

    if(ret != JPEGDEC_OK)
    {
        OSAL_Free(this);
        return NULL;
    }
    if(OSAL_AllocatorInit(&this->alloc) != OMX_ErrorNone)
    {
        JpegDecRelease(this->instance);
        OSAL_Free(this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

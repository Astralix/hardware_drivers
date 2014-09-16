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
--  Description :  Implementation for RV codec
--
------------------------------------------------------------------------------*/

//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_rv"
#include <utils/Log.h>

#include "codec_rv.h"
#include "post_processor.h"
#include <rvdecapi.h>
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

typedef struct CODEC_RV
{
    CODEC_PROTOTYPE base;

    RvDecInst instance;
    PPConfig pp_config;
    PP_STATE pp_state;
    PP_TRANSFORMS transforms;
    PPInst pp_instance;
    OMX_U32 maxWidth;
    OMX_U32 maxHeight;
    OMX_U32 pctszSize;
    OMX_BOOL bIsRV8;
    OMX_U32 framesize;
    OMX_BOOL update_pp_out;
    OMX_U32 picId;
    OMX_U32 sliceInfoNum;
    OMX_U8 pSliceInfo;

} CODEC_RV;

// destroy codec instance
static void decoder_destroy_rv(CODEC_PROTOTYPE * arg)
{
    CALLSTACK;

    CODEC_RV *this = (CODEC_RV *) arg;

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
            RvDecRelease(this->instance);
            this->instance = 0;
        }

        OSAL_Free(this);
    }
}


// try to consume stream data
static CODEC_STATE decoder_decode_rv(CODEC_PROTOTYPE * arg,
                                      STREAM_BUFFER * buf, OMX_U32 * consumed,
                                      FRAME * frame)
{
    CALLSTACK;

    CODEC_RV *this = (CODEC_RV *) arg;

    assert(this);
    assert(buf);
    assert(consumed);

    RvDecInput input;
    RvDecOutput output;
    CODEC_STATE stat = CODEC_ERROR_UNSPECIFIED;

    memset(&input, 0, sizeof(RvDecInput));
    memset(&output, 0, sizeof(RvDecOutput));

    input.pStream = buf->bus_data;
    input.streamBusAddress = buf->bus_address;
    input.dataLen = buf->streamlen;
    frame->size = 0;
    input.picId = this->picId;
    input.skipNonReference = 0;

    input.sliceInfoNum = buf->sliceInfoNum;
    input.pSliceInfo = (RvDecSliceInfo*) buf->pSliceInfo;
    input.timestamp = 0;

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

    RvDecRet ret = RvDecDecode(this->instance, &input, &output);

    switch (ret)
    {
        case RVDEC_STRM_PROCESSED:
            stat = CODEC_NEED_MORE;
            break;
        case RVDEC_HDRS_RDY:
            if(this->pp_state == PP_PIPELINE)
            {
                HantroHwDecOmx_pp_set_output_buffer(&this->pp_config, frame);
            }

            stat = CODEC_HAS_INFO;
            break;
        case RVDEC_PIC_DECODED:
            this->picId++;
            stat = CODEC_HAS_FRAME;
            break;
        case RVDEC_PARAM_ERROR:
            stat = CODEC_ERROR_INVALID_ARGUMENT;
            break;
        case RVDEC_STRM_ERROR:
            stat = CODEC_ERROR_STREAM;
            break;
        case RVDEC_STREAM_NOT_SUPPORTED:
            stat = CODEC_ERROR_STREAM_NOT_SUPPORTED;
            break;
        case RVDEC_NOT_INITIALIZED:
            stat = CODEC_ERROR_NOT_INITIALIZED;
            break;
        case RVDEC_HW_TIMEOUT:
            stat = CODEC_ERROR_HW_TIMEOUT;
            break;
        case RVDEC_SYSTEM_ERROR:
            stat = CODEC_ERROR_SYS;
            break;
        case RVDEC_HW_RESERVED:
            stat = CODEC_ERROR_HW_RESERVED;
            break;
        case RVDEC_HW_BUS_ERROR:
            stat = CODEC_ERROR_HW_BUS_ERROR;
            break;
    case RVDEC_NONREF_PIC_SKIPPED:
            this->picId++;
            output.dataLeft = 0;
            stat = CODEC_NEED_MORE;
            break;
        default:
            assert(!"unhandled RvDecRet");
            break;
    }

    if(stat != CODEC_ERROR_UNSPECIFIED)
    {
            if(ret == RVDEC_HDRS_RDY)
               *consumed = 0;
#ifdef HANTRO_TESTBENCH
            else if(ret == RVDEC_PIC_DECODED && output.dataLeft == 1)
                *consumed = input.dataLen; // fix for raw RV stream
#endif
            else
                *consumed = input.dataLen - output.dataLeft;

    }

    return stat;
}

// get stream info
static CODEC_STATE decoder_getinfo_rv(CODEC_PROTOTYPE * arg, STREAM_INFO * pkg)
{
    CALLSTACK;

    CODEC_RV *this = (CODEC_RV *) arg;

    assert(this != 0);
    assert(pkg);

    RvDecInfo info;

    memset(&info, 0, sizeof(RvDecInfo));
    RvDecRet ret = RvDecGetInfo(this->instance, &info);
    if (ret != RVDEC_OK)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }

    pkg->width = ALIGN(info.frameWidth, 16)*16;
    pkg->height = ALIGN(info.frameHeight, 16)*16;
    pkg->stride = ALIGN(info.codedWidth, 16);
    pkg->sliceheight = ALIGN(info.codedHeight, 16);
    pkg->interlaced = 0;
    pkg->framesize = pkg->width * pkg->height * 3 / 2;

    pkg->format = OMX_COLOR_FormatYUV420PackedSemiPlanar;

    /* information about output size is now available */
    /* update output settings before decoding */
    this->update_pp_out = OMX_TRUE;
    HantroHwDecOmx_pp_set_info(&this->pp_config, pkg, this->transforms);

    if(HantroHwDecOmx_pp_set(this->pp_instance, &this->pp_config) != PP_OK)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }

    this->framesize = pkg->framesize;
    return CODEC_OK;
}

// get decoded frame
static CODEC_STATE decoder_getframe_rv(CODEC_PROTOTYPE * arg, FRAME * frame,
                                        OMX_BOOL eos)
{
    RvDecPicture picture;

    CODEC_RV *this = (CODEC_RV *) arg;

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
    CALLSTACK;

    memset(&picture, 0, sizeof(RvDecPicture));

    u32 endofStream = eos == OMX_TRUE ? 1 : 0;

    RvDecRet ret = RvDecNextPicture(this->instance, &picture, endofStream);

    TRACE(" end of stream %d\n", endofStream);
    this = (CODEC_RV *) arg;

    TRACE("err mbs %d \n", picture.numberOfErrMBs);

    if(ret == RVDEC_PIC_RDY)
    {
        assert(this->framesize);

        if(this->pp_state != PP_DISABLED)
        {
            this->update_pp_out = OMX_TRUE;
            PPResult res = HantroHwDecOmx_pp_execute(this->pp_instance);
            if(res != PP_OK)
            {
                LOGE("[%s] res != PP_OK", __func__);
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
    else if(ret == RVDEC_OK)
    {
        return CODEC_OK;
    }
    else if(ret == RVDEC_PARAM_ERROR)
    {
        return CODEC_ERROR_INVALID_ARGUMENT;
    }

    return CODEC_ERROR_UNSPECIFIED;
}

static int decoder_scanframe_rv(CODEC_PROTOTYPE * arg, STREAM_BUFFER * buf,
                                 OMX_U32 * first, OMX_U32 * last)
{
    CALLSTACK;

    CODEC_RV *this = (CODEC_RV *) arg;

    assert(this);

    *first = 0;
    *last = buf->streamlen;
    return 1;
}

static CODEC_STATE decoder_setppargs_rv(CODEC_PROTOTYPE * codec,
                                         PP_ARGS * args)
{
    CALLSTACK;

    CODEC_RV *this = (CODEC_RV *) codec;

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
            TRACE("PP_DISABLED\n");
            this->pp_state = PP_DISABLED;
        }
        else
        {
            TRACE("PP_COMBINED_MODE\n");
            PPResult res;

            res =
                HantroHwDecOmx_pp_pipeline_enable(this->pp_instance,
                                                  this->instance,
                                                  PP_PIPELINED_DEC_TYPE_RV);
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
static CODEC_STATE decoder_setscaling_rv(CODEC_PROTOTYPE *codec, OMX_U32 width, OMX_U32 height)
{
    CODEC_RV *this = (CODEC_RV *) codec;

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
CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_rv(OMX_BOOL bIsRV8, OMX_U32 pctszSize, OMX_U32 maxWidth, OMX_U32 maxHeight)
{
    CALLSTACK;

    CODEC_RV *this = OSAL_Malloc(sizeof(CODEC_RV));

    memset(this, 0, sizeof(CODEC_RV));

    u32 imagesizes[4];

    this->base.destroy = decoder_destroy_rv;
    this->base.decode = decoder_decode_rv;
    this->base.getinfo = decoder_getinfo_rv;
    this->base.getframe = decoder_getframe_rv;
    this->base.scanframe = decoder_scanframe_rv;
    this->base.setppargs = decoder_setppargs_rv;
#ifdef DYNAMIC_SCALING
	this->base.setscaling = decoder_setscaling_rv;
#endif
    this->instance = 0;
    this->picId = 0;

    /* Should we somehow support variable picture size? or create error earlier?
     * */
    imagesizes[0] =
    imagesizes[2] =  maxWidth;

    imagesizes[1] =
    imagesizes[3] = maxHeight;

#ifdef IS_G1_DECODER
    RvDecRet ret = RvDecInit(&this->instance,
                             USE_VIDEO_FREEZE_CONCEALMENT,
                             pctszSize, bIsRV8 == OMX_TRUE ?  imagesizes : NULL ,
                             bIsRV8 ? 0 : 1,
                             maxWidth, maxHeight, FRAME_BUFFERS, DEC_REF_FRM_RASTER_SCAN);
#else
    RvDecRet ret = RvDecInit(&this->instance,
                             USE_VIDEO_FREEZE_CONCEALMENT,
                             pctszSize, bIsRV8 == OMX_TRUE ?  imagesizes : NULL ,
                             bIsRV8 ? 0 : 1,
                             maxWidth, maxHeight, FRAME_BUFFERS);
#endif

    if(ret != RVDEC_OK)
    {
        decoder_destroy_rv((CODEC_PROTOTYPE *) this);
        return NULL;
    }
    return (CODEC_PROTOTYPE *) this;
}

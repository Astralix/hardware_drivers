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
--  Description :  Common post-processor code.
--
------------------------------------------------------------------------------*/
//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_post_processor"
#include <utils/Log.h>
#define TRACE_PRINT LOGV

#include "post_processor.h"
#include <assert.h>

#if 0
OK #define PP_PIX_FMT_YCBCR_4_0_0                          0x080000U

OK #define PP_PIX_FMT_YCBCR_4_2_2_INTERLEAVED              0x010001U
OK #define PP_PIX_FMT_YCRYCB_4_2_2_INTERLEAVED             0x010005U
OK #define PP_PIX_FMT_CBYCRY_4_2_2_INTERLEAVED             0x010006U
OK #define PP_PIX_FMT_CRYCBY_4_2_2_INTERLEAVED             0x010007U
OK #define PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR               0x010002U

NOK #define PP_PIX_FMT_YCBCR_4_4_0                          0x010004U

OK #define PP_PIX_FMT_YCBCR_4_2_0_PLANAR                   0x020000U
OK #define PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR               0x020001U
NOK #define PP_PIX_FMT_YCBCR_4_2_0_TILED                    0x020002U

OK #define PP_PIX_FMT_RGB16_CUSTOM                         0x040000U
OK #define PP_PIX_FMT_RGB16_5_5_5                          0x040001U
OK #define PP_PIX_FMT_RGB16_5_6_5                          0x040002U
NOK #define PP_PIX_FMT_BGR16_5_5_5                          0x040003U
OK #define PP_PIX_FMT_BGR16_5_6_5                          0x040004U

NOK #define PP_PIX_FMT_RGB32_CUSTOM                         0x041000U
OK #define PP_PIX_FMT_RGB32                                0x041001U
OK #define PP_PIX_FMT_BGR32                                0x041002U
#endif
static int omxformat_to_pp(OMX_COLOR_FORMATTYPE format)
{
    CALLSTACK;
    switch (format)
    {
    case OMX_COLOR_FormatL8:
        return PP_PIX_FMT_YCBCR_4_0_0;
    case OMX_COLOR_FormatYUV420PackedPlanar:
        return PP_PIX_FMT_YCBCR_4_2_0_PLANAR;
    case OMX_COLOR_FormatYUV420Planar:
        return PP_PIX_FMT_YCBCR_4_2_0_PLANAR;
    case OMX_COLOR_FormatYUV420SemiPlanar:
        return PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:
        return PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
    case OMX_COLOR_FormatYUV422PackedSemiPlanar:
        return PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR;

    case OMX_COLOR_FormatYCbYCr:
        return PP_PIX_FMT_YCBCR_4_2_2_INTERLEAVED;
    case OMX_COLOR_FormatYCrYCb:
        return PP_PIX_FMT_YCRYCB_4_2_2_INTERLEAVED;
    case OMX_COLOR_FormatCbYCrY:
        return PP_PIX_FMT_CBYCRY_4_2_2_INTERLEAVED;
    case OMX_COLOR_FormatCrYCbY:
        return PP_PIX_FMT_CRYCBY_4_2_2_INTERLEAVED;

    case OMX_COLOR_Format32bitARGB8888:
        return PP_PIX_FMT_RGB32;
    case OMX_COLOR_Format32bitBGRA8888:
        return PP_PIX_FMT_BGR32;
    case OMX_COLOR_Format16bitARGB1555:
        return PP_PIX_FMT_RGB16_5_5_5;
    case OMX_COLOR_Format16bitARGB4444:
        return PP_PIX_FMT_RGB16_CUSTOM;
    case OMX_COLOR_Format16bitRGB565:
        return PP_PIX_FMT_RGB16_5_6_5;
    case OMX_COLOR_Format16bitBGR565:
        return PP_PIX_FMT_BGR16_5_6_5;

// TODO Supported input formats when in combined mode
    case PP_PIX_FMT_YCBCR_4_1_1_SEMIPLANAR:
        return PP_PIX_FMT_YCBCR_4_1_1_SEMIPLANAR;
    case PP_PIX_FMT_YCBCR_4_4_4_SEMIPLANAR:
        return PP_PIX_FMT_YCBCR_4_4_4_SEMIPLANAR;
    default:
        assert(!"unknown OMX->PP format");
        return -1;
    }
    return 0;
}

PPResult HantroHwDecOmx_pp_init(PPInst * pp_instance, PPConfig * pp_config)
{
    CALLSTACK;
    PPResult ret = PP_OK;

    if(!*pp_instance)
    {
#ifdef ENABLE_PP
        ret = PPInit(pp_instance);
#endif
    }

    if(ret == PP_OK)
    {
#ifdef ENABLE_PP
        ret = PPGetConfig(*pp_instance, pp_config);
#endif
    }
    return ret;
}

void HantroHwDecOmx_pp_destroy(PPInst * pp_instance)
{
    CALLSTACK;
#ifdef ENABLE_PP
    PPRelease(*pp_instance);
#endif
    *pp_instance = 0;
}

PP_TRANSFORMS HantroHwDecOmx_pp_set_output(PPConfig * pp_config, PP_ARGS * args,
                                           OMX_BOOL bDeIntEna)
{
    CALLSTACK;

    PP_TRANSFORMS state = PPTR_NONE;

    TRACE_PRINT("HantroHwDecOmx_pp_set_output\n");

	TRACE_PRINT("args scale = %dx%d", args->scale.width, args->scale.height);
	TRACE_PRINT("args crop = %dx%d", args->crop.width, args->crop.height);
	TRACE_PRINT("args format = %d", args->format);
	TRACE_PRINT("args rotation = %d", args->rotation);

    // set input video cropping parameters
    if(args->crop.width > 0 && args->crop.height > 0 &&
       args->crop.top >= 0 && args->crop.left >= 0)
    {
        pp_config->ppInCrop.enable = 1;
        pp_config->ppInCrop.width = args->crop.width;
        pp_config->ppInCrop.height = args->crop.height;
        pp_config->ppInCrop.originX = args->crop.left;
        pp_config->ppInCrop.originY = args->crop.top;
        pp_config->ppOutImg.width = args->crop.width;
        pp_config->ppOutImg.height = args->crop.height;
        state |= PPTR_CROP;
        state |= PPTR_SCALE;
    }
    else
    {
        pp_config->ppInCrop.enable = 0;
    }

    if(args->mask1.height && args->mask1.width)
    {
        pp_config->ppOutMask1.originX = args->mask1.originX;
        pp_config->ppOutMask1.originY = args->mask1.originY;
        pp_config->ppOutMask1.height = args->mask1.height;
        pp_config->ppOutMask1.width = args->mask1.width;
        pp_config->ppOutMask1.blendComponentBase = args->blend_mask_base;
        pp_config->ppOutMask1.enable = 1;
        pp_config->ppOutMask1.alphaBlendEna = args->blend_mask_base != 0;
        state |= PPTR_MASK1;
    }
    else
    {
        pp_config->ppOutMask1.enable = 0;
    }
    if(args->mask2.height && args->mask2.width)
    {
        pp_config->ppOutMask2.originX = args->mask2.originX;
        pp_config->ppOutMask2.originY = args->mask2.originY;
        pp_config->ppOutMask2.width = args->mask2.width;
        pp_config->ppOutMask2.height = args->mask2.height;
        pp_config->ppOutMask2.enable = 1;
        state |= PPTR_MASK2;
    }
    else
    {
        pp_config->ppOutMask2.enable = 0;
    }

    // set output video resolution
    if(args->scale.width > 0 && args->scale.height > 0)
    {
        pp_config->ppOutImg.width = args->scale.width;
        pp_config->ppOutImg.height = args->scale.height;
        state |= PPTR_SCALE;
    }

    // set output color format
    if(args->format != OMX_COLOR_FormatUnused &&
       (args->format != OMX_COLOR_FormatYUV420SemiPlanar ||
		args->format != OMX_COLOR_FormatYUV420PackedSemiPlanar))
    {
        int format = omxformat_to_pp(args->format);

        if(format == -1)
            return PPTR_ARG_ERROR;
        else if (format == PP_PIX_FMT_RGB16_CUSTOM)
        {
            pp_config->ppOutRgb.rgbBitmask.maskR = 0x0F00;
            pp_config->ppOutRgb.rgbBitmask.maskG = 0x00F0;
            pp_config->ppOutRgb.rgbBitmask.maskB = 0x000F;
            pp_config->ppOutRgb.rgbBitmask.maskAlpha = 0xF000;
        }
        pp_config->ppOutImg.pixFormat = omxformat_to_pp(args->format);

        TRACE_PRINT("output format 0x%x\n",pp_config->ppOutImg.pixFormat);
#ifdef OMX_DECODER_VIDEO_DOMAIN
        state |= PPTR_FORMAT;
#endif

        if(format == PP_PIX_FMT_RGB32 ||
           format == PP_PIX_FMT_BGR32 ||
           format == PP_PIX_FMT_RGB16_5_5_5 ||
           format == PP_PIX_FMT_RGB16_5_6_5 ||
           format == PP_PIX_FMT_BGR16_5_6_5 || format == PP_PIX_FMT_RGB16_CUSTOM)
        {

            pp_config->ppInImg.videoRange = 0;
            pp_config->ppOutRgb.rgbTransform = PP_YCBCR2RGB_TRANSFORM_BT_601;
            pp_config->ppOutRgb.contrast = args->contrast;
            pp_config->ppOutRgb.brightness = args->brightness;
            pp_config->ppOutRgb.saturation = args->saturation;
            pp_config->ppOutRgb.alpha = args->alpha;
            pp_config->ppOutRgb.ditheringEnable = args->dither;
        }
    }
    else
    {
        pp_config->ppOutImg.pixFormat = PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR;
    }

    // set rotation
    if(args->rotation != ROTATE_NONE)
    {
        switch (args->rotation)
        {
        case ROTATE_LEFT_90:
            pp_config->ppInRotation.rotation = PP_ROTATION_LEFT_90;
            break;
        case ROTATE_RIGHT_90:
            pp_config->ppInRotation.rotation = PP_ROTATION_RIGHT_90;
            break;
        case ROTATE_180:
            pp_config->ppInRotation.rotation = PP_ROTATION_180;
            break;
        case ROTATE_FLIP_VERTICAL:
            pp_config->ppInRotation.rotation = PP_ROTATION_VER_FLIP;
            break;
        case ROTATE_FLIP_HORIZONTAL:
            pp_config->ppInRotation.rotation = PP_ROTATION_HOR_FLIP;
            break;
        default:
            assert(0);
            break;
        }
        state |= PPTR_ROTATE;
    }
    else
    {
        pp_config->ppInRotation.rotation = PP_ROTATION_NONE;
    }

    if(bDeIntEna == OMX_TRUE)
    {
        state |= PPTR_DEINTERLACE;
    }

    return state;
}

// setup input image params
void HantroHwDecOmx_pp_set_info(PPConfig * pp_config, STREAM_INFO * info,
                                PP_TRANSFORMS state)
{
    CALLSTACK;
    if(!(state & PPTR_CROP))
    {
        if(state & PPTR_ROTATE)
        {
            // perform automatic cropping of image down to next even multiple
            // of 16 pixels. This is because when frame has "padding" and it
            // is rotated the padding is also rotated. With cropping this problem can
            // be removed. The price of this is some lost image data.

            if(!info->isVc1Stream) // Don't align VC1 dimensions
            {
                info->width = (info->width / 16) * 16;
                info->height = (info->height / 16) * 16;
            }
            pp_config->ppInCrop.enable = 1;
            pp_config->ppInCrop.width = info->width;
            pp_config->ppInCrop.height = info->height;
        }

    }

    pp_config->ppInImg.width = info->width; //stride
    pp_config->ppInImg.height = info->height;   //sliceheight

    if(!(state & PPTR_SCALE))
    {
        pp_config->ppOutImg.width = info->width;
        pp_config->ppOutImg.height = info->height;
    }

    if(state & PPTR_ROTATE)
    {
        if(pp_config->ppInRotation.rotation == PP_ROTATION_LEFT_90 ||
           pp_config->ppInRotation.rotation == PP_ROTATION_RIGHT_90)
        {
            int temp = pp_config->ppOutImg.width;

            pp_config->ppOutImg.width = pp_config->ppOutImg.height;
            pp_config->ppOutImg.height = temp;
        }
    }

    info->width = pp_config->ppOutImg.width;
    info->height = pp_config->ppOutImg.height;
    info->sliceheight = pp_config->ppOutImg.height;
    info->stride = pp_config->ppOutImg.width;

    if(state & PPTR_DEINTERLACE)
    {
        pp_config->ppOutDeinterlace.enable = info->interlaced;
    }


    pp_config->ppInImg.pixFormat = omxformat_to_pp(info->format);
    info->framesize = HantroHwDecOmx_pp_get_framesize(pp_config);
}

// setup output image
void HantroHwDecOmx_pp_set_output_buffer(PPConfig * pp_config, FRAME * frame)
{
    CALLSTACK;

    TRACE_PRINT("HantroHwDecOmx_pp_set_output_buffer\n");
    pp_config->ppOutImg.bufferBusAddr = frame->fb_bus_address;
    pp_config->ppOutImg.bufferChromaBusAddr = frame->fb_bus_address +
        (pp_config->ppOutImg.width * pp_config->ppOutImg.height);
    TRACE_PRINT("  bufferBusAddr = 0x%08x\n",pp_config->ppOutImg.bufferBusAddr);
}

// setup input image (standalone mode only)
void HantroHwDecOmx_pp_set_input_buffer(PPConfig * pp_config, u32 bus_addr)
{
    CALLSTACK;
    pp_config->ppInImg.bufferBusAddr = bus_addr;
    pp_config->ppInImg.bufferCbBusAddr = bus_addr +
        (pp_config->ppInImg.width * pp_config->ppInImg.height);

    pp_config->ppInImg.bufferCrBusAddr = bus_addr +
        (pp_config->ppInImg.width * pp_config->ppInImg.height) +
        (pp_config->ppInImg.width / 2 * pp_config->ppInImg.height / 2);
}

void HantroHwDecOmx_pp_set_input_buffer_planes(PPConfig * pp_config,
                                               u32 bus_addr_y,
                                               u32 bus_addr_cbcr)
{
    CALLSTACK;
    pp_config->ppInImg.bufferBusAddr = bus_addr_y;
    pp_config->ppInImg.bufferCbBusAddr = bus_addr_cbcr;
}

// get result
PPResult HantroHwDecOmx_pp_execute(PPInst pp_instance)
{
    CALLSTACK;
    PPResult ret = PP_OK;
    TRACE_PRINT("HantroHwDecOmx_pp_execute\n");
#ifdef ENABLE_PP
    ret = PPGetResult(pp_instance);
#endif
    return ret;
}

// set configuration to pp
PPResult HantroHwDecOmx_pp_set(PPInst pp_instance, PPConfig * pp_config)
{
    CALLSTACK;
    PPResult ret = PP_OK;
    TRACE_PRINT("HantroHwDecOmx_pp_set\n");
#ifdef ENABLE_PP
//    TRACE_PRINT("output format %x\n",pp_config->ppOutImg.pixFormat);
//    pp_config->ppOutImg.pixFormat = PP_PIX_FMT_RGB16_5_6_5; /* TODO */
//    TRACE_PRINT("output format %d\n",pp_config->ppOutImg.pixFormat);
    ret = PPSetConfig(pp_instance, pp_config);
#endif
    return ret;
}

PPResult HantroHwDecOmx_pp_pipeline_enable(PPInst pp_instance,
                                           const void *codec_instance, u32 type)
{
    CALLSTACK;
    PPResult ret = PP_OK;
#ifdef ENABLE_PP
    ret = PPDecCombinedModeEnable(pp_instance, codec_instance, type);
#endif
    return ret;
}

void HantroHwDecOmx_pp_pipeline_disable(PPInst pp_instance,
                                        const void *codec_instance)
{
    CALLSTACK;
#ifdef ENABLE_PP
    PPDecCombinedModeDisable(pp_instance, codec_instance);
#endif
}

int HantroHwDecOmx_pp_get_framesize(const PPConfig * pp_config)
{
    CALLSTACK;
    assert(pp_config);
    int framesize = pp_config->ppOutImg.width * pp_config->ppOutImg.height;

    switch (pp_config->ppOutImg.pixFormat)
    {
    case PP_PIX_FMT_YCBCR_4_0_0:
        break;
    case PP_PIX_FMT_YCBCR_4_2_0_PLANAR:
    case PP_PIX_FMT_YCBCR_4_2_0_SEMIPLANAR:
        framesize = framesize * 3 / 2;
        break;
    case PP_PIX_FMT_YCBCR_4_2_2_SEMIPLANAR:
    case PP_PIX_FMT_YCBCR_4_2_2_INTERLEAVED:
    case PP_PIX_FMT_YCRYCB_4_2_2_INTERLEAVED:
    case PP_PIX_FMT_CBYCRY_4_2_2_INTERLEAVED:
    case PP_PIX_FMT_CRYCBY_4_2_2_INTERLEAVED:
        framesize *= 2;
        break;
    case PP_PIX_FMT_RGB32:
    case PP_PIX_FMT_BGR32:
        framesize *= 4;
        break;
    case PP_PIX_FMT_RGB16_5_5_5:
    case PP_PIX_FMT_RGB16_5_6_5:
    case PP_PIX_FMT_BGR16_5_6_5:
    case PP_PIX_FMT_RGB16_CUSTOM:
        framesize *= 2;
        break;
    default:
        assert(!"pp_config->ppOutImg.pixFormat");
        return -1;
    }
    return framesize;
}

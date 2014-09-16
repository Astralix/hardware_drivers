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
--  Description :  Post-processor definitions.
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_POST_PROCESSOR_H
#define HANTRO_POST_PROCESSOR_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"
#include <ppapi.h>

    typedef enum PP_TRANSFORMS
    {
        PPTR_NONE = 0,
        PPTR_SCALE = 1,
        PPTR_CROP = 2,
        PPTR_FORMAT = 4,
        PPTR_ROTATE = 8,
        PPTR_MIRROR = 16,
        PPTR_MASK1 = 32,
        PPTR_MASK2 = 64,
        PPTR_DEINTERLACE = 128,
        PPTR_ARG_ERROR = 256
    } PP_TRANSFORMS;

// create pp instance & get default config
    PPResult HantroHwDecOmx_pp_init(PPInst * pp_instance, PPConfig * pp_config);

// destroy pp instance
    void HantroHwDecOmx_pp_destroy(PPInst * pp_instance);

// setup necessary transformations
    PP_TRANSFORMS HantroHwDecOmx_pp_set_output(PPConfig * pp_config,
                                               PP_ARGS * args,
                                               OMX_BOOL bDeIntEna);

// setup input image params
    void HantroHwDecOmx_pp_set_info(PPConfig * pp_config, STREAM_INFO * info,
                                    PP_TRANSFORMS state);

// setup input image (standalone mode only)
    void HantroHwDecOmx_pp_set_input_buffer(PPConfig * pp_config, u32 bus_addr);

    void HantroHwDecOmx_pp_set_input_buffer_planes(PPConfig * pp_config,
                                                   u32 bus_addr_y,
                                                   u32 bus_addr_cbcr);

// setup output image
    void HantroHwDecOmx_pp_set_output_buffer(PPConfig * pp_config,
                                             FRAME * frame);

// set configuration to pp
    PPResult HantroHwDecOmx_pp_set(PPInst pp_instance, PPConfig * pp_config);

// get result (standalone mode only)
    PPResult HantroHwDecOmx_pp_execute(PPInst pp_instance);

// enable pipeline mode
    PPResult HantroHwDecOmx_pp_pipeline_enable(PPInst pp_instance,
                                               const void *codec_instance,
                                               u32 type);

// disable pipeline mode
    void HantroHwDecOmx_pp_pipeline_disable(PPInst pp_instance,
                                            const void *codec_instance);

    int HantroHwDecOmx_pp_get_framesize(const PPConfig * pp_config);

    typedef enum PP_STATE
    {
        PP_DISABLED,
        PP_PIPELINE,
        PP_STANDALONE
    } PP_STATE;

#ifdef __cplusplus
}
#endif
#endif                       // HANTRO_POST_PROCESSOR_H

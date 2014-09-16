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
--  Description :  MPEG4 encoder header
--
------------------------------------------------------------------------------*/

#ifndef ENCODER_MPEG4_H_
#define ENCODER_MPEG4_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"
#include "OMX_Video.h"

typedef struct MPEG4_CONFIG
{
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE error_ctrl_config;

    // stream header info
    OMX_U32 nVideoRange; 
    
    ENCODER_COMMON_CONFIG       common_config;
    OMX_VIDEO_PARAM_MPEG4TYPE   mp4_config;
    RATE_CONTROL_CONFIG         rate_config;
    PRE_PROCESSOR_CONFIG        pp_config;
} MPEG4_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_mpeg4(const MPEG4_CONFIG* config);
// change encoding frame rate
CODEC_STATE HantroHwEncOmx_encoder_frame_rate_mpeg4(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate);
#ifdef __cplusplus
}
#endif
#endif /*CODEC_MPEG4_H_*/

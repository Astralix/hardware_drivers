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
--  Description : H263 encoder header
--
------------------------------------------------------------------------------*/

#ifndef ENCODER_H263_H_
#define ENCODER_H263_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"
#include "OMX_Video.h"

typedef struct H263_CONFIG
{
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE error_ctrl_config;

    OMX_U32 nVideoRange;
    
    ENCODER_COMMON_CONFIG       common_config;
    OMX_VIDEO_PARAM_H263TYPE    h263_config;
    RATE_CONTROL_CONFIG         rate_config;
    PRE_PROCESSOR_CONFIG        pp_config;
} H263_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_h263(const H263_CONFIG* config);
// change encoding frame rate
CODEC_STATE HantroHwEncOmx_encoder_frame_rate_h263(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate);
#ifdef __cplusplus
}
#endif
#endif /*CODEC_H263_H_*/

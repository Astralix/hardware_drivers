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
--  Description :  H264 encoder header
--
------------------------------------------------------------------------------*/


#ifndef ENCODER_H264_H
#define ENCODER_H264_H

#ifdef __cplusplus
extern  "C" {
#endif

#include "codec.h"
#include "OMX_Video.h"

typedef struct H264_CONFIG
{
    // ----------config-----------
    OMX_VIDEO_PARAM_AVCTYPE h264_config;
    
    // ---------codingctrl-----------
    // sliceSize * 16
    // Only slice height can be changed once stream has been started.
    OMX_U32 nSliceHeight;
    
    // I frame period.
    OMX_U32 nPFrames;
    
    // seiMessages
    OMX_BOOL bSeiMessages;

    // videoFullRange
    OMX_U32 nVideoFullRange;

    // constrainedIntraPrediction
    OMX_BOOL bConstrainedIntraPrediction;
    
    // disableDeblockingFilter
    OMX_BOOL bDisableDeblocking;
    
    ENCODER_COMMON_CONFIG common_config;
    RATE_CONTROL_CONFIG rate_config;
    PRE_PROCESSOR_CONFIG pp_config;
} H264_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_h264(const H264_CONFIG* config);
// change intra frame period
CODEC_STATE HantroHwEncOmx_encoder_intra_period_h264(ENCODER_PROTOTYPE* arg, OMX_U32 nPFrames);
// change encoding frame rate
CODEC_STATE HantroHwEncOmx_encoder_frame_rate_h264(ENCODER_PROTOTYPE* arg, OMX_U32 xFramerate);
#ifdef __cplusplus
}
#endif
#endif // ENCODER_H264_H



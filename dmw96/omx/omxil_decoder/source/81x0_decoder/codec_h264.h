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
--  Description :  
--
------------------------------------------------------------------------------*/

#ifndef CODEC_H264_H
#define CODEC_H264_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"

// create codec instance
    CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_h264(OMX_BOOL
                                                        conceal_errors);

#ifdef __cplusplus
}
#endif
#endif                       // CODEC_H264_H

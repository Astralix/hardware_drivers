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

#ifndef CODEC_PP_H
#define CODEC_PP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "codec.h"
#include <OMX_Types.h>
#include <OMX_IVCommon.h>

    CODEC_PROTOTYPE *HantroHwDecOmx_decoder_create_pp(OMX_U32 input_width,
                                                      OMX_U32 input_height,
                                                      OMX_COLOR_FORMATTYPE
                                                      input_format);

#ifdef __cplusplus
}
#endif
#endif

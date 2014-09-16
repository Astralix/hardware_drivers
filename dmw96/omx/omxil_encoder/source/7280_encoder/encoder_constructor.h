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
--  Description :  Encoder constructor prototypes.
--
------------------------------------------------------------------------------*/


#ifndef ENCODER_CONSTRUCTOR_H_
#define ENCODER_CONSTRUCTOR_H_


/******************************************************
  1. Includes 
******************************************************/
#include <OMX_Types.h>
#include <OMX_Component.h>

/******************************************************
 4. Function prototypes 
******************************************************/

/** 
 * This function takes care of constructing the instance of the component.
 * It is executed upon a getHandle() call.
 */
#ifdef OMX_ENCODER_VIDEO_DOMAIN
OMX_ERRORTYPE HantroHwEncOmx_hantro_encoder_video_constructor(OMX_COMPONENTTYPE *openmaxStandComp,
        OMX_STRING cComponentName);
#endif
#ifdef OMX_ENCODER_IMAGE_DOMAIN
OMX_ERRORTYPE HantroHwEncOmx_hantro_encoder_image_constructor(OMX_COMPONENTTYPE *openmaxStandComp,
        OMX_STRING cComponentName);
#endif




#endif //6280ENCODER_CONSTRUCTOR_H_


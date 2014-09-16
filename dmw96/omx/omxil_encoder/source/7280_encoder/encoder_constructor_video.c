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
--  Description :  OMX Encoder component constructor.
--
------------------------------------------------------------------------------*/


#include <OMX_Core.h>
#include "encoder_version.h"
#include "encoder_constructor.h"
#include "encoder.h"

/** 
 * Component construction function, executed whe OMX_GetHandle()
 * is called.
*/
OMX_ERRORTYPE HantroHwEncOmx_hantro_encoder_video_constructor(OMX_COMPONENTTYPE *openmaxStandComp,
        OMX_STRING cComponentName)
{
    OMX_ERRORTYPE err;

    if (strcmp(cComponentName, VIDEO_COMPONENT_NAME))
        return OMX_ErrorInvalidComponentName;
        
    openmaxStandComp->nSize = sizeof(OMX_COMPONENTTYPE);
    openmaxStandComp->pComponentPrivate = NULL;
    openmaxStandComp->pApplicationPrivate = NULL;
    openmaxStandComp->nVersion.s.nVersionMajor = COMPONENT_VERSION_MAJOR;
    openmaxStandComp->nVersion.s.nVersionMinor = COMPONENT_VERSION_MINOR;
    openmaxStandComp->nVersion.s.nRevision = COMPONENT_VERSION_REVISION;
    openmaxStandComp->nVersion.s.nStep = COMPONENT_VERSION_STEP;
         
    err = HantroHwEncOmx_encoder_init( openmaxStandComp );

    return err;
}


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
--  Description : Entry point function for Bellagio OpenMAX IL Core component loader
--
------------------------------------------------------------------------------*/


// Include the interface definition for components
// that are to be loaded by the Bellagio static component loader
#include <st_static_component_loader.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "encoder_version.h"
#include "encoder_constructor.h"

int omx_component_library_Setup(stLoaderComponentType** stComponents)
{
    if ( stComponents == NULL )
        return 1; //Number of components

#ifdef OMX_ENCODER_VIDEO_DOMAIN
#ifdef OMX_ENCODER_IMAGE_DOMAIN
#error Both VIDEO and IMAGE domain defined! Only one is allowed!
#endif
#endif

    /** Component 1 - 7280 Video / Image Encoder */
    stComponents[0]->componentVersion.s.nVersionMajor = COMPONENT_VERSION_MAJOR;
    stComponents[0]->componentVersion.s.nVersionMinor = COMPONENT_VERSION_MINOR;
    stComponents[0]->componentVersion.s.nRevision =     COMPONENT_VERSION_REVISION;
    stComponents[0]->componentVersion.s.nStep =         COMPONENT_VERSION_STEP;

int roles = 1;
#ifdef ENCH1
roles++;
#endif
const int ROLES = roles;

#ifdef OMX_ENCODER_VIDEO_DOMAIN
    stComponents[0]->constructor = HantroHwEncOmx_hantro_encoder_video_constructor;
    //Component name
    stComponents[0]->name = (char*) calloc( 1, OMX_MAX_STRINGNAME_SIZE );
    if ( stComponents[0]->name == NULL )
        return OMX_ErrorInsufficientResources;
    strncpy( stComponents[0]->name, VIDEO_COMPONENT_NAME, OMX_MAX_STRINGNAME_SIZE-1 );
#endif // OMX_ENCODER_VIDEO_DOMAIN

#ifdef OMX_ENCODER_IMAGE_DOMAIN
    stComponents[0]->constructor = HantroHwEncOmx_hantro_encoder_image_constructor;
    //Component name
    stComponents[0]->name = (char*) calloc( 1, OMX_MAX_STRINGNAME_SIZE );
    if ( stComponents[0]->name == NULL )
        return OMX_ErrorInsufficientResources;
    strncpy( stComponents[0]->name, IMAGE_COMPONENT_NAME, OMX_MAX_STRINGNAME_SIZE-1 );
#endif // OMX_ENCODER_IMAGE_DOMAIN

    //Set amount of component roles and allocate list of role names and roles
    stComponents[0]->name_specific_length = ROLES;
    stComponents[0]->name_specific = (char **) calloc(1, ROLES * sizeof(char *));
    stComponents[0]->role_specific = (char **) calloc(1, ROLES *sizeof(char *));
    if(!stComponents[0]->name_specific || !stComponents[0]->role_specific)
    {
        return OMX_ErrorInsufficientResources;
    }

        // allocate the array members
    int i;

    for(i = 0; i < ROLES; ++i)
    {
        stComponents[0]->name_specific[i] =
            (char *) calloc(1, OMX_MAX_STRINGNAME_SIZE);
        stComponents[0]->role_specific[i] =
            (char *) calloc(1, OMX_MAX_STRINGNAME_SIZE);
        if(stComponents[0]->role_specific[i] == 0 ||
           stComponents[0]->name_specific[i] == 0)
            return OMX_ErrorInsufficientResources;
    }
    int j = 0;
#ifdef OMX_ENCODER_VIDEO_DOMAIN
    strncpy(stComponents[0]->name_specific[j], COMPONENT_NAME_H264,
            OMX_MAX_STRINGNAME_SIZE - 1);
    strncpy(stComponents[0]->role_specific[j], COMPONENT_ROLE_H264,
            OMX_MAX_STRINGNAME_SIZE - 1);
    j++;
#ifdef ENCH1
    strncpy(stComponents[0]->name_specific[j], COMPONENT_NAME_VP8,
            OMX_MAX_STRINGNAME_SIZE - 1);
    strncpy(stComponents[0]->role_specific[j], COMPONENT_ROLE_VP8,
            OMX_MAX_STRINGNAME_SIZE - 1);
#endif
#endif // OMX_ENCODER_VIDEO_DOMAIN

#ifdef OMX_ENCODER_IMAGE_DOMAIN
    j = 0;
    strncpy(stComponents[0]->name_specific[j], COMPONENT_NAME_JPEG,
            OMX_MAX_STRINGNAME_SIZE - 1);
    strncpy(stComponents[0]->role_specific[j], COMPONENT_ROLE_JPEG,
            OMX_MAX_STRINGNAME_SIZE - 1);
    j++;
#ifdef ENCH1
    strncpy(stComponents[0]->name_specific[j], COMPONENT_NAME_WEBP,
            OMX_MAX_STRINGNAME_SIZE - 1);
    strncpy(stComponents[0]->role_specific[j], COMPONENT_ROLE_WEBP,
            OMX_MAX_STRINGNAME_SIZE - 1);
#endif
#endif // OMX_ENCODER_IMAGE_DOMAIN

    return 1;
}





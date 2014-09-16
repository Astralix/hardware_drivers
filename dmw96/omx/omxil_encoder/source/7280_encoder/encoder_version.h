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
--  Description :  Encoder version information. 
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_ENCODER_VERSION_H
#define HANTRO_ENCODER_VERSION_H

#define COMPONENT_VERSION_MAJOR    1
#define COMPONENT_VERSION_MINOR    1
#define COMPONENT_VERSION_REVISION 1
#define COMPONENT_VERSION_STEP     0


#define VIDEO_COMPONENT_NAME "OMX.hantro.7280.video.encoder"
#define IMAGE_COMPONENT_NAME "OMX.hantro.7280.image.encoder"

/* Roles currently disabled. Standard roles requires support for YUV420Planar 
 * format which is not implemeted. */
#define VIDEO_COMPONENT_ROLES 0
#define IMAGE_COMPONENT_ROLES 0


//VIDEO DOMAIN ROLES
#ifdef OMX_ENCODER_VIDEO_DOMAIN
/** Role 1 - MPEG4 Encoder */
#define COMPONENT_NAME_MPEG4 "OMX.hantro.7280.video.encoder.mpeg4"
#define COMPONENT_ROLE_MPEG4 "video_encoder.mpeg4"
/** Role 2 - H264 Encoder */
#define COMPONENT_NAME_H264  "OMX.hantro.7280.video.encoder.avc"
#define COMPONENT_ROLE_H264  "video_encoder.avc"
/** Role 3 - H263 Encoder */
#define COMPONENT_NAME_H263  "OMX.hantro.7280.video.encoder.h263"
#define COMPONENT_ROLE_H263  "video_encoder.h263"
#endif //~OMX_ENCODER_VIDEO_DOMAIN

//IMAGE DOMAIN ROLES
#ifdef OMX_ENCODER_IMAGE_DOMAIN
/** Role 3 - JPEG Encoder */
#define COMPONENT_NAME_JPEG  "OMX.hantro.7280.image.encoder.jpeg"
#define COMPONENT_ROLE_JPEG  "image_encoder.jpeg"
#endif //OMX_ENCODER_IMAGE_DOMAIN


#endif // HANTRO_ENCODER_VERSION_H


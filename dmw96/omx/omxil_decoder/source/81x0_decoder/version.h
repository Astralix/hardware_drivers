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
--  Description :  Library version information.
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_DECODER_VERSION_H
#define HANTRO_DECODER_VERSION_H

#define COMPONENT_VERSION_MAJOR    1
#define COMPONENT_VERSION_MINOR    1
#define COMPONENT_VERSION_REVISION 2
#define COMPONENT_VERSION_STEP     0

#define COMPONENT_NAME_VIDEO      "OMX.hantro.G1.video.decoder"
#define COMPONENT_NAME_IMAGE      "OMX.hantro.G1.image.decoder"

#ifdef OMX_DECODER_VIDEO_DOMAIN
/** Role 1 - MPEG4 Decoder */
#ifdef ENABLE_CODEC_MPEG4
#define COMPONENT_NAME_MPEG4 "OMX.hantro.G1.video.decoder.mpeg4"
#define COMPONENT_ROLE_MPEG4 "video_decoder.mpeg4"
#endif /* ENABLE_CODEC_MPEG4 */

/** Role 2 - H264 Decoder */
#ifdef ENABLE_CODEC_H264
#define COMPONENT_NAME_H264  "OMX.hantro.G1.video.decoder.avc"
#define COMPONENT_ROLE_H264  "video_decoder.avc"
#endif /* ENABLE_CODEC_H264 */

/** Role 3 - H263 Decoder */
#ifdef ENABLE_CODEC_H263
#define COMPONENT_NAME_H263  "OMX.hantro.G1.video.decoder.h263"
#define COMPONENT_ROLE_H263  "video_decoder.h263"
#endif /* ENABLE_CODEC_H263 */

/** Role 4 - VC1 Decoder */
#ifdef ENABLE_CODEC_VC1
#define COMPONENT_NAME_VC1   "OMX.hantro.G1.video.decoder.wmv"
#define COMPONENT_ROLE_VC1   "video_decoder.wmv"
#endif /* ENABLE_CODEC_VC1 */

/** Role 5 - MPEG2 Decoder */
#ifdef ENABLE_CODEC_MPEG2
#define COMPONENT_NAME_MPEG2 "OMX.hantro.G1.video.decoder.mpeg2"
#define COMPONENT_ROLE_MPEG2 "video_decoder.mpeg2"
#endif /* ENABLE_CODEC_MPEG2 */

/** Role 6 - AVS Decoder */
#ifdef ENABLE_CODEC_AVS
#define COMPONENT_NAME_AVS  "OMX.hantro.G1.video.decoder.avs"
#define COMPONENT_ROLE_AVS  "video_decoder.avs"
#endif /* ENABLE_CODEC_AVS */

/** Role 7 - VP8 Decoder */
#ifdef ENABLE_CODEC_VP8
#define COMPONENT_NAME_VP8  "OMX.hantro.G1.video.decoder.vp8"
#define COMPONENT_ROLE_VP8  "video_decoder.vp8"
#endif /* ENABLE_CODEC_VP8 */

/** Role 8 - VP6 Decoder */
#ifdef ENABLE_CODEC_VP6
#define COMPONENT_NAME_VP6 "OMX.hantro.G1.video.decoder.vp6"
#define COMPONENT_ROLE_VP6 "video_decoder.vp6"
#endif /* ENABLE_CODEC_VP6 */

/** Role 9 - MJPEG Decoder */
#ifdef ENABLE_CODEC_MJPEG
#define COMPONENT_NAME_MJPEG  "OMX.hantro.G1.video.decoder.jpeg"
#define COMPONENT_ROLE_MJPEG  "video_decoder.jpeg"
#endif /* ENABLE_CODEC_MJPEG */

/** Role 10 - RV Decoder */
#ifdef ENABLE_CODEC_RV
#define COMPONENT_NAME_RV "OMX.hantro.G1.video.decoder.rv"
#define COMPONENT_ROLE_RV "video_decoder.rv"
#endif /* ENABLE_CODEC_RV */

#endif /* OMX_DECODER_VIDEO_DOMAIN */

#ifdef OMX_DECODER_IMAGE_DOMAIN
/** Role 1 - JPEG Decoder */
#ifdef ENABLE_CODEC_JPEG
#define COMPONENT_NAME_JPEG  "OMX.hantro.G1.image.decoder.jpeg"
#define COMPONENT_ROLE_JPEG  "image_decoder.jpeg"
#endif /* ENABLE_CODEC_JPEG */

/** Role 2 - WEBP Decoder */
#ifdef ENABLE_CODEC_WEBP
#define COMPONENT_NAME_WEBP  "OMX.hantro.G1.image.decoder.webp"
#define COMPONENT_ROLE_WEBP  "image_decoder.webp"
#endif /* ENABLE_CODEC_WEBP */

#endif //OMX_DECODER_IMAGE_DOMAIN

#endif // HANTRO_DECODER_VERSION_H

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
--  Description :  JPEG encoder header
--
------------------------------------------------------------------------------*/

#ifndef ENCODER_JPEG_H_
#define ENCODER_JPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "codec.h"
#include "OMX_Video.h"
#include "jpegencapi.h"
    
typedef struct JPEG_CONFIG {
    // ----------config-----------

    OMX_U32 qLevel;     // The quantization level. [0..9]
    OMX_U32 sliceHeight;    

    // ---------- APP0 header related config -----------
    OMX_BOOL bAddHeaders;
    JpegEncAppUnitsType unitsType;      // Specifies unit type of xDensity and yDensity in APP0 header 
    JpegEncTableMarkerType markerType;  // Specifies the type of the DQT and DHT headers.
    JpegEncCodingType codingType;   
    OMX_U32 xDensity;                   // Horizontal pixel density to APP0 header.
    OMX_U32 yDensity;                   // Vertical pixel density to APP0 header.
    
    //OMX_U32 comLength;                // length of comment header data
    //OMX_U8    *pCom;                  // Pointer to comment header data of size comLength.
    //OMX_U32 thumbnail;                // Indicates if thumbnail is added to stream.
    
    // ---------- encoder options -----------
    OMX_U32 codingWidth;
    OMX_U32 codingHeight;
    OMX_U32 yuvFormat; // output picture YUV format
    PRE_PROCESSOR_CONFIG pp_config;
    

} JPEG_CONFIG;

// create codec instance
ENCODER_PROTOTYPE* HantroHwEncOmx_encoder_create_jpeg(const JPEG_CONFIG* config);

#ifdef __cplusplus
}
#endif

#endif /*CODEC_JPEG_H_*/

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
--  Description :  Internal video codec interface.
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_ENCODER_H
#define HANTRO_ENCODER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OSAL.h>

#define TIME_RESOLUTION         90000 // time resolution for h264 & vp8
#define TIME_RESOLUTION_MPEG4   1000

// hantro encoder interface

/**
 * Structure for encoded stream
 */
typedef struct STREAM_BUFFER
{
    OMX_U8* bus_data;           // set by common
    OSAL_BUS_WIDTH bus_address; // set by common
    OMX_U32 buf_max_size;       // set by common
    OMX_U32 streamlen;          // set by codec

    OMX_U32 *pOutBuf[9];        // vp8 partition pointers
    OMX_U32 streamSize[9];      // vp8 partition sizes
} STREAM_BUFFER;

typedef enum FRAME_TYPE
{
    INTRA_FRAME,
    PREDICTED_FRAME
} FRAME_TYPE;

/**
 * Structure for raw input frame
 */
typedef struct FRAME
{
    OMX_U8* fb_bus_data;    // set by common
    OMX_U32 fb_bus_address; // set by common
    OMX_U32 fb_frameSize;       // set by common
    OMX_U32 fb_bufferSize;  //set by common
    FRAME_TYPE frame_type;  // set by common

    OMX_U32 bitrate;
#ifdef ENCH1
    // flags for vp8 reference frame mode
    OMX_BOOL bPrevRefresh;
    OMX_BOOL bGoldenRefresh;
    OMX_BOOL bAltRefresh;
    OMX_BOOL bUsePrev;
    OMX_BOOL bUseGolden;
    OMX_BOOL bUseAlt;
#endif
} FRAME;

typedef enum CODEC_STATE
{
    CODEC_OK,
    CODEC_CODED_INTRA,
    CODEC_CODED_PREDICTED,
    CODEC_CODED_SLICE,
    CODEC_ERROR_HW_TIMEOUT = -1,
    CODEC_ERROR_HW_BUS_ERROR = -2,
    CODEC_ERROR_HW_RESET = -3,
    CODEC_ERROR_SYSTEM = -4,
    CODEC_ERROR_UNSPECIFIED = -5,
    CODEC_ERROR_RESERVED = -6,
    CODEC_ERROR_INVALID_ARGUMENT = -7,
    CODEC_ERROR_BUFFER_OVERFLOW = -8,
    CODEC_ERROR_INVALID_STATE = -9
} CODEC_STATE;

typedef struct ENCODER_PROTOTYPE ENCODER_PROTOTYPE;

// internal ENCODER interface, which wraps up Hantro API
struct ENCODER_PROTOTYPE
{
    void (*destroy)( ENCODER_PROTOTYPE*);
    CODEC_STATE (*stream_start)( ENCODER_PROTOTYPE*, STREAM_BUFFER*);
    CODEC_STATE (*stream_end)( ENCODER_PROTOTYPE*, STREAM_BUFFER*);
    CODEC_STATE (*encode)( ENCODER_PROTOTYPE*, FRAME*, STREAM_BUFFER*);
};

// common configuration structure for encoder pre-processor
typedef struct PRE_PROCESSOR_CONFIG
{   
    OMX_U32 origWidth;
    OMX_U32 origHeight;
    
    // picture cropping
    OMX_U32 xOffset;
    OMX_U32 yOffset;
    
    // color format conversion
    OMX_COLOR_FORMATTYPE formatType;
    
    // picture rotation
    OMX_S32 angle;
    
    // H264 frame stabilization
    OMX_BOOL frameStabilization;

} PRE_PROCESSOR_CONFIG;

typedef struct RATE_CONTROL_CONFIG
{
    // If different than zero it will enable the VOP/picture based rate control. The QP will be
    // changed between VOPs/pictures. Enabled by default.
    //OMX_U32 vopRcEnabled;
    // Value range is [0, 1]
    OMX_U32 nPictureRcEnabled;

    // If different than zero it will enable the macroblock based rate control. The QP will
    // be adjusting also inside a VOP. Enabled by default.  
    //OMX_U32 mbRcEnabled;
    // Value range is [0, 1]
    OMX_U32 nMbRcEnabled;

    // If different than zero then frame skipping is enabled, i.e. the rate control is allowed
    // to skip frames if needed. When VBV is enabled, the rate control may have to skip
    // frames despite of this value. Disabled by default.
    //OMX_BOOL bVopSkipEnabled;
    // Value range is [0, 1]
    //OMX_U32 nSkippingEnabled;

    // Valid value range: [1, 31]
    // The default quantization parameter used by the encoder. If the rate control is
    // enabled then this value is used just at the beginning of the encoding process.
    // When the rate control is disabled then this QP value is used all the time. Default
    // value is 10.
    OMX_U32 nQpDefault;

    // The minimum QP that can be used by the RC in the stream. Default value is 1.
    // Valid value range: [1, 31]
    OMX_U32 nQpMin;

    // The maximum QP that can be used by the RC in the stream. Default value is 31.
    // Valid value range: [1, 31]
    OMX_U32 nQpMax;

    // The target bit rate as bits per second (bps) when the rate control is enabled. The
    // rate control is considered enabled when any of the VOP or MB based rate control is
    // enabled. Default value is the minimum between the maximum bitrate allowed for
    // the selected profile&level and 4mbps.
    OMX_U32 nTargetBitrate;
    
    //OMX_U32 eControlRate;
    
    // If different than zero it will enable the VBV model. Value 1 represents the video
    // buffer size defined by the standard. Otherwise this value represents the video
    // buffer size in units of 16384 bits. Enabled by default with size set by standard.
    // NOTE! This should always be ON since turning it OFF may not be 100% standard compatible.
    // (See Hantro MPEG4 API User Manual)
    OMX_U32 nVbvEnabled;

    OMX_VIDEO_CONTROLRATETYPE eRateControl;

    // Hypothetical Reference Decoder model, [0,1]
    // restricts the instantaneous bitrate and
    // total bit amount of every coded picture. 
    // Enabling HRD will cause tight constrains
    // on the operation of the rate control.
    // NOTE! If HRD is OFF output stream may not be
    // 100% standard compatible. (see hantro API user guide) 
    // NOTE! If HDR is used, bitrate can't be changed after stream
    // has been created.
    // u32 hrd;
    OMX_U32 nHrdEnabled;    
    
    // size in bits of Coded Picture Buffer used in HRD
    // NOTE: check if OMX supports this?
    // NOTE! Do not change unless you really know what you are doing!
    // u32 hrdCpbSize; 
    //OMX_U32 hrdCobSize;
} RATE_CONTROL_CONFIG;

typedef struct ENCODER_COMMON_CONFIG
{
    OMX_U32 nOutputWidth;
    OMX_U32 nOutputHeight;

    // frameRateNum
    // frameRateDenom
    OMX_U32 nInputFramerate;    // framerate in Q16 format

} ENCODER_COMMON_CONFIG;

#ifdef __cplusplus
}
#endif
#endif // HANTRO_ENCODER_H

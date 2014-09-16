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

#ifndef HANTRO_DECODER_H
#define HANTRO_DECODER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <OMX_Types.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <OSAL.h>

#include "dbgmacros.h"

#define USE_VIDEO_FREEZE_CONCEALMENT 0

    typedef enum MPEG4_FORMAT
    {
        MPEG4FORMAT_MPEG4,
        MPEG4FORMAT_SORENSON,
        MPEG4FORMAT_CUSTOM_1,
        MPEG4FORMAT_CUSTOM_1_3
    } MPEG4_FORMAT;

    typedef struct STREAM_BUFFER
    {
        OMX_U8 *bus_data;
        OSAL_BUS_WIDTH bus_address; // use this for HW
        OMX_U32 streamlen;
        OMX_U32 sliceInfoNum;
        OMX_U8 *pSliceInfo;
    } STREAM_BUFFER;

    typedef struct STREAM_INFO
    {
        OMX_COLOR_FORMATTYPE format;    // stream color format
        OMX_U32 framesize;   // framesize in bytes
        OMX_U32 width;       // picture display width
        OMX_U32 height;      // picture display height
        OMX_U32 sliceheight; // picture slice height
        OMX_U32 stride;      // picture scan line width
        OMX_U32 interlaced;  // is sequence interlaced
        OMX_U32 imageSize;   // size of image in memory
        OMX_BOOL isVc1Stream;
    } STREAM_INFO;

    typedef struct FRAME
    {
        OMX_U8 *fb_bus_data; // pointer to DMA accesible output buffer.
        OMX_U32 fb_bus_address; // memory buffer address on the bus
        OMX_U32 fb_size;     // buffer size
        OMX_U32 size;        // output frame size in bytes
        OMX_U32 MB_err_count;   // decoding macroblock error count
#ifdef ENABLE_CODEC_VP8
        OMX_BOOL isIntra;
        OMX_BOOL isGoldenOrAlternate;
#endif
    } FRAME;

    typedef enum ROTATION
    {
        ROTATE_NONE,
        ROTATE_RIGHT_90 = 90,
        ROTATE_LEFT_90 = -90,
        ROTATE_180 = 180,
        ROTATE_FLIP_VERTICAL,
        ROTATE_FLIP_HORIZONTAL
    } ROTATION;

#ifdef ANDROID
    /* Opencore specific */
    /* OMX COMPONENT CAPABILITY RELATED MEMBERS */ typedef struct
        PV_OMXComponentCapabilityFlagsType
        {
                OMX_BOOL iIsOMXComponentMultiThreaded;
                OMX_BOOL iOMXComponentSupportsExternalOutputBufferAlloc;
                OMX_BOOL iOMXComponentSupportsExternalInputBufferAlloc;
                OMX_BOOL iOMXComponentSupportsMovableInputBuffers;
                OMX_BOOL iOMXComponentSupportsPartialFrames;
                OMX_BOOL iOMXComponentUsesNALStartCode;
                OMX_BOOL iOMXComponentCanHandleIncompleteFrames;
                OMX_BOOL iOMXComponentUsesFullAVCFrames;
        } PV_OMXComponentCapabilityFlagsType;
#endif

//
// Post processor configuration arguments. Use 0 for default or "don't care".
//
    typedef struct PP_ARGS
    {
        struct scale
        {
            OMX_S32 width;
            OMX_S32 height;
        } scale;

        // Input video cropping.
        struct crop
        {
            OMX_S32 left;    // cropping start X offset.
            OMX_S32 top;     // cropping start Y offset.
            OMX_S32 width;
            OMX_S32 height;
        } crop;

        struct mask
        {
            OMX_S32 originX;
            OMX_S32 originY;
            OMX_U32 height;
            OMX_U32 width;
        } mask1, mask2;

        // Base address for the buffer from where the external blending data is read.
        OSAL_BUS_WIDTH blend_mask_base;

        // rotate in 90 degree units.
        // negative values imply rotation counterclockwise and positive values in clockwise rotation
        ROTATION rotation;

        // Specificy output color format.
        // Allowed values are:
        // OMX_COLOR_FormatYUV420PackedPlanar
        // OMX_COLOR_FormatYCbYCr
        // OMX_COLOR_Format32bitARGB8888
        // OMX_COLOR_Format32bitBGRA8888
        // OMX_COLOR_Format16bitARGB1555
        // OMX_COLOR_Format16bitARGB4444
        // OMX_COLOR_Format16bitRGB565
        // OMX_COLOR_Format16bitBGR565
        OMX_COLOR_FORMATTYPE format;

        // Adjust contrast on the output picture. A negative value will
        // decrease the contrast of the output picture while a positive one will increase it.
        // Only has an effect when output is in RGB.
        // Valid range: [-64, 64]
        int contrast;

        // Adjust the brigthness level of the output picture. A negative value
        // will decrese the brightness level of the output picture while a positive
        // one will increase it.
        // Only has an effect when output is in RGB.
        // Valid range: [-128, 127]
        int brightness;

        // Adjust the color saturation of the output picture. A negative value
        // will decrease the amount of color in the output picture while a positive
        // one will increase it.
        // Only has an effect when output is in RGB.
        // Valid range: [-64, 128]
        int saturation;

        // Set the alpha channel value in PP output data when the output format is ARGB or BGRA.
        // Only has an effect when output is in RGB.
        // Valid range: [0-255]
        // This feature does not affect the alpha blending mask feature
        // of the post-processor.
        int alpha;

        int dither;

    } PP_ARGS;

    typedef enum CODEC_STATE
    {
        CODEC_NEED_MORE,
        CODEC_HAS_FRAME,
        CODEC_HAS_INFO,
        CODEC_OK,
        CODEC_ERROR_HW_TIMEOUT = -1,
        CODEC_ERROR_HW_BUS_ERROR = -2,
        CODEC_ERROR_SYS = -3,
        CODEC_ERROR_DWL = -4,
        CODEC_ERROR_UNSPECIFIED = -5,
        CODEC_ERROR_STREAM = -6,
        CODEC_ERROR_INVALID_ARGUMENT = -7,
        CODEC_ERROR_NOT_INITIALIZED = -8,
        CODEC_ERROR_INITFAIL = -9,
        CODEC_ERROR_HW_RESERVED = -10,
        CODEC_ERROR_MEMFAIL = -11,
        CODEC_ERROR_STREAM_NOT_SUPPORTED = -12,
        CODEC_ERROR_FORMAT_NOT_SUPPORTED = -13
    } CODEC_STATE;

    typedef struct CODEC_PROTOTYPE CODEC_PROTOTYPE;

// internal CODEC interface, which wraps up Hantro API
    struct CODEC_PROTOTYPE
    {
        //
        // Destroy the codec instance
        //
        void (*destroy) (CODEC_PROTOTYPE *);
        //
        // Decode n bytes of data given in the stream buffer object.
        // On return consumed should indicate how many bytes were consumed from the buffer.
        //
        // The function should return one of following:
        //
        //    CODEC_NEED_MORE  - nothing happened, codec needs more data.
        //    CODEC_HAS_INFO   - headers were parsed and information about stream is ready.
        //    CODEC_HAS_FRAME  - codec has one or more headers ready
        //    less than zero   - one of the enumerated error values
        //
        // Parameters:
        //
        //    CODEC_PROTOTYPE  - this codec instance
        //    STREAM_BUFFER    - data to be decoded
        //    OMX_U32          - pointer to an integer that should on return indicate how many bytes were used from the input buffer
        //    FRAME            - where to store any frames that are ready immediately
         CODEC_STATE(*decode) (CODEC_PROTOTYPE *, STREAM_BUFFER *, OMX_U32 *,
                               FRAME *);

        //
        // Get info about the current stream. On return stream information is stored in STREAM_INFO object.
        //
        // The function should return one of the following:
        //
        //   CODEC_OK         - got information succesfully
        //   less than zero   - one of the enumerated error values
        //
         CODEC_STATE(*getinfo) (CODEC_PROTOTYPE *, STREAM_INFO *);

        //
        // Get a frame from the decoder. On return the FRAME object contains the frame data. If codec does internal
        // buffering this means that codec needs to copy the data into the specied buffer. Otherwise it might be
        // possible for a codec implementation to store the frame directly into the frame's buffer.
        //
        // The function should return one of the following:
        //
        //  CODEC_OK         - everything OK but no frame was available.
        //  CODEC_HAS_FRAME  - got frame ok.
        //  less than zero   - one of the enumerated error values
        //
        // Parameters:
        //
        //  CODEC_PROTOTYPE  - this codec instance
        //  FRAME            - location where frame data is to be written
        //  OMX_BOOL         - end of stream (EOS) flag
        //
         CODEC_STATE(*getframe) (CODEC_PROTOTYPE *, FRAME *, OMX_BOOL);

        //
        // Scan for complete frames in the stream buffer. First should be made to
        // give an offset to the first frame within the buffer starting from the start
        // of the buffer. Respectively last should be made to give an offset to the
        // start of the last frame in the buffer.
        //
        // Note that this does not tell whether the last frame is complete or not. Just that
        // complete frames possibly exists between first and last offsets.
        //
        // The function should return one of the following.
        //
        //   -1             - no frames found. Value of first and last undefined.
        //    1             - frames were found.
        //
        // Parameters:
        //
        //  CODEC_PROTOTYPE - this codec instance
        //  STREAM_BUFFER   - frame data pointer
        //  OMX_U32         - first offset pointer
        //  OMX_U32         - last offset pointer
        //
        int (*scanframe) (CODEC_PROTOTYPE *, STREAM_BUFFER *, OMX_U32 * first,
                          OMX_U32 * last);

        //
        // Set post-processor arguments.
        //
        // The function should return one of the following:
        // CODEC_OK         - parameters were set succesfully.
        // CODEC_ERROR_INVALID_ARGUMENT - an invalid post-processor argument was given.
        //
         CODEC_STATE(*setppargs) (CODEC_PROTOTYPE *, PP_ARGS *);

#ifdef DYNAMIC_SCALING
        //
        // Set post-processor scale arguments.
        //
        // The function should return one of the following:
        // CODEC_OK         - parameters were set succesfully.
        // CODEC_ERROR_INVALID_ARGUMENT - an invalid post-processor argument was given.
        //
         CODEC_STATE(*setscaling) (CODEC_PROTOTYPE *, OMX_U32 width, OMX_U32 height);
#endif
    };

#ifdef __cplusplus
}
#endif
#endif                       // HANTRO_DECODER_H

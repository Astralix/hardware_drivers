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
--  Abstract : Hantro 7280 H.264 Encoder API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: h264encapi.h,v $
--  $Revision: 1.7 $
--  $Date: 2010/04/19 09:19:02 $
--
------------------------------------------------------------------------------*/

#ifndef __H264ENCAPI_H__
#define __H264ENCAPI_H__

#include "basetype.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

    typedef const void *H264EncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

/* Function return values */
    typedef enum
    {
        H264ENC_OK = 0,
        H264ENC_FRAME_READY = 1,

        H264ENC_ERROR = -1,
        H264ENC_NULL_ARGUMENT = -2,
        H264ENC_INVALID_ARGUMENT = -3,
        H264ENC_MEMORY_ERROR = -4,
        H264ENC_EWL_ERROR = -5,
        H264ENC_EWL_MEMORY_ERROR = -6,
        H264ENC_INVALID_STATUS = -7,
        H264ENC_OUTPUT_BUFFER_OVERFLOW = -8,
        H264ENC_HW_BUS_ERROR = -9,
        H264ENC_HW_DATA_ERROR = -10,
        H264ENC_HW_TIMEOUT = -11,
        H264ENC_HW_RESERVED = -12,
        H264ENC_SYSTEM_ERROR = -13,
        H264ENC_INSTANCE_ERROR = -14,
        H264ENC_HRD_ERROR = -15,
        H264ENC_HW_RESET = -16
    } H264EncRet;

/* Stream type for initialization */
    typedef enum
    {
        H264ENC_BYTE_STREAM = 0,    /* H.264 annex B: NAL unit starts with '00 00 00 01' */
        H264ENC_NAL_UNIT_STREAM = 1 /* Plain NAL units */
    } H264EncStreamType;

/* Profile and level for initialization */
    typedef enum
    {
        H264ENC_BASELINE_LEVEL_1 = 10,
        H264ENC_BASELINE_LEVEL_1_b = 99,
        H264ENC_BASELINE_LEVEL_1_1 = 11,
        H264ENC_BASELINE_LEVEL_1_2 = 12,
        H264ENC_BASELINE_LEVEL_1_3 = 13,
        H264ENC_BASELINE_LEVEL_2 = 20,
        H264ENC_BASELINE_LEVEL_2_1 = 21,
        H264ENC_BASELINE_LEVEL_2_2 = 22,
        H264ENC_BASELINE_LEVEL_3 = 30,
        H264ENC_BASELINE_LEVEL_3_1 = 31,
        H264ENC_BASELINE_LEVEL_3_2 = 32
    } H264EncProfileAndLevel;

/* Picture input type for pre-processing */
    typedef enum
    {
        H264ENC_YUV420_PLANAR = 0,              /* YYYY... UUUU... VVVV */
        H264ENC_YUV420_SEMIPLANAR = 1,          /* YYYY... UVUVUV...    */
        H264ENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */
        H264ENC_YUV422_INTERLEAVED_UYVY = 3,    /* UYVYUYVY...          */
        H264ENC_RGB565 = 4,                     /* 16-bit RGB           */
        H264ENC_BGR565 = 5,                     /* 16-bit RGB           */
        H264ENC_RGB555 = 6,                     /* 15-bit RGB           */
        H264ENC_BGR555 = 7,                     /* 15-bit RGB           */
        H264ENC_RGB444 = 8,                     /* 12-bit RGB           */
        H264ENC_BGR444 = 9                      /* 12-bit RGB           */
    } H264EncPictureType;

/* Picture rotation for pre-processing */
    typedef enum
    {
        H264ENC_ROTATE_0 = 0,
        H264ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
        H264ENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
    } H264EncPictureRotation;

/* Picture color space conversion (RGB input) for pre-processing */
    typedef enum
    {
        H264ENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
        H264ENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
        H264ENC_RGBTOYUV_USER_DEFINED = 2  /* User defined color conversion */
    } H264EncColorConversionType;

/* Picture type for encoding */
    typedef enum
    {
        H264ENC_INTRA_FRAME = 0,
        H264ENC_PREDICTED_FRAME = 1,
        H264ENC_NOTCODED_FRAME
    } H264EncPictureCodingType;

/* Complexity level */
    typedef enum
    {
        H264ENC_COMPLEXITY_1 = 1
    } H264EncComplexityLevel;

/*------------------------------------------------------------------------------
    3. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Configuration info for initialization
 * Width and height are picture dimensions after rotation
 * Width and height are restricted by level limitations
 */
    typedef struct
    {
        H264EncStreamType streamType;
        H264EncProfileAndLevel profileAndLevel;
        u32 width;  /* Encoded picture width in pixels, multiple of 4 */
        u32 height; /* Encoded picture height in pixels, multiple of 2 */
        u32 frameRateNum;   /* The stream time scale, [1..65535] */
        u32 frameRateDenom; /* Maximum frame rate is frameRateNum/frameRateDenom in 
                             * frames/second. The actual frame rate will be defined
                             * by timeIncrement of encoded pictures.
                             * [1..frameRateNum]
                             */
        H264EncComplexityLevel complexityLevel;
    } H264EncConfig;

/* Coding control parameters */
    typedef struct
    {
        u32 sliceSize;  /* Slice size in macroblock rows,
                         * 0 to encode each picture in one slice,
                         * [0..height/16]
                         */
        u32 seiMessages;    /* Insert picture timing and buffering 
                             * period SEI messages into the stream,
                             * [0,1]
                             */
        u32 videoFullRange; /* Input video signal sample range, [0,1]
                             * 0 = Y range in [16..235],
                             * Cb&Cr range in [16..240]
                             * 1 = Y, Cb and Cr range in [0..255]
                             */
        u32 constrainedIntraPrediction; /* [0,1] */
        u32 disableDeblockingFilter;    /* [0,2] */
        u32 sampleAspectRatioWidth; /* Horizontal size of the sample aspect
                                     * ratio (in arbitrary units), 0 for
                                     * unspecified, [0..65535]
                                     */
        u32 sampleAspectRatioHeight;    /* Vertical size of the sample aspect ratio
                                         * (in same units as sampleAspectRatioWidth)
                                         * 0 for unspecified, [0..65535]
                                         */
    } H264EncCodingCtrl;

/* Rate control parameters */
    typedef struct
    {
        u32 pictureRc;  /* Adjust QP between pictures, [0,1] */
        u32 mbRc;   /* Adjust QP inside picture, [0,1] */
        u32 pictureSkip;    /* Allow rate control to skip pictures, [0,1] */
        i32 qpHdr;  /* QP for next encoded picture, [-1..51]
                     * -1 = Let rate control calculate initial QP
                     * This QP is used for all pictures if 
                     * HRD and pictureRc and mbRc are disabled
                     * If HRD is enabled it may override this QP
                     */
        u32 qpMin;  /* Minimum QP for any picture, [0..51] */
        u32 qpMax;  /* Maximum QP for any picture, [0..51] */
        u32 bitPerSecond;   /* Target bitrate in bits/second, this is 
                             * needed if pictureRc, mbRc, pictureSkip or 
                             * hrd is enabled
                             */
        u32 hrd;    /* Hypothetical Reference Decoder model, [0,1]
                     * restricts the instantaneous bitrate and
                     * total bit amount of every coded picture. 
                     * Enabling HRD will cause tight constrains
                     * on the operation of the rate control
                     */
        u32 hrdCpbSize; /* size in bits of Coded Picture Buffer used in HRD */
        u32 gopLen;  /* Length for Group of Pictures, indicates the distance of 
                        two intra pictures, including first intra [1..150] */
    } H264EncRateCtrl;

/* Encoder input structure */
    typedef struct
    {
        u32 busLuma;    /* Bus address for input picture
                         * planar format: luminance component
                         * semiplanar format: luminance component 
                         * interleaved format: whole picture
                         */
        u32 busChromaU; /* Bus address for input chrominance
                         * planar format: cb component
                         * semiplanar format: both chrominance
                         * interleaved format: not used
                         */
        u32 busChromaV; /* Bus address for input chrominance
                         * planar format: cr component
                         * semiplanar format: not used
                         * interleaved format: not used
                         */
        u32 timeIncrement;  /* The previous picture duration in units
                             * of frameRateDenom/frameRateNum.
                             * 0 for the very first picture.
                             */
        u32 *pOutBuf;   /* Pointer to output stream buffer */
        u32 busOutBuf;  /* Bus address of output stream buffer */
        u32 outBufSize; /* Size of output stream buffer in bytes */
        u32 *pNaluSizeBuf;  /* Output buffer for NAL unit sizes
                             * pNaluSizeBuf[0] = NALU 0 size in bytes
                             * pNaluSizeBuf[1] = NALU 1 size in bytes 
                             * etc
                             */
        u32 naluSizeBufSize;    /* Size of pNaluSizeBuf in bytes */
        H264EncPictureCodingType codingType;    /* Proposed picture coding type, 
                                                 * INTRA/PREDICTED
                                                 */
        u32 busLumaStab;    /* bus address of next picture to stabilize (luminance) */
    } H264EncIn;

/* Encoder output structure */
    typedef struct
    {
        H264EncPictureCodingType codingType;    /* Realized picture coding type,
                                                 * INTRA/PREDICTED/NOTCODED
                                                 */
        u32 streamSize; /* Size of output stream in bytes */
    } H264EncOut;

/* Input pre-processing */
    typedef struct
    {
        H264EncColorConversionType type;
        u16 coeffA;    /* User defined color conversion coefficient */
        u16 coeffB;    /* User defined color conversion coefficient */
        u16 coeffC;    /* User defined color conversion coefficient */
        u16 coeffE;    /* User defined color conversion coefficient */
        u16 coeffF;    /* User defined color conversion coefficient */
    } H264EncColorConversion;

    typedef struct
    {
        u32 origWidth;
        u32 origHeight;
        u32 xOffset;
        u32 yOffset;
        H264EncPictureType inputType;
        H264EncPictureRotation rotation;
        u32 videoStabilization;
        H264EncColorConversion colorConversion;
    } H264EncPreProcessingCfg;

/* Version information */
    typedef struct
    {
        u32 major;  /* Encoder API major version */
        u32 minor;  /* Encoder API minor version */
    } H264EncApiVersion;

    typedef struct
    {
        u32 swBuild;    /* Software build ID */
        u32 hwBuild;    /* Hardware build ID */
    } H264EncBuild;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Version information */
    H264EncApiVersion H264EncGetApiVersion(void);
    H264EncBuild H264EncGetBuild(void);

/* Initialization & release */
    H264EncRet H264EncInit(const H264EncConfig * pEncConfig,
                           H264EncInst * instAddr);
    H264EncRet H264EncRelease(H264EncInst inst);

/* Encoder configuration before stream generation */
    H264EncRet H264EncSetCodingCtrl(H264EncInst inst, const H264EncCodingCtrl *
                                    pCodingParams);
    H264EncRet H264EncGetCodingCtrl(H264EncInst inst, H264EncCodingCtrl *
                                    pCodingParams);

/* Encoder configuration before and during stream generation */
    H264EncRet H264EncSetRateCtrl(H264EncInst inst,
                                  const H264EncRateCtrl * pRateCtrl);
    H264EncRet H264EncGetRateCtrl(H264EncInst inst,
                                  H264EncRateCtrl * pRateCtrl);

    H264EncRet H264EncSetPreProcessing(H264EncInst inst,
                                       const H264EncPreProcessingCfg *
                                       pPreProcCfg);
    H264EncRet H264EncGetPreProcessing(H264EncInst inst,
                                       H264EncPreProcessingCfg * pPreProcCfg);

/* Encoder user data insertion during stream generation */
    H264EncRet H264EncSetSeiUserData(H264EncInst inst, const u8 * pUserData,
                                     u32 userDataSize);

/* Stream generation */

/* H264EncStrmStart generates the SPS and PPS. SPS is the first NAL unit and PPS
 * is the second NAL unit. NaluSizeBuf indicates the size of NAL units.
 */
    H264EncRet H264EncStrmStart(H264EncInst inst, const H264EncIn * pEncIn,
                                H264EncOut * pEncOut);

/* H264EncStrmEncode encodes one video frame. If SEI messages are enabled the
 * first NAL unit is a SEI message.
 */
    H264EncRet H264EncStrmEncode(H264EncInst inst, const H264EncIn * pEncIn,
                                 H264EncOut * pEncOut);

/* H264EncStrmEnd ends a stream with an EOS code. */
    H264EncRet H264EncStrmEnd(H264EncInst inst, const H264EncIn * pEncIn,
                              H264EncOut * pEncOut);

/* Hantro internal encoder testing */
    H264EncRet H264EncSetTestId(H264EncInst inst, u32 testId);

/*------------------------------------------------------------------------------
    5. Encoder API tracing callback function
------------------------------------------------------------------------------*/

    void H264EncTrace(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /*__H264ENCAPI_H__*/

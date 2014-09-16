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
--  Abstract : Hantro 7280 MPEG4 Encoder API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: mp4encapi.h,v $
--  $Revision: 1.5 $
--  $Date: 2008/09/16 07:36:29 $
--
------------------------------------------------------------------------------*/

#ifndef __MP4ENCAPI_H__
#define __MP4ENCAPI_H__

#include "basetype.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

    typedef const void *MP4EncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

/* Function return values */
    typedef enum
    {
        ENC_OK = 0,
        ENC_VOP_READY = 1,
        ENC_GOV_READY = 2,

        ENC_ERROR = -1,
        ENC_NULL_ARGUMENT = -2,
        ENC_INVALID_ARGUMENT = -3,
        ENC_MEMORY_ERROR = -4,
        ENC_EWL_ERROR = -5,
        ENC_EWL_MEMORY_ERROR = -6,
        ENC_INVALID_STATUS = -7,
        ENC_OUTPUT_BUFFER_OVERFLOW = -8,
        ENC_HW_BUS_ERROR = -9,
        ENC_HW_DATA_ERROR = -10,
        ENC_HW_TIMEOUT = -11,
        ENC_HW_RESERVED = -12,
        ENC_SYSTEM_ERROR = -13,
        ENC_INSTANCE_ERROR = -14,
        ENC_HW_RESET = -16
    } MP4EncRet;

/* Stream type for initialization */
    typedef enum
    {
        MPEG4_PLAIN_STRM = 0,
        MPEG4_VP_STRM = 1,
        MPEG4_VP_DP_STRM = 2,
        MPEG4_VP_DP_RVLC_STRM = 3,
        MPEG4_SVH_STRM = 4,
        H263_STRM = 5
    } MP4EncStrmType;

/* Profile and level for initialization */
    typedef enum
    {
        MPEG4_SIMPLE_PROFILE_LEVEL_0 = 8,
        MPEG4_SIMPLE_PROFILE_LEVEL_0B = 9,
        MPEG4_SIMPLE_PROFILE_LEVEL_1 = 1,
        MPEG4_SIMPLE_PROFILE_LEVEL_2 = 2,
        MPEG4_SIMPLE_PROFILE_LEVEL_3 = 3,
        MPEG4_SIMPLE_PROFILE_LEVEL_4A = 4,
        MPEG4_SIMPLE_PROFILE_LEVEL_5 = 5,
        MPEG4_SIMPLE_PROFILE_LEVEL_6 = 6,
        MPEG4_MAIN_PROFILE_LEVEL_4 = 52,    /* onlyone to support 1080p resolution */
        MPEG4_ADV_SIMPLE_PROFILE_LEVEL_3 = 243,
        MPEG4_ADV_SIMPLE_PROFILE_LEVEL_4 = 244,
        MPEG4_ADV_SIMPLE_PROFILE_LEVEL_5 = 245,
        H263_PROFILE_0_LEVEL_10 = 1001,
        H263_PROFILE_0_LEVEL_20 = 1002,
        H263_PROFILE_0_LEVEL_30 = 1003,
        H263_PROFILE_0_LEVEL_40 = 1004,
        H263_PROFILE_0_LEVEL_50 = 1005,
        H263_PROFILE_0_LEVEL_60 = 1006,
        H263_PROFILE_0_LEVEL_70 = 1007
    } MP4EncProfileAndLevel;

/* User data type */
    typedef enum
    {
        MPEG4_VOS_USER_DATA,
        MPEG4_VO_USER_DATA,
        MPEG4_VOL_USER_DATA,
        MPEG4_GOV_USER_DATA
    } MP4EncUsrDataType;

/* Picture input type for pre-processing */
    typedef enum
    {
        ENC_YUV420_PLANAR = 0,              /* YYYY... UUUU... VVVV */
        ENC_YUV420_SEMIPLANAR = 1,          /* YYYY... UVUVUV...    */
        ENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */
        ENC_YUV422_INTERLEAVED_UYVY = 3,    /* UYVYUYVY...          */
        ENC_RGB565 = 4,                     /* 16-bit RGB           */
        ENC_BGR565 = 5,                     /* 16-bit RGB           */
        ENC_RGB555 = 6,                     /* 15-bit RGB           */
        ENC_BGR555 = 7,                     /* 15-bit RGB           */
        ENC_RGB444 = 8,                     /* 12-bit RGB           */
        ENC_BGR444 = 9                      /* 12-bit RGB           */
    } MP4EncPictureType;

/* Picture rotation for pre-processing */
    typedef enum
    {
        ENC_ROTATE_0 = 0,
        ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
        ENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
    } MP4EncPictureRotation;

/* Picture color space conversion (RGB input) for pre-processing */
    typedef enum
    {
        ENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
        ENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
        ENC_RGBTOYUV_USER_DEFINED = 2  /* User defined color conversion */
    } MP4EncColorConversionType;

/* VOP type for encoding */
    typedef enum
    {
        INTRA_VOP = 0,
        PREDICTED_VOP = 1,
        NOTCODED_VOP    /* Used just as a return type */
    } MP4EncVopType;

/*------------------------------------------------------------------------------
    3. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Configuration info for initialization */
    typedef struct
    {
        MP4EncStrmType strmType;
        MP4EncProfileAndLevel profileAndLevel;
        u32 width;
        u32 height;
        u32 frmRateNum;
        u32 frmRateDenom;
    } MP4EncCfg;

    typedef struct
    {
        u32 videoRange;
        u32 pixelAspectRatioWidth;
        u32 pixelAspectRatioHeight;
    } MP4EncStreamInfo;

/* Coding control parameters */
    typedef struct
    {
        u32 insHEC;
        u32 insGOB;
        u32 insGOV;
        u32 vpSize;
    } MP4EncCodingCtrl;

/* Rate control parameters */
    typedef struct
    {
        u32 vopRc;
        u32 mbRc;
        u32 vopSkip;
        i32 qpHdr;          /* [1..31] Initial quantization parameter value, 
                                -1 = Let RC calculate initial QP */
        u32 qpMin;
        u32 qpMax;
        u32 bitPerSecond;
        u32 vbv;
        u32 gopLen;
    } MP4EncRateCtrl;

/* Encoder input structure */
    typedef struct
    {
        u32 busLuma;
        u32 busChromaU;
        u32 busChromaV;
        u32 *pOutBuf;
        u32 outBufBusAddress;
        u32 outBufSize;
        MP4EncVopType vopType;
        u32 timeIncr;
        u32 *pVpBuf;    /* Table for video packet sizes */
        u32 vpBufSize;  /* Size of pVpBuf in bytes */
        u32 busLumaStab;    /* bus address of next picture to stabilize (luminance) */
    } MP4EncIn;

/* Time code structure for encoder output */
    typedef struct
    {
        u32 hours;
        u32 minutes;
        u32 seconds;
        u32 timeIncr;
        u32 timeRes;
    } TimeCode;

/* Encoder output structure */
    typedef struct
    {
        TimeCode timeCode;
        MP4EncVopType vopType;
        u32 strmSize;
    } MP4EncOut;

/* Input pre-processing */
    typedef struct
    {
        MP4EncColorConversionType type;
        u16 coeffA;    /* User defined color conversion coefficient */
        u16 coeffB;    /* User defined color conversion coefficient */
        u16 coeffC;    /* User defined color conversion coefficient */
        u16 coeffE;    /* User defined color conversion coefficient */
        u16 coeffF;    /* User defined color conversion coefficient */
    } MP4EncColorConversion;

    typedef struct
    {
        u32 origWidth;
        u32 origHeight;
        u32 xOffset;
        u32 yOffset;
        MP4EncPictureType inputType;
        MP4EncPictureRotation rotation;
        u32 videoStabilization;
        MP4EncColorConversion colorConversion;
    } MP4EncPreProcessingCfg;

/* Version information */
    typedef struct
    {
        u32 major;  /* Encoder API major version */
        u32 minor;  /* Encoder API minor version */
    } MP4EncApiVersion;

    typedef struct
    {
        u32 swBuild;    /* Software build ID */
        u32 hwBuild;    /* Hardware build ID */
    } MP4EncBuild;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Version information */
    MP4EncApiVersion MP4EncGetApiVersion(void);
    MP4EncBuild MP4EncGetBuild(void);

/* Initialization & release */
    MP4EncRet MP4EncInit(const MP4EncCfg * pEncCfg, MP4EncInst * instAddr);
    MP4EncRet MP4EncRelease(MP4EncInst inst);

/* Encoder configuration */
    MP4EncRet MP4EncSetStreamInfo(MP4EncInst inst,
                                  const MP4EncStreamInfo * pStreamInfo);
    MP4EncRet MP4EncGetStreamInfo(MP4EncInst inst,
                                  MP4EncStreamInfo * pStreamInfo);

    MP4EncRet MP4EncSetCodingCtrl(MP4EncInst inst, const MP4EncCodingCtrl *
                                  pCodeParams);
    MP4EncRet MP4EncGetCodingCtrl(MP4EncInst inst, MP4EncCodingCtrl *
                                  pCodeParams);
    MP4EncRet MP4EncSetRateCtrl(MP4EncInst inst,
                                const MP4EncRateCtrl * pRateCtrl);
    MP4EncRet MP4EncGetRateCtrl(MP4EncInst inst, MP4EncRateCtrl * pRateCtrl);
    MP4EncRet MP4EncSetUsrData(MP4EncInst inst, const u8 * pBuf, u32 length,
                               MP4EncUsrDataType type);

/* Stream generation */
    MP4EncRet MP4EncStrmStart(MP4EncInst inst, const MP4EncIn * pEncIn,
                              MP4EncOut * pEncOut);
    MP4EncRet MP4EncStrmEncode(MP4EncInst inst, const MP4EncIn * pEncIn,
                               MP4EncOut * pEncOut);
    MP4EncRet MP4EncStrmEnd(MP4EncInst inst, MP4EncIn * pEncIn,
                            MP4EncOut * pEncOut);

/* Pre processing */
    MP4EncRet MP4EncSetPreProcessing(MP4EncInst inst,
                                     const MP4EncPreProcessingCfg *
                                     pPreProcCfg);
    MP4EncRet MP4EncGetPreProcessing(MP4EncInst inst,
                                     MP4EncPreProcessingCfg * pPreProcCfg);

/* Hantro internal encoder testing */
    MP4EncRet MP4EncSetTestId(MP4EncInst inst, u32 testId);

/*------------------------------------------------------------------------------
    5. Encoder API tracing callback function
------------------------------------------------------------------------------*/

    void MP4Enc_Trace(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /*__MP4ENCAPI_H__*/

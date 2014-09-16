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
--  Abstract : Hantro 7280 Jpeg Encoder API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: jpegencapi.h,v $
--  $Revision: 1.7 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents

    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef __JPEGENCAPI_H__
#define __JPEGENCAPI_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/

    typedef const void *JpegEncInst;

    typedef enum
    {
        JPEGENC_FRAME_READY = 1,
        JPEGENC_RESTART_INTERVAL = 2,
        JPEGENC_OK = 0,
        JPEGENC_ERROR = -1,
        JPEGENC_NULL_ARGUMENT = -2,
        JPEGENC_INVALID_ARGUMENT = -3,
        JPEGENC_MEMORY_ERROR = -4,
        JPEGENC_INVALID_STATUS = -5,
        JPEGENC_OUTPUT_BUFFER_OVERFLOW = -6,
        JPEGENC_EWL_ERROR = -7,
        JPEGENC_EWL_MEMORY_ERROR = -8,
        JPEGENC_HW_BUS_ERROR = -9,
        JPEGENC_HW_DATA_ERROR = -10,
        JPEGENC_HW_TIMEOUT = -11,
        JPEGENC_HW_RESERVED = -12,
        JPEGENC_SYSTEM_ERROR = -13,
        JPEGENC_INSTANCE_ERROR = -14,
        JPEGENC_HW_RESET = -15
    } JpegEncRet;

/* Picture YUV type for initialization */
    typedef enum
    {
        JPEGENC_YUV420_PLANAR = 0,              /* YYYY... UUUU... VVVV */
        JPEGENC_YUV420_SEMIPLANAR = 1,          /* YYYY... UVUVUV...    */
        JPEGENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */
        JPEGENC_YUV422_INTERLEAVED_UYVY = 3,    /* UYVYUYVY...          */
        JPEGENC_RGB565 = 4,                     /* 16-bit RGB           */
        JPEGENC_BGR565 = 5,                     /* 16-bit RGB           */
        JPEGENC_RGB555 = 6,                     /* 15-bit RGB           */
        JPEGENC_BGR555 = 7,                     /* 15-bit RGB           */
        JPEGENC_RGB444 = 8,                     /* 12-bit RGB           */
        JPEGENC_BGR444 = 9,                     /* 12-bit RGB           */
        JPEGENC_YUV422_PLANAR = 10              /* YYYY... UUUU... VVVV */
    } JpegEncFrameType;

/* Picture rotation for initialization */
    typedef enum
    {
        JPEGENC_ROTATE_0 = 0,
        JPEGENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
        JPEGENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
    } JpegEncPictureRotation;

/* Picture color space conversion for RGB input */
    typedef enum
    {
        JPEGENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
        JPEGENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
        JPEGENC_RGBTOYUV_USER_DEFINED = 2  /* User defined color conversion */
    } JpegEncColorConversionType;

    typedef enum
    {
        JPEGENC_WHOLE_FRAME = 0,    /* The whole frame is stored in linear memory */
        JPEGENC_SLICED_FRAME    /* The frame is sliced into restart intervals; Input address is given for each slice */
    } JpegEncCodingType;

    typedef enum
    {
        JPEGENC_420_MODE = 0,   /* Encoding in YUV 4:2:0 mode */
        JPEGENC_422_MODE        /* Encoding in YUV 4:2:2 mode, requires
                                   4:2:2 input frame type */
    } JpegEncCodingMode;

    typedef enum
    {
        JPEGENC_NO_UNITS = 0,   /* No units, X and Y specify the pixel aspect ratio */
        JPEGENC_DOTS_PER_INCH = 1,  /* X and Y are dots per inch */
        JPEGENC_DOTS_PER_CM = 2 /* X and Y are dots per cm   */
    } JpegEncAppUnitsType;

    typedef enum
    {
        JPEGENC_SINGLE_MARKER = 0,  /* Luma/Chroma tables are written behind own marker/components */
        JPEGENC_MULTI_MARKER    /* Luma/Chroma tables are written behind one marker/components */
    } JpegEncTableMarkerType;

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/
/* Version information */
    typedef struct
    {
        u32 major;  /* Encoder API major version                    */
        u32 minor;  /* Encoder API minor version                    */
    } JpegEncApiVersion;
    typedef struct
    {
        u32 swBuild;    /* Software build ID */
        u32 hwBuild;    /* Hardware build ID */
    } JpegEncBuild;

/* thumbnail info */
    typedef struct
    {
        u32 inputWidth;         /* Number of pixels/line in input image       */
        u32 inputHeight;        /* Number of lines in input image             */
        u32 xOffset;            /* Pixels from top-left corner of input image */
        u32 yOffset;            /*   to top-left corner of encoded image      */
        u32 codingWidth;        /* Width of encoded image                     */
        u32 codingHeight;       /* Height of encoded image                    */
    } JpegEncCfgThumb;

/* RGB input to YUV color conversion */
    typedef struct
    {
        JpegEncColorConversionType type;
        u16 coeffA;             /* User defined color conversion coefficient  */
        u16 coeffB;             /* User defined color conversion coefficient  */
        u16 coeffC;             /* User defined color conversion coefficient  */
        u16 coeffE;             /* User defined color conversion coefficient  */
        u16 coeffF;             /* User defined color conversion coefficient  */
    } JpegEncColorConversion;

/* Encoder configuration */
    typedef struct
    {
        u32 inputWidth;         /* Number of pixels/line in input image       */
        u32 inputHeight;        /* Number of lines in input image             */
        u32 xOffset;            /* Pixels from top-left corner of input image */
        u32 yOffset;            /*   to top-left corner of encoded image      */
        u32 codingWidth;        /* Width of encoded image                     */
        u32 codingHeight;       /* Height of encoded image                    */
        u32 restartInterval;    /* Restart interval (MCU lines)               */
        u32 qLevel;             /* Quantization level (0 - 10)                */
        JpegEncFrameType frameType;             /* Input frame YUV/RGB format */
        JpegEncColorConversion colorConversion; /* RGB to YUV conversion      */
        JpegEncPictureRotation rotation;        /* rotation off/-90/+90       */
        JpegEncCodingType codingType;       /* Whole frame / restart interval */
        JpegEncCodingMode codingMode;       /* 4:2:0 / 4:2:2 coding           */
        JpegEncAppUnitsType unitsType;      /* Units for X & Y density in APP0*/
        JpegEncTableMarkerType markerType;  /* Table marker type              */
        u32 xDensity;           /* Horizontal pixel density                   */
        u32 yDensity;           /* Vertical pixel density                     */
        u32 comLength;          /* Length of COM header                       */
        u8 *pCom;               /* Comment header pointer                     */
        u32 thumbnail;              /* thumbnail                              */
        JpegEncCfgThumb cfgThumb;   /* thumbnail dimension                    */
    } JpegEncCfg;

/* Input info */
    typedef struct
    {
        u32 frameHeader;        /* Enable/disable creation of frame headers   */
        u32 busLum;             /* Bus address of luminance input   (Y)       */
        u32 busCb;              /* Bus address of chrominance input (Cb)      */
        u32 busCr;              /* Bus address of chrominance input (Cr)      */
        u8 *pLum;               /* Pointer to luminance input   (Y)           */
        u8 *pCb;                /* Pointer to chrominance input (Cb)          */
        u8 *pCr;                /* Pointer to chrominance input (Cr)          */
        u8 *pOutBuf;            /* Pointer to output buffer                   */
        u32 busOutBuf;          /* Bus address of output stream buffer        */
        u32 outBufSize;         /* Size of output buffer (bytes)              */
    } JpegEncIn;

/* Output info */
    typedef struct
    {
        u32 jfifSize;           /* Encoded JFIF size (bytes)                  */
    } JpegEncOut;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

/* Version information */
    JpegEncApiVersion JpegEncGetApiVersion(void);

/* Build information */
    JpegEncBuild JpegEncGetBuild(void);

/* Initialization & release */
    JpegEncRet JpegEncInit(JpegEncCfg * pEncCfg, JpegEncInst * instAddr);
    JpegEncRet JpegEncRelease(JpegEncInst inst);

/* Mode setting */
    JpegEncRet JpegEncSetThumbnailMode(JpegEncInst inst, JpegEncCfg * pEncCfg);
    JpegEncRet JpegEncSetFullResolutionMode(JpegEncInst inst,
                                            JpegEncCfg * pEncCfg);

/* Jfif generation */
    JpegEncRet JpegEncEncode(JpegEncInst inst,
                             JpegEncIn * pEncIn, JpegEncOut * pEncOut);

/*------------------------------------------------------------------------------
    5. Encoder API tracing callback function
------------------------------------------------------------------------------*/

    void JpegEnc_Trace(const char *msg);

#ifdef __cplusplus
}
#endif

#endif

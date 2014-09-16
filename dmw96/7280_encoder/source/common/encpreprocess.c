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
--  Description : Preprocessor setup
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: encpreprocess.c,v $
--  $Revision: 1.6 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "encpreprocess.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------

	EncPreProcessCheck

	Check image size: Cropped frame _must_ fit inside of source image

	Input	preProcess Pointer to preProcess_s structure.

	Return	ENCHW_OK	No errors.
		ENCHW_NOK	Error condition.

------------------------------------------------------------------------------*/
i32 EncPreProcessCheck(const preProcess_s * preProcess)
{
    i32 status = ENCHW_OK;
    u32 tmp;
    u32 width, height;

    ASSERT(preProcess->mbHeight == 8 || preProcess->mbHeight == 16);

#if 0
    u32 w_mask;

    if(preProcess->inputFormat == 0)
        w_mask = 0x0F;  /* 16 multiple */
    else
        w_mask = 0x07;  /* 8 multiple  */

    if(preProcess->lumWidthSrc & w_mask)
    {
        status = ENCHW_NOK;
    }
#endif

    if(preProcess->lumHeightSrc & 0x01)
    {
        status = ENCHW_NOK;
    }

    if(preProcess->lumWidthSrc > MAX_INPUT_IMAGE_WIDTH)
    {
        status = ENCHW_NOK;
    }

    width = preProcess->lumWidth;
    height = preProcess->lumHeight;
    if(preProcess->rotation)
    {
        u32 tmp;

        tmp = width;
        width = height;
        height = tmp;
    }

    /* Bottom right corner */
    tmp = preProcess->horOffsetSrc + width;
    if(tmp > preProcess->lumWidthSrc)
    {
        status = ENCHW_NOK;
    }

    tmp = preProcess->verOffsetSrc + height;
    if(tmp > preProcess->lumHeightSrc)
    {
        status = ENCHW_NOK;
    }

    return status;
}

/*------------------------------------------------------------------------------

	EncPreProcess

	Preform cropping

	Input	asic	Pointer to asicData_s structure
		preProcess Pointer to preProcess_s structure.

------------------------------------------------------------------------------*/
void EncPreProcess(asicData_s * asic, const preProcess_s * preProcess)
{
    u32 tmp;
    u32 width, height;
    regValues_s *regs;
    u32 stride;

    ASSERT(asic != NULL && preProcess != NULL);

    regs = &asic->regs;

    stride = (preProcess->lumWidthSrc + 15) & (~15); /* 16 pixel multiple stride */

    /* cropping */
    if(preProcess->inputFormat < 2 || preProcess->inputFormat == 10)
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc;
        tmp *= stride;
        tmp += preProcess->horOffsetSrc;
        regs->inputLumBase += (tmp & (~7));
        regs->inputLumaBaseOffset = tmp & 7;

        if(preProcess->videoStab)
        {
            regs->vsNextLumaBase += (tmp & (~7));
        }

        /* Chroma */
        if(preProcess->inputFormat == 0)    /* 420 planar */
        {
            tmp = preProcess->verOffsetSrc / 2;
            tmp *= stride / 2;
            tmp += preProcess->horOffsetSrc / 2;
            regs->inputCbBase += (tmp & (~7));
            regs->inputCrBase += (tmp & (~7));
            regs->inputChromaBaseOffset = tmp & 7;
        }
        else if(preProcess->inputFormat == 1)   /* 420 semiplanar */
        {
            tmp = preProcess->verOffsetSrc / 2;
            tmp *= stride / 2;
            tmp += preProcess->horOffsetSrc / 2;

            tmp *= 2;

            regs->inputCbBase += (tmp & (~7));
            regs->inputChromaBaseOffset = tmp & 7;
        }
        else /* 422 planar */
        {
            tmp = preProcess->verOffsetSrc;
            tmp *= stride / 2;
            tmp += preProcess->horOffsetSrc / 2;

            regs->inputCbBase += (tmp & (~7));
            regs->inputCrBase += (tmp & (~7));
            regs->inputChromaBaseOffset = tmp & 7;
        }
    }
    else
    {
        /* Input image position after crop and stabilization */
        tmp = preProcess->verOffsetSrc;
        tmp *= stride;
        tmp += preProcess->horOffsetSrc;
        tmp *= 2;

        regs->inputLumBase += (tmp & (~7));
        regs->inputLumaBaseOffset = tmp & 7;
        regs->inputChromaBaseOffset = (regs->inputLumaBaseOffset / 4) * 4;

        if(preProcess->videoStab)
        {
            regs->vsNextLumaBase += (tmp & (~7));
        }

    }

    /* YUV subsampling and rotation */
    if (preProcess->inputFormat <= 3)
        regs->inputImageFormat = preProcess->inputFormat;   /* YUV */
    else if (preProcess->inputFormat <= 5)
        regs->inputImageFormat = 4;                         /* 16-bit RGB */
    else if (preProcess->inputFormat <= 7)
        regs->inputImageFormat = 5;                         /* 15-bit RGB */
    else if (preProcess->inputFormat <= 9)
        regs->inputImageFormat = 6;                         /* 12-bit RGB */
    else
        regs->inputImageFormat = 7;                         /* YUV422 planar */

    regs->inputImageRotation = preProcess->rotation;

    /* source image setup, size and fill */
    width = preProcess->lumWidth;
    height = preProcess->lumHeight;
    if(preProcess->rotation)
    {
        u32 tmp;

        tmp = width;
        width = height;
        height = tmp;
    }

    /* Set mandatory input parameters in asic structure */
    regs->mbsInRow = (width + 15) / 16;
    regs->mbsInCol = (height + preProcess->mbHeight-1) / preProcess->mbHeight;
    regs->pixelsOnRow = stride;

    /* Set the overfill values */
    if(width & 0x0F)
        regs->xFill = (16 - (width & 0x0F)) / 4;
    else
        regs->xFill = 0;

    ASSERT(preProcess->mbHeight == 8 || preProcess->mbHeight == 16);

    if(height & (preProcess->mbHeight-1))
        regs->yFill = preProcess->mbHeight - (height & (preProcess->mbHeight-1));
    else
        regs->yFill = 0;

    /* video stabilization */
    if(regs->codingType != ASIC_JPEG && preProcess->videoStab != 0)
    {
        regs->vsMode = 2;
    }
    else
    {
        regs->vsMode = 0;
    }

#ifdef TRACE_PREPROCESS
    EncTracePreProcess(preProcess);
#endif

    return;
}

/*------------------------------------------------------------------------------

	EncSetColorConversion

	Set color conversion coefficients and RGB input mask

	Input	asic	Pointer to asicData_s structure
		preProcess Pointer to preProcess_s structure.

------------------------------------------------------------------------------*/
void EncSetColorConversion(preProcess_s * preProcess, asicData_s * asic)
{
    regValues_s *regs;

    ASSERT(asic != NULL && preProcess != NULL);

    regs = &asic->regs;

    switch (preProcess->colorConversionType)
    {
        case 0:         /* BT.601 */
        default:
            /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
             * Cb = 0.5647 (B - Y) + 128
             * Cr = 0.7132 (R - Y) + 128
             */
            preProcess->colorConversionType = 0;
            regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 19589;
            regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 38443;
            regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 7504;
            regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 37008;
            regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 46740;
            break;

        case 1:         /* BT.709 */
            /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
             * Cb = 0.5389 (B - Y) + 128
             * Cr = 0.6350 (R - Y) + 128
             */
            regs->colorConversionCoeffA = preProcess->colorConversionCoeffA = 13933;
            regs->colorConversionCoeffB = preProcess->colorConversionCoeffB = 46871;
            regs->colorConversionCoeffC = preProcess->colorConversionCoeffC = 4732;
            regs->colorConversionCoeffE = preProcess->colorConversionCoeffE = 35317;
            regs->colorConversionCoeffF = preProcess->colorConversionCoeffF = 41615;
            break;

        case 2:         /* User defined */
            /* Limitations for coefficients: A+B+C <= 65536 */
            regs->colorConversionCoeffA = preProcess->colorConversionCoeffA;
            regs->colorConversionCoeffB = preProcess->colorConversionCoeffB;
            regs->colorConversionCoeffC = preProcess->colorConversionCoeffC;
            regs->colorConversionCoeffE = preProcess->colorConversionCoeffE;
            regs->colorConversionCoeffF = preProcess->colorConversionCoeffF;
    }

    /* Setup masks to separate R, G and B from RGB */
    switch (preProcess->inputFormat)
    {
        case 4: /* RGB565 */
            regs->rMaskMsb = 15;
            regs->gMaskMsb = 10;
            regs->bMaskMsb = 4;
            break;
        case 5: /* BGR565 */
            regs->bMaskMsb = 15;
            regs->gMaskMsb = 10;
            regs->rMaskMsb = 4;
            break;
        case 6: /* RGB555 */
            regs->rMaskMsb = 14;
            regs->gMaskMsb = 9;
            regs->bMaskMsb = 4;
            break;
        case 7: /* BGR555 */
            regs->bMaskMsb = 14;
            regs->gMaskMsb = 9;
            regs->rMaskMsb = 4;
            break;
        case 8: /* RGB444 */
            regs->rMaskMsb = 11;
            regs->gMaskMsb = 7;
            regs->bMaskMsb = 3;
            break;
        case 9: /* BGR444 */
            regs->bMaskMsb = 11;
            regs->gMaskMsb = 7;
            regs->rMaskMsb = 3;
            break;
        default:
            /* No masks needed for YUV format */
            regs->rMaskMsb = regs->gMaskMsb = regs->bMaskMsb = 0;
    }
}


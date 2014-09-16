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
-
-  Description : Video stabilization standalone control
-
--------------------------------------------------------------------------------
-
-  Version control information, please leave untouched.
-
-  $RCSfile: vidstabinternal.c,v $
-  $Revision: 1.8 $
-  $Date: 2009/09/15 11:36:21 $
-
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "vidstabcommon.h"
#include "vidstabinternal.h"
#include "vidstabcfg.h"
#include "ewl.h"

#ifdef ASIC_WAVE_TRACE_TRIGGER
extern i32 trigger_point;    /* picture which will be traced */
#endif

/* Mask fields */
#define mask_2b         (u32)0x00000003
#define mask_3b         (u32)0x00000007
#define mask_4b         (u32)0x0000000F
#define mask_5b         (u32)0x0000001F
#define mask_6b         (u32)0x0000003F
#define mask_11b        (u32)0x000007FF
#define mask_14b        (u32)0x00003FFF
#define mask_16b        (u32)0x0000FFFF

#define HSWREG(n)       ((n)*4)

/*------------------------------------------------------------------------------
    Function name   : VSCheckInput
    Description     : 
    Return type     : i32 
    Argument        : const VideoStbParam * param
------------------------------------------------------------------------------*/
i32 VSCheckInput(const VideoStbParam * param)
{
    /* Input picture minimum dimensions */
    if((param->inputWidth < 104) || (param->inputHeight < 104))
        return 1;

    /* Stabilized picture minimum  values */
    if((param->stabilizedWidth < 96) || (param->stabilizedHeight < 96))
        return 1;

    /* Stabilized dimensions multiple of 4 */
    if(((param->stabilizedWidth & 3) != 0) ||
       ((param->stabilizedHeight & 3) != 0))
        return 1;

    /* Edge >= 4 pixels */
    if((param->inputWidth < (param->stabilizedWidth + 8)) ||
       (param->inputHeight < (param->stabilizedHeight + 8)))
        return 1;

    /* stride 8 multiple */
    if((param->stride < param->inputWidth) || (param->stride & 7) != 0)
        return 1;

    /* input format */
    if(param->format > VIDEOSTB_BGR444)
    {
        return 1;
    }

    return 0;
}

/*------------------------------------------------------------------------------
    Function name   : VSInitAsicCtrl
    Description     : 
    Return type     : void 
    Argument        : VideoStb * pVidStab
------------------------------------------------------------------------------*/
void VSInitAsicCtrl(VideoStb * pVidStab)
{
    RegValues *val = &pVidStab->regval;
    u32 *regMirror = pVidStab->regMirror;

    ASSERT(pVidStab != NULL);

    /* Initialize default values from defined configuration */
    val->asicCfgReg =
        ((VS7280_AXI_WRITE_ID & (255)) << 24) |
        ((VS7280_AXI_READ_ID & (255)) << 16) |
        ((VS7280_BURST_LENGTH & (63)) << 8) |
        ((VS7280_BURST_INCR_TYPE_ENABLED & (1)) << 6) |
        ((VS7280_BURST_DATA_DISCARD_ENABLED & (1)) << 5) |
        ((VS7280_ASIC_CLOCK_GATING_ENABLED & (1)) << 4) |
        ((VS7280_INPUT_ENDIAN_WIDTH & (1)) << 2) | (VS7280_INPUT_ENDIAN & (1));

    val->irqDisable = VS7280_IRQ_DISABLE;
    val->rwStabMode = ASIC_VS_MODE_ALONE;

    (void) EWLmemset(regMirror, 0, sizeof(regMirror));
}

/*------------------------------------------------------------------------------
    Function name   : VSSetupAsicAll
    Description     : 
    Return type     : void 
    Argument        : VideoStb * pVidStab
------------------------------------------------------------------------------*/
void VSSetupAsicAll(VideoStb * pVidStab)
{

    const void *ewl = pVidStab->ewl;
    RegValues *val = &pVidStab->regval;
    u32 *regMirror = pVidStab->regMirror;

    regMirror[1] = ((val->irqDisable & 1) << 1);    /* clear IRQ status */

    regMirror[2] = val->asicCfgReg; /* ASIC system configuration */

    /* Input picture buffers */
    regMirror[11] = val->inputLumBase;

    /* Common control register */
    regMirror[14] = (val->mbsInRow << 19) | (val->mbsInCol << 9);

    /* PreP control */
    regMirror[15] =
        (val->inputLumaBaseOffset << 25) |
        (val->pixelsOnRow << 11) |
        (val->xFill << 9) | (val->yFill << 5) | (val->inputImageFormat << 2);

    regMirror[39] = val->rwNextLumaBase;
    regMirror[40] = val->rwStabMode << 30;

    regMirror[53] = ((val->colorConversionCoeffB & mask_16b) << 16) |
                     (val->colorConversionCoeffA & mask_16b);

    regMirror[54] = ((val->colorConversionCoeffE & mask_16b) << 16) |
                     (val->colorConversionCoeffC & mask_16b);

    regMirror[55] = ((val->bMaskMsb & mask_5b) << 26) |
                    ((val->gMaskMsb & mask_5b) << 21) |
                    ((val->rMaskMsb & mask_5b) << 16) |
                     (val->colorConversionCoeffF & mask_16b);

#ifdef ASIC_WAVE_TRACE_TRIGGER
    if(val->vop_count++ == trigger_point)
    {
        /* logic analyzer triggered by writing to the ID reg */
        EWLWriteReg(ewl, 0x00, ~0);
    }
#endif

    {
        i32 i;

        for(i = 1; i <= 55; i++)
        {
            if(i != 26)
                EWLWriteReg(ewl, HSWREG(i), regMirror[i]);
        }
    }

    /* enable bit is written last */
    regMirror[14] |= ASIC_STATUS_ENABLE;

    EWLEnableHW(ewl, HSWREG(14), regMirror[14]);
}

/*------------------------------------------------------------------------------
    Function name   : CheckAsicStatus
    Description     : 
    Return type     : i32 
    Argument        : u32 status
------------------------------------------------------------------------------*/
i32 CheckAsicStatus(u32 status)
{
    i32 ret;

    if(status & ASIC_STATUS_HW_RESET)
    {
        ret = VIDEOSTB_HW_RESET;
    }
    else if(status & ASIC_STATUS_FRAME_READY)
    {
        ret = VIDEOSTB_OK;
    }
    else
    {
        ret = VIDEOSTB_HW_BUS_ERROR;
    }

    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : VSWaitAsicReady
    Description     : 
    Return type     : i32 
    Argument        : VideoStb * pVidStab
------------------------------------------------------------------------------*/
i32 VSWaitAsicReady(VideoStb * pVidStab)
{
    const void *ewl = pVidStab->ewl;
    u32 *regMirror = pVidStab->regMirror;
    i32 ret;

    /* Wait for IRQ */
    ret = EWLWaitHwRdy(ewl);

    if(ret != EWL_OK)
    {
        if(ret == EWL_ERROR)
        {
            /* IRQ error => Stop and release HW */
            ret = VIDEOSTB_SYSTEM_ERROR;
        }
        else    /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
        {
            /* IRQ Timeout => Stop and release HW */
            ret = VIDEOSTB_HW_TIMEOUT;
        }

        EWLEnableHW(ewl, HSWREG(14), 0);    /* make sure ASIC is OFF */
    }
    else
    {
        i32 i;

        regMirror[1] = EWLReadReg(ewl, HSWREG(1));  /* IRQ status */
        for(i = 40; i <= 50; i++)
        {
            regMirror[i] = EWLReadReg(ewl, HSWREG(i));  /* VS results */
        }

        ret = CheckAsicStatus(regMirror[1]);
    }

    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : VSSetCropping
    Description     : 
    Return type     : void 
    Argument        : VideoStb * pVidStab
    Argument        : u32 currentPictBus
    Argument        : u32 nextPictBus
------------------------------------------------------------------------------*/
void VSSetCropping(VideoStb * pVidStab, u32 currentPictBus, u32 nextPictBus)
{
    u32 byteOffsetCurrent;
    u32 width, height;
    RegValues *regs;

    ASSERT(pVidStab != NULL && currentPictBus != 0 && nextPictBus != 0);

    regs = &pVidStab->regval;

    regs->inputLumBase = currentPictBus;
    regs->rwNextLumaBase = nextPictBus;

    /* RGB conversion coefficients for RGB input */
    if (pVidStab->yuvFormat >= 4) {
        regs->colorConversionCoeffA = 19589;
        regs->colorConversionCoeffB = 38443;
        regs->colorConversionCoeffC = 7504;
        regs->colorConversionCoeffE = 37008;
        regs->colorConversionCoeffF = 46740;
    }

    /* Setup masks to separate R, G and B from RGB */
    switch (pVidStab->yuvFormat)
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

    if (pVidStab->yuvFormat <= 3)
        regs->inputImageFormat = pVidStab->yuvFormat;       /* YUV */
    else if (pVidStab->yuvFormat <= 5)
        regs->inputImageFormat = 4;                         /* 16-bit RGB */
    else if (pVidStab->yuvFormat <= 7)
        regs->inputImageFormat = 5;                         /* 15-bit RGB */
    else
        regs->inputImageFormat = 6;                         /* 12-bit RGB */


    regs->pixelsOnRow = pVidStab->stride;

    /* cropping */
    if(regs->inputImageFormat < 2)
    {
        /* Current image position */
        byteOffsetCurrent = pVidStab->data.stabOffsetY;
        byteOffsetCurrent *= pVidStab->stride;
        byteOffsetCurrent += pVidStab->data.stabOffsetX;
    }
    else
    {
        /* Current image position */
        byteOffsetCurrent = pVidStab->data.stabOffsetY;
        byteOffsetCurrent *= pVidStab->stride;
        byteOffsetCurrent += pVidStab->data.stabOffsetX;
        byteOffsetCurrent *= 2;
    }

    regs->inputLumBase += (byteOffsetCurrent & (~7));
    regs->inputLumaBaseOffset = byteOffsetCurrent & 7;

    /* next picture's offset same as above */
    regs->rwNextLumaBase += (byteOffsetCurrent & (~7));

    /* source image setup, size and fill */
    width = pVidStab->data.stabilizedWidth;
    height = pVidStab->data.stabilizedHeight;

    /* Set stabilized picture dimensions */
    regs->mbsInRow = (width + 15) / 16;
    regs->mbsInCol = (height + 15) / 16;

    return;
}

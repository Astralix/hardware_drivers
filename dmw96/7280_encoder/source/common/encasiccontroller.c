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
--  Description : ASIC low level controller
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: encasiccontroller.c,v $
--  $Revision: 1.22 $
--  $Date: 2009/04/21 07:33:58 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/
#include <stdio.h>

#include "enccommon.h"
#include "encasiccontroller.h"
#include "encpreprocess.h"
#include "ewl.h"

#ifdef MPEG4_HW_RLC_MODE_ENABLED
#include "encbuffering.h"
#endif

/*------------------------------------------------------------------------------
    External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

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

/* MPEG-4 motion estimation parameters */
static const i32 mpeg4InterFavor[32] = { 0,
    0, 120, 140, 160, 200, 240, 280, 340, 400, 460, 520, 600, 680,
    760, 840, 920, 1000, 1080, 1160, 1240, 1320, 1400, 1480, 1560,
    1640, 1720, 1800, 1880, 1960, 2040, 2120
};

static const u32 mpeg4DiffMvPenalty[32] = { 0,
    4, 5, 6, 7, 8, 9, 10, 11, 14, 17, 20, 23, 27, 31, 35, 38, 41,
    44, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59
};

/* H.264 motion estimation parameters */
static const u32 h264InterFavor[52] = {
    40, 40, 41, 42, 43, 44, 45, 48, 51, 53, 55, 60, 62, 67, 69, 72,
    78, 84, 90, 96, 110, 120, 135, 152, 170, 189, 210, 235, 265,
    297, 335, 376, 420, 470, 522, 572, 620, 670, 724, 770, 820,
    867, 915, 970, 1020, 1076, 1132, 1180, 1230, 1275, 1320, 1370
};

static const u32 h264PrevModeFavor[52] = {
    7, 7, 8, 8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 24, 25, 27, 29, 30, 32, 34, 36, 38, 41, 43, 46,
    49, 51, 55, 58, 61, 65, 69, 73, 78, 82, 87, 93, 98, 104, 110,
    117, 124, 132, 140
};

static const u32 h264DiffMvPenalty[52] =
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 8, 9, 10,
    11, 13, 14, 16, 18, 20, 23, 25, 29, 32, 36, 40, 45, 51, 57, 64,
    72, 81, 91
};

/* JPEG QUANT table order */
static const u32 qpReorderTable[64] =
    { 0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
      2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
      4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
      6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

/*------------------------------------------------------------------------------
    Local function prototypes
------------------------------------------------------------------------------*/

#ifdef MPEG4_HW_RLC_MODE_ENABLED
static u32 EncAsicGetMbCount(const void *ewl);
#endif

#ifdef TRACE_REGS
#define LOG_NDEBUG 0
#define LOG_TAG "encasiccontroller"
#include <cutils/log.h>
void EncTraceRegs(const void *ewl, u32 mbNum, u32 startReg, u32 endReg)
{
	int i;
	FILE *fRegs = fopen("/regs/sw_reg.trc", "a+");

	if(fRegs == NULL)
		fRegs = stderr;

	if (mbNum == 0) {
		fprintf(fRegs, "\n=== NEW VOP ====================================\n");
	}

	fprintf(fRegs, "\nASIC register dump, mb=%4d\n\n", mbNum);

	/* Dump registers */
	for(i = startReg; i <= endReg; i += 4) {
		LOGV("Reg 0x%02x    0x%08x\n", i, EWLReadReg(ewl, i));
		fprintf(fRegs, "Reg 0x%02x    0x%08x\n", i, EWLReadReg(ewl, i));
	}

	fprintf(fRegs, "\n");

	fclose(fRegs);
}
#endif

/*------------------------------------------------------------------------------
    Initialize empty structure with default values.
------------------------------------------------------------------------------*/
i32 EncAsicControllerInit(asicData_s * asic)
{
    ASSERT(asic != NULL);

    /* Initialize default values from defined configuration */
    asic->regs.irqDisable = ENC7280_IRQ_DISABLE;

    asic->regs.asicCfgReg =
        ((ENC7280_AXI_WRITE_ID & (255)) << 24) |
        ((ENC7280_AXI_READ_ID & (255)) << 16) |
        ((ENC7280_OUTPUT_ENDIAN_16 & (1)) << 15) |
        ((ENC7280_INPUT_ENDIAN_16 & (1)) << 14) |
        ((ENC7280_BURST_LENGTH & (63)) << 8) |
        ((ENC7280_BURST_INCR_TYPE_ENABLED & (1)) << 6) |
        ((ENC7280_BURST_DATA_DISCARD_ENABLED & (1)) << 5) |
        ((ENC7280_ASIC_CLOCK_GATING_ENABLED & (1)) << 4) |
        ((ENC7280_OUTPUT_ENDIAN_WIDTH & (1)) << 3) |
        ((ENC7280_INPUT_ENDIAN_WIDTH & (1)) << 2) |
        ((ENC7280_OUTPUT_ENDIAN & (1)) << 1) | (ENC7280_INPUT_ENDIAN & (1));

    /* Initialize default values */
    asic->regs.roundingCtrl = 1;
    asic->regs.cpDistanceMbs = 0;

    /* User must set these */
    asic->regs.inputLumBase = 0;
    asic->regs.inputCbBase = 0;
    asic->regs.inputCrBase = 0;

    asic->internalImageLuma[0].virtualAddress = NULL;
    asic->internalImageChroma[0].virtualAddress = NULL;
    asic->internalImageLuma[1].virtualAddress = NULL;
    asic->internalImageChroma[1].virtualAddress = NULL;
    asic->controlBase.virtualAddress = NULL;

    asic->swControl = NULL;

#ifdef ASIC_WAVE_TRACE_TRIGGER
    asic->regs.vop_count = 0;
#endif

    /* get ASIC ID value */
    asic->regs.asicHwId = EncAsicGetId(asic->ewl);

/* we do NOT reset hardware at this point because */
/* of the multi-instance support                  */

    return ENCHW_OK;
}

#ifdef MPEG4_HW_RLC_MODE_ENABLED
/*------------------------------------------------------------------------------

    EncAsicMemAlloc

    Allocate HW/SW shared memory

    Input:
        asic        asicData structure
        width       width of encoded image, multiple of four
        height      height of encoded image
        rlcBufSize  size of the RLC buffer

    Output:
        asic        base addresses point to allocated memory areas

    Return:
        ENCHW_OK        Success.
        ENCHW_NOK       Error: memory allocation failed, no memories allocated
                        and EWL instance released

------------------------------------------------------------------------------*/
i32 EncAsicMemAlloc(asicData_s * asic, u32 width, u32 height, u32 rlcBufSize)
{
    u32 size, mbTotal;
    u32 mbControl;
    regValues_s *regs;

    ASSERT(asic != NULL);
    ASSERT(width != 0);
    ASSERT(height != 0);
    ASSERT((height % 2) == 0);
    ASSERT((width % 4) == 0);

    regs = &asic->regs;

    width = (width + 15) / 16;
    height = (height + 15) / 16;

    mbTotal = width * height;

    /* Allocate RLC buffers */
    size = (rlcBufSize + 7) & (~7); /* 8 byte multiple */

    /* Maximum size for RLC buffer: limited by ASIC
     * Minimum size for RLC buffer: ENC_BUFFER_LIMIT + 64 bytes */
    if(size > (1 << 24))
        size = (1 << 24);
    if(size < ENC7280_BUFFER_LIMIT + 64)
        size = ENC7280_BUFFER_LIMIT + 64;

    if(EncAllocateBuffers(asic, ENC7280_BUFFER_AMOUNT, size, mbTotal) !=
       ENCHW_OK)
    {
        EncAsicMemFree(asic);
        return ENCHW_NOK;
    }

    /* Allocate HW output macroblock control table */
    mbControl = mbTotal * 6 * 4;

    if(EWLMallocLinear(asic->ewl, mbControl, &asic->controlBase) != EWL_OK)
    {
        EncAsicMemFree(asic);
        return ENCHW_NOK;
    }

    regs->controlBase = asic->controlBase.busAddress;

    /* internal reference frames */
    {
        /* The sizes of the memories */
        u32 internalImageLumaSize = mbTotal * (16 * 16);

        u32 internalImageChromaSize = mbTotal * (2 * 8 * 8);

        /* Allocate internal image, not needed for JPEG */
        if(EWLMallocRefFrm(asic->ewl, internalImageLumaSize,
                           &asic->internalImageLuma[0]) != EWL_OK)
        {
            EncAsicMemFree(asic);
            return ENCHW_NOK;
        }

        if(EWLMallocRefFrm(asic->ewl, internalImageChromaSize,
                           &asic->internalImageChroma[0]) != EWL_OK)
        {
            EncAsicMemFree(asic);
            return ENCHW_NOK;
        }

        /* Allocate internal image, not needed for JPEG */
        if(EWLMallocRefFrm(asic->ewl, internalImageLumaSize,
                           &asic->internalImageLuma[1]) != EWL_OK)
        {
            EncAsicMemFree(asic);
            return ENCHW_NOK;
        }

        if(EWLMallocRefFrm(asic->ewl, internalImageChromaSize,
                           &asic->internalImageChroma[1]) != EWL_OK)
        {
            EncAsicMemFree(asic);
            return ENCHW_NOK;
        }

        /* Set base addresses to the registers */
        regs->internalImageLumBaseW = asic->internalImageLuma[0].busAddress;
        regs->internalImageChrBaseW = asic->internalImageChroma[0].busAddress;
        regs->internalImageLumBaseR = asic->internalImageLuma[1].busAddress;
        regs->internalImageChrBaseR = asic->internalImageChroma[1].busAddress;
    }

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    EncAsicMemFree

    Free HW/SW shared memory

------------------------------------------------------------------------------*/
void EncAsicMemFree(asicData_s * asic)
{
    ASSERT(asic != NULL);

    if(asic->internalImageLuma[0].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageLuma[0]);

    if(asic->internalImageChroma[0].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageChroma[0]);

    if(asic->internalImageLuma[1].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageLuma[1]);

    if(asic->internalImageChroma[1].virtualAddress != NULL)
        EWLFreeRefFrm(asic->ewl, &asic->internalImageChroma[1]);

    if(asic->controlBase.virtualAddress != NULL)
        EWLFreeLinear(asic->ewl, &asic->controlBase);

    asic->internalImageLuma[0].virtualAddress = NULL;
    asic->internalImageChroma[0].virtualAddress = NULL;
    asic->internalImageLuma[1].virtualAddress = NULL;
    asic->internalImageChroma[1].virtualAddress = NULL;

    asic->controlBase.virtualAddress = NULL;

    EncFreeBuffers(asic);
}

/*------------------------------------------------------------------------------

    EncAsicMbType

    Return the macroblock type from the control field

------------------------------------------------------------------------------*/
asicMbType_e EncAsicMbType(const u32 * control)
{
    u32 controlWord;
    u32 type;

    controlWord = control[0];

    type = (controlWord >> 29);

    ASSERT(type < 5);

    return (asicMbType_e) type;
}

/*------------------------------------------------------------------------------

    EncAsicQp

    Return the qp from the control field

------------------------------------------------------------------------------*/
i32 EncAsicQp(const u32 * control)
{
    u32 word = *control;

    word = (word << 8) >> 24;

    ASSERT(word < 32 && word > 0);

    return (i32) word;
}

/*------------------------------------------------------------------------------

    EncAsicMv

    Stores the X or Y motion vectors from the control table to an array

------------------------------------------------------------------------------*/
void EncAsicMv(const u32 * control, i8 mv[4], i32 xy)
{
    /* Motion vectors are stored as 8-bit 2's complement */
    if(xy == 0) /* X-vectors */
    {
        i32 tmp1 = control[2];
        i32 tmp2 = control[3];

        *mv++ = (i8) (tmp1 >> 24);
        *mv++ = (i8) ((tmp1 << 16) >> 24);
        *mv++ = (i8) (tmp2 >> 24);
        *mv++ = (i8) ((tmp2 << 16) >> 24);
    }
    else    /* Y-vectors */
    {
        i32 tmp1 = control[2];
        i32 tmp2 = control[3];

        *mv++ = (i8) ((tmp1 << 8) >> 24);
        *mv++ = (i8) ((tmp1 << 24) >> 24);
        *mv++ = (i8) ((tmp2 << 8) >> 24);
        *mv++ = (i8) ((tmp2 << 24) >> 24);
    }
}

/*------------------------------------------------------------------------------

    EncAsicDc

    Read the DC components of a macroblock from the control table to an array

------------------------------------------------------------------------------*/
void EncAsicDc(i32 * mbDc, const u32 * control)
{
    i32 i;

    control += (8 / 4);

    for(i = 3; i > 0; i--)
    {
        i32 tmp = (i32) (*control++);

        *mbDc++ = (i32) (tmp >> 16);
        *mbDc++ = (i32) ((tmp << 16) >> 16);
    }
}

/*------------------------------------------------------------------------------

    EncAsicRlcCount

    Read the RLC count for each block of a macroblock from the HW output table 
    to an array and store the start pointers of each block RLC data.

    OUT mbRlc           pointers to each block RLC data
    OUT mbRlcCount      amount of RLC words for each block
    IN rlcData          start of the macroblock RLC data
    IN mbControl        pointer to the macroblock control data
    
    Return the size of the RLC data in bytes, -1 for error.

------------------------------------------------------------------------------*/
i32 EncAsicRlcCount(const u32 * mbRlc[6], i32 mbRlcCount[6],
                    const u32 * rlcData, const u32 * mbControl)
{
    i32 i;
    i32 mbRlcSize = 0;

    /* Get the RLC amount for each block */
    u32 tmp = *mbControl++;

    mbRlcCount[0] = (tmp >> 8) & 0xFF;
    mbRlcCount[1] = tmp & 0xFF;

    tmp = *mbControl++;

    mbRlcCount[2] = (tmp >> 24);
    mbRlcCount[3] = (tmp >> 16) & 0xFF;
    mbRlcCount[4] = (tmp >> 8) & 0xFF;
    mbRlcCount[5] = tmp & 0xFF;

    /* Each RLC word uses 32-bits and each block is 64-bit aligned
     * First block RLC data start from the given pointer, the following
     * block's RLC data start depends on the HW output alignment */
    for(i = 6; i > 0; i--)
    {
        tmp = *mbRlcCount++;

        if(tmp > 64)
            return -1;

        *mbRlc++ = rlcData + mbRlcSize;

        /* Calculate the amount of bytes in the HW output RLC table used
         * for current macroblock. */
        mbRlcSize += tmp;
    }

    /* Align the MB RLC boundary to 64-bits */
    mbRlcSize *= 4;
    mbRlcSize += mbRlcSize & 4;

    return mbRlcSize;
}

#endif /* MPEG4_HW_RLC_MODE_ENABLED */

/*------------------------------------------------------------------------------

    EncAsicSetQuantTable

    Set new jpeg quantization table to be used by ASIC

------------------------------------------------------------------------------*/
void EncAsicSetQuantTable(asicData_s * asic,
                          const u8 * lumTable, const u8 * chTable)
{
    i32 i;

    ASSERT(lumTable);
    ASSERT(chTable);

    for(i = 0; i < 64; i++)
    {
        asic->regs.quantTable[i] = lumTable[qpReorderTable[i]];
    }
    for(i = 0; i < 64; i++)
    {
        asic->regs.quantTable[64 + i] = chTable[qpReorderTable[i]];
    }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetId(const void *ewl)
{
    return EWLReadReg(ewl, 0x0);
}

/*------------------------------------------------------------------------------
    When the frame is successfully encoded the internal image is recycled
    so that during the next frame the current internal image is read and
    the new reconstructed image is written two macroblock rows earlier.
------------------------------------------------------------------------------*/
void EncAsicRecycleInternalImage(regValues_s * val)
{
    /* The next encoded frame will use current reconstructed frame as */
    /* the reference */
    u32 tmp;

    tmp = val->internalImageLumBaseW;
    val->internalImageLumBaseW = val->internalImageLumBaseR;
    val->internalImageLumBaseR = tmp;

    tmp = val->internalImageChrBaseW;
    val->internalImageChrBaseW = val->internalImageChrBaseR;
    val->internalImageChrBaseR = tmp;
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void CheckRegisterValues(regValues_s * val)
{
    u32 i;

    ASSERT(val->irqDisable <= 1);
    ASSERT(val->rlcLimitSpace / 2 < (1 << 20));
    ASSERT(val->irqInterval <= 4095);
    ASSERT(val->mbsInCol <= 1023);
    ASSERT(val->mbsInRow <= 511);
    ASSERT(val->filterDisable <= 2);
    ASSERT(val->qp <= 51);
    ASSERT(val->constrainedIntraPrediction <= 1);
    ASSERT(val->roundingCtrl <= 1);
    ASSERT(val->frameCodingType <= 1);
    ASSERT(val->codingType <= 3);
    ASSERT(val->pixelsOnRow >= 16 && val->pixelsOnRow <= 8192); /* max input for cropping */
    ASSERT(val->xFill <= 3);
    ASSERT(val->yFill <= 14 && ((val->yFill & 0x01) == 0));
    ASSERT(val->sliceAlphaOffset >= -6 && val->sliceAlphaOffset <= 6);
    ASSERT(val->sliceBetaOffset >= -6 && val->sliceBetaOffset <= 6);
    ASSERT(val->chromaQpIndexOffset >= -12 && val->chromaQpIndexOffset <= 12);
    ASSERT(val->sliceSizeMbRows <= 63);
    ASSERT(val->inputImageFormat <= 7);
    ASSERT(val->inputImageRotation <= 2);
    ASSERT(val->cpDistanceMbs >= 0 && val->cpDistanceMbs <= 2047);
    ASSERT(val->vpMbBits <= 15);
    ASSERT(val->vpSize <= 65535);

    if(val->codingType != ASIC_JPEG && val->cpTarget != NULL)
    {
        ASSERT(val->cpTargetResults != NULL);

        for(i = 0; i < 10; i++)
        {
            ASSERT(*val->cpTarget < (1 << 16));
        }

        ASSERT(val->targetError != NULL);

        for(i = 0; i < 7; i++)
        {
            ASSERT((*val->targetError) >= -32768 &&
                   (*val->targetError) < 32768);
        }

        ASSERT(val->deltaQp != NULL);

        for(i = 0; i < 7; i++)
        {
            ASSERT((*val->deltaQp) >= -8 && (*val->deltaQp) < 8);
        }
    }

    (void) val;
}

/*------------------------------------------------------------------------------
    Function name   : EncAsicFrameStart
    Description     : 
    Return type     : void 
    Argument        : const void *ewl
    Argument        : regValues_s * val
------------------------------------------------------------------------------*/
void EncAsicFrameStart(const void *ewl, regValues_s * val)
{
    u32 interFavor = 0, diffMvPenalty = 0, prevModeFavor = 0;

    /* Set the interrupt interval in macroblock rows (JPEG) or
     * in macroblocks (video) */
    if(val->codingType != ASIC_JPEG)
    {
        switch (val->codingType)
        {
        case ASIC_MPEG4:
        case ASIC_H263:
            interFavor = mpeg4InterFavor[val->qp];
            diffMvPenalty = mpeg4DiffMvPenalty[val->qp];
            break;
        case ASIC_H264:
            interFavor = h264InterFavor[val->qp];
            prevModeFavor = h264PrevModeFavor[val->qp];
            diffMvPenalty = h264DiffMvPenalty[val->qp];
            break;
        default:
            break;
        }
    }

    if(val->rlcOutMode != 0)
        val->irqInterval = ENC7280_IRQ_INTERVAL_FRAME_START;
    else
        val->irqInterval = 0;

    CheckRegisterValues(val);

    EWLmemset(val->regMirror, 0, sizeof(val->regMirror));

    /* encoder interrupt */
    val->regMirror[1] = (val->irqDisable << 1);

    /* system configuration */
    if(val->rlcOutMode != 0)
    {
        /* in RLC mode ASIC output always in BIG endian */
        val->asicCfgReg &= (~(1 << 1));
        val->asicCfgReg |= (ASIC_BIG_ENDIAN << 1);
    }
    else
    {
        val->asicCfgReg |= ((ENC7280_OUTPUT_ENDIAN & (1)) << 1);
    }

    val->regMirror[2] = val->asicCfgReg;

    /* output stream buffer or RLC buffer */
    if(val->rlcOutMode == 0)
    {
        val->regMirror[5] = val->outputStrmBase;

        if(val->codingType == ASIC_H264)
        {
            /* NAL size buffer or MB control buffer */
            val->regMirror[6] = val->sizeTblBase.nal;
            if(val->sizeTblBase.nal)
                val->sizeTblPresent = 1;
        }
        else if(val->codingType == ASIC_H263)
        {
            /* GOB size buffer or MB control buffer */
            val->regMirror[6] = val->sizeTblBase.gob;
            if(val->sizeTblBase.gob)
                val->sizeTblPresent = 1;
        }
        else
        {
            /* VP size buffer or MB control buffer */
            val->regMirror[6] = val->sizeTblBase.vp;
            if(val->sizeTblBase.vp)
                val->sizeTblPresent = 1;
        }
    }
    else
    {
        /* RLC mode IRQ interval */
        val->regMirror[4] = val->irqInterval;
        val->regMirror[5] = val->rlcBase;
        val->regMirror[6] = val->controlBase;
    }

    /* Video encoding reference picture buffers */
    if(val->codingType != ASIC_JPEG)
    {
        val->regMirror[7] = val->internalImageLumBaseR;
        val->regMirror[8] = val->internalImageChrBaseR;
        val->regMirror[9] = val->internalImageLumBaseW;
        val->regMirror[10] = val->internalImageChrBaseW;
    }

    /* Input picture buffers */
    val->regMirror[11] = val->inputLumBase;
    val->regMirror[12] = val->inputCbBase;
    val->regMirror[13] = val->inputCrBase;

    /* Common control register */
    val->regMirror[14] = (val->sizeTblPresent << 29) |
        (val->rlcOutMode << 28) |
        (val->mbsInRow << 19) |
        (val->mbsInCol << 9) |
        (val->frameCodingType << 3) | (val->codingType << 1);

    /* PreP control */
    val->regMirror[15] =
        (val->inputChromaBaseOffset << 28) |
        (val->inputLumaBaseOffset << 25) |
        (val->pixelsOnRow << 11) |
        (val->xFill << 9) |
        (val->yFill << 5) |
        (val->inputImageFormat << 2) | (val->inputImageRotation);

    /* H.264 control */
    val->regMirror[16] =
        (val->picInitQp << 26) |
        ((val->sliceAlphaOffset & mask_4b) << 22) |
        ((val->sliceBetaOffset & mask_4b) << 18) |
        ((val->chromaQpIndexOffset & mask_5b) << 13) |
        (val->sliceSizeMbRows << 7) |
        (val->filterDisable << 5) |
        (val->idrPicId << 1) | (val->constrainedIntraPrediction);

    val->regMirror[17] =
        (val->ppsId << 24) | (prevModeFavor << 16) | (val->intra16Favor);

    val->regMirror[18] = (val->h264Inter4x4Disabled << 17) |
        (val->h264StrmMode << 16) | val->frameNum;

    /* MPEG-4/H.263 control */
    {
        u32 swreg19 = (val->roundingCtrl << 27);

        if(val->codingType == ASIC_H263)
        {
            if(val->mbsInCol > 25)
            {
                swreg19 |= 1 << 26; /*  2 MB rows in GOB */
            }

            swreg19 |= val->gobFrameId << 24;
            swreg19 |= val->gobHeaderMask;
        }
        val->regMirror[19] = swreg19;

    }
    /* JPEG control */
    val->regMirror[20] = ((val->jpegMode << 25) |
                          (val->jpegSliceEnable << 24) |
                          (val->jpegRestartInterval << 16) |
                          (val->jpegRestartMarker));

    /* Motion Estimation control */
    val->regMirror[21] = (diffMvPenalty << 16) | interFavor;

    /* stream buffer limits */
    if(val->rlcOutMode == 0)
    {
        val->regMirror[22] = val->strmStartMSB;
        val->regMirror[23] = val->strmStartLSB;
        val->regMirror[24] = val->outputStrmSize;
    }
    else
    {
        val->regMirror[24] = (val->rlcLimitSpace / 2);
    }

    /* video encoding rate control */
    if(val->codingType != ASIC_JPEG)
    {
        val->regMirror[27] = (val->qp << 26) | (val->qpMax << 20) |
            (val->qpMin << 14) | (val->cpDistanceMbs);

        if(val->cpTarget != NULL)
        {
            val->regMirror[28] = (val->cpTarget[0] << 16) | (val->cpTarget[1]);
            val->regMirror[29] = (val->cpTarget[2] << 16) | (val->cpTarget[3]);
            val->regMirror[30] = (val->cpTarget[4] << 16) | (val->cpTarget[5]);
            val->regMirror[31] = (val->cpTarget[6] << 16) | (val->cpTarget[7]);
            val->regMirror[32] = (val->cpTarget[8] << 16) | (val->cpTarget[9]);

            val->regMirror[33] = ((val->targetError[0] & mask_16b) << 16) |
                (val->targetError[1] & mask_16b);
            val->regMirror[34] = ((val->targetError[2] & mask_16b) << 16) |
                (val->targetError[3] & mask_16b);
            val->regMirror[35] = ((val->targetError[4] & mask_16b) << 16) |
                (val->targetError[5] & mask_16b);

            val->regMirror[36] = ((val->deltaQp[0] & mask_4b) << 24) |
                ((val->deltaQp[1] & mask_4b) << 20) |
                ((val->deltaQp[2] & mask_4b) << 16) |
                ((val->deltaQp[3] & mask_4b) << 12) |
                ((val->deltaQp[4] & mask_4b) << 8) |
                ((val->deltaQp[5] & mask_4b) << 4) |
                (val->deltaQp[6] & mask_4b);
        }
    }

    /* Stream start offset */
    val->regMirror[37] = (val->firstFreeBit << 22);

    if(val->codingType != ASIC_JPEG)
    {
        val->regMirror[39] = val->vsNextLumaBase;
        val->regMirror[40] = val->vsMode << 30;
    }

    /* VP and HEC stuff */
    val->regMirror[51] = ((val->vopFcode & mask_3b) << 27) |
                         ((val->intraDcVlcThr & mask_3b) << 24) |
                         ((val->moduloTimeBase & mask_3b) << 21) |
                         ((val->hec & 1) << 20) |
                         ((val->vpMbBits & mask_4b) << 16) |
                          (val->vpSize & mask_16b);

    val->regMirror[52] = ((val->timeIncBits & mask_4b) << 16) |
                          (val->timeInc & mask_16b);

    val->regMirror[53] = ((val->colorConversionCoeffB & mask_16b) << 16) |
                          (val->colorConversionCoeffA & mask_16b);

    val->regMirror[54] = ((val->colorConversionCoeffE & mask_16b) << 16) |
                          (val->colorConversionCoeffC & mask_16b);

    val->regMirror[55] = ((val->bMaskMsb & mask_5b) << 26) |
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
            if(i != 26) /* swreg[26] is for 6280 JPEG QP table */
                EWLWriteReg(ewl, HSWREG(i), val->regMirror[i]);
        }
    }

    /* Write JPEG quantization tables to regs if needed (JPEG) */
    if(val->codingType == ASIC_JPEG)
    {
        i32 i = 0;

        for(i = 0; i < 128; i += 4)
        {
            /* swreg[64] to swreg[95] */
            EWLWriteReg(ewl, HSWREG(64 + (i/4)), (val->quantTable[i] << 24) |
                        (val->quantTable[i + 1] << 16) |
                        (val->quantTable[i + 2] << 8) |
                        (val->quantTable[i + 3]));
        }
    }

    /* Register with enable bit is written last */
    val->regMirror[14] |= ASIC_STATUS_ENABLE;

#ifdef TRACE_REGS
    EncTraceRegs(ewl, 0, 0, 0x70);
#endif


    EWLEnableHW(ewl, HSWREG(14), val->regMirror[14]);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicFrameContinue(const void *ewl, regValues_s * val)
{
    /* clear status bits, clear IRQ => HW restart */
    u32 status = val->regMirror[1];

    status &= (~ASIC_STATUS_ALL);
    status &= ~ASIC_IRQ_LINE;

    val->regMirror[1] = status;

    /* Set the interrupt interval in macroblock rows (JPEG) or
     * in macroblocks (video) */
    if(val->codingType == ASIC_JPEG)
    {
        val->irqInterval =
            (ENC7280_IRQ_INTERVAL + val->mbsInRow - 1) / val->mbsInRow;
    }
    else
    {
        val->irqInterval = ENC7280_IRQ_INTERVAL;
    }

    /*CheckRegisterValues(val); */

    /* Write only registers which may be updated mid frame */
    EWLWriteReg(ewl, HSWREG(24), (val->rlcLimitSpace / 2));
    EWLWriteReg(ewl, HSWREG(4), val->irqInterval);

    val->regMirror[5] = val->rlcBase;
    EWLWriteReg(ewl, HSWREG(5), val->regMirror[5]);

#ifdef TRACE_REGS
    EncTraceRegs(ewl, /*(val->status & 0xFFFF0000) >> 16*/ 0, 0, 0x70);
#endif

    /* Register with status bits is written last */
    EWLEnableHW(ewl, HSWREG(1), val->regMirror[1]);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicGetRegisters(const void *ewl, regValues_s * val)
{
    u32 word;

    if(val->rlcOutMode != 0)
    {
        val->rlcLimitSpace = EWLReadReg(ewl, HSWREG(24)) / 8;   /* bytes */
    }
    else
    {
        val->outputStrmSize = EWLReadReg(ewl, HSWREG(24)) / 8;  /* bytes */
    }

    if(val->codingType != ASIC_JPEG && val->cpTarget != NULL)
    {
        /* video coding with MB rate control ON */
        u32 i, reg = HSWREG(28);
        u32 cpt_prev = 0;
        u32 overflow = 0;

        for(i = 0; i < 10; i += 2, reg += 0x04)
        {
            u32 cpt;

            word = EWLReadReg(ewl, reg);    /* 2 results per reg */

            /* first result from reg */
            cpt = (word >> 16) * 4;

            /* detect any overflow */
            if(cpt < cpt_prev)
                overflow += (1 << 18);
            cpt_prev = cpt;

            val->cpTargetResults[i] = cpt + overflow;

            /* second result from same reg */
            cpt = (word & mask_16b) * 4;

            /* detect any overflow */
            if(cpt < cpt_prev)
                overflow += (1 << 18);
            cpt_prev = cpt;

            val->cpTargetResults[i + 1] = cpt + overflow;
        }
    }

    val->qpSum = EWLReadReg(ewl, HSWREG(25)) * 2;   /* from swreg25 */

    /* Non-zero coefficient count, 22-bits from swreg37 */
    val->rlcCount = EWLReadReg(ewl, HSWREG(37)) & 0x3FFFFF;

    /* get stabilization results if needed */
    if(val->vsMode != 0)
    {
        i32 i;

        for(i = 40; i <= 50; i++)
        {
            val->regMirror[i] = EWLReadReg(ewl, HSWREG(i));
        }
    }
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
void EncAsicStop(const void *ewl)
{
    EWLDisableHW(ewl, HSWREG(14), 0);
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetStatus(const void *ewl)
{
    return EWLReadReg(ewl, HSWREG(1));
}

#ifdef MPEG4_HW_RLC_MODE_ENABLED

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
u32 EncAsicGetMbCount(const void *ewl)
{
    return EWLReadReg(ewl, HSWREG(38));
}

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
i32 EncAsicCheckStatus(asicData_s * asic)
{
    i32 ret = ENCHW_OK;
    u32 hw_release = 0;

    regValues_s *regs = &asic->regs;

    u32 status = regs->regMirror[1];    /* Get the previous status */

    /* Read the HW status register if frame is not finished
     * When frame is finished we must not interfere with the HW anymore because
     * another codec can possibly be using it.
     * The status register could be buffered in EWL since it only changes 
     * at HW interrupts. */
    if((status & ASIC_STATUS_FRAME_READY) == 0)
    {
        status = EncAsicGetStatus(asic->ewl);
    }

    /* If status has changed since the last time */
    if(regs->regMirror[1] != status)
    {
        u32 lastHwMb;

        /* Store the current status to be used after next macroblock */
        regs->regMirror[1] = status;

        if((status & ASIC_STATUS_ERROR) || (status & ASIC_STATUS_HW_RESET))
        {
            /* frame encoding can't continue */
            EncSetHwError(&asic->buffering);

            ret = ENCHW_NOK;

            hw_release = 1;

            goto end;   /* go and release HW */
        }

        lastHwMb = EncAsicGetMbCount(asic->ewl);
        ASSERT(lastHwMb > 0);

        /* Update the index of last HW processed macroblock [0..n-1]
         * ASIC returns the amount of processed macroblocks [1..n] */
        EncSetHwMb(&asic->buffering, (lastHwMb - 1));

        if(status & ASIC_STATUS_BUFF_FULL)
        {
            ASSERT(lastHwMb != asic->regs.mbsInCol * asic->regs.mbsInRow);
            /* ASIC has RLC buffer full 
             * => we need to read the amount of data in the buffer */
            EncAsicGetRegisters(asic->ewl, &asic->regs);

            /* HW buffer is full and we know how much data is in the buffer
             * => mark the buffer full and store the amount of data */
            EncSetHwBufferFull(&asic->buffering, regs->rlcLimitSpace);

            /* ASIC will be restarted in EncNextState() if necessary */
        }
        else if(status & ASIC_STATUS_FRAME_READY)
        {
            ASSERT(lastHwMb == asic->regs.mbsInCol * asic->regs.mbsInRow);
            /* ASIC has finished the frame 
             * => Read the final information from ASIC registers */
            EncAsicGetRegisters(asic->ewl, &asic->regs);
            hw_release = 1;
        }
        else if(status & ASIC_STATUS_IRQ_INTERVAL)
        {
            ASSERT(lastHwMb != asic->regs.mbsInCol * asic->regs.mbsInRow);

            regs->regMirror[1] &= (~ASIC_STATUS_IRQ_INTERVAL);

#ifdef TRACE_REGS
    EncTraceRegs(asic->ewl, /*(val->status & 0xFFFF0000) >> 16*/ 0, 0, 0x70);
#endif

            EWLEnableHW(asic->ewl, HSWREG(1), regs->regMirror[1]);
        }
        else
        {
            ASSERT(0);  /* there is no other status defined */
        }

    }

  end:
    if(hw_release)
    {
        /* Release HW so that it can be used by other codecs */
        EWLReleaseHw(asic->ewl);
    }
    return ret;
}
#endif /* MPEG4_HW_RLC_MODE_ENABLED */

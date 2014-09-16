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
--  Abstract : Encoder setup according to a test vector
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "H264TestId.h"

#include <stdio.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void H264FrameQuantizationTest(h264Instance_s *inst);
static void H264SliceTest(h264Instance_s *inst);
static void H264MbQuantizationTest(h264Instance_s *inst);
static void H264FilterTest(h264Instance_s *inst);
static void H264UserDataTest(h264Instance_s *inst);
static void H264Intra16FavorTest(h264Instance_s *inst);
static void H264RgbInputMaskTest(h264Instance_s *inst);
static void H264NotCodedFrameTest(h264Instance_s *inst);


/*------------------------------------------------------------------------------

    TestID defines a test configuration for the encoder. If the encoder control
    software is compiled with INTERNAL_TEST flag the test ID will force the 
    encoder operation according to the test vector. 

    TestID  Description
    0       No action, normal encoder operation
    1       Frame quantization test, adjust qp for every frame, qp = 0..51
    2       Slice test, adjust slice amount for each frame
    6       Checkpoint quantization test.
            Checkpoints with minimum and maximum values. Gradual increase of
            checkpoint values.
    7       Filter test, set disableDeblocking and filterOffsets A and B
    12      User data test
    15      Intra16Favor test, set to maximum value
    19      RGB input mask test, set all values
    25      Not coded frame test


------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------

    H264ConfigureTestBeforeStream

    Function configures the encoder instance before starting stream encoding

------------------------------------------------------------------------------*/
void H264ConfigureTestBeforeStream(h264Instance_s * inst)
{
    ASSERT(inst);

    switch(inst->testId)
    {
        default:
            break;
    }

}

/*------------------------------------------------------------------------------

    H264ConfigureTestBeforeFrame

    Function configures the encoder instance before starting frame encoding

------------------------------------------------------------------------------*/
void H264ConfigureTestBeforeFrame(h264Instance_s * inst)
{
    ASSERT(inst);

    switch(inst->testId)
    {
        case 1:
            H264FrameQuantizationTest(inst);
            break;
        case 2:
            H264SliceTest(inst);
            break;
        case 6:
            H264MbQuantizationTest(inst);
            break;
        case 7:
            H264FilterTest(inst);
            break;
        case 12:
            H264UserDataTest(inst);
            break;
        case 15:
            H264Intra16FavorTest(inst);
            break;
        case 19:
            H264RgbInputMaskTest(inst);
            break;
        case 25:
            H264NotCodedFrameTest(inst);
            break;
        default:
            break;
    }

}

/*------------------------------------------------------------------------------
  H264QuantizationTest
------------------------------------------------------------------------------*/
void H264FrameQuantizationTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    /* Inter frame qp start zero */
    inst->rateControl.qpHdr = MIN(51, MAX(0, (vopNum-1)%52));

    printf("H264FrameQuantTest# qpHdr %d\n", inst->rateControl.qpHdr);
}

/*------------------------------------------------------------------------------
  H264SliceTest
------------------------------------------------------------------------------*/
void H264SliceTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;

    inst->slice.sliceSize = (inst->mbPerRow*vopNum)%inst->mbPerFrame;

    printf("H264SliceTest# sliceSize %d\n", inst->slice.sliceSize);
}

/*------------------------------------------------------------------------------
  H264QuantizationTest
  NOTE: ASIC resolution for wordCntTarget and wordError is value/4
------------------------------------------------------------------------------*/
void H264MbQuantizationTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;
    h264QpCtrl_s *qc = &inst->rateControl.qpCtrl;
    h264RateControl_s *rc = &inst->rateControl;

    i32 tmp, i;
    i32 bitCnt = 0xffff / 200;

    rc->qpMin = vopNum/4;
    rc->qpMax = 51 - vopNum/4;
    rc->qpLastCoded = rc->qpTarget = rc->qpHdr = 26;

    for (i = 0; i < CHECK_POINTS_MAX; i++) {
        if (vopNum < 2) {
            tmp = 1;
        } else if (vopNum < 4) {
            tmp = 0xffff;
        } else {
            tmp = (vopNum *  bitCnt)/4;
        }
        qc->wordCntTarget[i] = MIN(0xffff, tmp);
    }

    printf("H264MbQuantTest# wordCntTarget[] %d\n", qc->wordCntTarget[0]);

    for (i = 0; i < CTRL_LEVELS; i++) {
        if (vopNum < 2) {
            tmp = -0x8000;
        } else if (vopNum < 4) {
            tmp = 0x7fff;
        } else {
            tmp = (-bitCnt/2*vopNum + vopNum * i * bitCnt/5)/4;
        }
        qc->wordError[i] = MIN(0x7fff, MAX(-0x8000, tmp));

        tmp = -8 + i*3;
        if ((vopNum&4) == 0) {
            tmp = -tmp;
        }
        qc->qpChange[i] = MIN(0x7, MAX(-0x8, tmp));

        printf("                 wordError[%d] %d   qpChange[%d] %d\n",
                                i, qc->wordError[i], i, qc->qpChange[i]);
    }

}

/*------------------------------------------------------------------------------
  H264FilterTest
------------------------------------------------------------------------------*/
void H264FilterTest(h264Instance_s *inst)
{
    i32 vopNum = inst->frameCnt;
    slice_s *slice = &inst->slice; 

    slice->disableDeblocking = (vopNum/2)%3;
    if (vopNum == 0) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = 12;
    } else if (vopNum < 77) {
        if (vopNum%6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB -= 2;
        }
    } else if (vopNum == 78) {
        slice->filterOffsetA = -12;
        slice->filterOffsetB = -12;
    } else if (vopNum < 155) {
        if (vopNum%6 == 0) {
            slice->filterOffsetA += 2;
            slice->filterOffsetB += 2;
        }
    }

    printf("H264FilterTest# disableDeblock = %d, filterOffA = %i filterOffB = %i\n",
             slice->disableDeblocking, slice->filterOffsetA,
             slice->filterOffsetB);

}

/*------------------------------------------------------------------------------
  H264UserDataTest
------------------------------------------------------------------------------*/
void H264UserDataTest(h264Instance_s *inst)
{
    static u8 *userDataBuf = NULL;
    i32 userDataLength = (16 + inst->frameCnt*11) % 2048;
    i32 i;

    /* Allocate a buffer for user data, encoder reads data from this buffer
     * and writes it to the stream. This is never freed. */
    if (!userDataBuf)
        userDataBuf = (u8*)malloc(2048);

    if (!userDataBuf)
        return;

    for(i = 0; i < userDataLength; i++)
    {
        /* Fill user data buffer with ASCII symbols from 48 to 125 */
        userDataBuf[i] = 48 + i % 78;
    }

    /* Enable user data insertion */
    inst->rateControl.sei.userDataEnabled = ENCHW_YES;
    inst->rateControl.sei.pUserData = userDataBuf;
    inst->rateControl.sei.userDataSize = userDataLength;

    printf("H264UserDataTest# userDataSize %d\n", userDataLength);
}

/*------------------------------------------------------------------------------
  H264Intra16FavorTest
------------------------------------------------------------------------------*/
void H264Intra16FavorTest(h264Instance_s *inst)
{
    /* Force intra16 favor to maximum value */
    inst->asic.regs.intra16Favor = 65535;

    printf("H264Intra16FavorTest# intra16Favor %d\n", 
            inst->asic.regs.intra16Favor);
}

/*------------------------------------------------------------------------------
  H264RgbInputMaskTest
------------------------------------------------------------------------------*/
void H264RgbInputMaskTest(h264Instance_s *inst)
{
    i32 frameNum = inst->frameCnt;
    static u32 rMsb = 0;
    static u32 gMsb = 0;
    static u32 bMsb = 0;
    static u32 lsMask = 0;  /* Lowest possible mask position (3 or 4) */

    /* First frame normal
     * 1..13 step rMaskMsb values
     * 14..26 step gMaskMsb values
     * 27..39 step bMaskMsb values */
    if (frameNum == 0) {
        rMsb = inst->asic.regs.rMaskMsb;
        gMsb = inst->asic.regs.gMaskMsb;
        bMsb = inst->asic.regs.bMaskMsb;
        lsMask = MIN(rMsb, gMsb);
        lsMask = MIN(bMsb, lsMask);
    } else if (frameNum <= 13) {    /* frame 1..13, maskMsb 3..15 */
        inst->asic.regs.rMaskMsb = MAX(frameNum+2, lsMask);
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = bMsb;
    } else if (frameNum <= 26) {    /* frame 14..26, maskMsb 3..15 */
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = MAX(frameNum-13+2, lsMask);
        if (inst->asic.regs.inputImageFormat == 4)  /* RGB 565 special case */
            inst->asic.regs.gMaskMsb = MAX(frameNum-13+2, lsMask+1);
        inst->asic.regs.bMaskMsb = bMsb;
    } else if (frameNum <= 39) {    /* frame 27..39, maskMsb 3..15 */
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = MAX(frameNum-26+2, lsMask);
    } else {
        inst->asic.regs.rMaskMsb = rMsb;
        inst->asic.regs.gMaskMsb = gMsb;
        inst->asic.regs.bMaskMsb = bMsb;
    }

    printf("H264RgbInputMaskTest#  %d %d %d\n", inst->asic.regs.rMaskMsb,
            inst->asic.regs.gMaskMsb, inst->asic.regs.bMaskMsb);
}

/*------------------------------------------------------------------------------
  H264NotCodedFrameTest
------------------------------------------------------------------------------*/
void H264NotCodedFrameTest(h264Instance_s *inst)
{
    i32 frameNum = inst->frameCnt;

    /* Force some of the frames to be encoded as "not coded" frames */
    switch (frameNum%10)
    {
        case 1:
        case 3:
        case 4:
        case 8:
            printf("H264NotCodedFrameTest#\n");
            inst->codingType = (u32)H264ENC_NOTCODED_FRAME;
            break;
        default:
            break;
    }
}


#if 0
/*------------------------------------------------------------------------------
  MbPerInterruptTest
------------------------------------------------------------------------------*/
void MbPerInterruptTest(trace_s *trace)
{
    if (trace->testId != 3) {
            return;
    }

    trace->control.mbPerInterrupt = trace->vopNum;
}

/*------------------------------------------------------------------------------
RlcBufferFullTest()
------------------------------------------------------------------------------*/
void RlcBufferFullTest(trace_s *trace)
{
    i32 vopNum = trace->vopNum;
    control_s *control = &trace->control;
    i32 tmp ;

    if (trace->testId != 4) {
            return;
    }

    tmp = 800 / (vopNum + 2);

    control->dataBufferSize = (tmp & -0x8);
}

/*------------------------------------------------------------------------------
RandomInterruptTest()
------------------------------------------------------------------------------*/
void RandomInterruptTest(trace_s *trace)
{
    i32 vopNum = trace->vopNum;
    control_s *control = &trace->control;

    if (trace->testId != 5) {
            return;
    }

    /* 08.05.06: No irq-interval for stream mode */
    if (vopNum < 2) {
            control->dataBufferSize  = 448;
            /* control->mbPerInterrupt = 11; */
    } else if (vopNum < 4) {
            control->dataBufferSize  = 336;
            /* control->mbPerInterrupt = 22; */
    } else if (vopNum < 6) {
            control->dataBufferSize  = 224;
            /* control->mbPerInterrupt = 33; */
    } else if (vopNum < 8) {
            control->dataBufferSize  = 200;
            /* control->mbPerInterrupt = 0; */
    } else if (vopNum < 10) {
            control->dataBufferSize  = 168;
            /* control->mbPerInterrupt = 1; */
    }
}

/*------------------------------------------------------------------------------
H264FilterTest
------------------------------------------------------------------------------*/
void H264FilterTest(trace_s *trace, slice_s *slice)
{
    i32 vopNum = trace->vopNum;

    if (trace->testId != 7) {
            return;
    }

    slice->disableDeblocking = (vopNum/2)%3;
    if (vopNum == 0) {
            slice->filterOffsetA = -12;
            slice->filterOffsetB = 12;
    } else if (vopNum < 77) {
            if (vopNum%6 == 0) {
                    slice->filterOffsetA += 2;
                    slice->filterOffsetB -= 2;
            }
    } else if (vopNum == 78) {
            slice->filterOffsetA = -12;
            slice->filterOffsetB = -12;
    } else if (vopNum < 155) {
            if (vopNum%6 == 0) {
                    slice->filterOffsetA += 2;
                    slice->filterOffsetB += 2;
            }
    }
}

/*------------------------------------------------------------------------------
H264SliceQuantTest()  Change sliceQP from min->max->min.
------------------------------------------------------------------------------*/
void H264SliceQuantTest(trace_s *trace, slice_s *slice, mb_s *mb,
            rateControl_s *rc)
{
    if (trace->testId != 8) {
            return;
    }

    rc->vopRc   = NO;
    rc->mbRc    = NO;
    rc->picSkip = NO;

    if (mb->qpPrev == rc->qpMin) {
            mb->qpLum = rc->qpMax;
    } else if (mb->qpPrev == rc->qpMax) {
            mb->qpLum = rc->qpMin;
    } else {
            mb->qpLum = rc->qpMax;
    }

    mb->qpCh  = qpCh[MIN(51, MAX(0, mb->qpLum + mb->chromaQpOffset))];
    rc->qp = rc->qpHdr = mb->qpLum;
    slice->sliceQpDelta = mb->qpLum - slice->picInitQpMinus26 - 26;
}

#endif


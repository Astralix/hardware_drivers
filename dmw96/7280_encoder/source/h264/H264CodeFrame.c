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
--  Description :  Encode picture
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264CodeFrame.c,v $
--  $Date: 2008/05/28 11:14:15 $
--  $Revision: 1.5 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "ewl.h"
#include "H264CodeFrame.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

static const u32 h264Intra16Favor[52] = {
    24, 24, 24, 26, 27, 30, 32, 35, 39, 43, 48, 53, 58, 64, 71, 78,
    85, 93, 102, 111, 121, 131, 142, 154, 167, 180, 195, 211, 229,
    248, 271, 296, 326, 361, 404, 457, 523, 607, 714, 852, 1034,
    1272, 1588, 2008, 2568, 3318, 4323, 5672, 7486, 9928, 13216,
    17648
};

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void H264SetNewFrame(h264Instance_s * inst);

/*------------------------------------------------------------------------------

    H264CodeFrame

------------------------------------------------------------------------------*/
h264EncodeFrame_e H264CodeFrame(h264Instance_s * inst)
{
    asicData_s *asic = &inst->asic;
    h264EncodeFrame_e ret;

    H264SetNewFrame(inst);

    EncAsicFrameStart(inst->asic.ewl, &inst->asic.regs);

    {
        /* Encode one frame */
        i32 ewl_ret;

        /* Wait for IRQ */
        ewl_ret = EWLWaitHwRdy(asic->ewl);

        if(ewl_ret != EWL_OK)
        {
            if(ewl_ret == EWL_ERROR)
            {
                /* IRQ error => Stop and release HW */
                ret = H264ENCODE_SYSTEM_ERROR;
            }
            else    /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            {
                /* IRQ Timeout => Stop and release HW */
                ret = H264ENCODE_TIMEOUT;
            }

            EncAsicStop(asic->ewl);
            /* Release HW so that it can be used by other codecs */
            EWLReleaseHw(asic->ewl);

        }
        else
        {
            i32 status = EncAsicCheckStatus_V2(asic);

            switch (status)
            {
            case ASIC_STATUS_ERROR:
                ret = H264ENCODE_HW_ERROR;
                break;
            case ASIC_STATUS_BUFF_FULL:
                ret = H264ENCODE_OK;
                inst->stream.overflow = ENCHW_YES;
                break;
            case ASIC_STATUS_HW_RESET:
                ret = H264ENCODE_HW_RESET;
                break;
            case ASIC_STATUS_FRAME_READY:
                {
                    /* last not full 64-bit counted in HW data */
                    const u32 hw_offset = inst->stream.byteCnt & (0x07U);

                    inst->stream.byteCnt +=
                        asic->regs.outputStrmSize - hw_offset;
                    inst->stream.stream +=
                        asic->regs.outputStrmSize - hw_offset;

                    ret = H264ENCODE_OK;
                    break;
                }
            default:
                /* should never get here */
                ASSERT(0);
                ret = H264ENCODE_HW_ERROR;
            }
        }
    }

    /* Reset the intra16Favor value for next frame */ 
    inst->asic.regs.intra16Favor = 0;

    return ret;
}

/*------------------------------------------------------------------------------

    Set encoding parameters at the beginning of a new frame.

------------------------------------------------------------------------------*/
void H264SetNewFrame(h264Instance_s * inst)
{
    asicData_s *asic = &inst->asic;
    regValues_s *regs = &inst->asic.regs;

    regs->outputStrmSize -= inst->stream.byteCnt;
    regs->outputStrmSize /= 8;  /* 64-bit addresses */
    regs->outputStrmSize &= (~0x07);    /* 8 multiple size */

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (inst->stream.byteCnt & (~0x07));

    /* bit offset in the last 64-bit word */
    regs->firstFreeBit = (inst->stream.byteCnt & 0x07) * 8;

    /* header remainder is byte aligned, max 7 bytes = 56 bits */
    if(regs->firstFreeBit != 0)
    {
        /* 64-bit aligned stream pointer */
        const u8 *pTmp = (u8 *) ((size_t) (inst->stream.stream) & (u32) (~0x07));
        u32 val;

        val = pTmp[0] << 24;
        val |= pTmp[1] << 16;
        val |= pTmp[2] << 8;
        val |= pTmp[3];

        regs->strmStartMSB = val;  /* 32 bits to MSB */

        if(regs->firstFreeBit > 32)
        {
            val = pTmp[4] << 24;
            val |= pTmp[5] << 16;
            val |= pTmp[6] << 8;

            regs->strmStartLSB = val;
        }
        else
            regs->strmStartLSB = 0;
    }
    else
    {
        regs->strmStartMSB = regs->strmStartLSB = 0;
    }

    regs->frameNum = inst->slice.frameNum;
    regs->idrPicId = inst->slice.idrPicId;

    /* Store the final register values in the register structure */
    regs->sliceSizeMbRows = inst->slice.sliceSize / inst->mbPerRow;
    regs->chromaQpIndexOffset = inst->picParameterSet.chromaQpIndexOffset;

    regs->picInitQp = (u32) (inst->picParameterSet.picInitQpMinus26 + 26);

    regs->qp = inst->rateControl.qpHdr;
    regs->qpMin = inst->rateControl.qpMin;
    regs->qpMax = inst->rateControl.qpMax;

    if(inst->rateControl.mbRc)
    {
        regs->cpTarget = (u32 *) inst->rateControl.qpCtrl.wordCntTarget;
        regs->targetError = inst->rateControl.qpCtrl.wordError;
        regs->deltaQp = inst->rateControl.qpCtrl.qpChange;

        regs->cpDistanceMbs = inst->rateControl.qpCtrl.checkPointDistance;

        regs->cpTargetResults = (u32 *) inst->rateControl.qpCtrl.wordCntPrev;
    }
    else
    {
        regs->cpTarget = NULL;
    }

    regs->filterDisable = inst->slice.disableDeblocking;
    if(inst->slice.disableDeblocking != 1)
    {
        regs->sliceAlphaOffset = inst->slice.filterOffsetA / 2;
        regs->sliceBetaOffset = inst->slice.filterOffsetB / 2;
    }
    else
    {
        regs->sliceAlphaOffset = 0;
        regs->sliceBetaOffset = 0;
    }

    regs->constrainedIntraPrediction =
        (inst->picParameterSet.constIntraPred == ENCHW_YES) ? 1 : 0;

    regs->frameCodingType =
        (inst->slice.sliceType == ISLICE) ? ASIC_INTRA : ASIC_INTER;

    /* If intra16Favor has not been set earlier by testId use default */
    if (regs->intra16Favor == 0)
        regs->intra16Favor = h264Intra16Favor[regs->qp];

    regs->sizeTblBase.nal = asic->sizeTbl.nal.busAddress;
    regs->sizeTblPresent = 1;

#if defined(TRACE_RECON) || defined(ASIC_WAVE_TRACE_TRIGGER)
    {
        u32 index;

        if(asic->regs.internalImageLumBaseW ==
           asic->internalImageLuma[0].busAddress)
            index = 0;
        else
            index = 1;

        EWLmemset(asic->internalImageLuma[index].virtualAddress, 0,
                  asic->internalImageLuma[index].size);
    }
#endif

#ifdef TRACE_ASIC
    EWLmemset(asic->controlBase.virtualAddress, 0, asic->controlBase.size);
#endif

}

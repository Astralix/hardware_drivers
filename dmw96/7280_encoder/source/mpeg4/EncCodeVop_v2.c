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
--  Abstract :  Encode one picture in full HW mode 
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncCodeVop_v2.c,v $
--  $Revision: 1.8 $
--  $Date: 2008/06/04 12:51:55 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncCodeVop.h"
#include "EncInstance.h"
#include "encbuffering.h"
#include "ewl.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
void Mpeg4SetNewVop_V2(instance_s * inst, stream_s * stream);

/*------------------------------------------------------------------------------

    EncCodeVop

------------------------------------------------------------------------------*/
encodeFrame_e EncCodeVop_V2(instance_s * inst, stream_s * stream)
{
    svh_s *svh;
    rateControl_s *rc;

    asicData_s *asic;
    encodeFrame_e ret = ENCODE_OK;

    ASSERT(inst != NULL && stream != NULL && stream->stream != NULL);

    rc = &inst->rateControl;
    asic = &inst->asic;
    svh = &inst->shortVideoHeader;

    /* Check parameters */
    ASSERT(svh->header == ENCHW_YES ||
           inst->videoObjectLayer.dataPart == ENCHW_NO);

    /* Generate frame start header */
    if(svh->header == ENCHW_YES)
    {
        EncSvhHdr(stream, svh, rc->qpHdr);
        EncGobFrameIdUpdate(svh);
    }
    else
    {
        EncVopHdr(stream, &inst->videoObjectPlane, rc->qpHdr);
    }

    /* Set parameters at the beginning of every VOP */
    Mpeg4SetNewVop_V2(inst, stream);

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
                ret = ENCODE_SYSTEM_ERROR;
            }
            else    /*if(ewl_ret == EWL_HW_WAIT_TIMEOUT) */
            {
                /* IRQ Timeout => Stop and release HW */
                ret = ENCODE_TIMEOUT;
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
                ret = ENCODE_HW_ERROR;
                break;
            case ASIC_STATUS_BUFF_FULL:
                ret = ENCODE_OK;
                stream->overflow = ENCHW_YES;
                break;
            case ASIC_STATUS_HW_RESET:
                ret = ENCODE_HW_RESET;
                break;
            case ASIC_STATUS_FRAME_READY:

                /* update first VP/GOB size with stream headers,
                 * HW output VP/GOB table doesn't include header remainder */
                if(svh->header == ENCHW_YES)
                {
                    *asic->sizeTbl.gob.virtualAddress += stream->bitCnt;
                }
                else
                {
                    *asic->sizeTbl.vp.virtualAddress += stream->bitCnt;
                }

                /* HW output size includes the header remainder bits, remove
                 * them first so that they are not counted twice */
                stream->byteCnt &= (~0x07);
                stream->bitCnt  &= (~0x3F);

                /* update total stream size */
                stream->byteCnt += asic->regs.outputStrmSize;
                stream->bitCnt  += asic->regs.outputStrmSize*8;

                ret = ENCODE_OK;
                break;
            default:
                /* should never get here */
                ASSERT(0);
                ret = ENCODE_DATA_ERROR;
            }
        }
    }

    return ret;
}

/*------------------------------------------------------------------------------

    Set the structures and parameters at the start of a new VOP.

------------------------------------------------------------------------------*/
void Mpeg4SetNewVop_V2(instance_s * inst, stream_s * stream)
{
    const rateControl_s *rc = &inst->rateControl;
    const vop_s *vop = &inst->videoObjectPlane;
    const svh_s *svh = &inst->shortVideoHeader;
    const mb_s *mb = &inst->macroblock;
    asicData_s *asic = &inst->asic;

    regValues_s *regs = &asic->regs;

    regs->outputStrmSize -= stream->byteCnt;
    regs->outputStrmSize /= 8;  /* 64-bit addresses */
    regs->outputStrmSize &= (~0x07);    /* 8 multiple size */

    /* 64-bit aligned stream base address */
    regs->outputStrmBase += (stream->byteCnt & (~0x07));

    /* bit offset in the last 64-bit word */
    regs->firstFreeBit = (stream->bitCnt % 64);

    if(regs->firstFreeBit != 0)
    {
        /* 64-bit aligned stream pointer */
        const u8 *pTmp = (u8 *) ((size_t) (stream->stream) & (~0x07));
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
            val |= pTmp[7];

            regs->strmStartLSB = val;
        }
        else
            regs->strmStartLSB = 0;
    }
    else
    {
        regs->strmStartMSB = regs->strmStartLSB = 0;
    }

    /* Store the final register values in the register structure */
    regs->qp = rc->qpHdr;
    regs->qpMin = rc->qpMin;
    regs->qpMax = rc->qpMax;

    if(rc->mbRc)
    {
        regs->cpTarget = (u32 *) rc->qpCtrl.wordCntTarget;
        regs->targetError = rc->qpCtrl.wordError;
        regs->deltaQp = rc->qpCtrl.qpChange;

        regs->cpDistanceMbs = rc->qpCtrl.checkPointDistance;

        regs->cpTargetResults = (u32 *) rc->qpCtrl.wordCntPrev;
    }
    else
    {
        regs->cpTarget = NULL;
    }

    /* ASIC frame coding type */
    regs->frameCodingType = (vop->vopType == IVOP) ? ASIC_INTRA : ASIC_INTER;

    if(svh->header == ENCHW_YES)
    {
        regs->gobHeaderMask = svh->gobPlace;
        regs->gobFrameId = svh->gobFrameIdBit;

        regs->sizeTblBase.gob = asic->sizeTbl.gob.busAddress;
        regs->sizeTblPresent = 1;
    }
    else
    {
        u32 bits = 1;

        while ((1U << bits) < mb->mbPerVop)
            bits++;

        regs->vpSize = mb->vpSize;
        regs->vpMbBits = bits;

        regs->sizeTblBase.vp = asic->sizeTbl.vp.busAddress;
        regs->sizeTblPresent = 1;

        regs->hec = vop->hec;
        regs->moduloTimeBase = (vop->moduloTimeBase <= 3) ? vop->moduloTimeBase : 3;
        regs->intraDcVlcThr = vop->intraDcVlcThr;
        regs->vopFcode = vop->vopFcodeForward;
        regs->timeInc = vop->vopTimeInc;

        bits = 1;

        while ((1 << bits) < vop->vopTimeIncRes)
            bits++;

        regs->timeIncBits = bits;

    }

    regs->roundingCtrl = vop->roundControl;
}

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
--  Abstract :  Encode video packaged picture in RLC HW mode 
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncCodeVop.c,v $
--  $Revision: 1.4 $
--  $Date: 2009/07/24 10:23:46 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncCodeVop.h"
#include "EncCoder.h"
#include "encbuffering.h"
#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void Mpeg4SetNewVop(mb_s * mb, vop_s * vop, asicData_s * asic,
                           const rateControl_s * rc);
static void Mpeg4NextHeader(instance_s * inst, stream_s * stream,
                            true_e vpHeader);
static encodeFrame_e Mpeg4GetAsicMbData(mb_s * mb, asicData_s * asic);
static void Mpeg4EncodeMacroblock(stream_s * stream, stream_s * stream1,
                                  stream_s * stream2, instance_s * inst);
static true_e Mpeg4NewVp(mb_s * mb, svh_s * svh, vol_s * vol, u32);
static void Mpeg4EndVp(stream_s * stream, stream_s * stream1,
                       stream_s * stream2, svh_s * svh, vol_s * vol,
                       vop_s * vop);
static void Mpeg4Flip(mb_s * mb);
static void Mpeg4SetStore(stream_s * stream, stream_s * stream1,
                          stream_s * stream2);
static void Mpeg4FlushStore(stream_s * stream, stream_s * stream1,
                            stream_s * stream2);

/*------------------------------------------------------------------------------

    EncCodeVop

------------------------------------------------------------------------------*/
encodeFrame_e EncCodeVop(instance_s * inst, stream_s * stream)
{
    mb_s *mb;
    vol_s *vol;
    vop_s *vop;
    svh_s *svh;
    rateControl_s *rc;
    asicData_s *asic;

    stream_s stream1;   /* Temporary stream stores for */
    stream_s stream2;   /* data partitioned stream */
    true_e vpHeader = ENCHW_NO; /* Add VP/GOB header before next mb */
    i32 bitCnt = 0; /* Current stream bit count */
    u32 vpStartBitCnt = 0;  /* Bit count at start of video packet */
    encodeFrame_e ret = ENCODE_OK;
    bufferState_e state;

    ASSERT(inst != NULL && stream != NULL && stream->stream != NULL);

    mb = &inst->macroblock;
    vol = &inst->videoObjectLayer;
    vop = &inst->videoObjectPlane;
    svh = &inst->shortVideoHeader;
    rc = &inst->rateControl;
    asic = &inst->asic;

#ifdef MPEG4_HW_VLC_MODE_ENABLED
    /* Check stream type, only video packages supported here */
    ASSERT(svh->header == ENCHW_NO || vol->resyncMarkerDisable == ENCHW_NO);
#endif

    /* Set parameters at the beginning of every VOP */
    Mpeg4SetNewVop(mb, vop, asic, rc);

    /* Output buffer is temporary store for data partitioned stream */
    Mpeg4SetStore(stream, &stream1, &stream2);

    /* The input must be initialized for the first macroblock 
     * input for SW == output from ASIC */
    asic->swControl = asic->controlBase.virtualAddress;
    EncResetBuffers(&asic->buffering);

    state = EncNextState(asic);

    /* Encode one frame macroblock by macroblock */
    while(state != DONE)
    {
        if(state == HWON_SWOFF)
        {   /* SWOFF => wait for HW */
#ifdef TRACE_BUFFERING
            EncTraceAsicEvent(mb->mbNum);
#endif
            /* Wait for IRQ */
            i32 ewl_ret = EWLWaitHwRdy(asic->ewl);

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
                break;  /* end the encoding loop */
            }
        }
        else    /* SWON => process a macroblock with SW */
        {

#if defined (ENC7280_CTRL_DATA_WORD_SWAP) && (ENC7280_OUTPUT_ENDIAN_WIDTH == 1)
            /* do the word swapping in the 64-bit data */
            {
                u32 * mb_control_base = asic->swControl;
                u32 tmp;
                tmp = mb_control_base[0];
                mb_control_base[0] = mb_control_base[1];
                mb_control_base[1] = tmp;
                tmp = mb_control_base[2];
                mb_control_base[2] = mb_control_base[3];
                mb_control_base[3] = tmp;
                tmp = mb_control_base[4];
                mb_control_base[4] = mb_control_base[5];
                mb_control_base[5] = tmp;
            }
#endif      

            /* Get macroblock QP from control field */
            mb->qp = mb->qpLastCoded;

            /* Write next header if needed (VOP, VP, GOB) */
            Mpeg4NextHeader(inst, stream, vpHeader);

            mb->qp = EncAsicQp(asic->swControl);

            /* Get macroblock data from ASIC output and store in mb structure */
            if(Mpeg4GetAsicMbData(mb, asic) != ENCODE_OK)
            {
                ret = ENCODE_DATA_ERROR;
                break;  /* end the encoding loop */
            }

            /* Encode macroblock depending on stream type */
            Mpeg4EncodeMacroblock(stream, &stream1, &stream2, inst);

            /* Move to the next macroblock */
            mb->mbNum++;
            mb->column++;
            if(mb->column == mb->lastColumn)
            {
                mb->column = 0;
                mb->row++;
            }

            bitCnt = stream->bitCnt + stream1.bitCnt + stream2.bitCnt;

            /* Check if new video packet or GOB or VOP should begin */
            vpHeader = Mpeg4NewVp(mb, svh, vol, bitCnt);

            /* Handle stream when end of VP/GOB/VOP */
            if(vpHeader == ENCHW_YES)
            {
                i32 vpSize;

                Mpeg4EndVp(stream, &stream1, &stream2, svh, vol, vop);

                vpSize = (i32) (stream->bitCnt - vpStartBitCnt) / 8;

                /* Store VP size (bytes) in table */
                if(mb->vpSizes != NULL)
                {
                    if(mb->vpSizeSpace > 1)
                    {
                        *mb->vpSizes++ = vpSize;
                        *mb->vpSizes = 0;
                        mb->vpSizeSpace--;
                    }
                    else
                    {
                        mb->vpSizes[0] += vpSize;
                    }
                }

                vpStartBitCnt = stream->bitCnt;

                /* Maximum bits before next video packet */
                mb->bitCntVp = stream->bitCnt + mb->vpSize;
            }

        }

        /* Get the status and update buffering accordingly */
        if(EncAsicCheckStatus(asic) != ENCHW_OK)
        {
            /* encoding ends in EncNextState() */
            if(asic->regs.regMirror[1] & ASIC_STATUS_ERROR)
            {
                ret = ENCODE_HW_ERROR;
            }
            else    /* ASIC_STATUS_HW_RESET */
            {
                ret = ENCODE_HW_RESET;
            }
        }

#ifdef TRACE_BUFFERING
        EncTraceAsicStatus(asic->regs.status, mb->mbNum);
#endif

        state = EncNextState(asic);
    }

    if(ret == ENCODE_OK)
    {
        /* Use the reconstructed frame as the reference for the next frame */
        /* EncAsicRecycleInternalImage(&asic->regs); */

        /* done on API level because RC might discard the current VOP and then
         * we should not swap the internal frames */
    }
    else    /* ERROR case */
    {
        /* check that HW is not already stopped and released */
        if((asic->regs.regMirror[1] &
            (ASIC_STATUS_ERROR | ASIC_STATUS_HW_RESET |
             ASIC_STATUS_FRAME_READY)) == 0)
        {
            EncAsicStop(asic->ewl);
            /* Release HW so that it can be used by other codecs */
            EWLReleaseHw(asic->ewl);
        }
    }

    return ret;
}

/*------------------------------------------------------------------------------

    Set the structures and parameters at the start of a new VOP.

------------------------------------------------------------------------------*/
void Mpeg4SetNewVop(mb_s * mb, vop_s * vop, asicData_s * asic,
                    const rateControl_s * rc)
{
    regValues_s *regs = &asic->regs;

    /* Set rest of the VOP parameters */
    vop->vopFcodeForward = 1;

    /* Set macroblock parameters */
    mb->vopType = vop->vopType;
    mb->fCode = vop->vopFcodeForward;
    mb->intraDcVlcThr = vop->intraDcVlcThr;
    mb->mbNum = 0;
    mb->column = 0;
    mb->row = 0;
    mb->vpMbNum = 0;
    mb->type = INTRA;
    mb->vpNum = 0;

    mb->qp = mb->qpLastCoded = rc->qpHdr;

    /* Set HEC for the first VP in this VOP */
    vop->hecVp = vop->hec;

    /* Bit limit for first video packet */
    mb->bitCntVp = mb->vpSize;

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

    regs->roundingCtrl = vop->roundControl;
}

/*------------------------------------------------------------------------------

    Write the header data to the stream. The header data depends on the 
    coding type and current macroblock and may be one of:
    MPEG-4 VOP header or MPEG-4 videopacket header
    
------------------------------------------------------------------------------*/
void Mpeg4NextHeader(instance_s * inst, stream_s * stream, true_e vpHeader)
{
    mb_s *mb = &inst->macroblock;
    const vop_s *vop = &inst->videoObjectPlane;

    if((mb->mbNum == 0) && (mb->type != STUFFING))
    {
        /* First macroblock of Vop */
        EncVopHdr(stream, vop, mb->qp);
        mb->qpLastCoded = mb->qp;
    }
    else if(vpHeader == ENCHW_YES)
    {
        /* First macroblock of Video Packet */
        mb->vpMbNum = mb->mbNum;
        mb->vpNum++;

        EncVpHdr(stream, (vop_s *) vop, mb->mbNum, mb->vpMbNumBits, mb->qp);
        mb->qpLastCoded = mb->qp;
    }
}

/*------------------------------------------------------------------------------

    Get data for one macroblock from ASIC output tables. The data is parsed
    and stored in mb structure. Returns ENCODE_OK if everything ENCHW_OK and
    ENCODE_DATA_ERROR if the data is invalid.
    
------------------------------------------------------------------------------*/
encodeFrame_e Mpeg4GetAsicMbData(mb_s * mb, asicData_s * asic)
{
    const u32 *swRlc = EncGetSwBufferPos(&asic->buffering);
    i32 mbRlcSize;
    asicMbType_e mbType;

    ASSERT(swRlc);

    /* Flip mbType and Dc-predictor store */
    if(mb->column == 0)
        Mpeg4Flip(mb);

    mb->notCoded = 0;   /* The macroblock is coded by default */

#ifdef TRACE_ASIC
    EncDumpControl(asic->swControl, (4 * 6));
    EncDumpRlcMb(swRlc);
#endif

#ifdef TRACE_RLC
    EncTraceRlcMb(mb->mbNum,
                  asic->buffering.bufferLast[asic->buffering.swBuffer]);
#endif

    /* Get macroblock type from control field */
    mbType = EncAsicMbType(asic->swControl);
    switch (mbType)
    {
    case ASIC_P_16x16:
        mb->type = INTER;
        break;
    case ASIC_P_8x8:
        mb->type = INTER4V;
        break;
    case ASIC_I_4x4:
        mb->type = INTRA;
        break;
    default:
        ASSERT(0);
        return ENCODE_DATA_ERROR;
    }

    /* Get RLC counts for each block from control field */
    mbRlcSize = EncAsicRlcCount(mb->rlc, mb->rlcCount, swRlc, asic->swControl);

    /* Get DC-components or motion vectors from control field */
    if(mb->type == INTRA)
    {
        EncAsicDc(mb->dc, asic->swControl);
    }
    else
    {
        EncAsicMv(asic->swControl, mb->mv[mb->mbNum].x, 0);
        EncAsicMv(asic->swControl, mb->mv[mb->mbNum].y, 1);
    }

#ifdef TRACE_MB
    EncTraceMb(mb);
#endif

    if(mbRlcSize == -1)
    {
        ASSERT(0);
        return ENCODE_DATA_ERROR;
    }

    /* Update dQuant if qp changed from previous macroblock */
    if(mb->qp != mb->qpLastCoded)
    {
        /* QP changed so the MB type must be INTRA_Q or INTER_Q */
        i32 dQuant;

        if(mb->type == INTER4V)
        {
            ASSERT(0);
            return ENCODE_DATA_ERROR;
        }
        mb->type = (type_e) (mb->type + 1);

        /* mb->dQuant is index to dQuant-table in EncMbHeader */
        dQuant = mb->qp - mb->qpLastCoded + 2;

        /* Check that qp change is valid */
        if(((u32) dQuant) > 4U)
        {
            ASSERT(0);
            return ENCODE_DATA_ERROR;
        }

        mb->dQuant = dQuant;
    }

    /* Update mb structure */
    mb->typeCurrent[mb->column] = mb->type;

#ifdef TRACE_ASIC
    EncDumpRlc(swRlc, mbRlcSize);
#endif

    /* Update pointers to beginning of next macroblock data */
    swRlc += mbRlcSize / 4;
    EncSetSwBufferPos(&asic->buffering, (u32 *) swRlc);
    asic->swControl += 6;

    EncSetSwMbNext(&asic->buffering);

    return ENCODE_OK;
}

/*------------------------------------------------------------------------------

    Encode one macroblock. The input data for macroblock is read from ASIC
    output tables and is available for the SW. The macroblock stream is 
    generated based on the coding type.
    
------------------------------------------------------------------------------*/
void Mpeg4EncodeMacroblock(stream_s * stream, stream_s * stream1,
                           stream_s * stream2, instance_s * inst)
{
    const vol_s *vol = &inst->videoObjectLayer;
    mb_s *mb = &inst->macroblock;

    if(vol->dataPart == ENCHW_YES) {
        if((mb->type == INTRA) || (mb->type == INTRA_Q))
            EncIntraDp(stream, stream1, stream2, mb);
        else
            EncInterDp(stream, stream1, stream2, mb);
    } else {
        if((mb->type == INTRA) || (mb->type == INTRA_Q))
            EncIntra(stream, mb);
        else
            EncInter(stream, mb);
    }
}

/*------------------------------------------------------------------------------

    Check the bitcount and current macroblock number to determine
    if a new VP or GOB should be started.

------------------------------------------------------------------------------*/
true_e Mpeg4NewVp(mb_s * mb, svh_s * svh, vol_s * vol, u32 bitCnt)
{
    /* Check for end of VOP */
    if(mb->mbNum == mb->mbPerVop)
        return ENCHW_YES;

    /* Check for end of VP */
    if((mb->vpSize != 0) && (bitCnt >= mb->bitCntVp))
        return ENCHW_YES;

    return ENCHW_NO;
}

/*------------------------------------------------------------------------------

    Write the end of current coding unit (VP / GOB / RI / VOP / FRAME)
    into stream.
    
------------------------------------------------------------------------------*/
void Mpeg4EndVp(stream_s * stream, stream_s * stream1, stream_s * stream2,
                svh_s * svh, vol_s * vol, vop_s * vop)
{
    if(vol->dataPart == ENCHW_YES)
    {
        /* Handle stream stores of data partitioned stream */
        ASSERT((vop->vopType == IVOP) || (vop->vopType == PVOP));
        if(vop->vopType == IVOP)
        {
            EncPutBits(stream, 438273, 19); /* DcMarker */
            COMMENT("DcMarker");
        }
        else
        {
            EncPutBits(stream, 126977, 17); /* MotionMarker */
            COMMENT("MotionMarker");
        }

        /* Flush temporary store to output buffer */
        Mpeg4FlushStore(stream, stream1, stream2);

        /* Set new temporary stores for next VP */
        Mpeg4SetStore(stream, stream1, stream2);
    }

    /* Add stuffing depending on stream type */
    if(svh->header == ENCHW_YES)
        EncNextByteAligned(stream);
    else
        EncNextStartCode(stream);

}

/*------------------------------------------------------------------------------

    SetStore

    Set temporary byte buffer.

    Input   stream  Pointer to original stream buffer structure.
            stream1 Pointer to temporary stream buffer structure.
            stream2 Pointer to temporary stream buffer structure.

------------------------------------------------------------------------------*/
void Mpeg4SetStore(stream_s * stream, stream_s * stream1, stream_s * stream2)
{
    u32 space;

    /* Divide the empty space in original stream buffer: 3/8 + 1/8 + 4/8 */

    space = (stream->size - stream->byteCnt) / 8;
    (void) EncSetBuffer(stream1, stream->stream + space * 3, space);
    (void) EncSetBuffer(stream2, stream->stream + space * 4, space * 4);

    return;
}

/*------------------------------------------------------------------------------

    FlushStore

------------------------------------------------------------------------------*/
void Mpeg4FlushStore(stream_s * stream, stream_s * stream1, stream_s * stream2)
{
    u8 *tmp;
    u32 i;

    /* Check if stream1 or stream2 have overflown */
    if(stream1->overflow == ENCHW_YES)
    {
        stream->overflow = ENCHW_YES;
        COMMENT("\nFlush failed: stream1 overflow");
        return;
    }
    if(stream2->overflow == ENCHW_YES)
    {
        stream->overflow = ENCHW_YES;
        COMMENT("\nFlush failed: stream2 overflow");
        return;
    }

    /* Check if stream has overflown */
    tmp = stream1->stream - stream1->byteCnt;

    if(tmp < stream->stream)
    {
        stream->overflow = ENCHW_YES;
        COMMENT("\nFlush failed: stream overflow");
        return;
    }

    /* Put temporary stream1 to the stream buffer */
    for(i = 0; i < stream1->byteCnt; i++)
    {
        EncPutBits(stream, *tmp++, 8);
        COMMENT("Flush temporary stream1");
    }
    if(tmp[1] > 0)
    {
        EncPutBits(stream, tmp[0] >> (8 - tmp[1]), tmp[1]);
        COMMENT("Flush temporary stream1, Byte buffer");
    }

    /* Put temporary stream2 to the stream buffer */
    tmp = stream2->stream - stream2->byteCnt;
    for(i = 0; i < stream2->byteCnt; i++)
    {
        EncPutBits(stream, *tmp++, 8);
        COMMENT("Flush temporary stream2");
    }
    if(tmp[1] > 0)
    {
        EncPutBits(stream, tmp[0] >> (8 - tmp[1]), tmp[1]);
        COMMENT("Flush temporary stream2, Byte buffer");
    }

    return;
}

/*------------------------------------------------------------------------------

    Flip

    Flip stores needed by prediction and motion vector differential coding.
    Should be called in the beginning of macroblock column.

------------------------------------------------------------------------------*/
void Mpeg4Flip(mb_s * mb)
{
    pred_s *predTmp;
    type_e *typeTmp;

    predTmp = mb->predAbove;
    mb->predAbove = mb->predCurrent;
    mb->predCurrent = predTmp;
    typeTmp = mb->typeAbove;
    mb->typeAbove = mb->typeCurrent;
    mb->typeCurrent = typeTmp;

    return;
}

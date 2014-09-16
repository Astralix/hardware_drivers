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
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#ifdef ROI_SUPPORT
#include "EncRoiModel.h"
#endif

#include "EncRateControl.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#ifdef TRACE_RC
#include <stdio.h>
FILE *fpRCTrc = NULL;
/* Select debug output: fpRCTrc or stdout */
#define DBGOUTPUT fpRCTrc
/* Select debug level: 0 = minimum, 2 = maximum */
#define DBG_LEVEL 2
#define DBG(l, str) if (l <= DBG_LEVEL) fprintf str
#else
#define DBG(l, str)
#endif

#define INITIAL_BUFFER_FULLNESS   60    /* Decoder Buffer in procents */
#define MIN_PIC_SIZE              50    /* Procents from picPerPic */

#define MAX_PREDICTIONS   132 /* DCT refresh after this many predictions */
#define CLIP3(x,y,z)      (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))
#define DIV(a, b)         (((a) + (SIGN(a) * (b)) / 2) / (b))
#define DSCY              1 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define QP_DELTA          2
#define INTRA_QP_DELTA    0
#define WORD_CNT_MAX      65535

/* Define this if VBV should be saturated when it is about to overflow */
/*#define MPEG4_VBV_OVERFLOW_SATURATE*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 InitialQp(i32 bits, i32 pels);
static void MbQuant(rateControl_s * rc);
static void SourceParameter(rateControl_s * rc);
static i32 VirtualBuffer(virtualBuffer_s * vb, i32 timeInc);
static void VopQuant(rateControl_s * rc);
static void VideoBufferVerifierSet(vbv_s *, i32, i32);
static void LinearModel(rateControl_s * rc);
static void AdaptiveModel(rateControl_s * rc);
static void VopSkip(rateControl_s * rc);
static void VopQuantLimit(rateControl_s * rc);
static void VideoBufferVerifier(vbv_s *, i32);
static void Limit(virtualBuffer_s *);
static i32 avg_rc_error(linReg_s *p);
static void update_rc_error(linReg_s *p, i32 bits);
static i32 avg_overhead(linReg_s *p);
static void update_overhead(linReg_s *p, i32 bits, i32 clen);
static i32 gop_avg_qp(rateControl_s *rc);
static i32 new_pic_quant(linReg_s *p, i32 bits);
static void update_tables(linReg_s *p, i32 qp, i32 bits);
static void update_model(linReg_s *p);
static i32 lin_sy(i32 *qp, i32 *r, i32 n);
static i32 lin_sx(i32 *qp, i32 n);
static i32 lin_sxy(i32 *qp, i32 *r, i32 n);
static i32 lin_nsxx(i32 *qp, i32 n);

/*------------------------------------------------------------------------------

    EncRcCheck

------------------------------------------------------------------------------*/
bool_e EncRcInit(rateControl_s * rc)
{
    virtualBuffer_s *vb = &rc->virtualBuffer;
    vbv_s *vbv = &rc->videoBuffer;
    i32 tmp;

    /* Saturates really high settings */
    /* bits per unpacked frame */
    tmp = 8 * rc->mbPerPic * 384;
    /* bits per second */
    tmp = EncCalculate(tmp, vb->vopTimeIncRes, vb->timeInc);
    if (vb->bitPerSecond > (tmp / 2))
        vb->bitPerSecond = tmp / 2;

    /* QP -1: Initial QP estimation done by RC */
    if (rc->qpHdr == -1) {
        i32 tmp = EncCalculate(vb->bitPerSecond, vb->timeInc,
                    vb->vopTimeIncRes);
        rc->qpHdr = InitialQp(tmp, rc->mbPerPic*16*16);
    }

    if((rc->qpMin < 1) || (rc->qpMax > 31))
    {
        return ENCHW_NOK;
    }
    if((rc->qpHdr > rc->qpMax) || (rc->qpHdr < rc->qpMin))
    {
        return ENCHW_NOK;
    }
    ASSERT(vb->vopTimeIncRes > 0);

    /* Video buffer verifier */
    if(rc->videoBuffer.videoBufferSize > 0)
    {
        vbv->vbv = ENCHW_YES;
    }

    /* VBV needs frame RC and macroblock RC */
    if (vbv->vbv == ENCHW_YES) {
        rc->vopRc = ENCHW_YES;
        rc->mbRc = ENCHW_YES;
    }

    /* Macroblock RC needs frame RC */
    if (rc->mbRc == ENCHW_YES)
        rc->vopRc = ENCHW_YES;

    /* ROI disables macroblock RC */
    if (rc->roiRc && rc->mbRc)
        rc->mbRc = ENCHW_NO;

    /* Virtual buffer Initial bit counts */
    vb->bitPerVop = EncCalculate(vb->bitPerSecond, vb->timeInc, vb->vopTimeIncRes);
    vb->bitPerVop = MIN(0x7fffff, vb->bitPerVop);

    vb->virtualBitCnt = 0;
    vb->vopTimeInc = 0;

    vb->realBitCnt = 0;
    vb->skipBuffBit = 0;
    vb->skipBuffFlush = ENCHW_NO;
    rc->qpHdrPrev   = rc->qpHdr;
    rc->fixedQp   = rc->qpHdr;

    /* Check points are smootly distributed except last one */
    if(rc->mbRc == ENCHW_YES)
    {
        u32 tmp = (rc->mbPerPic / (CHECK_POINTS + 1));

        rc->qpCtrl.checkPointDistance = tmp;
    }
    else
    {
        rc->qpCtrl.checkPointDistance = 0;
    }

#if defined(TRACE_RC) && (DBGOUTPUT == fpRCTrc)
    if (!fpRCTrc) fpRCTrc = fopen("rc.trc", "wt");
#endif

    /* new rate control algorithm */
    update_rc_error(&rc->rError, 0x7fffffff);
    rc->frameCnt = 0;
    rc->linReg.pos = 0;
    rc->linReg.len = 0;
    rc->linReg.a1  = 0;
    rc->linReg.a2  = 0;
    rc->linReg.qs[0]   = 2 * 31;
    rc->linReg.bits[0] = 0;
    // Set to previous qphdr rather than 0. This will help in smoother visual quality when RC is 
    // re-initialised during dynamic param change (like bitrate).
    // qpHdrPrev will be zero initially during encoder start.
    rc->linReg.qp_prev = rc->qpHdrPrev;

    rc->gopQpSum = 0;
    rc->gopQpDiv = 0;
    vb->gopRem = rc->gopLen;
    rc->sad = 768 * rc->mbPerPic;
    rc->targetPicSize = 0;
    rc->frameBitCnt = 0;

    vbv->bitPerSecond = vb->bitPerSecond;
    vbv->bitPerVop = vb->bitPerVop;
    if(vb->setFirstVop == ENCHW_YES)
    {
        vbv->timeInc = vb->timeInc;
        vbv->vopTimeIncRes = vb->vopTimeIncRes;
    }

    vb->setFirstVop = ENCHW_NO;

    rc->vopCount = 0;

    DBG(0, (DBGOUTPUT, "\nRcCheck: frameRc\t%i  vbv\t%i  vopSkip\t%i\n",
                     rc->vopRc, vbv->vbv, rc->vopSkip));
    DBG(0, (DBGOUTPUT, "  mbRc\t\t\t%i  roiRc\t%i  qpHdr\t%i  Min,Max\t%i,%i\n",
                     rc->mbRc, rc->roiRc, rc->qpHdr, rc->qpMin, rc->qpMax));
    DBG(0, (DBGOUTPUT, "  checkPointDistance\t%i\n",
                     rc->qpCtrl.checkPointDistance));

    DBG(0, (DBGOUTPUT, "  CPBsize\t\t%i\n BitRate\t%i\n BitPerPic\t%i\n",
                     vbv->videoBufferSize, vb->bitPerSecond, vb->bitPerVop));

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------
  InitialQp()  Returns sequence initial quantization parameter.
------------------------------------------------------------------------------*/
static i32 InitialQp(i32 bits, i32 pels)
{
    const i32 qp_tbl[2][8] = {
                    {43, 48, 53, 62, 72, 86, 110, 0x7FFFFFFF},
                    {31, 28, 25, 22, 19, 16, 13, 10}};
    const i32 upscale = 1000;
    i32 i = -1;

    /* prevents overflow, QP would anyway be 10 with this high bitrate
       for all resolutions under and including 1920x1088 */
    if (bits > 1000000)
        return 10;

    /* Make room for multiplication */
    pels >>= 8;
    bits >>= 2;

    /* Adjust the bits value for the current resolution */
    bits *= pels + 250;
    ASSERT(pels > 0);
    ASSERT(bits > 0);
    bits /= 350 + (3 * pels) / 4;
    bits = EncCalculate(bits, upscale, pels << 6);

    while (qp_tbl[0][++i] < bits);

    return qp_tbl[1][i];
}

/*------------------------------------------------------------------------------

    VirtualBuffer

    Virtual buffer and real bit count grow until one second. After one
    second output bit rate per second must remove from virtual buffer and
    realBitCnt. Bit drifting has been taken care of.

------------------------------------------------------------------------------*/
static i32 VirtualBuffer(virtualBuffer_s * vb, i32 timeInc)
{
    i32 drift, target;

    vb->vopTimeInc += timeInc;
    vb->virtualBitCnt += EncCalculate(vb->bitPerSecond, timeInc,
                                   vb->vopTimeIncRes);

    vb->bitAvailable = target = vb->virtualBitCnt - vb->realBitCnt;

    /* Saturate target */
    if (target > 0x1FFFFFFF)
        target = 0x1FFFFFFF;
    if (target < -0x1FFFFFFF)
        target = -0x1FFFFFFF;

    /* vopTimeInc must be in range of [0, vopTimeIncRes) */
    while(vb->vopTimeInc >= vb->vopTimeIncRes)
    {
        vb->vopTimeInc -= vb->vopTimeIncRes;
        vb->virtualBitCnt -= vb->bitPerSecond;
        vb->realBitCnt -= vb->bitPerSecond;
    }

    drift = EncCalculate(vb->bitPerSecond, vb->vopTimeInc, vb->vopTimeIncRes);
    drift -= vb->virtualBitCnt;
    vb->driftPerVop = drift;
    vb->virtualBitCnt += drift;

    Limit(vb);

    DBG(1, (DBGOUTPUT, "virtualBitCnt: %6i  realBitCnt: %6i  ",
                vb->virtualBitCnt, vb->realBitCnt));
    DBG(1, (DBGOUTPUT, "target: %8i  timeInc: %i\n", target, timeInc));

    return target;
}

/*------------------------------------------------------------------------------

    EncAfterVopRc

    Check video buffer verifyer and update source model.

------------------------------------------------------------------------------*/
void EncAfterVopRc(rateControl_s * rc, i32 nzCnt, i32 byteCnt, i32 qpSum)
{
    i32 tmp, bitCnt = (i32)byteCnt * 8;

    rc->zeroCnt = rc->coeffCnt - nzCnt;
    rc->nzCnt = nzCnt;
    rc->bitCntLastVop = byteCnt * 8;
    rc->qpSum = qpSum;

    if (rc->targetPicSize) {
        tmp = ((bitCnt - rc->targetPicSize) * 100) /
                rc->targetPicSize;
    } else {
        tmp = -1;
    }

    DBG(0, (DBGOUTPUT, "AFTER VOP RC: nzCnt = %d qpSum = %d\n", nzCnt, qpSum));
    DBG(0, (DBGOUTPUT, "BitCnt %3d  BitErr/avg %3d%%  ", bitCnt,
                    ((bitCnt - rc->virtualBuffer.bitPerVop) * 100) /
                    (rc->virtualBuffer.bitPerVop+1)));
    DBG(0, (DBGOUTPUT, "BitErr/target %3i%%  qpHdr %2i  avgQp %4.1f\n",
                    tmp, rc->qpHdr,
                    (float)rc->qpSum / rc->mbPerPic));

    /* Calculate the source parameter only for INTER frames */
    if (rc->vopTypeCur != IVOP)
        SourceParameter(rc);

    /* Video buffer verifier */
    VideoBufferVerifier(&rc->videoBuffer, rc->bitCntLastVop);

    /* After video buffer verifier underflow */
    if(rc->videoBuffer.underflow == ENCHW_YES)
    {
        DBG(0, (DBGOUTPUT, "VBV underflow, frame discard\n"));
        return;
    }

    rc->vopTypePrev = rc->vopTypeCur;
    rc->virtualBuffer.realBitCnt += byteCnt * 8;

    /* Needs number of bits used for residual */
    if (rc->vopTypeCur != IVOP || rc->gopLen == 1) {
        /* Use frame average QP to update RC history tables */
        tmp = DIV(rc->qpSum, rc->mbPerPic);

        /* ROI can mess up average QP, so use the RC target QP */
        if (rc->roiRc) {
                tmp = rc->qpHdrPrev;
        }

        update_tables(&rc->linReg, tmp,
                EncCalculate(bitCnt, 131072, rc->sad ? rc->sad : 531072));
        update_overhead(&rc->overhead, 0, 2);

        if (rc->virtualBuffer.gopRem == rc->gopLen-1) {
            /* First INTER frame of GOP */
            update_rc_error(&rc->rError, 0x7fffffff);
            DBG(1, (DBGOUTPUT, "P    --- I     --- D     --- \n"));
        } else {
            /* Store the error between target and actual frame size
             * Saturate the error to avoid inter frames with
             * mostly intra MBs to affect too much */
            update_rc_error(&rc->rError,
                    MIN(bitCnt - rc->targetPicSize, 2*rc->targetPicSize));
        }
        update_model(&rc->linReg);
    } else {
        /* INTRA frame */
        DBG(1, (DBGOUTPUT, "P    xxx I     xxx D     xxx\n"));
    }
}

/*------------------------------------------------------------------------------

    EncBeforeVopRc

------------------------------------------------------------------------------*/
void EncBeforeVopRc(rateControl_s * rc, i32 timeInc)
{
    virtualBuffer_s *vb = &rc->virtualBuffer;
    i32 i, tmp = 0;

    for (i = 0; i < CHECK_POINTS; i++)
        rc->qpCtrl.wordCntTarget[i] = 0;

    /* After vbv Underflow */
    if(rc->videoBuffer.underflow == ENCHW_YES)
    {
        rc->vopTypeCur = IVOP;
    }

    if (rc->vopTypeCur == IVOP || vb->gopRem == 1) {
        vb->gopRem = rc->gopLen;
    } else {
        vb->gopRem--;
    }

    DBG(0, (DBGOUTPUT, "\nBEFORE PIC RC:\n"));
    DBG(0, (DBGOUTPUT, "Vop type current: %2i\n", rc->vopTypeCur));

    /* Virtual buffer */
    tmp = VirtualBuffer(&rc->virtualBuffer, timeInc);

    rc->targetPicSize = vb->bitPerVop + DIV(tmp,
                     MAX(rc->virtualBuffer.gopRem, 3));

    rc->targetPicSize = MAX(0, rc->targetPicSize);
    
    DBG(1, (DBGOUTPUT, "GopRem %d  ", vb->gopRem ));
    DBG(1, (DBGOUTPUT, "Rd: %6d  ", avg_rc_error(&rc->rError)));
    DBG(1, (DBGOUTPUT, "Tr: %7d\n",rc->targetPicSize));

    /* Video buffer verifier model */
    VideoBufferVerifierSet(&rc->videoBuffer, timeInc,
                           rc->virtualBuffer.driftPerVop);

    /* Vop skiping */
    VopSkip(rc);

    /* maximum prediction length 128 */
    if(rc->vopTypeCur == IVOP)
        rc->predictionCount = 0;
    else
        rc->predictionCount++;

    if(rc->predictionCount > 128)
    {
        rc->vopTypeCur = IVOP;
        rc->predictionCount = 0;
    }

    /* Vop quantization parameter (source model rate control) */
    VopQuant(rc);

    /* Vop quantization parameter user defined limitations */
    VopQuantLimit(rc);

    /* Store the final RC start QP, before ROI adjustment */
    rc->qpHdrPrev   = rc->qpHdr;

#ifdef ROI_SUPPORT
    if (rc->roiRc == ENCHW_YES) {
            EncRoiModel(rc);
            VopQuantLimit(rc);
    }
    else
#endif
    if (rc->mbRc) {
            /* Mb rate control (check point rate control) */
            MbQuant(rc);
    }


    /* Necessaries changes of structures */
    rc->stuffEnable = ENCHW_NO;

#ifdef RC_ASIC_TEST
    EncMbQuantizationTest(rc);
    rc->vopCount++;
#endif

    DBG(0, (DBGOUTPUT, "\nVop type current\t%i\n",rc->vopTypeCur));
    DBG(0, (DBGOUTPUT, "Vop coded\t\t%i\n",rc->vopCoded));
    DBG(0, (DBGOUTPUT, "Vop qpHdr\t\t%i\n",rc->qpHdr));

    for(i = 0; i < CHECK_POINTS; i++) {
        DBG(1, (DBGOUTPUT, "CP %i  mbNum %4i  wTarg %5i\n", i,
                (rc->qpCtrl.checkPointDistance * (i + 1)),
                rc->qpCtrl.wordCntTarget[i]*4));
    }

}

/*------------------------------------------------------------------------------

    MbQuant

    Caclucate table ChekPoint:
    mbNum   bitCntTarget
    0   0       Mb nro. 0 no rc action.
    1   1       Mb nro. 1 no rc action, stuffin enabled.
    2   300     Mb nro. 2 target byte cnt is 300.

    Caclucate table of qpChange:
    -400    -3      Check point error -400 bit. Qp change is -3.

    Input   rc  Pointer to structure.

------------------------------------------------------------------------------*/
void MbQuant(rateControl_s * rc)
{
    if(rc->mbRc != ENCHW_YES)
    {
        return;
    }

    /* Stuffing mb befere last mabroblock if needed
     * if (rc->videoBuffer.vbv == ENCHW_YES) {
     * qc->wordCntTarget[CHECK_POINTS] = 1;
     * } */

    if (rc->vopTypeCur == IVOP)
            return;

    if (rc->srcPrm == 0) {
            return;
    }

    /* Required zero cnt */
    rc->nzCntReq = EncCalculate(rc->targetPicSize, 256, rc->srcPrm);
    rc->nzCntReq = MIN(rc->coeffCnt, MAX(0, rc->nzCntReq));

    /* Use linear model when previous frame can't be used for prediction */
    if ((rc->bitCntLastVop == 0) ||
        (rc->vopTypeCur != rc->vopTypePrev) ||
        (rc->nzCnt == 0))
    {
        LinearModel(rc);
    }
    else
    {
        AdaptiveModel(rc);
    }
}

/*------------------------------------------------------------------------------

    LinearModel

    wordCntTarget and wordError must scale down by 4 because of ASIC.
    wordCntTarget range is [2 ... 2^16-1] (using 16bit) because of ASIC.
    wordError range is [-2^16 ... 2^16-1] (sing 16bit) because of ASIC.

------------------------------------------------------------------------------*/
void LinearModel(rateControl_s * rc)
{
    qpCtrl_s *qc = &rc->qpCtrl;
    i32 nzCntReq = rc->nzCntReq;
    i32 scaler;
    i32 i, tmp;

    if(nzCntReq > 0)
    {
        scaler = EncCalculate(nzCntReq, 1024, CHECK_POINTS + 1);
    }
    else
    {
        return;
    }

    DBG(1, (DBGOUTPUT, "\n Linear Target: %6d Count: %5d Scaler: %f\n",
                rc->nzCntReq, rc->nzCnt, (float)scaler/1024));

    for(i = 0; i < CHECK_POINTS; i++)
    {
        tmp = ((i + 1) * scaler) / 1024;
        tmp = MIN(WORD_CNT_MAX, tmp / 4 + 1);

        qc->wordCntTarget[i] = tmp;
    }

    /* Qp change table */
    tmp = EncCalculate(rc->virtualBuffer.bitPerVop, 256, rc->srcPrm);

    DBG(1, (DBGOUTPUT, "  Error Limit: %6d SrcPrm: %3d\n",
                    tmp, rc->srcPrm/256));

    qc->wordError[0] = -tmp * 4;
    qc->qpChange[0] = -3;
    qc->wordError[1] = -tmp * 3;
    qc->qpChange[1] = -2;
    qc->wordError[2] = -tmp * 2;
    qc->qpChange[2] = -1;
    qc->wordError[3] = tmp * 1;
    qc->qpChange[3] = 0;
    qc->wordError[4] = tmp * 2;
    qc->qpChange[4] = 1;
    qc->wordError[5] = tmp * 3;
    qc->qpChange[5] = 2;
    qc->wordError[6] = tmp * 4;
    qc->qpChange[6] = 3;

    for(i = 0; i < CTRL_LEVELS; i++)
    {
        tmp = qc->wordError[i];
        tmp = CLIP3(-32768, 32767, tmp / 4);

        qc->wordError[i] = tmp;
    }

    return;
}

/*------------------------------------------------------------------------------

    AdaptiveModel

    wordCntPrev has been scaled down by 256, see CheckPoint(). Also
    wordError must scale down by 256 because of ASIC.
    wordCntTarget range is [2 ... 2^13-1] (using 13bit) because of ASIC.
    wordError range is [-2^13 ... 2^13-1] (sing 14bit) because of ASIC.

------------------------------------------------------------------------------*/
void AdaptiveModel(rateControl_s * rc)
{
    qpCtrl_s *qc = &rc->qpCtrl;
    i32 nzCntReq = rc->nzCntReq;
    i32 scaler;
    i32 i, tmp;

    if((nzCntReq > 0) && (rc->nzCnt > 0))
    {
        scaler = EncCalculate(nzCntReq, 1024, rc->nzCnt);
    }
    else
    {
        return;
    }

    DBG(1, (DBGOUTPUT, "\n Adaptive Target:\t%6d prevCnt: %5d Scaler: %f\n",
                nzCntReq, rc->nzCnt, (float)scaler/1024));

    for(i = 0; i < CHECK_POINTS; i++)
    {
        tmp = (qc->wordCntPrev[i] * scaler) / 1024;
        tmp = MIN(WORD_CNT_MAX, tmp / 4 + 1);

        qc->wordCntTarget[i] = tmp;
        DBG(2, (DBGOUTPUT, " CP %i  wordCntPrev %6i  wordCntTarget %6i\n",
                    i, qc->wordCntPrev[i], qc->wordCntTarget[i]));
    }

    /* Qp change table */
    tmp = EncCalculate(rc->virtualBuffer.bitPerVop, 256, rc->srcPrm * 3);

    DBG(1, (DBGOUTPUT, "  Error Limit: %6d SrcPrm: %3d\n",
                    tmp, rc->srcPrm/256));

    qc->wordError[0] = -tmp * 4;
    qc->qpChange[0] = -2;
    qc->wordError[1] = -tmp * 3;
    qc->qpChange[1] = -1;
    qc->wordError[2] = -tmp * 2;
    qc->qpChange[2] = 0;
    qc->wordError[3] = tmp * 1;
    qc->qpChange[3] = 0;
    qc->wordError[4] = tmp * 2;
    qc->qpChange[4] = 1;
    qc->wordError[5] = tmp * 3;
    qc->qpChange[5] = 2;
    qc->wordError[6] = tmp * 4;
    qc->qpChange[6] = 3;

    for(i = 0; i < CTRL_LEVELS; i++)
    {
        tmp = qc->wordError[i];
        tmp = CLIP3(-32768, 32767, tmp / 4);

        qc->wordError[i] = tmp;
    }
    return;
}

/*------------------------------------------------------------------------------

    SourceParameter

    Source parameter and scaler of last coded VOP. Parameters has been
    scaled up by factor 256.

------------------------------------------------------------------------------*/
void SourceParameter(rateControl_s * rc)
{
    i32 notZeroCnt;
    i32 zeroCnt;
    i32 bitCntLastVop;

    /* Stuffing macroblock fix */
    bitCntLastVop = rc->bitCntLastVop;
#if 0
    if(rc->videoBuffer.vbv == ENCHW_YES)
    {
        rc->videoBuffer.stuffBitCnt = 0;
        if(rc->videoBuffer.stuffWordCnt > 0)
        {
            /* Word count of last check point */
            tmp = rc->qpCtrl.wordCntPrev[CHECK_POINTS] * 8;
            if(tmp < rc->videoBuffer.stuffWordCnt)
            {
                bitCntLastVop = tmp * 32;
                tmp = rc->bitCntLastVop - bitCntLastVop;
                rc->videoBuffer.stuffBitCnt = tmp;
            }
        }
    }
#endif

    zeroCnt = rc->zeroCnt;
    notZeroCnt = rc->coeffCnt - zeroCnt;
    if (!notZeroCnt) {
        notZeroCnt++;
    }

    /* Calculate can handle division by 0 */
    rc->srcPrm = EncCalculate(bitCntLastVop, 256, notZeroCnt);

    ASSERT(zeroCnt <= rc->coeffCnt);
    ASSERT(zeroCnt >= 0);

    DBG(1, (DBGOUTPUT, "nonZeroCnt %d,  srcPrm %d\n",
                notZeroCnt, rc->srcPrm));
}

/*------------------------------------------------------------------------------

    VopSkip

    Try to avoid vop skip by bufferin.

------------------------------------------------------------------------------*/
void VopSkip(rateControl_s * rc)
{
    virtualBuffer_s *vb = &rc->virtualBuffer;
    i32 bitAvailable;
    i32 skipLimit;

    rc->vopCoded = ENCHW_YES;
    if(rc->vopSkip != ENCHW_YES)
    {
        return;
    }

    if(vb->bitAvailable > vb->bitPerVop)
    {
        vb->skipBuffBit = 0;
    }

    /* Skip buffer recovery */
    if((vb->skipBuffBit == 0) && (vb->bitAvailable > 0))
    {
        vb->skipBuffFlush = ENCHW_NO;
        vb->skipBuffBit = vb->bitPerVop / 2;
    }

    bitAvailable = vb->bitAvailable;
    skipLimit = vb->bitPerVop / 4;
    if(bitAvailable <= skipLimit)
    {
        vb->skipBuffFlush = ENCHW_YES;
        bitAvailable += vb->skipBuffBit;
        if(bitAvailable <= skipLimit)
        {
            rc->vopCoded = ENCHW_NO;
        }
    }

    /* Flush skip buffer */
    if(vb->skipBuffFlush == ENCHW_YES)
    {
        vb->bitAvailable += vb->skipBuffBit;
        vb->skipBuffBit /= 2;
        Limit(vb);
    }

    return;
}

/*------------------------------------------------------------------------------

    VopQuant

    Next quantization parameter of VOP based on source model.

------------------------------------------------------------------------------*/
void VopQuant(rateControl_s * rc)
{
    i32 tmp = 0;
    i32 avgOverhead, tmpValue, avgRcError;

    if(rc->vopRc != ENCHW_YES)
    {
        rc->qpHdr = rc->fixedQp;
        return;
    }

    /* determine initial quantization parameter for current picture */
    if (rc->vopTypeCur == IVOP) {
        /* INTRA frame */
        avgOverhead = avg_overhead(&rc->overhead);

        /* If all frames or every other frame is intra we calculate new QP
         * for intra the same way as for inter */
        if ((rc->gopLen == 1) || (rc->gopLen == 2)) {
            tmp = new_pic_quant(&rc->linReg, EncCalculate(
                    rc->targetPicSize - avgOverhead,
                    131072, 531072 /*rc->sad*/));
        } else {
            DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: xx xx D:  xxxx newQP: xx\n"));
            tmp = gop_avg_qp(rc);
        }
        if (tmp)
            rc->qpHdr = tmp;
        else if(rc->videoBuffer.underflow == ENCHW_YES)
            /* No frames encoded so far and VBV skip => use max qp */
            rc->qpHdr = rc->qpMax;
        /* else use the same QP as previously */
    } else if (rc->vopTypePrev == IVOP) {
        /* INTER frame after INTRA */
        DBG(1, (DBGOUTPUT, "R/cx:  xxxx  QP: == == D:  ==== newQP: ==\n"));
        rc->qpHdr = rc->qpHdrPrev;
        VopQuantLimit(rc);
        rc->gopQpSum += rc->qpHdr;
        rc->gopQpDiv++;
    } else {
        /* INTER frame after INTER */
        avgRcError = avg_rc_error(&rc->rError);
        avgOverhead = avg_overhead(&rc->overhead);
        tmpValue = EncCalculate(rc->targetPicSize  - avgRcError -
                avgOverhead, 131072, rc->sad);
        rc->qpHdr = new_pic_quant(&rc->linReg, tmpValue);
        VopQuantLimit(rc);
        rc->gopQpSum += rc->qpHdr;
        rc->gopQpDiv++;
    }
}

/*------------------------------------------------------------------------------

    VopQuantLimit

------------------------------------------------------------------------------*/
void VopQuantLimit(rateControl_s * rc)
{
    rc->qpHdr = MIN(rc->qpMax, MAX(rc->qpHdr, rc->qpMin));

    return;
}

/*------------------------------------------------------------------------------

    VideoBufferVerifierSet

    Fix video buffer driting and set minimum bit count of nex vop.

------------------------------------------------------------------------------*/
void VideoBufferVerifierSet(vbv_s * vbv, i32 timeInc, i32 driftPerVop)
{
    i32 tmp;

    if(vbv->vbv != ENCHW_YES)
    {
        return;
    }

    /* Fix video buffer drifting */
    vbv->videoBufferBitCnt += driftPerVop;

    /* Time since last coded Vop */
    vbv->vopTimeInc += timeInc;

    /* Next vop mimimum word count */
    tmp = EncCalculate(vbv->bitPerSecond, vbv->vopTimeInc, vbv->vopTimeIncRes);

    vbv->stuffWordCnt = 2 * tmp;
    vbv->stuffWordCnt += vbv->videoBufferBitCnt - vbv->videoBufferSize;
    vbv->stuffWordCnt += vbv->bitPerVop * 2;
    vbv->stuffWordCnt /= 32;    /* Words */
    tmp = vbv->videoBufferSize * 3 / 4 / 32;    /* Words */
    vbv->stuffWordCnt = MIN(vbv->stuffWordCnt, tmp);
    vbv->stuffWordCnt = MAX(0, vbv->stuffWordCnt);

    DBG(0, (DBGOUTPUT, "\nStuffing bit count\t%i\n", vbv->stuffWordCnt * 32));

}

/*------------------------------------------------------------------------------

    VideoBufferVerifier

------------------------------------------------------------------------------*/
void VideoBufferVerifier(vbv_s * vbv, i32 bitCntLastVop)
{
    i32 tmp;
    i32 bitCnt;

    if(vbv->vbv != ENCHW_YES)
    {
        return;
    }

    DBG(1, (DBGOUTPUT, "\nVIDEO BUFFER VERIFIER:\n"));

    /* Video buffer fullnes */
    bitCnt = vbv->videoBufferBitCnt +
            EncCalculate(vbv->bitPerSecond, vbv->vopTimeInc, vbv->vopTimeIncRes);

#ifdef MPEG4_VBV_OVERFLOW_SATURATE
    /* Video buffer must not overflow => saturate */
    if (bitCnt > vbv->videoBufferSize) {
        bitCnt = vbv->videoBufferSize;
        DBG(1, (DBGOUTPUT, "Video buffer overflow, saturating to max\n"));
    }
#else
    /* Video buffer can overflow when the framerate is too low =>
     * not enough bits is generated. However, our understanding is that
     * this is an invalid setting and the buffer should not be saturated
     * to the upper limit. Instead we let the buffer grow and only saturate
     * when it is about to cause invalid rate control operation. */
    if (bitCnt > 0x3FFFFFFF)
        bitCnt = 0x3FFFFFFF;
#endif

    bitCnt -= bitCntLastVop;

    DBG(1, (DBGOUTPUT, "Video buffer size\t%i\n", vbv->videoBufferSize));
    DBG(1, (DBGOUTPUT, "Video buffer bit Cnt\t%i\n", bitCnt));
    DBG(1, (DBGOUTPUT, "Video buffer fullnes\t%i\n",
                 bitCnt * 100 / vbv->videoBufferSize));
    DBG(1, (DBGOUTPUT, "Last vop bit Cnt\t%i\n", bitCntLastVop));
    DBG(1, (DBGOUTPUT, "Used vopTimeInc\t\t%i\n\n", vbv->vopTimeInc));

    /* Video buffer status */
    vbv->underflow = ENCHW_NO;
    vbv->overflow = ENCHW_NO;
    if(bitCnt < 0)
    {
        vbv->underflow = ENCHW_YES;
        return;
    }

    tmp = bitCnt + bitCntLastVop;
    if(tmp > vbv->videoBufferSize)
    {
        vbv->overflow = ENCHW_YES;
    }

    /* Coded vop is new time reference */
    vbv->vopTimeInc = 0;
    vbv->videoBufferBitCnt = bitCnt;
}

/*------------------------------------------------------------------------------

    EncCalculate

    I try to avoid overflow and calculate good enough result of a*b/c.

------------------------------------------------------------------------------*/
i32 EncCalculate(i32 a, i32 b, i32 c)
{
    i32 left = 31;
    i32 right = 0;
    i32 sift;
    i32 sign = 1;
    i32 tmp;

    if(a < 0)
    {
        sign = -1;
        a = -a;
    }
    if(b < 0)
    {
        sign *= -1;
        b = -b;
    }
    if(c < 0)
    {
        sign *= -1;
        c = -c;
    }

    if(c == 0)
    {
        return 0x7FFFFFFF * sign;
    }

    if(b > a)
    {
        tmp = b;
        b = a;
        a = tmp;
    }

    while(((a << left) >> left) != a)
        left--;
    while((b >> right++) > c) ;
    sift = left - right;

    return (((a << sift) / c * b) >> sift) * sign;
}

/*------------------------------------------------------------------------------

    Limit

    Try to avoid virtual buffer overflow: Limit bitAvailable to range [0
    (2^31-1)/256] and realBitCnt to range [-2^30 2^30-1]. After realBitCnt
    limit rate control will not work properly because it lose bits.

------------------------------------------------------------------------------*/
void Limit(virtualBuffer_s * vb)
{
    vb->bitAvailable = MIN(vb->bitPerVop * 5, vb->bitAvailable);
    vb->bitAvailable = MIN(0x7fffff, vb->bitAvailable);
    vb->bitAvailable = MAX(-0x7fffff, vb->bitAvailable);
    vb->realBitCnt = MIN(0x3FFFFFFF, MAX(-0x40000000, vb->realBitCnt));
}

/*------------------------------------------------------------------------------
  avg_rc_error()  PI(D)-control for rate prediction error.
------------------------------------------------------------------------------*/
static i32 avg_rc_error(linReg_s *p)
{
    return DIV(p->bits[2] * 4 + p->bits[1] * 6 + p->bits[0] * 0, 100);
}

/*------------------------------------------------------------------------------
  update_overhead()  Update PI(D)-control values
------------------------------------------------------------------------------*/
static void update_rc_error(linReg_s *p, i32 bits)
{
    p->len = 3;

    if (bits == (i32)0x7fffffff) {
        /* RESET */
        p->bits[0] = 0;
        p->bits[1] = 0;
        p->bits[2] = 0;
        return;
    }
    p->bits[0] = bits - p->bits[2]; /* Derivative */
    p->bits[1] = bits + p->bits[1]; /* Integral */
    p->bits[2] = bits;              /* Proportional */
    DBG(1, (DBGOUTPUT, "P %6d I %7d D %7d\n", p->bits[2],  p->bits[1], p->bits[0]));
}

/*------------------------------------------------------------------------------
  avg_overhead()  Average number of bits used for overhead data.
------------------------------------------------------------------------------*/
static i32 avg_overhead(linReg_s *p)
{
    i32 i, tmp ;

    if (p->len) {
        for (i = 0, tmp = 0; i < p->len; i++) {
            tmp += p->bits[i];
        }
        return DIV(tmp, p->len);
    }
    return 0;
}

/*------------------------------------------------------------------------------
  update_overhead()  Update overhead (non texture data) table. Only statistics
  of PSLICE, please.
------------------------------------------------------------------------------*/
static void update_overhead(linReg_s *p, i32 bits, i32 clen)
{
    i32 tmp = p->pos;

    p->bits[tmp] = bits;

    if (++p->pos >= clen) {
        p->pos = 0;
    }
    if (p->len < clen) {
        p->len++;
    }
}

/*------------------------------------------------------------------------------
  gop_avg_qp()  Average quantization parameter of P frames of the previous GOP.
------------------------------------------------------------------------------*/
i32 gop_avg_qp(rateControl_s *rc)
{
    i32 tmp = 0;

    if (rc->gopQpSum) {
        tmp = DIV(rc->gopQpSum, rc->gopQpDiv);
        tmp -= INTRA_QP_DELTA;
    }
    rc->gopQpSum = 0;
    rc->gopQpDiv = 0;

    return tmp;
}

/*------------------------------------------------------------------------------
  new_pic_quant()  Calculate new quantization parameter from the 2nd degree R-Q
  equation. Further adjust Qp for "smoother" visual quality.
------------------------------------------------------------------------------*/
static i32 new_pic_quant(linReg_s *p, i32 bits)
{
    i32 tmp, qp_best = p->qp_prev, qp = p->qp_prev, diff;
    i32 diff_best = 0x7FFFFFFF;

    DBG(1, (DBGOUTPUT, "R/cx:%6d ",bits));

    if (p->a1 == 0 && p->a2 == 0) {
        DBG(1, (DBGOUTPUT, " QP: xx xx D:  ==== newQP: %2d\n", qp));
        return qp;
    }
    if (bits <= 0) {
        qp = MIN(31, MAX(1, qp + QP_DELTA));
        DBG(1, (DBGOUTPUT, " QP: xx xx D:  ---- newQP: %2d\n", qp));
        return qp;
    }

    do {
        tmp  = DIV(p->a1, 2 * qp);
        tmp += DIV(p->a2, 4 * qp * qp);
        diff = ABS(tmp - bits);

        if (diff < diff_best) {
            diff_best = diff;
            qp_best   = qp;
            if ((tmp - bits) <= 0) {
                if (qp < 2) {
                    break;
                }
                qp--;
            } else {
                if (qp > 30) {
                    break;
                }
                qp++;
            }
        } else {
            break;
        }
    } while ((qp >= 1) && (qp <= 31));
    qp = qp_best;

    DBG(1, (DBGOUTPUT, " QP: %2d  Diff: %5d", qp, diff_best));
    DBG(1, (DBGOUTPUT, " newQP: %2d ", qp));

    /* Limit Qp change for smoother visual quality */
    tmp = qp - p->qp_prev;
    if (tmp > QP_DELTA) {
        qp = p->qp_prev + QP_DELTA;
    } else if (tmp < -QP_DELTA) {
        qp = p->qp_prev - QP_DELTA;
    }

    DBG(1, (DBGOUTPUT, "(limited to %d)\n", qp));

    return qp;
}

/*------------------------------------------------------------------------------
  update_tables()  only statistics of PSLICE, please.
------------------------------------------------------------------------------*/
static void update_tables(linReg_s *p, i32 qp, i32 bits)
{
    const i32 clen = 10;
    i32 tmp = p->pos;

    p->qp_prev   = qp;
    p->qs[tmp]   = 2 * qp;
    p->bits[tmp] = bits;

    if (++p->pos >= clen) {
        p->pos = 0;
    }
    if (p->len < clen) {
        p->len++;
    }
}

/*------------------------------------------------------------------------------
            update_model()  Update model parameter by Linear Regression.
------------------------------------------------------------------------------*/
static void update_model(linReg_s *p)
{
    i32 *qs = p->qs, *r = p->bits, n = p->len;
    i32 i, a1, a2, sx = lin_sx(qs, n), sy = lin_sy(qs, r, n);

    for (i = 0; i < n; i++) {
        DBG(2, (DBGOUTPUT, "model: qs %i  r %i\n",qs[i], r[i]));
    }

    a1 = lin_sxy(qs, r, n);
    a1 = a1 < I32_MAX / n ? a1 * n : I32_MAX;

    if (sy == 0) {
        a1 = 0;
    } else {
        a1 -= sx < I32_MAX / sy ? sx * sy : I32_MAX;
    }

    a2 = (lin_nsxx(qs, n) - (sx * sx));
    if (a2 == 0) {
        if (p->a1 == 0) {
            /* If encountered in the beginning */
            a1 = 0;
        } else {
            a1 = (p->a1 * 2) / 3;
        }
    } else {
        a1 = EncCalculate(a1, DSCY, a2);
    }

    /* Value of a1 shouldn't be excessive (small) */
    a1 = MAX(a1, -262144);
    a1 = MIN(a1,  262143);

    ASSERT(ABS(a1) * sx >= 0);
    ASSERT(sx * DSCY >= 0);
    ASSERT(sy * DSCY + ABS(a1) * sx >= 0);
    a2 = DIV(sy * DSCY - a1 * sx, n);

    DBG(2, (DBGOUTPUT, " a2:%9d ,a1:%8d\n", a2, a1));

    if (p->len > 0) {
        p->a1 = a1;
        p->a2 = a2;
    }
}

/*------------------------------------------------------------------------------
  lin_sy()  calculate value of Sy for n points.
------------------------------------------------------------------------------*/
static i32 lin_sy(i32 *qp, i32 *r, i32 n)
{
    i32 sum = 0;

    while (n--) {
        sum += qp[n] * qp[n] * r[n];
        if (sum < 0) {
            return I32_MAX / DSCY;
        }
    }
    return DIV(sum, DSCY);
}

/*------------------------------------------------------------------------------
  lin_sx()  calculate value of Sx for n points.
------------------------------------------------------------------------------*/
static i32 lin_sx(i32 *qp, i32 n)
{
    i32 tmp = 0;

    while (n--) {
        ASSERT(qp[n]);
        tmp += qp[n];
    }
    return tmp;
}

/*------------------------------------------------------------------------------
  lin_sxy()  calculate value of Sxy for n points.
------------------------------------------------------------------------------*/
static i32 lin_sxy(i32 *qp, i32 *r, i32 n)
{
    i32 tmp, sum = 0;

    while (n--) {
        tmp = qp[n] * qp[n] * qp[n];
        if (tmp > r[n]) {
            sum += DIV(tmp, DSCY) * r[n];
        } else {
            sum += tmp * DIV(r[n], DSCY);
        }
        if (sum < 0) {
            return I32_MAX;
        }
    }
    return sum;
}

/*------------------------------------------------------------------------------
  lin_nsxx()  calculate value of n * Sxy for n points.
------------------------------------------------------------------------------*/
static i32 lin_nsxx(i32 *qp, i32 n)
{
    i32 tmp = 0, sum = 0, d = n ;

    while (n--) {
        tmp = qp[n];
        tmp *= tmp;
        sum += d * tmp;
    }
    return sum;
}

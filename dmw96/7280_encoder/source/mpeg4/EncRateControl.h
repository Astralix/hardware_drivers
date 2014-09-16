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
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncRateControl.h,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_RATE_CONTROL__
#define __ENC_RATE_CONTROL__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncVideoObjectPlane.h"
#include "EncMbHeader.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define CTRL_LEVELS     7
#define CHECK_POINTS    10

typedef struct {
    i32  a1;               /* model parameter */
    i32  a2;               /* model parameter */
    i32  qp_prev;          /* previous QP */
    i32  qs[15];           /* quantization step size */
    i32  bits[15];         /* Number of bits needed to code residual */
    i32  pos;              /* current position */
    i32  len;              /* current lenght */
    i32  zero_div;         /* a1 divisor is 0 */
} linReg_s;


typedef struct
{
    i32 wordError[CTRL_LEVELS]; /* Check point error bit */
    i32 qpChange[CTRL_LEVELS];  /* Check point qp difference */
    i32 wordCntTarget[CHECK_POINTS];    /* Required bit count */
    i32 wordCntPrev[CHECK_POINTS];  /* Real bit count */
    u32 checkPointDistance;
} qpCtrl_s;

/* Very accurate virtual buffer, ready to use. */
typedef struct
{
    i32 bitPerSecond;   /* Input bit rate per second */
    i32 bitPerVop;  /* Bit per vop */
    true_e setFirstVop; /* Allow set first vop parameters */
    i32 vopTimeInc; /* TimeInc since last coded Vop */
    i32 vopTimeIncRes;  /* Input frame rate numerator */
    i32 timeInc;    /* Input frame rate denominator */
    i32 virtualBitCnt;  /* Virtual (channel) bit count */
    i32 driftPerVop;    /* Bit drift per vop */
    i32 realBitCnt; /* Real world bit usage */
    i32 bitAvailable;   /* Bit count of next vop */
    true_e skipBuffFlush;   /* Skip buffer is in use */
    i32 skipBuffBit;    /* Skip limit buffer */
    i32 targetPicSize;       /* target number of bits per picture */
    i32 gopRem;
    i32 nonZeroTarget;
} virtualBuffer_s;

/* Video buffer verifier */
typedef struct
{
    true_e vbv; /* Video buffer verifier is in use */
    true_e underflow;   /* Next vop must be Intra */
    true_e overflow;    /* Vbv overFlow, you just can't recover this */
    i32 stuffWordCnt;   /* Minimun bit per vop */
    i32 stuffBitCnt;    /* Added stuffing bits */
    i32 vopTimeInc; /* Used timeInc beginnin of coded vop */
    i32 vopTimeIncRes;  /* Input frame rate numerator */
    i32 timeInc;    /* Current timeInc beginnin of previous vop */
    i32 bitPerSecond;   /* Bit rate */
    i32 bitPerVop;  /* Bit per vop */
    i32 videoBufferBitCnt;  /* Video buffer verifier bit count */
    i32 videoBufferSize;    /* Video buffer verifier size */
} vbv_s;

typedef struct
{
    true_e vopRc;   /* Vop header qp can vary, src. model rc */
    true_e mbRc;    /* Mb header qp can vary, check point rc */
    true_e vopSkip; /* Vop Skip enable */
    true_e stuffEnable; /* Stuff macrobock enable */

    i32 mbPerPic; /* Number of macroblock per VOP */
    i32 coeffCnt;   /* Number of coeff per VOP */
    i32 srcPrm; /* Source parameter */

    i32 zeroCnt;    /* Zero coefficient counter */
    i32 nzCnt;  /* Non zero coeff count, exluding dc coeff if separated */
    i32 nzCntReq;   /* Non zero target for mb ratecontrol */
    i32 qpSum;  /* Qp sum counter */
    i32 bitCntLastVop;  /* Bit count of last coded vop */

    vopType_e vopTypeCur;   /* Vop type */
    vopType_e vopTypePrev;  /* Vop type */
    true_e vopCoded;    /* Vop coded information */

    i32 qpHdr;  /* Vop header qp of current voded VOP */
    i32 qpMin;  /* Vop header minimum qp, user set */
    i32 qpMax;  /* Vop header maximum qp, user set */
    i32 qpHdrPrev;   /* Vop header qp of previous voded VOPs */
    i32 fixedQp;   /* Vop header qp at RC Init */

    u32 vopCount;
    u32 predictionCount;

    qpCtrl_s qpCtrl;
    virtualBuffer_s virtualBuffer;
    vbv_s videoBuffer;
    /* new rate control */
    linReg_s linReg;       /* Data for R-Q model */
    linReg_s overhead;
    linReg_s rError;       /* Rate prediction error (bits) */
    i32 targetPicSize;
    i32 sad;
    i32 mbRows;
    i32 frameBitCnt;
    /* for gop rate control */
    i32 gopQpSum;
    i32 gopQpDiv;
    u32 frameCnt;
    i32 gopLen;
    true_e roiRc;
    i32 roiQpHdr;
    i32 roiQpDelta;
    i32 roiStart;
    i32 roiLength;
} rateControl_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
bool_e EncRcInit(rateControl_s * rc);
void EncBeforeVopRc(rateControl_s * rc, i32 timeInc);
void EncAfterVopRc(rateControl_s * rc, i32 zeroCnt, i32 byteCnt, i32 qpSum);
i32 EncCalculate(i32 a, i32 b, i32 c);

#endif

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
--  $RCSfile: EncMbHeader.h,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_MB_HEADER_H__
#define __ENC_MB_HEADER_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncVideoObjectPlane.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef enum
{
    ZIGZAG = 0,
    HORIZONTAL = 1,
    VERTICAL = 2
} scan_e;

typedef enum
{
    INTER = 0,
    INTER_Q = 1,
    INTER4V = 2,
    INTRA = 3,
    INTRA_Q = 4,
    STUFFING = 5
} type_e;

typedef struct
{
    i8 x[4];    /* INTER4V */
    i8 y[4];
    /*type_e    type; *//* Macroblock type after motion estimation */
} mv_s;

typedef struct
{
    i32 dc[6];  /* Quantized Dc-coeffisients */
    i32 qp; /* Quantization parameter */
} pred_s;

typedef struct
{
    vopType_e vopType;  /* Table 6-20 */
    true_e svHdr;   /* ENCHW_YES/ENCHW_NO */
    true_e rvlc;    /* ENCHW_YES/ENCHW_NO */
    i32 fCode;  /* Table 7-5 */
    /*true_e    acPred; *//* ENCHW_YES/ENCHW_NO */
    i32 intraDcVlcThr;  /* Table 6-21 */
    u32 vpSize; /* Video packet size */
    u32 bitCntVp;   /* Maximum bits before next video packets */
    /*i32   zeroCnt; *//* Number of zero coeff, rate controll stuff */
    u32 width;  /* Width of reference image */
    u32 height; /* Height of reference image */

    mv_s *mv;   /* Motion vector */
    i32 zeroBlock[6];   /* Zero block */
    i32 dc[6];  /* Macroblock DC */
    scan_e scan[6]; /* Scan direction */
    pred_s *predAbove;  /* Coeff store because of prediction */
    pred_s *predCurrent;    /* Coeff store because of prediction */
    type_e *typeAbove;  /* Mb type store */
    type_e *typeCurrent;    /* Mb type store */
    type_e type;    /* Macroblock type */
    /*i32   *data; *//* Macroblock data */
    /*u8    *pred; *//* Predictor data */
    u32 mbNum;  /* Current macroblock number */
    u32 mbPerVop;   /* Number of macroblock per VOP */
    u32 vpMbNumBits;
    u32 vpMbNum;    /* First mb number of video packet */
    i32 lastColumn; /* Length of macroblock column */
    i32 row;    /* Current macroblock row */
    i32 column; /* Current macroblock column */
    i32 notCoded;   /* Not coded Macroblock */
    i32 dQuant; /* dquant parameter */
    i32 qp; /* Quantization parameter */
    i32 qpLastCoded;    /* Last coded quantization parameter */
    i32 acPredFlag; /* Macroblock based ac-prediction flag */
    i32 intraDcVlc; /* Intra dc coeff are coded separately */
    i32 runningQp;  /* Running Qp, Table 6-21 */
    i32 rlcCount[6];    /* RLC count for each block */
    const u32 *rlc[6];  /* Pointer to the RLC data for each block */
    i32 *vpSizes;
    i32 vpSizeSpace;
    u32 vpNum;
} mb_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
bool_e EncMbAlloc(mb_s * mb);
void EncMbFree(mb_s * mb);
void EncMbHeader(stream_s *, mb_s *);
void EncMbHeaderIntraDp(stream_s *, stream_s *, mb_s *, true_e);
void EncMbHeaderInterDp(stream_s *, stream_s *, mb_s *, true_e);
void EncStuffing(stream_s *, vopType_e);

#endif

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
--  $RCSfile: EncPrediction.c,v $
--  $Revision: 1.1 $
--
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncPrediction.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* macro to perform division with rounding, divisor has to be positive. Macro
 * is tested to work for all dc-scaling and quantization purposes, should not
 * be used to anything else unless carefully tested */
#define DIV_W_ROUND(a,b) (((a) + ((a)<0 ? -1 : 1)*((i32)((b)>>1)))/(i32)(b))

static const u32 dcScalerLum[32] = {
    0, 8, 8, 8, 8, 10,12,14,16,17,18,19,20,21,22,23,24,25,26,27,28,29,
    30,31,32,34,36,38,40,42,44,46
};

static const u32 dcScalerCh[32]  = {
    0, 8, 8, 8, 8, 9, 9, 10,10,11,11,12,12,13,13,14,14,15,15,16,16,17,
    17,18,18,19,20,21,22,23,24,25
};


/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 GetLeftMb(true_e, pred_s *, i32, i32);
static i32 GetAboveMb(true_e, pred_s *, i32, i32);
static i32 GetAboveLeftMb(true_e, pred_s *, i32, i32);
static i32 CurrentMb(pred_s *, i32, i32);
static true_e LeftValid(true_e, type_e *, i32);
static true_e AboveValid(true_e, type_e *, i32, i32);
static true_e AboveLeftValid(true_e, type_e *, i32, i32);
static scan_e DcPrediction(i32, i32, i32, i32 *, u32);

/*------------------------------------------------------------------------------

    EncPrediction

    Input:
        mb      pointer to macroblock structure

    Output:
        mb->dc  DC-coefficients after prediction are stored here

------------------------------------------------------------------------------*/
void EncPrediction(mb_s *mb)
{
    i32 candA;
    i32 candB;
    i32 candC;
    i32 tmp;
    i32 col = mb->column;
    i32 row = mb->row;
    scan_e  *scan = mb->scan;
    true_e left = ENCHW_NO;
    true_e aboveLeft = ENCHW_NO;
    true_e above = ENCHW_NO;
    true_e leftValid;
    true_e aboveLeftValid;
    true_e aboveValid;

    i32 *dc = mb->dc;
    
    ASSERT((mb->type == INTRA) || (mb->type == INTRA_Q));
    
    /* Update current macroblock prediction parameter's */
    {
        i32 i;
        i32 *predDc = mb->predCurrent[col].dc;
        const i32 *mbDc = dc;
        for(i = 6; i > 0; i--)
        {
            *predDc++ = *mbDc++;
        }
    }

    mb->predCurrent[col].qp = mb->qp; 

    /* Left predictor macroblock is out of video packet */
    if (mb->vpMbNum+1 > mb->mbNum) {
        left = ENCHW_YES;
    }
    /* Above predictor macroblock is out of video packet */
    if (mb->vpMbNum+mb->lastColumn > mb->mbNum) {
        above = ENCHW_YES;
    }
    /* Above left predictor macroblock is out of video packet */
    if (mb->vpMbNum+mb->lastColumn+1 > mb->mbNum) {
        aboveLeft = ENCHW_YES;
    }

    /* Neighborhood macroblock valid? */
    leftValid = LeftValid(left, mb->typeCurrent, col);
    aboveLeftValid = AboveLeftValid(aboveLeft, mb->typeAbove, row, col);
    aboveValid = AboveValid(above, mb->typeAbove, row, col);

    /* Block 0 */
    candA = GetLeftMb(leftValid, mb->predCurrent, 1, col);
    candB = GetAboveLeftMb(aboveLeftValid, mb->predAbove,3 ,col);
    candC = GetAboveMb(aboveValid, mb->predAbove, 2,col);
    (void)DcPrediction(candA, candB, candC, dc, dcScalerLum[mb->qp]);
    tmp = candA; /* This is candB of block 2 */

    /* Block 1 */
    candA = CurrentMb(mb->predCurrent, 0, col);
    candB = candC;
    candC = GetAboveMb(aboveValid, mb->predAbove, 3, col);
    (void)DcPrediction(candA, candB, candC, dc+1, dcScalerLum[mb->qp]);

    /* Block 2 */
    candC = candA;
    candA = GetLeftMb(leftValid, mb->predCurrent, 3, col);
    (void)DcPrediction(candA, tmp, candC, dc+2, dcScalerLum[mb->qp]);

    /* Block 3 */
    candA = CurrentMb(mb->predCurrent, 2, col);
    candB = candC;
    candC = CurrentMb(mb->predCurrent, 1, col);
    (void)DcPrediction(candA, candB, candC, dc+3, dcScalerLum[mb->qp]);

    /* Block 4 */
    candA = GetLeftMb(leftValid, mb->predCurrent, 4, col);
    candB = GetAboveLeftMb(aboveLeftValid, mb->predAbove, 4, col);
    candC = GetAboveMb(aboveValid, mb->predAbove, 4, col);
    (void)DcPrediction(candA, candB, candC, dc+4, dcScalerCh[mb->qp]);

    /* Block 5 */
    candA = GetLeftMb(leftValid, mb->predCurrent, 5, col);
    candB = GetAboveLeftMb(aboveLeftValid, mb->predAbove, 5, col);
    candC = GetAboveMb(aboveValid, mb->predAbove, 5, col);
    (void)DcPrediction(candA, candB, candC, dc+5, dcScalerCh[mb->qp]);


    (void)scan;
    /* No ac-prediction */
#if 0 /* all these are zero initialized by calloc */
    scan[0] = scan[1] = scan[2] = scan[3] = scan[4] = scan[5] = ZIGZAG;
    mb->acPredFlag = 0;
#endif
    return;
}

/*------------------------------------------------------------------------------

    LeftValid

------------------------------------------------------------------------------*/
true_e LeftValid(true_e vpOut, type_e *type, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if ((column == 0) || (vpOut == ENCHW_YES)) {
        return ENCHW_NO;
    }

    /* Macroblock is INTER */
    column -= 1;
    if (!((type[column] == INTRA) || (type[column] == INTRA_Q))) {
        return ENCHW_NO;
    }

    return ENCHW_YES;
}

/*------------------------------------------------------------------------------

    AboveValid

------------------------------------------------------------------------------*/
true_e AboveValid(true_e vpOut, type_e *type, i32 row, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if ((row == 0) || (vpOut == ENCHW_YES)) {
        return ENCHW_NO;
    }

    /* Macroblock is INTER */
    if (!((type[column] == INTRA) || (type[column] == INTRA_Q))) {
        return ENCHW_NO;
    }

    return ENCHW_YES;
}

/*------------------------------------------------------------------------------

    AboveLeftValid

------------------------------------------------------------------------------*/
true_e AboveLeftValid(true_e vpOut, type_e *type, i32 row, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if ((row == 0) || (column == 0) || (vpOut == ENCHW_YES)) {
        return ENCHW_NO;
    }

    /* Macroblock is INTER */
    column -= 1;
    if (!((type[column] == INTRA) || (type[column] == INTRA_Q))) {
        return ENCHW_NO;
    }

    return ENCHW_YES;
}



/*------------------------------------------------------------------------------

    GetLeftMb

------------------------------------------------------------------------------*/
i32 GetLeftMb(true_e valid, pred_s *pred, i32 blockNum, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if (valid == ENCHW_NO) {
        return 1024;
    }

    /* Macroblock is valid */
    column -= 1;
    if (blockNum < 4) {
        return pred[column].dc[blockNum]*dcScalerLum[pred[column].qp];
    } else {
        return pred[column].dc[blockNum]*dcScalerCh[pred[column].qp];
    }
}

/*------------------------------------------------------------------------------

    GetAboveMb

------------------------------------------------------------------------------*/
i32 GetAboveMb(true_e valid, pred_s *pred, i32 blockNum, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if (valid == ENCHW_NO) {
        return 1024;
    }

    /* Macroblock is valid */
    if (blockNum < 4) {
        return pred[column].dc[blockNum]*dcScalerLum[pred[column].qp];
    } else {
        return pred[column].dc[blockNum]*dcScalerCh[pred[column].qp];
    }
}

/*------------------------------------------------------------------------------

    GetAboveLeftMb

------------------------------------------------------------------------------*/
i32 GetAboveLeftMb(true_e valid, pred_s *pred, i32 blockNum, i32 column)
{
    /* Macroblock is out of Vop or video packet */
    if (valid == ENCHW_NO) {
        return 1024;
    }

    /* Macroblock is valid */
    column -= 1;
    if (blockNum < 4) {
        return pred[column].dc[blockNum]*dcScalerLum[pred[column].qp];
    } else {
        return pred[column].dc[blockNum]*dcScalerCh[pred[column].qp];
    }
}

/*------------------------------------------------------------------------------

    CurrentMb

------------------------------------------------------------------------------*/
i32 CurrentMb(pred_s *pred, i32 blockNum, i32 column)
{
    ASSERT(blockNum < 4);
    return pred[column].dc[blockNum]*dcScalerLum[pred[column].qp];
}

/*------------------------------------------------------------------------------

    DcPrediction

    Function selects prediction direction and does DC prediction.

    Input:
        fa          inverse quantized Dc-component of left block
        fb          inverse quantized Dc-component of left above block
        fc          inverse quantized Dc-component of above block
        pDc         quantized Dc-component of current block
        dcScaler    Dc-scaler of current block

    Output:
        pDc         Predicted Dc-component of current block

    Return:
        Scan direction

------------------------------------------------------------------------------*/
scan_e DcPrediction(i32 fa, i32 fb, i32 fc, i32 *pDc, u32 dcScaler)
{
    /* Define prediction direction and predict */
    if (ABS(fa-fb) < ABS(fb-fc))
    {
        i32 tmp = *pDc;
        *pDc = tmp - DIV_W_ROUND(fc, dcScaler);
        return HORIZONTAL;
    }
    else
    {
        i32 tmp = *pDc;
        *pDc = tmp - DIV_W_ROUND(fa, dcScaler);
        return VERTICAL;
    }
}



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
--  $RCSfile: EncDifferentialMvCoding.c,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncDifferentialMvCoding.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	true_e  valid;
	i32 x;
	i32 y;
} candidate_s;

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static candidate_s GetLeftMb(true_e, mv_s *, type_e *, i32, i32, i32);
static candidate_s GetAboveMb(true_e, mv_s *, type_e *,i32, i32, i32, i32, i32);
static candidate_s GetAboveRightMb(true_e, mv_s *, type_e *,i32, i32, i32, i32);
static candidate_s GetCurrentMb(mv_s *, i32, i32);
static mvData_s MvEncoding(i32, i32, i32);
static i32 MedianFilter(i32, i32, i32);

/*------------------------------------------------------------------------------

   5.1  Function name: EncDifferentialMvEncoding

------------------------------------------------------------------------------*/
dmv_s EncDifferentialMvEncoding(mb_s *mb, i32 block)
{
	i32 valid = 0;
	i32 tmp = 0;
	true_e left = ENCHW_NO;
	true_e aboveRight = ENCHW_NO;
	true_e above = ENCHW_NO;
	candidate_s candidate[3];
	i32 predictor;
	dmv_s dmv;
	i32 i;

	/* Left candidate macroblock is out of video packet */
	if (mb->vpMbNum+1 > mb->mbNum) {
		left = ENCHW_YES;
	}
	/* Above candidate macroblock is out of video packet */
	if (mb->vpMbNum+mb->lastColumn > mb->mbNum) {
		above = ENCHW_YES;
	}
	/* Above right candidate macroblock is out of video packet */
	if (mb->vpMbNum+mb->lastColumn-1 > mb->mbNum) {
		aboveRight = ENCHW_YES;
	}

	/* Candidate vector */
	switch (block) {
		case 0:
			candidate[0] = GetLeftMb(left, mb->mv, mb->typeCurrent,
					1, mb->mbNum, mb->column);
			candidate[1] = GetAboveMb(above, mb->mv, mb->typeAbove,
					2, mb->mbNum, mb->row, mb->column,
					mb->lastColumn);
			candidate[2] = GetAboveRightMb(aboveRight, mb->mv,
					mb->typeAbove, mb->mbNum, mb->row,
					mb->column, mb->lastColumn);
			break;
		case 1:
			candidate[0] = GetCurrentMb(mb->mv, 0, mb->mbNum);
			candidate[1] = GetAboveMb(above, mb->mv, mb->typeAbove,
					3, mb->mbNum, mb->row, mb->column,
					mb->lastColumn);
			candidate[2] = GetAboveRightMb(aboveRight, mb->mv,
					mb->typeAbove, mb->mbNum, mb->row,
					mb->column, mb->lastColumn);
			break;
		case 2:
			candidate[0] = GetLeftMb(left, mb->mv, mb->typeCurrent,
					3, mb->mbNum, mb->column);
			candidate[1] = GetCurrentMb(mb->mv, 0, mb->mbNum);
			candidate[2] = GetCurrentMb(mb->mv, 1, mb->mbNum);
			break;
		default:	/* Block 3 */
			candidate[0] = GetCurrentMb(mb->mv, 2, mb->mbNum);
			candidate[1] = GetCurrentMb(mb->mv, 0, mb->mbNum);
			candidate[2] = GetCurrentMb(mb->mv, 1, mb->mbNum);
			break;
	}

	/* If two and only two candidate predictors are not valid */
	/* they are set to the third candidate predictor. */
	for (i = 0; i < 3; i++) {
		if (candidate[i].valid == ENCHW_YES) {
			valid++;
			tmp = i;
		}
	}
	if (valid == 1) {
		candidate[0].x = candidate[1].x = candidate[2].x =
			candidate[tmp].x;
		candidate[0].y = candidate[1].y = candidate[2].y =
			candidate[tmp].y;
	}

	/* Horizontal motion vector prediction */
	predictor = MedianFilter(candidate[0].x, candidate[1].x,
			candidate[2].x);
	dmv.x = MvEncoding(predictor, mb->mv[mb->mbNum].x[block], mb->fCode);

	/* Vertical motion vector prediction */
	predictor = MedianFilter(candidate[0].y, candidate[1].y,
			candidate[2].y);
	dmv.y = MvEncoding(predictor, mb->mv[mb->mbNum].y[block], mb->fCode);

	return dmv;
}

/*------------------------------------------------------------------------------

   5.2  Function name: GetLeftMb

------------------------------------------------------------------------------*/
candidate_s GetLeftMb(true_e vpOut, mv_s *mv, type_e *type, i32 block, i32
		mbNum, i32 column)
{
	candidate_s candidate = {ENCHW_NO,0,0};

	/* Macroblock is out of Vop */
	if ((column == 0) || (vpOut == ENCHW_YES)) {
		return candidate;
	}

	/* Macroblock is INTRA */
	column -= 1;
	if ((type[column] == INTRA) || (type[column] == INTRA_Q)) {
		candidate.valid = ENCHW_YES;
		return candidate;
	}

	/* Macroblock is valid */
	candidate.valid = ENCHW_YES;
	candidate.x = mv[mbNum-1].x[block];
	candidate.y = mv[mbNum-1].y[block];

	return candidate;
}

/*------------------------------------------------------------------------------

   5.3  Function name: GetAboveMb

------------------------------------------------------------------------------*/
candidate_s GetAboveMb(true_e vpOut, mv_s *mv, type_e *type, i32 block, i32
		mbNum, i32 row, i32 column, i32 lastColumn)
{
	candidate_s candidate = {ENCHW_NO,0,0};

	/* Macroblock is out of Vop */
	if ((row == 0) || (vpOut == ENCHW_YES)) {
		return candidate;
	}

	/* Macroblock is INTRA */
	if ((type[column] == INTRA) || (type[column] == INTRA_Q)) {
		candidate.valid = ENCHW_YES;
		return candidate;
	}

	/* Macroblock is valid */
	candidate.valid = ENCHW_YES;
	candidate.x = mv[mbNum-lastColumn].x[block];
	candidate.y = mv[mbNum-lastColumn].y[block];

	return candidate;
}

/*------------------------------------------------------------------------------

   5.4  Function name: GetAboveRightMb

------------------------------------------------------------------------------*/
candidate_s GetAboveRightMb(true_e vpOut, mv_s *mv, type_e *type, i32 mbNum,
		i32 row, i32 column, i32 lastColumn)
{
	candidate_s candidate = {ENCHW_NO,0,0};

	if ((row == 0) || (column == (lastColumn-1)) || (vpOut == ENCHW_YES)) {
		return candidate;
	}

	/* Macroblock is INTRA */
	if ((type[column+1] == INTRA) || (type[column+1] == INTRA_Q)) {
		candidate.valid = ENCHW_YES;
		return candidate;
	}

	/* Macroblock is valid */
	candidate.valid = ENCHW_YES;
	candidate.x = mv[mbNum-lastColumn+1].x[2];
	candidate.y = mv[mbNum-lastColumn+1].y[2];

	return candidate;
}

/*------------------------------------------------------------------------------

   5.5  Function name: GetCurrentMb

------------------------------------------------------------------------------*/
candidate_s GetCurrentMb(mv_s *mv, i32 block, i32 mbNum)
{
	candidate_s candidate;

	/* Macroblock is valid */
	candidate.valid = ENCHW_YES;
	candidate.x = mv[mbNum].x[block];
	candidate.y = mv[mbNum].y[block];

	return candidate;
}

/*------------------------------------------------------------------------------

   5.6  Function name: MvEncoding

        Purpose: Calculate mv_data and mv_residual
                 ( See 7.6.3 General motion vector decoding process )

        Input:  predictor   Value of motion vector predictor
                mv          Value of current motion vector component
                fCode       Value of vop_fcode_forvard

------------------------------------------------------------------------------*/
mvData_s MvEncoding(i32 predictor, i32 mv, i32 fCode)
{
	mvData_s mvData;
	i32 f, high, low, range;
	i32 mvd;

	/* Motion vector prediction */
	f = 1 << (fCode-1);
	high  =  32*f-1;
	low   = -32*f;
	range =  64*f;

	/* Differential motion vector */
	mvd = mv - predictor;

	/* Scale range */
	if (mvd < low) {
		mvd += range;
	}

	if (mvd > high) {
		mvd -= range;
	}

	/* Calculate mvData and mvResidual */
	if (mvd == 0 || f == 1) {
		mvData.data     = mvd;
		mvData.residual = 0;
	} else {
		mvData.data     = (ABS(mvd)+f-1) / f;
		mvData.residual =  ABS(mvd)+f - f*mvData.data - 1;
		if (mvd < 0) {
			mvData.data = -mvData.data;
		}
	}

	return mvData;
}

/*------------------------------------------------------------------------------

   5.7  Function name: MedianFilter

        Purpose: median filtering of three input values

        Input:
            a,b,c   input values to be median filtered

        Output:
            median of three input values

------------------------------------------------------------------------------*/
i32 MedianFilter(i32 a, i32 b, i32 c)
{
	i32 max,min,med;
	max = min = med = a;

	if (b > max) {
		max = b;
	} else if (b < min) {
		min = b;
	}

	if (c > max) {
		med = max;
	} else if (c < min) {
		med = min;
	} else {
		med = c;
	}

	return med;
}

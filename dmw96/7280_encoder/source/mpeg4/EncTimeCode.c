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
--  $RCSfile: EncTimeCode.c,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncTimeCode.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 Calculate(i32, i32, i32);

/*------------------------------------------------------------------------------

	EncTimeIncrement

------------------------------------------------------------------------------*/
void EncTimeIncrement(timeCode_s *timeCode, i32 timeInc)
{
	i32 time;
	i32 tmp;

	time = timeInc;
	timeCode->timeInc = timeInc;

	if (timeCode->fixedVopRate == ENCHW_YES) {
		timeCode->vopTimeInc += timeCode->fixedVopTimeInc;
	} else {
		timeCode->vopTimeInc += time;
	}

	/* Temporal reference */
	if (timeCode->svHdr == ENCHW_YES) {
		/* One stamp of temporal reference is (1001/30000)s */
		/* 34987 ~ ((1001/30000)*2^20) */
		tmp = Calculate(time, 0xFFFFF, timeCode->vopTimeIncRes);
		tmp += timeCode->tempRefRem; 
		while (tmp >= 34987) {
			timeCode->tempRef++;
			tmp -= 34987;
		}
		timeCode->tempRefRem = tmp;
		timeCode->tempRef &= 0xFF;
	}

	return;
}

/*------------------------------------------------------------------------------

	EncTimeCode

	GoVop header time code based on previous vop time code, so I do not
	reset timeCode->moduloTimeBase = 0. This is because of API: we write
	GoVop header before we know final time code of curret vop, after we
	have write GoVop header, next vop might be over one second of previous
	vop and that why we need moduloTimeBase, I know, standard told me to
	reset moduloTimeBase after GoVop header, but then time code of GoVop
	header must based on curret vop header and that is impossible.

------------------------------------------------------------------------------*/
void EncTimeCode(timeCode_s *timeCode, govop_s *govop)
{
	/* vopTimeInc must be range of [0, vopTimeIncRes) */
	timeCode->moduloTimeBase = 0;
	while (timeCode->vopTimeInc >= timeCode->vopTimeIncRes) {
		timeCode->vopTimeInc -= timeCode->vopTimeIncRes;
		timeCode->moduloTimeBase++;
		timeCode->timeCodeSecond++;
		if (timeCode->timeCodeSecond > 59) {
			timeCode->timeCodeMinutes++;
			timeCode->timeCodeSecond = 0;
			if (timeCode->timeCodeMinutes > 59) {
				timeCode->timeCodeHours++;
				timeCode->timeCodeMinutes = 0;
				if (timeCode->timeCodeHours > 23) {
					timeCode->timeCodeHours = 0;
				}
			}
		}
	}
	/* Hundredh of second, not actually needed but nice to know */
	timeCode->secondHundredth = timeCode->vopTimeInc*100 /
		timeCode->vopTimeIncRes;

	/* GoVop header time code */
	govop->timeCodeHours = timeCode->timeCodeHours;
	govop->timeCodeMinutes = timeCode->timeCodeMinutes;
	govop->timeCodeSecond = timeCode->timeCodeSecond;

	return;
}

/*------------------------------------------------------------------------------

	EncCopyTimeCode

------------------------------------------------------------------------------*/
void EncCopyTimeCode(timeCode_s *dest, timeCode_s *src)
{
	dest->fixedVopRate    = src->fixedVopRate;
	dest->svHdr           = src->svHdr;
	dest->vopTimeIncRes   = src->vopTimeIncRes;
	dest->fixedVopTimeInc = src->fixedVopTimeInc;
	dest->timeInc         = src->timeInc;
	dest->vopTimeInc      = src->vopTimeInc;
	dest->moduloTimeBase  = src->moduloTimeBase;
	dest->tempRef         = src->tempRef;
	dest->tempRefRem      = src->tempRefRem;
	dest->timeCodeSecond  = src->timeCodeSecond;
	dest->timeCodeMinutes = src->timeCodeMinutes;
	dest->timeCodeHours   = src->timeCodeHours;
	dest->secondHundredth = src->secondHundredth;

	return;
}

/*------------------------------------------------------------------------------

	EncTimeCodeCheck

------------------------------------------------------------------------------*/
bool_e EncTimeCodeCheck(timeCode_s *timeCode)
{
	if (timeCode->vopTimeIncRes > 65535) {
		return ENCHW_NOK;
	}
	if (timeCode->timeInc > 65535) {
		return ENCHW_NOK;
	}

	return ENCHW_OK;
}

/*------------------------------------------------------------------------------

	Calculate

	I try to avoid overflow and calculate good enough result of a*b/c.

------------------------------------------------------------------------------*/
i32 Calculate(i32 a, i32 b, i32 c)
{
	i32 left = 31;
	i32 right = 0;
	i32 sift;
	i32 sign = 1;
	i32 tmp;

	if (a < 0) {
		sign = -1;
		a = -a;
	}
	if (b < 0) {
		sign *= -1;
		b = -b;
	}
	if (c < 0) {
		sign *= -1;
		c = -c;
	}

	if (c == 0) {
		return 0x7FFFFFFF * sign;
	}

	if (b > a) {
		tmp = b;
		b = a;
		a = tmp;
	}

	while (((a<<left)>>left) != a) left--;
	while ((b>>right++) > c);
	sift = left - right;

	return (((a<<sift) / c * b)>>sift)*sign;
}

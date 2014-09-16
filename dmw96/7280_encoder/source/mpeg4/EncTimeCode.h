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
--  $RCSfile: EncTimeCode.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_TIME_CODE_H__
#define __ENC_TIME_CODE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncGroupOfVideoObjectPlane.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef struct {
	true_e	fixedVopRate;		/* ENCHW_YES/ENCHW_NO */
	true_e	svHdr;			/* ENCHW_YES/ENCHW_NO */

	i32	vopTimeIncRes;		/* Frame rate numerator */
	i32	fixedVopTimeInc;	/* Frame rate denominator */
	i32	timeInc;		/* Time increment */

	i32	vopTimeInc;		/* MPEG4 */
	i32	moduloTimeBase;

	i32	tempRef;		/* H263 */
	i32	tempRefRem;

	i32	timeCodeSecond;		/* GoVopHdr */
	i32	timeCodeMinutes;
	i32	timeCodeHours;
	i32	secondHundredth;
} timeCode_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncTimeIncrement(timeCode_s *timeCode, i32 timeInc);
void EncTimeCode(timeCode_s *timeCode, govop_s *govop);
void EncCopyTimeCode(timeCode_s *timeCodeSrc, timeCode_s *timeCodeDest);
bool_e EncTimeCodeCheck(timeCode_s *timeCode);

#endif


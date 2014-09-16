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
--  Description :  Mpeg4 internal traces
--
--------------------------------------------------------------------------------
--  Version control information, please leave untouched.
--
--  $RCSfile: EncMpeg4Trace.h,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_TRACE_H__
#define __ENC_TRACE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "EncMbHeader.h"
#include "EncRateControl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncRcTrace(rateControl_s *rc);
void EncTraceMb(const mb_s *mb);

#endif

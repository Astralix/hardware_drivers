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
--  $RCSfile: EncCoder.h,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_CODER_H__
#define __ENC_CODER_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncMbHeader.h"
#include "encputbits.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

void EncIntra(stream_s *stream, mb_s *mb);
void EncInter(stream_s *stream, mb_s *mb);
void EncIntraDp(stream_s *stream, stream_s *stream1, stream_s *stream2,
        mb_s *mb);
void EncInterDp(stream_s *stream, stream_s *stream1, stream_s *stream2, 
        mb_s *mb);

#endif

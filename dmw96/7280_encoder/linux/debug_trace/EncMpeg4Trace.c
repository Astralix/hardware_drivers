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
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncMpeg4Trace.c,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncMpeg4Trace.h"

#include <stdio.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

static FILE *fMb = NULL;

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    EncTraceMb

------------------------------------------------------------------------------*/
void EncTraceMb(const mb_s *mb)
{
    if (fMb == NULL)
        fMb = fopen("sw_mb.trc", "w");

    if (fMb == NULL)
        return;
    
    fprintf(fMb, 
            "MB: %d\tType: %d\tQp: %d\t"
            "RlcCnt: %2d %2d %2d %2d %2d %2d",
            mb->mbNum, mb->type, mb->qp, mb->rlcCount[0], mb->rlcCount[1],
            mb->rlcCount[2], mb->rlcCount[3], mb->rlcCount[4], mb->rlcCount[5]);

    if (mb->type == INTRA)
    {
        fprintf(fMb, 
            "  Dc: %3d %3d %3d %3d %3d %3d\n", 
            mb->dc[0], mb->dc[1], mb->dc[2], mb->dc[3], mb->dc[4], mb->dc[5]);
    }
    else
    {
        fprintf(fMb,
            "  Mv: (%3d,%3d) (%3d,%3d) (%3d,%3d) (%3d,%3d)\n",
            mb->mv[mb->mbNum].x[0], mb->mv[mb->mbNum].y[0],
            mb->mv[mb->mbNum].x[1], mb->mv[mb->mbNum].y[1],
            mb->mv[mb->mbNum].x[2], mb->mv[mb->mbNum].y[2],
            mb->mv[mb->mbNum].x[3], mb->mv[mb->mbNum].y[3]);
    }
}


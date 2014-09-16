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
--  $RCSfile: EncVlc.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_VLC_H__
#define __ENC_VLC_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "encputbits.h"
#include "enccommon.h"
#include "EncMbHeader.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/* Intra DC coding type in MPEG-4 VLC: Intra DC VLC, Intra AC VLC, not coded */
typedef enum {
    DC_VLC_INTRA_DC = 1,
    DC_VLC_INTRA_AC = 2,
    DC_VLC_NO = 3
} dcVlc_e;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void EncVlcIntra(stream_s *, mb_s *, dcVlc_e);
void EncVlcIntraSvh(stream_s *, mb_s *);
void EncRvlcIntra(stream_s *, mb_s *, dcVlc_e);
void EncVlcInter(stream_s *, mb_s *);
void EncVlcInterSvh(stream_s *, mb_s *);
void EncRvlcInter(stream_s *, mb_s *);
void EncDcCoefficient(stream_s *, i32, i32);

#endif

/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract : RVLC
--
------------------------------------------------------------------------------*/

#ifndef RVLC_H_INCLUDED_
#define RVLC_H_INCLUDED_

#include "mp4dechwd_container.h"

u32 StrmDec_DecodeRvlc(DecContainer * pDecContainer, u32 mbNumber,
    u32 numMbs);

#endif /*define RVLC_H_INCLUDED_ */

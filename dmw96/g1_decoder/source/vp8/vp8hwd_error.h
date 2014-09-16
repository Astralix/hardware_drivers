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
-
-  Description : ...
-
------------------------------------------------------------------------------*/

#ifndef __VP8_ERROR_H__
#define __VP8_ERROR_H__

#include "basetype.h"
#include "vp8hwd_decoder.h"

typedef struct
{
    i32 hor;
    i32 ver;
} mv_t;

typedef struct
{
    u32 totWeight[3]; /* last, golden, alt */
    mv_t totMv[3];
} ecMv_t;

typedef struct vp8ec_t
{
    ecMv_t *mvs;
    u32 width;
    u32 height;
    u32 numMvsPerMb;
} vp8ec_t;

u32 vp8hwdInitEc(vp8ec_t *ec, u32 w, u32 h, u32 mvsPerMb);
void vp8hwdReleaseEc(vp8ec_t *ec);
void vp8hwdEc(vp8ec_t *ec, u32 *pRef, u32 *pOut, u32 startMb, u32 all);

#endif /* __VP8_ERROR_H__ */

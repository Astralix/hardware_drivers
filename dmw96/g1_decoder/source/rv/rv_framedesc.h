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
--  Abstract : algorithm header file
--
------------------------------------------------------------------------------*/

/*****************************************************************************/

#ifndef RV_FRAMEDESC_H
#define RV_FRAMEDESC_H

#include "basetype.h"

typedef struct
{
    u32 frameNumber;
    u32 frameTimePictures;
    u32 picCodingType;
    u32 totalMbInFrame;
    u32 frameWidth; /* in macro blocks */
    u32 frameHeight;    /* in macro blocks */
    u32 timeCodeHours;
    u32 timeCodeMinutes;
    u32 timeCodeSeconds;
    u32 vlcSet;
    u32 qp;
} DecFrameDesc;

#endif /* #ifndef RV_FRAMEDESC_H */

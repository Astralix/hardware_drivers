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

#ifndef DECSVDESC_H_DEFINED
#define DECSVDESC_H_DEFINED

#include "basetype.h"

typedef struct DecSvDesc_t
{
    u32 splitScreenIndicator;
    u32 documentCameraIndicator;
    u32 fullPictureFreezeRelease;
    u32 sourceFormat;
    u32 gobFrameId;
    u32 temporalReference;
    u32 tics;   /* total tics from the beginning */
    u32 numMbsInGob;
    u32 numGobsInVop;

    u32 cpcf;   /* flag indicating if custom picture clock freq used */
    u32 cpcfc;  /* custom picture clock frequency code */
    u32 etr;    /* extended temporal reference */
} DecSvDesc;

#endif

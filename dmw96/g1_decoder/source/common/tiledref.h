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
--  Abstract : Header file for stream decoding utilities
--
------------------------------------------------------------------------------*/

#ifndef TILEDREF_H_DEFINED
#define TILEDREF_H_DEFINED

#include "basetype.h"
#include "decapicommon.h"

#define TILED_REF_NONE              (0)
#define TILED_REF_8x4               (1)
#define TILED_REF_8x4_INTERLACED    (2)

u32 DecSetupTiledReference( u32 *regBase, u32 tiledModeSupport, 
                            DecDpbMode dpbMode, u32 interlacedStream );
u32 DecCheckTiledMode( u32 tiledModeSupport, DecDpbMode dpbMode,
                       u32 interlacedStream );                            

#endif /* TILEDREF_H_DEFINED */

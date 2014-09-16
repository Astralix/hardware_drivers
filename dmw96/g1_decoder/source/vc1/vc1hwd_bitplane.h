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
--  Description : interface for bitplane decoding module
--
------------------------------------------------------------------------------*/

#ifndef VC1HWD_BITPLANE_H
#define VC1HWD_BITPLANE_H

/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "vc1hwd_stream.h"
#include "vc1hwd_util.h"

/*------------------------------------------------------------------------------
    Module defines
------------------------------------------------------------------------------*/

/* Bitplane Coding Modes */
typedef enum {
    BPCM_RAW,
    BPCM_NORMAL_2,
    BPCM_DIFFERENTIAL_2,
    BPCM_NORMAL_6,
    BPCM_DIFFERENTIAL_6,
    BPCM_ROW_SKIP,
    BPCM_COLUMN_SKIP
} BitPlaneCodingMode_e;

/*------------------------------------------------------------------------------
    Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

u16x vc1hwdDecodeBitPlane( strmData_t * const strmData, const u16x colMb,
                           const u16x rowMb, u8 *pData, const u16x bit,
                           u16x * const pRawMask ,  const u16x maskbit,
                           const u16x syncMarker );

#endif /* #ifndef VC1HWD_BITPLANE_H */


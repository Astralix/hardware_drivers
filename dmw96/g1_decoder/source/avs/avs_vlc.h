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
--  Abstract : Decode Exp-Golomb code words
--
------------------------------------------------------------------------------*/

#ifndef AVS_VLC_H
#define AVS_VLC_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/

#include "basetype.h"

#include "avs_container.h"

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

u32 AvsDecodeExpGolombUnsigned(DecContainer *pDecCont, u32 *value);

u32 AvsDecodeExpGolombSigned(DecContainer *pDecCont, i32 *value);

#endif /* #ifdef AVS_VLC_H */

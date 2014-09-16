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

#ifndef __VP8_PROBS_H__
#define __VP8_PROBS_H__

#include "basetype.h"
#include "vp8hwd_bool.h"
#include "vp8hwd_decoder.h"

void vp8hwdResetProbs( vp8Decoder_t* dec );
u32  vp8hwdDecodeMvUpdate( vpBoolCoder_t *bc, vp8Decoder_t *dec );
u32  vp8hwdDecodeCoeffUpdate( vpBoolCoder_t *bc, vp8Decoder_t *dec );

#endif /* __VP8_PROBS_H__ */

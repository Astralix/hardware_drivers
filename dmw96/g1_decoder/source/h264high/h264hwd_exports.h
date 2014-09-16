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
--  Abstract : Macroblock level stream decoding and macroblock reconstruction
--
------------------------------------------------------------------------------*/
     
    
#ifndef __H264HWD_EXPORTS_H__
#define __H264HWD_EXPORTS_H__
    
#include "h264hwd_asic.h"
#include "h264hwd_storage.h"

u32 h264bsdDecodeMacroblock(storage_t * pStorage, u32 mbNum, i32 * qpY,
                                 DecAsicBuffers_t * pAsicBuff);

u32 PrepareIntraPrediction(mbStorage_t * pMb, macroblockLayer_t * mbLayer,
                             u32 constrainedIntraPred,
                             DecAsicBuffers_t * pAsicBuff);

u32 PrepareInterPrediction(mbStorage_t * pMb, macroblockLayer_t * pMbLayer,
                             dpbStorage_t * dpb, DecAsicBuffers_t * pAsicBuff);

#endif  /* __H264HWD_EXPORTS_H__ */

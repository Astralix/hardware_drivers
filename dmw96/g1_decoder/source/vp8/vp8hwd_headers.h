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

#ifndef __VP8_HEADERS_H__
#define __VP8_HEADERS_H__

#include "basetype.h"

#include "vp8hwd_decoder.h"
#include "vp8hwd_bool.h"

void vp8hwdDecodeFrameTag( const u8 *pStrm, vp8Decoder_t* dec );
u32 vp8hwdSetPartitionOffsets( const u8 *stream, u32 len, vp8Decoder_t *dec );
u32 vp8hwdDecodeFrameHeader( const u8 *pStrm, u32 strmLen, vpBoolCoder_t*bc, 
                             vp8Decoder_t* dec );

#endif /* __VP8_BOOL_H__ */

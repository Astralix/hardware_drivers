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
--  Abstract : Header file for video packet decoding functionality
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of context 

     1. xxx...
   
 
------------------------------------------------------------------------------*/

#ifndef STRMDEC_VIDEO_PACKET_H_DEFINED
#define STRMDEC_VIDEO_PACKET_H_DEFINED

#include "mp4dechwd_container.h"

u32 StrmDec_DecodeVideoPacketHeader(DecContainer * pDecContainer);
u32 StrmDec_DecodeVideoPacket(DecContainer * pDecContainer);
u32 StrmDec_CheckNextVpMbNumber(DecContainer * pDecContainer);

#endif

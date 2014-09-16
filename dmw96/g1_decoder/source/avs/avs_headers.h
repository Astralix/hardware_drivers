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
--  Abstract : 
--
------------------------------------------------------------------------------*/

#ifndef AVSSTRMDEC_DECODEHEADERS_H
#define AVSSTRMDEC_DECODEHEADERS_H

#include "basetype.h"
#include "avs_container.h"

u32 AvsStrmDec_DecodeExtensionHeader(DecContainer * pDecContainer);
u32 AvsStrmDec_DecodeSequenceHeader(DecContainer * pDecContainer);
u32 AvsStrmDec_DecodeIPictureHeader(DecContainer * pDecContainer);
u32 AvsStrmDec_DecodePBPictureHeader(DecContainer * pDecContainer);

u32 AvsStrmDec_DecodeSeqDisplayExtHeader(DecContainer * pDecContainer);

#endif /* #ifndef AVSSTRMDEC_DECODEHEADERS_H */

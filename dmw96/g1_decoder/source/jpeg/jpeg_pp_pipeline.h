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
--  Abstract : JPEG decoder and PP pipeline support
--
------------------------------------------------------------------------------*/

#ifndef JPEG_PP_PIPELINE_H
#define JPEG_PP_PIPELINE_H

#include "decppif.h"

i32 jpegRegisterPP(const void *decInst, const void *ppInst,
                   void (*PPRun) (const void *, const DecPpInterface *),
                   void (*PPEndCallback) (const void *),
                   void (*PPConfigQuery) (const void *, DecPpQuery *));

i32 jpegUnregisterPP(const void *decInst, const void *ppInst);

#endif /* #ifdef JPEG_PP_PIPELINE_H */

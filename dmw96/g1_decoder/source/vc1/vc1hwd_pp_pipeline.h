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
--  Abstract : VC-1 decoder and PP pipeline support
--
------------------------------------------------------------------------------*/

#ifndef VC1HWD_PP_PIPELINE_H
#define VC1HWD_PP_PIPELINE_H

i32 vc1RegisterPP(const void *decInst, const void *ppInst,
                   void (*PPDecStartPp) (const void *, const DecPpInterface *),
                   void (*PPDecPipelineEndCallback) (const void *),
                   void  (*PPConfigQuery)(const void *, DecPpQuery *),
		   void (*PPDisplayIndex)(const void *, u32),
		   void (*PPBufferData) (const void *, u32, u32, u32));

i32 vc1UnregisterPP(const void *decInst, const void *ppInst);

#endif /* #ifdef VC1HWD_PP_PIPELINE_H */

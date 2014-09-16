/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Abstract :  MPEG4 Encoder extende API (used in internal testing)
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: mp4encapi_ext.c,v $
--  $Revision: 1.1 $
--  $Date: 2007/07/16 09:28:57 $
------------------------------------------------------------------------------*/
#include "mp4encapi.h"
#include "mp4encapi_ext.h"
#include "EncInstance.h"

MP4EncRet MP4EncSetHwBurstSize(MP4EncInst inst, u32 burst)
{
    instance_s *pEncInst = (instance_s *) inst;

    ASSERT(inst != NULL);
    ASSERT(burst < 64);

    pEncInst->asic.regs.asicCfgReg &=  ~(63 << 8);
    pEncInst->asic.regs.asicCfgReg |=  ((burst & (63)) << 8);

    return ENC_OK;
}

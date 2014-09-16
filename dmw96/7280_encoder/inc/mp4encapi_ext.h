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
--  Abstract : Hantro MPEG4 Encoder Extended API
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: mp4encapi_ext.h,v $
--  $Revision: 1.2 $
--  $Date: 2008/05/22 06:25:55 $
--
------------------------------------------------------------------------------*/

#ifndef __MP4ENCAPI_EXT_H__
#define __MP4ENCAPI_EXT_H__

#include "basetype.h"
#include "mp4encapi.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MP4EncRet MP4EncSetHwBurstSize(MP4EncInst inst, u32 burst);

#ifdef __cplusplus
}
#endif

#endif /*__MP4ENCAPI_EXT_H__*/

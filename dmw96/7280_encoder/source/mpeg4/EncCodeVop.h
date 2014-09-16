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
--  Abstract :
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: EncCodeVop.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_CODE_VOP_H__
#define __ENC_CODE_VOP_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "EncInstance.h"
#include "EncVideoObjectLayer.h"
#include "EncVideoObjectPlane.h"
#include "EncShortVideoHeader.h"
#include "EncMbHeader.h"
#include "EncRateControl.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

typedef enum
{
    ENCODE_OK = 0,
    ENCODE_TIMEOUT = 1,
    ENCODE_DATA_ERROR = 2,
    ENCODE_HW_ERROR = 3,
    ENCODE_SYSTEM_ERROR = 4,
    ENCODE_HW_RESET
} encodeFrame_e;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

encodeFrame_e EncCodeVop(instance_s * inst, stream_s * stream);

encodeFrame_e EncCodeVop_V2(instance_s * inst, stream_s * stream);

#endif

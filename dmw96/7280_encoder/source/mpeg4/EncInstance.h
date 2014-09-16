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
--  $RCSfile: EncInstance.h,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_INSTANCE_H__
#define __ENC_INSTANCE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncVisualObjectSequence.h"
#include "EncVisualObject.h"
#include "EncVideoObject.h"
#include "EncVideoObjectLayer.h"
#include "EncGroupOfVideoObjectPlane.h"
#include "EncVideoObjectPlane.h"
#include "EncShortVideoHeader.h"
#include "EncRateControl.h"
#include "EncMbHeader.h"
#include "EncUserData.h"
#include "EncCoder.h"
#include "EncTimeCode.h"
#include "EncProfile.h"
#include "encpreprocess.h"
#include "encasiccontroller.h"

#ifdef VIDEOSTAB_ENABLED
#include "vidstabcommon.h"
#endif

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
enum EncStatus
{
    ENCSTAT_INIT = 0xA1,
    ENCSTAT_START_STREAM,
    ENCSTAT_START_VOP,
    ENCSTAT_NEW_REF_IMG,
    ENCSTAT_ERROR
};

typedef struct {
    u32 encStatus;
    u32 frameCnt;
    u32 testId;
    vos_s visualObjectSequence;
    visob_s visualObject;
    vidob_s videoObject;
    vol_s videoObjectLayer;
    govop_s groupOfVideoObjectPlane;
    vop_s videoObjectPlane;
    mb_s macroblock;
    svh_s shortVideoHeader;
    rateControl_s rateControl;
    preProcess_s preProcess;
    timeCode_s timeCode;
    profile_s profile;
    asicData_s asic;
    const void * inst;
#ifdef VIDEOSTAB_ENABLED
    HWStabData vsHwData;
    SwStbData vsSwData;
#endif
} instance_s;

#endif


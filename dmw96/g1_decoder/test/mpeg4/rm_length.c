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
/*------------------------------------------------------------------------------
    Include headers
------------------------------------------------------------------------------*/

#include "mp4decapi.h"
#include "mp4dechwd_container.h"
#include "mp4dechwd_vop.h"

/*------------------------------------------------------------------------------
    2. External identifiers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Module indentifiers
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

   5.1  Function name: MP4GetResyncLength

        Purpose:

        Input:

        Output:

------------------------------------------------------------------------------*/

u32 MP4GetResyncLength(MP4DecInst decInst, u8 *pStrm)
{

    u32 i, tmp;
    i32 itmp;
    u32 moduloTimeBase, vopTimeIncrement;
    DecContainer *pDecContainer = (DecContainer*)decInst;
    DecContainer copyContainer = *pDecContainer;

    copyContainer.StrmDesc.pStrmCurrPos =
    copyContainer.StrmDesc.pStrmBuffStart = pStrm;
    copyContainer.StrmDesc.strmBuffReadBits =
    copyContainer.StrmDesc.bitPosInWord = 0;
    copyContainer.StrmDesc.strmBuffSize = 1024;

    /* check that memories allocated already */
    if(pDecContainer->StrmStorage.data[0].busAddress != 0)
        if (!StrmDec_DecodeVopHeader(&copyContainer))
            if (copyContainer.VopDesc.vopCodingType < 2 ||
                copyContainer.VopDesc.fcodeFwd >
                copyContainer.VopDesc.fcodeBwd)
                return(16+copyContainer.VopDesc.fcodeFwd);
            else
                return(16+copyContainer.VopDesc.fcodeBwd);
        else
            return 17;
    return 0;

}

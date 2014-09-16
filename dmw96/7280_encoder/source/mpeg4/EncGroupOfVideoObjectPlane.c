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
--  $RCSfile: EncGroupOfVideoObjectPlane.c,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncStartCode.h"
#include "EncGroupOfVideoObjectPlane.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

	EncGoVopHdr

------------------------------------------------------------------------------*/
void EncGoVopInit(govop_s * govop)
{
    govop->header = ENCHW_NO;
    govop->timeCodeHours = 0;
    govop->timeCodeMinutes = 0;
    govop->timeCodeSecond = 0;
    govop->closedGov = 0;
    govop->brokenLink = 0;
    govop->userData.header = ENCHW_NO;

    return;
}

/*------------------------------------------------------------------------------

	EncGoVopHdr

	After GoVopHdr header next vop must be IVOP. Rate control must
	reset this. See EncBeforeVopRc().

------------------------------------------------------------------------------*/
void EncGoVopHdr(stream_s * stream, govop_s * govop)
{
    if(govop->header == ENCHW_NO)
    {
        return;
    }

    /* Start Code Prefix And Start Code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    EncPutBits(stream, START_CODE_GOB_VAL, START_CODE_GOB_NUM);
    COMMENT("Group Of Vop Start Code");

    /* Time Code Hours */
    ASSERT(govop->timeCodeHours < 24);
    EncPutBits(stream, govop->timeCodeHours, 5);
    COMMENT("Time Code Hours");

    /* Time Code Minutes */
    ASSERT(govop->timeCodeMinutes < 60);
    EncPutBits(stream, govop->timeCodeMinutes, 6);
    COMMENT("Time Code Minutes");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Time Code Seconds */
    ASSERT(govop->timeCodeSecond < 60);
    EncPutBits(stream, govop->timeCodeSecond, 6);
    COMMENT("Time Code Seconds");

    /* Closed Gov */
    EncPutBits(stream, govop->closedGov, 1);
    COMMENT("Closed Gov");

    /* Broken Link */
    EncPutBits(stream, govop->brokenLink, 1);
    COMMENT("Broken Link ");

    /* Next Start Code */
    EncNextStartCode(stream);
    COMMENT("Next start Code");

    /* User data */
    EncUserData(stream, &govop->userData);

    return;
}

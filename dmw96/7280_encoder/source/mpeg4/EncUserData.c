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
--  $RCSfile: EncUserData.c,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncUserData.h"
#include "EncStartCode.h"
#include "enccommon.h"

#include "ewl.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

REMOVE_HANTRO_USER_DATA

If this is defined, Hantro watermark userdata is not inserted.

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

	EncUserDataAlloc

	Perform memory allocation needed by user data.

	Input	userData_s Pointer to userData_s structure.
		data	Poiter to user data source.
		byteCnt	User data size of source.

	Return	ENCHW_OK	Memory allocation OK.
		    ENCHW_NOK	Memory allocation failed.

------------------------------------------------------------------------------*/
bool_e EncUserDataAlloc(userData_s * userData, const u8 * data, u32 byteCnt)
{
    bool_e status = ENCHW_OK;

    ASSERT(data != NULL);
    ASSERT(byteCnt > 0);

    /* Free unused user data */
    if(userData->header == ENCHW_YES)
    {
        EncUserDataFree(userData);
    }

    if((userData->data = (u8 *) EWLcalloc(1, byteCnt)) == NULL)
    {
        userData->byteCnt = 0;
        userData->header = ENCHW_NO;
        status = ENCHW_NOK;
    }
    else
    {
        if(EWLmemcpy(userData->data, data, byteCnt) != userData->data)
        {
            status = ENCHW_NOK;
            EncUserDataFree(userData);
        }
        else
        {
            userData->byteCnt = byteCnt;
            userData->header = ENCHW_YES;
        }
    }

    return status;
}

/*------------------------------------------------------------------------------

	EncUserDataFree

	Input	userData Pointer to userData_s structure.

------------------------------------------------------------------------------*/
void EncUserDataFree(userData_s * userData)
{
    EWLfree(userData->data);
    userData->data = NULL;
    userData->header = ENCHW_NO;
    userData->byteCnt = 0;

    return;
}

/*------------------------------------------------------------------------------

	EncUserData

------------------------------------------------------------------------------*/
void EncUserData(stream_s * stream, userData_s * userData)
{
    const u8 *data;
    u8 next;
    u32 byteCnt = 0;
    u32 zeroCnt = 0;
    u32 zeroIn = 0;
    u32 i;

#ifndef REMOVE_HANTRO_USER_DATA
    const u8 udata[] = { 0, 9, 0, 0xFF, 0xFF, 9, 0, 2, 2, 0 };

    byteCnt = sizeof(udata);
#endif

    if(userData->header != ENCHW_NO)
    {
        byteCnt += userData->byteCnt;
        data = userData->data;
    }
    else
    {
#ifndef REMOVE_HANTRO_USER_DATA
        data = udata;
#else
        return;
#endif
    }

    /* start code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");
    EncPutBits(stream, START_CODE_USER_DATA_VAL, START_CODE_USER_DATA_NUM);
    COMMENT("User Data start code");

    while(byteCnt > 0)
    {
#ifndef REMOVE_HANTRO_USER_DATA
        if(byteCnt == sizeof(udata))
        {
            data = udata;
        }
#endif

        next = data[0];

        /* Incoming zeros */
        i = 0;
        while(((next >> i) & 0xFF) > 0)
        {
            i++;
        }
        zeroIn = 8 - i;

        /* Consecutive zeros shall not be a string of 23 or more zeros */
        if(zeroCnt + zeroIn > 22)
        {
            EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
            COMMENT("Start Code Prefix");
            EncPutBits(stream, START_CODE_USER_DATA_VAL,
                       START_CODE_USER_DATA_NUM);
            COMMENT("User Data -Break header-");
            zeroCnt = 0;
            zeroIn = 0;
        }
        else
        {
            if(zeroIn == 8)
            {
                zeroCnt += 8;
            }
            else
            {
                i = 0;
                while(((next << i) & 0xFF) > 0)
                {
                    i++;
                }
                zeroCnt = 8 - i;
            }
            EncPutBits(stream, next, 8);
            COMMENT("User Data");
            data++;
            byteCnt--;
        }
    }
    EncUserDataFree(userData);

    return;
}

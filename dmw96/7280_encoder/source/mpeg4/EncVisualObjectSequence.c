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
--  $RCSfile: EncVisualObjectSequence.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncVisualObjectSequence.h"
#include "EncStartCode.h"
#include "EncUserData.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table G-1
0,  8 Reserved
1,  8 Simple Profile/Level 1
2,  8 Simple Profile/Level 2
3,  8 Simple Profile/Level 3
4,  8 Reserved
8,  8 Simple Profile/Level 0
17, 8 Simple Scalable Profile/Level 1
18, 8 Simple Scalable Profile/Level 2
19, 8 Reserved
33, 8 Core Profile/Level 1
34, 8 Core Profile/Level 2
35, 8 Reserved
50, 8 Main Profile/Level 2
51, 8 Main Profile/Level 3
52, 8 Main Profile/Level 3
53, 8 Reserved
66, 8 N-bit Profile/Level 2
67, 8 Reserved
81, 8 Scalable Texture Profile/Level 1
82, 8 Reserved
97, 8 Simple Face Animation Profile/Level 1
98, 8 Simple Face Animation Profile/Level 2
99, 8 Reserved
113,8 Basic Animated Texture Profile/Level 1
114,8 Basic Animated Texture Profile/Level 2
115,8 Reserved
129,8 Hybrid Profile/Level 1
130,8 Hybrid Profile/Level 2
131,8 Reserved */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

	EncVosInit

------------------------------------------------------------------------------*/
void EncVosInit(vos_s * vos)
{
    vos->header = ENCHW_YES;
    vos->profile = 1;
    vos->userData.header = ENCHW_NO;

    return;
}

/*------------------------------------------------------------------------------

	EncVosHrd

------------------------------------------------------------------------------*/
void EncVosHrd(stream_s * stream, vos_s * vos)
{
    if(vos->header == ENCHW_NO)
    {
        return;
    }

    /* Byte buffer must be clean */
    ASSERT((stream->stream[0] == 0) && (stream->stream[1] == 0));

    /* Start Code Prefix And Start Code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");
    EncPutBits(stream, START_CODE_VOS_VAL, START_CODE_VOS_NUM);
    COMMENT("Visual Object Sequence start code");

    /* profile and level indication */
    EncPutBits(stream, vos->profile, 8);
    COMMENT("Profile and Level");

    /* User data */
    EncUserData(stream, &vos->userData);

    return;
}

/*------------------------------------------------------------------------------

	EncVosEndHrd

------------------------------------------------------------------------------*/
void EncVosEndHrd(stream_s * stream, vos_s * vos)
{
    if(vos->header == ENCHW_NO)
    {
        return;
    }

    /* Byte buffer must be clean */
    ASSERT((stream->stream[0] == 0) && (stream->stream[1] == 0));

    /* start code prefix */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");

    /* Start code */
    EncPutBits(stream, START_CODE_VOS_END_VAL, START_CODE_VOS_END_NUM);
    COMMENT("Visual Object Sequence end code");

    return;
}

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
--  $RCSfile: EncVisualObject.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncVisualObject.h"
#include "EncStartCode.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table 6-4
0,4 reserved
1,4 ISO/IEC 14496-2
2,4 ISO/IEC 14496-2 AMD1
3,4 reserved

Table 6-5, see visualObjectType_e
0,4 reserved
1,4 video ID
2,4 still texture ID
3,4 Me.h ID
4,4 FBA ID
5,4 3D Me.h ID

Table 6-6
0,3 Component
1,3 PAL
2,3 NTSC
3,3 SECAM
4,3 MAC
5,3 Unspecified video format

Table 6-7
0,8 Forbidden
1,8 Recommendation ITU-R BT.709
2,8 Unspecified Video
3,8 Reserved
4,8 Recommendation ITU-R BT.470-2 System M
5,8 Recommendation ITU-R BT.470-2 System B, G
6,8 SMPTE 170M
7,8 SMPTE 240M (1987)
8,8 Generic film

Table 6-8
0, 8 Forbidden
1, 8 Recommendation ITU-R BT.709
2, 8 Unspecified Video
3, 8 Reserved
4, 8 Recommendation ITU-R BT.470-2 System M
5, 8 Recommendation ITU-R BT.470-2 System B, G
6, 8 SMPTE 170M
7, 8 SMPTE 240M (1987)
8, 8 Linear transfer characteristic
9, 8 Logarithmic transfer char. (100:1 range)
10,8 Logarithmic transfer char. (316.22777:1 range)

Table 6-9
0,8 Forbidden
1,8 Recommendation ITU-R BT.709
2,8 Unspecified Video
3,8 Reserved
4,8 FCC
5,8 Recommendation ITU-R BT.470-2 System B, G
6,8 SMPTE 170M
7,8 SMPTE 240M (1987) */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void VideoSignalType(stream_s *, videoSignalType_s *);

/*------------------------------------------------------------------------------

	EncVisObInit

------------------------------------------------------------------------------*/
void EncVisObInit(visob_s * visob)
{
    videoSignalType_s *vst = &visob->videoSignalType;

    visob->header = ENCHW_YES;
    visob->isVisualObjectIdentifier = ENCHW_NO;
    visob->visualObjectVerid = 1;
    visob->visualObjectPriority = 1;
    visob->visualObjectType = VIDEO_ID;
    visob->userData.header = ENCHW_NO;

    vst->videoSignalType = ENCHW_NO;
    vst->videoFormat = 5;   /* '101' unspecified format */
    vst->videoRange = 0;
    vst->colourDescription = ENCHW_NO;
    vst->colourPrimaries = 1;
    vst->transferCharacteristics = 1;
    vst->matrixCoefficients = 1;

    return;
}

/*------------------------------------------------------------------------------

	EncVisObHdr

------------------------------------------------------------------------------*/
void EncVisObHdr(stream_s * stream, visob_s * visob)
{
    if(visob->header == ENCHW_NO)
    {
        return;
    }

    /* Start Code Prefix And Start Code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");
    EncPutBits(stream, START_CODE_VO_VAL, START_CODE_VO_NUM);
    COMMENT("Visual Object start code");

    /* Visual object identifier */
    if(visob->isVisualObjectIdentifier == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Is Visual Object Identifier");

        /* Visual Object Verid */
        EncPutBits(stream, visob->visualObjectVerid, 4);
        COMMENT("Visual Object Verid");

        /* Visual Object Priority */
        EncPutBits(stream, visob->visualObjectPriority, 3);
        COMMENT("Visual Object Priority");
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Is Visual Object Identifier");
    }

    /* Visual Object Type */
    EncPutBits(stream, visob->visualObjectType, 4);
    COMMENT("Visual Object Type");

    /* visualObjectType == "video ID" ||      */
    /* visualObjectType == "still texture ID" */
    if((visob->visualObjectType == VIDEO_ID) || (visob->visualObjectType
                                                 == STILL_TEXTURE_ID))
    {
        VideoSignalType(stream, &visob->videoSignalType);
    }

    /* Next Start Code */
    EncNextStartCode(stream);

    /* User data */
    EncUserData(stream, &visob->userData);

    return;
}

/*------------------------------------------------------------------------------

	VideoSignalType

------------------------------------------------------------------------------*/
void VideoSignalType(stream_s * stream, videoSignalType_s * vst)
{
    if(vst->videoSignalType == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Video Signal Type");
        EncPutBits(stream, vst->videoFormat, 3);
        COMMENT("Video Format");
        EncPutBits(stream, vst->videoRange, 1);
        COMMENT("Video Range");
        if(vst->colourDescription == ENCHW_YES)
        {
            EncPutBits(stream, 1, 1);
            COMMENT("Colour Description");
            EncPutBits(stream, vst->colourPrimaries, 8);
            COMMENT("Colour Primaries");
            EncPutBits(stream, vst->transferCharacteristics, 8);
            COMMENT("Transfer Characteristics");
            EncPutBits(stream, vst->matrixCoefficients, 8);
            COMMENT("Matrix Coefficients");
        }
        else
        {
            EncPutBits(stream, 0, 1);
            COMMENT("Colour Description");
        }
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Video Signal Type");
    }

    return;
}

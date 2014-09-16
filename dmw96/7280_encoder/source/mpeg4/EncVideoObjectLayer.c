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
--  $RCSfile: EncVideoObjectLayer.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncVideoObjectLayer.h"
#include "EncStartCode.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table 6-10
0,8 Reserved
1,8 Simple Object Type
2,8 Simple Scalable Object Type
3,8 Core Object Type
4,8 Main Object Type
5,8 N-bit Object Type
6,8 Basic Anim. 2D Texture
7,8 Anim. 2D Mesh
8,8 Simple Face
9,8 Still Scalable Texture

Table 6-11
0,4 reserved
1,4 ISO/IEC 14496-2
2,4 ISO/IEC 14496-2 AMD1

Table 6-12
0, 4 Forbidden
1, 4 1:1 (Square)
2, 4 12:11 (625-type for 4:3 picture)
3, 4 10:11 (525-type for 4:3 picture)
4, 4 16:11 (625-type stretched for 16:9 picture)
5, 4 40:33 (525-type stretched for 16:9 picture)
6, 4 Reserved
15,4 extended PAR

Table 6-13
0,2 Reserved
1,2 4:2:0
2,2 Reserved
3,2 Reserved

Table 6-14
0, 2 rectangular
1, 2 binary
2, 2 binary only
3, 2 grayscale */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void VolControlParameter(stream_s *, volControlParameter_s *);
static void FixedVopTimeIncrement(stream_s *, i32, i32);
static void NotBinary(stream_s *, vol_s * vol);

/*------------------------------------------------------------------------------

	EncVolInit

------------------------------------------------------------------------------*/
void EncVolInit(vol_s * vol)
{
    volControlParameter_s *volCtrPrm = &vol->volControlParameter;

    vol->header = ENCHW_YES;
    vol->isObjectLayerIdentifier = ENCHW_NO;
    vol->isVolControlParameter = ENCHW_NO;
    vol->fixedVopRate = ENCHW_NO;
    vol->complexityEstimationDisable = ENCHW_YES;
    vol->resyncMarkerDisable = ENCHW_YES;
    vol->dataPart = ENCHW_NO;
    vol->rvlc = ENCHW_NO;
    vol->randomAccessibleVol = 0;
    vol->videoObjectTypeIndication = 1;
    vol->videoObjectLayerVerid = 1;
    vol->videoObjectLayerPriority = 1;
    vol->aspectRatioInfo = 1;
    vol->parWidth = 1;
    vol->parHeight = 1;
    vol->vopTimeIncRes = 0;
    vol->fixedVopTimeInc = 0;
    vol->videoObjectLayerWidth = 0;
    vol->videoObjectLayerHeight = 0;
    vol->userData.header = ENCHW_NO;

    volCtrPrm->vbvParameter = ENCHW_NO;
    volCtrPrm->chromaFormat = 1;
    volCtrPrm->lowDelay = 1;
    volCtrPrm->firstHalfBitRate = 0;
    volCtrPrm->latterHalfBitRate = 0;
    volCtrPrm->firstHalfVbvBufferSize = 0;
    volCtrPrm->latterHalfVbvBufferSize = 0;
    volCtrPrm->firstHalfVbvOccupancy = 0;
    volCtrPrm->latterHalfVbvOccupancy = 0;

    return;
}

/*------------------------------------------------------------------------------

	EncVolHdr

------------------------------------------------------------------------------*/
void EncVolHdr(stream_s * stream, vol_s * vol)
{
    if(vol->header == ENCHW_NO)
    {
        return;
    }

    /* Start Code Prefix And Start Code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");
    EncPutBits(stream, START_CODE_VOL_VAL, START_CODE_VOL_NUM);
    COMMENT("Video Object Layer Start Code");

    /* Random Accessible Vol */
    EncPutBits(stream, vol->randomAccessibleVol, 1);
    COMMENT("Random Accessible Vol");

    /* Video Object Type Indication */
    EncPutBits(stream, vol->videoObjectTypeIndication, 8);
    COMMENT("Video Object Type Indication");

    /* Object Layer Identifier */
    if(vol->isObjectLayerIdentifier == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Is Object Layer Identifier");

        /* Video Object Layer Verid */
        EncPutBits(stream, vol->videoObjectLayerVerid, 4);
        COMMENT("Video Object Layer Verid");

        /* Video Object Layer Priority */
        EncPutBits(stream, vol->videoObjectLayerPriority, 3);
        COMMENT("Video Object Layer Priority");
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Is Object Layer Identifier");
    }

    /* Aspect Ratio Info */
    EncPutBits(stream, vol->aspectRatioInfo, 4);
    COMMENT("Aspect Ratio Info");

    /* Aspect Ratio Info == "extended_PAR" */
    if(vol->aspectRatioInfo == 15)
    {
        /* Par Width */
        EncPutBits(stream, vol->parWidth, 8);
        COMMENT("Par Width");

        /* Par Height */
        EncPutBits(stream, vol->parHeight, 8);
        COMMENT("Par Height");
    }

    /* Vol Control Parameters */
    if(vol->isVolControlParameter == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Vol Control Parameters");
        VolControlParameter(stream, &vol->volControlParameter);
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Vol Control Parameters");
    }

    /* Video Object Layer Shape */
    EncPutBits(stream, RECTANGULAR, 2);
    COMMENT("Video Object Layer Shape");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Vop Time Increment Resolution */
    EncPutBits(stream, vol->vopTimeIncRes, 16);
    COMMENT("Vop Time Increment Resolution");
    /* Debug: vopTimeIncRes > 0 */
    ASSERT(vol->vopTimeIncRes > 0);

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Fixed Vop Rate */
    if(vol->fixedVopRate == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Fixed Vop Rate");

        /* Fixed Vop Time Increment */
        FixedVopTimeIncrement(stream, vol->vopTimeIncRes, vol->fixedVopTimeInc);
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Fixed Vop Rate");
    }

    /* Video Object Layer Shape == Binary Only not implemented */
    NotBinary(stream, vol);

    /* Next Start Code */
    EncNextStartCode(stream);

    /* User data */
    EncUserData(stream, &vol->userData);

    return;
}

/*------------------------------------------------------------------------------

   5.2  Function name: VolControlParameter

------------------------------------------------------------------------------*/
void VolControlParameter(stream_s * stream, volControlParameter_s * vcp)
{
    /* Chroma Format */
    EncPutBits(stream, vcp->chromaFormat, 2);
    COMMENT("Chroma Format");

    /* Low Delay */
    EncPutBits(stream, vcp->lowDelay, 1);
    COMMENT("Low Delay");

    /* vbv Parameters */
    if(vcp->vbvParameter == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("vbv Parameters");

        /* Firs Half Bit Rate */
        EncPutBits(stream, vcp->firstHalfBitRate, 15);
        COMMENT("Firs Half Bit Rate");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");

        /* Latter Half Bit Rate */
        EncPutBits(stream, vcp->latterHalfBitRate, 15);
        COMMENT("Latter Half Bit Rate");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");

        /* First Half vbv Buffer Size */
        EncPutBits(stream, vcp->firstHalfVbvBufferSize, 15);
        COMMENT("First Half vbv Buffer Size");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");

        /* Latter Half vbv Buffer Size */
        EncPutBits(stream, vcp->latterHalfVbvBufferSize, 3);
        COMMENT("Latter Half vbv Buffer Size");

        /* First Half vbv Occupancy */
        EncPutBits(stream, vcp->firstHalfVbvOccupancy, 11);
        COMMENT("First Half vbv Occupancy");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");

        /* Latter Half vbv Occupancy */
        EncPutBits(stream, vcp->latterHalfVbvOccupancy, 15);
        COMMENT("Latter Half vbv Occupancy");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("vbv Parameters");
    }

    return;
}

/*------------------------------------------------------------------------------

   5.3  Function name: FixedVopTimeIncrement

------------------------------------------------------------------------------*/
void FixedVopTimeIncrement(stream_s * stream, i32 vopTimeIncRes,
                           i32 fixedVopTimeInc)
{
    i32 bits = 0;

    /* Calculate bits */
    ASSERT(fixedVopTimeInc < vopTimeIncRes);
    while((1 << ++bits) < vopTimeIncRes) ;

    EncPutBits(stream, fixedVopTimeInc, bits);
    COMMENT("Fixed Vop Time Increment");

    return;
}

/*------------------------------------------------------------------------------

   5.4  Function name: NotBinary

------------------------------------------------------------------------------*/
void NotBinary(stream_s * stream, vol_s * vol)
{
    /* "Video Object Layer Shape" == "Rectangular" (Table 6-14) */
    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Video Object Layer Width */
    EncPutBits(stream, vol->videoObjectLayerWidth, 13);
    COMMENT("Video Object Layer Width");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Video Object Layer Height */
    EncPutBits(stream, vol->videoObjectLayerHeight, 13);
    COMMENT("Video Object Layer Height");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Interlaced */
    EncPutBits(stream, 0, 1);
    COMMENT("Interlaced");

    /* Obmc Disable */
    EncPutBits(stream, 1, 1);
    COMMENT("Obmc Disable");

    /* Sprite Enable not implemented */
    EncPutBits(stream, 0, 1);
    COMMENT("Sprite Enable");

    /* Not Eight Bit */
    EncPutBits(stream, 0, 1);
    COMMENT("Not Eight Bit");

    /* quantType == 1 not implemented */
    EncPutBits(stream, 0, 1);
    COMMENT("Quant Type");

    /* Complexity Estimation not implemented */
    ASSERT(vol->complexityEstimationDisable == ENCHW_YES);
    EncPutBits(stream, 1, 1);
    COMMENT("Complexity Estimation Disable");

    /* Resync Marker Disable */
    if(vol->resyncMarkerDisable == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Resync Marker Disable");
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Resync Marker Disable");
    }

    /* Data Partitioned */
    if(vol->dataPart == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Data Partitioned");

        if(vol->rvlc == ENCHW_YES)
        {
            EncPutBits(stream, 1, 1);
            COMMENT("Reversible Vlc");
        }
        else
        {
            EncPutBits(stream, 0, 1);
            COMMENT("Reversible Vlc");
        }
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Data Partitioned");
    }

    /* Scalability */
    EncPutBits(stream, 0, 1);
    COMMENT("Scalability");

    return;
}

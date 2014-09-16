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
--  $RCSfile: EncVideoObjectPlane.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncVideoObjectPlane.h"
#include "EncStartCode.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/* Table 6-20, see vopType_e
0,2 intra-coded (I)
1,2 predictive-coded (P)
2,2 bidirectionally-predictive-coded (B)
3,2 sprite (S)

Table 6-21
0,3 Use Intra DC VLC for entire VOP
1,3 Switch to Intra AC VLC at running Qp >= 13
2,3 Switch to Intra AC VLC at running Qp >= 15
3,3 Switch to Intra AC VLC at running Qp >= 17
4,3 Switch to Intra AC VLC at running Qp >= 19
5,3 Switch to Intra AC VLC at running Qp >= 21
6,3 Switch to Intra AC VLC at running Qp >= 23
7,3 Use Intra AC VLC for entire VOP

Table 7-5: Range for motion vectors
0,3 Forbidden
1,3 [-32,31]
2,3 [-64,63]
3,3 [-128,127]
4,3 [-256,255]
5,3 [-512,511]
6,3 [-1024,1023]
7,3 [-2048,2047] */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static void TimeCode(stream_s *, i32, i32, i32);
static void HecHdr(stream_s *, const vop_s *);

/*------------------------------------------------------------------------------

	EncVopHdr

------------------------------------------------------------------------------*/
void EncVopInit(vop_s * vop)
{
    vop->header = ENCHW_YES;
    vop->vopCoded = ENCHW_YES;
    vop->hec = ENCHW_NO;
    vop->hecVp = ENCHW_NO;
    vop->acPred = ENCHW_NO;
    vop->vopType = IVOP;
    vop->vopTimeInc = 0;
    vop->roundControl = 0;
    vop->intraDcVlcThr = 0;
    vop->vopFcodeForward = 1;
    vop->moduloTimeBase = 0;

    return;
}

/*------------------------------------------------------------------------------

	EncVopHdr

------------------------------------------------------------------------------*/
void EncVopHdr(stream_s * stream, const vop_s * vop, i32 qp)
{
    /* Start Code Prefix And Start Code */
    EncPutBits(stream, START_CODE_PREFIX_VAL, START_CODE_PREFIX_NUM);
    COMMENT("Start Code Prefix");
    EncPutBits(stream, START_CODE_VOP_VAL, START_CODE_VOP_NUM);
    COMMENT("Video Object Plane Start Code");

    /* Vop Coding Type */
    EncPutBits(stream, vop->vopType, 2);
    COMMENT("Vop Coding Type");

    /* Modulo Time Base */
    TimeCode(stream, vop->moduloTimeBase, vop->vopTimeIncRes, vop->vopTimeInc);

    /* Vop Coded */
    if(vop->vopCoded == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Vop Coded");
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Vop Coded");

        /* Next Start Code */
        EncNextStartCode(stream);
        COMMENT("Next start Code");

        return;
    }

    if(vop->vopType == PVOP)
    {
        /* Vop Rounding Type */
        EncPutBits(stream, vop->roundControl, 1);
        COMMENT("Vop Rounding Type");
    }

    /* Intra Dc Vlc Thr */
    EncPutBits(stream, vop->intraDcVlcThr, 3);
    COMMENT("Intra Dc Vlc Thr");

    /* Vop Quant */
    EncPutBits(stream, qp, 5);
    COMMENT("Vop Quant");

    if(vop->vopType != IVOP)
    {
        EncPutBits(stream, vop->vopFcodeForward, 3);
        COMMENT("Vop Fcode Forward");
    }

    return;
}

/*------------------------------------------------------------------------------

	TimeCode

------------------------------------------------------------------------------*/
void TimeCode(stream_s * stream, i32 seconds, i32 vopTimeIncRes, i32 vopTimeInc)
{
    i32 bits = 0;

    /* Modulo Time Base */
    while(seconds > 0)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Modulo Time Base Seconds");
        seconds--;
    }
    EncPutBits(stream, 0, 1);
    COMMENT("Modulo Time Base");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* The number of bits representing the value of vopTimeIcrement is the
     * minimun number of bits required to representing range of [0
     * vopTimeIncRes) */
    ASSERT(vopTimeInc >= 0);
    ASSERT(vopTimeInc < vopTimeIncRes);
    while((1 << ++bits) < vopTimeIncRes) ;

    /* Vop Time Increment */
    EncPutBits(stream, vopTimeInc, bits);
    COMMENT("Vop Time Increment");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    return;
}

/*------------------------------------------------------------------------------

	EncVpHdr

------------------------------------------------------------------------------*/
void EncVpHdr(stream_s * stream, vop_s * vop, u32 mbNum, u32 mbNumBits, u32 qp)
{
    ASSERT((vop->vopType == IVOP) || (vop->vopType == PVOP));
    /* Resync Marker */
    if(vop->vopType == IVOP)
    {
        EncPutBits(stream, 1, 17);
        COMMENT("Resync Marker IVOP");
    }
    else
    {
        EncPutBits(stream, 1, 16 + vop->vopFcodeForward);
        COMMENT("Resync Marker PVOP");
    }

    EncPutBits(stream, mbNum, mbNumBits);
    COMMENT("Macroblock Number");

    /* Quant Scale */
    EncPutBits(stream, qp, 5);
    COMMENT("Quant Scale");

    /* Header Extension Code */
    if(vop->hecVp == ENCHW_YES)
    {
        EncPutBits(stream, 1, 1);
        COMMENT("Header Extension Code");
        HecHdr(stream, vop);
        vop->hecVp = ENCHW_NO;  /* User must re-enable Hec */
    }
    else
    {
        EncPutBits(stream, 0, 1);
        COMMENT("Header Extension Code");
    }

    return;
}

/*------------------------------------------------------------------------------

	HecHdr

------------------------------------------------------------------------------*/
void HecHdr(stream_s * stream, const vop_s * vop)
{
    /* Modulo Time Base */
    TimeCode(stream, vop->moduloTimeBase, vop->vopTimeIncRes, vop->vopTimeInc);

    /* Vop Coding Type */
    EncPutBits(stream, vop->vopType, 2);
    COMMENT("Vop Coding Type");

    /* Intra Dc Vlc Thr */
    EncPutBits(stream, vop->intraDcVlcThr, 3);
    COMMENT("Intra Dc Vlc Thr");

    /* Vop Fcode Forward */
    if(vop->vopType != IVOP)
    {
        EncPutBits(stream, vop->vopFcodeForward, 3);
        COMMENT("Vop Fcode Forward");
    }

    return;
}

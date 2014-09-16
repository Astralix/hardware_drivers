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
--  $RCSfile: EncCoder.c,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "EncCoder.h"
#include "EncPrediction.h"
#include "EncVlc.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static true_e IntraDcDecision(i32 , i32,  i32);
static true_e IntraDc(i32, i32);
static void ZeroBlock(i32 *, const i32 *, const i32 *, true_e);
static i32 NotCodedMb(mv_s *, i32 *);

/*------------------------------------------------------------------------------

     EncIntra

     Encodes one INTRA macroblock using plain MPEG-4 stream syntax

------------------------------------------------------------------------------*/
void EncIntra(stream_s *stream, mb_s *mb)
{
    true_e dcSeparately;

    /* Dc prediction */
    EncPrediction(mb);

    /* Separately coded DC-coeff and zero block decision */
    dcSeparately = IntraDcDecision(mb->qpLastCoded, mb->qp, mb->intraDcVlcThr);

    ZeroBlock(mb->zeroBlock, mb->rlcCount, mb->dc, dcSeparately);

    EncMbHeader(stream, mb);

    if (dcSeparately == ENCHW_YES) {
        EncVlcIntra(stream, mb, DC_VLC_INTRA_DC);
    } else {
        EncVlcIntra(stream, mb, DC_VLC_INTRA_AC);
    }

    return;
}

/*------------------------------------------------------------------------------

    EncInter

    Encodes one INTER macroblock using plain MPEG-4 stream syntax

------------------------------------------------------------------------------*/
void EncInter(stream_s *stream, mb_s *mb)
{

    /* Zero block check, inter DC is coded together with AC-components */
    ZeroBlock(mb->zeroBlock, mb->rlcCount, NULL, ENCHW_NO);

    mb->notCoded = NotCodedMb(&mb->mv[mb->mbNum], mb->zeroBlock);

    EncMbHeader(stream, mb);

    EncVlcInter(stream, mb);

    return;
}

/*------------------------------------------------------------------------------

    EncIntraDp

    Encodes one INTRA macroblock using data partitioned stream syntax

------------------------------------------------------------------------------*/
void EncIntraDp(stream_s *stream, stream_s *stream1, stream_s *stream2,
        mb_s *mb)
{
    true_e dcSeparately;
    dcVlc_e dcCodingType;


    /* Dc prediction */
    EncPrediction(mb);

    /* Separately coded DC-coeff and zero block decision */
    dcSeparately = IntraDcDecision(mb->qpLastCoded, mb->qp, mb->intraDcVlcThr);

    ZeroBlock(mb->zeroBlock, mb->rlcCount, mb->dc, dcSeparately);

    /* Macroblock header is different for IVOP intra mb and PVOP intra mb */
    if (mb->vopType == IVOP) {
        EncMbHeaderIntraDp(stream, stream1, mb, dcSeparately);
    } else {
        EncMbHeaderInterDp(stream, stream1, mb, dcSeparately);
    }

    /* Separately coded Intra DC-coeffs are coded in MB header */
    if (dcSeparately == ENCHW_YES) 
        dcCodingType = DC_VLC_NO;
    else
        dcCodingType = DC_VLC_INTRA_AC;
    
    if (mb->rvlc == ENCHW_YES) {
        EncRvlcIntra(stream2, mb, dcCodingType);
    } else {
        EncVlcIntra(stream2, mb, dcCodingType);
    }

    return;
}

/*------------------------------------------------------------------------------

    EncInterDp

    Encodes one INTER macroblock using data partitioned stream syntax

------------------------------------------------------------------------------*/
void EncInterDp(stream_s *stream, stream_s *stream1, stream_s *stream2, 
        mb_s *mb)
{

    /* Zero block check, inter DC is coded together with AC-components */
    ZeroBlock(mb->zeroBlock, mb->rlcCount, NULL, ENCHW_NO);
    
    mb->notCoded = NotCodedMb(&mb->mv[mb->mbNum], mb->zeroBlock);
    
    EncMbHeaderInterDp(stream, stream1, mb, ENCHW_NO);

    if (mb->rvlc == ENCHW_YES) {
        EncRvlcInter(stream2, mb);
    } else {
        EncVlcInter(stream2, mb);
    }

    return;
}

/*------------------------------------------------------------------------------

    IntraDcDecision

    Running Qp is defined as the DCT quantization parameter for
    luminance and chrominance used for immediately previous coded
    macroblock, except for the first coded macroblock in a VOP
    or a video packet. At the first coded macroblock in a VOP
    or a video packet, the running Qp is defined at the quantization
    parameter value for the current macroblock

------------------------------------------------------------------------------*/
true_e IntraDcDecision(i32 qpLastCoded, i32 qp, i32 intraDcVlcThr)
{
    if (qpLastCoded == 0) {
        return IntraDc(qp, intraDcVlcThr);
    } else {
        return IntraDc(qpLastCoded, intraDcVlcThr);
    }
}

/*------------------------------------------------------------------------------

    IntraDc

    Function checks Table 6-21 and according to value of
    intraDcVlcThr and runningQp determines do we have to
    code Dc coeff separately.

------------------------------------------------------------------------------*/
true_e IntraDc(i32 runningQp, i32 intraDcVlcThr)
{
    if (intraDcVlcThr == 0) {
        return ENCHW_YES;
    }

    if (intraDcVlcThr == 7) {
        return ENCHW_NO;
    }

    if ((intraDcVlcThr == 1) && (runningQp >= 13)) {
        return ENCHW_NO;
    } else if ((intraDcVlcThr == 2) && (runningQp >= 15)) {
        return ENCHW_NO;
    } else if ((intraDcVlcThr == 3) && (runningQp >= 17)) {
        return ENCHW_NO;
    } else if ((intraDcVlcThr == 4) && (runningQp >= 19)) {
        return ENCHW_NO;
    } else if ((intraDcVlcThr == 5) && (runningQp >= 21)) {
        return ENCHW_NO;
    } else if ((intraDcVlcThr == 6) && (runningQp >= 23)) {
        return ENCHW_NO;
    } else {
        return ENCHW_YES;
    }
}

/*------------------------------------------------------------------------------

    ZeroBlock

    Checks each block in macroblock data and fills zeroBlock and last
    arrays with corresponding values.

    Input:   
        zeroBlock       Pointer zero block table that will be filled. 
                        0   the block is zero block
                        1   the block is not zero
        rlcCount        Pointer to RLC count values for each block
        dc              Pointer to DC values for each block, 
                        NULL when DC is in RLC
        dcSeparately    DC coded separately from AC-coefficients

------------------------------------------------------------------------------*/
void ZeroBlock(i32 *zeroBlock, const i32 *rlcCount, const i32 *dc, 
        true_e dcSeparately)
{
    i32 i;

    for (i = 0; i < 6; i++) {

        /* Assume that this block is zero */
        zeroBlock[i] = 0;

        /* RLC coded coeffs, for intra this is AC and for inter this is all */
        if (rlcCount[i] != 0)
            zeroBlock[i] = 1;

        /* Dc-coeff affects when not separately coded */
        if ((dc != NULL) && (dcSeparately == ENCHW_NO) && (dc[i] != 0))
            zeroBlock[i] = 1;
        
    }

    return;
}


/*------------------------------------------------------------------------------

    NotCodedMb

    Checks if current macroblock is not coded.

    Input:
        mv          Pointer to motion vector structure
        zeroBlock   Pointer to zero block table

    Return:  
        0           Macroblock is coded
        1           Macroblock is totally zero = not coded

------------------------------------------------------------------------------*/
i32 NotCodedMb(mv_s *mv, i32 *zeroBlock)
{
    i32 i;

    /* Zeroblock */
    for (i = 0; i < 6; i++) {
        if (zeroBlock[i] != 0) {
            return 0;
        }
    }

    /* Motion vector */
    for (i = 0; i < 4; i++) {
        if ((mv->x[i] != 0) || (mv->y[i] != 0)) {
            return 0;
        }
    }

    return 1;
}



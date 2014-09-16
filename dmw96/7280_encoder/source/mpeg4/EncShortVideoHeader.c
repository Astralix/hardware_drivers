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
--  $RCSfile: EncShortVideoHeader.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "enccommon.h"
#include "EncShortVideoHeader.h"
#include "EncStartCode.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static u32 GobFrameId(svh_s *);
static void PlusHeader(stream_s *, svh_s *);

/*------------------------------------------------------------------------------

	EncSvhInit

------------------------------------------------------------------------------*/
void EncSvhInit(svh_s * svh)
{
    svh->header = ENCHW_NO;
    svh->vopType = IVOP;
    svh->tempRef = 0;
    svh->splitScreenIndicator = 0;
    svh->documentCameraIndicator = 0;
    svh->fullPictureFreezeRelease = 0;
    svh->sourceFormat = 0;
    svh->gobPlace = 0;
    svh->mbInGob = 0;
    svh->gobNum = 0;
    svh->gobFrameId = 0;
    svh->gobFrameIdBit = 0;
    svh->timeCode = 0;

    svh->plusHeader = ENCHW_NO;
    svh->roundControl = 0;
    svh->videoObjectLayerWidth = 0;
    svh->videoObjectLayerHeight = 0;
    svh->pwi = 0;
    svh->phi = 0;
    svh->ufepTempRef = 0;
    svh->ufepTempRefPrev = 0;
    svh->ufepCnt = 0;

    return;
}

/*------------------------------------------------------------------------------

	EncSvhHdr

------------------------------------------------------------------------------*/
void EncSvhHdr(stream_s * stream, svh_s * svh, i32 qp)
{
    /* Picture size is wrong */
    ASSERT(svh->sourceFormat != 0);

    /* Short Video Start Marker */
    EncPutBits(stream, START_CODE_SVH_S_VAL, START_CODE_SVH_S_NUM);
    COMMENT("Short Video Start Marker");

    /* Temporal Reference */
    EncPutBits(stream, svh->tempRef, 8);
    COMMENT("Temporal Reference");

    /* Marker Bit */
    EncPutBits(stream, 1, 1);
    COMMENT("Marker Bit");

    /* Zero Bit */
    EncPutBits(stream, 0, 1);
    COMMENT("Zero Bit");

    /* Split Screen Indicator */
    EncPutBits(stream, svh->splitScreenIndicator, 1);
    COMMENT("Split Screen Indicator");

    /* Document Camera Indicator */
    EncPutBits(stream, svh->documentCameraIndicator, 1);
    COMMENT("Document Camera Indicator");

    /* Full Picture Freeze Release */
    EncPutBits(stream, svh->fullPictureFreezeRelease, 1);
    COMMENT("Full Picture Freeze Release");

    if(svh->plusHeader == ENCHW_NO)
    {
        /* Source Format */
        EncPutBits(stream, svh->sourceFormat, 3);
        COMMENT("Source Format");

        /* Picture Coding Type */
        ASSERT((svh->vopType == IVOP) || (svh->vopType == PVOP));
        EncPutBits(stream, svh->vopType, 1);
        COMMENT("Picture Coding Type");

        /* Four Reserved Zero Bits */
        EncPutBits(stream, 0, 4);
        COMMENT("Four Reserved Zero Bits");

        /* Vop Quant */
        EncPutBits(stream, qp, 5);
        COMMENT("Vop Quant (Svh)");

        /* CPM */
        EncPutBits(stream, 0, 1);
        COMMENT("CPM");
    }
    else
    {
        /* Source Format */
        EncPutBits(stream, 7, 3);
        COMMENT("Source Format: extended PTYPE");
        PlusHeader(stream, svh);

        /* Vop Quant */
        EncPutBits(stream, qp, 5);
        COMMENT("Vop Quant (Svh)");
    }

    /* Pei, always zero */
    EncPutBits(stream, 0, 1);
    COMMENT("Pei, always zero");

    return;
}

/*------------------------------------------------------------------------------

	PlusHeader

------------------------------------------------------------------------------*/
void PlusHeader(stream_s * stream, svh_s * svh)
{
    true_e oppType = ENCHW_NO;
    i32 tempRef = 0;

    /* OPPTYPE decision */
    if(svh->vopType == IVOP)
    {
        oppType = ENCHW_YES;
    }

    if(svh->tempRef <= svh->ufepTempRefPrev)
    {
        svh->ufepTempRef -= 256;
    }
    svh->ufepTempRefPrev = svh->tempRef;
    svh->ufepCnt++;

    tempRef = svh->tempRef - svh->ufepTempRef;
    if((svh->ufepCnt > 4) && (tempRef > 149))
    {
        oppType = ENCHW_YES;
    }

    /* UFEP and OPPTYPE */
    if(oppType == ENCHW_NO)
    {
        EncPutBits(stream, 0, 3);
        COMMENT("UFEP");
    }
    else
    {
        EncPutBits(stream, 1, 3);
        COMMENT("UFEP");

        /* Source Format */
        EncPutBits(stream, svh->sourceFormat, 3);
        COMMENT("Source Format (OPPTYPE)");

        /* Optional Custom PCF = 0 */
        /* Optional Unrestricted Motion Vector = 0 */
        /* Optional Syntax-based Arithmetic Coding = 0 */
        /* Optional Advanced Prediction = 0 */
        /* Optional Advanced Intra Coding = 0 */
        /* Optional Deblocking Filter = 0 */
        /* Optional Slice Structured mode = 0 */
        /* Optional Reference Picture Selection = 0 */
        /* Optional Independent Segment Decoding = 0 */
        /* Optional Alternative INTER VLC mode = 0 */
        /* Optional Modified Quantization mode = 0 */
        /* 1000 */
        EncPutBits(stream, 8, 15);
        COMMENT("The rest of OPPTYPE");
        svh->ufepTempRef = svh->tempRef;
        svh->ufepCnt = 0;
    }

    /* MPPTYPE */
    /* Picture Coding Type */
    ASSERT((svh->vopType == IVOP) || (svh->vopType == PVOP));
    EncPutBits(stream, svh->vopType, 3);
    COMMENT("Picture Coding Type");

    /* Optional Reference Picture Resampling mode = 0 */
    /* Optional Reduced-Resolution Update mode = 0 */
    EncPutBits(stream, 0, 2);
    COMMENT("RPR and RRU");

    /* Rounding Type */
    if(svh->vopType == PVOP)
    {
        EncPutBits(stream, svh->roundControl, 1);
    }
    else
    {
        EncPutBits(stream, 0, 1);
    }
    COMMENT("Rounding Type");

    /* The rest MPPTYPE bits */
    EncPutBits(stream, 1, 3);
    COMMENT("Two reserved zero and 1");

    /* CPM */
    EncPutBits(stream, 0, 1);
    COMMENT("CPM");

    /* CPFMT (Custom Picture Format */
    if((oppType == ENCHW_YES) && (svh->sourceFormat == 6))
    {

        /* Pixel Aspec Ratio 4 bits (12:11 (CIF for 4:3 picture)) */
        EncPutBits(stream, 2, 4);
        COMMENT("Pixel Aspect Ratio");

        /* Picture Width Indication */
        EncPutBits(stream, svh->pwi, 9);
        COMMENT("Picture Width Indication");

        /* Marker Bit */
        EncPutBits(stream, 1, 1);
        COMMENT("Marker Bit");

        /* Picture Height Indication */
        EncPutBits(stream, svh->phi, 9);
        COMMENT("Picture Height Indication");
    }
}

/*------------------------------------------------------------------------------

       EncGobFrameIdUpdate

------------------------------------------------------------------------------*/
void EncGobFrameIdUpdate(svh_s * svh)
{
    u32 newGobFrameId;

    newGobFrameId = GobFrameId(svh);

    if(svh->gobFrameId != newGobFrameId)
    {
        /* Some field differ so update gobFrameId */
        svh->gobFrameIdBit = (svh->gobFrameIdBit + 1) & 0x03;
        svh->gobFrameId = newGobFrameId;
    }
}

/*------------------------------------------------------------------------------

	EncSvhCheck

------------------------------------------------------------------------------*/
bool_e EncSvhCheck(svh_s * svh)
{
    bool_e status = ENCHW_OK;
    i32 width;
    i32 height;

    if(svh->header == ENCHW_NO)
    {
        return ENCHW_OK;
    }

    width = svh->videoObjectLayerWidth;
    height = svh->videoObjectLayerHeight;

    if((width == 128) && (height == 96))
    {
        svh->sourceFormat = 1;
    }
    else if((width == 176) && (height == 144))
    {
        svh->sourceFormat = 2;
    }
    else if((width == 352) && (height == 288))
    {
        svh->sourceFormat = 3;
    }
    else if((width == 704) && (height == 576))
    {
        svh->sourceFormat = 4;
    }
    else if((width == 1408) && (height == 1152))
    {
        svh->sourceFormat = 5;
    }
    else
    {
        svh->sourceFormat = 6;  /* Custom source format */
    }

    if(svh->sourceFormat == 6)
    {
        svh->plusHeader = ENCHW_YES;
        svh->pwi = width / 4 - 1;
        svh->phi = height / 4;
        if(width != ((svh->pwi + 1) * 4) || (height != (svh->phi * 4)))
        {
            status = ENCHW_NOK;
        }
    }

    /* GOB */
    if(height <= 400)
    {
        svh->mbInGob = 1;
    }
    else if(height <= 800)
    {
        svh->mbInGob = 2;
    }
    else
    {
        svh->mbInGob = 4;
    }
    svh->mbInGob *= (width + 15) / 16;

    {
        u32 mbs = (width + 15) / 16;

        mbs *= (height + 15) / 16;

        svh->gobs = (mbs + svh->mbInGob - 1) / svh->mbInGob;
    }

    svh->gobFrameId = GobFrameId(svh);

    return status;
}

/*------------------------------------------------------------------------------

	GobFrameId

------------------------------------------------------------------------------*/
u32 GobFrameId(svh_s * svh)
{
    u32 gobFrameId;

    gobFrameId = svh->splitScreenIndicator;
    gobFrameId = (gobFrameId << 1) + svh->documentCameraIndicator;
    gobFrameId = (gobFrameId << 1) + svh->fullPictureFreezeRelease;
    gobFrameId = (gobFrameId << 3) + svh->sourceFormat;
    gobFrameId = (gobFrameId << 1) + svh->vopType;

    return gobFrameId;
}

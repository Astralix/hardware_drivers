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
--  Abstract  : 
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264Slice.h,v $
--  $Date: 2010/04/19 09:19:03 $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __H264_SLICE_H__
#define __H264_SLICE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "enccommon.h"
#include "H264PutBits.h"
#include "H264NalUnit.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/
typedef enum
{
    PSLICE = 0,
    ISLICE = 2,
    PSLICES = 5,
    ISLICES = 7
} sliceType_e;

typedef struct
{
    true_e byteStream;
    u32 sliceSize;
    sliceType_e sliceType;
    nalUnitType_e nalUnitType;
    u32 picParameterSetId;
    u32 prevFrameNum;
    u32 frameNum;
    u32 frameNumBits;
    u32 idrPicId;
    u32 nalRefIdc;
    u32 disableDeblocking;
    i32 filterOffsetA;
    i32 filterOffsetB;
    i32 sliceQpDelta;
} slice_s;

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void H264SliceInit(slice_s * slice);
void H264SliceHdr(stream_s *stream, slice_s *slice);

#endif

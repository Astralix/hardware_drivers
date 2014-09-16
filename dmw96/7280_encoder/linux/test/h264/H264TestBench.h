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
--  Abstract : H264 Encoder testbench for linux
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: H264TestBench.h,v $
--  $Revision: 1.6 $
--
------------------------------------------------------------------------------*/
#ifndef _H264TESTBENCH_H_
#define _H264TESTBENCH_H_

#include "basetype.h"
#include "h264encapi.h"

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -100

/* Structure for command line options */
typedef struct
{
    char *input;
    char *output;
    char *userData;
    i32 firstVop;
    i32 lastVop;
    i32 width;
    i32 height;
    i32 lumWidthSrc;
    i32 lumHeightSrc;
    i32 horOffsetSrc;
    i32 verOffsetSrc;
    i32 outputRateNumer;
    i32 outputRateDenom;
    i32 inputRateNumer;
    i32 inputRateDenom;
    i32 level;
    i32 hrdConformance;
    i32 cpbSize;
    i32 intraVopRate;
    i32 constIntraPred;
    i32 disableDeblocking;
    i32 mbPerSlice;
    i32 qpHdr;
    i32 qpMin;
    i32 qpMax;
    i32 bitPerSecond;
    i32 vopRc;
    i32 mbRc;
    i32 vopSkip;
    i32 rotation;
    i32 inputFormat;
    i32 colorConversion;
    i32 videoBufferSize;
    i32 videoRange;
    i32 chromaQpOffset;
    i32 filterOffsetA;
    i32 filterOffsetB;
    i32 testId;
    i32 burst;
    i32 bursttype;
    i32 sei;
    i32 byteStream;
    i32 videoStab;
    i32 notCoded;
    i32 notCodedInterval;
} commandLine_s;

void TestNaluSizes(i32 * pNaluSizes, u8 * stream, u32 strmSize, u32 vopBytes);


#endif

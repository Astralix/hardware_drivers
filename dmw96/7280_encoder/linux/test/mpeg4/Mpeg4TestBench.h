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
--  Abstract : MPEG4 Encoder testbench
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: Mpeg4TestBench.h,v $
--  $Revision: 1.5 $
--
------------------------------------------------------------------------------*/
#ifndef _MPEG4TESTBENCH_H_
#define _MPEG4TESTBENCH_H_


#include "basetype.h"
#include "mp4encapi.h"

/*--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define DEFAULT -100
#define UNCHANGED -100

#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif


/* Structure for command line options */
typedef struct {
	char *input;
	char *output;
	char *userDataVos;
	char *userDataVisObj;
	char *userDataVol;
	char *userDataGov;
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
	i32 scheme;
	i32 profile;
	i32 videoRange;
	i32 intraVopRate;
	i32 goVopRate;
	i32 vpSize;
	i32 dataPart;
	i32 rvlc;
	i32 hec;
	i32 gobPlace;
	i32 bitPerSecond;
	i32 vopRc;
	i32 mbRc;
	i32 videoBufferSize;
	i32 vopSkip;
	i32 qpHdr;
	i32 qpMin;
	i32 qpMax;
	i32 rotation;
	i32 inputFormat;
	i32 colorConversion;
    i32 testId;
    i32 rlcBufferSize;
    i32 mbPerInterrupt;
    i32 burst;
    i32 videoStab;    
} commandLine_s;

void InternalTestValues(commandLine_s * cml, MP4EncInst enc);
void TestVpSizes(u32 * pVpSizes, u8 * stream, u32 strmSize, u32 vopBytes);

#endif


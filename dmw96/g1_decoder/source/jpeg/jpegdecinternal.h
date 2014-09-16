/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : Jpeg Decoder Internal functions
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------

    Table of contents 
   
    1. Include headers
    2. Module defines
    3. Data types
    4. Function prototypes

------------------------------------------------------------------------------*/

#ifndef JPEGDECINTERNAL_H
#define JPEGDECINTERNAL_H

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "jpegdecapi.h"
#include "jpegdeccontainer.h"

#ifdef JPEGDEC_ASIC_TRACE
#include <stdio.h>
#endif

/*------------------------------------------------------------------------------
    2. Module defines
------------------------------------------------------------------------------*/
#ifdef JPEGDEC_ASIC_TRACE
#define JPEGDEC_TRACE_INTERNAL(args) printf args
#else
#define JPEGDEC_TRACE_INTERNAL(args)
#endif

/*------------------------------------------------------------------------------
    3. Data types
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
void JpegDecClearStructs(JpegDecContainer * pJpegDecCont);
JpegDecRet JpegDecInitHW(JpegDecContainer * pJpegDecCont);
void JpegDecInitHWContinue(JpegDecContainer * pJpegDecCont);
void JpegDecInitHWInputBuffLoad(JpegDecContainer * pJpegDecCont);
void JpegDecInitHWProgressiveContinue(JpegDecContainer * pJpegDecCont);
void JpegDecInitHWNonInterleaved(JpegDecContainer * pJpegDecCont);
JpegDecRet JpegDecAllocateResidual(JpegDecContainer * pJpegDecCont);
void JpegDecSliceSizeCalculation(JpegDecContainer * pJpegDecCont);
JpegDecRet JpegDecNextScanHdrs(JpegDecContainer * pJpegDecCont);
void JpegRefreshRegs(JpegDecContainer * pJpegDecCont);
void JpegFlushRegs(JpegDecContainer * pJpegDecCont);
void JpegDecInitHWEmptyScan(JpegDecContainer * pJpegDecCont, u32 componentId);

#endif /* #endif JPEGDECDATA_H */

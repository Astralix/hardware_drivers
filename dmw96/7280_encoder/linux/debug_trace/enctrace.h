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
--  Description : Internal traces
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: enctrace.h,v $
--  $Revision: 1.3 $
--
------------------------------------------------------------------------------*/

#ifndef __ENCTRACE_H__
#define __ENCTRACE_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "encpreprocess.h"
#include "encasiccontroller.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/
i32 EncPrintBlock(i32 *, char *);

void EncTraceAsicParameters(asicData_s * asic);

void EncTraceAsicQp(i32 qp);
void EncTraceAsicStatus(i32 status, u32 mbNum);
void EncTraceAsicEvent(i32 mbNum);

void EncTracePreProcess(preProcess_s * preProcess);
void EncTraceStabilator(preProcess_s * preProcess, i32 horGmv, i32 verGmv);

void EncTraceRlc(const u32 * ptr, u32 block, u32 run, i32 level);
void EncTraceRlcMb(u32 mbNum, u32 * lastRlc);

void EncTraceRegs(const void *ewl, u32 mbNum);

void EncDumpControl(const u32 * data, u32 length);
void EncDumpRlc(const i16 * data, u32 length);
void EncDumpRlcMb(const i16 * data);
void EncDumpRecon(asicData_s * asic);

void EncTraceBufferingInit(hwSwBuffering_s *);
void EncTraceBuffering(hwSwBuffering_s *);
void EncTraceBufferingSwEnd(hwSwBuffering_s * buff);
void EncTraceBufferingHwEnd(hwSwBuffering_s * buff);
void EncTraceBufferingAsicEnable(hwSwBuffering_s * buff);

void EncTraceCloseAll(void);

#endif

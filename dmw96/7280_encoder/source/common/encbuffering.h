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
--  Description : HW/SW Buffering
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: encbuffering.h,v $
--  $Revision: 1.1 $
--
------------------------------------------------------------------------------*/

#ifndef __ENC_BUFFERING_H__
#define __ENC_BUFFERING_H__

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "basetype.h"
#include "ewl.h"
#include "encasiccontroller.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

TRACE_BUFFERING: Write buffering status trace to file

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Function prototypes
------------------------------------------------------------------------------*/

/* Allocate buffers */
bool_e EncAllocateBuffers(asicData_s * asic,
                          i32 bufferAmount, i32 bufferSize, i32 mbPerFrame);

/* Free buffers */
void EncFreeBuffers(asicData_s * asic);

/* Reset buffers in the beginning of each frame */
void EncResetBuffers(hwSwBuffering_s * buffering);

/* Get the previous state 
bufferState_e EncGetPreviousState(hwSwBuffering_s *buffering);*/

/* Evaluate and possibly perform a state change and return the new state */
bufferState_e EncNextState(asicData_s * asic);

/* If state is HWON_SWON or HWOFF_SWON get the SW input buffer position */
u32 *EncGetSwBufferPos(hwSwBuffering_s * buffering);

/* After SW processing set the reached buffer position */
void EncSetSwBufferPos(hwSwBuffering_s * buffering, u32 * bufferPos);

/* Get the next HW buffer bus address */
u32 EncGetHwBufferBase(hwSwBuffering_s * buffering);

/* If end of buffer is reached by HW set it accordingly */
void EncSetHwBufferFull(hwSwBuffering_s * buffering, u32 bufferedWords);

/* Set the counter of HW processed macroblocks */
void EncSetHwMb(hwSwBuffering_s * buffering, i32 hwMb);

/* Increase the counter of SW processed macroblocks */
void EncSetSwMbNext(hwSwBuffering_s * buffering);

/* HW error detected => exit straight to DONE */
void EncSetHwError(hwSwBuffering_s * buffering);

#endif

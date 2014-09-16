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
--  Description : HW/SW buffer handling
--
--------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: encbuffering.c,v $
--  $Revision: 1.2 $
--
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
#include "encbuffering.h"
#include "enccommon.h"

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

TRACE_BUFFERING

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static i32 NextBuffer(i32 bufferNum, i32 bufferAmount);

/*------------------------------------------------------------------------------

    Allocate buffers

------------------------------------------------------------------------------*/
bool_e EncAllocateBuffers(asicData_s * asic,
                          i32 bufferAmount, i32 bufferSize, i32 mbPerFrame)
{
    i32 i, j;
    hwSwBuffering_s *buffering;

    ASSERT(asic);

    buffering = &asic->buffering;

    if(bufferAmount > ENC7280_BUFFER_AMOUNT)
        return ENCHW_NOK;

    buffering->bufferAmount = bufferAmount;
    buffering->mbPerFrame = mbPerFrame;

#ifdef TRACE_BUFFERING
    EncTraceBufferingInit(&asic->buffering);
#endif

    for(i = 0; i < bufferAmount; i++)
    {
        i32 ret = EWLMallocLinear(asic->ewl, bufferSize, &buffering->buffer[i]);

        /* If any of the malloc's fails, free all allocated buffers */
        if(ret != EWL_OK || buffering->buffer[i].virtualAddress == NULL)
        {
            for(j = 0; j < i; j++)
            {
                EWLFreeLinear(asic->ewl, &buffering->buffer[j]);
                buffering->buffer[j].virtualAddress = NULL;
                buffering->buffer[j].busAddress = 0;
                buffering->buffer[j].size = 0;
            }
            buffering->bufferAmount = 0;
            buffering->mbPerFrame = 0;

            return ENCHW_NOK;
        }
    }

    for(i = 0; i < bufferAmount; i++)
    {
        /* Buffer limit in 32-bit words */
        buffering->bufferLimit[i] = (bufferSize - ENC7280_BUFFER_LIMIT) / 4;
    }

    EncResetBuffers(&asic->buffering);

    return ENCHW_OK;
}

/*------------------------------------------------------------------------------

    Free all allocated buffers

------------------------------------------------------------------------------*/
void EncFreeBuffers(asicData_s * asic)
{
    u32 i;
    hwSwBuffering_s *buffering;

    ASSERT(asic);

    buffering = &asic->buffering;

    for(i = 0; i < buffering->bufferAmount; i++)
    {
        EWLFreeLinear(asic->ewl, &buffering->buffer[i]);
        buffering->buffer[i].virtualAddress = NULL;
        buffering->buffer[i].busAddress = 0;
        buffering->buffer[i].size = 0;
        buffering->bufferLimit[i] = 0;
        buffering->bufferLast[i] = NULL;
        buffering->bufferFull[i] = 0;
    }

    buffering->bufferAmount = 0;

    EncResetBuffers(&asic->buffering);
}

/*------------------------------------------------------------------------------

    Reset the buffers to empty and IDLE state

------------------------------------------------------------------------------*/
void EncResetBuffers(hwSwBuffering_s * buffering)
{
    u32 i;

    ASSERT(buffering);

    for(i = 0; i < buffering->bufferAmount; i++)
    {
        buffering->bufferFull[i] = 0;
    }

    buffering->hwBuffer = 0;
    buffering->swBuffer = 0;
    buffering->swBufferPos =
        buffering->buffer[buffering->swBuffer].virtualAddress;
    buffering->hwMb = 0;
    buffering->swMb = 0;
    buffering->state = IDLE;

#ifdef TRACE_BUFFERING
    EncTraceBuffering(buffering);
#endif

}

/*------------------------------------------------------------------------------
    
    Evaluate and possibly perform a state change and return the new state.
    This must be executed between each macroblock. The state machine 
    implemented in this function is documented in SW Architecture Specification.
    
------------------------------------------------------------------------------*/
bufferState_e EncNextState(asicData_s * asic)
{
    hwSwBuffering_s *buffering;

    ASSERT(asic);

    buffering = &asic->buffering;

    /* Note:
     * hwMb is the last macroblock encoded by HW [0..mbPerFrame-1]
     * swMb is the next macroblock that shall be encoded by SW */

    if(buffering->swMb >= buffering->mbPerFrame)
    {
        /* Frame ready => DONE */
        buffering->state = DONE;
    }
    else if(buffering->state == IDLE)
    {
        /* IDLE state => start HW and change state */
        buffering->state = HWON_SWOFF;

        /* Set HW regs */
        asic->regs.rlcBase = buffering->buffer[buffering->hwBuffer].busAddress;
        asic->regs.rlcLimitSpace = buffering->bufferLimit[buffering->hwBuffer];

        buffering->bufferLast[buffering->hwBuffer] =
            buffering->buffer[buffering->hwBuffer].virtualAddress +
            buffering->bufferLimit[buffering->hwBuffer];

#ifdef TRACE_BUFFERING
        EncTraceBufferingAsicEnable(&asic->buffering);
#endif

        /* Write regs and enable HW */
        EncAsicFrameStart(asic->ewl, &asic->regs);

    }
    else if(buffering->state == HWON_SWOFF)
    {
        /* HW is encoding but SW is not => check if
         * 1) HW buffer full => HW paused and SW can encode
         * 2) HW ahead of SW => SW can encode */
        if(buffering->bufferFull[buffering->hwBuffer] == 1)
        {
            /* HW reached end of buffer */
            buffering->state = HWOFF_SWON;

            /* Move on to next buffer */
            buffering->hwBuffer =
                NextBuffer(buffering->hwBuffer, buffering->bufferAmount);
        }
        else if(buffering->hwMb >= buffering->swMb)
        {
            /* HW ahead of SW */
            buffering->state = HWON_SWON;
        }

    }
    else if(buffering->state == HWON_SWON)
    {
        /* HW and SW are encoding => check if
         * 1) HW finished frame => HW stop and SW can encode
         * 2) HW buffer full => HW paused and SW can encode
         * 3) SW reached HW => HW encodes and SW must wait */
        if(buffering->hwMb >= (buffering->mbPerFrame - 1))
        {
            /* HW reached end of frame */
            buffering->state = HWOFF_SWON;
        }
        else if(buffering->bufferFull[buffering->hwBuffer] == 1)
        {
            /* HW reached end of buffer */
            buffering->state = HWOFF_SWON;

            /* Move on to next buffer */
            buffering->hwBuffer =
                NextBuffer(buffering->hwBuffer, buffering->bufferAmount);
        }
        else if(buffering->swMb > buffering->hwMb)
        {
            /* SW reached HW */
            buffering->state = HWON_SWOFF;
        }

    }
    else if(buffering->state == HWOFF_SWON)
    {
        /* HW is paused or stopped and SW is encoding => check if
         * 1) SW reached HW => HW must be restarted
         * 2) empty buffer available for HW and HW not finished frame 
         *      => HW must be restarted */
        if(buffering->swMb > buffering->hwMb)
        {
            /* SW reached HW => SW off
             * This also means that all the buffers are empty => HW on */
            buffering->state = HWON_SWOFF;
        }
        else if((buffering->bufferFull[buffering->hwBuffer] == 0) &&
                (buffering->hwMb < (buffering->mbPerFrame - 1)))
        {
            /* Empty buffer available for HW and frame not ready => continue */
            buffering->state = HWON_SWON;
        }

        /* If state is changing => restart HW */
        if(buffering->state != HWOFF_SWON)
        {
            /* Reset the buffer to empty */
            buffering->bufferFull[buffering->hwBuffer] = 0;
            buffering->bufferLast[buffering->hwBuffer] =
                buffering->buffer[buffering->hwBuffer].virtualAddress +
                buffering->bufferLimit[buffering->hwBuffer];

            /* Put new buffer to regs and restart HW */
            asic->regs.rlcBase =
                buffering->buffer[buffering->hwBuffer].busAddress;
            asic->regs.rlcLimitSpace =
                buffering->bufferLimit[buffering->hwBuffer];

#ifdef TRACE_BUFFERING
            EncTraceBufferingAsicEnable(&asic->buffering);
#endif

            EncAsicFrameContinue(asic->ewl, &asic->regs);

        }
    }

#ifdef TRACE_BUFFERING
    EncTraceBuffering(&asic->buffering);
#endif

    return buffering->state;
}

/*------------------------------------------------------------------------------
    
    Get the SW input buffer position or NULL if SW should wait for HW
    
------------------------------------------------------------------------------*/
u32 *EncGetSwBufferPos(hwSwBuffering_s * buffering)
{
    ASSERT(buffering);

    if((buffering->state == HWON_SWON) || (buffering->state == HWOFF_SWON))
        return buffering->swBufferPos;
    else
        return NULL;
}

/*------------------------------------------------------------------------------
    
    After SW processing set the reached buffer position
    
------------------------------------------------------------------------------*/
void EncSetSwBufferPos(hwSwBuffering_s * buffering, u32 * bufferPos)
{
    ASSERT(buffering);
    /*ASSERT(bufferPos <= buffering->bufferLast[buffering->swBuffer]); */

    if(bufferPos >= buffering->bufferLast[buffering->swBuffer])
    {
        /* SW has reached end of the buffer
         * => buffer is marked empty and pointer is moved to the beginning
         * of next buffer */

#ifdef TRACE_BUFFERING
        EncTraceBufferingSwEnd(buffering);
#endif

        buffering->bufferFull[buffering->swBuffer] = 0;
        buffering->swBuffer =
            NextBuffer(buffering->swBuffer, buffering->bufferAmount);
        buffering->swBufferPos =
            buffering->buffer[buffering->swBuffer].virtualAddress;
    }
    else
    {
        buffering->swBufferPos = bufferPos;
    }
}

/*------------------------------------------------------------------------------
    
    Get next HW buffer 
    
------------------------------------------------------------------------------*/
u32 EncGetHwBufferBase(hwSwBuffering_s * buffering)
{
    ASSERT(buffering);

    return buffering->buffer[buffering->hwBuffer].busAddress;
}

/*------------------------------------------------------------------------------

    End of buffer is reached by HW and the size of data in buffer is known

------------------------------------------------------------------------------*/
void EncSetHwBufferFull(hwSwBuffering_s * buffering, u32 bufferedBytes)
{
    ASSERT(buffering);

    buffering->bufferFull[buffering->hwBuffer] = 1;
    buffering->bufferLast[buffering->hwBuffer] =
        buffering->buffer[buffering->hwBuffer].virtualAddress +
        bufferedBytes / 4;

#ifdef TRACE_BUFFERING
    EncTraceBufferingHwEnd(buffering);
#endif
}

/*------------------------------------------------------------------------------

    Set the index of last HW processed macroblock

------------------------------------------------------------------------------*/
void EncSetHwMb(hwSwBuffering_s * buffering, i32 hwMb)
{
    ASSERT(buffering);

    buffering->hwMb = hwMb;
}

/*------------------------------------------------------------------------------

    Increase the counter of next SW macroblock

------------------------------------------------------------------------------*/
void EncSetSwMbNext(hwSwBuffering_s * buffering)
{
    ASSERT(buffering);

    buffering->swMb++;
}

/*------------------------------------------------------------------------------

    HW error detected => exit straight to DONE

------------------------------------------------------------------------------*/
void EncSetHwError(hwSwBuffering_s * buffering)
{
    ASSERT(buffering);

    buffering->swMb = buffering->mbPerFrame;
}

/*------------------------------------------------------------------------------

    Get the index number for next buffer

------------------------------------------------------------------------------*/
i32 NextBuffer(i32 bufferNum, i32 bufferAmount)
{
    bufferNum++;
    if(bufferNum >= bufferAmount)
        bufferNum = 0;

    return bufferNum;
}

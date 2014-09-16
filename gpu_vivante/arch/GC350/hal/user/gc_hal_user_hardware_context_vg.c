/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




#include "gc_hal_user_hardware_precomp_vg.h"

#if gcdENABLE_VG

#define _GC_OBJ_ZONE            gcvZONE_VG

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define gcdEMPTY_MOD_LIST \
    ((gcsVGCONTEXT_MAP_PTR) ~0)

#define _STATE(reg) \
    StateInitFunction(\
        Hardware, \
        InitInfo, \
        index, \
        reg ## _Address >> 2, \
        reg ## _ResetValue, \
        reg ## _Count \
        )

#define _STATE_COUNT(reg, count) \
    StateInitFunction(\
        Hardware, \
        InitInfo, \
        index, \
        reg ## _Address >> 2, \
        reg ## _ResetValue, \
        count \
        )

#define _STATE_X(reg) \
    StateInitFunction(\
        Hardware, \
        InitInfo, \
        index, \
        reg ## _Address >> 2, \
        reg ## _ResetValue, \
        reg ## _Count \
        )

#define _ALIGN_BUFFER() \
    StateInitFunction(\
        Hardware, \
        InitInfo, \
        index, \
        0, 0, 0 \
        )

typedef struct _gcsVGCONTEXT_INIT * gcsVGCONTEXT_INIT_PTR;
typedef struct _gcsVGCONTEXT_INIT
{
    gctUINT32       loadIndex;
    gctUINT32       stateIndex;
    gctSIZE_T       valueCount;
    gctUINT32       bufferIndex;
}
gcsVGCONTEXT_INIT;

typedef gctSIZE_T (* gctSTATE_INIT) (
    IN gcoVGHARDWARE Hardware,
    IN gcsVGCONTEXT_INIT_PTR InitInfo,
    IN gctSIZE_T BufferIndex,
    IN gctUINT32 StateIndex,
    IN gctUINT32 ResetValue,
    IN gctSIZE_T ValueCount
    );


/*******************************************************************************
**
**  _MeasureState
**
**  Measure the size of the state.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctSIZE_T BufferIndex
**          Current index within the UINT32 context buffer.
**
**      gctUINT32 StateIndex
**          Address of the state in its index form (for ex. 0x0E03).
**
**      gctUINT32 ResetValue
**          Initial value of the state after chip reset.
**
**      gctSIZE_T ValueCount
**          Number of values for this state.
**
**  OUTPUT:
**
**      The number of buffer indices needed for the states.
*/

static gctSIZE_T
_MeasureState(
    IN gcoVGHARDWARE Hardware,
    IN gcsVGCONTEXT_INIT_PTR InitInfo,
    IN gctSIZE_T BufferIndex,
    IN gctUINT32 StateIndex,
    IN gctUINT32 ResetValue,
    IN gctSIZE_T ValueCount
    )
{
    gctSIZE_T result;
    gctSIZE_T mapLast;

    /* End of buffer alignement? */
    if (ValueCount == 0)
    {
        /* Save information for this LoadState. */
        InitInfo->bufferIndex = gcmALIGN(BufferIndex, 2);

        /* Return end of buffer. */
        result = 0;
    }
    else
    {
        /* See if we can append this state to the previous one. */
        if (StateIndex == InitInfo->stateIndex)
        {
            /* Update the state index. */
            InitInfo->stateIndex += ValueCount;

            /* Return number of slots required. */
            result = ValueCount;
        }
        else
        {
            /* Determine if we need alignment. */
            gctSIZE_T align = BufferIndex & 1;

            /* Save information for this LoadState. */
            InitInfo->bufferIndex = BufferIndex + align;
            InitInfo->stateIndex  = StateIndex  + ValueCount;

            /* Return number of slots required. */
            result = align + 1 + ValueCount;
        }

        /* Determine the new map size. */
        mapLast = StateIndex + ValueCount - 1;

        /* Update the map dimensions as necessary. */
#ifndef __QNXNTO__
        if (mapLast > Hardware->context.mapLast)
        {
            Hardware->context.mapLast = mapLast;
        }

        if (StateIndex < Hardware->context.mapFirst)
        {
            Hardware->context.mapFirst = StateIndex;
        }
#else
        if (mapLast > Hardware->pContext->mapLast)
        {
            Hardware->pContext->mapLast = mapLast;
        }

        if (StateIndex < Hardware->pContext->mapFirst)
        {
            Hardware->pContext->mapFirst = StateIndex;
        }
#endif
    }

    /* Return size for load state. */
    return result;
}


/*******************************************************************************
**
**  _InitState
**
**  Initialize and map the specified state or a set of states.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctSIZE_T BufferIndex
**          Current index within the UINT32 context buffer.
**
**      gctUINT32 StateIndex
**          Address of the state in its index form (for ex. 0x0E03).
**
**      gctUINT32 ResetValue
**          Initial value of the state after chip reset.
**
**      gctSIZE_T ValueCount
**          Number of values for this state.
**
**  OUTPUT:
**
**      The number of buffer indices needed for the states.
*/

static gctSIZE_T
_InitState(
    IN gcoVGHARDWARE Hardware,
    IN gcsVGCONTEXT_INIT_PTR InitInfo,
    IN gctSIZE_T BufferIndex,
    IN gctUINT32 StateIndex,
    IN gctUINT32 ResetValue,
    IN gctSIZE_T ValueCount
    )
{
    gctSIZE_T result;
    gctUINT32_PTR buffer;
    gcsVGCONTEXT_MAP_PTR mapPrev;
    gcsVGCONTEXT_MAP_PTR mapCurr;
    gctSIZE_T i;

    /* Create shortcuts. */
#ifndef __QNXNTO__
    buffer  = Hardware->context.buffer;
    mapPrev = Hardware->context.mapPrev;
    mapCurr = Hardware->context.mapCurr;
#else
    buffer  = Hardware->pContext->buffer;
    mapPrev = Hardware->pContext->mapPrev;
    mapCurr = Hardware->pContext->mapCurr;
#endif

    /* End of buffer alignemnt? */
    if (ValueCount == 0)
    {
        /* Determine if we need alignment. */
        gctSIZE_T align = BufferIndex & 1;

        /* Align in case when the previous state left the context
           buffer unaligned. */
        if (align)
        {
            /* Put a filler in place. */
            buffer[BufferIndex] = 0xDEADDEAD;

            /* Advance the index. */
            BufferIndex += 1;
        }

        /* Save information for this LoadState. */
        InitInfo->bufferIndex = BufferIndex;

        /* Return number of slots required. */
        result = 0;
    }
    else
    {
        /* Determnine map index. */
#ifndef __QNXNTO__
        gctUINT32 mapIndex = StateIndex - Hardware->context.mapFirst;
#else
        gctUINT32 mapIndex = StateIndex - Hardware->pContext->mapFirst;
#endif

        /* See if we can append this state to the previous one. */
        if (StateIndex == InitInfo->stateIndex)
        {
            /* Update last load state. */
            gcmVERIFY_OK(gcoVGHARDWARE_StateCommand(
                Hardware,
                &buffer[InitInfo->bufferIndex],
                InitInfo->loadIndex,
                InitInfo->valueCount + ValueCount,
                gcvNULL
                ));

            /* Walk all the states. */
            for (i = 0; i < ValueCount; ++i)
            {
                /* Set state to uninitialized value. */
                buffer[BufferIndex + i] = ResetValue;

                /* Set index in state mapping table. */
                mapPrev[mapIndex + i].index = BufferIndex + i;
                mapCurr[mapIndex + i].index = BufferIndex + i;
            }

            /* Update last address and size. */
            InitInfo->stateIndex += ValueCount;
            InitInfo->valueCount += ValueCount;

            /* Return number of slots required. */
            result = ValueCount;
        }
        else
        {
            /* Determine if we need alignment. */
            gctSIZE_T align = BufferIndex & 1;

            /* Align in case when the previous state left the context
               buffer unaligned. */
            if (align)
            {
                /* Put a filler in place. */
                buffer[BufferIndex] = 0xDEADDEAD;

                /* Advance the index. */
                BufferIndex += 1;
            }

            /* Assemble load state command. */
            gcmVERIFY_OK(gcoVGHARDWARE_StateCommand(
                Hardware,
                &buffer[BufferIndex],
                StateIndex,
                ValueCount,
                gcvNULL
                ));

            /* Walk all the states. */
            for (i = 0; i < ValueCount; ++i)
            {
                /* Set state to uninitialized value. */
                buffer[BufferIndex + 1 + i] = ResetValue;

                /* Set index in state mapping table. */
                mapPrev[mapIndex + i].index = BufferIndex + 1 + i;
                mapCurr[mapIndex + i].index = BufferIndex + 1 + i;
            }

            /* Save information for this LoadState. */
            InitInfo->loadIndex   = StateIndex;
            InitInfo->bufferIndex = BufferIndex;
            InitInfo->stateIndex  = StateIndex + ValueCount;
            InitInfo->valueCount  = ValueCount;

            /* Return number of slots required. */
            result = align + 1 + ValueCount;
        }
    }

    /* Return size for load state. */
    return result;
}


/*******************************************************************************
**
**  _InitializeContextBuffer
**
**  Initialize and map all hardware states.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctSTATE_INIT StateInitFunction
**          Pointer to the init function.
**
**      gcsVGCONTEXT_INIT_PTR InitInfo
**          Pointer to the initialization info structure.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS
_InitializeContextBuffer(
    IN gcoVGHARDWARE Hardware,
    IN gctSTATE_INIT StateInitFunction,
    IN gcsVGCONTEXT_INIT_PTR InitInfo
    )
{
    /* Set initial index. */
    gctSIZE_T index = 0;

    /* Reset init info. */
    InitInfo->stateIndex = (gctUINT32)~0;

    /* VG states. */
    index += StateInitFunction(Hardware, InitInfo, index, 0x02800 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02804 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02808 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0280C >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02810 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02818 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02820 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02828 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0282C >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02830 >> 2, 0x00000000, 4);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02840 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02844 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02848 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0284C >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02850 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02854 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02858 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0285C >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02860 >> 2, 0x00000000, 3);

    /* Flush is not a part of the context. */
    /* index += _STATE(GCREG_VG_FLUSH); */              /* A1B x1  */

    index += StateInitFunction(Hardware, InitInfo, index, 0x02870 >> 2, 0x00000000, 3);

    /* Flush is not a part of the context. */
    /* index += _STATE(GCREG_VG_IMAGE_FLUSH); */        /* A1F x1  */

    index += StateInitFunction(Hardware, InitInfo, index, 0x02880 >> 2, 0x00000000, 3);

    /* Debug registers are not a part of the context. */
    /* index += _STATE(GCREG_VG_DEBUG0); */             /* A23 x1  */

    index += StateInitFunction(Hardware, InitInfo, index, 0x02890 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02898 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028A0 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028A8 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028B0 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028B8 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028C0 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028C4 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028C8 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028CC >> 2, 0x00000000, 1);

    if (!Hardware->vg20)
    {
        /* Latest TS control register has reset
           field in it - can't cache that. */
        index += StateInitFunction(Hardware, InitInfo, index, 0x028D0 >> 2, 0x00000000, 1);
    }

    index += StateInitFunction(Hardware, InitInfo, index, 0x028D4 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028D8 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028DC >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028E0 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028E4 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028E8 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028EC >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028F0 >> 2, 0x00000000, 1);

    /* TS clear is a command register. */
    /* index += _STATE(GCREG_TS_CLEAR); */              /* A3D x1  */

    index += StateInitFunction(Hardware, InitInfo, index, 0x028F8 >> 2, 0x00000055, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x028FC >> 2, 0x70707074, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02900 >> 2, 0x00000000, 6);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02918 >> 2, 0x75057545, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0291C >> 2, 0x70007000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02920 >> 2, 0x70707074, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02924 >> 2, 0x74007000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02928 >> 2, 0x70007000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0292C >> 2, 0x70007000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02930 >> 2, 0x70007000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02934 >> 2, 0x00402008, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02938 >> 2, 0x04001000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0293C >> 2, 0x00800200, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02940 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02948 >> 2, 0x00000000, 2);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02950 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02954 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02958 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x0295C >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02960 >> 2, 0x00000000, 1);
    index += StateInitFunction(Hardware, InitInfo, index, 0x02980 >> 2, 0x00000000, 25);

    /* Align the buffer. */
    _ALIGN_BUFFER();

    /* Success. */
    return gcvSTATUS_OK;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_OpenContext
**
**  Initialize the context management structures and buffers.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_OpenContext(
    IN gcoVGHARDWARE Hardware
    )
{
    gceSTATUS last, status;
    gcuVIDMEM_NODE_PTR node = gcvNULL;
    gcsVGCONTEXT_PTR context;
#ifdef __QNXNTO__
    gctPHYS_ADDR physical;
#endif

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gcsVGCONTEXT_INIT initInfo;
        gctUINT32 mapItemCount, mapSize;
        gctUINT32 stateSize, dataSize, bufferSize, dataCount;
        gctUINT32 headerAddress, bufferAddress, bufferOffset;
        gcsCMDBUFFER_PTR header;
        gctUINT32_PTR endCommand;

        /* Context shortcut. */
#ifndef __QNXNTO__
        context = &Hardware->context;
#else
        context = Hardware->pContext;
#endif

        /* Set initial pipe. */
        context->currentPipe = 0x2;

        /* Reset the mod list. */
        context->firstPrevMap = gcdEMPTY_MOD_LIST;
        context->firstCurrMap = gcdEMPTY_MOD_LIST;

        /* Reset map start index. */
        context->mapFirst = (gctSIZE_T)~0;

        /* Determine state caching enable flag.
           State caching depends on whether or not TS double buffering is
           enabled or not. The reason is that when the state caching is not
           enabled, there will be enough VG states to accumulate and block TS
           until VG has done it's current processing before TS is released
           again and cleared the contents of the TS buffer for the next
           primitive. When state caching is enabled, it will usually remove
           enough VG states from the command buffer to allow TS to go forward,
           which will lead to the problem with TS clear. We have to use two or
           three pairs of semaphore/stall to stall TS clear to avoid these
           issues. */
#if gcdENABLE_TS_DOUBLE_BUFFER
        /* With TS double buffering enabled on the high level, state caching
           enable will depend on whether double buffering is supported by
           the hardware. If double buffering is not supported, we have to
           disable state caching so that VG states will stall TS clear.
           If double buffering is supported, we can enable state caching
           to offload some of the states off of the command buffer and use
           semaphore/stall pairs for TS clear synchronization. */
        context->stateCachingEnabled = Hardware->vgDoubleBuffer;
#else
        /* Always enanble state caching here, there is a hardcoded
           semaphore/stall right before TS clear in this case. */
        context->stateCachingEnabled = gcvTRUE;
#endif

        /***********************************************************************
        ** Determine context buffer parameters.
        */

        /* Compute the size of the context buffer and the state map. */
        gcmERR_BREAK(_InitializeContextBuffer(
            Hardware, _MeasureState, &initInfo
            ));

        /* Compute parameters. */
        mapItemCount = context->mapLast - context->mapFirst + 1;
        mapSize      = mapItemCount * gcmSIZEOF(gcsVGCONTEXT_MAP) * 2;
        stateSize    = initInfo.bufferIndex * gcmSIZEOF(gctUINT32);
        dataSize     = stateSize + Hardware->bufferInfo.staticTailSize;
        dataCount    = dataSize  / Hardware->bufferInfo.commandAlignment;

        bufferSize
            = gcmSIZEOF(gcsCMDBUFFER)
            + Hardware->bufferInfo.addressAlignment
            + dataSize;


        /***********************************************************************
        ** Allocate the state map and the context buffer.
        */

        /* Allocate two maps with one allocation. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoOS_Allocate(
            Hardware->os, mapSize, (gctPOINTER *) &context->mapContainer
            ));
#else
        context->mapContainerSize = mapSize;
        gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &context->mapContainerSize, &physical, (gctPOINTER *) &context->mapContainer
            ));
#endif

        /* Reset the map. */
        gcmERR_BREAK(gcoOS_ZeroMemory(
            context->mapContainer, mapSize
            ));

        /* Determine map pointers. */
        context->mapPrev = context->mapContainer;
        context->mapCurr = context->mapContainer + mapItemCount;

        /* Allocate the context buffer. */
        gcmERR_BREAK(gcoVGHARDWARE_AllocateLinearVideoMemory(
            Hardware,
            bufferSize, 1,
            gcvPOOL_SYSTEM,
            &node, &headerAddress,
            (gctPOINTER *) &header
            ));


        /***********************************************************************
        ** Initialize the context buffer.
        */

        /* Determine the buffer address. */
        bufferAddress = gcmALIGN(
            headerAddress + gcmSIZEOF(gcsCMDBUFFER),
            Hardware->bufferInfo.addressAlignment
            );

        /* Compute the buffer offset from the header. */
        bufferOffset = bufferAddress - headerAddress;

        /* Store the header pointer. */
        context->header = header;

        /* Determine the buffer pointer. */
        context->buffer
            = (gctUINT32_PTR) (((gctUINT8_PTR) header) + bufferOffset);

        /* Initialize gcsCMDBUFFER. */
        header->completion    = gcvVACANT_BUFFER;
        header->node          = node;
        header->address       = bufferAddress;
        header->bufferOffset  = bufferOffset;
        header->size          = stateSize;
        header->offset        = stateSize;
        header->dataCount     = dataCount;
        header->nextAllocated = gcvNULL;
        header->nextSubBuffer = gcvNULL;

        /* Determine the location of the END command. */
        endCommand = context->buffer + initInfo.bufferIndex;

        /* Append the END command. */
        gcmERR_BREAK(gcoVGHARDWARE_EndCommand(
            Hardware,
            endCommand,
            Hardware->bufferInfo.feBufferInt, gcvNULL
            ));

        /* Initialize the context buffer with states. */
        gcmERR_BREAK(_InitializeContextBuffer(
            Hardware, _InitState, &initInfo
            ));

        gcmFOOTER_NO();
        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Free the context buffer. */
    if (node != gcvNULL)
    {
        gcmCHECK_STATUS(gcoVGHARDWARE_FreeVideoMemory(Hardware, node));
    }

    /* Free the state map. */
    if (context->mapContainer != gcvNULL)
    {
#ifndef __QNXNTO__
        gcmCHECK_STATUS(gcoOS_Free(Hardware->os, context->mapContainer));
#else
        gcmCHECK_STATUS(gcoOS_FreeNonPagedMemory(gcvNULL, context->mapContainerSize, gcvNULL, context->mapContainer));
#endif
    }
    gcmFOOTER();

    /* Return the status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_CloseContext
**
**  Release resources associated with context management.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_CloseContext(
    IN gcoVGHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Context shortcut. */
#ifndef __QNXNTO__
        gcsVGCONTEXT_PTR context = &Hardware->context;
#else
        gcsVGCONTEXT_PTR context = Hardware->pContext;
#endif

        /* Free the state mapping. */
        if (context->mapContainer != gcvNULL)
        {
            /* Free the map. */
#ifndef __QNXNTO__
            gcmERR_BREAK(gcoOS_Free(Hardware->os, context->mapContainer));
#else
            gcmERR_BREAK(gcoOS_FreeNonPagedMemory(gcvNULL, context->mapContainerSize, gcvNULL, context->mapContainer));
#endif

            /* Reset the pointer. */
            context->mapContainer = gcvNULL;
        }

        /* Free the context buffer. */
        if (context->header != gcvNULL)
        {
            /* Must have a node. */
            gcmASSERT(context->header->node != gcvNULL);

            /* Schedule the buffer. */
            gcmERR_BREAK(gcoVGHARDWARE_FreeVideoMemory(
                Hardware, context->header->node
                ));

            /* Reset the header. */
            context->header = gcvNULL;
        }
        gcmFOOTER_NO();
        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return the status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_MergeContext
**
**  Merge temporary context buffer changes if any to the main context buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_MergeContext(
    IN gcoVGHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gcsVGCONTEXT_PTR context;
        gcsVGCONTEXT_MAP_PTR temp;

        /* Context shortcut. */
#ifndef __QNXNTO__
        context = &Hardware->context;
#else
        context = Hardware->pContext;
#endif

        /* Check to see if there are any accumulated modifications
           to be merged in. */
        if (context->firstPrevMap != gcdEMPTY_MOD_LIST)
        {
            /* Get the first mod. */
            gcsVGCONTEXT_MAP_PTR mod = context->firstPrevMap;

            /* Buffer shortcut. */
            gctUINT32_PTR buffer = context->buffer;

            /* Make sure the context is idle. */
            gcmERR_BREAK(gcoVGHARDWARE_WaitCompletion(
                Hardware, context->header
                ));

            /* Copy all mods to the main context. */
            while (mod != gcdEMPTY_MOD_LIST)
            {
                /* Copy the current mod pointer. */
                gcsVGCONTEXT_MAP_PTR currMod = mod;

                /* Get the next mod and reset the list pointer. */
                mod = currMod->next;
                currMod->next = gcvNULL;

                /* Copy modified data. */
                buffer[currMod->index] = currMod->data;
            }
        }
        else
        {
            /* Success. */
            status = gcvSTATUS_OK;
        }

        /* Swap mod lists. */
        context->firstPrevMap = context->firstCurrMap;
        context->firstCurrMap = gcdEMPTY_MOD_LIST;

        temp = context->mapPrev;
        context->mapPrev = context->mapCurr;
        context->mapCurr = temp;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_UpdateState
**
**  Update all 32 bits of a single state in the context.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Address of the state to set.
**
**      gctUINT32 Data
**          State value to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_UpdateState(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gcsVGCONTEXT_PTR context;

        /* Context shortcut. */
#ifndef __QNXNTO__
        context = &Hardware->context;
#else
        context = Hardware->pContext;
#endif

        /* Is the state a part of the context? */
        if (Address <= context->mapLast)
        {
            /* Determine map entry index. */
            gctINT mapIndex = Address - context->mapFirst;

            /* Is the state a part of the context? */
            if (mapIndex >= 0)
            {
                /* Shortcut to the map entry. */
                gcsVGCONTEXT_MAP_PTR currMapEntry = &context->mapCurr[mapIndex];

                /* Lookup the index into the context buffer. */
                gctUINT32 dataIndex = currMapEntry->index;

                /* Is the state a part of the context? */
                if (dataIndex != 0)
                {
                    /* Has the state been loaded already within the current context? */
                    if (currMapEntry->next != gcvNULL)
                    {
                        if (context->stateCachingEnabled)
                        {
                            /* Verify if it's the same state value as before. */
                            if (currMapEntry->data == Data)
                            {
                                /* Same, ignore. */
                                status = gcvSTATUS_OK;
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (context->stateCachingEnabled)
                        {
                            /* Shortcut to the previous map entry. */
                            gcsVGCONTEXT_MAP_PTR prevMapEntry = &context->mapPrev[mapIndex];

                            /* Make sure the index is the same. */
                            gcmASSERT(prevMapEntry->index == dataIndex);

                            /* Has the state been previously loaded? */
                            if (prevMapEntry->next != gcvNULL)
                            {
                                /* Verify if it's the same state value as before. */
                                if (prevMapEntry->data == Data)
                                {
                                    /* Same, ignore. */
                                    status = gcvSTATUS_OK;
                                    break;
                                }
                            }
                            else
                            {
                                /* Verify if it's the same state value as before. */
                                if (context->buffer[dataIndex] == Data)
                                {
                                    /* Same, ignore. */
                                    status = gcvSTATUS_OK;
                                    break;
                                }
                            }
                        }

                        /* Value has changed, add it to the list of modifications. */
                        currMapEntry->next = context->firstCurrMap;
                        context->firstCurrMap = currMapEntry;
                    }

                    /* Update the data in the map. */
                    currMapEntry->data = Data;
                }
            }
        }

        /* The value has been modified. */
        status = gcvSTATUS_MISMATCH;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_SetState
**
**  Set all 32 bits of a single state.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Address of the state to set.
**
**      gctUINT32 Data
**          State value to set.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_SetState(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT32_PTR memory;

        /* Update the state */
        status = gcoVGHARDWARE_UpdateState(Hardware, Address, Data);

        /* No change? */
        if (gcmIS_SUCCESS(status))
        {
            break;
        }

        /* Reserve space in the command buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Hardware->buffer,
            8, gcvTRUE,
            (gctPOINTER *) &memory
            ));

        /* Assemble load state command. */
        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory, Address, 1,
            gcvNULL
            ));

        /* Set the state value. */
        memory[1] = Data;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGHARDWARE_SetStates
**
**  Set all 32 bits of a specified number of states.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Address of the state to set.
**
**      gctSIZE_T Count
**          The number of states to set.
**
**      gctPOINTER Data
**          Pointer to the state values.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_SetStates(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT i;
        gcsVGCONTEXT_PTR context;
        gctUINT32_PTR memory;

        /* Context shortcut. */
#ifndef __QNXNTO__
        context = &Hardware->context;
#else
        context = Hardware->pContext;
#endif

        /* Is the state a part of the context? */
        if (Address <= context->mapLast)
        {
            /* Determine map entry index. */
            gctINT mapIndex = Address - context->mapFirst;

            /* Is the state a part of the context? */
            if (mapIndex >= 0)
            {
                /* Shortcut to the map entry. */
                gcsVGCONTEXT_MAP_PTR currMapEntry = &context->mapCurr[mapIndex];

                /* Lookup the index into the context buffer. */
                gctUINT32 dataIndex = currMapEntry->index;

                /* Is the state a part of the context? */
                if (dataIndex != 0)
                {
                    /* Has the state been loaded already within the current context? */
                    if (currMapEntry->next != gcvNULL)
                    {
                        if (context->stateCachingEnabled)
                        {
                            /* Scan all states. */
                            for (i = 0; i < Count; i += 1)
                            {
                                /* All states in the group must be loaded already. */
                                gcmASSERT(currMapEntry[i].next != gcvNULL);

                                /* Has the state changed? */
                                if (currMapEntry[i].data != ((gctUINT32_PTR) Data)[i])
                                {
                                    break;
                                }
                            }

                            /* No change? */
                            if (i == Count)
                            {
                                /* Same, ignore. */
                                status = gcvSTATUS_OK;
                                break;
                            }
                        }

                        /* Update the data in the map. */
                        for (i = 0; i < Count; i += 1)
                        {
                            /* All states in the group must be loaded already. */
                            gcmASSERT(currMapEntry[i].next != gcvNULL);

                            /* Update the data in the map. */
                            currMapEntry[i].data = ((gctUINT32_PTR) Data)[i];
                        }
                    }
                    else
                    {
                        if (context->stateCachingEnabled)
                        {
                            /* Shortcut to the previous map entry. */
                            gcsVGCONTEXT_MAP_PTR prevMapEntry = &context->mapPrev[mapIndex];

                            /* Make sure the index is the same. */
                            gcmASSERT(prevMapEntry->index == dataIndex);

                            /* Has the state been previously loaded? */
                            if (prevMapEntry->next != gcvNULL)
                            {
                                /* Scan all states. */
                                for (i = 0; i < Count; i += 1)
                                {
                                    /* All states in the group must be loaded already. */
                                    gcmASSERT(prevMapEntry[i].next != gcvNULL);

                                    /* Has the state changed? */
                                    if (prevMapEntry[i].data != ((gctUINT32_PTR) Data)[i])
                                    {
                                        break;
                                    }
                                }

                                /* No change? */
                                if (i == Count)
                                {
                                    /* Same, ignore. */
                                    status = gcvSTATUS_OK;
                                    break;
                                }
                            }
                            else
                            {
                                /* Scan all states. */
                                for (i = 0; i < Count; i += 1)
                                {
                                    /* All states in the group must be loaded already. */
                                    gcmASSERT(currMapEntry[i].next == gcvNULL);

                                    /* Has the state changed? */
                                    if (context->buffer[dataIndex] != ((gctUINT32_PTR) Data)[i])
                                    {
                                        break;
                                    }
                                }

                                /* No change? */
                                if (i == Count)
                                {
                                    /* Same, ignore. */
                                    status = gcvSTATUS_OK;
                                    break;
                                }
                            }
                        }

                        /* Update the data in the map. */
                        for (i = 0; i < Count; i += 1)
                        {
                            /* All states in the group must be loaded already. */
                            gcmASSERT(currMapEntry->next == gcvNULL);

                            /* Update the data in the map. */
                            currMapEntry->data = ((gctUINT32_PTR) Data)[i];

                            /* Add the state to the mod list if not there yet. */
                            currMapEntry->next = context->firstCurrMap;
                            context->firstCurrMap = currMapEntry;

                            /* Advance to the next entry. */
                            currMapEntry += 1;
                        }
                    }
                }
            }
        }

        /* Reserve space in the command buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Hardware->buffer,
            gcmSIZEOF(gctUINT32) * (1 + Count), gcvTRUE,
            (gctPOINTER *) &memory
            ));

        /* Assemble load state command. */
        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory, Address, Count,
            gcvNULL
            ));

        /* Advance to the first data entry. */
        memory += 1;

        /* Set the state values. */
        for (i = 0; i < Count; i += 1)
        {
            /* Update the data. */
            memory[i] = ((gctUINT32_PTR) Data) [i];
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

#endif /* gcdENABLE_VG */


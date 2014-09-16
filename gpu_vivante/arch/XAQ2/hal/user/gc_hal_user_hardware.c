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




#include "gc_hal_user_hardware_precomp.h"
#include "gc_hal_user.h"
#include "gc_hal_user_brush.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

/* Multi-sample grid selection:
**
**  0   Square grid only.
**  1   Rotated diamonds but no jitter.
**  2   Rotated diamonds in a 2x2 jitter matrix.
*/
#define gcdGRID_QUALITY 1

/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

static gceSTATUS _ResetDelta(
    IN gcsSTATE_DELTA_PTR StateDelta
    )
{
    /* The delta should not be attached to any context. */
    gcmASSERT(StateDelta->refCount == 0);

    /* Not attached yet, advance the ID. */
    StateDelta->id += 1;

    /* Did ID overflow? */
    if (StateDelta->id == 0)
    {
        /* Reset the map to avoid erroneous ID matches. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            StateDelta->mapEntryID, StateDelta->mapEntryIDSize
            ));

        /* Increment the main ID to avoid matches after reset. */
        StateDelta->id += 1;
    }

    /* Reset the vertex element count. */
    StateDelta->elementCount = 0;

    /* Reset the record count. */
    StateDelta->recordCount = 0;

    /* Success. */
    return gcvSTATUS_OK;
}

static gceSTATUS _MergeDelta(
    IN gcsSTATE_DELTA_PTR StateDelta
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSTATE_DELTA_PTR prevDelta;
    gcsSTATE_DELTA_RECORD_PTR record;
    gctUINT i, count;

    /* Get the record count. */
    count = StateDelta->recordCount;

    /* Set the first record. */
    record = StateDelta->recordArray;

    /* Get the previous delta. */
    prevDelta = StateDelta->prev;

    /* Go through all records. */
    for (i = 0; i < count; i += 1)
    {
        /* Update the delta. */
        gcmONERROR(gcoHARDWARE_UpdateDelta(
            prevDelta, gcvFALSE, record->address, record->mask, record->data
            ));

        /* Advance to the next state. */
        record += 1;
    }

    /* Update the element count. */
    if (StateDelta->elementCount != 0)
    {
        prevDelta->elementCount = StateDelta->elementCount;
    }

OnError:
    /* Return the status. */
    return status;
}

gceSTATUS gcoHARDWARE_UpdateDelta(
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gctBOOL FixedPoint,
    IN gctUINT32 Address,
    IN gctUINT32 Mask,
    IN gctUINT32 Data
    )
{
    gcsSTATE_DELTA_RECORD_PTR recordArray;
    gcsSTATE_DELTA_RECORD_PTR recordEntry;
    gctUINT32_PTR mapEntryID;
    gctUINT32_PTR mapEntryIndex;
    gctUINT deltaID;

    /* Get the current record array. */
    recordArray = StateDelta->recordArray;

    /* Get shortcuts to the fields. */
    deltaID       = StateDelta->id;
    mapEntryID    = StateDelta->mapEntryID;
    mapEntryIndex = StateDelta->mapEntryIndex;

    /* Has the entry been initialized? */
    if (mapEntryID[Address] != deltaID)
    {
        /* No, initialize the map entry. */
        mapEntryID    [Address] = deltaID;
        mapEntryIndex [Address] = StateDelta->recordCount;

        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Add the state to the list. */
        recordEntry->address = Address;
        recordEntry->mask    = Mask;
        recordEntry->data    = Data;

        /* Update the number of valid records. */
        StateDelta->recordCount += 1;
    }

    /* Regular (not masked) states. */
    else if (Mask == 0)
    {
        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Update the state record. */
        recordEntry->mask = 0;
        recordEntry->data = Data;
    }

    /* Masked states. */
    else
    {
        /* Get the current record. */
        recordEntry = &recordArray[mapEntryIndex[Address]];

        /* Update the state record. */
        recordEntry->mask |=  Mask;
        recordEntry->data &= ~Mask;
        recordEntry->data |= (Data & Mask);
    }

    /* Success. */
    return gcvSTATUS_OK;
}

static gceSTATUS _LoadStates(
    IN gctUINT32 Address,
    IN gctBOOL FixedPoint,
    IN gctSIZE_T Count,
    IN gctUINT32 Mask,
    IN gctPOINTER Data
    )
{
    gceSTATUS status;
    gctUINT32_PTR source;
    gctUINT i;
    gcoHARDWARE hardware;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Address=%x Count=%d Data=0x%x",
                  Address, Count, Data);

    /* Verify the arguments. */
    gcmGETHARDWARE(hardware);
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Cast the pointers. */
    source = (gctUINT32_PTR) Data;

    /* Determine the size of the buffer to reserve. */
    reserveSize = gcmALIGN((1 + Count) * gcmSIZEOF(gctUINT32), 8);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(hardware, reserve, stateDelta, memory, reserveSize);

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, Address, Count );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (FixedPoint) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (Count ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

    for (i = 0; i < Count; i ++)
    {
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, FixedPoint, Address + i,
            *source++
            );
    }

    if ((Count & 1) == 0)
    {
        gcmSETFILLER(
            reserve, memory
            );
    }

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gctUINT32 _SetBitWidth(
    gctUINT32 Value,
    gctINT8 CurWidth,
    gctINT8 NewWidth
    )
{
    gctUINT32 result;
    gctINT8 widthDiff;

    /* Mask source bits. */
    Value &= ((gctUINT64) 1 << CurWidth) - 1;

    /* Init the result. */
    result = Value;

    /* Determine the difference in width. */
    widthDiff = NewWidth - CurWidth;

    /* Until the difference is not zero... */
    while (widthDiff)
    {
        /* New value is thiner then current? */
        if (widthDiff < 0)
        {
            result >>= -widthDiff;
            widthDiff = 0;
        }

        /* Full source replication? */
        else if (widthDiff >= CurWidth)
        {
            result = (CurWidth == 32) ? Value
                                      : ((result << CurWidth) | Value);
            widthDiff -= CurWidth;
        }

        /* Partial source replication. */
        else
        {
            result = (result << widthDiff) | (Value >> (CurWidth - widthDiff));
            widthDiff = 0;
        }
    }

    /* Return result. */
    return result;
}

static gceSTATUS
_ConvertComponent(
    gctUINT8* SrcPixel,
    gctUINT8* TrgPixel,
    gctUINT SrcBit,
    gctUINT TrgBit,
    gcsFORMAT_COMPONENT* SrcComponent,
    gcsFORMAT_COMPONENT* TrgComponent,
    gcsBOUNDARY_PTR SrcBoundary,
    gcsBOUNDARY_PTR TrgBoundary,
    gctUINT32 Default
    )
{
    gctUINT32 srcValue;
    gctUINT8 srcWidth;
    gctUINT8 trgWidth;
    gctUINT32 trgMask;
    gctUINT32 bits;

    /* Exit if target is beyond the boundary. */
    if ((TrgBoundary != gcvNULL) &&
        ((TrgBoundary->x < 0) || (TrgBoundary->x >= TrgBoundary->width) ||
         (TrgBoundary->y < 0) || (TrgBoundary->y >= TrgBoundary->height)))
    {
        return gcvSTATUS_SKIP;
    }

    /* Exit if target component is not present. */
    if (TrgComponent->width == gcvCOMPONENT_NOTPRESENT)
    {
        return gcvSTATUS_SKIP;
    }

    /* Extract target width. */
    trgWidth = TrgComponent->width & gcvCOMPONENT_WIDTHMASK;

    /* Extract the source. */
    if ((SrcComponent == gcvNULL) ||
        (SrcComponent->width == gcvCOMPONENT_NOTPRESENT) ||
        (SrcComponent->width &  gcvCOMPONENT_DONTCARE)   ||
        ((SrcBoundary != gcvNULL) &&
         ((SrcBoundary->x < 0) || (SrcBoundary->x >= SrcBoundary->width) ||
          (SrcBoundary->y < 0) || (SrcBoundary->y >= SrcBoundary->height))))
    {
        srcValue = Default;
        srcWidth = 32;
    }
    else
    {
        /* Extract source width. */
        srcWidth = SrcComponent->width & gcvCOMPONENT_WIDTHMASK;

        /* Compute source position. */
        SrcBit += SrcComponent->start;
        SrcPixel += SrcBit >> 3;
        SrcBit &= 7;

        /* Compute number of bits to read from source. */
        bits = SrcBit + srcWidth;

        /* Read the value. */
        srcValue = SrcPixel[0] >> SrcBit;

        if (bits > 8)
        {
            /* Read up to 16 bits. */
            srcValue |= SrcPixel[1] << (8 - SrcBit);
        }

        if (bits > 16)
        {
            /* Read up to 24 bits. */
            srcValue |= SrcPixel[2] << (16 - SrcBit);
        }

        if (bits > 24)
        {
            /* Read up to 32 bits. */
            srcValue |= SrcPixel[3] << (24 - SrcBit);
        }
    }

    /* Make the source component the same width as the target. */
    srcValue = _SetBitWidth(srcValue, srcWidth, trgWidth);

    /* Compute destination position. */
    TrgBit += TrgComponent->start;
    TrgPixel += TrgBit >> 3;
    TrgBit &= 7;

    /* Determine the target mask. */
    trgMask = (gctUINT32) (((gctUINT64) 1 << trgWidth) - 1);
    trgMask <<= TrgBit;

    /* Align the source value. */
    srcValue <<= TrgBit;

    /* Loop while there are bits to set. */
    while (trgMask != 0)
    {
        /* Set 8 bits of the pixel value. */
        if ((trgMask & 0xFF) == 0xFF)
        {
            /* Set all 8 bits. */
            *TrgPixel = (gctUINT8) srcValue;
        }
        else
        {
            /* Set the required bits. */
            *TrgPixel = (gctUINT8) ((*TrgPixel & ~trgMask) | srcValue);
        }

        /* Next 8 bits. */
        TrgPixel ++;
        trgMask  >>= 8;
        srcValue >>= 8;
    }

    return gcvSTATUS_OK;
}

#ifndef VIVANTE_NO_3D
static gctUINT32 _Average2Colors(
    gctUINT32 Color1,
    gctUINT32 Color2
    )
{
    gctUINT32 byte102 =  Color1 & 0x00FF00FF;
    gctUINT32 byte113 =  (Color1 & 0xFF00FF00) >> 1;

    gctUINT32 byte202 =  Color2 & 0x00FF00FF;
    gctUINT32 byte213 =  (Color2 & 0xFF00FF00) >> 1;

    gctUINT32 sum02 = (byte102 + byte202) >> 1;
    gctUINT32 sum13 = (byte113 + byte213);

    gctUINT32 average
        = (sum02 & 0x00FF00FF)
        | (sum13 & 0xFF00FF00);

    return average;
}

static gctUINT32 _Average4Colors(
    gctUINT32 Color1,
    gctUINT32 Color2,
    gctUINT32 Color3,
    gctUINT32 Color4
    )
{
    gctUINT32 byte102 =  Color1 & 0x00FF00FF;
    gctUINT32 byte113 = (Color1 & 0xFF00FF00) >> 2;

    gctUINT32 byte202 =  Color2 & 0x00FF00FF;
    gctUINT32 byte213 = (Color2 & 0xFF00FF00) >> 2;

    gctUINT32 byte302 =  Color3 & 0x00FF00FF;
    gctUINT32 byte313 = (Color3 & 0xFF00FF00) >> 2;

    gctUINT32 byte402 =  Color4 & 0x00FF00FF;
    gctUINT32 byte413 = (Color4 & 0xFF00FF00) >> 2;

    gctUINT32 sum02 = (byte102 + byte202 + byte302 + byte402) >> 2;
    gctUINT32 sum13 = (byte113 + byte213 + byte313 + byte413);

    gctUINT32 average
        = (sum02 & 0x00FF00FF)
        | (sum13 & 0xFF00FF00);

    return average;
}
#endif /* VIVANTE_NO_3D */

gceSTATUS
_DisableTileStatus(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    /* Determine the size of the buffer to reserve. */
    /* FLUSH_HACK */
    reserveSize = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 3, 8);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    if (Surface->type == gcvSURF_DEPTH)
    {
        /* Flush the depth cache. */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

        /* FLUSH_HACK */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0xFFFF, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0xFFFF) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0xFFFF, 0);gcmENDSTATEBATCH(reserve, memory);};

        /* Disable depth tile status. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));

        /* Make sure auto-disable is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

        /* Make sure compression is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

        /* Make sure hierarchical turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13)));
    }
    else
    {
        /* Flush the color cache. */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) );gcmENDSTATEBATCH(reserve, memory);};

        /* FLUSH_HACK */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0xFFFF, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0xFFFF) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0xFFFF, 0);gcmENDSTATEBATCH(reserve, memory);};

        /* Disable color tile status. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));

        /* Make sure auto-disable is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));

        /* Make sure compression is turned off as well. */
        Hardware->memoryConfig = ((((gctUINT32) (Hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)));
    }

    /* Program memory configuration register. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0595, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0595) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0595, Hardware->memoryConfig );     gcmENDSTATEBATCH(reserve, memory);};

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

     /* Make sure raster won't start reading Z values until tile status is
     ** actually disabled. */
     gcmONERROR(gcoHARDWARE_Semaphore(
         gcvWHERE_RASTER, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE
         ));


OnError:
    return status;
}

/******************************************************************************\
****************************** gcoHARDWARE API code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHARDWARE_Construct
**
**  Construct a new gcoHARDWARE object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoHARDWARE * Hardware
**          Pointer to a variable that will hold the gcoHARDWARE object.
*/
gceSTATUS
gcoHARDWARE_Construct(
    IN gcoHAL Hal,
    OUT gcoHARDWARE * Hardware
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware = gcvNULL;
    gcsHAL_INTERFACE iface;
    gctUINT16 data = 0xff00;
    gctPOINTER pointer;
    gctUINT i;
#ifdef __QNXNTO__
    gctSIZE_T objectSize;
    gctPHYS_ADDR physical;
#endif

    gcmHEADER_ARG("Hal=0x%x", Hal);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);
    gcmDEBUG_VERIFY_ARGUMENT(Hardware != gcvNULL);

    /***************************************************************************
    ** Allocate and reset the gcoHARDWARE object.
    */

    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(struct _gcoHARDWARE),
                              &pointer));
    hardware = pointer;

    /* Reset the object. */
    gcmONERROR(gcoOS_ZeroMemory(hardware, gcmSIZEOF(struct _gcoHARDWARE)));

    /* Initialize the gcoHARDWARE object. */
    hardware->object.type = gcvOBJ_HARDWARE;

    hardware->buffer = gcvNULL;
    hardware->queue  = gcvNULL;

    /* Check if big endian */
    hardware->bigEndian = (*(gctUINT8 *)&data == 0xff);

    /* Don't stall before primitive. */
    hardware->stallPrimitive = gcvFALSE;

#ifndef VIVANTE_NO_3D
    /* Current pipe. */
    hardware->currentPipe = gcvPIPE_2D;

    /* Set default API. */
    hardware->api = gcvAPI_OPENGL;

    hardware->stencilStates.mode = gcvSTENCIL_NONE;
    hardware->stencilEnabled     = gcvFALSE;

    hardware->earlyDepth = gcvFALSE;

    /* Disable anti-alias. */
    hardware->sampleMask   = 0xF;
    hardware->sampleEnable = 0xF;
    hardware->samples.x    = 1;
    hardware->samples.y    = 1;
    hardware->vaa          = gcvVAA_NONE;

    /* Disable dithering. */
    hardware->dither[0] = hardware->dither[1] = ~0U;
    hardware->peDitherDirty = gcvTRUE;

    /***************************************************************************
    ** Determine multi-sampling constants.
    */

    /* Two color buffers with 8-bit coverage. Used for Flash applications.

          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      0 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      1 |   |   |   |   |   | X |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      2 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      3 |   |   |   |   |   |   |   |   |   |   |   |   |   | C |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      4 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      5 |   |   |   | X |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      6 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      7 |   |   |   |   |   |   |   |   |   |   |   | C |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      8 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      9 |   |   |   |   |   |   |   | X |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     10 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     11 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | C |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     12 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     13 |   | X |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     14 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     15 |   |   |   |   |   |   |   |   |   | C |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    */

    hardware->vaa8SampleCoords
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (3) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (7) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (9) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (13) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));


    /* Two color buffers with 16-bit coverage. Used for Flash applications.

          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      0 |   | C |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      1 |   |   |   |   |   | X |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      2 |   |   |   |   |   |   |   |   |   | C |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      3 |   |   |   |   |   |   |   |   |   |   |   |   |   | C |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      4 |   |   |   | C |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      5 |   |   |   |   |   |   |   | X |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      6 |   |   |   |   |   |   |   |   |   |   |   | C |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      7 |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   | C |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      8 |   |   | C |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
      9 |   |   |   |   |   |   | X |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     10 |   |   |   |   |   |   |   |   |   |   | C |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     11 |   |   |   |   |   |   |   |   |   |   |   |   |   |   | C |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     12 | C |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     13 |   |   |   |   | X |   |   |   |   |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     14 |   |   |   |   |   |   |   |   | C |   |   |   |   |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
     15 |   |   |   |   |   |   |   |   |   |   |   |   | C |   |   |   |
        +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    */

    hardware->vaa16SampleCoords
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (7) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (5) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (6) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (9) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (4) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (13) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));


    /* 4x MSAA jitter index. */

#if gcdGRID_QUALITY == 0

    /* Square only. */
    hardware->jitterIndex
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

#elif gcdGRID_QUALITY == 1

    /* No jitter. */
    hardware->jitterIndex = 0;

#else

    /* Rotated diamonds. */
    hardware->jitterIndex
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

#endif


    /* 2x MSAA sample coordinates. */

    hardware->sampleCoords2
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));


    /* 4x MSAA sample coordinates.
    **
    **                        1 1 1 1 1 1                        1 1 1 1 1 1
    **    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  0| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  1| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  2| | | | | | |X| | | | | | | | | |  | | | | | | | | | | |X| | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  3| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  4| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  5| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  6| | | | | | | | | | | | | | |X| |  | | |X| | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  7| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  8| | | | | | | | |o| | | | | | | |  | | | | | | | | |o| | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    **  9| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 10| | |X| | | | | | | | | | | | | |  | | | | | | | | | | | | | | |X| |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 11| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 12| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 13| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 14| | | | | | | | | | |X| | | | | |  | | | | | | |X| | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ** 15| | | | | | | | | | | | | | | | |  | | | | | | | | | | | | | | | | |
    **   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    /* Diamond. */
    hardware->sampleCoords4[0]
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (6) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (14) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (6) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (14) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));

    /* Mirrored diamond. */
    hardware->sampleCoords4[1]
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (6) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (14) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (6) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (14) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));

    /* Square. */
    hardware->sampleCoords4[2]
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (10) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));


    /* Compute centroids. */
    gcmONERROR(gcoHARDWARE_ComputeCentroids(
        hardware,
        1, &hardware->sampleCoords2, &hardware->centroids2
        ));

    gcmONERROR(gcoHARDWARE_ComputeCentroids(
        hardware,
        3, hardware->sampleCoords4, hardware->centroids4
        ));
#else
    hardware->currentPipe = gcvPIPE_2D;
#endif /* VIVANTE_NO_3D */


    /***********************************************************************
    ** Query chip identity.
    */

    iface.command = gcvHAL_QUERY_CHIP_IDENTITY;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    hardware->chipModel              = iface.u.QueryChipIdentity.chipModel;
    hardware->chipRevision           = iface.u.QueryChipIdentity.chipRevision;
    hardware->chipFeatures           = iface.u.QueryChipIdentity.chipFeatures;
    hardware->chipMinorFeatures      = iface.u.QueryChipIdentity.chipMinorFeatures;
    hardware->chipMinorFeatures1     = iface.u.QueryChipIdentity.chipMinorFeatures1;
    hardware->chipMinorFeatures2     = iface.u.QueryChipIdentity.chipMinorFeatures2;
    hardware->chipMinorFeatures3     = iface.u.QueryChipIdentity.chipMinorFeatures3;
    hardware->pixelPipes             = iface.u.QueryChipIdentity.pixelPipes;

#if !defined(VIVANTE_NO_3D)
    /* Multi render target. */
    if (((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))))
    {
        hardware->renderTargets = 4;
    }
    else
    {
        hardware->renderTargets = 1;
    }

    hardware->streamCount            = iface.u.QueryChipIdentity.streamCount;
    hardware->registerMax            = iface.u.QueryChipIdentity.registerMax;
    hardware->threadCount            = iface.u.QueryChipIdentity.threadCount;
    hardware->shaderCoreCount        = iface.u.QueryChipIdentity.shaderCoreCount;
    hardware->vertexCacheSize        = iface.u.QueryChipIdentity.vertexCacheSize;
    hardware->vertexOutputBufferSize = iface.u.QueryChipIdentity.vertexOutputBufferSize;
    hardware->instructionCount       = iface.u.QueryChipIdentity.instructionCount;
    hardware->numConstants           = iface.u.QueryChipIdentity.numConstants;
    hardware->bufferSize             = iface.u.QueryChipIdentity.bufferSize;

    /* Determine whether we have unified constant space. */
    hardware->unifiedConst = (hardware->numConstants > 256);

    /* Determine shader parameters. */
    /* This is just a rough scetch, to be revisited. */
    /* TODO. */
    if (hardware->chipModel < gcv2000)
    {
        hardware->vsConstBase  = 0x1400;
        hardware->psConstBase  = 0x1C00;
        hardware->vsConstMax   = 168;
        hardware->psConstMax   = 64;
        hardware->constMax     = 168 + 64;

        hardware->unifiedShader = gcvFALSE;
        hardware->vsInstBase    = 0x1000;
        hardware->psInstBase    = 0x1800;
        hardware->vsInstMax     = 256;
        hardware->psInstMax     = 256;
        hardware->instMax       = 256 + 256;
    }
    else if (hardware->chipModel < gcv4000)
    {
        hardware->vsConstBase  = 0x1400;
        hardware->psConstBase  = 0x1C00;
        hardware->vsConstMax   = 168;
        hardware->psConstMax   = 64;
        hardware->constMax     = 168 + 64;

        hardware->unifiedShader = gcvTRUE;
        hardware->vsInstBase    = 0x3000;
        hardware->psInstBase    = 0x2000;
        hardware->vsInstMax     = 1024;
        hardware->psInstMax     = 1024;
        hardware->instMax       = 1024;
    }
    else
    {
        if (hardware->unifiedConst)
        {
            hardware->vsConstBase  = 0xC000;
            hardware->psConstBase  = 0xC000;
            hardware->vsConstMax   =
            hardware->psConstMax   =
            hardware->constMax     = 2304 / 4;
        }
        else
        {
            hardware->vsConstBase  = 0x1400;
            hardware->psConstBase  = 0x1C00;
            hardware->vsConstMax   = 256;
            hardware->psConstMax   = 256;
            hardware->constMax     = 256 + 256;
        }

        hardware->unifiedShader = gcvTRUE;
        hardware->vsInstBase    = 0x8000;
        hardware->psInstBase    = 0x8000;
        hardware->vsInstMax     =
        hardware->psInstMax     =
        hardware->instMax       = hardware->instructionCount;
    }

    /* Compute alignment. */
    hardware->resolveAlignmentX = 16;
    hardware->resolveAlignmentY = (hardware->pixelPipes > 1) ? 8 : 4;
#endif

    /***************************************************************************
    ** Allocate the gckCONTEXT object.
    */

    iface.command = gcvHAL_ATTACH;

    gcmONERROR(gcoOS_DeviceControl(
        gcvNULL,
        IOCTL_GCHAL_INTERFACE,
        &iface, gcmSIZEOF(iface),
        &iface, gcmSIZEOF(iface)
        ));

    gcmONERROR(iface.status);

    /* Store the allocated context buffer object. */
    hardware->context = iface.u.Attach.context;
    gcmASSERT(hardware->context != gcvNULL);

    /* Store the number of states in the context. */
    hardware->stateCount = iface.u.Attach.stateCount;


    /**************************************************************************/
    /* Allocate the context and state delta buffers. **************************/

    for (i = 0; i < gcdCONTEXT_BUFFER_COUNT + 1; i += 1)
    {
        /* Allocate a state delta. */
        gcsSTATE_DELTA_PTR delta;
        gctUINT bytes;

        /* Allocate the state delta structure. */
#ifdef __QNXNTO__
        objectSize = gcmSIZEOF(gcsSTATE_DELTA);
        gcmONERROR(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &objectSize, &physical, (gctPOINTER *) &delta
            ));
#else
        gcmONERROR(gcoOS_Allocate(
            gcvNULL, gcmSIZEOF(gcsSTATE_DELTA), (gctPOINTER *) &delta
            ));
#endif

        /* Reset the context buffer structure. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(delta, gcmSIZEOF(gcsSTATE_DELTA)));

        /* Append to the list. */
        if (hardware->delta == gcvNULL)
        {
            delta->prev     = delta;
            delta->next     = delta;
            hardware->delta = delta;
        }
        else
        {
            delta->next = hardware->delta;
            delta->prev = hardware->delta->prev;

            hardware->delta->prev->next = delta;
            hardware->delta->prev       = delta;
        }

        /* Set the number of delta in the order of creation. */
#if gcmIS_DEBUG(gcdDEBUG_CODE)
        delta->num = i;
#endif
        if (hardware->stateCount > 0)
        {
            /* Allocate state record array. */
#ifdef __QNXNTO__
            objectSize = gcmSIZEOF(gcsSTATE_DELTA_RECORD) * hardware->stateCount;
            gcmONERROR(gcoOS_AllocateNonPagedMemory(
                gcvNULL, gcvTRUE, &objectSize, &physical,
                (gctPOINTER *) &delta->recordArray
                ));
#else
            gcmONERROR(gcoOS_Allocate(
                gcvNULL,
                gcmSIZEOF(gcsSTATE_DELTA_RECORD) * hardware->stateCount,
                (gctPOINTER *) &delta->recordArray
                ));
#endif

            /* Compute UINT array size. */
            bytes = gcmSIZEOF(gctUINT) * hardware->stateCount;

            /* Allocate map ID array. */
#ifdef __QNXNTO__
            objectSize = bytes;
            gcmONERROR(gcoOS_AllocateNonPagedMemory(
                gcvNULL, gcvTRUE, &objectSize, &physical,
                (gctPOINTER *) &delta->mapEntryID
                ));
#else
            gcmONERROR(gcoOS_Allocate(
                gcvNULL, bytes, (gctPOINTER *) &delta->mapEntryID
                ));
#endif

            /* Set the map ID size. */
            delta->mapEntryIDSize = bytes;

            /* Reset the record map. */
            gcmONERROR(gcoOS_ZeroMemory(delta->mapEntryID, bytes));

            /* Allocate map index array. */
#ifdef __QNXNTO__
            objectSize = bytes;
            gcmONERROR(gcoOS_AllocateNonPagedMemory(
                gcvNULL, gcvTRUE, &objectSize, &physical,
                (gctPOINTER *) &delta->mapEntryIndex
                ));
#else
            gcmONERROR(gcoOS_Allocate(
                gcvNULL, bytes, (gctPOINTER *) &delta->mapEntryIndex
                ));
#endif
        }
    }

    /* Reset the current state delta. */
    _ResetDelta(hardware->delta);


    /***********************************************************************
    ** Construct the command buffer and event queue.
    */

   gcmONERROR(gcoBUFFER_Construct(
        Hal,
        hardware,
        hardware->context,
        gcdCMD_BUFFER_SIZE,
        &hardware->buffer
        ));

    gcmONERROR(gcoQUEUE_Construct(gcvNULL, &hardware->queue));

    for (i = 0; i < gcdTEMP_SURFACE_NUMBER; i += 1)
    {
        hardware->temp2DSurf[i] = gcvNULL;
    }

    /***********************************************************************
    ** Initialize filter blit states.
    */
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_UNIFIED].type  = gcvFILTER_SYNC;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_UNIFIED].kernelSize  = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_UNIFIED].scaleFactor = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_UNIFIED].kernelAddress = 0x01800;

    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_VERTICAL].type  = gcvFILTER_SYNC;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_VERTICAL].kernelSize  = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_VERTICAL].scaleFactor = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_VERTICAL].kernelAddress = 0x02A00;

    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_HORIZONTAL].type  = gcvFILTER_SYNC;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_HORIZONTAL].kernelSize  = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_HORIZONTAL].scaleFactor = 0;
    hardware->loadedKernel[gcvFILTER_BLIT_KERNEL_HORIZONTAL].kernelAddress = 0x02800;

    /***********************************************************************
    ** Reset the temporary surface.
    */

    gcmONERROR(gcoOS_ZeroMemory(
        &hardware->tempBuffer, sizeof(hardware->tempBuffer)
        ));

    hardware->tempBuffer.node.pool = gcvPOOL_UNKNOWN;

    /***********************************************************************
    ** Determine available features.
    */

    /* Determine whether 2D hardware is present. */
    hardware->hw2DEngine = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))));

    /* Don't force software by default. */
    hardware->sw2DEngine = gcvFALSE;

#ifndef VIVANTE_NO_3D
    /* Determine whether 3D hardware is present. */
    hardware->hw3DEngine = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));
#else
    hardware->hw3DEngine = gcvFALSE;
#endif

    /* Determine whether PE 2.0 is present. */
    hardware->hw2DPE20 = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));

    /* Determine whether byte write feature is present in the chip. */
    hardware->byteWrite = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));

    /* Determine whether full rotation is present in the chip. */
    if (hardware->chipRevision < 0x4310)
    {
        hardware->fullBitBlitRotation    = gcvFALSE;
        hardware->fullFilterBlitRotation = gcvFALSE;
    }
    else if (hardware->chipRevision == 0x4310)
    {
        hardware->fullBitBlitRotation    = gcvTRUE;
        hardware->fullFilterBlitRotation = gcvFALSE;
    }
    else if (hardware->chipRevision >= 0x4400)
    {
        hardware->fullBitBlitRotation    = gcvTRUE;
        hardware->fullFilterBlitRotation = gcvTRUE;
    }

    /* MASK register is missing on 4.3.1_rc0. */
    if (hardware->chipRevision == 0x4310)
    {
        hardware->shadowRotAngleReg = gcvTRUE;
        hardware->rotAngleRegShadow = 0x00000000;
    }
    else
    {
        hardware->shadowRotAngleReg = gcvFALSE;
    }

    hardware->dither2DandAlphablendFilter = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));

    hardware->mirrorExtension = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 18:18) & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))));

    /* Determine whether we support full DFB features. */
    hardware->hw2DFullDFB = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));

    /* Determine whether we support one pass filter. */
    hardware->hw2DOPF = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 17:17) & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))));

    /* Determine whether we support multi-source blit. */
    hardware->hw2DMultiSrcBlit = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))));

    hardware->hw2DNewFeature0 = ((((gctUINT32) (hardware->chipMinorFeatures3)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));

    if (hardware->hw2DNewFeature0)
    {
        hardware->hw2DMultiSrcBlit = gcvTRUE;
    }

    hardware->hw2D420L2Cache = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 26:26) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))));

    hardware->hw2DNoIndex8_Brush = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 28:28) & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1)))))));

#ifndef VIVANTE_NO_3D
    /* Determine whether threadwalker is in PS for OpenCL. */
    hardware->threadWalkerInPS = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 18:18) & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))));

    /* Determine whether composition engine is supported. */
    hardware->hwComposition = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));

    /* Initialize composition shader parameters. */
    if (hardware->hwComposition)
    {
        gcmONERROR(gcoHARDWARE_InitializeComposition(hardware));
    }

    /* Initialize variables for bandwidth optimization. */
    hardware->colorStates.colorWrite       = 0xF;
    hardware->colorStates.colorCompression = gcvFALSE;
    hardware->colorStates.destinationRead  = ~0U;
    hardware->colorStates.rop              = 0xC;

    /* Determine striping. */
    if ((hardware->chipModel >= gcv860)
    &&  !((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 4:4) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))))
    )
    {
        /* Get number of cache lines. */
        hardware->needStriping = hardware->hw2DEngine ? 32 : 16;
    }
    else
    {
        /* No need for striping. */
        hardware->needStriping = 0;
    }

    /* Initialize virtual stream state. */
    if ((hardware->streamCount == 1)
    ||  ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 25:25) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))))
    )
    {
        hardware->mixedStreams = gcvTRUE;
    }
    else
    {
        hardware->mixedStreams = gcvFALSE;
    }
#endif /* VIVANTE_NO_3D */

    if ((hardware->chipModel == gcv320) && (hardware->chipRevision == 0x5007))
    {
        hardware->hw2DAppendCacheFlush = gcvTRUE;
    }
    else
    {
        hardware->hw2DAppendCacheFlush = gcvFALSE;
    }

    if (hardware->chipModel == gcv420)
    {
        hardware->hw2DWalkerVersion = 1;
    }
    else
    {
        hardware->hw2DWalkerVersion = 0;
    }

	hardware->hw2DCacheFlushSurf = gcvNULL;

    gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &hardware->stallSignal));

    gcmTRACE_ZONE(
        gcvLEVEL_INFO, gcvZONE_SIGNAL,
        "%s(%d): stall created signal 0x%08X\n",
        __FUNCTION__, __LINE__,
        hardware->stallSignal);

    /* Return pointer to the gcoHARDWARE object. */
    *Hardware = hardware;

    /* Success. */
    gcmFOOTER_ARG("*Hardware=0x%x", *Hardware);
    return gcvSTATUS_OK;

OnError:
    if (hardware != gcvNULL)
    {
        gcmVERIFY_OK(gcoHARDWARE_Destroy(hardware));
    }

    /* Return pointer to the gcoHARDWARE object. */
    *Hardware = gcvNULL;

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Destroy
**
**  Destroy an gcoHARDWARE object.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoHARDWARE_Destroy(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;
    gcsSTATE_DELTA_PTR deltaHead;
    gctINT i;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->hw2DCacheFlushSurf)
    {
        gcmONERROR(gcoHARDWARE_Put2DTempSurface(
            Hardware->hw2DCacheFlushSurf));

        Hardware->hw2DCacheFlushSurf = gcvNULL;
    }

    /* Destroy temporary surface */
    if (Hardware->tempSurface != gcvNULL)
    {
        gcmONERROR(gcoSURF_Destroy(Hardware->tempSurface));
        Hardware->tempSurface = gcvNULL;
    }

    for (i = 0; i < gcdTEMP_SURFACE_NUMBER; i += 1)
    {
        gcsSURF_INFO_PTR surf = Hardware->temp2DSurf[i];

        if (surf != gcvNULL)
        {
            if (surf->node.valid)
            {
                gcmONERROR(gcoHARDWARE_Unlock(
                    &surf->node, gcvSURF_BITMAP
                    ));
            }

            /* Free the video memory by event. */
            if (surf->node.u.normal.node != gcvNULL)
            {
                gcmONERROR(gcoHARDWARE_ScheduleVideoMemory(
                    &Hardware->temp2DSurf[i]->node
                    ));

                surf->node.u.normal.node = gcvNULL;
            }

            gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Hardware->temp2DSurf[i]));
        }
    }

#if !defined(VIVANTE_NO_3D)
    /* Destroy compositon objects. */
    gcmONERROR(gcoHARDWARE_DestroyComposition(Hardware));
#endif

    /* Destroy the command buffer. */
    if (Hardware->buffer != gcvNULL)
    {
        gcmONERROR(gcoBUFFER_Destroy(Hardware->buffer));
        Hardware->buffer = gcvNULL;
    }

    if (Hardware->queue != gcvNULL)
    {
        /* Commit the event queue. */
        status = gcoQUEUE_Commit(Hardware->queue);

        if (gcmIS_SUCCESS(status))
        {
            gcmONERROR(gcoHARDWARE_Stall());
        }
        else
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): Failed to commit event queue (%d).\n",
                __FUNCTION__, __LINE__, status
                );
        }

        /* Destroy the event queue. */
        gcmONERROR(gcoQUEUE_Destroy(Hardware->queue));
        Hardware->queue = gcvNULL;
    }

    /* Free state deltas. */
    for (deltaHead = Hardware->delta; Hardware->delta != gcvNULL;)
    {
        /* Get a shortcut to the current delta. */
        gcsSTATE_DELTA_PTR delta = Hardware->delta;

        /* Get the next delta. */
        gcsSTATE_DELTA_PTR next = delta->next;

        /* Last item? */
        if (next == deltaHead)
        {
            next = gcvNULL;
        }

        /* Free map index array. */
        if (delta->mapEntryIndex != gcvNULL)
        {
#ifdef __QNXNTO__
            gcmONERROR(gcoOS_FreeNonPagedMemory(
                gcvNULL, delta->mapEntryIDSize, gcvNULL, delta->mapEntryIndex
                ));
            delta->mapEntryIndex = gcvNULL;
#else
            gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, delta->mapEntryIndex));
#endif
        }

        /* Allocate map ID array. */
        if (delta->mapEntryID != gcvNULL)
        {
#ifdef __QNXNTO__
            gcmONERROR(gcoOS_FreeNonPagedMemory(
                gcvNULL, delta->mapEntryIDSize, gcvNULL, delta->mapEntryID
                ));
            delta->mapEntryID = gcvNULL;
#else
            gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, delta->mapEntryID));
#endif
        }

        /* Free state record array. */
        if (delta->recordArray != gcvNULL)
        {
#ifdef __QNXNTO__
            gcmONERROR(gcoOS_FreeNonPagedMemory(
                gcvNULL,
                delta->mapEntryIDSize / gcmSIZEOF(gcsSTATE_DELTA_RECORD) * gcmSIZEOF(gcsSTATE_DELTA_RECORD),
                gcvNULL,
                delta->recordArray
                ));
            delta->recordArray = gcvNULL;
#else
            gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, delta->recordArray));
#endif
        }

        /* Free state delta. */
#ifdef __QNXNTO__
            gcmONERROR(gcoOS_FreeNonPagedMemory(
                gcvNULL, gcmSIZEOF(gcsSTATE_DELTA), gcvNULL, delta
                ));
            delta = gcvNULL;
#else
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, delta));
#endif

        /* Remove from the list. */
        Hardware->delta = next;
    }

    /* Detach the process. */
    if (Hardware->context != gcvNULL)
    {
        gcsHAL_INTERFACE iface;
        iface.command = gcvHAL_DETACH;
        iface.u.Detach.context = Hardware->context;

        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, gcmSIZEOF(iface),
            &iface, gcmSIZEOF(iface)
            ));

        Hardware->context = gcvNULL;
    }

    /* Free temporary buffer allocated by filter blit operation. */
    gcmONERROR(gcoHARDWARE_FreeTemporarySurface(gcvFALSE));

    /* Destroy the stall signal. */
    if (Hardware->stallSignal != gcvNULL)
    {
        gcmONERROR(gcoOS_DestroySignal(gcvNULL, Hardware->stallSignal));
        Hardware->stallSignal = gcvNULL;
    }

    /* Mark the gcoHARDWARE object as unknown. */
    Hardware->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoHARDWARE object. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Hardware));

OnError:
    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_LoadState
**
**  Load a state buffer.
**
**  INPUT:
**
**      gctUINT32 Address
**          Starting register address of the state buffer.
**
**      gctUINT32 Count
**          Number of states in state buffer.
**
**      gctPOINTER Data
**          Pointer to state buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadState(
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Address=%x Count=%d Data=0x%x",
                  Address, Count, Data);

    /* Call generic function. */
    gcmONERROR(_LoadStates(Address >> 2, gcvFALSE, Count, 0, Data));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !defined(VIVANTE_NO_3D)
/*******************************************************************************
**
**  gcoHARDWARE_LoadStateX
**
**  Load a state buffer with fixed point states.  The states are meant for the
**  3D pipe.
**
**  INPUT:
**
**      gctUINT32 Address
**          Starting register address of the state buffer.
**
**      gctUINT32 Count
**          Number of states in state buffer.
**
**      gctPOINTER Data
**          Pointer to state buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadStateX(
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Address=%x Count=%d Data=0x%x",
                  Address, Count, Data);

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Call LoadState function. */
    gcmONERROR(_LoadStates(Address >> 2, gcvTRUE, Count, 0, Data));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gcoHARDWARE_LoadState32
**
**  Load one 32-bit state.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctUINT32 Address
**          Register address of the state.
**
**      gctUINT32 Data
**          Value of the state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Address=%x Data=0x%x",
                  Address, Data);

    /* Call generic function. */
    gcmONERROR(_LoadStates(Address >> 2, gcvFALSE, 1, 0, &Data));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_LoadState32x
**
**  Load one 32-bit state, which is represented in 16.16 fixed point.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gctUINT32 Address
**          Register address of the state.
**
**      gctFIXED_POINT Data
**          Value of the state in 16.16 fixed point format.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadState32x(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctFIXED_POINT Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Address=%x Data=0x%x",
                  Address, Data);

    /* Call generic function. */
    gcmONERROR(_LoadStates(Address >> 2, gcvTRUE, 1, 0, &Data));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_LoadState64
**
**  Load one 64-bit state.
**
**  INPUT:
**
**      gctUINT32 Address
**          Register address of the state.
**
**      gctUINT64 Data
**          Value of the state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadState64(
    IN gctUINT32 Address,
    IN gctUINT64 Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Address=%x Data=0x%x",
                  Address, Data);

    /* Call generic function. */
    gcmONERROR(_LoadStates(Address >> 2, gcvFALSE, 2, 0, &Data));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoHARDWARE_LoadShaders
**
**  Load the vertex and pixel shaders.
**
**  INPUT:
**
**      gctSIZE_T StateBufferSize
**          The number of bytes in the 'StateBuffer'.
**
**      gctPOINTER StateBuffer
**          Pointer to the states that make up the shader program.
**
**      gcsHINT_PTR Hints
**          Pointer to a gcsHINT structure that contains information required
**          when loading the shader states.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_LoadShaders(
    IN gctSIZE_T StateBufferSize,
    IN gctPOINTER StateBuffer,
    IN gcsHINT_PTR Hints
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("StateBufferSize=%d StateBuffer=0x%x Hints=0x%x",
                  StateBufferSize, StateBuffer, Hints);

    gcmGETHARDWARE(hardware);

    /* Store shader states. */
    hardware->shaderStates.stateBufferSize = StateBufferSize;
    hardware->shaderStates.stateBuffer     = (gctUINT32_PTR) StateBuffer;
    hardware->shaderStates.hints           = Hints;

    /* Invalidate the states. */
    hardware->shaderDirty = gcvTRUE;

OnError:
    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gcoHARDWARE_Commit
**
**  Commit the current command buffer to the hardware.
**
**  INPUT:
**
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Commit(
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER();

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Dump the commit. */
    gcmDUMP(gcvNULL, "@[commit]");

    /* Commit command buffer and return status. */
    gcmONERROR(gcoBUFFER_Commit(
        hardware->buffer,
        hardware->currentPipe,
        hardware->delta,
        hardware->queue
        ));

    /* Did the delta become associated? */
    if (hardware->delta->refCount == 0)
    {
        /* No, merge with the previous. */
        _MergeDelta(hardware->delta);
    }
    else
    {
        /* The delta got associated, move to the next one. */
        hardware->delta = hardware->delta->next;
        gcmASSERT(hardware->delta->refCount == 0);
    }

    /* Reset the current. */
    _ResetDelta(hardware->delta);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Stall
**
**  Stall the thread until the hardware is finished.
**
**  INPUT:
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Stall(
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER();

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Dump the stall. */
    gcmDUMP(gcvNULL, "@[stall]");

    /* Create a signal event. */
    iface.command            = gcvHAL_SIGNAL;
    iface.u.Signal.signal    = hardware->stallSignal;
    iface.u.Signal.auxSignal = gcvNULL;
    iface.u.Signal.process   = gcoOS_GetCurrentProcessID();
    iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

    /* Send the event. */
    gcmONERROR(gcoHARDWARE_CallEvent(&iface));

    /* Commit the event queue. */
    gcmONERROR(gcoQUEUE_Commit(hardware->queue));

    /* Wait for the signal. */
    gcmONERROR(
        gcoOS_WaitSignal(gcvNULL, hardware->stallSignal, gcvINFINITE));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  _ConvertResolveFormat
**
**  Converts HAL resolve format into its hardware equivalent.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gceSURF_FORMAT Format
**          HAL format value.
**
**  OUTPUT:
**
**      gctUINT32 * HardwareFormat
**          Hardware format value.
**
**      gctBOOL * Flip
**          RGB component flip flag.
*/
static gceSTATUS _ConvertResolveFormat(
    IN gcoHARDWARE Hardware,
    IN gceSURF_FORMAT Format,
    OUT gctUINT32 * HardwareFormat,
    OUT gctBOOL * Flip
    )
{
    gctUINT32 format;
    gctBOOL flip;

    switch (Format)
    {
    case gcvSURF_X4R4G4B4:
        format = 0x0;
        flip   = gcvFALSE;
        break;

    case gcvSURF_A4R4G4B4:
        format = 0x1;
        flip   = gcvFALSE;
        break;

    case gcvSURF_X1R5G5B5:
        format = 0x2;
        flip   = gcvFALSE;
        break;

    case gcvSURF_A1R5G5B5:
        format = 0x3;
        flip   = gcvFALSE;
        break;

    case gcvSURF_X1B5G5R5:
        format = 0x2;
        flip   = gcvTRUE;
        break;

    case gcvSURF_A1B5G5R5:
        format = 0x3;
        flip   = gcvTRUE;
        break;

    case gcvSURF_R5G6B5:
        format = 0x4;
        flip   = gcvFALSE;
        break;

    case gcvSURF_X8R8G8B8:
        format = 0x5;
        flip   = gcvFALSE;
        break;

    case gcvSURF_A8R8G8B8:
        format = 0x6;
        flip   = gcvFALSE;
        break;

    case gcvSURF_X8B8G8R8:
        format = 0x5;
        flip   = gcvTRUE;
        break;

    case gcvSURF_A8B8G8R8:
        format = 0x6;
        flip   = gcvTRUE;
        break;

    case gcvSURF_YUY2:
        if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))))
        {
            format = 0x7;
            flip   = gcvFALSE;
            break;
        }
        return gcvSTATUS_INVALID_ARGUMENT;

    /* Fake 16-bit formats. */
    case gcvSURF_D16:
    case gcvSURF_UYVY:
        format = 0x1;
        flip   = gcvFALSE;
        break;

    /* Fake 32-bit formats. */
    case gcvSURF_D24S8:
    case gcvSURF_D24X8:
    case gcvSURF_D32:
        format = 0x6;
        flip   = gcvFALSE;
        break;

    default:
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    *HardwareFormat = format;

    if (Flip != gcvNULL)
    {
        *Flip = flip;
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  _GetAlignmentX
**
**  Get the alignment for X coordinate.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR SurfInfo
**          Surface info.
**
**  OUTPUT:
**
**      gctUINT32 AlignmentX
*/
static void _GetAlignmentX(
    IN gcsSURF_INFO_PTR SurfInfo,
    OUT gctUINT32_PTR AlignmentX
    )
{
    if (SurfInfo->superTiled)
    {
        *AlignmentX = 64;
    }
    else if (SurfInfo->node.pool == gcvPOOL_VIRTUAL)
    {
        if (SurfInfo->is16Bit)
        {
            *AlignmentX = 32;
        }
        else
        {
            *AlignmentX = 16;
        }
    }
    else
    {
        *AlignmentX = 4;
    }

    return;
}

/*******************************************************************************
**
**  _AlignResolveRect
**
**  Align the specified rectangle to the resolve block requirements.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR SurfInfo
**          Surface info.
**
**      gcsPOINT_PTR RectOrigin
**          Unaligned origin of the rectangle.
**
**      gcsPOINT_PTR RectSize
**          Unaligned size of the rectangle.
**
**  OUTPUT:
**
**      gcsPOINT_PTR AlignedOrigin
**          Resolve-aligned origin of the rectangle.
**
**      gcsPOINT_PTR AlignedSize
**          Resolve-aligned size of the rectangle.
*/
static void _AlignResolveRect(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SurfInfo,
    IN gcsPOINT_PTR RectOrigin,
    IN gcsPOINT_PTR RectSize,
    OUT gcsPOINT_PTR AlignedOrigin,
    OUT gcsPOINT_PTR AlignedSize
    )
{
    gctUINT32 maskX;
    gctUINT32 maskY;

    /* Determine region's right and bottom coordinates. */
    gctINT32 right  = RectOrigin->x + RectSize->x;
    gctINT32 bottom = RectOrigin->y + RectSize->y;

    /* Determine the outside or "external" coordinates aligned for resolve
       to completely cover the requested rectangle. */
    _GetAlignmentX(SurfInfo, &maskX);
    maskX -= 1;

    maskY = (SurfInfo->superTiled ? 64 : 4) * Hardware->pixelPipes - 1;

    AlignedOrigin->x = RectOrigin->x & ~maskX;
    AlignedOrigin->y = RectOrigin->y & ~maskY;

    AlignedSize->x   = gcmALIGN(right  - AlignedOrigin->x, 16);
    AlignedSize->y   = gcmALIGN(bottom - AlignedOrigin->y, 4 * Hardware->pixelPipes);
}

/*******************************************************************************
**
**  _ComputePixelLocation
**
**  Compute the offset of the specified pixel and determine its format.
**
**  INPUT:
**
**      gctUINT X, Y
**          Pixel coordinates.
**
**      gctUINT Stride
**          Surface stride.
**
**      gcsSURF_FORMAT_INFO_PTR * Format
**          Pointer to the pixel format (even/odd pair).
**
**      gctBOOL Tiled
**          Surface tiled vs. linear flag.
**
**      gctBOOL SuperTiled
**          Surface super tiled vs. normal tiled flag.
**
**  OUTPUT:
**
**      gctUINT32 PixelOffset
**          Offset of the pixel from the beginning of the surface.
**
**      gcsSURF_FORMAT_INFO_PTR * PixelFormat
**          Specific pixel format of this pixel.
*/
static void _ComputePixelLocation(
    IN gctUINT X,
    IN gctUINT Y,
    IN gctUINT Stride,
    IN gcsSURF_FORMAT_INFO_PTR * Format,
    IN gctBOOL Tiled,
    IN gctBOOL SuperTiled,
    OUT gctUINT32_PTR PixelOffset,
    OUT gcsSURF_FORMAT_INFO_PTR * PixelFormat
    )
{
    gctUINT8 bitsPerPixel = Format[0]->bitsPerPixel;

    if (Format[0]->interleaved)
    {
        /* Determine whether the pixel is at even or odd location. */
        gctUINT oddPixel = X & 1;

        /* Force to the even location for interleaved pixels. */
        X &= ~1;

        /* Pick the proper format. */
        *PixelFormat = Format[oddPixel];
    }
    else
    {
        *PixelFormat = Format[0];
    }

    if (Tiled)
    {
        gctUINT xt;
        gctUINT yt;

        if (SuperTiled)
        {
            xt = ((X &  0x03) << 0) |
                 ((Y &  0x03) << 2) |
                 ((X &  0x04) << 2) |
                 ((Y &  0x0C) << 3) |
                 ((X &  0x38) << 4) |
                 ((Y &  0x30) << 6) |
                 ((X & ~0x3F) << 6);

            yt = Y & ~0x3F;
        }
        else
        {
            /* 4x4 tiled. */
            xt = ((X &  0x03) << 0) +
                 ((Y &  0x03) << 2) +
                 ((X & ~0x03) << 2);

            yt = Y & ~3;
        }

        *PixelOffset
            = yt * Stride
            + xt * bitsPerPixel / 8;
    }
    else
    {
        *PixelOffset
            = Y * Stride
            + X * bitsPerPixel / 8;
    }
}

/*******************************************************************************
**
**  _BitBlitCopy
**
**  Make a copy of the specified rectangular area using 2D hardware.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be copied.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be copied.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be copied.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_BitBlitCopy(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctUINT32 format, swizzle, isYUVformat;
    gctINT destRight, destBottom;

    gctSIZE_T reserveSize;
    gcoCMDBUF reserve;

    gcmHEADER_ARG("Hardware=0x%x SrcInfo=0x%x DestInfo=0x%x SrcOrigin=%d,%d "
                  "DestOrigin=%d,%d RectSize=%d,%d",
                  Hardware, SrcInfo, DestInfo, SrcOrigin->x, SrcOrigin->y,
                  DestOrigin->x, DestOrigin->y, RectSize->x, RectSize->y);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Verify that the surfaces are locked. */
    gcmVERIFY_LOCK(SrcInfo);
    gcmVERIFY_LOCK(DestInfo);

    /* Determine the size of the buffer to reserve. */
    reserveSize
        /* Switch to 2D pipes. */
        = gcmALIGN(8 * gcmSIZEOF(gctUINT32), 8)

        /* Source setup. */
        + gcmALIGN((1 + 6) * gcmSIZEOF(gctUINT32), 8)

        /* Destination setup. */
        + gcmALIGN((1 + 4) * gcmSIZEOF(gctUINT32), 8)

        /* Clipping window setup. */
        + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

        /* ROP setup. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

        /* Rotation setup. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

        /* Blit commands. */
        + 4 * gcmSIZEOF(gctUINT32)

        /* Flush. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

#ifndef VIVANTE_NO_3D
        /* Switch to 3D pipes. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)
#endif
        ;

    Hardware->hw2DCmdBuffer = gcvNULL;
    Hardware->hw2DCmdIndex = 0;
    Hardware->hw2DCmdSize = reserveSize >> 2;

    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->buffer,
        reserveSize,
        gcvTRUE,
        &reserve
        ));

    Hardware->hw2DCmdBuffer = reserve->lastReserve;

    /* Flush the 3D pipe. */
    gcmONERROR(gcoHARDWARE_Load2DState32(
        Hardware,
        0x0380C,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
        ));

    gcmONERROR(gcoHARDWARE_Load2DState32(
        Hardware,
        0x03808,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        ));

    Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
    Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));

    gcmONERROR(gcoHARDWARE_Load2DState32(
        Hardware,
        0x03800,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (gcvPIPE_2D) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
        ));
    {
        gctUINT32 data[6];

        /***************************************************************************
        ** Setup source.
        */

        /* Convert source format. */
        gcmONERROR(gcoHARDWARE_TranslateSourceFormat(
            Hardware, SrcInfo->format, &format, &swizzle, &isYUVformat
            ));

        data[0] = SrcInfo->node.physical;

        data[1] = SrcInfo->stride;

        data[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

        data[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:24) - (0 ? 28:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:24) - (0 ? 28:24) + 1))))))) << (0 ? 28:24))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 28:24) - (0 ? 28:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:24) - (0 ? 28:24) + 1))))))) << (0 ? 28:24)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

        data[4] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (SrcOrigin->x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (SrcOrigin->y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

        data[5] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (RectSize->x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize->y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

        gcmONERROR(gcoHARDWARE_Load2DState(
            Hardware,
            0x01200,
            6,
            data
            ));

        /***************************************************************************
        ** Setup destination.
        */
        /* Convert destination format. */
        gcmONERROR(gcoHARDWARE_TranslateDestinationFormat(
            DestInfo->format, &format, &swizzle, &isYUVformat
            ));

        data[0] = DestInfo->node.physical;

        data[1] = DestInfo->stride;

        data[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

        data[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)));

        gcmONERROR(gcoHARDWARE_Load2DState(
            Hardware,
            0x01228,
            4,
            data
            ));

        /***************************************************************************
        ** Setup clipping window.
        */
        destRight  = DestOrigin->x + RectSize->x;
        destBottom = DestOrigin->y + RectSize->y;

        data[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0))) | (((gctUINT32) ((gctUINT32) (DestOrigin->x) & ((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16))) | (((gctUINT32) ((gctUINT32) (DestOrigin->y) & ((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)));

        data[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0))) | (((gctUINT32) ((gctUINT32) (destRight) & ((gctUINT32) ((((1 ? 14:0) - (0 ? 14:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:0) - (0 ? 14:0) + 1))))))) << (0 ? 14:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16))) | (((gctUINT32) ((gctUINT32) (destBottom) & ((gctUINT32) ((((1 ? 30:16) - (0 ? 30:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:16) - (0 ? 30:16) + 1))))))) << (0 ? 30:16)));

        gcmONERROR(gcoHARDWARE_Load2DState(
            Hardware,
            0x01260,
            2,
            data
            ));

        /***************************************************************************
        ** Blit the data.
        */

        /* Set ROP. */
        gcmONERROR(gcoHARDWARE_Load2DState32(
            Hardware,
            0x0125C,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (0xCC) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)))
            ));

        /* Set Rotation.*/
        if (Hardware->mirrorExtension)
        {
            gcmONERROR(gcoHARDWARE_Load2DState32(
                Hardware,
                0x012BC,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
                ));
        }
        else
        {
            gcmONERROR(gcoHARDWARE_Load2DState32(
                Hardware,
                0x0126C,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                ));
        }

        /* START_DE. */
        Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] =
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x04 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:16) - (0 ? 26:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:16) - (0 ? 26:16) + 1))))))) << (0 ? 26:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 26:16) - (0 ? 26:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:16) - (0 ? 26:16) + 1))))))) << (0 ? 26:16)));
        Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] = 0xDEADDEED;

        /* DestRectangle. */
        Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (DestOrigin->x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (DestOrigin->y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

        Hardware->hw2DCmdBuffer[Hardware->hw2DCmdIndex++] =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (destRight) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (destBottom) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));
    }

    gcmDUMP(gcvNULL, "@[prim2d 1 0x00000000");
    gcmDUMP(gcvNULL,
            "  %d,%d %d,%d",
            DestOrigin->x, DestOrigin->y, destRight, destBottom);
    gcmDUMP(gcvNULL, "] -- prim2d");

    /* Flush the 2D cache. */
    gcmONERROR(gcoHARDWARE_Load2DState32(
        Hardware,
        0x0380C,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))));

#ifndef VIVANTE_NO_3D
    /* Select the 3D pipe. */
    gcmONERROR(gcoHARDWARE_Load2DState32(
        Hardware,
        0x03800,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (gcvPIPE_3D) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
        ));
#endif

    gcmASSERT(Hardware->hw2DCmdSize == Hardware->hw2DCmdIndex);


    /* Commit the command buffer. */
    gcmONERROR(gcoHARDWARE_Commit());

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  _SoftwareCopy
**
**  Make a copy of the specified rectangular area using CPU.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be copied.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be copied.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be copied.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS _SoftwareCopy(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x SrcInfo=0x%x DestInfo=0x%x SrcOrigin=%d,%d "
                  "DestOrigin=%d,%d RectSize=%d,%d",
                  Hardware, SrcInfo, DestInfo, SrcOrigin->x, SrcOrigin->y,
                  DestOrigin->x, DestOrigin->y, RectSize->x, RectSize->y);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(RectSize->x > 0);
    gcmDEBUG_VERIFY_ARGUMENT(RectSize->y > 0);

    do
    {
        gctBOOL srcTiled, dstTiled;
        gcsSURF_FORMAT_INFO_PTR srcFormatInfo[2];
        gcsSURF_FORMAT_INFO_PTR dstFormatInfo[2];
        gctUINT srcX, srcY;
        gctUINT dstX, dstY;
        gctUINT srcRight, srcBottom;
        gctUINT32 srcOffset, dstOffset;
        gcsSURF_FORMAT_INFO_PTR srcFormat, dstFormat;

        /* Verify that the surfaces are locked. */
        gcmVERIFY_LOCK(SrcInfo);
        gcmVERIFY_LOCK(DestInfo);

        /* Flush and stall the pipe. */
        gcmERR_BREAK(gcoHARDWARE_FlushPipe());
        gcmERR_BREAK(gcoHARDWARE_Commit());
        gcmERR_BREAK(gcoHARDWARE_Stall());

        /* Query format specifics. */
        gcmERR_BREAK(gcoSURF_QueryFormat(SrcInfo->format, srcFormatInfo));
        gcmERR_BREAK(gcoSURF_QueryFormat(DestInfo->format, dstFormatInfo));

        /* Determine whether the destination is tiled. */
        srcTiled = (SrcInfo->type  != gcvSURF_BITMAP);
        dstTiled = (DestInfo->type != gcvSURF_BITMAP);

        /* Test for fast copy. */
        if (srcTiled
        &&  dstTiled
        &&  (SrcInfo->format == DestInfo->format)
        &&  (SrcOrigin->x == 0)
        &&  (SrcOrigin->y == 0)
        &&  (RectSize->x == (gctINT) DestInfo->alignedWidth)
        &&  (RectSize->y == (gctINT) DestInfo->alignedHeight)
        )
        {
            gctUINT32 sourceOffset = 0;
            gctUINT32 targetOffset = 0;
            gctINT y;

            for (y = 0; y < RectSize->y; y += 4)
            {
                gcoOS_MemCopy(DestInfo->node.logical + targetOffset,
                              SrcInfo->node.logical  + sourceOffset,
                              DestInfo->stride * 4);

                sourceOffset += SrcInfo->stride  * 4;
                targetOffset += DestInfo->stride * 4;
            }

            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Set initial coordinates. */
        srcX = SrcOrigin->x;
        srcY = SrcOrigin->y;
        dstX = DestOrigin->x;
        dstY = DestOrigin->y;

        /* Compute limits. */
        srcRight  = SrcOrigin->x + RectSize->x - 1;
        srcBottom = SrcOrigin->y + RectSize->y - 1;

        /* Loop through the rectangle. */
        while (gcvTRUE)
        {
            gctUINT8_PTR srcPixel, dstPixel;

            _ComputePixelLocation(
                srcX, srcY, SrcInfo->stride,
                srcFormatInfo, srcTiled, SrcInfo->superTiled,
                &srcOffset, &srcFormat
                );

            _ComputePixelLocation(
                dstX, dstY, DestInfo->stride,
                dstFormatInfo, dstTiled, DestInfo->superTiled,
                &dstOffset, &dstFormat
                );

            srcPixel = SrcInfo->node.logical  + srcOffset;
            dstPixel = DestInfo->node.logical + dstOffset;

            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                srcPixel,
                dstPixel,
                0, 0,
                srcFormat,
                dstFormat,
                gcvNULL,
                gcvNULL
                ));

            /* End of line? */
            if (srcX == srcRight)
            {
                /* Last row? */
                if (srcY == srcBottom)
                {
                    break;
                }

                /* Reset to the beginning of the line. */
                srcX = SrcOrigin->x;
                dstX = DestOrigin->x;

                /* Advance to the next line. */
                srcY++;
                dstY++;
            }
            else
            {
                /* Advance to the next pixel. */
                srcX++;
                dstX++;
            }
        }
    }
    while (gcvFALSE);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  _SourceCopy
**
**  Make a copy of the specified rectangular area.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be copied.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be copied.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be copied.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS _SourceCopy(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x SrcInfo=0x%x DestInfo=0x%x SrcOrigin=%d,%d "
                  "DestOrigin=%d,%d RectSize=%d,%d",
                  Hardware, SrcInfo, DestInfo, SrcOrigin->x, SrcOrigin->y,
                  DestOrigin->x, DestOrigin->y, RectSize->x, RectSize->y);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Only support BITMAP to BITMAP or TEXTURE to TEXTURE copy for now. */
        if (!(((SrcInfo->type == gcvSURF_BITMAP) && (DestInfo->type == gcvSURF_BITMAP))
        ||    ((SrcInfo->type == gcvSURF_TEXTURE) && (DestInfo->type == gcvSURF_TEXTURE)))
        )
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Is 2D pipe present? */
        if (Hardware->hw2DEngine
        &&  !Hardware->sw2DEngine
        /* GC500 needs properly aligned surfaces. */
        &&  ((Hardware->chipModel != gcv500)
            || ((DestInfo->rect.right & 7) == 0)
            )
        )
        {
            status = _BitBlitCopy(
                Hardware,
                SrcInfo,
                DestInfo,
                SrcOrigin,
                DestOrigin,
                RectSize
                );
        }
        else
        {
            status = _SoftwareCopy(
                Hardware,
                SrcInfo,
                DestInfo,
                SrcOrigin,
                DestOrigin,
                RectSize
                );
        }
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  _Tile420Surface
**
**  Tile linear 4:2:0 source surface.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be copied.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be copied.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be copied.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS _Tile420Surface(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctUINT32 srcFormat;
    gctBOOL tilerAvailable;
    gctUINT32 uvSwizzle;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x SrcInfo=0x%x DestInfo=0x%x SrcOrigin=%d,%d "
                  "DestOrigin=%d,%d RectSize=%d,%d",
                  Hardware, SrcInfo, DestInfo, SrcOrigin->x, SrcOrigin->y,
                  DestOrigin->x, DestOrigin->y, RectSize->x, RectSize->y);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Verify that the surfaces are locked. */
    gcmVERIFY_LOCK(SrcInfo);
    gcmVERIFY_LOCK(DestInfo);

    /* Determine hardware support for 4:2:0 tiler. */
    tilerAvailable = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 13:13) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))));

    /* Available? */
    if (!tilerAvailable)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Input limitations until more support is required. */
    if ((SrcOrigin->x  != 0) || (SrcOrigin->y  != 0) ||
        (DestOrigin->x != 0) || (DestOrigin->y != 0)
        )

    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Append FLUSH to the command buffer. */
    gcmONERROR(gcoHARDWARE_FlushPipe());

    /* Determine the size of the buffer to reserve. */
    reserveSize

        /* Tiler configuration. */
        = gcmALIGN((1 + 10) * gcmSIZEOF(gctUINT32), 8)

        /* Single states. */
        + gcmALIGN((1 + 1)  * gcmSIZEOF(gctUINT32) * 3, 8);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Determine the format. */
    if ((SrcInfo->format == gcvSURF_YV12) ||
        (SrcInfo->format == gcvSURF_I420))
    {
    	/* No need to swizzle as we set the U and V addresses correctly. */
	    uvSwizzle = 0x0;
        srcFormat = 0x0;
    }
    else if (SrcInfo->format == gcvSURF_NV12)
    {
        uvSwizzle = 0x0;
        srcFormat = 0x1;
    }
    else
    {
        uvSwizzle = 0x1;
        srcFormat = 0x1;
    }


    /***************************************************************************
    ** Set tiler configuration.
    */

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x059E, 10 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (10 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x059E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        /* Set tiler configuration. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x059E,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (srcFormat) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) ((gctUINT32) (uvSwizzle) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
            );

        /* Set window size. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x059F,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (RectSize->x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize->y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        /* Set Y plane. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A0,
            SrcInfo->node.physical
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A1,
            SrcInfo->stride
            );

        /* Set U plane. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A2,
            SrcInfo->node.physical2
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A3,
            SrcInfo->uStride
            );

        /* Set V plane. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A4,
            SrcInfo->node.physical3
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A5,
            SrcInfo->vStride
            );

        /* Set destination. */
        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A6,
            DestInfo->node.physical
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x05A7,
            DestInfo->stride
            );

        gcmSETFILLER(
            reserve, memory
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );


    /***************************************************************************
    ** Disable clear.
    */

    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x058F, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x058F, 0 );     gcmENDSTATEBATCH(reserve, memory);};


    /***************************************************************************
    ** Trigger resolve.
    */

    {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0580, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0580, 0xBADABEEB );gcmENDSTATEBATCH(reserve, memory);};


    /***************************************************************************
    ** Disable tiler.
    */

    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x059E, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x059E) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x059E, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );     gcmENDSTATEBATCH(reserve, memory);};


    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Commit the command buffer. */
    gcmONERROR(gcoHARDWARE_Commit());

    /* Return the status. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

typedef struct
{
    gctUINT32 mode;
    gctUINT32 horFactor;
    gctUINT32 verFactor;
}
gcsSUPERENTRY, *gcsSUPERENTRY_PTR;

typedef struct
{
    /* Source information. */
    gcsSURF_FORMAT_INFO_PTR srcFormatInfo[2];
    gceTILING srcTiling;
    gctBOOL srcMultiPipe;

    /* Destination information. */
    gcsSURF_FORMAT_INFO_PTR dstFormatInfo[2];
    gceTILING dstTiling;
    gctBOOL dstMultiPipe;

    /* Resolve information. */
    gctBOOL flipY;
    gcsSUPERENTRY_PTR superSampling;
}
gcsRESOLVE_VARS,
* gcsRESOLVE_VARS_PTR;

static gceSTATUS
_StripeResolve(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize,
    IN gcsRESOLVE_VARS_PTR Vars
    )
{
    gceSTATUS status;
    gcsRECT srcRect, dstRect;
    gctINT32 x, xStep, y, yStep;
    gctINT32 width, height;
    gctUINT32 srcOffset, dstOffset;
    gctINT hShift, vShift;
    gctINT32 dstY;
	gctUINT  loopCountX, loopCountY;
    gctUINT resolveCount;
    gctUINT32 srcAddress = 0, dstAddress = 0;
    gctUINT32 windowSize = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    /* Copy super-sampling factor. */
    hShift = (Vars->superSampling->horFactor == 1) ? 0 : 1;
    vShift = (Vars->superSampling->verFactor == 1) ? 0 : 1;

    /* Compute source bounding box. */
    srcRect.left   = SrcOrigin->x & ~15;
    srcRect.right  = gcmALIGN(SrcOrigin->x + (RectSize->x << hShift), 16);
    srcRect.top    = SrcOrigin->y & ~3;
    srcRect.bottom = gcmALIGN(SrcOrigin->y + (RectSize->y << vShift), 4);

    /* Compute destination bounding box. */
    dstRect.left   = DestOrigin->x & ~15;
    dstRect.right  = gcmALIGN(DestOrigin->x + RectSize->x, 16);
    dstRect.top    = DestOrigin->y & ~3;
    dstRect.bottom = gcmALIGN(DestOrigin->y + RectSize->y, 4);

    /* Count the number of resolves needed. */
    resolveCount = 0;

    for (x = srcRect.left, loopCountX =0; (x < srcRect.right) && (loopCountX < MAX_LOOP_COUNT); x += xStep, loopCountX++)
    {
        /* Compute horizontal step. */

        xStep = (x & 63) ? (x & 31) ? 16 : 32 : 64;
        gcmASSERT((x & ~63) == ((x + xStep - 1) & ~63));
        yStep = 16 * Hardware->needStriping / xStep;

        for (y = srcRect.top, loopCountY=0; (y < srcRect.bottom) && (loopCountY < MAX_LOOP_COUNT); y += yStep, loopCountY++)
        {
            /* Compute vertical step. */
            yStep = 16 * Hardware->needStriping / xStep;
            if ((y & ~63) != ((y + yStep - 1) & ~63))
            {
                yStep = gcmALIGN(y, 64) - y;
            }

            /* Update the number of resolves. */
            resolveCount += 1;
        }
    }

    /* Determine the size of the buffer to reserve. */
    reserveSize = (1 + 1) * 4 * 4 * resolveCount;

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Walk all stripes horizontally. */
    for (x = srcRect.left, loopCountX = 0; (x < srcRect.right) && (loopCountX < MAX_LOOP_COUNT); x += xStep, loopCountX++)
    {
        /* Compute horizontal step. */
        xStep = (x & 63) ? (x & 31) ? 16 : 32 : 64;
        gcmASSERT((x & ~63) == ((x + xStep - 1) & ~63));
        yStep = 16 * Hardware->needStriping / xStep;

        /* Compute width. */
        width = gcmMIN(srcRect.right - x, xStep);

        /* Walk the stripe vertically. */
        for (y = srcRect.top, loopCountY = 0; (y < srcRect.bottom) && (loopCountY < MAX_LOOP_COUNT); y += yStep, loopCountY++)
        {
            /* Compute vertical step. */
            yStep = 16 * Hardware->needStriping / xStep;
            if ((y & ~63) != ((y + yStep - 1) & ~63))
            {
                /* Don't overflow a super tile. */
                yStep = gcmALIGN(y, 64) - y;
            }

            /* Compute height. */
            height = gcmMIN(srcRect.bottom - y, yStep);

            /* Compute destination y. */
            dstY = Vars->flipY ? (dstRect.bottom - (y >> vShift) - height)
                               : (y >> vShift);

            /* Compute offsets. */
            gcmONERROR(
                gcoHARDWARE_ComputeOffset(x, y,
                                          SrcInfo->stride,
                                          Vars->srcFormatInfo[0]->bitsPerPixel / 8,
                                          Vars->srcTiling, &srcOffset));
            gcmONERROR(
                gcoHARDWARE_ComputeOffset(x >> hShift, dstY,
                                          DestInfo->stride,
                                          Vars->dstFormatInfo[0]->bitsPerPixel / 8,
                                          Vars->dstTiling, &dstOffset));

            /* Determine state values. */
            srcAddress = SrcInfo->node.physical + srcOffset;
            dstAddress = DestInfo->node.physical + dstOffset;
            windowSize = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (width) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                       | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (height) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

            /* Resolve one part of the stripe. */
            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0582, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0582) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0582, srcAddress );gcmENDSTATEBATCH(reserve, memory);};

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0584, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0584, dstAddress );gcmENDSTATEBATCH(reserve, memory);};

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0588, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0588, windowSize );gcmENDSTATEBATCH(reserve, memory);};

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0580, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0580, 0xBADABEEB );gcmENDSTATEBATCH(reserve, memory);};
        }
    }

    /* Update the state delta. */
    gcoHARDWARE_UpdateDelta(
        stateDelta, gcvFALSE, 0x0582, 0, srcAddress
        );

    gcoHARDWARE_UpdateDelta(
        stateDelta, gcvFALSE, 0x0584, 0, dstAddress
        );

    gcoHARDWARE_UpdateDelta(
        stateDelta, gcvFALSE, 0x0588, 0, windowSize
        );

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    return status;
}

/*******************************************************************************
**
**  _ResolveRect
**
**  Perform a resolve on a surface.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to an gcoHARDWARE object.
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS _ResolveRect(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
#   define _AA(HorFactor, VerFactor) \
    { \
        AQ_RS_CONFIG_RS_SRC_SUPER_SAMPLE_ENABLE \
            ## HorFactor ## X ## VerFactor, \
        HorFactor, \
        VerFactor \
    }

#   define _VAA \
    { \
        0x1, \
        2, \
        1 \
    }

#   define _NOAA \
    { \
        0x0, \
        1, \
        1 \
    }

#   define _INVALIDAA \
    { \
        ~0U, \
        ~0U, \
        ~0U \
    }

    static gcsSUPERENTRY superSamplingTable[17] =
    {
        /*  SOURCE 1x1                 SOURCE 2x1 */

            /* DEST 1x1  DEST 2x1      DEST 1x1  DEST 2x1 */
            _NOAA, _INVALIDAA, { 0x1, 2, 1}, _NOAA,

            /* DEST 1x2  DEST 2x2      DEST 1x2  DEST 2x2 */
            _INVALIDAA, _INVALIDAA, _INVALIDAA, _INVALIDAA,

        /*  SOURCE 1x2                 SOURCE 2x2 */

            /* DEST 1x1  DEST 2x1      DEST 1x1  DEST 2x1 */
            { 0x2, 1, 2}, _INVALIDAA, { 0x3, 2, 2}, { 0x2, 1, 2},

            /* DEST 1x2  DEST 2x2      DEST 1x2  DEST 2x2 */
            _NOAA, _INVALIDAA, { 0x1, 2, 1}, _NOAA,

            /* VAA */
            _VAA,
    };

    gceSTATUS status;
    gcsRESOLVE_VARS vars;
    gcsPOINT srcOrigin;
    gcsPOINT dstOrigin;
    gcsPOINT srcSize;
    gctUINT32 config;
    gctUINT32 srcAddress, dstAddress;
    gctUINT32 srcAddress2, dstAddress2;
    gctUINT32 srcStride, dstStride;
    gctUINT32 srcOffset, dstOffset;
    gctUINT32 srcFormat, dstFormat;
    gctUINT superSamplingIndex;
    gctBOOL srcFlip;
    gctBOOL dstFlip;
    gctUINT32 vaa, endian;
    gctBOOL needStriping;
    gctUINT32 dither[2];
    gctUINT srcBatchCount = 0;
    gctUINT dstBatchCount = 0;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x SrcInfo=0x%x DestInfo=0x%x SrcOrigin=%d,%d "
                  "DestOrigin=%d,%d RectSize=%d,%d",
                  Hardware, SrcInfo, DestInfo, SrcOrigin->x, SrcOrigin->y,
                  DestOrigin->x, DestOrigin->y, RectSize->x, RectSize->y);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT((SrcOrigin->x & 3) == 0);
    gcmDEBUG_VERIFY_ARGUMENT((SrcOrigin->y & 3) == 0);

    gcmDEBUG_VERIFY_ARGUMENT((DestOrigin->x & 3) == 0);
    gcmDEBUG_VERIFY_ARGUMENT((DestOrigin->y & 3) == 0);

    gcmDEBUG_VERIFY_ARGUMENT((RectSize->x & 15) == 0);
    gcmDEBUG_VERIFY_ARGUMENT((RectSize->y &  3) == 0);

    /* Verify that the surfaces are locked. */
    gcmVERIFY_LOCK(SrcInfo);
    gcmVERIFY_LOCK(DestInfo);

    /* Convert source and destination formats. */
    gcmONERROR(
        _ConvertResolveFormat(Hardware,
                              SrcInfo->format,
                              &srcFormat,
                              &srcFlip));

    gcmONERROR(
        gcoSURF_QueryFormat(SrcInfo->format, vars.srcFormatInfo));

    gcmONERROR(
        _ConvertResolveFormat(Hardware,
                              DestInfo->format,
                              &dstFormat,
                              &dstFlip));


    gcmONERROR(
        gcoSURF_QueryFormat(DestInfo->format, vars.dstFormatInfo));

    /* Determine if y flipping is required. */
    vars.flipY = SrcInfo->orientation != DestInfo->orientation;

    if (vars.flipY
    &&  !((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))))
    )
    {
        DestInfo->orientation = SrcInfo->orientation;
        vars.flipY            = gcvFALSE;
    }

    /* Determine source tiling. */
    vars.srcTiling =

        /* Super tiled? */
        SrcInfo->superTiled ? gcvSUPERTILED

        /* Tiled? */
        : (SrcInfo->type != gcvSURF_BITMAP) ? gcvTILED

        /* Linear. */
        : gcvLINEAR;

    vars.srcMultiPipe =  (Hardware->pixelPipes > 1)
                        && ((SrcInfo->tiling == gcvMULTI_SUPERTILED)
                            || (SrcInfo->tiling == gcvMULTI_TILED));

    /* Determine destination tiling. */
    vars.dstTiling =

        /* Super tiled? */
        DestInfo->superTiled ? gcvSUPERTILED

        /* Tiled? */
        : (DestInfo->type != gcvSURF_BITMAP) ? gcvTILED

        /* Linear. */
        : gcvLINEAR;

    vars.dstMultiPipe =  (Hardware->pixelPipes > 1)
                        && ((DestInfo->tiling == gcvMULTI_SUPERTILED)
                            || (DestInfo->tiling == gcvMULTI_TILED));

    /* Determine vaa value. */
    vaa = SrcInfo->vaa
        ? (srcFormat == 0x06)
          ? 0x2
          : 0x1
        : 0x0;

    /* Determine the supersampling mode. */
    if (SrcInfo->vaa && !DestInfo->vaa)
    {
        superSamplingIndex = 16;
    }
    else
    {
        superSamplingIndex = (SrcInfo->samples.y  << 3)
                           + (DestInfo->samples.y << 2)
                           + (SrcInfo->samples.x  << 1)
                           +  DestInfo->samples.x
                           -  15;

        if (SrcInfo->vaa && DestInfo->vaa)
        {
            srcFormat = 0x06;
            dstFormat = 0x06;
            vaa       = 0x0;
        }
    }

    vars.superSampling = &superSamplingTable[superSamplingIndex];

    /* Supported mode? */
    if (vars.superSampling->mode == ~0U)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "Supersampling mode is not defined for the given configuration:\n"
            );

        gcmTRACE(
            gcvLEVEL_ERROR,
            "  SrcInfo->vaa      = %d\n", SrcInfo->vaa
            );

        gcmTRACE(
            gcvLEVEL_ERROR,
            "  SrcInfo->samples  = %dx%d\n",
            SrcInfo->samples.x, SrcInfo->samples.y
            );

        gcmTRACE(
            gcvLEVEL_ERROR,
            "  DestInfo->vaa     = %d\n",
            DestInfo->vaa
            );

        gcmTRACE(
            gcvLEVEL_ERROR,
            "  DestInfo->samples = %dx%d\n",
            DestInfo->samples.x, DestInfo->samples.y
            );

        gcmTRACE(
            gcvLEVEL_ERROR,
            "superSamplingIndex  = %d\n",
            superSamplingIndex
            );

        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Determine dithering. */
    if (vars.srcFormatInfo[0]->bitsPerPixel > vars.dstFormatInfo[0]->bitsPerPixel)
    {
        dither[0] = Hardware->dither[0];
        dither[1] = Hardware->dither[1];
    }
    else
    {
        dither[0] = dither[1] = ~0U;
    }

    /* Flush the pipe. */
    gcmONERROR(gcoHARDWARE_FlushPipe());

    /* Switch to 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(Hardware, gcvPIPE_3D));

    /* Construct configuration state. */
    config

        /* Configure source. */
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (srcFormat) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (vars.srcTiling != gcvLINEAR) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))

        /* Configure destination. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (dstFormat) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) | (((gctUINT32) ((gctUINT32) (vars.dstTiling != gcvLINEAR) & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)))

        /* Configure flipping. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29))) | (((gctUINT32) ((gctUINT32) ((srcFlip ^ dstFlip)) & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (vars.flipY) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))

        /* Configure supersampling enable. */
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5))) | (((gctUINT32) ((gctUINT32) (vars.superSampling->mode) & ((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5)));

    /* Determine the source stride. */
    srcStride = (vars.srcTiling == gcvLINEAR)

              /* Linear. */
              ? SrcInfo->stride

              /* Tiled. */
              : ((((gctUINT32) (SrcInfo->stride * 4)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (vars.srcTiling == gcvSUPERTILED) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));

    if (vars.srcMultiPipe)
    {
        srcStride = ((((gctUINT32) (srcStride)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)));
    }

    /* Determine the destination stride. */
    dstStride = (vars.dstTiling == gcvLINEAR)

              /* Linear. */
              ? DestInfo->stride

              /* Tiled. */
              : ((((gctUINT32) (DestInfo->stride * 4)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (vars.dstTiling == gcvSUPERTILED) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));

    if (vars.dstMultiPipe)
    {
        dstStride = ((((gctUINT32) (dstStride)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)));
    }

    /* Set endian control */
    endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)));

    if (Hardware->bigEndian &&
        (SrcInfo->type != gcvSURF_TEXTURE) &&
        (DestInfo->type == gcvSURF_BITMAP))
    {
        gctUINT32 destBPP;

        /* Compute bits per pixel. */
        gcmONERROR(gcoHARDWARE_ConvertFormat(
            DestInfo->format, &destBPP, gcvNULL
            ));

        if (destBPP == 16)
        {
            endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) (0x1  & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)));
        }
        else if (destBPP == 32)
        {
            endian = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) (0x2  & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)));
        }
    }

    /* Determine whether resolve striping is needed. */
    needStriping
        =  Hardware->needStriping
        && (((((gctUINT32) (Hardware->memoryConfig)) >> (0 ? 1:1)) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) );

    /* Determine the size of the buffer to reserve. */
    reserveSize

        = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 3, 8)

        + gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 2, 8);

    if (!needStriping)
    {
        if (Hardware->pixelPipes == 2)
        {
            srcBatchCount = vars.srcMultiPipe? 2 : 1;
            dstBatchCount = vars.dstMultiPipe? 2 : 1;

            reserveSize

                += gcmALIGN((1 + srcBatchCount) * gcmSIZEOF(gctUINT32), 8)

                /* Target address states. */
                +  gcmALIGN((1 + dstBatchCount) * gcmSIZEOF(gctUINT32), 8);
        }
        else
        {
            reserveSize

                /* Source and target address states. */
                += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
        }
    }

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Append RESOLVE_CONFIG state. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0581, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0581, config );     gcmENDSTATEBATCH(reserve, memory);};

    /* Set source and destination stride. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0583, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0583) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0583, srcStride );     gcmENDSTATEBATCH(reserve, memory);};

    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0585, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0585) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0585, dstStride );     gcmENDSTATEBATCH(reserve, memory);};

    /* Append RESOLVE_DITHER commands. */
    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x058C, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x058C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x058C,
            dither[0]
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x058C + 1,
            dither[1]
            );

        gcmSETFILLER(
            reserve, memory
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Append RESOLVE_CLEAR_CONTROL state. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x058F, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x058F, 0 );     gcmENDSTATEBATCH(reserve, memory);};

    /* Append new configuration register. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05A8, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05A8) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05A8, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (vaa) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | endian );     gcmENDSTATEBATCH(reserve, memory);};

    if (needStriping)
    {
        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);

        /* Stripe the resolve. */
        gcmONERROR(_StripeResolve(Hardware,
                                  SrcInfo,
                                  DestInfo,
                                  SrcOrigin,
                                  DestOrigin,
                                  RectSize,
                                  &vars));
    }
    else
    {
        /* Determine the origins. */
        srcOrigin.x = SrcOrigin->x  * SrcInfo->samples.x;
        srcOrigin.y = SrcOrigin->y  * SrcInfo->samples.y;
        dstOrigin.x = DestOrigin->x * DestInfo->samples.x;
        dstOrigin.y = DestOrigin->y * DestInfo->samples.y;

        /* Determine the source base address offset. */
        gcmONERROR(
			gcoHARDWARE_ComputeOffset(srcOrigin.x, srcOrigin.y / (vars.srcMultiPipe ? 2 : 1),
                                      SrcInfo->stride,
                                      vars.srcFormatInfo[0]->bitsPerPixel / 8,
                                      vars.srcTiling, &srcOffset));

        /* Determine the destination base address offset. */
        gcmONERROR(
            gcoHARDWARE_ComputeOffset(dstOrigin.x, dstOrigin.y,
                                      DestInfo->stride,
                                      vars.dstFormatInfo[0]->bitsPerPixel / 8,
                                      vars.dstTiling, &dstOffset));

        /* Determine base addresses. */
        srcAddress = SrcInfo->node.physical  + srcOffset;
        dstAddress = DestInfo->node.physical + dstOffset;

        /* Determine source rectangle size. */
        srcSize.x = RectSize->x * vars.superSampling->horFactor;
        srcSize.y = RectSize->y * vars.superSampling->verFactor;

        if (Hardware->pixelPipes == 2)
        {
            gctUINT32 srcBankOffsetBytes = 0;
            gctUINT32 srcTopBufferSize = gcmALIGN(SrcInfo->alignedHeight / 2,
                                         SrcInfo->superTiled ? 64 : 4)
                                       * SrcInfo->stride;

            gctUINT32 dstBankOffsetBytes = 0;
            gctUINT32 dstTopBufferSize = gcmALIGN(DestInfo->alignedHeight / 2,
                                         DestInfo->superTiled ? 64 : 4)
                                       * DestInfo->stride;

#if gcdENABLE_BANK_ALIGNMENT
            gcmONERROR(
                gcoSURF_GetBankOffsetBytes(gcvNULL,
                                         SrcInfo->type,
                                         srcTopBufferSize,
                                         &srcBankOffsetBytes
                                         ));

            gcmONERROR(
                gcoSURF_GetBankOffsetBytes(gcvNULL,
                                         DestInfo->type,
                                         dstTopBufferSize,
                                         &dstBankOffsetBytes
                                         ));

            /* If surface doesn't have enough padding, then don't offset it. */
            if (SrcInfo->size <
                ((SrcInfo->alignedHeight * SrcInfo->stride) + srcBankOffsetBytes))
            {
                srcBankOffsetBytes = 0;
            }

            /* If surface doesn't have enough padding, then don't offset it. */
            if (DestInfo->size <
                ((DestInfo->alignedHeight * DestInfo->stride) + dstBankOffsetBytes))
            {
                dstBankOffsetBytes = 0;
            }
#endif

            srcAddress2
                = srcBankOffsetBytes
                + srcAddress
                + srcTopBufferSize;

            dstAddress2
                = dstBankOffsetBytes
                + dstAddress
                + dstTopBufferSize;


           /* Append RESOLVE_SOURCE addresses. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05B0, srcBatchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (srcBatchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x05B0,
                    srcAddress
                    );

                if (srcBatchCount == 2)
                {
                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x05B0 + 1,
                        srcAddress2
                        );

                    gcmSETFILLER(
                        reserve, memory
                        );
                }

            gcmENDSTATEBATCH(
                reserve, memory
                );

            /* Append RESOLVE_DESTINATION addresses. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05B8, dstBatchCount );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (dstBatchCount ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x05B8,
                    dstAddress
                    );

                if (dstBatchCount == 2)
                {
                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x05B8 + 1,
                        dstAddress2
                        );

                    gcmSETFILLER(
                        reserve, memory
                        );
                }

            gcmENDSTATEBATCH(
                reserve, memory
                );
        }
        else
        {
            /* Append RESOLVE_SOURCE commands. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0582, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0582) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0582, srcAddress );     gcmENDSTATEBATCH(reserve, memory);};

            /* Append RESOLVE_DESTINATION commands. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0584, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0584, dstAddress );     gcmENDSTATEBATCH(reserve, memory);};
        }

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);

        /* Program the resolve window and trigger it. */
        gcmONERROR(gcoHARDWARE_ProgramResolve(Hardware, srcSize));
    }

#ifndef VIVANTE_NO_3D
    /* Turn off tile status if we are getting resolved into. */
    if (!Hardware->inFlush
    &&  ((DestInfo == Hardware->colorStates.surface)
        || (DestInfo == Hardware->depthStates.surface)
        )
    )
    {
        gctBOOL entire = (DestOrigin->x == 0)
                      && (DestOrigin->y == 0)
                      && (RectSize->x   >= DestInfo->rect.right)
                      && (RectSize->y   >= DestInfo->rect.bottom);

        gcmONERROR(gcoHARDWARE_DisableTileStatus(DestInfo, !entire));
    }
#endif

    /* Commit the command buffer. */
    gcmONERROR(gcoHARDWARE_Commit());

    if ((DestInfo == Hardware->colorStates.surface)
    &&  (DestOrigin->x == 0)
    &&  (DestOrigin->y == 0)
    &&  (RectSize->x >= DestInfo->rect.right)
    &&  (RectSize->y >= DestInfo->rect.bottom)
    )
    {
        /* All pixels have been resolved. */
        Hardware->colorStates.dirty = gcvFALSE;
    }

    /* Tile status cache is dirty. */
    Hardware->cacheDirty = gcvTRUE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ComputeOffset
**
**  Compute the offset of the specified pixel location.
**
**  INPUT:
**
**      gctINT32 X, Y
**          Coordinates of the pixel.
**
**      gctUINT Stride
**          Surface stride.
**
**      gctINT BytesPerPixel
**          The number of bytes per pixel in the surface.
**
**      gctINT Tiling
**          Tiling type.
**
**  OUTPUT:
**
**      Computed pixel offset.
*/
gceSTATUS gcoHARDWARE_ComputeOffset(
    IN gctINT32 X,
    IN gctINT32 Y,
    IN gctUINT Stride,
    IN gctINT BytesPerPixel,
    IN gceTILING Tiling,
    OUT gctUINT32_PTR Offset
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    switch (Tiling)
    {
    case gcvLINEAR:
        /* Linear. */
        * Offset

            /* Skip full rows of pixels. */
            = Y * Stride

            /* Skip pixels to the left. */
            + X * BytesPerPixel;
        break;

    case gcvTILED:
        /* Tiled. */
        * Offset

            /* Skip full rows of tiles 4x4 to the top. */
            = (Y & ~3) * Stride

            + BytesPerPixel
            * (
                  /* Skip full 4x4 tiles to the left. */
                  ((X & ~0x03) << 2)

                  /* Skip rows/pixels inside the target 4x4 tile. */
                + ((Y &  0x03) << 2)
                + ((X &  0x03)     )
            );
        break;

    case gcvMULTI_TILED:
        /* Calc offset in the tile part of one PE. */
        {
            gctINT32 x, y;

            /* Adjust coordinates from screen into one PE. */
            x = (X  & ~0x8) | ((Y & 0x4) << 1);
            y = ((Y & ~0x7) >> 1) | (Y & 0x3);

            * Offset
                = (y & ~3) * Stride

                + BytesPerPixel
                * (
                      /* Skip full 4x4 tiles to the left. */
                      ((x & ~0x03) << 2)

                      /* Skip rows/pixels inside the target 4x4 tile. */
                    + ((y &  0x03) << 2)
                    + ((x &  0x03)     )
                );
        }
        break;

    case gcvMULTI_SUPERTILED:
        /* Calc offset in the tile part of one PE. */
        {
            gctINT32 x, y;

            /* Adjust coordinates from screen into one PE. */
            x = (X  & ~0x8) | ((Y & 0x4) << 1);
            y = ((Y & ~0x7) >> 1) | (Y & 0x3);

            * Offset
                /* Skip full rows of 64x64 tiles to the top. */
                = (y & ~63) * Stride

                + BytesPerPixel
                * (
                      /* Skip full 64x64 tiles to the left. */
                      ((x & ~0x3F) << 6)

                      /* Skip 8x16 tiles to the left and to the top. */
                    + ((y &  0x30) << 6)
                    + ((x &  0x38) << 4)

                      /* Skip full 4x4 tiles to the left and to the top. */
                    + ((y &  0x0C) << 3)
                    + ((x &  0x04) << 2)

                      /* Skip rows/pixels inside the target 4x4 tile. */
                    + ((y &  0x03) << 2)
                    + ((x &  0x03)     )
                  );
        }
        break;

    case gcvSUPERTILED:
        /* Super tiled. */
        * Offset

            /* Skip full rows of 64x64 tiles to the top. */
            = (Y & ~63) * Stride

            + BytesPerPixel
            * (
                  /* Skip full 64x64 tiles to the left. */
                  ((X & ~0x3F) << 6)

                  /* Skip full 16x16 tiles to the left and to the top. */
                + ((Y &  0x30) << 6)
                + ((X &  0x38) << 4)

                  /* Skip full 4x4 tiles to the left and to the top. */
                + ((Y &  0x0C) << 3)
                + ((X &  0x04) << 2)

                  /* Skip rows/pixels inside the target 4x4 tile. */
                + ((Y &  0x03) << 2)
                + ((X &  0x03)     )
            );
        break;

    default:
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ResolveRect
**
**  Resolve a rectangluar area of a surface to another surface.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR SrcInfo
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR DestInfo
**          Pointer to the destination surface descriptor.
**
**      gcsPOINT_PTR SrcOrigin
**          The origin of the source area to be resolved.
**
**      gcsPOINT_PTR DestOrigin
**          The origin of the destination area to be resolved.
**
**      gcsPOINT_PTR RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ResolveRect(
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctBOOL locked = gcvFALSE;
#ifndef VIVANTE_NO_3D
    gctBOOL pausedTileStatus = gcvFALSE;
#endif
    gctBOOL srcLocked = gcvFALSE;
    gctBOOL destLocked = gcvFALSE;
    gctBOOL resampling;
    gctBOOL dithering;
    gctBOOL srcTiled;
    gctBOOL destTiled;
    gcsPOINT alignedSrcOrigin, alignedSrcSize;
    gcsPOINT alignedDestOrigin, alignedDestSize;
    gcsPOINT alignedSrcOriginExpand = {0, 0};
    gcsPOINT alignedDestOriginExpand = {0, 0};
    gctBOOL originsMatch;
    gcsPOINT alignedRectSize;
    gcsSURF_FORMAT_INFO_PTR srcFormatInfo[2];
    gcsSURF_FORMAT_INFO_PTR dstFormatInfo[2];
    gcsSURF_FORMAT_INFO_PTR tmpFormatInfo;
    gcsPOINT tempOrigin;
    gctBOOL flip = gcvFALSE;
    gctBOOL expandRectangle = gcvTRUE;
    gctUINT32 srcAlignmentX = 0;
    gctUINT32 destAlignmentX = 0;
	gctUINT32 srcFormat, dstFormat;
    gctBOOL srcFlip;
    gctBOOL dstFlip;
	gctBOOL swcopy = gcvFALSE;
    gcoHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("SrcInfo=0x%x DestInfo=0x%x "
                    "SrcOrigin=0x%x DestOrigin=0x%x RectSize=0x%x",
                    SrcInfo, DestInfo,
                    SrcOrigin, DestOrigin, RectSize);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

#ifndef VIVANTE_NO_3D
    /* Pause tile status if the source is not the current render target. */
    if ((SrcInfo != hardware->colorStates.surface)
    &&  (SrcInfo != hardware->depthStates.surface)
    )
    {
        gcmONERROR(
            gcoHARDWARE_PauseTileStatus(hardware, gcvTILE_STATUS_PAUSE));

        pausedTileStatus = gcvTRUE;
    }
#endif

    /***********************************************************************
    ** Determine special functions.
    */

    srcTiled  = (SrcInfo->type  != gcvSURF_BITMAP);
    destTiled = (DestInfo->type != gcvSURF_BITMAP);

    resampling =  (SrcInfo->samples.x  != 1)
               || (SrcInfo->samples.y  != 1)
               || (DestInfo->samples.x != 1)
               || (DestInfo->samples.y != 1);

    dithering = hardware->dither[0] != hardware->dither[1];

#ifndef VIVANTE_NO_3D
    flip = SrcInfo->orientation != DestInfo->orientation;
#endif

    /* check if the format is supported by hw. */
    if (gcmIS_ERROR(_ConvertResolveFormat(hardware,
                              SrcInfo->format,
                              &srcFormat,
                              &srcFlip)) ||
	    gcmIS_ERROR(_ConvertResolveFormat(hardware,
                              DestInfo->format,
                              &dstFormat,
                              &dstFlip))
     )
	{
		swcopy = gcvTRUE;
	}

    /***********************************************************************
    ** Since 2D bitmaps live in linear space, we don't need to resolve them.
    ** We can just copy them using the 2D engine.  However, we need to flush
    ** and stall after we are done with the copy since we have to make sure
    ** the bits are there before we blit them to the screen.
    */

    if (!srcTiled && !destTiled && !resampling && !dithering && !flip)
    {
        gcmONERROR(_SourceCopy(
            hardware,
            SrcInfo, DestInfo,
            SrcOrigin, DestOrigin,
            RectSize
            ));
    }


    /***********************************************************************
    ** YUV 4:2:0 tiling case.
    */

    else if ((SrcInfo->format == gcvSURF_YV12) ||
             (SrcInfo->format == gcvSURF_I420) ||
             (SrcInfo->format == gcvSURF_NV12) ||
             (SrcInfo->format == gcvSURF_NV21))
    {
        if (!srcTiled && destTiled && (DestInfo->format == gcvSURF_YUY2))
        {
            gcmONERROR(_Tile420Surface(
                hardware,
                SrcInfo, DestInfo,
                SrcOrigin, DestOrigin,
                RectSize
                ));
        }
        else
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }


    /***********************************************************************
    ** Unsupported cases.
    */

    else if (
            /* Upsampling. */
               (SrcInfo->samples.x < DestInfo->samples.x)
            || (SrcInfo->samples.y < DestInfo->samples.y)

            /* the format that hw do not support*/
			|| swcopy
    )

    {
        gcmONERROR(gcoHARDWARE_CopyPixels(
            SrcInfo, DestInfo,
            SrcOrigin->x, SrcOrigin->y,
            DestOrigin->x, DestOrigin->y,
            RectSize->x, RectSize->y
            ));
    }


    /***********************************************************************
    ** Calculate the aligned source and destination rectangles; aligned to
    ** completely cover the specified source and destination areas.
    */

    else
    {
        gctBOOL srcMultiPipe = (SrcInfo->tiling == gcvMULTI_SUPERTILED)
                            || (SrcInfo->tiling == gcvMULTI_TILED);

        gctBOOL dstMultiPipe = (DestInfo->tiling == gcvMULTI_SUPERTILED)
                            || (DestInfo->tiling == gcvMULTI_TILED);

        _AlignResolveRect(
            hardware, SrcInfo, SrcOrigin, RectSize, &alignedSrcOrigin, &alignedSrcSize
            );

        _AlignResolveRect(
            hardware, DestInfo, DestOrigin, RectSize, &alignedDestOrigin, &alignedDestSize
            );

        /* Use the maximum rectangle. */
        alignedRectSize.x = gcmMAX(alignedSrcSize.x, alignedDestSize.x);
        alignedRectSize.y = gcmMAX(alignedSrcSize.y, alignedDestSize.y);

        /***********************************************************************
        ** If specified and aligned rectangles are the same, then the requested
        ** rectangle is prefectly aligned and we can do it in one shot.
        */

        originsMatch
            =  (alignedSrcOrigin.x  == SrcOrigin->x)
            && (alignedSrcOrigin.y  == SrcOrigin->y)
            && (alignedDestOrigin.x == DestOrigin->x)
            && (alignedDestOrigin.y == DestOrigin->y);

        if (!originsMatch)
        {
            /***********************************************************************
            ** The origins are changed.
            */

            gctINT srcDeltaX, srcDeltaY;
            gctINT destDeltaX, destDeltaY;
            gctINT maxDeltaX, maxDeltaY;

	        srcDeltaX  = SrcOrigin->x - alignedSrcOrigin.x;
	        srcDeltaY  = SrcOrigin->y - alignedSrcOrigin.y;

            destDeltaX = DestOrigin->x - alignedDestOrigin.x;
	        destDeltaY = DestOrigin->y - alignedDestOrigin.y;

	        maxDeltaX = gcmMAX(srcDeltaX, destDeltaX);
	        maxDeltaY = gcmMAX(srcDeltaY, destDeltaY);

            if (srcDeltaX == maxDeltaX)
            {
                /* The X coordinate of dest rectangle is changed. */
                alignedDestOriginExpand.x = DestOrigin->x - maxDeltaX;
                alignedSrcOriginExpand.x  = alignedSrcOrigin.x;
            }
            else
            {
                /* The X coordinate of src rectangle is changed. */
                alignedSrcOriginExpand.x  = SrcOrigin->x - maxDeltaX;
                alignedDestOriginExpand.x = alignedDestOrigin.x;
            }

            if (srcDeltaY == maxDeltaY)
            {
                /* Expand the Y coordinate of dest rectangle. */
                alignedDestOriginExpand.y = DestOrigin->y - maxDeltaY;
                alignedSrcOriginExpand.y  = alignedSrcOrigin.y;
            }
            else
            {
                /* Expand the Y cooridnate of src rectangle. */
                alignedSrcOriginExpand.y  = SrcOrigin->y - maxDeltaY;
                alignedDestOriginExpand.y = alignedDestOrigin.y;
            }

            if ((alignedSrcOriginExpand.x < 0)  ||
                (alignedSrcOriginExpand.y < 0)  ||
                (alignedDestOriginExpand.x < 0) ||
                (alignedDestOriginExpand.y < 0))
            {
                expandRectangle = gcvFALSE;
            }
            else
            {
                _GetAlignmentX(SrcInfo, &srcAlignmentX);
                _GetAlignmentX(DestInfo, &destAlignmentX);
            }
        }

        /* Fully aligned Origin and RectSize. */
        if (originsMatch &&
            (alignedRectSize.x == RectSize->x) &&
            (alignedRectSize.y == RectSize->y))
        {

            gcmONERROR(_ResolveRect(
                hardware,
                SrcInfo, DestInfo,
                SrcOrigin, DestOrigin,
                RectSize
                ));

        }
        /* Handle supertiling */
        else if ((originsMatch) &&
                 (destTiled) &&
                 ((RectSize->x > 64) && !(RectSize->x & 3)) &&
                 (!srcMultiPipe && !dstMultiPipe && !(RectSize->y & 3)) &&
                 (SrcInfo->superTiled && !DestInfo->superTiled) &&
                 (!(SrcOrigin->x  & 63) && !(SrcOrigin->y & 63))
                )
        {
            gcsPOINT srcOriginInterior, srcOriginExtra;
            gcsPOINT destOriginInterior, destOriginExtra;
            gcsPOINT rectSizeInterior, rectSizeExtra;
            gctUINT32 tileSize = 64;

            /* First resolve: Resolve tile aligned interior region. */
            srcOriginInterior.x  = SrcOrigin->x;
            srcOriginInterior.y  = SrcOrigin->y;
            destOriginInterior.x = DestOrigin->x;
            destOriginInterior.y = DestOrigin->y;
            rectSizeInterior.x   = (RectSize->x/tileSize)*tileSize;
            rectSizeInterior.y   = RectSize->y;

            gcmONERROR(_ResolveRect(
                hardware,
                SrcInfo, DestInfo,
                &srcOriginInterior, &destOriginInterior,
                &rectSizeInterior
                ));

            /* Second resolve: Resolve the right end. */
            srcOriginExtra.x  = SrcOrigin->x + rectSizeInterior.x;
            srcOriginExtra.y  = SrcOrigin->y;
            destOriginExtra.x = DestOrigin->x + rectSizeInterior.x;
            destOriginExtra.y = DestOrigin->y;
            rectSizeExtra.x   = RectSize->x - rectSizeInterior.x;
            /* Resolve entire column. */
            rectSizeExtra.y   = RectSize->y;

            gcmONERROR(gcoHARDWARE_CopyPixels(
                SrcInfo, DestInfo,
                srcOriginExtra.x, srcOriginExtra.y,
                destOriginExtra.x, destOriginExtra.y,
                rectSizeExtra.x, rectSizeExtra.y
                ));
        }
        /* Aligned Origin and tile aligned RectSize. */
        /* Assuming tile size of 4x4 pixels, and resolve size requirement of 4 tiles. */
        else if (originsMatch &&
                 (RectSize->x > 16) &&
                ((RectSize->x & 3) == 0) &&
                (!srcMultiPipe && !dstMultiPipe && ((RectSize->y & 3) == 0)))
        {
            gcsPOINT srcOriginInterior, srcOriginExtra;
            gcsPOINT destOriginInterior, destOriginExtra;
            gcsPOINT rectSizeInterior, rectSizeExtra;
            gctUINT32 extraTiles;
            gctUINT32 tileSize = 4, numTiles = 4;
			gctUINT32 loopCount = 0;

            extraTiles = numTiles - (alignedRectSize.x - RectSize->x) / tileSize;

            /* First resolve: Resolve 4 tile aligned interior region. */
            srcOriginInterior.x  = SrcOrigin->x;
            srcOriginInterior.y  = SrcOrigin->y;
            destOriginInterior.x = DestOrigin->x;
            destOriginInterior.y = DestOrigin->y;
            rectSizeInterior.x   = RectSize->x - extraTiles * tileSize;
            rectSizeInterior.y   = RectSize->y;

            gcmONERROR(_ResolveRect(
                hardware,
                SrcInfo, DestInfo,
                &srcOriginInterior, &destOriginInterior,
                &rectSizeInterior
                ));

            /* Second resolve: Resolve last 4 tiles at the right end. */
            srcOriginExtra.x  = SrcOrigin->x + RectSize->x - (4 * tileSize);
            srcOriginExtra.y  = SrcOrigin->y;
            destOriginExtra.x = DestOrigin->x + RectSize->x - (4 * tileSize);
            destOriginExtra.y = DestOrigin->y;
            rectSizeExtra.x   = 4 * tileSize;
            rectSizeExtra.y   = 4 * hardware->pixelPipes;

            do
            {
                gcmASSERT(srcOriginExtra.y < (SrcOrigin->y + RectSize->y));
                gcmONERROR(_ResolveRect(
                    hardware,
                    SrcInfo, DestInfo,
                    &srcOriginExtra, &destOriginExtra,
                    &rectSizeExtra
                    ));

                srcOriginExtra.y  += 4 * hardware->pixelPipes;
                destOriginExtra.y += 4 * hardware->pixelPipes;
                loopCount++;
            }
            while (srcOriginExtra.y != (SrcOrigin->y + RectSize->y) && (loopCount < MAX_LOOP_COUNT));
        }

        /***********************************************************************
        ** Not matched origins.
        ** We expand the rectangle to garantee the alignment requirments.
        */

        else if (!originsMatch && expandRectangle &&
                 !(alignedSrcOriginExpand.x & (srcAlignmentX -1)) &&
                 !(alignedSrcOriginExpand.y & 3) &&
                 !(alignedDestOriginExpand.x & (destAlignmentX - 1)) &&
                 !(alignedDestOriginExpand.y & 3))
        {
            gcmONERROR(_ResolveRect(
                hardware,
                SrcInfo, DestInfo,
                &alignedSrcOriginExpand, &alignedDestOriginExpand,
                &alignedRectSize
                ));
        }

        /***********************************************************************
        ** Handle all cases when source and destination are both tiled.
        */

        else if ((SrcInfo->type  != gcvSURF_BITMAP) &&
                 (DestInfo->type != gcvSURF_BITMAP))
        {
            static gctBOOL printed = gcvFALSE;

            /* Flush the pipe. */
            gcmONERROR(gcoHARDWARE_FlushPipe());

#ifndef VIVANTE_NO_3D
            /* Disable the tile tatus. */
            gcmONERROR(gcoHARDWARE_DisableTileStatus(
                SrcInfo, gcvTRUE
                ));
#endif

            /* Lock the source surface. */
            gcmONERROR(gcoHARDWARE_Lock(
                &SrcInfo->node, gcvNULL, gcvNULL
                ));

            srcLocked = gcvTRUE;

            /* Lock the destination surface. */
            gcmONERROR(gcoHARDWARE_Lock(
                &DestInfo->node, gcvNULL, gcvNULL
                ));

            destLocked = gcvTRUE;

            if (!printed)
            {
                printed = gcvTRUE;

                gcmPRINT("libGAL: Performing a software resolve!");
            }

            /* Perform software copy. */
            if (hardware->pixelPipes > 1)
            {
                gcmONERROR(gcoHARDWARE_CopyPixels(
                    SrcInfo, DestInfo,
                    SrcOrigin->x, SrcOrigin->y,
                    DestOrigin->x, DestOrigin->y,
                    RectSize->x, RectSize->y
                    ));
            }
            else
            {
                gcmONERROR(_SoftwareCopy(
                    hardware,
                    SrcInfo, DestInfo,
                    SrcOrigin, DestOrigin,
                    RectSize
                    ));
            }
        }

        /***********************************************************************
        ** At least one side of the rectangle is not aligned. In this case we
        ** will allocate a temporary buffer to resolve the aligned rectangle
        ** to and then use a source copy to complete the operation.
        */

        else
        {
            gctPOINTER memory = gcvNULL;

            /* Query format specifics. */
            gcmONERROR(gcoSURF_QueryFormat(SrcInfo->format, srcFormatInfo));
            gcmONERROR(gcoSURF_QueryFormat(DestInfo->format, dstFormatInfo));

            /* Pick the most compact format for the temporary surface. */
            tmpFormatInfo
                = (srcFormatInfo[0]->bitsPerPixel < dstFormatInfo[0]->bitsPerPixel)
                    ? srcFormatInfo[0]
                    : dstFormatInfo[0];

            /* Allocate the temporary surface. */
            gcmONERROR(gcoHARDWARE_AllocateTemporarySurface(
                hardware,
                alignedRectSize.x,
                alignedRectSize.y,
                tmpFormatInfo,
                DestInfo->type
                ));

            /* Lock the temporary surface. */
            gcmONERROR(gcoHARDWARE_Lock(
                &hardware->tempBuffer.node,
                gcvNULL,
                &memory
                ));

            /* Flush temporary surface cache, as it could be being re-used. */
            gcmONERROR(
                gcoSURF_NODE_Cache(&hardware->tempBuffer.node,
                                 memory,
                                 hardware->tempBuffer.size,
                                 gcvCACHE_FLUSH));

            /* Mark as locked. */
            locked = gcvTRUE;

            /* Set the temporary buffer origin. */
            tempOrigin.x = 0;
            tempOrigin.y = 0;

            /* Resolve the aligned rectangle into the temporary surface. */
            gcmONERROR(_ResolveRect(
                hardware,
                SrcInfo,
                &hardware->tempBuffer,
                &alignedSrcOrigin,
                &tempOrigin,
                &alignedRectSize
                ));

            /* Compute the temporary buffer origin. */
            tempOrigin.x = SrcOrigin->x - alignedSrcOrigin.x;
            tempOrigin.y = SrcOrigin->y - alignedSrcOrigin.y;

            /* Copy the unaligned area to the final destination. */
            gcmONERROR(_SourceCopy(
                hardware,
                &hardware->tempBuffer,
                DestInfo,
                &tempOrigin,
                DestOrigin,
                RectSize
                ));
        }
    }

OnError:
    if (hardware != gcvNULL)
    {
#ifndef VIVANTE_NO_3D
        /* Resume tile status if it was paused. */
        if (pausedTileStatus)
        {
            gcmVERIFY_OK(gcoHARDWARE_PauseTileStatus(
                hardware, gcvTILE_STATUS_RESUME
                ));
        }
#endif

        /* Unlock. */
        if (locked)
        {
            /* Unlock the temporary surface. */
            gcmVERIFY_OK(gcoHARDWARE_Unlock(
                &hardware->tempBuffer.node, hardware->tempBuffer.type
                ));
        }

        if (srcLocked)
        {
            /* Unlock the source. */
            gcmVERIFY_OK(gcoHARDWARE_Unlock(
                &SrcInfo->node, SrcInfo->type
                ));
        }

        if (destLocked)
        {
            /* Unlock the source. */
            gcmVERIFY_OK(gcoHARDWARE_Unlock(
                &DestInfo->node, DestInfo->type
                ));
        }
    }

    /* Return result. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHARDWARE_Lock
**
**  Lock video memory.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR Node
**          Pointer to a gcsSURF_NODE structure that describes the video
**          memory to lock.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address of the surface.
**
**      gctPOINTER * Memory
**          Logical address of the surface.
*/
gceSTATUS
gcoHARDWARE_Lock(
    IN gcsSURF_NODE_PTR Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x Address=0x%x Memory=0x%x",
                  Node, Address, Memory);

    if (!Node->valid)
    {
        gcsHAL_INTERFACE iface;

        /* User pools have to be mapped first. */
        if (Node->pool == gcvPOOL_USER)
        {
            gcmONERROR(gcvSTATUS_MEMORY_UNLOCKED);
        }

        /* Fill in the kernel call structure. */
        iface.command = gcvHAL_LOCK_VIDEO_MEMORY;
        iface.u.LockVideoMemory.node = Node->u.normal.node;
        iface.u.LockVideoMemory.cacheable = Node->u.normal.cacheable;

        /* Call the kernel. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL,
            IOCTL_GCHAL_INTERFACE,
            &iface, sizeof(iface),
            &iface, sizeof(iface)
            ));

        /* Success? */
        gcmONERROR(iface.status);

        /* Validate the node. */
        Node->valid = gcvTRUE;

        /* Store pointers. */
        Node->physical = iface.u.LockVideoMemory.address;
        Node->logical  = iface.u.LockVideoMemory.memory;

        /* Set locked in the kernel flag. */
        Node->lockedInKernel = gcvTRUE;
        gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &Node->lockedHardwareType));
    }

    /* Increment the lock count. */
    Node->lockCount++;

    /* Set the result. */
    if (Address != gcvNULL)
    {
        *Address = Node->physical;
    }

    if (Memory != gcvNULL)
    {
        *Memory = Node->logical;
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Unlock
**
**  Unlock video memory.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR Node
**          Pointer to a gcsSURF_NODE structure that describes the video
**          memory to unlock.
**
**      gceSURF_TYPE Type
**          Type of surface to unlock.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Unlock(
    IN gcsSURF_NODE_PTR Node,
    IN gceSURF_TYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gctBOOL currentTypeChanged = gcvFALSE;
    gceHARDWARE_TYPE currentType = gcvHARDWARE_INVALID;

    gcmHEADER_ARG("Node=0x%x Type=%d",
                   Node, Type);

    /* Verify whether the node is valid. */
    if (!Node->valid || (Node->lockCount <= 0))
    {
        gcmTRACE_ZONE(
            gcvLEVEL_WARNING, gcvZONE_SURFACE,
            "gcoHARDWARE_Unlock: Node=0x%x; unlock called on an unlocked surface.",
            Node
            );
    }

    /* Locked more then once? */
    else if (--Node->lockCount == 0)
    {
        if (Node->lockedInKernel)
        {
            /* Save the current hardware type */
            gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &currentType));

            if (Node->lockedHardwareType != currentType)
            {
                /* Change to the locked hardware type */
                gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, Node->lockedHardwareType));
                currentTypeChanged = gcvTRUE;
            }

            /* Unlock the video memory node. */
            iface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
            iface.u.UnlockVideoMemory.node = Node->u.normal.node;
            iface.u.UnlockVideoMemory.type = Type & ~gcvSURF_NO_VIDMEM;
            iface.u.UnlockVideoMemory.asynchroneous = gcvTRUE;

            /* Call the kernel. */
            gcmONERROR(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &iface, gcmSIZEOF(iface),
                &iface, gcmSIZEOF(iface)
                ));

            /* Success? */
            gcmONERROR(iface.status);

            /* Do we need to schedule an event for the unlock? */
            if (iface.u.UnlockVideoMemory.asynchroneous)
            {
                iface.u.UnlockVideoMemory.asynchroneous = gcvFALSE;
                gcmONERROR(gcoHARDWARE_CallEvent(&iface));
            }

            /* Reset locked in the kernel flag. */
            Node->lockedInKernel = gcvFALSE;

            if (currentTypeChanged)
            {
                /* Restore the current hardware type */
                gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, currentType));
                currentTypeChanged = gcvFALSE;
            }
        }

        /* Reset the valid flag. */
        if ((Node->pool == gcvPOOL_CONTIGUOUS) ||
            (Node->pool == gcvPOOL_VIRTUAL))
        {
            Node->valid = gcvFALSE;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (currentTypeChanged)
    {
        /* Restore the current hardware type */
        gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, currentType));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_CallEvent
**
**  Send an event to the kernel and append the required synchronization code to
**  the command buffer..
**
**  INPUT:
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure the defines the event to
**          send.
**
**  OUTPUT:
**
**      gcsHAL_INTERFACE * Interface
**          Pointer to an gcsHAL_INTERFACE structure the received information
**          from the kernel.
*/
gceSTATUS
gcoHARDWARE_CallEvent(
    IN OUT gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Interface=0x%x", Interface);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Interface != gcvNULL);

    /* Append the event to the event queue. */
    gcmONERROR(gcoQUEUE_AppendEvent(hardware->queue, Interface));

#if gcdIN_QUEUE_RECORD_LIMIT
    if (hardware->queue->recordCount >= gcdIN_QUEUE_RECORD_LIMIT)
    {
        gcmONERROR(gcoHARDWARE_Commit());
    }
#endif

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ScheduleVideoMemory
**
**  Schedule destruction for the specified video memory node.
**
**  INPUT:
**
**      gcsSURF_NODE_PTR Node
**          Pointer to the video momory node to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ScheduleVideoMemory(
    IN gcsSURF_NODE_PTR Node
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Node=0x%x", Node);

    if (Node->pool != gcvPOOL_USER)
    {
        gcsHAL_INTERFACE iface;

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_HARDWARE,
                      "node=0x%08x",
                      Node->u.normal.node);

        /* Free the allocated video memory asynchronously. */
        iface.command = gcvHAL_FREE_VIDEO_MEMORY;
        iface.u.FreeVideoMemory.node = Node->u.normal.node;

        /* Call kernel HAL. */
        gcmONERROR(gcoHARDWARE_CallEvent(&iface));
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_AllocateTemporarySurface
**
**  Allocates a temporary surface with specified parameters.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gctUINT Width, Height
**          The aligned size of the surface to be allocated.
**
**      gcsSURF_FORMAT_INFO_PTR Format
**          The format of the surface to be allocated.
**
**      gceSURF_TYPE Type
**          The type of the surface to be allocated.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_AllocateTemporarySurface(
    IN gcoHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gcsSURF_FORMAT_INFO_PTR Format,
    IN gceSURF_TYPE Type
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Hardware=0x%x Width=%d Height=%d "
                    "Format=%d Type=%d",
                    Hardware, Width, Height,
                    *Format, Type);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Do we have a compatible surface? */
    if ((Hardware->tempBuffer.type == Type) &&
        (Hardware->tempBuffer.format == Format->format) &&
        (Hardware->tempBuffer.rect.right == (gctINT) Width) &&
        (Hardware->tempBuffer.rect.bottom == (gctINT) Height))
    {
        status = gcvSTATUS_OK;
    }
    else
    {
        /* Delete existing buffer. */
        gcmONERROR(gcoHARDWARE_FreeTemporarySurface(gcvTRUE));

        Hardware->tempBuffer.alignedWidth  = Width;
        Hardware->tempBuffer.alignedHeight = Height;

        /* Align the width and height. */
        gcmONERROR(gcoHARDWARE_AlignToTile(
            Type,
            Format->format,
            &Hardware->tempBuffer.alignedWidth,
            &Hardware->tempBuffer.alignedHeight,
            gcvNULL
            ));

        /* Init the interface structure. */
        iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        iface.u.AllocateLinearVideoMemory.bytes     = Hardware->tempBuffer.alignedWidth
                                                    * Format->bitsPerPixel / 8
                                                    * Hardware->tempBuffer.alignedHeight;
        iface.u.AllocateLinearVideoMemory.alignment = 64;
        iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
        iface.u.AllocateLinearVideoMemory.type      = Type;

        /* Call kernel service. */
        gcmONERROR(gcoOS_DeviceControl(
            gcvNULL, IOCTL_GCHAL_INTERFACE,
            &iface, sizeof(gcsHAL_INTERFACE),
            &iface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmONERROR(iface.status);

        /* Set the new parameters. */
        Hardware->tempBuffer.type                = Type;
        Hardware->tempBuffer.format              = Format->format;
        Hardware->tempBuffer.stride              = Width * Format->bitsPerPixel / 8;
        Hardware->tempBuffer.size                = iface.u.AllocateLinearVideoMemory.bytes;
        Hardware->tempBuffer.node.valid          = gcvFALSE;
        Hardware->tempBuffer.node.lockCount      = 0;
        Hardware->tempBuffer.node.lockedInKernel = gcvFALSE;
        Hardware->tempBuffer.node.logical        = gcvNULL;
        Hardware->tempBuffer.node.physical       = ~0U;

        Hardware->tempBuffer.node.pool
            = iface.u.AllocateLinearVideoMemory.pool;
        Hardware->tempBuffer.node.u.normal.node
            = iface.u.AllocateLinearVideoMemory.node;
        Hardware->tempBuffer.node.u.normal.cacheable = gcvFALSE;

#ifndef VIVANTE_NO_3D
        Hardware->tempBuffer.samples.x = 1;
        Hardware->tempBuffer.samples.y = 1;
#endif /* VIVANTE_NO_3D */

        Hardware->tempBuffer.rect.left   = 0;
        Hardware->tempBuffer.rect.top    = 0;
        Hardware->tempBuffer.rect.right  = Width;
        Hardware->tempBuffer.rect.bottom = Height;
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_FreeTemporarySurface
**
**  Free the temporary surface.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_FreeTemporarySurface(
    IN gctBOOL Synchronized
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Synchronized=%d", Synchronized);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Is there a surface allocated? */
    if (hardware->tempBuffer.node.pool != gcvPOOL_UNKNOWN)
    {
        /* Schedule deletion. */
        if (Synchronized)
        {
            gcmONERROR(gcoHARDWARE_ScheduleVideoMemory(
                &hardware->tempBuffer.node
                ));
        }

        /* Not synchronized --> delete immediately. */
        else
        {
            gcsHAL_INTERFACE iface;

            /* Free the video memory. */
            iface.command = gcvHAL_FREE_VIDEO_MEMORY;
            iface.u.FreeVideoMemory.node = hardware->tempBuffer.node.u.normal.node;

            /* Call kernel API. */
            gcmONERROR(gcoHAL_Call(gcvNULL, &iface));
        }

        /* Reset the temporary surface. */
        gcmONERROR(gcoOS_ZeroMemory(
            &hardware->tempBuffer, sizeof(hardware->tempBuffer)
            ));

        hardware->tempBuffer.node.pool = gcvPOOL_UNKNOWN;
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Get2DTempSurface
**
**  Allocates a temporary surface with specified parameters.
**
**  INPUT:
**
**      gctUINT Width, Height
**          The aligned size of the surface to be allocated.
**
**      gceSURF_FORMAT Format
**          The format of the surface to be allocated.
**
**  OUTPUT:
**
**      gcsSURF_INFO_PTR *SurfInfo
*/
gceSTATUS
gcoHARDWARE_Get2DTempSurface(
    IN gctUINT Width,
    IN gctUINT Height,
    IN gceSURF_FORMAT Format,
    OUT gcsSURF_INFO_PTR *SurfInfo
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    gcsSURF_INFO_PTR surf = gcvNULL;
    gcsSURF_FORMAT_INFO_PTR formatInfo[2];
    gctUINT alignedWidth, alignedHeight, size, delta = 0;
    gctINT  i, idx = -1;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Width=%d Height=%d "
                    "Format=%d",
                    Width, Height,
                    Format);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    alignedWidth = Width;
    alignedHeight = Height;

    /* Align the width and height. */
    gcmONERROR(gcoHARDWARE_AlignToTile(
        gcvSURF_BITMAP, Format, &alignedWidth, &alignedHeight, gcvNULL
        ));

    gcmONERROR(gcoSURF_QueryFormat(Format, formatInfo));

    size = alignedWidth * formatInfo[0]->bitsPerPixel  / 8 * alignedHeight;

    /* Do we have a fit surface? */
    for (i = 0; i < gcdTEMP_SURFACE_NUMBER; i += 1)
    {
        if ((hardware->temp2DSurf[i] != gcvNULL)
            && (hardware->temp2DSurf[i]->node.size >= size))
        {
            if (idx == -1)
            {
                delta = hardware->temp2DSurf[i]->node.size - size;
                idx = i;
            }
            else if (hardware->temp2DSurf[i]->node.size - size < delta)
            {
                delta = hardware->temp2DSurf[i]->node.size - size;
                idx = i;
            }
        }
    }

    if (idx != -1)
    {
        gcmASSERT((idx >= 0) && (idx < gcdTEMP_SURFACE_NUMBER));

        *SurfInfo = hardware->temp2DSurf[idx];
        hardware->temp2DSurf[idx] = gcvNULL;

        (*SurfInfo)->format              = Format;
        (*SurfInfo)->stride              = alignedWidth * formatInfo[0]->bitsPerPixel / 8;
        (*SurfInfo)->alignedWidth        = alignedWidth;
        (*SurfInfo)->alignedHeight       = alignedHeight;
        (*SurfInfo)->is16Bit             = formatInfo[0]->bitsPerPixel == 16;
        (*SurfInfo)->rotation            = gcvSURF_0_DEGREE;
        (*SurfInfo)->orientation         = gcvORIENTATION_TOP_BOTTOM;

        (*SurfInfo)->rect.left   = 0;
        (*SurfInfo)->rect.top    = 0;
        (*SurfInfo)->rect.right  = Width;
        (*SurfInfo)->rect.bottom = Height;
    }
    else
    {
        gcmONERROR(gcoOS_Allocate(gcvNULL, sizeof(gcsSURF_INFO), (gctPOINTER *)&surf));
        gcmONERROR(gcoOS_ZeroMemory(surf, sizeof(gcsSURF_INFO)));

        surf->node.u.normal.node = gcvNULL;
        surf->node.valid          = gcvFALSE;

        /* Init the interface structure. */
        iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        iface.u.AllocateLinearVideoMemory.bytes     = size;
        iface.u.AllocateLinearVideoMemory.alignment = 64;
        iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
        iface.u.AllocateLinearVideoMemory.type      = gcvSURF_BITMAP;

        /* Call kernel service. */
        gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

        /* Set the new parameters. */
        surf->type                = gcvSURF_BITMAP;
        surf->format              = Format;
        surf->stride              = alignedWidth * formatInfo[0]->bitsPerPixel / 8;
        surf->alignedWidth        = alignedWidth;
        surf->alignedHeight       = alignedHeight;
        surf->size                = iface.u.AllocateLinearVideoMemory.bytes;
        surf->is16Bit             = formatInfo[0]->bitsPerPixel == 16;

#ifndef VIVANTE_NO_3D
        surf->samples.x           = 1;
        surf->samples.y           = 1;
#endif

        surf->rotation            = gcvSURF_0_DEGREE;
        surf->orientation         = gcvORIENTATION_TOP_BOTTOM;

        surf->rect.left   = 0;
        surf->rect.top    = 0;
        surf->rect.right  = Width;
        surf->rect.bottom = Height;

        surf->node.lockCount      = 0;
        surf->node.lockedInKernel = gcvFALSE;
        surf->node.logical        = gcvNULL;
        surf->node.physical       = ~0U;

        surf->node.pool
            = iface.u.AllocateLinearVideoMemory.pool;

        surf->node.u.normal.node
            = iface.u.AllocateLinearVideoMemory.node;

        surf->node.u.normal.cacheable = gcvFALSE;

        surf->node.size = iface.u.AllocateLinearVideoMemory.bytes;

        gcmONERROR(gcoHARDWARE_Lock(&surf->node, gcvNULL, gcvNULL));

        *SurfInfo = surf;
    }

OnError:
    if (gcmIS_ERROR(status) && (surf != gcvNULL))
    {
        if (surf->node.valid)
        {
            gcmVERIFY_OK(gcoHARDWARE_Unlock(
                &surf->node,
                gcvSURF_BITMAP
                ));
        }

        if (surf->node.u.normal.node != gcvNULL)
        {
            /* Free the video memory immediately. */
            iface.command = gcvHAL_FREE_VIDEO_MEMORY;
            iface.u.FreeVideoMemory.node = surf->node.u.normal.node;

            /* Call kernel API. */
            if (gcmIS_ERROR((gcoHAL_Call(gcvNULL, &iface))))
            {
                gcmTRACE_ZONE(gcvLEVEL_ERROR,
                              gcvZONE_HARDWARE,
                              "gcoHAL_Call fail.");
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surf));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_Put2DTempSurface
**
**  Put back the temporary surface.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR SurfInfo
**          The info of the surface to be free'd.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Put2DTempSurface(
    IN gcsSURF_INFO_PTR SurfInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;
    gcsSURF_INFO_PTR surf = SurfInfo;
    gctINT i;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("SurfInfo=0x%x", SurfInfo);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmASSERT(SurfInfo != gcvNULL);

    for (i = 0; i < gcdTEMP_SURFACE_NUMBER; i++)
    {
        /* Is there an empty slot? */
        if (hardware->temp2DSurf[i] == gcvNULL)
        {
            hardware->temp2DSurf[i] = surf;
            break;
        }

        /* Swap the smaller one. */
        else if (hardware->temp2DSurf[i]->node.size < surf->node.size)
        {
            gcsSURF_INFO_PTR temp = surf;

            surf = hardware->temp2DSurf[i];
            hardware->temp2DSurf[i] = temp;
        }
    }

    if (i == gcdTEMP_SURFACE_NUMBER)
    {
        if (surf->node.valid)
        {
            gcmONERROR(gcoHARDWARE_Unlock(
                &surf->node,
                gcvSURF_BITMAP
                ));
        }

        iface.command = gcvHAL_FREE_VIDEO_MEMORY;
        iface.u.FreeVideoMemory.node = surf->node.u.normal.node;

        /* Schedule deletion. */
        gcmONERROR(gcoHARDWARE_CallEvent(&iface));
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, surf));
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_ConvertPixel
**
**  Convert source pixel from source format to target pixel' target format.
**  The source format class should be either identical or convertible to
**  the target format class.
**
**  INPUT:
**
**      gctPOINTER SrcPixel, TrgPixel,
**          Pointers to source and target pixels.
**
**      gctUINT SrcBitOffset, TrgBitOffset
**          Bit offsets of the source and target pixels relative to their
**          respective pointers.
**
**      gcsSURF_FORMAT_INFO_PTR SrcFormat, TrgFormat
**          Pointers to source and target pixel format descriptors.
**
**      gcsBOUNDARY_PTR SrcBoundary, TrgBoundary
**          Pointers to optional boundary structures to verify the source
**          and target position. If the source is found to be beyond the
**          defined boundary, it will be assumed to be 0. If the target
**          is found to be beyond the defined boundary, the write will
**          be ignored. If boundary checking is not needed, gcvNULL can be
**          passed.
**
**  OUTPUT:
**
**      gctPOINTER TrgPixel + TrgBitOffset
**          Converted pixel.
*/
gceSTATUS gcoHARDWARE_ConvertPixel(
    IN gctPOINTER SrcPixel,
    OUT gctPOINTER TrgPixel,
    IN gctUINT SrcBitOffset,
    IN gctUINT TrgBitOffset,
    IN gcsSURF_FORMAT_INFO_PTR SrcFormat,
    IN gcsSURF_FORMAT_INFO_PTR TrgFormat,
    IN gcsBOUNDARY_PTR SrcBoundary,
    IN gcsBOUNDARY_PTR TrgBoundary
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("SrcPixel=0x%x TrgPixel=0x%x "
                    "SrcBitOffset=%d TrgBitOffset=%d SrcFormat=0x%x "
                    "TrgFormat=0x%x SrcBoundary=0x%x TrgBoundary=0x%x",
                    SrcPixel, TrgPixel,
                    SrcBitOffset, TrgBitOffset, SrcFormat,
                    TrgFormat, SrcBoundary, TrgBoundary);

    if (SrcFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.alpha,
                &TrgFormat->u.rgba.alpha,
                SrcBoundary, TrgBoundary,
                ~0U
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.red,
                &TrgFormat->u.rgba.red,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.green,
                &TrgFormat->u.rgba.green,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.blue,
                &TrgFormat->u.rgba.blue,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.red,
                &TrgFormat->u.lum.value,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.alpha,
                &TrgFormat->u.lum.alpha,
                SrcBoundary, TrgBoundary,
                ~0U
                ));
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_YUV)
        {
            gctUINT8 r[4] = {0};
            gctUINT8 g[4] = {0};
            gctUINT8 b[4] = {0};
            gctUINT8 y[4] = {0};
            gctUINT8 u[4] = {0};
            gctUINT8 v[4] = {0};

            /*
                Get RGB value.
            */

            gcmONERROR(_ConvertComponent(
                SrcPixel, r, SrcBitOffset, 0,
                &SrcFormat->u.rgba.red, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, g, SrcBitOffset, 0,
                &SrcFormat->u.rgba.green, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, b, SrcBitOffset, 0,
                &SrcFormat->u.rgba.blue, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            /*
                Convert to YUV.
            */

            gcoHARDWARE_RGB2YUV(
                 r[0], g[0], b[0],
                 y, u, v
                );

            /*
                Average Us and Vs for odd pixels.
            */
            if ((TrgFormat->interleaved & gcvCOMPONENT_ODD) != 0)
            {
                gctUINT8 curU[4] = {0}, curV[4] = {0};

                gcmONERROR(_ConvertComponent(
                    TrgPixel, curU, TrgBitOffset, 0,
                    &TrgFormat->u.yuv.u, &gcvPIXEL_COMP_XXX8,
                    TrgBoundary, gcvNULL, 0
                    ));

                gcmONERROR(_ConvertComponent(
                    TrgPixel, curV, TrgBitOffset, 0,
                    &TrgFormat->u.yuv.v, &gcvPIXEL_COMP_XXX8,
                    TrgBoundary, gcvNULL, 0
                    ));


                u[0] = (gctUINT8) (((gctUINT16) u[0] + (gctUINT16) curU[0]) >> 1);
                v[0] = (gctUINT8) (((gctUINT16) v[0] + (gctUINT16) curV[0]) >> 1);
            }

            /*
                Convert to the final format.
            */

            gcmONERROR(_ConvertComponent(
                y, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.y,
                gcvNULL, TrgBoundary, 0
                ));

            gcmONERROR(_ConvertComponent(
                u, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.u,
                gcvNULL, TrgBoundary, 0
                ));

            gcmONERROR(_ConvertComponent(
                v, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.v,
                gcvNULL, TrgBoundary, 0
                ));
        }

        else
        {
            /* Not supported combination. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_YUV)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_YUV)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.y,
                &TrgFormat->u.yuv.y,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.u,
                &TrgFormat->u.yuv.u,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.v,
                &TrgFormat->u.yuv.v,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
        {
            gctUINT8 y[4]={0}, u[4]={0}, v[4]={0};
            gctUINT8 r[4], g[4], b[4];

            /*
                Get YUV value.
            */

            gcmONERROR(_ConvertComponent(
                SrcPixel, y, SrcBitOffset, 0,
                &SrcFormat->u.yuv.y, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, u, SrcBitOffset, 0,
                &SrcFormat->u.yuv.u, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, v, SrcBitOffset, 0,
                &SrcFormat->u.yuv.v, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                ));

            /*
                Convert to RGB.
            */

            gcoHARDWARE_YUV2RGB(
                 y[0], u[0], v[0],
                 r, g, b
                );

            /*
                Convert to the final format.
            */

            gcmONERROR(_ConvertComponent(
                gcvNULL, TrgPixel, 0, TrgBitOffset,
                gcvNULL, &TrgFormat->u.rgba.alpha,
                gcvNULL, TrgBoundary, ~0U
                ));

            gcmONERROR(_ConvertComponent(
                r, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.red,
                gcvNULL, TrgBoundary, 0
                ));

            gcmONERROR(_ConvertComponent(
                g, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.green,
                gcvNULL, TrgBoundary, 0
                ));

            gcmONERROR(_ConvertComponent(
                b, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.blue,
                gcvNULL, TrgBoundary, 0
                ));
        }

        else
        {
            /* Not supported combination. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_INDEX)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_INDEX)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.index.value,
                &TrgFormat->u.index.value,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else
        {
            /* Not supported combination. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.lum.alpha,
                &TrgFormat->u.lum.alpha,
                SrcBoundary, TrgBoundary,
                ~0U
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.lum.value,
                &TrgFormat->u.lum.value,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else
        {
            /* Not supported combination. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_BUMP)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_BUMP)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.alpha,
                &TrgFormat->u.bump.alpha,
                SrcBoundary, TrgBoundary,
                ~0U
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.l,
                &TrgFormat->u.bump.l,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.v,
                &TrgFormat->u.bump.v,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.u,
                &TrgFormat->u.bump.u,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.q,
                &TrgFormat->u.bump.q,
                SrcBoundary, TrgBoundary,
                0
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.w,
                &TrgFormat->u.bump.w,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else
        {
            /* Not supported combination. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_DEPTH)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_DEPTH)
        {
            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.depth.depth,
                &TrgFormat->u.depth.depth,
                SrcBoundary, TrgBoundary,
                ~0U
                ));

            gcmONERROR(_ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.depth.stencil,
                &TrgFormat->u.depth.stencil,
                SrcBoundary, TrgBoundary,
                0
                ));
        }

        else
        {
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }
    }

    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Return status. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoHARDWARE_CopyPixels
**
**  Copy a rectangular area from one surface to another with format conversion.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR Source
**          Pointer to the source surface descriptor.
**
**      gcsSURF_INFO_PTR Target
**          Pointer to the destination surface descriptor.
**
**      gctINT SourceX, SourceY
**          Source surface origin.
**
**      gctINT TargetX, TargetY
**          Target surface origin.
**
**      gctINT Width, Height
**          The size of the area. If Height is negative, the area will
**          be copied with a vertical flip.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_CopyPixels(
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
    IN gctINT SourceX,
    IN gctINT SourceY,
    IN gctINT TargetX,
    IN gctINT TargetY,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;
    gcsSURF_INFO_PTR source;
    gctPOINTER memory = gcvNULL;
    gcsSURF_FORMAT_INFO_PTR srcFormat[2];
    gcsSURF_FORMAT_INFO_PTR trgFormat[2];
    gctBOOL fastPathCopy = gcvFALSE;
    gcoHARDWARE hardware = gcvNULL;
    gctUINT32 sFormat;
    gctBOOL srcFlip;
    gctBOOL hwcopy = gcvTRUE;
    gctBOOL stalled = gcvFALSE;

    gcmHEADER_ARG("Source=0x%x Target=0x%x "
                    "SourceX=%d SourceY=%d TargetX=%d "
                    "TargetY=%d Width=%d Height=%d",
                    Source, Target,
                    SourceX, SourceY, TargetX,
                    TargetY, Width, Height);

    gcmGETHARDWARE(hardware);

    /* Get surface formats. */
    gcmONERROR(
        gcoSURF_QueryFormat(Source->format, srcFormat));

    gcmONERROR(
        gcoSURF_QueryFormat(Target->format, trgFormat));

    do
    {
        /***********************************************************************
        ** Fast path.
        */


        gctINT     x;
        gctINT     y;

        gctUINT32  srcTopBufferSize;
        gctUINT32  srcBankOffsetBytes = 0;
        gctPOINTER srcAddr0;
        gctPOINTER srcAddr1;

        gctUINT32  trgTopBufferSize;
        gctUINT32  trgBankOffsetBytes = 0;
        gctPOINTER trgAddr0;
        gctPOINTER trgAddr1;

        gctUINT    bytesPerPixel;

        gctBOOL    flip = gcvFALSE;

        gctBOOL    srcTiled;
        gctBOOL    srcMultiPipe;
        gctBOOL    trgTiled;
        gctBOOL    trgMultiPipe;

        if (
        (
        /* Fast path won't process multi-sampling. */
            (Source->samples.x > 1) || (Source->samples.y > 1) ||
        /* Wont's process different bitsPerPixel case. */
            (srcFormat[0]->bitsPerPixel != trgFormat[0]->bitsPerPixel) ||
        /* Wont's process format conversion. */
            ((Source->format != Target->format) &&
               !((Source->format  == gcvSURF_A8R8G8B8) &&
                (Target->format == gcvSURF_X8R8G8B8)) &&
               !((Source->format  == gcvSURF_A8B8G8R8) &&
                (Target->format == gcvSURF_X8B8G8R8)))
        ))
        {
            break;
        }

        /* Commit hardware commands. */
        gcoHARDWARE_Commit();

        /* Get shortcut. */
        bytesPerPixel = srcFormat[0]->bitsPerPixel / 8;

        srcTopBufferSize = gcmALIGN(Source->alignedHeight / 2,
                           (Source->superTiled) ? 64 : 4)
                         * Source->stride;

        trgTopBufferSize = gcmALIGN(Target->alignedHeight / 2,
                           (Target->superTiled) ? 64 : 4)
                         * Target->stride;

#if gcdENABLE_BANK_ALIGNMENT
        gcmVERIFY_OK(
            gcoSURF_GetBankOffsetBytes(gcvNULL,
                                     Source->type,
                                     srcTopBufferSize,
                                     &srcBankOffsetBytes
                                     ));

        gcmVERIFY_OK(
            gcoSURF_GetBankOffsetBytes(gcvNULL,
                                     Target->type,
                                     trgTopBufferSize,
                                     &trgBankOffsetBytes
                                     ));

        if (Source->size <
            (Source->alignedHeight * Source->stride + srcBankOffsetBytes))
        {
            srcBankOffsetBytes = 0;
        }

        if (Target->size <
            (Target->alignedHeight * Target->stride + trgBankOffsetBytes))
        {
            trgBankOffsetBytes = 0;
        }
#endif

        /* Lock source surface. */
        if (gcmIS_ERROR(gcoHARDWARE_Lock(&Source->node,
                                         gcvNULL,
                                         &srcAddr0)))
        {
            break;
        }

        /* Lock target surface. */
        if (gcmIS_ERROR(gcoHARDWARE_Lock(&Target->node,
                                         gcvNULL,
                                         &trgAddr0)))
        {
            gcmVERIFY_OK(
                gcoHARDWARE_Unlock(&Source->node,
                                   Source->type));

            break;
        }

        /* Flush surface cache. */
        gcmVERIFY_OK(
            gcoSURF_NODE_Cache(&Source->node,
                               Source->node.logical,
                               Source->size,
                               gcvCACHE_FLUSH));

        gcmVERIFY_OK(
            gcoSURF_NODE_Cache(&Target->node,
                               Target->node.logical,
                               Target->size,
                               gcvCACHE_FLUSH));

        /* Get flip. */
        if (Height < 0)
        {
            flip   = gcvTRUE;
            Height = -Height;
        }

        /* Get source tiling info. */
        if (Source->type == gcvSURF_BITMAP)
        {
            srcTiled     = gcvFALSE;
            srcMultiPipe = gcvFALSE;
        }
        else
        {
            srcTiled     = gcvTRUE;

            srcMultiPipe = (Source->tiling == gcvMULTI_TILED)
                        || (Source->tiling == gcvMULTI_SUPERTILED);
        }

        /* Get target tiling info. */
        if (Target->type == gcvSURF_BITMAP)
        {
            trgTiled     = gcvFALSE;
            trgMultiPipe = gcvFALSE;
        }
        else
        {
            trgTiled     = gcvTRUE;

            trgMultiPipe = (Target->tiling == gcvMULTI_TILED)
                        || (Target->tiling == gcvMULTI_SUPERTILED);
        }

        /* Compute source second address. */
        srcAddr1 = (gctPOINTER) (
                   (gctUINT8_PTR) srcAddr0
                       + srcBankOffsetBytes + srcTopBufferSize
                   );

        /* Compute dest second address. */
        trgAddr1 = (gctPOINTER) (
                   (gctUINT8_PTR) trgAddr0
                       + trgBankOffsetBytes + trgTopBufferSize
                   );

        /* Stall hardware and start copying. */
        gcoHARDWARE_Stall();

        for (y = 0; y < Height; y++)
        {
            for (x = 0; x < Width; x++)
            {
                gctUINT32 xt, yt;
                gctUINT32 xxt, yyt;

                gctPOINTER srcAddr;
                gctPOINTER trgAddr;

                gctUINT32 xl = SourceX + x;
                gctUINT32 yl = SourceY + y;

                if (srcTiled)
                {
                    if (srcMultiPipe)
                    {
                        /* X' = 15'{ X[14:4], Y[2], X[2:0] } */
                        xt = (xl & ~0x8) | ((yl & 0x4) << 1);
                        /* Y' = 15'{ 0, Y[14:3], Y[1:0] } */
                        yt = ((yl & ~0x7) >> 1) | (yl & 0x3);

                        /* Get base source address. */
                        srcAddr = (((xl >> 3) ^ (yl >> 2)) & 0x01) ? srcAddr1
                                : srcAddr0;
                    }
                    else
                    {
                        xt = x;
                        yt = y;

                        /* Get base source address. */
                        srcAddr = srcAddr0;
                    }

                    if (Source->superTiled)
                    {
                        /* coord = 21'{ X[21-1:6], Y[5:4],X[5:3],Y[3:2],X[2],Y[1:0],X[1:0] }. */
                        xxt = ((xt &  0x03) << 0) |
                              ((yt &  0x03) << 2) |
                              ((xt &  0x04) << 2) |
                              ((yt &  0x0C) << 3) |
                              ((xt &  0x38) << 4) |
                              ((yt &  0x30) << 6) |
                              ((xt & ~0x3F) << 6);

                        yyt = yt & ~0x3F;
                    }
                    else
                    {
                        xxt = ((xt &  0x03) << 0) +
                              ((yt &  0x03) << 2) +
                              ((xt & ~0x03) << 2);

                        yyt = yt & ~0x03;
                    }

                    srcAddr = (gctPOINTER) ((gctUINT8_PTR) srcAddr
                            + yyt * Source->stride
                            + xxt * bytesPerPixel);
                }
                else
                {
                    yl = flip ? Source->alignedHeight - yl - 1 : yl;

                    srcAddr = (gctPOINTER) ((gctUINT8_PTR) srcAddr0
                            + yl * Source->stride
                            + xl * bytesPerPixel);
                }

                xl = TargetX + x;
                yl = TargetY + y;

                if (trgTiled)
                {
                    if (trgMultiPipe)
                    {
                        /* X' = 15'{ X[14:4], Y[2], X[2:0] } */
                        xt = (xl & ~0x8) | ((yl & 0x4) << 1);
                        /* Y' = 15'{ 0, Y[14:3], Y[1:0] } */
                        yt = ((yl & ~0x7) >> 1) | (yl & 0x3);

                        /* Get base target address. */
                        trgAddr = (((xl >> 3) ^ (yl >> 2)) & 0x01) ? trgAddr1
                                : trgAddr0;
                    }
                    else
                    {
                        xt = x;
                        yt = y;

                        /* Get base source address. */
                        trgAddr = trgAddr0;
                    }

                    if (Target->superTiled)
                    {
                        /* coord = 21'{ X[21-1:6], Y[5:4],X[5:3],Y[3:2],X[2],Y[1:0],X[1:0] }. */
                        xxt = ((xt &  0x03) << 0) |
                              ((yt &  0x03) << 2) |
                              ((xt &  0x04) << 2) |
                              ((yt &  0x0C) << 3) |
                              ((xt &  0x38) << 4) |
                              ((yt &  0x30) << 6) |
                              ((xt & ~0x3F) << 6);

                        yyt = yt & ~0x3F;
                    }
                    else
                    {
                        xxt = ((xt &  0x03) << 0) +
                              ((yt &  0x03) << 2) +
                              ((xt & ~0x03) << 2);

                        yyt = yt & ~0x03;
                    }

                    trgAddr = (gctPOINTER) ((gctUINT8_PTR) trgAddr
                            + yyt * Target->stride
                            + xxt * bytesPerPixel);
                }
                else
                {
                    yl = flip ? Target->alignedHeight - yl - 1 : yl;

                    trgAddr = (gctPOINTER) ((gctUINT8_PTR) trgAddr0
                            + yl * Target->stride
                            + xl * bytesPerPixel);
                }

                switch (bytesPerPixel)
                {
                case 4:
                default:
                    *(gctUINT32_PTR) trgAddr = *(gctUINT32_PTR) srcAddr;
                    break;

                case 8:
                    *(gctUINT64_PTR) trgAddr = *(gctUINT64_PTR) srcAddr;
                    break;

                case 2:
                    *(gctUINT16_PTR) trgAddr = *(gctUINT16_PTR) srcAddr;
                    break;

                case 1:
                    *(gctUINT8_PTR)  trgAddr = *(gctUINT8_PTR)  srcAddr;
                    break;
                }
            }
        }

        gcmVERIFY_OK(
            gcoHARDWARE_Unlock(&Source->node,
                               Source->type));

        gcmVERIFY_OK(
            gcoHARDWARE_Unlock(&Target->node,
                               Target->type));

        /* Return status. */
        gcmFOOTER();
        return status;
    }
    while (gcvFALSE);


    /* Check the format the format to see if hw support*/
    if(gcmIS_ERROR(_ConvertResolveFormat(hardware,
                                         Source->format,
                                         &sFormat,
                                         &srcFlip)) &&
       (Target->type == gcvSURF_TEXTURE)
	   )
    {
       hwcopy = gcvFALSE;
	}

    /* Check if the source has multi-sampling or super-tiling. */
    if (
        /* Check whether the source is multi-sampled and is larger than the
        ** resolve size. */
        (((((Source->samples.x > 1) || (Source->samples.y > 1))
        && (Target->alignedWidth  / Source->samples.x >= hardware->resolveAlignmentX)
        && (Target->alignedHeight / Source->samples.y >= hardware->resolveAlignmentY)
        )

        /* Check whether the source is super tiled. */
        ||  Source->superTiled

        /* Check whether this is a multi-pixel render target or depth buffer. */
        ||  ((hardware->pixelPipes > 1)
            && ((Source->type == gcvSURF_RENDER_TARGET)
            ||  (Source->type == gcvSURF_DEPTH))
        )))
		&& hwcopy
    )
    {
        gcsPOINT zero, origin, size;
        gceORIENTATION orientation;
        gctUINT32 tileWidth, tileHeight;

        /* Find the tile aligned region to resolve. */
        if (Source->superTiled)
        {
            tileWidth = 64;
            tileHeight = 64 * hardware->pixelPipes;

            /* Align origin to top right. */
            origin.x = SourceX & ~(tileWidth - 1);
            origin.y = SourceY & ~(tileHeight - 1);

            /* Align size to end of bottom right tile corner. */
            size.x = gcmALIGN((SourceX + Width), tileWidth)
                     - origin.x;
            size.y = gcmALIGN((SourceY + Height), tileHeight)
                     - origin.y;
        }
        else
        {
            /* Align origin to top right. */
            origin.x = SourceX & (hardware->resolveAlignmentX - 1);
            origin.y = SourceY & (hardware->resolveAlignmentY - 1);

            /* Align size to end of bottom right tile corner. */
            size.x = gcmALIGN((SourceX + Width), hardware->resolveAlignmentX)
                     - origin.x;
            size.y = gcmALIGN((SourceY + Height), hardware->resolveAlignmentY)
                     - origin.y;
        }

        /* Create a temporary surface. */
        if (Target->type == gcvSURF_BITMAP)
        {
            gcmONERROR(
                gcoHARDWARE_AllocateTemporarySurface(hardware,
                                                 size.x,
                                                 size.y,
                                                 trgFormat[0],
                                                 gcvSURF_BITMAP));
        }
        else
        {
            gcmONERROR(
                gcoHARDWARE_AllocateTemporarySurface(hardware,
                                                 size.x,
                                                 size.y,
                                                 srcFormat[0],
                                                 gcvSURF_BITMAP));
        }

        /* Use same orientation as source. */
        orientation = hardware->tempBuffer.orientation;
        hardware->tempBuffer.orientation = Source->orientation;

        /* Lock it. */
        gcmONERROR(
            gcoHARDWARE_Lock(&hardware->tempBuffer.node,
                             gcvNULL,
                             &memory));

        /* Flush temporary surface cache, as it could be being re-used. */
        gcmONERROR(
            gcoSURF_NODE_Cache(&hardware->tempBuffer.node,
                             memory,
                             hardware->tempBuffer.size,
                             gcvCACHE_FLUSH));

        /* Resolve the source into the temporary surface. */
        zero.x = 0;
        zero.y = 0;
        gcmASSERT(size.x == hardware->tempBuffer.rect.right);
        gcmASSERT(size.y == hardware->tempBuffer.rect.bottom);

        gcmONERROR(
            _ResolveRect(hardware,
                         Source,
                         &hardware->tempBuffer,
                         &origin,
                         &zero,
                         &size));

        /* Stall the hardware. */
        gcmONERROR(
            gcoHARDWARE_Commit());

        gcmONERROR(
            gcoHARDWARE_Stall());

        stalled = gcvTRUE;

        /* Restore orientation. */
        hardware->tempBuffer.orientation = orientation;

        /* Change SourceX, SourceY, to be relative to tempBuffer. */
        SourceX -= origin.x;
        SourceY -= origin.y;

		if (Target->type == gcvSURF_BITMAP)
        {
            gctINT32 i;
            gctINT srcOffset, trgOffset, bpp;
            gctUINT8_PTR src;
            gctUINT8_PTR trg;

            bpp = trgFormat[0]->bitsPerPixel / 8;

            srcOffset = SourceY * hardware->tempBuffer.stride + SourceX * bpp;
            trgOffset = TargetY * Target->stride + TargetX * bpp;

            src = hardware->tempBuffer.node.logical + srcOffset;
            trg = Target->node.logical + trgOffset;

            if (((gctUINT)Target->rect.right == hardware->tempBuffer.alignedWidth) &&
                ((gctUINT)Target->rect.bottom == hardware->tempBuffer.alignedHeight))
            {
                gcoOS_MemCopy(trg, src, hardware->tempBuffer.alignedWidth *
										hardware->tempBuffer.alignedHeight * bpp);
            }
            else
            {
				gctINT copy_size = Width * bpp;
                for (i = 0; i < Height; i++)
                {
                    gcoOS_MemCopy(trg, src, copy_size);
					src += hardware->tempBuffer.stride;
					trg += Target->stride;
                }
            }

            if (memory != gcvNULL)
            {
                /* Unlock the temporay buffer. */
                gcmVERIFY_OK(
                    gcoHARDWARE_Unlock(&hardware->tempBuffer.node,
                                       hardware->tempBuffer.type));
            }

            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

#if !defined(VIVANTE_NO_3D)
        if (Target->type == gcvSURF_TEXTURE)
        {
            gctUINT32   srcOffset;
            gctUINT32   srcBitsPerPixel;

            gcmONERROR(
               gcoHARDWARE_ConvertFormat(Source->format,
                                         &srcBitsPerPixel,
                                         gcvNULL));


            srcOffset = (SourceY * hardware->tempBuffer.alignedWidth + SourceX)* (srcBitsPerPixel / 8);

            /* Try using faster UploadTexture function. */
            status = gcoHARDWARE_UploadTexture(Target->format,
                                               Target->node.physical,
                                               Target->node.logical,
                                               0,
                                               Target->stride,
                                               TargetX,
                                               TargetY,
                                               Width,
                                               Height,
                                               (gctPOINTER)((gctUINT32)memory + srcOffset),
                                               hardware->tempBuffer.stride,
                                               hardware->tempBuffer.format);

            if (status == gcvSTATUS_OK)
            {
                if (memory != gcvNULL)
                {
                    /* Unlock the temporay buffer. */
                    gcmVERIFY_OK(
                        gcoHARDWARE_Unlock(&hardware->tempBuffer.node,
                                           hardware->tempBuffer.type));
                }

                /* Success. */
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }
#endif

        /* Use temporary buffer as source. */
        source = &hardware->tempBuffer;
    }
    else
    {
        /* Use source as-is. */
        source = Source;
    }

    if (!stalled)
    {
        /* Synchronize with the GPU. */
        gcmONERROR(gcoHARDWARE_Commit());
        gcmONERROR(gcoHARDWARE_Stall());
    }

    do
    {
        /***********************************************************************
        ** Local variable definitions.
        */

        /* Tile dimensions. */
        gctINT32 srcTileWidth, srcTileHeight, srcTileSize;
        gctINT32 trgTileWidth, trgTileHeight, trgTileSize;

        /* Pixel sizes. */
        gctINT srcPixelSize, trgPixelSize;

        /* Walking boundaries. */
        gctINT trgX1, trgY1, trgX2, trgY2;
        gctINT srcX1, srcY1, srcX2, srcY2;

        /* Source step. */
        gctINT srcStepX, srcStepY;

        /* Coordinate alignment. */
        gctUINT srcAlignX, srcAlignY;
        gctUINT trgAlignX, trgAlignY;

        /* Pixel group sizes. */
        gctUINT srcGroupX, srcGroupY;
        gctUINT trgGroupX, trgGroupY;

        /* Line surface offsets. */
        gctUINT srcLineOffset, trgLineOffset;

        /* Counts to keep track of when to add long vs. short offsets. */
        gctINT srcLeftPixelCount, srcMidPixelCount;
        gctINT srcTopLineCount, srcMidLineCount;
        gctINT trgLeftPixelCount, trgMidPixelCount;
        gctINT trgTopLineCount, trgMidLineCount;
        gctINT srcPixelCount, trgPixelCount;
        gctINT srcLineCount, trgLineCount;

        /* Long and short offsets. */
        gctINT srcShortStride, srcLongStride;
        gctINT srcShortOffset, srcLongOffset;
        gctINT trgShortStride, trgLongStride;
        gctINT trgShortOffset, trgLongOffset;

        /* Direct copy flag and the intermediate format. */
        gctBOOL directCopy;
        gcsSURF_FORMAT_INFO_PTR intFormat[2];

        /* Boundary checking. */
        gcsBOUNDARY srcBoundary;
        gcsBOUNDARY trgBoundary;
        gcsBOUNDARY_PTR srcBoundaryPtr;
        gcsBOUNDARY_PTR trgBoundaryPtr;

        gctUINT loopCount = 0;
        /***********************************************************************
        ** Get surface format details.
        */

        /* Compute pixel sizes. */
        srcPixelSize = srcFormat[0]->bitsPerPixel / 8;
        trgPixelSize = trgFormat[0]->bitsPerPixel / 8;

        /***********************************************************************
        ** Validate inputs.
        */

        /* Verify that the surfaces are locked. */
        gcmVERIFY_LOCK(source);
        gcmVERIFY_LOCK(Target);

        /* Check support. */
        if (Width < 0)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        if ((srcFormat[0]->interleaved) && (source->samples.x != 1))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        if ((trgFormat[0]->interleaved) && (Target->samples.x != 1))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* FIXME:
         * Need support super tile, multi-tile, multi-supertile targets. */
        if ((Target->tiling == gcvSUPERTILED) ||
            (Target->tiling == gcvMULTI_TILED) ||
            (Target->tiling == gcvMULTI_SUPERTILED))
        {
            gcmPRINT("libGAL: Target tiling=%d not supported",
                     Target->tiling);

            /* Break. */
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /***********************************************************************
        ** Determine the type of operation and the intermediate format.
        */

        directCopy
            =  (source->samples.x == Target->samples.x)
            && (source->samples.y == Target->samples.y);

        if (!directCopy)
        {
            gceSURF_FORMAT intermediateFormat
                = (srcFormat[0]->fmtClass == gcvFORMAT_CLASS_RGBA)
                    ? gcvSURF_A8R8G8B8
                    : srcFormat[0]->format;

            gcmERR_BREAK(gcoSURF_QueryFormat(intermediateFormat, intFormat));
        }

        /***********************************************************************
        ** Determine the size of the pixel groups.
        */

        srcGroupX = (srcFormat[0]->interleaved || (source->samples.x != 1))
            ? 2
            : 1;

        trgGroupX = (trgFormat[0]->interleaved || (Target->samples.x != 1))
            ? 2
            : 1;

        srcGroupY = source->samples.y;
        trgGroupY = Target->samples.y;

        /* Determine coordinate alignments. */
        srcAlignX = ~(srcGroupX - 1);
        srcAlignY = ~(srcGroupY - 1);
        trgAlignX = ~(trgGroupX - 1);
        trgAlignY = ~(trgGroupY - 1);

        /***********************************************************************
        ** Determine tile parameters.
        */

        gcmERR_BREAK(gcoHARDWARE_GetSurfaceTileSize(
            source, &srcTileWidth, &srcTileHeight
            ));

        gcmERR_BREAK(gcoHARDWARE_GetSurfaceTileSize(
            Target, &trgTileWidth, &trgTileHeight
            ));

        srcTileSize = srcTileWidth * srcTileHeight * srcPixelSize;
        trgTileSize = trgTileWidth * trgTileHeight * trgPixelSize;

        /* Determine pixel and line counts per tile. */
        srcMidPixelCount = (srcTileWidth  == 1) ? 1 : srcTileWidth  / srcGroupX;
        srcMidLineCount  = (srcTileHeight == 1) ? 1 : srcTileHeight / srcGroupY;
        trgMidPixelCount = (trgTileWidth  == 1) ? 1 : trgTileWidth  / trgGroupX;
        trgMidLineCount  = (trgTileHeight == 1) ? 1 : trgTileHeight / trgGroupY;

        /***********************************************************************
        ** Determine the initial horizontal source coordinates.
        */

        srcX1    = source->samples.x *  SourceX;
        srcX2    = source->samples.x * (SourceX + Width);
        srcStepX = source->samples.x;

        /* Pixels left to go before using the long offset. */
        srcLeftPixelCount
            = ((~((gctUINT) srcX1)) & (srcTileWidth - 1)) / srcGroupX + 1;

        srcShortOffset
            = srcPixelSize * srcGroupX;

        srcLongOffset
            = srcTileSize - srcPixelSize * (srcTileWidth - srcGroupX);

        /***********************************************************************
        ** Determine the initial vertical source coordinates.
        */

        if (Height < 0)
        {
            srcY1    =            source->samples.y * (SourceY - Height - 1);
            srcY2    =            source->samples.y * (SourceY          - 1);
            srcStepY = - (gctINT) source->samples.y;

            /* Lines left to go before using the long stride. */
            srcTopLineCount
                = (((gctUINT) srcY1) & (srcTileHeight - 1)) / srcGroupY + 1;

            srcLongStride = - (gctINT) ((srcTileHeight == 1)
                ? source->stride * srcGroupY
                : source->stride * srcTileHeight
                  - srcTileWidth * srcPixelSize * (srcTileHeight - srcGroupY));

            srcShortStride = (srcTileHeight == 1)
                ? srcLongStride
                : - (gctINT) (srcTileWidth * srcPixelSize * srcGroupY);

            /* Determine the vertical target range. */
            trgY1 = Target->samples.y *  TargetY;
            trgY2 = Target->samples.y * (TargetY - Height);
        }
        else
        {
            srcY1    = source->samples.y *  SourceY;
            srcY2    = source->samples.y * (SourceY + Height);
            srcStepY = source->samples.y;

            /* Lines left to go before using the long stride. */
            srcTopLineCount
                = ((~((gctUINT) srcY1)) & (srcTileHeight - 1)) / srcGroupY + 1;

            srcLongStride = (srcTileHeight == 1)
                ? source->stride * srcGroupY
                : source->stride * srcTileHeight
                  - srcTileWidth * srcPixelSize * (srcTileHeight - srcGroupY);

            srcShortStride = (srcTileHeight == 1)
                ? srcLongStride
                : (gctINT) (srcTileWidth * srcPixelSize * srcGroupY);

            /* Determine the vertical target range. */
            trgY1 = Target->samples.y *  TargetY;
            trgY2 = Target->samples.y * (TargetY + Height);
        }

        /***********************************************************************
        ** Determine the initial target coordinates.
        */

        trgX1 = Target->samples.x *  TargetX;
        trgX2 = Target->samples.x * (TargetX + Width);

        /* Pixels left to go before using the long offset. */
        trgLeftPixelCount
            = ((~((gctUINT) trgX1)) & (trgTileWidth - 1)) / trgGroupX + 1;

        /* Lines left to go before using the long stride. */
        trgTopLineCount
            = ((~((gctUINT) trgY1)) & (trgTileHeight - 1)) / trgGroupY + 1;

        trgShortOffset = trgPixelSize * trgGroupX;
        trgLongOffset = trgTileSize - trgPixelSize * (trgTileWidth - trgGroupX);

        trgLongStride = (trgTileHeight == 1)
            ? Target->stride * trgGroupY
            : Target->stride * trgTileHeight
              - trgTileWidth * trgPixelSize * (trgTileHeight - trgGroupY);

        trgShortStride = (trgTileHeight == 1)
            ? trgLongStride
            : (gctINT) (trgTileWidth * trgPixelSize * trgGroupY);

        /***********************************************************************
        ** Setup the boundary checking.
        */

        if ((srcX1 < 0) || (srcX2 >= source->rect.right) ||
            (srcY1 < 0) || (srcY2 >= source->rect.bottom))
        {
            srcBoundaryPtr = &srcBoundary;
            srcBoundary.width  = source->rect.right;
            srcBoundary.height = source->rect.bottom;
        }
        else
        {
            srcBoundaryPtr = gcvNULL;
        }

        if ((trgX1 < 0) || (trgX2 > Target->rect.right) ||
            (trgY1 < 0) || (trgY2 > Target->rect.bottom))
        {
            trgBoundaryPtr = &trgBoundary;
            trgBoundary.width  = Target->rect.right;
            trgBoundary.height = Target->rect.bottom;
        }
        else
        {
            trgBoundaryPtr = gcvNULL;
        }

        /***********************************************************************
        ** Check the fast path for copy
        */
        if ((srcTileHeight == 1) && (trgTileHeight == 1) &&         /*no tile*/
            (source->samples.x == 1) && (source->samples.y == 1) && /*no multisample*/
            (Target->samples.x == 1) && (Target->samples.y == 1) &&
            ((source->format == Target->format) ||
              (source->format == gcvSURF_X8B8G8R8 && Target->format == gcvSURF_A8B8G8R8)))                     /*no format convertion*/
            fastPathCopy = gcvTRUE;

        /***********************************************************************
        ** Compute the source and target initial offsets.
        */

        srcLineOffset

            /* Skip full tile lines. */
            = ((gctINT) (srcY1 & srcAlignY & ~(srcTileHeight - 1)))
                * source->stride

            /* Skip full tiles in the same line. */
            + ((((gctINT) (srcX1 & srcAlignX & ~(srcTileWidth - 1)))
                * srcTileHeight * srcFormat[0]->bitsPerPixel) >> 3)

            /* Skip full rows within the target tile. */
            + ((((gctINT) (srcY1 & srcAlignY & (srcTileHeight - 1)))
                * srcTileWidth * srcFormat[0]->bitsPerPixel) >> 3)

            /* Skip pixels on the target row. */
            + (((gctINT) (srcX1 & srcAlignX & (srcTileWidth - 1)))
                * srcFormat[0]->bitsPerPixel >> 3);

        trgLineOffset

            /* Skip full tile lines. */
            = ((gctINT) (trgY1 & trgAlignY & ~(trgTileHeight - 1)))
                * Target->stride

            /* Skip full tiles in the same line. */
            + ((((gctINT) (trgX1 & trgAlignX & ~(trgTileWidth - 1)))
                * trgTileHeight * trgFormat[0]->bitsPerPixel) >> 3)

            /* Skip full rows within the target tile. */
            + ((((gctINT) (trgY1 & trgAlignY & (trgTileHeight - 1)))
                * trgTileWidth * trgFormat[0]->bitsPerPixel) >> 3)

            /* Skip pixels on the target row. */
            + (((gctINT) (trgX1 & trgAlignX & (trgTileWidth - 1)))
                * trgFormat[0]->bitsPerPixel >> 3);

        /* Initialize line counts. */
        srcLineCount = srcTopLineCount;
        trgLineCount = trgTopLineCount;

        /* Initialize the vertical coordinates. */
        srcBoundary.y = srcY1;
        trgBoundary.y = trgY1;

        /* Go through all lines. */
        while ((srcBoundary.y != srcY2) && (loopCount < MAX_LOOP_COUNT))
        {
            gctUINT loopCountA = 0;

            /* Initialize the initial pixel addresses. */
            gctUINT8_PTR srcPixelAddress = source->node.logical + srcLineOffset;
            gctUINT8_PTR trgPixelAddress = Target->node.logical + trgLineOffset;

            loopCount++;

            if (fastPathCopy)
            {
                gctINT bytePerLine = (srcX2 - srcX1) * srcPixelSize;

                gcoOS_MemCopy(trgPixelAddress, srcPixelAddress, bytePerLine);

                srcLineOffset += srcLongStride;
                trgLineOffset += trgLongStride;
                srcBoundary.y += srcStepY;

                continue;
            }

            /* Initialize pixel counts. */
            srcPixelCount = srcLeftPixelCount;
            trgPixelCount = trgLeftPixelCount;

            /* Initialize the horizontal coordinates. */
            srcBoundary.x = srcX1;
            trgBoundary.x = trgX1;

            /* Go through all columns. */
            while ((srcBoundary.x != srcX2) && (loopCountA < MAX_LOOP_COUNT))
            {
                /* Determine oddity of the pixels. */
                gctINT srcOdd = srcBoundary.x & srcFormat[0]->interleaved;
                gctINT trgOdd = trgBoundary.x & trgFormat[0]->interleaved;

                loopCountA++;
                      /* Direct copy without resampling. */
                if (directCopy)
                {
                    gctUINT8 x, y;

                    for (y = 0; y < source->samples.y; y++)
                    {
                        /* Determine the vertical pixel offsets. */
                        gctUINT srcVerOffset = y * srcTileWidth * srcPixelSize;
                        gctUINT trgVerOffset = y * trgTileWidth * trgPixelSize;

                        for (x = 0; x < source->samples.x; x++)
                        {
                            /* Determine the horizontal pixel offsets. */
                            gctUINT srcHorOffset = x * srcPixelSize;
                            gctUINT trgHorOffset = x * trgPixelSize;

                            /* Convert pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                srcPixelAddress + srcVerOffset + srcHorOffset,
                                trgPixelAddress + trgVerOffset + trgHorOffset,
                                0, 0,
                                srcFormat[srcOdd],
                                trgFormat[trgOdd],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }

                        /* Error? */
                        if (gcmIS_ERROR(status))
                        {
                            break;
                        }
                    }

                    /* Error? */
                    if (gcmIS_ERROR(status))
                    {
                        break;
                    }
                }

                /* Need to resample. */
                else
                {
                    gctUINT32 data[4];

                    /* Read the top left pixel. */
                    gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                        srcPixelAddress,
                        &data[0],
                        0, 0,
                        srcFormat[0],
                        intFormat[0],
                        srcBoundaryPtr,
                        trgBoundaryPtr
                        ));

                    /* Replicate horizotnally? */
                    if (source->samples.x == 1)
                    {
                        data[1] = data[0];

                        /* Replicate vertically as well? */
                        if (source->samples.y == 1)
                        {
                            data[2] = data[0];
                        }
                        else
                        {
                            /* Read the bottom left pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                srcPixelAddress + srcTileWidth * srcPixelSize,
                                &data[2],
                                0, 0,
                                srcFormat[0],
                                intFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }

                        /* Replicate the bottom right. */
                        data[3] = data[2];
                    }

                    else
                    {
                        /* Read the top right pixel. */
                        gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                            srcPixelAddress + srcPixelSize,
                            &data[1],
                            0, 0,
                            srcFormat[0],
                            intFormat[0],
                            srcBoundaryPtr,
                            trgBoundaryPtr
                            ));

                        /* Replicate vertically as well? */
                        if (source->samples.y == 1)
                        {
                            data[2] = data[0];
                            data[3] = data[1];
                        }
                        else
                        {
                            /* Read the bottom left pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                srcPixelAddress + srcTileWidth * srcPixelSize,
                                &data[2],
                                0, 0,
                                srcFormat[0],
                                intFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));

                            /* Read the bottom right pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                srcPixelAddress + (srcTileWidth + 1) * srcPixelSize,
                                &data[3],
                                0, 0,
                                srcFormat[0],
                                intFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }
                    }

                    /* Compute the destination. */
                    if (Target->samples.x == 1)
                    {
                        if (Target->samples.y == 1)
                        {
                            /* Average four sources. */
                            gctUINT32 dstPixel = _Average4Colors(
                                data[0], data[1], data[2], data[3]
                                );

                            /* Convert pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                &dstPixel,
                                trgPixelAddress,
                                0, 0,
                                intFormat[0],
                                trgFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }
                        else
                        {
                            /* Average two horizontal pairs of sources. */
                            gctUINT32 dstTop = _Average2Colors(
                                data[0], data[1]
                                );

                            gctUINT32 dstBottom = _Average2Colors(
                                data[2], data[3]
                                );

                            /* Convert the top pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                &dstTop,
                                trgPixelAddress,
                                0, 0,
                                intFormat[0],
                                trgFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));

                            /* Convert the bottom pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                &dstBottom,
                                trgPixelAddress + trgTileWidth * trgPixelSize,
                                0, 0,
                                intFormat[0],
                                trgFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }
                    }
                    else
                    {
                        if (Target->samples.y == 1)
                        {
                            /* Average two vertical pairs of sources. */
                            gctUINT32 dstLeft = _Average2Colors(
                                data[0], data[2]
                                );

                            gctUINT32 dstRight = _Average2Colors(
                                data[1], data[3]
                                );

                            /* Convert the left pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                &dstLeft,
                                trgPixelAddress,
                                0, 0,
                                intFormat[0],
                                trgFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));

                            /* Convert the right pixel. */
                            gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                &dstRight,
                                trgPixelAddress + trgPixelSize,
                                0, 0,
                                intFormat[0],
                                trgFormat[0],
                                srcBoundaryPtr,
                                trgBoundaryPtr
                                ));
                        }
                        else
                        {
                            /* Copy four sources as they are. */
                            gctUINT8 x, y;

                            for (y = 0; y < 2; y++)
                            {
                                /* Determine the vertical pixel offset. */
                                gctUINT trgVerOffset = y * trgTileWidth * trgPixelSize;

                                for (x = 0; x < 2; x++)
                                {
                                    /* Determine the horizontal pixel offset. */
                                    gctUINT trgHorOffset = x * trgPixelSize;

                                    /* Convert pixel. */
                                    gcmERR_BREAK(gcoHARDWARE_ConvertPixel(
                                        &data[x + y * 2],
                                        trgPixelAddress + trgVerOffset + trgHorOffset,
                                        0, 0,
                                        intFormat[0],
                                        trgFormat[0],
                                        srcBoundaryPtr,
                                        trgBoundaryPtr
                                        ));
                                }

                                /* Error? */
                                if (gcmIS_ERROR(status))
                                {
                                    break;
                                }
                            }

                            /* Error? */
                            if (gcmIS_ERROR(status))
                            {
                                break;
                            }
                        }
                    }
                }

                /* Advance to the next column. */
                if (srcOdd || !srcFormat[0]->interleaved)
                {
                    if (--srcPixelCount == 0)
                    {
                        srcPixelAddress += srcLongOffset;
                        srcPixelCount    = srcMidPixelCount;
                    }
                    else
                    {
                        srcPixelAddress += srcShortOffset;
                    }
                }

                if (trgOdd || !trgFormat[0]->interleaved)
                {
                    if (--trgPixelCount == 0)
                    {
                        trgPixelAddress += trgLongOffset;
                        trgPixelCount    = trgMidPixelCount;
                    }
                    else
                    {
                        trgPixelAddress += trgShortOffset;
                    }
                }

                srcBoundary.x += srcStepX;
                trgBoundary.x += Target->samples.x;
            }

            /* Error? */
            if (gcmIS_ERROR(status))
            {
                break;
            }

            /* Advance to the next line. */
            if (--srcLineCount == 0)
            {
                srcLineOffset += srcLongStride;
                srcLineCount   = srcMidLineCount;
            }
            else
            {
                srcLineOffset += srcShortStride;
            }

            if (--trgLineCount == 0)
            {
                trgLineOffset += trgLongStride;
                trgLineCount   = trgMidLineCount;
            }
            else
            {
                trgLineOffset += trgShortStride;
            }

            srcBoundary.y += srcStepY;
            trgBoundary.y += Target->samples.y;
        }
    }
    while (gcvFALSE);

#if gcdDUMP
    if (Target->node.pool == gcvPOOL_USER)
    {
        gctUINT offset, bytes, size;
        gctUINT step;

        if (source->type == gcvSURF_BITMAP)
        {
            int bpp = srcFormat[0]->bitsPerPixel / 8;
            offset  = SourceY * source->stride + SourceX * bpp;
            size    = Width * gcmABS(Height) * bpp;
            bytes   = Width * bpp;
            step    = source->stride;

            if (bytes == step)
            {
                bytes *= gcmABS(Height);
                step   = 0;
            }
        }
        else
        {
            offset = 0;
            size   = source->stride * source->alignedHeight;
            bytes  = size;
            step   = 0;
        }

        while (size > 0)
        {
            gcmDUMP_BUFFER(gcvNULL,
                           "verify",
                           source->node.physical,
                           source->node.logical,
                           offset,
                           bytes);

            offset += step;
            size   -= bytes;
        }
    }
    else
    {
        gcmDUMP_BUFFER(gcvNULL,
                       "memory",
                       Target->node.physical,
                       Target->node.logical,
                       0,
                       Target->size);
    }
#endif

OnError:
    if ((hardware != gcvNULL)
     && (memory != gcvNULL))
    {
        /* Unlock the temporary buffer. */
        gcmVERIFY_OK(
            gcoHARDWARE_Unlock(&hardware->tempBuffer.node,
                               hardware->tempBuffer.type));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHARDWARE_FlushPipe
**
**  Flush the current graphics pipe.
**
**  INPUT:
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_FlushPipe(
    )
{
    gceSTATUS status;
    gctUINT32 flush;
    gctBOOL flushFixed;
    gcoHARDWARE hardware;

    /* Define state buffer variables. */
    gcmDEFINESECUREUSER()
    gctSIZE_T reserveSize;
    gcoCMDBUF reserve;
    gctUINT32_PTR memory;

    gcmHEADER();

    gcmGETHARDWARE(hardware);

    if (hardware->currentPipe == 0x1)
    {
        /* Flush 2D cache. */
        flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));
    }
    else
    {
        /* Flush Z and Color caches. */
        flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) |
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));
    }

    /* RTL bug in older chips - flush leaks the next state.
       There is no actial bit for the fix. */
    flushFixed = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 31:31) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))));

    /* FLUSH_HACK */
    flushFixed = gcvFALSE;

    /* Determine the size of the buffer to reserve. */
    if (flushFixed)
    {
        reserveSize = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
    }
    else
    {
        reserveSize = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
    }

    /* Reserve space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        hardware->buffer, reserveSize, gcvTRUE, &reserve
        ));
    memory = (gctUINT32_PTR) reserve->lastReserve;
    /*stateDelta = Hardware->delta;*/
    gcmBEGINSECUREUSER();

        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, flush );gcmENDSTATEBATCH(reserve, memory);};

        if (!flushFixed)
        {
            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, flush );gcmENDSTATEBATCH(reserve, memory);};
        }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

#ifndef VIVANTE_NO_3D
    if (hardware->chipModel == gcv700
#if gcd6000_SUPPORT
        || 1
#endif
        )
    {
        /* Flush the L2 cache. */
        gcmONERROR(gcoHARDWARE_FlushL2Cache(hardware));
    }
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !defined(VIVANTE_NO_3D)
/*******************************************************************************
**
**  gcoHARDWARE_Semaphore
**
**  Send sempahore and stall until sempahore is signalled.
**
**  INPUT:
**
**      gceWHERE From
**          Semaphore source.
**
**      GCHWERE To
**          Sempahore destination.
**
**      gceHOW How
**          What to do.  Can be a one of the following values:
**
**              gcvHOW_SEMAPHORE            Send sempahore.
**              gcvHOW_STALL                Stall.
**              gcvHOW_SEMAPHORE_STALL  Send semaphore and stall.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_Semaphore(
    IN gceWHERE From,
    IN gceWHERE To,
    IN gceHOW How
    )
{
    gceSTATUS status;
    gctUINT32_PTR memory;
    gctBOOL semaphore, stall;
    gctUINT32 source, destination;
    gcoCMDBUF reserve;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("From=%d To=%d How=%d",
                  From, To, How);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    semaphore = (How == gcvHOW_SEMAPHORE) || (How == gcvHOW_SEMAPHORE_STALL);
    stall     = (How == gcvHOW_STALL)     || (How == gcvHOW_SEMAPHORE_STALL);

    /* Special case for RASTER to PIEL_ENGINE semaphores. */
    if ((From == gcvWHERE_RASTER) && (To == gcvWHERE_PIXEL))
    {
        if (How == gcvHOW_SEMAPHORE)
        {
            /* Set flag so we can issue a semaphore/stall when required. */
            hardware->stallPrimitive = gcvTRUE;

            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (How == gcvHOW_STALL)
        {
            /* Make sure we do a semaphore/stall here. */
            semaphore = gcvTRUE;
            stall     = gcvTRUE;
        }
    }

    switch (From)
    {
    case gcvWHERE_COMMAND:
        source = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));
        break;

    case gcvWHERE_RASTER:
        source = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x05 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));

        /* We need to stall on the next primitive if this is only a
           semaphore. */
        hardware->stallPrimitive = (How == gcvHOW_SEMAPHORE);
        break;

    default:
        gcmASSERT(gcvFALSE);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmASSERT(To == gcvWHERE_PIXEL);
    destination = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

    /* Reserve command buffer space. */
    gcmONERROR(gcoBUFFER_Reserve(
        hardware->buffer,
        ((semaphore & stall) ? 2 : 1) * 8,
        gcvTRUE,
        &reserve
        ));

    /* Cast the reserve pointer. */
    memory = (gctUINT32_PTR) reserve->lastReserve;

    if (semaphore)
    {
        /* Send sempahore token. */
        memory[0]
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        memory[1]
            = source
            | destination;

        /* Dump the semaphore. */
        gcmDUMP(gcvNULL, "@[semaphore 0x%08X 0x%08X]", source, destination);

        /* Point to stall command is required. */
        memory += 2;
    }

    if (stall)
    {
        if (From == gcvWHERE_COMMAND)
        {
            /* Stall command processor until semaphore is signalled. */
            memory[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
        }
        else
        {
            /* Send stall token. */
            memory[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0F00) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
        }

        memory[1]
            = source
            | destination;

        /* Dump the stall. */
        gcmDUMP(gcvNULL, "@[stall 0x%08X 0x%08X]", source, destination);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS gcoHARDWARE_PauseTileStatus(
    IN gcoHARDWARE Hardware,
    IN gceTILE_STATUS_CONTROL Control
    )
{
    gceSTATUS status;
    gctUINT32 config;

    gcmHEADER_ARG("Hardware=0x%x Control=%d", Hardware, Control);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Determine configuration. */
    config = (Control == gcvTILE_STATUS_PAUSE) ? 0
                                               : Hardware->memoryConfig;

    gcmONERROR(gcoHARDWARE_SelectPipe(Hardware, gcvPIPE_3D));

    gcmONERROR(gcoHARDWARE_FlushPipe());

    gcmONERROR(_LoadStates(
        0x0595, gcvFALSE, 1, 0, &config
        ));

    Hardware->paused = (Control == gcvTILE_STATUS_PAUSE);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_GetCurrentSurface
**
**  Get the Surface to current surface in hardware.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR * Surface
**          Ptr to current surface info ptr.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_GetCurrentSurface(
    OUT gcsSURF_INFO_PTR *Surface
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    *Surface = hardware->colorStates.surface;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_SetCurrentSurface
**
**  Set the Surface to current surface in hardware.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR Surface
**          Surface info.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SetCurrentSurface(
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;
    gcmHEADER_ARG("Surface=0x%x", Surface);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    hardware->colorStates.surface = Surface;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_SetTileStatus
**
**  Handle the tile status config based on the Surface config.
**
**  INPUT:
**
**      gcsSURF_INFO_PTR Surface
**          Surface info.
**
**      gctUINT32 TileStatusAddress
**          Tile Status buffer address.
**
**      gcsSURF_NODE_PTR HzTileStatus
**          Hz tile status node:
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SetTileStatus(
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 TileStatusAddress,
    IN gcsSURF_NODE_PTR HzTileStatus
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x TileStatusAddress=0x%08x "
                  "HzTileStatus=0x%x",
                  Surface, TileStatusAddress, HzTileStatus);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->tileStatusDisabled)
    {
        /* This surface has the tile status disabled - don't turn it on. */
        gcmONERROR(gcoHARDWARE_DisableTileStatus(Surface, gcvFALSE));
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    else
    {
        gcmONERROR(gcoHARDWARE_EnableTileStatus(Surface, TileStatusAddress, HzTileStatus));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_EnableTileStatus(
    IN gcsSURF_INFO_PTR Surface,
    IN gctUINT32 TileStatusAddress,
    IN gcsSURF_NODE_PTR HzTileStatus
    )
{
    gceSTATUS status;
    gctBOOL hasCompression;
    gctBOOL enableCmpression = gcvFALSE;
    gctBOOL autoDisable, correctAutoDisableCount;
    gctBOOL hierarchicalZ = gcvFALSE;
    gcsSURF_FORMAT_INFO_PTR info[2];
    gctUINT32 tileCount;
    gctUINT32 physicalBaseAddress;
    int format = -1;
    gcoHARDWARE hardware;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Surface=0x%x TileStatusAddress=0x%08x "
                  "HzTileStatus=0x%x",
                  Surface, TileStatusAddress, HzTileStatus);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->tileStatusDisabled || (TileStatusAddress == 0))
    {
        /* This surface has the tile status disabled - disable tile status. */
        status = _DisableTileStatus(hardware, Surface);

        gcmFOOTER();
        return status;
    }

    hasCompression = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));

    autoDisable = ((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));

    /* Verify that the surface is locked. */
    gcmVERIFY_LOCK(Surface);

    /* Convert the format into bits per pixel. */
    gcmONERROR(gcoSURF_QueryFormat(Surface->format, info));

    if ((info[0]->bitsPerPixel  == 16)
    &&  (hardware->chipModel    == gcv500)
    &&  (hardware->chipRevision < 3)
    )
    {
        /* 32-bytes per tile. */
        tileCount = Surface->size / 32;
    }
    else
    {
        /* 64-bytes per tile. */
        tileCount = Surface->size / 64;
    }

    correctAutoDisableCount = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));

    /* Disable autoDisable, if tileCount is too large. */
    if (!correctAutoDisableCount && (tileCount >= (1 << 20)))
    {
        autoDisable = gcvFALSE;
    }

    /* Determine the common (depth and pixel) reserve size. */
    reserveSize

        /* FLUSH_HACK */
        = gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2

        /* Fast clear registers. */
        + gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8)

        /* Memory configuration register. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

    if (Surface->type == gcvSURF_DEPTH)
    {
        if (Surface != hardware->depthStates.surface)
        {
            /* Depth buffer is not current - no need to enable. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (!hasCompression && autoDisable)
        {
            /* Add a state for surface auto disable counter. */
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }

        /* Determine whether hierarchical Z is supported. */
        hierarchicalZ = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 27:27) & ((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1)))))));

        if (hierarchicalZ)
        {
            /* Add three states for hierarchical Z. */
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 3, 8);
        }
    }
    else
    {
        if (Surface != hardware->colorStates.surface)
        {
            /* Render target is not current - no need to enable. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Determine color compression format. */
        switch (Surface->format)
        {
        case gcvSURF_A4R4G4B4:
        case gcvSURF_X4R4G4B4:
            format = 0x0;
            break;

        case gcvSURF_A1R5G5B5:
        case gcvSURF_X1R5G5B5:
            format = 0x1;
            break;

        case gcvSURF_R5G6B5:
            format = 0x2;
            break;

        case gcvSURF_X8R8G8B8:
            format = Surface->vaa
                   ? 0x0
                   : (hardware->chipRevision < 0x4500)
                   ? 0x3
                   : 0x4;
            break;

        case gcvSURF_A8R8G8B8:
            format = 0x3;
            break;

        default:
            format = -1;
        }

        enableCmpression =
        (
            hasCompression
        && (Surface->samples.x * Surface->samples.y > 1)
        && (format != -1)
        );

        if (!enableCmpression && autoDisable)
        {
            /* Add a state for surface auto disable counter. */
            reserveSize += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
        }
    }

#if gcdSECURE_USER
    physicalBaseAddress = 0;
#else
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20) == gcvSTATUS_TRUE)
    {
        physicalBaseAddress = 0;
    }
    else
    {
        gcmONERROR(gcoOS_GetBaseAddress(gcvNULL, &physicalBaseAddress));
    }
#endif

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(hardware, reserve, stateDelta, memory, reserveSize);

    /* Check for depth surfaces. */
    if (Surface->type == gcvSURF_DEPTH)
    {
        /* Flush the depth cache. */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

        /* FLUSH_HACK */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0xFFFF, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0xFFFF) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0xFFFF, 0);gcmENDSTATEBATCH(reserve, memory);};

        /* Enable fast clear. */
        hardware->memoryConfig = ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));

        /* Set depth format. */
        hardware->memoryConfig = ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (info[0]->bitsPerPixel == 16) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

        if (hasCompression)
        {
            /* Enable compression. */
            hardware->memoryConfig =
                ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        }
        else if (autoDisable)

        {
            /* Automatically disable fast clear. */
            hardware->memoryConfig =
                ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

            /* Program surface auto disable counter. */
            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x059C, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x059C) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x059C, tileCount );     gcmENDSTATEBATCH(reserve, memory);};
        }

        /* Save physical tile status address. */
        hardware->physicalTileDepth = TileStatusAddress;

        /* Hierarchical Z. */
        if (hierarchicalZ)
        {
            gctBOOL hasHz = (HzTileStatus->pool != gcvPOOL_UNKNOWN);

            hardware->memoryConfig = ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (hasHz) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));

            hardware->memoryConfig = ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13))) | (((gctUINT32) ((gctUINT32) (hasHz) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1))))))) << (0 ? 13:13)));

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05A9, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05A9) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05A9, HzTileStatus->physical + physicalBaseAddress );     gcmENDSTATEBATCH(reserve, memory);};

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05AB, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05AB) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05AB, Surface->hzNode.size / 16 );     gcmENDSTATEBATCH(reserve, memory);};

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05AA, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05AA) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x05AA, Surface->clearValueHz );     gcmENDSTATEBATCH(reserve, memory);};
        }

        /* Program fast clear registers. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0599, 3 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0599) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            /* Program tile status base address register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0599,
                TileStatusAddress + physicalBaseAddress
                );

            /* Program surface base address register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x059A,
                Surface->node.physical + physicalBaseAddress
                );

            /* Program clear value register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x059B,
                Surface->clearValue
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );
    }

    /* Render target surfaces. */
    else
    {
        /* Flush the color cache. */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0E03, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0E03, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) );gcmENDSTATEBATCH(reserve, memory);};

        /* FLUSH_HACK */
        {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0xFFFF, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0xFFFF) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0xFFFF, 0);gcmENDSTATEBATCH(reserve, memory);};

        /* Enable fast clear. */
        hardware->memoryConfig = ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));

        if (enableCmpression)
        {
            /* Turn on color compression when in MSAA mode. */
            hardware->memoryConfig =
                ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)));

            hardware->memoryConfig =
                ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)));

            /* Color compression. */
            hardware->colorStates.colorCompression = gcvTRUE;
            hardware->colorConfigDirty             = gcvTRUE;
        }
        else
        {
            /* No color compression. */
            hardware->colorStates.colorCompression = gcvFALSE;
            hardware->colorConfigDirty             = gcvTRUE;

            if (autoDisable)
            {
                /* Automatically disable fast clear. */
                hardware->memoryConfig =
                    ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));

                /* Program surface auto disable counter. */
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x059D, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x059D) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x059D, tileCount );     gcmENDSTATEBATCH(reserve, memory);};
            }
        }

        /* Program fast clear registers. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 3 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            /* Program tile status base address register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0596,
                TileStatusAddress + physicalBaseAddress
                );

            /* Program surface base address register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0597,
                Surface->node.physical + physicalBaseAddress
                );

            /* Program clear value register. */
            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0598,
                Surface->clearValue
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Save physical tile status address. */
        hardware->physicalTileColor = TileStatusAddress;
    }

    /* Program memory configuration register. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0595, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0595) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0595, hardware->memoryConfig );     gcmENDSTATEBATCH(reserve, memory);};

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FillFromTileStatus(
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
    gctUINT32 physicalBaseAddress, tileCount;
    gcoHARDWARE hardware;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Surface=0x%x",
                  Surface);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Nothing to do when there is no fast clear. */
    if (!(((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 0:0)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) )
    )
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Nothing to do when there is no fast clear. */
    if (!gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_TILE_FILLER) == gcvSTATUS_TRUE
    )
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }

    if ((Surface->size == 0)
     || ((Surface->size & 63) != 0)
     || (Surface->node.physical == ~0U)
     || (Surface->tileStatusNode.physical == ~0U))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

#if gcdSECURE_USER
    physicalBaseAddress = 0;
#else
    /* Get physical base address. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20) == gcvSTATUS_TRUE)
    {
        physicalBaseAddress = 0;
    }
    else
    {
        gcmONERROR(gcoOS_GetBaseAddress(gcvNULL, &physicalBaseAddress));
    }
#endif

    /* Program TileFiller. */
    tileCount = Surface->size / 64;

    /* Determine the size of the buffer to reserve. */
    reserveSize

        /* Tile status states. */
        = gcmALIGN((1 + 3) * gcmSIZEOF(gctUINT32), 8)

        /* Trigger state. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(hardware, reserve, stateDelta, memory, reserveSize);

    /* Reprogram tile status. */
    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 3 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (3 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0596,
            Surface->tileStatusNode.physical + physicalBaseAddress
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0597,
            Surface->node.physical + physicalBaseAddress
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0598,
            Surface->clearValue
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Trigger the fill. */
    {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05AC, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05AC) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x05AC, tileCount );gcmENDSTATEBATCH(reserve, memory);};

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_DisableTileStatus(
    IN gcsSURF_INFO_PTR Surface,
    IN gctBOOL CpuAccess
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Surface=0x%x CpuAccess=%d",
                  Surface, CpuAccess);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Surface != gcvNULL);

    if (Surface->type == gcvSURF_DEPTH)
    {
        if (Surface != hardware->depthStates.surface)
        {
            /* The surface has not yet been detiled. So we need to do that
            ** now. */
            if (CpuAccess
            &&  !Surface->tileStatusDisabled
            &&  (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
            )
            {
                /* Resolve the surface on itself. */
                gcmONERROR(gcoHARDWARE_FlushTileStatus(
                    Surface,
                    CpuAccess));
            }

            /* Disable the tile status for this surface. */
            Surface->tileStatusDisabled = CpuAccess;

            /* Depth buffer is not current - no need to disable. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }
    else
    {
        if (hardware->colorStates.surface != Surface)
        {
            /* The surface has not yet been detiled. So we need to do that
            ** now. */
            if (CpuAccess
            &&  !Surface->tileStatusDisabled
            &&  (Surface->tileStatusNode.pool != gcvPOOL_UNKNOWN)
            )
            {
                /* Save the current surface. */
                gcsSURF_INFO_PTR saved = hardware->colorStates.surface;

                /* Start working in the requested surface. */
                hardware->colorStates.surface = Surface;

                /* Enable the tile status for the requested surface. */
                status = gcoHARDWARE_EnableTileStatus(
                    hardware->colorStates.surface,
                    Surface->tileStatusNode.physical,
                    &Surface->hzTileStatusNode);

                if (gcmIS_SUCCESS(status))
                {
                    /* Now we can properly disable the tile status for this
                    ** surface. */
                    status = gcoHARDWARE_DisableTileStatus(Surface,
                                                           CpuAccess);
                }

                /* Restore the original surface. */
                hardware->colorStates.surface = saved;

                if (saved != gcvNULL)
                {
                    /* Reprogram the tile status to the current surface. */
                    gcmVERIFY_OK(gcoHARDWARE_EnableTileStatus(
                        saved,
                        (saved->tileStatusNode.pool != gcvPOOL_UNKNOWN) ?
                        saved->tileStatusNode.physical : 0,
                        &saved->hzTileStatusNode));
                }

                /* In case of an error. */
                gcmONERROR(status);
            }

            /* Disable the tile status for this surface. */
            Surface->tileStatusDisabled = CpuAccess;

            /* Render target is not current - no need to disable. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    if (Surface->tileStatusDisabled)
    {
        /* This surface already has the tile status disabled. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Make sure the tile status cache is flushed. */
    gcmONERROR(gcoHARDWARE_FlushTileStatus(Surface, CpuAccess));

    /* Finally program the registers. */
    gcmONERROR(_DisableTileStatus(hardware, Surface));

    /* Disable the tile status for this surface. */
    Surface->tileStatusDisabled = CpuAccess;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
_FlushTileStatusCache(
    IN gcoHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Surface
    )
{
    gceSTATUS status;
    gctUINT32 physical[3] = {0};
    gctINT stride;
    gctPOINTER logical[3] = {gcvNULL};
    gctUINT32 format;
    gctUINT32 physicalBaseAddress;
    gcsPOINT rectSize;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Hardware=0x%x  Surface=0x%x", Hardware, Surface);

    /* Get physical base address. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20))
    {
        physicalBaseAddress = 0;
    }
    else
    {
        gcmONERROR(gcoOS_GetBaseAddress(gcvNULL, &physicalBaseAddress));
    }

    if (Hardware->tempSurface == gcvNULL)
    {
        /* Construct a temporary surface. */
        gcmONERROR(gcoSURF_Construct(gcvNULL,
                                     64, 64, 1,
                                     gcvSURF_RENDER_TARGET,
                                     gcvSURF_A8R8G8B8,
                                     gcvPOOL_DEFAULT,
                                     &Hardware->tempSurface));
    }

    /* Set rect size as 16x8. */
    rectSize.x = 16;
    rectSize.y =  8;

    /* Lock the temporarty surface. */
    gcmONERROR(gcoSURF_Lock(Hardware->tempSurface, physical, logical));

    /* Get the stride of the temporary surface. */
    gcmONERROR(gcoSURF_GetAlignedSize(Hardware->tempSurface,
                                      gcvNULL, gcvNULL,
                                      &stride));

    /* Convert the source format. */
    gcmONERROR(_ConvertResolveFormat(Hardware,
                                     Hardware->tempSurface->info.format,
                                     &format,
                                     gcvNULL));

    /* Determine the size of the buffer to reserve. */
    reserveSize

        /* Tile status states. */
        = gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

        /* Tile status cache flush states. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8)

        /* Ungrouped resolve states. */
        + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 6;

    if (Hardware->pixelPipes == 2)
    {
        reserveSize

            /* Source address states. */
            += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

            /* Target address states. */
            +  gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

            /* Offset address states. */
            +  gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
    }
    else
    {
        reserveSize

            /* Source and target address states. */
            += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8) * 2;
    }

    if (Hardware->colorStates.surface != gcvNULL)
    {
        /* Add tile status states. */
        reserveSize += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8);
    }

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(Hardware, reserve, stateDelta, memory, reserveSize);

    /* Reprogram tile status. */
    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0596,
            Hardware->tempSurface->info.tileStatusNode.physical + physicalBaseAddress
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0597,
            Hardware->tempSurface->info.node.physical + physicalBaseAddress
            );

        gcmSETFILLER(
            reserve, memory
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Flush the tile status. */
    {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0594, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

    /* Append RESOLVE_CONFIG state. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0581, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0581, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 6:5) - (0 ? 6:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:5) - (0 ? 6:5) + 1))))))) << (0 ? 6:5))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) );     gcmENDSTATEBATCH(reserve, memory);};

    /* Set source and destination stride. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0583, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0583) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0583, stride << 2 );     gcmENDSTATEBATCH(reserve, memory);};

    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0585, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0585) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0585, stride << 2 );     gcmENDSTATEBATCH(reserve, memory);};

    if (Hardware->pixelPipes == 2)
    {
        gctUINT32 bankOffsetBytes = 0;
        gctUINT32 physical2;

        gctUINT32 topBufferSize = gcmALIGN(
                Hardware->tempSurface->info.alignedHeight / 2,
                Hardware->tempSurface->info.superTiled ? 64 : 4
                )
            * Hardware->tempSurface->info.stride;

#if gcdENABLE_BANK_ALIGNMENT
        gcmONERROR(
            gcoSURF_GetBankOffsetBytes(gcvNULL,
                                     Hardware->tempSurface->info.type,
                                     topBufferSize,
                                     &bankOffsetBytes
                                     ));

        /* If surface doesn't have enough padding, then don't offset it. */
        if (Hardware->tempSurface->info.size <
            ((Hardware->tempSurface->info.alignedHeight * Hardware->tempSurface->info.stride)
            + bankOffsetBytes))
        {
            bankOffsetBytes = 0;
        }
#endif

        physical2
            = bankOffsetBytes
            + physical[0]
            + topBufferSize;

        /* Append RESOLVE_SOURCE addresses. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05B0, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05B0,
                physical[0]
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05B0 + 1,
                physical2
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Append RESOLVE_DESTINATION addresses. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05B8, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05B8) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05B8,
                physical[0]
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05B8 + 1,
                physical2
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );

        /* Append offset. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x05C0, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05C0,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)))
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x05C0 + 1,
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (rectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)))
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );
    }
    else
    {
        /* Append RESOLVE_SOURCE commands. */
        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0582, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0582) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0582, physical[0] );     gcmENDSTATEBATCH(reserve, memory);};

        /* Append RESOLVE_DESTINATION commands. */
        {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0584, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0584) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0584, physical[0] );     gcmENDSTATEBATCH(reserve, memory);};
    }

    /* Program the window size. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0588, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0588, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (rectSize.x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (rectSize.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );     gcmENDSTATEBATCH(reserve, memory);};

    /* Make sure it is a resolve and not a clear. */
    {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x058F, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x058F, 0 );     gcmENDSTATEBATCH(reserve, memory);};

    /* Start the resolve. */
    {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0580, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0580, 0xBADABEEB );gcmENDSTATEBATCH(reserve, memory);};

    if (Hardware->colorStates.surface != gcvNULL)
    {
        /* Reprogram tile status. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0596,
                Hardware->physicalTileColor + physicalBaseAddress
                );

            gcmSETSTATEDATA(
                stateDelta, reserve, memory, gcvFALSE, 0x0597,
                Hardware->colorStates.surface->node.physical + physicalBaseAddress
                );

            gcmSETFILLER(
                reserve, memory
                );

        gcmENDSTATEBATCH(
            reserve, memory
            );
    }

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Unlock the temporary surface. */
    gcmONERROR(gcoSURF_Unlock(Hardware->tempSurface, logical[0]));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (logical[0] != gcvNULL)
    {
        /* Unlock the temporary surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(Hardware->tempSurface, logical[0]));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushTileStatus(
    IN gcsSURF_INFO_PTR Surface,
    IN gctBOOL Decompress
    )
{
    gceSTATUS status;
    gctBOOL flushed = gcvFALSE;
    gctBOOL flushedTileStatus = gcvFALSE;
    gctBOOL fastClearC, fastClearZ, fastClearFlush;
    gctUINT32 physicalBaseAddress;
    gctPOINTER logical[3] = {gcvNULL};
    gcoHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("Surface=0x%x Decompress=%d",
                  Surface, Decompress);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Nothing to do when there is no fast clear. */
    if (!(((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 0:0)) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) )
        /* Already in a flush - don't recurse! */
    ||  hardware->inFlush
        /* Need to be in 3D pipe. */
    ||  (hardware->currentPipe != 0x0)
        /* Skip this if the tile status got paused. */
    ||  hardware->paused
    )
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

#if gcdSECURE_USER
    physicalBaseAddress = 0;
#else
    /* Get physical base address. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20) == gcvSTATUS_TRUE)
    {
        physicalBaseAddress = 0;
    }
    else
    {
        gcmONERROR(gcoOS_GetBaseAddress(gcvNULL, &physicalBaseAddress));
    }
#endif

    /* Mark that we are in a flush. */
    hardware->inFlush = gcvTRUE;

    fastClearC = ((((gctUINT32) (hardware->memoryConfig)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));

    fastClearZ = ((((gctUINT32) (hardware->memoryConfig)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));

    fastClearFlush = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));

    /* If the hardware doesn't have a real flush and the cache is marked as
    ** dirty, we do need to do a partial resolve to make sure the dirty
    ** cache lines are pushed out. */
    if ((fastClearC || fastClearZ) && !fastClearFlush)
    {
        /* Flush the cache the hard way. */
        gcsPOINT origin, size;

        /* Setup the origin. */
        origin.x = origin.y = 0;

        /* Setup the size. */
        size.x = 16;
        size.y = 16 * 4 * 2;

        if (Decompress
        &&  (Surface == hardware->colorStates.surface)
        &&  (hardware->colorStates.surface->stride >= 256)
        &&  (hardware->colorStates.surface->alignedHeight < (gctUINT) size.y)
        )
        {
            size.x     = hardware->colorStates.surface->alignedWidth;
            size.y     = hardware->colorStates.surface->alignedHeight;
            Decompress = gcvFALSE;
        }

        /* Flush the pipe. */
        gcmONERROR(gcoHARDWARE_FlushPipe());

        /* The pipe has been flushed. */
        flushed = gcvTRUE;

        if ((hardware->colorStates.surface == gcvNULL)
        ||  (hardware->colorStates.surface->stride < 256)
        ||  (hardware->colorStates.surface->alignedHeight < (gctUINT) size.y)
        ||  !fastClearC
        )
        {
            gctUINT32 address[3] = {0};
            gcsSURF_INFO_PTR current;
            gctUINT32 stride;

            /* Define state buffer variables. */
            gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

            /* Save current target. */
            current = hardware->colorStates.surface;

            /* Construct a temporary surface. */
            if (hardware->tempSurface == gcvNULL)
            {
                gcmONERROR(gcoSURF_Construct(
                    gcvNULL,
                    64, size.y, 1,
                    gcvSURF_RENDER_TARGET,
                    gcvSURF_A8R8G8B8,
                    gcvPOOL_DEFAULT,
                    &hardware->tempSurface
                    ));
            }

            /* Lock the surface. */
            gcmONERROR(gcoSURF_Lock(
                hardware->tempSurface, address, logical
                ));

            /* Stride. */
            gcmONERROR(gcoSURF_GetAlignedSize(
                hardware->tempSurface, gcvNULL, gcvNULL, (gctINT_PTR) &stride
                ));

            /* Determine the size of the buffer to reserve. */
            reserveSize

                /* Grouped states. */
                = gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)

                /* Cache flush. */
                + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 2, 8)

                /* Grouped resolve states. */
                + gcmALIGN((1 + 5) * gcmSIZEOF(gctUINT32), 8)

                /* Ungrouped resolve states. */
                + gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 3, 8);

            if (!fastClearC)
            {
                reserveSize
                    += gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32) * 2, 8);
            }

            if (current)
            {
                reserveSize
                    += gcmALIGN((1 + 2) * gcmSIZEOF(gctUINT32), 8)
                    +  gcmALIGN((1 + 1) * gcmSIZEOF(gctUINT32), 8);
            }

            /* Determine the size of the buffer to reserve. */
            gcmBEGINSTATEBUFFER(hardware, reserve, stateDelta, memory, reserveSize);

            /* Reprogram tile status. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0596,
                      hardware->tempSurface->info.tileStatusNode.physical
                    + physicalBaseAddress
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0597,
                      hardware->tempSurface->info.node.physical
                    + physicalBaseAddress
                    );

                gcmSETFILLER(
                    reserve, memory
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0594, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) );gcmENDSTATEBATCH(reserve, memory);};

            if (!fastClearC)
            {
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0595, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0595) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0595, ((((gctUINT32) (hardware->memoryConfig)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) );     gcmENDSTATEBATCH(reserve, memory);};
            }

            /* Resolve this buffer on top of itself. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0581, 5 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (5 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0581) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0581,
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x06 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1))))))) << (0 ? 14:14)))
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0582,
                    address[0]
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0583,
                    stride << 2
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0584,
                    address[0]
                    );

                gcmSETSTATEDATA(
                    stateDelta, reserve, memory, gcvFALSE, 0x0585,
                    stride << 2
                    );

            gcmENDSTATEBATCH(
                reserve, memory
                );

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0588, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0588, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (size.x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (size.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) );     gcmENDSTATEBATCH(reserve, memory);};

            {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x058F, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x058F) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x058F, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) );     gcmENDSTATEBATCH(reserve, memory);};

            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0580, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0580, 0xBADABEEB );gcmENDSTATEBATCH(reserve, memory);};

            if (current != gcvNULL)
            {
                /* Reprogram tile status. */
                {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0596, 2 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0596) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0596,
                          hardware->physicalTileColor
                        + physicalBaseAddress
                        );

                    gcmSETSTATEDATA(
                        stateDelta, reserve, memory, gcvFALSE, 0x0597,
                          current->node.physical
                        + physicalBaseAddress
                        );

                    gcmSETFILLER(
                        reserve, memory
                        );

                gcmENDSTATEBATCH(
                    reserve, memory
                    );

                {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0594, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) );gcmENDSTATEBATCH(reserve, memory);};
            }

            if (!fastClearC)
            {
                {    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0595, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0595) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};    gcmSETSTATEDATA(stateDelta, reserve, memory, gcvFALSE, 0x0595, hardware->memoryConfig );     gcmENDSTATEBATCH(reserve, memory);};
            }

            /* Invalidate the tile status cache. */
            {{    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0594, 1);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0594) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};gcmSETCTRLSTATE(stateDelta, reserve, memory, 0x0594, ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) );gcmENDSTATEBATCH(reserve, memory);};

            /* Validate the state buffer. */
            gcmENDSTATEBUFFER(reserve, memory, reserveSize);

            /* Unlock the surface. */
            gcmONERROR(gcoSURF_Unlock(hardware->tempSurface, logical[0]));
            logical[0] = gcvNULL;
        }
        else
        {
            if (Decompress && hardware->colorStates.dirty && fastClearC)
            {
                /* We need to decompress as well, so resolve the entire
                ** target on top of itself. */
                size.x = hardware->colorStates.surface->alignedWidth;
                size.y = hardware->colorStates.surface->alignedHeight;
            }

            /* Resolve the current render target onto itself. */
            gcmONERROR(_ResolveRect(
                hardware,
                hardware->colorStates.surface,
                hardware->colorStates.surface,
                &origin, &origin, &size
                ));

            /* The pipe has been flushed by resolve. */
            flushed = gcvTRUE;
        }

        /* The tile status buffer has been flushed */
        flushedTileStatus = gcvTRUE;
    }

    /* Check for decompression request. */
    if (Decompress && hardware->colorStates.dirty)
    {
        gcsPOINT origin, size;

        /* Setup the origin. */
        origin.x = origin.y = 0;

        /* Setup the size. */
        size.x = Surface->alignedWidth;
        size.y = Surface->alignedHeight;

        if (fastClearC && (Surface->type == gcvSURF_RENDER_TARGET))
        {
            /* Resolve the current render target onto itself. */
            gcmONERROR(_ResolveRect(
                hardware, Surface, Surface, &origin, &origin, &size
                ));
        }

        else if (fastClearZ && (Surface->type == gcvSURF_DEPTH))
        {
            /* Resolve the current depth onto itself. */
            gcmONERROR(gcoHARDWARE_ResolveDepth(
                hardware->physicalTileDepth,
                Surface, Surface, &origin, &origin, &size
                ));
        }

        /* The pipe has been flushed by resolve. */
        flushed = gcvTRUE;
    }

    if (!flushed)
    {
        /* Flush the pipe. */
        gcmONERROR(gcoHARDWARE_FlushPipe());
    }

    /* Do we need to explicitly flush?. */
    if (flushedTileStatus ||
        (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_FC_FLUSH_STALL) == gcvSTATUS_TRUE))
    {
        /* Invalidate the tile status cache. */
        gcmONERROR(gcoHARDWARE_LoadState32(
            hardware,
            0x01650,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            ));
    }
    else
    {
        gcmONERROR(_FlushTileStatusCache(hardware, hardware->colorStates.surface));
    }

    /* Sempahore-stall before next primitive. */
    gcmONERROR(gcoHARDWARE_Semaphore(
        gcvWHERE_RASTER, gcvWHERE_PIXEL, gcvHOW_SEMAPHORE
        ));

    /* Tile status cache is no longer dirty. */
    hardware->cacheDirty = gcvFALSE;

    /* Flush finished. */
    hardware->inFlush = gcvFALSE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (hardware != gcvNULL)
	{
		if(logical[0] != gcvNULL)
	    {
	        /* Unlock the surface. */
	        gcmVERIFY_OK(gcoSURF_Unlock(hardware->tempSurface, logical[0]));
	    }

	    /* Flush finished. */
		hardware->inFlush = gcvFALSE;
	}

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_SetAntiAlias
**
**  Enable or disable anti-aliasing.
**
**  INPUT:
**
**      gctBOOL Enable
**          Enable anti-aliasing when gcvTRUE and disable anti-aliasing when
**          gcvFALSE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SetAntiAlias(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Store value. */
    hardware->sampleMask      = Enable ? 0xF : 0x0;
    hardware->msaaConfigDirty = gcvTRUE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHARDWARE_SetDither
**
**  Enable or disable dithering.
**
**  INPUT:
**
**      gctBOOL Enable
**          gcvTRUE to enable dithering or gcvFALSE to disable it.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_SetDither(
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* New dithering function. */
    if (Enable)
    {
        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x014A8, 0x6E4CA280));
        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x014AC, 0x5D7F91B3));
    }
    else
    {
        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x014A8, (gctUINT32)(~0)));
        gcmONERROR(gcoHARDWARE_LoadState32(hardware, 0x014AC, (gctUINT32)(~0)));
    }

    /* Set dithering values. */
    hardware->dither[0] = Enable ? 0x6E4CA280 : ~0;
    hardware->dither[1] = Enable ? 0x5D7F91B3 : ~0;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHARDWARE_UseSoftware2D
**
**  Sets the software 2D renderer force flag.
**
**  INPUT:
**
**      gctBOOL Enable
**          gcvTRUE to enable using software for 2D or
**          gcvFALSE to use underlying hardware.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_UseSoftware2D(
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Enable=%d", Enable);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Set the force flag. */
    hardware->sw2DEngine = Enable;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**
**  gcoHARDWARE_WriteBuffer
**
**  Write data into the command buffer.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer that contains the data to be copied.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to copy.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_WriteBuffer(
    IN gcoHARDWARE Hardware,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Data=0x%x Bytes=%d Aligned=%d",
                    Hardware, Data, Bytes, Aligned);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Call hardware. */
    gcmONERROR(gcoBUFFER_Write(Hardware->buffer, Data, Bytes, Aligned));

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHARDWARE_RGB2YUV
**
**  Convert RGB8 color value to YUV color space.
**
**  INPUT:
**
**      gctUINT8 R, G, B
**          RGB color value.
**
**  OUTPUT:
**
**      gctUINT8_PTR Y, U, V
**          YUV color value.
*/
void gcoHARDWARE_RGB2YUV(
    gctUINT8 R,
    gctUINT8 G,
    gctUINT8 B,
    gctUINT8_PTR Y,
    gctUINT8_PTR U,
    gctUINT8_PTR V
    )
{
    gcmHEADER_ARG("R=%d G=%d B=%d",
                    R, G, B);

    *Y = (gctUINT8) (((66 * R + 129 * G +  25 * B + 128) >> 8) +  16);
    *U = (gctUINT8) (((-38 * R -  74 * G + 112 * B + 128) >> 8) + 128);
    *V = (gctUINT8) (((112 * R -  94 * G -  18 * B + 128) >> 8) + 128);

    gcmFOOTER_ARG("*Y=%d *U=%d *V=%d",
                    *Y, *U, *V);
}

/*******************************************************************************
**
**  gcoHARDWARE_YUV2RGB
**
**  Convert YUV color value to RGB8 color space.
**
**  INPUT:
**
**      gctUINT8 Y, U, V
**          YUV color value.
**
**  OUTPUT:
**
**      gctUINT8_PTR R, G, B
**          RGB color value.
*/
void gcoHARDWARE_YUV2RGB(
    gctUINT8 Y,
    gctUINT8 U,
    gctUINT8 V,
    gctUINT8_PTR R,
    gctUINT8_PTR G,
    gctUINT8_PTR B
    )
{
    /* Clamp the input values to the legal ranges. */
    gctINT y = (Y < 16) ? 16 : ((Y > 235) ? 235 : Y);
    gctINT u = (U < 16) ? 16 : ((U > 240) ? 240 : U);
    gctINT v = (V < 16) ? 16 : ((V > 240) ? 240 : V);

    /* Shift ranges. */
    gctINT _y = y - 16;
    gctINT _u = u - 128;
    gctINT _v = v - 128;

    /* Convert to RGB. */
    gctINT r = (298 * _y            + 409 * _v + 128) >> 8;
    gctINT g = (298 * _y - 100 * _u - 208 * _v + 128) >> 8;
    gctINT b = (298 * _y + 516 * _u            + 128) >> 8;

    gcmHEADER_ARG("Y=%d U=%d V=%d",
                    Y, U, V);

    /* Clamp the result. */
    *R = (r < 0)? 0 : (r > 255)? 255 : (gctUINT8) r;
    *G = (g < 0)? 0 : (g > 255)? 255 : (gctUINT8) g;
    *B = (b < 0)? 0 : (b > 255)? 255 : (gctUINT8) b;

    gcmFOOTER_ARG("*R=%d *G=%d *B=%d",
                    *R, *G, *B);
}

#ifndef VIVANTE_NO_3D
gceSTATUS gcoHARDWARE_FlushL2Cache(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;

#if gcd6000_SUPPORT
    gcoCMDBUF reserve;
    gctUINT32 * memory, reserveSize;

	gcmHEADER_ARG("Hardware=0x%08x",
                  Hardware);

    reserveSize = 4 + 14 + 2;

    /* Allocate space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->buffer,
        reserveSize * gcmSIZEOF(gctUINT32),
        gcvTRUE,
        &reserve));

    /* Cast the reserve pointer. */
    memory = reserve->lastReserve;

    /*************************************************************/
    /* Semaphore stall pipe. */
    /* Send FE-PE sempahore token. */
    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

    /* Dump the semaphore. */
    gcmDUMP(gcvNULL, "@[semaphore 0x%08X 0x%08X]",
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));

    /* Send FE-PE stall token. */
    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0F00) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

    /* Dump the stall. */
    gcmDUMP(gcvNULL, "@[stall 0x%08X 0x%08X]",
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))),
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x07 & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))));

    /*************************************************************/
    /* Sync GPUs. */

    /* Enable chip ID 0. */
    *memory++ =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | (1 << 0);
    memory++;

    gcmDUMP(gcvNULL,
            "@[chipSelect 0x%04X]",
            (1 << 0));

    /* Send semaphore from FE to ChipID 1. */
    *memory++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

    /* Dump the semaphore. */
            gcmDUMP(gcvNULL, "@[semaphore chip=0x%04X]", 1);

    /* Wait for semaphore from ChipID 1. */
    *memory++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

    /* Dump the stall. */
    gcmDUMP(gcvNULL, "@[stall chip=0x%04X]", 0);

    /* Enable chip ID 1. */
    *memory++ =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | (1 << 1);
    memory++;

    gcmDUMP(gcvNULL,
            "@[chipSelect 0x%04X]",
            (1 << 1));

    /* Send semaphore from FE to ChipID 0. */
    *memory++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

    /* Dump the semaphore. */
    gcmDUMP(gcvNULL, "@[semaphore chip=0x%04X]", 0);

    /* Wait for semaphore from ChipID 0. */
    *memory++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

    /* Dump the stall. */
    gcmDUMP(gcvNULL, "@[stall chip=0x%04X]", 1);

    /* Enable all chips. */
    *memory++ =
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | (0xFFFF);
    memory++;

    gcmDUMP(gcvNULL,
            "@[chipSelect 0x%04X]",
            (0xFFFF));

    /*************************************************************/
    /* Flush L2. */
    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E03) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

    *memory++ =
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

    gcmDUMP(gcvNULL,
            "@[state 0x%04X 0x%08X]",
            0x0E03, memory[-1]);
#else
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Idle the pipe. */
    gcmONERROR(
        gcoHARDWARE_Semaphore(gcvWHERE_COMMAND,
                              gcvWHERE_PIXEL,
                              gcvHOW_SEMAPHORE_STALL));

    /* Flush the L2 cache. */
    gcmONERROR(
        gcoHARDWARE_LoadState32(Hardware, 0x01650,
                                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))));

    /* Idle the pipe (again). */
    gcmONERROR(
        gcoHARDWARE_Semaphore(gcvWHERE_COMMAND,
                              gcvWHERE_PIXEL,
                              gcvHOW_SEMAPHORE_STALL));
#endif

    Hardware->flushL2 = gcvFALSE;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

/*******************************************************************************
**
**  gcoHARDWARE_SetAutoFlushCycles
**
**  Set the GPU clock cycles after which the idle 2D engine will keep auto-flushing.
**
**  INPUT:
**
**      UINT32 Cycles
**          Source color premultiply with Source Alpha.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoHARDWARE_SetAutoFlushCycles(
    IN gctUINT32 Cycles
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Cycles=%d", Cycles);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    if (hardware->hw2DEngine && !hardware->sw2DEngine)
    {
        /* LoadState timeout value. */
        gcmONERROR(gcoHARDWARE_Load2DState32(
            hardware,
            0x00670,
            Cycles
            ));
    }
    else
    {
        /* Auto-flush is not supported by the software renderer. */
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/* Resolve depth buffer. */
gceSTATUS
gcoHARDWARE_ResolveDepth(
    IN gctUINT32 SrcTileStatusAddress,
    IN gcsSURF_INFO_PTR SrcInfo,
    IN gcsSURF_INFO_PTR DestInfo,
    IN gcsPOINT_PTR SrcOrigin,
    IN gcsPOINT_PTR DestOrigin,
    IN gcsPOINT_PTR RectSize
    )
{
    gceSTATUS status;
    gctUINT32 physicalBaseAddress = 0;
    gctUINT32 config;
    gctBOOL hasCompression;
    gctUINT32 format;
    gcoHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("SrcTileStatusAddress=%x SrcInfo=0x%x "
                    "DestInfo=0x%x SrcOrigin=0x%x DestOrigin=0x%x "
                    "RectSize=0x%x",
                    SrcTileStatusAddress, SrcInfo,
                    DestInfo, SrcOrigin, DestOrigin,
                    RectSize);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(SrcInfo != gcvNULL);

#ifndef VIVANTE_NO_3D
    /* If the tile status is disabled for the surface, just do a normal
    ** resolve. */
    if (SrcInfo->tileStatusDisabled || (SrcTileStatusAddress == ~0U))
    {
        gctBOOL pause = gcvFALSE;

        /* Check if color tile status is enabled. */
        if (((((gctUINT32) (hardware->memoryConfig)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))))
        {
            /* Pause the tile status. */
            gcmVERIFY_OK(
                gcoHARDWARE_PauseTileStatus(hardware, gcvTILE_STATUS_PAUSE));

            pause = gcvTRUE;
        }

        /* Use normal resolve. */
        status = gcoHARDWARE_ResolveRect(SrcInfo,
                                         DestInfo,
                                         SrcOrigin,
                                         DestOrigin,
                                         RectSize);

        if (pause)
        {
            /* Resume the tile status. */
            gcmVERIFY_OK(
                gcoHARDWARE_PauseTileStatus(hardware,
                                            gcvTILE_STATUS_RESUME));
        }

        /* Return the status. */
        gcmFOOTER();
        return status;
    }
#endif

#if !gcdSECURE_USER
    /* Get physical base address. */
    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_MC20) != gcvSTATUS_TRUE)
    {
        gcmONERROR(gcoOS_GetBaseAddress(gcvNULL, &physicalBaseAddress));
    }
#endif

    /* Determine compression. */
    hasCompression = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));

    /* Determine color format. */
    switch (SrcInfo->format)
    {
    case gcvSURF_D24X8:
        format = 0x6;
        break;

    case gcvSURF_D24S8:
        format = 0x5;
        break;

    case gcvSURF_D16:
		/* Fake this by using ARGB4 format. */
        format = 0x0;
        break;

    case gcvSURF_D32:
    default:
        gcmFOOTER();
        return gcvSTATUS_NOT_SUPPORTED;
    }

#ifndef VIVANTE_NO_3D
    /* Flush the pipe. */
    gcmONERROR(
        gcoHARDWARE_FlushPipe());

    /* Flush the tile status. */
    gcmONERROR(
        gcoHARDWARE_FlushTileStatus(SrcInfo, gcvFALSE));
#endif /* VIVANTE_NO_3D */

    /* Program depth tile status into color fast clear. */
    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware, 0x01658,
                                SrcTileStatusAddress + physicalBaseAddress));

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware, 0x0165C,
                                SrcInfo->node.physical + physicalBaseAddress));

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware, 0x01660,
                                SrcInfo->clearValue));

    /* Build configuration register. */
    config = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1))))))) << (0 ? 2:2)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (hasCompression) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
           ;

    gcmONERROR(
        gcoHARDWARE_LoadState32(hardware, 0x01654,
                                config));

    /* Perform the resolve. */
    status = gcoHARDWARE_ResolveRect(SrcInfo,
                                     DestInfo,
                                     SrcOrigin,
                                     DestOrigin,
                                     RectSize);

OnError:
	if (hardware != gcvNULL)
	{
		if (hardware->colorStates.surface != gcvNULL)
		{
			/* Reset tile status. */
			gcmVERIFY_OK(
				gcoHARDWARE_LoadState32(hardware,
										0x01658,
										hardware->physicalTileColor +
										physicalBaseAddress));

			gcmVERIFY_OK(
				gcoHARDWARE_LoadState32(hardware,
										0x0165C,
										hardware->colorStates.surface->node.physical +
										physicalBaseAddress));

			gcmVERIFY_OK(
				gcoHARDWARE_LoadState32(hardware,
										0x01660,
										hardware->colorStates.surface->clearValue));
		}
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

gceSTATUS
gcoHARDWARE_Load2DState32(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Address=0x%08x Data=0x%08x",
                  Hardware, Address, Data);

    /* Call buffered load state to do it. */
    gcmONERROR(gcoHARDWARE_Load2DState(Hardware, Address, 1, &Data));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_Load2DState(
    IN gcoHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Address=0x%08x Count=%lu Data=0x%08x",
                  Hardware, Address, Count, Data);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

     /* Verify the arguments. */
    if (Hardware->hw2DCmdIndex & 1)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if (Hardware->hw2DCmdBuffer != gcvNULL)
    {
        gctUINT32_PTR memory;

        if (Hardware->hw2DCmdSize - Hardware->hw2DCmdIndex < gcmALIGN(1 + Count, 2))
        {
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        memory = Hardware->hw2DCmdBuffer + Hardware->hw2DCmdIndex;

        /* LOAD_STATE(Count, Address >> 2) */
        memory[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (Count) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Address >> 2) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        /* Copy the state buffer. */
        gcmONERROR(
            gcoOS_MemCopy(&memory[1], Data, Count * 4));
    }

    Hardware->hw2DCmdIndex += 1 + Count;

    /* Aligned */
    if (Hardware->hw2DCmdIndex & 1)
    {
        Hardware->hw2DCmdIndex += 1;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_2DAppendNop(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    if (Hardware->hw2DCmdIndex & 1)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    if ((Hardware->hw2DCmdBuffer != gcvNULL)
        && (Hardware->hw2DCmdSize > Hardware->hw2DCmdIndex))
    {
        gctUINT32_PTR memory = Hardware->hw2DCmdBuffer + Hardware->hw2DCmdIndex;
        gctUINT32 i = 0;

        while (i < Hardware->hw2DCmdSize - Hardware->hw2DCmdIndex)
        {
            /* Append NOP. */
            memory[i++] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
            memory[i++] = 0xdeaddead;
        }

        Hardware->hw2DCmdIndex = Hardware->hw2DCmdSize;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#ifndef VIVANTE_NO_3D
/*******************************************************************************
**  gcoHARDWARE_InvokeThreadWalker
**
**  Start OCL thread walker.
**
**  INPUT:
**
**      gcsTHREAD_WALKER_INFO_PTR Info
**          Pointer to the informational structure.
*/
gceSTATUS
gcoHARDWARE_InvokeThreadWalker(
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Info=0x%08X", Info);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

    /* Determine the size of the buffer to reserve. */
    reserveSize = gcmALIGN((1 + 9) * gcmSIZEOF(gctUINT32), 8);

    /* Reserve space in the command buffer. */
    gcmBEGINSTATEBUFFER(hardware, reserve, stateDelta, memory, reserveSize);

    {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, 0x0240, 9 );    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (gcvFALSE) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (9 ) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0240) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0240,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (Info->dimensions) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (Info->traverseOrder) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathX) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathY) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1))))))) << (0 ? 9:9)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10))) | (((gctUINT32) ((gctUINT32) (Info->enableSwathZ) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1))))))) << (0 ? 10:10)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeX) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeY) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (Info->swathSizeZ) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) | (((gctUINT32) ((gctUINT32) (Info->valueOrder) & ((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0241,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Info->globalSizeX) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetX) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0242,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Info->globalSizeY) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetY) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0243,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Info->globalSizeZ) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->globalOffsetZ) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0244,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeX - 1) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->workGroupCountX - 1) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0245,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeY - 1) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->workGroupCountY - 1) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0246,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0))) | (((gctUINT32) ((gctUINT32) (Info->workGroupSizeZ - 1) & ((gctUINT32) ((((1 ? 9:0) - (0 ? 9:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:0) - (0 ? 9:0) + 1))))))) << (0 ? 9:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->workGroupCountZ - 1) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            );

        gcmSETSTATEDATA(
            stateDelta, reserve, memory, gcvFALSE, 0x0247,
            Info->threadAllocation
            );

        gcmSETCTRLSTATE(
            stateDelta, reserve, memory, 0x0248,
            0xBADABEEB
            );

    gcmENDSTATEBATCH(
        reserve, memory
        );

    /* Validate the state buffer. */
    gcmENDSTATEBUFFER(reserve, memory, reserveSize);

    /* Flush the Shader L1 cache. */
    gcmONERROR(gcoHARDWARE_LoadState32(
        hardware,
        0x0380C,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcoHARDWARE_ComputeCentroids
**
**  Compute centroids.
**
**  INPUT:
**
*/
gceSTATUS
gcoHARDWARE_ComputeCentroids(
    IN gcoHARDWARE Hardware,
    IN gctUINT Count,
    IN gctUINT32_PTR SampleCoords,
    OUT gcsCENTROIDS_PTR Centroids
    )
{
    gctUINT i, j, count, sumX, sumY;

    gcmHEADER_ARG("Hardware=0x%08X", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    for (i = 0; i < Count; i += 1)
    {
        /* Zero out the centroid table. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(&Centroids[i], sizeof(gcsCENTROIDS)));

        /* Set the value for invalid pixels. */
        if (Hardware->api == gcvAPI_OPENGL)
        {
            Centroids[i].value[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (8) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (8) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)));
        }

        /* Compute all centroids. */
        for (j = 1; j < 16; j += 1)
        {
            /* Initializ sums and count. */
            sumX = sumY = 0;
            count = 0;

            if ((j == 0x5) || (j == 0xA)
            ||  (j == 0x7) || (j == 0xB)
            ||  (j == 0xD) || (j == 0xE)
            )
            {
                sumX = sumY = 0x8;
            }
            else
            {
                if (j & 0x1)
                {
                    /* Add sample 1. */
                    sumX += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 3:0)) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1)))))) );
                    sumY += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 7:4)) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1)))))) );
                    ++count;
                }

                if (j & 0x2)
                {
                    /* Add sample 2. */
                    sumX += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 11:8)) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1)))))) );
                    sumY += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 15:12)) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1)))))) );
                    ++count;
                }

                if (j & 0x4)
                {
                    /* Add sample 3. */
                    sumX += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 19:16)) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1)))))) );
                    sumY += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 23:20)) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1)))))) );
                    ++count;
                }

                if (j & 0x8)
                {
                    /* Add sample 4. */
                    sumX += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 27:24)) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1)))))) );
                    sumY += (((((gctUINT32) (SampleCoords[i])) >> (0 ? 31:28)) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1)))))) );
                    ++count;
                }

                /* Compute average. */
                if (count > 0)
                {
                    sumX /= count;
                    sumY /= count;
                }
            }

            switch (j & 3)
            {
            case 0:
                /* Set for centroid 0, 4, 8, or 12. */
                Centroids[i].value[j / 4]
                    = ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (sumX) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                    | ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (sumY) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)));
                break;

            case 1:
                /* Set for centroid 1, 5, 9, or 13. */
                Centroids[i].value[j / 4]
                    = ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (sumX) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))
                    | ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (sumY) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)));
                break;

            case 2:
                /* Set for centroid 2, 6, 10, or 14. */
                Centroids[i].value[j / 4]
                    = ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16))) | (((gctUINT32) ((gctUINT32) (sumX) & ((gctUINT32) ((((1 ? 19:16) - (0 ? 19:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:16) - (0 ? 19:16) + 1))))))) << (0 ? 19:16)))
                    | ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20))) | (((gctUINT32) ((gctUINT32) (sumY) & ((gctUINT32) ((((1 ? 23:20) - (0 ? 23:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:20) - (0 ? 23:20) + 1))))))) << (0 ? 23:20)));
                break;

            case 3:
                /* Set for centroid 3, 7, 11, or 15. */
                Centroids[i].value[j / 4]
                    = ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (sumX) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)))
                    | ((((gctUINT32) (Centroids[i].value[j / 4])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) ((gctUINT32) (sumY) & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));
                break;
            }
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoHARDWARE_ProgramResolve
**
**  Program the Resolve offset, Window and then trigger the resolve.
**
**  INPUT:
**
**      gcoHARDWARE Hardware
**          Pointer to the gcoHARDWARE object.
**
**      gcsPOINT RectSize
**          The size of the rectangular area to be resolved.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHARDWARE_ProgramResolve(
    IN gcoHARDWARE Hardware,
    IN gcsPOINT RectSize
    )
{
    gceSTATUS status;
    gcoCMDBUF reserve;
    gctUINT32 * memory, reserveSize, srcWndSize;
    gctUINT32 triggerResolveAtEnd = 1;

	gcmHEADER_ARG("Hardware=0x%08x RectSize=%d,%d",
                  Hardware, RectSize.x, RectSize.y);

    /* Trigger state. */
    reserveSize = 2;

    switch (Hardware->pixelPipes)
    {
    case 1:
        /* Window state. */
        reserveSize += 2;
        break;

    case 2:
        gcmASSERT((RectSize.y & 7) == 0);
        RectSize.y /= 2;

#if !gcd6000_SUPPORT
        reserveSize += 6;
#else
        reserveSize += (12*2 + 2) - 6;

        if (RectSize.x > 16)
        {
            /* Program second resolve window when vertically splitting. */
            reserveSize += 6;
        }
        else
        {
            /* Trigger resolve only for first chip. */
            triggerResolveAtEnd = 0;
        }
#endif
        break;

    default:
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Allocate space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->buffer,
        reserveSize * gcmSIZEOF(gctUINT32),
        gcvTRUE,
        &reserve));

    /* Cast the reserve pointer. */
    memory = reserve->lastReserve;

    if (Hardware->pixelPipes == 2)
    {
#if !gcd6000_SUPPORT
        srcWndSize = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (RectSize.x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                   | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

       /* Append RESOLVE_WINDOW commands. */
        *memory++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        *memory++
            = srcWndSize;

        gcmDUMP(gcvNULL,
                "@[state 0x%04X 0x%08X]",
                0x0588, memory[-1]);

        *memory++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        *memory++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

        *memory++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

        gcmDUMP(gcvNULL,
                "@[state 0x%04X 0x%08X]",
                0x05C0, memory[-2]);

        gcmDUMP(gcvNULL,
                "@[state 0x%04X 0x%08X]",
                0x05C0+1, memory[-1]);

        memory++;
#else
        {
            gctINT32 alignedWidthSplit;

            /*************************************************************/
            /* Enable chip ID 0. */
            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | (1 << 0);
            memory++;

            gcmDUMP(gcvNULL,
                    "@[chipSelect 0x%04X]",
                    (1 << 0));

            /* Resolve window for ChipID 0. */
            alignedWidthSplit = gcmALIGN(RectSize.x/2, 16);
            srcWndSize = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (alignedWidthSplit) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                       | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

            /* Append RESOLVE_WINDOW commands. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *memory++
                = srcWndSize;

            gcmDUMP(gcvNULL,
                    "@[state 0x%04X 0x%08X]",
                    0x0588, memory[-1]);

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

#define VERTICAL_SPLIT 1
#if VERTICAL_SPLIT
            /* Vertical split. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));
#else
            /* Horizontal split. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (alignedWidthSplit) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));
#endif

            gcmDUMP(gcvNULL,
                    "@[state 0x%04X 0x%08X]",
                    0x05C0, memory[-2]);

            gcmDUMP(gcvNULL,
                    "@[state 0x%04X 0x%08X]",
                    0x05C0+1, memory[-1]);

            memory++;

            /* Send semaphore from FE to ChipID 1. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

            /* Dump the semaphore. */
            gcmDUMP(gcvNULL, "@[semaphore chip=0x%04X]", 1);

            /* Wait for semaphore from ChipID 1. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "@[stall chip=0x%04X]", 0);

            /* Trigger resolve for chip 0 here, if chip1 will not be triggered. */
            if (triggerResolveAtEnd == 0)
            {
                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

                *memory++ = 0xBADABEEB;

                gcmDUMP(gcvNULL,
                        "@[state 0x%04X 0x%08X]",
                        0x0580, memory[-1]);
            }

            /*************************************************************/
            /* Enable chip ID 1. */
            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | (1 << 1);
            memory++;

            gcmDUMP(gcvNULL,
                    "@[chipSelect 0x%04X]",
                    (1 << 1));

            if (RectSize.x != alignedWidthSplit)
            {
                /* Resolve window for ChipID 1. */
                gcmASSERT(((RectSize.x - alignedWidthSplit) % 16) == 0);

                srcWndSize = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (RectSize.x - alignedWidthSplit) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                           | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

                /* Append RESOLVE_WINDOW commands. */
                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

                *memory++ = srcWndSize;

                gcmDUMP(gcvNULL,
                        "@[state 0x%04X 0x%08X]",
                        0x0588, memory[-1]);

                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x05C0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

#if VERTICAL_SPLIT
                /* Vertical split. */
                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (alignedWidthSplit) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (alignedWidthSplit) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));
#else
                /* Horizontal split. */
                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

                *memory++ =
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (RectSize.x) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));
#endif
                gcmDUMP(gcvNULL,
                        "@[state 0x%04X 0x%08X]",
                        0x05C0, memory[-2]);

                gcmDUMP(gcvNULL,
                        "@[state 0x%04X 0x%08X]",
                        0x05C0+1, memory[-1]);

                memory++;
            }

            /* Send semaphore from FE to ChipID 0. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E02) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

            /* Dump the semaphore. */
            gcmDUMP(gcvNULL, "@[semaphore chip=0x%04X]", 0);

            /* Wait for semaphore from ChipID 0. */
            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

            *memory++ =
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:24) - (0 ? 27:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:24) - (0 ? 27:24) + 1))))))) << (0 ? 27:24)));

            /* Dump the stall. */
            gcmDUMP(gcvNULL, "@[stall chip=0x%04X]", 1);

            /*************************************************************/
            /* Enable all chips. */
            *memory++ =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | (0xFFFF);
            memory++;

            gcmDUMP(gcvNULL,
                    "@[chipSelect 0x%04X]",
                    (0xFFFF));
        }
#endif
    }
    else
    {
        /* Append RESOLVE_WINDOW commands. */
        *memory++ =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0588) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        *memory++ =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (RectSize.x) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (RectSize.y) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));

        gcmDUMP(gcvNULL,
                "@[state 0x%04X 0x%08X]",
                0x0588, memory[-1]);
    }

    /* Trigger resolve. */
    if (triggerResolveAtEnd == 1)
    {
        *memory++ =
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0580) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        *memory++ = 0xBADABEEB;

        gcmDUMP(gcvNULL,
                "@[state 0x%04X 0x%08X]",
                0x0580, memory[-1]);
    }

OnError:
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */


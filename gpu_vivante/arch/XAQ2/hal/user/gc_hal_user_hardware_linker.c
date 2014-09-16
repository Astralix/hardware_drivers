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




/*
**  Hardware dependent gcSL linker module.
*/

#include "gc_hal_user_hardware_precomp.h"

#ifndef VIVANTE_NO_3D

#include "gc_hal_user_compiler.h"
#include "gc_hal_user.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#define _EXTRA_16BIT_CONVERSION_         0
#define _USE_TEMP_FORMAT_                1
#define _USE_ORIGINAL_DST_WRITE_MASK_    1

/*******************************************************************************
************************************************************** Shader Hinting **
*******************************************************************************/

typedef struct _gcsSL_REFERENCE     gcsSL_REFERENCE;
typedef struct _gcsSL_REFERENCE     * gcsSL_REFERENCE_PTR;
struct _gcsSL_REFERENCE
{
    /* Reference index number. */
    gctINT                          index;

    /* Instruction for reference. */
    gcSL_INSTRUCTION                instruction;

    /* Source index for reference. */
    gctINT                          sourceIndex;
};

/* Temp usage structure. */
typedef struct _gcsSL_USAGE         gcsSL_USAGE;
typedef struct _gcsSL_USAGE         * gcsSL_USAGE_PTR;
struct _gcsSL_USAGE
{
    /* Last instruction register is used. */
    gctINT                          lastUse[4];
};

#define gcvSL_AVAILABLE             -1
#define gcvSL_TEMPORARY             -2
#define gcvSL_RESERVED              0x7FFFFFFF

/* Physical code. */
typedef struct _gcsSL_PHYSICAL_CODE gcsSL_PHYSICAL_CODE;
typedef struct _gcsSL_PHYSICAL_CODE * gcsSL_PHYSICAL_CODE_PTR;
struct _gcsSL_PHYSICAL_CODE
{
    /* Pointer to next physical code structure. */
    gcsSL_PHYSICAL_CODE_PTR         next;

    /* Maximum number of instructions this structure can hold. */
    gctSIZE_T                       maxCount;

    /* Number of instructions this structure holds. */
    gctSIZE_T                       count;

    /* At least 1 instruction per structure. */
    gctUINT32                       states[4];
};

typedef struct _gcsSL_CONSTANT_TABLE gcsSL_CONSTANT_TABLE;
typedef struct _gcsSL_CONSTANT_TABLE * gcsSL_CONSTANT_TABLE_PTR;
struct _gcsSL_CONSTANT_TABLE
{
    /* Pointer to next constant. */
    gcsSL_CONSTANT_TABLE_PTR        next;

    /* Constant value. */
    gctINT                          count;
    gctFLOAT                        constant[4];

    /* Uniform index. */
    gctINT                          index;

    /* Uniform swizzle. */
    gctUINT8                        swizzle;
};

typedef struct _gcsSL_ADDRESS_REG_COLORING
{
    /* HW address register has 4 channels, each can be used */
    /* as relative address access. This structure is used to */
    /* codegen-time address register coloring. The channel */
    /* coloring here is not aggressive since we have no channel-based */
    /* live analysis for address register, so redundant mova may be */
    /* introduced for certain cases. */

    struct TMP2ADDRMap
    {
        gctINT   indexedReg; /* -1 means no assignment to real reg, but space is occupied,
                             so startChannelInAddressReg is meanful */
        gctINT   startChannelInIndexedReg; /* zero-based */
        gctUINT8 startChannelInAddressReg; /* zero-based */
        gctINT   channelCount;
    } tmp2addrMap[4];

    /* For current instruction, LSB 4bits is valid, 1-x, 2-y, 4-z, 8-w */
    gctUINT8 localAddrChannelUsageMask;

    gctINT countOfMap;

}gcsSL_ADDRESS_REG_COLORING, *gcsSL_ADDRESS_REG_COLORING_PTR;

typedef struct _gcsSL_FUNCTION_CODE
{
    /* References. */
    gcsSL_REFERENCE                 references[8];
    struct _gcSL_INSTRUCTION        tempInstruction[3];
    gcsSL_REFERENCE                 tempReferences[3];
    gctINT                          tempShift[3];

    /* Branches. */
    gcSL_BRANCH_LIST                branch;

    /* Register last used for indexing. */
    gcsSL_ADDRESS_REG_COLORING      addrRegColoring;

    /* Base and current IP. */
    gctUINT                         ipBase;
    gctUINT                         ip;

    /* Physical code. */
    gcsSL_PHYSICAL_CODE_PTR         root;
    gcsSL_PHYSICAL_CODE_PTR         code;
}
gcsSL_FUNCTION_CODE,
*gcsSL_FUNCTION_CODE_PTR;

typedef struct _gcsSL_CODE_MAP
{
    gcsSL_FUNCTION_CODE_PTR         function;
    gctUINT                         location;
}
gcsSL_CODE_MAP,
* gcsSL_CODE_MAP_PTR;

struct _gcsCODE_GENERATOR
{
    /* Link flags. */
    gceSHADER_FLAGS                 flags;

    /* Uniform usage table. */
    gcsSL_USAGE_PTR                 uniformUsage;
    gctSIZE_T                       uniformCount;

    /* Constant table. */
    gcsSL_CONSTANT_TABLE_PTR        constants;

    /* Register usage table. */
    gcsSL_USAGE_PTR                 registerUsage;
    gctSIZE_T                       registerCount;
    gctUINT                         maxRegister;

    /* Generated code. */
    gcsSL_FUNCTION_CODE_PTR         functions;
    gcsSL_FUNCTION_CODE_PTR         current;
    gcsSL_CODE_MAP_PTR              codeMap;
    gctUINT                         nextSource;
    gctUINT                         endMain;
    gctUINT                         endPC;
    gctUINT32_PTR                   previousCode;

    /* State buffer management. */
    gctPOINTER                      stateBuffer;
    gctSIZE_T                       stateBufferSize;
    gctUINT32                       stateBufferOffset;
    gctUINT32 *                     lastStateCommand;
    gctUINT32                       lastStateCount;
    gctUINT32                       lastStateAddress;

    /* FragCoord usage. */
    gctBOOL                         usePosition;
    gctUINT                         positionPhysical;

    /* FrontFacing usage. */
    gctBOOL                         useFace;
    gctUINT                         facePhysical;
    gctUINT8                        faceSwizzle;

    /* PointCoord usage. */
    gctBOOL                         usePointCoord;
    gctUINT                         pointCoordPhysical;

    /* Hardware flags. */
    gctBOOL                         hasSIGN_FLOOR_CEIL;
    gctBOOL                         hasSQRT_TRIG;
    gctBOOL                         hasCL;

    /* OpenCL shader. */
    gctBOOL                         clShader;

    /* Flags for gc2100 and gc4000. */
    gctBOOL                         isGC2100;
    gctBOOL                         isGC4000;

    /* Flags for bug fix for gc2100 and gc4000. */
    gctBOOL                         hasBugFixes10;
    gctBOOL                         hasBugFixes11;

    /* Reserved registre for LOAD for LOAD bug (bugFixes10). */
    gctUINT                         reservedRegForLoad;
    gctINT                          loadDestIndex;
    gctINT                          origAssigned;
    gctINT                          lastLoadUser;
};

extern gctBOOL
value_type0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    );

void
_SetValueType0(
    IN gctUINT ValueType,
    IN OUT gctUINT32 * States
    )
{
    gctUINT instType0 = ValueType & 0x1;
    gctUINT instType1 = ValueType >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (instType0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (instType1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));
}

#if _USE_TEMP_FORMAT_
extern gctUINT type_conv[];

static void
_SetValueType0FromFormat(
    IN gctUINT Format,
    IN OUT gctUINT32 * States
    )
{
    gctUINT valueType0 = type_conv[Format];
    gctUINT instType0 = valueType0 & 0x1;
    gctUINT instType1 = valueType0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (instType0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (instType1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));
}
#endif

gctUINT
gcsCODE_GENERATOR_GetIP(
    gcsCODE_GENERATOR_PTR CodeGen
    )
{
    gcmHEADER_ARG("CodeGen=0x%x", CodeGen);

    gcmASSERT(CodeGen->current != gcvNULL);

    gcmFOOTER_ARG("return=0x%x", CodeGen->current->ip);
    return CodeGen->current->ip;
}

gctBOOL
_hasSIGN_FLOOR_CEIL(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    return CodeGen->hasSIGN_FLOOR_CEIL;
}

gctBOOL
_hasSQRT_TRIG(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    return CodeGen->hasSQRT_TRIG;
}

gctBOOL
_hasCL(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    return CodeGen->hasCL;
}

gctBOOL
_isGC2100Signed_8_16(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    if (CodeGen->isGC2100 && !CodeGen->hasBugFixes11)
    {
        gcSL_FORMAT format = gcmSL_TARGET_GET(Instruction->temp, Format);

        switch (format)
        {
        case gcSL_INT8:
        case gcSL_INT16:
            return gcvTRUE;

        default:
            return gcvFALSE;
        }
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL
_isGC2100Unsigned_8_16(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    if (CodeGen->isGC2100 && !CodeGen->hasBugFixes11)
    {
        gcSL_FORMAT format = gcmSL_TARGET_GET(Instruction->temp, Format);

        switch (format)
        {
        case gcSL_UINT8:
        case gcSL_UINT16:
            return gcvTRUE;

        default:
            return gcvFALSE;
        }
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL
_isGC2100Signed_8_16_store(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    if (CodeGen->isGC2100 && !CodeGen->hasBugFixes11)
    {
        gcSL_FORMAT format = gcmSL_TARGET_GET(Instruction->temp, Format);

        switch (format)
        {
        case gcSL_INT8:
            if (Tree->tempArray[Instruction->tempIndex].format != gcSL_INT8)
            {
                return gcvTRUE;
            }
            return gcvFALSE;

        case gcSL_INT16:
            if (Tree->tempArray[Instruction->tempIndex].format != gcSL_INT8
            &&  Tree->tempArray[Instruction->tempIndex].format != gcSL_INT16)
            {
                return gcvTRUE;
            }
            return gcvFALSE;

        default:
            return gcvFALSE;
        }
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL
_isGC2100Unsigned_8_16_store(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    if (CodeGen->isGC2100 && !CodeGen->hasBugFixes11)
    {
        gcSL_FORMAT format = gcmSL_TARGET_GET(Instruction->temp, Format);

        switch (format)
        {
        case gcSL_UINT8:
            if (Tree->tempArray[Instruction->tempIndex].format != gcSL_UINT8)
            {
                return gcvTRUE;
            }
            return gcvFALSE;

        case gcSL_UINT16:
            if (Tree->tempArray[Instruction->tempIndex].format != gcSL_UINT8
            &&  Tree->tempArray[Instruction->tempIndex].format != gcSL_UINT16)
            {
                return gcvTRUE;
            }
            return gcvFALSE;

        default:
            return gcvFALSE;
        }
    }
    else
    {
        return gcvFALSE;
    }
}

gctBOOL
_isCLShader(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    )
{
    return CodeGen->clShader;
}

gctBOOL
_codeHasCaller(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen
    )
{
    return (Tree->hints[CodeGen->nextSource - 1].callers != gcvNULL);
}

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
extern void
_DumpShader(
    IN gctUINT32_PTR States,
    IN gctUINT32 StateBufferOffset,
    IN gctBOOL OutputFormat,
    IN gctUINT InstBase,
    IN gctUINT InstMax
    );

static void
_DumpUniform(
    IN gctUINT32 Address,
    IN gctFLOAT Value
    )
{
    gctUINT32 base = ((Address >= 0x1C00) &&
                       (Address < 0x1C00 + 1024) )
                   ? 0x1C00
                   : 0x1400;

    gctUINT32 index   = (Address - base) >> 2;
    gctUINT32 swizzle = (Address - base) &  3;

    const char * shader = (base == 0x1C00) ? "Pixel" : "Vertex";
    const char enable[] = "xyzw";

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                  "Uniform: %s Shader %u.%c = %f (0x%08X)",
                  shader,
                  index,
                  enable[swizzle],
                  Value,
                  *(gctUINT32_PTR) &Value);
}

static void
_DumpUniforms(
    IN gctUINT32_PTR StateBuffer,
    IN gctUINT32_PTR LastState
    )
{
    while (StateBuffer < LastState)
    {
        gctUINT32 state = *StateBuffer++;
        gctUINT32 address, count, i;

        gcmASSERT(((((gctUINT32) (state)) >> (0 ? 31:27) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1)))))) == (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))));

        address = (((((gctUINT32) (state)) >> (0 ? 15:0)) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1)))))) );
        count   = (((((gctUINT32) (state)) >> (0 ? 25:16)) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1)))))) );

        if ((address >= 0x1C00) &&
             (address < 0x1C00 + 1024) )
        {
            for (i = 0; i < count; ++i)
            {
                _DumpUniform(address++, *(gctFLOAT *) StateBuffer);
                ++StateBuffer;
            }

            if ((count & 1) == 0) ++StateBuffer;
        }

        else if ((address >= 0x1400) &&
                  (address < 0x1400 + 1024) )
        {
            for (i = 0; i < count; ++i)
            {
                _DumpUniform(address++, *(gctFLOAT *) StateBuffer);
                ++StateBuffer;
            }

            if ((count & 1) == 0) ++StateBuffer;
        }

        else
        {
            StateBuffer += count;
            if ((count & 1) == 0) ++StateBuffer;
        }
    }
}
#endif

extern gcsSL_PATTERN patterns[];

static gceSTATUS
_FindUsage(
    IN OUT gcsSL_USAGE_PTR Usage,
    IN gctSIZE_T Count,
    IN gcSHADER_TYPE Type,
    IN gctINT Length,
    IN gctINT LastUse,
    IN gctBOOL Restricted,
    OUT gctINT * Physical,
    OUT gctUINT8 * Swizzle,
    OUT gctINT * Shift,
    OUT gctUINT8 * Enable
    );


static gceSTATUS
_AllocateConst(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT8 Usage,
    IN gctFLOAT Constants[4],
    OUT gctINT_PTR Index,
    OUT gctUINT8_PTR Swizzle
    )
{
    gcsSL_CONSTANT_TABLE_PTR c;
    gceSTATUS status;
    gctINT shift, index = 0, count;
    gctBOOL valid[4];
    gctUINT8 swizzle = 0;

    valid[0] = (Usage & gcSL_ENABLE_X) ? gcvTRUE : gcvFALSE;
    valid[1] = (Usage & gcSL_ENABLE_Y) ? gcvTRUE : gcvFALSE;
    valid[2] = (Usage & gcSL_ENABLE_Z) ? gcvTRUE : gcvFALSE;
    valid[3] = (Usage & gcSL_ENABLE_W) ? gcvTRUE : gcvFALSE;
    count    = (Usage & gcSL_ENABLE_W) ? 4
             : (Usage & gcSL_ENABLE_Z) ? 3
             : (Usage & gcSL_ENABLE_Y) ? 2
                                       : 1;

    do
    {
        /* Walk all costants. */
        for (c = CodeGen->constants; c != gcvNULL; c = c->next)
        {
            gctBOOL match = gcvFALSE;

            if (count > c->count)
            {
                continue;
            }

            switch (count)
            {
            case 1:
                for (index = 0; index < c->count; ++index)
                {
                   /* if (c->constant[index] == Constants[0]) */
                    if (*((int *) (&c->constant[index])) == *((int *) (&Constants[0])))
                    {
                        match = gcvTRUE;
                        break;
                    }
                }
                break;

            case 2:
                for (index = 0; index < c->count - 1; ++index)
                {
                    if ((!valid[0] || (c->constant[index + 0] == Constants[0]))
                    &&  (!valid[1] || (c->constant[index + 1] == Constants[1]))
                    )
                    {
                        match = gcvTRUE;
                        break;
                    }
                }
                break;

            case 3:
                for (index = 0; index < c->count - 2; ++index)
                {
                    if ((!valid[0] || (c->constant[index + 0] == Constants[0]))
                    &&  (!valid[1] || (c->constant[index + 1] == Constants[1]))
                    &&  (!valid[2] || (c->constant[index + 2] == Constants[2]))
                    )
                    {
                        match = gcvTRUE;
                        break;
                    }
                }
                break;

            case 4:
                index = 0;

                if ((!valid[0] || (c->constant[0] == Constants[0]))
                &&  (!valid[1] || (c->constant[1] == Constants[1]))
                &&  (!valid[2] || (c->constant[2] == Constants[2]))
                &&  (!valid[3] || (c->constant[3] == Constants[3]))
                )
                {
                    match = gcvTRUE;
                }
                break;
            }

            if (match)
            {
                break;
            }
        }

        if (c == gcvNULL)
        {
            gctPOINTER pointer = gcvNULL;

            /* Allocate a new constant. */
            gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                        sizeof(gcsSL_CONSTANT_TABLE),
                                        &pointer));

            c = pointer;

            /* Initialize the constant. */
            c->next        = CodeGen->constants;
            c->count       = count;
            for (index = 0; index < count; ++index)
            {
                c->constant[index] = Constants[index];
            }

            /* Link constant to head of tree. */
            CodeGen->constants = c;

            /* Allocate a physical spot for the uniform. */
            gcmERR_BREAK(_FindUsage(CodeGen->uniformUsage,
                                    CodeGen->uniformCount,
                                    (count == 1)   ? gcSHADER_FLOAT_X1
                                    : (count == 2) ? gcSHADER_FLOAT_X2
                                    : (count == 3) ? gcSHADER_FLOAT_X3
                                                   : gcSHADER_FLOAT_X4,
                                    1,
                                    gcvSL_RESERVED,
                                    gcvFALSE,
                                    &c->index,
                                    &c->swizzle,
                                    &shift,
                                    gcvNULL));

            index = 0;
        }

        /* Return constant index and swizzle. */
        *Index = c->index;

        swizzle = c->swizzle >> (index * 2);
        switch (count)
        {
        case 1:
            swizzle = (swizzle & 0x03) | ((swizzle & 0x03) << 2);
            /* fall through */
        case 2:
            swizzle = (swizzle & 0x0F) | ((swizzle & 0x0C) << 2);
            /* fall through */
        case 3:
            swizzle = (swizzle & 0x3F) | ((swizzle & 0x30) << 2);
            /* fall through */
        default:
            break;
        }

        *Swizzle = swizzle;

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gcSL_SWIZZLE
_ExtractSwizzle(
    IN gctUINT8 Swizzle,
    IN gctUINT Index
    )
{
    switch (Index)
    {
    case 0:
        return (gcSL_SWIZZLE) ((Swizzle >> 0) & 0x3);

    case 1:
        return (gcSL_SWIZZLE) ((Swizzle >> 2) & 0x3);

    case 2:
        return (gcSL_SWIZZLE) ((Swizzle >> 4) & 0x3);

    case 3:
        return (gcSL_SWIZZLE) ((Swizzle >> 6) & 0x3);

    default:
        break;
    }

    gcmFATAL("Invalid swizzle index %d.", Index);
    return gcSL_SWIZZLE_INVALID;
}

static gctUINT8
_Swizzle2Enable(
    IN gcSL_SWIZZLE X,
    IN gcSL_SWIZZLE Y,
    IN gcSL_SWIZZLE Z,
    IN gcSL_SWIZZLE W
    )
{
    static gctUINT8 _enable[] =
    {
        gcSL_ENABLE_X,
        gcSL_ENABLE_Y,
        gcSL_ENABLE_Z,
        gcSL_ENABLE_W
    };

    /* Return combined enables for each swizzle. */
    return _enable[X] | _enable[Y] | _enable[Z] | _enable[W];
}

static gctUINT8
_AdjustSwizzle(
    IN gctUINT8 Swizzle,
    IN gctUINT32 AssignedSwizzle
    )
{
    /* Decode assigned swizzles. */
    gctUINT8 swizzle[4];
    swizzle[0] = (AssignedSwizzle >> 0) & 0x3;
    swizzle[1] = (AssignedSwizzle >> 2) & 0x3;
    swizzle[2] = (AssignedSwizzle >> 4) & 0x3;
    swizzle[3] = (AssignedSwizzle >> 6) & 0x3;

    /* Convert swizzles to their assigned values. */
    return (swizzle[(Swizzle >> 0) & 0x3] << 0) |
           (swizzle[(Swizzle >> 2) & 0x3] << 2) |
           (swizzle[(Swizzle >> 4) & 0x3] << 4) |
           (swizzle[(Swizzle >> 6) & 0x3] << 6);
}

static gcSL_SWIZZLE
_SingleEnable2Swizzle(
    IN gctUINT8 Enable
    )
{
    switch (Enable)
    {
    case gcSL_ENABLE_X:
        return gcSL_SWIZZLE_XXXX;

    case gcSL_ENABLE_Y:
        return gcSL_SWIZZLE_YYYY;

    case gcSL_ENABLE_Z:
        return gcSL_SWIZZLE_ZZZZ;

    case gcSL_ENABLE_W:
        return gcSL_SWIZZLE_WWWW;

    default:
        break;
    }

    gcmFATAL("Invalid single enable 0x%x.", Enable);
    return gcSL_SWIZZLE_INVALID;
}

gctBOOL
_GetPreviousCode(
    IN gcsCODE_GENERATOR_PTR CodeGen,
    OUT gctUINT32_PTR * Code
)
{
    if (CodeGen->previousCode != gcvNULL)
    {
        *Code = CodeGen->previousCode;
        return gcvTRUE;
    }

    return gcvFALSE;
}

static gctBOOL
_AddReference(
    IN gcsSL_FUNCTION_CODE_PTR Function,
    IN gctINT Reference,
    IN gcSL_INSTRUCTION Instruction,
    IN gctINT SourceIndex
    )
{
    gctSIZE_T r;

    /* Process all referecnes. */
    for (r = 0; r < gcmCOUNTOF(Function->references); ++r)
    {
        /* See if we have an empty slot. */
        if (Function->references[r].index == 0)
        {
            /* Save reference. */
            Function->references[r].index       = Reference;
            Function->references[r].instruction = Instruction;
            Function->references[r].sourceIndex = SourceIndex;

            /* Success. */
            return gcvTRUE;
        }
    }

    /* No more empty slots. */
    return gcvFALSE;
}

static gctUINT
_Bits(
    IN gcsSL_FUNCTION_CODE_PTR Function,
    IN gctINT Reference
    )
{
    gcsSL_REFERENCE_PTR match = gcvNULL;
    gctSIZE_T r;
    gctUINT16 enable;
    gctUINT bits;

    if (Reference == 0)
    {
        return 0;
    }

    switch (Reference)
    {
    case gcSL_CG_TEMP1:
        /* fall through */
    case gcSL_CG_TEMP1_X:
        /* fall through */
    case gcSL_CG_TEMP1_XY:
        /* fall through */
    case gcSL_CG_TEMP1_XYZ:
        /* fall through */
    case gcSL_CG_TEMP1_XYZW:
        match = &Function->tempReferences[0];
        break;

    case gcSL_CG_TEMP2:
        /* fall through */
    case gcSL_CG_TEMP2_X:
        /* fall through */
    case gcSL_CG_TEMP2_XY:
        /* fall through */
    case gcSL_CG_TEMP2_XYZ:
        /* fall through */
    case gcSL_CG_TEMP2_XYZW:
        match = &Function->tempReferences[1];
        break;

    case gcSL_CG_TEMP3:
        /* fall through */
    case gcSL_CG_TEMP3_X:
        /* fall through */
    case gcSL_CG_TEMP3_XY:
        /* fall through */
    case gcSL_CG_TEMP3_XYZ:
        /* fall through */
    case gcSL_CG_TEMP3_XYZW:
        match = &Function->tempReferences[2];
        break;

    default:
        if (Reference < 0)
        {
            Reference = -Reference;
        }
        for (r = 0; r < gcmCOUNTOF(Function->references); ++r)
        {
            if (Function->references[r].index == Reference)
            {
                match = &Function->references[r];
                break;
            }
        }
    }

    if (match == gcvNULL || match->instruction == gcvNULL)
    {
        return 0;
    }
#if !_USE_ORIGINAL_DST_WRITE_MASK_
    if (match->sourceIndex == -1)
    {
        /* Get the referenced destination. */
        enable = gcmSL_TARGET_GET(match->instruction->temp, Enable);
    }
    else
    {
        /* Get the referenced source. */
        gctUINT16 source = (match->sourceIndex == 0)
            ? match->instruction->source0
            : match->instruction->source1;

        enable = _Swizzle2Enable((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
                                 (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
                                 (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
                                 (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));
    }

    bits = 0;

    if (enable & 0x1) ++bits;
    if (enable & 0x2) ++bits;
    if (enable & 0x4) ++bits;
    if (enable & 0x8) ++bits;
#else
	enable = gcmSL_TARGET_GET(match->instruction->temp, Enable);
    bits = enable;
#endif

    return bits;
}

static gctBOOL
_FindReference(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctINT Reference,
    OUT gcsSL_REFERENCE_PTR * Match,
    IN gcsSL_PATTERN_PTR Pattern
    )
{
    gctSIZE_T r;
    gceSTATUS status;
    gcSHADER_TYPE type = (gcSHADER_TYPE) 0;
    gctINT index;
    gctUINT8 swizzle;
    gctUINT8 enable;
    gctUINT16 target;
    gcSL_INSTRUCTION instruction;
    gcsSL_REFERENCE_PTR reference;
    gctINT_PTR shift;
    gcsSL_FUNCTION_CODE_PTR function = CodeGen->current;

    switch (Reference)
    {
    case gcSL_CG_TEMP1:
        /* fall through */
    case gcSL_CG_TEMP1_X:
        /* fall through */
    case gcSL_CG_TEMP1_XY:
        /* fall through */
    case gcSL_CG_TEMP1_XYZ:
        /* fall through */
    case gcSL_CG_TEMP1_XYZW:
        reference   = &function->tempReferences[0];
        instruction = &function->tempInstruction[0];
        shift       = &function->tempShift[0];
        break;

    case gcSL_CG_TEMP2:
        /* fall through */
    case gcSL_CG_TEMP2_X:
        /* fall through */
    case gcSL_CG_TEMP2_XY:
        /* fall through */
    case gcSL_CG_TEMP2_XYZ:
        /* fall through */
    case gcSL_CG_TEMP2_XYZW:
        reference   = &function->tempReferences[1];
        instruction = &function->tempInstruction[1];
        shift       = &function->tempShift[1];
        break;

    case gcSL_CG_TEMP3:
        /* fall through */
    case gcSL_CG_TEMP3_X:
        /* fall through */
    case gcSL_CG_TEMP3_XY:
        /* fall through */
    case gcSL_CG_TEMP3_XYZ:
        /* fall through */
    case gcSL_CG_TEMP3_XYZW:
        reference   = &function->tempReferences[2];
        instruction = &function->tempInstruction[2];
        shift       = &function->tempShift[2];
        break;

    default:
        /* Look in all references. */
        for (r = 0; r < gcmCOUNTOF(function->references); ++r)
        {
            /* See if reference matches. */
            if (function->references[r].index == Reference)
            {
                /* Return match. */
                *Match = &function->references[r];

                /* Success. */
                return gcvTRUE;
            }
        }

        /* Reference not found. */
        return gcvFALSE;
    }

    /* See if the temporary register needs to be allocated. */
    if (reference->instruction == gcvNULL)
    {
        gctUINT bits = 0;
        gctUINT bitsSrc1 = 0;
        gctUINT bitsSrc2 = 0;
        gctINT lastUse;

        /* Get number of components. */
        switch (Reference)
        {
        case gcSL_CG_TEMP1:
        /* fall through */
        case gcSL_CG_TEMP2:
        /* fall through */
        case gcSL_CG_TEMP3:
            if (Pattern == gcvNULL)
            {
                return gcvFALSE;
            }

            bits = _Bits(function, Pattern->source0);
            bitsSrc1 = _Bits(function, Pattern->source1);
            bitsSrc2 = _Bits(function, Pattern->source2);
            bits = gcmMAX(bits, bitsSrc1);
            bits = gcmMAX(bits, bitsSrc2);

#if !_USE_ORIGINAL_DST_WRITE_MASK_
            switch (bits)
            {
            case 1:
                type = gcSHADER_FLOAT_X1;
                break;

            case 2:
                type = gcSHADER_FLOAT_X2;
                break;

            case 3:
                type = gcSHADER_FLOAT_X3;
                break;

            case 4:
                type = gcSHADER_FLOAT_X4;
                break;

            default:
                break;
            }
#else
		    switch (bits)
		    {
		    case 0x1:
		        /* 1-component temporary register. */
		        type = gcSHADER_FLOAT_X1;
		        break;

		    case 0x2:
		    /* fall through */
		    case 0x3:
		        /* 2-component temporary register. */
		        type = gcSHADER_FLOAT_X2;
		        break;

		    case 0x4:
		    /* fall through */
		    case 0x5:
		    /* fall through */
		    case 0x6:
		    /* fall through */
		    case 0x7:
		        /* 3-component temporary register. */
		        type = gcSHADER_FLOAT_X3;
		        break;

		    case 0x8:
		    /* fall through */
		    case 0x9:
		    /* fall through */
		    case 0xA:
		    /* fall through */
		    case 0xB:
		    /* fall through */
		    case 0xC:
		    /* fall through */
		    case 0xD:
		    /* fall through */
		    case 0xE:
		    /* fall through */
		    case 0xF:
		        /* 4-component temporary register. */
		        type = gcSHADER_FLOAT_X4;
		        break;

		    default:
		        /* Special case for uninitialized variables. */
		        type = gcSHADER_FLOAT_X1;
		        break;
		    }
#endif
            break;

        case gcSL_CG_TEMP1_X:
            /* fall through */
        case gcSL_CG_TEMP2_X:
            /* fall through */
        case gcSL_CG_TEMP3_X:
            /* One component. */
            type = gcSHADER_FLOAT_X1;
            break;

        case gcSL_CG_TEMP1_XY:
            /* fall through */
        case gcSL_CG_TEMP2_XY:
            /* fall through */
        case gcSL_CG_TEMP3_XY:
            /* Two components. */
            type = gcSHADER_FLOAT_X2;
            break;

        case gcSL_CG_TEMP1_XYZ:
            /* fall through */
        case gcSL_CG_TEMP2_XYZ:
            /* fall through */
        case gcSL_CG_TEMP3_XYZ:
            /* Three components. */
            type = gcSHADER_FLOAT_X3;
            break;

        case gcSL_CG_TEMP1_XYZW:
            /* fall through */
        case gcSL_CG_TEMP2_XYZW:
            /* fall through */
        case gcSL_CG_TEMP3_XYZW:
            /* Four components. */
            type = gcSHADER_FLOAT_X4;
            break;

        default:
            break;
        }

        /* Find an empty spot for the temporary register. */
        lastUse = (Tree->hints[CodeGen->nextSource - 1].lastUseForTemp == (gctINT) CodeGen->nextSource - 1) ?
                  (gctINT) CodeGen->nextSource - 1 : Tree->hints[CodeGen->nextSource - 1].lastUseForTemp;
        status = _FindUsage(CodeGen->registerUsage,
                            CodeGen->registerCount,
                            type,
                            1,
                            lastUse /* CodeGen->nextSource - 1 */,
                            (bits > 1),
                            &index,
                            &swizzle,
                            shift,
                            &enable);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            return gcvFALSE;
        }

        /* Setup instruction. */
        gcmASSERT(enable == _Swizzle2Enable(_ExtractSwizzle(swizzle, 0),
                                            _ExtractSwizzle(swizzle, 1),
                                            _ExtractSwizzle(swizzle, 2),
                                            _ExtractSwizzle(swizzle, 3)));
#if !_USE_ORIGINAL_DST_WRITE_MASK_
        target = gcmSL_TARGET_SET(0, Enable, enable)
#else
        target = gcmSL_TARGET_SET(0, Enable, (bits == 0) ? enable : (gctUINT8)(bits << *shift))
#endif
               | gcmSL_TARGET_SET(0, Indexed, gcSL_NOT_INDEXED)
               | gcmSL_TARGET_SET(0, Condition, gcSL_ALWAYS)
               | gcmSL_TARGET_SET(0, Format, gcSL_FLOAT);

        /* Setup instruction. */
        instruction->temp        = target;
        instruction->tempIndex   = (gctUINT16) (-1 - index);
        instruction->tempIndexed = 0;

        /* Setup reference. */
        reference->instruction = instruction;
        reference->sourceIndex = -1;
    }

    /* Return reference. */
    *Match = reference;

    /* Success. */
    return gcvTRUE;
}

static gctUINT8
_ChangeSwizzleForInstCombine(
    IN gctUINT8 usageSwizzle,
    IN gctUINT8 defWriteMask,
    IN gctUINT8 defSwizzle)
{
    gctINT i;
    gctUINT enabledChannelIdx, newChannelSwizzle;
    gctUINT8 newSwizzle = 0, swizzle2Mask = 0;

    for (i = 0; i < 4; i ++)
    {
        /* Get enable channel index from usage swizzle */
        enabledChannelIdx = (usageSwizzle >> (i*2)) & 0x3;

        swizzle2Mask |= (1 << enabledChannelIdx);

        /* Get channel swizzle from source of def based on enabled channel */
        newChannelSwizzle = (defSwizzle >> (enabledChannelIdx*2)) & 0x3;

        /* Set this new channel swizzle */
        newSwizzle |= (newChannelSwizzle << (i*2));
    }

    /*  Only valid for same enable */
    gcmASSERT(swizzle2Mask == defWriteMask);

    return newSwizzle;
}

static gctBOOL
_CompareInstruction(
    IN gcSL_INSTRUCTION TargetSource,
    IN gctINT TargetSourceIndex,
    IN gcSL_INSTRUCTION Source,
    IN gctINT SourceIndex
    )
{
    gctUINT16 target = TargetSource->temp;

    /* Extract proper source values. */
    gctUINT16 source  = (SourceIndex == 0)
                             ? Source->source0
                             : Source->source1;
    gctUINT16 index   = (SourceIndex == 0)
                             ? Source->source0Index
                             : Source->source1Index;
    gctUINT16 indexed = (SourceIndex == 0)
                             ? Source->source0Indexed
                             : Source->source1Indexed;

    /* Dispatch on target index. */
    switch (TargetSourceIndex)
    {
    case -1:
        /* Compare with destination: source must be a temp */
        if ((gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
        &&   /* With the same index. */
             (index == TargetSource->tempIndex)
        &&   /* Of the same format. */
             (((gcmSL_SOURCE_GET(source, Format) == gcSL_FLOAT) &&
               (gcmSL_TARGET_GET(target, Format) == gcSL_FLOAT)) ||
              ((gcmSL_SOURCE_GET(source, Format) != gcSL_FLOAT) &&
               (gcmSL_TARGET_GET(target, Format) != gcSL_FLOAT)))
        &&   /* The swizzle must match the enables. */
             (_Swizzle2Enable((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
                              (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
                              (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
                              (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW)) == gcmSL_TARGET_GET(target, Enable))
        &&   /* With the same index mode. */
             (gcmSL_SOURCE_GET(source, Indexed) == gcmSL_TARGET_GET(target, Indexed))
        &&   /* And indexed register. */
             (indexed == TargetSource->tempIndexed)
        )
        {
            /* Only swizzle matches write-enable is not enough, for example,
               mul r0.xy, r1.xy, r2.xy
               add r3.zw, r0.xyxy, r3.zwzw
               although dest of mul and source1 of add has same swizzle/enable,
               but we can not directly say they can be merged to mad unless we
               do some changes for source of mul.
            */
            gctUINT8 newSwizzle;
            gctUINT16 source0, source1;

            /* source 0 */
            newSwizzle = _ChangeSwizzleForInstCombine(
                                            gcmSL_SOURCE_GET(source, Swizzle),
                                            gcmSL_TARGET_GET(target, Enable),
                                            gcmSL_SOURCE_GET(TargetSource->source0, Swizzle));

            source0 = gcmSL_SOURCE_SET(TargetSource->source0, Swizzle, newSwizzle);
            TargetSource->source0 = source0;

            /* source 1 */
            newSwizzle = _ChangeSwizzleForInstCombine(
                                            gcmSL_SOURCE_GET(source, Swizzle),
                                            gcmSL_TARGET_GET(target, Enable),
                                            gcmSL_SOURCE_GET(TargetSource->source1, Swizzle));

            source1 = gcmSL_SOURCE_SET(TargetSource->source1, Swizzle, newSwizzle);
            TargetSource->source1 = source1;

            /* We have a match. */
            return gcvTRUE;
        }
        break;

    case 0:
        /* Compare with source 0. */
        if ((source  == Source->source0)
        &&  (index   == Source->source0Index)
        &&  (indexed == Source->source0Indexed)
        )
        {
            /* We have a match. */
            return gcvTRUE;
        }
        break;

    case 1:
        /* Compare with source 1. */
        if ((source  == Source->source1)
        &&  (index   == Source->source1Index)
        &&  (indexed == Source->source1Indexed)
        )
        {
            /* We have a match. */
            return gcvTRUE;
        }
        break;

    default:
        break;
    }

    /* No match. */
    return gcvFALSE;
}

static gcsSL_PATTERN_PTR
_FindPattern(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcsSL_PATTERN_PTR Patterns,
    IN gcSL_INSTRUCTION Code,
    IN OUT gctINT * CodeCount
    )
{
    gcsSL_PATTERN_PTR p;
    gceSTATUS status;
    gctSIZE_T index;
    gcsSL_REFERENCE_PTR match;
    gctINT instructions = 0;
    gcsSL_FUNCTION_CODE_PTR function = CodeGen->current;
    gctUINT loopCount, loopCountA = 0;

    /* Process all patterns. */
    for (p = Patterns, loopCount = 0; p->count > 0 && loopCount < MAX_LOOP_COUNT; loopCount++)
    {
        /* Zero the references. */
        gcmERR_BREAK(gcoOS_ZeroMemory(function->references,
                                      gcmSIZEOF(function->references)));
        gcmERR_BREAK(gcoOS_ZeroMemory(function->tempReferences,
                                      gcmSIZEOF(function->tempReferences)));

        /* Skip those search patterns that have more instructions than are
           available. */
        if (p->count <= (gctINT) *CodeCount)
        {
            /* Start a new search. */
            index        = 0;
            instructions = p->count;

            /* Process all search patterns. */
            while (p->count > 0 && loopCountA < MAX_LOOP_COUNT)
            {
                loopCountA++;
                /* Bail out of the opcode doesn't match. */
                if (p->opcode != Code[index].opcode)
                {
                    break;
                }

                /* Add reference for destination. */
                if (p->dest != 0)
                {
                    _AddReference(function,
                                  p->dest,
                                  Code + index,
                                  -1);

                    if (p->function != gcvNULL)
                    {
                        if (!(*p->function)(Tree, CodeGen, Code + index, gcvNULL))
                        {
                            /* Bail out of function returns gcvFALSE. */
                            break;
                        }
                    }
                }

                /* Process source 0 search pattern. */
                if (p->source0 != 0)
                {
                    /* Check is source 0 reference is already used. */
                    if (_FindReference(Tree, CodeGen,
                                       gcmABS(p->source0),
                                       &match,
                                       gcvNULL))
                    {
                        /* Compare source 0 reference. */
                        if (!_CompareInstruction(match->instruction,
                                                 match->sourceIndex,
                                                 Code + index,
                                                 0))
                        {
                            /* Bail out if source 0 doesn't match. */
                            break;
                        }
                    }

                    else
                    {
                        /* Add source 0 reference. */
                        if (!_AddReference(function,
                                           p->source0,
                                           Code + index,
                                           0))
                        {
                            /* Bail out of on any error. */
                            break;
                        }
                    }
                }

                /* Process source 1 search pattern. */
                if (p->source1 != 0)
                {
                    /* Check is source 1 reference is already used. */
                    if (_FindReference(Tree, CodeGen,
                                       gcmABS(p->source1),
                                       &match,
                                       gcvNULL))
                    {
                        /* Compare source 1 reference. */
                        if (!_CompareInstruction(match->instruction,
                                                 match->sourceIndex,
                                                 Code + index,
                                                 1))
                        {
                            /* Bail out if source 1 doesn't match. */
                            break;
                        }
                    }
                    else
                    {
                        /* Add source 1 reference. */
                        if (!_AddReference(function,
                                           p->source1,
                                           Code + index,
                                           1))
                        {
                            /* Bail out of on any error. */
                            break;
                        }
                    }
                }

                if ((index > 0)
                &&  (p->source0 != 0)
                &&  (p->source1 != 0)
                &&  (gcmABS(p->source0) != gcmABS(p->source1))
                )
                {
                    if (_FindReference(Tree, CodeGen,
                                       gcmABS(p->source0),
                                       &match,
                                       gcvNULL))
                    {
                        if (_CompareInstruction(match->instruction,
                                                match->sourceIndex,
                                                Code + index,
                                                1))
                        {
                            /* Bail out if the sources match. */
                            break;
                        }
                    }
                }

                /* Next pattern. */
                ++p;
                ++index;
            }
        }

        if (p->count < 0)
        {
            /* Bail if we have a matching pattern. */
            break;
        }

        /* Skip over pattern matches. */
        p += p->count;

        /* Skip over code generation. */
        p += -p->count;
    }

    /* Test for unsupported pattern. */
    if (p->count == 0)
    {
        gcmFATAL("!!TODO!! Code pattern not yet supported.");
        p = gcvNULL;
    }

    /* Return number of matched instructions. */
    *CodeCount = instructions;

    /* Return pattern pointer. */
    return p;
}

static gctBOOL
_IsAvailable(
    IN gcsSL_USAGE_PTR Usage,
    IN gctINT Rows,
    IN gctUINT8 Enable
    )
{
    /* Loop through all rows. */
    while (Rows-- > 0)
    {
        /* Test if x-component is available. */
        if ((Enable & 0x1) && (Usage->lastUse[0] != gcvSL_AVAILABLE) )
        {
            return gcvFALSE;
        }

        /* Test if y-component is available. */
        if ((Enable & 0x2) && (Usage->lastUse[1] != gcvSL_AVAILABLE) )
        {
            return gcvFALSE;
        }

        /* Test if z-component is available. */
        if ((Enable & 0x4) && (Usage->lastUse[2] != gcvSL_AVAILABLE) )
        {
            return gcvFALSE;
        }

        /* Test if w-component is available. */
        if ((Enable & 0x8) && (Usage->lastUse[3] != gcvSL_AVAILABLE) )
        {
            return gcvFALSE;
        }

        /* Test next row. */
        ++Usage;
    }

    /* All requested rows and components are available. */
    return gcvTRUE;
}

static void
_SetUsage(
    IN gcsSL_USAGE_PTR Usage,
    IN gctINT Rows,
    IN gctUINT8 Enable,
    IN gctINT LastUse
    )
{
    /* Process all rows. */
    while (Rows-- > 0)
    {
        /* Set last usage for x-component if enabled. */
        if (Enable & gcSL_ENABLE_X)
        {
            Usage->lastUse[0] = LastUse;
        }

        /* Set last usage for y-component if enabled. */
        if (Enable & gcSL_ENABLE_Y)
        {
            Usage->lastUse[1] = LastUse;
        }

        /* Set last usage for z-component if enabled. */
        if (Enable & gcSL_ENABLE_Z)
        {
            Usage->lastUse[2] = LastUse;
        }

        /* Set last usage for w-component if enabled. */
        if (Enable & gcSL_ENABLE_W)
        {
            Usage->lastUse[3] = LastUse;
        }

        /* Process next row. */
        ++Usage;
    }
}

void
_ConvertType(
    IN gcSHADER_TYPE Type,
    IN gctINT Length,
    OUT gctINT * Components,
    OUT gctINT * Rows
    )
{
    /* Determine the number of required rows and components. */
    switch (Type)
    {
    case gcSHADER_FLOAT_X1:
        /* fall through */
    case gcSHADER_BOOLEAN_X1:
        /* fall through */
    case gcSHADER_INTEGER_X1:
        /* fall through */
    case gcSHADER_FIXED_X1:
        /* 1 component, 1 row. */
        *Components = 1;
        *Rows       = Length;
        break;

    case gcSHADER_FLOAT_X2:
        /* fall through */
    case gcSHADER_BOOLEAN_X2:
        /* fall through */
    case gcSHADER_INTEGER_X2:
        /* fall through */
    case gcSHADER_FIXED_X2:
        /* 2 components, 1 row. */
        *Components = 2;
        *Rows       = Length;
        break;

    case gcSHADER_FLOAT_X3:
        /* fall through */
    case gcSHADER_BOOLEAN_X3:
        /* fall through */
    case gcSHADER_INTEGER_X3:
        /* fall through */
    case gcSHADER_FIXED_X3:
        /* 3 components, 1 row. */
        *Components = 3;
        *Rows       = Length;
        break;

    case gcSHADER_FLOAT_X4:
        /* fall through */
    case gcSHADER_BOOLEAN_X4:
        /* fall through */
    case gcSHADER_INTEGER_X4:
        /* fall through */
    case gcSHADER_FIXED_X4:
        /* 4 components, 1 row. */
        *Components = 4;
        *Rows       = Length;
        break;

    case gcSHADER_FLOAT_2X2:
        /* 2 components, 2 rows. */
        *Components = 2;
        *Rows       = 2 * Length;
        break;

    case gcSHADER_FLOAT_3X3:
        /* 3 components, 3 rows. */
        *Components = 3;
        *Rows       = 3 * Length;
        break;

    case gcSHADER_FLOAT_4X4:
        /* 4 components, 4 rows. */
        *Components = 4;
        *Rows       = 4 * Length;
        break;

    case gcSHADER_IMAGE_2D:
    case gcSHADER_IMAGE_3D:
    case gcSHADER_SAMPLER:
        /* 1 component, 1 row. */
        *Components = 1;
        *Rows       = Length;
        break;

    default:
        break;
    }
}

static gceSTATUS
_FindUsage(
    IN OUT gcsSL_USAGE_PTR Usage,
    IN gctSIZE_T Count,
    IN gcSHADER_TYPE Type,
    IN gctINT Length,
    IN gctINT LastUse,
    IN gctBOOL Restricted,
    OUT gctINT_PTR Physical,
    OUT gctUINT8_PTR Swizzle,
    OUT gctINT_PTR Shift,
    OUT gctUINT8_PTR Enable
    )
{
    gctSIZE_T i;
    gctINT components = 0, rows = 0, shift;
    gctUINT8 swizzle = 0, enable = 0;

    /* Determine the number of required rows and components. */
    _ConvertType(Type, Length, &components, &rows);

    if ((gctINT) Count < rows)
    {
        /* Not enough hardware resources. */
        return gcvSTATUS_OUT_OF_RESOURCES;
    }

    /* Walk through all possible usages. */
    for (i = 0; i <= Count - rows; ++i)
    {
        /* Assume there is no room. */
        shift = -1;

        /* Test number of required components. */
        switch (components)
        {
        case 1:
            /* See if x-component is available. */
            if (_IsAvailable(Usage + i, rows, 0x1 << 0))
            {
                shift   = 0;
                enable  = gcSL_ENABLE_X;
                swizzle = gcSL_SWIZZLE_XXXX;
            }

            /* See if y-component is available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x1 << 1))
            {
                shift   = 1;
                enable  = gcSL_ENABLE_Y;
                swizzle = gcSL_SWIZZLE_YYYY;
            }

            /* See if z-component is available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x1 << 2))
            {
                shift   = 2;
                enable  = gcSL_ENABLE_Z;
                swizzle = gcSL_SWIZZLE_ZZZZ;
            }

            /* See if w-component is available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x1 << 3))
            {
                shift   = 3;
                enable  = gcSL_ENABLE_W;
                swizzle = gcSL_SWIZZLE_WWWW;
            }

            break;

        case 2:
            /* See if x- and y-components are available. */
            if (_IsAvailable(Usage + i, rows, 0x3 << 0))
            {
                shift   = 0;
                enable  = gcSL_ENABLE_XY;
                swizzle = gcSL_SWIZZLE_XYYY;
            }

            /* See if y- and z-components are available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x3 << 1))
            {
                shift   = 1;
                enable  = gcSL_ENABLE_YZ;
                swizzle = gcSL_SWIZZLE_YZZZ;
            }

            /* See if z- and w-components are available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x3 << 2))
            {
                shift   = 2;
                enable  = gcSL_ENABLE_ZW;
                swizzle = gcSL_SWIZZLE_ZWWW;
            }

            break;

        case 3:
            /* See if x-, y- and z-components are available. */
            if (_IsAvailable(Usage + i, rows, 0x7 << 0))
            {
                shift   = 0;
                enable  = gcSL_ENABLE_XYZ;
                swizzle = gcSL_SWIZZLE_XYZZ;
            }

            /* See if y-, z- and w-components are available. */
            else if (!Restricted && _IsAvailable(Usage + i, rows, 0x7 << 1))
            {
                shift   = 1;
                enable  = gcSL_ENABLE_YZW;
                swizzle = gcSL_SWIZZLE_YZWW;
            }

            break;

        case 4:
            /* See if x-, y-, z- and w-components are available. */
            if (_IsAvailable(Usage + i, rows, 0xF << 0))
            {
                shift   = 0;
                enable  = gcSL_ENABLE_XYZW;
                swizzle = gcSL_SWIZZLE_XYZW;
            }

            break;

        default:
            break;
        }

        /* See if there is enough room. */
        if (shift >= 0)
        {
            /* Return allocation. */
            *Physical = i;
            *Shift    = shift;
            *Swizzle  = swizzle;
            if (Enable)
            {
                *Enable = enable;
            }

            /* Set the usage. */
            _SetUsage(Usage + i, rows, enable, LastUse);

            /* Success. */
            return gcvSTATUS_OK;
        }
    }

    /* Out of resources. */
    return gcvSTATUS_OUT_OF_RESOURCES;
}

static gceSTATUS
_MapUniforms(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN OUT gcsSL_USAGE_PTR Usage,
    IN gctSIZE_T UsageCount
    )
{
    gceSTATUS status;
    gctSIZE_T i;
    gctSIZE_T vsSamplers, psSamplers;

    /* Extract the gcSHADER object. */
    gcSHADER shader = Tree->shader;

    /* Determine base address for uniforms. */
    const gctUINT32 uniformBaseAddress =
        (Tree->shader->type == gcSHADER_TYPE_VERTEX)
            ? 0x05000
            : 0x07000;

    gctINT sampler, maxSampler;

    gcmONERROR(
        gcoHARDWARE_QuerySamplerBase(&vsSamplers,
                                     gcvNULL,
                                     &psSamplers,
                                     gcvNULL));

    /* Determine starting sampler index. */
    sampler = (Tree->shader->type == gcSHADER_TYPE_VERTEX)
            ? psSamplers
            : 0;

    /* Determine maximum sampler index. */
    maxSampler = (Tree->shader->type == gcSHADER_TYPE_VERTEX)
               ? psSamplers + vsSamplers
               : psSamplers;

    /* Map all uniforms. */
    for (i = 0; i < shader->uniformCount; ++i)
    {
        /* Get uniform. */
        gcUNIFORM uniform = shader->uniforms[i];
        gctINT shift;

        if(!uniform) continue;
        switch (uniform->type)
        {
        case gcSHADER_SAMPLER_1D:
        /* fall through */
        case gcSHADER_SAMPLER_2D:
        /* fall through */
        case gcSHADER_SAMPLER_3D:
        /* fall through */
        case gcSHADER_SAMPLER_EXTERNAL_OES:
        case gcSHADER_SAMPLER_CUBIC:
            /* Test if sampler is in range. */
            if (sampler >= maxSampler)
            {
                gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
            }
            else
            {
                /* Use next sampler. */
                uniform->physical = sampler++;
            }
            break;

        default:
            /* Find physical location for uniform. */
            gcmONERROR(_FindUsage(Usage, UsageCount,
                                  uniform->type,
                                  uniform->arraySize,
                                  gcvSL_RESERVED,
                                  gcvFALSE,
                                  &uniform->physical,
                                  &uniform->swizzle,
                                  &shift,
                                  gcvNULL));

            /* Set physical address for uniform. */
            uniform->address = uniformBaseAddress +
                               uniform->physical * 16 + shift * 4;
            break;
        }
    }

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_MapAttributes(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN OUT gcsSL_USAGE_PTR Usage
    )
{
    gctINT reg;
    gctSIZE_T i;
    gceSTATUS status;

    /* Extract the gcSHADER object. */
    gcSHADER shader = Tree->shader;

    do
    {
        if (shader->type != gcSHADER_TYPE_FRAGMENT)
        {
            /* Start at register 0 for vertex shaders. */
            reg = 0;
        }
        else
        {
            /* Start at register 1 for fragment shaders. */
            reg = 1;

            /* Mark register 0 as used (position). */
            Usage[0].lastUse[0] =
            Usage[0].lastUse[1] =
            Usage[0].lastUse[2] =
            Usage[0].lastUse[3] = gcvSL_RESERVED;
        }

        /* Process all attributes. */
        for (i = 0; i < shader->attributeCount; ++i)
        {
            /* Only process enabled attributes. */
            if (Tree->attributeArray[i].inUse)
            {
                /* Get attribute. */
                gcATTRIBUTE attribute = shader->attributes[i];
                gctINT components = 0, rows = 0;
                gctUINT8 enable = 0;

                /* Check for special POSITION attribute. */
                if (attribute->nameLength == gcSL_POSITION)
                {
                    gcmASSERT(shader->type == gcSHADER_TYPE_FRAGMENT);

                    attribute->inputIndex = 0;
                    CodeGen->usePosition  =
                        (CodeGen->flags & gcvSHADER_USE_GL_POSITION);
                    continue;
                }

                /* Check for special FRONT_FACING attribute. */
                if (attribute->nameLength == gcSL_FRONT_FACING)
                {
                    gcmASSERT(shader->type == gcSHADER_TYPE_FRAGMENT);

                    attribute->inputIndex = 0;
                    CodeGen->useFace      =
                        (CodeGen->flags & gcvSHADER_USE_GL_FACE);
                    continue;
                }

                /* Assign input register. */
                attribute->inputIndex = reg;

                /* Determine rows and components. */
                _ConvertType(attribute->type,
                             attribute->arraySize,
                             &components,
                             &rows);

                if (shader->type == gcSHADER_TYPE_VERTEX)
                {
                    /* Reserve all components for vertex shaders. */
                    enable = 0xF;
                }
                else
                {
                    /* Get the proper component enable bits. */
                    switch (components)
                    {
                    case 1:
                        enable = gcSL_ENABLE_X;
                        break;

                    case 2:
                        enable = gcSL_ENABLE_XY;
                        break;

                    case 3:
                        enable = gcSL_ENABLE_XYZ;
                        break;

                    case 4:
                        enable = gcSL_ENABLE_XYZW;
                        break;

                    default:
                        break;
                    }

                    if (attribute->nameLength == gcSL_POINT_COORD)
                    {
                        CodeGen->usePointCoord      =
                            (CodeGen->flags & gcvSHADER_USE_GL_POINT_COORD);
                        CodeGen->pointCoordPhysical = reg;
                    }
                }

                /* Set register usage. */
                _SetUsage(Usage + reg,
                          rows,
                          enable,
                          Tree->attributeArray[i].lastUse);

                /* Move to next register. */
                reg += rows;
            }
        }

        if (CodeGen->clShader && ! CodeGen->hasBugFixes10)
        {
            gcmASSERT(reg <= 3);
            CodeGen->reservedRegForLoad = reg;
            CodeGen->loadDestIndex = -1;
            CodeGen->origAssigned = -1;
            CodeGen->lastLoadUser = -1;

            /* Mark register 3 as used (position). */
            Usage[reg].lastUse[0] =
            Usage[reg].lastUse[1] =
            Usage[reg].lastUse[2] =
            Usage[reg].lastUse[3] = gcvSL_RESERVED;
        }
        else
        {
            CodeGen->reservedRegForLoad = ~0U;
            CodeGen->loadDestIndex = -1;
            CodeGen->origAssigned = -1;
            CodeGen->lastLoadUser = -1;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

extern gctUINT8
_Enable2Swizzle(
    IN gctUINT32 Enable
    );

extern gctUINT16
_SelectSwizzle(
    IN gctUINT16 Swizzle,
    IN gctUINT16 Source
    );

gceSTATUS
_AddConstantVec1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctFLOAT Constant,
    OUT gctINT * Index,
    OUT gctUINT8 * Swizzle
    )
{
    gctFLOAT constants[4];
    constants[0] = Constant;

    return _AllocateConst(Tree,
                          CodeGen,
                          gcSL_ENABLE_X,
                          constants,
                          Index,
                          Swizzle);
}

gceSTATUS
_AddConstantVec2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctFLOAT Constant1,
    IN gctFLOAT Constant2,
    OUT gctINT * Index,
    OUT gctUINT8 * Swizzle
    )
{
    gctFLOAT constants[4];
    constants[0] = Constant1;
    constants[1] = Constant2;

    return _AllocateConst(Tree,
                          CodeGen,
                          gcSL_ENABLE_X | gcSL_ENABLE_Y,
                          constants,
                          Index,
                          Swizzle);
}

gceSTATUS
_AddConstantIVec1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctINT Constant,
    OUT gctINT * Index,
    OUT gctUINT8 * Swizzle
    )
{
    gctINT constants[4];
    constants[0] = Constant;

    return _AllocateConst(Tree,
                          CodeGen,
                          gcSL_ENABLE_X,
                          (gctFLOAT *) constants,
                          Index,
                          Swizzle);
}

static void
_SetOpcode(
    IN OUT gctUINT32 States[4],
    IN gctUINT32 Opcode,
    IN gctUINT32 Condition,
    IN gctBOOL Saturate
    )
{
    /* Set opcode. */
    States[0] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (Opcode & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
              |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) ((gctUINT32) (Condition) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
              |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (Saturate) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
    States[2] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (Opcode >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
}

extern gceSTATUS
_GetVariableIndexingRange(
    IN gcSHADER Shader,
    IN gcVARIABLE variable,
    IN gctBOOL whole,
    OUT gctUINT *Start,
    OUT gctUINT *End);

static gcSHADER_TYPE
_Usage2Type(
    IN gctUINT8 usage
    )
{
    gcSHADER_TYPE type;

    switch (usage)
    {
    case 0x1:
        /* 1-component temporary register. */
        type = gcSHADER_FLOAT_X1;
        break;

    case 0x2:
    /* fall through */
    case 0x3:
        /* 2-component temporary register. */
        type = gcSHADER_FLOAT_X2;
        break;

    case 0x4:
    /* fall through */
    case 0x5:
    /* fall through */
    case 0x6:
    /* fall through */
    case 0x7:
        /* 3-component temporary register. */
        type = gcSHADER_FLOAT_X3;
        break;

    case 0x8:
    /* fall through */
    case 0x9:
    /* fall through */
    case 0xA:
    /* fall through */
    case 0xB:
    /* fall through */
    case 0xC:
    /* fall through */
    case 0xD:
    /* fall through */
    case 0xE:
    /* fall through */
    case 0xF:
        /* 4-component temporary register. */
        type = gcSHADER_FLOAT_X4;
        break;

    default:
        /* Special case for uninitialized variables. */
        type = gcSHADER_FLOAT_X1;
        break;
    }

    return type;
}

static gceSTATUS
_AssignTemp(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcLINKTREE_TEMP Temp
    )
{
    gcSHADER_TYPE type;
    gctUINT count = 1, i;
    gceSTATUS status;

    /* Convert usage into type. */
    type = _Usage2Type(Temp->usage);

    /* TODO: */
    /* We need refine it to make more flexible for RA for array or matrix that are not indexed access*/
    /* In that case, we can regard them as separated registers so to get better HW register usage*/
    if (Temp->variable != gcvNULL)
    {
        if (Temp->isIndexing && (Temp->variable->parent != -1))
        {
            gcLINKTREE_TEMP thisTemp;
            gctUINT8 maxUsage = 0;
            gctUINT startIndex, endIndex;
            _GetVariableIndexingRange(Tree->shader, Temp->variable, gcvTRUE,
                                      &startIndex, &endIndex);
            count = endIndex - startIndex;

            for (i = startIndex; i < endIndex; i ++)
            {
                thisTemp = Tree->tempArray + startIndex;
                if (maxUsage < thisTemp->usage)
                    maxUsage = thisTemp->usage;
            }

            /* Convert usage into type. */
            type = _Usage2Type(maxUsage);

            Temp = Tree->tempArray + startIndex;
        }
        else if ((Temp->variable->arraySize > 1) || IS_MATRIX_TYPE(Temp->variable->u.type))
        {
            gcVARIABLE variable = Temp->variable;
            gctUINT tempIndex = Temp - Tree->tempArray;
            gctINT components, rows = 0;

            /* Determine the number of rows per element. */
            _ConvertType(variable->u.type, 1, &components, &rows);

            gcmASSERT(count == 1 || count <= variable->arraySize);
            count = variable->arraySize * rows;
            if (tempIndex != variable->tempIndex)
            {
                Temp = Tree->tempArray + variable->tempIndex;

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
{
                gctINT index = tempIndex - variable->tempIndex;
                gcmASSERT((index > 0) && (index < (gctINT) count));
}
#endif
            }
        }
    }

    do
    {
        gctUINT8 enable;

        /* Allocate physical register. */
        gcmERR_BREAK(
            _FindUsage(CodeGen->registerUsage,
                       CodeGen->registerCount,
                       type,
                       count,
                       (Temp->lastUse == -1) ? gcvSL_RESERVED : Temp->lastUse,
                       (Temp->lastUse == -1),
                       &Temp->assigned,
                       &Temp->swizzle,
                       &Temp->shift,
                       &enable));

        /* Assign all temps in the output array. */
        for (i = 1; i < count; ++i)
        {
            Temp[i].assigned = Temp->assigned + i;
            Temp[i].swizzle  = Temp->swizzle;
            Temp[i].shift    = Temp->shift;

            /* Update lastUse for array. */
            if (Temp[i].lastUse > Temp->lastUse)
            {
                _SetUsage(CodeGen->registerUsage + Temp->assigned + i, 1, enable, Temp[i].lastUse);
            }
        }
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetDest(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN OUT gctUINT32 * States,
    IN gctINT Index,
    IN gctUINT32 Relative,
    IN gctUINT32 Enable,
    OUT gctINT_PTR Shift
    )
{
    gceSTATUS status;
    gctUINT32 index, enable;

    do
    {
        /* Extract temporary register information. */
        gcLINKTREE_TEMP temp = (Index >= 0)
            ? &Tree->tempArray[Index]
            : gcvNULL;

        /* See if temporary register needs to be assigned. */
        if ((temp != gcvNULL) && (temp->assigned == -1) )
        {
            gcmERR_BREAK(_AssignTemp(Tree, CodeGen, temp));
            gcmASSERT(temp->assigned != -1);
        }

        /* Load physical mapping. */
        index  = (temp == gcvNULL) ? -Index - 1 : temp->assigned;
        enable = (temp == gcvNULL) ? Enable : (Enable << temp->shift);
        gcmASSERT((enable & ~gcSL_ENABLE_XYZW) == 0);

        /* Special handling for LOAD SW workaround optimization. */
        if (index == CodeGen->reservedRegForLoad)
        {
            gcmASSERT(Index == CodeGen->loadDestIndex);
            index = CodeGen->origAssigned;
        }

        /* Set DEST fields. */
        States[0] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (Relative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

        if(temp != gcvNULL && temp->variable != gcvNULL)
        {
            gctINT components, rows = 0;
            /* Determine the number of rows per element. */
            _ConvertType(temp->variable->u.type, 1, &components, &rows);

            if(temp->variable->arraySize > 1)
            {
                index = index + temp->variable->arraySize * rows - 1;
            }
            else
            {
                index = index + rows - 1;
            }
        }

        /* Keep maximum physical register. */
        if (index > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = index;
        }

        if (Shift != gcvNULL)
        {
            *Shift = (temp != gcvNULL) ? temp->shift : -1;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetSampler(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN OUT gctUINT32 * States,
    IN gctUINT32 Index,
    IN gctUINT32 Relative,
    IN gctUINT32 Swizzle
    )
{
    gceSTATUS status;
    gctUINT32 sampler;

    do
    {
        /* Load physical sampler number. */
        sampler = Tree->shader->uniforms[Index]->physical;

        /* Set SAMPLER fields. */
        States[0] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) ((gctUINT32) (sampler) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
        States[1] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (Relative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3))) | (((gctUINT32) ((gctUINT32) (Swizzle) & ((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1))))))) << (0 ? 10:3)));

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetSource(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN OUT gctUINT32 States[4],
    IN gctUINT Source,
    IN gcSL_TYPE Type,
    IN gctINT32 Index,
    IN gctUINT32 ConstIndex,
    IN gctUINT32 Relative,
    IN gctUINT32 Swizzle,
    IN gctBOOL Negate,
    IN gctBOOL Absolute
    )
{
    gctUINT32 type = 0;
    gctUINT32 index = 0;
    gctUINT8 swizzle = 0;

    /* Dispatch on given source type. */
    switch (Type)
    {
    case gcSL_TEMP:
        /* Temporary register. */
        if (Index < 0)
        {
            type    = 0x0;
            index   = -Index - 1;
            swizzle = (gctUINT8) Swizzle;
        }
        else
        {
            gctBOOL useConst = gcvTRUE;
            gctUINT8 usage = _Swizzle2Enable((gcSL_SWIZZLE) ((Swizzle >> 0) & 3),
                                             (gcSL_SWIZZLE) ((Swizzle >> 2) & 3),
                                             (gcSL_SWIZZLE) ((Swizzle >> 4) & 3),
                                             (gcSL_SWIZZLE) ((Swizzle >> 6) & 3));

            /* Test for constant propagation in X component. */
            if ((usage & gcSL_ENABLE_X)
            &&  (Tree->tempArray[Index].constUsage[0] != 1)
            )
            {
                useConst = gcvFALSE;
            }

            /* Test for constant propagation in Y component. */
            if ((usage & gcSL_ENABLE_Y)
            &&  (Tree->tempArray[Index].constUsage[1] != 1)
            )
            {
                useConst = gcvFALSE;
            }

            /* Test for constant propagation in Z component. */
            if ((usage & gcSL_ENABLE_Z)
            &&  (Tree->tempArray[Index].constUsage[2] != 1)
            )
            {
                useConst = gcvFALSE;
            }

            /* Test for constant propagation in W component. */
            if ((usage & gcSL_ENABLE_W)
            &&  (Tree->tempArray[Index].constUsage[3] != 1)
            )
            {
                useConst = gcvFALSE;
            }

            /* Is this register used as constant propagation? */
            if (useConst)
            {
                gcmVERIFY_OK(_AllocateConst(Tree,
                                            CodeGen,
                                            usage,
                                            Tree->tempArray[Index].constValue,
                                            (gctINT32_PTR) &index,
                                            (gctUINT8_PTR) &swizzle));

                type = 0x2;
                swizzle = _AdjustSwizzle((gctUINT8) Swizzle, swizzle);
            }

            else
            {
                /* See if temporary register needs to be assigned. */
                if (Tree->tempArray[Index].assigned == -1)
                {
                    gcmVERIFY_OK(
                        _AssignTemp(Tree, CodeGen, &Tree->tempArray[Index]));
                }

                type    = 0x0;
                index   = Tree->tempArray[Index].assigned;
                swizzle = _AdjustSwizzle((gctUINT8) Swizzle,
                                         Tree->tempArray[Index].swizzle);
            }
        }

        /* Keep maximum physical register. */
        if ((index > CodeGen->maxRegister)
        &&  (type == 0x0)
        )
        {
            CodeGen->maxRegister = index;
        }
        break;

    case gcSL_ATTRIBUTE:
        /* Attribute. */
        gcmASSERT(Tree->shader->attributes[Index]->inputIndex != -1);

        if (Tree->shader->attributes[Index]->nameLength == gcSL_FRONT_FACING)
        {
            if (CodeGen->useFace)
            {
                type    = 0x0;
                index   = CodeGen->facePhysical;
                swizzle = CodeGen->faceSwizzle;
            }
            else
            {
                type    = 0x1;
                index   = 0;
                swizzle = gcSL_SWIZZLE_XXXX;
            }
        }

        else if (Tree->shader->attributes[Index]->nameLength == gcSL_POSITION)
        {
            type    = 0x0;
            index   = CodeGen->usePosition ? CodeGen->positionPhysical : 0;
            swizzle = (gctUINT8) Swizzle;
        }

        else
        {
            type    = 0x0;
            index   = Tree->shader->attributes[Index]->inputIndex + ConstIndex;
            swizzle = (gctUINT8) Swizzle;

            /* Keep maximum physical register. */
            if(Relative > 0 && Tree->shader->attributes[Index]->arraySize > 1)
            {
                if((Tree->shader->attributes[Index]->inputIndex + Tree->shader->attributes[Index]->arraySize - 1)
                    > CodeGen->maxRegister)
                {
                    CodeGen->maxRegister = Tree->shader->attributes[Index]->inputIndex +
                        Tree->shader->attributes[Index]->arraySize - 1;
                }
            }
            else if (index > CodeGen->maxRegister)
            {
                CodeGen->maxRegister = index;
            }
        }
        break;

    case gcSL_UNIFORM:
        /* Uniform. */
        gcmASSERT(Tree->shader->uniforms[Index]->physical != -1);

        type    = 0x2;
        index   = Tree->shader->uniforms[Index]->physical + ConstIndex;
        swizzle = _AdjustSwizzle((gctUINT8) Swizzle,
                                 Tree->shader->uniforms[Index]->swizzle);
        break;

    case gcSL_CONSTANT:
        /* Constant. */
        type    = 0x2;
        index   = Index;
        swizzle = (gctUINT8) Swizzle;
        break;

    case gcSL_PHYSICAL:
        /* Constant. */
        type    = 0x0;
        index   = Index;
        swizzle = (gctUINT8) Swizzle;
        break;

    case gcSL_NONE:
        return gcvSTATUS_OK;

    default:
        gcmFATAL("!!TODO!! Unknown Type %u", Type);
    }

    switch (Source)
    {
    case 0:
        /* Set SRC0 fields. */
        States[1] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) (Negate) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (Absolute) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
        States[2] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (Relative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)));
        break;

    case 1:
        /* Set SRC1 fields. */
        States[2] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (Relative) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25))) | (((gctUINT32) ((gctUINT32) (Negate) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1))))))) << (0 ? 25:25)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (Absolute) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));
        States[3] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));
        break;

    case 2:
        /* Set SRC2 fields. */
        States[3] |= ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (Relative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (Negate) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))
                  |  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) ((gctUINT32) (Absolute) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));
        break;

    default:
        break;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

static void
_UpdateUsage(
    IN OUT gcsSL_USAGE_PTR Usage,
    IN gctSIZE_T UsageCount,
    IN gctINT Index
    )
{
    gctSIZE_T u, i;

    /* Process all usages */
    for (u = 0; u < UsageCount; ++u)
    {
        /* Process each component. */
        for (i = 0; i < gcmCOUNTOF(Usage->lastUse); ++i)
        {
            /* See if the component can be made available. */
            if (Usage[u].lastUse[i] == Index)
            {
                Usage[u].lastUse[i] = gcvSL_AVAILABLE;
            }
        }
    }
}

gctUINT8
_ReplicateSwizzle(
    IN gctUINT8 Swizzle,
    IN gctUINT Index
    )
{
    gctUINT8 swizzle = _ExtractSwizzle(Swizzle, Index);
    swizzle |= swizzle << 2;
    return swizzle | (swizzle << 4);
}

gctUINT8
_ReplicateSwizzle2(
    IN gctUINT8 Swizzle,
    IN gctUINT Index
    )
{
    gctUINT8 swizzle;

    switch (Index)
    {
    case 0:
        swizzle = Swizzle & 0xF;
        return swizzle | (swizzle << 4);

    case 1:
        swizzle = Swizzle & 0xF0;
        return swizzle | (swizzle >> 4);

    default:
        break;
    }

    gcmFATAL("Invalid swizzle index %d.", Index);
    return 0;
}

static gceSTATUS
_FinalEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4]
    );

static gceSTATUS
_TempEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4],
    IN gctUINT Source
    )
{
    gctUINT physical;
    gctINT shift;
    gctUINT8 swizzle;
    gctUINT32 states[4];
    gceSTATUS status;
    gctUINT address = 0, relative = 0, type = 0;
    gctINT lastUse;

    do
    {
        /* Allocate a temporary register. */
        lastUse = (Tree->hints[CodeGen->nextSource - 1].lastUseForTemp == (gctINT) CodeGen->nextSource - 1) ?
                  gcvSL_TEMPORARY : Tree->hints[CodeGen->nextSource - 1].lastUseForTemp;
        gcmERR_BREAK(_FindUsage(CodeGen->registerUsage,
                                CodeGen->registerCount,
                                gcSHADER_FLOAT_X4,
                                1,
                                lastUse /*gcvSL_TEMPORARY*/,
                                gcvFALSE,
                                (gctINT_PTR) &physical,
                                &swizzle,
                                &shift,
                                gcvNULL));

        if (((((((gctUINT32) (States[1])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) )
            && ((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) == 0x0)
            && ((((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) ) == physical)
            )
        ||     ((((((gctUINT32) (States[2])) >> (0 ? 6:6)) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) )
            && ((((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) == 0x0)
            && ((((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) ) == physical)
            )
        ||     ((((((gctUINT32) (States[3])) >> (0 ? 3:3)) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) )
            && ((((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) == 0x0)
            && ((((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ) == physical)
            )
        )
        {
            gcmERR_BREAK(_FindUsage(CodeGen->registerUsage,
                                    CodeGen->registerCount,
                                    gcSHADER_FLOAT_X4,
                                    1,
                                    lastUse /*gcvSL_TEMPORARY*/,
                                    gcvFALSE,
                                    (gctINT_PTR) &physical,
                                    &swizzle,
                                    &shift,
                                    gcvNULL));
        }

        if (physical > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical;
        }

        /* MOV temp, sourceX */
        switch (Source)
        {
        case 0:
            address  = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
            relative = (((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
            type     = (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) );
            break;

        case 1:
            address  = (((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
            relative = (((((gctUINT32) (States[2])) >> (0 ? 29:27)) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1)))))) );
            type     = (((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
            break;

        case 2:
            address  = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
            relative = (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) );
            type     = (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) );
            break;

        default:
            break;
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (0xF) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (0xE4) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (relative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (type) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

        gcmERR_BREAK(_FinalEmit(Tree, CodeGen, states));

        /* Modify sourceX. */
        switch (Source)
        {
        case 0:
            States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)));
            break;

        case 1:
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
            States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));
            break;

        case 2:
            States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)));
            States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)));
            States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));
            break;

        default:
            break;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_FinalEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4]
    )
{
    gceSTATUS status;
    gctUINT8 enable = 0, swizzle = 0, mask, newSwizzle = 0;
    gctBOOL again;

    do
    {
        gcsSL_PHYSICAL_CODE_PTR code;

        /* Test for invalid uniform usage. */
        gctUINT32 s0Type  = (((((gctUINT32) (States[1])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) )
            ? (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) )
            : (gctUINT32) 0xFFFFFFFF;
        gctUINT32 s0Index = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
        gctUINT32 s1Type  = (((((gctUINT32) (States[2])) >> (0 ? 6:6)) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) )
            ? (((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) )
            : (gctUINT32) 0xFFFFFFFF;
        gctUINT32 s1Index = (((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
        gctUINT32 s2Type  = (((((gctUINT32) (States[3])) >> (0 ? 3:3)) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) )
            ? (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) )
            : (gctUINT32) 0xFFFFFFFF;
        gctUINT32 s2Index = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );

        if ((s0Type == 0x2)
        &&  (s1Type == 0x2)
        &&  (s2Type != 0x2)
        &&  (s0Index != s1Index)
        )
        {
            /* Source 0 and 1 collision: Copy source 0 to temp. */
            gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
        }

        if ((s0Type == 0x2)
        &&  (s1Type != 0x2)
        &&  (s2Type == 0x2)
        &&  (s0Index != s2Index)
        )
        {
            /* Source 0 and 2 collision: copy source 0 to temp. */
            gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
        }

        if ((s0Type != 0x2)
        &&  (s1Type == 0x2)
        &&  (s2Type == 0x2)
        &&  (s1Index != s2Index)
        )
        {
            /* Source 1 and 2 collision: copy source 1 to temp. */
            gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 1));
        }

        if ((s0Type == 0x2)
        &&  (s1Type == 0x2)
        &&  (s2Type == 0x2)
        &&  ((s0Index != s1Index)
            || (s0Index != s2Index)
            || (s1Index != s2Index)
            )
        )
        {
            if (s0Index == s1Index)
            {
                /* Source 0/1 and 2 collision: copy source 2 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 2));
            }
            else if (s0Index == s2Index)
            {
                /* Source 0/2 and 1 collision: copy source 1 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 1));
            }
            else if (s1Index == s2Index)
            {
                /* Source 0 and 1/2 collision: copy source 0 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
            }
            else
            {
                /* Source 0, 1, and 2 collision: copy source 0 and 1 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 1));
            }
        }

        /* Extract the current code array. */
        code = CodeGen->current->code;

        /* If there is no code array yet or the current code array is full,
           create a new code array. */
        if ((CodeGen->current->root == gcvNULL)
        ||  (code->count == code->maxCount)
        )
        {
            /* Allocate a new code array. */
            gctSIZE_T bytes = gcmSIZEOF(gcsSL_PHYSICAL_CODE) +
                           31 * gcmSIZEOF(code->states);
            gctPOINTER pointer = gcvNULL;

            gcmERR_BREAK(gcoOS_Allocate(gcvNULL, bytes, &pointer));

            code = pointer;

            /* Initilaize the code array. */
            code->next     = gcvNULL;
            code->maxCount = 32;
            code->count    = 0;

            /* Link in the code array to the end of the list. */
            if (CodeGen->current->root == gcvNULL)
            {
                CodeGen->current->root = code;
            }
            else
            {
                CodeGen->current->code->next = code;
            }

            /* Save the new code array. */
            CodeGen->current->code = code;
        }

        /* Assume we don't need to split the instruction. */
        again = gcvFALSE;

        switch ((((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) ))
        {
            /* Set DEST_VALID bit to 0 for STORE. */
        case 0x33:
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));
            break;

        case 0x0C:
        /* fall through */
        case 0x0D:
        /* fall through */
        case 0x11:
        /* fall through */
        case 0x12:
        /* fall through */
        case 0x21:
        /* fall through */
        case 0x23:
        /* fall through */
        case 0x22:
        /* fall through */
            enable  = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

            if ((swizzle & 0x03) != ((swizzle >> 2) & 0x03))
            {
                mask       = 0x1;
                newSwizzle = (swizzle & 0xFC)
                           | (_ReplicateSwizzle(swizzle, 1) & 0x03);
            }
            else if ((swizzle & 0x03) != ((swizzle >> 4) & 0x03))
            {
                mask       = 0x3;
                newSwizzle = (swizzle & 0xF0)
                           | (_ReplicateSwizzle(swizzle, 2) & 0x0F);
            }
            else if ((swizzle & 0x03) != ((swizzle >> 6) & 0x03))
            {
                mask       = 0x7;
                newSwizzle = _ReplicateSwizzle(swizzle, 3);
            }
            else
            {
                mask = 0x0;
            }

            if (enable & mask)
            {
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable & mask) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

                States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

                again   = gcvTRUE;
                enable &= ~mask;
                swizzle = newSwizzle;
            }
            break;

        default:
            break;
        }

        /* Copy the states into the code array. */
        gcmERR_BREAK(gcoOS_MemCopy(code->states + code->count * 4,
                                   States,
                                   4 * sizeof(gctUINT32)));

        /* Store location of states for future optimization. */
        CodeGen->previousCode = code->states + code->count * 4;

        /* Incease code counter and current instruction pointer. */
        ++code->count;
        ++CodeGen->current->ip;

        if (again)
        {
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

            States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

            gcmERR_BREAK(_FinalEmit(Tree, CodeGen, States));
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

typedef enum _gceCONVERT_TYPE {
    gcvCONVERT_NONE,
    gcvCONVERT_EXTRA,
    gcvCONVERT_2COMPONENTS,
    gcvCONVERT_COMPONENTXY,
    gcvCONVERT_COMPONENTZW,
    gcvCONVERT_ROTATE,
    gcvCONVERT_LEADZERO,
    gcvCONVERT_LSHIFT,
    gcvCONVERT_DIVMOD,
    gcvCONVERT_HI,
    gcvCONVERT_LOAD,
    gcvCONVERT_STORE
}
gceCONVERT_TYPE;

static gceSTATUS
_SourceConvertEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4],
    IN gctUINT ValueType,
    IN gctUINT Source,
    IN gceCONVERT_TYPE ConvertType
    )
{
    gceSTATUS status;
    gctUINT32 states[4];
    gctUINT physical, physical1;
    gctUINT8 swizzle, swizzle1;
    gctINT shift, shift1;
    gctUINT8 enable, enable1;
    gctINT constPhysical, constPhysical1;
    gctUINT8 constSwizzle, constSwizzle1;
    gctUINT sourcePhysical;
    gctUINT8 sourceSwizzle;
    gctUINT sourceRelative;
    gctUINT sourceType;
    gctINT lastUse;

    /* Get source info. */
    switch (Source)
    {
    case 0:
        sourcePhysical = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
        sourceRelative = (((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
        sourceSwizzle  = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
        sourceType     = (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) );
        break;

    case 1:
        sourcePhysical = (((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
        sourceRelative = (((((gctUINT32) (States[2])) >> (0 ? 29:27)) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1)))))) );
        sourceSwizzle  = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );
        sourceType     = (((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
        break;

    case 2:
        sourcePhysical = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
        sourceRelative = (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) );
        sourceSwizzle  = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );
        sourceType     = (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) );
        break;

    default:
        /* Add redundant assignments to avoid warnings. */
        sourcePhysical =
        sourceRelative =
        sourceType     = 0;
        sourceSwizzle  = 0;
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    lastUse = (Tree->hints[CodeGen->nextSource - 1].lastUseForTemp == (gctINT) CodeGen->nextSource - 1) ?
              gcvSL_TEMPORARY : Tree->hints[CodeGen->nextSource - 1].lastUseForTemp;

    /* Add conversion instructions. */
    switch (ConvertType)
    {
    case gcvCONVERT_ROTATE:
        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X1,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical,
                              &swizzle,
                              &shift,
                              &enable));

        /* Allocate another temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X1,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        if (ValueType == 0x7
        ||  ValueType == 0x4)
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x000000FF,
                                          &constPhysical,
                                          &constSwizzle));
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x01010101,
                                          &constPhysical1,
                                          &constSwizzle1));
        }
        else
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x0000FFFF,
                                          &constPhysical,
                                          &constSwizzle));
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x00010001,
                                          &constPhysical1,
                                          &constSwizzle1));
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x5D,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (constPhysical1) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (constSwizzle1) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

       _SetOpcode(states,
                  0x3C,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;

    case gcvCONVERT_LEADZERO:
        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X1,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical,
                              &swizzle,
                              &shift,
                              &enable));

        /* Allocate another temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X1,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        if (ValueType == 0x7
        ||  ValueType == 0x4)
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          24,
                                          &constPhysical,
                                          &constSwizzle));
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x00FFFFFF,
                                          &constPhysical1,
                                          &constSwizzle1));
        }
        else
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          16,
                                          &constPhysical,
                                          &constSwizzle));
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x0000FFFF,
                                          &constPhysical1,
                                          &constSwizzle1));
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x59,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical1) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle1) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x5C,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;

    case gcvCONVERT_LSHIFT:
        if (sourceType == 0x2)
        {
            /* Get constant. */
            gctUINT8 swizzle = sourceSwizzle & 0x3;
            gctINT value = 0;
            gcsSL_CONSTANT_TABLE_PTR c;

            for (c = CodeGen->constants; c != gcvNULL; c = c->next)
            {
                if (c->index == (gctINT) sourcePhysical)
                {
                    gctINT i;

                    for (i = 0; i < c->count; i++)
                    {
                        if (((c->swizzle >> (i * 2)) & 0x3) == swizzle)
                        {
                            value = *((int *) (&c->constant[i]));
                            break;
                        }
                    }
                    if (i < c->count)
                    {
                        break;
                    }
                }
            }

            /* Check if constant in the range. */
            if (ValueType == 0x7
            ||  ValueType == 0x4)
            {
                if (value < 8)
                {
                    /* No action needed. */
                    return gcvSTATUS_OK;
                }
            }
            else
            {
                if (value < 16)
                {
                    /* No action needed. */
                    return gcvSTATUS_OK;
                }
            }
        }

        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X1,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        if (ValueType == 0x7
        ||  ValueType == 0x4)
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0x7,
                                          &constPhysical,
                                          &constSwizzle));
        }
        else
        {
            gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                          CodeGen,
                                          0xF,
                                          &constPhysical,
                                          &constSwizzle));
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x5D,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;

    case gcvCONVERT_DIVMOD:
        if (sourceSwizzle == gcSL_SWIZZLE_XXXX)
        {
            /* No action needed. */
            return gcvSTATUS_OK;
        }

        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X4,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        /* Use x component only. */
        enable1 = gcSL_ENABLE_X;
        swizzle1 = gcSL_SWIZZLE_XXXX;

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x09,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;

#if _EXTRA_16BIT_CONVERSION_
    case gcvCONVERT_COMPONENTXY:
        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X4,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        /* Set enable and swizzle. */
        enable1 = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
        switch (enable1)
        {
        case gcSL_ENABLE_X:
            swizzle1 = gcSL_SWIZZLE_XXXX;
            sourceSwizzle = _ReplicateSwizzle(sourceSwizzle, 0);
            break;

        case gcSL_ENABLE_Y:
            swizzle1 = gcSL_SWIZZLE_YYYY;
            sourceSwizzle = _ReplicateSwizzle(sourceSwizzle, 1);
            break;

        case gcSL_ENABLE_XY:
            swizzle1 = gcSL_SWIZZLE_XYYY;
            sourceSwizzle = _ReplicateSwizzle2(sourceSwizzle, 0);
            break;

        default:
            /* Error. */
            break;
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x09,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;

    case gcvCONVERT_COMPONENTZW:
        /* Allocate a temporary register. */
        gcmONERROR(_FindUsage(CodeGen->registerUsage,
                              CodeGen->registerCount,
                              gcSHADER_INTEGER_X4,
                              1,
                              lastUse /*gcvSL_TEMPORARY*/,
                              gcvFALSE,
                              (gctINT_PTR) &physical1,
                              &swizzle1,
                              &shift1,
                              &enable1));

        if (physical1 > CodeGen->maxRegister)
        {
            CodeGen->maxRegister = physical1;
        }

        /* Set enable and swizzle. */
        enable1 = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
        switch (enable1)
        {
        case gcSL_ENABLE_Z:
            swizzle1 = gcSL_SWIZZLE_ZZZZ;
            sourceSwizzle = _ReplicateSwizzle(sourceSwizzle, 2);
            break;

        case gcSL_ENABLE_W:
            swizzle1 = gcSL_SWIZZLE_WWWW;
            sourceSwizzle = _ReplicateSwizzle(sourceSwizzle, 3);
            break;

        case gcSL_ENABLE_ZW:
            swizzle1 = gcSL_SWIZZLE_XYZW;
            sourceSwizzle = _ReplicateSwizzle2(sourceSwizzle, 1);
            break;

        default:
            /* Error. */
            break;
        }

        states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable1) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
        states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
        states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
        states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (sourcePhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (sourceRelative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                  | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (sourceType) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

       _SetOpcode(states,
                  0x09,
                  0x00,
                  gcvFALSE);

       _SetValueType0(0x5, states);

        gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        break;
#endif


    default:
        return gcvSTATUS_MISMATCH;
    }

    /* Modify sourceX. */
    switch (Source)
    {
    case 0:
        States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)));
        States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle1) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
        States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));
        States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)));
        break;

    case 1:
        States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)));
        States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle1) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));
        States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));
        break;

    case 2:
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (physical1) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)));
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle1) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)));
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));
        break;

    default:
        break;
    }

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

static gctUINT8
_Enable2SwizzleWShift(
    IN gctUINT32 Enable
    )
{
    switch (Enable)
    {
    case gcSL_ENABLE_X:
        return gcSL_SWIZZLE_XXXX;

    case gcSL_ENABLE_Y:
        return gcSL_SWIZZLE_YYYY;

    case gcSL_ENABLE_Z:
        return gcSL_SWIZZLE_ZZZZ;

    case gcSL_ENABLE_W:
        return gcSL_SWIZZLE_WWWW;

    case gcSL_ENABLE_XY:
        return gcSL_SWIZZLE_XYYY;

    case gcSL_ENABLE_XZ:
        return gcSL_SWIZZLE_XZZZ;

    case gcSL_ENABLE_XW:
        return gcSL_SWIZZLE_XWWW;

    case gcSL_ENABLE_YZ:
        return gcSL_SWIZZLE_YYZZ;

    case gcSL_ENABLE_YW:
        return gcSL_SWIZZLE_YYWW;

    case gcSL_ENABLE_ZW:
        return gcSL_SWIZZLE_ZZZW;

    case gcSL_ENABLE_XYZ:
        return gcSL_SWIZZLE_XYZZ;

    case gcSL_ENABLE_XYW:
        return gcSL_SWIZZLE_XYWW;

    case gcSL_ENABLE_XZW:
        return gcSL_SWIZZLE_XZZW;

    case gcSL_ENABLE_YZW:
        return gcSL_SWIZZLE_YYZW;

    case gcSL_ENABLE_XYZW:
        return gcSL_SWIZZLE_XYZW;

    default:
        break;

    }

    gcmFATAL("ERROR: Invalid enable 0x%04X", Enable);
    return gcSL_SWIZZLE_XYZW;
}

static gceSTATUS
_TargetConvertEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4],
    IN gctUINT ValueType,
    IN gceCONVERT_TYPE ConvertType,
    IN gctBOOL Saturate
    )
{
    gceSTATUS status;
    gctBOOL isSigned = gcvTRUE;
    gctUINT32 states[4];
    gctUINT physical;
    gctUINT8 swizzle;
    gctINT shift;
    gctUINT8 enable;
    gctINT constPhysical, constPhysical1;
    gctUINT8 constSwizzle, constSwizzle1;
    gctUINT targetPhysical;
    gctUINT targetRelative;
    gctUINT8 targetEnable;
    gctINT lastUse;

    /* Get original target info. */
    targetPhysical = (((((gctUINT32) (States[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) );
    targetRelative = (((((gctUINT32) (States[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) );
    targetEnable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );

    if (ConvertType == gcvCONVERT_LOAD)
    {
        /* Special workaround for Load L1 cache hang. */
        /* The workaround is to always load the data to the same register and then move to target. */

        /* Change target to reservedRegForLoad. */
        States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->reservedRegForLoad) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)));
        States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)));

        /* Output the modified original instruction. */
        gcmONERROR(_FinalEmit(Tree, CodeGen, States));

        /* Check if this extra MOV can be eliminated. */
        if (Tree->hints[CodeGen->nextSource - 1].lastLoadUser < 0)
        {
            states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
            states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
            states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
            states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (CodeGen->reservedRegForLoad) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_Enable2SwizzleWShift(targetEnable)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

            _SetOpcode(states,
                       0x09,
                       0x00,
                       gcvFALSE);

            /*_SetValueType0(0x5, states);*/

            gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        }
        else
        {
            /* Change temp register assign to reservedRegForLoad. */
            gctUINT index = Tree->hints[CodeGen->nextSource - 1].loadDestIndex;
            CodeGen->loadDestIndex = index;
            gcmASSERT(Tree->tempArray[index].assigned == (gctINT) targetPhysical);
            gcmASSERT(CodeGen->origAssigned < 0);
            gcmASSERT(CodeGen->lastLoadUser < 0);
            CodeGen->origAssigned = Tree->tempArray[index].assigned;
            Tree->tempArray[index].assigned = CodeGen->reservedRegForLoad;
            CodeGen->lastLoadUser = Tree->hints[CodeGen->nextSource - 1].lastLoadUser;
        }

        /* Success. */
        return gcvSTATUS_OK;
    }

    /* For GC2100 only. */
    gcmASSERT(CodeGen->isGC2100);

    /* Allocate a temporary register. */
    lastUse = (Tree->hints[CodeGen->nextSource - 1].lastUseForTemp == (gctINT) CodeGen->nextSource - 1) ?
              gcvSL_TEMPORARY : Tree->hints[CodeGen->nextSource - 1].lastUseForTemp;
    gcmONERROR(_FindUsage(CodeGen->registerUsage,
                          CodeGen->registerCount,
                          gcSHADER_INTEGER_X1,
                          1,
                          lastUse /*gcvSL_TEMPORARY*/,
                          gcvFALSE,
                          (gctINT_PTR) &physical,
                          &swizzle,
                          &shift,
                          &enable));

    if (physical > CodeGen->maxRegister)
    {
        CodeGen->maxRegister = physical;
    }

    /* Add conversion instruction(s). */
    if (Saturate)
    {
        gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                         | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;

        /* GC2100 does not support IADDSAT, IMULLOSAT0, IMADLOSAT0, and IMADHISAT0. */
        /* Use ADD, IMULLO0, and IMADLO0 with post processing to implement these instructions. */

        /* Change opcodes to supported opcodes. */
        switch (opcode)
        {
        case 0x3B:
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x01) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            break;

        case 0x3E:
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x3C) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            break;

        case 0x4E:
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x4C & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (0x4C >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
            break;

        case 0x52:
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x50 & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (0x50 >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
            break;

        }

        if (ConvertType == gcvCONVERT_NONE)
        {
            if (ValueType == 0x7
            ||  ValueType == 0x6)
            {
                /* Need to add one instruction, so change target to temp. */
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)));
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)));
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                isSigned = gcvFALSE;
            }

            /* Output the modified original instruction. */
            gcmONERROR(_FinalEmit(Tree, CodeGen, States));

            if (isSigned)
            {
                if (ValueType == 0x2)
                {
                    /* TODO - Speical workarounds. */
                }
                else
                {
                    if (ValueType == 0x4)
                    {
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0x0000007F,
                                                      &constPhysical,
                                                      &constSwizzle));
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0xFFFFFF80,
                                                      &constPhysical1,
                                                      &constSwizzle1));
                    }
                    else
                    {
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0x00007FFF,
                                                      &constPhysical,
                                                      &constSwizzle));
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0xFFFF8000,
                                                      &constPhysical1,
                                                      &constSwizzle1));
                    }

                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (_SingleEnable2Swizzle(targetEnable)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_SingleEnable2Swizzle(targetEnable)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    _SetOpcode(states,
                               0x0F,
                               0x01,
                               gcvFALSE);

                    _SetValueType0(0x2, states);

                    gcmONERROR(_FinalEmit(Tree, CodeGen, states));

                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (constPhysical1) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (constSwizzle1) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    _SetOpcode(states,
                               0x0F,
                               0x02,
                               gcvFALSE);

                    _SetValueType0(0x2, states);

                    gcmONERROR(_FinalEmit(Tree, CodeGen, states));
                }
            }
            else
            {
                gctUINT modifier = (((((gctUINT32) (States[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) );

                if (ValueType == 0x5)
                {
                    /* TODO - Speical workarounds. */
                }
                else if (modifier == 0)
                {
                    if (ValueType == 0x7)
                    {
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0x000000FF,
                                                      &constPhysical,
                                                      &constSwizzle));
                    }
                    else
                    {
                        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                      CodeGen,
                                                      0x0000FFFF,
                                                      &constPhysical,
                                                      &constSwizzle));
                    }

                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    _SetOpcode(states,
                               0x0F,
                               0x01,
                               gcvFALSE);

                    _SetValueType0(0x5, states);

                    gcmONERROR(_FinalEmit(Tree, CodeGen, states));
                }
                else
                {
                    gctUINT sourcePhysical0 = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
                    gctUINT sourceSwizzle0  = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
                    gctUINT sourceRelative0 = (((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
                    gctUINT sourceType0     = (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) );
                    gctUINT sourcePhysical2 = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
                    gctUINT sourceSwizzle2  = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );
                    gctUINT sourceRelative2 = (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) );
                    gctUINT sourceType2     = (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) );

                    gcmASSERT(opcode == 0x3B);

                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (sourcePhysical0) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle0) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (sourceRelative0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (sourceType0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (sourcePhysical2) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (sourceSwizzle2) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27))) | (((gctUINT32) ((gctUINT32) (sourceRelative2) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1))))))) << (0 ? 29:27)));
                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (sourceType2) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    _SetOpcode(states,
                               0x31,
                               0x01,
                               gcvFALSE);

                    _SetValueType0(0x5, states);

                    gcmONERROR(_FinalEmit(Tree, CodeGen, states));
                }
            }
        }
        else
        {
            gcmASSERT(ConvertType == gcvCONVERT_HI);
            gcmASSERT(opcode == 0x52);

            /* Change hi opcode to lo opcode. */
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x4C & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (0x4C >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

            /* Output the modified original instruction. */
            gcmONERROR(_FinalEmit(Tree, CodeGen, States));

            /* OpenCL does not have mad_hi_sat. */
            /* TODO - Need to implement when needed. */
            gcmASSERT(ConvertType != gcvCONVERT_HI);
        }
    }
    else
    {
        if (ValueType == 0x7
        ||  ValueType == 0x6
        ||  ConvertType == gcvCONVERT_HI)
        {
            /* Need to add one instruction, so change target to temp. */
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)));
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)));
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
            isSigned = gcvFALSE;
        }

        if (ConvertType == gcvCONVERT_NONE)
        {
            /* Output the (maybe modified) original instruction. */
            gcmONERROR(_FinalEmit(Tree, CodeGen, States));

            if (isSigned)
            {
                if (ValueType == 0x4)
                {
                    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                  CodeGen,
                                                  24,
                                                  &constPhysical,
                                                  &constSwizzle));
                }
                else
                {
                    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                  CodeGen,
                                                  16,
                                                  &constPhysical,
                                                  &constSwizzle));
                }

                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (_SingleEnable2Swizzle(targetEnable)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                _SetOpcode(states,
                           0x59,
                           0x00,
                           gcvFALSE);

                _SetValueType0(0x2, states);

                gcmONERROR(_FinalEmit(Tree, CodeGen, states));

                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                _SetOpcode(states,
                           0x5A,
                           0x00,
                           gcvFALSE);

                _SetValueType0(0x2, states);

                gcmONERROR(_FinalEmit(Tree, CodeGen, states));
            }
            else
            {
                if (ValueType == 0x7)
                {
                    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                  CodeGen,
                                                  0x000000FF,
                                                  &constPhysical,
                                                  &constSwizzle));
                }
                else
                {
                    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                                  CodeGen,
                                                  0x0000FFFF,
                                                  &constPhysical,
                                                  &constSwizzle));
                }

                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                _SetOpcode(states,
                           0x5D,
                           0x00,
                           gcvFALSE);

                _SetValueType0(0x5, states);

                gcmONERROR(_FinalEmit(Tree, CodeGen, states));
            }
        }
        else
        {
            gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                             | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;

            gcmASSERT(ConvertType == gcvCONVERT_HI);

            /* Change hi opcode to lo opcode. */
            switch (opcode)
            {
            case 0x40:
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x3C & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (0x3C >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
                break;

            case 0x50:
                States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0x4C & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (0x4C >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));
                break;

            }

            /* Output the (maybe modified) original instruction. */
            gcmONERROR(_FinalEmit(Tree, CodeGen, States));

            if (ValueType == 0x4
            ||  ValueType == 0x7)
            {
                gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                              CodeGen,
                                              8,
                                              &constPhysical,
                                              &constSwizzle));
            }
            else
            {
                gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                              CodeGen,
                                              16,
                                              &constPhysical,
                                              &constSwizzle));
            }

            states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (targetPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (targetRelative) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (targetEnable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));
            states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (physical) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
            states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
            states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (constPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (constSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                      | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

            _SetOpcode(states,
                       0x5A,
                       0x00,
                       gcvFALSE);

            if (ValueType == 0x4
            ||  ValueType == 0x3)
            {
                _SetValueType0(0x2, states);
            }
            else
            {
                _SetValueType0(0x5, states);
            }

            gcmONERROR(_FinalEmit(Tree, CodeGen, states));
        }
    }

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_ExtraEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4]
    )
{
    gceSTATUS status;
    gctUINT instType0 = (((((gctUINT32) (States[1])) >> (0 ? 21:21)) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) );
    gctUINT instType1 = (((((gctUINT32) (States[2])) >> (0 ? 31:30)) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1)))))) );
    gctUINT valueType0 = instType0 | (instType1 << 1);
    gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                     | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;

    if (valueType0 == 0x0
    && opcode != 0x32)
    {
        return _FinalEmit(Tree, CodeGen, States);
    }

    if (valueType0 == 0x2
    ||  valueType0 == 0x5)
    {
        /* GC2100 does not support IADDSAT, IMULLOSAT0, IMADLOSAT0, and IMADHISAT0. */
        /* Need to use implement them by other instructions. */
        if (((opcode != 0x3B
        &&  opcode != 0x3E
        &&  opcode != 0x4E
        &&  opcode != 0x52)
        ||  CodeGen->hasBugFixes11 == gcvTRUE)
        &&  opcode != 0x32)
        {
            return _FinalEmit(Tree, CodeGen, States);
        }
    }

    /* IDIV0 and IMOD0 should use 16-bit value types. */
    if (opcode == 0x44
    ||  opcode == 0x48)
    {
        /* Change value type to 16-bit. */
        if (valueType0 == 0x4)
        {
            _SetValueType0(0x3, States);
        }
        else if (valueType0 == 0x7)
        {
            _SetValueType0(0x6, States);
        }

        /* Sources must be x component for DIV/MOD. */
        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 0, gcvCONVERT_DIVMOD));
        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 1, gcvCONVERT_DIVMOD));

        if (valueType0 == 0x7
        ||  valueType0 == 0x6)
        {
            return _FinalEmit(Tree, CodeGen, States);
        }
        /* else fall through. */
    }
    else

    /* All other integer instructions should use 32-bit value types, except LOAD/STORE. */
    if (opcode != 0x32
    &&  opcode != 0x33)
    {
        /* Change value type to 32-bit. */
        if (valueType0 == 0x4
        ||  valueType0 == 0x3)
        {
            _SetValueType0(0x2, States);
        }
        else if (valueType0 == 0x7
             ||  valueType0 == 0x6)
        {
            _SetValueType0(0x5, States);
        }
    }

    /* Add source conversion. */
    switch (opcode)
    {
    case 0x5B:
        /*
         * Need to add conversion for SRC0 before execution:
         * For  8-bit data, AND 0x000000FF and then multiply 0x01010101.
         * For 16-bit data, AND 0x0000FFFF and then multiply 0x00010001.
         */

        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 0, gcvCONVERT_ROTATE));
        break;

    case 0x58:
        /*
         * Need to add conversion for SRC2 before:
         * For  8 bit, shift left 24 bits and then OR 0x00FFFFFF.
         * For 16 bit, shift left 16 bits and then OR 0x0000FFFF.
         */

        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 2, gcvCONVERT_LEADZERO));
        break;

    case 0x59:
    case 0x5A:
        /*
         * Need to add conversion for SRC2 before:
         * For  8 bit, AND 0x00000007.
         * For 16 bit, AND 0x0000000F.
         */

        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 2, gcvCONVERT_LSHIFT));
        break;

    case 0x33:
        /*
         * Need to add conversion for SRC2 before execution to avoid unwanted saturate:
         * For unsigned data, add AND.
         * For   signed data, add LEFTSHIFT and RIGHTSHIFT.
         */

        gcmONERROR(_SourceConvertEmit(Tree, CodeGen, States, valueType0, 2, gcvCONVERT_STORE));
        break;

    default:
        break;
    }

    /* Add target conversion. */
    switch (opcode)
    {
    case 0x01:
    case 0x2E:
    case 0x42:
    case 0x3C:
    case 0x4C:
    case 0x59:
    case 0x5B:
    case 0x44:
    case 0x48:
    case 0x5C:
    case 0x5D:
    case 0x5E:
    case 0x5F:
        /*
         * Need to add data conversion for target after execution:
         * For unsigned data, add AND.
         * For   signed data, add LEFTSHIFT and RIGHTSHIFT.
         */

        return _TargetConvertEmit(Tree, CodeGen, States, valueType0, gcvCONVERT_NONE, gcvFALSE);

    case 0x40:
    case 0x50:
        /*
         * Need to add data conversion for target after execution:
         * For unsigned data, add AND.
         * For   signed data, add LEFTSHIFT and RIGHTSHIFT.
         */

        return _TargetConvertEmit(Tree, CodeGen, States, valueType0, gcvCONVERT_HI, gcvFALSE);

    case 0x3B:
    case 0x3E:
    case 0x4E:
        /*
         * Need to add data conversion for target after execution:
         * For unsigned data, add MIN.
         * For   signed data, add MIN and MAX.
         */

        return _TargetConvertEmit(Tree, CodeGen, States, valueType0, gcvCONVERT_NONE, gcvTRUE);

    case 0x52:
        /*
         * Need to add data conversion for target after execution:
         * For unsigned data, add MIN.
         * For   signed data, add MIN and MAX.
         */

        return _TargetConvertEmit(Tree, CodeGen, States, valueType0, gcvCONVERT_HI, gcvTRUE);

    case 0x32:
        return _TargetConvertEmit(Tree, CodeGen, States, valueType0, gcvCONVERT_LOAD, gcvTRUE);

    default:
        return _FinalEmit(Tree, CodeGen, States);
    }

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_ComponentEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4],
    IN gctUINT SourceMask,
    IN gctUINT Enable,
    IN gctUINT8 Swizzle0,
    IN gctUINT8 Swizzle1,
    IN gctUINT8 Swizzle2,
    IN gceCONVERT_TYPE ExtraHandling
    )
{
#if _EXTRA_16BIT_CONVERSION_
    gceSTATUS status = gcvSTATUS_OK;
#endif
    gctUINT32 states[4];

    states[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (Enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

    if (SourceMask & 0x1)
    {
        states[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (Swizzle0) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
    }
    else
    {
        states[1] = States[1];
    }

    if (SourceMask & 0x2)
    {
        states[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (Swizzle1) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));
    }
    else
    {
        states[2] = States[2];
    }

    if (SourceMask & 0x4)
    {
        states[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (Swizzle2) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));
    }
    else
    {
        states[3] = States[3];
    }

    switch (ExtraHandling)
    {
    case gcvCONVERT_NONE:
        return _FinalEmit(Tree, CodeGen, states);

    case gcvCONVERT_EXTRA:
        return _ExtraEmit(Tree, CodeGen, states);

    case gcvCONVERT_COMPONENTXY:
#if _EXTRA_16BIT_CONVERSION_
        /* Sources must be x or y components. */
        if ((SourceMask & 0x1) &&
            (((Enable & gcSL_ENABLE_X) && (Swizzle0 & 0x3) != gcSL_SWIZZLE_X) ||
             ((Enable & gcSL_ENABLE_Y) && (Swizzle0 & 0xc) != (gcSL_SWIZZLE_Y << 2))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          0,
                                          ExtraHandling));
        }
        if ((SourceMask & 0x2) &&
            (((Enable & gcSL_ENABLE_X) && (Swizzle1 & 0x3) != gcSL_SWIZZLE_X) ||
             ((Enable & gcSL_ENABLE_Y) && (Swizzle1 & 0xc) != (gcSL_SWIZZLE_Y << 2))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          1,
                                          ExtraHandling));
        }
        if ((SourceMask & 0x4) &&
            (((Enable & gcSL_ENABLE_X) && (Swizzle2 & 0x3) != gcSL_SWIZZLE_X) ||
             ((Enable & gcSL_ENABLE_Y) && (Swizzle2 & 0xc) != (gcSL_SWIZZLE_Y << 2))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          2,
                                          ExtraHandling));
        }
#endif
        return _FinalEmit(Tree, CodeGen, states);
        break;

    case gcvCONVERT_COMPONENTZW:
#if _EXTRA_16BIT_CONVERSION_
        /* Sources must be z or w components. */
        if ((SourceMask & 0x1) &&
            (((Enable & gcSL_ENABLE_Z) && (Swizzle0 & 0x30) != (gcSL_SWIZZLE_Z << 4)) ||
             ((Enable & gcSL_ENABLE_W) && (Swizzle0 & 0xc0) != (gcSL_SWIZZLE_W << 6))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          0,
                                          ExtraHandling));
        }
        if ((SourceMask & 0x2) &&
            (((Enable & gcSL_ENABLE_Z) && (Swizzle1 & 0x30) != (gcSL_SWIZZLE_Z << 4)) ||
             ((Enable & gcSL_ENABLE_W) && (Swizzle1 & 0xc0) != (gcSL_SWIZZLE_W << 6))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          1,
                                          ExtraHandling));
        }
        if ((SourceMask & 0x4) &&
            (((Enable & gcSL_ENABLE_Z) && (Swizzle2 & 0x30) != (gcSL_SWIZZLE_Z << 4)) ||
             ((Enable & gcSL_ENABLE_W) && (Swizzle2 & 0xc0) != (gcSL_SWIZZLE_W << 6))))
        {
            gcmONERROR(_SourceConvertEmit(Tree,
                                          CodeGen,
                                          states,
                                          0x3,
                                          2,
                                          ExtraHandling));
        }
#endif
        return _FinalEmit(Tree, CodeGen, states);
        break;

    default:
        return _FinalEmit(Tree, CodeGen, states);
    }

#if _EXTRA_16BIT_CONVERSION_
OnError:
    /* Return the status. */
    return status;
#endif
}

static gceSTATUS
_DuplicateEmit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4],
    IN gctUINT SourceMask,
    IN gctUINT Enable,
    IN gctUINT8 Swizzle0,
    IN gctUINT8 Swizzle1,
    IN gctUINT8 Swizzle2,
    IN gctBOOL DuplicateOneComponent,
    IN gceCONVERT_TYPE ExtraHandling
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT /*addr,*/ addr0 = 0, addr1 = 0, addr2 = 0;
    gctUINT /*swizzle,*/ swizzle0, swizzle1, swizzle2;
    /*gctUINT swizzleMask, destEnable;*/
    gctUINT comp0 = 0, comp1 = 0, comp2 = 0;
    gctUINT restore0 = 0, restore1 = 0, restore2 = 0;
    gctUINT currentSource = CodeGen->nextSource - 1;
    const gctUINT enable[] = { 1, 2, 4, 8 };

    /* Restore back the lastUse for sources. */
    if ((((((gctUINT32) (States[1])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) ) &&
        (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) == 0x0)
    {
        addr0 = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
        swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
        comp0 = enable[swizzle0 & 0x3] | enable[(swizzle0 >> 2) & 0x3] |
                enable[(swizzle0 >> 4) & 0x3] | enable[(swizzle0 >> 6) & 0x3];
        if ((comp0 & 0x1) && (CodeGen->registerUsage[addr0].lastUse[0] == -1))
        {
            CodeGen->registerUsage[addr0].lastUse[0] = currentSource;
            restore0 |= 0x1;
        }
        if ((comp0 & 0x2) && (CodeGen->registerUsage[addr0].lastUse[1] == -1))
        {
            CodeGen->registerUsage[addr0].lastUse[1] = currentSource;
            restore0 |= 0x2;
        }
        if ((comp0 & 0x4) && (CodeGen->registerUsage[addr0].lastUse[2] == -1))
        {
            CodeGen->registerUsage[addr0].lastUse[2] = currentSource;
            restore0 |= 0x4;
        }
        if ((comp0 & 0x8) && (CodeGen->registerUsage[addr0].lastUse[3] == -1))
        {
            CodeGen->registerUsage[addr0].lastUse[3] = currentSource;
            restore0 |= 0x8;
        }
    }
    if ((((((gctUINT32) (States[2])) >> (0 ? 6:6)) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) ) &&
        (((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) == 0x0)
    {
        addr1 = (((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
        swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );
        comp1 = enable[swizzle1 & 0x3] | enable[(swizzle1 >> 2) & 0x3] |
                enable[(swizzle1 >> 4) & 0x3] | enable[(swizzle1 >> 6) & 0x3];
        if ((comp1 & 0x1) && (CodeGen->registerUsage[addr1].lastUse[0] == -1))
        {
            CodeGen->registerUsage[addr1].lastUse[0] = currentSource;
            restore1 |= 0x1;
        }
        if ((comp1 & 0x2) && (CodeGen->registerUsage[addr1].lastUse[1] == -1))
        {
            CodeGen->registerUsage[addr1].lastUse[1] = currentSource;
            restore1 |= 0x2;
        }
        if ((comp1 & 0x4) && (CodeGen->registerUsage[addr1].lastUse[2] == -1))
        {
            CodeGen->registerUsage[addr1].lastUse[2] = currentSource;
            restore1 |= 0x4;
        }
        if ((comp1 & 0x8) && (CodeGen->registerUsage[addr1].lastUse[3] == -1))
        {
            CodeGen->registerUsage[addr1].lastUse[3] = currentSource;
            restore1 |= 0x8;
        }
    }
    if ((((((gctUINT32) (States[3])) >> (0 ? 3:3)) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) ) &&
        (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) == 0x0)
    {
        addr2 = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
        swizzle2 = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );
        comp2 = enable[swizzle2 & 0x3] | enable[(swizzle2 >> 2) & 0x3] |
                enable[(swizzle2 >> 4) & 0x3] | enable[(swizzle2 >> 6) & 0x3];
        if ((comp2 & 0x1) && (CodeGen->registerUsage[addr2].lastUse[0] == -1))
        {
            CodeGen->registerUsage[addr2].lastUse[0] = currentSource;
            restore2 |= 0x1;
        }
        if ((comp2 & 0x2) && (CodeGen->registerUsage[addr2].lastUse[1] == -1))
        {
            CodeGen->registerUsage[addr2].lastUse[1] = currentSource;
            restore2 |= 0x2;
        }
        if ((comp2 & 0x4) && (CodeGen->registerUsage[addr2].lastUse[2] == -1))
        {
            CodeGen->registerUsage[addr2].lastUse[2] = currentSource;
            restore2 |= 0x4;
        }
        if ((comp2 & 0x8) && (CodeGen->registerUsage[addr2].lastUse[3] == -1))
        {
            CodeGen->registerUsage[addr2].lastUse[3] = currentSource;
            restore2 |= 0x8;
        }
    }


    if (DuplicateOneComponent)
    {
        if (Enable & gcSL_ENABLE_X)
        {
            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      gcSL_ENABLE_X,
                                      _ReplicateSwizzle(Swizzle0, 0),
                                      _ReplicateSwizzle(Swizzle1, 0),
                                      _ReplicateSwizzle(Swizzle2, 0),
                                      ExtraHandling));
        }

        if (Enable & gcSL_ENABLE_Y)
        {
            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      gcSL_ENABLE_Y,
                                      _ReplicateSwizzle(Swizzle0, 1),
                                      _ReplicateSwizzle(Swizzle1, 1),
                                      _ReplicateSwizzle(Swizzle2, 1),
                                      ExtraHandling));
        }

        if (Enable & gcSL_ENABLE_Z)
        {
            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      gcSL_ENABLE_Z,
                                      _ReplicateSwizzle(Swizzle0, 2),
                                      _ReplicateSwizzle(Swizzle1, 2),
                                      _ReplicateSwizzle(Swizzle2, 2),
                                      ExtraHandling));
        }

        if (Enable & gcSL_ENABLE_W)
        {
            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      gcSL_ENABLE_W,
                                      _ReplicateSwizzle(Swizzle0, 3),
                                      _ReplicateSwizzle(Swizzle1, 3),
                                      _ReplicateSwizzle(Swizzle2, 3),
                                      ExtraHandling));
        }
    }
    else
    {
        if (Enable & gcSL_ENABLE_XY)
        {
            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      Enable & gcSL_ENABLE_XY,
                                      _ReplicateSwizzle2(Swizzle0, 0),
                                      _ReplicateSwizzle2(Swizzle1, 0),
                                      _ReplicateSwizzle2(Swizzle2, 0),
                                      gcvCONVERT_COMPONENTXY));
        }

        if (Enable & gcSL_ENABLE_ZW)
        {
            gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                             | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;

            /* Change opcode from 0x3C to 0x3D, and so for others. */
            /* Assume each pair of opcodes are adjacent. */
            opcode++;
            States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (opcode & 0x3F) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (opcode >> 6) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

            gcmONERROR(_ComponentEmit(Tree,
                                      CodeGen,
                                      States,
                                      SourceMask,
                                      Enable & gcSL_ENABLE_ZW,
                                      _ReplicateSwizzle2(Swizzle0, 1),
                                      _ReplicateSwizzle2(Swizzle1, 1),
                                      _ReplicateSwizzle2(Swizzle2, 1),
                                      gcvCONVERT_COMPONENTZW));
        }
    }

OnError:
    /* Reset lastUse that was restored. */
    if (restore0)
    {
        if (restore0 & 0x1)
        {
            CodeGen->registerUsage[addr0].lastUse[0] = -1;
        }
        if (restore0 & 0x2)
        {
            CodeGen->registerUsage[addr0].lastUse[1] = -1;
        }
        if (restore0 & 0x4)
        {
            CodeGen->registerUsage[addr0].lastUse[2] = -1;
        }
        if (restore0 & 0x8)
        {
            CodeGen->registerUsage[addr0].lastUse[3] = -1;
        }
    }
    if (restore1)
    {
        if (restore1 & 0x1)
        {
            CodeGen->registerUsage[addr1].lastUse[0] = -1;
        }
        if (restore1 & 0x2)
        {
            CodeGen->registerUsage[addr1].lastUse[1] = -1;
        }
        if (restore1 & 0x4)
        {
            CodeGen->registerUsage[addr1].lastUse[2] = -1;
        }
        if (restore1 & 0x8)
        {
            CodeGen->registerUsage[addr1].lastUse[3] = -1;
        }
    }
    if (restore2)
    {
        if (restore2 & 0x1)
        {
            CodeGen->registerUsage[addr2].lastUse[0] = -1;
        }
        if (restore2 & 0x2)
        {
            CodeGen->registerUsage[addr2].lastUse[1] = -1;
        }
        if (restore2 & 0x4)
        {
            CodeGen->registerUsage[addr2].lastUse[2] = -1;
        }
        if (restore2 & 0x8)
        {
            CodeGen->registerUsage[addr2].lastUse[3] = -1;
        }
    }

    /* Return the status. */
    return status;
}

static gceSTATUS
_Emit(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 States[4]
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT valueType0;
    gctUINT instType0;
    gctUINT instType1;
    gctUINT8 enable;
    gctUINT8 swizzle0, swizzle1, swizzle2;

    if (!CodeGen->clShader)
    {
        return _FinalEmit(Tree, CodeGen, States);
    }

    /* Handle special requests for integer instructions for gc4000. */
    if (CodeGen->isGC4000)
    {
        /* Has only one 32-bit multiplier. */
        /* Has only one 8-/16-bit divider that cannot handle constant registers. */
        /* Need to duplicate instructions for each component. */
        /* Note that it takes only source component x only. */
        gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                         | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;
        gctBOOL duplicateOneComponent = gcvTRUE;

        switch (opcode)
        {
        case 0x3C:
            /* fall through */
        case 0x3E:
            /* fall through */
        case 0x40:
            /* fall through */
        case 0x42:
            /* Get value type. */
            instType0 = (((((gctUINT32) (States[1])) >> (0 ? 21:21)) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) );
            instType1 = (((((gctUINT32) (States[2])) >> (0 ? 31:30)) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1)))))) );
            valueType0 = instType0 | (instType1 << 1);

            if (valueType0 == 0x4
            ||  valueType0 == 0x7)
            {
                /* No extra handling needed. */
                break;
            }

            if (valueType0 == 0x3
            ||  valueType0 == 0x6)
            {
                duplicateOneComponent = gcvFALSE;
            }

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x3, /* Use src0 and src1. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      0,
                                      duplicateOneComponent,
                                      gcvCONVERT_NONE));
            return gcvSTATUS_OK;

        case 0x4C:
            /* fall through */
        case 0x4E:
            /* fall through */
        case 0x50:
            /* fall through */
        case 0x52:
            instType0 = (((((gctUINT32) (States[1])) >> (0 ? 21:21)) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) );
            instType1 = (((((gctUINT32) (States[2])) >> (0 ? 31:30)) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1)))))) );
            valueType0 = instType0 | (instType1 << 1);

            if (valueType0 == 0x4
            ||  valueType0 == 0x7)
            {
                /* No extra handling needed. */
                break;
            }

            if (valueType0 == 0x3
            ||  valueType0 == 0x6)
            {
                duplicateOneComponent = gcvFALSE;
            }

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );
            swizzle2 = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x7, /* Use src0, src1, and src2. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      swizzle2,
                                      duplicateOneComponent,
                                      gcvCONVERT_NONE));
            return gcvSTATUS_OK;

            /* Special handling for IDIV/IMOD. */
            /* Sources have to be temp registers. */
        case 0x44:
            /* fall through */
        case 0x48:
            if ((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) == 0x2)
            {
                /* Source 0 is constant: Copy source 0 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
            }
            if ((((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) == 0x2)
            {
                /* Source 1 is constant: Copy source 1 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 1));
            }

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x3, /* Use src0 and src1. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      0,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

        case 0x32:
            if (! CodeGen->hasBugFixes10)
            {
                return _ExtraEmit(Tree, CodeGen, States);
            }
            break;

        default:
            break;
        }

        return _FinalEmit(Tree, CodeGen, States);
    }

    /* Handle special requests for integer instructions for gc2100. */
    if (CodeGen->isGC2100)
    {
        /* Has only one 32-bit multiplier and one 32-bit adder. */
        /* Has only one 16-bit divider that cannot handle constant registers. */
        /* Need to duplicate instructions for each component. */
        /* Note that it takes only source component x only. */
        gctUINT32 opcode = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) )
                         | (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) ) << 6;

        switch (opcode)
        {
            /* One-operand opcodes that use SRC0. */
        case 0x2D:
            /* fall through */
        case 0x2E:

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x1, /* Use src0. */
                                      enable,
                                      swizzle0,
                                      0,
                                      0,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

            /* One-operand opcodes that use SRC2. */
        case 0x5F:
            /* fall through */
        case 0x57:
            /* fall through */
        case 0x58:

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle2 = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x4, /* Use src2. */
                                      enable,
                                      0,
                                      0,
                                      swizzle2,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

            /* Two-operand opcodes that use SRC0 and SRC1. */
        case 0x3C:
            /* fall through */
        case 0x40:
            /* fall through */
        case 0x3E:

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x3, /* Use src0 and src1. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      0,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

            /* Special handling for IDIV/IMOD. */
            /* Sources have to be temp registers. */
        case 0x44:
            /* fall through */
        case 0x48:

            if ((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) == 0x2)
            {
                /* Source 0 is constant: Copy source 0 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 0));
            }
            if ((((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) == 0x2)
            {
                /* Source 1 is constant: Copy source 1 to temp. */
                gcmERR_BREAK(_TempEmit(Tree, CodeGen, States, 1));
            }

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x3, /* Use src0 and src1. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      0,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

            /* Two-operand opcodes that use SRC0 and SRC2. */
        case 0x01:
            /* Get value type. */
            instType0 = (((((gctUINT32) (States[1])) >> (0 ? 21:21)) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) );
            instType1 = (((((gctUINT32) (States[2])) >> (0 ? 31:30)) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1)))))) );
            valueType0 = instType0 | (instType1 << 1);

            if (valueType0 == 0x0)
            {
                break;
            }
            /* fall through */
        case 0x3B:
            /* fall through */
        case 0x59:
            /* fall through */
        case 0x5A:
            /* fall through */
        case 0x5B:
            /* fall through */
        case 0x5C:
            /* fall through */
        case 0x5D:
            /* fall through */
        case 0x5E:

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle2 = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x5, /* Use src0 and src2. */
                                      enable,
                                      swizzle0,
                                      0,
                                      swizzle2,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;

            /* Three-operand opcodes. */
        case 0x4C:
            /* fall through */
        case 0x50:
            /* fall through */
        case 0x4E:
            /* fall through */
        case 0x52:

            enable   = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
            swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );
            swizzle2 = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

            gcmONERROR(_DuplicateEmit(Tree,
                                      CodeGen,
                                      States,
                                      0x7, /* Use src0, src1, and src2. */
                                      enable,
                                      swizzle0,
                                      swizzle1,
                                      swizzle2,
                                      gcvTRUE,
                                      gcvCONVERT_EXTRA));
            return gcvSTATUS_OK;


        case 0x32:
            if (! CodeGen->hasBugFixes10)
            {
                return _ExtraEmit(Tree, CodeGen, States);
            }
            break;

        default:
            break;
        }

        return _FinalEmit(Tree, CodeGen, States);
    }

    return _FinalEmit(Tree, CodeGen, States);

OnError:
    /* Return the status. */
    return status;
}

static gceSTATUS
_CodeGenMOVA(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctINT indexedReg,
    IN gctUINT formatOfIndexedReg,
    IN gctUINT32 addrEnable,
    IN gctUINT8 indexedRegSwizzle
    )
{
    gceSTATUS status;

    do
    {
        gctUINT states[4];

#if _USE_TEMP_FORMAT_
        gctUINT format = Tree->tempArray[indexedReg].format;
#else
        gctUINT format = formatOfIndexedReg;
#endif

        /* Zero the instruction. */
        gcmERR_BREAK(gcoOS_ZeroMemory(states, gcmSIZEOF(states)));

        if (format == gcSL_FLOAT)
        {
            /* MOVAF opcode. */
            _SetOpcode(states,
	 		           0x0B,
			           0x00,
			           gcvFALSE);
        }
        else
        {
            /* MOVAI opcode. */
            _SetOpcode(states,
			           0x56,
			           0x00,
			           gcvFALSE);
#if _USE_TEMP_FORMAT_
            /* MOVAI assumes non-32-bit data are packed. */
            /* Always use uint for now. */
	        /*_SetValueType0FromFormat(format, states);*/
	        _SetValueType0FromFormat(gcSL_UINT32, states);
#else
	        value_type0(Tree, CodeGen, instruction, states);
#endif
        }

        /* A0 register. */
        gcmERR_BREAK(_SetDest(Tree,
					          CodeGen,
					          states,
					          -1,
					          0x0,
					          addrEnable,
					          gcvNULL));

        /* Temporary register. */
        gcmERR_BREAK(_SetSource(Tree,
						        CodeGen,
						        states,
						        2,
						        gcSL_TEMP,
							    indexedReg,
						        0,
						        0x0,
						        indexedRegSwizzle, /*0xE4,*/
						        gcvFALSE,
						        gcvFALSE));

        /* Reset destination valid bit for a0 register. */
        states[0] = ((((gctUINT32) (states[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));

        /* Emit the instruction. */
        gcmERR_BREAK(_Emit(Tree, CodeGen, states));
    }
    while (gcvFALSE);

    return status;
}

static void
_SplitTmpAddrMapTableEntry(
    gcsSL_ADDRESS_REG_COLORING_PTR addRegColoring,
    gctINT entryToSplit,
    gctINT newChannelCount)
{
    gctINT i;

    gcmASSERT(addRegColoring->countOfMap < 4);

    /* split */
    for (i = addRegColoring->countOfMap - 1; i >= entryToSplit; i --)
    {
        if (i != entryToSplit)
        {
            addRegColoring->tmp2addrMap[i+1].channelCount =
                                       addRegColoring->tmp2addrMap[i].channelCount;

            addRegColoring->tmp2addrMap[i+1].indexedReg =
                                       addRegColoring->tmp2addrMap[i].indexedReg;

            addRegColoring->tmp2addrMap[i+1].startChannelInAddressReg =
                                       addRegColoring->tmp2addrMap[i].startChannelInAddressReg;

            addRegColoring->tmp2addrMap[i+1].startChannelInIndexedReg =
                                       addRegColoring->tmp2addrMap[i].startChannelInIndexedReg;
        }
        else
        {
            addRegColoring->tmp2addrMap[i].channelCount = addRegColoring->tmp2addrMap[i].channelCount - newChannelCount;
            addRegColoring->tmp2addrMap[i+1].channelCount = newChannelCount;
            /* Split will make old and new entries are all invalid, i.e indexedReg == -1 */
            addRegColoring->tmp2addrMap[i].indexedReg = addRegColoring->tmp2addrMap[i+1].indexedReg = -1;
            addRegColoring->tmp2addrMap[i+1].startChannelInAddressReg =
                   (gctUINT8)(addRegColoring->tmp2addrMap[i].startChannelInAddressReg + addRegColoring->tmp2addrMap[i].channelCount);
            addRegColoring->tmp2addrMap[i+1].startChannelInIndexedReg = -1;
        }
    }

    addRegColoring->countOfMap ++;
}

static void
_MergeTmpAddrMapTableAdjacentEntries(
    gcsSL_ADDRESS_REG_COLORING_PTR addRegColoring,
    gctINT PivotEntryToMerge,
    gctBOOL bRightAdj)
{
    gctINT i, leftIdx, rightIdx;

    gcmASSERT(addRegColoring->countOfMap <= 4);

    if (bRightAdj)
    {
        leftIdx = PivotEntryToMerge;
        rightIdx = leftIdx + 1;
    }
    else
    {
        rightIdx = PivotEntryToMerge;
        leftIdx = rightIdx - 1;
    }

    /* Merge will make new entry is invalid, i.e indexedReg == -1 */
    addRegColoring->tmp2addrMap[leftIdx].channelCount += addRegColoring->tmp2addrMap[rightIdx].channelCount;
    addRegColoring->tmp2addrMap[leftIdx].indexedReg = -1;

    for (i = rightIdx+1; i < addRegColoring->countOfMap; i ++)
    {
        addRegColoring->tmp2addrMap[i-1].channelCount =
                                   addRegColoring->tmp2addrMap[i].channelCount;

        addRegColoring->tmp2addrMap[i-1].indexedReg =
                                   addRegColoring->tmp2addrMap[i].indexedReg;

        addRegColoring->tmp2addrMap[i-1].startChannelInAddressReg =
                                   addRegColoring->tmp2addrMap[i].startChannelInAddressReg;

        addRegColoring->tmp2addrMap[i-1].startChannelInIndexedReg =
                                   addRegColoring->tmp2addrMap[i].startChannelInIndexedReg;
    }

    addRegColoring->countOfMap --;
}

static gceSTATUS
_FindAddressRegChannel(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctINT indexedReg,
    IN gctUINT formatOfIndexedReg,
    IN gcSL_INDEXED IndexedChannelOfReg,
    OUT gctUINT8* singleAddrEnable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT    startChannelInAddressReg = -1;
    gctUINT8  curAddrEnableMask = 0;
    gcsSL_ADDRESS_REG_COLORING_PTR addRegColoring = &CodeGen->current->addrRegColoring;

    do
    {
        gctBOOL bNeedGenMova = gcvTRUE;
        gctINT mapIdx, i;

        gcmASSERT(indexedReg >= 0);

        /* Check whether there is a mova generated for this indexed channel already */
        for (mapIdx = 0; mapIdx < CodeGen->current->addrRegColoring.countOfMap; mapIdx ++)
        {
            /* This reg has been mapped */
            if (addRegColoring->tmp2addrMap[mapIdx].indexedReg == indexedReg)
            {
                /* Shift it to zero-based */
                gctINT adjustedIndexedChannelOfReg = IndexedChannelOfReg - 1;

                if ((adjustedIndexedChannelOfReg >= addRegColoring->tmp2addrMap[mapIdx].startChannelInIndexedReg) &&
                    (adjustedIndexedChannelOfReg <= (addRegColoring->tmp2addrMap[mapIdx].startChannelInIndexedReg +
                     addRegColoring->tmp2addrMap[mapIdx].channelCount - 1)))
                {
                    bNeedGenMova = gcvFALSE;
                    startChannelInAddressReg = addRegColoring->tmp2addrMap[mapIdx].startChannelInAddressReg;

                    for (i = startChannelInAddressReg; i < startChannelInAddressReg +
                                      addRegColoring->tmp2addrMap[mapIdx].channelCount; i ++)
                    {
                        curAddrEnableMask |= (1 << i);
                    }

                    break;
                }
            }
        }

        if (bNeedGenMova)
	    {
            gctUINT8 indexedRegEnableMask = 0, indexedRegEnableMaskTemp = 0;
            gctUINT8 indexedRegSwizzle = 0;
            gctINT mapIdxToInsert = -1;
            gctINT startChannelInIndexedReg = -1, enabledChannelCount = 0;
            gcsLINKTREE_LIST_PTR def;
            gcsLINKTREE_LIST_PTR user;
            gctINT j;

            /* Figure out how many channel are used and which channel is the first channel in temp register */
            gcmASSERT(Tree->tempArray[indexedReg].constUsage[0] != 1 ||
                      Tree->tempArray[indexedReg].constUsage[1] != 1 ||
                      Tree->tempArray[indexedReg].constUsage[2] != 1 ||
                      Tree->tempArray[indexedReg].constUsage[3] != 1);

            def = Tree->tempArray[indexedReg].defined;
            while (def)
            {
                gcSL_INSTRUCTION inst = Tree->shader->code + def->index;
                indexedRegEnableMaskTemp |= gcmSL_TARGET_GET(inst->temp, Enable);
                def = def->next;
            }

            user = Tree->tempArray[indexedReg].users;
            while (user)
            {
                gcSL_INSTRUCTION inst = Tree->shader->code + user->index;

                /* Only check indexed user */
                if (inst->tempIndexed == indexedReg &&
                    gcmSL_TARGET_GET(inst->temp, Indexed) != gcSL_NOT_INDEXED)
                {
                    indexedRegEnableMask |= (1 << (gcmSL_TARGET_GET(inst->temp, Indexed) - 1));
                }

                if (inst->source0Indexed == indexedReg &&
                    gcmSL_SOURCE_GET(inst->source0, Indexed) != gcSL_NOT_INDEXED)
                {
                    indexedRegEnableMask |= (1 << (gcmSL_SOURCE_GET(inst->source0, Indexed) - 1));
                }

                if (inst->source1Indexed == indexedReg &&
                    gcmSL_SOURCE_GET(inst->source1, Indexed) != gcSL_NOT_INDEXED)
                {
                    indexedRegEnableMask |= (1 << (gcmSL_SOURCE_GET(inst->source1, Indexed) - 1));
                }

                user = user->next;
            }

            gcmASSERT((indexedRegEnableMask & indexedRegEnableMaskTemp) == indexedRegEnableMask);

            for (i = 0; i < 4; i ++)
            {
                if ((indexedRegEnableMask >> i) & 0x01)
                {
                    if (startChannelInIndexedReg == -1)
                        startChannelInIndexedReg = i;

                    /* Note we permit a hole among channels, otherwise we need */
                    /* more complicated algorithm to handle it, at least we need */
                    /* address register in glSL level. */
                    enabledChannelCount = i - startChannelInIndexedReg + 1;
                }
            }

            gcmASSERT(enabledChannelCount > 0 && startChannelInIndexedReg >= 0);

            /* Check whether we have enough space to hold current address channels assignment */
            if (addRegColoring->countOfMap > 0)
            {
                while (startChannelInAddressReg == -1)
                {
                    gcmASSERT(addRegColoring->countOfMap <= 4);

                    /* Check empty, maybe needs map table split */
                    for (mapIdx = 0; mapIdx < CodeGen->current->addrRegColoring.countOfMap; mapIdx ++)
                    {
                        if (addRegColoring->tmp2addrMap[mapIdx].indexedReg == -1)
                        {
                            gctINT deltaChannelCount = addRegColoring->tmp2addrMap[mapIdx].channelCount - enabledChannelCount;

                            if (deltaChannelCount < 0)
                                continue;

                            if (deltaChannelCount > 0)
                            {
                                _SplitTmpAddrMapTableEntry(addRegColoring, mapIdx, deltaChannelCount);
                            }

                            mapIdxToInsert = mapIdx;
                            startChannelInAddressReg = addRegColoring->tmp2addrMap[mapIdx].startChannelInAddressReg;
                            break;
                        }
                    }

                    /* Check tail unused part */
                    if (startChannelInAddressReg == -1)
                    {
                        gcmASSERT(addRegColoring->tmp2addrMap[addRegColoring->countOfMap - 1].indexedReg != -1);

                        if (4 - (addRegColoring->tmp2addrMap[addRegColoring->countOfMap - 1].startChannelInAddressReg +
                            addRegColoring->tmp2addrMap[addRegColoring->countOfMap - 1].channelCount) >= enabledChannelCount)
                        {
                            startChannelInAddressReg = addRegColoring->tmp2addrMap[addRegColoring->countOfMap - 1].startChannelInAddressReg +
                                                   addRegColoring->tmp2addrMap[addRegColoring->countOfMap - 1].channelCount;
                        }
                    }

                    /* Register spill here and split/merge */
                    if (startChannelInAddressReg == -1)
                    {
                        gctUINT8  tempAddrEnableMask = 0, cstart;
                        gctINT    ccount;
                        gctBOOL   bFound = gcvFALSE;
                        gctINT    startIdx = -1, endIdx = -1, tcount = 0;

                        /* A very rough heuristic here since we have no live analysis */
                        /* and spill cost analysis for address register. At least, we */
                        /* should add last usage of temp register for address access */
                        /* purpose then we can mark this unused after last usage */
                        /*            ****NEED REFINE THIS SPILL****             */

                        /* Always select foremost entries that can accommodate it */
                        for (mapIdx = 0; mapIdx < CodeGen->current->addrRegColoring.countOfMap; mapIdx ++)
                        {
                            cstart = addRegColoring->tmp2addrMap[mapIdx].startChannelInAddressReg;
                            ccount = addRegColoring->tmp2addrMap[mapIdx].channelCount;
                            tcount += ccount;

                            for (i = cstart; i < cstart + ccount; i ++)
                            {
                                tempAddrEnableMask |= (1 << i);
                            }

                            /* Need consider local usage mask here */
                            if (!(tempAddrEnableMask & addRegColoring->localAddrChannelUsageMask))
                            {
                                endIdx = mapIdx;
                                if (startIdx == -1)
                                    startIdx = mapIdx;

                                if (tcount >= enabledChannelCount)
                                {
                                    bFound = gcvTRUE;
                                    break;
                                }
                            }
                            else
                            {
                                /* reset */
                                startIdx = -1;
                                endIdx = -1;
                                tcount = 0;
                                tempAddrEnableMask = 0;
                            }
                        }

                        if (bFound)
                        {
                            if (startIdx == endIdx)
                            {
                                addRegColoring->tmp2addrMap[startIdx].indexedReg = -1;
                            }
                            else
                            {
                                /* merge */
                                for (mapIdx = endIdx-1; mapIdx >= startIdx; mapIdx --)
                                {
                                    _MergeTmpAddrMapTableAdjacentEntries(addRegColoring, mapIdx, gcvTRUE);
                                }
                            }
                        }
                        else
                        {
                            /* Goto compile error */
                            break;
                        }
                    }
                }

                /* Oops, fail to allocate since no enough channel space */
                if (startChannelInAddressReg == -1)
                {
                    status = gcvSTATUS_OUT_OF_RESOURCES;

                    /* This 'break' must be at the outmost loop for status return */
                    break;
                }
            }
            else
            {
                startChannelInAddressReg = 0;
            }

            gcmASSERT(startChannelInAddressReg >= 0);

            /* Ok, we can assign space safely, so update tmp2add mapping table */
            if (mapIdxToInsert == -1)
            {
                addRegColoring->tmp2addrMap[addRegColoring->countOfMap].channelCount = enabledChannelCount;
                addRegColoring->tmp2addrMap[addRegColoring->countOfMap].indexedReg = indexedReg;
                addRegColoring->tmp2addrMap[addRegColoring->countOfMap].startChannelInAddressReg = (gctUINT8)startChannelInAddressReg;
                addRegColoring->tmp2addrMap[addRegColoring->countOfMap].startChannelInIndexedReg = startChannelInIndexedReg;
                CodeGen->current->addrRegColoring.countOfMap ++;
            }
            else
            {
                gcmASSERT(addRegColoring->tmp2addrMap[mapIdxToInsert].indexedReg == -1);

                addRegColoring->tmp2addrMap[mapIdxToInsert].channelCount = enabledChannelCount;
                addRegColoring->tmp2addrMap[mapIdxToInsert].indexedReg = indexedReg;
                addRegColoring->tmp2addrMap[mapIdxToInsert].startChannelInAddressReg = (gctUINT8)startChannelInAddressReg;
                addRegColoring->tmp2addrMap[mapIdxToInsert].startChannelInIndexedReg = startChannelInIndexedReg;
            }

            /* Now, add a "MOVAF a0, temp(x)" instruction. */
            for (i = startChannelInAddressReg, j = 0; i < startChannelInAddressReg + enabledChannelCount; i ++, j ++)
            {
                curAddrEnableMask |= (1 << i);
                indexedRegSwizzle |= ((startChannelInIndexedReg + j) << i*2);
            }

            gcmERR_BREAK(_CodeGenMOVA(Tree, CodeGen, indexedReg, formatOfIndexedReg, curAddrEnableMask, indexedRegSwizzle));
	    }
    }
    while (gcvFALSE);

    if (!gcmIS_ERROR(status))
    {
        gctUINT channelInAddrReg = (startChannelInAddressReg + IndexedChannelOfReg - 1);
        gcmASSERT(channelInAddrReg <= 3);

        *singleAddrEnable = (1 << channelInAddrReg);

        /* Update local usage mask */
        addRegColoring->localAddrChannelUsageMask |= curAddrEnableMask;
    }

    return status;
}

static gceSTATUS
_ProcessDestination(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    gctINT Reference,
    IN OUT gctUINT32 * States,
    OUT gctINT_PTR Shift,
    IN gcsSL_PATTERN_PTR Pattern
    )
{
    gcsSL_REFERENCE_PTR match = gcvNULL;
    gcSL_INSTRUCTION instruction;
    gceSTATUS status;
    gcsSL_FUNCTION_CODE_PTR function = CodeGen->current;

    gctUINT8 addrEnable;
    gctUINT32 addrIndexed;

    addrEnable = 0;
    addrIndexed = gcSL_NOT_INDEXED;

    do
    {
        /* Find the reference. */
        if (!_FindReference(Tree, CodeGen, Reference, &match, Pattern))
        {
            gcmERR_BREAK(gcvSTATUS_NOT_FOUND);
        }

        /* Reference the instruction. */
        instruction = match->instruction;

        /* Check whether this is a destination. */
        if (match->sourceIndex == -1)
        {
            /* Get the referenced destination. */
            gctUINT16 target  = instruction->temp;
            gctBOOL skipConst = (instruction->opcode == gcSL_MOV);

            /* Check whether the target is indexed by a different register. */
            if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            {
                gcmERR_BREAK(_FindAddressRegChannel(Tree,
                                                    CodeGen,
                                                    instruction->tempIndexed,
                                                    gcmSL_TARGET_GET(target, Format),
                                                    gcmSL_TARGET_GET(target, Indexed),
                                                    &addrEnable
                                                    ));
            }

            /* Test if this is a MOV which could be a constant propagation. */
            if (skipConst
            &&  (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_X)
            &&  (instruction->tempIndex < Tree->tempCount)
            &&  (Tree->tempArray[instruction->tempIndex].constUsage[0] != 1)
            )
            {
                /* X is used but is not marked as a constant. */
                skipConst = gcvFALSE;
            }

            if (skipConst
            &&  (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_Y)
            &&  (instruction->tempIndex < Tree->tempCount)
            &&  (Tree->tempArray[instruction->tempIndex].constUsage[1] != 1)
            )
            {
                /* Y is used but is not marked as a constant. */
                skipConst = gcvFALSE;
            }

            if (skipConst
            &&  (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_Z)
            &&  (instruction->tempIndex < Tree->tempCount)
            &&  (Tree->tempArray[instruction->tempIndex].constUsage[2] != 1)
            )
            {
                /* Z is used but is not marked as a constant. */
                skipConst = gcvFALSE;
            }

            if (skipConst
            &&  (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_W)
            &&  (instruction->tempIndex < Tree->tempCount)
            &&  (Tree->tempArray[instruction->tempIndex].constUsage[3] != 1)
            )
            {
                /* W is used but is not marked as a constant. */
                skipConst = gcvFALSE;
            }

            if (skipConst)
            {
                /* Skip generation of MOV to constant propagation registers. */
                return gcvSTATUS_SKIP;
            }

            if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            {
                switch (addrEnable)
                {
                case 1:
	                addrIndexed = gcSL_INDEXED_X; break;
                case 2:
	                addrIndexed = gcSL_INDEXED_Y; break;
                case 4:
	                addrIndexed = gcSL_INDEXED_Z; break;
                case 8:
	                addrIndexed = gcSL_INDEXED_W; break;
                }
            }

            /* Program destination. */
            gcmERR_BREAK(_SetDest(Tree,
                                  CodeGen,
                                  States,
                                  *(gctINT16_PTR) &instruction->tempIndex,
                                  addrIndexed,
                                  gcmSL_TARGET_GET(target, Enable),
                                  Shift));
        }
        else
        {
            /* Get the referenced source. */
            gctUINT16 source = (match->sourceIndex == 0)
                ? instruction->source0
                : instruction->source1;

            gctUINT16 sourceIndex = (match->sourceIndex == 0)
                ? instruction->source0Index
                : instruction->source1Index;

            gctINT index = gcmSL_INDEX_GET(sourceIndex, Index);

            gcmASSERT(gcmSL_SOURCE_GET(source, Type)    == gcSL_TEMP ||
                      gcmSL_SOURCE_GET(source, Type)    == gcSL_ATTRIBUTE);
            gcmASSERT(gcmSL_SOURCE_GET(source, Indexed) == gcSL_NOT_INDEXED);

            if (gcmSL_SOURCE_GET(source, Type)    == gcSL_ATTRIBUTE)
            {
                /* set the index to be the attribute register location */
                 index  = - (Tree->shader->attributes[index]->inputIndex + 1);
            }

            /* Program destination. */
            gcmERR_BREAK(_SetDest(Tree,
                                  CodeGen,
                                  States,
                                  index,
                                  0x0,
                                  _Swizzle2Enable((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
                                                  (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
                                                  (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
                                                  (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW)),
                                  Shift));
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (gcmNO_ERROR(status) && (*Shift == -1) )
    {
        switch (Reference)
        {
        case gcSL_CG_TEMP1_X:
        /* fall through */
        case gcSL_CG_TEMP1_XY:
        /* fall through */
        case gcSL_CG_TEMP1_XYZ:
        /* fall through */
        case gcSL_CG_TEMP1_XYZW:
            *Shift = function->tempShift[0];
            break;

        case gcSL_CG_TEMP2_X:
        /* fall through */
        case gcSL_CG_TEMP2_XY:
        /* fall through */
        case gcSL_CG_TEMP2_XYZ:
        /* fall through */
        case gcSL_CG_TEMP2_XYZW:
            *Shift = function->tempShift[1];
            break;

        case gcSL_CG_TEMP3_X:
        /* fall through */
        case gcSL_CG_TEMP3_XY:
        /* fall through */
        case gcSL_CG_TEMP3_XYZ:
        /* fall through */
        case gcSL_CG_TEMP3_XYZW:
            *Shift = function->tempShift[2];
            break;

        default:
            break;
        }
    }

    /* Return the status. */
    return status;
}

static gceSTATUS
_ProcessSource(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    gctINT Reference,
    IN OUT gctUINT32 * States,
    IN gctUINT Source,
    IN gctINT Shift
    )
{
    gcsSL_REFERENCE_PTR match = gcvNULL;
    gcSL_INSTRUCTION instruction;
    gceSTATUS status;
    gctUINT8 swizzle;
    gctUINT8 addrEnable;
    gctUINT32 addrIndexed;

    addrEnable = 0;
    addrIndexed = gcSL_NOT_INDEXED;

    do
    {
        /* Special case constants. */
        if (gcmABS(Reference) == gcSL_CG_CONSTANT)
        {
            status = _SetSource(Tree,
                                CodeGen,
                                States,
                                Source,
                                gcSL_CONSTANT,
                                0,
                                0,
                                0x0,
                                gcSL_SWIZZLE_XXXX,
                                Reference < 0,
                                gcvFALSE);
            break;
        }

        /* Find the referenced instruction. */
        if (!_FindReference(Tree, CodeGen, gcmABS(Reference), &match, gcvNULL))
        {
            gcmERR_BREAK(gcvSTATUS_NOT_FOUND);
        }

        instruction = match->instruction;

        if (match->sourceIndex < 0)
        {
            /* Extract the referenced target. */
            gctUINT16 target = instruction->temp;
            gctUINT8 targetEnable = gcmSL_TARGET_GET(target, Enable);

            /* For dest as source for some spliting case, if dest dost not
               start with x channel, we need shift it, for example
               sin r2.yz, r1.wwy =>

               mul r2.yz, r1.wwy, c0.x
               sin r2.yz, r2.yyz (can not be yz here)
            */
            gctINT shiftInternal = 0;

            gcmASSERT(gcmABS(Reference) == 1 || gcmABS(Reference) >= gcSL_CG_TEMP1);

            while (!((targetEnable >> shiftInternal) & 0x1))
            {
                shiftInternal ++;
            }

            Shift =  (Shift < 0) ? shiftInternal : (shiftInternal + Shift);

            swizzle = _Enable2Swizzle(targetEnable);

            while (Shift-- > 0)
            {
                swizzle = (swizzle << 2) | (swizzle & 0x3);
            }

            /* Set the source opcode. */
            /* Check whether the target is indexed by a different register. */
            if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            {
                gcmERR_BREAK(_FindAddressRegChannel(Tree,
                                                    CodeGen,
                                                    instruction->tempIndexed,
                                                    gcmSL_TARGET_GET(target, Format),
                                                    gcmSL_TARGET_GET(target, Indexed),
                                                    &addrEnable
                                                    ));
            }

            if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            {
                switch (addrEnable)
                {
                case 1:
	                addrIndexed = gcSL_INDEXED_X; break;
                case 2:
	                addrIndexed = gcSL_INDEXED_Y; break;
                case 4:
	                addrIndexed = gcSL_INDEXED_Z; break;
                case 8:
	                addrIndexed = gcSL_INDEXED_W; break;
                }
            }

            gcmERR_BREAK(_SetSource(Tree,
                                    CodeGen,
                                    States,
                                    Source,
                                    gcSL_TEMP,
                                    *(gctINT16_PTR) &instruction->tempIndex,
                                    0,
                                    addrIndexed,
                                    swizzle,
                                    Reference < 0,
                                    gcvFALSE));
        }
        else
        {
            /* Extract the referenced source. */
            gctUINT16 source = (match->sourceIndex == 0)
                ? instruction->source0
                : instruction->source1;
            gctUINT16 index = (match->sourceIndex == 0)
                ? instruction->source0Index
                : instruction->source1Index;
            gctINT indexed = (match->sourceIndex == 0)
                ? instruction->source0Indexed
                : instruction->source1Indexed;

            /* See if the source is a constant. */
            if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
            {
                gctINT uniform;

                /* Extract the float constant. */
                union
                {
                    gctFLOAT f;
                    gctUINT16 hex[2];
                }
                value;

                value.hex[0] = (match->sourceIndex == 0)
                    ? instruction->source0Index
                    : instruction->source1Index;
                value.hex[1] = (match->sourceIndex == 0)
                    ? instruction->source0Indexed
                    : instruction->source1Indexed;

                /* Allocate the constant. */
                gcmERR_BREAK(_AddConstantVec1(Tree,
                                              CodeGen,
                                              value.f,
                                              &uniform,
                                              &swizzle));

                /* Set the source operand. */
                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        States,
                                        Source,
                                        gcSL_CONSTANT,
                                        uniform,
                                        0,
                                        0x0,
                                        swizzle,
                                        Reference < 0,
                                        gcvFALSE));
            }
            else
            {
                gctUINT constIndex;

                /* Check whether the source is indexed by a different register. */
                if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                {
                    gcmERR_BREAK(_FindAddressRegChannel(Tree,
                                                        CodeGen,
                                                        indexed,
                                                        gcmSL_SOURCE_GET(source, Format),
                                                        gcmSL_SOURCE_GET(source, Indexed),
                                                        &addrEnable
                                                        ));
                }

                /* Get constant index. */
                constIndex = (gcmSL_SOURCE_GET(source, Indexed) == gcSL_NOT_INDEXED)
                           ? indexed + gcmSL_INDEX_GET(index, ConstValue)
                           : gcmSL_INDEX_GET(index, ConstValue);

                if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                {
                    switch (addrEnable)
	                {
	                case 1:
		                addrIndexed = gcSL_INDEXED_X; break;
	                case 2:
		                addrIndexed = gcSL_INDEXED_Y; break;
	                case 4:
		                addrIndexed = gcSL_INDEXED_Z; break;
	                case 8:
		                addrIndexed = gcSL_INDEXED_W; break;
	                }
                }

                /* Compute shifted swizzle. */
                swizzle = gcmSL_SOURCE_GET(source, Swizzle);

                while (Shift-- > 0)
                {
                    swizzle = (swizzle << 2) | (swizzle & 0x3);
                }

                /* Set the source operand. */
                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        States,
                                        Source,
                                        (gcSL_TYPE) gcmSL_SOURCE_GET(source, Type),
                                        gcmSL_INDEX_GET(index, Index),
                                        constIndex,
                                        addrIndexed,
                                        swizzle,
                                        Reference < 0,
                                        gcvFALSE));
            }
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the staus. */
    return status;
}

static gceSTATUS
_ProcessSampler(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    gctINT Reference,
    IN OUT gctUINT32 States[4]
    )
{
    gcsSL_REFERENCE_PTR match = gcvNULL;
    gcSL_INSTRUCTION instruction;
    gceSTATUS status;
    gctUINT16 index = 0;
    gctUINT32 indexed = 0, swizzle = 0, relative = 0;
    gctUINT8 addrEnable = 0;
    gctUINT srcFormat = gcSL_FLOAT;

    do
    {
        /* Find the reference. */
        if (!_FindReference(Tree, CodeGen, Reference, &match, gcvNULL))
        {
            gcmERR_BREAK(gcvSTATUS_NOT_FOUND);
        }

        /* Reference the instruction. */
        instruction = match->instruction;

        /* Decode source. */
        switch (match->sourceIndex)
        {
        case 0:
            relative = gcmSL_SOURCE_GET(instruction->source0, Indexed);
            swizzle  = gcmSL_SOURCE_GET(instruction->source0, Swizzle);
            index    = instruction->source0Index;
            indexed  = instruction->source0Indexed;
            srcFormat = gcmSL_SOURCE_GET(instruction->source0, Format);
            break;

        case 1:
            relative = gcmSL_SOURCE_GET(instruction->source1, Indexed);
            swizzle  = gcmSL_SOURCE_GET(instruction->source1, Swizzle);
            index    = instruction->source1Index;
            indexed  = instruction->source1Indexed;
            srcFormat = gcmSL_SOURCE_GET(instruction->source1, Format);
            break;

        default:
            gcmFATAL("Sample cannot be a target???");
        }

        switch (relative)
        {
        case gcSL_NOT_INDEXED:
            relative = 0x0;
            break;

        case gcSL_INDEXED_X:
            if (Tree->tempArray[indexed].constUsage[0] == 1)
            {
                index    = gcmSL_INDEX_SET(index,
                                           Index,
                                           gcmSL_INDEX_GET(index, Index)
                                           + (gctUINT32) Tree->tempArray[indexed].constValue[0]);
                relative = 0x0;
            }
            else
            {
                relative = 0x1;
            }
            break;

        case gcSL_INDEXED_Y:
            if (Tree->tempArray[indexed].constUsage[1] == 1)
            {
                index    = gcmSL_INDEX_SET(index,
                                           Index,
                                           gcmSL_INDEX_GET(index, Index)
                                           + (gctUINT32) Tree->tempArray[indexed].constValue[1]);
                relative = 0x0;
            }
            else
            {
                relative = 0x2;
            }
            break;

        case gcSL_INDEXED_Z:
            if (Tree->tempArray[indexed].constUsage[2] == 1)
            {
                index    = gcmSL_INDEX_SET(index,
                                           Index,
                                           gcmSL_INDEX_GET(index, Index)
                                           + (gctUINT32) Tree->tempArray[indexed].constValue[2]);
                relative = 0x0;
            }
            else
            {
                relative = 0x3;
            }
            break;

        case gcSL_INDEXED_W:
            if (Tree->tempArray[indexed].constUsage[3] == 1)
            {
                index    = gcmSL_INDEX_SET(index,
                                           Index,
                                           gcmSL_INDEX_GET(index, Index)
                                           + (gctUINT32) Tree->tempArray[indexed].constValue[3]);
                relative = 0x0;
            }
            else
            {
                relative = 0x4;
            }
            break;

        default:
            break;
        }

        if (relative != 0x0)
        {
            gcmERR_BREAK(_FindAddressRegChannel(Tree,
                                                CodeGen,
                                                indexed,
                                                srcFormat,
                                                relative,
                                                &addrEnable
                                                ));

            switch (addrEnable)
            {
            case 1:
                relative = 0x1; break;
            case 2:
                relative = 0x2; break;
            case 4:
                relative = 0x3; break;
            case 8:
                relative = 0x4; break;
            }
        }

        /* Set sampler. */
        gcmERR_BREAK(_SetSampler(Tree,
                                 CodeGen,
                                 States,
                                 gcmSL_INDEX_GET(index, Index),
                                 relative,
                                 swizzle));
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetTarget(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcsSL_FUNCTION_CODE_PTR Function,
    IN gctUINT Ip,
    IN gctUINT Target
    )
{
    gcsSL_PHYSICAL_CODE_PTR code;

    /* Find the code that needs to be patched. */
    for (code = Function->root; code != gcvNULL; code = code->next)
    {
        if (Ip < code->count)
        {
            /* Get the opcode for the instruction. */
            gctUINT32 opcode = (((((gctUINT32) (code->states[Ip * 4 + 0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) );

            if ((opcode != 0x14)
            &&  (opcode != 0x16)
            )
            {
                /* Move to the next instruction - emit might have inserted
                   instructions! */
                ++Ip;
            }
        }

        if (Ip < code->count)
        {
            /* Patch the code. */
            code->states[Ip * 4 + 3] = ((((gctUINT32) (code->states[Ip * 4 + 3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7))) | (((gctUINT32) ((gctUINT32) (Target) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1))))))) << (0 ? 18:7)));

            /* Success. */
            return gcvSTATUS_OK;
        }

        /* Next list in the code. */
        Ip -= code->count;
    }

    /* Index not found. */
    return gcvSTATUS_INVALID_INDEX;
}

static gctUINT32
_gc2shCondition(
    IN gcSL_CONDITION Condition
    )
{
    switch (Condition)
    {
    case gcSL_ALWAYS:
        return 0x00;

    case gcSL_NOT_EQUAL:
        return 0x06;

    case gcSL_LESS_OR_EQUAL:
        return 0x04;

    case gcSL_LESS:
        return 0x02;

    case gcSL_EQUAL:
        return 0x05;

    case gcSL_GREATER:
        return 0x01;

    case gcSL_GREATER_OR_EQUAL:
        return 0x03;

    case gcSL_AND:
        return 0x07;

    case gcSL_OR:
        return 0x08;

    case gcSL_XOR:
        return 0x09;

    case gcSL_NOT_ZERO:
        return 0x0B;

    default:
        gcmFATAL("Unknown condition: %X", Condition);
        return 0x00;
    }
}

typedef enum _gceINSTRUCTION_TYPE
{
    gcvINSTRUCTION_NORMAL,
    gcvINSTRUCTION_TRANSCENDENTAL_ONE_SOURCE,
    gcvINSTRUCTION_TRANSCENDENTAL_TWO_SOURCES
}
gceINSTRUCTION_TYPE;

static gceINSTRUCTION_TYPE
_GetInstructionType(
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction
    )
{
    switch (Instruction->opcode)
    {
    case gcSL_RCP:
        /* fall through */
    case gcSL_RSQ:
        /* fall through */
    case gcSL_EXP:
        /* fall through */
    case gcSL_LOG:
        /* fall through */
    case gcSL_SQRT:
        return gcvINSTRUCTION_TRANSCENDENTAL_ONE_SOURCE;

    case gcSL_MUL:
        if (gcmSL_TARGET_GET(Instruction->temp, Format) == gcSL_FLOAT)
        {
            return gcvINSTRUCTION_NORMAL;
        }

        /* fall through */
    case gcSL_MULLO:
        /* fall through */
    case gcSL_MULHI:
        /* fall through */
    case gcSL_MULSAT:
        if (CodeGen->isGC4000)
        {
            gcSL_FORMAT format = (gcSL_FORMAT) gcmSL_SOURCE_GET(Instruction->temp, Format);

            if (format != gcSL_INT32 && format != gcSL_UINT32)
            {
                return gcvINSTRUCTION_NORMAL;
            }
        }
        return gcvINSTRUCTION_TRANSCENDENTAL_TWO_SOURCES;

    case gcSL_NOT_BITWISE:
        /* fall through */
    case gcSL_LEADZERO:
        /* fall through */
    case gcSL_CONV:
        /* fall through */
    case gcSL_I2F:
        /* fall through */
    case gcSL_F2I:
        if (CodeGen->isGC2100)
        {
            return gcvINSTRUCTION_TRANSCENDENTAL_ONE_SOURCE;
        }
        return gcvINSTRUCTION_NORMAL;

    case gcSL_ADD:
        /* fall through */
    case gcSL_SUB:
        if (gcmSL_TARGET_GET(Instruction->temp, Format) == gcSL_FLOAT)
        {
            return gcvINSTRUCTION_NORMAL;
        }

        /* fall through */
    case gcSL_DIV:
        /* fall through */
    case gcSL_MOD:
        /* fall through */
    case gcSL_AND_BITWISE:
        /* fall through */
    case gcSL_OR_BITWISE:
        /* fall through */
    case gcSL_XOR_BITWISE:
        /* fall through */
    case gcSL_LSHIFT:
        /* fall through */
    case gcSL_RSHIFT:
        /* fall through */
    case gcSL_ROTATE:
        /* fall through */
    case gcSL_ADDLO:
        /* fall through */
    case gcSL_ADDSAT:
        /* fall through */
    case gcSL_SUBSAT:
        if (CodeGen->isGC2100)
        {
            return gcvINSTRUCTION_TRANSCENDENTAL_TWO_SOURCES;
        }

        /* fall through */
    default:
        return gcvINSTRUCTION_NORMAL;
    }
}

static gceSTATUS
_GenerateFunction(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctPOINTER Function,
    IN gctBOOL isKernelFunction
    )
{
    gctUINT codeBase;
    gctSIZE_T codeCount, i;
    gcSHADER shader = Tree->shader;
    gceSTATUS status = gcvSTATUS_OK;

    /* Find function. */
    if (Function == gcvNULL)
    {
        CodeGen->current = &CodeGen->functions[0];
    }
    else
    {
        if (isKernelFunction) {

            for (i = 0; i < shader->kernelFunctionCount; ++i)
            {
                if (Function == shader->kernelFunctions[i])
                {
                    CodeGen->current = &CodeGen->functions[i + 1 + shader->functionCount];
                    break;
                }
            }
            gcmASSERT(i < shader->kernelFunctionCount);

        } else {

            for (i = 0; i < shader->functionCount; ++i)
            {
                if (Function == shader->functions[i])
                {
                    CodeGen->current = &CodeGen->functions[i + 1];
                    break;
                }
            }
            gcmASSERT(i < shader->functionCount);
        }
    }

    /* Check if function has already been generated. */
    if (CodeGen->current->root != gcvNULL)
    {
        return gcvSTATUS_OK;
    }

    /* Reset instruction pointer for current function. */
    CodeGen->current->ip        = 0;
    CodeGen->current->addrRegColoring.countOfMap = 0;

    if (Function == gcvNULL)
    {
        codeBase  = 0;
        codeCount = shader->codeCount;

        /* Allocate register for position. */
        if (CodeGen->usePosition)
        {
            do
            {
                gctINT shift;
                gctUINT8 swizzle;
                gctUINT32 states[4];
                gctINT positionW = -1;

                for (i = 0; i < Tree->attributeCount; ++i)
                {
                    gcATTRIBUTE attribute = Tree->shader->attributes[i];

                    if ((attribute != gcvNULL)
                    &&  (attribute->nameLength == gcSL_POSITION_W)
                    )
                    {
                        positionW = attribute->inputIndex;
                    }
                }

                if (positionW == -1)
                {
                    CodeGen->positionPhysical = 0;
                }
                else
                {
                    /* Allocate one vec4 register. */
                    gcmERR_BREAK(_FindUsage(CodeGen->registerUsage,
                                            CodeGen->registerCount,
                                            gcSHADER_FLOAT_X4,
                                            1,
                                            gcvSL_RESERVED,
                                            gcvFALSE,
                                            (gctINT_PTR)&CodeGen->positionPhysical,
                                            &swizzle,
                                            &shift,
                                            gcvNULL));

                    if (CodeGen->positionPhysical > CodeGen->maxRegister)
                    {
                        CodeGen->maxRegister = CodeGen->positionPhysical;
                    }

                    /* MOV temp, r0 */
                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->positionPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_XYZW) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));

                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XYZW) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    gcmERR_BREAK(_Emit(Tree, CodeGen, states));

                    /* RCP temp.w, attribute(#POSITION_W).x */
                    states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x0C & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->positionPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_W) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

                    states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));

                    states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

                    states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (positionW) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XXXX) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                              | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                    gcmERR_BREAK(_Emit(Tree, CodeGen, states));
                }
            }
            while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                return status;
            }
        }

        /* Allocate register for face. */
        if (CodeGen->useFace)
        {
            do
            {
                gctINT shift;
                gctUINT8 enable;
                gctUINT32 states[4];

                /* Allocate one float register. */
                gcmERR_BREAK(_FindUsage(CodeGen->registerUsage,
                                        CodeGen->registerCount,
                                        gcSHADER_FLOAT_X1,
                                        1,
                                        gcvSL_RESERVED,
                                        gcvFALSE,
                                        (gctINT_PTR)&CodeGen->facePhysical,
                                        &CodeGen->faceSwizzle,
                                        &shift,
                                        &enable));

                if (CodeGen->facePhysical > CodeGen->maxRegister)
                {
                    CodeGen->maxRegister = CodeGen->facePhysical;
                }

                /* SET.NOT temp.enable, face.x */
                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x10 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0A & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->facePhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (enable) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_XXXX) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));

                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x1) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

                gcmERR_BREAK(_Emit(Tree, CodeGen, states));
            }
            while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                return status;
            }
        }

        /* Modify PointCoord. */
        if (CodeGen->usePointCoord)
        {
            do
            {
                gctUINT32 states[4];
                gctINT index;
                gctUINT8 swizzle;

                gcmERR_BREAK(_AddConstantVec1(Tree,
                                              CodeGen,
                                              1.0f,
                                              &index,
                                              &swizzle));

                /* ADD pointCoord.y, 1, -pointCoord.y */
                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1))))))) << (0 ? 15:13)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->pointCoordPhysical) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1))))))) << (0 ? 22:16)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_Y) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));

                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1))))))) << (0 ? 5:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));

                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (CodeGen->pointCoordPhysical) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (gcSL_SWIZZLE_YYYY) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

                gcmERR_BREAK(_Emit(Tree, CodeGen, states));
            }
            while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                return status;
            }
        }
    }
    else
    {
        if (isKernelFunction) {
            codeBase  = ((gcKERNEL_FUNCTION)Function)->codeStart;
            codeCount = ((gcKERNEL_FUNCTION)Function)->codeEnd -
                        ((gcKERNEL_FUNCTION)Function)->codeStart;
        } else {
            codeBase  = ((gcFUNCTION)Function)->codeStart;
            codeCount = ((gcFUNCTION)Function)->codeCount;
        }
    }

    /* Process all instructions. */
    for (i = 0; i < codeCount;)
    {
        /* Extract the instruction. */
        gcSL_INSTRUCTION instruction = &shader->code[codeBase + i];
        gctSIZE_T j;
        gcsSL_PATTERN_PTR pattern;
        gctINT count;
        gceINSTRUCTION_TYPE instructionType;
        gctBOOL bDeferredCleanupUsage = gcvFALSE;

        /* If this is the main function, we have to ignore any instructions
           that live inside a function. */
        if ((Function == gcvNULL) && (Tree->hints[i].owner != Function))
        {
            if (Tree->hints[i].isOwnerKernel) {
                i += ((gcKERNEL_FUNCTION)Tree->hints[i].owner)->codeEnd -
                     ((gcKERNEL_FUNCTION)Tree->hints[i].owner)->codeStart;
            } else {
                i += ((gcFUNCTION)Tree->hints[i].owner)->codeCount;
            }
            continue;
        }

        /* Clean the previous code and previous index if this is an instruction reachable by
        ** jump. */
        if (Tree->hints[i].callers != gcvNULL)
        {
            CodeGen->previousCode = gcvNULL;
            CodeGen->current->addrRegColoring.countOfMap = 0;
        }

        /* Skip NOP instructions. */
        if (instruction->opcode == gcSL_NOP)
        {
            CodeGen->codeMap[codeBase + i].function = CodeGen->current;
            CodeGen->codeMap[codeBase + i].location = CodeGen->current->ip;
            ++i;
            continue;
        }

        /* Find a code pattern. */
        count   = (gctINT) (codeCount - i);
        pattern = _FindPattern(Tree,
                               CodeGen,
                               patterns,
                               instruction,
                               &count);
        if (pattern == gcvNULL)
            return gcvSTATUS_INVALID_ARGUMENT;

        for (j = 0; j < (gctSIZE_T) count; ++j)
        {
            /* Save physical IP for generated instruction. */
            CodeGen->codeMap[codeBase + i + j].function = CodeGen->current;
            CodeGen->codeMap[codeBase + i + j].location = CodeGen->current->ip;
        }

        if (instruction->opcode == gcSL_CALL)
        {
            gcsSL_FUNCTION_CODE_PTR current = CodeGen->current;
            gctBOOL isKernelFunction = gcvFALSE;

            for (j = 0; j < shader->functionCount; ++j)
            {
                if (shader->functions[j]->codeStart == instruction->tempIndex)
                {
                    break;
                }
            }

            if (j == shader->functionCount) {

                for (j = 0; j < shader->kernelFunctionCount; ++j)
                {
                    if (shader->kernelFunctions[j]->codeStart == instruction->tempIndex)
                    {
                        isKernelFunction = gcvTRUE;
                        break;
                    }
                }
                gcmASSERT(j < shader->kernelFunctionCount);
            }

            gcmASSERT(isKernelFunction || j < shader->functionCount);

            current->branch = Tree->branch;
            Tree->branch    = gcvNULL;

            if (isKernelFunction) {
                gcmERR_BREAK(_GenerateFunction(Tree,
                                               CodeGen,
                                               shader->kernelFunctions[j],
                                               gcvTRUE));
            } else {
                gcmERR_BREAK(_GenerateFunction(Tree,
                                               CodeGen,
                                               shader->functions[j],
                                               gcvFALSE));
            }

            CodeGen->current = current;
            Tree->branch     = current->branch;
        }

        /* Transcendental instruction will be splited to several instructions based on enabled component */
        /* count, so if instruction like RCP r0.xy, r0.yx is generated, we can not assign same register */
        /* to dest and source. To do this, we dont clean up its source usage before dst register assigned */
        instructionType = _GetInstructionType(CodeGen, instruction);
        if (instructionType != gcvINSTRUCTION_NORMAL)
        {
            gctUINT8 enableDst = gcmSL_TARGET_GET(instruction->temp, Enable);
            gctUINT16 channelIdx = 1; /* Start from 1 */

            while ((enableDst >> channelIdx) & 0x01)
            {
                gctUINT16 channelIdxPre;
                gcSL_SWIZZLE swizzle;

                if (gcmSL_SOURCE_GET(instruction->source0, Type) == gcSL_TEMP ||
                    gcmSL_SOURCE_GET(instruction->source0, Type) == gcSL_ATTRIBUTE)
                {
                    swizzle = _SelectSwizzle(channelIdx, instruction->source0);
                    for (channelIdxPre = 0; channelIdxPre < channelIdx; channelIdxPre ++)
                    {
                        if (((enableDst >> channelIdxPre) & 0x01) && channelIdxPre == swizzle)
                        {
                            bDeferredCleanupUsage = gcvTRUE;
                            break;
                        }
                    }
                    if (channelIdx == 3 || bDeferredCleanupUsage)
                    {
                        break;
                    }
                }

                if (instructionType == gcvINSTRUCTION_TRANSCENDENTAL_TWO_SOURCES
                &&  (gcmSL_SOURCE_GET(instruction->source1, Type) == gcSL_TEMP ||
                     gcmSL_SOURCE_GET(instruction->source1, Type) == gcSL_ATTRIBUTE))
                {
                    swizzle = _SelectSwizzle(channelIdx, instruction->source1);
                    for (channelIdxPre = 0; channelIdxPre < channelIdx; channelIdxPre ++)
                    {
                        if (((enableDst >> channelIdxPre) & 0x01) && channelIdxPre == swizzle)
                        {
                            bDeferredCleanupUsage = gcvTRUE;
                            break;
                        }
                    }

                    if (channelIdx == 3 || bDeferredCleanupUsage)
                    {
                        break;
                    }
                }

                channelIdx ++;
            }
        }

        /* Save index for next source. */
        CodeGen->nextSource = codeBase + i + count;

        while (pattern->count < 0)
        {
            gctINT shift = 0;
            gctUINT32 states[4];
            gctBOOL skip, emit;
            CodeGen->current->addrRegColoring.localAddrChannelUsageMask = 0;

            if (pattern->count == -1 && !bDeferredCleanupUsage)
            {
                while (count-- > 0)
                {
                    /* Clean up the register usage. */
                    _UpdateUsage(CodeGen->registerUsage,
                                 CodeGen->registerCount,
                                 codeBase + i++);
                }
            }

            /* Zero out the generated instruction. */
            gcmERR_BREAK(gcoOS_ZeroMemory(states, sizeof(states)));

            /* Set opcode. */
            _SetOpcode(states,
                       pattern->opcode,
                       _gc2shCondition(
                           (gcSL_CONDITION)
                           gcmSL_TARGET_GET(instruction->temp, Condition)),
                       gcvFALSE);

            /* Process destination. */
            if (pattern->dest != 0)
            {
                gcmERR_BREAK(_ProcessDestination(Tree,
                                                 CodeGen,
                                                 pattern->dest,
                                                 states,
                                                 &shift,
                                                 pattern));

                switch (pattern->opcode)
                {
                case 0x05:
                    /* fall through */
                case 0x06:

#ifdef AQ_INST_OP_CODE_ATOM_CMP_XCHG
                    /* fall through */
                case AQ_INST_OP_CODE_ATOM_CMP_XCHG:
#endif
                    shift = 0;
                    break;

                default:
                    break;
                }

                skip = (status == gcvSTATUS_SKIP);
            }
            else
            {
                skip = gcvFALSE;
            }

            if (bDeferredCleanupUsage)
            {
                if (pattern->count == -1)
                {
                    while (count-- > 0)
                    {
                        /* Clean up the register usage. */
                        _UpdateUsage(CodeGen->registerUsage,
                                     CodeGen->registerCount,
                                     codeBase + i++);
                    }
                }
            }

            /* Process source 0. */
            if (!skip && (pattern->source0 != 0))
            {
                gcmERR_BREAK(_ProcessSource(Tree,
                                            CodeGen,
                                            pattern->source0,
                                            states,
                                            0,
                                            shift));
            }

            /* Process source 1. */
            if (!skip && (pattern->source1 != 0))
            {
                gcmERR_BREAK(_ProcessSource(Tree,
                                            CodeGen,
                                            pattern->source1,
                                            states,
                                            1,
                                            shift));
            }

            /* Process source 2. */
            if (!skip && (pattern->source2 != 0))
            {
                gcmERR_BREAK(_ProcessSource(Tree,
                                            CodeGen,
                                            pattern->source2,
                                            states,
                                            2,
                                            shift));
            }

            /* Process sampler. */
            if (!skip && (pattern->sampler != 0))
            {
                gcmERR_BREAK(_ProcessSampler(Tree,
                                             CodeGen,
                                             pattern->sampler,
                                             states));
            }

            /* Process user function. */
            if (!skip && (pattern->function != gcvNULL))
            {
                emit = (*pattern->function)(Tree,
                                            CodeGen,
                                            instruction,
                                            states);
            }
            else
            {
                emit = gcvTRUE;
            }

            /* Emit code. */
            if (!skip && emit)
            {
                gcmERR_BREAK(_Emit(Tree, CodeGen, states));

                /* Clean up the temporary register usage. */
                _UpdateUsage(CodeGen->registerUsage,
                             CodeGen->registerCount,
                             gcvSL_TEMPORARY);
            }

            /* Next pattern. */
            ++pattern;
        }

        /* Special optimization for LOAD SW workaround. */
        if (CodeGen->lastLoadUser > 0 && CodeGen->nextSource > (gctUINT) CodeGen->lastLoadUser)
        {
            /* Restore back assigned register for loadDestIndex. */
            Tree->tempArray[CodeGen->loadDestIndex].assigned = CodeGen->origAssigned;
            CodeGen->origAssigned = -1;
            CodeGen->lastLoadUser = -1;
        }

        /* Break on error. */
        gcmERR_BREAK(status);
    }

    if (gcmIS_ERROR(status))
    {
        return status;
    }

    /* End of main function. */
    if (Function == gcvNULL)
    {
        /* Save end of main. */
        CodeGen->endMain = CodeGen->current->ip;

        if ((shader->type == gcSHADER_TYPE_VERTEX)
        &&  (CodeGen->flags & gcvSHADER_USE_GL_Z)
        )
        {
            /*
                The GC family of GPU cores require the Z to be from 0 <= z <= w.
                However, OpenGL specifies the Z to be from -w <= z <= w.  So we
                have to a conversion here:

                    z = (z + w) / 2.

                So here we append two instructions to the vertex shader.
            */
            do
            {
                gctSIZE_T o;
                gctINT index = -1;
                gctUINT8 swizzle;
                gctUINT32 states[4];

                /* Walk all outputs to find the position. */
                for (o = 0; o < Tree->outputCount; ++o)
                {
                    if ((Tree->shader->outputs[o] != gcvNULL)
                    &&  (Tree->shader->outputs[o]->nameLength == gcSL_POSITION)
                    )
                    {
                        /* Save temporary register holding the position. */
                        index = Tree->outputArray[o].tempHolding;
                        break;
                    }
                }

                gcmASSERT(index != -1);

                /* ADD pos.z, pos.z, pos.w */
                gcmERR_BREAK(gcoOS_ZeroMemory(states, gcmSIZEOF(states)));

                _SetOpcode(states,
                           0x01,
                           0x00,
                           gcvFALSE);

                gcmERR_BREAK(_SetDest(Tree,
                                      CodeGen,
                                      states,
                                      index,
                                      0x0,
                                      gcSL_ENABLE_Z,
                                      gcvNULL));

                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        states,
                                        0,
                                        gcSL_TEMP,
                                        index,
                                        0,
                                        0x0,
                                        gcSL_SWIZZLE_ZZZZ,
                                        gcvFALSE,
                                        gcvFALSE));

                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        states,
                                        2,
                                        gcSL_TEMP,
                                        index,
                                        0,
                                        0x0,
                                        gcSL_SWIZZLE_WWWW,
                                        gcvFALSE,
                                        gcvFALSE));

                gcmERR_BREAK(_Emit(Tree, CodeGen, states));

                /* MUL pos.z, pos.z, 0.5 */
                gcmERR_BREAK(gcoOS_ZeroMemory(states, gcmSIZEOF(states)));

                _SetOpcode(states,
                           0x03,
                           0x00,
                           gcvFALSE);

                gcmERR_BREAK(_SetDest(Tree,
                                      CodeGen,
                                      states,
                                      index,
                                      0x0,
                                      gcSL_ENABLE_Z,
                                      gcvNULL));

                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        states,
                                        0,
                                        gcSL_TEMP,
                                        index,
                                        0,
                                        0x0,
                                        gcSL_SWIZZLE_ZZZZ,
                                        gcvFALSE,
                                        gcvFALSE));

                gcmERR_BREAK(_AddConstantVec1(Tree,
                                              CodeGen,
                                              0.5f,
                                              &index,
                                              &swizzle));

                gcmERR_BREAK(_SetSource(Tree,
                                        CodeGen,
                                        states,
                                        1,
                                        gcSL_CONSTANT,
                                        index,
                                        0,
                                        0x0,
                                        swizzle,
                                        gcvFALSE,
                                        gcvFALSE));

                gcmERR_BREAK(_Emit(Tree, CodeGen, states));
            }
            while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                return status;
            }
        }

        /* Save end of program. */
        CodeGen->endPC = gcmMAX(CodeGen->current->ip, 1);

        if ((Tree->shader->functionCount +  Tree->shader->kernelFunctionCount > 0)
        ||  (CodeGen->current->ip == 0)
        )
        {
            do
            {
                gctUINT32 states[4];

                /* Append a NOP. */
                states[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));
                states[1] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
                states[2] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
                states[3] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));

                gcmERR_BREAK(_Emit(Tree, CodeGen, states));
            }
            while (gcvFALSE);

            if (gcmIS_ERROR(status))
            {
                return status;
            }
        }
    }

    CodeGen->current->branch = Tree->branch;
    Tree->branch             = gcvNULL;

    return gcvSTATUS_OK;
}

static gceSTATUS
_GenerateCode(
    IN gcLINKTREE Tree,
    IN OUT gcsCODE_GENERATOR_PTR CodeGen
    )
{
    gceSTATUS status;

    /* Reset register count. */
    CodeGen->maxRegister = 0;

    do
    {
        gctSIZE_T i;
        gctUINT base, max;
        gctUINT vsInstMax, psInstMax;
        gcSHADER shader = Tree->shader;

        /* Determine the maximum number of instructions. */
        gcmERR_BREAK(
            gcoHARDWARE_QueryShaderCaps(gcvNULL,
                                        gcvNULL,
                                        gcvNULL,
                                        gcvNULL,
                                        gcvNULL,
                                        &vsInstMax,
                                        &psInstMax));

        /* Generate the main function. */
        gcmERR_BREAK(_GenerateFunction(Tree, CodeGen, gcvNULL, gcvFALSE));

        /* Walk all functions. */
        for (i = 0, base = 0; i <= shader->functionCount + shader->kernelFunctionCount; ++i)
        {
            CodeGen->functions[i].ipBase = base;
            base += CodeGen->functions[i].ip;
        }

        switch (Tree->shader->type)
        {
        case gcSHADER_TYPE_VERTEX:
            max = vsInstMax;
            break;

        case gcSHADER_TYPE_FRAGMENT:
            max = psInstMax;
            break;

        default:
            max = base;
            break;
        }

        if (base > max)
        {
            gcmFATAL("Shader has too many instructions: %d (maximum is %d)",
                     base,
                     max);
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        /* Walk all functions. */
        for (i = 0; i <= shader->functionCount + shader->kernelFunctionCount; ++i)
        {
            gcsSL_FUNCTION_CODE_PTR function = &CodeGen->functions[i];

            /* Walk all branches. */
            while (function->branch != gcvNULL)
            {
                gctUINT target;

                gcSL_BRANCH_LIST branch = function->branch;
                function->branch        = branch->next;

                if (branch->target >= shader->codeCount)
                {
                    target = CodeGen->endMain;
                }
                else
                if (!branch->call
                &&  (function != CodeGen->codeMap[branch->target].function)
                )
                {
                    gcmASSERT(function->ipBase == 0);
                    target = CodeGen->endMain;
                }
                else
                {
                    target = CodeGen->codeMap[branch->target].function->ipBase
                           + CodeGen->codeMap[branch->target].location;
                }

                /* Update the TARGET field. */
                gcmERR_BREAK(_SetTarget(Tree,
                                        CodeGen,
                                        function,
                                        branch->ip,
                                        target));

                /* Free the branch structure. */
                gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, branch));
            }

            if (gcmIS_ERROR(status))
            {
                break;
            }
        }
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

static gceSTATUS
_SetState(
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gctUINT32 maxLoadStateCount = gcmALIGN((((((gctUINT32) (~0)) >> (0 ? 25:16)) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1)))))) ), 4)-4;

    /* Check if this address is sequential to the last loaded state. */
    if ((Address == CodeGen->lastStateAddress + CodeGen->lastStateCount)
    &&  (CodeGen->lastStateCount < maxLoadStateCount)
    )
    {
        /* Make sure we have enough room in the state buffer. */
        if (CodeGen->stateBufferOffset + 4 > CodeGen->stateBufferSize)
        {
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* Increment state count of last LoadState command. */
        ++ CodeGen->lastStateCount;

        if (CodeGen->lastStateCommand != gcvNULL) {

            /* Physical LoadState command. */
            *CodeGen->lastStateCommand = ((((gctUINT32) (*CodeGen->lastStateCommand)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->lastStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
        }
    }
    else
    {
        /* Align last load state to 64-bit. */
        CodeGen->stateBufferOffset = gcmALIGN(CodeGen->stateBufferOffset, 8);

        /* Make sure we have enough room in the state buffer. */
        if (CodeGen->stateBufferOffset + 8 > CodeGen->stateBufferSize)
        {
            return gcvSTATUS_BUFFER_TOO_SMALL;
        }

        /* New LoadState command. */
        CodeGen->lastStateAddress = Address;
        CodeGen->lastStateCount   = 1;

        if (CodeGen->stateBuffer != gcvNULL)
        {
            /* Save address of LoadState command. */
            CodeGen->lastStateCommand = (gctUINT32 *) CodeGen->stateBuffer +
                                        CodeGen->stateBufferOffset / 4;

            /* Physical LoadState command. */
            *CodeGen->lastStateCommand =
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) |
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) |
                ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));
        }

        /* Increment state buffer offset. */
        CodeGen->stateBufferOffset += 4;
    }

    if (CodeGen->stateBuffer != gcvNULL)
    {
        /* Physical data. */
        ((gctUINT32 *) CodeGen->stateBuffer)[CodeGen->stateBufferOffset / 4] =
            Data;
    }

    /* Increment state buffer offset. */
    CodeGen->stateBufferOffset += 4;

    /* Success. */
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenerateStates(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctPOINTER StateBuffer,
    IN OUT gctSIZE_T * Size,
    OUT gcsHINT_PTR Hints
    )
{
    gceSTATUS status;
    gctSIZE_T i, attributeCount, outputCount, f;
    gcsSL_CONSTANT_TABLE_PTR c;
    gctUINT32 uniformAddress, codeAddress, instrBase;
    gcsSL_PHYSICAL_CODE_PTR code;
    gctUINT32 timeout;
    gctUINT32 maxNumInstrStates;
    gctUINT32 numInstrStates = 0;
    gcoHARDWARE hardware;
    gctINT components = 0, rows = 0;

    gcmGETHARDWARE(hardware);

    /* Initialize state buffer management. */
    CodeGen->stateBuffer       = StateBuffer;
    CodeGen->stateBufferSize   = (StateBuffer == gcvNULL) ? ~0U : *Size;
    CodeGen->stateBufferOffset = 0;
    CodeGen->lastStateCommand  = gcvNULL;

	if (CodeGen->clShader && !CodeGen->hasBugFixes10) {
        /* Bump up number of registers to artificially restrict
        ** number of shader groups */
        if (CodeGen->isGC4000) {
            CodeGen->maxRegister = gcmMAX(3, CodeGen->maxRegister);
        } else {
            CodeGen->maxRegister = gcmMAX(9, CodeGen->maxRegister);
        }
    }

    /* Count number of active attributes. */
    for (i = attributeCount = 0; i < Tree->attributeCount; ++i)
    {
        if (Tree->attributeArray[i].inUse
        &&  (Tree->shader->attributes[i]->nameLength != gcSL_POSITION)
        &&  (Tree->shader->attributes[i]->nameLength != gcSL_FRONT_FACING)
        )
        {
            components = rows = 0;
            _ConvertType(Tree->shader->attributes[i]->type,
                         Tree->shader->attributes[i]->arraySize,
                         &components,
                         &rows);
            attributeCount += rows;
        }
    }

    /* Count number of active outputs. */
    for (i = outputCount = 0; i < Tree->outputCount; ++i)
    {
        if (Tree->outputArray[i].inUse
             ||
             ((Tree->shader->outputs[i] != gcvNULL)
               &&
               ((Tree->shader->outputs[i]->nameLength == gcSL_POSITION)
                 ||
                 (Tree->shader->outputs[i]->nameLength == gcSL_POINT_SIZE)
                 ||
                 (Tree->shader->outputs[i]->nameLength == gcSL_COLOR)
                 ||
                 (Tree->shader->outputs[i]->nameLength == gcSL_DEPTH)
               )
             )
           )
        {
            components = rows = 0;
            _ConvertType(Tree->shader->outputs[i]->type,
                         1,
                         &components,
                         &rows);
            outputCount += rows;
        }
    }

    if (Tree->shader->type == gcSHADER_TYPE_VERTEX)
    {
        /* Vertex shader. */
        gctUINT32 output[8];
        gctSIZE_T evenOutputCount;
        gctBOOL hasPointSize = gcvFALSE;

        /* Zero the outputs. */
        gcmONERROR(gcoOS_ZeroMemory(output, sizeof(output)));

        /* Walk through all outputs. */
        for (i = 0; i < Tree->outputCount; ++i)
        {
            gctINT link, index, shift, temp, reg;
            gctBOOL inUse;

            if (Tree->shader->outputs[i] == gcvNULL)
            {
                continue;
            }

            /* Extract internal name for output. */
            switch (Tree->shader->outputs[i]->nameLength)
            {
            case gcSL_POSITION:
                /* Position is always at r0. */
                link  = 0;
                inUse = gcvTRUE;
                break;

            case gcSL_POINT_SIZE:
                /* Point size is always at the end. */
                link         = outputCount - 1;
                inUse        = gcvTRUE;
                hasPointSize = gcvTRUE;
                break;

            default:
                /* Determine linked fragment attribute register. */
                link  = Tree->outputArray[i].fragmentIndex + 1;
                inUse = Tree->outputArray[i].inUse;
                break;
            }

            /* Only process enabled outputs. */
            if (inUse)
            {
                gctINT endLink, thisLink, thisTemp;
                components = rows = 0;
                _ConvertType(Tree->shader->outputs[i]->type,
                             1,
                             &components,
                             &rows);
                gcmASSERT(rows ==
                          (Tree->outputArray[i].fragmentIndexEnd - Tree->outputArray[i].fragmentIndex + 1));

                temp = Tree->outputArray[i].tempHolding;
                endLink = link + rows;

                for (thisLink = link, thisTemp = temp; thisLink < endLink; thisLink ++, thisTemp ++)
                {
                    /* Convert link register into output array index. */
                    index = thisLink >> 2;
                    shift = (thisLink & 3) * 8;

                    /* Copy register into output array. */
                    reg  = Tree->tempArray[thisTemp].assigned;
                    if (Tree->tempArray[thisTemp].defined)
                    {
                        gcmASSERT(reg != -1);

                        output[index] |= reg << shift;
                        if ((gctUINT) reg > CodeGen->maxRegister)
                        {
                            CodeGen->maxRegister = reg;
                        }
                    }
                }
            }
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (StateBuffer != gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "VS: EndPC=%u",
                          CodeGen->endPC);
        }
#endif

        /* Hints */
        if (Hints != gcvNULL)
        {
            Hints->vsOutputCount  = outputCount;
            Hints->vsHasPointSize = hasPointSize;

			if (CodeGen->clShader == gcvTRUE)
			{
                /* Kernel shader does not have extra attribute as fragment shader. */
                /* However, fsInputCount cannot be 0. */
                if (attributeCount == 0) attributeCount = 1;
                Hints->fsInputCount   = attributeCount;
                Hints->fsMaxTemp      = CodeGen->maxRegister + 1;
                Hints->elementCount   = 0;

			}
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (StateBuffer != gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "VS: outputCount=%u %s",
                          outputCount,
                          hasPointSize ? "(pointSize)" : "");
        }
#endif

        /* AQVertexShaderInputControl */
        timeout = gcmALIGN(attributeCount * 4 + 4, 16) / 16;
        gcmONERROR(_SetState(CodeGen,
                             0x0202,
                             ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (attributeCount) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) |
                             ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (timeout) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))));
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (StateBuffer != gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "VS: attributeCount=%u",
                          attributeCount);
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "VS: timeout=%u",
                          timeout);
        }
#endif

        /* AQVertexShaderTemporaryRegisterControl */
        gcmONERROR(
            _SetState(CodeGen,
                      0x0203,
                      CodeGen->maxRegister + 1));

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (StateBuffer != gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "VS: numRegisters=%u",
                          CodeGen->maxRegister + 1);
        }
#endif

        /* 0x0204 */
        for (i = 0; i < (outputCount + 3) / 4; ++i)
        {
            gcmONERROR(_SetState(CodeGen,
                                 0x0204 + i,
                                 output[i]));
        }

        /* Load balancing min and max. */
        if (Hints != gcvNULL)
        {
            if (outputCount > 0)
            {
                evenOutputCount   = gcmALIGN(outputCount, 2);
                Hints->balanceMin = ((256 * 10
                                      * 8 /* Pipe line depth. */
                                      / (hardware->vertexOutputBufferSize - evenOutputCount * hardware->vertexCacheSize )
                                      ) + 9
                                    ) / 10;
                Hints->balanceMax = gcmMIN(255, 256 / (hardware->shaderCoreCount * (evenOutputCount >> 1)));
            }
            else
            {
                Hints->balanceMin = 0;
                Hints->balanceMax = 0;
            }
        }

        if(Hints != gcvNULL)
        {
            Hints->vsMaxTemp      = CodeGen->maxRegister + 1;
            Hints->threadWalkerInPS = gcvFALSE;
        }

		if (CodeGen->clShader == gcvTRUE)
        {
            gctUINT32 varyingPacking[2] = {0, 0};

            if (Hints != gcvNULL) {

                switch (Hints->elementCount)
                {
                case 3:
                    varyingPacking[1] = 2;
                    /*fall through*/
                case 2:
                    varyingPacking[0] = 2;
                }

                Hints->componentCount = gcmALIGN(varyingPacking[0] + varyingPacking[1], 2);
            }

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E08,
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (varyingPacking[0]) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (varyingPacking[1]) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))));

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E0D,
                          0));
        }

        if (hardware->instructionCount > 1024)
        {
            gcmONERROR(
                _SetState(CodeGen,
                          0x0217,
                            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->endPC-1 ) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                          ));
            gcmONERROR(
                _SetState(CodeGen,
                          0x0218,
                            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (0x0) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                          ));

            /* Set uniform and code address. */
            uniformAddress = 0x1400;
            instrBase      = 0x8000;
            maxNumInstrStates    = hardware->instructionCount << (CodeGen->clShader ? 2 : 1);

        } else if (hardware->instructionCount > 256) {
            gcmONERROR(_SetState(CodeGen,
                                 0x0217,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                                 | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (CodeGen->endPC-1 ) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                                 ));

            /* Workaround for hang issue on gc2100. */
            /* TODO - the problem may be in context switching. */

            /* AQVertexShaderStartPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x020E,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) |
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                                 ));

            /* AQVertexShaderEndPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0200,
                                 CodeGen->endPC
                                 ));

            /* Set uniform and code address. */
            uniformAddress = 0x1400;
            instrBase      = 0x3000;
            maxNumInstrStates    = hardware->instructionCount << (CodeGen->clShader ? 2 : 1);

        } else {

            /* AQVertexShaderStartPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x020E,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) |
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                                 ));

            /* AQVertexShaderEndPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0200,
                                 CodeGen->endPC
                                 ));

            /* Set uniform and code address. */
            uniformAddress = 0x1400;
            instrBase      = 0x1000;
            maxNumInstrStates    = 1024;
        }
    }
    else
    {
        /* Fragment shader. */
        gctUINT32 address;
        gctBOOL halfAttribute = gcvFALSE;
        gctBOOL hasFragDepth = gcvFALSE;
        gctINT index;

        /* Find the last used attribute. */
        for (index = Tree->attributeCount - 1; index >= 0; --index)
        {
            if (Tree->attributeArray[index].inUse)
            {
                /* Test type of attribute. */
                switch (Tree->shader->attributes[index]->type)
                {
                case gcSHADER_FLOAT_X1:
                case gcSHADER_FLOAT_X2:
                case gcSHADER_FLOAT_2X2:
                    /* Half attribute can be enabled for FLOAT_X1 and
                       FLOAT_X2 types. */
                    halfAttribute = gcvTRUE;
                    break;

                default:
                    break;
                }

                /* Bail out loop. */
                break;
            }
        }

        /* AQRasterControl */
        gcmONERROR(_SetState(CodeGen,
                             0x0380,
                             ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) |
                             ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) ((gctUINT32) (halfAttribute) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))));

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (StateBuffer != gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                          "FS: EndPC=%u",
                          CodeGen->endPC);
        }
#endif

        /* Walk through all outputs. */
        for (i = 0; i < Tree->outputCount; ++i)
        {
            gctINT temp, reg;

            if (Tree->shader->outputs[i] == gcvNULL)
            {
                continue;
            }

            /* Extract internal name for output. */
            switch (Tree->shader->outputs[i]->nameLength)
            {
            case gcSL_COLOR:
                break;
            case gcSL_DEPTH:
                hasFragDepth = gcvTRUE;
                break;

            default:
               continue;
            }

            temp = Tree->outputArray[i].tempHolding;
            reg  = Tree->tempArray[temp].assigned;

            if (reg == -1)
            {
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                if (StateBuffer != gcvNULL)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                                  "Fragment Shader has no output, using r0.");
                }
#endif
                reg = 0;
            }

            gcmASSERT(reg != -1);
            if (Tree->shader->outputs[i]->nameLength == gcSL_COLOR)
            {
                gcmONERROR(_SetState(CodeGen,
                                     0x0401,
                                     reg));
            }

            if ((gctUINT) reg > CodeGen->maxRegister)
            {
                CodeGen->maxRegister = reg;
            }
        }

        if (Hints != gcvNULL)
        {
            gcmASSERT(attributeCount <= 8);
            Hints->elementCount   = attributeCount;

            Hints->psHasFragDepthOut = hasFragDepth;

            if (Tree->shader->type == gcSHADER_TYPE_FRAGMENT)
            {
                Hints->fsInputCount   = 1 + attributeCount;
                Hints->fsMaxTemp      = 1 + CodeGen->maxRegister;
            }
            else
            {
                /* Kernel shader does not have extra attribute as fragment shader. */
                /* However, fsInputCount cannot be 0. */
                Hints->fsInputCount   = attributeCount ? attributeCount : 1;
                Hints->fsMaxTemp      = CodeGen->maxRegister + 1;
                Hints->threadWalkerInPS = gcvTRUE;
            }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            if (StateBuffer != gcvNULL)
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                              "FS: elementCount=%u",
                              Hints->elementCount);
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                              "FS: fsInputCount=%u",
                              Hints->fsInputCount);
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                              "FS: fsMaxTemp=%u",
                              Hints->fsMaxTemp);
            }
#endif
        }

        /* Generate element type. */
        address = 0x0290;
        for (i = 0; i < Tree->attributeCount; ++i)
        {
            /* Only process enabled attributes that are not predefined. */
            if (Tree->attributeArray[i].inUse
            &&  (Tree->shader->attributes[i]->nameLength > 0)
            )
            {
                /* Determine texture usage. */
                gctUINT32 texture = Tree->shader->attributes[i]->isTexture
                    ? 0xF
                    : 0x0;

                /* AQPAClipFlatColorTex */
                gcmONERROR(
                    _SetState(CodeGen,
                              address++,
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (texture) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (texture) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) |
                              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (0x2) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)))));
            }
        }

        if (Tree->shader->type != gcSHADER_TYPE_CL)
        {
            gctUINT32 componentIndex = 0;
            gctUINT32 componentType[8 * 4];
            gctUINT32 varyingIndex = 0;
            gctUINT32 varyingPacking[8];

            /* Generate the varying packing. */
            for (i = 0; i < Tree->attributeCount; ++i)
            {
                gcSHADER_TYPE type;
                gctBOOL isTexture;
                gctSIZE_T size;

                if (!Tree->attributeArray[i].inUse
                ||  (Tree->shader->attributes[i]->nameLength == gcSL_POSITION)
                ||  (Tree->shader->attributes[i]->nameLength == gcSL_FRONT_FACING)
                )
                {
                    continue;
                }

                if (hardware->chipModel < gcv1000)
                {
                    type = gcSHADER_FLOAT_X4;
                }
                else
                {
                    type      = Tree->shader->attributes[i]->type;
                }

                isTexture = Tree->shader->attributes[i]->isTexture;

                components = rows = 0;
                _ConvertType(Tree->shader->attributes[i]->type,
                             Tree->shader->attributes[i]->arraySize,
                             &components,
                             &rows);
                size      = gcmMAX(1, rows);

                while (size-- > 0)
                {
                    if(varyingIndex >= 8)
                    {
                        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                    }

                    switch (type)
                    {
                    case gcSHADER_FLOAT_X1:
                        /* fall through */
                    case gcSHADER_BOOLEAN_X1:
                        /* fall through */
                    case gcSHADER_INTEGER_X1:
                        gcmASSERT(varyingIndex < 8);
                        varyingPacking[varyingIndex++] = 1;

                        gcmASSERT(componentIndex < 32);
                        componentType[componentIndex++] = isTexture
                            ? 0x2
                            : 0x1;
                        break;

                    case gcSHADER_FLOAT_X2:
                        /* fall through */
                    case gcSHADER_BOOLEAN_X2:
                        /* fall through */
                    case gcSHADER_INTEGER_X2:
                        /* fall through */
                    case gcSHADER_FLOAT_2X2:
                        gcmASSERT(varyingIndex < 8);
                        varyingPacking[varyingIndex++] = 2;

                        gcmASSERT(varyingIndex < 31);
                        componentType[componentIndex++] = isTexture
                            ? 0x2
                            : 0x1;
                        componentType[componentIndex++] = isTexture
                            ? 0x3
                            : 0x1;
                        break;

                    case gcSHADER_FLOAT_X3:
                        /* fall through */
                    case gcSHADER_BOOLEAN_X3:
                        /* fall through */
                    case gcSHADER_INTEGER_X3:
                        /* fall through */
                    case gcSHADER_FLOAT_3X3:
                        gcmASSERT(varyingIndex < 8);
                        varyingPacking[varyingIndex++] = 3;

                        gcmASSERT(varyingIndex < 30);
                        componentType[componentIndex++] = isTexture
                            ? 0x2
                            : 0x1;
                        componentType[componentIndex++] = isTexture
                            ? 0x3
                            : 0x1;
                        componentType[componentIndex++] =
                            0x1;
                        break;

                    case gcSHADER_FLOAT_X4:
                        /* fall through */
                    case gcSHADER_BOOLEAN_X4:
                        /* fall through */
                    case gcSHADER_INTEGER_X4:
                        /* fall through */
                    case gcSHADER_FLOAT_4X4:
                        gcmASSERT(varyingIndex < 8);
                        varyingPacking[varyingIndex++] = 4;

                        gcmASSERT(varyingIndex < 29);
                        componentType[componentIndex++] = isTexture
                            ? 0x2
                            : 0x1;
                        componentType[componentIndex++] = isTexture
                            ? 0x3
                            : 0x1;
                        componentType[componentIndex++] =
                            0x1;
                        componentType[componentIndex++] =
                            0x1;
                        break;

                    default:
                        gcmFATAL("Huh? Some kind of weird attribute (%d) here?",
                                 Tree->shader->attributes[i]->type);
                        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                    }
                }
            }

            gcmASSERT(varyingIndex == attributeCount);

            while (varyingIndex < 8)
            {
                varyingPacking[varyingIndex++] = 0;
            }

            if (Hints != gcvNULL)
            {
                Hints->componentCount = gcmALIGN(componentIndex, 2);

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                if (StateBuffer != gcvNULL)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                                  "FS: componentCount=%u",
                                  Hints->componentCount);
                }
#endif
            }

            while (componentIndex < 8 * 4)
            {
                componentType[componentIndex++] =
                    0x0;
            }

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E08,
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (varyingPacking[0]) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (varyingPacking[1]) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (varyingPacking[2]) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) (varyingPacking[3]) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) (varyingPacking[4]) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20))) | (((gctUINT32) ((gctUINT32) (varyingPacking[5]) & ((gctUINT32) ((((1 ? 22:20) - (0 ? 22:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:20) - (0 ? 22:20) + 1))))))) << (0 ? 22:20))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) | (((gctUINT32) ((gctUINT32) (varyingPacking[6]) & ((gctUINT32) ((((1 ? 26:24) - (0 ? 26:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:24) - (0 ? 26:24) + 1))))))) << (0 ? 26:24))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) (varyingPacking[7]) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)))));

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            if (StateBuffer != gcvNULL)
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
                              "FS: packing=%u,%u,%u,%u,%u,%u,%u,%u",
                              varyingPacking[0],
                              varyingPacking[1],
                              varyingPacking[2],
                              varyingPacking[3],
                              varyingPacking[4],
                              varyingPacking[5],
                              varyingPacking[6],
                              varyingPacking[7]);
            }
#endif

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E0A,
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (componentType[0]) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) ((gctUINT32) (componentType[1]) & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (componentType[2]) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) ((gctUINT32) (componentType[3]) & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (componentType[4]) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) | (((gctUINT32) ((gctUINT32) (componentType[5]) & ((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (componentType[6]) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (componentType[7]) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (componentType[8]) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) | (((gctUINT32) ((gctUINT32) (componentType[9]) & ((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (componentType[10]) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (componentType[11]) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) ((gctUINT32) (componentType[12]) & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) | (((gctUINT32) ((gctUINT32) (componentType[13]) & ((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) | (((gctUINT32) ((gctUINT32) (componentType[14]) & ((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (componentType[15]) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)))));

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E0B,
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (componentType[16]) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) | (((gctUINT32) ((gctUINT32) (componentType[17]) & ((gctUINT32) ((((1 ? 3:2) - (0 ? 3:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:2) - (0 ? 3:2) + 1))))))) << (0 ? 3:2))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (componentType[18]) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) | (((gctUINT32) ((gctUINT32) (componentType[19]) & ((gctUINT32) ((((1 ? 7:6) - (0 ? 7:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:6) - (0 ? 7:6) + 1))))))) << (0 ? 7:6))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (componentType[20]) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) | (((gctUINT32) ((gctUINT32) (componentType[21]) & ((gctUINT32) ((((1 ? 11:10) - (0 ? 11:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:10) - (0 ? 11:10) + 1))))))) << (0 ? 11:10))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (componentType[22]) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (componentType[23]) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (componentType[24]) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) | (((gctUINT32) ((gctUINT32) (componentType[25]) & ((gctUINT32) ((((1 ? 19:18) - (0 ? 19:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:18) - (0 ? 19:18) + 1))))))) << (0 ? 19:18))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (componentType[26]) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) | (((gctUINT32) ((gctUINT32) (componentType[27]) & ((gctUINT32) ((((1 ? 23:22) - (0 ? 23:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:22) - (0 ? 23:22) + 1))))))) << (0 ? 23:22))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) ((gctUINT32) (componentType[28]) & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) | (((gctUINT32) ((gctUINT32) (componentType[29]) & ((gctUINT32) ((((1 ? 27:26) - (0 ? 27:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:26) - (0 ? 27:26) + 1))))))) << (0 ? 27:26))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) | (((gctUINT32) ((gctUINT32) (componentType[30]) & ((gctUINT32) ((((1 ? 29:28) - (0 ? 29:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:28) - (0 ? 29:28) + 1))))))) << (0 ? 29:28))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (componentType[31]) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)))));
        }
        else if (hardware->threadWalkerInPS)
        {
            gctUINT32 varyingPacking[2] = {0, 0};

            if (Hints != gcvNULL) {

                switch (Hints->elementCount)
                {
                case 3:
                    varyingPacking[1] = 2;
                    /*fall through*/
                case 2:
                    varyingPacking[0] = 2;
                }

                Hints->componentCount = gcmALIGN(varyingPacking[0] + varyingPacking[1], 2);
            }

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E08,
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (varyingPacking[0]) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) |
                          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (varyingPacking[1]) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))));

            gcmONERROR(
                _SetState(CodeGen,
                          0x0E0D,
                          0));
        }

        if (hardware->instructionCount > 1024)
        {
            gctUINT psOffset = 0;

            if (Tree->shader->type != gcSHADER_TYPE_CL)
            {
                /* Use half of instruction memory for PS. */
                psOffset = hardware->instructionCount / 2;
            }

            gcmONERROR(
                _SetState(CodeGen,
                          0x0407,
                            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (psOffset) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (psOffset + CodeGen->endPC-1) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                          ));
            gcmONERROR(
                _SetState(CodeGen,
                          0x0218,
                            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) ((gctUINT32) (0x1) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
                          | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (0x1) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                          ));

            /* Set uniform and code address. */
            uniformAddress = 0x1C00;
            instrBase      = 0x8000 + (psOffset<<2);
            maxNumInstrStates    = hardware->instructionCount << (CodeGen->clShader ? 2 : 1);

        } else if (hardware->instructionCount > 256){

            gctUINT psOffset = 0;

            if (Tree->shader->type != gcSHADER_TYPE_CL)
            {
                /* Use half of instruction memory for PS. */
                psOffset = hardware->instructionCount / 2;
            }

            gcmONERROR(_SetState(CodeGen,
                                 0x0407,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (psOffset) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                                 | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (psOffset + CodeGen->endPC - 1) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))));

            /* Workaround for hang issue on gc2100. */
            /* TODO - the problem may be in context switching. */
            /* AQPixelShaderStartPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0406,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) |
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))));

            /* AQPixelShaderEndPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0400,
                                 CodeGen->endPC));

            /* Set uniform and code address. */
            uniformAddress = 0x1C00;
            instrBase      = 0x3000 + (psOffset << 2);
            maxNumInstrStates    = hardware->instructionCount << (CodeGen->clShader ? 2 : 1);

        } else {

            /* AQPixelShaderStartPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0406,
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) |
                                 ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))));

            /* AQPixelShaderEndPC */
            gcmONERROR(_SetState(CodeGen,
                                 0x0400,
                                 CodeGen->endPC));

            /* Set uniform and code address. */
            uniformAddress = 0x1C00;
            instrBase      = 0x1800;
            maxNumInstrStates    = 1024;
        }
    }

    /* Process all constants. */
    for (c = CodeGen->constants; c != gcvNULL; c = c->next)
    {
        /* Determine offset of uniform. */
        gctINT u;

        for (u = 0; u < c->count; ++u)
        {
            /* Program uniform constant. */
            gctUINT32 index = c->index * 4 + _ExtractSwizzle(c->swizzle, u);

            gcmONERROR(_SetState(CodeGen,
                                 uniformAddress + index,
                                 *(gctUINT32_PTR) &c->constant[u]));
        }
    }

    /* Process all code. */
    for (f = 0, codeAddress = instrBase; f <= Tree->shader->functionCount + Tree->shader->kernelFunctionCount; ++f)
    {
        for (code = CodeGen->functions[f].root;
             code != gcvNULL;
             code = code->next)
        {
            /* Process all states. */
            for (i = 0; i < code->count * 4; ++i)
            {
                /* Program instruction. */
                gcmONERROR(_SetState(CodeGen,
                                     codeAddress++,
                                     code->states[i]));
                numInstrStates ++;
            }
        }
    }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    /* Debug shader. */
    if (CodeGen->lastStateCommand != gcvNULL)
    {
        _DumpUniforms(StateBuffer, CodeGen->lastStateCommand);
        _DumpShader(StateBuffer, CodeGen->stateBufferOffset, CodeGen->clShader, instrBase, maxNumInstrStates);
    }
#endif

    if ((CodeGen->lastStateCommand != gcvNULL) && (numInstrStates > maxNumInstrStates)) {
        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
            "Not enough instruction memory, need %d, have %d", numInstrStates, maxNumInstrStates);
        gcmONERROR(gcvSTATUS_OUT_OF_RESOURCES);
    }

OnError:

    /* Return the required size. */
    *Size = gcmALIGN(CodeGen->stateBufferOffset, 8);

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcLINKTREE_GenerateStates
**
**  Generate hardware states from the shader.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE object.
**
**      gceSHADER_FLAGS Flags
**          Linker flags.
**
**      gctSIZE_T * StateBufferSize
**          Size of state buffer on entry.
**
**      gctPOINTER * StateBuffer
**          Pointer to state buffer.
**
**  OUTPUT:
**
**      gctSIZE_T * StateBufferSize
**          Pointer to a variable receiving the number of bytes ni the state
**          buffer.
**
**      gctPOINTER * StateBuffer
**          Pointer to a variable receiving the state buffer pointer.
**
**      gcsHINT_PTR * Hints
**          Pointer to a variable receiving a gcsHINT structure pointer that
**          contains information required when loading the shader states.
*/
gceSTATUS
gcLINKTREE_GenerateStates(
    IN gcLINKTREE Tree,
    IN gceSHADER_FLAGS Flags,
    IN OUT gctSIZE_T * StateBufferSize,
    IN OUT gctPOINTER * StateBuffer,
    OUT gcsHINT_PTR * Hints
    )
{
    gceSTATUS status;
    gctSIZE_T size;
    gctUINT8 * stateBuffer = gcvNULL;
    gcoHARDWARE hardware;
    gctPOINTER pointer = gcvNULL;
    gctUINT32 vsUniformCount, psUniformCount;

    /* Extract the gcSHADER shader object. */
    gcSHADER shader = Tree->shader;

    /* The common code generator structure. */
    gcsCODE_GENERATOR codeGen = { (gceSHADER_FLAGS) 0 };

    gcmHEADER_ARG("Tree=0x%x Flags=%d StateBufferSize=0x%x "
                    "StateBuffer=0x%x Hints=0x%x",
                    Tree, Flags, StateBufferSize,
                    StateBuffer, Hints);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(StateBufferSize != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(StateBuffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Hints != gcvNULL);

    /* Determine if OpenCL shader. */
    codeGen.clShader = (shader->type == gcSHADER_TYPE_CL);

    /* Cache hardware flags. */
    codeGen.isGC2100 = (hardware->chipModel == gcv2100 ||
                       (hardware->chipModel == gcv2000 && hardware->chipRevision == 0x5108));
    codeGen.isGC4000 = (hardware->chipModel >= gcv4000);
    codeGen.hasCL    = (hardware->chipModel >  gcv2000 || codeGen.isGC2100);

    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_BUG_FIXES10))
    {
        codeGen.hasBugFixes10 = gcvTRUE;
    }
    else
    {
        codeGen.hasBugFixes10 = gcvFALSE;

        /* Special workaround for old gc2100 with 512 instruction limit. */
        if (codeGen.clShader && hardware->chipModel == gcv2000 && hardware->chipRevision == 0x5108)
        {
            if (Tree->shader->codeCount > 422 && Tree->shader->codeCount < 434 &&
                Tree->shader->attributeCount == 1 && Tree->shader->uniformCount == 2)
            {
                /* No STORE1. */
                gctUINT i;
                gcSL_INSTRUCTION codes = Tree->shader->code;
#define __NUM_INST__    26
                gctUINT16 opcodes[__NUM_INST__] = {
                    gcSL_MOV, gcSL_MOV, gcSL_JMP, gcSL_JMP,
                    gcSL_MOV, gcSL_MOV, gcSL_MOV, gcSL_MOV, gcSL_LOAD,
                    gcSL_STORE, gcSL_STORE, gcSL_STORE, gcSL_STORE,
                    gcSL_MOV, gcSL_LOAD,
                    gcSL_STORE, gcSL_STORE, gcSL_STORE, gcSL_STORE,
                    gcSL_MOV, gcSL_LOAD, gcSL_LOAD,
                    gcSL_STORE, gcSL_STORE, gcSL_STORE, gcSL_STORE,
                };

                for (i = 0; i < __NUM_INST__; i++)
                {
                    if (codes[i].opcode != opcodes[i]) break;
                }
                if (i == __NUM_INST__)
                {
                    codeGen.hasBugFixes10 = gcvTRUE;
                }
            }
            else if (Tree->shader->codeCount > 545 && Tree->shader->codeCount < 561 &&
                Tree->shader->attributeCount == 1 && Tree->shader->uniformCount == 2)
            {
                /* No STORE1. */
                gctUINT i;
                gcSL_INSTRUCTION codes = Tree->shader->code;
#define __NUM_INST1__    37
                gctUINT16 opcodes[__NUM_INST1__] = {
                    gcSL_MOV, gcSL_MOV, gcSL_JMP, gcSL_JMP,
                    gcSL_MOV, gcSL_MOV, gcSL_MOV, gcSL_MOV, gcSL_LOAD,
                    gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1,
                    gcSL_MOV, gcSL_LOAD,
                    gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1,
                    gcSL_MOV, gcSL_LOAD, gcSL_LOAD,
                    gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1, gcSL_ADD, gcSL_STORE1,
                };

                for (i = 0; i < __NUM_INST1__; i++)
                {
                    if (codes[i].opcode != opcodes[i]) break;
                }
                if (i == __NUM_INST1__)
                {
                    codeGen.hasBugFixes10 = gcvTRUE;
                }
            }
        }
    }

    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_BUG_FIXES11))
    {
        codeGen.hasBugFixes11 = gcvTRUE;
    }
    else
    {
        codeGen.hasBugFixes11 = gcvFALSE;
    }

    if (! hardware->threadWalkerInPS)
    {
        /* Treat as vertex shader for linking purposes from this point on */
        if (shader->type == gcSHADER_TYPE_CL) {
            shader->type = gcSHADER_TYPE_VERTEX;
        }
    }

    /* Determine the maximum number of uniforms. */
    gcmONERROR(
        gcoHARDWARE_QueryShaderCaps(&vsUniformCount,
                                    &psUniformCount,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL));

    codeGen.uniformCount = (shader->type == gcSHADER_TYPE_VERTEX)
        ? vsUniformCount
        : psUniformCount;

    /* Get the maximum number of registers. */
    codeGen.registerCount = hardware->registerMax;
    codeGen.flags = Flags;

    codeGen.hasSIGN_FLOOR_CEIL = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));

    codeGen.hasSQRT_TRIG = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));

    /* Create uniform usage table. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(gcsSL_USAGE) * codeGen.uniformCount,
                       &pointer));

    codeGen.uniformUsage = pointer;

    gcmONERROR(
        gcoOS_MemFill(codeGen.uniformUsage,
                      0xFF,
                      gcmSIZEOF(gcsSL_USAGE) * codeGen.uniformCount));

    /* Create register usage table. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(gcsSL_USAGE) * codeGen.registerCount,
                       &pointer));

    codeGen.registerUsage = pointer;

    gcmONERROR(
        gcoOS_MemFill(codeGen.registerUsage,
                      0xFF,
                      gcmSIZEOF(gcsSL_USAGE) * codeGen.registerCount));

    /* Map all attributes. */
    gcmONERROR(_MapAttributes(Tree, &codeGen, codeGen.registerUsage));

    /* Map all uniforms. */
    gcmONERROR(_MapUniforms(Tree,
                              &codeGen,
                              codeGen.uniformUsage,
                              codeGen.uniformCount));

    /* Create function structures. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                                gcmSIZEOF(gcsSL_FUNCTION_CODE) *
                                    (shader->functionCount + shader->kernelFunctionCount + 1),
                                &pointer));

    codeGen.functions = pointer;

    gcmONERROR(gcoOS_ZeroMemory(codeGen.functions,
                                  gcmSIZEOF(gcsSL_FUNCTION_CODE) *
                                      (shader->functionCount + shader->kernelFunctionCount + 1)));

    /* Create code mapping table. */
    if (shader->codeCount > 0)
    {
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(gcsSL_CODE_MAP) *
                                        shader->codeCount,
                                    &pointer));

        codeGen.codeMap = pointer;

        gcmONERROR(
            gcoOS_ZeroMemory(codeGen.codeMap,
                             gcmSIZEOF(gctUINT) * shader->codeCount));

    }
    else
    {
        codeGen.codeMap = gcvNULL;
    }

    /* Generate code for each instruction. */
    gcmONERROR(_GenerateCode(Tree, &codeGen));

    /* Compute the size of the state buffer. */
    gcmONERROR(_GenerateStates(Tree, &codeGen, gcvNULL, &size, gcvNULL));

    /* Allocate a new state buffer. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                                *StateBufferSize + size,
                                &pointer));
    stateBuffer = pointer;

    /* Copy the old state buffer if there is any. */
    if (*StateBufferSize > 0)
    {
        gcmASSERT(*StateBuffer != gcvNULL);

        gcmONERROR(gcoOS_MemCopy(stateBuffer,
                                   *StateBuffer,
                                   *StateBufferSize));
    }

    /* Allocate a new hint structure. */
    if (*Hints == gcvNULL)
    {
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _gcsHINT),
                                    &pointer));

        *Hints = pointer;
    }

    /* Fill the state buffer. */
    gcmONERROR(_GenerateStates(Tree,
                                 &codeGen,
                                 stateBuffer + *StateBufferSize,
                                 &size,
                                 *Hints));

    /* Free any old state buffer. */
    if (*StateBufferSize > 0)
    {
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, *StateBuffer));
    }

    /* Set new state buffer. */
    *StateBuffer      = stateBuffer;
    *StateBufferSize += size;
    stateBuffer       = gcvNULL;

OnError:
    /* Free up uniform usage table. */
    if (codeGen.uniformUsage != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeGen.uniformUsage));
    }

    /* Free up register usage table. */
    if (codeGen.registerUsage != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeGen.registerUsage));
    }

    /* Free up the code tables. */
    if (codeGen.functions != gcvNULL)
    {
        gctSIZE_T i;

        for (i = 0; i <= shader->functionCount + shader->kernelFunctionCount; ++i)
        {
            while (codeGen.functions[i].root != gcvNULL)
            {
                gcsSL_PHYSICAL_CODE_PTR code = codeGen.functions[i].root;
                codeGen.functions[i].root = code->next;

                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, code));
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeGen.functions));
    }

    /* Free up the code mapping table. */
    if (codeGen.codeMap != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeGen.codeMap));
    }

    /* Free up the constant table. */
    while (codeGen.constants != gcvNULL)
    {
        gcsSL_CONSTANT_TABLE_PTR constant = codeGen.constants;
        codeGen.constants = constant->next;

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, constant));
    }

    /* Free up the state buffer. */
    if (stateBuffer != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, stateBuffer));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                                gcLoadShaders
********************************************************************************
**
**  Load a pre-compiled and pre-linked shader program into the hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
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
**      gcePRIMITIVE PrimitiveType
**          Primitive type to be rendered.
*/
gceSTATUS
gcLoadShaders(
    IN gcoHAL Hal,
    IN gctSIZE_T StateBufferSize,
    IN gctPOINTER StateBuffer,
    IN gcsHINT_PTR Hints
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hal=0x%x StateBufferSize=%d StateBuffer=0x%x Hints=0x%x",
                  Hal, StateBufferSize, StateBuffer, Hints);

    /* Call down to the hardware object. */
    status = gcoHARDWARE_LoadShaders(StateBufferSize, StateBuffer, Hints);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                                gcLoadKernel
********************************************************************************
**
**  Load a pre-compiled and pre-linked kernel program into the hardware.
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
*/
gceSTATUS
gcLoadKernel(
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

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Set HAL API to OpenCL. */
    gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvAPI_OPENCL));

    /* Load shaders. */
    if (StateBufferSize > 0)
    {
        gcmONERROR(gcoHARDWARE_LoadShaders(StateBufferSize,
                                           StateBuffer,
                                           Hints));
    }

    if (hardware->threadWalkerInPS)
    {
        /* Set input count. */
        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x01008,
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (Hints->fsInputCount) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) |
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))));

        /* Set temporary register control. */
        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x0100C,
                                           Hints->fsMaxTemp));
    }
    else
    {
        /* Set input count. */
        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x00808,
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (Hints->fsInputCount) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) |
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (~0) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)))));

        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x00820,
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) |
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) |
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (2) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) |
                                           ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (3) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)))));
        gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                           0x0080C,
                                           Hints->fsMaxTemp));
    }

    /* Set output count. */
    gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                       0x00804,
                                       ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))));

    /* Set local storage register count. */
    /* Set it to 0 since it is not used. */
    gcmONERROR(gcoHARDWARE_LoadState32(hardware,
                                       0x00924,
                                       ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))));

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcInvokeThreadWalker(
    IN gcsTHREAD_WALKER_INFO_PTR Info
    )
{
    gceSTATUS status;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Info=0x%x", Info);

    gcmGETHARDWARE(hardware);

    /* Switch to the 3D pipe. */
    gcmONERROR(gcoHARDWARE_SelectPipe(gcvNULL, gcvPIPE_3D));

    /* Set HAL API to OpenCL. */
    gcmVERIFY_OK(gcoHARDWARE_SetAPI(gcvAPI_OPENCL));

    if (hardware->shaderDirty)
    {
        /* Flush shader states. */
        gcmONERROR(gcoHARDWARE_FlushShaders(hardware, gcvPRIMITIVE_TRIANGLE_LIST));
    }

    /* Route to hardware. */
    status = gcoHARDWARE_InvokeThreadWalker(Info);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gctSIZE_T
gcSHADER_GetHintSize(
    void
    )
{
    gcmHEADER();

    gcmFOOTER_ARG("return=%d", gcmSIZEOF(struct _gcsHINT));
    return gcmSIZEOF(struct _gcsHINT);
}

gctBOOL
gcSHADER_CheckBugFixes10(
    void
    )
{
    gctBOOL hasBugFixes10;

    gcmHEADER();

    if (gcoHARDWARE_IsFeatureAvailable(gcvFEATURE_BUG_FIXES10))
    {
        hasBugFixes10 = gcvTRUE;
    }
    else
    {
        hasBugFixes10 = gcvFALSE;
    }

    gcmFOOTER_ARG("return=%d", hasBugFixes10);
    return hasBugFixes10;
}

#endif /* VIVANTE_NO_3D */


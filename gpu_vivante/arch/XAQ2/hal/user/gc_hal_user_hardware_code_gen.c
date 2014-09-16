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
**  Hardware dependent gcSL code generator.
*/

#include "gc_hal_user_hardware_precomp.h"

#ifndef VIVANTE_NO_3D

#include "gc_hal_user_compiler.h"

#define _HAS_GETEXPT_GETMANT_       0

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#define _SUPPORT_INTEGER_NEGATIVE_MODIFIER_    0

#ifndef M_PI
#   define M_PI 3.14159265358979323846f
#endif

gctUINT type_conv[] =
{
	0x0, /* gcSL_FLOAT     */
	0x2, /* gcSL_INTEGER   */
	0x2, /* gcSL_BOOLEAN   */
	0x5, /* gcSL_UINT32    */
	0x4, /* gcSL_INT8      */
	0x7, /* gcSL_UINT8     */
	0x3, /* gcSL_INT16     */
	0x6, /* gcSL_UINT16    */
	0x2, /* gcSL_INT64   ? */
	0x5, /* gcSL_UINT64  ? */
	0x2, /* gcSL_INT128  ? */
	0x5, /* gcSL_UINT128 ? */
	0x1, /* gcSL_FLOAT16   */
	0x0, /* gcSL_FLOAT64 ? */
	0x0, /* gcSL_FLOAT128? */
};

extern void
_ConvertType(
    IN gcSHADER_TYPE Type,
    IN gctINT Length,
    OUT gctINT * Components,
    OUT gctINT * Rows
    );

extern gctBOOL
_GetPreviousCode(
    IN gcsCODE_GENERATOR_PTR CodeGen,
    OUT gctUINT32_PTR * Code
    );

extern gceSTATUS
_AddConstantVec1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctFLOAT Constant,
    OUT gctINT_PTR Index,
    OUT gctUINT8_PTR Swizzle
    );

extern gceSTATUS
_AddConstantVec2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctFLOAT Constant1,
    IN gctFLOAT Constant2,
    OUT gctINT_PTR Index,
    OUT gctUINT8_PTR Swizzle
    );

extern gceSTATUS
_AddConstantIVec1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gctINT Constant,
    OUT gctINT_PTR Index,
    OUT gctUINT8_PTR Swizzle
    );

extern gctUINT8
_ReplicateSwizzle(
    IN gctUINT8 Swizzle,
    IN gctUINT Index
    );

extern gctUINT8
_Enable2Swizzle(
    IN gctUINT32 Enable
    );

extern gctBOOL
_hasSIGN_FLOOR_CEIL(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    );

extern gctBOOL
_hasSQRT_TRIG(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32_PTR States
    );

extern gctBOOL
_hasCL(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_isGC2100Signed_8_16_store(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_isGC2100Unsigned_8_16_store(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_isGC2100Signed_8_16(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_isGC2100Unsigned_8_16(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_isCLShader(
	IN gcLINKTREE Tree,
	IN gcsCODE_GENERATOR_PTR CodeGen,
	IN gcSL_INSTRUCTION Instruction,
	IN OUT gctUINT32_PTR States
	);

extern gctBOOL
_codeHasCaller(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen
    );

extern void
_SetValueType0(
    IN gctUINT ValueType,
    IN OUT gctUINT32 * States
    );

gctBOOL
value_type0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_TARGET_GET(Instruction->temp, Format);
    gctUINT value_type0 = type_conv[format];
	gctUINT inst_type0 = value_type0 & 0x1;
	gctUINT inst_type1 = value_type0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (inst_type0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (inst_type1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

    return gcvTRUE;
}


gctBOOL
value_type0_F2I(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_TARGET_GET(Instruction->temp, Format);
    gctUINT value_type0 = type_conv[format];
	gctUINT inst_type0 = value_type0 & 0x1;
	gctUINT inst_type1 = value_type0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (inst_type0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (inst_type1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

    return gcvTRUE;
}

gctBOOL
value_type0_32bit_from_src0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT value_type0;
	gctUINT inst_type0;
	gctUINT inst_type1;
    gctUINT format = gcmSL_SOURCE_GET(Instruction->source0, Format);

	/* Select does not support 8/16-bit integer type. */
	/* Convert it to 32-bit for select. */
	if (format == gcSL_INT8 || format == gcSL_INT16)
	{
		format = gcSL_INT32;
	}
	else if (format == gcSL_UINT8 || format == gcSL_UINT16)
	{
		format = gcSL_UINT32;
	}
    value_type0 = type_conv[format];
	inst_type0 = value_type0 & 0x1;
	inst_type1 = value_type0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (inst_type0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (inst_type1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

    return gcvTRUE;
}

static gctBOOL
value_type0_from_src0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_SOURCE_GET(Instruction->source0, Format);
    gctUINT value_type0 = type_conv[format];
	gctUINT inst_type0 = value_type0 & 0x1;
	gctUINT inst_type1 = value_type0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (inst_type0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (inst_type1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

    return gcvTRUE;
}

gctBOOL
value_type0_from_next_inst(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gcSL_INSTRUCTION nextInstruction = Instruction + 1;
    gctUINT format = gcmSL_TARGET_GET(nextInstruction->temp, Format);
    gctUINT value_type0 = type_conv[format];
	gctUINT inst_type0 = value_type0 & 0x1;
	gctUINT inst_type1 = value_type0 >> 1;

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21))) | (((gctUINT32) ((gctUINT32) (inst_type0) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1))))))) << (0 ? 21:21)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30))) | (((gctUINT32) ((gctUINT32) (inst_type1) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1))))))) << (0 ? 31:30)));

    return gcvTRUE;
}


static gctUINT32
_sl2gcCondition(
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
    }

    gcmFATAL("ERROR: Invalid condition 0x%04X", Condition);
    return 0x00;
}

static gctBOOL
saturate(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));

    return gcvTRUE;
}

static gctBOOL
conditionLT(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0_32bit_from_src0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
conditionLTAbs1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0_32bit_from_src0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
conditionNZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0B & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
conditionGE(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x03 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
swizzle2X(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle((((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ), 0)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
swizzle2ZorW(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT16 source = Instruction->source1;
    gctUINT16 index  = Instruction->source1Index;

    gctUINT32 usage = 0;
    if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
    {
        usage = Tree->tempArray[gcmSL_INDEX_GET(index, Index)].usage;
    }
    else if (gcmSL_SOURCE_GET(source, Type) == gcSL_ATTRIBUTE)
    {
        switch (Tree->shader->attributes[gcmSL_INDEX_GET(index, Index)]->type)
        {
        case gcSHADER_FLOAT_X1:
            usage = 0x1;
            break;

        case gcSHADER_FLOAT_X2:
            usage = 0x3;
            break;

        case gcSHADER_FLOAT_X3:
            usage = 0x7;
            break;

        case gcSHADER_FLOAT_X4:
            usage = 0xF;
            break;

        default:
            break;
        }
    }
    else if (gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM)
    {
        switch (Tree->shader->uniforms[gcmSL_INDEX_GET(index, Index)]->type)
        {
        case gcSHADER_FLOAT_X1:
            usage = 0x1;
            break;

        case gcSHADER_FLOAT_X2:
            usage = 0x3;
            break;

        case gcSHADER_FLOAT_X3:
            usage = 0x7;
            break;

        case gcSHADER_FLOAT_X4:
            usage = 0xF;
            break;

        default:
            break;
        }
    }

    gcmASSERT(usage != 0);

    if (usage == 0x7)
    {
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle((((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ), 2)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));
    }
    else if (usage == 0xF)
    {
        States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle((((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ), 3)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));
    }

    return gcvTRUE;
}

static gctBOOL
conditionGZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
conditionGT(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0_32bit_from_src0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
conditionGTAbs1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0_32bit_from_src0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
crossSwizzle(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT32 swizzle0 = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
    gctUINT32 swizzle1 = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );

    swizzle0 = (((swizzle0 >> 4) & 3) << 0)
             | (((swizzle0 >> 0) & 3) << 2)
             | (((swizzle0 >> 2) & 3) << 4)
             | (((swizzle0 >> 2) & 3) << 6);
    swizzle1 = (((swizzle1 >> 2) & 3) << 0)
             | (((swizzle1 >> 4) & 3) << 2)
             | (((swizzle1 >> 0) & 3) << 4)
             | (((swizzle1 >> 0) & 3) << 6);

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle0) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle1) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
branch(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gcSL_BRANCH_LIST entry;
    gctPOINTER pointer;

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) ((gctUINT32) (_sl2gcCondition((gcSL_CONDITION) gcmSL_TARGET_GET(Instruction->temp, Condition))) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   gcmSIZEOF(struct _gcSL_BRANCH_LIST),
                                   &pointer)))
    {
        return gcvFALSE;
    }
    entry = pointer;

    entry->next   = Tree->branch;
    entry->ip     = gcsCODE_GENERATOR_GetIP(CodeGen);
    entry->target = Instruction->tempIndex;
    entry->call   = (Instruction->opcode == gcSL_CALL);

    Tree->branch  = entry;
    if (_isCLShader(Tree, CodeGen, Instruction, States))
    {
    	value_type0_from_src0(Tree, CodeGen, Instruction, States);
    }
    return gcvTRUE;
}

static gctBOOL
_NoLabel(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT pc = Instruction - Tree->shader->code;

    if (Tree->hints[pc].callers != gcvNULL)
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

/* check to see if the jmp target is the one after next instruction */
static gctBOOL
_jmpToNextPlusOne(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctINT   jmpTarget     = Instruction->tempIndex;
    gctINT   curCodeIndex  = Instruction - Tree->shader->code;


    return (jmpTarget == curCodeIndex+2) && _NoLabel(Tree, CodeGen, Instruction, States) ;
}

/* check to see if the jmp target is the one after next two instruction */
static gctBOOL
_jmpToNextPlusTwo(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctINT   jmpTarget     = Instruction->tempIndex;
    gctINT   curCodeIndex  = Instruction - Tree->shader->code;


    return (jmpTarget == curCodeIndex+3)  && _NoLabel(Tree, CodeGen, Instruction, States);
}

static gctBOOL
kill(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) ((gctUINT32) (_sl2gcCondition((gcSL_CONDITION) gcmSL_TARGET_GET(Instruction->temp, Condition))) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    return gcvTRUE;
}

static gctBOOL
add2mad(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT32_PTR code;
    gctINT32 srcAddress[3];

    /* Get previous instruction. */
    if (!_GetPreviousCode(CodeGen, &code))
    {
        /* No previous instruction. */
        return gcvTRUE;
    }

    /* Get source 0 and 1 types and addressses. */
    if ((((((gctUINT32) (code[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) ==
        0x2)
    {
        srcAddress[0] = (((((gctUINT32) (code[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
    }
    else
    {
        srcAddress[0] = -1;
    }
    if ((((((gctUINT32) (code[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) ==
        0x2)
    {
        srcAddress[1] = (((((gctUINT32) (code[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
    }
    else
    {
        srcAddress[1] = -1;
    }

    /* Check that the previous instruction is a MUL. */
    if (((((gctUINT32) (code[0])) >> (0 ? 5:0) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) == (0x03 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))))

    /* Both instructions must have no conditional code. */
    &&  ((((gctUINT32) (States[0])) >> (0 ? 10:6) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))) == (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))))
    &&  ((((gctUINT32) (code[0])) >> (0 ? 10:6) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))) == (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))))

    /* Both instructions must have the same destination. */
    &&  ((((((gctUINT32) (States[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ) ==
         (((((gctUINT32) (code[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ))
    &&  ((((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) ) ==
         (((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) ))

    /* Add sources are not the same. */
    &&  (((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ))
        || ((((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ))
        || ((((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) ))
        || ((((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ))
        || ((((((gctUINT32) (States[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) ))
        || ((((((gctUINT32) (States[1])) >> (0 ? 31:31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) ) !=
            (((((gctUINT32) (States[3])) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) ))
        )
    )
    {
        gctBOOL diffConst;

        /* Get source 2 type and address. */
        if ((((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) ==
            0x2)
        {
            srcAddress[2] = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
        }
        else
        {
            srcAddress[2] = -1;
        }

        /* Determine the usage of constants. */
        diffConst = gcvFALSE;
        if ((srcAddress[0] >= 0)
        &&  (srcAddress[2] >= 0)
        &&  (srcAddress[0] != srcAddress[2])
        )
        {
            /* Source 0 and 2 are using different constants. */
            diffConst = gcvTRUE;
        }

        if ((srcAddress[1] >= 0)
        &&  (srcAddress[2] >= 0)
        &&  (srcAddress[1] != srcAddress[2])
        )
        {
            /* Source 1 and 2 are using different constants. */
            diffConst = gcvTRUE;
        }

        /* First, see if the previous destination matches the current source0:
        **
        **  MUL rd, s0(0), s1(1)
        **  ADD rd, rd(0), s2(2)
        */
        if (((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) ==
             0x0)
        &&  ((((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) ) ==
             (((((gctUINT32) (code[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ))
        &&  ((((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) ) ==
             (((((gctUINT32) (code[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) ))
        &&  ((((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) ) ==
             _Enable2Swizzle((((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) )))
        &&  !diffConst
        )
        {
            /* Replace previous source2 with current source2 and change opcode
            ** to MAD:
            **
            **  MAD rd, s0(0), s1(1), s2(2)
            */
            code[0] = ((((gctUINT32) (code[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            code[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (code[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) )) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));

            /* Expand ABS modifier for rd to MUL sources. */
            if ((((((gctUINT32) (States[1])) >> (0 ? 31:31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) ))
            {
                code[1] = ((((gctUINT32) (code[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
                code[2] = ((((gctUINT32) (code[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));
            }

            /* Copy NEG modifier for rd to MUL source0. */
            if ((((((gctUINT32) (States[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) ))
            {
                code[1] = ((((gctUINT32) (code[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (code[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) ) ^ 1) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)));
            }

            /* Disgard current instruction. */
            return gcvFALSE;
        }

        /* Get source 2 type and address. */
        if ((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) ) ==
            0x2)
        {
            srcAddress[2] = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
        }
        else
        {
            srcAddress[2] = -1;
        }

        /* Determine the usage of constants. */
        diffConst = gcvFALSE;
        if ((srcAddress[0] >= 0)
        &&  (srcAddress[2] >= 0)
        &&  (srcAddress[0] != srcAddress[2])
        )
        {
            /* Source 0 and 2 are using different constants. */
            diffConst = gcvTRUE;
        }

        if ((srcAddress[1] >= 0)
        &&  (srcAddress[2] >= 0)
        &&  (srcAddress[1] != srcAddress[2])
        )
        {
            /* Source 0 and 2 are using different constants. */
            /* Source 1 and 2 have different constants. */
            diffConst = gcvTRUE;
        }

        /* Otherwise, see if the previous destination matches the current
        **  source2:
        **
        **  MUL rd, s0(0), s1(1)
        **  ADD rd, s2(0), rd(2)
        */
        if (((((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) ==
             0x0)
        &&  ((((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ) ==
             (((((gctUINT32) (code[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ))
        &&  ((((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) ) ==
             (((((gctUINT32) (code[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) ))
        &&  ((((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ) ==
             _Enable2Swizzle((((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) )))
        &&  !diffConst
        )
        {
            /* Replace previous source2 with current source0 and change opcode
            ** to MAD:
            **
            **  MAD rd, s0(0), s1(1), s2(0)
            */
            code[0] = ((((gctUINT32) (code[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) )) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) )) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) )) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1))))))) << (0 ? 22:22)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[1])) >> (0 ? 31:31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) )) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) )) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1))))))) << (0 ? 27:25)));
            code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) )) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1))))))) << (0 ? 30:28)));

            /* Expand ABS modifier for rd to MUL sources. */
            if ((((((gctUINT32) (States[3])) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) ))
            {
                code[1] = ((((gctUINT32) (code[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
                code[2] = ((((gctUINT32) (code[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26)));
            }

            /* Copy NEG modifier for rd to MUL source0. */
            if ((((((gctUINT32) (States[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) ))
            {
                code[1] = ((((gctUINT32) (code[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (code[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) ) ^ 1) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30)));
            }

            /* Disgard current instruction. */
            return gcvFALSE;
        }
    }

    /* Nothing can be merged. */
    return gcvTRUE;
}

static gctBOOL
mov(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT32_PTR code;

    if (((((gctUINT32) (States[0])) >> (0 ? 10:6) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))) == (0x00 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))))
    &&  ((((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) ==
         0x0 )
    &&  ((((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ) ==
         (((((gctUINT32) (States[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ) )
    &&  ((((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) ) ==
         (((((gctUINT32) (States[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) ) )
    &&  ((((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) ) ==
         _Enable2Swizzle((((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) )))
    )
    {
        /* Source and destination are the same. */
        return gcvFALSE;
    }

    if (! _codeHasCaller(Tree, CodeGen)
    &&  _GetPreviousCode(CodeGen, &code)
    &&  ((((gctUINT32) (code[0])) >> (0 ? 5:0) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) == (0x09 & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))))
    &&  ((((((gctUINT32) (code[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ) ==
         (((((gctUINT32) (States[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ))
    &&  ((((((gctUINT32) (code[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) ) ==
         (((((gctUINT32) (States[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) ))
    &&  (!((((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) ) &
           (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) )))
    &&  ((((((gctUINT32) (code[0])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) ) ==
         (((((gctUINT32) (States[0])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) ))
    &&  ((((((gctUINT32) (code[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ) ==
         (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) ))
    &&  ((((((gctUINT32) (code[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ) ==
         (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ))
    &&  ((((((gctUINT32) (code[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) ) ==
         (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) ))
    &&  ((((((gctUINT32) (code[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) ) ==
         (((((gctUINT32) (States[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) ))
    &&  ((((((gctUINT32) (code[3])) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) ) ==
         (((((gctUINT32) (States[3])) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) ))
    )
    {
        gctUINT32 enable        = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
        gctUINT32 codeSwizzle   = (((((gctUINT32) (code[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );
        gctUINT32 sourceSwizzle = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );

        /* Check if current instruction depends on previous instruction. */
        if ((((((gctUINT32) (code[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) ) ==
            (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) ))
        {
            gctUINT32 codeEnable    = (((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
            if ((codeEnable & gcSL_ENABLE_X) &&
                (((sourceSwizzle & 0x03) == 0x00) ||
                 ((sourceSwizzle & 0x0C) == 0x00) ||
                 ((sourceSwizzle & 0x30) == 0x00) ||
                 ((sourceSwizzle & 0xC0) == 0x00)))
            {
                return gcvTRUE;
            }

            if ((codeEnable & gcSL_ENABLE_Y) &&
                (((sourceSwizzle & 0x03) == 0x01) ||
                 ((sourceSwizzle & 0x0C) == 0x04) ||
                 ((sourceSwizzle & 0x30) == 0x10) ||
                 ((sourceSwizzle & 0xC0) == 0x40)))
            {
                return gcvTRUE;
            }

            if ((codeEnable & gcSL_ENABLE_Z) &&
                (((sourceSwizzle & 0x03) == 0x02) ||
                 ((sourceSwizzle & 0x0C) == 0x08) ||
                 ((sourceSwizzle & 0x30) == 0x20) ||
                 ((sourceSwizzle & 0xC0) == 0x80)))
            {
                return gcvTRUE;
            }

            if ((codeEnable & gcSL_ENABLE_W) &&
                (((sourceSwizzle & 0x03) == 0x03) ||
                 ((sourceSwizzle & 0x0C) == 0x0C) ||
                 ((sourceSwizzle & 0x30) == 0x30) ||
                 ((sourceSwizzle & 0xC0) == 0xC0)))
            {
                return gcvTRUE;
            }
        }

        if (enable & gcSL_ENABLE_X)
        {
            codeSwizzle = (codeSwizzle & ~0x03) | (sourceSwizzle & 0x03);
        }

        if (enable & gcSL_ENABLE_Y)
        {
            codeSwizzle = (codeSwizzle & ~0x0C) | (sourceSwizzle & 0x0C);
        }

        if (enable & gcSL_ENABLE_Z)
        {
            codeSwizzle = (codeSwizzle & ~0x30) | (sourceSwizzle & 0x30);
        }

        if (enable & gcSL_ENABLE_W)
        {
            codeSwizzle = (codeSwizzle & ~0xC0) | (sourceSwizzle & 0xC0);
        }

        code[0] = ((((gctUINT32) (code[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) ((((((gctUINT32) (code[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) ) | (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) )) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

        code[3] = ((((gctUINT32) (code[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (codeSwizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

        return gcvFALSE;
    }

    return gcvTRUE;
}
static gctBOOL
one_0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f,
                                  &index,
                                  &swizzle));

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
              | ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));

    return gcvTRUE;
}
static gctBOOL
rcppi2_1_dot5_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  1.0f / (2.0f * (float) M_PI),
                                  0.5f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
pi2_1_pi_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  2.0f * (float) M_PI,
                                  (float) M_PI,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}
static gctBOOL
abs_0_abs_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));

    return gcvTRUE;
}
static gctBOOL
halfpi_0_abs_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  (float) M_PI / 2.0f,
                                  &index,
                                  &swizzle));

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
              | ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));
    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23)));

    return gcvTRUE;
}

static gctBOOL
gt_abs_0_one_1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f,
                                  &index,
                                  &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));
    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
rcppi2_1(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  2.0f / (float) M_PI,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
half_pi_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  (float) M_PI / 2.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
tan9_1_tan7_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  5237760.0f / 239500800.0f,
                                  65280.0f / 1209600.0f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
tan5_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  4032.0f / 30240.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
tan3_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  240.0f / 720.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
asin9_1_asin7_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  35.0f / 1152.0f,
                                  5.0f / 112.0f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
asin5_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  3.0f / 40.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
asin3_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f / 6.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
atan9_1_atan7_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  0.023060280510707944f,
                                  0.09045060332177933f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
atan5_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  0.18449097954748866f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
atan3_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  0.33168528523552876f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor9_1_factor7_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  1.0f / 362880.0f,
                                  1.0f / 5040.0f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor8_1_factor6_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec2(Tree,
                                  CodeGen,
                                  1.0f / 40320.0f,
                                  1.0f / 720.0f,
                                  &index,
                                  &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 0)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (_ReplicateSwizzle(swizzle, 1)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor5_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f / 120.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor4_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f / 24.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor3_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f / 6.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
factor2_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f / 2.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}


static gctBOOL
one_2(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
minusOne_2_value_type0_from_src0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                  CodeGen,
                                  -1,
                                  &index,
                                  &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

	value_type0_from_src0(Tree, CodeGen, Instruction, States);

    return gcvTRUE;
}

static gctBOOL
_is_value_type_float(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_TARGET_GET(Instruction->temp, Format);

    if (format == gcSL_FLOAT)
    {
        return gcvTRUE;
    }

    return gcvFALSE;
}

static gctBOOL
_hasSIGN_FLOOR_CEIL_and_float_type(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    if (! _hasSIGN_FLOOR_CEIL(Tree, CodeGen, Instruction, States))
    {
        return gcvFALSE;
    }
    else
    {
        return _is_value_type_float(Tree, CodeGen, Instruction, States);
    }
}

static gctBOOL
one_1_conditionGZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f,
                                  &index,
                                  &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
one_1_conditionLZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  1.0f,
                                  &index,
                                  &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
zero_1_conditionEQ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  0.0f,
                                  &index,
                                  &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x05 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    return gcvTRUE;
}

static gctBOOL
int_one_1_conditionGZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   1,
                                   &index,
                                   &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
int_minus_one_1_conditionLZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   -1,
                                   &index,
                                   &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0F & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
int_zero_1_conditionEQ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0,
                                   &index,
                                   &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x05 & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
smallest0_2_GZ(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantVec1(Tree,
                                  CodeGen,
                                  0.0f,
                                  &index,
                                  &swizzle));

    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6))) | (((gctUINT32) (0x0D & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1))))))) << (0 ? 10:6)));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    return gcvTRUE;
}

static gctBOOL
enable_w(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23))) | (((gctUINT32) ((gctUINT32) (gcSL_ENABLE_W) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1))))))) << (0 ? 26:23)));

    return gcvTRUE;
}

static gctBOOL
source0_add_swizzle_w(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT8 sourceSwizzle;
    gctUINT8 newSourceSwizzle;

    sourceSwizzle  = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );

    newSourceSwizzle = gcmComposeSwizzle(gcmExtractSwizzle(sourceSwizzle, 0),
                                         gcmExtractSwizzle(sourceSwizzle, 1),
                                         gcmExtractSwizzle(sourceSwizzle, 2),
                                         gcSL_SWIZZLE_W );

    /* adjust swizzle to use .w */
    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (newSourceSwizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));

    return gcvTRUE;
}

static gctBOOL
_NoLabel_isGC2100Signed_8_16_store(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT pc = Instruction - Tree->shader->code;

    if (Tree->hints[pc].callers != gcvNULL)
    {
        return gcvFALSE;
    }

    return _isGC2100Signed_8_16_store(Tree, CodeGen, Instruction, States);
}

static gctBOOL
_NoLabel_isGC2100Unsigned_8_16_store(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT pc = Instruction - Tree->shader->code;

    if (Tree->hints[pc].callers != gcvNULL)
    {
        return gcvFALSE;
    }

    return _isGC2100Unsigned_8_16_store(Tree, CodeGen, Instruction, States);
}

static gctBOOL
_UseDestInNextOnly(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT temp = Instruction->tempIndex;
    if ((Tree->tempArray[temp].users == gcvNULL)
    ||  (Tree->tempArray[temp].users->next != gcvNULL)
    )
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

static gctBOOL
_UseDestInNextTwoOnly(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT temp = Instruction->tempIndex;

    /* work around the missing usage in the user list */
    if (_UseDestInNextOnly(Tree, CodeGen, Instruction, States))
        return gcvTRUE;

    if ((Tree->tempArray[temp].users == gcvNULL)
    ||  (Tree->tempArray[temp].users->next == gcvNULL)
    ||  (Tree->tempArray[temp].users->next->next != gcvNULL)
    )
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

static gcSL_ENABLE
_GetUsedComponents(
    IN gcSL_INSTRUCTION Instruction,
    IN gctINT           SourceNo
    )
{
    gctUINT16    enable = gcmSL_TARGET_GET(Instruction->temp, Enable);
    gctUINT16    source = SourceNo == 0 ? Instruction->source0 : Instruction->source1;
    gcSL_ENABLE  usedComponents = 0;
    gctINT       i;

    for (i=0; i < gcSL_COMPONENT_COUNT; i++)
    {
        if (gcmIsComponentEnabled(enable, i))
        {
            gcSL_SWIZZLE swizzle = 0;
            switch (i)
            {
            case 0:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX);
                break;
            case 1:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY);
                break;
            case 2:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ);
                break;
            case 3:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW);
                break;
            default:
                gcmASSERT(0);
                break;
            } /* switch */
            /* the component in swizzle is used, convert swizzle to enable */
            usedComponents |= 1 << swizzle;
        } /* if */
    } /* for */
    return usedComponents;
}

static gctBOOL
_HandleBiasedTextureLoad(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctINT temp = gcmSL_INDEX_GET(Instruction->source1Index, Index);
    gctINT components = 0, rows = 0;

    /* source is Attribute ot Temp ?*/
    if (gcmSL_SOURCE_GET(Instruction->source1, Type) == gcSL_TEMP)
    {
        /* don't know if the temp is allocated in proper register which
         * can extend to use w component, so skip it */
        return gcvFALSE;

    }
    else if (gcmSL_SOURCE_GET(Instruction->source1, Type) == gcSL_ATTRIBUTE)
    {
        gcsLINKTREE_LIST_PTR users;

        gcSHADER_TYPE type = Tree->shader->attributes[temp]->type;
        _ConvertType(Tree->shader->attributes[temp]->type,
                     Tree->shader->attributes[temp]->arraySize,
                     &components,
                     &rows);
        if (rows > 1)
        {
            /* cannot handle multiple rows */
            return gcvFALSE;
        }

        if (type == gcSHADER_FLOAT_X4)
        {
            /* check if the w component is used in any users */
            users = Tree->attributeArray[temp].users;

            for (; users != gcvNULL; users = users->next)
            {
                gcSL_INSTRUCTION code = &Tree->shader->code[users->index];
                gcSL_ENABLE      usedComponents = 0;
                switch (code->opcode) {
                case gcSL_TEXLD:
                case gcSL_TEXLDP:
                case gcSL_TEXBIAS:
                case gcSL_TEXGRAD:
                case gcSL_TEXLOD:
                case gcSL_JMP:
                case gcSL_CALL:
                case gcSL_RET:
                case gcSL_KILL:
                    continue;
                    /* break; */
                default :
                    /* check source 0 */
                    if (gcmSL_SOURCE_GET(code->source0, Type) == gcSL_ATTRIBUTE &&
                        gcmSL_INDEX_GET(code->source0Index, Index) == temp)
                    {
                        usedComponents = _GetUsedComponents(code, 0);
                        if ((usedComponents & gcSL_ENABLE_W) != 0)
                            return gcvFALSE;
                    }
                    /* check source 1 */
                    if (gcmSL_SOURCE_GET(code->source1, Type) == gcSL_ATTRIBUTE &&
                        gcmSL_INDEX_GET(code->source1Index, Index) == temp)
                    {
                        usedComponents = _GetUsedComponents(code, 1);
                        if ((usedComponents & gcSL_ENABLE_W) != 0)
                            return gcvFALSE;
                    }
                    break;
                }
            }
        }
        else  /* not gcSHADER_FLOAT_X4 */
        {
            /* change the type to gcSHADER_FLOAT_X4 so we can use the w component */
            gcmASSERT(components == 2 || components == 3);
            Tree->shader->attributes[temp]->type = gcSHADER_FLOAT_X4;
        }

        return gcvTRUE;
    }
    return gcvFALSE;
}

static gctBOOL
_IntUseDestInNextOnly(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    if (gcmSL_TARGET_GET(Instruction->temp, Format) == gcSL_FLOAT)
    {
        return gcvFALSE;
    }
    else
    {
        gctUINT temp = Instruction->tempIndex;
        if ((Tree->tempArray[temp].users == gcvNULL)
        ||  (Tree->tempArray[temp].users->next != gcvNULL)
        )
        {
            return gcvFALSE;
        }

        return gcvTRUE;
    }
}

static gctBOOL
_IntOpcode(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    if (gcmSL_TARGET_GET(Instruction->temp, Format) == gcSL_FLOAT)
    {
        return gcvFALSE;
    }
    else
    {
        return gcvTRUE;
    }
}


static gctBOOL
_SatAbs0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    /* Enable saturation. */
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));

    /* Set source 0 to absolute modifier. */
    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31)));
    return gcvTRUE;
}

static gctBOOL
_Sat0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    /* Enable saturation. */
    States[0] = ((((gctUINT32) (States[0])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)));
    return gcvTRUE;
}

static gctBOOL
int_value_type0_const_0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0x0,
                                   &index,
                                   &swizzle));

    States[1] = ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1))))))) << (0 ? 20:12)))
              | ((((gctUINT32) (States[1])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1))))))) << (0 ? 29:22)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
int_value_type0_const_7F800000(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0x7F800000,
                                   &index,
                                   &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
int_value_type0_const_23(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   23,
                                   &index,
                                   &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
int_value_type0_const_Minus127(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0xFFFFFF81,
                                   &index,
                                   &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
uint_value_type0_const_7FFFFF(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0x007FFFFF,
                                   &index,
                                   &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x5, States);

    return gcvTRUE;
}

static gctBOOL
uint_value_type0_const_800000(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0x00800000,
                                   &index,
                                   &swizzle));

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x5, States);

    return gcvTRUE;
}

static gctBOOL
int_value_type0_const_24_16(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_TARGET_GET(Instruction->temp, Format);
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    if (format == gcSL_INT8)
    {
        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                       CodeGen,
                                       24,
                                       &index,
                                       &swizzle));
    }
    else
    {
        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                       CodeGen,
                                       16,
                                       &index,
                                       &swizzle));
    }

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x2, States);

    return gcvTRUE;
}

static gctBOOL
uint_value_type0_const_FF_FFFF(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
    gctUINT format = gcmSL_TARGET_GET(Instruction->temp, Format);
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    if (format == gcSL_UINT8)
    {
        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                       CodeGen,
                                       0x000000FF,
                                       &index,
                                       &swizzle));
    }
    else
    {
        gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                       CodeGen,
                                       0x0000FFFF,
                                       &index,
                                       &swizzle));
    }

    States[3] = ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1))))))) << (0 ? 12:4)))
              | ((((gctUINT32) (States[3])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1))))))) << (0 ? 21:14)));

    _SetValueType0(0x5, States);

    return gcvTRUE;
}


static gctBOOL
value_type0_const_0(
    IN gcLINKTREE Tree,
    IN gcsCODE_GENERATOR_PTR CodeGen,
    IN gcSL_INSTRUCTION Instruction,
    IN OUT gctUINT32 * States
    )
{
	gctINT index = 0;
	gctUINT8 swizzle = 0;

    gcmVERIFY_OK(_AddConstantIVec1(Tree,
                                   CodeGen,
                                   0x0,
                                   &index,
                                   &swizzle));

    States[2] = ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7))) | (((gctUINT32) ((gctUINT32) (index) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1))))))) << (0 ? 15:7)))
              | ((((gctUINT32) (States[2])) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1))))))) << (0 ? 24:17)));

    value_type0(Tree, CodeGen, Instruction, States);

    return gcvTRUE;
}

gcsSL_PATTERN patterns[] =
{
	/*
        ADD 1, 2, 3
        SAT 4, 1
            add.sat 4, 2, 0, 3, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SAT, 4, 1 },
        { -1, 0x01, 4, 2, 0, 3, 0, _Sat0 },

	/*
        SUB 1, 2, 3
        SAT 4, 1
            add.sat 4, 2, 0, -3, 0
    */
    { 2, gcSL_SUB, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SAT, 4, 1 },
            { -1, 0x01, 4, 2, 0, -3, 0, _Sat0 },

	/*
        MUL 1, 2, 3
        SAT 4, 1
            mul.sat 4, 2, 3, 0, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SAT, 4, 1 },
            { -1, 0x03, 4, 2, 3, 0, 0, _Sat0 },

    /*
        MUL 1, 2, 3
        ADD 4, 5, 1
        SAT 6, 4
            mad.sat 6, 2, 3, 5
    */
    { 3, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 2, gcSL_ADD, 4, 5, 1, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SAT, 6, 4 },
        { -1, 0x02, 6, 2, 3, 5, 0, _Sat0 },

    /*
        MUL 1, 2, 3
        ADD 4, 5, 1
            imadlo0 4, 2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_ADD, 4, 5, 1, 0, 0, _NoLabel },
        { -1, 0x4C, 4, 2, 3, 5, 0, value_type0 },

    /*
        MUL 1, 2, 3
        ADD 4, 5, 1
            mad 4, 2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ADD, 4, 5, 1, 0, 0, _NoLabel },
        { -1, 0x02, 4, 2, 3, 5, 0 },

    /*
        MUL 1, 2, 3
        ADD 4, 1, 5
            imadlo0 4, 2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_ADD, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x4C, 4, 2, 3, 5, 0, value_type0 },

    /*
        MUL 1, 2, 3
        ADD 4, 1, 5
            mad 4, 2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ADD, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x02, 4, 2, 3, 5, 0 },

#if _SUPPORT_INTEGER_NEGATIVE_MODIFIER_
    /*
        MUL 1, 2, 3
        SUB 4, 1, 5
            imadlo0 4, 2, 3, -5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_SUB, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x4C, 4, 2, 3, -5, 0, value_type0 },
#else
    /* Negative modifier is not allowed in imadlo0. */
    /*
        MUL 1, 2, 3
        SUB 4, 1, 5
            imullo0 1, 2, 3, 0, 0
            add     4, 1, 0, -5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_SUB, 4, 1, 5, 0, 0, _NoLabel },
        { -2, 0x3C, 1, 2, 3, 0, 0, value_type0 },
        { -1, 0x01, 4, 1, 0, -5, 0, value_type0 },
#endif

    /*
        MUL 1, 2, 3
        SUB 4, 1, 5
            mad 4, 2, 3, -5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUB, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x02, 4, 2, 3, -5, 0 },

#if _SUPPORT_INTEGER_NEGATIVE_MODIFIER_
    /*
        MUL 1, 2, 3
        SUB 4, 5, 1
            imadlo0 4, -2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_SUB, 4, 5, 1, 0, 0, _NoLabel },
        { -1, 0x4C, 4, -2, 3, 5, 0, value_type0 },
#else
    /* Negative modifier is not allowed in imadlo0. */
    /*
        MUL 1, 2, 3
        SUB 4, 5, 1
            imullo0 1, 2, 3, 0, 0
            add     4, 5, 0, -1, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _IntUseDestInNextOnly },
    { 1, gcSL_SUB, 4, 5, 1, 0, 0, _NoLabel },
        { -2, 0x3C, 1, 2, 3, 0, 0, value_type0 },
        { -1, 0x01, 4, 5, 0, -1, 0, value_type0 },
#endif

    /*
        MUL 1, 2, 3
        SUB 4, 5, 1
            mad 4, -2, 3, 5, 0
    */
    { 2, gcSL_MUL, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUB, 4, 5, 1, 0, 0, _NoLabel },
        { -1, 0x02, 4, -2, 3, 5, 0 },

    /*
        MOV     1, 2
        TEXBIAS 0, 3, 1
        TEXLD   4, 3, 5
            mov 5.w, 0, 0, 2, 0
            texldb 4, 5, 0, 0, 3
    */
    { 3, gcSL_MOV, 1, 2, 0, 0, 0, _UseDestInNextTwoOnly }, /* TEXLD has a special dependcy on this MOV */
    { 2, gcSL_TEXBIAS, 0, 3, 1 },
    { 1, gcSL_TEXLD, 4, 3, 5, 0, 0, _HandleBiasedTextureLoad },
        { -2, 0x09, 5, 0, 0, 2, 0, enable_w },
        { -1, 0x19, 4, 5, 0, 0, 3, source0_add_swizzle_w },

    /*
        MOV     4, 5
        TEXBIAS 0, 2, 4
        TEXLD   1, 2, 3
            mov TEMP1_XYZW, 0, 0, 3, 0
            mov TEMP1_XYZW.w, 0, 0, 5, 0
            texldb 1, TEMP1_XYZW, 0, 0, 2
    */
    { 3, gcSL_MOV, 4, 5, 0, 0, 0, _UseDestInNextTwoOnly }, /* TEXLD has a special dependcy on this MOV */
    { 2, gcSL_TEXBIAS, 0, 2, 4 },
    { 1, gcSL_TEXLD, 1, 2, 3 },
        { -3, 0x09, gcSL_CG_TEMP1_XYZW, 0, 0, 3, 0 },
        { -2, 0x09, gcSL_CG_TEMP1_XYZW, 0, 0, 5, 0, enable_w },
        { -1, 0x19, 1, gcSL_CG_TEMP1_XYZW, 0, 0, 2 },

    /*
        MUL 1, 2, 3
            imul0 1, 2, 3, 0, 0
    */
    { 1, gcSL_MUL, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x3C, 1, 2, 3, 0, 0, value_type0 },

    /*
        DIV 1, 2, 3
            div 1, 2, 3, 0, 0
    */
    { 1, gcSL_DIV, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x44, 1, 2, 3, 0, 0, value_type0 },

    /*
        MUL 1, 2, 3
            mul 1, 2, 3, 0, 0
    */
    { 1, gcSL_MUL, 1, 2, 3 },
        { -1, 0x03, 1, 2, 3, 0, 0 },

	/*
        MULLO 1, 2, 3
            mul 1, 2, 3, 0, 0
    */
    { 1, gcSL_MULLO, 1, 2, 3 },
        { -1, 0x29, 1, 2, 3, 0, 0 },

    /*
        MOV 1, 2
            mov 1, 0, 0, 2, 0
    */
    { 1, gcSL_MOV, 1, 2 },
        { -1, 0x09, 1, 0, 0, 2, 0, mov },

    /*
        TEXBIAS 0, 2, 4
        TEXLD 1, 2, 3
            mov TEMP1_XYZW, 0, 0, 3, 0
            mov TEMP1_XYZW.w, 0, 0, 4, 0
            texldb 1, TEMP1_XYZW, 0, 0, 2
    */
    { 2, gcSL_TEXBIAS, 0, 2, 4 },
    { 1, gcSL_TEXLD, 1, 2, 3 },
        { -3, 0x09, gcSL_CG_TEMP1_XYZW, 0, 0, 3, 0 },
        { -2, 0x09, gcSL_CG_TEMP1_XYZW, 0, 0, 4, 0, enable_w },
        { -1, 0x19, 1, gcSL_CG_TEMP1_XYZW, 0, 0, 2 },

    /*
        TEXLD 1, 2, 3
            texld 1, 3, 0, 0, 2
    */
    { 1, gcSL_TEXLD, 1, 2, 3 },
        { -1, 0x18, 1, 3, 0, 0, 2 },

    /*
        TEXLDP 1, 2, 3
            rcp TEMP1, 0, 0, 3.<last>
            mul TEMP1, 3, TEMP1, 0
            texld 1, TEMP1, 0, 0, 2
    */
    { 1, gcSL_TEXLDP, 1, 2, 3 },
        { -3, 0x0C, gcSL_CG_TEMP1, 0, 0, 3, 0, swizzle2ZorW },
        { -2, 0x03, gcSL_CG_TEMP1, 3, gcSL_CG_TEMP1, 0, 0 },
        { -1, 0x18, 1, gcSL_CG_TEMP1, 0, 0, 2 },


    /*
        TEXBIAS 0, 2, 4
        TEXLDP  1, 2, 3
            rcp TEMP1, 0, 0, 3.<last>
            mul TEMP1, 3, TEMP1, 0
            mov TEMP1, 0, 0, 4
            texld 1, TEMP1, 0, 0, 2
    */
    { 2, gcSL_TEXBIAS, 0, 2, 4 },
    { 1, gcSL_TEXLDP, 1, 2, 3 },
        { -4, 0x0C, gcSL_CG_TEMP1, 0, 0, 3, 0, swizzle2ZorW },
        { -3, 0x03, gcSL_CG_TEMP1, 3, gcSL_CG_TEMP1, 0, 0 },
        { -2, 0x09, gcSL_CG_TEMP1, 0, 0, 4, 0, enable_w },
        { -1, 0x18, 1, gcSL_CG_TEMP1, 0, 0, 2 },

    /*
        DP3 1, 2, 3
            dp3 1, 2, 3, 0, 0
    */
    { 1, gcSL_DP3, 1, 2, 3 },
        { -1, 0x05, 1, 2, 3, 0, 0 },

    /*
        DP4 1, 2, 3
            dp4 1, 2, 3, 0, 0
    */
    { 1, gcSL_DP4, 1, 2, 3 },
        { -1, 0x06, 1, 2, 3, 0, 0 },

    /*
        DSX 1, 2
            dsx 1, 2, 0, 2, 0
    */
    { 1, gcSL_DSX, 1, 2 },
        { -1, 0x07, 1, 2, 0, 2, 0 },

    /*
        DSY 1, 2
            dsy 1, 2, 0, 2, 0
    */
    { 1, gcSL_DSY, 1, 2 },
        { -1, 0x08, 1, 2, 0, 2, 0 },

    /*
        FWIDTH 1, 2
            dsx TEMP1, 2, 0, 2, 0
            dsy TEMP2, 2, 0, 2, 0
            add 1, |TEMP1|, 0, |TEMP2|, 0
    */
    { 1, gcSL_FWIDTH, 1, 2 },
        { -3, 0x07, gcSL_CG_TEMP1, 2, 0, 2, 0 },
        { -2, 0x08, gcSL_CG_TEMP2, 2, 0, 2, 0 },
        { -1, 0x01, 1, gcSL_CG_TEMP1, 0, gcSL_CG_TEMP2, 0, abs_0_abs_2 },

    /*
        RCP   1, 2
        SQRT  3, 1
            rsq 3, 0, 0, 2, 0
    */
    { 2, gcSL_RCP, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SQRT, 3, 1 },
            { -1, 0x0D, 3, 0, 0, 2, 0 },

    /*
        RCP 1, 2
            rcp 1, 0, 0, 2, 0
    */
    { 1, gcSL_RCP, 1, 2 },
        { -1, 0x0C, 1, 0, 0, 2, 0 },

    /*
        SAT 1, 2
            mov.sat 1, 2, 0, 0, 0
    */
    { 1, gcSL_SAT, 1, 2 },
        { -1, 0x09, 1, 0, 0, 2, 0, saturate },

    /*
        MAX 1, 2, 3
            select.lt 1, 2, 3, 2, 0
    */
    { 1, gcSL_MAX, 1, 2, 3 },
        { -1, 0x0F, 1, 2, 3, 2, 0, conditionLT },

    /*
        MIN 1, 2, 3
            select.gt 1, 2, 3, 2, 0
    */
    { 1, gcSL_MIN, 1, 2, 3 },
        { -1, 0x0F, 1, 2, 3, 2, 0, conditionGT },

    /*
        This is a special pattern generated by the ES 1.1 driver for fog.

        ABS 1, 2
        MUL 3, 1, 4
        ADD 5, 3, 6
        SAT 7, 5
            mad.sat 7, |2|, 4, 6
    */
    { 4, gcSL_ABS, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 3, gcSL_MUL, 3, 1, 4, 0, 0, _UseDestInNextOnly },
    { 2, gcSL_ADD, 5, 3, 6, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SAT, 7, 5 },
        { -1, 0x02, 7, 2, 4, 6, 0, _SatAbs0 },

    /*
        ABS 1, 2
            select.lt 1, 2, -2, 2, 0
    */
    { 1, gcSL_ABS, 1, 2, 0, 0, 0, _IntOpcode },
        { -2, 0x01, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, 0, -2, 0, int_value_type0_const_0 },
        { -1, 0x0F, 1, 2, gcSL_CG_TEMP1, 2, 0, conditionLT },

    /*
        ABS 1, 2
        MIN 3, 1, 4
            select.gt 3, 4, |2|, 4, 0
    */
    { 2, gcSL_ABS, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_MIN, 3, 1, 4, 0, 0, _NoLabel },
        { -1, 0x0F, 3, 4, 2, 4, 0, conditionGTAbs1 },

    /*
        ABS 1, 2
        MIN 3, 4, 1
            select.gt 3, 4, |2|, 4, 0
    */
    { 2, gcSL_ABS, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_MIN, 3, 4, 1, 0, 0, _NoLabel },
        { -1, 0x0F, 3, 4, 2, 4, 0, conditionGTAbs1 },

    /*
        ABS 1, 2
        MAX 3, 1, 4
            select.lt 3, 4, |2|, 4, 0
    */
    { 2, gcSL_ABS, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_MAX, 3, 1, 4, 0, 0, _NoLabel },
        { -1, 0x0F, 3, 4, 2, 4, 0, conditionLTAbs1 },

    /*
        ABS 1, 2
        MAX 3, 4, 1
            select.lt 3, 4, |2|, 4, 0
    */
    { 2, gcSL_ABS, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_MAX, 3, 4, 1, 0, 0, _NoLabel },
        { -1, 0x0F, 3, 4, 2, 4, 0, conditionLTAbs1 },

    /*
        ABS 1, 2
            select.lt 1, 2, -2, 2, 0
    */
    { 1, gcSL_ABS, 1, 2 },
        { -1, 0x0F, 1, 2, -2, 2, 0, conditionLT },


    /*
        JMP.cond 1, 2, 3
        MOV      4, 5
        JMP      6
        MOV      4, 7
            set.cond   temp 2, 3
            select.NZ  4, temp, 5, 7, 0, conditionNZ
    */
    { 4, gcSL_JMP, 1, 2, 3, 0, 0, _jmpToNextPlusTwo },
    { 3, gcSL_MOV, 4, 5, 0, 0, 0, _NoLabel },
    { 2, gcSL_JMP, 6, 0, 0, 0, 0, _jmpToNextPlusOne },
    { 1, gcSL_MOV, 4, 7, 0 ,0, 0, _NoLabel },
        { -2, 0x10, gcSL_CG_TEMP1, 2, 3, 0, 0 },
        { -1, 0x0F, 4, gcSL_CG_TEMP1, 5, 7, 0, conditionNZ },

    /*
        JMP.cond 1, 2, 3
            branch.cond 0, 1, 2, 0, 0
    */
    { 1, gcSL_JMP, 0, 1, 2 },
        { -1, 0x16, 0, 1, 2, 0, 0, branch },

    /*
        CALL.cond 1, 2, 3
            call.cond 0, 1, 2, 0, 0
    */
    { 1, gcSL_CALL, 0, 1, 2 },
        { -1, 0x14, 0, 1, 2, 0, 0, branch },

    /*
        RET
            ret 0, 0, 0, 0, 0
    */
    { 1, gcSL_RET },
        { -1, 0x15, 0, 0, 0, 0, 0 },

    /*
        KILL.cond 1, 2
            texkill.cond 0, 1, 2, 0, 0
    */
    { 1, gcSL_KILL, 0, 1, 2 },
        { -1, 0x17, 0, 1, 2, 0, 0, kill },

    /*
        SET.cond 1, 2, 3
            set.cond 1, 2, 3, 0, 0
    */
    { 1, gcSL_SET, 1, 2, 3 },
        { -1, 0x10, 1, 2, 3, 0, 0 },

    /*
        NORM 1, 2
            dp3 TEMP1, 2, 2, 0, 0
            rsq TEMP1, 0, 0, TEMP1.x, 0
            mul 1, 2, TEMP1, 0, 0
    */
    { 1, gcSL_NORM, 1, 2 },
        { -3, 0x05, gcSL_CG_TEMP1, 2, 2, 0, 0 },
        { -2, 0x0D, gcSL_CG_TEMP1, 0, 0, gcSL_CG_TEMP1, 0, swizzle2X },
        { -1, 0x03, 1, 2, gcSL_CG_TEMP1, 0, 0 },

    /*
        CROSS 1, 2, 3
            mul 1, 2.zxy, 3.yzx, 0, 0
            mad 1, 3.zxy, 2.yzx, -1, 0
    */
    { 1, gcSL_CROSS, 1, 2, 3 },
        { -2, 0x03, 1, 2, 3, 0, 0, crossSwizzle },
        { -1, 0x02, 1, 3, 2, -1, 0, crossSwizzle },

    /* Use floor to implement fraction to work around fraction problem. */
    /*
        FRAC 1, 2
            floor 1, 0, 0, 2, 0
            add 1, 2, 0, -1, 0
    */
    { 1, gcSL_FRAC, 1, 2, 0, 0, 0, _hasSIGN_FLOOR_CEIL },
        { -2, 0x25, 1, 0, 0, 2, 0 },
        { -1, 0x01, 1, 2, 0, -1, 0 },

    /*
        FRAC 1, 2
            frc 1, 0, 0, 2, 0
    */
    { 1, gcSL_FRAC, 1, 2 },
        { -1, 0x13, 1, 0, 0, 2, 0 },

	/*
	GETEXP 1, 2
		getexp 1, 0, 0, 2, 0
    */
#if _HAS_GETEXPT_GETMANT_
    { 1, gcSL_GETEXP, 1, 2 },
        { -1, 0x35, 1, 0, 0, 2, 0 },
#else
    /* GC2100 and GC4000 do not have GETEXP. */
    { 1, gcSL_GETEXP, 1, 2 },
        { -3, 0x5D, gcSL_CG_TEMP1, 2, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_7F800000 },
        { -2, 0x5A, gcSL_CG_TEMP2, gcSL_CG_TEMP1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_23 },
        { -1, 0x01, 1, gcSL_CG_TEMP2, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_Minus127 },
#endif

	/*
	GETMANT 1, 2
		getmant 1, 0, 0, 2, 0
    */
#if _HAS_GETEXPT_GETMANT_
    { 1, gcSL_GETMANT, 1, 2 },
        { -1, 0x36, 1, 0, 0, 2, 0 },
#else
    /* GC2100 and GC4000 do not have GETMANT. */
    { 1, gcSL_GETMANT, 1, 2 },
        { -2, 0x5D, gcSL_CG_TEMP1, 2, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_7FFFFF },
        { -1, 0x5C, 1, gcSL_CG_TEMP1, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_800000 },
#endif

	/*
        MOD 1, 2, 3
            mod 1, 2, 3, 0, 0
    */
    { 1, gcSL_MOD, 1, 2, 3 },
        { -1, 0x48, 1, 2, 3, 0, 0, value_type0 },



    /*
        FLOOR 1, 2
            floor 1, 0, 0, 2
    */
    { 1, gcSL_FLOOR, 1, 2, 0, 0, 0, _hasSIGN_FLOOR_CEIL },
        { -1, 0x25, 1, 0, 0, 2 },

    /*
        FLOOR 1, 2
            frc 1, 0, 0, 2, 0
            add 1, 2, 0, -1, 0
    */
    { 1, gcSL_FLOOR, 1, 2 },
        { -2, 0x13, 1, 0, 0, 2, 0 },
        { -1, 0x01, 1, 2, 0, -1, 0 },


    /*
        SIGN 1, 2
            sign 1, 0, 0, 2
    */
    { 1, gcSL_SIGN, 1, 2, 0, 0, 0, _hasSIGN_FLOOR_CEIL_and_float_type },
       /* { -1, 0x27, 1, 0, 0, 2 },*/
		{ -1, 0x27, 1, 0, 0, 2, 0, value_type0 },

    /*
        SIGN 1, 2
            select.gz 1, 2, one, 2
            select.eq 1, 1, zero, 1
            select.lz 1, 1, -one, 2
    */
    { 1, gcSL_SIGN, 1, 2, 0, 0, 0, _is_value_type_float },
        { -3, 0x0F, 1, 2, gcSL_CG_CONSTANT, 2, 0, one_1_conditionGZ },
        { -2, 0x0F, 1, 2, gcSL_CG_CONSTANT, 1, 0, zero_1_conditionEQ },
        { -1, 0x0F, 1, 2, -gcSL_CG_CONSTANT, 1, 0, one_1_conditionLZ },

    /*
        SIGN 1, 2
            select.gz 1, 2, one, 2
            select.eq 1, 1, zero, 1
            select.lz 1, 1, -one, 2
    */
    { 1, gcSL_SIGN, 1, 2 },
        { -3, 0x0F, 1, 2, gcSL_CG_CONSTANT, 2, 0, int_one_1_conditionGZ },
        { -2, 0x0F, 1, 2, gcSL_CG_CONSTANT, 1, 0, int_zero_1_conditionEQ },
        { -1, 0x0F, 1, 2, gcSL_CG_CONSTANT, 1, 0, int_minus_one_1_conditionLZ },

    /*
        CEIL 1, 2
            ceil 1, 0, 0, 2
    */
    { 1, gcSL_CEIL, 1, 2, 0, 0, 0, _hasSIGN_FLOOR_CEIL },
        { -1, 0x26, 1, 0, 0, 2 },

    /*
        CEIL 1, 2
            frc 1, 0, 0, 2, 0
            add TEMP1_XYZW, one, 0, -1, 0
            select.gz 1, 1, TEMP1_XYZW, 1, 0
            add 1, 2, 0, 1, 0
    */
    { 1, gcSL_CEIL, 1, 2 },
        { -4, 0x13, 1, 0, 0, 2, 0 },
        { -3, 0x01, gcSL_CG_TEMP1_XYZW, gcSL_CG_CONSTANT, 0, -1, 0, one_0 },
        { -2, 0x0F, 1, 1, gcSL_CG_TEMP1_XYZW, 1, 0, conditionGZ },
        { -1, 0x01, 1, 2, 0, 1, 0 },

    /*
        STEP 1, 2, 3
            set.ge 1, 3, 2, 0, 0
    */
    { 1, gcSL_STEP, 1, 2, 3 },
        { -1, 0x10, 1, 3, 2, 0, 0, conditionGE },

    /*
        RSQ 1, 2
            rsq 1, 0, 0, 2, 0
    */
    { 1, gcSL_RSQ, 1, 2 },
        { -1, 0x0D, 1, 0, 0, 2, 0 },

	/*
        SQRT 1, 2
        RCP  3, 1
            rsq 3, 0, 0, 2, 0
    */
    { 2, gcSL_SQRT, 1, 2, 0, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_RCP, 3, 1 },
            { -1, 0x0D, 3, 0, 0, 2, 0 },

    /*
        SQRT 1, 2
            sqrt 1, 0, 0, 2, 0
    */
    { 1, gcSL_SQRT, 1, 2, 0, 0, 0, _hasSQRT_TRIG },
        { -1, 0x21, 1, 0, 0, 2, 0 },

    /*
        SQRT 1, 2
            rsq 1, 0, 0, 2, 0
            rcp 1, 0, 0, 1, 0
    */
    { 1, gcSL_SQRT, 1, 2 },
        { -2, 0x0D, 1, 0, 0, 2, 0 },
        { -1, 0x0C, 1, 0, 0, 1, 0 },

    /*
        LOG 1, 2
            --- log 1, 0, 0, 2, 0
            select.gz 1, 2, 2, smallest0, 0
            log 1, 0, 0, 1, 0
    */
    { 1, gcSL_LOG, 1, 2 },
        /*{ -1, 0x12, 1, 0, 0, 2, 0 },*/
        { -2, 0x0F, 1, 2, 2, gcSL_CG_CONSTANT, 0, smallest0_2_GZ },
        { -1, 0x12, 1, 0, 0, 1, 0 },

    /*
        EXP 1, 2
            exp 1, 0, 0, 2, 0
    */
    { 1, gcSL_EXP, 1, 2 },
        { -1, 0x11, 1, 0, 0, 2, 0 },

    /*
        POW 1, 2, 3
            --- log 1, 0, 0, 2, 0
            select.gz 1, 2, 2, smallest0, 0
            log 1, 0, 0, 1, 0
            mul 1, 1, 3, 0, 0
            exp 1, 0, 0, 1, 0
    */
    { 1, gcSL_POW, 1, 2, 3 },
        /*{ -3, 0x12, 1, 0, 0, 2, 0 },*/
        { -3, 0x12, 1, 0, 0, 2, 0 },
        { -2, 0x03, 1, 1, 3, 0, 0 },
        { -1, 0x11, 1, 0, 0, 1, 0 },

    /*
        SIN 1, 2
            mul 1, 2, rcppi2, 0
            sin 1, 0, 0, 1
    */
    { 1, gcSL_SIN, 1, 2, 0, 0, 0, _hasSQRT_TRIG },
        { -2, 0x03, 1, 2, gcSL_CG_CONSTANT, 0, 0, rcppi2_1 },
        { -1, 0x22, 1, 0, 0, 1 },

    /*
        SIN 1, 2
            mad TEMP1, 2, rcppi2, dot5
            frc TEMP1, TEMP1
            mad TEMP1, TEMP1, pi2, -pi
            mul TEMP2, TEMP1, TEMP1
            mad 1, TEMP2, factor9, -factor7
            mad 1, TEMP2, 1, factor5
            mad 1, TEMP2, 1, -factor3
            mad 1, TEMP2, 1, one
            mul 1, 1, TEMP1
    */
    { 1, gcSL_SIN, 1, 2 },
        { -9, 0x02, gcSL_CG_TEMP1, 2, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, rcppi2_1_dot5_2 },
        { -8, 0x13, gcSL_CG_TEMP1, 0, 0, gcSL_CG_TEMP1, 0 },
        { -7, 0x02, gcSL_CG_TEMP1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, pi2_1_pi_2 },
        { -6, 0x03, gcSL_CG_TEMP2, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, 0 },
        { -5, 0x02, 1, gcSL_CG_TEMP2, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, factor9_1_factor7_2 },
        { -4, 0x02, 1, gcSL_CG_TEMP2, 1, gcSL_CG_CONSTANT, 0, factor5_2 },
        { -3, 0x02, 1, gcSL_CG_TEMP2, 1, -gcSL_CG_CONSTANT, 0, factor3_2 },
        { -2, 0x02, 1, gcSL_CG_TEMP2, 1, gcSL_CG_CONSTANT, 0, one_2 },
        { -1, 0x03, 1, 1, gcSL_CG_TEMP1, 0, 0 },

    /*
        COS 1, 2
            mul 1, 2, rcppi2, 0
            cos 1, 0, 0, 1
    */
    { 1, gcSL_COS, 1, 2, 0, 0, 0, _hasSQRT_TRIG },
        { -2, 0x03, 1, 2, gcSL_CG_CONSTANT, 0, 0, rcppi2_1 },
        { -1, 0x23, 1, 0, 0, 1 },

    /*
        COS 1, 2
            mad TEMP1, 2, rcppi2, dot5
            frc TEMP1, TEMP1
            mad TEMP1, TEMP1, pi2, -pi
            mul TEMP1, TEMP1, TEMP1
            mad 1, TEMP1, factor8, -factor6
            mad 1, TEMP1, 1, factor4
            mad 1, TEMP1, 1, -factor2
            mad 1, TEMP1, 1, one
    */
    { 1, gcSL_COS, 1, 2 },
        { -8, 0x02, gcSL_CG_TEMP1, 2, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, rcppi2_1_dot5_2 },
        { -7, 0x13, gcSL_CG_TEMP1, 0, 0, gcSL_CG_TEMP1, 0 },
        { -6, 0x02, gcSL_CG_TEMP1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, pi2_1_pi_2 },
        { -5, 0x03, gcSL_CG_TEMP1, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, 0 },
        { -4, 0x02, 1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, factor8_1_factor6_2 },
        { -3, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, factor4_2 },
        { -2, 0x02, 1, gcSL_CG_TEMP1, 1, -gcSL_CG_CONSTANT, 0, factor2_2 },
        { -1, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, one_2 },

    /*
        TAN 1, 2
            mul temp1, 2, rcppi2, 0
            cos temp2, 0, 0, temp1
            sin temp3, 0, 0, temp1
            rcp 1, temp2
            mul 1, 1, temp3 // tan(x) = sin(x)/cos(x)
    */
    { 1, gcSL_TAN, 1, 2, 0, 0, 0, _hasSQRT_TRIG },
        { -5, 0x03, gcSL_CG_TEMP1, 2, gcSL_CG_CONSTANT, 0, 0, rcppi2_1 },
        { -4, 0x23, gcSL_CG_TEMP2, 0, 0, gcSL_CG_TEMP1},
        { -3, 0x22, gcSL_CG_TEMP3, 0, 0, gcSL_CG_TEMP1},
        { -2, 0x0C, 1, 0, 0, gcSL_CG_TEMP2, 0, },
        { -1, 0x03, 1, gcSL_CG_TEMP3, 1, 0, 0, },
    /*
        TAN 1, 2
            mad TEMP1, 2, rcppi2, dot5
            frc TEMP1, TEMP1
            mad TEMP1, TEMP1, pi2, -pi

            tan(x) = x + 1/3 x^3 + 2/15 x^5 + 17/315 x^7 + 62/2835 x^9
                   = x (1 + x^2 (1/3 + x^2 (2/15 + x^2 (17/315 + 62/2835 x^2 ) ) ) )

            mul TEMP2, TEMP1, TEMP1
            mad 1, TEMP2, tan9, tan7
            mad 1, TEMP2, 1, tan5
            mad 1, TEMP2, 1, tan3
            mad 1, TEMP2, 1, one
            mul 1, TEMP1, 1
    */
    { 1, gcSL_TAN, 1, 2 },
        { -9, 0x02, gcSL_CG_TEMP1, 2, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, rcppi2_1_dot5_2 },
        { -8, 0x13, gcSL_CG_TEMP1, 0, 0, gcSL_CG_TEMP1, 0 },
        { -7, 0x02, gcSL_CG_TEMP1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, pi2_1_pi_2 },
        { -6, 0x03, gcSL_CG_TEMP2, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, 0 },
        { -5, 0x02, 1, gcSL_CG_TEMP2, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, tan9_1_tan7_2 },
        { -4, 0x02, 1, gcSL_CG_TEMP2, 1, gcSL_CG_CONSTANT, 0, tan5_2 },
        { -3, 0x02, 1, gcSL_CG_TEMP2, 1, gcSL_CG_CONSTANT, 0, tan3_2 },
        { -2, 0x02, 1, gcSL_CG_TEMP2, 1, gcSL_CG_CONSTANT, 0, one_2 },
        { -1, 0x03, 1, gcSL_CG_TEMP1, 1, 0, 0, },

    /*
        ACOS 1, 2
            ACOS(x) = 1/2 pi - (x + 1/6 x^3 + 3/40 x^5 + 5/112 x^7 + 35/1152 x^9)
                    = 1/2 pi - (x (1 + x^2 (1/6 + x^2 (3/40 + x^2 (5/112 + 35/1152 x^2)))))

            mul TEMP1, 2, 2
            mad 1, TEMP1, asin9, asin7
            mad 1, TEMP1, 1, asin5
            mad 1, TEMP1, 1, asin3
            mad 1, TEMP1, 1, one
            mad 1, 2, -1, half_pi
    */
    { 1, gcSL_ACOS, 1, 2 },
        { -6, 0x03, gcSL_CG_TEMP1, 2, 2, 0, 0 },
        { -5, 0x02, 1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, asin9_1_asin7_2 },
        { -4, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, asin5_2 },
        { -3, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, asin3_2 },
        { -2, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, one_2 },
        { -1, 0x02, 1, 2, -1, gcSL_CG_CONSTANT, 0, half_pi_2 },

    /*
        ASIN 1, 2
            ASIN(x) = x + 1/6 x^3 + 3/40 x^5 + 5/112 x^7 + 35/1152 x^9
                    = x (1 + x^2 (1/6 + x^2 (3/40 + x^2 (5/112 + 35/1152 x^2))))

            mul TEMP1, 2, 2
            mad 1, TEMP1, asin9, asin7
            mad 1, TEMP1, 1, asin5
            mad 1, TEMP1, 1, asin3
            mad 1, TEMP1, 1, one
            mul 1, 2, 1
    */
    { 1, gcSL_ASIN, 1, 2 },
        { -6, 0x03, gcSL_CG_TEMP1, 2, 2, 0, 0 },
        { -5, 0x02, 1, gcSL_CG_TEMP1, gcSL_CG_CONSTANT, gcSL_CG_CONSTANT, 0, asin9_1_asin7_2 },
        { -4, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, asin5_2 },
        { -3, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, asin3_2 },
        { -2, 0x02, 1, gcSL_CG_TEMP1, 1, gcSL_CG_CONSTANT, 0, one_2 },
        { -1, 0x03, 1, 2, 1, 0, 0 },

    /*
        ATAN 1, 2

            if (|x| > 1) flag = 1; x = 1 / x; else flag = 0;

                set.gt TEMP1, |x|, 1
                rcp TEMP2, x
                select.nz TEMP2, TEMP1, TEMP2, x

            atan(x) = x - 1/3 x^3 + 1/5 x^5 - 1/7 x^7 + 1/9 x^9
                    = x (1 + x^2 (-1/3 + x^2 (1/5 + x^2 (-1/7 + 1/9 x^2 ) ) ) )

                mul TEMP3, TEMP2, TEMP2
                mad 1, TEMP3, atan9, -atan7
                mad 1, TEMP3, 1, atan5
                mad 1, TEMP3, 1, -atan3
                mad 1, TEMP3, 1, one
                mul 1, TEMP2, 1, 0

            if (x < 0) t2 = -pi/2 - abs(atan); else t2 = pi/2 - abs(atan);

                add TEMP2, PI/2, 0, |atan|
                select.lt TEMP2, x, -TEMP2, TEMP2

            return flag ? t2 : atan;

                select.nz 1, TEMP1, TEMP2, 1

    */
    { 1, gcSL_ATAN, 1, 2 },
        { -12, 0x10, gcSL_CG_TEMP1, 2, gcSL_CG_CONSTANT, 0, 0, gt_abs_0_one_1 },
        { -11, 0x0C, gcSL_CG_TEMP2, 0, 0, 2, 0, },
        { -10, 0x0F, gcSL_CG_TEMP2, gcSL_CG_TEMP1, gcSL_CG_TEMP2, 2, 0, conditionNZ },
        { -9, 0x03, gcSL_CG_TEMP3, gcSL_CG_TEMP2, gcSL_CG_TEMP2, 0, 0, },
        { -8, 0x02, 1, gcSL_CG_TEMP3, gcSL_CG_CONSTANT, -gcSL_CG_CONSTANT, 0, atan9_1_atan7_2 },
        { -7, 0x02, 1, gcSL_CG_TEMP3, 1, gcSL_CG_CONSTANT, 0, atan5_2 },
        { -6, 0x02, 1, gcSL_CG_TEMP3, 1, -gcSL_CG_CONSTANT, 0, atan3_2 },
        { -5, 0x02, 1, gcSL_CG_TEMP3, 1, gcSL_CG_CONSTANT, 0, one_2 },
        { -4, 0x03, 1, gcSL_CG_TEMP2, 1, 0, 0, },
        { -3, 0x01, gcSL_CG_TEMP2, gcSL_CG_CONSTANT, 0, -1, 0, halfpi_0_abs_2 },
        { -2, 0x0F, gcSL_CG_TEMP2, 2, -gcSL_CG_TEMP2, gcSL_CG_TEMP2, 0, conditionLT },
        { -1, 0x0F, 1, gcSL_CG_TEMP1, gcSL_CG_TEMP2, 1, 0, conditionNZ },

	/*
		LSHIFT 1, 2, 3
			lshift 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_LSHIFT, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x59, 1, 2, 0, 3, 0, value_type0 },


	/*
		RSHIFT 1, 2, 3
			rshift 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_RSHIFT, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x5A, 1, 2, 0, 3, 0, value_type0 },


	/*
		ROTATE 1, 2, 3
			rotate 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_ROTATE, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x5B, 1, 2, 0, 3, 0, value_type0 },


	/*
		gcSL_AND_BITWISE 1, 2, 3
			and 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_AND_BITWISE, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x5D, 1, 2, 0, 3, 0, value_type0 },


	/*
		gcSL_OR_BITWISE 1, 2, 3
			or 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_OR_BITWISE, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x5C, 1, 2, 0, 3, 0, value_type0 },


	/*
		gcSL_XOR_BITWISE 1, 2, 3
			xor 1, 2, 0, 3, 0
	*/
	{ 1, gcSL_XOR_BITWISE, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x5E, 1, 2, 0, 3, 0, value_type0 },


	/*
		gcSL_NOT_BITWISE 1, 2
			not 1, 0, 0, 2, 0
	*/
	{ 1, gcSL_NOT_BITWISE, 1, 2, 0, 0, 0, _hasCL },
		{ -1, 0x5F, 1, 0, 0, 2, 0, value_type0 },


	/*
		LOAD 1, 2, 3
			load 1, 2, 3, 0, 0
	*/
	{ 1, gcSL_LOAD, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x32, 1, 2, 3, 0, 0, value_type0 },

	/*
		STORE 1, 2, 3
			store 0, 2, 3, 1, 0
	*/
	{ 1, gcSL_STORE, 1, 2, 3, 0, 0, _isGC2100Signed_8_16_store },
		{ -3, 0x59, gcSL_CG_TEMP1, 1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -2, 0x5A, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -1, 0x33, gcSL_CG_TEMP1, 2, 3, gcSL_CG_TEMP1, 0, value_type0 },

	/*
		STORE 1, 2, 3
			store 0, 2, 3, 1, 0
	*/
	{ 1, gcSL_STORE, 1, 2, 3, 0, 0, _isGC2100Unsigned_8_16_store },
		{ -2, 0x5D, gcSL_CG_TEMP1, 1, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_FF_FFFF },
		{ -1, 0x33, gcSL_CG_TEMP1, 2, 3, gcSL_CG_TEMP1, 0, value_type0 },

	/*
		STORE 1, 2, 3
			store 0, 2, 3, 1, 0
	*/
	{ 1, gcSL_STORE, 1, 2, 3, 0, 0, _hasCL },
		{ -1, 0x33, 1, 2, 3, 1, 0, value_type0 },

	/*
        ADD    1, 2, 3
        STORE1 4, 1, 5
			store 0, 2, 3, 1, 0
	*/
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
	{ 1, gcSL_STORE1, 4, 1, 5, 0, 0, _NoLabel_isGC2100Signed_8_16_store },
		{ -3, 0x59, gcSL_CG_TEMP1, 5, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -2, 0x5A, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -1, 0x33, 4, 2, 3, gcSL_CG_TEMP1, 0, value_type0_from_next_inst },

	/*
        ADD    1, 2, 3
        STORE1 4, 1, 5
			store 0, 2, 3, 1, 0
	*/
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
	{ 1, gcSL_STORE1, 4, 1, 5, 0, 0, _NoLabel_isGC2100Unsigned_8_16_store },
		{ -2, 0x5D, gcSL_CG_TEMP1, 5, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_FF_FFFF },
		{ -1, 0x33, 4, 2, 3, gcSL_CG_TEMP1, 0, value_type0_from_next_inst },

    /*
        ADD    1, 2, 3
        STORE1 4, 1, 5
            store 0, 2, 3, 4, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_STORE1, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x33, 4, 2, 3, 5, 0, value_type0_from_next_inst },

	/*
		STORE1 1, 2, 3
			store 0, 2, constant_0, 3, 0
	*/
	{ 1, gcSL_STORE1, 1, 2, 3, 0, 0, _isGC2100Signed_8_16_store },
		{ -3, 0x59, gcSL_CG_TEMP1, 3, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -2, 0x5A, gcSL_CG_TEMP1, gcSL_CG_TEMP1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -1, 0x33, 1, 2, gcSL_CG_CONSTANT, gcSL_CG_TEMP1, 0, value_type0 },

	/*
		STORE1 1, 2, 3
			store 0, 2, constant_0, 3, 0
	*/
	{ 1, gcSL_STORE1, 1, 2, 3, 0, 0, _isGC2100Unsigned_8_16_store },
		{ -2, 0x5D, gcSL_CG_TEMP1, 3, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_FF_FFFF },
		{ -1, 0x33, 1, 2, gcSL_CG_CONSTANT, gcSL_CG_TEMP1, 0, value_type0 },

    /*
        STORE1 1, 2, 3
            store 0, 2, constant_0, 3, 0
    */
    { 1, gcSL_STORE1, 1, 2, 3 },
        { -1, 0x33, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        MULHI 1, 2, 3
            imulhi0 1, 2, 3, 0, 0
    */
    { 1, gcSL_MULHI, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x40, 1, 2, 3, 0, 0, value_type0 },

    /*
        CMP 1, 2, 3
            cmp 1, 2, 3, minus_one, 0
    */
    { 1, gcSL_CMP, 1, 2, 3 },
        { -1, 0x31, 1, 2, 3, gcSL_CG_CONSTANT, 0, minusOne_2_value_type0_from_src0 },

	/*
		LEADZERO 1, 2, 3
			leadzero 1, 2, 3, 0, 0
	*/
	{ 1, gcSL_LEADZERO, 1, 2, 0, 0, 0, _IntOpcode },
		{ -1, 0x58, 1, 0, 0, 2, 0, value_type0 },


    /*
        CONV 1, 2
            lshift 1, 2, 0, constant_24_or_16, 0
            rshift 1, 1, 0, constant_24_or_16, 0
    */
    { 1, gcSL_CONV, 1, 2, 0, 0, 0, _isGC2100Signed_8_16 },
		{ -2, 0x59, 1, 2, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },
		{ -1, 0x5A, 1, 1, 0, gcSL_CG_CONSTANT, 0, int_value_type0_const_24_16 },

    /*
        CONV 1, 2
            and 1, 2, 0, constant_FF_or_FFFF, 0
    */
    { 1, gcSL_CONV, 1, 2, 0, 0, 0, _isGC2100Unsigned_8_16 },
		{ -1, 0x5D, 1, 2, 0, gcSL_CG_CONSTANT, 0, uint_value_type0_const_FF_FFFF },

    /*
        CONV 1, 2
            mov 1, 0, 0, 2, 0
    */
    { 1, gcSL_CONV, 1, 2, 0, 0, 0 },
        { -1, 0x09, 1, 0, 0, 2, 0 },

    /*
        I2F 1, 2
            I2F 1, 0, 0, 2, 0
    */
    { 1, gcSL_I2F, 1, 2, 0, 0, 0, _hasCL },
        { -1, 0x2D, 1, 2, 0, 0, 0, value_type0_from_src0 },

    /*
        F2I 1, 2, 3
            F2I 1, 2, 3, 0, 0
    */
    { 1, gcSL_F2I, 1, 2, 3, 0, 0, _hasCL },
        { -1, 0x2E, 1, 2, 3, 0, 0, value_type0 },

    /*
        BARRIER 1
            barrier 0, 0, 0, 0, 0
    */
    { 1, gcSL_BARRIER },
        { -1, 0x2A, 0, 0, 0, 0, 0 },

    /*
        MULSAT 1, 2, 3
        ADDSAT 4, 5, 1
            imadsat 4, 2, 3, 5, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ADDSAT, 4, 5, 1, 0, 0, _NoLabel },
		{ -1, 0x4E, 4, 2, 3, 5, 0, value_type0 },

    /*
        MULSAT 1, 2, 3
        ADDSAT 4, 1, 5
            imadsat 4, 2, 3, 5, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ADDSAT, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x4E, 4, 2, 3, 5, 0, value_type0 },

    /*
	temporary use, MULSAD+MULSAD = MAD0 | MAD1, for short-integer
        MULSAT 1, 2, 3
        MULSAT 4, 1, 5
            imadsat0 temp1, 2, 3, 5, 0
			imadsat1 4, 2, 3, 5, 0
			add 4, temp1, 4

    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_MULSAT, 4, 1, 5, 0, 0, _NoLabel },
        { -3, 0x4E, gcSL_CG_TEMP1, 2, 3, 5, 0, value_type0 },
		{ -2, 0x4F, 4, 2, 3, 5, 0, value_type0 },
		{ -1, 0x5C, 4, 4, 0, gcSL_CG_TEMP1, 0, value_type0 },

#if _SUPPORT_INTEGER_NEGATIVE_MODIFIER_
    /*
        MULSAT 1, 2, 3
        SUBSAT 4, 1, 5
            imadsat 4, 2, 3, -5, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUBSAT, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x4E, 4, 2, 3, -5, 0, value_type0},
#else
    /* Negative modifier is not allowed in imadlo0. */
    /*
        MULSAT 1, 2, 3
        SUBSAT 4, 1, 5
            imulsat 1, 2, 3, 0, 0
            iaddsat 4, 1, 0, -5, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUBSAT, 4, 1, 5, 0, 0, _NoLabel },
        { -2, 0x3E, 1, 2, 3, 0, 0, value_type0 },
        { -1, 0x3B, 4, 1, 0, -5, 0, value_type0 },
#endif

#if _SUPPORT_INTEGER_NEGATIVE_MODIFIER_
    /*
        MULSAT 1, 2, 3
        SUBSAT 4, 5, 1
            imadsat 4, -2, 3, 5, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUBSAT, 4, 5, 1, 0, 0, _NoLabel },
        { -1, 0x4E, 4, -2, 3, 5, 0, value_type0 },
#else
    /* Negative modifier is not allowed in imadlo0. */
    /*
        MULSAT 1, 2, 3
        SUBSAT 4, 5, 1
            imulsat 1, 2, 3, 0, 0
            iaddsat 4, 5, 0, -1, 0
    */
    { 2, gcSL_MULSAT, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_SUBSAT, 4, 5, 1, 0, 0, _NoLabel },
        { -2, 0x3E, 1, 2, 3, 0, 0, value_type0 },
        { -1, 0x3B, 4, 5, 0, -1, 0, value_type0 },
#endif

    /*
        MULSAT 1, 2, 3
            imulsat 1, 2, 3, 0, 0
    */
    { 1, gcSL_MULSAT, 1, 2, 3 },
        { -1, 0x3E, 1, 2, 3, 0, 0 },

    { 1, gcSL_ADDSAT, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x3B, 1, 2, 0, 3, 0, value_type0 },
    /*
        ADDSAT 1, 2, 3
            iaddsat 1, 2, 0, 3, 0
    */
    { 1, gcSL_ADDSAT, 1, 2, 3 },
        { -1, 0x3B, 1, 2, 0, 3, 0, add2mad },

    { 1, gcSL_SUBSAT, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x3B, 1, 2, 0, -3, 0, value_type0 },
    /*
        SUBSAT 1, 2, 3
            iaddsat 1, 2, 0, -3, 0
    */
    { 1, gcSL_SUBSAT, 1, 2, 3 },
        { -1, 0x3B, 1, 2, 0, -3, 0 },

    /*
        MULHI 1, 2, 3
        ADD 4, 1, 5
            imadhi 4, 2, 3, 5, 0
    */
    { 2, gcSL_MULHI, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ADD, 4, 1, 5, 0, 0, _NoLabel },
        { -1, 0x50, 4, 2, 3, 5, 0, value_type0 },

#ifdef AQ_INST_OP_CODE_ATOM_ADD
    /*
        ADD 1, 2, 3
        ATOMADD 4, 1, 5
            atom_add 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMADD, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_ADD, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMADD 1, 2, 3
            atom_add 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMADD, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_ADD, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMSUB 4, 1, 5
            atom_add 4, 2, 3, -5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMSUB, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_ADD, 4, 2, 3, -5, 0, value_type0 },

    /*
        ATOMSUB 1, 2, 3
            atom_add 1, 2, constant_0, -3, 0
    */
    { 1, gcSL_ATOMSUB, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_ADD, 1, 2, gcSL_CG_CONSTANT, -3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMXCHG 4, 1, 5
            atom_xchg 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMXCHG, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_XCHG, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMXCHG 1, 2, 3
            atom_xchg 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMXCHG, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_XCHG, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMCMPXCHG 4, 1, 5
            atom_cmpxchg 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMCMPXCHG, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_CMP_XCHG, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMCMPXCHG 1, 2, 3
            atom_cmpxchg 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMCMPXCHG, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_CMP_XCHG, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMMIN 4, 1, 5
            atom_min 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMMIN, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_MIN, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMMIN 1, 2, 3
            atom_min 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMMIN, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_MIN, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMMAX 4, 1, 5
            atom_max 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMMAX, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_MAX, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMMAX 1, 2, 3
            atom_max 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMMAX, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_MAX, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMOR 4, 1, 5
            atom_or 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMOR, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_OR, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMOR 1, 2, 3
            atom_or 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMOR, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_OR, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMAND 4, 1, 5
            atom_and 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMAND, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_AND, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMAND 1, 2, 3
            atom_and 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMAND, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_AND, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },

    /*
        ADD 1, 2, 3
        ATOMXOR 4, 1, 5
            atom_xor 4, 2, 3, 5, 0
    */
    { 2, gcSL_ADD, 1, 2, 3, 0, 0, _UseDestInNextOnly },
    { 1, gcSL_ATOMXOR, 4, 1, 5, 0, 0, _NoLabel },
        { -1, AQ_INST_OP_CODE_ATOM_XOR, 4, 2, 3, 5, 0, value_type0 },

    /*
        ATOMXOR 1, 2, 3
            atom_xor 1, 2, constant_0, 3, 0
    */
    { 1, gcSL_ATOMXOR, 1, 2, 3, 0, 0 },
        { -1, AQ_INST_OP_CODE_ATOM_XOR, 1, 2, gcSL_CG_CONSTANT, 3, 0, value_type0_const_0 },
#endif

    /*
        ADD 1, 2, 3
            add 1, 2, 0, 3, 0
    */
    { 1, gcSL_ADD, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x01, 1, 2, 0, 3, 0, value_type0 },

    /*
        ADD 1, 2, 3
            add 1, 2, 0, 3, 0
    */
    { 1, gcSL_ADD, 1, 2, 3 },
        { -1, 0x01, 1, 2, 0, 3, 0, add2mad },

	/*
        ADDLO 1, 2, 3
            add 1, 2, 0, 3, 0
    */
    { 1, gcSL_ADDLO, 1, 2, 3 },
        { -1, 0x28, 1, 2, 0, 3, 0, add2mad },

    /*
        SUB 1, 2, 3
            add 1, 2, 0, -3, 0
    */
    { 1, gcSL_SUB, 1, 2, 3, 0, 0, _IntOpcode },
        { -1, 0x01, 1, 2, 0, -3, 0, value_type0 },

    /*
        SUB 1, 2, 3
            add 1, 2, 0, -3, 0
    */
    { 1, gcSL_SUB, 1, 2, 3 },
        { -1, 0x01, 1, 2, 0, -3, 0 },

	/*
		End of list.
	*/
	{ 0 }
};

#endif /*VIVANTE_NO_3D*/



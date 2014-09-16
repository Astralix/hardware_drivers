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
**	Shader disassembler.
*/

#include "gc_hal_user_hardware_precomp.h"

#ifndef VIVANTE_NO_3D

#include "gc_hal_user_compiler.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
static void
_DebugSwizzle(
	IN char * Buffer,
	IN gctSIZE_T BufferSize,
	IN OUT gctUINT * Offset,
	IN gctUINT32 Swizzle
	)
{
	static const char _swizzle[] = "xyzw";

	gctUINT32 swizzle[4];
	swizzle[0] = (Swizzle >> 0) & 3;
	swizzle[1] = (Swizzle >> 2) & 3;
	swizzle[2] = (Swizzle >> 4) & 3;
	swizzle[3] = (Swizzle >> 6) & 3;

	/* Only decode swizzle if not .xyzw. */
	if ((swizzle[0] != 0)
	||	(swizzle[1] != 1)
	||	(swizzle[2] != 2)
	||	(swizzle[3] != 3)
	)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
							   ".%c", _swizzle[swizzle[0]]));

		/* Continue if swizzle is 2 or more components. */
		if ((swizzle[1] != swizzle[0])
		||  (swizzle[2] != swizzle[0])
		||  (swizzle[3] != swizzle[0])
		)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
									"%c", _swizzle[swizzle[1]]));

			/* Continue if swizzle is 3 or more components. */
			if ((swizzle[2] != swizzle[1])
			||  (swizzle[3] != swizzle[1])
			)
			{
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
									   "%c", _swizzle[swizzle[2]]));

				/* Continue if swizzle is 4 components. */
				if (swizzle[3] != swizzle[2])
				{
					gcmVERIFY_OK(
						gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
										   "%c", _swizzle[swizzle[3]]));
				}
			}
		}
	}
}

static void
_DebugAddressing(
	IN char * Buffer,
	IN gctSIZE_T BufferSize,
	IN OUT gctUINT * Offset,
	IN gctUINT32 Relative
	)
{
	static const char * _relative[] =
	{
		"",
		"[a0.x]",
		"[a0.y]",
		"[a0.z]",
		"[a0.w]",
		"[aL]",
	};

	/* Decode addressing. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, _relative[Relative]));
}

static void
_DebugRegister(
	IN char * Buffer,
	IN gctSIZE_T BufferSize,
	IN OUT gctUINT * Offset,
	IN gctUINT32 Type,
	IN gctUINT32 Address,
	IN gctUINT32 Relative,
	IN gctUINT32 Swizzle,
	IN gctUINT32 Negate,
	IN gctUINT32 Absolute
	)
{
	static const char _type[] = "rfcc";

	if (Negate)
	{
		/* Negate prefix. */
		gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "-"));
	}

	if (Absolute)
	{
		/* Absolute prefix. */
		gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "|"));
	}

	/* Decode register type and address. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
						   "%c%u", _type[Type], Address));

	/* Decode addressing. */
	_DebugAddressing(Buffer, BufferSize, Offset, Relative);

	/* Decode swizzle. */
	_DebugSwizzle(Buffer, BufferSize, Offset, Swizzle);

	if (Absolute)
	{
		/* Absolute postfix. */
		gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "|"));
	}
}

static void
_DebugShader(
	IN gctUINT32 Address,
	IN gctUINT32 * States,
    IN gctBOOL OutputFormat
	)
{
	char buffer[96];
	gctUINT offset = 0;
	const gctUINT column = 24;

	/* Get instruction fields. */
	gctUINT32 opcode          = (((((gctUINT32) (States[0])) >> (0 ? 5:0)) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1)))))) );
	gctUINT32 condition       = (((((gctUINT32) (States[0])) >> (0 ? 10:6)) & ((gctUINT32) ((((1 ? 10:6) - (0 ? 10:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:6) - (0 ? 10:6) + 1)))))) );
	gctUINT32 saturate        = (((((gctUINT32) (States[0])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) );
	gctUINT32 dest_valid      = (((((gctUINT32) (States[0])) >> (0 ? 12:12)) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) );
	gctUINT32 dest_rel        = (((((gctUINT32) (States[0])) >> (0 ? 15:13)) & ((gctUINT32) ((((1 ? 15:13) - (0 ? 15:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:13) - (0 ? 15:13) + 1)))))) );
	gctUINT32 dest_addr       = (((((gctUINT32) (States[0])) >> (0 ? 22:16)) & ((gctUINT32) ((((1 ? 22:16) - (0 ? 22:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:16) - (0 ? 22:16) + 1)))))) );
	gctUINT32 dest_enable	  = (((((gctUINT32) (States[0])) >> (0 ? 26:23)) & ((gctUINT32) ((((1 ? 26:23) - (0 ? 26:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:23) - (0 ? 26:23) + 1)))))) );
	gctUINT32 sampler_num     = (((((gctUINT32) (States[0])) >> (0 ? 31:27)) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1)))))) );
	gctUINT32 sampler_rel     = (((((gctUINT32) (States[1])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
	gctUINT32 sampler_swizzle = (((((gctUINT32) (States[1])) >> (0 ? 10:3)) & ((gctUINT32) ((((1 ? 10:3) - (0 ? 10:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:3) - (0 ? 10:3) + 1)))))) );
	gctUINT32 src0_valid      = (((((gctUINT32) (States[1])) >> (0 ? 11:11)) & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1)))))) );
	gctUINT32 src0_addr       = (((((gctUINT32) (States[1])) >> (0 ? 20:12)) & ((gctUINT32) ((((1 ? 20:12) - (0 ? 20:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:12) - (0 ? 20:12) + 1)))))) );
	gctUINT32 src0_swizzle    = (((((gctUINT32) (States[1])) >> (0 ? 29:22)) & ((gctUINT32) ((((1 ? 29:22) - (0 ? 29:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:22) - (0 ? 29:22) + 1)))))) );
	gctUINT32 src0_neg        = (((((gctUINT32) (States[1])) >> (0 ? 30:30)) & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1)))))) );
	gctUINT32 src0_abs        = (((((gctUINT32) (States[1])) >> (0 ? 31:31)) & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1)))))) );
	gctUINT32 src0_rel        = (((((gctUINT32) (States[2])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
	gctUINT32 src0_type       = (((((gctUINT32) (States[2])) >> (0 ? 5:3)) & ((gctUINT32) ((((1 ? 5:3) - (0 ? 5:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:3) - (0 ? 5:3) + 1)))))) );
	gctUINT32 src1_valid      = (((((gctUINT32) (States[2])) >> (0 ? 6:6)) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) );
	gctUINT32 src1_addr       = (((((gctUINT32) (States[2])) >> (0 ? 15:7)) & ((gctUINT32) ((((1 ? 15:7) - (0 ? 15:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:7) - (0 ? 15:7) + 1)))))) );
	gctUINT32 src1_swizzle    = (((((gctUINT32) (States[2])) >> (0 ? 24:17)) & ((gctUINT32) ((((1 ? 24:17) - (0 ? 24:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:17) - (0 ? 24:17) + 1)))))) );
	gctUINT32 src1_neg        = (((((gctUINT32) (States[2])) >> (0 ? 25:25)) & ((gctUINT32) ((((1 ? 25:25) - (0 ? 25:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:25) - (0 ? 25:25) + 1)))))) );
	gctUINT32 src1_abs        = (((((gctUINT32) (States[2])) >> (0 ? 26:26)) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))) );
	gctUINT32 src1_rel        = (((((gctUINT32) (States[2])) >> (0 ? 29:27)) & ((gctUINT32) ((((1 ? 29:27) - (0 ? 29:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:27) - (0 ? 29:27) + 1)))))) );
	gctUINT32 opcode_msb6     = (((((gctUINT32) (States[2])) >> (0 ? 16:16)) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) );
	gctUINT32 src1_type       = (((((gctUINT32) (States[3])) >> (0 ? 2:0)) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1)))))) );
	gctUINT32 src2_valid      = (((((gctUINT32) (States[3])) >> (0 ? 3:3)) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) );
	gctUINT32 src2_addr       = (((((gctUINT32) (States[3])) >> (0 ? 12:4)) & ((gctUINT32) ((((1 ? 12:4) - (0 ? 12:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:4) - (0 ? 12:4) + 1)))))) );
	gctUINT32 src2_swizzle    = (((((gctUINT32) (States[3])) >> (0 ? 21:14)) & ((gctUINT32) ((((1 ? 21:14) - (0 ? 21:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:14) - (0 ? 21:14) + 1)))))) );
	gctUINT32 src2_neg        = (((((gctUINT32) (States[3])) >> (0 ? 22:22)) & ((gctUINT32) ((((1 ? 22:22) - (0 ? 22:22) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 22:22) - (0 ? 22:22) + 1)))))) );
	gctUINT32 src2_abs        = (((((gctUINT32) (States[3])) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) );
	gctUINT32 src2_rel        = (((((gctUINT32) (States[3])) >> (0 ? 27:25)) & ((gctUINT32) ((((1 ? 27:25) - (0 ? 27:25) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:25) - (0 ? 27:25) + 1)))))) );
	gctUINT32 src2_type       = (((((gctUINT32) (States[3])) >> (0 ? 30:28)) & ((gctUINT32) ((((1 ? 30:28) - (0 ? 30:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:28) - (0 ? 30:28) + 1)))))) );
	gctUINT32 target          = (((((gctUINT32) (States[3])) >> (0 ? 18:7)) & ((gctUINT32) ((((1 ? 18:7) - (0 ? 18:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:7) - (0 ? 18:7) + 1)))))) );

	static const char * _opcode[] =
	{
		"nop",
		"add",
 		"mad",
		"mul",
		"dst",
		"dp3",
		"dp4",
		"dsx",
		"dsy",
		"mov",
		"movar",
		"movaf",
		"rcp",
		"rsq",
		"litp",
		"select",
		"set",
		"exp",
		"log",
		"frc",
		"call",
		"ret",
		"branch",
		"texkill",
		"texld",
		"texldb",
		"texldd",
		"texldl",
		"texldpcf",
		"rep",
		"endrep",
		"loop",
		"endloop",
		"sqrt",
		"sin",
		"cos",
		"poly",
		"floor",
		"ceil",
		"sign",
        "addlo",
        "mullo",
        "barrier",
        "swizzle",
        "i2i",
        "i2f",
        "f2i",
        "f2irnd",
        "f2i7",
        "cmp",
        "load",
        "store",
        "copysign",
        "getexp",
        "getmant",
        "nan",
        "nextafter",
        "roundeven",
        "roundaway",
        "iaddsat",
        "imullo0",
        "imullo1",
        "imullosat0",
        "imullosat1",
        "imulhi0",
        "imulhi1",
        "imul0",
        "imul1",
        "idiv0",
        "idiv1",
        "idiv2",
        "idiv3",
        "imod0",
        "imod1",
        "imod2",
        "imod3",
        "imadlo0",
        "imadlo1",
        "imadlosat0",
        "imadlosat1",
        "imadhi0",
        "imadhi1",
        "imadhisat0",
        "imadhisat1",
        "halfadd",
        "halfaddinc",
        "movai",
        "iabs",
        "leadzero",
        "lshift",
        "rshift",
        "rotate",
        "or",
        "and",
        "xor",
        "not",
        "bitselect",
        "popcount",
	};

	static const char * _condition[] =
	{
		"true",
		"gt",
		"lt",
		"ge",
		"le",
		"eq",
		"ne",
		"and",
		"or",
		"xor",
		"not",
		"nz",
		"gez",
		"gz",
		"lez",
		"lz",
	};

    static const char * _typeNames[] =
    {
       "f32",
       "f16",
       "s32",
       "s16",
       "s8",
       "u32",
       "u16",
       "u8"
    };

    /* Assemble opcode. */
    opcode += opcode_msb6 << 6;

	/* Decode address and opcode. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%03u: %s", Address, _opcode[opcode]));

	/* Decode condition if not gcvTRUE. */
	if (condition != 0x00)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   ".%s", _condition[condition]));
	}

    if (OutputFormat)
    {
	    gctUINT32 inst_type0  = (((((gctUINT32) (States[1])) >> (0 ? 21:21)) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) );
	    gctUINT32 inst_type1  = (((((gctUINT32) (States[2])) >> (0 ? 31:30)) & ((gctUINT32) ((((1 ? 31:30) - (0 ? 31:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:30) - (0 ? 31:30) + 1)))))) );
	    gctUINT32 value_type0 = (inst_type1 << 1) + inst_type0;
	    gctUINT32 value_type1 = sampler_num;

        /* Decode format for instructions supporting multiple formats. */
        switch (opcode)
        {
        case 0x01:
        case 0x0F:
        case 0x27:
        case 0x32:
        case 0x33:
        case 0x2E:
        case 0x2D:
        case 0x57:
        case 0x58:
        case 0x5F:
        case 0x44:
        case 0x48:
        case 0x3B:
        case 0x54:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5C:
        case 0x5D:
        case 0x5E:
        case 0x42:
        case 0x43:
        case 0x3C:
        case 0x3D:
        case 0x40:
        case 0x41:
        case 0x3E:
        case 0x3F:
        case 0x4C:
        case 0x4D:
        case 0x50:
        case 0x51:
        case 0x4E:
        case 0x4F:
        case 0x52:
        case 0x53:
        case 0x61:
        case 0x56:
		    gcmVERIFY_OK(
			    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							       ".%s", _typeNames[value_type0]));
            break;

        case 0x2C:
		    gcmVERIFY_OK(
			    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							       ".%s.%s", _typeNames[value_type1],
                                   _typeNames[value_type0]));
            break;

        default:
            break;
        }
    }

	/* Decode saturate post modifier. */
	if (saturate)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ".sat"));
	}

	/* Move to operand column. */
	while (offset < column)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, " "));
	}

	/* Decode destination. */
	if (dest_valid && (dest_enable != 0) )
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   "%c%u",
							   ((opcode == 0x0A)
							   || (opcode == 0x0B)
							   || (opcode == 0x56)
							   ) ? 'a' : 'r',
							   dest_addr));

		_DebugAddressing(buffer, gcmSIZEOF(buffer), &offset, dest_rel);

		if (dest_enable != 0xF)
		{
			gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "."));
			if (dest_enable & 0x1)
			{
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "x"));
			}
			if (dest_enable & 0x2)
			{
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "y"));
			}
			if (dest_enable & 0x4)
			{
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "z"));
			}
			if (dest_enable & 0x8)
			{
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "w"));
			}
		}
	}

	/* Decode source 0. */
	if (src0_valid)
	{
		/* Append comma if not first operand. */
		if (offset > column)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
		}

		_DebugRegister(buffer, gcmSIZEOF(buffer), &offset,
					   src0_type,
					   src0_addr,
					   src0_rel,
					   src0_swizzle,
					   src0_neg,
					   src0_abs);
	}

	/* Decode source 1. */
	if (src1_valid)
	{
		/* Append comma if not first operand. */
		if (offset > column)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
		}

		_DebugRegister(buffer, gcmSIZEOF(buffer), &offset,
					   src1_type,
					   src1_addr,
					   src1_rel,
					   src1_swizzle,
					   src1_neg,
					   src1_abs);
	}

	/* Decode source 2. */
	if (src2_valid)
	{
		/* Append comma if not first operand. */
		if (offset > column)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
		}

		_DebugRegister(buffer, gcmSIZEOF(buffer), &offset,
					   src2_type,
					   src2_addr,
					   src2_rel,
					   src2_swizzle,
					   src2_neg,
					   src2_abs);
	}

	/* Handle special opcodes. */
	switch (opcode)
	{
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
		/* Append comma if not first operand. */
		if (offset > column)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
		}

		/* Append sampler. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   "s%u", sampler_num));
		_DebugAddressing(buffer, gcmSIZEOF(buffer), &offset, sampler_rel);
		_DebugSwizzle(buffer, gcmSIZEOF(buffer), &offset, sampler_swizzle);
		break;

	case 0x16:
	case 0x14:
		/* Append comma if not first operand. */
		if (offset > column)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
		}

		/* Append target. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   "%u", target));
		break;
	}

	gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER, buffer);
}

void
_DumpShader(
    IN gctUINT32_PTR States,
    IN gctUINT32 StateBufferOffset,
    IN gctBOOL OutputFormat,
    IN gctUINT InstBase,
    IN gctUINT InstMax
	)
{
	gctUINT32 lastState, address, count, nextAddress;

	lastState = (gctUINT32) States + StateBufferOffset;
	nextAddress = 0;

    while ((gctUINT32) States < lastState)
    {
        gctUINT32 state = *States++;

		gcmASSERT(((((gctUINT32) (state)) >> (0 ? 31:27) & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1)))))) == (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))));

        address = (((((gctUINT32) (state)) >> (0 ? 15:0)) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1)))))) );
        count   = (((((gctUINT32) (state)) >> (0 ? 25:16)) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1)))))) );

		if ((address >= InstBase) &&
              (address < InstBase + InstMax) )
        {
			if (nextAddress == 0) {
				/* Header. */
				gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER,
							  "***** [ Generated Shader Code ] *****");
			}

			/* Dump all states. */
			for (address = 0; count >= 4; count -= 4)
			{
				/* Dump shader code. */
				_DebugShader(address++ + nextAddress, States, OutputFormat);

				/* Next instruction. */
				States += 4;
			}
			nextAddress += address;
        }
        else
        {
            States += count;
        }

        if ((count & 1) == 0) ++States;
    }
}
#endif /* gcmIS_DEBUG(gcdDEBUG_TRACE) */

gceSTATUS
gcoHARDWARE_ProgramUniform(IN gctUINT32 Address,
                           IN gctUINT Columns,
                           IN gctUINT Rows,
                           IN gctPOINTER Values,
                           IN gctBOOL FixedPoint
                           )
{
    gceSTATUS status;
    gcoHARDWARE hw;
    gctUINT i, j;
    gctUINT32_PTR src = (gctUINT32_PTR) Values;
    gctUINT32 address = Address >> 2;

    /* Define state buffer variables. */
    gcmDEFINESTATEBUFFER(reserve, stateDelta, memory, reserveSize);

    gcmHEADER_ARG("Address=%u Columns=%u Rows=%u Values=%p FixedPoint=%d",
                  Address, Columns, Rows, Values, FixedPoint);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Values != gcvNULL);

    /* Get the current hardware object. */
    gcmGETHARDWARE(hw);

    /* Fast case when we have 4 columns or just 1 row. */
    if ((Columns == 4) || (Rows == 1))
    {
        /* Determine the size of the buffer to reserve. */
        reserveSize = gcmALIGN((1 + Columns * Rows) * gcmSIZEOF(gctUINT32), 8);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(hw, reserve, stateDelta, memory, reserveSize);

        /* Begin the state batch. */
        {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, address, Columns * Rows);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (FixedPoint) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (Columns * Rows) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

        /* Walk all states. */
        for (i = 0; i < Columns * Rows; i++)
        {
            /* Set the state value. */
            gcmSETSTATEDATA(stateDelta,
                            reserve,
                            memory,
                            FixedPoint,
                            address + i,
                            *src++);
        }

        if ((i & 1) == 0)
        {
            /* Align. */
            gcmSETFILLER(reserve, memory);
        }

        /* End the state batch. */
        gcmENDSTATEBATCH(reserve, memory);

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);
    }
    else
    {
        /* Determine the size of the buffer to reserve. */
        reserveSize = Rows * gcmALIGN((1 + Columns) * gcmSIZEOF(gctUINT32), 8);

        /* Reserve space in the command buffer. */
        gcmBEGINSTATEBUFFER(hw, reserve, stateDelta, memory, reserveSize);

        /* Walk all rows. */
        for (i = 0; i < Rows; i++)
        {
            /* Begin the state batch. */
            {    gcmASSERT(((memory - (gctUINT32_PTR) reserve->lastReserve) & 1) == 0);    gcmVERIFYLOADSTATEDONE(reserve);    gcmSTORELOADSTATE(reserve, memory, address, Columns);    *memory++ = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | (((gctUINT32) ((gctUINT32) (FixedPoint) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1))))))) << (0 ? 26:26))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (Columns) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));gcmSKIPSECUREUSER();};

            /* Walk all columns. */
            for (j = 0; j < Columns; j++)
            {
                /* Set the state value. */
                gcmSETSTATEDATA(stateDelta,
                                reserve,
                                memory,
                                FixedPoint,
                                address + j,
                                *src++);
            }

            if ((j & 1) == 0)
            {
                /* Align. */
                gcmSETFILLER(reserve, memory);
            }

            /* End the state batch. */
            gcmENDSTATEBATCH(reserve, memory);

            /* Next row. */
            address += 4;
        }

        /* Validate the state buffer. */
        gcmENDSTATEBUFFER(reserve, memory, reserveSize);
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#endif /*VIVANTE_NO_3D*/



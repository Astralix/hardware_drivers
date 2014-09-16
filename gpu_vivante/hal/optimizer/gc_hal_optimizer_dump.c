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
**	Optimizer and shader output module.
*/

#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

#include "gc_hal_optimizer.h"

#define _GC_OBJ_ZONE	gcvZONE_COMPILER

#define __DUMP_OPTIMIZER_DETAILS__		0

#define INDENT_GAP	4

/*******************************************************************************
**								gcOptimizer Logging
*******************************************************************************/
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
void
gcOpt_DumpBuffer(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSTRING Buffer,
	IN gctSIZE_T ByteCount
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	if (File)
	{
		status = gcoOS_Write(Os, File, ByteCount, Buffer);
		if (gcmIS_ERROR(status))
		{
			gcmTRACE(gcvLEVEL_ERROR, "gcoOS_Write fail.");
		}
	}
	else
	{
		/* Print string to debug terminal. */
		gcmTRACE(gcvLEVEL_ERROR, Buffer);
	}
}
#else
void
gcOpt_DumpBuffer(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSTRING Buffer,
	IN gctSIZE_T ByteCount
	)
{
	gcoOS_Write(Os, File, ByteCount, Buffer);
}
#endif

void
gcOpt_DumpMessage(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSTRING Message
	)
{
	gctSIZE_T length = 0;

#if !gcdDEBUG
	if (File == gcvNULL) return;
#endif

	gcoOS_StrLen(Message, &length);
	gcOpt_DumpBuffer(Os, File, Message, length);
}

/*******************************************************************************
**									_DumpName
********************************************************************************
**
**	Print the name.
**
**	INPUT:
**
**		gctSIZE_T Length
**			Length of the name.
**
**		gctCONST_STRING Name
**			Pointer to the name.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
**
**	RETURN VALUE:
**
**		gctINT
**			The number of characters copied into the output buffer.
*/
static gctINT
_DumpName(
	IN gctSIZE_T Length,
	IN gctCONST_STRING Name,
	OUT char * Buffer,
	gctSIZE_T BufferSize
	)
{
	gctUINT offset = 0;
	gctSIZE_T size;

	switch (Length)
	{
	case gcSL_POSITION:
		/* Predefined position. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#Position"));
		return offset;

	case gcSL_POINT_SIZE:
		/* Predefined point size. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#PointSize"));
		return offset;

	case gcSL_COLOR:
		/* Predefined color. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#Color"));
		return offset;

	case gcSL_DEPTH:
		/* Predefined depth. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#Depth"));
		return offset;

	case gcSL_FRONT_FACING:
		/* Predefined front facing. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#FrontFacing"));
		return offset;

	case gcSL_POINT_COORD:
		/* Predefined point coordinate. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#PointCoord"));
		return offset;

	case gcSL_POSITION_W:
		/* Predefined position.w. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "#Position.w"));
		return offset;
	}

	/* Copy the name into the buffer. */
	size = gcmMIN(Length, BufferSize - 1);
	if(size) {
	  gcmVERIFY_OK(gcoOS_MemCopy(Buffer, Name, size));
	}
	Buffer[size] = '\0';
	return size;
}

/*******************************************************************************
**									_DumpSwizzle
********************************************************************************
**
**	Print the swizzle.
**
**	INPUT:
**
**		gcSL_SWIZZLE SwizzleX
**			Swizzle for the x component.
**
**		gcSL_SWIZZLE SwizzleY
**			Swizzle for the y component.
**
**		gcSL_SWIZZLE SwizzleW
**			Swizzle for the w component.
**
**		gcSL_SWIZZLE SwizzleZ
**			Swizzle for the z component.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
**
**	RETURN VALUE:
**
**		gctINT
**			The number of characters copied into the output buffer.
*/
static gctINT
_DumpSwizzle(
	IN gcSL_SWIZZLE SwizzleX,
	IN gcSL_SWIZZLE SwizzleY,
	IN gcSL_SWIZZLE SwizzleZ,
	IN gcSL_SWIZZLE SwizzleW,
	OUT char * Buffer,
	IN gctSIZE_T BufferSize
	)
{
	gctUINT offset = 0;

	static const char swizzle[] =
	{
		'x', 'y', 'z', 'w',
	};

	/* Don't print anything for the default swizzle (.xyzw). */
	if ((SwizzleX == gcSL_SWIZZLE_X)
	&&  (SwizzleY == gcSL_SWIZZLE_Y)
	&&  (SwizzleZ == gcSL_SWIZZLE_Z)
	&&  (SwizzleW == gcSL_SWIZZLE_W)
	)
	{
		return 0;
	}

	/* Print the period and the x swizzle. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
						   ".%c", swizzle[SwizzleX]));

	/* Only continue of the other swizzles are different. */
	if ((SwizzleY != SwizzleX)
	||  (SwizzleZ != SwizzleX)
	||  (SwizzleW != SwizzleX)
	)
	{
		/* Append the y swizzle. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
							   "%c", swizzle[SwizzleY]));

		/* Only continue of the other swizzles are different. */
		if ( (SwizzleZ != SwizzleY) || (SwizzleW != SwizzleY) )
		{
			/* Append the z swizzle. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
								   "%c", swizzle[SwizzleZ]));

			/* Only continue of the other swizzle are different. */
			if (SwizzleW != SwizzleZ)
			{
				/* Append the w swizzle. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
									   "%c", swizzle[SwizzleW]));
			}
		}
	}

	/* Return the number of characters printed. */
	return offset;
}

/*******************************************************************************
**									_DumpRegister
********************************************************************************
**
**	Print the format, type, and addressing mode of a register.
**
**	INPUT:
**
**		gcSL_TYPE Type
**			Type of the register.
**
**		gcSL_FORMAT Format
**			Format of the register.
**
**		gcSL_INDEX Index
**			Index of the register.
**
**		gcSL_INDEXED Mode
**			Addressing mode for the register.
**
**		gctINT Indexed
**			Indexed register if addressing mode is indexed.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
**
**	RETURN VALUE:
**
**		gctINT
**			The number of characters copied into the output buffer.
*/
static gctINT
_DumpRegister(
	IN gcSL_TYPE Type,
	IN gcSL_FORMAT Format,
	IN gctUINT16 Index,
	IN gcSL_INDEXED Mode,
	IN gctINT Indexed,
	OUT char * Buffer,
	IN gctSIZE_T BufferSize
	)
{
	gctUINT offset = 0;

	static gctCONST_STRING type[] =
	{
		"instruction",			/* Use gcSL_NONE as an instruction. */
		"temp",
		"attribute",
		"uniform",
		"sampler",
		"constant",
		"output",
	};

	static gctCONST_STRING format[] =
	{
		"",						/* Don't append anything for float registers. */
		".int",
		".bool",
		".uint",
		".int8",
		".uint8",
		".int16",
		".uint16",
		".int64",
		".uint64",
		".int128",
		".uint128",
		".float16",
		".float64",
		".float128",
	};

	static const char index[] =
	{
		'?', 'x', 'y', 'z', 'w',
	};

	/* Print type, format and index of register. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
						   "%s", type[Type]));
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
						   "%s(%d", format[Format], gcmSL_INDEX_GET(Index, Index)));

	if (gcmSL_INDEX_GET(Index, ConstValue) > 0)
	{
		/* Append the addressing mode. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
							   "+%d", gcmSL_INDEX_GET(Index, ConstValue)));
	}

	if (Mode != gcSL_NOT_INDEXED)
	{
		/* Append the addressing mode. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
							   "+%s",
							   type[gcSL_TEMP]));
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
							   "%s(%d).%c",
							   format[gcSL_INTEGER],
							   Indexed,
							   index[Mode]));
	}
	else if (Indexed > 0)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, "+%d", Indexed));
	}

	/* Append the final ')'. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, ")"));

	/* Return the number of characters printed. */
	return offset;
}

/*******************************************************************************
**									 _DumpSource
********************************************************************************
**
**	Print a source operand.
**
**	INPUT:
**
**		gcSL_SOURCE Source
**			The source operand.
**
**		gcSL_INDEX Index
**			Index of the source operand.
**
**		gctINT Indexed
**			Index register if the source operand addressing mode is indexed.
**
**		gctBOOL AddComma
**			Prepend the source operand with a comma.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
**
**	RETURN VALUE:
**
**		gctINT
**			The number of characters copied into the output buffer.
*/
static gctINT
_DumpSource(
	IN gctUINT16 Source,
	IN gctUINT16 Index,
	IN gctINT Indexed,
	IN gctBOOL AddComma,
	OUT char * Buffer,
	IN gctSIZE_T BufferSize
	)
{
	gctUINT offset = 0;

	/* Only process source operand if it is valid. */
	if (gcmSL_SOURCE_GET(Source, Type) != gcSL_NONE)
	{
		if (AddComma)
		{
			/* Prefix a comma. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, ", "));
		}

		if (gcmSL_SOURCE_GET(Source, Type) == gcSL_CONSTANT)
		{
			/* Assemble the 32-bit value. */
			gctUINT32 value = Index | (Indexed << 16);

			switch (gcmSL_SOURCE_GET(Source, Format))
			{
			case gcSL_FLOAT:
				/* Print the floating point constant. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
									   "%f", gcoMATH_UIntAsFloat(value)));
				break;

			case gcSL_INTEGER:
				/* Print the integer constant. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
									   "%d", value));
				break;

			case gcSL_UINT32:
				/* Print the unsigned integer constant. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
									   "%u", value));
				break;

			case gcSL_BOOLEAN:
				/* Print the boolean constant. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
									   "%s", value ? "gcvTRUE" : "gcvFALSE"));
				break;
			}
		}
		else
		{
			/* Decode the register. */
			offset += _DumpRegister((gcSL_TYPE) gcmSL_SOURCE_GET(Source, Type),
									(gcSL_FORMAT) gcmSL_SOURCE_GET(Source, Format),
									Index,
									(gcSL_INDEXED) gcmSL_SOURCE_GET(Source, Indexed),
									Indexed,
									Buffer + offset,
									BufferSize - offset);

			/* Decode the swizzle. */
			offset += _DumpSwizzle((gcSL_SWIZZLE) gcmSL_SOURCE_GET(Source, SwizzleX),
								   (gcSL_SWIZZLE) gcmSL_SOURCE_GET(Source, SwizzleY),
								   (gcSL_SWIZZLE) gcmSL_SOURCE_GET(Source, SwizzleZ),
								   (gcSL_SWIZZLE) gcmSL_SOURCE_GET(Source, SwizzleW),
								   Buffer + offset,
								   BufferSize - offset);
		}
	}

	/* Return the number of characters printed. */
	return offset;
}

/*******************************************************************************
**								 _DumpSourceTargetFormat
********************************************************************************
**
**	Print a source which contains the target format used in gcSL_CONV.
**
**	INPUT:
**
**		gcSL_SOURCE Source
**			The source operand.
**
**		gcSL_INDEX Index
**			Index of the source operand.
**
**		gctINT Indexed
**			Index register if the source operand addressing mode is indexed.
**
**		gctBOOL AddComma
**			Prepend the source operand with a comma.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
**
**	RETURN VALUE:
**
**		gctINT
**			The number of characters copied into the output buffer.
*/
static gctINT
_DumpSourceTargetFormat(
	IN gctUINT16 Source,
	IN gctUINT16 Index,
	IN gctINT Indexed,
	IN gctBOOL AddComma,
	OUT char * Buffer,
	IN gctSIZE_T BufferSize
	)
{
	static gctCONST_STRING targetFormat[] =
	{
		"float",
		"int",
		"bool",
		"uint",
		"int8",
		"uint8",
		"int16",
		"uint16",
		"int64",
		"uint64",
		"int128",
		"uint128",
		"float16",
		"float64",
		"float128",
	};
	gctUINT offset = 0;
	gctUINT32 value = Index | (Indexed << 16);

	if (AddComma)
	{
		/* Prefix a comma. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, ", "));
	}
	gcmASSERT(gcmSL_SOURCE_GET(Source, Type) == gcSL_CONSTANT);
	/* Assemble the 32-bit value. */

	gcmASSERT(gcmSL_SOURCE_GET(Source, Format) == gcSL_UINT32);

	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
						   "%s", targetFormat[value]));

	/* Return the number of characters printed. */
	return offset;
}

/*******************************************************************************
**									  _DumpDataFlowList
********************************************************************************
**
**	Print data flow list.
**
**	INPUT:
**
**		gctCONST_STRING Title
**			Pointer to the title for the list.
**
**		gcsLINKTREE_LIST_PTR List
**			Pointer to the first gcsLINKTREE_LIST_PTR structure to print.
**
**		gctUINT Offset
**			Pointer to the offset of output.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
*/
static void
_DumpDataFlowList(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctCONST_STRING Title,
	IN gcOPT_LIST List,
	IN gctUINT * Offset,
	OUT char * Buffer,
	IN gctSIZE_T BufferSize
	)
{
	gctSIZE_T length = 0;

	if (List == gcvNULL)
	{
		/* Don't print anything on an empty list. */
		return;
	}

	gcmVERIFY_OK(gcoOS_StrLen(Title, &length));
	length += 8;	/* extra indent for instruction number. */
	length = gcmMIN(length, BufferSize);

	/* Extra indent. */
	for (; *Offset < 8; (*Offset)++)
	{
		if (*Offset >= BufferSize)
		{
			break;
		}

		Buffer[*Offset] = ' ';
	}

	/* Print the title. */
	gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, Title));

	/* Walk while there are entries in the list. */
	while (List != gcvNULL)
	{
		/* Check if we have reached the right margin. */
		if (*Offset > 70)
		{
			/* Dump the assembled line. */
			gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, ",\n"));
			gcOpt_DumpBuffer(Os, File, Buffer, *Offset);

			/* Indent to the end of the title. */
			for (*Offset = 0; *Offset < length; (*Offset)++)
			{
				Buffer[*Offset] = ' ';
			}
		}
		else if (*Offset > length)
		{
			/* Print comma and space. */
			gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, ", "));
		}

		/* output list index. */
		if (List->index >= 0)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, Offset,
								   "%d", List->code->id));
		}
		else if (List->index == gcvOPT_INPUT_REGISTER)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "input"));
		}
		else if (List->index == gcvOPT_OUTPUT_REGISTER)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "output"));
		}
		else if (List->index == gcvOPT_GLOBAL_REGISTER)
		{
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "global"));
		}

		/* Move to the next entry in the list. */
		List = List->next;
	}

	/* Dump the final assembled line. */
	gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, Offset, "\n"));
	gcOpt_DumpBuffer(Os, File, Buffer, *Offset);
	*Offset = 0;
}

/*******************************************************************************
**									  _DumpCodeDataFlow
********************************************************************************
**
**	Print data flow list for one code.
**
**	INPUT:
**
**		gcOPT_CODE code
**			Pointer to the code.
**
**	OUTPUT:
**
**		none
*/
static void
_DumpCodeDataFlow(
	IN gcoOS Os,
	IN gctFILE File,
	IN gcOPT_CODE code)
{
	char buffer[256];
	gctUINT offset = 0;

	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "  %4d: ", code->id));

	if (code->users)
	{
		_DumpDataFlowList(Os,
						  File,
						  "Users: ",
						  code->users,
						  &offset,
						  buffer, gcmSIZEOF(buffer));
	}

	if (code->dependencies0)
	{
		_DumpDataFlowList(Os,
						  File,
						  "Src 0: ",
						  code->dependencies0,
						  &offset,
						  buffer, gcmSIZEOF(buffer));
	}

	if (code->dependencies1)
	{
		_DumpDataFlowList(Os,
						  File,
						  "Src 1: ",
						  code->dependencies1,
						  &offset,
						  buffer, gcmSIZEOF(buffer));
	}

	if (code->prevDefines)
	{
		_DumpDataFlowList(Os,
						  File,
						  "P Def: ",
						  code->prevDefines,
						  &offset,
						  buffer, gcmSIZEOF(buffer));
	}

	if (code->nextDefines)
	{
		_DumpDataFlowList(Os,
						  File,
						  "N Def: ",
						  code->nextDefines,
						  &offset,
						  buffer, gcmSIZEOF(buffer));
	}

	if (offset > 0)
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "\n"));
		gcOpt_DumpBuffer(Os, File, buffer, offset);
	}
}

/*******************************************************************************
**									  _DumpDataFlow
********************************************************************************
**
**	Print data flow list.
**
**	INPUT:
**
**		gcOPT_FUNCTION Function
**			Pointer to a function.
**
**	OUTPUT:
**
**		char * Buffer
**			Pointer to the buffer to be used as output.
*/
static void
_DumpDataFlow(
	IN gcoOS Os,
	IN gctFILE File,
	IN gcOPT_FUNCTION Function
	)
{

    gcOPT_CODE code;

	/* Output data flow for each code. */
    for (code = Function->codeHead; code != gcvNULL && code != Function->codeTail->next; code = code->next)
	{
		_DumpCodeDataFlow(Os, File, code);
	}
}

/*******************************************************************************
**									  _DumpIR
********************************************************************************
**
**	Print IR to File
**
**	INPUT:
**
**      gctINT CodeId
**          the instruction's code id in a funciton, do not print the id if -1
**
**		gcSL_INSTRUCTION code
**			Pointer to an instruction.
**
**	OUTPUT:
**
**		none
*/
static void
_DumpIR(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctINT  CodeId,
	IN gcSL_INSTRUCTION code)
{
	char buffer[256];
	gctUINT offset = 0;

	static struct _DECODE
	{
		gctCONST_STRING	opcode;
		gctBOOL		hasTarget;
		gctBOOL		hasLabel;
	}
	decode[] =
	{
        { "NOP",        gcvFALSE,   gcvFALSE },
        { "MOV",        gcvTRUE,    gcvFALSE },
        { "SAT",        gcvTRUE,    gcvFALSE },
        { "DP3",        gcvTRUE,    gcvFALSE },
        { "DP4",        gcvTRUE,    gcvFALSE },
        { "ABS",        gcvTRUE,    gcvFALSE },
        { "JMP",        gcvFALSE,   gcvTRUE  },
        { "ADD",        gcvTRUE,    gcvFALSE },
        { "MUL",        gcvTRUE,    gcvFALSE },
        { "RCP",        gcvTRUE,    gcvFALSE },
        { "SUB",        gcvTRUE,    gcvFALSE },
        { "KILL",       gcvFALSE,   gcvFALSE },
        { "TEXLD",      gcvTRUE,    gcvFALSE },
        { "CALL",       gcvFALSE,   gcvTRUE  },
        { "RET",        gcvFALSE,   gcvFALSE },
        { "NORM",       gcvTRUE,    gcvFALSE },
        { "MAX",        gcvTRUE,    gcvFALSE },
        { "MIN",        gcvTRUE,    gcvFALSE },
        { "POW",        gcvTRUE,    gcvFALSE },
        { "RSQ",        gcvTRUE,    gcvFALSE },
        { "LOG",        gcvTRUE,    gcvFALSE },
        { "FRAC",       gcvTRUE,    gcvFALSE },
        { "FLOOR",      gcvTRUE,    gcvFALSE },
        { "CEIL",       gcvTRUE,    gcvFALSE },
        { "CROSS",      gcvTRUE,    gcvFALSE },
        { "TEXLDP",     gcvTRUE,    gcvFALSE },
        { "TEXBIAS",    gcvFALSE,   gcvFALSE },
        { "TEXGRAD",    gcvFALSE,   gcvFALSE },
        { "TEXLOD",     gcvFALSE,   gcvFALSE },
        { "SIN",        gcvTRUE,    gcvFALSE },
        { "COS",        gcvTRUE,    gcvFALSE },
        { "TAN",        gcvTRUE,    gcvFALSE },
        { "EXP",        gcvTRUE,    gcvFALSE },
        { "SIGN",       gcvTRUE,    gcvFALSE },
        { "STEP",       gcvTRUE,    gcvFALSE },
        { "SQRT",       gcvTRUE,    gcvFALSE },
        { "ACOS",       gcvTRUE,    gcvFALSE },
        { "ASIN",       gcvTRUE,    gcvFALSE },
        { "ATAN",       gcvTRUE,    gcvFALSE },
        { "SET",        gcvTRUE,    gcvFALSE },
        { "DSX",        gcvTRUE,    gcvFALSE },
        { "DSY",        gcvTRUE,    gcvFALSE },
        { "FWIDTH",     gcvTRUE,    gcvFALSE },
        { "DIV",        gcvTRUE,    gcvFALSE },
        { "MOD",        gcvTRUE,    gcvFALSE },
        { "AND_BITWISE",gcvTRUE,    gcvFALSE },
        { "OR_BITWISE", gcvTRUE,    gcvFALSE },
        { "XOR_BITWISE",gcvTRUE,    gcvFALSE },
        { "NOT_BITWISE",gcvTRUE,    gcvFALSE },
        { "LSHIFT",     gcvTRUE,    gcvFALSE },
        { "RSHIFT",     gcvTRUE,    gcvFALSE },
        { "ROTATE",     gcvTRUE,    gcvFALSE },
        { "BITSEL",     gcvTRUE,    gcvFALSE },
        { "LEADZERO",   gcvTRUE,    gcvFALSE },
        { "LOAD",       gcvTRUE,    gcvFALSE },
        { "STORE",      gcvTRUE,    gcvFALSE },
        { "BARRIER",    gcvFALSE,   gcvFALSE },
        { "STORE1",     gcvTRUE,    gcvFALSE },
        { "ATOMADD",    gcvTRUE,    gcvFALSE },
        { "ATOMSUB",    gcvTRUE,    gcvFALSE },
        { "ATOMXCHG",   gcvTRUE,    gcvFALSE },
        { "ATOMCMPXCHG",gcvTRUE,    gcvFALSE },
        { "ATOMMIN",    gcvTRUE,    gcvFALSE },
        { "ATOMMAX",    gcvTRUE,    gcvFALSE },
        { "ATOMOR",     gcvTRUE,    gcvFALSE },
        { "ATOMAND",    gcvTRUE,    gcvFALSE },
        { "ATOMXOR",    gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "UNUSED",     gcvTRUE,    gcvFALSE },
        { "ADDLO",      gcvTRUE,    gcvFALSE },
        { "MULLO",      gcvTRUE,    gcvFALSE },
        { "CONV",       gcvTRUE,    gcvFALSE },
        { "GETEXP",     gcvTRUE,    gcvFALSE },
        { "GETMANT",    gcvTRUE,    gcvFALSE },
        { "MULHI",      gcvTRUE,    gcvFALSE },
        { "CMP",        gcvTRUE,    gcvFALSE },
        { "I2F",        gcvTRUE,    gcvFALSE },
        { "F2I",        gcvTRUE,    gcvFALSE },
        { "ADDSAT",     gcvTRUE,    gcvFALSE },
        { "SUBSAT",     gcvTRUE,    gcvFALSE },
        { "MULSAT",     gcvTRUE,    gcvFALSE },
	};

	static gctCONST_STRING condition[] =
	{
		"",
		".NE",
		".LE",
		".L",
		".EQ",
		".G",
		".GE",
		".AND",
		".OR",
		".XOR",
        ".NZ",
	};

	gctUINT16 target;
	offset = 0;

	/* Get target. */
	target = code->temp;

	/* Instruction number, opcode and condition. */
	if (CodeId != -1) {
	    gcmVERIFY_OK(
		    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "  %4d: ",
						   CodeId));
	}
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%s%s",
						   decode[code->opcode].opcode,
						   condition[gcmSL_TARGET_GET(target, Condition)]));

	/* Align to column 24. */
	do
	{
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, " "));
	}
	while (offset < 24);

	/* Does the instruction specify a target? */
	if (decode[code->opcode].hasTarget)
	{
		/* Decode register. */
		offset += _DumpRegister(gcSL_TEMP,
								(gcSL_FORMAT) gcmSL_TARGET_GET(target, Format),
								code->tempIndex,
								(gcSL_INDEXED) gcmSL_TARGET_GET(target, Indexed),
								code->tempIndexed,
								buffer + offset,
								gcmSIZEOF(buffer) - offset);

		if (gcmSL_TARGET_GET(target, Enable) != gcSL_ENABLE_XYZW)
		{
			/* Enable dot. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   "."));

			if (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_X)
			{
				/* Enable x. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "x"));
			}

			if (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_Y)
			{
				/* Enable y. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "y"));
			}

			if (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_Z)
			{
				/* Enable z. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "z"));
			}

			if (gcmSL_TARGET_GET(target, Enable) & gcSL_ENABLE_W)
			{
				/* Enable w. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "w"));
			}
		}
	}

	/* Does the instruction specify a label? */
	else if (decode[code->opcode].hasLabel)
	{
		/* Append label. */
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   "%d", code->tempIndex));
	}

	/* Append source operand 0. */
	offset += _DumpSource(code->source0,
						  code->source0Index,
						  code->source0Indexed,
						  offset > 24,
						  buffer + offset, gcmSIZEOF(buffer) - offset);

	/* Append source operand 1. */
	if(code->opcode == gcSL_CONV) {
	   offset += _DumpSourceTargetFormat(code->source1,
						  code->source1Index,
						  code->source1Indexed,
						  offset > 24,
						  buffer + offset, gcmSIZEOF(buffer) - offset);
	}
	else {
	   offset += _DumpSource(code->source1,
						  code->source1Index,
						  code->source1Indexed,
						  offset > 24,
						  buffer + offset, gcmSIZEOF(buffer) - offset);
	}

	/* Dump the instruction. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "\n"));
	gcOpt_DumpBuffer(Os, File, buffer, offset);
}

/* dump instruction to debugger output */
void dbg_dumpIR(IN gcSL_INSTRUCTION Inst, gctINT n)
{

	_DumpIR(gcvNULL, gcvNULL, n, Inst);
}

/* dump optimizer to debugger output */
void dbg_dumpOptimizer(IN gcOPTIMIZER Optimizer)
{
	gcOpt_Dump(gcvNULL, "Dump Optimizer", Optimizer, gcvNULL);
}

/* dump shader to debugger output */
void dbg_dumpShader(IN gcSHADER Shader)
{
	gcOpt_Dump(gcvNULL, "Dump Shader", gcvNULL, Shader);
}

/* dump code with data flow to debugger output */
void dbg_dumpCode(IN gcOPT_CODE Code)
{
	_DumpIR(gcvNULL, gcvNULL, Code->id, &Code->instruction);
	_DumpCodeDataFlow(gcvNULL, gcvNULL, Code);
}

gctINT gcvDumpOption =
#if GC_ENABLE_LOADTIME_OPT
            0 /* gceLTC_DUMP_EXPESSION | gceLTC_DUMP_COLLECTING | gceLTC_DUMP_UNIFORM*/;
#else
            0;
#endif

/* return true if the dump option Opt is set, otherwise return false */
gctBOOL
gcDumpOption(gctINT Opt)
{
    return (gcvDumpOption & Opt) != 0;
}

/*******************************************************************************
**									 gcOpt_Dump
********************************************************************************
**
**	Print the shader, flow graph, and dependency tree for debugging.
**
**	INPUT:
**
**		gctCONST_STRING Text
**			Pointer to text to print before dumping shader, etc.
**
**		gcOPTIMIZER Optimizer
**			Pointer to the gcOPTIMIZER structure to print.
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object holding information about the compiled shader.
**			Shader is used only when Optimizer is gcvNULL.
*/
void
gcOpt_Dump(
	IN gctFILE File,
	IN gctCONST_STRING Text,
	IN gcOPTIMIZER Optimizer,
	IN gcSHADER Shader
	)
{
	gctSIZE_T i;
	char buffer[256];
	gctUINT offset = 0;
	gcoOS os = gcvNULL;
	gcSHADER shader = Shader;
	gctBOOL dumpOptimizer = gcvFALSE;
	gctUINT functionCount;
	gcSL_INSTRUCTION code;
	gcOPT_CODE codeNode = gcvNULL;
	gctUINT codeCount;

	static gctCONST_STRING type[] =
	{
		"float",
		"float2",
		"float3",
		"float4",
		"matrix2x2",
		"matrix3x3",
		"matrix4x4",
		"bool",
		"bool2",
		"bool3",
		"bool4",
		"int",
		"int2",
		"int3",
		"int4",
		"sampler1D",
		"sampler2D",
		"sampler3D",
		"samplerCubic",
		"fixed",
		"fixed2",
		"fixed3",
		"fixed4",
		"image2d_t",
		"image2d_t",
		"sampler_t",
		"matrix2x3",
		"matrix2x4",
		"matrix3x2",
		"matrix3x4",
		"matrix4x2",
		"matrix4x3",
		"isampler2D",
		"isampler3D",
		"isamplerCubic",
		"usampler2D",
		"usampler3D",
		"usamplerCubic",
		"samplerExternalOES",
	};

	static gctCONST_STRING doubleDashLine = "===============================================================================\n";
	static gctCONST_STRING dashLine = "*******************************************************************************\n";

	gcmHEADER_ARG("File=%p Text=0x%x Optimizer=%p Shader=%p",
					File, Text, Optimizer, Shader);
	gcmTRACE(gcvLEVEL_VERBOSE, "Text=%s", Text);

	if (File == gcvNULL)
	{
#if gcmIS_DEBUG(gcdDEBUG_CODE)
#if !__DUMP_OPTIMIZER_DETAILS__
        /* The default for non-standalone is to dump incoming and final shaders only. */
    	if (Optimizer != gcvNULL)
        {
		    gcmFOOTER_NO();
		    return;
        }
#endif
#else
		gcmFOOTER_NO();
		return;
#endif
	}

	if (Optimizer != gcvNULL)
	{
		dumpOptimizer = gcvTRUE;
		shader = Optimizer->shader;
	}

	/* Print header. */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%s", doubleDashLine));
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%s\n", Text));
	gcOpt_DumpBuffer(os, File, buffer, offset);

	if (dumpOptimizer)
	{
		functionCount = Optimizer->functionCount;
		codeCount = Optimizer->codeTail->id + 1;
        code = &Optimizer->codeHead->instruction;
	}
	else
	{
	    if(shader == gcvNULL)
	    {
	        gcmASSERT(shader);
	        gcmFOOTER_NO();
	        return;
	    }
#if __SUPPORT_OCL__
        functionCount = shader->functionCount + shader->kernelFunctionCount;
#else
		functionCount = shader->functionCount;
#endif
		codeCount = shader->codeCount;
		code = shader->code;
	}

	/***************************************************************************
	********************************************************* Dump functions. **
	***************************************************************************/

	if (functionCount > 0)
	{
		offset = 0;

		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   "%s[FUNCTIONS]\n\n  main() := [%u-%u]\n",
							   dashLine, 0, codeCount - 1));
		gcOpt_DumpBuffer(os, File, buffer, offset);

		/* Walk all functions. */
		for (i = 0; i < functionCount; ++i)
		{
            gctSIZE_T nameLength;
            char * name;
			gctUINT codeStart, codeEnd;
            gctSIZE_T argumentCount;
            gcsFUNCTION_ARGUMENT_PTR arguments;
			gctSIZE_T j;

			offset = 0;

			if (dumpOptimizer)
			{
#if __SUPPORT_OCL__
                if (Optimizer->functionArray[i].shaderFunction)
                {
				    gcFUNCTION shaderFunction = Optimizer->functionArray[i].shaderFunction;
				    codeStart = Optimizer->functionArray[i].codeHead->id;
				    codeEnd   = Optimizer->functionArray[i].codeTail->id;
                    nameLength = shaderFunction->nameLength;
                    name       = shaderFunction->name;
                    argumentCount = shaderFunction->argumentCount;
                    arguments     = shaderFunction->arguments;
                }
                else
                {
				    gcKERNEL_FUNCTION kernelFunction = Optimizer->functionArray[i].kernelFunction;
				    codeStart = Optimizer->functionArray[i].codeHead->id;
				    codeEnd   = Optimizer->functionArray[i].codeTail->id;
                    nameLength = kernelFunction->nameLength;
                    name       = kernelFunction->name;
                    argumentCount = kernelFunction->argumentCount;
                    arguments     = kernelFunction->arguments;
                }
#else
				gcFUNCTION shaderFunction = Optimizer->functionArray[i].shaderFunction;
				codeStart = Optimizer->functionArray[i].codeHead->id;
				codeEnd   = Optimizer->functionArray[i].codeTail->id;
                nameLength = shaderFunction->nameLength;
                name       = shaderFunction->name;
                argumentCount = shaderFunction->argumentCount;
                arguments     = shaderFunction->arguments;
#endif
			}
			else
			{
#if __SUPPORT_OCL__
                if (i < shader->functionCount)
                {
				    gcFUNCTION shaderFunction = shader->functions[i];
				    codeStart = shaderFunction->codeStart;
				    codeEnd   = codeStart + shaderFunction->codeCount - 1;
                    nameLength = shaderFunction->nameLength;
                    name       = shaderFunction->name;
                    argumentCount = shaderFunction->argumentCount;
                    arguments     = shaderFunction->arguments;
                }
                else
                {
				    gcKERNEL_FUNCTION kernelFunction = shader->kernelFunctions[i - shader->functionCount];
				    codeStart = kernelFunction->codeStart;
				    codeEnd   = kernelFunction->codeEnd - 1;
                    nameLength = kernelFunction->nameLength;
                    name       = kernelFunction->name;
                    argumentCount = kernelFunction->argumentCount;
                    arguments     = kernelFunction->arguments;
                }
#else
				gcFUNCTION shaderFunction = shader->functions[i];
				codeStart = shaderFunction->codeStart;
				codeEnd   = codeStart + shaderFunction->codeCount - 1;
                nameLength = shaderFunction->nameLength;
                name       = shaderFunction->name;
                argumentCount = shaderFunction->argumentCount;
                arguments     = shaderFunction->arguments;
#endif
			}

			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "\n  "));

			offset += _DumpName(nameLength,
							    name,
							    buffer + offset,
							    gcmSIZEOF(buffer) - offset);

			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   "() := [%u-%u]\n",
								   codeStart, codeEnd));
			gcOpt_DumpBuffer(os, File, buffer, offset);

			for (j = 0; j < argumentCount; ++j)
			{
				offset = 0;

				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "    argument(%u) := temp(%u)",
									   j, arguments[j].index));

				switch (arguments[j].enable)
				{
				case 0x1:
					gcmVERIFY_OK(
						gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
										   ".x"));
					break;

				case 0x3:
					gcmVERIFY_OK(
						gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
										   ".xy"));
					break;

				case 0x7:
					gcmVERIFY_OK(
						gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
										   ".xyz"));
					break;
				}

				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "\n"));
				gcOpt_DumpBuffer(os, File, buffer, offset);
			}
		}
	}

	/***************************************************************************
	** I: Dump shader.
	*/

	offset = 0;
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%s\n[SHADER]\n\n", dashLine));
	gcOpt_DumpBuffer(os, File, buffer, offset);

	/* Walk all attributes. */
	for (i = 0; i < shader->attributeCount; i++)
	{
		/* Get the gcATTRIBUTE pointer. */
		gcATTRIBUTE attribute = shader->attributes[i];

		if (attribute != gcvNULL)
		{
			offset = 0;

			/* Attribute header and type. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   "  attribute(%d) := %s ",
								   i, type[attribute->type]));

			/* Append name. */
			offset += _DumpName(attribute->nameLength,
							    attribute->name,
							    buffer + offset,
							    gcmSIZEOF(buffer) - offset);

			if (attribute->arraySize > 1)
			{
				/* Append array size. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "[%d]", attribute->arraySize));
			}

			/* Dump the attribute. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ";\n"));
			gcOpt_DumpBuffer(os, File, buffer, offset);
		}
	}

	/* Walk all uniforms. */
	for (i = 0; i < shader->uniformCount; i++)
	{
		/* Get the gcUNIFORM pointer. */
		gcUNIFORM uniform = shader->uniforms[i];

		if (uniform != gcvNULL)
		{
			offset = 0;

			/* Uniform header and type. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   "  uniform(%d) := %s ",
								   i, type[uniform->type]));

			/* Append name. */
			offset += _DumpName(uniform->nameLength,
							    uniform->name,
							    buffer + offset,
							    gcmSIZEOF(buffer) - offset);

			if (uniform->arraySize > 1)
			{
				/* Append array size. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "[%d]", uniform->arraySize));
			}

			/* Dump the uniform. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ";\n"));
			gcOpt_DumpBuffer(os, File, buffer, offset);
		}
	}

	/* Walk all outputs. */
	for (i = 0; i < shader->outputCount; i++)
	{
		/* Get the gcOUTPUT pointer. */
		gcOUTPUT output = shader->outputs[i];

		if (output != gcvNULL)
		{
			offset = 0;

			/* Output header and type. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   "  output(%d) := %s ",
								   i, type[output->type]));

			/* Append name. */
			offset += _DumpName(output->nameLength,
							    output->name,
							    buffer + offset, gcmSIZEOF(buffer) - offset);

			if (output->arraySize > 1)
			{
				/* Append array size. */
				gcmVERIFY_OK(
					gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									   "[%d]", output->arraySize));
			}

			/* Append assigned temporary register. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
								   " ==> temp(%d)", output->tempIndex));

			/* Dump the output. */
			gcmVERIFY_OK(
				gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ";\n"));
			gcOpt_DumpBuffer(os, File, buffer, offset);
		}
	}

	if (dumpOptimizer)
	{
		codeCount = Optimizer->codeTail->id + 1;
		codeNode = Optimizer->codeHead;
        code = &codeNode->instruction;
	}
	else
	{
		codeCount = shader->codeCount;
		code = shader->code;
	}

    /* Walk all code. */
	for (i = 0; i < codeCount; i++)
	{
		_DumpIR(os, File, i, code);

        /* Get next code. */
        if (dumpOptimizer)
        {
            codeNode = codeNode->next;
            if (codeNode == gcvNULL)
            {
                break;
            }
            code = &codeNode->instruction;
        }
        else
        {
            code++;
        }
	}

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */
    if (shader->ltcUniformCount > 0)
    {

        offset = 0;
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   " ltcUniformCount (%d), ltcUniformBegin (%d) ",
                               shader->ltcUniformCount, shader->ltcUniformBegin));
        gcOpt_DumpBuffer(os, File, buffer, offset);

        for (i = 0; i < (gctUINT)shader->ltcInstructionCount; i++)
        {
            if (shader->ltcCodeUniformIndex[i] == -1 ) continue;
            offset = 0;
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
				                   " \t ltcCodeUniformIndex[%d] = %d ",
                                   i, shader->ltcCodeUniformIndex[i]));
            gcOpt_DumpBuffer(os, File, buffer, offset);
        }


        /* LTC expressions */
        offset = 0;
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							   " ltcInstructionCount (%d)",
                               shader->ltcInstructionCount));
        gcOpt_DumpBuffer(os, File, buffer, offset);

        /* Walk all code. */
        gcOpt_DumpBuffer(os, File, buffer, offset);
		code = shader->ltcExpressions;
	    for (i = 0; i < shader->ltcInstructionCount; i++)
	    {
		    _DumpIR(os, File, i, code);

            code++;
	    }
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

    if (dumpOptimizer)
    {
	    /***************************************************************************
	    ** II: Dump data flow.
	    */

	    /* Print header. */
	    offset = 0;
	    gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
									    "\n%s\n[DATA FLOW]\n", dashLine));
	    gcOpt_DumpBuffer(os, File, buffer, offset);

	    /* Process functions first. */
	    for (i = 0; i < Optimizer->functionCount; i++)
	    {
		    gcOPT_FUNCTION function = Optimizer->functionArray + i;
#if __SUPPORT_OCL__
            gctSIZE_T nameLength;
            char * name;
#else
		    gcFUNCTION shaderFunction = function->shaderFunction;
#endif
		    offset = 0;
		    gcmVERIFY_OK(
			    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "\n  "));

#if __SUPPORT_OCL__
            if (function->shaderFunction)
            {
				gcFUNCTION shaderFunction = function->shaderFunction;
                nameLength = shaderFunction->nameLength;
                name       = shaderFunction->name;
            }
            else
            {
				gcKERNEL_FUNCTION kernelFunction = function->kernelFunction;
                nameLength = kernelFunction->nameLength;
                name       = kernelFunction->name;
            }
		    offset += _DumpName(nameLength,
						        name,
						        buffer + offset, gcmSIZEOF(buffer) - offset);
#else
		    offset += _DumpName(shaderFunction->nameLength,
						        shaderFunction->name,
						        buffer + offset, gcmSIZEOF(buffer) - offset);
#endif

		    gcmVERIFY_OK(
			    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
							       "() : [%u - %u]\n",
                                   function->codeHead->id, function->codeTail->id));
		    gcOpt_DumpBuffer(os, File, buffer, offset);

		    _DumpDataFlow(os, File, function);
	    }

	    /* Process main program. */
	    offset = 0;
	    gcmVERIFY_OK(
		    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						       "\n  main() : [%u - %u]\n",
						       Optimizer->main->codeHead->id,
                               Optimizer->main->codeTail->id));
	    gcOpt_DumpBuffer(os, File, buffer, offset);

	    _DumpDataFlow(os, File, Optimizer->main);
    }

	offset = 0;
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "%s", doubleDashLine));
	gcOpt_DumpBuffer(os, File, buffer, offset);

    if (File != gcvNULL)
    {
        gcoOS_Flush(os, File);
    }

	gcmFOOTER_NO();
}
#endif /* VIVANTE_NO_3D */


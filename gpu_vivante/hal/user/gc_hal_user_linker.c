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




#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

#define _GC_OBJ_ZONE    gcvZONE_COMPILER

#define _USE_NEW_VARIABLE_HANDLER_  0

/*******************************************************************************
*********************************************************** Shader Compacting **
*******************************************************************************/

typedef struct _gcsJUMP *       gcsJUMP_PTR;
typedef struct _gcsJUMP
{
    gcsJUMP_PTR                 next;
    gctINT                      from;
}
gcsJUMP;

gceSTATUS
CompactShader(
    IN gcoOS Os,
    IN gcSHADER Shader
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsJUMP_PTR * jumpTable = gcvNULL;
    gctSIZE_T i;
    gctPOINTER pointer = gcvNULL;

    do
    {
        if (Shader->codeCount == 0)
        {
            break;
        }

        /* Allocate array of pointers. */
        gcmERR_BREAK(gcoOS_Allocate(Os,
                                    gcmSIZEOF(gcsJUMP_PTR) * Shader->codeCount,
                                    &pointer));

        jumpTable = pointer;

        /* Initialize array of pointers to NULL. */
        gcmERR_BREAK(
            gcoOS_ZeroMemory(jumpTable,
                             gcmSIZEOF(gcsJUMP_PTR) * Shader->codeCount));

        /* Walk all instructions. */
        for (i = 0; i < Shader->codeCount; ++i)
        {
            gcSL_INSTRUCTION code;
            gctINT target;

            /* Get pointer to instruction. */
            code = Shader->code + i;

            /* Find JMP opcodes. */
            switch (code->opcode)
            {
            case gcSL_JMP:
                /* Check for a JMP.ALWAYS. */
                if (gcmSL_TARGET_GET(Shader->code[i].temp, Condition) == gcSL_ALWAYS)
                {
                    /* Get target of JMP. */
                    target = code->tempIndex;
                    break;
                }

                /* fall through */

            default:
                target = -1;
                break;
            }

            if ((target >= 0) && (target < (gctINT) Shader->codeCount))
            {
                gcsJUMP_PTR jump;

                /* Allocate a new gcsJUMP structure. */
                gcmERR_BREAK(gcoOS_Allocate(Os,
                                            gcmSIZEOF(gcsJUMP),
                                            &pointer));

                jump = pointer;

                /* Set location being jumped from. */
                jump->from = i;

                /* Link gcsJUMP structure to target instruction. */
                jump->next        = jumpTable[target];
                jumpTable[target] = jump;
            }
        }

        if (gcmIS_ERROR(status))
        {
            /* Break on error. */
            break;
        }

        /* Walk all entries in array of pointers. */
        for (i = 0; i < Shader->codeCount; ++i)
        {
            const gctSIZE_T size = gcmSIZEOF(struct _gcSL_INSTRUCTION);
            gcsJUMP_PTR jump, modify;

            for (jump = jumpTable[i]; jump != gcvNULL; jump = jump->next)
            {
                if (jump->from == -1)
                {
                    continue;
                }

                for (modify = jump->next;
                     modify != gcvNULL;
                     modify = modify->next)
                {
                    gctINT source = jump->from;
                    gctINT target = modify->from;

                    if (target == -1)
                    {
                        continue;
                    }

                    while (gcmIS_SUCCESS(gcoOS_MemCmp(Shader->code + source,
                                                      Shader->code + target,
                                                      size)))
                    {
                        if (jumpTable[target] != gcvNULL)
                        {
                            break;
                        }

                        --source;
                        --target;

                        if ((source < 0) || (target < 0))
                        {
                            break;
                        }
                    }

                    if (modify->from - ++target > 0)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(Shader->code + target,
                                                   Shader->code + modify->from,
                                                   size));

                        Shader->code[target].tempIndex = (gctUINT16) (source + 1);

                        while (target++ != modify->from)
                        {
                            gcmVERIFY_OK(gcoOS_ZeroMemory(Shader->code + target,
                                                          size));

                            #if defined(gcSL_NOP) && gcSL_NOP
                                Shader->code[target].opcode = gcSL_NOP;
                            #endif
                        }

                        modify->from = -1;
                    }
                }
            }
        }
    }
    while (gcvFALSE);

    if (jumpTable != gcvNULL)
    {
        /* Free up the entire array of pointers. */
        for (i = 0; i < Shader->codeCount; ++i)
        {
            /* Free of the linked list of gcsJUMP structures. */
            while (jumpTable[i] != gcvNULL)
            {
                gcsJUMP_PTR jump = jumpTable[i];
                jumpTable[i] = jump->next;

                gcmVERIFY_OK(gcmOS_SAFE_FREE(Os, jump));
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(Os, jumpTable));
    }

    /* Return the status. */
    return status;
}

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
#   define DUMP(Text, Tree)     _Print(Text, Tree)
#else
#   define DUMP(Text, Tree)
#endif

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
/*******************************************************************************
**                                  _GetName
********************************************************************************
**
**  Print the name.
**
**  INPUT:
**
**      gctSIZE_T Length
**          Length of the name.
**
**      gctCONST_STRING Name
**          Pointer to the name.
**
**  OUTPUT:
**
**      char * Buffer
**          Pointer to the buffer to be used as output.
**
**  RETURN VALUE:
**
**      gctINT
**          The number of characters copied into the output buffer.
*/
static gctINT
_GetName(
    IN gctSIZE_T Length,
    IN gctCONST_STRING Name,
    IN char * Buffer,
    IN gctSIZE_T BufferSize
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
        /* Predefined frag depth. */
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

    default:
        break;
    }

    /* Copy the name into the buffer. */
    size = gcmMIN(BufferSize - 1, Length);
    gcmVERIFY_OK(gcoOS_MemCopy(Buffer, Name, size));
    Buffer[size] = '\0';
    return (gctINT) size;
}

/*******************************************************************************
**                                  _Swizzle
********************************************************************************
**
**  Print the swizzle.
**
**  INPUT:
**
**      gcSL_SWIZZLE SwizzleX
**          Swizzle for the x component.
**
**      gcSL_SWIZZLE SwizzleY
**          Swizzle for the y component.
**
**      gcSL_SWIZZLE SwizzleW
**          Swizzle for the w component.
**
**      gcSL_SWIZZLE SwizzleZ
**          Swizzle for the z component.
**
**  OUTPUT:
**
**      char * Buffer
**          Pointer to the buffer to be used as output.
**
**  RETURN VALUE:
**
**      gctINT
**          The number of characters copied into the output buffer.
*/
static gctINT
_Swizzle(
    IN gcSL_SWIZZLE SwizzleX,
    IN gcSL_SWIZZLE SwizzleY,
    IN gcSL_SWIZZLE SwizzleZ,
    IN gcSL_SWIZZLE SwizzleW,
    IN char * Buffer,
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
**                                  _Register
********************************************************************************
**
**  Print the format, type, and addressing mode of a register.
**
**  INPUT:
**
**      gcSL_TYPE Type
**          Type of the register.
**
**      gcSL_FORMAT Format
**          Format of the register.
**
**      gcSL_INDEX Index
**          Index of the register.
**
**      gcSL_INDEXED Mode
**          Addressing mode for the register.
**
**      gctINT Indexed
**          Indexed register if addressing mode is indexed.
**
**      gctBOOL Physical
**          Use physical registers instead of logical ones.
**
**  OUTPUT:
**
**      char * Buffer
**          Pointer to the buffer to be used as output.
**
**  RETURN VALUE:
**
**      gctINT
**          The number of characters copied into the output buffer.
*/
static gctINT
_Register(
    IN gcSL_TYPE Type,
    IN gcSL_FORMAT Format,
    IN gctUINT16 Index,
    IN gcSL_INDEXED Mode,
    IN gctINT Indexed,
    IN gctBOOL Physical,
    IN char * Buffer,
    IN gctSIZE_T BufferSize
    )
{
    gctUINT offset = 0;

    static gctCONST_STRING type[] =
    {
        "instruction",          /* Use gcSL_NONE as an instruction. */
        "temp",
        "attribute",
        "uniform",
        "sampler",
        "constant",
        "output",
    };

    static gctCONST_STRING physicalType[] =
    {
        "",
        "r",
        "v",
        "c",
        "s",
        "c",
        "o",
    };

    static gctCONST_STRING format[] =
    {
        "",                     /* Don't append anything for float registers. */
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
                           "%s",
                           Physical ? physicalType[Type] : type[Type]));
    gcmVERIFY_OK(
        gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
                           "%s(%d",
                           format[Format],
                           gcmSL_INDEX_GET(Index, Index)));

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
                               Physical ? physicalType[gcSL_TEMP]
                                        : type[gcSL_TEMP]));
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
    gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, ")"));

    /* Return the number of characters printed. */
    return offset;
}

/*******************************************************************************
**                                   _Source
********************************************************************************
**
**  Print a source operand.
**
**  INPUT:
**
**      gctUINT16 Source
**          The source operand.
**
**      gctUINT16 Index
**          Index of the source operand.
**
**      gctINT Indexed
**          Index register if the source operand addressing mode is indexed.
**
**      gctBOOL AddComma
**          Prepend the source operand with a comma.
**
**      gctBOOL Physical
**          Use physical registers instead of logical ones.
**
**  OUTPUT:
**
**      char * Buffer
**          Pointer to the buffer to be used as output.
**
**  RETURN VALUE:
**
**      gctINT
**          The number of characters copied into the output buffer.
*/
static gctINT
_Source(
    IN gctUINT16 Source,
    IN gctUINT16 Index,
    IN gctINT Indexed,
    IN gctBOOL AddComma,
    IN gctBOOL Physical,
    IN char * Buffer,
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
            gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferSize, &offset, ", "));
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
                                       "%f", *(float *) &value));
                break;

            case gcSL_INTEGER:
                /* Print the integer constant. */
                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
                                       "%d", value));
                break;

            case gcSL_BOOLEAN:
                /* Print the boolean constant. */
                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
                                       "%s", value ? "gcvTRUE" : "gcvFALSE"));
                break;

            case gcSL_UINT32:
                /* Print the integer constant. */
                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(Buffer, BufferSize, &offset,
                                       "%u", value));
                break;

            default:
                break;
            }
        }
        else
        {
            /* Decode the register. */
            offset += _Register((gcSL_TYPE) gcmSL_SOURCE_GET(Source, Type),
                                (gcSL_FORMAT) gcmSL_SOURCE_GET(Source, Format),
                                Index,
                                (gcSL_INDEXED) gcmSL_SOURCE_GET(Source, Indexed),
                                Indexed,
                                Physical,
                                Buffer + offset,
                                BufferSize - offset);

            /* Decode the swizzle. */
            offset += _Swizzle((gcSL_SWIZZLE) gcmSL_SOURCE_GET(Source, SwizzleX),
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
**                                    _List
********************************************************************************
**
**  Print all entries in a gcsLINKTREE_LIST list.
**
**  INPUT:
**
**      gctCONST_STRING Title
**          Pointer to the title for the list.
**
**      gcsLINKTREE_LIST_PTR List
**          Pointer to the first gcsLINKTREE_LIST_PTR structure to print.
**
**      gctBOOL Physical
**          Use physical registers instead of logical ones.
**
**  OUTPUT:
**
**      Nothing.
*/
static void
_List(
    IN gctCONST_STRING Title,
    IN gcsLINKTREE_LIST_PTR List,
    IN gctBOOL Physical
    )
{
    char buffer[256];
    gctUINT offset = 0;
    gctSIZE_T length;

    gcmVERIFY_OK(gcoOS_StrLen(Title, &length));
    length = gcmMIN(length, gcmSIZEOF(buffer) - 1);

    if (List == gcvNULL)
    {
        /* Don't print anything on an empty list. */
        return;
    }

    /* Print the title. */
    gcmVERIFY_OK(
        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, Title));

    /* Walk while there are entries in the list. */
    while (List != gcvNULL)
    {
        /* Check if we have reached the right margin. */
        if (offset > 70)
        {
            /* Dump the assembled line. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s,", buffer);

            /* Indent to the end of the title. */
            for (offset = 0; offset < length; offset++)
            {
                buffer[offset] = ' ';
            }
        }
        else if (offset > length)
        {
            /* Print comma and space. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, ", "));
        }

        /* Decode the register. */
        offset += _Register(List->type,
                            gcSL_FLOAT,
                            (gctUINT16) List->index,
                            gcSL_NOT_INDEXED,
                            -1,
                            Physical,
                            buffer + offset,
                            gcmSIZEOF(buffer) - offset);

        /* Move to the next entry in the list. */
        List = List->next;
    }

    /* Dump the final assembled line. */
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,"%s",  buffer);
}

/*******************************************************************************
**                                   _Print
********************************************************************************
**
**  Print the shader and dependency tree for debugging.
**
**  INPUT:
**
**      gctCONST_STRING Text
**          Pointer to text to print before dumping shader and tree.
**
**      gcLINKTREE Tree
**          Pointer to the gcLINKTREE structure to print.
**
**  OUTPUT:
**
**      gcLINKTREE * Tree
**          Pointer to a variable receiving the gcLINKTREE structure pointer.
*/
static void
_Print(
    IN gctCONST_STRING Text,
    IN gcLINKTREE Tree
    )
{
    gctSIZE_T i;
    char buffer[256];
    gcSHADER shader = Tree->shader;

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
		"image2d",
		"image3d",
		"sampler",
    };

    static struct _DECODE
    {
        gctCONST_STRING opcode;
        gctBOOL         hasTarget;
        gctBOOL         hasLabel;
    }
	/* Must be in sync with gcSL_OPCODE */
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

    /* Print header. */
    for (i = 0; i < 79; i++)
    {
        buffer[i] = '=';
    }
    buffer[i] = '\0';

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", Text);

    /***************************************************************************
    ********************************************************* Dump functions. **
    ***************************************************************************/

    if (shader->functionCount > 0)
    {
        for (i = 0; i < 79; i++)
        {
            buffer[i] = '*';
        }
        buffer[i] = '\0';

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "[FUNCTIONS]");
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "  main() := [%u-%u]",
                      0,
                      shader->codeCount - 1);

        /* Walk all functions. */
        for (i = 0; i < shader->functionCount; ++i)
        {
            gcFUNCTION function = shader->functions[i];
            gctUINT offset = 0;
            gctSIZE_T j;

            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

            offset = 0;
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "  "));

            offset += _GetName(function->nameLength,
                               function->name,
                               buffer + offset,
                               gcmSIZEOF(buffer) - offset);

            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "() := [%u-%u]",
                                   function->codeStart,
                                   function->codeStart + function->codeCount
                                                       - 1));

            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

            for (j = 0; j < function->argumentCount; ++j)
            {
                offset = 0;

                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                       "    argument(%u) := temp(%u)",
                                       j, function->arguments[j].index));

                switch (function->arguments[j].enable)
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

                default:
                    break;
                }

                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
            }
        }
    }
    /***************************************************************************
    ************************************************** Dump kernel functions. **
    ***************************************************************************/

    if (shader->kernelFunctionCount > 0)
    {
        for (i = 0; i < 79; i++)
        {
            buffer[i] = '*';
        }
        buffer[i] = '\0';

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "[KERNEL FUNCTIONS]");
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "  main() := [%u-%u]",
                      0,
                      shader->codeCount - 1);

        /* Walk all kernel functions. */
        for (i = 0; i < shader->kernelFunctionCount; ++i)
        {
            gcKERNEL_FUNCTION kernelFunction = shader->kernelFunctions[i];
            gctUINT offset = 0;
            gctSIZE_T j;

            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

            offset = 0;
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "  "));

            offset += _GetName(kernelFunction->nameLength,
                               kernelFunction->name,
                               buffer + offset,
                               gcmSIZEOF(buffer) - offset);

            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "() := [%u-%u]",
                                   kernelFunction->codeStart,
                                   kernelFunction->codeEnd-1));

            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

            for (j = 0; j < kernelFunction->argumentCount; ++j)
            {
                offset = 0;

                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                       "    argument(%u) := temp(%u)",
                                       j, kernelFunction->arguments[j].index));

                switch (kernelFunction->arguments[j].enable)
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

                default:
                    break;
                }

                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
            }
        }
    }

    /***************************************************************************
    ** I: Dump shader.
    */

    for (i = 0; i < 79; i++)
    {
        buffer[i] = '*';
    }
    buffer[i] = '\0';

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "[SHADER]");
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");

    /* Walk all attributes. */
    for (i = 0; i < shader->attributeCount; i++)
    {
        /* Get the gcATTRIBUTE pointer. */
        gcATTRIBUTE attribute = shader->attributes[i];

        if (attribute != gcvNULL)
        {
            gctUINT offset = 0;

            /* Attribute header and type. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "  attribute(%d) := %s ",
                                   i, type[attribute->type]));

            /* Append name. */
            offset += _GetName(attribute->nameLength,
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

            if (attribute->isTexture)
            {
                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                       " (Texture)"));
            }

            /* Dump the attribute. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s;", buffer);
        }
    }

    /* Walk all uniforms. */
    for (i = 0; i < shader->uniformCount; i++)
    {
        /* Get the gcUNIFORM pointer. */
        gcUNIFORM uniform = shader->uniforms[i];

        if (uniform != gcvNULL)
        {
            gctUINT offset = 0;

            /* Uniform header and type. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer),
                                    &offset,
                                    "  uniform(%d) := %s ",
                                    i,
                                    type[uniform->type]));

            /* Append name. */
            offset += _GetName(uniform->nameLength,
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
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s;", buffer);
        }
    }

    /* Walk all outputs. */
    for (i = 0; i < shader->outputCount; i++)
    {
        /* Get the gcOUTPUT pointer. */
        gcOUTPUT output = shader->outputs[i];

        if (output != gcvNULL)
        {
            gctUINT offset = 0;

            /* Output header and type. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "  output(%d) := %s ",
                                   i, type[output->type]));

            /* Append name. */
            offset += _GetName(output->nameLength,
                               output->name,
                               buffer + offset,
                               gcmSIZEOF(buffer) - offset);

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
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s;", buffer);
        }
    }

    /* Walk all code. */
    for (i = 0; i < shader->codeCount; i++)
    {
        gcSL_INSTRUCTION code;
        gctUINT16 target;
        gctUINT offset = 0;

        /* Get pointers. */
        code   = &shader->code[i];
        target = code->temp;

        /* Instruction number, opcode and condition. */
        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                               "  %4d: %s",
                               i,
                               decode[code->opcode].opcode));
        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                               "%s",
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
            offset += _Register(gcSL_TEMP,
                                (gcSL_FORMAT) gcmSL_TARGET_GET(target, Format),
                                code->tempIndex,
                                (gcSL_INDEXED) gcmSL_TARGET_GET(target, Indexed),
                                code->tempIndexed,
                                Tree->physical,
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
        offset += _Source(code->source0,
                          code->source0Index,
                          code->source0Indexed,
                          offset > 24,
                          Tree->physical,
                          buffer + offset,
                          gcmSIZEOF(buffer) - offset);

        /* Append source operand 1. */
        offset += _Source(code->source1,
                          code->source1Index,
                          code->source1Indexed,
                          offset > 24,
                          Tree->physical,
                          buffer + offset,
                          gcmSIZEOF(buffer) - offset);

        /* Dump the instruction. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
    }

    /***************************************************************************
    ** II: Dump dependency tree.
    */

    /* Print header. */
    for (i = 0; i < 79; i++)
    {
        buffer[i] = '*';
    }
    buffer[i] = '\0';

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "[TREE]");
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "");

    /* Walk all attributes. */
    for (i = 0; i < Tree->attributeCount; i++)
    {
        /* Get the gcLINKTREE_ATTRIBUTE pointer. */
        gcLINKTREE_ATTRIBUTE attribute = Tree->attributeArray + i;

        /* Only dump valid attributes. */
        if (attribute->lastUse < 0)
        {
            continue;
        }

        /* Dump the attribute life span. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "  Attribute %d:",
                      i);
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "    No longer referenced after instruction %d",
                      attribute->lastUse);

        /* Dump the users of the attribute. */
        _List("    Users: ", attribute->users, gcvFALSE);
    }

    for (i = 0; i < Tree->tempCount; i++)
    {
        /* Get the gcLINKTREE_TEMP pointer. */
        gcLINKTREE_TEMP temp = Tree->tempArray + i;
        gctUINT offset = 0;

        /* Only process if temporary register is defined. */
        if (temp->defined == gcvNULL)
        {
            continue;
        }

        /* Header of the temporary register. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "  Temp %d:", i);

        /* Check if this temporay register is an attribute. */
        if (temp->owner != gcvNULL)
        {
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "    %s attribute for function ",
                                   (temp->inputOrOutput == gcvFUNCTION_INPUT)
                                       ? "Input" :
                                   (temp->inputOrOutput == gcvFUNCTION_OUTPUT)
                                       ? "Output" : "Inout"));

			if (temp->isOwnerKernel) {
				offset += _GetName(((gcKERNEL_FUNCTION)temp->owner)->nameLength,
								   ((gcKERNEL_FUNCTION)temp->owner)->name,
								   buffer + offset,
								   gcmSIZEOF(buffer) - offset);
			} else {
				offset += _GetName(((gcFUNCTION)temp->owner)->nameLength,
								   ((gcFUNCTION)temp->owner)->name,
								   buffer + offset,
								   gcmSIZEOF(buffer) - offset);
			}

            /* Dump the buffer. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
        }
        else
        {
            /* Life span of the temporary register. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                          "    No longer referenced after instruction %d",
                          temp->lastUse);
        }

        offset = 0;

        /* Check if this temporay register is a variable. */
        if (temp->variable != gcvNULL)
        {
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "    Variable: %s",
                                   temp->variable->name));

            /* Dump the buffer. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

            offset = 0;
        }

        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                               "    Usage: ."));

        if (temp->usage & gcSL_ENABLE_X)
        {
            /* X is used. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "x"));
        }

        if (temp->usage & gcSL_ENABLE_Y)
        {
            /* Y is used. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "y"));
        }

        if (temp->usage & gcSL_ENABLE_Z)
        {
            /* Z is used. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "z"));
        }

        if (temp->usage & gcSL_ENABLE_W)
        {
            /* W is used. */
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "w"));
        }

        /* Dump the temporary register. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);

        /* Dump the definitions of the temporary register. */
        _List("    Definitions: ", temp->defined, gcvFALSE);

        /* Dump the dependencies of the temporary register. */
        _List("    Dependencies: ", temp->dependencies, gcvFALSE);

        /* Dump the constant values for the components. */
        if ((temp->constUsage[0] == 1)
        ||  (temp->constUsage[1] == 1)
        ||  (temp->constUsage[2] == 1)
        ||  (temp->constUsage[3] == 1)
        )
        {
            gctINT index;
            gctUINT8 usage = temp->usage;

            offset = 0;
            gcmVERIFY_OK(
                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                   "    Constants: {"));

            for (index = 0; (index < 4) && (usage != 0); ++index, usage >>= 1)
            {
                if (offset > 16)
                {
                    gcmVERIFY_OK(
                        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                           ", "));
                }

                if (temp->constUsage[index] == 1)
                {
                    gcmVERIFY_OK(
                        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                           "%f", temp->constValue[index]));
                }
                else if (temp->constUsage[index] == -1)
                {
                    gcmVERIFY_OK(
                        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                           "xxx"));
                }
                else
                {
                    gcmVERIFY_OK(
                        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                           "---"));
                }
            }

            if (offset > 16)
            {
                gcmVERIFY_OK(
                    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                       "}"));

                gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
            }
        }

        /* Dump the users of the temporary register. */
        _List("    Users: ", temp->users, gcvFALSE);

        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "    Last Use: %d", temp->lastUse);
	}

    /* Walk all outputs. */
    for (i = 0; i < Tree->outputCount; i++)
    {
        /* Only process outputs that are not dead. */
        if (Tree->outputArray[i].tempHolding < 0)
        {
            continue;
        }

        /* Dump dependency. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "  Output %d:", i);
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                      "    Dependent on %s(%d)",
                      Tree->physical ? "r" : "temp",
                      Tree->outputArray[i].tempHolding);

        if (Tree->outputArray[i].fragmentAttribute >= 0)
        {
            /* Dump linkage. */
            gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER,
                          "    Linked to fragment index %d (attribute %d)",
                          Tree->outputArray[i].fragmentIndex,
                          Tree->outputArray[i].fragmentAttribute);
        }
    }

    /* Trailer. */
	for (i = 0; i < 79; i++)
    {
        buffer[i] = '=';
    }
    buffer[i] = '\0';

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "%s", buffer);
}


/*******************************************************************************
**                               dump_LinkTree
********************************************************************************
**
**  Print the shader and dependency tree for debugging.
**  Allways callable from debugger.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to the gcLINKTREE structure to print.
**
**  OUTPUT:
**
**      none
*/
void
dump_LinkTree(
			  IN gcLINKTREE Tree
			  )
{
	_Print("", Tree);
}

#endif /* gcmIS_DEBUG(gcdDEBUG_TRACE) */

/*******************************************************************************
**                            gcLINKTREE_Construct
********************************************************************************
**
**  Construct a new gcLINKTREE structure.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to an gcoOS object.
**
**  OUTPUT:
**
**      gcLINKTREE * Tree
**          Pointer to a variable receiving the gcLINKTREE structure pointer.
*/
gceSTATUS
gcLINKTREE_Construct(
    IN gcoOS Os,
    OUT gcLINKTREE * Tree
    )
{
    gceSTATUS status;
    gcLINKTREE tree = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Os=0x%x", Os);

    /* Allocate the gcLINKTREE structure. */
    gcmONERROR(
        gcoOS_Allocate(Os, sizeof(struct _gcLINKTREE), &pointer));

    tree = pointer;

    /* Initialize the gcLINKTREE structure.  */
    tree->shader         = gcvNULL;
    tree->attributeCount = 0;
    tree->attributeArray = gcvNULL;
    tree->tempCount      = 0;
    tree->tempArray      = gcvNULL;
    tree->outputCount    = 0;
    tree->outputArray    = gcvNULL;
    tree->physical       = gcvFALSE;
    tree->branch         = gcvNULL;
    tree->hints          = gcvNULL;

    /* Return the gcLINKTREE structure pointer. */
    *Tree = tree;

    /* Success. */
    gcmFOOTER_ARG("*Tree=0x%x", *Tree);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                             gcLINKTREE_Destroy
********************************************************************************
**
**  Destroy a gcLINKTREE structure.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcLINKTREE_Destroy(
    IN gcLINKTREE Tree
    )
{
    gctSIZE_T i;
    gcsLINKTREE_LIST_PTR node;

    gcmHEADER_ARG("Tree=0x%x", Tree);

    /* See if there are any attributes. */
    if (Tree->attributeArray != gcvNULL)
    {
        /* Walk all attributes. */
        for (i = 0; i < Tree->attributeCount; i++)
        {
            /* Loop while there are users. */
            while (Tree->attributeArray[i].users != gcvNULL)
            {
                /* Get the user. */
                node = Tree->attributeArray[i].users;

                /* Remove the user from the list. */
                Tree->attributeArray[i].users = node->next;

                /* Free the gcLINKTREE_LIST structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
            }
        }

        /* Free the array of attributes. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->attributeArray));
    }

    /* Verify if there are any temporary registers. */
    if (Tree->tempArray != gcvNULL)
    {
        /* Loop through all temporary registers. */
        for (i = 0; i < Tree->tempCount; i++)
        {
            /* Loop while there are definitions. */
            while (Tree->tempArray[i].defined != gcvNULL)
            {
                /* Get the definition. */
                node = Tree->tempArray[i].defined;

                /* Remove the definitions from the list. */
                Tree->tempArray[i].defined = node->next;

                /* Free the gcLINKTREE_LIST structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
            }

            /* Loop while there are dependencies. */
            while (Tree->tempArray[i].dependencies != gcvNULL)
            {
                /* Get the dependency. */
                node = Tree->tempArray[i].dependencies;

                /* Remove the dependency from the list. */
                Tree->tempArray[i].dependencies = node->next;

                /* Free the gcLINKTREE_LIST structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
            }

            /* Loop while we have users. */
            while (Tree->tempArray[i].users != gcvNULL)
            {
                /* Get the user. */
                node = Tree->tempArray[i].users;

                /* Remove the user from the list. */
                Tree->tempArray[i].users = node->next;

                /* Free the gcLINKTREE_LIST structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
            }
        }

        /* Free the array of temporary registers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->tempArray));
    }

    /* Verify if there are any output registers. */
    if (Tree->outputArray != gcvNULL)
    {
        /* Free the array of output registers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->outputArray));
    }

    /* Destroy any branches. */
    while (Tree->branch != gcvNULL)
    {
        /* Unlink branch from linked list. */
        gcSL_BRANCH_LIST branch = Tree->branch;
        Tree->branch = branch->next;

        /* Free the branch. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, branch));
    }

    /* Destroy any hints. */
    if (Tree->hints != gcvNULL)
    {
        for (i = 0; i < Tree->shader->codeCount; ++i)
        {
            /* Free any linked caller. */
            while (Tree->hints[i].callers != gcvNULL)
            {
                gcsCODE_CALLER_PTR caller = Tree->hints[i].callers;
                Tree->hints[i].callers = caller->next;

                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, caller));
            }
        }

        /* Free the hints. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->hints));
    }

    /* Free the gcLINKTREE structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**                             gcLINKTREE_AddList
********************************************************************************
**
**  Insert a new node into a linke list.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gcsLINKTREE_LIST_PTR * Root
**          Pointer to a variable holding the gcLINKTREE_LIST structure pointer.
**
**      gcSL_TYPE Type
**          Type for the new node.
**
**      gctINT Index
**          Index for the new node.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcLINKTREE_AddList(
    IN gcLINKTREE Tree,
    IN gcsLINKTREE_LIST_PTR * Root,
    IN gcSL_TYPE Type,
    IN gctINT Index
    )
{
    gceSTATUS status;
    gcsLINKTREE_LIST_PTR list;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Tree=0x%x Root=0x%x Type=%d Index=%d", Tree, Root, Type, Index);

    /* Walk all entries in the list. */
    for (list = *Root; list != gcvNULL; list = list->next)
    {
        /* Does the current list entry matches the new one? */
        if ( (list->type == Type) && (list->index == Index) )
        {
            /* Success. */
            ++list->counter;
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    /* Allocate a new gcsLINKTREE_LIST structure. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(gcsLINKTREE_LIST),
                       &pointer));

    list = pointer;

    /* Initialize the gcLINKTREE_LIST structure. */
    list->next    = *Root;
    list->type    = Type;
    list->index   = Index;
    list->counter = 1;

    /* Link the new gcLINKTREE_LIST structure into the list. */
    *Root = list;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                              _AttributeSource
********************************************************************************
**
**  Add an attribute as a source operand to the dependency tree.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gctUIN16 Index
**          Index for the attribute.
**
**      gctINT TempIndex
**          Temporary register using the attribute.  When 'TempIndex' is less
**          than zero, there is no temporary register and the user is a
**          conditional instruction.
**
**      gctINT InstructionCounter
**          Current instruction counter.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_AttributeSource(
    IN gcLINKTREE Tree,
    IN gctUINT16 Index,
    IN gctINT TempIndex,
    IN gctINT InstructionCounter
    )
{
    gcLINKTREE_ATTRIBUTE attribute;
    gceSTATUS status;
    gctUINT16 index = gcmSL_INDEX_GET(Index, Index);

    /* Get root for attribute. */
    gcmASSERT(index < Tree->attributeCount);
    attribute = &Tree->attributeArray[index];

    /* Update last usage of attribute. */
    attribute->lastUse = InstructionCounter;

    /* Add instruction user for attribute. */
    status = gcLINKTREE_AddList(Tree,
                                &attribute->users,
                                gcSL_NONE,
                                InstructionCounter);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    if (TempIndex >= 0)
    {
        /* Add attribute as dependency for target. */
        status = gcLINKTREE_AddList(Tree,
                                    &Tree->tempArray[TempIndex].dependencies,
                                    gcSL_ATTRIBUTE,
                                    index);
    }

    /* Return the status. */
    return status;
}

/*******************************************************************************
**                                 _TempSource
********************************************************************************
**
**  Add an temporary register as a source operand to the dependency tree.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gctSIZE_T Index
**          Index for the temporary register.
**
**      gctINT TempIndex
**          Temporary register using the temporary register.  When 'TempIndex'
**          is less than zero, there is no temporary register and the user is a
**          conditional instruction.
**
**      gctINT InstructionCounter
**          Current instruction counter.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_TempSource(
    IN gcLINKTREE Tree,
    IN gctSIZE_T Index,
    IN gctINT TempIndex,
    IN gctINT InstructionCounter
    )
{
    gcLINKTREE_TEMP temp;
    gceSTATUS status;

    /* Get root for temporary register. */
    gcmASSERT(Index < Tree->tempCount);
    temp = &Tree->tempArray[Index];

    /* Update last usage of temporary register. */
    temp->lastUse = InstructionCounter;

    /* Add instruction user for temporary register. */
    status = gcLINKTREE_AddList(Tree,
                                &temp->users,
                                gcSL_NONE,
                                InstructionCounter);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    if (TempIndex >= 0)
    {
        /* Add temporary register as dependency for target. */
        status = gcLINKTREE_AddList(Tree,
                                    &Tree->tempArray[TempIndex].dependencies,
                                    gcSL_TEMP,
                                    Index);
    }

    /* Return the status. */
    return status;
}

/*******************************************************************************
**                                 _IndexedSource
********************************************************************************
**
**  Add an temporary register as a source index to the dependency tree.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gctSIZE_T Index
**          Index for the temporary register.
**
**      gctINT InstructionCounter
**          Current instruction counter.
**
**      gctINT TempIndex
**          Temporary register using the temporary register.  When 'TempIndex'
**          is less than zero, there is no temporary register and the user is a
**          conditional instruction.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_IndexedSource(
    IN gcLINKTREE Tree,
    IN gctSIZE_T Index,
    IN gctINT InstructionCounter,
    IN gctINT TempIndex
    )
{
    gcLINKTREE_TEMP temp;
    gceSTATUS status;

    /* Get root for temporary register. */
    gcmASSERT(Index < Tree->tempCount);
    temp = &Tree->tempArray[Index];

    /* Update last usage of temporary register. */
    temp->lastUse = InstructionCounter;

    /* Mark the temporary register is used as an index. */
    temp->isIndex = gcvTRUE;

    /* Add instruction user for temporary register. */
    status = gcLINKTREE_AddList(Tree,
                                &temp->users,
                                gcSL_NONE,
                                InstructionCounter);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    if (TempIndex >= 0)
    {
        /* Add temporary register as dependency for target. */
        status = gcLINKTREE_AddList(Tree,
                                    &Tree->tempArray[TempIndex].dependencies,
                                    gcSL_TEMP,
                                    Index);
    }

    /* Return the status. */
    return status;
}

/*******************************************************************************
**                                   _Delete
********************************************************************************
**
**  Delete all nodes inside a linked list.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gcsLINKTREE_LIST_PTR * Root
**          Pointer to the variable holding the linked list pointer.
**
**  OUTPUT:
**
**      Nothing.
*/
static void
_Delete(
    IN gcLINKTREE Tree,
    IN gcsLINKTREE_LIST_PTR * Root
    )
{
    /* Loop while there are valid nodes. */
    while (*Root != gcvNULL)
    {
        /* Get the node pointer. */
        gcsLINKTREE_LIST_PTR node = *Root;

        /* Remove the node from the linked list. */
        *Root = node->next;

        /* Free the node. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
    }
}

static void
_FindCallers(
    IN gcLINKTREE Tree,
    IN gctPOINTER Owner,
    IN gctBOOL IsOwnerKernel,
    IN gctINT Nesting,
    IN OUT gctINT_PTR LastUse
    )
{
    gcsCODE_CALLER_PTR caller;

	if (IsOwnerKernel) {
		caller = Tree->hints[((gcKERNEL_FUNCTION)Owner)->codeStart].callers;
	} else {
		caller = Tree->hints[((gcFUNCTION)Owner)->codeStart].callers;
	}

    for (;
         caller != gcvNULL;
         caller = caller->next)
    {
        if ((Tree->hints[caller->caller].owner != gcvNULL)
        &&  (Tree->hints[caller->caller].callNest > Nesting)
        )
        {
            _FindCallers(Tree,
                         Tree->hints[caller->caller].owner,
                         Tree->hints[caller->caller].isOwnerKernel,
                         Nesting,
                         LastUse);
        }
        else if (*LastUse < 0 ||
                 Tree->hints[caller->caller].callNest < Tree->hints[*LastUse].callNest)
        {
            *LastUse = caller->caller;
        }
        else if (*LastUse < caller->caller)
        {
            *LastUse = caller->caller;
        }
    }
}

static
gctUINT _getTypeSize(gcSHADER_TYPE type)
{
    /* Number of temp registers needed for gcSHADER_TYPE. */
    static gctUINT typeSize[] =
    {
        1,  /* gcSHADER_FLOAT_X1 */
        1,  /* gcSHADER_FLOAT_X2 */
        1,  /* gcSHADER_FLOAT_X3 */
        1,  /* gcSHADER_FLOAT_X4 */
        2,  /* gcSHADER_FLOAT_2X2 */
        3,  /* gcSHADER_FLOAT_3X3 */
        4,  /* gcSHADER_FLOAT_4X4 */
        1,  /* gcSHADER_BOOLEAN_X1 */
        1,  /* gcSHADER_BOOLEAN_X2 */
        1,  /* gcSHADER_BOOLEAN_X3 */
        1,  /* gcSHADER_BOOLEAN_X4 */
        1,  /* gcSHADER_INTEGER_X1 */
        1,  /* gcSHADER_INTEGER_X2 */
        1,  /* gcSHADER_INTEGER_X3 */
        1,  /* gcSHADER_INTEGER_X4 */
        1,  /* gcSHADER_SAMPLER_1D */
        1,  /* gcSHADER_SAMPLER_2D */
        1,  /* gcSHADER_SAMPLER_3D */
        1,  /* gcSHADER_SAMPLER_CUBIC */
        1,  /* gcSHADER_FIXED_X1 */
        1,  /* gcSHADER_FIXED_X2 */
        1,  /* gcSHADER_FIXED_X3 */
        1,  /* gcSHADER_FIXED_X4 */
        1,  /* gcSHADER_IMAGE_2D for OCL*/
        1,  /* gcSHADER_IMAGE_3D for OCL*/
        1,  /* gcSHADER_SAMPLER for OCL*/
        2,  /* gcSHADER_FLOAT_2X3 */
        2,  /* gcSHADER_FLOAT_2X4 */
        3,  /* gcSHADER_FLOAT_3X2 */
        3,  /* gcSHADER_FLOAT_3X4 */
        4,  /* gcSHADER_FLOAT_4X2 */
        4,  /* gcSHADER_FLOAT_4X3 */
        1,  /* gcSHADER_ISAMPLER_2D */
        1,  /* gcSHADER_ISAMPLER_3D */
        1,  /* gcSHADER_ISAMPLER_CUBIC */
        1,  /* gcSHADER_USAMPLER_2D */
        1,  /* gcSHADER_USAMPLER_3D */
        1,  /* gcSHADER_USAMPLER_CUBIC */
        1,  /* gcSHADER_SAMPLER_EXTERNAL_OES */
    };

    return typeSize[type];
}

extern gceSTATUS
_GetVariableIndexingRange(
    IN gcSHADER Shader,
    IN gcVARIABLE variable,
    IN gctBOOL whole,
    OUT gctUINT *Start,
    OUT gctUINT *End);

/*******************************************************************************
**                              gcLINKTREE_Build
********************************************************************************
**
**  Build a tree based on a gcSHADER object.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gceSHADER_FLAGS Flags
**          Link flags.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcLINKTREE_Build(
    IN gcLINKTREE Tree,
    IN gcSHADER Shader,
    IN gceSHADER_FLAGS Flags
    )
{
    gcoOS os;
    gceSTATUS status = gcvSTATUS_OK;
    gctSIZE_T i, pc, f;
    gctSIZE_T tempCount, texAttrCount;
    gcSL_INSTRUCTION code;
    gctUINT16 source;
    gcLINKTREE_TEMP temp = gcvNULL;
    gctUINT16 target = 0;
    gctUINT16 index;
    gcsLINKTREE_LIST_PTR * texAttr = gcvNULL;
    gctINT nest;
    gctBOOL modified;
    gctPOINTER pointer = gcvNULL;

    /* Extract the gcoOS object pointer. */
    os = gcvNULL;

    /* Save the shader object. */
    Tree->shader = Shader;

    /***************************************************************************
    ** I - Count temporary registers.
    */

    /* Initialize number of temporary registers. */
    tempCount    = 0;
    texAttrCount = 0;

    /* Check symbol table to find the maximal index. */
    if (Shader->variableCount > 0)
    {
        gctUINT variableCount = Shader->variableCount;

        for (i = 0; i < variableCount; i++)
        {
            gcVARIABLE variable = Shader->variables[i];

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
            {
                gctUINT size = variable->arraySize * _getTypeSize(variable->u.type);
                gctUINT end = variable->tempIndex + size;

                if (end > tempCount) tempCount = end;
            }
        }
    }

    /* We need check output for VS since VS can declare output but without using */
    if ((Shader->type == gcSHADER_TYPE_VERTEX) && (Shader->outputCount > 0))
	{
        gctUINT outputCount = Shader->outputCount;

		for (i = 0; i < outputCount; i++)
		{
            gctUINT	size;
            gctUINT end;
            gcOUTPUT output = Shader->outputs[i];

            if (!output) continue;
			size = output->arraySize * _getTypeSize(output->type);
			end = output->tempIndex + size;

			if (end > tempCount) tempCount = end;
		}
	}

    /******************** Check the argument temp index Function **********************/
    for (i = 0; i < Shader->functionCount; ++i)
    {
        gcFUNCTION function = Shader->functions[i];
        gctSIZE_T j;

        for (j = 0; j < function->argumentCount; ++j)
        {
            gctINT argIndex = function->arguments[j].index;

            if  (argIndex >= (gctINT) tempCount)
            {
                tempCount = argIndex + 1;
            }
        }
    }

    /*************** Check the argument temp index for Kernel Function *****************/
    for (i = 0; i < Shader->kernelFunctionCount; ++i)
    {
        gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[i];
        gctSIZE_T j;

        for (j = 0; j < kernelFunction->argumentCount; ++j)
        {
            gctINT argIndex = kernelFunction->arguments[j].index;
            if  (argIndex >= (gctINT) tempCount)
            {
                tempCount = argIndex + 1;
            }
        }
    }

    /* Walk all instructions. */
    for (i = 0; i < Shader->codeCount; i++)
    {
        /* Get instruction. */
        code = &Shader->code[i];

        /* Determine temporary register usage. */
        switch (code->opcode)
        {
        case gcSL_NOP:
            /* fall through */
        case gcSL_KILL:
            /* fall through */
        case gcSL_JMP:
            /* fall through */
        case gcSL_CALL:
            /* fall through */
        case gcSL_RET:
            /* Skip control flow instructions. */
            break;

        case gcSL_TEXBIAS:
            /* fall through */
        case gcSL_TEXGRAD:
            /* fall through */
        case gcSL_TEXLOD:
            if ((gctUINT) code->source0Index >= texAttrCount)
            {
                /* Adjust texture attribute count. */
                texAttrCount = code->source0Index + 1;
            }
            break;

        default:
            if ((gctUINT) code->tempIndex >= tempCount)
            {
                /* Adjust temporary register count. */
                tempCount = code->tempIndex + 1;
            }

            if ((gcmSL_SOURCE_GET(code->source0, Type) == gcSL_TEMP)
            &&  (code->source0Index >= tempCount)
            )
            {
                /* Adjust temporary register count. */
                tempCount = code->source0Index + 1;
            }

            if ((gcmSL_SOURCE_GET(code->source1, Type) == gcSL_TEMP)
            &&  (code->source1Index >= tempCount)
            )
            {
                /* Adjust temporary register count. */
                tempCount = code->source1Index + 1;
            }
        }
    }

    /***************************************************************************
    ** II - Allocate register arrays.
    */

    do
    {
        if (Shader->attributeCount > 0)
        {
            /* Allocate attribute array. */
            gcmERR_BREAK(gcoOS_Allocate(os,
                                        gcmSIZEOF(struct _gcLINKTREE_ATTRIBUTE)
                                            * Shader->attributeCount,
                                        &pointer));

            Tree->attributeArray = pointer;

            /* Initialize all attributes as dead and having no users. */
            for (i = 0; i < Shader->attributeCount; i++)
            {
                Tree->attributeArray[i].inUse   = gcvFALSE;
                Tree->attributeArray[i].lastUse = -1;
                Tree->attributeArray[i].users   = gcvNULL;
            }

            /* Set attribute count. */
            Tree->attributeCount = Shader->attributeCount;
        }

        if (tempCount > 0)
        {
            gcSL_FORMAT format = gcSL_FLOAT;

            if (Shader->type == gcSHADER_TYPE_CL)
            {
                format = gcSL_UINT32;
            }

            /* Allocate temporary register array. */
            gcmERR_BREAK(gcoOS_Allocate(os,
                                        gcmSIZEOF(struct _gcLINKTREE_TEMP) *
                                            tempCount,
                                        &pointer));

            Tree->tempArray = pointer;

            /* Initialize all temporary registers as not defined. */
            for (i = 0; i < tempCount; i++)
            {
                Tree->tempArray[i].inUse         = gcvFALSE;
                Tree->tempArray[i].usage         = 0;
                Tree->tempArray[i].isIndex       = gcvFALSE;
                Tree->tempArray[i].isIndexing    = gcvFALSE;
                Tree->tempArray[i].defined       = gcvNULL;
                Tree->tempArray[i].constUsage[0] = 0;
                Tree->tempArray[i].constUsage[1] = 0;
                Tree->tempArray[i].constUsage[2] = 0;
                Tree->tempArray[i].constUsage[3] = 0;
                Tree->tempArray[i].lastUse       = -1;
                Tree->tempArray[i].dependencies  = gcvNULL;
                Tree->tempArray[i].users         = gcvNULL;
                Tree->tempArray[i].assigned      = -1;
                Tree->tempArray[i].owner         = gcvNULL;
                Tree->tempArray[i].isOwnerKernel = gcvFALSE;
                Tree->tempArray[i].variable      = gcvNULL;
                Tree->tempArray[i].format        = format;
            }

            /* Set temporary register count. */
            Tree->tempCount = tempCount;

            /* Initialize variable. */
            if (Shader->variableCount > 0)
            {
                gctUINT variableCount = Shader->variableCount;

                for (i = 0; i < variableCount; i++)
                {
                    gcVARIABLE variable = Shader->variables[i];

                    if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                    {
                        gctUINT startIndex, endIndex;
                        _GetVariableIndexingRange(Tree->shader, variable, gcvTRUE,
                                                  &startIndex, &endIndex);

                        if ((variable->arraySize > 1) || IS_MATRIX_TYPE(variable->u.type) ||
                            (endIndex - startIndex > 1))
                        {
                            gctUINT size = variable->arraySize * _getTypeSize(variable->u.type);
                            gctUINT j;

                            gcmASSERT(variable->tempIndex + size <= Tree->tempCount);
                            temp = Tree->tempArray + variable->tempIndex;
                            for (j = 0; j < size; j++, temp++)
                            {
                                temp->variable = variable;

#if !_USE_NEW_VARIABLE_HANDLER_
                                /* Mark entire register as non optimizable. */
                                temp->constUsage[0] =
                                temp->constUsage[1] =
                                temp->constUsage[2] =
                                temp->constUsage[3] = -1;
#endif
                            }
                        }
                    }
                }
            }
        }

        if (Shader->outputCount > 0)
        {
            /* Allocate output array. */
            gcmERR_BREAK(gcoOS_Allocate(os,
                                        gcmSIZEOF(struct _gcLINKTREE_OUTPUT) *
                                            Shader->outputCount,
                                        &pointer));

            Tree->outputArray = pointer;

            /* Initialize all outputs as not being defined. */
            for (i = 0; i < Shader->outputCount; i++)
            {
                Tree->outputArray[i].inUse             = gcvFALSE;
                Tree->outputArray[i].tempHolding       = -1;
                Tree->outputArray[i].fragmentAttribute = -1;
                Tree->outputArray[i].fragmentIndex     = 0;
                Tree->outputArray[i].fragmentIndexEnd  = 0;
            }

            /* Copy number of outputs. */
            Tree->outputCount = Shader->outputCount;
        }

        if (texAttrCount > 0)
        {
            /* Allocate texture attribute array. */
            gcmERR_BREAK(gcoOS_Allocate(os,
                                        gcmSIZEOF(gcsLINKTREE_LIST_PTR) *
                                            texAttrCount,
                                        &pointer));

            texAttr = pointer;

            for (i = 0; i < texAttrCount; ++i)
            {
                texAttr[i] = gcvNULL;
            }
        }

        if (Shader->codeCount > 0)
        {
            /* Allocate hints. */
            gcmERR_BREAK(gcoOS_Allocate(os,
                                        gcmSIZEOF(gcsCODE_HINT) *
                                            Shader->codeCount,
                                        &pointer));

            Tree->hints = pointer;

            /* Reset all hints. */
            for (i = 0; i < Tree->shader->codeCount; ++i)
            {
                Tree->hints[i].owner          = gcvNULL;
                Tree->hints[i].isOwnerKernel  = gcvFALSE;
                Tree->hints[i].callers        = gcvNULL;
                Tree->hints[i].callNest       = -1;
                Tree->hints[i].lastUseForTemp = i;
                Tree->hints[i].lastLoadUser   = -1;
                Tree->hints[i].loadDestIndex  = -1;
            }
        }
    }
    while (gcvFALSE);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    /***************************************************************************
    ** III - Determine dependency and users of all registers.
    */

    /* Walk all instructions. */
    for (i = 0; i < Shader->codeCount; i++)
    {
        gctBOOL isTexAttr = gcvFALSE;
        gcsCODE_CALLER_PTR caller;

        /* Determine ownership of the code for functions. */
        for (f = 0; f < Shader->functionCount; ++f)
        {
            gcFUNCTION function = Shader->functions[f];

            if ((i >= function->codeStart)
            &&  (i < function->codeStart + function->codeCount)
            )
            {
                Tree->hints[i].owner = function;
                Tree->hints[i].isOwnerKernel = gcvFALSE;
                break;
            }
        }

        /* Determine ownership of the code for kernel functions. */
        for (f = 0; f < Shader->kernelFunctionCount; ++f)
        {
            gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[f];

            if (kernelFunction->isMain)
            {
                continue;
            }

            if ((i >= kernelFunction->codeStart)
            &&  (i < kernelFunction->codeEnd)
            )
            {
                Tree->hints[i].owner = kernelFunction;
                Tree->hints[i].isOwnerKernel = gcvTRUE;
                break;
            }
        }

        if (Tree->hints[i].owner == gcvNULL)
        {
            Tree->hints[i].callNest = 0;
        }

        /* Set PC. */
        pc = (Flags & gcvSHADER_RESOURCE_USAGE) ? i : (gctSIZE_T) -1;

        /* Get instruction. */
        code = &Shader->code[i];

        /* Dispatch on opcode. */
        switch (code->opcode)
        {
        case gcSL_CALL:
            /* fall through */
        case gcSL_JMP:
            if (code->tempIndex < Shader->codeCount)
            {
                /* Allocate a gcsCODE_CALLER structure. */
                gcmERR_BREAK(gcoOS_Allocate(os,
                                            gcmSIZEOF(gcsCODE_CALLER),
                                            &pointer));

                caller = pointer;

                /* Add the current caller to the list of callees. */
                caller->caller = i;
                caller->next   = Tree->hints[code->tempIndex].callers;

                Tree->hints[code->tempIndex].callers = caller;

                /* Add last use for temp register used in code gen. */
                if (code->opcode == gcSL_JMP && code->tempIndex < i)
                {
                    gctSIZE_T inst;

                    for (inst = code->tempIndex; inst < i; inst++)
                    {
                        if (Tree->hints[inst].lastUseForTemp < (gctINT) i)
                        {
                            Tree->hints[inst].lastUseForTemp = (gctINT) i;
                        }
                        else
                        {
                            gcmASSERT(Tree->hints[inst].lastUseForTemp < (gctINT) i);
                        }
                    }
                }
            }
			/* fall through */

        case gcSL_RET:
            /* fall through */
        case gcSL_NOP:
            /* fall through */
        case gcSL_KILL:
            /* Skip control flow instructions. */
            temp = gcvNULL;
            break;

        case gcSL_TEXBIAS:
            /* fall through */
        case gcSL_TEXGRAD:
            /* fall through */
        case gcSL_TEXLOD:
            /* Skip texture state instructions. */
            temp      = gcvNULL;
            isTexAttr = gcvTRUE;
            break;

        default:
            /* Get gcSL_TARGET field. */
            target = code->temp;

            /* Get pointer to temporary register. */
            temp = &Tree->tempArray[code->tempIndex];

            if (code->opcode != gcSL_STORE)
            {
                gcSL_FORMAT format = gcmSL_TARGET_GET(target, Format);

                /* Add instruction to definitions. */
                gcLINKTREE_AddList(Tree, &temp->defined, gcSL_NONE, i);

                /* Set register format. */
                if (format == gcSL_FLOAT16)
                {
                    gcmASSERT(code->opcode == gcSL_LOAD ||
                              code->opcode == gcSL_STORE ||
                              code->opcode == gcSL_STORE1);
                    format = gcSL_FLOAT;
                }

                 /* Cannot enforce consistent format for each register
                  * due to math built-in functions and unions.
                  */
                temp->format = format;
            }
            else
            {
                /* Target is actually a source. */
                gcmERR_BREAK(_TempSource(Tree, code->tempIndex, -1, i));
            }

            /* Update register usage. */
            temp->usage |= gcmSL_TARGET_GET(target, Enable);

            /* If the target is indexed temp register, mark all array temp registers as target. */
            if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            {
                index = code->tempIndexed;

                /* Add index usage. */
                gcmERR_BREAK(_IndexedSource(Tree,
                                            gcmSL_INDEX_GET(index, Index),
                                            pc,
                                            (temp == gcvNULL)
                                            ? -1
                                            : code->tempIndex));

                Tree->tempArray[code->tempIndex].isIndexing = gcvTRUE;

                if (Tree->tempArray[code->tempIndex].variable)
                {
                    gcVARIABLE variable = Tree->tempArray[code->tempIndex].variable;

                    gctUINT startIndex, endIndex, j;
                    gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                    _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                              &startIndex, &endIndex);
                    gcmASSERT(startIndex == variable->tempIndex);

                    for (j = startIndex + 1; j < endIndex; j++)
                    {
                        /* Get pointer to temporary register. */
                        gcLINKTREE_TEMP tempA = &Tree->tempArray[j];

                        tempA->isIndexing = gcvTRUE;

                        /* Add instruction to definitions. */
                        gcLINKTREE_AddList(Tree, &tempA->defined, gcSL_NONE, i);

                        /* Update register usage. */
                        tempA->usage |= gcmSL_TARGET_GET(target, Enable);

                        gcmERR_BREAK(_IndexedSource(Tree,
                                                    gcmSL_INDEX_GET(index, Index),
                                                    pc,
                                                    (tempA == gcvNULL)
                                                    ? -1
                                                    : (gctINT)j));
                    }
                }
                else
                {
                    gcmASSERT(Tree->tempArray[code->tempIndex].variable);
                }
            }
            break;
        }

        /* Determine usage of source0. */
        source = code->source0;

        switch (gcmSL_SOURCE_GET(source, Type))
        {
        case gcSL_ATTRIBUTE:
            /* Source is an attribute. */
            gcmERR_BREAK(_AttributeSource(Tree,
                                          code->source0Index,
                                          (temp == gcvNULL)
                                              ? -1
                                              : code->tempIndex,
                                          pc));
            break;

        case gcSL_TEMP:
            /* Source is a temporary register. */
            gcmERR_BREAK(_TempSource(Tree,
                                     code->source0Index,
                                     (temp == gcvNULL) ? -1 : code->tempIndex,
                                     pc));

            /* If the target is indexed temp register, mark all array temp registers as target. */
            if ((temp != gcvNULL)
            &&  (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
            &&  (Tree->tempArray[code->tempIndex].variable)
            )
            {
                gcVARIABLE variable = Tree->tempArray[code->tempIndex].variable;

                gctUINT startIndex, endIndex, j;
                gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                          &startIndex, &endIndex);
                gcmASSERT(startIndex == variable->tempIndex);

                Tree->tempArray[code->tempIndex].isIndexing = gcvTRUE;

                /* Source is a temporary register array. */
                for (j = startIndex + 1; j < endIndex; j++)
                {
                    gcLINKTREE_TEMP tempA = &Tree->tempArray[j];
                    tempA->isIndexing = gcvTRUE;

                    gcmERR_BREAK(_TempSource(Tree,
                                             code->source0Index,
                                             j,
                                             pc));
                }
            }
            break;
        }

        /* Break on error. */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        switch (gcmSL_SOURCE_GET(source, Indexed))
        {
        case gcSL_INDEXED_X:
            /* fall through */
        case gcSL_INDEXED_Y:
            /* fall through */
        case gcSL_INDEXED_Z:
            /* fall through */
        case gcSL_INDEXED_W:
            index = code->source0Indexed;

            /* Add index usage for source 0. */
            gcmERR_BREAK(_IndexedSource(Tree,
                                        gcmSL_INDEX_GET(index, Index),
                                        pc,
                                        (temp == gcvNULL)
                                            ? -1
                                            : code->tempIndex));

            /* If the source is temp register, mark all array temp registers as inputs. */
            if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
            {
                if (Tree->tempArray[code->source0Index].variable)
                {
                    gcVARIABLE variable = Tree->tempArray[code->source0Index].variable;

                    gctUINT startIndex, endIndex, j;
                    gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                    _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                              &startIndex, &endIndex);
                    gcmASSERT(startIndex == variable->tempIndex);

                    /* Source is a temporary register array. */
                    for (j = startIndex; j < endIndex; j++)
                    {
                        gcLINKTREE_TEMP tempA = &Tree->tempArray[j];
                        tempA->isIndexing = gcvTRUE;

                        if (temp == gcvNULL)
                        {
                            gcmERR_BREAK(_TempSource(Tree,
                                                     j,
                                                     -1,
                                                     pc));
                        }
                        else if (gcmSL_TARGET_GET(target, Indexed) == gcSL_NOT_INDEXED ||
                                 Tree->tempArray[code->tempIndex].variable == gcvNULL)
                        {
                            gcmERR_BREAK(_TempSource(Tree,
                                                     j,
                                                     code->tempIndex,
                                                     pc));
                        }
                        else
                        {
                            gcVARIABLE variableT = Tree->tempArray[code->tempIndex].variable;

                            gctUINT startIndexT, endIndexT, k;
                            gcmASSERT(variableT->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                            _GetVariableIndexingRange(Tree->shader, variableT, gcvFALSE,
                                                      &startIndexT, &endIndexT);
                            gcmASSERT(startIndexT == variableT->tempIndex);

                            /* Source is a temporary register array. */
                            for (k = startIndexT; k < endIndexT; k++)
                            {
                                gcLINKTREE_TEMP tempA = &Tree->tempArray[k];
                                tempA->isIndexing = gcvTRUE;

                                gcmERR_BREAK(_TempSource(Tree,
                                                         j,
                                                         k,
                                                         pc));
                            }
                        }
                    }
                }
                else
                {
                    gcmASSERT(Tree->tempArray[code->source0Index].variable);
                }
            }
        }

        /* Break on error. */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        /* Special case for TEXLD. */
        if ((code->opcode == gcSL_TEXLD || code->opcode == gcSL_TEXLDP)
        &&  (texAttrCount > 0)
        &&  (code->source0Index < texAttrCount)
        )
        {
            gcsLINKTREE_LIST_PTR list;

            /* Add any texture attribute dependencies. */
            gcmASSERT(temp != gcvNULL);
            gcmASSERT(gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM);

            for (list = texAttr[code->source0Index]; list != gcvNULL; list = list->next)
            {
                if (list->type == gcSL_ATTRIBUTE)
                {
                    gcmERR_BREAK(_AttributeSource(Tree,
                                                  (gctUINT16)list->index,
                                                  code->tempIndex,
                                                  pc));
                }
                else if (list->type == gcSL_TEMP)
                {
                    gcmERR_BREAK(_TempSource(Tree,
                                             list->index,
                                             code->tempIndex,
                                             pc));
                }
                else
                {
                    gcmASSERT(gcvFALSE);
                }
            }

            /* Clean up texAttr at current sampler# */
            _Delete(Tree, &texAttr[code->source0Index]);
            texAttr[code->source0Index] = gcvNULL;
        }

        /* Determine usage of source1. */
        source = code->source1;

        switch (gcmSL_SOURCE_GET(source, Type))
        {
        case gcSL_ATTRIBUTE:
            /* Source is an attribute. */

            if (!isTexAttr)
            {
                gcmERR_BREAK(_AttributeSource(Tree,
                                              code->source1Index,
                                              (temp == gcvNULL)
                                                  ? -1
                                                  : code->tempIndex,
                                              pc));
            }
            else
            {
                gcmERR_BREAK(gcLINKTREE_AddList(Tree,
                                                &texAttr[code->source0Index],
                                                gcSL_ATTRIBUTE,
                                                code->source1Index));
            }

            break;

        case gcSL_TEMP:
            /* Source is a temporary register. */
            if (!isTexAttr)
            {
                gcmERR_BREAK(_TempSource(Tree,
                                         code->source1Index,
                                         (temp == gcvNULL) ? -1 : code->tempIndex,
                                         pc));

                /* If the target is indexed temp register, mark all array temp registers as target. */
                if (temp != gcvNULL && gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED &&
                    Tree->tempArray[code->tempIndex].variable)
                {
                    gcVARIABLE variable = Tree->tempArray[code->tempIndex].variable;

                    gctUINT startIndex, endIndex, j;
                    gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                    _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                              &startIndex, &endIndex);
                    gcmASSERT(startIndex == variable->tempIndex);

                    Tree->tempArray[code->tempIndex].isIndexing = gcvTRUE;

                    /* Source is a temporary register array. */
                    for (j = startIndex + 1; j < endIndex; j++)
                    {
                        gcLINKTREE_TEMP tempA = &Tree->tempArray[j];
                        tempA->isIndexing = gcvTRUE;

                        gcmERR_BREAK(_TempSource(Tree,
                                                 code->source1Index,
                                                 j,
                                                 pc));
                    }
                }
            }
            else
            {
                gcmERR_BREAK(gcLINKTREE_AddList(Tree,
                                                &texAttr[code->source0Index],
                                                gcSL_TEMP,
                                                code->source1Index));
            }
            break;
        default:
            break;
        }

        /* Break on error. */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        switch (gcmSL_SOURCE_GET(source, Indexed))
        {
        case gcSL_INDEXED_X:
            /* fall through */
        case gcSL_INDEXED_Y:
            /* fall through */
        case gcSL_INDEXED_Z:
            /* fall through */
        case gcSL_INDEXED_W:
            index = code->source1Indexed;

            /* Add index usage for source 1. */
            gcmERR_BREAK(_IndexedSource(Tree,
                                        gcmSL_INDEX_GET(index, Index),
                                        pc,
                                        (temp == gcvNULL)
                                            ? -1
                                            : code->tempIndex));

            /* If the source is temp register, mark all array temp registers as inputs. */
            if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
            {
                if (Tree->tempArray[code->source1Index].variable)
                {
                    gcVARIABLE variable = Tree->tempArray[code->source1Index].variable;

                    gctUINT startIndex, endIndex, j;
                    gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                    _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                              &startIndex, &endIndex);
                    gcmASSERT(startIndex == variable->tempIndex);

                    /* Source is a temporary register array. */
                    for (j = startIndex; j < endIndex; j++)
                    {
                        gcLINKTREE_TEMP tempA = &Tree->tempArray[j];
                        tempA->isIndexing = gcvTRUE;

                        if (temp == gcvNULL)
                        {
                            gcmERR_BREAK(_TempSource(Tree,
                                                     j,
                                                     -1,
                                                     pc));
                        }
                        else if (gcmSL_TARGET_GET(target, Indexed) == gcSL_NOT_INDEXED ||
                                 Tree->tempArray[code->tempIndex].variable == gcvNULL)
                        {
                            gcmERR_BREAK(_TempSource(Tree,
                                                     j,
                                                     code->tempIndex,
                                                     pc));
                        }
                        else
                        {
                            gcVARIABLE variableT = Tree->tempArray[code->tempIndex].variable;

                            gctUINT startIndexT, endIndexT, k;
                            gcmASSERT(variableT->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                            _GetVariableIndexingRange(Tree->shader, variableT, gcvFALSE,
                                                      &startIndexT, &endIndexT);
                            gcmASSERT(startIndexT == variableT->tempIndex);

                            /* Source is a temporary register array. */
                            for (k = startIndexT; k < endIndexT; k++)
                            {
                                gcLINKTREE_TEMP tempA = &Tree->tempArray[k];
                                tempA->isIndexing = gcvTRUE;

                                gcmERR_BREAK(_TempSource(Tree,
                                                         j,
                                                         k,
                                                         pc));
                            }
                        }
                    }
                }
                else
                {
                    gcmASSERT(Tree->tempArray[code->source1Index].variable);
                }
            }
            break;

        default:
            break;
        }

        /* Break on error. */
        if (gcmIS_ERROR(status))
        {
            break;
        }
    }

    /* Update last use for temp register used in code gen. */
    /* For each function, all its statements are set to the last call. */
    for (i = 0; i < Shader->functionCount + Shader->kernelFunctionCount; ++i)
    {
		gctUINT codeStart, codeEnd, ic;
        gctINT lastUse = -1;
		gcsCODE_CALLER_PTR caller;

		if (i < Shader->functionCount) {
			gcFUNCTION function = Shader->functions[i];
			codeStart = function->codeStart;
			codeEnd   = codeStart + function->codeCount;
		} else {
            gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[ i - Shader->functionCount ];
			codeStart = kernelFunction->codeStart;
			codeEnd   = kernelFunction->codeEnd;
		}

        for (caller = Tree->hints[codeStart].callers;
             caller != gcvNULL;
             caller = caller->next)
        {
            if (Tree->hints[caller->caller].lastUseForTemp > lastUse)
            {
                lastUse = Tree->hints[caller->caller].lastUseForTemp;
            }
        }

        if (lastUse >= 0)
        {
            for (ic = codeStart; ic < codeEnd; ic++)
            {
                Tree->hints[ic].lastUseForTemp = lastUse;
            }
        }
    }

    /* Determine call nesting level for functions and kernel functions. */
    for (nest = 1, modified = gcvTRUE; modified; ++nest)
    {
        modified = gcvFALSE;

        for (i = 0; i < Shader->functionCount + Shader->kernelFunctionCount; ++i)
        {
			gctUINT codeStart, codeCount;
			gcsCODE_CALLER_PTR caller;

			if (i < Shader->functionCount) {
				gcFUNCTION function = Shader->functions[i];
				codeStart = function->codeStart;
				codeCount = function->codeCount;
			} else {
	            gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[ i - Shader->functionCount ];
				codeStart = kernelFunction->codeStart;
				codeCount = kernelFunction->codeEnd - kernelFunction->codeStart;
			}
            if (Tree->hints != gcvNULL){
                if (Tree->hints[codeStart].callNest > 0)
                {
                    continue;
                }

                for (caller = Tree->hints[codeStart].callers;
                     (caller != gcvNULL);
                     caller = caller->next)
                {
                    if (Tree->hints[caller->caller].callNest == nest - 1)
                    {
                        gctSIZE_T j;

                        for (j = 0; j < codeCount; ++j)
                        {
                            Tree->hints[codeStart + j].callNest = nest;
                        }

                        modified = gcvTRUE;
                        break;
                    }
                }
            }
        }

        if (!modified)
        {
            break;
        }
    }

    /***************************************************************************
    ** IV - Add output dependencies.
    */

    if (gcmNO_ERROR(status))
    {
        /* Loop through all outputs. */
        for (i = 0; i < Shader->outputCount; i++)
        {
            gcOUTPUT output;
            gcLINKTREE_TEMP tempOutput;
            gctUINT size, regIdx, endRegIdx;

            /* Get gcOUTPUT pointer. */
            output = Shader->outputs[i];

            if (output != gcvNULL)
            {
                gctUINT16 tempIndex = output->tempIndex;

                if (output->nameLength == gcSL_POINT_COORD)
                {
                    /* The output is just a placeholder.  Use gl_Position's tempIndex. */
                    gctSIZE_T j;

                    for (j = 0; j < Shader->outputCount; j++)
                    {
                        if (Shader->outputs[j]->nameLength == gcSL_POSITION)
                        {
                            tempIndex = Shader->outputs[j]->tempIndex;
                            break;
                        }
                    }

                    gcmASSERT(j < Shader->outputCount);
                }

                /* Copy temporary register to output. */
                Tree->outputArray[i].tempHolding = tempIndex;

                /* we expand output array to elements of VS, but for fragment shader, */
                /* we consider it as entity. We should refine it to match at two sides later! */
                size = 1; /*output->arraySize;*/

                if (IS_MATRIX_TYPE(output->type)) size *= _getTypeSize(output->type);
                endRegIdx = tempIndex + size;

                for (regIdx = tempIndex; regIdx < endRegIdx; regIdx ++)
                {
                    /* Get temporary register defining this output. */
                    tempOutput = &Tree->tempArray[regIdx];

                    /* Make sure the temp is marked as an output. */
                    tempOutput->lastUse = -1;

                    /* Add output to users of temporary register. */
                    gcmERR_BREAK(gcLINKTREE_AddList(Tree,
                                                    &tempOutput->users,
                                                    gcSL_OUTPUT,
                                                    i));
                }
            }
        }
    }

    if (texAttr != gcvNULL)
    {
        /* Free each dependency list in the texture attribute array. */
        for (i = 0; i < texAttrCount; ++i)
        {
            if (texAttr[i] != gcvNULL)
            {
                _Delete(Tree, &texAttr[i]);
            }
        }

        /* Free the texture attribute array. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(os, texAttr));
    }

    /***************************************************************************
    *********************************** Temporaries that are used beyond CALL **
    ***************************************************************************/

    if (Flags & gcvSHADER_RESOURCE_USAGE)
    {
        /* For each attribute, if it is used inside a function, update the
           last usage to the last callee of that function.  Or, if the
           attribute is used inside a loop, update the last usage to the end
           of the loop. */
        for (i = 0; i < Tree->attributeCount; ++i)
        {
            gcsLINKTREE_LIST_PTR user;
            gctINT lastUse = -1;

            for (user = Tree->attributeArray[i].users;
                 user != gcvNULL;
                 user = user->next)
            {
                /* Does the user belongs to a function? */
                if(Tree->hints != gcvNULL){
                    if (Tree->hints[user->index].owner != gcvNULL)
                    {
                        /* Find the last callee of the function inside main(). */
                        _FindCallers(Tree,
                                     Tree->hints[user->index].owner,
                                     Tree->hints[user->index].isOwnerKernel,
                                     0,
                                     &lastUse);
                    }
                    else
                    {
                        gctINT userIndex;
                        gcsCODE_CALLER_PTR caller;

                        if (user->index > lastUse)
                        {
                            lastUse = user->index;
                        }

                        /* Walk the code backwards to find a JMP from beyond this
                        user. */
                        for (userIndex = user->index; userIndex >= 0; --userIndex)
                        {
                            if (Tree->hints[userIndex].owner != gcvNULL)
                            {
						    	if (Tree->hints[userIndex].isOwnerKernel) {
							    	index = (gctUINT16) ((gcKERNEL_FUNCTION)Tree->hints[userIndex].owner)->codeStart;
    							} else {
	    							index = (gctUINT16) ((gcFUNCTION)Tree->hints[userIndex].owner)->codeStart;
		    					}
                                continue;
                            }

                            for (caller = Tree->hints[userIndex].callers;
                                 caller != gcvNULL;
                                 caller = caller->next)
                            {
                                if ((Tree->hints[caller->caller].owner == gcvNULL)
                                &&  (caller->caller > user->index)
                                &&  (caller->caller > lastUse)
                                )
                                {
                                    lastUse = caller->caller;
                                }
                            }
                        }
                    }
                }
            }

            Tree->attributeArray[i].lastUse = lastUse;
        }

        /* For each temp, if it is used both inside and outside a function, update
           the last usage to the last callee of that function in the outermost
           usage.  Or, if the temp is used inside a loop, update the last usage to
           the end of the loop. */
        for (i = 0; i < Tree->tempCount; ++i)
        {
            gcLINKTREE_TEMP tempReg = &Tree->tempArray[i];
            gcsLINKTREE_LIST_PTR define, user;
            gctINT lastUse = -1, lastNest = 0;
            gctPOINTER lastOwner = gcvNULL;
			gctBOOL isLastOwnerKernel = gcvFALSE;

            if (tempReg->defined == gcvNULL)
            {
                continue;
            }

            for (define = tempReg->defined;
                 define != gcvNULL;
                 define = define->next)
            {
            if (Tree->hints != gcvNULL){
                    if (((lastUse == -1)
                    ||  (  (Tree->hints[define->index].owner == lastOwner)
                        && (define->index > lastUse)
                        )
                    ||  (Tree->hints[define->index].callNest < lastNest)
                    )
					&& (Tree->hints[define->index].callNest != -1)
                    )
                    {
                        lastUse   = define->index;
                        lastNest  = Tree->hints[lastUse].callNest;
                        lastOwner = Tree->hints[lastUse].owner;
					    isLastOwnerKernel = Tree->hints[lastUse].isOwnerKernel;
                    }
                }
            }

            if (lastNest > 0)
            {
                /* Check if lastOwner has multiple callers directly or indirectly. */
                gctINT upperUse       = lastUse;
                gctINT upperNest      = lastNest;
                gctPOINTER upperOwner = lastOwner;
				gctBOOL isUpperOwnerKernel = isLastOwnerKernel;
				gcsCODE_CALLER_PTR nextCaller;

                while (upperNest > 0)
                {
                    _FindCallers(Tree,
                                 upperOwner,
								 isUpperOwnerKernel,
                                 upperNest - 1,
                                 &upperUse);

                    upperNest  = Tree->hints[upperUse].callNest;
                    if(Tree->hints != gcvNULL){
    					if (isUpperOwnerKernel) {
	    					nextCaller = Tree->hints[((gcKERNEL_FUNCTION)upperOwner)->codeStart].callers->next;
		    			} else {
			    			nextCaller = Tree->hints[((gcFUNCTION)upperOwner)->codeStart].callers->next;
				    	}
                        if (nextCaller)
                        {
                            /* Multiple callers. */
                            lastUse   = upperUse;
                            lastNest  = upperNest;
                            lastOwner = upperOwner = Tree->hints[upperUse].owner;
						    isLastOwnerKernel = isUpperOwnerKernel = Tree->hints[upperUse].isOwnerKernel;
                        }
                        else
                        {
                            upperOwner = Tree->hints[upperUse].owner;
		    				isUpperOwnerKernel = Tree->hints[upperUse].isOwnerKernel;
                        }
                    }
                }
            }

            for (user = tempReg->users; user != gcvNULL; user = user->next)
            {
                if (user->type == gcSL_OUTPUT)
                {
                    lastUse = -1;
                    break;
                }

                if (user->type != gcSL_NONE)
                {
                    lastUse = -1;
                    break;
                }
                if(Tree->hints != gcvNULL){
                    if (Tree->hints[user->index].callNest > lastNest)
                    {
                        _FindCallers(Tree,
                                     Tree->hints[user->index].owner,
                                     Tree->hints[user->index].isOwnerKernel,
                                     lastNest,
                                     &lastUse);
                    }
                    else if (Tree->hints[user->index].callNest < lastNest)
                    {
                        /* Caller of lastOwner or temp is global variable. */
                        lastUse = user->index;
                    }
                    else if (Tree->hints[user->index].owner == lastOwner)
                    {
                        gctINT userIndex;

                        if (user->index > lastUse)
                        {
                            lastUse = user->index;
                        }

                        for (userIndex = user->index; userIndex >= 0; --userIndex)
                        {
                            gcsCODE_CALLER_PTR caller;

                            if (Tree->hints[userIndex].owner != lastOwner)
                            {
                                if (lastOwner != gcvNULL)
                                {
                                    break;
                                }
					    		if (Tree->hints[userIndex].isOwnerKernel) {
						    		userIndex = (gctUINT16) ((gcKERNEL_FUNCTION)Tree->hints[userIndex].owner)->codeStart;
							    } else {
								    userIndex = (gctUINT16) ((gcFUNCTION)Tree->hints[userIndex].owner)->codeStart;
    							}
                                continue;
                            }

                            for (caller = Tree->hints[userIndex].callers;
                                 caller != gcvNULL;
                                 caller = caller->next)
                            {
                                if ((Tree->hints[caller->caller].owner == lastOwner)
                                &&  (caller->caller > user->index)
                                &&  (caller->caller > lastUse)
                                )
                                {
                                    lastUse = caller->caller;
                                }
                            }
                        }
                    }
                }
            }

            tempReg->lastUse = lastUse;
        }
    }

#if _USE_NEW_VARIABLE_HANDLER_
    /* Update lastUse for variables. */
    if (Shader->variableCount > 0)
    {
        gctUINT variableCount = Shader->variableCount;

        for (i = 0; i < variableCount; i++)
        {
            gcVARIABLE variable = Shader->variables[i];

            if ((variable->arraySize > 1) || IS_MATRIX_TYPE(variable->type))
            {
                gctUINT size = variable->arraySize * _getTypeSize(variable->type);
                gctINT lastUse = -1;
                gctUINT j;

                gcmASSERT(variable->tempIndex + size <= Tree->tempCount);
                temp = Tree->tempArray + variable->tempIndex;
                for (j = 0; j < size; j++, temp++)
                {
                    if (temp->lastUse > lastUse)
                    {
                        lastUse = temp->lastUse;
                    }
                }
                temp = Tree->tempArray + variable->tempIndex;
                for (j = 0; j < size; j++, temp++)
                {
                    temp->lastUse = lastUse;
                }
            }
        }
    }
#endif

    /***************************************************************************
    ****************************************************** Function Arguments **
    ***************************************************************************/

    for (i = 0; i < Shader->functionCount; ++i)
    {
        gcFUNCTION function = Shader->functions[i];
        gctSIZE_T j;

        for (j = 0; j < function->argumentCount; ++j)
        {
            gctINT argIndex = function->arguments[j].index;
            gcsLINKTREE_LIST_PTR user;

            gcmASSERT(Tree->tempArray[argIndex].owner == gcvNULL);
            Tree->tempArray[argIndex].owner = function;
            Tree->tempArray[argIndex].isOwnerKernel = gcvFALSE;

            Tree->tempArray[argIndex].inputOrOutput = gcvFUNCTION_INPUT;

            for (user = Tree->tempArray[argIndex].users;
                 user != gcvNULL;
                 user = user->next)
            {
                if (Tree->hints[user->index].owner != function)
                {
                    Tree->tempArray[argIndex].inputOrOutput =
                        gcvFUNCTION_OUTPUT;
                    break;
                }
            }
        }
    }

    /***************************************************************************
    *********************************************** Kernel Function Arguments **
    ***************************************************************************/

    for (i = 0; i < Shader->kernelFunctionCount; ++i)
    {
        gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[i];
        gctSIZE_T j;

        for (j = 0; j < kernelFunction->argumentCount; ++j)
        {
            gctINT argIndex = kernelFunction->arguments[j].index;
            gcsLINKTREE_LIST_PTR user;

            gcmASSERT(Tree->tempArray[argIndex].owner == gcvNULL);
            Tree->tempArray[argIndex].owner = kernelFunction;
            Tree->tempArray[argIndex].isOwnerKernel = gcvTRUE;

            Tree->tempArray[argIndex].inputOrOutput = gcvFUNCTION_INPUT;

            for (user = Tree->tempArray[argIndex].users;
                 user != gcvNULL;
                 user = user->next)
            {
                if (Tree->hints[user->index].owner != kernelFunction)
                {
                    Tree->tempArray[argIndex].inputOrOutput =
                        gcvFUNCTION_OUTPUT;
                    break;
                }
            }
        }
    }

    if (Tree->shader->type == gcSHADER_TYPE_FRAGMENT)
    {
        for (i = 0; i < Tree->attributeCount; ++i)
        {
            if ((Tree->shader->attributes[i] != gcvNULL)
            &&  (Tree->shader->attributes[i]->nameLength == gcSL_POSITION_W)
            )
            {
                Tree->attributeArray[i].inUse   = gcvTRUE;
                Tree->attributeArray[i].lastUse = 0;
            }
        }
    }

    /* Set up hints for special optimization for LOAD SW workaround. */
    if (Tree->shader->loadUsers)
    {
        gctINT * loadUsers = Tree->shader->loadUsers;
        for (i = 0; i < Tree->shader->codeCount; i++)
        {
            if (loadUsers[i] >= 0)
            {
                /* Get instruction. */
                code = &Shader->code[i];
                gcmASSERT(code->opcode == gcSL_LOAD);
                Tree->hints[i].lastLoadUser = loadUsers[i];
                Tree->hints[i].loadDestIndex = code->tempIndex;
            }
        }
    }

    /* Return the status. */
    return status;
}

/*******************************************************************************
**                          _AddDependencies
********************************************************************************
**
**  Mark the dependedncies of a register as used.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gcsLINKTREE_LIST_PTR List
**          Pointer to the linked list of dependencies.
**
**  OUTPUT:
**
**      Nothing.
*/
static void
_AddDependencies(
    IN gcLINKTREE Tree,
    IN gcsLINKTREE_LIST_PTR List
    )
{
    /* Loop while we have nodes. */
    while (List != gcvNULL)
    {
        /* Dispatch on the type of register. */
        switch (List->type)
        {
        case gcSL_TEMP:
            /* Only process temporary register if not yet used. */
            if (!Tree->tempArray[List->index].inUse)
            {
                /* Mark temporary register as used. */
                Tree->tempArray[List->index].inUse = gcvTRUE;

                /* Process dependencies of temporary register. */
                _AddDependencies(Tree,
                                 Tree->tempArray[List->index].dependencies);
            }
            break;

        case gcSL_ATTRIBUTE:
            /* Mark attribute as used. */
            Tree->attributeArray[List->index].inUse = gcvTRUE;
            break;

        default:
            return;
        }

        /* Move to next entry in the list. */
        List = List->next;
    }
}

/*******************************************************************************
**                                   _RemoveItem
********************************************************************************
**
**  Remove an item from a list.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**      gcsLINKTREE_LIST_PTR * Root
**          Pointer to the variable holding the linked list pointer.
**
**      gcSL_TYPE Type
**          Type of the item to remove.
**
**      gctINT Index
**          Index of the item to remove.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_RemoveItem(
    IN gcLINKTREE Tree,
    IN gcsLINKTREE_LIST_PTR * Root,
    IN gcSL_TYPE Type,
    IN gctINT Index
    )
{
    gcsLINKTREE_LIST_PTR list, last = gcvNULL;
    gceSTATUS status;

    for (list = *Root; list != gcvNULL; list = list->next)
    {
        if ((list->type == Type) && (list->index == Index))
        {
            if (list->counter > 1)
            {
                list->counter -= 1;
                break;
            }

            if (last == gcvNULL)
            {
                *Root = list->next;
            }
            else
            {
                last->next = list->next;
            }

            status = gcmOS_SAFE_FREE(gcvNULL, list);
            return status;
        }

        last = list;
    }

    return gcvSTATUS_OK;
}

/*******************************************************************************
**                          gcLINKTREE_RemoveDeadCode
********************************************************************************
**
**  Remove dead code from a shader.
**
**  INPUT:
**
**      gcLINKTREE Tree
**          Pointer to a gcLINKTREE structure.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcLINKTREE_RemoveDeadCode(
    IN gcLINKTREE Tree
    )
{
    gctSIZE_T i;

    /***************************************************************************
    ** I: Mark all defined outputs and their dependency tree as used.
    */

    /* Walk all outputs. */
    for (i = 0; i < Tree->outputCount; i++)
    {
        /* Get temporary register holding this output. */
        gctINT temp = Tree->outputArray[i].tempHolding;

        if (temp < 0)
        {
            /* Remove output from shader. */
            if (Tree->shader->outputs[i] != gcvNULL)
            {
                /* Delete the output. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->shader->outputs[i]));
            }
        }

        else
        {
            gctUINT size, regIdx, endRegIdx;
            gcOUTPUT output = Tree->shader->outputs[i];

            size = output->arraySize;
            if (IS_MATRIX_TYPE(output->type)) size *= _getTypeSize(output->type);
            endRegIdx = temp + size;

            for (regIdx = temp; regIdx < endRegIdx; regIdx ++)
            {
                /* Only process if temporary register is not yet in use. */
                if (!Tree->tempArray[regIdx].inUse)
                {
                    /* Mark the temporary register as used. */
                    Tree->tempArray[regIdx].inUse = gcvTRUE;

                    /* Process all dependencies. */
                    _AddDependencies(Tree, Tree->tempArray[regIdx].dependencies);
                }
            }
        }
    }

    /***************************************************************************
    ** II: Mark all dependencies of JMP instructions as used.
    */

    for (i = 0; i < Tree->shader->codeCount; ++i)
    {
        gcSL_INSTRUCTION code = &Tree->shader->code[i];
        gctINT index;

        switch (code->opcode)
        {
        case gcSL_JMP:
            /* fall through */
        case gcSL_CALL:
            /* fall through */
        case gcSL_RET:
            /* fall through */
        case gcSL_KILL:
            if (gcmSL_TARGET_GET(code->temp, Condition) != gcSL_ALWAYS)
            {
                switch (gcmSL_SOURCE_GET(code->source0, Type))
                {
                case gcSL_TEMP:
                    index = gcmSL_INDEX_GET(code->source0Index, Index);

                    /* Only process if temporary register is not yet in use. */
                    if (!Tree->tempArray[index].inUse)
                    {
                        /* Mark the temporary register as used. */
                        Tree->tempArray[index].inUse = gcvTRUE;

                        /* Process all dependencies. */
                        _AddDependencies(Tree,
                                         Tree->tempArray[index].dependencies);
                    }
                    break;

                case gcSL_ATTRIBUTE:
                    index = gcmSL_INDEX_GET(code->source0Index, Index);

                    /* Mark the attribute as used. */
                    Tree->attributeArray[index].inUse = gcvTRUE;
                    break;

                default:
                    break;
                }

                switch (gcmSL_SOURCE_GET(code->source1, Type))
                {
                case gcSL_TEMP:
                    index = gcmSL_INDEX_GET(code->source1Index, Index);

                    /* Only process if temporary register is not yet in use. */
                    if (!Tree->tempArray[index].inUse)
                    {
                        /* Mark the temporary register as used. */
                        Tree->tempArray[index].inUse = gcvTRUE;

                        /* Process all dependencies. */
                        _AddDependencies(Tree,
                                         Tree->tempArray[index].dependencies);
                    }
                    break;

                case gcSL_ATTRIBUTE:
                    index = gcmSL_INDEX_GET(code->source1Index, Index);

                    /* Mark the attribute as used. */
                    Tree->attributeArray[index].inUse = gcvTRUE;
                    break;

                default:
                    break;
                }
            }
            break;

        default:
            break;
        }
    }

    /* Walk all attributes. */
    for (i = 0; i < Tree->attributeCount; i++)
    {
        /* Only process if attribute is dead. */
        if (!Tree->attributeArray[i].inUse)
        {
            /* Mark attribute as dead. */
            Tree->attributeArray[i].lastUse = -1;

            /* Loop while we have users. */
            _Delete(Tree, &Tree->attributeArray[i].users);

            /* Only process if the shader defines this attribute. */
            if ( (Tree->shader->type == gcSHADER_TYPE_FRAGMENT) &&
                 (Tree->shader->attributes[i] != gcvNULL) )
            {
                /* Free the gcATTRIBUTE structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Tree->shader->attributes[i]));

                /* Mark the attribute as invalid. */
                Tree->shader->attributes[i] = gcvNULL;
            }

            else if (Tree->shader->attributes[i] != gcvNULL)
            {
                /* Mark the attribute as disabled. */
                Tree->shader->attributes[i]->enabled = gcvFALSE;
            }
        }
    }

    /* Walk all temporary registers. */
    for (i = 0; i < Tree->tempCount; i++)
    {
        /* Only process if temporary register is dead. */
        if (!Tree->tempArray[i].inUse
        &&  (Tree->tempArray[i].defined != gcvNULL)
        )
        {
            gctINT minPC, maxPC;
            gcsLINKTREE_LIST_PTR node;

            /* Find the minimum instruction index. */
            minPC = Tree->tempArray[i].defined->index;

            for (node = Tree->tempArray[i].defined->next;
                 node != gcvNULL;
                 node = node->next)
            {
                if (node->index < minPC)
                {
                    minPC = node->index;
                }
            }

            /* Get the maximum instruction index. */
            maxPC = gcmMAX(Tree->tempArray[i].lastUse,
                          (gctINT) Tree->shader->codeCount - 1);

            /* Loop through the instructions where the temporary register is
               alive. */
            while (minPC <= maxPC)
            {
                /* Get the instruction pointer. */
                gcSL_INSTRUCTION code = Tree->shader->code + minPC;

                if (code->tempIndex == i)
                {
                    switch (code->opcode)
                    {
                    case gcSL_JMP:
                        /* fall through */
                    case gcSL_CALL:
                        /* fall through */
                    case gcSL_RET:
                        /* fall through */
                    case gcSL_KILL:
                        break;

                    default:
                        /* Replace instruction with a NOP. */
                        gcmVERIFY_OK(gcoOS_ZeroMemory(
                            code,
                            gcmSIZEOF(struct _gcSL_INSTRUCTION)));
                    }
                }

                /* Next instruction. */
                minPC++;
            }

            /* Remove the temporary register usage. */
            Tree->tempArray[i].lastUse = -1;

            /* Remove all definitions. */
            _Delete(Tree, &Tree->tempArray[i].defined);

            /* Remove all dependencies. */
            _Delete(Tree, &Tree->tempArray[i].dependencies);

            /* Remove all users. */
            _Delete(Tree, &Tree->tempArray[i].users);
        }
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**                               gcLINKTREE_Link
********************************************************************************
**
**  Link the output of the vertex tree to the attributes of the fragment tree.
**
**  INPUT:
**
**      gcLINKTREE VertexTree
**          Pointer to a gcLINKTREE structure representing the vertex shader.
**
**      gcLINKTREE FragmentTree
**          Pointer to a gcLINKTREE structure representing the fragment shader.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcLINKTREE_Link(
    IN gcLINKTREE VertexTree,
    IN gcLINKTREE FragmentTree
    )
{
    gctSIZE_T i, j, k;
    gctINT index = 0;

    /***************************************************************************
    ** I - Walk all attributes in the fragment tree and find the corresponding
    **     vertex output.
    */

    /* Walk all attributes. */
    for (i = 0; i < FragmentTree->attributeCount; ++i)
    {
        /* Get the gcATTRIBUTE pointer. */
        gcATTRIBUTE attribute = FragmentTree->shader->attributes[i];

        /* Only process valid attributes. */
        if (attribute == gcvNULL)
        {
            continue;
        }

        if (attribute->nameLength == gcSL_FRONT_FACING)
        {
            continue;
        }

        /* Walk all vertex outputs. */
        for (j = 0; j < VertexTree->outputCount; ++j)
        {
            /* Get the gcOUTPUT pointer. */
            gcOUTPUT output = VertexTree->shader->outputs[j];

            /* Output already got killed before. */
            if (output == gcvNULL)
            {
                continue;
            }

            /* Compare the names of the output and attribute. */
            if ((output->nameLength == attribute->nameLength)
            &&  (  ((gctINT) output->nameLength < 0)
                || gcmIS_SUCCESS(gcoOS_MemCmp(output->name,
                                              attribute->name,
                                              output->nameLength))
                )
            )
            {
                /* Make sure the varying variables are of the same type. */
                if ((output->type != attribute->type)
                ||  (output->arraySize != attribute->arraySize)
                )
                {
                    /* Type mismatch. */
                    return gcvSTATUS_VARYING_TYPE_MISMATCH;
                }

                for (k = 0; k < attribute->arraySize; ++k)
                {
	                /* Mark vertex output as in-use and link to fragment attribute. */
	                VertexTree->outputArray[j + k].inUse             = gcvTRUE;
	                VertexTree->outputArray[j + k].fragmentAttribute = i;
	                VertexTree->outputArray[j + k].fragmentIndex     = index;

	                if (output->nameLength != gcSL_POSITION)
	                {
	                    index += _getTypeSize(attribute->type);
                        VertexTree->outputArray[j + k].fragmentIndexEnd = index - 1;
	                }
                    else
                    {
                        /* Actually fragmentIndex and fragmentIndexEnd are no meanful for POSITION since
                         * it always be put at first attribute and assign it to R0 */
                        VertexTree->outputArray[j + k].fragmentIndexEnd = index;
                    }
                }

                /* Stop looping. */
                break;
            }
        }

        if (j == VertexTree->outputCount)
        {
            /* No matching vertex output found. */
            return gcvSTATUS_UNDECLARED_VARYING;
        }
    }

    /***************************************************************************
    ** II - Walk all outputs in the vertex tree to find any dead attribute.
    */

    /* Walk all outputs. */
    for (i = 0; i < VertexTree->outputCount; i++)
    {
        /* Output already got killed before. */
        if (VertexTree->shader->outputs[i] == gcvNULL)
        {
            continue;
        }

        /* Check if vertex output is dead. */
        if (!VertexTree->outputArray[i].inUse
        &&  ((gctINT) VertexTree->shader->outputs[i]->nameLength > 0)
        )
        {
            /* Mark vertex output as dead. */
            VertexTree->outputArray[i].tempHolding = -1;

            /* Free the gcOUTPUT structure from the shader. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL,
                                    VertexTree->shader->outputs[i]));

            /* Mark the output as invalid. */
            VertexTree->shader->outputs[i] = gcvNULL;
        }
    }

    /* Success. */
    return gcvSTATUS_OK;
}

gctUINT16
_SelectSwizzle(
    IN gctUINT16 Swizzle,
    IN gctUINT16 Source
    )
{
    /* Select on swizzle. */
    switch ((gcSL_SWIZZLE) Swizzle)
    {
    case gcSL_SWIZZLE_X:
        /* Select swizzle x. */
        return gcmSL_SOURCE_GET(Source, SwizzleX);

    case gcSL_SWIZZLE_Y:
        /* Select swizzle y. */
        return gcmSL_SOURCE_GET(Source, SwizzleY);

    case gcSL_SWIZZLE_Z:
        /* Select swizzle z. */
        return gcmSL_SOURCE_GET(Source, SwizzleZ);

    case gcSL_SWIZZLE_W:
        /* Select swizzle w. */
        return gcmSL_SOURCE_GET(Source, SwizzleW);

    default:
        return (gctUINT16) -1;
    }
}

gctUINT8
_Enable2Swizzle(
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
        return gcSL_SWIZZLE_YZZZ;

    case gcSL_ENABLE_YW:
        return gcSL_SWIZZLE_YWWW;

    case gcSL_ENABLE_ZW:
        return gcSL_SWIZZLE_ZWWW;

    case gcSL_ENABLE_XYZ:
        return gcSL_SWIZZLE_XYZZ;

    case gcSL_ENABLE_XYW:
        return gcSL_SWIZZLE_XYWW;

    case gcSL_ENABLE_XZW:
        return gcSL_SWIZZLE_XZWW;

    case gcSL_ENABLE_YZW:
        return gcSL_SWIZZLE_YZWW;

    case gcSL_ENABLE_XYZW:
        return gcSL_SWIZZLE_XYZW;

    default:
        break;

    }

    gcmFATAL("ERROR: Invalid enable 0x%04X", Enable);
    return gcSL_SWIZZLE_XYZW;
}

gctBOOL
_BackwardMOV(
    IN OUT gcLINKTREE Tree,
    IN gctSIZE_T CodeIndex
    )
{
    gcLINKTREE_TEMP dependency;
    gcSL_INSTRUCTION dependentCode;
    gcsLINKTREE_LIST_PTR defined;

    /* Get shader. */
    gcSHADER shader = Tree->shader;

    /* Get instruction. */
    gcSL_INSTRUCTION code = &shader->code[CodeIndex];

    /* Get target register information. */
    gcLINKTREE_TEMP codeTemp = &Tree->tempArray[code->tempIndex];

    /* The only dependency should be a temporary register. */
    if ( (codeTemp->dependencies == gcvNULL) ||
         (codeTemp->dependencies->next != gcvNULL) ||
         (codeTemp->dependencies->type != gcSL_TEMP) )
    {
        return gcvFALSE;
    }

    /* The dependent register should have only the current instruction as its
       user. */
    dependency = &Tree->tempArray[codeTemp->dependencies->index];
    if ( (dependency->users->next != gcvNULL) ||
         (dependency->users->type != gcSL_NONE) ||
         (dependency->users->index != (gctINT) CodeIndex) )
    {
        return gcvFALSE;
    }

    /* The dependent register is not an output. */
    if (dependency->lastUse == -1)
    {
        return gcvFALSE;
    }

    /* The register cannot be defined more than once. */
    if (codeTemp->defined->next != gcvNULL)
    {
        return gcvFALSE;
    }

    /* Make sure the entire dependency is used. */
    if (gcmSL_TARGET_GET(code->temp, Enable) != dependency->usage)
    {
        return gcvFALSE;
    }

    /* Make sure the dependency is swizzling the entire register usage. */
    if (_Enable2Swizzle(dependency->usage) != (code->source0 >> 8))
    {
        return gcvFALSE;
    }

    /* Make sure the dependency is not indexed accessing */
    for (defined = dependency->defined;
         defined != gcvNULL;
         defined = defined->next)
    {
        dependentCode = &shader->code[defined->index];
        if (gcmSL_TARGET_GET(dependentCode->temp, Indexed) != gcSL_NOT_INDEXED)
        {
            return gcvFALSE;
        }
    }

    /* Now we backward optimize the MOV, which means changing the target of
       the dependent instruction to our current target and deleting the
       dependent target. */
    for (defined = dependency->defined;
         defined != gcvNULL;
         defined = defined->next)
    {
        dependentCode = &shader->code[defined->index];

        /*dependentCode->temp        = code->temp;*/
        /* Copy the indexed value. */
        dependentCode->temp = gcmSL_TARGET_SET(dependentCode->temp, Indexed,
                                               gcmSL_TARGET_GET(code->temp, Indexed));
        dependentCode->tempIndex   = code->tempIndex;
        dependentCode->tempIndexed = code->tempIndexed;
    }

    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeTemp->dependencies));
    codeTemp->dependencies = dependency->dependencies;

    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, codeTemp->defined));
    codeTemp->defined = dependency->defined;

    gcmVERIFY_OK(gcoOS_ZeroMemory(code, sizeof(*code)));

    _Delete(Tree, &dependency->users);
    dependency->dependencies = gcvNULL;
    dependency->defined      = gcvNULL;

    /* Mark the target as not used. */
    dependency->inUse   = gcvFALSE;
    dependency->lastUse = -1;
    dependency->usage   = 0;

    return gcvTRUE;
}

gctBOOL
_PreviousMOV(
    IN gcLINKTREE Tree,
    IN gctUINT CodeIndex
    )
{
    gcSL_INSTRUCTION current, previous;
    gctUINT16 enable;
    gctUINT16 enableCurrent;
    gctUINT16 source0Current, source0Previous, source1Previous;
    gcsLINKTREE_LIST_PTR list;
    gcLINKTREE_TEMP tempCurrent, tempPrevious;

    /* Ignore instruction 0 - there is no previous instruction. */
    if (CodeIndex == 0)
    {
        return gcvFALSE;
    }

    /* Make sure this instruction is not the target of a JMP or CALL. */
    if (Tree->hints[CodeIndex].callers != gcvNULL)
    {
        return gcvFALSE;
    }

    /* Get the current and previous instructions. */
    current  = &Tree->shader->code[CodeIndex];
    previous = &Tree->shader->code[CodeIndex - 1];

    enableCurrent = gcmSL_TARGET_GET(current->temp, Enable);
    source0Current = current->source0;
    source0Previous = previous->source0;
    source1Previous = previous->source1;

    /* Make sure the source for the current instruction is the target of the
       previous instruction. */
    if ((previous->opcode == gcSL_LOAD)
    ||  (previous->opcode == gcSL_CMP)
    ||  (previous->opcode == gcSL_F2I)
    ||  (gcmSL_SOURCE_GET(source0Current, Type) != gcSL_TEMP)
    ||  (gcmSL_SOURCE_GET(source0Current, Indexed) != gcSL_NOT_INDEXED)
    ||  (current->source0Index != previous->tempIndex)
    ||  (gcmSL_TARGET_GET(current->temp, Indexed) != gcSL_NOT_INDEXED)
    ||  (gcmSL_TARGET_GET(previous->temp, Indexed) != gcSL_NOT_INDEXED)
    ||  (Tree->tempArray[current->source0Index].users->next != gcvNULL)
    ||  (gcmSL_TARGET_GET(current->temp, Format) != gcmSL_TARGET_GET(previous->temp, Format))
    )
    {
        return gcvFALSE;
    }

    /* Determine if current swizzle/usage and previous enable are the same. */
    enable = 0;
    if (enableCurrent & gcSL_ENABLE_X)
    {
        enable |= (1 << gcmSL_SOURCE_GET(source0Current, SwizzleX));
    }

    if (enableCurrent & gcSL_ENABLE_Y)
    {
        enable |= (1 << gcmSL_SOURCE_GET(source0Current, SwizzleY));
    }

    if (enableCurrent & gcSL_ENABLE_Z)
    {
        enable |= (1 << gcmSL_SOURCE_GET(source0Current, SwizzleZ));
    }

    if (enableCurrent & gcSL_ENABLE_W)
    {
        enable |= (1 << gcmSL_SOURCE_GET(source0Current, SwizzleW));
    }

    if (gcmSL_TARGET_GET(previous->temp, Enable) != enable)
    {
        return gcvFALSE;
    }

    /* If previous instruction is texld and source of MOV has
       swizzle seq other than enable of texld, we should abort
       it
    */
    if ((previous->opcode == gcSL_TEXLD || previous->opcode == gcSL_TEXLDP) &&
        (((enable & gcSL_ENABLE_X) && gcmSL_SOURCE_GET(source0Current, SwizzleX) != gcSL_SWIZZLE_X)||
         ((enable & gcSL_ENABLE_Y) && gcmSL_SOURCE_GET(source0Current, SwizzleY) != gcSL_SWIZZLE_Y)||
         ((enable & gcSL_ENABLE_Z) && gcmSL_SOURCE_GET(source0Current, SwizzleZ) != gcSL_SWIZZLE_Z)||
         ((enable & gcSL_ENABLE_W) && gcmSL_SOURCE_GET(source0Current, SwizzleW) != gcSL_SWIZZLE_W)))
    {
        return gcvFALSE;
    }

    /***************************************************************************
    *************** Yahoo, we can fold the MOV with the previous instruction. **
    ***************************************************************************/

    tempCurrent  = &Tree->tempArray[current->tempIndex];
    tempPrevious = &Tree->tempArray[previous->tempIndex];

    /* Modify define of current temp from current to previous. */
    for (list = tempCurrent->defined; list != gcvNULL; list = list->next)
    {
        if (list->index == (gctINT) CodeIndex)
        {
            list->index = CodeIndex - 1;
            break;
        }
    }

    /* Remove previous define from previous temp. */
    gcmVERIFY_OK(_RemoveItem(Tree,
                             &tempPrevious->defined,
                             gcSL_NONE,
                             CodeIndex - 1));

    /* Remove the previous temp from current temp dependency list. */
    gcmVERIFY_OK(_RemoveItem(Tree,
                             &tempCurrent->dependencies,
                             gcSL_TEMP,
                             previous->tempIndex));

    /* Remove the current usage of the previous temp. */
    gcmVERIFY_OK(_RemoveItem(Tree,
                             &tempPrevious->users,
                             gcSL_NONE,
                             CodeIndex));

    /* Test source0. */
    switch (gcmSL_SOURCE_GET(source0Previous, Type))
    {
    case gcSL_TEMP:
        /* fall through */
    case gcSL_ATTRIBUTE:
        /* Add the previous source0 dependency to the current temp. */
        gcmVERIFY_OK(
            gcLINKTREE_AddList(Tree,
                               &tempCurrent->dependencies,
                               (gcSL_TYPE) gcmSL_SOURCE_GET(source0Previous, Type),
                               previous->source0Index));

        /* Remove the previous source0 dependency to the previous temp. */
        gcmVERIFY_OK(_RemoveItem(Tree,
                                 &tempPrevious->dependencies,
                                 (gcSL_TYPE) gcmSL_SOURCE_GET(source0Previous, Type),
                                 previous->source0Index));

        /* Test for indexing. */
        if (gcmSL_SOURCE_GET(source0Previous, Indexed) != gcSL_NOT_INDEXED)
        {
            /* Add the previous source0 dependency to the current temp. */
            gcmVERIFY_OK(gcLINKTREE_AddList(Tree,
                                            &tempCurrent->dependencies,
                                            gcSL_TEMP,
                                            previous->source0Indexed));

            /* Remove the previous source0 dependency to the previous temp. */
            gcmVERIFY_OK(_RemoveItem(Tree,
                                     &tempPrevious->dependencies,
                                     gcSL_TEMP,
                                     previous->source0Indexed));
        }
        break;
    default:
        break;
    }

    /* Test source1. */
    switch (gcmSL_SOURCE_GET(source1Previous, Type))
    {
    case gcSL_TEMP:
        /* fall through */
    case gcSL_ATTRIBUTE:
        /* Add the previous source1 dependency to the current temp. */
        gcmVERIFY_OK(
            gcLINKTREE_AddList(Tree,
                               &tempCurrent->dependencies,
                               (gcSL_TYPE) gcmSL_SOURCE_GET(source1Previous, Type),
                               previous->source1Index));

        /* Remove the previous source1 dependency to the previous temp. */
        gcmVERIFY_OK(_RemoveItem(Tree,
                                 &tempPrevious->dependencies,
                                 (gcSL_TYPE) gcmSL_SOURCE_GET(source1Previous, Type),
                                 previous->source1Index));

        /* Test for indexing. */
        if (gcmSL_SOURCE_GET(source1Previous, Indexed) != gcSL_NOT_INDEXED)
        {
            /* Add the previous source1 dependency to the current temp. */
            gcmVERIFY_OK(gcLINKTREE_AddList(Tree,
                                            &tempCurrent->dependencies,
                                            gcSL_TEMP,
                                            previous->source1Indexed));

            /* Remove the previous source1 dependency to the previous temp. */
            gcmVERIFY_OK(_RemoveItem(Tree,
                                     &tempPrevious->dependencies,
                                     gcSL_TEMP,
                                     previous->source1Indexed));
        }
        break;
    default:
        break;
    }

    /* Check if the enables are the same, */
    /* and the target enable and swizzle are in the same order. */
    if ((previous->opcode != gcSL_DP3) && (previous->opcode != gcSL_DP4)
    &&  ((enableCurrent != gcmSL_TARGET_GET(previous->temp, Enable))
    ||  ((enableCurrent & gcSL_ENABLE_X) && gcmSL_SOURCE_GET(source0Current, SwizzleX) != gcSL_SWIZZLE_X)
    ||  ((enableCurrent & gcSL_ENABLE_Y) && gcmSL_SOURCE_GET(source0Current, SwizzleY) != gcSL_SWIZZLE_Y)
    ||  ((enableCurrent & gcSL_ENABLE_Z) && gcmSL_SOURCE_GET(source0Current, SwizzleZ) != gcSL_SWIZZLE_Z)
    ||  ((enableCurrent & gcSL_ENABLE_W) && gcmSL_SOURCE_GET(source0Current, SwizzleW) != gcSL_SWIZZLE_W)))
    {
        /* Complicated case--need to change previous sources to match current target. */

        /* Modify previous source(s) to match current target. */
        if (enableCurrent & gcSL_ENABLE_X)
        {
            switch (gcmSL_SOURCE_GET(source0Current, SwizzleX))
            {
            case gcSL_SWIZZLE_X:
                /* No change is needed. */
                break;
            case gcSL_SWIZZLE_Y:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleX, gcmSL_SOURCE_GET(source0Previous, SwizzleY));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleX, gcmSL_SOURCE_GET(source1Previous, SwizzleY));
                break;
            case gcSL_SWIZZLE_Z:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleX, gcmSL_SOURCE_GET(source0Previous, SwizzleZ));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleX, gcmSL_SOURCE_GET(source1Previous, SwizzleZ));
                break;
            case gcSL_SWIZZLE_W:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleX, gcmSL_SOURCE_GET(source0Previous, SwizzleW));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleX, gcmSL_SOURCE_GET(source1Previous, SwizzleW));
                break;
            default:
                break;
            }
        }

        if (enableCurrent & gcSL_ENABLE_Y)
        {
            switch (gcmSL_SOURCE_GET(source0Current, SwizzleY))
            {
            case gcSL_SWIZZLE_X:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleY, gcmSL_SOURCE_GET(source0Previous, SwizzleX));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleY, gcmSL_SOURCE_GET(source1Previous, SwizzleX));
                break;
            case gcSL_SWIZZLE_Y:
                /* No change is needed. */
                break;
            case gcSL_SWIZZLE_Z:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleY, gcmSL_SOURCE_GET(source0Previous, SwizzleZ));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleY, gcmSL_SOURCE_GET(source1Previous, SwizzleZ));
                break;
            case gcSL_SWIZZLE_W:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleY, gcmSL_SOURCE_GET(source0Previous, SwizzleW));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleY, gcmSL_SOURCE_GET(source1Previous, SwizzleW));
                break;
            default:
                break;
            }
        }

        if (enableCurrent & gcSL_ENABLE_Z)
        {
            switch (gcmSL_SOURCE_GET(source0Current, SwizzleZ))
            {
            case gcSL_SWIZZLE_X:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleZ, gcmSL_SOURCE_GET(source0Previous, SwizzleX));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleZ, gcmSL_SOURCE_GET(source1Previous, SwizzleX));
                break;
            case gcSL_SWIZZLE_Y:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleZ, gcmSL_SOURCE_GET(source0Previous, SwizzleY));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleZ, gcmSL_SOURCE_GET(source1Previous, SwizzleY));
                break;
            case gcSL_SWIZZLE_Z:
                /* No change is needed. */
                break;
            case gcSL_SWIZZLE_W:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleZ, gcmSL_SOURCE_GET(source0Previous, SwizzleW));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleZ, gcmSL_SOURCE_GET(source1Previous, SwizzleW));
                break;
            default:
                break;
            }
        }

        if (enableCurrent & gcSL_ENABLE_W)
        {
            switch (gcmSL_SOURCE_GET(source0Current, SwizzleW))
            {
            case gcSL_SWIZZLE_X:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleW, gcmSL_SOURCE_GET(source0Previous, SwizzleX));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleW, gcmSL_SOURCE_GET(source1Previous, SwizzleX));
                break;
            case gcSL_SWIZZLE_Y:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleW, gcmSL_SOURCE_GET(source0Previous, SwizzleY));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleW, gcmSL_SOURCE_GET(source1Previous, SwizzleY));
                break;
            case gcSL_SWIZZLE_Z:
                previous->source0 = gcmSL_SOURCE_SET(previous->source0, SwizzleW, gcmSL_SOURCE_GET(source0Previous, SwizzleZ));
                previous->source1 = gcmSL_SOURCE_SET(previous->source1, SwizzleW, gcmSL_SOURCE_GET(source1Previous, SwizzleZ));
                break;
            case gcSL_SWIZZLE_W:
                /* No change is needed. */
                break;
            default:
                break;
            }
        }
    }

    /* Fold it! */
    previous->temp        = current->temp;
    previous->tempIndex   = current->tempIndex;
    previous->tempIndexed = current->tempIndexed;
    gcmVERIFY_OK(gcoOS_ZeroMemory(current, gcmSIZEOF(*current)));

    return gcvTRUE;
}

gceSTATUS
gcLINKTREE_Optimize(
    IN OUT gcLINKTREE Tree
    )
{
    gctSIZE_T i;
    gcSL_INSTRUCTION code;
    gcSL_INSTRUCTION usedCode;
    gcSHADER shader = Tree->shader;

    /***************************************************************************
    ** I - Find all MOV instructions.
    */

    /* Walk all instructions. */
    for (i = 0; i < shader->codeCount; i++)
    {
        /* Get instruction. */
        code = &shader->code[i];

        /* Test for MOV instruction. */
        if (code->opcode == gcSL_MOV)
        {
            gcsLINKTREE_LIST_PTR dependency;
            gcsLINKTREE_LIST_PTR user;
            gcLINKTREE_TEMP dependencyTemp;
            gctINT reDefinedPc;
            gcsLINKTREE_LIST_PTR defined;
            gctUINT16 enable;

            /* Get the temporary register. */
            gcLINKTREE_TEMP codeTemp = &Tree->tempArray[code->tempIndex];

            /* See if we can fold this MOV with the previous instruction. */
            if (_PreviousMOV(Tree, i))
            {
                continue;
            }

            /* Cannot optimize a MOV if the temporary register is not fully
               written to here. */
            if (gcmSL_TARGET_GET(code->temp, Enable) ^ codeTemp->usage)
            {
                continue;
            }

            /* Cannot optimize a MOV if the source is a constant. */
            if (gcmSL_SOURCE_GET(code->source0, Type) == gcSL_CONSTANT)
            {
                continue;
            }

            /* Cannot optimize a MOV if the target is an argument. */
            if (codeTemp->owner != gcvNULL)
            {
                continue;
            }

            /* Cannot optimize a MOV if the source is an attribute or uniform
               and the temporay register is used as an output. */
            if (gcmSL_SOURCE_GET(code->source0, Type) != gcSL_TEMP)
            {
                /* Walk through all users. */
                for (user = codeTemp->users; user != gcvNULL; user = user->next)
                {
                    /* Bail out if the user is an output. */
                    if (user->type == gcSL_OUTPUT)
                    {
                        break;
                    }

                    /* Bail out if the user is a STORE instruction. */
                    /* TODO - Need to remove when STORE can handle uniform and swizzle, etc.*/
                    if (user->type == gcSL_NONE)
                    {
                        gcSL_INSTRUCTION userCode = &shader->code[user->index];

                        if (userCode->opcode == gcSL_STORE)
                        {
                            break;
                        }
                    }
                }

                if (user != gcvNULL)
                {
                    /* Don't optimize MOV. */
                    continue;
                }
            }

            /* Cannot optimize if the source is an argument. */
            else if ((gcmSL_SOURCE_GET(code->source0, Type) == gcSL_TEMP)
            &&       (Tree->tempArray[code->source0Index].owner != gcvNULL)
            )
            {
                continue;
            }

            /* Try a backwards optimization. */
            if (_BackwardMOV(Tree, i))
            {
                continue;
            }

            /* Cannot optimize a MOV if target is not optimizable. */
            if ((codeTemp->constUsage[0] == -1) ||
                (codeTemp->constUsage[1] == -1) ||
                (codeTemp->constUsage[2] == -1) ||
                (codeTemp->constUsage[3] == -1))
            {
                continue;
            }

            /* Cannot optimize a MOV if there are multiple dependencies. */
            if ((codeTemp->dependencies != gcvNULL) &&
                (codeTemp->dependencies->next != gcvNULL))
            {
                continue;
            }

            /* Cannot optimize a MOV if the dependency is an output. */
            if ((codeTemp->dependencies != gcvNULL)
            &&  (codeTemp->dependencies->type == gcSL_TEMP)
            &&  (Tree->tempArray[codeTemp->dependencies->index].lastUse == -1)
            )
            {
                continue;
            }

            /* Cannot optimize a MOV if there are multiple definintions. */
            if (codeTemp->defined->next != gcvNULL)
            {
                continue;
            }

            /* Cannot optimize a MOV if the target is used as an index. */
            if (codeTemp->isIndex)
            {
                continue;
            }

            /* Cannot optimize a MOV if the target enable and source swizzle are not in the same order. */
            enable = gcmSL_TARGET_GET(code->temp, Enable);
            if ((enable & gcSL_ENABLE_X) && gcmSL_SOURCE_GET(code->source0, SwizzleX) != gcSL_SWIZZLE_X)
            {
                continue;
            }

            if ((enable & gcSL_ENABLE_Y) && gcmSL_SOURCE_GET(code->source0, SwizzleY) != gcSL_SWIZZLE_Y)
            {
                continue;
            }

            if ((enable & gcSL_ENABLE_Z) && gcmSL_SOURCE_GET(code->source0, SwizzleZ) != gcSL_SWIZZLE_Z)
            {
                continue;
            }

            if ((enable & gcSL_ENABLE_W) && gcmSL_SOURCE_GET(code->source0, SwizzleW) != gcSL_SWIZZLE_W)
            {
                continue;
            }

            /* DependencyTemp cannot be redefined before all users. */

            /* Find the closest dependency temp's redefined pc. */
            if (codeTemp->dependencies != gcvNULL)
            {
                dependencyTemp = &Tree->tempArray[codeTemp->dependencies->index];
                reDefinedPc = shader->codeCount;
                for (defined = dependencyTemp->defined; defined; defined = defined->next)
                {
                    gctINT definedPc = defined->index;

                    if (definedPc > (gctINT) i && definedPc < reDefinedPc)
                    {
                        reDefinedPc = definedPc;
                    }
                }

                /* Check if any user after the dependency temp's redefined pc. */
                for (user = codeTemp->users; user != gcvNULL; user = user->next)
                {
                    /* Output user index is always infinite */
                    if ((user->index >= reDefinedPc) || (user->type == gcSL_OUTPUT))
                    {
                        break;
                    }
                }

                if (user)
                {
                    continue;
                }
            }

            /*
                1: mul temp(1), attr(1), 4      users: 5        dep: ...
                5: mov temp(5), temp(1)         users: 8, 12    dep: 1
                8: add temp(8), temp(5), ...    users: ...      dep: 5, ...
                12: mul temp(12), temp(5), ...  users: ...      dep: 5, ...

                1:                              users: 8, 12    dep: ...
                8:  add temp(8), temp(1), ...   users: ...      dep: 1
                12: mul temp(12), temp(1), ...  users: ...      dep: 1

                (5) FOR EACH USER:  PATCH CODE
                                    REMOVE (5) FROM DEP
                                    ADD DEP FROM (5)
                (5) FOR EACH DEP:   REMOVE (5) FROM USER
                                    ADD USERS FROM (5)
            */

            /* For all users, patch the source and modify their dependencies. */
            for (user = codeTemp->users; user != gcvNULL; user = user->next)
            {
                gctUINT16 source = code->source0;

                /* Get the user instruction and target register. */
                gcSL_INSTRUCTION userCode = &shader->code[user->index];
                gcLINKTREE_TEMP userTemp  = &Tree->tempArray[userCode->tempIndex];

                /* Get source operands. */
                gctUINT16 source0 = userCode->source0;
                gctUINT16 source1 = userCode->source1;

                switch (userCode->opcode)
                {
                case gcSL_JMP:
                    /* fall through */
                case gcSL_CALL:
                    /* fall through */
                case gcSL_KILL:
                    /* fall through */
                case gcSL_NOP:
                    /* fall through */
                case gcSL_RET:
                    userTemp = gcvNULL;
                    break;
                default:
                    break;
                }

                if (user->type == gcSL_OUTPUT)
                {
                    Tree->outputArray[user->index].tempHolding =
                        code->source0Index;

                    if (Tree->shader->outputs[user->index])
                    {
                        Tree->shader->outputs[user->index]->tempIndex =
                            code->source0Index;
                    }

                    continue;
                }

                /* Test if source0 is the target of the MOV. */
                if ( (gcmSL_SOURCE_GET(source0, Type) == gcSL_TEMP) &&
                     (userCode->source0Index == code->tempIndex) )
                {
                    source0 = gcmSL_SOURCE_SET(source0, Type, gcmSL_SOURCE_GET(source, Type));
                    source0 = gcmSL_SOURCE_SET(source0, Indexed,gcmSL_SOURCE_GET(source, Indexed));
                    source0 = gcmSL_SOURCE_SET(source0, Format, gcmSL_SOURCE_GET(source, Format));
                    source0 = gcmSL_SOURCE_SET(source0, SwizzleX, _SelectSwizzle(gcmSL_SOURCE_GET(source0, SwizzleX), source));
                    source0 = gcmSL_SOURCE_SET(source0, SwizzleY, _SelectSwizzle(gcmSL_SOURCE_GET(source0, SwizzleY), source));
                    source0 = gcmSL_SOURCE_SET(source0, SwizzleZ, _SelectSwizzle(gcmSL_SOURCE_GET(source0, SwizzleZ), source));
                    source0 = gcmSL_SOURCE_SET(source0, SwizzleW, _SelectSwizzle(gcmSL_SOURCE_GET(source0, SwizzleW), source));

                    userCode->source0        = source0;
                    userCode->source0Index   = code->source0Index;
                    userCode->source0Indexed = code->source0Indexed;
                }

                /* Test if source1 is the target of the MOV. */
                if ( (gcmSL_SOURCE_GET(source1, Type) == gcSL_TEMP) &&
                     (userCode->source1Index == code->tempIndex) )
                {
                    source1 = gcmSL_SOURCE_SET(source1, Type, gcmSL_SOURCE_GET(source, Type));
                    source1 = gcmSL_SOURCE_SET(source1, Indexed, gcmSL_SOURCE_GET(source, Indexed));
                    source1 = gcmSL_SOURCE_SET(source1, Format, gcmSL_SOURCE_GET(source, Format));
                    source1 = gcmSL_SOURCE_SET(source1, SwizzleX, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleX), source));
                    source1 = gcmSL_SOURCE_SET(source1, SwizzleY, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleY), source));
                    source1 = gcmSL_SOURCE_SET(source1, SwizzleZ, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleZ), source));
                    source1 = gcmSL_SOURCE_SET(source1, SwizzleW, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleW), source));

                    userCode->source1        = *(gctUINT16 *) &source1;
                    userCode->source1Index   = code->source0Index;
                    userCode->source1Indexed = code->source0Indexed;
                }

                if (userCode->opcode == gcSL_TEXLD ||
                    userCode->opcode == gcSL_TEXLDP)
                {
                    gcSL_INSTRUCTION texAttrCode = &shader->code[user->index - 1];

                    if (texAttrCode->opcode == gcSL_TEXBIAS ||
                        texAttrCode->opcode == gcSL_TEXLOD ||
                        texAttrCode->opcode == gcSL_TEXGRAD)
                    {
                        gctUINT16 source1OfTexAttrCode = texAttrCode->source1;

                        if ( (gcmSL_SOURCE_GET(source1OfTexAttrCode, Type) == gcSL_TEMP) &&
                             (texAttrCode->source1Index == code->tempIndex) )
                        {
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, Type, gcmSL_SOURCE_GET(source, Type));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, Indexed, gcmSL_SOURCE_GET(source, Indexed));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, Format, gcmSL_SOURCE_GET(source, Format));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, SwizzleX, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleX), source));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, SwizzleY, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleY), source));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, SwizzleZ, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleZ), source));
                            source1OfTexAttrCode = gcmSL_SOURCE_SET(source1OfTexAttrCode, SwizzleW, _SelectSwizzle(gcmSL_SOURCE_GET(source1, SwizzleW), source));

                            texAttrCode->source1        = *(gctUINT16 *) &source1OfTexAttrCode;
                            texAttrCode->source1Index   = code->source0Index;
                            texAttrCode->source1Indexed = code->source0Indexed;
                        }
                    }
                }

                if (userCode->opcode != gcSL_STORE)
                {
                    if (userTemp != gcvNULL)
                    {
                        /* Remove target dependency. */
                        _RemoveItem(Tree,
                                    &userTemp->dependencies,
                                    gcSL_TEMP,
                                    code->tempIndex);

                        /* Append source dependencies. */
                        for (dependency = codeTemp->dependencies;
                             dependency != gcvNULL;
                             dependency = dependency->next)
                        {
                            gcmVERIFY_OK(
                                gcLINKTREE_AddList(Tree,
                                                   &userTemp->dependencies,
                                                   dependency->type,
                                                   dependency->index));
                        }
                    }

                    if (gcmSL_TARGET_GET(userCode->temp, Indexed) != gcSL_NOT_INDEXED)
                    {
                        if (Tree->tempArray[userCode->tempIndex].variable)
                        {
                            gcVARIABLE variable = Tree->tempArray[userCode->tempIndex].variable;

                            gctUINT startIndex, endIndex, j;
                            gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                            _GetVariableIndexingRange(Tree->shader, variable, gcvFALSE,
                                                      &startIndex, &endIndex);
                            gcmASSERT(startIndex == variable->tempIndex);

                            for (j = startIndex + 1; j < endIndex; j++)
                            {
                                /* Get pointer to temporary register. */
                                gcLINKTREE_TEMP userTemp = &Tree->tempArray[j];

                                /* Remove target dependency. */
                                _RemoveItem(Tree,
                                            &userTemp->dependencies,
                                            gcSL_TEMP,
                                            code->tempIndex);

                                /* Append source dependencies. */
                                for (dependency = codeTemp->dependencies;
                                     dependency != gcvNULL;
                                     dependency = dependency->next)
                                {
                                    gcmVERIFY_OK(
                                        gcLINKTREE_AddList(Tree,
                                                           &userTemp->dependencies,
                                                           dependency->type,
                                                           dependency->index));
                                }
                            }
                        }
                    }
                }
                else if (userCode->tempIndex == code->tempIndex)
                {
                    userCode->temp        = gcmSL_TARGET_SET(userCode->temp, Indexed,
                                                             gcmSL_SOURCE_GET(source, Indexed));
                    userCode->tempIndex   = code->source0Index;
                    userCode->tempIndexed = code->source0Indexed;
                }
            }

            /* For all dependencies, modify their users. */
            for (dependency = codeTemp->dependencies;
                 dependency != gcvNULL;
                 dependency = dependency->next)
            {
                /* Get root of users tree. */
                gcsLINKTREE_LIST_PTR * root = gcvNULL;
                gctINT * lastUse = gcvNULL;

                switch (dependency->type)
                {
                case gcSL_ATTRIBUTE:
                    root    = &Tree->attributeArray[dependency->index].users;
                    lastUse = &Tree->attributeArray[dependency->index].lastUse;
                    break;

                case gcSL_UNIFORM:
                    root    = gcvNULL;
                    lastUse = gcvNULL;
                    break;

                case gcSL_TEMP:
                    root    = &Tree->tempArray[dependency->index].users;
                    lastUse = &Tree->tempArray[dependency->index].lastUse;
                    break;

                default:
                    gcmFATAL("ERROR: Invalid dependency %u at 0x%x",
                          dependency->type,
                          dependency);
                }

				if ((root != gcvNULL) && (lastUse != gcvNULL))
				{
                    /* Update last usage. */
                    if ( lastUse && ((codeTemp->lastUse > *lastUse) || (codeTemp->lastUse == -1)) )
                    {
                        *lastUse = codeTemp->lastUse;
                    }

                    /* Remove the current instruction as a user. */
                    _RemoveItem(Tree, root, gcSL_NONE, i);

                    /* Append all users. */
                    for (user = codeTemp->users; user != gcvNULL; user = user->next)
                    {
                        gcmVERIFY_OK(gcLINKTREE_AddList(Tree,
                                                        root,
                                                        user->type,
                                                        user->index));
                    }
				}
            }

            /* Set current instruction to NOP. */
            gcmVERIFY_OK(gcoOS_ZeroMemory(code, sizeof(struct _gcSL_INSTRUCTION)));

            /* Delete the target's definition tree. */
            _Delete(Tree, &codeTemp->defined);

            /* Delete the target's dependency tree. */
            _Delete(Tree, &codeTemp->dependencies);

            /* Delete the target's user tree. */
            _Delete(Tree, &codeTemp->users);

            /* Mark the target as not used. */
            codeTemp->inUse   = gcvFALSE;
            codeTemp->lastUse = -1;
            codeTemp->usage   = 0;
        }
    }

    /* II - Find all MOVs with constants. */

    /* Walk all instructions. */
    for (i = 0; i < shader->codeCount; i++)
    {
        /* Get instruction. */
        code = &shader->code[i];

        /* Test for MOV instruction. */
        if ((code->opcode == gcSL_MOV)

        /* Test if the source is a constant. */
        &&  (gcmSL_SOURCE_GET(code->source0, Type) == gcSL_CONSTANT)

        /* We cannot optimize an output register. */
        &&  (Tree->tempArray[code->tempIndex].lastUse != -1)
        )
        {
            gctUINT16 target = code->temp;
            gcLINKTREE_TEMP temp = &Tree->tempArray[code->tempIndex];
            gctINT index;
            gctUINT32 valueInt;
            gcuFLOAT_UINT32 value;

            usedCode = &shader->code[Tree->tempArray[code->tempIndex].lastUse];

            /* The used instruction has dest */
            if(gcmSL_TARGET_GET(usedCode->temp, Enable))
            {
                /* The dest dependen is more than one */
                if(Tree->tempArray[usedCode->tempIndex].dependencies != gcvNULL
                    && Tree->tempArray[usedCode->tempIndex].dependencies->next != gcvNULL)
                    continue;
            }

            valueInt = (code->source0Index & 0xFFFF)
                     | (code->source0Indexed << 16);
            value.u = valueInt;

            for (index = 0; index < 4; ++index)
            {
                if (gcmSL_TARGET_GET(target, Enable) & 1)
                {
                    switch (temp->constUsage[index])
                    {
                    case 0:
                        temp->constUsage[index] = 1;
                        temp->constValue[index] = value.f;
                        break;

                    case 1:
                        if (temp->constValue[index] != value.f)
                        {
                            temp->constUsage[index] = -1;
                        }
                        break;

                    default:
                        break;
                    }
                }

                target = gcmSL_TARGET_SET(target,
                                          Enable,
                                          gcmSL_TARGET_GET(target,
                                                           Enable) >> 1);
            }
        }

        else
        switch (code->opcode)
        {
        case gcSL_JMP:
            /* fall through */
        case gcSL_CALL:
            /* fall through */
        case gcSL_KILL:
            /* fall through */
        case gcSL_NOP:
            /* fall through */
        case gcSL_RET:
            break;

        default:
            {
                /* Mark entire register as non optimizable. */
                if (code->opcode != gcSL_TEXBIAS &&
                    code->opcode != gcSL_TEXGRAD &&
                    code->opcode != gcSL_TEXLOD)
                {
                    gcLINKTREE_TEMP temp = &Tree->tempArray[code->tempIndex];

                    temp->constUsage[0] =
                    temp->constUsage[1] =
                    temp->constUsage[2] =
                    temp->constUsage[3] = -1;
                }
            }
            break;
        }
    }

    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gcLINKTREE_Cleanup(
    IN OUT gcLINKTREE Tree
    )
{
    gctSIZE_T i;
    gcSHADER shader = Tree->shader;

    for (i = 0; i < shader->codeCount; ++i)
    {
        gcSL_INSTRUCTION code = shader->code + i;
        gcLINKTREE_TEMP temp;

        switch (code->opcode)
        {
        case gcSL_JMP:
            /* fall through */
        case gcSL_CALL:
            /* fall through */
        case gcSL_RET:
            /* fall through */
        case gcSL_NOP:
            /* fall through */
        case gcSL_KILL:
            /* fall through */
        case gcSL_TEXBIAS:
        case gcSL_TEXGRAD:
        case gcSL_TEXLOD:
            /* Skip texture state instructions. */
        break;

        default:
            temp = &Tree->tempArray[code->tempIndex];
            if (!temp->inUse)
            {
                code->opcode         = gcSL_NOP;
                code->temp           = 0;
                code->tempIndex      = 0;
                code->tempIndexed    = 0;
                code->source0        = 0;
                code->source0Index   = 0;
                code->source0Indexed = 0;
                code->source1        = 0;
                code->source1Index   = 0;
                code->source1Indexed = 0;

                if (temp->dependencies != gcvNULL)
                {
                    _Delete(Tree, &temp->dependencies);
                }

                if (temp->users != gcvNULL)
                {
                    _Delete(Tree, &temp->users);
                }
            }
        }
    }

    return gcvSTATUS_OK;
}


/* for orignal code Index (before removing NOPs) in Shader,
 * get the new index to NOP removed code, if the original code
 * is NOP, find the next code which is non NOP and get the
 * the new index, if all the rest codes are NOPs, return index
 * to Shader's last instruction (lastInstruction -1) */
static gctINT
_GetNewIndex2NextCode(
    IN gcSHADER    Shader,
    IN gctINT16 *  Index_map,
    IN gctINT      Map_entries,
    IN gctINT      Index
    )
{
    gctSIZE_T new_index;

    gcmASSERT(Index >= 0 && Index < Map_entries);

    while (Index < Map_entries && Index_map[Index] == -1)
        Index++;

    if (Index >= Map_entries)
    {
        /* no next non NOP instruction found, use the lastInstruction */
        new_index = Shader->lastInstruction;
    }
    else
    {
        new_index = Index_map[Index];
    }

    gcmASSERT(new_index == Shader->lastInstruction ||
              Shader->code[new_index].opcode != gcSL_NOP);
    return new_index;
}

/* for orignal code Index (before removing NOPs) in Shader,
 * get the new index to NOP removed code, if the original code
 * is NOP, find the previous code which is non NOP and get the
 * the new index, if all the rest codes are NOPs, return
 * Shader's first Instruction */
gctINT
_GetNewIndex2PrevCode(
    IN gcSHADER    Shader,
    IN gctINT16 *  Index_map,
    IN gctINT      Map_entries,
    IN gctINT      Index
    )
{
    gctSIZE_T new_index;

    gcmASSERT(Index >= 0);

    while (Index >= 0 && Index_map[Index] == -1)
        Index--;

    if (Index < 0)
    {
        /* no next non NOP instruction found, use the lastInstruction */
        new_index = 0;
    }
    else
    {
        new_index = Index_map[Index];
    }

    gcmASSERT(new_index == 0 ||
              Shader->code[new_index].opcode != gcSL_NOP);
    return new_index;
}


gceSTATUS
gcLINKTREE_RemoveNops(
    IN OUT gcLINKTREE Tree
    )
{
    gctSIZE_T    i;
    gcSHADER     Shader         = Tree->shader;

    gceSTATUS    status;
    gctINT16 *   index_map;
    gctSIZE_T    map_entries    = Shader->codeCount;
    gctSIZE_T    bytes          = map_entries * gcmSIZEOF(gctINT16);
    gctPOINTER   pointer        = gcvNULL;
    gctINT       nop_counts     = 0;
    gctINT       first_nop_index=-1;  /* first available nop index */
    gctBOOL      has_jump_to_last_nop_stmt = gcvFALSE;

     /* Check for empty shader */
    if (bytes == 0)
    {
        return gcvSTATUS_OK;
    }

    gcmASSERT(Shader->lastInstruction <= Shader->codeCount);

    /* Allocate array of gcsSL_JUMPED_FROM structures. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Return on error. */
        return status;
    }

    index_map = pointer;

    /* Initialize array with -1. */
    for (i = 0; i < Shader->codeCount; ++i)
        index_map[i] = -1;

    /* check if the lastInstruction is NOP */
    if (Shader->code[Shader->lastInstruction-1].opcode == gcSL_NOP)
    {
        /* find the pervious non NOP instruction */
        gctINT last = Shader->lastInstruction;
        while (last > 0 &&
               Shader->code[--last].opcode == gcSL_NOP) ;

        if (Shader->code[last].opcode == gcSL_NOP)
        {
            /* all instructions are NOP */
            return gcvSTATUS_OK;
        }
        else
        {
            /* found non NOP instruction */
            Shader->codeCount = Shader->lastInstruction = last+1;
        }
    } /* if */

    /* Walk through all the instructions, finding JMP instructions. */
    for (i = 0; i < Shader->codeCount; ++i)
    {
        if (Shader->code[i].opcode == gcSL_NOP)
        {
            if (first_nop_index == -1)
                first_nop_index = i;

            nop_counts ++;
        }
        else
        {
            /* not a nop, copy to last nop location if there is one */
            if (first_nop_index != -1)
            {
                /* copy the instruction */
                Shader->code[first_nop_index] = Shader->code[i];
                Shader->code[i].opcode = gcSL_NOP;

                /* map the index */
                index_map[i] = (gctINT16) first_nop_index;

                /* make the next instruction to be first available nop instruction */
                first_nop_index++;
            }
            else
            {
                index_map[i] = (gctINT16) i;
            }
        }
    } /* for */

    /* fix the lastInstruction */
    Shader->codeCount =
        Shader->lastInstruction = Shader->lastInstruction - nop_counts;

    for (i = 0; i < Shader->codeCount; ++i)
    {
        /* fix up jmp/call. */
        if (Shader->code[i].opcode == gcSL_JMP ||
            Shader->code[i].opcode == gcSL_CALL)
        {
            /* Get target of jump instrution. */
            gctINT new_label =
                _GetNewIndex2NextCode(Shader,
                                      index_map,
                                      map_entries,
                                      Shader->code[i].tempIndex);

            if (new_label >= (gctINT)Shader->lastInstruction)
                has_jump_to_last_nop_stmt = gcvTRUE;

            Shader->code[i].tempIndex = (gctUINT16)new_label;
        }
    } /* for */

    if (has_jump_to_last_nop_stmt == gcvTRUE)
    {
        /* the code can jump to last NOP stmt:
         *
         *    005   JMP 010
         *    ...
         *    010   NOP
         *    011   NOP
         *
         * we need to keep the last NOP in the code to
         * make it semantic correct after remove nops
         */
        gcmASSERT(Shader->code[Shader->lastInstruction].opcode == gcSL_NOP);
        Shader->codeCount =
            Shader->lastInstruction = Shader->lastInstruction + 1;
    } /* if */

    /* fix functions */
    /******************** Check the argument temp index Function **********************/
    for (i = 0; i < Shader->functionCount; ++i)
    {
        gcFUNCTION function = Shader->functions[i];
        gctINT newCodeStart;
        gctINT newCodeCount = 0;
        gctSIZE_T j;

        newCodeStart =
            _GetNewIndex2NextCode(Shader,
                                  index_map,
                                  map_entries,
                                  function->codeStart);

        /* count the new code in the function */
        for (j = 0; j < function->codeCount; j++)
        {
            if (index_map[function->codeStart + j] != -1)
                newCodeCount++;
        }

        gcmASSERT(newCodeCount != 0);  /* at least one instruction (RET) */

        function->codeStart = newCodeStart;
        function->codeCount = newCodeCount;
    } /* for */

    /*************** Check the argument temp index for Kernel Function *****************/
    for (i = 0; i < Shader->kernelFunctionCount; ++i)
    {
        gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[i];
        gctINT newCodeStart;
        gctINT newCodeCount = 0;
        gctSIZE_T j;

        newCodeStart =
            _GetNewIndex2NextCode(Shader,
                                  index_map,
                                  map_entries,
                                  kernelFunction->codeStart);

        /* count the new code in the function */
        for (j = 0; j < kernelFunction->codeCount; j++)
        {
            if (index_map[kernelFunction->codeStart + j] != -1)
                newCodeCount++;
        }

        kernelFunction->codeStart = newCodeStart;
        kernelFunction->codeCount = newCodeCount;
        kernelFunction->codeEnd   = newCodeStart + newCodeCount;
    } /* for */

    /* fix labels */
    if (Shader->labels != gcvNULL)
    {
    }

    do {
        gcSL_BRANCH_LIST			branch;

        /* Walk all attributes. */
        for (i = 0; i < Tree->attributeCount; i++)
        {
            gcLINKTREE_ATTRIBUTE attribute = &Tree->attributeArray[i];
            gcsLINKTREE_LIST_PTR users;

            if (attribute->lastUse >= 0)
            {
                attribute->lastUse = _GetNewIndex2NextCode(Shader,
                                                       index_map,
                                                       map_entries,
                                                       attribute->lastUse);
            }

            /* Loop while there are users. */
            users = attribute->users;
            while (users != gcvNULL)
            {
                users->index = _GetNewIndex2NextCode(Shader,
                                                     index_map,
                                                     map_entries,
                                                     users->index);

                /* Remove the user from the list. */
                users = users->next;
            }
        } /* for */

        /* Loop through all temporary registers. */
        for (i = 0; i < Tree->tempCount; i++)
        {
            gcLINKTREE_TEMP             tempArray = &Tree->tempArray[i];
            gcsLINKTREE_LIST_PTR		defined, users;

            if (tempArray->lastUse >= 0)
                tempArray->lastUse =  _GetNewIndex2NextCode(Shader,
                                                            index_map,
                                                            map_entries,
                                                            tempArray->lastUse);

            /* Loop while there are definitions. */
            defined = tempArray->defined;
            while (defined != gcvNULL)
            {
                /* fix the definition's index. */
                defined->index = _GetNewIndex2NextCode(Shader,
                                                       index_map,
                                                       map_entries,
                                                       defined->index);

                defined = defined->next;
            }

            /* Loop while we have users. */
            users = tempArray->users;
            while (users != gcvNULL)
            {
                /* fix the users' index. */
                users->index = _GetNewIndex2NextCode(Shader,
                                                     index_map,
                                                     map_entries,
                                                     users->index);

                users = users->next;
            }
        } /* for */

        branch = Tree->branch;
        while (branch != gcvNULL)
        {
            /* fix branch's target. */
            branch->target = _GetNewIndex2NextCode(Shader,
                                                   index_map,
                                                   map_entries,
                                                   branch->target);
            branch = branch->next;
        } /* while */

        /* compact hints */
        for (i = 0; i < map_entries; ++i)
        {
            if (index_map[i] != -1 && index_map[i] != (gctINT16)i)
            {
                gcsCODE_HINT_PTR hint;
                gctINT new_hint_index;

                /* find the hint's new location in hint array */
                new_hint_index = index_map[i];
                gcmASSERT(new_hint_index < (gctINT)i);

                /* copy to new location */
                Tree->hints[new_hint_index] = Tree->hints[i];
                hint = &Tree->hints[new_hint_index];

                /* fix the hint's lastUseForTemp */
                if (hint->lastUseForTemp >= 0)
                {
                    hint->lastUseForTemp =
                        _GetNewIndex2NextCode(Shader,
                                              index_map,
                                              map_entries,
                                              hint->lastUseForTemp);
                }

                /* fix callers */
                if (hint->callers)
                {
                    gcsCODE_CALLER_PTR	callers = hint->callers;
                    while (callers != gcvNULL)
                    {
                        callers->caller =
                            _GetNewIndex2NextCode(Shader,
                                                  index_map,
                                                  map_entries,
                                                  callers->caller);
                        callers = callers->next;
                    }
                } /* if */
            } /* if */
        } /* for */
    } while (0);

    /* Free index_map. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, index_map));

    /* Return success. */
    return gcvSTATUS_OK;
} /* gcLINKTREE_RemoveNops */

static gceSTATUS
PatchShaders(
    IN gcSHADER VertexShader,
    IN gcSHADER FragmentShader
    )
{
    gctSIZE_T i;
    gceSTATUS status = gcvSTATUS_OK;
    gctINT position = -1;
    gctBOOL hasPositionW = gcvFALSE;

    for (i = 0; i < FragmentShader->attributeCount; ++i)
    {
        /* the attribute is not in use and was freed */
        if (FragmentShader->attributes[i] == gcvNULL)
            continue;

        if (FragmentShader->attributes[i]->nameLength == gcSL_POINT_COORD)
        {
            gctSIZE_T j;

            FragmentShader->attributes[i]->isTexture = gcvTRUE;

            for (j = 0; j < VertexShader->outputCount; ++j)
            {
                if ((VertexShader->outputs[j] != gcvNULL)
                &&  (VertexShader->outputs[j]->nameLength == gcSL_POINT_COORD)
                )
                {
                    break;
                }
            }

            if (j == VertexShader->outputCount)
            {
                gcmERR_BREAK(gcSHADER_AddOutput(VertexShader,
                                                "#PointCoord",
                                                gcSHADER_FLOAT_X2,
                                                1,
                                                0));
            }

            break;
        }

        else if (FragmentShader->attributes[i]->nameLength == gcSL_POSITION)
        {
            gctSIZE_T j;

            for (j = 0; j < VertexShader->outputCount; ++j)
            {
                if ((VertexShader->outputs[j] != gcvNULL)
                &&  (VertexShader->outputs[j]->nameLength == gcSL_POSITION)
                )
                {
                    position = VertexShader->outputs[j]->tempIndex;
                }

                if ((VertexShader->outputs[j] != gcvNULL)
                &&  (VertexShader->outputs[j]->nameLength == gcSL_POSITION_W)
                )
                {
                    hasPositionW = gcvTRUE;
                    break;
                }
            }
        }
    }

    if (position != -1)
    {
        if (!hasPositionW
        &&  (gcoHAL_IsFeatureAvailable(gcvNULL,
                                       gcvFEATURE_SHADER_HAS_W)
             == gcvSTATUS_FALSE)
        )
        {
            gctINT temp = -1;

            for (i = 0; i < VertexShader->codeCount; ++i)
            {
                switch (VertexShader->code[i].opcode)
                {
                case gcSL_NOP:
                    /* fall through */
                case gcSL_JMP:
                    /* fall through */
                case gcSL_KILL:
                    /* fall through */
                case gcSL_CALL:
                    /* fall through */
                case gcSL_RET:
                    /* fall through */
                case gcSL_TEXBIAS:
                    /* fall through */
                case gcSL_TEXGRAD:
                    /* fall through */
                case gcSL_TEXLOD:
                    break;

                default:
                    if (VertexShader->code[i].tempIndex > temp)
                    {
                        temp = VertexShader->code[i].tempIndex;
                    }
                }
            }

            do
            {
                gcATTRIBUTE positionW;

                gcmERR_BREAK(gcSHADER_AddOpcode(VertexShader,
                                                gcSL_MOV,
                                                (gctUINT16) (temp + 1),
                                                0x1,
                                                gcSL_FLOAT));

                gcmERR_BREAK(gcSHADER_AddSource(VertexShader,
                                                gcSL_TEMP,
                                                (gctUINT16) position,
                                                gcSL_SWIZZLE_WWWW,
                                                gcSL_FLOAT));

                gcmERR_BREAK(gcSHADER_AddOutput(VertexShader,
                                                "#Position.w",
                                                gcSHADER_FLOAT_X1,
                                                1,
                                                (gctUINT16) (temp + 1)));

                gcmERR_BREAK(gcSHADER_Pack(VertexShader));

                gcmERR_BREAK(gcSHADER_AddAttribute(FragmentShader,
                                                   "#Position.w",
                                                   gcSHADER_FLOAT_X1,
                                                   1,
                                                   gcvFALSE,
                                                   &positionW));
            }
            while (gcvFALSE);
        }
    }



    return status;
}

gceSTATUS
gcLINKTREE_MarkAllAsUsed(
    IN gcLINKTREE Tree
    )
{
    gctSIZE_T i;

    for (i = 0; i < Tree->attributeCount; ++i)
    {
        Tree->attributeArray[i].inUse = gcvTRUE;
    }

    for (i = 0; i < Tree->tempCount; ++i)
    {
        Tree->tempArray[i].inUse = gcvTRUE;
    }

    return gcvSTATUS_OK;
}

typedef struct _gcsSL_JUMPED_FROM   gcsSL_JUMPED_FROM,
                                    * gcsSL_JUMPED_FROM_PTR;
struct _gcsSL_JUMPED_FROM
{
    gctBOOL blockBegin;
    gctBOOL reached;
};

static gceSTATUS
_OptimizeJumps(
    IN gcoOS Os,
    IN gcSHADER Shader
    )
{
    gctSIZE_T i;
    gctUINT16 label;
    gceSTATUS status;
    gcsSL_JUMPED_FROM_PTR jumps;
    gctSIZE_T bytes = Shader->codeCount* gcmSIZEOF(gcsSL_JUMPED_FROM);
    gctPOINTER pointer = gcvNULL;

     /* Check for empty shader */
    if (bytes == 0)
    {
        return gcvSTATUS_OK;
    }

    /* Allocate array of gcsSL_JUMPED_FROM structures. */
    status = gcoOS_Allocate(Os, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Return on error. */
        return status;
    }

    jumps = pointer;

    /* Initialize array as empty. */
    gcmVERIFY_OK(gcoOS_MemFill(jumps, 0, bytes));

    /* Walk through all the instructions, finding JMP instructions. */
    for (i = 0; i < Shader->codeCount; ++i)
    {
        /* Only look for JMP instructions. */
        if (Shader->code[i].opcode == gcSL_JMP)
        {
            /* Get target of jump instrution. */
            label = Shader->code[i].tempIndex;

            /* If target is next instruction, replace the jump with nop. */
            if (label == i + 1)
            {
                Shader->code[i].opcode = gcSL_NOP;
                continue;
            }

            /* Check for a JMP.ALWAYS. */
            if ((i + 1 < Shader->codeCount)
            &&  (gcmSL_TARGET_GET(Shader->code[i].temp, Condition) ==
                    gcSL_ALWAYS)
            )
            {
                /* Mark this as a beginning of a new block. */
                jumps[i + 1].blockBegin = gcvTRUE;
            }

            /* Loop while the target of the JMP is another JMP.ALWAYS. */
            while ((label < Shader->codeCount)
            &&     (Shader->code[label].opcode == gcSL_JMP)
            &&     (gcmSL_TARGET_GET(Shader->code[label].temp, Condition) ==
                       gcSL_ALWAYS)
                   /* Skip backward jump because it will screw up loop scope identification. */
            &&     (Shader->code[label].tempIndex > label)
            )
            {
                /* Load the new target for a serial JMP. */
                label = Shader->code[label].tempIndex;
            }

            /* Store the target of the final JMP. */
            Shader->code[i].tempIndex = label;

            /* Mark the target of the final JMP as reached. */
            if (label < Shader->codeCount)
            {
                jumps[label].reached = gcvTRUE;
            }
        }
    }

    /* Now walk all the gcsSL_JUMPED_FROM structures. */
    for (i = 0; i < Shader->codeCount; ++i)
    {
        /* If this is the beginning of a new block and the code has not been
        ** reached, convert the instruction into a NOP. */
        if (jumps[i].blockBegin && !jumps[i].reached)
        {
            Shader->code[i].opcode = gcSL_NOP;
        }
    }

    /* Free the array of gcsSL_JUMPED_FROM structures. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(Os, jumps));

    /* Return success. */
    return gcvSTATUS_OK;
}

/* convert the compiler flag to optimizer option:
 *  the ES1.1 driver calls gcLinkShader() without setting
 *  the loadtime optimization flag, while ES2.0 driver calls
 *  gcLinkShader() with the gcvSHADER_LOADTIME_OPTIMIZER, so
 *  we can turn off the Loadtime Optimization for ES1.1
 *
 *  TODO: set the flag from command line and environment
 *        variable
 *
 */
static gceSHADER_OPTIMIZATION
_GetOptimizerOption(
    IN gceSHADER_FLAGS Flags
    )
{
    gceSHADER_OPTIMIZATION opt = gcvOPTIMIZATION_NONE;

    if ((Flags & gcvSHADER_OPTIMIZER) !=0)
        opt = gcvOPTIMIZATION_FULL;

    /* check if need to do loadtime optimization */
    if ((gcvSHADER_LOADTIME_OPTIMIZER & Flags) == 0)
        opt &= ~gcvOPTIMIZATION_LOADTIME_CONSTANT;

    return opt;
} /* _GetOptimizerOption */

/******************************************************************************\
|********************************* Linker Code ********************************|
\******************************************************************************/

/*******************************************************************************
**                                gcLinkShaders
********************************************************************************
**
**  Link two shaders and generate a harwdare specific state buffer by compiling
**  the compiler generated code through the resource allocator and code
**  generator.
**
**  INPUT:
**
**      gcSHADER VertexShader
**          Pointer to a gcSHADER object holding information about the compiled
**          vertex shader.
**
**      gcSHADER FragmentShader
**          Pointer to a gcSHADER object holding information about the compiled
**          fragment shader.
**
**      gceSHADER_FLAGS Flags
**          Compiler flags.  Can be any of the following:
**
**              gcvSHADER_DEAD_CODE       - Dead code elimination.
**              gcvSHADER_RESOURCE_USAGE  - Resource usage optimizaion.
**              gcvSHADER_OPTIMIZER       - Full optimization.
**              gcvSHADER_USE_GL_Z        - Use OpenGL ES Z coordinate.
**              gcvSHADER_USE_GL_POSITION - Use OpenGL ES gl_Position.
**              gcvSHADER_USE_GL_FACE     - Use OpenGL ES gl_FaceForward.
**
**          gcvSHADER_LOADTIME_OPTIMZATION is set if load-time optimizaiton
**          is needed
**
**  OUTPUT:
**
**      gctSIZE_T * StateBufferSize
**          Pointer to a variable receiving the number of bytes in the buffer
**          returned in 'StateBuffer'.
**
**      gctPOINTER * StateBuffer
**          Pointer to a variable receiving a buffer pointer that contains the
**          states required to download the shaders into the hardware.
**
**      gcsHINT_PTR * Hints
**          Pointer to a variable receiving a gcsHINT structure pointer that
**          contains information required when loading the shader states.
*/
gceSTATUS
gcLinkShaders(
    IN gcSHADER VertexShader,
    IN gcSHADER FragmentShader,
    IN gceSHADER_FLAGS Flags,
    OUT gctSIZE_T * StateBufferSize,
    OUT gctPOINTER * StateBuffer,
    OUT gcsHINT_PTR * Hints
    )
{
    gcoOS os;
    gceSTATUS status;
#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
    gctUINT32 prevDebugLevel;
    gctUINT32 prevDebugZone;
#endif
    gcLINKTREE vertexTree = gcvNULL;
    gcLINKTREE fragmentTree = gcvNULL;

    gcmHEADER_ARG("VertexShader=0x%x FragmentShader=0x%x Flags=%x",
                  VertexShader, FragmentShader, Flags);

    /* Verify the arguments. */
	if (VertexShader)
	{
		gcmVERIFY_OBJECT(VertexShader, gcvOBJ_SHADER);
	}
	if (FragmentShader)
	{
		gcmVERIFY_OBJECT(FragmentShader, gcvOBJ_SHADER);
	}

    /* Extract the gcoOS object pointer. */
    os = gcvNULL;

#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
    /* Set debug level to gcvLEVEL_INFO or higher to dump shader machine code. */
    gcoOS_GetDebugLevel(&prevDebugLevel);
	gcoOS_SetDebugLevel(gcvLEVEL_WARNING);
    gcoOS_GetDebugZone(gcvZONE_COMPILER, &prevDebugZone);
    gcoOS_SetDebugZone(gcvZONE_COMPILER);
#endif

	do
    {
#if GC_ENABLE_LOADTIME_OPT
         /* vertex shader full optimization */
         if (VertexShader)
         {
            gcmERR_BREAK(gcSHADER_SetOptimizationOption(VertexShader, _GetOptimizerOption(Flags)));
            gcmERR_BREAK(gcOptimizeShader(VertexShader, gcvNULL));
         }

         /* fragment shader full optimization */
         if (FragmentShader)
         {
            gcmERR_BREAK(gcSHADER_SetOptimizationOption(FragmentShader, _GetOptimizerOption(Flags)));
            gcmERR_BREAK(gcOptimizeShader(FragmentShader, gcvNULL));
         }
#endif /* GC_ENABLE_LOADTIME_OPT */

        /* Remove some flags depending on the IP. */
        if (Flags & gcvSHADER_USE_GL_Z)
        {
            gceCHIPMODEL model;

            gcmERR_BREAK(
                gcoHAL_QueryChipIdentity(gcvNULL,
                                         &model, gcvNULL, gcvNULL, gcvNULL));

            if (model >= gcv1000 || model == gcv880)
            {
                Flags &= ~gcvSHADER_USE_GL_Z;
            }
        }

        /* Special case for Fragment Shader using PointCoord. */
		if (VertexShader && FragmentShader)
		{
			gcmERR_BREAK(PatchShaders(VertexShader, FragmentShader));
		}

        if (Flags & gcvSHADER_OPTIMIZER)
        {
			if (VertexShader)
			{
				/* Optimize the jumps in the vertex shader. */
				gcmERR_BREAK(_OptimizeJumps(os, VertexShader));

				/* Compact the vertex shader. */
				gcmERR_BREAK(CompactShader(os, VertexShader));
			}

			if (FragmentShader)
			{
				/* Optimize the jumps in the fragment shader. */
				gcmERR_BREAK(_OptimizeJumps(os, FragmentShader));

				/* Compact the fragment shader. */
				gcmERR_BREAK(CompactShader(os, FragmentShader));
			}
        }

        /* Construct the vertex shader tree. */
        gcmERR_BREAK(gcLINKTREE_Construct(os, &vertexTree));

		if (VertexShader) {
			/* Build the vertex shader tree. */
			gcmERR_BREAK(gcLINKTREE_Build(vertexTree, VertexShader, Flags));
			DUMP("Incoming vertex shader", vertexTree);
		}

        /* Construct the fragment shader tree. */
        gcmERR_BREAK(gcLINKTREE_Construct(os, &fragmentTree));

		if (FragmentShader)
		{
			/* Build the fragment shader tree. */
			gcmERR_BREAK(gcLINKTREE_Build(fragmentTree, FragmentShader, Flags));
			DUMP("Incoming fragment shader", fragmentTree);

			/* Remove dead code from the fragment shader. */
			if (Flags & gcvSHADER_DEAD_CODE)
			{
				gcmERR_BREAK(gcLINKTREE_RemoveDeadCode(fragmentTree));
				DUMP("Removed dead code from the fragment shader", fragmentTree);
			}
			else
			{
				/* Mark all temps and attributes as used. */
				gcmERR_BREAK(gcLINKTREE_MarkAllAsUsed(fragmentTree));
			}
		}

		if (VertexShader && FragmentShader)
		{
			/* Link vertex and fragment shaders. */
			gcmERR_BREAK(gcLINKTREE_Link(vertexTree, fragmentTree));
			DUMP("Linked vertex shader and fragment shader", vertexTree);
		}

        /* Only continue if we need to do code generation. */
        if (StateBufferSize != gcvNULL)
        {
			if (VertexShader)
			{
				if (Flags & gcvSHADER_DEAD_CODE)
				{
					/* Remove dead code from the vertex shader. */
					gcmERR_BREAK(gcLINKTREE_RemoveDeadCode(vertexTree));
					DUMP("Removed dead code from the vertex shader", vertexTree);
				}
				else
				{
					/* Mark all temps and attributes as used. */
					gcmERR_BREAK(gcLINKTREE_MarkAllAsUsed(vertexTree));
				}
			}

			if (Flags & gcvSHADER_OPTIMIZER)
			{
				if (VertexShader)
				{
					/* Optimize the vertex shader by removing MOV instructions. */
					gcmERR_BREAK(gcLINKTREE_Optimize(vertexTree));
					DUMP("Optimized the vertex shader", vertexTree);

                    /* Remove NOPs in the vertex shader. */
                    gcmERR_BREAK(gcLINKTREE_RemoveNops(vertexTree));
                    DUMP("Remove NOPs in the vertex shader.", vertexTree);
				}

				if (FragmentShader)
				{
					/* Optimize the fragment shader by removing MOV instructions. */
					gcmERR_BREAK(gcLINKTREE_Optimize(fragmentTree));
					DUMP("Optimized the fragment shader", fragmentTree);

					/* Clean up the fragment shader. */
					gcmERR_BREAK(gcLINKTREE_Cleanup(fragmentTree));
					DUMP("Cleaned up the fragment tree.", fragmentTree);

                    /* Remove NOPs in the fragment shader. */
                    gcmERR_BREAK(gcLINKTREE_RemoveNops(fragmentTree));
                    DUMP("Remove NOPs in the fragment shader.", fragmentTree);
				}
			}

			/* Generate vertex shader states. */
			if (VertexShader)
			{
				gcmERR_BREAK(gcLINKTREE_GenerateStates(vertexTree,
													   Flags,
													   StateBufferSize,
													   StateBuffer,
													   Hints));
			}

            /* Generate fragment shader states. */
			if (FragmentShader)
			{
				gcmERR_BREAK(gcLINKTREE_GenerateStates(fragmentTree,
													   Flags,
													   StateBufferSize,
													   StateBuffer,
													   Hints));
			}
        }
    }
    while (gcvFALSE);

#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
	gcoOS_SetDebugLevel(prevDebugLevel);
	gcoOS_SetDebugZone(prevDebugZone);
#endif

	if (vertexTree != gcvNULL)
	{
		/* Destroy the vertex shader tree. */
		gcmVERIFY_OK(gcLINKTREE_Destroy(vertexTree));
	}

	if (fragmentTree != gcvNULL)
	{
		/* Destroy the fragment shader tree. */
		gcmVERIFY_OK(gcLINKTREE_Destroy(fragmentTree));
	}

	/* Return the status. */
	if (gcmIS_SUCCESS(status) && (StateBufferSize != gcvNULL))
	{
		gcmFOOTER_ARG("*StateBufferSize=%lu *StateBuffer=0x%x *Hints=0x%x",
					  *StateBufferSize, *StateBuffer, *Hints);
	}
	else
	{
		gcmFOOTER();
	}
	return status;
}

/*******************************************************************************
**                                gcLinkKernel
********************************************************************************
**
**	Link OpenCL kernel and generate a harwdare specific state buffer by compiling
**	the compiler generated code through the resource allocator and code
**	generator.
**
**	INPUT:
**
**		gcSHADER Kernel
**			Pointer to a gcSHADER object holding information about the compiled
**			OpenCL kernel.
**
**		gceSHADER_FLAGS Flags
**			Compiler flags.  Can be any of the following:
**
**				gcvSHADER_DEAD_CODE       - Dead code elimination.
**				gcvSHADER_RESOURCE_USAGE  - Resource usage optimizaion.
**				gcvSHADER_OPTIMIZER       - Full optimization.
**
**          gcvSHADER_LOADTIME_OPTIMZATION is set if load-time optimizaiton
**          is needed
**
**	OUTPUT:
**
**		gctSIZE_T * StateBufferSize
**			Pointer to a variable receiving the number of bytes in the buffer
**			returned in 'StateBuffer'.
**
**		gctPOINTER * StateBuffer
**			Pointer to a variable receiving a buffer pointer that contains the
**			states required to download the shaders into the hardware.
**
**		gcsHINT_PTR * Hints
**			Pointer to a variable receiving a gcsHINT structure pointer that
**			contains information required when loading the shader states.
*/
gceSTATUS
gcLinkKernel(
	IN gcSHADER Kernel,
	IN gceSHADER_FLAGS Flags,
	OUT gctSIZE_T * StateBufferSize,
	OUT gctPOINTER * StateBuffer,
	OUT gcsHINT_PTR * Hints
	)
{
	gcoOS os;
	gceSTATUS status;
#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
    gctUINT32 prevDebugLevel;
    gctUINT32 prevDebugZone;
#endif
	gcLINKTREE kernelTree = gcvNULL;

	gcmHEADER_ARG("Kernel=0x%x Flags=%x",
				  Kernel, Flags);

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Kernel, gcvOBJ_SHADER);

	/* Extract the gcoOS object pointer. */
	/*os = Kernel->hal->os;*/
    os = gcvNULL;

    if (gcSHADER_CheckBugFixes10())
    {
        /* Full optimization. */
        gcmVERIFY_OK(gcSHADER_SetOptimizationOption(Kernel, _GetOptimizerOption(Flags)));
    }
    else
    {
        /* Special LOAD optimization for LOAD SW workaround. */
        gcmVERIFY_OK(gcSHADER_SetOptimizationOption(Kernel,
            _GetOptimizerOption(Flags) | gcvOPTIMIZATION_LOAD_SW_WORKAROUND));
    }

    /* Optimize kernel shader. */
    status = gcOptimizeShader(Kernel, gcvNULL);
    if(gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
	/* Set this to gcvLEVEL_INFO or higher to dump shader machine code. */
    gcoOS_GetDebugLevel(&prevDebugLevel);
    gcoOS_SetDebugLevel(gcvLEVEL_WARNING /* gcvLEVEL_INFO */);
    gcoOS_GetDebugZone(gcvZONE_COMPILER, &prevDebugZone);
	gcoOS_SetDebugZone(gcvZONE_COMPILER);
#endif

	do
	{
		if (Flags & gcvSHADER_OPTIMIZER)
		{
			/* Optimize the jumps in the kernel shader. */
			gcmERR_BREAK(_OptimizeJumps(os, Kernel));

			/* Compact the kernel shader. */
			gcmERR_BREAK(CompactShader(os, Kernel));
		}

		/* Construct the kernel shader tree. */
		gcmERR_BREAK(gcLINKTREE_Construct(os, &kernelTree));

		/* Build the kernel shader tree. */
		gcmERR_BREAK(gcLINKTREE_Build(kernelTree, Kernel, Flags));
		DUMP("Incoming kernel shader", kernelTree);
		/*_FindPositionAttribute(kernelTree);*/

		/* Only continue if we need to do code generation. */
		if (StateBufferSize != gcvNULL)
		{
			if (Flags & gcvSHADER_DEAD_CODE)
			{
				/* Remove dead code from the kernel shader. */
				gcmERR_BREAK(gcLINKTREE_RemoveDeadCode(kernelTree));
				DUMP("Removed dead code from the kernel shader", kernelTree);
			}
			else
			{
				/* Mark all temps and attributes as used. */
				gcmERR_BREAK(gcLINKTREE_MarkAllAsUsed(kernelTree));
			}

			if (Flags & gcvSHADER_OPTIMIZER)
			{
				/* Optimize the kernel shader by removing MOV instructions. */
				gcmERR_BREAK(gcLINKTREE_Optimize(kernelTree));
				DUMP("Optimized the kernel shader", kernelTree);

				/* Clean up the kernel shader. */
				gcmERR_BREAK(gcLINKTREE_Cleanup(kernelTree));
				DUMP("Cleaned up the kernel tree.", kernelTree);
			}

			/* Generate kernel shader states. */
			gcmERR_BREAK(gcLINKTREE_GenerateStates(kernelTree,
												   Flags,
												   StateBufferSize,
												   StateBuffer,
												   Hints));
		}
	}
	while (gcvFALSE);

#if gcmIS_DEBUG(gcdDEBUG_OPTIMIZER)
	gcoOS_SetDebugLevel(prevDebugLevel);
	gcoOS_SetDebugZone(prevDebugZone);
#endif

	if (kernelTree != gcvNULL)
	{
		/* Destroy the kernel shader tree. */
		gcmVERIFY_OK(gcLINKTREE_Destroy(kernelTree));
	}

	/* Return the status. */
	if (gcmIS_SUCCESS(status))
	{
		gcmFOOTER_ARG("*StateBufferSize=%lu *StateBuffer=0x%x *Hints=0x%x",
					  *StateBufferSize, *StateBuffer, *Hints);
	}
	else
	{
		gcmFOOTER();
	}
	return status;
}

/** Program binary file header format: -
       Word 1:  gcmCC('P', 'R', 'G', 'M') Program binary file signature
       Word 2:  ('\od' '\od' '\od' '\od') od = octal digits; program binary file version
       Word 3:  ('\E' '\S' '\0' '\0') or Language type
                ('C' 'L' '\0' '\0')
       Word 4: ('\0' '\0' '\10' '\0') chip model  e.g. gc800
       Wor5: ('\1' '\3' '\6' '\4') chip version e.g. 4.6.3_rc1
       Word 6: size of program binary file in bytes excluding this header */
#define _gcdProgramBinaryHeaderSize   (6 * 4)  /*Program binary file header size in bytes*/

/*******************************************************************************
**  _gcLoadProgramHeader
**
**  Load a shader proggram from a binary buffer.  The binary buffer is layed out
**  as follows:
**      // Six word header
**      // Signature, must be 'S','H','D','R'.
**      gctINT8             signature[4];
**      gctUINT32           prgBinFileVersion;
**      gctUINT32           laguage;
**      gctUINT32           chipModel;
**      gctUINT32           chipVersion;
**      gctUINT32           binarySize;
**
**  INPUT:
**
**      gctPOINTER Buffer
**          Pointer to a program binary buffer containing the program data to load.
**
**      gctSIZE_T BufferSize
**          Number of bytes inside the binary buffer pointed to by 'Buffer'.
**
**  OUTPUT:
**      gctUINT32 *Language
**          the third word in the header containing Language type
**
*/
static gceSTATUS
_gcLoadProgramHeader(
    IN gctPOINTER Buffer,
    IN gctSIZE_T BufferSize,
    OUT gctPOINTER Language
    )
{
    gctUINT32 * signature;
    gctSIZE_T bytes;
    gctUINT32 *version;
    gctUINT32 *language;
    gctUINT32 *chipModel;
    gctUINT32 *chipVersion;
    gctUINT32 *size;
    gctUINT8 *bytePtr;

    gcmHEADER_ARG("Buffer=0x%x BufferSize=%lu",
                  Buffer, BufferSize);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BufferSize > 0);

    /* Load the Header */
    bytes = BufferSize;
    if(bytes < _gcdProgramBinaryHeaderSize) {
        /* Invalid file format */
        gcmFATAL("_gcLoadProgramHeader: Invalid program binary file format");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Verify the signature. */
    signature = (gctUINT32 *) Buffer;
    if (*signature != gcmCC('P', 'R', 'G', 'M')) {
        /* Signature mismatch. */
        gcmFATAL("_gcLoadProgramHeader: Signature does not match with 'PRGM'");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Word 2: program binary file version # */
    version = (gctUINT32 *) (signature + 1);
    if(*version > gcdSL_PROGRAM_BINARY_FILE_VERSION) {
#if gcdDEBUG
        gctUINT8 *inputVerPtr, *curVerPtr;
        gctUINT32 curVer[1] = {gcdSL_PROGRAM_BINARY_FILE_VERSION};

        inputVerPtr = (gctUINT8 *) version;
        curVerPtr = (gctUINT8 *) curVer;
        gcmFATAL("_gcLoadProgramHeader: program binary file's version of %u.%u.%u:%u "
                 "is newer than current version %u.%u.%u:%u",
                 inputVerPtr[0], inputVerPtr[1], inputVerPtr[2], inputVerPtr[3],
                 curVerPtr[0], curVerPtr[1], curVerPtr[2], curVerPtr[3]);
#endif
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Word 3: language type */
    language = (gctUINT32 *) (version + 1);
    bytePtr = (gctUINT8 *) language;

    if((bytePtr[0] == 'E' && bytePtr[1] != 'S') ||
       (bytePtr[0] == 'C' && bytePtr[1] != 'L') ||
       (bytePtr[0] != 'E' && bytePtr[0] != 'C')) {
       gcmFATAL("_gcLoadProgramHeader: unrecognizable laguage type \"%c%c\"", bytePtr[0], bytePtr[1]);
       gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
       return gcvSTATUS_INVALID_DATA;
    }

    /* Word 4: chip model */
    chipModel = (gctUINT32 *) (language + 1);
/* check chip model here */

    /* Word 5: chip version */
    chipVersion  = (gctUINT32 *) (chipModel + 1);
/* check chip model here */

    /* Word 6: size of program binary excluding header */
    bytes -= _gcdProgramBinaryHeaderSize;
    size = (gctUINT32 *) (chipVersion + 1);
    if(bytes != *size) {
        /* program binary size mismatch. */
        gcmFATAL("_gcLoadProgramHeader: program binary size %u does not match actual file size %u",
                 bytes, *size);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }
    /* Success. */
    *(gctUINT32 *)Language = *language;
    gcmFOOTER_ARG("*Language=%u", *language);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**                                gcSaveProgram
********************************************************************************
**
**  Save pre-compiled shaders and pre-linked programs to a binary file.
**
**  INPUT:
**
**      gcSHADER VertexShader
**          Pointer to vertex shader object.
**
**      gcSHADER FragmentShader
**          Pointer to fragment shader object.
**
**      gctSIZE_T ProgramBufferSize
**          Number of bytes in 'ProgramBuffer'.
**
**      gctPOINTER ProgramBuffer
**          Pointer to buffer containing the program states.
**
**      gcsHINT_PTR Hints
**          Pointer to HINTS structure for program states.
**
**  OUTPUT:
**
**      gctPOINTER * Binary
**          Pointer to a variable receiving the binary data to be saved.
**
**      gctSIZE_T * BinarySize
**          Pointer to a variable receiving the number of bytes inside 'Binary'.
*/
gceSTATUS
gcSaveProgram(
    IN gcSHADER VertexShader,
    IN gcSHADER FragmentShader,
    IN gctSIZE_T ProgramBufferSize,
    IN gctPOINTER ProgramBuffer,
    IN gcsHINT_PTR Hints,
    OUT gctPOINTER * Binary,
    OUT gctSIZE_T * BinarySize
    )
{
    gctSIZE_T vertexShaderBytes;
    gctSIZE_T fragmentShaderBytes;
    gctSIZE_T bytes;
    gctUINT8_PTR bytePtr;
    gctSIZE_T bufferSize;
    gctUINT8_PTR buffer = gcvNULL;
    gceSTATUS status;

    gcmHEADER_ARG("VertexShader=0x%x FragmentShader=0x%x ProgramBufferSize=%lu "
                  "ProgramBuffer=0x%x Hints=0x%x",
                  VertexShader, FragmentShader, ProgramBufferSize,
                  ProgramBuffer, Hints);

    do
    {
        /* Get size of vertex shader bibary. */
        gcmERR_BREAK(gcSHADER_Save(VertexShader,
                                   gcvNULL,
                                   &vertexShaderBytes));

        /* Get size of fragment shader binary. */
        gcmERR_BREAK(gcSHADER_Save(FragmentShader,
                                   gcvNULL,
                                   &fragmentShaderBytes));

        /* Compute number of bytes required for binary buffer. */
        bufferSize = _gcdProgramBinaryHeaderSize
                     + gcmSIZEOF(gctSIZE_T) + gcmALIGN(vertexShaderBytes, 4)
                     + gcmSIZEOF(gctSIZE_T) + gcmALIGN(fragmentShaderBytes, 4)
                     + gcmSIZEOF(gctSIZE_T) + ProgramBufferSize
                     + gcmSIZEOF(gctSIZE_T) + gcSHADER_GetHintSize();

        if (BinarySize)
        {
            *BinarySize = bufferSize;
        }

        if (Binary == gcvNULL)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

	if(*Binary) { /* a buffer has been passed here */
	   /* Make sure the buffer is large enough. */
	   if(BinarySize) {
	      if (*BinarySize < bufferSize) {
                 *BinarySize = bufferSize;
                 gcmFOOTER_ARG("*BinarySize=%d status=%d",
                               *BinarySize, gcvSTATUS_BUFFER_TOO_SMALL);
                 return gcvSTATUS_BUFFER_TOO_SMALL;
	      }
	   }
	}
	else { /* Allocate binary buffer. */
           gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                       bufferSize,
                                       Binary));
	}

        /* Cast binary buffer to UINT8 pointer for easy byte copying. */
        buffer      = (gctUINT8_PTR) *Binary;

	/* Word 1: program binary signature */
        buffer[0] = 'P';
        buffer[1] = 'R';
        buffer[2] = 'G';
        buffer[3] = 'M';
        buffer += 4;

	/* Word 2: binary file version # */
	*(gctUINT32 *) buffer = gcdSL_PROGRAM_BINARY_FILE_VERSION;
	buffer += sizeof(gctUINT32);

        /* Word 3: language type */
	*(gctUINT32 *) buffer = VertexShader->compilerVersion[0];
	buffer += sizeof(gctUINT32);

	/* Word 4: chip model */
	*(gctUINT32 *) buffer = gcmCC('\0', '\0', '\0', '\0');
	buffer += sizeof(gctUINT32);

	/* Word 5: chip version */
	*(gctUINT32 *) buffer = gcmCC('\1', gcvVERSION_PATCH, gcvVERSION_MINOR, gcvVERSION_MAJOR);
	buffer += sizeof(gctUINT32);

	/* Word 6: size of binary excluding header */
	*(gctUINT32 *) buffer = bufferSize - _gcdProgramBinaryHeaderSize;
	buffer += sizeof(gctUINT32);

        /* Copy the size of the vertex shader binary. */
        gcmERR_BREAK(gcoOS_MemCopy(buffer,
                                   &vertexShaderBytes,
                                   gcmSIZEOF(gctSIZE_T)));
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Save the vertex shader binary. */
        gcmERR_BREAK(gcSHADER_Save(VertexShader,
                                   buffer,
                                   &vertexShaderBytes));
	bytePtr = buffer + vertexShaderBytes;
        buffer += gcmALIGN(vertexShaderBytes, 4);
	while(bytePtr < buffer) { /* clear the alignment bytes to help file comparison when needed*/
	  *bytePtr++ = '\0';
	}

        /* Copy the size of the fragment shader binary. */
        gcmERR_BREAK(gcoOS_MemCopy(buffer,
                                   &fragmentShaderBytes,
                                   gcmSIZEOF(gctSIZE_T)));
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Save the fragment shader binary. */
        gcmERR_BREAK(gcSHADER_Save(FragmentShader,
                                   buffer,
                                   &fragmentShaderBytes));
	bytePtr = buffer + fragmentShaderBytes;
        buffer += gcmALIGN(fragmentShaderBytes, 4);
	while(bytePtr < buffer) { /* clear the alignment bytes to help file comparison when needed*/
	  *bytePtr++ = '\0';
	}

        /* Copy the size of the state buffer. */
        gcmERR_BREAK(gcoOS_MemCopy(buffer,
                                   &ProgramBufferSize,
                                   gcmSIZEOF(gctSIZE_T)));
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Copy the state buffer. */
        gcmERR_BREAK(gcoOS_MemCopy(buffer,
                                   ProgramBuffer,
                                   ProgramBufferSize));
        buffer += ProgramBufferSize;

        /* Copy the size of the HINT structure. */
        bytes = gcSHADER_GetHintSize();
        gcmERR_BREAK(gcoOS_MemCopy(buffer,
                                   &bytes,
                                   gcmSIZEOF(gctSIZE_T)));
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Copy the HINT structure. */
        gcmERR_BREAK(gcoOS_MemCopy(buffer, Hints, bytes));
        buffer += bytes;

        /* Assert we copied the right number of bytes */
        gcmASSERT(buffer - *BinarySize == *Binary);

        /* Success. */
        gcmFOOTER_ARG("*Binary=0x%x *BinarySize=%ul", *Binary, *BinarySize);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (gcmIS_ERROR(status) && (buffer != gcvNULL))
    {
        /* Free binary buffer on error. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, *Binary));

        /* Reset pointers on error. */
        *Binary     = gcvNULL;
        if (BinarySize)
        {
            *BinarySize = 0;
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                                gcLoadProgram
********************************************************************************
**
**	Load pre-compiled shaders and pre-linked programs from a binary file.
**
**	INPUT:
**
**		gctPOINTER Binary
**			Pointer to the binary data loaded.
**
**		gctSIZE_T BinarySize
**			Number of bytes in 'Binary'.
**
**		gcSHADER VertexShader
**			Pointer to a vertex shader object.
**
**		gcSHADER FragmentShader
**			Pointer to a fragment shader object.
**
**	OUTPUT:
**
**		gctSIZE_T * ProgramBufferSize
**			Pointer to a variable receiving the number of bytes in the buffer
**			returned in 'ProgramBuffer'.
**
**		gctPOINTER * ProgramBuffer
**			Pointer to a variable receiving a buffer pointer that contains the
**			states required to download the shaders into the hardware.
**			if ProgramBuffer is NULL, no states will be loaded.
**
**		gcsHINT_PTR * Hints
**			Pointer to a variable receiving a gcsHINT structure pointer that
**			contains information required when loading the shader states.
**			if Hints is NULL, no hints will be loaded.
*/
gceSTATUS
gcLoadProgram(
	IN gctPOINTER Binary,
	IN gctSIZE_T BinarySize,
	IN gcSHADER VertexShader,
	IN gcSHADER FragmentShader,
	OUT gctSIZE_T * ProgramBufferSize,
	OUT gctPOINTER * ProgramBuffer,
	OUT gcsHINT_PTR * Hints
	)
{
    gcoOS os;
    gctSIZE_T bytes;
    gctUINT8_PTR buffer;
    gctSIZE_T *bufferSize;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT8 language[4];
    gctPOINTER pointer;

    gcmHEADER_ARG("Binary=0x%x BinarySize=%lu",
                  Binary, BinarySize);

    do {
	if(ProgramBufferSize) {
	  *ProgramBufferSize = 0;
	}
	if(ProgramBuffer) {
	  if(ProgramBufferSize == gcvNULL)
	  {
	    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
	    return gcvSTATUS_INVALID_DATA;
	  }
	  *ProgramBuffer = gcvNULL;
	}
	if(Hints) {
	  *Hints = gcvNULL;
	}

	/* Extract the gcoOS object pointer. */
	os = gcvNULL;

        gcmERR_BREAK(_gcLoadProgramHeader(Binary, BinarySize, (gctPOINTER)language));

	if(language[0] != 'E' || language[1] != 'S') {
           gcmFATAL("gcLoadProgram: expect language type 'ES' instead of %c%c",
                    language[0], language[1]);
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
	}

	buffer = (gctUINT8 *)Binary + _gcdProgramBinaryHeaderSize;
        bytes = BinarySize - _gcdProgramBinaryHeaderSize;

	bufferSize = (gctSIZE_T *) buffer;

	if (bytes < sizeof(gctSIZE_T) || bytes < (*bufferSize + sizeof(gctSIZE_T))) {
           /* Invalid vertex shader size. */
           gcmFATAL("gcLoadProgram: Invalid vertex shader size %u", bytes);
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
	}

	bytes -=  sizeof(gctSIZE_T);
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Load vertex shader bibary. */
        gcmERR_BREAK(gcSHADER_Load(VertexShader,
                                   buffer,
                                   *bufferSize));

	*bufferSize = gcmALIGN(*bufferSize, 4);
	buffer += *bufferSize;
	bytes -=  *bufferSize;
	bufferSize = (gctSIZE_T *) buffer;

	if (bytes < sizeof(gctSIZE_T) || bytes < (*bufferSize + sizeof(gctSIZE_T))) {
           /* Invalid fragment shader size. */
           gcmFATAL("gcLoadProgram: Invalid fragment shader size %u", bytes);
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
	}

	bytes -=  sizeof(gctSIZE_T);
        buffer += gcmSIZEOF(gctSIZE_T);

        /* Load fragment shader binary. */
        gcmERR_BREAK(gcSHADER_Load(FragmentShader,
                                   buffer,
                                   *bufferSize));

	*bufferSize = gcmALIGN(*bufferSize, 4);
	buffer += *bufferSize;
	bytes -=  *bufferSize;
	bufferSize = (gctSIZE_T *) buffer;

	if (bytes < sizeof(gctSIZE_T) || bytes < (*bufferSize + sizeof(gctSIZE_T))) {
           /* Invalid program states size. */
           gcmFATAL("gcLoadProgram: Invalid program states size %u", bytes);
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
	}

	bytes -=  gcmSIZEOF(gctSIZE_T);
	buffer += gcmSIZEOF(gctSIZE_T);

	if(ProgramBuffer) {
          /* Allocate program states buffer. */
          gcmERR_BREAK(gcoOS_Allocate(os,
                                      *bufferSize,
                                      &pointer));
	  *ProgramBuffer = pointer;
          *ProgramBufferSize = *bufferSize;

          gcmERR_BREAK(gcoOS_MemCopy(*ProgramBuffer,
                                     buffer,
                                     *bufferSize));
	}

        buffer += *bufferSize;
	bytes -=  *bufferSize;
	bufferSize = (gctSIZE_T *) buffer;

	if (bytes < sizeof(gctSIZE_T) ||
	    bytes < (*bufferSize + sizeof(gctSIZE_T)) ||
            (Hints && *bufferSize != gcSHADER_GetHintSize()) ) {
           /* Invalid hint size. */
           gcmFATAL("gcLoadProgram: Invalid hints size %u", bytes);
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
	}

	buffer += gcmSIZEOF(gctSIZE_T);

	if(Hints) {
          /* Allocate hint structure buffer. */
          gcmERR_BREAK(gcoOS_Allocate(os,
				      gcSHADER_GetHintSize(),
                                      &pointer));
	  *Hints = pointer;
          /* Copy the HINT structure. */
          gcmERR_BREAK(gcoOS_MemCopy(*Hints,
                                     buffer,
                                     *bufferSize));
	}
        buffer += *bufferSize;

        /* Assert we copied the right number of bytes */
        gcmASSERT(buffer - BinarySize == Binary);

        /* Success. */
        gcmFOOTER_ARG("*ProgramBufferSize=%lu *ProgramBuffer=0x%x *Hints=0x%x",
                      ProgramBufferSize ? *ProgramBufferSize : 0,
		      ProgramBuffer ? *ProgramBuffer : gcvNULL,
		      Hints ? *Hints : gcvNULL);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);
    /* Return the status. */
    gcmFOOTER_ARG("*ProgramBufferSize=%lu *ProgramBuffer=0x%x *Hints=0x%x",
                   ProgramBufferSize ? *ProgramBufferSize : 0,
		   ProgramBuffer ? *ProgramBuffer : gcvNULL,
		   Hints ? *Hints : gcvNULL);
    return status;
}
#endif /*VIVANTE_NO_3D*/

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


#define  WINVER			0x0600
#define _WIN32_WINNT	0x0600
#define _WIN32_WINDOWS	0x0410
#define _WIN32_IE		0x0700

#include <stdio.h>
#include <tchar.h>
#include <afx.h>
#include <math.h>
#include "Generator.h"
#include "Formats.h"

typedef gctFLOAT (* vgtMATHFUNCTION) (
	gctFLOAT Color
	);

gctFLOAT vgfGetColor(
	gctFLOAT Color
	)
{
	return Color;
}

static void _GenerateReadConverter(
	CStdioFile& File,
	vgtMATHFUNCTION Function,
	gctSTRING Name,
	gctUINT Width
	)
{
	gctUINT count    = 1 << Width;
	gctUINT maxValue = count - 1;

	vgfWriteLine(File, "static gctUINT32 _%s_%dbit[%d] =\n", Name, Width, count);
	vgfWriteLine(File, "{\n");

	for (gctUINT i = 0; i <= maxValue; i++)
	{
		gctFLOAT color = ((gctFLOAT) i) / maxValue;

		gctFLOAT res = Function(color);

		if (i > 0)
		{
			vgfWriteLine(File, ",");
		}

		if (((i % 4) == 0) && (i > 0))
		{
			vgfWriteLine(File, "\n");
		}

		if ((i % 4) == 0)
		{
			vgfWriteLine(File, "\t");
		}
		else if (i > 0)
		{
			vgfWriteLine(File, " ");
		}

		vgfWriteLine(File, "0x%08X", *(DWORD*) &res);
	}

	vgfWriteLine(File, "\n};\n");
}

void GenerateChannelMaskLookup(
	CStdioFile& File
	)
{
	vgfWriteLine(File, "/*******************************************************************************\n");
	vgfWriteLine(File, "** Format-dependent channel mask lookup tables.\n");
	vgfWriteLine(File, "*/\n");
	vgfWriteLine(File, "\n");

	gctBOOL firstArray = gcvTRUE;
	vgsFORMAT_LIST_PTR formatGroups;
	gctUINT groupCount;

	/* Get format groups. */
	vgfGetFormatGroupList(&formatGroups, &groupCount);

	/* Go through all swizzle groups. */
	for (gctUINT currentGroup = 0; currentGroup < groupCount; currentGroup += 1)
	{
		gctUINT formatCount = formatGroups[currentGroup].count;

		/* Go through all formats in the swizzle group. */
		for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
		{
			/* Get the current format. */
			vgsFORMAT_INFO_PTR format = &formatGroups[currentGroup].group[currentFormat];

			/* Valid format? */
			if (format->name == gcvNULL)
			{
				continue;
			}

			/* RGBx? */
			gctBOOL rgbx
				=  (format->r.width != 0)
				&& (format->g.width != 0)
				&& (format->b.width != 0);

			/* Skip if not. */
			if (!rgbx)
			{
				continue;
			}

			if (!firstArray)
			{
				vgfWriteLine(File, "\n");
			}

			firstArray = gcvFALSE;

			vgfWriteLine(File, "static gctUINT%d _%s_channelMask[16] =\n",
				format->bitsPerPixel, format->name
				);

			vgfWriteLine(File, "{\n");

			/* Go through all masks. */
			for (gctUINT currentMask = 0; currentMask < gcmCOUNTOF(maskArray); currentMask += 1)
			{
				gctUINT32 mask = 0;

				gctBOOL lastMask = (currentMask == (gcmCOUNTOF(maskArray) - 1));

				gctUINT32 pixelMask = (format->bitsPerPixel == 32)
					? 0xFFFFFFFF
					: 0x0000FFFF;

				if ((currentMask & VG_ALPHA) != 0)
				{
					if (format->a.width != 0)
					{
						mask |= format->a.mask << format->a.start;
					}
				}

				if ((currentMask & VG_RED) != 0)
				{
					mask |= format->r.mask << format->r.start;
				}

				if ((currentMask & VG_GREEN) != 0)
				{
					mask |= format->g.mask << format->g.start;
				}

				if ((currentMask & VG_BLUE) != 0)
				{
					mask |= format->b.mask << format->b.start;
				}

				vgfWriteLine(\
					File, "\t/* %s */\t0x%0*X%s\n",
					maskArray[currentMask],
					format->bitsPerPixel / 4,
					~mask & pixelMask,
					lastMask ? "" : ","
					);
			};

			vgfWriteLine(File, "};\n");
		}
	}
}

void GenerateTableZero2One(
	CStdioFile& File
	)
{
	vgfWriteLine(File, "/*******************************************************************************\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** Color conversion look-up tables:\n");
	vgfWriteLine(File, "**   1. Convert to [0..1] range.\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** The tables are in 32-bit floating point binary format.\n");
	vgfWriteLine(File, "*/\n");
	vgfWriteLine(File, "\n");

	_GenerateReadConverter(File, vgfGetColor, "zero2one", 1);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColor, "zero2one", 4);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColor, "zero2one", 5);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColor, "zero2one", 6);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColor, "zero2one", 8);
}

void GenerateTableZero2One_S2L(
	CStdioFile& File
	)
{
	vgfWriteLine(File, "/*******************************************************************************\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** Color conversion look-up tables:\n");
	vgfWriteLine(File, "**   1. Convert to [0..1] range.\n");
	vgfWriteLine(File, "**   2. Convert non-linear to linear color space.\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** The tables are in 32-bit floating point binary format.\n");
	vgfWriteLine(File, "*/\n");
	vgfWriteLine(File, "\n");


	_GenerateReadConverter(File, vgfGetColorInverseGamma, "zero2one_s2l", 4);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColorInverseGamma, "zero2one_s2l", 5);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColorInverseGamma, "zero2one_s2l", 6);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColorInverseGamma, "zero2one_s2l", 8);
}

void GenerateTableZero2One_L2S(
	CStdioFile& File
	)
{
	vgfWriteLine(File, "/*******************************************************************************\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** Color conversion look-up tables:\n");
	vgfWriteLine(File, "**   1. Convert to [0..1] range.\n");
	vgfWriteLine(File, "**   2. Convert linear to non-linear color space.\n");
	vgfWriteLine(File, "**\n");
	vgfWriteLine(File, "** The tables are in 32-bit floating point binary format.\n");
	vgfWriteLine(File, "*/\n");
	vgfWriteLine(File, "\n");



	_GenerateReadConverter(File, vgfGetColorGamma, "zero2one_l2s", 5);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColorGamma, "zero2one_l2s", 6);
	vgfWriteLine(File, "\n");
	_GenerateReadConverter(File, vgfGetColorGamma, "zero2one_l2s", 8);
}

void GenerateLookupTables(
	CStdioFile& File
	)
{
	vgfWriteLine(File, "/*******************************************************************************\n");
	vgfWriteLine(File, "** Table access macros.\n");
	vgfWriteLine(File, "*/\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "#define vgmCHANNELMASK(Format, Mask) \\\n");
	vgfWriteLine(File,     "\t_ ## Format ## _channelMask[Mask]\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "#define vgmZERO2ONE(Value, BitWidth) \\\n");
	vgfWriteLine(File,     "\t* ((gctFLOAT_PTR) & _zero2one_ ## BitWidth ## bit [Value])\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "#define vgmZERO2ONE_L2S(Value, BitWidth) \\\n");
	vgfWriteLine(File,     "\t* ((gctFLOAT_PTR) & _zero2one_l2s_ ## BitWidth ## bit [Value])\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "#define vgmZERO2ONE_S2L(Value, BitWidth) \\\n");
	vgfWriteLine(File,     "\t* ((gctFLOAT_PTR) & _zero2one_s2l_ ## BitWidth ## bit [Value])\n");
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");

	GenerateChannelMaskLookup(File);
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");

	GenerateTableZero2One(File);
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");

	GenerateTableZero2One_S2L(File);
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");

	GenerateTableZero2One_L2S(File);
}

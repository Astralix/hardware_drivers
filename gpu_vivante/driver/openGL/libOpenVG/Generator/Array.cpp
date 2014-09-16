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

typedef struct _vgsGROUP_INFO
{
	gctSTRING titleSwizzle;
	gctSTRING arrayNameSwizzle;
}
vgsGROUP_INFO;

static vgsGROUP_INFO groupInfo[4] =
{
	{ "RGB-A", "rgba" },
	{ "A-RGB", "argb" },
	{ "BGR-A", "bgra" },
	{ "A-BGR", "abgr" }
};

static void _GenerateFormatEntry(
	CStdioFile& File,
	vgsFORMAT_INFO_PTR Format,
	gctINT GroupIndex,
	gctINT FormatIndex,
	gctUINT& FunctionCount
	)
{
	/* Luminance? */
	gctBOOL luminance
		= (Format->l.width != 0);

	/* Alpha-only? */
	gctBOOL alpha
		=  (Format->a.width != 0)
		&& (Format->r.width == 0)
		&& (Format->g.width == 0)
		&& (Format->b.width == 0);

	/* RGB? */
	gctBOOL rgb
		=  (Format->a.width == 0)
		&& (Format->r.width != 0)
		&& (Format->g.width != 0)
		&& (Format->b.width != 0);

	/* RGBA? */
	gctBOOL rgba
		=  (Format->a.width != 0)
		&& (Format->r.width != 0)
		&& (Format->g.width != 0)
		&& (Format->b.width != 0);

	/* Grayscale? */
	gctBOOL grayscale
		= luminance || alpha;

	/* Determine the tabbing. */
	CString tab(GroupIndex >= 0 ? "\t\t" : "\t");

	/* Determine the upsampled format name. */
	CString upsampledName;

	if (Format->upsampledFormat == gcvNULL)
	{
		upsampledName = "gcvNULL";
	}
	else
	{
		upsampledName.Format("&" vgvUPSAMPLED_INFO_NAME, Format->upsampledFormat);
	}

	/* Determine the function name postfix. */
	CString writePostfix;

	if (GroupIndex >= 0)
	{
		writePostfix = Format->name;
	}
	else
	{
		writePostfix.Format(vgvUPSAMPLED_FUNC_NAME, Format->name);
	}

	vgfWriteLine(File, "%s/* Group Index      : */\t%d,\n", (LPCTSTR) tab, GroupIndex);
	vgfWriteLine(File, "%s/* Format Index     : */\t%d,\n", (LPCTSTR) tab, FormatIndex);
	vgfWriteLine(File, "%s/* Internal Format  : */\t%s,\n", (LPCTSTR) tab, Format->internalFormat);
	vgfWriteLine(File, "%s/* Upsampled Format : */\t%s,\n", (LPCTSTR) tab, (LPCTSTR) upsampledName);
	vgfWriteLine(File, "%s/* Bits per pixel   : */\t%d,\n", (LPCTSTR) tab, Format->bitsPerPixel);
	vgfWriteLine(File, "%s/* Native format    : */\t%s,\n", (LPCTSTR) tab, Format->nativeFormat  ? "gcvTRUE" : "gcvFALSE");
	vgfWriteLine(File, "%s/* Premultiplied    : */\t%s,\n", (LPCTSTR) tab, Format->premultiplied ? "gcvTRUE" : "gcvFALSE");
	vgfWriteLine(File, "%s/* Linear           : */\t%s,\n", (LPCTSTR) tab, Format->linear        ? "gcvTRUE" : "gcvFALSE");
	vgfWriteLine(File, "%s/* Grayscale        : */\t%s,\n", (LPCTSTR) tab, grayscale             ? "gcvTRUE" : "gcvFALSE");
	vgfWriteLine(File, "%s/* Luminance        : */\t%s,\n", (LPCTSTR) tab, luminance             ? "gcvTRUE" : "gcvFALSE");

	if (luminance)
	{
		vgfWriteLine(File, "%s/* Red channel      : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2, Format->l.start, Format->l.width);
		vgfWriteLine(File, "%s/* Green channel    : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2,               0,               0);
		vgfWriteLine(File, "%s/* Blue channel     : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2,               0,               0);
		vgfWriteLine(File, "%s/* alpha channel    : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2,               0,               0);
	}
	else
	{
		vgfWriteLine(File, "%s/* Red channel      : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2, Format->r.start, Format->r.width);
		vgfWriteLine(File, "%s/* Green channel    : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2, Format->g.start, Format->g.width);
		vgfWriteLine(File, "%s/* Blue channel     : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2, Format->b.start, Format->b.width);
		vgfWriteLine(File, "%s/* alpha channel    : */\t{ %*d, %d },\n", (LPCTSTR) tab, 2, Format->a.start, Format->a.width);
	}

	vgfWriteLine(File, "\n");

	/*
		Write pixels readers.
	*/
	vgfWriteLine(File, "%s/* Reader array. */\n", (LPCTSTR) tab);
	vgfWriteLine(File, "%s{\n", (LPCTSTR) tab);

	/* Go through all color spaces. */
	for (gctUINT currentColorSpace = 0; currentColorSpace < gcmCOUNTOF(colorSpaceArray); currentColorSpace += 1)
	{
		/* Go through all color spaces. */
		for (gctUINT currentPremul = 0; currentPremul < gcmCOUNTOF(premulArray); currentPremul += 1)
		{
			CString formatName(Format->name);
			CString targetColorSpace(colorSpaceArray[currentColorSpace]);

			gctBOOL lastFunction
				=  (currentPremul     == gcmCOUNTOF(premulArray)     - 1)
				&& (currentColorSpace == gcmCOUNTOF(colorSpaceArray) - 1);

			gctSTRING premulPostfix = (luminance || rgb)
				? premulArray[0]
				: premulArray[currentPremul];

			if (alpha)
			{
				targetColorSpace.Empty();
			}

			if (luminance && (Format->l.width == 8) && (Format->linear == currentColorSpace))
			{
				formatName.Delete(0, 1);
				targetColorSpace.Empty();
			}

			if (rgb && (Format->linear == currentColorSpace))
			{
				formatName.Delete(0, 1);
				targetColorSpace.Empty();
			}

			vgfWriteLine(
				File, "%s\t" vgvPIXEL_READ "%s",
				(LPCTSTR) tab,
				(LPCTSTR) formatName,
				(LPCTSTR) targetColorSpace, premulPostfix,
				lastFunction ? "\n" : ",\n"
				);
		}
	}

	vgfWriteLine(File, "%s},\n", (LPCTSTR) tab);
	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "%s/* Writer array. */\n", (LPCTSTR) tab);
	vgfWriteLine(File, "%s{\n", (LPCTSTR) tab);

	/* Go through all color spaces. */
	for (gctUINT currentColorSpace = 0; currentColorSpace < gcmCOUNTOF(colorSpaceArray); currentColorSpace += 1)
	{
		/* Go through all color spaces. */
		for (gctUINT currentPremul = 0; currentPremul < gcmCOUNTOF(premulArray); currentPremul += 1)
		{
			gctBOOL lastColorVariation
				=  (currentPremul     == gcmCOUNTOF(premulArray)     - 1)
				&& (currentColorSpace == gcmCOUNTOF(colorSpaceArray) - 1);

			/* Go through all masks. */
			for (gctUINT currentMask = 0; currentMask < gcmCOUNTOF(maskArray); currentMask += 1)
			{
				gctBOOL lastMask
					= (currentMask == gcmCOUNTOF(maskArray) - 1);

				gctBOOL lastFunction
					= lastColorVariation && lastMask;

				do
				{
					/* Write mask comment. */
					vgfWriteLine(File, "%s\t/* %s */\t", (LPCTSTR) tab, maskArray[currentMask]);

					/* Luminance? */
					if (luminance)
					{
						vgfWriteLine(
							File, vgvFULL_PIXEL_WRITE "%s",
							colorSpaceArray[currentColorSpace], maskArray[VG_RGB], premulArray[currentPremul],
							(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
							);

						if (currentMask == 0)
						{
							FunctionCount += 1;
						}

						break;
					}

					/* Alpha-only? */
					if (alpha)
					{
						if ((currentMask & VG_ALPHA) == 0)
						{
							vgfWriteLine(File, "gcvNULL,\n");
						}
						else
						{
							vgfWriteLine(
								File, vgvFULL_PIXEL_WRITE "%s",
								"", maskArray[VG_ALPHA], "",
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);

							if (currentMask == VG_ALPHA)
							{
								FunctionCount += 1;
							}
						}

						break;
					}

					/* RGB format? */
					if (rgb)
					{
						if ((currentMask == 0) || (currentMask == VG_ALPHA))
						{
							vgfWriteLine(File, "gcvNULL,\n");
							break;
						}

						if ((currentMask & VG_RGB) == VG_RGB)
						{
							vgfWriteLine(
								File, vgvFULL_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);
						}
						else
						{
							vgfWriteLine(
								File, vgvMASKED_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);
						}

						if ((currentMask == VG_RGB) || (currentMask == VG_RED))
						{
							FunctionCount += 1;
						}

						break;
					}

					/* RGBA format? */
					if (rgba)
					{
						if (currentMask == VG_RGBA)
						{
							vgfWriteLine(
								File, vgvFULL_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);

							FunctionCount += 1;

							break;
						}

						if (!Format->premultiplied)
						{
							vgfWriteLine(
								File, vgvMASKED_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);

							if (currentMask == 0)
							{
								FunctionCount += 1;
							}

							break;
						}

						if ((currentMask & VG_ALPHA) == VG_ALPHA)
						{
							vgfWriteLine(
								File, vgvMASKED_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGBA], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);
						}
						else
						{
							vgfWriteLine(
								File, vgvMASKED_PIXEL_WRITE "%s",
								colorSpaceArray[currentColorSpace], maskArray[VG_RGB], premulArray[currentPremul],
								(LPCTSTR) writePostfix, lastFunction ? "\n" : ",\n"
								);
						}

						if ((currentMask == 0) || (currentMask == 1))
						{
							FunctionCount += 1;
						}

						break;
					}

					ASSERT(FALSE);
				}
				while (gcvFALSE);

				if (lastMask && !lastFunction)
				{
					vgfWriteLine(File, "\n");
				}
			}
		}
	}

	vgfWriteLine(File, "%s}\n", (LPCTSTR) tab);
}

void GenerateUpsampledFormatArray(
	CStdioFile& File
	)
{
	vgsFORMAT_LIST_PTR formatList;
	gctUINT formatCount;
	gctUINT functionCount;
	gctBOOL firstFunction;

	/* Reset the number of functions. */
	functionCount = 0;

	/* Get format groups. */
	vgfGetUpsampledFormatList(&formatList);

	/* Get the number of formats. */
	formatCount = formatList->count;

	/* Write the title. */
	vgfGenerateCaption(File, "Upsampled formats.");

	/* Set the first function flag. */
	firstFunction = gcvTRUE;

	/* Go through all formats in the swizzle group. */
	for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
	{
		/* Get the current format. */
		vgsFORMAT_INFO_PTR format = &formatList->group[currentFormat];

		/* Add spacing. */
		if (!firstFunction)
		{
			vgfWriteLine(File, "\n");
		}

		firstFunction = gcvFALSE;

		/* Write array definition. */
		vgfWriteLine(
			File, "static vgsFORMAT " vgvUPSAMPLED_INFO_NAME " =\n",
			format->name
			);

		vgfWriteLine(File, "{\n");

		/* Generate the entry. */
		_GenerateFormatEntry(File, format, -1, 0, functionCount);

		vgfWriteLine(File, "};\n");
	}

	printf("  Total of %d upsampled writer functions have been created.\n", functionCount);
}

void GenerateFormatArray(
	CStdioFile& File
	)
{
	vgsFORMAT_LIST_PTR formatGroups;
	gctUINT groupCount;
	gctUINT functionCount;

	/* Reset the number of functions. */
	functionCount = 0;

	/* Get format groups. */
	vgfGetFormatGroupList(&formatGroups, &groupCount);

	/* Go through all swizzle groups. */
	for (gctUINT currentGroup = 0; currentGroup < groupCount; currentGroup += 1)
	{
		gctUINT formatCount = formatGroups[currentGroup].count;

		/* Add space. */
		vgfWriteLine(File, "\n");
		vgfWriteLine(File, "\n");

		/* Write the group title. */
		{
			CString groupTitle;

			groupTitle.Format(
				"Format table for %s channel ordering.",
				groupInfo[currentGroup].titleSwizzle
				);

			vgfGenerateCaption(File, groupTitle);
		}

		/* Write array definition. */
		{
			vgfWriteLine(
				File, "static vgsFORMAT _%sFormatTable[vgvFORMAT_COUNT] =\n",
				groupInfo[currentGroup].arrayNameSwizzle
				);

			vgfWriteLine(
				File, "{\n"
				);
		}

		/* Go through all formats in the swizzle group. */
		for (gctUINT currentFormat = 0; currentFormat < formatCount; currentFormat += 1)
		{
			/* Get the current format. */
			vgsFORMAT_INFO_PTR format = &formatGroups[currentGroup].group[currentFormat];

			/* Determine whether this is the last format. */
			gctBOOL lastFormat = (currentFormat == (formatCount - 1));

			/* Valid format? */
			if (format->name == gcvNULL)
			{
				vgfWriteLine(File, "\t/* DUMMY ENTRY */\n");
				vgfWriteLine(File, "\t{\n");
				vgfWriteLine(File, "\t\t~0, ~0, gcvSURF_UNKNOWN\n");
			}
			else
			{
				vgfWriteLine(File, "\t/* VG_%s */\n", format->name);
				vgfWriteLine(File, "\t{\n");

				/* Generate the entry. */
				_GenerateFormatEntry(File, format, currentGroup, currentFormat, functionCount);
			}

			if (lastFormat)
			{
				vgfWriteLine(File, "\t}\n");
			}
			else
			{
				vgfWriteLine(File, "\t},\n\n");
			}
		}

		vgfWriteLine(File, "};\n");
	}

	vgfWriteLine(File, "\n");
	vgfWriteLine(File, "\n");

	vgfGenerateCaption(File, "Master image format table.");

	/* Write array definition. */
	vgfWriteLine(File, "static vgsFORMAT_PTR _masterFormatTable[] =\n");
	vgfWriteLine(File, "{\n");

	/* Go through all swizzle groups. */
	for (gctUINT currentGroup = 0; currentGroup < groupCount; currentGroup += 1)
	{
		gctBOOL lastGroup = (currentGroup == (groupCount - 1));

		vgfWriteLine(
			File, "\t_%sFormatTable%s\n",
			groupInfo[currentGroup].arrayNameSwizzle,
			lastGroup ? "" : ","
			);
	}

	vgfWriteLine(File, "};\n");

	printf("  Total of %d writer functions have been created.\n", functionCount);
}

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

#include <tchar.h>
#include <afx.h>
#include "Generator.h"
#include "Lookup.h"
#include "Array.h"
#include "Reader.h"
#include "Writer.h"

gctSTRING maskArray[16] =
{
	"xxxx",
	"xxxA",
	"xxBx",
	"xxBA",
	"xGxx",
	"xGxA",
	"xGBx",
	"xGBA",
	"Rxxx",
	"RxxA",
	"RxBx",
	"RxBA",
	"RGxx",
	"RGxA",
	"RGBx",
	"RGBA"
};

gctSTRING colorSpaceArray[2] =
{
	"s",
	"l"
};

gctSTRING premulArray[2] =
{
	"",
	"_PRE"
};

static gctBOOL _GenerateFileHeader(
	CStdioFile& File,
	CString Filename,
	CString Description
	)
{
	printf("Creating output file: %s\n", (LPCTSTR) Filename);

	gctBOOL success = File.Open(
		Filename,
		CFile::modeCreate | CFile::modeWrite | CFile::typeText
		);

	if (!success)
	{
		return gcvFALSE;
	}

	CString description;
	description.Format("|*   %s %*s *|\n",
		Description,
		71 - Description.GetLength(), ""
		);

	File.WriteString("\n");
	File.WriteString("\n");

	return gcvTRUE;
}

void vgfWriteLine(
	CStdioFile& File,
	LPCTSTR Line,
	...
	)
{
	va_list arguments;
	va_start(arguments, Line);

	char buffer[1024];
	vsprintf_s(buffer, sizeof(buffer), Line, arguments);

	File.WriteString(buffer);
}

void vgfGenerateCaption(
	CStdioFile& File,
	CString Description
	)
{
	CString description = "********************************************************************************\n";

	Description = " " + Description + " ";
	gctUINT len = Description.GetLength();
	gctUINT pos = (81 - len) / 2;

	for (gctUINT i = 0; i < len; i += 1)
	{
		description.SetAt(pos + i, Description[i]);
	}

	vgfWriteLine(File, "/******************************************************************************\\\n");
	vgfWriteLine(File, description);
	vgfWriteLine(File, "\\******************************************************************************/\n");
	vgfWriteLine(File, "\n");
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	gctBOOL success;
	CStdioFile lookupTableFile;
	CStdioFile formatArrayFile;
	CStdioFile writerFile;
	CStdioFile readerFile;

	{
		success = _GenerateFileHeader(
			lookupTableFile, "vgFormatLookup.c", "Lookup tables for pixel access functions."
			);

		if (success)
		{
			GenerateLookupTables(lookupTableFile);
			lookupTableFile.Close();
		}
	}

	{
		success = _GenerateFileHeader(
			formatArrayFile, "vgFormatArray.c", "Format description arrays."
			);

		if (success)
		{
			GenerateUpsampledFormatArray(formatArrayFile);
			GenerateFormatArray(formatArrayFile);
			formatArrayFile.Close();
		}
	}

	{
		success = _GenerateFileHeader(
			writerFile, "vgFormatWriter.c", "Pixel writer routines."
			);

		if (success)
		{
			GenerateDefinitions(writerFile);
			GenerateUpsampledWriters(writerFile);
			GenerateWriters(writerFile);
			writerFile.Close();
		}
	}

	{
		success = _GenerateFileHeader(
			readerFile, "vgFormatReader.c", "Pixel reader routines."
			);

		if (success)
		{
			GenerateReaders(readerFile);
			readerFile.Close();
		}
	}

	return 0;
}

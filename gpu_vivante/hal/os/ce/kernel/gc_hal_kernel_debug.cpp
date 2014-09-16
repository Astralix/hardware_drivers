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




#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include "gc_hal.h"


/******************************************************************************\
********************************** Debug Code **********************************
\******************************************************************************/

EXTERN_C void
OutputString(
	IN gctCONST_STRING Message
	)
{
	wchar_t buffer[256] = L"GCHAL: ";
	int i;

	/* Convert string to wide format. */
	i = 6;

	do
	{
		buffer[i] = *Message++;
	}
	while (buffer[i++] != '\0');

	/* Print string to debug terminal. */
	OutputDebugString(buffer);
}

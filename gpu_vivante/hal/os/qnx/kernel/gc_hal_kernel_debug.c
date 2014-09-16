/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/




#include "gc_hal_kernel_qnx.h"
#include <stdarg.h>

/*
    gcdBUFFERED_OUTPUT

    When set to non-zero, all output is collected into a buffer with the
    specified size.  Once the buffer gets full, or the token "$$FLUSH$$" has
    been received, the debug buffer will be printed to the console.
*/
#define gcdBUFFERED_OUTPUT  0


void
OutputDebugString(
    IN gctCONST_STRING String
    )
{
#if gcdBUFFERED_OUTPUT
    static gctCHAR outputBuffer[gcdBUFFERED_OUTPUT];
    static gctINT outputBufferIndex = 0;
    gctINT n, i;

    n = (String != gcvNULL) ? strlen(String) + 1 : 0;

    if ((n == 0) || (outputBufferIndex + n > gcmSIZEOF(outputBuffer)))
    {
        for (i = 0; i < outputBufferIndex; i += strlen(outputBuffer + i) + 1)
        {
            printf(outputBuffer + i);
        }

        outputBufferIndex = 0;
    }

    if (n > 0)
    {
        memcpy(outputBuffer + outputBufferIndex, String, n);
        outputBufferIndex += n;
    }
#else
    if (String != gcvNULL)
    {
        printf(String);
    }
#endif
}


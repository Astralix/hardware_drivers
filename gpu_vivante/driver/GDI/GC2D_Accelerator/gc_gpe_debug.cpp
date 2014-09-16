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



#include "gc_gpe_precomp.h"

#ifdef DEBUG
static const gctUINT32 TRACE_DOMAIN = GC2D_TRACE_DOMAIN;

#define MAX_PRINT_SIZE 1024
static void
_Print(
    IN char * Prefix,
    IN char * Message
    )
{
    wchar_t buffer[MAX_PRINT_SIZE];
    int n = 0, m = 0;

    if (Message == NULL)
        return;

    /* Convert prefix to wide format. */
    while (Prefix[n] != '\0' && n < MAX_PRINT_SIZE)
    {
        buffer[n] = Prefix[n];
        ++n;
    }

    /* Convert string to wide format. */
    while (Message[m] != '\0' && n < MAX_PRINT_SIZE)
    {
        buffer[n] = Message[m];
        ++n;
        ++m;
    }

    if (buffer[n - 1] != '\n' && buffer[n - 1] != '\r')
    {
        if (n < MAX_PRINT_SIZE)
        {
            buffer[n++] = '\r';
        }

        if (n < MAX_PRINT_SIZE)
        {
            buffer[n++] = '\n';
        }
    }

    if (n == MAX_PRINT_SIZE)
    {
        OutputDebugString(TEXT("Print Error: out of print buffer\n"));
    }
    else
    {
        buffer[n] = 0;

        /* Print string to debug terminal. */
        OutputDebugString(buffer);
    }
}

#endif
/*******************************************************************************
**
**    GC2D_DebugFatal
**
**    Send a message to the debugger and break into the debugger.
**
**    INPUT:
**
**        char * Message
**            Pointer to message.
**
**        ...
**            Optional arguments.
**
**    OUTPUT:
**
**        Nothing.
*/
void
GC2D_DebugFatal(
    IN char* Message,
    ...
    )
{
#ifdef DEBUG
    va_list arguments;
    char string[256];

    /* Get pointer to the optional arguments. */
    va_start(arguments, Message);

    /* Format message and the optional arguments. */
    vsprintf(string, Message, arguments);

    /* Remove pointer to the optional arguments. */
    va_end(arguments);

    /* Print string to debug terminal. */
    _Print("GC2D FATAL ERROR:", string);

    /* Break into the debugger. */
    DebugBreak();
#endif
}

/*******************************************************************************
**
**    GC2D_DebugTrace
**
**    Send a leveled message to the debugger.
**
**    INPUT:
**
**        gctUINT32 Level
**            Debug level of message.
**
**        char * Message
**            Pointer to message.
**
**        ...
**            Optional arguments.
**
**    OUTPUT:
**
**        Nothing.
*/
void GC2D_DebugTrace(
    IN gctUINT32 Flag,
    IN char* Message,
    ...
    )
{
#ifdef DEBUG
    va_list arguments;
    char string[256];

    if (!(Flag & TRACE_DOMAIN))
        return;

    /* Get pointer to the optional arguments. */
    va_start(arguments, Message);

    /* Format message and the optional arguments. */
    vsprintf(string, Message, arguments);

    /* Remove pointer to the optional arguments. */
    va_end(arguments);

    /* Print string to debug terminal. */
    if (Flag & GC2D_ERROR_TRACE)
    {
        _Print("GC2D ERROR:", string);
    }
    else
    {
        _Print("GC2D:", string);
    }
#endif
}

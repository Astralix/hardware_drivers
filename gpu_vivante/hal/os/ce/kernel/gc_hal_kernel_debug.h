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




#ifndef __gc_hal_kernel_debug_h_
#define __gc_hal_kernel_debug_h_

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** OS-dependent Macros *****************************
\******************************************************************************/

typedef va_list gctARGUMENTS;

#define gcmkARGUMENTS_START(Arguments, Pointer) \
    va_start(Arguments, Pointer)

#define gcmkARGUMENTS_END(Arguments) \
    va_end(Arguments)

#define	gcmkDECLARE_LOCK(__lockHandle__) \
    static HANDLE __lockHandle__;

#define gcmkLOCKSECTION(__lockHandle__) \
    if (__lockHandle__ == gcvNULL) \
    { \
        __lockHandle__ = CreateMutex(gcvNULL, gcvFALSE, gcvNULL); \
    } \
    \
    WaitForSingleObject(__lockHandle__, INFINITE)

#define gcmkUNLOCKSECTION(__lockHandle__) \
    ReleaseMutex(__lockHandle__)

#define gcmkGETPROCESSID() \
    GetCurrentProcessId()

#define gcmkGETTHREADID() \
    GetCurrentThreadId()

void
OutputString(
	IN gctCONST_STRING Message
	);

#define gcmkOUTPUT_STRING(String) \
    OutputString(String)

#define gcmkSPRINTF(Destination, Size, Message, Value) \
    _snprintf(Destination, Size, Message, Value)

#define gcmkSPRINTF2(Destination, Size, Message, Value1, Value2) \
    _snprintf(Destination, Size, Message, Value1, Value2)

#define gcmkSPRINTF3(Destination, Size, Message, Value1, Value2, Value3) \
    _snprintf(Destination, Size, Message, Value1, Value2, Value3)

#define gcmkVSPRINTF(Destination, Size, Message, Arguments) \
    _vsnprintf(Destination, Size, Message, Arguments)

#define gcmkSTRCAT(Destination, Size, String) \
    strncat(Destination, String, Size)

/* If not zero, forces data alignment in the variable argument list
   by its individual size. */
#define gcdALIGNBYSIZE      0

#ifdef __cplusplus
}
#endif

#endif /* __gc_hal_kernel_debug_h_ */

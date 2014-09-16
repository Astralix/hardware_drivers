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




#include "gc_glff_precomp.h"

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const char * _GLES_CM_VERSION = "\n\0$VERSION$"
                                gcmTXT2STR(gcvVERSION_MAJOR) "."
                                gcmTXT2STR(gcvVERSION_MINOR) "."
                                gcmTXT2STR(gcvVERSION_MINOR) ":"
                                gcmTXT2STR(gcvVERSION_BUILD)
                                "$\n";

#ifdef WIN32
#include <windows.h>

gctBOOL __stdcall DllMain(
    IN HINSTANCE Instance,
    IN DWORD Reason,
    IN LPVOID Reserved
)
{
    return gcvTRUE;
}
#endif

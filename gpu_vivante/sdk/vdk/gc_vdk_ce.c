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

/*******************************************************************************
** Windows OS wrappers. ********************************************************
*/

/*
    DllMain

    DLL entry point.  We do nothing here.

    PARAMETERS:

        HMODULE Module
            Module of the library.

        DWORD ReasonForCall
            Reason why the entry point is called:

            DLL_PROCESS_ATTACH
                Indicates the DLL is loaded by the current process.

            DLL_THREAD_ATTACH
                Indicates the current process is creating a new thread.

            DLL_THREAD_DETACH
                Indicates that a thread is exiting.

            DLL_PROCESS_DETACH
                Indicates the current process is exiting or called FreeLibrary.

        LPVOID Reserved
            NULL is all cases except when the reason is DLL_PROCESS_DETACH and
            the current process is exiting.

    RETURNS:

        BOOL
            TRUE if the DLL initialized successfully or FALSE if there was an
            error.
*/
#if !gcdSTATIC_LINK
BOOL APIENTRY
DllMain(
    HMODULE Module,
    DWORD ReasonForCall,
    LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER(Module);
    UNREFERENCED_PARAMETER(ReasonForCall);
    UNREFERENCED_PARAMETER(Reserved);

    /* Success. */
    return TRUE;
}
#endif
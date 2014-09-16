//
//  wiProcess.h -- WISE Process and Thread Utilities
//  Copyright 2011 DSP Group, Inc. All rights reserved.
//

#ifndef __WIPROCESS_H
#define __WIPROCESS_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MULTITHREADING SUPPORT
//
//  Multithreading support is enabled by defining WISE_BUILD_MULTITHREADED. The threading method
//  is implementation dependent. If WISE_USE_PTHREADS is enabled, then the POSIX thread library
//  is used; otherwise, if WIN32 is defined, then the Windows library is used.
//
//  If multithreading is enabled, WISE_MAX_THREADS defines the number of threads. This includes the
//  base process, so if wiTest will launch 4 parallel TxRxPacket threads, WISE_MAX_THREADS is 5 so
//  that each worker thread is indexed 1..4 and the base process has index 0. If multithreading is
//  not enabled, then WISE_MAX_THREADS is simply 1 to account for the process.
//
#if defined WISE_BUILD_MULTITHREADED

    #if defined WISE_USE_PTHREADS // POSIX Threading Library

        #include <pthread.h>
        typedef pthread_mutex_t wiMutex_t;
        typedef pthread_t       wiThread_t;

    #elif defined WIN32 // Windows Threading Library

        typedef void * wiMutex_t;
        typedef void * wiThread_t;

    #else // No Threading Library Selected

        #pragma message("undefine WISE_BUILD_MULTITHREADED; no threading library selected")
        #undef WISE_BUILD_MULTITHREADED

    #endif
#else
    typedef void * wiMutex_t;
    typedef void * wiThread_t;
#endif

#if defined WISE_BUILD_MULTITHREADED

    #ifndef WISE_MAX_THREADS
        #define WISE_MAX_THREADS (1 + 4)
    #endif

#else

    #ifdef WISE_MAX_THREADS
    #undef WISE_MAX_THREADS
    #endif
    #define WISE_MAX_THREADS 1

#endif

#define WISE_MAX_THREADINDEX (WISE_MAX_THREADS - 1)

typedef enum
{
    WIPROCESS_PRIORITY_NORMAL = 0,
    WIPROCESS_PRIORITY_IDLE = 1,

} wiProcessPriority_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Pointer to a thread start function
//  Thread start functions passed to wiProcess_CreateThread must use this prototype.
//
typedef wiStatus (* wiThreadFn_t)(void *pArgument);

//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================
wiStatus  wiProcess_Init(void);
wiStatus  wiProcess_Close(void);

//---- Process Control/Information ----------------------------------------------------------------
wiStatus wiProcess_SetPriority(wiProcessPriority_t Priority);
wiReal   wiProcess_Time(void);

//---- Thread Management --------------------------------------------------------------------------
unsigned wiProcess_ThreadIndex(void);
wiStatus wiProcess_InitializeThreadIndex(wiUInt ThreadIndex);
wiStatus wiProcess_ReleaseThreadIndex   (wiUInt ThreadIndex);

wiStatus wiProcess_CreateThread(wiThreadFn_t StartFunction, void *Argument, wiUInt *pThreadIndex);
wiStatus wiProcess_WaitForThread(wiUInt ThreadIndex);
wiStatus wiProcess_CloseThread  (wiUInt ThreadIndex);
wiStatus wiProcess_WaitForAllThreadsAndClose(void);

//---- Mutex --------------------------------------------------------------------------------------
wiStatus wiProcess_CreateMutex (wiMutex_t *pMutex);
wiStatus wiProcess_CloseMutex  (wiMutex_t *pMutex);
wiStatus wiProcess_WaitForMutex(wiMutex_t *pMutex);
wiStatus wiProcess_ReleaseMutex(wiMutex_t *pMutex);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __WIPROCESS_H
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

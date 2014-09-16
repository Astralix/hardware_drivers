//--< wiProcess.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Process and Thread Utilities
//  Copyright 2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include "wiProcess.h"
#include "wiUtil.h"
#include "wiError.h"

/////////////////////////////////////////////////////////////////////////////////////////
///
///  Platform-Specific Definitions
///
#if defined WIN32

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

#endif

#if defined WISE_USE_PTHREADS

    #include <pthread.h>
    #include <errno.h>
    static pthread_key_t wiProcessKey;

#elif defined WIN32

    static DWORD  dwTlsIndex;

#endif
///
/////////////////////////////////////////////////////////////////////////////////////////

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean  ModuleIsInitialized = WI_FALSE;

#if defined(WISE_BUILD_MULTITHREADED)

    typedef struct
    {
        wiUInt       ThreadIndex; // 1..WISE_MAX_THREADS
        wiBoolean    ThreadInUse; // is thread active?
        wiThread_t   Thread;      // thread handle
        wiThreadFn_t StartFn;     // function pointer
        wiStatus     ExitStatus;  // from StartFn
        void        *Argument;    // to pass to StartFn

    } wiProcessThreadData_t;

    static wiProcessThreadData_t ThreadData[WISE_MAX_THREADS + 1] = {{0}};

#endif

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(Status) WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status) < 0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION  : wiProcess_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_Init(void)
{
    if (ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

    #ifdef WISE_BUILD_MULTITHREADED

        #if defined WISE_USE_PTHREADS

            pthread_key_create(&wiProcessKey, NULL);

        #elif defined WIN32 
        {
            LPVOID lpvData = (LPVOID) LocalAlloc(LPTR, sizeof(int));
            if (!lpvData) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
	        
            // Allocate thread-local memory used to store ThreadIndex
            dwTlsIndex = TlsAlloc(); 
            TlsSetValue(dwTlsIndex, lpvData);
            (*(int *)lpvData) = 0;
        }
        #else
            XSTATUS( WI_ERROR );
        #endif
    #endif

    ModuleIsInitialized = WI_TRUE;

    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_Close(void)
{
    if (!ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

    #ifdef WISE_BUILD_MULTITHREADED

        #if defined WISE_USE_PTHREADS

            // nothing to do

        #elif defined WIN32 
        {
            LPVOID lpvData = TlsGetValue(dwTlsIndex); 
            LocalFree((HLOCAL)lpvData);
            TlsFree(dwTlsIndex);
        }
        #endif
    #endif

    ModuleIsInitialized = WI_FALSE;
    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_SetPriority()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_SetPriority(wiProcessPriority_t Priority)
{
    #if defined WIN32

        switch (Priority)
        {
            case WIPROCESS_PRIORITY_NORMAL: SetPriorityClass(GetCurrentProcess(),NORMAL_PRIORITY_CLASS); break;
            case WIPROCESS_PRIORITY_IDLE  : SetPriorityClass(GetCurrentProcess(),  IDLE_PRIORITY_CLASS); break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }

    #endif

    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_Time()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal wiProcess_Time(void)
{
    double tCPU = 0.0;

    #if defined WIN32
    {
        FILETIME CreationTime, ExitTime, KernelTime, UserTime;

        GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime);

        tCPU += (double)(KernelTime.dwLowDateTime ) *  100.0E-9;
        tCPU += (double)(KernelTime.dwHighDateTime) * (100.0E-9 * 4294067296.0);

        tCPU += (double)(UserTime.dwLowDateTime   ) *  100.0E-9;
        tCPU += (double)(UserTime.dwHighDateTime  ) * (100.0E-9 * 4294067296.0);
    }
    #endif

    return tCPU;
}


// ================================================================================================
// FUNCTION  : wiProcess_ThreadIndex()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
unsigned int wiProcess_ThreadIndex(void)
{
    #ifdef WISE_BUILD_MULTITHREADED

        #if defined WISE_USE_PTHREADS
        {
            void *lpvData = pthread_getspecific(wiProcessKey);
            return lpvData ? *((unsigned int *)lpvData) : 0;
        }
        #elif defined WIN32
        {
            LPVOID lpvData = TlsGetValue(dwTlsIndex); 
            return *((unsigned int *)lpvData);
        }
        #else
            return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
        #endif

    #else
        return 0;
    #endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef WISE_BUILD_MULTITHREADED // Multithreading Support /////////////////////////////////////////

// ================================================================================================
// FUNCTION  : wiProcess_InitializeThreadIndex()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_InitializeThreadIndex(unsigned ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADINDEX)) return WI_ERROR_PARAMETER1;

    #if defined WISE_USE_PTHREADS
    {
        void *lpvData = wiMalloc(sizeof(int));
        if (!lpvData) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
        pthread_setspecific(wiProcessKey, lpvData);
        (*(unsigned int *)lpvData) = ThreadIndex;
        return WI_SUCCESS;
    }
    #elif defined WIN32
    {
        LPVOID lpvData;
        
        lpvData = (LPVOID) LocalAlloc(LPTR, sizeof(int));
        TlsSetValue(dwTlsIndex, lpvData);
        (*(unsigned int *)lpvData) = ThreadIndex;
        return WI_SUCCESS;
    }
    #else
        return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
    #endif
}


// ================================================================================================
// FUNCTION  : wiProcess_ReleaseThreadIndex()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_ReleaseThreadIndex(unsigned ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADINDEX)) return WI_ERROR_PARAMETER1;

    #if defined WISE_USE_PTHREADS
    {
        void *lpvData = pthread_getspecific(wiProcessKey);
        WIFREE(lpvData);
        pthread_setspecific(wiProcessKey, NULL);
        return WI_SUCCESS;
    }
    #elif defined WIN32
    {
        LPVOID lpvData;
        
        lpvData = TlsGetValue(dwTlsIndex); 
        LocalFree((HLOCAL)lpvData);
        return WI_SUCCESS;
    }
    #else
        return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
    #endif
}


#if defined WIN32

    // ==================================================================================
    // FUNCTION  : wiProcess_WindowsStartFn
    // ----------------------------------------------------------------------------------
    // ==================================================================================
    static DWORD WINAPI wiProcess_WindowsStartFn( LPVOID lpParam )
    {
        DWORD ReturnCode = 0;
        wiProcessThreadData_t *pData = (wiProcessThreadData_t *)lpParam;

        if (STATUS( wiProcess_InitializeThreadIndex(pData->ThreadIndex)) < WI_SUCCESS)
            ReturnCode = 1;
        else 
        {
            if (STATUS( pData->StartFn(pData->Argument) ) < WI_SUCCESS) ReturnCode = 2;
            if (STATUS( wiProcess_ReleaseThreadIndex(pData->ThreadIndex) ) < WI_SUCCESS) ReturnCode = 3;
        }
        return ReturnCode;
    }

#endif

#if defined WISE_USE_PTHREADS

    // ==================================================================================
    // FUNCTION  : wiProcess_pThreadStartFn
    // ----------------------------------------------------------------------------------
    // ==================================================================================
    static void * wiProcess_pThreadStartFn( void *lpParam )
    {
        wiProcessThreadData_t *pData = (wiProcessThreadData_t *)lpParam;

        if (STATUS( wiProcess_InitializeThreadIndex(pData->ThreadIndex)) < WI_SUCCESS)
            ;
        else 
        {
            STATUS( pData->StartFn(pData->Argument) );
            STATUS( wiProcess_ReleaseThreadIndex(pData->ThreadIndex) );
        }
        pthread_exit (NULL);
        return NULL;
    }

#endif

// ================================================================================================
// FUNCTION  : wiProcess_CreateThread
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_CreateThread(wiThreadFn_t StartFunction, void *Argument, wiUInt *pThreadIndex)
{
    wiProcessThreadData_t *pThreadData;
    wiUInt i, ThreadIndex = 0;

    for (i = 1; i <= WISE_MAX_THREADS; i++) {
        if (!ThreadData[i].ThreadInUse) {
            ThreadIndex = i; break;
        }
    }
    if (!ThreadIndex) 
        return STATUS(WI_ERROR); // no more threads can be started
    else
    {
        if (pThreadIndex) *pThreadIndex = ThreadIndex;
        pThreadData = ThreadData + ThreadIndex;

        pThreadData->ThreadInUse = WI_TRUE;
        pThreadData->ThreadIndex = ThreadIndex;
        pThreadData->StartFn     = StartFunction;
        pThreadData->Argument    = Argument;
    }
    #if defined WISE_USE_PTHREADS
    {
        int rc = pthread_create(&(pThreadData->Thread), NULL, wiProcess_pThreadStartFn, pThreadData);
        if (rc != 0)
        {
            wiUtil_WriteLogFile("pthread_create returned %d (EAGAIN=%d, EINVAL=%d, EPERM=%d\n",rc,EAGAIN,EINVAL,EPERM);
            return STATUS(WI_ERROR);
        }
        return WI_SUCCESS;
    }
    #elif defined WIN32
    {
        pThreadData->Thread = CreateThread( NULL, 0, wiProcess_WindowsStartFn, pThreadData, 0, NULL );
        if (pThreadData->Thread == NULL) return STATUS(WI_ERROR);
        return WI_SUCCESS;
    }
    #else
        return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
    #endif
}

// ================================================================================================
// FUNCTION  : wiProcess_CloseThread
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_CloseThread(wiUInt ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADINDEX)) return WI_ERROR_PARAMETER1;

    ThreadData[ThreadIndex].ThreadInUse = WI_FALSE;
    
    #if defined WISE_USE_PTHREADS
        return WI_SUCCESS;
    #elif defined WIN32
        CloseHandle(ThreadData[ThreadIndex].Thread);
        return WI_SUCCESS;
    #else
        return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
    #endif
}

// ================================================================================================
// FUNCTION  : wiProcess_WaitForThread
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_WaitForThread(wiUInt ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADINDEX)) return WI_ERROR_PARAMETER1;

    #if defined WISE_USE_PTHREADS
    {
        void *pVoid;
        int rc = pthread_join(ThreadData[ThreadIndex].Thread, &pVoid);
        if (rc != 0)
        {
            wiUtil_WriteLogFile("pthread_join returned %d\n",rc);
            return WI_ERROR;
        }
        return WI_SUCCESS;
    }
    #elif defined WIN32
    {
        DWORD TimeOut = 0xFFFFFFFF; // milliseconds
        wiStatus status; DWORD dwStatus;

        WaitForMultipleObjects(1, &(ThreadData[ThreadIndex].Thread), TRUE, TimeOut);
        GetExitCodeThread(ThreadData[ThreadIndex].Thread, &dwStatus);
        status = (wiStatus)dwStatus;
        if (status != WI_SUCCESS)
            wiUtil_WriteLogFile("ExitCode for ThreadIndex = %d returned 0x%08X\n", ThreadIndex, dwStatus);
        if (status < WI_SUCCESS) XSTATUS(status);
        return WI_SUCCESS;
    }
    #else
        return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
    #endif
}

// ================================================================================================
// FUNCTION  : wiProcess_WaitForAllThreadsAndClose
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiProcess_WaitForAllThreadsAndClose(void)
{
    int i;

    for (i = 1; i < WISE_MAX_THREADS; i++)
    {
        if (ThreadData[i].ThreadInUse)
        {
            XSTATUS( wiProcess_WaitForThread(i) );
            XSTATUS( wiProcess_CloseThread  (i) );
        }
    }
    return WI_SUCCESS;
}

#endif // WISE_BUILD_MULTITHREADED ////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

// ================================================================================================
// FUNCTION  : wiProcess_CreateMutex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Create a mutext object to be used by the WaitForMutex and ReleaseMutex functions.
// ================================================================================================
wiStatus wiProcess_CreateMutex(wiMutex_t *pMutex)
{
    #ifdef WISE_BUILD_MULTITHREADED
    
        #if defined WISE_USE_PTHREADS

            int rc = pthread_mutex_init( pMutex, NULL );
            if (rc != 0)
            {
                wiUtil_WriteLogFile("pthread_mutex_init returnd %d\n",rc);
                return STATUS(WI_ERROR);
            }

        #elif defined WIN32

            *pMutex = CreateMutex(NULL, FALSE, NULL);
            if (*pMutex == NULL) {
                wiUtil_WriteLogFile("CreateMutex error: %d\n", GetLastError());
                return STATUS(WI_ERROR);
            }

        #else
            return WI_ERROR_NO_MULTITHREAD_SUPPORT;
        #endif

    #else

        *pMutex = NULL;

    #endif
    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_CloseMutex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the Mutex object created with wiProcess_CreateMutex
// ================================================================================================
wiStatus wiProcess_CloseMutex(wiMutex_t *pMutex)
{
    #ifdef WISE_BUILD_MULTITHREADED


        #if defined WISE_USE_PTHREADS

            int rc = pthread_mutex_destroy( pMutex );
            if (rc != 0)
            {
                wiUtil_WriteLogFile("pthread_mutex_destroy returnd %d\n",rc);
                return STATUS(WI_ERROR);
            }

        #elif defined WIN32

            if (!CloseHandle(*pMutex)) {
                wiUtil_WriteLogFile("CloseHandle error: %d\n", GetLastError());
                return STATUS(WI_ERROR);
            }

        #else
            return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);
        #endif

    #endif

    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_WaitForMutex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wait until the thread can get ownership of the mutex whose handle is passed by
//             reference. For non-multithreaded builds, the function return immediately. Operation
//             is conditioned on ModuleIsInitialized so the log file can be used by the main
//             process thread before calling wiProcess_Init(). 
// ================================================================================================
wiStatus wiProcess_WaitForMutex(wiMutex_t *pMutex)
{
    #ifdef WISE_BUILD_MULTITHREADED

        if (ModuleIsInitialized)
        {
            #if defined WISE_USE_PTHREADS

                int rc = pthread_mutex_lock( pMutex );
                if (rc != 0)
                {
                    wiUtil_WriteLogFile("pthread_mutex_lock result = %d\n",rc);
                    return WI_ERROR;
                }

            #elif defined WIN32

                DWORD dwWaitResult = WaitForSingleObject( *((HANDLE *)pMutex), INFINITE );
    
                if (dwWaitResult != WAIT_OBJECT_0)
                {
                    wiUtil_WriteLogFile("dwWaitResult = %d (%d, %d)\n",dwWaitResult,WAIT_ABANDONED,WAIT_TIMEOUT);
                    wiUtil_WriteLogFile("GetLastError: %d\n",GetLastError());
                    wiUtil_WriteLogFile("WaitForSingleObject() returned %d\n",dwWaitResult);
                    return STATUS(WI_ERROR);
                }

            #else

                return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);

            #endif
        }
    #endif
    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : wiProcess_ReleaseMutex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Release the mutex obtained with wiProcess_WaitForMutex()
// ================================================================================================
wiStatus wiProcess_ReleaseMutex(wiMutex_t *pMutex)
{
    #ifdef WISE_BUILD_MULTITHREADED

        if (ModuleIsInitialized)
        {
            #if defined WISE_USE_PTHREADS

                int rc = pthread_mutex_unlock( pMutex );
                if (rc != 0)
                {
                    wiUtil_WriteLogFile("pthread_mutex_unlock result = %d\n",rc);
                    return WI_ERROR;
                }

            #elif defined WIN32

                if (!ReleaseMutex( *((HANDLE *)pMutex) ) )
                {
                    wiUtil_WriteLogFile("ReleaseMutex failed\n");
                    return STATUS(WI_ERROR);
                }

            #else

                return STATUS(WI_ERROR_NO_MULTITHREAD_SUPPORT);

            #endif
        }
    #endif
    return WI_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

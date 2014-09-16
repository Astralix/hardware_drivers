//--< RvUtil.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  Remote VWAL Utilities
//  Copyright 2007 DSP Group, Inc. All rights reserved.
//
//  Written by Barrett Brickner
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#ifdef WIN32
#include <Winsock.h>
#endif

#include <stdio.h>
#include "RvTypes.h"
#include "RvUtil.h"

#ifdef WIN32
static HANDLE RemoteVWAL_Mutex = NULL;
#endif

static int RvUtilVerboseState = 0;
static RvPrintfFn_t pRvPrintf = printf;  // ptr -> function for RvPrintf()

#define VPRINTF if(RvUtil_Verbose()) RvPrintf

// ================================================================================================
// FUNCTION   : RvUtil_Verbose()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
int RvUtil_Verbose(void)
{
    return RvUtilVerboseState;
}
// end of RvUtil_Verbose()

// ================================================================================================
// FUNCTION   : RvUtil_SetVerbose()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void RvUtil_SetVerbose(int Verbose)
{
    RvUtilVerboseState = Verbose;
}
// end of RvUtil_SetVerbose()

// ================================================================================================
// FUNCTION   : RvUtil_PrintfPtr()
// ------------------------------------------------------------------------------------------------
// Purpose    : Returns a pointer to the registered "printf" function
// ================================================================================================
RvPrintfFn_t RvUtil_PrintfPtr(void)
{
    return pRvPrintf;
}
// end of RvUtil_PrintfPtr()

// ================================================================================================
// FUNCTION   : RvUtil_SetPrintf()
// ------------------------------------------------------------------------------------------------
// Purpose    : Assign a function pointer to be used for RvPrintf calls
// Parameters : pPrintfFn -- Pointer to a function with the prototype of "printf"
// ================================================================================================
RvStatus_t RvUtil_SetPrintf(RvPrintfFn_t pPrintfFn)
{
    pRvPrintf = pPrintfFn ? pPrintfFn : printf;
    return RVWAL_SUCCESS;
}
// end of RvUtil_SetPrintf()

// ================================================================================================
// FUNCTION   : RvUtil_Error_Handler()
// ------------------------------------------------------------------------------------------------
// Purpose    : Error reporting
// Parameters : Status - Status codes as enumerated in RemoteVWAL.h
//              File   - Name of the file where the status was generated
//              Line   - Source code line from which the status was generated
// ================================================================================================
RvStatus_t RvUtil_Error_Handler(RvStatus_t Status, char *File, int Line)
{
    if( Status != RVWAL_SUCCESS )
    {
        RvPrintf("\n");
        RvPrintf(" *** RVWAL ERROR: %d\n",Status);
        RvPrintf(" *** File: %s Line: %d\n",File,Line);
        RvPrintf("\n");
    }
    return Status;
}
// end of RvUtil_Error_Handler()

// ================================================================================================
// FUNCTION   : RvUtil_MutexLock()
// ------------------------------------------------------------------------------------------------
// Purpose    : Set a lock for RVWAL mutual exclusion (intended to manage access to sockets)
// Parameters : none
// ================================================================================================
RvStatus_t RvUtil_MutexLock(void)
{
    #ifdef WIN32

        VPRINTF(" > RvUtil_MutexLock()\n");

        RemoteVWAL_Mutex = CreateMutex( NULL, FALSE, "RemoteVWAL_Mutex" );
        if( !RemoteVWAL_Mutex ) return RVWAL_ERROR;

        switch( WaitForSingleObject( RemoteVWAL_Mutex, 1000 ) )
        {
            case WAIT_OBJECT_0 : // got ownership cleanly
            case WAIT_ABANDONED: // got ownership because last owner died
            {
                VPRINTF(" Locked Remote VWAL Mutex, Handle=0x%08X\n",RemoteVWAL_Mutex);
                break;
            }
            case WAIT_TIMEOUT  : // timeout due to busy connection
            {
                VPRINTF(" ERROR: Unable to Lock Remote VWAL Mutex\n");
                CloseHandle( RemoteVWAL_Mutex );
                RemoteVWAL_Mutex = NULL;
                return RVWAL_ERROR;
                break;
            }
            case WAIT_FAILED   : // some other error
            {
                VPRINTF(" ERROR: WAIT FAILED Unable to Lock Remote VWAL Mutex\n");
                CloseHandle( RemoteVWAL_Mutex );
                RemoteVWAL_Mutex = NULL;
                return RVWAL_ERROR;
                break;
            }
            default:             // should not be another case
            {
                VPRINTF(" ERROR: ??? Unable to Lock Remote VWAL Mutex\n");
                CloseHandle( RemoteVWAL_Mutex );
                RemoteVWAL_Mutex = NULL;
                return RVWAL_ERROR;
            }
        }

    #endif
    return RVWAL_SUCCESS;
}
// end of RVwalMutex_Lock()


// ================================================================================================
// FUNCTION   : RvUtil_MutexUnlock()
// ------------------------------------------------------------------------------------------------
// Purpose    : Release a locked RVWAL mutual exclusion
// Parameters : none
// ================================================================================================
RvStatus_t RvUtil_MutexUnlock(void)
{
    #ifdef WIN32

        VPRINTF(" > RvUtil_MutexUnlock()\n");

        if( RemoteVWAL_Mutex )
        {
            VPRINTF(" Releasing Remote VWAL Mutex, Handle=0x%08X\n",RemoteVWAL_Mutex);
            ReleaseMutex( RemoteVWAL_Mutex );
        }
    
    #endif
    return RVWAL_SUCCESS;
}
// end of RVwalMutex_Unlock()

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

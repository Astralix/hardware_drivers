//
// RvUtil.h -- Remote VWAL Utilities
// Copyright 2007 DSP Group, Inc. All rights reserved.
//

#ifndef __RVUTIL_H
#define __RVUTIL_H

#if defined(__cplusplus) || defined(__cplusplus__)
   extern "C" {
#endif

#include "RvTypes.h"

// ================================================================================================
// RVWAL FUNCTION WRAPPER
// Use this macro around functions returning a RVWAL status code (RvStatus_t) to report errors
// ================================================================================================
#define RVCHECK(Status) (RvUtil_Error_Handler((Status),__FILE__,__LINE__))

//=================================================================================================
//  PRINT TO STDIO
//  Define the RvPrintf function used in place of printf.
//=================================================================================================
#define RvPrintf (RvUtil_PrintfPtr())

RvPrintfFn_t RvUtil_PrintfPtr(void);

int          RvUtil_Verbose(void);
void         RvUtil_SetVerbose(int Verbose);

RvStatus_t   RvUtil_Error_Handler(RvStatus_t Status, char *File, int Line);
RvStatus_t   RvUtil_SetPrintf(RvPrintfFn_t pRvPrintfFn);

RvStatus_t   RvUtil_MutexLock(void);
RvStatus_t   RvUtil_MutexUnlock(void);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
   }
#endif

#endif // __RVUTIL_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

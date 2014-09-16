//
//  wiUtil.h -- WISE Utilitiy Functions and Definitions
//  Copyright 2001 Bermai, 2007-2010 DSP Group, Inc. All rights reserved.
//
//  This module contains various utility functions and macro definitions used in WiSE. The
//  utilities include textual descriptions for status codes in wiError as well as log file
//  management. In addition, an interface for redefing the functions used for WiSE printing and
//  memory management is included. These interfaces are provided to simplify the sharing of code
//  between the stand-alone executable and the MEX library version of WiSE.
//

#ifndef __WIUTIL_H
#define __WIUTIL_H

#include "wiType.h"
#include "wiArray.h"
#include "wiError.h"
#include "wiMemory.h"
#include "wiProcess.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  WISE FUNCTION WRAPPER
//  Use this macro around functions returning a WISE Status code to catch and handle errors.
//  USE: WICHK(function(...));
//
#define WICHK(Status) (wiUtil_ErrorHandler((Status), __FILE__, __LINE__))

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  RANGE CHECKING
//  This macro inplements basic parameter range checking.
//
#define InvalidRange(x, minx, maxx) ((((x)<(minx)) || ((x)>(maxx)))? WI_TRUE:WI_FALSE)

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  GENERIC MACROS
//
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef DEBUG
#define DEBUG wiPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  PRINT TO STDIO
//  Define the wiPrintf function used in place of printf.
//
#define wiPrintf (wiUtil_PrintfPtr())

// Define alias for backward compatibility
//
#define wiUtil_EnableTraceMemoryManagement wiMemory_EnableTraceMemoryManagement

//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================
wiStatus  wiUtil_Init(void);
wiStatus  wiUtil_Close(void);

//---- Error handling -----------------------------------------------------------------------------
wiStatus  wiUtil_ErrorHandler(wiStatus status, wiString file, int line);
const char * wiUtil_ErrorMessage(wiStatus status);
wiStatus  wiUtil_LastStatus(void);

//---- Verbose/debugging --------------------------------------------------------------------------
wiStatus  wiUtil_SetVerbose(wiBoolean verbose);
wiBoolean wiUtil_Verbose(void);
wiStatus  wiUtil_Debug(wiString filename, int line);

//---- Log file functions -------------------------------------------------------------------------
wiStatus  wiUtil_OpenLogFile(wiString filename);
wiStatus  wiUtil_CloseLogFile(void);

wiStatus  wiUtil_WriteLogFile_FileLine(wiString file, int line, const char *format, ...);
int       wiUtil_WriteLogFile(const char *format, ...);

wiStatus  wiUtil_UseLogFile(wiBoolean state);

//---- Save a numerical array to a text file ------------------------------------------------------
wiStatus  dump_wiByte     (wiString filename, wiByte    *x, int n);
wiStatus  dump_wiInt      (wiString filename, wiInt     *x, int n);
wiStatus  dump_wiReal     (wiString filename, wiReal    *x, int n);
wiStatus  dump_wiComplex  (wiString filename, wiComplex *x, int n);
wiStatus  dump_wiWORD     (wiString filename, wiWORD    *x, int n);
wiStatus  dump_wiUWORD    (wiString filename, wiUWORD   *x, int n);
wiStatus  dump_wiWORD_pair(wiString filename, wiWORD    *xI, wiWORD *y, int n);

//---- wiUWORD bitfield access --------------------------------------------------------------------
wiStatus  wiUWORD_SetUnsignedField(wiUWORD *pX, int msb, int lsb, unsigned val);
wiStatus  wiUWORD_SetSignedField  (wiUWORD *pX, int msb, int lsb, int val);
unsigned  wiUWORD_GetUnsignedField(wiUWORD *pX, int msb, int lsb);
int       wiUWORD_GetSignedField  (wiUWORD *pX, int msb, int lsb);

//---- WORD bitfield access -----------------------------------------------------------------------
wiStatus  wiWORD_SetSignedWord   (int value, int high_bit, int low_bit, void *x);

//---- String formatting functions ----------------------------------------------------------------
wiString  wiUtil_Comma(unsigned int x);
wiString  wiUtil_Boolean(wiBoolean x);
wiString  wiUtil_BinStr(int x, int num_bits);

wiBoolean wiUtil_ReceivedSigBreak(void);

//---- Set a pointer to the internal "printf" function -------------------------------------------
wiStatus  wiUtil_SetPrintf(wiPrintfFn  printfFn);

// ---- Function Pointer: Do not call function below; use wiPrintf()
wiPrintfFn wiUtil_PrintfPtr(void);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __WIUTIL_H
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

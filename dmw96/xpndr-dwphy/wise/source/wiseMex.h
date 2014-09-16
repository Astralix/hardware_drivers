//
//  wiseMex.h -- Master header file for MATLAB wiseMex DLL
//  Copyright 2007-2011 DSP Group, Inc. All rights reserved.
//

#ifndef __WISEMEX_H
#define __WISEMEX_H

#include "wiMexUtil.h"
#include "wise.h"

/////////////////////////////////////////////////////////////////////////////////////////
//
//  PREVENT C++ NAME MANGLING
//
//  Although C++ code may be included in the library when it is built, the interface
//  for this library requires that all functions are exported as standard 'C' functions
//
#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
//  ERROR HANDLING/DEBUGGING MACROS
//
#ifndef MCHK
#define MCHK(fn)    wiseMex_ErrorHandler((fn), __FILE__, __LINE__, 0)
#endif

#ifndef XSTATUS
#define XSTATUS(fn) wiseMex_ErrorHandler((fn), __FILE__, __LINE__, 1)
#endif

#ifndef DEBUG
#define DEBUG       wiPrintf("### FILE: %s, LINE: %d\n",__FILE__,__LINE__);
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
//  WRAPPER FOR WISEMEX COMMAND HANDLER CALLS
//
#ifndef WISEMEX_COMMAND
#define WISEMEX_COMMAND(fn)   if (MCHK((fn)) == WI_SUCCESS) return WI_SUCCESS;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
//
// Prototypes for functions in wiseMex.c
//
wiStatus  _fastcall wiseMex_ErrorHandler(wiStatus Status, char *file, int line, int AbortOnError);
wiBoolean wiseMex_AllowSubstitution(void);

/////////////////////////////////////////////////////////////////////////////////////////
//
// PROTOTYPES FOR FUNCTIONS IN wiseMex_*.c
//
// Functions for these headers are typically conditionally included based on which header
// files are included in wise.h.
//
wiStatus wiseMex_CalcEVM  (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_Phy11n   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_DW52     (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_DW74     (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_Mojave   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_Nevada   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_NevadaPHY(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_Tamale   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_TamalePHY(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_RF22     (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_RF52     (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_RF55     (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_Coltrane (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);

wiStatus wiseMex_wiParse  (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_wiTest   (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_wiChanl  (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
wiStatus wiseMex_wiTest_ParseLine(wiString text);
//---------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

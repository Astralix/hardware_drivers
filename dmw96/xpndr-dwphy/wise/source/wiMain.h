//
//  wiMain.h -- WISE Top Level and Entry Point Functions
//  Copyright 2001 Bermai, 2007-2010 DSP Group, Inc. All rights reserved.
//
//  This is the top-level functional code in WiSE. The function wiMain() is called directly
//  from the 'C' function main() using the same arguments. Execution involves initializing 
//  module within WiSE and then parsing a script file containing parameter value assignment 
//  and test execution information.
//
//  To bypass the execution flow through wiMain, modify the function call in the 'C' 
//  function main().

#ifndef __WIMAIN_H
#define __WIMAIN_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================
int      wiMain(int argc, char *argv[]);
wiStatus wiMain_Init(void);
wiStatus wiMain_Close(void);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

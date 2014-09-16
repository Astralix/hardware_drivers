//--< mexLab_AWG520.c >----------------------------------------------------------------------------
//=================================================================================================
//
//  Tektronix AWG520 Ethernet Interface Functions for BERLab
//  Copyright 2007 DSP Group, Inc.
//
//  Developed by Barrett Brickner
//
//=================================================================================================

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include "ViMexUtil.h"
#include "AWG520net.h"


#define MAX_LINES   50
#define LINE_WIDTH 100

// ================================================================================================
// --  LOCAL FILE PARAMETERS
// ================================================================================================
static ViBoolean mexLab_AWG520_Initialized = 0;  // Has everything been initialized?

// ================================================================================================
// --  INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
static void __cdecl mexLab_AWG520_Init(void);
static void __cdecl mexLab_AWG520_Close(void);

// ================================================================================================
// FUNCTION  : mexFunction
// ------------------------------------------------------------------------------------------------
// Purpose   : Entry point for calls from MATLAB
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
void __cdecl mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char *tcmd, cmd[256]; // passed command parameter

    // --------------
    // Check for Init
    // --------------
    if(!mexLab_AWG520_Initialized) mexLab_AWG520_Init();

    // ------------------
    // Dummy library call
    // ------------------
    if(!nrhs && !nlhs) 
    {
        mexPrintf("\n");
        mexPrintf("   MATLAB AWG520 Network Interface (%s %s)\n",__DATE__,__TIME__);
        mexPrintf("   Copyright 2007 DSP Group, Inc.\n");
        mexPrintf("\n");
        return;
    }
    // ----------------------
    // Get the command string
    // ----------------------
    if(nrhs<1) mexErrMsgTxt("A string argument must be given as the first parameter.");
    tcmd = ViMexUtil_GetStringValue(prhs[0],0);
    strncpy(cmd, tcmd, 256); mxFree(tcmd); cmd[255] = 0; // copied and free tcmd as it is not free'd below

    //******************************************
    //*****                                *****
    //*****   COMMAND LOOKUP-AND-EXECUTE   *****
    //*****                                *****
    //******************************************

    if(strstr(cmd,"openAWG520"))
    {
        char *hostname; int retVal;
        ViMexUtil_CheckParameters(nlhs,nrhs,1,2);
        hostname = ViMexUtil_GetStringValue(prhs[1], NULL);
        retVal = openAWG520(hostname);
        ViMexUtil_PutIntValue(retVal, &plhs[0]);
        mxFree(hostname);
        return;
    }
  
    if(!strcmp(cmd,"closeAWG520"))
    {
        ViMexUtil_CheckParameters(nlhs,nrhs,1,1);
        closeAWG520();
        ViMexUtil_PutIntValue(0, &plhs[0]);
        return;
    }
    if(!strcmp(cmd,"writeAWG520"))
    {
        int retVal;
        char *command;

        ViMexUtil_CheckParameters(nlhs,nrhs,1,2);
        command = ViMexUtil_GetStringValue(prhs[1], NULL);
        retVal = writeAWG520(command);
        ViMexUtil_PutIntValue(retVal, &plhs[0]);
        mxFree(command);
        return;
    }
    if(!strcmp(cmd,"readAWG520"))
    {
        int retVal;
        char buf[1024] = {0};

        ViMexUtil_CheckParameters(nlhs,nrhs,2,1);
        retVal = readAWG520(buf,1024);
        ViMexUtil_PutIntValue(retVal, &plhs[0]);
        ViMexUtil_PutString(buf, &plhs[1]);
        return;
    }
    // ******************************************************************
    // --------------------------------------------
    // No Command Found...Generate an Error Message
    // --------------------------------------------
    mexErrMsgTxt("mexLab_AWG520: Unrecognized command in the first argument.");
}
// end of mexFunction()

// ================================================================================================
// FUNCTION  : mexLab_AWG520_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialization of the mexLab_AWG520 library
// Parameters: none
// ================================================================================================
void mexLab_AWG520_Init(void)
{
    mexLab_AWG520_Initialized = 1;

    // -----------------------------------------------
    // Register the close/cleanup function with MATLAB
    // -----------------------------------------------
    mexAtExit(mexLab_AWG520_Close);

    mexLab_AWG520_Initialized = 1;
}
// end of mexLab_AWG520_Init()

// ================================================================================================
// FUNCTION  : mexLab_AWG520_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Shutdown/cleanup the mexLab_AWG520 library
// Parameters: none
// ================================================================================================
void mexLab_AWG520_Close(void)
{
}
// end of mexLab_AWG520_Close()

/*************************************************************************************************/
/*=== END OF SOURCE CODE ========================================================================*/
/*************************************************************************************************/

// Revised 2007-02-07 B.Brickner
// - Removed support for Agilent 16700 Logic Analyzer (RPI)
// - Removed support for parallel port control
// Revised 2007-06-19 B.Brickner: converted mexLab to mexLab_AWG520
// Revised 2007-09-10 B.Brickner
// - Added automatic copy and free of command string

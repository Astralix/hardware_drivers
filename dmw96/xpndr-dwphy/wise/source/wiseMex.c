//--< wiseMex.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  Matlab Executable Library Entry Point for WiSE
//  Copyright 2001-2004 Bermai, 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "wise.h"
#include "wiseMex.h"
#include "wiChanl.h"
#include "wiFilter.h"
#include "wiMAC.h"
#include "wiMain.h"
#include "wiMath.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiRnd.h"
#include "wiUtil.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean wiseMex_Initialized = 0;            // Has everything been initialized?
static wiBoolean wiseMex_AllowSubstitutionState = 1; // Allow wiParse string substitutions

//=================================================================================================
//--  PRIVATE FUNCTION PROTOTYPES
//=================================================================================================
wiStatus wiseMex(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd);
void   wiseMex_Init(int nrhs, const mxArray *prhs[]);
void   wiseMex_Close(void);
void * wiseMex_Malloc (size_t __size);
void * wiseMex_Calloc (size_t __nitems, size_t __size);
void * wiseMex_Realloc(void * __block, size_t __size);
void   wiseMex_Free   (void * __block);
wiStatus wiseMex_ParseLine(wiString text);

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)

// ================================================================================================
// FUNCTION  : mexFunction()
// ------------------------------------------------------------------------------------------------
// Purpose   : Entry point for calls from MATLAB
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
void __cdecl mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    char *cmd;  // passed command parameter

    // --------------
    // Check for Init
    // --------------
    if (!wiseMex_Initialized) wiseMex_Init(nrhs, prhs);

    // ----------------------------
    // Return on dummy library call
    // ----------------------------
    if (!nrhs)
    {
        wiPrintf("\n");
        wiPrintf(" %s -- Wireless Simulation Environment\n",WISE_VERSION_STRING);
        wiPrintf("   Copyright 2001-2004 Bermai, 2006-2011 DSP Group, Inc. All rights reserved.\n");
        wiPrintf("\n");
        return;
    }

    // ----------------------
    // Get the command string
    // ----------------------
    if (nrhs < 1) mexErrMsgTxt("A string argument must be given as the first parameter.");
    cmd = wiMexUtil_GetStringValue(prhs[0], 0);

    /////////////////////////////////////////////////////////////////////////////////////
	//
    //  Call Command Handlers
	//
	if (MCHK( wiseMex(nlhs, plhs, nrhs, prhs, cmd) ) == WI_SUCCESS) 
    {
        mxFree(cmd);
        return;
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////////////
	    //
	    //  No command found...generate an error message
	    //
        mxFree(cmd);
        mexErrMsgTxt("wiseMex did not recognize command in the first argument.");
    }
}
// end of mexFunction()

// ================================================================================================
// FUNCTION  : wiseMex()
// ------------------------------------------------------------------------------------------------
// Purpose   : Lookup-and-execute specific instructions
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- Command string
// ================================================================================================
wiStatus wiseMex(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *CommandStr)
{
    char *cmd = CommandStr;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Skip Initialization Commands
    //
    if (strcmp(cmd, "-Trace") == 0) return WI_SUCCESS;

    /////////////////////////////////////////////////////////////////////////////////////
	//
    //  Perform string substitution specified in wiParse
	//
    if (wiseMex_AllowSubstitution())
    {
        MCHK( wiParse_SubstituteStrings(&cmd) );
    }

    /////////////////////////////////////////////////////////////////////////////////////
	//
    //  Call Command Handlers
	//
	WISEMEX_COMMAND( wiseMex_wiTest (nlhs, plhs, nrhs, prhs, cmd) );
	WISEMEX_COMMAND( wiseMex_wiChanl(nlhs, plhs, nrhs, prhs, cmd) );

    /////////////////////////////////////////////////////////////////////////////////////
	//
    //  PHY-Specific Module Functions
	//
	//  These are conditionally called based on whether the PHY module header file
	//  is included (typically in wise.h).
	//
    #ifdef __DW52_H
        WISEMEX_COMMAND( wiseMex_DW52     (nlhs, plhs, nrhs, prhs, cmd) );
    #endif
    #ifdef __DW74_H
        WISEMEX_COMMAND( wiseMex_DW74     (nlhs, plhs, nrhs, prhs, cmd) );
    #endif
    #ifdef __CALCEVM_H
        WISEMEX_COMMAND( wiseMex_CalcEVM  (nlhs, plhs, nrhs, prhs, cmd) );
    #endif
    #ifdef __NEVADAPHY_H
        WISEMEX_COMMAND( wiseMex_NevadaPHY(nlhs, plhs, nrhs, prhs, cmd) );
    #endif
    #ifdef __TAMALEPHY_H
        WISEMEX_COMMAND( wiseMex_TamalePHY(nlhs, plhs, nrhs, prhs, cmd) );
    #endif
    #ifdef __PHY11N_H
        WISEMEX_COMMAND( wiseMex_Phy11n(nlhs, plhs, nrhs, prhs, cmd) );
    #endif

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiFilter_
	//
    if (strstr(cmd,"wiMath_"))
    {
        if COMMAND("wiMath_FletcherChecksum")
        {
            wiUInt *x, Z; int N;
            wiMexUtil_CheckParameters(nlhs, nrhs, 1, 3);
            x = wiMexUtil_GetUIntArray(prhs[1], &N);
            Z = wiMexUtil_GetUIntValue(prhs[2]);

            wiMath_FletcherChecksum(N*sizeof(wiUInt), x, &Z);
            wiMexUtil_PutUIntValue(Z, &plhs[0]);
            return WI_SUCCESS;
        }
    }
            
    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiFilter_
	//
    if (strstr(cmd,"wiFilter_"))
    {
        if COMMAND("wiFilter_Butterworth")
        {
            int N, method; wiReal *b, *a, fc;
            wiMexUtil_CheckParameters(nlhs,nrhs,2,4);
            N      = wiMexUtil_GetIntValue (prhs[1]);
            fc     = wiMexUtil_GetRealValue(prhs[2]);
            method = wiMexUtil_GetIntValue (prhs[3]);
            a  = (wiReal *)mxCalloc(N+1, sizeof(wiReal));
            b  = (wiReal *)mxCalloc(N+1, sizeof(wiReal));

            MCHK(wiFilter_Butterworth(N, fc, b, a, method));
            wiMexUtil_PutRealArray(N+1, b, &plhs[0]);
            wiMexUtil_PutRealArray(N+1, a, &plhs[1]);
            mxFree(b); mxFree(a);

            return WI_SUCCESS;
        }

        if COMMAND("wiFilter_IIR")
        {
            int N, Nx;
            wiReal *a, *b, *x, *y, *w;
            wiMexUtil_CheckParameters(nlhs,nrhs,1,6);
            N = wiMexUtil_GetIntValue (prhs[1]);
            b = wiMexUtil_GetRealArray(prhs[2], NULL);
            a = wiMexUtil_GetRealArray(prhs[3], NULL);
            x = wiMexUtil_GetRealArray(prhs[4], &Nx );
            w = wiMexUtil_GetRealArray(prhs[5], NULL);
            y = (wiReal *)mxCalloc(Nx, sizeof(wiReal));

            MCHK(wiFilter_IIR(N, b, a, Nx, x, y, w));
            wiMexUtil_PutRealArray(Nx, y, &plhs[0]);
            mxFree(w); mxFree(y); mxFree(x); mxFree(a); mxFree(b);

            return WI_SUCCESS;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiMAC_
	//
    if (strstr(cmd, "wiMAC_"))
    {
        if COMMAND("wiMAC_DataFrame")
        {
            int nBytes, FrameBodyType, Broadcast;
            wiUWORD PSDU[4095];

            wiMexUtil_CheckParameters(nlhs,nrhs,1,4);
            nBytes        = wiMexUtil_GetIntValue (prhs[1]);
            FrameBodyType = wiMexUtil_GetIntValue (prhs[2]);
            Broadcast     = wiMexUtil_GetIntValue (prhs[3]);

            MCHK( wiMAC_DataFrame  (nBytes, PSDU, FrameBodyType, Broadcast) );
            wiMexUtil_PutUWORDArray(nBytes, PSDU, &plhs[0]);

            return WI_SUCCESS;
        }

		if COMMAND("wiMAC_AMPDUDataFrame")
        {
            int nBytes, FramebodyType, Broadcast;
            wiUWORD PSDU[65535]; 

            wiMexUtil_CheckParameters(nlhs,nrhs,1,4);
            nBytes        = wiMexUtil_GetIntValue (prhs[1]);
            FramebodyType = wiMexUtil_GetIntValue (prhs[2]);
            Broadcast     = wiMexUtil_GetIntValue (prhs[3]);
				
            MCHK( wiMAC_AMPDUDataFrame (nBytes, PSDU, FramebodyType, Broadcast) );
    		wiMexUtil_PutUWORDArray(nBytes, PSDU, &plhs[0]);

            return WI_SUCCESS;
        }

		if COMMAND("wiMAC_AMSDUDataFrame")
        {
            int nBytes, FramebodyType, Broadcast;
            wiUWORD PSDU[65535]; 

            wiMexUtil_CheckParameters(nlhs,nrhs,1,4);
            nBytes        = wiMexUtil_GetIntValue (prhs[1]);
            FramebodyType = wiMexUtil_GetIntValue (prhs[2]);
            Broadcast     = wiMexUtil_GetIntValue (prhs[3]);
				
            MCHK( wiMAC_AMSDUDataFrame (nBytes, PSDU, FramebodyType, Broadcast) );
    		wiMexUtil_PutUWORDArray(nBytes, PSDU, &plhs[0]);

            return WI_SUCCESS;
        }

        if COMMAND("wiMAC_SetDataBuffer")
        {
            wiUWORD *DataBuffer;
            int BufferLength;
            char SetLength[256];

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);

            DataBuffer = wiMexUtil_GetUWORDArray(prhs[1], &BufferLength);
            MCHK( wiMAC_SetDataBuffer(BufferLength, DataBuffer) );
            
            sprintf(SetLength,"wiTest.Length = %d",BufferLength + 28);

            MCHK( wiParse_Line("wiTest.DataType = 8") );
            MCHK( wiParse_Line(SetLength) );

            mxFree(DataBuffer);
            return WI_SUCCESS;
        }
    }
	
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiMain_
	//
    if (strstr(cmd, "wiMain_"))
    {
        if COMMAND("wiMain_VersionString")
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiMexUtil_PutString(WISE_VERSION_STRING, &plhs[0]);
            return WI_SUCCESS;
        }
    }

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiParse_
	//
    if (strstr(cmd, "wiParse_"))
    {

        if COMMAND("wiParse_ScriptFile")
        {
            wiString filename;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
            filename = wiMexUtil_GetStringValue(prhs[1], NULL);

			if (MCHK(wiParse_ScriptFile(filename)) < WI_SUCCESS)
			{
				char *msg = mxMalloc( sizeof(filename) + 80 );
				sprintf(msg, "wiseMex error processing file \"%s\"",filename);
				mexErrMsgTxt(msg); 
			}
            mxFree(filename);

            return WI_SUCCESS;
        }

        if COMMAND("wiParse_Line")
        {
            wiString text;
            wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
            text = wiMexUtil_GetStringValue(prhs[1], NULL);

			if (MCHK( wiParse_Line (text) ) != WI_SUCCESS)
			{
				char *msg = mxMalloc( sizeof(text) + 80);
				sprintf(msg,  "wiseMex error parsing text \"%s\"",text);
				mexErrMsgTxt(msg);
			}
            mxFree(text);
            return WI_SUCCESS;
        }

        if COMMAND("wiParse_ResumeScript")
        {
            wiMexUtil_CheckParameters(nlhs, nrhs, 0, 1);
            MCHK( wiParse_ResumeScript() );
            return WI_SUCCESS;
        }
        if COMMAND("wiParse_ExitScript")
        {
            wiMexUtil_CheckParameters(nlhs, nrhs, 0, 1);
            MCHK( wiParse_ExitScript() );
            return WI_SUCCESS;
        }
        if COMMAND("wiParse_PauseScript")
        {
            wiMexUtil_CheckParameters(nlhs, nrhs, 0, 1);
            MCHK( wiParse_PauseScript() );
            return WI_SUCCESS;
        }
        if COMMAND("wiParse_ScriptIsPaused")
        {
            wiMexUtil_CheckParameters(nlhs, nrhs, 1, 1);
            wiMexUtil_PutIntValue( wiParse_ScriptIsPaused(), &plhs[0] );
            return WI_SUCCESS;
        }
        if COMMAND("wiParse_Substitute")
        {
            char *text, *cptr;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            cptr = text = wiMexUtil_GetStringValue(prhs[1], NULL);

            MCHK( wiParse_SubstituteStrings(&cptr) );
            wiMexUtil_PutString( cptr, &plhs[0] );
            mxFree(text);
            return WI_SUCCESS;
        }
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiPHY_
	//
    if (strstr(cmd, "wiPHY_"))
    {
        if COMMAND("wiPHY_TX")
        {
            int nTX, Ns,  DataRate, Length;
            wiUWORD *TxData;
            wiReal Prms, Fs;
            wiComplex **S = 0;

            wiMexUtil_CheckParameters(nlhs,nrhs,3,3);

            DataRate = wiMexUtil_GetIntValue (prhs[1]);
            TxData   = wiMexUtil_GetUWORDArray(prhs[2], &Length);

            MCHK( wiPHY_TX(DataRate, Length, TxData, &Ns, &S, &Fs, &Prms) );

            if (S)
            {
                for (nTX=0; (nTX<WICHANL_TXMAX) && S[nTX]; nTX++) ; // count the number of transmit streams
                wiMexUtil_PutComplexMatrix(nTX, Ns, S, &plhs[0]);
                wiMexUtil_PutRealValue(Fs,   &plhs[1]);
                wiMexUtil_PutRealValue(Prms, &plhs[2]);
            }
            mxFree(TxData);
            return WI_SUCCESS;
        }

        if COMMAND("wiPHY_RX")
        {
            wiInt i, Nr, Nd, nRX;
            wiComplex **R = 0;
            wiUWORD *PHY_RX_D;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            R = wiMexUtil_GetComplexMatrix(prhs[1], &Nr, &nRX);

            MCHK(wiPHY_RX(&Nd, &PHY_RX_D, Nr, R) );

			wiMexUtil_PutUWORDArray(max(0,Nd), PHY_RX_D, &plhs[0]);
            for (i=0; i<nRX; i++) if (R[i]) mxFree(R[i]);
            if (R) mxFree(R);
            return WI_SUCCESS;
        }

        if COMMAND("wiPHY_WriteRegister")
        {
            wiUInt Addr, Data;
            wiMexUtil_CheckParameters(nlhs, nrhs, 0, 3);
            
            Addr = wiMexUtil_GetUIntValue(prhs[1]);
            Data = wiMexUtil_GetUIntValue(prhs[2]);

            MCHK(wiPHY_WriteRegister(Addr, Data));
            return WI_SUCCESS;
        }

        if COMMAND("wiPHY_ReadRegister")
        {
            wiUInt Addr, Data;
            wiMexUtil_CheckParameters(nlhs, nrhs, 1, 2);
            
            Addr = wiMexUtil_GetUIntValue(prhs[1]);

            MCHK(wiPHY_ReadRegister(Addr, &Data));
            wiMexUtil_PutUIntValue(Data, &plhs[0]);
            return WI_SUCCESS;
        }

        if COMMAND("wiPHY_ActiveMethod")
        {
            wiMexUtil_CheckParameters(nlhs, nrhs, 1, 1);
            wiMexUtil_PutString( wiPHY_ActiveMethodName(), &plhs[0] );
            return WI_SUCCESS;
        }
    }

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiRnd_
	//
    if (strstr(cmd, "wiRnd_"))
    {
        if COMMAND("wiRnd_Init")
        {
            int Seed; unsigned int Step[2];
            wiMexUtil_CheckParameters(nlhs,nrhs,0,4);
            Seed    = wiMexUtil_GetIntValue(prhs[1]);
            Step[0] = wiMexUtil_GetUIntValue(prhs[2]);
            Step[1] = wiMexUtil_GetUIntValue(prhs[3]);
            MCHK( wiRnd_Init(Seed, Step[0], Step[1]) );
            return WI_SUCCESS;
        }
        if COMMAND("wiRnd_GetState")
        {
            unsigned int state[3];
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiRnd_GetState( (int *)(state+0), state+1, state+2 );
            wiMexUtil_PutUIntArray(3, state, &plhs[0]);
            return WI_SUCCESS;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiUtil_
	//
    if (strstr(cmd, "wiUtil_"))
    {
        if COMMAND("wiUtil_LastStatus")
        {
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiMexUtil_PutIntValue( wiUtil_LastStatus(), &plhs[0] );
            return WI_SUCCESS;
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiseMex_
	//
    if (strstr(cmd, "wiseMex_"))
    {
        if COMMAND("wiseMex_RxPacket") // this function is deprecated; use wiPHY_RX instead
        {
			mexErrMsgTxt(" wiseMex_RxPacket() is obsolete: use wiPHY_RX instead.");
        }
        if (COMMAND("wiseMex_AllowSubstitutions") || COMMAND("wiseMex_AllowSubstitution"))
        {
            if (nlhs > 0) wiMexUtil_PutIntValue(wiseMex_AllowSubstitutionState, &plhs[0]);
            if (nrhs > 1) wiseMex_AllowSubstitutionState = wiMexUtil_GetIntValue(prhs[1]);
            return WI_SUCCESS;
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    if COMMAND("WISE_BUILD_VERSION")
    {
        wiMexUtil_CheckParameters(nlhs, nrhs, 1, 1);
        wiMexUtil_PutUIntValue(WISE_BUILD_VERSION, &plhs[0]);
        return WI_SUCCESS;
    }
    if COMMAND("WISE_VERSION_STRING")
    {
        wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        wiMexUtil_PutString(WISE_VERSION_STRING, &plhs[0]);
        return WI_SUCCESS;
    }

    if (cmd != CommandStr)
        wiUtil_WriteLogFile("wiParse substitution: \"%s\" --> \"%s\"\n",CommandStr,cmd);

    wiUtil_WriteLogFile("wiseMex unrecognized command: \"%s\"\n",cmd);
	
    return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex()

// ================================================================================================
// FUNCTION  : wiseMex_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the wiseMex/WiSE Libraries
// ================================================================================================
void wiseMex_Init(int nrhs, const mxArray *prhs[])
{
    time_t t; t = time(0);

    if (nrhs == 1)
    {
        char *cmd = wiMexUtil_GetStringValue(prhs[0],0);

        if (strcmp(cmd, "-Trace") == 0) 
            WICHK( wiUtil_EnableTraceMemoryManagement() );

        mxFree(cmd);
    }

    wiUtil_WriteLogFile(
        " ----------------------------------------------------------------------------\n"
        " %s MATLAB Toolbox starting at %s", WISE_VERSION_STRING, ctime(&t) );

    wiUtil_SetPrintf(mexPrintf); // Register the "print to console" function

    // Initialize the WiSE modules
    //
    WICHK( wiMain_Init() );   
    WICHK( wiParse_AddMethod(wiseMex_ParseLine) );
    WICHK( wiParse_AddMethod(wiseMex_wiTest_ParseLine) );

    mexAtExit(wiseMex_Close); // Register the close/cleanup function with MATLAB

    wiseMex_Initialized = 1;
}
// end of wiseMex_Init()

// ================================================================================================
// FUNCTION  : wiseMex_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Shutdown/close the wiseMex/WiSE Library
// ================================================================================================
void wiseMex_Close(void)
{
    time_t t = time(0);

    wiMexUtil_ResetMsgFunc();
    WICHK( wiParse_RemoveMethod(wiseMex_wiTest_ParseLine) );
    WICHK( wiParse_RemoveMethod(wiseMex_ParseLine) );
    WICHK( wiMain_Close() );

    wiUtil_WriteLogFile(" %s MATLAB Toolbox ending at %s"
                " -----------------------------------------------------------------------------\n",
                WISE_VERSION_STRING, ctime(&t));
    wiUtil_CloseLogFile();
}
// end of wiseMex_Close()

// ================================================================================================
// FUNCTION  : wiseMex_ErrorHandler()
// ------------------------------------------------------------------------------------------------
// Purpose   : Handle errors from function in wiseMex or WiSE
// ================================================================================================
wiStatus __fastcall wiseMex_ErrorHandler(wiStatus Status, char *file, int line, int AbortOnError)
{
	if (Status < 0)
    {
        const char *errMsg = wiUtil_ErrorMessage(Status);

        mexPrintf("\n");
        mexPrintf("??? wiseMex 0x%08X: %s\n", Status, errMsg);
		mexPrintf("??? File: %s, Line: %d\n", file, line);
        mexPrintf("\n");

        wiUtil_WriteLogFile_FileLine(file, line, "0x%08X: %s\n",Status, errMsg);

		if (AbortOnError) mexErrMsgTxt("wiseMex Error");
    }
    return Status;
}
// end of wiseMex_ErrorHandler()

// ================================================================================================
// FUNCTION  : wiseMex_Malloc()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wrapper for MEX memory allocation used for core WiSE functions
// ================================================================================================
void *wiseMex_Malloc(size_t size)
{
    void *ptr = mxMalloc(size);
    if (ptr) mexMakeMemoryPersistent(ptr);
    return ptr;
}
// end of wiseMex_Malloc

// ================================================================================================
// FUNCTION  : wiseMex_Calloc()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wrapper for MEX memory allocation used for core WiSE functions
// ================================================================================================
void *wiseMex_Calloc(size_t nitems, size_t size)
{
    void *ptr = mxCalloc(nitems, size);
    if (ptr) mexMakeMemoryPersistent(ptr);
    return ptr;
}
// end of wiseMex_Calloc

// ================================================================================================
// FUNCTION  : wiseMex_Realloc()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wrapper for MEX memory allocation used for core WiSE functions
// ================================================================================================
void *wiseMex_Realloc(void *block, size_t size)
{
    void *ptr = mxRealloc(block, size);
    if (ptr) mexMakeMemoryPersistent(ptr);
    return ptr;
}
// end of wiseMex_Realloc

// ================================================================================================
// FUNCTION  : wiseMex_Free()
// ------------------------------------------------------------------------------------------------
// Purpose   : Wrapper for MEX memory allocation used for core WiSE functions
// ================================================================================================
void wiseMex_Free(void *block)
{
    mxFree(block);
}
// end of wiseMex_Free

// ================================================================================================
// FUNCTION  : wiseMex_AllowSubstitution()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiBoolean wiseMex_AllowSubstitution(void)
{
    return wiseMex_AllowSubstitutionState;
}
// end of wiseMex_Free

// ================================================================================================
// FUNCTION : wiseMex_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiseMex_ParseLine(wiString text)
{
    char buf[256]; wiStatus status;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  mexCallMATLAB
    //
    //  Call an arbitrary MATLAB function with no parameters and a single integer return 
    //  code. The return code should be 0 for success, > 0 for a warning, and < 0 for an 
    //  error that cause script execution to stop.
    //
    if ((status = WICHK(wiParse_Function(text,"mexCallMATLAB(%255s)",buf))) == WI_SUCCESS)
    {
        mxArray *X;
        int ReturnStatus;

        mexCallMATLAB(1, &X, 0, NULL, buf);
        ReturnStatus = wiMexUtil_GetIntValue(X);
        if (ReturnStatus < 0) 
        {
            wiPrintf(           "mexCallMATLAB(\"%s\") returned status %d\n",buf,ReturnStatus);
            wiUtil_WriteLogFile("mexCallMATLAB(\"%s\") returned status %d\n",buf,ReturnStatus);
            return WI_ERROR_RETURNED;
        }
        else if (ReturnStatus > 0)
        {
            wiPrintf(           "mexCallMATLAB(\"%s\") returned status %d\n",buf,ReturnStatus);
            wiUtil_WriteLogFile("mexCallMATLAB(\"%s\") returned status %d\n",buf,ReturnStatus);
        }
        return WI_SUCCESS;
    }
    else if (status < WI_SUCCESS) return WI_ERROR_RETURNED;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  mexEvalString
    //
    //  Evaluate an arbitrary string in the MATLAB workspace
    //
    if ((status = WICHK(wiParse_Function(text,"mexEvalString(%255s)",buf))) == WI_SUCCESS)
    {
        mexEvalString(buf);
        return WI_SUCCESS;
    }
    else if (status < WI_SUCCESS) return WI_ERROR_RETURNED;

    return WI_WARN_PARSE_FAILED;
}
// end of wiseMex_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

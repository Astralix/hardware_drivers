//--< wiseMex_Coltrane.c >-----------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the Coltrane Radio Model
//  Copyright 2009-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))

// ================================================================================================
// FUNCTION  : wiseMex_Coltrane()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
wiStatus wiseMex_Coltrane(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
    #ifdef COLTRANE_IS_RF55 ////// String replacement RF55 --> Coltrane

        char NewCmd[256];
        if (strstr(cmd, "RF55_") && (strlen(cmd) < 240)) 
        {
            sprintf(NewCmd, "Coltrane_%s", strstr(cmd,"RF55_") + 5);
            cmd = NewCmd;
        }

    #endif

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Coltrane_
	//
    if (strstr(cmd, "Coltrane_")) ////////////////////////////////////////////////////////////
    {
		#ifdef __COLTRANE_H

			//---------------------------------------------------------------------------
			if COMMAND("Coltrane_GetRxState")
			{
				const char *field_names[] = {"ModelLevel","Fs_MHz","N","trace"};
				const mwSize dims[2] = {1,1};
				int nfields = (sizeof(field_names)/sizeof(*field_names));

				Coltrane_TxState_t *pTX; Coltrane_RxState_t *pRX;
				wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
				MCHK(Coltrane_StatePointer(&pTX, &pRX));
            
				plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
				if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
            
				mxSetField(plhs[0],0,"ModelLevel",mxCreateDoubleScalar(pRX->ModelLevel));
				mxSetField(plhs[0],0,"Fs_MHz",    mxCreateDoubleScalar(pRX->Fs_MHz));
				mxSetField(plhs[0],0,"N",         mxCreateDoubleScalar(pRX->Ntrace));
				if (pRX->trace) mxSetField(plhs[0],0,"trace", wiMexUtil_CreateComplexArray(pRX->Ntrace, pRX->trace));

				return WI_SUCCESS;
			}
			//---------------------------------------------------------------------------
			if COMMAND("Coltrane_WriteConfig")
			{
				Coltrane_WriteConfig(wiMexUtil_Output);
				return WI_SUCCESS;
			}

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: Coltrane_ command received but Coltrane.h not included.");

		#endif // __Coltrane_H
    }

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_Coltrane()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


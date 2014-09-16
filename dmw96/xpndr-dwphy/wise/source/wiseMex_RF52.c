//--< wiseMex_RF52.c >-----------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the RF52 Radio Model
//  Copyright 2008 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))

// ================================================================================================
// FUNCTION  : wiseMex_RF52()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
wiStatus wiseMex_RF52(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  RF52_
	//
    if (strstr(cmd, "RF52_")) ////////////////////////////////////////////////////////////
    {

		#ifdef __RF52_H

			//---------------------------------------------------------------------------
			if COMMAND("RF52_GetRxState")
			{
				const char *field_names[] = {"ModelLevel","Fs_MHz","N","trace"};
				int dims[2] = {1,1};
				int nfields = (sizeof(field_names)/sizeof(*field_names));

				RF52_TxState_t *pTX; RF52_RxState_t *pRX;
				wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
				MCHK(RF52_StatePointer(&pTX, &pRX));
            
				plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
				if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
            
				mxSetField(plhs[0],0,"ModelLevel",mxCreateDoubleScalar(pRX->ModelLevel));
				mxSetField(plhs[0],0,"Fs_MHz",    mxCreateDoubleScalar(pRX->Fs_MHz));
				mxSetField(plhs[0],0,"N",         mxCreateDoubleScalar(pRX->Ntrace));
				if (pRX->trace) mxSetField(plhs[0],0,"trace", wiMexUtil_CreateComplexArray(pRX->Ntrace, pRX->trace));

				return WI_SUCCESS;
			}
			//---------------------------------------------------------------------------
			if COMMAND("RF52_WriteConfig")
			{
				RF52_WriteConfig(&mexPrintf);
				return WI_SUCCESS;
			}

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: RF52_ command received but RF52.h not included.");

		#endif // __RF52_H
    }

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_RF52()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


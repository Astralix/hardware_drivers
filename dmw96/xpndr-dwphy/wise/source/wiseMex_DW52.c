//--< wiseMex_DW52.c >-----------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the DW52 PHY method
//  Copyright 2007-2008 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))

// ================================================================================================
// FUNCTION  : wiseMex_DW52()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- wiseMex command string
// ================================================================================================
wiStatus wiseMex_DW52( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Call command handlers for modules associated with this PHY type
	//
	WISEMEX_COMMAND( wiseMex_Mojave(nlhs, plhs, nrhs, prhs, cmd) );
	WISEMEX_COMMAND( wiseMex_RF52  (nlhs, plhs, nrhs, prhs, cmd) );

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  DW52_
	//
    if (strstr(cmd, "DW52_"))
    {
		#ifdef __DW52_H

			//---------------------------------------------------------------------------
			if COMMAND("DW52_GetState")
			{
				const char *field_names[] = {"M","NsTX","NsRX","sTX","sRX0"};
				int dims[2] = {1,1};
				int nfields = (sizeof(field_names)/sizeof(*field_names));

				DW52_State *pState;
				wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
				MCHK(DW52_StatePointer(&pState));
            
				plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
				if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
            
				mxSetField(plhs[0],0,"M",   mxCreateDoubleScalar(pState->M));
				mxSetField(plhs[0],0,"NsTX",mxCreateDoubleScalar(pState->TX.Ns));
				mxSetField(plhs[0],0,"NsRX",mxCreateDoubleScalar(pState->RX.Ns));
				if (pState->TX.s[0]) mxSetField(plhs[0],0,"sTX", wiMexUtil_CreateComplexArray(pState->TX.Ns, pState->TX.s[0]));
				if (pState->RX.s[0]) mxSetField(plhs[0],0,"sRX0",wiMexUtil_CreateComplexArray(pState->RX.Ns, pState->RX.s[0]));

				return WI_SUCCESS;
			}
			//---------------------------------------------------------------------------
			if COMMAND("DW52_LoadRxADC")
			{
				wiComplex *x; int N;
				wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
				x = wiMexUtil_GetComplexArray(prhs[1], &N);
				MCHK(DW52_LoadRxADC_from_Array(N, x));
				if (x) mxFree(x);

				return WI_SUCCESS;
			}

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: DW52_ command received but DW52.h not included.");

		#endif // __DW52_H
    }

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_DW52()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


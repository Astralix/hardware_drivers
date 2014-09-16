//--< wiseMex_TamalePHY.c >------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the TamalePHY method
//  Copyright 2007-2009 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)


// ================================================================================================
// FUNCTION  : wiseMex_TamalePHY()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
wiStatus wiseMex_TamalePHY(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Call command handlers for modules associated with this PHY type
	//
	WISEMEX_COMMAND( wiseMex_Tamale  (nlhs, plhs, nrhs, prhs, cmd) );
	WISEMEX_COMMAND( wiseMex_Coltrane(nlhs, plhs, nrhs, prhs, cmd) );

	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  TamalePHY_
	//
    if (strstr(cmd, "TamalePHY_") || strstr(cmd, "TamalePhy_"))
    {
	#ifdef __TAMALEPHY_H

		//---------------------------------------------------------------------------
		if (COMMAND("TamalePHY_GetState") || COMMAND("TamalePHY_State")
        ||  COMMAND("TamalePhy_GetState") || COMMAND("TamalePhy_State"))
		{
			const char *field_names[] = {"M","NsTX","NsRX","sTX","sRX0","AV"};
            const char *AV_field_names[] = {"Checker","Limit","LimitLow","LimitHigh",
                "Parameter","ParamLow","ParamHigh","ParamStep"};
			const mwSize dims[2] = {1,1};
			int nfields = (sizeof(field_names)/sizeof(*field_names));
            int mfields = (sizeof(AV_field_names)/sizeof(*AV_field_names));
    		mxArray *pAV;

			TamalePHY_State_t *pState = TamalePHY_State();
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
        
			plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
    		pAV     = mxCreateStructArray(2, dims, mfields, AV_field_names);
			if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
        
			mxSetField(plhs[0],0,"M",   mxCreateDoubleScalar(pState->M));
			mxSetField(plhs[0],0,"NsTX",mxCreateDoubleScalar(pState->TX.Ns));
			mxSetField(plhs[0],0,"NsRX",mxCreateDoubleScalar(pState->RX.Ns));
			if (pState->TX.s[0]) mxSetField(plhs[0],0,"sTX", wiMexUtil_CreateComplexArray(pState->TX.Ns, pState->TX.s[0]));
			if (pState->RX.s[0]) mxSetField(plhs[0],0,"sRX0",wiMexUtil_CreateComplexArray(pState->RX.Ns, pState->RX.s[0]));

    		mxSetField(plhs[0],0,"AV", pAV);
            mxSetField(pAV,0, "Checker",  mxCreateString      (pState->AV.Checker) );
    		mxSetField(pAV,0, "Limit",    mxCreateDoubleScalar(pState->AV.Limit) );
    		mxSetField(pAV,0, "LimitLow", mxCreateDoubleScalar(pState->AV.LimitLow) );
    		mxSetField(pAV,0, "LimitHigh",mxCreateDoubleScalar(pState->AV.LimitHigh) );
            
			return WI_SUCCESS;
		}
		//---------------------------------------------------------------------------
		if (COMMAND("TamalePHY_GetParameter") || COMMAND("TamalePhy_GetParameter"))
		{
            char *Param;
			TamalePHY_State_t *pState = TamalePHY_State();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Param = wiMexUtil_GetStringValue(prhs[1],0);

            if      MATCH(Param,"M"              ) wiMexUtil_PutIntValue    (pState->M,              &plhs[0]);
            else if MATCH(Param,"RxValidFCS"     ) wiMexUtil_PutBooleanValue(pState->RxValidFCS,     &plhs[0]);
            else if MATCH(Param,"dumpTX"         ) wiMexUtil_PutBooleanValue(pState->dumpTX,         &plhs[0]);
            else if MATCH(Param,"dumpRX"         ) wiMexUtil_PutBooleanValue(pState->dumpRX,         &plhs[0]);
            else if MATCH(Param,"ShortPreamble"  ) wiMexUtil_PutBooleanValue(pState->ShortPreamble,  &plhs[0]);
            else if MATCH(Param,"TXPWRLVL"       ) wiMexUtil_PutUIntValue   (pState->TXPWRLVL.word,  &plhs[0]);
            else if MATCH(Param,"TxAggregate"    ) wiMexUtil_PutBooleanValue(pState->TxAggregate,    &plhs[0]);
            else if MATCH(Param,"TxGreenfield"   ) wiMexUtil_PutBooleanValue(pState->TxGreenfield,   &plhs[0]);
            else if MATCH(Param,"TxOpProtect"    ) wiMexUtil_PutBooleanValue(pState->TxOpProtect,    &plhs[0]);
            else if MATCH(Param,"AV.Checker"     ) wiMexUtil_PutString      (pState->AV.Checker,     &plhs[0]);
            else if MATCH(Param,"AV.Limit"       ) wiMexUtil_PutRealValue   (pState->AV.Limit,       &plhs[0]);
            else if MATCH(Param,"AV.LimitLow"    ) wiMexUtil_PutRealValue   (pState->AV.LimitLow,    &plhs[0]);
            else if MATCH(Param,"AV.LimitHigh"   ) wiMexUtil_PutRealValue   (pState->AV.LimitHigh,   &plhs[0]);
            else if MATCH(Param,"AV.N"           ) wiMexUtil_PutIntValue    (pState->AV.N,           &plhs[0]);
            else if MATCH(Param,"AV.Parameter"   ) wiMexUtil_PutString      (pState->AV.Parameter,   &plhs[0]);
            else if MATCH(Param,"AV.ParamLow"    ) wiMexUtil_PutRealValue   (pState->AV.ParamLow,    &plhs[0]);
            else if MATCH(Param,"AV.ParamHigh"   ) wiMexUtil_PutRealValue   (pState->AV.ParamHigh,   &plhs[0]);
            else if MATCH(Param,"AV.ParamStep"   ) wiMexUtil_PutRealValue   (pState->AV.ParamStep,   &plhs[0]);
            else if MATCH(Param,"AV.UIntValue"   ) wiMexUtil_PutUIntArray   (8, pState->AV.UIntValue,&plhs[0]);
            else if MATCH(Param,"Checksum.Enable") wiMexUtil_PutBooleanValue(pState->Checksum.Enable,&plhs[0]);
            else if MATCH(Param,"Checksum.Result") wiMexUtil_PutUIntValue   (pState->Checksum.Result,&plhs[0]);
            else if MATCH(Param,"Checksum.TX"    ) wiMexUtil_PutUIntValue   (pState->Checksum.TX,    &plhs[0]);
            else if MATCH(Param,"Checksum.RX"    ) wiMexUtil_PutUIntValue   (pState->Checksum.RX,    &plhs[0]);
            else if MATCH(Param,"TX.s"           ) wiMexUtil_PutComplexArray(pState->TX.Ns, pState->TX.s[0], &plhs[0]);
            else if MATCH(Param,"RX.s"           ) wiMexUtil_PutComplexArray(pState->RX.Ns, pState->RX.s[0], &plhs[0]);
            else 
                mexErrMsgTxt("wiTest_GetParameter: parameter not available");

            mxFree(Param);

            return WI_SUCCESS;
        }
		//---------------------------------------------------------------------------
		if COMMAND("TamalePHY_LoadRxADC")
		{
			wiComplex *x; int N;
			wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
			x = wiMexUtil_GetComplexArray(prhs[1], &N);
			MCHK(TamalePHY_LoadRxADC_from_Array(N, x));
			if (x) mxFree(x);

			return WI_SUCCESS;
		}
		//---------------------------------------------------------------------------
		if (COMMAND("TamalePHY_WriteConfig") || COMMAND("TamalePhy_WriteConfig"))
		{
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");
			mexPrintf(" wiseMex (%s): TamalePHY Configuration\n",__DATE__);
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");
			MCHK( TamalePHY_WriteConfig(wiMexUtil_Output) );
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");

			return WI_SUCCESS;
		}
		//---------------------------------------------------------------------------
		if (COMMAND("TamalePHY_AV_Checker") || COMMAND("TamalePhy_AV_Checker"))
		{
			TamalePHY_State_t *pState = TamalePHY_State();
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiMexUtil_PutString(pState->AV.Checker, &plhs[0]);

            return WI_SUCCESS;
        }

	#else // no header loaded; therefore, no functions to call

		mexErrMsgTxt("wiseMex: TamalePHY_ command received but TamalePHY.h not included.");

	#endif // __TAMALEPHY_H
    }

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_TamalePHY()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------


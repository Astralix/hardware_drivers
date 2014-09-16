//--< wiseMex_wiChanl.c >--------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for wiChanl
//  Copyright 2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"
#include "wiChanl.h"
#include "wiMath.h"
#include "wiUtil.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)

// ================================================================================================
// FUNCTION  : wiseMex_wiChanl()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
wiStatus wiseMex_wiChanl(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
    /////////////////////////////////////////////////////////////////////////////////////
	//
	//  wiChanl_
	//
    if (strstr(cmd, "wiChanl_"))
    {
		    //---------------------------------------------------------------------------
		    if COMMAND("wiChanl_GetChecksum")
		    {
                int i; wiUInt Checksum = 0;
                wiChanl_State_t *pState = wiChanl_State();

                wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
                
                for (i=0; i<pState->NumRx; i++)
                    wiMath_FletcherChecksum(pState->Nr * sizeof(wiComplex), pState->r[i], &Checksum);

                wiMexUtil_PutUIntValue(Checksum, &plhs[0] );
                return WI_SUCCESS;
            }
        if COMMAND("wiChanl_GetParameter")
        {
            char *Param;
			wiChanl_State_t *pState = wiChanl_State();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Param = wiMexUtil_GetStringValue(prhs[1],0);

                 if MATCH(Param,"Model"                 ) wiMexUtil_PutIntValue        (pState->Model,                 &plhs[0]);
            else if MATCH(Param,"Channel"               ) wiMexUtil_PutIntValue        (pState->Channel,               &plhs[0]);
            else if MATCH(Param,"Fs"                    ) wiMexUtil_PutRealValue       (pState->Fs,                    &plhs[0]);
            else if MATCH(Param,"GainStep.Enabled"      ) wiMexUtil_PutBooleanValue    (pState->GainStep.Enabled,      &plhs[0]);
            else if MATCH(Param,"GainStep.k0"           ) wiMexUtil_PutUIntValue       (pState->GainStep.k0,           &plhs[0]);
            else if MATCH(Param,"GainStep.dB"           ) wiMexUtil_PutRealValue       (pState->GainStep.dB[0],        &plhs[0]);
            else if MATCH(Param,"GainStep.dB[0]"        ) wiMexUtil_PutRealValue       (pState->GainStep.dB[0],        &plhs[0]);
            else if MATCH(Param,"GainStep.dB[1]"        ) wiMexUtil_PutRealValue       (pState->GainStep.dB[1],        &plhs[0]);
            else if MATCH(Param,"PositionOffset.Enabled") wiMexUtil_PutBooleanValue    (pState->PositionOffset.Enabled,&plhs[0]);
            else if MATCH(Param,"PositionOffset.Range"  ) wiMexUtil_PutUIntValue       (pState->PositionOffset.Range,  &plhs[0]);
            else if MATCH(Param,"PositionOffset.dt"     ) wiMexUtil_PutUIntValue       (pState->PositionOffset.dt,     &plhs[0]);
            else if MATCH(Param,"PositionOffset.t0"     ) wiMexUtil_PutUIntValue       (pState->PositionOffset.t0,     &plhs[0]);
            else if MATCH(Param,"Interference.Model"    ) wiMexUtil_PutIntValue        (pState->Interference.Model,    &plhs[0]);
            else if MATCH(Param,"r"                     ) wiMexUtil_PutComplexMatrix(pState->NumRx, pState->Nr, pState->r, &plhs[0]);
            else 
                mexErrMsgTxt("wiChanl_GetParameter: parameter not available");

            mxFree(Param);

            return WI_SUCCESS;
        }

        if COMMAND("wiChanl_GetState")
        {
            const char *field_names[] = {"Fs","NumRx","N_Rx","Nr","r","A"};
            const mwSize dims[2] = {1,1};
            int nfields = (sizeof(field_names)/sizeof(*field_names));
            mxArray *pR;

            wiChanl_State_t *pState = wiChanl_State();
            if (!pState) MCHK( WI_ERROR_MODULE_NOT_INITIALIZED);
            wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            
            plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
            if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
            
            wiMexUtil_PutComplexMatrix(pState->NumRx, pState->Nr, pState->r, &pR); 
        
            mxSetField(plhs[0],0,"Fs",   mxCreateDoubleScalar(pState->Fs));
            mxSetField(plhs[0],0,"Nr",   mxCreateDoubleScalar(pState->Nr));
            mxSetField(plhs[0],0,"NumRx",mxCreateDoubleScalar(pState->NumRx));
            mxSetField(plhs[0],0,"N_Rx", mxCreateDoubleScalar(pState->NumRx)); // old style
            mxSetField(plhs[0],0,"r", pR);
            mxSetField(plhs[0],0,"A",    mxCreateDoubleScalar(pState->A));

            return WI_SUCCESS;
        }

        if COMMAND("wiChanl_Model")
        {
            wiComplex **X, **R;
            int Nx, nTX; wiReal Fs, Pr_dBm, Prms, Nrms;
            wiChanl_State_t *pState = wiChanl_State();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,6);
            if (!pState) MCHK(WI_ERROR_MODULE_NOT_INITIALIZED);

            X = wiMexUtil_GetComplexMatrix(prhs[1], &Nx, &nTX);
            if (nTX > 4) mexErrMsgTxt("Too many transmit streams: maximum allowable is 4.");
            
            Fs     = wiMexUtil_GetRealValue(prhs[2]);
            Pr_dBm = wiMexUtil_GetRealValue(prhs[3]);
            Prms   = wiMexUtil_GetRealValue(prhs[4]);
            Nrms   = wiMexUtil_GetRealValue(prhs[5]);
            
            MCHK( wiChanl_Model(Nx, X, Nx, &R, Fs, Pr_dBm, Prms, Nrms) );
            wiMexUtil_PutComplexMatrix(pState->NumRx, Nx, R, &plhs[0]);
            return WI_SUCCESS;
        }

        if COMMAND("wiChanl_LoadChannelResponse")
        {
            int iTx, iRx, Nc;
            wiComplex *c;

            wiMexUtil_CheckParameters(nlhs, nrhs, 0, 4);
            iRx = wiMexUtil_GetIntValue(prhs[1]);
            iTx = wiMexUtil_GetIntValue(prhs[2]);
            c   = wiMexUtil_GetComplexArray(prhs[3], &Nc);

            MCHK( wiChanl_LoadChannelResponse(iRx, iTx, Nc, c) );
            return WI_SUCCESS;
        }
        if COMMAND("wiChanl_GetChannelResponse")
        {
            int iTx, iRx;
            wiChanl_State_t *pState = wiChanl_State();
            if (!pState) MCHK(WI_ERROR_MODULE_NOT_INITIALIZED);

            wiMexUtil_CheckParameters(nlhs, nrhs, 1, 3);
            iRx = wiMexUtil_GetIntValue(prhs[1]);
            iTx = wiMexUtil_GetIntValue(prhs[2]);
            if (InvalidRange(iTx, 0, WICHANL_TXMAX-1) || InvalidRange(iRx, 0, WICHANL_RXMAX-1))
                mexErrMsgTxt("TX or RX stream index out of range");
            if (!pState->c[iRx][iTx]) 
                mexErrMsgTxt("Requested channel response not available");

            wiMexUtil_PutComplexArray(pState->Nc, pState->c[iRx][iTx], &plhs[0]);
            return WI_SUCCESS;
        }
    }
	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_wiChanl()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

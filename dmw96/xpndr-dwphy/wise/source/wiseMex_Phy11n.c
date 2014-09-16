//--< wiseMex_Phy11n.c >--------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Lookup-and-Execute for the Phy11n PHY method
//  Copyright 2011 DSP Group, Inc. All Rights Reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"
#include "wiMath.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)

// ================================================================================================
// FUNCTION  : wiseMex_Phy11n()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- wiseMex command string
// ================================================================================================
wiStatus wiseMex_Phy11n(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	/////////////////////////////////////////////////////////////////////////////////////
	//
	//  Phy11n_
	//
	if (strstr(cmd, "Phy11n_")) /////////////////////////////////////////////////////////
    {
		#ifdef __PHY11N_H

		    //---------------------------------------------------------------------------
		    if COMMAND("Phy11n_GetChecksum")
		    {
                int i;
                wiUInt TxChecksum = 0, RxChecksum = 0, Checksum = 0;
                Phy11n_TxState_t  *pTX = Phy11n_TxState();
                Phy11n_RxState_t  *pRX = Phy11n_RxState();

                if (nrhs != 1) mexErrMsgTxt("Only 1 input parameter was expected.\n");

                wiMath_FletcherChecksum(pTX->a.Length * sizeof(int), pTX->a.ptr, &TxChecksum);
                wiMath_FletcherChecksum(pTX->c.Length * sizeof(int), pTX->c.ptr, &TxChecksum);

                for (i=0; i<pTX->N_STS; i++)
                    wiMath_FletcherChecksum(pTX->Nx * sizeof(wiComplex), pTX->x[i], &TxChecksum);

                for (i=0; i<pRX->N_RX; i++)
                    wiMath_FletcherChecksum(pRX->Ny * sizeof(wiComplex), pRX->y[i], &RxChecksum);

                wiMath_FletcherChecksum(pRX->Na * sizeof(int), pRX->a,  &RxChecksum);
                wiMath_FletcherChecksum(pRX->Nc * sizeof(int), pRX->Lc, &RxChecksum);

                wiMath_FletcherChecksum(sizeof(wiUInt), (wiByte *)&TxChecksum, &Checksum);
                wiMath_FletcherChecksum(sizeof(wiUInt), (wiByte *)&RxChecksum, &Checksum);
                
                if (nlhs >= 1) wiMexUtil_PutUIntValue (   Checksum, &plhs[0] );
                if (nlhs >= 2) wiMexUtil_PutUIntValue ( TxChecksum, &plhs[1] );
                if (nlhs >= 3) wiMexUtil_PutUIntValue ( RxChecksum, &plhs[2] );

                return WI_SUCCESS;
            }
		    //---------------------------------------------------------------------------
		    if COMMAND("Phy11n_GetTxParameter")
		    {
                char *Param;
                                
                Phy11n_TxState_t* pState = Phy11n_TxState();
                wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
                Param = wiMexUtil_GetStringValue(prhs[1],0);
                
                     if MATCH(Param,"MCS"           ) wiMexUtil_PutIntValue (pState->MCS,            &plhs[0]);
                else if MATCH(Param,"HT_LENGTH"     ) wiMexUtil_PutIntValue (pState->HT_LENGTH,      &plhs[0]);
                else if MATCH(Param,"N_SYM"         ) wiMexUtil_PutIntValue (pState->N_SYM,          &plhs[0]);
                else if MATCH(Param,"N_SS"          ) wiMexUtil_PutIntValue (pState->N_SS,           &plhs[0]);
                else if MATCH(Param,"a"             ) wiMexUtil_PutIntArray (pState->a.Length, pState->a.ptr,  &plhs[0]);
                else if MATCH(Param,"b"             ) wiMexUtil_PutIntArray (pState->b.Length, pState->b.ptr,  &plhs[0]);
                else if MATCH(Param,"c"             ) wiMexUtil_PutIntArray (pState->c.Length, pState->c.ptr,  &plhs[0]);
                else if MATCH(Param,"stit[0]"       ) wiMexUtil_PutIntArray (pState->Nsp[0], pState->stit[0], &plhs[0]);
                else if MATCH(Param,"stit[1]"       ) wiMexUtil_PutIntArray (pState->Nsp[1], pState->stit[1], &plhs[0]);
                else if MATCH(Param,"stit[2]"       ) wiMexUtil_PutIntArray (pState->Nsp[2], pState->stit[2], &plhs[0]);
                else if MATCH(Param,"stit[3]"       ) wiMexUtil_PutIntArray (pState->Nsp[3], pState->stit[3], &plhs[0]);
                else if MATCH(Param,"stsp[0]"       ) wiMexUtil_PutIntArray (pState->Nsp[0], pState->stsp[0], &plhs[0]);
                else if MATCH(Param,"stsp[1]"       ) wiMexUtil_PutIntArray (pState->Nsp[1], pState->stsp[1], &plhs[0]);
                else if MATCH(Param,"stsp[2]"       ) wiMexUtil_PutIntArray (pState->Nsp[2], pState->stsp[2], &plhs[0]);
                else if MATCH(Param,"stsp[3]"       ) wiMexUtil_PutIntArray (pState->Nsp[3], pState->stsp[3], &plhs[0]);
                else if MATCH(Param,"x"             ) wiMexUtil_PutComplexMatrix(pState->N_TX, pState->Nx, pState->x, &plhs[0]);
                else 
                    mexErrMsgTxt("Phy11n_GetTxParameter: parameter not available");

                mxFree(Param);

                return WI_SUCCESS;
            }
		    //---------------------------------------------------------------------------
		    if COMMAND("Phy11n_GetRxParameter")
		    {
                char *Param;
                                
                Phy11n_RxState_t* pState = Phy11n_RxState();
                wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
                Param = wiMexUtil_GetStringValue(prhs[1],0);
                
                     if MATCH(Param,"MCS"           ) wiMexUtil_PutIntValue (pState->MCS,            &plhs[0]);
                else if MATCH(Param,"HT_LENGTH"     ) wiMexUtil_PutIntValue (pState->HT_LENGTH,      &plhs[0]);
                else if MATCH(Param,"N_SYM"         ) wiMexUtil_PutIntValue (pState->N_SYM,          &plhs[0]);
                else if MATCH(Param,"N_SS"          ) wiMexUtil_PutIntValue (pState->N_SS,           &plhs[0]);
                else if MATCH(Param,"a"             ) wiMexUtil_PutIntArray (pState->Na,     pState->a,        &plhs[0]);
                else if MATCH(Param,"b"             ) wiMexUtil_PutIntArray (pState->Nb,     pState->b,        &plhs[0]);
                else if MATCH(Param,"Lc"            ) wiMexUtil_PutRealArray(pState->Nc,     pState->Lc,       &plhs[0]);
                else if MATCH(Param,"Lc_e"          ) wiMexUtil_PutRealArray(pState->Nc,     pState->Lc_e,     &plhs[0]);
                else if MATCH(Param,"stit[0]"       ) wiMexUtil_PutRealArray(pState->Nsp[0], pState->stit[0], &plhs[0]);
                else if MATCH(Param,"stit[1]"       ) wiMexUtil_PutRealArray(pState->Nsp[1], pState->stit[1], &plhs[0]);
                else if MATCH(Param,"stit[2]"       ) wiMexUtil_PutRealArray(pState->Nsp[2], pState->stit[2], &plhs[0]);
                else if MATCH(Param,"stit[3]"       ) wiMexUtil_PutRealArray(pState->Nsp[3], pState->stit[3], &plhs[0]);
                else if MATCH(Param,"stsp[0]"       ) wiMexUtil_PutRealArray(pState->Nsp[0], pState->stsp[0], &plhs[0]);
                else if MATCH(Param,"stsp[1]"       ) wiMexUtil_PutRealArray(pState->Nsp[1], pState->stsp[1], &plhs[0]);
                else if MATCH(Param,"stsp[2]"       ) wiMexUtil_PutRealArray(pState->Nsp[2], pState->stsp[2], &plhs[0]);
                else if MATCH(Param,"stsp[3]"       ) wiMexUtil_PutRealArray(pState->Nsp[3], pState->stsp[3], &plhs[0]);
                else if MATCH(Param,"y"             ) wiMexUtil_PutComplexMatrix(pState->N_RX, pState->Ny, pState->y, &plhs[0]);
                else 
                    mexErrMsgTxt("Phy11n_GetTxParameter: parameter not available");

                mxFree(Param);

                return WI_SUCCESS;
            }
			//---------------------------------------------------------------------------
            if COMMAND("Phy11n_RX_ViterbiDecoder")
            {
                int CodeRate12, TruncationDepth, NumBits, NumLLR, *pOutputData;
                wiReal *InputLLR;

                wiMexUtil_CheckParameters(nlhs, nrhs, 1, 4);

                InputLLR        = wiMexUtil_GetRealArray(prhs[1], &NumLLR);
                CodeRate12      = wiMexUtil_GetIntValue (prhs[2]);
                TruncationDepth = wiMexUtil_GetIntValue (prhs[3]);

                NumBits = NumLLR * CodeRate12 / 12;
                pOutputData = (int *)mxCalloc(NumBits, sizeof(int));
				if (!pOutputData) mexErrMsgTxt("Memory Allocation Error");

                MCHK( Phy11n_RX_ViterbiDecoder(NumBits, InputLLR, pOutputData, CodeRate12, TruncationDepth, WI_FALSE) );

                wiMexUtil_PutIntArray(NumBits, pOutputData, &plhs[0]);
                mxFree(pOutputData);
                return WI_SUCCESS;
            }
			//---------------------------------------------------------------------------
            if COMMAND("Phy11n_RX_Deinterleave")
            {
                int N_CBPSS, N_BPSC, Iss, Nsp;
                wiReal *x, *y;

                wiMexUtil_CheckParameters(nlhs, nrhs, 1, 5);

                N_CBPSS   = wiMexUtil_GetIntValue(prhs[1]);
                N_BPSC    = wiMexUtil_GetIntValue(prhs[2]);
                Iss       = wiMexUtil_GetIntValue(prhs[3]);
                y         = wiMexUtil_GetRealArray(prhs[4], &Nsp);

                x = (wiReal *)mxCalloc(Nsp, sizeof(wiReal));
				if (!x) mexErrMsgTxt("Memory Allocation Error");

                MCHK( Phy11n_RX_Deinterleave(N_CBPSS, N_BPSC, Iss, x, y, Nsp) );

                wiMexUtil_PutRealArray(Nsp, x, &plhs[0]);
                mxFree(y);
                return WI_SUCCESS;
            }
			//---------------------------------------------------------------------------
            if COMMAND("Phy11n_RX_Descrambler")
            {
                wiInt *b, *a, NumBits;

                wiMexUtil_CheckParameters(nlhs, nrhs, 1, 2);

                b = wiMexUtil_GetIntArray(prhs[1], &NumBits);

                a = (wiInt *)mxCalloc(NumBits, sizeof(wiInt));
				if (!a) mexErrMsgTxt("Memory Allocation Error");

                MCHK( Phy11n_RX_Descrambler(NumBits, b, a) );

                wiMexUtil_PutIntArray(NumBits, a, &plhs[0]);
                mxFree(a);
                return WI_SUCCESS;
            }
			//---------------------------------------------------------------------------
			if COMMAND("Phy11n_GetSingleTreeSearchState")
			{
				Phy11n_SingleTreeSearchSphereDecoder_t *pState = &(Phy11n_RxState()->SingleTreeSearchSphereDecoder);
				wiMexUtil_CheckParameters(nlhs,nrhs,14,1);
				wiMexUtil_PutIntValue (pState->ModifiedQR,         &plhs[0]);
				wiMexUtil_PutIntValue (pState->Sorted,             &plhs[1]);
				wiMexUtil_PutIntValue (pState->MMSE,               &plhs[2]);
				wiMexUtil_PutIntValue (pState->MMSE,               &plhs[3]);
				wiMexUtil_PutIntValue (pState->LLRClip,            &plhs[4]);
				wiMexUtil_PutRealValue (pState->LMax,              &plhs[5]);
				wiMexUtil_PutIntValue (pState->GetStatistics,      &plhs[6]);
				wiMexUtil_PutRealValue (pState->AvgCycles,         &plhs[7]);
				wiMexUtil_PutRealValue (pState->AvgNodes,          &plhs[8]);
				wiMexUtil_PutRealValue (pState->AvgLeaves,         &plhs[9]);
				wiMexUtil_PutIntValue (pState->NSTS,				          &plhs[10]);
				wiMexUtil_PutRealArray(pState->NSTS, pState->NCyclesPSTS,     &plhs[11]);
				wiMexUtil_PutRealArray(pState->NSTS, pState->PH2PSTS,		  &plhs[12]);
				wiMexUtil_PutRealArray(pState->NSTS, pState->R2RatioPSTS,	  &plhs[13]);
				return WI_SUCCESS;
			}
            //---------------------------------------------------------------------------
            if COMMAND("Phy11n_GetBFState")
            {
                Phy11n_BFDemapper_t *pState = &(Phy11n_RxState()->BFDemapper);
                if (nlhs > 3 || nlhs < 1)
	                wiMexUtil_CheckParameters(nlhs,nrhs,3,1);
                wiMexUtil_PutIntValue  (pState->K,       &plhs[0]);
                if (nlhs > 1) wiMexUtil_PutRealValue (pState->mode, &plhs[1]);
                if (nlhs > 2) wiMexUtil_PutIntValue  (pState->K1,   &plhs[2]);

                return WI_SUCCESS;
            }

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: Phy11n_ command received but Phy11n.h not included.");

		#endif // PHY11N_H
	}
	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_Phy11n()

//-------------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------------


//--< wiseMex_CalcEVM.c >--------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Lookup-and-Execute for the CalcEVM PHY method
//  Copyright 2007 DSP Group, Inc. All Rights Reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"
#include "wiUtil.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))

// ================================================================================================
// FUNCTION  : wiseMex_CalcEVM()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- wiseMex command string
// ================================================================================================
wiStatus wiseMex_CalcEVM(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	/////////////////////////////////////////////////////////////////////////////////////
	//
	// CalcEVM_OFDM_
	// This is a sub-module used by CalcEVM. These commands must be checked before other
	// CalcEVM_ calls because "CalcEVM_" is a common command prefix.
	//
	if (strstr(cmd, "CalcEVM_OFDM_")) ///////////////////////////////////////////////////
    {

		#ifdef __CALCEVM_OFDM_H

			//---------------------------------------------------------------------------
			if COMMAND("CalcEVM_OFDM_RxState")
			{
				const char *field_names[] = {"v","S_N52","FSOffset", "SNR_Legacy", "SNR_HT"};
				int nfields = (sizeof(field_names)/sizeof(*field_names));
				const mwSize dims[2] = {1,1};

				CalcEVM_OFDM_RxState_t *pRX = CalcEVM_OFDM_RxState();

				plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
				if (pRX->v) mxSetField(plhs[0],0,"v",wiMexUtil_CreateComplexArray(pRX->Nv, pRX->v));
				mxSetField(plhs[0],0,"S_N52",wiMexUtil_CreateRealArray(52, pRX->S_N52));
				mxSetField(plhs[0],0,"FSOffset",mxCreateDoubleScalar(pRX->FSOffset));
				mxSetField(plhs[0],0,"SNR_Legacy",wiMexUtil_CreateRealArray(64, pRX->SNR_Legacy));
				mxSetField(plhs[0],0,"SNR_HT",wiMexUtil_CreateRealArray(64, pRX->SNR_HT));
				
				return WI_SUCCESS;
			}
            if COMMAND("CalcEVM_OFDM_ViterbiDecoder")
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

                MCHK( CalcEVM_OFDM_ViterbiDecoder(NumBits, InputLLR, pOutputData, CodeRate12, TruncationDepth) );

                wiMexUtil_PutIntArray(NumBits, pOutputData, &plhs[0]);
                mxFree(pOutputData);
                return WI_SUCCESS;
            }

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: CalcEVM_OFDM_ command received but CalcEVM_OFDM.h not included.");

		#endif // CalcEVM_OFDM_H
	}

	/////////////////////////////////////////////////////////////////////////////////////
	//
	// CalcEVM_
	// Commands that relate to CalcEVM.c. This code should be placed after any commands
	// with a common CalcEVM_ prefix such as CalcEVM_OFDM_.
	//
    if (strstr(cmd, "CalcEVM_")) ////////////////////////////////////////////////////////
    {

		#ifdef __CALCEVM_H
        
			//---------------------------------------------------------------------------
			if COMMAND("CalcEVM_GetRxData")
			{
				CalcEVM_State_t *pState = CalcEVM_State();
				CalcEVM_OFDM_RxState_t *pRX = CalcEVM_OFDM_RxState();

                wiMexUtil_CheckParameters(nlhs,nrhs,19,1);
			
				if (!pState->UseMojaveRX) // Output Floating Point OFDM RX Data
				{

					if (pRX->N_PHY_RX_WR >= 8)
                        wiMexUtil_PutUWORDArray(max(0,pRX->N_PHY_RX_WR-8), pRX->PHY_RX_D+8, &plhs[0]);
                    else
                        wiMexUtil_PutRealValue(0.0, &plhs[0]);

                    wiMexUtil_PutComplexArray(64, pState->EVM.h, &plhs[1]); 
					wiMexUtil_PutIntValue (pRX->Fault, &plhs[2]);
        
					// -------------------
					// Unpack the EVM data
					// -------------------
					if (!pRX->Fault)
					{	
						wiBoolean MixedMode = pRX->MM_DATA;												

						int NxEVM = 64 * (MixedMode ? (pState->EVM.N_SYM + 3) : (pState->EVM.N_SYM + 1));
						int NDPLL = MixedMode ? (pState->EVM.N_SYM + 5) : (pState->EVM.N_SYM + 1);

						if (!NxEVM) mexErrMsgTxt("EVM Arrays are Empty");
						if (!pState->EVM.x) mexErrMsgTxt("EVM.x is empty");
						if (!pState->EVM.u) mexErrMsgTxt("EVM.u is empty");
            
						wiMexUtil_PutComplexArray(NxEVM, pState->EVM.u, &plhs[3]); //includes SIG
						wiMexUtil_PutIntArray (NDPLL, pRX->AdvDly, &plhs[4]);  //includes SIG
						wiMexUtil_PutRealArray(NDPLL, pRX->cSP,    &plhs[5]);  //includes SIG
						wiMexUtil_PutRealArray(NDPLL, pRX->cCP,    &plhs[6]);  //includes SIG
						wiMexUtil_PutRealValue(pState->EVM.EVMdB,       &plhs[7]);
						wiMexUtil_PutComplexArray(NxEVM, pState->EVM.x, &plhs[8]);  //includes SIG
						wiMexUtil_PutUIntValue(pRX->RATE.w7,&plhs[9]);
						wiMexUtil_PutUIntValue(pRX->LENGTH, &plhs[10]);
						wiMexUtil_PutIntValue (pRX->N_SYM,  &plhs[11]);
						wiMexUtil_PutRealArray(64, pState->EVM.cSe2, &plhs[12]);
						wiMexUtil_PutRealArray(pRX->N_SYM, pState->EVM.nSe2, &plhs[13]);
						wiMexUtil_PutRealValue(pState->EVM.cCFO,    &plhs[14]);
						wiMexUtil_PutRealValue(pState->EVM.fCFO,    &plhs[15]);
						wiMexUtil_PutRealValue(pState->EVM.W_EVMdB, &plhs[16]);
						wiMexUtil_PutUIntValue(pState->UseMojaveRX, &plhs[17]);
						wiMexUtil_PutIntValue(MixedMode, &plhs[18]);					
					}
					else
					{
                        int i;
                        for (i=3; i<=18; i++)
                            wiMexUtil_PutIntValue(0, &plhs[i]);
                    }
				}
				else  // output Mojave RX Data
				{
					#ifdef __MOJAVE_H
					{
						Mojave_RX_State *pRX;
						MojaveRegisters *pReg;
						
                        XSTATUS(Mojave_StatePointer(NULL, &pRX, &pReg));

						if (pRX->N_PHY_RX_WR >= 8)
							wiMexUtil_PutUWORDArray(max(0,pRX->N_PHY_RX_WR-8), pRX->PHY_RX_D+8, &plhs[0]);

                        wiMexUtil_PutComplexArray(64, pState->EVM.h, &plhs[1]); 
						wiMexUtil_PutIntValue (pRX->Fault, &plhs[2]);
        
						// -------------------
						// Unpack the EVM data
						// -------------------
						if (!pRX->Fault)
						{
            				const du = 82; // du is set to be consistant with Dakota;

							int NxEVM = 64 * (pState->EVM.N_SYM + 1);

							if (!NxEVM)         mexErrMsgTxt("EVM Arrays are Empty");
							if (!pState->EVM.x) mexErrMsgTxt("EVM.x is empty");
                            if (!pState->EVM.u) mexErrMsgTxt("EVM.u is empty");
            
							wiMexUtil_PutComplexArray(pRX->Nu-du, pState->EVM.u, &plhs[3]);
							wiMexUtil_PutWORDArray (pRX->Ns, pRX->AdvDly, &plhs[4]);
							wiMexUtil_PutWORDArray(pRX->N_SYM+1, pRX->cSP,    &plhs[5]);
							wiMexUtil_PutWORDArray(pRX->N_SYM+1, pRX->cCP,    &plhs[6]);
							wiMexUtil_PutRealValue(pState->EVM.EVMdB,       &plhs[7]);
							wiMexUtil_PutComplexArray(NxEVM, pState->EVM.x, &plhs[8]);
							wiMexUtil_PutUIntValue(pReg->RX_RATE.w4,&plhs[9]);
							wiMexUtil_PutUIntValue(pReg->RX_LENGTH.word, &plhs[10]);
							wiMexUtil_PutIntValue (pRX->N_SYM,  &plhs[11]);
							wiMexUtil_PutRealArray(64, pState->EVM.cSe2, &plhs[12]);
							wiMexUtil_PutRealArray(pRX->N_SYM, pState->EVM.nSe2, &plhs[13]);
							wiMexUtil_PutRealValue(pState->EVM.cCFO,    &plhs[14]);
							wiMexUtil_PutRealValue(pState->EVM.fCFO,    &plhs[15]);
							wiMexUtil_PutRealValue(pState->EVM.W_EVMdB, &plhs[16]);
							wiMexUtil_PutUIntValue(pState->UseMojaveRX, &plhs[17]);
							wiMexUtil_PutIntValue(0,                    &plhs[18]);
						}
						else
						{
                            int i;
                            for (i=3; i<=18; i++)
                                wiMexUtil_PutIntValue(0, &plhs[i]);
                        }
					}
					#else

						mexErrMsgTxt("wiseMex_CalcEVM: Mojave not included in this build");
					
					#endif
				}
				if (pState->EVM.x) wiFree(pState->EVM.x); pState->EVM.x=NULL;
				if (pState->EVM.u) wiFree(pState->EVM.u); pState->EVM.u=NULL;

				return WI_SUCCESS;
			}
			//---------------------------------------------------------------------------
			if COMMAND("CalcEVM_GetRxParameters")  //Get the RX Register Value
			{
				const char *field_names[] = {"aModemTX","DCXSelect", "DCXShift", "SigDetFilter", "SigDetTh1", 
				  "SigDetTh2f", "SigDetDelay", "SyncFilter", "SyncShift", "SyncSTDelay", "SyncPKDelay", 
				  "SGSigDet","FSOffset", "PKThreshold", "CFOFilter", "DelayCFO", "Ca","Cb", "Sa", "Sb"};
				int nfields = (sizeof(field_names)/sizeof(*field_names));
				const mwSize dims[2] = {1,1};
				
                CalcEVM_OFDM_RxState_t *pRX = CalcEVM_OFDM_RxState();

                plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
				mxSetField(plhs[0],0,"aModemTX",    mxCreateDoubleScalar(pRX->aModemTX));
				mxSetField(plhs[0],0,"DCXSelect",   mxCreateDoubleScalar(pRX->DCXSelect));
				mxSetField(plhs[0],0,"DCXShift",    mxCreateDoubleScalar(pRX->DCXShift));
				mxSetField(plhs[0],0,"SigDetFilter",mxCreateDoubleScalar(pRX->SigDetFilter));
				mxSetField(plhs[0],0,"SigDetTh1",   mxCreateDoubleScalar(pRX->SigDetTh1));
				mxSetField(plhs[0],0,"SigDetTh2f",  mxCreateDoubleScalar(pRX->SigDetTh2f));
				mxSetField(plhs[0],0,"SigDetDelay", mxCreateDoubleScalar(pRX->SigDetDelay));
				mxSetField(plhs[0],0,"SyncFilter",  mxCreateDoubleScalar(pRX->SyncFilter));
				mxSetField(plhs[0],0,"SyncShift",   mxCreateDoubleScalar(pRX->SyncShift));
				mxSetField(plhs[0],0,"SyncSTDelay", mxCreateDoubleScalar(pRX->SyncSTDelay));
				mxSetField(plhs[0],0,"SyncPKDelay", mxCreateDoubleScalar(pRX->SyncPKDelay));
				mxSetField(plhs[0],0,"PKThreshold", mxCreateDoubleScalar(pRX->PKThreshold));
				mxSetField(plhs[0],0,"CFOFilter",   mxCreateDoubleScalar(pRX->CFOFilter));
				mxSetField(plhs[0],0,"DelayCFO",    mxCreateDoubleScalar(pRX->DelayCFO));
				mxSetField(plhs[0],0,"Ca", mxCreateDoubleScalar(pRX->Ca));
				mxSetField(plhs[0],0,"Cb", mxCreateDoubleScalar(pRX->Cb));
				mxSetField(plhs[0],0,"Sa", mxCreateDoubleScalar(pRX->Sa));
				mxSetField(plhs[0],0,"Sb", mxCreateDoubleScalar(pRX->Sb));
				mxSetField(plhs[0],0,"SGSigDet", mxCreateDoubleScalar(pRX->SGSigDet));
				mxSetField(plhs[0],0,"FSOffset", mxCreateDoubleScalar(pRX->FSOffset));

				return WI_SUCCESS;
			}

		#else // no header loaded; therefore, no functions to call

			mexErrMsgTxt("wiseMex: CalcEVM_ command received but CalcEVM.h not included.");

		#endif  // CalcEVM_H
    }

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_CalcEVM()

//-------------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------------

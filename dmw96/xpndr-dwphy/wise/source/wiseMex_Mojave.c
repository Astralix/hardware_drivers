//--< wiseMex_Mojave.c >---------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the Mojave Baseband Model
//  Copyright 2007-2008 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))

// ================================================================================================
// FUNCTION  : wiseMex_Mojave()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
//             cmd  -- wiseMex command string
// ================================================================================================
wiStatus wiseMex_Mojave( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
    if (!strstr(cmd, "Mojave_")) 
        return WI_WARN_PARSE_FAILED; // indicate no command match

	#ifdef __MOJAVE_H

		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_GetTxState")
		{
			const char *field_names[] = {
				"a","b","c","p","v","x","y","z","u","d",
				"Encoder_Shift_Register","Pilot_Shift_Register","Fault","DAC"
			};
			const char *DAC_field_names[] = {"nEff","Mask","Vpp","c"};
			int dims[2] = {1,1};
			int nfields = (sizeof(field_names)/sizeof(*field_names));
			mxArray *pDAC;

			Mojave_TX_State *pTX;
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
			MCHK(Mojave_StatePointer(&pTX, NULL, NULL));

			plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
			pDAC    = mxCreateStructArray(2, dims, 4, DAC_field_names);
			if (!plhs[0] || !pDAC) mexErrMsgTxt("Memory Allocation Error");
        
			if (pTX->a)     mxSetField(plhs[0],0,"a",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Na, pTX->a, NULL));
			if (pTX->b)     mxSetField(plhs[0],0,"b",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nb, pTX->b, NULL));
			if (pTX->p)     mxSetField(plhs[0],0,"p",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Np, pTX->p, NULL));
			if (pTX->vReal) mxSetField(plhs[0],0,"v",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nv, pTX->vReal, pTX->vImag));
			if (pTX->xReal) mxSetField(plhs[0],0,"x",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nx, pTX->xReal, pTX->xImag));
			if (pTX->yReal) mxSetField(plhs[0],0,"y",wiMexUtil_CreateWORDArrayAsDouble (pTX->Ny, pTX->yReal, pTX->yImag));
			if (pTX->zReal) mxSetField(plhs[0],0,"z",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nz, pTX->zReal, pTX->zImag));
			if (pTX->uReal) mxSetField(plhs[0],0,"u",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nz, pTX->uReal, pTX->uImag));
			if (pTX->d)     mxSetField(plhs[0],0,"d",wiMexUtil_CreateComplexArray      (pTX->Nd, pTX->d));

			if (pTX->c)     mxSetField(plhs[0],0,"c",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nc, pTX->c, NULL)); // OFDM
			else if (pTX->cReal) 
						   mxSetField(plhs[0],0,"c",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nc, (wiWORD *)pTX->cReal, (wiWORD *)pTX->cImag)); // DSSS/CCK

			mxSetField(plhs[0],0,"Encoder_Shift_Register",  mxCreateDoubleScalar(pTX->Encoder_Shift_Register.word));
			mxSetField(plhs[0],0,"Pilot_Shift_Register",    mxCreateDoubleScalar(pTX->Pilot_Shift_Register.word));
			mxSetField(plhs[0],0,"Fault",                   mxCreateDoubleScalar(pTX->Fault));
			mxSetField(plhs[0],0,"DAC", pDAC);
			mxSetField(pDAC,0,   "nEff", mxCreateDoubleScalar(pTX->DAC.nEff));
			mxSetField(pDAC,0,   "Mask", mxCreateDoubleScalar(pTX->DAC.Mask));
			mxSetField(pDAC,0,   "Vpp",  mxCreateDoubleScalar(pTX->DAC.Vpp));
			mxSetField(pDAC,0,   "c",    mxCreateDoubleScalar(pTX->DAC.c));

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_GetRxState")
		{
			const char *field_names[] = {
				"N_SYM","N_DBPS","N_CBPS","N_BPSC","RX_RATE","RX_LENGTH","PHY_RX_D","aLength","rLength","Bypass_CFO","Bypass_PLL",
				"q","c","r","z","y","s","w","x","v","u","Lp","Lc","cA","cB","b","a","d","H2","AGain","DGain",
				"DFEState","DFEFlags","AdvDly","cSP","cCP","kFFT","h2","h","Fault","x_ofs","DownsamPhase","NoiseShift",
				"ADC","EVM","EventFlag","aSigOut","bSigOut","traceRxState","traceRxControl","traceRadioIO","traceDFE",
				"traceRSSI","traceFrameSync","traceSigDet","traceDFS","traceDataConv"
			};
			const char *ADC_field_names[] = {"nEff","Mask","Offset","Vpp","c"};
			const char *EVM_field_names[] = {"Enabled","N_SYM","Se2","cSe2","nSe2","EVM","EVMdB"};
			int dims[2] = {1,1}; int dims2[2] = {1,2};
			int nfields = (sizeof(field_names)/sizeof(*field_names));
			mxArray *pADC, *pEVM, *pA;
			wiWORD *h2[2], *hReal[2], *hImag[2];
			unsigned int N80, N40, N20, N22;

			Mojave_RX_State *pRX;
			bMojave_RX_State *pbRX;
			MojaveRegisters *pReg;
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
			MCHK(Mojave_StatePointer(NULL, &pRX, &pReg));
			MCHK(bMojave_StatePointer(&pbRX));

			plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
			pADC    = mxCreateStructArray(2, dims, 5, ADC_field_names);
			pEVM    = mxCreateStructArray(2, dims, 7, EVM_field_names);
			if (!plhs[0] || !pADC || !pEVM) mexErrMsgTxt("Memory Allocation Error");

			N80 = pRX->N80; N40 = pRX->N40; N20 = pRX->N20; N22 = pRX->N22;

			mxSetField(plhs[0],0,"N_SYM",   mxCreateDoubleScalar(pRX->N_SYM));
			mxSetField(plhs[0],0,"N_DBPS",  mxCreateDoubleScalar(pRX->N_DBPS));
			mxSetField(plhs[0],0,"N_CBPS",  mxCreateDoubleScalar(pRX->N_CBPS));
			mxSetField(plhs[0],0,"N_BPSC",  mxCreateDoubleScalar(pRX->N_BPSC));
			mxSetField(plhs[0],0,"RX_RATE", mxCreateDoubleScalar(pReg->RX_RATE.w4));
			mxSetField(plhs[0],0,"RX_LENGTH",mxCreateDoubleScalar(pReg->RX_LENGTH.w12));
			mxSetField(plhs[0],0,"aLength", mxCreateDoubleScalar(pRX->aLength));
			mxSetField(plhs[0],0,"rLength", mxCreateDoubleScalar(pRX->rLength));
			mxSetField(plhs[0],0,"PHY_RX_D",wiMexUtil_CreateUWORDArrayAsDouble(pRX->N_PHY_RX_WR, pRX->PHY_RX_D, NULL));
			mxSetField(plhs[0],0,"q" ,wiMexUtil_CreateUWORDMatrixAsDouble(2, pRX->Nr, pRX->qReal, pRX->qImag));
			mxSetField(plhs[0],0,"c" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->cReal, pRX->cImag));
			mxSetField(plhs[0],0,"r" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->rReal, pRX->rImag));
			mxSetField(plhs[0],0,"z" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->zReal, pRX->zImag));
			mxSetField(plhs[0],0,"y" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->yReal, pRX->yImag));
			mxSetField(plhs[0],0,"s" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->sReal, pRX->sImag));
			mxSetField(plhs[0],0,"w" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->wReal, pRX->wImag));
			mxSetField(plhs[0],0,"x" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->xReal, pRX->xImag));
			mxSetField(plhs[0],0,"v" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->vReal, pRX->vImag));
			mxSetField(plhs[0],0,"u" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->uReal, pRX->uImag));
			mxSetField(plhs[0],0,"Lp",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nc, pRX->Lp, NULL));
			mxSetField(plhs[0],0,"Lc",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nc, pRX->Lc, NULL));
			mxSetField(plhs[0],0,"cA",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->cB, NULL));
			mxSetField(plhs[0],0,"cB",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->cA, NULL));
			mxSetField(plhs[0],0,"b", wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->b,  NULL));
			mxSetField(plhs[0],0,"a", wiMexUtil_CreateUWORDArrayAsDouble (pRX->Na, pRX->a,  NULL));
			mxSetField(plhs[0],0,"d", wiMexUtil_CreateWORDArrayAsDouble  (pRX->Nd, pRX->dReal, pRX->dImag));
			mxSetField(plhs[0],0,"H2",wiMexUtil_CreateWORDArrayAsDouble  (pRX->Nd, pRX->H2, NULL));
			mxSetField(plhs[0],0,"DGain",   wiMexUtil_CreateWORDArrayAsDouble  (pRX->N20, pRX->DGain,   NULL));
			mxSetField(plhs[0],0,"AdvDly",  wiMexUtil_CreateWORDArrayAsDouble  (pRX->Ns, pRX->AdvDly,  NULL));
			mxSetField(plhs[0],0,"cSP",     wiMexUtil_CreateWORDArrayAsDouble  (pRX->N_SYM+1, pRX->cSP,     NULL));
			mxSetField(plhs[0],0,"cCP",     wiMexUtil_CreateWORDArrayAsDouble  (pRX->N_SYM+1, pRX->cCP,     NULL));
			wiMexUtil_PutIntArray(pRX->N_SYM+1, pRX->kFFT, &pA); mxSetField(plhs[0],0,"kFFT", pA);
			mxSetField(plhs[0],0,"Fault",       mxCreateDoubleScalar(pRX->Fault));
			mxSetField(plhs[0],0,"x_ofs",       mxCreateDoubleScalar(pRX->x_ofs));
			mxSetField(plhs[0],0,"DownsamPhase",mxCreateDoubleScalar(pRX->DownsamPhase));
			h2[0]=pRX->h2[0]; h2[1]=pRX->h2[1];   
			hReal[0]=pRX->hReal[0]; hReal[1]=pRX->hReal[1]; hImag[0]=pRX->hImag[0]; hImag[1]=pRX->hImag[1];
			mxSetField(plhs[0],0,"h2",wiMexUtil_CreateWORDMatrixAsDouble(2, 64, h2, NULL));
			mxSetField(plhs[0],0,"h", wiMexUtil_CreateWORDMatrixAsDouble(2, 64, hReal, hImag));
			wiMexUtil_PutUWORDArray(2, pRX->NoiseShift, &pA); mxSetField(plhs[0],0,"NoiseShift",pA);
			mxSetField(plhs[0],0,"ADC", pADC);
			mxSetField(pADC,   0,"nEff",        mxCreateDoubleScalar(pRX->ADC.nEff));
			mxSetField(pADC,   0,"Mask",        mxCreateDoubleScalar(pRX->ADC.Mask));
			mxSetField(pADC,   0,"Offset",      mxCreateDoubleScalar(pRX->ADC.Offset));
			mxSetField(pADC,   0,"Vpp",         mxCreateDoubleScalar(pRX->ADC.Vpp));
			mxSetField(pADC,   0,"c",           mxCreateDoubleScalar(pRX->ADC.c));
			mxSetField(plhs[0],0,"EventFlag",   mxCreateDoubleScalar(pRX->EventFlag.word));

			if (pRX->EnableTrace)
			{
				mxSetField(plhs[0],0,"AGain",         wiMexUtil_CreateUWORDArrayAsDouble (pRX->N20, pRX->AGain,                NULL));
				mxSetField(plhs[0],0,"DFEState",      wiMexUtil_CreateUWORDArrayAsDouble (pRX->N20, pRX->DFEState,             NULL));
				mxSetField(plhs[0],0,"DFEFlags",      wiMexUtil_CreateUWORDArrayAsDouble (pRX->N20, pRX->DFEFlags,             NULL));
				mxSetField(plhs[0],0,"aSigOut",       wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->aSigOut,        NULL));
				mxSetField(plhs[0],0,"bSigOut",       wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pRX->bSigOut,        NULL));
				mxSetField(plhs[0],0,"traceRxState",  wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceRxState,   NULL));
				mxSetField(plhs[0],0,"traceRxControl",wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceRxControl, NULL));
				mxSetField(plhs[0],0,"traceRadioIO",  wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceRadioIO,   NULL));
				mxSetField(plhs[0],0,"traceDFE"      ,wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceDFE,       NULL));
				mxSetField(plhs[0],0,"traceRSSI",     wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceRSSI,      NULL));
				mxSetField(plhs[0],0,"traceFrameSync",wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceFrameSync, NULL));
				mxSetField(plhs[0],0,"traceSigDet",   wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceSigDet,    NULL));
				mxSetField(plhs[0],0,"traceDFS",      wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceDFS,       NULL));
				mxSetField(plhs[0],0,"traceDataConv", wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceDataConv,  NULL));
			}      
			mxSetField(plhs[0],0,"EVM", pEVM);
			mxSetField(pEVM,   0,"Enabled",     mxCreateDoubleScalar(pRX->EVM.Enabled));
			mxSetField(pEVM,   0,"N_SYM",       mxCreateDoubleScalar(pRX->EVM.N_SYM));

			wiMexUtil_PutRealArray(2,   pRX->EVM.EVM,    &pA); mxSetField(pEVM,0,"EVM",  pA);
			wiMexUtil_PutRealArray(2,   pRX->EVM.EVMdB,  &pA); mxSetField(pEVM,0,"EVMdB",pA);
			wiMexUtil_PutRealArray(2,   pRX->EVM.Se2,    &pA); mxSetField(pEVM,0,"Se2",  pA);
			wiMexUtil_PutRealArray(64,  pRX->EVM.cSe2[0],&pA); mxSetField(pEVM,0,"cSe2", pA);
			wiMexUtil_PutRealArray(pRX->N_SYM,pRX->EVM.nSe2[0],&pA); mxSetField(pEVM,0,"nSe2", pA);
			mxSetField(plhs[0],0,"EventFlag",   mxCreateDoubleScalar(pRX->EventFlag.word));

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("bMojave_GetRxState")
		{
			const char *field_names[] = {
				"x","y","z","r","w","u","EDOut","CQOut","CQPeak","EnableTraceCCK","traceCCK","traceState",
				"traceControl","traceDPLL","traceBarker","h1","h2","Np","Np2","Npc","Np2c","k80"
			};
			int dims[2] = {1,1}; int dims2[2] = {1,2};
			int nfields = (sizeof(field_names)/sizeof(*field_names));
			int N22,N11;

			bMojave_RX_State *pbRX;
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
			MCHK(bMojave_StatePointer(&pbRX));

			plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
			if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");

			N22 = pbRX->N22;
			N11 = N22/2;

			mxSetField(plhs[0],0,"x" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N40, pbRX->xReal, pbRX->xImag));
			mxSetField(plhs[0],0,"y" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N40, pbRX->yReal, pbRX->yImag));
			mxSetField(plhs[0],0,"z" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N22, pbRX->zReal, pbRX->zImag));
			mxSetField(plhs[0],0,"r" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N22, pbRX->rReal, pbRX->rImag));
			mxSetField(plhs[0],0,"w" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N11, pbRX->wReal, pbRX->wImag));
			mxSetField(plhs[0],0,"u" ,          wiMexUtil_CreateWORDArrayAsDouble (pbRX->N11, pbRX->uReal, pbRX->uImag));
			if (pbRX->EnableTraceCCK)
			{
				mxSetField(plhs[0],0,"EDOut",       wiMexUtil_CreateUWORDArrayAsDouble (N22, pbRX->EDOut, NULL));
				mxSetField(plhs[0],0,"CQOut",       wiMexUtil_CreateUWORDArrayAsDouble (N22, pbRX->CQOut, NULL));
				mxSetField(plhs[0],0,"traceCCK",    wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->traceCCK, NULL));
				mxSetField(plhs[0],0,"traceState",  wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->traceState, NULL));
				mxSetField(plhs[0],0,"traceControl",wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->traceControl, NULL));
				mxSetField(plhs[0],0,"traceDPLL",   wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->traceDPLL, NULL));
				mxSetField(plhs[0],0,"traceBarker", wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->traceBarker, NULL));
				mxSetField(plhs[0],0,"k80",         wiMexUtil_CreateUWORDArrayAsDouble (N22, (wiUWORD *)pbRX->k80, NULL));
			}
			mxSetField(plhs[0],0,"h1",          wiMexUtil_CreateWORDArrayAsDouble (pbRX->Nhc, pbRX->hcReal, pbRX->hcImag));
			mxSetField(plhs[0],0,"h2",          wiMexUtil_CreateWORDArrayAsDouble (pbRX->Nh, pbRX->hReal, pbRX->hImag));
			mxSetField(plhs[0],0,"CQPeak",      mxCreateDoubleScalar(pbRX->CQPeak.word));
			mxSetField(plhs[0],0,"Np",          mxCreateDoubleScalar(pbRX->Np));
			mxSetField(plhs[0],0,"Np2",         mxCreateDoubleScalar(pbRX->Np2));
			mxSetField(plhs[0],0,"Npc",         mxCreateDoubleScalar(pbRX->Npc));
			mxSetField(plhs[0],0,"Np2c",        mxCreateDoubleScalar(pbRX->Np2c));
			mxSetField(plhs[0],0,"EnableTraceCCK",mxCreateDoubleScalar(pbRX->EnableTraceCCK));

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_GetSamplesEVM")
		{
			int NxEVM = 0;
			wiComplex *x[2] = {0,0};
			Mojave_RX_State *pRX;

			wiMexUtil_CheckParameters(nlhs,nrhs,2,1);
			MCHK(Mojave_StatePointer(NULL, &pRX, NULL));
			if (!pRX->Fault)
			{
				int i,j,k=0;
				NxEVM = 64 * pRX->EVM.N_SYM;
				if (!NxEVM) mexErrMsgTxt("EVM Arrays are Empty");
				if (!pRX->EVM.x[0][0] & !pRX->EVM.x[1][0]) mexErrMsgTxt("EVM.x[*][0] is empty");
				x[0] = (wiComplex *)mxCalloc(NxEVM, sizeof(wiComplex));
				x[1] = (wiComplex *)mxCalloc(NxEVM, sizeof(wiComplex));
				if (!x[0] || !x[1]) mexErrMsgTxt("Memory Allocation Error");
            
				for (i=k=0; i<pRX->EVM.N_SYM; i++) {
					for (j=0; j<64; j++, k++) {
						x[0][k] = pRX->EVM.x[0][j][i];
						x[1][k] = pRX->EVM.x[1][j][i];
					}
				}
				if (!pRX->Fault)
				{
					wiMexUtil_PutComplexArray(NxEVM, x[0], &plhs[0]);
					wiMexUtil_PutComplexArray(NxEVM, x[1], &plhs[1]);
				}
				else
				{
					wiMexUtil_PutComplexArray(0, NULL, &plhs[0]);
					wiMexUtil_PutComplexArray(0, NULL, &plhs[1]);
				}
				if (x[0]) mxFree(x[0]);
				if (x[1]) mxFree(x[1]);
			}
			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_TxPacket")
		{
			int Length, Nx;
			wiComplex *x;
			wiUWORD *MAC_TX_D;
			Mojave_TX_State *pTX;

			wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
			MAC_TX_D = wiMexUtil_GetUWORDArray(prhs[1], &Length );

			MCHK(Mojave_TX(Length, MAC_TX_D, &Nx, &x, 1));
			MCHK(Mojave_StatePointer(&pTX, NULL, NULL));
			wiMexUtil_PutWORDPairArray(pTX->Nz, pTX->zReal, pTX->zImag, &plhs[0]);

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_ReadRegisterMap")
		{
			wiUWORD *xRegMap;
			wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
			xRegMap = wiMexUtil_GetUWORDArray(prhs[1], NULL);
			MCHK(Mojave_SetRegisterMap(xRegMap));
			if (xRegMap) mxFree(xRegMap);

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_WriteRegisterMap")
		{
			wiUWORD *RegMap;   // register map (Tx/Rx combined)
			wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
			MCHK(Mojave_GetRegisterMap(&RegMap));
			wiMexUtil_PutUWORDArray(256, RegMap, &plhs[0]);

			return WI_SUCCESS;
		}
		//-------------------------------------------------------------------------------
		if COMMAND("Mojave_WriteConfig")
		{
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");
			mexPrintf(" wiseMex (%s): Mojave Configuration\n",__DATE__);
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");
			MCHK( Mojave_WriteConfig(wiMexUtil_Output) );
			mexPrintf(" -----------------------------------------------------------------------------------------------\n");

			return WI_SUCCESS;
		}

	#else // no header loaded; therefore, no functions to call

		mexErrMsgTxt("wiseMex: Mojave_ command received but Mojave.h not included.");

	#endif  // MOJAVE_H

	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_Mojave()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

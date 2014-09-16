//--< wiseMex_Nevada.c >---------------------------------------------------------------------------
//=================================================================================================
//
//  wiseMex Command Dispatch for the Nevada method
//  Copyright 2008-2009 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <string.h>
#include "wiseMex.h"

#define COMMAND(TargetStr) (wiMexUtil_CommandMatch(cmd, TargetStr))
#define MATCH(s1,s2)       ((strcmp(s1,s2) == 0) ? 1:0)

// ================================================================================================
// FUNCTION  : wiseMex_Nevada()
// ------------------------------------------------------------------------------------------------
// Purpose   : Command lookup and execute from wiseMex
// Parameters: nlhs -- Number of passed left-hand-side parameters
//             plhs -- Pointer to the left-hand-side parameter list
//             nrhs -- Number of passed right-hand-side arguments
//             prhs -- Pointer to the right-hand-side argument list
// ================================================================================================
wiStatus wiseMex_Nevada(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[], char *cmd)
{
	if (strstr(cmd, "Nevada_"))
    {
	#ifdef __NEVADA_H

        //---------------------------------------------------------------------------
	    if COMMAND("Nevada_GetDvParameter")
	    {
            char *Param;
		    Nevada_DvState_t *pState = Nevada_DvState();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Param = wiMexUtil_GetStringValue(prhs[1],0);

                 if MATCH(Param,"VerilogChecker"   ) wiMexUtil_PutIntValue    (pState->VerilogChecker,   &plhs[0]);
            else if MATCH(Param,"ExpectPacketFail" ) wiMexUtil_PutBooleanValue(pState->ExpectPacketFail, &plhs[0]);
            else if MATCH(Param,"LongSimulation"   ) wiMexUtil_PutBooleanValue(pState->LongSimulation,   &plhs[0]);
            else if MATCH(Param,"BreakCheck"       ) wiMexUtil_PutBooleanValue(pState->BreakCheck,       &plhs[0]);
            else if MATCH(Param,"NumCheckRegisters") wiMexUtil_PutUIntValue   (pState->NumCheckRegisters,&plhs[0]);
            else if MATCH(Param,"CheckRegAddr"     ) wiMexUtil_PutUIntArray   (pState->NumCheckRegisters, pState->CheckRegAddr, &plhs[0]);
            else if MATCH(Param,"TxStartMinT80"    ) wiMexUtil_PutUIntValue   (pState->TxStartMinT80,    &plhs[0]);
            else if MATCH(Param,"TxStartMaxT80"    ) wiMexUtil_PutUIntValue   (pState->TxStartMaxT80,    &plhs[0]);
            else if MATCH(Param,"TxEndMinT80"      ) wiMexUtil_PutUIntValue   (pState->TxEndMinT80,      &plhs[0]);
            else if MATCH(Param,"TxEndMaxT80"      ) wiMexUtil_PutUIntValue   (pState->TxEndMaxT80,      &plhs[0]);
            else if MATCH(Param,"kStart"           ) wiMexUtil_PutUIntValue   (pState->kStart,           &plhs[0]);
            else if MATCH(Param,"kEnd"             ) wiMexUtil_PutUIntValue   (pState->kEnd,             &plhs[0]);
            else if MATCH(Param,"aTxStart"         ) wiMexUtil_PutUIntValue   (pState->aTxStart,         &plhs[0]);
            else if MATCH(Param,"bTxStart"         ) wiMexUtil_PutUIntValue   (pState->bTxStart,         &plhs[0]);
            else if MATCH(Param,"aTxEnd"           ) wiMexUtil_PutUIntValue   (pState->aTxEnd,           &plhs[0]);
            else if MATCH(Param,"bTxEnd"           ) wiMexUtil_PutUIntValue   (pState->bTxEnd,           &plhs[0]);
            else if MATCH(Param,"TestLimit"        ) wiMexUtil_PutIntValue    (pState->TestLimit,        &plhs[0]);
            else if MATCH(Param,"N_PHY_TX_RD"      ) wiMexUtil_PutUIntArray   (NEVADA_MAX_DV_TX, pState->N_PHY_TX_RD, &plhs[0]);
            else if MATCH(Param,"MAC_TX_D"         ) wiMexUtil_PutUIntArray   (pState->N_PHY_TX_RD[0], (wiUInt *)(pState->MAC_TX_D[0]), &plhs[0]);
            else if MATCH(Param,"MAC_TX_D[0]"      ) wiMexUtil_PutUIntArray   (pState->N_PHY_TX_RD[0], (wiUInt *)(pState->MAC_TX_D[0]), &plhs[0]);
            else if MATCH(Param,"MAC_TX_D[1]"      ) wiMexUtil_PutUIntArray   (pState->N_PHY_TX_RD[1], (wiUInt *)(pState->MAC_TX_D[1]), &plhs[0]);
            else if MATCH(Param,"MAC_TX_D[2]"      ) wiMexUtil_PutUIntArray   (pState->N_PHY_TX_RD[2], (wiUInt *)(pState->MAC_TX_D[2]), &plhs[0]);
            else if MATCH(Param,"MAC_TX_D[3]"      ) wiMexUtil_PutUIntArray   (pState->N_PHY_TX_RD[3], (wiUInt *)(pState->MAC_TX_D[3]), &plhs[0]);
            else 
                mexErrMsgTxt("Nevada_GetDvParameter: parameter not available");

            mxFree(Param);

            return WI_SUCCESS;
        }
	    //---------------------------------------------------------------------------
	    if COMMAND("Nevada_GetTxParameter")
	    {
            char *Param;
		    Nevada_TxState_t *pState = Nevada_TxState();

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Param = wiMexUtil_GetStringValue(prhs[1],0);

                    if MATCH(Param,"TxMode"        ) wiMexUtil_PutUIntValue(pState->TxMode,         &plhs[0]);
            else if MATCH(Param,"RATE"          ) wiMexUtil_PutUIntValue(pState->RATE.w7,        &plhs[0]);
            else if MATCH(Param,"LENGTH"        ) wiMexUtil_PutUIntValue(pState->LENGTH.w16,     &plhs[0]);
            else if MATCH(Param,"N_SYM"         ) wiMexUtil_PutUIntValue(pState->N_SYM,          &plhs[0]);
            else if MATCH(Param,"TimingShift"   ) wiMexUtil_PutRealValue(pState->TimingShift,    &plhs[0]);
            else if MATCH(Param,"PacketDuration") wiMexUtil_PutRealValue(pState->PacketDuration, &plhs[0]);
            else if MATCH(Param,"bResamplePhase") wiMexUtil_PutIntValue (pState->bResamplePhase, &plhs[0]);
            else if MATCH(Param,"a"    ) wiMexUtil_PutUWORDArray   (pState->Na, pState->a,  &plhs[0]);
            else if MATCH(Param,"b"    ) wiMexUtil_PutUWORDArray   (pState->Nb, pState->b,  &plhs[0]);
            else if MATCH(Param,"c"    ) wiMexUtil_PutUWORDArray   (pState->Nc, pState->c,  &plhs[0]);
            else if MATCH(Param,"p"    ) wiMexUtil_PutUWORDArray   (pState->Np, pState->p,  &plhs[0]);
            else if MATCH(Param,"v"    ) wiMexUtil_PutWORDPairArray(pState->Nv, pState->vReal, pState->vImag, &plhs[0]);
            else if MATCH(Param,"x"    ) wiMexUtil_PutWORDPairArray(pState->Nx, pState->xReal, pState->xImag, &plhs[0]);
            else if MATCH(Param,"y"    ) wiMexUtil_PutWORDPairArray(pState->Ny, pState->yReal, pState->yImag, &plhs[0]);
            else if MATCH(Param,"z"    ) wiMexUtil_PutWORDPairArray(pState->Nz, pState->zReal, pState->zImag, &plhs[0]);
            else if MATCH(Param,"q"    ) wiMexUtil_PutWORDPairArray(pState->Nz, pState->qReal, pState->qImag, &plhs[0]);
            else if MATCH(Param,"r"    ) wiMexUtil_PutWORDPairArray(pState->Nz, pState->rReal, pState->rImag, &plhs[0]);
            else if MATCH(Param,"s"    ) wiMexUtil_PutWORDPairArray(pState->Nz, pState->sReal, pState->sImag, &plhs[0]);
            else if MATCH(Param,"d"    ) wiMexUtil_PutComplexArray (pState->Nd, pState->d,  &plhs[0]);
            else if MATCH(Param,"Trace.kMarker") wiMexUtil_PutUIntArray(5, pState->Trace.kMarker, &plhs[0]);
            else if MATCH(Param,"traceTx") wiMexUtil_PutUIntArray(pState->Nd, (unsigned *)(pState->traceTx), &plhs[0]);
            else if MATCH(Param,"Checksum.Enable"  ) wiMexUtil_PutBooleanValue(pState->Checksum.Enable,   &plhs[0]);
            else if MATCH(Param,"Checksum.MAC_TX_D") wiMexUtil_PutUIntValue   (pState->Checksum.MAC_TX_D, &plhs[0]);
            else if MATCH(Param,"Checksum.a")        wiMexUtil_PutUIntValue   (pState->Checksum.a,        &plhs[0]);
            else if MATCH(Param,"Checksum.u")        wiMexUtil_PutUIntValue   (pState->Checksum.u,        &plhs[0]);
            else if MATCH(Param,"Checksum.v")        wiMexUtil_PutUIntValue   (pState->Checksum.v,        &plhs[0]);
            else if MATCH(Param,"Checksum.x")        wiMexUtil_PutUIntValue   (pState->Checksum.x,        &plhs[0]);
            else if MATCH(Param,"LgSigAFE"  )        wiMexUtil_PutIntValue    (pState->LgSigAFE,          &plhs[0]);
            else 
                mexErrMsgTxt("Nevada_GetTxParameter: parameter not available");

            mxFree(Param);

            return WI_SUCCESS;
        }

	    //---------------------------------------------------------------------------
	    if COMMAND("Nevada_GetRxParameter")
	    {
            char *Param;
		    Nevada_RxState_t  *pRX = Nevada_RxState();
            Nevada_bRxState_t *pbRX   = Nevada_bRxState();
            unsigned int N80 = pRX->N80;
            unsigned int N20 = pRX->N20;
            unsigned int N22 = pRX->N22;

            wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            Param = wiMexUtil_GetStringValue(prhs[1],0);

                    if MATCH(Param,"PHY_RX_D") wiMexUtil_PutUWORDArray(pRX->N_PHY_RX_WR, pRX->PHY_RX_D, &plhs[0]);
            else if MATCH(Param,"MAC_NOFILTER"  ) wiMexUtil_PutBooleanValue(pRX->MAC_NOFILTER,        &plhs[0]);
            else if MATCH(Param,"x_ofs") wiMexUtil_PutIntValue     (pRX->x_ofs, &plhs[0]);
            else if MATCH(Param,"a"    ) wiMexUtil_PutUWORDArray   (pRX->Na, pRX->a,  &plhs[0]);
            else if MATCH(Param,"b"    ) wiMexUtil_PutUWORDArray   (pRX->Nb, pRX->b,  &plhs[0]);
            else if MATCH(Param,"r"    ) wiMexUtil_PutWORDPairArray(pRX->Nr, pRX->rReal[0], pRX->rImag[0], &plhs[0]);
            else if MATCH(Param,"c"    ) wiMexUtil_PutWORDPairArray(pRX->Nr, pRX->cReal[0], pRX->cImag[0], &plhs[0]);
            else if MATCH(Param,"d"    ) wiMexUtil_PutWORDPairArray(pRX->Nd, pRX->dReal,    pRX->dImag,    &plhs[0]);
            else if MATCH(Param,"D"    ) wiMexUtil_PutWORDPairArray(pRX->Nd, pRX->DReal,    pRX->DImag,    &plhs[0]);
            else if MATCH(Param,"t"    ) wiMexUtil_PutWORDPairArray(pRX->Nr, pRX->tReal[0], pRX->tImag[0], &plhs[0]);
            else if MATCH(Param,"t[0]" ) wiMexUtil_PutWORDPairArray(pRX->Nr, pRX->tReal[0], pRX->tImag[0], &plhs[0]);
            else if MATCH(Param,"t[1]" ) wiMexUtil_PutWORDPairArray(pRX->Nr, pRX->tReal[1], pRX->tImag[1], &plhs[0]);
            else if MATCH(Param,"EventFlag"      ) wiMexUtil_PutUIntValue   (pRX->EventFlag.word,       &plhs[0]);
            else if MATCH(Param,"aPacketDuration") wiMexUtil_PutUIntValue   (pRX->aPacketDuration.word, &plhs[0]);
            else if MATCH(Param,"EnableTrace"    ) wiMexUtil_PutBooleanValue(pRX->EnableTrace,          &plhs[0]);
            else if MATCH(Param,"EnableModelIO"  ) wiMexUtil_PutBooleanValue(pRX->EnableModelIO,        &plhs[0]);
            else if MATCH(Param,"EnableIQCal"    ) wiMexUtil_PutBooleanValue(pRX->EnableIQCal,          &plhs[0]);
            else if MATCH(Param,"NonRTL.Enabled" ) wiMexUtil_PutBooleanValue(pRX->NonRTL.Enabled,       &plhs[0]);
            else if MATCH(Param,"NonRTL.GainUpdateType") wiMexUtil_PutIntArray(8, (int *)pRX->NonRTL.GainUpdateType, &plhs[0]);
            else if MATCH(Param,"bPacketDuration") wiMexUtil_PutUIntValue   (pbRX->bPacketDuration.word,   &plhs[0]);
            else if MATCH(Param,"bLENGTH")         wiMexUtil_PutUIntValue   (pbRX->bLENGTH.word,           &plhs[0]);
            else if MATCH(Param,"bRATE")           wiMexUtil_PutUIntValue   (pbRX->bRATE.word,             &plhs[0]);
            else if MATCH(Param,"RefPwrAGC")       wiMexUtil_PutUIntValue   (pRX->RefPwrAGC,               &plhs[0]);
            else if MATCH(Param,"ADC.nEff")        wiMexUtil_PutIntValue    (pRX->ADC.nEff,   &plhs[0]);
            else if MATCH(Param,"ADC.Offset")      wiMexUtil_PutIntValue    (pRX->ADC.Offset, &plhs[0]);
            else if MATCH(Param,"ADC.Vpp")         wiMexUtil_PutRealValue   (pRX->ADC.Vpp,    &plhs[0]);
            else if MATCH(Param,"ADC.Rounding")    wiMexUtil_PutBooleanValue(pRX->ADC.Rounding,&plhs[0]);
            else if MATCH(Param,"aSigOut")         wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->aSigOut,       &plhs[0]);
            else if MATCH(Param,"bSigOut")         wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->bSigOut,       &plhs[0]);
            else if MATCH(Param,"traceRxClock")    wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceRxClock,  &plhs[0]);
            else if MATCH(Param,"traceRxState")    wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceRxState,  &plhs[0]);
            else if MATCH(Param,"traceRxControl")  wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceRxControl,&plhs[0]);
            else if MATCH(Param,"traceDFE")        wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceDFE,      &plhs[0]);
            else if MATCH(Param,"traceAGC")        wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceAGC,      &plhs[0]);
            else if MATCH(Param,"traceRSSI")       wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceRSSI,     &plhs[0]);
            else if MATCH(Param,"traceDFS")        wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceDFS,      &plhs[0]);
            else if MATCH(Param,"traceSigDet")     wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceSigDet,   &plhs[0]);
            else if MATCH(Param,"traceFrameSync")  wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceFrameSync,&plhs[0]);
            else if MATCH(Param,"traceRadioIO")    wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceRadioIO,  &plhs[0]);
            else if MATCH(Param,"traceDataConv")   wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceDataConv, &plhs[0]);
            else if MATCH(Param,"traceRX2")        wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceRX2,      &plhs[0]);
            else if MATCH(Param,"traceRx")         wiMexUtil_PutUIntArray(N80, (unsigned *)pRX->traceRx,       &plhs[0]);
            else if MATCH(Param,"traceImgDet")     wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceImgDet,   &plhs[0]);
            else if MATCH(Param,"traceIQCal")      wiMexUtil_PutUIntArray(N20, (unsigned *)pRX->traceIQCal,    &plhs[0]);
            else if MATCH(Param,"bRX.traceState")  wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceState,   &plhs[0]);
            else if MATCH(Param,"bRX.traceControl")wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceControl, &plhs[0]);
            else if MATCH(Param,"bRX.traceDPLL"   )wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceDPLL,    &plhs[0]);
            else if MATCH(Param,"bRX.traceBarker" )wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceBarker,  &plhs[0]);
            else if MATCH(Param,"bRX.traceDSSS"   )wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceDSSS,    &plhs[0]);
            else if MATCH(Param,"bRX.traceCCK"    )wiMexUtil_PutUIntArray(N22, (unsigned *)pbRX->traceCCK,     &plhs[0]);
            else 
                mexErrMsgTxt("Nevada_GetRxParameter: parameter not available");

            mxFree(Param);

            return WI_SUCCESS;
        }
	    //---------------------------------------------------------------------------
	    if COMMAND("Nevada_XXX")
	    {
		    Nevada_RxState_t  *pRX = Nevada_RxState();
            wiInt *vReal, *vImag, Nv, i;

            wiMexUtil_CheckParameters(nlhs,nrhs,0,3);

            vReal = wiMexUtil_GetIntArray(prhs[1], &Nv);
            vImag = wiMexUtil_GetIntArray(prhs[2], NULL);

            for (i=0; i<Nv; i++)
            {
                pRX->vReal[0][i].word = vReal[i];
                pRX->vImag[0][i].word = vImag[i];
            }
            mxFree(vReal);
            mxFree(vImag);

            return WI_SUCCESS;
        }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_GetTxChecksum")
	    {
		    const char *field_names[] = {"Enable","MAC_TX_D","a","u","v","x"};
		    const mwSize dims[2] = {1,1};
		    int nfields = (sizeof(field_names)/sizeof(*field_names));

		    Nevada_TxState_t *pTX;
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
		    pTX = Nevada_TxState();

		    plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
		    if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
        
            mxSetField(plhs[0],0,"Enable",  mxCreateDoubleScalar(pTX->Checksum.Enable  ));
            mxSetField(plhs[0],0,"MAC_TX_D",mxCreateDoubleScalar(pTX->Checksum.MAC_TX_D));
            mxSetField(plhs[0],0,"a",       mxCreateDoubleScalar(pTX->Checksum.a       ));
            mxSetField(plhs[0],0,"u",       mxCreateDoubleScalar(pTX->Checksum.u       ));
            mxSetField(plhs[0],0,"v",       mxCreateDoubleScalar(pTX->Checksum.v       ));
            mxSetField(plhs[0],0,"x",       mxCreateDoubleScalar(pTX->Checksum.x       ));

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_GetRxFrameCount")
	    {
		    const char *field_names[] = {"Total","BadFCS","ValidFCS","FrameCheck","FrameValid"};
		    const mwSize dims[2] = {1,1};
		    int nfields = (sizeof(field_names)/sizeof(*field_names));

		    Nevada_RxState_t *pRX = Nevada_RxState();
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);

		    plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
		    if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");
        
            mxSetField(plhs[0],0,"Total",     mxCreateDoubleScalar(pRX->FrameCount.Total     ));
            mxSetField(plhs[0],0,"BadFCS",    mxCreateDoubleScalar(pRX->FrameCount.BadFCS    ));
            mxSetField(plhs[0],0,"ValidFCS",  mxCreateDoubleScalar(pRX->FrameCount.ValidFCS  ));
            mxSetField(plhs[0],0,"FrameCheck",mxCreateDoubleScalar(pRX->FrameCount.FrameCheck));
            mxSetField(plhs[0],0,"FrameValid",mxCreateDoubleScalar(pRX->FrameCount.FrameValid));

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_GetTxState")
	    {
		    const char *field_names[] = {
			    "a","b","c","p","v","x","y","z","q","r","s","u","d",
			    "Encoder_Shift_Register","Pilot_Shift_Register","Fault","DAC",
                "FixedTimingShift", "TimingShift","PacketDuration","traceTx"
		    };
		    const char *DAC_field_names[] = {"nEff","Mask","Vpp","c"};
		    const mwSize dims[2] = {1,1};
		    int nfields = (sizeof(field_names)/sizeof(*field_names));
		    mxArray *pDAC;

		    Nevada_TxState_t *pTX;
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
		    pTX = Nevada_TxState();

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
		    if (pTX->qReal) mxSetField(plhs[0],0,"q",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nz, pTX->qReal, pTX->qImag));
		    if (pTX->rReal) mxSetField(plhs[0],0,"r",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nz, pTX->rReal, pTX->rImag));
		    if (pTX->sReal) mxSetField(plhs[0],0,"s",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nz, pTX->sReal, pTX->sImag));
		    if (pTX->uReal) mxSetField(plhs[0],0,"u",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nz, pTX->uReal, pTX->uImag));
		    if (pTX->d)     mxSetField(plhs[0],0,"d",wiMexUtil_CreateComplexArray      (pTX->Nd, pTX->d));

		    if (pTX->c)     
                mxSetField(plhs[0],0,"c",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nc, pTX->c, NULL)); // OFDM
		    else if (pTX->cReal) 
			    mxSetField(plhs[0],0,"c",wiMexUtil_CreateWORDArrayAsDouble (pTX->Nc, (wiWORD *)pTX->cReal, (wiWORD *)pTX->cImag)); // DSSS/CCK

            if (pTX->traceTx) mxSetField(plhs[0],0,"traceTx",wiMexUtil_CreateUWORDArrayAsDouble(pTX->Nz, (wiUWORD *)pTX->traceTx, NULL));

            mxSetField(plhs[0],0,"FixedTimingShift",        mxCreateDoubleScalar(pTX->FixedTimingShift));
            mxSetField(plhs[0],0,"TimingShift",             mxCreateDoubleScalar(pTX->TimingShift));
            mxSetField(plhs[0],0,"PacketDuration",          mxCreateDoubleScalar(pTX->PacketDuration));
		    mxSetField(plhs[0],0,"Encoder_Shift_Register",  mxCreateDoubleScalar(pTX->Encoder_Shift_Register.word));
		    mxSetField(plhs[0],0,"Pilot_Shift_Register",    mxCreateDoubleScalar(pTX->Pilot_Shift_Register.word));
		    mxSetField(plhs[0],0,"Fault",                   mxCreateDoubleScalar(0)); // [BB:090116] Fault removed from TX structure 
		    mxSetField(plhs[0],0,"DAC", pDAC);
		    mxSetField(pDAC,0,   "nEff", mxCreateDoubleScalar(pTX->DAC.nEff));
		    mxSetField(pDAC,0,   "Mask", mxCreateDoubleScalar(pTX->DAC.Mask));
		    mxSetField(pDAC,0,   "Vpp",  mxCreateDoubleScalar(pTX->DAC.Vpp));
		    mxSetField(pDAC,0,   "c",    mxCreateDoubleScalar(pTX->DAC.c));

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_GetRxState")
	    {
		    const char *field_names[] = {
			    "N_SYM","N_DBPS","N_CBPS","N_BPSC","RX_RATE","RX_LENGTH","PHY_RX_D",
                "aLength","rLength","Bypass_CFO","Bypass_PLL","q","c","t","r","z","y","s",
                "w","x","v","u","Lp","Lc","cA","cB","b","a","d","H2","AGain","DGain",
                "DFEState","DFEFlags","AdvDly","cSP","cCP","kFFT","h2","h","Fault",
                "x_ofs","DownsamPhase","NoiseShift","ADC","EventFlag",
                "EnableTrace","EnableModelIO","EnableIQCal","aSigOut","bSigOut",
                "traceRxState","traceRxControl","traceRadioIO","traceDFE","traceAGC",
                "traceRSSI","traceFrameSync","traceSigDet","traceDFS","traceDataConv",
                "traceRX2","traceImgDet","traceIQCal","traceRx","traceH2","traceH"
		    };
		    const char *ADC_field_names[] = {"nEff","Mask","Offset","Vpp","c"};
		    const mwSize dims[2] = {1,1};
		    int nfields = (sizeof(field_names)/sizeof(*field_names));
		    mxArray *pADC, *pA;
		    wiWORD *h2[2], *hReal[2], *hImag[2];
		    unsigned int N80, N40, N20, N22;

		    Nevada_RxState_t *pRX = Nevada_RxState();
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);

		    plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
		    pADC    = mxCreateStructArray(2, dims, 5, ADC_field_names);
		    if (!plhs[0] || !pADC) mexErrMsgTxt("Memory Allocation Error");

		    N80 = pRX->N80; N40 = pRX->N40; N20 = pRX->N20; N22 = pRX->N22;

		    mxSetField(plhs[0],0,"N_SYM",   mxCreateDoubleScalar(pRX->N_SYM));
		    mxSetField(plhs[0],0,"N_DBPS",  mxCreateDoubleScalar(pRX->N_DBPS));
		    mxSetField(plhs[0],0,"N_CBPS",  mxCreateDoubleScalar(pRX->N_CBPS));
		    mxSetField(plhs[0],0,"N_BPSC",  mxCreateDoubleScalar(pRX->N_BPSC));
		    mxSetField(plhs[0],0,"RX_RATE", mxCreateDoubleScalar(pRX->RX_RATE.w7));
		    mxSetField(plhs[0],0,"RX_LENGTH",mxCreateDoubleScalar(pRX->RX_LENGTH.w16));
		    mxSetField(plhs[0],0,"aLength", mxCreateDoubleScalar(pRX->aLength));
		    mxSetField(plhs[0],0,"rLength", mxCreateDoubleScalar(pRX->rLength));
		    mxSetField(plhs[0],0,"PHY_RX_D",wiMexUtil_CreateUWORDArrayAsDouble(pRX->N_PHY_RX_WR, pRX->PHY_RX_D, NULL));
		    mxSetField(plhs[0],0,"q" ,wiMexUtil_CreateUWORDMatrixAsDouble(2, pRX->Nr, pRX->qReal, pRX->qImag));
		    mxSetField(plhs[0],0,"c" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->cReal, pRX->cImag));
		    mxSetField(plhs[0],0,"t" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->tReal, pRX->tImag));
		    mxSetField(plhs[0],0,"r" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->rReal, pRX->rImag));
		    mxSetField(plhs[0],0,"z" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Nr, pRX->zReal, pRX->zImag));
		    mxSetField(plhs[0],0,"y" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->yReal, pRX->yImag));
		    mxSetField(plhs[0],0,"s" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->sReal, pRX->sImag));
		    mxSetField(plhs[0],0,"w" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->wReal, pRX->wImag));
		    mxSetField(plhs[0],0,"x" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->xReal, pRX->xImag));
		    mxSetField(plhs[0],0,"v" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->vReal, pRX->vImag));
		    mxSetField(plhs[0],0,"u" ,wiMexUtil_CreateWORDMatrixAsDouble (2, pRX->Ns, pRX->uReal, pRX->uImag));
		    mxSetField(plhs[0],0,"Lp",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Np, pRX->Lp, NULL));
		    mxSetField(plhs[0],0,"Lc",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nc, pRX->Lc, NULL));
		    mxSetField(plhs[0],0,"cA",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->cB, NULL));
		    mxSetField(plhs[0],0,"cB",wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->cA, NULL));
		    mxSetField(plhs[0],0,"a", wiMexUtil_CreateUWORDArrayAsDouble (pRX->Na, pRX->a,  NULL));
		    mxSetField(plhs[0],0,"b", wiMexUtil_CreateUWORDArrayAsDouble (pRX->Nb, pRX->b,  NULL));
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
		    mxSetField(pADC,   0,"nEff",         mxCreateDoubleScalar(pRX->ADC.nEff));
		    mxSetField(pADC,   0,"Mask",         mxCreateDoubleScalar(pRX->ADC.Mask));
		    mxSetField(pADC,   0,"Offset",       mxCreateDoubleScalar(pRX->ADC.Offset));
		    mxSetField(pADC,   0,"Vpp",          mxCreateDoubleScalar(pRX->ADC.Vpp));
		    mxSetField(pADC,   0,"c",            mxCreateDoubleScalar(pRX->ADC.c));
		    mxSetField(plhs[0],0,"EventFlag",    mxCreateDoubleScalar(pRX->EventFlag.word));
            mxSetField(plhs[0],0,"EnableTrace",  mxCreateDoubleScalar(pRX->EnableTrace));
            mxSetField(plhs[0],0,"EnableModelIO",mxCreateDoubleScalar(pRX->EnableModelIO));
            mxSetField(plhs[0],0,"EnableIQCal",  mxCreateDoubleScalar(pRX->EnableIQCal));

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
			    mxSetField(plhs[0],0,"traceAGC"      ,wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceAGC,       NULL));
			    mxSetField(plhs[0],0,"traceRSSI",     wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceRSSI,      NULL));
			    mxSetField(plhs[0],0,"traceFrameSync",wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceFrameSync, NULL));
			    mxSetField(plhs[0],0,"traceSigDet",   wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceSigDet,    NULL));
			    mxSetField(plhs[0],0,"traceDFS",      wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceDFS,       NULL));
			    mxSetField(plhs[0],0,"traceDataConv", wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceDataConv,  NULL));
			    mxSetField(plhs[0],0,"traceRX2",      wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceRX2,       NULL));
			    mxSetField(plhs[0],0,"traceRx",       wiMexUtil_CreateUWORDArrayAsDouble (N80, (wiUWORD *)pRX->traceRx,        NULL));
			    mxSetField(plhs[0],0,"traceImgDet",   wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceImgDet,    NULL));
			    mxSetField(plhs[0],0,"traceIQCal",    wiMexUtil_CreateUWORDArrayAsDouble (N20, (wiUWORD *)pRX->traceIQCal,     NULL));
                mxSetField(plhs[0],0,"traceH2",       wiMexUtil_CreateWORDArrayAsDouble  (N20, pRX->traceH2[0],                NULL));
                mxSetField(plhs[0],0,"traceH",        wiMexUtil_CreateWORDArrayAsDouble  (N20, pRX->traceHReal[0], pRX->traceHImag[0]));
		    }      
		    mxSetField(plhs[0],0,"EventFlag",   mxCreateDoubleScalar(pRX->EventFlag.word));

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if (COMMAND("bNevada_GetRxState") || COMMAND("Nevada_b_GetRxState"))
	    {
		    const char *field_names[] = {
			    "x","y","z","r","u","w","EDOut","CQOut","CQPeak","traceCCK","traceState","traceControl",
			    "traceDPLL","traceBarker","h1","h2","Np","Np2","Npc","Np2c","k80"
		    };
		    const mwSize dims[2] = {1,1};
		    int nfields = (sizeof(field_names)/sizeof(*field_names));
		    int N22,N11;

            Nevada_bRxState_t *pbRX = Nevada_bRxState();
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);

		    plhs[0] = mxCreateStructArray(2, dims, nfields, field_names);
		    if (!plhs[0]) mexErrMsgTxt("Memory Allocation Error");

		    N22 = pbRX->N22;
		    N11 = N22/2;

		    mxSetField(plhs[0],0,"x" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N40, pbRX->xReal, pbRX->xImag));
		    mxSetField(plhs[0],0,"y" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N40, pbRX->yReal, pbRX->yImag));
		    mxSetField(plhs[0],0,"z" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N22, pbRX->zReal, pbRX->zImag));
		    mxSetField(plhs[0],0,"r" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N22, pbRX->rReal, pbRX->rImag));
		    mxSetField(plhs[0],0,"u" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N11, pbRX->uReal, pbRX->uImag));
		    mxSetField(plhs[0],0,"w" ,          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->N11, pbRX->wReal, pbRX->wImag));
		    if (Nevada_RxState()->EnableTrace)
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
		    mxSetField(plhs[0],0,"h1",          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->Nhc, pbRX->hcReal, pbRX->hcImag));
		    mxSetField(plhs[0],0,"h2",          wiMexUtil_CreateWORDArrayAsDouble  (pbRX->Nh, pbRX->hReal, pbRX->hImag));
		    mxSetField(plhs[0],0,"CQPeak",      mxCreateDoubleScalar(pbRX->CQPeak.word));
		    mxSetField(plhs[0],0,"Np",          mxCreateDoubleScalar(pbRX->Np));
		    mxSetField(plhs[0],0,"Np2",         mxCreateDoubleScalar(pbRX->Np2));
		    mxSetField(plhs[0],0,"Npc",         mxCreateDoubleScalar(pbRX->Npc));
		    mxSetField(plhs[0],0,"Np2c",        mxCreateDoubleScalar(pbRX->Np2c));

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_TxPacket")
	    {
		    int Length, Nx;
		    wiUWORD *MAC_TX_D;
		    wiComplex *x;
		    Nevada_TxState_t *pTX = Nevada_TxState();

		    wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
		    MAC_TX_D = wiMexUtil_GetUWORDArray(prhs[1], &Length );
          		
		    MCHK(Nevada_TX(Length, MAC_TX_D, &Nx, &x, 1));
        
		    wiMexUtil_PutWORDPairArray(pTX->Nz, pTX->zReal, pTX->zImag, &plhs[0]);

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_bTX_ResampleToDAC")
	    {
		    wiMexUtil_CheckParameters(nlhs,nrhs,0,1);
          		
		    MCHK( Nevada_bTX_ResampleToDAC() );
		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_ReadRegister")
	    {
		    wiUInt Addr, Data;

		    wiMexUtil_CheckParameters(nlhs,nrhs,1,2);
            
            Addr = wiMexUtil_GetUIntValue(prhs[1]);
            if (Addr > 1023)
                mexErrMsgTxt("Nevada_ReadRegister: Address out of range (0-1023)");

            Data = Nevada_ReadRegister(Addr);
            wiMexUtil_PutUIntValue(Data, &plhs[0]);
		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_WriteRegister")
	    {
		    wiUInt Addr, Data;

		    wiMexUtil_CheckParameters(nlhs,nrhs,0,3);
            
            Addr = wiMexUtil_GetUIntValue(prhs[1]);
            Data = wiMexUtil_GetUIntValue(prhs[2]);
            if (Addr > 1023)
                mexErrMsgTxt("Nevada_ReadRegister: Address out of range (0-1023)");

            Data = Nevada_WriteRegister(Addr, Data);
		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if (COMMAND("Nevada_ReadRegisterMap") || COMMAND("Nevada_PutRegisterMap"))
	    {
		    wiUInt *pRegMap;
            int NumRegs;

		    wiMexUtil_CheckParameters(nlhs,nrhs,0,2);
		    pRegMap = wiMexUtil_GetUIntArray(prhs[1], &NumRegs);
		    MCHK(Nevada_SetRegisterMap(pRegMap, NumRegs));
		    if (pRegMap) mxFree(pRegMap);

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if (COMMAND("Nevada_WriteRegisterMap") || COMMAND("Nevada_GetRegisterMap"))
	    {
		    wiUWORD *RegMap;   // register map (Tx/Rx combined)
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
		    MCHK(Nevada_GetRegisterMap(&RegMap));
		    wiMexUtil_PutUWORDArray(256, RegMap, &plhs[0]);

		    return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_GetRxHeader")
	    {
		    Nevada_RxState_t *pRX = Nevada_RxState();
		    wiMexUtil_CheckParameters(nlhs,nrhs,1,1);
            wiMexUtil_PutUWORDArray(8, pRX->PHY_RX_D, &plhs[0]);

            return WI_SUCCESS;
	    }
	    //-------------------------------------------------------------------------------
	    if COMMAND("Nevada_WriteConfig")
	    {
		    mexPrintf(" -----------------------------------------------------------------------------------------------\n");
		    mexPrintf(" wiseMex (%s): Nevada Configuration\n",__DATE__);
		    mexPrintf(" -----------------------------------------------------------------------------------------------\n");
		    MCHK( Nevada_WriteConfig(wiMexUtil_Output) );
		    mexPrintf(" -----------------------------------------------------------------------------------------------\n");
		    return WI_SUCCESS;

	    }

	#else // no header loaded; therefore, no functions to call

		mexErrMsgTxt("wiseMex: Nevada_ command received but Nevada.h not included.");

	#endif  // NEVADA_H
    }
	return WI_WARN_PARSE_FAILED; // indicate no command match
}
// end of wiseMex_Nevada()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

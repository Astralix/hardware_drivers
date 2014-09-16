//--< Phy11n_SphereDecoder.c >---------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n_SphereDecoder - Phy11n Receiver Demaper Models (Sphere Decoding)
//  Copyright 2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiUtil.h"
#include "Phy11n.h"

static const int NumDataSubcarriers  = 52;
//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

wiStatus Phy11n_GrayDeMapperFxP(int y[], wiInt *index, wiInt X,  int N_BPSC, wiInt A);
wiStatus Phy11n_GrayMapperFxP(wiInt y[], wiInt index, int M); 

/*===========================================================================+
|  Function  : Phy11n_GrayMapperFxP()                                        |
|----------------------------------------------------------------------------|
|  Purpose   : Gray Mapper for PAM                                           |
|  Parameters: y      -- output array of coded bits                          |
|              index  -- index of PAM elements                               |
|              M      -- # of PAM constellation points                       |
+===========================================================================*/
wiStatus Phy11n_GrayMapperFxP(int y[], wiInt index, int M)
{	

	switch (M)
    {           // ----
    case 2:  // BPSK, QPSK (2-PAM)        
		y[0] = index & 0x1;        
        break;
        // ------
    case 4:  // 16-QAM (4-PAM)
		y[0] = (index>>1)& 0x1; 
		y[1] =  index & 0x1;     
		y[1] = y[0]? (y[1] ^0x1): y[1];
        break;
        // ------
    case 8:  // 8-PAM (64-QAM)
		y[0] = (index>>2) & 0x1;
		
		y[1] = (index>>1) & 0x1;
		y[1] = y[0]? (y[1] ^0x1): y[1];

		y[2] = index & 0x1;
		y[2] = (index>>1) & 0x1 ? (y[2]^0x1) :y[2];       
        break;
        // -------
        
     default:
        printf("\n*** ERROR: Invalid N_BPSC in PHY_802_11n_TX_Modulation()\n\n");
        exit(1);
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RealDemapper()
// ------------------------------------------------------------------------------------------------
// Purpose   : Gray Demapper for PAM
// Parameters: y      -- output array of coded bits
//             X      -- Input array of QAM symbol per dimension
//             N_BPSC -- Number of coded bits per subcarrier
// ================================================================================================
wiStatus Phy11n_RealDemapper(int y[], wiInt *index, wiReal X,  int N_BPSC)
{   	
    wiInt S;
	wiInt M;

	S = (((int) floor(X/2))<<1) +1;
	switch (N_BPSC)
    {           // ----
    case 1:  // BPSK        
    case 2:  // QPSK
        // ----
        M = 2;	
		break;
        // ------
    case 4:  // 16-QAM
        // ------
		M = 4;
		break;
        // ------
    case 6:  // 64-QAM
		M = 8;	
        break;
        // -------
        
     default:
        printf("\n*** ERROR: Invalid N_BPSC in PHY_802_11n_TX_Modulation()\n\n");
        exit(1);
    }
	S = max(-(M-1), S);
	S = min((M-1), S);

	if(index)
	{	*index = (S+(M-1))/2;
		if(y) XSTATUS(Phy11n_GrayMapperFxP(y, *index, M));
	}
	
    return WI_SUCCESS;
}
// end of Phy11n_RealDemapper()


// ================================================================================================
// FUNCTION  : Phy11n_GrayDeMapperFxP()
// ------------------------------------------------------------------------------------------------
// Purpose   : Gray Demapper for PAM
// Parameters: y      -- output array of coded bits
//             X      -- Input array of QAM symbol per dimension
//             N_BPSC -- Number of coded bits per subcarrier
// ================================================================================================
wiStatus Phy11n_GrayDeMapperFxP(int y[], wiInt *index, wiInt X,  int N_BPSC, wiInt A)
{    
	wiInt y1[3];
    
    switch (N_BPSC)
    {           // ----
    case 1:  // BPSK        
    case 2:  // QPSK
        // ----
        y[0] = (X>0)? 1 : 0;  
		y1[0] = y[0];
		if(index) *index = y1[0]; 
        break;
        // ------
    case 4:  // 16-QAM
        // ------
		y[0] = (X>0)? 1 : 0;
		y[1] = (X>(A<<1) || X<-(A<<1)) ?  0 : 1; 
		y1[0] = y[0]; 
		y1[1] = y[0]? (y[1]^0x1) : y[1];  
		if(index) *index = (y1[0]<<1) + y1[1];
        break;
        // ------
    case 6:  // 64-QAM
        y[0] = (X>0)? 1 : 0;
		y[1] = (X>(A<<2) || X<-(A<<2)) ?  0 : 1;
		y[2] = (abs(X)>((A<<2)+(A<<1)) || abs(X)<(A<<1))? 0: 1;
		y1[0] = y[0];
		y1[1] = y[0]? (y[1]^ 0x1): y[1];
		y1[2] = (y[0]^y[1])? (y[2]^0x1): y[2];
		if(index) *index = (y1[0]<<2) + (y1[1]<<1) + y1[2];
        break;
        // -------
        
     default:
        printf("\n*** ERROR: Invalid N_BPSC in PHY_802_11n_TX_Modulation()\n\n");
        exit(1);
    }
    return WI_SUCCESS;
}
// end of Phy11n_GrayDeMapperFxP()

// ================================================================================================
// FUNCTION  : Phy11n_HardDemapperFxP()
// ------------------------------------------------------------------------------------------------
// Purpose   : Gray Demapper for PAM
// Parameters: y      -- output array of coded bits
//             index  -- PAM constellation index
//             yHat   -- Equivalent input signal 
//             R      -- Equivalent channel estimates
//             N_BPSC -- Number of coded bits per subcarrier
// ================================================================================================
wiStatus Phy11n_HardDemapperFxP(int y[], wiInt *index, wiWORD yHat,  wiWORD R, int N_BPSC )
{    
	wiInt y1[3];
	  
    switch (N_BPSC)
    {           // ----
    case 1:  // BPSK        
    case 2:  // QPSK
        // ----
        y[0] =(yHat.word>0)^(R.word>0)? 0:1;		
		y1[0] = y[0];
		if(index) *index = y1[0]; 
        break;
        // ------
    case 4:  // 16-QAM
        // ------
		y[0] = (yHat.word>0)^(R.word>0)? 0:1;		
		y[1] = abs(yHat.word)>(abs(R.word))<<1 ?  0 : 1; 
		y1[0] = y[0]; 
		y1[1] = y[0]? (y[1]^0x1) : y[1];  
		if(index) *index = (y1[0]<<1) + y1[1];
        break;
        // ------
    case 6:  // 64-QAM
        y[0] = (yHat.word>0)^(R.word>0)? 0:1;		
		y[1] = abs(yHat.word)>(abs(R.word))<<2 ?  0 : 1; 
		y[2] = (abs(yHat.word)>((abs(R.word))<<2) + ((abs(R.word))<<1) || abs(yHat.word)<(abs(R.word))<<1)? 0: 1;
		y1[0] = y[0];
		y1[1] = y[0]? (y[1]^ 0x1): y[1];
		y1[2] = (y[0]^y[1])? (y[2]^0x1): y[2];
		if(index) *index = (y1[0]<<2) + (y1[1]<<1) + y1[2];
        break;
        // -------
        
     default:
        printf("\n*** ERROR: Invalid N_BPSC in PHY_802_11n_TX_Modulation()\n\n");
        exit(1);
    }
    return WI_SUCCESS;
}
// end of Phy11n_HardDemapperFxP()

// ================================================================================================
// FUNCTION  : Phy11n_DivisionDemapperFxP()
// ------------------------------------------------------------------------------------------------
// Purpose   : Gray Demapper for PAM
// Parameters: y      -- output array of coded bits
//             X      -- Input array of QAM symbol per dimension
//             N_BPSC -- Number of coded bits per subcarrier
// ================================================================================================
wiStatus Phy11n_DivisionDemapperFxP(int y[], wiInt *index, wiWORD yHat,  wiWORD R, int M)
{    
	wiInt S;
	wiWORD s;

	s.word = R.word? (yHat.word<<9)/R.word: yHat.word; // use Equlized signal
	S = ((s.word>>(1+9))<<1) +1;
	S = max(-(M-1), S);
	S = min((M-1), S);

	if(index)
	{	*index = (S+(M-1))/2;
		if(y) XSTATUS(Phy11n_GrayMapperFxP(y, *index, M));
	}
	
    return WI_SUCCESS;
}
// end of Phy11n_DivisionDemapperFxP()



wiStatus Phy11n_UnsignedLimiter(wiUWORD *x, wiInt N)
{
	x->word = (x->word > (unsigned) ((1<<N) -1))? (unsigned) ((1<<N) -1): x->word;
	return WI_SUCCESS;
}

wiStatus Phy11n_SignedLimiter(wiWORD *x, wiInt N)
{
	x->word = (x->word > (1<<(N-1)) -1)? (1<<(N-1))-1: (x->word<1-(1<<(N-1))? 1-(1<<(N-1)): x->word);
	return WI_SUCCESS;
}

/*====================================================================+
|  Find the first child of the current node
*/
wiStatus Phy11n_MIMO_STS_MCU_FxP(wiInt Child[], wiInt *ChildBits, wiWORD SFL[], 
	wiUWORD d2FL[], wiUWORD PED2FL[], wiInt delta[], wiInt CPoint[], wiWORD *yHatFL, wiInt k, wiWORD yFL[], 
	wiWORD RFL[][8], wiInt N_BPSC, wiInt M,  wiInt A, wiInt Unbiased, wiUWORD NoiseStdFL, wiUWORD NoiseP2FL, wiInt tx2, wiInt Norm1, Phy11n_SingleTreeSearchSphereDecoderFxP_t *pFLState) 
{	
	wiReal s;
	wiInt temp=0;
	wiWORD d;
	wiInt sign, i;

	if(k>tx2)  return STATUS(WI_ERROR_PARAMETER3);
	
	yHatFL[k].word = yFL[k].word; 					
	for (i = k+1; i <tx2; i++)
		yHatFL[k].word = yHatFL[k].word - RFL[k][i].word * SFL[i].w4;

	XSTATUS(Phy11n_SignedLimiter(yHatFL+k, pFLState->Intgy));
	
	s = (double)yHatFL[k].word/ (double) RFL[k][k].word;

//	XSTATUS(Phy11n_RealDemapper(ChildBits+k*max(1, N_BPSC/2), &temp, s, N_BPSC));
	XSTATUS(Phy11n_HardDemapperFxP(ChildBits+k*max(1, N_BPSC/2), Child+k, yHatFL[k], RFL[k][k], N_BPSC));
//	XSTATUS(Phy11n_DivisionDemapperFxP(ChildBits+k*max(1, N_BPSC/2), Child+k, yHatFL[k], RFL[k][k], M));
	SFL[k].word = (Child[k]<<1) -(M-1); 
	d.word = yHatFL[k].word - (RFL[k][k].word*SFL[k].w4);	
	d.word =pFLState->Shiftd? d.word>>pFLState->Shiftd: d.word; 
	XSTATUS(Phy11n_SignedLimiter(&d, pFLState->Intgd));

	d2FL[k].word = Norm1? abs(d.word): d.word*d.word;
	if(Unbiased && (M>2))	
	{
		if(Norm1)
		d2FL[k].word += ((M-1)- abs(SFL[k].w4))*A*NoiseStdFL.word;
		else
		d2FL[k].word += ((M-1)*(M-1)- SFL[k].w4*SFL[k].w4)*A*A*NoiseP2FL.word;
	}

	d2FL[k].word =pFLState->Shiftd2? d2FL[k].word>>pFLState->Shiftd2: d2FL[k].word; 

	XSTATUS(Phy11n_UnsignedLimiter(d2FL+k, pFLState->Intgd2));
		
	if(k==tx2-1)
		PED2FL[k].word = d2FL[k].word;
	else
		PED2FL[k].word = (Norm1 ==2) ? max(PED2FL[k+1].word, d2FL[k].word) : PED2FL[k+1].word + d2FL[k].word;

	XSTATUS(Phy11n_UnsignedLimiter(PED2FL+k, pFLState->IntgM));

	// Beginning point of zig-zag enumeration;
	sign = ((d.word>0)^(RFL[k][k].word>0))? -1: 1;		
	delta[k] =sign;   // sign of d/R.val[k][k].Real;	
	CPoint[k] = 0;

	return WI_SUCCESS;
}

/*====================================================================+
|  Enumerate on the next sibling of current node
*/
wiStatus Phy11n_MIMO_STS_MEU_FxP( wiInt Sib[], wiUWORD d2_SibFL[], wiUWORD PED2FL[], wiUWORD PED2SibFL[], wiInt AllPoints[], 
	wiInt delta[], wiInt CPoint[], wiInt Up[], wiInt Low[],  wiInt k, wiInt Child[], wiWORD yHatFL[], wiWORD RFL[][8],  wiInt M, wiInt A, wiInt Unbiased, wiUWORD NoiseStdFL, wiUWORD NoiseP2FL, wiInt tx2, wiInt Norm1, Phy11n_SingleTreeSearchSphereDecoderFxP_t *pFLState)
{
	wiInt sign;	
	wiWORD S, d;
			
	if(CPoint[k] >= M-1)
	{
		AllPoints[k] = 1;
		return WI_SUCCESS;
	}
	
	if(!CPoint[k]) // get the second child
	{				
		Sib[k] = Child[k];
		Up[k] = 0;
		Low[k] = 0; 
		AllPoints[k] = 0; 
	}
	else 
	{		
		if(Low[k]) 
			delta[k] = 1; // only enumerate in upper direction 
		else if(Up[k])
			delta[k] = -1; // only enumerate in lower direction
		else // zig-zag
		{ 
			sign = (delta[k]>0)? 1: -1; 
			delta[k] = -delta[k] - sign; 
		}
	}
	
	Sib[k] = Sib[k] + delta[k];
	
	while (Sib[k]<0 || Sib[k]>M-1) // find the valid point
	{
		// Set the boundary indication
		if(Sib[k]<0)		
			Low[k] = 1;		
		else		
			Up[k] = 1;
		
		// zig-zag to the other side during the same cycle
		sign = (delta[k]>0)? 1: -1; 
		delta[k] = -delta[k]-sign;
		Sib[k] = Sib[k] + delta[k];
	}

	S.word = (Sib[k]<<1) - (M-1);
	d.word = yHatFL[k].word - RFL[k][k].word*S.w4;
	d.word =pFLState->Shiftd? d.word>>pFLState->Shiftd: d.word; 
	XSTATUS(Phy11n_SignedLimiter(&d, pFLState->Intgd));

	d2_SibFL[k].word = Norm1? abs(d.word) : d.word*d.word;	
	if(Unbiased && (M>2))	
	{
		if(Norm1)
			d2_SibFL[k].word += ((M-1) - abs(S.w4))*A*NoiseStdFL.word;
		else
			d2_SibFL[k].word += ((M-1)*(M-1) - S.w4*S.w4)*A*A*NoiseP2FL.word;
	}
		d2_SibFL[k].word =pFLState->Shiftd2? d2_SibFL[k].word>>pFLState->Shiftd2: d2_SibFL[k].word; 
	XSTATUS(Phy11n_UnsignedLimiter(d2_SibFL+k, pFLState->Intgd2));	
	
	if(k==tx2-1)
		PED2SibFL[k].word = d2_SibFL[k].word;
	else
		PED2SibFL[k].word = Norm1 ==2 ? max(PED2FL[k+1].word, d2_SibFL[k].word) : PED2FL[k+1].word + d2_SibFL[k].word;

	XSTATUS(Phy11n_UnsignedLimiter(PED2SibFL+k, pFLState->IntgM));
	
	CPoint[k] += 1;
		
	return WI_SUCCESS;
}

/*====================================================================================================+
|  Check if the current node meet the criterion
*/
wiStatus Phy11n_MIMO_STS_Prune_FxP(wiInt *NoPrune, wiInt k, wiInt Init, wiInt Child[],  wiInt *ChildBits, 
    wiUWORD d2FL[], wiUWORD PED2FL[], wiInt *ML_bits, wiUWORD LambdaMLFL, wiUWORD *LambdaCounterFL,
	wiInt tx2, wiInt Nb)
{
	wiInt i, j;
	if (k>=tx2)  return STATUS(WI_ERROR_PARAMETER2);

	
	if (Init) 
	{
		*NoPrune = 1;
		return WI_SUCCESS;
	}
	else
	{	
		*NoPrune = 0; 
		
		///////////////////////////////////////////////////////////////////
		// Prune a sub-tree only if it could not update metric ML or any 
		// of the counter-hypothesis
		// The criterion is the relaxed one corresponding to Beta set in
		// the Soft-output VLSI implementation
		//
		if (PED2FL[k].word<LambdaMLFL.word)
		{
			*NoPrune = 1;
			return WI_SUCCESS;
		}

		for (i=k+1; i<tx2; i++) 
			for (j=0; j<Nb; j++)
			{   				
				if ((PED2FL[k].word<LambdaCounterFL[i*Nb+j].word) && (ChildBits[i*Nb+j]^ML_bits[i*Nb+j])) 
				{
					*NoPrune = 1; 									
					return WI_SUCCESS;
				}
			}

		for (i= 0; i<=k; i++)
			for (j = 0; j<Nb; j++)
				if (PED2FL[k].word < LambdaCounterFL[i*Nb+j].word)
				{
					*NoPrune = 1;					
					return WI_SUCCESS;
				}
	}	

	return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_SingleTreeSearchSphereDecoderFxP_Hybrid() 
// Purpose   : Wrapper for Phy11n_SingleTreeSearchSphereDecoderFxP to allow truncation of the
//             search using MMSE.
// ================================================================================================
wiStatus Phy11n_SingleTreeSearchSphereDecoderFxP_Hybrid(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                                     Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], 
                                                     wiReal noise_std)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    XSTATUS( Phy11n_SingleTreeSearchSphereDecoderFxP(Y, H, extr_LLR, prior_LLR, N_BPSC, noise_std) );

	if (pRX->SingleTreeSearchSphereDecoder.Hybrid)
	{
        int i, j, k;
        Phy11nSymbolLLR_t stit_MMSE; // Hybrid solution STS with MMSE

		XSTATUS( Phy11n_MMSE_SingleInversion(Y, H, stit_MMSE, prior_LLR, N_BPSC, noise_std) );
										
		for (k=0; k<NumDataSubcarriers; k++)					
			if(pRX->SingleTreeSearchSphereDecoder.Trunc[k]==1)
			{							
				for (i=0; i<pRX->N_SS; i++)
					for (j = 0; j < pRX->N_BPSC[i]; j++) 
						extr_LLR[i][k*pRX->N_BPSC[i]+j] = stit_MMSE[i][k*pRX->N_BPSC[i]+j];
			}
	}
    return WI_SUCCESS;
}
// end of Phy11n_SingleTreeSearchSphereDecoderFxP_Hybrid()

// ================================================================================================
// FUNCTION  : Phy11n_SingleTreeSearchSphereDecoderFxP() 
// Purpose   : Single Tree Search Sphere Decoder receiver for Y = HX + N,                                                           
//              assume QAM constellations are normalized to have average engergy one per symbol				
//              Do not support STBC                                            
//  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len            
//              H: an arry of channel matrix; each element corresponds to a MIMO channel                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
//              prior_llr: input priori soft bit information matrix            
//              extr_llr:  output extrinsic soft bit information matrix        
//              N_BPSC:    an array of number of bits per symbol per TX        
//              noise_std: white noise standard deviation (complex) 
// ================================================================================================
wiStatus Phy11n_SingleTreeSearchSphereDecoderFxP(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std)
{
    wiInt i, j, p, k, k1, k2, i1, j1;
	// current and next step; 
	wiInt step, nstep, CounterUpdates[24];
	wiReal nNodes, nLeaves, nCycles; 	
    wiInt tx, rx, tx2, rx2, len;
	wiWORD y2FL[8], RFL[8][8], SFL[8], S_MLFL[8], SCounterFL[24][8], dFL;
	wiUWORD d2FL[8], PED2FL[8], d2_SibFL[8], PED2SibFL[8], LambdaMLFL, LMaxFL, RadiusFL, LambdaCounterFL[24];
	wiWORD yHatFL[8];
	wiWORD MetricMLFL, MetricCounterFL;
	wiUWORD NoiseStdFL, NoiseP2FL;
	wiReal LLR, Noise;
	wiInt A = 0; // Scale factor;   	
	wiInt M, Nb; //size of PAM; number of bits for PAM
	wiInt delta[8]; // current delta for enumeration; 
	wiInt Child[8], Sib[8], CPoint[8], AllPoints[8], Permu[8]; // Child and Sib index; CPoint: # of constellation points reached
	wiInt Up[8], Low[8]; // indicating reached upper or lower bound of enumberation
	wiInt ML_bits[24], ChildBits[24];
   	wiInt Init, NoPrune; // Init: Not find a leaf yet. NoPrune:{0: prune a subtree,  1: keep a subtree}
	wiCMat y,y1,y2, H1, Q, QConj, R;
	wiCMat HAug, H1Aug, QAug, Aug; // MMSE-QR
	wiInt print1 = 0, print2 = 0, print3 = 0, print4 = 0, print_QR= 0, print_ML = 0; // STS, LLR, Leaf, Metric, QR
	FILE  *F2, *F4; 		
   	wiReal maxNorm, minNorm, NormR; // Norm of channel estimation	
	wiInt ABPSK = 201;
  	Phy11n_RxState_t *pRX = Phy11n_RxState();
	Phy11n_SingleTreeSearchSphereDecoder_t *pState = &(pRX->SingleTreeSearchSphereDecoder);
	wiInt   N_SYM = pRX->N_SYM;	    
    wiReal *pCycles     = &(pState->AvgCycles);
    wiReal *pLeaves     = &(pState->AvgLeaves);
	wiReal *pNodes      = &(pState->AvgNodes);
	wiInt   ModifiedQR  = pState->ModifiedQR;
	wiInt   Sorted      = pState->Sorted; 
	wiInt   MMSE        = pState->MMSE;
	wiInt   Unbiased    = pState->MMSE? pState->Unbiased: 0;
	wiInt   NSTS        = pState->NSTS;
	wiReal *pNCyclesPSTS  = pState->NCyclesPSTS;
	wiReal *pPH2PSTS      = pState->PH2PSTS; 
	wiReal *pR2RatioPSTS = pState->R2RatioPSTS;
	wiInt  *psym         = &(pState->sym);
	wiInt  Terminated    = pState->Terminated;
	wiInt  BlockSchedule = pState->BlockSchedule;
	wiReal MeanCycles    = pState->MeanCycles;
	wiReal MinCycles     = pState->MinCycles;
	wiInt  *pTrunc       = pState->Trunc;
	
	Phy11n_SingleTreeSearchSphereDecoderFxP_t *pFLState = &(pRX->SingleTreeSearchSphereDecoderFxP);
	
	wiReal MaxCycles; // Maximum allowable Cycles of current subcarrier
	wiReal TotalCycles; // Total budget cycles per block;
	wiReal AccuCycles= 0.0; // Accumulated cycles used per block;
			
	tx = H[0].col; // # of transmit antennas
    rx = H[0].row; // # of receive antennas
	
	len = Y->col;  // # of subcarriers

	if(N_BPSC[0]>1)
	{
		tx2 = 2*tx;
		rx2 = 2*rx;
	}
	else  // Only keep the upper part (real of the expanded real tree)
	{
		tx2 = tx;
		rx2 = 2*rx;
		if(ModifiedQR) return STATUS(WI_ERROR_UNDEFINED_CASE);
	}
		
	if(Terminated && BlockSchedule)
	{
		if(MinCycles>MeanCycles) return STATUS(WI_ERROR_UNDEFINED_CASE);
		TotalCycles = len*MeanCycles; 
	}
 
	///////////////////////////////////////////////////////////////////////////////
	//
	//  Initialize the Matrix every symbol
	//
	// 	
    XSTATUS(wiCMatInit(&y,  rx,  1));	
	XSTATUS(wiCMatInit(&y1, rx2,  1)); // real equivalent	
	XSTATUS(wiCMatInit(&y2, tx2, 1));  // Q^H *y1; equivalent received 	
	XSTATUS(wiCMatInit(&H1, rx2, tx2)); // real equivalent	
	XSTATUS(wiCMatInit(&Q,  rx2, tx2)); // orthoginal matrix in QR		
	XSTATUS(wiCMatInit(&QConj,  tx2, rx2)); 		
	XSTATUS(wiCMatInit(&R,  tx2, tx2)); // upper triangular matrix in QR
	
	if(MMSE)
	{
		XSTATUS(wiCMatInit(&HAug,   rx+tx, tx));
		XSTATUS(wiCMatInit(&Aug,    tx,    tx));
		XSTATUS(wiCMatInit(&H1Aug,  2*(rx+tx), tx2)); 
		XSTATUS(wiCMatInit(&QAug,   2*(rx+tx), tx2));		
	}
	
	if(pState->GetStatistics && !*psym)
	{
		memset(pNCyclesPSTS, 0, NSTS*sizeof(wiReal));
		memset(pPH2PSTS, 0, NSTS*sizeof(wiReal));
	}

	if(pState->Hybrid)			
		memset(pTrunc, 0, NumDataSubcarriers*sizeof(wiInt));
		
	//////////////////////////////
	//
	//  Restrict noise
	//
	//
    if (noise_std<pow(10.0,-8.0))
        noise_std = pow(10.0,-8.0);
	NoiseStdFL.word = (unsigned int) (pFLState->FLScale*ABPSK*noise_std +0.5);
	NoiseP2FL.word = (unsigned int) (pFLState->FLScale*pFLState->FLScale*pow(noise_std, 2.0)*ABPSK*ABPSK+0.5); 
	NoiseStdFL.word = pFLState->Shiftd2? NoiseStdFL.word>>pFLState->Shiftd2: NoiseStdFL.word; 
	NoiseP2FL.word = pFLState->Shiftd2?( NoiseP2FL.word>>pFLState->Shiftd2): NoiseP2FL.word;
	XSTATUS(Phy11n_UnsignedLimiter(&NoiseStdFL, pFLState->Intgn));
	XSTATUS(Phy11n_UnsignedLimiter(&NoiseP2FL, pFLState->IntgM));

	Noise = (double) pState->Norm1? NoiseStdFL.word*NoiseStdFL.word: NoiseP2FL.word;

    LMaxFL.word = (unsigned int) (pState->Norm1?(pState->LMax1*NoiseStdFL.word +0.5):(pState->LMax*NoiseP2FL.word+0.5));	
	XSTATUS(Phy11n_UnsignedLimiter(&LMaxFL, pFLState->IntgM));

	/////////////////////////////
	//
	//  Constellation 
	//
	//
	switch (N_BPSC[0])
    {
        case  1:
			A = 201;
			M = 2;			
			break;
        case  2:
			A = 139;
			M = 2;
			break;
        case  4:
			A = 62;
			M = 4;
			break;
        case  6: 
			A = 31;
			M = 8; 
			break;
		default:
			return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
	Nb = max(1, N_BPSC[0]/2);
	
    for (p=0;p<len;p++) // p: index of subcarriers
    {			
		///////////////////////////////////////////////////////////////////////////////
		//
		//  Initialization the Metric for hypothesis and counter-hypothesis
		//
		// 
		LambdaMLFL.word = (1<<pFLState->IntgM)-1;
    	memset(ML_bits, 0, N_BPSC[0]*tx*sizeof(wiInt));
		memset(ChildBits, 0, N_BPSC[0]*tx*sizeof(wiInt));
		memset(SFL, 0, tx2*sizeof(wiWORD));
		memset(d2FL, 0, tx2*sizeof(wiUWORD));
		memset(PED2FL, 0, tx2*sizeof(wiUWORD));
		memset(PED2SibFL, 0, tx2*sizeof(wiUWORD));
		memset(yHatFL, 0, tx2*sizeof(wiWORD));
		memset(d2_SibFL, 0, tx2*sizeof(wiUWORD));
		memset(delta, 0, tx2*sizeof(wiInt));
		memset(Child, 0, tx2*sizeof(wiInt));
		memset(Sib, 0, tx2*sizeof(wiInt));
		memset(AllPoints, 0, 8*sizeof(wiInt));
		memset(CPoint, 0, tx2*sizeof(wiInt));
		memset(Up, 0, tx2*sizeof(wiInt));
		memset(Low, 0, tx2*sizeof(wiInt));
		memset(Permu, 0, tx2*sizeof(wiInt));
		memset(CounterUpdates, 0, tx2*Nb*sizeof(wiInt));
		memset(S_MLFL, 0, tx2*sizeof(wiWORD));
		memset(SCounterFL, 0, tx2*sizeof(wiWORD));

		RadiusFL.word = (1<<pFLState->IntgM)-1;
		
		for (i=0; i<N_BPSC[0]*tx; i++)
			LambdaCounterFL[i].word =RadiusFL.word;
			
		if(Terminated)				
				MaxCycles = BlockSchedule? TotalCycles - AccuCycles - (len-p-1)*MinCycles:  pState->MaxCycles ;			
					
			   		
		///////////////////////////////////////////////////////////////////////////////
		//
		//  Preprocessing. Get the received matrices; Transform to real algorithm
		//  Expanding the tree to tx2 levels 
		//  Doing QR decomposition; Get the Equivalent channel
		// 	
		XSTATUS(wiCMatGetSubMtx(Y, &y, 0, p, rx-1,p) );
		XSTATUS(wiCMatComplexToReal(&y, &y1, 0));	
		if(MMSE)
		{
			// Get the augmented matrix [H; noise_std*I]
			wiCMatSetIden(&Aug);
			wiCMatScaleSelfReal(&Aug, noise_std);
			wiCMatSetSubMtx(&HAug, H+p, 0, 0, rx-1, tx-1);
			wiCMatSetSubMtx(&HAug, &Aug, rx, 0, rx+tx-1, tx-1);
			XSTATUS(wiCMatComplexToReal(&HAug, &H1Aug, ((N_BPSC[0]>1)?1:0)));
		      
			// QR decomposition of augemented channel matrix
			wiCMatQRWrapper(&H1Aug, &QAug, &R, Permu, ModifiedQR, Sorted);
            wiCMatGetSubMtx(&QAug, &Q, 0, 0, rx2-1, tx2-1);
		}
		else
		{
			XSTATUS(wiCMatComplexToReal(H+p, &H1, ((N_BPSC[0]>1)?1:0)));				
			wiCMatQRWrapper(&H1, &Q, &R, Permu, ModifiedQR, Sorted);
		}
			
        wiCMatConjTrans(&QConj, &Q);

		// Equivalent received signal
		wiCMatMul(&y2, &QConj, &y1);	
		
		// Equivalent finite length signal
		for (i=0; i<tx2; i++)
		{
			y2FL[i].word = (int) (y2.val[i][0].Real*pFLState->FLScale*ABPSK + (y2.val[i][0].Real>0? 0.5: -0.5));
			XSTATUS(Phy11n_SignedLimiter(y2FL+i, pFLState->Intgy));
			
			for(j=0; j<tx2; j++)
			{
				RFL[i][j].word = (int)(R.val[i][j].Real*pFLState->FLScale*A + (R.val[i][j].Real>0? 0.5: -0.5));
				XSTATUS(Phy11n_SignedLimiter(RFL[i]+j, pFLState->Intgy));
			}
		}
				
		///////////////////////////////////////////////////////////////////////////////
		//
		//  Depth first single tree search with soft output
		//  Vertical expansion, horizontal expansion and pruning occurs in parallel
		// 	
		k = tx2-1;  // Start from the root of the tree
		step = 1; // Initial step
		nNodes = 0.0;
		nLeaves = 0.0; // number of leaves reached;
		nCycles = 0.0; 
		NoPrune = 1;
		Init = 1; // Start tree search. Not reached a leaf yet.

		while (k<tx2)
		{				
			switch (step)
			{
			//////////////////////////////////////////////////////////////////////////
			//
			// root level MCU find the best child
			//			
			case 1:  
				k = tx2 -1;						
				XSTATUS(Phy11n_MIMO_STS_MCU_FxP(Child, ChildBits, SFL, d2FL, PED2FL, delta, CPoint, yHatFL, k, y2FL, RFL, N_BPSC[0], M, A, Unbiased, NoiseStdFL, NoiseP2FL, tx2, pState->Norm1, pFLState));
				nCycles += 1.0;
				nstep = 2;
				break;

			//////////////////////////////////////////////////////////////////////////
			//
			// Middle level processing. MCU, MEU and Pruning occurs in parallel
			//		
			case 2: 
				
				// MCU: find the best child in level k-1;
				XSTATUS(Phy11n_MIMO_STS_MCU_FxP(Child, ChildBits, SFL, d2FL, PED2FL, delta, CPoint, yHatFL, k-1, y2FL, RFL, N_BPSC[0], M, A, Unbiased, NoiseStdFL, NoiseP2FL, tx2, pState->Norm1, pFLState));

				// MEU: enumerate the next sibling in level k;
				XSTATUS(Phy11n_MIMO_STS_MEU_FxP(Sib, d2_SibFL, PED2FL, PED2SibFL, AllPoints, delta, CPoint, Up, Low, k, Child, yHatFL, RFL, M, A, Unbiased, NoiseStdFL, NoiseP2FL,tx2, pState->Norm1, pFLState));
				
				// Prune: Check the prune condition for child[k]
				XSTATUS(Phy11n_MIMO_STS_Prune_FxP(&NoPrune, k, Init, Child, ChildBits, d2FL, PED2FL, ML_bits, LambdaMLFL, LambdaCounterFL, tx2, Nb));
			
				if (NoPrune)
				{
					k -= 1;
					nstep = k ? 2: 3;					
				}
				else
				{	
					AllPoints[k] = 1; 
					while (AllPoints[k])
						k += 1;
					if(k<tx2)
					{
						Child[k] = Sib[k];
						d2FL[k].word = d2_SibFL[k].word; 
						PED2FL[k].word = PED2SibFL[k].word;
						SFL[k].word = (Child[k]<<1) -(M-1);
						XSTATUS(Phy11n_GrayMapperFxP(ChildBits+k*Nb, Child[k], M));
					}
					nstep = 2;
				}
				nNodes += 1.0;
				nCycles += 1.0;
				break;
				

			//////////////////////////////////////////////////////////////////////////////
			//
			// Leaf processing. MEU and Pruning occurs in parallel
			// Update Metric for hypothesis and counter-hypothesis
			// Need to enumerate all leaves because of the soft-output
			//
			case 3: 
				
				// MEU:
				XSTATUS(Phy11n_MIMO_STS_MEU_FxP(Sib,d2_SibFL, PED2FL, PED2SibFL, AllPoints, delta, CPoint, Up, Low, k,  Child, yHatFL, RFL, M, A, Unbiased, NoiseStdFL, NoiseP2FL, tx2, pState->Norm1, pFLState));
				// Prune
				XSTATUS(Phy11n_MIMO_STS_Prune_FxP(&NoPrune, k, Init, Child, ChildBits, d2FL, PED2FL, ML_bits, LambdaMLFL, LambdaCounterFL, tx2, Nb));
				
				if (NoPrune)
				{
					/////////////////////////////////////////////
					//
					// If new hypothesis found, update all valid
					// counter-hypothesis followed by ML updates
					//
					if (PED2FL[0].word< LambdaMLFL.word)
					{
						for(i=0; i<Nb*tx2; i++)
						{
							if(ChildBits[i]^ML_bits[i])
							{
								LambdaCounterFL[i].word =  LambdaMLFL.word; 
								if(!Init)
								{
									CounterUpdates[i] = 1;
									for(j=0; j<tx2; j++)
										SCounterFL[i][j].word = S_MLFL[j].word;
								}
								ML_bits[i] = ChildBits[i];
							}							
						}
						LambdaMLFL.word = PED2FL[0].word;
						for (j=0; j<tx2; j++)
							S_MLFL[j].word = SFL[j].word;

						/////////////////////////////////////////////
						// LLR Cliping to LMax
						
						if(pState->LLRClip)
						{
							RadiusFL.word = LambdaMLFL.word + LMaxFL.word;
							XSTATUS(Phy11n_UnsignedLimiter(&RadiusFL, pFLState->IntgM));

							for (i=0; i<Nb*tx2; i++)
								LambdaCounterFL[i].word = (LambdaCounterFL[i].word> RadiusFL.word)? RadiusFL.word : LambdaCounterFL[i].word;
						}
	
					} // ML

					////////////////////////////////////////////////
					//
					// Update one or more counter-hypothesis metrics
					//
					else
					{
						for(i=0; i<Nb*tx2; i++)		
						{
							if((ChildBits[i]^ML_bits[i]) && (PED2FL[0].word <LambdaCounterFL[i].word))
							{
								CounterUpdates[i] = 1;
								LambdaCounterFL[i].word = PED2FL[0].word;
								for(j=0; j<tx2; j++)
										SCounterFL[i][j].word = SFL[j].word;
							}
						}							
						
					} // counter-hypo
					Init = 0; // Reached a leaf
					nstep = 3; // x.yu visit another leaf
				} // if Noprune
										
				else  // if pruned visit upper level
				{
					k += 1;
					while(AllPoints[k])
						k +=1;					
					nstep = 2;
				}

				/////////////////////////////////////////////
				//
				// Update the next node;
				//
				if(k<tx2)
				{
					Child[k] = Sib[k];
					d2FL[k].word = d2_SibFL[k].word;
					PED2FL[k].word = PED2SibFL[k].word;
					SFL[k].word = (Child[k]<<1) -(M-1);
					XSTATUS(Phy11n_GrayMapperFxP(ChildBits+k*Nb, Child[k], M));
				}
				nNodes += 1.0;
				nCycles += 1.0;
				nLeaves += 1.0;
				break;

			default:
				printf("\n*** ERROR: Invalid STS SD state\n\n");
				exit(1);				
			} // switch;
		
			step = nstep; // update step;

			if(Terminated && nCycles >=MaxCycles) 
			{
				if(pState->Hybrid) pTrunc[p] = 1;				
				break;
			}

		}  // while k<tx2 loop;		

		if(print4) 
		{
			F4 = fopen("MetricFL.txt", "at");
			fprintf(F4, " Metric for p = %d  LambdaMLFL = %d \n", p, LambdaMLFL.word);
			
			for(i=0; i<Nb*tx2; i++)
				fprintf(F4, " ML_bits[%d] = %d LambdaCounter[%d] = %d \n", i, ML_bits[i], i, LambdaCounterFL[i].word);
			fprintf(F4, "\n");
			fclose(F4);
		}

		//////////////////////////////////////////////////////////////////
		//
		// Generate soft output according to MAX-Log-MAP
		// Prior information is not used because it is soft-output only
		// 
		//
		if(print2) 	F2 = fopen("LLRFL.txt", "at");
		if(pState->Norm1 && pState->ExactLLR)	// Recalculate Metric according to L2-Norm	
		{	
			PED2FL[0].word = 0;
			for(i=0; i<tx2; i++)
			{				
				yHatFL[i].word = y2FL[i].word;
				for (j=i+1; j<tx2; j++)
					yHatFL[i].word -= RFL[i][j].word*S_MLFL[j].word;
	//			XSTATUS(Phy11n_SignedLimiter(yHatFL+i, pFLState->Intgy));
				dFL.word = yHatFL[i].word - RFL[i][i].word*S_MLFL[i].word;
	//			XSTATUS(Phy11n_SignedLimiter(&dFL, pFLState->Intgd));
				d2FL[i].word = dFL.word*dFL.word;
	//			XSTATUS(Phy11n_UnsignedLimiter(d2FL+i, pFLState->Intgd2));
				PED2FL[0].word += d2FL[i].word;	
	//			XSTATUS(Phy11n_UnsignedLimiter(PED2FL, pFLState->IntgM));
			}
			MetricMLFL.word = PED2FL[0].word;
		}
		else if(pState->Norm1)
			MetricMLFL.word = LambdaMLFL.word*LambdaMLFL.word; // for ML (positive value)
		else
			MetricMLFL.word = LambdaMLFL.word; // for ML (positive value)

		for(i=0; i<tx2; i++)
		{
			k1 = Permu[i];
			for (j = 0; j<Nb; j++)
			{
				if(pState->Norm1 && pState->ExactLLR && CounterUpdates[i*Nb+j])
				{
					PED2FL[0].word = 0;
					for (i1 =0; i1<tx2; i1++)
					{
						yHatFL[i1].word = y2FL[i1].word;
						for (j1=i1+1; j1<tx2; j1++)
							yHatFL[i1].word -= RFL[i1][j1].word*SCounterFL[i*Nb+j][j1].word;
		//				XSTATUS(Phy11n_SignedLimiter(yHatFL+i1, pFLState->Intgy));
						dFL.word = yHatFL[i1].word - RFL[i1][i1].word*SCounterFL[i*Nb+j][i1].word;
		//				XSTATUS(Phy11n_SignedLimiter(&dFL, pFLState->Intgd));
						d2FL[i1].word = dFL.word*dFL.word;
		//				XSTATUS(Phy11n_UnsignedLimiter(d2FL+i1, pFLState->Intgd2));
						PED2FL[0].word += d2FL[i1].word;	
		//				XSTATUS(Phy11n_UnsignedLimiter(PED2FL, pFLState->IntgM));
					}
					MetricCounterFL.word = PED2FL[0].word;
				}
				else if(pState->Norm1)
					MetricCounterFL.word = LambdaCounterFL[i*Nb+j].word*LambdaCounterFL[i*Nb+j].word;
				else
					MetricCounterFL.word = LambdaCounterFL[i*Nb+j].word; // positive
				
				LLR = (double) (MetricMLFL.word - MetricCounterFL.word);
						
				if(N_BPSC[0]>1)
				{
					k2 = k1>>1;					
					extr_LLR[k2][p*N_BPSC[k2] + (k1&0x1)*Nb + j] = (ML_bits[i*Nb+j]? -LLR : LLR)/(Noise? Noise: 1.0);


				}
				else // BPSK
				{					
					extr_LLR[k1][p*N_BPSC[k1]+j] = (ML_bits[i*Nb+j]? -LLR: LLR)/(Noise? Noise:1.0);

				}			
			}
		}
			if(print2)
			{
				fprintf(F2, "\n\n");
				fclose(F2);
			}

		if(pState->GetStatistics)
		{			
			*pCycles +=  nCycles/(52*N_SYM);
			*pLeaves +=  nLeaves/(52*N_SYM);
			*pNodes  +=  nNodes/(52*N_SYM);
			pNCyclesPSTS[*psym*52+p] = nCycles;
			pPH2PSTS[*psym*52+p] = wiCMatFrobeNormSquared(H+p);	

			for(i=0; i<tx2; i++)
			{
				NormR = pow(R.val[i][i].Real, 2.0); 

				if(!i)
				{
					maxNorm = NormR;
					minNorm = NormR;
				}
				else
				{
					if(NormR > maxNorm) maxNorm = NormR;
					if(NormR < minNorm) minNorm = NormR;
				}
			}
			pR2RatioPSTS[*psym*52+p] = (minNorm>1.0e-15)? maxNorm/minNorm : 0.0;			
		}
		if(Terminated && BlockSchedule) AccuCycles += nCycles; // accumulate cycles;

    } // loop on p for subcarriers
	
    wiCMatFree(&y);
    wiCMatFree(&y1);
	wiCMatFree(&y2);
	wiCMatFree(&H1);
    wiCMatFree(&Q);
    wiCMatFree(&QConj);
    wiCMatFree(&R);

	if(MMSE)
	{
		wiCMatFree(&HAug);
		wiCMatFree(&Aug);
		wiCMatFree(&H1Aug);
		wiCMatFree(&QAug);	
	}
	
	if(pState->GetStatistics) 	*psym +=1;
//	fclose(F1); // x.yu
//	fclose(F2); // x.yu
    return WI_SUCCESS;
}
// end of Phy11n_SingleTreeSearchSphereDecoder()


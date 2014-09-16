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

wiStatus Phy11n_GrayDemapper(wiInt y[], wiInt *index, wiReal X, int N_BPSC); 
wiStatus Phy11n_GrayMapper(wiInt y[], wiInt index, int M); 
wiStatus Phy11n_MaxValueArray(wiReal *max_X, int *index, wiReal X[], wiInt size); 
wiStatus Phy11n_MinValueArray(wiReal *min_X, wiInt *index, wiReal X[], wiInt size); 


// ================================================================================================
// FUNCTION  : Phy11n_GrayDemapper()
// ------------------------------------------------------------------------------------------------
// Purpose   : Gray Demapper for PAM
// Parameters: y      -- output array of coded bits
//             X      -- Input array of QAM symbol per dimension
//             N_BPSC -- Number of coded bits per subcarrier
// ================================================================================================
wiStatus Phy11n_GrayDemapper(int y[], wiInt *index, wiReal X,  int N_BPSC)
{    
	wiInt y1[3];
    
    switch (N_BPSC)
    {           // ----
    case 1:  // BPSK        
    case 2:  // QPSK
        // ----
        y[0] = (X>0.0)? 1 : 0;  
		y1[0] = y[0];
		if(index) *index = y1[0]; 
        break;
        // ------
    case 4:  // 16-QAM
        // ------
		y[0] = (X>0.0)? 1 : 0;
		y[1] = (X>2.0 || X<-2.0) ?  0 : 1; 
		y1[0] = y[0]; 
		y1[1] = y[0]? (y[1]^0x1) : y[1];  
		if(index) *index = (y1[0]<<1) + y1[1];
        break;
        // ------
    case 6:  // 64-QAM
        y[0] = (X>0)? 1 : 0;
		y[1] = (X>4.0 || X<-4.0) ?  0 : 1;
		y[2] = (fabs(X)>6.0 || fabs(X)<2.0)? 0: 1;
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
// end of Phy11n_GrayDemapper()


/*===========================================================================+
|  Function  : Phy11n_GrayMapper()                                          |
|----------------------------------------------------------------------------|
|  Purpose   : Gray Mapper for PAM                                           |
|  Parameters: y      -- output array of coded bits                          |
|              index  -- index of PAM elements                               |
|              M      -- # of PAM constellation points                       |
+===========================================================================*/
wiStatus Phy11n_GrayMapper(int y[], wiInt index, int M)
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
// FUNCTION  : Phy11n_ListSphereDecoder()
//  Purpose   :  List Sphere Decoder receiver for Y = HX + N,                                                           
//              assume QAM constellations are normalized to have average engergy one per symbol										  |					
//				assume equal modulation (MCS no larger than 32)	
//              Do not support STBC                                            
//  Parameters: Y: received signal matrix after RX FFT, N_RX-by-len            
//              H: an arry of channel matrix; each element corresponds to a MIMO channel                                                |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     
//              prior_llr: input priori soft bit information matrix            
//              extr_llr:  output extrinsic soft bit information matrix        
//              N_BPSC:    an array of number of bits per symbol per TX        
//              noise_std: white noise standard deviation (complex) 
// ================================================================================================

wiStatus Phy11n_ListSphereDecoder(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                  Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std)
{
    wiInt i, j, p, index, k, n, g;
	wiInt step, nstep, npoint; // current and next step;
	wiInt Init_d2, Update_d2;
    wiInt tx,rx, tx2, rx2, len;
    wiReal a[4][6],b[4][6];
    wiReal c, llr;
	wiReal d2[8], r2, min_s, max_s;  
	wiReal A = 0.0; // Scale factor;
    wiReal UBs[8], s[8], y_hat[8]; // Upper bound on s, corrent s
	wiReal ED, max_ED, min_ED; // ED of current point; maximum ED in the Candidate list

	wiReal ave_point=0, ave_NCand=0;  
	
    wiCMat y,y1,y2, H1, Q, QConj, R, x;

    Phy11n_RxState_t *pRX = Phy11n_RxState();

    wiInt NCand = pRX->ListSphereDecoder.NCand;
    wiReal alpha = pRX->ListSphereDecoder.alpha;
    wiReal FixedRadius = pRX->ListSphereDecoder.FixedRadius;
	wiInt *b_lsd = pRX->ListSphereDecoder.b_lsd; 
	wiReal *d2_lsd = pRX->ListSphereDecoder.d2_lsd; 
	wiInt Nd = pRX->ListSphereDecoder.Nd;
	wiInt Nb = pRX->ListSphereDecoder.Nb;

    tx = H[0].col; // # of transmit antennas
    rx = H[0].row; // # of receive antennas

	if(N_BPSC[0]>1)
	{
		tx2 = 2*tx;
		rx2 = 2*rx;
	}
	else // BPSK, only care about real part
	{
		tx2 = tx;
		rx2 = 2*rx;
	}

    len = Y->col;  // # of subcarriers
    	
    XSTATUS(wiCMatInit(&y,  rx,  1));
    XSTATUS(wiCMatInit(&y1, rx2,  1)); // real equivalent
	XSTATUS(wiCMatInit(&y2, tx2, 1));  // Q^H *y1; equivalent received signal
	XSTATUS(wiCMatInit(&H1, rx2, tx2));
	XSTATUS(wiCMatInit(&Q,  rx2, tx2)); // orthoginal matrix in QR
	XSTATUS(wiCMatInit(&QConj,  tx2, rx2)); 
	XSTATUS(wiCMatInit(&R,  tx2, tx2)); // upper triangular matrix in QR
	XSTATUS(wiCMatInit(&x,  tx2,  1));

	if(!d2_lsd || NCand!= Nd)
	{
		if(d2_lsd) wiFree(d2_lsd);
		WICALLOC(d2_lsd, NCand, wiReal);
		Nd = NCand;
	}

	if(!b_lsd || NCand*N_BPSC[0]*tx != Nb)
	{
		if(b_lsd) wiFree(b_lsd);
		WICALLOC(b_lsd, NCand*tx*N_BPSC[0], wiInt);
		Nb = NCand*tx*N_BPSC[0];
	}
	

    if (noise_std<pow(10.0,-8))
        noise_std = pow(10.0,-8);
    
    
	switch (N_BPSC[0])
    {
        case  1:
			A = 1.0;
			min_s = -1.0;
			max_s =  1.0;			
			break;
        case  2:
			A = sqrt(  2.0);
			min_s = -1.0;
			max_s =  1.0;
			break;
        case  4:
			A = sqrt( 10.0);
			min_s = -3.0;
			max_s =  3.0;
			break;
        case  6: 
			A = sqrt( 42.0);
			min_s = -7.0;
			max_s =  7.0;
			break;
		default:
			return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

	r2 = (alpha ==0.0)? FixedRadius: alpha*tx2*pow(noise_std,2.0)/2; // radius constraint  factor /2 is to change to noise power per dimension A^2, change to nature dimention
	
    for (p=0;p<len;p++) // i: index of subcarriers
    {   	
		// initialize log values, corresponding to P = 0; ln(0) -> -Inf
		for (i = 0; i< tx; i++)       
			for (j=0;j<N_BPSC[i];j++)
			{
				a[i][j] = -pow(10.0,6); b[i][j] = -pow(10.0,6);  
			}
					
		memset(d2_lsd, 0, NCand*sizeof(wiReal));
		memset(b_lsd, 0, NCand*tx*N_BPSC[0]*sizeof(wiInt));		
			
		wiCMatGetSubMtx(Y, &y, 0, p, rx-1,p);
				
		Init_d2 = 1; //use initial sphere constraint
		Update_d2 = 0; // Reset Update_d2;

		// Change to Real Matrix. Expand the tree to tx2 levels
		for (i=0; i<rx; i++)
		{			
			y1.val[i][0].Real    = y.val[i][0].Real;
			y1.val[i+rx][0].Real = y.val[i][0].Imag;
			
			
			for (j=0; j<tx; j++)
			{
				H1.val[i][j].Real       = H[p].val[i][j].Real;
				H1.val[i+rx][j].Real    = H[p].val[i][j].Imag;
				if(N_BPSC[0]>1)
				{				
					H1.val[i][j+tx].Real    = -H[p].val[i][j].Imag;
					H1.val[i+rx][j+tx].Real = H[p].val[i][j].Real;				
				}				 
			}
		}		
		// QR decomposition on equivalent real channel matrix
		wiCMatQR(&H1, &Q, &R);
        wiCMatConjTrans(&QConj, &Q);

		// Equivalent received signal
		wiCMatMul(&y2, &QConj, &y1);		
		

		k = 0;  // root of the tree
		step = 1; // Initial step
		n = 0;  // number of Candidates in the list
		npoint = 0; // number of points reached;
		index = 0;
		max_ED = 1.0e6;
		while (k<tx2)
		{			
			switch (step)
			{
				
			case 1:
				k = tx2 -1;
				d2[k] = r2;
				y_hat[k] = y2.val[k][0].Real;
				nstep = 2;
				break;

			case 2:

				if(R.val[k][k].Real>0)
				{
					UBs[k] = floor((sqrt(d2[k]) + y_hat[k]) *A /R.val[k][k].Real); // change to natureal scale lattice
					s[k] = ceil((-sqrt(d2[k]) + y_hat[k]) *A /R.val[k][k].Real);
				}
				else
				{
					UBs[k] = floor((-sqrt(d2[k]) + y_hat[k])*A /R.val[k][k].Real); // change to natureal scale lattice
					s[k] = ceil((sqrt(d2[k]) + y_hat[k]) *A /R.val[k][k].Real);
				}

				if(s[k]<min_s) s[k] = min_s -2.0; // lattice step is 2
				else if (!(((int) s[k]) & 0x1)) s[k] = s[k] -1.0;  // if even do s[k]+1 -2;
				else s[k] = s[k] - 2.0; 
				nstep = 3;
				break;

			case 3:
				s[k] = s[k] + 2.0; 
				if(Update_d2)
				{			
					if(d2[k]>0)
						UBs[k] = floor(((R.val[k][k].Real>0 ? 1.0 : -1.0 )* sqrt(d2[k]) + y_hat[k]) *A /R.val[k][k].Real);
					else
						UBs[k] = min_s - 1.0;
				}
				if((s[k] > UBs[k]) || (s[k]> max_s))
					nstep = 4;
				else
					nstep = 5;	
				break;

			case 4:
				k = k + 1;  // up one level in the tree;
				if(k<tx2)
					nstep = 3;  // didn't check k=tx2 here. Don't know if it will work;
				else
					nstep = 0;	
				break; 

			case 5:				
				if(k == 0)
					nstep = 6;
				else
				{
					k = k - 1;
					// Get effective received signal for level k
					y_hat[k] = y2.val[k][0].Real; 					
					for (i = k+1; i <tx2; i++)
						y_hat[k] = y_hat[k] - R.val[k][i].Real * s[i]/A;

					d2[k] = d2[k+1] - pow((y_hat[k+1]-R.val[k+1][k+1].Real*s[k+1]/A),2.0); // PED for level k;
					nstep = 2;
				}
				break;
			
			case 6:
				ED = d2[tx2-1] - d2[0] + pow(y_hat[0] - R.val[0][0].Real*s[0]/A, 2.0);
				npoint += 1;

				if (n<NCand)
				{
					
					d2_lsd[n] = ED; // record the ED for each candidate
	
					for (i = 0, j= 0; i < tx; i++) // record the bits of the candidate;
					{
						XSTATUS(Phy11n_GrayDemapper(b_lsd+n*tx*N_BPSC[0]+j, &g, s[i], N_BPSC[0])); // Real component of antenna i

						if(N_BPSC[0]>1)
						{
							j += N_BPSC[0]/2;
							XSTATUS(Phy11n_GrayDemapper(b_lsd+n*tx*N_BPSC[0]+j,&g, s[i+tx], N_BPSC[0])); // Imag component of antenna i
							j += N_BPSC[0]/2; 
						}
						else // BPSC
							j += N_BPSC[0]; 

					}
    				n += 1;
					
				}
				else
				{					
					// Find the maximum ED value in current list. 
					if (Init_d2 == 1)
					{
						XSTATUS(Phy11n_MaxValueArray(&max_ED, &index, d2_lsd, NCand));
						Init_d2 = 0;
					}

					if(ED < max_ED)  // Update to the current point
					{
						d2_lsd[index] = ED;
	
						for (i = 0, j= 0; i < tx; i++) // record the bits of the candidate;
						{
							Phy11n_GrayDemapper(b_lsd+index*tx*N_BPSC[0]+j, &g, s[i], N_BPSC[0]); // Real component of antenna i; N_BPSC[0]/2 bits			
							if(N_BPSC[0]>1)
							{
							j += N_BPSC[0]/2;
							Phy11n_GrayDemapper(b_lsd+index*tx*N_BPSC[0]+j, &g, s[i+tx], N_BPSC[0]); // Imag component of antenna i	
							j += N_BPSC[0]/2; 
							}
							else
								j += N_BPSC[0];
						}
						// Update d2 to current max_ED;  // 
						XSTATUS(Phy11n_MaxValueArray(&max_ED, &index, d2_lsd, NCand));
						
						if(max_ED < d2[tx2-1])
						{
							for (i = 0; i< tx2; i++)
							{
								d2[i] = d2[i] + max_ED - d2[tx2-1]; 
							}

							Update_d2 = 1;
						}
					}					
				}						

				nstep = 3;
				break;
			default:
				printf("\n*** ERROR: Invalid List SD state\n\n");
				exit(1);				
			} // switch;
				
		step = nstep; // update step;

		}  // while k<tx2 loop;
		
		if(!n)
			printf(" \n *** ERROR: No point find \n\n");
		else
		{
			XSTATUS(Phy11n_MinValueArray(&min_ED, &index, d2_lsd, n));
		}
		
		ave_point += npoint;
		ave_NCand += n;
	
		// Generate LLR based on Candidate;
		for (index = 0; index <n; index ++)  // loop through the Candidate list
		{
			c = -1.0*d2_lsd[index]/pow(noise_std, 2.0);  //  -||y-Hs||^2 /noise_std^2;
			llr = 0;
			
			// prior probality for the candidate
			for(i = 0; i< tx; i++) // ith stream
				for(j = 0; j< N_BPSC[i]; j++) // jth bit in ith stream
					llr += prior_LLR[i][p*N_BPSC[i]+j]*b_lsd[index*tx*N_BPSC[0]+i*N_BPSC[0]+j];  
			
			// update log values, cancalling prior info to get extrinsic info
			for (i = 0; i< tx; i++)
			{
				for(j = 0; j <N_BPSC[i]; j++)
				{					
					if(b_lsd[index*tx*N_BPSC[0]+i*N_BPSC[0]+j] == 1)
						a[i][j] = Phy11n_LogExpAdd(a[i][j], c + llr - prior_LLR[i][p*N_BPSC[i]+j]*b_lsd[index*tx*N_BPSC[0]+i*N_BPSC[0]+j]);
					else
						b[i][j] = Phy11n_LogExpAdd(b[i][j], c + llr - prior_LLR[i][p*N_BPSC[i]+j]*b_lsd[index*tx*N_BPSC[0]+i*N_BPSC[0]+j]);				
				}
			}

		} // loop on index for Candidates
		
		for(i = 0; i< tx; i++) // ith stream
				for(j = 0; j< N_BPSC[i]; j++) // jth bit in ith stream
				{
					extr_LLR[i][p*N_BPSC[i]+j] = a[i][j] - b[i][j]; 
					if(extr_LLR[i][p*N_BPSC[i]+j] > 1000.0) 
						extr_LLR[i][p*N_BPSC[i]+j] = 1000.0;
						else if(extr_LLR[i][p*N_BPSC[i]+j] < -1000.0)  
							extr_LLR[i][p*N_BPSC[i]+j] = -1000.0;
				}	  
    } // loop on p for subcarriers
    
    wiCMatFree(&y);
    wiCMatFree(&y1);
	wiCMatFree(&y2);
	wiCMatFree(&H1);
    wiCMatFree(&x);
    wiCMatFree(&Q);
    wiCMatFree(&QConj);
    wiCMatFree(&R);
	

    return WI_SUCCESS;
}
// end of Phy11n_ListSphereDecoder()


/*====================================================================+
|  Find the first child of the current node
*/
wiStatus Phy11n_MIMO_STS_MCU(wiInt Child[], wiInt *ChildBits, wiReal S[], 
	wiReal d2[], wiReal PED2[], wiInt delta[], wiInt CPoint[], wiReal *y_hat, wiInt k, wiCMat y, 
	wiCMat R, wiInt N_BPSC, wiInt M,  wiReal A, wiInt Unbiased, wiReal noise_std, wiInt Norm1) 
{		
	wiReal s, d;
	wiInt sign, i;

	if(k>y.row)  return STATUS(WI_ERROR_PARAMETER3);
	
	y_hat[k] = y.val[k][0].Real; 					
	for (i = k+1; i <y.row; i++)
		y_hat[k] = y_hat[k] - R.val[k][i].Real * S[i]/A;

	s = y_hat[k]/R.val[k][k].Real*A;
	XSTATUS(Phy11n_GrayDemapper(ChildBits+k*max(1, N_BPSC/2), Child+k, s,  N_BPSC));
	S[k]= 2.0*Child[k] -(M-1); 
	d = y_hat[k] - R.val[k][k].Real*S[k]/A;	
	d2[k] = Norm1? fabs(d):pow(d, 2.0);

	if(Unbiased && (M>2))
	{
		if(Norm1)
			d2[k] += (M-1-fabs(S[k]))*noise_std/A;		
		else
			d2[k] += (pow((M-1), 2.0) - pow(S[k], 2.0))*pow(noise_std/A,2.0);
	}
	// Beginning point of zig-zag enumeration;
	sign = ((d>0)^(R.val[k][k].Real>0))? -1: 1;		
	delta[k] =sign;   // sign of d/R.val[k][k].Real;	
	
	PED2[k] = (k==y.row-1) ? d2[k]: (Norm1 ==2 ? max(PED2[k+1], d2[k]): PED2[k+1] + d2[k]);	
	CPoint[k] = 0;

	return WI_SUCCESS;
}

/*====================================================================+
|  Enumerate on the next sibling of current node
*/
wiStatus Phy11n_MIMO_STS_MEU( wiInt Sib[], wiReal d2_Sib[], wiReal PED2[], wiReal PED2Sib[], wiInt AllPoints[], 
	wiInt delta[], wiInt CPoint[], wiInt Up[], wiInt Low[],  wiInt k, wiInt Child[], wiReal y_hat[], wiCMat R,  wiInt M, wiReal A, wiInt Unbiased, wiReal noise_std, wiInt Norm1)
{
	wiInt sign;	
	wiReal S, d;
			
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
	S = 2.0*Sib[k] - (M-1);
	d = y_hat[k] - R.val[k][k].Real*S/A;
	d2_Sib[k] = Norm1? fabs(d): pow(d, 2.0);
	
	if(Unbiased && (M>2))
	{
		if(Norm1)
			d2_Sib[k] += (M-1-fabs(S))*noise_std/A;	
		else
			d2_Sib[k] += (pow((M-1), 2.0) - pow(S, 2.0))*pow(noise_std/A ,2.0);
	}

	PED2Sib[k] = (k==R.row-1)? d2_Sib[k] : (Norm1 == 2? max(PED2[k+1], d2_Sib[k]): PED2[k+1] + d2_Sib[k]);
	CPoint[k] += 1;
		
	return WI_SUCCESS;
}

/*====================================================================================================+
|  Check if the current node meet the criterion
*/
wiStatus Phy11n_MIMO_STS_Prune(wiInt *NoPrune, wiInt k, wiInt Init, wiInt Child[],  wiInt *ChildBits, 
 wiReal PED2[], wiInt *ML_bits, wiReal LambdaML, wiReal *LambdaCounter,
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
		// Can be implemented by parallel comparator to reduce time 
		if (PED2[k]<LambdaML)
		{
			*NoPrune = 1;
			return WI_SUCCESS;
		}

		for (i=k+1; i<tx2; i++) 
			for (j=0; j<Nb; j++)
			{   				
				if ((PED2[k]<LambdaCounter[i*Nb+j]) && (ChildBits[i*Nb+j]^ML_bits[i*Nb+j])) 
				{
					*NoPrune = 1; 									
					return WI_SUCCESS;
				}
			}

		for (i= 0; i<=k; i++)
			for (j = 0; j<Nb; j++)
				if (PED2[k] < LambdaCounter[i*Nb+j])
				{
					*NoPrune = 1;					
					return WI_SUCCESS;
				}
	}	

	return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_SingleTreeSearchSphereDecoder_Hybrid() 
// Purpose   : Wrapper for Phy11n_SingleTreeSearchSphereDecoder to allow truncation of the search
//             using MMSE.
// ================================================================================================
wiStatus Phy11n_SingleTreeSearchSphereDecoder_Hybrid(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                                     Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    XSTATUS( Phy11n_SingleTreeSearchSphereDecoder(Y, H, extr_LLR, prior_LLR, N_BPSC, noise_std) );

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
// end of Phy11n_SingleTreeSearchSphereDecoder_Hybrid()


// ================================================================================================
// FUNCTION  : Phy11n_SingleTreeSearchSphereDecoder() 
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
wiStatus Phy11n_SingleTreeSearchSphereDecoder(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t extr_LLR, 
                                              Phy11nSymbolLLR_t prior_LLR, wiInt N_BPSC[], wiReal noise_std)
{
    wiInt i,j,p,k, k1, k2, i1, j1;
	// current and next step; 
	wiInt step, nstep;
	wiReal nNodes, nLeaves, nCycles; 	
    wiInt tx,rx, tx2, rx2, len;
    wiReal d2[8], PED2[8], PED2Sib[8], y_hat[8], d2_Sib[8]; // current and partial ED;  Effective received signal
	wiReal A = 0; // Scale factor;   	
	wiInt M, Nb; //size of PAM; number of bits for PAM
	wiReal S[8], S_ML[8]; // signal points of PAM;
	wiInt delta[8]; // current delta for enumeration; 
	wiInt Child[8], Sib[8], CPoint[8], AllPoints[8], Permu[8]; // Child and Sib index; CPoint: # of constellation points reached
	wiInt Up[8], Low[8]; // indicating reached upper or lower bound of enumberation
	wiReal LambdaML, LMax, LambdaCounterBd; // Metric for hypothesis and counter hypothesis, Clipping level;
	wiInt ML_bits[24], ChildBits[24], CounterUpdates[24];
    wiReal MetricML, MetricCounter, LambdaCounter[24], SCounter[24][8];
	wiReal AvgCUpdates = 0.0;
	wiInt Init, NoPrune; // Init: Not find a leaf yet. NoPrune:{0: prune a subtree,  1: keep a subtree}
	wiCMat y,y1,y2, H1, Q, QConj, R;
	wiCMat HAug, H1Aug, QAug, Aug; // MMSE-QR
	wiReal maxNorm, minNorm, NormR; // Norm of channel estimation
	wiInt print1 = 0, print2 = 0, print3 = 0, print4 = 0, print_QR= 0; // STS, LLR, Leaf, Metric, QR
	FILE *F1, *F2, *F3, *F4, *F_QR; 	
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

	
	//wiPrintf(" WICALLOC \n");
	//////////////////////////////
	//
	//  Restrict noise
	//
	//
    if (noise_std<pow(10.0,-8.0))
        noise_std = pow(10.0,-8.0);

    LMax = pState->Norm1? pState->LMax1*noise_std : pState->LMax*pow(noise_std,2.0);
	
	/////////////////////////////
	//
	//  Constellation 
	//
	//
	switch (N_BPSC[0])
    {
        case  1:
			A = 1.0;
			M = 2;			
			break;
        case  2:
			A = sqrt(  2.0);
			M = 2;
			break;
        case  4:
			A = sqrt( 10.0);
			M = 4;
			break;
        case  6: 
			A = sqrt( 42.0);
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
		LambdaML = pow(10.0,6.0);
		memset(ML_bits, 0, N_BPSC[0]*tx*sizeof(wiInt));
		memset(ChildBits, 0, N_BPSC[0]*tx*sizeof(wiInt));
		memset(d2, 0, tx2*sizeof(wiReal));
		memset(PED2, 0, tx2*sizeof(wiReal));
		memset(y_hat, 0, tx2*sizeof(wiReal));
		memset(d2_Sib, 0, tx2*sizeof(wiReal));
		memset(delta, 0, tx2*sizeof(wiInt));
		memset(Child, 0, tx2*sizeof(wiInt));
		memset(Sib, 0, tx2*sizeof(wiInt));
		memset(AllPoints, 0, 8*sizeof(wiInt));
		memset(CPoint, 0, tx2*sizeof(wiInt));
		memset(S, 0, tx2*sizeof(wiReal));
		memset(Up, 0, tx2*sizeof(wiInt));
		memset(Low, 0, tx2*sizeof(wiInt));
		memset(Permu, 0, tx2*sizeof(wiInt));
		memset(CounterUpdates, 0, N_BPSC[0]*tx*sizeof(wiInt));
		memset(S_ML, 0, tx2*sizeof(wiReal));
		memset(SCounter, 0, tx2*sizeof(wiReal));
				
		for (i=0; i<N_BPSC[0]*tx; i++)
			LambdaCounter[i] = pow(10.0, 6.0);
			
		if(Terminated)
		{
			if(BlockSchedule)			
				MaxCycles = TotalCycles - AccuCycles - (len-p-1)*MinCycles;			
			else
				MaxCycles = pState->MaxCycles; // uniform upper bound for each subcarrier 				
		}

	   		
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
		
		///////////////////////////////////////////////////////////////////////////////
		//
		//  Single tree search with soft output
		//  Depth first; Vertical expansion, horizontal expansion and pruning occurs 
		// 	in parallel
		//
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
			if(print1)
			{
				F1 = fopen("STS.txt","at");
				fprintf(F1, " Current level k =%d \n", k);
				fclose(F1);
			}

			switch (step)
			{
			//////////////////////////////////////////////////////////////////////////
			//
			// root level MCU find the best child
			//
			//
			case 1:  
				k = tx2 -1;		
				
				XSTATUS(Phy11n_MIMO_STS_MCU(Child, ChildBits, S, d2, PED2, delta, CPoint, y_hat, k, y2, R, N_BPSC[0], M, A, Unbiased, noise_std, pState->Norm1));
				nCycles += 1.0;
				nstep = 2;
				break;

			//////////////////////////////////////////////////////////////////////////
			//
			// Middle level processing. MCU, MEU and Pruning occurs in parallel
			//
			//
			case 2: 
				
				// MCU: find the best child in level k-1;
				XSTATUS(Phy11n_MIMO_STS_MCU(Child, ChildBits, S, d2, PED2, delta, CPoint, y_hat, k-1, y2, R, N_BPSC[0], M, A, Unbiased, noise_std, pState->Norm1));

				// MEU: enumerate the next sibling in level k;
				XSTATUS(Phy11n_MIMO_STS_MEU(Sib, d2_Sib, PED2, PED2Sib, AllPoints, delta, CPoint, Up, Low, k, Child, y_hat, R, M, A, Unbiased, noise_std, pState->Norm1));
				
				// Prune: Check the prune condition for child[k]
				XSTATUS(Phy11n_MIMO_STS_Prune(&NoPrune, k, Init, Child, ChildBits,  PED2, ML_bits, LambdaML, LambdaCounter, tx2, Nb));
			
				if (NoPrune)
				{
					k -= 1;
					if (k)					
						nstep = 2;	
					else // leaf
						nstep = 3;
				}
				else
				{	
					AllPoints[k] = 1; 
					while (AllPoints[k])
						k += 1;
					if(k<tx2)
					{
						Child[k] = Sib[k];
						d2[k] = d2_Sib[k]; 
						PED2[k] = PED2Sib[k];
						S[k] = 2.0*Child[k] -(M-1);
						XSTATUS(Phy11n_GrayMapper(ChildBits+k*Nb, Child[k], M));
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
				XSTATUS(Phy11n_MIMO_STS_MEU(Sib,d2_Sib, PED2, PED2Sib, AllPoints, delta, CPoint, Up, Low, k,  Child, y_hat, R, M, A, Unbiased, noise_std, pState->Norm1));
				// Prune
				XSTATUS(Phy11n_MIMO_STS_Prune(&NoPrune, k, Init, Child, ChildBits, PED2, ML_bits, LambdaML, LambdaCounter, tx2, Nb));
				
				
				if(print3)
				{
					F3 = fopen("Leaf.txt", "at");
					fprintf(F3, " New Leaf Keep? %d \n", NoPrune? 1: 0); 
					for (i=0; i<tx2; i++)
						fprintf(F3, "Child[%d]= %d  Sib[%d] = %d d2[%d] = %.10f d2_Sib[%d] =%.10f S[%d] = %.1f AllPoints[%d] = %d\n", i, Child[i], i, Sib[i], i, d2[i], i, d2_Sib[i],  i, S[i], i, AllPoints[i]);
					fprintf(F3, " k=%d NoPrune = %d Init =%d d2[%d] =%f PED2[%d]=%.10f  LambdaML = %.10f \n", k, NoPrune, Init, k, d2[k], k,PED2[k], LambdaML);					
					fclose(F3);
				}

				if (NoPrune)
				{				

					/////////////////////////////////////////////
					//
					// If new hypothesis found, update all valid
					// counter-hypothesis followed by ML updates
					//
					if (PED2[0]< LambdaML)
					{
						for(i=0; i<Nb*tx2; i++)
						{
							if(ChildBits[i]^ML_bits[i])
							{
								LambdaCounter[i] =  LambdaML; 

								if(!Init)
								{
									CounterUpdates[i] = 1;
									for (j=0; j<tx2; j++)
										SCounter[i][j] = S_ML[j];
								}
								ML_bits[i] = ChildBits[i];
							}
							
						}
						LambdaML = PED2[0];

						for (j=0; j<tx2; j++) // record the ML solution
							S_ML[j] = S[j];

						

						/////////////////////////////////////////////
						// LLR Cliping to LMax
						if(pState->LLRClip)
						{	LambdaCounterBd = LambdaML + LMax;
							for (i=0; i<Nb*tx2; i++)
								LambdaCounter[i] = (LambdaCounter[i]> LambdaCounterBd)? LambdaCounterBd : LambdaCounter[i];
						}

						if(print3)
						{
							F3 = fopen("Leaf.txt", "at");
							fprintf(F3," Update ML LambdaML = %.10f PED2[0]= %.10f \n", LambdaML, PED2[0]);
							for (i =0; i<Nb*tx2; i++)
								fprintf(F3, " ML_bits[%d] = %d ChildBits[%d] = %d, LambdaCounter[%d]= %.10f \n", i, ML_bits[i], i, ChildBits[i], i, LambdaCounter[i]);
							fclose(F3);
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
							if((ChildBits[i]^ML_bits[i]) && (PED2[0]<LambdaCounter[i]))
							{
								CounterUpdates[i] = 1;
								LambdaCounter[i] = PED2[0]; 
								for (j=0; j<tx2; j++)
									SCounter[i][j] = S[j];
							}
						}
						if(print3)
						{
							F3 = fopen("Leaf.txt", "at");
							fprintf(F3," Update Counter hypothesis LambdaML = %.10f PED2[0]= %.10f \n", LambdaML, PED2[0]);
							for (i =0; i<Nb*tx2; i++)
								fprintf(F3, " ML_bits[%d] = %d ChildBits[%d] = %d, LambdaCounter[%d]= %.10f \n", i, ML_bits[i], i, ChildBits[i], i, LambdaCounter[i]);
							fclose(F3);
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
					d2[k] = d2_Sib[k];
					PED2[k] = PED2Sib[k];
					S[k] = 2.0*Child[k] -(M-1);
					XSTATUS(Phy11n_GrayMapper(ChildBits+k*Nb, Child[k], M));
				}
				nNodes += 1.0;
				nCycles += 1.0;
				nLeaves += 1.0;
	
				if(print3)
				{
					F3 = fopen("Leaf.txt", "at");
					fprintf(F3, "End of Leaf :nCycles =%f nNodes = %f nLeaves = %f step = %d nstep = %d k =%d NoPrune =%d \n", nCycles, nNodes, nLeaves, step, nstep, k, NoPrune);
					fclose(F3);
				}

				break;

			default:
				printf("\n*** ERROR: Invalid STS SD state\n\n");
				exit(1);				
			} // switch;
		
			
			if(print1 ==1)
			{
				F1 = fopen("STS.txt", "at");
				fprintf(F1, " nCycles = %f nNodes = %f  nLeaves %f  step =%d   nstep = %d k= %d NoPrune = %d \n", nCycles, nNodes, nLeaves, step, nstep, k, NoPrune);
				for (i=0; i<tx2; i++)
					fprintf(F1, " Child[%d]= %d  Sib[%d] = %d  PED2[%d] = %.10f S[%d] = %.1f AllPoints[%d] = %d\n", i, Child[i], i, Sib[i], i, i, PED2[i], S[i], i, AllPoints[i]);
				
				fprintf(F1, "\n");
				fclose(F1);
			}

			step = nstep; // update step;

			if(Terminated && nCycles >=MaxCycles) 
			{
				if(pState->Hybrid) pTrunc[p] = 1;
				
				break;
			}

		}  // while k<tx2 loop;		

		if(print4) 
		{
			F4 = fopen("Metric.txt", "at");
			fprintf(F4, " Metric for p = %d  LambdaML = %f \n", p, LambdaML);
			
			for(i=0; i<Nb*tx2; i++)
				fprintf(F4, " ML_bits[%d] = %d LambdaCounter[%d] = %f \n", i, ML_bits[i], i, LambdaCounter[i]);
			fprintf(F4, "\n");
			fclose(F4);
		}					
					
		//////////////////////////////////////////////////////////////////
		//
		// Generate soft output according to MAX-Log-MAP
		// Prior information is not used because it is soft-output only
		// 
		//
		if(print2) 	F2 = fopen("LLR.txt", "at");
		if(pState->Norm1 && pState->ExactLLR)	// Recalculate Metric according to L2-Norm	
		{
			MetricML = 0.0;
			for (i=0; i<tx2; i++)
			{
				y_hat[i] = y2.val[i][0].Real;
				for (j=i+1; j<tx2; j++)
					y_hat[i] -= R.val[i][j].Real*S_ML[j]/A;
				MetricML += pow(y_hat[i] - R.val[i][i].Real*S_ML[i]/A, 2.0);
			}
			MetricML *=-1.0/pow(noise_std, 2.0);
		}
		else if(pState->Norm1)
			MetricML = -1.0*pow(LambdaML,2.0)/pow(noise_std, 2.0);
		else
			MetricML = -1.0*LambdaML/pow(noise_std, 2.0); 

		for(i=0; i<tx2; i++)
		{
			k1 = Permu[i];
			for (j = 0; j<Nb; j++)
			{
				if(CounterUpdates[i*Nb+j]) AvgCUpdates +=1.0;
				if(pState->Norm1 && pState->ExactLLR && CounterUpdates[i*Nb+j])
				{
					MetricCounter = 0.0;
					for (i1=0; i1<tx2; i1++)
					{
						y_hat[i1] = y2.val[i1][0].Real;
						for(j1 = i1+1; j1 < tx2; j1++)
							y_hat[i1] -= R.val[i1][j1].Real*SCounter[i*Nb+j][j1]/A;
						MetricCounter += pow(y_hat[i1] - R.val[i1][i1].Real *SCounter[i*Nb+j][i1]/A, 2.0);
					}
					MetricCounter *= -1.0/pow(noise_std, 2.0);
				}
				else if (pState->Norm1)
				MetricCounter = -1.0*pow(LambdaCounter[i*Nb+j],2.0)/pow(noise_std, 2.0); 
				else
				MetricCounter = -1.0*LambdaCounter[i*Nb+j]/pow(noise_std, 2.0); 
								
				
				if(N_BPSC[0]>1)
				{
					k2 = k1>>1;								
					extr_LLR[k2][p*N_BPSC[k2] + (k1&0x1)*Nb + j] = (2*ML_bits[i*Nb+j]-1)*(MetricML - MetricCounter);
					if(print2) fprintf(F2, " i = %d  k1= %d k2 =%d  j =%d Nb = %d ML_bit[%d] = %d MetricCounter =%f MetricML = %f extr_LLR[%d][%d] = %f \n", i, k1, k2, j, Nb, i*Nb+j, ML_bits[i*Nb+j], MetricCounter, MetricML, k1>>1, p*N_BPSC[k1>>1] + (k1&0x1)*Nb + j, extr_LLR[k1>>1][p*N_BPSC[k1>>1] + (k1&0x1)*Nb + j]);

				}
				else // BPSK
				{				
					extr_LLR[k1][p*N_BPSC[k1]+j] = (2*ML_bits[i*Nb+j]-1)*(MetricML - MetricCounter);
					if(print2) fprintf(F2, " i = %d k1 =%d j =%d Nb = %d ML_bit[%d] = %d MetricCounter =%f MetricML = %f extr_LLR[%d][%d] = %f \n", i, k1, j, Nb, i*Nb+j, ML_bits[i*Nb+j], MetricCounter, MetricML, k1, p*N_BPSC[k1] + j, extr_LLR[k1][p*N_BPSC[k1] + j]);
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
//	wiPrintf(" Average Counter Updates = %f \n", AvgCUpdates/(len*tx2*Nb));
	if(pState->GetStatistics) 	*psym +=1;
	return WI_SUCCESS;
}
// end of Phy11n_SingleTreeSearchSphereDecoder()

/*===============================================================================+
|  Function  : Phy11n_MaxValueArray()                                                   |
|--------------------------------------------------------------------------------|
|  Purpose   : Find the maximum value and index of a none-negative double array  |
|				                                                                 |
|  Parameters: y      -- output array of coded bits                              |
|              X      -- Input array of QAM symbol per dimension                 |
|              N_BPSC -- Number of coded bits per subcarrier.                    |
+===============================================================================*/
wiStatus Phy11n_MaxValueArray(wiReal *max_X, wiInt *index, wiReal X[], wiInt size)
{
	wiInt i, j = 0;
	wiReal temp = 0.0;

	for( i = 0; i < size; i++)
	{
		if(X[i]>temp)
		{
			temp = X[i];
			j = i;
		}
	}
	*max_X = temp;
	*index = j;

	return WI_SUCCESS;
}


/*===============================================================================+
|  Function  : Phy11n_MinValueArray()                                            |
|--------------------------------------------------------------------------------|
|  Purpose   : Find the minimum value and index of a none-negative double array  |
|				                                                                 |
|  Parameters: y      -- output array of coded bits                              |
|              X      -- Input array of QAM symbol per dimension                 |
|              N_BPSC -- Number of coded bits per subcarrier.                    |
+===============================================================================*/
wiStatus Phy11n_MinValueArray(wiReal *min_X, wiInt *index, wiReal X[], wiInt size)
{
	wiInt i, j = 0;
	wiReal temp = 1000.0;

	for( i = 0; i < size; i++)
	{
		if(X[i]<temp)
		{
			temp = X[i];
			j = i;
		}
	}
	*min_X = temp;
	*index = j;

	return WI_SUCCESS;
}

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

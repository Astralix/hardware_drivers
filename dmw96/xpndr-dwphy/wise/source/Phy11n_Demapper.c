//--< Phy11n_Demapper.c >--------------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n_Demapper - Phy11n Receiver Demaper Models
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiUtil.h"
#include "Phy11n.h"

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif

wiStatus Phy11n_EnumerateQAM(wiComplex s[], int N_BPSC);
wiStatus Phy11n_QAM_SoftDetector(wiComplex d, wiReal *p,   wiReal NoisePower, wiInt N_BPSC);
wiStatus Phy11n_QAM_HardDetector(wiComplex d, wiInt *bits, wiReal NoisePower, wiInt N_BPSC);

__inline static int Phy11n_Bin2Int(int bits[], int NumBits) // binary array to integer conversion
{
    int n = 0;
    for (NumBits--; NumBits >= 0; NumBits--)
        n += (bits[NumBits] << NumBits);
    return n;
}

__inline static void Phy11n_Int2Bin(int i, int *bits, int len) // integer to binary {0,1} array
{
    int k;
    for (k=0; k<len; k++)
        bits[k] = (i>>k) & 1;
}

__inline static void Phy11n_Int2BinaryAntipodal(int i, int *bits, int len) // integer to binary {-1,+1} array
{
    int k;
    for (k=0; k<len; k++)
        bits[k] = (i>>k) & 1 ? 1 : -1;
}

// ================================================================================================
// FUNCTION  : Phy11n_Demapper_TreeSearchMMSE()
// ------------------------------------------------------------------------------------------------
//  Purpose  : Modified version of Moon & Li's Tree Search
// ================================================================================================
wiStatus Phy11n_Demapper_TreeSearchMMSE(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, 
                                        Phy11nSymbolLLR_t PriorLLR, wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiReal NoisePower[PHY11N_TXMAX];

    wiInt i, j, k, m;
    wiCMat H_aug, Q_aug, A, A2, a1, B, b1, C; 
    wiCMat y, y1, y2, SoftEst, SoftEstStd, tmp3, tmp4;
    wiComplex yEq, tmp;
    wiComplex s[PHY11N_MAX_SS][64];
    wiReal tanhPriorLLR[6];
    wiInt  bits[6], ConstellationSize[4];
    double a[6],b[6];
    double dist, prob[64];

    // allocate memory
    XSTATUS(wiCMatInit(&H_aug,  N_RX+N_SS,  N_SS));
    XSTATUS(wiCMatInit(&Q_aug,  N_RX+N_SS,  N_SS));
    XSTATUS(wiCMatInit(&A,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&B,  N_SS,  N_RX));
    XSTATUS(wiCMatInit(&C,  N_RX,  N_SS));
    XSTATUS(wiCMatInit(&y,  N_SS, 1));
    XSTATUS(wiCMatInit(&y1, N_RX, 1));
    XSTATUS(wiCMatInit(&SoftEst, N_SS, 1));
    XSTATUS(wiCMatInit(&SoftEstStd, N_SS, N_SS));
    XSTATUS(wiCMatInit(&b1, 1, N_RX));
    XSTATUS(wiCMatInit(&y2, 2, 1));
    XSTATUS(wiCMatInit(&A2, 2, N_SS));
    XSTATUS(wiCMatInit(&a1, 1, N_SS));
    XSTATUS(wiCMatInit(&tmp3, 2, 1));
    XSTATUS(wiCMatInit(&tmp4, 2, 1));
        
    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
    }

    for (k = 0; k < NumberOfSubcarriers; k++) // decode tranmitted symbol vector one by one
    {
        // ------------------------------------------------------------------
        // compute soft estimate and standard deviation from prior information
        // ------------------------------------------------------------------    
        wiCMatSetZero(&SoftEst);
        wiCMatSetZero(&SoftEstStd);
        for (i = 0; i < N_SS; i++) // i: index of transmit antennas
        {   
            for (m=0; m<N_BPSC[i]; m++)
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);

            for (j = 0; j < ConstellationSize[i]; j++)
            {
                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);
                prob[j] = 1.0;
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m]*tanhPriorLLR[m])/2.0;                 
                
                SoftEst.val[i][0].Real += prob[j] * s[i][j].Real;
                SoftEst.val[i][0].Imag += prob[j] * s[i][j].Imag;
            }
            
            // compute variance of SoftEstimate error
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                wiComplex e = wiComplexSub(SoftEst.val[i][0], s[i][j]);
                SoftEstStd.val[i][i].Real += prob[j] * wiComplexNormSquared(e);
            }
            SoftEstStd.val[i][i].Real = sqrt(SoftEstStd.val[i][i].Real);
        }
        
        // ------------------------------------------------------------------
        // compute MMSE QR decomposition
        // ------------------------------------------------------------------
        // build augemented channel matrix
        wiCMatSetSubMtx(&H_aug, H+k, 0, 0, N_RX-1, N_SS-1);
        wiCMatSetIden(&A);
        wiCMatScaleSelfReal(&A, NoiseStd);
        wiCMatSetSubMtx(&H_aug, &A, N_RX, 0, N_RX+N_SS-1, N_SS-1);       
        
        // QR decomposition of augemented channel matrix
        wiCMatQR (&H_aug, &Q_aug, &A); 
        wiCMatMul(&H_aug, &Q_aug, &A);
        
        // obtain y = A*x + B*n
        wiCMatGetSubMtx(&Q_aug, &C, 0, 0, N_RX-1, N_SS-1);
        wiCMatConjTrans(&B, &C); 
        
        wiCMatMul(&A, &B, H+k);
    
        wiCMatGetSubMtx(Y, &y1, 0, k, N_RX-1, k); 
        wiCMatMul(&y, &B, &y1); 
        
        // compute transformed noise power
        for (m=0; m<N_SS; m++)
        {
            wiCMatGetSubMtx(&B, &b1, m, 0, m, N_RX-1);
            NoisePower[m] = SQR(NoiseStd) * wiCMatFrobeNormSquared(&b1);
        }
        
        // ------------------------------------------------------------------
        // decode symbols from N_SS - 1 to 1
        // ------------------------------------------------------------------
        for (i = N_SS - 1; i >= 1; i--) // i: index of transmit antennas
        {         
            int ConstellationSize = 1 << N_BPSC[i];
            wiComplex current = {0, 0};
            double SumProb = 0;

            for (m = 0; m < N_BPSC[i]; m++) {
                a[m] = b[m] = -1E+6;
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);
            }
            for (j = 0; j < ConstellationSize; j++) // enumerate all possibilities for symbol transmitted from TX i
            {
                SoftEst.val[i][0] = s[i][j];
                tmp = y.val[i-1][0];
                for (m = i; m < N_SS; m++)
                {
                    tmp = wiComplexSubtract(tmp, wiComplexMultiply(A.val[i-1][m], SoftEst.val[m][0]));
                }
                yEq = wiComplexDiv(tmp, A.val[i-1][i-1]);
                Phy11n_QAM_HardDetector(yEq, bits, 0.5, N_BPSC[i-1]); // here noise power is set to 0.5, which does not matter for hard decoding
                SoftEst.val[i-1][0] = s[i][Phy11n_Bin2Int(bits, N_BPSC[i-1])];
                
                // obtain noise+interference power for the current symbol
                // maybe simplified
                SoftEstStd.val[i-1][i-1].Real = 0;
                SoftEstStd.val[i][i].Real = 0;   
                wiCMatGetSubMtx(&A, &A2, i-1, 0, i, N_SS-1);            
                wiCMatMulSelfRight(&A2, &SoftEstStd);   
                wiCMatGetSubMtx(&A2, &a1, 0, 0, 0, N_SS-1);
                tmp3.val[0][0].Real = sqrt(NoisePower[i-1] + wiCMatFrobeNormSquared(&a1));
                wiCMatGetSubMtx(&A2, &a1, 1, 0, 1, N_SS-1);
                tmp3.val[1][0].Real = sqrt(NoisePower[i]   + wiCMatFrobeNormSquared(&a1));
                
                tmp3.val[0][0].Imag = tmp3.val[1][0].Imag = 0;

                // Compute distance
                wiCMatGetSubMtx(&y, &y2, i-1, 0, i, 0);
                wiCMatGetSubMtx(&A, &A2, i-1, 0, i, N_SS-1);
                wiCMatMul(&tmp4, &A2, &SoftEst);            
                wiCMatSub(&tmp4, &tmp4, &y2);

                y2.val[0][0] = wiComplexDiv(tmp4.val[0][0], tmp3.val[0][0]);
                y2.val[1][0] = wiComplexDiv(tmp4.val[1][0], tmp3.val[1][0]);
                
                // compute the probability term in log domain
                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);   
                prob[j] = 0.0;
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] += PriorLLR[i][k*N_BPSC[i]+m] * bits[m];
                dist = -wiCMatFrobeNormSquared(&y2) + prob[j]/2.0;            
                
                // compute the probability of each symbol and the mean value
                prob[j] = exp(-wiCMatFrobeNormSquared(&y2));
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m]*tanhPriorLLR[m])/2.0;
                SumProb += prob[j];
                current.Real += prob[j] * SoftEst.val[i][0].Real;
                current.Imag += prob[j] * SoftEst.val[i][0].Imag;            
                
                // update log bit values
                for (m=0; m<N_BPSC[i]; m++)
                {
                    if (bits[m] == 1) a[m] = Phy11n_LogExpAdd(dist, a[m]);
                    else              b[m] = Phy11n_LogExpAdd(dist, b[m]);
                }
            } // end j
            
            // compute extrinsic soft information
            for (m=0; m<N_BPSC[i]; m++)
                ExtrinsicLLR[i][k*N_BPSC[i]+m] = a[m] - b[m] - PriorLLR[i][k*N_BPSC[i]+m];
            
            // obtain soft esimate of symbol transmitted from the jth TX
            SoftEst.val[i][0] = wiComplexScaled( current, 1/SumProb );

            // get variance of the error of soft estimate
            SoftEstStd.val[i][i].Real = 0;
            for (j = 0; j < ConstellationSize; j++)
            {
                wiComplex e = wiComplexSub(s[i][j], SoftEst.val[i][0]);
                SoftEstStd.val[i][i].Real += prob[j] * wiComplexNormSquared(e);
            }
            SoftEstStd.val[i][i].Real = sqrt(SoftEstStd.val[i][i].Real / SumProb);
        } 
        
        // ------------------------------------------------------------------
        // decode symbol 0
        // ------------------------------------------------------------------
        for (j = 0; j < N_BPSC[0]; j++)
            a[j] = b[j] = -1E+6; // initialize log values

        for (j = 0; j < ConstellationSize[0]; j++) // enumerate all possibilities for symbol transmitted from TX 0
        {
            wiReal Pn = NoisePower[0];

            Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[0]);
            SoftEst.val[0][0] = s[i][j];
            
            yEq = y.val[0][0];
            SoftEstStd.val[0][0] = wiComplexZero;
            for (m = 0; m < N_SS; m++)
            {
                yEq = wiComplexSub( yEq,   wiComplexMultiply(A.val[0][m], SoftEst.val[m][0]) );
                Pn += wiComplexNormSquared(wiComplexMultiply(A.val[0][m], SoftEstStd.val[m][m]));
            }         
            
            // compute the probability of each symbol
            prob[j] = 0.0;
            for (m = 0; m < N_BPSC[0]; m++)
                prob[j] += PriorLLR[0][k*N_BPSC[0]+m] * bits[m];

            dist = -sqrt(wiComplexNormSquared(yEq)/Pn) + prob[j]/2.0; // to be changed
            
            // update log bit values
            for (m = 0; m < N_BPSC[0]; m++)
            {
                if (bits[m] == 1) a[m] = Phy11n_LogExpAdd(dist, a[m]);
                else              b[m] = Phy11n_LogExpAdd(dist, b[m]);
            }
        }
        
        // compute extrinsic soft information
        for (m = 0; m < N_BPSC[0]; m++)
            ExtrinsicLLR[0][k*N_BPSC[0]+m] = a[m] - b[m] - PriorLLR[0][k*N_BPSC[0]+m];

    } // end k
    
    // free memory
    wiCMatFree(&H_aug);
    wiCMatFree(&Q_aug);
    wiCMatFree(&A);
    wiCMatFree(&A2);
    wiCMatFree(&a1);
    wiCMatFree(&B);
    wiCMatFree(&C);
    wiCMatFree(&y);
    wiCMatFree(&y1);
    wiCMatFree(&y2);
    wiCMatFree(&b1);
    wiCMatFree(&SoftEst);
    wiCMatFree(&SoftEstStd);
    wiCMatFree(&tmp3);
    wiCMatFree(&tmp4);
        
    return WI_SUCCESS;
}
// end of Phy11n_Demapper_TreeSearchMMSE()


// ================================================================================================
// FUNCTION  : Phy11n_MMSE_MultipleInversion()
// ------------------------------------------------------------------------------------------------
//  Purpose  : MMSE MIMO demapper (original implementation)
// ================================================================================================
wiStatus Phy11n_MMSE_MultipleInversion(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, 
                                       Phy11nSymbolLLR_t PriorLLR, wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiReal NoisePower = SQR(NoiseStd);

    wiInt i,j,k,m;
    wiCMat A, A1, B, SigmaX, SigmaX1, a, b;
    wiCMat x_mmse, y, y1, SoftEst, SoftEst1;
    wiComplex xEq;
    wiReal NoiseEq;
    wiReal tanhPriorLLR[6];
    wiReal prob[64]; // up to 64QAM
    wiInt  bits[6], ConstellationSize[4];
    wiComplex s[PHY11N_MAX_SS][64];

    // allocate memory
    XSTATUS(wiCMatInit(&A,  N_RX,  N_RX));
    XSTATUS(wiCMatInit(&A1, N_RX,  N_RX));
    XSTATUS(wiCMatInit(&B,  N_SS,  N_RX));
    XSTATUS(wiCMatInit(&SigmaX,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&SigmaX1,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&a,  1, N_SS));
    XSTATUS(wiCMatInit(&b,  1, N_RX));
    XSTATUS(wiCMatInit(&x_mmse, 1, 1));
    XSTATUS(wiCMatInit(&y, N_RX, 1)); 
    XSTATUS(wiCMatInit(&y1, N_RX, 1)); 
    XSTATUS(wiCMatInit(&SoftEst, N_SS, 1)); 
    XSTATUS(wiCMatInit(&SoftEst1, N_SS, 1)); 
    
    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
    }
            
    for (k = 0; k < NumberOfSubcarriers; k++) // // decode tranmitted symbol vector one by one
    {
        wiCMatSetZero(&SigmaX);
        for (i = 0; i < N_SS; i++) // i: index of transmit antennas
        {
            // compute soft estimate from prior information
            for (m = 0; m < N_BPSC[i]; m++)
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);

            SoftEst.val[i][0] = wiComplexZero;
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);
                prob[j] = 1.0;
                
                for (m = 0; m < N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m] * tanhPriorLLR[m]) / 2.0;

                SoftEst.val[i][0].Real += (prob[j] * s[i][j].Real);
                SoftEst.val[i][0].Imag += (prob[j] * s[i][j].Imag);
            }
            
            // compute variance of SoftEstimate error
            for (j = 0; j < ConstellationSize[i]; j++)
                SigmaX.val[i][i].Real += prob[j] * wiComplexNormSquared( wiComplexSub(SoftEst.val[i][0], s[i][j]) );
        }

        for (i = 0; i < N_SS; i++) // i: index of transmit antennas
        {
            // to compute the extrinsic information of symbol i, its prior information
            // should not be used. Hence we assume symbol i is a complex normal variable with mean 0 and variance 1
            wiCMatCopy(&SigmaX, &SigmaX1);
            SigmaX1.val[i][i].Real = 1;
            wiCMatCopy(&SoftEst, &SoftEst1);
            SoftEst1.val[i][0] = wiComplexZero;
            
            // compute MMSE vector for symbol i
            wiCMatConjTrans(&B, H+k);      
            wiCMatMulSelfLeft(&B, &SigmaX1);
            wiCMatMul(&A, H+k, &B);
            wiCMatSetIden(&A1);
            wiCMatScaleSelfReal(&A1, NoisePower);
            wiCMatAddSelf(&A, &A1);
            XSTATUS( wiCMatInv(&A1, &A) );
            wiCMatGetSubMtx(H+k, &y, 0, i, N_RX-1, i);
            wiCMatConjTrans(&b, &y);
            wiCMatMulSelfRight(&b, &A1);
            wiCMatMul(&a, &b, H+k);
            
            // cancel the mean of other symbols from received signal vector
            wiCMatGetSubMtx(Y, &y, 0, k, N_RX-1, k);
            wiCMatMul(&y1, H+k, &SoftEst1);         
            wiCMatSub(&y, &y, &y1);
           
            wiCMatMul(&x_mmse, &b, &y);    // compute MMSE estimation
            
            // get equivalent data 
            xEq = wiComplexDiv(x_mmse.val[0][0], a.val[0][i]);
            
            // compute equivalent noise+interference power
            for (j = 0; j < N_SS; j++)
                a.val[0][j] = wiComplexScaled(a.val[0][j], sqrt(SigmaX1.val[j][j].Real));

            NoiseEq  = wiCMatFrobeNormSquared(&a) - wiComplexNormSquared(a.val[0][i]);
            NoiseEq += wiCMatFrobeNormSquared(&b) * NoisePower;
            NoiseEq /= wiComplexNormSquared(a.val[0][i]);
            
            // generate soft bit information
            Phy11n_QAM_SoftDetector(xEq, ExtrinsicLLR[i]+k*N_BPSC[i], NoiseEq, N_BPSC[i]);
        }         
    }
    
    // free memory
    wiCMatFree(&A);
    wiCMatFree(&A1);
    wiCMatFree(&B);
    wiCMatFree(&SigmaX);
    wiCMatFree(&SigmaX1);
    wiCMatFree(&a);
    wiCMatFree(&b);
    wiCMatFree(&x_mmse);
    wiCMatFree(&y);
    wiCMatFree(&y1);
    wiCMatFree(&SoftEst);
    wiCMatFree(&SoftEst1);
    
    return WI_SUCCESS;
}
// end of Phy11n_MMSE_MultipleInversion()

// ================================================================================================
// FUNCTION  : Phy11n_MMSE_SingleInversion()
// ------------------------------------------------------------------------------------------------
//  Purpose  : MMSE MIMO demapper (based on suggestion from Eli F)
// ================================================================================================
wiStatus Phy11n_MMSE_SingleInversion(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, 
                                     Phy11nSymbolLLR_t PriorLLR, wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiReal NoisePower = SQR(NoiseStd);

    wiInt i,j,k,m;
    wiCMat A, A1, B, C, C1, a, b, SigmaX, SoftEst, SoftEst1;
    wiCMat x_mmse,  y, y1;
    wiComplex xEq;
    wiReal NoiseEq; 
    wiReal tanhPriorLLR[6];
    wiReal t, prob[64]; // up to 64QAM
    wiComplex s[PHY11N_MAX_SS][64];
    wiInt  bits[6], ConstellationSize[PHY11N_MAX_SS];

    XSTATUS(wiCMatInit(&A,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&A1,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&B,  N_SS,  N_RX));
    XSTATUS(wiCMatInit(&C,  N_RX,  N_RX));
    XSTATUS(wiCMatInit(&C1, N_RX,  N_RX));
    XSTATUS(wiCMatInit(&SigmaX,  N_SS,  N_SS));
    XSTATUS(wiCMatInit(&SoftEst, N_SS, 1)); 
    XSTATUS(wiCMatInit(&SoftEst1, N_SS, 1)); 
    XSTATUS(wiCMatInit(&a,  1, N_SS));
    XSTATUS(wiCMatInit(&b,  1, N_RX));
    XSTATUS(wiCMatInit(&y, N_RX, 1)); 
    XSTATUS(wiCMatInit(&y1, N_RX, 1));
    XSTATUS(wiCMatInit(&x_mmse, 1, 1))

    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
    }
            
    for (k = 0; k < NumberOfSubcarriers; k++)
    {
        wiCMatSetZero(&SigmaX);
        for (i = 0; i < N_SS; i++) // i: index of transmit antennas
        {
            for (m=0; m<N_BPSC[i]; m++)
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);

            // compute soft estimate from prior information
            SoftEst.val[i][0] = wiComplexZero;
            for (j=0; j<ConstellationSize[i]; j++)
            {
                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);
                prob[j] = 1.0;
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m]*tanhPriorLLR[m])/2.0;

                SoftEst.val[i][0] = wiComplexAdd( SoftEst.val[i][0], wiComplexScaled(s[i][j], prob[j]) );
            }
            
            // compute variance of SoftEstimate error
            for (j=0; j<ConstellationSize[i]; j++)
            {
                wiComplex e = wiComplexSub(SoftEst.val[i][0], s[i][j]);
                SigmaX.val[i][i].Real += prob[j] * wiComplexNormSquared( e );
            }
        }
        wiCMatConjTrans(&B, H+k);      
        wiCMatMulSelfLeft(&B, &SigmaX);
        wiCMatMul(&C1, H+k, &B);

        for (j = 0; j < N_RX; j++)
            C1.val[j][j].Real += NoisePower;

        XSTATUS( wiCMatInv(&C, &C1) );

        for (i = 0; i < N_SS; i++) // i: index of transmit antennas
        {    
            wiCMatCopy(&SoftEst, &SoftEst1);
            SoftEst1.val[i][0] = wiComplexZero;

            // compute MMSE vector for symbol i
            wiCMatGetSubMtx(H+k, &y, 0, i, N_RX-1, i);
            wiCMatConjTrans(&b, &y);
            wiCMatMul(&C1, &y, &b);
            wiCMatMulSelfRight(&C1, &C);
            t = wiComplexReal( wiCMatTrace(&C1) ) + 1.0/(1.0 - SigmaX.val[i][i].Real);
            wiCMatScaleSelfReal(&C1, -1/t);         
            wiCMatMulSelfLeft(&C1, &C);
            wiCMatAddSelf(&C1, &C);      
            wiCMatMulSelfRight(&b, &C1);
            wiCMatMul(&a, &b, H+k);
            
            // cancel the mean of other symbols from received signal vector
            wiCMatGetSubMtx(Y, &y, 0, k, N_RX-1, k);
            wiCMatMul(&y1, H+k, &SoftEst1);         
            wiCMatSub(&y, &y, &y1);
            
            // compute MMSE estimation
            wiCMatMul(&x_mmse, &b, &y);   
            
            // get equivalent data 
            xEq = wiComplexDiv(x_mmse.val[0][0], a.val[0][i]);
            
            // compute equivalent noise+interference power
            for (j=0; j<N_SS; j++)
            {
                if (j == i) continue;
                a.val[0][j].Real *= sqrt(SigmaX.val[j][j].Real);
                a.val[0][j].Imag *= sqrt(SigmaX.val[j][j].Real);
            }
            NoiseEq  = wiCMatFrobeNormSquared(&a) - wiComplexNormSquared(a.val[0][i]);
            NoiseEq += wiCMatFrobeNormSquared(&b) * NoisePower;
            NoiseEq /=  wiComplexNormSquared(a.val[0][i]);
            
            // generate soft bit information
            Phy11n_QAM_SoftDetector(xEq, ExtrinsicLLR[i]+k*N_BPSC[i], NoiseEq, N_BPSC[i]);
        }         
    }
    wiCMatFree(&A);
    wiCMatFree(&A1);
    wiCMatFree(&B);
    wiCMatFree(&C);
    wiCMatFree(&C1);
    wiCMatFree(&SigmaX);
    wiCMatFree(&SoftEst);
    wiCMatFree(&SoftEst1);
    wiCMatFree(&a);
    wiCMatFree(&b);
    wiCMatFree(&y);
    wiCMatFree(&y1);
    wiCMatFree(&x_mmse);

    return WI_SUCCESS;
}
// end of Phy11n_MMSE_SingleInversion()

// ================================================================================================
// FUNCTION  : Phy11n_MMSE_STBC()
// ------------------------------------------------------------------------------------------------
//  Purpose  : MMSE MIMO demapper for space-time block codes
// ================================================================================================
wiStatus Phy11n_MIMO_MMSE_STBC(wiCMat Y52[], wiCMat H52[], Phy11nSymbolLLR_t ExtrinsicLLR[],
                               Phy11nSymbolLLR_t PriorLLR[], wiInt N_BPSC_i[], wiReal NoiseStd, wiInt STBC)
{
    int N_SS = H52[0].col - STBC; // number of spatial streams
    int N_RX = H52[0].row;        // number of receive antennas
    int NumberOfSubcarriers = Y52->col; 

    wiReal NoisePower = SQR(NoiseStd);

    wiInt i,j,k,n;
    wiCMat A, A1, B, C, C1, a, b, SigmaX, SoftEst, SoftEst1;
    wiCMat x_mmse, y, y1, Y, H[52];
    wiComplex xEq;
    wiReal t, NoiseEq, tanhPriorLLR[6];
    wiReal prob[64]; // up to 64QAM
    wiInt  bits[6], ConstellationSize[6];
    wiInt N_BPSC[6] = {0}, N_CBPSS[6] = {0};
    wiComplex s[6][64];

    for (k=0; k<NumberOfSubcarriers; k++)
        wiCMatInit(H+k, 2*N_RX, 2*N_SS);

    wiCMatInit(&Y, 2*N_RX, NumberOfSubcarriers);
    for (i = 0; i < 2*N_SS; i++)
    {
        N_BPSC[i]  =    N_BPSC_i[i/2];
        N_CBPSS[i] = 52*N_BPSC[i];
        if (N_CBPSS[i] > PHY11N_MAX_CBPSS) return STATUS(WI_ERROR);
    }
    // Derive the equivalent channel matrix H, chanel output Y, and prior information PriorLLR
    for (i = 0; i < N_RX; i++)
    {
        switch (STBC) // Equivalent channel matrix, H
        {
            case 1:

                for (k=0; k<52; k++)
                {
                    {
                        H[k].val[2*i  ][0] =               H52[k].val[i][0];
                        H[k].val[2*i  ][1] = wiComplexNeg (H52[k].val[i][1]);
                        H[k].val[2*i+1][0] = wiComplexConj(H52[k].val[i][1]);
                        H[k].val[2*i+1][1] = wiComplexConj(H52[k].val[i][0]);
                    }
                    if (N_SS > 1)
                    {
                        H[k].val[2*i  ][2] =               H52[k].val[i][2];
                        H[k].val[2*i+1][2] = wiComplexZero;
                        H[k].val[2*i  ][3] = wiComplexZero;
                        H[k].val[2*i+1][3] = wiComplexConj(H52[k].val[i][2]);
                    }
                    if (N_SS > 2)
                    {
                        H[k].val[2*i  ][4] =               H52[k].val[i][3];
                        H[k].val[2*i+1][4] = wiComplexZero;
                        H[k].val[2*i]  [5] = wiComplexZero;
                        H[k].val[2*i+1][5] = wiComplexConj(H52[k].val[i][3]);
                    }
                }
                break;

            case 2:  // N_SS=2, N_STS=4

                for (k=0; k<52; k++)
                {
                    H[k].val[2*i  ][0] =               H52[k].val[i][0];
                    H[k].val[2*i  ][1] = wiComplexNeg (H52[k].val[i][1]);
                    H[k].val[2*i+1][0] = wiComplexConj(H52[k].val[i][1]);
                    H[k].val[2*i+1][1] = wiComplexConj(H52[k].val[i][0]);

                    H[k].val[2*i  ][2] =               H52[k].val[i][2];
                    H[k].val[2*i  ][3] = wiComplexNeg (H52[k].val[i][3]);
                    H[k].val[2*i+1][2] = wiComplexConj(H52[k].val[i][3]);
                    H[k].val[2*i+1][3] = wiComplexConj(H52[k].val[i][2]);
                }
                break;

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        // Equivalent channel output, Y
        for (k = 0; k < 52; k++)
        {
            Y.val[2*i  ][k] =               Y52[0].val[i][k];
            Y.val[2*i+1][k] = wiComplexConj(Y52[1].val[i][k]);
        }
    }
    XSTATUS(wiCMatInit(&A,  2*N_SS, 2*N_SS));
    XSTATUS(wiCMatInit(&A1, 2*N_SS, 2*N_SS));
    XSTATUS(wiCMatInit(&B,  2*N_SS, 2*N_RX));
    XSTATUS(wiCMatInit(&C,  2*N_RX, 2*N_RX));
    XSTATUS(wiCMatInit(&C1, 2*N_RX, 2*N_RX));
    XSTATUS(wiCMatInit(&a,     1,   2*N_SS));
    XSTATUS(wiCMatInit(&b,     1,   2*N_RX));
    XSTATUS(wiCMatInit(&y,  2*N_RX,   1)); 
    XSTATUS(wiCMatInit(&y1, 2*N_RX,   1));
    XSTATUS(wiCMatInit(&x_mmse, 1, 1))
    XSTATUS(wiCMatInit(&SigmaX,   2*N_SS, 2*N_SS));
    XSTATUS(wiCMatInit(&SoftEst,  2*N_SS,   1)); 
    XSTATUS(wiCMatInit(&SoftEst1, 2*N_SS,   1)); 

    // Precompute the constellation signal points
    //
    for (i = 0; i < 2*N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
        if (i % 2) {
            for (j = 0; j < ConstellationSize[i]; j++)
                s[i][j].Imag = -s[i][j].Imag;
        }
    }
            
    for (k=0; k<NumberOfSubcarriers; k++) // Process bits - Subcarrier Loop ///////////////////////
    {
        wiCMatSetZero(&SigmaX);
        for (i = 0; i < 2*N_SS; i++)
        {
            for (j=0; j<N_BPSC[i]; j++)
                tanhPriorLLR[j] = tanh(PriorLLR[i%2][i/2][k*N_BPSC[i]+j]/2.0);

            // compute soft estimate from prior information
            SoftEst.val[i][0] = wiComplexZero;
            for (n=0; n<ConstellationSize[i]; n++)
            {
                Phy11n_Int2BinaryAntipodal(n, bits, N_BPSC[i]);
                prob[n] = 1.0;

                for (j=0; j<N_BPSC[i]; j++)
                    prob[n] *= (1.0 + bits[j]*tanhPriorLLR[j]) / 2.0;
            
                //////////////////////////////////////////////////////////////////////////////
                //
                //  Debug - Hossein
                //
                //  Barrett: The code below originally printed a diagnostic message every time
                //  it reached here. This seems to happen during normal operation running with
                //  symbol-by-symbol iterative processing. This may or may not be a bug, but
                //  to avoid an endless list of disagnostic messages, only the first occurance
                //  is output.
                if (prob[n] == 0)
                { 
                    static wiBoolean FirstTime = WI_TRUE;
                    if (FirstTime) {
                        wiPrintf("ERROR in MIMO_MMSE_1_OneInv_STBC(): prob[%d]=0\n",n);
                        FirstTime = WI_FALSE;
                    }
                }
                SoftEst.val[i][0].Real += prob[n] * s[i][n].Real;
                SoftEst.val[i][0].Imag += prob[n] * s[i][n].Imag;
            }
            
            // compute variance of SoftEstimate error
            for (n = 0; n < ConstellationSize[i]; n++)
            {
                wiComplex e = wiComplexSub(SoftEst.val[i][0], s[i][n]);
                SigmaX.val[i][i].Real += prob[n] * wiComplexNormSquared(e);
            }
        }
        wiCMatConjTrans(&B, H+k);      
        wiCMatMulSelfLeft(&B, &SigmaX);
        wiCMatMul(&C1, H+k, &B);
        wiCMatSetIden(&C);
        wiCMatScaleSelfReal(&C, NoisePower);
        wiCMatAddSelf(&C1, &C);
        XSTATUS( wiCMatInv(&C, &C1) );

        for (i = 0; i < 2*N_SS; i++) // i: index of transmit antennas
        {    
            wiCMatCopy(&SoftEst, &SoftEst1);
            SoftEst1.val[i][0] = wiComplexZero;

            // compute MMSE vector for symbol i
            wiCMatGetSubMtx(H+k, &y, 0, i, 2*N_RX-1, i);
            wiCMatConjTrans(&b, &y);
            wiCMatMul(&C1, &y, &b);
            wiCMatMulSelfRight(&C1, &C);
            t = wiComplexReal( wiCMatTrace(&C1) ) + 1.0/(1.0 - SigmaX.val[i][i].Real);
            wiCMatScaleSelfReal(&C1, -1/t);         
            wiCMatMulSelfLeft(&C1, &C);
            wiCMatAddSelf(&C1, &C);      
            wiCMatMulSelfRight(&b, &C1);
            wiCMatMul(&a, &b, H+k);
            
            // cancel the mean of other symbols from received signal vector
            wiCMatGetSubMtx(&Y, &y, 0, k, 2*N_RX-1, k);
            wiCMatMul(&y1, H+k, &SoftEst1);         
            wiCMatSub(&y, &y, &y1);
            
            // compute MMSE estimation
            wiCMatMul(&x_mmse, &b, &y);   
            
            // get equivalent data 
            xEq = wiComplexDiv(x_mmse.val[0][0], a.val[0][i]);
            xEq.Imag = (i % 2) ? -xEq.Imag : xEq.Imag;
            
            // compute equivalent noise+interference power
            for (j = 0; j < 2*N_SS; j++)
            {
                if (j == i) continue;
                a.val[0][j].Real *= sqrt(SigmaX.val[j][j].Real);
                a.val[0][j].Imag *= sqrt(SigmaX.val[j][j].Real);
            }
            NoiseEq  = wiCMatFrobeNormSquared(&a) - wiComplexNormSquared(a.val[0][i]);
            NoiseEq += wiCMatFrobeNormSquared(&b) * NoisePower;
            NoiseEq /= wiComplexNormSquared(a.val[0][i]);
            
            Phy11n_QAM_SoftDetector(xEq, ExtrinsicLLR[i%2][i/2]+k*N_BPSC[i], NoiseEq, N_BPSC[i]);
        }         
    }

    for (k=0; k<52; k++)
        wiCMatFree(H+k);

    wiCMatFree(&Y);
    wiCMatFree(&A);
    wiCMatFree(&A1);
    wiCMatFree(&B);
    wiCMatFree(&C);
    wiCMatFree(&C1);
    wiCMatFree(&SigmaX);
    wiCMatFree(&SoftEst);
    wiCMatFree(&SoftEst1);
    wiCMatFree(&a);
    wiCMatFree(&b);
    wiCMatFree(&x_mmse);
    wiCMatFree(&y);
    wiCMatFree(&y1);
    return WI_SUCCESS;
}
// end of Phy11n_MIMO_MMSE_STBC()

// ================================================================================================
// FUNCTION  : Phy11n_MIMO_QR()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_MIMO_QR(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, Phy11nSymbolLLR_t PriorLLR, 
                        wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiInt i,j,k,m;
    wiCMat Q, Qconj, R, x, y;
    wiComplex xEq, SoftEst[PHY11N_TXMAX] = {{0}};
    wiReal NoiseEq;
    wiReal NoisePower = SQR(NoiseStd);
    wiReal tanhPriorLLR[6], prob[64], SumProb;
    wiInt  bits[6], ConstellationSize[4];
    wiComplex s[PHY11N_MAX_SS][64];
    wiReal SigmaX[PHY11N_TXMAX] = {0};
    
    XSTATUS(wiCMatInit(&Q,  N_RX,  N_SS));
    XSTATUS(wiCMatInit(&Qconj,  N_SS,  N_RX));
    XSTATUS(wiCMatInit(&R,  N_SS,  N_SS));   
    XSTATUS(wiCMatInit(&x, N_SS, 1));
    XSTATUS(wiCMatInit(&y, N_RX, 1)); 
    
    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
    }
    for (k = 0; k < NumberOfSubcarriers; k++) // k: index of subcarriers
    {   
        wiCMatQR(H+k, &Q, &R);
        wiCMatConjTrans(&Qconj, &Q);
        
        wiCMatGetSubMtx(Y, &y, 0, k, N_RX-1, k); 
        wiCMatMul(&x, &Qconj, &y);   
        
        for (i=N_SS-1; i>=0; i--) // i: index of transmit antennas
        {
            NoiseEq = NoisePower;
            for (j=i+1; j<N_SS; j++)
            {
                x.val[i][0] = wiComplexSub(x.val[i][0], wiComplexMultiply(R.val[i][j], SoftEst[j]));
                NoiseEq += wiComplexNormSquared(R.val[i][j]) * SigmaX[j];
            }
            
            // get equivalent data 
            xEq = wiComplexDiv(x.val[i][0], R.val[i][i]);
            NoiseEq = NoiseEq / wiComplexNormSquared(R.val[i][i]);

            // generate soft bit information
            Phy11n_QAM_SoftDetector(xEq, ExtrinsicLLR[i]+k*N_BPSC[i], NoiseEq, N_BPSC[i]);

            for (m=0; m<N_BPSC[i]; m++)
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);

            // compute SoftEstimate and variance
            SumProb = 0;
            SoftEst[i] = wiComplexZero;
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                wiComplex e = wiComplexSub(xEq, s[i][j]);
                prob[j] = exp(-wiComplexNormSquared(e) / NoiseEq);

                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m]*tanhPriorLLR[m])/2.0; 

                SumProb += prob[j];

                SoftEst[i] = wiComplexAdd( SoftEst[i], wiComplexScaled(s[i][j], prob[j]) );
            }
            SoftEst[i] = wiComplexScaled(SoftEst[i], 1.0/SumProb);
            
            // compute variance of SoftEstimate error
            SigmaX[i] = 0;
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                wiComplex e = wiComplexSub(SoftEst[i], s[i][j]);
                SigmaX[i] += (prob[j] / SumProb) * wiComplexNormSquared(e);
            }
        }
    }
    wiCMatFree(&Q);
    wiCMatFree(&Qconj);
    wiCMatFree(&R);
    wiCMatFree(&x);
    wiCMatFree(&y);

    return WI_SUCCESS;
}
// end of Phy11n_MIMO_QR()

// ================================================================================================
// FUNCTION  : Phy11n_MIMO_MRC()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_MIMO_MRC(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, Phy11nSymbolLLR_t PriorLLR, 
                   wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiInt k,i,j,m;
    wiCMat SoftEst, SoftEst1;
    wiCMat x, x1, y, y1, h, z;
    wiComplex xEq;
    wiReal NoiseEq;
    wiReal SigmaX[PHY11N_TXMAX] = {0};
    wiReal tanhPriorLLR[6];
    wiReal prob[64]; // up to 64QAM
    wiInt  bits[6], ConstellationSize[4];
    wiComplex s[PHY11N_MAX_SS][64];
    
    wiReal NoisePower = SQR(NoiseStd);

    XSTATUS(wiCMatInit(&SoftEst, N_SS, 1)); 
    XSTATUS(wiCMatInit(&SoftEst1, N_SS, 1)); 
    XSTATUS(wiCMatInit(&x, N_RX, 1));
    XSTATUS(wiCMatInit(&x1, 1, N_RX));
    XSTATUS(wiCMatInit(&y, N_RX, 1)); 
    XSTATUS(wiCMatInit(&y1, N_RX, 1));
    XSTATUS(wiCMatInit(&h, 1, N_SS));
    XSTATUS(wiCMatInit(&z, 1, 1));
    
    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
    {
        ConstellationSize[i] = 1 << N_BPSC[i];
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );
    }
    for (k = 0; k < NumberOfSubcarriers; k++)
    {
        for (i = 0; i < N_SS; i++)
        {
            SigmaX[i] = 0;

            for (m = 0; m < N_BPSC[i]; m++)
                tanhPriorLLR[m] = tanh(PriorLLR[i][k*N_BPSC[i]+m]/2.0);

            // compute soft estimate from prior information
            SoftEst.val[i][0] = wiComplexZero;
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                Phy11n_Int2BinaryAntipodal(j, bits, N_BPSC[i]);
                prob[j] = 1.0;
                for (m=0; m<N_BPSC[i]; m++)
                    prob[j] *= (1.0 + bits[m]*tanhPriorLLR[m])/2.0;

                SoftEst.val[i][0].Real += prob[j] * s[i][j].Real;
                SoftEst.val[i][0].Imag += prob[j] * s[i][j].Imag;
            }
            // compute variance of SoftEstimate error
            for (j = 0; j < ConstellationSize[i]; j++)
            {
                wiComplex e = wiComplexSub(SoftEst.val[i][0], s[i][j]);
                SigmaX[i] += prob[j] * wiComplexNormSquared(e);
            }
        }

        for (i = 0; i < N_SS; i++)
        {
            wiCMatCopy(&SoftEst, &SoftEst1);
            SoftEst1.val[i][0] = wiComplexZero;
            wiCMatGetSubMtx(Y, &y, 0, k, N_RX-1, k); 
            wiCMatMul(&y1, H+k, &SoftEst1);
            wiCMatSub(&y, &y, &y1);

            for (j=0; j<N_RX; j++)
            {
                wiCMatGetSubMtx(H+k, &h, j, 0, j, N_SS-1);

                h.val[0][i] = wiComplexZero;
                for (m=0; m<N_SS; m++)
                    h.val[0][m] = wiComplexScaled( h.val[0][m], sqrt(SigmaX[m]) );

                NoiseEq = sqrt(NoisePower + wiCMatFrobeNormSquared(&h));   
                
                y.val[j][0] = wiComplexScaled(   y.val[j][0], NoiseStd/NoiseEq);
                x.val[j][0] = wiComplexScaled(H[k].val[j][i], NoiseStd/NoiseEq);
            }
            wiCMatConjTrans(&x1, &x);
            wiCMatMul(&z, &x1, &y);
            xEq.Real = z.val[0][0].Real / wiCMatFrobeNormSquared(&x);
            xEq.Imag = z.val[0][0].Imag / wiCMatFrobeNormSquared(&x);
            Phy11n_QAM_SoftDetector(xEq, ExtrinsicLLR[i]+k*N_BPSC[i], NoisePower/wiCMatFrobeNormSquared(&x), N_BPSC[i]);
        }      
    }
    wiCMatFree(&SoftEst);
    wiCMatFree(&SoftEst1);
    wiCMatFree(&x);
    wiCMatFree(&x1);
    wiCMatFree(&y);
    wiCMatFree(&y1);
    wiCMatFree(&h);
    wiCMatFree(&z);

    return WI_SUCCESS;
}
// end of Phy11n_MIMO_MRC()


// ================================================================================================
// FUNCTION  : Phy11n_Demapper_LogMap
// ------------------------------------------------------------------------------------------------
// Purpose   : MAP receiver for Y = HX + N
// ================================================================================================
wiStatus Phy11n_Demapper_LogMap(wiCMat *Y, wiCMat H[], Phy11nSymbolLLR_t ExtrinsicLLR, 
                                Phy11nSymbolLLR_t PriorLLR, wiInt N_BPSC[], wiReal NoiseStd)
{
    int N_SS = H[0].col; // number of spatial streams
    int N_RX = H[0].row; // number of receive antennas
    int NumberOfSubcarriers = Y->col; 
    
    wiReal NoisePower = SQR(max(NoiseStd, 1E-8));

    int i, j, k, p;
    int Shift[PHY11N_TXMAX], Mask[PHY11N_TXMAX];
    int bits[PHY11N_TXMAX][6] = {{0}};
    double a[PHY11N_TXMAX][6];
    double b[PHY11N_TXMAX][6];
    double c, LLR;
    wiCMat x, y, z;
    wiComplex s[PHY11N_MAX_SS][64];
    
    int TotalPoints = 1;

    for (i = 0; i < N_SS; i++) 
    {
        Shift[i] = i? Shift[i-1] + N_BPSC[i-1] : 0;
        Mask[i] = (1 << N_BPSC[i]) - 1;
        TotalPoints *= (1 << N_BPSC[i]);
    }

    XSTATUS( wiCMatInit(&x, N_SS, 1) );
    XSTATUS( wiCMatInit(&y, N_RX, 1) );
    XSTATUS( wiCMatInit(&z, N_RX, 1) );

    // Precompute the constellation signal points
    //
    for (i = 0; i < N_SS; i++)
        XSTATUS( Phy11n_EnumerateQAM(s[i],  N_BPSC[i]) );

    for (k = 0; k < NumberOfSubcarriers; k++) // Loop over subcarriers ////////////////////////////
    {    
        wiCMatGetSubMtx(Y, &y, 0, k, N_RX-1, k); 
        
        for (i = 0; i < N_SS; i++)
            for (j = 0; j < N_BPSC[i]; j++)
                a[i][j] = b[i][j] = -1E+6; // initialize log values

        for (p = 0; p < TotalPoints; p++) // Loop over all constellation points //////////////
        {
            for (i = 0; i < N_SS; i++)
            {
                int n = (p >> Shift[i]) & Mask[i];          // constellation index in the ith spatial stream
                Phy11n_Int2Bin(n, bits[i], N_BPSC[i]);      // convert to a binary vector in bits
                x.val[i][0] = s[i][n];
            }
            wiCMatMul(&z, H+k, &x);
            wiCMatSub(&z, &z, &y);
            c = -wiCMatFrobeNormSquared(&z) / NoisePower; 
                    
            LLR = 0;
            for (i = 0; i < N_SS; i++)
                for (j = 0; j < N_BPSC[i]; j++)
                    LLR += PriorLLR[i][k*N_BPSC[i]+j] * bits[i][j];
                    
            // update log values, note the prior info. is cancelled
            for (i = 0; i < N_SS; i++)
            {
                for (j = 0; j < N_BPSC[i]; j++)
                {
                    if (bits[i][j] == 1)
                        a[i][j] = Phy11n_LogExpAdd(a[i][j], c + LLR - PriorLLR[i][k*N_BPSC[i]+j]*bits[i][j]);
                    else
                        b[i][j] = Phy11n_LogExpAdd(b[i][j], c + LLR - PriorLLR[i][k*N_BPSC[i]+j]*bits[i][j]);
                }
            }
        }

        // Compute extrinsic information
        //
        for (i = 0; i < N_SS; i++)  {
            for (j = 0; j < N_BPSC[i]; j++)
                ExtrinsicLLR[i][k*N_BPSC[i]+j] = a[i][j] - b[i][j];
        }    
    }
    wiCMatFree(&x);
    wiCMatFree(&y);
    wiCMatFree(&z);
    
    return WI_SUCCESS;
}
// end of Phy11n_Demapper_LogMap

// ================================================================================================
// FUNCTION  : Phy11n_QAM_SoftDetector
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute soft-output (approximate log-likelihood estimates) for the coded data bits
//             represented by equalized signal d
// ================================================================================================
wiStatus Phy11n_QAM_SoftDetector(wiComplex d, wiReal *p, wiReal NoisePower, wiInt N_BPSC)
{
    wiReal I, Q, K, a, c;
    
    // Scale d to restore original constellation values
    switch (N_BPSC)
    {
        case  1: a = 1.0;               break;
        case  2: a = 1.0 / sqrt(  2.0); break;
        case  4: a = 1.0 / sqrt( 10.0); break;
        case  6: a = 1.0 / sqrt( 42.0); break;
        default: return WI_ERROR_PARAMETER4;
    }
    
    c = 2.0 * a * a;
    I = d.Real / a;
    Q = d.Imag / a;
    K = c / max(1e-6, NoisePower);

    // Compute soft decisions (approximate LLR)
    switch (N_BPSC)
    {
        case  1:  // BPSK
     
            p[0] = K * I;
            break;
     
        case  2:  // QPSK
     
            p[0] = K * I;
            p[1] = K * Q;
            break;

        case  4:  // 16-QAM
        
            p[0] = K * I;
            p[1] = K * (2 - fabs(I));
            p[2] = K * Q;
            p[3] = K * (2 - fabs(Q));
            break;

        case  6:  // 64-QAM
        
            p[0] = K *                    I;
            p[1] = K *          (4 - fabs(I));
            p[2] = K * (2 - fabs(4 - fabs(I)));
            p[3] = K *                    Q;
            p[4] = K *          (4 - fabs(Q));
            p[5] = K * (2 - fabs(4 - fabs(Q)));
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_QAM_SoftDetector()

// ================================================================================================
// FUNCTION  : Phy11n_QAM_HardDetector
// ------------------------------------------------------------------------------------------------
// Purpose   : Perform B/QPSK or QAM demapping of the signal in d and return the corresponding
//             binary data in array b.
// ================================================================================================
wiStatus Phy11n_QAM_HardDetector(wiComplex d, wiInt b[], wiReal NoisePower, wiInt N_BPSC)
{
    int i; wiReal p[8];

    XSTATUS( Phy11n_QAM_SoftDetector(d, p, NoisePower, N_BPSC) );

    for (i=0; i<N_BPSC; i++)
        b[i] = (p[i] > 0) ? 1:0;

    return WI_SUCCESS;
}
// end of Phy11n_QAM_HardDetector()

// ================================================================================================
// FUNCTION  : Phy11n_EnumerateQAM
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate a set of 2^N_BPSC constellation points in array s
// ================================================================================================
wiStatus Phy11n_EnumerateQAM(wiComplex s[], int N_BPSC)
{
    int n;
    
    switch (N_BPSC)
    {
        case 1:  // BPSK

            s[0].Real = -1;  s[0].Imag = 0;
            s[1].Real =  1;  s[1].Imag = 0;
            break;

        case 2:  // QPSK
        {       
            const wiReal a = 1.0 / sqrt(2.0);
            for (n = 0; n<4; n++)
            {
                s[n].Real = (n & 1) ? a:-a;
                s[n].Imag = (n & 2) ? a:-a;
            }
            break;
        }
        case 4:  // 16-QAM
        {
            const wiReal Kmod = 1.0 / sqrt(10.0);
            wiReal c[4] = {-3, 3, -1, 1};

            for (n = 0; n < 4; n++) c[n] *= Kmod;

            for (n = 0; n < 16; n++)
            {
                s[n].Real = c[n & 3];
                s[n].Imag = c[n >> 2];
            }
            break;
        }
        case 6:  // 64-QAM
        {
            const wiReal Kmod = 1.0 / sqrt(42.0);
            wiReal c[8] = {-7, 7,-1, 1,-5, 5,-3, 3};

            for (n = 0; n < 8; n++) c[n] *= Kmod;
            
            for (n = 0; n < 64; n++)
            {
                s[n].Real = c[n & 7];
                s[n].Imag = c[n >> 3];
            }
            break;
        }
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_EnumerateQAM()

// ================================================================================================
// FUNCTION  : Phy11n_DemapperMethod()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns a function pointer for the requested method
// Parameters: DemapperType -- Method type
//             Iteration    -- Loop number for iterative processing (0 is first)
//             pFn          -- Function pointer (by reference)
// ================================================================================================
wiStatus Phy11n_DemapperMethod(int DemapperType, int Iteration, Phy11nDemapperFn *pFn)
{
    wiBoolean First = (Iteration == 0) ? 1 : 0; // first iteration ?

    switch (DemapperType)
    {
        case  1: *pFn = Phy11n_Demapper_LogMap;                                break;
        case  2: *pFn = Phy11n_Demapper_TreeSearchMMSE;                        break;
        case  3: *pFn = Phy11n_MMSE_SingleInversion;                           break;
        case  4: *pFn = Phy11n_MMSE_MultipleInversion;                         break;
        case  5: *pFn = First ? Phy11n_MMSE_SingleInversion : Phy11n_MIMO_MRC; break;
        case  6: *pFn = Phy11n_MIMO_QR;                                        break;
        case  7: *pFn = First ? Phy11n_MIMO_QR : Phy11n_MIMO_MRC;              break;
        case  8: *pFn = Phy11n_ListSphereDecoder;                              break;
        case  9: *pFn = Phy11n_SingleTreeSearchSphereDecoder_Hybrid;           break;
        case 10: *pFn = Phy11n_KbestDemapper;                                  break;
        case 11: *pFn = Phy11n_MLDemapper;                                     break;
        case 12: *pFn = Phy11n_LFSDemapper;                                    break;
        case 13: *pFn = Phy11n_KbvtDemapper;                                   break;
		case 14: *pFn = Phy11n_SingleTreeSearchSphereDecoderFxP_Hybrid;		   break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_DemapperMethod()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

//--< Phy11n_RX.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Phy11n_RX - Phy11n Receiver Model
//  Copyright 2006-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiMath.h"
#include "wiParse.h"
#include "wiUtil.h"
#include "wiRnd.h"
#include "Phy11n.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static const int PunctureA_12[] = { 1 };
static const int PunctureB_12[] = { 1 };

static const int PunctureA_23[] = { 1, 1, 1, 1, 1, 1 };
static const int PunctureB_23[] = { 1, 0, 1, 0, 1, 0 };

static const int PunctureA_34[] = { 1, 1, 0, 1, 1, 0, 1, 1, 0 };
static const int PunctureB_34[] = { 1, 0, 1, 1, 0, 1, 1, 0, 1 };

static const int PunctureA_56[] = { 1, 1, 0, 1, 0 };
static const int PunctureB_56[] = { 1, 0, 1, 0, 1 };

static const double pi = 3.14159265358979323846;

static const int NumDataSubcarriers  = 52;
static const int NumTotalSubcarriers = 56; // data + pilot (total modulated subcarriers)

static const int SubcarrierMap56[] = {
     4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
    33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60
};
static const int DataSubcarrierMap56[] = {
      0, 1, 2, 3, 4, 5, 6, 8, 9,10,11,12,13,14,15,16,17,18,19,20,22,23,24,25,26,27,
     28,29,30,31,32,33,35,36,37,38,39,40,41,42,43,44,45,46,47,49,50,51,52,53,54,55
 };

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#define max(a,b) (((a)>(b))? (a):(b))
#endif

#define Phy11nSubcarrierIsPilotTone(k) (((k == -21+28) || (k == -7+28) || (k == 7+27) || (k == 21+27)) ? 1 : 0)
#define Phy11nSubcarrierIsDataTone(k)  (((k == -21+28) || (k == -7+28) || (k == 7+27) || (k == 21+27)) ? 0 : 1)

// ================================================================================================
// FUNCTION  : Phy11n_RX_DigitalFrontEnd()
// ------------------------------------------------------------------------------------------------
// Purpose   : Frame synchronization and CFO estimation/correction
// Parameters: R  -- array of pointers to receive vectors
//             Nr -- length of vectors in R
// ================================================================================================
wiStatus Phy11n_RX_DigitalFrontEnd(wiComplex *R[], int Nr)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();
    wiComplex w;
    wiReal CFO;
    int i, k, kOffset;
    int k0 = Phy11n_TxState()->aPadFront;

    XSTATUS( Phy11n_RX_FrameSync_CoarseCFO( R, pRX->FrameSyncDelay, k0, &kOffset, &CFO, 
                                            pRX->EnableFrameSync, pRX->CFOEstimation ) );

    if (kOffset < -16) return STATUS(WI_ERROR); // this can cause a memory fault
    if (Nr < pRX->Ny + kOffset) return STATUS(WI_ERROR_PARAMETER2);
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Apply Coarse CFO Correction
    //
    for (i = 0; i < pRX->N_RX; i++)
    {
        wiReal     a = 2.0 * pi * CFO;
        wiComplex *r = R[i] + kOffset;

        if (CFO)
        {
            for (k = 160; k < pRX->Ny; k++)
                pRX->y[i][k] = wiComplexMultiply(r[k], wiComplexExp(-a*k));
        }
        else
        {
            for (k = 160; k < pRX->Ny; k++)
                pRX->y[i][k] = r[k];
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Fine CFO Estimation (using L-LTF) and Correction
    //
    if (pRX->CFOEstimation) // if CFO estmiation is enabled
    {
        wiComplex sum = {0, 0};

        for (k = 160 + 96; k < 160 + 160; k++)
        {
            for (i = 0; i < pRX->N_RX; i++)
            {
                sum.Real +=  pRX->y[i][k].Real * pRX->y[i][k - 64].Real + pRX->y[i][k].Imag * pRX->y[i][k - 64].Imag;
                sum.Imag += -pRX->y[i][k].Real * pRX->y[i][k - 64].Imag + pRX->y[i][k].Imag * pRX->y[i][k - 64].Real;
            }
        }

        CFO = atan2(sum.Imag, sum.Real) / 64.0 / 2.0 / pi;  // discrete frequency offset 

        if (CFO)
        {
            for (i = 0; i < pRX->N_RX; i++)
            {
                for (k = 160; k < pRX->Ny; k++)
                {
                    w.Real =  cos(2 * pi * k * CFO);
                    w.Imag = -sin(2 * pi * k * CFO);
                    pRX->y[i][k] = wiComplexMultiply(pRX->y[i][k], w);
                }
            }
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_DigitalFrontEnd()

// ================================================================================================
// FUNCTION  : Phy11n_RX_EstimateNoise
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate noise using the difference of the L-LTF symbols
// Parameters: none
// ================================================================================================
wiStatus Phy11n_RX_EstimateNoise(void)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();
    wiComplex L1[64], L2[64], e;
    wiReal Se2 = 0.0;
    int i, j;

    for (i = 0; i < pRX->N_RX; i++)
    {
        Phy11n_RX_DFT(pRX->y[i] + 160 + 32,      L1);
        Phy11n_RX_DFT(pRX->y[i] + 160 + 32 + 64, L2);

        for (j = 0; j < 63; j++)
        {
            e = wiComplexSubtract(L1[j], L2[j]);
            Se2 += wiComplexNormSquared(e);
        }
    }
    pRX->NoiseRMS = sqrt(Se2 / (2*64*pRX->N_RX));
    return WI_SUCCESS;
}
// end of Phy11n_RX_EstimateNoise()

// ================================================================================================
// FUNCTION  : Phy11n_RX_SerialToParallelData()
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert data from a serial stream to a byte (octet) array
// Parameters: none
// ================================================================================================
wiStatus Phy11n_RX_SerialToParallelData(void)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();
    int *a = pRX->a;
    int i, k;
    wiUWORD *PSDU;

    GET_WIARRAY( PSDU, pRX->PSDU, pRX->HT_LENGTH, wiUWORD );

    pRX->SERVICE = 0;
    for (k = 0; k < 16; k++)
        pRX->SERVICE = pRX->SERVICE | (*(a++) << k);

    for (i = 0; i < pRX->HT_LENGTH; i++)
    {
        PSDU[i].word = 0;
        for (k = 0; k < 8; k++)
            PSDU[i].word = PSDU[i].w8 | (*(a++) << k);
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_SerialToParallelData()

// ================================================================================================
// FUNCTION  : Phy11n_RX_ComputeDataSymbolDFTs()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute DFTs for data field symbols and extract data and data+pilot tone arrays
// ================================================================================================
wiStatus Phy11n_RX_ComputeDataSymbolDFTs(void)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    wiComplex Y[64];
    int iRx, k, n, p, q;
    int k0 = 640 + 80*pRX->N_HT_DLTF + 16; // offset to first data symbol

    for (iRx = 0; iRx < pRX->N_RX; iRx++)
    {
        for (n = 0; n < pRX->N_SYM; n++)
        {
            Phy11n_RX_DFT(pRX->y[iRx] + 80*n + k0, Y);
            p = q = 0;

            // Copy the DFT output into the Y52 and Y56 arrays, which correspond to data
            // observations, and all modulated observations, respectively.
            //
            for (k = -28; k <= 28; k++)
            {
                if (k == 0) continue;

                if ((k != -21) && (k != -7) && (k != 7) && (k != 21)) {
                    pRX->Y52.val[iRx][52*n + p] = Y[k+32];
                    p++;
                }
                pRX->Y56.val[iRx][56*n + q] = Y[k+32];
                q++;
            }
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_ComputeDataSymbolDFTs()


// ================================================================================================
// FUNCTION  : Phy11n_RX_ChannelTracking()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_ChannelTracking(wiComplex Y56[][56], Phy11nSymbolLLR_t stit_e, wiCMat H56[], int i)
{
    switch (Phy11n_RxState()->ChanlTracking.Type)
    {
        case 0:   // No channel tracking
            break;

        case 1:   // Channel tracking based on demapper soft information
            Phy11n_RX_PilotChannelTracking   (i, Y56, H56);
            Phy11n_RX_DataChannelTracking_LLR(Y56, stit_e, H56);
            break;

        case 2:   // Genie-aided tracking
            Phy11n_RX_PilotChannelTracking     (i, Y56, H56);
            Phy11n_RX_DataChannelTracking_Genie(Y56, H56, i);
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_ChannelTracking()


// ================================================================================================
// FUNCTION  : Phy11n_RX_DPLL
// ------------------------------------------------------------------------------------------------
// Purpose   : Carrier phase tracking: phase error detector and loop-filter
// ================================================================================================
wiStatus Phy11n_RX_DPLL(wiBoolean FirstSymbol, wiReal *pVCOoutput, Phy11nSymbolLLR_t stit_e, 
                        wiComplex Y56[4][56], wiCMat H56[56], int i)
{
    wiReal a, CarrierPhase = 0;
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    if (FirstSymbol) {
        pRX->DPLL.b = pRX->DPLL.VCO = 0.0;  // reset PLL values
    }

    if (stit_e)
    {
        XSTATUS( Phy11n_RX_PhaseErrorDetector(&CarrierPhase, stit_e, Y56, H56, i, pRX->PhaseTrackingType) );

        // Loop Filter and VCO
        a  = pRX->DPLL.K1 * CarrierPhase; // proportional term
        pRX->DPLL.b += pRX->DPLL.K2 * CarrierPhase; // integral term

        pRX->DPLL.VCO += (a + pRX->DPLL.b);
    }
    else pRX->DPLL.VCO += pRX->DPLL.b; // not PED input; just update the integral term

    *pVCOoutput = pRX->DPLL.VCO;

    return WI_SUCCESS;
}
// end of Phy11n_RX_DPLL()


// ================================================================================================
// FUNCTION  : Phy11n_RX_Frame()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_Frame(void)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    int  N_SS    = pRX->N_SS;    // local copy for notational convenience
    int *N_CBPSS = pRX->N_CBPSS;
    int  Nbits   = (16 + 8*pRX->HT_LENGTH) / pRX->N_ES + 6;

    wiCMat *H56 = pRX->H56;
    wiCMat H52[52], Y52[2];
    wiComplex Y56[2][4][56];

    wiReal VCOoutput = 0.0;
    Phy11nSymbolLLR_t stit[2], stit_e[2]; // LLR values per symbol (next is 2nd symbol for STBC)
	Phy11nDemapperFn DemapperFn = NULL;

    wiBoolean PacketUsesSTBC = (pRX->STBC == 0) ? 0 : 1;
    int  M  = PacketUsesSTBC ? 2 : 1; // number of symbols to process in each "n" loop
    int i, j, k, m, n, iRx, Iteration;

    for (k = 0; k < NumDataSubcarriers; k++) {
        wiCMatInit(H52 + k, pRX->N_RX, pRX->N_TX);
    }
    for (m = 0; m < M; m++)
        wiCMatInit(Y52+m, pRX->N_RX, NumDataSubcarriers);

    for (i = 0; i < N_SS; i++)
        memset( pRX->stit_e[i], 0, pRX->Nsp[i] * sizeof(wiReal) );

    // Iterative Demapper/Decoder Cycle ///////////////////////////////////////////////////////////
    //
    for (Iteration = 0; Iteration <= pRX->TotalIterations; Iteration++)
    {
        for (k = 0; k < NumTotalSubcarriers; k++)     // reset the channel estimate at the start
            wiCMatCopy(pRX->InitialH56 + k, H56 + k); // ...of each iteration

        for (n = 0; n < pRX->N_SYM; n += M)    // loop with symbol index
        {
            for (k = 0; k < 52; k++) 
                wiCMatCopy(H56 + DataSubcarrierMap56[k], H52 + k);

            for (m = 0; m < M; m++) // Copy obvservations from pRX->Y56 to local Y56 and Y52
            {
                for (iRx = 0; iRx < pRX->N_RX; iRx++)
                {
                    for (k = 0; k < 56; k++)
                        Y56[m][iRx][k] = pRX->Y56.val[iRx][56*(n+m) + k];
                 
                    for (k = 0; k < 52; k++)
                        Y52[m].val[iRx][k] = Y56[m][iRx][DataSubcarrierMap56[k]];
                }
                if (m) XSTATUS( Phy11n_RX_DPLL(n==0, &VCOoutput, NULL, NULL, NULL, n) ); // advance the VCO for M>1
                XSTATUS( Phy11n_RX_PhaseCorrection(VCOoutput, Y56[m], Y52[m]) );
            }

            for (m = 0; m < M; m++)
                for (i = 0; i < N_SS; i++)
                    for (j = 0; j < N_CBPSS[i]; j++)
                        stit_e[m][i][j] = pRX->stit_e[i][(n+m)*N_CBPSS[i] + j];

            if (!PacketUsesSTBC) //  No STBC  ///////////////////////////////////////////
            {
                XSTATUS( Phy11n_DemapperMethod(pRX->DemapperType, Iteration, &DemapperFn) );
                XSTATUS( DemapperFn(Y52+0, H52, stit[0], stit_e[0], pRX->N_BPSC, pRX->NoiseRMS) );

                for (i = 0; i < N_SS; i++) {
                    for (j = 0; j < N_CBPSS[i]; j++) {
                        pRX->stit[i][n*N_CBPSS[i] + j] = stit[0][i][j];
                        stit_e[0][i][j] += stit[0][i][j]; //posterior info.
                    }
                }
                XSTATUS( Phy11n_RX_DPLL(n==0, &VCOoutput, stit_e[0], Y56[0], H56, n) );
                XSTATUS( Phy11n_RX_ChannelTracking(Y56[0], stit_e[0], H56, n) );
            }
            else //  STBC ///////////////////////////////////////////////////////////////
            {
                XSTATUS( Phy11n_MIMO_MMSE_STBC(Y52, H52, stit, stit_e, pRX->N_BPSC, pRX->NoiseRMS, pRX->STBC) );

                for (i = 0; i < N_SS; i++) {
                    for (j = 0; j < N_CBPSS[i]; j++) {
                        for (m = 0; m < M; m++) {
                            pRX->stit[i][(n+m)*N_CBPSS[i] + j] = stit[m][i][j];
                            stit[m][i][j] = pRX->stit[i][(n+m)*N_CBPSS[i] + j];
                            stit_e[m][i][j] += stit[m][i][j];   //posterior info.
                        }
                    }
                }
                XSTATUS( Phy11n_RX_DPLL(n==0, &VCOoutput, stit_e[1], Y56[1], H56, n+1) ); // phase track with 2nd symbol
                XSTATUS( Phy11n_RX_ChannelTracking(Y56[0], stit_e[0], H56, n) );
                XSTATUS( Phy11n_RX_ChannelTracking(Y56[1], stit_e[1], H56, n+1) );
            }
        }

        for (i = 0; i < N_SS; i++)
            Phy11n_RX_Deinterleave(N_CBPSS[i], pRX->N_BPSC[i], i, pRX->stsp[i], pRX->stit[i], pRX->Nsp[i]);

        Phy11n_RX_StreamDeparser(pRX->Nc, pRX->Lc, pRX->stsp, pRX->N_BPSC, N_SS);

        switch (pRX->DecoderType)
        {
            case  0: XSTATUS( Phy11n_RX_ViterbiDecoder(Nbits, pRX->Lc, pRX->b, pRX->CodeRate12, 64, WI_FALSE) ); break;
            case  1: XSTATUS( Phy11n_RX_ViterbiDecoder(Nbits, pRX->Lc, pRX->b, pRX->CodeRate12, 64, WI_TRUE) ); break;
            case  2: XSTATUS( Phy11n_RX_SOVA          (Nbits, pRX->Lc, pRX->b, pRX->CodeRate12, 64, pRX->Lc_e, 0, 0, 0) ); break;
            case  3: XSTATUS( Phy11n_RX_BCJR          (Nbits, pRX->Lc, pRX->b, pRX->CodeRate12, pRX->Lc_e) ); break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        if (pRX->TotalIterations > 0) // construct prior information to demapper for iterative processing
        {
            Phy11n_RX_StreamParser(pRX->Nc, pRX->Lc_e, pRX->stsp_e, pRX->N_BPSC, N_SS);
            for (i = 0; i < N_SS; i++)
                Phy11n_RX_Interleave(N_CBPSS[i], pRX->N_BPSC[i], i, pRX->stsp_e[i], pRX->stit_e[i], pRX->Nsp[i]);
        }
    } 

    // Free memory      
    for (k = 0; k < NumDataSubcarriers;  k++) wiCMatFree(H52 + k);
    for (m = 0; m < M; m++) wiCMatFree(Y52+m);

    return WI_SUCCESS;
}
// end of Phy11n_RX_Frame()

// ================================================================================================
// FUNCTION  : Phy11n_RX_Symbol
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_Symbol(void)
{
    int i, j, k, m, n, iRx;

    Phy11nDemapperFn DemapperFn = NULL;
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    int SovaState[PHY11N_MAX_ITERATIONS + 1] = {0};
    wiReal VCOoutput = 0.0;

    wiCMat *H56 = pRX->H56;
    wiCMat  H52[52], Y52[2];

    wiComplex Y56[2][4][56];
    wiInt Iteration, s, d, iChanlUpdate;
    Phy11nSymbolLLR_t stit[2], stit_e[2]; // LLR values per symbol

    wiBoolean PacketUsesSTBC = (pRX->STBC == 0) ? 0 : 1;
    int  M  = PacketUsesSTBC ? 2 : 1; // number of symbols to process in each symbol loop
    int  Nbits = (16 + pRX->HT_LENGTH * 8) / pRX->N_ES + 6;

    for (k = 0; k < NumDataSubcarriers; k++)
        wiCMatInit(H52 + k, pRX->N_RX, pRX->N_TX);

    for (m = 0; m < M; m++)
        wiCMatInit(Y52+m, pRX->N_RX, NumDataSubcarriers);

    for (i = 0; i < pRX->N_SS; i++)
        memset( pRX->stit_e[i], 0, pRX->Nsp[i] * sizeof(wiReal) );
    
    memset( pRX->Lc_e, 0, pRX->Nc * sizeof(wiReal) );

    // Demapping and decoding
    for (s = 0; s < pRX->N_SYM + M*(pRX->TotalIterations + 1); s += M) // loop with symbol index
    {
        for (Iteration = 0; Iteration < pRX->TotalIterations + 1; Iteration++)    // Iteration
        {
            n = s - M*Iteration;  // the symbol to be de-mapped
            d = n - M;            // the symbol to be decoded

            if (n < 0) continue; // nothing to do

            if (n < pRX->N_SYM)   // demap the nth symbol
            {
                for (k = 0; k < 52; k++) 
                    wiCMatCopy(H56 + DataSubcarrierMap56[k], H52 + k);

                for (m = 0; m < M; m++)
                {
                    wiCMatGetSubMtx(&(pRX->Y52), Y52+m, 0, 52*(n+m), pRX->N_RX - 1, 52*(n+m+1) - 1);

                    for (iRx = 0; iRx < pRX->N_RX; iRx++)
                        for (k = 0; k < 56; k++)
                                Y56[m][iRx][k] = pRX->Y56.val[iRx][56*(n+m) + k];

                    XSTATUS( Phy11n_RX_PhaseCorrection(VCOoutput, Y56[m], Y52[m]) );

                    for (i = 0; i < pRX->N_SS; i++)
                        for (j = 0; j < pRX->N_CBPSS[i]; j++)
                            stit_e[m][i][j] = pRX->stit_e[i][(n + m) * pRX->N_CBPSS[i] + j];
                }
            }
            if (n <= pRX->N_SYM - M)   // demap the nth symbol
            {
                if (!PacketUsesSTBC)     // ************* No STBC ***************
                {
                    XSTATUS( Phy11n_DemapperMethod(pRX->DemapperType, Iteration, &DemapperFn) );
                    XSTATUS( DemapperFn(Y52+0, H52, stit[0], stit_e[0], pRX->N_BPSC, pRX->NoiseRMS) );

                    for (i = 0; i < pRX->N_SS; i++) {
                        for (j = 0; j < pRX->N_CBPSS[i]; j++)
                        {
                            pRX->stit[i][n * pRX->N_CBPSS[i] + j] = stit[0][i][j];
                            stit_e[0][i][j] += stit[0][i][j]; //posterior info.
                        }
                    }
                    //  Phase error tracking [Barrett: Not sure this is the right location]
                    //
                    XSTATUS( Phy11n_RX_DPLL(n==0 && Iteration==0, &VCOoutput, stit_e[0], Y56[0], H56, n) );

                    // 2: Soft-info. DEM feed back channel tracking
                    if ((pRX->ChanlTracking.Type == 2) && (Iteration == pRX->TotalIterations))
                    {
                        Phy11n_RX_PilotChannelTracking(n, Y56[0], H56);
                        Phy11n_RX_DataChannelTracking_LLR (Y56[0], stit_e[0], H56);
                    }
                } 
                else // ************* We have STBC ***************
                {
                    XSTATUS( Phy11n_MIMO_MMSE_STBC(Y52, H52, stit, stit_e, pRX->N_BPSC, pRX->NoiseRMS, pRX->STBC) );

                    for (m = 0; m < 2; m++) {
                        for (i = 0; i < pRX->N_SS; i++) {
                            for (j = 0; j < pRX->N_CBPSS[i]; j++) {
                                pRX->stit[i][(n + m) * pRX->N_CBPSS[i] + j] = stit[m][i][j];
                                stit_e[m][i][j] += stit[m][i][j];   //posterior info.
                            }
                        }
                    }
                    XSTATUS( Phy11n_RX_DPLL(n==0 && Iteration==0, &VCOoutput, stit_e[0], Y56[0], H56, n) );

                    for (m = 0; m < M; m++)
                        XSTATUS( Phy11n_RX_ChannelTracking(Y56[m], stit_e[m], H56, n+m) );
                }
            }
            // [Hossein] : Currently, De-interleaving, Stream Parsing, Interleaving, and
            //             re-parsing are implemented in a frame-wise rather than a 
            //             symbol-wise fashion
            //
            if ((d >= 0) && (d <= pRX->N_SYM - M))
            {
                for (i = 0; i < pRX->N_SS; i++)
                    Phy11n_RX_Deinterleave(pRX->N_CBPSS[i], pRX->N_BPSC[i], i, pRX->stsp[i], pRX->stit[i], pRX->Nsp[i]);

                XSTATUS(Phy11n_RX_StreamDeparser(pRX->Nc, pRX->Lc, pRX->stsp, pRX->N_BPSC, pRX->N_SS)); // Stream Parsing

                for (m = 0; m < M; m++) {
                    XSTATUS( Phy11n_RX_SOVA( Nbits, pRX->Lc, pRX->b, pRX->CodeRate12, 64, pRX->Lc_e, 
                                             d + m, pRX->N_SYM, SovaState+Iteration ) );
                }    
                Phy11n_RX_StreamParser(pRX->Nc, pRX->Lc_e, pRX->stsp_e, pRX->N_BPSC, pRX->N_SS);  // Re-Stream Parsing
                for (i = 0; i < pRX->N_SS; i++)
                    Phy11n_RX_Interleave(pRX->N_CBPSS[i], pRX->N_BPSC[i], i, pRX->stsp_e[i], pRX->stit_e[i], pRX->Nsp[i]);
            }

            if (!PacketUsesSTBC) // [Barrett]: The code below is functionally equivalent to the intern model.
            {                    //            It does not handle STBC!
                iChanlUpdate = d - 3; // symbol index for decoder feedback tracking

                // channel is updated when the decoder output of the last Iteration. stage is available.
                if ((n < pRX->N_SYM - 1) && (iChanlUpdate >= 0) && (d < pRX->N_SYM) 
                    && (Iteration == pRX->TotalIterations) && (pRX->ChanlTracking.Type != 0))
                {
                    // Copy the observation signals and extrinsic information for symbol iChanlUpdate
                    //
                    for (iRx = 0; iRx < pRX->N_RX; iRx++) {
                        for (k = 0; k < 56; k++)
                            Y56[0][iRx][k] = pRX->Y56.val[iRx][56*iChanlUpdate + k];
                    }
                    for (i = 0; i < pRX->N_SS; i++) {
                        for (j = 0; j < pRX->N_CBPSS[i]; j++)
                            stit_e[0][i][j] = pRX->stit_e[i][iChanlUpdate * pRX->N_CBPSS[i] + j] 
                                              + pRX->stit[i][iChanlUpdate * pRX->N_CBPSS[i] + j];   // Copy decoded information
                    }

                    switch (pRX->ChanlTracking.Type) // Channel Tracking //////////////////////////
                    {
                        case 0: break;
                        case 1:    //Softtracking
                            Phy11n_RX_PilotChannelTracking(n, Y56[0], H56);
                            Phy11n_RX_DataChannelTracking_LLR (Y56[0], stit_e[0], H56);
                            return STATUS(WI_ERROR);
                            break;
                        case 2:
                            break;
                        case 3:    //Genuine tracking
                            Phy11n_RX_PilotChannelTracking(n, Y56[0], H56);
                            Phy11n_RX_DataChannelTracking_Genie    (Y56[0], H56, n);
                            break;
                        default:
                            return STATUS(WI_ERROR_UNDEFINED_CASE);
                    }
                }
            }
        }  // end of symbol loop
    }  // end of iteration loop

    for (k = 0; k < 52; k++) wiCMatFree(H52 + k);
    for (m = 0; m <  M; m++) wiCMatFree(Y52 + m);

    return WI_SUCCESS;
}
// end of Phy11n_RX_Symbol()

// ================================================================================================
// FUNCTION  : Phy11n_RX_BCJR()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_BCJR(int Nbits, wiReal L[], int x[], int CodeRate12, wiReal Le[])
{
    const double inf = 1.0e10;
    const int *pA, *pB;

    int k, M, p1 = 0, p2 = 0;
    int A0[64], A1[64], B0[64], B1[64], C0[64], C1[64], D0[64], D1[64], S, S0, S1, X;

    wiReal gamma0, gamma1, temp0, temp1, temp0_A, temp1_A, temp0_B, temp1_B;
    wiReal *alpha[64], *beta[64];
    wiReal llr_A = 0, llr_B = 0;

    // ------------
    // Uncoded case
    // ------------
    if (CodeRate12 == 12) // uncoded case
    {
        for (k = 0; k < Nbits; k++)  {
            x[k] = (L[k] >= 0.0) ? 1 : 0;
        }
        return WI_SUCCESS;
    }
    // -----------------------------
    // Select the puncturing pattern
    // -----------------------------
    switch (CodeRate12)
    {
        case  6: pA = PunctureA_12;  pB = PunctureB_12;  M = 1;  break;
        case  8: pA = PunctureA_23;  pB = PunctureB_23;  M = 6;  break;
        case  9: pA = PunctureA_34;  pB = PunctureB_34;  M = 9;  break;
        case 10: pA = PunctureA_56;  pB = PunctureB_56;  M = 5;  break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // ----------------------------------------------------
    // Allocate memory
    // ----------------------------------------------------
    for (S = 0; S < 64; S++)
    {
        GET_WIARRAY( alpha[S], Phy11n_RxState()->BCJR.alpha[S], Nbits, wiReal );
        GET_WIARRAY( beta[S],  Phy11n_RxState()->BCJR.beta[S],  Nbits, wiReal );
    }
    // ----------------------------------------------------------
    // Build a table of coded bits as a function of old+new state
    // ----------------------------------------------------------
    for (S = 0; S < 64; S++)
    {
        A0[S] = ((S >> 0) ^ (S >> 2) ^ (S >> 3) ^ (S >> 5) ^ 0) & 1;
        A1[S] = ((S >> 0) ^ (S >> 2) ^ (S >> 3) ^ (S >> 5) ^ 1) & 1;

        B0[S] = ((S >> 0) ^ (S >> 1) ^ (S >> 2) ^ (S >> 3) ^ 0) & 1;
        B1[S] = ((S >> 0) ^ (S >> 1) ^ (S >> 2) ^ (S >> 3) ^ 1) & 1;

        X = (S << 1) | 0;
        C0[S] = ((X >> 0) ^ (X >> 2) ^ (X >> 3) ^ (X >> 5) ^ (X >> 6)) & 1;
        D0[S] = ((X >> 0) ^ (X >> 1) ^ (X >> 2) ^ (X >> 3) ^ (X >> 6)) & 1;

        X = (S << 1) | 1;
        C1[S] = ((X >> 0) ^ (X >> 2) ^ (X >> 3) ^ (X >> 5) ^ (X >> 6)) & 1;
        D1[S] = ((X >> 0) ^ (X >> 1) ^ (X >> 2) ^ (X >> 3) ^ (X >> 6)) & 1;
    }
    // -------------------------
    // Initialize alpha and beta
    // -------------------------
    alpha[0][0] = beta[0][Nbits-1] = 0;
    for (S = 1; S < 64; S++)
        alpha[S][0] = beta[S][Nbits-1] = -inf;

    // ----------------------------
    // trace forward, compute alpha 
    // ----------------------------
    p1 = 0;
    for (k = 1; k < Nbits; k++)
    {
        llr_A = pA[(k-1)%M] ? L[p1++] : 0;
        llr_B = pB[(k-1)%M] ? L[p1++] : 0;

        for (S = 0; S < 64; S++)
        {
            S0 = (S >> 1);      // previous state with last register 0
            S1 = S0 | 0x20;     // previous state with last register 1

            // compute gamma 
            // the term -log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B)) is constant for all summation
            // terms and thus can be omitted
            gamma0 = A0[S] * llr_A + B0[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));
            gamma1 = A1[S] * llr_A + B1[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));
            alpha[S][k] = Phy11n_LogExpAdd(gamma0 + alpha[S0][k - 1], gamma1 + alpha[S1][k - 1]);
        }
    }

    // ----------------------------
    // trace backward, compute beta 
    // ----------------------------
    p1 = 12 * Nbits / CodeRate12 - 1;
    for (k = Nbits - 2; k >= 0; k--)
    {
        llr_B = pB[(k+1)%M] ? L[p1--] : 0;
        llr_A = pA[(k+1)%M] ? L[p1--] : 0;

        for (S = 0; S < 64; S++)
        {
            S0 = (0x3F & (S << 1)) | 0; // next state with input 0
            S1 = (0x3F & (S << 1)) | 1; // next state with input 1

            // compute gamma 
            gamma0 = C0[S] * llr_A + D0[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));
            gamma1 = C1[S] * llr_A + D1[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));
            beta[S][k] = Phy11n_LogExpAdd(gamma0 + beta[S0][k + 1], gamma1 + beta[S1][k + 1]);
        }
    }

    // ----------------------------
    // compute the LLR of the info. 
    // bit and the coded bits
    // ----------------------------
    p1 = p2 = 0;
    for (k = 0; k < Nbits; k++)
    {
        llr_A = pA[k%M] ? L[p1++] : 0;
        llr_B = pB[k%M] ? L[p1++] : 0;

        temp0 = temp0_A = temp0_B = -1e10;
        temp1 = temp1_A = temp1_B = -1e10;

        for (S = 0; S < 64; S++)
        {
            S0 = (0x3F & (S << 1)) | 0; // next state with input 0
            S1 = (0x3F & (S << 1)) | 1; // next state with input 1

            // compute the LLR of the info. bits
            gamma0 = C0[S] * llr_A + D0[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));
            gamma1 = C1[S] * llr_A + D1[S] * llr_B; //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B));

            temp0 = Phy11n_LogExpAdd(temp0, alpha[S][k] + gamma0 + beta[S0][k]);
            temp1 = Phy11n_LogExpAdd(temp1, alpha[S][k] + gamma1 + beta[S1][k]);

            // compute the LLR for 1st coded bit
            if (C0[S] == 0) temp0_A = Phy11n_LogExpAdd(temp0_A, alpha[S][k] + gamma0 + beta[S0][k]);
            else            temp1_A = Phy11n_LogExpAdd(temp1_A, alpha[S][k] + gamma0 + beta[S0][k]);

            if (C1[S] == 0) temp0_A = Phy11n_LogExpAdd(temp0_A, alpha[S][k] + gamma1 + beta[S1][k]);
            else            temp1_A = Phy11n_LogExpAdd(temp1_A, alpha[S][k] + gamma1 + beta[S1][k]);

            // compute the LLR for 2nd coded bit
            if (D0[S] == 0) temp0_B = Phy11n_LogExpAdd(temp0_B, alpha[S][k] + gamma0 + beta[S0][k]);
            else            temp1_B = Phy11n_LogExpAdd(temp1_B, alpha[S][k] + gamma0 + beta[S0][k]);

            if (D1[S] == 0) temp0_B = Phy11n_LogExpAdd(temp0_B, alpha[S][k] + gamma1 + beta[S1][k]);
            else            temp1_B = Phy11n_LogExpAdd(temp1_B, alpha[S][k] + gamma1 + beta[S1][k]);

        }
        x[k] = temp1 > temp0 ? 1 : 0;
        
        if (pA[k % M]) Le[p2++] = temp1_A - temp0_A;  // compute the LLR for 1st coded bit
        if (pB[k % M]) Le[p2++] = temp1_B - temp0_B;  // compute the LLR for 2nd coded bit
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_BCJR()

// ================================================================================================
// FUNCTION  : Phy11n_RX_SOVA_Core()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_SOVA_Core(int Nbits, wiReal L[], int OutputData[], int CodeRate12, int Depth, 
                             wiReal Le[], wiInt sym, wiInt N_SYM, wiInt *pLastState)
{
    wiBoolean SymbolProcessing = (pLastState == NULL) ? WI_FALSE : WI_TRUE; // frame or symbol processing

    Phy11n_SOVA_t *pSOVA = Phy11n_RxState()->SOVA;

    wiInt (*state)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->state;
    wiInt (*bit_S)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->bit_S;
    wiInt (*bit_A)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->bit_A;
    wiInt (*bit_B)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->bit_B;
    wiReal (*Le_S)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->Le_S;
    wiReal (*Le_A)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->Le_A;
    wiReal (*Le_B)[64][PHY11N_MAX_SOVA_WINDOW + 1] = pSOVA->Le_B;

    wiReal Mk[2][64], Mk_max;
    wiReal llr_A = 0, llr_B = 0, m0 = 0, m1 = 0, delta;

    int i, j, k, k0 = 0, k1, M, p1, p2, tau;
    int A0[64], A1[64], B0[64], B1[64], S, S0, S1, Sf, Sx;
    int StartBit, EndBit;
    
    const int *pA, *pB;
    const int PositionModulus = PHY11N_SOVA_POSITION_LENGTH;

    if (Depth > PHY11N_MAX_SOVA_WINDOW) return WI_ERROR_PARAMETER5;

    if (SymbolProcessing) // symbol-by-symbol processing
    {
        if (sym >= N_SYM) return STATUS(WI_ERROR_PARAMETER7);
        p1 = pSOVA->Position1[sym % PositionModulus];
        p2 = pSOVA->Position2[sym % PositionModulus];
        StartBit = sym * Nbits / N_SYM;    //= sym*pRX->N_DBPS
        EndBit  = (sym + 1) * Nbits / N_SYM;
        Sx = *pLastState;
    }
    else // full frame processing
    {
        p1 = p2 = 0;
        StartBit = 0;
        EndBit = Nbits;
        Sx = 0;
    }

    tau = min(Depth, Nbits - StartBit); // Restrict the path memory depth to the message length

    // -----------------------------
    // Select the puncturing pattern
    // -----------------------------
    switch (CodeRate12)
    {
        case  6: pA = PunctureA_12;  pB = PunctureB_12;  M = 1;  break;
        case  8: pA = PunctureA_23;  pB = PunctureB_23;  M = 6;  break;
        case  9: pA = PunctureA_34;  pB = PunctureB_34;  M = 9;  break;
        case 10: pA = PunctureA_56;  pB = PunctureB_56;  M = 5;  break;
        default: return WI_ERROR_PARAMETER4;
    }
    // ----------------------------------------------------------
    // Build a table of coded bits as a function of old+new state
    // ----------------------------------------------------------
    for (S = 0; S < 64; S++)
    {
        A0[S] = ((S >> 0) ^ (S >> 2) ^ (S >> 3) ^ (S >> 5) ^ 0) & 1;
        A1[S] = ((S >> 0) ^ (S >> 2) ^ (S >> 3) ^ (S >> 5) ^ 1) & 1;

        B0[S] = ((S >> 0) ^ (S >> 1) ^ (S >> 2) ^ (S >> 3) ^ 0) & 1;
        B1[S] = ((S >> 0) ^ (S >> 1) ^ (S >> 2) ^ (S >> 3) ^ 1) & 1;
    }
    // ----------------------
    // Initialize the metrics
    // ----------------------
    k1 = (StartBit + 1) % 2;
    for (S = 0; S < 64; S++) {
        for (k = 0; k < 2; k++) {
            for (i = 0; i <= tau; i++)
            {
                Le_S[k][S][i] = 1.0e+6;
                Le_A[k][S][i] = 1.0e+6;
                Le_B[k][S][i] = 1.0e+6;
            }
        }
        Mk[k1][S] = -1.0E+6; 
    }
    Mk[k1][Sx] = 0.0; // initial state

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Decoder Processing Loop
    //
    for (k = StartBit; k < min(Nbits, EndBit + tau); k++)
    {
        llr_A = llr_B = 0;

        k0 = (k + 0) % 2;
        k1 = (k + 1) % 2;

        if (pA[k % M]) llr_A = L[p2++];
        if (pB[k % M]) llr_B = L[p2++];

        ////////////////////////////////////////////////////////////////////////////
        //
        // Loop through state-by-state operations
        //
        for (S = 0; S < 64; S++)
        {
            S0 = (S >> 1);
            S1 = S0 | 0x20;

            // Compute path metric in log domain
            // the common term -log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B)) is cancelled
            m0 = Mk[k1][S0] + A0[S] * llr_A + B0[S] * llr_B;    //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B))
            m1 = Mk[k1][S1] + A1[S] * llr_A + B1[S] * llr_B;    //-log(1+exp_clip(llr_A))-log(1+exp_clip(llr_B))

            // --------------------
            // Select survivor path
            // --------------------
            if (m0 >= m1)
            {
                Mk[k0][S] = m0;
                Sx = S0;
                Sf = S1;
                delta = m0 - m1;
                bit_A[k0][S][0] = A0[S];
                bit_B[k0][S][0] = B0[S];
            }
            else
            {
                Mk[k0][S] = m1;
                Sx = S1;
                Sf = S0;
                delta = m1 - m0;
                bit_A[k0][S][0] = A1[S];
                bit_B[k0][S][0] = B1[S];
            }
            bit_S[k0][S][0] = S & 1;
            state[k0][S][0] = Sx;   //Survivor previous state

            for (i = 1; i <= tau; i++)
            {
                bit_S[k0][S][i] = bit_S[k1][Sx][i-1];
                bit_A[k0][S][i] = bit_A[k1][Sx][i-1];
                bit_B[k0][S][i] = bit_B[k1][Sx][i-1];
                Le_S[k0][S][i]  = Le_S[k1][Sx][i-1];
                Le_A[k0][S][i]  = Le_A[k1][Sx][i-1];
                Le_B[k0][S][i]  = Le_B[k1][Sx][i-1];

                state[k0][S][i] = state[k1][Sx][i-1];
            }
            // -----------------------
            // update soft information
            // -----------------------
            j = (k >= tau) ? tau : k;
            for (i = 1; i <= j; i++)
            {
                if (bit_A[k1][Sx][i-1] == bit_A[k1][Sf][i-1])
                    Le_A[k0][S][i] = min(Le_A[k1][Sf][i-1] + delta, Le_A[k1][Sx][i-1]);
                else
                    Le_A[k0][S][i] = min(Le_A[k1][Sx][i-1], delta);

                if (bit_B[k1][Sx][i-1] == bit_B[k1][Sf][i-1])
                    Le_B[k0][S][i] = min(Le_B[k1][Sf][i-1] + delta, Le_B[k1][Sx][i-1]);
                else
                    Le_B[k0][S][i] = min(Le_B[k1][Sx][i-1], delta);
            }
            Le_A[k0][S][0] = (A0[S] == A1[S]) ? 10.0e+6 : delta;
            Le_B[k0][S][0] = (B0[S] == B1[S]) ? 10.0e+6 : delta;

        } // end state-by-state loop S /////////////////////////////////////////////

        if (k - StartBit >= tau) // Output a decision bit
        {
            Sx = 0;
            Mk_max = Mk[k0][0];
            for (S = 1; S < 64; S++)
            {
                if (Mk[k0][S] > Mk_max)
                {
                    Mk_max = Mk[k0][S];
                    Sx = S;
                }
            }
            OutputData[k - tau] = bit_S[k0][Sx][tau];

            // Update the extrinsic information
            if (pA[(k - tau) % M]) Le[p1++] = Le_A[k0][Sx][tau] * (2 * bit_A[k0][Sx][tau] - 1);
            if (pB[(k - tau) % M]) Le[p1++] = Le_B[k0][Sx][tau] * (2 * bit_B[k0][Sx][tau] - 1);
        }
        // store the initial positions, p1 and p2, for the next symbol
        if (SymbolProcessing && (k == EndBit - 1))
        {
            pSOVA->Position2[(sym + 1) % PositionModulus] = p2;

        }
    } // end k ///////////////////////////////////////////////////////////////////////////////

    for (; k < EndBit + tau; k++)  // Enters this loop if Nbits<EndBit+tau
    {
        int j = Nbits + tau - 1 - k;
        OutputData[k-tau] = bit_S[k0][0][j];

        if (pA[(k-tau) % M]) Le[p1++] = (2 * bit_A[k0][0][j] - 1) * Le_A[k0][0][j];
        if (pB[(k-tau) % M]) Le[p1++] = (2 * bit_B[k0][0][j] - 1) * Le_B[k0][0][j];
    }
    // Determine the final state for stage EndBit
    if (SymbolProcessing)
    {
        if (EndBit == Nbits)
            *pLastState = 0;
        else
        {
            if (EndBit + tau >= Nbits)
                *pLastState = state[k0][0][Nbits - EndBit - 1];
            else
                *pLastState = state[k0][Sx][tau - 1]; // Sx is the survivor state at k = EndBit - 1
        }
        pSOVA->Position1[(sym + 1) % PositionModulus] = p1;
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_SOVA_Core()


// ================================================================================================
// FUNCTION  : Phy11n_RX_SOVA()
// ------------------------------------------------------------------------------------------------
// Purpose   : Soft-output Viterbi algorithm (wrapper for memory allocation)
// Parameters: Nbits      -- number of bits to decode
//             L          -- input LLRs
//             OutputData -- decoded data bits
//             CodeRate12 -- code rate * 12
//             Depth      -- truncation depth
//             Le         -- extrinsic information (soft-output)
//             sym        -- symbol index (for symbol-by-symbol processing)
//             N_SYM      -- total number of symbols (for symbol-by-symbol processing)
//             pLastState -- end state for symbol-by-symbol processing; NULL for full message
// ================================================================================================
wiStatus Phy11n_RX_SOVA(int Nbits, wiReal L[], int OutputData[], int CodeRate12, int Depth, wiReal Le[], 
                        wiInt sym, wiInt N_SYM, wiInt *pLastState)
{
    if (CodeRate12 == 12) // uncoded case
    {
        int k;
        for (k = 0; k < Nbits; k++)
            OutputData[k] = (L[k] >= 0.0) ? 1 : 0;
    }
    else // general case...allocate memory (if needed) and call the core function
    {
        Phy11n_RxState_t *pRX = Phy11n_RxState();

        if (!pRX->SOVA) WICALLOC( pRX->SOVA, 1, Phy11n_SOVA_t );
        XSTATUS( Phy11n_RX_SOVA_Core(Nbits, L, OutputData, CodeRate12, Depth, Le, sym, N_SYM, pLastState) );
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_SOVA()

// ======================================================================================
// FUNCTION  : Phy11n_RX_Interleave()
// --------------------------------------------------------------------------------------
// Purpose   : interleaver for 20MHz
// Parameters: 
// ======================================================================================
wiStatus Phy11n_RX_Interleave(int N_CBPSS, int N_BPSC, int Iss, wiReal x[], wiReal y[], int Nsp)
{
    int Nrow = 4 * N_BPSC;
    int Nrot = 11;
    int Ncol = 13;

    int i, j, r, k, m;
    int s = max(N_BPSC / 2, 1);

    Ncol = 13;
    Nrot = 11;
    Nrow = 4 * N_BPSC;

    if (Phy11n_TxState()->InterleaverDisabled)
    {
        memcpy(x, y, Nsp * sizeof(wiReal));
    }
    else
    {
        for (m = 0; m < Nsp / N_CBPSS; m++)
        {
            for (k = 0; k < N_CBPSS; k++)
            {
                i = Nrow * (k % Ncol) + (k / Ncol);
                j = s * (i / s) + ((i + N_CBPSS - (Ncol * i / N_CBPSS)) % s);
                r = (N_CBPSS + (j - ((Iss * 2) % 3 + 3 * (Iss / 3)) * Nrot * N_BPSC)) % N_CBPSS;
                y[m * N_CBPSS + r] = x[m * N_CBPSS + k];
            }
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_Interleave()

// ================================================================================================
// FUNCTION  : Phy11n_RX_Deinterleave()
// ------------------------------------------------------------------------------------------------
// Purpose   : 802.11 OFDM Deinterleaver (20 MHz channels)
// Parameters: N_CBPSS -- Number of code bits per symbol per spatial stream
//             N_BPSC  -- Number of code bits per subcarrier
//             Iss     -- Spatial stream index {0, 1, 2, 3}
//             x       -- Output array
//             y       -- Input array
//             Nsp     -- Number of code bits per spatial stream (includes padding)
// ================================================================================================
wiStatus Phy11n_RX_Deinterleave(int N_CBPSS, int N_BPSC, int Iss, wiReal x[], wiReal y[], int Nsp)
{
    int Ncol = 13;
    int Nrot = 11;
    int Nrow = 4 * N_BPSC;
    int s = max(N_BPSC / 2, 1);

    int i, j, r, k, m;

    if (Phy11n_TxState()->InterleaverDisabled)
    {
        memcpy(x, y, Nsp * sizeof(wiReal));
    }
    else
    {
        for (m = 0; m < Nsp / N_CBPSS; m++)
        {
            for (k = 0; k < N_CBPSS; k++)
            {
                i = Nrow * (k % Ncol) + (k / Ncol);
                j = s * (i / s) + ((i + N_CBPSS - (Ncol * i / N_CBPSS)) % s);
                r = (N_CBPSS + (j - ((Iss * 2) % 3 + 3 * (Iss / 3)) * Nrot * N_BPSC)) % N_CBPSS;
                x[m * N_CBPSS + k] = y[m * N_CBPSS + r];
            }
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_Deinterleave()

// ================================================================================================
// FUNCTION  : Phy11n_RX_StreamParser()
// ------------------------------------------------------------------------------------------------
// Purpose   : Stream Parser
// Parameters: pState -- Pointer to the state structure (by reference)
// ================================================================================================
wiStatus Phy11n_RX_StreamParser(int Nc, wiReal Lc[], wiReal * stsp[], int N_BPSC[], int N_SS)
{
    int s[4], s_sum = 0;
    int i, j, k, pt = 0;

    for (i = 0; i < N_SS; i++)
    {
        s[i] = max(1, N_BPSC[i] / 2);
        s_sum += s[i];
    }

    for (k = 0; k < Nc / s_sum; k++)
        for (i = 0; i < N_SS; i++)
            for (j = 0; j < s[i]; j++)
                stsp[i][k * s[i] + j] = Lc[pt++];

    return WI_SUCCESS;
}
// end of Phy11n_RX_StreamParser()

 // ===============================================================================================
// FUNCTION  : Phy11n_RX_StreamDeparser()
// ------------------------------------------------------------------------------------------------
// Purpose   : Stream Deparser
// Parameters: pState -- Pointer to the state structure (by reference)
// ================================================================================================
wiStatus Phy11n_RX_StreamDeparser(int Nc, wiReal Lc[], wiReal * stsp[], int N_BPSCS[], int N_SS)
{
    int s[4], s_sum = 0;
    int i, j, k, pt;

    for (i = 0; i < N_SS; i++)
    {
        s[i] = max(1, N_BPSCS[i] / 2);
        s_sum += s[i];
    }
    pt = 0;
    for (k = 0; k < Nc / s_sum; k++)
        for (i = 0; i < N_SS; i++)
            for (j = 0; j < s[i]; j++)
                Lc[pt++] = stsp[i][k * s[i] + j];

    return WI_SUCCESS;
}
// end of Phy11n_RX_StreamDeparser()

// ================================================================================================
// FUNCTION  : Phy11n_RX_Descrambler()
// ------------------------------------------------------------------------------------------------
// Purpose   : Scramble the input sequence using PRBS x^7+x^4+1. Because the operation is modulo-2,
//             it undoes a prior scrambler with the same polynomial. The first seven bits of the 
//             "unscrambled" bit sequence are zero, so this is used to determine the state of the 
//             scrambler shift register.
// Parameters: n -- Number of bits to scramble
//             b -- Input array
//             a -- Output array
// ================================================================================================
wiStatus Phy11n_RX_Descrambler(int n, int b[], int a[])
{
    if (Phy11n_TxState()->ScramblerDisabled)
    {
        memcpy(a, b, n * sizeof(int));
    }
    else
    {
        int k, x, X = 0;
        int X0, X1, X2, X3, X4, X5, X6;

        // ------------------------------------
        // Get the shift register initial state
        // ------------------------------------
        X0 = b[6] ^ b[2];
        X1 = b[5] ^ b[1];
        X2 = b[4] ^ b[0];
        X3 = b[3] ^ X0;
        X4 = b[2] ^ X1;
        X5 = b[1] ^ X2;
        X6 = b[0] ^ X3;

        X = X0 | (X1 << 1) | (X2 << 2) | (X3 << 3) | (X4 << 4) | (X5 << 5) | (X6 << 6);

        // ----------------------------------------------
        // Apply the scrambler to the rest of the message
        // ----------------------------------------------
        for (k = 0; k < n; k++)
        {
            x = ((X >> 6) ^ (X >> 3)) & 1;  // P(X) = X^7 + X^4 + 1;
            X = (X << 1) | x;
            a[k] = b[k] ^ x;
        }
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_Descrambler()

// ================================================================================================
//  Function  : Phy11n_OFDM_ViterbiDecoder()                                      
//-------------------------------------------------------------------------------------------------
//  Purpose   : Implement a soft-input Viterbi decoder for the rate 1/2 convolutional code
//              specified for 802.11a/g/n (OFDM).                     
//  Parameters: NumBits         -- Number of (decoded) bits to compute in OutputData
//              pInputLLR       -- Input array of log-likelihood ratios                 
//              OutputData      -- Output array (decoded data bits)                     
//              CodeRate12      -- CodeRate * 12...defines the puncturing pattern       
//              TruncationDepth -- Trellis trunction depth
//              HardDecoding    -- Limit LLR values to sign{LLR} = {-1, 0, +1}
// ================================================================================================
wiStatus Phy11n_RX_ViterbiDecoder(int NumBits, wiReal pInputLLR[], int OutputData[], int CodeRate12, 
                                  int TruncationDepth, wiBoolean HardDecoding)
{
    int i, k, M;
    const int *pA, *pB;
    int nSR_1, tau;
    int A0[64], A1[64], B0[64], B1[64], S, S0, S1, Sx;    //A0, A1: output register for bit 0 corresponding to 

    unsigned int Pk[64][8], Pk_1[64][8], outBit;     //use two memories to do register exchange;

    wiReal Mk[64], Mk_1[64], MinMetric;
    wiReal LLR_A, LLR_B, m0, m1;
    
    if (InvalidRange(NumBits, 7, 16777216)) return WI_ERROR_PARAMETER1;
    if (pInputLLR == NULL) return WI_ERROR_PARAMETER2;
    if (OutputData == NULL) return WI_ERROR_PARAMETER3;
    if (InvalidRange(TruncationDepth, 15, 255)) return WI_ERROR_PARAMETER4;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Select the puncturing pattern
    //  Parameter CodeRate12 is the code rate multiplied by 12; i.e., for an unpunctured
    //  rate 1/2 code, CodeRate12 = 12(1/2) = 6.
    //  
    switch (CodeRate12)
    {
        case 6:  pA = PunctureA_12;  pB = PunctureB_12;  M = 1;  break;
        case 8:  pA = PunctureA_23;  pB = PunctureB_23;  M = 6;  break;
        case 9:  pA = PunctureA_34;  pB = PunctureB_34;  M = 9;  break;
		case 10: pA = PunctureA_56;  pB = PunctureB_56;  M = 5;  break; // M: pattern length 
        case 12:
        {
            for (k=0; k<NumBits; k++) // uncoded...simple hard decision
                OutputData[k] = (pInputLLR[k] >= 0.0) ? 1:0;

            return WI_SUCCESS;
            break;
        }
        default: return WI_ERROR_PARAMETER5;
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Restrict the path memory depth (tau) to the message length. Then compute the
    //  number of shift registers required along with the position of the output bit.
    //
    tau = (TruncationDepth > NumBits) ? NumBits : TruncationDepth;
    nSR_1 = ((tau+32) / 32) - 1; 
    outBit = 1 << (tau % 32);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clear the path history registers and initialize the metrics (bias so state 0 has
    //  the best starting metric.
    //
    for (S=0; S<64; S++)
        for (i=0; i<8; i++)
            Pk_1[S][i] = 0;

    for (S=1; S<64; S++)
        Mk_1[S] = 1.0E+6;  // suppress these states
    Mk_1[0] = 0.0;         // initial state is state 0

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Build a table of coded bits as a function of old+new state
    //
    for (S=0; S<64; S++)
    {
        A0[S] = ((S>>0) ^ (S>>2) ^ (S>>3) ^ (S>>5) ^ 0) & 1;
        A1[S] = ((S>>0) ^ (S>>2) ^ (S>>3) ^ (S>>5) ^ 1) & 1;

        B0[S] = ((S>>0) ^ (S>>1) ^ (S>>2) ^ (S>>3) ^ 0) & 1;
        B1[S] = ((S>>0) ^ (S>>1) ^ (S>>2) ^ (S>>3) ^ 1) & 1;
    }
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    // Apply the Viterbi decoder
    // 
    for (k=0; k<NumBits; k++)
    {
        LLR_A = pA[k%M] ? *(pInputLLR++) : 0.0;
        LLR_B = pB[k%M] ? *(pInputLLR++) : 0.0;

        if (HardDecoding)
        {
            if (LLR_A > 0) LLR_A = +1; else if (LLR_A < 0) LLR_A = -1;
            if (LLR_B > 0) LLR_B = +1; else if (LLR_B < 0) LLR_B = -1; 
        }

        ////////////////////////////////////////////////////////////////////////////
        //
        //  Loop through state-by-state operations
        //
        for (S=0; S<64; S++)
        {
            S0 = (S >> 1);  // The MSB is 0
            S1 = S0 | 0x20; // The MSB is 1

            ////////////////////////////////////////////////////////////////////////
            //
            //  Metric computation
            //  The sign of the LLR is reversed so a "minimum" metric is favored
            //
            m0 = Mk_1[S0] + (A0[S]? -LLR_A:LLR_A) + (B0[S]? -LLR_B:LLR_B);
            m1 = Mk_1[S1] + (A1[S]? -LLR_A:LLR_A) + (B1[S]? -LLR_B:LLR_B);

            // --------------------
            // Select survivor path
            // --------------------
            if (m0 <= m1) {
                Mk[S] = m0;  Sx = S0;
            }
            else {
                Mk[S] = m1;  Sx = S1;
            }
            // -----------------------------------
            // Copy the path memory shift register
            // -----------------------------------
            for (i=1; i<8; i++)
                Pk[S][i] = (Pk_1[Sx][i]);

            Pk[S][0] = Pk_1[Sx][0] | (S&1);  // Shift the input bit to the path memory
        }
        // ---------------------
        // Output a decision bit
        // ---------------------
        if (k>=tau)
        {
            Sx = 0;
            MinMetric = Mk[0];
            for (S=1; S<64; S++) 
            {
                if (Mk[S] < MinMetric) 
                {
                    MinMetric = Mk[S];
                    Sx = S;
                }
            }
            OutputData[k-tau] = (Pk[Sx][nSR_1] & outBit) ? 1 : 0;
        }
        // -------------------
        // Copy ?k[] to ?k_1[]
        // -------------------
        for (S=0; S<64; S++) 
        {
            Mk_1[S] = Mk[S];
            for (i=7; i>0; i--) {
                Pk_1[S][i] = (Pk[S][i] << 1) | (Pk[S][i-1] >> 31);
            }
            Pk_1[S][0] = Pk[S][0] << 1;
        }
    }
    // --------------------------------------------
    // Copy the survivor path into the output array
    // --------------------------------------------
    for ( ; k < (NumBits + tau); k++)
    {
        for (i=nSR_1; i>0; i--)
            Pk[0][i] = (Pk[0][i] << 1) | (Pk[0][i-1] >> 31);
        Pk[0][0] = Pk[0][0] << 1;

        OutputData[k-tau] = (Pk[0][nSR_1] & outBit) ? 1 : 0;
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_ViterbiDecoder()


// ================================================================================================
// FUNCTION  : Phy11n_RX_DFT
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the 64-point DFT using a maping where the output sequence corresponds to
//             ascending frequency; i.e., -32,...,-1,0,1,...,31.
// Parameters: x -- Input
//             Y -- Output
// ================================================================================================
wiStatus Phy11n_RX_DFT(wiComplex x[], wiComplex Y[])
{
    wiComplex *W = Phy11n_RxState()->DFT_W;
    wiComplex Z[64];
    int n, k;

    //  Compute the multiplier weights.
    //
    if (W[0].Real != 1.0)
    {
        for (k = 0; k < 32; k++)
        {
            W[k] = wiComplexExp(-2.0 * pi * (wiReal) k / 64);
            W[k+32] = wiComplexScaled(W[k], -1);
        }
    }
    //
    //  Implement the 64-point DFT
    //
    for (n = 0; n < 64; n++)
    {
        Z[n] = x[0];
        for (k = 1; k < 64; k++)
            Z[n] = wiComplexAdd( Z[n], wiComplexMultiply(x[k], W[(k*n)%64]) );
    }
    //
    // Permute Z into ascending frequency order in Y
    //
    for (k = 0; k < 64; k++)
    {
        n = (k + 32) % 64;
        Y[(k + 32) % 64] = Z[k];
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_DFT()


// ================================================================================================
// FUNCTION  : Phy11n_RX_InitialChanlEstimation()
// ------------------------------------------------------------------------------------------------
// Purpose   : Channel estimation based on HT-LTF
// ================================================================================================
wiStatus Phy11n_RX_InitialChanlEstimation(wiComplex **R, int k0, wiCMat H56[], wiCMat H52[])
{
    const int p[4][4] = { {1, -1, 1, 1}, {1, 1, -1, 1}, {1, 1, 1, -1}, {-1, 1, 1, 1} };

    const int L[] = { 
        1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1, 1, 1, 1, 1, 1, -1, -1, 1, 1, -1, 1, -1, 1, 1, 1, 1,
        1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1, -1, -1, -1, 1, 1, -1, -1, 1, -1, 1, -1, 1, 1, 1, 1, -1, -1
    };

    Phy11n_TxState_t *pTX = Phy11n_TxState();
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    wiComplex U[64], Y56[PHY11N_RXMAX][5][56];
    wiCMat P, Z;
    int iRx, iTx, k, n;

    if (pRX->N_HT_DLTF > 5) return STATUS(WI_ERROR_RANGE);

    //Initialize Matrix var.
    wiCMatInit(&P, pRX->N_HT_DLTF, pTX->N_TX     );
    wiCMatInit(&Z, pRX->N_RX,      pRX->N_HT_DLTF);

    //  Compute FFT for N_DLTF symbols and deal the 56 modulated subcarriers into
    //  array Y56.
    for (iRx = 0; iRx < pRX->N_RX; iRx++)
    {
        for (n = 0; n < pRX->N_HT_DLTF; n++)
        {
            Phy11n_RX_DFT(R[iRx] + 80*n + 16 + k0, U);

            for (k=0; k<56; k++)
                Y56[iRx][n][k] = U[SubcarrierMap56[k]];
        }
    }
    for (iTx = 0; iTx < pTX->N_TX; iTx++)
    {
        for (n = 0; n < pRX->N_HT_DLTF; n++)
        {
            P.val[n][iTx].Real = p[iTx][n];
            P.val[n][iTx].Imag = 0;
        }
    }
    for (k = 0; k < 56; k++)  // For each subcarrier, estimate channel
    { 
        for (n = 0; n < pRX->N_HT_DLTF; n++)
        {
            for (iRx = 0; iRx < pRX->N_RX; iRx++)
            {
                Z.val[iRx][n].Real = L[k] * Y56[iRx][n][k].Real / pRX->N_HT_DLTF;
                Z.val[iRx][n].Imag = L[k] * Y56[iRx][n][k].Imag / pRX->N_HT_DLTF;
            }
        }
        wiCMatMul (H56+k, &Z, &P);
        wiCMatCopy(H56+k, pRX->InitialH56+k);
    }
    for (k = 0; k < 52; k++)
        wiCMatCopy(H56 + DataSubcarrierMap56[k], H52 + k);

    wiCMatFree(&P);
    wiCMatFree(&Z);
    return WI_SUCCESS;
}
// end of Phy11n_RX_InitialChanlEstimation()

// ================================================================================================
// FUNCTION  : Phy11n_RX_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : set parameters
// Parameters: 
// ================================================================================================
wiStatus Phy11n_RX_Init(void)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();
    Phy11n_TxState_t *pTX = Phy11n_TxState();

    int i;

    if (!pRX->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( Phy11n_InitializeThread(wiProcess_ThreadIndex()) )
    }
    pRX->N_TX  = pTX->N_TX;
    pRX->N_SS  = pTX->N_SS;
    pRX->N_STS = pTX->N_STS;

    pRX->HT_LENGTH = pTX->HT_LENGTH;
    pRX->STBC      = pTX->STBC;

    pRX->N_SYM  = pTX->N_SYM;   // Number of OFDM symbols for DATA for each spatial stream
    pRX->N_DBPS = pTX->N_DBPS;  // Number of data bits per OFDM symbol (with all spatial streams)
    pRX->N_CBPS = pTX->N_CBPS;  // Number of coded bits per OFDM symbol (with all spatial streams)
    pRX->N_TBPS = pTX->N_TBPS;  // Number of total coded bits per subcarrier (with all spatial streams)
    for (i = 0; i < pTX->N_SS; i++)
    {
        pRX->N_CBPSS[i] = pTX->N_CBPSS[i];
        pRX->N_BPSC[i]  = pTX->N_BPSC[i];
    }
    pRX->N_ES = pTX->N_ES;      // Number of FEC encoders

    pRX->N_HT_DLTF = pTX->N_HT_DLTF;
    pRX->CodeRate12 = pTX->CodeRate12;

	// List Sphere Decoder
	if (!pRX->ListSphereDecoder.NCand) 	     pRX->ListSphereDecoder.NCand       = 128;
	if (!pRX->ListSphereDecoder.FixedRadius) pRX->ListSphereDecoder.FixedRadius = 100.0;

	// Single Tree Search Sphere Decoder
	pRX->SingleTreeSearchSphereDecoder.AvgCycles = 0.0;
	pRX->SingleTreeSearchSphereDecoder.AvgNodes  = 0.0;
	pRX->SingleTreeSearchSphereDecoder.AvgLeaves = 0.0;	
	if(!pRX->SingleTreeSearchSphereDecoder.LLRClip) pRX->SingleTreeSearchSphereDecoder.LMax = 1.0e6;	

    // [Barrett] See comment in Phy11n_TX_Init()
    //
    pTX->PilotShiftRegister = 0x7F;

    return WI_SUCCESS;
}
// end of Phy11n_RX_Init()

// ================================================================================================
// FUNCTION  : Phy11n_RX_FrameSync_CoarseCFO()
// ------------------------------------------------------------------------------------------------
// Purpose   : estime time offset and coarse CFO
// Parameters: R: received signal matrix
//             SyncDelay: usually set to 16, the lookafter delay for validate the correct peak
//             k0: the position where the FrameSync starts
//             time_offset: pointer to estimated offset
//             CFO: pointer to estimated coasre CFO
//             FrameSync_ctrl: 0: use zero offest, 1: use estimated offset
//             CFO_ctrl:  0: turn off CFO
//                        1: use samples where frame sync starts till the end of short preamble to estimate offset
//                        2: use only last three training symbols to estimate offset
// ================================================================================================
wiStatus Phy11n_RX_FrameSync_CoarseCFO(wiComplex *R[], wiInt SyncDelay, wiInt k0,
                                       wiInt * time_offset, wiReal * CFO, wiInt EnableFrameSync, wiInt CFO_ctrl)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    wiReal peak, rho[200];      // normalized correlation   
    wiReal sum_1, sum_2;        // sums of sequence power
    wiComplex sum;              // store sequence
    int i, k, k_max;

    if (InvalidRange(SyncDelay, 0, 39)) return WI_ERROR_PARAMETER2;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Frame/Symbol Synchronization
    //
    if (!EnableFrameSync)
    {
        k_max = 159;
    }
    else
    {
        if (pRX->RandomFrameSyncStart)
        {
            //  Generate a ranom offset between 16 and 111. This is intended to model the
            //  variable delay to start of frame sync due to carrier sense and AGC. The
            //  upper limit leave 3 L-STF symbols (required for reasonable performance).
            //
            k0 += (16 + (int)floor(96 * wiRnd_UniformSample()));
        }
        else k0 += 16; // otherwise, just skip 1 L-STF symbol for carrier sense

        sum_2 = 0.0;
        for (k = 0; k < 15; k++)
        {
            rho[k] = 0.0;
            for (i = 0; i < pRX->N_RX; i++)
                sum_2 += wiComplexNormSquared(R[i][k+k0]);
        }

        sum = wiComplexZero;
        sum_1 = 0.0;
        k_max = 0;
        peak = 0.0;
        for (k = 16; k < 160 + SyncDelay; k++)
        {
            for (i = 0; i < pRX->N_RX; i++)
            {
                sum = wiComplexAdd( sum, wiComplexMultiply( R[i][k+k0], wiComplexConj(R[i][k+k0-16]) ) );

                sum_1 += wiComplexNormSquared(R[i][k+k0-16]);
                sum_2 += wiComplexNormSquared(R[i][k+k0   ]);
            }
            rho[k] = wiComplexNormSquared(sum) / sum_1 / sum_2;
            if (rho[k] > peak)
            {
                peak = rho[k];
                k_max = k;
            }
            if (k - k_max >= SyncDelay)
                break;
        }
    }
    k_max += pRX->SyncShift;    // peak position shift(bias)
    *time_offset = k0 + k_max - 159;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Carrier Frequency Offset (CFO) Estimation
    //
    switch (CFO_ctrl)
    {
        case 0:                // turn off CFO
            *CFO = 0.0;
            break;

        case 1:                // start CFO from where the frame synchronization starts
            sum.Real = sum.Imag = 0.0;
            for (k = 16; k <= k_max; k++)
            {
                for (i = 0; i < pRX->N_RX; i++)
                {
                    sum.Real += R[i][k + k0].Real * R[i][k + k0 - 16].Real + R[i][k + k0].Imag * R[i][k + k0 - 16].Imag;
                    sum.Imag +=-R[i][k + k0].Real * R[i][k + k0 - 16].Imag + R[i][k + k0].Imag * R[i][k + k0 - 16].Real;
                }
            }
            *CFO = atan2(sum.Imag, sum.Real) / 16.0 / 2.0 / pi; // discrete frequency offset
            break;

        case 2:                // // use the last three training symbols for CFO
            sum.Real = sum.Imag = 0.0;
            for (k = k_max; k > k_max - 32; k--)
            {
                for (i = 0; i < pRX->N_RX; i++)
                {
                    sum.Real +=  R[i][k + k0].Real * R[i][k + k0 - 16].Real + R[i][k + k0].Imag * R[i][k + k0 - 16].Imag;
                    sum.Imag += -R[i][k + k0].Real * R[i][k + k0 - 16].Imag + R[i][k + k0].Imag * R[i][k + k0 - 16].Real;
                }
            }
            *CFO = atan2(sum.Imag, sum.Real) / 16.0 / 2.0 / pi; // discrete frequency offset
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of Phy11n_RX_FrameSync_CoarseCFO()

// ================================================================================================
// FUNCTION  : Phy11n_RX_PilotChannelTracking()
// ------------------------------------------------------------------------------------------------
// Purpose   : Update channel estimate for the pilot tones
// Parameters: nSym -- Symbol index
//             Y    -- Received signal
//             H    -- Channel matrix array (input and output)
// ================================================================================================
wiStatus Phy11n_RX_PilotChannelTracking(int nSym, wiComplex Y[][56], wiCMat H56[])
{
    const int PilotToneIndex[4] = { -21+28, -7+28, 7+27, 21+27 };
    const wiReal alpha = 0.10;

    Phy11n_RxState_t *pRX = Phy11n_RxState();
    wiReal pilot[4][4];  // pilot tone phase for the current symbol
    wiComplex Z[4], H0, **H;
    int iRx, iTx, n, j, k;

    XSTATUS( Phy11n_PilotTonePhase(pilot, pRX->N_TX, nSym % 4, PHY11N_SYMBOL_IS_HT_OFDM) );

    for (n=0; n<4; n++) // loop through 4 pilot tones
    {
        k = PilotToneIndex[n];
        H = H56[k].val;

        for (iRx = 0; iRx < pRX->N_RX; iRx++)
        { 
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                Z[iTx] = Y[iRx][k];             // start with the observation, and ...
                
                for (j = 0; j < pRX->N_TX; j++) // remove signal due to pilots from other antennas
                {
                    if (j != iTx) 
                    {
                        Z[iTx].Real -= pilot[j][n] * H[iRx][j].Real;
                        Z[iTx].Imag -= pilot[j][n] * H[iRx][j].Imag;
                    }
                }
            }
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                // Channel response estimate and update
                H0 = wiComplexScaled( Z[iTx], pilot[iTx][n] );

                H[iRx][iTx].Real = (alpha * H0.Real) + ((1 - alpha) * H[iRx][iTx].Real);
                H[iRx][iTx].Imag = (alpha * H0.Imag) + ((1 - alpha) * H[iRx][iTx].Imag);
            }
        }
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX_DataChannelTracking_LLR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Channel tracking based on soft information from the demapper
// ================================================================================================
wiStatus Phy11n_RX_DataChannelTracking_LLR(wiComplex Y[][56], Phy11nSymbolLLR_t LLR, wiCMat H56[])
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    wiBoolean UpdateChannelMatrix[4];
    int ConstellationSize[4];
    int k, j, i, iRx, iTx, kData = 0;

    wiReal Ps[4][64];   // Ps[iTx][i] = Pr{s[i]} for iTx
    wiComplex S[64];    // Constellation symbol
    wiComplex sBar[4];  // Sum(s[i]*Pr{s[i]})
    wiComplex Z[4];
    wiComplex H0, Hi, q;
    wiComplex **H;

    wiReal alpha = (pRX->ChanlTracking.Type == 2) ? 0.1 : 0.15;

    for (iTx = 0; iTx < pRX->N_TX; iTx++) {
        ConstellationSize[iTx] = 1 << pRX->N_BPSC[iTx]; // 2^N_BPSC
    }

    for (k = 0; k < NumTotalSubcarriers; k++)
    { 
        H = H56[k].val;
        if (Phy11nSubcarrierIsPilotTone(k)) continue; // skip pilot tones

        // Compute symbol posteriori probability. 
        // QAM symbols in S, sBar = sum(s[i]*Pr{S[i]})
        //
        for (iTx = 0; iTx < pRX->N_TX; iTx++)
        {
            int ofs = kData * pRX->N_BPSC[iTx];
            XSTATUS( Phy11n_RX_PosterioriProb(LLR[iTx] + ofs, pRX->N_BPSC[iTx], Ps[iTx], S, &sBar[iTx]) );
            
            UpdateChannelMatrix[iTx] = WI_FALSE;
            for (i = 0; i < ConstellationSize[iTx]; i++)
                if (Ps[iTx][i] > pRX->ChanlTracking.SoftProbTh)
                    UpdateChannelMatrix[iTx] = WI_TRUE;        //  Update channel if ANY P_si > CHTR_Soft_ProbTH
        }

        for (iRx = 0; iRx < pRX->N_RX; iRx++)
        {
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                if (UpdateChannelMatrix[iTx])
                {
                    Z[iTx] = Y[iRx][k];
                    for (j = 0; j < pRX->N_TX; j++)
                    {
                        if (j != iTx)  {
                            Hi = wiComplexMultiply(H[iRx][j], sBar[j]);
                            Z[iTx].Real -= Hi.Real;
                            Z[iTx].Imag -= Hi.Imag;
                        }
                    }
                }
            }
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                if (UpdateChannelMatrix[iTx])
                {
                    H0 = wiComplexZero;
                
                    for (i = 0; i < ConstellationSize[iTx]; i++)
                    {
                        q = wiComplexDiv(Z[iTx], S[i]);

                        H0.Real += (Ps[iTx][i] * q.Real);
                        H0.Imag += (Ps[iTx][i] * q.Imag);
                    }
                    H[iRx][iTx].Real = (alpha * H0.Real) + ((1 - alpha) * H[iRx][iTx].Real);
                    H[iRx][iTx].Imag = (alpha * H0.Imag) + ((1 - alpha) * H[iRx][iTx].Imag);
                }
            }
        }
        kData++; // increment the data subcarrier count
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX_DataChannelTracking_Genie()
// ------------------------------------------------------------------------------------------------
// Purpose   : Genie-aided data channel tracking using data from the TX state
// ================================================================================================
wiStatus Phy11n_RX_DataChannelTracking_Genie(wiComplex Y[][56], wiCMat H56[], wiInt nSym)
{
    Phy11n_TxState_t *pTX = Phy11n_TxState();
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    const wiReal alpha = 0.10;

    wiInt k, j, iRx, iTx, kData = 0;
    wiComplex Z[4], Hi, H0;
    wiComplex **H;

    for (k = 0; k < NumTotalSubcarriers; k++)
    { 
        H = H56[k].val;
        if (Phy11nSubcarrierIsPilotTone(k)) continue; // skip pilot tones

        for (iRx = 0; iRx < pRX->N_RX; iRx++)
        {
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                Z[iTx] = Y[iRx][k];
                for (j = 0; j < pRX->N_TX; j++)
                {
                    if (j != iTx)  {
                        Hi = wiComplexMultiply(H[iRx][j], pTX->stmp[j][52*nSym + kData]);
                        Z[iTx].Real -= Hi.Real;
                        Z[iTx].Imag -= Hi.Imag;
                    }
                }
            }
            for (iTx = 0; iTx < pRX->N_TX; iTx++)
            {
                H0 = wiComplexDiv(Z[iTx],pTX->stmp[iTx][52*nSym + kData]); 

                H[iRx][iTx].Real = (alpha * H0.Real) + ((1 - alpha) * H[iRx][iTx].Real);
                H[iRx][iTx].Imag = (alpha * H0.Imag) + ((1 - alpha) * H[iRx][iTx].Imag);
            }
        }
        kData++; // increment the data subcarrier count
    }
    return WI_SUCCESS;
}


// ================================================================================================
// FUNCTION  : PPhy11n_RX_PosterioriProb()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_PosterioriProb(wiReal LLR[], wiInt N_BPSC, wiReal Ps[], wiComplex S[], wiComplex *sBar)
{
    int ConstellationSize = 1 << N_BPSC; // 2^N_BPSC
    wiReal Pb[2][8], eLLR;
    int b[8], i, j;

    if (InvalidRange(N_BPSC, 1, 8)) return WI_ERROR_PARAMETER2;

    for (i = 0; i < N_BPSC; i++) // Convert LLR to probabilities for each bit
    {                       
             if (LLR[i] >  10)  eLLR = 1.0E+6;
        else if (LLR[i] < -10)  eLLR = 0;
        else                    eLLR = exp(LLR[i]);

        Pb[0][i] =    1 / (1 + eLLR);
        Pb[1][i] = eLLR / (1 + eLLR);
    }

    for (i = 0; i < ConstellationSize; i++)
    {
        Ps[i] = 1;
        for (j = 0; j < N_BPSC; j++)
        {
            b[j]   = (i >> j) & 1;           // bit vector corresponding to constellation point i
            Ps[i] *= Pb[b[j]][j];            // Ps[i] = Prod{Pb}
        }
        Phy11n_TX_QAM(S+i, b, N_BPSC, 1, 0); // generate constellation point S[i]
    }

    if (sBar)
    {
        sBar->Real = sBar->Imag = 0;

        for (i = 0; i < ConstellationSize; i++)
        {
            sBar->Real += S[i].Real * Ps[i];     // sBar = Sum{Si * Pr{Si}}
            sBar->Imag += S[i].Imag * Ps[i];
        }
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX_HardDemapLLR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Convert LLRs from soft demapping to a constellation point
// Parameters: LLR        -- demapper soft information output
//             N_BPSC     -- bits per constellation point {1, 2, 4, 6, 8}
//             s          -- constellation point
//             PsThreshold-- threshold to qualify signal as strong probability
//             StrongProb -- set if Px|LLR > 0.7
// ================================================================================================
wiStatus Phy11n_RX_HardDemapLLR(wiReal LLR[], wiInt N_BPSC, wiComplex *s, wiReal PsThreshold, wiBoolean *StrongProb)
{
    int ConstellationSize = 1 << N_BPSC;

    wiReal Pb[2][8];
    wiReal eLLR, Ps, PsMax = -1;
    int b[8], i, j;

    if (InvalidRange(N_BPSC, 1, 8)) return STATUS(WI_ERROR);

    for (i = 0; i < N_BPSC; i++)
    {                           // for loop for subcarriers
             if (LLR[i] >  10) eLLR = 1.0E+6;
        else if (LLR[i] < -10) eLLR = 0;
        else                   eLLR = exp(LLR[i]);

        Pb[0][i] =    1 / (1 + eLLR);
        Pb[1][i] = eLLR / (1 + eLLR);
    }

    for (i = 0; i < ConstellationSize; i++)
    {
        Ps = 1;
        for (j = 0; j < N_BPSC; j++)
        {
            b[j] = (i >> j) & 1;
            Ps *= Pb[b[j]][j];
        }
        if (Ps > PsMax)
        {
            Phy11n_TX_QAM(s, b, N_BPSC, 1, 0);
            PsMax = Ps;
        }
    }
    if (StrongProb)
        *StrongProb = (PsMax > PsThreshold) ? WI_TRUE : WI_FALSE;

    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX_PhaseCorrection()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Phy11n_RX_PhaseCorrection(wiReal PhaseError, wiComplex Y56[4][56], wiCMat Y52)
{
    if (PhaseError != 0.0)
    {
        int iRx, k;
        wiComplex w = wiComplexExp(-PhaseError);

        for (iRx = 0; iRx < Phy11n_RxState()->N_RX; iRx++)
        {
            for (k = 0; k < 56; k++)
                Y56[iRx][k] = wiComplexMultiply(Y56[iRx][k], w);

            for (k = 0; k < NumDataSubcarriers; k++)
                Y52.val[iRx][k] = Y56[iRx][DataSubcarrierMap56[k]];
        }
    }
    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX_PhaseErrorDetector()
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate carrier phase rotation of the symbol
// Parameters: CarrierPhase -- Estimated carrier phase
//             LLR          -- Demapper soft information output
//             Y            -- Observations input to the detector
//             H56          -- Array of channel response matrices
//             nSym         -- Index for the current symbol
//             Method       -- Method to obtain transmitted constellation point
// ================================================================================================
wiStatus Phy11n_RX_PhaseErrorDetector(wiReal *CarrierPhase, Phy11nSymbolLLR_t LLR, wiComplex Y[4][56], 
                                      wiCMat * H56, wiInt nSym, int Method)
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    int N_TX = pRX->N_TX; // notational convenience
    int k, iRx, iTx, kData = 0, kPilot = 0;

    wiReal SumH2 = 0; // sum of |H|^2
    wiReal SumWc = 0; // sum of weighted carrier phase angles

    wiReal pilot[4][4];
    wiComplex X[52][4];             // demapper constellation points
    wiBoolean HighProbPoint[52][4]; // demapper quality flag

    if (Method == PHY11N_PHASETRACK_NONE) // No phase tracking
    {
        *CarrierPhase = 0;
        return WI_SUCCESS;
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Estimate transmitted constellation point based on selected method
    //
    for (iTx = 0; iTx < N_TX; iTx++)
    {
        int N_BPSC = pRX->N_BPSC[iTx];

        for (k = 0; k < NumDataSubcarriers; k++)
        {
            switch (Method)
            {
                case PHY11N_PHASETRACK_DEMAPPER:

                    Phy11n_RX_HardDemapLLR(LLR[iTx] + k*N_BPSC, N_BPSC, X[k]+iTx, 0.0, HighProbPoint[k]+iTx);
                    HighProbPoint[k][iTx] = 1;
                    break;

                case PHY11N_PHASETRACK_GENIE:

                    X[k][iTx] = Phy11n_TxState()->stmp[iTx][52*nSym + k];
                    HighProbPoint[k][iTx] = 1;
                    break;

                case PHY11N_PHASETRACK_DEMAPPER_REJECTION:

                    Phy11n_RX_HardDemapLLR(LLR[iTx] + k*N_BPSC, N_BPSC, X[k]+iTx, 0.7, HighProbPoint[k]+iTx);
                    break;

                default: return STATUS(WI_ERROR_UNDEFINED_CASE);
            }
        }
    }
    XSTATUS(Phy11n_PilotTonePhase(pilot, N_TX, nSym % 4, PHY11N_SYMBOL_IS_HT_OFDM));

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Phase Error Detection
    //
    for (k = 0; k < NumTotalSubcarriers; k++) 
    {
        wiComplex Z;
        wiComplex **H = H56[k].val;
        wiComplex r = {0, 0};
        wiReal   H2 = 0;

        if (Phy11nSubcarrierIsDataTone(k))
        {
            wiBoolean UseSubcarrier = WI_TRUE;

            for (iTx = 0; iTx < N_TX; iTx++) {
                if (!HighProbPoint[kData][iTx]) UseSubcarrier = WI_FALSE;
            }
            if (UseSubcarrier)
            {
                for (iRx = 0; iRx < pRX->N_RX; iRx++)
                {
                    wiComplex S = {0,0};
    
                    for (iTx = 0; iTx < N_TX; iTx++)
                    {
                        Z = wiComplexMultiply(X[kData][iTx], H[iRx][iTx]);
                        S = wiComplexAdd(S, Z);
                        H2 += wiComplexNormSquared(H[iRx][iTx]);
                    }
                    S.Imag = -S.Imag; // S = S*
                    Z = wiComplexMultiply(S, Y[iRx][k]);
                    r = wiComplexAdd(r, Z);
                }
            }
            kData++;
        }
        else // pilot tone processing...same as above, but the "pilot" signal is real, not complex
        {
            for (iRx = 0; iRx < pRX->N_RX; iRx++)
            {
                wiComplex S = {0, 0};
                for (iTx = 0; iTx < N_TX; iTx++)
                {
                    S.Real += H[iRx][iTx].Real * pilot[iTx][kPilot];
                    S.Imag -= H[iRx][iTx].Imag * pilot[iTx][kPilot];

                    H2 += wiComplexNormSquared(H[iRx][iTx]);
                }
                r = wiComplexAdd( r, wiComplexMul(S, Y[iRx][k]) );
            }
            kPilot++;
        }
        if (r.Real != 0)
        {
            SumH2 += H2;
            SumWc += H2 * (r.Imag / r.Real);
        }
    }
    *CarrierPhase = SumWc / SumH2;

    return WI_SUCCESS;
}

// ================================================================================================
// FUNCTION  : Phy11n_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus Phy11n_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[])
{
    Phy11n_RxState_t *pRX = Phy11n_RxState();

    if (!R)    return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);

    // Count the number of receive sample arrays
    for (pRX->N_RX = 0; pRX->N_RX <= 4; pRX->N_RX++)
        if (R[pRX->N_RX] == NULL) break;

    if ( (!R[1] && pRX->N_RX > 1) 
      || (!R[2] && pRX->N_RX > 2) 
      || (!R[3] && pRX->N_RX > 3) ) return STATUS(WI_ERROR_PARAMETER4);

    XSTATUS( Phy11n_RX_Init() );
    XSTATUS( Phy11n_RX_AllocateMemory() );

    // Time domain processing
    //
    XSTATUS( Phy11n_RX_DigitalFrontEnd(R, Nr) );
    XSTATUS( Phy11n_RX_EstimateNoise() );
    XSTATUS( Phy11n_RX_InitialChanlEstimation(pRX->y, 640, pRX->H56, pRX->H52) );
    XSTATUS( Phy11n_RX_ComputeDataSymbolDFTs() );

    // Frequency-domain processing
    //
    switch (pRX->IterationMethod)
    {
        case 0:  XSTATUS( Phy11n_RX_Frame () );  break;
        case 1:  XSTATUS( Phy11n_RX_Symbol() );  break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    XSTATUS( Phy11n_RX_Descrambler(pRX->Nb, pRX->b, pRX->a) ); 
    XSTATUS( Phy11n_RX_SerialToParallelData() );
    
    // Output data
    if (Length) *Length = pRX->HT_LENGTH;
    if (data)   *data   = pRX->PSDU.ptr;

    return WI_SUCCESS;
}
// end of Phy11n_RX()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

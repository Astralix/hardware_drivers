//--< wiChanl.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Channel Model(s)
//  Copyright 2001-2003 Bermai, 2005-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <math.h>
#include "wiChanl.h"
#include "wiFilter.h"
#include "wiMath.h"
#include "wiParse.h"
#include "wiRnd.h"
#include "wiUtil.h"
#include "wiTGnChanl.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiChanl_LookupTable_t *pLookupTable = NULL;
static wiChanl_State_t       *LocalState[WISE_MAX_THREADS] = {NULL};

static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  MODULE FUNCTION PROTOTYPES
//=================================================================================================
wiStatus wiChanl_PowerAmp(int NumTx, int Nx, wiComplex *X[], wiComplex *Y[], wiReal Prms);
wiStatus wiChanl_RappPA  (int Nx, wiComplex x[], wiComplex *y, wiReal Prms, wiReal p, wiReal OBOdB, wiReal Ap);
wiStatus wiChanl_KeNingPA(int Nx, wiComplex x[], wiComplex *y, wiReal Anorm, wiReal Amax, 
                          wiReal g1, wiReal g3, wiReal g5);
wiStatus wiChanl_Resample(int Ny, wiComplex y[], int iRx);
wiStatus wiChanl_MultipathCore(int NumTx, int Nx, wiComplex *X[], int Ns, wiComplex *s, int iRx, int t0);
wiStatus wiChanl_GetPositionOffset(wiChanl_State_t *pState);
wiStatus wiChanl_ParseLine(wiString text);

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(Status) WICHK((Status))
#define XSTATUS(Status) { if (STATUS(Status)<0) return WI_ERROR_RETURNED; }
#define PSTATUS(arg)      if ((pstatus=WICHK(arg))<=0) return pstatus;

#ifndef min
#define min(a,b) ( ((a)<(b)) ? (a):(b) )
#define max(a,b) ( ((a)>(b)) ? (a):(b) )
#endif

// ================================================================================================
// FUNCTION  : wiChanl_Init
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_Init(void)
{
    int i, k;
    wiChanl_State_t *pState;

    if (LocalState[0]) return WI_ERROR_MODULE_ALREADY_INIT;

    WICALLOC( pLookupTable,  1, wiChanl_LookupTable_t );
    WICALLOC( LocalState[0], 1, wiChanl_State_t );

    pState = LocalState[0];

    // ----------------
    // Default Settings
    // ----------------
    pState->Model=   1;
    pState->AntennaBalance = 0;
    pState->Fs   =  80.0e+6;
    pState->Trms =  50.0E-9;
    pState->Nrms =   0.0;
    pState->Nc   =   0;
    pState->NumRx=   2;
    pState->fDoppler = 0;
    pState->PowerAmp.Enabled = 0;
    pState->PowerAmp.Model   = 2;
    pState->PowerAmp.m       = 3;
    pState->PowerAmp.Amax    = 1.0;
    pState->PowerAmp.Anorm   = 0.0;
    pState->PowerAmp.g1      = 1.0;
    pState->PowerAmp.g3      = 0.0;
    pState->PowerAmp.g5      =-0.1;
    pState->PowerAmp.ApRapp  = 1.0;
    pState->IQAngleErrorDeg  = 0.0;
    pState->IQGainRatio      = 1.0;
    pState->AM.Enabled = 0;
    pState->AM.Ramp.A = 1.0;
    pState->Radar.Model = 0;
    pState->Radar.N = 15;
    pState->Radar.W = 1;
    pState->Radar.PRF = 750;
    pState->Radar.PrdBm = -50;
    pState->Radar.f_MHz = 5;
    pState->Radar.fRangeMHz = 5.0;
    pState->Radar.k0 = 200;
    pState->Interference.pRapp = 3;
    pState->Interference.OBOdB = 3;
    pState->Interference.ApRapp= 1;

    XSTATUS(wiParse_AddMethod(wiChanl_ParseLine));

    // ---------------------------
    // Compute the DFT multipliers
    // ---------------------------
    for (k=0; k<256; k++)
    {
        double a = 2.0 * pi * (double)k / 256.0;
        pLookupTable->IDFT_W_256[k].Real = cos(a) / 256.0;
        pLookupTable->IDFT_W_256[k].Imag = sin(a) / 256.0;
    }

    // --------------------------------------------------------------
    // Compute the interpolator filters: Hanning windowed sinc filter
    // --------------------------------------------------------------
    {
        int k0 = (WICHANL_INTERPOLATOR_TAPS -1)/2;
        int i0 = (WICHANL_INTERPOLATOR_STEPS-1)/2;
        double x, dt;

        for (i=0; i<WICHANL_INTERPOLATOR_STEPS; i++)
        {
            dt = (double)(i-i0) / (WICHANL_INTERPOLATOR_STEPS-1);
            for (k=0; k<WICHANL_INTERPOLATOR_TAPS; k++) 
            {
                x = pi*((k-k0) + dt);
                pLookupTable->wInterpolate[i][k] = x ? (sin(x)/x * 0.5 * (1 - cos(2*pi*k/WICHANL_INTERPOLATOR_TAPS))) : 1.0;
            }
        }
    }
    // ---------------------------
    // Initialize Included Methods
    // ---------------------------
    #ifdef __WITGNCHANL_H
        XSTATUS( wiTGnChanl_Init() );
    #endif

    return WI_SUCCESS;
}
// end of wiChanl_Init()

// ================================================================================================
// FUNCTION  : wiChanl_InitializeThread
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_InitializeThread(void)
{
    int i, j;

    wiChanl_State_t *pState = (wiChanl_State_t *)wiCalloc(1, sizeof(wiChanl_State_t));
    if (!pState) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    LocalState[wiProcess_ThreadIndex()] = pState;

    memcpy(pState, LocalState[0], sizeof(wiChanl_State_t));

    for (i=0; i<WICHANL_RXMAX; i++)
        for (j=0; j<WICHANL_TXMAX; j++)  
            pState->c[i][j] = NULL;

    for (i=0; i< WICHANL_TXMAX; i++) pState->PowerAmp.y[i].ptr = NULL;
    for (i=0; i<=WICHANL_RXMAX; i++) pState->R[i].ptr = NULL;

    pState->PhaseNoise.a.ptr = NULL;
    pState->PhaseNoise.u.ptr = NULL;
    pState->SampleJitter.dt.ptr = NULL;
    pState->SampleJitter.v.ptr = NULL;
    pState->SampleJitter.z.ptr = NULL;

    for (i=0; i<WICHANL_TXMAX; i++) {
        pState->LoadPacket.y[i].ptr = NULL;
    }
    pState->LoadPacket.r.ptr = NULL;

    for (i=0; i<WICHANL_MAXPACKETS; i++)
        for (j=0; j<WICHANL_RXMAX; j++)
            pState->Interference.Packet[i].s[j].ptr = NULL;

    pState->Interference.v.ptr = NULL;

    return WI_SUCCESS;
}
// end of wiChanl_InitializeThread()

// ================================================================================================
// FUNCTION  : wiChanl_CloseAllThreads
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_CloseAllThreads(int FirstThread)
{
    int i, j, n;

    if (!LocalState[0]) return WI_ERROR_MODULE_NOT_INITIALIZED;

    #ifdef __WITGNCHANL_H
        XSTATUS( wiTGnChanl_CloseAllThreads(FirstThread) );
    #endif

    for (n = FirstThread; n < WISE_MAX_THREADS; n++)
    {
        wiChanl_State_t *pState = LocalState[n];
        if (!pState) continue;

        for (i=0; i<WICHANL_RXMAX; i++) {
            for (j=0; j<WICHANL_TXMAX; j++) {
                WIFREE( pState->c[i][j] );
            }
        }
        for (i=0; i<WICHANL_RXMAX; i++) WIFREE_ARRAY( pState->R[i] );
        for (i=0; i<WICHANL_RXMAX; i++) WIFREE_ARRAY( pState->RxCoupling.s[i] );
        for (i=0; i<WICHANL_TXMAX; i++) WIFREE_ARRAY( pState->PowerAmp.y[i] );

		WIFREE_ARRAY( pState->PhaseNoise.u );
        WIFREE_ARRAY( pState->PhaseNoise.a   );

        WIFREE_ARRAY( pState->SampleJitter.dt);
        WIFREE_ARRAY( pState->SampleJitter.v );
		WIFREE_ARRAY( pState->SampleJitter.z );

        for (i=0; i<WICHANL_MAXPACKETS; i++) {
            for (j=0; j<WICHANL_RXMAX; j++) {
                WIFREE_ARRAY( pState->Interference.Packet[i].s[j] );
            }
        }
        WIFREE_ARRAY( pState->Interference.v );
        WIFREE_ARRAY( pState->Radar.MissingPulse );

        WIFREE_ARRAY( pState->LoadPacket.r );
        for (i=0; i<WICHANL_TXMAX; i++) {
            WIFREE_ARRAY( pState->LoadPacket.y[i] );
        }
        WIFREE_ARRAY( pState->Model_1.n );
        WIFREE( LocalState[n] );

    }
    return WI_SUCCESS;
}
// end of wiChanl_CloseAllThreads()

// ================================================================================================
// FUNCTION  : wiChanl_Close
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_Close(void)
{
    if (!LocalState[0]) return WI_ERROR_MODULE_NOT_INITIALIZED;

    #ifdef __WITGNCHANL_H
        XSTATUS( wiTGnChanl_Close() );  // Close included TGn channel model
    #endif

    XSTATUS( wiChanl_CloseAllThreads(0) );
    WIFREE( pLookupTable );

    XSTATUS( wiParse_RemoveMethod(wiChanl_ParseLine) );

    return WI_SUCCESS;
}
// end of wiChanl_Close()

// ================================================================================================
// FUNCTION  : wiChanl_LookupTable
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiChanl_LookupTable_t * wiChanl_LookupTable(void)
{
    return pLookupTable;
}
// end of wiChanl_LookupTable()

// ================================================================================================
// FUNCTION  : wiChanl_State
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiChanl_State_t * wiChanl_State(void)
{
    return LocalState[wiProcess_ThreadIndex()];
}
// end of wiChanl_State()

// ================================================================================================
// FUNCTION  : wiChanl_IQMismatch
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a baseband-equivalent I/Q mismatch in the received signal array
// ================================================================================================
wiStatus wiChanl_IQMismatch(wiChanl_State_t *pState)
{
    if (pState->IQAngleErrorDeg || (pState->IQGainRatio != 1.0))
    {
        int i, k;
        double Ci = pState->IQGainRatio * cos(pi/180.0 * pState->IQAngleErrorDeg);
        double Cq = pState->IQGainRatio * sin(pi/180.0 * pState->IQAngleErrorDeg);

        for (i=0; i<pState->NumRx; i++) 
        {
            wiComplex *r = pState->R[i].ptr;

            for (k=0; k<pState->Nr; k++) 
            {
                r[k].Imag = Ci * r[k].Imag + Cq * r[k].Real; 
            }
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_IQMismatch()

// ================================================================================================
// FUNCTION  : wiChanl_GainStep
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a gain step in the received signal array
// Parameters: none
// ================================================================================================
wiStatus wiChanl_GainStep(wiChanl_State_t *pState)
{
    if (pState->GainStep.Enabled)
    {
        int i, k;

        for (i=0; i<pState->NumRx; i++) 
        {
            double A = pow(10.0, pState->GainStep.dB[i]/20.0);
            wiComplex *r = pState->R[i].ptr;

            for (k=pState->GainStep.k0; k<pState->Nr; k++) 
            {
                r[k].Real *= A;
                r[k].Imag *= A;
            }
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_GainStep()

// ================================================================================================
// FUNCTION  : wiChanl_RxCoupling
// ------------------------------------------------------------------------------------------------
// Purpose   : Produce a linear combination of receive signals to model coupling at the 
//             receiver RF input.
// Parameters: none
// ================================================================================================
wiStatus wiChanl_RxCoupling(wiChanl_State_t *pState)
{
    if (pState->RxCoupling_dB && (pState->NumRx > 1))
    {
        int i, k;
        wiComplex *s[WICHANL_RXMAX];
        wiComplex *r[WICHANL_RXMAX];

        // [Barrett-110214] Assuming the input signals are uncorrelated, the math below
        // slightly increase the overall receive power. Consider a = sqrt(1 - (NumRx-1)b^2)
        //
        double b = pow(10.0, pState->RxCoupling_dB/20.0); // coupled term
        double a = 1.0; 

        for (i=0; i<pState->NumRx; i++)
        {
            GET_WIARRAY( s[i], pState->RxCoupling.s[i], pState->Nr, wiComplex );
            r[i] = pState->R[i].ptr;
        }
        switch (pState->NumRx)
        {
            case 2:
                for (k=0; k<pState->Nr; k++) 
                {
                    s[0][k].Real = a*r[0][k].Real + b*r[1][k].Real;
                    s[0][k].Imag = a*r[0][k].Imag + b*r[1][k].Imag;

                    s[1][k].Real = a*r[1][k].Real + b*r[0][k].Real;
                    s[1][k].Imag = a*r[1][k].Imag + b*r[0][k].Imag;
                }
                break;

            case 3:
                for (k=0; k<pState->Nr; k++) 
                {
                    s[0][k].Real = a*r[0][k].Real + b*(r[1][k].Real + r[2][k].Real);
                    s[0][k].Imag = a*r[0][k].Imag + b*(r[1][k].Imag + r[2][k].Imag);

                    s[1][k].Real = a*r[1][k].Real + b*(r[0][k].Real + r[2][k].Real);
                    s[1][k].Imag = a*r[1][k].Imag + b*(r[0][k].Imag + r[2][k].Imag);

                    s[2][k].Real = a*r[2][k].Real + b*(r[0][k].Real + r[1][k].Real);
                    s[2][k].Imag = a*r[2][k].Imag + b*(r[0][k].Imag + r[1][k].Imag);
                }
                break;

            case 4:
                for (k=0; k<pState->Nr; k++) 
                {
                    s[0][k].Real = a*r[0][k].Real + b*(r[1][k].Real + r[2][k].Real + r[3][k].Real);
                    s[0][k].Imag = a*r[0][k].Imag + b*(r[1][k].Imag + r[2][k].Imag + r[3][k].Imag);

                    s[1][k].Real = a*r[1][k].Real + b*(r[0][k].Real + r[2][k].Real + r[3][k].Real);
                    s[1][k].Imag = a*r[1][k].Imag + b*(r[0][k].Imag + r[2][k].Imag + r[3][k].Imag);

                    s[2][k].Real = a*r[2][k].Real + b*(r[0][k].Real + r[1][k].Real + r[3][k].Real);
                    s[2][k].Imag = a*r[2][k].Imag + b*(r[0][k].Imag + r[1][k].Imag + r[3][k].Imag);

                    s[3][k].Real = a*r[3][k].Real + b*(r[0][k].Real + r[1][k].Real + r[2][k].Real);
                    s[3][k].Imag = a*r[3][k].Imag + b*(r[0][k].Imag + r[1][k].Imag + r[2][k].Imag);
                }
                break;

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        // Exchange r and s arrays
        //
        for (i=0; i<pState->NumRx; i++)
        {
            wiComplexArray_t tempArray = pState->RxCoupling.s[i];
            pState->RxCoupling.s[i] = pState->R[i];
            pState->R[i] = tempArray;
            pState->r[i] = pState->R[i].ptr;
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_RxCoupling()

// ================================================================================================
// FUNCTION  : wiChanl_LoadPacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate and store a receive packet for later use by the interference models
// Parameters: Nx      -- Number of samples in x
//             x       -- Input array           
//             Fs      -- Sample rate (Hz)
//             Prms    -- Input signal power (complex, rms)
//             iPacket -- Packet index (-1 inserts packet at 0 and advances 0 to 1)
//             PrdBm   -- Receive power level (dBm)
//             f_MHz   -- Frequency offset (MHz)
//             t0      -- Time offset (s) if applicable
// ================================================================================================
wiStatus wiChanl_LoadPacket( int Nx, wiComplex *X[], double Fs, double Prms, int iPacket,
                             double PrdBm, double f_MHz, double t0 )
{
    wiChanl_State_t *pState = wiChanl_State();

    int iTx, iRx, NumTx, Ns, k;
    wiComplex c;

    int PositionOffset = (pState->Channel == 2) ? 0 : pState->PositionOffset.t0;

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!pState)                     return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (InvalidRange(Nx, 1, 1<<28))  return WI_ERROR_PARAMETER1;
    if (X == NULL)                   return WI_ERROR_PARAMETER2;
    if (!Fs)                         return WI_ERROR_PARAMETER3;
    if (!Prms)                       return WI_ERROR_PARAMETER4;
    if (InvalidRange(iPacket,-1,WICHANL_MAXPACKETS-1)) return WI_ERROR_PARAMETER5;
    if (InvalidRange(PrdBm, -200, 60.0)) return WI_ERROR_PARAMETER6;
	
    // ----------------------------------------
    // Determine the number of transmit streams
    // ----------------------------------------
    for (NumTx=0; (NumTx<=WICHANL_TXMAX && X[NumTx]); NumTx++) ;
    if (NumTx > WICHANL_TXMAX) return STATUS(WI_ERROR_RANGE);

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Signal Length
    //
    //  This is just the input signal length increase by 100 to account for the channel filter
    //  group delay. 100 is somewhat arbitrary but matches the delay that was used with
    //  previous code for Mojave verification. It is kept the same to yield consistent results.
    // 
    Ns = Nx + 100;
   
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Packet Shift for Interference Models
    //
    //  The current packet is stored for interference models using the previous transmission.
    //  This case is designated by wiChanl_Model using iPacket == -1. The packets are then
    //  shifted from 0 to 1; i.e., if there is a packet at Packet[0] it is moved to Packet[1].
    //  Then, the current packet is stored into Packet[0].
    //
    //  The interference power and frequency offset are extracted from the pState->Interference
    //  parameters with the same name. This is used to maintain compatibility with the 
    //  interference models in WiSE 2.x
    //
    if (iPacket == -1)
    {
        for (iRx=0; iRx<pState->NumRx; iRx++)
        {
            wiComplexArray_t temp = pState->Interference.Packet[1].s[iRx];
            pState->Interference.Packet[1].s[iRx] = pState->Interference.Packet[0].s[iRx];
            pState->Interference.Packet[0].s[iRx] = temp;
        }
        iPacket = 0;

        pState->Interference.Packet[0].PrdBm = pState->Interference.PrdBm;
        pState->Interference.Packet[0].f_MHz = pState->Interference.f_MHz; 
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Record Model Parameters
    //
    pState->Interference.Packet[iPacket].PrdBm = PrdBm;
    pState->Interference.Packet[iPacket].f_MHz = f_MHz;
    pState->Interference.Packet[iPacket].t0    = t0;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate the Channel Response(s)
    //
    XSTATUS( wiChanl_GenerateChannelResponse(NumTx, pState->NumRx) );

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate the Receive Signal and Store
    //
    for (iRx=0; iRx<pState->NumRx; iRx++)
    {
        double A = pow(10.0, (pState->Interference.Packet[iPacket].PrdBm-30)/20) / sqrt(Prms);
        double w = 2.0 * pi * pState->Interference.Packet[iPacket].f_MHz / (Fs/1.0E+6);
        wiComplex *r, *s;

        GET_WIARRAY( r, pState->LoadPacket.r, Ns, wiComplex );
        GET_WIARRAY( s, pState->Interference.Packet[iPacket].s[iRx], Ns, wiComplex );
        memset( r, 0, Ns*sizeof(wiComplex) );

        switch (pState->Interference.Model)
        {
            // only the nonlinear last packet case is special, all others are handled
            // by the default case
            //
            case WICHANL_INTERFERENCE_NONLINEAR_LAST_PACKET: /////////////////////////////////
            {
                wiComplex *Y[WICHANL_TXMAX]; // temp: RappPA output
                
                for (iTx=0; iTx<NumTx; iTx++)
                {
                    GET_WIARRAY( Y[iTx], pState->LoadPacket.y[iTx], Ns, wiComplex );
    
                    XSTATUS( wiChanl_RappPA(Nx, X[iTx], Y[iTx], Prms, pState->Interference.pRapp, 
                                            pState->Interference.OBOdB, pState->Interference.ApRapp) );

                    if (pState->Interference.Pn_dBm_MHz)
                    {
                        int k1 =     (int)pState->Interference.nk1;
                        int k2 = min((int)pState->Interference.nk2, Ns-1);

                        wiReal std = pow(10.0, (pState->Interference.Pn_dBm_MHz-30)/20) * sqrt(pState->Fs/1e6) / A;
                        XSTATUS( wiRnd_ComplexNormal(Ns, r, std) );

                        for (k=k1; k<=k2; k++) 
                        {
                            Y[iTx][k].Real += r[k].Real;
                            Y[iTx][k].Imag += r[k].Imag;
                        }
                        memset( r, 0, Ns*sizeof(wiComplex) ); // clear because same space is used below
                    }
                }
                XSTATUS( wiChanl_MultipathCore(NumTx, Nx, Y, Ns, r, iRx, PositionOffset) );
                break;
            }
            default: /////////////////////////////////////////////////////////////////////////
            {
                XSTATUS( wiChanl_MultipathCore(NumTx, Nx, X, Ns, r, iRx, PositionOffset) );
                break;
            }
        }
        // Frequency Shift and Signal Scaling
        //
        for (k=0; k<Ns; k++)
        {
            c.Real = A * cos(w*k);
            c.Imag = A * sin(w*k);

            s[k] = wiComplexMultiply(c, r[k]);
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_LoadPacket()

// ================================================================================================
// FUNCTION  : wiChanl_ClearPackets()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clear any/all packets loaded into the pState->Interference.Packet structure. This
//             clears only the packet signal and length, not the parameters such as power, time
//             offset, and frequency shift
// Parameters: none
// ================================================================================================
wiStatus wiChanl_ClearPackets(void)
{
    wiChanl_State_t *pState = wiChanl_State();

    if (pState)
    {
        int i, j;
        for (i=0; i<WICHANL_MAXPACKETS; i++)
        {
            for (j=0; j<WICHANL_RXMAX; j++)
                pState->Interference.Packet[i].s[j].Length = 0;
        }
    }
    return WI_SUCCESS;
}    
// end of wiChanl_ClearPackets()

// ================================================================================================
// FUNCTION  : wiChanl_NormalizedFading()
// ------------------------------------------------------------------------------------------------
// Purpose   : Normalize a channel response to have unity gain with fading
// Parameters: c -- pointer to channel response coefficients
// ================================================================================================
wiStatus wiChanl_NormalizedFading(wiChanl_State_t *pState, wiComplex c[])
{
    int k, M;
    double a, S2 = 0.0;

    if (pState->NormalizedFading)
    {
        M = (int)floor(0.5 +50e-9*pState->Fs);

        for (k=0; k<pState->Nc; k++)
            if (k%M) c[k].Real = c[k].Imag = 0.0;

        for (k=0; k<pState->Nc; k++)
            S2 += wiComplexNormSquared(c[k]);

        a = 1.0 / sqrt(S2);

        for (k=0; k<pState->Nc; k++) 
        {
            c[k].Real *= a;
            c[k].Imag *= a;
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_NormalizedFading()

// ================================================================================================
// FUNCTION  : wiChanl_LoadChannelResponse()
// ------------------------------------------------------------------------------------------------
// Purpose   : Load a complex FIR channel response
// Parameters: iRx -- receive antenna index
//             iTx -- transmit antenna index
//             Nc  -- filter response length
//             c   -- filter response
// ================================================================================================
wiStatus wiChanl_LoadChannelResponse(int iRx, int iTx, int Nc, wiComplex c[])
{
    wiChanl_State_t *pState = wiChanl_State();
    int k;

    if (InvalidRange(iRx, 0, WICHANL_RXMAX-1))          return WI_ERROR_PARAMETER1;
    if (InvalidRange(iTx, 0, WICHANL_TXMAX-1))          return WI_ERROR_PARAMETER2;
    if (InvalidRange(Nc,  1, WICHANL_MAX_CHANNEL_TAPS)) return WI_ERROR_PARAMETER3;
    if (c == NULL) return WI_ERROR_PARAMETER4;

    pState->Nc = Nc;

    //  Allocate memory for the channel response
    //
    if (pState->c[iRx][iTx] == NULL)
        WICALLOC (pState->c[iRx][iTx], WICHANL_MAX_CHANNEL_TAPS, wiComplex );

    // Load the FIR coefficents
    //
    for (k=0; k<Nc; k++)
        pState->c[iRx][iTx][k] = c[k];

    return WI_SUCCESS;
}
// end of wiChanl_LoadChannelResponse()

// ================================================================================================
// FUNCTION  : wiChanl_GenerateChannelResponse()
// ------------------------------------------------------------------------------------------------
// Purpose   : Implements the selected multipath channel model
// Parameters: NumTx -- number of transmit streams
//             NumRx -- number of receive antennas
// ================================================================================================
wiStatus wiChanl_GenerateChannelResponse(int NumTx, int NumRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    int i, j, k;
    double Ts = 1.0 / pState->Fs;

    if (pState->SkipGenerateChannel) return WI_SUCCESS;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the FIR Length
    //
    //  For multipath channels, the length is computed to contain the majority of the
    //  energy for the average unconstrained response
    //
    switch (pState->Channel)
    {
        case 0: 
        {
            pState->Nc = 1; // AWGN
            break;
        }
        case 1:
        case 2:
        {
            const double minP = 0.9999;
            pState->Nc = (int)ceil(-(pState->Trms /Ts) * log(1.0 - minP));
            break;
        }
        case 3:
        {
            pState->Nc = 1;
            for (i=0; i<NumRx; i++)
                pState->Nc = max(pState->Nc, 1 + abs(pState->Channel3Dn[i]));
            break;
        }
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    pState->Nc = min(max(pState->Nc, 1), WICHANL_MAX_CHANNEL_TAPS); // limit range

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate the NumTx x NumRx Channels
    //
    for (i=0; i<NumRx; i++)
    {
        for (j=0; j<NumTx; j++)
        {
            wiComplex *c;

            //  Allocate memory for the channel response
            //
			if (pState->c[i][j] == NULL) {
                WICALLOC( pState->c[i][j], WICHANL_MAX_CHANNEL_TAPS, wiComplex );
            }
			c = pState->c[i][j];

            //////////////////////////////////////////////////////////////////////////////////
            //
            //  Compute the channel FIR filter
            //
            //  For Channel = 0, the model is simply line of sight, so the filter is just 1.
            //  Channels 1 and 2 use an exponential multipath channel. The filter is a series
            //  of complex Gaussian random variables with exponentially decaying amplitude.
            //  The rate of decay is specified by the rms delay spread in Trms.
            //
            switch (pState->Channel)
            {
                case 0: // AWGN (Line-of-Sight), c = 1
                {
                    c[0].Real = 1.0;
                    c[0].Imag = 0.0;
                    break;
                }
                case 1: // Exponential Multipath
                case 2: // Exponential Multipath, Doppler
                {
                    wiReal t = (pState->Trms / Ts); // normalize delay spread

                    XSTATUS( wiRnd_ComplexNormal(pState->Nc, c, 1.0) );

                    for (k=0; k<pState->Nc; k++) 
                    {
                        wiReal Sc  = t ? sqrt( (1-exp(-1/t)) * exp(-k/t)) : 1.0;
                        c[k].Real *= Sc;
                        c[k].Imag *= Sc;
                    }
                    XSTATUS( wiChanl_NormalizedFading(pState, c) )
                    break;
                }

                case 3: // (1 +/- D^n) / sqrt(2)
                {
                    wiReal a = 1.0 / sqrt(2.0);

                    if (pState->Channel3Dn[i] == 0)
                        c[0].Real = 1.0;
                    else
                    {
                        c[0].Real = a;
                        c[abs(pState->Channel3Dn[i])].Real = (pState->Channel3Dn[i] > 0) ? a : -a;
                    }
                    break;
                }

                default: return STATUS(WI_ERROR_UNDEFINED_CASE);
            }
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_GenerateChannelResponse()

// ================================================================================================
// FUNCTION  : wiChanl_MultipathCore()
// ------------------------------------------------------------------------------------------------
// Purpose   : Implements the selected multipath channel model
// Parameters: NumTx -- number of transmit streams
//             Nx   -- number of input samples in array x
//             X    -- input signal array
//             Ns   -- number of output samples in array s
//             s    -- output signal array
//             iRx  -- index for the receive array
//             t0   -- number of samples to offset causal tap
// ================================================================================================
wiStatus wiChanl_MultipathCore(int NumTx, int Nx, wiComplex *X[], int Ns, wiComplex *s, int iRx, int t0)
{
    wiChanl_State_t *pState = wiChanl_State();
    double Ts = 1.0 / pState->Fs;
    int iTx, i, j, k;

    // ----------------------
    // Apply Channel Response
    // ----------------------
    switch (pState->Channel)
    {
        // ------------------------------
        // AWGN (Fourier Matrix for MIMO)
        // ------------------------------
        case 0:
        {
            int N = min(Ns-t0, Nx);
            memcpy(s+t0, X[0], N*sizeof(wiComplex)); // copy from first antenna

            for (iTx=1; iTx<NumTx; iTx++)   
            {
                double a  = 2.0*pi*iTx*iRx / max(NumTx,  pState->NumRx);
                double wR =  cos(a);
                double wI = -sin(a);

                for (k=0; k<N; k++)
                {
                    s[t0+k].Real += (wR*X[iTx][k].Real - wI*X[iTx][k].Imag);
                    s[t0+k].Imag += (wR*X[iTx][k].Imag + wI*X[iTx][k].Real);
                }
            }
            break;
        }
        // -----------------------------------
        // Exponential Multipath or Simple FIR
        // -----------------------------------
        case 1:
        case 3:
        {
            XSTATUS( wiMath_ComplexConvolve(Ns-t0, s+t0, Nx, X[0], pState->Nc, pState->c[iRx][0]) );
            for (iTx=1; iTx<NumTx; iTx++) {
                XSTATUS( wiMath_ComplexConvolve_Accumulate(Ns-t0, s+t0, Nx, X[iTx], pState->Nc, pState->c[iRx][iTx]) );
            }
            break;
        }
        // ----------------------------------
        // Exponential Multipath With Doppler
        // ----------------------------------
        case 2:
        {
			wiReal u[WICHANL_MAX_CHANNEL_TAPS];
			wiReal a[WICHANL_MAX_CHANNEL_TAPS];
            int Ny = Ns - t0;
            int N = (Nx<Ny) ? Nx:Ny;
            wiComplex h, *y = s + t0;

            memset(y, 0, Ny * sizeof(wiComplex)); // clear the output array

            // -----------------------------
            // Loop through transmit signals
            // -----------------------------
            for (iTx=0; iTx<NumTx; iTx++)
            {
                wiComplex *x = X[iTx];
                wiComplex *c = pState->c[iRx][iTx];

                // -------------
                // Doppler model
                // -------------
                wiRnd_Uniform(pState->Nc, u);
                for (i=0; i<pState->Nc; i++)
                    a[i] = 2.0 * pi * pState->fDoppler * cos(2*pi*u[i]) * Ts;

                // -------------------
                // Complex convolution
                // -------------------
                for (k=0; k<pState->Nc; k++) {
                    for (j=0; j<=k; j++) 
                    {
                        h = wiComplexMultiply( c[j], wiComplexExp(a[j]*k) );
                        
                        y[k].Real += ((x[k-j].Real * h.Real) - (x[k-j].Imag * h.Imag));
                        y[k].Imag += ((x[k-j].Imag * h.Real) + (x[k-j].Real * h.Imag));
                    }
                }
                for (; k<N; k++) {
                    for (j=0; j<pState->Nc; j++) 
                    {
                        h = wiComplexMultiply( c[j], wiComplexExp(a[j]*k) );
                        
                        y[k].Real += ((x[k-j].Real * h.Real) - (x[k-j].Imag * h.Imag));
                        y[k].Imag += ((x[k-j].Imag * h.Real) + (x[k-j].Real * h.Imag));
                    }
                }
                for (; k<Ny; k++) {
                    for (j=0; j<pState->Nc; j++) 
                    {
                        if (k-j >= Nx) continue;

                        h = wiComplexMultiply( c[j], wiComplexExp(a[j]*k) );

                        y[k].Real += ((x[k-j].Real * h.Real) - (x[k-j].Imag * h.Imag));
                        y[k].Imag += ((x[k-j].Imag * h.Real) + (x[k-j].Real * h.Imag));
                    }
                }
            }
            break;
        }
        // -----------------------
        // Default: Undefined Case
        // -----------------------
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of wiChanl_MultipathCore()

// ================================================================================================
// FUNCTION  : wiChanl_GetPositionOffset()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the position offset for the current packet
// ================================================================================================
wiStatus wiChanl_GetPositionOffset(wiChanl_State_t *pState)
{
    if (pState->PositionOffset.Enabled)
    {
        pState->PositionOffset.t0 = (int)(pState->PositionOffset.Range * wiRnd_UniformSample());
        pState->PositionOffset.t0 += pState->PositionOffset.dt;
    }
    else 
        pState->PositionOffset.t0 = 0;

    return WI_SUCCESS;
}
// end of wiChanl_GetPositionOffset()

// ================================================================================================
// FUNCTION  : wiChanl_InterferenceModel
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level channel model interface function
// Parameters: Ns    -- number of samples in s
//             s     -- input/output array           
//             iRx   -- receive path index
// ================================================================================================
wiStatus wiChanl_InterferenceModel(int Ns, wiComplex s[], int iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    int i, j, k;
    int k0 = (int)floor(pState->Interference.t0*pState->Fs + 0.5); // interference start time index

    switch (pState->Interference.Model)
    {
        case WICHANL_INTERFERENCE_NONE: // No interference
            break;

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate a single complex tone. 
        //  The phase of the tone is a uniform random variable
        //
        case WICHANL_INTERFERENCE_TONE:
        {
            double A   = pow(10.0, (pState->Interference.PrdBm-30)/20);
            double w   = 2.0 * pi * pState->Interference.f_MHz / (pState->Fs/1.0E+6);
            double phi = 2.0 * pi * wiRnd_UniformSample(); 

            for (k = max(0, k0); k < Ns; k++)
            { 
                s[k].Real += A * cos(w*k + phi);
                s[k].Imag += A * sin(w*k + phi);
            }
            break;
        }

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Use the previous packet. On the first call (where a prior packet is not available)
        //  the current packet is used. Two models are available: a simple linear addition and
        //  a nonlinear model that applies both nonlinearity and additive noise to approximate
        //  distortion and TX noise.
        //
        //  In both cases, the wiChanl_Model() function automatically stores the interference
        //  packet so no additional effort is required from the test routine.
        //
        case WICHANL_INTERFERENCE_LAST_PACKET:           // Models adding the previous packet
        case WICHANL_INTERFERENCE_NONLINEAR_LAST_PACKET:
        {
            int i = (pState->Interference.Packet[1].s[0].Length > 0) ? 1 : 0;

            int LastPacketLength   = pState->Interference.Packet[i].s[iRx].Length;
            wiComplex *pLastPacket = pState->Interference.Packet[i].s[iRx].ptr;

            int j = (k0 < 0) ? -k0: 0;
            int k = (k0 < 0) ?   0:k0;

            while (k<Ns && j<LastPacketLength)
            {
                s[k].Real += pLastPacket[j].Real;
                s[k].Imag += pLastPacket[j].Imag;
                j++; k++;
            }
            break;
        }

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Interference generated using Guassian noise shaped to match the 802.11a transmit
        //  mask. This is useful for looking at simple signal-to-interference cases where the
        //  added authenticity of a preamble is not required.
        //
        case WICHANL_INTERFERENCE_80211A_MASK:
        {
            wiComplex H[256], h[256], *v, *z;
            wiReal f, A;
            wiReal Mask[256];
            double Fs_MHz = pState->Fs / 1.0E+6;

            // Build the transmit spectrum mask
            // --------------------------------
            for (i=-128; i<128; i++)
            {
                f = fabs((Fs_MHz/256.0)*i - pState->Interference.f_MHz);
                k = (256+i)%256; 

                     if (f< 9) Mask[k] =   0.0;
                else if (f<11) Mask[k] = -10.0        *(f- 9);
                else if (f<20) Mask[k] = -20.0-8.0/9.0*(f-11);
                else if (f<30) Mask[k] = -28.0-1.2    *(f-20);
                else           Mask[k] = -40.0;
            }
            // Create the frequency domain filter
            // ----------------------------------
            for (k=0; k<256; k++)
            {
                A = pow(10.0, (Mask[k] + pState->Interference.PrdBm-30)/20.0);
                H[k].Real = A * cos(-pi*k);
                H[k].Imag = A * sin(-pi*k);
            }
            // Convert to frequency domain to an FIR filter
            // --------------------------------------------
            {
                wiComplex *w = pLookupTable->IDFT_W_256;
    
                for (k=0; k<256; k++)
                {
                    h[k].Real = h[k].Imag = 0.0;
                    for (j=0; j<256; j++)
                    {
                        i = (k * j) % 256;
                        h[k].Real += (H[j].Real * w[i].Real - H[j].Imag * w[i].Imag);
                        h[k].Imag += (H[j].Real * w[i].Imag + H[j].Imag * w[i].Real);
                    }
                }
            }
            // Generate white Gaussian sequence
            // --------------------------------
            GET_WIARRAY( v, pState->Interference.v, Ns+256, wiComplex )
            XSTATUS(wiRnd_ComplexNormal(Ns+256, v, sqrt(Fs_MHz/20.0*9/8)));

            // Add the interfering channel by filtering the WGN with the spectral mask filter
            // ------------------------------------------------------------------------------
            z = v + 256; // add 256 sample delay to make convolution below causal
            for (k = max(0, k0); k < Ns; k++)
            { 
                for (i=0; i<256; i++)
                {
                    s[k].Real += (h[i].Real * z[k-i].Real - h[i].Imag * z[k-i].Imag);             
                    s[k].Imag += (h[i].Real * z[k-i].Imag + h[i].Imag * z[k-i].Real);             
                }
            }
            break;
        } 

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  The multiple packet interference model requires the test routine to explicity load
        //  interference packets using wiTest_LoadPacket.
        //
        case WICHANL_INTERFERENCE_MULTIPLE_PACKETS:
        {
            int i;

            for (i=0; i<WICHANL_MAXPACKETS; i++) // loop through all possible stored packets
            { 
                if (pState->Interference.Packet[i].s[0].Length > 0) // if there is a packet array...
                {
                    int k0 = (int)floor(0.5 +pState->Interference.Packet[i].t0*pState->Fs);
                    int LastPacketLength   = pState->Interference.Packet[i].s[iRx].Length;
                    wiComplex *pLastPacket = pState->Interference.Packet[i].s[iRx].ptr;

                    int j = (k0<0) ? -k0 :  0;
                    int k = (k0<0) ?   0 : k0;

                    while (k<Ns && j<LastPacketLength)
                    {
                        s[k].Real += pLastPacket[j].Real;
                        s[k].Imag += pLastPacket[j].Imag;
                        j++; k++;
                    }
                }
            }
            break;
        }

        default: return STATUS( WI_ERROR_UNDEFINED_CASE );
    }
    return WI_SUCCESS;
}
// end of wiChanl_InterferenceModel()

// ================================================================================================
// FUNCTION  : wiChanl_DFS_RadarModel
// ------------------------------------------------------------------------------------------------
// Purpose   : Add dynamic frequency selection (DFS) type radar signal to received signal
// Parameters: Ns    -- number of samples in s
//             s     -- input/output array           
// ================================================================================================
wiStatus wiChanl_DFS_RadarModel(int Ns, wiComplex s[], int iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    if (pState->Radar.Model)
    {
        int    k0 = pState->Radar.k0;                                  // sample index at start of burst
        int    n  = (int)floor(pState->Fs*pState->Radar.W*1e-6 + 0.5); // n = # samples in each pulse
        int    m  = (int)floor(pState->Fs/pState->Radar.PRF    + 0.5); // m = # samples in 10 microsecond period
        double A  = pow(10.0, (pState->Radar.PrdBm - 30)/20);          // signal gain
        
        int i, j, k;
        double theta;

        wiBoolean *MissingPulse;

        GET_WIARRAY( MissingPulse, pState->Radar.MissingPulse, pState->Radar.N, wiBoolean );

        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Compute a random starting position. If enabled, this model causes the radar burst
        //  to begin at a random point within the signal stream. Currently the model allows
        //  the burst to begin anywhere, which implies that for some cases it may start too
        //  late to allow detection within the signal stream
        //
        if (pState->Radar.RandomStart)
        {
            if (iRx == 0) 
                pState->Radar.k0RandomStart = (int)floor(wiRnd_UniformSample() * (double)Ns);
            k0 = pState->Radar.k0RandomStart;
        }
        //
        //  Select "missing" pulses
        //
        if (iRx == 0)
        {
            for (i = 0; i < pState->Radar.N; i++)
                MissingPulse[i] = wiRnd_UniformSample() < pState->Radar.pMissing ? 1:0;
        }
        // ----------------
        // Add Interference
        // ----------------
        switch (pState->Radar.Model)
        {
            //////////////////////////////////////////////////////////////////////////////////
            //
            //  Pulsed Tone Radar Signal
            //
            //  The pulsed tone is simply a carrier (complex tone) that is gated on and off
            //  with the specified width and period. The probability of a missing pulse
            //  (pMissing) may be specified to model the absence of pulses due to other
            //  channel activity.
            //
            case 1:
            {
                double w  = 2.0 * pi * pState->Radar.f_MHz / (pState->Fs/1.0E+6);

                for (i=0; i<pState->Radar.N; i++)
                {
                    if (!MissingPulse[i])
                    {
                        k = k0 + i*m + ((i<WICHANL_RADAR_MAX_DK) ? pState->Radar.dk[i] : 0);
                        for (j=0; (j<n && k<Ns); j++, k++)
                        {
                            theta = w * k ;
                            s[k].Real += A * cos(theta);
                            s[k].Imag += A * sin(theta);
                        }
                    }
                }
            }  break;

            //////////////////////////////////////////////////////////////////////////////////
            //
            //  Chirp Radar Signal
            //
            //  A chirp radar signal is a pulse frequency modulated with linearly increasing
            //  frequency. In this case, the center frequency and frequency width are given.
            //  As with the pulsed tone, pMissing is the probability of missing a pulse.
            //
            //  Note that the computation of dw includes a divide-by-2 at the end that may
            //  appear to be incorrect. This arises because the expression gives phase. When
            //  the derivative is taken to get frequency, the t^2 term multiplying dw gives
            //  2*t so the 2's multiply to one and the frequency modulation is linear.
            //
            case 2:
            {
                double w  = 2.0 * pi * (pState->Radar.f_MHz - 0.5*pState->Radar.fRangeMHz) / (pState->Fs/1.0E+6);
                double dw = 2.0 * pi * (pState->Radar.fRangeMHz) / (pState->Fs/1.0E+6) / n / 2.0;

                for (i=0; i<pState->Radar.N; i++)
                {
                    if (!MissingPulse[i])
                    {
                        k = k0 + i*m + ((i<WICHANL_RADAR_MAX_DK) ? pState->Radar.dk[i] : 0);
                        for (j=0; (j<n && k<Ns); j++, k++)
                        {
                            theta = w*k + j*j*dw;
                            s[k].Real += A * cos(theta);
                            s[k].Imag += A * sin(theta);
                        }
                    }
                }
            }  break;

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_DFS_RadarModel()

// ================================================================================================
// FUNCTION  : wiChanl_AmplitudeModulation
// ------------------------------------------------------------------------------------------------
// Purpose   : Phase Noise/Carrier Frequency Offset Model
// Parameters: Ns    -- number of samples in s
//             s     -- input/output array           
// ================================================================================================
wiStatus wiChanl_AmplitudeModulation(int Ns, wiComplex s[], int iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Sinusoidal Amplitude Modulation
    //  Amplitude is modulated linearly or logarithmically according to a sinusoidal
    //  profiles. Each receiver path is treated independently.
    //
    if (pState->AM.Enabled && pState->AM.f[iRx] && pState->AM.dB[iRx]) 
    {
        wiInt k;
        wiReal w   = 2.0 * pi * pState->AM.f[iRx] / pState->Fs;
        wiReal phi = 2.0 * pi * pState->AM.dT[iRx];
        wiReal c;

        if (pState->AM.Linear)
        {   
            wiReal A = min(1.0, pow(10.0, -fabs(pState->AM.dB[iRx])/20.0) - 1.0);

            for (k=pState->AM.k0; k<Ns; k++)
            {
                c = 1.0 + A * cos(w*(k - pState->AM.k0) + phi);
                s[k].Real *= c;
                s[k].Imag *= c;
            }
        }
        else
        {
            for (k=pState->AM.k0; k<Ns; k++)
            {
                c = pow(10.0, pState->AM.dB[iRx]/20.0*cos(w*(k - pState->AM.k0) + phi));
                s[k].Real *= c;
                s[k].Imag *= c;
            }
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Amplitude Ramp
    //  Linear gain change from samples index k1 to k2. The gain is maintained at the
    //  final value from k2 to the end of the waveform
    //
    if (pState->AM.Enabled && pState->AM.Ramp.A != 1)
    {
        wiUInt k;
        wiReal A;
        wiReal dA = pState->AM.Ramp.A - 1.0;
        wiReal T = (double)(pState->AM.Ramp.k2 - pState->AM.Ramp.k1);

        for (k=pState->AM.Ramp.k1; k<min(pState->AM.Ramp.k2, (unsigned)Ns); k++)
        {
            A = 1.0 + dA * (double)(k - pState->AM.Ramp.k1)/T;
            s[k].Real *= A;
            s[k].Imag *= A;
        }
        for (k=pState->AM.Ramp.k2; k<(unsigned)Ns; k++)
        {
            s[k].Real *= pState->AM.Ramp.A;
            s[k].Imag *= pState->AM.Ramp.A;
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_AmplitudeModulation()

// ================================================================================================
// FUNCTION  : wiChanl_PhaseFrequencyError
// ------------------------------------------------------------------------------------------------
// Purpose   : Phase Noise/Carrier Frequency Offset Model
// Parameters: Ns    -- number of samples in s
//             s     -- input/output array           
// ================================================================================================
wiStatus wiChanl_PhaseFrequencyError(int Ns, wiComplex s[], int iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    wiReal     u; // random phase between RX and TX
    wiReal    *a;
    int        k;

    if (!pState->PhaseNoise.PSD && !pState->fc_kHz && !pState->RandomPhase) 
        return WI_SUCCESS; // no phase/frequency errors...nothing to do here

    // ---------------------------
    // Generate Common Phase Error
    // ---------------------------
    if (iRx == 0)
    {
        GET_WIARRAY( a, pState->PhaseNoise.a, Ns, wiReal );
        memset(a, 0, Ns*sizeof(wiReal));

        // --------------------
        // Generate phase noise
        // --------------------
        if (pState->PhaseNoise.PSD)
        {
            wiInt  FilterN = 1;
            wiReal B[3], A[3], Su, fc, *u;
			
            GET_WIARRAY( u, pState->PhaseNoise.u, Ns, wiReal )

            Su = pState->Fs * sqrt(1/pState->Fs * pow(10.0, pState->PhaseNoise.PSD/10.0));
            fc = pState->PhaseNoise.fc / pState->Fs;

            XSTATUS(wiRnd_Normal(Ns, u, Su));
            XSTATUS(wiFilter_Butterworth(FilterN, fc, B, A, WIFILTER_IMPULSE_INVARIANCE));
            XSTATUS(wiFilter_IIR(FilterN, B, A, Ns, u, a, NULL));
        }
        // ---------------------
        // Add frequency offsets
        // ---------------------
        if (pState->fc_kHz)
        {
            wiReal w = 2.0 * pi * (pState->fc_kHz * 1.0E+3) / pState->Fs;
            for (k=0; k<Ns; k++)
                a[k] += (wiReal)k * w;
        }
    }
    else a = pState->PhaseNoise.a.ptr;

    // ----------------------------------------------
    // Generate the per-antenna random TX-to-RX phase
    // ----------------------------------------------
    u = pState->RandomPhase ? 2*pi * wiRnd_UniformSample() : 0;

    // -------------------------------
    // Apply the phase/frequency error
    // -------------------------------
    for (k=0; k<Ns; k++)
        s[k] = wiComplexMultiply(s[k], wiComplexExp(a[k] + u)); // s[k] = s[k] * exp(j*(a[k]+u))

    return WI_SUCCESS;
}
// end of wiChanl_PhaseFrequencyError()


// ================================================================================================
// FUNCTION  : wiChanl_AddNoise()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_AddNoise(int Nr, wiComplex r[])
{
    wiChanl_State_t *pState = wiChanl_State();

    if (pState->Nrms)
    {
        int k;
        wiComplex *n;

        GET_WIARRAY( n, pState->Model_1.n, Nr, wiComplex )
        XSTATUS( wiRnd_ComplexNormal(Nr, n, pState->Nrms) );

        for (k=0; k<Nr; k++)
        {
            r[k].Real += n[k].Real;
            r[k].Imag += n[k].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_AddNoise()


// ================================================================================================
// FUNCTION  : wiChanl_ScaleReceivedSignal()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiChanl_ScaleReceivedSignal(int Nr, wiComplex r[], int iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    if (pState->Av[iRx] != 1.0)
    {
        int k;
        for (k=0; k<Nr; k++)
        {
            r[k].Real *= pState->Av[iRx];
            r[k].Imag *= pState->Av[iRx];
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_ScaleReceivedSignal()


// ================================================================================================
// FUNCTION  : wiChanl_RappPA
// ------------------------------------------------------------------------------------------------
// Purpose   : Rapp Power Amplifier Model
// Parameters: Nx -- number of samples in x
//             X  -- input array
//             y  -- output array
// ================================================================================================
wiStatus wiChanl_RappPA(int Nx, wiComplex x[], wiComplex *y, wiReal Prms, wiReal p, wiReal OBOdB, wiReal Ap)
{
    int k;
    double Psat, Pin, Av;
    Psat = pow(10.0,OBOdB/10.0) * Prms / pow(pow(10,p/10)-1,1/p);

    for (k=0; k<Nx; k++)
    {
        Pin = (x[k].Real * x[k].Real) + (x[k].Imag * x[k].Imag);
        Av = Pin ? sqrt(Ap / pow(1 + pow(Pin/Psat, p), 1/p)) : sqrt(Ap);

        y[k].Real = Av * x[k].Real;
        y[k].Imag = Av * x[k].Imag;
    }
    return WI_SUCCESS;
}
// end of wiChanl_RappPA()

// ================================================================================================
// FUNCTION  : wiChanl_KeNingPA
// ------------------------------------------------------------------------------------------------
// Purpose   : Model the power amplifier transfer function as a polynomial. This baseband 
//             model is from Dakota Notebook p. 10-12. The saturation region model was 
//             derived by Ke Ning.                  
// Parameters: Nx -- number of samples in x
//             X  -- input array
//             y  -- output array
// ================================================================================================
wiStatus wiChanl_KeNingPA( int Nx, wiComplex x[], wiComplex *y, wiReal Anorm, wiReal Amax, 
                           wiReal g1, wiReal g3, wiReal g5 )
{
    int k;
    wiReal A, A2, A4, b, c, t; 

    for (k=0; k<Nx; k++)
    {
        A  = Anorm * sqrt(x[k].Real*x[k].Real + x[k].Imag*x[k].Imag);
        A2 = A  * A;
        A4 = A2 * A2;
        //
        // saturated region
        //
        if (A > Amax) 
        {
            t = 1.0 / pi * acos(Amax/A);
            c = (1.000 - 2.00*t - 1/pi*sin(2*pi*t))
              + (0.750 - 1.50*t - 1/pi*sin(2*pi*t) - 1/(8*pi)*sin(4*pi*t))*g3*A2
              + (0.625 - 1.25*t - 15/(16*pi)*sin(2*pi*t) - 3/(16*pi)*sin(4*pi*t))*g5*A4
              + 4/pi*sin(pi*t)/A;
        }
        //
        // operating region...polynomial
        //
        else if (A > 0)
        {
            b = g1 + 0.75*g3*A2 + 0.625*g5*A4;
            c = (b/g1);
        }
        //
        // zero input signal
        //
        else c = 0.0;

        y[k].Real = c * x[k].Real;
        y[k].Imag = c * x[k].Imag;
    }
    return WI_SUCCESS;
}
// end of wiChanl_KeNingPA()

// ================================================================================================
// FUNCTION  : wiChanl_PowerAmp
// ------------------------------------------------------------------------------------------------
// Purpose   : Power amplifier model for multiple transmit streams
// Parameters: NumTx -- number of transmit streams
//             Nx   -- number of samples in x
//             X    -- input arrays
//             y    -- output arrays
//             Prms  -- Input signal power (complex, rms)
// ================================================================================================
wiStatus wiChanl_PowerAmp(int NumTx, int Nx, wiComplex *X[], wiComplex *Y[], wiReal Prms)
{
    wiChanl_State_t *pState = wiChanl_State();
    int iTx;

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!pState)                    return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (InvalidRange(Nx, 1, 1<<28)) return WI_ERROR_PARAMETER1;
    if (!X)                         return WI_ERROR_PARAMETER2;
    if (!Y)                         return WI_ERROR_PARAMETER3;

    // ---------------------
    // Power Amplifier Model
    // ---------------------
    if (pState->PowerAmp.Enabled)
    {
        for (iTx=0; iTx<NumTx; iTx++)
        {
            GET_WIARRAY( Y[iTx], pState->PowerAmp.y[iTx], Nx, wiComplex );

            switch (pState->PowerAmp.Model)
            {
                case 1: // Ke Ning's Model
                    XSTATUS( wiChanl_KeNingPA(Nx, X[iTx], Y[iTx], pState->PowerAmp.Anorm, pState->PowerAmp.Amax, 
                                              pState->PowerAmp.g1, pState->PowerAmp.g3, pState->PowerAmp.g5) );
                    break;

                case 2: // Rapp's Model
                    XSTATUS( wiChanl_RappPA(Nx, X[iTx], Y[iTx], Prms, pState->PowerAmp.p, pState->PowerAmp.OBO, 
                                            pState->PowerAmp.ApRapp) ); 
                    break;

                default: return STATUS(WI_ERROR_UNDEFINED_CASE);
            }
        }
    }
    else
    {
        for (iTx=0; iTx<NumTx; iTx++)
            Y[iTx] = X[iTx];
    }
    return WI_SUCCESS;
}
// end of wiChanl_PowerAmp()

// ================================================================================================
// FUNCTION  : wiChanl_Resample
// ------------------------------------------------------------------------------------------------
// Purpose   : Resample a signal to add timing errors (timing frequency offset + jitter)
// Parameters: Nx  -- number of samples in x
//             X   -- signal array (output replaces input)
//             iRx -- RX antenna number
// ================================================================================================
wiStatus wiChanl_Resample(int Nx, wiComplex x[], wiInt iRx)
{
    wiChanl_State_t *pState = wiChanl_State();

    int Nz, i, j, k, k0, n;
    wiComplex *z, *y; wiReal *dt;
    double t, T, s;
    double *w;

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!pState)                     return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (InvalidRange(Nx, 1, 1<<28)) return WI_ERROR_PARAMETER1;
    if (x == NULL)                  return WI_ERROR_PARAMETER2;

    // -------------------------------------
    // Return if model disabled or no errors
    // -------------------------------------
    if (!pState->TimingErrorModel || (!pState->fs_ppm && !pState->SampleJitter.Jrms && !pState->SampleJitter.Jpk)) 
        return WI_SUCCESS;

    // -------------------------------------
    // New sample period relative to the old
    // -------------------------------------
    T = 1.0 + pState->fs_ppm/1.0E+6;

    // -----------------------------------------
    // Copy the input array to a temporary array
    // -----------------------------------------
    k0 = WICHANL_INTERPOLATOR_TAPS/2;
    Nz = (int)ceil((double)Nx * max(1,T)) + WICHANL_INTERPOLATOR_TAPS + 3;

    GET_WIARRAY( z, pState->SampleJitter.z, Nz, wiComplex )

    memcpy(z+k0, x, Nx * sizeof(wiComplex));

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Create the timing error array
    //
    //  This is done only for iRx == 0. The idea is that jitter should be common for all
    //  antennas as the DACs on the TX side or ADCs on the RX side use a common clock.
    //  The random timing error is Gaussian shaped with a single pole low pass filter.
    //  If no  bandwidth is specified the spectrum is flat.
    //
    //  A periodic timing error is created by modulating the timing with a sinusoid. This is
    //  useful for creating a bounded timing error.
    //
    GET_WIARRAY( dt, pState->SampleJitter.dt, Nx, wiReal );
    if (iRx == 0)
    {
        if (pState->SampleJitter.Jrms) ////// Random Timing Error //////
        {
            if (pState->SampleJitter.fc)
            {
                wiReal B[3], A[3];
                wiReal *v;

                double fc = pState->SampleJitter.fc / pState->Fs;
                double Jrms = (pState->SampleJitter.Jrms * pState->Fs) / sqrt((pi/2) * (2*fc));
                
                GET_WIARRAY( v, pState->SampleJitter.v, Nx, wiReal );

                wiRnd_Normal(Nx, v, Jrms);
                XSTATUS( wiFilter_Butterworth(1, fc, B, A, WIFILTER_IMPULSE_INVARIANCE) );
                XSTATUS( wiFilter_IIR(1, B, A, Nx, v, dt, NULL) );
            }
            else
            {
                double Jrms = pState->SampleJitter.Jrms * pState->Fs;
                wiRnd_Normal(Nx, dt, Jrms);
            }
        }
        if (pState->SampleJitter.Jpk) ///// Periodic Timing Error /////
        {
            double w = 2.0 * pi * pState->SampleJitter.fHz / pState->Fs; // timing modulation frequency
            double A = pState->SampleJitter.Jpk * pState->Fs;            // timing error scaled to sample period

            for (k=0; k<Nx; k++)
                dt[k] += A * sin(w*k);
        }
    }

    // ----------------------
    // Clear the output array
    // ----------------------
    for (k=0; k<Nx; k++) 
        x[k] = wiComplexZero;

    // -----------------------------------------------------
    // Rename pointers to x is the input and y is the output
    // -----------------------------------------------------
    y = x;
    x = z;

    // -----------------------------
    // Resample so that y(k) = x(kT)
    // -----------------------------
    for (k=0; k<Nx; k++)
    {
        t = k*T + dt[k];                  // new time at sample index k
        t = min(t, Nx + 1);
        t = max(t, 0.0);
        j = (int)floor(t + 0.5);          // closest input sample index
        s = t - j;                        // phase shift
        n = (int)floor(0.5 + (s + 0.5)*(WICHANL_INTERPOLATOR_STEPS - 1)); // select filter
        w = pLookupTable->wInterpolate[n];// w points to filter taps

        for (i=WICHANL_INTERPOLATOR_TAPS-1; i>=0; i--, j++)
        {
            y[k].Real += w[i] * x[j].Real;
            y[k].Imag += w[i] * x[j].Imag;
        }
    }
    return WI_SUCCESS;
}
// end of wiChanl_Resample()

// ================================================================================================
// FUNCTION  : wiChanl_WriteConfig_Model_1
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiChanl_WriteConfig_Model_1(wiMessageFn MsgFunc)
{
    wiChanl_State_t *pState = wiChanl_State();

    if (pState->SkipGenerateChannel)
    {
        switch (pState->Channel)
        {
            case 0:
            case 1:MsgFunc(" Channel Response      = Manually Generated FIR\n"); break;
            case 2:MsgFunc(" Channel Response      = Manually Generated FIR, fDoppler=%1.0f Hz\n", pState->fDoppler); break;
        }
    }
    else
    {
        switch (pState->Channel)
        {
            case 0:MsgFunc(" Channel Response      = AWGN\n"); break;
            case 1:MsgFunc(" Channel Response      = Exponential Multipath Fading with Trms=%1.0fns\n", pState->Trms*1e9); break;
            case 2:MsgFunc(" Channel Response      = Exponential Multipath with Trms=%1.0fns, fDoppler=%1.0f Hz\n", pState->Trms*1e9, pState->fDoppler); break;
            case 3:MsgFunc(" Channel Response      = 1 +/ D^n; D = [%d, %d]\n",pState->Channel3Dn[0],pState->Channel3Dn[1]); break;
        }
    }
    if (pState->NormalizedFading && pState->Channel) MsgFunc(" Normalized Fading     = ENABLED\n");
    if (pState->FixedNoiseFloor)                    MsgFunc(" Fixed Noise Floor     = ENABLED\n");
    if (pState->AntennaBalance)                     MsgFunc(" Antenna Balance       = ENABLED\n");
    if (pState->dBGain[0] || pState->dBGain[1])
        MsgFunc(" Antenna 0/1 Gain      = %3.1f dB, %3.1f dB\n", pState->dBGain[0],pState->dBGain[1]);

    MsgFunc(" Antennas (TXxRX)      = ? x %d\n",pState->NumRx);

    if (pState->GainStep.Enabled)
    {
        int i;
        MsgFunc(" Gain Step             : k0 = %d, A = [",pState->GainStep.k0);
        for (i=0; i<pState->NumRx; i++) MsgFunc(" %1.1f ",pState->GainStep.dB[i]);
        MsgFunc("] dB\n");
    }
    if (pState->AM.Enabled)
    {
        if (pState->AM.Linear)
        {
            wiReal A = min(1.0, pow(10, pState->AM.dB[0]/20.0));
            MsgFunc(" Amplitude Modulation  : k0 = %d, f[0] = %1.3f kHz, A[0] = 1.0 +/- %1.2f\n",pState->AM.k0,pState->AM.f[0]/1e3,A);
        }
        else
            MsgFunc(" Amplitude Modulation  : k0 = %d, f[0] = %1.3f kHz, A[0] = %1.1f dB\n",pState->AM.k0,pState->AM.f[0]/1e3,pState->AM.dB[0]);
    }
    
    if (pState->PowerAmp.Enabled)
    {
        switch (pState->PowerAmp.Model)
        {
            case 1:
                MsgFunc(" Power Amplifier Model : g(x) = x(%6.4f - %6.4fx^2 - %8.6fx^4)",
                    pState->PowerAmp.g1, -pState->PowerAmp.g3, -pState->PowerAmp.g5);
                MsgFunc("\n");
                MsgFunc("                       : Amax = %6.4f, Anorm = %6.4f\n",pState->PowerAmp.Amax, pState->PowerAmp.Anorm);
                break;
            case 2:
                MsgFunc(" Rapp PA Model         : p = %2.1f, OBO = %1.1f dB, Av = %1.1f dB\n",pState->PowerAmp.p, pState->PowerAmp.OBO,10.0*log10(pState->PowerAmp.ApRapp));
                break;
        }  
    }
    switch (pState->Interference.Model)
    {
        case 0: MsgFunc(" Interference Model    : None\n"); break;
        case 1: MsgFunc(" Interference Model    : Complex sinusoid at f=%2.3f MHz, Pr=%2.1f dBm, dt=%1.1fus\n",pState->Interference.f_MHz, pState->Interference.PrdBm,pState->Interference.t0*1.0e6); break;
        case 2: MsgFunc(" Interference Model    : Last Tx, df=%2.1f MHz, Pr=%2.1f dBm, dt=%1.1fus\n",pState->Interference.f_MHz, pState->Interference.PrdBm,pState->Interference.t0*1.0e6); break;
        case 3: MsgFunc(" Interference Model    : 802.11a Tx Mask at f0=%g MHz, Pr=%gf dBm, dt=%gus\n",pState->Interference.f_MHz, pState->Interference.PrdBm,pState->Interference.t0*1.0e6); break;
        case 4: MsgFunc(" Interference Model    : Last Tx, df=%g MHz, Pr=%g dBm, dt=%gus, Rapp(p=%g, OBO=%gdB, Av=%1.1fdB)\n",pState->Interference.f_MHz, pState->Interference.PrdBm,pState->Interference.t0*1.0e6,pState->Interference.pRapp,pState->Interference.OBOdB,10*log10(pState->Interference.ApRapp));
            if (pState->Interference.Pn_dBm_MHz && pState->Interference.nk2 > pState->Interference.nk1) 
               MsgFunc("                         Pn = %1.1f dBm/MHz, k1=%d, k2=%d\n",pState->Interference.Pn_dBm_MHz,pState->Interference.nk1,pState->Interference.nk2);
            break;
        case 5: MsgFunc(" Interference Model    : Multiple Tx Packets\n"); break;
        default:MsgFunc(" Interference Model    : Model=%d <add description in %s line %d\n",pState->Interference.Model,__FILE__,__LINE__);
    }
    if (pState->Radar.Model)
    {
        char k0str[16] = {"Random\0"};
        if (!pState->Radar.RandomStart) sprintf(k0str,"%d",pState->Radar.k0);
        switch (pState->Radar.Model)
        {
            case 0: MsgFunc(" DFS Radar Model       : None\n"); break;
            case 1: 
                MsgFunc(" DFS Radar Model       : Pulsed Tone: W=%1.1f us, PRF=%1.0f pps, N=%d pulses\n",pState->Radar.W,pState->Radar.PRF,pState->Radar.N); 
                MsgFunc("                                      f=%2.1f MHz, Pr=%2.1f dBm, k0=%s\n",pState->Radar.f_MHz, pState->Radar.PrdBm, k0str); 
                break;
            case 2: 
                MsgFunc(" DFS Radar Model       : Chirp: W=%1.1f us, PRF=%1.0f pps, N=%d pulses\n",pState->Radar.W,pState->Radar.PRF,pState->Radar.N); 
                MsgFunc("                                f=(%2.1f, %2.1f) MHz, Pr=%2.1f dBm, k0=%s\n",pState->Radar.f_MHz, pState->Radar.f_MHz+pState->Radar.fRangeMHz, pState->Radar.PrdBm, k0str); 
                break;
            default:MsgFunc(" DFS Radar Model       : Model=%d, <add description in %s line %d\n",pState->Radar.Model,__FILE__,__LINE__);
        }
    }
    MsgFunc(" Carrier Freq Offset   = %2.1f kHz, RandomPhase = %d\n",pState->fc_kHz,pState->RandomPhase);
    if (pState->PhaseNoise.PSD)
        MsgFunc(" Phase Noise           = %1.1f dBc/Hz, fc=%1.1f kHz\n",pState->PhaseNoise.PSD,pState->PhaseNoise.fc/1000.0);
    if (pState->TimingErrorModel) {
        MsgFunc(" Timing Error Model    : SFO = %2.1f ppm, Jitter = %1.3f ns,rms, fc=%1.0f kHz\n",pState->fs_ppm,pState->SampleJitter.Jrms*1e9,pState->SampleJitter.fc/1000.0);
        if (pState->SampleJitter.Jpk)
            MsgFunc("                       : Sinusoidal Timing Error = %1.3f ns @ %1.3f MHz\n",pState->SampleJitter.Jpk*1e9,pState->SampleJitter.fHz/1.0e6);
    }
    if (pState->PositionOffset.Enabled)
        MsgFunc(" Packet Position Offset= (-%d,-%d) Integer Samples\n",pState->PositionOffset.dt,pState->PositionOffset.Range+pState->PositionOffset.dt);
}
// end of wiChanl_WriteConfig_Model_1()

// ================================================================================================
// FUNCTION  : wiChanl_WriteConfig
// ------------------------------------------------------------------------------------------------
// Purpose   : Output the model configuration using the message handling function
// Parameters: MsgFunc -- output function (similar to printf)
// ================================================================================================
wiStatus wiChanl_WriteConfig(wiMessageFn MsgFunc)
{
    wiChanl_State_t *pState = wiChanl_State();

    if (!pState) return WI_ERROR_MODULE_NOT_INITIALIZED;

    // Write Model-Specific Information
    //
    switch (pState->Model)
    {
        case 0:
        case 1:
            MsgFunc(" Channel Model         = 1\n");
            wiChanl_WriteConfig_Model_1(MsgFunc);
            break;

    #ifdef __WITGNCHANL_H
        case 2:
            MsgFunc(" Channel Model         = 2 (TGn Model)\n");
            XSTATUS( wiTGnChanl_WriteConfig(MsgFunc) );
            break;
    #endif

        default:
            MsgFunc(" Channel Model         = %d <--- UNKNOWN MODEL\n",pState->Model);
    }
    //
    // Write Common Model Information
    //
    if (pState->IQAngleErrorDeg || (pState->IQGainRatio!=0.0 && pState->IQGainRatio!=1.0))
        MsgFunc(" I/Q Mismatch          : Phase=%3.1f deg, Amplitude=%4.2f dB\n",
                pState->IQAngleErrorDeg, -20.0*log(pState->IQGainRatio)/log(10.0));

    if (pState->RxCoupling_dB && pState->NumRx > 1)
        MsgFunc(" Receiver Coupling     : %1.1f dB\n",pState->RxCoupling_dB);

    return WI_SUCCESS;
}
// end of wiChanl_WriteConfig()

// ================================================================================================
// FUNCTION  : wiChanl_ParseLine
// ------------------------------------------------------------------------------------------------
// Purpose   : Parse a line of text and retrieve parameters relevent to this module
// Parameters: text -- single line of text to examine
// ================================================================================================
wiStatus wiChanl_ParseLine(wiString text)
{
    if (strstr(text, "wiChanl"))
    {
        wiChanl_State_t *pState = wiChanl_State();
        wiStatus pstatus, status, status2;

        if (!pState) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

        PSTATUS( wiParse_Boolean  (text,"wiChanl.AntennaBalance",     &(pState->AntennaBalance)) );
        PSTATUS( wiParse_Int      (text,"wiChanl.Model",              &(pState->Model),        0,      2) );
        PSTATUS( wiParse_Int      (text,"wiChanl.Channel",            &(pState->Channel),      0,      3) );
        PSTATUS( wiParse_Real     (text,"wiChanl.fDoppler",           &(pState->fDoppler),     0,   4000) );
        PSTATUS( wiParse_Boolean  (text,"wiChanl.SkipGenerateChannel",&(pState->SkipGenerateChannel)) );

        PSTATUS( wiParse_Real     (text,"wiChanl.SNR",                NULL,                   -10.0, 1000.0) );
        PSTATUS( wiParse_Real     (text,"wiChanl.Trms",               &(pState->Trms),        0.0,    0.1) );
        PSTATUS( wiParse_Int      (text,"wiChanl.N_Rx",               &(pState->NumRx),         1,      WICHANL_RXMAX) ); // old style
        PSTATUS( wiParse_Int      (text,"wiChanl.NumRx",              &(pState->NumRx),         1,      WICHANL_RXMAX) );
        PSTATUS( wiParse_Real     (text,"wiChanl.fc_kHz",             &(pState->fc_kHz),   -10000,  10000) );
        PSTATUS( wiParse_Real     (text,"wiChanl.fs_ppm",             &(pState->fs_ppm),    -1000,   1000) );
        PSTATUS( wiParse_Boolean  (text,"wiChanl.RandomPhase",        &(pState->RandomPhase)) );
        PSTATUS( wiParse_Boolean  (text,"wiChanl.TimingErrorModel",   &(pState->TimingErrorModel)) );
        PSTATUS( wiParse_Real     (text,"wiChanl.PhaseNoise.PSD",     &(pState->PhaseNoise.PSD   ), -200, 0.0) );
        PSTATUS( wiParse_Real     (text,"wiChanl.PhaseNoise.fc",      &(pState->PhaseNoise.fc    ),  1.0, 1.0E+7) );
        PSTATUS( wiParse_Real     (text,"wiChanl.SampleJitter.Jrms",  &(pState->SampleJitter.Jrms),  0.0, 1.0E-8) );
        PSTATUS( wiParse_Real     (text,"wiChanl.SampleJitter.fc",    &(pState->SampleJitter.fc  ),  0.0, 40.0E+7) );
        PSTATUS( wiParse_Real     (text,"wiChanl.SampleJitter.Jpk",   &(pState->SampleJitter.Jpk ),  0.0, 1.5E-7) );
        PSTATUS( wiParse_Real     (text,"wiChanl.SampleJitter.fHz",   &(pState->SampleJitter.fHz ),  0.0, 1.0E+7) );
        PSTATUS( wiParse_IntArray (text,"wiChanl.Channel3Dn",         pState->Channel3Dn, WICHANL_RXMAX, -160,160) );

        PSTATUS( wiParse_Boolean  (text,"wiChanl.NormalizedFading",   &(pState->NormalizedFading)));

        PSTATUS( wiParse_Boolean  (text,"wiChanl.PowerAmp.Enabled",   &(pState->PowerAmp.Enabled)) );
        PSTATUS( wiParse_Int      (text,"wiChanl.PowerAmp.Model",     &(pState->PowerAmp.Model),       2,      2));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.Amax",      &(pState->PowerAmp.Anorm),     0.0, 1000.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.Anorm",     &(pState->PowerAmp.Anorm),     0.0, 1000.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.g1",        &(pState->PowerAmp.g1),     -100.0,  100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.g3",        &(pState->PowerAmp.g3),     -100.0,  100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.g5",        &(pState->PowerAmp.g5),     -100.0,  100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.p",         &(pState->PowerAmp.p),         0.1,  100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.OBO",       &(pState->PowerAmp.OBO),       0.0,   40.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.PowerAmp.ApRapp",    &(pState->PowerAmp.ApRapp),    1.0,    2.0));

        PSTATUS( wiParse_Int      (text,"wiChanl.Interference.Model", &(pState->Interference.Model),     0,  5));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.PrdBm", &(pState->Interference.PrdBm), -200.0, 60));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.Pr_dBm",&(pState->Interference.PrdBm), -200.0, 60));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.f_MHz", &(pState->Interference.f_MHz), -100.0, 100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.t0",    &(pState->Interference.t0),      -0.1,0.1));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.pRapp", &(pState->Interference.pRapp),   0.5, 99.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.OBOdB", &(pState->Interference.OBOdB),   0.0, 20.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.ApRapp",&(pState->Interference.ApRapp),  1.0,  2.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.Pn_dBm_MHz",&(pState->Interference.Pn_dBm_MHz),-150.0,  30.0));
        PSTATUS( wiParse_Int      (text,"wiChanl.Interference.nk1",   &(pState->Interference.nk1), 0, (int)1e7));
        PSTATUS( wiParse_Int      (text,"wiChanl.Interference.nk2",   &(pState->Interference.nk2), 0, (int)1e7));

        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.Packet[1].t0", &(pState->Interference.Packet[1].t0), -0.1, 0.1));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.Packet[2].t0", &(pState->Interference.Packet[2].t0), -0.1, 0.1));
        PSTATUS( wiParse_Real     (text,"wiChanl.Interference.Packet[3].t0", &(pState->Interference.Packet[3].t0), -0.1, 0.1));

        PSTATUS( wiParse_Int      (text,"wiChanl.Radar.Model",        &(pState->Radar.Model),      0,   4));
        PSTATUS( wiParse_Int      (text,"wiChanl.Radar.N",            &(pState->Radar.N),          1, WICHANL_RADAR_MAX_N));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.W",            &(pState->Radar.W),        0.5, 100));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.PRF",          &(pState->Radar.PRF),        1,10000));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.PrdBm",        &(pState->Radar.PrdBm), -120.0, 30));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.Pr_dBm",       &(pState->Radar.PrdBm), -120.0, 30));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.f_MHz",        &(pState->Radar.f_MHz), -100.0, 100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.fRangeMHz",    &(pState->Radar.fRangeMHz), 0.5,20));
        PSTATUS( wiParse_Boolean  (text,"wiChanl.Radar.RandomStart",  &(pState->Radar.RandomStart)));
        PSTATUS( wiParse_Int      (text,"wiChanl.Radar.k0",           &(pState->Radar.k0),       (int)-1e6, (int)1e6));
        PSTATUS( wiParse_Real     (text,"wiChanl.Radar.pMissing",     &(pState->Radar.pMissing), 0.0, 1.0));
        PSTATUS( wiParse_IntArray (text,"wiChanl.Radar.dk",           pState->Radar.dk, WICHANL_RADAR_MAX_DK, -800, 800) );

        PSTATUS( wiParse_Real     (text,"wiChanl.IQAngleErrorDeg",    &(pState->IQAngleErrorDeg), -45.0, 45.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.IQGainRatio",        &(pState->IQGainRatio),       0.5,  1.0));

        PSTATUS( wiParse_Real     (text,"wiChanl.RxCoupling_dB",      &(pState->RxCoupling_dB),   -99.0,  0.0));

        PSTATUS( wiParse_Boolean  (text,"wiChanl.FixedNoiseFloor",    &(pState->FixedNoiseFloor )) );
        PSTATUS( wiParse_RealArray(text,"wiChanl.dBGain",             pState->dBGain, WICHANL_RXMAX, -100, 100));

        PSTATUS( wiParse_Boolean  (text,"wiChanl.PositionOffset.Enabled",&(pState->PositionOffset.Enabled)));
        PSTATUS( wiParse_UInt     (text,"wiChanl.PositionOffset.Range",  &(pState->PositionOffset.Range), 0, 1000));
        PSTATUS( wiParse_UInt     (text,"wiChanl.PositionOffset.dt",     &(pState->PositionOffset.dt), 0, 1000));

        PSTATUS( wiParse_Boolean  (text,"wiChanl.GainStep.Enabled",&(pState->GainStep.Enabled)));
        PSTATUS( wiParse_UInt     (text,"wiChanl.GainStep.k0",     &(pState->GainStep.k0), 0, 80000000));
        PSTATUS( wiParse_Real     (text,"wiChanl.GainStep.dB",     &(pState->GainStep.dB[0]), -100, 100));
        PSTATUS( wiParse_RealArray(text,"wiChanl.GainStep.dB",     pState->GainStep.dB, WICHANL_RXMAX, -100, 100));

        PSTATUS( wiParse_Boolean  (text,"wiChanl.AM.Enabled",      &(pState->AM.Enabled)));
        PSTATUS( wiParse_Boolean  (text,"wiChanl.AM.Linear",       &(pState->AM.Linear)));
        PSTATUS( wiParse_UInt     (text,"wiChanl.AM.k0",           &(pState->AM.k0), 0, (unsigned)1e8));
        PSTATUS( wiParse_Real     (text,"wiChanl.AM.f",            &(pState->AM.f[0]),             0, 1.0E+7));
        PSTATUS( wiParse_RealArray(text,"wiChanl.AM.f",              pState->AM.f,  WICHANL_RXMAX, 0, 1.0E+7));
        PSTATUS( wiParse_Real     (text,"wiChanl.AM.dB",           &(pState->AM.dB[0]),            0, 100.0));
        PSTATUS( wiParse_RealArray(text,"wiChanl.AM.dB",             pState->AM.dB, WICHANL_RXMAX, 0, 100.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.AM.dT",           &(pState->AM.dT[0]),            0,   1.0));
        PSTATUS( wiParse_RealArray(text,"wiChanl.AM.dT",             pState->AM.dT, WICHANL_RXMAX, 0,   1.0));
        PSTATUS( wiParse_Real     (text,"wiChanl.AM.Ramp.A",       &(pState->AM.Ramp.A),           0, 100.0));
        PSTATUS( wiParse_UInt     (text,"wiChanl.AM.Ramp.k1",      &(pState->AM.Ramp.k1), 0, (unsigned)1e8));
        PSTATUS( wiParse_UInt     (text,"wiChanl.AM.Ramp.k2",      &(pState->AM.Ramp.k2), 0, (unsigned)1e8));

        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Clear Interference(Background) Packet Waveforms
        //
        XSTATUS(status = wiParse_Command(text,"wiChanl_ClearInterferencePackets()"));
        XSTATUS(status2= wiParse_Command(text,"wiChanl_ClearBackgroundPackets()"));
        if (status == WI_SUCCESS || status2 == WI_SUCCESS)
        {
            int i, j;            

            for (i=0; i<WICHANL_MAXPACKETS; i++)
                for (j=0; j<WICHANL_RXMAX; j++)
                    pState->Interference.Packet[i].s[j].Length = 0;

            return WI_SUCCESS;
        }
    }
    return WI_WARN_PARSE_FAILED;
}
// end of wiChanl_ParseLine()

// ================================================================================================
// FUNCTION  : wiChanl_Model
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level channel model interface function
// Parameters: Nx    -- Number of samples in x
//             x     -- Input array           
//             Nr    -- Number of samples in r
//             R     -- Output array of arrays (by reference)
//             Fs    -- Sample rate (Hz)
//             PrdBm -- Receive power (dBm) into 1 Ohm
//             Prms  -- Input signal power (complex, rms)
//             Nrms  -- Noise amplitude (complex, rms)
// ================================================================================================
wiStatus wiChanl_Model( int Nx, wiComplex *X[], int Nr, wiComplex **R[], double Fs, double PrdBm, 
                        double Prms, double Nrms )
{
    wiChanl_State_t *pState = wiChanl_State();
    wiComplex *Y[WICHANL_TXMAX];
    int iRx, NumTx;

    if ((wiProcess_ThreadIndex() != 0) && !pState) // Create the thread state if it does not exist
    {
        XSTATUS( wiChanl_InitializeThread() );
        pState = wiChanl_State();
    }
    // Error/Range Checking
    //
    if (!pState)                    return WI_ERROR_MODULE_NOT_INITIALIZED;
    if (InvalidRange(Nx, 1, 1<<28)) return WI_ERROR_PARAMETER1;
    if (X == NULL)                  return WI_ERROR_PARAMETER2;
    if (InvalidRange(Nr, 1, 1<<28)) return WI_ERROR_PARAMETER3;
    if (R == NULL)                  return WI_ERROR_PARAMETER4;
    if (!Fs)                        return WI_ERROR_PARAMETER5;
    if (!Prms)                      return WI_ERROR_PARAMETER6;

    // Determine the number of transmit streams
    //
    for (NumTx = 0; (NumTx <= WICHANL_TXMAX && X[NumTx]); NumTx++ ) ;
    if (NumTx > WICHANL_TXMAX) return STATUS(WI_ERROR_RANGE);

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Configure and Store Model Parameters
    //
    //  Receive power in PrdBm is converted to gain based on the reported input signal level.
    //  If the gain is within 0.01% of 1, it is forced to exactly 1 with the assumuption that
    //  unity gain was intended. Noise power is then scaled according to any signal gain to
    //  accomodate SNR based models.
    //
    pState->A = sqrt(pow(10.0, (PrdBm-30)/10) / Prms);
    if (fabs(log10(pState->A)) < 0.0001) pState->A = 1.0;
    
    pState->Fs   = Fs;
    pState->Nrms = Nrms * pState->A;

    for (iRx=0; iRx<pState->NumRx; iRx++) 
        pState->Av[iRx] = pState->A * pow(10.0, pState->dBGain[iRx]/20.0);

    if (pState->NumRx==2 && pState->AntennaBalance)
    {
        wiReal w = 2*pi*wiRnd_UniformSample();
        pState->Av[0] *= fabs(cos(w));
        pState->Av[1] *= fabs(sin(w));
    }
    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate a position offset for the start of packet. This model allows the starting
    //  position to be varied randomly by integer samples to avoid "traps" where a design
    //  exhibits a certain behavior for a single particular phase within the waveform.
    //
    XSTATUS( wiChanl_GetPositionOffset(pState) );

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Record the Packet for Interference Models
    //  The parameters PrdBm, f_MHz, and t0 are taken from the top-level interference
    //  parameters for backwards compatibility
    // 
    switch ( pState->Interference.Model )
    {
        case WICHANL_INTERFERENCE_LAST_PACKET:
        case WICHANL_INTERFERENCE_NONLINEAR_LAST_PACKET:
        {
            XSTATUS( wiChanl_LoadPacket( Nx, X, Fs, Prms, -1, pState->Interference.PrdBm, 
                                         pState->Interference.f_MHz, pState->Interference.t0) );
            break;
        }
        default: break;
    }

    // Allocate memory for the output array(s)
    // ---------------------------------------
    for (iRx=0; iRx<pState->NumRx; iRx++)
    {
        GET_WIARRAY( pState->r[iRx], pState->R[iRx], Nr, wiComplex );
		memset(pState->r[iRx], 0, Nr*sizeof(wiComplex));
    }
    for ( ; iRx<WICHANL_RXMAX; iRx++)
    {
        pState->r[iRx] = NULL;
    }
    pState->Nr = Nr;

    //////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Channel Model
    //
    //  The core model includes a power amplifier nonlinearity model. Generally, nonlinearity
    //  is implemented in the radio model, but this is available as an alternative. A choice 
    //  is made between several models. Once the channel TX-to-RX mapping is complete,
    //  coupling between RX antennas is modeled. Finally, quadrature mismatch is applied to
    //  the signals.
    //
    XSTATUS( wiChanl_PowerAmp(NumTx, Nx, X, Y, Prms) );

    switch (pState->Model) // TX-to-RX channel model
    {
        case 0 :
        case 1 : 
            XSTATUS( wiChanl_GenerateChannelResponse(NumTx, pState->NumRx) );

            for (iRx=0; iRx<pState->NumRx; iRx++)
                XSTATUS( wiChanl_MultipathCore(NumTx, Nx, X, Nr, pState->R[iRx].ptr, iRx, pState->PositionOffset.t0) );
            break;

        #ifdef __WITGNCHANL_H
        case 2 :
            XSTATUS( wiTGnChanl(Nx, X, Nr, pState->r, NumTx, pState->NumRx, pState->Fs) );
            break;
        #endif

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

    for (iRx=0; iRx<pState->NumRx; iRx++)
    {
        wiComplex *r = pState->R[iRx].ptr;

        XSTATUS( wiChanl_ScaleReceivedSignal(Nr, r, iRx) );
        XSTATUS( wiChanl_InterferenceModel  (Nr, r, iRx) );
        XSTATUS( wiChanl_DFS_RadarModel     (Nr, r, iRx) );
        XSTATUS( wiChanl_AmplitudeModulation(Nr, r, iRx) );
        XSTATUS( wiChanl_AddNoise           (Nr, r     ) );
        XSTATUS( wiChanl_PhaseFrequencyError(Nr, r, iRx) );
        XSTATUS( wiChanl_Resample           (Nr, r, iRx) );
    }
    XSTATUS( wiChanl_RxCoupling(pState) );
    XSTATUS( wiChanl_IQMismatch(pState) );
    XSTATUS( wiChanl_GainStep  (pState) );

    if (R) *R = pState->r; // Return a pointer to the internal output matrix 

    return WI_SUCCESS;
}
// end of wiChanl_Model()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

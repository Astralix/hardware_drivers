//--< Coltrane.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  Baseband Equivalent Model for a 1x2 Radio
//  Copyright 2005-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "Coltrane.h"
#include "wiFilter.h"
#include "wiRnd.h"
#include "wiMath.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static Coltrane_TxState_t TX[WISE_MAX_THREADS] = {{0}}; // transmitter state
static Coltrane_RxState_t RX[WISE_MAX_THREADS] = {{0}}; // receiver state
static wiBoolean ModuleIsInit = 0;                    // module has been initialized

static const double kBoltzmann = 1.380658e-23; // Boltzmann's constant
static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  FUNCTION PROTOTPYES
//=================================================================================================
wiStatus Coltrane_RX_ClearFilterMemory(void);
wiStatus Coltrane_ParseLine(wiString text);
wiStatus Coltrane_TxIQLoopBack(wiComplex *rOut, wiInt n);

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;

// DB20 -- Convert voltage gain to dB
// SQR  -- square = x^2
#ifndef DB20
#define DB20(Av) (20.0*log(Av)/log(10.0))
#endif

#ifndef SQR
#define SQR(x)   ((x)*(x))
#endif

// Pseudonym for the add noise sample function
#define Coltrane_AddNoise(x, Nrms) wiRnd_AddComplexNormal(x, Nrms)

// ================================================================================================
// FUNCTION  : Coltrane_LgSigDetector
// ------------------------------------------------------------------------------------------------
// Purpose   : Large signal detector
// Parameters: x          -- complex input signal
//             ThLgSigAFE -- threshold for detection
//             LgSigAFE   -- detector output
// ================================================================================================
__inline void Coltrane_LgSigDetector(wiComplex *x, wiReal ThLgSigAFE, wiInt *LgSigAFE)
{
    *LgSigAFE = ((fabs(x->Real) > ThLgSigAFE) || (fabs(x->Imag) > ThLgSigAFE)) ? 1:0;
}
// end of Coltrane_LgSigDetector()

// ================================================================================================
// FUNCTION  : Coltrane_AddOffset
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a DC term to the I and Q components
// Parameters: x -- complex I/O signal
//             c -- offset amplitude
// ================================================================================================
__inline void Coltrane_AddOffset(wiComplex *x, wiReal c)
{
    x->Real += c;
    x->Imag += c;
}
// end of Coltrane_AddOffset()

// ================================================================================================
// FUNCTION  : Coltrane_Amplifier
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a simple gain term for the complex signal.
// Parameters: x -- complex I/O signal
//             A -- amplifier gain (V/V)
// ================================================================================================
__inline void Coltrane_Amplifier(wiComplex *x, wiReal A)
{
    x->Real *= A;
    x->Imag *= A;
}
// end of Coltrane_Amplifier()

// ================================================================================================
// FUNCTION  : Coltrane_NonlinAmp
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement an amplifier with 2nd and 3rd order nonlinearity
// Parameters: x -- complex I/O signal
//             G -- gain structure
// ================================================================================================
__inline void Coltrane_NonlinAmp(wiComplex *x, Coltrane_GainState_t *G)
{
    // -----------
    // Linear Gain
    // -----------
    x->Real *= G->Av;
    x->Imag *= G->Av;

    // ---------------
    // Amplitude Limit
    // ---------------
    if (x->Real<G->Vmin) x->Real = G->Vmin;
    if (x->Real>G->Vmax) x->Real = G->Vmax;
    if (x->Imag<G->Vmin) x->Imag = G->Vmin;
    if (x->Imag>G->Vmax) x->Imag = G->Vmax;

    // -----------------------
    // Polynomial Nonlinearity
    // -----------------------
    {
        wiReal x2, x3;

        x2 = x->Real * x->Real; x3 = x2 * x->Real;
        x->Real += (G->c2*x2 + G->c3*x3);

        x2 = x->Imag * x->Imag; x3 = x2 * x->Imag;
        x->Imag += (G->c2*x2 + G->c3*x3);
    }
}
// end of Coltrane_NonlinAmp()

// ================================================================================================
// FUNCTION  : Coltrane_TX_AC_Pole
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a single AC coupling pole (fixed position)
// Parameters: x    -- i/o signal (by reference)
//             n    -- signal path index
//             pole -- pole filter structure (by reference)
// ================================================================================================
__inline void Coltrane_TX_AC_Pole(wiComplex *x, wiInt n, Coltrane_PoleState_t *pole)
{
    pole->w[n][1] = pole->w[n][0];

    pole->w[n][0].Real = x->Real - (pole->a[0] * pole->w[n][1].Real);
    pole->w[n][0].Imag = x->Imag - (pole->a[1] * pole->w[n][1].Imag);

    x->Real = pole->b[0] * (pole->w[n][0].Real - pole->w[n][1].Real);
    x->Imag = pole->b[1] * (pole->w[n][0].Imag - pole->w[n][1].Imag);
}
// end of Coltrane_TX_AC_Pole()

// ================================================================================================
// FUNCTION  : Coltrane_RX_AC_Pole
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a single AC coupling pole
// Parameters: x    -- i/o signal (by reference)
//             n    -- signal path index
//             pole -- pole filter structure (by reference)
//             t    -- fraction of time in gain switch (high for 1>=t>0, nominal @ t=0)
// ================================================================================================
void Coltrane_RX_AC_Pole(wiComplex *x, wiInt n, Coltrane_PoleState_t *pole, wiReal t)
{
    double a[2],b[2];
    wiComplex d;
 
    Coltrane_RxState_t *pRX = Coltrane_RxState();

    // --------------------------------
    // Set Filter Cutoff (Dynamic Pole)
    // --------------------------------
    if (t && pole->fc_kHz) // fast pole
    {
        if (pRX->AC.Tslide && (t<=pRX->AC.t1))
        {
            double fc_kHz, beta;
            fc_kHz = (t/(double)pRX->AC.t1)*(pRX->AC.fckHz_Slide - pole->fc_kHz) + pole->fc_kHz;
            beta = tan(3.14159265 * fc_kHz / (1000.0*pRX->Fs_MHz));

            b[0] = b[1] = 1.0/(1.0+beta);
            a[0] = a[1] = (beta-1.0)/(beta+1.0);
        }
        else
        {
            a[0] = a[1] = b[0] = b[1] = 0.0; // inf BW
        }
    }
    else // nominal pole
    {
        a[0] = pole->a[0];  a[1] = pole->a[1];
        b[0] = pole->b[0];  b[1] = pole->b[1];
    }
    // ---------------------------
    // Filter Difference Equations
    // ---------------------------
    d.Real = x->Real - pole->xD[n][0].Real;
    d.Imag = x->Imag - pole->xD[n][0].Imag;

    pole->xD[n][0].Real = x->Real; pole->xD[n][0].Imag = x->Imag;

    x->Real = b[0]*d.Real - a[0]*pole->yD[n][0].Real;
    x->Imag = b[1]*d.Imag - a[1]*pole->yD[n][0].Imag;

    pole->yD[n][0].Real = x->Real; pole->yD[n][0].Imag = x->Imag;
}
// end of Coltrane_AC_Pole()

// ================================================================================================
// FUNCTION  : Coltrane_Filter
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the "ideal" bandpass filter
// Parameters: n      -- signal path index
//             t      -- time sample index
//             Filter -- filter state structure (by reference)
//             x      -- i/o signal (by reference)
// ================================================================================================
__inline wiStatus Coltrane_Filter(wiComplex *x, Coltrane_FilterState_t *Filter, int n)
{
    wiComplex  z = {0,0};
    wiComplex *w = Filter->w[n];
    int        N = Filter->N+1;
    int k;

    // ------
    // Filter
    // ------
    w[0].Real = x->Real;
    w[0].Imag = x->Imag;

    for (k=1; k<N; k++)
    {
        w[0].Real -= Filter->a[0][k] * w[k].Real;
        w[0].Imag -= Filter->a[1][k] * w[k].Imag;
    }
    for (k=0; k<N; k++)
    {
        z.Real += Filter->b[0][k] * w[k].Real;
        z.Imag += Filter->b[1][k] * w[k].Imag;
    }
    // ---------------------
    // Advance Filter Memory
    // ---------------------
    for (k=N; k>0; k--)
    {
        w[k].Real = w[k-1].Real;
        w[k].Imag = w[k-1].Imag;
    }
    x->Real = z.Real;
    x->Imag = z.Imag;

    return WI_SUCCESS;
}
// end of Coltrane_Filter()

// ================================================================================================
// FUNCTION  : Coltrane_TX_Mixer()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the transmit mixer
// Parameters: Nx -- number of samples in x
//             x  -- input/output signal array
// ================================================================================================
wiStatus Coltrane_TX_Mixer(int Nx, wiComplex *x)
{
    Coltrane_TxState_t *pTX = Coltrane_TxState();
    int k;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  I/Q Quadrature Mismatch Model
    //  The sign on pTX->Mixer.Gain_dB is needed so that radio and baseband DAC
    //  gain errors with the same sign add constructively.
    //
    if (pTX->Mixer.Gain_dB || pTX->Mixer.PhaseDeg)
    {
        wiReal eps = (pow(10.0, pTX->Mixer.Gain_dB/20.0)-1)/(pow(10.0, pTX->Mixer.Gain_dB/20.0)+1);
        wiReal phi = pi/180.0 * pTX->Mixer.PhaseDeg;
        wiReal c11 =  (1 + eps) * cos(phi/2);
        wiReal c12 = -(1 - eps) * sin(phi/2);
        wiReal c21 = -(1 + eps) * sin(phi/2);
        wiReal c22 =  (1 - eps) * cos(phi/2);
        wiReal temp;

        for (k=0; k<Nx; k++)
        {
            temp      = c11 * x[k].Real + c12 * x[k].Imag;
            x[k].Imag = c21 * x[k].Real + c22 * x[k].Imag;
            x[k].Real = temp;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //
    //  LO Phase Noise and CFO
    //  x[k] = x[k] * exp(j*a[k])
    //
    if (pTX->PLL.Enabled)
    {
        wiComplex y, c;
        wiReal *a = pTX->PLL.a.ptr;

        for (k=0; k<Nx; k++)
        {
            c.Real = cos(a[k]);
            c.Imag = sin(a[k]);

            y = x[k];
            x[k].Real = (c.Real * y.Real) - (c.Imag * y.Imag);
            x[k].Imag = (c.Imag * y.Real) + (c.Real * y.Imag);
        }
    }
    return WI_SUCCESS;
}
// end of Coltrane_TX_Mixer()

// ================================================================================================
// FUNCTION  : Coltrane_PowerAmp()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the power amplifier using the Rapp model
// Parameters: Pin -- input power
//             Nx  -- number of samples in x
//             x   -- input/output signal array
// ================================================================================================
wiStatus Coltrane_PowerAmp(wiReal Pin, int Nx, wiComplex *x)
{
    Coltrane_TxState_t *pTX = Coltrane_TxState();

    // ---------------------------
    // Power Amplifier: Rapp Model
    // ---------------------------
    if (pTX->PowerAmp.Enabled || pTX->ModelLevel==2)
    {
        double p = pTX->PowerAmp.p;
        double Psat, Av;
        int k;

        Psat = pow(10.0,pTX->PowerAmp.OBO/10.0) * Pin / pow(pow(10,p/10)-1,1/p);

        for (k=0; k<Nx; k++)
        {
            Pin = (x[k].Real * x[k].Real) + (x[k].Imag * x[k].Imag);
            Av = Pin? sqrt(pTX->PowerAmp.Ap / pow(1 + pow(Pin/Psat, p), 1/p)) : sqrt(pTX->PowerAmp.Ap);

            x[k].Real *= Av;
            x[k].Imag *= Av;
        }
    }
    return WI_SUCCESS;
}
// end of Coltrane_PowerAmp()

// ================================================================================================
// FUNCTION  : Coltrane_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the analog TX radio
// Parameters: Ns   -- number of samples in the i/o arrays
//             sIn  -- input signal array (DAC output)
//             sOut -- output signal array (antenna output)
// ================================================================================================
wiStatus Coltrane_TX(int Ns, wiComplex *sIn, wiComplex *sOut, int nPadBack)
{
    Coltrane_TxState_t *pTX = Coltrane_TxState();

    double A = 1.0/sqrt(pTX->Prms);
    int k;

    // -------------------
    // Normal Output Power
    // -------------------
    if (pTX->ModelLevel < 2)
    {
        for (k=0; k<Ns; k++) 
        {
            sOut[k].Real = A * sIn[k].Real;
            sOut[k].Imag = A * sIn[k].Imag;
        }
    }
    // ---------------------------------------
    // Coltrane TX Models by Complexity Level
    // ---------------------------------------
    switch (pTX->ModelLevel)
    {
        case 0:
        {
            Coltrane_TX_Mixer(Ns, sOut);
            Coltrane_PowerAmp(1.0, Ns, sOut);
        }  break;

        case 1:
        {
            for (k=0; k<Ns; k++) {
                Coltrane_Filter(sOut+k, &(pTX->Filter), 0);
            }
            if (pTX->AC.Enabled) {
                for (k=0; k<Ns; k++) {
                    Coltrane_TX_AC_Pole(sOut+k, 0, pTX->AC.s+0);
                    Coltrane_TX_AC_Pole(sOut+k, 0, pTX->AC.s+1);
                }
            }
            Coltrane_TX_Mixer(Ns, sOut);
            Coltrane_PowerAmp(1.0, Ns, sOut);
        }  break;

        case 2:
        {
            for (k=0; k<Ns; k++)
            {
                // Transmit Filter
                sOut[k] = sIn[k];
                Coltrane_TX_AC_Pole  (sOut+k, 0, pTX->AC.s+0);
                Coltrane_AddNoise (sOut+k, pTX->Filter.Nrms/sqrt(2.0));
                Coltrane_Filter   (sOut+k, &(pTX->Filter), 0);
                Coltrane_AddNoise (sOut+k, pTX->Filter.Nrms/sqrt(2.0));

                // TX Mixer
                Coltrane_TX_AC_Pole(sOut+k, 0, pTX->AC.s+1);
                Coltrane_AddNoise  (sOut+k, pTX->Mixer.Nrms);
            }
            // -----------------
            // Startup-Transient
            // -----------------
            if (pTX->Startup.tau) 
            {
                double Av;
                for (k=0; k<Ns; k++)
                {
                    Av = (k<pTX->Startup.k0)? 0.0 : 1.0 - exp(-(k-pTX->Startup.k0)/pTX->Startup.ktau);
                    sOut[k].Real *= Av;
                    sOut[k].Imag *= Av;
                }
            }
            // ---------------
            // Mixer/Predriver
            // ---------------
            Coltrane_TX_Mixer(Ns, sOut);

            // ----------------
            // Normalize Output
            // ----------------
            for (k=0; k<Ns; k++) 
            {
                sOut[k].Real *= A;
                sOut[k].Imag *= A;
            }
            Coltrane_PowerAmp (1.0, Ns, sOut);
        }  break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // --------------------------
    // Squelch Pad Samples at End
    // --------------------------
    {
        int k0 = Ns-nPadBack + (int)(pTX->Fs_MHz * 1e6 * pTX->PowerAmp.DelayShutdown);
        for (k=k0; k<Ns; k++)
            sOut[k].Real = sOut[k].Imag = 0.0;
    }
    return WI_SUCCESS;
}
// end of Coltrane_TX()

// ================================================================================================
// FUNCTION  : Coltrane_TxIQLoopBack()
// ------------------------------------------------------------------------------------------------
// Purpose   : Simulate the loop-back path using Envelope Detector for Tx I/Q Calibration.
// Parameters: rOut -- output signal to ADC input
//             n    -- antenna path {0,1}
// ================================================================================================
wiStatus Coltrane_TxIQLoopBack(wiComplex *rOut, wiInt n)
{
    const double  edLPF_a[] = {1.0000, -1.0422, 0.3706};
    const double  edLPF_b[] = {0.0821,  0.1642, 0.0821};
    const double  edHPF_a[] = {1.0000, -0.9615};
    const double  edHPF_b[] = {0.9807, -0.9807};
    const double  noamp     = 5.0238*1e-7;
    const double  dc        = 3e-3;

    Coltrane_TxIQLoopback_t *pState = &(Coltrane_RxState()->TxIQLoopback);

    if ( (pState->NumRx == 1) || (pState->NumRx == 2 && n == 0) )
    {
        wiReal env, lpfOut, hpfOut;

        //////////////////////////////////////////////////////////////
        //
        // Envelope Detector
        //        
        env = sqrt(rOut->Real*rOut->Real + rOut->Imag*rOut->Imag);
        env += wiRnd_NormalSample(sqrt(noamp));

        // LPF
        //
        lpfOut = edLPF_b[0] * env + edLPF_b[1] * pState->lpfBuffer1[0] + edLPF_b[2] * pState->lpfBuffer1[1]
            - edLPF_a[1] * pState->lpfBuffer2[0] - edLPF_a[2] * pState->lpfBuffer2[1];    

        pState->lpfBuffer1[1] = pState->lpfBuffer1[0];
        pState->lpfBuffer1[0] = env;

        pState->lpfBuffer2[1] = pState->lpfBuffer2[0];
        pState->lpfBuffer2[0] = lpfOut;

        // HPF
        //
        hpfOut = edHPF_b[0] * lpfOut + edHPF_b[1] * pState->hpfBuffer1[0] - edHPF_a[1] * pState->hpfBuffer2[0];

        pState->hpfBuffer1[0] = lpfOut;
        pState->hpfBuffer2[0] = hpfOut;

        pState->edOut.Real = pState->scaling * (hpfOut + dc);
        pState->edOut.Imag = 0;
    }

    *rOut = pState->edOut;

    return WI_SUCCESS;
}
// end of Coltrane_TxIQLoopBack()

// ================================================================================================
// FUNCTION  : Coltrane_RX_Mixer
// ------------------------------------------------------------------------------------------------
// Purpose   : Model RX mixer effects such as phase noise and I/Q mismatch
// Parameters: x -- complex I/O signal
//             t -- sample time index
// ================================================================================================
void Coltrane_RX_Mixer(wiComplex *x, wiInt t)
{
    Coltrane_RxState_t *pRX = Coltrane_RxState();

    // ------------------
    // Mixer: Phase Noise
    // ------------------
    if (pRX->PLL.Enabled)
    {
        wiComplex y, c; 
        wiReal *a = pRX->PLL.a.ptr;

        // --------------------------
        // Compute: x = x*exp(j*a[t])
        // --------------------------
        c.Real = cos(a[t]);
        c.Imag = sin(a[t]);

        y = *x;
        x->Real = (c.Real * y.Real) - (c.Imag * y.Imag);
        x->Imag = (c.Imag * y.Real) + (c.Real * y.Imag);
    }
    // ------------------
    // Mixer I/Q Mismatch
    // ------------------
    {
        x->Imag = (pRX->Mixer.cI * x->Real) + (pRX->Mixer.cQ * x->Imag);
    }
}
// end of Coltrane_RX_Mixer()

// ================================================================================================
// FUNCTION  : Coltrane_RX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the combined effect of the analog VGA and LPF
// Parameters: n       -- antenna path {0,1}
//             t       -- sample time index
//             rIn     -- input signal from antenna (power/1Ohm)
//             rOut    -- output signal to ADC input
//             SigIn   -- input signals from baseband
//             SigOut  -- output signals to baseband
// ================================================================================================
wiStatus Coltrane_RX(int n, int t, wiComplex rIn, wiComplex *rOut, Coltrane_SignalState_t SigIn, Coltrane_SignalState_t *SigOut)
{
    Coltrane_RxState_t *pRX = Coltrane_RxState();
    double A = 1;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Non-RX and Mode Transition
    // 
    //  When not in RX mode, simply output a zero value. However, on a transition into
    //  RX mode, clear the filter memories so the model is in a consistent, known state.
    //  Set dLNAGain = 99 on a transition so then when entering RX, dynamic AC coupling
    //  will be triggered.
    //
    if (SigIn.MC != pRX->dMC)
    {
        pRX->dLNAGain = 99;
        XSTATUS( Coltrane_RX_ClearFilterMemory() );
    }
    pRX->dMC = SigIn.MC;

    if (SigIn.MC != 3)
    {
        rOut->Real = rOut->Imag = 0.0;
        return WI_SUCCESS;
    }
    // ---------------------------------
    // 20 MHz Clock for Sequential Logic
    // ---------------------------------
    if (!n)
    {
        if ((t % pRX->M_20MHz)==0) pRX->regLgSigAFE.Q = pRX->regLgSigAFE.D;

        pRX->GainChange = (SigIn.AGAIN!=pRX->dAGain || SigIn.LNAGAIN != pRX->dLNAGain)? 1:0;
        if (!(t%4)) {pRX->dAGain = SigIn.AGAIN; pRX->dLNAGain=SigIn.LNAGAIN;}
    }
    // ------------------------
    // Dynamic AC Coupling Pole
    // ------------------------
    if ((t<16 || pRX->GainChange) && pRX->AC.Dynamic) 
    {
        double Fs = pRX->Fs_MHz * 1e6;
        pRX->AC.t1   = floor((pRX->AC.Tslide                ) * Fs + 0.5); // time to start slide
        pRX->xACpole = floor((pRX->AC.Tstep + pRX->AC.Tslide) * Fs + 0.5); // fast pole period
        pRX->xRFattn = floor((pRX->Squelch.Tstep            ) * Fs + 0.5); // RF squelch period
    }
    else
    {
        if (pRX->xRFattn && !n) pRX->xRFattn--;
        if (pRX->xACpole && !n) pRX->xACpole--;
    }
    // --------------------------------------------------
    // LNA Input: Signal with RFE Gain + Noise (1 Ohm,ref)
    // --------------------------------------------------
    {
        wiReal Av = SigIn.SW1? (pRX->RFE.Av[n] * 0.1) : pRX->RFE.Av[n]; // model -20 dB if SW1 out of position
        Av = pRX->RFE.Av[n];
        rOut->Real = (Av * rIn.Real);
        rOut->Imag = (Av * rIn.Imag);

        wiRnd_AddComplexNormal(rOut, pRX->Nrms);
    }
    // ---------------------------------------
    // Calculate Net Gain for ModelLevel != 2)
    // ---------------------------------------
    if (pRX->ModelLevel != 2)
    {
        A  = pRX->RF.sqrtRin;
        A *= pRX->RF.Gain[SigIn.LNAGAIN].Av * sqrt(2.0);
        A *= pRX->SGA.Gain[SigIn.AGAIN].Av;
        A *= pRX->Filter.Gain.Av;
        A *= pRX->PGA.Gain.Av;
        A *= pRX->ADCBuf.Gain.Av;
        if (pRX->LNA2.Enabled) A *= pRX->LNA2.Av[SigIn.LNAGAIN2];
    }
    // ------------------------------------
    // Coltrane Models by Complexity Level
    // ------------------------------------
    switch (pRX->ModelLevel)
    {
        // ---------------------
        // Gain Model Only + LPF
        // ---------------------
        case 0:
            rOut->Real *= A;
            rOut->Imag *= A;
            break;

        // -------------------
        // Gain with Ideal BPF
        // -------------------
        case 1:
            rOut->Real *= A;
            rOut->Imag *= A;

            if (pRX->PLL.Enabled) 
            {
                Coltrane_RX_Mixer (rOut, t);
            }
            if (pRX->AC.Enabled)
            {
                Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+0, pRX->xACpole);
                Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+1, pRX->xACpole);
            }
            Coltrane_Filter(rOut, &(pRX->Filter), n);

            Coltrane_LgSigDetector(rOut, pRX->ThLgSigAFE*pRX->PGA.Gain.Av, &(pRX->regLgSigAFE.D.word));

            if (pRX->AC.Enabled) 
                Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+2, pRX->xACpole);
            break;

        // ------------------------------
        // Full Baseband Equivalent Model
        // ------------------------------
        case 2:
        {
            // -----------------------------
            // Power-to-Voltage at LNA Input
            // -----------------------------
            Coltrane_Amplifier(rOut, pRX->RF.sqrtRin);

            // ------------
            // External LNA
            // ------------
            if (pRX->LNA2.Enabled)
            {
                Coltrane_AddNoise (rOut, pRX->LNA2.Nrms[SigIn.LNAGAIN2]);
                Coltrane_Amplifier(rOut, pRX->LNA2.Av  [SigIn.LNAGAIN2]);
            }
            // ----------------
            // LNA + Mixer (RF)
            // ----------------
            Coltrane_AddOffset(rOut, pRX->RF.Vo);                                   // Offset due to coupled LO
            if (pRX->xRFattn) Coltrane_Amplifier(rOut, pRX->Squelch.Gain[SigIn.LNAGAIN]); // RF Squelch
            Coltrane_AddNoise (rOut, pRX->RF.Nrms[SigIn.LNAGAIN]);
            Coltrane_Amplifier(rOut, sqrt(2.0));                 // adjust gain to reflect real-to-complex gain
            Coltrane_NonlinAmp(rOut, pRX->RF.Gain+SigIn.LNAGAIN);
            Coltrane_RX_Mixer (rOut, t);
            Coltrane_AddOffset(rOut, pRX->Mixer.Vo);

            // ---
            // SGA
            // ---
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+0, pRX->xACpole);
            Coltrane_AddOffset(rOut, pRX->SGA.Vo);
            Coltrane_AddNoise (rOut, pRX->SGA.Nrms[SigIn.AGAIN]);
            Coltrane_NonlinAmp(rOut, pRX->SGA.Gain+SigIn.AGAIN);
            if (n==0) pRX->trace[t] = *rOut;

            // ---------------------
            // Channel Select Filter
            // ---------------------
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+1, pRX->xACpole);
            Coltrane_AddOffset(rOut, pRX->Filter.Vo);
            Coltrane_AddNoise (rOut, pRX->Filter.Nrms_3dB);
            Coltrane_NonlinAmp(rOut,&pRX->Filter.Gain);
            Coltrane_Filter   (rOut,&pRX->Filter, n);
            Coltrane_AddNoise (rOut, pRX->Filter.Nrms_3dB);

            // ---------------------
            // Large Signal Detector
            // ---------------------
            Coltrane_LgSigDetector(rOut, pRX->ThLgSigAFE, &(pRX->regLgSigAFE.D.word));

            // --------------
            // PGA Gain Stage
            // --------------
            Coltrane_AddOffset(rOut, pRX->PGA.Vo);
            Coltrane_AddNoise (rOut, pRX->PGA.Nrms);
            Coltrane_NonlinAmp(rOut,&pRX->PGA.Gain);
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+2, pRX->xACpole);

            // ----------------
            // ADC Buffer Stage
            // ----------------
            Coltrane_AddOffset(rOut, pRX->ADCBuf.Vo);
            Coltrane_AddNoise (rOut, pRX->ADCBuf.Nrms);
            Coltrane_NonlinAmp(rOut,&pRX->ADCBuf.Gain);

            Coltrane_TX_AC_Pole(rOut, n, pRX->AC.s+3); // this pole is at the input of the Baseband ADC
            break;
        }

        // ------------------------------------------------
        // Filter Only with Single AC coupling (for BERLAB)
        // ------------------------------------------------
        case 4:
            rOut->Real = A * pRX->RFE.Av[n] * rIn.Real;
            rOut->Imag = A * pRX->RFE.Av[n] * rIn.Imag;
            Coltrane_TX_AC_Pole(rOut, n, pRX->AC.s+0);
            Coltrane_Filter (rOut, &(pRX->Filter), n);
            break;

        // -------------------------------------
        // Offset Only for Baseband Verification
        // -------------------------------------
        case 5:
            switch (n)
            {
                case 0:
                    rOut->Real = pRX->ADCBuf.Vo5AI;
                    rOut->Imag = pRX->ADCBuf.Vo5AQ;
                    break;

                case 1:
                    rOut->Real = pRX->ADCBuf.Vo5BI;
                    rOut->Imag = pRX->ADCBuf.Vo5BQ;
                    break;
            }
            break;

        // -----------------------------------------------
        // Modeling Loop-back path for Tx I/Q Calibration
        // -----------------------------------------------
        case 6:

            *rOut = rIn;
            Coltrane_TxIQLoopBack(rOut, n);

            // ---
            // SGA
            // ---
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+0, pRX->xACpole);
            Coltrane_AddOffset(rOut, pRX->SGA.Vo);
            Coltrane_AddNoise (rOut, pRX->SGA.Nrms[SigIn.AGAIN]);
            Coltrane_NonlinAmp(rOut, pRX->SGA.Gain+SigIn.AGAIN);
            if (n==0) pRX->trace[t] = *rOut;

            // ---------------------
            // Channel Select Filter
            // ---------------------
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+1, pRX->xACpole);
            Coltrane_AddOffset(rOut, pRX->Filter.Vo);
            Coltrane_AddNoise (rOut, pRX->Filter.Nrms_3dB);
            Coltrane_NonlinAmp(rOut,&pRX->Filter.Gain);
            Coltrane_Filter   (rOut,&pRX->Filter, n);
            Coltrane_AddNoise (rOut, pRX->Filter.Nrms_3dB);

            // --------------
            // PGA Gain Stage
            // --------------
            Coltrane_AddOffset(rOut, pRX->PGA.Vo);
            Coltrane_AddNoise (rOut, pRX->PGA.Nrms);
            Coltrane_NonlinAmp(rOut,&pRX->PGA.Gain);
            Coltrane_RX_AC_Pole(rOut, n, pRX->AC.s+2, pRX->xACpole);

            // ----------------
            // ADC Buffer Stage
            // ----------------
            Coltrane_AddOffset(rOut, pRX->ADCBuf.Vo);
            Coltrane_AddNoise (rOut, pRX->ADCBuf.Nrms);
            Coltrane_NonlinAmp(rOut,&pRX->ADCBuf.Gain);

            Coltrane_TX_AC_Pole(rOut, n, pRX->AC.s+3); // this pole is at the input of the Baseband ADC
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // ------------
    // Wire Outputs
    // ------------
    SigOut->LGSIGAFE = pRX->regLgSigAFE.Q.b0;

    return WI_SUCCESS;
}
// end of Coltrane_RX()

// ================================================================================================
// FUNCTION  : Coltrane_PhaseFrequencyError()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the baseband equivalent mixer for RF mixer phase/frequency errors
// Parameters: N      - number of time points
//             Fs_MHz - sample rate in MS/s
//             pState - pointer to the PLL state structure
// ================================================================================================
wiStatus Coltrane_PhaseFrequencyError(wiInt N, wiReal Fs_MHz, Coltrane_PllState_t *pPLL)
{
    if (pPLL->Enabled)
    {
        wiInt  k;
        wiReal B[8], A[8], Su, fc, fs;
        wiReal *a, *u;

        GET_WIARRAY( a, pPLL->a, N, wiReal );
        GET_WIARRAY( u, pPLL->u, N, wiReal );

        fs = Fs_MHz * 1.0E+6;
        Su = sqrt(fs) * pow(10.0, pPLL->PSD/20.0);
        fc = pPLL->fc/fs;

        XSTATUS(wiRnd_Normal(N, u, Su));
        XSTATUS(wiFilter_Butterworth(pPLL->FilterN, fc, B, A, WIFILTER_IMPULSE_INVARIANCE));
        XSTATUS(wiFilter_IIR(pPLL->FilterN, B, A, N, u, a, NULL));

        // ---------------------
        // Add frequency offsets
        // ---------------------
        if (pPLL->CFO_ppm || pPLL->Transient.PeakCFO_ppm)
        {
            wiReal w = 2.0 * pi * pPLL->CFO_ppm * 1e-6 * pPLL->f_MHz / Fs_MHz;

            if (pPLL->Transient.PeakCFO_ppm)
            {
                wiReal dw  = 2.0 * pi * pPLL->Transient.PeakCFO_ppm * 1e-6 * pPLL->f_MHz / Fs_MHz;
                wiReal wc  = 2.0 * pi * (pPLL->Transient.fc/1.0E+6) / Fs_MHz;
                wiReal tau = (pPLL->Transient.tau*1.0E+6) * Fs_MHz;
                wiInt  k0  = pPLL->Transient.k0;
                wiInt  dk;

                for (k=0; k<k0; k++)
                    a[k] += (double)k * w;

                if (tau) 
                {
                    for (; k<N; k++) {
                        dk = k - k0;
                        a[k] += (double)k * (w + dw * sin(wc*dk) * exp(-(double)dk/tau));
                    }
                }
                else 
                {
                    for (; k<N; k++) {
                        dk = k - k0;
                        a[k] += (double)k * w + dw/wc * cos(wc*dk);
                    }
                }
            }
            else
            {
                for (k=0; k<N; k++)
                    a[k] += (double)k * w;
            }
        }
    }
    return WI_SUCCESS;
}
// end of Coltrane_PhaseFrequencyError()
        
// ================================================================================================
// FUNCTION  : Coltrane_TX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reset/configure the transmit model. This clears all memory elements and
//             calculates per-packet parameters.
// Parameters: N      -- number of samples to be processed
//             Fs_MHz -- sample rate (MHz)
//             Prms   -- complex input signal power
// ================================================================================================
wiStatus Coltrane_TX_Restart(int N, double Fs_MHz, double Prms)
{
    Coltrane_TxState_t *pTX = Coltrane_TxState();
    int i,j,k;

    pTX->Fs_MHz = Fs_MHz;
    pTX->Prms   = Prms;

    // -----------
    // Noise Setup
    // -----------
    {
        double rtHz = sqrt(pTX->Fs_MHz * 1.0E+6);
        pTX->Filter.Nrms = pTX->Filter.Ni * 1e-9 * rtHz * sqrt(2.0);
        pTX->Mixer.Nrms  = pTX->Mixer.Ni  * 1e-9 * rtHz * sqrt(2.0);
    }
    if (pTX->ModelLevel > 0)
    {
        double fc0 = (pTX->Filter.fcMHz / pTX->Fs_MHz) * (1+pTX->Filter.fcMismatch/2);
        double fc1 = (pTX->Filter.fcMHz / pTX->Fs_MHz) * (1-pTX->Filter.fcMismatch/2);
        XSTATUS(wiFilter_Butterworth(pTX->Filter.N, fc0, pTX->Filter.b[0], pTX->Filter.a[0], WIFILTER_IMPULSE_INVARIANCE) );
        XSTATUS(wiFilter_Butterworth(pTX->Filter.N, fc1, pTX->Filter.b[1], pTX->Filter.a[1], WIFILTER_IMPULSE_INVARIANCE) );
        for (i=0; i<2; i++) for (j=0; j<2; j++) pTX->Filter.w[i][j].Real = pTX->Filter.w[i][j].Imag = 0.0;
    }
    // -----------------
    // Startup Transient
    // -----------------
    pTX->Startup.k0   = (int)(pTX->Startup.t0 * (Fs_MHz * 1.0E+6) + 0.5);
    pTX->Startup.ktau = pTX->Startup.tau * (Fs_MHz * 1.0E+6);

    // ------------------
    // AC Coupling Filter
    // ------------------
    if (pTX->AC.Enabled || pTX->ModelLevel==2)
    {
        for (i=0; i<2; i++)
        {
            double betaR = tan(3.14159265 * (pTX->AC.s[i].fc_kHz*(1+pTX->AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            double betaI = tan(3.14159265 * (pTX->AC.s[i].fc_kHz*(1-pTX->AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            pTX->AC.s[i].b[0] = 1.0/(1.0+betaR);
            pTX->AC.s[i].a[0] = (betaR-1.0)/(betaR+1.0);
            pTX->AC.s[i].b[1] = 1.0/(1.0+betaI);
            pTX->AC.s[i].a[1] = (betaI-1.0)/(betaI+1.0);
            for (j=0; j<2; j++) {
                for (k=0; k<2; k++) {
                    pTX->AC.s[i].w[j][k].Real  = pTX->AC.s[i].w[j][k].Imag  = 0.0;
                    pTX->AC.s[i].yD[j][k].Real = pTX->AC.s[i].yD[j][k].Imag = 0.0;
                    pTX->AC.s[i].yD[j][k].Real = pTX->AC.s[i].yD[j][k].Imag = 0.0;
                }
            }
        }
    }
    // -------------------------
    // Clear the filter memories
    // -------------------------
    for (i=0; i<1; i++) {
        for (j=0; j<10; j++)
            pTX->Filter.w[i][j].Real = pTX->Filter.w[i][j].Imag = 0.0;
    }
    // -----------
    // Mixer Setup
    // -----------
    XSTATUS(Coltrane_PhaseFrequencyError(N, Fs_MHz, &(pTX->PLL)));

    return WI_SUCCESS;
}
// end of Coltrane_TX_Restart()

// ================================================================================================
// FUNCTION  : Coltrane_RX_ClearFilterMemory()
// ------------------------------------------------------------------------------------------------
// Purpose   : Restart the receiver model for a new packet/signal stream. All memory
//             elements are cleared.
// ================================================================================================
wiStatus Coltrane_RX_ClearFilterMemory(void)
{
    Coltrane_RxState_t *pRX = Coltrane_RxState();
    int i, j, k;

    //
    //  Low Pass Filters
    //
    for (i=0; i<2; i++) {
        for (j=0; j<10; j++) {
            pRX->Filter.w[i][j].Real = pRX->Filter.w[i][j].Imag = 0.0;
        }
    }
    //
    //  AC Coupling
    //
    for (i=0; i<4; i++) {
        for (j=0; j<2; j++) {
            for (k=0; k<2; k++) {
                pRX->AC.s[i].w[j][k].Real  = pRX->AC.s[i].w[j][k].Imag  = 0.0;
                pRX->AC.s[i].yD[j][k].Real = pRX->AC.s[i].yD[j][k].Imag = 0.0;
                pRX->AC.s[i].yD[j][k].Real = pRX->AC.s[i].yD[j][k].Imag = 0.0;
                pRX->AC.s[i].xD[j][k].Real = pRX->AC.s[i].xD[j][k].Imag = 0.0;
            }
        }
    }
    return WI_SUCCESS;
}
// end of Coltrane_RX_ClearFilterMemory()

// ================================================================================================
// FUNCTION  : Coltrane_RX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Restart the receiver model for a new packet/signal stream. All memory
//             elements are cleared.
// Parameters: N      -- number of samples to be processed
//             Fs_MHz -- sample rate (MHz)
//             NumRx  -- number of receive paths/antennas {1,2}
// ================================================================================================
wiStatus Coltrane_RX_Restart(int N, double Fs_MHz, int NumRx)
{
    Coltrane_RxState_t *pRX = Coltrane_RxState();
    int i;

    pRX->Fs_MHz = Fs_MHz;
    pRX->M_20MHz = (int)floor(Fs_MHz/20.0 + 1e-9);

    if (InvalidRange(N,      1, 1e9    )) return STATUS(WI_ERROR_PARAMETER1);
    if (InvalidRange(Fs_MHz, 1, 1000000)) return STATUS(WI_ERROR_PARAMETER2); 
    if (InvalidRange(NumRx,  1, 2      )) return STATUS(WI_ERROR_PARAMETER3);

    // --------------
    // Trace Variable
    // --------------
	if (!pRX->trace || pRX->Ntrace != N)
	{
	    WIFREE  ( pRX->trace );
        WICALLOC( pRX->trace, N, wiComplex );
	    pRX->Ntrace= N;
	}

    // ---------------
    // Radio Front End
    // ---------------
    pRX->RFE.Av[0] = pow(10.0, pRX->RFE.PdB[0]/20.0);
    pRX->RFE.Av[1] = pow(10.0, pRX->RFE.PdB[1]/20.0);
    pRX->RF.sqrtRin = sqrt(pRX->RF.Rin);
    pRX->RF.Vo      = pow(10,(pRX->RF.Pc-30-3)/20) * sqrt(pRX->RF.Rin); // -3dB for Vo(1+j);

    // -----------------------------
    // Receiver Input Referred Noise
    // -----------------------------
    {
        double T, B = Fs_MHz * 1.0E+6;         // bandwidth (Hz)
        if (pRX->ModelLevel < 2) T = 290.0;
        else                     T = pRX->T_degK;
        pRX->Nrms    = sqrt(kBoltzmann*T*B);     // rms noise amplitude (1 Ohm ref.)
        if (pRX->ModelLevel < 2)
            pRX->Nrms*= pow(10, pRX->NFdB/20.0);    // noise figure for models 0,1
    }
    // -----------
    // Mixer Setup
    // -----------
    {
        double A = pow(10.0, pRX->Mixer.Gain_dB/20.0);

        pRX->Mixer.cI = A * sin(pi/180.0 * pRX->Mixer.PhaseDeg);
        pRX->Mixer.cQ = A * cos(pi/180.0 * pRX->Mixer.PhaseDeg);

        XSTATUS(Coltrane_PhaseFrequencyError(N, Fs_MHz, &(pRX->PLL)));
    }
    // -----------
    // Noise Setup
    // -----------
    {
        double T, B = Fs_MHz * 1.0E+6;         // bandwidth (Hz)
        double rtHz = sqrt(B);
        if (pRX->ModelLevel < 2) T = 290.0;
        else                     T = pRX->T_degK;

        pRX->LNA2.Nrms[0]= sqrt(kBoltzmann*T*B) * (pow(10.0, pRX->LNA2.NFdB[0]/20.0) - 1.0);
        pRX->LNA2.Nrms[1]= sqrt(kBoltzmann*T*B) * (pow(10.0, pRX->LNA2.NFdB[1]/20.0) - 1.0);
        pRX->RF.Nrms[0]  = pRX->RF.Ni[0]  * 1E-9 * rtHz;
        pRX->RF.Nrms[1]  = pRX->RF.Ni[1]  * 1E-9 * rtHz;
        pRX->PGA.Nrms    = pRX->PGA.Ni    * 1E-9 * rtHz;
        pRX->Filter.Nrms = pRX->Filter.Ni * 1E-9 * rtHz;
        pRX->ADCBuf.Nrms = pRX->ADCBuf.Ni * 1E-9 * rtHz;

        pRX->Filter.Nrms_3dB = pRX->Filter.Nrms / sqrt(2.0); // precompute half-power noise level

        for (i=0; i<64; i++) {
            pRX->SGA.Nrms[i] = pRX->SGA.Ni[i] * 1E-9 * rtHz;
        }
    }
    // --------------
    // Lowpass Filter
    // --------------
    {
        double fc0 = (pRX->Filter.fcMHz / pRX->Fs_MHz) * (1+pRX->Filter.fcMismatch/2);
        double fc1 = (pRX->Filter.fcMHz / pRX->Fs_MHz) * (1-pRX->Filter.fcMismatch/2);

        XSTATUS(wiFilter_Butterworth(pRX->Filter.N, fc0, pRX->Filter.b[0], pRX->Filter.a[0], WIFILTER_IMPULSE_INVARIANCE) );
        XSTATUS(wiFilter_Butterworth(pRX->Filter.N, fc1, pRX->Filter.b[1], pRX->Filter.a[1], WIFILTER_IMPULSE_INVARIANCE) );
    }
    // ------------------
    // AC Coupling Filter
    // ------------------
    if (pRX->AC.Enabled || pRX->ModelLevel >= 2)
    {
        for (i=0; i<4; i++)
        {
            double betaR = tan(3.14159265 * (pRX->AC.s[i].fc_kHz*(1+pRX->AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            double betaI = tan(3.14159265 * (pRX->AC.s[i].fc_kHz*(1-pRX->AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            pRX->AC.s[i].b[0] = 1.0/(1.0+betaR);
            pRX->AC.s[i].a[0] = (betaR-1.0)/(betaR+1.0);
            pRX->AC.s[i].b[1] = 1.0/(1.0+betaI);
            pRX->AC.s[i].a[1] = (betaI-1.0)/(betaI+1.0);
        }
    }
    XSTATUS( Coltrane_RX_ClearFilterMemory() );

    // -----------------------
    // Tx I/Q Calibration Path
    // -----------------------
    if (pRX->ModelLevel == 6)
    {
        pRX->TxIQLoopback.lpfBuffer1[0] = 0;
        pRX->TxIQLoopback.lpfBuffer1[1] = 0;
        pRX->TxIQLoopback.lpfBuffer2[0] = 0;
        pRX->TxIQLoopback.lpfBuffer2[1] = 0;

        pRX->TxIQLoopback.hpfBuffer1[0] = 0;
        pRX->TxIQLoopback.hpfBuffer2[0] = 0;

        pRX->TxIQLoopback.NumRx = NumRx;

        pRX->TxIQLoopback.scaling = sqrt(7225)*0.011;
    }

    return WI_SUCCESS;
}
// end of Coltrane_RX_Restart()

// ================================================================================================
// FUNCTION  : Coltrane_CalcNonlinAmpModel()
// ------------------------------------------------------------------------------------------------
// Purpose   : Calculate parameters for the nonlinear amplifier model
// Parameters: 
// ================================================================================================
void Coltrane_CalcNonlinAmpModel(Coltrane_GainState_t *G)
{
    double a1, a2, a3, c2, c3, r;

    a1 = G->Av;
    a2 = -a1/G->IIV2;
    a3 = -4.0/3.0 * a1/SQR(G->IIV3);
    
    G->c2 = c2 = a2/(a1*a1);
    G->c3 = c3 = a3/(a1*a1*a1);

    r = sqrt(c2*c2 - 3*c3);
    G->Vmin =-min(min(fabs((-c2+r)/(3*c3)), fabs((-c2-r)/(3*c3))), 1.8*sqrt(2.0));
    G->Vmax = min(min(fabs((-c2+r)/(3*c3)), fabs((-c2-r)/(3*c3))), 1.8*sqrt(2.0));
}
// end of Coltrane_CalcNonlinAmpModel()

// ================================================================================================
// FUNCTION  : Coltrane_SetGainState()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: 
// ================================================================================================
void Coltrane_SetGainState(Coltrane_GainState_t *G, wiReal Av, wiReal IIV2, wiReal IIV3)
{
    G->Av   = Av;
    G->IIV2 = IIV2;
    G->IIV3 = IIV3;
    Coltrane_CalcNonlinAmpModel(G);
}
// end of Coltrane_SetGainState()

// ================================================================================================
// FUNCTION  : Coltrane_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the Coltrane model; setup default parameter values.
// Parameters: none
// ================================================================================================
wiStatus Coltrane_Init(void)
{
    int i;
    Coltrane_RxState_t *pRX = Coltrane_RxState();
    Coltrane_TxState_t *pTX = Coltrane_TxState();

    if (ModuleIsInit) return WI_WARN_MODULE_ALREADY_INIT;

    // -------------------
    // Default Model Level
    // -------------------
    pRX->ModelLevel = 0;
    pTX->ModelLevel = 0;

    // -----------------
    // Model Sample Rate
    // -----------------
    pRX->Fs_MHz = 80.0;
    pTX->Fs_MHz = 80.0;

    // ---------------
    // Front End Noise
    // ---------------
    pRX->T_degK = 290.0;

    // ---------------
    // Radio Front End
    // ---------------
    pRX->RFE.PdB[0] = -2.0;
    pRX->RFE.PdB[1] = -2.0;

    // -------------------
    // External/Second LNA
    // -------------------
    pRX->LNA2.Enabled = 0;
    pRX->LNA2.Av[0] = 1.0;
    pRX->LNA2.Av[1] = 1.0;
    pRX->LNA2.NFdB[0] = 0.0;
    pRX->LNA2.NFdB[1] = 0.0;

    // ----------------
    // LNA + Mixer (RF)
    // ----------------
    pRX->RF.Rin     = 50.0;
    pRX->RF.Ni[1]   =  0.459;
    pRX->RF.Ni[0]   =  1.748;
    pRX->RF.Pc      = -70; // dBm
    pRX->Mixer.Gain_dB = 0.2;
    pRX->Mixer.PhaseDeg= 1.0;
    pRX->Mixer.Vo      = 0.01;

    // ---
    // SGA
    // ---
    pRX->SGA.A0    = 1.0/64.0;
    pRX->SGA.Vmax  = 2.0;
    pRX->SGA.Vo    = 0.01;

    pRX->SGA.Ni[ 0] = 2328;
    pRX->SGA.Ni[ 1] = 1957;
    pRX->SGA.Ni[ 2] = 1646;
    pRX->SGA.Ni[ 3] = 1384;
    pRX->SGA.Ni[ 4] = 1164;
    pRX->SGA.Ni[ 5] = 979;
    pRX->SGA.Ni[ 6] = 823;
    pRX->SGA.Ni[ 7] = 692;
    pRX->SGA.Ni[ 8] = 582;
    pRX->SGA.Ni[ 9] = 489;
    pRX->SGA.Ni[10] = 411;
    pRX->SGA.Ni[11] = 346;
    pRX->SGA.Ni[12] = 302;
    pRX->SGA.Ni[13] = 265;
    pRX->SGA.Ni[14] = 238;
    pRX->SGA.Ni[15] = 180;
    pRX->SGA.Ni[16] = 158;
    pRX->SGA.Ni[17] = 139;
    pRX->SGA.Ni[18] = 125;
    pRX->SGA.Ni[19] = 95;
    pRX->SGA.Ni[20] = 84;
    pRX->SGA.Ni[21] = 74;
    pRX->SGA.Ni[22] = 67;
    pRX->SGA.Ni[23] = 53;
    pRX->SGA.Ni[24] = 47;
    pRX->SGA.Ni[25] = 42;
    pRX->SGA.Ni[26] = 39;
    pRX->SGA.Ni[27] = 34;
    pRX->SGA.Ni[28] = 31;
    pRX->SGA.Ni[29] = 29;
    pRX->SGA.Ni[30] = 28;
    pRX->SGA.Ni[31] = 21;
    pRX->SGA.Ni[32] = 15;
    pRX->SGA.Ni[33] = 15;
    pRX->SGA.Ni[34] = 15;
    pRX->SGA.Ni[35] = 15;
    pRX->SGA.Ni[36] = 15;
    pRX->SGA.Ni[37] = 15;
    pRX->SGA.Ni[38] = 15;
    pRX->SGA.Ni[39] = 15;
    pRX->SGA.Ni[40] = 15;
    pRX->SGA.Ni[41] = 15;
    pRX->SGA.Ni[42] = 15;
    pRX->SGA.Ni[43] = 15;
    pRX->SGA.Ni[44] = 15;
    pRX->SGA.Ni[45] = 15;
    pRX->SGA.Ni[46] = 15;
    pRX->SGA.Ni[47] = 15;

    // ---
    // PGA
    // ---
    pRX->PGA.Ni    = 65.0;
    pRX->PGA.Vo    = 0.01;

    // ----------
    // ADC Buffer
    // ----------
    pRX->ADCBuf.Ni = 100;
    pRX->ADCBuf.Vo = 0.01;

    // -------------
    // Coltrane Filters
    // -------------
    pRX->Filter.N         = 5;
    pRX->Filter.fcMHz     = 10;
    pRX->Filter.fcMismatch= 0.01;
    pRX->Filter.Ni        = 54.8;
    pRX->Filter.Vo        = 0.02;

    // --------------
    // RX Phase Noise
    // --------------
    pRX->PLL.Enabled = 1;
    pRX->PLL.CFO_ppm = 0;               // DO NOT USE (Keep at 0)
    pRX->PLL.Transient.PeakCFO_ppm = 0; // DO NOT USE (Keep at 0)
    pRX->PLL.FilterN = 1;
    pRX->PLL.fc = 225.0E+3;
    pRX->PLL.PSD = -94.5;

    // ---------------------
    // Large Signal Detector
    // ---------------------
    pRX->ThLgSigAFE = 0.7;

    // --------------
    // TX Phase Noise
    // --------------
    pTX->PLL.Enabled = 1;
    pTX->PLL.f_MHz = 5800;
    pTX->PLL.CFO_ppm = 0;
    pTX->PLL.Transient.PeakCFO_ppm = 0;
    pTX->PLL.Transient.fc = 280.0E+3;
    pTX->PLL.Transient.tau = 1.6E-6;
    pTX->PLL.Transient.k0 = 1980;
    pTX->PLL.FilterN = 1;
    pTX->PLL.fc = 225.0E+3;
    pTX->PLL.PSD = -94.5;

    pRX->Squelch.Enabled = 1;
    pRX->Squelch.Gain[0] = 0.01;   // -40 dB
    pRX->Squelch.Gain[1] = 0.01;   // -40 dB
    pRX->Squelch.Tstep   = 325e-9; // squelch period = 325ns

    pRX->AC.fcMismatch = 0.00;
    pRX->AC.Dynamic = 1;      // enable dynamic poles by default
    pRX->AC.Tstep   = 0.3e-6; // fast-pole step
    pRX->AC.Tslide  = 0.0;
    pRX->AC.fckHz_Slide = 1200;
    for (i=0; i<4; i++) pRX->AC.s[i].fc_kHz = 20.0;
    pRX->AC.s[3].fc_kHz = 0.0;

    pTX->AC.Enabled = 1;
    pTX->AC.fcMismatch = 0.00;
    pTX->AC.s[0].fc_kHz = 20.0;
    pTX->AC.s[1].fc_kHz = 20.0;

    pTX->Filter.N         = 3;
    pTX->Filter.fcMHz     = 20;      
    pTX->Filter.fcMismatch= 0.01;
    pTX->Filter.Ni        = 42.4;

    pTX->Mixer.Ni         = 0.0;
    pTX->Mixer.Gain_dB    = 0.0;
    pTX->Mixer.PhaseDeg   = 0.0;

    pTX->PowerAmp.Enabled = 0;
    pTX->PowerAmp.OBO = 10;
    pTX->PowerAmp.p = 2;
    pTX->PowerAmp.Ap = 1;
    pTX->PowerAmp.DelayShutdown = 2e-6; // end-of-packet to TX shutdown

    // --------------------------------
    // New RX Polynomial Amp Parameters
    // --------------------------------
    Coltrane_SetGainState(&(pRX->RF.Gain[0]),   2.51,   3.5,  0.85);
    Coltrane_SetGainState(&(pRX->RF.Gain[1]),  28.30,   0.53, 0.071);

    Coltrane_SetGainState(&(pRX->Filter.Gain),  0.94, 135.0, 5.0 );
    Coltrane_SetGainState(&(pRX->PGA.Gain),     4.00,  15.0, 5.0 );
    Coltrane_SetGainState(&(pRX->ADCBuf.Gain),  0.89,  30.0, 9.0 );
    
    for (i=0; i<64; i++)
        Coltrane_SetGainState(pRX->SGA.Gain+i, pow(2.0,(double)(min(i,47)-24)/4.0), 30, 10);

    // ---------------------------
    // Register the wiParse method
    // ---------------------------
    XSTATUS(wiParse_AddMethod(Coltrane_ParseLine));

    ModuleIsInit = 1;
    return WI_SUCCESS;
}
// end of Coltrane_Init()

// ================================================================================================
// FUNCTION  : Coltrane_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the Coltrane model
// Parameters: none
// ================================================================================================
wiStatus Coltrane_Close(void)
{
    int n;
    if (!ModuleIsInit) return WI_WARN_MODULE_NOT_INITIALIZED;

    for (n=0; n<WISE_MAX_THREADS; n++)
    {
        WIFREE_ARRAY( TX[n].PLL.a );
		WIFREE_ARRAY( TX[n].PLL.u );
        WIFREE_ARRAY( RX[n].PLL.a );
		WIFREE_ARRAY( RX[n].PLL.u );
        WIFREE( RX[n].trace );
    }
    XSTATUS(wiParse_RemoveMethod(Coltrane_ParseLine));

    ModuleIsInit = 0;
    return WI_SUCCESS;
}
// end of Coltrane_Close()

// ================================================================================================
// FUNCTION  : Coltrane_InitalizeThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Coltrane_InitializeThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        Coltrane_TxState_t *pTX = TX + ThreadIndex;
        Coltrane_RxState_t *pRX = RX + ThreadIndex;

        memcpy( pTX, TX, sizeof(Coltrane_TxState_t) );
        memcpy( pRX, RX, sizeof(Coltrane_RxState_t) );

        pTX->PLL.a.ptr = pRX->PLL.a.ptr = NULL;
		pTX->PLL.u.ptr = pRX->PLL.u.ptr = NULL;
        pRX->trace = NULL;
    }
    return WI_SUCCESS;
}
// end of Coltrane_InitializeThread()

// ================================================================================================
// FUNCTION  : Coltrane_CloseThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Coltrane_CloseThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        Coltrane_TxState_t *pTX = TX + ThreadIndex;
        Coltrane_RxState_t *pRX = RX + ThreadIndex;

        WIFREE_ARRAY( pTX->PLL.a );
        WIFREE_ARRAY( pTX->PLL.u );
        WIFREE_ARRAY( pRX->PLL.a );
        WIFREE_ARRAY( pRX->PLL.u );
        WIFREE( pRX->trace )
    }
    return WI_SUCCESS;
}
// end of Coltrane_CloseThread()

// ================================================================================================
// FUNCTION : Coltrane_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Coltrane_StatePointer(Coltrane_TxState_t **pTX, Coltrane_RxState_t **pRX)
{
    if (pTX) *pTX = TX;
    if (pRX) *pRX = RX;
    return WI_SUCCESS;
}
// end of Coltrane_StatePointer()

// ================================================================================================
// FUNCTION : Coltrane_TxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Coltrane_TxState_t * Coltrane_TxState(void)
{
    return TX + wiProcess_ThreadIndex();
}
// end of Coltrane_TxState()

// ================================================================================================
// FUNCTION : Coltrane_RxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
Coltrane_RxState_t * Coltrane_RxState(void)
{
    return RX + wiProcess_ThreadIndex();
}
// end of Coltrane_RxState()

// ================================================================================================
// FUNCTION : Coltrane_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Coltrane_ParseLine(wiString text)
{
    #define PSTATUS(arg) if ((pstatus=WICHK(arg))<=0) return pstatus;

    #ifdef COLTRANE_IS_RF55 ////// String replacement RF55 --> Coltrane

        char NewText[WIPARSE_MAX_LINE_LENGTH];
        if (strstr(text, "RF55") == text)
        {
            sprintf(NewText, "Coltrane%s", strstr(text,"RF55")+4);
            text = NewText;
        }

    #endif

    if (strstr(text, "Coltrane"))
    {
        wiStatus status, pstatus; 
        wiReal x; wiInt i;

        Coltrane_RxState_t *pRX = Coltrane_RxState();
        Coltrane_TxState_t *pTX = Coltrane_TxState();

        PSTATUS(wiParse_Int    (text,"Coltrane.RX.ModelLevel",    &(pRX->ModelLevel), 0, 6));
        PSTATUS(wiParse_Int    (text,"Coltrane.TX.ModelLevel",    &(pTX->ModelLevel), 0, 2));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RFE.PdB[0]",    &(pRX->RFE.PdB[0]),-20.0, 0.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RFE.PdB[1]",    &(pRX->RFE.PdB[1]),-20.0, 0.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.T_degK",        &(pRX->T_degK),  0.0, 10000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.NFdB",          &(pRX->NFdB), -100.0,   100.0));

        PSTATUS(wiParse_Boolean(text,"Coltrane.RX.LNA2.Enabled",  &(pRX->LNA2.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.LNA2.Av[0]",    &(pRX->LNA2.Av[0]), 0, 1e3));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.LNA2.Av[1]",    &(pRX->LNA2.Av[1]), 0, 1e3));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.LNA2.NFdB[0]",  &(pRX->LNA2.NFdB[0]),0.0,20.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.LNA2.NFdB[1]",  &(pRX->LNA2.NFdB[0]),0.0,20.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Av[0]",      &(pRX->RF.Gain[0].Av),  0, 1e3));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Av[1]",      &(pRX->RF.Gain[1].Av),  0, 1e3));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Ni[0]",      &(pRX->RF.Ni[0]),  0.0, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Ni[1]",      &(pRX->RF.Ni[1]),  0.0, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Vmax[0]",    &(pRX->RF.Gain[0].Vmax),0.01, 10.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Vmax[1]",    &(pRX->RF.Gain[1].Vmax),0.01, 10.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.RF.Pc",         &(pRX->RF.Pc), -1000.0, 0.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Mixer.PhaseDeg",&(pRX->Mixer.PhaseDeg), -45.0,  45.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Mixer.Gain_dB", &(pRX->Mixer.Gain_dB),   -6.0,   6.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.SGA.A0",        &(pRX->SGA.A0), 1e-2, 1e2));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.SGA.Vmax",      &(pRX->SGA.Vmax), 1e-2,   10.0));
        PSTATUS(wiParse_RealArray(text,"Coltrane.RX.SGA.Ni",      pRX->SGA.Ni, 64, 0.0, 10000.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PGA.Av",        &(pRX->PGA.Gain.Av),    0.1,  1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PGA.Ni",        &(pRX->PGA.Ni),    0.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PGA.Vmax",      &(pRX->PGA.Gain.Vmax),  0.01,  10.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.ThLgSigAFE",       &(pRX->ThLgSigAFE), 0.0, 1E+6));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.Av",     &(pRX->Filter.Gain.Av),0.01, 100.0));
        PSTATUS(wiParse_Int    (text,"Coltrane.RX.Filter.N",      &(pRX->Filter.N),    1, 7));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.fcMHz",  &(pRX->Filter.fcMHz), 1, 35));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.fc",     &(pRX->Filter.fcMHz), 1, 35));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.Ni",     &(pRX->Filter.Ni), 0.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.Vmax",   &(pRX->Filter.Gain.Vmax), 0.01, 10.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.fcMismatch",&(pRX->Filter.fcMismatch),   -0.5, 0.5));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Av",     &(pRX->ADCBuf.Gain.Av),    0.1,  100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Ni",     &(pRX->ADCBuf.Ni),    0.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Vo5AI",  &(pRX->ADCBuf.Vo5AI), -100.0, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Vo5AQ",  &(pRX->ADCBuf.Vo5AQ), -100.0, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Vo5BI",  &(pRX->ADCBuf.Vo5BI), -100.0, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Vo5BQ",  &(pRX->ADCBuf.Vo5BQ), -100.0, 100.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Mixer.Vo",      &(pRX->Mixer.Vo),  -1.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.SGA.Vo",        &(pRX->SGA.Vo),    -1.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Filter.Vo",     &(pRX->Filter.Vo), -1.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PGA.Vo",        &(pRX->PGA.Vo),    -1.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.ADCBuf.Vo",     &(pRX->ADCBuf.Vo), -1.0, 1000.0));

        PSTATUS(wiParse_Boolean(text,"Coltrane.RX.Squelch.Enabled",&(pRX->Squelch.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Squelch.Gain[0]",&(pRX->Squelch.Gain[0]), 0.0, 1.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Squelch.Gain[1]",&(pRX->Squelch.Gain[1]), 0.0, 1.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.Squelch.Tstep",  &(pRX->Squelch.Tstep), 0.0, 1e-6));

        PSTATUS(wiParse_Boolean(text,"Coltrane.RX.AC.Enabled",    &(pRX->AC.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.fcMismatch", &(pRX->AC.fcMismatch),   -0.5, 0.5));
        PSTATUS(wiParse_Boolean(text,"Coltrane.RX.AC.Dynamic",    &(pRX->AC.Dynamic)));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.Tstep",      &(pRX->AC.Tstep),  0.0,  1e-6));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.Tslide",     &(pRX->AC.Tslide), 0.0, 10e-6));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.fckHz_Slide",&(pRX->AC.fckHz_Slide), 0.0, 10e3));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.s[0].fc_kHz",&(pRX->AC.s[0].fc_kHz), 0.0,10000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.s[1].fc_kHz",&(pRX->AC.s[1].fc_kHz), 0.0,10000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.s[2].fc_kHz",&(pRX->AC.s[2].fc_kHz), 0.0,10000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.AC.s[3].fc_kHz",&(pRX->AC.s[3].fc_kHz), 0.0,10000.0));
    
        PSTATUS(wiParse_Boolean(text,"Coltrane.TX.AC.Enabled",    &(pTX->AC.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.AC.fcMismatch", &(pTX->AC.fcMismatch),   -0.5, 0.5));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.AC.s[0].fc_kHz",&(pTX->AC.s[0].fc_kHz), 0.0, 2000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.AC.s[1].fc_kHz",&(pTX->AC.s[1].fc_kHz), 0.0, 2000.0));

        PSTATUS(wiParse_Int    (text,"Coltrane.TX.Filter.N",      &(pTX->Filter.N), 1, 7));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Filter.fcMHz",  &(pTX->Filter.fcMHz), 1.0, 40.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Filter.fc",     &(pTX->Filter.fcMHz), 1.0, 40.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Filter.Ni",     &(pTX->Filter.Ni), 0.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Filter.fcMismatch",&(pTX->Filter.fcMismatch),   -0.5, 0.5));

        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Mixer.Ni",      &(pTX->Mixer.Ni), 0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Mixer.PhaseDeg",&(pTX->Mixer.PhaseDeg), -45.0,  45.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Mixer.Gain_dB", &(pTX->Mixer.Gain_dB),   -6.0,   6.0));

        PSTATUS(wiParse_Boolean(text,"Coltrane.TX.PLL.Enabled",   &(pTX->PLL.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.CFO_ppm",   &(pTX->PLL.CFO_ppm), -1000.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.f_MHz",     &(pTX->PLL.f_MHz),   1.0, 1.0E+4));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.fc",        &(pTX->PLL.fc), 1.0, 10.0E+6));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.PSD",       &(pTX->PLL.PSD), -200.0, 0.0));
        PSTATUS(wiParse_Int    (text,"Coltrane.TX.PLL.FilterN",   &(pTX->PLL.FilterN), 1, 9));

        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.Transient.PeakCFO_ppm",&(pTX->PLL.Transient.PeakCFO_ppm), 0.0, 1000.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.Transient.fc",         &(pTX->PLL.Transient.fc),          1.0, 20E+6));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PLL.Transient.tau",        &(pTX->PLL.Transient.tau),         0.0, 1E-3));
        PSTATUS(wiParse_Int    (text,"Coltrane.TX.PLL.Transient.k0",         &(pTX->PLL.Transient.k0),         -1000, 100000));

        PSTATUS(wiParse_Boolean(text,"Coltrane.RX.PLL.Enabled",      &(pRX->PLL.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PLL.fc",           &(pRX->PLL.fc), 1.0, 10.0E+6));
        PSTATUS(wiParse_Real   (text,"Coltrane.RX.PLL.PSD",          &(pRX->PLL.PSD), -200.0, 0.0));
        PSTATUS(wiParse_Int    (text,"Coltrane.RX.PLL.FilterN",      &(pRX->PLL.FilterN), 1, 9));

        PSTATUS(wiParse_Boolean(text,"Coltrane.TX.PowerAmp.Enabled", &(pTX->PowerAmp.Enabled)));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PowerAmp.p",       &(pTX->PowerAmp.p), 0.1, 100.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PowerAmp.OBO",     &(pTX->PowerAmp.OBO), 0, 40.0));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.PowerAmp.Ap",      &(pTX->PowerAmp.Ap), 1.0, 2.0));

        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Startup.t0",       &(pTX->Startup.t0),  0.0, 0.001));
        PSTATUS(wiParse_Real   (text,"Coltrane.TX.Startup.tau",      &(pTX->Startup.tau), 0.0, 0.001));

        status = STATUS(wiParse_Real(text,"Coltrane.PLL.PSD",&x, -200,0.0));
        if (status == WI_SUCCESS) {
            pTX->PLL.PSD = pRX->PLL.PSD = x;
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Int(text,"Coltrane.PLL.FilterN",&i, 1, 7));
        if (status == WI_SUCCESS) {
            pTX->PLL.FilterN = pRX->PLL.FilterN = i;
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Real(text,"Coltrane.PLL.fc", &x, 1.0, 10.0E+6));
        if (status == WI_SUCCESS) {
            pTX->PLL.fc = pRX->PLL.fc = x;
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Real(text,"Coltrane.PLL.f_MHz", &x, 1.0, 1.0E+4));
        if (status == WI_SUCCESS) {
            pTX->PLL.f_MHz = pRX->PLL.f_MHz = x;
            return WI_SUCCESS;
        }
        status = STATUS(wiParse_Real(text,"Coltrane.AC.fc_kHz", &x, 0.0, 1.0E+4));
        if (status == WI_SUCCESS) {
            int i;
            for (i=0; i<2; i++) pTX->AC.s[i].fc_kHz = x;
            for (i=0; i<3; i++) pRX->AC.s[i].fc_kHz = x;
            return WI_SUCCESS;
        }
        // ----------------------------
        // Set Stage Gain and Linearity
        // ----------------------------
        {
            double Av, IIV2, IIV3;
            char paramstr[128]; int i;

            XSTATUS( status = wiParse_Function(text,"Coltrane_SetGainState(%127s, %f, %f, %f)",&paramstr, &Av, &IIV2, &IIV3) );
            if (status == WI_SUCCESS)
            {
                     if (!strcmp(paramstr, "RX.RF.Gain[0]" )) Coltrane_SetGainState(&(pRX->RF.Gain[0]),  Av, IIV2, IIV3);
                else if (!strcmp(paramstr, "RX.RF.Gain[1]" )) Coltrane_SetGainState(&(pRX->RF.Gain[1]),  Av, IIV2, IIV3);
                else if (!strcmp(paramstr, "RX.Filter.Gain")) Coltrane_SetGainState(&(pRX->Filter.Gain), Av, IIV2, IIV3);
                else if (!strcmp(paramstr, "RX.PGA.Gain"   )) Coltrane_SetGainState(&(pRX->PGA.Gain),    Av, IIV2, IIV3);
                else if (!strcmp(paramstr, "RX.ADCBuf.Gain")) Coltrane_SetGainState(&(pRX->ADCBuf.Gain), Av, IIV2, IIV3);
                else if (!strcmp(paramstr, "RX.SGA.Gain"))  
                {
                    for (i=0; i<64; i++)
                        Coltrane_SetGainState(pRX->SGA.Gain+i, pow(2.0,(double)(min(i,47)-24)/4.0), IIV2, IIV3);
                }
                else 
                    return STATUS(WI_ERROR_PARAMETER1);

                return WI_SUCCESS;
            }
        }
    }
    return WI_WARN_PARSE_FAILED;
}
// end of Coltrane_ParseLine()

// ================================================================================================
// FUNCTION : Coltrane_CalcPhaseNoise()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal Coltrane_CalcPhaseNoise(wiInt Npoles, wiReal fc, wiReal PSDdB)
{
    double dBc = PSDdB + 10*log10( (fc * 4 * pi) / (4 * Npoles * sin(pi / (2 * Npoles))) );
    return dBc;
}
// end of Coltrane_CalcPhaseNoise()

// ================================================================================================
// FUNCTION : Coltrane_CalcIMRdB()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiReal Coltrane_CalcIMRdB(wiReal Gain_dB, wiReal PhaseDeg)
{
    double A  = pow(10.0, Gain_dB/20.0);
    double AA = 1 + A*A;
	double BB = 2*A*cos(PhaseDeg * pi/180);
	double IMRdB = 10*log10( (AA+BB)/(AA-BB) );

    return IMRdB;
}
// end of Coltrane_CalcIMRdB()

// ================================================================================================
// FUNCTION : Coltrane_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Coltrane_WriteConfig(wiMessageFn MsgFunc)
{
    int i;

    Coltrane_TxState_t *pTX = Coltrane_TxState();
    Coltrane_RxState_t *pRX = Coltrane_RxState();

    double TxPn = Coltrane_CalcPhaseNoise( pTX->PLL.FilterN, pTX->PLL.fc, pTX->PLL.PSD );
    double RxPn = Coltrane_CalcPhaseNoise( pRX->PLL.FilterN, pRX->PLL.fc, pRX->PLL.PSD );

    double TxIMRdB = Coltrane_CalcIMRdB( pTX->Mixer.Gain_dB, pTX->Mixer.PhaseDeg );
    double RxIMRdB = Coltrane_CalcIMRdB( pRX->Mixer.Gain_dB, pRX->Mixer.PhaseDeg );

    // --------
    // Receiver
    // --------
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Coltrane Radio Model  : pTX->ModelLevel = %d, pRX->ModelLevel = %d\n",pTX->ModelLevel,pRX->ModelLevel);
    if (pRX->T_degK )
        if (pRX->ModelLevel<2)
            MsgFunc(" RX System Noise       : NF=%1.1fdB\n",pRX->NFdB);
        else
            MsgFunc(" RX System Noise       : T=%3.0f degK (Ni=%1.0fdBm/Hz, NF=%1.1fdB)\n",pRX->T_degK,30+10*log10(kBoltzmann*pRX->T_degK),10*log10(pRX->T_degK/290));
    else
        MsgFunc(" RX System Noise       : T=%3.0f degK (Noiseless)\n",pRX->T_degK);

    switch (pRX->ModelLevel)
    {
        case 0:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-pRX->RFE.PdB[0],-pRX->RFE.PdB[1]);
            if (pRX->LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(pRX->LNA2.Av[1]),DB20(pRX->LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(pRX->RF.Gain[1].Av),DB20(pRX->RF.Gain[0].Av),pRX->RF.Rin);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(pRX->SGA.A0),DB20(pRX->PGA.Gain.Av),DB20(pRX->ADCBuf.Gain.Av));
            break;

        case 1:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-pRX->RFE.PdB[0],-pRX->RFE.PdB[1]);
            if (pRX->LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(pRX->LNA2.Av[1]),DB20(pRX->LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(pRX->RF.Gain[1].Av),DB20(pRX->RF.Gain[0].Av),pRX->RF.Rin);
            if (pRX->PLL.Enabled) 
                MsgFunc("    Phase Noise        : %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",RxPn,pRX->PLL.PSD,pRX->PLL.fc/1e3,pRX->PLL.FilterN);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(pRX->SGA.A0),DB20(pRX->PGA.Gain.Av),DB20(pRX->ADCBuf.Gain.Av));
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",pRX->Filter.N,pRX->Filter.fcMHz,100.0*pRX->Filter.fcMismatch);
            if (pRX->AC.Enabled) {
                MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",pRX->AC.s[0].fc_kHz,pRX->AC.s[1].fc_kHz,pRX->AC.s[2].fc_kHz,100.0*pRX->AC.fcMismatch);
                MsgFunc("                       : Dynamic = %d, Tstep=%1.0fns, Tslide=%1.0fns, fcSlide=%1.0f kHz\n",pRX->AC.Dynamic,pRX->AC.Tstep*1e9,pRX->AC.Tslide*1e9,pRX->AC.fckHz_Slide);
                MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(pRX->Squelch.Gain[0]),DB20(pRX->Squelch.Gain[1]),pRX->Squelch.Tstep*1e9,pRX->Squelch.Enabled);
            }
			break;

        case 2:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB, Rin=%1.1f Ohms\n",-pRX->RFE.PdB[0],-pRX->RFE.PdB[1],pRX->RF.Rin);
            if (pRX->LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB, NF={%1.1f,%1.1f} dB\n",DB20(pRX->LNA2.Av[1]),DB20(pRX->LNA2.Av[0]),pRX->LNA2.NFdB[1],pRX->LNA2.NFdB[0]);
            MsgFunc("    LNA+Mixer LNAGain=1: Av=%5.2f, IIV2=%5.2f, IIV3=%5.3f, Ni=%5.2fnV/rtHz\n",pRX->RF.Gain[1].Av,pRX->RF.Gain[1].IIV2,pRX->RF.Gain[1].IIV3,pRX->RF.Ni[1]);
            MsgFunc("              LNAGain=0: Av=%5.2f, IIV2=%5.2f, IIV3=%5.3f, Ni=%5.2fnV/rtHz\n",pRX->RF.Gain[0].Av,pRX->RF.Gain[0].IIV2,pRX->RF.Gain[0].IIV3,pRX->RF.Ni[0]);
            i=24;
            MsgFunc("    SGA (AGAIN =%3d)   : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",i,pRX->SGA.Gain[i].Av,pRX->SGA.Gain[i].IIV2,pRX->SGA.Gain[i].IIV3,pRX->SGA.Ni[i]);
            MsgFunc("    Low Pass Filter    : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",pRX->Filter.Gain.Av,pRX->Filter.Gain.IIV2,pRX->Filter.Gain.IIV3,pRX->Filter.Ni);
            MsgFunc("    PGA                : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",pRX->PGA.Gain.Av,pRX->PGA.Gain.IIV2,pRX->PGA.Gain.IIV3,pRX->PGA.Ni);
            MsgFunc("    ADCBuf (Radio Out) : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",pRX->ADCBuf.Gain.Av,pRX->ADCBuf.Gain.IIV2,pRX->ADCBuf.Gain.IIV3,pRX->ADCBuf.Ni);
            if (pRX->Mixer.PhaseDeg || pRX->Mixer.Gain_dB)
                MsgFunc("    Mixer I/Q Mismatch : %5.1f dBc (Phase=%1.1f deg, Gain=%1.2f dB)\n",-RxIMRdB,pRX->Mixer.PhaseDeg,pRX->Mixer.Gain_dB);
            if (pRX->PLL.Enabled) 
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",RxPn,pRX->PLL.PSD,pRX->PLL.fc/1e3,pRX->PLL.FilterN);
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",pRX->Filter.N,pRX->Filter.fcMHz,100.0*pRX->Filter.fcMismatch);
            MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f, %1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",pRX->AC.s[0].fc_kHz,pRX->AC.s[1].fc_kHz,pRX->AC.s[2].fc_kHz,pRX->AC.s[3].fc_kHz,100.0*pRX->AC.fcMismatch);
            MsgFunc("                       : Dynamic = %d, Tstep=%1.0fns, Tslide=%1.0fns, fcSlide=%1.0f kHz\n",pRX->AC.Dynamic,pRX->AC.Tstep*1e9,pRX->AC.Tslide*1e9,pRX->AC.fckHz_Slide);
            MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(pRX->Squelch.Gain[0]),DB20(pRX->Squelch.Gain[1]),pRX->Squelch.Tstep*1e9,pRX->Squelch.Enabled);
            MsgFunc("    DC/Offsets (mV)    : PcRF=%3.0fdBm, Mixer=%2.0f, SGA=%2.0f, LPF=%2.0f, PGA=%2.0f, Buf=%2.0f\n",pRX->RF.Pc,1000*pRX->Mixer.Vo, 1000*pRX->SGA.Vo,1000*pRX->Filter.Vo,1000*pRX->PGA.Vo, 1000*pRX->ADCBuf.Vo);
            MsgFunc("    Large Signal Detect: Th =%1.0fmV\n",1000*pRX->ThLgSigAFE);
            break;

        case 4:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-pRX->RFE.PdB[0],-pRX->RFE.PdB[1]);
            if (pRX->LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(pRX->LNA2.Av[1]),DB20(pRX->LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(pRX->RF.Gain[1].Av),DB20(pRX->RF.Gain[0].Av),pRX->RF.Rin);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(pRX->SGA.A0),DB20(pRX->PGA.Gain.Av),DB20(pRX->ADCBuf.Gain.Av));
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",pRX->Filter.N,pRX->Filter.fcMHz,100.0*pRX->Filter.fcMismatch);
            if (pRX->AC.Enabled) {
                MsgFunc("    AC Coupling Pole   : fc = %1.0f kHz (%1.1f%% Mismatch)\n",pRX->AC.s[0].fc_kHz,100.0*pRX->AC.fcMismatch);
                MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(pRX->Squelch.Gain[0]),DB20(pRX->Squelch.Gain[1]),pRX->Squelch.Tstep*1e9,pRX->Squelch.Enabled);
            }
			break;

        case 5:
            MsgFunc("    Fixed Offset Only  : AI = %1.4f, AQ = %1.4f, BI = %1.4f, BQ = %1.4f\n",pRX->ADCBuf.Vo5AI,pRX->ADCBuf.Vo5AQ,pRX->ADCBuf.Vo5BI,pRX->ADCBuf.Vo5BQ);
            break;

        case 6:
            MsgFunc("    Tx I/Q Calibration loop-back\n");
            break;

        default: XSTATUS(WI_ERROR_UNDEFINED_CASE);
    }

    // -----------
    // Transmitter
    // -----------
    switch (pTX->ModelLevel)
    {
        case 0:
        case 1:
            if (pTX->ModelLevel>0) {    
                                            MsgFunc(" TX LPF                : N = %d pole Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",pTX->Filter.N,pTX->Filter.fcMHz,100.0*pTX->Filter.fcMismatch);
                if (pTX->AC.Enabled)    MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",pTX->AC.s[0].fc_kHz,pTX->AC.s[1].fc_kHz,100.0*pTX->AC.fcMismatch);
            }
            if (pTX->Mixer.PhaseDeg || pTX->Mixer.Gain_dB)
                 MsgFunc("    Mixer I/Q Mismatch : %5.1f dBc (Phase=%1.1f deg, Gain=%1.2f dB)\n",-TxIMRdB,pTX->Mixer.PhaseDeg,pTX->Mixer.Gain_dB);
            else MsgFunc("    Mixer I/Q Mismatch : none\n");
            if (pTX->PLL.Enabled)
            {
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d); CFO = %3.1f kHz\n",TxPn,pTX->PLL.PSD,pTX->PLL.fc/1e3,pTX->PLL.FilterN,pTX->PLL.CFO_ppm*pTX->PLL.f_MHz/1e3);
                if (pTX->PLL.Transient.PeakCFO_ppm)
                    MsgFunc("    Transient CFO      : k0=%d, Peak=%1.1fppm, fc=%1.0fkHz, tau=%1.1fus\n",pTX->PLL.Transient.k0,pTX->PLL.Transient.PeakCFO_ppm,pTX->PLL.Transient.fc/1E3,1E6*pTX->PLL.Transient.tau);
            }
            if (pTX->PowerAmp.Enabled) MsgFunc("    PowerAmp           : Rapp Model p=%g, OBO=%g dB, Ap=%g dB\n",pTX->PowerAmp.p,pTX->PowerAmp.OBO,10*log10(pTX->PowerAmp.Ap));
            else                     MsgFunc("    PowerAmp           : Linear\n");
            break;

        case 2:
            MsgFunc(" TX LPF                : N = %d pole Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",pTX->Filter.N,pTX->Filter.fcMHz,100.0*pTX->Filter.fcMismatch);
            MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",pTX->AC.s[0].fc_kHz,pTX->AC.s[1].fc_kHz,100.0*pTX->AC.fcMismatch);
            if (pTX->Mixer.PhaseDeg || pTX->Mixer.Gain_dB)
                 MsgFunc("    Mixer I/Q Mismatch : %5.1f dBc (Phase=%1.1f deg, Gain=%1.2f dB)\n",-TxIMRdB,pTX->Mixer.PhaseDeg,pTX->Mixer.Gain_dB);
            else MsgFunc("    Mixer I/Q Mismatch : none\n");
            if (pTX->PLL.Enabled)
            {
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",TxPn,pTX->PLL.PSD,pTX->PLL.fc/1e3,pTX->PLL.FilterN);
                MsgFunc("    Carrier Freq Offset: %5.1f ppm @ %5.3f GHz = %4.1f kHz\n",pTX->PLL.CFO_ppm,pTX->PLL.f_MHz/1e3,pTX->PLL.CFO_ppm*pTX->PLL.f_MHz/1e3);
                if (pTX->PLL.Transient.PeakCFO_ppm)
                    MsgFunc("    Transient CFO      : k0=%d, Peak=%1.1fppm, fc=%1.0fkHz, tau=%1.1fus\n",pTX->PLL.Transient.k0,pTX->PLL.Transient.PeakCFO_ppm,pTX->PLL.Transient.fc/1E3,1E6*pTX->PLL.Transient.tau);
            }
            MsgFunc("    PowerAmp           : Rapp Model p=%1.1f, OBO=%1.1fdB\n",pTX->PowerAmp.p,pTX->PowerAmp.OBO);
            if (pTX->Startup.tau)
                MsgFunc("    Startup Transient  : t0=%1.2fus, tau=%1.2fus\n",pTX->Startup.t0*1.0E+6,pTX->Startup.tau*1.0E+6);
            break;
    }
    return WI_SUCCESS;
}
// end of Coltrane_WriteConfig()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

//--< RF52.c >-------------------------------------------------------------------------------------
//=================================================================================================
//
//  Baseband Equivalent Model for the RF52 Radio
//           
//  Developed by Barrett Brickner
//  Copyright 2005-2006 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "RF52.h"
#include "wiFilter.h"
#include "wiRnd.h"
#include "wiMath.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean ModuleIsInit = 0;   // module has been initialized
static RF52_TxState_t TX      = {0}; // transmitter state
static RF52_RxState_t RX      = {0}; // receiver state
static wiStatus _status       = 0;   // return code

static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  FUNCTION PROTOTPYES
//=================================================================================================
static wiStatus RF52_ParseLine(wiString text);

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((_status=WICHK(arg))<=0) return _status;

#define RF52_AddNoise(x,Nrms) wiRnd_AddComplexNormal(x,Nrms)

#define DB20(Av) (20.0*log(Av)/log(10.0))  /* Convert voltage gain to dB */
#define SQR(x)   ((x)*(x))                 /* square = x^2               */

static const double kBoltzmann = 1.380658e-23; // Boltzmann's constant

// ================================================================================================
// FUNCTION  : RF52_LgSigDetector
// ------------------------------------------------------------------------------------------------
// Purpose   : Large signal detector
// Parameters: x          -- complex input signal
//             ThLgSigAFE -- threshold for detection
//             LgSigAFE   -- detector output
// ================================================================================================
__inline void RF52_LgSigDetector(wiComplex *x, wiReal ThLgSigAFE, wiInt *LgSigAFE)
{
    *LgSigAFE = ((fabs(x->Real) > ThLgSigAFE) || (fabs(x->Imag) > ThLgSigAFE)) ? 1:0;
}
// end of RF52_LgSigDetector()

// ================================================================================================
// FUNCTION  : RF52_AddOffset
// ------------------------------------------------------------------------------------------------
// Purpose   : Add a DC term to the I and Q components
// Parameters: x -- complex I/O signal
//             c -- offset amplitude
// ================================================================================================
__inline void RF52_AddOffset(wiComplex *x, wiReal c)
{
    x->Real += c;
    x->Imag += c;
}
// end of RF52_AddOffset()

// ================================================================================================
// FUNCTION  : RF52_Amplifier
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a simple gain term for the complex signal.
// Parameters: x -- complex I/O signal
//             A -- amplifier gain (V/V)
// ================================================================================================
__inline void RF52_Amplifier(wiComplex *x, wiReal A)
{
    x->Real *= A;
    x->Imag *= A;
}
// end of RF52_Amplifier()

// ================================================================================================
// FUNCTION  : RF52_NonlinAmp
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement an amplifier with 2nd and 3rd order nonlinearity
// Parameters: x -- complex I/O signal
//             G -- gain structure
// ================================================================================================
__inline void RF52_NonlinAmp(wiComplex *x, RF52_GainState_t *G)
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
// end of RF52_NonlinAmp()

// ================================================================================================
// FUNCTION  : RF52_TX_AC_Pole
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a single AC coupling pole (fixed position)
// Parameters: x    -- i/o signal (by reference)
//             n    -- signal path index
//             pole -- pole filter structure (by reference)
// ================================================================================================
__inline void RF52_TX_AC_Pole(wiComplex *x, wiInt n, RF52_PoleState_t *pole)
{
    pole->w[n][1] = pole->w[n][0];

    pole->w[n][0].Real = x->Real - (pole->a[0] * pole->w[n][1].Real);
    pole->w[n][0].Imag = x->Imag - (pole->a[1] * pole->w[n][1].Imag);

    x->Real = pole->b[0] * (pole->w[n][0].Real - pole->w[n][1].Real);
    x->Imag = pole->b[1] * (pole->w[n][0].Imag - pole->w[n][1].Imag);
}
// end of RF52_TX_AC_Pole()

// ================================================================================================
// FUNCTION  : RF52_RX_AC_Pole
// ------------------------------------------------------------------------------------------------
// Purpose   : Implement a single AC coupling pole
// Parameters: x    -- i/o signal (by reference)
//             n    -- signal path index
//             pole -- pole filter structure (by reference)
//             t    -- fraction of time in gain switch (high for 1>=t>0, nominal @ t=0)
// ================================================================================================
void RF52_RX_AC_Pole(wiComplex *x, wiInt n, RF52_PoleState_t *pole, wiReal t)
{
    double a[2],b[2];
    wiComplex d;
 
    // --------------------------------
    // Set Filter Cutoff (Dynamic Pole)
    // --------------------------------
    if (t && pole->fc_kHz) // fast pole
    {
        if (RX.AC.Tslide && (t<=RX.AC.t1))
        {
            double fc_kHz, beta;
            fc_kHz = (t/(double)RX.AC.t1)*(RX.AC.fckHz_Slide - pole->fc_kHz) + pole->fc_kHz;
            beta = tan(3.14159265 * fc_kHz / (1000.0*RX.Fs_MHz));

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
// end of RF52_AC_Pole()

// ================================================================================================
// FUNCTION  : RF52_Filter
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the "ideal" bandpass filter
// Parameters: n      -- signal path index
//             t      -- time sample index
//             Filter -- filter state structure (by reference)
//             x      -- i/o signal (by reference)
// ================================================================================================
__inline wiStatus RF52_Filter(wiComplex *x, RF52_FilterState_t *Filter, int n)
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
// end of RF52_Filter()

// ================================================================================================
// FUNCTION  : RF52_TX_Mixer()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the transmit mixer
// Parameters: Nx -- number of samples in x
//             x  -- input/output signal array
// ================================================================================================
wiStatus RF52_TX_Mixer(int Nx, wiComplex *x)
{
    // ----------------------------------------------
    // Mixer: Phase Noise + CFO: x[k] = x*exp(j*a[k])
    // ----------------------------------------------
    if (TX.PLL.Enabled)
    {
        wiComplex y, c; int k;
        for (k=0; k<Nx; k++)
        {
            c.Real = cos(TX.PLL.a[k]);
            c.Imag = sin(TX.PLL.a[k]);

            y = x[k];
            x[k].Real = (c.Real * y.Real) - (c.Imag * y.Imag);
            x[k].Imag = (c.Imag * y.Real) + (c.Real * y.Imag);
        }
    }
    // ------------------
    // I/Q Mismatch Model
    // ------------------
    if (TX.IQ.Enabled || TX.ModelLevel==2)
    {
        double A1 = TX.IQ.A1;
        double A2 = TX.IQ.A2;
        double a1 = pi * TX.IQ.a1_deg / 180.0;
        double a2 = pi * TX.IQ.a2_deg / 180.0;
        double cII, cIQ, cQQ, cQI;
        wiComplex t; int k;

        if (A1!=1.0 || A2 !=1.0 || a1 || a2)
        {
            cII = (1.0 + A1*A2*cos(a1+a2))  / 2.0;
            cIQ =-(A2*sin(a2) + A1*sin(a1)) / 2.0;
            cQI = (      A1*A2*sin(a1+a2))  / 2.0;
            cQQ = (A2*cos(a2) + A1*cos(a1)) / 2.0;

            for (k=0; k<Nx; k++)
            {
                t = x[k];
                x[k].Real = (cII * t.Real) + (cIQ * t.Imag);
                x[k].Imag = (cQI * t.Real) + (cQQ * t.Imag);
            }
        }
    }
    return WI_SUCCESS;
}
// end of RF52_TX_Mixer()

// ================================================================================================
// FUNCTION  : RF52_PowerAmp()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the power amplifier using the Rapp model
// Parameters: Pin -- input power
//             Nx  -- number of samples in x
//             x   -- input/output signal array
// ================================================================================================
wiStatus RF52_PowerAmp(wiReal Pin, int Nx, wiComplex *x)
{
    // ---------------------------
    // Power Amplifier: Rapp Model
    // ---------------------------
    if (TX.PowerAmp.Enabled || TX.ModelLevel==2)
    {
        double p = TX.PowerAmp.p;
        double Psat, Av;
        int k;

        Psat = pow(10.0,TX.PowerAmp.OBO/10.0) * Pin / pow(pow(10,p/10)-1,1/p);

        for (k=0; k<Nx; k++)
        {
            Pin = (x[k].Real * x[k].Real) + (x[k].Imag * x[k].Imag);
            Av = Pin? sqrt(TX.PowerAmp.Ap / pow(1 + pow(Pin/Psat, p), 1/p)) : sqrt(TX.PowerAmp.Ap);

            x[k].Real *= Av;
            x[k].Imag *= Av;
        }
    }
    return WI_SUCCESS;
}
// end of RF52_PowerAmp()

// ================================================================================================
// FUNCTION  : RF52_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the analog TX radio
// Parameters: Ns   -- number of samples in the i/o arrays
//             sIn  -- input signal array (DAC output)
//             sOut -- output signal array (antenna output)
// ================================================================================================
wiStatus RF52_TX(int Ns, wiComplex *sIn, wiComplex *sOut, int nPadBack)
{
    int k;
    double A = 1.0/sqrt(TX.Prms);

    // -------------------
    // Normal Output Power
    // -------------------
    if (TX.ModelLevel < 2)
    {
        for (k=0; k<Ns; k++) 
        {
            sOut[k].Real = A * sIn[k].Real;
            sOut[k].Imag = A * sIn[k].Imag;
        }
    }
    // ---------------------------------------
    // RF52 TX Models by Complexity Level
    // ---------------------------------------
    switch (TX.ModelLevel)
    {
        case 0:
        {
            RF52_TX_Mixer(Ns, sOut);
            RF52_PowerAmp(1.0, Ns, sOut);
        }  break;

        case 1:
        {
            for (k=0; k<Ns; k++) {
                RF52_Filter(sOut+k, &(TX.Filter), 0);
            }
            if (TX.AC.Enabled) {
                for (k=0; k<Ns; k++) {
                    RF52_TX_AC_Pole(sOut+k, 0, TX.AC.s+0);
                    RF52_TX_AC_Pole(sOut+k, 0, TX.AC.s+1);
                }
            }
            RF52_TX_Mixer(Ns, sOut);
            RF52_PowerAmp(1.0, Ns, sOut);
        }  break;

        case 2:
        {
            for (k=0; k<Ns; k++)
            {
                // Transmit Filter
                sOut[k] = sIn[k];
                RF52_TX_AC_Pole  (sOut+k, 0, TX.AC.s+0);
                RF52_AddNoise (sOut+k, TX.Filter.Nrms/sqrt(2));
                RF52_Filter   (sOut+k, &(TX.Filter), 0);
                RF52_AddNoise (sOut+k, TX.Filter.Nrms/sqrt(2));

                // TX Mixer
                RF52_TX_AC_Pole(sOut+k, 0, TX.AC.s+1);
                RF52_AddNoise  (sOut+k, TX.Mixer.Nrms);
            }
            // -----------------
            // Startup-Transient
            // -----------------
            if (TX.Startup.tau) 
            {
                double Av;
                for (k=0; k<Ns; k++)
                {
                    Av = (k<TX.Startup.k0)? 0.0 : 1.0 - exp(-(k-TX.Startup.k0)/TX.Startup.ktau);
                    sOut[k].Real *= Av;
                    sOut[k].Imag *= Av;
                }
            }
            // ---------------
            // Mixer/Predriver
            // ---------------
            RF52_TX_Mixer(Ns, sOut);

            // ----------------
            // Normalize Output
            // ----------------
            for (k=0; k<Ns; k++) 
            {
                sOut[k].Real *= A;
                sOut[k].Imag *= A;
            }
            RF52_PowerAmp (1.0, Ns, sOut);
        }  break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // --------------------------
    // Squelch Pad Samples at End
    // --------------------------
    {
        int k0 = Ns-nPadBack + (int)(TX.Fs_MHz * 1e6 * TX.PowerAmp.DelayShutdown);
        for (k=k0; k<Ns; k++)
            sOut[k].Real = sOut[k].Imag = 0.0;
    }
    return WI_SUCCESS;
}
// end of RF52_TX()

// ================================================================================================
// FUNCTION  : RF52_RX_Mixer
// ------------------------------------------------------------------------------------------------
// Purpose   : Model RX mixer effects such as phase noise and I/Q mismatch
// Parameters: x -- complex I/O signal
//             n -- antenna path {0,1}
//             t -- sample time index
// ================================================================================================
void RF52_RX_Mixer(wiComplex *x, wiInt n, wiInt t)
{
    // ------------------
    // Mixer: Phase Noise
    // ------------------
    if (RX.PLL.Enabled)
    {
        wiComplex y, c; 

        // --------------------------
        // Compute: x = x*exp(j*a[t])
        // --------------------------
        c.Real = cos(RX.PLL.a[t]);
        c.Imag = sin(RX.PLL.a[t]);

        y = *x;
        x->Real = (c.Real * y.Real) - (c.Imag * y.Imag);
        x->Imag = (c.Imag * y.Real) + (c.Real * y.Imag);
    }
    // ------------------
    // Mixer I/Q Mismatch
    // ------------------
    {
        x->Imag = (RX.Mixer.cI * x->Real) + (RX.Mixer.cQ * x->Imag);
    }
}
// end of RF52_RX_Mixer()

// ================================================================================================
// FUNCTION  : RF52_RX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the combined effect of the analog VGA and LPF
// Parameters: n       -- antenna path {0,1}
//             t       -- sample time index
//             rIn     -- input signal from antenna (power/1Ohm)
//             rOut    -- output signal to ADC input
//             SigIn   -- input signals from baseband
//             SigOut  -- output signals to baseband
// ================================================================================================
wiStatus RF52_RX(int n, int t, wiComplex rIn, wiComplex *rOut, RF52_SignalState_t SigIn, RF52_SignalState_t *SigOut)
{
    static wiBoolean   GainChange = 0;
    static wiUReg regLgSigAFE = {0,0};
    static double xACpole, xRFattn; // AC coupling pole location
    static wiUInt dAGain, dLNAGain;
    static double A;

    // ------------------
    // Check Mode Control
    // ------------------
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
        if ((t % RX.M_20MHz)==0) regLgSigAFE.Q = regLgSigAFE.D;

        GainChange = (SigIn.AGAIN!=dAGain || SigIn.LNAGAIN != dLNAGain)? 1:0;
        if (!(t%4)) {dAGain = SigIn.AGAIN; dLNAGain=SigIn.LNAGAIN;}
    }
    // ------------------------
    // Dynamic AC Coupling Pole
    // ------------------------
    if ((t<16 || GainChange) && RX.AC.Dynamic) 
    {
        RX.AC.t1= floor((RX.AC.Tslide              ) * RX.Fs_MHz * 1e6 + 0.5); // time to start slide
        xACpole = floor((RX.AC.Tstep + RX.AC.Tslide) * RX.Fs_MHz * 1e6 + 0.5); // fast pole period
        xRFattn = floor((RX.Squelch.Tstep          ) * RX.Fs_MHz * 1e6 + 0.5); // RF squelch period
    }
    else
    {
        if (xRFattn && !n) xRFattn--;
        if (xACpole && !n) xACpole--;
    }
    // --------------------------------------------------
    // LNA Input: Signal with RFE Gain + Noise (1 Ohm,ref)
    // --------------------------------------------------
    {
        wiReal Av = SigIn.SW1? (RX.RFE.Av[n] * 0.1) : RX.RFE.Av[n];
        Av = RX.RFE.Av[n];
        rOut->Real = (Av * rIn.Real);
        rOut->Imag = (Av * rIn.Imag);

        wiRnd_AddComplexNormal(rOut, RX.Nrms);
    }
    // -------------------------------------
    // Calculate Net Gain for ModelLevel < 2
    // -------------------------------------
    if (RX.ModelLevel!=2)
    {
        A  = RX.RF.sqrtRin;
        A *= RX.RF.Gain[SigIn.LNAGAIN].Av * sqrt(2.0);
        A *= RX.SGA.Gain[SigIn.AGAIN].Av;
        A *= RX.Filter.Gain.Av;
        A *= RX.PGA.Gain.Av;
        A *= RX.ADCBuf.Gain.Av;
        if (RX.LNA2.Enabled) A *= RX.LNA2.Av[SigIn.LNAGAIN2];
    }
    // ------------------------------------
    // RF52 Models by Complexity Level
    // ------------------------------------
    switch (RX.ModelLevel)
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

            if (RX.PLL.Enabled) 
            {
                RF52_RX_Mixer (rOut, n, t);
            }
            if (RX.AC.Enabled)
            {
                RF52_RX_AC_Pole(rOut, n, RX.AC.s+0, xACpole);
                RF52_RX_AC_Pole(rOut, n, RX.AC.s+1, xACpole);
            }
            RF52_Filter(rOut, &(RX.Filter), n);

            // ---------------------
            // Large Signal Detector
            // ---------------------
            RF52_LgSigDetector(rOut, RX.ThLgSigAFE*RX.PGA.Gain.Av, &(regLgSigAFE.D.word));

            if (RX.AC.Enabled) 
                RF52_RX_AC_Pole(rOut, n, RX.AC.s+2, xACpole);
            break;

        // ------------------------------
        // Full Baseband Equivalent Model
        // ------------------------------
        case 2:
        {
            // -----------------------------
            // Power-to-Voltage at LNA Input
            // -----------------------------
            RF52_Amplifier(rOut, RX.RF.sqrtRin);

            // ------------
            // External LNA
            // ------------
            if (RX.LNA2.Enabled)
            {
                RF52_AddNoise (rOut, RX.LNA2.Nrms[SigIn.LNAGAIN2]);
                RF52_Amplifier(rOut, RX.LNA2.Av  [SigIn.LNAGAIN2]);
            }
            // ----------------
            // LNA + Mixer (RF)
            // ----------------
            RF52_AddOffset(rOut, RX.RF.Vo);                                   // Offset due to coupled LO
            if (xRFattn) RF52_Amplifier(rOut, RX.Squelch.Gain[SigIn.LNAGAIN]); // RF Squelch
            RF52_AddNoise (rOut, RX.RF.Nrms[SigIn.LNAGAIN]);
            RF52_Amplifier(rOut, sqrt(2.0));                 // adjust gain to reflect real-to-complex gain
            RF52_NonlinAmp(rOut, RX.RF.Gain+SigIn.LNAGAIN);
            RF52_RX_Mixer (rOut, n, t);
            RF52_AddOffset(rOut, RX.Mixer.Vo);

            // ---
            // SGA
            // ---
            RF52_RX_AC_Pole(rOut, n, RX.AC.s+0, xACpole);
            RF52_AddOffset(rOut, RX.SGA.Vo);
            RF52_AddNoise (rOut, RX.SGA.Nrms[SigIn.AGAIN]);
            RF52_NonlinAmp(rOut, RX.SGA.Gain+SigIn.AGAIN);
            if (n==0) RX.trace[t] = *rOut;

            // ---------------------
            // Channel Select Filter
            // ---------------------
            RF52_RX_AC_Pole(rOut, n, RX.AC.s+1, xACpole);
            RF52_AddOffset(rOut, RX.Filter.Vo);
            RF52_AddNoise (rOut, RX.Filter.Nrms_3dB);
            RF52_NonlinAmp(rOut,&RX.Filter.Gain);
            RF52_Filter   (rOut,&RX.Filter, n);
            RF52_AddNoise (rOut, RX.Filter.Nrms_3dB);

            // ---------------------
            // Large Signal Detector
            // ---------------------
            RF52_LgSigDetector(rOut, RX.ThLgSigAFE, &(regLgSigAFE.D.word));

            // --------------
            // PGA Gain Stage
            // --------------
            RF52_AddOffset(rOut, RX.PGA.Vo);
            RF52_AddNoise (rOut, RX.PGA.Nrms);
            RF52_NonlinAmp(rOut,&RX.PGA.Gain);
            RF52_RX_AC_Pole(rOut, n, RX.AC.s+2, xACpole);

            // ----------------
            // ADC Buffer Stage
            // ----------------
            RF52_AddOffset(rOut, RX.ADCBuf.Vo);
            RF52_AddNoise (rOut, RX.ADCBuf.Nrms);
            RF52_NonlinAmp(rOut,&RX.ADCBuf.Gain);

            RF52_TX_AC_Pole(rOut, n, RX.AC.s+3); // this pole is at the input of the Baseband ADC

        }  break;

        // ------------------------------------------------
        // Filter Only with Single AC coupling (for BERLAB)
        // ------------------------------------------------
        case 4:
            rOut->Real = A * RX.RFE.Av[n] * rIn.Real;
            rOut->Imag = A * RX.RFE.Av[n] * rIn.Imag;
            RF52_TX_AC_Pole(rOut, n, RX.AC.s+0);
            RF52_Filter (rOut, &(RX.Filter), n);
            break;

        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // ------------
    // Wire Outputs
    // ------------
    SigOut->LGSIGAFE = regLgSigAFE.Q.b0;

    return WI_SUCCESS;
}
// end of RF52_RX()

// ================================================================================================
// FUNCTION  : RF52_PhaseFrequencyError()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the baseband equivalent mixer for RF mixer phase/frequency errors
// Parameters: N      - number of time points
//             Fs_MHz - sample rate in MS/s
//             pState - pointer to the PLL state structure
// ================================================================================================
wiStatus RF52_PhaseFrequencyError(wiInt N, wiReal Fs_MHz, RF52_PllState_t *pPLL)
{
    if (pPLL->Enabled)
    {
        wiInt  k;
        wiReal B[8], A[8], Su, fc, fs;

        wiReal *u = wiCalloc(N, sizeof(wiReal));
        if (!u) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

        if (pPLL->a) wiFree(pPLL->a);
        pPLL->a = (wiReal *)wiCalloc(N, sizeof(wiReal));

        fs = Fs_MHz * 1.0E+6;
        Su = sqrt(fs) * pow(10.0, pPLL->PSD/20.0);
        fc = pPLL->fc/fs;

        XSTATUS(wiRnd_Normal(N, u, Su));
        XSTATUS(wiFilter_Butterworth(pPLL->FilterN, fc, B, A, WIFILTER_IMPULSE_INVARIANCE));
        XSTATUS(wiFilter_IIR(pPLL->FilterN, B, A, N, u, pPLL->a, NULL));
        wiFree(u);

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
                    pPLL->a[k] += (double)k * w;

                if (tau) 
                {
                    for (; k<N; k++) {
                        dk = k - k0;
                        pPLL->a[k] += (double)k * (w + dw * sin(wc*dk) * exp(-(double)dk/tau));
                    }
                }
                else 
                {
                    for (; k<N; k++) {
                        dk = k - k0;
                        pPLL->a[k] += (double)k * w + dw/wc * cos(wc*dk);
                    }
                }
            }
            else
            {
                for (k=0; k<N; k++)
                    pPLL->a[k] += (double)k * w;
            }
        }
    }
    return WI_SUCCESS;
}
// end of RF52_PhaseFrequencyError()
        
// ================================================================================================
// FUNCTION  : RF52_TX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reset/configure the transmit model. This clears all memory elements and
//             calculates per-packet parameters.
// Parameters: N      -- number of samples to be processed
//             Fs_MHz -- sample rate (MHz)
//             Prms   -- complex input signal power
// ================================================================================================
wiStatus RF52_TX_Restart(int N, double Fs_MHz, double Prms)
{
    int i,j,k;

    TX.Fs_MHz = Fs_MHz;
    TX.Prms   = Prms;

    // -----------
    // Noise Setup
    // -----------
    {
        double rtHz = sqrt(TX.Fs_MHz * 1.0E+6);
        TX.Filter.Nrms = TX.Filter.Ni * 1e-9 * rtHz * sqrt(2.0);
        TX.Mixer.Nrms  = TX.Mixer.Ni * 1e-9 * rtHz * sqrt(2.0);
    }
    if (TX.ModelLevel>0)
    {
        double fc0 = (TX.Filter.fc / TX.Fs_MHz) * (1+TX.Filter.fcMismatch/2);
        double fc1 = (TX.Filter.fc / TX.Fs_MHz) * (1-TX.Filter.fcMismatch/2);
        XSTATUS(wiFilter_Butterworth(TX.Filter.N, fc0, TX.Filter.b[0], TX.Filter.a[0], WIFILTER_IMPULSE_INVARIANCE) );
        XSTATUS(wiFilter_Butterworth(TX.Filter.N, fc1, TX.Filter.b[1], TX.Filter.a[1], WIFILTER_IMPULSE_INVARIANCE) );
        for (i=0; i<2; i++) for (j=0; j<2; j++) TX.Filter.w[i][j].Real = TX.Filter.w[i][j].Imag = 0.0;
    }
    // -----------------
    // Startup Transient
    // -----------------
    TX.Startup.k0   = (int)(TX.Startup.t0 * (Fs_MHz * 1.0E+6) + 0.5);
    TX.Startup.ktau = TX.Startup.tau * (Fs_MHz * 1.0E+6);

    // ------------------
    // AC Coupling Filter
    // ------------------
    if (TX.AC.Enabled || TX.ModelLevel==2)
    {
        for (i=0; i<2; i++)
        {
            double betaR = tan(3.14159265 * (TX.AC.s[i].fc_kHz*(1+TX.AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            double betaI = tan(3.14159265 * (TX.AC.s[i].fc_kHz*(1-TX.AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            TX.AC.s[i].b[0] = 1.0/(1.0+betaR);
            TX.AC.s[i].a[0] = (betaR-1.0)/(betaR+1.0);
            TX.AC.s[i].b[1] = 1.0/(1.0+betaI);
            TX.AC.s[i].a[1] = (betaI-1.0)/(betaI+1.0);
            for (j=0; j<2; j++) {
                for (k=0; k<2; k++) {
                    TX.AC.s[i].w[j][k].Real  = TX.AC.s[i].w[j][k].Imag  = 0.0;
                    TX.AC.s[i].yD[j][k].Real = TX.AC.s[i].yD[j][k].Imag = 0.0;
                    TX.AC.s[i].yD[j][k].Real = TX.AC.s[i].yD[j][k].Imag = 0.0;
                }
            }
        }
    }
    // -------------------------
    // Clear the filter memories
    // -------------------------
    for (i=0; i<1; i++) {
        for (j=0; j<10; j++)
            TX.Filter.w[i][j].Real = TX.Filter.w[i][j].Imag = 0.0;
    }
    // -----------
    // Mixer Setup
    // -----------
    XSTATUS(RF52_PhaseFrequencyError(N, Fs_MHz, &(TX.PLL)));

    return WI_SUCCESS;
}
// end of RF52_TX_Restart()

// ================================================================================================
// FUNCTION  : RF52_RX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Restart the receiver model for a new packet/signal stream. All memory
//             elements are cleared.
// Parameters: N      -- number of samples to be processed
//             Fs_MHz -- sample rate (MHz)
//             N_Rx   -- number of receive paths/antennas {1,2}
// ================================================================================================
wiStatus RF52_RX_Restart(int N, double Fs_MHz, int N_Rx)
{
    int i, j, k;

    RX.Fs_MHz = Fs_MHz;
    RX.M_20MHz = (int)floor(Fs_MHz/20.0 + 1e-9);

    if (InvalidRange(N,      1, 1e9    ))  return STATUS(WI_ERROR_PARAMETER1);
    if (InvalidRange(Fs_MHz, 1, 1000000)) return STATUS(WI_ERROR_PARAMETER2); 
    if (InvalidRange(N_Rx,   1, 2      ))    return STATUS(WI_ERROR_PARAMETER3);

    // --------------
    // Trace Variable
    // --------------
    if (RX.trace) wiFree(RX.trace);
    RX.trace = (wiComplex *)wiCalloc(N, sizeof(wiComplex));
    if (!RX.trace) return STATUS(WI_ERROR_UNDEFINED_CASE);
    RX.Ntrace= N;

    // ---------------
    // Radio Front End
    // ---------------
    RX.RFE.Av[0] = pow(10.0, RX.RFE.PdB[0]/20.0);
    RX.RFE.Av[1] = pow(10.0, RX.RFE.PdB[1]/20.0);
    RX.RF.sqrtRin = sqrt(RX.RF.Rin);
    RX.RF.Vo      = pow(10,(RX.RF.Pc-30-3)/20) * sqrt(RX.RF.Rin); // -3dB for Vo(1+j);

    // -----------------------------
    // Receiver Input Referred Noise
    // -----------------------------
    {
        double T, B = Fs_MHz * 1.0E+6;         // bandwidth (Hz)
        if (RX.ModelLevel<2) T = 290.0;
        else                T = RX.T_degK;
        RX.Nrms    = sqrt(kBoltzmann*T*B);     // rms noise amplitude (1 Ohm ref.)
        if (RX.ModelLevel<2)
            RX.Nrms*= pow(10, RX.NFdB/20.0);    // noise figure for models 0,1
    }
    // -----------
    // Mixer Setup
    // -----------
    {
        double A = pow(10.0, RX.Mixer.Gain_dB/20.0);

        RX.Mixer.cI = A * sin(pi/180.0 * RX.Mixer.PhaseDeg);
        RX.Mixer.cQ = A * cos(pi/180.0 * RX.Mixer.PhaseDeg);

        XSTATUS(RF52_PhaseFrequencyError(N, Fs_MHz, &(RX.PLL)));
    }
    // -----------
    // Noise Setup
    // -----------
    {
        double T, B = Fs_MHz * 1.0E+6;         // bandwidth (Hz)
        double rtHz = sqrt(B);
        if (RX.ModelLevel<2) T = 290.0;
        else                T = RX.T_degK;

        RX.LNA2.Nrms[0]= sqrt(kBoltzmann*T*B) * (pow(10.0, RX.LNA2.NFdB[0]/20.0) - 1.0);
        RX.LNA2.Nrms[1]= sqrt(kBoltzmann*T*B) * (pow(10.0, RX.LNA2.NFdB[1]/20.0) - 1.0);
        RX.RF.Nrms[0]  = RX.RF.Ni[0]  * 1E-9 * rtHz;
        RX.RF.Nrms[1]  = RX.RF.Ni[1]  * 1E-9 * rtHz;
        RX.PGA.Nrms    = RX.PGA.Ni    * 1E-9 * rtHz;
        RX.Filter.Nrms = RX.Filter.Ni * 1E-9 * rtHz;
        RX.ADCBuf.Nrms = RX.ADCBuf.Ni * 1E-9 * rtHz;

        RX.Filter.Nrms_3dB = RX.Filter.Nrms / sqrt(2.0); // precompute half-power noise level

        for (i=0; i<64; i++) {
            RX.SGA.Nrms[i] = RX.SGA.Ni[i] * 1E-9 * rtHz;
        }
    }
    // --------------
    // Lowpass Filter
    // --------------
    {
        double fc0 = (RX.Filter.fc / RX.Fs_MHz) * (1+RX.Filter.fcMismatch/2);
        double fc1 = (RX.Filter.fc / RX.Fs_MHz) * (1-RX.Filter.fcMismatch/2);
        XSTATUS(wiFilter_Butterworth(RX.Filter.N, fc0, RX.Filter.b[0], RX.Filter.a[0], WIFILTER_IMPULSE_INVARIANCE) );
        XSTATUS(wiFilter_Butterworth(RX.Filter.N, fc1, RX.Filter.b[1], RX.Filter.a[1], WIFILTER_IMPULSE_INVARIANCE) );
    }
    // ------------------
    // AC Coupling Filter
    // ------------------
    if (RX.AC.Enabled || RX.ModelLevel>=2)
    {
        for (i=0; i<4; i++)
        {
            double betaR = tan(3.14159265 * (RX.AC.s[i].fc_kHz*(1+RX.AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            double betaI = tan(3.14159265 * (RX.AC.s[i].fc_kHz*(1-RX.AC.fcMismatch/2)) / (1000.0*Fs_MHz));
            RX.AC.s[i].b[0] = 1.0/(1.0+betaR);
            RX.AC.s[i].a[0] = (betaR-1.0)/(betaR+1.0);
            RX.AC.s[i].b[1] = 1.0/(1.0+betaI);
            RX.AC.s[i].a[1] = (betaI-1.0)/(betaI+1.0);

            for (j=0; j<2; j++) 
                for (k=0; k<2; k++) {
                    RX.AC.s[i].w[j][k].Real  = RX.AC.s[i].w[j][k].Imag  = 0.0;
                    RX.AC.s[i].yD[j][k].Real = RX.AC.s[i].yD[j][k].Imag = 0.0;
                    RX.AC.s[i].yD[j][k].Real = RX.AC.s[i].yD[j][k].Imag = 0.0;
                    RX.AC.s[i].xD[j][k].Real = RX.AC.s[i].xD[j][k].Imag = 0.0;
                }
        }
    }
    // -------------------------
    // Clear the filter memories
    // -------------------------
    for (i=0; i<2; i++) {
        for (j=0; j<10; j++) 
            RX.Filter.w[i][j].Real = RX.Filter.w[i][j].Imag = 0.0;
    }
    return WI_SUCCESS;
}
// end of RF52_RX_Restart()

// ================================================================================================
// FUNCTION  : RF52_CalcNonlinAmpModel()
// ------------------------------------------------------------------------------------------------
// Purpose   : Calculate parameters for the nonlinear amplifier model
// Parameters: 
// ================================================================================================
void RF52_CalcNonlinAmpModel(RF52_GainState_t *G)
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
// end of RF52_CalcNonlinAmpModel()

// ================================================================================================
// FUNCTION  : RF52_SetGainState()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// Parameters: 
// ================================================================================================
void RF52_SetGainState(RF52_GainState_t *G, wiReal Av, wiReal IIV2, wiReal IIV3)
{
    G->Av   = Av;
    G->IIV2 = IIV2;
    G->IIV3 = IIV3;
    RF52_CalcNonlinAmpModel(G);
}
// end of RF52_SetGainState()

// ================================================================================================
// FUNCTION  : RF52_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the RF52 model; setup default parameter values.
// Parameters: none
// ================================================================================================
wiStatus RF52_Init()
{
    int i;
    if (ModuleIsInit) return WI_WARN_MODULE_ALREADY_INIT;

    // -------------------
    // Default Model Level
    // -------------------
    RX.ModelLevel = 0;
    TX.ModelLevel = 0;

    // -----------------
    // Model Sample Rate
    // -----------------
    RX.Fs_MHz = 80.0;
    TX.Fs_MHz = 80.0;

    // ---------------
    // Front End Noise
    // ---------------
    RX.T_degK = 290.0;

    // ---------------
    // Radio Front End
    // ---------------
    RX.RFE.PdB[0] = -2.0;
    RX.RFE.PdB[1] = -2.0;

    // -------------------
    // External/Second LNA
    // -------------------
    RX.LNA2.Enabled = 0;
    RX.LNA2.Av[0] = 1.0;
    RX.LNA2.Av[1] = 1.0;
    RX.LNA2.NFdB[0] = 0.0;
    RX.LNA2.NFdB[1] = 0.0;

    // ----------------
    // LNA + Mixer (RF)
    // ----------------
    RX.RF.Rin     = 50.0;
    RX.RF.Ni[1]   =  0.459;
    RX.RF.Ni[0]   =  1.748;
    RX.RF.Pc      = -70; // dBm
    RX.Mixer.Gain_dB = 0.2;
    RX.Mixer.PhaseDeg= 1.0;
    RX.Mixer.Vo      = 0.01;

    // ---
    // SGA
    // ---
    RX.SGA.A0    = 1.0/64.0;
    RX.SGA.Vmax  = 2.0;
    RX.SGA.Vo    = 0.01;

    RX.SGA.Ni[ 0] = 2328;
    RX.SGA.Ni[ 1] = 1957;
    RX.SGA.Ni[ 2] = 1646;
    RX.SGA.Ni[ 3] = 1384;
    RX.SGA.Ni[ 4] = 1164;
    RX.SGA.Ni[ 5] = 979;
    RX.SGA.Ni[ 6] = 823;
    RX.SGA.Ni[ 7] = 692;
    RX.SGA.Ni[ 8] = 582;
    RX.SGA.Ni[ 9] = 489;
    RX.SGA.Ni[10] = 411;
    RX.SGA.Ni[11] = 346;
    RX.SGA.Ni[12] = 302;
    RX.SGA.Ni[13] = 265;
    RX.SGA.Ni[14] = 238;
    RX.SGA.Ni[15] = 180;
    RX.SGA.Ni[16] = 158;
    RX.SGA.Ni[17] = 139;
    RX.SGA.Ni[18] = 125;
    RX.SGA.Ni[19] = 95;
    RX.SGA.Ni[20] = 84;
    RX.SGA.Ni[21] = 74;
    RX.SGA.Ni[22] = 67;
    RX.SGA.Ni[23] = 53;
    RX.SGA.Ni[24] = 47;
    RX.SGA.Ni[25] = 42;
    RX.SGA.Ni[26] = 39;
    RX.SGA.Ni[27] = 34;
    RX.SGA.Ni[28] = 31;
    RX.SGA.Ni[29] = 29;
    RX.SGA.Ni[30] = 28;
    RX.SGA.Ni[31] = 21;
    RX.SGA.Ni[32] = 15;
    RX.SGA.Ni[33] = 15;
    RX.SGA.Ni[34] = 15;
    RX.SGA.Ni[35] = 15;
    RX.SGA.Ni[36] = 15;
    RX.SGA.Ni[37] = 15;
    RX.SGA.Ni[38] = 15;
    RX.SGA.Ni[39] = 15;
    RX.SGA.Ni[40] = 15;
    RX.SGA.Ni[41] = 15;
    RX.SGA.Ni[42] = 15;
    RX.SGA.Ni[43] = 15;
    RX.SGA.Ni[44] = 15;
    RX.SGA.Ni[45] = 15;
    RX.SGA.Ni[46] = 15;
    RX.SGA.Ni[47] = 15;

    // ---
    // PGA
    // ---
    RX.PGA.Ni    = 65.0;
    RX.PGA.Vo    = 0.01;

    // ----------
    // ADC Buffer
    // ----------
    RX.ADCBuf.Ni = 100;
    RX.ADCBuf.Vo = 0.01;

    // -------------
    // RF52 Filters
    // -------------
    RX.Filter.N         = 5;
    RX.Filter.fc        = 10;
    RX.Filter.fcMismatch= 0.01;
    RX.Filter.Ni        = 54.8;
    RX.Filter.Vo        = 0.02;

    // --------------
    // RX Phase Noise
    // --------------
    RX.PLL.Enabled = 1;
    RX.PLL.CFO_ppm = 0;               // DO NOT USE (Keep at 0)
    RX.PLL.Transient.PeakCFO_ppm = 0; // DO NOT USE (Keep at 0)
    RX.PLL.FilterN = 1;
    RX.PLL.fc = 225.0E+3;
    RX.PLL.PSD = -94.5;

    // ---------------------
    // Large Signal Detector
    // ---------------------
    RX.ThLgSigAFE = 0.7;

    // --------------
    // TX Phase Noise
    // --------------
    TX.PLL.Enabled = 1;
    TX.PLL.f_MHz = 5800;
    TX.PLL.CFO_ppm = 0;
    TX.PLL.Transient.PeakCFO_ppm = 0;
    TX.PLL.Transient.fc = 280.0E+3;
    TX.PLL.Transient.tau = 1.6E-6;
    TX.PLL.Transient.k0 = 1980;
    TX.PLL.FilterN = 1;
    TX.PLL.fc = 225.0E+3;
    TX.PLL.PSD = -94.5;

    TX.IQ.A1 = 1.0;
    TX.IQ.A2 = 1.0;
    TX.IQ.a1_deg =  0;
    TX.IQ.a2_deg =  0;

    RX.Squelch.Enabled = 1;
    RX.Squelch.Gain[0] = 0.01;   // -40 dB
    RX.Squelch.Gain[1] = 0.01;   // -40 dB
    RX.Squelch.Tstep   = 325e-9; // squelch period = 325ns

    RX.AC.fcMismatch = 0.00;
    RX.AC.Dynamic = 1;      // enable dynamic poles by default
    RX.AC.Tstep   = 0.3e-6; // fast-pole step
    RX.AC.Tslide  = 0.0;
    RX.AC.fckHz_Slide = 1200;
    for (i=0; i<4; i++) RX.AC.s[i].fc_kHz = 20.0;
    RX.AC.s[3].fc_kHz = 0.0;

    TX.AC.Enabled = 1;
    TX.AC.fcMismatch = 0.00;
    TX.AC.s[0].fc_kHz = 20.0;
    TX.AC.s[1].fc_kHz = 20.0;

    TX.Filter.N         = 3;
    TX.Filter.fc        = 20;      
    TX.Filter.fcMismatch= 0.01;
    TX.Filter.Ni        = 42.4;
    TX.Mixer.Ni         = 0.0;

    TX.PowerAmp.Enabled = 0;
    TX.PowerAmp.OBO = 10;
    TX.PowerAmp.p = 2;
    TX.PowerAmp.Ap = 1;
    TX.PowerAmp.DelayShutdown = 2e-6; // end-of-packet to TX shutdown

    // --------------------------------
    // New RX Polynomial Amp Parameters
    // --------------------------------
    RF52_SetGainState(&(RX.RF.Gain[0]),   2.51,   3.5,  0.85);
    RF52_SetGainState(&(RX.RF.Gain[1]),  28.30,   0.53, 0.071);

    RF52_SetGainState(&(RX.Filter.Gain),  0.94, 135.0, 5.0 );
    RF52_SetGainState(&(RX.PGA.Gain),     4.00,  15.0, 5.0 );
    RF52_SetGainState(&(RX.ADCBuf.Gain),  0.89,  30.0, 9.0 );
    
    for (i=0; i<64; i++)
        RF52_SetGainState(RX.SGA.Gain+i, pow(2.0,(double)(min(i,47)-24)/4.0), 30, 10);

    // ---------------------------
    // Register the wiParse method
    // ---------------------------
    XSTATUS(wiParse_AddMethod(RF52_ParseLine));

    ModuleIsInit = 1;
    return WI_SUCCESS;
}
// end of RF52_Init()

// ================================================================================================
// FUNCTION  : RF52_Close()
// ------------------------------------------------------------------------------------------------
// Purpose   : Close the RF52 model
// Parameters: none
// ================================================================================================
wiStatus RF52_Close()
{
    if (!ModuleIsInit) return WI_WARN_MODULE_NOT_INITIALIZED;

    if (TX.PLL.a) wiFree(TX.PLL.a);     TX.PLL.a = NULL;
    if (RX.PLL.a) wiFree(RX.PLL.a);     RX.PLL.a = NULL;
    WIFREE( RX.trace );

    XSTATUS(wiParse_RemoveMethod(RF52_ParseLine));

    ModuleIsInit = 0;
    return WI_SUCCESS;
}
// end of RF52_Close()

// ================================================================================================
// FUNCTION : RF52_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus RF52_StatePointer(RF52_TxState_t **pTX, RF52_RxState_t **pRX)
{
    if (pTX) *pTX = &TX;
    if (pRX) *pRX = &RX;
    return WI_SUCCESS;
}
// end of RF52_StatePointer()

// ================================================================================================
// FUNCTION : RF52_TxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
RF52_TxState_t * RF52_TxState(void)
{
    return &TX;
}
// end of RF52_TxState()

// ================================================================================================
// FUNCTION : RF52_RxState()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
RF52_RxState_t * RF52_RxState(void)
{
    return &RX;
}
// end of RF52_RxState()

// ================================================================================================
// FUNCTION : RF52_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static wiStatus RF52_ParseLine(wiString text)
{
    wiStatus status; wiReal x; wiInt i;

    if (!strstr(text, "RF52")) return WI_WARN_PARSE_FAILED; // not an RF52 parameter

    PSTATUS(wiParse_Int    (text,"RF52.RX.ModelLevel",    &(RX.ModelLevel), 0, 4));
    PSTATUS(wiParse_Int    (text,"RF52.TX.ModelLevel",    &(TX.ModelLevel), 0, 2));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RFE.PdB[0]",    &(RX.RFE.PdB[0]),-20.0, 0.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RFE.PdB[1]",    &(RX.RFE.PdB[1]),-20.0, 0.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.T_degK",        &(RX.T_degK),  0.0, 10000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.NFdB",          &(RX.NFdB), -100.0,   100.0));

    PSTATUS(wiParse_Boolean(text,"RF52.RX.LNA2.Enabled",  &(RX.LNA2.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.RX.LNA2.Av[0]",    &(RX.LNA2.Av[0]), 0, 1e3));
    PSTATUS(wiParse_Real   (text,"RF52.RX.LNA2.Av[1]",    &(RX.LNA2.Av[1]), 0, 1e3));
    PSTATUS(wiParse_Real   (text,"RF52.RX.LNA2.NFdB[0]",  &(RX.LNA2.NFdB[0]),0.0,20.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.LNA2.NFdB[1]",  &(RX.LNA2.NFdB[0]),0.0,20.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Av[0]",      &(RX.RF.Gain[0].Av), 1e-3,   1e3));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Av[1]",      &(RX.RF.Gain[1].Av), 1e-3,   1e3));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Ni[0]",      &(RX.RF.Ni[0]),  0.0, 100.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Ni[1]",      &(RX.RF.Ni[1]),  0.0, 100.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Vmax[0]",    &(RX.RF.Gain[0].Vmax),0.01, 10.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Vmax[1]",    &(RX.RF.Gain[1].Vmax),0.01, 10.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.RF.Pc",         &(RX.RF.Pc), -1000.0, 0.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.Mixer.PhaseDeg",&(RX.Mixer.PhaseDeg), -45.0,  45.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Mixer.Gain_dB", &(RX.Mixer.Gain_dB),   -6.0,   6.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.SGA.A0",        &(RX.SGA.A0), 1e-2, 1e2));
    PSTATUS(wiParse_Real   (text,"RF52.RX.SGA.Vmax",      &(RX.SGA.Vmax), 1e-2,   10.0));
    PSTATUS(wiParse_RealArray(text,"RF52.RX.SGA.Ni",      RX.SGA.Ni, 64, 0.0, 10000.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.PGA.Av",        &(RX.PGA.Gain.Av),    0.1,  100.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.PGA.Ni",        &(RX.PGA.Ni),    0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.PGA.Vmax",      &(RX.PGA.Gain.Vmax),  0.01,  10.0));

    PSTATUS(wiParse_Real   (text,"RF52.ThLgSigAFE",       &(RX.ThLgSigAFE), 0.0, 1E+6));

    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.Av",     &(RX.Filter.Gain.Av),0.01, 100.0));
    PSTATUS(wiParse_Int    (text,"RF52.RX.Filter.N",      &(RX.Filter.N),    1, 7));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.fcMHz",  &(RX.Filter.fc),   1, 35));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.fc",     &(RX.Filter.fc),   1, 35));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.Ni",     &(RX.Filter.Ni), 0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.Vmax",   &(RX.Filter.Gain.Vmax), 0.01, 10.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.fcMismatch",&(RX.Filter.fcMismatch),   -0.5, 0.5));

    PSTATUS(wiParse_Real   (text,"RF52.RX.ADCBuf.Av",     &(RX.ADCBuf.Gain.Av),    0.1,  100.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.ADCBuf.Ni",     &(RX.ADCBuf.Ni),    0.0, 1000.0));

    PSTATUS(wiParse_Real   (text,"RF52.RX.Mixer.Vo",      &(RX.Mixer.Vo),  0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.SGA.Vo",        &(RX.SGA.Vo),    0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Filter.Vo",     &(RX.Filter.Vo), 0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.PGA.Vo",        &(RX.PGA.Vo),    0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.ADCBuf.Vo",     &(RX.ADCBuf.Vo), 0.0, 1000.0));

    PSTATUS(wiParse_Boolean(text,"RF52.RX.Squelch.Enabled",&(RX.Squelch.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Squelch.Gain[0]",&(RX.Squelch.Gain[0]), 0.0, 1.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Squelch.Gain[1]",&(RX.Squelch.Gain[1]), 0.0, 1.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.Squelch.Tstep",  &(RX.Squelch.Tstep), 0.0, 1e-6));

    PSTATUS(wiParse_Boolean(text,"RF52.RX.AC.Enabled",    &(RX.AC.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.fcMismatch", &(RX.AC.fcMismatch),   -0.5, 0.5));
    PSTATUS(wiParse_Boolean(text,"RF52.RX.AC.Dynamic",    &(RX.AC.Dynamic)));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.Tstep",      &(RX.AC.Tstep),  0.0,  1e-6));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.Tslide",     &(RX.AC.Tslide), 0.0, 10e-6));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.fckHz_Slide",&(RX.AC.fckHz_Slide), 0.0, 10e3));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.s[0].fc_kHz",&(RX.AC.s[0].fc_kHz), 0.0,10000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.s[1].fc_kHz",&(RX.AC.s[1].fc_kHz), 0.0,10000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.s[2].fc_kHz",&(RX.AC.s[2].fc_kHz), 0.0,10000.0));
    PSTATUS(wiParse_Real   (text,"RF52.RX.AC.s[3].fc_kHz",&(RX.AC.s[3].fc_kHz), 0.0,10000.0));
    
    PSTATUS(wiParse_Boolean(text,"RF52.TX.AC.Enabled",    &(TX.AC.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.TX.AC.fcMismatch", &(TX.AC.fcMismatch),   -0.5, 0.5));
    PSTATUS(wiParse_Real   (text,"RF52.TX.AC.s[0].fc_kHz",&(TX.AC.s[0].fc_kHz), 0.0, 2000.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.AC.s[1].fc_kHz",&(TX.AC.s[1].fc_kHz), 0.0, 2000.0));

    PSTATUS(wiParse_Int    (text,"RF52.TX.Filter.N",      &(TX.Filter.N), 1, 7));
    PSTATUS(wiParse_Real   (text,"RF52.TX.Filter.fcMHz",  &(TX.Filter.fc), 1.0, 40.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.Filter.fc",     &(TX.Filter.fc), 1.0, 40.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.Filter.Ni",     &(TX.Filter.Ni), 0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.Filter.fcMismatch",&(TX.Filter.fcMismatch),   -0.5, 0.5));

    PSTATUS(wiParse_Boolean(text,"RF52.TX.IQ.Enabled",    &(TX.IQ.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.TX.IQ.A1",         &(TX.IQ.A1), 0.5, 2.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.IQ.A2",         &(TX.IQ.A2), 0.5, 2.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.IQ.a1_deg",     &(TX.IQ.a1_deg), -15.0, 15.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.IQ.a2_deg",     &(TX.IQ.a2_deg), -15.0, 15.0));

    PSTATUS(wiParse_Real   (text,"RF52.TX.Mixer.Ni",      &(TX.Mixer.Ni), 0, 1000.0));

    PSTATUS(wiParse_Boolean(text,"RF52.TX.PLL.Enabled",   &(TX.PLL.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.CFO_ppm",   &(TX.PLL.CFO_ppm), -1000.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.f_MHz",     &(TX.PLL.f_MHz),   1.0, 1.0E+4));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.fc",        &(TX.PLL.fc), 1.0, 10.0E+6));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.PSD",       &(TX.PLL.PSD), -200.0, 0.0));
    PSTATUS(wiParse_Int    (text,"RF52.TX.PLL.FilterN",   &(TX.PLL.FilterN), 1, 9));

    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.Transient.PeakCFO_ppm",&(TX.PLL.Transient.PeakCFO_ppm), 0.0, 1000.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.Transient.fc",         &(TX.PLL.Transient.fc),          1.0, 20E+6));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PLL.Transient.tau",        &(TX.PLL.Transient.tau),         0.0, 1E-3));
    PSTATUS(wiParse_Int    (text,"RF52.TX.PLL.Transient.k0",         &(TX.PLL.Transient.k0),         -1000, 100000));

    PSTATUS(wiParse_Boolean(text,"RF52.RX.PLL.Enabled",      &(RX.PLL.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.RX.PLL.fc",           &(RX.PLL.fc), 1.0, 10.0E+6));
    PSTATUS(wiParse_Real   (text,"RF52.RX.PLL.PSD",          &(RX.PLL.PSD), -200.0, 0.0));
    PSTATUS(wiParse_Int    (text,"RF52.RX.PLL.FilterN",      &(RX.PLL.FilterN), 1, 9));

    PSTATUS(wiParse_Boolean(text,"RF52.TX.PowerAmp.Enabled", &(TX.PowerAmp.Enabled)));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PowerAmp.p",       &(TX.PowerAmp.p), 0.1, 100.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PowerAmp.OBO",     &(TX.PowerAmp.OBO), 0, 40.0));
    PSTATUS(wiParse_Real   (text,"RF52.TX.PowerAmp.Ap",      &(TX.PowerAmp.Ap), 1.0, 2.0));

    PSTATUS(wiParse_Real   (text,"RF52.TX.Startup.t0",       &(TX.Startup.t0),  0.0, 0.001));
    PSTATUS(wiParse_Real   (text,"RF52.TX.Startup.tau",      &(TX.Startup.tau), 0.0, 0.001));

    status = STATUS(wiParse_Real(text,"RF52.PLL.PSD",&x, -200,0.0));
    if (status == WI_SUCCESS) {
        TX.PLL.PSD = RX.PLL.PSD = x;
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Int(text,"RF52.PLL.FilterN",&i, 1, 7));
    if (status == WI_SUCCESS) {
        TX.PLL.FilterN = RX.PLL.FilterN = i;
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Real(text,"RF52.PLL.fc", &x, 1.0, 10.0E+6));
    if (status == WI_SUCCESS) {
        TX.PLL.fc = RX.PLL.fc = x;
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Real(text,"RF52.PLL.f_MHz", &x, 1.0, 1.0E+4));
    if (status == WI_SUCCESS) {
        TX.PLL.f_MHz = RX.PLL.f_MHz = x;
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Real(text,"RF52.AC.fc_kHz", &x, 0.0, 1.0E+4));
    if (status == WI_SUCCESS) {
        int i;
        for (i=0; i<2; i++) TX.AC.s[i].fc_kHz = x;
        for (i=0; i<3; i++) RX.AC.s[i].fc_kHz = x;
        return WI_SUCCESS;
    }
    // ----------------------------
    // Set Stage Gain and Linearity
    // ----------------------------
    {
        double Av, IIV2, IIV3;
        char paramstr[128]; int i;

        XSTATUS( status = wiParse_Function(text,"RF52_SetGainState(%127s, %f, %f, %f)",&paramstr, &Av, &IIV2, &IIV3) );
        if (status == WI_SUCCESS)
        {
                 if (!strcmp(paramstr, "RX.RF.Gain[0]" )) RF52_SetGainState(&(RX.RF.Gain[0]),  Av, IIV2, IIV3);
            else if (!strcmp(paramstr, "RX.RF.Gain[1]" )) RF52_SetGainState(&(RX.RF.Gain[1]),  Av, IIV2, IIV3);
            else if (!strcmp(paramstr, "RX.Filter.Gain")) RF52_SetGainState(&(RX.Filter.Gain), Av, IIV2, IIV3);
            else if (!strcmp(paramstr, "RX.PGA.Gain"   )) RF52_SetGainState(&(RX.PGA.Gain),    Av, IIV2, IIV3);
            else if (!strcmp(paramstr, "RX.ADCBuf.Gain")) RF52_SetGainState(&(RX.ADCBuf.Gain), Av, IIV2, IIV3);
            else if (!strcmp(paramstr, "RX.SGA.Gain"))  
            {
                for (i=0; i<64; i++)
                    RF52_SetGainState(RX.SGA.Gain+i, pow(2.0,(double)(min(i,47)-24)/4.0), IIV2, IIV3);
            }
            else 
                return STATUS(WI_ERROR_PARAMETER1);

            return WI_SUCCESS;
        }
    }
    return WI_WARN_PARSE_FAILED;
}
// end of RF52_ParseLine()

// ================================================================================================
// FUNCTION : RF52_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus RF52_WriteConfig(wiMessageFn MsgFunc)
{
    int i;

    // ----------------------
    // Integrated Phase Noise
    // ----------------------
    double PnRX = (RX.PLL.PSD + 10*log10(RX.PLL.fc*4*pi/(4*RX.PLL.FilterN*sin(pi/(2*RX.PLL.FilterN)))));
    double PnTX = (TX.PLL.PSD + 10*log10(TX.PLL.fc*4*pi/(4*TX.PLL.FilterN*sin(pi/(2*TX.PLL.FilterN)))));

	// ---------------------
	// RX Mixer Image Reject
	// ---------------------
    double A = pow(10.0, RX.Mixer.Gain_dB/20.0);
    double AA = 1 + A*A;
	double BB = 2*A*cos(RX.Mixer.PhaseDeg * pi/180);
	double IMRdB = 10*log10( (AA+BB)/(AA-BB) );
    
    // --------
    // Receiver
    // --------
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" RF52 Radio Model      : TX.ModelLevel = %d, RX.ModelLevel = %d\n",TX.ModelLevel,RX.ModelLevel);
    if (RX.T_degK )
        if (RX.ModelLevel<2)
            MsgFunc(" RX System Noise       : NF=%1.1fdB\n",RX.NFdB);
        else
            MsgFunc(" RX System Noise       : T=%3.0f degK (Ni=%1.0fdBm/Hz, NF=%1.1fdB)\n",RX.T_degK,30+10*log10(kBoltzmann*RX.T_degK),10*log10(RX.T_degK/290));
    else
        MsgFunc(" RX System Noise       : T=%3.0f degK (Noiseless)\n",RX.T_degK);

    switch (RX.ModelLevel)
    {
        case 0:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-RX.RFE.PdB[0],-RX.RFE.PdB[1]);
            if (RX.LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(RX.LNA2.Av[1]),DB20(RX.LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(RX.RF.Gain[1].Av),DB20(RX.RF.Gain[0].Av),RX.RF.Rin);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(RX.SGA.A0),DB20(RX.PGA.Gain.Av),DB20(RX.ADCBuf.Gain.Av));
            break;
        case 1:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-RX.RFE.PdB[0],-RX.RFE.PdB[1]);
            if (RX.LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(RX.LNA2.Av[1]),DB20(RX.LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(RX.RF.Gain[1].Av),DB20(RX.RF.Gain[0].Av),RX.RF.Rin);
            if (RX.PLL.Enabled) 
                MsgFunc("    Phase Noise        : %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",PnRX,RX.PLL.PSD,RX.PLL.fc/1e3,RX.PLL.FilterN);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(RX.SGA.A0),DB20(RX.PGA.Gain.Av),DB20(RX.ADCBuf.Gain.Av));
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",RX.Filter.N,RX.Filter.fc,100.0*RX.Filter.fcMismatch);
            if (RX.AC.Enabled)
                MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",RX.AC.s[0].fc_kHz,RX.AC.s[1].fc_kHz,RX.AC.s[2].fc_kHz,100.0*RX.AC.fcMismatch);
                MsgFunc("                       : Dynamic = %d, Tstep=%1.0fns, Tslide=%1.0fns, fcSlide=%1.0f kHz\n",RX.AC.Dynamic,RX.AC.Tstep*1e9,RX.AC.Tslide*1e9,RX.AC.fckHz_Slide);
                MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(RX.Squelch.Gain[0]),DB20(RX.Squelch.Gain[1]),RX.Squelch.Tstep*1e9,RX.Squelch.Enabled);
            break;
        case 2:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB, Rin=%1.1f Ohms\n",-RX.RFE.PdB[0],-RX.RFE.PdB[1],RX.RF.Rin);
            if (RX.LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB, NF={%1.1f,%1.1f} dB\n",DB20(RX.LNA2.Av[1]),DB20(RX.LNA2.Av[0]),RX.LNA2.NFdB[1],RX.LNA2.NFdB[0]);
            MsgFunc("    LNA+Mixer LNAGain=1: Av=%5.2f, IIV2=%5.2f, IIV3=%5.3f, Ni=%5.2fnV/rtHz\n",RX.RF.Gain[1].Av,RX.RF.Gain[1].IIV2,RX.RF.Gain[1].IIV3,RX.RF.Ni[1]);
            MsgFunc("              LNAGain=0: Av=%5.2f, IIV2=%5.2f, IIV3=%5.3f, Ni=%5.2fnV/rtHz\n",RX.RF.Gain[0].Av,RX.RF.Gain[0].IIV2,RX.RF.Gain[0].IIV3,RX.RF.Ni[0]);
            i=24;
            MsgFunc("    SGA (AGAIN =%3d)   : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",i,RX.SGA.Gain[i].Av,RX.SGA.Gain[i].IIV2,RX.SGA.Gain[i].IIV3,RX.SGA.Ni[i]);
            MsgFunc("    Low Pass Filter    : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",RX.Filter.Gain.Av,RX.Filter.Gain.IIV2,RX.Filter.Gain.IIV3,RX.Filter.Ni);
            MsgFunc("    PGA                : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",RX.PGA.Gain.Av,RX.PGA.Gain.IIV2,RX.PGA.Gain.IIV3,RX.PGA.Ni);
            MsgFunc("    ADCBuf (Radio Out) : Av=%5.2f, IIV2=%5.1f, IIV3=%5.2f, Ni=%5.1fnV/rtHz\n",RX.ADCBuf.Gain.Av,RX.ADCBuf.Gain.IIV2,RX.ADCBuf.Gain.IIV3,RX.ADCBuf.Ni);
            if (RX.Mixer.PhaseDeg || RX.Mixer.Gain_dB)
                MsgFunc("    Mixer I/Q Mismatch : %5.1f dBc (Phase=%1.1f deg, Gain=%1.2f dB)\n",-IMRdB,RX.Mixer.PhaseDeg,RX.Mixer.Gain_dB);
            if (RX.PLL.Enabled) 
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",PnRX,RX.PLL.PSD,RX.PLL.fc/1e3,RX.PLL.FilterN);
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",RX.Filter.N,RX.Filter.fc,100.0*RX.Filter.fcMismatch);
            MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f, %1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",RX.AC.s[0].fc_kHz,RX.AC.s[1].fc_kHz,RX.AC.s[2].fc_kHz,RX.AC.s[3].fc_kHz,100.0*RX.AC.fcMismatch);
            MsgFunc("                       : Dynamic = %d, Tstep=%1.0fns, Tslide=%1.0fns, fcSlide=%1.0f kHz\n",RX.AC.Dynamic,RX.AC.Tstep*1e9,RX.AC.Tslide*1e9,RX.AC.fckHz_Slide);
          MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(RX.Squelch.Gain[0]),DB20(RX.Squelch.Gain[1]),RX.Squelch.Tstep*1e9,RX.Squelch.Enabled);
            MsgFunc("    DC/Offsets (mV)    : PcRF=%3.0fdBm, Mixer=%2.0f, SGA=%2.0f, LPF=%2.0f, PGA=%2.0f, Buf=%2.0f\n",RX.RF.Pc,1000*RX.Mixer.Vo, 1000*RX.SGA.Vo,1000*RX.Filter.Vo,1000*RX.PGA.Vo, 1000*RX.ADCBuf.Vo);
            MsgFunc("    Large Signal Detect: Th =%1.0fmV\n",1000*RX.ThLgSigAFE);
            break;

        case 4:
            MsgFunc("    Radio Front End    : Loss[0]=%1.1f dB, Loss[1]=%1.1f dB\n",-RX.RFE.PdB[0],-RX.RFE.PdB[1]);
            if (RX.LNA2.Enabled)
                MsgFunc("    External LNA       : Av={%4.1f,%4.1f} dB\n",DB20(RX.LNA2.Av[1]),DB20(RX.LNA2.Av[0]));
            MsgFunc("    LNA + Mixer        : Av={%4.1f,%4.1f} dB, Rin=%1.0f Ohms\n",DB20(RX.RF.Gain[1].Av),DB20(RX.RF.Gain[0].Av),RX.RF.Rin);
            MsgFunc("    SGA/PGA/ADCBuf     : SGA:A0=%4.1fdB, PGA=%4.1fdB. ADCBuf=%4.1fdB\n",DB20(RX.SGA.A0),DB20(RX.PGA.Gain.Av),DB20(RX.ADCBuf.Gain.Av));
            MsgFunc("    Low Pass Filter    : N =%2d pole, Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",RX.Filter.N,RX.Filter.fc,100.0*RX.Filter.fcMismatch);
            if (RX.AC.Enabled)
                MsgFunc("    AC Coupling Pole   : fc = %1.0f kHz (%1.1f%% Mismatch)\n",RX.AC.s[0].fc_kHz,100.0*RX.AC.fcMismatch);
                MsgFunc("    RF Squelch         : Gain = {%5.1f dB, %5.1f dB}, Tstep=%1.0f ns, Enabled=%d\n",DB20(RX.Squelch.Gain[0]),DB20(RX.Squelch.Gain[1]),RX.Squelch.Tstep*1e9,RX.Squelch.Enabled);
            break;

        default: XSTATUS(WI_ERROR_UNDEFINED_CASE);
    }

    // -----------
    // Transmitter
    // -----------
    switch (TX.ModelLevel)
    {
        case 0:
        case 1:
            if (TX.ModelLevel>0) {    
                                            MsgFunc(" TX LPF                : N = %d pole Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",TX.Filter.N,TX.Filter.fc,100.0*TX.Filter.fcMismatch);
                if (TX.AC.Enabled)    MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",TX.AC.s[0].fc_kHz,TX.AC.s[1].fc_kHz,100.0*TX.AC.fcMismatch);
            }
            if (TX.IQ.Enabled)         MsgFunc("    Mixer I/Q Mismatch : A1=%4.3f, A2=%4.3f, a1=%1.1f deg, a2=%1.1f deg\n",TX.IQ.A1,TX.IQ.A2,TX.IQ.a1_deg,TX.IQ.a2_deg);
            else                    MsgFunc("    Mixer I/Q Mismatch : none\n");
            if (TX.PLL.Enabled)
            {
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d); CFO = %3.1f kHz\n",PnTX,TX.PLL.PSD,TX.PLL.fc/1e3,TX.PLL.FilterN,TX.PLL.CFO_ppm*TX.PLL.f_MHz/1e3);
                if (TX.PLL.Transient.PeakCFO_ppm)
                    MsgFunc("    Transient CFO      : k0=%d, Peak=%1.1fppm, fc=%1.0fkHz, tau=%1.1fus\n",TX.PLL.Transient.k0,TX.PLL.Transient.PeakCFO_ppm,TX.PLL.Transient.fc/1E3,1E6*TX.PLL.Transient.tau);
            }
            if (TX.PowerAmp.Enabled) MsgFunc("    PowerAmp           : Rapp Model p=%g, OBO=%g dB, Ap=%g dB\n",TX.PowerAmp.p,TX.PowerAmp.OBO,10*log10(TX.PowerAmp.Ap));
            else                     MsgFunc("    PowerAmp           : Linear\n");
            break;

        case 2:
            MsgFunc(" TX LPF                : N = %d pole Butterworth, fc=%2.1fMHz (%1.1f%% Mismatch)\n",TX.Filter.N,TX.Filter.fc,100.0*TX.Filter.fcMismatch);
//         MsgFunc("    LPF Noise/Nonlinear: Av=%5.1f dB, Ni=%5.1f nV/rtHz, alpha=%6.4f\n",DB20(TX.Filter.Av),TX.Filter.Ni,TX.Filter.alpha);
            MsgFunc("    AC Coupling Poles  : fc={%1.0f, %1.0f} kHz (%1.1f%% Mismatch)\n",TX.AC.s[0].fc_kHz,TX.AC.s[1].fc_kHz,100.0*TX.AC.fcMismatch);
            MsgFunc("    Mixer I/Q Mismatch : A1=%4.2f dB, A2=%4.2f dB, a1=%1.1f deg, a2=%1.1f deg\n",DB20(TX.IQ.A1),DB20(TX.IQ.A2),TX.IQ.a1_deg,TX.IQ.a2_deg);
            if (TX.PLL.Enabled)
            {
                MsgFunc("    LO/Mixer PhaseNoise: %5.1f dBc (%5.1f dBc/Hz, fc=%1.1f kHz, N=%d)\n",PnTX,TX.PLL.PSD,TX.PLL.fc/1e3,TX.PLL.FilterN);
                MsgFunc("    Carrier Freq Offset: %5.1f ppm @ %5.3f GHz = %4.1f kHz\n",TX.PLL.CFO_ppm,TX.PLL.f_MHz/1e3,TX.PLL.CFO_ppm*TX.PLL.f_MHz/1e3);
                if (TX.PLL.Transient.PeakCFO_ppm)
                    MsgFunc("    Transient CFO      : k0=%d, Peak=%1.1fppm, fc=%1.0fkHz, tau=%1.1fus\n",TX.PLL.Transient.k0,TX.PLL.Transient.PeakCFO_ppm,TX.PLL.Transient.fc/1E3,1E6*TX.PLL.Transient.tau);
            }
            MsgFunc("    PowerAmp           : Rapp Model p=%1.1f, OBO=%1.1fdB\n",TX.PowerAmp.p,TX.PowerAmp.OBO);
            if (TX.Startup.tau)
                MsgFunc("    Startup Transient  : t0=%1.2fus, tau=%1.2fus\n",TX.Startup.t0*1.0E+6,TX.Startup.tau*1.0E+6);
            break;
    }
    return WI_SUCCESS;
}
// end of RF52_WriteConfig()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2006-09-22 Brickner: Removed 4th AC coupling pole from RX model. Retain in data structure so old scripts still execute
// 2006-10-16 Brickner: Fixed definition for RX.AC.t1
// 2006-10-30 Brickner: Removed filter from RX.ModelLevel = 0 (dummy radio) for RTL SIFS testing
// 2006-12-08 Brickner: Restored 4th AC coupling pole in RX ModelLevel=2 only
// 2008-06-18 Brickner: RF52_RX_Mixer() Allow I/Q mismatch with any ModelLevel

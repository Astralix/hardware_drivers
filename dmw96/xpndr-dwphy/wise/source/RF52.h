//
//  RF52.h -- RF52 (analog) 2.4/5 GHz Radio Model
//  Copyright 2005 DSP Group, Inc. All rights reserved.
//

#ifndef __RF52_H
#define __RF52_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  POLYNOMIAL GAIN STAGE 
//=================================================================================================
typedef struct
{
    wiReal Av;     // linear gain
    wiReal IIV2;   // second order intercept
    wiReal IIV3;   // third order intercept
    wiReal Vmin;   // amplitude limit following linear gain
    wiReal Vmax;   //      "      "       "        "    "
    wiReal c2, c3; // nonlinearity: y = x + c2*x^2 + c3*x^3
}  RF52_GainState_t;

//=================================================================================================
//  FILTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiReal     Ni;                 // input noise density (nV/rtHz)
    wiReal     Nrms;               // input noise (Vrms)
    wiReal     Nrms_3dB;           // input noise (Vrms) / sqrt(2)
    wiInt      N;                  // filter order
    wiReal     fc;                 // passband corner frequency (MHz)    
    wiReal     fcMismatch;         // mismatch error between quadrature terms
    wiReal     b[2][10], a[2][10]; // filter coefficients [I/Q][tap #]
    wiComplex  w[2][10];           // filter memory
    wiReal     Vo;                 // input offset voltage (same value applied to I, Q)
    RF52_GainState_t Gain;         // polynomial gain state (RX only)
}  RF52_FilterState_t;

//=================================================================================================
//  AC COUPLING POLE STRUCTURE
//=================================================================================================
typedef struct
{
    wiReal    fc_kHz;             // pole location (kHz)
    wiReal    b[2], a[2];         // filter coefficients (2 for I/Q)
    wiComplex w[2][2];            // filter memory
    wiComplex yD[2][2];
    wiComplex xD[2][2];
}  RF52_PoleState_t;

//=================================================================================================
//  PLL PHASE/FREQUENCY ERROR STRUCTURE
//=================================================================================================
typedef struct {
    wiBoolean Enabled;            // enable the PLL/Phase noise model
    wiReal    CFO_ppm;            // CFO ppm
    wiReal    f_MHz;              // channel center frequency (MHz)
    wiReal    PSD;                // phase noise PSD dBc
    wiInt     FilterN;            // phase noise filter order
    wiReal    fc;                 // cutoff frequency (Hz)
    wiReal   *a;                  // total error angle
    struct
    {
        wiReal PeakCFO_ppm;        // peak CFO deviation
        wiReal fc;                 // loop filter cutoff
        wiReal tau;                // loop filter time constant
        wiInt  k0;                 // loop filter start time
    } Transient;                  // Transient response of PLL

} RF52_PllState_t;

//=================================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiInt  ModelLevel;            // {0=Gain Only, 1=Include Filters, 2=Include All}
    wiReal Fs_MHz;                // Model sample rate (MHz)
    wiReal Prms;                  // RMS input signal

    RF52_FilterState_t Filter;    // Bandpass filter
    RF52_PllState_t PLL;

    struct {
        wiBoolean Enabled;        // Enable the AC coupling model for ModelLevel=1
        wiReal    fcMismatch;     // mismatch error between quadrature terms
        RF52_PoleState_t s[2];    // AC coupling poles
    } AC;
    struct
    {
        wiBoolean Enabled;         // Enabled the baseband I/Q mismatch model
        wiReal    A1, A2;          // Quadrature gain term (relative to one)
        wiReal    a1_deg, a2_deg;  // Quadrature angle error (degrees)
    } IQ;
    struct
    {
        wiReal    Ni;
        wiReal    Nrms;
    } Mixer;
    struct
    {
        wiBoolean Enabled;
        wiReal    p;
        wiReal    OBO;
        wiReal    Ap;
        wiReal    DelayShutdown;
    } PowerAmp;
    struct
    {
        wiReal    t0;
        wiReal    tau;
        wiInt     k0;
        wiReal    ktau;
    } Startup;
}  RF52_TxState_t;

//=================================================================================================
//  RECEIVER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiInt  ModelLevel;               // {0=Gain Only, 1=Include Filters, 2=Include All}
    wiReal T_degK;                   // noise temperature
    wiReal NFdB;                     // noise figure for modes 0 and 1
    wiReal Nrms;
    wiReal Fs_MHz;                   // model sample rate (MHz)
    wiInt  M_20MHz;                  // Fs_MHz / 20 MHz
    wiComplex *trace;                // trace variable
    wiInt Ntrace;                    // length of trace array
    struct {                         // Radio Front End (antenna, package, balun, etc. ---
        wiReal     PdB[2];            // power change at front end (dB, <0)
        wiReal     Av[2];             // power converted to amplitude gain
    } RFE;
    struct {
        wiBoolean  Enabled;           // enable LNA2 model
        wiReal     Av[2];             // amplitude gain
        wiReal     NFdB[2];           // noise figure
        wiReal     Nrms[2];           // input noise (Vrms)
    } LNA2;
    struct {                         // LNA + Mixer --------------------------------------
        wiReal     Rin;               // input resistance
        wiReal     sqrtRin;           // sqrt(Rin)
        wiReal     Av[2];             // gain scale for LNAGain
        wiReal     Tdelay;            // delay for LNA gain change
        wiInt      nT;                // number of delay periods at sample rate for Tdelay
        wiReal     Ni[2];             // input noise density (nV/rtHz)
        wiReal     Nrms[2];           // input noise (Vrms)
        wiReal     Vmax[2];           // maximum amplitude
        wiReal     Pc;                // coupled LO power at radio/LNA input [dBm]
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        RF52_GainState_t Gain[2];     // gain as a function of LNAGAIN
    } RF;
    struct {
        wiBoolean Enabled;            // enable squelch during AC coupling offset acquisition
        wiReal    Gain[2];            // squelch gain as a function of LNAGain
        wiReal    Tstep;              // squelch period
    } Squelch;
    struct {
        wiReal     PhaseDeg;          // quadrature angle error (degrees)
        wiReal     Gain_dB;           // quadrature amplitude error (dB)
        wiReal     cI;                // coefficient for "I" term
        wiReal     cQ;                // coefficient for "Q" term
        wiReal     Vo;                // output offset voltage (same value applied to I, Q)
    } Mixer;
    RF52_PllState_t PLL;
    struct {                         // Switched Gain Amplifier
        wiReal     A0;                // voltage gain with AGAIN=0
        wiReal     Av[64];            // voltage gain [AGAIN]
        wiReal     Ni[64];            // input noise density (nV/rtHz) [AGAIN]
        wiReal     Nrms[64];          // input noise (Vrms) [AGAIN]
        wiReal     alpha;             // output nonlinearity: third order compression
        wiReal     Tdelay;            // delay for SGA gain change
        wiInt      nT;                // number of delay periods at sample rate for Tdelay
        wiReal     Vmax;              // maximum amplitude
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        RF52_GainState_t Gain[64];    // nonlinear gain state
    } SGA;
    RF52_FilterState_t Filter;   // Bandpass filter
    struct {
        wiBoolean Enabled;            // Enable the AC coupling model for ModelLevel=1
        wiReal    fcMismatch;         // mismatch error between quadrature terms
        wiBoolean Dynamic;            // enable dynamic poles
        wiReal    Tstep;              // period for fast pole
        wiReal    Tslide;             // time for pole slide
        wiReal    fckHz_Slide;        // corner frequency at start of slide
        wiReal    t1;                 // "t" count at start of slide
        RF52_PoleState_t s[4];    // AC coupling poles
    } AC;
    struct {                         // Programmable Gain Amplifier ----------------------
        wiReal     Ni;                // input noise density (nV/rtHz)
        wiReal     Nrms;              // input noise (Vrms)
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        RF52_GainState_t Gain;          // nonlinear gain state
    } PGA;
    struct {
        wiReal     Ni;                // input noise density (nV/rtHz)
        wiReal     Nrms;              // input noise (Vrms)
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        RF52_GainState_t Gain;          // nonlinear gain state
    } ADCBuf;
    wiReal ThLgSigAFE;               // Threshold for LgSigAFE (mag)
}  RF52_RxState_t;

typedef union // ----------------- Baseband/Radio Digital I/O (RX ONLY) ----------------         
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int RESETB       : 1; // < 0>   reset (active low)
        unsigned int __unused1    : 1; // < 1>   unused
        unsigned int MC           : 2; // < 2:3> mode control
        unsigned int AGAIN        : 6; // < 4:9> AGAIN[5:0]
        unsigned int LNAGAIN      : 1; // <10>   LNA Gain
        unsigned int LNAGAIN2     : 1; // <11>   LNA2 Gain
        unsigned int LGSIGAFE     : 1; // <12>   Large signal detect (radio out)
        unsigned int __unused13   : 1; // <13>   unused (RF52 GPIO)
        unsigned int __unused14   : 1; // <14>   unused (PHY_SEN)
        unsigned int __unused15   : 1; // <15>   unused (PHY_SDCLK)
        unsigned int __unused16   : 1; // <16>   unused (PHY_SDATA)
        unsigned int SW1          : 1; // <17>   RF Switch #1 
        unsigned int SW1B         : 1; // <18>   RF Switch #1 (complement)
        unsigned int SW2          : 1; // <19>   RF Switch #2
        unsigned int SW2B         : 1; // <20>   RF Switch #2 (complement)
        unsigned int PA24         : 1; // <21>   Enable for 2.4 GHz PA
        unsigned int PA50         : 1; // <22>   Enable for 5 GHz PA
        unsigned int __unused23   : 1; // <23>   unused
        unsigned int __unused24   : 1; // <24>   unused
        unsigned int LNA24A       : 1; // <25>   Enable for external LNA (Path A, 2.4 GHz)
        unsigned int LNA24B       : 1; // <26>   Enable for external LNA (Path B, 2.4 GHz)
        unsigned int LNA50A       : 1; // <27>   Enable for external LNA (Path A, 5 GHz)
        unsigned int LNA50B       : 1; // <28>   Enable for external LNA (Path B, 5 GHz)
        unsigned int __unused29   : 1; // <29>   unused
        unsigned int __unused30   : 1; // <30>   unused
        unsigned int __unused31   : 1; // <31>   unused
    };
} RF52_SignalState_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus RF52_TX(int Ns, wiComplex *sIn, wiComplex *sOut, int nPadBack);
wiStatus RF52_RX(int n, int k, wiComplex rIn, wiComplex *rOut, RF52_SignalState_t SigIn, RF52_SignalState_t *SigOut);

wiStatus RF52_TX_Restart(int N, wiReal Fs_MHz, wiReal Prms);
wiStatus RF52_RX_Restart(int N, wiReal Fs_MHz, int N_Rx);

wiStatus RF52_Init(void);
wiStatus RF52_Close(void);

wiStatus RF52_WriteConfig(wiMessageFn MsgFunc);
wiStatus RF52_StatePointer(RF52_TxState_t **pTX, RF52_RxState_t **pRX);

RF52_TxState_t * RF52_TxState(void);
RF52_RxState_t * RF52_RxState(void);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


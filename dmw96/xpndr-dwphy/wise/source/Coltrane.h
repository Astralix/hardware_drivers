//
//  Coltrane.h -- Coltrane (analog) 2.4/5 GHz Radio Model
//  Copyright 2005, 2009 DSP Group, Inc. All rights reserved.
//

#ifndef __COLTRANE_H
#define __COLTRANE_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Pseudonym for RF55
//
#define COLTRANE_IS_RF55

#ifdef COLTRANE_IS_RF55

    #define RF55_InitializeThread Coltrane_InitializeThread
    #define RF55_CloseThread      Coltrane_CloseThread
    #define RF55_TX_Restart       Coltrane_TX_Restart
    #define RF55_RX_Restart       Coltrane_RX_Restart
    #define RF55_TX               Coltrane_TX
    #define RF55_RX               Coltrane_RX
    #define RF55_WriteConfig      Coltrane_WriteConfig
    #define RF55_Init             Coltrane_Init
    #define RF55_Close            Coltrane_Close
    #define RF55_RxState          Coltrane_RxState
    #define RF55_TxState          Coltrane_TxState
    #define RF55_TX_Mixer         Coltrane_TX_Mixer
    #define RF55_SignalState_t    Coltrane_SignalState_t
    #define RF55_RxState_t        Coltrane_RxState_t
    #define RF55_TxState_t        Coltrane_TxState_t
    #define RF55_GainState_t      Coltrane_GainState_t
    #define RF55_FilterState_t    Coltrane_FilterState_t
    #define RF55_PoleState_t      Coltrane_PoleState_t
    #define RF55_PllState_t       Coltrane_PllState_t
    #define RF55_TxIQLoopback_t   Coltrane_TxIQLoopback_t

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

}  Coltrane_GainState_t;

//=================================================================================================
//  FILTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiReal     Ni;                 // input noise density (nV/rtHz)
    wiReal     Nrms;               // input noise (Vrms)
    wiReal     Nrms_3dB;           // input noise (Vrms) / sqrt(2)
    wiInt      N;                  // filter order
    wiReal     fcMHz;              // passband corner frequency (MHz)    
    wiReal     fcMismatch;         // mismatch error between quadrature terms
    wiReal     b[2][10], a[2][10]; // filter coefficients [I/Q][tap #]
    wiComplex  w[2][10];           // filter memory
    wiReal     Vo;                 // input offset voltage (same value applied to I, Q)
    Coltrane_GainState_t Gain;         // polynomial gain state (RX only)

}  Coltrane_FilterState_t;

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

}  Coltrane_PoleState_t;

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
	wiInt     Npts;              // size of a and u
    wiRealArray_t u;              // white Gaussian noise before shaping into "a"
    wiRealArray_t a;              // total error angle

    struct
    {
        wiReal PeakCFO_ppm;        // peak CFO deviation
        wiReal fc;                 // loop filter cutoff
        wiReal tau;                // loop filter time constant
        wiInt  k0;                 // loop filter start time
    } Transient;                  // Transient response of PLL

} Coltrane_PllState_t;

//=================================================================================================
//  TX I/Q Loopback (for calibration) model
//=================================================================================================
typedef struct
{
    wiInt     NumRx;
    double    lpfBuffer1[2], lpfBuffer2[2];
    double    hpfBuffer1[1], hpfBuffer2[1];
    double    scaling;
    wiComplex edOut;

}  Coltrane_TxIQLoopback_t;

//=================================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiInt  ModelLevel;            // {0=Gain Only, 1=Include Filters, 2=Include All}
    wiReal Fs_MHz;                // Model sample rate (MHz)
    wiReal Prms;                  // RMS input signal

    Coltrane_FilterState_t Filter;    // Bandpass filter
    Coltrane_PllState_t PLL;

    struct {
        wiBoolean Enabled;        // Enable the AC coupling model for ModelLevel=1
        wiReal    fcMismatch;     // mismatch error between quadrature terms
        Coltrane_PoleState_t s[2];    // AC coupling poles
    } AC;
    struct
    {
        wiReal    Ni;
        wiReal    Nrms;
        wiReal    PhaseDeg;        // quadrature angle error (degrees)
        wiReal    Gain_dB;         // quadrature amplitude error (dB)
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

}  Coltrane_TxState_t;

//=================================================================================================
//  RECEIVER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiInt  ModelLevel;               // {0=Gain Only, 1=Include Filters, 2=Include All}
    wiReal T_degK;                   // noise temperature
    wiReal NFdB;                     // noise figure for modes 0 and 1
    wiReal Nrms;
    wiReal Fs_MHz;                    // model sample rate (MHz)
    wiInt  M_20MHz;                  // Fs_MHz / 20 MHz

    wiComplex *trace;                // trace variable
    wiInt Ntrace;                    // length of trace array

    wiBoolean GainChange;
    wiReg regLgSigAFE;
    double xACpole, xRFattn;          // AC coupling pole location
    wiUInt dAGain, dLNAGain, dMC;

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
        Coltrane_GainState_t Gain[2];     // gain as a function of LNAGAIN
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
    Coltrane_PllState_t PLL;
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
        Coltrane_GainState_t Gain[64];    // nonlinear gain state
    } SGA;
    Coltrane_FilterState_t Filter;   // Bandpass filter
    struct {
        wiBoolean Enabled;            // Enable the AC coupling model for ModelLevel=1
        wiReal    fcMismatch;         // mismatch error between quadrature terms
        wiBoolean Dynamic;            // enable dynamic poles
        wiReal    Tstep;              // period for fast pole
        wiReal    Tslide;             // time for pole slide
        wiReal    fckHz_Slide;        // corner frequency at start of slide
        wiReal    t1;                 // "t" count at start of slide
        Coltrane_PoleState_t s[4];    // AC coupling poles
    } AC;
    struct {                         // Programmable Gain Amplifier ----------------------
        wiReal     Ni;                // input noise density (nV/rtHz)
        wiReal     Nrms;              // input noise (Vrms)
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        Coltrane_GainState_t Gain;          // nonlinear gain state
    } PGA;
    struct {
        wiReal     Ni;                // input noise density (nV/rtHz)
        wiReal     Nrms;              // input noise (Vrms)
        wiReal     Vo;                // input offset voltage (same value applied to I, Q)
        wiReal     Vo5AI, Vo5AQ;      // output offset for RX.ModelLevel = 5, RXA I/Q
        wiReal     Vo5BI, Vo5BQ;      // output offset for RX.ModelLevel = 5, RXA I/Q
        Coltrane_GainState_t Gain;          // nonlinear gain state
    } ADCBuf;
    wiReal ThLgSigAFE;               // Threshold for LgSigAFE (mag)

    Coltrane_TxIQLoopback_t TxIQLoopback; 

}  Coltrane_RxState_t;

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
        unsigned int __unused13   : 1; // <13>   unused (Coltrane GPIO)
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
} Coltrane_SignalState_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus Coltrane_TX(int Ns, wiComplex *sIn, wiComplex *sOut, int nPadBack);
wiStatus Coltrane_RX(int n, int k, wiComplex rIn, wiComplex *rOut, Coltrane_SignalState_t SigIn, Coltrane_SignalState_t *SigOut);

wiStatus Coltrane_TX_Restart(int N, wiReal Fs_MHz, wiReal Prms);
wiStatus Coltrane_RX_Restart(int N, wiReal Fs_MHz, int N_Rx);

wiStatus Coltrane_Init(void);
wiStatus Coltrane_Close(void);

wiStatus Coltrane_InitializeThread(unsigned int ThreadIndex);
wiStatus Coltrane_CloseThread     (unsigned int ThreadIndex);

wiStatus Coltrane_WriteConfig(wiMessageFn MsgFunc);
wiStatus Coltrane_StatePointer(Coltrane_TxState_t **pTX, Coltrane_RxState_t **pRX);

Coltrane_TxState_t * Coltrane_TxState(void);
Coltrane_RxState_t * Coltrane_RxState(void);

wiStatus Coltrane_TX_Mixer(int Nx, wiComplex *x);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


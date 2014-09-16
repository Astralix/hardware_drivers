//
//  wiChanl.h -- WISE Channel Model
//  Copyright 2001-2002 Bermai, 2006-2011 DSP Group, Inc. All rights reserved.
//

#ifndef __WICHANL_H
#define __WICHANL_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

// Define Maximum Number of Transmit and Receive Antennas
// and maximum number of packets in interference packet history
//
#define WICHANL_TXMAX       8
#define WICHANL_RXMAX       8
#define WICHANL_MAXPACKETS 31

// Maximum FIR filter length for channel model 1
//        
#define WICHANL_MAX_CHANNEL_TAPS 201

// Define maximum number of interpolator steps and filter length
//
#define WICHANL_INTERPOLATOR_STEPS 1001
#define WICHANL_INTERPOLATOR_TAPS    31

// Define maximum number of pulses in a burst for the radar model
//
#define WICHANL_RADAR_MAX_N  999
#define WICHANL_RADAR_MAX_DK  25

// Enumerate Interference Models
//
enum wiChanlInterferenceModelE
{
    WICHANL_INTERFERENCE_NONE                  = 0,
    WICHANL_INTERFERENCE_TONE                  = 1,
    WICHANL_INTERFERENCE_LAST_PACKET           = 2,
    WICHANL_INTERFERENCE_80211A_MASK           = 3,
    WICHANL_INTERFERENCE_NONLINEAR_LAST_PACKET = 4,
    WICHANL_INTERFERENCE_MULTIPLE_PACKETS      = 5
};


typedef struct
{
    wiReal     wInterpolate[WICHANL_INTERPOLATOR_STEPS][WICHANL_INTERPOLATOR_TAPS]; 
    wiComplex  IDFT_W_256[256];
} wiChanl_LookupTable_t;


//=================================================================================================
//  CHANNEL PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiInt      Model;              // Internal model/module number
    wiInt      Channel;            // {0=AWGN, 1=Exp. Multipath, 2=1+Doppler, 3=1+/-D^n}

    wiBoolean  FixedNoiseFloor;    // Keep noise fixed and vary the signal power
    wiBoolean  NormalizedFading;   // Normalize the fading to give constant SNR
    wiBoolean  AntennaBalance;     // Keep antenna receive powers related as sin(a),cos(a)

    wiReal     Fs;                 // Sample frequency (Hz)
    wiReal     A;                  // Signal gain for fixed noise floor of 1
    wiReal     Av[WICHANL_RXMAX];  // Gain per receive antenna (A modified by dBGain, AntennaBalance)
    wiReal     Trms;               // RMS delay spread
    wiReal     Nrms;               // RMS value of the AWGN
    wiReal     fc;                 // Carrier frequency
    wiReal     fDoppler;           // Doppler frequency
    wiReal     fc_kHz;             // Carrier frequency offset/error (kHz)
    wiReal     fs_ppm;             // Sampling frequency error for model 3
    wiBoolean  RandomPhase;        // Enable random phase between antennas

    wiInt      Nc;                 // Number of terms in c
    wiInt      Nr;                 // Number of samples in array "r"
    wiInt      NumRx;              // Number of receive antennas (fka N_Rx)
    wiBoolean  TimingErrorModel;   // Enable the timing/phase error model
    wiReal     IQAngleErrorDeg;    // Error in angle between I and Q (+=<90deg)
    wiReal     IQGainRatio;        // Gain on I component (Q gain is 1), (0-1)
    wiReal     RxCoupling_dB;        // Rx signal cross-coupling (dB)
    wiReal     dBGain[WICHANL_RXMAX];// Fixed gain per RX antenna path
    wiBoolean  SkipGenerateChannel; // Do not generate channel responses (manually loaded)
    wiComplex *c[WICHANL_RXMAX][WICHANL_TXMAX];// Channel FIR coefficients
    wiComplexArray_t R[WICHANL_RXMAX+1];
    wiComplex *r[WICHANL_RXMAX+1];  // pointers to array elements in R

    wiInt      Channel3Dn[WICHANL_RXMAX]; // value for channel = 3

    struct
    {
        wiComplexArray_t n; // additive noise
    } Model_1;

    struct
    {
        wiBoolean  Enabled;         // Enable the model
        wiInt      Model;           // Model type {1,2}
        int        m;               // Polynomial order 
        wiReal     g1, g3, g5;      // Polynomial coefficients
        wiReal     Amax;            // Maximum amplitude
        wiReal     Anorm;           // Gain required to normalize the model
        wiReal     p;               // Rapp model "p" parameter
        wiReal     OBO;             // Output backoff relative to 1dB compression
        wiReal     ApRapp;          // gain for RappPA (dB)
        wiComplexArray_t y[WICHANL_TXMAX];// Output array
    }  PowerAmp;

    struct
    {
        wiReal PSD;               // phase noise at the carrier (dBc/Hz)
        wiReal fc;                // -3 dB for -20dB/decade rolloff
        wiRealArray_t u;          // phase error noise (before filter)
        wiRealArray_t a;          // phase error angle
    }  PhaseNoise;

    struct
    {
        wiReal Jrms;              // rms jitter (seconds, Gaussian)
        wiReal fc;                // -3 dB for -20dB/decade rolloff (Hz)
        wiReal Jpk;               // peak jitter (seconds, periodic)
        wiReal fHz;               // frequency for jitter tone (Hz)
        wiRealArray_t    dt;         // array of per-sample time devations
        wiComplexArray_t z;           // intermediate calculation
        wiRealArray_t    v;
    }   SampleJitter;             

    struct
    {
        wiBoolean Enabled;        // enable the position offset model
        wiUInt    Range;          // maximum negative offset (multiples of Ts)
        wiUInt    dt;             // constant offset (multiples of Ts)
        wiUInt    t0;             // time-shift (current packet)
    }   PositionOffset;             

    struct
    {
        wiBoolean Enabled;          // enable the gain step model
        wiUInt    k0;               // sample index for the gain change
        wiReal    dB[WICHANL_RXMAX];// relative gain per receiver (dB)
    }   GainStep;

    struct
    {
        wiBoolean Enabled;          // enable the amplitude modulation model
        wiBoolean Linear;           // tone adjusts linear amplitude
        wiUInt    k0;               // sample index to start AM
        wiReal    f[WICHANL_RXMAX]; // frequency (Hz) per receiver
        wiReal    dB[WICHANL_RXMAX];// zero-to-peak modulation (dB) per receiver
        wiReal    dT[WICHANL_RXMAX];// phase (0.0 - 1.0)/f for the tone
        struct
        {
            wiUInt k1, k2;          // end points
            wiReal A;               // relative linear gain at k2 relative to k1
        } Ramp;
    }   AM;

    struct
    {
        wiInt      Model; // type of interference model (0=disabled)
        wiInt      Channel;        // channel relative to the main channel
        wiReal     PrdBm;          // receive power level (dBm)
        wiReal     f_MHz;          // baseband center frequency (MHz)
        wiReal     t0;             // interference time offset, r(t)=s(t)+i(t+t0)
        wiReal     pRapp;          // "p" for RappPA model
        wiReal     OBOdB;          // output backoff for RappPA model
        wiReal     ApRapp;         // gain for RappPA (dB)(to compensate compressive attenuation)
        wiReal     Pn_dBm_MHz;     // relative additive noise floor for model=4
        wiInt      nk1, nk2;       // end points for additive noise
        wiComplexArray_t v;        // intermediate calculated value
    
        struct                     // packet store for multiple packet streams
        {
            wiReal PrdBm;                     // receive power level (dBm)
            wiReal f_MHz;                     // baseband center frequency (MHz)
            wiReal t0;                        // packet starting time relative to final TX
            wiComplexArray_t s[WICHANL_RXMAX];// stored packet waveform
        } Packet[WICHANL_MAXPACKETS];

    }  Interference;

    struct
    {
        wiComplexArray_t r;
        wiComplexArray_t y[WICHANL_TXMAX];
    } LoadPacket;

    struct
    {
        wiInt      Model;          // type of interference model (0=disabled, 1=pulsed tone, 2=chirp)
        wiInt      N;              // number of pulses
        wiReal     PRF;            // pulse repetition frequency (pps)
        wiReal     W;              // pulse width (microseconds)
        wiReal     PrdBm;          // receive power level (dBm)
        wiReal     f_MHz;          // radar center frequency (MHz)
        wiReal     fRangeMHz;      // frequency range for chirp radar
        wiInt      k0;             // offset to starting sample for radar model
        wiBoolean  RandomStart;    // override k0 and select random offset
        wiInt      k0RandomStart;  // k0 value when RandomStart is true
        wiInt      dk[WICHANL_RADAR_MAX_DK]; // position deviation per radar pulse
        wiReal     pMissing;       // probability of missing a pulse
        wiBooleanArray_t MissingPulse; // pulse is missing
    }  Radar;

    struct // internal memory for the RxCoupling model
    {
        wiComplexArray_t s[WICHANL_RXMAX];
    } RxCoupling;

}  wiChanl_State_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================
wiStatus wiChanl_Init  (void);
wiStatus wiChanl_Close (void);
wiStatus wiChanl_InitializeThread(void);
wiStatus wiChanl_CloseAllThreads(int FirstThread);

wiChanl_State_t       *wiChanl_State(void);
wiChanl_LookupTable_t *wiChanl_LookupTable(void);

wiStatus wiChanl_Model     (int Nx, wiComplex *X[], int Nr, wiComplex **R[], double Fs, double PrdBm, double Prms, double Nrms);
wiStatus wiChanl_LoadPacket(int Nx, wiComplex *X[], double Fs, double Prms, int iPacket, double PrdBm, double f_MHz, double t0);
wiStatus wiChanl_GenerateChannelResponse(int NumTx, int NumRx);
wiStatus wiChanl_ClearPackets(void);
wiStatus wiChanl_LoadChannelResponse(int iTx, int iRx, int Nc, wiComplex *c);

wiStatus wiChanl_WriteConfig(wiMessageFn MsgFunc);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

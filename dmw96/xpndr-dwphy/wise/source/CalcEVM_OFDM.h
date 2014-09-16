//
//  CalcEVM_OFDM.h -- OFDM EVM Baseband Processor
//  Copyright 2006 DSP Group, Inc. All rights reserved.
//

#ifndef __CALCEVM_OFDM_H
#define __CALCEVM_OFDM_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif


//=================================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    int    PadFront;                // Pad zeros in front of the data streams; 20MHz;
    int    PadBack;                 // Pad zeros at the end of the data streams, 20MHz;
    int    datarate;                // Data rate (mbps)

    int    RATE;                    // Rate field (4-bits)
    int    LENGTH;                  // Length field (12-bits, 1-4095)
    int    SERVICE;                 // Service field (16-bits, typically 0)
    wiByte PSDU[4096];              // Data bytes to be transmitted

    int    N_SYM;                   // Number of OFDM symbols for DATA
	int    N_HTSYM;                 // Number of OFDM symbols for HT-DATA
    int    N_DBPS;                  // Number of data bits per symbol
    int    N_CBPS;                  // Number of coded bits per OFDM symbol
    int    N_BPSC;                  // Number of coded bits per subcarrier

    int    M;                       // Oversample rate = T_ADC/T_s

    int       *a; int Na;           // Serialized PSDU
    int       *b; int Nb;           // Scrambled PSDU
    int       *c; int Nc;           // Coded data bits
    int       *p; int Np;           // Permuted coded bits
    wiComplex *d; int Nd;           // Data subcarriers
    wiComplex *x; int Nx;           // Baseband time signal samples
    wiComplex *s; int Ns;           // Oversampled signal with offset carrier

    wiComplex dSERVICE[48];

    struct {
        int    b[65];               // fixed point FIR polynomial
    } LPF;

    wiReal     AClip;               // Digital Clipping amplitude

    wiReal     PowerAmp_Vmax;       // Maximum power amplifier output amplitude
    wiReal     PowerAmp_p;          // "p" parameter for nonlinearity model

    int Scrambler_Shift_Register;   // State of the transmit scrambler
    int Pilot_Shift_Register;       // State of the transmit pilot sign 

    wiInt     SignalInterface;      // Baseband=0, Carrier@f0=1, Carrier@fc=2
    wiBoolean Scrambler_Disabled;   // Disable the scrambler
    wiBoolean Interleaver_Disabled; // Disable the interleaver/permuter
    wiBoolean aModemRX;             // Use with aModem receiver?
    wiBoolean LoadUpsampleFIR;      // Runtime load upsample/interpolation filter

    // new parameters for WiSE 2.x
    wiUWORD MAC_TX_D[4104];

    wiBoolean  dumpTX;            // Output transmit signals;
    wiComplex *trace_t;            // Trace pointer for receiver signals;                         
    wiInt      trace_Nt;         // Trace length;          

}  CalcEVM_OFDM_TxState_t;

//=============================================================================
//  RECEIVER PARAMETER STRUCTURE
//=============================================================================
typedef struct
{
    int N_SYM;                // Number of OFDM symbols for DATA
	int N_DBPS;               // Number of data bits per symbol
    int N_CBPS;               // Number of coded bits per OFDM symbol
    int N_BPSC;               // Number of coded bits per subcarrier
    int N_RX;                 // Number of receiver paths (1-2 antennas)
    int M;                    // Oversample rate = T_ADC / T_Sample

    wiUWORD  RATE;            // Rate field (7-bits)
    int      LENGTH;          // Length field (16-bits, 1-65535)
	int      L_LENGTH;        // Legacy Length
    int      SERVICE;         // Service field (16-bits, typically 0)
    wiUWORD  *PHY_RX_D;       // Data bytes to be transmitted
    int      N_PHY_RX_WR;     // Number of PHY_RX_D writes (including 8 Byte header)
	wiBoolean HT_MM;          // Parsing parameter to indicate it should be treated as MM packet at RX
	wiBoolean MM_DATA;        // The decoded header indicates a HT Mixed Mode packets
	wiBoolean Rotated6Mbps;   // Indicate that it is HT-SIG 

    int       *a;     int Na; // Serialized PSDU
    int       *b;     int Nb; // Scrambled PSDU
    wiReal    *Lc;    int Nc; // ~LLR for the coded bits
    wiReal    *Lp;    int Np; // ~LLR for the permuted coded bits
    wiComplex *de;  int N_de; // Equalized data subcarriers
    wiComplex *d;  int Nd;    // Data subcarriers after phase-correction;
    wiComplex *v;  int Nv;    // Baseband time signal samples
    wiComplex *r;  int Nr;    // Oversampled received signal

    int        bSIG[24];      // Decoded SIG field
    int        bHTSIG[48];    // Decoded HT-SIG field
	int        x_ofs;         // Offset in x array (estimated)

    wiComplex H0;             // Channel estimation: center subcarrier
    wiComplex H52[52];        // Channel Estimation on pilot and data sc;   
    wiReal    S_N52[52];      // Average SNR per subcarrier

	wiComplex H[64];          // Legacy channel estimation
	wiReal    SNR_Legacy[64]; // Legacy SNR;

	wiComplex H_HT[64];       // HT channnel estimation;
	wiReal    SNR_HT[64];     // HT SNR 

    wiBoolean aModemTX;       // Use with aModem transmitter?

    int      Fault;           // {0=None, 1=SIGNAL, 2=Array overrun, 3=Missed SigDet}   

    wiBoolean DownmixOn;      // Enable 10 MHz IF downmix
    
    wiUInt   DCXSelect;       // Select t he DCX output
    wiInt    DCXShift;        // Selects pole position of DCX HPF
    wiUInt   DCXClear;        // Clear the accu;
    wiUInt   DCXHoldAcc;      // Hold the accumulator value (no updates)
    wiUInt   DCXFastLoad;     // No fast-charge of accumulators on ClearDCX
        
    wiUInt   SigDetFilter;    // (SD)  Signal detector input filter
    wiUInt   SigDetTh1;       // (SD)  Signal detector threshold #1
    wiReal   SigDetTh2f;      // (SD)  Floating point signal detector threshold 2; 
    wiUInt   SigDetDelay;     // (SD)  Signal detector startup delay
    wiInt    SGSigDet;        // (SD)  Signal detected point (Output);    
    wiUInt   SigDetTh2H;      // (SD)  Signal detector threshold #2 (bits [15:8]) //not used
    wiUInt   SigDetTh2L;      // (SD)  Signal detector threshold #2 (bits [ 7:0]  //not used

    wiUInt   SyncEnabled;     // (FS)  Frame Sync Enabled;
    wiInt    SyncFilter;      // (FS)  Frame Sync filter;   
    wiInt    SyncShift;       // (FS)  Negative delay for sync position
    wiUInt   SyncPKDelay;     // (FS)  Delay for peak detection (qualify)
    wiUInt   SyncSTDelay;     // (FS)  Delay from SGSigDet to frame sync start;
    wiInt    FSOffset;        // (FS)  FrameSync Output, Beginning of LTF;    
    wiReal   PKThreshold;     // (FS)  Threshold to qualify a peak.         

    wiUInt   CFOEnabled;      // (DFE) Enable CFO 
    wiUInt   CFOFilter;       // (DFE) CFO measurement filter
    wiUInt   DelayCFO;        // (DFE) Delay to load at least 1 short preamble symbol
    wiReal   cCFO;            // (DFE) Coarse CFO;
    wiReal   fCFO;            // (DFE) Fine CFO; 

    wiReal   eCP;             // (PED) Carrier phase error;
    wiReal   eSP;             // (PED) Sampling phase error;
    wiReal   *cCP;            // (DPLL) correcting carrier phase error;   2560 is corresponding to 10ms packet duration;
    wiReal   *cSP;            // (DPLL) correcting sampling phase error;

    wiUInt   ChanlEstEnabled; //  Channel Estimation Enable signal;
    wiUInt   cSPEnabled;      // (DPLL) Sampling phase correction enabled;
    wiUInt   cCPEnabled;      // (DPLL) Carrier phase correction enabled;
    int      *AdvDly;         // (DPLL) FFT window position offset controlled by DFLL   
    wiInt    Ca;              // (DPLL) proportional gain for carrier phase correcting
    wiInt    Cb;              // (DPLL) integral gain for carrier phase correcting
    wiInt    Sa;              // (DPLL) proportional gain for sampling phase correcting
    wiInt    Sb;              // (DPLL) integral gain for sampling phase correcting

    wiUInt  aPPDUMaxTimeH;   // Limit for packet duration is 256*aPPDUMaxTimeH + aPPDUMaxTimeL   
    wiUInt  aPPDUMaxTimeL;   
   
	wiBoolean   dumpRX;       // Output receiver signals;
    wiComplex *trace_r;       // Trace pointer for receiver signals;                         
    wiInt      trace_Nr;      // Trace length;          

}  CalcEVM_OFDM_RxState_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus CalcEVM_OFDM_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms);
wiStatus CalcEVM_OFDM_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[]);
wiStatus CalcEVM_OFDM_TXWriteConfig(wiMessageFn MsgFunc);
wiStatus CalcEVM_OFDM_RXWriteConfig(wiMessageFn MsgFunc);
wiStatus CalcEVM_OFDM_Init();
wiStatus CalcEVM_OFDM_Close();

CalcEVM_OFDM_TxState_t *CalcEVM_OFDM_TxState(void);
CalcEVM_OFDM_RxState_t *CalcEVM_OFDM_RxState(void);

wiStatus CalcEVM_OFDM_ViterbiDecoder(int NumBits, wiReal pInputLLR[], int OutputData[], int CodeRate12, int TruncationDepth);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


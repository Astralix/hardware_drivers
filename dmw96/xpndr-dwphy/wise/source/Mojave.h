//
//  Mojave.h -- Mojave Bit-Accurate Model
//  Copyright 2002-2003 Bermai, 2005-2007 DSP Group, Inc. All rights reserved.
//

#ifndef __MOJAVE_H
#define __MOJAVE_H

///////////////////////////////////////////////////////////////////////////////
//
// Silicon Version: Only define one of the following
//
//     MOJAVE_VERSION_1  -- Original DW52MB design
//     MOJAVE_VERSION_1B -- Metal spin
//
#if !defined(MOJAVE_VERSION_1) && !defined(MOJAVE_VERSION_1B)
    #define  MOJAVE_VERSION_1B
#endif
///////////////////////////////////////////////////////////////////////////////

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  PHY REGISTER SET STRUCTURE
//=================================================================================================
typedef struct
{                            // ---------------------------------------------
    wiUWORD ID;              // Baseband ID register
                             // ---------------------------------------------
    wiUWORD ADCBypass;       // Bypass ADC with test bus into baseband RX
    wiUWORD Disable72;       // Disable 72 Mbps mode
    wiUWORD PathSwap;        // Active radio path for single channel (0,1)
    wiUWORD PathSel;         // Receive path selection (1,2,3)
                             // ---------------------------------------------
    wiUWORD SREdge;          // Serial port interface read clock edge
    wiUWORD SRFreq;          // Serial port interface clock rate
    wiUWORD RXMode;          // (Top) Baseband receiver mode (802.11a, 11b, 11g)
                             // ---------------------------------------------
    wiUWORD DCXSelect;       // Select the DCX output
    wiUWORD DCXShift;        // Selects pole position of DCX HPF
    wiUWORD DCXHoldAcc;      // Hold the accumulator value (no updates)
    wiUWORD DCXFastLoad;     // No fast-charge of accumulators on ClearDCX
    wiWORD  DCXAcc0RH;       // DCX Accumulators, Low and High words
    wiUWORD DCXAcc0RL;       //  "        " 
    wiWORD  DCXAcc0IH;       //  "        "   
    wiUWORD DCXAcc0IL;       //  "        " 
    wiWORD  DCXAcc1RH;       //  "        " 
    wiUWORD DCXAcc1RL;       //  "        " 
    wiWORD  DCXAcc1IH;       //  "        " 
    wiUWORD DCXAcc1IL;       //  "        " 
    wiUWORD DCXThDFS;        // Threshold for large DC term with DFS PulseDet
                             // ---------------------------------------------
    wiUWORD UpmixConj;       // Apply complex conjugate to the upmixer output
    wiUWORD DownmixConj;     // Apply complex conjugate to the downmixer input
    wiUWORD DmixEnDis;       // RTL verification test signal (from Lijun)
    wiUWORD UpmixDown;       // Make the up-mixer a down-mixer
    wiUWORD DownmixUp;       // Make the up-mixer a down-mixer
    wiUWORD UpmixOff;        // Disable/bypass the upmixer
    wiUWORD DownmixOff;      // Disable/bypass the downmixer
    wiUWORD TestMode;        // Baseband test mode control
                             // ---------------------------------------------
    wiUWORD PrdDACSrcR;      // (PrD) Source for DAC input (Real)
    wiUWORD PrdDACSrcI;      // (PrD) Source for DAC input (Imag)
                             // ---------------------------------------------
    wiUWORD DLY_TXB;
    wiUWORD TxExtend;
    wiUWORD TxDelay;
    wiUWORD TX_RATE;         // Rate field (4-bits)
    wiUWORD TX_PWRLVL;       // Power level
    wiUWORD TX_LENGTH;       // Length field (12-bits, 0-4095)
    wiUWORD TX_SERVICE;      // Service field (16-bits, typically 0)
    wiUWORD TxFault;       
    wiUWORD TX_PRBS;         // Starting state for the TX scrambler
                             // ---------------------------------------------
    wiUWORD RxFault;         // Set for invalid RATE/LENGTH/Parity in SIGNAL
    wiUWORD RX_RATE;         // Rate field (4 bits)
    wiUWORD RX_LENGTH;       // Length field (12-bits, 0-4095)
    wiUWORD RX_SERVICE;      // Service field (16-bits, typically 0)
    wiUWORD RSSI0;           // RSSI for path 0
    wiUWORD RSSI1;           // RSSI for path 1
    wiUWORD maxLENGTH;       // Limit for maximum packet length (divided-by-16)
    wiUWORD Reserved0;       // Required reserved bit in SIGNAL field to be 0
                             // ---------------------------------------------
    wiUWORD PilotPLLEn;      // (PLL) Enable Pilot Tone PLL
    wiUWORD aC;              // (PLL) proportional gain for Carrier phase correction
    wiUWORD bC;              // (PLL) integral     gain for Carrier phase correction
    wiUWORD cC;              // (PLL) forward      gain for Carrier phase correction
    wiUWORD Sa_Sb[16];       // (PLL) Sampling PLL gain terms
    wiUWORD Ca_Cb[16];       // (PLL) Carrier PLL gain terms
                             // ---------------------------------------------
    wiUWORD LgSigAFEValid;   // (AGC) Flag validating the LgSigAFE signal
    wiUWORD ArmLgSigDet3;    // (AGC) Re-arm large signal detect in DFE state 3
    wiUWORD StepLNA;         // (AGC) LNA gain step
    wiUWORD FixedGain;       // (AGC) Disable AGC
    wiUWORD FixedLNAGain;    // (AGC) LNAGain value for FixedGain
    wiUWORD InitAGain;       // (AGC) Initial AGain setting
    wiUWORD ThSigLarge;      // (AGC) Large signal suppression threshold
    wiUWORD ThSigSmall;      // (AGC) Small signal suppression threshold
    wiUWORD StepLarge;       // (AGC) Large suppression gain step
    wiUWORD StepSmall;       // (AGC) Small suppression gain step
    wiUWORD ThSwitchLNA;     // (AGC) Threshold for reducing LNA gain
    wiUWORD AbsPwrL;         // (AGC) Lower corner of fixed gain, measured power
    wiUWORD AbsPwrH;         // (AGC) Upper corner of fixed gain, measured power
    wiUWORD RefPwrDig;       // (AGC) Target power for AGC output signal
    wiUWORD Pwr100dBm;       // (AGC) Measured power at -100dBm input
    wiUWORD ThCCA1;          // (AGC) Threshold for CCA
    wiUWORD ThCCA2;          // (AGC) Threshold for CCA after signal detect
    wiUWORD StepLgSigDet;    // (AGC) LgSigDet suppression gain step
    wiUWORD UseLNA2;         // (AGC) Enable the use of LNAGain2
    wiUWORD FixedLNA2Gain;   // (AGC) LNAGain2 value for FixedGain
    wiUWORD UpdateOnLNA;     // (AGC) Force another gain update after reducing LNAGain
    wiUWORD StepLNA2;        // (AGC) LNA2 gain step
    wiUWORD ThSwitchLNA2;    // (AGC) Threshold for reducing LNA2 gain
    wiUWORD MaxUpdateCount;  // (AGC) Maximum number of updates before final update
    wiUWORD ThStepUpRefPwr;  // (AGC) Max packet power to enable StepUp
    wiUWORD ThStepUp;        // (AGC) Power increase to signal StepUp
    wiUWORD ThStepDown;      // (AGC) Power descrease to signal StepDown
                             // ---------------------------------------------
    wiUWORD ThDFSUp;         // (DFS) Threshold for rising edge of DFS pulse
    wiUWORD ThDFSDn;         // (DFS) Threshold for falling edge of DFS pulse
    wiUWORD DFSOn;           // (DFS) Enable DFS signal detection/classification
    wiUWORD DFSCCA;          // (DFS) Replace power-level CCA with pulse detector
    wiUWORD DFS2Candidates;  // (DFS) Maintain 2 candidate pulses in classifier
    wiUWORD DFSSmooth;       // (DFS) Merge closely spaced pulses
    wiUWORD DFSMACFilter;    // (DFS) Ignore pulses when MAC_NOFILTER is asserted
    wiUWORD DFSHdrFilter;    // (DFS) Ignore pulses if width indicated by SIGNAL is consistent with width
    wiUWORD DFSIntDet;       // (DFS) Enable interrupt on detected radar
    wiUWORD DFSminPPB;       // (DFS) minimum number of pulses per burst
    wiUWORD DFSPattern;      // (DFS) pattern types for classifier
    wiUWORD DFSdTPRF;        // (DFS) tolerance on time between pulse rising edges
    wiUWORD DFSmaxWidth;     // (DFS) maximum pulse width
    wiUWORD DFSmaxTPRF;      // (DFS) maximum pulse repetition period
    wiUWORD DFSminTPRF;      // (DFS) minimum pulse repetition period
    wiUWORD DFSminGap;       // (DFS) minimum gap between pulses to avoid merging
    wiUWORD DFSPulseSR;      // (DFS) contents of classifier pattern shift register
    wiUWORD detRadar;        // (DFS) set when radar is detected; cleared on DFSOn: 0->1
    wiUWORD detHeader;       // (DFS) set when header is detected
    wiUWORD detPreamble;     // (DFS) set when OFDM preamble is detected
    wiUWORD detStepDown;     // (DFS) detected step-down event
    wiUWORD detStepUp;       // (DFS) detected step-up event
                             // ---------------------------------------------
    wiUWORD CFOFilter;       // (DFE) CFO measurement filter
    wiUWORD CFOMux;          // (DFE) Select digital path for CFO test output
    wiUWORD CFOTest;         // (DFE) Select CFO output for test port
    wiUWORD WaitAGC;         // (DFE) Delay between signal detect and AGC
    wiUWORD DelayAGC;        // (DFE) AGC latency in 20 MHz clock periods
    wiUWORD DelayCFO;        // (DFE) Delay to load 1 short preamble symbol
    wiUWORD DelayRSSI;       // (DFE) Delay to valid RSSI after gain change
    wiUWORD SigDetWindow;    // (DFE) Window for validating the signal detect
    wiUWORD IdleFSEnB;       // (DFE) FSEnB setting in DFE state 1 (pre-SigDet)
    wiUWORD IdleSDEnB;       // (DFE) SDEnB setting in DFE state 1 (i.e. enable Signal Detect?)
    wiUWORD IdleDAEnB;       // (DFE) DAEnB setting in DFE state 1 (i.e. enable Digital Amp?)
    wiUWORD DelayRestart;    // delay for restarting DFE
    wiUWORD SyncShift;       // (FS)  Negative delay for sync position
                             // ---------------------------------------------
    wiUWORD SigDetFilter;    // (SD)  Signal detector input filter
    wiUWORD SigDetTh1;       // (SD)  Signal detector threshold #1
    wiUWORD SigDetTh2H;      // (SD)  Signal detector threshold #2 (bits [15:8])
    wiUWORD SigDetTh2L;      // (SD)  Signal detector threshold #2 (bits [ 7:0]
    wiUWORD SigDetDelay;     // (SD)  Signal detector startup delay
                             // ---------------------------------------------
    wiUWORD SyncFilter;      // (FS)  FrameSync input filter
    wiUWORD SyncMag;         // (FS)  Magnitude for sign-only sample inputs
    wiUWORD SyncThSig;       // (FS)  Threshold significant
    wiWORD  SyncThExp;       // (FS)  Threshold exponent (qualify)
    wiUWORD SyncDelay;       // (FS)  Delay for peak detection (qualify)
                             // ---------------------------------------------
    wiUWORD MinShift;        // (ChEst) Minimum shift value
    wiUWORD ChEstShift0;     // (ChEst) Noise power estimate, path 0
    wiUWORD ChEstShift1;     // (ChEst) Noise power estimate, path 1
                             // ---------------------------------------------
    wiUWORD OFDMSwDEn;       // (SwD) Enable switched antenna diversity
    wiUWORD OFDMSwDInit;     // (SwD) Initial antenna for switched antenna diversity
    wiUWORD OFDMSwDSave;     // (SwD) Use previously chosen antenna
    wiUWORD OFDMSwDTh;       // (SwD) Signal power threshold for antenna search
    wiUWORD OFDMSwDDelay;    // (SwD) Propagation delay associated with antenna switch
                             // ---------------------------------------------
                             // DSSS/CCK (bMojave) Modem
                             // ---------------------------------------------
    wiUWORD aC1;             // proportional gain of carrier, #right bit shift [0,6], DSSS
    wiUWORD bC1;             // integral gain for carrier, #right bit shift [0,7], DSSS
    wiUWORD aS1;             // proportional gain of sampling, #right bi shift [0,6], DSSS
    wiUWORD bS1;             // integral gain for sampling, #right bit shift [0,7], DSSS

    wiUWORD aC2;             // proportional gain of carrier, #right bit shift [0,6], CCK
    wiUWORD bC2;             // integral gain for carrier, #right bit shift [0,7], CCK
    wiUWORD aS2;             // proportional gain of sampling, #right bi shift [0,6], CCK
    wiUWORD bS2;             // integral gain for sampling, #right bit shift [0,7], CCK
                             // ---------------------------------------------
    wiUWORD CQwindow;        // #symbols for correlation quality, exponent of 2
    wiUWORD CQthreshold;     // threshold for CQ, 2 significant+4 exponent bits
    wiUWORD EDwindow;        // #symbols for energy detection, exponent of 2
    wiUWORD EDthreshold;     // threshold for energy detect, power of 2^(1/4)
    wiUWORD CSMode;          // carrier sense mode (0=CQ&ED, 1=CQ, 2=ED, 3=ED|CQ)
    wiUWORD bCFOQual;        // qualify autocorrelation value used for CFO estimate
    wiUWORD bSwDDelay;       // settling time for antenna switch at 11MHz clocks
    wiUWORD bSwDTh;          // threshold for antenna switch, power of 2^(1/4)
    wiUWORD bWaitAGC;        // waiting period before AGC starts at 22MHz clocks
    wiUWORD bAGCdelay;       // settling time for gain update at 22MHz clocks
    wiUWORD bMaxUpdateCount; // maximum number of gain updates for "b" AGC
    wiUWORD SSwindow;        // #symbols for symbol sync and CFO estimation, eponent of 2
    wiUWORD INITdelay;       // waiting period before starting carrier sensing
    wiUWORD SFDWindow;       // #symbols for SFD search
    wiUWORD SymHDRCE;        // #header symbols usable for channel estimation
    wiUWORD bThSigLarge;     // threshold for a fixed gain update
    wiUWORD bStepLarge;      // step size for a fixed gain update
    wiUWORD bRefPwr;         // target power of gain control
    wiUWORD bThSwitchLNA;    // threshld for LNA switching based on VGA gain
    wiUWORD bThSwitchLNA2;   // threshld for LNA2 switching based on VGA gain
    wiUWORD bInitAGain;      // initial gain of VGA
    wiUWORD bAGCBounded4;    // value for AGCBounded in state 4, RXMode = 2
    wiUWORD bLgSigAFEValid;  // Flag validating the LgSigAFE signal
    wiUWORD bStepLNA;        // LNA gain step
    wiUWORD bStepLNA2;       // LNA2 gain step
    wiUWORD bSwDEn;          // antenna diversity: 1=enable,0=disable
    wiUWORD bSwDSave;        // retain previous antenna from packet to packet
    wiUWORD bSwDInit;        // initial value for antenna selection search
    wiUWORD CENSym1;         // #symbols for 1st ch. estimate, exponent of 2
    wiUWORD CENSym2;         // #symbols for 2nd ch. estimate, exponent of 2 (This includes those symbols for the first estimation)
    wiUWORD hThreshold1;     // threshold to remove small cursors, 1st est.
    wiUWORD hThreshold2;     // threshold for 1st cursor detect for CCK
    wiUWORD hThreshold3;     // threshold to remove small cursors, 2nd est.
    wiUWORD DAmpGainRange;   // maximum gain change for digital Amp, 1.5dBstep, <=16
    wiUWORD bPwr100dBm;      // (bAGC) Measured power at -100dBm input
    wiUWORD bPwrStepDet;     // Enable power step detector
    wiUWORD bThStepUpRefPwr; // Minimum reference power for step up detector
    wiUWORD bThStepUp;       // Threshold for power step-up detector
    wiUWORD bThStepDown;     // Threshold for power step-down detector
                             // ---------------------------------------------
                             // Top Level
                             // ---------------------------------------------
    wiUWORD AntSel;
    wiUWORD CFO;
    wiUWORD RxFaultCount;
    wiUWORD RestartCount;
    wiUWORD NuisanceCount;
    wiUWORD RSSIdB;          // RSSI format {0: 3/4 dB; 1: 1dB}
    wiUWORD ClrOnWake;       // Clear counters on PHY wakeup
    wiUWORD ClrOnHdr;        // Clear counters after transfer to the MAC
    wiUWORD ClrNow;          // Clear counters now
                             // ---------------------------------------------
                             // (Nuisance Packet) Address Filter
                             // ---------------------------------------------
    wiUWORD AddrFilter;
    wiUWORD FilterAllTypes;
    wiUWORD FilterBPSK;
    wiUWORD FilterQPSK;
    wiUWORD Filter16QAM;
    wiUWORD Filter64QAM;
    wiUWORD STA0;
    wiUWORD STA1;
    wiUWORD STA2;
    wiUWORD STA3;
    wiUWORD STA4;
    wiUWORD STA5;
    wiUWORD minRSSI;
                             // ---------------------------------------------
    wiUWORD StepUpRestart;   // Enable abort/restart on RSSI step up
    wiUWORD StepDownRestart; // Enable abort/restart on RSSI step down
    wiUWORD NoStepAfterSync; // Disable abort/restart after frame sync
    wiUWORD NoStepAfterHdr;  // Disable abort/restart after valid PLCP header
    wiUWORD KeepCCA_New;     // Keep CCA timer on packet abort due to new signal
    wiUWORD KeepCCA_Lost;    // Keep CCA timer on packet abort due to lost signal
    wiUWORD WakeupTimeH;     // Countdown time to wake the PHY from Sleep, bits [15:8]
    wiUWORD WakeupTimeL;     // Countdown time to wake the PHY from Sleep, bits [ 7:0]
    wiUWORD DelayStdby;      // Time for transition from RF Standby->RX
    wiUWORD DelayBGEnB;      // Time to enable DAC bandgap
    wiUWORD DelayADC1;       // Time to enable ADCs from Sleep
    wiUWORD DelayADC2;       // Time to enable ADCs from TX/Standby
    wiUWORD DelayModem;      // Time to enable baseband DSP
    wiUWORD DelayDFE;        // Time to enable preamble processing
    wiUWORD DelayFCAL1;      // Time to assert FCAL at startup
    wiUWORD DelayFCAL2;      // Time to assert FCAL at TX-to-RX and nuisance filter-to-RX
    wiUWORD TxRxTime;        // Countdown time to enable RX from TX
    wiUWORD DelayRFSW;       // Delay antenna switch from TX-to-RX on TX-to-RX transition
    wiUWORD DelayPA;         // Delay PA enable from start of TX operation
    wiUWORD DelayState28;    // Wait time in state 28 (for TX-RX-Standby transition)
    wiUWORD DelaySleep;      // Delay from RX-to-Sleep
    wiUWORD DelayShutdown;   // Delay from RX-to-Shutdown
    wiUWORD TimeExtend;      // Time extension for CCA and standby on nuisance packets
    wiUWORD SquelchTime;     // Timer for squelch on RX gain changes
    wiUWORD DelaySquelchRF;  // Time to release RF switch squelch
    wiUWORD DelayClearDCX;   // Time to release reset on DCX accumulator
    wiUWORD SelectCCA;       // Selection of signals used to generate CCA
                             // ---------------------------------------------
                             // I/O
                             // ---------------------------------------------
    wiUWORD RadioMC;         // Mode control values by PHY state
    wiUWORD ADCAOPM;         // Mode control values by ADC mode
    wiUWORD ADCBOPM;         // Mode control values by ADC mode
    wiUWORD DACOPM;          // Mode control values by DAC mode
    wiUWORD RFSW1;           // Control bits for RF SW1(B)
    wiUWORD RFSW2;           // Control bits for RF SW2(B)
    wiUWORD LNAEnB;          // Enable conditions for external LNAs
    wiUWORD LNAxxBOnSW2;     // Map LNA24B and LNA50B to SW2 and SW2B
    wiUWORD LNA24A_MODE;     // LNA24A signal selection
    wiUWORD LNA24B_MODE;     // LNA24B signal selection
    wiUWORD LNA50A_MODE;     // LNA50A signal selection
    wiUWORD LNA50B_MODE;     // LNA50B signal selection
    wiUWORD PA24_MODE;       // configuration for PA24 signals
    wiUWORD PA50_MODE;       // configuration for PA50 signals
    wiUWORD ADCFSCTRL;       // direct connect to ADC:FSCTRL
    wiUWORD ADCOE;           // direct connect to ADC:OE
    wiUWORD ADCOFCEN;        // direct connect to ADC:OFCEN
    wiUWORD ADCDCREN;        // direct connect to ADC:DCREN
    wiUWORD ADCMUX0;         // direct connect to ADC:MUX0
    wiUWORD ADCARDY;         // direct connect to read ADCA:RDY
    wiUWORD ADCBRDY;         // direct connect to read ADCB:RDY
    wiUWORD ADCFCAL2;        // enable FCAL2
    wiUWORD ADCFCAL1;        // enable FCAL1
    wiUWORD ADCFCAL;         // software OR of FCAL
    wiUWORD DACIFS_CTRL;     // direct connect to DAC:IFS_CTRL
    wiUWORD DACFG_CTRL0;     // direct connect to DAC:FG_CTRL0
    wiUWORD DACFG_CTRL1;     // direct connect to DAC:FG_CTRL1
    wiUWORD RESETB;          // direct control for radio RESETB
    
} MojaveRegisters;

//=================================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    unsigned int aPadFront;          // Pad samples at front of an OFDM packet (20MHz)
    unsigned int aPadBack;           // Pad samples at end of an OFDM packet (20MHz)
    unsigned int bPadFront;          // Pad samples at front of a CCK packet (11MHz)
    unsigned int bPadBack;           // Pad samples at end of a CCK packet (11MHz)
    unsigned int OfsEOP;             // Offset to approximate end-of-packet

    wiUWORD *MAC_TX_D;               // Pointer to data bytes to be transmitted
    unsigned  N_SYM;                 // Number of OFDM symbols

    wiUWORD   *a;       int Na;      // Serialized SIGNAL+PSDU
    wiUWORD   *b;       int Nb;      // Scrambled  bit stream
    wiUWORD   *c;       int Nc;      // Coded data bits (OFDM)
    wiUWORD   *p;       int Np;      // Permuted coded bits
    wiUWORD   *cReal, *cImag;        // CCK modulator output (QPSK format)
    wiWORD    *vReal, *vImag; int Nv;// Baseband frequency-domain samples
    wiWORD    *xReal, *xImag; int Nx;// Baseband time-domain samples
    wiWORD    *yReal, *yImag; int Ny;// Upsampled time-domain signal samples
    wiWORD    *zReal, *zImag; int Nz;// Low-IF digital samples (2's-complement)
    wiUWORD   *uReal, *uImag;        // DAC input (unsigned)
    wiComplex *d;       int Nd;      // DAC output

    wiWORD    ModulationROM[256];    // Modulator look-up table

    wiUWORD   jROM[2][2048];         // Interleaver "j" ROM
    wiUWORD   kROM[2][2048];         // Interleaver "k" ROM
    wiUWORD   bpscROM[16];           // RATE->BPSC

    int      bResamplePhase;         // phase between 44/40 MHz for bMojave_TX_Resample

    wiUWORD Encoder_Shift_Register;  // State of the transmit encoder
    wiUWORD Pilot_Shift_Register;    // State of the transmit pilot sign 
    int     Fault;                   // {0=No Fault, 1=RATE/LENGTH Error}
    wiBoolean SetPBCC;               // set Modulation selection bit to PBCC (even though not implemented)
    wiBoolean UseForcedSIGNAL;       // override the SIGNAL generation and use the ForcedSIGNAL value
    wiUWORD   ForcedSIGNAL;          // SIGNAL field used when ForceSIGNAL is set
    wiBoolean UseForcedbHeader;      // override the DSSS PLCP header values (but generate valid CRC)
    wiUWORD   ForcedbHeader;         // forced SIGNAL+SERVICE+LENGTH field in DSSS PLCP header

    struct {
        int      nEff;                // Number of effective DAC Bits
        unsigned Mask;                // Mask corresponding to DAC_nEff;
        double   Vpp;                 // Full scale Vpp
        double   c;                   // Scale (2^10 / Vpp-max)
        double   IQdB;                // I/Q Amplitude matching (dB)
    } DAC;

}  Mojave_TX_State;

//=================================================================================================
//  RECEIVER CLOCKS
//=================================================================================================
typedef struct
{
    unsigned int posedgeCLK20MHz;
    unsigned int posedgeCLK40MHz;
    unsigned int posedgeCLK80MHz;
    unsigned int posedgeCLK11MHz;
    unsigned int posedgeCLK22MHz;
    unsigned int posedgeCLK44MHz;
    unsigned int InitialPhase;    // advances sequence this many 80 MHz periods at reset
}  Mojave_RX_ClockState;

//=================================================================================================
//  OFDM/TOP RECEIVER SIGNALS
//=================================================================================================
typedef union // ----------------- Trace RX Top State -----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid     : 1; //< 0>
        unsigned int posedgeCLK20MHz: 1; //< 1>
        unsigned int posedgeCLK22MHz: 1; //< 2>
        unsigned int posedgeCLK44MHz: 1; //< 3>
        unsigned int State          : 5; //< 4:8>
        unsigned int PktTime        :16; //< 9:24>
        unsigned int phase80        : 7; //<25:31>
    };
} Mojave_traceRxStateType;

typedef union // ----------------- Trace RX Control -------------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int DW_PHYEnB      : 1; // < 0>
        unsigned int PHY_Sleep      : 1; // < 1>
        unsigned int MAC_TX_EN      : 1; // < 2>
        unsigned int PHY_TX_DONE    : 1; // < 3> unused
        unsigned int PHY_CCA        : 1; // < 4> 
        unsigned int PHY_RX_EN      : 1; // < 5> unused
        unsigned int PHY_RX_ABORT   : 1; // < 6> unused
        unsigned int TxEnB          : 1; // < 7>
        unsigned int TxDone         : 1; // < 8>
        unsigned int RadioSleep     : 1; // < 9>
        unsigned int RadioStandby   : 1; // <10>
        unsigned int RadioTX        : 1; // <11>
        unsigned int PAEnB          : 1; // <12>
        unsigned int AntTxEnB       : 1; // <13>
        unsigned int BGEnB          : 1; // <14>
        unsigned int DACEnB         : 1; // <15>
        unsigned int ADCEnB         : 1; // <16>
        unsigned int ModemSleep     : 1; // <17>
        unsigned int PacketAbort    : 1; // <18>
        unsigned int NuisanceCCA    : 1; // <19>
        unsigned int RestartRX      : 1; // <20>
        unsigned int DFEEnB         : 1; // <21>
        unsigned int DFERestart     : 1; // <22>
        unsigned int TurnOnOFDM     : 1; // <23> OFDM RX active
        unsigned int TurnOnCCK      : 1; // <24> DSSS/CCK RX active
        unsigned int NuisancePacket : 1; // <25> Nuisance packet detected with address filter
        unsigned int ClearDCX       : 1; // <26>
        unsigned int bPathMux       : 1; // <27>
        unsigned int __unused28     : 1; // <28>
        unsigned int __unused29     : 1; // <29>
        unsigned int posedgeCLK40MHz: 1; // <30>
        unsigned int posedgeCLK11MHz: 1; // <31>
    };
} Mojave_traceRxControlType;

typedef union // ----------------- Trace Digital Front End -------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid : 1; //< 0>
        unsigned int DFEEnB     : 1; //< 1>
        unsigned int State      : 6; //< 2: 7> // extra bit for future expansion
        unsigned int LgSigAFE   : 1; //< 8>
        unsigned int AntSel     : 1; //< 9>
        unsigned int RestartRX  : 1; //<10>
        unsigned int __unused11 : 1; //<11>
        unsigned int __unused12 : 1; //<12>
        unsigned int CCA        : 2; //<13:14>
        unsigned int CSCCK      : 1; //<15>
        unsigned int Run11b     : 1; //<16>
        unsigned int counter    : 8; //<17:24>
        unsigned int PeakFound  : 1; //<25>
    };
} Mojave_traceDFEType;

typedef union // ----------------- Trace OFDM AGC Signals -------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int AGCEnB      :1; //< 0>
        unsigned int InitGains   :1; //< 1>
        unsigned int UpdateGains :1; //< 2>
        unsigned int State       :5; //< 3:7>
        unsigned int LNAGain     :2; //< 8:9>
        unsigned int AGAIN       :6; //<10:15>
        signed   int DGAIN       :6; //<16:21>
        unsigned int CCA         :2; //<22:23>
        unsigned int LgSigAFE    :1; //<24>
        unsigned int AntSearch   :1; //<25>
        unsigned int AntSuggest  :1; //<26>
        unsigned int RecordPwr   :1; //<27>
        unsigned int UseRecordPwr:1; //<28>
        unsigned int UpdateCount :3; //<29:31>
    };
} Mojave_traceAGCType;

typedef union // ----------------- Trace Power Measurements ----------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int RSSI0       :8; //< 0:7>
        unsigned int RSSI1       :8; //< 8:15>
        unsigned int msrpwr0     :7; //<16:22>
        unsigned int msrpwr1     :7; //<23:29>
    };
} Mojave_traceRSSIType;

typedef union // ----------------- Trace DFS Radar Detection ----------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int State        : 3; //< 0:2>
        unsigned int PulseDet     : 1; //< 3>
        unsigned int PulsePass    : 1; //< 4>
        unsigned int CheckFCS     : 1; //< 5>
        unsigned int FCSPass      : 1; //< 6>
        unsigned int InPacket     : 1; //< 7>
        unsigned int Match        : 1; //< 8>
        unsigned int RadarDet     : 1; //< 9>
        unsigned int Counter      :12; //<10:21>
        unsigned int PulseSR      :10; //<22:31>
    };
} Mojave_traceDFSType;

typedef union // ----------------- Trace Signal Detect ----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int SDEnB      : 1; //< 0>
        unsigned int State      : 8; //< 1: 8>
        unsigned int SigDet     : 2; //< 9:10>
        unsigned int x          :21; //<11:31> // ~magnitude of autocorrelator
    };
} Mojave_traceSigDetType;

typedef union // ----------------- Trace Frame Sync -------------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int FSEnB      : 1; //< 0>
        unsigned int State      : 8; //< 1: 8>
        unsigned int RatioSig   :11; //< 9:19>
        unsigned int RatioExp   : 8; //<20:27> // ~magnitude of autocorrelator
        unsigned int SigDetValid: 1; //<28>
        unsigned int PeakFound  : 1; //<29>
        unsigned int AGCDone    : 1; //<30>
        unsigned int PeakSearch : 1; //<31>
    };
} Mojave_traceFrameSyncType;

//=================================================================================================
//  SIGNAL STATE -- GROUP OF SIGNALS BETWEEN CCK/OFDM BASEBANDS AND TOP CONTROL
//=================================================================================================
typedef union
{
    unsigned int word;
    struct
    {
        unsigned int SigDet      :1; // < 0> indicates signal detection and active processing
        unsigned int AGCdone     :1; // < 1> AGC is complete (RSSI is valid)
        unsigned int SyncFound   :1; // < 2> frame sync complete
        unsigned int HeaderValid :1; // < 3> SIGNAL symbol decoded to valid values
        unsigned int RxFault     :1; // < 4> SIGNAL symbol invalid
        unsigned int StepUp      :1; // < 5> RSSI step-up detected
        unsigned int StepDown    :1; // < 6> RSSI step-down detected
        unsigned int __unused7   :1; // < 7> unused
        unsigned int PulseDet    :1; // < 8> DFS pulse detector
        unsigned int ValidRSSI   :1; // < 9> indicates RSSI measurement is valid
        unsigned int dValidRSSI  :1; // <10> delayed version of ValidRSSI (T=1/20MHz)
        unsigned int RxDone      :1; // <11> RX done (end of packet)
        unsigned int PeakFound   :1; // <12> found autocorrelator peak: frame sync
        unsigned int Run11b      :1; // <13> for 11g: hand packet off to DSSS/CCK receiver
        unsigned int AntSel      :1; // <14> selected antenna
        unsigned int LargeDCX    :1; // <15> flag large DCX term
    } ;
}  Mojave_aSigState;

typedef union
{
    unsigned int word;
    struct
    {
        unsigned int SigDet      :1; // < 0>
        unsigned int AGCdone     :1; // < 1>
        unsigned int SyncFound   :1; // < 2>
        unsigned int HeaderValid :1; // < 3>
        unsigned int StepUp      :1; // < 4>
        unsigned int StepDown    :1; // < 5>
        unsigned int EnergyDetect:1; // < 6>
        unsigned int RxDone      :1; // < 7>
        unsigned int CSCCK       :1; // < 8> carrier sense out from DSSS/CCK
        unsigned int AntSel      :1; // < 9> selected antenna
        unsigned int PwrStepValid:1; // <10> flag valid operation for power step up/down
        unsigned int ValidRSSI   :1; // <11> indicate RSSI measurement is valid
    } ;
}  Mojave_bSigState;

typedef union // ----------------- Registers in Top-Level Controler ---------         
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int __unused0     : 1; // < 0> unused
        unsigned int State         : 5; // < 1:5> controller state
        unsigned int TurnOnOFDM    : 1; // < 6> OFDM RX active
        unsigned int TurnOnCCK     : 1; // < 7> DSSS/CCK RX active
        unsigned int NuisancePacket: 1; // < 8> Nuisance packet detected with address filter
        unsigned int TimerCCA      : 1; // < 9> CCA based on packet timer
        unsigned int PHY_CCA       : 1; // <10> CCA to MAC
        unsigned int dLNAGain2     : 1; // <11> delayed LNAGain[1:0]
        unsigned int dLNAGain      : 1; // <12> delayed LNAGain[1:0]
        unsigned int dAGain        : 6; // <13:18> delayed AGain[5:0]
        unsigned int bPathMux      : 1; // <19> mux control for bMojave input
        unsigned int bRXEnB        : 1; // <20> bMojave TopCtrl enable
        unsigned int PHY_Sleep     : 1; // <21> PHY_SLEEP
        unsigned int RestartRX     : 1; // <22> RestartRX
        unsigned int DFERestart    : 1; // <23> DFERestart
    };
} Mojave_CtrlState;

typedef union // ----------------- Data Converter (ADC/DAC) Control Signals ------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int ADCAOPM      : 2; // <0:1> operating mode: ADCA
        unsigned int ADCBOPM      : 2; // <2:3> operating mode: ADCB
        unsigned int DACOPM       : 3; // <4:6> operating mode: DACs
        unsigned int ADCFCAL      : 1; // <  7> reset/calibrate trigger
    };
} Mojave_DataConvCtrlState;

typedef union // ----------------- Baseband/Radio Digital I/O --------------------         
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
} Mojave_RadioDigitalIO;

typedef union // ----------------- RX Exception Event Flags ----------------------         
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int DecodedOFDM     : 1; // <  0> Completed decode of an OFDM packet
        unsigned int DecodedDSSS_CCK : 1; // <  1> Completed deocde of a DSSS/CCK packet
        unsigned int PacketCount     : 2; // <2:3> Number of decoded packets
        unsigned int rLimit_SIGNAL   : 1; // <  4> Overrun of array "r" detected in Mojave_RX2_Signal()
        unsigned int xLimit_SIGNAL   : 1; // <  5> Overrun of array "x" detected in Mojave_RX2_Signal()
        unsigned int xLimit_DataDemod: 1; // <  6> Overrun of array "x" detected in Mojave_RX2_Signal()
        unsigned int kLimit_FrontCtrl: 1; // <  7> Overrun of "k44" counter in bMojave_RX_Top()
        unsigned int Truncated_N_SYM : 1; // <  8> OFDM "N_SYM" forced to 1 to avoid array overrun in Mojave_RX2_Signal()
    };
} Mojave_RTL_ExceptionFlags;

//=================================================================================================
//  RECEIVER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    unsigned int N_SYM;              // Number of OFDM symbols for DATA
    unsigned int N_DBPS;             // Number of data bits per symbol
    unsigned int N_CBPS;             // Number of coded bits per OFDM symbol
    unsigned int N_BPSC;             // Number of coded bits per subcarrier

    unsigned int k20, k40, k80;      // Sample indices for the 20, 40, 80 MHz
    unsigned int k11, k22, k44;      // ...and 11, 22, 44 MHz clocks

    unsigned int aLength;            // (maximum) length for array "a"
    unsigned int rLength;            // (maximum) length for array "r"

    wiUWORD PHY_RX_D[4104];          // Decoded received data
    unsigned int N_PHY_RX_WR;        // Number of PHY_RX_D writes

    wiBoolean MAC_NOFILTER;          // State of the MAC_NOFILTER signal

    struct
    {
        wiBoolean Enabled;            // Enable non-RTL processing
        wiBoolean DisableCFO;         // disable CFO correction
        wiBoolean DisablePLL;         // disable PLL correction
        wiUInt    MaxSamples_CFO;     // maximum samples for coarse CFO estimate
    }  NonRTL;                       // struct NonRTL -- features not implemented in RTL    

    int N80, N40, N20, N22;                // number of samples at 80,40,20,22 MHz

    wiWORD  *pReal[2], *pImag[2];             // Digital amplifier output (pre-limter)
    wiUWORD *qReal[2], *qImag[2];             // ADC output samples
    wiWORD  *cReal[2], *cImag[2];             // DCX output samples
    wiWORD  *rReal[2], *rImag[2]; unsigned Nr;// Data select output samples
    wiWORD  *zReal[2], *zImag[2];             // Signal mixed to zero-IF (80 MS/s)
    wiWORD  *yReal[2], *yImag[2];             // Decimated low pass filter output
    wiWORD  *sReal[2], *sImag[2]; unsigned Ns;// Digital amplifier output (limited)
    wiWORD  *wReal[2], *wImag[2];             // Digital front end input (resolution reduced s)
    wiWORD  *xReal[2], *xImag[2]; unsigned Nx;// Baseband time-domain samples
    wiWORD  *vReal[2], *vImag[2]; unsigned Nv;// FFT output
    wiWORD  *uReal[2], *uImag[2]; unsigned Nu;// Phase corrector output
    wiWORD  *DReal,    *DImag;                // Hard decisions from EqD
    wiUWORD *Lp;                              // ~LLR for the permuted coded bits
    wiUWORD *Lc;            unsigned Nc;      // ~LLR for the coded bits
    wiUWORD *cA, *cB;                         // ~LLR pairs after depuncturing (Nb each)
    wiUWORD *b;             unsigned Nb;      // Scrambled PSDU
    wiUWORD *a;             unsigned Na;      // Serialized PSDU
    wiWORD  *dReal, *dImag; unsigned Nd;      // Soft-demapper input
    wiWORD  *H2;                              // Soft-demapper input

    wiUWORD *LNAGain;
    wiUWORD *AGain;
    wiWORD  *DGain;
    wiUWORD *DFEState;
    wiUWORD *DFEFlags;
    wiWORD  *AdvDly;

    wiWORD   cSP[1370], cCP[1370];    // PLL sampling and carrier phase correction
    int      kFFT[1370];              // Index for FFT start (k20)

    wiWORD   h2[2][64];                  // Frequency domain channel + noise power
    wiWORD   hReal[2][64], hImag[2][64]; // Frequency channel response (estimated)

    wiWORD   DemodROM[256];          // Hard decision Demodulator look-up table
    wiUWORD  NoiseShift[2];          // floor(log2(Noise Power)) for EqD
    wiUWORD  CFO;                    // estimated CFO from OFDM CFO estimator

    int      Fault;                  // {0=None, 1=SIGNAL, 2=Array overrun, 3=Missed SigDet}
    int      x_ofs;                  // Offset in x array (estimated)
    int      DownmixPhase;           // phase for downmix oscillator (counter value)
    int      DownsamPhase;           // phase for downsampling from 80 MHz to 20 MHz
//   int      FFT_Delay;              // Delay of FFT output from input
//   int      PLL_Delay;              // Delay of PLL output from input

    struct {
        int       nEff;               // Number of effective ADC Bits
        unsigned  Mask;               // Mask corresponding to ADC_nEff;
        int       Offset;             // Output offset (LSBs)
        double    Vpp;                // Full scale Vpp
        double    c;                  // Scale (2^10 / Vpp-max)
        wiBoolean Rounding;           // Enable rounding during quantization
    } ADC;

    int ViterbiType;                 // Viterbi function (0=Register Exchange, 1=Trace Back) [obsolete, but referenced in MojaveTest code]
    Mojave_RX_ClockState clock;      // Baseband receiver clock edges

    int dx, du, dv, dc;              // Array point offsets (used in RX2)
    wiWORD cCPP;                     // DPLL correction terms for carrier phase from pilot tone PLL
    wiWORD dAdvDly, d2AdvDly;        // One and two-symbol delayed version of AdvDly
    int PacketType;                  // Type of last packet with valid header; (0=none, 1=OFDM, 2=DSSS/CCK)
    wiUWORD aPacketDuration;         // OFDM     Packet length (microseconds)

    struct {                         // TX phase: simulate I/O and top-level control during TX event
        wiBoolean    Enabled;         // enable TX timing model during RX
        unsigned int k1;              // k80 index for rising  edge of MAC_TX_EN
        unsigned int k2;              // k80 index for falling edge of MAC_TX_EN
        unsigned int kTxDone;         // k80 index for TxDone strobe
    } TX;

    struct {                         // EVM Test Structure (not in Dakota ASIC)
        wiBoolean Enabled;            // Enable EVM function
        wiInt     N_SYM;              // Number of symbols in EVM
        wiComplex h[2][64];           // Estimated channel response
        wiComplex g[2][64];           // Equalizer
        wiComplex *x[2][64];          // Equalized samples by x[subcarrier][symbol]
        wiReal    cSe2[2][64];        // Se2 by subcarrier
        wiReal    nSe2[2][1024];      // Se2 by symbol #
        wiReal    Se2[2];             // Sum of squared errors
        wiReal    EVM[2];             // Measured EVM (linear)
        wiReal    EVMdB[2];           // Measured EVM [dB]
        wiUInt    NcCFO;              // Coarse CFO estimator sample size (samples at 20 MHz)
        wiReal    cCFO;               // Coarse estimated CFO (fraction of channel)
        wiReal    fCFO;               // Fine estimated CFO
        wiInt     nAGC;               // Number of AGC updates
    } EVM;                           

    wiUWORD  EventFlag;              // Event flags to verification purposes (legacy)
                                                // b0  = Missed signal detect
                                                // b1  = RX_LENGTH = 0
                                                // b2  = Parity error in SIGNAL symbol
                                                // b3  = Unrecognized RX_RATE
                                                // b4  = DFE detected false signal detect
                                                // b5  = LNA gain reduced
                                                // b6  = AdvDly other than 0 during packet
                                                // b7  = carrier phase error exceeds +/- pi
                                                // b8  = Received data set to short...buffer overrrun
                                                // b9  = Noise shift for demapper limited to MinShift in ChEst
                                                // b10 = Viterbi metric normalization exercised
                                                // b11 = Multiple minimum metrics in Viterbi decoder
                                                // b12 = EqD output value(s) to PLL limited
                                                // b13 = x_ofs artificially limited
                                                // b14-16 = # of gain updates

    Mojave_RTL_ExceptionFlags  RTL_Exception;  // exception event flags used for RTL verification

    Mojave_aSigState          *aSigOut;        // trace "aSigState" register output
    Mojave_bSigState          *bSigOut;        // trace "bSigState" register output
    Mojave_traceRxStateType   *traceRxState;   // trace RX top state
    Mojave_traceRxControlType *traceRxControl; // trace top-level RX control
    Mojave_traceDFEType       *traceDFE;       // trace OFDM Digital Front End
    Mojave_traceAGCType       *traceAGC;       // trace signals from OFDM AGC
    Mojave_traceRSSIType      *traceRSSI;      // trace power measurements from OFDM AGC/RSSI
    Mojave_traceDFSType       *traceDFS;       // trace DFS
    Mojave_traceSigDetType    *traceSigDet;    // trace OFDM signal detect
    Mojave_traceFrameSyncType *traceFrameSync; // trace OFDM frame synchronization
    Mojave_RadioDigitalIO     *traceRadioIO;// trace radio digital interface   
    Mojave_DataConvCtrlState  *traceDataConv;  // trace data converter control interface

    wiBoolean EnableModelIO;         // Model interface I/O precisely
    wiBoolean EnableTrace;           // Enable signal trace for RTL verification
    wiBoolean TraceKeyOp;            // Verbose information during key operations

}  Mojave_RX_State;

//=================================================================================================
//  DSSS/CCK RECEIVER SIGNALS
//=================================================================================================
typedef union // ----------------- Trace CCK Baseband Interface -------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int bRXEnB     :1; //< 0> input to bMojave_RX_top
        unsigned int Run11b     :1; //< 1> input to bMojave_RX_top
        unsigned int LgSigAFE   :1; //< 2> input to bMojave_RX_top
        unsigned int bRXDone    :1; //< 3> output from bMojave_RX_top
        unsigned int AntSel     :1; //< 4> output from bMojave_RX_top
        unsigned int CCACCK     :1; //< 5> output from bMojave_RX_top
        unsigned int CSCCK      :1; //< 6> output from bMojave_RX_top
        unsigned int Restart    :1; //< 7> input to bMojave_RX_top
        unsigned int RSSICCK    :8; //<8:15> output from bMojave_RX_top
        unsigned int LNAGain    :1; //<16> output from bMojave_RX_top
        unsigned int AGain      :6; //<17:22> output from bMojave_RX_top
        signed   int DGain      :5; //<23:27> output from AGC
        unsigned int LNAGain2   :1; //<28> output from bMojave_RX_top
    };
} bMojave_traceCCKType;

typedef union // ----------------- Trace CCK Top State ----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int State   :6;  // < 0: 5>
        unsigned int Counter :11; // < 6:16>
        unsigned int CQPeak  :5;  // <17:21>
    };
} bMojave_traceStateType;

typedef union          
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int EDEnB      :1; //< 0>
        unsigned int traceValid :1; //< 1>
        unsigned int CQEnB      :1; //< 2>
        unsigned int CQDone     :1; //< 3>
        unsigned int CSEnB      :1; //< 4>
        unsigned int AGCEnB     :1; //< 5>
        unsigned int LargeSignal:1; //< 6>
        unsigned int GainUpdate :1; //< 7>
        unsigned int CEEnB      :1; //< 8>
        unsigned int CEDone     :1; //< 9>
        unsigned int CEMode     :1; //<10>
        unsigned int CEGet      :1; //<11>
        unsigned int __unused12 :1; //<12>
        unsigned int CFOeEnB    :1; //<13>
        unsigned int __unused14 :1; //<14>
        unsigned int CFOcEnB    :1; //<15>
        unsigned int __unused16 :1; //<16>
        unsigned int __unused17 :1; //<17>
        unsigned int __unused18 :1; //<18>
        unsigned int __unused19 :1; //<19>
        unsigned int SFDEnB     :1; //<20>
        unsigned int SFDDone    :1; //<21>
        unsigned int SymPhase   :1; //<22>
        unsigned int Preamble   :1; //<23>
        unsigned int HDREnB     :1; //<24>
        unsigned int __unused25 :1; //<25>
        unsigned int HDRDone    :1; //<26>
        unsigned int PSDUEnB    :1; //<27>
        unsigned int PSDUInit   :1; //<28>
        unsigned int PSDUDone   :1; //<29>
        unsigned int TransCCK   :1; //<30>
        unsigned int DeciFreeze :1; //<31>
    };
} bMojave_traceControlType;

typedef union // ----------------- Trace bMojave RX Interpolator Signals ----
{             // ----------------- Updated at 80 MHz Clock; valid on posedges
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int posedgeCLK40MHz : 1; //< 0>
        unsigned int posedgeCLK44MHz : 1; //< 1>
        unsigned int posedgeCLK22MHz : 1; //< 2>
        unsigned int posedgeCLK11MHz : 1; //< 3>
        unsigned int INTERPinit      : 1; //< 4>
        unsigned int m               : 7; //< 5:11>
        unsigned int NCO             :13; //<12:24>
    };
} bMojave_traceResampleType;

typedef union // ----------------- Trace CCK DPLL Signals -------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int PLLEnB    :1; //< 0>
        unsigned int __unused1 :1; //< 1> unused 
        unsigned int IntpFreeze:1; //< 2>
        unsigned int InterpInit:1; //< 3>
        signed int   cCP       :6; //< 4: 9>
        signed int   cSP       :16;//<10:25>
    };
} bMojave_traceDPLLType;

typedef union // ----------------- Trace Barker Correlator Signals ----------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int BCEnB      :1;  //< 0>
        unsigned int BCMode     :1;  //< 1>
        signed int   BCOutR     :10; //< 2:11>
        signed int   BCOutI     :10; //<12:21>
        unsigned int DesOutValid:1;  //<22>
        unsigned int DesOut     :8;  //<23:30>
    };
} bMojave_traceBarkerType;

typedef union // ----------------- Trace DSSS Demodulator Signals ----------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int DMSym       :1;  //< 0>
        unsigned int SSOutValid  :1;  //< 1>
        unsigned int SSOut       :2;  //< 2:3>
        unsigned int SSSliOutR   :1;  //< 4>
        unsigned int SSSliOutI   :1;  //< 5>
        signed   int SSCorOutR   :12; //< 6:17>
        signed   int SSCorOutI   :12; //<18:29>
    };
} bMojave_traceDSSSType;

typedef union // ----------------- Trace CCK Input Signals ----------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        signed int wReal       :8;  //< 0: 7>
        signed int wImag       :8;  //< 8:15>
        signed int yReal       :8;  //<16:23>
        signed int yImag       :8;  //<24:31>
    };
} bMojave_traceCCKFBFType;

typedef union // ----------------- Trace CCK TVMF Signals ----------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        signed int zReal       :12;  //< 0:11>
        signed int zImag       :12;  //<12:23>
        unsigned int NSym      :8;   //<24:31>
    };
} bMojave_traceCCKTVMFType;

typedef union // ----------------- Trace CCK FWT/metrics ----------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        signed int xcReal:14;  //< 0:13>
        signed int met   :15;  //<14:28>
        unsigned int Nc64: 1;  //<29>
    };
} bMojave_traceCCKFWTType;

typedef union          
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int AGCEnB     :1;
        unsigned int AGCInit    :1;
        unsigned int AGCbounded :1;
        unsigned int BCMode     :1;
        unsigned int BCEnB      :1;
        unsigned int CEEnB      :1;
        unsigned int CEGet      :1;
        unsigned int CEInit     :1;
        unsigned int CEMode     :1;
        unsigned int CFOcEnB    :1;
        unsigned int ArmLgSigDet:1;
        unsigned int CFOeEnB    :1;
        unsigned int CFOeGet    :1;
        unsigned int CQEnB      :1;
        unsigned int PLLEnB     :1;
        unsigned int DBPSK      :1;
        unsigned int DMEnB      :1;
        unsigned int EDEnB      :1;
        unsigned int SPEnB      :1;
        unsigned int SFDEnB     :1;
        unsigned int HDREnB     :1;
        unsigned int PSDUEnB    :1;
        unsigned int CSEnB      :1;
        unsigned int SymPhase   :1;
        unsigned int SymGo      :1;
        unsigned int SymGoCCK   :1;
        unsigned int SymCMF     :1; // indicate start of DSSS symbol at CMF output
        unsigned int SymCCK     :1; // indicate start of CCK symbol at CMF output
        unsigned int TransCCK   :1;
        unsigned int CMFEnB     :1; // enable for WiSE CMF
        unsigned int DeciOutEnB :1; // enable DeciOut and downsampler controller
        unsigned int CCA        :1;
    };
} bMojave_FrontCtrlRegType;

//=================================================================================================
//  DSSS/CCK RECEIVER PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiUWORD PHY_RX_D[4104];          // Decoded received data
    int     N_PHY_RX_WR;             // Number of PHY_RX_D writes

    wiUWORD      *a;      int da;   // descrambled SFD, PLCL header, PSDU
    wiUWORD      *b;      int db;   // demodulated/decoded header, PSDU bits
    int         Na,Nb;            // used to prevent memory access violation

    wiWORD      hReal[22],hImag[22];   // current channel estimate
    int         Nh;         // current channel length
    int         Np;         // length of precursor used for RAKE receiver
    int         Np2;      // length of precursor used for CCK demodulation

    wiWORD      hcReal[22],hcImag[22];   // coarse channel estimate
    int         Nhc;      // coarse channel length
    int         Npc;      // #precursor of coarse estiamte for RAKE
    int         Np2c;      // #precrsors of coarse estimate for CCK

    wiUWORD   LengthPSDU;
    wiUWORD   bRATE;
    wiUWORD   bRSSI;
    wiUWORD   bLENGTH;
    wiUWORD   bSERVICE;
    wiUWORD   bDataRate;
    wiUWORD   bModulation;
    wiUWORD   bPacketDuration; // DSSS/CCK Packet length (microseconds)

    wiBoolean CRCpass;         // {0=header crc error, 1=CRC correct}

    unsigned int N44, N40, N22, N11; // number of samples at 44,40,22,11 MHz

    wiWORD   *xReal, *xImag;      // LPF output, 40MS/s
    wiWORD   *yReal, *yImag;      // digital amp output, 40MS/s
    wiWORD   *sReal, *sImag;      // interpolator output, 44MS/s
    wiWORD   *zReal, *zImag;      // decimator1 output, 22MS/s
    wiWORD   *vReal, *vImag;      // decimator2 output, 11MS/s
    wiWORD   *uReal, *uImag;      // CFO output, 11MS/s
    wiWORD   *wReal, *wImag;      // serial to parallel output
    wiWORD   *rReal, *rImag;      // Barker Correlator outputs (22 MS/s)
    wiWORD   *pReal, *pImag;      // correlator output (constellation)

    wiWORD   *tReal, *tImag;      // CMF output

    wiUWORD  *EDOut;                       // energy detect output
    wiUWORD  *CQOut;                       // correlation quality output
    wiUWORD   CQPeak;                      // last value of CQPeak

    wiUWORD  *State;                       // state of digital front end control
    wiBoolean EnableTraceCCK;              // Enable signal trace in bMojave_RX_top

    bMojave_traceCCKType      *traceCCK;     // trace signals from CCK interface
    bMojave_traceStateType    *traceState;   // trace state from CCK controller
    bMojave_traceControlType  *traceControl; // trace signals from CCK controller
    bMojave_FrontCtrlRegType  *traceFrontReg;// trace "outReg" from bMojave_RX_top
    bMojave_traceDPLLType     *traceDPLL;    // trace signals from DPLL
    bMojave_traceBarkerType   *traceBarker;  // trace signals from Barker correlator
    bMojave_traceDSSSType     *traceDSSS;    // trace signals from DSSS demodulator
    bMojave_traceCCKFBFType   *traceCCKInput;// trace CCK input/DFE output
    bMojave_traceCCKTVMFType  *traceCCKTVMF; // trace CCK TVMF output
    bMojave_traceCCKFWTType   *traceCCKFWT;  // trace CCK FWT/metric output
    bMojave_traceResampleType *traceResample;// trace bRX Resample NCOs
    unsigned int              *k80;          // 80 MHz clock tick at trace event

}  bMojave_RX_State;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================

void Mojave_RX_ClockGenerator(wiBoolean Reset);

wiStatus Mojave_TX(int Length, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M);

wiStatus aMojave_TX(wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M);
wiReal   Mojave_Prms(void);

wiStatus Mojave_TX_Restart(void);
wiStatus Mojave_RX_Restart(bMojave_RX_State *bRX);

void     Mojave_ADC(Mojave_DataConvCtrlState SigIn, Mojave_DataConvCtrlState *SigOut, wiComplex ADCAIQ, wiComplex ADCBIQ, 
                          wiUWORD *ADCOut1I, wiUWORD *ADCOut1Q, wiUWORD *ADCOut2I, wiUWORD *ADCOut2Q);
void     Mojave_DAC(int N, wiUWORD uR[], wiUWORD uI[], wiComplex s[], int M);

void Mojave_DCX  (int n, wiWORD DCXIn, wiWORD *DCXOut, wiBoolean DCXClear, Mojave_aSigState *aSigIn) ;
void Mojave_Upmix(int N, wiWORD UpmixInR[], wiWORD UpmixInI[], wiWORD UpmixOutR[], wiWORD UpmixOutI[]);
void Mojave_Prd  (int N, wiWORD PrdInR[], wiWORD PrdInI[], wiUWORD PrdOutR[], wiUWORD PrdOutI[], wiUWORD PrdDACSrcR, wiUWORD PrdDACSrcI);

wiStatus Mojave_GetRegisterMap(wiUWORD **pRegMap);
wiStatus Mojave_SetRegisterMap(wiUWORD *xRegMap);

wiStatus Mojave_RESET(void);
wiStatus Mojave_WriteConfig(wiMessageFn MsgFunc);
wiStatus Mojave_StatePointer(Mojave_TX_State **pTX, Mojave_RX_State **pRX, MojaveRegisters **pReg);
wiStatus Mojave_ParseLine(wiString text);
wiStatus Mojave_WriteRegister(wiUInt Addr, wiUInt Data);

wiStatus Mojave_RX_FreeMemory(void);
wiStatus Mojave_TX_FreeMemory(void);
wiStatus Mojave_TX_AllocateMemory(int N_SYM, int N_BPSC, int N_DBPS, int M);
wiStatus Mojave_RX_AllocateMemory(void);

wiStatus aMojave_RX2_DATA_Demod (int n);
wiStatus aMojave_RX2_DATA_Decode(void);
wiStatus Mojave_PowerOn(void);
wiStatus Mojave_PowerOff(void);

wiStatus Mojave_RX(wiBoolean DW_PHYEnB,Mojave_RadioDigitalIO *Radio, Mojave_DataConvCtrlState *DataConvIn, 
                   Mojave_DataConvCtrlState DataConvOut, Mojave_CtrlState *inReg, Mojave_CtrlState outReg);
                   

wiStatus bMojave_ParseLine(wiString text);
wiStatus bMojave_StatePointer(bMojave_RX_State **pbRX);
wiStatus bMojave_PowerOn(void);
wiStatus bMojave_RX_AllocateMemory(void);
wiStatus bMojave_RX_FreeMemory(void);
wiStatus bMojave_TX_AllocateMemory(int Na, int Nc, int M);
wiStatus bMojave_TX ( wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, 
                      int M, MojaveRegisters *pReg, Mojave_TX_State *pTX );

// Functions in bMojave.c
void bMojave_RX_top(Mojave_RX_State *pRX, bMojave_RX_State *RX, MojaveRegisters *pReg, wiBoolean bRXEnB, 
                    wiWORD  *qReal, wiWORD  *qImag, wiUWORD *AGain, wiUWORD *LNAGain, wiBoolean LgSigAFE, 
                    wiUWORD *CCACCK, wiUWORD *RSSICCK, wiBoolean Run11b, wiBoolean Restart,
                    Mojave_bSigState *bSigOut, Mojave_bSigState bSigIn);

// Functions in Mojave_FFT.c
void Mojave_FFT(int TxBRx, wiWORD FFTTxDIR[], wiWORD FFTTxDII[], wiWORD FFTRxDIR[], wiWORD FFTRxDII[],
                wiWORD *FFTTxDOR, wiWORD *FFTTxDOI, wiWORD *FFTRxDOR, wiWORD *FFTRxDOI);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

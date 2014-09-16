//
//  Verona.h -- 802.11n Single Spatial Stream Handheld Baseband
//  Copyright 2007-2010 DSP Group, Inc. All rights reserved.
//

#ifndef __VERONA_H
#define __VERONA_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Part ID
//  This is hard-coded to override RegMap changes that result from loading values to RegMap
//
#define VERONA_PART_ID 9
        
//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Timing Definitions
//  These values are adjusted to match hardware latencies.
//  ------------------
//  VERONA_FFT_OFFSET     - delay from peak found to the start of an FFT (RX control)
//  VERONA_FFT_DELAY      - processing/pipeline delay for the (I)FFT
//  VERONA_PLL_DELAY      - delay due to the PLL phase corrector
//  VERONA_SIG_LATENCY    - latency from L-SIG FFT start to the decoded output
//  VERONA_GF_SIG_LATENCY - latency from the HT-SIG FFT start to decoded output (Greenfield)
//  VERONA_DPD_DELAY      - signal delay for DPD vs bypass
//
#define VERONA_FFT_OFFSET      2
#define VERONA_FFT_DELAY      82
#define VERONA_PLL_DELAY       0
#define VERONA_SIG_LATENCY    70
#define VERONA_HTSIG_LATENCY  76 
#define VERONA_DPD_DELAY       4

//////////////////////////////////////////////////////////////////////////////////////////////
//
//  ARRAY LENGTHS
//  -------------
//  VERONA_MAX_N_SYM      - maximum number of OFDM symbols. 2560 --> 10.2 ms which is greater
//                          than any 802.11a packet or 802.11n aPPDUMaxTime (10.0 ms)
//  VERONA_MAC_DV_TX      - maximum number of MAC_TX_D arrays stored in DvState
//
#define VERONA_MAX_N_SYM 2560
#define VERONA_MAX_DV_TX    4 

//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Enumerate Subcarrier Type Labels
//
enum VeronaSubcarrierTypeE
{
    VERONA_NULL_SUBCARRIER  = 0,
    VERONA_DATA_SUBCARRIER  = 1,
    VERONA_PILOT_SUBCARRIER = 2
};

//======================================================================================
//  PHY REGISTER SET STRUCTURE
//======================================================================================
typedef struct
{                            // ---------------------------------------------
    wiUWORD ID;              // Baseband ID register
                             // ---------------------------------------------
    wiUWORD ADCBypass;       // Bypass ADC with test bus into baseband RX
    wiUWORD PathSel;         // Receive path selection (1,2,3)
                             // ---------------------------------------------
    wiUWORD RXMode;          // (Top) Baseband receiver mode (802.11a, 11b, 11g, 11n)
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
    wiUWORD UpmixDown;       // Make the up-mixer a down-mixer
    wiUWORD DownmixUp;       // Make the up-mixer a down-mixer
    wiUWORD UpmixOff;        // Disable/bypass the upmixer
    wiUWORD DownmixOff;      // Disable/bypass the downmixer
    wiUWORD TestMode;        // Baseband test mode control
                             // ---------------------------------------------
    wiUWORD TxIQSrcR;        // (TxSrc) Source for I/Q input (Real)
    wiUWORD TxIQSrcI;        // (TxSrc) Source for I/Q input (Imag)
    wiUWORD PrdDACSrcR;      // (PrD) Source for DAC input (Real)
    wiUWORD PrdDACSrcI;      // (PrD) Source for DAC input (Imag)
                             // ---------------------------------------------
    wiUWORD DLY_TXB;
    wiUWORD aTxExtend;
    wiUWORD aTxDelay;
    wiUWORD bTxExtend;
    wiUWORD bTxDelay;
    wiUWORD TX_SERVICE;      // Service field (16-bits, typically 0)
    wiUWORD TX_PRBS;         // Starting state for the TX scrambler
    wiUWORD TxIQSrcRun;      // Run logic from TxIQSrc to DAC input regardless of RX/TX state
    wiUWORD TxSmoothing;     // Smoothing bit in transmitted HT-SIG
                             // ---------------------------------------------
    wiUWORD RX_SERVICE;      // Service field (16-bits, typically 0)
    wiUWORD RSSI0;           // RSSI for path 0
    wiUWORD RSSI1;           // RSSI for path 1
    wiUWORD maxLENGTH;       // Limit for maximum packet length (divided-by-16)
    wiUWORD Reserved0;       // Required reserved bit in SIGNAL field to be 0. Set to 1 to enable checking
    wiUWORD nReserved1;      // Required reserved bit in HT-SIG field to be 1. Set to 1 to enable checking
    wiUWORD NotSounding;     // Treat packets with NotSounding = 0 as unsupported if set
    wiUWORD nPktDuration;    // Update the HT PktDuration for HT unsupported packets
    wiUWORD maxLENGTHnH;     // Limit for maximum packet length MSByte;
    wiUWORD maxLENGTHnL;     // Limit for maximum packet length LSByte; maxLengthn = 256*maxLENGTHnH+maxLENGTHnL
    wiUWORD aPPDUMaxTimeH;   // Limit for packet duration is 256*aPPDUMaxTimeH + aPPDUMaxTimeL   
    wiUWORD aPPDUMaxTimeL;   
                             // ---------------------------------------------
    wiUWORD PilotPLLEn;      // (PLL) Enable Pilot Tone PLL
    wiUWORD aC;              // (PLL) proportional gain for Carrier phase correction
    wiUWORD bC;              // (PLL) integral     gain for Carrier phase correction
    wiUWORD cC;              // (PLL) forward      gain for Carrier phase correction
    wiUWORD Sa, Sb;          // (PLL) Sampling PLL gain terms
    wiUWORD Ca, Cb;          // (PLL) Carrier PLL gain terms
                             // ---------------------------------------------
    wiUWORD sTxIQ;
    wiWORD   TxIQa11;
    wiWORD   TxIQa22;
    wiWORD   TxIQa12;
    wiUWORD sRxIQ;
    wiWORD   RxIQ0a11;
    wiWORD   RxIQ0a22;
    wiWORD   RxIQ0a12;
    wiWORD   RxIQ1a11;
    wiWORD   RxIQ1a22;
    wiWORD   RxIQ1a12;
                             // ---------------------------------------------
    wiWORD   ImgDetCoefRH;
    wiUWORD  ImgDetCoefRL;
    wiWORD   ImgDetCoefIH;
    wiUWORD  ImgDetCoefIL;
    wiUWORD  ImgDetLength;
    wiUWORD  ImgDetLengthH;
    wiUWORD  ImgDetEn;
    wiUWORD  ImgDetPostShift;
    wiUWORD  ImgDetPreShift;
    wiUWORD  ImgDetDelay;
    wiUWORD  ImgDetAvgNum;
    wiUWORD  ImgDetPowerH;
    wiUWORD  ImgDetPowerL;
    wiUWORD  CalDone;
    wiUWORD  CalResultType;
    wiUWORD  ImgDetDone;
    wiUWORD  CalRun;
    wiUWORD  CalPwrAdjust;
    wiUWORD  CalMode;
    wiUWORD  CalCoefUpdate;
    wiUWORD  CalFineStep;
    wiUWORD  CalCoarseStep;
    wiUWORD  CalPwrRatio;
    wiUWORD  CalIterationTh;
    wiUWORD  CalDelay;
    wiUWORD  CalGain;
    wiUWORD  CalPhase;
                             // ---------------------------------------------
    wiUWORD LgSigAFEValid;   // (AGC) Flag validating the LgSigAFE signal
    wiUWORD ArmLgSigDet3;    // (AGC) Re-arm large signal detect in DFE state 3
    wiUWORD UpdateGainMF;    // (AGC) Update gain in state 18 for HT-MF
    wiUWORD UpdateGain11b;   // (AGC) Update gain for 11b packet after classified as not-OFDM
    wiUWORD StepLNA;         // (AGC) LNA gain step
    wiUWORD FixedGain;       // (AGC) Disable AGC
    wiUWORD InitLNAGain;     // (AGC) Initial LNAGain value
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
    wiUWORD StepLgSigDet;    // (AGC) LgSigDet suppression gain step
    wiUWORD StepSmallPeak;   // (AGC) Small suppression gain step with detected amplitude peak
    wiUWORD UseLNA2;         // (AGC) Enable the use of LNAGain2
    wiUWORD InitLNAGain2;    // (AGC) Initial LNAGain2 value
    wiUWORD UpdateOnLNA;     // (AGC) Force another gain update after reducing LNAGain
    wiUWORD StepLNA2;        // (AGC) LNA2 gain step
    wiUWORD ThSwitchLNA2;    // (AGC) Threshold for reducing LNA2 gain
    wiUWORD MaxUpdateCount;  // (AGC) Maximum number of updates before final update
    wiUWORD ThStepUpRefPwr;  // (AGC) Max packet power to enable StepUp
    wiUWORD ThStepUp;        // (AGC) Power increase to signal StepUp
    wiUWORD ThStepDown;      // (AGC) Power decrease to signal StepDown
    wiUWORD ThStepDownMsrPwr;// (AGC) Minimum measured power to qualify StepDown
                             // ---------------------------------------------
    wiUWORD ThDFSUp;         // (DFS) Threshold for rising edge of DFS pulse
    wiUWORD ThDFSDn;         // (DFS) Threshold for falling edge of DFS pulse
    wiUWORD DFSOn;           // (DFS) Enable DFS signal detection/classification
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
    wiUWORD DelayRIFS;       // (DFE) Window for RIFS signal detect
    wiUWORD SigDetWindow;    // (DFE) Window for validating the signal detect
    wiUWORD IdleFSEnB;       // (DFE) FSEnB setting in DFE state 1 (pre-SigDet)
    wiUWORD IdleSDEnB;       // (DFE) SDEnB setting in DFE state 1 (i.e. enable Signal Detect?)
    wiUWORD IdleDAEnB;       // (DFE) DAEnB setting in DFE state 1 (i.e. enable Digital Amp?)
    wiUWORD SDEnB17;         // (DFE) SDEnB setting in DFE state 17
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
    wiUWORD ChTracking;      // (ChEst) Enable Adaptive ChEst
    wiUWORD ChTrackGain1;    // (ChEst) First scaling factor for channel tracking  
    wiUWORD ChTrackGain2;    // (ChEst) Second scaling factor for channel tracking 

    // ---------------------------------------------
    wiUWORD OFDMSwDEn;       // (SwD) Enable switched antenna diversity
    wiUWORD OFDMSwDInit;     // (SwD) Initial antenna for switched antenna diversity
    wiUWORD OFDMSwDSave;     // (SwD) Use previously chosen antenna
    wiUWORD OFDMSwDTh;       // (SwD) Signal power threshold for antenna search
    wiUWORD OFDMSwDThLow;    // (SwD) Lower bound on signal power for antenna search
    wiUWORD OFDMSwDDelay;    // (SwD) Propagation delay associated with antenna switch
                             // ---------------------------------------------
                             // DSSS/CCK (bVerona) Modem
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
    wiUWORD TxToneDiv;
    wiUWORD TxWordRH;
    wiUWORD TxWordRL;
    wiUWORD TxWordIH;
    wiUWORD TxWordIL;
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
    wiUWORD bRestartDelay;   // INITdelay used on RX-RX transition following a DSSS/CCK packet
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
    wiUWORD CENSym2;         // #symbols for 2nd ch. estimate, exponent of 2 (This includes CENSym1)
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
    wiUWORD DW_PHYEnB;       // DW_PHYEnB from MAC
    wiUWORD TopState;        // Top level state machine   
    wiUWORD sPHYEnB;         // Selection 
    wiUWORD PHYEnB;          //   
    wiUWORD CFO;
    wiUWORD RxFaultCount;
    wiUWORD RestartCount;
    wiUWORD NuisanceCount;
    wiUWORD NoStepUpAgg;     // do not allow step-up restart during A-MPDU
    wiUWORD RSSIdB;          // RSSI format {0: 3/4 dB; 1: 1dB}
    wiUWORD UnsupportedGF;   // Treat HT-GF as unsupported HT-MF
    wiUWORD ClrOnWake;       // Clear counters on PHY wakeup
    wiUWORD ClrOnHdr;        // Clear counters after transfer to the MAC
    wiUWORD ClrNow;          // Clear counters now
                             // ---------------------------------------------
                             // (Nuisance Packet) Address Filter
                             // ---------------------------------------------
    wiUWORD AddrFilter;
    wiUWORD FilterAllTypes;
    wiUWORD FilterUndecoded; // Unsupported packets as Nuisance
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
    wiUWORD RxRIFS;          // Enable reception of RIFS-seperated packets
    wiUWORD ForceRestart;    // Forced Restart
    wiUWORD KeepCCA_New;     // Keep CCA timer on packet abort due to new signal
    wiUWORD KeepCCA_Lost;    // Keep CCA timer on packet abort due to lost signal
    wiUWORD PktTimePhase80;  // Phase80 initial value when loading packet duration to PktTime
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
    wiUWORD DelayState51;    // Wait time in state 51 (for TX-RX-Standby transition)
    wiUWORD DelaySleep;      // Delay from RX-to-Sleep
    wiUWORD DelayShutdown;   // Delay from RX-to-Shutdown
    wiUWORD TimeExtend;      // Time extension for CCA and standby on nuisance packets
    wiUWORD SquelchTime;     // Timer for squelch on RX gain changes
    wiUWORD DelaySquelchRF;  // Time to release RF switch squelch
    wiUWORD DelayClearDCX;   // Time to release reset on DCX accumulator
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

    wiUWORD ADCBCTRLR;       // direct connect to ADC:BCTRL_R
    wiUWORD ADCBCTRL;        // direct connect to ADC:BCTRL
    wiUWORD ADCFSCTRL;       // direct connect to ADC:FSCTRL
    wiUWORD ADCCHSEL;        // direct connect to ADC:CHSEL
    wiUWORD ADCOE;           // direct connect to ADC:OE
    wiUWORD ADCOFCEN;        // direct connect to ADC:OFCEN
    wiUWORD ADCDCREN;        // direct connect to ADC:DCREN
    wiUWORD ADCMUX0;         // direct connect to ADC:MUX0
    wiUWORD ADCARDY;         // direct connect to read ADCA:RDY
    wiUWORD ADCBRDY;         // direct connect to read ADCB:RDY
    wiUWORD PPSEL_MODE;      // control for ADC PP_SEL signal
    wiUWORD ADCFCAL2;        // enable FCAL2
    wiUWORD ADCFCAL1;        // enable FCAL1
    wiUWORD ADCFCAL;         // software OR of FCAL
    wiUWORD VCMEN;           // control for AFE vcmen signal
    wiUWORD DACIFS_CTRL;     // direct connect to DAC:IFS_CTRL
    wiUWORD DACFG_CTRL0;     // direct connect to DAC:FG_CTRL0
    wiUWORD DACFG_CTRL1;     // direct connect to DAC:FG_CTRL1
    wiUWORD RESETB;          // direct control for radio RESETB
    wiUWORD TxLNAGAIN2;      // value of LNAGAIN2 during RadioTX
    wiUWORD LgSigSelDPD;     // LgSigAFE input select to DPD (0=LgSigAFE, 1=AntSel)
    wiUWORD SREdge;          // serial register read clock edge
    wiUWORD SRFreq;          // serial register clock frequency
    wiUWORD sLNA24A;         // multiplex select for LNA24A output
    wiUWORD sLNA24B;         // multiplex select for LNA24B output
    wiUWORD sLNA50A;         // multiplex select for LNA50A output
    wiUWORD sLNA50B;         // multiplex select for LNA50B output
    wiUWORD sPA24;           // multiplex select for PA24 output
    wiUWORD sPA50;           // multiplex select for PA50 output
                             // ---------------------------------------------
                             // CCA/PHY_RX_ERROR/Step-Up/Down Restart
                             // ---------------------------------------------
    wiUWORD SelectCCA;       // Selection of signals used to generate CCA

    wiUWORD ExtendBusyGF;    // Time (50 ns ticks) to extend CCA busy on Greenfield packets
    wiUWORD ExtendRxError;   // Time (50 ns ticks) to extend PHY_RX_ERROR
    wiUWORD ExtendRxError1;  // Time (50 ns ticks) to extend PHY_RX_ERROR, HT-GF, 1SS, 1SYM

    wiUWORD ThCCA_SigDet;    // Minimum RSSI for CCA busy on carrier sense
    wiUWORD ThCCA_RSSI1;     // Rising  edge for CCA busy on RSSI
    wiUWORD ThCCA_RSSI2;     // Falling edge for CCA busy on RSSI
    wiUWORD ThCCA_GF1;       // Rising  edge RSSI for Greenfield CCA
    wiUWORD ThCCA_GF2;       // Falling edge RSSI for Greenfield CCA


    // ---------------------------------------------
    // Digital Predistortion (DPD) Registers
    // ---------------------------------------------
	wiWORD  P1_coef_t0_r_s0;
	wiWORD  P1_coef_t0_i_s0;
	wiWORD  P1_coef_t1_r_s0;
	wiWORD  P1_coef_t1_i_s0;
	wiWORD  P1_coef_t2_r_s0;
	wiWORD  P1_coef_t2_i_s0;
	wiWORD  P1_coef_t3_r_s0;
	wiWORD  P1_coef_t3_i_s0;
	wiWORD  P1_coef_t4_r_s0;
	wiWORD  P1_coef_t4_i_s0;
	wiWORD  P3_coef_t0_r_s0;
	wiWORD  P3_coef_t0_i_s0;
	wiWORD  P3_coef_t1_r_s0;
	wiWORD  P3_coef_t1_i_s0;
	wiWORD  P3_coef_t2_r_s0;
	wiWORD  P3_coef_t2_i_s0;
	wiWORD  P5_coef_t0_r_s0;
	wiWORD  P5_coef_t0_i_s0;
	wiWORD  P5_coef_t1_r_s0;
	wiWORD  P5_coef_t1_i_s0;
	wiWORD  P5_coef_t2_r_s0;
	wiWORD  P5_coef_t2_i_s0;
	wiWORD  P1_coef_t0_r_s1;
	wiWORD  P1_coef_t0_i_s1;
	wiWORD  P1_coef_t1_r_s1;
	wiWORD  P1_coef_t1_i_s1;
	wiWORD  P1_coef_t2_r_s1;
	wiWORD  P1_coef_t2_i_s1;
	wiWORD  P1_coef_t3_r_s1;
	wiWORD  P1_coef_t3_i_s1;
	wiWORD  P1_coef_t4_r_s1;
	wiWORD  P1_coef_t4_i_s1;
	wiWORD  P3_coef_t0_r_s1;
	wiWORD  P3_coef_t0_i_s1;
	wiWORD  P3_coef_t1_r_s1;
	wiWORD  P3_coef_t1_i_s1;
	wiWORD  P3_coef_t2_r_s1;
	wiWORD  P3_coef_t2_i_s1;
	wiWORD  P5_coef_t0_r_s1;
	wiWORD  P5_coef_t0_i_s1;
	wiWORD  P5_coef_t1_r_s1;
	wiWORD  P5_coef_t1_i_s1;
	wiWORD  P5_coef_t2_r_s1;
	wiWORD  P5_coef_t2_i_s1;
	wiWORD  P1_coef_t0_r_s2;
	wiWORD  P1_coef_t0_i_s2;
	wiWORD  P1_coef_t1_r_s2;
	wiWORD  P1_coef_t1_i_s2;
	wiWORD  P1_coef_t2_r_s2;
	wiWORD  P1_coef_t2_i_s2;
	wiWORD  P1_coef_t3_r_s2;
	wiWORD  P1_coef_t3_i_s2;
	wiWORD  P1_coef_t4_r_s2;
	wiWORD  P1_coef_t4_i_s2;
	wiWORD  P3_coef_t0_r_s2;
	wiWORD  P3_coef_t0_i_s2;
	wiWORD  P3_coef_t1_r_s2;
	wiWORD  P3_coef_t1_i_s2;
	wiWORD  P3_coef_t2_r_s2;
	wiWORD  P3_coef_t2_i_s2;
	wiWORD  P5_coef_t0_r_s2;
	wiWORD  P5_coef_t0_i_s2;
	wiWORD  P5_coef_t1_r_s2;
	wiWORD  P5_coef_t1_i_s2;
	wiWORD  P5_coef_t2_r_s2;
	wiWORD  P5_coef_t2_i_s2;
	wiWORD  P1_coef_t0_r_s3;
	wiWORD  P1_coef_t0_i_s3;
	wiWORD  P1_coef_t1_r_s3;
	wiWORD  P1_coef_t1_i_s3;
	wiWORD  P1_coef_t2_r_s3;
	wiWORD  P1_coef_t2_i_s3;
	wiWORD  P1_coef_t3_r_s3;
	wiWORD  P1_coef_t3_i_s3;
	wiWORD  P1_coef_t4_r_s3;
	wiWORD  P1_coef_t4_i_s3;
	wiWORD  P3_coef_t0_r_s3;
	wiWORD  P3_coef_t0_i_s3;
	wiWORD  P3_coef_t1_r_s3;
	wiWORD  P3_coef_t1_i_s3;
	wiWORD  P3_coef_t2_r_s3;
	wiWORD  P3_coef_t2_i_s3;
	wiWORD  P5_coef_t0_r_s3;
	wiWORD  P5_coef_t0_i_s3;
	wiWORD  P5_coef_t1_r_s3;
	wiWORD  P5_coef_t1_i_s3;
	wiWORD  P5_coef_t2_r_s3;
	wiWORD  P5_coef_t2_i_s3;

	wiUWORD cross_pwr1;
	wiUWORD	cross_pwr2;
	wiUWORD	cross_pwr3;
	// wiUWORD cross_rate;
	wiUWORD Tx_Rate_OFDM_HT;
	wiUWORD Tx_Rate_OFDM_LL;
	wiUWORD Tx_Rate_OFDM_LM;

	// dpd_ctrl1
	// wiUWORD Coef_sel_eovr;
	// wiUWORD	Coef_sel_vovr;
	wiUWORD DPD_spare;
	wiUWORD	DPD_LOW_TMP_DIS;
	wiUWORD DPD_PA_tmp;
	wiUWORD DPD_LOW_RATE_DIS;
	wiUWORD DPD_coef_interspersion;

	// dpd_ctrl2
	wiUWORD Cbc_coef_sel;
	wiUWORD cnfg_eovr;
	wiUWORD Coef_sel_vovr;
	wiUWORD	Vfb_byp_vovr;
	wiUWORD	Pa_tmp_ext;

    // ---------------------------------------------
    // Data Converter BIST
    // ---------------------------------------------
    wiUWORD BISTDone;
    wiUWORD BISTADCSel;
    wiUWORD BISTNoPrechg;
    wiUWORD BISTFGSel;
    wiUWORD BISTMode;
    wiUWORD StartBIST;
    wiUWORD BISTPipe;
    wiUWORD BISTN_val;
    wiUWORD BISTOfsR;
    wiUWORD BISTOfsI;
    wiUWORD BISTTh1;
    wiUWORD BISTTh2;
    wiUWORD BISTFG_CTRLI;
    wiUWORD BISTFG_CTRLR;
    wiUWORD BISTStatus;
    wiUWORD BISTAccR;
    wiUWORD BISTAccI;
    wiUWORD BISTRangeLimH;
    wiUWORD BISTRangeLimL;
    // ---------------------------------------------
    // DFS/Radar Detect
    // ---------------------------------------------
    wiUWORD DFSIntEOB;
    wiUWORD DFSIntFIFO;
    wiUWORD DFSThFIFO;
    wiUWORD DFSDepthFIFO;
    wiUWORD DFSReadFIFO;
    wiUWORD DFSSyncFIFO;
    // ---------------------------------------------
    // MAC/PHY Header
    // ---------------------------------------------
    wiUWORD TxHeader1; // TX Header
    wiUWORD TxHeader2;
    wiUWORD TxHeader3;
    wiUWORD TxHeader4;
    wiUWORD TxHeader5;
    wiUWORD TxHeader6;
    wiUWORD TxHeader7;
    wiUWORD TxHeader8;
    wiUWORD RxHeader1; // RX Header
    wiUWORD RxHeader2;
    wiUWORD RxHeader3;
    wiUWORD RxHeader4;
    wiUWORD RxHeader5;
    wiUWORD RxHeader6;
    wiUWORD RxHeader7;
    wiUWORD RxHeader8;

    // ---------------------------------------------
    // Test Signals
    // ---------------------------------------------
    wiUWORD TestSigEn;
    wiUWORD ADCAI;
    wiUWORD ADCAQ;
    wiUWORD ADCBI;
    wiUWORD ADCBQ;
    wiUWORD aSigDet;
    wiUWORD aAGCDone;
    wiUWORD aSyncFound;
    wiUWORD aHeaderValid;
    wiUWORD bSigDet;
    wiUWORD bAGCDone;
    wiUWORD bSyncFound;
    wiUWORD bHeaderValid;

    // ---------------------------------------------
    // Baseband Status
    // ---------------------------------------------
    wiUWORD SelectSignal1;
    wiUWORD SelectSignal2;
    wiUWORD SelectTrigger1;
    wiUWORD SelectTrigger2;
    wiUWORD TriggerState1;
    wiUWORD TriggerState2;
    wiUWORD TriggerRSSI;
    wiUWORD LgSigAFE;
    wiUWORD Signal1;
    wiUWORD Signal2;
    wiUWORD Trigger1;
    wiUWORD Trigger2;

    wiUWORD ClearInt;
    wiUWORD IntEvent2;
    wiUWORD IntEvent1;
    wiUWORD IntRxFault;
    wiUWORD IntRxAbortTx;
    wiUWORD IntTxSleep;
    wiUWORD IntTxAbort;
    wiUWORD IntTxFault;
    wiUWORD Interrupt;
    wiUWORD Event2;
    wiUWORD Event1;
    wiUWORD RxFault;         // Set for invalid RATE/LENGTH/Parity in SIGNAL
    wiUWORD RxAbortTx;
    wiUWORD TxSleep;
    wiUWORD TxAbort;
    wiUWORD TxFault;
    
} VeronaRegisters_t;

typedef union // ----------------- Transmit General Purpose Trace Signals ---
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int Marker1   : 1; //< 0> general purpose timing marker pulse
        unsigned int Marker2   : 1; //< 1> general purpose timing marker pulse
        unsigned int Marker3   : 1; //< 2> general purpose timing marker pulse
        unsigned int Marker4   : 1; //< 3> general purpose timing marker pulse
        unsigned int __unused4 : 1; //< 4>
        unsigned int __unused5 : 1; //< 5>
        unsigned int __unused6 : 1; //< 6>
        unsigned int __unused7 : 1; //< 7>
        unsigned int PktStart  : 1; //< 8> mark the start of the packet (at the TX antenna)
        unsigned int PktEnd    : 1; //< 9> mark the end of the packet (at the TX antenna)
    };
} Verona_traceTxType;

//======================================================================================
//  LOOKUP TABLE STRUCTURE
//======================================================================================
typedef struct
{
    wiWORD  ModulationROM[512];    // Modulator look-up table; increased ROM size to accomodate n Mode;
    wiWORD  DemodROM[512];         // Hard decision Demodulator look-up table  

    wiUWORD jROM[2][4096];         // Interleaver "j" ROM
    wiUWORD kROM[2][4096];         // Interleaver "k" ROM
    wiUWORD bpscROM[32];           // RATE->BPSC

    wiWORD  CC55Real[ 4][8],  CC55Imag[ 4][8]; // CCK cross-correlation arrays
    wiWORD CC110Real[64][8], CC110Imag[64][8]; // ...defined in Verona_b_Init()

} Verona_LookupTable_t;


//======================================================================================
//  TRANSMITTER PARAMETER STRUCTURE
//======================================================================================
typedef struct
{
    unsigned int aPadFront;          // Pad samples at front of an OFDM packet (20MHz)
    unsigned int aPadBack;           // Pad samples at end of an OFDM packet (20MHz)
    unsigned int bPadFront;          // Pad samples at front of a CCK packet (11MHz)
    unsigned int bPadBack;           // Pad samples at end of a CCK packet (11MHz)
    unsigned int OfsEOP;             // Offset to approximate end-of-packet

    wiUWORD   *MAC_TX_D;             // Pointer to data bytes to be transmitted
    wiUWORD   RATE;                  // 802.11a RATE[3:0] or 802.11n MCS[6:0]
    wiUWORD   LENGTH;                // Packet Length;
    unsigned int N_SYM;              // Number of OFDM symbols
    unsigned int TxMode;             // 0=802.11a, 1=802.11b, 3=802.11n

    wiUWORD   *a;       int Na;      // Serialized SIGNAL+PSDU
    wiUWORD   *b;       int Nb;      // Scrambled  bit stream
    wiUWORD   *c;       int Nc;      // Coded data bits (OFDM)
    wiUWORD   *p;       int Np;      // Permuted coded bits
    wiUWORD   *cReal, *cImag;        // CCK modulator output (QPSK format)
    wiWORD    *vReal, *vImag; int Nv;// Baseband frequency-domain samples
    wiWORD    *xReal, *xImag; int Nx;// Baseband time-domain samples
    wiWORD    *yReal, *yImag; int Ny;// Upsampled time-domain signal samples
    wiWORD    *zReal, *zImag; int Nz;// Low-IF digital samples (2's-complement)
    wiWORD    *qReal, *qImag;        // I/Q correction input
    wiWORD    *rReal, *rImag;        // I/Q correction output
    wiWORD    *sReal, *sImag;        // Digital predistortion output
    wiUWORD   *uReal, *uImag;        // DAC input (unsigned)
    wiComplex *d;       int Nd;      // DAC output

    int      bResamplePhase;         // phase between 44/40 MHz for Verona_b_TX_Resample
    int      UpmixPhase;             // phase adjustment for Upmix counter
    int      TxTonePhase;            // phase adjustment for the TxTone counter

    wiUWORD Encoder_Shift_Register;  // State of the transmit encoder
    wiUWORD Pilot_Shift_Register;    // State of the transmit pilot sign 
    wiBoolean SetPBCC;               // set Modulation selection bit to PBCC (even though not implemented)
    wiBoolean UseForcedbSFD;         // override the DSSS SFD field
    wiBoolean UseForcedbSIGNAL;      // override the DSSS PLCP Header SIGNAL field
    wiBoolean UseForcedbSERVICE;     // override the DSSS PLCP Header SERVICE field
    wiBoolean UseForcedbLENGTH;      // override the DSSS PLCP Header LENGTH field
    wiBoolean UseForcedbCRC;         // override the DSSS PLCP Header CRC field
    wiBoolean UseForcedSIGNAL;       // override the L-SIGNAL generation and use the ForcedSIGNAL value
    wiBoolean UseForcedHTSIG;        // override the HT-SIG generation and use the ForceHTSIG# values
    wiBoolean UseForcedHTSIG_CRC;    // override the HT-SIG CRC and Termination field bits on UseForcedHTSIG
    wiUWORD   ForcedbSFD;            // DSSS SFD field when UseForcedbSFD is set
    wiUWORD   ForcedbSIGNAL;         // DSSS PLCP Header SIGNAL  field when UseForcedbSIGNAL is set
    wiUWORD   ForcedbSERVICE;        // DSSS PLCP Header SERVICE field when UseForcedbSERVICE is set
    wiUWORD   ForcedbLENGTH;         // DSSS PLCP Header LENGTH  field when UseForcedbLENGTH is set
    wiUWORD   ForcedbCRC;            // DSSS PLCP Header CRC     field when UseForcedbCRC is set
    wiUWORD   ForcedSIGNAL;          // L-SIGNAL field used when ForceSIGNAL is set
    wiUWORD   ForcedHTSIG1;          // HT-SIG1 field used when ForceHTSIG is set
    wiUWORD   ForcedHTSIG2;          // HT-SIG2 field used when ForceHTSIG is set (see UseForcedHT_SIG_CRC)

    wiBoolean NoTxHeaderUpdate;      // Do not update the TxHeader registers (used for DV test of TxFault)

    wiBoolean FixedTimingShift;      // do not update TimingShift value
    wiReal    TimingShift;           // timing shift for plotting (used outside WiSE)
	wiReal    PacketDuration;        // computed packet duration
  
    wiWORD    TxIQa11;               // Tx I/Q correction coefficient a11
    wiWORD    TxIQa22;               // Tx I/Q correction coefficient a22
    wiWORD    TxIQa12;               // Tx I/Q correction coefficient a12

    wiWORD dTxIQa11[2];              // 1T and 2T delayed coefficients
    wiWORD dTxIQa22[2];
    wiWORD dTxIQa12[2];

    wiBoolean LgSigAFE;              // Value of LgSigAFE during TX (for DPD model)

	struct {
		wiFunction Fn;
		wiInt Size[8];
	} LastAllocation;

    struct {
        unsigned int kMarker[5];     // Marker pulse times
    } Trace;                         // traceTx configuration structure

    struct {
        unsigned int bit_count;
        wiUWORD  PRBS;
    } DataGenerator;

    struct {
        int      nEff;               // Number of effective DAC Bits
        unsigned Mask;               // Mask corresponding to DAC_nEff;
        double   Vpp;                // Full scale Vpp
        double   c;                  // Scale (2^10 / Vpp-max)
        double   IQdB;               // I/Q Amplitude matching (dB)
    } DAC;

    struct {
        wiBoolean Enable;            // Enable checksum computation
        wiUInt    MAC_TX_D;          // Checksum for MAC_TX_D
        wiUInt    a, u, v, x;        // Checksums for arrays a, u, v, x
    } Checksum;

    Verona_traceTxType *traceTx;     // trace for general purpose signals/markers

} Verona_TxState_t;

//======================================================================================
//  RECEIVER CLOCKS
//======================================================================================
typedef struct
{
    unsigned int clock40;         // generator internal state
    unsigned int clock44;         // generator internal state
    wiUWORD clockGate40;          // generator internal state: 80->40 MHz clock gating cadence
    wiUWORD clockGate44;          // generator internal state: 80->44 MHz clock gating cadence

    unsigned int posedgeCLK20MHz;
    unsigned int posedgeCLK40MHz;
    unsigned int posedgeCLK80MHz;
    unsigned int posedgeCLK11MHz;
    unsigned int posedgeCLK22MHz;
    unsigned int posedgeCLK44MHz;
      signed int InitialPhase;    // advance sequence this many 80 MHz periods at reset
} Verona_RxClockState_t;

//=======================================================================================
//  OFDM/TOP RECEIVER SIGNALS
//=======================================================================================
typedef union // ----------------- Trace RX Top State -----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid     : 1; //< 0>
        unsigned int posedgeCLK20MHz: 1; //< 1>
        unsigned int posedgeCLK40MHz: 1; //< 2>
        unsigned int posedgeCLK80MHz: 1; //< 3>
        unsigned int posedgeCLK11MHz: 1; //< 4>
        unsigned int posedgeCLK22MHz: 1; //< 5>
        unsigned int posedgeCLK44MHz: 1; //< 6>
    };
} Verona_traceRxClockType;

typedef union // ----------------- Trace RX Top State -----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid     : 1; //< 0>
        unsigned int posedgeCLK20MHz: 1; //< 1>
        unsigned int posedgeCLK22MHz: 1; //< 2>
        unsigned int State          : 6; //< 3:8>
        unsigned int PktTime        :16; //< 9:24>
        unsigned int phase80        : 7; //<25:31>
    };
} Verona_traceRxStateType;

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
        unsigned int PHY_RX_EN      : 1; // < 5>
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
        unsigned int PHY_RX_ERROR   : 1; // <28>
        unsigned int PHY_DFS_INT    : 1; // <29>
        unsigned int posedgeCLK40MHz: 1; // <30>
        unsigned int posedgeCLK11MHz: 1; // <31>
    };
} Verona_traceRxControlType;

typedef union // ----------------- Trace Digital Front End -------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid : 1; //< 0>
        unsigned int DFEEnB     : 1; //< 1>
        unsigned int State      : 6; //< 2: 7>
        unsigned int LgSigAFE   : 1; //< 8>
        unsigned int AntSel     : 1; //< 9>
        unsigned int RestartRX  : 1; //<10>
        unsigned int __unused11 : 1; //<11>
        unsigned int __unused12 : 1; //<12>
        unsigned int __unused13 : 1; //<13>
        unsigned int __unused14 : 1; //<14>
        unsigned int CSCCK      : 1; //<15>
        unsigned int Run11b     : 1; //<16>
        unsigned int counter    : 8; //<17:24>
        unsigned int PeakFound  : 1; //<25>
    };
} Verona_traceDFEType;

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
        unsigned int wPeakDet    :1; //<22>
        unsigned int __unused23  :1; //<23>
        unsigned int LgSigAFE    :1; //<24>
        unsigned int AntSearch   :1; //<25>
        unsigned int AntSuggest  :1; //<26>
        unsigned int RecordPwr   :1; //<27>
        unsigned int UseRecordPwr:1; //<28>
        unsigned int UpdateCount :3; //<29:31>
    };
} Verona_traceAGCType;

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
} Verona_traceRSSIType;

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
} Verona_traceDFSType;

typedef union // ----------------- Trace Signal Detect ----------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int SDEnB      : 1; //< 0>
        unsigned int State      : 8; //< 1: 8>
        unsigned int SigDet     : 2; //< 9:10>
        unsigned int x          :20; //<11:30> // ~magnitude of autocorrelator
        unsigned int CheckSigDet: 1; //<31>
    };
} Verona_traceSigDetType;

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
} Verona_traceFrameSyncType;

typedef union // ----------------- Trace Image Detector ---------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int EnB        : 1; //< 0>
        unsigned int State      :10; //<1:10>
        unsigned int AcmAveEn   : 1; //<11>
        unsigned int PwrCalcEn  : 1; //<12>
        unsigned int DetDone    : 1; //<13>
        unsigned int KeepResult : 1; //<14>
        unsigned int ImgDetDone : 1; //<15>
        unsigned int ImgDetPwr  :16; //<16:31>
    };
} Verona_traceImgDetType;

typedef union // ----------------- Trace Calibration Engine -----------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int CalEn         : 1; //< 0>
        unsigned int State         : 4; //<1:4>
        unsigned int Delay         : 7; //<5:11>
        unsigned int SkipImgDet    : 1; //<12>
        unsigned int SolFound      : 1; //<13>
        unsigned int IteCount      : 3; //<14:16>
        unsigned int NeighborCount : 3; //<17:19>
        unsigned int GainLUTIndx   : 6; //<20:25>
        unsigned int PhaseLUTIndx  : 5; //<26:30>
        unsigned int CalDone       : 1; //<31:31>
    };
} Verona_traceIQCalType;

typedef union // ----------------- Trace OFDM RX2 ---------------------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid       : 1; //< 0>
        unsigned int CheckRotate      : 1; //< 1>
        unsigned int Rotated6Mbps     : 1; //< 2>
        unsigned int Rotated6MbpsValid: 1; //< 3>
        unsigned int Greenfield       : 1; //< 4>
        unsigned int Unsupported      : 1; //< 5>
        unsigned int ValidOutputFFT   : 1; //< 6> vReal, vImag contain symbol data
    };
} Verona_traceRX2Type;

typedef union // ----------------- Receiver General Purpose Trace Signals ---
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int traceValid : 1; //< 0> trace array data valid
        unsigned int Marker1    : 1; //< 1> general purpose timing marker (pulse)
        unsigned int Marker2    : 1; //< 2> general purpose timing marker (pulse)
        unsigned int Marker3    : 1; //< 3> general purpose timing marker (pulse)
        unsigned int Marker4    : 1; //< 4> general purpose timing marker (pulse)
        unsigned int Window     : 1; //< 5> window for signal comparison (OR Windows 1-4)
        unsigned int Window1    : 1; //< 6> comparison window #1
        unsigned int Window2    : 1; //< 7> comparison window #2
        unsigned int Window3    : 1; //< 8> comparison window #3
        unsigned int Window4    : 1; //< 9> comparison window #4
        unsigned int Window5    : 1; //<10> comparison window #5
        unsigned int Window6    : 1; //<11> comparison window #6
        unsigned int Window7    : 1; //<12> comparison window #7
        unsigned int Window8    : 1; //<13> comparison window #8
        signed   int FCS        : 2; //<14:15> FCS Check {-1:Fail, 0:NoCheck, +1:Pass}
    };
} Verona_traceRxType;

//======================================================================================
//  SIGNAL STATE -- GROUP OF SIGNALS BETWEEN CCK/OFDM BASEBANDS AND TOP CONTROL
//======================================================================================
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
        unsigned int ChEstDone   :1; // <16> HT ChEST Done      
        unsigned int RX_EN       :1; // <17> used to generate PHY_RX_EN
		unsigned int TimeoutRIFS :1; // <18> indicates no RIFS event detected within the searching window
		unsigned int RIFSDet     :1; // <19> Detected RIFS after nuisance packet
    };
} Verona_aSigState_t;

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
        unsigned int RX_EN       :1; // <12> used to generate PHY_RX_EN
    };
}  Verona_bSigState_t;

typedef union // ----------------- Registers in Top-Level Control -----------         
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int State         : 6; // < 0:5> controller state
        unsigned int TurnOnOFDM    : 1; // < 6> OFDM RX active
        unsigned int TurnOnCCK     : 1; // < 7> DSSS/CCK RX active
        unsigned int NuisancePacket: 1; // < 8> Nuisance packet detected with address filter
        unsigned int TimerCCA      : 1; // < 9> CCA based on packet timer
        unsigned int PHY_CCA       : 1; // <10> CCA to MAC
        unsigned int dLNAGain2     : 1; // <11> delayed LNAGain[1:0]
        unsigned int dLNAGain      : 1; // <12> delayed LNAGain[1:0]
        unsigned int dAGain        : 6; // <13:18> delayed AGain[5:0]
        unsigned int bPathMux      : 1; // <19> mux control for bVerona input
        unsigned int bRXEnB        : 1; // <20> bVerona TopCtrl enable
        unsigned int PHY_Sleep     : 1; // <21> PHY_SLEEP
        unsigned int RestartRX     : 1; // <22> RestartRX
        unsigned int DFERestart    : 1; // <23> DFERestart
		unsigned int CheckRIFS     : 1; // <24> Enable RIFS carrier sense checking
        unsigned int UpdateAGC     : 1; // <25> Enable gain update on HT-MF packet
        unsigned int PHY_RX_ERROR  : 1; // <26> PHY_RX_ERROR to MAC
        unsigned int GreenfieldCCA : 1; // <27> Greefield packet power > detection threshold
		unsigned int HoldGain      : 1; // <28> HoldGain when DFE reset for Nuisance packet with RIFS support
    };
} Verona_CtrlState_t;

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
        unsigned int ADCARDY      : 1; // <  8>
        unsigned int ADCBRDY      : 1; // <  9>
        unsigned int PP_SELA      : 1; // < 10> ping-pong select, ADCA
        unsigned int PP_SELB      : 1; // < 11> ping-pong select, ADCB
        unsigned int VCMEN        : 1; // < 12> common mode voltage enable
    };
} Verona_DataConvCtrlState_t;

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
} Verona_RadioDigitalIO_t;

typedef union // ----------------- RX Exception Event Flags ----------------------         
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int DecodedOFDM     : 1; // <  0> Completed decode of an OFDM packet
        unsigned int DecodedDSSS_CCK : 1; // <  1> Completed deocde of a DSSS/CCK packet
        unsigned int PacketCount     : 2; // <2:3> Number of decoded packets
        unsigned int rLimit_SIGNAL   : 1; // <  4> Overrun of array "r" detected in Verona_RX2_Signal()
        unsigned int xLimit_SIGNAL   : 1; // <  5> Overrun of array "x" detected in Verona_RX2_Signal()
        unsigned int xLimit_DataDemod: 1; // <  6> Overrun of array "x" detected in Verona_RX2_Signal()
        unsigned int kLimit_FrontCtrl: 1; // <  7> Overrun of "k44" counter in Verona_b_RX_Top()
        unsigned int Truncated_N_SYM : 1; // <  8> OFDM "N_SYM" forced to 1 to avoid array overrun in Verona_RX2_Signal()
    };
} Verona_RTL_ExceptionFlags_t;

//======================================================================================
//  DESIGN VERIFICATION PARAMETER STRUCTURE
//======================================================================================
typedef struct
{
    wiInt                   VerilogChecker;   // HDL (Verilog) checker type
    wiBoolean               ExpectPacketFail; // indicate packet decode failure is expected (used to check script integrity)
    wiBoolean               LongSimulation;   // indicates a long (time) test
    wiBoolean               BreakCheck;       // indicates VeronaDV_CheckScript should not continue to later waveforms

    wiUInt                  NumCheckRegisters; // number of registers to check
    wiUInt                  CheckRegAddr[512]; // list of registers (by address) to check

    wiBoolean               FixedTxTimingLimits; // manually set limits below (otherwise computed)

    wiUInt                  aTxStart;          // nominal TxStart for OFDM,     aTxDelay = 0
    wiUInt                  bTxStart;          // nominal TxStart for DSSS/CCK, bTxDelay = 0
    wiUInt                  aTxEnd;            // nominal TxStart for OFDM,     aTxExtend = 0
    wiUInt                  bTxEnd;            // nominal TxStart for DSSS/CCK, bTxExtend = 0
    wiUInt                  aTxStartTolerance; // +/- range on TxStart timing (OFDM)
    wiUInt                  bTxStartTolerance; // +/- range on TxStart timing (DSSS/CCK)
    wiUInt                  aTxEndTolerance;   // +/- range on TxEnd timing (OFDM)
    wiUInt                  bTxEndTolerance;   // +/- range on TxEnd timing (DSSS/CCK)

    wiUInt                  TxStartMinT80;     // min delay from MAC_TX_EN to PktStart
    wiUInt                  TxStartMaxT80;     // max delay from MAC_TX_EN to PktStart
    wiUInt                  TxEndMinT80;       // min delay from PktEnd to PHY_TX_DONE
    wiUInt                  TxEndMaxT80;       // max delay from PktEnd to PHY_TX_DONE

    wiUInt                  kStart;            // internal value used to construct trace array
    wiUInt                  kEnd;              // internal value used to construct trace array

    wiUInt                  LastTxStart;       // k80 for the last MAC_TX_EN assertion

    wiInt                   TestLimit;         // general purpose DV test limit value
    wiInt                   DelayT80;          // timeshift (units of 80 MHz clock periods)

    wiUWORD   *MAC_TX_D[VERONA_MAX_DV_TX];     // Pointer to data bytes to be transmitted
    wiUInt     N_PHY_TX_RD[VERONA_MAX_DV_TX];  // Length of array in pMAC_TX_D (includes PHY header)

    wiBoolean  NoRegMapUpdateOnTx;             // do not automatically update on packet TX
    wiUWORD    RegMap[1024];                   // copy of the register map prior to TX

}  Verona_DvState_t;


typedef struct  // ----------------- Verona_Downmix State --------------------------
{
    wiReg InR[2], InI[2]; // input register
    int   c[2];           // angle counter, (4) 90deg steps

} Verona_Downmix_t;

typedef struct  // ----------------- Verona_DownsamFIR State -----------------------
{
    int    c[4];      // counters for clock phase (model only)
    wiReg  x[4][21];  // FIR delay line
    wiReg  In[4][4];  // input registers
    wiReg  Out[4];    // output registers

} Verona_DownsamFIR_t;

typedef struct  // ----------------- Verona_WeightedPilotPhaseTrack State ----------
{
    wiWORD mcI;      // integral path memory
    wiWORD mcVCO;   // VCO memory
    int Shift_Reg;
    wiReg cSP_dly;   // a delay for the cSP input

} Verona_WeightedPilotPhaseTrack_t;

typedef struct  // ----------------- Verona_PLL State ------------------------------
{
    wiWORD w[64], wc[64], ws[64]; // weight storage
    wiWORD sumwc, sumws; // accumulator storage
    wiUWORD sym_cnt;     // symbol counter

    wiWORD mSI;       // accumulator for intergral path, sampling phase
    wiWORD mSVCO;     // accumulator for VCO, sampling phase
    wiWORD delayline; // phase adjustment due to the advance/delay signal
    int counter;      // time past from the last update of delay/advance value

    wiWORD mCI;    // accumulator for intergral path, carrier phase
    wiWORD mCVCO; // accumulator for VCO, carrier phase

} Verona_PLL_t;

typedef struct  // ----------------- Verona_EqD State ------------------------------
{
    int dv;          // TX: ideal information offset
    int k0_Delay[9]; // circular buffer for delayed EqD input index. Maximum delay 9    

} Verona_EqD_t;

typedef struct  // ----------------- Verona_RX2 State ------------------------------
{
    unsigned int nSym;         // number of OFDM symbols in the current packet
    unsigned int kRX2;         // symbol processing index
    unsigned int nDone;        // offset from last symbol demod to end of decode
    wiBoolean SigDemodDone;    // indicate SigDemod is done; to determine operations at state 11
    wiBoolean GfSig;           // indicates a Greenfield format packet
    unsigned int kRise, kFall; // indicating the rising and falling edge of RX_EN;    

} Verona_RX2_t;

typedef struct  // ----------------- Verona_RX_CCA State ---------------------------
{
    wiBoolean dPowerCCA;     // register delayed version of PowerCCA;
    wiBoolean dSigDetCCA;    // register delayed version of SigDetCCA;
    wiBoolean dPacketCCA;    // register delayed version of Packet Header or Data CCA
    wiBoolean dGreenfieldCCA;// register delayed version of Greenfield CCA
    wiUReg ExtendCCATimer;

} Verona_RX_CCA_t;

typedef struct  // ----------------- Verona_RX (Top) State -------------------------
{
    wiUWORD   LNAGainOFDM, LNAGainCCK;
    wiUWORD   AGainOFDM, AGainCCK;
    wiUWORD   RSSI0OFDM, RSSI1OFDM, RSSICCK;
    wiUWORD   RSSI0, RSSI1;  // registers for multiplexed RSSI values
    wiUWORD   RSSI;          // RSSI (maximum of multiple antennas)
    wiBoolean dLgSigAFE, d2LgSigAFE; // register delayed versions of LgSigAFE
    wiReg     bInR, bInI;    // input registers for "b" baseband

    wiUReg phase80; //  7b modulo-80 counter at 80 MHz (returns to 0 at 1 MHz rate)
    wiUReg PktTime; // 16b timer (count-down @ 1MHz) for CCA, nuisance packet, and power sequence
    wiUReg SqlTime; //  6b squelch timer (count-down) for radio RX gain changes

    wiUReg aValidPacketRSSI; // flag valid RSSI and AGC done
    
    // State Machine Outputs
    wiBoolean RadioSleep, RadioStandby, RadioTX;
    wiBoolean PAEnB, AntTxEnB, BGEnB, DACEnB, ADCEnB;
    wiBoolean ModemSleep, SquelchRX, dSquelchRX, RxAggregate;
    wiBoolean PacketAbort, RestartRX, DFEEnB, DFERestart, CheckRIFS, HoldGain;
    wiBoolean MAC_TX_EN; // MAC: Transmit Enable
    wiBoolean PHY_RX_EN; // PHY: Receive Enable
    wiBoolean PHY_DFS_INT; // PHY: Interrupt
    wiBoolean dPHY_DFS_INT, d2PHY_DFS_INT; // delayed versions
    wiBitDelayLine dPacketAbort;

    wiBitDelayLine DaPSDU_EN, DbPSDU_EN;
    wiBoolean aPSDU_EN, bPSDU_EN; 
			
    wiUReg Signal1, Signal2;
    wiBoolean RxDone, dRxDone;

    Verona_aSigState_t aSigIn, aSigOut;  // signals from OFDM DFE to top level control
    Verona_bSigState_t bSigIn, bSigOut;  // signals from CCK FrontCtrl to top level control

} Verona_RxTop_t;

typedef struct //  ----------------- CFO Estimate/Correct State --------------------
{
    int k;
    wiWORD AcmAxR,AcmAxI;   // accumuator output
    wiUReg state, counter;
    wiReg  theta, step;
    wiReg  s0R, s0I, s1R, s1I;
    wiUReg startCFO, endCFO;

    wiWORD w0R[80], w0I[80], w1R[80], w1I[80]; // RAM for delay and lag
    wiReg  x0R[2], x0I[2], x1R[2], x1I[2];

    unsigned int debug_nT; // counter for debug

} Verona_CFO_t;

typedef struct //  ----------------- FrameSync State -------------------------------
{
    wiUWORD    tpeak;       // peak location, temporary
    wiWORD     mem0R[176];  // real, delay line, first path
    wiWORD     mem0I[176];  // imaginary, delay line, first path
    wiWORD     mem1R[176];  // real, delay line, second path
    wiWORD     mem1I[176];  // imaginary, delay line, second path

    int    A0, A1, A2, A3;         // address counters
    wiUReg State;                  // controller state
    wiReg  w0R[3], w0I[3], w1R[3], w1I[3];     // signal input registers

    wiReg  AcmAxR, AcmAxI;         // autocorrelation accumulator
    wiReg  AcmPw0, AcmPw1;         // power measurement accumulator
    wiUReg PeakFound, SigDetValid; // peak found, signal detection valid
    wiUReg SigDet;                 // signal detected (OFDM carrier sense)
    wiUReg EnB, PathSel;
    wiUReg AGCDone, PeakSearch;

    wiReg  AxExp;    wiUReg AxSig;   // autocorrelation term in floating point (numerator)
    wiReg  PwExp;    wiUReg PwSig;   // power measurement product in floating point (denominator)
    wiReg  RatioExp; wiUReg RatioSig;// test ratio of autocorrelation to power
    wiReg  MaxExp;   wiUReg MaxSig;  // maximum measured ratio

} Verona_FrameSync_t;

typedef struct //  ----------------- AGC State -------------------------------------
{
    wiWORD  mem0R[17],mem0I[17]; // delay line for path1
    wiWORD  mem1R[17],mem1I[17]; // delay line for path2
    wiWORD  DI0R, DI0I;          // data input, path 0
    wiWORD  DI1R, DI1I;          // data input, path 1
    wiUReg  State;               // controller state
    wiUReg  AcmPwr0, AcmPwr1;    // power measurement accumulators
    wiUReg  AGain;               // analog gain
    wiReg   DGain;               // digital gain
    wiUReg  LNAGain;             // LNA gain switch
    wiUReg  LgSigDet;            // large signal detected
    wiUReg  EnB, DAEnB;          // overall AGC and DGain enables
    wiUReg  PathSel;
    wiUReg  InitGains;
    wiUWORD UpdateCount;         // implicit register (update counter)
    wiUWORD A1, A2;              // memory address counters
    wiUWORD detThDFSDn;          // delay line for RSSI < ThDFSDn
    
    wiUReg RSSI0,   RSSI1;   // calibrated RSSI values
    wiUReg msrpwrA, msrpwrB; // recorded measured power (antenna selection)
    wiUReg abspwrA, abspwrB; // recorded absolute power (antenna selection)
    wiUReg refpwr;           // recorded packet reference power (StepUp/StepDown)
    wiUReg ValidRefPwr;      // refpwr is valid for current packet
    
    wiBitDelayLine wPeak;// signal in w0R, w0I, w1R, or w1I at peak amplitude

} Verona_AGC_t;

typedef struct
{
    wiUReg  EnB; // 0:enable, 1:disable        
    wiUReg  State;  
    wiUReg  absSqr;
    wiReg   inR, inI;
    wiReg   preAcmR, preAcmI;  
    wiReg   acmR, acmI;  
    wiReg   acmAveR, acmAveI;
    wiUReg  aveCounter;
    wiUReg  delay;

    wiUWORD pwr;

} Verona_ImageDetector_t;

typedef struct //  ----------------- SigDet State ----------------------------------
{
    wiWORD mem0R[96], mem0I[96];  // delay line memory, path 0
    wiWORD mem1R[96], mem1I[96];  // delay line memory, path 1
    wiReg  AcmAxR, AcmAxI;        // autocorrelation accumulator
    wiUReg  AcmPwr;               // power measurement accumulator
    wiWORD  w0R[3], w0I[3];       // input registers, path 0
    wiWORD  w1R[3], w1I[3];       // input registers, path 1
    wiUReg SigDet;                // signal detect
    wiUReg State;                 // controller state
    wiUReg EnB;                   // block enable
    wiUReg PathSel;               // data path select
    int    A0, A1, A2, A3, A4;    // address counters

} Verona_SignalDetect_t;

typedef struct //  ----------------- DigitalFrontEnd State -------------------------
{
    // gains
    wiUWORD LNAGain;    // LNA gain {0->low gain, 1->high gain} b0->LNAGain, b1->LNAGain2
    wiUWORD AGain;      // VGA gain

    // state machine
    wiUReg  State;      // controller state
    wiUReg  counter;    // counter used for state machine
    wiUReg  SigDetCount;// counter for elapsed time from a signal detect event
    wiUReg  RSSITimer;  // down counter for period when RSSI is invalid

    // control signals
    wiUWORD SigDetValid;// flag indicating frame sync has validated signal detection
    wiUWORD SigDet;     // temporary flag for FlagDS
    wiUWORD LgSigDFE;   // large signal observed at the digital amp output
    wiUWORD AGCdone;    // AGC is done
    wiUWORD PeakFound;  // peak found (frame sync)
    wiUWORD PeakSearch; // enabling peak search
    wiUWORD StartCFO;   // start CFO coarse estimation
    wiUWORD ArmLgSigDet;// arm AFE large signal detector

    wiUReg  LgSigDetAFE;// AFE LgSigDet
    wiUReg  oldLNAGain; // register previous LNAGain
    wiUReg  oldAGain;   // register previous AGain
    wiUWORD UpdateGains;// AGC should update gains
    wiUWORD ForceLinear;// AGC should update gain assuming linear power measurement

    wiUWORD InitGains;  // AGC initialize gains
    wiUWORD AGCEnB, DAEnB, SDEnB, FSEnB, CFOEnB; // module enable signals

    int GainUpdateCount;
    wiUWORD Antenna;

    wiUWORD UseRecordPwr;
    wiUWORD RecordPwr;
    wiUWORD RecordRefPwr;
    wiUWORD AntSel;
    wiUWORD AntSearch;
    wiUWORD RSSI_A, RSSI_B; // for 'C' model only (not in RTL)
    wiUWORD AntSuggest;

} Verona_DigitalFrontEnd_t;

typedef struct //  ----------------- I/Q Calibration State -------------------------
{
    // registers
    wiUReg calEn;          // calibration enable
    wiUReg state;          // state variable
    wiUReg gainLUTIndx;    // LUT index for both a11LUT[] and a22LUT[]
    wiUReg phaseLUTIndx;   // LUT index for a12LUT[] 
    wiUReg iterationCount; // number of fine-stepsize loop
    wiUReg neighborCount;  // number of neighbor visited
    wiUReg delayCount;     // delay from previous detector done till next detector enable 
    wiUReg pwr;            // image power of last valid point 
    wiUReg skipImgDet;     // 1:skip image detect; 0 otherwise
    wiUReg solutionFound;  // 1:calibration process done; 0 otherwise 
    wiUReg typeSel;        // 0:calibrating a11/a22, 1:calibrating a12
    wiReg  adjustVector;   // signed adjustment for gain/phase LUT indices
    wiReg  a11Value;       // a11 LUT output
    wiReg  a22Value;       // a22 LUT output
    wiReg  a12Value;       // a12 LUT output

    // state machine input signals
    wiBoolean validPoint;  
    wiBoolean ImgDetDone;
    wiUWORD   ImgDetPwr;

    // state machine output signals 
    wiBoolean recordLUTIndx;      
    wiBoolean recordImgPwr;       
    wiBoolean clearNeighborCount; 
    wiBoolean dirChange;          
    wiBoolean imgDetEnable;
    wiBoolean incIteCount;
    wiBoolean incNeighborCount;
    wiBoolean loadCoarseStep;
    wiBoolean loadFineStep;

} Verona_IQCalibration_t;

//======================================================================================
//  RECEIVER PARAMETER STRUCTURE
//======================================================================================
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
    wiUWORD     *PHY_RX_D;           // decoded received data bytes
    unsigned int N_PHY_RX_WR;        // number of PHY_RX_D writes

    wiBoolean    MAC_NOFILTER;       // State of the MAC_NOFILTER signal

    wiBoolean    RX_nMode;           // indicate the received packet mode;
    wiUWORD      RX_RATE;            // indicate the rate of received packet; Put in the RX structure for convenience; 
    wiUWORD      RX_LENGTH;          // indicate the length of received packet; Put in the RX structure for convenience;
	wiUWORD      L_LENGTH;           // L_LENGTH field from L-SIG field [ replaces traceRxLLength ]
    wiBoolean    Greenfield;         // indicate whether it is GreenField. 1: GreenField, 0: Mixed Mode;   
    wiBoolean    CheckRotate;        // enable the sum-of-magnitude program;
    wiBoolean    Rotated6Mbps;       // indicates rotated BPSK constellation in RX2;   
    wiBoolean    Rotated6MbpsValid;  // indicate the  Rotated6Mbps signal is valid in RX2
	wiBoolean    RxRot6Mbps;         // indicates rotated BPSK constellation to RX top state;
    wiBoolean	 RxRot6MbpsValid;    // indicate the  Rotated6Mbps signal is valid to RX top state
    wiBoolean    Unsupported;        // unsupported case
    wiBoolean    Aggregation;        // aggregated packet as indicated in PLCP header
    wiBoolean    ReservedHTSig;      // Indication of reserved HT-SIG;
	wiBoolean    LegacyPacket;       // Set to 1 if L-SIG indicates rate other than 6; Set to 1 if Rotated6Mbps is 0 after Rotation Check; for PHY_RX_EN generation
	wiBoolean    HtHeaderValid;      // Set in HTSigDecode. Used to update CtrlState HeaderValid at appropriate time during state 22 to keep 22-23 timing match RTL
	wiBoolean    HtRxFault;          // Set in HTSigDecode. Used to update CtrlState RxFault at appropriate time during state 22 to keep 22-31 timing as before to match RTL    
    struct
    {
        wiBoolean Enabled;            // Enable non-RTL processing
        wiBoolean DisableCFO;         // disable CFO correction
        wiBoolean DisablePLL;         // disable PLL correction
        wiUInt    MaxSamplesCFO;      // maximum samples for coarse CFO estimate
        wiBoolean MojaveFrontCtrlEOP; // treat end-of-packet in FrontCtrl like Mojave
        enum
        {
            VERONA_GAIN_UPDATE_LINEAR      = 1,
            VERONA_GAIN_UPDATE_THSIGSMALL  = 2,
            VERONA_GAIN_UPDATE_THSMALLPEAK = 3,
            VERONA_GAIN_UPDATE_THSIGLARGE  = 4,
            VERONA_GAIN_UPDATE_LGSIGAFE    = 5
        } GainUpdateType[8];

    }  NonRTL;                        // struct NonRTL -- features not implemented in RTL    

    int N80, N40, N20, N22;           // number of samples at 80,40,20,22 MHz

    wiWORD  *pReal[2], *pImag[2];             // Digital amplifier output (pre-limter)
    wiUWORD *qReal[2], *qImag[2];             // ADC output samples
    wiWORD  *cReal[2], *cImag[2];             // DCX output samples
    wiWORD  *tReal[2], *tImag[2];             // I/Q balance corrector output samples
    wiWORD  *rReal[2], *rImag[2]; unsigned Nr;// Data select output samples
    wiWORD  *zReal[2], *zImag[2];             // Signal mixed to zero-IF (80 MS/s)
    wiWORD  *yReal[2], *yImag[2];             // Decimated low pass filter output
    wiWORD  *sReal[2], *sImag[2]; unsigned Ns;// Digital amplifier output (limited)
    wiWORD  *wReal[2], *wImag[2];             // Digital front end input (resolution reduced s)
    wiWORD  *xReal[2], *xImag[2]; unsigned Nx;// Baseband time-domain samples
    wiWORD  *vReal[2], *vImag[2]; unsigned Nv;// FFT output
    wiWORD  *uReal[2], *uImag[2]; unsigned Nu;// Phase corrector output
    wiWORD  *DReal,    *DImag;                // Hard decisions from EqD
    wiUWORD *Lp;            unsigned Np;      // ~LLR for the permuted coded bits; Added Np to accommodate 6 Mbps
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

    wiWORD  *cSP, *cCP; // PLL sampling and carrier phase correction (per symbol)
    wiInt   *kFFT;      // Index for FFT start (k20

    wiWORD   h2[2][64];                  // Frequency domain channel + noise power
    wiWORD   hReal[2][64], hImag[2][64]; // Frequency channel response (estimated)
      
    wiUWORD  NoiseShift[2];          // floor(log2(Noise Power)) for EqD
    wiUWORD  CFO;                    // estimated CFO from OFDM CFO estimator

    int      Fault;                  // {0=None, 1=SIGNAL, 2=Array overrun, 3=Missed SigDet} 
    int      x_ofs;                  // Offset in x array (estimated)
    int      DownmixPhase;           // phase for downmix oscillator (counter value)
    int      DownsamPhase;           // phase for downsampling from 80 MHz to 20 MHz
    wiUInt   RefPwrAGC;              // last recorded RefPwr from the AGC

    wiWORD   RxIQ0a11;               // Rx I/Q correction coefficient a11 (path 0)
    wiWORD   RxIQ0a22;               // Rx I/Q correction coefficient a22 (path 0)
    wiWORD   RxIQ0a12;               // Rx I/Q correction coefficient a12 (path 0)
    wiWORD   RxIQ1a11;               // Rx I/Q correction coefficient a11 (path 1)
    wiWORD   RxIQ1a22;               // Rx I/Q correction coefficient a22 (path 1)
    wiWORD   RxIQ1a12;               // Rx I/Q correction coefficient a12 (path 1)

    wiWORD dRxIQ0a11, dRxIQ0a22, dRxIQ0a12; // registers for calibration engine output
    wiWORD dRxIQ1a11, dRxIQ1a22, dRxIQ1a12;

    Verona_RxClockState_t clock;     // clock generator state

    struct {
        int       nEff;              // Number of effective ADC Bits
        unsigned  Mask;              // Mask corresponding to ADC_nEff;
        int       Offset;            // Output offset (LSBs)
        double    Vpp;               // Full scale Vpp
        double    c;                 // Scale (2^10 / Vpp-max)
        wiBoolean Rounding;          // Enable rounding during quantization

        wiComplex rA[6], rB[6];      // Circular buffer to emulate pipeline delay
        int i;                       // Circular buffer index
    
    } ADC;

    struct {
        wiReg  Out0R,Out0I,Out1R,Out1I; // output registers
    } DSel;

    struct {
        wiReg z[4];                  // filter delay element (2 paths each with I and Q)
    } DCX;

    wiBitDelayLine NuisanceFilterDataValid;

    struct {
        wiUReg RxEndTimer;
        wiBoolean ShortTime; // used as a register
    } RX_ERROR;

    struct {
        wiBoolean ImgDetEnB;
        wiUReg    ImgDetDone;  
        wiUReg    ImgDetPwr;
    } RX1;

    struct
    {
        unsigned int *DB_u, *DB_l; // array of decision bits (upper, lower 32 of 64 bits)
        int Nbits;
    } VitDec;

    Verona_Downmix_t                 Downmix;
    Verona_DownsamFIR_t              DownsamFIR;
    Verona_ImageDetector_t           ImageDetector;
    Verona_IQCalibration_t           IQCalibration;
    Verona_CFO_t                     CFOState;
    Verona_FrameSync_t               FrameSync;
    wiReg                            DigitalAmpOut[4];
    Verona_AGC_t                     AGC;
    Verona_SignalDetect_t            SignalDetect;
    Verona_DigitalFrontEnd_t         DigitalFrontEnd;
    Verona_WeightedPilotPhaseTrack_t WeightedPilotPhaseTrack;
    Verona_PLL_t                     PLL;
    Verona_EqD_t                     EqD;
    Verona_RX2_t                     RX2;
    Verona_RX_CCA_t                  CCA;
    Verona_RxTop_t                   RxTop;

    int dx, du, dv, dc, dp;          // Array point offsets (used in RX2). dp is the offset for Lp.  
    wiWORD cCPP;                     // DPLL correction terms for carrier phase from pilot tone PLL
    wiWORD dAdvDly, d2AdvDly;        // One and two-symbol delayed version of AdvDly
    int PacketType;                  // Type of last packet with valid header; (0=none, 1=OFDM, 2=DSSS/CCK)
    wiUWORD aPacketDuration;         // OFDM Packet length (microseconds) for all OFDM format
   
    struct {                         // TX phase: simulate I/O and top-level control during TX event
        wiBoolean    Enabled;        // enable TX timing model during RX
        wiBoolean    Usek2Delay;     // enable k2 = kTxDone + k1
        unsigned int k1[4];          // k80 index for rising  edge of MAC_TX_EN
        unsigned int k2[4];          // k80 index for falling edge of MAC_TX_EN
        unsigned int kTxDone[4];     // k80 index for TxDone strobe
        unsigned int k2Delay;        // delay from TxDone to falling edge of MAC_TX_EN
        unsigned int kTxFault;       // time to assert TxFault
    } TX;

    struct {
        unsigned int Total;          // number of received packets
        unsigned int ValidFCS;       // number with valid FCS
        unsigned int BadFCS;         // number with bad FCS
        unsigned int FrameCheck;     // shift register with 1 indicating the position of a frame
        unsigned int FrameValid;     // shift register with 1 indication a frame with valid FCS
    } FrameCount;

    struct {
        unsigned int kMarker[5];     // Marker pulse times
        unsigned int kStart[9];      // Comparison window start times
        unsigned int kEnd[9];        // Comparison window end time
    } Trace;                         // traceTx configuration structure

    struct {
		wiBoolean    Enabled;        // enable Forced Nuisance filtering for testing of OFDM Nuisance packet
		unsigned int kNuisance;      // k80 index to generate forced NuisancePacket signal
	} Nuisance;

	wiUWORD  EventFlag;              // Event flags to verification purposes (legacy)
 									 // b0  = Missed signal detect
									 // b1  = RX_LENGTH = 0
									 // b2  = Parity/CRC error in SIGNAL symbol  
									 // b3  = Unrecognized RX_RATE
									 // b4  = DFE detected false signal detect
									 // b5  = LNA gain reduced
									 // b6  = AdvDly other than 0 during packet
									 // b7  = carrier phase error exceeds +/- pi
									 // b8  = Received data set to short...buffer overrrun
									 // b9  = <unused>
									 // b10 = Viterbi metric normalization exercised
									 // b11 = Multiple minimum metrics in Viterbi decoder
									 // b12 = EqD output value(s) to PLL limited
									 // b13 = x_ofs artificially limited
									 // b14-16 = # of gain updates

    Verona_RTL_ExceptionFlags_t RTL_Exception;  // exception event flags used for RTL verification  

    Verona_aSigState_t         *aSigOut;        // trace "aSigState" register output
    Verona_bSigState_t         *bSigOut;        // trace "bSigState" register output
    Verona_traceRxClockType    *traceRxClock;   // trace RX clocks in top-level controller
    Verona_traceRxStateType    *traceRxState;   // trace RX top state
    Verona_traceRxControlType  *traceRxControl; // trace top-level RX control
    Verona_traceDFEType        *traceDFE;       // trace OFDM Digital Front End
    Verona_traceAGCType        *traceAGC;       // trace signals from OFDM AGC
    Verona_traceRSSIType       *traceRSSI;      // trace power measurements from OFDM AGC/RSSI
    Verona_traceDFSType        *traceDFS;       // trace DFS
    Verona_traceSigDetType     *traceSigDet;    // trace OFDM signal detect
    Verona_traceFrameSyncType  *traceFrameSync; // trace OFDM frame synchronization
    Verona_RadioDigitalIO_t    *traceRadioIO;   // trace radio digital interface   
    Verona_DataConvCtrlState_t *traceDataConv;  // trace data converter control interface
    Verona_traceRX2Type        *traceRX2;       // trace for OFDM RX2 demodulation/decoding
    Verona_traceRxType         *traceRx;        // trace for general purpose signals/markers
    Verona_traceImgDetType     *traceImgDet;    // trace for image detector
    Verona_traceIQCalType      *traceIQCal;     // trace for IQ calibration engine
    wiWORD  *traceH2[2];                        // trace channel estimation power
    wiWORD  *traceHReal[2], *traceHImag[2];     // trace channel estimation value the length
 
    wiBoolean EnableModelIO;         // Model interface I/O precisely
    wiBoolean EnableTrace;           // Enable signal trace for greater observability
    wiBoolean EnableIQCal;          // Enable image detector and calibration engine

}  Verona_RxState_t;

//=============================================================================
//  DSSS/CCK RECEIVER SIGNALS
//=============================================================================
typedef union // ----------------- Trace CCK Baseband Interface -------------
{
    unsigned int word;
    wiBitField32Struct;
    struct
    {
        unsigned int bRXEnB     :1; //< 0> input to Verona_b_RX_top
        unsigned int Run11b     :1; //< 1> input to Verona_b_RX_top
        unsigned int LgSigAFE   :1; //< 2> input to Verona_b_RX_top
        unsigned int bRXDone    :1; //< 3> output from Verona_b_RX_top
        unsigned int AntSel     :1; //< 4> output from Verona_b_RX_top
        unsigned int __unused5 : 1; //< 5>
        unsigned int CSCCK      :1; //< 6> output from Verona_b_RX_top
        unsigned int Restart    :1; //< 7> input to Verona_b_RX_top
        unsigned int RSSICCK    :8; //<8:15> output from Verona_b_RX_top
        unsigned int LNAGain    :1; //<16> output from Verona_b_RX_top
        unsigned int AGain      :6; //<17:22> output from Verona_b_RX_top
        signed   int DGain      :5; //<23:27> output from AGC
        unsigned int LNAGain2   :1; //<28> output from Verona_b_RX_top
    };
} Verona_b_traceCCKType;

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
} Verona_b_traceStateType;

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
        unsigned int CFOeGet    :1; //<12>
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
} Verona_b_traceControlType;

typedef union // ----------------- Trace bVerona RX Interpolator Signals ----
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
} Verona_b_traceResampleType;

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
} Verona_b_traceDPLLType;

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
} Verona_b_traceBarkerType;

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
} Verona_b_traceDSSSType;

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
} Verona_b_traceCCKFBFType;

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
} Verona_b_traceCCKTVMFType;

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
} Verona_b_traceCCKFWTType;

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
    };
} Verona_b_FrontCtrlRegType;


typedef struct
{
    wiUWORD m;  // counter for RAM write address (40 MHz sample index)

    wiUWORD   b44n;   // integer part    of 11/32 "buffer" phase counter
    wiUWORD   b44p;   // fractional part of 11/32 "buffer" phase counter

    wiUWORD   k44n;   // integer part    of 11/32 phase counter for free-running 44 MHz clock
    wiUWORD   k44p;   // fractional part of 11/32 phase counter for free-running 44 MHz clock

    wiUReg n;      // integer part of sampling error
    wiUReg p;      // fractional part of sampling error [0, 1)*N

    wiReg  NCO;   // net NCO
    wiReg  dLPFOutR,  dLPFOutI;  // input register (1T delayed)
    wiReg  d2LPFOutR, d2LPFOutI; // input register (2T delayed)
    wiReg  yR, yI;               // output registers

    wiBitDelayLine dTransCCK;    // TransCCK signal delay line
    wiBitDelayLine dIntpFreeze;  // FIFO Underflow delay line (for Interpolator Freeze)

    struct VERONA_RAM_128x32_S { // 128x32, 2-port RAM
        int  xReal:8;   // with data word divided into four 8-bit Bytes
        int dxReal:8;
        int  xImag:8;
        int dxImag:8;
    } RAM[128];

} Verona_bRX_Interpolate_t;

typedef struct
{
    wiUReg  RegBCEnB;         // BCEnB register
    wiReg   rReal, rImag;     // correlator output register
    wiWORD  dR[25], dI[25];   // input delay line
    wiUWORD counter;          // sample counter at 22 MHz
    wiBoolean TFF;

} Verona_bBarkerCorrelator_t;

typedef struct
{
    wiUReg dEDEnB;
    wiReg  zReal, zImag;
    wiUReg acc;
    wiUReg count;
    wiUReg EDdone;
    wiUReg FirstMeasure;

} Verona_bEnergyDetect_t;

typedef struct
{
    wiReg   dBCOutR, dBCOutI;
    wiUReg  RdAdr, WrAdr, WrEnB, RAMInit, CQClr, OutEnB, state;
    wiUWORD cqRAM[32]; // RAM - only 22 of 32 addresses used

} Verona_bCorrelationQuality_t;

typedef struct
{
    wiUReg    GainLimit;   // flag for maximum attenuation
    wiReg     XGain;       // internal AGain+DGain setting
    wiReg     negGain;     // negative gain used to adjust measured energy
    wiBoolean dAGCEnB;     // delayed version of AGCEnB
    wiUWORD   UpdateCount; // gain update counter (4-bit)

} Verona_bAGC_t;

typedef struct
{
    wiWORD BCOutR_RAM[22], BCOutI_RAM[22]; // RAM - delay line for BCOut
    wiReg AcmAxR, AcmAxI;                  // autocorrelator accumulator
    wiReg ratio;                            // ratio used to calculate angle
    wiUInt addr;                           // RAM address
    int count;                               // number of input samples

} Verona_bEstimateCFO_t;

typedef struct
{
    wiBitDelayLine dTransCCK;
    wiReg xR, xI, yR, yI, dcCP;
    unsigned count;
    wiWORD q;

} Verona_bCorrectCFO_t;

typedef struct
{
    wiUWORD phase ;                 // modulated symbol phase

    wiWORD  dBCOutR[32],dBCOutI[32];// Barker correlator samples (RAM)
    wiUWORD WrAdr, RdAdr;           // address for dBCOut RAM

    wiUWORD dDMOut[4];   // buffer for demodulator output (symbol decisions, 2b wide)
    wiUWORD dMod[4];     // buffer for modulation type
    wiUInt  SymBufCount; // number of symbols in the buffers
    wiUInt  NSym;        // number of symbols summed in correlation accumulators
    wiUInt  count;       // counter for the number of buffered BCOut symbols

    wiWORD aReal[22], aImag[22]; // correlator sum after phase correction
    wiWORD hReal[22], hImag[22]; // channel estimate after correction filtering

    int CNTDM;         // count symbol decisions
    int CNTdelay;      // delay count
    int CNTstart;      // start counting
    int save_Nh, save_Np, save_Np2;

} Verona_bChanEst_t;


typedef struct
{
    wiUInt wrptr, rdptr;       // RAM write/read address pointers
    wiBitDelayLine dTransCCK;  // TransCCK signal delay line (11 MHz)

} Verona_bRX_SP_t;

typedef struct
{
     wiBoolean dCMFEnB;     // delayed CMFEnB signal
     wiWORD dR[23], dI[23]; // filter delay line
     wiWORD hR[22], hI[22]; // channel response
     int Nhs;               // channel length
     wiWORD zReal[21], zImag[21]; // delay line for CMF outputs

} Verona_bRX_CMF_t;

typedef struct
{
    int NSymRake;               // number of symbols processed by RAKE receiver
    int NSymHDR;               // number of header symbols processed
    wiWORD  pp1Real, pp1Imag;
    wiUWORD pp2Real, pp2Imag;

    wiReg CBCOutR, CBCOutI;
    wiBoolean CBCEnB, RBCEnB;
    wiUReg state;

} Verona_bRX_DSSS_t;

typedef struct
{
    wiUWORD ICI[64];    // ICI terms (shared for 5.5/11 Mbps)
    wiWORD   (*ccReal)[8],(*ccImag)[8];   // pointer to complementary code table
    wiUWORD NSym; // symbol counter (really only need LSB for odd/even symbol)
    wiBoolean ppReal, ppImag;

} Verona_bRX_CCK_t;

typedef struct
{
    wiUWORD NSym;    // number of symbols examined
    wiUWORD d;       // shift register delay line

} Verona_bRX_SFD_t;

typedef struct
{
    int Nbit; // data bit counter

} Verona_bRX_Header_t;

typedef struct
{
    wiUWORD BitCount;

} Verona_bRX_PSDU_t;

typedef struct
{
    wiWORD cV; // vco for carrier
    wiWORD cI; // integral path memory for carrier

    wiWORD sV; // vco for sample timing
    wiWORD sI; // integral path memory for sampling

} Verona_bRX_DPLL_t;

typedef struct
{
    wiUWORD S;

} Verona_bRX_Descrambler_t;

typedef struct
{
    wiReg  d44R, d44I;
    wiReg  d22R[2], d22I[2];
    wiReg  d11R, d11I;
    wiUReg s22, s11; // controller states for 22/11 MHz outputs, clocked at 44/22 MHz
    wiUReg Deci22Freeze;

} Verona_b_Downsample_t;

typedef struct
{
    // variables for state machine
    wiUReg state;       // state of the digital front end control
    wiUReg count;       // counter at 22MHz clock
    wiUReg SFDcount;    // timeout counter for SFD search
    wiUReg SymCount;    // counter for DSSS symbol boundaries (1us period)
    wiUReg SymCountCCK; // counter for CCK symbol boundaries (8/11 * 1us period)
    wiUReg LengthCount; // counter for PSDU length (1us period) with state 36

    // variables for AGC
    wiUWORD AbsEnergy;    // absolute energy used for antenna switching
    wiUWORD AbsEnergyOld;// previous absolute energy used for antenna switching

    // variables for channel estimation
    wiBoolean CEDone;    // indicator that channel estimation is finished

    // variables for PSDU decoding
    wiBoolean cckInit;    // flag for per-packet initialization of CCK demodulator
    wiBoolean IntpFreeze;

    // variables for PLL
    wiWORD cCP;         // carrier phase error correction
    wiWORD CCKWalOutR,    CCKWalOutI;    // correlator output
    wiWORD CCKMatOutR[8], CCKMatOutI[8]; // CCK-TVMF outputs
    wiWORD *SSMatOutR,    *SSMatOutI;

    // variables for symbol decoding
    wiWORD BFReal[22],BFImag[22];   // feedback filter input

    wiBoolean INTERPinit;      // intialization flag for interpolator
    wiBoolean ResetState;  // state reset for restart

    wiWORD *ccReal, *ccImag;

    // memories for implementation delays
    wiBoolean dRestart;

    wiBoolean pCCACS, pCCAED;
    wiBoolean SFDCntEnB;

    // Explicit Registers
    wiUReg LNAGain;              // radio (b0) and external (b1) LNA gain output in AGC
    wiUReg AGain;                // radio SGA gain output in AGC
    wiReg  DGain;                // digital gain output in AGC
    wiUReg EDOut;                // energy detector output
    wiUReg AbsPwr;               // absolute power (from AGC)
    wiUReg CQPeak;               // peak position for CQOut
    wiUReg CQOut;                // output from CorrelationQuality block
    wiUReg LargeSignal;          // flag large signal AGC update
    wiUReg GainUpdate;           // flag AGC update
    wiReg  cSP;                  // sampling phase error correction
    wiReg  CFO;                  // CFO estimate
    wiUReg SSOut, SSOutValid;    // DSSS demodulator output
    wiUReg cckWalOutValid;       // CCK correlator output valid (cycle accurate)
    wiUReg cckOut, cckOutValid;  // CCK demodulator output (not exactly cycle accurate)
    wiUReg DesOut, DesOutValid;  // descrambler output
    wiReg  SSCorOutR, SSCorOutI; // DSSS rake receiver Barker correlator outputs
    wiUReg SSCorOutValid;        // indicate valid output on SSCorOutR/I
    wiUReg SSSliOutR, SSSliOutI; // DSSS demod quantized constellation coordinates
    wiUReg SFDDone, Preamble;
    wiUReg LgSigDetAFE;          // AFE LgSigDet

    Verona_b_FrontCtrlRegType inReg, outReg; // packed register I/O
    wiUReg Phase44, DeciFreeze;

    wiBitDelayLine DMSym;       // demodulate symbol trigger
    wiBitDelayLine cckSym;      // demodulate symbol trigger
    wiBitDelayLine PSDUDone;
    wiBitDelayLine HDRDone;
    wiBitDelayLine posedge_cckWalOutValid; // used to align DPLL with demodulator
    wiBitDelayLine posedge_SSCorOutValid;  // used to align DPLL with demodulator
    wiBitDelayLine dDeciFreeze;            // deciFreeze delayed on 11 MHz clock 

    wiBitDelayLine DelayEn;  // for PHY_RX_EN model
	wiBitDelayLine Delay2En; 

    unsigned CQWinCount, EDWinCount, SSWinCount, SFDWinCount;

} Verona_bRxTop_t;

//=============================================================================
//  DSSS/CCK RECEIVER PARAMETER STRUCTURE
//=============================================================================
typedef struct
{
    int     N_PHY_RX_WR;           // Number of PHY_RX_D writes
    wiUWORD PHY_RX_D[4104];        // Decoded received data

    wiUWORD   *a;      int da;     // descrambled SFD, PLCL header, PSDU
    wiUWORD   *b;      int db;     // demodulated/decoded header, PSDU bits
    int       Na,Nb;               // used to prevent memory access violation

    wiWORD    dRxFilter[2][16];    // delay line for Verona_bRX_Filter

    wiWORD    hReal[22],hImag[22]; // current channel estimate
    int       Nh;                  // current channel length
    int       Np;                  // length of precursor used for RAKE receiver
    int       Np2;                 // length of precursor used for CCK demodulation

    wiWORD    hcReal[22],hcImag[22]; // coarse channel estimate
    int       Nhc;                   // coarse channel length
    int       Npc;                   // #precursor of coarse estimate for RAKE
    int       Np2c;                  // #precrsors of coarse estimate for CCK

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

    wiUWORD  *EDOut;              // energy detect output
    wiUWORD  *CQOut;              // correlation quality output
    wiUWORD   CQPeak;             // last value of CQPeak

    Verona_bRX_Interpolate_t     Interpolate; // state for RX interpolator
    Verona_bBarkerCorrelator_t   BarkerCorrelator;
    Verona_bEnergyDetect_t       EnergyDetect;
    Verona_bCorrelationQuality_t CorrelationQuality;
    Verona_bAGC_t                AGC;
    Verona_bEstimateCFO_t        EstimateCFO;
    Verona_bCorrectCFO_t         CorrectCFO;
    Verona_bChanEst_t            ChanEst;
    Verona_bRX_SP_t              RX_SP;
    Verona_bRX_CMF_t             CMF;
    Verona_bRX_DSSS_t            RX_DSSS;
    Verona_bRX_CCK_t             RX_CCK;
    Verona_bRX_SFD_t             RX_SFD;
    Verona_bRX_Header_t          RX_Header;
    Verona_bRX_PSDU_t            RX_PSDU;
    Verona_bRX_DPLL_t            RX_DPLL;
    Verona_bRX_Descrambler_t     Descrambler;
    Verona_b_Downsample_t        Downsample;
    Verona_bRxTop_t              RxTop;

    Verona_b_traceCCKType      *traceCCK;     // trace signals from CCK interface
    Verona_b_traceStateType    *traceState;   // trace state from CCK controller
    Verona_b_traceControlType  *traceControl; // trace signals from CCK controller
    Verona_b_FrontCtrlRegType  *traceFrontReg;// trace "outReg" from Verona_b_RX_top
    Verona_b_traceDPLLType     *traceDPLL;    // trace signals from DPLL
    Verona_b_traceBarkerType   *traceBarker;  // trace signals from Barker correlator
    Verona_b_traceDSSSType     *traceDSSS;    // trace signals from DSSS demodulator
    Verona_b_traceCCKFBFType   *traceCCKInput;// trace CCK input/DFE output
    Verona_b_traceCCKTVMFType  *traceCCKTVMF; // trace CCK TVMF output
    Verona_b_traceCCKFWTType   *traceCCKFWT;  // trace CCK FWT/metric output
    Verona_b_traceResampleType *traceResample;// trace bRX Resample NCOs
    unsigned int               *k80;          // 80 MHz clock tick at trace event

}  Verona_bRxState_t;

//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================

wiStatus Verona_Init(void);
wiStatus Verona_b_Init(void);
wiStatus Verona_Close(void);

wiStatus Verona_InitializeThread(unsigned int ThreadIndex);
wiStatus Verona_CloseThread     (unsigned int ThreadIndex);

wiStatus Verona_GetRegisterMap(wiUWORD **pRegMap);
wiStatus Verona_SetRegisterMap(wiUInt pRegData[], unsigned int NumRegs);
wiStatus Verona_WriteRegisterMap(wiUWORD *RegMap);

wiStatus Verona_RESET(void);
wiStatus Verona_WriteConfig(wiMessageFn MsgFunc);
wiStatus Verona_ParseLine(wiString text);
wiUInt   Verona_ReadRegister (wiUInt Addr);
wiStatus Verona_WriteRegister(wiUInt Addr, wiUInt Data);

wiStatus Verona_InvertArrayBit(wiString Name, unsigned int n, unsigned int k, unsigned int m);
wiStatus Verona_SetArrayWord  (wiString Name, unsigned int n, unsigned int k, unsigned int x);

Verona_TxState_t*     Verona_TxState(void);
Verona_RxState_t*     Verona_RxState(void);
Verona_DvState_t*     Verona_DvState(void);
Verona_bRxState_t*    Verona_bRxState(void);
VeronaRegisters_t*    Verona_Registers(void);
Verona_LookupTable_t* Verona_LookupTable(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Transmit functions in Verona_TX.c
//
wiStatus Verona_TX_Init(void);
wiStatus Verona_TX_Restart(void);
wiStatus Verona_TX(int Length, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M);
wiReal   Verona_Prms(void);

void     Verona_DAC       (int N, wiUWORD uR[], wiUWORD uI[], wiComplex s[], int M);
void     Verona_Upmix     (int N, wiWORD UpmixInR[], wiWORD UpmixInI[], wiWORD UpmixOutR[], wiWORD UpmixOutI[]);
void     Verona_TxIQSource(int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI);
void     Verona_TxIQ      (int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI);
void     Verona_Prd       (int N, wiWORD PrdInR[], wiWORD PrdInI[], wiUWORD PrdOutR[], wiUWORD PrdOutI[]);

wiStatus Verona_TX_FreeMemory(Verona_TxState_t *pTX, wiBoolean ClearPointerOnly);
wiStatus Verona_TX_AllocateMemory(int N_SYM, int N_BPSC, int N_DBPS, int M, wiBoolean nMode, wiBoolean Greenfield);

void Verona_Interleaver( wiUWORD InRATE, wiUWORD IntlvIn[], wiUWORD IntlvOut[], int IntlvTxRx, 
                         wiBoolean nMode, wiBoolean Rotated6Mbps );

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_TX_DPD.c
//
void     Verona_TX_DPD(int N, wiWORD xR[], wiWORD xI[], wiWORD *yR, wiWORD *yI);
wiStatus Verona_TX_DPD_Init(void);
wiStatus Verona_TX_DPD_Close(void);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_RX.c
//
wiStatus Verona_RX( wiBoolean DW_PHYEnB, Verona_RadioDigitalIO_t *Radio, 
                    Verona_DataConvCtrlState_t *DataConvIn, Verona_DataConvCtrlState_t DataConvOut, 
                    Verona_CtrlState_t *inReg, Verona_CtrlState_t outReg );
                   
wiStatus Verona_RX_Init(void);
wiStatus Verona_RX_Restart(void);
wiStatus Verona_RX_FreeMemory(Verona_RxState_t *pRX, wiBoolean ClearPointerOnly);
wiStatus Verona_RX_AllocateMemory(void);
wiStatus Verona_RX_EndOfWaveform(void);

void Verona_RX_ClockGenerator(Verona_RxState_t *pRX, wiBoolean Reset);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_RX1.c
//
wiStatus Verona_RX1( wiBoolean DFEEnB, wiBoolean LgSigAFE, wiUWORD *LNAGainOFDM, wiUWORD *AGainOFDM, 
                     wiUWORD *RSSI0OFDM, wiUWORD *RSSI1OFDM, wiBoolean CSCCK,
                     wiBoolean bPathMux, wiReg *bInR, wiReg *bInI, 
                     wiBoolean RestartRX, wiBoolean DFERestart, wiBoolean CheckRIFS, wiBoolean HoldGain,
                     wiBoolean UpdateAGC, wiBoolean ClearDCX, wiBoolean SquelchRX,
                     Verona_aSigState_t *aSigIn, Verona_aSigState_t aSigOut, 
                     Verona_RxState_t *pRX, VeronaRegisters_t *pReg );

wiStatus Verona_RX1_Restart(void);

void Verona_ADC( Verona_DataConvCtrlState_t SigIn, Verona_DataConvCtrlState_t *SigOut, 
                 Verona_RxState_t *pRX, wiComplex ADCAIQ, wiComplex ADCBIQ, 
                 wiUWORD *ADCOut1I, wiUWORD *ADCOut1Q, wiUWORD *ADCOut2I, wiUWORD *ADCOut2Q );

void Verona_DCX( int n, wiWORD DCXIn, wiWORD *DCXOut, wiBoolean DCXClear, Verona_RxState_t *pRX, 
                 VeronaRegisters_t *pReg );

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_RX2.c
//
wiStatus Verona_RX2( Verona_aSigState_t *aSigIn, Verona_aSigState_t aSigOut, Verona_CtrlState_t outReg,
                     Verona_RxState_t *pRX, VeronaRegisters_t *pReg );

void Verona_DFT_pilot(wiWORD xR[], wiWORD xI[], wiWORD *pR, wiWORD *pI);

void Verona_PhaseCorrect( wiUWORD PathSel, int Nk, wiWORD cSP, wiWORD cCP, wiWORD cCPP, Verona_RxState_t *pRX,
                          wiWORD *v0R, wiWORD *v0I, wiWORD *v1R, wiWORD *v1I,
                          wiWORD *u0R, wiWORD *u0I, wiWORD *u1R, wiWORD *u1I );

void Verona_PLL( wiWORD I1[], wiWORD Q1[], wiWORD H2[], wiWORD DI[], wiWORD DQ[], int iFlag, int HoldVCO, int UpdateH,
                 wiWORD *AdvDly, wiWORD *_cSP, wiWORD *_cCP, Verona_RxState_t *pRX, VeronaRegisters_t *pReg );

void Verona_WeightedPilotPhaseTrack( int FInit, int HoldVCO, wiUWORD PathSel, wiWORD v0R[], wiWORD v0I[],
                                     wiWORD v1R[], wiWORD v1I[], wiWORD h0R[], wiWORD h0I[],
                                     wiWORD h1R[], wiWORD h1I[], wiWORD cSP, wiWORD AdvDly, wiWORD *cCPP, 
                                     Verona_RxState_t *pRX, VeronaRegisters_t *pReg, wiBoolean nMode, int i );

wiStatus Verona_VitDec(int Nbits, wiUWORD VitDetLLR0[], wiUWORD VitDetLLR1[], wiUWORD VitDetOut[], Verona_RxState_t *pRX);

wiStatus Verona_RX2_DATA_Demod (unsigned int n,  Verona_RxState_t *pRX, VeronaRegisters_t *pReg);
wiStatus Verona_RX2_DATA_Decode(wiBoolean nMode, Verona_RxState_t *pRX, VeronaRegisters_t *pReg);

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_DFS.c
//
void Verona_CheckFCS(wiUInt nBytes, wiUWORD X[], wiUInt *FCSPass);

void Verona_DFS( wiBoolean DFSEnB, Verona_aSigState_t SigIn, Verona_aSigState_t *SigOut, 
                 Verona_RxState_t *pRX, VeronaRegisters_t *pReg );

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_DigitalFrontEnd.c
//
void Verona_DigitalFrontEnd( wiBoolean DFEEnB, wiBoolean DFERestart, wiBoolean CheckRIFS, wiBoolean HoldGain,
                             wiBoolean UpdateAGC, wiBoolean DFELgSigAFE,
                             wiWORD  DFEIn0R,  wiWORD  DFEIn0I,  wiWORD  DFEIn1R,  wiWORD  DFEIn1I,
                             wiWORD *DFEOut0R, wiWORD *DFEOut0I, wiWORD *DFEOut1R, wiWORD *DFEOut1I,
                             wiUWORD *DFELNAGain, wiUWORD *DFEAGain, wiUWORD *DFERSSI0, wiUWORD *DFERSSI1,
                             Verona_RxState_t *pRX, VeronaRegisters_t *pReg, wiBoolean CSCCK, 
                             wiBoolean RestartRX, Verona_aSigState_t *aSigOut, Verona_aSigState_t aSigIn );

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_FFT.c
//
void Verona_FFT( int TxBRx, wiWORD FFTTxDIR[], wiWORD FFTTxDII[], wiWORD FFTRxDIR[], wiWORD FFTRxDII[],
                 wiWORD *FFTTxDOR, wiWORD *FFTTxDOI, wiWORD *FFTRxDOR, wiWORD *FFTRxDOI );

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Functions in Verona_b.c
//
wiStatus Verona_bRX_AllocateMemory(void);
wiStatus Verona_bRX_FreeMemory(Verona_bRxState_t *pRX, wiBoolean ClearPointerOnly);
wiStatus Verona_bTX_AllocateMemory(int Na, int Nc, int M);

wiStatus Verona_bTX( wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M );
wiStatus Verona_bTX_ResampleToDAC(void);

void Verona_bRX_top( Verona_RxState_t *pRX, Verona_bRxState_t *RX, VeronaRegisters_t *pReg, wiBoolean bRXEnB, 
                     wiWORD  *qReal, wiWORD  *qImag, wiUWORD *AGain, wiUWORD *LNAGain, wiBoolean LgSigAFE, 
                     wiUWORD *RSSICCK, wiBoolean Run11b, wiBoolean Restart,
                     Verona_bSigState_t *bSigOut, Verona_bSigState_t bSigIn );

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

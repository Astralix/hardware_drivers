//--< Mojave.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  Mojave - Baseband Model
//           
//  Developed by Barrett Brickner, Younggyun Kim, and Ross Ewert
//  Copyright 2001-2003 Bermai, 2005-2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "wiUtil.h"
#include "wiParse.h"
#include "Mojave.h"
#include "Mojave_DigitalFrontEnd.h"

#define aMOJAVE_FFT_OFFSET       2 /* delay from peak found to start of FFT (RX control)    */
#define aMOJAVE_FFT_DELAY       82 /* processing delay for the (I)FFT)                      */
#define aMOJAVE_PLL_DELAY        0 /* delay due to the PLL phase corrector                  */
#define aMOJAVE_SIGNAL_LATENCY  70 /* latency from SIGNAL FFT start to decoding output      */

#if defined(MOJAVE_VERSION_1)
    #pragma message(" Building Selected Model: Mojave Version 1 (PartID = 5, Original)")
#endif
#if defined(MOJAVE_VERSION_1B)
    #pragma message(" Building Selected Model: Mojave Version 1b (PartID = 6, Metal Spin)")
#endif


//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean Verbose        = 0;
static Mojave_TX_State TX       = {0};  // transmitter state
static Mojave_RX_State RX       = {0};  // receiver state
static bMojave_RX_State *bRX    = NULL; // pointer to "b" baseband RX state (assign with RX_Restart)
static MojaveRegisters Register = {0};  // baseband configuration registers
static wiStatus _status         = 0;    // return code
static wiUWORD   RegMap[256]    = {0};  // register map (Tx/Rx combined)

// subcarrier types {0=null, 1=data, 2=pilot}
static int subcarrier_type[64] = {  0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1,   //  0-15
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,   // 16-31
                                    0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,   // 32-47
                                    1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 }; // 48-63

//=================================================================================================
//--  CODE PUNCTURING TABLES
//=================================================================================================
static int pA_12[] = {1};                  // Rate 1/2...Puncturer is rate 1/1
static int pB_12[] = {1};

static int pA_23[] = {1,1,1,1,1,1};        // Rate 2/3...Puncturer is rate 3/4
static int pB_23[] = {1,0,1,0,1,0};

static int pA_34[] = {1,1,0,1,1,0,1,1,0};  // Rate 3/4...Puncturer is rate 2/3
static int pB_34[] = {1,0,1,1,0,1,1,0,1};

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if(STATUS(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if((_status=WICHK(arg))<=0) return _status;

#define REG_CLR(reg)  (reg.D.word = reg.Q.word = 0)  /* clear register */
#define ClockRegister(arg) {(arg).Q=(arg).D;};
#define ClockDelayLine(SR,value) {(SR).word = ((SR).word << 1) | ((value)&1);};

#ifndef min
#define min(a,b) (((a)<(b))? (a):(b))
#endif

// ================================================================================================
// FUNCTION  : Mojave_RX_ClockGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate clocks for the baseband receiver. Internally, the module operates
//             an 880 MHz clock and produces {11,22,44,20,40,80} MHz divided clocks. 
//
//             When called, the master clock is run until at least one positive edge 
//             occurs on one of the generated clocks. The generated clocks are used to 
//             time cycle-accurate modules. Clock edge indicators are stored in the
//             Mojave RX state.
//
// Parameters: Reset - Reset and synchronize clocks (active high)
// ================================================================================================
void Mojave_RX_ClockGenerator(wiBoolean Reset)
{
    static unsigned int clock40, clock44;
    static wiUWORD clockGate40 = {0xAAAAA}; // 80->40 MHz clock gating cadence
    static wiUWORD clockGate44 = {0xAAB55}; // 80->44 MHz clock gating cadence

    // -----------
    // reset clock
    // -----------
    if(Reset)
    {
        unsigned int k;
        RX.clock.posedgeCLK80MHz = 1;
        clockGate40.word = 0xAAAAA;
        clockGate44.word = 0xAAB55;
        clock40 = clock44 = 3;
        for(k=0; k<RX.clock.InitialPhase; k++) // advance clock generator specified periods
            Mojave_RX_ClockGenerator(0);

        RX.k80 = RX.k40 = RX.k20 = RX.k44 = RX.k22 = RX.k11 = 0;
    }
    RX.clock.posedgeCLK40MHz = clockGate40.b19;
    RX.clock.posedgeCLK44MHz = clockGate44.b19;

    clockGate40.w20 = (clockGate40.w19 << 1) | clockGate40.b19;
    clockGate44.w20 = (clockGate44.w19 << 1) | clockGate44.b19;

    if(RX.clock.posedgeCLK40MHz) clock40++;
    if(RX.clock.posedgeCLK44MHz) clock44++;

    // -----------------------------------------------
    // indicate a positive edge for the divided clocks
    // -----------------------------------------------
    RX.clock.posedgeCLK20MHz = RX.clock.posedgeCLK40MHz & (clock40%2? 0:1);
    RX.clock.posedgeCLK22MHz = RX.clock.posedgeCLK44MHz & (clock44%2? 0:1);
    RX.clock.posedgeCLK11MHz = RX.clock.posedgeCLK22MHz & (clock44%4? 0:1);

    // -------------------------
    // Increment sample counters
    // -------------------------
    if(RX.clock.posedgeCLK80MHz) RX.k80++;
    if(RX.clock.posedgeCLK40MHz) RX.k40++;
    if(RX.clock.posedgeCLK20MHz) RX.k20++;
    if(RX.clock.posedgeCLK44MHz) RX.k44++;
    if(RX.clock.posedgeCLK22MHz) RX.k22++;
    if(RX.clock.posedgeCLK11MHz) RX.k11++;
}
// end of Mojave_RX_ClockGenerator()

// ================================================================================================
// FUNCTION  : Mojave_Prms()
// ------------------------------------------------------------------------------------------------
// Purpose   : Returns the transmit signal scaling
// Parameters: none
// Returns   : rms transmit power at DAC output
// ================================================================================================
wiReal Mojave_Prms(void)
{
    double Xout = 128.3495/512.0;    // RMS LSBs at DAC input
    double Pout = Xout * Xout * 2.0; // Power into 1 Ohm (both DACs - complex)
    double ADAC = (TX.DAC.Vpp/2.0);  // Digital-to-Analog scaling (Digital 1=0 to Full Scale)
    Pout = Pout * (ADAC * ADAC);
    return Pout;
}
// end of Mojave_Prms()

// ================================================================================================
// FUNCTION  : Mojave_ReadRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Read TX/Register parameters from a register map
// Parameters: RegMap -- pointer to the register map from which to read parameters
// ================================================================================================
wiStatus Mojave_ReadRegisterMap(wiUWORD *RegMap)
{
    int i;

    if(Verbose) wiPrintf(" Mojave_ReadRegisterMap(@x%08X)\n",RegMap);

    Register.ID.word               = wiUWORD_GetUnsignedField(RegMap+0x01, 7,0);
    Register.PathSel.word          = wiUWORD_GetUnsignedField(RegMap+0x03, 1,0);
    Register.RXMode.word           = wiUWORD_GetUnsignedField(RegMap+0x04, 1,0);
    Register.UpmixConj.word        = wiUWORD_GetUnsignedField(RegMap+0x06, 7,7);
    Register.DownmixConj.word      = wiUWORD_GetUnsignedField(RegMap+0x06, 6,6);
    Register.DmixEnDis.word        = wiUWORD_GetUnsignedField(RegMap+0x06, 5,5);
    Register.UpmixDown.word        = wiUWORD_GetUnsignedField(RegMap+0x06, 3,3);
    Register.DownmixUp.word        = wiUWORD_GetUnsignedField(RegMap+0x06, 2,2);
    Register.UpmixOff.word         = wiUWORD_GetUnsignedField(RegMap+0x06, 1,1);
    Register.DownmixOff.word       = wiUWORD_GetUnsignedField(RegMap+0x06, 0,0);
    Register.TestMode.word         = wiUWORD_GetUnsignedField(RegMap+0x07, 7,4);
    Register.PrdDACSrcR.word       = wiUWORD_GetUnsignedField(RegMap+0x07, 3,2);
    Register.PrdDACSrcI.word       = wiUWORD_GetUnsignedField(RegMap+0x07, 1,0);
    Register.TxExtend.word         = wiUWORD_GetUnsignedField(RegMap+0x08, 7,6);
    Register.TxDelay.word          = wiUWORD_GetUnsignedField(RegMap+0x08, 5,0);
    Register.TX_RATE.word          = wiUWORD_GetUnsignedField(RegMap+0x09, 7,4);
    Register.TX_PWRLVL.word        = wiUWORD_GetUnsignedField(RegMap+0x0B, 7,0);
    Register.TxFault.word          = wiUWORD_GetUnsignedField(RegMap+0x0E, 7,7);
    Register.TX_PRBS.word          = wiUWORD_GetUnsignedField(RegMap+0x0E, 6,0);
    Register.RxFault.word          = wiUWORD_GetUnsignedField(RegMap+0x11, 0,0);
    Register.RX_RATE.word          = wiUWORD_GetUnsignedField(RegMap+0x12, 7,4);
    Register.RSSI0.word            = wiUWORD_GetUnsignedField(RegMap+0x16, 7,0);
    Register.RSSI1.word            = wiUWORD_GetUnsignedField(RegMap+0x17, 7,0);
    Register.maxLENGTH.word        = wiUWORD_GetUnsignedField(RegMap+0x18, 7,0);
    Register.Disable72.word        = wiUWORD_GetUnsignedField(RegMap+0x19, 1,1);
    Register.Reserved0.word        = wiUWORD_GetUnsignedField(RegMap+0x19, 0,0);
    Register.PilotPLLEn.word       = wiUWORD_GetUnsignedField(RegMap+0x1E, 6,6);
    Register.aC.word               = wiUWORD_GetUnsignedField(RegMap+0x1E, 5,3);
    Register.bC.word               = wiUWORD_GetUnsignedField(RegMap+0x1E, 2,0);
    Register.cC.word               = wiUWORD_GetUnsignedField(RegMap+0x1F, 2,0);
    // --------------------------------------------------------------------
    for(i=0; i<16; i++) {
        Register.Sa_Sb[i].word  = wiUWORD_GetUnsignedField(RegMap+0x20+i,5,0);
        Register.Ca_Cb[i].word  = wiUWORD_GetUnsignedField(RegMap+0x30+i,5,0);
    }
    // --------------------------------------------------------------------
    Register.RefPwrDig.word        = wiUWORD_GetUnsignedField(RegMap+0x40, 6,0);
    Register.InitAGain.word        = wiUWORD_GetUnsignedField(RegMap+0x41, 5,0);
    Register.AbsPwrL.word          = wiUWORD_GetUnsignedField(RegMap+0x42, 6,0);
    Register.AbsPwrH.word          = wiUWORD_GetUnsignedField(RegMap+0x43, 6,0);
    Register.ThSigLarge.word       = wiUWORD_GetUnsignedField(RegMap+0x44, 6,0);
    Register.ThSigSmall.word       = wiUWORD_GetUnsignedField(RegMap+0x45, 6,0);
    Register.StepLgSigDet.word     = wiUWORD_GetUnsignedField(RegMap+0x46, 4,0);
    Register.StepLarge.word        = wiUWORD_GetUnsignedField(RegMap+0x47, 4,0);
    Register.StepSmall.word        = wiUWORD_GetUnsignedField(RegMap+0x48, 4,0);
    Register.ThSwitchLNA.word      = wiUWORD_GetUnsignedField(RegMap+0x49, 5,0);
    Register.StepLNA.word          = wiUWORD_GetUnsignedField(RegMap+0x4A, 4,0);
    Register.ThSwitchLNA2.word     = wiUWORD_GetUnsignedField(RegMap+0x4B, 5,0);
    Register.StepLNA2.word         = wiUWORD_GetUnsignedField(RegMap+0x4C, 4,0);
    Register.Pwr100dBm.word        = wiUWORD_GetUnsignedField(RegMap+0x4D, 4,0);
    Register.ThCCA1.word           = wiUWORD_GetUnsignedField(RegMap+0x4E, 5,0);
    Register.ThCCA2.word           = wiUWORD_GetUnsignedField(RegMap+0x4F, 5,0);
    Register.FixedGain.word        = wiUWORD_GetUnsignedField(RegMap+0x50, 7,7);
    Register.FixedLNAGain.word     = wiUWORD_GetUnsignedField(RegMap+0x50, 6,6);
    Register.FixedLNA2Gain.word    = wiUWORD_GetUnsignedField(RegMap+0x50, 5,5);
    Register.UpdateOnLNA.word      = wiUWORD_GetUnsignedField(RegMap+0x50, 4,4);
    Register.MaxUpdateCount.word   = wiUWORD_GetUnsignedField(RegMap+0x50, 2,0);
    Register.ThStepUpRefPwr.word   = wiUWORD_GetUnsignedField(RegMap+0x51, 7,0);
    Register.ThStepUp.word         = wiUWORD_GetUnsignedField(RegMap+0x52, 7,4);
    Register.ThStepDown.word       = wiUWORD_GetUnsignedField(RegMap+0x52, 3,0);
    Register.ThDFSUp.word          = wiUWORD_GetUnsignedField(RegMap+0x53, 6,0);
    Register.ThDFSDn.word          = wiUWORD_GetUnsignedField(RegMap+0x54, 6,0);
    Register.DFSOn.word            = wiUWORD_GetUnsignedField(RegMap+0x57, 7,7);
    Register.DFSCCA.word           = wiUWORD_GetUnsignedField(RegMap+0x57, 6,6);
    Register.DFS2Candidates.word   = wiUWORD_GetUnsignedField(RegMap+0x57, 5,5);
    Register.DFSSmooth.word        = wiUWORD_GetUnsignedField(RegMap+0x57, 4,4);
    Register.DFSMACFilter.word     = wiUWORD_GetUnsignedField(RegMap+0x57, 3,3);
    Register.DFSHdrFilter.word     = wiUWORD_GetUnsignedField(RegMap+0x57, 2,2);
    Register.DFSIntDet.word        = wiUWORD_GetUnsignedField(RegMap+0x57, 0,0);
    Register.DFSPattern.word       = wiUWORD_GetUnsignedField(RegMap+0x58, 5,4);
    Register.DFSminPPB.word        = wiUWORD_GetUnsignedField(RegMap+0x58, 2,0);
    Register.DFSdTPRF.word         = wiUWORD_GetUnsignedField(RegMap+0x59, 3,0);
    Register.DFSmaxWidth.word      = wiUWORD_GetUnsignedField(RegMap+0x5A, 7,0);
    Register.DFSmaxTPRF.word       = wiUWORD_GetUnsignedField(RegMap+0x5B, 7,0);
    Register.DFSminTPRF.word       = wiUWORD_GetUnsignedField(RegMap+0x5C, 7,0);
    Register.DFSminGap.word        = wiUWORD_GetUnsignedField(RegMap+0x5D, 6,0);
    Register.DFSPulseSR.word       = wiUWORD_GetUnsignedField(RegMap+0x5E, 7,0);
    Register.detRadar.word         = wiUWORD_GetUnsignedField(RegMap+0x5F, 7,7);
    Register.detHeader.word        = wiUWORD_GetUnsignedField(RegMap+0x5F, 3,3);
    Register.detPreamble.word      = wiUWORD_GetUnsignedField(RegMap+0x5F, 2,2);
    Register.detStepDown.word      = wiUWORD_GetUnsignedField(RegMap+0x5F, 1,1);
    Register.detStepUp.word        = wiUWORD_GetUnsignedField(RegMap+0x5F, 0,0);
    Register.WaitAGC.word          = wiUWORD_GetUnsignedField(RegMap+0x60, 6,0);
    Register.DelayAGC.word         = wiUWORD_GetUnsignedField(RegMap+0x61, 4,0);
    Register.SigDetWindow.word     = wiUWORD_GetUnsignedField(RegMap+0x62, 7,0);
    Register.SyncShift.word        = wiUWORD_GetUnsignedField(RegMap+0x63, 3,0);
    Register.DelayRestart.word     = wiUWORD_GetUnsignedField(RegMap+0x64, 7,0);
    Register.OFDMSwDEn.word        = wiUWORD_GetUnsignedField(RegMap+0x65, 7,7);
    Register.OFDMSwDSave.word      = wiUWORD_GetUnsignedField(RegMap+0x65, 6,6);
    Register.OFDMSwDInit.word      = wiUWORD_GetUnsignedField(RegMap+0x65, 5,5);
    Register.OFDMSwDDelay.word     = wiUWORD_GetUnsignedField(RegMap+0x65, 4,0);
    Register.OFDMSwDTh.word        = wiUWORD_GetUnsignedField(RegMap+0x66, 7,0);
    Register.CFOFilter.word        = wiUWORD_GetUnsignedField(RegMap+0x67, 7,7);
    Register.ArmLgSigDet3.word     = wiUWORD_GetUnsignedField(RegMap+0x67, 5,5);
    Register.LgSigAFEValid.word    = wiUWORD_GetUnsignedField(RegMap+0x67, 4,4);
    Register.IdleFSEnB.word        = wiUWORD_GetUnsignedField(RegMap+0x67, 3,3);
    Register.IdleSDEnB.word        = wiUWORD_GetUnsignedField(RegMap+0x67, 2,2);
    Register.IdleDAEnB.word        = wiUWORD_GetUnsignedField(RegMap+0x67, 1,1);
    Register.DelayCFO.word         = wiUWORD_GetUnsignedField(RegMap+0x68, 4,0);
    Register.DelayRSSI.word        = wiUWORD_GetUnsignedField(RegMap+0x69, 5,0);
    Register.SigDetTh1.word        = wiUWORD_GetUnsignedField(RegMap+0x70, 5,0);
    Register.SigDetTh2H.word       = wiUWORD_GetUnsignedField(RegMap+0x71, 7,0);
    Register.SigDetTh2L.word       = wiUWORD_GetUnsignedField(RegMap+0x72, 7,0);
    Register.SigDetDelay.word      = wiUWORD_GetUnsignedField(RegMap+0x73, 7,0);
    Register.SigDetFilter.word     = wiUWORD_GetUnsignedField(RegMap+0x74, 7,7);
    Register.SyncFilter.word       = wiUWORD_GetUnsignedField(RegMap+0x78, 7,7);
    Register.SyncMag.word          = wiUWORD_GetUnsignedField(RegMap+0x78, 4,0);
    Register.SyncThSig.word        = wiUWORD_GetUnsignedField(RegMap+0x79, 7,4);
    Register.SyncThExp.word        = wiUWORD_GetSignedField  (RegMap+0x79, 3,0);
    Register.SyncDelay.word        = wiUWORD_GetUnsignedField(RegMap+0x7A, 4,0);
    Register.MinShift.word         = wiUWORD_GetUnsignedField(RegMap+0x80, 3,0);
    Register.ChEstShift0.word      = wiUWORD_GetUnsignedField(RegMap+0x81, 7,4);
    Register.ChEstShift1.word      = wiUWORD_GetUnsignedField(RegMap+0x81, 3,0);
    Register.aC1.word              = wiUWORD_GetUnsignedField(RegMap+0x84, 5,3);
    Register.bC1.word              = wiUWORD_GetUnsignedField(RegMap+0x84, 2,0);
    Register.aS1.word              = wiUWORD_GetUnsignedField(RegMap+0x85, 5,3);
    Register.bS1.word              = wiUWORD_GetUnsignedField(RegMap+0x85, 2,0);
    Register.aC2.word              = wiUWORD_GetUnsignedField(RegMap+0x86, 5,3);
    Register.bC2.word              = wiUWORD_GetUnsignedField(RegMap+0x86, 2,0);
    Register.aS2.word              = wiUWORD_GetUnsignedField(RegMap+0x87, 5,3);
    Register.bS2.word              = wiUWORD_GetUnsignedField(RegMap+0x87, 2,0);
    Register.DCXSelect.word        = wiUWORD_GetUnsignedField(RegMap+0x90, 7,7);
    Register.DCXFastLoad.word      = wiUWORD_GetUnsignedField(RegMap+0x90, 6,6);
    Register.DCXHoldAcc.word       = wiUWORD_GetUnsignedField(RegMap+0x90, 5,5);
    Register.DCXShift.word         = wiUWORD_GetUnsignedField(RegMap+0x90, 2,0);
    Register.DCXAcc0RH.word        = wiUWORD_GetSignedField  (RegMap+0x91, 4,0);
    Register.DCXAcc0RL.word        = wiUWORD_GetUnsignedField(RegMap+0x92, 7,0);
    Register.DCXAcc0IH.word        = wiUWORD_GetSignedField  (RegMap+0x93, 4,0);
    Register.DCXAcc0IL.word        = wiUWORD_GetUnsignedField(RegMap+0x94, 7,0);
    Register.DCXAcc1RH.word        = wiUWORD_GetSignedField  (RegMap+0x95, 4,0);
    Register.DCXAcc1RL.word        = wiUWORD_GetUnsignedField(RegMap+0x96, 7,0);
    Register.DCXAcc1IH.word        = wiUWORD_GetSignedField  (RegMap+0x97, 4,0);
    Register.DCXAcc1IL.word        = wiUWORD_GetUnsignedField(RegMap+0x98, 7,0);
    Register.INITdelay.word        = wiUWORD_GetUnsignedField(RegMap+0xA0, 7,0);
    Register.EDwindow.word         = wiUWORD_GetUnsignedField(RegMap+0xA1, 7,6);
    Register.CQwindow.word         = wiUWORD_GetUnsignedField(RegMap+0xA1, 5,4);
    Register.SSwindow.word         = wiUWORD_GetUnsignedField(RegMap+0xA1, 2,0);
    Register.CQthreshold.word      = wiUWORD_GetUnsignedField(RegMap+0xA2, 5,0);
    Register.EDthreshold.word      = wiUWORD_GetUnsignedField(RegMap+0xA3, 5,0);
    Register.bWaitAGC.word         = wiUWORD_GetUnsignedField(RegMap+0xA4, 5,0);
    Register.bAGCdelay.word        = wiUWORD_GetUnsignedField(RegMap+0xA5, 5,0);
    Register.bRefPwr.word          = wiUWORD_GetUnsignedField(RegMap+0xA6, 5,0);
    Register.bInitAGain.word       = wiUWORD_GetUnsignedField(RegMap+0xA7, 5,0);
    Register.bThSigLarge.word      = wiUWORD_GetUnsignedField(RegMap+0xA8, 5,0);
    Register.bStepLarge.word       = wiUWORD_GetUnsignedField(RegMap+0xA9, 4,0);
    Register.bThSwitchLNA.word     = wiUWORD_GetUnsignedField(RegMap+0xAA, 5,0);
    Register.bStepLNA.word         = wiUWORD_GetUnsignedField(RegMap+0xAB, 4,0);
    Register.bThSwitchLNA2.word    = wiUWORD_GetUnsignedField(RegMap+0xAC, 5,0);
    Register.bStepLNA2.word        = wiUWORD_GetUnsignedField(RegMap+0xAD, 4,0);
    Register.bPwr100dBm.word       = wiUWORD_GetUnsignedField(RegMap+0xAE, 4,0);
    Register.DAmpGainRange.word    = wiUWORD_GetUnsignedField(RegMap+0xAF, 3,0);
    Register.bMaxUpdateCount.word  = wiUWORD_GetUnsignedField(RegMap+0xB0, 3,0);
    Register.CSMode.word           = wiUWORD_GetUnsignedField(RegMap+0xB1, 5,4);
    Register.bPwrStepDet.word      = wiUWORD_GetUnsignedField(RegMap+0xB1, 3,3);
    Register.bCFOQual.word         = wiUWORD_GetUnsignedField(RegMap+0xB1, 2,2);
    Register.bAGCBounded4.word     = wiUWORD_GetUnsignedField(RegMap+0xB1, 1,1);
    Register.bLgSigAFEValid.word   = wiUWORD_GetUnsignedField(RegMap+0xB1, 0,0);
    Register.bSwDEn.word           = wiUWORD_GetUnsignedField(RegMap+0xB2, 7,7);
    Register.bSwDSave.word         = wiUWORD_GetUnsignedField(RegMap+0xB2, 6,6);
    Register.bSwDInit.word         = wiUWORD_GetUnsignedField(RegMap+0xB2, 5,5);
    Register.bSwDDelay.word        = wiUWORD_GetUnsignedField(RegMap+0xB2, 4,0);
    Register.bSwDTh.word           = wiUWORD_GetUnsignedField(RegMap+0xB3, 7,0);
    Register.bThStepUpRefPwr.word  = wiUWORD_GetUnsignedField(RegMap+0xB5, 7,0);
    Register.bThStepUp.word        = wiUWORD_GetUnsignedField(RegMap+0xB6, 7,4);
    Register.bThStepDown.word      = wiUWORD_GetUnsignedField(RegMap+0xB6, 3,0);
    Register.SymHDRCE.word         = wiUWORD_GetUnsignedField(RegMap+0xB8, 5,0);
    Register.CENSym1.word          = wiUWORD_GetUnsignedField(RegMap+0xB9, 6,4);
    Register.CENSym2.word          = wiUWORD_GetUnsignedField(RegMap+0xB9, 2,0);
    Register.hThreshold1.word      = wiUWORD_GetUnsignedField(RegMap+0xBA, 6,4);
    Register.hThreshold2.word      = wiUWORD_GetUnsignedField(RegMap+0xBA, 2,0);
    Register.hThreshold3.word      = wiUWORD_GetUnsignedField(RegMap+0xBB, 2,0);
    Register.SFDWindow.word        = wiUWORD_GetUnsignedField(RegMap+0xBC, 7,0);
    Register.AntSel.word           = wiUWORD_GetUnsignedField(RegMap+0xC0, 7,7);
    Register.CFO.word              = wiUWORD_GetUnsignedField(RegMap+0xC1, 7,0);
    Register.RxFaultCount.word     = wiUWORD_GetUnsignedField(RegMap+0xC2, 7,0);
    Register.RestartCount.word     = wiUWORD_GetUnsignedField(RegMap+0xC3, 7,0);
    Register.NuisanceCount.word    = wiUWORD_GetUnsignedField(RegMap+0xC4, 7,0);
    Register.RSSIdB.word           = wiUWORD_GetUnsignedField(RegMap+0xC5, 4,4);
    Register.ClrOnWake.word        = wiUWORD_GetUnsignedField(RegMap+0xC5, 2,2);
    Register.ClrOnHdr.word         = wiUWORD_GetUnsignedField(RegMap+0xC5, 1,1);
    Register.ClrNow.word           = wiUWORD_GetUnsignedField(RegMap+0xC5, 0,0);
    Register.Filter64QAM.word      = wiUWORD_GetUnsignedField(RegMap+0xC6, 7,7);
    Register.Filter16QAM.word      = wiUWORD_GetUnsignedField(RegMap+0xC6, 6,6);
    Register.FilterQPSK.word       = wiUWORD_GetUnsignedField(RegMap+0xC6, 5,5);
    Register.FilterBPSK.word       = wiUWORD_GetUnsignedField(RegMap+0xC6, 4,4);
    Register.FilterAllTypes.word   = wiUWORD_GetUnsignedField(RegMap+0xC6, 1,1);
    Register.AddrFilter.word       = wiUWORD_GetUnsignedField(RegMap+0xC6, 0,0);
    Register.STA0.word             = wiUWORD_GetUnsignedField(RegMap+0xC7, 7,0);
    Register.STA1.word             = wiUWORD_GetUnsignedField(RegMap+0xC8, 7,0);
    Register.STA2.word             = wiUWORD_GetUnsignedField(RegMap+0xC9, 7,0);
    Register.STA3.word             = wiUWORD_GetUnsignedField(RegMap+0xCA, 7,0);
    Register.STA4.word             = wiUWORD_GetUnsignedField(RegMap+0xCB, 7,0);
    Register.STA5.word             = wiUWORD_GetUnsignedField(RegMap+0xCC, 7,0);
    Register.minRSSI.word          = wiUWORD_GetUnsignedField(RegMap+0xCD, 7,0);
    Register.StepUpRestart.word    = wiUWORD_GetUnsignedField(RegMap+0xCE, 7,7);
    Register.StepDownRestart.word  = wiUWORD_GetUnsignedField(RegMap+0xCE, 6,6);
    Register.NoStepAfterSync.word  = wiUWORD_GetUnsignedField(RegMap+0xCE, 5,5);
    Register.NoStepAfterHdr.word   = wiUWORD_GetUnsignedField(RegMap+0xCE, 4,4);
    Register.KeepCCA_New.word      = wiUWORD_GetUnsignedField(RegMap+0xCE, 1,1);
    Register.KeepCCA_Lost.word     = wiUWORD_GetUnsignedField(RegMap+0xCE, 0,0);
    Register.WakeupTimeH.word      = wiUWORD_GetUnsignedField(RegMap+0xD0, 7,0);
    Register.WakeupTimeL.word      = wiUWORD_GetUnsignedField(RegMap+0xD1, 7,0);
    Register.DelayStdby.word       = wiUWORD_GetUnsignedField(RegMap+0xD2, 7,0);
    Register.DelayBGEnB.word       = wiUWORD_GetUnsignedField(RegMap+0xD3, 7,0);
    Register.DelayADC1.word        = wiUWORD_GetUnsignedField(RegMap+0xD4, 7,0);
    Register.DelayADC2.word        = wiUWORD_GetUnsignedField(RegMap+0xD5, 7,0);
    Register.DelayModem.word       = wiUWORD_GetUnsignedField(RegMap+0xD6, 7,0);
    Register.DelayDFE.word         = wiUWORD_GetUnsignedField(RegMap+0xD7, 7,0);
    Register.DelayRFSW.word        = wiUWORD_GetUnsignedField(RegMap+0xD8, 7,4);
    Register.TxRxTime.word         = wiUWORD_GetUnsignedField(RegMap+0xD8, 3,0);
    Register.DelayFCAL1.word       = wiUWORD_GetUnsignedField(RegMap+0xD9, 7,4);
    Register.DelayPA.word          = wiUWORD_GetUnsignedField(RegMap+0xD9, 3,0);
    Register.DelayFCAL2.word       = wiUWORD_GetUnsignedField(RegMap+0xDA, 7,4);
    Register.DelayState28.word     = wiUWORD_GetUnsignedField(RegMap+0xDA, 3,0);
    Register.DelayShutdown.word    = wiUWORD_GetUnsignedField(RegMap+0xDB, 7,4);
    Register.DelaySleep.word       = wiUWORD_GetUnsignedField(RegMap+0xDB, 3,0);
    Register.TimeExtend.word       = wiUWORD_GetUnsignedField(RegMap+0xDC, 7,0);
    Register.SquelchTime.word      = wiUWORD_GetUnsignedField(RegMap+0xDD, 5,0);
    Register.DelaySquelchRF.word   = wiUWORD_GetUnsignedField(RegMap+0xDE, 7,4);
    Register.DelayClearDCX.word    = wiUWORD_GetUnsignedField(RegMap+0xDE, 3,0);
    Register.SelectCCA.word        = wiUWORD_GetUnsignedField(RegMap+0xDF, 3,0);
    Register.RadioMC.word          = wiUWORD_GetUnsignedField(RegMap+0xE0, 7,0);
    Register.DACOPM.word           = wiUWORD_GetUnsignedField(RegMap+0xE1, 5,0);
    Register.ADCAOPM.word          = wiUWORD_GetUnsignedField(RegMap+0xE2, 5,0);
    Register.ADCBOPM.word          = wiUWORD_GetUnsignedField(RegMap+0xE3, 5,0);
    Register.RFSW1.word            = wiUWORD_GetUnsignedField(RegMap+0xE4, 7,0);
    Register.RFSW2.word            = wiUWORD_GetUnsignedField(RegMap+0xE5, 7,0);
    Register.LNAEnB.word           = wiUWORD_GetUnsignedField(RegMap+0xE6, 7,6);
    Register.PA24_MODE.word        = wiUWORD_GetUnsignedField(RegMap+0xE6, 3,2);
    Register.PA50_MODE.word        = wiUWORD_GetUnsignedField(RegMap+0xE6, 1,0);
    Register.LNA24A_MODE.word      = wiUWORD_GetUnsignedField(RegMap+0xE7, 7,6);
    Register.LNA24B_MODE.word      = wiUWORD_GetUnsignedField(RegMap+0xE7, 5,4);
    Register.LNA50A_MODE.word      = wiUWORD_GetUnsignedField(RegMap+0xE7, 3,2);
    Register.LNA50B_MODE.word      = wiUWORD_GetUnsignedField(RegMap+0xE7, 1,0);
    Register.ADCFSCTRL.word        = wiUWORD_GetUnsignedField(RegMap+0xE8, 7,6);
    Register.ADCOE.word            = wiUWORD_GetUnsignedField(RegMap+0xE8, 4,4);
    Register.LNAxxBOnSW2.word      = wiUWORD_GetUnsignedField(RegMap+0xE8, 3,3);
    Register.ADCOFCEN.word         = wiUWORD_GetUnsignedField(RegMap+0xE8, 2,2);
    Register.ADCDCREN.word         = wiUWORD_GetUnsignedField(RegMap+0xE8, 1,1);
    Register.ADCMUX0.word          = wiUWORD_GetUnsignedField(RegMap+0xE8, 0,0);
    Register.ADCARDY.word          = wiUWORD_GetUnsignedField(RegMap+0xE9, 7,7);
    Register.ADCBRDY.word          = wiUWORD_GetUnsignedField(RegMap+0xE9, 6,6);
    Register.ADCFCAL2.word         = wiUWORD_GetUnsignedField(RegMap+0xE9, 2,2);
    Register.ADCFCAL1.word         = wiUWORD_GetUnsignedField(RegMap+0xE9, 1,1);
    Register.ADCFCAL.word          = wiUWORD_GetUnsignedField(RegMap+0xE9, 0,0);
    Register.DACIFS_CTRL.word      = wiUWORD_GetUnsignedField(RegMap+0xEA, 2,0);
    Register.DACFG_CTRL0.word      = wiUWORD_GetUnsignedField(RegMap+0xEB, 7,0);
    Register.DACFG_CTRL1.word      = wiUWORD_GetUnsignedField(RegMap+0xEC, 7,0);
    Register.RESETB.word           = wiUWORD_GetUnsignedField(RegMap+0xED, 7,7);
    Register.SREdge.word           = wiUWORD_GetUnsignedField(RegMap+0xED, 2,2);
    Register.SRFreq.word           = wiUWORD_GetUnsignedField(RegMap+0xED, 1,0);
    // --------------------------------------------------------------------
    Register.TX_SERVICE.word       =(wiUWORD_GetUnsignedField(RegMap+0x0C, 7,0)<<8) 
                                  | wiUWORD_GetUnsignedField(RegMap+0x0D, 7,0);

    return WI_SUCCESS;
}
// end of Mojave_ReadRegisterMap()

// ================================================================================================
// FUNCTION  : Mojave_WriteRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write TX/Register parameters to a register map
// Parameters: RegMap -- pointer to the register map to which to parameters are written
// ================================================================================================
wiStatus Mojave_WriteRegisterMap(wiUWORD *RegMap)
{
    wiWORD Zero = {0};
    int i;

    if(Verbose) wiPrintf(" Mojave_WriteRegisterMap(@x%08X)\n",RegMap);

    Mojave_DCX(-2, Zero, NULL, 0, NULL); // Load values from DCX accumulators

    wiUWORD_SetUnsignedField(RegMap+0x01, 7,0, Register.ID.w8);
    wiUWORD_SetUnsignedField(RegMap+0x03, 1,0, Register.PathSel.w2);
    wiUWORD_SetUnsignedField(RegMap+0x04, 1,0, Register.RXMode.w2);
    wiUWORD_SetUnsignedField(RegMap+0x06, 7,7, Register.UpmixConj.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 6,6, Register.DownmixConj.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 5,5, Register.DmixEnDis.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 3,3, Register.UpmixDown.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 2,2, Register.DownmixUp.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 1,1, Register.UpmixOff.w1);
    wiUWORD_SetUnsignedField(RegMap+0x06, 0,0, Register.DownmixOff.w1);
    wiUWORD_SetUnsignedField(RegMap+0x07, 7,4, Register.TestMode.w4);
    wiUWORD_SetUnsignedField(RegMap+0x07, 3,2, Register.PrdDACSrcR.w2);
    wiUWORD_SetUnsignedField(RegMap+0x07, 1,0, Register.PrdDACSrcI.w2);
    wiUWORD_SetUnsignedField(RegMap+0x08, 7,6, Register.TxExtend.w2);
    wiUWORD_SetUnsignedField(RegMap+0x08, 5,0, Register.TxDelay.w6);
    wiUWORD_SetUnsignedField(RegMap+0x09, 7,4, Register.TX_RATE.w4);
    wiUWORD_SetUnsignedField(RegMap+0x0B, 7,0, Register.TX_PWRLVL.w8);
    wiUWORD_SetUnsignedField(RegMap+0x0E, 7,7, Register.TxFault.w1);
    wiUWORD_SetUnsignedField(RegMap+0x0E, 6,0, Register.TX_PRBS.w7);
    wiUWORD_SetUnsignedField(RegMap+0x11, 0,0, Register.RxFault.w1);
    wiUWORD_SetUnsignedField(RegMap+0x12, 7,4, Register.RX_RATE.w4);
    wiUWORD_SetUnsignedField(RegMap+0x16, 7,0, Register.RSSI0.w8);
    wiUWORD_SetUnsignedField(RegMap+0x17, 7,0, Register.RSSI1.w8);
    wiUWORD_SetUnsignedField(RegMap+0x18, 7,0, Register.maxLENGTH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x19, 1,1, Register.Disable72.w1);
    wiUWORD_SetUnsignedField(RegMap+0x19, 0,0, Register.Reserved0.w1);
    wiUWORD_SetUnsignedField(RegMap+0x1E, 6,6, Register.PilotPLLEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x1E, 5,3, Register.aC.w3);
    wiUWORD_SetUnsignedField(RegMap+0x1E, 2,0, Register.bC.w3);
    wiUWORD_SetUnsignedField(RegMap+0x1F, 2,0, Register.cC.w3);
    //---------------------------------------------------------------------
    for(i=0; i<16; i++) {
        wiUWORD_SetUnsignedField(RegMap+0x20+i, 5,0, Register.Sa_Sb[i].word);
        wiUWORD_SetUnsignedField(RegMap+0x30+i, 5,0, Register.Ca_Cb[i].word);
    }
    //---------------------------------------------------------------------
    wiUWORD_SetUnsignedField(RegMap+0x40, 6,0, Register.RefPwrDig.w7);
    wiUWORD_SetUnsignedField(RegMap+0x41, 5,0, Register.InitAGain.w6);
    wiUWORD_SetUnsignedField(RegMap+0x42, 6,0, Register.AbsPwrL.w7);
    wiUWORD_SetUnsignedField(RegMap+0x43, 6,0, Register.AbsPwrH.w7);
    wiUWORD_SetUnsignedField(RegMap+0x44, 6,0, Register.ThSigLarge.w7);
    wiUWORD_SetUnsignedField(RegMap+0x45, 6,0, Register.ThSigSmall.w7);
    wiUWORD_SetUnsignedField(RegMap+0x46, 4,0, Register.StepLgSigDet.w5);
    wiUWORD_SetUnsignedField(RegMap+0x47, 4,0, Register.StepLarge.w5);
    wiUWORD_SetUnsignedField(RegMap+0x48, 4,0, Register.StepSmall.w5);
    wiUWORD_SetUnsignedField(RegMap+0x49, 5,0, Register.ThSwitchLNA.w6);
    wiUWORD_SetUnsignedField(RegMap+0x4A, 4,0, Register.StepLNA.w5);
    wiUWORD_SetUnsignedField(RegMap+0x4B, 5,0, Register.ThSwitchLNA2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x4C, 4,0, Register.StepLNA2.w5);
    wiUWORD_SetUnsignedField(RegMap+0x4D, 4,0, Register.Pwr100dBm.w5);
    wiUWORD_SetUnsignedField(RegMap+0x4E, 5,0, Register.ThCCA1.w6);
    wiUWORD_SetUnsignedField(RegMap+0x4F, 5,0, Register.ThCCA2.w6);
    wiUWORD_SetUnsignedField(RegMap+0x50, 7,7, Register.FixedGain.w1);
    wiUWORD_SetUnsignedField(RegMap+0x50, 6,6, Register.FixedLNAGain.w1);
    wiUWORD_SetUnsignedField(RegMap+0x50, 5,5, Register.FixedLNA2Gain.w1);
    wiUWORD_SetUnsignedField(RegMap+0x50, 4,4, Register.UpdateOnLNA.w1);
    wiUWORD_SetUnsignedField(RegMap+0x50, 2,0, Register.MaxUpdateCount.w3);
    wiUWORD_SetUnsignedField(RegMap+0x51, 7,0, Register.ThStepUpRefPwr.w8);
    wiUWORD_SetUnsignedField(RegMap+0x52, 7,4, Register.ThStepUp.w4);
    wiUWORD_SetUnsignedField(RegMap+0x52, 3,0, Register.ThStepDown.w4);
    wiUWORD_SetUnsignedField(RegMap+0x53, 6,0, Register.ThDFSUp.w7);
    wiUWORD_SetUnsignedField(RegMap+0x54, 6,0, Register.ThDFSDn.w7);
    wiUWORD_SetUnsignedField(RegMap+0x57, 7,7, Register.DFSOn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 6,6, Register.DFSCCA.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 5,5, Register.DFS2Candidates.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 4,4, Register.DFSSmooth.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 3,3, Register.DFSMACFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 2,2, Register.DFSHdrFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x57, 0,0, Register.DFSIntDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0x58, 5,4, Register.DFSPattern.w2);
    wiUWORD_SetUnsignedField(RegMap+0x58, 2,0, Register.DFSminPPB.w3);
    wiUWORD_SetUnsignedField(RegMap+0x59, 3,0, Register.DFSdTPRF.w4);
    wiUWORD_SetUnsignedField(RegMap+0x5A, 7,0, Register.DFSmaxWidth.w8);
    wiUWORD_SetUnsignedField(RegMap+0x5B, 7,0, Register.DFSmaxTPRF.w8);
    wiUWORD_SetUnsignedField(RegMap+0x5C, 7,0, Register.DFSminTPRF.w8);
    wiUWORD_SetUnsignedField(RegMap+0x5D, 6,0, Register.DFSminGap.w7);
    wiUWORD_SetUnsignedField(RegMap+0x5E, 7,0, Register.DFSPulseSR.w8);
    wiUWORD_SetUnsignedField(RegMap+0x5F, 7,7, Register.detRadar.w1);
    wiUWORD_SetUnsignedField(RegMap+0x5F, 3,3, Register.detHeader.w1);
    wiUWORD_SetUnsignedField(RegMap+0x5F, 2,2, Register.detPreamble.w1);
    wiUWORD_SetUnsignedField(RegMap+0x5F, 1,1, Register.detStepDown.w1);
    wiUWORD_SetUnsignedField(RegMap+0x5F, 0,0, Register.detStepUp.w1);
    wiUWORD_SetUnsignedField(RegMap+0x60, 6,0, Register.WaitAGC.w7);
    wiUWORD_SetUnsignedField(RegMap+0x61, 4,0, Register.DelayAGC.w5);
    wiUWORD_SetUnsignedField(RegMap+0x62, 7,0, Register.SigDetWindow.w8);
    wiUWORD_SetUnsignedField(RegMap+0x63, 3,0, Register.SyncShift.w4);
    wiUWORD_SetUnsignedField(RegMap+0x64, 7,0, Register.DelayRestart.w8);
    wiUWORD_SetUnsignedField(RegMap+0x65, 7,7, Register.OFDMSwDEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0x65, 6,6, Register.OFDMSwDSave.w1);
    wiUWORD_SetUnsignedField(RegMap+0x65, 5,5, Register.OFDMSwDInit.w1);
    wiUWORD_SetUnsignedField(RegMap+0x65, 4,0, Register.OFDMSwDDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0x66, 7,0, Register.OFDMSwDTh.w8);
    wiUWORD_SetUnsignedField(RegMap+0x67, 7,7, Register.CFOFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x67, 5,5, Register.ArmLgSigDet3.w1);
    wiUWORD_SetUnsignedField(RegMap+0x67, 4,4, Register.LgSigAFEValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0x67, 3,3, Register.IdleFSEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x67, 2,2, Register.IdleSDEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x67, 1,1, Register.IdleDAEnB.w1);
    wiUWORD_SetUnsignedField(RegMap+0x68, 4,0, Register.DelayCFO.w5);
    wiUWORD_SetUnsignedField(RegMap+0x69, 5,0, Register.DelayRSSI.w6);
    wiUWORD_SetUnsignedField(RegMap+0x70, 5,0, Register.SigDetTh1.w6);
    wiUWORD_SetUnsignedField(RegMap+0x71, 7,0, Register.SigDetTh2H.w8);
    wiUWORD_SetUnsignedField(RegMap+0x72, 7,0, Register.SigDetTh2L.w8);
    wiUWORD_SetUnsignedField(RegMap+0x73, 7,0, Register.SigDetDelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0x74, 7,7, Register.SigDetFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x78, 7,7, Register.SyncFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0x78, 4,0, Register.SyncMag.w5);
    wiUWORD_SetUnsignedField(RegMap+0x79, 7,4, Register.SyncThSig.w4);
    wiUWORD_SetSignedField  (RegMap+0x79, 3,0, Register.SyncThExp.w4);
    wiUWORD_SetUnsignedField(RegMap+0x7A, 4,0, Register.SyncDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0x80, 3,0, Register.MinShift.w4);
    wiUWORD_SetUnsignedField(RegMap+0x81, 7,4, Register.ChEstShift0.w4);
    wiUWORD_SetUnsignedField(RegMap+0x81, 3,0, Register.ChEstShift1.w4);
    wiUWORD_SetUnsignedField(RegMap+0x84, 5,3, Register.aC1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x84, 2,0, Register.bC1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x85, 5,3, Register.aS1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x85, 2,0, Register.bS1.w3);
    wiUWORD_SetUnsignedField(RegMap+0x86, 5,3, Register.aC2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x86, 2,0, Register.bC2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x87, 5,3, Register.aS2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x87, 2,0, Register.bS2.w3);
    wiUWORD_SetUnsignedField(RegMap+0x90, 7,7, Register.DCXSelect.w1);
    wiUWORD_SetUnsignedField(RegMap+0x90, 6,6, Register.DCXFastLoad.w1);
    wiUWORD_SetUnsignedField(RegMap+0x90, 5,5, Register.DCXHoldAcc.w1);
    wiUWORD_SetUnsignedField(RegMap+0x90, 2,0, Register.DCXShift.w3);
    wiUWORD_SetSignedField  (RegMap+0x91, 4,0, Register.DCXAcc0RH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x92, 7,0, Register.DCXAcc0RL.w8);
    wiUWORD_SetSignedField  (RegMap+0x93, 4,0, Register.DCXAcc0IH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x94, 7,0, Register.DCXAcc0IL.w8);
    wiUWORD_SetSignedField  (RegMap+0x95, 4,0, Register.DCXAcc1RH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x96, 7,0, Register.DCXAcc1RL.w8);
    wiUWORD_SetSignedField  (RegMap+0x97, 4,0, Register.DCXAcc1IH.w5);
    wiUWORD_SetUnsignedField(RegMap+0x98, 7,0, Register.DCXAcc1IL.w8);
    wiUWORD_SetUnsignedField(RegMap+0xA0, 7,0, Register.INITdelay.w8);
    wiUWORD_SetUnsignedField(RegMap+0xA1, 7,6, Register.EDwindow.w2);
    wiUWORD_SetUnsignedField(RegMap+0xA1, 5,4, Register.CQwindow.w2);
    wiUWORD_SetUnsignedField(RegMap+0xA1, 2,0, Register.SSwindow.w3);
    wiUWORD_SetUnsignedField(RegMap+0xA2, 5,0, Register.CQthreshold.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA3, 5,0, Register.EDthreshold.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA4, 5,0, Register.bWaitAGC.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA5, 5,0, Register.bAGCdelay.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA6, 5,0, Register.bRefPwr.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA7, 5,0, Register.bInitAGain.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA8, 5,0, Register.bThSigLarge.w6);
    wiUWORD_SetUnsignedField(RegMap+0xA9, 4,0, Register.bStepLarge.w5);
    wiUWORD_SetUnsignedField(RegMap+0xAA, 5,0, Register.bThSwitchLNA.w6);
    wiUWORD_SetUnsignedField(RegMap+0xAB, 4,0, Register.bStepLNA.w5);
    wiUWORD_SetUnsignedField(RegMap+0xAC, 5,0, Register.bThSwitchLNA2.w6);
    wiUWORD_SetUnsignedField(RegMap+0xAD, 4,0, Register.bStepLNA2.w5);
    wiUWORD_SetUnsignedField(RegMap+0xAE, 4,0, Register.bPwr100dBm.w5);
    wiUWORD_SetUnsignedField(RegMap+0xAF, 3,0, Register.DAmpGainRange.w4);
    wiUWORD_SetUnsignedField(RegMap+0xB0, 3,0, Register.bMaxUpdateCount.w4);
    wiUWORD_SetUnsignedField(RegMap+0xB1, 5,4, Register.CSMode.w2);
    wiUWORD_SetUnsignedField(RegMap+0xB1, 3,3, Register.bPwrStepDet.w1);
    wiUWORD_SetUnsignedField(RegMap+0xBB, 2,2, Register.bCFOQual.b0);
    wiUWORD_SetUnsignedField(RegMap+0xBB, 1,1, Register.bAGCBounded4.b0);
    wiUWORD_SetUnsignedField(RegMap+0xB1, 0,0, Register.bLgSigAFEValid.w1);
    wiUWORD_SetUnsignedField(RegMap+0xB2, 7,7, Register.bSwDEn.w1);
    wiUWORD_SetUnsignedField(RegMap+0xB2, 6,6, Register.bSwDSave.w1);
    wiUWORD_SetUnsignedField(RegMap+0xB2, 5,5, Register.bSwDInit.w1);
    wiUWORD_SetUnsignedField(RegMap+0xB2, 4,0, Register.bSwDDelay.w5);
    wiUWORD_SetUnsignedField(RegMap+0xB3, 7,0, Register.bSwDTh.w8);
    wiUWORD_SetUnsignedField(RegMap+0xB5, 7,0, Register.bThStepUpRefPwr.w8);
    wiUWORD_SetUnsignedField(RegMap+0xB6, 7,4, Register.bThStepUp.w4);
    wiUWORD_SetUnsignedField(RegMap+0xB6, 3,0, Register.bThStepDown.w4);
    wiUWORD_SetUnsignedField(RegMap+0xB8, 5,0, Register.SymHDRCE.w6);
    wiUWORD_SetUnsignedField(RegMap+0xB9, 6,4, Register.CENSym1.w3);
    wiUWORD_SetUnsignedField(RegMap+0xB9, 2,0, Register.CENSym2.w3);
    wiUWORD_SetUnsignedField(RegMap+0xBA, 6,4, Register.hThreshold1.w3);
    wiUWORD_SetUnsignedField(RegMap+0xBA, 2,0, Register.hThreshold2.w3);
    wiUWORD_SetUnsignedField(RegMap+0xBB, 2,0, Register.hThreshold3.w3);
    wiUWORD_SetUnsignedField(RegMap+0xBC, 7,0, Register.SFDWindow.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC0, 7,7, Register.AntSel.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC1, 7,0, Register.CFO.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC2, 7,0, Register.RxFaultCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC3, 7,0, Register.RestartCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC4, 7,0, Register.NuisanceCount.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC5, 4,4, Register.RSSIdB.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC5, 2,2, Register.ClrOnWake.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC5, 1,1, Register.ClrOnHdr.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC5, 0,0, Register.ClrNow.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 7,7, Register.Filter64QAM.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 6,6, Register.Filter16QAM.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 5,5, Register.FilterQPSK.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 4,4, Register.FilterBPSK.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 1,1, Register.FilterAllTypes.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC6, 0,0, Register.AddrFilter.w1);
    wiUWORD_SetUnsignedField(RegMap+0xC7, 7,0, Register.STA0.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC8, 7,0, Register.STA1.w8);
    wiUWORD_SetUnsignedField(RegMap+0xC9, 7,0, Register.STA2.w8);
    wiUWORD_SetUnsignedField(RegMap+0xCA, 7,0, Register.STA3.w8);
    wiUWORD_SetUnsignedField(RegMap+0xCB, 7,0, Register.STA4.w8);
    wiUWORD_SetUnsignedField(RegMap+0xCC, 7,0, Register.STA5.w8);
    wiUWORD_SetUnsignedField(RegMap+0xCD, 7,0, Register.minRSSI.w8);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 7,7, Register.StepUpRestart.w1);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 6,6, Register.StepDownRestart.w1);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 5,5, Register.NoStepAfterSync.w1);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 4,4, Register.NoStepAfterHdr.w1);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 1,1, Register.KeepCCA_New.w1);
    wiUWORD_SetUnsignedField(RegMap+0xCE, 0,0, Register.KeepCCA_Lost.w1);
    wiUWORD_SetUnsignedField(RegMap+0xD0, 7,0, Register.WakeupTimeH.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD1, 7,0, Register.WakeupTimeL.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD2, 7,0, Register.DelayStdby.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD3, 7,0, Register.DelayBGEnB.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD4, 7,0, Register.DelayADC1.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD5, 7,0, Register.DelayADC2.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD6, 7,0, Register.DelayModem.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD7, 7,0, Register.DelayDFE.w8);
    wiUWORD_SetUnsignedField(RegMap+0xD8, 7,4, Register.DelayRFSW.w4);
    wiUWORD_SetUnsignedField(RegMap+0xD8, 3,0, Register.TxRxTime.w4);
    wiUWORD_SetUnsignedField(RegMap+0xD9, 7,4, Register.DelayFCAL1.w4);
    wiUWORD_SetUnsignedField(RegMap+0xD9, 3,0, Register.DelayPA.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDA, 7,4, Register.DelayFCAL2.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDA, 3,0, Register.DelayState28.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDB, 7,4, Register.DelayShutdown.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDB, 3,0, Register.DelaySleep.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDC, 7,0, Register.TimeExtend.w8);
    wiUWORD_SetUnsignedField(RegMap+0xDD, 5,0, Register.SquelchTime.w6);
    wiUWORD_SetUnsignedField(RegMap+0xDE, 7,4, Register.DelaySquelchRF.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDE, 3,0, Register.DelayClearDCX.w4);
    wiUWORD_SetUnsignedField(RegMap+0xDF, 3,0, Register.SelectCCA.w4);
    wiUWORD_SetUnsignedField(RegMap+0xE0, 7,0, Register.RadioMC.w8);
    wiUWORD_SetUnsignedField(RegMap+0xE1, 5,0, Register.DACOPM.w6);
    wiUWORD_SetUnsignedField(RegMap+0xE2, 5,0, Register.ADCAOPM.w6);
    wiUWORD_SetUnsignedField(RegMap+0xE3, 5,0, Register.ADCBOPM.w6);
    wiUWORD_SetUnsignedField(RegMap+0xE4, 7,0, Register.RFSW1.w8);
    wiUWORD_SetUnsignedField(RegMap+0xE5, 7,0, Register.RFSW2.w8);
    wiUWORD_SetUnsignedField(RegMap+0xE6, 7,6, Register.LNAEnB.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE6, 3,2, Register.PA24_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE6, 1,0, Register.PA50_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE7, 7,6, Register.LNA24A_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE7, 5,4, Register.LNA24B_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE7, 3,2, Register.LNA50A_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE7, 1,0, Register.LNA50B_MODE.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 7,6, Register.ADCFSCTRL.w2);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 4,4, Register.ADCOE.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 3,3, Register.LNAxxBOnSW2.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 2,2, Register.ADCOFCEN.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 1,1, Register.ADCDCREN.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE8, 0,0, Register.ADCMUX0.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE9, 7,7, Register.ADCARDY.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE9, 6,6, Register.ADCBRDY.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE9, 2,2, Register.ADCFCAL2.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE9, 1,1, Register.ADCFCAL1.w1);
    wiUWORD_SetUnsignedField(RegMap+0xE9, 0,0, Register.ADCFCAL.w1);
    wiUWORD_SetUnsignedField(RegMap+0xEA, 2,0, Register.DACIFS_CTRL.w3);
    wiUWORD_SetUnsignedField(RegMap+0xEB, 7,0, Register.DACFG_CTRL0.w8);
    wiUWORD_SetUnsignedField(RegMap+0xEC, 7,0, Register.DACFG_CTRL1.w8);
    wiUWORD_SetUnsignedField(RegMap+0xED, 7,7, Register.RESETB.w1);
    wiUWORD_SetUnsignedField(RegMap+0xED, 2,2, Register.SREdge.w1);
    wiUWORD_SetUnsignedField(RegMap+0xED, 1,0, Register.SRFreq.w2);

    // Entries below added manually -------------------------------------------
    wiUWORD_SetUnsignedField(RegMap+0x12, 3,0, Register.RX_LENGTH.w12 >> 8);
    wiUWORD_SetUnsignedField(RegMap+0x13, 7,0, Register.RX_LENGTH.w8);
    wiUWORD_SetUnsignedField(RegMap+0x14, 7,0, Register.RX_SERVICE.w16 >> 8);
    wiUWORD_SetUnsignedField(RegMap+0x15, 7,0, Register.RX_SERVICE.w8);

    return WI_SUCCESS;
}
// end of Mojave_WriteRegisterMap()

// ================================================================================================
// FUNCTION  : Mojave_WriteRegister()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write a register by address
// ================================================================================================
wiStatus Mojave_WriteRegister(wiUInt Addr, wiUInt Data)
{
    if(Verbose) wiPrintf(" Mojave_WriteRegister(0x%03X, 0x%02X)\n",Addr,Data);
    if(InvalidRange(Addr, 0, 0xFF)) return STATUS(WI_ERROR_PARAMETER1);
    if(InvalidRange(Data, 0, 0xFF)) return STATUS(WI_ERROR_PARAMETER2);

    XSTATUS(Mojave_WriteRegisterMap(RegMap)); // copy parameters to register map
    RegMap[Addr].word = Data;                 // write data to register map
    XSTATUS(Mojave_ReadRegisterMap(RegMap));  // load parameters from register map
    return WI_SUCCESS;
}
// end of Mojave_WriteRegister()

// ================================================================================================
// FUNCTION  : Mojave_LoadRegisters()
// ------------------------------------------------------------------------------------------------
// Purpose   : Save a register map to a file
// Parameters: RegMap   -- pointer to the register map to which to parameters are written
//             Filename -- name of the file to write
// ================================================================================================
wiStatus Mojave_LoadRegisters(wiUWORD *RegMap, wiString Filename)
{
    char buf[256], *cptr, *endptr;
    int addr, data;
    FILE *F;

    if(Verbose) wiPrintf(" Mojave_LoadRegisters(@x%08X, \"%s\")\n",RegMap,Filename);

    F = wiParse_OpenFile(Filename, "rt");
    if(!F) return STATUS(WI_ERROR_FILE_OPEN);

    while(!feof(F))
    {
        if(feof(F)) continue;

        fgets(buf, 255, F);
        if(strstr(buf,"//")) *strstr(buf,"//")='\0'; // remove trailing comments
        cptr = buf;
        while(*cptr==' ' || *cptr=='\t' || *cptr=='\n') cptr++;
        if(!*cptr) continue;
        cptr = strstr(buf,"Reg[");
        if(!cptr) {fclose(F); return STATUS(WI_ERROR);}
        cptr+=4;
        while(*cptr==' ') cptr++;
        addr = strtoul(cptr, &endptr, 10);
        while(*endptr==' ') endptr++;
        if(strstr(buf,"]=8'd") != endptr) {fclose(F); return STATUS(WI_ERROR);}
        cptr = endptr + 5;
        data = strtoul(cptr, &endptr, 10);
        if(*endptr==';') endptr++; // allow semicolon at end of assignment
        while(*endptr==' ' || *endptr=='\t' || *endptr=='\r' || *endptr=='\n') endptr++;
        if(*endptr != '\0') {fclose(F); return STATUS(WI_ERROR);}
        if(addr>255) {fclose(F); return STATUS(WI_ERROR);}
        RegMap[addr].word = data;
    }
    fclose(F);
    return WI_SUCCESS;
}
// end of Mojave_LoadRegisters()

// ================================================================================================
// FUNCTION  : Mojave_SaveRegisters()
// ------------------------------------------------------------------------------------------------
// Purpose   : Save a register map to a file
// Parameters: RegMap   -- pointer to the register map to which to parameters are written
//             Filename -- name of the file to write
// ================================================================================================
wiStatus Mojave_SaveRegisters(wiUWORD *RegMap, wiString Filename)
{
    int i; time_t t;
    FILE *F;

    F = fopen(Filename, "wt"); time(&t);
    if(!F) return STATUS(WI_ERROR_FILE_OPEN);
    fprintf(F,"// ----------------------------------------------------------------------------\n");
    fprintf(F,"// Settings from WiSE::Mojave: %s",ctime(&t));
    fprintf(F,"// ----------------------------------------------------------------------------\n");
    for(i=0; i<256; i++) {
        fprintf(F,"Reg[%3d]=8'd%-3u //Reg[0x%03X]=0x%02X\n",i,RegMap[i].w8,i,RegMap[i]);
    }
    fclose(F);
    return WI_SUCCESS;
}
// end of Mojave_SaveRegisters()

// ================================================================================================
// FUNCTION  : Mojave_GetRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Update and return the internal register map
// Parameters: pRegMap -- pointer (by reference) to the register map
// ================================================================================================
wiStatus Mojave_GetRegisterMap(wiUWORD **pRegMap)
{
    if(Verbose) wiPrintf(" Mojave_GetRegisterMap(@x%08X<-@x%08X)\n",pRegMap,RegMap);
    XSTATUS(Mojave_WriteRegisterMap(RegMap));
    *pRegMap = RegMap;
    return WI_SUCCESS;
}
// end of Mojave_GetRegisterMap()

// ================================================================================================
// FUNCTION  : Mojave_SetRegisterMap()
// ------------------------------------------------------------------------------------------------
// Purpose   : Set the contents of the internal register map and update parameters
// Parameters: xRegMap -- pointer to the external register map
// ================================================================================================
wiStatus Mojave_SetRegisterMap(wiUWORD *xRegMap)
{
    int i;
    if(Verbose) wiPrintf(" Mojave_SetRegisterMap(@x%08X->@x%08X)\n",xRegMap,RegMap);

    for(i=0; i<256; i++) RegMap[i] = xRegMap[i];
    XSTATUS(Mojave_ReadRegisterMap(RegMap));
    return WI_SUCCESS;
}
// end of Mojave_SetRegisterMap()

// ================================================================================================
// FUNCTION  : aMojave_DataGenerator()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmitter Packet Data Generator
// Parameters: PDG_Header  -- Indicates the PHY controller header bytes are being read
//             PDG_Signal  -- Indicates the SIGNAL symbol is being generated 
//             PDG_Data    -- PSDU octets, 8 bits (circuit input from MAC FIFO)
//             a           -- serialized packet data to encoder (circuit output @ 80 MHz)
//             N_DBPS      -- number of data bits per symbol (model input)
//             pReg        -- pointer to PHY register array (model input)
//             TxDone      -- indicates end of packet (last symbol)
// ================================================================================================
wiStatus aMojave_DataGenerator(int PDG_Header, int PDG_Signal, wiUWORD PDG_Data[], wiUWORD a[], 
                               wiUWORD PDG_Out[], int N_DBPS, MojaveRegisters *pReg, wiBoolean TxDone)
{
    static wiUWORD X; // scrambler state
    static unsigned int bit_count;
    int i, k;
    wiUWORD p;

    if(PDG_Header)
    {
        // ------------------------------------------------------
        // Read the first four data bytes and extract information
        // ------------------------------------------------------
        pReg->TX_PWRLVL.word = (unsigned int)  PDG_Data[0].w8;
        pReg->TX_RATE.word   = (unsigned int) (PDG_Data[2].w8&0xF0)>>4;
        pReg->TX_LENGTH.word = (unsigned int)((PDG_Data[2].w8&0x0F)<<8) | PDG_Data[3].w8;
    }
    else if(PDG_Signal)
    {
        if(TX.UseForcedSIGNAL)
        {
            // --------------------------------------------------------------
            // Override the normal "SIGNAL generation and force the data bits
            // --------------------------------------------------------------
            for(i=0; i<24; i++)
            {
                a[i].bit = ( TX.ForcedSIGNAL.w24 >> i ) & 1;
            }
        }
        else
        {
            // ---------------------------------------
            // Generate and Insert the "SIGNAL" Symbol
            // ---------------------------------------
            a[ 0].bit = pReg->TX_RATE.b3;   // RATE[3]
            a[ 1].bit = pReg->TX_RATE.b2;   // RATE[2]
            a[ 2].bit = pReg->TX_RATE.b1;   // RATE[1]
            a[ 3].bit = pReg->TX_RATE.b0;   // RATE[0]
            a[ 4].bit = 0;                  // Reserved (0)
            a[ 5].bit = pReg->TX_LENGTH.b0; // LENGTH[0]  
            a[ 6].bit = pReg->TX_LENGTH.b1; // LENGTH[1]  
            a[ 7].bit = pReg->TX_LENGTH.b2; // LENGTH[2]  
            a[ 8].bit = pReg->TX_LENGTH.b3; // LENGTH[3]  
            a[ 9].bit = pReg->TX_LENGTH.b4; // LENGTH[4]  
            a[10].bit = pReg->TX_LENGTH.b5; // LENGTH[5]  
            a[11].bit = pReg->TX_LENGTH.b6; // LENGTH[6]  
            a[12].bit = pReg->TX_LENGTH.b7; // LENGTH[7]  
            a[13].bit = pReg->TX_LENGTH.b8; // LENGTH[8]  
            a[14].bit = pReg->TX_LENGTH.b9; // LENGTH[9]  
            a[15].bit = pReg->TX_LENGTH.b10;// LENGTH[10]  
            a[16].bit = pReg->TX_LENGTH.b11;// LENGTH[11]  

            // Calculate the parity bit
            p.bit = 0;
            for(i=0; i<17; i++)
                p.bit = p.bit ^ a[i].bit;

            a[17].bit = p.bit;        // Parity
            a[18].bit = 0;            // Decoder Termination Bit (0)
            a[19].bit = 0;            // Decoder Termination Bit (0)
            a[20].bit = 0;            // Decoder Termination Bit (0)
            a[21].bit = 0;            // Decoder Termination Bit (0)
            a[22].bit = 0;            // Decoder Termination Bit (0)
            a[23].bit = 0;            // Decoder Termination Bit (0)
        }
        for(i=0; i<24; i++) PDG_Out[i] = a[i]; // Scrambler DISABLED for SIGNAL
        bit_count = 0;            // reset the data bit counter

        X = Register.TX_PRBS; // load the PRBS seed for subsequent processing
    }
    else
    {
        // --------------------------------
        // Create the DATA field bit stream
        // --------------------------------
        for(k=0; k<N_DBPS; k++, bit_count++)
        {
            if(bit_count<16) 
                a[k].bit = (pReg->TX_SERVICE.w16 >> k);
            else if(bit_count<16+8*pReg->TX_LENGTH.w12) 
                a[k].bit = (PDG_Data[(bit_count-16)/8].w8 >> (bit_count%8)) & 1;
            else
                a[k].bit = 0;

            // ---------         
            // Scrambler
            // ---------         
            if((bit_count<16+8*pReg->TX_LENGTH.w12) || (bit_count>=22+8*pReg->TX_LENGTH.w12))
            {
                X.w7 = (X.w6 << 1) | (X.b6 ^ X.b3);
                PDG_Out[k].bit = a[k].bit ^ (X.b6 ^ X.b3);
            }
            else
                PDG_Out[k].bit = a[k].bit;
        }
    }
    if(TxDone) 
    {
        // update the PRBS seed using a second PRBS
        X.word = Register.TX_PRBS.w7;
        Register.TX_PRBS.word = (X.w6 << 1) | (X.b6 ^ X.b3);
    }
    return WI_SUCCESS;
}
// end of aMojave_DataGenerator()

// ================================================================================================
// FUNCTION  : aMojave_Encoder()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit data encoder, rate 1/2 + puncturer
// Parameters: EncRate -- Data rate selection, 4-bits (circuit input)
//             EncIn   -- Encoder input bit stream    (circuit output)
//             EncOut  -- Encoder output bit stream
//             Nbits   -- Length of input bit stream  (model input)
// ================================================================================================
void aMojave_Encoder(wiUWORD EncRATE, wiUWORD EncIn[], wiUWORD EncOut[], int Nbits)
{
    wiUWORD *X = &(TX.Encoder_Shift_Register);
    int i, M, A, B, *pA, *pB;

    // -----------------------------
    // Select the puncturing pattern
    // -----------------------------
    switch(EncRATE.w4)
    {
        case 0xD: pA = pA_12; pB = pB_12; M=1; break;
        case 0xF: pA = pA_34; pB = pB_34; M=9; break;
        case 0x5: pA = pA_12; pB = pB_12; M=1; break;
        case 0x7: pA = pA_34; pB = pB_34; M=9; break;
        case 0x9: pA = pA_12; pB = pB_12; M=1; break;
        case 0xB: pA = pA_34; pB = pB_34; M=9; break;
        case 0x1: pA = pA_23; pB = pB_23; M=6; break;
        case 0x3: pA = pA_34; pB = pB_34; M=9; break;
        case 0xA: pA = pA_34; pB = pB_34; M=9; break;
        default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
    }
    // -------------------------------
    // Apply the convolutional encoder
    // -------------------------------
    for(i=0; i<Nbits; i++)
    {
        X->w7 = (X->w7 << 1) | EncIn[i].bit; 

        // -----------------------------------------------------------------
        // Output two bits from octal polynomials 0133 and 0171
        // -----------------------------------------------------------------
        A = X->b0 ^ X->b2 ^ X->b3 ^ X->b5 ^ X->b6;
        B = X->b0 ^ X->b1 ^ X->b2 ^ X->b3 ^ X->b6;

        // -----------------------------------------------------------------
        // The bits are output only if the puncturing pattern contains a '1'
        // -----------------------------------------------------------------
        if(pA[i%M]) (EncOut++)->word = A;
        if(pB[i%M]) (EncOut++)->word = B;
    }
}
// end of aMojave_Encoder()

// ================================================================================================
// FUNCTION  : aMojave_Interleaver()
// ------------------------------------------------------------------------------------------------
// Purpose   : Interleaver/Deinterleaver for both Transmit and Receive
// Parameters: IntlvRATE -- RATE input indicating the data rate (circuit input)
//             IntlvIn   -- Interleaver input, 5 bits
//             IntlvOut  -- Interleaver output, 5 bits
//             IntlvTxRx -- transmit/receive switch, 0=transmit, 1=receive (circuit input)
// ------------------------------------------------------------------------------------------------
void aMojave_Interleaver(wiUWORD IntlvRATE, wiUWORD IntlvIn[], wiUWORD IntlvOut[], int IntlvTxRx)
{
    const int cbps[] = {288, 96, 192, 48, 384};
    wiUWORD RAM[4][128];
    wiUWORD *rROM[2], *wROM[2];
    wiUWORD addr, bpsc;
    int a, c, i, p;

    switch(IntlvRATE.w4) // Look-up table
    {
        case 0xD: case 0xF: bpsc.w3 = 3; break;
        case 0x5: case 0x7: bpsc.w3 = 1; break;
        case 0x9: case 0xB: bpsc.w3 = 2; break;
        case 0x1: case 0x3: bpsc.w3 = 0; break;
        case 0xA:           bpsc.w3 = 4; break;
        default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
    }
    addr.b10 = bpsc.b2;
    addr.b9  = bpsc.b1;
    addr.b8  = bpsc.b0;

    if(IntlvTxRx==0) // Transmit -----------------
    {
        wROM[0] = TX.jROM[0]; wROM[1] = TX.jROM[1];
        rROM[0] = TX.kROM[0]; rROM[1] = TX.kROM[1];
    }
    else             // Receive ------------------
    {
        wROM[0] = TX.kROM[0]; wROM[1] = TX.kROM[1];
        rROM[0] = TX.jROM[0]; rROM[1] = TX.jROM[1];
    }
    for(i=0; i<cbps[bpsc.w3]/2; i++) { // Write --
        addr.w8 = i;
        for(p=0; p<2; p++) {
            c = wROM[p][addr.w11].w9 & 0x3;
            a = wROM[p][addr.w11].w9 >> 2;
            RAM[c][a].w5 = IntlvIn[2*i+p].w5;
        }
    }
    for(i=0; i<cbps[bpsc.w3]/2; i++) { // Read ---
        addr.w8 = i;
        for(p=0; p<2; p++) {
            c = rROM[p][addr.w11].w9 & 0x3;
            a = rROM[p][addr.w11].w9 >> 2;
            IntlvOut[2*i+p].word = RAM[c][a].w5;
        }
    }
}
// end of aMojave_Interleaver()

// ================================================================================================
// FUNCTION  : aMojave_Modulator()
// ------------------------------------------------------------------------------------------------
// Parameters: ModuRate  -- RATE input to define the constellation, 4-bit (circuit input)
//             ModuX     -- Modulator input, 1-bit (modeled circuit input @ 80 MHz)
//             ModuOutR  -- Modulator output, 9-bit (circuit output @ 20 MHz)
//             ModuOutI  -- Modulator output, 9-bit (circuit output @ 20 MHz)
// ================================================================================================
void aMojave_Modulator(wiUWORD ModuRate, wiUWORD ModuX[], wiWORD ModuOutR[], wiWORD ModuOutI[])
{
    wiUWORD c, p, AdrR, AdrI;
    int k;

    // --------------------------------------------
    // Obtain the pilot tone polarity from the PRBS
    // --------------------------------------------
    {
        wiUWORD *X = &(TX.Pilot_Shift_Register);
        p.bit = X->b6 ^ X->b3;
        X->w7 = (X->w7 << 1) | p.bit;
    }
    // ------------------------------------------------
    // Set the high nibble on the PSK/QAM ROM addresses
    // ------------------------------------------------
    AdrR.w8 = AdrI.w8 = (ModuRate.w4 << 4);

    for(k=0; k<64; k++)
    {
        switch(subcarrier_type[k])
        {
            
            case 0: // Null Subcarrier -------------------------------------------
            {
                ModuOutR[k].word = 0;
                ModuOutI[k].word = 0;
                break;
            }
            case 1: // Data Subcarrier -------------------------------------------
            {
                switch(ModuRate.w4)
                {          
                    case 0xD:
                    case 0xF:
                        AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                        break;
            
                    case 0x5:
                    case 0x7:
                        AdrR.b3=0; AdrR.b2=0; AdrR.b1=0; AdrR.b0=(ModuX++)->bit;
                        AdrI.b3=0; AdrI.b2=0; AdrI.b1=0; AdrI.b0=(ModuX++)->bit;
                        break;

                    case 0x9:
                    case 0xB:
                        AdrR.b3=0; AdrR.b2=0; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                        AdrI.b3=0; AdrI.b2=0; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                        break;

                    case 0x1:
                    case 0x3:
                        AdrR.b3=0; AdrR.b2=(ModuX++)->bit; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                        AdrI.b3=0; AdrI.b2=(ModuX++)->bit; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                        break;

                    case 0xA:
                        AdrR.b3=(ModuX++)->bit; AdrR.b2=(ModuX++)->bit; AdrR.b1=(ModuX++)->bit; AdrR.b0=(ModuX++)->bit;
                        AdrI.b3=(ModuX++)->bit; AdrI.b2=(ModuX++)->bit; AdrI.b1=(ModuX++)->bit; AdrI.b0=(ModuX++)->bit;
                        break;
                }
                if((ModuRate.w4 == 0xD) || (ModuRate.w4 == 0xF))
                {
                    ModuOutR[k].word = TX.ModulationROM[AdrR.w8].w5 << 4;
                    ModuOutI[k].word = 0;
                }
                else
                {
                    ModuOutR[k].word = TX.ModulationROM[AdrR.w8].w5 << 4;
                    ModuOutI[k].word = TX.ModulationROM[AdrI.w8].w5 << 4;
                }
            }  break;

            case 2: // Pilot Subcarrier ------------------------------------------
            {            
                c.bit = p.bit ^ ((k==53)? 1:0);
                ModuOutR[k].word = c.bit? -208:208;
                ModuOutI[k].word = 0;
            }  break;
        }
    }            
}
// end of aMojave_TX_Modulator()

// ================================================================================================
// FUNCTION  : aMojave_Preamble()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit preamble generator
// Parameters: PreamOutR -- Preamble output, real component, 10-bits (circuit output)
//             PreamOutI -- Preamble output, imag component, 10-bits (circuit output)
// ================================================================================================
void aMojave_Preamble(wiWORD PreamOutR[], wiWORD PreamOutI[])
{
    const int shortR[16] = { 19,-55,-6,59,38,59,-6,-55,19,1,-33,-5,0,-5,-33,1};
    const int shortI[16] = { 19,1,-33,-5,0,-5,-33,1,19,-55,-6,59,38,59,-6,-55};
    const int longR[64]  = { 65,-2,17,40,9,25,-48,-16,41,22,0,-57,10,24,-9,50,26,15,-24,-55,34,29,-25,-23,-15,-51,-53,31,-1,-38,38,5,-65,5,38,-38,-1,31,-53,-51,-15,-23,-25,29,34,-55,-24,15,26,50,-9,24,10,-57,0,22,41,-16,-48,25,9,40,17,-2};
    const int longI[64]  = { 0,-50,-46,34,12,-36,-23,-44,-11,2,-48,-20,-24,-6,67,-2,-26,41,16,27,38,6,34,-9,-63,-7,-9,-31,22,48,44,41,0,-41,-44,-48,-22,31,9,7,63,9,-34,-6,-38,-27,-16,-41,26,2,-67,6,24,20,48,-2,11,44,23,36,-12,-34,46,50};
    wiUWORD k;

    // ----------------------------
    // Insert the "Short Sequences"
    // ----------------------------
    for(k.w9=0; k.w9<320; k.w9++)
    {
        if(k.w9 < 160)
        {
            PreamOutR->word = shortR[k.w4] << 1;
            PreamOutI->word = shortI[k.w4] << 1;
        }
        else
        {
            PreamOutR->word = longR[k.w6] << 1;
            PreamOutI->word = longI[k.w6] << 1;
        }
        PreamOutR++;
        PreamOutI++;
    }
}
// end of aMojave_Preamble()

// ================================================================================================
// FUNCTION  : aMojave_UpsamFIR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Core FIR filter implementation used with module "Upsam"
// Parameters: Nx -- Length of array x (model input)
//             x  -- Input data array, 10 bit (circuit input)
//             Ny -- Length of array x (model input)
//             y  -- Output data array, 11 bit (circuit output)
// ================================================================================================
void aMojave_UpsamFIR(int Nx, wiWORD x[], int Ny, wiWORD y[])
{
    const int b0[] = { 0,  0,  0,  0,  0,  0, 64,  0,  0,  0,  0,  0};
    const int b1[] = { 0,  1, -2,  4, -7, 19, 57,-11,  5, -3,  1, -1};
    const int b2[] = {-1,  2, -3,  6,-12, 40, 40,-12,  6, -3,  2, -1};
    const int b3[] = {-1,  1, -3,  5,-11, 57, 19, -7,  4, -2,  1,  0};

    wiWORD v[4], z[12]={0,0,0,0,0,0,0,0,0,0,0,0};
    int i, j, k;

    for(k=0; k<Ny; k++)
    {
        if(k%4==0)
        {
            for(i=11; i>0; i--) 
                z[i].w10 = z[i-1].w10;
        
            j = k/4;
            z[0].word = (j<Nx)? x[j].w10:0;
            v[0].word = v[1].word = v[2].word = v[3].word = 0;

            for(i=0; i<12; i++)
            {
                v[0].w16 += (b0[i] * z[i].w10);
                v[1].w16 += (b1[i] * z[i].w10);
                v[2].w16 += (b2[i] * z[i].w10);
                v[3].w16 += (b3[i] * z[i].w10);
            }
        }
        switch(k%4)
        {
            case 0: y[k].word = v[0].w16 >> 5; break;
            case 1: y[k].word = v[1].w16 >> 5; break;
            case 2: y[k].word = v[2].w16 >> 5; break;
            case 3: y[k].word = v[3].w16 >> 5; break;
        }
    }
}
// end of aMojave_UpsamFIR()

// ================================================================================================
// FUNCTION  : aMojave_Upsam()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmitter Up-Sampling (Interpolation) Filter
// Parameters: Nx        -- length of UpsamInR and UpsamInI arrays (model input)
//             UpsamInR  -- filter input, real component, 10 bits (circuit input @ 20MHz)
//             UpsamInI  -- filter input, imag component, 10 bits (circuit input @ 20MHz)
//             Ny        -- length of UpsamOutR and UpsamOutI arrays (model input)
//             UpsamOutR -- filter output, real component, 11 bits (circuit output @ 80MHz)
//             UpsamOutI -- filter output, imag component, 11 bits (circuit output @ 80MHz)
// ================================================================================================
void aMojave_Upsam(int Nx, wiWORD UpsamInR[],  wiWORD UpsamInI[], 
                        int Ny, wiWORD UpsamOutR[], wiWORD UpsamOutI[])
{
    aMojave_UpsamFIR(Nx, UpsamInR, Ny, UpsamOutR);
    aMojave_UpsamFIR(Nx, UpsamInI, Ny, UpsamOutI);
}
// end of aMojave_Upsam()

// ================================================================================================
// FUNCTION  : Mojave_Upmix()
// ------------------------------------------------------------------------------------------------
// Purpose   : Frequency shift the transmit channel
// Parameters: N         -- length of input/output arrays (model input)
//             UpmixInR  -- mixer input, real component,  11 bits (circuit input  @ 80MHz)
//             UpmixInI  -- mixer input, imag component,  11 bits (circuit input  @ 80MHz)
//             UpmixOutR -- mixer output, real component, 10 bits (circuit output @ 80MHz)
//             UpmixOutI -- mixer output, imag component, 10 bits (circuit output @ 80MHz)
// ================================================================================================
void Mojave_Upmix(int N, wiWORD UpmixInR[], wiWORD UpmixInI[], wiWORD UpmixOutR[], wiWORD UpmixOutI[])
{
    const int wR[8] = { 256, 181,   0,-181,-256,-181,   0, 181};
    const int wI[8] = {   0, 181, 256, 181,   0,-181,-256,-181};
    int k, i;

    if(Register.UpmixOff.b0) // bypass mixer - scaling only
    {
        for(k=0; k<N; k++)
        {
            UpmixOutR[k].word = UpmixInR[k].w11;
            UpmixOutI[k].word = UpmixInI[k].w11;

            // -----------------------------
            // Limit Output Range to 10 bits
            // -----------------------------
            if(abs(UpmixOutR[k].w11)>511) UpmixOutR[k].word = (UpmixOutR[k].word<0)? -511:511;
            if(abs(UpmixOutI[k].w11)>511) UpmixOutI[k].word = (UpmixOutI[k].word<0)? -511:511;
        }
    }
    else // active mixer: +/-10MHz 
    {
        for(k=0; k<N; k++)
        {
            i = Register.UpmixDown.b0? (N+1-k)%8 : k%8;
            UpmixOutR[k].word = ((wR[i] * UpmixInR[k].w11) - (wI[i] * UpmixInI[k].w11)); 
            UpmixOutI[k].word = ((wR[i] * UpmixInI[k].w11) + (wI[i] * UpmixInR[k].w11)); 

            UpmixOutR[k].word = UpmixOutR[k].w20 >> 8;
            UpmixOutI[k].word = UpmixOutI[k].w20 >> 8;

            // -----------------------------
            // Limit Output Range to 10 bits
            // -----------------------------
            if(abs(UpmixOutR[k].word)>511) UpmixOutR[k].word = (UpmixOutR[k].word<0)? -511:511;
            if(abs(UpmixOutI[k].word)>511) UpmixOutI[k].word = (UpmixOutI[k].word<0)? -511:511;
        }
    }
    // -----------------
    // Complex Conjugate
    // -----------------
    if(Register.UpmixConj.b0)
    {
        for(k=0; k<N; k++)
            UpmixOutI[k].word = -UpmixOutI[k].word;
    }
}
// end of aMojave_Upmix()

// ================================================================================================
// FUNCTION : Mojave_Prd()
// ------------------------------------------------------------------------------------------------
// Purpose   : This function is a placeholder for a predistortion function. It currently
//             performs a translation from two's-complement to unsigned notation.
// Parameters: N       -- length of input/output arrays (model input)
//             PrdInR  -- PrD in,  real, 10 bits (circuit input @ 80MHz)
//             PrdInI  -- PrD in,  imag, 10 bits (circuit input @ 80MHz)
//             PrdOutR -- PrD out, real, 10 bits (circuit output @ 80MHz)
//             PrdOutI -- PrD out, imag, 10 bits (circuit output @ 80MHz)
// ================================================================================================
void Mojave_Prd(int N, wiWORD PrdInR[], wiWORD PrdInI[], wiUWORD PrdOutR[], wiUWORD PrdOutI[],
                wiUWORD PrdDACSrcR, wiUWORD PrdDACSrcI)
{
    const unsigned int cosLUT[8] = {768, 693, 512, 331, 256, 331, 512, 693}; // +10 MHz tone
    const unsigned int sinLUT[8] = {512, 693, 768, 693, 512, 331, 256, 331};
    int k;

    switch(PrdDACSrcR.w2)
    {
        case 0: for(k=0; k<N; k++) PrdOutR[k].word = PrdInR[k].w10 + 512; break;
        case 1:
        case 3: for(k=0; k<N; k++) PrdOutR[k].word = 512; break;
        case 2: for(k=0; k<N; k++) PrdOutR[k].word = cosLUT[k%8];
    }
    switch(PrdDACSrcI.w2)
    {
        case 0: for(k=0; k<N; k++) PrdOutI[k].word = PrdInI[k].w10 + 512; break;
        case 1:
        case 3: for(k=0; k<N; k++) PrdOutI[k].word = 512; break;
        case 2: for(k=0; k<N; k++) PrdOutI[k].word = sinLUT[k%8];
    }
}
// end of Mojave_Prd()

// ================================================================================================
// FUNCTION  : Mojave_DAC()
// ------------------------------------------------------------------------------------------------
// Parameters: N  -- Number of input samples in {uR,uI}
//             uR -- Real DAC input
//             uI -- Imaginary DAC input
//             s  -- DAC output signal (analog)
//             M  -- Oversample rate of s relative to {uR,uI}
// ================================================================================================
void Mojave_DAC(int N, wiUWORD uR[], wiUWORD uI[], wiComplex s[], int M)
{
    int k, i;

    if(M==1) // normal output
    {
        for(k=0; k<N; k++)
        {
            s[k].Real = TX.DAC.c * (signed int)((uR[k].w10 & TX.DAC.Mask) - 512);
            s[k].Imag = TX.DAC.c * (signed int)((uI[k].w10 & TX.DAC.Mask) - 512);
        }
    }
    else // oversampled output
    {
        for(k=0; k<N; k++) {
            for(i=0; i<M; i++)
            {
                s[k*M+i].Real = TX.DAC.c * (signed int)((uR[k].w10 & TX.DAC.Mask) - 512);
                s[k*M+i].Imag = TX.DAC.c * (signed int)((uI[k].w10 & TX.DAC.Mask) - 512);
            }
        }
    }
    // -------------------
    // I/Q Amplitude Error
    // -------------------
    if(TX.DAC.IQdB)
    {
        double a = atan(pow(10, TX.DAC.IQdB/20.0));
        double aReal = sqrt(2.0) * sin(a);
        double aImag = sqrt(2.0) * cos(a);
        for(k=0; k<N*M; k++)
        {
            s[k].Real *= aReal;
            s[k].Imag *= aImag;
        }
    }
}
// end of Mojave_DAC()

// ================================================================================================
// FUNCTION  : Mojave_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Baseband Top-level TX routine
// Parameters: Length   -- Number of bytes in MAC_TX_D
//             MAC_TX_D -- Data bytes transferred from MAC, including header
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
// ================================================================================================
wiStatus Mojave_TX(int Length, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M)
{
    wiUWORD TXPWRLVL, TXbMode, RATE, LENGTH;

    if(Verbose) wiPrintf(" Mojave_TX(%d,*,*,%d)\n",Length,M);

    // --------------------
    // Error/Range Checking
    // --------------------
    if(InvalidRange(Length,8,4095+8)) return STATUS(WI_ERROR_PARAMETER1);
    if(!MAC_TX_D)                     return STATUS(WI_ERROR_PARAMETER2);
    if(!Nx)                           return STATUS(WI_ERROR_PARAMETER3);
    if(!x)                            return STATUS(WI_ERROR_PARAMETER4);
    if(InvalidRange(M, 1, 40000))     return STATUS(WI_ERROR_PARAMETER5);

    // -----------------------------------
    // Point to the input data (for trace)
    // -----------------------------------
    TX.MAC_TX_D = MAC_TX_D;

    // -------------------------------------------------
    // Read the PHY control bytes from the MAC interface
    // -------------------------------------------------
    TXPWRLVL.word =   MAC_TX_D[0].w8;
    TXbMode.word  =   MAC_TX_D[1].b7;
    RATE.word     =  (MAC_TX_D[2].w8&0xF0)>>4;
    LENGTH.word   = ((MAC_TX_D[2].w8&0x0F)<<8) | MAC_TX_D[3].w8;

    // ----------------------------
    // Store in Read-Only Registers
    // ----------------------------
    Register.TX_PWRLVL.word = TXPWRLVL.w8;
    Register.TX_RATE.word   = RATE.w4;
    Register.TX_LENGTH.word = LENGTH.w12;
    
    // -----------------------------
    // Select OFDM or DSSS/CCK Modem
    // -----------------------------
    switch(TXbMode.b0)
    {
        case 0: XSTATUS( aMojave_TX(RATE, LENGTH, MAC_TX_D+8, Nx, x, M) ); break;
        case 1: XSTATUS( bMojave_TX(RATE, LENGTH, MAC_TX_D+8, Nx, x, M, &Register, &TX) ); break;
    }
    return WI_SUCCESS;
}
// end of Mojave_TX()

// ================================================================================================
// FUNCTION  : aMojave_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-Level OFDM TX routine
// Parameters: RATE     -- Data rate field from PHY Header
//             LENGTH   -- Number of PSDU bytes (in MAC_TX_D)
//             MAC_TX_D -- Data bytes transferred from MAC, including header
//             Nx       -- Number of transmit samples (by reference)
//             x        -- Transmit signal array/DAC output (by reference)
//             M        -- Oversample rate for analog signal
// ================================================================================================
wiStatus aMojave_TX(wiUWORD RATE, wiUWORD LENGTH, wiUWORD *MAC_TX_D, int *Nx, wiComplex **x, int M)
{
    unsigned int i, db, dx, dv;
    int N, ofs, N_BPSC, N_DBPS;
    wiUWORD Rate;
    wiBoolean TxDone;

    if(Verbose) wiPrintf(" aMojave_TX(0x%X,%d,*,*,%d)\n",RATE.w4,RATE.w12,M);

    // --------------------
    // Error/Range Checking
    // --------------------
    if(!MAC_TX_D)                     return STATUS(WI_ERROR_PARAMETER3);
    if(!Nx)                           return STATUS(WI_ERROR_PARAMETER4);
    if(!x)                            return STATUS(WI_ERROR_PARAMETER5);
    if(InvalidRange(M, 1, 40000))     return STATUS(WI_ERROR_PARAMETER6);

    // --------------------------------------------------------
    // Signal the TX components that a transmission is starting
    // --------------------------------------------------------
    TX.Encoder_Shift_Register.w7 = 0x00; // reset the encoder
    TX.Pilot_Shift_Register.w7   = 0x7F; // reset the pilot tone polarity PRBS

    // -------------------------------------------
    // Calculate timing information for controller
    // -------------------------------------------
    switch(RATE.w4)
    {
        case 0xD: N_BPSC = 1; N_DBPS =  24; break;
        case 0xF: N_BPSC = 1; N_DBPS =  36; break;
        case 0x5: N_BPSC = 2; N_DBPS =  48; break;
        case 0x7: N_BPSC = 2; N_DBPS =  72; break;
        case 0x9: N_BPSC = 4; N_DBPS =  96; break;
        case 0xB: N_BPSC = 4; N_DBPS = 144; break;
        case 0x1: N_BPSC = 6; N_DBPS = 192; break;
        case 0x3: N_BPSC = 6; N_DBPS = 216; break;
        case 0xA: N_BPSC = 8; N_DBPS = 288; break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // ---------------------------------------------------------------------
    // Compute the number of bits in the DATA field. The data field contains
    // 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and enough pad bits
    // to fill a whole number of OFDM symbols, each containing N_DBPS bits.
    // The leading multiply by 4 is for 80MS/s vs. 20MS/s
    // ---------------------------------------------------------------------
    TX.N_SYM = (16 + 8*LENGTH.w12 + 6 + (N_DBPS-1)) / N_DBPS;
    N     = N_DBPS * TX.N_SYM;

    // ---------------
    // Allocate memory
    // ---------------
    XSTATUS(Mojave_TX_AllocateMemory(TX.N_SYM, N_BPSC, N_DBPS, M));

    // ---------------
    // Output Preamble
    // ---------------
    dx = TX.aPadFront;
    for(i=0; i<dx; i++)
        TX.xReal[i].word = TX.xImag[i].word = 0;
    aMojave_Preamble(TX.xReal+dx, TX.xImag+dx);

    // -------------
    // SIGNAL symbol
    // -------------
    Rate.word = 0xD; // 6 Mbps
    dx = 320 + TX.aPadFront;

    aMojave_DataGenerator(0, 1, MAC_TX_D+8, TX.a, TX.b, 24, &(Register), 0);
    aMojave_Encoder      (Rate, TX.b, TX.c, 24);   // Rate 1/2 convolutional code
    aMojave_Interleaver  (Rate, TX.c, TX.p, 0);
    aMojave_Modulator    (Rate, TX.p, TX.vReal, TX.vImag);
    Mojave_FFT           (0, TX.vReal, TX.vImag, NULL, NULL, TX.xReal+dx, TX.xImag+dx, NULL, NULL);

    // ----------------------------------
    // Create the DATA field OFDM symbols
    // ----------------------------------
    for(i=1; i<=TX.N_SYM; i++)
    {
        ofs = N_BPSC*48*(i-1) + 48;
        db  = N_DBPS*(i-1) + 24;
        dx  = 80*i + 320 + TX.aPadFront;
        dv  = 64*i;
        TxDone = (i==TX.N_SYM)? 1:0; // flag for last symbol

        aMojave_DataGenerator(0, 0, MAC_TX_D, TX.a+db, TX.b+db, N_DBPS, &(Register), TxDone);   
        aMojave_Encoder      (RATE, TX.b+db,  TX.c+ofs, N_DBPS);
        aMojave_Interleaver  (RATE, TX.c+ofs, TX.p+ofs, 0);
        aMojave_Modulator    (RATE, TX.p+ofs, TX.vReal+dv, TX.vImag+dv);
        Mojave_FFT           (0, TX.vReal+dv, TX.vImag+dv, NULL, NULL, TX.xReal+dx, TX.xImag+dx, NULL, NULL);
    }
    // ---------------------------------------------------------
    // Back end padding (zero-extend to flush out channel model)
    // ---------------------------------------------------------
    dx = 80*(TX.N_SYM+1) + 320 + TX.aPadFront;
    TX.OfsEOP = M*(4*dx+24); // approximate end-of-packet
    for(i=0; i<TX.aPadBack; i++)
    {
        TX.xReal[dx+i].word = TX.xImag[dx+i].word = 0;
    }
    // -------------------------
    // Stream out data after FFT
    // -------------------------
    aMojave_Upsam(TX.Nx, TX.xReal, TX.xImag, TX.Nz, TX.yReal, TX.yImag); // Upsampled 20->80 MHz
    Mojave_Upmix (TX.Nz, TX.yReal, TX.yImag, TX.zReal, TX.zImag);        // Mix to low IF
    Mojave_Prd   (TX.Nz, TX.zReal, TX.zImag, TX.uReal, TX.uImag, Register.PrdDACSrcR, Register.PrdDACSrcI); // DAC interface
    Mojave_DAC   (TX.Nz, TX.uReal, TX.uImag, TX.d, M);                   // Digital-to-analog conversion

    if(Nx) *Nx = TX.Nd;
    if(x)  *x  = TX.d;

    return WI_SUCCESS;
}
// end of aMojave_TX()

// ================================================================================================
// FUNCTION  : Mojave_ADC()
// ------------------------------------------------------------------------------------------------
// Purpose   : Models the effect of the analog-to-digital converter
// Parameters: OPM    -- Operating Mode, 2 bits
//             rIn    -- Analog intput (complex)
//             rOutR  -- ADC output, real component, 10 bits
//             rOutI  -- ADC output, imag component, 10 bits
// ================================================================================================
void Mojave_ADC(Mojave_DataConvCtrlState SigIn, Mojave_DataConvCtrlState *SigOut, wiComplex ADCAIQ, wiComplex ADCBIQ, 
                wiUWORD *ADCOut1I, wiUWORD *ADCOut1Q, wiUWORD *ADCOut2I, wiUWORD *ADCOut2Q)
{   
    static wiComplex rA[6], rB[6];
    static int i = 0;
    int xR, xI;

    // ----------------------
    // Pipeline Delay for ADC
    // ----------------------
    rA[i] = ADCAIQ;
    rB[i] = ADCBIQ;
    i     = (i+1)%6; // delay is relative to sample clock @ 40 MHz (6T = 150ns)

    if(SigIn.ADCAOPM==3) // Quadrature ADC "A"
    {
        // --------------------------------------------
        // Quantization: 10 bit ADC (with offset error)
        // --------------------------------------------
        if (RX.ADC.Rounding)
        {
            if(rA[i].Real < 0) xR = (int)(RX.ADC.c * rA[i].Real - 0.5) + RX.ADC.Offset;
            else               xR = (int)(RX.ADC.c * rA[i].Real + 0.5) + RX.ADC.Offset;

            if(rA[i].Imag < 0) xI = (int)(RX.ADC.c * rA[i].Imag - 0.5) + RX.ADC.Offset;
            else               xI = (int)(RX.ADC.c * rA[i].Imag + 0.5) + RX.ADC.Offset;
        }
        else
        {
            xR = (int)(RX.ADC.c * rA[i].Real) + RX.ADC.Offset;
            xI = (int)(RX.ADC.c * rA[i].Imag) + RX.ADC.Offset;
        }
        // ---------------------------------
        // ADC Input Saturation (memoryless)
        // ---------------------------------
        if(abs(xR)>511) xR = (xR<0)? -512:511;
        if(abs(xI)>511) xI = (xI<0)? -512:511;

        // ------------------------------------
        // ADC output format is 10 bit unsigned
        // ------------------------------------
        ADCOut1I->word = (unsigned)(xR+512) & RX.ADC.Mask;
        ADCOut1Q->word = (unsigned)(xI+512) & RX.ADC.Mask;
    }
    else ADCOut1I->word = ADCOut1Q->word = 0; // ADC disabled

    if(SigIn.ADCBOPM==3) // Quadrature ADC "B"
    {
        // --------------------------------------------
        // Quantization: 10 bit ADC (with offset error)
        // --------------------------------------------
        if (RX.ADC.Rounding)
        {
            if(rB[i].Real < 0) xR = (int)(RX.ADC.c * rB[i].Real - 0.5) + RX.ADC.Offset;
            else               xR = (int)(RX.ADC.c * rB[i].Real + 0.5) + RX.ADC.Offset;

            if(rB[i].Imag < 0) xI = (int)(RX.ADC.c * rB[i].Imag - 0.5) + RX.ADC.Offset;
            else               xI = (int)(RX.ADC.c * rB[i].Imag + 0.5) + RX.ADC.Offset;
        }
        else
        {
            xR = (int)(RX.ADC.c * rB[i].Real) + RX.ADC.Offset;
            xI = (int)(RX.ADC.c * rB[i].Imag) + RX.ADC.Offset;
        }
        // ---------------------------------
        // ADC Input Saturation (memoryless)
        // ---------------------------------
        if(abs(xR)>511) xR = (xR<0)? -512:511;
        if(abs(xI)>511) xI = (xI<0)? -512:511;

        // ------------------------------------
        // ADC output format is 10 bit unsigned
        // ------------------------------------
        ADCOut2I->word = (unsigned)(xR+512) & RX.ADC.Mask;
        ADCOut2Q->word = (unsigned)(xI+512) & RX.ADC.Mask;
    }
    else ADCOut2I->word = ADCOut2Q->word = 0; // ADC disabled
}
// end of Mojave_ADC()

// ================================================================================================
// FUNCTION : Mojave_DSel()
// ------------------------------------------------------------------------------------------------
// Purpose   : Select the analog-to-digital path mapping and data format converion
// Parameters: DSelIn0R   -- data select in  (0) real, 10 bits (circuit input @ 80MHz)
//             DSelIn0I   -- data select in  (0) imag, 10 bits (circuit input @ 80MHz)
//             DSelIn1R   -- data select in  (1) real, 10 bits (circuit input @ 80MHz)
//             DSelIn1I   -- data select in  (1) imag, 10 bits (circuit input @ 80MHz)
//             DSelOut1R  -- data select out (0) real, 10 bits (circuit output @ 80MHz)
//             DSelOut1I  -- data select out (0) imag, 10 bits (circuit output @ 80MHz)
//             DSelOut1R  -- data select out (1) real, 10 bits (circuit output @ 80MHz)
//             DSelOut1I  -- data select out (1) imag, 10 bits (circuit output @ 80MHz)
//             DSelPathSel-- data enable for digital paths, 2 bits (circuit input, static)
//             Rst40B     -- global reset
// ================================================================================================
void Mojave_DSel( wiUWORD DSelIn0R,   wiUWORD DSelIn0I,  wiUWORD DSelIn1R,  wiUWORD DSelIn1I,
                        wiWORD *DSelOut0R,   wiWORD *DSelOut0I, wiWORD *DSelOut1R, wiWORD *DSelOut1I, 
                        wiUWORD DSelPathSel, wiBoolean Rst40B)
{
    static wiReg  Out0R,Out0I,Out1R,Out1I; // output registers

    if(!Rst40B)
    {
        Out0R.D.word = Out0R.Q.word = 0;    Out0I.D.word = Out0I.Q.word = 0;
        Out1R.D.word = Out1R.Q.word = 0;    Out1I.D.word = Out1I.Q.word = 0;
    }
    else
    {
        // -----------------------------------------------------------------------
        // Multiplexers: (Not shown here) Circuit has option for test signal input
        // -----------------------------------------------------------------------

        // ------------------------------------------------------
        // Format conversion: unsigned to two's complement signed
        // ------------------------------------------------------
        Out0R.D.word = (signed)(DSelIn0R.w10 - 512);
        Out0I.D.word = (signed)(DSelIn0I.w10 - 512);
        Out1R.D.word = (signed)(DSelIn1R.w10 - 512);
        Out1I.D.word = (signed)(DSelIn1I.w10 - 512);

        // ----------------
        // Output Registers
        // ----------------
        DSelOut0R->word = DSelPathSel.b0? Out0R.Q.w10:0;  Out0R.Q = Out0R.D;
        DSelOut0I->word = DSelPathSel.b0? Out0I.Q.w10:0;  Out0I.Q = Out0I.D;
        DSelOut1R->word = DSelPathSel.b1? Out1R.Q.w10:0;  Out1R.Q = Out1R.D;
        DSelOut1I->word = DSelPathSel.b1? Out1I.Q.w10:0;  Out1I.Q = Out1I.D;
    }
}
// end of Mojave_DSel()

// ================================================================================================
// FUNCTION  : Mojave_DCX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute the ADC DC offset and subtract from the samples
// Clock     : 40 MHz
// Parameters: n         -- path number (model input, specifies instance, -1=RESET)
//             DCXIn     -- data input from data select block, 10 bits (circuit input)
//             DCXOut    -- data output to Downmixer, 10 bits (circuit output)
//             DCXClear  -- reinitialize DCX and clear the output (infinite pole)
// ================================================================================================
void Mojave_DCX(int n, wiWORD DCXIn, wiWORD *DCXOut, wiBoolean DCXClear, Mojave_aSigState *aSigIn) 
{
    static wiReg z[4]; // filter memory element

    if(n<0) // Endpoint Processing
    {
        if(n==-1) // RESET
        {
            z[0].D.word = 256*Register.DCXAcc0RH.w5 + Register.DCXAcc0RL.w8;
            z[1].D.word = 256*Register.DCXAcc0IH.w5 + Register.DCXAcc0IL.w8;
            z[2].D.word = 256*Register.DCXAcc1RH.w5 + Register.DCXAcc1RL.w8;
            z[3].D.word = 256*Register.DCXAcc1IH.w5 + Register.DCXAcc1IL.w8;
        }
        else if(n==-2) // load register from accumulator (continuous in RTL, end-of-RX in WiSE)
        {
            Register.DCXAcc0RH.word =            z[0].D.word >> 8;
            Register.DCXAcc0RL.word = ((unsigned)z[0].D.word) & 0xFF;
            Register.DCXAcc0IH.word =            z[1].D.word >> 8;
            Register.DCXAcc0IL.word = ((unsigned)z[1].D.word) & 0xFF;
            Register.DCXAcc1RH.word =            z[2].D.word >> 8;
            Register.DCXAcc1RL.word = ((unsigned)z[2].D.word) & 0xFF;
            Register.DCXAcc1IH.word =            z[3].D.word >> 8;
            Register.DCXAcc1IL.word = ((unsigned)z[3].D.word) & 0xFF;
        }
        if(DCXOut) DCXOut->word = 0;
    }
    else
    {
       z[n].Q = z[n].D;   // clock register

        if(DCXClear)
        {
            if(!Register.DCXHoldAcc.b0)
                z[n].D.word  = Register.DCXFastLoad.b0? (DCXIn.w10 << (2+Register.DCXShift.w3)) : 0;
            DCXOut->word = 0;
        }
        else
        {
            wiWORD a, b, c, d;

            b.word = z[n].Q.w13 >> (2+Register.DCXShift.w3);
            c.word = DCXIn.w10 - b.w11;
            a.word = z[n].Q.w13 + (Register.DCXHoldAcc.b0? 0:c.w12);
            z[n].D.word = (abs(a.w14)<4096)? a.w13 : (a.w14<0)? -4095:4095;
            d.word = (abs(c.w12)<512)? c.w10 : (c.w12<0)? -511:511;
            DCXOut->word = Register.DCXSelect.b0? d.w10 : DCXIn.w10;
        }
    }
}
// end of Mojave_DCX()

// ================================================================================================
// FUNCTION  : Mojave_Downmix()
// ------------------------------------------------------------------------------------------------
// Purpose   : Frequency shift the receive channel by -10 MHz
// Clock     : 40 MHz
// Parameters: n           -- path number (model input, specifies module instance)
//             DownmixInR  -- mixer input, real, 10 bits (circuit input)
//             DownmixInI  -- mixer input, imag, 10 bits (circuit input)
//             DownmixOutR -- mixer output, real,10 bits (circuit output)
//             DownmixOutI -- mixer output, imag,10 bits (circuit output)
//             Rst40B      -- global reset
//             pReg        -- pointer to registers
// ================================================================================================
void Mojave_Downmix(int n, wiWORD DownmixInR, wiWORD DownmixInI, wiWORD *DownmixOutR, wiWORD *DownmixOutI, 
                            wiBoolean Rst40B, MojaveRegisters *pReg) 
{
    static wiReg InR[2], InI[2]; // input register
    static int   c[2] = {0};     // angle counter, (4) 90deg steps

    // -----
    // Reset
    // -----
    if(!Rst40B)   
    {
        c[n] = RX.DownmixPhase;  // reset the mixer phase
        InR[0].D.word = InR[1].D.word = InI[0].D.word = InI[1].D.word = 0;
        DownmixOutR->word = 0;
        DownmixOutI->word = 0;
    }
    else
    {
        if(pReg->DownmixOff.b0 && !pReg->DownmixConj.b0) // no mixer or conjugation
        {
            DownmixOutR->word = DownmixInR.w10;
            DownmixOutI->word = DownmixInI.w10;
        }
        else // run either mixer, conjugation, or both (use input register)
        {
            wiWORD yR, yI; // computed mixer output words

            // ----------------------------------
            // Input Registers - Limit [-511,511]
            // ----------------------------------
            ClockRegister(InR[n]); InR[n].D.word = max(-511, DownmixInR.w10);
            ClockRegister(InI[n]); InI[n].D.word = max(-511, DownmixInI.w10);

            // -------------
            // 2-bit counter
            // -------------
            c[n] = pReg->DownmixOff.b0? 0 : (pReg->DownmixUp.b0? (c[n]+3)%4 : (c[n]+1)%4);

            // -----
            // Mixer
            // -----
            switch(c[n])
            {
                case 0: yR.word = InR[n].Q.w10; yI.word = InI[n].Q.w10; break;
                case 1: yR.word = InI[n].Q.w10; yI.word =-InR[n].Q.w10; break;
                case 2: yR.word =-InR[n].Q.w10; yI.word =-InI[n].Q.w10; break;
                case 3: yR.word =-InI[n].Q.w10; yI.word = InR[n].Q.w10; break;
            }
            // -----------------
            // Complex Conjugate
            // -----------------
            if(pReg->DownmixConj.b0) yR.word = -yR.word;

            DownmixOutR->word = yR.w10;
            DownmixOutI->word = yI.w10;
        }
    }
}
// end of Mojave_Downmix()

// ================================================================================================
// FUNCTION  : aMojave_DownsamFIR()
// ------------------------------------------------------------------------------------------------
// Purpose   : Low pass filter and down sample the signal by 2x
// Clock     : Input @ 40 MHz, Filter and Output at 20 MHz
// Parameters: n          -- filter number (model input, specifies module instance)
//             DownsamIn  -- input, (10 bit circuit input)
//             DownsamOut -- output (12 bit circuit output)
//             Rst20B     -- input, global reset (circuit input)
//             Rst40B     -- input, global reset (circuit input)
// ================================================================================================
void aMojave_DownsamFIR(int n, wiWORD  DownsamIn, wiWORD *DownsamOut, wiBoolean Rst20B, wiBoolean Rst40B) 
{
    const  wiWORD b[11]={3, 0, -7, 0, 20, 32, 20, 0, -7, 0, 3};

    static int    c[4] = {0,0,0,0}; // counters for clock phase (model only)
    static wiReg  x[4][21];  // FIR delay line
    static wiReg  In[4][4];  // input registers
    static wiReg  Out[4];    // output registers

    int i, s;
    wiWORD a; // accumulator

    // --------------
    // 40 MHz Circuit
    // --------------
    if(!Rst40B) {
        c[n] = 0; // reset the output phase counter
    }
    else
    {
        // -------------------------------------------------------------------------
        // Count 20 MHz clock periods in each 40 MHz period
        // Used to model the relationship between registers in the two clock domains
        // -------------------------------------------------------------------------
        c[n] = (c[n]+1)                 % 2; // phase of the T40=1/40MHz period in the T20=1/20 MHz period
        s    = (2-c[n]+RX.DownsamPhase) % 2; // enumerate the selected input register (1 of 2)
        In[n][s].Q = In[n][s].D; In[n][s].D.word = DownsamIn.w10;
    }
    // --------------
    // 20 MHz Circuit
    // --------------
    if(c[n]==RX.DownsamPhase)
    {
        if(!Rst20B)
        {
            for(i=0; i<11; i++) REG_CLR(x[n][i]);
            REG_CLR(Out[n]);
            DownsamOut->word = 0;
        }
        else
        {
            // --------------------
            // Clock the delay line
            // --------------------
            for(i=0; i<2; i++) { x[n][i].Q = x[n][i].D; x[n][i].D = In[n][i].Q;   }
            for(; i<11; i++)   { x[n][i].Q = x[n][i].D; x[n][i].D =  x[n][i-2].Q; }

            // -----------------------
            // FIR Filter Calculations
            // -----------------------
            a.word = 0;
            for(i=0; i<11; i++) 
                a.w17 = a.w17 + (b[i].w7 * x[n][i].Q.w10);
            Out[n].D.word = a.w17 /32; // 060109: changed from shift to divider
        }
    }
    // -------------------------
    // Output Register (20 MHz)
    // -------------------------
    if(c[n]==RX.DownsamPhase) Out[n].Q = Out[n].D;
    DownsamOut->word = Out[n].Q.w12;
}
// end of aMojave_DownsamFIR()

// ================================================================================================
// FUNCTION  : aMojave_Downsam()
// ------------------------------------------------------------------------------------------------
// Purpose   : Low pass filter and down sample the signal by 2x
// Parameters: n           -- path number (model input, specifies module instance)
//             DownsamInR  -- input, real, 10 bits (circuit input @ 40MHz)
//             DownsamInI  -- input, imag, 10 bits (circuit input @ 40MHz)
//             DownsamOutR -- output, real,12 bits (circuit output@ 40MHz)
//             DownsamOutI -- output, imag,12 bits (circuit output@ 40MHz)
//             Rst20B      -- input, global reset  (circuit input @ 20MHz)
//             Rst40B      -- input, global reset  (circuit input @ 40MHz)
// ================================================================================================
void aMojave_Downsam(int n, wiWORD  DownsamInR,  wiWORD  DownsamInI, wiWORD *DownsamOutR, wiWORD *DownsamOutI,
                    wiBoolean Rst20B, wiBoolean Rst40B) 
{
    aMojave_DownsamFIR(n+0, DownsamInR, DownsamOutR, Rst20B, Rst40B);
    aMojave_DownsamFIR(n+2, DownsamInI, DownsamOutI, Rst20B, Rst40B);
}
// end of aMojave_Downsam()

// ================================================================================================
// FUNCTION  : aMojave_ChEst_Core()
// ------------------------------------------------------------------------------------------------
// Purpose   : Core functional block for the channel estimator (ChEst)
// Parameters: n            -- Circuit instance (model input)
//             ChEstInR     -- Signal input, real component, 10 bit (circuit input)
//             ChEstInI     -- Signal input, imag component, 10 bit (circuit input)
//             ChEstOutNoise-- Channel noise power estimate, 4 bit (circuit output)
//             ChEstOutShift-- Noise shift for EqD, 4 bit (circuit output)
//             ChEstOutR    -- Channel response, real, 10 bit (circuit output)
//             ChEstOutI    -- Channel response, imag, 10 bit (circuit output)
//             ChEstOutP    -- Channel power, 20 bit (circuit output)
// ================================================================================================
void aMojave_ChEst_Core(int n, wiWORD ChEstInR[], wiWORD ChEstInI[], wiUWORD ChEstMinShift, wiUWORD *ChEstOutNoise,
                       wiUWORD *ChEstOutShift, wiWORD *ChEstOutR, wiWORD *ChEstOutI, wiWORD *ChEstOutP )
                
{
    const int L[64]={0,0,0,0,0,0,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,1,1,-1,-1,1,1,-1,1,-1,1,1,1,1,0,
                    1,-1,-1,1,1,-1,1,-1,1,-1,-1,-1,-1,-1,1,1,-1,-1,1,-1,1,-1,1,1,1,1,0,0,0,0,0};
    int k;
    wiWORD  d, z;
    wiUWORD PwrAcm, Shift, p;

    // -----------------------------------
    // Compute channel and power estimates
    // -----------------------------------
    for(k=0; k<64; k++) 
    {
        ChEstOutR[k].word = L[k] * ((ChEstInR[k].w10 + ChEstInR[k+64].w10) >> 1);
        ChEstOutI[k].word = L[k] * ((ChEstInI[k].w10 + ChEstInI[k+64].w10) >> 1);

        ChEstOutP[k].word = (ChEstOutR[k].w10*ChEstOutR[k].w10) + (ChEstOutI[k].w10*ChEstOutI[k].w10);
    }
    // ---------------------------------------------------
    // EVM Channel Estimate (Diagnostic: NOT PART OF ASIC)
    // ---------------------------------------------------
    if(RX.EVM.Enabled)
    {
        for(k=0; k<64; k++) 
        {
            RX.EVM.h[n][k].Real = (double)L[k] * (ChEstInR[k].w10 + ChEstInR[k+64].w10) / 2.0;
            RX.EVM.h[n][k].Imag = (double)L[k] * (ChEstInI[k].w10 + ChEstInI[k+64].w10) / 2.0;
        }
    }
    // -----------------------
    // Compute noise estimates
    // -----------------------
    PwrAcm.word = 0;

    for(k=0; k<64; k++) 
    {
        d.word = (ChEstInR[k].w10 - ChEstInR[k+64].w10) >> 2;
        z.word = (abs(d.w9)>63)? ((d.w9<0)? -63:63):d.w7;
        p.word = z.w7 * z.w7;

        if(L[k]) PwrAcm.word = PwrAcm.w15 + p.w13;
    }
    // --------------------------------------------
    // Priority encoder to give floor(log2(PwrAcm))
    // --------------------------------------------
         if(PwrAcm.b15) Shift.word = 15;
    else if(PwrAcm.b14) Shift.word = 14;
    else if(PwrAcm.b13) Shift.word = 13;
    else if(PwrAcm.b12) Shift.word = 12;
    else if(PwrAcm.b11) Shift.word = 11;
    else if(PwrAcm.b10) Shift.word = 10;
    else if(PwrAcm.b9 ) Shift.word =  9;
    else if(PwrAcm.b8 ) Shift.word =  8;
    else if(PwrAcm.b7 ) Shift.word =  7;
    else if(PwrAcm.b6 ) Shift.word =  6;
    else if(PwrAcm.b5 ) Shift.word =  5;
    else if(PwrAcm.b4 ) Shift.word =  4;
    else if(PwrAcm.b3 ) Shift.word =  3;
    else if(PwrAcm.b2 ) Shift.word =  2;
    else if(PwrAcm.b1 ) Shift.word =  1;
    else                Shift.word =  0;

    // -------------------
    // Limit minimum shift
    // -------------------
    if(Shift.w4 > ChEstMinShift.w4)
    {
        ChEstOutShift->word = Shift.w4;
    }
    else
    {
        ChEstOutShift->word = ChEstMinShift.w4;
        if((Register.PathSel.w2>>n)&1 && Shift.w4!=ChEstMinShift.w4) RX.EventFlag.b9 = 1;
    }
    ChEstOutNoise->word = Shift.w4;
}
// end of aMojave_ChEst_Core()

// ================================================================================================
// FUNCTION  : aMojave_ChEst()
// ------------------------------------------------------------------------------------------------
// Purpose   : Estimate channel response and signal/noise power
// Parameters: ChEstIn0R     -- Signal, path 0, real component, 10 bit (circuit input)
//             ChEstIn0I     -- Signal, path 0, imag component, 10 bit (circuit input)
//             ChEstIn1R     -- Signal, path 1, real component, 10 bit (circuit input)
//             ChEstIn1I     -- Signal, path 1, imag component, 10 bit (circuit input)
//             ChEstOutNoise0-- Noise power estimate, path 0,    4 bit (circuit output)
//             ChEstOutNoise1-- Noise power estimate, path 1,    4 bit (circuit output)
//             ChEstOutShift0-- Shift for EqD, path 0,           4 bit (circuit output)
//             ChEstOutShift1-- Shift for EqD, path 1,           4 bit (circuit output)
//             ChEstOut0R    -- Channel response, path 0, real, 10 bit (circuit output)
//             ChEstOut0I    -- Channel response, path 0, imag, 10 bit (circuit output)
//             ChEstOutP0    -- Channel power,    path 0,       19 bit (circuit output)
//             ChEstOut1R    -- Channel response, path 1, real, 10 bit (circuit output)
//             ChEstOut1I    -- Channel response, path 1, imag, 10 bit (circuit output)
//             ChEstOutP1    -- Channel power,    path 1,       19 bit (circuit output)
//             pReg          -- Pointer to modeled register file (model input)
// ================================================================================================
void aMojave_ChEst(wiWORD ChEstIn0R[], wiWORD ChEstIn0I[], wiWORD ChEstIn1R[], wiWORD ChEstIn1I[],
                        wiUWORD *ChEstOutNoise0, wiUWORD *ChEstOutNoise1, wiUWORD *ChEstOutShift0, wiUWORD *ChEstOutShift1, 
                        wiWORD *ChEstOut0R, wiWORD *ChEstOut0I, wiWORD *ChEstOutP0, 
                        wiWORD *ChEstOut1R, wiWORD *ChEstOut1I, wiWORD *ChEstOutP1, MojaveRegisters *pReg)
                
{
    aMojave_ChEst_Core(0, ChEstIn0R, ChEstIn0I, pReg->MinShift, ChEstOutNoise0, ChEstOutShift0, ChEstOut0R, ChEstOut0I, ChEstOutP0);
    aMojave_ChEst_Core(1, ChEstIn1R, ChEstIn1I, pReg->MinShift, ChEstOutNoise1, ChEstOutShift1, ChEstOut1R, ChEstOut1I, ChEstOutP1);
}
// end of aMojave_ChEst()

// ================================================================================================
// FUNCTION   : aMojave_EqD()
// ------------------------------------------------------------------------------------------------
// Purpose    : Combined equalizer and soft demapper
// Parameters : EqDPathSel  -- Select active digital paths (2 bit input)
//              EqDRate     -- Data rate selection (4 bit input)
//              EqDShiftIn0 -- Log-2 noise estimate, path 0 (4 bit input)
//              EqDShiftIn1 -- Log-2 noise estimate, path 1 (4 bit input)
//              EqDChEst0R  -- Channel estimate, path 0, real component (10 bit input)
//              EqDChEst0I  -- Channel estimate, path 0, imag component (10 bit input)
//              EqDChEst1R  -- Channel estimate, path 1, real component (10 bit input)
//              EqDChEst1I  -- Channel estimate, path 1, imag component (10 bit input)
//              EqDChEstP0  -- Channel power estimate, path 0 (19 bit input)
//              EqDChEstP1  -- Channel power estimate, path 1 (19 bit input)
//              EqDIn0R     -- Data input, path 0, real component (10 bit input)
//              EqDIn0I     -- Data input, path 0, imag component (10 bit input)
//              EqDIn1R     -- Data input, path 1, real component (10 bit input)
//              EqDIn1I     -- Data input, path 1, imag component (10 bit input)
//              EqDOutH2    -- Channel power estimate output to PLL (18 bit output)
//              EqDOutZR    -- Channel estimate to PLL, real component (18 bit output)
//              EqDOutZI    -- Channel estimate to PLL, imag component (18 bit output)
//              EqDOutDR    -- Hard decision output to PLL, real component (5 bit output)
//              EqDOutDI    -- Hard decision output to PLL, imag component (5 bit output)
//              k0          -- Sample index offset (model input)
//              EqDOut      -- Soft decision output (serialized for model, 5 bit output)
// ================================================================================================
void aMojave_EqD(wiUWORD EqDPathSel, wiUWORD EqDRate, wiUWORD EqDShiftIn0, wiUWORD EqDShiftIn1,
                wiWORD EqDChEst0R[], wiWORD EqDChEst0I[], wiWORD EqDChEst1R[], wiWORD EqDChEst1I[],
                wiWORD EqDChEstP0[], wiWORD EqDChEstP1[],
                wiWORD EqDIn0R[], wiWORD EqDIn0I[], wiWORD EqDIn1R[], wiWORD EqDIn1I[],
                wiWORD *EqDOutH2, wiWORD *EqDOutZR, wiWORD *EqDOutZI, 
                wiWORD *EqDOutDR, wiWORD *EqDOutDI, int k0, wiUWORD *EqDOut)
{
    wiWORD a1[2], a2[2];
    wiWORD m1[2], m2[2], m3[2], m4[2];
    wiWORD b[8], c[8], d[8];
    wiWORD Xr[2], Xi[2], Hr[2], Hi[2];
    int k, i, j, s, n;
    wiUWORD AdrR, AdrI;

    wiWORD dR, dI, pdR, pdI; wiUWORD H2, pH2;

    // --------------------------------------------
    // Select constellation dependent normalization
    // --------------------------------------------
    switch(EqDRate.w4)
    {
        case 0xD: case 0xF: s = 0; break;
        case 0x5: case 0x7: s = 1; break;
        case 0x9: case 0xB: s = 2; break;
        case 0x1: case 0x3: s = 3; break;
        case 0xA:           s = 4; break;
        default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
    }
    switch(EqDPathSel.w2)
    {
        case 1: s = s + EqDShiftIn0.w4; break;
        case 2: s = s + EqDShiftIn1.w4; break;
        case 3: s = s + ((EqDShiftIn0.w4 + EqDShiftIn1.w4) >> 1); break;
        default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
    }
    for(j=k=0; k<64; k++)
    {
        if(subcarrier_type[k]) //--------data/pilot subcarrier processing block
        {
            Xr[0].word = EqDIn0R[k+k0].w10;
            Xi[0].word = EqDIn0I[k+k0].w10;
            Hr[0].word = EqDChEst0R[k].w10;
            Hi[0].word = EqDChEst0I[k].w10;

            m1[0].word = Xr[0].w10 * Hr[0].w10;
            m2[0].word = Xr[0].w10 * Hi[0].w10;
            m3[0].word = Xi[0].w10 * Hr[0].w10;
            m4[0].word = Xi[0].w10 * Hi[0].w10;
            a1[0].word = m1[0].w19 + m4[0].w19;
            a2[0].word = m3[0].w19 - m2[0].w19;

            Xr[1].word = EqDIn1R[k+k0].w10;
            Xi[1].word = EqDIn1I[k+k0].w10;
            Hr[1].word = EqDChEst1R[k].w10;
            Hi[1].word = EqDChEst1I[k].w10;

            m1[1].word = Xr[1].w10 * Hr[1].w10;
            m2[1].word = Xr[1].w10 * Hi[1].w10;
            m3[1].word = Xi[1].w10 * Hr[1].w10;
            m4[1].word = Xi[1].w10 * Hi[1].w10;
            a1[1].word = m1[1].w19 + m4[1].w19;
            a2[1].word = m3[1].w19 - m2[1].w19;

            // ---------------------------------
            // Combine the two digital path data
            // ---------------------------------
            dR.word = (EqDPathSel.b0?         a1[0].w20:0) + (EqDPathSel.b1?         a1[1].w20:0);
            dI.word = (EqDPathSel.b0?         a2[0].w20:0) + (EqDPathSel.b1?         a2[1].w20:0);
            H2.word = (EqDPathSel.b0? EqDChEstP0[k].w20:0) + (EqDPathSel.b1? EqDChEstP1[k].w20:0);

            // --------------------------
            // Form the output to the PLL
            // --------------------------
            n = (EqDPathSel.w2==3)? 1:0; // right shift by 1 for two-path reception
            pdR.word = dR.w21 >> n;
            pdI.word = dI.w21 >> n;
            pH2.word = H2.w21 >> n;

            EqDOutH2[k+k0].word = (pH2.word==pH2.w18)? pH2.w18 : (pH2.word<0)? -131071:131071;
            EqDOutZR[k+k0].word = (pdR.word==pdR.w18)? pdR.w18 : (pdR.word<0)? -131071:131071;
            EqDOutZI[k+k0].word = (pdI.word==pdI.w18)? pdI.w18 : (pdI.word<0)? -131071:131071;
            if(pH2.word!=pH2.w18 || pdR.word!=pdR.w18 || pdI.word!=pdI.w18) RX.EventFlag.b12=1;

            // ------------------
            // Apply 13/2 scaling
            // ------------------
            dR.word = (dR.w21 * 13) >> 1;
            dI.word = (dI.w21 * 13) >> 1;
            
            // ------------------------------------------------
            // Limiter: Reduce dynamic range from 24 to 23 bits
            // ------------------------------------------------
            if(abs(dR.w24)>4194303) {dR.w24 = (dR.w24<0)? -4194303:4194303;}
            if(abs(dI.w24)>4194303) {dI.w24 = (dI.w24<0)? -4194303:4194303;}
            if(abs(H2.w24)>4194303) {H2.w21 = 4194303;                     } 

            // ----------------------------
            // Compute raw soft information
            // ----------------------------
            b[0].word = dR.w23;                    b[4].word = dI.w23;
            b[1].word = 4*H2.w21 - abs(b[0].w23);  b[5].word = 4*H2.w21 - abs(b[4].w23);
            b[2].word = 2*H2.w21 - abs(b[1].w24);  b[6].word = 2*H2.w21 - abs(b[5].w24);
            b[3].word = 1*H2.w21 - abs(b[2].w25);  b[7].word = 1*H2.w21 - abs(b[6].w25);

            // -------------------------------------------
            // Normalize and quantize the soft information
            // -------------------------------------------
            for(i=0; i<8; i++)
            {
                c[i].word = b[i].w26 >> s;
                d[i].word = (abs(c[i].w26)>15)? ((c[i].w26<0)? -15:15) : c[i].w26; 
            }
            // ---------------------------------------------
            // Compute hard decisions for data constellation
            // ---------------------------------------------
            switch(EqDRate.w4)
            {
                case 0xD: case 0xF: 
                case 0x5: case 0x7:
                    AdrR.w4 = (b[0].word>=0)? 1:0;
                    AdrI.w4 = (b[4].word>=0)? 1:0;
                    break;
                case 0x9: case 0xB:
                    AdrR.w4 = ((b[0].word>=0)? 2:0) | ((b[1].word>=0)? 1:0);
                    AdrI.w4 = ((b[4].word>=0)? 2:0) | ((b[5].word>=0)? 1:0);
                    break;
                case 0x1: case 0x3:
                    AdrR.w4 = ((b[0].word>=0)? 4:0) | ((b[1].word>=0)? 2:0) | ((b[2].word>=0)? 1:0);
                    AdrI.w4 = ((b[4].word>=0)? 4:0) | ((b[5].word>=0)? 2:0) | ((b[6].word>=0)? 1:0);
                    break;
                case 0xA:
                    AdrR.word = ((b[0].word>=0)? 8:0) | ((b[1].word>=0)? 4:0) | ((b[2].word>=0)? 2:0) | ((b[3].word>=0)? 1:0);
                    AdrI.word = ((b[4].word>=0)? 8:0) | ((b[5].word>=0)? 4:0) | ((b[6].word>=0)? 2:0) | ((b[7].word>=0)? 1:0);
                    break;
            }
            // -------------
            // Hard decision
            // -------------
            if(subcarrier_type[k]==1)
            {
                AdrR.word = (EqDRate.w4<<4) | AdrR.w4;
                AdrI.word = (EqDRate.w4<<4) | AdrI.w4;
            }
            else
            {
                AdrR.word =  0xD0 | ((b[0].word>=0)? 1:0);
                AdrI.word =  0xD0 | AdrI.w4;
            }
            EqDOutDR[k+k0].word = RX.DemodROM[AdrR.w8].word;
            EqDOutDI[k+k0].word = (EqDRate.w4==0xD || EqDRate.w4==0xF || subcarrier_type[k]==2)? 0 : RX.DemodROM[AdrI.w8].word;
            
            // --------------------------
            // Block low magnitude points
            // --------------------------
            if(subcarrier_type[k]==1)
            {
                if(EqDRate.w4==0x01 || EqDRate.w4==0x03)
                {
                    if((abs(EqDOutDR[k+k0].w5)<=1) && (abs(EqDOutDI[k+k0].w5)<=1))
                        EqDOutDR[k+k0].word = EqDOutDI[k+k0].word = 0;
                }
                else if(EqDRate.w4 == 0xA)
                {
                    if((abs(EqDOutDR[k+k0].w5)<=4) && (abs(EqDOutDI[k+k0].w5)<=4))
                        EqDOutDR[k+k0].word = EqDOutDI[k+k0].word = 0;
                }
            }
            // -------------------------------------------
            // Skip subsequence processing for pilot tones
            // -------------------------------------------
            if(subcarrier_type[k]==2) continue;

            // --------------------------------------------------------
            // Select relavent soft information and convert to unsigned
            // --------------------------------------------------------
            switch(EqDRate.w4)
            {            
                case 0xD: case 0xF:
                    EqDOut[j++].word = d[0].w5 + 16;
                    break;

                case 0x5: case 0x7:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    break;

                case 0x9: case 0xB:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[1].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    EqDOut[j++].word = d[5].w5 + 16;
                    break;

                case 0x1: case 0x3:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[1].w5 + 16;
                    EqDOut[j++].word = d[2].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    EqDOut[j++].word = d[5].w5 + 16;
                    EqDOut[j++].word = d[6].w5 + 16;
                    break;
            
                case 0xA:
                    EqDOut[j++].word = d[0].w5 + 16;
                    EqDOut[j++].word = d[1].w5 + 16;
                    EqDOut[j++].word = d[2].w5 + 16;
                    EqDOut[j++].word = d[3].w5 + 16;
                    EqDOut[j++].word = d[4].w5 + 16;
                    EqDOut[j++].word = d[5].w5 + 16;
                    EqDOut[j++].word = d[6].w5 + 16;
                    EqDOut[j++].word = d[7].w5 + 16;
                    break;
            }
        }    // end data subcarrier processing block
        else // processing for non-data subcarrier
        {
            EqDOutZR[k+k0].word = 0;
            EqDOutZI[k+k0].word = 0;
            EqDOutH2[k+k0].word = 0;
        }
    }
}
// end of aMojave_EqD()

// ================================================================================================
// FUNCTION   : aMojave_Depuncture()
// ------------------------------------------------------------------------------------------------
// Purpose    : Received soft-information depuncturer
// Parameters : N       -- Number of output pairs/data bits (model input)
//            : DepIn   -- Data input, aka {DepunIn0,DepunIn1}, 5 bits (circuit input)
//            : DepOut0 -- Data output 1/2, 5 bits (circuit output)
//            : DepOut1 -- Data output 2/2, 5 bits (circuit output)
//            : DepRate -- Data rate parameter, 4 bits (circuit input)
// ================================================================================================
void aMojave_Depuncture(int Nbits, wiUWORD DepIn[], wiUWORD DepOut0[], wiUWORD DepOut1[], wiUWORD DepRate)
{
    int k, M, *pA, *pB;

    // ----------------------------------
    // Select the (de-)puncturing pattern
    // ----------------------------------
    switch(DepRate.w4)
    {
        case 0xD: case 0x5: case 0x9:                     pA=pA_12; pB=pB_12; M=1;  break;
        case 0xF: case 0x7: case 0xB: case 0x3: case 0xA: pA=pA_34; pB=pB_34; M=9;  break;
        case 0x1:                                         pA=pA_23; pB=pB_23; M=6;  break;
        default: STATUS(WI_ERROR_UNDEFINED_CASE); return;
    }
    for(k=0; k<Nbits; k++)
    {
        DepOut0[k].word = pA[k%M]? (DepIn++)->w5 : 16;
        DepOut1[k].word = pB[k%M]? (DepIn++)->w5 : 16;
    }
}
// end of aMojave_Depuncture()

// ================================================================================================
// FUNCTION   : aMojave_VitDec()
// ------------------------------------------------------------------------------------------------
// Purpose    : Soft-input Viterbi decoder (trace-back path management)
// Parameters : Nbits   -- Number of data bits (model input)
//              VitDetLLR0 -- LLR for encoder output bit A, 5 bit (circuit input)
//              VitDetLLR1 -- LLR for encoder output bit B, 5 bit (circuit input)
//              VitDetOut  -- Decoded output data, 1 bit (circuit output)
//              pRX        -- Pointer to Mojave RX data structure
// ================================================================================================
void aMojave_VitDec(int Nbits, wiUWORD VitDetLLR0[], wiUWORD VitDetLLR1[], wiUWORD VitDetOut[], Mojave_RX_State *pRX)
{
    static const sA0[64]={0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0};
    static const sA1[64]={1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1};
    static const sB0[64]={0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0};
    static const sB1[64]={1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1};

    wiUWORD Mk[64], Mk_1[64];
    wiUWORD LLR_A, LLR_B, _LLR_A, _LLR_B, b0, b1, m0, m1, t;

    int i, k;
    int multiple_min;
    int S, S0, S1, Sx;
    int dec_st, dec_bit;
    unsigned *DB_u, *DB_l; // array of decision bits (upper, lower 32 of 64 bits)

    // ---------------------------------------------
    // Allocated Memory for Flat Path History Buffer
    // ---------------------------------------------
    DB_l = (unsigned *)wiCalloc(Nbits+128, sizeof(unsigned));
    DB_u = (unsigned *)wiCalloc(Nbits+128, sizeof(unsigned));
    if(!DB_l || !DB_u) {
        WICHK(WI_ERROR_MEMORY_ALLOCATION);
        exit(1);
    }
    // ----------------------
    // Initialize the metrics
    // ----------------------
    t.bit = 0;               // clear the normalization flag
    Mk_1[0].word = 0x000;    // initial state is state 0
    for(S=1; S<64; S++)      // for all other states,
        Mk_1[S].word = 0x0FF; // ...bias to a less favorable metric

    // -------------------------
    // Apply the Viterbi decoder
    // -------------------------
    for(k=0; k<(Nbits+128); k++)
    {
        if (k<Nbits)
        {
            // ----------------
            // Compute +/- LLRs
            // ----------------
            LLR_A.word = VitDetLLR0[k].w5;
            LLR_B.word = VitDetLLR1[k].w5;
        
            _LLR_A.w5 = (~LLR_A.w5 + 1)|((~LLR_A.w5)&0x10);
            _LLR_B.w5 = (~LLR_B.w5 + 1)|((~LLR_B.w5)&0x10);

            // -------------------------------------------------
            // Loop through state-by-state (parallel) operations
            // -------------------------------------------------
            for(S=0; S<64; S++)
            {
                S0 = (S << 1) & 0x3F;
                S1 = S0 | 0x01;

                // -------------------------------
                // Add the path and branch metrics
                // -------------------------------
                b0.word = (sA0[S]? _LLR_A.w5:LLR_A.w5) + (sB0[S]? _LLR_B.w5:LLR_B.w5);
                b1.word = (sA1[S]? _LLR_A.w5:LLR_A.w5) + (sB1[S]? _LLR_B.w5:LLR_B.w5);

                m0.word = Mk_1[S0].w10 + b0.w6;
                m1.word = Mk_1[S1].w10 + b1.w6;

                m0.word = m0.w10;
                m1.word = m1.w10;

                // ------------------
                // Compare and select
                // ------------------
                if(m0.word <= m1.word)
                {
                Mk[S]=m0; Sx=S0;
                }
                else
                {
                Mk[S]=m1; Sx=S1;
                }
                // ---------------
                // Save Decision Bits
                // ---------------
                if (S < 32)
                {
                    DB_l[k] = DB_l[k] | ((m0.word <= m1.word ? 0:1)<<S); 
                }
                else
                {
                    DB_u[k] = DB_u[k] | ((m0.word <= m1.word ? 0:1)<<(S-32));
                }
            }
            // ----------
            // Min Finder
            // ----------
            Sx = 0;
            multiple_min = 0;
            for(S=1; S<64; S++) 
            {
                if((Mk[S].w10&0x3FC)<(Mk[Sx].w10&0x3FC))
                {
                    Sx = S;
                    multiple_min = 0;
                }
                else if((Mk[S].w10&0x3FC)==(Mk[Sx].w10&0x3FC))
                {
                    Sx = S;
                    multiple_min = 1; // flag more than one minimum metric
                    pRX->EventFlag.b11 = 1;
                }
            }
            // --------------------------
            // Normalize the path metrics
            // --------------------------
            if(t.bit)
            {
                for(S=0; S<64; S++)
                    Mk[S].b9 = 0;
                pRX->EventFlag.b10 = 1;
            }
 
            t.bit = 1;
            for(S=0; S<64; S++)
                t.bit = t.bit & Mk[S].b9;

            // -------------------
            // Clock Metric registers
            // -------------------

            for(S=0; S<64; S++) 
                Mk_1[S]    = Mk[S];
        } 
        else
        {
            Sx = 0;
        }
        if (k>= 96)
        {
            if(k%32==0)
            {
                dec_st = Sx & 0x3F;
                for (i=0; i<96; i++)
                {
                    if (dec_st < 32)
                    {
                        dec_bit = DB_l[k-i] >> dec_st&0x1;
                    }
                    else
                    {
                        dec_bit = DB_u[k-i] >> (dec_st-32) & 0x1;
                    }
                    dec_st = (dec_st<<1 | dec_bit) & 0x3F;
                    if ((i>63) & (k-i>5))
                    {
                        VitDetOut[k-i-6].bit = dec_bit;
                    }
                }
            }
        }      
    }
    // -----------
    // Free Memory
    // -----------
    if(DB_l) wiFree(DB_l); DB_l = NULL;
    if(DB_u) wiFree(DB_u); DB_u = NULL;
}
// end of aMojave_VitDec()

// ================================================================================================
// FUNCTION   : aMojave_Descramble()
// ------------------------------------------------------------------------------------------------
// Purpose    : Received data descrambler
// Parameters : n        -- Number of data bits to process (model input)
//            : DescrIn  -- Data input,  1 bit (circuit input)
//            : DescrOut -- Data output, 1 bit (circuit output)
// ================================================================================================
void aMojave_Descramble(int n, wiUWORD DescrIn[], wiUWORD DescrOut[])
{
    wiWORD x, X; int k;
    wiUWORD *b = DescrIn;
    wiUWORD *a = DescrOut;

    // ------------------------------------
    // Get the shift register initial state
    // ------------------------------------
    {
        wiWORD X0, X1, X2, X3, X4, X5, X6;
    
        X0.bit = b[6].bit ^ b[2].bit;
        X1.bit = b[5].bit ^ b[1].bit;
        X2.bit = b[4].bit ^ b[0].bit;
        X3.bit = b[3].bit ^ X0.bit;
        X4.bit = b[2].bit ^ X1.bit;
        X5.bit = b[1].bit ^ X2.bit;
        X6.bit = b[0].bit ^ X3.bit;

        X.word = X0.bit | (X1.bit<<1) | (X2.bit<<2) | (X3.bit<<3) | (X4.bit<<4) | (X5.bit<<5) | (X6.bit<<6);
    }
    // ----------------------------------------------
    // Apply the scrambler to the rest of the message
    // ----------------------------------------------
    for(k=0; k<n; k++)
    {
        x.bit = X.b6 ^ X.b3;
        X.w7 = (X.w7 << 1) | x.bit;
        a[k].bit = b[k].bit ^ x.bit;
    }
}
// end of aMojave_Descramble()

// ================================================================================================
// FUNCTION : aMojave_DumpEVM()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus aMojave_DumpEVM()
{
    FILE *F;
    int i, k;

    F = fopen("EVM_x.out","wt");

    for(k=0; k<RX.EVM.N_SYM; k++)   {
        for(i=0; i<64; i++)
        {
            fprintf(F,"%8.4f %8.4f   %8.4f %8.4f\n",
                        RX.EVM.x[0][i][k].Real,RX.EVM.x[0][i][k].Imag,
                        RX.EVM.x[1][i][k].Real,RX.EVM.x[1][i][k].Imag);
        }
    }
    fclose(F);

    return WI_SUCCESS;
} 
// end of aMojave_DumpEVM()

// ================================================================================================
// FUNCTION : aMojave_EVM2()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus aMojave_EVM2(int du, int n)
{
    int aReal, aImag, mReal, mImag;
    double A, P; wiComplex e; double e2;
    int i, j;

    // -------------------------
    // Scaling for Constellation
    // -------------------------
    switch(RX.N_BPSC)
    {
        case 1: A = 13.0/13.0;   mReal= 1; mImag=0;     break;
        case 2: A = 13.0/ 9.0;   mReal= 1; mImag=mReal; break;
        case 4: A = 13.0/ 4.0;   mReal= 3; mImag=mReal; break;
        case 6: A = 13.0/ 2.0;   mReal= 7; mImag=mReal; break;
        case 8: A = 13.0/ 1.0;   mReal=15; mImag=mReal; break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    P = A * A;

    // -------------------
    // Equalize the Symbol
    // -------------------
    for(i=0; i<2; i++) 
    {
        if((Register.PathSel.w2>>i)&1)
        {
            wiComplex *g = RX.EVM.g[i];
            wiWORD    *uReal = RX.uReal[i] + du;
            wiWORD    *uImag = RX.uImag[i] + du;
            for(j=0; j<64; j++) 
            {
                RX.EVM.x[i][j][n].Real = A * ((g[j].Real * (double)uReal[j].word) - (g[j].Imag * (double)uImag[j].word));
                RX.EVM.x[i][j][n].Imag = A * ((g[j].Imag * (double)uReal[j].word) + (g[j].Real * (double)uImag[j].word));
            }
        }
    }
    // ----------------------
    // Hard Decision Decoding
    // ----------------------
    for(i=0; i<2; i++) 
    {
        if((Register.PathSel.w2>>i)&1)
        {
            for(j=0; j<64; j++)
            {
                if(subcarrier_type[j] == 1)
                {
                    aReal = (int)(1.0 + 2.0 * floor(RX.EVM.x[i][j][n].Real / 2.0));
                    aImag = (int)(1.0 + 2.0 * floor(RX.EVM.x[i][j][n].Imag / 2.0));

                    if(abs(aReal)>mReal) aReal=(aReal<0)? -mReal:mReal;
                    if(abs(aImag)>mImag) aImag=(aImag<0)? -mImag:mImag;
                    
                    e.Real = RX.EVM.x[i][j][n].Real - aReal;
                    e.Imag = RX.EVM.x[i][j][n].Imag - aImag;
                    e2 = ((e.Real * e.Real) + (e.Imag * e.Imag)) / P;

                    RX.EVM.Se2[i] += e2;
                    RX.EVM.cSe2[i][j] += e2;
                    RX.EVM.nSe2[i][RX.EVM.N_SYM] += e2;
                }
            }
        }
    }
    RX.EVM.N_SYM++;

    return WI_SUCCESS;
}
// end of aMojave_EVM2()

// ================================================================================================
// FUNCTION : aMojave_EVM1()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus aMojave_EVM1(void)
{
    const int maxN_SYM = 1 + (4095+3)/3;
    int i, j;
    double A, theta;

    // ------------------------------------
    // Allocate Memory for the EVM.x arrays
    // ------------------------------------
    for(i=0; i<2; i++) {
        for(j=0; j<64; j++) {
            if(!RX.EVM.x[i][j]) RX.EVM.x[i][j] = (wiComplex *)wiCalloc(maxN_SYM, sizeof(wiComplex));
            if(!RX.EVM.x[i][j]) return STATUS(WI_ERROR_UNDEFINED_CASE);
            RX.EVM.cSe2[i][j] = 0.0;
        }
        RX.EVM.Se2[i] = 0.0;
        RX.EVM.EVM[i] = 0.0;
        RX.EVM.EVMdB[i] = 0.0;
        for(j=0; j<1024; j++) RX.EVM.nSe2[i][j] = 0.0;
    }
    // ---------------
    // Build Equalizer
    // ---------------
    for(i=0; i<2; i++) {
        for(j=0; j<64; j++) {
            if((RX.EVM.h[i][j].Real==0.0) && (RX.EVM.h[i][j].Imag==0.0)) {
                RX.EVM.g[i][j].Real = RX.EVM.g[i][j].Imag = 0.0;
            }
            else {
                A = 1.0/sqrt(RX.EVM.h[i][j].Real*RX.EVM.h[i][j].Real + RX.EVM.h[i][j].Imag*RX.EVM.h[i][j].Imag);
                theta = -atan2(RX.EVM.h[i][j].Imag, RX.EVM.h[i][j].Real);

                RX.EVM.g[i][j].Real = A * cos(theta);
                RX.EVM.g[i][j].Imag = A * sin(theta);
            }
        }
    }
    // --------------------------
    // Equalize the SIGNAL Symbol
    // --------------------------
    for(i=0; i<2; i++) {
        if((Register.PathSel.w2>>i)&1)
        {
            wiComplex *g = RX.EVM.g[i];
            wiWORD    *uReal = RX.uReal[i] + RX.x_ofs + aMOJAVE_FFT_DELAY + aMOJAVE_PLL_DELAY + 144;
            wiWORD    *uImag = RX.uImag[i] + RX.x_ofs + aMOJAVE_FFT_DELAY + aMOJAVE_PLL_DELAY + 144;
            for(j=0; j<64; j++) {
                RX.EVM.x[i][j][0].Real = (g[j].Real * uReal[j].word) - (g[j].Imag * uImag[j].word);
                RX.EVM.x[i][j][0].Imag = (g[j].Imag * uReal[j].word) + (g[j].Real * uImag[j].word);
            }
        }
    }
    // ------------------------
    // Clear the Symbol Counter
    // ------------------------
    RX.EVM.N_SYM = 0;

    return WI_SUCCESS;
}
// end of aMojave_EVM1()

// ================================================================================================
// FUNCTION : Mojave_RX2_SIGNAL()
// ------------------------------------------------------------------------------------------------
// Purpose  : Implement OFDM RX2 modules and process the SIGNAL symbol
// ================================================================================================
wiStatus Mojave_RX2_SIGNAL(Mojave_aSigState *aSigOut)
{
    int m, i;
    unsigned int dx, dv, du;

    wiUWORD SignalRate = {0xD};

    wiWORD pReal[2][4], pImag[2][4];

    aSigOut->RxFault = 0; 

    RX.cCPP.word = 0; // initial carrier phase error is 0
    RX.N_PHY_RX_WR = 0;
    RX.dAdvDly.word = RX.d2AdvDly.word = 0;

    RX.cSP[0].word = RX.cCP[0].word = 0;
    
    if(!RX.xReal) return STATUS(WI_ERROR);

    RX.Fault = 0;
    RX.EVM.EVMdB[0] = RX.EVM.EVMdB[1] = 0.0;

    // ------------
    // Clear arrays
    // ------------
    memset(RX.AdvDly, 0, RX.Nx*sizeof(wiWORD));
    
    // --------------------
    // Estimate the channel
    // --------------------
    dx = RX.x_ofs;               // starting position of FFT
    dv = dx + aMOJAVE_FFT_DELAY; // 20MHz sample offset to FFT output
    du = dv + aMOJAVE_PLL_DELAY; // 20MHz sample offset to phase corrector output

    // ------------------------------------------------
    // Extract the FFT of the two long preamble symbols
    // ------------------------------------------------
    if(Register.PathSel.b0)
    {
        Mojave_FFT(1, NULL, NULL, RX.xReal[0]+dx,    RX.xImag[0]+dx,    NULL, NULL, RX.vReal[0]+dx,    RX.vImag[0]+dx);
        Mojave_FFT(1, NULL, NULL, RX.xReal[0]+dx+64, RX.xImag[0]+dx+64, NULL, NULL, RX.vReal[0]+dx+64, RX.vImag[0]+dx+64);
    }
    if(Register.PathSel.b1)
    {
        Mojave_FFT(1, NULL, NULL, RX.xReal[1]+dx,    RX.xImag[1]+dx,    NULL, NULL, RX.vReal[1]+dx,    RX.vImag[1]+dx);
        Mojave_FFT(1, NULL, NULL, RX.xReal[1]+dx+64, RX.xImag[1]+dx+64, NULL, NULL, RX.vReal[1]+dx+64, RX.vImag[1]+dx+64);
    }

    // ---------------------------------------
    // Phase Correction and Channel Estimation
    // ---------------------------------------
    aMojave_PhaseCorrect(Register.PathSel, 64, RX.cSP[0], RX.cCP[0], RX.cCPP, &(RX),
                       RX.vReal[0]+dv, RX.vImag[0]+dv, RX.vReal[1]+dv, RX.vImag[1]+dv,
                       RX.uReal[0]+du, RX.uImag[0]+du, RX.uReal[1]+du, RX.uImag[1]+du);

    aMojave_PhaseCorrect(Register.PathSel, 80, RX.cSP[0], RX.cCP[0], RX.cCPP, &(RX),
                       RX.vReal[0]+dv+64, RX.vImag[0]+dv+64, RX.vReal[1]+dv+64, RX.vImag[1]+dv+64,
                       RX.uReal[0]+du+64, RX.uImag[0]+du+64, RX.uReal[1]+du+64, RX.uImag[1]+du+64);

    aMojave_ChEst(RX.uReal[0]+du, RX.uImag[0]+du, RX.uReal[1]+du, RX.uImag[1]+du,
                &(Register.ChEstShift0), &(Register.ChEstShift1), &(RX.NoiseShift[0]), &(RX.NoiseShift[1]), 
                RX.hReal[0], RX.hImag[0], RX.h2[0], RX.hReal[1], RX.hImag[1], RX.h2[1], &(Register));

    // -------------------------------------------------
    // Compute vector offset and check for array overrun
    // -------------------------------------------------
    dx+=144; dv+=144; du+=144; // 144 = 128 (preamble) + 16 (SIGNAL Guard Interval)
    if(dx>RX.Nx) 
    {
        RX.Fault = 2;
        RX.EventFlag.b8 = 1;
        RX.RTL_Exception.xLimit_SIGNAL = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }
    RX.kFFT[0] = dx;
    if(Register.PathSel.b0) Mojave_FFT(1, NULL, NULL, RX.xReal[0]+dx, RX.xImag[0]+dx, NULL, NULL, RX.vReal[0]+dx, RX.vImag[0]+dx);
    if(Register.PathSel.b1) Mojave_FFT(1, NULL, NULL, RX.xReal[1]+dx, RX.xImag[1]+dx, NULL, NULL, RX.vReal[1]+dx, RX.vImag[1]+dx);

    if(Register.PathSel.b0) aMojave_DFT_pilot(RX.xReal[0]+dx, RX.xImag[0]+dx, pReal[0], pImag[0]);
    if(Register.PathSel.b1) aMojave_DFT_pilot(RX.xReal[1]+dx, RX.xImag[1]+dx, pReal[1], pImag[1]);

    // carrier phase tracking based on pilot tones
    aMojave_WeightedPilotPhaseTrack(1, Register.PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                    RX.hReal[0], RX.hImag[0], RX.hReal[1], RX.hImag[1], RX.cSP[0], RX.d2AdvDly, &(RX.cCPP), &(Register));

    aMojave_PhaseCorrect(Register.PathSel, 80, RX.cSP[0], RX.cCP[0], RX.cCPP, &(RX),
                         RX.vReal[0]+dv, RX.vImag[0]+dv, RX.vReal[1]+dv, RX.vImag[1]+dv,
                         RX.uReal[0]+du, RX.uImag[0]+du, RX.uReal[1]+du, RX.uImag[1]+du);
  
    aMojave_EqD(Register.PathSel, SignalRate, RX.NoiseShift[0], RX.NoiseShift[1], RX.hReal[0], RX.hImag[0], RX.hReal[1], RX.hImag[1], 
                RX.h2[0], RX.h2[1], RX.uReal[0], RX.uImag[0], RX.uReal[1], RX.uImag[1], RX.H2, RX.dReal, RX.dImag, RX.DReal, RX.DImag, du, RX.Lp);

    aMojave_PLL(RX.dReal+du, RX.dImag+du, RX.H2+du, RX.DReal+du, RX.DImag+du, 1, RX.AdvDly+du, RX.cSP+0, RX.cCP+0, &RX, &(Register));
    RX.d2AdvDly = RX.dAdvDly; RX.dAdvDly = RX.AdvDly[du];
    if (RX.NonRTL.Enabled && RX.NonRTL.DisablePLL) RX.dAdvDly.word = 0; // TEST Feature not in ASIC

    aMojave_Interleaver  (SignalRate, RX.Lp, RX.Lc, 1);
    aMojave_Depuncture   (24, RX.Lc, RX.cA, RX.cB, SignalRate);
    aMojave_VitDec       (24, RX.cA, RX.cB, RX.b, &RX);

    Register.RX_RATE.b3 = RX.b[0].bit;
    Register.RX_RATE.b2 = RX.b[1].bit;
    Register.RX_RATE.b1 = RX.b[2].bit;
    Register.RX_RATE.b0 = RX.b[3].bit;

    Register.RX_LENGTH.w12 = 0;
    for(i=0; i<12; i++)
        Register.RX_LENGTH.w12 = Register.RX_LENGTH.w12 | (RX.b[5+i].bit << i);

    if(Register.RX_LENGTH.w12==0 || (Register.RX_LENGTH.w12>>4) > Register.maxLENGTH.w8)
    {
        RX.Fault = 1;
        RX.EventFlag.b1 = 1;
        aSigOut->RxFault = 1;
    }
    // ------------
    // Parity check
    // ------------
    {
        int z;
        for(z=0, i=0; i<24; i++)
            z = z ^ RX.b[i].bit;
    
        if(z) 
        {
            RX.Fault = 1;
            RX.EventFlag.b2 = 1;
            aSigOut->RxFault = 1;
        }
    }
    // -------------------------------------------------------
    // Determine the code and subcarrier modulation parameters
    // -------------------------------------------------------
    switch(Register.RX_RATE.w4)
    {
        case 0xD: RX.N_BPSC = 1; RX.N_DBPS =  24; break;
        case 0xF: RX.N_BPSC = 1; RX.N_DBPS =  36; break;
        case 0x5: RX.N_BPSC = 2; RX.N_DBPS =  48; break;
        case 0x7: RX.N_BPSC = 2; RX.N_DBPS =  72; break;
        case 0x9: RX.N_BPSC = 4; RX.N_DBPS =  96; break;
        case 0xB: RX.N_BPSC = 4; RX.N_DBPS = 144; break;
        case 0x1: RX.N_BPSC = 6; RX.N_DBPS = 192; break;
        case 0x3: RX.N_BPSC = 6; RX.N_DBPS = 216; break;
        case 0xA: RX.N_BPSC = 8; RX.N_DBPS = 288; if(!Register.Disable72.b0) break;
        default : RX.N_BPSC = 1; RX.N_DBPS =  24; // defaults to prevent divide-by-zero below
            RX.Fault = 1;
            RX.EventFlag.b3 = 1;
            aSigOut->RxFault = 1;
    }
    // ------------------------
    // Check the "Reserved" Bit
    // ------------------------
    if(Register.Reserved0.b0 && RX.b[4].bit) aSigOut->RxFault = 1; // reserved bit is "1" not "0"

    // ---------------------------------------------------------------------
    // Compute the number of bits in the DATA field. The data field contains
    // 16 SERVICE bits, 8*LENGTH data bits, 6 tail bits, and enough pad bits
    // to fill a whole number of OFDM symbols, each containing N_DBPS bits.
    // ---------------------------------------------------------------------
    RX.N_SYM = (16 + 8*Register.RX_LENGTH.w12 + 6 + (RX.N_DBPS-1)) / RX.N_DBPS;
    
    if(RX.N_SYM >=(RX.rLength-4*du)/320) {RX.N_SYM=1; RX.Fault=4; RX.RTL_Exception.Truncated_N_SYM = 1;} // SIGNAL incorrectly decoded
    if(RX.N_SYM >  RX.aLength/RX.N_DBPS) {RX.N_SYM=1; RX.Fault=4; RX.RTL_Exception.Truncated_N_SYM = 1;}

    RX.Nu = RX.Nv = 64*(3+RX.N_SYM);

    // --------------------------------------------------------------
    // Check that the decoded LENGTH is valid for the receiver buffer
    // --------------------------------------------------------------
    if((RX.N_SYM+6) * 320 > RX.rLength) 
    {
        RX.Fault = 2;
        RX.EventFlag.b8 = 1;
        aSigOut->RxFault = Register.RxFault.b0 = 1;
        RX.RTL_Exception.rLimit_SIGNAL = 1;
        return WI_WARN_PHY_SIGNAL_LENGTH;
    }
    // ------------------------------------------------------------------
    // Compute the convolutioanl encoder rate x 12, e.g., CodeRate = m/12
    // Also compute the number of code bits per symbol
    // ------------------------------------------------------------------
    m = RX.N_DBPS / (4 * RX.N_BPSC);
    RX.N_CBPS = RX.N_DBPS * 12 / m;

    // -------------------------
    // Advance and Save Pointers
    // -------------------------
    RX.dx     = dx+80;
    RX.du     = du+80;
    RX.dv     = dv+80;
    RX.dc     = 48; // code bit (LLR) offset

    aSigOut->HeaderValid = aSigOut->RxFault? 0:1; // valid header if no fault
    Register.RxFault.b0  = aSigOut->RxFault;
    RX.aPacketDuration.word = aSigOut->HeaderValid? (RX.N_SYM<2? 0 : 4*RX.N_SYM-5) : 0; // adjust offset to match hardware latency

    if(RX.EVM.Enabled) XSTATUS(aMojave_EVM1());

    return WI_SUCCESS;
}
// end of Mojave_RX2_SIGNAL()

// ================================================================================================
// FUNCTION : Mojave_RX2_DATA_Demod()
// ------------------------------------------------------------------------------------------------
// Purpose  : Implement RX2 demodulate/demapping of OFDM DATA symbols
// ================================================================================================
wiStatus Mojave_RX2_DATA_Demod(unsigned int n)
{
    unsigned int dc = RX.dc;
    unsigned int dx = RX.dx;
    unsigned int dv = RX.dv;
    unsigned int du = RX.du;

    wiWORD pReal[2][4], pImag[2][4];

    // -----------------------------------------------------------------------
    // Demodulate the "DATA" OFDM symbols and de-interleave the soft decisions
    // -----------------------------------------------------------------------
    if(dx>RX.Nx) 
    {
        RX.N_SYM = n - 2; // set N_SYM to prevent trace errors
        RX.Fault = 2;
        RX.EventFlag.b8 = 1;
        RX.RTL_Exception.xLimit_DataDemod = 1;
        return WI_WARN_PHY_BAD_PACKET;
    }
    RX.kFFT[n] = dx;

    Mojave_FFT(1, NULL, NULL, RX.xReal[0]+dx, RX.xImag[0]+dx, NULL, NULL, RX.vReal[0]+dx, RX.vImag[0]+dx);
    Mojave_FFT(1, NULL, NULL, RX.xReal[1]+dx, RX.xImag[1]+dx, NULL, NULL, RX.vReal[1]+dx, RX.vImag[1]+dx);

    aMojave_DFT_pilot(RX.xReal[0]+dx, RX.xImag[0]+dx, pReal[0], pImag[0]);
    aMojave_DFT_pilot(RX.xReal[1]+dx, RX.xImag[1]+dx, pReal[1], pImag[1]);

    // carrier phase tracking based on pilot tones
    aMojave_WeightedPilotPhaseTrack(0, Register.PathSel, pReal[0], pImag[0], pReal[1], pImag[1],
                                   RX.hReal[0], RX.hImag[0], RX.hReal[1], RX.hImag[1], RX.cSP[n-1], RX.d2AdvDly, &(RX.cCPP), &(Register));

    aMojave_PhaseCorrect(Register.PathSel, 80, RX.cSP[n-1], RX.cCP[n-1], RX.cCPP, &(RX),
                       RX.vReal[0]+dv, RX.vImag[0]+dv, RX.vReal[1]+dv, RX.vImag[1]+dv,
                       RX.uReal[0]+du, RX.uImag[0]+du, RX.uReal[1]+du, RX.uImag[1]+du);

    if(RX.EVM.Enabled) XSTATUS(aMojave_EVM2(du,n));

    aMojave_EqD(Register.PathSel, Register.RX_RATE, RX.NoiseShift[0], RX.NoiseShift[1],  RX.hReal[0], RX.hImag[0], RX.hReal[1], RX.hImag[1], 
              RX.h2[0], RX.h2[1], RX.uReal[0], RX.uImag[0], RX.uReal[1], RX.uImag[1], RX.H2, RX.dReal, RX.dImag, RX.DReal, RX.DImag, du, RX.Lp+dc);

    aMojave_PLL(RX.dReal+du, RX.dImag+du, RX.H2+du, RX.DReal+du, RX.DImag+du, 0, RX.AdvDly+du, RX.cSP+n, RX.cCP+n, &(RX), &(Register));
    RX.d2AdvDly = RX.dAdvDly; RX.dAdvDly = RX.AdvDly[du];
    if(RX.d2AdvDly.w2 && n<RX.N_SYM) RX.EventFlag.b6 = 1; // symbol timing shift during packet

    aMojave_Interleaver(Register.RX_RATE, RX.Lp+dc, RX.Lc+dc, 1);

    // -----------------------------------------------
    // Advance and save array pointers for next symbol
    // -----------------------------------------------
    RX.dx = dx + 80 + RX.d2AdvDly.w2;
    RX.dv = dv + 80 + RX.d2AdvDly.w2;
    RX.du = du + 80 + RX.d2AdvDly.w2;
    RX.dc = dc + RX.N_CBPS;

    return WI_SUCCESS;
}
// end of Mojave_RX2_DATA_Demod()

// ================================================================================================
// FUNCTION : Mojave_RX2_DATA_Decode()
// ------------------------------------------------------------------------------------------------
// Purpose  : Implement RX2 decoding of OFDM DATA symbols
// ================================================================================================
wiStatus Mojave_RX2_DATA_Decode(void)
{
    int Nbits  = 16 + 8*Register.RX_LENGTH.w12 + 6; // number of bits in DATA field (including tail)
    wiUWORD *a = RX.a+24; // serial data pointer offset past SIGNAL symbol
    unsigned int i, k;

    // -------------------------------
    // Decode/Descramble the data bits
    // -------------------------------
    aMojave_Depuncture(RX.N_SYM*RX.N_DBPS, RX.Lc+48, RX.cA+24, RX.cB+24, Register.RX_RATE);
    aMojave_VitDec    (Nbits, RX.cA+24, RX.cB+24, RX.b+24, &RX);
    aMojave_Descramble(Nbits, RX.b +24, RX.a +24);

    // -----------------------------------
    // Re-pack the SERVICE and PSDU fields
    // -----------------------------------
    Register.RX_SERVICE.word = 0;
    for(k=0; k<16; k++, a++)
        Register.RX_SERVICE.w16 = Register.RX_SERVICE.w16 | (a->bit << k);

    RX.N_PHY_RX_WR = 8 + Register.RX_LENGTH.w12; // number of bytes on PHY_RX_D
    for(i=8; i<RX.N_PHY_RX_WR; i++) 
    {
        RX.PHY_RX_D[i].word = 0;      
        for(k=0; k<8; k++, a++)
            RX.PHY_RX_D[i].word = RX.PHY_RX_D[i].w8 | (a->bit << k);
    }
    RX.RTL_Exception.DecodedOFDM = 1; // flag completion of OFDM decode
    RX.RTL_Exception.PacketCount = min(3, RX.RTL_Exception.PacketCount+1);

    // -------------------------------------
    // EVM Test Structure (NOT PART OF ASIC)
    // -------------------------------------
    if(RX.EVM.Enabled)
    {
        double N = 48.0 * RX.EVM.N_SYM;

        RX.EVM.EVM[0]   = RX.EVM.Se2[0]? RX.EVM.Se2[0]/N : 1.0;
        RX.EVM.EVM[1]   = RX.EVM.Se2[1]? RX.EVM.Se2[1]/N : 1.0;

        RX.EVM.EVMdB[0] = 10.0*log10(RX.EVM.EVM[0]);
        RX.EVM.EVMdB[1] = 10.0*log10(RX.EVM.EVM[1]);
    }
    return WI_SUCCESS;
}
// end of Mojave_RX2_DATA_Decode()

// ================================================================================================
// FUNCTION  : Mojave_TX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : 
// ================================================================================================
wiStatus Mojave_TX_Restart(void)
{
    if(Verbose) wiPrintf(" Mojave_TX_Restart()\n");
    
    // --------------------------------------------
    // Define the DAC Mask for Effective Resolution
    // --------------------------------------------
    TX.DAC.Mask = ((1<<(TX.DAC.nEff))-1) << (10-TX.DAC.nEff);
    TX.DAC.c    = TX.DAC.Vpp / 1024.0;

    // -------------------------
    // Clear the fault indicator
    // -------------------------
    TX.Fault = 0;

    return WI_SUCCESS;
}
// end of Mojave_TX_Restart()

// ================================================================================================
// FUNCTION  : Mojave_RX_Restart()
// ------------------------------------------------------------------------------------------------
// Purpose   : Reset model state in anticipation of an RX operation
// Parameters: _bRX -- Pointer to bMojave RX state
// ================================================================================================
wiStatus Mojave_RX_Restart(bMojave_RX_State *_bRX)
{
    wiWORD Zero = {0}, Dummy; wiUWORD UZero = {0};
    int i;

    if(Verbose) wiPrintf(" Mojave_RX_Restart()\n");

    // --------------------
    // Reset DSel Registers
    // --------------------
    Mojave_DSel(UZero, UZero, UZero, UZero, &Dummy, &Dummy, &Dummy, &Dummy, Register.PathSel, 0);

    // ---------------------------------------------
    // Reset the phase counters in the mixer and LPF
    // ---------------------------------------------
    aMojave_Downsam(0, Zero, Zero, &Dummy, &Dummy, 0, 0);
    aMojave_Downsam(1, Zero, Zero, &Dummy, &Dummy, 0, 0);

    Mojave_Downmix(0, Zero, Zero, &Dummy, &Dummy, 0, &Register);
    Mojave_Downmix(1, Zero, Zero, &Dummy, &Dummy, 0, &Register);

    // ---------------------------
    // Reset/Disable the DCX block
    // ---------------------------
    Mojave_DCX(-1, Zero, NULL, 0, NULL);

    // -------------------------------
    // Clear the downmix output arrays
    // -------------------------------
    for (i=0; i<2; i++)
    {
        if (RX.zReal[i]) memset(RX.zReal[i], 0, RX.N40*sizeof(wiWORD));
        if (RX.zImag[i]) memset(RX.zImag[i], 0, RX.N40*sizeof(wiWORD));
        if (RX.yReal[i]) memset(RX.yReal[i], 0, RX.N20*sizeof(wiWORD));
        if (RX.yImag[i]) memset(RX.yImag[i], 0, RX.N20*sizeof(wiWORD));
    }
    // --------------------------------------------
    // Define the ADC Mask for Effective Resolution
    // --------------------------------------------
    RX.ADC.Mask = ((1<<(RX.ADC.nEff))-1) << (10-RX.ADC.nEff);
    RX.ADC.c    = 1024.0 / RX.ADC.Vpp;

    // ------------------------------
    // Point to RX State in bMojave.c
    // ------------------------------
    bRX = _bRX;

    // -------------------------
    // Clear the Exception Flags
    // -------------------------
    RX.RTL_Exception.word = 0;

    return WI_SUCCESS;
}
// end of Mojave_RX_Restart()

// ================================================================================================
// FUNCTION : Mojave_PowerOn()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_PowerOn()
{
    const int PSK[2]     = {-1,1};
    const int QAM16[4]   = {-3,-1,3,1};
    const int QAM64[8]   = {-7,-5,-1,-3, 7, 5, 1, 3};
    const int QAM256[16] = {-15,-13,-9,-11,-1,-3,-7,-5,15,13,9,11,1,3,7,5};

    wiUWORD X, Y, A;

    if(Verbose) wiPrintf(" Mojave_PowerOn()\n");

    // ------------------
    // Default Parameters
    // ------------------
    if(!TX.aPadFront) TX.aPadFront =1000; // pad the packet start with 50 us
    if(!TX.aPadBack)  TX.aPadBack  = 200; // pad the packet end with   10 us (account for decoding latency)
    if(!RX.aLength)   RX.aLength   = 16+8*4095+6+287; // last term fills out interger # symbols @ 72
    if(!RX.rLength)   RX.rLength   = 2*65536;
    if(!RX.ADC.nEff)  RX.ADC.nEff  = 9;   // 9 bit effective ADC resolution   
    if(!RX.ADC.Vpp)   RX.ADC.Vpp   = 1.0; // 1.2Vpp full-scale input
    if(!RX.ADC.Offset)RX.ADC.Offset= 6;   // 6 LSB offset
    if(!TX.DAC.nEff)  TX.DAC.nEff  = 9;   // 9 bit effective DAC resolution   
    if(!TX.DAC.Vpp)   TX.DAC.Vpp   = 1.0; // 1.0Vpp full-scale output

    if(!TX.bPadFront) TX.bPadFront = 550;
    if(!TX.bPadBack)  TX.bPadBack  = 88;

    XSTATUS(Mojave_RX_AllocateMemory());

    // -----------------------------------------------
    // Build the subcarrier mapper look-up table (ROM)
    // -----------------------------------------------
    for(Y.word=0; Y.word<16; Y.word++)
    {
        for(X.word=0; X.word<16; X.word++)
        {
            A.w8 = (Y.w4<<4) | X.w4;
            switch(Y.w4)
            {
                case 0xD: case 0xF: TX.ModulationROM[A.w8&0xF1].word = 13*PSK[X.w1];    break; //  0.00 dBr
                case 0x5: case 0x7: TX.ModulationROM[A.w8&0xF1].word =  9*PSK[X.w1];    break; // -0.19 dBr 
                case 0x9: case 0xB: TX.ModulationROM[A.w8&0xF3].word =  4*QAM16[X.w2];  break; // -0.24 dBr
                case 0x1: case 0x3: TX.ModulationROM[A.w8&0xF7].word =  2*QAM64[X.w3];  break; // -0.03 dBr
                case 0xA:           TX.ModulationROM[A.w8&0xFF].word =  1*QAM256[X.w4]; break; // +0.03 dBr
                default:            TX.ModulationROM[A.w8&0xFF].word =  0;
            }
        }
    }
    // ----------------------------------------------------
    // Build the hard decision demapper look-up table (ROM)
    // ----------------------------------------------------
    for(Y.word=0; Y.word<16; Y.word++)
    {
        for(X.word=0; X.word<16; X.word++)
        {
            A.w8 = (Y.w4<<4) | X.w4;
            switch(Y.w4)
            {
                case 0xD: case 0xF: RX.DemodROM[A.w8].word = PSK[X.w1];    break;
                case 0x5: case 0x7: RX.DemodROM[A.w8].word = PSK[X.w1];    break;
                case 0x9: case 0xB: RX.DemodROM[A.w8].word = QAM16[X.w2];  break;
                case 0x1: case 0x3: RX.DemodROM[A.w8].word = QAM64[X.w3];  break;
                case 0xA:           RX.DemodROM[A.w8].word = QAM256[X.w4]; break;
                default:            RX.DemodROM[A.w8].word = 0;
            }
        }
    }
    // ---------------------------------------------------------
    // Build the interleaver/de-interleaver address tables (ROM)
    // ---------------------------------------------------------
    {
        const int cbps[] = {288, 96, 192, 48, 384};
        const int s[]    = {3,1,2,1,4};
        const int d[]    = {3,4,4,4,4};
        int m, n, p, i[2], j[2], k[2], a[2], c[2], addr, data;

        for(m=0; m<5; m++) {
            for(n=0; n<cbps[m]/2; n++) {
                for(p=0; p<2; p++) {
                    k[p] = (2*n + p);
                    i[p] = (cbps[m]/16) * (k[p]%16) + (k[p]/16);
                    j[p] = s[m]*(i[p]/s[m]) + ((i[p]+cbps[m]-(16*i[p]/cbps[m]))%s[m]);
                    a[p] = j[p] / d[m];
                    c[p] = j[p] % d[m];
                    addr = (m << 8) | n;
                    data = ((a[p]&0x7F)<<2) | (c[p]&0x03);
                    TX.jROM[p][addr].word = data;
                }
            }
        }
        for(m=0; m<5; m++) {
            for(n=0; n<cbps[m]/2; n++) {
                for(p=0; p<2; p++) {
                    k[p] = (2*n + p);
                    a[p] = k[p] / d[m];
                    c[p] = k[p] % d[m];
                    addr = (m << 8) | n;
                    data = ((a[p]&0x7F)<<2) | (c[p]&0x03);
                    TX.kROM[p][addr].word = data;
                }
            }
        }
        TX.bpscROM[0xD].word = TX.bpscROM[0xF].word = 3;
        TX.bpscROM[0x5].word = TX.bpscROM[0x7].word = 1;
        TX.bpscROM[0x9].word = TX.bpscROM[0xB].word = 2;
        TX.bpscROM[0x1].word = TX.bpscROM[0x3].word = 0;
        TX.bpscROM[0xA].word                        = 4;
    }
    XSTATUS( bMojave_PowerOn() );
    
    // --------------
    // Power on reset
    // --------------
    XSTATUS( Mojave_RESET() );

    return WI_SUCCESS;
}
// end of Mojave_PowerOn()

// ================================================================================================
// FUNCTION : Mojave_PowerOff()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_PowerOff()
{
    int i, j;
    if(Verbose) wiPrintf(" Mojave_PowerOff()\n");

    Mojave_TX_FreeMemory();
    Mojave_RX_FreeMemory();
    bMojave_RX_FreeMemory();

//   if(RX.s[0]) wiFree(RX.s[0]); RX.s[0] = NULL;
//   if(RX.s[1]) wiFree(RX.s[1]); RX.s[1] = NULL;
    RX.Nr = 0;

    for(i=0; i<2; i++) {
        for(j=0; j<64; j++) {
            if(RX.EVM.x[i][j]) wiFree(RX.EVM.x[i][j]);
            RX.EVM.x[i][j] = NULL;
        }
    }
    return WI_SUCCESS;
}
// end of Mojave_PowerOff()

// ================================================================================================
// FUNCTION : Mojave_RESET()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_RESET(void)
{
    if(Verbose) wiPrintf(" Mojave_Reset()\n");
    Mojave_TX_FreeMemory();

    // ---------------------------
    // Set Default Register Values
    // ---------------------------
    {
        wiUWORD DefaultRegisters[256] = {
              0,  5,  0,  3,  3,  0, 35,  0, 20,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,
            255,  3,  0,  0,  0,  0,  3,  0, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
              2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 60, 37,  0, 56, 72, 68, 20, 12,
              8, 13, 14, 63,  0,  9, 20, 50, 19, 51, 72, 30, 18,  0,  0, 61, 36,  4, 70,195,128, 25,  0,  0,
             12, 15,144,  4, 20, 92, 60,176, 20, 28,  0,  0,  0,  0,  0,  0,  4,  0, 17,162,  0,  0,  0,  0,
            132, 76, 16,  0,  0,  0,  0,  0,  4,  0,  0,  0, 21, 31, 21, 31,  0,  0,  0,  0,  0,  0,  0,  0,
            194,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 66, 35, 21, 26, 22, 50, 32, 37,
             40, 10, 28, 14, 63,  0,  8, 12,  7, 31, 90, 45,  0, 32,102,  0, 16, 53, 17,  5,144,  0,  0,  0,
              0,  0,  0,  0,  0,  0, 48,255,255,255,255,255,255, 30,128,  0,  0,220, 20,  8,  8,  8,  2,  1,
            103, 67, 67,100, 33, 24,  1, 15, 30, 11,  7,  7,165, 60,  0,  0,215,  6,  0,128,128,133,  0,  0,
              0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
        };
        XSTATUS(Mojave_SetRegisterMap(DefaultRegisters));
    }
    // ----------------------------
    // Resets to Individual Modules
    // ----------------------------
    {
        wiWORD Zero = {0};
        Mojave_DCX(-1, Zero, NULL, 0, NULL);
    }
    return WI_SUCCESS;
}
// end of Mojave_RESET()

// ================================================================================================
// FUNCTION : Mojave_TX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_TX_FreeMemory(void)
{
    if(Verbose) wiPrintf(" Mojave_TX_FreeMemory()\n");

    if(TX.a)     wiFree(TX.a);      TX.a  = NULL;  TX.Na = 0;
    if(TX.b)     wiFree(TX.b);      TX.b  = NULL;  TX.Nb = 0;
    if(TX.c)     wiFree(TX.c);      TX.c  = NULL;  TX.Nc = 0;
    if(TX.p)     wiFree(TX.p);      TX.p  = NULL;  TX.Np = 0;
    if(TX.cReal) wiFree(TX.cReal);  TX.cReal = NULL;  TX.Nc = 0;
    if(TX.cImag) wiFree(TX.cImag);  TX.cImag = NULL;  
    if(TX.vReal) wiFree(TX.vReal);  TX.vReal = NULL;  TX.Nv = 0;
    if(TX.vImag) wiFree(TX.vImag);  TX.vImag = NULL;  
    if(TX.xReal) wiFree(TX.xReal);  TX.xReal = NULL;  TX.Nx = 0;
    if(TX.xImag) wiFree(TX.xImag);  TX.xImag = NULL;  TX.Nx = 0;
    if(TX.yReal) wiFree(TX.yReal);  TX.yReal = NULL;  TX.Ny = 0;
    if(TX.yImag) wiFree(TX.yImag);  TX.yImag = NULL;  TX.Ny = 0;
    if(TX.zReal) wiFree(TX.zReal);  TX.zReal = NULL;  TX.Nz = 0;
    if(TX.zImag) wiFree(TX.zImag);  TX.zImag = NULL;  TX.Nz = 0;
    if(TX.uReal) wiFree(TX.uReal);  TX.uReal = NULL;
    if(TX.uImag) wiFree(TX.uImag);  TX.uImag = NULL;
    if(TX.d)     wiFree(TX.d);      TX.d     = NULL;  TX.Nd = 0;

    return WI_SUCCESS;
}
// end of Mojave_TX_FreeMemory()

// ================================================================================================
// FUNCTION : Mojave_TX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_TX_AllocateMemory(int N_SYM, int N_BPSC, int N_DBPS, int M)
{
    if(Verbose) wiPrintf(" Mojave_TX_AllocateMemory()\n");

    Mojave_TX_FreeMemory();

    TX.Na = 24 + (N_SYM+1)*N_DBPS;
    TX.Nb = TX.Na;
    TX.Nc = 48 + (48 * (N_SYM+1) * N_BPSC);
    TX.Np = 48 + TX.Nc;
    TX.Nx = ((N_SYM+1) * 80) + 400 + 2 + TX.aPadFront + TX.aPadBack;
    TX.Nv = TX.Nx;
    TX.Ny = 4 * (TX.Nx+10);
    TX.Nz = 4 * (TX.Nx+10);
    TX.Nd = M * TX.Nz;

    TX.a     = (wiUWORD  *)wiCalloc(TX.Na, sizeof(wiUWORD));
    TX.b     = (wiUWORD  *)wiCalloc(TX.Nb, sizeof(wiUWORD));
    TX.c     = (wiUWORD  *)wiCalloc(TX.Nc, sizeof(wiUWORD));
    TX.p     = (wiUWORD  *)wiCalloc(TX.Np, sizeof(wiUWORD));
    TX.vReal = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD ));
    TX.vImag = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD ));
    TX.xReal = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD ));
    TX.xImag = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD ));
    TX.yReal = (wiWORD   *)wiCalloc(TX.Ny, sizeof(wiWORD ));
    TX.yImag = (wiWORD   *)wiCalloc(TX.Ny, sizeof(wiWORD ));
    TX.zReal = (wiWORD   *)wiCalloc(TX.Nz, sizeof(wiWORD ));
    TX.zImag = (wiWORD   *)wiCalloc(TX.Nz, sizeof(wiWORD ));
    TX.uReal = (wiUWORD  *)wiCalloc(TX.Nz, sizeof(wiUWORD));
    TX.uImag = (wiUWORD  *)wiCalloc(TX.Nz, sizeof(wiUWORD));
    TX.d     = (wiComplex*)wiCalloc(TX.Nd, sizeof(wiComplex));

    if(!TX.a  || !TX.b  || !TX.c  || !TX.p  || !TX.xReal || !TX.xImag || !TX.yReal || !TX.yImag || !TX.zReal || !TX.zImag || !TX.uReal || !TX.uImag || !TX.d)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of Mojave_TX_AllocateMemory()

// ================================================================================================
// FUNCTION : bMojave_TX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// Parameters: Na -- Number of serial bits
//             Nc -- Number of samples at Baud rate (11 MHz)
//             M  -- Oversample rate at DAC output
// ================================================================================================
wiStatus bMojave_TX_AllocateMemory(int Na, int Nc, int M)
{
    if(Verbose) wiPrintf(" bMojave_TX_AllocateMemory()\n");

    Mojave_TX_FreeMemory();

    TX.Na = Na;                              // number of serial bits
    TX.Nb = Na;                              // number of scrambled bits
    TX.Nc = Nc + TX.bPadFront + TX.bPadBack; // add padding samples at 11 MHz
    TX.Nv = 4*TX.Nc + 12 + 9;                // 44 MHz samples, add for filter and delays
    TX.Nx = 10*(TX.Nv+10)/11;                // 40 MHz samples (+10 to cover delays, CYA)
    TX.Ny = 2*TX.Nx;                         // 80 MHz samples
    TX.Nz =   TX.Ny;                         // Mixer output
    TX.Nd = M*TX.Nz;                         // M times oversampled DAC output
    
    TX.a     = (wiUWORD  *)wiCalloc(TX.Na, sizeof(wiUWORD));
    TX.b     = (wiUWORD  *)wiCalloc(TX.Nb, sizeof(wiUWORD));
    TX.cReal = (wiUWORD  *)wiCalloc(TX.Nc, sizeof(wiUWORD));
    TX.cImag = (wiUWORD  *)wiCalloc(TX.Nc, sizeof(wiUWORD));
    TX.vReal = (wiWORD   *)wiCalloc(TX.Nv, sizeof(wiWORD));
    TX.vImag = (wiWORD   *)wiCalloc(TX.Nv, sizeof(wiWORD));
    TX.xReal = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD));
    TX.xImag = (wiWORD   *)wiCalloc(TX.Nx, sizeof(wiWORD));
    TX.yReal = (wiWORD   *)wiCalloc(TX.Ny, sizeof(wiWORD));
    TX.yImag = (wiWORD   *)wiCalloc(TX.Ny, sizeof(wiWORD));
    TX.zReal = (wiWORD   *)wiCalloc(TX.Nz, sizeof(wiWORD));
    TX.zImag = (wiWORD   *)wiCalloc(TX.Nz, sizeof(wiWORD));
    TX.uReal = (wiUWORD  *)wiCalloc(TX.Nz, sizeof(wiUWORD));
    TX.uImag = (wiUWORD  *)wiCalloc(TX.Nz, sizeof(wiUWORD));
    TX.d     = (wiComplex*)wiCalloc(TX.Nd, sizeof(wiComplex));

    if(!TX.a || !TX.b || !TX.cReal || !TX.cImag || !TX.vReal || !TX.vImag || !TX.xReal || !TX.xImag 
        || !TX.yReal || !TX.yImag || !TX.zReal || !TX.zImag || !TX.uReal || !TX.uImag || !TX.d)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    return WI_SUCCESS;
}
// end of bMojave_TX_AllocateMemory()

// ================================================================================================
// FUNCTION : Mojave_RX_FreeMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_RX_FreeMemory(void)
{
    int n;

    if(Verbose) wiPrintf(" Mojave_RX_FreeMemory()\n");

    if(RX.a)     wiFree(RX.a);     RX.a     = NULL;  RX.Na = 0;
    if(RX.b)     wiFree(RX.b);     RX.b     = NULL;  RX.Nb = 0;
    if(RX.Lc)    wiFree(RX.Lc);    RX.Lc    = NULL;  RX.Nc = 0;
    if(RX.Lp)    wiFree(RX.Lp);    RX.Lp    = NULL;
    if(RX.Lc)    wiFree(RX.Lc);    RX.Lc    = NULL;
    if(RX.cA)    wiFree(RX.cA);    RX.cA    = NULL;
    if(RX.cB)    wiFree(RX.cB);    RX.cB    = NULL;
    if(RX.dReal) wiFree(RX.dReal); RX.dReal = NULL;
    if(RX.dImag) wiFree(RX.dImag); RX.dImag = NULL;
    if(RX.DReal) wiFree(RX.DReal); RX.DReal = NULL;
    if(RX.DImag) wiFree(RX.DImag); RX.DImag = NULL;
    if(RX.H2)    wiFree(RX.H2);    RX.H2    = NULL;  RX.Nd = 0;

    if(RX.DGain)     wiFree(RX.DGain);     RX.DGain     = NULL;
    if(RX.AdvDly)    wiFree(RX.AdvDly);    RX.AdvDly    = NULL;

    if(RX.AGain)          wiFree(RX.AGain);          RX.AGain         = NULL;
    if(RX.DFEState)       wiFree(RX.DFEState);       RX.DFEState      = NULL;
    if(RX.DFEFlags)       wiFree(RX.DFEFlags);       RX.DFEFlags      = NULL;
    if(RX.aSigOut)        wiFree(RX.aSigOut);        RX.aSigOut       = NULL;
    if(RX.bSigOut)        wiFree(RX.bSigOut);        RX.bSigOut       = NULL;
    if(RX.traceRxState)   wiFree(RX.traceRxState);   RX.traceRxState  = NULL;
    if(RX.traceRxControl) wiFree(RX.traceRxControl); RX.traceRxControl= NULL;
    if(RX.traceRadioIO)   wiFree(RX.traceRadioIO);   RX.traceRadioIO  = NULL;
    if(RX.traceDataConv)  wiFree(RX.traceDataConv);  RX.traceDataConv = NULL;
    if(RX.traceDFE)       wiFree(RX.traceDFE);       RX.traceDFE      = NULL;
    if(RX.traceAGC)       wiFree(RX.traceAGC);       RX.traceAGC      = NULL;
    if(RX.traceRSSI)      wiFree(RX.traceRSSI);      RX.traceRSSI     = NULL;
    if(RX.traceDFS)       wiFree(RX.traceDFS);       RX.traceDFS      = NULL;
    if(RX.traceSigDet)    wiFree(RX.traceSigDet);    RX.traceSigDet   = NULL;
    if(RX.traceFrameSync) wiFree(RX.traceFrameSync); RX.traceFrameSync= NULL;

    for(n=0; n<2; n++)
    {
        if(RX.pReal[n]) wiFree(RX.pReal[n]);  RX.pReal[n] = NULL;
        if(RX.pImag[n]) wiFree(RX.pImag[n]);  RX.pImag[n] = NULL;
        if(RX.qReal[n]) wiFree(RX.qReal[n]);  RX.qReal[n] = NULL;
        if(RX.qImag[n]) wiFree(RX.qImag[n]);  RX.qImag[n] = NULL;
        if(RX.cReal[n]) wiFree(RX.cReal[n]);  RX.cReal[n] = NULL;
        if(RX.cImag[n]) wiFree(RX.cImag[n]);  RX.cImag[n] = NULL;
        if(RX.rReal[n]) wiFree(RX.rReal[n]);  RX.rReal[n] = NULL;
        if(RX.rImag[n]) wiFree(RX.rImag[n]);  RX.rImag[n] = NULL;
        if(RX.sReal[n]) wiFree(RX.sReal[n]);  RX.sReal[n] = NULL;
        if(RX.sImag[n]) wiFree(RX.sImag[n]);  RX.sImag[n] = NULL;
        if(RX.uReal[n]) wiFree(RX.uReal[n]);  RX.uReal[n] = NULL;
        if(RX.uImag[n]) wiFree(RX.uImag[n]);  RX.uImag[n] = NULL;
        if(RX.vReal[n]) wiFree(RX.vReal[n]);  RX.vReal[n] = NULL;
        if(RX.vImag[n]) wiFree(RX.vImag[n]);  RX.vImag[n] = NULL;
        if(RX.wReal[n]) wiFree(RX.wReal[n]);  RX.wReal[n] = NULL;
        if(RX.wImag[n]) wiFree(RX.wImag[n]);  RX.wImag[n] = NULL;
        if(RX.xReal[n]) wiFree(RX.xReal[n]);  RX.xReal[n] = NULL;
        if(RX.xImag[n]) wiFree(RX.xImag[n]);  RX.xImag[n] = NULL;
        if(RX.yReal[n]) wiFree(RX.yReal[n]);  RX.yReal[n] = NULL;
        if(RX.yImag[n]) wiFree(RX.yImag[n]);  RX.yImag[n] = NULL;
        if(RX.zReal[n]) wiFree(RX.zReal[n]);  RX.zReal[n] = NULL;
        if(RX.zImag[n]) wiFree(RX.zImag[n]);  RX.zImag[n] = NULL;
    }
    return WI_SUCCESS;
}
// end of Mojave_RX_FreeMemory()

// ================================================================================================
// FUNCTION : Mojave_RX_AllocateMemory()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_RX_AllocateMemory(void)
{
    int n;
    int N80 = RX.rLength;
    int N40 = N80/2 + 1;
    int N20 = N80/4 + 2 + (aMOJAVE_FFT_DELAY + aMOJAVE_PLL_DELAY);
    int N44 = (N80*11/10)/2+1;
    int N22 = N44/2 + 1;

    if(Verbose) wiPrintf(" Mojave_RX_AllocateMemory(): N80=%d, N20=%d\n",N80,N20);

    RX.N80 = N80;
    RX.N40 = N40;
    RX.N20 = N20;
    RX.N22 = N22;

    Mojave_RX_FreeMemory();

    RX.Na = RX.aLength + 288;
    RX.Nb = RX.Na + 288;
    RX.Nc = 2 * RX.aLength + 384;
    RX.Nd = N20;

    RX.Nr = N40-1;
    RX.Nx = N20-1;
    RX.Ns = N20-1;

    RX.a     = (wiUWORD   *)wiCalloc(RX.Na, sizeof(wiUWORD));
    RX.b     = (wiUWORD   *)wiCalloc(RX.Nb, sizeof(wiUWORD));
    RX.Lp    = (wiUWORD   *)wiCalloc(RX.Nc, sizeof(wiUWORD));
    RX.Lc    = (wiUWORD   *)wiCalloc(RX.Nc, sizeof(wiUWORD));
    RX.cA    = (wiUWORD   *)wiCalloc(RX.Nb, sizeof(wiUWORD));
    RX.cB    = (wiUWORD   *)wiCalloc(RX.Nb, sizeof(wiUWORD));
    RX.H2    = (wiWORD    *)wiCalloc(RX.Nd, sizeof(wiWORD));
    RX.dReal = (wiWORD    *)wiCalloc(RX.Nd, sizeof(wiWORD));
    RX.dImag = (wiWORD    *)wiCalloc(RX.Nd, sizeof(wiWORD));
    RX.DReal = (wiWORD    *)wiCalloc(RX.Nd, sizeof(wiWORD));
    RX.DImag = (wiWORD    *)wiCalloc(RX.Nd, sizeof(wiWORD));

    RX.DGain    = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
    RX.AdvDly   = (wiWORD *)wiCalloc(N20, sizeof(wiWORD ));

    for(n=0; n<2; n++)
    {
        RX.pReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.pImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.qReal[n] = (wiUWORD*)wiCalloc(N40, sizeof(wiUWORD));
        RX.qImag[n] = (wiUWORD*)wiCalloc(N40, sizeof(wiUWORD));
        RX.rReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
        RX.rImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
        RX.cReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
        RX.cImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
        RX.sReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.sImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.uReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.uImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.vReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.vImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.wReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.wImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.xReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.xImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.yReal[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.yImag[n] = (wiWORD *)wiCalloc(N20, sizeof(wiWORD));
        RX.zReal[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
        RX.zImag[n] = (wiWORD *)wiCalloc(N40, sizeof(wiWORD));
    }
    // ---------------
    // Trace Variables
    // ---------------
    if(RX.EnableTrace)
    {
        RX.AGain         = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        RX.DFEState      = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        RX.DFEFlags      = (wiUWORD *)wiCalloc(N20, sizeof(wiUWORD));
        RX.aSigOut       = (Mojave_aSigState         *)wiCalloc(N20, sizeof(Mojave_aSigState         ));
        RX.bSigOut       = (Mojave_bSigState         *)wiCalloc(N22, sizeof(Mojave_bSigState         ));
        RX.traceRxState  = (Mojave_traceRxStateType  *)wiCalloc(N80, sizeof(Mojave_traceRxStateType  ));
        RX.traceRxControl= (Mojave_traceRxControlType*)wiCalloc(N80, sizeof(Mojave_traceRxControlType));
        RX.traceRadioIO  = (Mojave_RadioDigitalIO    *)wiCalloc(N80, sizeof(Mojave_RadioDigitalIO    ));
        RX.traceDataConv = (Mojave_DataConvCtrlState *)wiCalloc(N80, sizeof(Mojave_DataConvCtrlState ));
        RX.traceDFE      = (Mojave_traceDFEType      *)wiCalloc(N20, sizeof(Mojave_traceDFEType      ));
        RX.traceAGC      = (Mojave_traceAGCType      *)wiCalloc(N20, sizeof(Mojave_traceAGCType      ));
        RX.traceRSSI     = (Mojave_traceRSSIType     *)wiCalloc(N20, sizeof(Mojave_traceRSSIType     ));
        RX.traceDFS      = (Mojave_traceDFSType      *)wiCalloc(N20, sizeof(Mojave_traceDFSType      ));
        RX.traceSigDet   = (Mojave_traceSigDetType   *)wiCalloc(N20, sizeof(Mojave_traceSigDetType   ));
        RX.traceFrameSync= (Mojave_traceFrameSyncType*)wiCalloc(N20, sizeof(Mojave_traceFrameSyncType));
    }
    // ---------------------------------
    // Check for memory allocation fault
    // ---------------------------------
    if(!RX.a  || !RX.b  || !RX.Lc || !RX.Lp || !RX.cA || !RX.cB || !RX.dReal || !RX.dImag)
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for(n=0; n<2; n++)
    {
        if(!RX.rReal[n] || !RX.rImag[n] || !RX.zReal[n] || !RX.zImag[n] || !RX.yReal[n] || !RX.yImag[n] 
        || !RX.wReal[n] || !RX.wImag[n] || !RX.xReal[n] || !RX.xImag[n] || !RX.vReal[n] || !RX.vImag[n] 
        || !RX.uReal[n] || !RX.uImag[n] || !RX.qReal[n] || !RX.qImag[n] || !RX.cReal[n] || !RX.cImag[n])
            return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    return WI_SUCCESS;
}
// end of Mojave_RX_AllocateMemory()

// ================================================================================================
// FUNCTION : Mojave_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_StatePointer(Mojave_TX_State **pTX, Mojave_RX_State **pRX, MojaveRegisters **pReg)
{
    if(pTX)  *pTX  = &TX;
    if(pRX)  *pRX  = &RX;
    if(pReg) *pReg = &Register;
    return WI_SUCCESS;
}
// end of Mojave_StatePointer()

// ================================================================================================
// FUNCTION : Mojave_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_WriteConfig(wiMessageFn MsgFunc)
{
    const char *RXModeStr[] = {"undefined","802.11a (OFDM)","802.11b (DSSS/CCK)","802.11g (DSSS/CCK + OFDM)"};
    MojaveRegisters *p = &Register;  // baseband configuration registers

    int i;
    MsgFunc(" REG: PathSel          = %d\n",p->PathSel.w2);
    MsgFunc(" REG: RXMode           = %d, %s\n",p->RXMode.w2,RXModeStr[p->RXMode.w2]);
    MsgFunc(" REG: Transmit         = TX_PRBS   = 0x%02X, TX_SERVICE  = 0x%04X\n",p->TX_PRBS,p->TX_SERVICE);
    MsgFunc(" REG: Packet Filters   : AddrFilter  =%3d, Disable72   =%3d, Reserved0   =%3d, maxLENGTH   =%3d\n",p->AddrFilter,p->Disable72,p->Reserved0,p->maxLENGTH.w8);
    MsgFunc(" REG: Restart Control  : StepUpRstart=%3d, StepDnRstart=%3d, NoStep>Sync =%3d, NoStep>Hdr  =%3d\n",p->StepUpRestart,p->StepDownRestart,p->NoStepAfterSync,p->NoStepAfterHdr);
    MsgFunc(" REG: RX GainUpdate    : SquelchTime =%3d, DelaySqlchRF=%3d, DelayClrDCX =%3d, RFSW1/SW2 =%02X,%02X\n",p->SquelchTime,p->DelaySquelchRF,p->DelayClearDCX,p->RFSW1,p->RFSW2); 
    MsgFunc(" REG: AGC              : WaitAGC     =%3d, DelayAGC    =%3d, DelayCFO    =%3d, MaxUpdateCnt=%3d\n",p->WaitAGC,p->DelayAGC,p->DelayCFO.w5,p->MaxUpdateCount.w3);
    MsgFunc("                       : AbsPwrL     =%3d, AbsPwrH     =%3d, RefPwrDig   =%3d, InitAGain   =%3d\n",p->AbsPwrL,p->AbsPwrH,p->RefPwrDig,p->InitAGain);
    MsgFunc("                       : UpdateOnLNA =%3d, Pwr100dBm   =%3d, LgSigAFEVald=%3d, ArmLgSigDet3=%3d\n",p->UpdateOnLNA,p->Pwr100dBm,p->LgSigAFEValid,p->ArmLgSigDet3);
    MsgFunc("                       : StepSmall   =%3d, StepLarge   =%3d, StepLgSigDet=%3d, DelayRSSI   =%3d\n",p->StepSmall,p->StepLarge,p->StepLgSigDet,p->DelayRSSI);
    MsgFunc("                       : ThSigSmall  =%3d, ThSigLarge  =%3d, ThStepUp    =%3d, ThStepDown  =%3d\n",p->ThSigSmall,p->ThSigLarge,p->ThStepUp,p->ThStepDown);
    MsgFunc(" REG: LNA Configuration: StepLNA     =%3d, StepLNA2    =%3d, ThSwitchLNA =%3d, ThSwitchLNA2=%3d\n",p->StepLNA,p->StepLNA2,p->ThSwitchLNA,p->ThSwitchLNA2);
    MsgFunc(" REG: Signal Detect    : SigDetTh1   =%3d, SigDetTh2   =%3d, SigDetFilter=%3d, SigDetWindow=%3d\n",p->SigDetTh1,p->SigDetTh2H.w8<<8|p->SigDetTh2L.w8,p->SigDetFilter,p->SigDetWindow);
    MsgFunc(" REG: Frame Sync       : SyncMag     =%3d, SyncThSig   =%3d, SyncThExp   =%3d, SyncDelay   =%3d\n",p->SyncMag,p->SyncThSig,p->SyncThExp,p->SyncDelay);
    MsgFunc(" REG: Frame Sync       : CFOFilter   =%3d, SyncFilter  =%3d, IdleFSEnB   =%3d, SyncShift   =%3d\n",p->CFOFilter.b0,p->SyncFilter,p->IdleFSEnB,p->SyncShift);
    MsgFunc(" REG: DCX              : DCXSelect   =%3d, DCXShift    =%3d, DCXHoldAcc  =%3d, DCXFastLoad =%3d\n",p->DCXSelect,p->DCXShift.word,p->DCXHoldAcc,p->DCXFastLoad);
    MsgFunc(" REG: Prd/Misc         : DACSrcR     =%3d, DACSrcI     =%3d, MinShift    =%3d, DelayRestart=%3d\n",p->PrdDACSrcR,p->PrdDACSrcI,p->MinShift,p->DelayRestart);
    MsgFunc(" REG: DFS Radar Detect : DFSOn       =%3d, ThUp=%d, ThDn=%d, minPPB=%d, Pattern=%d\n",p->DFSOn,p->ThDFSUp,p->ThDFSDn,p->DFSminPPB,p->DFSPattern); 
    MsgFunc(" REG: Mixers           : TxUpmix     =%3s%sRxDownmix   =%3s%s\n",
        p->UpmixOff.b0 ? "OFF":p->UpmixDown.b0? "-10":"+10", p->UpmixConj.b0? "*,":", ",
        p->DownmixOff.b0? "OFF":p->DownmixUp.b0? "-10":"+10",p->UpmixConj.b0? "*":"" );
    MsgFunc(" REG: PilotTone PLL    : PilotPLLEn  =%3d, aC=%d, bC=%d, cC=%d\n",p->PilotPLLEn.b0,p->aC.w3,p->bC.w3,p->cC.w3);
    MsgFunc(" REG: DPLL SaSb (octal): [%d%d",p->Sa_Sb[0].w6>>3,p->Sa_Sb[0].w3);
    for(i=1; i<15; i++) MsgFunc(",%d%d",p->Sa_Sb[i].w6>>3,p->Sa_Sb[i].w3); MsgFunc("]\n");
    MsgFunc(" REG: DPLL CaCb (octal): [%d%d",p->Ca_Cb[0].w6>>3,p->Ca_Cb[0].w3);
    for(i=1; i<15; i++) MsgFunc(",%d%d",p->Ca_Cb[i].w6>>3,p->Ca_Cb[i].w3); MsgFunc("]\n");
    MsgFunc(" REG: OFDM SwD         : Enable      =%3d, Init=%d, Save=%d, Th=%3d, Delay=%2d\n",p->OFDMSwDEn,p->OFDMSwDInit,p->OFDMSwDSave,p->OFDMSwDTh,p->OFDMSwDDelay);
    MsgFunc(" REG: DSSS SwD         : Enable      =%3d, Init=%d, Save=%d, Th=%3d, Delay=%2d\n",p->bSwDEn,p->bSwDInit,p->bSwDSave,p->bSwDTh,p->bSwDDelay);
    MsgFunc(" REG: b DPLL           : aC=%d,%d, bC=%d,%d, aS=%d,%d, bS=%d,%d\n",p->aC1, p->aC2, p->bC1, p->bC2, p->aS1, p->aS2, p->bS1, p->bS2);
    MsgFunc(" REG: b FrontCtrl      : INITdelay   =%3d, SFDWindow   =%3d, SymHDRCE    =%3d, SSwindow    =%3d\n",p->INITdelay,p->SFDWindow,p->SymHDRCE,p->SSwindow);
    MsgFunc(" REG: b Carrier Sense  : CQwindow    =%3d, CQThreshold =%3d, CSMode      =%3d, bPwrStepDet =%3d\n",p->CQwindow,p->CQthreshold,p->CSMode,p->bPwrStepDet);
    MsgFunc(" REG: b AGC/ED         : EDwindow    =%3d, EDThreshold =%3d, ThSigLarge  =%3d, bStepLarge  =%3d\n",p->EDwindow,p->EDthreshold,p->bThSigLarge,p->bStepLarge);
    MsgFunc(" REG: b AGC            : bWaitAGC    =%3d, bAGCDelay   =%3d, bMaxUpdateCt=%3d, LgSigAFEVald=%3d\n",p->bWaitAGC, p->bAGCdelay, p->bMaxUpdateCount,p->bLgSigAFEValid);
    MsgFunc("                       : bInitAGain  =%3d, bRefPwr     =%3d, DAmpGainRang=%3d, Pwr100dBm   =%3d\n",p->bInitAGain,p->bRefPwr,p->DAmpGainRange,p->Pwr100dBm);
    MsgFunc(" REG: bLNA Config      : bStepLNA    =%3d, bStepLNA2   =%3d, bThSwitchLNA=%3d, bThSwtchLNA2=%3d\n",p->bStepLNA,p->bStepLNA2,p->bThSwitchLNA,p->bThSwitchLNA2);
    MsgFunc(" REG: bChanEst         : CENSym1,2   =%d,%d, hThreshold1 =%3d, hThreshold2 =%3d, hThreshold3 =%3d\n",p->CENSym1,p->CENSym2,p->hThreshold1,p->hThreshold2,p->hThreshold3);

/*
    MsgFunc(" Raw RegDump           : ");
    for(i=0; i<8; i++) {
        if(i) MsgFunc("                         ");
        MsgFunc("%02X",RegMap[32*i].w8);
        for(j=1; j<32; j++) MsgFunc(",%02X",RegMap[32*i+j].w8);
        MsgFunc("\n");
    }
*/
    MsgFunc(" Clock Phase           : RX.Downmix  =%3d, RX.Downsamp =%3d, RX.ClockGen =%3d, bResamplePhase=%d\n",RX.DownmixPhase,RX.DownsamPhase,RX.clock.InitialPhase,TX.bResamplePhase);
    MsgFunc(" TX.Pad Front,Back     : aPadFront =%5d, aPadBack    =%3d, bPadFront  =%4d, bPackBack   =%3d\n",TX.aPadFront,TX.aPadBack,TX.bPadFront,TX.bPadBack);
    MsgFunc(" Data Converters - DAC : Range = %1.1fVpp, nEff = %d, I/Q Mismatch = %3.2f dB\n",TX.DAC.Vpp,TX.DAC.nEff,TX.DAC.IQdB);
    MsgFunc(" Data Converters - ADC : Range = %1.1fVpp, nEff = %d, Offset = %d, Rounding = %d\n",RX.ADC.Vpp,RX.ADC.nEff,RX.ADC.Offset,RX.ADC.Rounding);
    if(RX.NonRTL.Enabled)
    {
        MsgFunc(" >>> ===================================================================== <<<\n");
        MsgFunc(" >>> NON-RTL PARAMETERS: MaxSamples_CFO = %2d\n",RX.NonRTL.MaxSamples_CFO);
        MsgFunc(" >>>                     DisableCFO = %d, BypassPLL = %d\n",RX.NonRTL.DisableCFO,RX.NonRTL.DisablePLL);
        MsgFunc(" >>> ===================================================================== <<<\n");
    }
    return WI_SUCCESS;
}
// end of Mojave_WriteConfig()

// ================================================================================================
// FUNCTION : Mojave_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus Mojave_ParseLine(wiString text)
{
    wiStatus status;
    int i; char buf[256]; wiUWORD X, Y;

    if( !strstr(text, "Mojave") ) return WI_WARN_PARSE_FAILED; // not a Nevada parameter

    PSTATUS(bMojave_ParseLine(text));

    PSTATUS(wiParse_Boolean(text,"Mojave.TraceKeyOp",              &(RX.TraceKeyOp)));
    PSTATUS(wiParse_Boolean(text,"Mojave.EnableModelIO",           &(RX.EnableModelIO)));
    PSTATUS(wiParse_UInt   (text,"Mojave.aPadFront",               &(TX.aPadFront), 0,2000000));
    PSTATUS(wiParse_UInt   (text,"Mojave.aPadBack",                &(TX.aPadBack),  0,  20000));
    PSTATUS(wiParse_UInt   (text,"Mojave.bPadFront",               &(TX.bPadFront), 0,  20000));
    PSTATUS(wiParse_UInt   (text,"Mojave.bPadBack",                &(TX.bPadBack),  0,  20000));
    PSTATUS(wiParse_UInt   (text,"Mojave.RX.InitialPhase",         &(RX.clock.InitialPhase), 0, 80));
    PSTATUS(wiParse_Int    (text,"Mojave.RX.DownsamPhase",         &(RX.DownsamPhase),     0,  1));
    PSTATUS(wiParse_Int    (text,"Mojave.RX.DownmixPhase",         &(RX.DownmixPhase),     0,  3));
    PSTATUS(wiParse_Int    (text,"Mojave.RX.ViterbiType",          &(RX.ViterbiType),      0,  0));
    PSTATUS(wiParse_Boolean(text,"Mojave.TX.SetPBCC",              &(TX.SetPBCC)));

    PSTATUS(wiParse_Int    (text,"Mojave.ADC.Offset",              &(RX.ADC.Offset), -256, 256));
    PSTATUS(wiParse_Int    (text,"Mojave.ADC.nEff",                &(RX.ADC.nEff), 2, 10));
    PSTATUS(wiParse_Real   (text,"Mojave.ADC.Vpp",                 &(RX.ADC.Vpp),  0.5, 3.0));
    PSTATUS(wiParse_Boolean(text,"Mojave.ADC.Rounding",            &(RX.ADC.Rounding)));
    PSTATUS(wiParse_Int    (text,"Mojave.DAC.nEff",                &(TX.DAC.nEff), 2, 10));
    PSTATUS(wiParse_Real   (text,"Mojave.DAC.Vpp",                 &(TX.DAC.Vpp),  0.5, 3.0));
    PSTATUS(wiParse_Real   (text,"Mojave.DAC.IQdB",                &(TX.DAC.IQdB),-6.0, 6.0));
    PSTATUS(wiParse_Boolean(text,"Mojave.EVM.Enabled",             &(RX.EVM.Enabled)));   
    PSTATUS(wiParse_Int    (text,"Mojave.TX.bResamplePhase",       &(TX.bResamplePhase),  0,19));

    PSTATUS(wiParse_Boolean(text,"Mojave.RX.NonRTL.Enabled",       &(RX.NonRTL.Enabled)));
    PSTATUS(wiParse_UInt   (text,"Mojave.RX.NonRTL.MaxSamples_CFO",&(RX.NonRTL.MaxSamples_CFO), 0, 160));
    PSTATUS(wiParse_Boolean(text,"Mojave.RX.NonRTL.DisableCFO",    &(RX.NonRTL.DisableCFO)));
    PSTATUS(wiParse_Boolean(text,"Mojave.RX.NonRTL.DisablePLL",    &(RX.NonRTL.DisablePLL)));

    PSTATUS(wiParse_Boolean(text,"Mojave.RX.TX.Enabled",           &(RX.TX.Enabled)))7;
    PSTATUS(wiParse_UInt   (text,"Mojave.RX.TX.k1",                &(RX.TX.k1),      0, 1<<20));
    PSTATUS(wiParse_UInt   (text,"Mojave.RX.TX.k2",                &(RX.TX.k2),      0, 1<<20));
    PSTATUS(wiParse_UInt   (text,"Mojave.RX.TX.kTxDone",           &(RX.TX.kTxDone), 0, 1<<20));

    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PathSel",        &(Register.PathSel.word),         0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RXMode",         &(Register.RXMode.word),          0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.UpmixConj",      &(Register.UpmixConj.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DownmixConj",    &(Register.DownmixConj.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DmixEnDis",      &(Register.DmixEnDis.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.UpmixDown",      &(Register.UpmixDown.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DownmixUp",      &(Register.DownmixUp.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.UpmixOff",       &(Register.UpmixOff.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DownmixOff",     &(Register.DownmixOff.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ADCBypass",      &(Register.ADCBypass.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TestMode",       &(Register.TestMode.word),        0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PrdDACSrcR",     &(Register.PrdDACSrcR.word),      0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PrdDACSrcI",     &(Register.PrdDACSrcI.word),      0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TxExtend",       &(Register.TxExtend.word),        0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TxDelay",        &(Register.TxDelay.word),         0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TX_RATE",        &(Register.TX_RATE.word),         0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TX_PWRLVL",      &(Register.TX_PWRLVL.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TxFault",        &(Register.TxFault.word),         0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TX_PRBS",        &(Register.TX_PRBS.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TX_SERVICE",     &(Register.TX_SERVICE.word),      0, 0xFFFF));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RxFault",        &(Register.RxFault.word),         0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RX_RATE",        &(Register.RX_RATE.word),         0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RSSI0",          &(Register.RSSI0.word),           0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RSSI1",          &(Register.RSSI1.word),           0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.maxLENGTH",      &(Register.maxLENGTH.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.Disable72",      &(Register.Disable72.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.Reserved0",      &(Register.Reserved0.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PilotPLLEn",     &(Register.PilotPLLEn.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.aC",             &(Register.aC.word),              0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bC",             &(Register.bC.word),              0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.cC",             &(Register.cC.word),              0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RefPwrDig",      &(Register.RefPwrDig.word),       0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.InitAGain",      &(Register.InitAGain.word),       0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.AbsPwrL",        &(Register.AbsPwrL.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.AbsPwrH",        &(Register.AbsPwrH.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThSigLarge",     &(Register.ThSigLarge.word),      0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThSigSmall",     &(Register.ThSigSmall.word),      0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepLgSigDet",   &(Register.StepLgSigDet.word),    0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepLarge",      &(Register.StepLarge.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepSmall",      &(Register.StepSmall.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThSwitchLNA",    &(Register.ThSwitchLNA.word),     0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepLNA",        &(Register.StepLNA.word),         0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThSwitchLNA2",   &(Register.ThSwitchLNA2.word),    0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepLNA2",       &(Register.StepLNA2.word),        0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.Pwr100dBm",      &(Register.Pwr100dBm.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThCCA1",         &(Register.ThCCA1.word),          0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThCCA2",         &(Register.ThCCA2.word),          0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FixedGain",      &(Register.FixedGain.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FixedLNAGain",   &(Register.FixedLNAGain.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FixedLNA2Gain",  &(Register.FixedLNA2Gain.word),   0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.UpdateOnLNA",    &(Register.UpdateOnLNA.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.MaxUpdateCount", &(Register.MaxUpdateCount.word),  0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThStepUpRefPwr", &(Register.ThStepUpRefPwr.word),  0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThStepUp",       &(Register.ThStepUp.word),        0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThStepDown",     &(Register.ThStepDown.word),      0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThDFSUp",        &(Register.ThDFSUp.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ThDFSDn",        &(Register.ThDFSDn.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSOn",          &(Register.DFSOn.word),           0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSCCA",         &(Register.DFSCCA.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFS2Candidates", &(Register.DFS2Candidates.word),  0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSSmooth",      &(Register.DFSSmooth.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSMACFilter",   &(Register.DFSMACFilter.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSHdrFilter",   &(Register.DFSHdrFilter.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSIntDet",      &(Register.DFSIntDet.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSPattern",     &(Register.DFSPattern.word),      0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSminPPB",      &(Register.DFSminPPB.word),       0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSdTPRF",       &(Register.DFSdTPRF.word),        0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSmaxWidth",    &(Register.DFSmaxWidth.word),     0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSmaxTPRF",     &(Register.DFSmaxTPRF.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSminTPRF",     &(Register.DFSminTPRF.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSminGap",      &(Register.DFSminGap.word),       0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DFSPulseSR",     &(Register.DFSPulseSR.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.detRadar",       &(Register.detRadar.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.detHeader",      &(Register.detHeader.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.detPreamble",    &(Register.detPreamble.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.detStepDown",    &(Register.detStepDown.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.detStepUp",      &(Register.detStepUp.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.WaitAGC",        &(Register.WaitAGC.word),         0, 127));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayAGC",       &(Register.DelayAGC.word),        0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetWindow",   &(Register.SigDetWindow.word),    0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SyncShift",      &(Register.SyncShift.word),       0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayRestart",   &(Register.DelayRestart.word),    0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.OFDMSwDEn",      &(Register.OFDMSwDEn.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.OFDMSwDSave",    &(Register.OFDMSwDSave.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.OFDMSwDInit",    &(Register.OFDMSwDInit.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.OFDMSwDDelay",   &(Register.OFDMSwDDelay.word),    0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.OFDMSwDTh",      &(Register.OFDMSwDTh.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CFOFilter",      &(Register.CFOFilter.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ArmLgSigDet3",   &(Register.ArmLgSigDet3.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LgSigAFEValid",  &(Register.LgSigAFEValid.word),   0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.IdleFSEnB",      &(Register.IdleFSEnB.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.IdleSDEnB",      &(Register.IdleSDEnB.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.IdleDAEnB",      &(Register.IdleDAEnB.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayCFO",       &(Register.DelayCFO.word),        0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayRSSI",      &(Register.DelayRSSI.word),       0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetTh1",      &(Register.SigDetTh1.word),       0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetTh2H",     &(Register.SigDetTh2H.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetTh2L",     &(Register.SigDetTh2L.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetDelay",    &(Register.SigDetDelay.word),     0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SigDetFilter",   &(Register.SigDetFilter.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SyncFilter",     &(Register.SyncFilter.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SyncMag",        &(Register.SyncMag.word),         0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SyncThSig",      &(Register.SyncThSig.word),       0, 15));
    PSTATUS(wiParse_Int    (text,"Mojave.Register.SyncThExp",      &(Register.SyncThExp.word),      -8, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SyncDelay",      &(Register.SyncDelay.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.MinShift",       &(Register.MinShift.word),        0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ChEstShift0",    &(Register.ChEstShift0.word),     0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ChEstShift1",    &(Register.ChEstShift1.word),     0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.aC1",            &(Register.aC1.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bC1",            &(Register.bC1.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.aS1",            &(Register.aS1.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bS1",            &(Register.bS1.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.aC2",            &(Register.aC2.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bC2",            &(Register.bC2.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.aS2",            &(Register.aS2.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bS2",            &(Register.bS2.word),             0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXSelect",      &(Register.DCXSelect.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXFastLoad",    &(Register.DCXFastLoad.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXHoldAcc",     &(Register.DCXHoldAcc.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXShift",       &(Register.DCXShift.word),        0, 7));
    PSTATUS(wiParse_Int    (text,"Mojave.Register.DCXAcc0RH",      &(Register.DCXAcc0RH.word),     -16, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXAcc0RL",      &(Register.DCXAcc0RL.word),       0, 255));
    PSTATUS(wiParse_Int    (text,"Mojave.Register.DCXAcc0IH",      &(Register.DCXAcc0IH.word),     -16, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXAcc0IL",      &(Register.DCXAcc0IL.word),       0, 255));
    PSTATUS(wiParse_Int    (text,"Mojave.Register.DCXAcc1RH",      &(Register.DCXAcc1RH.word),     -16, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXAcc1RL",      &(Register.DCXAcc1RL.word),       0, 255));
    PSTATUS(wiParse_Int    (text,"Mojave.Register.DCXAcc1IH",      &(Register.DCXAcc1IH.word),     -16, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DCXAcc1IL",      &(Register.DCXAcc1IL.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.INITdelay",      &(Register.INITdelay.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.EDwindow",       &(Register.EDwindow.word),        0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CQwindow",       &(Register.CQwindow.word),        0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SSwindow",       &(Register.SSwindow.word),        0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CQthreshold",    &(Register.CQthreshold.word),     0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.EDthreshold",    &(Register.EDthreshold.word),     0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bWaitAGC",       &(Register.bWaitAGC.word),        0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bAGCdelay",      &(Register.bAGCdelay.word),       0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bRefPwr",        &(Register.bRefPwr.word),         0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bInitAGain",     &(Register.bInitAGain.word),      0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThSigLarge",    &(Register.bThSigLarge.word),     0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bStepLarge",     &(Register.bStepLarge.word),      0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThSwitchLNA",   &(Register.bThSwitchLNA.word),    0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bStepLNA",       &(Register.bStepLNA.word),        0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThSwitchLNA2",  &(Register.bThSwitchLNA2.word),   0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bStepLNA2",      &(Register.bStepLNA2.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bPwr100dBm",     &(Register.bPwr100dBm.word),      0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DAmpGainRange",  &(Register.DAmpGainRange.word),   0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bMaxUpdateCount",&(Register.bMaxUpdateCount.word), 0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CSMode",         &(Register.CSMode.word),          0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bPwrStepDet",    &(Register.bPwrStepDet.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bCFOQual",       &(Register.bCFOQual.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bAGCBounded4",   &(Register.bAGCBounded4.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bLgSigAFEValid", &(Register.bLgSigAFEValid.word),  0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bSwDEn",         &(Register.bSwDEn.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bSwDSave",       &(Register.bSwDSave.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bSwDInit",       &(Register.bSwDInit.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bSwDDelay",      &(Register.bSwDDelay.word),       0, 31));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bSwDTh",         &(Register.bSwDTh.word),          0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThStepUpRefPwr",&(Register.bThStepUpRefPwr.word), 0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThStepUp",      &(Register.bThStepUp.word),       0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.bThStepDown",    &(Register.bThStepDown.word),     0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SymHDRCE",       &(Register.SymHDRCE.word),        0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CENSym1",        &(Register.CENSym1.word),         0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CENSym2",        &(Register.CENSym2.word),         0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.hThreshold1",    &(Register.hThreshold1.word),     0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.hThreshold2",    &(Register.hThreshold2.word),     0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.hThreshold3",    &(Register.hThreshold3.word),     0, 7));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SFDWindow",      &(Register.SFDWindow.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.AntSel",         &(Register.AntSel.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.CFO",            &(Register.CFO.word),             0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RxFaultCount",   &(Register.RxFaultCount.word),    0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RestartCount",   &(Register.RestartCount.word),    0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.NuisanceCount",  &(Register.NuisanceCount.word),   0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RSSIdB",         &(Register.RSSIdB.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ClrOnWake",      &(Register.ClrOnWake.word),       0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ClrOnHdr",       &(Register.ClrOnHdr.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ClrNow",         &(Register.ClrNow.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.Filter64QAM",    &(Register.Filter64QAM.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.Filter16QAM",    &(Register.Filter16QAM.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FilterQPSK",     &(Register.FilterQPSK.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FilterBPSK",     &(Register.FilterBPSK.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.FilterAllTypes", &(Register.FilterAllTypes.word),  0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.AddrFilter",     &(Register.AddrFilter.word),      0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA0",           &(Register.STA0.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA1",           &(Register.STA1.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA2",           &(Register.STA2.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA3",           &(Register.STA3.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA4",           &(Register.STA4.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.STA5",           &(Register.STA5.word),            0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.minRSSI",        &(Register.minRSSI.word),         0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepUpRestart",  &(Register.StepUpRestart.word),   0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.StepDownRestart",&(Register.StepDownRestart.word), 0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.NoStepAfterSync",&(Register.NoStepAfterSync.word), 0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.NoStepAfterHdr", &(Register.NoStepAfterHdr.word),  0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.KeepCCA_New",    &(Register.KeepCCA_New.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.KeepCCA_Lost",   &(Register.KeepCCA_Lost.word),    0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.WakeupTimeH",    &(Register.WakeupTimeH.word),     0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.WakeupTimeL",    &(Register.WakeupTimeL.word),     0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayStdby",     &(Register.DelayStdby.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayBGEnB",     &(Register.DelayBGEnB.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayADC1",      &(Register.DelayADC1.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayADC2",      &(Register.DelayADC2.word),       0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayModem",     &(Register.DelayModem.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayDFE",       &(Register.DelayDFE.word),        0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayRFSW",      &(Register.DelayRFSW.word),       0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TxRxTime",       &(Register.TxRxTime.word),        0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayFCAL1",     &(Register.DelayFCAL1.word),      0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayPA",        &(Register.DelayPA.word),         0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayFCAL2",     &(Register.DelayFCAL2.word),      0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayState28",   &(Register.DelayState28.word),    0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayShutdown",  &(Register.DelayShutdown.word),   0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelaySleep",     &(Register.DelaySleep.word),      0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.TimeExtend",     &(Register.TimeExtend.word),      0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SquelchTime",    &(Register.SquelchTime.word),     0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelaySquelchRF", &(Register.DelaySquelchRF.word),  0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DelayClearDCX",  &(Register.DelayClearDCX.word),   0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SelectCCA",      &(Register.SelectCCA.word),       0, 15));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RadioMC",        &(Register.RadioMC.word),         0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.DACOPM",         &(Register.DACOPM.word),          0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ADCAOPM",        &(Register.ADCAOPM.word),         0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ADCBOPM",        &(Register.ADCBOPM.word),         0, 63));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RFSW1",          &(Register.RFSW1.word),           0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.RFSW2",          &(Register.RFSW2.word),           0, 255));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNAEnB",         &(Register.LNAEnB.word),          0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PA24_MODE",      &(Register.PA24_MODE.word),       0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.PA50_MODE",      &(Register.PA50_MODE.word),       0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNA24A_MODE",    &(Register.LNA24A_MODE.word),     0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNA24B_MODE",    &(Register.LNA24B_MODE.word),     0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNA50A_MODE",    &(Register.LNA50A_MODE.word),     0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNA50B_MODE",    &(Register.LNA50B_MODE.word),     0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.LNAxxBOnSW2",    &(Register.LNAxxBOnSW2.word),     0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SREdge",         &(Register.SREdge.word),          0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.SRFreq",         &(Register.SRFreq.word),          0, 3));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ADCFCAL1",       &(Register.ADCFCAL1.word),        0, 1));
    PSTATUS(wiParse_UInt   (text,"Mojave.Register.ADCFCAL2",       &(Register.ADCFCAL2.word),        0, 1));
    PSTATUS(wiParse_Boolean(text,"Mojave.TX.UseForcedSIGNAL",      &(TX.UseForcedSIGNAL)                 ));
    PSTATUS(wiParse_UInt   (text,"Mojave.TX.ForcedSIGNAL",         &(TX.ForcedSIGNAL.word),   0, 16777215));
    PSTATUS(wiParse_Boolean(text,"Mojave.TX.UseForcedbHeader",     &(TX.UseForcedbHeader)                ));
    PSTATUS(wiParse_UInt   (text,"Mojave.TX.ForcedbHeader",        &(TX.ForcedbHeader.word), 0,0xFFFFFFFF));
    PSTATUS(wiParse_Boolean(text,"Mojave.RX.MAC_NOFILTER",         &(RX.MAC_NOFILTER)                    ));

    // -----------
    // Array Sizes
    // -----------
    status = WICHK(wiParse_Boolean(text,"Mojave.EnableTrace", &(RX.EnableTrace)));
    if(status==WI_SUCCESS)
    {
        XSTATUS(Mojave_RX_AllocateMemory()); // make sure trace arrays are allocated
        return WI_SUCCESS;
    }
    else if(status<0) return status;

    // -----------
    // Array Sizes
    // -----------
    status = STATUS(wiParse_UInt(text, "Mojave.aLength", &(RX.aLength), 1, 16+8*4096+6));
    if(status == WI_SUCCESS) {
        XSTATUS(Mojave_RX_AllocateMemory());
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_UInt(text, "Mojave.rLength", &(RX.rLength), 1, 1000000));
    if(status == WI_SUCCESS) {
        XSTATUS(Mojave_RX_AllocateMemory());
        return WI_SUCCESS;
    }
    // --------------
    // PLL parameters
    // --------------
    for(i=0; i<16; i++)
    {
        char param[256];
        sprintf(param, "Mojave.Register.Sa_Sb[%d]",i);
        PSTATUS(wiParse_UInt(text,param, &(Register.Sa_Sb[i].word), 0, 63));
        sprintf(param, "Mojave.Register.Ca_Cb[%d]",i);
        PSTATUS(wiParse_UInt(text,param, &(Register.Ca_Cb[i].word), 0, 63));
    }
    status = STATUS(wiParse_UInt(text,"Mojave.DPLL.Sa_Sb",&(X.word), 0, 63));
    if(status == WI_SUCCESS) {
        for(i=0; i<16; i++)
            Register.Sa_Sb[i] = X;
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_UInt(text,"Mojave.DPLL.Ca_Cb",&(X.word), 0, 63));
    if(status == WI_SUCCESS) {
        for(i=0; i<16; i++)
            Register.Ca_Cb[i] = X;
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Command(text,"Mojave_RESET()"));
    if(status == WI_SUCCESS) {
        STATUS(Mojave_RESET());
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Function(text,"Mojave_SaveRegisters(%255s)",&buf));
    if(status == WI_SUCCESS) {
        XSTATUS(Mojave_WriteRegisterMap(RegMap));
        XSTATUS(Mojave_SaveRegisters(RegMap, buf));
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Function(text,"Mojave_LoadRegisters(%255s)",&buf));
    if(status == WI_SUCCESS) {
        XSTATUS(Mojave_WriteRegisterMap(RegMap));
        XSTATUS(Mojave_LoadRegisters(RegMap, buf));
        XSTATUS(Mojave_ReadRegisterMap(RegMap));
        return WI_SUCCESS;
    }
    status = STATUS(wiParse_Function(text,"Mojave_WriteRegister(%d,%d)",&(X.word),&(Y.word)));
    if(status == WI_SUCCESS)
    {
        XSTATUS(Mojave_WriteRegister(X.word, Y.word));
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of Mojave_ParseLine()

// ================================================================================================
// FUNCTION : Mojave_RX_NuisanceFilter()
// ------------------------------------------------------------------------------------------------
// Purpose  : Look for "nuisance" packets: unicast packets with non-matching STA address
// ================================================================================================
void Mojave_RX_NuisanceFilter( wiBoolean CheckNuisance, wiBoolean MAC_NOFILTER, 
                               Mojave_CtrlState SigIn, Mojave_CtrlState *SigOut )
{
    static wiBitDelayLine DataValid = {0};

    SigOut->NuisancePacket = 0; // default result

    if(Register.AddrFilter.b0 && CheckNuisance && !MAC_NOFILTER)
    {
        ClockDelayLine(DataValid, (RX.N_PHY_RX_WR==18)? 1:0); // 8 Byte PHY Control Header + First 10 Bytes of MAC Header
        if(DataValid.delay_13T && !DataValid.delay_14T)       // Adjust timing to match RTL]
        {
            wiUWORD   Protocol  = {(RX.PHY_RX_D[8].w8 >> 0) & 0x3}; // bits [1:0]
            wiUWORD   FrameType = {(RX.PHY_RX_D[8].w8 >> 2) & 0x3}; // bits [3:2]
            wiUWORD  *Address1  = RX.PHY_RX_D + 12;                 // first byte of Address 1
            wiUWORD   ModType   = {(RX.PHY_RX_D[2].w8 >> 6) & 0x3}; // bits [7:6]
            wiBoolean GroupAddr = Address1[0].b0;                   // bit 0 -- Broadcast/Multicast
            wiBoolean ValidRSSI = max(RX.PHY_RX_D[0].w8, RX.PHY_RX_D[1].w8) >= Register.minRSSI.w8;
            wiBoolean DSSSCCK   = RX.PHY_RX_D[7].b7;                // 0=OFDM, 1=DSSS/CCK
            wiBoolean ValidRate;
            wiBoolean ValidType = (FrameType.w2==2) || Register.FilterAllTypes.b0;
            wiBoolean AddrMatch = (  (Address1[0].w8 == Register.STA0.w8)
                                &&(Address1[1].w8 == Register.STA1.w8)
                                &&(Address1[2].w8 == Register.STA2.w8)
                                &&(Address1[3].w8 == Register.STA3.w8)
                                &&(Address1[4].w8 == Register.STA4.w8)
                                &&(Address1[5].w8 == Register.STA5.w8) )? 1:0;

            ValidRate = (DSSSCCK || (Register.FilterBPSK.b0  && (ModType.w2 == 3))
                                        || (Register.FilterQPSK.b0  && (ModType.w2 == 1))
                                        || (Register.Filter16QAM.b0 && (ModType.w2 == 2))
                                        || (Register.Filter64QAM.b0 && (ModType.w2 == 0)) )? 1:0;

            SigOut->NuisancePacket = ((Protocol.w2==0) && ValidType && !GroupAddr && !AddrMatch && ValidRate && ValidRSSI)? 1:0;
        }
    }
}
// end of Mojave_RX_NuisanceFilter

// ================================================================================================
// FUNCTION : Mojave_RX1()
// ------------------------------------------------------------------------------------------------
// Purpose  : Front-end top for OFDM Baseband (ADC output to FFT input) with cycle
//            accurate timing.
// Timing   : Call on 40 MHz clock
// ================================================================================================
wiStatus Mojave_RX1( wiBoolean DFEEnB, wiBoolean LgSigAFE, wiUWORD *LNAGainOFDM, wiUWORD *AGainOFDM, 
                            wiUWORD *CCAOFDM, wiUWORD *RSSI0OFDM, wiUWORD *RSSI1OFDM, wiBoolean CSCCK,
                            wiBoolean bPathMux, wiReg *bInR, wiReg *bInI, wiBoolean RestartRX, wiBoolean DFERestart,
                            wiBoolean ClearDCX, Mojave_aSigState *aSigIn, Mojave_aSigState aSigOut)
{
    int k40 = RX.k40;
    int k20 = RX.k20;

    // ----------------------------------------------------------------------
    // Front-End Signal Processing: HPF and Mixer for all modes (802.11a/b/g)
    // ----------------------------------------------------------------------
    Mojave_DSel(RX.qReal[0][k40], RX.qImag[0][k40], RX.qReal[1][k40], RX.qImag[1][k40], 
                RX.rReal[0]+k40,  RX.rImag[0]+k40,  RX.rReal[1]+k40,  RX.rImag[1]+k40, Register.PathSel, 1);

    aSigIn->LargeDCX = 0;
    if(Register.PathSel.b0)
    {
        Mojave_DCX(0, RX.rReal[0][k40], RX.cReal[0]+k40, ClearDCX, aSigIn);
        Mojave_DCX(1, RX.rImag[0][k40], RX.cImag[0]+k40, ClearDCX, aSigIn);
        Mojave_Downmix(0, RX.cReal[0][k40], RX.cImag[0][k40], RX.zReal[0]+k40, RX.zImag[0]+k40, 1, &Register);
    }
    if(Register.PathSel.b1)
    {
        Mojave_DCX(2, RX.rReal[1][k40], RX.cReal[1]+k40, ClearDCX, aSigIn);
        Mojave_DCX(3, RX.rImag[1][k40], RX.cImag[1]+k40, ClearDCX, aSigIn);
        Mojave_Downmix(1, RX.cReal[1][k40], RX.cImag[1][k40], RX.zReal[1]+k40, RX.zImag[1]+k40, 1, &Register);
    }
    // -------------------------------------
    // OFDM Lowpass Filter in 802.11a/g mode
    // -------------------------------------
    if(Register.RXMode.b0)
    {
        aMojave_Downsam(0, RX.zReal[0][k40], RX.zImag[0][k40], RX.yReal[0]+k20, RX.yImag[0]+k20, 1, 1);
        aMojave_Downsam(1, RX.zReal[1][k40], RX.zImag[1][k40], RX.yReal[1]+k20, RX.yImag[1]+k20, 1, 1);
    }

    // --------------------------------------------------
    // Call Digital Front End at 20 MHz in 802.11a/g mode
    // --------------------------------------------------
    if(RX.clock.posedgeCLK20MHz && Register.RXMode.b0)
    {
        aMojave_DigitalFrontEnd(DFEEnB, DFERestart, LgSigAFE,
                                        RX.yReal[0][k20], RX.yImag[0][k20], RX.yReal[1][k20], RX.yImag[1][k20],
                                        RX.xReal[0]+k20,  RX.xImag[0]+k20,  RX.xReal[1]+k20,  RX.xImag[1]+k20, 
                                        LNAGainOFDM, AGainOFDM, RSSI0OFDM, RSSI1OFDM, CCAOFDM, 
                                        &RX, &(Register), CSCCK, RestartRX, aSigIn, aSigOut); 
    }
    // ---------------------------------
    // Input Signal Mux Based on PathSel
    // ---------------------------------
    bInR->D.word = bPathMux? RX.zReal[1][k40].w10 : RX.zReal[0][k40].w10;
    bInI->D.word = bPathMux? RX.zImag[1][k40].w10 : RX.zImag[0][k40].w10;

    return WI_SUCCESS;
}
// end of Mojave_RX1()

// ================================================================================================
// FUNCTION : Mojave_RX2()
// ------------------------------------------------------------------------------------------------
// Purpose  : 
// Timing   : This functions runs at 20 MHz (ASIC contains faster clocks)
// ================================================================================================
wiStatus Mojave_RX2(Mojave_aSigState *aSigIn, Mojave_aSigState aSigOut, Mojave_CtrlState outReg)
{
    static unsigned int nSym; // number of OFDM symbols in the current packet
    static unsigned int kRX2; // time to evaluate next OFDM symbol
    static unsigned int nDone; // offset from last symbol demod to end of decode

    if(!RX.k20) 
    {
        RX.dx = RX.dv = RX.du = RX.dc = 0;
        RX.PacketType = 0; // clear packet type
        nSym = 1;
        kRX2 = 0;
    }
    {
        // OFDM RX2 Alignment (FFT Start for Long Preamble)
        if(aSigOut.PeakFound) 
        {
            RX.x_ofs = RX.k20 + aMOJAVE_FFT_OFFSET;              // position of FFT start for preamble symbol L1
            kRX2     = RX.x_ofs + 3*80 + aMOJAVE_SIGNAL_LATENCY; // time at which SIGNAL decoding finishes
            aSigIn->RxDone = 0;
        }
        // ---------------------------------------------------------------------------------
        // OFDM RX2 SIGNAL DECODE
        // Evaluate at clock when output is valid, offset is x_ofs + 3*80 + decoding latency
        // ---------------------------------------------------------------------------------
        if((outReg.State==11) && outReg.TurnOnOFDM && (RX.k20 == kRX2))
        {
            unsigned n;
            XSTATUS( Mojave_RX2_SIGNAL(aSigIn) );
            kRX2 = RX.du + 64; // time to evaluation next
            nSym = 1;          // set symbol count to 1
            n = 1 + (16 +8*Register.RX_LENGTH.w12 + 6 - 1)%RX.N_DBPS; // number of bits in last symbol
            nDone = n/4 + 36; // excess clocks at 20 MHz
        }
        // ---------------------------------------------------------------------------------
        // OFDM RX2 DATA DEMOD/DEMAP/DECODE
        // ---------------------------------------------------------------------------------
        if((outReg.State>=13) && outReg.TurnOnOFDM && RX.dc && (RX.k20==kRX2))
        {
            if(nSym<RX.N_SYM) {
                XSTATUS( Mojave_RX2_DATA_Demod(nSym++) );
                kRX2 = RX.du + 64;
            }
            else if(nSym==RX.N_SYM) {
                XSTATUS( Mojave_RX2_DATA_Demod(nSym++) );
                kRX2 = RX.kFFT[RX.N_SYM] + aMOJAVE_FFT_DELAY + aMOJAVE_PLL_DELAY + 80 + nDone;
            }
            else  {
                XSTATUS( Mojave_RX2_DATA_Decode() );
                aSigIn->RxDone = 1;
                kRX2 = 0; // done processing: block further RX2 processing
            }
        }
    }
    return WI_SUCCESS;
}
// end of Mojave_RX2()

// ================================================================================================
// FUNCTION : Mojave_RX_Header()
// ------------------------------------------------------------------------------------------------
// Purpose  : Construct PHY header for RX packets
// ================================================================================================
wiStatus Mojave_RX_Header(Mojave_CtrlState outReg, wiUInt AntSel, wiUWORD RSSI0, wiUWORD RSSI1)
{
    wiUWORD RATE, LENGTH;

    if(!outReg.TurnOnCCK) Register.CFO = RX.CFO; // record CFO on a valid OFDM header decode

    // -----------------------------
    // Multiplex RATE, LENGTH values
    // -----------------------------
    RATE.word   = outReg.TurnOnCCK? bRX->bRATE.w4       : Register.RX_RATE.w4;
    LENGTH.word = outReg.TurnOnCCK? bRX->LengthPSDU.w12 : Register.RX_LENGTH.w12; 

    // ---------------------------
    // Construct PHY Header to MAC
    // ---------------------------
    RX.PHY_RX_D[0].word = Register.RSSIdB.b0? (RSSI0.w8*3/4) : RSSI0.w8;
    RX.PHY_RX_D[1].word = Register.RSSIdB.b0? (RSSI1.w8*3/4) : RSSI1.w8;

    RX.PHY_RX_D[2].word = (RATE.w4<<4) | (LENGTH.w12>>8);
    RX.PHY_RX_D[3].word = LENGTH.w8;

    RX.PHY_RX_D[4].word = Register.RxFaultCount.w8;
    RX.PHY_RX_D[5].word = Register.RestartCount.w8;
    RX.PHY_RX_D[6].word = outReg.TurnOnCCK? 0:Register.CFO.w8;

    RX.PHY_RX_D[7].b7   = outReg.TurnOnCCK;                   // set MSB for DSSS/CCK packets
    RX.PHY_RX_D[7].b6   = Register.detRadar.b0;               // DFS radar detect
    RX.PHY_RX_D[7].b5   = AntSel;                             // antenna selection for switched antenna diversity
    RX.PHY_RX_D[7].w4   = min(31, Register.NuisanceCount.w8); // nuisance filter count

    return WI_SUCCESS;
}
// end of Mojave_RX_Header()

// ================================================================================================
// FUNCTION : Mojave_RX()
// ------------------------------------------------------------------------------------------------
// Purpose  : Front-end top for OFDM Baseband (ADC output to FFT input) with cycle
//            accurate timing. For Mojave, this module contains controls from the top
//            level as well as all DSSS/CCK RX processing.
// Timing   : This function runs on the 80 MHz clock
// ================================================================================================
wiStatus Mojave_RX(wiBoolean DW_PHYEnB, Mojave_RadioDigitalIO *Radio, Mojave_DataConvCtrlState *DataConvIn, 
                   Mojave_DataConvCtrlState DataConvOut,Mojave_CtrlState *inReg, Mojave_CtrlState outReg)
{
    static wiUWORD   LNAGainOFDM, LNAGainCCK;
    static wiUWORD   AGainOFDM, AGainCCK;
    static wiUWORD   RSSI0OFDM, RSSI1OFDM, RSSICCK;
    static wiUWORD   CCAOFDM, CCACCK;
    static wiBoolean dLgSigAFE, d2LgSigAFE; // register delayed versions of LgSigAFE

    static wiReg     bInR, bInI;    // input registers for "b" baseband
    static wiUWORD   RSSI0, RSSI1;  // registers for multiplexed RSSI values

    static Mojave_aSigState aSigIn={0}, aSigOut; // signals from OFDM DFE to top level control
    static Mojave_bSigState bSigIn={0}, bSigOut; // signals from CCK FrontCtrl to top level control

    static wiUReg phase80;     //  7b modulo-80 counter at 80 MHz (returns to 0 at 1 MHz rate)
    static wiUReg PktTime;     // 16b timer (count-down) for CCA, nuisance packet, and power sequencing, 1MHz Update
    static wiUReg SqlTime;     //  6b squelch timer (count-down) for radio RX gain changes
    static wiUReg aValidPacketRSSI; // flag valid RSSI and AGC done

    // (Moore) State Machine Outputs
    // declared as static to avoid repetitive assignments in switch-case structure
    static wiBoolean RadioSleep, RadioStandby, RadioTX;
    static wiBoolean PAEnB, AntTxEnB, BGEnB, DACEnB, ADCEnB;
    static wiBoolean ModemSleep, SquelchRX;
    static wiBoolean PacketAbort, NuisanceCCA, RestartRX, DFEEnB, DFERestart;

    static wiBoolean MAC_TX_EN = 0; // MAC: Transmit Enable

    wiBoolean dMAC_TX_EN = 0;       // registered version of MAC_TX_EN
    wiBoolean TxDone     = 0;       // TX Control: TX Complete
    wiBoolean AntSel;
    wiBoolean TxEnB;
    wiBoolean ClearDCX;
    wiBoolean SquelchRF;

    // ------------------------
    // Clock Explicit Registers
    // ------------------------
    if(RX.clock.posedgeCLK80MHz) 
    {
        ClockRegister(PktTime);
        ClockRegister(SqlTime);
        ClockRegister(phase80);
    }
    if(RX.clock.posedgeCLK40MHz)
    {
        ClockRegister(bInR);
        ClockRegister(bInI);
    }
    if(RX.clock.posedgeCLK20MHz) aSigOut = aSigIn; // these registers exist in the OFDM DFE
    if(RX.clock.posedgeCLK22MHz) bSigOut = bSigIn; // these registers exist in the CCK FrontCtrl

    // ----------------------------
    // RX and Top-Level PHY Control
    // ----------------------------
    if(RX.clock.posedgeCLK80MHz)
    {
        // -----------------------------------------
        // Combinational Logic for Aggregate Signals
        // -----------------------------------------
        wiBoolean SigDet     = (outReg.TurnOnOFDM  && aSigOut.SigDet)     ||(outReg.TurnOnCCK   && bSigOut.SigDet     )? 1:0;
        wiBoolean AGCdone    = (Register.RXMode.b0 && aSigOut.AGCdone)    ||(Register.RXMode.b1 && bSigOut.AGCdone    )? 1:0;
        wiBoolean SyncFound  = (Register.RXMode.b0 && aSigOut.SyncFound)  ||(Register.RXMode.b1 && bSigOut.SyncFound  )? 1:0;
        wiBoolean HeaderValid= (Register.RXMode.b0 && aSigOut.HeaderValid)||(Register.RXMode.b1 && bSigOut.HeaderValid)? 1:0;
        wiBoolean RxDone     = (Register.RXMode.b0 && aSigOut.RxDone)     ||(Register.RXMode.b1 && bSigOut.RxDone     )? 1:0;
        wiBoolean NewSig     = Register.StepUpRestart.b0   && ((Register.RXMode.b0 && aSigOut.StepUp)   || (Register.RXMode.b1 && bSigOut.StepUp)  );
        wiBoolean LostSig    = Register.StepDownRestart.b0 && ((Register.RXMode.b0 && aSigOut.StepDown) || (Register.RXMode.b1 && bSigOut.StepDown));
        wiBoolean detRestart = NewSig || LostSig; // detect restart condition

        AntSel = (Register.RXMode.b0? aSigOut.AntSel:bSigOut.AntSel);

        if(aSigOut.RxDone) aSigIn.RxDone = 0; // OFDM RxDone is an 80 MHz pulse (implemented in RX2)

        // ----------------------------
        // Default Counter/Timer Inputs
        // ----------------------------
        phase80.D.word = (phase80.Q.w7 + 1) % 80; // update modulo-80 count
        if(PktTime.Q.w16 && (phase80.Q.w7==79)) PktTime.D.word = PktTime.Q.w16 - 1; // decrement 1us timer
        if(SqlTime.Q.w6) SqlTime.D.word = SqlTime.Q.w6 - 1; // decrement 80 MHz squelch counter

        // ---------------
        // TX Timing Model
        // ---------------
        if(RX.TX.Enabled)
        {
            dMAC_TX_EN  = MAC_TX_EN; // register delay
            MAC_TX_EN   = (RX.k80 >= RX.TX.k1 && RX.k80 < RX.TX.k2)? 1:0;
            TxDone      = (RX.k80 == RX.TX.kTxDone)? 1:0;
        }
        // ------------------------------
        // RX and Top-Level State Machine
        // ------------------------------
        switch(outReg.State)
        {
            // ======================================
            // CONTROL: SLEEP AND SLEEP-TO-RX STARTUP
            // ======================================
            case 0: // PHY Sleep
                BGEnB          = 1;   // Data converter bandgap disabled
                ADCEnB         = 1;   // ADCs disabled
                DACEnB         = 1;   // DACs disabled
                PAEnB          = 1;   // Power amplifier disabled
                AntTxEnB       = 0;   // RF switch in TX position
                DFEEnB         = 1;   // baseband front-end disabled
                ModemSleep     = 1;   // baseband DSP disabled
                SquelchRX      = 1;   // clear RX input at DCX
                NuisanceCCA    = 0;   // clear nuisance packet CCA
                PacketAbort    = 0;   // clear packet abort
                DFERestart     = 0;   // clear DFE restart signal
                RadioSleep     = 1;   // radio sleep 
                RadioStandby   = 1;   // lesser included condition of sleep
                RadioTX        = 0;   // default active state is RX
                RestartRX      = 0;   // clear packet RX restart
                phase80.D.word = 0;   // clear phase counter
                PktTime.D.word = 0;   // clear packet timer
                SqlTime.D.word = 0;   // clear squelch counter
                inReg->PHY_Sleep=1;   // signal PHY in sleep mode
                inReg->State   = 1; 
                break;

            case 1: // PHY Sleep: Wait for Enable
                phase80.D.word = 0; // hold phase counter inactive
                if(!DW_PHYEnB) inReg->State = 2; // startup on DW_PHYEnB --> 0
                break;

            case 2: // PHY Startup: Initiate Wakeup
                PktTime.D.word = (Register.WakeupTimeH.w8 << 8) | (Register.WakeupTimeL.w8);
                RadioSleep     = 0; // bring radio out of sleep mode
                inReg->PHY_Sleep=0; // indicate PHY is no longer in sleep mode
                inReg->State   = 3;
                break;

            case 3: // PHY Startup: Sleep-to-RX Sequencing
                RadioStandby = (PktTime.Q.w16 > Register.DelayStdby.w8)? 1:0; // Radio Standby->RX
                AntTxEnB     = (PktTime.Q.w16 > Register.DelayStdby.w8)? 0:1; // RF switch to RX (1)
                BGEnB        = (PktTime.Q.w16 > Register.DelayBGEnB.w8)? 1:0; // Data converter bandgap enable
                ADCEnB       = (PktTime.Q.w16 > Register.DelayADC1.w8 )? 1:0; // RX ADC Startup
                ModemSleep   = (PktTime.Q.w16 > Register.DelayModem.w8)? 1:0; // Baseband Startup (backoff for FESP)
                SquelchRX    = (PktTime.Q.w16 >=Register.DelayModem.w8)? 1:0; // Baseband Initialization (clear inputs)
                DFEEnB       = (PktTime.Q.w16 > Register.DelayDFE.w8  )? 1:0; // Start post-FESP processing
                inReg->State = (PktTime.Q.w16 > 0                     )? 3:8; // advance when timer reaches 0
                if(DW_PHYEnB) inReg->State = 30;                              // shutdown
                break;

            case 4: inReg->State = 0; break; // unused state

            // ====================================
            // CONTROL: STANDBY FOR NUISANCE PACKET
            // ====================================
            case 5: // Reduce power during nuisance packet
                PktTime.D.word = PktTime.Q.w16 + Register.TimeExtend.w8;           // increase time for IFS+ACK
                Register.NuisanceCount.word = min(255,Register.NuisanceCount.w8+1);// increment nuisance counter
                NuisanceCCA    = 1; // set CCA for duration of nuisance packet
                PacketAbort    = 1; // set packet abort flag
                inReg->State   = 6; // select power-down/up sequence
                break;

            case 6: // PHY Startup: Nuisance Standby-to-RX Sequencing
                NuisanceCCA  =  1;                                            // set CCA for duration of nuisance packet
                PacketAbort  =  0;                                            // clear packet abort flag
                RadioStandby = (PktTime.Q.w16 > Register.DelayStdby.w8)? 1:0; // Radio Standby->RX
                ADCEnB       = (PktTime.Q.w16 > Register.DelayADC2.w8 )? 1:0; // RX ADC Startup
                ModemSleep   = (PktTime.Q.w16 > Register.DelayModem.w8)? 1:0; // Baseband Startup (backoff for FESP)
                SquelchRX    = (PktTime.Q.w16 >=Register.DelayModem.w8)? 1:0; // Baseband Initialization (clear inputs)
                DFEEnB       = (PktTime.Q.w16 > Register.DelayDFE.w8  )? 1:0; // Start post-FESP processing
                     if(DW_PHYEnB     ) inReg->State = 30; // shutdown
                else if(!PktTime.Q.w16) inReg->State =  8; // advance to RX idle when timer reaches 0
                break;

            // ============================
            // CONTROL: TX-to-RX transition
            // ============================
            case 7: // TX-to-RX transition
                RadioTX    = 0; // change radio mode to RX
                DACEnB     = 1; // disable DACs
                PAEnB      = 1; // Power amplifier disabled
                AntTxEnB = (PktTime.Q.w4 > Register.DelayRFSW.w4 )? 0:1; // delay antenna switch to RX
                ADCEnB   = (PktTime.Q.w4 > Register.DelayADC2.w8 )? 1:0; // RX ADC Startup
                DFEEnB   = (PktTime.Q.w4 > Register.DelayDFE.w8  )? 1:0; // Start post-FESP processing
                
                     if(DW_PHYEnB    ) inReg->State = 27; // shutdown
                else if(!PktTime.Q.w4) inReg->State =  8; // advance to RX idle when timer reaches 0
                break;

            // =========================================
            // CONTROL: ACTIVE RECEIVE - IDLE/DEMODULATE
            // =========================================
            case 8: // RX: Idle - Wait for Signal Detect/Carrier Sense
                DFERestart     = 1; // maintain DFE restart signal for RX-to-RX
                NuisanceCCA    = 0; // clear CCA from nuisance filter (state 6)
                RestartRX      = 0; // clear RX packet restart
                SquelchRX      = 0; // clear any residual squelch

                     if(DW_PHYEnB ) inReg->State = 30; // shutdown
                else if(dMAC_TX_EN) inReg->State = 22; // start transmit
                else if(SigDet    ) inReg->State =  9; // signal detect
                break;

            case 9: // RX: AGC
                DFERestart     = 0;                    // clear DFE restart
                     if(DW_PHYEnB ) inReg->State = 30; // shutdown
                else if(dMAC_TX_EN) inReg->State = 22; // start transmit
                else if(AGCdone   ) inReg->State = 10; // AGC complete (RSSI valid)
                break;

            case 10: // RX: Preamble Processing
                     if(DW_PHYEnB  ) inReg->State = 30; // shutdown
                else if(dMAC_TX_EN ) inReg->State = 22; // start transmit
                else if(SyncFound  ) inReg->State = 11; // sync found
                else if(!SigDet    ) inReg->State =  8; // false detect: sync timeout
                else if(detRestart ) inReg->State = 17; // new signal detect
                aSigIn.RxFault     = 0; // clear the RX2_SIGNAL fault flag (RX2)
                aSigIn.HeaderValid = 0; // clear the valid header flag (RX2)
                break;

            case 11: // RX: Frame Synchronized
                     if(DW_PHYEnB  )                                inReg->State = 30; // shutdown
                else if(dMAC_TX_EN )                                inReg->State = 22; // start transmit
                else if(HeaderValid)                                inReg->State = 12; // valid header
                else if(!SyncFound || aSigOut.RxFault) {            inReg->State = 16; // bad packet: bad CRC/SIGNAL
                    Register.RxFaultCount.word = min(255,Register.RxFaultCount.w8+1);  // ...update fault counter
                }
                else if(detRestart && !Register.NoStepAfterSync.b0) inReg->State = 16; // new signal detect
                break;

            case 12: // RX: Valid Header
                Mojave_RX_Header(outReg, AntSel, RSSI0, RSSI1); // once-per-packet processing on valid header
                PktTime.D.word = outReg.TurnOnCCK? bRX->bPacketDuration.w16:RX.aPacketDuration.w16; // load packet duration from PLCP header
                phase80.D.word =  0; // reset phase counter
                inReg->State   = 13;
                break;

            case 13: // RX: Receive PSDU
                     if(DW_PHYEnB)                                 inReg->State = 29; // shutdown
                else if(dMAC_TX_EN)                                inReg->State = 21; // abort RX to start transmit
                else if(outReg.NuisancePacket)                     inReg->State =  5; // nuisance packet...go to standby
                else if(PktTime.Q.w16<=4)                          inReg->State = 14; // near end of packet in air
                else if(detRestart && !Register.NoStepAfterHdr.b0) inReg->State = 16; // new signal detect
                else if(RxDone   )                                 inReg->State = 15; // unexpected finish - restart 
                break;

            case 14: // RX: Near End of PSDU, Lock Out Restart Events
                     if(DW_PHYEnB )       inReg->State = 29; // shutdown
                else if(dMAC_TX_EN )      inReg->State = 21; // abort RX to start transmit
                else if(PktTime.Q.w16==0) inReg->State = 15; // end of packet at antenna
                else if(RxDone   )        inReg->State = 15; // unexpected finish - restart 
                RX.PacketType = outReg.TurnOnCCK? 2:1; // record packet type
                break;

            case 15: // RX: Finish demod and transfer to MAC
                DFERestart     = 1; // restart DFE
                PktTime.D.word = 0; // clear packet timer (for unexpected transition from 14->15 on RxDone)
                     if(DW_PHYEnB)  inReg->State = 29; // shutdown
                else if(dMAC_TX_EN) inReg->State = 21; // abort RX to start transmit
                else if(RxDone   )  inReg->State =  8; // end of packet
                break;

            // ==========================================
            // CONTROL: RX RESTART ON FAULT OR NEW SIGDET
            // ==========================================
            case 16: // RX: Abort packet before restart (enter here after valid header)
                if((NewSig && !Register.KeepCCA_New.b0)||(LostSig && !Register.KeepCCA_Lost.b0)) PktTime.D.word = 0; // reset packet timer (to clear old CCA)
                PacketAbort  =  1;
                inReg->State = 17;
                break;
            
            case 17: // RX: Signal abort/restart on new signal detect or lost signal
                PacketAbort = 0;                  // clear the abort strobe (from state 16)
                RestartRX   = 1;                  // flag restart (on new signal detect or lost signal)
                if(dMAC_TX_EN) inReg->State = 22; // start TX
                else           inReg->State = 18; // continue
                Register.RestartCount.word = min(255, Register.RestartCount.w8+1);
                break;

            case 18: // RX: Wait for end of abort/restart
                RestartRX = 1;                         // hold restart signal
                     if(DW_PHYEnB ) inReg->State = 30; // shutdown PHY
                else if(dMAC_TX_EN) inReg->State = 22; // start transmit
                else if(!SigDet && !SyncFound) inReg->State =  8; // wait for old signal detect to clear
                break;

            case 19: inReg->State = 0; break; // unused state(s)

            // ===============================================================
            // CONTROL: ACTIVE TRANSMIT (descriptive...not functional in WiSE)
            // ===============================================================
            case 20: // request TX abort - (same as state 25, but entry from state 23)
                PAEnB = 1;          // disable or keep disabled the PA
                phase80.D.word = 0; // reset phase counter 
                PktTime.D.word = Register.TxRxTime.w4; // load delay for TX-to-RX phase (state 7)
                     if(TxDone   ) inReg->State = 26;  // TX returns control to RX
                else if(DW_PHYEnB) inReg->State = 27;  // shutdown PHY
                break;
            
            case 21: // Abort RX to Start TX
                DFERestart   =  0; // clear restart from state 18
                PacketAbort  =  1; // signal an RX packet abort
                inReg->State = 22;
                break;

            case 22: // RX-to-TX transition (Entry for normal TX)
                DFERestart = 0; // clear any DFE restart signal
                RestartRX  = 0; // clear any restart signal (states 17, 18)
                DFEEnB     = 1; // disable RX
                ADCEnB     = 1; // disable ADCs
                DACEnB     = 0; // enabled DACs
                AntTxEnB   = 0; // RF switch to TX position
                RadioTX    = 1; // set radio to transmit
                PacketAbort= 0; // clear the abort flag (from state 21)
                phase80.D.word = Register.DelayPA.b0? 40:0;                     // Timer = 0.5us * DelayPA
                PktTime.D.word =(Register.DelayPA.w4>>1) + Register.DelayPA.b0; //   "       "       "
                inReg->State   = 23;
                break;

            case 23: // start TX but delay PA enable
                     if( DW_PHYEnB    ) inReg->State = 27; // abort TX and shutdown
                else if(!dMAC_TX_EN   ) inReg->State = 20; // requested TX abort
                else if(!PktTime.Q.w16) inReg->State = 24; // enable PA and continue with TX
                break;

            case 24: // active TX
                PAEnB          = 0;                     // enable power amp
                phase80.D.word = 0;                     // clear the phase counter
                PktTime.D.word = Register.TxRxTime.w4;  // load delay for TX-to-RX phase (state 7)
                     if(DW_PHYEnB  ) inReg->State = 27; // abort TX and shutdown
                else if(TxDone     ) inReg->State = 26; // TX returns control to RX (normal)
                else if(!dMAC_TX_EN) inReg->State = 25; // requested TX abort
                break;

            case 25: // request TX abort - wait for return of control from TX state machine
                phase80.D.word = 0; // reset phase counter 
                PktTime.D.word = Register.TxRxTime.w4; // load delay for TX-to-RX phase (state 7)
                     if(TxDone   ) inReg->State = 26;  // TX returns control to RX
                else if(DW_PHYEnB) inReg->State = 27;  // shutdown PHY
                break;

            case 26: // wait for MAC to acknowledge end-of-transmission
                RadioTX = 0; // change radio mode to RX
                PAEnB   = 1; // Power amplifier disabled
                     if(!dMAC_TX_EN) inReg->State =  7; // normal end of TX
                else if(DW_PHYEnB )  inReg->State = 27; // shutdown PHY
                break;

            // =====================================
            // CONTROL: SHUTDOWN FROM TX/RX TO SLEEP
            // =====================================
            case 27: // Shutdown from TX or TX-RX Transition
                phase80.D.word = 0; // clear phase counter
                PktTime.D.word = 0; // clear timer
                RadioTX    = 0; // put radio into RX mode
                PAEnB      = 1; // Power amplifier disabled
                AntTxEnB   = 0; // RF switch to TX position (hold position for shutdown from TX)
                BGEnB      = 1; // disable data converter bandgap (full shutdown)
                DACEnB     = 1; // disable DACs
                ADCEnB     = 1; // disable ADCs
                DFEEnB     = 1; // shutdown baseband DFE/FrontCtrl
                ModemSleep = 1; // shutdown all baesband FESP
                SquelchRX  = 1; // clear all RX inputs (at DCX)
                inReg->State = 28; 
                break;

            case 28: // pass-thru radio RX during shutdown
                if(phase80.Q.w8 == 4*Register.DelayState28.w4) inReg->State = 30;
                break;

            case 29: // Abort RX and Shutdown
                DFERestart   =  0; // clear restart from state 18
                PacketAbort  =  1;
                inReg->State = 30;
                break;
                
            case 30: // RX-to-Standby Transition
                phase80.D.word= 0; // reset phase counter
                PktTime.D.word= 0; // clear timer
                PacketAbort   = 0; // clear packet abort from state 29
                RestartRX     = 0; // clear any restart signal (states 17, 18)
                DFERestart    = 0; // clear any DFE restart signal
                AntTxEnB      = 0; // RF switch to TX position (hold position for shutdown from TX)
                BGEnB         = 1; // disable data converter bandgap (full shutdown)
                DACEnB        = 1; // disable DACs
                ADCEnB        = 1; // disable ADCs
                DFEEnB        = 1; // shutdown baseband DFE/FrontCtrl
                ModemSleep    = 1; // shutdown all baesband FESP
                SquelchRX     = 1; // clear all RX inputs (at DCX)
                RadioStandby  = 1; // put radio in standby mode
                NuisanceCCA   = 0; // clear nuisance packet CCA
                inReg->State  = 31;
                break;

            case 31: // Shutdown: Wait for RX-to-Standby Transition
                RadioSleep   = (phase80.Q.w8 < 4*Register.DelaySleep.w4   )?  0:1; // Standby-to-Sleep
                inReg->State = (phase80.Q.w8 < 4*Register.DelayShutdown.w4)? 31:0; // Sleep-to-Shutdown
                break;
    
            default: inReg->State = 0; // for unused states and on RESET
        }
    }
    // ------------------------------------
    // Clear RX Fault Counters If Requested
    // ------------------------------------
    #if defined( MOJAVE_VERSION_1 )
        if(Register.ClrNow.b0 || (Register.ClrOnHdr.b0 && outReg.State==13) || (Register.ClrOnWake.b0 && outReg.State==3))
    #elif defined( MOJAVE_VERSION_1B )
        if(Register.ClrNow.b0 || (Register.ClrOnHdr.b0 && outReg.State==15) || (Register.ClrOnWake.b0 && outReg.State==3))
    #endif
    {
        Register.RxFaultCount.word  = 0;
        Register.RestartCount.word  = 0;
        Register.NuisanceCount.word = 0;
    }
    // -----------------
    // RX Initialization
    // -----------------
    if(DFEEnB || RestartRX || DFERestart)
    {
        inReg->TurnOnOFDM = Register.RXMode.b0;
        inReg->TurnOnCCK  = Register.RXMode.b1;
    }
    if(!RX.k80) 
    {
        RX.dx = RX.dv = RX.du = RX.dc = 0;
        RX.PacketType = 0; // clear packet type
    }
    // -------------------------------------------
    // Register Control Signals as Required by RTL
    // -------------------------------------------
    inReg->RestartRX  = RestartRX;
    inReg->DFERestart = DFERestart;

    // ---------------
    // Squelch Signals
    // ---------------
    ClearDCX  = (SqlTime.Q.w6>4*Register.DelayClearDCX.w4  || SquelchRX)? 1:0; // reset DCX accumulator
    SquelchRF = (SqlTime.Q.w6>4*Register.DelaySquelchRF.w4             )? 1:0; // enable squelch with RF switch

    // --------------------------------------------
    // Common/OFDM Receiver Processing at 20,40 MHz
    // --------------------------------------------
    if(RX.clock.posedgeCLK40MHz)
    {
        // ---------------------
        // Common/OFDM Front End
        // ---------------------
        Mojave_RX1(DFEEnB, d2LgSigAFE, &LNAGainOFDM, &AGainOFDM, &CCAOFDM, &RSSI0OFDM, &RSSI1OFDM,
                 bSigOut.CSCCK, outReg.bPathMux, &bInR, &bInI, outReg.RestartRX, outReg.DFERestart, ClearDCX, &aSigIn, aSigOut);

        if(RX.clock.posedgeCLK20MHz && outReg.TurnOnOFDM)
        {
            wiUWORD RSSI;
            RSSI.word = max(RSSI0OFDM.w8, RSSI1OFDM.w8);
            if(RX.EnableTrace) aMojave_DFS(DFEEnB, aSigOut, &aSigIn, &RX, &(Register));
            XSTATUS( Mojave_RX2(&aSigIn, aSigOut, outReg) );
        }
    }
    // -----------------------------------------
    // DSSS/CCK Receiver Processing at 40,44 MHz
    // -----------------------------------------
    inReg->bRXEnB = DFEEnB || !(Register.RXMode.b1 && outReg.TurnOnCCK); // enable for "b" DSP/FrontCtrl
    if((RX.clock.posedgeCLK40MHz || RX.clock.posedgeCLK44MHz) && Register.RXMode.b1)
    {
        bMojave_RX_top(&RX, bRX, &Register, outReg.bRXEnB, &(bInR.Q), &(bInI.Q), &AGainCCK, &LNAGainCCK, 
                            d2LgSigAFE, &CCACCK, &RSSICCK, aSigOut.Run11b, outReg.RestartRX, &bSigIn, bSigOut);
    }
    // --------------------
    // Processing at 20 MHz
    // --------------------
    if(RX.clock.posedgeCLK20MHz)
    {
        ClockRegister(aValidPacketRSSI);
        aValidPacketRSSI.D.word = aSigOut.ValidRSSI & aSigOut.AGCdone;

        // -----------------------------------------------
        // Double-Buffer LgSigAFE with Registers at 20 MHz
        // -----------------------------------------------
        d2LgSigAFE = dLgSigAFE;
        dLgSigAFE  = Radio->LGSIGAFE;

        // ----------------------------------
        // DSSS/CCK and OFDM Mode Arbitration
        // ----------------------------------
        if((Register.RXMode.w2==3) && (outReg.State==10 || outReg.State==11))
        {
            if(aSigOut.Run11b)    inReg->TurnOnOFDM = 0; // turn off OFDM DSP
            if(aSigOut.PeakFound) inReg->TurnOnCCK  = 0; // turn off CCK DSP
        }
        // --------------------------------------
        // "b" Path Selection for Dual RX "g" Mode
        // ---------------------------------------
        if(outReg.TurnOnOFDM && aSigOut.Run11b && Register.PathSel.w2==3) {
           inReg->bPathMux = (RSSI1OFDM.w8 > RSSI0OFDM.w8)? 1:0; // select strong signal for two antenna case
        }
        else if(!aSigOut.Run11b)
            inReg->bPathMux = Register.PathSel.b0? 0:1;           // otherwise default=0 unless this path is disabled
    }
    // --------------
    // Multiplex RSSI
    // --------------
    if(Register.RXMode.b0) { // 802.11a/g
        if(aValidPacketRSSI.D.b0 && !aValidPacketRSSI.Q.b0) { // record RSSI when it becomes valid after AGC
            RSSI0.word = RSSI0OFDM.w8;
            RSSI1.word = RSSI1OFDM.w8;
        }
    }
    else {                   // 802.11b
        if(outReg.State==10 && bSigOut.SyncFound) { // record RSSI after SFD
            RSSI0.word = RSSICCK.w8;
            RSSI1.word = 0;
        }
    }
    // ----------------------
    // Nuisance Packet Filter
    // ----------------------
    Mojave_RX_NuisanceFilter(outReg.State==13, RX.MAC_NOFILTER, outReg, inReg);

    // ------------------------------------
    // Radio/Data Converter I/O and Control
    // ------------------------------------
    if(RX.EnableModelIO)
    {
        // Transmit Enable ________________________________________________
        TxEnB = (outReg.State==22 || outReg.State==23 || (outReg.State==24 && dMAC_TX_EN))? 0:1;

        // Radio Mode Control ___________________________________________________
             if(RadioSleep  ) Radio->MC = (Register.RadioMC.w8 >> 6) & 0x3; // bits [7:6]: Sleep
        else if(RadioStandby) Radio->MC = (Register.RadioMC.w8 >> 4) & 0x3; // bits [5:4]: Standby
        else if(RadioTX     ) Radio->MC = (Register.RadioMC.w8 >> 0) & 0x3; // bits [1:0]: Transmit
        else                  Radio->MC = (Register.RadioMC.w8 >> 2) & 0x3; // bits [3:2]: Receive

        // Data Converters ___________________________________________________
        {
            wiBoolean sA = Register.PathSel.b0;
            wiBoolean sB = Register.PathSel.b1;

                 if(!ADCEnB & sA) DataConvIn->ADCAOPM = (Register.ADCAOPM.w6 >> 0) & 0x3; // operational
            else if(!BGEnB  & sA) DataConvIn->ADCAOPM = (Register.ADCAOPM.w6 >> 2) & 0x3; // sleep/standby
            else                  DataConvIn->ADCAOPM = (Register.ADCAOPM.w6 >> 4) & 0x3; // power down

            // Select logic based on part version
            //
            #if defined(MOJAVE_VERSION_1B)

                     if(!ADCEnB & sB)    DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 0) & 0x3; // operational
                else if( ADCEnB & BGEnB) DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 4) & 0x3; // power down
                else                     DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 2) & 0x3; // sleep/standby

            #else // original


                     if(!ADCEnB & sB) DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 0) & 0x3; // operational
                else if(!BGEnB  & sB) DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 2) & 0x3; // sleep/standby
                else                  DataConvIn->ADCBOPM = (Register.ADCBOPM.w6 >> 4) & 0x3; // power down

            #endif

                 if(!DACEnB )     DataConvIn->DACOPM  = (Register.DACOPM.w6  >> 0) & 0x3; // operational
            else if(!BGEnB  )     DataConvIn->DACOPM  = (Register.DACOPM.w6  >> 2) & 0x3; // sleep/standby
            else                  DataConvIn->DACOPM  = (Register.DACOPM.w6  >> 4) & 0x3; // power down

            DataConvIn->DACOPM = (DataConvIn->DACOPM << 1) | (DataConvIn->DACOPM & 1);    // 2b DACOPM to 3b OPM mapping

            switch(outReg.State)
            {
                case 3 : DataConvIn->ADCFCAL = Register.ADCFCAL.b0 || (Register.ADCFCAL1.b0 && (PktTime.Q.w16==Register.DelayFCAL1.w4)); break;
                case 6 :
                case 7 : DataConvIn->ADCFCAL = Register.ADCFCAL.b0 || (Register.ADCFCAL2.b0 && (PktTime.Q.w16==Register.DelayFCAL2.w4)); break;
                default: DataConvIn->ADCFCAL = Register.ADCFCAL.b0;
            }
        }
        // External LNAs ______________________________________________________
        {
            wiBoolean c  = (RadioSleep || (RadioStandby && Register.LNAEnB.b1) || (RadioTX && Register.LNAEnB.b0))? 1:0;
            wiBoolean X[4] = {0, 1, c, c^1}; 

            Radio->LNA24A = X[Register.LNA24A_MODE.w2];
            Radio->LNA24B = X[Register.LNA24B_MODE.w2];
            Radio->LNA50A = X[Register.LNA50A_MODE.w2];
            Radio->LNA50B = X[Register.LNA50B_MODE.w2];
        }
        // Power Amplifiers ____________________________________________________
        {
            wiBoolean c  = (RadioSleep || PAEnB)? 1:0;
            wiBoolean X[4] = {0, 1, c, c^1}; 

            Radio->PA24 = X[Register.PA24_MODE.w2];
            Radio->PA50 = X[Register.PA50_MODE.w2];
        }
        // RF Switches _________________________________________________________
        {
            wiUWORD s;  s.b1=AntSel; s.b0=(AntTxEnB && !SquelchRF);
            
            Radio->SW1  =                            (Register.RFSW1.w4      >> s.w2) & 1;
            Radio->SW1B =                            (Register.RFSW1.w8 >> 4 >> s.w2) & 1;
            Radio->SW2  = !Register.LNAxxBOnSW2.b0? ((Register.RFSW2.w4      >> s.w2) & 1) : Radio->LNA24B;
            Radio->SW2B = !Register.LNAxxBOnSW2.b0? ((Register.RFSW2.w8 >> 4 >> s.w2) & 1) : Radio->LNA50B;
        }
        // Radio Gain Control Signals __________________________________________
        {
            if(RadioTX) // transmit mode selection
            {
                wiUWORD TXPWRLVL = Register.TX_PWRLVL; // RTL: TXPWRLVL is first byte on MAC_TX_D; output from TX Control
                Radio->LNAGAIN2  = 1;
                Radio->LNAGAIN   = TXPWRLVL.b5; // TXPWRLVL[5]
                Radio->AGAIN     = TXPWRLVL.w5; // TXPWRLVL[4:0]
            }
            else // receive/sleep mode selection
            {
                Radio->LNAGAIN2 = RadioSleep? 0 : (Register.RXMode.b0? LNAGainOFDM.b1:LNAGainCCK.b1 );
                Radio->LNAGAIN  = RadioSleep? 0 : (Register.RXMode.b0? LNAGainOFDM.b0:LNAGainCCK.b0 );
                Radio->AGAIN    = RadioSleep? 0 : (Register.RXMode.b0? AGainOFDM.w6  :AGainCCK.w6   );
            }
        }
        // Clear Channel Assessment (CCA) _______________________________________
        {
            wiBoolean PowerCCA  = Register.SelectCCA.b0 && (Register.RXMode.b0? CCAOFDM.b1:0);       // busy on power > threshold
            wiBoolean HeaderCCA = (outReg.State==12 || outReg.State==13 || outReg.State==14)? 1:0;   // busy on valid header decode
            wiBoolean SyncCCA   = (outReg.State==11 || HeaderCCA)? 1:0;                              // busy on valid sync
            wiBoolean SigDetCCA = Register.RXMode.b0? CCAOFDM.b0:CCACCK.b0;                          // busy on signal detect
            wiBoolean PacketCCA = (Register.SelectCCA.b3 && SigDetCCA)||(Register.SelectCCA.b2 && SyncCCA) 
                               || (Register.SelectCCA.b1 && HeaderCCA)|| NuisanceCCA;                // select timer CCA input
            inReg->TimerCCA = PacketCCA || (PktTime.Q.w16 && outReg.TimerCCA);                       // hold as busy for duration of packet
            inReg->PHY_CCA  = PowerCCA  || outReg.TimerCCA;                                          // combine packet and power CCA
//         wiPrintf("%X> %d%d%d%d %d%d %d\n",Register.SelectCCA.w4,PowerCCA,HeaderCCA,SyncCCA,SigDetCCA,PacketCCA,outReg.TimerCCA,outReg.PHY_CCA);
        }
    }
    else // simple I/O
    {
        Radio->MC           = (RadioSleep || RadioStandby)? 0:3; // Sleep:Receive switching
        Radio->SW2          = AntSel; // follow AntSel
        DataConvIn->ADCAOPM = 3;      // enabled
        DataConvIn->ADCBOPM = 3;      // enabled

        Radio->LNAGAIN2 = RadioSleep? 0 : (Register.RXMode.b0? LNAGainOFDM.b1:LNAGainCCK.b1 );
        Radio->LNAGAIN  = RadioSleep? 0 : (Register.RXMode.b0? LNAGainOFDM.b0:LNAGainCCK.b0 );
        Radio->AGAIN    = RadioSleep? 0 : (Register.RXMode.b0? AGainOFDM.w6  :AGainCCK.w6   );
    }
    // -------------------------------------------
    // Start Squelch Timer on Gain or Mode Changes
    // -------------------------------------------
    if(!RadioSleep && !RadioTX)
    {
        inReg->dLNAGain2 = Radio->LNAGAIN2; // register gain state for edge detector below
        inReg->dLNAGain  = Radio->LNAGAIN;
        inReg->dAGain    = Radio->AGAIN;
        if(DFEEnB || (Radio->LNAGAIN2 != outReg.dLNAGain2) || (Radio->LNAGAIN != outReg.dLNAGain) || (Radio->AGAIN != outReg.dAGain))
            SqlTime.D.word = Register.SquelchTime.w6; // set time on gain change or DFEEnB->0
    }
    // -------------
    // Trace Signals
    // -------------
    if(RX.EnableTrace)
    {
        if(RX.clock.posedgeCLK20MHz) RX.aSigOut[RX.k20] = aSigOut;
        if(RX.clock.posedgeCLK22MHz) RX.bSigOut[RX.k22] = bSigOut;

        RX.traceRxState[RX.k80].traceValid      = 1;
        RX.traceRxState[RX.k80].posedgeCLK20MHz = RX.clock.posedgeCLK20MHz;
        RX.traceRxState[RX.k80].posedgeCLK22MHz = RX.clock.posedgeCLK22MHz;
        RX.traceRxState[RX.k80].posedgeCLK44MHz = RX.clock.posedgeCLK44MHz;
        RX.traceRxState[RX.k80].State           = outReg.State;
        RX.traceRxState[RX.k80].PktTime         = PktTime.Q.w16;
        RX.traceRxState[RX.k80].phase80         = phase80.Q.w7;

        RX.traceRxControl[RX.k80].DW_PHYEnB      = DW_PHYEnB;
        RX.traceRxControl[RX.k80].PHY_Sleep      = outReg.PHY_Sleep;
        RX.traceRxControl[RX.k80].MAC_TX_EN      = MAC_TX_EN;
        RX.traceRxControl[RX.k80].PHY_TX_DONE    = 0; // not used
        RX.traceRxControl[RX.k80].PHY_CCA        = outReg.PHY_CCA;
        RX.traceRxControl[RX.k80].PHY_RX_EN      = 0; // not used
        RX.traceRxControl[RX.k80].PHY_RX_ABORT   = 0; // not used
        RX.traceRxControl[RX.k80].TxEnB          = TxEnB;
        RX.traceRxControl[RX.k80].TxDone         = TxDone;
        RX.traceRxControl[RX.k80].RadioSleep     = RadioSleep;
        RX.traceRxControl[RX.k80].RadioStandby   = RadioStandby;
        RX.traceRxControl[RX.k80].RadioTX        = RadioTX;
        RX.traceRxControl[RX.k80].PAEnB          = PAEnB;
        RX.traceRxControl[RX.k80].AntTxEnB       = AntTxEnB;
        RX.traceRxControl[RX.k80].BGEnB          = BGEnB;
        RX.traceRxControl[RX.k80].DACEnB         = DACEnB;
        RX.traceRxControl[RX.k80].ADCEnB         = ADCEnB;
        RX.traceRxControl[RX.k80].ModemSleep     = ModemSleep;
        RX.traceRxControl[RX.k80].PacketAbort    = PacketAbort;
        RX.traceRxControl[RX.k80].NuisanceCCA    = NuisanceCCA;
        RX.traceRxControl[RX.k80].RestartRX      = RestartRX;
        RX.traceRxControl[RX.k80].DFEEnB         = DFEEnB;
        RX.traceRxControl[RX.k80].DFERestart     = DFERestart;
        RX.traceRxControl[RX.k80].TurnOnOFDM     = outReg.TurnOnOFDM;
        RX.traceRxControl[RX.k80].TurnOnCCK      = outReg.TurnOnCCK;
        RX.traceRxControl[RX.k80].NuisancePacket = outReg.NuisancePacket;
        RX.traceRxControl[RX.k80].ClearDCX       = ClearDCX;
        RX.traceRxControl[RX.k80].bPathMux       = outReg.bPathMux;
        RX.traceRxControl[RX.k80].posedgeCLK40MHz= RX.clock.posedgeCLK40MHz;
        RX.traceRxControl[RX.k80].posedgeCLK11MHz= RX.clock.posedgeCLK11MHz;
    }
    return WI_SUCCESS;
}
// end of Mojave_RX()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// Post Architecture-Freeze Revisions
//
// 2006-09-10 Brickner: Changed DataConvIn-FCAL to DataConvIn->ADCFCAL to match RTL/interface naming
// 2006-12-19 R.Ewert :   Terminate Viterbi decoder in state zero at end of the packet (Sx=0)
// 2007-01-31 Brickner: Increased noise power accumulator from 14b to 15b in channel estimate per Simon
// 2007-02-20 Brickner: Added 80 MHz register to MAC_TX_EN to match change in RTL
//
// Post Silicon Revisions
//
// 2007-08-06 Brickner: Initialized SERVICE to 0 before ORing received data to the field
// 2007-08-14 Brickner: Modified state to implement ClrOnHdr from 13 to 15 (for metal spin)
// 2007-10-15 Brickner: Added MAC_NOFILTER to the RX state and made  programmable from script parser
//                    : Modified ADCBOPM logic for metal spin
// 2007-12-02 Brickner: Modified Mojave_ADC to implement rounding rather than truncation.
//                      This reduces the quantization error. Because all prior simulations were
//                      based on truncation, a parameter is provided to enable rounding
// 2008-04-29 Brickner: Modified Mojave_RX_Restart to clear downmixer and downsample output arrays.
//                      This prevents problems when a single path is simulated after two path RX
// 2008-08-11 Brickner: Added ForcedbHeader and UseForcedbHeader parameters used in bMojave.c
// 2009-01-07 Brickner: Fixed sign check in ADC rounding model

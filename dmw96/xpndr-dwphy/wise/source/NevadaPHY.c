//--< NevadaPHY.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Model for the 11n handheld PHY (Nevada + Coltrane)
//  Copyright 2007-2010 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiPHY.h"
#include "wiMAC.h"
#include "wiMath.h"
#include "wiUtil.h"
#include "wiParse.h"
#include "NevadaPHY.h"
#include "Nevada.h"
#include "Coltrane.h"

//=================================================================================================
//--  MODULE LOCAL PARAMETERS
//=================================================================================================
static NevadaPHY_State_t State[WISE_MAX_THREADS] = {{0}};

//=================================================================================================
//--  INTERNAL ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg) < 0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((pstatus = WICHK(arg)) <= 0) return pstatus;

// ================================================================================================
// FUNCTION  : NevadaPHY_InitializeThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_InitializeThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        NevadaPHY_State_t *pState = State + ThreadIndex;
        XSTATUS( NevadaPHY_CloseThread(ThreadIndex) )

        memcpy( pState, State, sizeof(NevadaPHY_State_t) );
    
        pState->MAC_TX_D = (wiUWORD *)wiCalloc(65536+8, sizeof(wiUWORD));
        if (!pState->MAC_TX_D) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

        pState->TX.s[0] = pState->TX.s[1] = NULL;
        pState->ExternalRxADC.xI = pState->ExternalRxADC.xQ = NULL;

        XSTATUS( Nevada_InitializeThread(ThreadIndex) );
        XSTATUS( Coltrane_InitializeThread(ThreadIndex) );
        pState->ThreadIsInitialized = WI_TRUE;
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_InitializeThread()

// ================================================================================================
// FUNCTION  : NevadaPHY_CloseThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_CloseThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        NevadaPHY_State_t *pState = State + ThreadIndex;

        WIFREE( pState->MAC_TX_D )
        WIFREE( pState->TX.s[0]  )
        WIFREE( pState->RX.s[0]  )
        WIFREE( pState->RX.s[1]  )
        pState->ThreadIsInitialized = WI_FALSE;

        XSTATUS( Nevada_CloseThread(ThreadIndex) );
        XSTATUS( Coltrane_CloseThread(ThreadIndex) );
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_CloseThread()


// ================================================================================================
// FUNCTION  : NevadaPHY_TX()
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level PHY routine
// Parameters: bps      -- PHY transmission rate (bits/second)
//             Length   -- Number of bytes in "data"
//             data     -- Data to be transmitted
//             Ns       -- Returned number of transmit samples
//             S        -- Array of transmit signals (by ref), only *S[0] is used
//             Fs       -- Sample rate for S (Hz)
//             Prms     -- Complex power in S (rms)
// ================================================================================================
wiStatus NevadaPHY_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    int i, Nx = 0;
    wiUWORD RATE = {0}, MCS = {0}, LENGTH;
    wiBoolean TXbMode, TXnMode;
    wiComplex *x = NULL;

    NevadaPHY_State_t *pState = NevadaPHY_State();

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!data) return STATUS(WI_ERROR_PARAMETER3);
    if (!Ns)   return STATUS(WI_ERROR_PARAMETER4);
    if (!S)    return STATUS(WI_ERROR_PARAMETER5);  
    
    if (!pState->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( NevadaPHY_InitializeThread(wiProcess_ThreadIndex()) )
    }

    if (bps)
    {
        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Generate the PHY control header
        //
        //  This is the usual processing path for WiSE simulations. Given a data rate, this
        //  code generates the PHY header emulating functions performed in the MAC.
        //
        switch ((unsigned)bps)
        {
            case  1000000:   
                if (pState->ShortPreamble) {RATE.word = 0x0; TXbMode = 1; TXnMode = 0; }
                else                       {RATE.word = 0x0; TXbMode = 1; TXnMode = 0; }
                break;
            case  2000000:   
                if (pState->ShortPreamble) {RATE.word = 0x5; TXbMode = 1; TXnMode = 0; }
                else                       {RATE.word = 0x1; TXbMode = 1; TXnMode = 0; } 
                break;
            case  5500000:   
                if (pState->ShortPreamble) {RATE.word = 0x6; TXbMode = 1; TXnMode = 0; }
                else                       {RATE.word = 0x2; TXbMode = 1; TXnMode = 0; } 
                break;
            case 11000000:   
                if (pState->ShortPreamble) {RATE.word = 0x7; TXbMode = 1; TXnMode = 0; }
                else                       {RATE.word = 0x3; TXbMode = 1; TXnMode = 0; } 
                break;

            case  6000000: RATE.word = 0xD; TXbMode = 0; TXnMode = 0; break;
            case  9000000: RATE.word = 0xF; TXbMode = 0; TXnMode = 0; break;
            case 12000000: RATE.word = 0x5; TXbMode = 0; TXnMode = 0; break;
            case 18000000: RATE.word = 0x7; TXbMode = 0; TXnMode = 0; break;
            case 24000000: RATE.word = 0x9; TXbMode = 0; TXnMode = 0; break;
            case 36000000: RATE.word = 0xB; TXbMode = 0; TXnMode = 0; break;
            case 48000000: RATE.word = 0x1; TXbMode = 0; TXnMode = 0; break;
            case 54000000: RATE.word = 0x3; TXbMode = 0; TXnMode = 0; break;
            
            case  6500000: MCS.word  =   0; TXbMode = 0; TXnMode = 1; break;
            case 13000000: MCS.word  =   1; TXbMode = 0; TXnMode = 1; break;            
            case 19500000: MCS.word  =   2; TXbMode = 0; TXnMode = 1; break;            
            case 26000000: MCS.word  =   3; TXbMode = 0; TXnMode = 1; break;            
            case 39000000: MCS.word  =   4; TXbMode = 0; TXnMode = 1; break;            
            case 52000000: MCS.word  =   5; TXbMode = 0; TXnMode = 1; break;            
            case 58500000: MCS.word  =   6; TXbMode = 0; TXnMode = 1; break;            
            case 65000000: MCS.word  =   7; TXbMode = 0; TXnMode = 1; break;            

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        if (TXnMode) // 802.11n HT //////////////////////////////////////////////////////
        {
            if (InvalidRange(Length,1,65535)) return STATUS(WI_ERROR_PARAMETER2);
            LENGTH.w16 = (unsigned int)Length;

            pState->MAC_TX_D[0].word = pState->TXPWRLVL.w8;
            pState->MAC_TX_D[1].word = 128 | 64;
            pState->MAC_TX_D[2].word = MCS.w7;
            pState->MAC_TX_D[3].word = LENGTH.w16 >> 8;
            pState->MAC_TX_D[4].word = LENGTH.w8;
            pState->MAC_TX_D[5].word =(pState->TxGreenfield ? 8:0) | (pState->TxOpProtect ? 4:0);
            pState->MAC_TX_D[6].word = pState->TxAggregate ? 32:0;
            pState->MAC_TX_D[7].word = 0;
        }
        else // 802.11a/b/g /////////////////////////////////////////////////////////////
        {
            if (InvalidRange(Length,1,4095)) return STATUS(WI_ERROR_PARAMETER2);
            LENGTH.w12 = (unsigned int)Length;

            pState->MAC_TX_D[0].word = pState->TXPWRLVL.w8;              // TXPWRLVL[7:0]
            pState->MAC_TX_D[1].word = TXbMode ? 128:0;                // Indicate Legacy OFDM or DSSS/CCK modulation
            pState->MAC_TX_D[2].word = (RATE.w4<<4) | (LENGTH.w12>>8); // RATE[3:0]|LENGTH[11:8]
            pState->MAC_TX_D[3].word = LENGTH.w8;                      // LENGTH[7:0]
            pState->MAC_TX_D[4].word = 0;
            pState->MAC_TX_D[5].word = 0;
            pState->MAC_TX_D[6].word = 0;
            pState->MAC_TX_D[7].word = 0;
        }
        for (i = 0; i < Length; i++) 
            pState->MAC_TX_D[i+8].word = data[i].w8;                  // PSDU         
    }
    else
    {  
        //////////////////////////////////////////////////////////////////////////////////////
        //
        //  Extract RATE from the PHY control header
        //
        //  This block allows a byte stream containing the 8-byte PHY header to be processed
        //  by the model. It is included for special test cases and is not normally used.
        //
        //  The code below is extrapolated from the DW52 PHY module but has not been fully
        //  tested in NevadaPHY.
        //
        DEBUG // BB> generate a message...not sure if the code below has been tested

        if (InvalidRange(Length-8, 1, 65535)) return STATUS(WI_ERROR_PARAMETER2);
        TXnMode = data[1].b6;
        if (TXnMode) //11n Mode
        {
            Length = (data[3].w8 << 8) | data[4].w8; // number of data bytes
            TXbMode = 0;                    
            for (i = 0; i < Length + 8; i++)
                pState->MAC_TX_D[i].word = data[i].w8; // PSDU           
        }
        else
        {
            if (InvalidRange(Length-8, 1, 4095)) return STATUS(WI_ERROR_PARAMETER2);
            Length = (data[2].w4 << 8) | data[3].w8; // number of data bytes
            TXbMode = data[1].b7;                    // modulation type: 0=OFDM, 1=DSSS/CCK
            for (i = 0; i < Length + 8; i++)
                pState->MAC_TX_D[i].word = data[i].w8; // PSDU
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    //
    //  Force the PHY TX Header Values
    //
    if (pState->PhyTxHeader.ForceHeader)
    {
        for (i = 0; i < 8; i++)
            pState->MAC_TX_D[i].word = pState->PhyTxHeader.Byte[i] & 0xFF;
    }
    XSTATUS( Nevada_TX_Restart() );
    XSTATUS( Nevada_TX(Length+8, pState->MAC_TX_D, &Nx, &x, pState->M) );
    
    // ---------------------------------
    // Allocate Memory for the TX signal
    // ---------------------------------
    if (!pState->TX.s[0] || (pState->TX.Ns != Nx))
    {
        WIFREE( pState->TX.s[0] );
        pState->TX.s[0] = (wiComplex *)wiCalloc(Nx, sizeof(wiComplex));
        if (!pState->TX.s[0]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
        pState->TX.Ns = Nx;
    }
    // --------
    // TX Radio
    // --------
    XSTATUS( Coltrane_TX_Restart(Nx, 80.0*pState->M, Nevada_Prms()) );
    XSTATUS( Coltrane_TX(Nx, x, pState->TX.s[0], Nx - Nevada_TxState()->OfsEOP) );

    ////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the transmit checksum
    //
    if (pState->Checksum.Enable)
    {
        pState->Checksum.TX = 0;
        wiMath_FletcherChecksum( pState->TX.Ns * sizeof(wiComplex), pState->TX.s[0], &(pState->Checksum.TX) );
    }

    // -----------------------------------------
    // Diagnostic: Dump TX vectors and terminate
    // -----------------------------------------
    if (pState->dumpTX)
    {
        wiPrintf("\n>>> Dumping TX arrays and terminating...\n");
        dump_wiUWORD  ("MAC_TX_D.out",pState->MAC_TX_D, Length+8);
        dump_wiComplex("x.out",x, Nx);
        dump_wiComplex("s.out",pState->TX.s[0],pState->TX.Ns);
        wiUtil_WriteLogFile("Exit after dumpTX\n");
        exit(1);
    }
    // -----------------
    // Output parameters
    // -----------------
    if (S)    *S    = pState->TX.s;
    if (Ns)   *Ns   = pState->TX.Ns;
    if (Fs)   *Fs   = 80.0E+6 * pState->M;
    if (Prms) *Prms = 1.0;
  
    return WI_SUCCESS;
}
// end of NevadaPHY_TX()

// ================================================================================================
// FUNCTION  : NevadaPHY_TxIQLoopback
// ------------------------------------------------------------------------------------------------
// Purpose   : Models baseband side of the TX I/Q loopback for calibration (used with radio RX
//             ModelLevel 6).
// ================================================================================================
wiStatus NevadaPHY_TxIQLoopback(wiComplex *R[], Coltrane_SignalState_t *RFSigIn, NevadaPHY_State_t *pState)
{
    Nevada_TxState_t  *pBasebandTX = Nevada_TxState();

    int    t = Nevada_RxState()->k80;
    int    k = pState->TxIQLoopback.k;
    double A = pState->TxIQLoopback.A;

    wiWORD yR, yI;
    wiComplex z;

    if (RFSigIn->MC == 3)
    {
        if (k < Nevada_TxState()->Nz)
        {

            //////////////////////////////////////////////////////////////
            //
            // Tx I/Q Correction
            //     
            Nevada_TxIQ(1, &(pBasebandTX->qReal[k]), &(pBasebandTX->qImag[k]), &yR, &yI);

            //////////////////////////////////////////////////////////////
            //
            // DAC Processing
            //    
            pBasebandTX->uReal[k].word = yR.word + 512;
            pBasebandTX->uImag[k].word = yI.word + 512;

            Nevada_DAC(1, &(pBasebandTX->uReal[k]), &(pBasebandTX->uImag[k]), &z, 1);
            pBasebandTX->d[k] = z;

            //////////////////////////////////////////////////////////////
            //
            // Tx Mixer
            //    
            z.Real = A * z.Real;
            z.Imag = A * z.Imag;

            Coltrane_TX_Mixer(1, &z);

            pState->TX.s[0][k] = z;
        }
        else 
            z = R[0][t];

        if (R[0]) R[0][t] = z;
        if (R[1]) R[1][t] = z;

        (pState->TxIQLoopback.k)++;
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_TxIQLoopback()

// ================================================================================================
// FUNCTION  : NevadaPHY_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus NevadaPHY_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[])
{
    int i, m, NumRx, t = 0;
    int ClockPad;
    wiBoolean DW_PHYEnB = 1;
    wiBoolean UseR1 = R[1]? 1:0; // is R[1] array from second antenna valid?
    unsigned int k80; // Nevada 80 MHz clock index

    Coltrane_SignalState_t *RFSigIn, *RFSigOut;   // (type cast)RadioDigitalIO

    NevadaPHY_State_t *pState    = NevadaPHY_State();
    Nevada_RxState_t  *pNevadaRx = Nevada_RxState();
  
    wiBoolean TxIQLoopback = (Coltrane_RxState()->ModelLevel == 6) ? 1:0; // TX I/Q Calibration

    if (InvalidRange(Nr, 400, 1<<28)) return STATUS(WI_ERROR_PARAMETER3);
    if (!R   ) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[1] && (Nevada_Registers()->PathSel.w2==3)) return STATUS(WI_ERROR_PARAMETER4);

    if (!pState->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( NevadaPHY_InitializeThread(wiProcess_ThreadIndex()) )
    }

    if (TxIQLoopback && (pState->M != 1))
    {
        wiUtil_WriteLogFile("TxIQLoopBack is only valid for oversample rate M = 1\n");
        return STATUS(WI_ERROR);
    }

    if (pState->ExternalRxADC.Use) Nr = 2*pState->ExternalRxADC.Nx;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Resize Arrays
    //
    //  Arrays are resized only if the "b" baseband N44 could is zero or the value in
    //  "rLength" does not match the value required to process the current waveform. For
    //  typical PER simulations, the waveform on successive calls is the same, so this
    //  avoids unnecessary re-sizing but allows the waveform length to change between
    //  tests.
    //
    if (!Nevada_bRxState()->N44 || ((unsigned)Nr/pState->M != pNevadaRx->rLength))
    {
        pNevadaRx->rLength = Nr/pState->M;
        Nevada_bRxState()->N44    = Nr/pState->M * 44/80 + 2; // +2 for rounding and initial clock index=1
        
        XSTATUS( Nevada_RX_AllocateMemory() );
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Restart the PHY modules
    //
    //  Per-packet initialization for the PHY modules is performed at the start of each
    //  waveform processing. Note that this clears memory that would otherwise retain
    //  its value for packet-by-packet processing in the system. What is effectively
    //  modeled here is a startup from sleep at the start of waveform processing. To
    //  model packet-to-packet processing, multiple packets must be included in a single
    //  waveform, but only the decoded data for the last packet will be returned.
    //
    NumRx = (R[0]? 1:0) + (R[1]? 1:0);                         // number of receive antenna signals
    XSTATUS( Coltrane_RX_Restart(Nr, 80.0*pState->M, NumRx) ); // radio restart
    XSTATUS( Nevada_RX_Restart() );                            // baseband restart
    Nevada_RX_ClockGenerator(pNevadaRx, 1);                    // reset clock generator
    pState->inReg.bRXEnB = 1;                                  // reset state for bRXEnB
    pState->TxIQLoopback.k = 0;                                // reset TX I/Q loopback index
    pState->TxIQLoopback.A = 0.1/sqrt(2*Coltrane_TxState()->Prms);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Cast Radio Interface Structures
    //
    //  WiSE coding convention requires the baseband and radio models to be independent
    //  for implementation-specific data types. However, the radio signal state structure
    //  is the same for Nevada and Coltrane (a model requirement). The cast below is used
    //  to avoid unwanted compiler warnings.
    //
    RFSigIn  = (Coltrane_SignalState_t *)(void *)(&pState->RadioSigIn );
    RFSigOut = (Coltrane_SignalState_t *)(void *)(&pState->RadioSigOut);
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Reset RX/Top control state machine between packets
    //
    //  This is necessary because the OFDM simulation is not cycle-accurate to the 
    //  decoder output so the aRxDone signal cannot be generated.
    //
    pState->inReg.State = pState->outReg.State = 0;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clear the Radio and AFE signals/registers
    //
    //  Until the baseband runs a few clocks, these are undefined, so a zero value more
    //  closely matches the expected end state. In particular, it puts the radio into
    //  sleep mode so an MC = 3 from a previous packet does not cause additional random
    //  samples to be drawn from wiRnd.
    //
    pState->RadioSigOut.word = pState->RadioSigIn.word = 0;
    pState->DataConvOut.word = pState->DataConvIn.word = 0;
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Setup the input/receive sample arrays
    //
    //  For system simulations the array size is often constant from one packet to the 
    //  next. This block compares the current array size with the input length. If they 
    //  are the same the existing array is retained; otherwise it is resized.
    //
    if (!pState->RX.s[0] || (Nr != pState->RX.Ns))
    {
        pState->RX.Ns = Nr;
        WIFREE( pState->RX.s[0] ); pState->RX.s[0] = (wiComplex *)wiCalloc(Nr, sizeof(wiComplex));
        WIFREE( pState->RX.s[1] ); pState->RX.s[1] = (wiComplex *)wiCalloc(Nr, sizeof(wiComplex));
        if (!pState->RX.s[0] || !pState->RX.s[1]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clear the event/fault flags
    //
    pNevadaRx->EventFlag.word = 0;
    pNevadaRx->Fault = 0;
    pNevadaRx->x_ofs = -1;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  ClockPad.
    //  Certain values for the clock InitialPhase (other than 0) cause an array overrun.
    //  To avoid spending time to track down this fault, the last sample is not passed
    //  into the simulator. Empirically, this fixes the problem. External functions such
    //  as the Matlab NevadaPhyWaveform show this final "invalid" sample so the padding
    //  this clock. If the clock generator is initialized to a non-zero state, the last
    //
    ClockPad = pNevadaRx->clock.InitialPhase ? 1 : 0;
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  RECEIVER -----------------------------------------------------------------------
    //  --------------------------------------------------------------------------------
    //  Clocked operation with mixed clock/symbol/packet timing accuracy. The while()
    //  loop below iterates once per 80 MHz clock tick. All other timing is derived from
    //  this clock. If the clock generator is initialized to a non-zero state, the last
    //
    while (pNevadaRx->k40 < (pNevadaRx->Nr - ClockPad))
    {
        Nevada_RX_ClockGenerator(pNevadaRx, 0); // generate next clock edge(s)
        k80 = pNevadaRx->k80;

        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Startup/Shutdown Sequence
        //
        //  Transitions on DW_PHYEnB are modeled here to allow simulation of PHY behavior
        //  with this signal.
        //
        {
            wiBoolean T1 = (k80 >= pState->DW_PHYEnB.kEnable[0]) && (k80 < pState->DW_PHYEnB.kDisable[0]);
            wiBoolean T2 = (k80 >= pState->DW_PHYEnB.kEnable[1]) && (k80 < pState->DW_PHYEnB.kDisable[1]);
            wiBoolean T3 = (k80 >= pState->DW_PHYEnB.kEnable[2]) && (k80 < pState->DW_PHYEnB.kDisable[2]);
            wiBoolean T4 = (k80 >= pState->DW_PHYEnB.kEnable[3]) && (k80 < pState->DW_PHYEnB.kDisable[3]);

            DW_PHYEnB = (T1 || T2 || T3 || T4) ? 0 : 1;
        }
        /////////////////////////////////////////////////////////////////////////////////
        //
        //  Operate PHY at 80 MHz
        //
        if (pNevadaRx->clock.posedgeCLK80MHz)
        {
            wiBoolean AntSel = UseR1 && pState->RadioSigIn.SW2; // switch control for antenna selection

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Clock Registers at 80 MHz
            //  
            //  Only registers at the boundary of independent modules called from this
            //  function are modeled here. Registers for signals contained entirely
            //  within a module are modeled with that module.
            //
            pState->RadioSigIn  = pState->RadioSigOut;
            pState->DataConvOut = pState->DataConvIn;
            pState->outReg      = pState->inReg;

            /////////////////////////////////////////////////////////////////////////////
            //
            //  TX I/Q Loopback for Calibration Model
            //
            if (TxIQLoopback)
            {
                XSTATUS( NevadaPHY_TxIQLoopback(R, RFSigIn, pState) )
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Radio Receiver
            //
            //  The radio model is operated at a sample rate of M * (80 MHz), but the
            //  rest of the PHY model operates at 80 MHz so M samples are processed at
            //  once below.
            //
            for (m = 0; m < pState->M; m++)
            {
                t = pState->M * pNevadaRx->k80 + m;
                if (R[0]) { XSTATUS(Coltrane_RX(0, t, R[AntSel^0][t], &(pState->RX.s[0][t]), *RFSigIn, RFSigOut)); }
                if (R[1]) { XSTATUS(Coltrane_RX(1, t, R[AntSel^1][t], &(pState->RX.s[1][t]), *RFSigIn, RFSigOut)); }
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Analog-to-Digital Conversion (ADC)
            //
            //  The data converters operate at 40 MHz when the baseband clock model
            //  shows a rising edge on the 40 MHz clock. The model includes and option
            //  to bypass the internal model and load data to the ExternalRxADC array.
            //
            if (pNevadaRx->clock.posedgeCLK40MHz)
            {
                int k40 = pNevadaRx->k40;

                if (pState->ExternalRxADC.Use)
                {
                    if (k40 < pState->ExternalRxADC.Nx)
                    {
                        pNevadaRx->qReal[0][k40] = pState->ExternalRxADC.xI[k40];
                        pNevadaRx->qImag[0][k40] = pState->ExternalRxADC.xQ[k40];
                        pNevadaRx->qReal[1][k40] = pState->ExternalRxADC.xI[k40];
                        pNevadaRx->qImag[1][k40] = pState->ExternalRxADC.xQ[k40];
                    }
                    else
                    {
                        pNevadaRx->qReal[0][k40].word = 512;
                        pNevadaRx->qImag[0][k40].word = 512;
                        pNevadaRx->qReal[1][k40].word = 512;
                        pNevadaRx->qImag[1][k40].word = 512;
                    }
                }
                else if(pState->FixedOutputADC.Use)
                {
                    pNevadaRx->qReal[0][k40].word = pState->FixedOutputADC.AI;
                    pNevadaRx->qImag[0][k40].word = pState->FixedOutputADC.AQ;
                    pNevadaRx->qReal[1][k40].word = pState->FixedOutputADC.BI;
                    pNevadaRx->qImag[1][k40].word = pState->FixedOutputADC.BQ;
                }
                else
                {
                    Nevada_ADC( pState->DataConvIn, &pState->DataConvOut, pNevadaRx, 
                                pState->RX.s[0][t], pState->RX.s[1][t],
                                pNevadaRx->qReal[0] + k40, pNevadaRx->qImag[0] + k40,
                                pNevadaRx->qReal[1] + k40, pNevadaRx->qImag[1] + k40 );
                }
            }
            /////////////////////////////////////////////////////////////////////////////
            //
            //  Digital Baseband Receiver (Nevada)
            //
            XSTATUS( Nevada_RX( DW_PHYEnB, &pState->RadioSigOut, &pState->DataConvIn, 
                                pState->DataConvOut, &pState->inReg, pState->outReg ) );
                    
            // -------------
            // Trace Signals
            // -------------
            if (pNevadaRx->EnableTrace)
            {
                pNevadaRx->traceRadioIO [pNevadaRx->k80] = pState->RadioSigIn;
                pNevadaRx->traceDataConv[pNevadaRx->k80] = pState->DataConvOut;
            }

            /////////////////////////////////////////////////////////////////////////////
            //
            //  Write Register(s)
            //
            //  Write specified register address/data at a particular clock interval to
            //  model dynamic register access. Write in completes at k80 and data becomes
            //  valid on the next clock edge.
            //
            if (pState->WriteRegister.Use)
            {
                for (i = 0; i <= NEVADAPHY_MAX_WRITEREGISTER; i++)
                {
                    if ((pState->WriteRegister.k80[i] == k80) && (pState->WriteRegister.Addr[i]))
                    {
                        wiWORD Zero = {0};
                        Nevada_DCX(-2, Zero, NULL, 0, pNevadaRx, Nevada_Registers()); // Load values from DCX accumulators
                        Nevada_WriteRegister(pState->WriteRegister.Addr[i], pState->WriteRegister.Data[i]);
                    }
                }
            }
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  End of Waveform Processing
    //
    XSTATUS( Nevada_RX_EndOfWaveform() );
   
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Return the PSDU
    //
    //  The PHY data payload is returned in array data. Because the test routines do not
    //  process the design-specific PHY header used between the MAC and PHY, the first
    //  eight bytes are excluded from the returned data.
    //
    if (Length) *Length = pNevadaRx->N_PHY_RX_WR - 8;
    if (data)     *data = pNevadaRx->PHY_RX_D    + 8;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Frame Check Sequence (FCS)
    //
    //  Perform error detection using the 802.11 FCS. The 'C' model allows arbitrary
    //  PSDU data to be transmitted, so this check is not valid for all cases. Only
    //  valid 802.11 frames will return a valid result. The information produced here
    //  is not generally used within WiSE but simplifies error checking for external
    //  test suites.
    //
    if (pNevadaRx->N_PHY_RX_WR > 8)
    {
        pState->RxValidFCS = wiMAC_ValidFCS( pNevadaRx->N_PHY_RX_WR - 8, 
                                             pNevadaRx->PHY_RX_D    + 8 );
    }
    ////////////////////////////////////////////////////////////////////////////////
    //
    //  Compute the Receive and TX+RX Checksums
    //
    if (pState->Checksum.Enable)
    {
        int n;
        Nevada_RxState_t  *p = Nevada_RxState();
        Nevada_bRxState_t *b = Nevada_bRxState();

        pState->Checksum.RX = 0;

        for (n = 0; n < 2; n++)
        {
            if ((n==0) && !Nevada_Registers()->PathSel.b0) continue;
            if ((n==1) && !Nevada_Registers()->PathSel.b1) continue;

            wiMath_FletcherChecksum( p->N40*sizeof(wiWORD), p->cReal[n], &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N40*sizeof(wiWORD), p->cImag[n], &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N20*sizeof(wiWORD), p->xReal[n], &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N20*sizeof(wiWORD), p->xImag[n], &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->Nu *sizeof(wiWORD), p->uReal[n], &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->Nu *sizeof(wiWORD), p->uImag[n], &(pState->Checksum.RX) );
        }
        wiMath_FletcherChecksum( b->N44*sizeof(wiWORD), b->sReal, &(pState->Checksum.RX) );
        wiMath_FletcherChecksum( b->N44*sizeof(wiWORD), b->sImag, &(pState->Checksum.RX) );
        wiMath_FletcherChecksum( p->N_PHY_RX_WR*sizeof(wiUWORD), p->PHY_RX_D, &(pState->Checksum.RX) );

        if (p->EnableTrace)
        {
            wiMath_FletcherChecksum( p->N80*sizeof(Nevada_traceRxStateType),   p->traceRxState,   &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N80*sizeof(Nevada_traceRxControlType), p->traceRxControl, &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N20*sizeof(Nevada_traceImgDetType),    p->traceImgDet,    &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N20*sizeof(Nevada_aSigState_t),        p->aSigOut,        &(pState->Checksum.RX) );
            wiMath_FletcherChecksum( p->N22*sizeof(Nevada_bSigState_t),        p->bSigOut,        &(pState->Checksum.RX) );
        }
        pState->Checksum.Result = 0;
        wiMath_FletcherChecksum( sizeof(wiUWORD), &(pState->Checksum.RX), &(pState->Checksum.Result) );
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Diagnostic: Dump RX vectors and terminate
    //
    if (pState->dumpRX)
    {
        int n=0;

        wiPrintf("\n>>> Dumping RX arrays and terminating...\n");

        dump_wiComplex("t.out",Coltrane_RxState()->trace,Nr);
        dump_wiComplex("s.out",pState->RX.s[n],pState->RX.Ns);
        if (                 R[1]) dump_wiComplex("r1.out",R[1],Nr);
        if (pState->RX.s[1] && R[1]) dump_wiComplex("s1.out",pState->RX.s[1],pState->RX.Ns);
        dump_wiWORD("rReal.out",pNevadaRx->rReal[n],pNevadaRx->N40);
        dump_wiWORD("rImag.out",pNevadaRx->rImag[n],pNevadaRx->N40);
        dump_wiWORD("cReal.out",pNevadaRx->cReal[n],pNevadaRx->N40);
        dump_wiWORD("cImag.out",pNevadaRx->cImag[n],pNevadaRx->N40);
        dump_wiUWORD("PHY_RX_D.out",pNevadaRx->PHY_RX_D,pNevadaRx->N_PHY_RX_WR);

        if (pState->outReg.TurnOnCCK)
        {
            Nevada_bRxState_t *p = Nevada_bRxState();

            dump_wiWORD("xReal.out", p->xReal, p->N40);
            dump_wiWORD("yReal.out", p->yReal, p->N40);
            dump_wiWORD("sReal.out", p->sReal, p->N44);
            dump_wiWORD("zReal.out", p->zReal, p->N22);
            dump_wiWORD("zImag.out", p->zImag, p->N22);
            dump_wiWORD("vReal.out", p->vReal, p->N11);
            dump_wiWORD("vImag.out", p->vImag, p->N11);
            dump_wiWORD("bcReal.out",p->rReal, p->N22);
            dump_wiWORD("bcImag.out",p->rImag, p->N22);

            if (pNevadaRx->EnableTrace)
            {
                dump_wiWORD("EDOut.out",(wiWORD *)p->EDOut,p->N22);
                dump_wiWORD("CQOut.out",(wiWORD *)p->CQOut,p->N22);
            }
        }
        else   
        {
            dump_wiWORD("sReal.out",pNevadaRx->sReal[n],pNevadaRx->Ns);
            dump_wiWORD("wReal.out",pNevadaRx->wReal[n],pNevadaRx->Ns);
            dump_wiWORD("wImag.out",pNevadaRx->wImag[n],pNevadaRx->Ns);
            dump_wiWORD("vReal.out",pNevadaRx->vReal[n],pNevadaRx->Nv);
            dump_wiWORD("xReal.out",pNevadaRx->xReal[n],pNevadaRx->Nx);
        }
        if (pNevadaRx->EnableTrace)
        {
            dump_wiUWORD("traceRxState.out", (wiUWORD *)pNevadaRx->traceRxState, pNevadaRx->N80);
            dump_wiUWORD("traceDFE.out",     (wiUWORD *)pNevadaRx->traceDFE,     pNevadaRx->N20);
        }
        wiPrintf("    Nr = %d\n",pNevadaRx->Nr);
        wiPrintf("    AGAIN = %d\n",pState->RadioSigOut.AGAIN);
        wiUtil_WriteLogFile("Exit after dumpRX\n");
        exit(1);
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_RX()

// ================================================================================================
// FUNCTION : NevadaPHY_LoadRxADC_from_Array
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_LoadRxADC_from_Array(wiInt N, wiComplex *x)
{
    int i;
    NevadaPHY_State_t *pState = NevadaPHY_State();

    if (InvalidRange(N, 1024, 16e6)) return STATUS(WI_ERROR_PARAMETER1);

    WIFREE( pState->ExternalRxADC.xI );
    WIFREE( pState->ExternalRxADC.xQ );
    pState->ExternalRxADC.xI = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    pState->ExternalRxADC.xQ = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    if (!pState->ExternalRxADC.xI) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    if (!pState->ExternalRxADC.xQ) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for (i = 0; i < N; i++) 
    {
        pState->ExternalRxADC.xI[i].word = (unsigned)(x[i].Real);
        pState->ExternalRxADC.xQ[i].word = (unsigned)(x[i].Imag);
    }
    pState->ExternalRxADC.Nx = N;
    strcpy(pState->ExternalRxADC.Filename, "Array Load");
    pState->ExternalRxADC.Use = 1;

    return WI_SUCCESS;
}
// end of NevadaPHY_LoadRxADC_from_Array()

// ================================================================================================
// FUNCTION : NevadaPHY_LoadRxADC_from_File
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_LoadRxADC_from_File(char filename[])
{
    char buf[256], *cptr, *endptr;
    int i, N;

    NevadaPHY_State_t *pState = NevadaPHY_State();

    FILE *F = wiParse_OpenFile(filename,"rt");
    if (!F) return STATUS(WI_ERROR_FILE_OPEN);

    for (N = 0; !feof(F); N++) fgets(buf, 255, F); // determine file length
    rewind(F); N--;
    if ((N<1024) || (N>1e7)) return STATUS(WI_ERROR_FILE_FORMAT);

    WIFREE( pState->ExternalRxADC.xI );
    WIFREE( pState->ExternalRxADC.xQ );
    pState->ExternalRxADC.xI = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    pState->ExternalRxADC.xQ = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    if (!pState->ExternalRxADC.xI || !pState->ExternalRxADC.xQ) 
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for (i = 0; i < N; i++)
    {
        if (feof(F)) return STATUS(WI_ERROR_FILE_FORMAT);
        fgets(buf, 255, F); cptr=buf;
        while (*cptr==' ') cptr++;
        pState->ExternalRxADC.xI[i].word = strtoul(cptr, &endptr, 0); cptr = endptr;
        while (*cptr==' ') cptr++;
        pState->ExternalRxADC.xQ[i].word = strtoul(cptr, &endptr, 0);
    }
    fclose(F);

    pState->ExternalRxADC.Nx = N;
    strcpy(pState->ExternalRxADC.Filename, filename);
    pState->ExternalRxADC.Use = 1;

    return WI_SUCCESS;
}
// end of NevadaPHY_LoadRxADC_from_File()

// ================================================================================================
// FUNCTION : NevadaPHY_ClearRxADC
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_ClearRxADC(void)
{
    NevadaPHY_State_t *pState = NevadaPHY_State();

    if (pState->ExternalRxADC.Use)
    {
        pState->ExternalRxADC.Use = 0;
        WIFREE( pState->ExternalRxADC.xI );
        WIFREE( pState->ExternalRxADC.xQ );
    }
    return WI_SUCCESS;
}
// end of NevadaPHY_ClearRxADC()

// ================================================================================================
// FUNCTION : NevadaPHY_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_ParseLine(wiString text)
{
    char buf[256]; wiStatus status, pstatus;
    const unsigned int MaxPHYEnB = (unsigned)80e6;
    int i, addr, data, k;

    NevadaPHY_State_t *pState = NevadaPHY_State();

    // Allow both "NevadaPhy" and "NevadaPHY" for this module
    //
    if (text == strstr(text,"NevadaPhy")) {
        strncpy(text, "NevadaPHY", 9);
    }

    PSTATUS(wiParse_Int    (text,"NevadaPHY.M",              &(pState->M), 1, 10000));
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.dumpTX",         &(pState->dumpTX)) );
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.dumpRX",         &(pState->dumpRX)) );
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.DumpTX",         &(pState->dumpTX)) );
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.DumpRX",         &(pState->dumpRX)) );
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.ShortPreamble",  &(pState->ShortPreamble)));
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.shortPreamble",  &(pState->ShortPreamble))); // legacy support, deprecated
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.TxGreenfield",   &(pState->TxGreenfield)));  
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.TxAggregate",    &(pState->TxAggregate)));  
    PSTATUS(wiParse_Boolean(text,"NevadaPHY.TxOpProtect",    &(pState->TxOpProtect)));  
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.TXPWRLVL",       &(pState->TXPWRLVL.word), 0, 255));

    PSTATUS(wiParse_UInt   (text,"NevadaPHY.DW_PHYEnB.kEnable",  pState->DW_PHYEnB.kEnable,  0, MaxPHYEnB));
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.DW_PHYEnB.kDisable", pState->DW_PHYEnB.kDisable, 0, MaxPHYEnB));

    PSTATUS(wiParse_UIntArray(text, "NevadaPHY.DW_PHYEnB.kEnable",  pState->DW_PHYEnB.kEnable,  3, 0, MaxPHYEnB));
    PSTATUS(wiParse_UIntArray(text, "NevadaPHY.DW_PHYEnB.kDisable", pState->DW_PHYEnB.kDisable, 3, 0, MaxPHYEnB));

    PSTATUS( wiParse_Boolean  (text,"NevadaPHY.PhyTxHeader.ForceHeader", &(pState->PhyTxHeader.ForceHeader)) );
    PSTATUS( wiParse_UIntArray(text,"NevadaPHY.PhyTxHeader.Byte", &(pState->PhyTxHeader.Byte[0]), 7, 0, 255) );

    PSTATUS( wiParse_String (text,"NevadaPHY.AV.Checker",     pState->AV.Checker, NEVADAPHY_AV_NAME_LENGTH) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.Limit",     &(pState->AV.Limit),     -1e15, 1e15) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.LimitLow",  &(pState->AV.LimitLow),  -1e15, 1e15) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.LimitHigh", &(pState->AV.LimitHigh), -1e15, 1e15) );
    PSTATUS( wiParse_Int    (text,"NevadaPHY.AV.N",         &(pState->AV.N),         INT_MIN, INT_MAX) );  
    PSTATUS( wiParse_String (text,"NevadaPHY.AV.Parameter",   pState->AV.Parameter, NEVADAPHY_AV_NAME_LENGTH) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.ParamLow",  &(pState->AV.ParamLow),  -1e15, 1e15) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.ParamHigh", &(pState->AV.ParamHigh), -1e15, 1e15) );
    PSTATUS( wiParse_Real   (text,"NevadaPHY.AV.ParamStep", &(pState->AV.ParamStep), -1e15, 1e15) );
    PSTATUS( wiParse_UIntArray(text,"NevadaPHY.AV.UIntValue", pState->AV.UIntValue, 8, 0, 0xFFFFFFFF) );

    PSTATUS(wiParse_Boolean(text,"NevadaPHY.FixedOutputADC.Use", &(pState->FixedOutputADC.Use)) );
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.FixedOutputADC.AI",  &(pState->FixedOutputADC.AI), 0, 1023) );
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.FixedOutputADC.AQ",  &(pState->FixedOutputADC.AQ), 0, 1023) );
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.FixedOutputADC.BI",  &(pState->FixedOutputADC.BI), 0, 1023) );
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.FixedOutputADC.BQ",  &(pState->FixedOutputADC.BQ), 0, 1023) );

    PSTATUS(wiParse_Boolean  (text,"NevadaPHY.WriteRegister.Use", &(pState->WriteRegister.Use)) );
    PSTATUS(wiParse_UIntArray(text,"NevadaPHY.WriteRegister.k80",   pState->WriteRegister.k80,  NEVADAPHY_MAX_WRITEREGISTER, 0, 80000000));
    PSTATUS(wiParse_UIntArray(text,"NevadaPHY.WriteRegister.Addr",  pState->WriteRegister.Addr, NEVADAPHY_MAX_WRITEREGISTER, 0, 1023));
    PSTATUS(wiParse_UIntArray(text,"NevadaPHY.WriteRegister.Data",  pState->WriteRegister.Data, NEVADAPHY_MAX_WRITEREGISTER, 0,  255));

    PSTATUS(wiParse_Boolean  (text,"NevadaPHY.Checksum.Enable", &(pState->Checksum.Enable)) );

    if ((status = STATUS(wiParse_Function(text,"NevadaPHY_WriteRegister(%d, %d, %d, %d)",&i,&addr,&data,&k))) == WI_SUCCESS)
    {
        if (InvalidRange(i,    0, NEVADAPHY_MAX_WRITEREGISTER)) return STATUS(WI_ERROR_PARAMETER1);
        if (InvalidRange(addr, 0, 1023))                        return STATUS(WI_ERROR_PARAMETER2);
        if (InvalidRange(data, 0,  255))                        return STATUS(WI_ERROR_PARAMETER3);
        if (InvalidRange(k,    0, 80000000))                    return STATUS(WI_ERROR_PARAMETER4);
        pState->WriteRegister.Use = 1;
        pState->WriteRegister.Addr[i] = addr;
        pState->WriteRegister.Data[i] = data;
        pState->WriteRegister.k80[i]  = k;
        return WI_SUCCESS;
    }

    // legacy support code (deprecated)
    //
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.T_PHYEnB",       &(pState->DW_PHYEnB.kEnable[0] ), 0, MaxPHYEnB));
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.T_PHYEnB[0]",    &(pState->DW_PHYEnB.kEnable[0] ), 0, MaxPHYEnB));
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.T_PHYEnB[1]",    &(pState->DW_PHYEnB.kDisable[0]), 0, MaxPHYEnB));
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.T_PHYEnB[2]",    &(pState->DW_PHYEnB.kEnable[1] ), 0, MaxPHYEnB));
    PSTATUS(wiParse_UInt   (text,"NevadaPHY.T_PHYEnB[3]",    &(pState->DW_PHYEnB.kDisable[1]), 0, MaxPHYEnB));

    if ((status = STATUS(wiParse_Function(text,"NevadaPHY_LoadRxADC(%255s)",buf))) == WI_SUCCESS)
    {
        XSTATUS( NevadaPHY_LoadRxADC_from_File(buf) );
        return WI_SUCCESS;
    }
    else if (status < WI_SUCCESS) return WI_ERROR_RETURNED;

    if ((status = STATUS(wiParse_Command(text,"NevadaPHY_ClearRxADC()"))) == WI_SUCCESS)
    {
        XSTATUS(NevadaPHY_ClearRxADC());
        return WI_SUCCESS;
    }
    else if (status < WI_SUCCESS) return WI_ERROR_RETURNED;

    return WI_WARN_PARSE_FAILED;
}
// end of NevadaPHY_ParseLine()

// ================================================================================================
// FUNCTION : NevadaPHY_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus NevadaPHY_WriteConfig(wiMessageFn MsgFunc)
{
    char t[4][32]; int i;
    NevadaPHY_State_t *pState = NevadaPHY_State();

    sprintf(t[0], "%s", wiUtil_Comma(pState->DW_PHYEnB.kEnable[0]));
    sprintf(t[1], "%s", wiUtil_Comma(pState->DW_PHYEnB.kDisable[0]));
    sprintf(t[2], "%s", wiUtil_Comma(pState->DW_PHYEnB.kEnable[1]));
    sprintf(t[3], "%s", wiUtil_Comma(pState->DW_PHYEnB.kDisable[1]));

    if (pState->dumpTX) MsgFunc(" dumpTX ENABLED\n");
    if (pState->dumpRX) MsgFunc(" dumpRX ENABLED\n");

    MsgFunc(" Channel Sample Rate   = %d x 80 MS/s\n",pState->M);
    MsgFunc(" 802.11b Preamble Mode = %s\n",pState->ShortPreamble? "Short":"Long");
    MsgFunc(" 802.11n Preamble Mode = %s\n",pState->TxGreenfield? "Greenfield":"Mixed Mode" );
    MsgFunc(" PHY Header            : TXPWRLVL = 0x%02X, Aggregation = %d, TxOpProtect = %d\n",
        pState->TXPWRLVL,pState->TxAggregate,pState->TxOpProtect);

    MsgFunc(" DW_PHYEnB Assertion   =");
    for (i = 0; i < 4; i++)
    {
        char tEnable[32], tDisable[32];
        if (pState->DW_PHYEnB.kEnable[i] < pState->DW_PHYEnB.kDisable[i])
        {
            sprintf(tEnable,  "%s", wiUtil_Comma(pState->DW_PHYEnB.kEnable[i]));
            sprintf(tDisable, "%s", wiUtil_Comma(pState->DW_PHYEnB.kDisable[i]));
            MsgFunc(" [%s-%s)T",tEnable,tDisable);
        }
    }
    MsgFunc("\n");

    if (pState->ExternalRxADC.Use) {
        char s[512];
        sprintf( s, " >>>>>>  Externl RxADC = %s  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",
            pState->ExternalRxADC.Filename );
        s[78]='\n'; s[79]='\0';
        MsgFunc(s);
    }
    else if (pState->FixedOutputADC.Use) {
        MsgFunc(" >>>>>> Fixed Output ADC: AI=%u, AQ=%u, BI=%u, BQ=%u\n",pState->FixedOutputADC.AI, 
            pState->FixedOutputADC.AQ, pState->FixedOutputADC.BI, pState->FixedOutputADC.BQ);
    }
    else
    {
        XSTATUS( Coltrane_WriteConfig(MsgFunc) );
    }
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Modem                 = Nevada Baseband\n");
    XSTATUS( Nevada_WriteConfig(MsgFunc) );
    return WI_SUCCESS;
}
// end of NevadaPHY_WriteConfig()

// ================================================================================================
// FUNCTION : NevadaPHY_State()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
NevadaPHY_State_t * NevadaPHY_State(void)
{
    return State + wiProcess_ThreadIndex();
}
// end of NevadaPHY_StatePointer()

// ================================================================================================
// FUNCTION : NevadaPHY()
// ------------------------------------------------------------------------------------------------
// Purpose  : This is the top-level interface function accessed by wiPHY; it does not
//            contain code used to define Nevada hardware.
// ================================================================================================
wiStatus NevadaPHY(int command, void *pData)
{
    switch (command & 0xFFFF0000)
    {
        case WIPHY_WRITE_CONFIG:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pData);
            XSTATUS(NevadaPHY_WriteConfig(MsgFunc));
            break;
        }
        case WIPHY_ADD_METHOD:    
        {
            NevadaPHY_State_t *pState = NevadaPHY_State();

            wiPhyMethod_t *method = (wiPhyMethod_t *)pData;
            method->TxFn = NevadaPHY_TX;            
            method->RxFn = NevadaPHY_RX;        
            XSTATUS(wiParse_AddMethod(NevadaPHY_ParseLine));
            XSTATUS(wiParse_AddMethod(Nevada_ParseLine));

            pState->MAC_TX_D = (wiUWORD *)wiCalloc(65536+8, sizeof(wiUWORD));
            if (!pState->MAC_TX_D) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

            XSTATUS( Coltrane_Init() );
            XSTATUS( Nevada_Init() );

            pState->M = 1;
            pState->DW_PHYEnB.kEnable[0]  = pState->T_PHYEnB[0] = 20;
            pState->DW_PHYEnB.kDisable[0] = pState->T_PHYEnB[1] = (unsigned)80e6;
            pState->ThreadIsInitialized = WI_TRUE;
            break;
        }
        case WIPHY_REMOVE_METHOD:
        {
            int i;

            XSTATUS(wiParse_RemoveMethod(NevadaPHY_ParseLine));
            XSTATUS(wiParse_RemoveMethod(Nevada_ParseLine));
            XSTATUS(Coltrane_Close());
            XSTATUS(Nevada_Close());
            XSTATUS(NevadaPHY_ClearRxADC());

            for (i = 0; i < WISE_MAX_THREADS; i++)
            {
                WIFREE( State[i].MAC_TX_D );
                WIFREE( State[i].TX.s[0]  );
                WIFREE( State[i].RX.s[0]  );
                WIFREE( State[i].RX.s[1]  );
            }
            break;
        }
        case WIPHY_SET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;
        case WIPHY_GET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;

        case WIPHY_WRITE_REGISTER:
        {
            wiPhyRegister_t *pReg = (wiPhyRegister_t *)pData;
            XSTATUS( Nevada_WriteRegister(pReg->Addr, pReg->Data) );
            break;
        }
        case WIPHY_READ_REGISTER:
        {
            wiPhyRegister_t *pReg = (wiPhyRegister_t *)pData;
            pReg->Data = Nevada_ReadRegister(pReg->Addr);
            break;
        }
        case WIPHY_CLOSE_ALL_THREADS:
        {
            unsigned int n;
            for (n = 1; n < WISE_MAX_THREADS; n++) {
                XSTATUS( NevadaPHY_CloseThread(n) )
            }
            break;
        }
        default: return WI_WARN_METHOD_UNHANDLED_COMMAND;
    }
    return WI_SUCCESS;
}
// end of NevadaPHY()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

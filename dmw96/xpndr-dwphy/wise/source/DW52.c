//--< DW52.c >-------------------------------------------------------------------------------------
//=================================================================================================
//
//  Model for the DW52 PHY (Mojave + RF52)
//           
//  Developed by Barrett Brickner
//  Copyright 2005-2006 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wiPHY.h"
#include "wiUtil.h"
#include "wiParse.h"
#include "DW52.h"

#include "wiChanl.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean  Verbose = 0;
static wiStatus   _status = 0;  // return code
static DW52_State State   ={0};

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((_status=WICHK(arg))<=0) return _status;

// ================================================================================================
// FUNCTION  : DW52_TX()
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
wiStatus DW52_TX(double bps, int Length, wiUWORD *data, int *Ns, wiComplex ***S, double *Fs, double *Prms)
{
    int i, Nx;
    wiUWORD RATE, LENGTH;
    wiBoolean TXbMode;
    wiComplex *x;

    if (Verbose) wiPrintf(" DW52_TX_PHY(%d,@x%08X,...)\n",Length,data);

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!data) return STATUS(WI_ERROR_PARAMETER3);
    if (!Ns)   return STATUS(WI_ERROR_PARAMETER4);
    if (!S)    return STATUS(WI_ERROR_PARAMETER5);
    
    if (bps)
    {
        // -------------------------------------------
        // Generate the PHY control header (MAC Level)
        // -------------------------------------------
        if (InvalidRange(Length,1,4095)) return STATUS(WI_ERROR_PARAMETER2);
        LENGTH.w12 = (unsigned int)Length;
        switch ((unsigned)bps)
        {
            case  1000000:   
                if (State.shortPreamble) {RATE.word = 0x0; TXbMode=1; }
                else                    {RATE.word = 0x0; TXbMode=1; }    
                break;
            case  2000000:   
                if (State.shortPreamble) {RATE.word = 0x5; TXbMode=1; }
                else                    {RATE.word = 0x1; TXbMode=1; } 
                break;
            case  5500000:   
                if (State.shortPreamble) {RATE.word = 0x6; TXbMode=1; }
                else                    {RATE.word = 0x2; TXbMode=1; } 
                break;
            case 11000000:   
                if (State.shortPreamble) {RATE.word = 0x7; TXbMode=1; }
                else                    {RATE.word = 0x3; TXbMode=1; } 
                break;

            case  6000000: RATE.word = 0xD; TXbMode=0; break;
            case  9000000: RATE.word = 0xF; TXbMode=0; break;
            case 12000000: RATE.word = 0x5; TXbMode=0; break;
            case 18000000: RATE.word = 0x7; TXbMode=0; break;
            case 24000000: RATE.word = 0x9; TXbMode=0; break;
            case 36000000: RATE.word = 0xB; TXbMode=0; break;
            case 48000000: RATE.word = 0x1; TXbMode=0; break;
            case 54000000: RATE.word = 0x3; TXbMode=0; break;
            case 72000000: RATE.word = 0xA; TXbMode=0; break;
            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
        State.MAC_TX_D[0].word = State.TXPWRLVL.w8;              // TXPWRLVL[7:0]
        State.MAC_TX_D[1].word = TXbMode? 128 : 0;               // OFDM(=0) or DSSS/CCK(MSB=1)
        State.MAC_TX_D[2].word = (RATE.w4<<4) | (LENGTH.w12>>8); // RATE[3:0]|LENGTH[11:8]
        State.MAC_TX_D[3].word = LENGTH.w8;                      // LENGTH[7:0]
        State.MAC_TX_D[4].word = 0;
        State.MAC_TX_D[5].word = 0;
        State.MAC_TX_D[6].word = 0;
        State.MAC_TX_D[7].word = 0;

        for (i=0; i<Length; i++)
            State.MAC_TX_D[i+8].word = data[i].w8;                // PSDU
    }
    else
    {
        // ------------------------------------
        // Extract RATE from PHY control header
        // ------------------------------------
        if (InvalidRange(Length-8,1,4095)) return STATUS(WI_ERROR_PARAMETER2);
        Length = (data[2].w4 << 8) | data[3].w8; // number of data bytes
        TXbMode = data[1].b7;                    // modulation type: 0=OFDM, 1=DSSS/CCK
        for (i=0; i<(Length+8); i++)
            State.MAC_TX_D[i].word = data[i].w8;  // PSDU
    }
    XSTATUS( Mojave_TX_Restart() );
    XSTATUS( Mojave_TX(Length+8, State.MAC_TX_D, &Nx, &x, State.M) );

    // ---------------------------------
    // Allocate Memory for the TX signal
    // ---------------------------------
    if (State.TX.s[0]) wiFree(State.TX.s[0]);
    State.TX.s[0] = (wiComplex *)wiCalloc(Nx, sizeof(wiComplex));
    State.TX.s[1] = NULL; // terminator
    if (!State.TX.s[0]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    State.TX.Ns = Nx;

    // --------
    // TX Radio
    // --------
    XSTATUS( RF52_TX_Restart(Nx, 80.0*State.M, Mojave_Prms()) );
    XSTATUS( RF52_TX(Nx, x, State.TX.s[0], Nx-State.Mojave.pTX->OfsEOP) );

    // -----------------------------------------
    // Diagnostic: Dump TX vectors and terminate
    // -----------------------------------------
    if (State.dumpTX)
    {
        dump_wiUWORD  ("MAC_TX_D.out",State.MAC_TX_D,Length+8);
        dump_wiComplex("x.out",x, Nx);
        dump_wiComplex("s.out",State.TX.s[0],State.TX.Ns);
        exit(1);
    }
    // -----------------
    // Output parameters
    // -----------------
    if (S)    *S    = State.TX.s;
    if (Ns)   *Ns   = State.TX.Ns;
    if (Fs)   *Fs   = 80.0E+6 * State.M;
    if (Prms) *Prms = 1.0;

    return WI_SUCCESS;
}
// end of DW52_TX()

// ================================================================================================
// FUNCTION  : DW52_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus DW52_RX(int *Length, wiUWORD **data, int Nr, wiComplex *R[])
{
    static Mojave_CtrlState          inReg,      outReg;      // top-level control signals
    static Mojave_DataConvCtrlState  DataConvIn, DataConvOut; // data converter control
    static Mojave_RadioDigitalIO     RadioSigIn, RadioSigOut; // radio/RF signals
    RF52_SignalState_t              *RFSigIn,   *RFSigOut;    // (type cast)RadioDigitalIO

    int m, t, N_Rx;
    wiBoolean DW_PHYEnB = 1;
    wiBoolean UseR1 = R[1]? 1:0; // is R[1] array from second antenna valid?

    if (Verbose) wiPrintf(" DW52_RX(@x%08X,@x%08X,%d,@x%08X)\n",Length,data,Nr,R);
    if (InvalidRange(Nr, 400, 1<<28)) return STATUS(WI_ERROR_PARAMETER3);
    if (!R   ) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[1] && (State.Mojave.pReg->PathSel.w2==3)) return STATUS(WI_ERROR_PARAMETER4);

    if (State.ExternalRxADC.Use) Nr = 2*State.ExternalRxADC.Nx;

    // ----------------------------------------------
    // Resize Arrays for excess receive signal length
    // ----------------------------------------------
    if (!State.Mojave.pbRX->N44 || ((unsigned)Nr/State.M != State.Mojave.pRX->rLength))
    {
        State.Mojave.pRX->rLength = Nr/State.M;
        State.Mojave.pbRX->N44    = Nr/State.M * 44/80 + 2; // +2 for rounding and initial clock index=1
        XSTATUS(  Mojave_RX_AllocateMemory() );
        XSTATUS( bMojave_RX_AllocateMemory() );
    }
    // ------------------------------------------
    // Restart the PHY modules for the new packet
    // ------------------------------------------
    N_Rx = (R[0]? 1:0) + (R[1]? 1:0);                   // number of receive antenna signals
    XSTATUS( RF52_RX_Restart(Nr, 80.0*State.M, N_Rx) ); // radio restart
    XSTATUS( Mojave_RX_Restart(State.Mojave.pbRX) );    // baseband restart
    Mojave_RX_ClockGenerator(1);                        // reset clock generator
    inReg.bRXEnB = 1;                                   // reset state for bRXEnB

    // ----------------------------------------------------------
    // Create pointer cast to radio interface structure
    // Elements must align between the Mojave and RF52 structures
    // ----------------------------------------------------------
    RFSigIn  = (RF52_SignalState_t *)(&RadioSigIn );
    RFSigOut = (RF52_SignalState_t *)(&RadioSigOut);
    
    // Reset RX/Top control state machine between packets
    // This is necessary because the OFDM simulation is not cycle-accurate to the decoder output
    // so the aRxDone signal cannot be generated.
    //
    {
        inReg.State = outReg.State = 0;
    }

    // -------------------------------------------
    // Setup the input/receive samples
    // Reallocate memory if the length has changed
    // -------------------------------------------
    if (Nr != State.RX.Ns)
    {
        State.RX.Ns = Nr;
        if (State.RX.s[0]) wiFree(State.RX.s[0]); State.RX.s[0] = (wiComplex *)wiCalloc(Nr, sizeof(wiComplex));
        if (State.RX.s[1]) wiFree(State.RX.s[1]); State.RX.s[1] = (wiComplex *)wiCalloc(Nr, sizeof(wiComplex));
        if (!State.RX.s[0] || !State.RX.s[1]) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    }
    // ---------------------------
    // Clear the event/fault flags
    // ---------------------------
    State.Mojave.pRX->EventFlag.word = 0;
    State.Mojave.pRX->Fault = 0;
    State.Mojave.pRX->x_ofs = -1;

    // ----------------------------------------------------------------
    // RECEIVER -------------------------------------------------------
    // Clocked Operation With Mixed Clock/Symbol/Packet Timing Accuracy
    // ...the base clock is 80 MHz; all other timing is derived
    // ----------------------------------------------------------------
    while (State.Mojave.pRX->k40<State.Mojave.pRX->Nr)
    {
        Mojave_RX_ClockGenerator(0); // generate next clock edge(s)

        // --------------------------------------------
        // Startup/Shutdown Sequencing (Outside of PHY)
        // --------------------------------------------
        {
            wiBoolean T1 = (State.Mojave.pRX->k80 >= State.T_PHYEnB[0]) && (State.Mojave.pRX->k80 < State.T_PHYEnB[1]);
            wiBoolean T2 = (State.Mojave.pRX->k80 >= State.T_PHYEnB[2]) && (State.Mojave.pRX->k80 < State.T_PHYEnB[3]);
            DW_PHYEnB = (T1 || T2)? 0:1; 
        }
        // -----------------------------
        // PHY RX at Base Rate of 80 MHz
        // -----------------------------
        if (State.Mojave.pRX->clock.posedgeCLK80MHz)
        {
            wiBoolean AntSel = UseR1 && RadioSigIn.SW2; // switch control for antenna selection

            // -------------------------
            // Clock Registers at 80 MHz
            // -------------------------
            RadioSigIn  = RadioSigOut;
            DataConvOut = DataConvIn;
            outReg      = inReg;

            // ---------------------------------------
            // Call Radio RX Mode at Rate of M*(80MHz)
            // ---------------------------------------
            for (m=0; m<State.M; m++)
            {
                t = State.M*State.Mojave.pRX->k80 + m;
                if (R[0]) { XSTATUS(RF52_RX(0,t, R[AntSel^0][t], &(State.RX.s[0][t]), *RFSigIn, RFSigOut)); }
                if (R[1]) { XSTATUS(RF52_RX(1,t, R[AntSel^1][t], &(State.RX.s[1][t]), *RFSigIn, RFSigOut)); }
            }
            // -----------------------------------------
            // Data Conversion - Analog-to-Digital (ADC)
            // -----------------------------------------
            if (State.Mojave.pRX->clock.posedgeCLK40MHz)
            {
                int k40 = State.Mojave.pRX->k40;
                if (State.ExternalRxADC.Use)
                {
                    if (k40<State.ExternalRxADC.Nx)
                    {
                        State.Mojave.pRX->qReal[0][k40] = State.ExternalRxADC.xI[k40];
                        State.Mojave.pRX->qImag[0][k40] = State.ExternalRxADC.xQ[k40];
                        State.Mojave.pRX->qReal[1][k40] = State.ExternalRxADC.xI[k40];
                        State.Mojave.pRX->qImag[1][k40] = State.ExternalRxADC.xQ[k40];
                    }
                    else
                    {
                        State.Mojave.pRX->qReal[0][k40].word = 512;
                        State.Mojave.pRX->qImag[0][k40].word = 512;
                        State.Mojave.pRX->qReal[1][k40].word = 512;
                        State.Mojave.pRX->qImag[1][k40].word = 512;
                    }
                }
                else
                {
                    Mojave_ADC(DataConvIn, &DataConvOut, State.RX.s[0][t], State.RX.s[1][t],
                             State.Mojave.pRX->qReal[0]+k40, State.Mojave.pRX->qImag[0]+k40,
                             State.Mojave.pRX->qReal[1]+k40, State.Mojave.pRX->qImag[1]+k40);
                }
            }
            // ----------------------------------
            // Digital Baseband Receiver (Mojave)
            // ----------------------------------
            XSTATUS( Mojave_RX(DW_PHYEnB, &RadioSigOut, &DataConvIn, DataConvOut, &inReg, outReg) );

            // -------------
            // Trace Signals
            // -------------
            if (State.Mojave.pRX->EnableTrace)
            {
                State.Mojave.pRX->traceRadioIO [State.Mojave.pRX->k80] = RadioSigIn;
                State.Mojave.pRX->traceDataConv[State.Mojave.pRX->k80] = DataConvOut;
            }
        }
    }
    // -----------------------------------------------
    // Return PSDU and Length (drop 8 byte PHY header)
    // -----------------------------------------------
    if (Length) *Length = State.Mojave.pRX->N_PHY_RX_WR - 8;
    if (data)     *data = State.Mojave.pRX->PHY_RX_D    + 8;

    // -----------------------------------------
    // Diagnostic: Dump RX vectors and terminate
    // -----------------------------------------
    if (State.dumpRX)
    {
        int n=0;
        static int count = 0;
        count++;

        dump_wiComplex("t.out",State.RF52.pRX->trace,Nr);
        dump_wiComplex("r.out",R[0],Nr);
        dump_wiComplex("s.out",State.RX.s[n],State.RX.Ns);
        if (                 R[1]) dump_wiComplex("r1.out",R[1],Nr);
        if (State.RX.s[1] && R[1]) dump_wiComplex("s1.out",State.RX.s[1],State.RX.Ns);
        dump_wiWORD("rReal.out",State.Mojave.pRX->rReal[n],State.Mojave.pRX->N40);
        dump_wiWORD("rImag.out",State.Mojave.pRX->rImag[n],State.Mojave.pRX->N40);
        dump_wiWORD("cReal.out",State.Mojave.pRX->cReal[n],State.Mojave.pRX->N40);
        dump_wiWORD("cImag.out",State.Mojave.pRX->cImag[n],State.Mojave.pRX->N40);
        dump_wiUWORD("PHY_RX_D.out",State.Mojave.pRX->PHY_RX_D,State.Mojave.pRX->N_PHY_RX_WR);

        if (outReg.TurnOnCCK)
        {
            dump_wiWORD("xReal.out",State.Mojave.pbRX->xReal,  State.Mojave.pbRX->N40);
            dump_wiWORD("yReal.out",State.Mojave.pbRX->yReal,  State.Mojave.pbRX->N40);
            dump_wiWORD("sReal.out",State.Mojave.pbRX->sReal,  State.Mojave.pbRX->N44);
            dump_wiWORD("zReal.out",State.Mojave.pbRX->zReal,  State.Mojave.pbRX->N22);
            dump_wiWORD("zImag.out",State.Mojave.pbRX->zImag,  State.Mojave.pbRX->N22);
            dump_wiWORD("vReal.out",State.Mojave.pbRX->vReal,  State.Mojave.pbRX->N11);
            dump_wiWORD("vImag.out",State.Mojave.pbRX->vImag,  State.Mojave.pbRX->N11);
            dump_wiWORD("bcReal.out",State.Mojave.pbRX->rReal,  State.Mojave.pbRX->N22);
            dump_wiWORD("bcImag.out",State.Mojave.pbRX->rImag,  State.Mojave.pbRX->N22);

            if (State.Mojave.pbRX->EnableTraceCCK)
            {
                dump_wiWORD("EDOut.out",(wiWORD *)State.Mojave.pbRX->EDOut,State.Mojave.pbRX->N22);
                dump_wiWORD("CQOut.out",(wiWORD *)State.Mojave.pbRX->CQOut,State.Mojave.pbRX->N22);
            }
        }
        else   
        {
            dump_wiWORD("sReal.out",State.Mojave.pRX->sReal[n],State.Mojave.pRX->Ns);
            dump_wiWORD("wReal.out",State.Mojave.pRX->wReal[n],State.Mojave.pRX->Ns);
            dump_wiWORD("wImag.out",State.Mojave.pRX->wImag[n],State.Mojave.pRX->Ns);
            dump_wiWORD("vReal.out",State.Mojave.pRX->vReal[n],State.Mojave.pRX->Nv);
            dump_wiWORD("xReal.out",State.Mojave.pRX->xReal[n],State.Mojave.pRX->Nx);
            dump_wiWORD("yReal.out",State.Mojave.pRX->yReal[n],State.Mojave.pRX->Nx);
        }
        if (State.Mojave.pRX->EnableTrace)
        {
            dump_wiUWORD("traceRxState.out", (wiUWORD *)State.Mojave.pRX->traceRxState, State.Mojave.pRX->N80);
            dump_wiUWORD("traceDFE.out",     (wiUWORD *)State.Mojave.pRX->traceDFE,     State.Mojave.pRX->N20);
        }
        wiPrintf("Nr = %d\n",State.Mojave.pRX->Nr);
        wiPrintf("AGAIN = %d\n",RadioSigOut.AGAIN);
        if (count==1) exit(1);
    }
    if (State.Mojave.pRX->TraceKeyOp) {
        wiPrintf("RSSI=%3d, LNAGain=(%d,%d), AGain=%2d\n",outReg.TurnOnCCK? State.Mojave.pbRX->PHY_RX_D->w8-100:State.Mojave.pRX->PHY_RX_D->w8-100, RadioSigOut.LNAGAIN2,RadioSigOut.LNAGAIN,RadioSigOut.AGAIN);
    }
    return WI_SUCCESS;
}
// end of DW52_RX()

// ================================================================================================
// FUNCTION : DW52_LoadRxADC_from_Array
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_LoadRxADC_from_Array(wiInt N, wiComplex *x)
{
    int i;
    if (InvalidRange(N, 1024, 16e6)) return STATUS(WI_ERROR_PARAMETER1);

    if (State.ExternalRxADC.xI) wiFree(State.ExternalRxADC.xI);
    if (State.ExternalRxADC.xQ) wiFree(State.ExternalRxADC.xQ);
    State.ExternalRxADC.xI = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    State.ExternalRxADC.xQ = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    if (!State.ExternalRxADC.xI) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    if (!State.ExternalRxADC.xQ) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for (i=0; i<N; i++) {
        State.ExternalRxADC.xI[i].word = (unsigned)(x[i].Real);
        State.ExternalRxADC.xQ[i].word = (unsigned)(x[i].Imag);
    }
    State.ExternalRxADC.Nx = N;
    strcpy(State.ExternalRxADC.Filename, "Array Load");
    State.ExternalRxADC.Use = 1;

    return WI_SUCCESS;
}
// end of DW52_LoadRxADC_from_Array()

// ================================================================================================
// FUNCTION : DW52_LoadRxADC_from_File
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_LoadRxADC_from_File(char filename[])
{
    char buf[256], *cptr, *endptr;
    int i, N;

    FILE *F = wiParse_OpenFile(filename,"rt");
    if (!F) return STATUS(WI_ERROR_FILE_OPEN);

    for (N=0; !feof(F); N++) fgets(buf, 255, F); // determine file length
    rewind(F); N--;
    if ((N<1024) || (N>1e7)) return STATUS(WI_ERROR_FILE_FORMAT);

    if (State.ExternalRxADC.xI) wiFree(State.ExternalRxADC.xI);
    if (State.ExternalRxADC.xQ) wiFree(State.ExternalRxADC.xQ);
    State.ExternalRxADC.xI = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    State.ExternalRxADC.xQ = (wiUWORD *)wiCalloc(N, sizeof(wiUWORD));
    if (!State.ExternalRxADC.xI || !State.ExternalRxADC.xQ) 
        return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    for (i=0; i<N; i++)
    {
        if (feof(F)) return STATUS(WI_ERROR_FILE_FORMAT);
        fgets(buf, 255, F); cptr=buf;
        while (*cptr==' ') cptr++;
        State.ExternalRxADC.xI[i].word = strtoul(cptr, &endptr, 0); cptr=endptr;
        while (*cptr==' ') cptr++;
        State.ExternalRxADC.xQ[i].word = strtoul(cptr, &endptr, 0);
    }
    fclose(F);

    State.ExternalRxADC.Nx = N;
    strcpy(State.ExternalRxADC.Filename, filename);
    State.ExternalRxADC.Use = 1;

    return WI_SUCCESS;
}
// end of DW52_LoadRxADC_from_File()

// ================================================================================================
// FUNCTION : DW52_ClearRxADC
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_ClearRxADC(void)
{
    if (State.ExternalRxADC.Use)
    {
        State.ExternalRxADC.Use = 0;
        if (State.ExternalRxADC.xI) wiFree(State.ExternalRxADC.xI);
        if (State.ExternalRxADC.xQ) wiFree(State.ExternalRxADC.xQ);
        State.ExternalRxADC.xI = NULL;
        State.ExternalRxADC.xQ = NULL;
    }
    return WI_SUCCESS;
}
// end of DW52_ClearRxADC()

// ================================================================================================
// FUNCTION : DW52_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_ParseLine(wiString text)
{
    char buf[256]; wiStatus status;

    PSTATUS(wiParse_Int    (text,"DW52.M",              &(State.M), 1, 10000));
    PSTATUS(wiParse_Boolean(text,"DW52.dumpTX",         &(State.dumpTX)) );
    PSTATUS(wiParse_Boolean(text,"DW52.dumpRX",         &(State.dumpRX)) );
    PSTATUS(wiParse_Boolean(text,"DW52.shortPreamble",  &(State.shortPreamble)));
    PSTATUS(wiParse_UInt   (text,"DW52.TXPWRLVL",       &(State.TXPWRLVL.word), 0, 255));
    PSTATUS(wiParse_UInt   (text,"DW52.T_PHYEnB",       &(State.T_PHYEnB[0]), 0, (unsigned)80e6));
    PSTATUS(wiParse_UInt   (text,"DW52.T_PHYEnB[0]",    &(State.T_PHYEnB[0]), 0, (unsigned)80e6));
    PSTATUS(wiParse_UInt   (text,"DW52.T_PHYEnB[1]",    &(State.T_PHYEnB[1]), 0, (unsigned)80e6));
    PSTATUS(wiParse_UInt   (text,"DW52.T_PHYEnB[2]",    &(State.T_PHYEnB[2]), 0, (unsigned)80e6));
    PSTATUS(wiParse_UInt   (text,"DW52.T_PHYEnB[3]",    &(State.T_PHYEnB[3]), 0, (unsigned)80e6));

    XSTATUS(status = wiParse_Function(text,"DW52_LoadRxADC(%255s)",buf));
    if (status==WI_SUCCESS)
    {
        XSTATUS(DW52_LoadRxADC_from_File(buf));
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"DW52_ClearRxADC()"));
    if (status==WI_SUCCESS)
    {
        XSTATUS(DW52_ClearRxADC());
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of DW52_ParseLine()

// ================================================================================================
// FUNCTION : DW52_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_WriteConfig(wiMessageFn MsgFunc)
{
    char t[4][32]; int i;

    for (i=0; i<4; i++) sprintf(t[i],"%s",wiUtil_Comma(State.T_PHYEnB[i]));

    if (State.dumpTX) MsgFunc(" dumpTX ENABLED\n");
    if (State.dumpRX) MsgFunc(" dumpRX ENABLED\n");
    MsgFunc(" Channel Sample Rate   = %d x 80 MS/s\n",State.M);
    MsgFunc(" 802.11b Preamble Mode = %s\n",State.shortPreamble? "Short":"Long");
    MsgFunc(" PHY Header TXPWRLVL   = 0x%02X\n",State.TXPWRLVL);
    if (State.T_PHYEnB[2] || State.T_PHYEnB[3])
        MsgFunc(" DW_PHYEnB Assertion   = [%s-%s)T; [%s-%s)T\n",t[0],t[1],t[2],t[3]);
    else
        MsgFunc(" DW_PHYEnB Assertion   = [%s-%s)T\n",t[0],t[1]);
    if (State.ExternalRxADC.Use) {
        char s[512];
        sprintf(s," >>>>>>  Externl RxADC = %s  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",State.ExternalRxADC.Filename);
        s[78]='\n'; s[79]='\0';
        MsgFunc(s);
    }
    else
    {
        XSTATUS( RF52_WriteConfig(MsgFunc) );
    }
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Modem                 = Mojave Baseband (DW52MBxx)\n");
    XSTATUS( Mojave_WriteConfig(MsgFunc) );
    return WI_SUCCESS;
}
// end of DW52_WriteConfig()

// ================================================================================================
// FUNCTION : DW52_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus DW52_StatePointer(DW52_State **pState)
{
    if (pState) *pState = &State;
    return WI_SUCCESS;
}
// end of DW52_StatePointer()

// ================================================================================================
// FUNCTION : DW52()
// ------------------------------------------------------------------------------------------------
// Purpose  : This is the top-level interface function accessed by wiPHY; it does not
//            contain code used to define Mojave hardware.
// ================================================================================================
wiStatus DW52(int command, void *pdata)
{
    if (Verbose) wiPrintf(" DW52(0x%08X, @x%08X)\n",command,pdata);

    switch (command & 0xFFFF0000)
    {
        case WIPHY_WRITE_CONFIG:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pdata);
            XSTATUS(DW52_WriteConfig(MsgFunc));
        }  break;

        case WIPHY_ADD_METHOD:    
        {
            wiPhyMethod_t *method = (wiPhyMethod_t *)pdata;
            method->TxFn = DW52_TX;
            method->RxFn = DW52_RX;
            XSTATUS(wiParse_AddMethod(DW52_ParseLine));
            XSTATUS(wiParse_AddMethod(Mojave_ParseLine));
            XSTATUS( Mojave_StatePointer   (&State.Mojave.pTX,  &State.Mojave.pRX,  &State.Mojave.pReg ));
            XSTATUS(bMojave_StatePointer   (&State.Mojave.pbRX));
            
            XSTATUS(RF52_Init());
            XSTATUS(RF52_StatePointer(&State.RF52.pTX, &State.RF52.pRX));

            XSTATUS( Mojave_PowerOn());
            State.M = 1;
            State.T_PHYEnB[0] = 20;
            State.T_PHYEnB[1] = (unsigned)80e6;
        }  break;

        case WIPHY_REMOVE_METHOD:
            XSTATUS(wiParse_RemoveMethod(DW52_ParseLine));
            XSTATUS(wiParse_RemoveMethod(Mojave_ParseLine));
            XSTATUS(RF52_Close());
            XSTATUS(Mojave_PowerOff());
            XSTATUS(DW52_ClearRxADC());
            if (State.TX.s[0]) { wiFree(State.TX.s[0]); State.TX.s[0] = NULL; }
            if (State.RX.s[0]) { wiFree(State.RX.s[0]); State.RX.s[0] = NULL; }
            if (State.RX.s[1]) { wiFree(State.RX.s[1]); State.RX.s[1] = NULL; }
            break;

        case WIPHY_SET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;
        case WIPHY_GET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;

        default: return WI_WARN_METHOD_UNHANDLED_COMMAND;
    }
    return WI_SUCCESS;
}
// end of DW52()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2006-10-31 B.Brickner: Added RF52 init/close to this module and removed from wiMain
// 2006-12-20 B.Brickner: Added element TX.s[1] = NULL (terminator)

//--< CalcEVM.c >----------------------------------------------------------------------------------
//=================================================================================================
//
//  EVM Calculator PHY Top
//  Copyright 2006-2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wiPHY.h"
#include "wiUtil.h"
#include "wiParse.h"
#include "CalcEVM.h"
#include "CalcEVM_OFDM.h"
#include "wiChanl.h"
#include "wiTest.h"

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean  Verbose = 0;
static CalcEVM_State_t State = {0};

static const double pi = 3.14159265358979323846;

//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((pstatus=WICHK(arg))<=0) return pstatus;

// ================================================================================================
// FUNCTION  : CalcEVM_TX()
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
wiStatus CalcEVM_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    if (Verbose) wiPrintf(" CalcEVM_TX_PHY(%d,@x%08X,...)\n",Length,data);

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!data) return STATUS(WI_ERROR_PARAMETER3);
    if (!Ns)   return STATUS(WI_ERROR_PARAMETER4);
    if (!S)    return STATUS(WI_ERROR_PARAMETER5);
    
    // ------------------------
    // Use the DW52 Transmitter
    // ------------------------
    if (State.UseMojaveTX)
    {
        #ifdef __DW52_H
        {
            DW52_State     *pState;
            XSTATUS( DW52_StatePointer(&pState) );
            XSTATUS( DW52_TX(bps, Length, data, Ns, S, Fs, Prms) );

            // -----------------
            // Output parameters
            // -----------------
            if (S)    *S    = pState->TX.s;
            if (Ns)   *Ns   = pState->TX.Ns;
            if (Fs)   *Fs   = 80.0E+6 * pState->M;
            if (Prms) *Prms = 1.0;
        
            return WI_SUCCESS;
        }
        #else
            wiUtil_WriteLogFile("DW52 Not Accessible from CalcEVM\n");  
            return STATUS(WI_ERROR);
        #endif
    }
    else
    {
        XSTATUS(CalcEVM_OFDM_TX(bps, Length, data, Ns, S, Fs, Prms) );
    }
    return WI_SUCCESS;
}
// end of CalcEVM_TX()

// ================================================================================================
// FUNCTION  : CalcEVM_Mojave_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
#ifdef __MOJAVE_H
wiStatus CalcEVM_Mojave_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[])
{
    static Mojave_CtrlState          inReg,      outReg;      // top-level control signals
    static Mojave_DataConvCtrlState  DataConvIn, DataConvOut; // data converter control
    static Mojave_RadioDigitalIO     RadioSigOut = {0};       // radio/RF signals
    wiComplex s[2] = {0};
    
    
    Mojave_RX_State *pRX;
    MojaveRegisters *pReg;
    bMojave_RX_State *pbRX;

    if (Verbose) wiPrintf(" CalcEVM_RX(@x%08X,@x%08X,%d,@x%08X)\n",Length,data,Nr,R);
    if (InvalidRange(Nr, 400, 1<<28)) return STATUS(WI_ERROR_PARAMETER3);
    if (!R   ) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);

    XSTATUS( Mojave_StatePointer(NULL, &pRX, &pReg ));
    XSTATUS(bMojave_StatePointer(&pbRX));

    // ----------------------------
    // Check Baseband Configuration
    // ----------------------------
    if (pReg->RXMode.w2  != 1) { wiUtil_WriteLogFile("Mojave RXMode must be set to 1 when used with CalcEVM\n");  return STATUS(WI_ERROR); }
    if (pReg->PathSel.w2 != 1) { wiUtil_WriteLogFile("Mojave PathSel must be set to 1 when used with CalcEVM\n"); return STATUS(WI_ERROR); }
    pRX->EVM.Enabled = 1; // force EVM calculation to be enabled

    
    // ----------------------------------------------
    // Resize Arrays for excess receive signal length
    // ----------------------------------------------
    if (!pbRX->N44 || ((unsigned)Nr != pRX->rLength))
    {  
        pRX->rLength = Nr;
        pbRX->N44    = Nr * 44/80 + 2; // +2 for rounding and initial clock index=1
        XSTATUS(  Mojave_RX_AllocateMemory() );
        XSTATUS( bMojave_RX_AllocateMemory() );
    }
    // ------------------------------------------
    // Restart the PHY modules for the new packet
    // ------------------------------------------
    XSTATUS( Mojave_RX_Restart(pbRX) ); // baseband restart
    Mojave_RX_ClockGenerator(1);                     // reset clock generator
    inReg.bRXEnB = 1;                                // reset state for bRXEnB
    inReg.State = outReg.State = 0;                  // reset RX/Top control state

    // ---------------------------
    // Clear the event/fault flags
    // ---------------------------
    pRX->EventFlag.word = 0;
    pRX->Fault = 0;
    pRX->x_ofs = -1;
        
    // ----------------------------------------------------------------
    // RECEIVER -------------------------------------------------------
    // Clocked Operation With Mixed Clock/Symbol/Packet Timing Accuracy
    // ...the base clock is 80 MHz; all other timing is derived
    // ----------------------------------------------------------------
    while (pRX->k40 < pRX->Nr)
    {
        Mojave_RX_ClockGenerator(0); // generate next clock edge(s)

        // -----------------------------
        // PHY RX at Base Rate of 80 MHz
        // -----------------------------
        if (pRX->clock.posedgeCLK80MHz)
        { 
            // -------------------------
            // Clock Registers at 80 MHz
            // -------------------------
            DataConvOut = DataConvIn;
            outReg      = inReg;

            // -----------------------------------------
            // Data Conversion - Analog-to-Digital (ADC)
            // -----------------------------------------
            if (pRX->clock.posedgeCLK40MHz)
            {
                if (R[0]) s[0] = R[0][pRX->k80];
                if (R[1]) s[1] = R[1][pRX->k80];

                Mojave_ADC(DataConvIn, &DataConvOut, s[0], s[1], pRX->qReal[0]+pRX->k40, pRX->qImag[0]+pRX->k40, pRX->qReal[1]+pRX->k40, pRX->qImag[1]+pRX->k40);
           }
            // ----------------------------------
            // Digital Baseband Receiver (Mojave)
            // ----------------------------------
       XSTATUS( Mojave_RX(0, &RadioSigOut, &DataConvIn, DataConvOut, &inReg, outReg) );
            
        }
    }
    // ----------------
    // Extract EVM Data
    // ----------------
    {
        int i, j, k;
//      int du=pRX->x_ofs+82+144;   //82 for FFT delay; 144: LPF+16 for SIG GI. Starting point of SIG;
        int du=82; //du is set to be consistant with Dakota;
    
        State.EVM.W_EVMdB = 0.0;
        for (i=0; i<  64; i++) State.EVM.h[i] = pRX->EVM.h[0][i];   
        State.EVM.N_SYM = pRX->EVM.N_SYM;
        State.EVM.Se2   = pRX->EVM.Se2[0];
        State.EVM.EVM   = pRX->EVM.EVM[0];
        State.EVM.cCFO  = pRX->EVM.cCFO*10e6/pi/16;
        State.EVM.fCFO  = pRX->EVM.fCFO*10e6/pi/16;
        
        for (i=0; i<  64; i++) State.EVM.cSe2[i] = pRX->EVM.cSe2[0][i];
        for (i=0; i<State.EVM.N_SYM; i++) State.EVM.nSe2[i] = pRX->EVM.nSe2[0][i];
        
        if (!pRX->Fault)
        {    
            if (State.EVM.x) wiFree(State.EVM.x);
            State.EVM.x = (wiComplex *)wiCalloc(64 * (State.EVM.N_SYM+1), sizeof(wiComplex));
            if (!State.EVM.x) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
            
            if (State.EVM.u) wiFree(State.EVM.u); 
            State.EVM.u=(wiComplex *)wiCalloc(pRX->Nu-du, sizeof(wiComplex));
            if (!State.EVM.u) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

            for (i=k=0; i<=pRX->EVM.N_SYM; i++) {
                for (j=0; j<64; j++, k++) {
                    State.EVM.x[k] = pRX->EVM.x[0][j][i];
                }
            }

            for (i=0; i<(int)(pRX->Nu-du); i++)
            {
                State.EVM.u[i].Real=(double) pRX->uReal[0][i+du].word;
                State.EVM.u[i].Imag=(double) pRX->uImag[0][i+du].word;
            }
        }
    }  
    // -----------------------------------------------
    // Return PSDU and Length (drop 8 byte PHY header)
    // -----------------------------------------------
    if (Length) *Length = pRX->N_PHY_RX_WR - 8;
    if (data)     *data = pRX->PHY_RX_D    + 8;

    return WI_SUCCESS;
}
#endif
// end of CalcEVM_Mojave_RX()

// ================================================================================================
// FUNCTION  : CalcEVM_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus CalcEVM_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[])
{
    if (Verbose) wiPrintf(" CalcEVM_RX(@x%08X,@x%08X,%d,@x%08X)\n",Length,data,Nr,R);
    if (!R   ) return STATUS(WI_ERROR_PARAMETER4);
    if (!R[0]) return STATUS(WI_ERROR_PARAMETER4);

    if (State.UseMojaveRX)
    {   
        #ifdef __MOJAVE_H
            
            XSTATUS( CalcEVM_Mojave_RX(Length, data, Nr, R) );
            State.EVM.EVMdB = 10.0*log10(State.EVM.EVM);
        #else
            return STATUS( WI_ERROR_UNDEFINED_CASE);
        #endif
    }
    else
    {  
        XSTATUS( CalcEVM_OFDM_RX(Length, data, Nr, R) );
        State.EVM.EVMdB = 20.0*log10(State.EVM.EVM);
    }
    
    return WI_SUCCESS;
}
// end of CalcEVM_RX()

// ================================================================================================
// FUNCTION   : CalcEVM_Test()
// ------------------------------------------------------------------------------------------------
// Purpose    : Calling Pr_dBm model to calculate EVM for multiple packets  
// Parameters : none
// ================================================================================================
wiStatus CalcEVM_Test(void)
{ 
    wiTest_State_t *pState  = wiTest_State();
    CalcEVM_OFDM_RxState_t *pRX = CalcEVM_OFDM_RxState();
    
    unsigned int n, i, N=0;
    double EVMdB = 0.0, Se2 = 0.0;
    double cSe2[64] = {0};
    double W_EVM = 0.0, W_EVMdB = 0.0;
    int GoodPackets = 0;
    
    if (Verbose) wiPrintf("CalcEVM_Test()\n");
    XSTATUS( wiTest_StartTest("CalcEVM_Test()") );
    wiTestWr(" Pr = %1.1f dBm\n\n",pState->MinPrdBm);
        
    XSTATUS( wiTest_ClearCounters() );

    for (n=0; n<pState->MaxPackets; n++)
    {
    
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );

        if (pRX->Fault) {
            wiPrintf("Fault=%d   SigDet=%d    FSOffset=%d \n", pRX->Fault, pRX->SGSigDet, pRX->FSOffset);
            wiTestWr("Fault=%d   SigDet=%d    FSOffset=%d \n", pRX->Fault, pRX->SGSigDet, pRX->FSOffset);
        }
        
        if (!pRX->Fault)
        {
            for (i=0; i<64; i++) cSe2[i] += State.EVM.cSe2[i];
            Se2 += State.EVM.Se2;
            N   += (48 * (State.EVM.N_SYM));  
            EVMdB = 10.0 * log10(Se2 / ((double)N));
    
            W_EVM += pow(10.0, State.EVM.W_EVMdB/10.0);  
            GoodPackets += 1;
            W_EVMdB = 10*log10(W_EVM/GoodPackets);  

            wiTestWr(" EVM = %5.2f dB, Mean EVM = %5.2f dB \n",State.EVM.EVMdB,EVMdB);
        }
            
    }    
    wiTestWr(" Number of Packets =  %d   Good Packets=%d \n",pState->MaxPackets, GoodPackets);
    if (GoodPackets > 0)
    {  
        wiPrintf(" Samples/Packet    =  48x%d\n",State.EVM.N_SYM); 
        wiPrintf(" Mean EVM          = %1.3f dB\n",EVMdB); 
        wiPrintf(" Weighted EVM =%1.3f dB \n", W_EVMdB);  
        wiPrintf(" BER=%7.2f    PER=%7.2f \n", pState->BER, pState->PER); 

        wiTestWr(" Samples/Packet    =  48x%d\n",State.EVM.N_SYM);
        wiTestWr(" Mean EVM          = %1.3f dB\n",EVMdB);
        wiTestWr(" Weighted EVM =%1.3f dB \n", W_EVMdB);  
        wiTestWr(" BER=%7.2f    PER=%7.2f \n", pState->BER, pState->PER); 
    }

    XSTATUS( wiTest_EndTest() );
    return WI_SUCCESS;
}
// end of CalcEVM_Test()


// ================================================================================================
// FUNCTION  : CalcEVM_EVM_Sensitivity()
// ------------------------------------------------------------------------------------------------
// Purpose   : Meaure sensitivity of EVM to a paramter, (SNR in this case) 
// Parameters: none
// ================================================================================================
wiStatus CalcEVM_EVM_Sensitivity(int paramtype, char paramstr[], double x1, double dx, double x2)
{
    wiUInt n, Init,GoodPackets, done=0;
    double x, Se2, EVMdB, N, minSe2, maxSe2, minEVMdB, maxEVMdB, pktSe2;
    double W_EVM=0.0, W_EVMdB;  
    char paramset[256];
    wiInt N_SYM = 0;  //local N_SYM variable;

    CalcEVM_OFDM_RxState_t *pRX = CalcEVM_OFDM_RxState();
    wiTest_State_t  *pState = wiTest_State();

    XSTATUS( wiTest_StartTest("CalcEVM_EVM_Sensitivity()") );

    switch (paramtype)
    {
        case 1: wiTestWr(" Parameter Sweep: %s = %d:%d:%d\n",paramstr,(int)x1,(int)dx,(int)x2); break;
        case 2: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        case 3: wiTestWr(" Parameter Sweep: %s = %d:%d (%d)\n",paramstr,(int)x1,(int)x2,(int)dx); break;
        case 4: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        default: return WI_ERROR_PARAMETER1;
    }
    wiTestWr("\n");
    wiTestWr("  SNR     EVM     W_EVM     min      max     TotalPackets   GoodPackets     BER      PER  \n");
    wiTestWr(" -----  -------  -------  -------  -------  --------------  ------------  -------  -------\n");

    for (x=x1; !done; )
    {
        switch (paramtype)
        {
            case 1: sprintf(paramset,"%s = %d",paramstr, (int)x); break;
            case 2: sprintf(paramset,"%s = %8.12e",paramstr,  x); break;
            case 3: sprintf(paramset,"%s = %d",paramstr, (int)x); break;
            case 4: sprintf(paramset,"%s = %8.12e",paramstr,  x); break;
        }
        XSTATUS( wiParse_Line(paramset) );
        XSTATUS( wiTest_ClearCounters() );
    
        Se2 = 0.0;
        W_EVM=0.0;
        Init=0;  //For the first nonfault packet, initialize minSe2 and maxSe2;
        minSe2=0.0;
        maxSe2=0.0;
        GoodPackets=0; //# of non-fault packets; 
       
        for (n=0; n<pState->MaxPackets; n++) {
            XSTATUS( wiTest_TxRxPacket(0, pState->MinSNR, pState->DataRate, pState->Length) );
                    
            if (!pRX->Fault)
            {
                N_SYM=State.EVM.N_SYM;
                N = 48.0 * State.EVM.N_SYM;
                pktSe2 = min(N, State.EVM.Se2);  // clamp EVM at 100% (0 dB)
                Se2 = Se2 + pktSe2;
                W_EVM+=pow(10.0,State.EVM.W_EVMdB/10.0); 
                GoodPackets+=1;
                
                if (Init) {
                    minSe2 = min(minSe2, pktSe2);
                    maxSe2 = max(maxSe2, pktSe2);
                }
                else   {minSe2 = pktSe2; maxSe2 = pktSe2; Init=1;}
            }
        }
        if (GoodPackets)
        {
            N = 48.0 * N_SYM;
            minEVMdB = minSe2? 10*log10(minSe2/N) : 0.0;
            maxEVMdB = minSe2? 10*log10(maxSe2/N) : 0.0;
            N = 48.0 * N_SYM * GoodPackets;  //only count the good packets;
            EVMdB = (Se2 && N)? 10*log10(Se2/N) :0.0;
            W_EVMdB=10*log10(W_EVM/GoodPackets);  
        }
        else
        {
            minEVMdB=0.0;
            maxEVMdB=0.0;
            EVMdB=0.0;
            W_EVMdB=0.0;
        }

        switch (paramtype)
        {
            case 1: case 3: wiPrintf(" %s =% 6d: EVM = %4.2f dB  Weighted EVM=%4.2f dB BER=%.5f  PER=%.5f \n", paramstr, (int)x, EVMdB,W_EVMdB,pState->BER, pState->PER); break;
            case 2: case 4: wiPrintf(" %s =% 8.4e: EVM = %4.2f dB  Weighted EVM=%4.2f dB  BER=%.5f  PER=%.5f \n", paramstr, x, EVMdB, W_EVMdB,pState->BER, pState->PER); break;
        }
        switch (paramtype)
        {
            case 1: wiTestWr(" % 5d % 7.2f  %7.2f % 7.2f  % 7.2f  %12d  %12d     %7.4f   %7.4f\n", (int)x, EVMdB, W_EVMdB,minEVMdB, maxEVMdB, pState->MaxPackets, GoodPackets, pState->BER, pState->PER); break;
            case 2: wiTestWr("% 13.4e % 7.2f %7.2f  % 7.2f  % 7.2f  %12d  %12d  %7.4f %7.4f \n",      x, EVMdB, W_EVMdB, minEVMdB, maxEVMdB,pState->MaxPackets, GoodPackets,pState->BER, pState->PER); break;
            case 3: wiTestWr(" % 5d % 7.2f  %7.2f  % 7.2f  % 7.2f %12d  %12d  %7.4f  %7.4f\n", (int)x, EVMdB, W_EVMdB, minEVMdB, maxEVMdB,pState->MaxPackets, GoodPackets,pState->BER, pState->PER); break;
            case 4: wiTestWr("% 13.4e % 7.2f %7.2f  % 7.2f  % 7.2f %12d  %12d %7.4f  %7.4f \n",      x, EVMdB, W_EVMdB, minEVMdB, maxEVMdB,pState->MaxPackets, GoodPackets,pState->BER, pState->PER); break;
        }
     
        switch (paramtype)
        {
            case 1: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 2: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 3: done = (x*dx>x2); x *= dx; break;
            case 4: done = (x*dx>x2); x *= dx; break;
        }
    }
    WIFREE( pRX->trace_r );
    XSTATUS(wiTest_EndTest());
    return WI_SUCCESS;
}
// end of CalcEVM_EVM_Sensitivity()


// ================================================================================================
// FUNCTION : CalcEVM_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus CalcEVM_ParseLine(wiString text)
{
    wiStatus status, pstatus;
    
    PSTATUS(wiParse_Boolean(text,"CalcEVM.UseMojaveRX", &(State.UseMojaveRX)));
    PSTATUS(wiParse_Boolean(text,"CalcEVM.UseMojaveTX", &(State.UseMojaveTX)));
    PSTATUS(wiParse_Boolean(text,"CalcEVM.Verbose", &Verbose));

    XSTATUS(status = wiParse_Command(text,"CalcEVM_Test()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(CalcEVM_Test());
        return WI_SUCCESS; 
    }

    {
        double x1, x2, dx;
        char paramstr[128]; int paramtype;

        XSTATUS(status = wiParse_Function(text,"CalcEVM_EVM_Sensitivity(%d, %128s, %f, %f, %f)",&paramtype, paramstr, &x1, &dx, &x2));
        if (status == WI_SUCCESS) {
            XSTATUS(CalcEVM_EVM_Sensitivity(paramtype, paramstr, x1, dx, x2));
            return WI_SUCCESS; }
    }
 
    return WI_WARN_PARSE_FAILED;
}
// end of CalcEVM_ParseLine()

// ================================================================================================
// FUNCTION : CalcEVM_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus CalcEVM_WriteConfig(wiMessageFn MsgFunc)
{
    char s_TX[2][64] = {"Floating Point OFDM TX","Mojave Baseband TX (DW52)"};
    char s_RX[2][64] = {"Floating Point OFDM RX","Mojave Baseband RX (DW52)"};

    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" TX Modem          = %s\n",s_TX[State.UseMojaveTX]);
    
    if (!State.UseMojaveTX)
    {
        XSTATUS(CalcEVM_OFDM_TXWriteConfig(MsgFunc));
    }
    else
    {
        #ifdef __MOJAVE_H
            RF52_TxState_t *pRF_TX;
            XSTATUS(RF52_StatePointer(&pRF_TX, NULL));
            XSTATUS( Mojave_WriteConfig(MsgFunc) );
            MsgFunc("                    \n");
            MsgFunc(" TX RF.AC.Enabled      = %d\n", pRF_TX->AC.Enabled);
        #endif
    }

    MsgFunc("                           \n");   
    MsgFunc(" RX Modem              = %s\n",s_RX[State.UseMojaveRX]);
    
    if (!State.UseMojaveRX) 
    {
        XSTATUS( CalcEVM_OFDM_RXWriteConfig(MsgFunc) );
    }
    return WI_SUCCESS;
}
// end of CalcEVM_WriteConfig()

// ================================================================================================
// FUNCTION : CalcEVM_StatePointer()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
CalcEVM_State_t *CalcEVM_State(void)
{
    return &State;
}
// end of CalcEVM_State()

// ================================================================================================
// FUNCTION : CalcEVM()
// ------------------------------------------------------------------------------------------------
// Purpose  : This is the top-level interface function accessed by wiPHY
// ================================================================================================
wiStatus CalcEVM(int command, void *pdata)
{
    if (Verbose) wiPrintf(" CalcEVM(0x%08X, @x%08X)\n",command,pdata);

    switch (command & 0xFFFF0000)
    {
        case WIPHY_WRITE_CONFIG:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pdata);
            XSTATUS(CalcEVM_WriteConfig(MsgFunc));
        }  break;

        case WIPHY_ADD_METHOD:    
        {
            wiPhyMethod_t *method = (wiPhyMethod_t *)pdata;
            method->TxFn = CalcEVM_TX;
            method->RxFn = CalcEVM_RX;
            State.UseMojaveTX = 1;
            State.UseMojaveRX = 0;
            XSTATUS(wiParse_AddMethod(CalcEVM_ParseLine));
            XSTATUS(CalcEVM_OFDM_Init());
        }  break;

        case WIPHY_REMOVE_METHOD: 
            XSTATUS(wiParse_RemoveMethod(CalcEVM_ParseLine));
            WIFREE( State.TX.s[0] );
            WIFREE( State.EVM.x   );
            WIFREE( State.EVM.u   );
            XSTATUS(CalcEVM_OFDM_Close());
            break;

        case WIPHY_SET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;
        case WIPHY_GET_DATA : return WI_WARN_METHOD_UNHANDLED_PARAMETER; break;

        default: return STATUS(WI_WARN_METHOD_UNHANDLED_COMMAND);
    }
    return WI_SUCCESS;
}
// end of DW52()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

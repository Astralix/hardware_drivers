//--< wiTest.c >-----------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Test Routines
//  Copyright 2001-2002 Bermai, 2005-2010 DSP Group, Inc. All Rights Reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "wise.h"
#include "wiMAC.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiRnd.h"
#include "wiTest.h"
#include "wiUtil.h"

static wiTest_State_t *State = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  TxRxPacket Parameter Structure
//  The parameters for wiTest_TxRxPacketCore as passed as a pointer to this structure to accomodate
//  the requirements for the thread start function.
//
typedef struct
{
    wiInt  Mode;
    wiReal PrdBm;
    wiReal bps;
    wiInt  nBytes;

} wiTest_TxRxPacket_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Module Local Function Prototypes
//
int      wiTest_Output(const char *format, ...);
wiStatus wiTest_ParseLine(wiString text);

#define  STATUS(arg)  WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg) < 0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((pstatus=WICHK(arg)) <= 0) return pstatus;

// ================================================================================================
// FUNCTION  : wiTest_Init()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize the test module
// Parameters: none
// ================================================================================================
wiStatus wiTest_Init(void)
{
    if (State) return WI_ERROR_MODULE_ALREADY_INIT;

    WICALLOC( State, 1, wiTest_State_t );

    // Setup default values
    //
    State->DataRate   = 54000000;
    State->FixedByte  = 0xC5;
    State->Length     = 1000;
    State->MinSNR     =    0.0;
    State->MaxSNR     =   60.0;
    State->StepSNR    =    1.0;
    State->MinPER     =    0.001;
    State->MinFail    =  100;
    State->MaxPackets =10000;
    State->UpdateRate =  100;
    State->MinPrdBm   =  -90;
    State->MaxPrdBm   =   10;
    State->StepPrdBm  =    1;
    State->UpdateMinPacketCount = WI_TRUE;
    State->RndSeed    = WITEST_RNDSEED_RANDOM;
    sprintf(State->FilenameExtension, "out");

    State->BackgroundPacket[1].DataRate = 24000000; // set for compatibility with Mojave regression tests
    State->BackgroundPacket[1].Length   = 1000;

    WICALLOC( State->TxData[0], WITEST_MAX_LENGTH, wiUWORD );

    XSTATUS(wiParse_AddMethod(wiTest_ParseLine));
    
    return WI_SUCCESS;
}
// end of wiTest_Init()

// ================================================================================================
// FUNCTION  : wiTest_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_Close(void)
{
    int i;
    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    for (i = 0; i < WISE_MAX_THREADS; i++) {
        WIFREE(State->TxData[i]);
    }
    WIFREE(State);
    XSTATUS( wiParse_RemoveMethod(wiTest_ParseLine) );

    return WI_SUCCESS;
}
// end of wiTest_Close()

// ================================================================================================
// FUNCTION  : wiTest_FreeThreadData()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_FreeThreadData(void)
{
    int i;

    XSTATUS( wiPHY_Command(WIPHY_CLOSE_ALL_THREADS) );
    XSTATUS( wiChanl_CloseAllThreads(1) );
    for (i = 1; i < WISE_MAX_THREADS; i++)
        WIFREE( State->TxData[i] );

    return WI_SUCCESS;
}
// end of wiTest_FreeThreadData()

// ================================================================================================
// FUNCTION  : wiTest_SetTxRxCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_SetTxRxCallback(wiTestCallbackFn_t Fn)
{
    State->TxRxCallback = Fn;
    return WI_SUCCESS;
}
// end of wiTest_SetTxRxCallback()

// ================================================================================================
// FUNCTION  : wiTest_SetOnePacketCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_SetOnePacketCallback(wiTestCallbackFn_t Fn)
{
    State->OnePacketCallback = Fn;
    return WI_SUCCESS;
}
// end of wiTest_SetOnePacketCallback()

// ================================================================================================
// FUNCTION  : wiTest_SetScriptCallback()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_SetScriptCallback(wiTestCallbackFn_t Fn)
{
    State->ScriptCallback = Fn;
    return WI_SUCCESS;
}
// end of wiTest_SetScriptCallback()

// ================================================================================================
// FUNCTION  : wiTest_ClearCounters()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clear packet, error counters
// Parameters: none
// ================================================================================================
wiStatus wiTest_ClearCounters(void)
{
    State->BitCount         = 0;
    State->BitErrorCount    = 0;
    State->PacketCount      = 0;
    State->PacketErrorCount = 0;
    State->MinPacketCount   = 0;
    State->BER              = 0.0;
    State->PER              = 0.0;
    return WI_SUCCESS;
}
// end of wiTest_ClearCounters()

// ================================================================================================
// FUNCTION  : wiTest_UpdateCountByPoint()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_UpdateCountByPoint(wiReal ParamValue)
{
    if (State->CountByPoint.NumberOfPoints < WITEST_MAX_POINTS)
    {
        unsigned int k = State->CountByPoint.NumberOfPoints;

        State->CountByPoint.ParamValue[k]       = ParamValue;
        State->CountByPoint.BitCount[k]         = State->BitCount;
        State->CountByPoint.BitErrorCount[k]    = State->BitErrorCount;
        State->CountByPoint.PacketCount[k]      = State->PacketCount;
        State->CountByPoint.PacketErrorCount[k] = State->PacketErrorCount;
        State->CountByPoint.BER[k]              = State->BER;
        State->CountByPoint.PER[k]              = State->PER;

        State->CountByPoint.NumberOfPoints++;
    }
    return WI_SUCCESS;
}
// end of wiTest_UpdateCountByPoint()

// ================================================================================================
// Function  : wiTest_StatvSNR()
// ------------------------------------------------------------------------------------------------
//=======================================================================================
wiStatus wiTest_StatvSNR(void)
{
    unsigned int i;
    wiReal  SNR, X;
    wiReal  Xmin, Xmax, SX;
    wiReal  s = (State->StepPrdBm < 0) ? -1:1;

    XSTATUS( wiTest_StartTest("wiTest_Stat_vs_SNR()") );

    wiTestWr("\n");
    wiTestWr(" SNR(dB)  Minimum   Maximum     Mean    Std.Dev.\n");
    wiTestWr(" -------  --------  --------  --------  --------\n");

    for (SNR = State->MinSNR; s*SNR <=s *State->MaxSNR; SNR += State->StepSNR)
    {
        Xmin = 1e+20;
        Xmax = -1e+20;
        SX   = 0;
        for (i = 0; i < State->MaxPackets; i++)
        {
            XSTATUS( wiTest_TxRxPacket(0, SNR, State->DataRate, State->Length) );
            XSTATUS( wiPHY_GetData(WIPHY_RX_METRIC, &X) );
            SX += X;
            if (X < Xmin) Xmin = X;
            if (X > Xmax) Xmax = X;
        }
        wiPrintf(" SNR=% 5.1f: min=%8.2e, max=%8.2e, mean=%8.2e\n", SNR,Xmin,Xmax,SX/i);
        wiTestWr(" % 6.2f  %9.2e  % 9.2e % 9.2e %9.2e\n", SNR, Xmin, Xmax, SX/i, 0.0);
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of wiTest_StatvSNR()

// ================================================================================================
// Function  : wiTest_PERvSNR()
// ------------------------------------------------------------------------------------------------
//=======================================================================================
wiStatus wiTest_PERvSNR(void)
{
    wiReal  SNR;
    wiReal  s = (State->StepPrdBm < 0)? -1:1;

    XSTATUS( wiTest_StartTest("wiTest_PERvSNR()") );

    wiTestWr("\n");
    wiTestWr("  SNR(dB)   # Errors     BER     # Failures     PER    \n");
    wiTestWr(" ---------  --------  ---------  ----------  --------- \n");

    for (SNR = State->MinSNR; s*SNR <= s*State->MaxSNR; SNR += State->StepSNR)
    {
        XSTATUS( wiTest_ClearCounters() );

        while ((State->PacketErrorCount < State->MinFail) || (State->PacketCount < State->MinPacketCount))
        {
            XSTATUS( wiTest_TxRxPacket(0, SNR, State->DataRate, State->Length) );
            if (State->PacketCount >= State->MaxPackets) break;
        }
        XSTATUS( wiTest_UpdateCountByPoint(SNR) );

        wiPrintf(" SNR =% 5.1f: BER = %8.2e, PER = %8.2e\n", SNR, State->BER, State->PER);
        wiTestWr(" % 8.2f  % 8d  % 9.2e % 10d  % 9.2e\n", SNR, State->BitErrorCount, State->BER, State->PacketErrorCount, State->PER);

        if (State->PER < State->MinPER) break;
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of wiTest_PERvSNR()

// ================================================================================================
// FUNCTION  : wiTest_PERvPrdBm()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_PERvPrdBm(void)
{
    wiReal PrdBm;
    wiReal s = (State->StepPrdBm < 0) ? -1:1; // direction...count down (-1) or up (+1)

    XSTATUS( wiTest_StartTest("wiTest_PERvPrdBm()") );

    wiTestWr("\n");
    wiTestWr(" Pr[dBm]  # Errors     BER     # Failures     PER    \n");
    wiTestWr(" -------  --------  ---------  ----------  --------- \n");

    for (PrdBm = State->MinPrdBm; s*PrdBm <= s*State->MaxPrdBm; PrdBm += State->StepPrdBm)
    {
        XSTATUS( wiTest_ClearCounters() );

        while ((State->PacketErrorCount < State->MinFail) || (State->PacketCount < State->MinPacketCount))
        {
            XSTATUS( wiTest_TxRxPacket(1, PrdBm, State->DataRate, State->Length) );
            if (State->PacketCount >= State->MaxPackets) break;
        }
        XSTATUS( wiTest_UpdateCountByPoint(PrdBm) );

        wiPrintf(" % 5.1f dBm: BER = %8.2e, PER = %8.2e\n", PrdBm, State->BER, State->PER);
        wiTestWr(" % 6.1f  % 8d  % 9.2e % 10d  % 9.2e\n", PrdBm, State->BitErrorCount, State->BER, State->PacketErrorCount, State->PER);

        if (State->PER < State->MinPER) break;
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of wiTest_PERvPrdBm()

// ================================================================================================
// FUNCTION  : wiTest_ComputeCPER
// ------------------------------------------------------------------------------------------------
// Purpose   : Compute corrected PER
// ================================================================================================
wiStatus wiTest_ComputeCPER(wiInt s, wiInt I, wiInt *tmin)
{
    int eCount=0; unsigned int i, j, k, m, n, e;
    wiUWORD *a = State->TxData[wiProcess_ThreadIndex()];
    wiUWORD *b = State->RxData[wiProcess_ThreadIndex()];
    unsigned A, B;

    *tmin = 0;
    n = 1<<s; // number of code bytes assuming maximum length

    // byte symbols
    if (s == 8)
    {
        for (k = 0; k < State->Length; k++)
        {
            e = 0;
            if (k%n == 0) eCount = 0;             // reset error counter
            for (i=0; i<(unsigned)I; i++) 
            {
                if (a[k].word != b[k].word) e++; // byte error
                if (k < State->Length) k++;
            }
            if (e) eCount++;
        }
        *tmin = max(*tmin, eCount); // maximum number of errors per codeword
    }
    else
    {
        i = 0; j = 0; eCount = 0;
        for (k = 0; k < n; k++)
        {
            A = B = 0;
            for (m = 0; m < (unsigned)s; m++)
            {
                A = (A << 1) |  ((a[k].word >> j) & 1);
                B = (B << 1) |  ((b[k].word >> j) & 1);
                j++;
                if (j==8) {
                    i++;
                    j = 0;
                    if (i==n) {m=s; break;}
                }
            }
            if (A != B) eCount++;
        }
        *tmin = eCount;
    }
    return WI_SUCCESS;
}
// end of wiTest_ComputeCPER()

// ================================================================================================
// FUNCTION  : wiTest_CPERvPrdBm()
// ------------------------------------------------------------------------------------------------
// Purpose   : Measure BER/PER/CPER vs Pr[dBm] with "effective" Reed-Solomon coding
// Parameters: none
// ================================================================================================
wiStatus wiTest_CPERvPrdBm(void)
{
    wiReal PrdBm;
    wiReal s = (State->StepPrdBm < 0) ? -1:1;
    wiInt e[2][4]; // correction failure count
    int i, j, t, I;

    XSTATUS( wiTest_StartTest("wiTest_CPERvPrdBm()") );

    t = 32; // max t for s=8
    I = (int)ceil((double)State->Length / (255-2*t));

    wiTestWr("\n");
    wiTestWr("                             |<--- CPER (s=8,I=%d) --->|  |<----- CPER (s=12) ----->|\n",I);
    wiTestWr(" Pr[dBm]     BER       PER    t= 4   t= 8   t=16   t=32    t= 8   t=16   t=32   t=64 \n");
    wiTestWr(" -------  ---------  ------  ------ ------ ------ ------  ------ ------ ------ ------\n");

    for (PrdBm = State->MinPrdBm; s*PrdBm <= s*State->MaxPrdBm; PrdBm += State->StepPrdBm)
    {
        XSTATUS( wiTest_ClearCounters() );
        for (i = 0; i < 2; i++) for (j = 0; j < 4; j++) e[i][j] = 0;
        while (State->PacketCount < State->MaxPackets)
        {
            XSTATUS( wiTest_TxRxPacket(1, PrdBm, State->DataRate, State->Length) );
            XSTATUS( wiTest_ComputeCPER( 8, I, &t) );
            if (t> 4) e[0][0]++; 
            if (t> 8) e[0][1]++; 
            if (t>16) e[0][2]++; 
            if (t>32) e[0][3]++;
            XSTATUS( wiTest_ComputeCPER(12, 1, &t) );
            if (t> 8) e[1][0]++; 
            if (t>16) e[1][1]++; 
            if (t>32) e[1][2]++; 
            if (t>64) e[1][3]++;
        }
        wiPrintf(" % 5.1f dBm: BER = %8.2e, PER = %8.2e\n", PrdBm, State->BER, State->PER);
        wiTestWr(" % 6.1f  % 9.2e % 6.4f ", PrdBm, State->BER, State->PER);
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 4; j++)
                wiTestWr(" %6.4f",(double)e[i][j]/State->MaxPackets);
            wiTestWr(" ");
        }
        wiTestWr("\n");
        if (State->PER < State->MinPER) break;
    }
    XSTATUS( wiTest_EndTest() );

    return WI_SUCCESS;
}
// end of wiTest_CPERvPr()

// ================================================================================================
// FUNCTION  : wiTest_ByteErrorPDF()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_ByteErrorPDF(void)
{
    wiInt Ne[4096];
    unsigned int i, c = 0;

    XSTATUS( wiTest_StartTest("wiTest_ByteErrorPDF()") );

    wiTestWr("\n");
    wiTestWr(" Byte Errors     Count        CDF    \n");
    wiTestWr(" -----------  ----------  -----------\n");

    XSTATUS( wiTest_ClearCounters() );
    while (State->PacketCount < State->MaxPackets)
    {
        XSTATUS( wiTest_TxRxPacket(1, State->MinPrdBm, State->DataRate, State->Length) );
        Ne[State->NumberOfByteErrors]++;
    }
    for (i = 0; i < State->Length; i++) 
    {
        c += Ne[i];
        wiTestWr(" %11d  %10d  %9.4e\n", i, Ne[i], (double)c/State->MaxPackets);
    }
    XSTATUS( wiTest_EndTest() );

    return WI_SUCCESS;
}
// end of wiTest_ByteErrorPDF()

// ================================================================================================
// FUNCTION  : wiTest_OnePacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit/receive a single packet
// Parameters: none
// ================================================================================================
wiStatus wiTest_OnePacket(void)
{
    XSTATUS( wiTest_StartTest("wiTest_OnePacket()") );
    XSTATUS( wiTest_ClearCounters() );

    if (State->OnePacketUsesSNR)
    {
        wiTestWr(" SNR = %1.1f dB\n\n",State->MinSNR);
        XSTATUS( wiTest_TxRxPacket(0, State->MinSNR, State->DataRate, State->Length) );
    }
    else
    {
        wiTestWr(" Pr = %1.1f dBm\n\n",State->MinPrdBm);
        XSTATUS( wiTest_TxRxPacket(1, State->MinPrdBm, State->DataRate, State->Length) );
    }
    wiTestWr(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiTestWr(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiTestWr("\n");

    wiPrintf(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiPrintf(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiPrintf("\n");
        
    XSTATUS( wiTest_UpdateCountByPoint(State->OnePacketUsesSNR ? State->MinSNR : State->MinPrdBm) );
    XSTATUS( wiTest_EndTest() );
    
    if (State->OnePacketCallback != NULL) {
        XSTATUS( State->OnePacketCallback() );
    }
    return WI_SUCCESS;
}
// end of wiTest_OnePacket()

// ================================================================================================
// FUNCTION  : wiTest_OnePacketTx2()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit two packets; receive the second
// Parameters: none
// ================================================================================================
wiStatus wiTest_OnePacketTx2(void)
{
    wiBoolean MultipleTxPackets = State->MultipleTxPackets;
    State->MultipleTxPackets = 0;

    XSTATUS( wiTest_StartTest("wiTest_OnePacketTx2()") );

    wiTestWr(" First TX: Data Rate = %1.1f Mb/s\n",State->BackgroundPacket[1].DataRate/1.0e6);
    wiTestWr(" First TX: Length    = %d Bytes\n",State->BackgroundPacket[1].Length);
    wiTestWr("\n");
    wiTestWr(" Pr = %1.1f dBm\n",State->MinPrdBm);
    wiTestWr("\n");

    XSTATUS( wiChanl_ClearPackets() );
    XSTATUS( wiTest_TxPacket  (1, State->MinPrdBm, State->BackgroundPacket[1].DataRate, State->BackgroundPacket[1].Length, 
                                  State->BackgroundPacket[1].DataType, NULL, NULL, NULL) );

    XSTATUS( wiTest_ClearCounters() );
    XSTATUS( wiTest_TxRxPacket(1, State->MinPrdBm, State->DataRate,  State->Length) );

    wiTestWr(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiTestWr(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiTestWr("\n");

    wiPrintf(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiPrintf(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiPrintf("\n");
        
    XSTATUS( wiTest_UpdateCountByPoint(State->MinPrdBm) );
    XSTATUS( wiTest_EndTest() );

    State->MultipleTxPackets = MultipleTxPackets;
    return WI_SUCCESS;
}
// end of wiTest_OnePacketTx2()

// ================================================================================================
// FUNCTION  : wiTest_OnePacketTxMultiple()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit multiple packets; receive and measure PER on the last
// Parameters: none
// ================================================================================================
wiStatus wiTest_OnePacketTxMultiple(void)
{
    wiBoolean MultipleTxPackets = State->MultipleTxPackets;
    State->MultipleTxPackets = 1;

    XSTATUS( wiTest_OnePacket() );
    XSTATUS( wiTest_UpdateCountByPoint(State->MinPrdBm) );

    State->MultipleTxPackets = MultipleTxPackets;
    return WI_SUCCESS;
}
// end of wiTest_OnePacketTxMultiple()

// ================================================================================================
// FUNCTION  : wiTest_Benchmark()
// ------------------------------------------------------------------------------------------------
// Purpose   : Measure simulation throughput
// Parameters: none
// ================================================================================================
wiStatus wiTest_Benchmark(void)
{
    unsigned int bps, max_BitCount;
    double PrdBm, pps, tCPU, tRuntime;
    clock_t t0 = clock();

    // ---------------------------
    max_BitCount = 2000000;
    PrdBm        = max(-60.0, State->MinPrdBm);
    // ---------------------------

    XSTATUS(wiTest_StartTest("wiTest_Benchmark()"));

    wiTestWr(" Pr       = %4.1f dBm\n",PrdBm);
    wiTestWr(" LENGTH   = %d Bytes\n",State->Length);
    wiTestWr(" DataRate = %1.1f Mb/s\n", State->DataRate/1e6);
    wiTestWr("\n");

    wiPrintf(" Pr       = %4.2f dBm\n",PrdBm);
    wiPrintf(" LENGTH   = %d Bytes\n",State->Length);
    wiPrintf(" DataRate = %1.1f Mb/s\n", State->DataRate/1e6);
    wiPrintf("\n");

    while (State->BitCount < max_BitCount)
    {
        XSTATUS( wiTest_TxRxPacket(1, PrdBm, State->DataRate, State->Length) );
    }
    tRuntime = (double)(clock() - t0)/CLOCKS_PER_SEC;
    tCPU = wiProcess_Time();
    bps = (unsigned int)((double)(State->TotalPacketCount * 8 * State->Length)/ tRuntime);
    pps = (double)State->TotalPacketCount / tRuntime;

    wiTestWr(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiTestWr(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiTestWr("\n");
    wiTestWr(" Runtime     =% 7.1f seconds\n",tRuntime);
    wiTestWr(" CPU Time    =% 7.2f seconds\n",tCPU);
    wiTestWr(" Throughput  = %s bps\n",wiUtil_Comma(bps));
    wiTestWr(" Packet Rate = %4.1f pps\n",pps);
    wiTestWr("\n");

    wiPrintf(" BER = %d / %d = %8.2e\n",State->BitErrorCount,State->BitCount,State->BER);
    wiPrintf(" PER = %d / %d = %8.2e\n",State->PacketErrorCount,State->PacketCount,State->PER);
    wiPrintf("\n");
    wiPrintf(" Runtime     = %4.1f seconds\n",tRuntime);
    wiPrintf(" CPU Time    = %4.2f seconds\n",tCPU);
    wiPrintf(" Throughput  = %s bps\n",wiUtil_Comma(bps));
    wiPrintf(" Packet Rate = %4.1f pps\n",pps);

    XSTATUS(wiTest_EndTest());
    
    return WI_SUCCESS;
}
// end of wiTest_Benchmark()

// ================================================================================================
// FUNCTION  : wiTest_PERvParam()
// ------------------------------------------------------------------------------------------------
// Purpose   : Meaure PER versus an arbitrary parameter
// Parameters: none
// ================================================================================================
wiStatus wiTest_PERvParam(int paramtype, char paramstr[], double x1, double dx, double x2)
{
    wiBoolean done = 0;
    double x = x1;
    char paramset[256];
    wiStatus status;

    XSTATUS( wiTest_StartTest("wiTest_PERvParam()") );

    wiTestWr(" Pr = %1.1f dBm\n",State->MinPrdBm);
    wiTestWr("\n");
    switch (paramtype)
    {
        case 1: wiTestWr(" Parameter Sweep: %s = %d:%d:%d\n",paramstr,(int)x1,(int)dx,(int)x2); break;
        case 2: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        case 3: wiTestWr(" Parameter Sweep: %s = %d:%d (%d)\n",paramstr,(int)x1,(int)x2,(int)dx); break;
        case 4: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        default: return WI_ERROR_PARAMETER1;
    }
    wiTestWr("\n");
    wiTestWr("  ParamValue   # Errors     BER     # Failures     PER    \n");
    wiTestWr(" ------------  --------  ---------  ----------  --------- \n");

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Test Loop
    //
    //  This code executes while parameter "x" increments/decrements from its initial
    //  to its final value.
    //
    while (!done)
    {
        switch (paramtype)
        {
            case 1: sprintf(paramset,"%s = %d",    paramstr, (int)x); break;
            case 2: sprintf(paramset,"%s = %8.12e",paramstr,      x); break;
            case 3: sprintf(paramset,"%s = %d",    paramstr, (int)x); break;
            case 4: sprintf(paramset,"%s = %8.12e",paramstr,      x); break;
        }

        STATUS( status = wiParse_Line(paramset) );
        if (status != WI_SUCCESS)
        {
            wiPrintf(" wiTest_PERvParam failed to parse \"%s\"\n",paramset);
            return WI_ERROR_PARSE_FAILED;
        }
        XSTATUS( wiTest_ClearCounters() );

        while ((State->PacketErrorCount < State->MinFail) || (State->PacketCount < State->MinPacketCount))
        {
            XSTATUS( wiTest_TxRxPacket(1, State->MinPrdBm, State->DataRate, State->Length) );
            if (State->PacketCount >= State->MaxPackets) break;
        }
        XSTATUS( wiTest_UpdateCountByPoint(x) );

        switch (paramtype)
        {
            case 1: case 3: wiPrintf(" %s =% 6d: BER = %8.2e, PER = %6.2f%%\n",   paramstr, (int)x, State->BER, 100.0*State->PER); break;
            case 2: case 4: wiPrintf(" %s =% 8.4e: BER = %8.2e, PER = %6.2f%%\n", paramstr,      x, State->BER, 100.0*State->PER); break;
        }
        switch (paramtype)
        {
            case 1: case 3: wiTestWr( " % 12d % 8d  % 9.2e % 10d  % 9.2e\n", (int)x, State->BitErrorCount, State->BER, State->PacketErrorCount, State->PER); break;
            case 2: case 4: wiTestWr(" %12.4e % 8d  % 9.2e % 10d  % 9.2e\n",      x, State->BitErrorCount, State->BER, State->PacketErrorCount, State->PER); break;
        }
        switch (paramtype)
        {
            case 1: done = (dx < 0) ? (x+dx < x2):(x+dx > x2);  x += dx;  break;
            case 2: done = (dx < 0) ? (x+dx < x2):(x+dx > x2);  x += dx;  break;
            case 3: done = (x*dx > x2);                         x *= dx;  break;
            case 4: done = (x*dx > x2);                         x *= dx;  break;
        }
        if (State->PER < State->MinPER) done = 1;
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of wiTest_PERvParam()

// ================================================================================================
// FUNCTION  : wiTest_ErrorCounter()
// ------------------------------------------------------------------------------------------------
// Purpose   : Count errors and packet failures
// Parameters: n -- Number of message bytes
//             a -- Byte stream #1
//             b -- Byte stream #2
// ================================================================================================
wiStatus wiTest_ErrorCounter(int n, wiUWORD a[], wiUWORD b[]) 
{
    unsigned int e, num_errors = 0;
    int i, k;

    State->NumberOfByteErrors = 0;

    for (k = 0; k < n; k++)
    {
        if (a[k].word != b[k].word)
        {
            State->NumberOfByteErrors++;
            e = a[k].word ^ b[k].word;

            for (i = 0; i < 8; i++)
                num_errors += ((e>>i) & 1);
        }
    }
    State->BitCount         += 8 * n;
    State->BitErrorCount    += num_errors;
    
    State->PacketCount      += 1;
    State->PacketErrorCount += (num_errors ? 1:0);

    State->TotalPacketCount      += 1;
    State->TotalPacketErrorCount += (num_errors ? 1:0);

    State->BER = (double)State->BitErrorCount/State->BitCount;
    State->PER = (double)State->PacketErrorCount/State->PacketCount;

    #if 0 // Diagnostic messages...comment out by default
    wiPrintf(" wiTest TX MSG:"); for (k=0; k<n; k++) wiPrintf(" %02X",a[k].word); wiPrintf("\n");
    wiPrintf(" wiTest RX MSG:"); for (k=0; k<n; k++) wiPrintf(" %02X",b[k].word); wiPrintf("\n");
    #endif

    if (State->TraceKeyOp) wiPrintf(" wiTest: PacketError=%d, PER=%1.2f%% %s\n",num_errors? 1:0, State->PER*100,num_errors? "******":"");
    
    //if (num_errors) wiPrintf("*\n"); // Diagnostic messages...comment out by default

    #if 0 // diagnostic code...comment out by default
    if (num_errors) 
    {
        wiPrintf("Packet Bytes Errors = %d, Bit Errors = %d, PER=%5.1f%% (%d/%d)\n",State->NumberOfByteErrors,num_errors,100.0*State->PER,State->PacketErrorCount,State->PacketCount);
        for (k=0; k<n; k++)
            wiPrintf("%c",(a[k].word==b[k].word? '.':'X'));
        wiPrintf("\n");
        //exit(1);
    }
    #endif
    return WI_SUCCESS;
}
// end of wiTest_ErrorCounter()

// ================================================================================================
// FUNCTION  : wiTest_RandomMessage()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate a random message of the specified length
// Parameters: n   -- Length (in bytes) of the message buffer
//             buf -- Buffer into which the random message is placed
// ================================================================================================
wiStatus wiTest_RandomMessage(int n, wiUWORD buf[])
{
    wiReal x[WITEST_MAX_LENGTH+1];
    int i;

    if (InvalidRange(n, 1, WITEST_MAX_LENGTH)) return WI_ERROR_PARAMETER1;

    wiRnd_Uniform(n, x);

    for (i = 0; i < n; i++)
        buf[i].word = (unsigned int)(256.0 * x[i]);

    return WI_SUCCESS;
}
// end of wiTest_RandomMessage()

// ================================================================================================
// FUNCTION  : wiTest_Annex_G_Message()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate the 100-byte message as defined in Annex G of 802.11n-D1.10
// Parameters: buf -- Buffer into which the message is placed
// ================================================================================================
wiStatus wiTest_Annex_G_Message(int n, wiUWORD buf[])
{
    const wiByte msg[] = { 0x04, 0x02, 0x00, 0x2E, 0x00, 0x60, 0x08, 0xCD, 0x37, 0xA6,
                           0x00, 0x20, 0xD6, 0x01, 0x3C, 0xF1, 0x00, 0x60, 0x08, 0xAD,
                           0x3B, 0xAF, 0x00, 0x00, 0x4A, 0x6F, 0x79, 0x2C, 0x20, 0x62,
                           0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x73, 0x70, 0x61, 0x72,
                           0x6B, 0x20, 0x6F, 0x66, 0x20, 0x64, 0x69, 0x76, 0x69, 0x6E,
                           0x69, 0x74, 0x79, 0x2C, 0x0A, 0x44, 0x61, 0x75, 0x67, 0x68,
                           0x74, 0x65, 0x72, 0x20, 0x6F, 0x66, 0x20, 0x45, 0x6C, 0x79,
                           0x73, 0x69, 0x75, 0x6D, 0x2C, 0x0A, 0x46, 0x69, 0x72, 0x65,
                           0x2D, 0x69, 0x6E, 0x73, 0x69, 0x72, 0x65, 0x64, 0x20, 0x77,
                           0x65, 0x20, 0x74, 0x72, 0x65, 0x61, 0x67, 0x33, 0x21, 0xB6 };
    int i;

    if (n != 100) return STATUS(WI_ERROR_PARAMETER1);
    for (i = 0; i < 100; i++) buf[i].word = msg[i];

    return WI_SUCCESS;
}
// end of Annex_G_Message()

// ================================================================================================
// FUNCTION  : wiTest_SingleByteMessage()
// ------------------------------------------------------------------------------------------------
// Purpose   : Generate a PSDU using a constant byte pattern
// Parameters: n   -- Length (in bytes) of the message
//             buf -- Buffer into which the message is placed
//             pat -- Data pattern
// ================================================================================================
wiStatus wiTest_SingleByteMessage(int n, wiUWORD buf[], unsigned pat)
{
    int i;
    for (i = 0; i < n; i++) buf[i].word = pat;
    return WI_SUCCESS;
}
// end of wiTest_SingleByteMessage()

// ================================================================================================
// FUNCTION  : wiTest_FileMessage
// ------------------------------------------------------------------------------------------------
// Purpose   : Load the data buffer from a text file
// Parameters: filename -- name of the file containing the data
// ================================================================================================
wiStatus wiTest_FileMessage(char filename[])
{
    int i, N_MAC_TX_D;
    char buf[256];

    wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

    FILE *F = wiParse_OpenFile(filename, "rt");
    if (!F) return STATUS(WI_ERROR_FILE_OPEN);

    // ----------------------
    // Read the Bermai header
    // ----------------------
    for (i = 0; i < 8; i++)
    {
        if (feof(F)) {
            fclose(F); 
            return STATUS(WI_ERROR_FILE_READ);
        }
        fgets(buf, 255, F);
        pTxData[i].word = strtoul(buf, NULL, 0);
    }
    // --------------------------
    // Compute the message Length
    // --------------------------
    N_MAC_TX_D = (((pTxData[2].w8 & 0xF) << 8) | pTxData[3].w8) + 8;

    // -------------------
    // Read the PSDU bytes
    // -------------------
    for ( ; i < N_MAC_TX_D; i++)
    {
        if (feof(F)) {
            fclose(F); 
            return STATUS(WI_ERROR_FILE_READ);
        }
        fgets(buf, 255, F);
        pTxData[i].word = strtol(buf, NULL, 0);
    }
    fclose(F);
    return WI_SUCCESS;
}
// end of wiTest_FileMessage()

// ================================================================================================
// FUNCTION  : wiTest_SetTxData()
// ------------------------------------------------------------------------------------------------
// Purpose   : Load an arbitrary byte pattern into the message buffer
// Parameters: NumBytes    -- Message length
//             pDataBuffer -- Message array
// ================================================================================================
wiStatus wiTest_SetTxData(wiInt NumBytes, wiUWORD *pDataBuffer)
{
    int i;
    wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

    if (InvalidRange(NumBytes, 1, WITEST_MAX_LENGTH)) return WI_ERROR_PARAMETER1;
    if (!pDataBuffer) return WI_ERROR_PARAMETER2;

    for (i = 0; i < NumBytes; i++)
        pTxData[i] = pDataBuffer[i];

    State->DataType = WITEST_DATATYPE_BUFFER;
    State->Length   = NumBytes;
    return WI_SUCCESS;
}
// end of wiTest_SetTxData()

// ================================================================================================
// FUNCTION  : wiTest_GetTxData()
// ------------------------------------------------------------------------------------------------
// Purpose   : Get a message with the specified type and length
// Parameters: DataType    -- Packet type
//             NumBytes    -- Message length
//             pDataBuffer -- Buffer into which the message is placed
// ================================================================================================
wiStatus wiTest_GetTxData(wiInt DataType, wiInt NumBytes, wiUWORD *pDataBuffer)
{
    if (!pDataBuffer) return WI_ERROR_PARAMETER3;

    switch (DataType)
    {
        // Generic Data
        //
        case  WITEST_DATATYPE_RANDOM    : XSTATUS( wiTest_RandomMessage(NumBytes, pDataBuffer) ); break;
        case  WITEST_DATATYPE_ANNEX_G   : XSTATUS( wiTest_Annex_G_Message(NumBytes, pDataBuffer) ); break;
        case  WITEST_DATATYPE_FIXED_BYTE: XSTATUS( wiTest_SingleByteMessage(NumBytes, pDataBuffer, State->FixedByte) ); break;
        case  WITEST_DATATYPE_BUFFER    : break; // use data already loaded into buffer
        //
        // 802.11 Data Frame
        //
        case WITEST_DATATYPE_802_11_DATA_MOD256           : XSTATUS( wiMAC_DataFrame(NumBytes, pDataBuffer, 0, 0) ); break;
        case WITEST_DATATYPE_802_11_DATA_RANDOM           : XSTATUS( wiMAC_DataFrame(NumBytes, pDataBuffer, 1, 0) ); break;
        case WITEST_DATATYPE_802_11_DATA_MOD256_BROADCAST : XSTATUS( wiMAC_DataFrame(NumBytes, pDataBuffer, 0, 1) ); break;
        case WITEST_DATATYPE_802_11_DATA_PRBS             : XSTATUS( wiMAC_DataFrame(NumBytes, pDataBuffer, 2, 0) ); break;
        case WITEST_DATATYPE_802_11_DATA_BUFFER           : XSTATUS( wiMAC_DataFrame(NumBytes, pDataBuffer, 3, 0) ); break;
        //
		// 802.11n Aggregated MPDU
        //
		case WITEST_DATATYPE_802_11_AMPDU_MOD256          : XSTATUS( wiMAC_AMPDUDataFrame(NumBytes, pDataBuffer, 0, 0) ); break;
        case WITEST_DATATYPE_802_11_AMPDU_RANDOM          : XSTATUS( wiMAC_AMPDUDataFrame(NumBytes, pDataBuffer, 1, 0) ); break;
        case WITEST_DATATYPE_802_11_AMPDU_MOD256_BROADCAST: XSTATUS( wiMAC_AMPDUDataFrame(NumBytes, pDataBuffer, 0, 1) ); break;
        //
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    return WI_SUCCESS;
}
// end of wiTest_GetTxData()

// ================================================================================================
// FUNCTION  : wiTest_TxPacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit a single packet
// Parameters: Mode     -- {0=SNR, 1=PrdBm}
//             PrdBm    -- Receive power [dBm] at the antenna or SNR[dB]
//             bps      -- PHY transmission rate (bits/second)
//             nBytes   -- PHY transmission length (bytes)
//             DataType -- Packet type
//             pNr      -- Receive length (output if 0 input; otherwise taken as input)
// ================================================================================================
wiStatus wiTest_TxPacket( wiInt Mode, wiReal PrdBm, wiReal bps, wiInt nBytes, wiInt DataType, 
                          int *pNr, wiComplex ***pR, wiReal *pFs )
{
    const int Npad = 100;
    wiComplex **S, **R;
    wiReal Prms, Fs, BW = 20.0e6;
    int Ns = 0, Nr;

    wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

    if (State->TraceKeyOp) 
    {
        switch (Mode) 
        {
            case 0: wiPrintf("\n wiTest_TxPacket: SNR=%1.1f dB, Rate=%g Mbps, Length=%d bytes\n",PrdBm,bps/1.0e6,nBytes);
            case 1: wiPrintf("\n wiTest_TxPacket: Pr=%1.1f dBm, Rate=%g Mbps, Length=%d bytes\n",PrdBm,bps/1.0e6,nBytes);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate the Transmit Signal
    //
    //  A payload is generated according to the specified DataType. A packet waveform is
    //  generated using the PHY TX model.
    //
    //  The transmit bandwidth is retrieved to be used for calculations when Mode=0(SNR).
    //
    //  Npad samples are added to the transmit waveform length to account for the FIR
    //  channel response. The PHY model often includes padding bytes at the end, in
    //  which case this is unnecessary but harmless.
    //
    XSTATUS( wiTest_GetTxData( DataType, nBytes, pTxData ) );
    XSTATUS( wiPHY_TX(bps, nBytes, pTxData, &Ns, &S, &Fs, &Prms) );
    XSTATUS( wiPHY_GetData(WIPHY_TX_BANDWIDTH_HZ, &BW) );
    Nr = Ns + Npad;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate the Receive Waveform
    //
    //  The channel model produces a receive waveform scaled with an amplitude that
    //  yields Pr[dBm] if applied to a 1-Ohm resistance. When the SNR model is used,
    //  Gaussian noise is added.
    //
    switch (Mode)
    {
        case 0: // by SNR[dB]
        {
            wiReal SNR_dB = PrdBm;
            wiReal SNR    = pow(10.0, SNR_dB/10.0);
            wiReal Nrms   = sqrt((Prms/SNR)*(Fs/BW));
            XSTATUS( wiChanl_Model (Ns, S, Nr, &R, Fs, State->PrdBmForSNR, Prms, Nrms) ); 
            break;
        }
        case 1: // by Pr[dBm]
        {
            XSTATUS( wiChanl_Model (Ns, S, Nr, &R, Fs, PrdBm, Prms, 0.0) ); 
            break;
        }
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }

	State->LastPrdBm = PrdBm; // record the receive power 

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Return results by reference
    //
    if (pNr != NULL) *pNr = Nr;
    if (pR  != NULL) *pR  = R;
    if (pFs != NULL) *pFs = Fs;

    return WI_SUCCESS;
}
// end of wiTest_TxPacket()

// ================================================================================================
// FUNCTION  : wiTest_TxRxPacketCore()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit/Receive a single waveform
// Parameters: Mode    -- {0:SNR, 1:PrdBm}
//             PrdBm   -- Receive power [dBm] at the antenna or SNR[dB]
//             bps     -- PHY transmission rate (bits/second)
//             nBytes  -- PHY transmission length (bytes)
// ================================================================================================
wiStatus wiTest_TxRxPacketCore(wiTest_TxRxPacket_t *pTxRxData)
{
    wiComplex **R; int Nr;
    wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  wiRnd Diagnostic Seed
    //
    //  Override the random number generator seed with a value related to the packet 
    //  transmission number. This allows generation of the same results regardless of
    //  whether multithreading is used.
    //
    if (State->RndSeed == WITEST_RNDSEED_ZERO)
        XSTATUS( wiRnd_InitThread(wiProcess_ThreadIndex(), 0, 0, 0) );

    if (State->RndSeed == WITEST_RNDSEED_PACKETNUMBER)
    {
        int ThreadIndex = wiProcess_ThreadIndex();
        int PacketNumber = State->TotalPacketCount + max(0, (int)ThreadIndex - 1);
        XSTATUS( wiRnd_InitThread(ThreadIndex, PacketNumber % WIRND_MAX_SEED, 0, PacketNumber/WIRND_MAX_SEED) );
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  MultipleTxPackets
    //
    //  Transmit background packets that do not include the priminary packet used for
    //  error rate testing. For this to be useful, the channel model must be configured
    //  to use wiChanl.InterferenceModel = 5 (WICHANL_INTERFERENCE_MULTIPLE_PACKETS).
    //  This causes the background packets to be overlapped in generation of the receive
    //  waveform and can be used for both interference packets and to generate waveforms
    //  with multiple receive packets.
    //
    if (State->MultipleTxPackets)
    {
        unsigned int i; int Ns;
        wiReal Prms, Fs;
        wiComplex **S;

        for (i = 1; i <= State->NumberOfBackgroundPackets; i++)
        {
            wiTestBackgroundPacket_t *pPacket = State->BackgroundPacket + i;

            XSTATUS( wiTest_GetTxData( pPacket->DataType, pPacket->Length, pTxData) );
            XSTATUS( wiPHY_TX        ( pPacket->DataRate, pPacket->Length, pTxData, &Ns, &S, &Fs, &Prms) );
            XSTATUS( wiChanl_LoadPacket( Ns, S, Fs, Prms, i, pPacket->PrdBm, pPacket->FcMHz, pPacket->Delay ) );
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Transmit + Receive Packet
    //
    XSTATUS( wiTest_TxPacket(pTxRxData->Mode, pTxRxData->PrdBm, pTxRxData->bps, pTxRxData->nBytes, 
                             State->DataType, &Nr, &R, NULL) );
    XSTATUS( wiPHY_RX(NULL, &(State->RxData[wiProcess_ThreadIndex()]), Nr, R) );

    return WI_SUCCESS;
}
// end of wiTest_TxRxPacketCore()

// ================================================================================================
// FUNCTION  : wiTest_TxRxPacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Transmit/Receive and count errors for a single packet
// Parameters: Mode    -- {0:SNR, 1:PrdBm}
//             PrdBm   -- Receive power [dBm] at the antenna or SNR[dB]
//             bps     -- PHY transmission rate (bits/second)
//             nBytes  -- PHY transmission length (bytes)
// ================================================================================================
wiStatus wiTest_TxRxPacket(wiInt Mode, wiReal PrdBm, wiReal bps, wiInt nBytes)
{
    wiTest_TxRxPacket_t TxRxData[WISE_MAX_THREADS];
    int NumberOfThreads   = State->NumberOfThreads ? State->NumberOfThreads : 1;
    int SingleThreadLoops = State->UseSingleTxRxThread ? NumberOfThreads : 1;
    int ErrorCounterLoops = State->UseSingleTxRxThread ? 1 : NumberOfThreads;
    int i, n, ofs;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Copy data packet parameters to the TxRxData structure. This allows the same
    //  data structure to be used for both threaded and non-threaded operation
    //
    for (i = 0; i < NumberOfThreads; i++)
    {
        TxRxData[i].Mode        = Mode;
        TxRxData[i].PrdBm       = PrdBm;
        TxRxData[i].bps         = bps;
        TxRxData[i].nBytes      = nBytes;
    }
    for (n = 0; n < SingleThreadLoops; n++)
    {
        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Transmit a packet, generate the waveform, and process with the receiver.
        //  If more than 1 thread is specified, the multithread dispatch function is used.
        //  Because the non-threaded case uses ThreadIndex = 0, the ofs parameter is provide
        //  so the error counter can reference the correct data.
        //
    #ifdef WISE_BUILD_MULTITHREADED

        if (NumberOfThreads > 1 && !State->UseSingleTxRxThread)
        {
            for (i = 0; i < NumberOfThreads; i++) {
                if (!State->TxData[i+1]) WICALLOC( State->TxData[i+1], WITEST_MAX_LENGTH, wiUWORD );
                XSTATUS( wiProcess_CreateThread( (wiThreadFn_t)wiTest_TxRxPacketCore, TxRxData + i, NULL) );
            }
            XSTATUS( wiProcess_WaitForAllThreadsAndClose() );
            ofs = 1;
        }
        else

    #endif
        {
            XSTATUS( wiTest_TxRxPacketCore(TxRxData) )
            ofs = 0;
        }

        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Update Statistics/Counters
        //
        for (i = 0; i < ErrorCounterLoops; i++)
        {
            XSTATUS( wiTest_ErrorCounter(nBytes, State->TxData[i+ofs], State->RxData[i+ofs]) );
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Write Status File
    //
    //  This is a small file used to record the current state of a PER simulation. It
    //  is generally useful for long simulations to monitor progress.
    //
    if ((State->PacketCount % State->UpdateRate) == 0)
    {
        FILE *F = fopen("wiseStatus.txt", "wt");
        if (F)
        {
            fprintf(F," Pr  = %4.1f dBm\n" ,PrdBm);
            fprintf(F," BER = %d / %d = %8.2e\n", State->BitErrorCount, State->BitCount, State->BER);
            fprintf(F," PER = %d / %d = %8.2e\n", State->PacketErrorCount, State->PacketCount, State->PER);
            fclose(F);
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Update Minimum Packet Count
    //
    //  When the minimum failure count is reached, the MinPacketCount parameter is set
    //  to a value corresponding to one more error at the current PER. The purpose is
    //  to avoid a bias in the result if the simulation is always halted on a bad packet.
    //
    if (State->UpdateMinPacketCount)
    {
        if ((State->PacketErrorCount == State->MinFail) && !State->MinPacketCount)
            State->MinPacketCount = (State->PacketCount * (State->PacketErrorCount + 1) / State->PacketErrorCount) + 1;
    }

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Process the Callback Function
    //
    //  This can be use for process at the end of a TX/RX packet. For example, in wiseMex
    //  it can call a function that extends to the MATLAB workspace to determine if
    //  execution should terminate.
    //
    if (State->TxRxCallback != NULL)
    {
        XSTATUS( State->TxRxCallback() );
    }

    return WI_SUCCESS;
}
// end of wiTest_TxRxPacket()

// ================================================================================================
// FUNCTION  : wiTest_WriteAWG_TxPacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write an AWG5x0 and ESG format file with one packet from the channel TX
//             input (radio output)
// Parameters: none
// ================================================================================================
wiStatus wiTest_WriteAWG_TxPacket(void)
{
    FILE *FI, *FQ, *F;
    char filename[256];
    unsigned short X;
    signed   short xI, xQ; char *cI, *cQ;
    int m,n,k; unsigned i;
    double A, x;
    int N;
    wiComplex **S;
    wiReal Prms, Ts;

    wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];
    
    XSTATUS( wiTest_StartTest("wiTest_WriteAWG_TxPacket()") );

    // ----------------
    // Get the raw data
    // ----------------
    switch (State->DataType)
    {
        case  0: XSTATUS( wiTest_RandomMessage(State->Length, pTxData) ); break;
        case  1: XSTATUS( wiTest_Annex_G_Message(State->Length, pTxData) ); break;
        case  2: XSTATUS( wiTest_SingleByteMessage(State->Length, pTxData, State->FixedByte) ); break;
        case  3: break;
        case  4: XSTATUS( wiMAC_DataFrame(State->Length, pTxData, 0, 0) ); break;
        case  5: XSTATUS( wiMAC_DataFrame(State->Length, pTxData, 1, 0) ); break;
        case  6: XSTATUS( wiMAC_DataFrame(State->Length, pTxData, 0, 1) ); break;
        case  7: XSTATUS( wiMAC_DataFrame(State->Length, pTxData, 2, 0) ); break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // -----------
    // Transmitter
    // -----------
    XSTATUS( wiPHY_TX(State->DataRate, State->Length, pTxData, &N, &S, &Ts, &Prms) );

    N = (N>>3)<<3; // round down to the nearest multiple of 8

    // --------------------------
    // Create AWG5x0 format files
    // --------------------------
    A = 511.0 / sqrt(8.0 * Prms);  // 4 sigma headroom
    n = 2*(N + 80); m = (int)ceil(log((double)n)/log(10.0));

    sprintf(filename,"Tx%d_%d_I.wfm",(int)(State->DataRate/1e6),State->Length);
    FI = fopen(filename,"wb"); 
    if (!FI) return STATUS(WI_ERROR_FILE_OPEN);

    sprintf(filename,"Tx%d_%d_Q.wfm",(int)(State->DataRate/1e6),State->Length);
    FQ = fopen(filename,"wb"); 
    if (!FQ) return STATUS(WI_ERROR_FILE_OPEN);

    fprintf(FI,"MAGIC 2000\n#%d%d",m,n);
    fprintf(FQ,"MAGIC 2000\n#%d%d",m,n);

    for (k = 0; k < 80; k++)       // Toggle Marker 1 high for 80 clock periods (1us)
    {                               
        fprintf(FI,"%c%c",0,0x22);
        fprintf(FQ,"%c%c",0,0x22);
    }
    for (k = 0; k < N; k++)        // Transmit data
    {
        x = A*S[0][k].Real; x = x<-512 ? -512:x;
        X = (unsigned short)(floor(x + 0.5) + 512);
        X = (unsigned short)(X>1023 ? 1023:X);
        fprintf(FI,"%c%c",X&0xFF, (X>>8)&0xFF);

        x = A*S[0][k].Imag; x = x<-512? -512:x; 
        X = (unsigned short)(floor(x + 0.5) + 512);
        X = (unsigned short)(X>1023 ? 1023:X);
        fprintf(FQ,"%c%c",X&0xFF, (X>>8)&0xFF);
    }
    fprintf(FI,"CLOCK 8.000000E+07\n");
    fprintf(FQ,"CLOCK 8.000000E+07\n");

    fclose(FI); fclose(FQ);

    // --------------------------------
    // Create Agilengt ESG format files
    // --------------------------------
    A = 32767.0 / sqrt(10.0 * Prms);  // 4.5 sigma headroom
    sprintf(filename,"Tx%d_%d.ESG",(int)(State->DataRate/1e6),State->Length);
    FI = fopen(filename,"wb"); 
    if (!FI) return STATUS(WI_ERROR_FILE_OPEN);
    
    cI = (char *)(&xI);   cQ = (char *)(&xQ);

    for (k = 0; k < 2*(N/2); k++)    // Transmit data
    {
        x = A*S[0][k].Real;  x = x<-32767? -32768:(x>32768? 32767:x);  xI = (unsigned short)floor(x+0.5);
        x = A*S[0][k].Imag;  x = x<-32767? -32768:(x>32768? 32767:x);  xQ = (unsigned short)floor(x+0.5);
        fprintf(FI,"%c%c%c%c",cI[1],cI[0], cQ[1],cQ[0]);
    }
    fclose(FI);

    // -----------------------
    // Save the tx_data stream
    // -----------------------
    F = fopen("tx_data.out","wt");
    for (i = 0; i < State->Length; i++)
        fprintf(F,"%u\n",pTxData[i].w8);
    fclose(F);

    // ------------------------------------
    // Save the transmit data in ASCII form
    // ------------------------------------
    sprintf(filename,"Tx%d_%d.out",(int)(State->DataRate/1e6),State->Length);
    F = fopen(filename,"wt");
    for (k = 0; k < N; k++) fprintf(F,"%9.6f %9.6f\n",S[0][k].Real,S[0][k].Imag);
    fclose(F);
        
    XSTATUS( wiTest_EndTest() );
    return WI_SUCCESS;
}
// end of wiTest_WriteAWG_TxPacket()

// ================================================================================================
// FUNCTION  : wiTest_ReopenOUTF()
// ------------------------------------------------------------------------------------------------
// Purpose   : Open the test output file
// Parameters: none
// ================================================================================================
wiStatus wiTest_ReopenOUTF(void)
{
    int attemptsRemaining = 100;
    char filename[384];

    if (State->OutputFile) fclose(State->OutputFile);
    State->OutputFile = NULL;
    
    sprintf(filename,"wiTest.%s",State->FilenameExtension);

    //  Open the output file
    //  The function will retry up to "attemptsRemaining" times to open the file if there
    //  is a failure. This mechanism was added to work around network issues that caused
    //  momentary loss of connect to the file server.
    //
    while (!State->OutputFile && attemptsRemaining)
    {
        State->OutputFile = fopen(filename,"at");
        if (!State->OutputFile)
        {
            time_t t1, t2;  
            int dt = 1;

            attemptsRemaining--;
            dt = min(300, dt+1); // increase time delay up to 5 minutes
            time(&t1); time(&t2);
            while (difftime(t2, t1) < dt) time(&t2); // delay dt [seconds]
        }
    }
    return State->OutputFile ? WI_SUCCESS : STATUS(WI_ERROR_FILE_OPEN);
}
// end of wiTest_ReopenOUTF()

// ================================================================================================
// FUNCTION  : wiTest_Initialize_wiRnd()
// ------------------------------------------------------------------------------------------------
// Purpose   : Initialize (seed) the wiRnd generator
// Parameters: none
// ================================================================================================
wiStatus wiTest_Initialize_wiRnd(void)
{
    if (State->RndSeed < WITEST_RNDSEED_RANDOM)
    {
        switch (State->RndSeed)
        {
            case WITEST_RNDSEED_RATELENGTH:
            {
                int seed = (int)(State->Length + State->DataRate) % 942377568;
                wiRnd_Init(seed, State->RndStep[1], State->RndStep[0]);
                break;
            }
            case WITEST_RNDSEED_PACKETNUMBER:
            case WITEST_RNDSEED_ZERO:
                wiRnd_Init(0, 0, 0);
                break;

            default: return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    else
    {
        wiRnd_Init(State->RndSeed, State->RndStep[1], State->RndStep[0]);
    }
    wiRnd_GetState( (int *)(State->RndInitState), State->RndInitState + 1, State->RndInitState + 2);

    return WI_SUCCESS;
}
// end of wiTest_Initialize_wiRnd()


// ================================================================================================
// FUNCTION  : wiTest_WriteConfig()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write configuration information via the supplied metod function
// Parameters: MsgFunc -- Message handling function (pointer)
// ================================================================================================
wiStatus wiTest_WriteConfig(wiMessageFn MsgFunc)
{
    const char *DataTypeS[]={ "Random","Fixed: 802.11a-1999 Annex G","Fixed Byte: 0x","<from memory>",
                              "MAC Data (Mod-256)","MAC Data (Random)","Broadcast Data(Mod-256)",
                              "MAC Data (PRBS)","MAC Data (Buffer)",
                              "<9>","<10>","<11>","<12>","<13>",
                              "A-MPDU (Mod-256)","A-MPDU (Random)","A-MPDU (Mod-256), Broadcast"};
    time_t t; time(&t);
    
    MsgFunc(" \n");
    MsgFunc(" %-72s DSP GROUP CONFIDENTIAL\n",WISE_VERSION_STRING);
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Simulation started: %s",ctime(&t));
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Data Rate             = %1.1f Mb/s\n",State->DataRate/1.0e6);
    MsgFunc(" Data Length           = %d Bytes/Packet\n",State->Length);
    MsgFunc(" Data Type             = %s",DataTypeS[State->DataType]); 
    switch (State->DataType)
    {
        case WITEST_DATATYPE_FIXED_BYTE: MsgFunc("%02X\n", State->FixedByte); break;
        default:                         MsgFunc("\n");
    }
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");

    XSTATUS( wiChanl_WriteConfig(MsgFunc) );

    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" PHY Layer Method      = %s\n",wiPHY_ActiveMethodName());
    XSTATUS( wiPHY_WriteConfig(MsgFunc) );
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    {
        #ifdef WIN32
            char *hostname = getenv("COMPUTERNAME");
            MsgFunc(" Runtime Location      = (%s)\n",hostname);
        #else
            char *hostname = getenv("HOSTNAME");
            char *pwd = getenv("PWD");
            MsgFunc(" Runtime Location      = (%s)%s\n",hostname,pwd);
        #endif
    }
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" wiRnd Seed Value      = %d; %d,%d\n",State->RndInitState[0],State->RndInitState[1],State->RndInitState[2]);
    MsgFunc(" Number of Threads     = %d\n",max(1, State->NumberOfThreads));
    MsgFunc(" Min Errors/Max Packets= %d/%d\n",State->MinFail,State->MaxPackets);
    MsgFunc(" Min PER               = %8.1e\n",State->MinPER);

    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    
    if (State->Comment[0]) 
    {
        MsgFunc(" %s\n",State->Comment);
        MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    }
    if (State->MultipleTxPackets)
    {
        unsigned int i;

        MsgFunc("\n");
        MsgFunc(" wiTest_OnePacketTxMultiple\n\n");
        MsgFunc("    Packet  DataType  Rate[Mbps]  Length  Pr[dBm]   f[MHz]   t0 (seconds)\n");
        MsgFunc("    ------  --------  ----------  ------  -------  --------  ------------\n");
        MsgFunc("       0*    %4d       %5.1f     %5d   %6.1f   %6.1f     %11.3e\n",
                 State->DataType, State->DataRate/1.0e6, State->Length, State->MinPrdBm, 0.0, 0.0);

        for (i = 1; i<=State->NumberOfBackgroundPackets; i++)
        {
            wiTestBackgroundPacket_t *pPacket = State->BackgroundPacket + i;
  
            MsgFunc( "    %4d     %4d       %5.1f     %5d   %6.1f   %6.1f     %11.3e\n",
                      i, pPacket->DataType, pPacket->DataRate/1.0e6, pPacket->Length, 
                      pPacket->PrdBm, pPacket->FcMHz, pPacket->Delay );
        }
        MsgFunc("\n");
    }
    return WI_SUCCESS;
}
// end of wiTest_WriteConfig()

// ================================================================================================
// FUNCTION  : wiTest_StartTest()
// ------------------------------------------------------------------------------------------------
// Purpose   : Setup for the start of a test
// Parameters: none
// ================================================================================================
wiStatus wiTest_StartTest(wiString TestFnName)
{
    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;
    State->TestIsDone = 0;

    State->TestStartClock = clock();
    time( &(State->TestStartTime) );

    wiPrintf(" %s...\n\n",TestFnName);

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Get PHY-Specific Parameters
    //
    //  wiTest parameters may be overriden for specific PHY models as appropriate.
    //  DataRate may be overriden which accomodates the requirement for a generic 802.11n
    //  implementation where multiple signal formats may map to a single data rate.
    //
    XSTATUS( wiPHY_GetData(WIPHY_TX_DATARATE_BPS, &(State->DataRate)) );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Set PHY-Specific Parameters
    //
    XSTATUS( wiPHY_SetData(WIPHY_RX_NUM_PATHS, wiChanl_State()->NumRx) );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Override the Length parameter when using the Annex G message
    //
    if (State->DataType == WITEST_DATATYPE_ANNEX_G)
        State->Length = 100;        
    
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Initialize the random number generator
    //
    XSTATUS( wiTest_Initialize_wiRnd() );

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Generate Output File Header
    //
    if (!State->DisableOutputFile)
    {
        char filename[384];
        FILE *F;                        
        
        sprintf(filename,"wiTest.%s",State->FilenameExtension);
        F = State->OutputFile = fopen(filename,"wt");
        if (!State->OutputFile) return WI_ERROR_FILE_OPEN;

        wiTest_WriteConfig(wiTest_Output);
        fflush(State->OutputFile);
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Clear counters and record the starting process time
    //
    XSTATUS( wiTest_ClearCounters() );
    if (State->ClearTotalsOnStartTest)
    {
        State->TotalPacketCount = 0;
        State->TotalPacketErrorCount = 0;
    }
    State->CountByPoint.NumberOfPoints = 0;
    State->ProcessTime0 = wiProcess_Time();

    XSTATUS( wiPHY_StartTest(max(1, State->NumberOfThreads)) ); //  Call PHY-specific start-of-test code

    return WI_SUCCESS;
}
// end of wiTest_StartTest()

// ================================================================================================
// FUNCTION  : wiTest_EndTest()
// ------------------------------------------------------------------------------------------------
// Purpose   : Clean up at the end of a test
// Parameters: none
// ================================================================================================
wiStatus wiTest_EndTest(void)
{
    time_t tEnd; time(&tEnd);

    if (State->PrintProcessTime)
    {
        clock_t t1 = State->TestStartClock;
        clock_t t2 = clock();

        double   TotalTime   = difftime(tEnd, State->TestStartTime);
        double   tCPU        = wiProcess_Time();
        double   ProcessTime = (double)(t2 - t1)/CLOCKS_PER_SEC;
        unsigned bps         = (unsigned int)((double)State->BitCount / TotalTime);
        double   pps         = (double)State->PacketCount / TotalTime;

        wiPrintf("\n");
        wiPrintf(" Total Time  = %4.1f seconds\n",TotalTime);
        wiPrintf(" Process Time= %4.1f seconds\n",ProcessTime);
        wiPrintf(" CPU Time    = %4.2f seconds\n",tCPU);
        wiPrintf(" Throughput  = %s bps\n",wiUtil_Comma(bps));
        wiPrintf(" Packet Rate = %4.1f pps\n",pps);

        wiTestWr("\n");
        wiTestWr(" Total Time  = %4.1f seconds\n",TotalTime);
        wiTestWr(" Process Time= %4.1f seconds\n",ProcessTime);
        wiTestWr(" CPU Time    =% 7.2f seconds\n",tCPU);
        wiTestWr(" Throughput  = %s bps\n",wiUtil_Comma(bps));
        wiTestWr(" Packet Rate = %4.1f pps\n",pps);
        wiTestWr("\n");
    }
    if (!State->DisableOutputFile)
    {    
        XSTATUS( wiTest_ReopenOUTF() );
        fprintf(State->OutputFile," \n");

        XSTATUS( wiPHY_EndTest(wiTest_Output) );

        if (!State->PrintProcessTime)
        {
            double t = difftime(tEnd, State->TestStartTime);
            if (t != 0.0)
            {
                char buf[128];

                if      (t > 300) sprintf(buf, " Run Time = %s seconds\n", wiUtil_Comma((int)(t+0.5)));
                else if (t >  60) sprintf(buf, " Run Time = %1.1f seconds\n", t);
                else              sprintf(buf, " Run Time = %1.2f seconds\n", t);
                fprintf(State->OutputFile, "%s", buf);
            }
        }
        fprintf(State->OutputFile," -----------------------------------------------------------------------------------------------\n");
        fclose(State->OutputFile); State->OutputFile = NULL;
    }
    else
    {
        XSTATUS( wiPHY_EndTest(wiPrintf) ); // print to console if output file is disabled
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Pause Script Parsing
    //
    //  The mechanism below is included to support digital design verification. When the
    //  condition is true, the script parser is instructed to pause which occurs after
    //  execution of the current instruction (test).
    //
    if (State->PauseAfterTest) {
        STATUS( wiParse_PauseScript() ); // no XSTATUS...continue after error
    }        

    XSTATUS( wiTest_FreeThreadData() );

    State->TestCount++;
    State->TestIsDone = 1;      
    return WI_SUCCESS;
}
// end of wiTest_EndTest()

// ================================================================================================
// FUNCTION  : wiTest_State()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return a pointer to the internal state structure
// ================================================================================================
wiTest_State_t * wiTest_State(void)
{
    return State;
}
// end of wiTest_State()

// ================================================================================================
// FUNCTION  : wiTest_TestIsDone()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return the end-of-test indicator
// ================================================================================================
wiBoolean wiTest_TestIsDone(void)
{
    if (!State) 
        return WI_FALSE;
    else
        return State->TestIsDone;
}
// end of wiTest_TestIsDone()

// ================================================================================================
// FUNCTION  : wiTest_ClearTestIsDone()
// ------------------------------------------------------------------------------------------------
// Purpose   : Return the end-of-test indicator
// ================================================================================================
wiStatus wiTest_ClearTestIsDone(void)
{
    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    State->TestIsDone = WI_FALSE;
    return WI_SUCCESS;
}
// end of wiTest_ClearTestIsDone()

// ================================================================================================
// FUNCTION  : wiTestWr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Append to the output file and close
// Parameters: same as "printf"
// ================================================================================================
wiStatus wiTestWr(char *format, ...)
{
    if (!State->DisableOutputFile)
    {
        va_list ap;

        XSTATUS(wiTest_ReopenOUTF());

        va_start(ap,format);
        vfprintf(State->OutputFile, format, ap);
        va_end(ap);

        fclose(State->OutputFile); 
        State->OutputFile = NULL;
    }
    return WI_SUCCESS;
}
// end of wiTestWr()

// ================================================================================================
// FUNCTION  : wiTest_Output()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write to the output file
// Parameters: same as "printf"
// ================================================================================================
int wiTest_Output(const char *format, ...)
{
    if (!State->DisableOutputFile)
    {
        va_list ap;

        if (!State->OutputFile) STATUS(WI_ERROR_FILE_WRITE);

        va_start(ap,format);
        vfprintf(State->OutputFile, format, ap);
        va_end(ap);
    }
    return 0;
}
// end of wiTest_Output()

// ================================================================================================
// FUNCTION : wiTest_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiTest_ParseLine(wiString text)
{
    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  wiTest Parameters/Commands
    //
    if (strstr(text, "wiTest"))
    {
        wiStatus status, status2, pstatus;

        PSTATUS( wiParse_Int (text,"wiTest.RndSeed",   &(State->RndSeed),  WITEST_RNDSEED_ZERO, 900000000) );
        PSTATUS( wiParse_UInt(text,"wiTest.RndStep1",  &(State->RndStep[1]),0, 0x7FFFFFFF) );
        PSTATUS( wiParse_UInt(text,"wiTest.RndStep0",  &(State->RndStep[0]),0, 0x7FFFFFFF) );
        PSTATUS( wiParse_Int (text,"wiTest.DataType",  (int *)&(State->DataType ), 0, WITEST_MAX_DATATYPE) );
        PSTATUS( wiParse_UInt(text,"wiTest.FixedByte", &(State->FixedByte), 0, 255)        );

        PSTATUS( wiParse_Real(text,"wiTest.MinSNR",    &(State->MinSNR),  -10.0, 1000.0) );
        PSTATUS( wiParse_Real(text,"wiTest.MaxSNR",    &(State->MaxSNR),  -10.0, 1000.0) );
        PSTATUS( wiParse_Real(text,"wiTest.StepSNR",   &(State->StepSNR), -10.0,   10.0) );

        PSTATUS( wiParse_Real(text,"wiTest.MinPrdBm",  &(State->MinPrdBm), -120.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.PrdBm",     &(State->MinPrdBm), -120.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.MaxPrdBm",  &(State->MaxPrdBm), -120.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.StepPrdBm", &(State->StepPrdBm), -10.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.PrdBmForSNR",&(State->PrdBmForSNR), -120.0, 60.0) );

        PSTATUS( wiParse_UInt(text,"wiTest.MinFail",   &(State->MinFail),   1,   1000000) );
        PSTATUS( wiParse_Real(text,"wiTest.MinPER",    &(State->MinPER),   -1, 1.0) );

        PSTATUS( wiParse_UInt(text,"wiTest.Length",    &(State->Length),    1,   WITEST_MAX_LENGTH) );
        PSTATUS( wiParse_UInt(text,"wiTest.MaxPackets",&(State->MaxPackets),1,10000000) );

        PSTATUS( wiParse_UInt(text,"wiTest.UpdateRate",&(State->UpdateRate), 10, 1000) );
        PSTATUS( wiParse_UInt(text,"wiTest.MinPacketCount",&(State->MinPacketCount), 0, 10000));

        PSTATUS( wiParse_Boolean(text,"wiTest.PrintProcessTime", &(State->PrintProcessTime)));

        PSTATUS( wiParse_String (text,"wiTest.Comment", State->Comment, WITEST_MAX_COMMENT) );
        PSTATUS( wiParse_String (text,"wiTest.FilenameExtension", State->FilenameExtension, WITEST_MAX_FILENAME_EXTENSION) );
        PSTATUS( wiParse_Boolean(text,"wiTest.TraceKeyOp", &(State->TraceKeyOp)));

        PSTATUS( wiParse_Int (text,"wiTest.DataType[1]", (int *)&(State->BackgroundPacket[1].DataType), 0, WITEST_MAX_DATATYPE) );
        PSTATUS( wiParse_UInt(text,"wiTest.Length[1]",   &(State->BackgroundPacket[1].Length),   1, WITEST_MAX_LENGTH) );
        PSTATUS( wiParse_Real(text,"wiTest.Delay[1]",    &(State->BackgroundPacket[1].Delay), -0.1, 0.1) );
        PSTATUS( wiParse_Real(text,"wiTest.PrdBm[1]",    &(State->BackgroundPacket[1].PrdBm), -120, 30) );
        PSTATUS( wiParse_Real(text,"wiTest.FcMHz[1]",    &(State->BackgroundPacket[1].FcMHz), -100,100) );

        PSTATUS( wiParse_UInt(text,"wiTest.NumberOfBackgroundPackets",&(State->NumberOfBackgroundPackets), 0, WITEST_MAX_BACKGROUNDPACKETS) );
        PSTATUS( wiParse_Boolean(text,"wiTest.DisableOutputFile", &(State->DisableOutputFile)));
        PSTATUS( wiParse_Boolean(text,"wiTest.MultipleTxPackets", &(State->MultipleTxPackets)));
        PSTATUS( wiParse_Boolean(text,"wiTest.ClearTotalsOnStartTest",&(State->ClearTotalsOnStartTest)));
        PSTATUS( wiParse_Boolean(text,"wiTest.PauseAfterTest",&(State->PauseAfterTest)));
        PSTATUS( wiParse_Boolean(text,"wiTest.TestIsDone", &(State->TestIsDone)));
        PSTATUS( wiParse_Boolean(text,"wiTest.OnePacketUsesSNR", &(State->OnePacketUsesSNR)) );
        PSTATUS( wiParse_Boolean(text,"wiTest.UpdateMinPacketCount",&(State->UpdateMinPacketCount)) );
        PSTATUS( wiParse_Boolean(text,"wiTest.UseSingleTxRxThread", &(State->UseSingleTxRxThread)) );

        PSTATUS( wiParse_Int    (text,"wiTest.NumberOfThreads",&(State->NumberOfThreads), 0, max(1, WISE_MAX_THREADS-1)) );


        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Legacy Compatibility Labels
        //
        //  Alternate labels compatibile with WiSE 3.04 and earlier are provided here to
        //  allow existing script files to operate without modification.
        //
        PSTATUS( wiParse_UInt(text,"wiTest.fixedByte",  &(State->FixedByte), 0, 255)        );
        PSTATUS( wiParse_Real(text,"wiTest.minSNR",     &(State->MinSNR),  -10.0, 1000.0) );
        PSTATUS( wiParse_Real(text,"wiTest.maxSNR",     &(State->MaxSNR),  -10.0, 1000.0) );
        PSTATUS( wiParse_Real(text,"wiTest.dSNR",       &(State->StepSNR), -10.0,   10.0) );
        PSTATUS( wiParse_Real(text,"wiTest.stepSNR",    &(State->StepSNR), -10.0,   10.0) );
        PSTATUS( wiParse_Real(text,"wiTest.minPr_dBm",  &(State->MinPrdBm), -120.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.maxPr_dBm",  &(State->MaxPrdBm), -120.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.stepPr_dBm", &(State->StepPrdBm), -10.0, 60.0) );
        PSTATUS( wiParse_Real(text,"wiTest.SNR_PrdBm",  &(State->PrdBmForSNR), -120.0, 60.0) );
        PSTATUS( wiParse_UInt(text,"wiTest.minFail",    &(State->MinFail),   1,   10000) );
        PSTATUS( wiParse_Real(text,"wiTest.minPER",     &(State->MinPER),   -1, 1.0) );
        PSTATUS( wiParse_UInt(text,"wiTest.maxPackets", &(State->MaxPackets),1,10000000) );
        PSTATUS( wiParse_UInt(text,"wiTest.update_rate",&(State->UpdateRate), 10, 1000) );
        PSTATUS( wiParse_Int (text,"wiTest.DataType1",  (int *)&(State->BackgroundPacket[1].DataType), 0, WITEST_MAX_DATATYPE) );
        PSTATUS( wiParse_UInt(text,"wiTest.Length1",    &(State->BackgroundPacket[1].Length),   1, WITEST_MAX_LENGTH) );

        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  DataRate
        //
        //  Alternate forms to specify data rate are provided. DataRate is directly specified
        //  in [Mbps]. Alternate forms allow bps or kbps specification. DataRate1 sets the
        //  rate for the first background packet. This is provided for compatibility with
        //  Mojave test/verification scripts.
        //  
        {
            double d;

            PSTATUS( wiParse_Real(text,"wiTest.DataRate_bps",&(State->DataRate),0.0, 1.0E+9) );
            XSTATUS(status = wiParse_Real(text,"wiTest.DataRate", &d, 0.5, 1000.0));
            if (status == WI_SUCCESS)
            {
                State->DataRate = d * 1E+6;
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Real(text,"wiTest.DataRate[1]", &d, 0.5, 1000.0));
            if (status == WI_SUCCESS)
            {
                State->BackgroundPacket[1].DataRate = d * 1e6;
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Real(text,"wiTest.DataRate1", &d, 0.5, 1000.0));
            if (status == WI_SUCCESS)
            {
                State->BackgroundPacket[1].DataRate = d * 1e6;
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Real(text,"wiTest.DataRate_kbps", &d, 500, 1E+6));
            if (status == WI_SUCCESS)
            {
                State->DataRate = d * 1E+3;
                return WI_SUCCESS;
            }
        }
        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Background Packet Array
        //
        //
        if (strstr(text, "wiTest.BackgroundPacket") != NULL)
        {
            wiReal DataRate, PrdBm, FcMHz, Delay;
            wiInt Length;
            wiInt DataType, i;
            wiStatus status;

            XSTATUS( status = wiParse_Function( text,"wiTest.BackgroundPacket(%d, %d, %f, %d, %f, %f, %f)",
                                                &i, &DataType, &DataRate, &Length, &PrdBm, &FcMHz, &Delay ) );
            if (status == WI_SUCCESS)
            {
                if (InvalidRange(i, 0, WITEST_MAX_BACKGROUNDPACKETS)) return STATUS(WI_ERROR_PARAMETER1);
                if (InvalidRange(DataType,  0, WITEST_MAX_DATATYPE) ) return STATUS(WI_ERROR_PARAMETER2);
                if (InvalidRange(DataRate,  0.5, 1000.0))             return STATUS(WI_ERROR_PARAMETER3);
                if (InvalidRange(Length,    0,  WITEST_MAX_LENGTH))   return STATUS(WI_ERROR_PARAMETER4);
                if (InvalidRange(PrdBm,  -120.0,   30.0))             return STATUS(WI_ERROR_PARAMETER5);
                if (InvalidRange(FcMHz,  -100.0,  100.0))             return STATUS(WI_ERROR_PARAMETER6);
                if (InvalidRange(Delay,    -0.1,    0.1))             return STATUS(WI_ERROR_PARAMETER7);

                if (i > 0)
                {
                    State->BackgroundPacket[i].DataType = DataType;
                    State->BackgroundPacket[i].DataRate = DataRate * 1e6; // convert from Mbps to bps
                    State->BackgroundPacket[i].Length   = Length;
                    State->BackgroundPacket[i].PrdBm    = PrdBm;
                    State->BackgroundPacket[i].FcMHz    = FcMHz;
                    State->BackgroundPacket[i].Delay    = Delay;
                }
                else if (i == 0)
                {
                    if (FcMHz != 0.0) return STATUS(WI_ERROR_PARAMETER6);
                    if (Delay != 0.0) return STATUS(WI_ERROR_PARAMETER7);
                    State->DataType = DataType;
                    State->DataRate = DataRate * 1e6;
                    State->Length   = Length;
                    State->MinPrdBm = PrdBm;
                }
                return WI_SUCCESS;
            }

            XSTATUS( status = wiParse_Function( text,"wiTest.BackgroundPacket.DataType(%d, %d)", &i, &DataType) );
            if (status == WI_SUCCESS)
            {
                if (InvalidRange(i, 1, WITEST_MAX_BACKGROUNDPACKETS)) return STATUS(WI_ERROR_PARAMETER1);
                if (InvalidRange(DataType, 0, WITEST_MAX_DATATYPE))   return STATUS(WI_ERROR_PARAMETER2);
                State->BackgroundPacket[i].DataType = DataType;
                return WI_SUCCESS;
            }

            XSTATUS( status = wiParse_Function( text,"wiTest.BackgroundPacket.DataRate(%d, %d)", &i, &DataRate) );
            if (status == WI_SUCCESS)
            {
                if (InvalidRange(i, 1, WITEST_MAX_BACKGROUNDPACKETS)) return STATUS(WI_ERROR_PARAMETER1);
                if (InvalidRange(DataRate, 0.5, 1000.0)) return STATUS(WI_ERROR_PARAMETER3);
                State->BackgroundPacket[i].DataRate = DataRate * 1e6; // convert from Mbps to bps
                return WI_SUCCESS;
            }

            XSTATUS( status = wiParse_Function( text,"wiTest.BackgroundPacket.Length(%d, %d)", &i, &Length) );
            if (status == WI_SUCCESS)
            {
                if (InvalidRange(i, 1, WITEST_MAX_BACKGROUNDPACKETS)) return STATUS(WI_ERROR_PARAMETER1);
                if (InvalidRange(Length, 0,  WITEST_MAX_LENGTH))      return STATUS(WI_ERROR_PARAMETER4);
                State->BackgroundPacket[i].Length = Length;
                return WI_SUCCESS;
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Manually Load a Background Packet using TxPacket()
        //
        if (strstr(text, "wiTest_LoadPacket(") != NULL)
        {
            wiInt DataType, Length, i, Ns;
            wiReal DataRate, PrdBm, FcMHz, Delay, Prms, Fs;
            wiComplex **S;
            wiStatus status;

            XSTATUS( status = wiParse_Function( text,"wiTest_LoadPacket(%d, %d, %f, %d, %f, %f, %f)",
                                                &i, &DataType, &DataRate, &Length, &PrdBm, &FcMHz, &Delay ) );
            if (status == WI_SUCCESS)
            {
                wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

                if (InvalidRange(i, 1, WITEST_MAX_BACKGROUNDPACKETS)) return STATUS(WI_ERROR_PARAMETER1);
                if (InvalidRange(DataType,  0, WITEST_MAX_DATATYPE) ) return STATUS(WI_ERROR_PARAMETER2);
                if (InvalidRange(DataRate,  0.5, 1000.0))             return STATUS(WI_ERROR_PARAMETER3);
                if (InvalidRange(Length,    0,  WITEST_MAX_LENGTH))   return STATUS(WI_ERROR_PARAMETER4);
                if (InvalidRange(PrdBm,  -120.0,   30.0))             return STATUS(WI_ERROR_PARAMETER5);
                if (InvalidRange(FcMHz,  -100.0,  100.0))             return STATUS(WI_ERROR_PARAMETER6);
                if (InvalidRange(Delay,    -0.1,    0.1))             return STATUS(WI_ERROR_PARAMETER7);

                DataRate *= 1.0e6; // convert from bps to Mbps

                XSTATUS( wiPHY_GetData(WIPHY_TX_DATARATE_BPS, &DataRate) ); // Get PHY-Specific Parameters

                XSTATUS( wiTest_GetTxData( DataType, Length, pTxData) );
                XSTATUS( wiPHY_TX        ( DataRate, Length, pTxData, &Ns, &S, &Fs, &Prms) );
                XSTATUS( wiChanl_LoadPacket( Ns, S, Fs, Prms, i, PrdBm, FcMHz, Delay ) );

                return WI_SUCCESS;
            }
        }
        /////////////////////////////////////////////////////////////////////////////////////
        //
        //  Execute Individual Test Routines
        //
        if (strstr(text, "wiTest_") != NULL)
        {
            XSTATUS(status = wiParse_Command(text,"wiTest_FreeThreadData()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_FreeThreadData() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_TxPacket()"));
            if (status == WI_SUCCESS)
            {
                int Ns; wiReal Prms, Fs; wiComplex **S;
                wiUWORD *pTxData = State->TxData[wiProcess_ThreadIndex()];

                XSTATUS( wiTest_StartTest("wiTest_TxPacket()") );
                XSTATUS( wiTest_ClearCounters() );
                XSTATUS( wiTest_GetTxData( State->DataType, State->Length, pTxData ) );
                STATUS ( wiPHY_TX(State->DataRate, State->Length, pTxData, &Ns, &S, &Fs, &Prms) );

                if (State->TxRxCallback != NULL) {
                    XSTATUS( State->TxRxCallback() );
                }
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_OnePacket()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_OnePacket() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_OnePacket2Tx()"));
            XSTATUS(status2= wiParse_Command(text,"wiTest_OnePacketTx2()"));
            if (status == WI_SUCCESS || status2 == WI_SUCCESS)
            {
                XSTATUS( wiTest_OnePacketTx2() );
                return WI_SUCCESS;
            }
            XSTATUS(status= wiParse_Command(text,"wiTest_OnePacketTxMultiple()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_OnePacketTxMultiple() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_Stat_vs_SNR()"));
            XSTATUS(status2= wiParse_Command(text,"wiTest_StatvSNR()"));
            if (status == WI_SUCCESS || status2 == WI_SUCCESS)
            {
                XSTATUS (wiTest_StatvSNR() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_PERvSNR()"));
            XSTATUS(status2= wiParse_Command(text,"wiTest_PER_vs_SNR()"));
            if (status == WI_SUCCESS || status2 == WI_SUCCESS)
            {
                XSTATUS( wiTest_PERvSNR() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_PER_vs_Pr()"));
            XSTATUS(status2= wiParse_Command(text,"wiTest_PERvPrdBm()"));
            if (status == WI_SUCCESS || status2 == WI_SUCCESS)
            {
                XSTATUS( wiTest_PERvPrdBm() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_CPER_vs_Pr()"));
            XSTATUS(status2= wiParse_Command(text,"wiTest_CPERvPrdBm()"));
            if (status == WI_SUCCESS || status2 == WI_SUCCESS)
            {
                XSTATUS( wiTest_CPERvPrdBm() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_ByteErrorPDF()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_ByteErrorPDF() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_Benchmark()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_Benchmark() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_WriteAWG_TxPacket()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_WriteAWG_TxPacket() );
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_Callback()"));
            if (status == WI_SUCCESS)
            {
                if (State->ScriptCallback) {
                    XSTATUS( State->ScriptCallback() );
                }
                return WI_SUCCESS;
            }
            XSTATUS(status = wiParse_Command(text,"wiTest_Initialize_wiRnd()"));
            if (status == WI_SUCCESS)
            {
                XSTATUS( wiTest_Initialize_wiRnd() );
                return WI_SUCCESS;
            }
            // ------------------------------
            // Parameter sweep test functions
            // ------------------------------
            {
                double x1, x2, dx;
                char paramstr[128]; int paramtype;

                XSTATUS( status = wiParse_Function(text,     "wiTest_PER_Sensitivity(%d, %128s, %f, %f, %f)",&paramtype, paramstr, &x1, &dx, &x2));
                if (status) XSTATUS(status = wiParse_Function(text,"wiTest_PERvParam(%d, %128s, %f, %f, %f)",&paramtype, paramstr, &x1, &dx, &x2));
                if (status == WI_SUCCESS)
                {
                    XSTATUS( wiTest_PERvParam(paramtype, paramstr, x1, dx, x2) );
                    return WI_SUCCESS;
                }
            }
        }
    }
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Handle legacy "Tag"
    //
    if (strstr(text,"Tag"))
    {
        wiStatus status; int Tag;

        XSTATUS( status = wiParse_Int(text,"Tag", &Tag, 0, 999999999) );
        if (status == WI_SUCCESS)
        {
            if ((Tag>0) && (Tag<1000)) sprintf(State->FilenameExtension,"%03d",Tag);
            else if (Tag >= 1000)      sprintf(State->FilenameExtension,"%d",Tag);
            else                       sprintf(State->FilenameExtension,"out");
            return WI_SUCCESS;
        }
    }

    return WI_WARN_PARSE_FAILED;
}
// end of wiTest_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

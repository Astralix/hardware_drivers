//--< MojaveTest.c >-------------------------------------------------------------------------------
//=================================================================================================
//
//  Test Routines Specific to the Mojave Baseband (DW52 PHY)
//  Copyright 2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include "MojaveTest.h"
#include "wiTest.h"
#include "DW52.h"
#include "Mojave.h"
#include "wiChanl.h"
#include "wiMAC.h"
#include "wiMath.h"
#include "wiParse.h"
#include "wiParse.h"
#include "wiPHY.h"
#include "wiRnd.h"
#include "wiUtil.h"

//=================================================================================================
//--  MODULE LOCAL PARAMETERS
//=================================================================================================
static wiStatus          Status  = 0;
static MojaveTest_State *State   = 0;
static wiTest_State_t   *pState  = 0;

//=================================================================================================
//--  INTERNAL ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (WICHK(arg)<0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((Status=WICHK(arg))<=0) return Status;

//=================================================================================================
//--  LOCAL FUNCTION PROTOTYPES
//=================================================================================================
static double   MojaveTest_tCPU(void);
static wiStatus MojaveTest_ParseLine(wiString text);

// ================================================================================================
// FUNCTION  : MojaveTest_Init
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus MojaveTest_Init(void)
{
    if (State) return WI_ERROR_MODULE_ALREADY_INIT;

    State = (MojaveTest_State *)wiCalloc(1, sizeof(MojaveTest_State));
    if (!State) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    // --------------------
    // Setup default values
    // --------------------
    State->minEvents  =  10;
    State->eventMask  = 0xFFFFFFFF;
    State->eventState = 0x80000000;

    XSTATUS(wiParse_AddMethod(MojaveTest_ParseLine));
    return WI_SUCCESS;
}
// end of MojaveTest_Init()

// ================================================================================================
// FUNCTION  : MojaveTest_Close
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus MojaveTest_Close(void)
{
    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    wiFree(State); State = NULL;
    XSTATUS(wiParse_RemoveMethod(MojaveTest_ParseLine));
    return WI_SUCCESS;
}
// end of MojaveTest_Close()

// ================================================================================================
// FUNCTION  : MojaveTest_StartTest
// ------------------------------------------------------------------------------------------------
// Purpose   : Setup for the start o a test
// Parameters: TestFnName -- name of the test function
// ================================================================================================
static wiStatus MojaveTest_StartTest(wiString TestFnName)
{
    XSTATUS( wiTest_StartTest(TestFnName) );
    pState = wiTest_State();
    return WI_SUCCESS;
}
// end of MojaveTest_StartTest()

// ================================================================================================
// FUNCTION  : MojaveTest_EndTest
// ------------------------------------------------------------------------------------------------
// Purpose   : Cleanup at the end of test
// Parameters: none
// ================================================================================================
static wiStatus MojaveTest_EndTest(void)
{
    XSTATUS( wiTest_EndTest() );
    pState = NULL;
    pState->OutputFile = NULL;
    return WI_SUCCESS;
}
// end of MojaveTest_EndTest()

// ================================================================================================
// FUNCTION  : MojaveTest_EventFlagDefinition
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus MojaveTest_EventFlagDefinition(FILE *F, unsigned EventFlag)
{
    wiUWORD x; x.word = EventFlag;

    if (!F) return WI_ERROR_PARAMETER1;
    fprintf(F," EventFlag = 0x%08X\n",x.word);
    fprintf(F,"    %c [31] = Packet Error\n",x.b31? '*':' ');
    fprintf(F,"    %c [13] = x_ofs artifically limited\n",x.b13? '*':' ');
    fprintf(F,"    %c [12] = EqD output value(s) to PLL limited\n",x.b12? '*':' ');
    fprintf(F,"    %c [11] = Multiple minimum metrics in Viterbi decoder\n",x.b11? '*':' ');
    fprintf(F,"    %c [10] = Viterbi metric normalization exercised\n",x.b10? '*':' ');
    fprintf(F,"    %c [ 9] = Noise shift for demapper limited in ChEst\n",x.b9? '*':' ');
    fprintf(F,"    %c [ 8] = Received data set too short...buffer overrun\n",x.b8? '*':' ');
    fprintf(F,"    %c [ 7] = Carrier phase error exceeds +/- pi\n",x.b7? '*':' ');
    fprintf(F,"    %c [ 6] = AdvDly other than 0 during packet\n",x.b6? '*':' ');
    fprintf(F,"    %c [ 5] = LNA gain reduced\n",x.b5? '*':' ');
    fprintf(F,"    %c [ 4] = DFE detected false signal detect\n",x.b4? '*':' ');
    fprintf(F,"    %c [ 3] = Unrecognized RX_RATE\n",x.b3? '*':' ');
    fprintf(F,"    %c [ 2] = Parity error in SIGNAL symbol\n",x.b2? '*':' ');
    fprintf(F,"    %c [ 1] = RX_LENGTH = 0\n",x.b1? '*':' ');
    fprintf(F,"    %c [ 0] = Missed signal detect\n",x.b0? '*':' ');

    return WI_SUCCESS;
}
// end of MojaveTest_EventFlagDefinition()

// ================================================================================================
// FUNCTION  : MojaveTest_OnePacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Single packet transmit/receive
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_OnePacket(void)
{
    Mojave_TX_State *pTX; Mojave_RX_State *pRX; bMojave_RX_State *pbRX;
    unsigned eventFlags;

    XSTATUS( MojaveTest_StartTest("MojaveTest_OnePacket()") );

    wiTestWr(" Pr = %1.1f dBm\n",pState->MinPrdBm);
    wiTestWr("\n");

    XSTATUS( wiTest_ClearCounters() );
    XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
    XSTATUS( Mojave_StatePointer(&pTX, &pRX, NULL) );
    XSTATUS( bMojave_StatePointer(&pbRX));

    eventFlags = pRX->EventFlag.word;
    if (pState->BitErrorCount) eventFlags |= 0x80000000;

    MojaveTest_EventFlagDefinition(pState->OutputFile, eventFlags);
    wiTestWr(" BER = %d / %d = %8.2e\n",pState->BitErrorCount,pState->BitCount,pState->BER);
    wiTestWr(" PER = %d / %d = %8.2e\n",pState->PacketErrorCount,pState->PacketCount,pState->BER);
    wiTestWr("\n");

//   MojaveTest_EventFlagDefinition(stdout, eventFlags);
    wiPrintf(" \n");
    wiPrintf(" BER = %d / %d = %8.2e\n",pState->BitErrorCount,pState->BitCount,pState->BER);
    wiPrintf(" PER = %d / %d = %8.2e\n",pState->PacketErrorCount,pState->PacketCount,pState->PER);
    wiPrintf("\n");

    {
        MojaveTest_CheckSums Z;
        MojaveTest_CalcCheckSums(&Z);
        wiPrintf("Checksums = 0x((%08X,%08X,%08X),%08X,(%08X,%08X,%08X),%08X,%08X)\n",Z.TX,Z.TXarrays,Z.TX_u,Z.Chanl_r0,Z.RX_s,Z.RXarrays,Z.RX2,Z.bRXarrays,Z.Reg);
    }

    XSTATUS( MojaveTest_EndTest() );
    return WI_SUCCESS;
}
// end of MojaveTest_OnePacket()

// ================================================================================================
// FUNCTION  : MojaveTest_ViterbiArchitecture
// ------------------------------------------------------------------------------------------------
// Purpose   : Compare register exchange and trace-back Viterbi
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_ViterbiArchitecture(void)
{
    Mojave_RX_State *pRX; Mojave_TX_State *pTX;
    MojaveRegisters *pReg;
    unsigned int n, nRE, nTB, nBoth;
    wiBoolean eRE, eTB;

    XSTATUS( MojaveTest_StartTest("MojaveTest_ViterbiArchitecture()") );
    XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );

    nBoth = nRE = nTB = 0;

    for (n=0; n<pState->MaxPackets; n++)
    {
        wiRnd_Init(n, 0, 0);
        pReg->TX_PRBS.word = 0x7F;
        pRX->ViterbiType = 0;
        XSTATUS( wiTest_ClearCounters() );
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
        eRE = pState->PacketErrorCount? 1:0;
        
        wiRnd_Init(n, 0, 0);
        pReg->TX_PRBS.word = 0x7F;
        pRX->ViterbiType = 1;
        XSTATUS( wiTest_ClearCounters() );
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
        eTB = pState->PacketErrorCount? 1:0;

        nRE += eRE;
        nTB += eTB;
        nBoth += (eRE&eTB)? 1:0;

        printf("    > Packet %4d:%d <eRE=%d, eTB=%d> (%2d, RE=%2d, TB=%2d)\n",n+1,pState->MaxPackets,eRE,eTB,nBoth,nRE-nBoth,nTB-nBoth);
    }
    wiTestWr(" \n");
    wiTestWr(" Pr = %1.1f dBm\n",pState->MinPrdBm);
    wiTestWr(" RndSeed = Packet # for each packet\n\n");
    wiTestWr(" N = %d packets\n\n",n);
    wiTestWr(" PER (RegisterExchange) = %5d/%d = %9.2e\n",nRE,n,(double)nRE/n);
    wiTestWr(" PER (Trace-Back)       = %5d/%d = %9.2e\n",nTB,n,(double)nTB/n);
    wiTestWr(" \n");
    wiTestWr(" Packet Errors By Architecture\n");
    wiTestWr(" -----------------------------\n");
    wiTestWr(" Both        =%4d\n",nBoth);
    wiTestWr(" RegExchange =%4d\n",nRE-nBoth);
    wiTestWr(" Trace-Back  =%4d\n",nTB-nBoth);
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_Viterbi_vs_Pr()

// ================================================================================================
// FUNCTION  : MojaveTest_FrameSync_vs_Pr()
// ------------------------------------------------------------------------------------------------
// Purpose   : Measure frame sync statistics versus receive power
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_FrameSync_vs_Pr(void)
{
     Mojave_TX_State *pTX; Mojave_RX_State *pRX;
    wiReal  Pr_dBm;
    double x, minx, maxx, Sx, Sx2, avgx, stdx, n;

    XSTATUS( MojaveTest_StartTest("MojaveTest_FrameSync_vs_Pr()") );
    n = (double)pState->MaxPackets;

    wiTestWr("\n");
    wiTestWr(" N = %d samples/point\n",(int)n);
    wiTestWr("                                 <---- FrameSync x_ofs ---->\n");
    wiTestWr(" Pr[dBm]  # Failures     PER      min   max  average std dev\n");
    wiTestWr(" -------  ----------  ---------  ----- ----- ------- -------\n");

    for (Pr_dBm=pState->MinPrdBm; Pr_dBm<=pState->MaxPrdBm; Pr_dBm+=pState->StepPrdBm)
    {
        minx = 1e9; maxx=-1e9,  Sx=Sx2=0.0;
        XSTATUS( wiTest_ClearCounters() );
        while (pState->PacketCount < pState->MaxPackets)
        {
            XSTATUS( wiTest_TxRxPacket(1, Pr_dBm, pState->DataRate, pState->Length) );
            XSTATUS( Mojave_StatePointer(&pTX, &pRX, NULL) );
            x = (double)pRX->x_ofs;
            minx = min(x, minx);
            maxx = max(x, maxx);
            Sx += x;
            Sx2 += (x*x);
        }
        avgx = Sx/n;
        stdx = sqrt((Sx2 - Sx*Sx/n)/(n-1));
        wiPrintf(" % 5.1f dBm: BER = %8.2e, PER = %8.2e\n", Pr_dBm, pState->BER, pState->PER);
        wiTestWr(" % 6.1f  % 10d  % 9.2e %5.0f %5.0f %8.2f %6.2f\n", Pr_dBm, pState->PacketErrorCount, pState->PER,minx,maxx,avgx,stdx);
        
        if (pState->PER < pState->MinPER) break;
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_FrameSync_vs_Pr()

// ================================================================================================
// FUNCTION   : MojaveTest_EVM()
// ------------------------------------------------------------------------------------------------
// Purpose    : Measure EVM using equalized constellation and hard decision decoding
// Parameters : none
// ================================================================================================
wiStatus MojaveTest_EVM(void)
{
    unsigned int n, i, N=0;
    double EVMdB, Se2 = 0.0;
    FILE *F;
    double cSe2[64] = {0};

    Mojave_RX_State *pRX; Mojave_TX_State *pTX; MojaveRegisters *pReg;

    XSTATUS( MojaveTest_StartTest("MojaveTest_EVM()") );

    wiTestWr(" Pr = %1.1f dBm\n",pState->MinPrdBm);
    wiTestWr("\n");

    F = fopen("EVM.out","wt");

    XSTATUS( wiTest_ClearCounters() );
    XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );

    for (n=0; n<pState->MaxPackets; n++)
    {
        {
            int seed; unsigned n1, n2;
            wiRnd_GetState(&seed, &n1, &n2);
            wiPrintf(" wiRnd Seed(%d; %u,%8u) PRBS=0x%02X:",seed,n1,n2,pReg->TX_PRBS);
        }
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );

        for (i=0; i<64; i++) cSe2[i] += pRX->EVM.cSe2[0][i];
/**
        {
            int i;
            printf("\n");
            for (i=0; i<64; i++) {
                if (pRX->EVM.cSe2[0][i])
                    printf(" %4d %6.1f \n",-32+i,10.0*log10(pRX->EVM.cSe2[0][i]/(double)pRX->EVM.N_SYM));
                else
                    printf(" %4d %6.1f \n",-32+i,0);
            }
        }
/**/
        Se2 += pRX->EVM.Se2[0];
        N += (48 * pRX->N_SYM);
        EVMdB = 10.0 * log10(Se2 / ((double)N));

        wiPrintf(" EVM = %5.1f dB, Mean EVM = %5.2f dB\n",pRX->EVM.EVMdB[0],EVMdB);
//      wiPrintf(" EVMc=["); for (i=0; i<64; i++) printf("%6.1f",10.0*log10(pRX->EVM.cSe2[0][i]/(double)pRX->N_SYM)); printf("]\n");
        if (State->saveEVMbySymbol)
        {
            unsigned int i;
            fprintf(F,"%8.2f   ",pRX->EVM.EVMdB[0]);
            for (i=0; i<pRX->N_SYM; i++)
                fprintf(F,"%7.2f",10.0*log10(pRX->EVM.nSe2[0][i]/48.0));
            fprintf(F,"\n");
        }
        else 
        {
            fprintf(F,"%10.2f",pRX->EVM.EVMdB[0]);
            if (State->saveCFOwithEVM)  fprintf(F," %10.6f %10.6f\n",pRX->EVM.cCFO,pRX->EVM.fCFO);
            if (State->saveSyncWithEVM) fprintf(F," %4d",pRX->x_ofs);
            if (State->saveAGCwithEVM)  fprintf(F," %1d",pRX->EVM.nAGC);
            if (State->saveRSSIwithEVM) fprintf(F," %3u %3u",pReg->RSSI0.w8,pReg->RSSI1.w8);
            fprintf(F,"\n");
        }
    }      
/**
    {
        int i;
        printf("\n");
        for (i=0; i<64; i++) {
            if (cSe2[i])
                printf(" %4d %6.1f \n",-32+i,10.0*log10(cSe2[i]/((double)N/48.0)));
            else
                printf(" %4d %6.1f \n",-32+i,0);
        }
    }
/**/
    wiTestWr(" Number of Packets =  %d\n",pState->MaxPackets);
    wiTestWr(" Samples/Packet    =  48x%d\n",pRX->N_SYM);
    wiTestWr(" Mean EVM          = %1.2f dB\n",EVMdB);
    fclose(F);

    XSTATUS( MojaveTest_EndTest() );
    return WI_SUCCESS;
}
// end of MojaveTest_EVM()

// ================================================================================================
// FUNCTION  : MojaveTest_EVM_Sensitivity()
// ------------------------------------------------------------------------------------------------
// Purpose   : Meaure sensitivity of EVM to a parameter
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_EVM_Sensitivity(int paramtype, char paramstr[], double x1, double dx, double x2)
{
    unsigned int n, done=0;
    double x, Se2, EVMdB, N, minSe2, maxSe2, minEVMdB, maxEVMdB, pktSe2;
    char paramset[256];
    Mojave_RX_State *pRX; Mojave_TX_State *pTX; MojaveRegisters *pReg;

    XSTATUS( MojaveTest_StartTest("MojaveTest_EVM_Sensitivity()") );

    switch (paramtype)
    {
        case 1: wiTestWr(" Parameter Sweep: %s = %d:%d:%d\n",paramstr,(int)x1,(int)dx,(int)x2); break;
        case 2: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        case 3: wiTestWr(" Parameter Sweep: %s = %d:%d (%d)\n",paramstr,(int)x1,(int)x2,(int)dx); break;
        case 4: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        default: return WI_ERROR_PARAMETER1;
    }
    wiTestWr("\n");
    wiTestWr("  ParamValue     EVM      min      max   \n");
    wiTestWr(" ------------  -------  -------  ------- \n");

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
        XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );
    
        Se2 = 0.0;
        for (n=0; n<pState->MaxPackets; n++) {
            XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
            N = 48.0 * pRX->EVM.N_SYM;
            pktSe2 = min(N, pRX->EVM.Se2[0]);  // clamp EVM at 100% (0 dB)
            Se2 = Se2 + pktSe2;
            if (n) {
                minSe2 = min(minSe2, pktSe2);
                maxSe2 = max(maxSe2, pktSe2);
            }
            else minSe2 = maxSe2 = pktSe2;
        }
        N = 48.0 * pRX->EVM.N_SYM;
        minEVMdB = minSe2? 10*log10(minSe2/N) : 0.0;
        maxEVMdB = minSe2? 10*log10(maxSe2/N) : 0.0;
        N = 48.0 * pRX->EVM.N_SYM * pState->MaxPackets;
        EVMdB = Se2? 10*log10(Se2/N) : 0.0;

        switch (paramtype)
        {
            case 1: case 3: wiPrintf(" %s =% 6d: EVM = %4.1f dB\n", paramstr, (int)x, EVMdB); break;
            case 2: case 4: wiPrintf(" %s =% 8.4e: EVM = %4.1f dB\n", paramstr, x, EVMdB); break;
        }
        switch (paramtype)
        {
            case 1: wiTestWr(" % 12d % 7.2f  % 7.2f  % 7.2f\n", (int)x, EVMdB, minEVMdB, maxEVMdB); break;
            case 2: wiTestWr("% 13.4e % 7.2f  % 7.2f  % 7.2f\n",      x, EVMdB, minEVMdB, maxEVMdB); break;
            case 3: wiTestWr(" % 12d % 7.2f  % 7.2f  % 7.2f\n", (int)x, EVMdB, minEVMdB, maxEVMdB); break;
            case 4: wiTestWr("% 13.4e % 7.2f  % 7.2f  % 7.2f\n",      x, EVMdB, minEVMdB, maxEVMdB); break;
        }
        switch (paramtype)
        {
            case 1: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 2: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 3: done = (x*dx>x2); x *= dx; break;
            case 4: done = (x*dx>x2); x *= dx; break;
        }
    }
    XSTATUS( MojaveTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_EVM_Sensitivity()

// ================================================================================================
// FUNCTION  : MojaveTest_MissedPacket_Sensitivity
// ------------------------------------------------------------------------------------------------
// Purpose   : Meaure sensitivity of Missing Packets to a Parameter
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_MissedPacket_Sensitivity(int paramtype, char paramstr[], double x1, double dx, double x2)
{
    unsigned int done=0, N, Nmin, n;
    double x, p;
    char paramset[256];
    Mojave_RX_State *pRX; Mojave_TX_State *pTX; MojaveRegisters *pReg;

    XSTATUS( MojaveTest_StartTest("MojaveTest_MissedPacket_Sensitivity()") );

    switch (paramtype)
    {
        case 1: wiTestWr(" Parameter Sweep: %s = %d:%d:%d\n",paramstr,(int)x1,(int)dx,(int)x2); break;
        case 2: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        case 3: wiTestWr(" Parameter Sweep: %s = %d:%d (%d)\n",paramstr,(int)x1,(int)x2,(int)dx); break;
        case 4: wiTestWr(" Parameter Sweep: %s = %8.4e:%8.4e:%8.4e\n",paramstr,x1,dx,x2); break;
        default: return WI_ERROR_PARAMETER1;
    }
    wiTestWr("\n");
    wiTestWr("  ParamValue   Npackets    Nfail     Pfail \n");
    wiTestWr(" ------------  --------  --------  --------\n");

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
        XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );
    
        N = n = Nmin = 0;
        while ((n<pState->MinFail) || (N<Nmin))
        {
            XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
            N++; 
            n += pRX->EventFlag.b0;
            if ((n==pState->MinFail) && !Nmin) {
                Nmin = N * n/max(1,n-1) + 1;
            }
            if (N >= pState->MaxPackets) break;
        }
        p = (double)n/N;
        switch (paramtype)
        {
            case 1: case 3: wiPrintf(" %s =% 6d: Pfail = %1.6f\n", paramstr, (int)x, p); break;
            case 2: case 4: wiPrintf(" %s =% 8.4e: Pfail = %1.6f  \n", paramstr, x, p); break;
        }
        switch (paramtype)
        {
            case 1: wiTestWr( " % 12d  % 8d  % 8d % 8.5f\n", (int)x, N, n, p); break;
            case 2: wiTestWr("% 13.4e  % 8d  % 8d % 8.5f\n",      x, N, n, p); break;
            case 3: wiTestWr( " % 12d  % 8d  % 8d % 8.5f\n", (int)x, N, n, p); break;
            case 4: wiTestWr("% 13.4e  % 8d  % 8d % 8.5f\n",      x, N, n, p); break;
        }
        switch (paramtype)
        {
            case 1: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 2: done = (dx<0)? (x+dx<x2):(x+dx>x2); x += dx; break;
            case 3: done = (x*dx>x2); x *= dx; break;
            case 4: done = (x*dx>x2); x *= dx; break;
        }
        if (p < pState->MinPER) done = 1;
    }
    XSTATUS( MojaveTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_MissedPacket_Sensitivity()

// ================================================================================================
// FUNCTION  : MojaveTest_MultiplePackets()
// ------------------------------------------------------------------------------------------------
// Purpose   : Single packet transmit/receive
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_MultiplePackets(void)
{
    MojaveTest_CheckSums Z;
    Mojave_TX_State *pTX; Mojave_RX_State *pRX; bMojave_RX_State *pbRX;
    unsigned eventFlags, n, i;
    FILE *F[2];

    XSTATUS( MojaveTest_StartTest("MojaveTest_MultiplePackets()") );
    F[0] = stdout; F[1] = pState->OutputFile;

    for (i=0; i<2; i++)
    {
        fprintf(F[i]," Pr = %1.1f dBm\n",pState->MinPrdBm);
        fprintf(F[i],"\n");

        fprintf(F[i],"\n");
        fprintf(F[i],"     ---PHY---       -PacketBER-            -------------- Mojave Checksums ------------          \n");
        fprintf(F[i],"  n  Mbps  Len PrdBm Errors Bits eventFlags    TX      RX.s      RX      bRX       Reg    RndStep0\n");
        fprintf(F[i]," --- ---- ---- ----- -----/----- ---------- --------,--------,--------,--------,-------- ---------\n");
    }
    for (n=0; n<pState->MaxPackets; n++)
    {
        int seed, n1, n0;
        XSTATUS( wiRnd_GetState(&seed, &n1, &n0) );

        XSTATUS( wiTest_ClearCounters() );
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
        XSTATUS( Mojave_StatePointer(&pTX, &pRX, NULL) );
        XSTATUS( bMojave_StatePointer(&pbRX));

        eventFlags = pRX->EventFlag.word;
        if (pState->BitErrorCount) eventFlags |= 0x80000000;

        MojaveTest_CalcCheckSums(&Z);

        for (i=0; i<2; i++)
        {
            fprintf(F[i],"%4u%c%4g %4u %5.1f %5u/%5u",n,pState->BER? '*':' ',pState->DataRate/1e6,pState->Length,pState->MinPrdBm,pState->BitErrorCount,pState->BitCount);
            fprintf(F[i]," 0x%08X",eventFlags);
            fprintf(F[i]," %08X,%08X,%08X,%08X,%08X",Z.TXarrays,Z.RX_s,Z.RXarrays,Z.bRXarrays,Z.Reg);
            fprintf(F[i],"%10u",n0);
            fprintf(F[i],"\n");
        }
        fflush(pState->OutputFile);
    }
    XSTATUS( MojaveTest_EndTest() );
    return WI_SUCCESS;
}
// end of MojaveTest_MultiplePackets()

// ================================================================================================
// FUNCTION   : MojaveTest_RSSI()
// ------------------------------------------------------------------------------------------------
// Purpose    : Measure RSSI v Pr[dBm]
// Parameters : none
// ================================================================================================
wiStatus MojaveTest_RSSI(void)
{
    unsigned int i, n;
    double RSSI[2], RSSI2[2], meanRSSI[2], sigmaRSSI[2], minRSSI[2], maxRSSI[2];
    double Pr_dBm;

    Mojave_RX_State *pRX; bMojave_RX_State *pbRX; Mojave_TX_State *pTX; MojaveRegisters *pReg;

    XSTATUS( MojaveTest_StartTest("MojaveTest_RSSI()") );
    XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );
    XSTATUS( bMojave_StatePointer(&pbRX));


    wiTestWr(" Number of Packets = %d\n",pState->MaxPackets);
    wiTestWr("\n");
    wiTestWr("\n");
    wiTestWr("                             <--------- RSSI[0] --------->  <--------- RSSI[1] --------->\n");
    wiTestWr(" Pr[dBm]  # Fail     PER      Mean  StdDev   Min     Max     Mean  StdDev   Min     Max  \n");
    wiTestWr(" -------  ------  ---------  ------ ------ ------- -------  ------ ------ ------- -------\n");

    for (Pr_dBm=pState->MinPrdBm; Pr_dBm<=pState->MaxPrdBm; Pr_dBm+=pState->StepPrdBm)
    {
        XSTATUS( wiTest_ClearCounters() );
        for (i=0; i<2; i++)
        {
            RSSI[i]    = 0.0;
            RSSI2[i]   = 0.0;
            minRSSI[i] = 1e9;
            maxRSSI[i] =-1e9;
        }
        for (n=0; n<pState->MaxPackets; n++)
        {
            wiUWORD *PHY_RX_D = (pReg->RXMode.w2==2)? pbRX->PHY_RX_D : pRX->PHY_RX_D; // select "b" or "a" mode
            XSTATUS( wiTest_TxRxPacket(1, Pr_dBm, pState->DataRate, pState->Length) );

            // ------------------
            // Collect Statistics
            // ------------------
            RSSI[0]    += (double)(PHY_RX_D[0].w8);
            RSSI2[0]   += (double)(PHY_RX_D[0].w8) * (PHY_RX_D[0].w8);
            minRSSI[0]  = min(minRSSI[0], (double)(PHY_RX_D[0].w8));
            maxRSSI[0]  = max(maxRSSI[0], (double)(PHY_RX_D[0].w8));

            RSSI[1]    += (double)(PHY_RX_D[1].w8);
            RSSI2[1]   += (double)(PHY_RX_D[1].w8) * (PHY_RX_D[1].w8);
            minRSSI[1]  = min(minRSSI[1], (double)(PHY_RX_D[1].w8));
            maxRSSI[1]  = max(maxRSSI[1], (double)(PHY_RX_D[1].w8));
        }
        // --------------------
        // Calculate statistics
        // --------------------
        for (i=0; i<2; i++)
        {
            double n  = (double)pState->MaxPackets;
            double Sx = RSSI[i];
            double Sx2= RSSI2[i];

            meanRSSI[i] = Sx / n;
            sigmaRSSI[i] = sqrt((Sx2 - Sx*Sx/n)/(n-1));
        }
        wiTestWr(" % 6.1f  % 6d  % 9.2e % 7.1f%7.2f % 7.1f %7.1f % 7.1f% 7.2f % 7.1f %7.1f\n",Pr_dBm, 
            pState->PacketErrorCount, pState->PER, meanRSSI[0],sigmaRSSI[0],minRSSI[0],maxRSSI[0], meanRSSI[1],sigmaRSSI[1],minRSSI[1],maxRSSI[1]);
    }
    XSTATUS( MojaveTest_EndTest() );
    return WI_SUCCESS;
}
// end of DakotaTest_CQM()

// ================================================================================================
// FUNCTION  : MojaveTest_FindEvent
// ------------------------------------------------------------------------------------------------
// Purpose   : Search for a particular event and report the conditions
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_FindEvent(void)
{
    Mojave_TX_State *pTX; Mojave_RX_State *pRX; MojaveRegisters *pReg; bMojave_RX_State *pbRX;
    int event_count = 0;
    wiUWORD TxPRBS;
    unsigned int eventFlags;
    int RndSeed; unsigned int RndStep[2];

    XSTATUS( MojaveTest_StartTest("MojaveTest_FindEvent()") );

    wiTestWr(" Pr = %4.2f dBm\n",pState->MinPrdBm);
    wiTestWr("\n");
    wiTestWr(" event = (0x%08X & 0x%08X) = 0x%08X\n",State->eventState,State->eventMask,State->eventState & State->eventMask);
    wiTestWr("\n");
    wiTestWr(" packet #   #   EventFlags  PRBS   RndSeed   RndStep[1]  RndStep[0]\n");
    wiTestWr(" --------  ---  ----------  ----  ---------  ----------  ----------\n");

    XSTATUS( wiTest_ClearCounters() );
    XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );
    XSTATUS( bMojave_StatePointer(&pbRX));

    while ((event_count<State->minEvents) || (pState->PacketCount < pState->MinPacketCount))
    {
        pState->BitErrorCount = 0;
        TxPRBS = pReg->TX_PRBS;
        XSTATUS(wiRnd_GetState(&RndSeed, RndStep+1, RndStep+0));
        XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
        eventFlags = pRX->EventFlag.word;
        if (pState->BitErrorCount) eventFlags |= 0x80000000;

        if ((eventFlags & State->eventMask) == (State->eventState & State->eventMask))
        {
            event_count++;
            printf("%4d  0x%08X  0x%02X  %9d %11d %11d\n",event_count, eventFlags, TxPRBS, RndSeed, RndStep[1], RndStep[0]);
            wiTestWr("%9d %4d  0x%08X  0x%02X  %9d %11d %11d\n",pState->PacketCount, event_count, eventFlags, TxPRBS, RndSeed, RndStep[1], RndStep[0]);
        }
    }
    XSTATUS( MojaveTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_FindEvent()

// ================================================================================================
// FUNCTION  : MojaveTest_SyncTh_Sensitivity()
// ------------------------------------------------------------------------------------------------
// Purpose   : Meaure sensitivity of PER to (SyncThSig)(2^SyncThExp)
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_SyncTh_Sensitivity(void)
{
    int done=0;
    double x;
    char paramset[256];
    int SyncThSig, SyncThExp;

    XSTATUS( MojaveTest_StartTest("MojaveTest_SyncTh_Sensitivity()") );

    wiTestWr(" Pr = %1.1f dBm\n",pState->MinPrdBm);
    wiTestWr("\n");
    wiTestWr(" Parameter Sweep: SyncThSig={4,5,6,7}; SyncThExp={-6,-5,-4,-3}\n");
    wiTestWr("\n");
    wiTestWr(" ParamValue  # Errors     BER     # Failures     PER    \n");
    wiTestWr(" ----------  --------  ---------  ----------  --------- \n");

    for (SyncThExp=-6; SyncThExp<=-3; SyncThExp++) {
        for (SyncThSig=4; SyncThSig<=7; SyncThSig++)
        {
            x = (double)SyncThSig * pow(2.0,SyncThExp);
            sprintf(paramset,"Mojave.Register.SyncThExp = %d",SyncThExp); XSTATUS(wiParse_Line(paramset));
            sprintf(paramset,"Mojave.Register.SyncThSig = %d",SyncThSig); XSTATUS(wiParse_Line(paramset));
                XSTATUS( wiTest_ClearCounters() );
            while ((pState->PacketErrorCount<pState->MinFail) || (pState->PacketCount < pState->MinPacketCount))
            {
                XSTATUS( wiTest_TxRxPacket(1, pState->MinPrdBm, pState->DataRate, pState->Length) );
                if (pState->PacketCount >= pState->MaxPackets) break;
            }
            wiPrintf(" SyncTh=(%d,%d): BER = %8.2e, PER = %6.2f%%\n", SyncThSig,SyncThExp, pState->BER, 100.0*pState->PER);
            wiTestWr("% 11.6f % 8d  % 9.2e % 10d  % 9.2e\n",      x, pState->BitErrorCount, pState->BER, pState->PacketErrorCount, pState->PER);
            if (pState->PER < pState->MinPER) done = 1;
        }
    }
    XSTATUS( wiTest_EndTest() );
    
    return WI_SUCCESS;
}
// end of MojaveTest_SyncTh_Sensitivity()

// ================================================================================================
// FUNCTION  : MojaveTest_SoftMAC_Frame
// ------------------------------------------------------------------------------------------------
// Purpose   : Configure DW52 to use a "SoftMAC" frame
// Parameters: pdata -- pointer to byte data array
// Notes     : used for DW52 chip-level verification
// ================================================================================================
wiStatus MojaveTest_SoftMAC_Frame(wiUWORD *pdata)
{
    DW52_State   *pDW52;
    wiUWORD RATE, LENGTH, TXPWRLVL, bTXMode;
    unsigned n;

    wiTest_State_t *pState = wiTest_State();
    wiUWORD *pTxData = pState->TxData[wiProcess_ThreadIndex()];
    
    XSTATUS(DW52_StatePointer(&pDW52));

    TXPWRLVL.word = pdata[0].w8; // Extract data fiels from "Bergana" header
    bTXMode.word  = pdata[1].b7;
    RATE.word     = pdata[2].w8 >> 4;
    LENGTH.word   =(pdata[2].w4 << 12) | pdata[3].w8;

    for (n=0; n<LENGTH.w12; n++)
        pTxData[n].word = pdata[n+8].w8; // Load PSDU into wiTest TX buffer

    wiMAC_InsertFCS(LENGTH.w12, pTxData); // Add FCS onto the end
    LENGTH.word += 4; // increment LENGTH to include FCS

    if (bTXMode.b0) // set the DataRate and shortPreamble fields base on RATE
    {
        switch (RATE.w4)
        {
            case 0x0: pState->DataRate = 1.0e6; pDW52->shortPreamble = 0; break;
            case 0x1: pState->DataRate = 2.0e6; pDW52->shortPreamble = 0; break;
            case 0x2: pState->DataRate = 5.5e6; pDW52->shortPreamble = 0; break;
            case 0x3: pState->DataRate =11.0e6; pDW52->shortPreamble = 0; break;
            case 0x4: pState->DataRate = 1.0e6; pDW52->shortPreamble = 1; break;
            case 0x5: pState->DataRate = 2.0e6; pDW52->shortPreamble = 1; break;
            case 0x6: pState->DataRate = 5.5e6; pDW52->shortPreamble = 1; break;
            case 0x7: pState->DataRate =11.0e6; pDW52->shortPreamble = 1; break;
            default : return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    else
    {
        switch (RATE.w4)
        {
            case 0xD: pState->DataRate =  6.0e6; break;
            case 0xF: pState->DataRate =  9.0e6; break;
            case 0x5: pState->DataRate = 12.0e6; break;
            case 0x7: pState->DataRate = 18.0e6; break;
            case 0x9: pState->DataRate = 24.0e6; break;
            case 0xB: pState->DataRate = 36.0e6; break;
            case 0x1: pState->DataRate = 48.0e6; break;
            case 0x3: pState->DataRate = 54.0e6; break;
            case 0xA: pState->DataRate = 72.0e6; break;
            default : return STATUS(WI_ERROR_UNDEFINED_CASE);
        }
    }
    pState->Length = LENGTH.w12; // set the wiTest length to the PSDU length
    pDW52->TXPWRLVL= TXPWRLVL;   // set the DW52 TXPWRLVL field

    pState->DataType = 3; // change data type to retain pState->TxData values
    return WI_SUCCESS;
}
// end of MojaveTest_SoftMAC_Frame()

// ================================================================================================
// FUNCTION  : MojaveTest_Load_SoftMAC_Frame
// ------------------------------------------------------------------------------------------------
// Purpose   : Load a "SoftMAC" frame using data from the specified file
// ================================================================================================
wiStatus MojaveTest_Load_SoftMAC_Frame(wiString filename)
{
    wiUWORD data[4112] ={0};
    char buf[256];
    unsigned n = 0;

    FILE *F = wiParse_OpenFile(filename,"rt");
    if (!F) XSTATUS(WI_ERROR_FILE_OPEN);

    while (!feof(F) && n<4112)
    {
        fgets(buf, 255, F);
        data[n++].word = strtol(buf, NULL, 0);
    }
    fclose(F);
    XSTATUS(MojaveTest_SoftMAC_Frame(data));

    return WI_SUCCESS;
}
// end of MojaveTest_Load_SoftMAC_Frame()

// ================================================================================================
// FUNCTION  : MojaveTest_CalcCheckSums
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus MojaveTest_CalcCheckSums(MojaveTest_CheckSums *Z)
{
    Mojave_TX_State *pTX; Mojave_RX_State *pRX; MojaveRegisters *pReg; bMojave_RX_State *pbRX;
    wiChanl_State_t *pChanl;
    wiUInt *z;
    int n;

    XSTATUS( Mojave_StatePointer(&pTX, &pRX, &pReg) );
    XSTATUS( bMojave_StatePointer(&pbRX));
    pChanl = wiChanl_State();

    memset(Z, 0xFF, sizeof(*Z)); // initial state for checksums is 0xFFFFFFFF

    // -------
    // Channel
    // -------
    wiMath_FletcherChecksum(sizeof(wiComplex)*(pChanl->Nr), (wiByte*)pChanl->r[0], &(Z->Chanl_r0));

    // --------
    // Transmit
    // --------
    z = &(Z->TXarrays);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Na), (wiByte*)pTX->a,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nb), (wiByte*)pTX->b,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nc), (wiByte*)pTX->c,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Np), (wiByte*)pTX->p,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nc), (wiByte*)pTX->cReal, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nc), (wiByte*)pTX->cImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nv), (wiByte*)pTX->vReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nv), (wiByte*)pTX->vImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nx), (wiByte*)pTX->xReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nx), (wiByte*)pTX->xImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Ny), (wiByte*)pTX->yReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Ny), (wiByte*)pTX->yImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nz), (wiByte*)pTX->zReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pTX->Nz), (wiByte*)pTX->zImag, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nz), (wiByte*)pTX->uReal, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nz), (wiByte*)pTX->uImag, z);

    Z->TX = Z->TXarrays;                                                               // keep checksums from arrays
//   wiMath_FletcherChecksum(sizeof(*pTX),              (wiByte*)pTX,        &(Z->TX)); // ...and add top-level structures

    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Na), (wiByte*)pTX->a, &(Z->TX_abc));
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nb), (wiByte*)pTX->b, &(Z->TX_abc));
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nc), (wiByte*)pTX->c, &(Z->TX_abc));

    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nz), (wiByte*)pTX->uReal, &(Z->TX_u));
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pTX->Nz), (wiByte*)pTX->uImag, &(Z->TX_u));

    // ------------------
    // Receive (OFDM/Top)
    // ------------------
    z = &(Z->RXarrays);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pRX->Na), (wiByte*)pRX->a,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pRX->Nb), (wiByte*)pRX->b,     z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pRX->Nc), (wiByte*)pRX->Lc,    z);
    for (n=0; n<2; n++)
    {
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nu), (wiByte*)pRX->uReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nu), (wiByte*)pRX->uImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nv), (wiByte*)pRX->vReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nv), (wiByte*)pRX->vImag[n], z);
    }
    Z->RX2 = Z->RXarrays;
    for (n=0; n<2; n++)
    {
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nx), (wiByte*)pRX->xReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nx), (wiByte*)pRX->xImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Ns), (wiByte*)pRX->sReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Ns), (wiByte*)pRX->sImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Ns), (wiByte*)pRX->yReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Ns), (wiByte*)pRX->yImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nr), (wiByte*)pRX->zReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nr), (wiByte*)pRX->zImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nr), (wiByte*)pRX->rReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiWORD )*(pRX->Nr), (wiByte*)pRX->rImag[n], z);
        wiMath_FletcherChecksum(sizeof(wiUWORD)*(pRX->Nr), (wiByte*)pRX->qReal[n], z);
        wiMath_FletcherChecksum(sizeof(wiUWORD)*(pRX->Nr), (wiByte*)pRX->qImag[n], z);
    }

    Z->RX = Z->RXarrays;                                                               // keep checksums from arrays
    wiMath_FletcherChecksum(sizeof(*pRX),               (wiByte*)pRX,       &(Z->RX)); // ...and add top-level structures

//   wiMath_FletcherChecksum(sizeof(wiComplex)*(pRX->Nr),(wiByte*)pRX->s[0], &(Z->RX_s));
//   wiMath_FletcherChecksum(sizeof(wiComplex)*(pRX->Nr),(wiByte*)pRX->s[1], &(Z->RX_s));

    // ------------------
    // Receive (DSSS/CCK)
    // ------------------
    z = &(Z->bRXarrays);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N40), (wiByte*)pbRX->xReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N40), (wiByte*)pbRX->xImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N40), (wiByte*)pbRX->yReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N40), (wiByte*)pbRX->yImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N44), (wiByte*)pbRX->sReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N44), (wiByte*)pbRX->sImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N22), (wiByte*)pbRX->zReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N22), (wiByte*)pbRX->zImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->vReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->vImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->uReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->uImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N22), (wiByte*)pbRX->rReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N22), (wiByte*)pbRX->rImag, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->tReal, z);
    wiMath_FletcherChecksum(sizeof(wiWORD )*(pbRX->N11), (wiByte*)pbRX->tImag, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pbRX->N22), (wiByte*)pbRX->State, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pbRX->N22), (wiByte*)pbRX->EDOut, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pbRX->N22), (wiByte*)pbRX->CQOut, z);
    wiMath_FletcherChecksum(sizeof(wiUWORD)*(pbRX->Na),  (wiByte*)pbRX->a,     z);

    Z->bRX = Z->bRXarrays;                                                                 // keep checksums from arrays
    wiMath_FletcherChecksum(sizeof(*pbRX),               (wiByte*)pbRX,        &(Z->bRX)); // ...and add top-level structures

    // ---------
    // Registers
    // ---------
    wiMath_FletcherChecksum(sizeof(*pReg), (wiByte*)pReg, &(Z->Reg));

    return WI_SUCCESS;
}
// end of bMojave_CalcCheckSums()

// ================================================================================================
// FUNCTION  : MojaveTest_WriteAWG_TxPacket()
// ------------------------------------------------------------------------------------------------
// Purpose   : Write an AWG5x0 format file with one packet from baseband DAC input
// Parameters: none
// ================================================================================================
wiStatus MojaveTest_WriteAWG_TxPacket(void)
{
    FILE *FI, *FQ, *F;
    char filename[256];
    unsigned short X;
    int m,n,k,N; unsigned i;
    wiComplex **S;
    wiReal Prms, Ts;
    Mojave_TX_State *pTX;
    wiUWORD *pTxData = pState->TxData[wiProcess_ThreadIndex()];

    XSTATUS( MojaveTest_StartTest("MojaveTest_WriteAWG_TxPacket()") );

    // ----------------
    // Get the raw data
    // ----------------
    switch (pState->DataType)
    {
        case 0: XSTATUS( wiTest_RandomMessage(pState->Length, pTxData) ); break;
        case 1: XSTATUS( wiTest_Annex_G_Message(pState->Length, pTxData) ); break;
        case 2: XSTATUS( wiTest_SingleByteMessage(pState->Length, pTxData, pState->FixedByte) ); break;
        case 3: break;
        case 4: XSTATUS( wiMAC_DataFrame(pState->Length, pTxData, 0, 0) ); break;
        case 5: XSTATUS( wiMAC_DataFrame(pState->Length, pTxData, 1, 0) ); break;
        case 6: XSTATUS( wiMAC_DataFrame(pState->Length, pTxData, 0, 1) ); break;
        default: return STATUS(WI_ERROR_UNDEFINED_CASE);
    }
    // -----------
    // Transmitter
    // -----------
    XSTATUS( wiPHY_TX(pState->DataRate, pState->Length, pTxData, &N, &S, &Ts, &Prms) );
    XSTATUS( Mojave_StatePointer(&pTX, NULL, NULL) );

    N = pTX->Nz;   // number of samples in uReal,uImag
    N = (N>>3)<<3; // round down to a multiple of 8

    // --------------------------
    // Create AWG5x0 format files
    // --------------------------
    n = 2*(N + 80); m = (int)ceil(log((double)n)/log(10.0));

    sprintf(filename,"Tx%d_%d_I.wfm",(int)(pState->DataRate/1e6),pState->Length);
    FI = fopen(filename,"wb"); 
    if (!FI) return STATUS(WI_ERROR_FILE_OPEN);

    sprintf(filename,"Tx%d_%d_Q.wfm",(int)(pState->DataRate/1e6),pState->Length);
    FQ = fopen(filename,"wb"); 
    if (!FQ) return STATUS(WI_ERROR_FILE_OPEN);

    fprintf(FI,"MAGIC 2000\n#%d%d",m,n);
    fprintf(FQ,"MAGIC 2000\n#%d%d",m,n);

    for (k=0; k<80; k++)       // Toggle Marker 1 high for 80 clock periods (1us)
    {                               
        fprintf(FI,"%c%c",0,0x22);
        fprintf(FQ,"%c%c",0,0x22);
    }
    for (k=0; k<N; k++)        // Transmit data
    {
        X = (unsigned short)(pTX->uReal[k].w10);
        fprintf(FI,"%c%c",X&0xFF, (X>>8)&0xFF);

        X = (unsigned short)(pTX->uImag[k].w10);
        fprintf(FQ,"%c%c",X&0xFF, (X>>8)&0xFF);
    }
    fprintf(FI,"CLOCK 8.000000E+07\n");
    fprintf(FQ,"CLOCK 8.000000E+07\n");

    fclose(FI); fclose(FQ);

    // -----------------------
    // Save the tx_data stream
    // -----------------------
    F = fopen("tx_data.out","wt");
    for (i=0; i<pState->Length; i++)
        fprintf(F,"%u\n",pTxData[i].w8);
    fclose(F);

    // ------------------------------------
    // Save the transmit data in ASCII form
    // ------------------------------------
    sprintf(filename,"Tx%d_%d.out",(int)(pState->DataRate/1e6),pState->Length);
    F = fopen(filename,"wt");
    for (k=0; k<N; k++) fprintf(F,"%4u %4u\n",pTX->uReal[k].w10,pTX->uImag[k].w10);
    fclose(F);
        
    XSTATUS( wiTest_EndTest() );
    return WI_SUCCESS;
}
// end of MojaveTest_WriteAWG_TxPacket()

// ================================================================================================
// FUNCTION  : MojaveTest_ParseLine
// ------------------------------------------------------------------------------------------------
// ================================================================================================
static wiStatus MojaveTest_ParseLine(wiString text)
{
    char buf[256];
    wiStatus status;

    if (!State) return WI_ERROR_MODULE_NOT_INITIALIZED;

    PSTATUS( wiParse_Int (text,"MojaveTest.minEvents", &(State->minEvents), 1, 10000) );
    PSTATUS( wiParse_UInt(text,"MojaveTest.eventMask", &(State->eventMask), 0, 0xFFFFFFFF) );
    PSTATUS( wiParse_UInt(text,"MojaveTest.eventState",&(State->eventState),0, 0xFFFFFFFF) );
    PSTATUS( wiParse_Boolean(text,"MojaveTest.saveEVMbySymbol", &(State->saveEVMbySymbol)) );
    PSTATUS( wiParse_Boolean(text,"MojaveTest.saveCFOwithEVM",  &(State->saveCFOwithEVM)) );
    PSTATUS( wiParse_Boolean(text,"MojaveTest.saveSyncWithEVM", &(State->saveSyncWithEVM)) );
    PSTATUS( wiParse_Boolean(text,"MojaveTest.saveAGCwithEVM",  &(State->saveAGCwithEVM)) );
    PSTATUS( wiParse_Boolean(text,"MojaveTest.saveRSSIwithEVM", &(State->saveRSSIwithEVM)) );

    // -------------------
    // Load SoftMAC Packet
    // -------------------
    XSTATUS(status = wiParse_Function(text,"MojaveTest_Load_SoftMAC_Frame(%255s)",buf));
    if (status==WI_SUCCESS)
    {
        XSTATUS(MojaveTest_Load_SoftMAC_Frame(buf));
        return WI_SUCCESS;
    }
    // --------------------------------
    // Parameter sweep test function(s)
    // --------------------------------
    {
        double x1, x2, dx;
        char paramstr[128]; int paramtype;

        XSTATUS(status = wiParse_Function(text,"MojaveTest_EVM_Sensitivity(%d, %128s, %f, %f, %f)",&paramtype, paramstr, &x1, &dx, &x2));
        if (status == WI_SUCCESS) {
            XSTATUS(MojaveTest_EVM_Sensitivity(paramtype, paramstr, x1, dx, x2));
            return WI_SUCCESS;
        }
        XSTATUS(status = wiParse_Function(text,"MojaveTest_MissedPacket_Sensitivity(%d, %128s, %f, %f, %f)",&paramtype, paramstr, &x1, &dx, &x2));
        if (status == WI_SUCCESS) {
            XSTATUS(MojaveTest_MissedPacket_Sensitivity(paramtype, paramstr, x1, dx, x2));
            return WI_SUCCESS;
        }
    }
    // -----------------------------
    // Run individual test functions
    // -----------------------------
    XSTATUS(status = wiParse_Command(text,"MojaveTest_FindEvent()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_FindEvent());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_OnePacket()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_OnePacket());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_ViterbiArchitecture()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_ViterbiArchitecture());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_EVM()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_EVM());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_RSSI()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_RSSI());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_MultiplePackets()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_MultiplePackets());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_FrameSync_vs_Pr()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_FrameSync_vs_Pr());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_SyncTh_Sensitivity()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_SyncTh_Sensitivity());
        return WI_SUCCESS;
    }
    XSTATUS(status = wiParse_Command(text,"MojaveTest_WriteAWG_TxPacket()"));
    if (status == WI_SUCCESS)
    {
        XSTATUS(MojaveTest_WriteAWG_TxPacket());
        return WI_SUCCESS;
    }
    return WI_WARN_PARSE_FAILED;
}
// end of MojaveTest_ParseLine()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

// 2007-08-31 Brickner - Added MojaveTest_WriteAWG_TxPacket()

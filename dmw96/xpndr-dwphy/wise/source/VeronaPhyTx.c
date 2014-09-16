//--< VeronaPhyTx.c >--------------------------------------------------------------------------------
//=================================================================================================
//
//  Model for the XpandR IV PHY (Verona + RF55)
//  Copyright 2007-2011 DSP Group, Inc. All rights reserved.
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
#include "VeronaPhyTx.h"
#include "Verona.h"
#include "Coltrane.h"

//=================================================================================================
//--  MODULE LOCAL PARAMETERS
//=================================================================================================
static VeronaPhyTx_State_t State[WISE_MAX_THREADS] = {{0}};

//=================================================================================================
//--  INTERNAL ERROR HANDLING
//=================================================================================================
#define  STATUS(arg) WICHK((arg))
#define XSTATUS(arg) if (STATUS(arg) < 0) return WI_ERROR_RETURNED;
#define PSTATUS(arg) if ((pstatus = WICHK(arg)) <= 0) return pstatus;

// ================================================================================================
// FUNCTION  : VeronaPhyTx_InitializeThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus VeronaPhyTx_InitializeThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        VeronaPhyTx_State_t *pState = State + ThreadIndex;
        XSTATUS( VeronaPhyTx_CloseThread(ThreadIndex) )

        memcpy( pState, State, sizeof(VeronaPhyTx_State_t) );
    
        pState->MAC_TX_D = (wiUWORD *)wiCalloc(65536+8, sizeof(wiUWORD));
        if (!pState->MAC_TX_D) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

        pState->TX.s[0] = pState->TX.s[1] = NULL;

        XSTATUS( Verona_InitializeThread(ThreadIndex) );
        XSTATUS( Coltrane_InitializeThread(ThreadIndex) );
        pState->ThreadIsInitialized = WI_TRUE;
    }
    return WI_SUCCESS;
}
// end of VeronaPhyTx_InitializeThread()

// ================================================================================================
// FUNCTION  : VeronaPhyTx_CloseThread()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus VeronaPhyTx_CloseThread(unsigned int ThreadIndex)
{
    if (InvalidRange(ThreadIndex, 1, WISE_MAX_THREADS)) 
        return WI_ERROR_PARAMETER1;
    else
    {
        VeronaPhyTx_State_t *pState = State + ThreadIndex;

        WIFREE( pState->MAC_TX_D )
        WIFREE( pState->TX.s[0]  )
        WIFREE( pState->RX.s[0]  )
        WIFREE( pState->RX.s[1]  )
        pState->ThreadIsInitialized = WI_FALSE;

        XSTATUS( Verona_CloseThread(ThreadIndex) );
        XSTATUS( Coltrane_CloseThread(ThreadIndex) );
    }
    return WI_SUCCESS;
}
// end of VeronaPhyTx_CloseThread()


// ================================================================================================
// FUNCTION  : VeronaPhyTx_TX()
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
wiStatus VeronaPhyTx_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms)
{
    int i, Nx = 0;
    wiUWORD RATE = {0}, MCS = {0}, LENGTH;
    wiBoolean TXbMode, TXnMode;
    wiComplex *x = NULL;

    VeronaPhyTx_State_t *pState = VeronaPhyTx_State();

    // --------------------
    // Error/Range Checking
    // --------------------
    if (!data) return STATUS(WI_ERROR_PARAMETER3);
    if (!Ns)   return STATUS(WI_ERROR_PARAMETER4);
    if (!S)    return STATUS(WI_ERROR_PARAMETER5);  
    
    if (!pState->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( VeronaPhyTx_InitializeThread(wiProcess_ThreadIndex()) )
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
            if (InvalidRange(Length, 1, 65535)) return STATUS(WI_ERROR_PARAMETER2);
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
            if (InvalidRange(Length, 1, 4095)) return STATUS(WI_ERROR_PARAMETER2);
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

        pState->LengthPSDU = Length;
    }
    else
    {
        return STATUS(WI_ERROR); // option not supported
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
    XSTATUS( Verona_TX_Restart() );
    XSTATUS( Verona_TX(Length+8, pState->MAC_TX_D, &Nx, &x, pState->M) );
    
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
    XSTATUS( Coltrane_TX_Restart(Nx, 80.0*pState->M, Verona_Prms()) );
    XSTATUS( Coltrane_TX(Nx, x, pState->TX.s[0], Nx - Verona_TxState()->OfsEOP) );

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
// end of VeronaPhyTx_TX()

// ================================================================================================
// FUNCTION  : VeronaPhyTx_RX
// ------------------------------------------------------------------------------------------------
// Purpose   : Top-level receive macro function for testing
// Parameters: Length -- Number of bytes in "data"
//             data   -- Decoded received data
//             Nr     -- Number of receive samples
//             R      -- Array of receive signals (by ref)
// ================================================================================================
wiStatus VeronaPhyTx_RX(int *Length, wiUWORD *data[], int Nr, wiComplex *R[])
{
    VeronaPhyTx_State_t *pState = VeronaPhyTx_State();
  
    if (!pState->ThreadIsInitialized && wiProcess_ThreadIndex()) {
        XSTATUS( VeronaPhyTx_InitializeThread(wiProcess_ThreadIndex()) )
    }
   
    /////////////////////////////////////////////////////////////////////////////////////
    //
    //  Return the PSDU
    //
    //  The PHY data payload is returned in array data. Because the test routines do not
    //  process the design-specific PHY header used between the MAC and PHY, the first
    //  eight bytes are excluded from the returned data.
    //
    if (Length) *Length = pState->LengthPSDU;
    if (data)     *data = pState->MAC_TX_D + 8;

    return WI_SUCCESS;
}
// end of VeronaPhyTx_RX()

// ================================================================================================
// FUNCTION : VeronaPhyTx_ParseLine()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus VeronaPhyTx_ParseLine(wiString text)
{
    if (strstr(text, "VeronaPhyTx"))
    {
        wiStatus status, pstatus;
        const unsigned int MaxPHYEnB = (unsigned)80e6;
        int i, addr, data, k;

        VeronaPhyTx_State_t *pState = VeronaPhyTx_State();

        PSTATUS(wiParse_Int    (text,"VeronaPhyTx.M",              &(pState->M), 1, 10000));
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.dumpTX",         &(pState->dumpTX)) );
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.dumpRX",         &(pState->dumpRX)) );
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.DumpTX",         &(pState->dumpTX)) );
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.DumpRX",         &(pState->dumpRX)) );
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.ShortPreamble",  &(pState->ShortPreamble)));
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.shortPreamble",  &(pState->ShortPreamble))); // legacy support, deprecated
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.TxGreenfield",   &(pState->TxGreenfield)));  
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.TxAggregate",    &(pState->TxAggregate)));  
        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.TxOpProtect",    &(pState->TxOpProtect)));  
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.TXPWRLVL",       &(pState->TXPWRLVL.word), 0, 255));

        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.DW_PHYEnB.kEnable",  pState->DW_PHYEnB.kEnable,  0, MaxPHYEnB));
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.DW_PHYEnB.kDisable", pState->DW_PHYEnB.kDisable, 0, MaxPHYEnB));

        PSTATUS(wiParse_UIntArray(text, "VeronaPhyTx.DW_PHYEnB.kEnable",  pState->DW_PHYEnB.kEnable,  3, 0, MaxPHYEnB));
        PSTATUS(wiParse_UIntArray(text, "VeronaPhyTx.DW_PHYEnB.kDisable", pState->DW_PHYEnB.kDisable, 3, 0, MaxPHYEnB));

        PSTATUS( wiParse_Boolean  (text,"VeronaPhyTx.PhyTxHeader.ForceHeader", &(pState->PhyTxHeader.ForceHeader)) );
        PSTATUS( wiParse_UIntArray(text,"VeronaPhyTx.PhyTxHeader.Byte", &(pState->PhyTxHeader.Byte[0]), 7, 0, 255) );

        PSTATUS( wiParse_String (text,"VeronaPhyTx.AV.Checker",     pState->AV.Checker, VERONAPHYTX_AV_NAME_LENGTH) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.Limit",     &(pState->AV.Limit),     -1e15, 1e15) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.LimitLow",  &(pState->AV.LimitLow),  -1e15, 1e15) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.LimitHigh", &(pState->AV.LimitHigh), -1e15, 1e15) );
        PSTATUS( wiParse_Int    (text,"VeronaPhyTx.AV.N",         &(pState->AV.N),         INT_MIN, INT_MAX) );  
        PSTATUS( wiParse_String (text,"VeronaPhyTx.AV.Parameter",   pState->AV.Parameter, VERONAPHYTX_AV_NAME_LENGTH) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.ParamLow",  &(pState->AV.ParamLow),  -1e15, 1e15) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.ParamHigh", &(pState->AV.ParamHigh), -1e15, 1e15) );
        PSTATUS( wiParse_Real   (text,"VeronaPhyTx.AV.ParamStep", &(pState->AV.ParamStep), -1e15, 1e15) );
        PSTATUS( wiParse_UIntArray(text,"VeronaPhyTx.AV.UIntValue", pState->AV.UIntValue, 8, 0, 0xFFFFFFFF) );

        PSTATUS(wiParse_Boolean(text,"VeronaPhyTx.FixedOutputADC.Use", &(pState->FixedOutputADC.Use)) );
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.FixedOutputADC.AI",  &(pState->FixedOutputADC.AI), 0, 1023) );
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.FixedOutputADC.AQ",  &(pState->FixedOutputADC.AQ), 0, 1023) );
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.FixedOutputADC.BI",  &(pState->FixedOutputADC.BI), 0, 1023) );
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.FixedOutputADC.BQ",  &(pState->FixedOutputADC.BQ), 0, 1023) );

        PSTATUS(wiParse_Boolean  (text,"VeronaPhyTx.WriteRegister.Use", &(pState->WriteRegister.Use)) );
        PSTATUS(wiParse_UIntArray(text,"VeronaPhyTx.WriteRegister.k80",   pState->WriteRegister.k80,  VERONAPHYTX_MAX_WRITEREGISTER, 0, 80000000));
        PSTATUS(wiParse_UIntArray(text,"VeronaPhyTx.WriteRegister.Addr",  pState->WriteRegister.Addr, VERONAPHYTX_MAX_WRITEREGISTER, 0, 1023));
        PSTATUS(wiParse_UIntArray(text,"VeronaPhyTx.WriteRegister.Data",  pState->WriteRegister.Data, VERONAPHYTX_MAX_WRITEREGISTER, 0,  255));

        PSTATUS(wiParse_Boolean  (text,"VeronaPhyTx.Checksum.Enable", &(pState->Checksum.Enable)) );

        if ((status = STATUS(wiParse_Function(text,"VeronaPhyTx_WriteRegister(%d, %d, %d, %d)",&i,&addr,&data,&k))) == WI_SUCCESS)
        {
            if (InvalidRange(i,    0, VERONAPHYTX_MAX_WRITEREGISTER)) return STATUS(WI_ERROR_PARAMETER1);
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
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.T_PHYEnB",       &(pState->DW_PHYEnB.kEnable[0] ), 0, MaxPHYEnB));
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.T_PHYEnB[0]",    &(pState->DW_PHYEnB.kEnable[0] ), 0, MaxPHYEnB));
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.T_PHYEnB[1]",    &(pState->DW_PHYEnB.kDisable[0]), 0, MaxPHYEnB));
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.T_PHYEnB[2]",    &(pState->DW_PHYEnB.kEnable[1] ), 0, MaxPHYEnB));
        PSTATUS(wiParse_UInt   (text,"VeronaPhyTx.T_PHYEnB[3]",    &(pState->DW_PHYEnB.kDisable[1]), 0, MaxPHYEnB));
    }
    return WI_WARN_PARSE_FAILED;
}
// end of VeronaPhyTx_ParseLine()

// ================================================================================================
// FUNCTION : VeronaPhyTx_WriteConfig()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus VeronaPhyTx_WriteConfig(wiMessageFn MsgFunc)
{
    char t[4][32]; int i;
    VeronaPhyTx_State_t *pState = VeronaPhyTx_State();

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

    if (pState->FixedOutputADC.Use) {
        MsgFunc(" >>>>>> Fixed Output ADC: AI=%u, AQ=%u, BI=%u, BQ=%u\n",pState->FixedOutputADC.AI, 
            pState->FixedOutputADC.AQ, pState->FixedOutputADC.BI, pState->FixedOutputADC.BQ);
    }
    else
    {
        XSTATUS( Coltrane_WriteConfig(MsgFunc) );
    }
    MsgFunc(" -----------------------------------------------------------------------------------------------\n");
    MsgFunc(" Modem                 = Verona Baseband\n");
    XSTATUS( Verona_WriteConfig(MsgFunc) );
    return WI_SUCCESS;
}
// end of VeronaPhyTx_WriteConfig()

// ================================================================================================
// FUNCTION : VeronaPhyTx_State()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
VeronaPhyTx_State_t * VeronaPhyTx_State(void)
{
    return State + wiProcess_ThreadIndex();
}
// end of VeronaPhyTx_StatePointer()

// ================================================================================================
// FUNCTION : VeronaPhyTx()
// ------------------------------------------------------------------------------------------------
// Purpose  : This is the top-level interface function accessed by wiPHY; it does not
//            contain code used to define Verona hardware.
// ================================================================================================
wiStatus VeronaPhyTx(int command, void *pData)
{
    switch (command & 0xFFFF0000)
    {
        case WIPHY_WRITE_CONFIG:
        {
            wiMessageFn MsgFunc = (wiMessageFn)*((wiFunction *)pData);
            XSTATUS(VeronaPhyTx_WriteConfig(MsgFunc));
            break;
        }
        case WIPHY_ADD_METHOD:    
        {
            VeronaPhyTx_State_t *pState = VeronaPhyTx_State();

            wiPhyMethod_t *method = (wiPhyMethod_t *)pData;
            method->TxFn = VeronaPhyTx_TX;            
            method->RxFn = VeronaPhyTx_RX;        
            XSTATUS(wiParse_AddMethod(VeronaPhyTx_ParseLine));
            XSTATUS(wiParse_AddMethod(Verona_ParseLine));

            pState->MAC_TX_D = (wiUWORD *)wiCalloc(65536+8, sizeof(wiUWORD));
            if (!pState->MAC_TX_D) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

            XSTATUS( Coltrane_Init() );
            XSTATUS( Verona_Init() );

            pState->M = 1;
            pState->DW_PHYEnB.kEnable[0]  = pState->T_PHYEnB[0] = 20;
            pState->DW_PHYEnB.kDisable[0] = pState->T_PHYEnB[1] = (unsigned)80e6;
            pState->ThreadIsInitialized = WI_TRUE;
            break;
        }
        case WIPHY_REMOVE_METHOD:
        {
            int i;

            XSTATUS(wiParse_RemoveMethod(VeronaPhyTx_ParseLine));
            XSTATUS(wiParse_RemoveMethod(Verona_ParseLine));
            XSTATUS(Coltrane_Close());
            XSTATUS(Verona_Close());

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
            XSTATUS( Verona_WriteRegister(pReg->Addr, pReg->Data) );
            break;
        }
        case WIPHY_READ_REGISTER:
        {
            wiPhyRegister_t *pReg = (wiPhyRegister_t *)pData;
            pReg->Data = Verona_ReadRegister(pReg->Addr);
            break;
        }
        case WIPHY_CLOSE_ALL_THREADS:
        {
            unsigned int n;
            for (n = 1; n < WISE_MAX_THREADS; n++) {
                XSTATUS( VeronaPhyTx_CloseThread(n) )
            }
            break;
        }
        default: return WI_WARN_METHOD_UNHANDLED_COMMAND;
    }
    return WI_SUCCESS;
}
// end of VeronaPhyTx()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------

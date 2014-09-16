//
//  DW52.h -- DW52 PHY
//  Copyright 2005 DSP Group, Inc. All rights reserved.
//

#ifndef __DW52_H
#define __DW52_H

#include "wiType.h"
#include "Mojave.h"
#include "RF52.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=============================================================================
//  DW52 PARAMETER STRUCTURE
//=============================================================================
typedef struct
{
    int  M;           // Oversample rate

    wiUWORD MAC_TX_D[4104];
    wiUWORD PHY_RX_D[4104];

    wiBoolean dumpTX;
    wiBoolean dumpRX;
    wiBoolean displayRSSI;
    wiBoolean shortPreamble;  // select 802.11b short preamble
    wiUInt    T_PHYEnB[4];    // number of clock period DW_PHYEnB is deasserted
    wiUWORD TXPWRLVL;

    struct
    {
        Mojave_TX_State *pTX;
        Mojave_RX_State *pRX;
        MojaveRegisters *pReg;
        bMojave_RX_State *pbRX;
    }  Mojave;

    struct
    {
        RF52_TxState_t *pTX;
        RF52_RxState_t *pRX;
    }  RF52;

    struct
    {
        int        Ns;
        wiComplex *s[2]; // radio output, channel input signal
    } TX;

    struct
    {
        int        Ns;
        wiComplex *s[2]; // radio output, ADC input signal
    } RX;

    struct
    {
        wiBoolean Use;
        char Filename[256];
        int Nx;
        wiUWORD *xI, *xQ;
    } ExternalRxADC;

}  DW52_State;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus DW52(int command, void *pdata);
wiStatus DW52_StatePointer(DW52_State **pState);
wiStatus DW52_LoadRxADC_from_Array(wiInt N, wiComplex *x);
wiStatus DW52_TX(double bps, int Length, wiUWORD *data, int *Ns, wiComplex ***S, double *Fs, double *Prms); 
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

//
//  TamalePHY.h -- TamalePHY (Tamale Baseband and RF55 Radio)
//  Copyright 2007-2009 DSP Group, Inc. All rights reserved.
//

#ifndef __TAMALEPHY_H
#define __TAMALEPHY_H

#include "wiType.h"
#include "Tamale.h"
#include "Coltrane.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#define TAMALEPHY_AV_NAME_LENGTH    128
#define TAMALEPHY_MAX_WRITEREGISTER  16

//=================================================================================================
//  TamalePHY PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    int  M;                  // Oversample rate
    int  NumRx;              // number of receivers (set by wiTest)
    wiBoolean ThreadIsInitialized;

    wiUWORD  *MAC_TX_D;
    wiBoolean RxValidFCS;    // indicate whether the packet data (PHY_RX_D) has a valid FCS

    wiBoolean dumpTX;
    wiBoolean dumpRX;
    wiBoolean displayRSSI;
    wiBoolean ShortPreamble;  // select 802.11b short preamble
    wiUInt    T_PHYEnB[4];    // number of clock period DW_PHYEnB is deasserted
    wiUWORD   TXPWRLVL;
    wiBoolean TxAggregate;    // Aggregate MPDU
    wiBoolean TxGreenfield;   // GreenField Packets
    wiBoolean TxOpProtect;    // Enable TXOP protection in mixed-mode transmissions

    Tamale_CtrlState_t         inReg,      outReg;      // top-level control signals
    Tamale_DataConvCtrlState_t DataConvIn, DataConvOut; // data converter control
    Tamale_RadioDigitalIO_t    RadioSigIn, RadioSigOut; // radio/RF signals

    struct
    {
        unsigned int kEnable[4];  // assertion for DW_PHYEnB (active low)
        unsigned int kDisable[4]; // de-assertion time for DW_PHYEnB

    } DW_PHYEnB;

    struct
    {
        int        Ns;
        wiComplex *s[2];      // radio output, channel input signal
    } TX;

    struct
    {
        int        Ns;
        wiComplex *s[2];     // radio output, ADC input signal
    } RX;

    struct
    {
        int    k;
        double A;
    } TxIQLoopback;

    struct
    {
        wiBoolean ForceHeader; // use the data below for the PHY header
        wiUInt    Byte[8];
    } PhyTxHeader;

    struct
    {
        char Checker[TAMALEPHY_AV_NAME_LENGTH]; // checker function name
        char Parameter[TAMALEPHY_AV_NAME_LENGTH]; // parameter name
        wiReal Limit;        // test limit
        wiReal LimitLow;     // test limit (lower value)
        wiReal LimitHigh;    // test limit (upper value)
        wiInt  N;            // test count (generic: number of packets, etc.)
        wiReal ParamLow;     // lower parameter value
        wiReal ParamHigh;    // upper parameter value
        wiReal ParamStep;    // parameter step
        wiUInt UIntValue[8];   // generic unsigned int
    } AV;

    struct
    {
        wiBoolean Use;
        char Filename[256];
        int Nx;
        wiUWORD *xI, *xQ;
    } ExternalRxADC;

    struct
    {
        wiBoolean Use;
        wiUInt    AI, AQ, BI, BQ;
    } FixedOutputADC;

    struct
    {
        wiBoolean Use;
        wiUInt    k80 [TAMALEPHY_MAX_WRITEREGISTER+1]; // 80 MHz time index to complete write
        wiUInt    Addr[TAMALEPHY_MAX_WRITEREGISTER+1]; // register address
        wiUInt    Data[TAMALEPHY_MAX_WRITEREGISTER+1]; // register data to write
    } WriteRegister;

    struct {
        wiBoolean Enable; // Enable checksum computation
        wiUInt    RX, TX; // Checksum for selected RX and TX arrays
        wiUInt    Result; // Combined TX and RX (computed after RX)
    } Checksum;

}  TamalePHY_State_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exported Functions)
//=================================================================================================
wiStatus TamalePHY(int command, void *pdata);
wiStatus TamalePHY_LoadRxADC_from_Array(wiInt N, wiComplex *x);
wiStatus TamalePHY_TX(double bps, int Length, wiUWORD data[], int *Ns, wiComplex **S[], double *Fs, double *Prms); 
wiStatus TamalePHY_WriteConfig(wiMessageFn MsgFunc);

wiStatus TamalePHY_InitializeThread(unsigned int ThreadIndex);
wiStatus TamalePHY_CloseThread     (unsigned int ThreadIndex);

TamalePHY_State_t * TamalePHY_State(void);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------


//
//  MojaveTest.h -- Dakota Test Routines
//  Copyright 2002 Bermai, Inc. All rights reserved.
//

#ifndef __MOJAVETEST_H
#define __MOJAVETEST_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  TEST PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiUInt     eventMask;
    wiUInt     eventState;
    wiInt      minEvents;
    wiBoolean  saveEVMbySymbol;
    wiBoolean  saveCFOwithEVM;
    wiBoolean  saveSyncWithEVM;
    wiBoolean  saveAGCwithEVM;
    wiBoolean  saveRSSIwithEVM;
}  MojaveTest_State;

//=================================================================================================
//  MOJAVE CHECKSUM STRUCTURE
//=================================================================================================
typedef struct
{
    wiUInt TX, RX, bRX, Reg;
    wiUInt TXarrays, RXarrays, bRXarrays;
    wiUInt TX_abc, TX_u;
    wiUInt RX_s, RX2;
    wiUInt Chanl_r0;
}  MojaveTest_CheckSums;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================
wiStatus MojaveTest_Init (void);
wiStatus MojaveTest_Close (void);
wiStatus MojaveTest_CalcCheckSums (MojaveTest_CheckSums *Z);
wiStatus MojaveTest_SoftMAC_Frame (wiUWORD *pByteArray);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

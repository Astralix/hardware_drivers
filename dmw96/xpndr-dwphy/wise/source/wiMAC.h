//
//  wiMAC.h -- WISE 802.11 MAC Emulation
//  Copyright 2004 Bermai, 2007 DSP Group, Inc. All rights reserved.
//

#ifndef __WIMAC_H
#define __WIMAC_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

//=================================================================================================
//  TEST PARAMETER STRUCTURE
//=================================================================================================
typedef struct
{
    wiUInt ProtocolVersion;
    wiUInt Type;
    wiUInt Subtype;
    wiUInt ToDS;
    wiUInt FromDS;
    wiUInt MoreFrag;
    wiUInt Retry;
    wiUInt PwrMgt;
    wiUInt MoreData;
    wiUInt WEP;
    wiUInt Order;
    wiUInt DurationID;
    wiUInt Address[4][6];   // MAC Addresses
    wiUInt SequenceControl;
    wiUInt QoSControl;
    wiInt  Ns_AMPDU;        // # of subframes in AMPDU   
    wiInt  Ns_AMSDU;        // # of subframes in AMSDU	
    wiUInt SeedPRBS;        // seed for the internal PRBS
    wiInt  BufferLength;    // number of byptes in the buffer
    wiByteArray_t DataBuffer; // user data buffer
}  wiMAC_State_t;

//=================================================================================================
//  USER-CALLABLE FUNCTION DEFINITIONS (Exportable Functions)
//=================================================================================================
wiStatus  wiMAC_Init (void);
wiStatus  wiMAC_Close(void);

wiStatus  wiMAC_PseudoRandomByteSequence(wiInt n, wiUWORD *pData);
wiStatus  wiMAC_InsertFCS(wiInt nBytes, wiUWORD X[]);
wiBoolean wiMAC_ValidFCS (wiInt nBytes, wiUWORD X[]);

wiStatus  wiMAC_SetDataBuffer(wiInt n, wiUWORD *pData);

wiStatus  wiMAC_DataFrame     (wiInt Length, wiUWORD *PSDU, wiInt  FrameBodyType, wiBoolean Broadcast);
wiStatus  wiMAC_AMPDUDataFrame(wiInt Length, wiUWORD *PSDU, wiBoolean RandomMSDU, wiBoolean Broadcast);
wiStatus  wiMAC_AMSDUDataFrame(wiInt Length, wiUWORD *PSDU, wiBoolean RandomMSDU, wiBoolean Broadcast);
//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

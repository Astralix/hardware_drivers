//
// RvClient.h -- Remote VWAL Client
// Copyright 2007, 2010 DSP Group, Inc. All rights reserved.
//

#ifndef __RVCLIENT_H
#define __RVCLIENT_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#include "RvTypes.h"
#include "stddef.h"

// ================================================================================================
// DEFINE RVWAL_API FOR THIS BUILD
// This is used export functions during a DLL build and to import them when the library is 
// included in an application.
// ================================================================================================
#ifndef RVWAL_NO_DLL

    #ifdef RVWAL_EXPORTS
        #define RVWAL_API __declspec(dllexport)
    #else
        #define RVWAL_API __declspec(dllimport)
    #endif

#else

    #define RVWAL_API

#endif

// ================================================================================================
// Remote VWAL Configuration and Utilities
// ================================================================================================
RVWAL_API RvStatus_t RvClientConnect   (char *Addr, uint16_t Port);
RVWAL_API RvStatus_t RvClientDisconnect(void);
RVWAL_API RvStatus_t RvClientDisplay   (RvPrintfFn_t pPrintfFn, int Verbose);
RVWAL_API RvStatus_t RvClientSeedRandomGenerator(uint32_t Seed);
RVWAL_API RvStatus_t RvClientShutdown  (void);

// ================================================================================================
// MIB Access and Packet API
// ================================================================================================
RVWAL_API RvStatus_t RvClientReadMib (uint16_t MibObjectID, void *pMibObject, size_t MibObjectSize);
RVWAL_API RvStatus_t RvClientWriteMib(uint16_t MibObjectID, void *pMibObject, size_t MibObjectSize);

RVWAL_API RvStatus_t RvClientReadMacAddress (uint8_t* pAddr);
RVWAL_API RvStatus_t RvClientWriteMacAddress(uint8_t* pAddr);

RVWAL_API RvStatus_t RvClientTxBufferPacket(uint8_t PhyRate, uint8_t PowerLevel);
RVWAL_API RvStatus_t RvClientTxSinglePacket(uint16_t Length, uint8_t PhyRate, uint8_t PowerLevel, 
                                            uint32_t PacketType, uint32_t LongPreamble);
RVWAL_API RvStatus_t RvClientTxMultiplePackets(uint8_t NumPkts, uint16_t Length[], uint8_t PhyRate[], 
                                               uint8_t PowerLevel[], uint32_t PacketType[], uint32_t LongPreamble[]);

RVWAL_API RvStatus_t RvClientLoadTxBuffer  (uint32_t Length, uint8_t* pData);

RVWAL_API RvStatus_t RvClientStartTxBatchBuffer(uint32_t NumTransmits, uint8_t PhyRate, 
                                                uint8_t PowerLevel, uint32_t LongPreamble);
RVWAL_API RvStatus_t RvClientStartTxBatch( uint32_t NumTransmits, uint16_t Length, uint8_t PhyRate, 
                                           uint8_t PowerLevel, uint32_t PacketType, uint32_t LongPreamble );
RVWAL_API RvStatus_t RvClientStopTxBatch(void);

RVWAL_API RvStatus_t RvClientQueryTxBatch(uint8_t *pData);
        
RVWAL_API RvStatus_t RvClientRxSinglePacket( RvRxPacketFrame_t* pFrame );

RVWAL_API RvStatus_t RvClientSystem( char *CommandString );

// ================================================================================================
// OS Wrapper Style Functions (these must match what is used internally by the WiFi driver)
// ================================================================================================
RVWAL_API uint32_t OS_RegRead   ( uint32_t Addr                 );
RVWAL_API void     OS_RegWrite  ( uint32_t Addr, uint32_t Value );
RVWAL_API uint8_t  OS_RegReadBB ( uint32_t Addr                 );
RVWAL_API void     OS_RegWriteBB( uint32_t Addr, uint8_t Value  );
RVWAL_API void     OS_Delay     ( uint32_t microsecond );

// ================================================================================================
// DwPhyMex Specific Utilities
// ================================================================================================
RVWAL_API RvStatus_t RvClientPhyTestMode    ( void );
RVWAL_API RvStatus_t RvClientGetPerCounters ( RvPerFrame_t* pFrame );
RVWAL_API RvStatus_t RvClientRxRSSI         ( RvRxPacketFrame_t* pFrame );
RVWAL_API RvStatus_t RvClientRxRSSIValues   ( RvBufferFrame_t* pFrame );
RVWAL_API RvStatus_t RvClientRegReadMultiple( RvBufferFrame_t* pFrame, uint32_t Addr );
RVWAL_API RvStatus_t RvClientReadRAM256( RvRamFrame_t* pFrame, uint32_t IndexAddr, uint32_t DataAddr[4] );


//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __RVCLIENT_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

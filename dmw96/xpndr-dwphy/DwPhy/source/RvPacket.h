//
// RvPacket.h -- Remote VWAL TX Packet Generator
// Copyright 2007 DSP Group, Inc. All rights reserved.
//

#ifndef __RVPACKET_H
#define __RVPACKET_H

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

#include "RvTypes.h"

// ================================================================================================
// Function Prototypes
// ================================================================================================
RvStatus_t RvPacket_SetRandomSeed  (uint32_t Seed);
RvStatus_t RvPacket_SimpleBroadcast(uint8_t* pTxPacketBuffer, uint16_t Length, uint8_t PacketType);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __RVPACKET_H

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

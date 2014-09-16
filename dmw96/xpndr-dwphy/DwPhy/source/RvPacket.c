//--< RvPacket.c >------------------------------------------------------------------------
//=================================================================================================
//
//  Remote VWAL Packet Generator
//  Copyright 2007 DSP Group, Inc. All rights reserved.
//
//=================================================================================================
//-------------------------------------------------------------------------------------------------

#include <stdlib.h>
#include "RvTypes.h"
#include "RvUtil.h"

// ================================================================================================
// MODULE-SPECIFIC PARAMETERS
// ================================================================================================


// ================================================================================================
// INTERNAL FUNCTION PROTOTYPES
// ================================================================================================
#define VPRINTF if(RvUtil_Verbose()) RvPrintf

// ================================================================================================
// FUNCTION   : RvPacket_SimpleBroadcast
// ------------------------------------------------------------------------------------------------
// Purpose    : Generate a simple broadcast data packet
// Parameters : pTxPacketBuffer -- Buffer into which the packet data is placed
//              Length          -- PSDU length minus 4 byte FCS field
//              PacketType      -- as defined in RvTypes.h
// ================================================================================================
RvStatus_t RvPacket_SimpleBroadcast(uint8_t* pTxPacketBuffer, uint16_t Length, uint8_t PacketType)
{
    const uint8_t MacHdr[] = {0x08, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x0E,
                              0x4C, 0x00, 0x00, 0x01, 0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x70, 0x18 };

    int i;

    if(Length < 24) return RVWAL_ERROR_PARAMETER_RANGE;

    switch(PacketType)
    {
        case RVWAL_PACKET_BROADCAST_MOD256:
        {
            for(i=0; i<24; i++)
                pTxPacketBuffer[i] = MacHdr[i];

            for(i=0; i<Length-24; i++)
                pTxPacketBuffer[24+i] = (uint8_t)(i%256);

            break;
        }

        case RVWAL_PACKET_BROADCAST_RANDOM:
        {
            for(i=0; i<24; i++)
                pTxPacketBuffer[i] = MacHdr[i];

            for(i=0; i<Length-24; i++)
                pTxPacketBuffer[24+i] = (uint8_t)(rand() % 256);

            break;
        }

        case RVWAL_PACKET_BROADCAST_ONES:
        {
            for(i=0; i<24; i++)
                pTxPacketBuffer[i] = MacHdr[i];

            for(i=0; i<Length-24; i++)
                pTxPacketBuffer[24+i] = 0xFF;;

            break;
        }
        default: return RVWAL_ERROR_UNDEFINED_CASE;
    }
    return RVWAL_SUCCESS;
} 
// end of RvPacket_SimpleBroadcast()

// ================================================================================================
// FUNCTION   : RvPacket_SetRandomSeed
// ------------------------------------------------------------------------------------------------
// Purpose    : Seed the random number generator
// Parameters : Seed - Seed value
// ================================================================================================
RvStatus_t RvPacket_SetRandomSeed(uint32_t Seed)
{
    VPRINTF(" RvPacket_SetRandomSeed(%u)\n",Seed);
    srand( Seed );
    return RVWAL_SUCCESS;
} 
// end of RvPacket_SetRandomSeed()

//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------

// 2008-08-03 Brickner; Added PacketType = RVWAL_PACKET_BROADCAST_ONES

#ifndef TXPACKETSETTINGS_H
#define TXPACKETSETTINGS_H

#include "TypeDefs.h"

#define MIN_PACKET_SIZE 48
#define MAX_PACKET_SIZE 1536

class CTxPacketSettings
{
public:

	CTxPacketSettings();

	~CTxPacketSettings();

	void DisplayCurrentPacketSettings();

	uint32 numTransmits;
	uint8  phyRate;
	uint16 packetSize;      
	uint8  txPowerLevel;
	uint8  payloadType;

};
#endif //TXPACKETSETTINGS_H

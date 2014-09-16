#include "TxPacketSettings.h"
#include <iostream>
#include <iomanip>
using namespace std;

CTxPacketSettings::CTxPacketSettings()
{
	numTransmits 	= 	10000; 
	phyRate			=	6;
	packetSize		=	1500;      
	txPowerLevel	=	36;
	payloadType 	= 	0;
}

CTxPacketSettings::~CTxPacketSettings()
{

}

void CTxPacketSettings::DisplayCurrentPacketSettings()
{

	cout<<endl<<endl<<"Current packet settings"<<endl;

	cout<<setw(20)<<left<<"Packet Size: "<<MIN_PACKET_SIZE+packetSize<<endl;
	cout<<setw(20)<<left<<"Phy Rate: "<<(uint32)phyRate<<endl;
	cout<<setw(20)<<left<<"Power Level: "<<(uint32)txPowerLevel<<endl;


	switch(payloadType)
	{
		case 0:
			cout<<setw(20)<<left<<"Payload Type: "<<"Fixed"<<endl;
			break;
		case 1:
			cout<<setw(20)<<left<<"Payload Type: "<<"Random"<<endl;
			break;
		case 2:
			cout<<setw(20)<<left<<"Payload Type: "<<"Incremental"<<endl;
			break;
		default:
			cout<<setw(20)<<left<<"Payload Type: "<<"Illegal"<<endl;
			break;

	}


}


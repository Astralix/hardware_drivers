///////////////////////////////////////////////////////////////////////////////
///
/// @file    WifiTPacketReceiver.h
/// 
/// @brief   Defines Wifi Test Manager Packet Receiver Interface
///
/// @info    Created on 31/10/2007
///
/// @author  Radomyslsky Rony
///
/// @version 1.0
///
///

#ifndef WIFITPACKETRECEIVER_H
#define WIFITPACKETRECEIVER_H

///////////////////////////////////////////////////////////////////////////////
// includes.
#include "WmTypes.h"
#include "WifiTestMgr.h"
#include <pthread.h>

class CWifiTestMgrRecvResults;
class CWmVwalWrapper;
///////////////////////////////////////////////////////////////////////////////
class CWifiTPacketReceiver 
{
	

public:

	///////////////////////////////////////////////////////////////////////////
	//Constructor
    CWifiTPacketReceiver();

	///////////////////////////////////////////////////////////////////////////
	//Destructor
	virtual ~CWifiTPacketReceiver();


	int SetMacAddressFilter(IN  const WmMacAddress& macAddr);

	int StartReceive(IN CWmVwalWrapper*  pVwalWrapper,IN uint32 timeout);


	int StopReceive();

	int	GetReceivedPacketStats(OUT CWifiTestMgrRecvResults* pRecvRes);

	void GetReceiveStatus(OUT bool* pReceiving);

	void* RecvProc();

private:
	static void* ThreadProc(void* ptr);
	
	pthread_t	 m_Thread; 

protected:

	void ProcessReceivedPackets(uint32 numRxDesc);
	bool CheckForMacFilter(WmRxWifiPacket& packet);
	bool CheckForErrors(WmRxWifiPacket& packet);

	bool	m_bIsRunning;
	bool 	m_bToRun;
    WmMacAddress	m_filterMac;
	uint32		m_iTimeout;
	bool			m_bToFilterPackets;
	uint32		m_iTotalPacketReceived;
	uint32		m_iFilteredPacketReceived;
	uint32		m_iTotalPacketReceivedWithFCSError;
	uint32		m_iFilteredPacketReceivedWithFCSError;
	double		m_dAvgRSSI_A;
	double		m_dAvgRSSI_B;
	CWmVwalWrapper*	 m_pWmVwalWrapper ;  // Instance of WM Vwal Wrapper 
	WmRxWifiPacket* m_rxDesc;

	




};


#endif //WIFITESTPACKETRECEIVER_H

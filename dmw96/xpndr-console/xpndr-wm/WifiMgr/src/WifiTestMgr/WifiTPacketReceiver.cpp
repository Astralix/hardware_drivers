///////////////////////////////////////////////////////////////////////////////
///
/// @file    WifiTPacketReceiver.h
/// 
/// @brief   Defines Wifi Test Manager Packet Receiver
///
/// @info    Created on 31/10/2007
///
/// @author  Radomyslsky Rony
///
/// @version 1.0
///
////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// includes.
#include "WifiTestMgr.h"
#include "WifiTPacketReceiver.h"
#include "VwalWrapper/WmVwalWrapper.h"
#include <unistd.h>
#include <time.h>
#include "cutils/log.h"

///////////////////////////////////////////////////////////////////////////////
// defines.

#define MAX_NUM_OF_PACKETS_TO_READ	100
#define RECV_PROC_SLEEP_TIME 5

//#include <iostream>


using namespace std;
///////////////////////////////////////////////////////////////////////////////
/// @brief Constructor
CWifiTPacketReceiver::CWifiTPacketReceiver():m_bIsRunning(false),
											m_bToRun(false),
											m_iTimeout(0),
											m_bToFilterPackets(false),
											m_iTotalPacketReceived(0),
											m_iFilteredPacketReceived(0),
											m_iTotalPacketReceivedWithFCSError(0),
											m_iFilteredPacketReceivedWithFCSError(0),
                                            m_pWmVwalWrapper(NULL)
											
{
	m_rxDesc = new WmRxWifiPacket[MAX_NUM_OF_PACKETS_TO_READ];
}

///////////////////////////////////////////////////////////////////////////////
/// @brief Destructor
CWifiTPacketReceiver::~CWifiTPacketReceiver()
{
	if(m_bIsRunning) {

		StopReceive();

	}

	if(m_rxDesc) {

		delete []m_rxDesc;

	}

}


int	CWifiTPacketReceiver::SetMacAddressFilter(IN  const WmMacAddress& macAddr)
{
	if(m_bIsRunning) {

		return -1;

	}

	m_filterMac = macAddr;

	m_bToFilterPackets = true;

	return 0;

}

int CWifiTPacketReceiver::StartReceive(IN CWmVwalWrapper*  pVwalWrapper,IN uint32 timeout)
{
	if(m_bIsRunning) {

		return -1;
	}

	if(pVwalWrapper == NULL) {

		return -2;
	}

	if(m_rxDesc == NULL) {

		return -3;
	}

	m_pWmVwalWrapper = pVwalWrapper;

	m_iTimeout = timeout;

	m_iTotalPacketReceived = 0;
	m_iFilteredPacketReceived = 0;
	m_iTotalPacketReceivedWithFCSError = 0;
    m_iFilteredPacketReceivedWithFCSError = 0;
	m_dAvgRSSI_A = 0.0;
	m_dAvgRSSI_B = 0.0;

	StopReceive();

    

	m_bToRun = true;
	m_bIsRunning = true;
	int threadId = pthread_create(&m_Thread, NULL, &CWifiTPacketReceiver::ThreadProc, this);
	if(threadId != 0) {
	    m_bIsRunning = false;
		return -4;

	}

	return 0;
}


int CWifiTPacketReceiver::StopReceive()
{
	//if( m_pThread )
	//{
		m_bToRun = false;
		//m_pTask = NULL;
		m_bIsRunning = false;
	//}

	return 0;
}


int CWifiTPacketReceiver::GetReceivedPacketStats(OUT CWifiTestMgrRecvResults* pRecvRes)
{
		//At current stage we do not want to let reading stats while
		//receiver proc is running
		if(m_bIsRunning) {

			return -1;
		}

		if(pRecvRes == NULL) {
			return -2;
		}

		pRecvRes->totalPacketsReceived = m_iTotalPacketReceived;

		pRecvRes->filteredPacketsReceived = m_iFilteredPacketReceived;

		pRecvRes->totalReceivedWithoutError = (uint32)(m_iTotalPacketReceived - 
													   m_iTotalPacketReceivedWithFCSError);

		pRecvRes->filteredReceivedWithoutError = (uint32)(m_iFilteredPacketReceived - 
														  m_iFilteredPacketReceivedWithFCSError);

		if(m_bToFilterPackets) {

			if(pRecvRes->filteredReceivedWithoutError > 0) {

				pRecvRes->avgRSSI_A = m_dAvgRSSI_A / (pRecvRes->filteredReceivedWithoutError);
		
				pRecvRes->avgRSSI_B = m_dAvgRSSI_B / (pRecvRes->filteredReceivedWithoutError);
			}
			else
			{
				pRecvRes->avgRSSI_A = 0.0;
				pRecvRes->avgRSSI_B = 0.0;
			}
		}
		else
		{
			if(pRecvRes->totalReceivedWithoutError > 0) {

				pRecvRes->avgRSSI_A = m_dAvgRSSI_A / (pRecvRes->totalReceivedWithoutError);

				pRecvRes->avgRSSI_B = m_dAvgRSSI_B / (pRecvRes->totalReceivedWithoutError);
			}
			else
			{
				pRecvRes->avgRSSI_A = 0.0;
				pRecvRes->avgRSSI_B = 0.0;
			}

		}

		return 0;

}


void CWifiTPacketReceiver::GetReceiveStatus(OUT bool* pReceiving)
{
	*pReceiving = m_bIsRunning;

}
///////////////////////////////////////////////////////////////////////////
// Thread's proc.
// 
void* CWifiTPacketReceiver::ThreadProc(void* ptr)
{
	CWifiTPacketReceiver* pThis = reinterpret_cast<CWifiTPacketReceiver*>(ptr);
	return pThis->RecvProc();

}

void* CWifiTPacketReceiver::RecvProc()
{
   
	if(m_rxDesc == NULL) {
		return NULL;
	}


	EWmStatus status;

	uint8 numRxDesc;

	time_t    startTime,currentTimeMsec,endTimeMsec;

	m_bIsRunning = true;

	if(m_iTimeout > 0) {

		startTime       = time(NULL)                                 ;
		currentTimeMsec = startTime                                  ;
		endTimeMsec     = currentTimeMsec + (m_iTimeout) *1000  ;

	}
    
	while (m_bToRun)
	{

		numRxDesc = MAX_NUM_OF_PACKETS_TO_READ;

		status = m_pWmVwalWrapper->ReceivePackets(m_rxDesc,&numRxDesc);

		if(status != WM_OK) {
		}

		if(numRxDesc == 0) {

			usleep( RECV_PROC_SLEEP_TIME * 1000 );

			//cout << "RcvProc Got " <<(uint32)numRxDesc << " packets" << endl;

			continue;

		}
		else {

		//	cout << "RcvProc Got " <<(uint32)numRxDesc << " packets" << endl;
		}



		ProcessReceivedPackets(numRxDesc);

		if(m_iTimeout > 0)
		{
			currentTimeMsec = time(NULL);

			if(currentTimeMsec > endTimeMsec)
			{
				break;
			}

		}

	}

	m_bIsRunning = false;
	return 0;

}


void CWifiTPacketReceiver::ProcessReceivedPackets(uint32 numRxDesc)
{
	if(numRxDesc == 0) {

		return;
	}

	if(numRxDesc > MAX_NUM_OF_PACKETS_TO_READ)
	{
		numRxDesc = MAX_NUM_OF_PACKETS_TO_READ;
	}

	bool bError;

	for(uint32 packetInd = 0;packetInd < numRxDesc;packetInd++)
	{

		m_iTotalPacketReceived++;

		
		bError = CheckForErrors(m_rxDesc[packetInd]);
		
		if(bError) {

			m_iTotalPacketReceivedWithFCSError++;
		}
		
		if(m_bToFilterPackets)
		{
			
			if(CheckForMacFilter(m_rxDesc[packetInd])) 
			{

				m_iFilteredPacketReceived++;

				if(bError) 
				{
					m_iFilteredPacketReceivedWithFCSError++;
				}
				else
				{
					m_dAvgRSSI_A += (double)(m_rxDesc[packetInd].rssiChannelA);
					m_dAvgRSSI_B += (double)(m_rxDesc[packetInd].rssiChannelB);
				}
			}

		}
		else
		{
			m_iFilteredPacketReceived = m_iTotalPacketReceived;
			m_iFilteredPacketReceivedWithFCSError = m_iTotalPacketReceivedWithFCSError;

			if(!bError) 
			{
				m_dAvgRSSI_A += (double)(m_rxDesc[packetInd].rssiChannelA);
				m_dAvgRSSI_B += (double)(m_rxDesc[packetInd].rssiChannelB);
			}

		}
		
	}

}

bool CWifiTPacketReceiver::CheckForErrors(WmRxWifiPacket& packet)
{

	if( (packet.flags & WM_RXFLAGS_FCS_ERROR) 		||
		(packet.flags & WM_RXFLAGS_LENGTH_EOF_ERROR) ||
		(packet.flags & WM_RXFLAGS_PROTOCOL_ERROR) 	|| 
		(packet.flags & WM_RXFLAGS_DECRYPT_ERROR)	 )
	{

		return true;
	}

	return false;
}

//#include<iostream>
bool CWifiTPacketReceiver::CheckForMacFilter(WmRxWifiPacket& packet)
{
    uint8   FromDS        ;
    uint8*  pSA    = NULL ;

	uint8 FrameCtrl = packet.packet[1];

	FromDS = FrameCtrl & 0x02;


	if(FromDS == 0)
    {
        pSA     = &(packet.packet[10]);
    }
    else
    {
        pSA     = &(packet.packet[16]);
    }

//	cout<<"Filter: "<<std::hex<<(uint32)(m_filterMac.Address[0])<<":"<<(uint32)(m_filterMac.Address[1])<<":"<<(uint32)(m_filterMac.Address[2])<<":"<<(uint32)(m_filterMac.Address[3])<<":"<<(uint32)(m_filterMac.Address[4])<<":"<<(uint32)(m_filterMac.Address[5])<<"\t\t";
//	cout<<"Packet: "<<std::hex<<(uint32)(pSA[0])<<":"<<(uint32)(pSA[1])<<":"<<(uint32)(pSA[2])<<":"<<(uint32)(pSA[3])<<":"<<(uint32)(pSA[4])<<":"<<(uint32)(pSA[5])<<endl;

	for(int addrByte = 0;addrByte < 6;addrByte++) {

		if(m_filterMac.Address[addrByte] != pSA[addrByte]) {

			return false;
		}

	}

	return true;

}

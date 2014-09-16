///////////////////////////////////////////////////////////////////////////////
///
/// @file    WifiTestMgr.cpp
/// 
/// @brief   Implements Wifi Test Manager
///
/// @info    Created on 20/09/2007
///
/// @author  Radomyslsky Rony
///
/// @version 1.0
///
///


///////////////////////////////////////////////////////////////////////////////
// includes.
#include "WifiTestMgr.h"
#include "WifiTPacketReceiver.h"
#include "CRandomPacketGenerator.h"
#include <string>
#ifdef ANDROID
#include "VwalWrapper/WmVwalWrapper.h"
#include <unistd.h>
#endif //ANDROID
//#include <iostream>

#define DW52_RF_REGISTER_OFFSET 256
#define DW52_NUISANCE_PKT_FILTER_REG 198
#define DW52_RATE_USE_LONG_PREAMBLE 0x80

using namespace std;

///////////////////////////////////////////////////////////////////////////////
// Static members.
CWifiTestMgr*		CWifiTestMgr::m_pInstance      = NULL;



CWifiTestMgr::CWifiTestMgr():m_Initialized(false),
						 m_pWmVwalWrapper(NULL)
{

	m_pTxDbgPkt = new WmTxBatchPacket;

	m_pPacketReceiver = new CWifiTPacketReceiver();

	m_isPacketSet = false;

}

/////////////////////////////////////////////////////////////////////////////
///	@brief	GetInstance
///		
CWifiTestMgr* CWifiTestMgr::GetInstance()
{
	// Guards.
//RR???	CInfraGuard<CInfraMutex> autoGuard(m_Mutex);

	if(m_pInstance == NULL)
	{
		m_pInstance = new CWifiTestMgr;
		
	}

//RR???	INFRA_ASSERT(m_pInstance);
	
//RR???	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "CWmCore::GetInstance <----" ) );

	return m_pInstance;

}

///////////////////////////////////////////////////////////////////////////
// @brief   FreeInstance
// 			Frees static member m_pInstance.
void CWifiTestMgr::FreeInstance()        
{

//RR???	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
//RR???				 "CWmCore::FreeInstance ---->" ) );

	// Guards.
//RR???	CInfraGuard<CInfraMutex> autoGuard(m_Mutex);

	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;
		
	}

//RR???	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "CWmCore::FreeInstance <----" ) );

}

///////////////////////////////////////////////////////////////////////////////
///	@brief	Init
/// 
/// 		Initializes WifiTestMgr internal parameters.
/// 		Returns  true  - initialization succeeded.
///					 false - o .w. 
bool CWifiTestMgr::Init()
{

	if(!m_Initialized) {


		m_pWmVwalWrapper = CWmVwalWrapper::GetInstance();

		if(m_pWmVwalWrapper) {

			m_Initialized = true;
		}
		



     }

	return m_Initialized;

}

///////////////////////////////////////////////////////////////////////////////
///	@brief	Terminate
/// 
/// 		Deallocates internal parameters memory.
///
void CWifiTestMgr::Terminate()
{

	CWmVwalWrapper::FreeInstance();
	m_Initialized = false;

}

EWifiTestStatus CWifiTestMgr::SetAutoPhyRateAdoptation(IN  bool  isEnabled)
{ 

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetAutoPhyRateAdoptation(isEnabled);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;


}

EWifiTestStatus CWifiTestMgr::GetAutoPhyRateAdoptation(OUT bool* pIsEnabled) { 

   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetAutoPhyRateAdoptation(pIsEnabled);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

	WmMacAddress pAddr;
	GetMacAddress(&pAddr);


   	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetMaxTxPhyRate(IN  uint32  maxTxPhyRate)
{ 
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->SetMaxTxRate(maxTxPhyRate);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
   
}

EWifiTestStatus CWifiTestMgr::GetMaxTxPhyRate(OUT uint32* pMaxTxPhyRate)
{
			
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetMaxTxRate(pMaxTxPhyRate);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetTxFreq(IN  uint32  txFreq)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}
	
	EWmStatus status = m_pWmVwalWrapper->SetChannelFrequency(txFreq);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetTxFreq(OUT uint32* pTxFreq)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetChannelFrequency(pTxFreq);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetTxPower(IN  uint32  txPower)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->SetMaxTxPower(txPower);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetTxPower(OUT uint32* pTxPower)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetMaxTxPower(pTxPower);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::GeneratePackets(IN  uint32 numOfBursts)
{ 
	
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	if(!m_isPacketSet) {

		return WIFITEST_ERROR;
	}
	EWmStatus status;

   
	m_pTxDbgPkt->numTransmits = numOfBursts;
/*	cout<<"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"<<endl;
  	cout<<"Starting transmit with following settings:"<<endl;
  	cout<<"NumOfTrasmits\t"<<m_pTxDbgPkt->numTransmits<<endl;
  	cout<<"PhyRate\t"<<(uint32)(m_pTxDbgPkt->phyRate)<<endl;
  	cout<<"PowerLevel\t"<<(uint32)(m_pTxDbgPkt->txPowerLevel)<<endl;
  	cout<<"PktLen\t"<<(uint32)(m_pTxDbgPkt->packetSize)<<endl;
    
*/
	status = m_pWmVwalWrapper->SetTxPacket(m_pTxDbgPkt);

//  	cout<<"Set packet for txburst, status: "<<status<<endl;

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}
/*
EWifiTestStatus CWifiTestMgr::SetTxPacket(IN dwMibTxBatch_t* pkt) 
{ 

	if(pkt == NULL) {

		return WIFITEST_ERROR;

	}

	m_pTxDbgPkt->numTransmits = pkt->numTransmits;
	m_pTxDbgPkt->phyRate = pkt->phyRate;
	m_pTxDbgPkt->powerLevel = pkt->powerLevel;
	m_pTxDbgPkt->pktLen = pkt->pktLen;

	memset ( m_pTxDbgPkt->pkt       , 
           0x00                     ,
           sizeof(pkt->pkt)  );

	memcpy(m_pTxDbgPkt->pkt,
		   pkt->pkt,
		   sizeof(pkt->pkt));


EWmStatus status = m_pWmVwalWrapper->SetTxPacket(m_pTxDbgPkt->numTransmits,
												m_pTxDbgPkt->phyRate,
												m_pTxDbgPkt->powerLevel,
												m_pTxDbgPkt->pktLen,
												m_pTxDbgPkt->pkt);

  	cout<<"Set new packet prototype, status: "<<status<<endl;

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	m_isPacketSet = true;

	return WIFITEST_OK;

}
*/
EWifiTestStatus CWifiTestMgr::SetTxPacketParams( IN uint32 numTransmits, 
						 IN uint8  phyRate,      
						 IN uint8	txPowerLevel,
						 IN CWifiTestMgrPacket* pPacketDef)
{ 

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	if(pPacketDef == NULL) {
		return WIFITEST_ERROR;
	}


	CRandomPacketGenerator          rpg;

	/////////////////////////////////////////////////////////
	//RPG - non variant parms settings:
	// 1. MAC addresses.
	/////////////////////////////////////////////////////////
	rpg.setAddresses (pPacketDef->sourceAddress   ,
					  pPacketDef->destAddress  ,
					  pPacketDef->bssId);


	/////////////////////////////////////////////////////////
	// RPG - variant parms settings:
	// 1. PHY rate.
	//    Allow all span with round-robin variation.
	/////////////////////////////////////////////////////////
	CRandomPacketGenerator::ePhyRate  _phyRate;

	_phyRate = rpg.convertPhyRateToEnum ( (uint32)phyRate ) ;

	rpg.setPhyRate (_phyRate   ,   // [IN ] Min PHY rate.           //
					_phyRate   ,   // [IN ] Max PHY rate.           //
					CRandomPacketGenerator::VT_FIXED_AT_MIN ) ; // [IN ] Variation type = None...//

	/////////////////////////////////////////////////////////
	// 2. Packet size.
	//    Random packet size.
	/////////////////////////////////////////////////////////
	rpg.setPacketSize(pPacketDef->packetSize,  // [IN ] Min payload size.             //
					  pPacketDef->packetSize,  // [IN ] Max payload size.             //
					  CRandomPacketGenerator::VT_FIXED_AT_MIN           ); // [IN ] Variation type = None...      //

	/////////////////////////////////////////////////////////
	// 3. TX retry count. 
	//    Incremental (roubd-robin).
	/////////////////////////////////////////////////////////
	rpg.setTxRetry   (pPacketDef->txRetryCount                    ,  // [IN ] Min TX retry (at 0).          //
					  pPacketDef->txRetryCount                    ,  // [IN ] Max TX retry (at Max).        //
					  CRandomPacketGenerator::VT_FIXED_AT_MIN ); // [IN ] No variation.                 //

	/////////////////////////////////////////////////////////
	// QOS parms - copy the EDCA parameters from test set.
	/////////////////////////////////////////////////////////
	rpg.setQoSParms  ( pPacketDef->tid,
					   pPacketDef->aifs  ,
					   pPacketDef->eCWmin ,
					   pPacketDef->eCWmax ) ;


	/////////////////////////////////////////////////////////
	// Announce the test ID to be used in this test.
	/////////////////////////////////////////////////////////
	rpg.setTestID    ( 0 ) ;

	/////////////////////////////////////////////////////////
	// Establish broadcast/multicast
	//
	//*Note - that this is irrespective of the MAC address
	//        used for the transmission!!
	//        We use a method by which the WIFI Driver preps
	//        up a boradcast packet with any DA MAC address 
	//        we give it (DA = Destination Address).
	/////////////////////////////////////////////////////////
	rpg.setBroadcast ( pPacketDef->broadcast ) ;

	rpg.setPowerLevel(txPowerLevel);

	rpg.setPayloadType((CRandomPacketGenerator::ePayloadFillType)(pPacketDef->payloadType));

	string strTestInfo = "This is a test";


	rpg.generateInto (m_pTxDbgPkt,(const void*)(strTestInfo.c_str()),(uint32)(strTestInfo.length())) ;

	m_pTxDbgPkt->numTransmits = numTransmits;

	m_isPacketSet = true;

	return WIFITEST_OK;
	

}

EWifiTestStatus CWifiTestMgr::SetCWTransmission(IN  float freq) { return WIFITEST_OK; }
EWifiTestStatus CWifiTestMgr::GetCWTransmission(OUT float* pFreq) { return WIFITEST_OK; }

EWifiTestStatus CWifiTestMgr::GetAvgRSSI(OUT  uint32* pAvgRSSI)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetAvgRssi(pAvgRSSI);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}



EWifiTestStatus CWifiTestMgr::SetAvgRSSIWindowSize(IN  uint32 avgRSSIWndSize)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->SetAvgRssiWindowSize(avgRSSIWndSize);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetAvgRSSIWindowSize(OUT  uint32* pAvgRSSIWndSize)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetAvgRssiWindowSize(pAvgRSSIWndSize);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::GetAvgRSSIData(OUT  uint32* pAvgRSSIData)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetAvgRssiData(pAvgRSSIData);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetAvgRSSIDataWindowSize(IN  uint32 avgRSSIDataWndSize)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->SetAvgRssiDataWindowSize(avgRSSIDataWndSize);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetAvgRSSIDataWindowSize(OUT  uint32* pAvgRSSIDataWndSize)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetAvgRssiDataWindowSize(pAvgRSSIDataWndSize);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::WriteRegister(IN REGADDR regAddress, IN uint32 value)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->WriteRegister(regAddress, value);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::ReadRegister (IN REGADDR regAddress, OUT uint32* pValue)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->ReadRegister(regAddress, pValue);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::ReadBBRegister (IN ERegisterType regType,
												IN REGADDR regAddress,
												OUT uint8* pValue)
{ 
	  if(!m_Initialized) {
		  return WIFITEST_ERROR;
	  }

	uint32 regAddrOffset = 0;
	EWmStatus status;
	if(regType != REGTYPE_INDIRECT)
	{

		if(regType == REGTYPE_RF)
		{
			regAddrOffset += DW52_RF_REGISTER_OFFSET;
		}

		status = m_pWmVwalWrapper->ReadBBRegister(regAddress + regAddrOffset, pValue);

		if(status != WM_OK) {

			return WIFITEST_ERROR;

		}
	}
	else // Indirect Register
	{

		regAddress -= 128;
		status = m_pWmVwalWrapper->WriteBBRegister(126+256, regAddress);	
		if(status != WM_OK)
			return WIFITEST_ERROR;
	
		status = m_pWmVwalWrapper->ReadBBRegister(127+256, pValue);	
		if(status != WM_OK)
			return WIFITEST_ERROR;
	}
	

	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::WriteBBRegister(IN ERegisterType regType,
											  IN REGADDR regAddress,
											  IN uint8 value)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	uint32 regAddrOffset = 0;
	EWmStatus status;

	if(regType != REGTYPE_INDIRECT)
	{
		if(regType == REGTYPE_RF)
		{
			regAddrOffset += DW52_RF_REGISTER_OFFSET;
		}
		
		status = m_pWmVwalWrapper->WriteBBRegister(regAddress + regAddrOffset, value);
		if(status != WM_OK)
			return WIFITEST_ERROR;

	}
	else // Indirect Register
	{

		regAddress -= 128;
		status = m_pWmVwalWrapper->WriteBBRegister(126+256, regAddress);	
		if(status != WM_OK)
			return WIFITEST_ERROR;
	
		status = m_pWmVwalWrapper->WriteBBRegister(127+256, value);	
		if(status != WM_OK)
			return WIFITEST_ERROR;
	}


	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::WriteBBRegisterField(IN ERegisterType regType,
												   IN REGADDR regAddress,
												   IN uint8 fieldMask,
												   IN uint8 fieldValue)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWifiTestStatus status;
	
	uint8 regVal = 0;

	status = ReadBBRegister(regType, regAddress ,&regVal);
	
	if(status != WIFITEST_OK)
		return WIFITEST_ERROR;

	regVal = (regVal & (0xFF ^ fieldMask)) | ( fieldValue & fieldMask);

	status = WriteBBRegister(regType, regAddress, regVal);

	  if(status != WIFITEST_OK)
		  return WIFITEST_ERROR;
	 

	  return WIFITEST_OK;

}


EWifiTestStatus CWifiTestMgr::WriteBBRegisterBit(IN ERegisterType regType,
												 IN REGADDR regAddress,
												 IN uint8 bitInd,
												 IN uint8 bitValue)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}


	uint8 regVal = 0;

	EWifiTestStatus status = ReadBBRegister(regType, regAddress , &regVal);
	
	if(status != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

	uint8 mask = 1<<bitInd;

	regVal = bitValue ? (regVal | mask) : (regVal & (0xFF^mask));

	status = WriteBBRegister(regType, regAddress, regVal);

	  if(status != WIFITEST_OK) {

		  return WIFITEST_ERROR;

	  }

	  return WIFITEST_OK;

}



EWifiTestStatus CWifiTestMgr::ReadBBRegisterField (IN ERegisterType regType,
												   IN REGADDR regAddress,
												   IN uint8 fieldMask,
												   OUT uint8* pFieldValue)
{ 

	if(!m_Initialized) 
	{
		  return WIFITEST_ERROR;
	}

	uint8 regVal = 0;

	EWifiTestStatus status = ReadBBRegister(regType, regAddress, &regVal);

	if(status != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

	*pFieldValue = (regVal & fieldMask);

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::ReadBBRegisterBit (IN ERegisterType regType,
												 IN REGADDR regAddress,
												 IN uint8 bitInd,
												 OUT uint8* pBitValue)
{ 

	uint8 mask = 1<<bitInd;

	EWifiTestStatus status = ReadBBRegisterField(regType,regAddress,mask,pBitValue);

	if(status == WIFITEST_OK)
	{
		if(*pBitValue > 0)
		{
			*pBitValue = 1;
		}

		return WIFITEST_OK;
	}
	else
	{
		return WIFITEST_ERROR;
	}
}


EWifiTestStatus CWifiTestMgr::SaveBBRegistersToFile(IN ERegisterType regType,
													IN char* fileName)
{
	 return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::LoadBBRegistersFromFile(IN ERegisterType regType,
													  IN char* fileName)
{ 
	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::UploadBBRegistersFile(IN ERegisterType regType,
													IN char* fileName)
{ 
	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::DownloadBBRegistersFile(IN ERegisterType regType,
													  IN char* fileName)
{ 
	return WIFITEST_OK;
}

//EWifiTestStatus CWifiTestMgr::WriteRFRegister(IN uint8 regAddress, IN BYTE value) { return WIFITEST_OK; }
//EWifiTestStatus CWifiTestMgr::ReadRFRegister (IN uint8 regAddress, OUT BYTE* pValue) { return WIFITEST_OK; }


//EWifiTestStatus CWifiTestMgr::SaveRFRegistersToFile(IN char* fileName) { return WIFITEST_OK; }
//EWifiTestStatus CWifiTestMgr::LoadRFRegistersFromFile(IN char* fileName) { return WIFITEST_OK; }

//EWifiTestStatus CWifiTestMgr::UploadRFRegistersFile(IN char* fileName) { return WIFITEST_OK; }
//EWifiTestStatus CWifiTestMgr::DownloadRFRegistersFile(IN char* fileName) { return WIFITEST_OK; }

EWifiTestStatus CWifiTestMgr::SetDFSSelectedChannel(IN  BYTE channelNumber) { return WIFITEST_OK; }
EWifiTestStatus CWifiTestMgr::GetDFSSelectedChannel(OUT BYTE* pChannelNumber) { return WIFITEST_OK; }

EWifiTestStatus CWifiTestMgr::SetDFSEvents(IN   bool eventsEnabled) { return WIFITEST_OK; }
EWifiTestStatus CWifiTestMgr::GetDFSEvents(OUT  bool* pEventsEnabled) { return WIFITEST_OK; }

EWifiTestStatus CWifiTestMgr::SetPwrSave(IN  bool  isPsEnabled)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status;

	if(isPsEnabled)
	{
		status = m_pWmVwalWrapper->EnablePs();
	}
	else
	{
		status = m_pWmVwalWrapper->DisablePs();
	}
	

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}
EWifiTestStatus CWifiTestMgr::GetPwrSave(OUT bool* pIsPsEnabled)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetPs(pIsPsEnabled);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::SetPwrSaveAction(IN  EWmPsAction pwrSaveAction)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetPsAction(pwrSaveAction);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}
EWifiTestStatus CWifiTestMgr::GetPwrSaveAction(OUT EWmPsAction* pPwrSaveAction)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetPsAction(pPwrSaveAction);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetPsMode( IN uint32 psdMode)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetPsMode(psdMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;


}

EWifiTestStatus CWifiTestMgr::GetPsMode( uint32* pPsMode)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetPsMode(pPsMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::GetTMdata(OUT WmMibTMdata* pWmMibTMdata) 
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetTMdata(pWmMibTMdata);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetNuisancePacketFilter(IN  bool  filterEnabled)
{ 

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status;

	uint8 data;

	status = m_pWmVwalWrapper->ReadBBRegister(DW52_NUISANCE_PKT_FILTER_REG, &data);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	//we need to update LSB of 198 register
	if(filterEnabled) {

		data = (data | 0x01);
	}
	else {

		data = (data & 0xFE);
	}

	status = m_pWmVwalWrapper->WriteBBRegister(DW52_NUISANCE_PKT_FILTER_REG,data);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetNuisancePacketFilter(OUT bool* pFilterEnabled)
{ 

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status;

	uint8 data;

	status = m_pWmVwalWrapper->ReadBBRegister(DW52_NUISANCE_PKT_FILTER_REG, &data);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	if(data & 0x01) {

		*pFilterEnabled = true;
	}
	else {

		*pFilterEnabled = false;
	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetAntennaDiversityMode(IN  uint32  diversityMode)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWifiTestStatus intern_status  = StopDriver();


	if (intern_status != WIFITEST_OK ) 
	{
		return WIFITEST_ERROR;

	}

	EWmStatus status = m_pWmVwalWrapper->SetAntennaDiversity(diversityMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	intern_status = StartDriver();

	if (intern_status != WIFITEST_OK ) 
	{
		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::GetAntennaDiversityMode(OUT uint32* pDiversityMode)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetAntennaDiversity(pDiversityMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::GetMacAddress( INOUT WmMacAddress* pAddr) 
{
    if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetMacAddress(pAddr);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetChannel(IN  uint32  channel)
{ 
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->SetChannel(channel);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetChannel(OUT uint32* pChannel)
{

   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetChannel(pChannel);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetDriverMode(IN uint32 mode)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetDriverMode(mode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;


}

EWifiTestStatus CWifiTestMgr::GetDriverMode(OUT uint32* pMode)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetDriverMode(pMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;


}

EWifiTestStatus CWifiTestMgr::GetConnectionState( OUT EWmConnectionState* 
										                 pConnectionState )
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetConnectionState(pConnectionState);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::StopDriver() 
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->StopDriver();

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	int numOfWaitIter = 0;
	EWmConnectionState ConnectionState;

	while(numOfWaitIter < 10)
	{
		status = m_pWmVwalWrapper->GetConnectionState(&ConnectionState);

		if(status != WM_OK) {

			return WIFITEST_ERROR;

		}

		if(ConnectionState == WM_STATE_IDLE)
		{
			break;
		}

		usleep(50000);

		numOfWaitIter++;

	}

	if(numOfWaitIter == 10)
	{
		return WIFITEST_ERROR;
	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::StartDriver() 
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

    uint32 TxFreq = 0;

	if( GetTxFreq(&TxFreq) != WIFITEST_OK)
	{
		return WIFITEST_ERROR;
	}

	if(TxFreq == 0) 
	{
		if (SetTxFreq(2412)!= WIFITEST_OK)
		{
			return WIFITEST_ERROR;
		}
	}

	EWmStatus status = m_pWmVwalWrapper->StartDriver();

	if(status != WM_OK) 
	{
		return WIFITEST_ERROR;
	}

	int numOfWaitIter = 0;
	EWmConnectionState ConnectionState;

	while(numOfWaitIter < 10)
	{
		status = m_pWmVwalWrapper->GetConnectionState(&ConnectionState);

		if(status != WM_OK) {

			return WIFITEST_ERROR;

		}

		if(ConnectionState != WM_STATE_IDLE)
		{
			break;
		}

		usleep(50000);

		numOfWaitIter++;

	}

	if(numOfWaitIter == 10)
	{
		return WIFITEST_ERROR;
	}

	return WIFITEST_OK;

}


EWifiTestStatus CWifiTestMgr::GenerateTxTone(uint8 Power)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}
	uint8 Channel;

	EWmStatus status = m_pWmVwalWrapper->ReadBBRegister(256+12, &Channel);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	
	status = m_pWmVwalWrapper->WriteRegister(0xFFFFFE3D,1);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	//uint8 Power   = strtoul(argv[2], NULL, 0);

	uint8 AGain        =(Power & 0x1F);
	uint8 FixedLNAGain =(Power & 0x20) << 1;
	uint8 FixedGain    = 0x80;
	uint8 Reg80        = FixedGain | FixedLNAGain; // combine fields

	//enable +10 MHz tone generator (also turns off any test mode) 
	status = m_pWmVwalWrapper->WriteBBRegister(7,0x0A);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	// set InitAGain = Power
	status = m_pWmVwalWrapper->WriteBBRegister(65, AGain);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	// fixed gain (keep Power on LNAGAIN/AGAIN[4:0])
	status = m_pWmVwalWrapper->WriteBBRegister( 80, Reg80);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	 // radio is always in TX mode (regardless of baseband mode) 
	status = m_pWmVwalWrapper->WriteBBRegister(224, 0xAA );

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	// baseband DACs are always on 
	status = m_pWmVwalWrapper->WriteBBRegister(225, 0x3F );

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	 // SW1 in TX position (always) 
	status = m_pWmVwalWrapper->WriteBBRegister(228, 0x0F );

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}
	 // turn on PA (5 GHz for channel<241, 2.4 GHz otherwise)
	status = m_pWmVwalWrapper->WriteBBRegister(230, Channel<241 ? 1 : 4);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::StartReceive(IN  WmMacAddress* pAddr,IN uint32 timeout)
{

	if(!m_Initialized) {

		return WIFITEST_ERROR;

	}

	if(m_pPacketReceiver == NULL)
	{
		return WIFITEST_ERROR;
	}

	bool isReceiveProcActive;

	m_pPacketReceiver->GetReceiveStatus(&isReceiveProcActive);

	if(isReceiveProcActive) {

		//cout<<"Receiver is already active"<<endl;

		return WIFITEST_ERROR;
	}

	if(pAddr != NULL) {

		if(m_pPacketReceiver->SetMacAddressFilter(*pAddr) < 0) 
		{
			return WIFITEST_ERROR;
		}
	}

	if(m_pPacketReceiver->StartReceive(m_pWmVwalWrapper,timeout) < 0) 
	{

		return WIFITEST_ERROR;
	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::StopReceive(OUT CWifiTestMgrRecvResults* pRecvRes)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

	if(m_pPacketReceiver == NULL)
	{
		return WIFITEST_ERROR;
	}

	if(pRecvRes == NULL)
	{
		return WIFITEST_ERROR;
	}

   	if(m_pPacketReceiver->StopReceive() < 0) 
   	{
   		return WIFITEST_ERROR;
   	}

   	if(!m_pPacketReceiver->GetReceivedPacketStats(pRecvRes) < 0)
   	{
   		return WIFITEST_ERROR;
   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::GetReceiveStatus(OUT bool* pReceiving)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	if(m_pPacketReceiver == NULL)
	{
		return WIFITEST_ERROR;
	}

	m_pPacketReceiver->GetReceiveStatus(pReceiving);

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetMib(IN uint32 mibCode, IN uint32 value)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetMib(mibCode, value);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetMib(IN uint32 mibCode, OUT uint32* pValue)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetMib(mibCode, pValue);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}


EWifiTestStatus CWifiTestMgr::SetMib(IN uint32 mibCode, IN  WmMacAddress* pWmMacAddr)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetMib(mibCode, pWmMacAddr);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetMib(IN uint32 mibCode, OUT  WmMacAddress* pWmMacAddr)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetMib(mibCode, pWmMacAddr);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetMib(IN uint32 mibCode, IN  WmSsid* pWmSsid)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetMib(mibCode, pWmSsid);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetMib(IN uint32 mibCode, OUT  WmSsid* pWmSsid)
{ 
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetMib(mibCode, pWmSsid);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetTxPowerLevelAll(IN  uint8  txPower)
{

	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetTxPowerLevelAll(txPower);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetTxPowerLevel(IN uint16 freq,
											  IN uint8 rate,
											  OUT uint8* pTxPower)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

	uint8 freqIndex;

    if(GetFreqIndex(freq,&freqIndex) != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

	uint8 rateIndex;

	bool longPreamble = rate & DW52_RATE_USE_LONG_PREAMBLE;

	rate = rate & ~DW52_RATE_USE_LONG_PREAMBLE;

    if(GetRateIndex(rate,longPreamble,&rateIndex) != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

   	return GetTxPowerLevelByIndex(freqIndex,
								  rateIndex,
								  pTxPower);
}

EWifiTestStatus CWifiTestMgr::GetTxPowerLevelByIndex(IN uint8 freqIndex,
													 IN uint8 rateIndex,
													 OUT uint8* pTxPower)
{
   	if(!m_Initialized) {
   		return WIFITEST_ERROR;
   	}

   	EWmStatus status = m_pWmVwalWrapper->GetTxPowerLevel(freqIndex,
   													rateIndex,
   													pTxPower);

   	if(status != WM_OK) {

   		return WIFITEST_ERROR;

   	}

   	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::SetTxPowerLevelByIndex(IN uint8 freqIndex,
													 IN uint8 rateIndex,
													 IN uint8 txPower)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetTxPowerLevel(freqIndex,
													rateIndex,
													txPower);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetTxPowerLevel(IN uint16 freq,
											  IN uint8 rate,
											  IN uint8 txPower)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	uint8 freqIndex;

    if(GetFreqIndex(freq,&freqIndex) != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

	uint8 rateIndex;

	bool longPreamble = rate & DW52_RATE_USE_LONG_PREAMBLE;

	rate = rate & ~DW52_RATE_USE_LONG_PREAMBLE;

	if(GetRateIndex(rate,longPreamble,&rateIndex) != WIFITEST_OK) {

		return WIFITEST_ERROR;

	}

	return SetTxPowerLevelByIndex(freqIndex,
								  rateIndex,
								  txPower);
}


EWifiTestStatus CWifiTestMgr::SetTxPowerLevelTable(IN WmPowerLevelTable* pTxPowerLevelTable)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetTxPowerLevelTable(pTxPowerLevelTable);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::GetTxPowerLevelTable(OUT WmPowerLevelTable* pTxPowerLevelTable)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetTxPowerLevelTable(pTxPowerLevelTable);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetPhyParameters(IN WmMibPhyParameter* pMibPhyParameter)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetPhyParameters(pMibPhyParameter);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus
	CWifiTestMgr::GetScanChanFreqList(OUT WmScanChanFreqList* pScanChanFreqList)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->GetScanChanFreqList(pScanChanFreqList);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus
	CWifiTestMgr::SetScanChanFreqList(IN WmScanChanFreqList* pScanChanFreqList)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetScanChanFreqList(pScanChanFreqList);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::SetScanChannelFreq(IN uint32 channelFreq)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->SetScanChannelFreq(channelFreq);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}

EWifiTestStatus CWifiTestMgr::AddChannelToScanFreqList(IN uint32 channelFreq)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	EWmStatus status = m_pWmVwalWrapper->AddChannelToScanFreqList(channelFreq);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;

}
/*
EWifiTestStatus CWifiTestMgr::StartTriggerMech(IN uint32 freq)
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	uint32 drvMode;

	EWmStatus status = m_pWmVwalWrapper->GetDriverMode(&drvMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;
	}

	if(drvMode != 1) {

		return WIFITEST_ERROR;
	}

	bool isPsEnabled;

	status = m_pWmVwalWrapper->GetPs(&isPsEnabled);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	if(isPsEnabled == false) 
	{
		return WIFITEST_ERROR;
	}


	EWmConnectionState ConnectionState;

	status = m_pWmVwalWrapper->GetConnectionState(&ConnectionState);

	if(status != WM_OK) {

			return WIFITEST_ERROR;

	}

	if(ConnectionState != WM_STATE_CONNECTED &&
	   ConnectionState != WM_STATE_CONNECTED_AND_SCANNING)
	{
		return WIFITEST_ERROR;
	}

	status = 
		m_pTrafficMgr->WmTrafficMgrConfTriggerMech(WM_TRIGGER_MECH_MANUAL,freq);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}

EWifiTestStatus CWifiTestMgr::StopTriggerMech()
{
	if(!m_Initialized) {
		return WIFITEST_ERROR;
	}

	uint32 drvMode;

	EWmStatus status = m_pWmVwalWrapper->GetDriverMode(&drvMode);

	if(status != WM_OK) {

		return WIFITEST_ERROR;
	}

	if(drvMode != 1) {

		return WIFITEST_ERROR;
	}

	bool isPsEnabled;

	status = m_pWmVwalWrapper->GetPs(&isPsEnabled);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	if(isPsEnabled == false) 
	{
		return WIFITEST_ERROR;
	}


	EWmConnectionState ConnectionState;

	status = m_pWmVwalWrapper->GetConnectionState(&ConnectionState);

	if(status != WM_OK) {

			return WIFITEST_ERROR;

	}

	if(ConnectionState != WM_STATE_CONNECTED &&
	   ConnectionState != WM_STATE_CONNECTED_AND_SCANNING)
	{
		return WIFITEST_ERROR;
	}

	status = 
		m_pTrafficMgr->WmTrafficMgrConfTriggerMech(	WM_TRIGGER_MECH_DISABLED,1);

	if(status != WM_OK) {

		return WIFITEST_ERROR;

	}

	return WIFITEST_OK;
}
*/
EWifiTestStatus CWifiTestMgr::GetBoardId(OUT uint8* pBoardId)
{
	ERegisterType regType = REGTYPE_RF;

	//Sets Register 94 bit to 204
	uint32 iRegAddr = 94;
	uint8 iRegByteValue  = 204;

	EWifiTestStatus status = 
		WriteBBRegister(regType,iRegAddr,iRegByteValue);

	if(status != WIFITEST_OK) 
	{
		return status;
	}

	//Sets Register 4 bit to 3
	iRegAddr = 4;
	iRegByteValue  = 3;

	status = WriteBBRegister(regType,iRegAddr,iRegByteValue);

	if(status != WIFITEST_OK) 
	{
		return status;
	}

	//Sets Register 90 to 34
	iRegAddr = 90;
    iRegByteValue  = 34;

	status = WriteBBRegister(regType,iRegAddr,iRegByteValue);

	if(status != WIFITEST_OK) 
	{
		return status;
	}

	//Sets Register 95 to 153
	iRegAddr = 95;
    iRegByteValue  = 153;

	status = WriteBBRegister(regType,iRegAddr,iRegByteValue);

	if(status != WIFITEST_OK) 
	{
		return status;
	}

    //Read Register 127 bit 0:3 - now holds board ID 
	iRegAddr = 127;
	uint32 fieldMask = 15;
	*pBoardId = 0;

	status = ReadBBRegisterField(regType,iRegAddr,(uint8)fieldMask,pBoardId);

	return status;

}

EWifiTestStatus CWifiTestMgr::GetFreqIndex(IN uint16 freq,OUT uint8* pFreqIndex )
{
	EWifiTestStatus status = WIFITEST_OK;

	switch (freq)
    {
    case 2412:  *pFreqIndex = 0; break;
    case 2417:  *pFreqIndex = 1; break;
    case 2422:  *pFreqIndex = 2; break;
    case 2427:  *pFreqIndex = 3; break;
    case 2432:  *pFreqIndex = 4; break;
    case 2437:  *pFreqIndex = 5; break;
    case 2442:  *pFreqIndex = 6; break;
    case 2447:  *pFreqIndex = 7; break;
    case 2452:  *pFreqIndex = 8; break;
    case 2457:  *pFreqIndex = 9; break;
    case 2462:  *pFreqIndex = 10; break;
    case 2467:  *pFreqIndex = 11; break;
    case 2472:  *pFreqIndex = 12; break;
    case 2484:  *pFreqIndex = 13; break;
    case 5040:  *pFreqIndex = 14; break;
    case 5060:  *pFreqIndex = 15; break;
    case 5080:  *pFreqIndex = 16; break;
    case 5170:  *pFreqIndex = 17; break;
    case 5180:  *pFreqIndex = 18; break;
    case 5190:  *pFreqIndex = 19; break;
    case 5200:  *pFreqIndex = 20; break;
    case 5210:  *pFreqIndex = 21; break;
    case 5220:  *pFreqIndex = 22; break;
    case 5230:  *pFreqIndex = 23; break;
    case 5240:  *pFreqIndex = 24; break;
    case 5260:  *pFreqIndex = 25; break;
    case 5280:  *pFreqIndex = 26; break;
    case 5300:  *pFreqIndex = 27; break;
    case 5320:  *pFreqIndex = 28; break;
    case 5500:  *pFreqIndex = 29; break;
    case 5520:  *pFreqIndex = 30; break;
    case 5540:  *pFreqIndex = 31; break;
    case 5560:  *pFreqIndex = 32; break;
    case 5580:  *pFreqIndex = 33; break;
    case 5600:  *pFreqIndex = 34; break;
    case 5620:  *pFreqIndex = 35; break;
    case 5640:  *pFreqIndex = 36; break;
    case 5660:  *pFreqIndex = 37; break;
    case 5680:  *pFreqIndex = 38; break;
    case 5700:  *pFreqIndex = 39; break;
    case 5745:  *pFreqIndex = 40; break;
    case 5765:  *pFreqIndex = 41; break;
    case 5785:  *pFreqIndex = 42; break;
    case 5805:  *pFreqIndex = 43; break;
    case 4920:  *pFreqIndex = 44; break;
    case 4940:  *pFreqIndex = 45; break;
    case 4960:  *pFreqIndex = 46; break;
    case 4980:  *pFreqIndex = 47; break;
    default:    *pFreqIndex = 0; status = WIFITEST_ERROR; break;
    }

	return status;
}

EWifiTestStatus CWifiTestMgr::GetRateIndex(IN uint8 rate,
										  bool longPreamble,
										  OUT uint8* pRateIndex )
{
	EWifiTestStatus status = WIFITEST_OK;

	switch (rate) 
	{
		case 1:		*pRateIndex = 0; break;
		case 2:		*pRateIndex = (longPreamble? 1 : 2); break;
		case 55:	*pRateIndex = (longPreamble? 3 : 4); break;
		case 11:	*pRateIndex = (longPreamble? 5 : 6); break;
        case  6 :	*pRateIndex = 7; break;
        case  9 :	*pRateIndex = 8; break;
        case 12 :	*pRateIndex = 9; break;
        case 18 :	*pRateIndex = 10; break;
        case 24 :	*pRateIndex = 11; break;
        case 36 :	*pRateIndex = 12; break;
        case 48 :	*pRateIndex = 13; break;
        case 54 :	*pRateIndex = 14; break;
        default :	*pRateIndex = 0; status = WIFITEST_ERROR; break;
    }

	return status;
}



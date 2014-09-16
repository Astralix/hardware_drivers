///////////////////////////////////////////////////////////////////////////////
///
/// @file    WifiTestMgr.h
/// 
/// @brief   Defines Wifi Test Manager Interface
///
/// @date   20/09/2007
///
/// @author  Radomyslsky Rony
///
/// @version 1.0
///
///

#ifndef WIFITESTMGR_H
#define WIFITESTMGR_H

///////////////////////////////////////////////////////////////////////////////
// includes.
#include "TypeDefs.h"

#ifndef ANDROID
#include "WifiMgr.h"
#else
#include "WmTypes.h"
#endif //ANDROID

#ifndef BYTE
	#define BYTE uint8
#endif

#ifndef REGADDR 
	#define REGADDR uint32
#endif

enum EWifiTestStatus
{
	WIFITEST_OK                    = 0,
	WIFITEST_ERROR	             = 13 //Driver Error.
};

enum ERegisterType
{
	REGTYPE_BB = 0,
	REGTYPE_RF,
	REGTYPE_INDIRECT
	

};

struct CWifiTestMgrRecvResults 
{
	uint32 totalPacketsReceived;
	uint32 filteredPacketsReceived;
	uint32 totalReceivedWithoutError;
	uint32 filteredReceivedWithoutError;
	double avgRSSI_A;
	double avgRSSI_B;
};

struct CWifiTestMgrPacket
{
   uint8   sourceAddress[6] ;
   uint8   destAddress  [6] ;
   uint8   bssId        [6] ;
   bool    broadcast	    ;
   uint16  packetSize  	    ;      
   uint8   txRetryCount     ;
   uint8   tid    		    ; // [IN ] TID to put in QoS header.
   uint8   aifs   		    ; // [IN ] AIFS used in this TID (mapped to AC).
   uint8   eCWmin 		    ; // [IN ] (log2 of) CWmin used for this TID.
   uint8   eCWmax           ;
   uint8   payloadType      ;
};

class CWifiTPacketReceiver;
#ifdef ANDROID
class CWmVwalWrapper;
#endif //ANDROID

class CWifiTestMgr
{
public:


	///////////////////////////////////////////////////////////////////////////
	// @brief   GetInstance
	// 			Returns an Instance of this class
	static CWifiTestMgr* GetInstance();

	///////////////////////////////////////////////////////////////////////////
	// @brief   FreeInstance
	// 			Frees static member m_pInstance.
	static void FreeInstance();

	///////////////////////////////////////////////////////////////////////////
	///	@brief	Terminate
	/// 
	/// 		Deallocates WifiTestMgr (dynamically memory allocated) internal 
	/// 		parameters.
	//
	virtual void  Terminate(); 	

	///////////////////////////////////////////////////////////////////////////
	///	@brief	Init
	/// 
	/// 		Initializes WifiTestMgr internal parameters,
	/// 		Returns  true .
	///		
	virtual bool  Init(); 	



	EWifiTestStatus SetAutoPhyRateAdoptation(IN  bool  isEnabled);
	EWifiTestStatus GetAutoPhyRateAdoptation(OUT bool* pIsEnabled);

	EWifiTestStatus SetMaxTxPhyRate(IN  uint32  maxTxPhyRate);
	EWifiTestStatus GetMaxTxPhyRate(OUT uint32* pMaxTxPhyRate);

	EWifiTestStatus SetTxFreq(IN  uint32  txFreq);
	EWifiTestStatus GetTxFreq(OUT uint32* pTxFreq);

	EWifiTestStatus SetTxPower(IN  uint32  txPower);
	EWifiTestStatus GetTxPower(OUT uint32* pTxPower);

	EWifiTestStatus GeneratePackets(IN  uint32 numOfBursts);

	EWifiTestStatus SetTxPacketParams(IN uint32 numTransmits, 
                                      IN uint8  phyRate,      
                                      IN uint8	txPowerLevel,
                                      IN CWifiTestMgrPacket* pPacketDef);


	EWifiTestStatus SetCWTransmission(IN  float freq);
	EWifiTestStatus GetCWTransmission(OUT float* pFreq);

	EWifiTestStatus GetAvgRSSI(OUT  uint32* pAvgRSSI);

	EWifiTestStatus SetAvgRSSIWindowSize(IN  uint32 avgRSSIWndSize);
	EWifiTestStatus GetAvgRSSIWindowSize(OUT  uint32* pAvgRSSIWndSize);

	EWifiTestStatus GetAvgRSSIData(OUT  uint32* pAvgRSSIData);

	EWifiTestStatus SetAvgRSSIDataWindowSize(IN  uint32 avgRSSIDataWndSize);
	EWifiTestStatus GetAvgRSSIDataWindowSize(OUT  uint32* pAvgRSSIDataWndSize);


	EWifiTestStatus WriteRegister(IN REGADDR regAddress, IN uint32 value);
	EWifiTestStatus ReadRegister (IN REGADDR regAddress, OUT uint32* pValue);

	EWifiTestStatus WriteBBRegister(IN ERegisterType regType,
									IN REGADDR regAddress,
									IN uint8 value);


	EWifiTestStatus WriteBBRegisterField(IN ERegisterType regType,
									     IN REGADDR regAddress,
									     IN uint8 fieldMask,
									     IN uint8 fieldValue);


	EWifiTestStatus WriteBBRegisterBit(IN ERegisterType regType,
									   IN REGADDR regAddress,
									   IN uint8 bitInd,
									   IN uint8 bitValue);


	EWifiTestStatus ReadBBRegister (ERegisterType regType,
									IN REGADDR regAddress,
									OUT uint8* pValue);

	EWifiTestStatus ReadBBRegisterField (IN ERegisterType regType,
										 IN REGADDR regAddress,
										 IN uint8 fieldMask,
										 OUT uint8* pFieldValue);

	EWifiTestStatus ReadBBRegisterBit (IN ERegisterType regType,
									   IN REGADDR regAddress,
									   IN uint8 bitInd,
									   OUT uint8* pBitValue);


//	EWifiTestStatus WriteRFRegister(IN uint8 regAddress, IN BYTE value);
//	EWifiTestStatus ReadRFRegister (IN uint8 regAddress, OUT BYTE* pValue);

//------------------------------------------------------------------------------
	//Will be implemented over Write/ReadBBRegisters
	EWifiTestStatus SaveBBRegistersToFile(IN ERegisterType regType,
										  IN char* fileName);

	EWifiTestStatus LoadBBRegistersFromFile(IN ERegisterType regType,
											IN char* fileName);

//	EWifiTestStatus SaveRFRegistersToFile(IN char* fileName);
//	EWifiTestStatus LoadRFRegistersFromFile(IN char* fileName);

	EWifiTestStatus UploadBBRegistersFile(IN ERegisterType regType,
										  IN char* fileName);

	EWifiTestStatus DownloadBBRegistersFile(IN ERegisterType regType,
											IN char* fileName);

//	EWifiTestStatus UploadRFRegistersFile(IN char* fileName);
//	EWifiTestStatus DownloadRFRegistersFile(IN char* fileName);
//-------------------------------------------------------------------------------

	EWifiTestStatus SetDFSSelectedChannel(IN  BYTE channelNumber);
	EWifiTestStatus GetDFSSelectedChannel(OUT BYTE* pChannelNumber);

	EWifiTestStatus SetDFSEvents(IN   bool eventsEnabled);
	EWifiTestStatus GetDFSEvents(OUT  bool* pEventsEnabled);

	EWifiTestStatus SetPwrSave(IN  bool  isPsEnabled);
	EWifiTestStatus GetPwrSave(OUT bool* pIsPsEnabled);

	EWifiTestStatus SetPwrSaveAction(IN  EWmPsAction  pwrSaveAction);
	EWifiTestStatus GetPwrSaveAction(OUT EWmPsAction* pPwrSaveAction);

	EWifiTestStatus SetPsMode( IN uint32 psdMode);

	EWifiTestStatus GetPsMode( OUT uint32* pPsMode);
	EWifiTestStatus GetTMdata(OUT WmMibTMdata* pWmMibTMdata); 


	EWifiTestStatus SetNuisancePacketFilter(IN  bool  filterEnabled);
	EWifiTestStatus GetNuisancePacketFilter(OUT bool* pFilterEnabled);

	EWifiTestStatus SetAntennaDiversityMode(IN  uint32  diversityMode);
	EWifiTestStatus GetAntennaDiversityMode(OUT uint32* pDiversityMode);
//--------------------------------------------------------------------------
	EWifiTestStatus GetMacAddress( INOUT WmMacAddress* pAddr);


	EWifiTestStatus SetChannel(IN  uint32  channel);
	EWifiTestStatus GetChannel(OUT uint32* pChannel);

	EWifiTestStatus StartDriver();
	EWifiTestStatus StopDriver();

	EWifiTestStatus GenerateTxTone(uint8 Power);

	EWifiTestStatus SetDriverMode(IN uint32 mode);
	EWifiTestStatus GetDriverMode(OUT uint32* mode);

	EWifiTestStatus GetConnectionState( OUT EWmConnectionState* 
										                 pConnectionState );

	EWifiTestStatus StartReceive(IN  WmMacAddress* pAddr,IN uint32 timeout);

	EWifiTestStatus StopReceive(OUT CWifiTestMgrRecvResults* pRecvRes);

	EWifiTestStatus GetReceiveStatus(OUT bool* pReceiving);

	EWifiTestStatus SetMib(IN uint32 mibCode, IN uint32 value);
	EWifiTestStatus GetMib(IN uint32 mibCode, OUT uint32* pValue);

	EWifiTestStatus SetMib(IN uint32 mibCode, IN  WmMacAddress* pWmMacAddr);
	EWifiTestStatus GetMib(IN uint32 mibCode, OUT WmMacAddress* pWmMacAddr);

	EWifiTestStatus SetMib(IN uint32 mibCode, IN  WmSsid* pWmSsid);
	EWifiTestStatus GetMib(IN uint32 mibCode, OUT WmSsid* pWmSsid);


	EWifiTestStatus SetTxPowerLevelAll(IN  uint8  txPower);


	EWifiTestStatus GetTxPowerLevelByIndex(IN uint8 freqIndex,
										   IN uint8 rateIndex,
										   OUT uint8* pTxPower);

	EWifiTestStatus GetTxPowerLevel(IN uint16 freq,
								    IN uint8 rate,
									OUT uint8* pTxPower);

	EWifiTestStatus SetTxPowerLevelByIndex(IN uint8 freqIndex,
										   IN uint8 rateIndex,
										   IN uint8 txPower);

	EWifiTestStatus SetTxPowerLevel(IN uint16 freq,
									IN uint8 rate,
									IN uint8 txPower);

	EWifiTestStatus SetTxPowerLevelTable(IN WmPowerLevelTable* pTxPowerLevelTable);

	EWifiTestStatus GetTxPowerLevelTable(OUT WmPowerLevelTable* pTxPowerLevelTable);

	EWifiTestStatus SetPhyParameters(IN WmMibPhyParameter* pMibPhyParameter);

	EWifiTestStatus
		GetScanChanFreqList(OUT WmScanChanFreqList* pScanChanFreqList);

	EWifiTestStatus
		SetScanChanFreqList(IN WmScanChanFreqList* pScanChanFreqList);

	EWifiTestStatus SetScanChannelFreq(IN uint32 channelFreq);

	EWifiTestStatus AddChannelToScanFreqList(IN uint32 channelFreq);
#ifndef ANDROID
	EWifiTestStatus StartTriggerMech(IN uint32 freq);

	EWifiTestStatus StopTriggerMech();
#endif //ANDROID
	EWifiTestStatus GetBoardId(OUT uint8* pBoardId);

    
private:

	///////////////////////////////////////////////////////////////////////////
	// Private functions - promises a singleton behavior

	// private constructor
	CWifiTestMgr();

	// private copy constructor
	CWifiTestMgr(const CWifiTestMgr&) {}; 
#ifdef ANDROID
	// private constructor
	virtual ~CWifiTestMgr(){};

#endif //ANDROID
	// private '=' operator 
	void operator=(const CWifiTestMgr&){};

	EWifiTestStatus GetFreqIndex(IN uint16 freq,
								 OUT uint8* pFreqIndex );

	EWifiTestStatus GetRateIndex(IN uint8 rate,
							     bool longPreamble,
								 OUT uint8* pRateIndex );



	///////////////////////////////////////////////////////////////////////////
	// Private members

	static CWifiTestMgr*		m_pInstance      ;  //instance to promises 
												// a single instance of this 
												// class.
#ifndef ANDROID
	IWifiMgr*     			m_pWifiMgr;

	IWmDebugMgr*  			m_pDebugMgr;

	IWmTrafficMgr*  		m_pTrafficMgr;
#else
	CWmVwalWrapper*		    m_pWmVwalWrapper ;  // Instance of WM Vwal Wrapper 
#endif //ANDROID
	bool                    m_Initialized    ; 

	WmTxBatchPacket*		m_pTxDbgPkt;

	bool					m_isPacketSet;

	CWifiTPacketReceiver*		m_pPacketReceiver;

	
	


};

#endif //WIFITESTMGR_H

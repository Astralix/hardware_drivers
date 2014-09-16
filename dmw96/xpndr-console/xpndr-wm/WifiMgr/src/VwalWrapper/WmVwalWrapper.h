///////////////////////////////////////////////////////////////////////////////
///
/// @file    	CWmVwalWrapper.h
///
/// @brief  	Provides a map of functions defined in Qwm.h
///         	to WiFi API calls.
/// 
/// @note   	For now, this adapter is for the Wifi Mib API only. 
///
/// @internal
///
/// @author  	sharons
///
/// @date    	12/10/2006
///
/// @version 	1.1
///
///
///				Copyright (C) 2001-2006 DSP GROUP, INC.   All Rights Reserved
///	

#ifndef WM_VWAL_WRAPPER_H
#define	WM_VWAL_WRAPPER_H

///////////////////////////////////////////////////////////////////////////////
// Includes
//
// 
// 
///////////////////////////////////////////////////////////////////////////////

#include "WmTypes.h"
//#include "WifiMgr/include/WifiMgr.h"
//#include "WmCommon.h"
//#include "infra/include/InfraTimer.h"
//#include "infra/include/InfraPtrList.h"
//#include "infra/include/InfraMutex.h"

#ifndef WM_EMUL
#include "vwal.h"
#endif

#define WM_UAPSD_BIT_MASK            0x80

///////////////////////////////////////////////////////////////////////////////
///
/// @class  CWmVwalWrapper
///
/// @brief  Wifi Mib API function calls wrapper.
///
class CWmVwalWrapper 
					   
{


public:

	///////////////////////////////////////////////////////////////////////////
	// Destructor
	virtual ~CWmVwalWrapper();


	///////////////////////////////////////////////////////////////////////////
	// @brief   GetInstance
	// 			Returns an Instance of this class
	static CWmVwalWrapper* GetInstance();

	///////////////////////////////////////////////////////////////////////////
	// @brief   FreeInstance
	// 			Frees static member m_pInstance.
	static void FreeInstance();
	


public:

	///////////////////////////////////////////////////////////////////////////
	// IDrvWrapper Interface
	// 
	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperInit
	///			For internal parameters initialization.
	/// 		WifiDeviceName - WiFi Device Name.	
	//virtual EWmStatus DrvWrapperInit( IN const char* const WifiDeviceName );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperTerminate
	///			
	//virtual void DrvWrapperTerminate( void );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperRegisterClient
	///			
	///			Register pClient as IWmNeMgr client.
	/// 		Note that you should call unregister function before deleting 
	/// 		pClient. 
	/// 		@param[in] pClient Pointer to IDrvWrapperClient.
	///
	//virtual void DrvWrapperRegisterClient( IN IDrvWrapperClient* pClient );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperUnRegisterClient
	///			
	///			Unregister pClient.
	/// 		@param[in] pClient Pointer to IWmNeMgrClient.
	///
	//virtual void DrvWrapperUnRegisterClient( IN IDrvWrapperClient* pClient );


    
	
	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperScan
	///			
	///			Triggers scan operation.
	/// 		@param[in] pInfo pointer to user information if exist...
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///
	//virtual EWmStatus DrvWrapperScan( void );


    

	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperConnect 
	///			
	/// 		Triggers connect operation.
	///			Connect to the Network defined by the given SSID.
	///			
	/// 		@param[in] pSsid pointer to the network's SSID.
	/// 		@param[in] pBssid pointer to the AP's BSSID, i.e., its mac 
	/// 				  address.
	/// 
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///
	//virtual EWmStatus DrvWrapperConnect( 
	//	IN WmMacAddress* pBssid, 
	//	IN WmSsid*       pSsid);
										   

	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperConnect 
	///			
	/// 		Triggers connect operation.
	///			Connect to the Network defined by the given SSID.
	///			
	/// 		@param[in] pSsid pointer to the network's SSID.
	/// 		@param[in] pInfo pointer to user information if exist...
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///
	//virtual EWmStatus DrvWrapperConnect( IN WmSsid* pSsid );
		
	
                      
	///////////////////////////////////////////////////////////////////////////
	///	@brief	DrvWrapperDisconnect 
	///			
	/// 		Triggers Disconnect operation.
	///			When connected, i.e., after a successful call to 
	/// 		Connect(), closes connection.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///
	//virtual EWmStatus DrvWrapperDisconnect( void ) ; 

public:

    enum EWmScanMode{
    	/// The driver will determine the best mode for scanning based on all
    	/// available information.  The driver can use the Country String,
    	/// Regulatory Domain, or Regulatory Class information MIB objects locally,
    	/// and will use Country string, Regulatory Domain, or regulatory Class
    	/// information (or whatever else it can use) from Beacons and Probe
    	/// Responses. 
    	WM_SCANNING_MODE_AUTO	= 0,
    
    	/// Forces the driver to use passive scanning only. 
    	WM_SCANNINGMODE_PASSIVE	= 1,
    
    	/// Forces the driver to use active scanning only.   
    	WM_SCANNINGMODE_ACTIVE	= 2,
    };
	
public:

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetMacAddress
	/// 
	/// 		Sets Mac Address state
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in] pBssid pointer that holds the desired Mac Address.
	/// 		@param[in] pInfo pointer to user information if exists...
	///@return  WM_OK  - function call succeeded.
	///   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetMacAddress( IN WmMacAddress* pBssid); 

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetMacAddress
	/// 
	/// 		Gets Mac Address state
	/// 		@param[out] pBssid on return will hold the value of the 
	/// 		MacAddress.
	/// 		@param[in] pInfo pointer to user information if exists...
	///@return  WM_OK  - function call succeeded.
	///   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetMacAddress(OUT WmMacAddress* pBssid);

    ///////////////////////////////////////////////////////////////////////////////
	///	@brief	SetMaxTxRate
	/// 
	/// 		Sets Max Tx Rate
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in] MaxRate max reate to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetMaxTxRate( IN uint32 pMaxRate);

	//////////////////////////////////////////////////////////////////////////////
	///	@brief	GetMaxTxRate
	/// 
	/// 		Gets Max Tx Rate
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out] pMaxTxRate on return will hold max rate value.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetMaxTxRate( OUT uint32* pMaxTxRate);


	///////////////////////////////////////////////////////////////////////////////
	///	@brief	SetChannel
	/// 
	/// 		Sets channel
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in] Channel channel to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetChannel( IN uint32 Channel);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetChannel
	/// 
	/// 		Gets channel
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out] pChannel on return will hold current channel.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetChannel( OUT uint32* pChannel);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetChannelFrequency
	/// 
	/// 		Sets channel frquency
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in]  Frequency to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetChannelFrequency( IN uint32 Frequency);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetChannelFrequency
	/// 
	/// 		Gets channel frequency.
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out]  pFrequency on return will hold current channel 
	/// 				    frequency.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetChannelFrequency( OUT uint32* pFrequency);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetMaxTxPower
	/// 
	/// 		Sets maximum transmit power.
	///			@param[in]  Power power to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetMaxTxPower( IN uint32 pPower);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetMaxTxPower
	/// 
	/// 		Gets max transmit power.
	///			@param[out]  pPower on return will hold max TX power.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetMaxTxPower( OUT uint32* pPower);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetAvgRssiWindowSize
	/// 
	/// 		Sets average RSSI window size for beacons and probe requests.
	///			@param[in]  Size Average RSSI window size to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetAvgRssiWindowSize( IN uint32 Size);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAvgRssiWindowSize
	/// 
	/// 		Gets average RSSI window size for beacons and probe requests.
	///			@param[out]  pSize on return will hold average RSSI window
	/// 						size.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAvgRssiWindowSize( OUT uint32* pSize);

	

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetAvgRssiDataWindowSize 
	/// 
	/// 		Sets average RSSI data window size (for data).
	///			@param[in]  Size Average RSSI data window size to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetAvgRssiDataWindowSize( IN uint32 Size);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAvgRssiDataWindowSize
	/// 
	/// 		Gets average RSSI data window size (for data).
	///			@param[out]  pSize on return will hold average data RSSI 
	/// 						window size.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAvgRssiDataWindowSize( OUT uint32* pSize);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAvgRssi
	/// 
	/// 		Gets average RSSI of recieved beacons and probe requests frames.
	///			@param[out]  pRssi average rssi.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAvgRssi( OUT uint32* pRssi);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAvgRssiData
	/// 
	/// 		Gets average RSSI of recieved data frames.
	///			@param[out]  pRssi average rssi.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAvgRssiData( OUT uint32* pRssi);



	///////////////////////////////////////////////////////////////////////////
	///@brief		GetStatistics
	/// 			Get statistics.
	///			@param[out]  pStatitics on return will hold statistics.
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
	///             
	///@return       0     WIFI MIB API call succeded.
	///				(>0)  WIFI MIB API call failed. 
	///
	EWmStatus GetStatistics( OUT WmStatistics* pStatistics );


    
	///////////////////////////////////////////////////////////////////////////
	///@brief		GetConnectionState
	///      
	///				Gets connection state
	///
	///             
    ///				Calls Wifi Mib API with the following Mib Object parameters:
	///				mibObjectID = MIBOID_CONNECTIONSTATE;
    ///             operation   = WIFI_MIB_API_OPERATION_READ;    
    ///             mibObject   = pConnectionState;
	///				
	///@param[out]  pConnectionState on return will hold the connection state. 
	///				
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
    ///
	EWmStatus GetConnectionState(OUT EWmConnectionState* pConnectionState);

	
	///////////////////////////////////////////////////////////////////////////
	///@brief		GetScanListSize
	///      
	///				Gets scan list size.
	///
	///             
    ///				Calls Wifi Mib API with the following Mib Object 
	/// 			parameters:
	///				mibObjectID = MIBOID_SCANLISTSIZE;
    ///             operation   = WIFI_MIB_API_OPERATION_READ;    
    ///             mibObject   = pScanListSize;
	///				
	///@param[out]  pScanListSize on return will hold the value of the scanned 
	/// 			list size.
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
	//EWmStatus GetScanListSize(OUT uint32* pScanListSize);


	///////////////////////////////////////////////////////////////////////////
	///@brief		GetScanListInfo
	///      
	///				Gets scan list AP's info. It is the user responsibility
	///				to allocate enough memory for ScanListInfoStructArray. In 
	///				particular, its size should be set to the value returned 
	/// 			in GetScanListSize.
	///             For the different MIB objects see the private functions
	///				below.
	///
	///@param[out]  pScanListInfoArray on return holds AP's info: BSSID, SSID, 
	///				Channel, etc. 
	///@param[in]   size pScanListInfoArray size. MUST be equal to the size 
	/// 			returned by GetScanListSize function call.
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
    ///
	//EWmStatus GetScanListInfo(OUT WmScanListInfo* pScanListInfoArray, 
	//						  IN  uint8           size              );


	////////////////////////////////////////////////////////////////////////////////
	///@brief		GetScanListInfo
	///      
	///				Gets scan list info of the given Bssid (if exists)
	///
	///@param[out]  pScanListInfoArray on return holds given AP's info: BSSID, SSID, 
	///				Channel, etc. 
	/// 
	///@param[in]   pBssid AP's Bssid
	/// 
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
	///
	//EWmStatus GetScanListInfo( OUT WmScanListInfo* pScanListInfoArray, 
	//						   IN WmMacAddress* pBssid );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetAntennaDiversity
	/// 
	/// 		Sets Antenna Diversity
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in] pAntennaDiversity antenna diversity to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetAntennaDiversity( IN uint32 antennaDiversity);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAntennaDiversity
	/// 
	/// 		Gets Antenna Diversity
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out] pAntennaDiversity on return will hold antenna diversity value.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAntennaDiversity( OUT uint32* pAntennaDiversity);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetAutoPhyRateAdoptation
	/// 
	/// 		Sets Auto Rate adoptation settings (rate fallback)
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in] isEnabled enable / disable phy rate adoptation.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetAutoPhyRateAdoptation(IN bool isEnabled);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAutoPhyRateAdoptation
	/// 
	/// 		Gets Auto Rate adoptation settings (rate fallback)
	///         @param[in] pIsEnabled on return will hold phy rate adoptation
	/// 				   settings (true/false enabled disabled).
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetAutoPhyRateAdoptation( OUT bool* pIsEnabled);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	WriteRegister
	/// 
	/// 		Sets the given register Register to hold the given value.
	///			@param[in] Register register number.
	/// 		@param[in] Value the value that should be set.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus WriteRegister(IN uint32 Register,
							IN uint32 Value   );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	WriteBBRegister
	/// 
	/// 		Sets the given baseband register Register to hold the given value.
	///			@param[in] Register register number.
	/// 		@param[in] Value the value that should be set.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus WriteBBRegister(IN uint32 Register,
							IN uint8 Value   );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	ReadRegister
	/// 
	/// 		Gets the value of the given  register (Register) and  
	/// 		stores it in Value.
	///			@param[in]  Register register number.
	/// 		@param[out] Value on return holds the value that is stored 
	/// 					in the given register.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus ReadRegister(IN  uint32 Register,
							 OUT uint32*   Value   );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	ReadBBRegister
	/// 
	/// 		Gets the value of the given baseband register (Register) and 
	/// 		stores it in Value.
	///			@param[in]  Register number.
	/// 		@param[out] Value on return holds the value that is stored 
	/// 					in the given register.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus ReadBBRegister(IN  uint32 Register,
							 OUT uint8*   Value   );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetTxPacket
	/// 
	/// 		Sets packets parameteres that will be transmitted when calling
	/// 	    GenerateTxPackets function
	///         @param[in]  numTransmits - number of packets to generate (0 to stop) 
	///						phyRate - specifies the phy rate for the packet
	///						powerLevel - PHY Header's TXPWRLVL field will be 
	/// 								 programmed with powerLevel 
	///						pktLen -  the length of the packet (which does not 
	/// 							  include FCS), specified in bytes
	///						pPkt - pointer to the 802.11 packet
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus SetTxPacket(IN WmTxBatchPacket* pTxBatchPacket);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GenerateTxPackets
	///			Start generating packets. Packet prototype was set in SetTxPacket() call.
	/// 		Packet generation will continue until all of the packets are transmitted
	/// 		or SetTxPacket is called with numTransmits equal to zero
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus GenerateTxPackets();

	///////////////////////////////////////////////////////////////////////////
	///	@brief	ReceivePackets
	/// 		Poll the driver to retrive an update to 'numRxDesc' packets. 
	/// 		These packets will be copied into the rxDesc array and numRxDesc 
	/// 		will be updated to indicate the number of successfully retrieved 
	/// 		packets. 
	/// 		@param[out] rxDesc array of RX_WIFI_PACKET_DESC with attached 
	/// 					buffers.
	/// 		@param[inout] numRxDesc specifices size of rxDesc array on function 
	/// 					entry.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	/// 
	EWmStatus ReceivePackets(WmRxWifiPacket rxDesc[], uint8* pNumRxDesc);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetPs
	/// 
	///			Get whether Power Save is on or off.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetPs( OUT bool* pIsPSEnabled);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	EnablePs
	/// 
	///			Enables Power Save.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus EnablePs();


	///////////////////////////////////////////////////////////////////////////
	///	@brief	DisablePs
	/// 
	///			Disables Power Save.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus DisablePs();

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetPsAction
	/// 
	/// 		Sets the an action the driver might take when it has an 
	/// 	   	opportunity to  sleep (i.e. there is no downlink traffic 
	/// 		buffered.
	///			@param[in]  psAction power svae mode to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetPsAction( IN EWmPsAction psAction);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetPsAction
	/// 
	/// 		Gets power save action.
	///			@param[out]  pPsAction on return will hold power save action.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	EWmStatus GetPsAction( OUT EWmPsAction* pPsAction);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetUapsdMaxSpLength
	/// 
	/// 		Sets UAPSD max service period length.
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in]  maxSpLength max service period to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetUapsdMaxSpLength( IN EWmUapsdMaxSpLen maxSpLength );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetsUapsdMaxSpLength
	/// 
	/// 		Gets UAPSD max service period length.
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out]  pMaxSpLength on return will hold max service period.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetUapsdMaxSpLength( OUT EWmUapsdMaxSpLen* pMaxSpLength );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetPsMode
	/// 
	/// 		Sets PS mode (UAPSD/LEGACY of the given AC).
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[in]  psMode power save mode to be set.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
    EWmStatus SetPsMode( IN uint32 psMode);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetPsMode
	/// 
	/// 		Gets PS mode (UAPSD/LEGACY of the given AC).
	/// 		IMPORTANT - Must be called from disconnected state. 
	///			@param[out] pPsMode on return will hold AC's power save mode.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	EWmStatus GetPsMode( OUT uint32* pPsMode);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SendNullPacket
	/// 
	///			Sends power mgmt frames.
	/// 		@param[in] Freq the frequency in which power mgmt packets will be 
	/// 				   sent.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SendNullPacket(uint32 Freq);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	StartDriver
	///			Starts driver
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus StartDriver();

	///////////////////////////////////////////////////////////////////////////////
	///@brief	StopDriver
	///      	Stops Driver.
	///			@return   WM_OK  - function call succeeded.
	///   		   		 !WM_OK  - otherwise.	 
	/// 
	EWmStatus StopDriver( void );

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetMib
	/// 
	/// 		Gets the uint32 value of the given mib and store it 
	/// 		in Value.
	///			@param[in]  mib code.
	/// 		@param[out] Value on return holds the value that is stored 
	/// 					in the mib.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus GetMib(IN  uint32   mibCode,
					 OUT uint32*  Value   ); 

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetMib
	/// 
	/// 		Sets the given mib to hold the given uint32 value.
	///			@param[in] mib code.
	/// 		@param[in] Value the value that should be set.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus SetMib( IN uint32 mibCode,
					  IN uint32   Value   ); 

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetMib
	/// 
	/// 		Gets the value of the given mib (macaddr) and store it 
	/// 		in pMacAddr.
	///			@param[in]  mib code.
	/// 		@param[out] pWmMacAddr on return holds the value that is stored 
	/// 					in the mib.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus GetMib(IN  uint32   mibCode,
					 OUT WmMacAddress*  pWmMacAddr   ); 

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetMib
	/// 
	/// 		Sets the given mib to hold the given value (macaddr).
	///			@param[in] mib code.
	/// 		@param[in] pWmMacAddr the value that should be set.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus SetMib( IN uint32 mibCode,
					  IN WmMacAddress*  pWmMacAddr ); 


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetMib
	/// 
	/// 		Gets the value of the given mib (ssid) and store it 
	/// 		in pSsid.
	///			@param[in]  mib code.
	/// 		@param[out] pWmSsid on return holds the value that is stored 
	/// 					in the mib.
	/// 		@return     WM_OK  - function call succeeded.
	/// 				   !WM_OK  - otherwise.		
	///		
	EWmStatus GetMib(IN  uint32   mibCode,
					 OUT WmSsid*  pWmSsid   ); 

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetMib
	/// 
	/// 		Sets the given mib to hold the given value (ssid).
	///			@param[in] mib code.
	/// 		@param[in] pWmSsid the value that should be set.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus SetMib( IN uint32 mibCode,
					  IN WmSsid*  pWmSsid ); 



	EWmStatus SetDriverMode(IN uint32 mode);

	EWmStatus GetDriverMode(OUT uint32* pMode);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetTxPowerLevelAll
	/// 
	/// 		Sets transmit power for all channels and frequencies.
	///			@param[in]  Power power to be set [0..63].
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetTxPowerLevelAll( IN uint8 power);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetTxPowerLevelAll
	/// 
	/// 		Gets transmit power
	///			@freqInd[in] Frequency index
	/// 		@rateInd[in] Rate index
	/// 		@param[in,out]  pPower on return will hold max TX power.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetTxPowerLevel(IN uint8 freqInd,
							  IN uint8 rateInd,
							  INOUT uint8* pPower);



	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetTxPowerLevel
	/// 
	/// 		Gets transmit power
	///			@param[in] freqInd - Frequency index
	/// 		@param[in] rateInd - Rate index
	/// 		@param[in] powerLevel power level 
	/// 							  for a couple freqInd,rateInd.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus SetTxPowerLevel(IN uint8 freqInd,
									  IN uint8 rateInd,
									  IN uint8 powerLevel);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetTxPowerLevelTable
	/// 
	/// 		Gets transmit power
	///			@param[in] pTxPowerLevelTable - power level table
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus 
		SetTxPowerLevelTable(IN WmPowerLevelTable* pTxPowerLevelTable);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetTxPowerLevelTable
	/// 
	/// 		Gets transmit power
	///			@param[out] pTxPowerLevelTable - on exit will hold 
	/// 										 power level table
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus 
		GetTxPowerLevelTable(OUT WmPowerLevelTable* pTxPowerLevelTable);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetScanChanFreqList
	/// 
	/// 		Gets scan frequencies
	///			@param[out] pScanChanFreqList - on exit will hold 
	/// 										 list of scan frequencies
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus 
		GetScanChanFreqList(OUT WmScanChanFreqList* pScanChanFreqList);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetScanChanFreqList
	/// 
	/// 		Sets scan frequencies
	///			@param[in] pScanChanFreqList - list of scan frequencies
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus 
		SetScanChanFreqList(IN WmScanChanFreqList* pScanChanFreqList);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetScanChannelFreq
	/// 
	/// 		Sets scan frequencies
	///			@param[in] channelFreq - force driver to scan only on this channel
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus SetScanChannelFreq(IN uint32 channelFreq);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	AddChannelToScanFreqList
	/// 
	/// 		Adds channelFreq to  scan frequencies list
	///			@param[in] channelFreq - force driver to scan only on this channel
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus AddChannelToScanFreqList(IN uint32 channelFreq);



	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetPhyParameters
	/// 
	/// 		Set phy parameters
	///			@param[in] pMibPhyParameter - Phy parameter struct
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	virtual EWmStatus 
		SetPhyParameters(IN WmMibPhyParameter* pMibPhyParameter);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetAssociatedMacAddress
	/// 
	/// 		In connected state, gets the Mac Address of the assocaised AP.
	///			@param[out] pBssid on return will hold associated AP's Mac 
	/// 					Address.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus GetAssociatedMacAddress( OUT WmMacAddress* pAddr ); 


	/////////////////////////////////////////////////////////////////////////
	///	@brief	SetSsidForScanning
	/// 
	/// 		Sets Ssid for scanning. 
	///			Stops when the first occurence of the given ssid is found 
	///			during a scan.
	///			@param[in] pSsid Ssid for scanning
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus SetSsidForScanning( IN WmSsid* pSsid );


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetBssidForScanning
	/// 
	/// 		Sets Bssid for scanning. 
	///			Stops when the first occurence of the given Bssid is found 
	///			during a scan.
	///			@param[in] pBssid Bssid for scanninf
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus SetBssidForScanning( IN WmMacAddress* pBssid ); 



	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetTxFrameCount
	/// 
	///			Gets transmiited frames counts.
	///			@param[out] pTxFrameCount on return holds tx frames count.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus GetTxFrameCount( OUT uint32* pTxFrameCount ); 


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetRxFrameCount
	/// 
	///			Gets recieved frames counts.
	///			@param[out] pTxFrameCount on return holds rx frames count.
	/// 		@return    WM_OK  - function call succeeded.
	/// 				  !WM_OK  - otherwise.		
	///		
	EWmStatus GetRxFrameCount( OUT uint32* pRxFrameCount ); 


	///////////////////////////////////////////////////////////////////////////////
	///@brief		GetDesiredApSettings
	///@param[out]	pWmDesiredSsid pointer to the AP's desired SSID.
	///@param[out]	pWmDesiredBssid pointer to the desired AP's BSSID, i.e.,
	/// 				its mac address.
	///@return      WM_OK  - function call succeeded.
	///   		   !WM_OK  - otherwise.		
	///
	
	EWmStatus GetDesiredApSettings( OUT WmSsid* pWmDesiredSsid,
									 OUT WmMacAddress* pWmDesiredBssid );


    ///////////////////////////////////////////////////////////////////////////
    ///	@brief	SetCountry
    /// 
    ///         Sets the country string in which this Wifi MAC will operate.
    ///			@param[in]  Country to be set.
    ///			@return  WM_OK  - function call succeeded.
    ///   		   		!WM_OK  - otherwise.		
    ///		
    EWmStatus SetCountry(IN const EWmCountry Country);

    ///////////////////////////////////////////////////////////////////////////
    ///	@brief	GetCountry
    /// 
    ///         Sets the country string in which this Wifi MAC will operate.
    ///			@param[in]  Country to be set.
    ///			@return  WM_OK  - function call succeeded.
    ///   		   		!WM_OK  - otherwise.		
    ///		
    EWmStatus GetCountry(IN EWmCountry* Country);


    ///////////////////////////////////////////////////////////////////////////
    ///	@brief	SetScanMode
    /// 
    ///         Sets scannig mode.
    ///			@param[in]  ScanMode scan mode.
    ///			@return  WM_OK  - function call succeeded.
    ///   		   		!WM_OK  - otherwise.		
    ///		
    EWmStatus SetScanMode(IN EWmScanMode ScanMode);


	///////////////////////////////////////////////////////////////////////////
    ///	@brief	SetNumProbes
    /// 
    ///         Sets num probes for scannig
    ///			@param[in]  NumProbes num probes.
    ///			@return  WM_OK  - function call succeeded.
    ///   		   		!WM_OK  - otherwise.		
    ///		
    EWmStatus SetNumProbes(IN uint32 NumProbes);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetDwellTime
	/// 
	///         Sets scan dwell time.
	///			@param[in]  Time scan dwell time
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetScanDwellTime(IN uint32 Time);

	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetScanHomeTime
	/// 
	///         Sets scan home time
	///			@param[in]  Time scan home time.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetScanHomeTime(IN uint32 Time);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	SetPsPowerLevel
	/// 
	///         Sets PS power level
	///			@param[in]  PowerLevel power level to be set
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus SetPsPowerLevel(IN uint32 PowerLevel);


	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetPsPowerLevel
	/// 
	///         Gets PS power level
	///			@param[in]  pPowerLevel On return holds power level.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetPsPowerLevel(IN uint32* pPowerLevel);




    
	///////////////////////////////////////////////////////////////////////////
	///	@brief	GetTMdata
	/// 
	///         Gets TM data MIB 
	///			@param[OUT]  pWmMibTMdata On return holds the TM data.
	///			@return  WM_OK  - function call succeeded.
	///   		   		!WM_OK  - otherwise.		
	///		
	EWmStatus GetTMdata(OUT WmMibTMdata* pWmMibTMdata); 




	
///////////////////////////////////////////////////////////////////////////////
//  Private Members

private:

	///////////////////////////////////////////////////////////////////////////
	// Private members

	static CWmVwalWrapper*	m_pInstance  ;  // WM instance to promises 
											// a single instance of this 
										    // class.
	//static CInfraMutex	m_Mutex          ;  // WM mutex.
	
	char  				m_DeviceName[MAX_DEVICE_NAME_LENGTH];	  
	//CInfraTimer         m_Timer			  ;
	//CInfraPtrList		m_VwClientsList   ;

#ifdef WM_EMUL
private:

	uint32          m_ConnectCounter ; //part of the stub implementation
	uint32          m_ScanningCounter; //part of the stub implementation
	uint32          m_Rssi           ; //part of the stub implementation
	uint32          m_ScanListSize   ; //part of the stub implementation
	uint32			m_Connected		 ; //part of the stub implementation 
	WmMacAddress	m_Bssid  		 ; //part of the stub implementation
	uint32          m_Channel        ; //part of the stub implementation								   
	uint32          m_MaxTxRate      ; //part of the stub implementation
	 
#endif // #ifdef WM_EMUL


private:

	///////////////////////////////////////////////////////////////////////////
	// Private functions - promises a singleton behavior

	// private constructor
	CWmVwalWrapper(); 

	// private copy constructor
	CWmVwalWrapper(const CWmVwalWrapper&) {}; 

    // private copy constructor
	//CWmVwalWrapper(const IDrvWrapper&) {}; 

	// private '=' operator 
	void operator=(const CWmVwalWrapper&){};

	// private '=' operator 
	//void operator=(const IDrvWrapper&) {};



private:
	///////////////////////////////////////////////////////////////////////////
	//  Private Functions
/*
    void HandleEvent( EWmDriverEvent event, EWmStatus status );
	void HandleOnScanEvent     (  EWmStatus status );
	void HandleOnConnectEvent  (  EWmStatus status );
	void HandleOnDisonnectEvent(  EWmStatus status );
	static void SignalHandler  (  int SignalNum    );
	

	///////////////////////////////////////////////////////////////////////////
	// Temporary - For Event Emulation.
	static void TimerConnectEvent  ( void * Cookie );
	static void TimerScanEvent     ( void * Cookie );
	static void TimerDisonnectEvent( void * Cookie );
*/	

#ifndef WM_EMUL

	//will hold signals file descriptor.
	//static int m_Fd;
	
	///////////////////////////////////////////////////////////////////////////
	//  Pivate Functions
	// 
	///////////////////////////////////////////////////////////////////////////
	///@brief		DoWifiMibApiCall
	/// 
	/// 			Does the actual call to DoWifiMibApi.
	/// 
	EWmStatus DoWifiMibApiCall(
		mibObjectContainer_t* pMibObjectArray, 
		uint32 				  num            );
	
	///////////////////////////////////////////////////////////////////////////
	//@brief    DisplayMIBObjContainerStatuses	
	//
	// 			Displays index number of MIB object in which the status field 
	// 			is not equal to WIFI_MIB_API_SUCCESS.
	
	void DisplayMIBObjContainerStatuses( 
		mibObjectContainer_t* pMibObjectContainer, 
		int 				  numMibObjects      );


	//debug functions
	//Print String description of Driver's state
	void PrintState();
	//Print given connection state
	void PrintState(uint32 connectionState);

#endif //WM_EMUL


};



#endif // #if !defined WM_VWAL_WRAPPER_H


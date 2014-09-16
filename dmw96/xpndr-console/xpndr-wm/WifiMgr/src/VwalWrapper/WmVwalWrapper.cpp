///
/// 
/// @file    	CVwalWrapper.cpp
///
/// @brief  Provides a set of methods that map the WiFi API calls.
/// 
/// @note   For now, this adapter is for the WiFi Mib API only. 
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
///				Copyright (C) 2001-2010 DSP GROUP, INC.   All Rights Reserved
///	

///////////////////////////////////////////////////////////////////////////////
// Includes.

#include "WmVwalWrapper.h"
#include "Utils/WmUtils.h"

#ifndef ANDROID
#ifndef WM_EMUL
#include "Vwal/include/WifiApi.h"
#include "Vwal/include/vwal.h"
#endif // #ifndef WM_EMUL
#include "infra/include/InfraMacros.h"
#include "infra/include/InfraLog.h"
#include "infra/include/InfraUtils.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#else
#include "WifiApi.h"
#include "vwal.h"
#include <unistd.h>
#include <assert.h>
#include "cutils/log.h"
#define LOG_TAG "VwalWrapper"
#endif


///////////////////////////////////////////////////////////////////////////////
// Static members.
#ifndef ANDROID
int				CWmVwalWrapper::m_Fd;
CInfraMutex		CWmVwalWrapper::m_Mutex;
#endif
CWmVwalWrapper*	CWmVwalWrapper::m_pInstance = NULL;


////////////////////////////////////////////////////////////////////////////////
// Constatnts

#define WM_TIMER_FREQUENCY 			 1000


///////////////////////////////////////////////////////////////////////////////
// structures - for internal use. Converts user structs to Vwal structs

///////////////////////////////////////////////////////////////////////////////
// WmSsid extended to support conversion from Vwal's SSID to WmSSID and
// vice versa.
struct WmSsid_p : public WmSsid{


	///////////////////////////////////////////////////////////////////////////
	// Internal member initialized when a conversion to Vwal's struct
	// is needed.
	SSID VwalSsid; 

	///////////////////////////////////////////////////////////////////////////
	//Copy constructor.
	WmSsid_p(const WmSsid* pWmSsid)
	{
		Length = pWmSsid->Length;
        for (uint8 loop=0; loop<pWmSsid->Length; ++loop)
		{
			Ssid[loop] = pWmSsid->Ssid[loop]; 
		}
	}


	///////////////////////////////////////////////////////////////////////////
	//Copy constructor. Converts Vwal's SSID to WmSsid_p.
    WmSsid_p( const SSID& VwalSsid )
	{
		Length = VwalSsid.length;
		for (uint8 loop=0; loop<VwalSsid.length; ++loop)
		{
			Ssid[loop] = VwalSsid.ssid[loop];
		}
	}

	///////////////////////////////////////////////////////////////////////////
	//Converts WmSsid_p to Vwal's SSID.
    SSID& ToVwalFormat()
	{
        VwalSsid.length = Length;
		for (uint8 loop=0; loop<Length; ++loop)
		{
			VwalSsid.ssid[loop] = Ssid[loop];
		}
		return VwalSsid;
	}

};


///////////////////////////////////////////////////////////////////////////////
// WmMacAddress extended to support conversion from Vwal's Mac Address to 
// WmMacAddress and vice versa.
struct WmMacAddress_p : WmMacAddress{

	///////////////////////////////////////////////////////////////////////////
	// Internal member initialized when a conversion to Vwal's struct
	// is needed.
	MAC_ADDRESS VwalBssid; 

	///////////////////////////////////////////////////////////////////////////
	//Copy constructor.
	WmMacAddress_p( WmMacAddress* pWmBssid )
	{
		
		for (uint8 loop=0; loop<WM_MAC_ADDR_LEN; ++loop)
		{
			Address[loop] = pWmBssid->Address[loop];
		}
	}


	///////////////////////////////////////////////////////////////////////////
	//Copy constructor. Converts Vwal's SSID to WmSsid_p.
    WmMacAddress_p( const MAC_ADDRESS&   VwalBssid )
	{
		
		for (uint8 loop=0; loop<WM_MAC_ADDR_LEN; ++loop)
		{
			Address[loop] = VwalBssid.address[loop];
		}
	}

	///////////////////////////////////////////////////////////////////////////
	//Converts WmSsid_p to Vwal's SSID.
    MAC_ADDRESS& ToVwalFormat()
	{
        for (uint8 loop=0; loop<WM_MAC_ADDR_LEN; ++loop)
		{
			VwalBssid.address[loop] = Address[loop];
		}
		return VwalBssid;
	}

};




CWmVwalWrapper::CWmVwalWrapper()
{
	LOGD("CWmVwalWrapper::CWmVwalWrapper --->" );

    int result = VWAL_Init();
#ifdef ANDROID
	int size = snprintf(m_DeviceName, 
						sizeof(m_DeviceName), "%s", "wifi0");


	assert( size > 0 && (size_t)size < sizeof(m_DeviceName)-1);

	
    
#else

	//Signal handling open file to get events.
	m_Fd = open("/dev/wifi0", O_RDONLY | O_NONBLOCK );

	INFRA_ASSERT(m_Fd >= 0);

	// install the signal handler
	signal(SIGIO, &SignalHandler);

	// set the process id that will recieve a SIGIO signal for event on fd.
	fcntl(m_Fd, F_SETOWN, getpid());

	//F_GETFL - get file flags
	//F_SETFL - set flags. In this case we add an async flag.
	fcntl(m_Fd, F_SETFL, fcntl(m_Fd, F_GETFL) | FASYNC);
#endif

	LOGD("CWmVwalWrapper::CWmVwalWrapper=%d <---",result );
}


///////////////////////////////////////////////////////////////////////////
// @brief   GetInstance
// 			Returns an Instance of this class

CWmVwalWrapper* CWmVwalWrapper::GetInstance()
{

	LOGD ( "CWmVwalWrapper::GetInstance ---->" );

	// Guards.
	//CInfraGuard<CInfraMutex> autoGuard(m_Mutex);

	if(m_pInstance == NULL)
	{
		m_pInstance = new CWmVwalWrapper;
		
	}

	assert(m_pInstance);
	
	LOGD ( "CWmVwalWrapper::GetInstance <----" );

	return m_pInstance;

}

///////////////////////////////////////////////////////////////////////////
// @brief   FreeInstance
// 			Frees static member m_pInstance.
void CWmVwalWrapper::FreeInstance()
{

	LOGD ("CWmVwalWrapper::FreeInstance ---->" );
#ifndef ANDROID
	// Guards.
	CInfraGuard<CInfraMutex> autoGuard(m_Mutex);
#endif
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;

	}

	LOGD ("CWmVwalWrapper::FreeInstance <----" );

}

CWmVwalWrapper::~CWmVwalWrapper()
{
	LOGD("CWmVwalWrapper::~CWmVwalWrapper --->" );
#ifndef ANDROID
	close(m_Fd);
#endif
    VWAL_Shutdown();

	LOGD("CWmVwalWrapper::~CWmVwalWrapper <---" );
}

#ifndef ANDROID
///////////////////////////////////////////////////////////////////////////////
///	@brief	DrvWrapperInit
///			For internal parameters initialization.
/// 		@param[in] WifiDeviceName - WiFi Device Name.  
/// 
EWmStatus CWmVwalWrapper::DrvWrapperInit( IN const char* const WifiDeviceName )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperInit --->" ) );


	INFRA_ASSERT( strlen(WifiDeviceName) < MAX_DEVICE_NAME_LENGTH );
	memset(m_DeviceName, 0, sizeof(m_DeviceName));



	int size = snprintf(m_DeviceName, 
						sizeof(m_DeviceName), "%s", WifiDeviceName);

	

	INFRA_ASSERT( size > 0 && (size_t)size < sizeof(m_DeviceName)-1);

/*	
	uint32 bssType  =  DesiredBSSTypeIsESS;
	uint32 phyMode  =  PHYMODE_80211A;


	uint8 u8status = 0;
	mibObjectContainer_t mibObjectContainer[] =
	{

		{MIBOID_BSSTYPE,   WIFI_MIB_API_OPERATION_WRITE, u8status, &bssType},
		{MIBOID_PHYMODE,   WIFI_MIB_API_OPERATION_WRITE, u8status, &phyMode},

	};
    uint32 numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result    =  DoWifiMibApiCall(mibObjectContainer, numMibObjects);
*/
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperInit <---" ) );

	EWmStatus result = WM_OK;
	return result;


}


///////////////////////////////////////////////////////////////////////////
///	@brief	DrvWrapperTerminate
///			
///			Register pClient as IWmNeMgr client.
/// 		Note that you should call unregister function before deleting 
/// 		pClient. 
/// 		@param[in] pClient Pointer to IWmNeMgrClient.
///
void CWmVwalWrapper::DrvWrapperTerminate( void )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperTerminate ---->" ) );

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperTerminate <----" ) );

}

///////////////////////////////////////////////////////////////////////////
///	@brief	DrvWrapperRegisterClient
///			
///			Register pClient as IWmNeMgr client.
/// 		Note that you should call unregister function before deleting 
/// 		pClient. 
/// 		@param[in] pClient Pointer to IDrvWrapperClient.
///
void CWmVwalWrapper::DrvWrapperRegisterClient( 
												IN IDrvWrapperClient* pClient
																			  )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperRegisterClient ---->" ) );
	if ( pClient )
		m_VwClientsList.AddToTail( pClient );

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperRegisterClient <----" ) );

}



///////////////////////////////////////////////////////////////////////////
///	@brief	DrvWrapperUnRegisterClient
///			
///			Unregister pClient.
/// 		@param[in] pClient Pointer to IDrvWrapperClient.
///
void CWmVwalWrapper::DrvWrapperUnRegisterClient(
												IN IDrvWrapperClient* pClient
																			  )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperUnRegisterClient ---->" ) );

	if ( pClient )
		m_VwClientsList.RemoveElement( pClient );

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperUnRegisterClient <----" ) );

}


///////////////////////////////////////////////////////////////////////////////
///@brief		DrvWrapperConnect
///@param[in]	pSsid pointer to the AP's SSID.
///AND/OR-
///@param[in]	pBssid pointer to theuint32 AP's BSSID, i.e., its mac address.
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///

EWmStatus CWmVwalWrapper::DrvWrapperConnect( IN WmSsid* pSsid )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperConnect(ssid) ---->" ) );

	INFRA_ASSERT(pSsid);
	
	uint32 command  =  WIFI_MIB_API_COMMAND_START;
    WmSsid_p ssid(pSsid);
	
	//Building MIB Objects...
	uint8 u8status = 0;
	mibObjectContainer_t mibObjectContainer[] =
    {

		{MIBOID_DESIREDSSID, WIFI_MIB_API_OPERATION_WRITE, u8status,
														 &ssid.ToVwalFormat()},
		{MIBOID_DSPGCOMMAND,      WIFI_MIB_API_OPERATION_WRITE, u8status,
																	  &command}
    
    };

	uint32 numMibObjects = sizeof(mibObjectContainer)
	                                             /sizeof(mibObjectContainer_t);

	m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY ,
						   &TimerConnectEvent ,
						   (void*)this        ,
						   false)             ; //Periodic

	
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperConnect(ssid) <----" ) );

	return ( DoWifiMibApiCall(mibObjectContainer, numMibObjects) );

}

///////////////////////////////////////////////////////////////////////////////
EWmStatus CWmVwalWrapper::DrvWrapperConnect( 
	IN WmMacAddress* pBssid, 
	IN WmSsid*       pSsid )

{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperConnect(bssid,ssid) ---->" ) );

	INFRA_ASSERT(pBssid);
	INFRA_ASSERT(pSsid);
	
	WmMacAddress_p     bssid(pBssid);
	WmSsid_p           ssid(pSsid);
	uint32 command  =  WIFI_MIB_API_COMMAND_START;
    
    
	uint8 u8status = 0;
	mibObjectContainer_t mibObjectContainer[] =
    {

		
		{MIBOID_DESIREDBSSID,  WIFI_MIB_API_OPERATION_WRITE, u8status,
			                                            &bssid.ToVwalFormat()},

		{MIBOID_DESIREDSSID, WIFI_MIB_API_OPERATION_WRITE, u8status, 
			                                             &ssid.ToVwalFormat()},

		{MIBOID_DSPGCOMMAND,      WIFI_MIB_API_OPERATION_WRITE, u8status, 
			                                                          &command}
    };

	uint32 numMibObjects = sizeof(mibObjectContainer)/
		                                         sizeof(mibObjectContainer_t);
	

	m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY ,
						   &TimerConnectEvent ,
						   (void*)this        ,
						   false)             ; //Periodic

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperConnect(bssid,ssid) <----" ) );

	return ( DoWifiMibApiCall (mibObjectContainer,numMibObjects) );
	

}
    

///////////////////////////////////////////////////////////////////////////////
///@brief	DrvWrapperDisconnect
///      	Disconnects.
///			@return   WM_OK  - function call succeeded.
///   		   		 !WM_OK  - otherwise.	 
/// 
EWmStatus CWmVwalWrapper::DrvWrapperDisconnect( void )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperDisconnect <----" ) );

	
	uint32 command = WIFI_MIB_API_COMMAND_STOP;
    
    mibObjectContainer_t mibObjectContainer[] = 
        {
            {MIBOID_DSPGCOMMAND, WIFI_MIB_API_OPERATION_WRITE, 0, &command}
		};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);


	m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY   ,
						   &TimerDisonnectEvent ,
						   (void*)this          ,
						   false)               ; //Periodic


	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::DrvWrapperDisconnect <----" ) );

	return DoWifiMibApiCall(mibObjectContainer,numMibObjects);
    
}
    
	
///////////////////////////////////////////////////////////////////////////////
///@brief		DrvWrapperScan
///      
///				Sends scan command to Mib Object, Command via Wifi Mib API.
///				mibObjectID = MIBOID_DSPGCOMMAND		
///				operation   = WIFI_MIB_API_OPERATION_WRITE;    
///				mibObject   = WIFI_MIB_API_COMMAND_SCAN;
///				
///
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///
EWmStatus CWmVwalWrapper::DrvWrapperScan(void)
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::DrvWrapperScan ---->" ) );

	
	uint32 command 		  = WIFI_MIB_API_COMMAND_SCAN;
	uint8  u8status       = 0;
    
    mibObjectContainer_t mibObjectContainer[] = 
        {
            {MIBOID_DSPGCOMMAND, WIFI_MIB_API_OPERATION_WRITE, u8status, 
				                                                      &command}
        };

	uint32 numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer , numMibObjects);

	
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::DrvWrapperScan <----\n\t"
				  "Scan returned %d",result ) );

	return result;
}
#endif
///////////////////////////////////////////////////////////////////////////////
///	@brief	SetMacAddress
/// 
/// 		Sets Mac Address 
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] pBssid pointer that holds the desired Mac Address.
/// 		@param[in] pInfo pointer to user information if exists...
///			@return  WM_OK  - function call succeeded.

///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMacAddress( IN WmMacAddress* pBssid)
{
	LOGD("CWmVwalWrapper::SetMacAddress ---->" );

	if(!pBssid)
	{
		LOGD( "CWmVwalWrapper::SetMacAddress <---- (pBssid not iinitialized)" );
		return WM_ILLEGAL_PARAMETER;
	}
    WmMacAddress_p bssid(pBssid);

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_MACADDRESS, WIFI_MIB_API_OPERATION_WRITE, 0, 
			                                           &bssid.ToVwalFormat() },
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD( "CWmVwalWrapper::SetMacAddress <---- (result=%d)",result );
	return result;

}


///////////////////////////////////////////////////////////////////////////////
///	@brief	GetMacAddress
/// 
/// 		Gets Mac Address 
/// 		IMPORTANT - Must be called from disconnected state, i.e, after
/// 		initialization or direct call to disconnect function. 
/// 		@param[out] pBssid on return will hold the value of the MacAddress.
/// 		@param[in] pInfo pointer to user information if exists...
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetMacAddress(OUT WmMacAddress* pBssid)
{

	LOGD("CWmVwalWrapper::GetMacAddress ---->" );

	MAC_ADDRESS       bssid;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_MACADDRESS, WIFI_MIB_API_OPERATION_READ, 0, &bssid},
	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if (result == WM_OK)
	{
		WmMacAddress_p wmBssid = bssid                ;
		*pBssid                = (WmMacAddress)wmBssid;

	}
	LOGD("CWmVwalWrapper::GetMacAddress <---- (result=%d)",result );

	return result;

}


///////////////////////////////////////////////////////////////////////////////
///	@brief	SetMaxTxRate
/// 
/// 		Sets Max Tx Rate
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] MaxRate max rate to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMaxTxRate( IN uint32 MaxRate)
{

	LOGD("CWmVwalWrapper::SetMaxTxRate ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_MAXTXRATE, WIFI_MIB_API_OPERATION_WRITE, 0, &MaxRate }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetMaxTxRate <----" );
	return result;

}


//////////////////////////////////////////////////////////////////////////////
///	@brief	GetMaxTxRate
/// 
/// 		Gets Max Tx Rate
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[out] pMaxTxRate on return will hold max rate value.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetMaxTxRate( OUT uint32* pMaxTxRate)
{

	LOGD("CWmVwalWrapper::GetMaxTxRate ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_MAXTXRATE, WIFI_MIB_API_OPERATION_READ, 0, pMaxTxRate }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetMaxTxRate <---- " );
	return result;

}


///////////////////////////////////////////////////////////////////////////////
///	@brief	SetChannel
/// 
/// 		Sets channel
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] Channel channel to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetChannel( IN uint32 Channel)
{

	LOGD("CWmVwalWrapper::SetChannel ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_CURRENTCHANNEL, WIFI_MIB_API_OPERATION_WRITE, 0, &Channel}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetChannel <----" );
	return result;

}


//////////////////////////////////////////////////////////////////////////////
///	@brief	GetChannel
/// 
/// 		Gets Channel
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[out] pChannel on return will hold current channel.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetChannel( OUT uint32* pChannel)
{

	LOGD("CWmVwalWrapper::GetChannel ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_CURRENTCHANNEL, WIFI_MIB_API_OPERATION_READ, 0, pChannel }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetChannel <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetChannelFrequency
/// 
/// 		Sets channel frquency
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in]  Frequency to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetChannelFrequency( IN uint32 Frequency)
{

	LOGD("CWmVwalWrapper::SetChannelFrequency ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{ MIBOID_CURRENTCHANNELFREQUENCY, 
			WIFI_MIB_API_OPERATION_WRITE, 0, &Frequency }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetChannelFrequency <----" );
	return result;
}


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
EWmStatus CWmVwalWrapper::GetChannelFrequency( OUT uint32* pFrequency)
{

	LOGD("CWmVwalWrapper::GetChannelFrequency ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_CURRENTCHANNELFREQUENCY, 
		   WIFI_MIB_API_OPERATION_READ, 0, pFrequency }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetChannelFrequency <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////////
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
EWmStatus CWmVwalWrapper::GetConnectionState( OUT EWmConnectionState* 
											                 pConnectionState )
{

	LOGD("CWmVwalWrapper::GetConnectionState ---->" );
	uint8 u8status         = 0;
	uint32 connectionState = 0;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_CONNECTIONSTATE, WIFI_MIB_API_OPERATION_READ, u8status, 
			                                                  &connectionState}
    };

	uint32 numMibObjects =  sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	


	//Go over with Dudi on States
	if (result == WM_OK)
	{
		switch(connectionState)
		{
		case WIFI_MIB_API_CONNECTION_STATE_STOPPED:
			*pConnectionState = WM_STATE_IDLE;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_SCANNING:
			*pConnectionState = WM_STATE_SCANNING;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_STARTED:
			*pConnectionState = WM_STATE_STARTED;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_CONNECTED:
			*pConnectionState = WM_STATE_CONNECTED;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_RESETTING:
		    *pConnectionState = WM_STATE_RESETTING;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_STARTED_AND_SCANNING:
			*pConnectionState = WM_STATE_SCANNING;
			break;
		case WIFI_MIB_API_CONNECTION_STATE_CONNECTED_AND_SCANNING:
			*pConnectionState = WM_STATE_CONNECTED_AND_SCANNING;
			break;
		
		}
	}
	LOGD("CWmVwalWrapper::GetConnectionState (connectionState=%d)<----", 
				 (*pConnectionState)  );

	return result;


}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetMaxTxPower
/// 
/// 		Sets maximum transmit power.
///			@param[in]  Power power to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMaxTxPower( IN uint32 Power)
{

	LOGD("CWmVwalWrapper::SetMaxTxPower ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_MAXTXPOWER, WIFI_MIB_API_OPERATION_WRITE, 0, &Power }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetMaxTxPower <---- " );
	return result;

}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetMaxTxPower
/// 
/// 		Gets max transmit power.
///			@param[out]  pPower on return will hold max TX power.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetMaxTxPower( OUT uint32* pPower)
{
	LOGD("CWmVwalWrapper::GetMaxTxPower ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_MAXTXPOWER, WIFI_MIB_API_OPERATION_READ, 0, pPower }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                      sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetMaxTxPower <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetAvgRssiWindowSize
/// 
/// 		Sets average RSSI window size (for beacons only).
///			@param[in]  Size Average RSSI window size to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetAvgRssiWindowSize( IN uint32 Size)
{
	LOGD("CWmVwalWrapper::SetAvgRssiWindowSize ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_AVERAGERSSIWINDOWSIZE, 
		   WIFI_MIB_API_OPERATION_WRITE, 0, &Size }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                      sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetAvgRssiWindowSize <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetAvgRssiWindowSize
/// 
/// 		Gets average RSSI window size (of beacons only).
///			@param[out]  pSize on return will hold average RSSI window
/// 						size.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAvgRssiWindowSize( OUT uint32* pSize)
{
	LOGD("CWmVwalWrapper::GetAvgRssiWindowSize ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_AVERAGERSSIWINDOWSIZE, 
		   WIFI_MIB_API_OPERATION_READ, 0, pSize }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                      sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetAvgRssiWindowSize <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetAvgRssiDataWindowSize 
/// 
/// 		Sets average RSSI data window size (for data).
///			@param[in]  Size Average RSSI data window size to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetAvgRssiDataWindowSize( IN uint32 Size)
{
	LOGD("CWmVwalWrapper::SetAvgRssiDataWindowSize ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_AVERAGERSSIDATAWINDOWSIZE, 
		   WIFI_MIB_API_OPERATION_WRITE, 0, &Size }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetAvgRssiDataWindowSize <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetAvgRssiDataWindowSize
/// 
/// 		Gets average RSSI data window size (for data).
///			@param[out]  pSize on return will hold average data RSSI 
/// 						window size.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAvgRssiDataWindowSize( OUT uint32* pSize)
{
	LOGD("CWmVwalWrapper::GetAvgRssiDataWindowSize ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_AVERAGERSSIDATAWINDOWSIZE, 
		   WIFI_MIB_API_OPERATION_READ, 0, pSize }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetAvgRssiDataWindowSize <---- " );
	return result;
}



///////////////////////////////////////////////////////////////////////////////
///@brief		GetAvgRssi
/// 		    Gets average RSSI of recieved beacons and probe requests frames.
///				  
///@param[out] pRssi On return will hold the Rssi value.
///             
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///
EWmStatus CWmVwalWrapper::GetAvgRssi( OUT uint32* pRssi )
{

	LOGD("CWmVwalWrapper::GetAvgRssi ---->" );
	
	
	
    mibObjectContainer_t mibObjectContainer[] = 
        {
			{MIBOID_AVERAGERSSI,  WIFI_MIB_API_OPERATION_READ,  0, pRssi}
        };

	uint32    numMibObjects =  
				sizeof(mibObjectContainer)/sizeof(mibObjectContainer_t);

	EWmStatus result        =  
				DoWifiMibApiCall(mibObjectContainer, numMibObjects);	
	
	
	LOGD("CWmVwalWrapper::GetAvgRssi <----");
				  
	return result;

}


///////////////////////////////////////////////////////////////////////////////
///	@brief	GetAvgRssiData
/// 
/// 		Gets average RSSI of recieved data frames.
///			@param[out]  pRssi average rssi.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAvgRssiData( OUT uint32* pRssi )
{

	LOGD("CWmVwalWrapper::GetAvgRssiData ---->" );
	
	
	
    mibObjectContainer_t mibObjectContainer[] = 
        {
			{MIBOID_AVERAGERSSIDATA,  WIFI_MIB_API_OPERATION_READ,  0, pRssi}
        };

	uint32    numMibObjects =  
				sizeof(mibObjectContainer)/sizeof(mibObjectContainer_t);

	EWmStatus result        =  
				DoWifiMibApiCall(mibObjectContainer, numMibObjects);	
	
	
	LOGD("CWmVwalWrapper::GetAvgRssiData <----");
				  
	return result;

}

///////////////////////////////////////////////////////////////////////////////
///@brief		GetStatistics
/// 			Get statistics.
///@param[out]	pStatistics On return holds Statistics values.
///             
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///
EWmStatus CWmVwalWrapper::GetStatistics( OUT WmStatistics* pStatistics ) 
{
	LOGD("CWmVwalWrapper::GetStatistics ---->" );


	EWmStatus result = WM_OK;
	
    mibObjectContainer_t mibObjectContainer[] = 
        {
			{MIBOID_FAILEDCOUNT,           WIFI_MIB_API_OPERATION_READ,  
			 0,                                 &pStatistics->FailedCount  },

			{MIBOID_RETRYCOUNT,            WIFI_MIB_API_OPERATION_READ,  
			 0, 								&pStatistics->RetryCount   },

			{MIBOID_FCSERRORCOUNT,         WIFI_MIB_API_OPERATION_READ,  
			 0, 								&pStatistics->FcsErrorCount},

			{MIBOID_TRANSMITTEDFRAMECOUNT, WIFI_MIB_API_OPERATION_READ,  
			 0,                         &pStatistics->TransmittedFrameCount},

			{MIBOID_AVERAGERSSI,                WIFI_MIB_API_OPERATION_READ,  
			0, 									&pStatistics->AverageRssi  }
        };

	uint32 numMibObjects =  
		sizeof(mibObjectContainer)/sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	
	

	LOGD("CWmVwalWrapper::GetStatistics result=%d<----",result );

	return result;
}

#ifndef ANDROID	
///////////////////////////////////////////////////////////////////////////////
///@brief		GetScanListSize
///      
///				Gets scan list size.
///
///             
///				Calls Wifi Mib API with the following Mib Object parameters:
///				mibObjectID = MIBOID_SCANLISTSIZE;
///             operation   = WIFI_MIB_API_OPERATION_READ;    
///             mibObject   = pScanListSize;
///				
///@param[out]  pScanListSize on return will hold the value of the scanned list 
///				size.
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		


EWmStatus CWmVwalWrapper::GetScanListSize( OUT uint32* pScanListSize )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListSize ---->" ) );
	*pScanListSize = 0;
	uint8 u8status = 0;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{ MIBOID_SCANLISTSIZE        , 
		  WIFI_MIB_API_OPERATION_READ, 
			u8status                 , 
			pScanListSize            }
    };

	uint32 numMibObjects = sizeof(mibObjectContainer)/
									sizeof(mibObjectContainer_t);
	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListSize <---- (result=%d,"
				  " pScanListSize=%d)",result, *pScanListSize ) );
	return result;
}



////////////////////////////////////////////////////////////////////////////////
///@brief		GetScanListInfo
///      
///				Gets scan list AP's info. It is the user responsibility
///				to allocate enough memory for ScanListInfoStructArray. In 
///				particular, its size should be set to the value returned in
///				GetScanListSize.
///             For the different MIB objects see the private functions
///				below.
///
///@param[out]  pScanListInfoArray on return holds AP's info: BSSID, SSID, 
///				Channel, etc. 
///				
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///
#define ENCRYPTION_ENABLED_MASK   0x10
EWmStatus CWmVwalWrapper::GetScanListInfo( 
									OUT WmScanListInfo* pScanListInfoArray, 
									IN  uint8           Size              )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListInfo ---->" ) );
	
	MAC_ADDRESS bssid;
	uint32   rssi       	 = 0;
	uint32   capabilities 	 = 0;
	uint32   channel         = 0;
	uint32   sizeOfIe        = 0;
	uint8 ie[WIFI_MAX_VAR_IE_LEN];	

	uint8    u8status   	 = 0;
	EWmStatus result     	 = WM_OK;

	//STILL NEED TO ADD BAND FIELD...
	for (uint32 scanListIndex=1; scanListIndex<=Size; scanListIndex++)
	{
	
		WmScanListInfo* currentInfo = &(pScanListInfoArray[scanListIndex-1]);

		mibObjectContainer_t mibObjectContainer[] =
		{
			{MIBOID_SCANLISTINDEX,    			WIFI_MIB_API_OPERATION_WRITE, 
				u8status, &scanListIndex},

			{MIBOID_SCANLISTBSSID,    			WIFI_MIB_API_OPERATION_READ,  
				u8status, &bssid},

			{MIBOID_SCANLISTRSSI,     			WIFI_MIB_API_OPERATION_READ,  
				u8status, &rssi},

			{MIBOID_SCANLISTCHANNEL,  			WIFI_MIB_API_OPERATION_READ,  	
				u8status, &channel},

			{MIBOID_SCANLISTCAPABILITIES,		WIFI_MIB_API_OPERATION_READ,  
				u8status, &capabilities},

			{MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS, 
				WIFI_MIB_API_OPERATION_READ, u8status, &sizeOfIe},

			{MIBOID_SCANLISTINFORMATIONELEMENTS,		
				WIFI_MIB_API_OPERATION_READ, u8status, &ie},
			
    	};

		uint32 numMibObjects = 
			sizeof(mibObjectContainer)/sizeof(mibObjectContainer_t);

		result = DoWifiMibApiCall(mibObjectContainer,numMibObjects);    

		if(result != WM_OK)
			return result;

		
		WmMacAddress_p wmBssid     = bssid;
		currentInfo->Bssid         = (WmMacAddress)wmBssid;
		currentInfo->Rssi    	   = rssi;
		currentInfo->Channel       = channel;


		currentInfo->AuthType   = 0;
		currentInfo->Capabilities = 0;
		currentInfo->SupportedCiphers = 0;
		currentInfo->PsMode = WM_PS_MODE_LEGACY;
        currentInfo->Country       = WM_COUNTRY_NOT_SUPPORTED;

        CWmIEParser::ParseInformationElement(ie, sizeOfIe, *currentInfo);
		//if encryption is enabled (5th bit of capabilities is on) and the AP 
		//does not use WPA/WPA2 then it must support WEP.

		if( currentInfo->AuthType == 0 )
		{
			if( (capabilities & ENCRYPTION_ENABLED_MASK) > 0 )
			{
				currentInfo->AuthType |= WM_AUTHTYPE_WEP;
			}
			else
			{
				currentInfo->AuthType |= WM_AUTHTYPE_OPEN;
			}
		}

//		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
//				  "CWmVwalWrapper::GetScanListInfo currentInfo->AuthType = %d"
//					  ,currentInfo->AuthType ) );

	}

	WmUtils::PrintScanList(pScanListInfoArray,Size);

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListInfo <----" ) );

	return result;
}




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
EWmStatus CWmVwalWrapper::GetScanListInfo( 
									OUT WmScanListInfo* pScanListInfoArray, 
									IN WmMacAddress* pBssid )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListInfo ---->" ) );
	
	MAC_ADDRESS bssid;
	uint32   rssi       	 = 0;
	uint32   capabilities 	 = 0;
	uint32   channel         = 0;
	uint32   sizeOfIe        = 0;
	uint32   scanListSize    = 0;
	uint8 ie[WIFI_MAX_VAR_IE_LEN];	

	uint8    u8status   	 = 0;
	EWmStatus result     	 = WM_OK;
	bool foundBssid          = false;
	
	result = GetScanListSize(&scanListSize);

	
	for (uint32 scanListIndex=1; scanListIndex<=scanListSize; scanListIndex++)
	{
	
		WmScanListInfo* currentInfo = pScanListInfoArray;

		mibObjectContainer_t mibObjectContainer[] =
		{
			{MIBOID_SCANLISTINDEX,    			WIFI_MIB_API_OPERATION_WRITE, 
				u8status, &scanListIndex},

			{MIBOID_SCANLISTBSSID,    			WIFI_MIB_API_OPERATION_READ,  
				u8status, &bssid},

			{MIBOID_SCANLISTRSSI,     			WIFI_MIB_API_OPERATION_READ,  
				u8status, &rssi},

			{MIBOID_SCANLISTCHANNEL,  			WIFI_MIB_API_OPERATION_READ,  	
				u8status, &channel},

			{MIBOID_SCANLISTCAPABILITIES,		WIFI_MIB_API_OPERATION_READ,  
				u8status, &capabilities},

			{MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS, 
				WIFI_MIB_API_OPERATION_READ, u8status, &sizeOfIe},

			{MIBOID_SCANLISTINFORMATIONELEMENTS,		
				WIFI_MIB_API_OPERATION_READ, u8status, &ie},
			
    	};

		uint32 numMibObjects = 
			sizeof(mibObjectContainer)/sizeof(mibObjectContainer_t);

		result = DoWifiMibApiCall(mibObjectContainer,numMibObjects);    

		if(result != WM_OK)
		{
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListInfo "
				  "(failed to get scan list info)<----" ) );
			return result;
		}

        
		
		WmMacAddress_p wmBssid     = bssid;
		currentInfo->Bssid         = (WmMacAddress)wmBssid;

		if(currentInfo->Bssid == *pBssid)
		{
		
			foundBssid                 = true;
			currentInfo->Rssi    	   = rssi;
			currentInfo->Channel       = channel;


			currentInfo->AuthType   = 0;
			currentInfo->Capabilities = 0;
			currentInfo->SupportedCiphers = 0;
			currentInfo->PsMode = WM_PS_MODE_LEGACY;
            currentInfo->Country       = WM_COUNTRY_NOT_SUPPORTED;
			CWmIEParser::ParseInformationElement(ie, sizeOfIe, *currentInfo);

            //if encryption is enabled (5th bit of capabilities is on) and the AP 
			//does not use WPA/WPA2 then it must support WEP.
	
			if( currentInfo->AuthType == 0 )
			{
				if( (capabilities & ENCRYPTION_ENABLED_MASK) > 0 )
				{
					currentInfo->AuthType |= WM_AUTHTYPE_WEP;
				}
				else
				{
					currentInfo->AuthType |= WM_AUTHTYPE_OPEN;
				}
			}
			break;
		}

	}

	if( !foundBssid )
	{
		result = WM_BSSID_NOT_IN_SCAN_LIST;
	}
	
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::GetScanListInfo <----" ) );

	return result;
}



/////  P R I V A T E   F U N C T I O N S  //////////////////////////////////////

#define EVENTS_BUFFER_SIZE     10

void CWmVwalWrapper::SignalHandler(int num)
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmVwalWrapper::SignalHandler ---->" ) );

	if (SIGIO == num)
	{
		wifiEvent_t buffer[EVENTS_BUFFER_SIZE];
		int count, pos, numEvents;

		// Read 1-10 events from the Wi-Fi Event Queue 
		count = read( m_Fd, buffer, sizeof( buffer ) );

		if (count < 0)
		{
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_ERROR,
						  "Could not read events from queue") );

			///////// Temporary patch for IL#2534
			TimerScanEvent(CWmVwalWrapper::GetInstance());

			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  " CWmVwalWrapper::SignalHandler <----" ) );
			return;
		}

		numEvents = count / sizeof( wifiEvent_t );

		// The Wi-Fi Event Queue should never contain partial events.
		INFRA_ASSERT( 0 == ( count % sizeof( wifiEvent_t ) ) );

		// iterate through the list of 1-10 events
		for( pos = 0 ; pos < numEvents ; ++pos )
		{
			switch ( buffer[pos].eventID )
			{
				case WIFI_EVENT_STAASSOC:
					INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
								  "WIFI_EVENT_STAASSOC\n") );
					break;

				case WIFI_EVENT_STADISASSOC:
					INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
								  "WIFI_EVENT_STADISASSOC"
								  " reason=%d\n", buffer[pos].reason) );
					break;

				case WIFI_EVENT_STASCANCOMPLETE:
					INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
								  "WIFI_EVENT_STASCANCOMPLETE\n") ); 

					CWmVwalWrapper::GetInstance()->HandleOnScanEvent(WM_OK);
					
					break;

				default:
					INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
								  "Unknown WIFI_EVENT\n") );
					break;
			}
		}
	}
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  " CWmVwalWrapper::SignalHandler <----" ) );
}
#endif

////////////////////////////////////////////////////////////////////////////////
//@brief    DisplayMIBObjContainerStatuses	
//
// 			Displays index number of MIB object in which the status field is
// 			not equal to WIFI_MIB_API_SUCCESS.

void CWmVwalWrapper::DisplayMIBObjContainerStatuses( 
	mibObjectContainer_t* mibObjectContainer, 
	int 				  numMibObjects     )
{

	LOGD("CWmVwalWrapper::DisplayMIBObjContainerStatuses --->" );
	for (int i = 0; i < numMibObjects; i++)
    {
        if (mibObjectContainer[i].status != WIFI_SUCCESS )
        {

			LOGD("Mib Object number %d "
						"had an error status value of %d", 
						  i+1,
						  (int)mibObjectContainer[i].status );
			
        }
    }
	LOGD("CWmVwalWrapper::DisplayMIBObjContainerStatuses <---" );
}


////////////////////////////////////////////////////////////////////////////////
///@brief		DoWifiMibApiCall
/// 
/// 			Does the actual call to DoWifiMibApi.
/// 
EWmStatus CWmVwalWrapper::DoWifiMibApiCall( 
										mibObjectContainer_t* mibObjectArray, 
										uint32 				  num          ){

	LOGD("CWmVwalWrapper::DoWifiMibApiCall ---->" );

	int32 wifiMibApiResult =
    	WifiMibAPI(m_DeviceName, mibObjectArray, num);
        
    if (wifiMibApiResult != WIFI_SUCCESS)
    {
		DisplayMIBObjContainerStatuses(mibObjectArray, num);

		LOGD("CWmVwalWrapper::DoWifiMibApiCall <----\n\tError = %lu", wifiMibApiResult );


		return (WM_ERROR);
		
    }
    else
    {
		LOGD("CWmVwalWrapper::DoWifiMibApiCall <----" );

		return (WM_OK);
		
    }

} 

#ifndef ANDROID
void CWmVwalWrapper::HandleOnDisonnectEvent( EWmStatus status )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnDisonnectEvent ---->" ) );

	CInfraPtrListIterator Iter(&m_VwClientsList);
	IDrvWrapperClient* pClient = (IDrvWrapperClient*)Iter.GetNext();

	
	while( NULL != pClient ) 
	{
		pClient->WmDrvWrapperOnDisconnect(status);
		pClient = (IDrvWrapperClient*)Iter.GetNext();
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnDisonnectEvent <----" ) );

}

void CWmVwalWrapper::HandleOnConnectEvent( EWmStatus status )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnConnectEvent ---->" ) );

	CInfraPtrListIterator Iter(&m_VwClientsList);
	IDrvWrapperClient* pClient = (IDrvWrapperClient*)Iter.GetNext();

	
	while( NULL != pClient ) 
	{
		pClient->WmDrvWrapperOnConnect(status);
		pClient = (IDrvWrapperClient*)Iter.GetNext();
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnConnectEvent <----" ) );

}
void CWmVwalWrapper::HandleOnScanEvent( EWmStatus status )
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnScanEvent ---->" ) );

	CInfraPtrListIterator Iter(&m_VwClientsList);
	IDrvWrapperClient* pClient = (IDrvWrapperClient*)Iter.GetNext();

	
	while( NULL != pClient ) 
	{
		pClient->WmDrvWrapperClientOnScan(status);
		pClient = (IDrvWrapperClient*)Iter.GetNext();
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleOnScanEvent <----" ) );

}


void CWmVwalWrapper::HandleEvent(EWmDriverEvent event, EWmStatus status)
{
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleEvent ---->" ) );

	switch	( event ) 
	{
	case WM_DRV_WRAPPER_EVENT_ON_SCAN:
		{
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
					  "CWmVwalWrapper::HandleEvent - "
					  "Event type(WM_DRV_WRAPPER_EVENT_ON_SCAN)") );

			HandleOnScanEvent( status );
		}
		break;
	case WM_DRV_WRAPPER_EVENT_ON_CONNECT:
		{
			
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
					  "CWmVwalWrapper::HandleEvent - "
					  "Event type(WM_DRV_WRAPPER_EVENT_ON_CONNECT)") );
			HandleOnConnectEvent( status );
		}
		break;
	case WM_DRV_WRAPPER_EVENT_ON_DISCONNECT:
    	{
			
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
					  "CWmVwalWrapper::HandleEvent - "
					  "Event type(WM_DRV_WRAPPER_EVENT_ON_DISCONNECT)") );

			HandleOnDisonnectEvent( status );
		}
		break;
		
	default:
	    INFRA_DLOG( ( LM_WIFIMGRLIB, LP_ERROR,
					  "CWmVwalWrapper::HandleEvent - "
					  "Event type(%d) is not supported!!!", event ) );
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::HandleEvent <----" ) );
}




///////////////////////////////////////////////////////////////////////////////
// Temporary - For Driver's Events Emulation.
void CWmVwalWrapper::TimerConnectEvent  ( void * Cookie )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerConnectEvent ---->" ) );

	EWmConnectionState connectionState = WM_STATE_STARTED;

	CWmVwalWrapper  *pVwalWrapper = (CWmVwalWrapper*)Cookie;
 
    

	EWmStatus result  = pVwalWrapper->GetConnectionState(&connectionState);


	if ( (result != WM_OK) || 
		 ((result==WM_OK) && 
		  ((connectionState == WM_STATE_CONNECTED) ||
		   (connectionState == WM_STATE_CONNECTED_AND_SCANNING))) )
	   
	{
		pVwalWrapper->HandleEvent(WM_DRV_WRAPPER_EVENT_ON_CONNECT, result);
	}

	//still connecting
	else
	{
		pVwalWrapper->m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY ,
											 &TimerConnectEvent ,
											 (void*)pVwalWrapper,
											 false)             ; //Periodic
	}
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerConnectEvent <----" ) );

}

void CWmVwalWrapper::TimerScanEvent     ( void * Cookie )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerScanEvent ---->" ) );

	EWmConnectionState connectionState = WM_STATE_SCANNING;

	CWmVwalWrapper     *pVwalWrapper   = (CWmVwalWrapper*)Cookie;
 
    

	EWmStatus result  = pVwalWrapper->GetConnectionState(&connectionState);

	// fail to get connection state or scanning endded
	if (  (result != WM_OK) ||
		 ((result == WM_OK) && 
		  (connectionState != WM_STATE_SCANNING) &&
		  (connectionState != WM_STATE_CONNECTED_AND_SCANNING))  )
	{
		pVwalWrapper->HandleEvent(WM_DRV_WRAPPER_EVENT_ON_SCAN, result);
	}
	// still scanning...
	else
	{
		pVwalWrapper->m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY ,
											 &TimerScanEvent    ,
											 (void*)pVwalWrapper,
											 false)             ; //Periodic
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerScanEvent <----" ) );

}


void CWmVwalWrapper::TimerDisonnectEvent( void * Cookie )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerDisonnectEvent ---->" ) );

	EWmConnectionState connectionState = WM_STATE_RESETTING;

	CWmVwalWrapper     *pVwalWrapper   = (CWmVwalWrapper*)Cookie;
 
    

	EWmStatus result  = pVwalWrapper->GetConnectionState(&connectionState);

	// fail to get connection state or disconnecting endded
	if (  (result != WM_OK) ||
		 ((result == WM_OK) && (connectionState != WM_STATE_RESETTING)) )
	{
		pVwalWrapper->HandleEvent(WM_DRV_WRAPPER_EVENT_ON_DISCONNECT, result);
	}


	// still disconnecting...
	else
	{
		pVwalWrapper->m_Timer.ScheduleTimer( WM_TIMER_FREQUENCY  ,
											 &TimerDisonnectEvent,
											 (void*)pVwalWrapper ,
											 false)              ; //Periodic
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmVwalWrapper::TimerDisonnectEvent <----" ) );

}

#endif

///////////////////////////////////////////////////////////////////////////
///	@brief	SetAntennaDiversity
/// 
/// 		Sets Antenna Diversity
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] pAntennaDiversity antenna diversity to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetAntennaDiversity( IN uint32 antennaDiversity)
{

	LOGD("CWmVwalWrapper::SetAntennaDiversity ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_DIVERSITYSUPPORT, WIFI_MIB_API_OPERATION_WRITE, 0, &antennaDiversity}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetAntennaDiversity <----" );
	return result;

}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetAntennaDiversity
/// 
/// 		Gets Antenna Diversity
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[out] pAntennaDiversity on return will hold antenna diversity value.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAntennaDiversity( OUT uint32* pAntennaDiversity)
{

	LOGD("CWmVwalWrapper::GetAntennaDiversity ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_DIVERSITYSUPPORT, WIFI_MIB_API_OPERATION_READ, 0, pAntennaDiversity }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetAntennaDiversity <---- " );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetAutoPhyRateAdoptation
/// 
/// 		Sets Auto Rate adoptation settings (rate fallback)
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] isEnabled enable / disable phy rate adoptation.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetAutoPhyRateAdoptation(IN bool isEnabled)
{

	LOGD("CWmVwalWrapper::SetAutoPhyRateAdoptation ---->" );


	uint32 fallbackEnable = 0;

	if(isEnabled) {
		fallbackEnable = 1;
	}

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_FALLBACKENABLE, WIFI_MIB_API_OPERATION_WRITE, 0, &fallbackEnable}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetAutoPhyRateAdoptation <----" );
	return result;

}



///////////////////////////////////////////////////////////////////////////
///	@brief	GetAntennaDiversity
/// 
/// 		Gets Antenna Diversity
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in] pIsEnabled on return will phy rate adoptation settings.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAutoPhyRateAdoptation( OUT bool* pIsEnabled)
{

	LOGD("CWmVwalWrapper::GetAutoPhyRateAdoptation ---->" );

	uint32 fallbackEnable = 0;
	mibObjectContainer_t mibObjectContainer[] =
    {
	   { MIBOID_FALLBACKENABLE, WIFI_MIB_API_OPERATION_READ, 0,&fallbackEnable }
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(fallbackEnable) {
		*pIsEnabled = true;
	}
	else {
		*pIsEnabled = false;
	}


	LOGD("CWmVwalWrapper::GetAutoPhyRateAdoptation <---- " );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	WriteRegister
/// 
/// 		Sets the given register Register to hold the given value.
///			@param[in] Register register number.
/// 		@param[in] Value the value that should be set.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::WriteRegister(IN uint32 Register,
										IN uint32 Value   )		
{
	LOGD("CWmVwalWrapper::WriteRegister ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_MAC_REG_ACCESS_ADDR, WIFI_MIB_API_OPERATION_WRITE, 0, &Register}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK) {

		return result;
	}

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_MAC_REG_ACCESS_DWORD, WIFI_MIB_API_OPERATION_WRITE, 0, &Value}

	};

	numMibObjects = sizeof(mibObjectContainer1)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

	LOGD("CWmVwalWrapper::WriteRegister <----" );
	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	WriteBBRegister
/// 
/// 		Sets the given baseband register Register to hold the given value.
///			@param[in] Register register number.
/// 		@param[in] Value the value that should be set.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::WriteBBRegister(IN uint32 Register,
										  IN uint8 Value   )
{

	LOGD("CWmVwalWrapper::WriteBBRegister ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_MAC_REG_ACCESS_ADDR, WIFI_MIB_API_OPERATION_WRITE, 0, &Register}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK) {

		return result;
	}

	uint32 Value32 = Value;

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_MAC_REG_ACCESS_DWORD, WIFI_MIB_API_OPERATION_WRITE, 0, &Value32}

	};

	numMibObjects = sizeof(mibObjectContainer1)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

    
	LOGD("CWmVwalWrapper::WriteBBRegister <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	GetPs
/// 
///			Get whether Power Save is on or off.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetPs( OUT bool* pIsPSEnabled)
{

	LOGD("CWmVwalWrapper::GetPs ---->" );

	uint32 psMode;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_MODE, WIFI_MIB_API_OPERATION_READ, 
			0, &psMode}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	if(psMode == WIFI_POWERSAVEMODE_ON)
	{
		*pIsPSEnabled = true;
	}
	else
	{
		*pIsPSEnabled = false;

	}

	LOGD("CWmVwalWrapper::GetPs <----" );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	EnablePs
/// 
///			Enables Power Save.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::EnablePs()
{

	LOGD("CWmVwalWrapper::EnablePs ---->" );

	uint32 psMode = WIFI_POWERSAVEMODE_ON;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_MODE, WIFI_MIB_API_OPERATION_WRITE, 
			0, &psMode}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::EnablePs <----" );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	DisablePs
/// 
///			Disables Power Save.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::DisablePs()
{
	LOGD("CWmVwalWrapper::DisablePs ---->" );

	uint32 psMode = WIFI_POWERSAVEMODE_OFF;
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_MODE, WIFI_MIB_API_OPERATION_WRITE, 
			0, &psMode}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::DisablePs <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetPsAction
/// 
/// 		Sets the action the driver might take when it has an 
/// 	   	opportunity to  sleep (i.e. there is no downlink traffic 
/// 		buffered.
///			@param[in]  psAction power save mode to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetPsAction( IN EWmPsAction psAction )
{
	LOGD("CWmVwalWrapper::SetPsAction ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_ACTION, WIFI_MIB_API_OPERATION_WRITE, 
			0, &psAction}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	LOGD("CWmVwalWrapper::SetPsAction <----" );


	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	GetPsAction
/// 
/// 		Gets power save action.
///			@param[out]  pPsAction on return will hold power save action.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetPsAction( OUT EWmPsAction* pPsAction)
{
	LOGD("CWmVwalWrapper::GetPsAction ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_ACTION, WIFI_MIB_API_OPERATION_READ, 
			0, pPsAction}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetPsAction <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetUapsdMaxSpLength
/// 
/// 		Sets UAPSD max service period length.
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in]  maxSpLength max service period to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetUapsdMaxSpLength( 
	IN EWmUapsdMaxSpLen maxSpLength )
{
	LOGD("CWmVwalWrapper::SetUapsdMaxSpLength ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_UAPSDMAXSPLEN, WIFI_MIB_API_OPERATION_WRITE, 
			0, &maxSpLength}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetUapsdMaxSpLength <----" );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetsUapsdMaxSpLength
/// 
/// 		Gets UAPSD max service period length.
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[out]  pMaxSpLength on return will hold max service period.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetUapsdMaxSpLength( 
	OUT EWmUapsdMaxSpLen* pMaxSpLength )
{
	LOGD("CWmVwalWrapper::GetUapsdMaxSpLength ---->" );

	
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_UAPSDMAXSPLEN, WIFI_MIB_API_OPERATION_READ, 
			0, pMaxSpLength}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetUapsdMaxSpLength <----" );
	return result;
}


////////////////////////////////////////////////////////////////////////////////
///	@brief	SetAcPsMode
/// 
/// 		Sets PS mode (WM_PS_MODE_UAPSD or WM_PS_MODE_LEGACY of the given AC).
/// 		IMPORTANT - Must be called from disconnected state. 
///			@param[in]  psMode power save mode to be set. Legal values are 
/// 					WM_PS_MODE_UAPSD and WM_PS_MODE_LEGACY only.
/// 
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
/// 
EWmStatus CWmVwalWrapper::SetPsMode( IN uint32 psMode)
{
	LOGD("CWmVwalWrapper::SetPsMode ---->" );

	
	if ( (psMode < 0) || (psMode > 1) ) 
	{

		LOGD("CWmVwalWrapper::SetAcPsMode <----" );

		return WM_ILLEGAL_PS_MODE;
	}
	

	uint32	uapsd = 0;

	uapsd = psMode;

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_POWERMGMT_UAPSDCONFIG, WIFI_MIB_API_OPERATION_WRITE, 
			0, &uapsd}
	};
	
	uint32 numMibObjects = sizeof(mibObjectContainer1)/
									  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

	LOGD("CWmVwalWrapper::SetAcPsMode <----" );

	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetAcPsMode
/// 
/// 		Gets PS mode (UAPSD/LEGACY of the given AC).
///			@param[out] pPsMode on return will hold AC's power save mode.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///     
EWmStatus CWmVwalWrapper::GetPsMode( OUT uint32* pPsMode )
{
	LOGD("CWmVwalWrapper::GetPsdMode ---->" );

	uint32	uapsd = 0;
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_UAPSDCONFIG, WIFI_MIB_API_OPERATION_READ, 
			0, &uapsd}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	*pPsMode = WM_PS_MODE_LEGACY;

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	*pPsMode = uapsd;

	LOGD("CWmVwalWrapper::GetAcUapsdMode <----" );

	return result;

}
///////////////////////////////////////////////////////////////////////////
///	@brief	SendNullPacket
/// 
///			Sends QoS Data-Null frame
/// 		@param[in] Freq the frequency in which power mgmt packets will be 
/// 				   sent.
/// 		IMPORTANT - Must be called from connected state. 
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SendNullPacket(uint32 Freq)
{
	LOGD("CWmVwalWrapper::SendNullPacket ---->" );

	//sahron
	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_POWERMGMT_AUTO_TRIGGER, WIFI_MIB_API_OPERATION_WRITE, 
			0, &Freq}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SendNullPacket <----" );
	return result;

}


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
EWmStatus CWmVwalWrapper::ReadRegister(IN  uint32 Register,
									   OUT uint32*   Value   )
{
	LOGD("CWmVwalWrapper::ReadRegister ---->" );

    mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_MAC_REG_ACCESS_ADDR, WIFI_MIB_API_OPERATION_WRITE, 0, &Register}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK) {

		return result;
	}

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_MAC_REG_ACCESS_DWORD, WIFI_MIB_API_OPERATION_READ, 0, Value}

	};

	numMibObjects = sizeof(mibObjectContainer1)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

	LOGD("CWmVwalWrapper::ReadRegister <----" );
	return result;
}


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
EWmStatus CWmVwalWrapper::ReadBBRegister(IN  uint32 Register,
										 OUT uint8*   Value   )
{

	LOGD("CWmVwalWrapper::ReadBBRegister ---->" );

    mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_MAC_REG_ACCESS_ADDR, WIFI_MIB_API_OPERATION_WRITE, 0, &Register}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK) {

		return result;
	}

	uint32 Value32;

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_MAC_REG_ACCESS_DWORD, WIFI_MIB_API_OPERATION_READ, 0, &Value32}

	};

	numMibObjects = sizeof(mibObjectContainer1)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

	*Value = (uint8)Value32;

	LOGD("CWmVwalWrapper::ReadBBRegister <----" );
	return result;


}

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
EWmStatus CWmVwalWrapper::SetTxPacket(IN WmTxBatchPacket* pTxBatchPacket)
{
	LOGD("CWmVwalWrapper::SetTxPacket ---->" );

	EWmStatus result;


	if(pTxBatchPacket != NULL) {
	
		dwMibTxBatch_t txDbgPkt;
	
		txDbgPkt.numTransmits = pTxBatchPacket->numTransmits;
		txDbgPkt.phyRate 	  =	pTxBatchPacket->phyRate;
		txDbgPkt.powerLevel   = pTxBatchPacket->txPowerLevel;
		txDbgPkt.pktLen 	  = pTxBatchPacket->packetSize;

		LOGD("CWmVwalWrapper::SetTxPacket (phyRate = %d)" ,txDbgPkt.phyRate );
		LOGD("CWmVwalWrapper::SetTxPacket (numTransmits = %lu)" ,txDbgPkt.numTransmits );
		LOGD("CWmVwalWrapper::SetTxPacket (powerLevel = %d)" ,txDbgPkt.powerLevel );
		LOGD("CWmVwalWrapper::SetTxPacket (pktLen = %d)" ,txDbgPkt.pktLen );



		memset ( txDbgPkt.pkt       , 
				 0x00                     ,
				 sizeof(txDbgPkt.pkt)  );
	
		memcpy(txDbgPkt.pkt,
			   pTxBatchPacket->pkt,
			   pTxBatchPacket->packetSize);
	
		mibObjectContainer_t mibObjectContainer[] =
		{
			{MIBOID_DBGBATCHTX, WIFI_MIB_API_OPERATION_WRITE, 0, &txDbgPkt}
			
		};
	
		uint32   numMibObjects = sizeof(mibObjectContainer)/
													  sizeof(mibObjectContainer_t);
	
		result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);
	}
	else
	{
		result = WM_ERROR;
	}

	LOGD("CWmVwalWrapper::SetTxPacket <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	GenerateTxPackets
///			Start generating packets. Packet prototype was set in SetTxPacket() call.
/// 		Packet generation will continue until all of the packets are transmitted
/// 		or SetTxPacket is called with numTransmits equal to zero
/// 		@return     WM_OK  - function call succeeded.
/// 				   !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GenerateTxPackets()
{
	LOGD("CWmVwalWrapper::GenerateTxPackets ---->" );


	//uint32 mode = DesiredBSSTypeIs80211BurstTx;
	uint32 mode =DesiredBSSTypeIs80211SimpleTxMode;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_BSSTYPE, WIFI_MIB_API_OPERATION_WRITE, 0, &mode}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GenerateTxPackets <----" );
	return result;


}

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
EWmStatus CWmVwalWrapper::ReceivePackets(WmRxWifiPacket rxDesc[], uint8* pNumRxDesc)
{
	//LOGD("CWmVwalWrapper::ReceivePackets ---->" );
	//LOGD("CWmVwalWrapper::ReceivePackets *pNumRxDesc=%d", *pNumRxDesc );

	uint32 Status = Wifi80211APIReceivePkt( (char*)WIFI_DEVICE_NAME, (RX_WIFI_PACKET_DESC*)rxDesc, pNumRxDesc );

	EWmStatus result;

	if(Status == WIFI_SUCCESS)
	{
        result =  WM_OK;
	}
	else 
	{
		result =  WM_ERROR;
	}

	//LOGD("CWmVwalWrapper::ReceivePackets <----" );

	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	StartDriver
///			Starts driver
/// 		@return     WM_OK  - function call succeeded.
/// 				   !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::StartDriver()
{
	LOGD("CWmVwalWrapper::StartDriver ---->" );


	uint32 command  =  WIFI_MIB_API_COMMAND_START;

	//Building the MIB Objects...
	uint8 u8status = 0;
	mibObjectContainer_t mibObjectContainer[] =
    {

		{MIBOID_DSPGCOMMAND, WIFI_MIB_API_OPERATION_WRITE, u8status, &command}
    
    };

	uint32 numMibObjects = sizeof(mibObjectContainer)
	                                             /sizeof(mibObjectContainer_t);

	LOGD("CWmVwalWrapper::StartDriver <----" );

	return  DoWifiMibApiCall(mibObjectContainer, numMibObjects);

}

///////////////////////////////////////////////////////////////////////////////
///@brief	StopDriver
///      	Stops Driver.
///			@return   WM_OK  - function call succeeded.
///   		   		 !WM_OK  - otherwise.	 
/// 
EWmStatus CWmVwalWrapper::StopDriver( void )
{
	LOGD("CWmVwalWrapper::StopDriver <----" );

	
	uint32 command = WIFI_MIB_API_COMMAND_STOP;
    
    mibObjectContainer_t mibObjectContainer[] = 
        {
            {MIBOID_DSPGCOMMAND, WIFI_MIB_API_OPERATION_WRITE, 0, &command}
		};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	LOGD("CWmVwalWrapper::StopDriver <----" );

	return DoWifiMibApiCall(mibObjectContainer,numMibObjects);
    
}
   

///////////////////////////////////////////////////////////////////////////
///	@brief	GetMib
/// 
/// 		Gets the value of the given mib and store it 
/// 		in Value.
///			@param[in]  mib code.
/// 		@param[out] Value on return holds the value that is stored 
/// 					in the mib.
/// 		@return     WM_OK  - function call succeeded.
/// 				   !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetMib(IN  uint32   mibCode,OUT uint32*  Value   )
{
	LOGD(
					  "CWmVwalWrapper::GetMib(uint32) ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_READ, 0, Value}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result    = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	LOGD("CWmVwalWrapper::GetMib(uint32) <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetMib
/// 
/// 		Sets the given mib to hold the given value.
///			@param[in] mib code.
/// 		@param[in] Value the value that should be set.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMib(IN  uint32   mibCode,OUT uint32  Value   )
{
	LOGD("CWmVwalWrapper::SetMib(uint32) ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_WRITE, 0, &Value}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result    = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	LOGD("CWmVwalWrapper::SetMib(uint32) <----" );
	return result;
}

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
EWmStatus CWmVwalWrapper::GetMib(IN  uint32   mibCode,
								 OUT WmMacAddress*  pWmMacAddr   )
{
	LOGD("CWmVwalWrapper::GetMib(macaddr) ---->" );

	if(!pWmMacAddr)
	{
		LOGD( "CWmVwalWrapper::GetMib(macaddr) <---- (pWmMacAddr not iinitialized)" );
		return WM_ILLEGAL_PARAMETER;
	}

	MAC_ADDRESS       bssid;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_READ, 0, &bssid},
	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if (result == WM_OK)
	{
		WmMacAddress_p wmBssid = bssid                ;
		*pWmMacAddr              = (WmMacAddress)wmBssid;

	}

	LOGD("CWmVwalWrapper::GetMib(macaddr) <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetMib
/// 
/// 		Sets the given mib to hold the given value (macaddr).
///			@param[in] mib code.
/// 		@param[in] pWmMacAddr the value that should be set.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMib(IN  uint32   mibCode,
								 IN WmMacAddress*  pWmMacAddr )
{
	LOGD("CWmVwalWrapper::SetMib(macaddr) ---->" );

	if(!pWmMacAddr)
	{
		LOGD( "CWmVwalWrapper::SetMib(macaddr) <---- (pWmMacAddr not iinitialized)" );
		return WM_ILLEGAL_PARAMETER;
	}

	WmMacAddress_p bssid(pWmMacAddr);

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_WRITE, 0,&bssid.ToVwalFormat() },
	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	LOGD("CWmVwalWrapper::SetMib(macaddr) <----" );
	return result;
}





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
EWmStatus CWmVwalWrapper::GetMib(IN  uint32   mibCode,
								 OUT WmSsid*  pWmSsid   )
{
	LOGD("CWmVwalWrapper::GetMib(ssid) ---->" );

	if(!pWmSsid)
	{
		LOGD( "CWmVwalWrapper::GetMib(ssidr) <---- (pWmSsid not iinitialized)" );
		return WM_ILLEGAL_PARAMETER;
	}

	SSID ssid;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_READ, 0,&ssid},
	};

	uint32 numMibObjects = sizeof(mibObjectContainer)
												 /sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	if (result == WM_OK)
	{
		WmSsid_p wmSsid= ssid;
		*pWmSsid = (WmSsid)wmSsid;

	}

	LOGD("CWmVwalWrapper::GetMib(ssid) <----" );
	return result;
}
///////////////////////////////////////////////////////////////////////////
///	@brief	SetMib
/// 
/// 		Sets the given mib to hold the given value (ssid).
///			@param[in] mib code.
/// 		@param[in] pWmSsid the value that should be set.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetMib(IN  uint32   mibCode,
								 IN WmSsid*  pWmSsid ) 
{
	LOGD("CWmVwalWrapper::SetMib(ssid) ---->" );

	if(!pWmSsid)
	{
		LOGD( "CWmVwalWrapper::SetMib(ssid) <---- (pWmSsid not iinitialized)" );
		return WM_ILLEGAL_PARAMETER;
	}

	WmSsid_p ssid(pWmSsid);

	mibObjectContainer_t mibObjectContainer[] =
	{
		{mibCode, WIFI_MIB_API_OPERATION_WRITE, 0,&ssid.ToVwalFormat()},
	};

	uint32 numMibObjects = sizeof(mibObjectContainer)
												 /sizeof(mibObjectContainer_t);


	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	LOGD("CWmVwalWrapper::SetMib(ssid) <----" );
	return result;
}


EWmStatus CWmVwalWrapper::SetDriverMode(IN uint32 mode)
{
	LOGD("CWmVwalWrapper::SetDriverMode ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_BSSTYPE, WIFI_MIB_API_OPERATION_WRITE, 0, &mode}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetDriverMode <----" );
	return result;

}

EWmStatus CWmVwalWrapper::GetDriverMode(OUT uint32* pMode)
{
	LOGD("CWmVwalWrapper::GetDriverMode ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_BSSTYPE, WIFI_MIB_API_OPERATION_READ, 0, pMode}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetDriverMode <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetTxPowerLevelAll
/// 
/// 		Sets transmit power for all channels and frequencies.
///			@param[in]  Power power to be set [0..63].
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetTxPowerLevelAll( IN uint8 power)
{
	LOGD("CWmVwalWrapper::SetTxPowerLevelAll ---->" );

	if(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t))
	{
		LOGD("CWmVwalWrapper::SetTxPowerLevelAll <----(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t)" );
		return WM_ERROR;
	}

	if(power > 63) {
		return WM_ERROR;
	}

	int freqInd,rateInd;

	dwMibPowerTable_t powerTable;

	for(freqInd=0;freqInd<POWERTABLE_MAX_FREQ_NUM;freqInd++) {

		for(rateInd=0;rateInd<POWERTABLE_MAX_RATE_NUM;rateInd++) {

			(powerTable.pt)[freqInd][rateInd] = power;
		}
		
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_WRITE, 0, &powerTable}
		
	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetTxPowerLevelAll <----" );
	return result;
}


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
EWmStatus CWmVwalWrapper::GetTxPowerLevel(IN uint8 freqInd,
										  IN uint8 rateInd,
										  INOUT uint8* pPower)
{

	LOGD("CWmVwalWrapper::GetTxPowerLeve ---->" );

	if(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t))
	{
		LOGD("CWmVwalWrapper::GetTxPowerLeve <----(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t)" );
		return WM_ERROR;
	}

	dwMibPowerTable_t powerTable;

	if(freqInd >= POWERTABLE_MAX_FREQ_NUM) {
		return WM_ERROR;
	}

	if(rateInd >= POWERTABLE_MAX_RATE_NUM) {
		return WM_ERROR;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_READ, 0, &powerTable}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK)
		return result;


	*pPower = (powerTable.pt)[freqInd][rateInd];


	LOGD("CWmVwalWrapper::GetTxPowerLevel <----" );
	return result;

}

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
EWmStatus CWmVwalWrapper::SetTxPowerLevel(IN uint8 freqInd,
										  IN uint8 rateInd,
										  IN uint8 powerLevel)
{

	LOGD("CWmVwalWrapper::SetTxPowerLevel ---->" );


	if(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t))
	{
			LOGD("CWmVwalWrapper::SetTxPowerLevel <---- sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t)" );
			return WM_ERROR;
	}

	dwMibPowerTable_t powerTable;

	if(freqInd >= POWERTABLE_MAX_FREQ_NUM) {
		return WM_ERROR;
	}

	if(rateInd >= POWERTABLE_MAX_RATE_NUM) {
		return WM_ERROR;
	}

	if(powerLevel > 63) {
		return WM_ERROR;
	}


	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_READ, 0, &powerTable}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK)
		return result;


	(powerTable.pt)[freqInd][rateInd] = powerLevel;


	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_WRITE, 0, &powerTable}

	};

	numMibObjects = sizeof(mibObjectContainer1)/
											  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);



	LOGD("CWmVwalWrapper::SetTxPowerLevel <----" );
	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetTxPowerLevelTable
/// 
/// 		Gets transmit power
///			@param[in] pTxPowerLevelTable - power level table
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus 
	CWmVwalWrapper::SetTxPowerLevelTable(IN WmPowerLevelTable* pTxPowerLevelTable)
{
	LOGD("CWmVwalWrapper::SetTxPowerLevelTable ---->" );

	if(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t))
	{
		LOGD("CWmVwalWrapper::SetTxPowerLevelTable <---- sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t)" );
		return WM_ERROR;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_WRITE, 0, pTxPowerLevelTable}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::SetTxPowerLevelTable <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetPhyParameters
/// 
/// 		Set phy parameters
///			@param[in] pMibPhyParameter - Phy parameter struct
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus 
	CWmVwalWrapper::SetPhyParameters(IN WmMibPhyParameter* pMibPhyParameter)
{
	LOGD("CWmVwalWrapper::SetPhyParameters ---->" );

	if(sizeof(WmMibPhyParameter) != sizeof(dwMibPhyParameter_t))
	{
		LOGD("CWmVwalWrapper::SetPhyParameters <---- sizeof(WmMibPhyParameter) != sizeof(dwMibPhyParameter_t)" );
		return WM_ERROR;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_PHYPARAMETER, WIFI_MIB_API_OPERATION_WRITE, 0, pMibPhyParameter}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::SetPhyParameters <----" );
	return result;


}



///////////////////////////////////////////////////////////////////////////
///	@brief	GetTxPowerLevelTable
/// 
/// 		Gets transmit power
///			@param[out] pTxPowerLevelTable - on exit will hold 
/// 										 power level table
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus 
	CWmVwalWrapper::GetTxPowerLevelTable(OUT WmPowerLevelTable* pTxPowerLevelTable)
{
	LOGD("CWmVwalWrapper::GetTxPowerLevelTable ---->" );

	if(sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t))
	{
		LOGD("CWmVwalWrapper::GetTxPowerLevelTable <---- sizeof(WmPowerLevelTable) != sizeof(dwMibPowerTable_t)" );
		return WM_ERROR;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERTABLE, WIFI_MIB_API_OPERATION_READ, 0, pTxPowerLevelTable}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::GetTxPowerLevelTable <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	GetAssociatedMacAddress
/// 
/// 		In connected state, gets the Mac Address of the assocaised AP.
///			@param[out] pBssid on return will hold associated AP's Mac 
/// 					Address.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetAssociatedMacAddress( OUT WmMacAddress* pAddr )
{
	LOGD("CWmVwalWrapper::GetAssociatedMacAddress ---->" );

	MAC_ADDRESS       bssid;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_ASSOCIATEDBSSID, WIFI_MIB_API_OPERATION_READ, 0, &bssid},
	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if (result == WM_OK)
	{
		WmMacAddress_p wmBssid = bssid                ;
		*pAddr                = (WmMacAddress)wmBssid;

		LOGD("CWmVwalWrapper::GetAssociatedMacAddress "
					  "<---- (result=%d)"
					  " MacAddr is : %2x:%2x:%2x:%2x:%2x:%2x",
					  result,
					  wmBssid.Address[0],
					  wmBssid.Address[1],
					  wmBssid.Address[2],
					  wmBssid.Address[3],
					  wmBssid.Address[4],
					  wmBssid.Address[5]);

	}
	else {

		LOGD(
				  "CWmVwalWrapper::GetAssociatedMacAddress "
				  "<---- (result=%d)",result );

	}

	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetSsidForScanning
/// 
/// 		Sets Ssid for scanning. 
///			Stops when the first occurence of the given ssid is found 
///			during a scan.
///			@param[in] pSsid Ssid for scanning
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetSsidForScanning( IN WmSsid* pSsid )
{
	LOGD("CWmVwalWrapper::SetSsidForScanning ---->" );

	if(!pSsid)
	{
		LOGD("CWmVwalWrapper::SetSsidForScanning <---- pSsid not initialized" );
		return 	WM_ILLEGAL_PARAMETER;
	}
	WmSsid_p ssid(pSsid);

    mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SSID_FOR_SCANNING, WIFI_MIB_API_OPERATION_WRITE, 0, 
			&ssid.ToVwalFormat() },
	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetSsidForScanning (result=%d)<----",result );

	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetBssidForScanning
/// 
/// 		Sets Bssid for scanning. 
///			Stops when the first occurence of the given Bssid is found 
///			during a scan.
///			@param[in] pBssid Bssid for scanning
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetBssidForScanning( IN WmMacAddress* pBssid )
{
	LOGD("CWmVwalWrapper::SetBssidForScanning ---->" );

	if(!pBssid)
	{
		LOGD("CWmVwalWrapper::SetBssidForScanning <---- pBssid not initialized" );
		return 	WM_ILLEGAL_PARAMETER;
	}
	WmMacAddress_p bssid(pBssid);

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_BSSID_FOR_SCANNING, WIFI_MIB_API_OPERATION_WRITE, 0, 
			&bssid.ToVwalFormat() },
	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::SetBssidForScanning <----"
				  " (result=%d)",result );
	return result;

}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetTxFrameCount
/// 
///			Gets transmiited frames counts.
///			@param[out] pTxFrameCount on return holds tx frames count.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetTxFrameCount( OUT uint32* pTxFrameCount )
{

	LOGD("CWmVwalWrapper::GetTxFrameCount ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_TRANSMITTEDFRAMECOUNT, WIFI_MIB_API_OPERATION_READ, 
			0, pTxFrameCount}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetTxFrameCount <----" );
	return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	GetRxFrameCount
/// 
///			Gets recieved frames counts.
///			@param[out] pTxFrameCount on return holds rx frames count.
/// 		@return    WM_OK  - function call succeeded.
/// 				  !WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetRxFrameCount( OUT uint32* pRxFrameCount )
{

	LOGD("CWmVwalWrapper::GetRxFrameCount ---->" );

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_RECEIVEDFRAGCOUNT, WIFI_MIB_API_OPERATION_READ, 
			0, pRxFrameCount}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD("CWmVwalWrapper::GetRxFrameCount <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////////
///@brief		GetDesiredApSettings
///@param[out]	pWmDesiredSsid pointer to the AP's desired SSID.
///@param[out]	pWmDesiredBssid pointer to the desired AP's BSSID, i.e.,
/// 				its mac address.
///@return      WM_OK  - function call succeeded.
///   		   !WM_OK  - otherwise.		
///

EWmStatus CWmVwalWrapper::GetDesiredApSettings( OUT WmSsid* pWmDesiredSsid,
												 OUT WmMacAddress* pWmDesiredBssid )
{
	LOGD("CWmVwalWrapper::GetDesiredApSettings ---->" );

	if(!pWmDesiredBssid || !pWmDesiredSsid)
	{
		LOGD("CWmVwalWrapper::GetDesiredApSettings <---- parameter not initialized" );
		return 	WM_ILLEGAL_PARAMETER;
		
	}
	
	//Building MIB Objects...
	uint8 u8status = 0;

	SSID desiredSsid;
	MAC_ADDRESS desiredBssid;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_DESIREDBSSID,  WIFI_MIB_API_OPERATION_READ,u8status,
														&desiredBssid},

		{MIBOID_DESIREDSSID, WIFI_MIB_API_OPERATION_READ, u8status,
														 &desiredSsid},
    
    };

	uint32 numMibObjects = sizeof(mibObjectContainer)
	                                             /sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	if (result == WM_OK)
	{
		WmMacAddress_p wmDesiredBssid = desiredBssid;
		*pWmDesiredBssid           = (WmMacAddress)wmDesiredBssid;
		WmSsid_p wmDesiredSsid= desiredSsid;
		*pWmDesiredSsid = (WmSsid)wmDesiredSsid;

	}

	LOGD("CWmVwalWrapper::GetDesiredApSettings ---->" );

	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	GetScanChanFreqList
/// 
/// 		Gets scan frequencies
///			@param[out] pScanChanFreqList - on exit will hold 
/// 										 list of scan frequencies
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///////////////////////////////////////////////////////////////////////////
EWmStatus 
	CWmVwalWrapper::GetScanChanFreqList(OUT WmScanChanFreqList* pScanChanFreqList)
{
	LOGD("CWmVwalWrapper::GetScanChanFreqList ---->" );

	if(sizeof(WmScanChanFreqList) != sizeof(dwScanChanFreqList_t))
	{
		LOGD("CWmVwalWrapper::GetScanChanFreqList <---- (sizeof(WmScanChanFreqList) != sizeof(dwScanChanFreqList_t)" );
		return 	WM_ERROR;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANCHANFREQLIST, WIFI_MIB_API_OPERATION_READ, 0, pScanChanFreqList}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::GetScanChanFreqList <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetScanChanFreqList
/// 
/// 		Sets scan frequencies
///			@param[in] pScanChanFreqList - list of scan frequencies
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///////////////////////////////////////////////////////////////////////////
EWmStatus 
	CWmVwalWrapper::SetScanChanFreqList(IN WmScanChanFreqList* pScanChanFreqList)
{
	LOGD("CWmVwalWrapper::SetScanChanFreqList ---->" );

	if(sizeof(WmScanChanFreqList) != sizeof(dwScanChanFreqList_t))
	{
		LOGD("CWmVwalWrapper::SetScanChanFreqList <---- (sizeof(WmScanChanFreqList) != sizeof(dwScanChanFreqList_t)" );
		return 	WM_ERROR;
	}

	if(!pScanChanFreqList)
	{
		LOGD("CWmVwalWrapper::SetScanChanFreqList <---- (pScanChanFreqList not initialized)" );
		return 	WM_ILLEGAL_PARAMETER;

	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANCHANFREQLIST, WIFI_MIB_API_OPERATION_WRITE, 0, pScanChanFreqList}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::SetScanChanFreqList <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetScanChannelFreq
/// 
/// 		Sets scan frequencies
///			@param[in] channelFreq - force driver to scan only on this channel
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///////////////////////////////////////////////////////////////////////////
EWmStatus 
	CWmVwalWrapper::SetScanChannelFreq(IN uint32 channelFreq)
{
	LOGD("CWmVwalWrapper::SetScanChannelFreq ---->" );

	dwScanChanFreqList_t chanFreqList;

	chanFreqList.nChanFreq = 1;

	chanFreqList.chanFreq[0] = (uint16)channelFreq;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANCHANFREQLIST, WIFI_MIB_API_OPERATION_WRITE, 0, &chanFreqList}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);



	LOGD("CWmVwalWrapper::SetScanChannelFreq <----" );
	return result;


}

///////////////////////////////////////////////////////////////////////////
///	@brief	AddChannelToScanFreqList
/// 
/// 		Adds channelFreq to  scan frequencies list
///			@param[in] channelFreq - force driver to scan only on this channel
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///////////////////////////////////////////////////////////////////////////
EWmStatus 
	CWmVwalWrapper::AddChannelToScanFreqList(IN uint32 channelFreq)
{
	LOGD("CWmVwalWrapper::AddChannelToScanFreqList ---->" );

	dwScanChanFreqList_t chanFreqList;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANCHANFREQLIST, WIFI_MIB_API_OPERATION_READ, 0, &chanFreqList}

	};

	uint32 numMibObjects = sizeof(mibObjectContainer)/
											  sizeof(mibObjectContainer_t);

	mibObjectContainer_t mibObjectContainer1[] =
	{
		{MIBOID_SCANCHANFREQLIST, WIFI_MIB_API_OPERATION_WRITE, 0, &chanFreqList}

	};


	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

	if(result != WM_OK)
	{
		goto exit;
	}

	if(chanFreqList.nChanFreq == WIFI_SCANCHANNELLIST_MAXCHANNELS)
	{
		result = WM_ERROR;

		goto exit;
	}

	chanFreqList.chanFreq[chanFreqList.nChanFreq] = (uint16)channelFreq;

	chanFreqList.nChanFreq += 1;


	numMibObjects = sizeof(mibObjectContainer1)/
										  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer1, numMibObjects);

exit:

	LOGD("CWmVwalWrapper::AddChannelToScanFreqList <----" );
	return result;


}



//Debug functions
void CWmVwalWrapper::PrintState()
{

	LOGD("CWmVwalWrapper::PrintState ---->" );
	uint8 u8status         = 0;
	uint32 connectionState = 0;

	mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_CONNECTIONSTATE, WIFI_MIB_API_OPERATION_READ, u8status, 
			                                                  &connectionState}
    };

	uint32 numMibObjects =  sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	


	//Go over with Dudi on States
	if (result == WM_OK)
	{
		PrintState(connectionState);
	}
	LOGD("CWmVwalWrapper::PrintState <----" );
}

void CWmVwalWrapper::PrintState(uint32 connectionState)
{

		switch(connectionState)
		{
		case WIFI_MIB_API_CONNECTION_STATE_STOPPED:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_STOPPED" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_SCANNING:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_SCANNING" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_STARTED:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_STARTED" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_CONNECTED:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_CONNECTED" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_RESETTING:
		    LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_RESETTING" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_STARTED_AND_SCANNING:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_STARTED_AND_SCANNING" );
			break;
		case WIFI_MIB_API_CONNECTION_STATE_CONNECTED_AND_SCANNING:
			LOGD(
				"CWmVwalWrapper::PrintState\n\t "
				"WIFI_MIB_API_CONNECTION_STATE_CONNECTED_AND_SCANNING" );
			break;
		
		}
	
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetCountry
/// 
///         Sets the country string in which this Wifi MAC will operate.
///			@param[in]  Country to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetCountry(IN const EWmCountry Country)
{
    LOGD("CWmVwalWrapper::SetCountry ---->" );

    EWmStatus result = WM_OK;


    CountryStringStruct countryString;
        
    result = CWmCountryUtils::EWmCountryToStr(Country, countryString.CountryString);

    LOGD("CWmVwalWrapper::SetCountry countryString.CountryString=%s",countryString.CountryString );

    if (result == WM_OK) {
    
        mibObjectContainer_t mibObjectContainer[] =
        {
            {MIBOID_COUNTRYSTRING, WIFI_MIB_API_OPERATION_WRITE, 0, &countryString}
        };
    
        uint32 numMibObjects =  sizeof(mibObjectContainer)/
                                                      sizeof(mibObjectContainer_t);
    
        result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	
    }

    LOGD("CWmVwalWrapper::SetCountry <---- (result=%d)",result );

    return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	GetCountry
/// 
///         Sets the country string in which this Wifi MAC will operate.
///			@param[in]  pCountry to be set.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::GetCountry(IN EWmCountry* pCountry)
{
    LOGD("CWmVwalWrapper::GetCountry ---->" );

    EWmStatus result = WM_OK;

    CountryStringStruct countryString;
       
    
    
    mibObjectContainer_t mibObjectContainer[] =
    {
        {MIBOID_COUNTRYSTRING, WIFI_MIB_API_OPERATION_READ, 0, &countryString}
    };

    uint32 numMibObjects =  sizeof(mibObjectContainer)/
                                                  sizeof(mibObjectContainer_t);

    result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	

   if (result == WM_OK) 
   {
       *pCountry = CWmCountryUtils::StrToEWmCountry(countryString.CountryString);
   }


    LOGD("CWmVwalWrapper::GetCountry <---- (result=%d, country=%s)",result, countryString.CountryString );

    return result;
}


///////////////////////////////////////////////////////////////////////////
///	@brief	SetScanningMode
/// 
///         Sets scannig mode.
///			@param[in]  ScanMode scan mode.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetScanMode(IN EWmScanMode ScanMode)
{
    LOGD("CWmVwalWrapper::SetScanMode ---->" );

    mibObjectContainer_t mibObjectContainer[] =
    {
		{MIBOID_SCANNINGMODE, WIFI_MIB_API_OPERATION_WRITE, 
			0, &ScanMode}
    };

	uint32   numMibObjects = sizeof(mibObjectContainer)/
		                                          sizeof(mibObjectContainer_t);

	EWmStatus result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);

    LOGD("CWmVwalWrapper::SetScanMode result=%d<----",result );

    return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetNumProbes
/// 
///         Sets num probes for scannig
///			@param[in]  NumProbes num probes.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetNumProbes(IN uint32 NumProbes)
{
	LOGD(
					  "CWmVwalWrapper::SetNumProbes ---->" );

	if( (NumProbes<WM_MIN_SCAN_NUM_PROBES) || (NumProbes>WM_MAX_SCAN_NUM_PROBES) ) 
	{
		LOGD(
				  "CWmVwalWrapper::SetNumProbes <----" );
		return WM_ILLEGAL_PARAMETER;
	}
	EWmStatus result = WM_OK;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_NUMPROBE_FOR_SCANNING, WIFI_MIB_API_OPERATION_WRITE, 0, &NumProbes}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD(
				  "CWmVwalWrapper::SetNumProbes <----" );
	return result;

}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetDwellTime
/// 
///         Sets scan dwell time.
///			@param[in]  Time scan dwell time
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetScanDwellTime(IN uint32 Time)
{
	LOGD(
					  "CWmVwalWrapper::SetScanDwellTime ---->" );

	if( (Time<WM_MIN_SCAN_DWELL_TIME) || (Time>WM_MAX_SCAN_DWELL_TIME) ) 
	{
		LOGD(
				  "CWmVwalWrapper::SetScanDwellTime <----" );
		return WM_ILLEGAL_PARAMETER;
	}
	EWmStatus result = WM_OK;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANMAXDWELLTIME, WIFI_MIB_API_OPERATION_WRITE, 0, &Time}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD(
				  "CWmVwalWrapper::SetScanDwellTime <----" );
	return result;
}

///////////////////////////////////////////////////////////////////////////
///	@brief	SetScanHomeTime
/// 
///         Sets scan home time
///			@param[in]  Time scan home time.
///			@return  WM_OK  - function call succeeded.
///   		   		!WM_OK  - otherwise.		
///		
EWmStatus CWmVwalWrapper::SetScanHomeTime(IN uint32 Time)
{

	LOGD(
					  "CWmVwalWrapper::SetScanHomeTime ---->" );

	if( (Time<WM_MIN_SCAN_HOME_TIME) || (Time>WM_MAX_SCAN_HOME_TIME) ) 
	{
		LOGD(
				  "CWmVwalWrapper::SetScanHomeTime <----" );

		return WM_ILLEGAL_PARAMETER;
	}
	EWmStatus result = WM_OK;

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_SCANHOMEDWELLTIME, WIFI_MIB_API_OPERATION_WRITE, 0, &Time}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD(
				  "CWmVwalWrapper::SetScanHomeTime <----" );
	return result;
}


EWmStatus CWmVwalWrapper::SetPsPowerLevel(IN uint32 PowerLevel)
{

	LOGD("CWmVwalWrapper::SetPsPowerLevel ---->" );

	
	EWmStatus result = WM_OK;

	if( ( PowerLevel<MIBOID_POWERMGMT_LEVEL_MIN) || 
		( PowerLevel>MIBOID_POWERMGMT_LEVEL_MAX ) ) 
	{
		LOGD("CWmVwalWrapper::SetPsPowerLevel <----" );

		return WM_ILLEGAL_PARAMETER;
	}

	mibObjectContainer_t mibObjectContainer[] =
	{
		{MIBOID_POWERMGMT_LEVEL, WIFI_MIB_API_OPERATION_WRITE, 0, &PowerLevel}

	};

	uint32   numMibObjects = sizeof(mibObjectContainer)/
												  sizeof(mibObjectContainer_t);

	result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);


	LOGD(
				  "CWmVwalWrapper::SetPsPowerLevel <----" );
	return result;
}


EWmStatus CWmVwalWrapper::GetPsPowerLevel(IN uint32* pPowerLevel)
{
	LOGD(
				"CWmVwalWrapper::GetPsPowerLevel ---->" );


	EWmStatus result = WM_OK;

    mibObjectContainer_t mibObjectContainer[] =
    {
        {MIBOID_POWERMGMT_LEVEL, WIFI_MIB_API_OPERATION_READ, 0,pPowerLevel}
    };

    uint32 numMibObjects =  sizeof(mibObjectContainer)/
                                                  sizeof(mibObjectContainer_t);

    result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	

   

    LOGD(
				"CWmVwalWrapper::GetPsPowerLevel <----" );

    return result;
}

EWmStatus CWmVwalWrapper::GetTMdata(OUT WmMibTMdata* pWmMibTMdata)
{

#ifdef TM_SUPPORTED
	LOGD("CWmVwalWrapper::GetTMdata ---->" );

	if(!pWmMibTMdata)
	{
		LOGD("CWmVwalWrapper::GetTMdata <---- (pWmMibTMdata not initialized)" );
		return 	WM_ILLEGAL_PARAMETER;
	}


	EWmStatus result = WM_OK;

    mibObjectContainer_t mibObjectContainer[] =
    {
        {MIBOID_TM_MONITOR_DATA, WIFI_MIB_API_OPERATION_READ, 0, (dwMibTM_t*) pWmMibTMdata}
    };

    uint32 numMibObjects =  sizeof(mibObjectContainer)/
                                                  sizeof(mibObjectContainer_t);

    result = DoWifiMibApiCall(mibObjectContainer, numMibObjects);	

       
	LOGD( "==>type > %d \n", pWmMibTMdata->type);  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>pWmMibTMdata_ave_A > %d \n", pWmMibTMdata->RxAccRssiA));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>pWmMibTMdata_ave_B > %d \n", pWmMibTMdata->RxAccRssiB));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>pWmMibTMdata_ave_A > %d \n", pWmMibTMdata->TxNAtmp));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>pWmMibTMdata_ave_B > %d \n", pWmMibTMdata->TxFail));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>RxFCSErr_TxAccOK > %d \n", pWmMibTMdata->RxFCSErr));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>RxFCSErr_TxAccOK > %d \n", pWmMibTMdata->RxFCSErr));  //[DE]
	LOGD( "==>NumberOfPAckets > %lu \n", pWmMibTMdata->NumberOfPAckets);  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_1_Mb, > %d \n", pWmMibTMdata->RateHistogram[1]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_2_Mb, > %d \n", pWmMibTMdata->RateHistogram[2]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_55_Mb, > %d \n", pWmMibTMdata->RateHistogram[3]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_6_Mb, > %d \n", pWmMibTMdata->RateHistogram[4]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_9_Mb, > %d \n", pWmMibTMdata->RateHistogram[5]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_11_Mb, > %d \n", pWmMibTMdata->RateHistogram[6]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_12_Mb, > %d \n", pWmMibTMdata->RateHistogram[7]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_18_Mb, > %d \n", pWmMibTMdata->RateHistogram[8]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_24_Mb, > %d \n", pWmMibTMdata->RateHistogram[9]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_36_Mb, > %d \n", pWmMibTMdata->RateHistogram[10]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_48_Mb, > %d \n", pWmMibTMdata->RateHistogram[11]));  //[DE]
//	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "==>DW_TM_RATE_54_Mb, > %d \n", pWmMibTMdata->RateHistogram[12]));  //[DE]

	LOGD("CWmVwalWrapper::GetTMdata <----" );

    return result;
#else
    return WM_OK;
#endif //TM_SUPPORTED
}

	


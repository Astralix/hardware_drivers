///////////////////////////////////////////////////////////////////////////////
///
/// @file    	WmTypes.h
///
/// @brief		WiFi Manager types definitions.
///
///
/// @internal
///
/// @author  	sharons
///
/// @date    	21/07/2007
///
/// @version 	1.1
///
///
///				Copyright (C) 2001-2008 DSP GROUP, INC.   All Rights Reserved
///	

#ifndef WM_TYPES_H
#define	WM_TYPES_H


///////////////////////////////////////////////////////////////////////////////
// Includes
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "TypeDefs.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
#define WM_BIT(n) (1 << (n))


#define WIFI_DEVICE_NAME   		"wifi0"
#define MAX_DEVICE_NAME_LENGTH  256   //including '\0'

///////////////////////////////////////////////////////////////////////////////
// Defines
enum EWmConnectionState
{
	WM_STATE_IDLE		         			= 0,
	WM_STATE_SCANNING           			= 1,
	WM_STATE_STARTED           			= 2,
	WM_STATE_CONNECTED            			= 3,
	WM_STATE_CONNECTED_AND_SCANNING         = 4,
	WM_STATE_RESETTING         			= 5,
	WM_STATE_UNDEFINED            			= 6

};


enum EWmStatus
{
	WM_OK                    			= 0,
	WM_ILLEGAL_BSSID		 			= 1, 
	WM_ILLEGAL_SSID		 	 			= 2, 
	WM_ILLEGAL_AP_CONFIG	 			= 3,
	WM_NOT_INITIALIZED       			= 4, //WM was not initialized.
	WM_NOT_REGISTERED        			= 5, //No client was registered. 
	WM_ILLEGAL_COMMAND       			= 6, //Illegal command.
	WM_NOT_IMPLEMENTED		 			= 7, //Function is not implemented. 
	WM_CONNECT_TIMEOUT		 			= 8, //Connect timeout. 
	WM_SCAN_TIMEOUT		    			= 9, //Scan timeout. 
	WM_STOP_TIMEOUT						= 10,//Stop timeout. 
	WM_NOT_CONNECTED		 			= 11,//Driver is not in connected state.  
	WM_COMMAND_TIMEOUT		 			= 12,//Driver's timeout
	WM_SUPPLICANT_ERROR		 			= 13,//Driver's supplicant is not 
											 //connected or returned an error.
	WM_ILLEGAL_PS_MODE					= 14,//Illegal power save mode
	WM_ILLEGAL_COMMAND_WHILE_CONNECTED	= 15,//Illegal power save mode
	WM_BSSID_NOT_IN_SCAN_LIST			= 16,//Failed to find ssid in 
											 //scan list
	WM_ILLEGAL_PARAMETER				= 17,//Illegal parameter.
	WM_ERROR	             			= 18,//Driver Error.
	WM_NOT_FOUND						= 19,//
	WM_WPS_CONNECT_ERROR                = 20,//WPS connect error
    WM_COUNTRY_ERROR                    = 21 //unsupported Country code 
};


///////////////////////////////////////////////////////////////////////////////
/// @enum EWmEncryptAlgo
/// 
/// @brief Enumeration of avilable encryption algorithms.
/*RR TODO To Generate Compliation Error
enum EWmEncryptAlgo
{
   	WM_WEP         =   WM_BIT(0),  ///< WEP authentication algorithm.
  	WM_WPA         =   WM_BIT(1),  ///< WPA authentication algorithm.
  	WM_WPA2        =   WM_BIT(2),  ///< WPA2 authentication algorithm.
	WM_PRE_AUTH	   =   WM_BIT(3),  ///< AP support preauthentication 
								  
};
*/
enum EWmAuthType
{
	WM_AUTHTYPE_OPEN		= WM_BIT(0),
	WM_AUTHTYPE_WEP			= WM_BIT(1),
	WM_AUTHTYPE_WPA_PSK		= WM_BIT(2),
	WM_AUTHTYPE_WPA_EAP		= WM_BIT(3),
	WM_AUTHTYPE_WPA2_PSK	= WM_BIT(4),
	WM_AUTHTYPE_WPA2_EAP	= WM_BIT(5),
	WM_AUTHTYPE_IEEE8021X	= WM_BIT(6),
};

enum EWmCiphers
{
	WM_CIPHERS_PAIRWISE_NONE		= WM_BIT(0),
	WM_CIPHERS_PAIRWISE_TKIP		= WM_BIT(1),
	WM_CIPHERS_PAIRWISE_CCMP		= WM_BIT(2),
	WM_CIPHERS_GROUP_WEP40			= WM_BIT(3),
	WM_CIPHERS_GROUP_TKIP			= WM_BIT(4),
	WM_CIPHERS_GROUP_CCMP			= WM_BIT(5),
	WM_CIPHERS_GROUP_WEP104			= WM_BIT(6),

};

enum EWmApCapabilities
{
	WM_CAPABILITIES_PREAUTHENTICATION		= WM_BIT(0),
	WM_CAPABILITIES_WPS						= WM_BIT(2),
	WM_CAPABILITIES_WPS_PIN_ACTIVE			= WM_BIT(1),
	WM_CAPABILITIES_WPS_PBC_ACTIVE			= WM_BIT(3),
};
///////////////////////////////////////////////////////////////////////////////
/// @enum EWmBand
/// 
/// @brief Enumeration of available encryption algorithms.

enum EWmBand
{
  	WM_BAND_A = WM_BIT(0), ///<802.11a
  	WM_BAND_B = WM_BIT(1), ///<802.11b
  	WM_BAND_G = WM_BIT(2), ///<802.11g
};

///////////////////////////////////////////////////////////////////////////////
/// @enum EWmTriggerMechMode
/// 
/// @brief Enumeration of Power Save mechanism modes. Used by the traffic  
/// manager module.
///
enum EWmTriggerMechMode
{
	WM_TRIGGER_MECH_AUTO  = 0, ///< The traffic Manager is reponsible for 
							   ///< periodically sending trigger packets when  
						       ///< UAPSD is enabled according to the device 
							   ///< state.
	WM_TRIGGER_MECH_MANUAL   , ///< The traffic manager will send trigger 
						       ///< packets on behalf of the  user but the user 
						       ///< is reponsible for setting the frequency in 
						       ///< which packets will be sending.
	WM_TRIGGER_MECH_DISABLED,  ///< No trigger packets are sent by the traffic 
						       ///< manager.	
};


///////////////////////////////////////////////////////////////////////////////
/// @enum EWmPsOperationMode
/// 
/// @brief Enumeration of Power Save operation modes (automatic/enabled/
/// disabled). Used by the traffic manager module.
///
enum EWmPsOperationMode
{
	WM_PS_OPERATION_AUTO = 0, ///< Power save configuration is handled by the 
						      ///< Traffic Manager. 
						      ///< The traffic manager is responsible for 
						      ///< enabling / disabling the devise Power Save 
							  ///< according to its state.  
	WM_PS_OPERATION_ENABLED , ///< Power save is enabled, controled by the user.
	WM_PS_OPERATION_DISABLED, ///< Power Save is disabled, controled by the user.
};


///////////////////////////////////////////////////////////////////////////////
/// @enum EWmPsMode
/// 
/// @brief Enumeration of Power Save modes.
///
enum EWmPsMode
{
    WM_PS_MODE_UAPSD = 0, ///< UAPSD power save	
	WM_PS_MODE_LEGACY   , ///< Legacy power save
	WM_PS_MODE_NONE     , ///< no power save
	WM_PS_MODE_AUTO     , ///< power save auto
};

///////////////////////////////////////////////////////////////////////////////
/// @enum EWmRoamingMode
/// @brief Enumration of roaming modes (enables/disbaled).
/// 
enum EWmRoamingMode
{
    WM_ROAMING_MODE_DISABLED = 0,
	WM_ROAMING_MODE_ENABLED  = 1
};

///////////////////////////////////////////////////////////////////////////////
/// @enum EWmUapsdMaxSpLen
/// 
/// @brief U-APSD max service period length (in packets)
enum EWmUapsdMaxSpLen
{
	WM_UAPSD_MAXSPLEN_ALL = 0,
	WM_UAPSD_MAXSPLEN_2,
	WM_UAPSD_MAXSPLEN_4,
	WM_UAPSD_MAXSPLEN_6,
};

///////////////////////////////////////////////////////////////////////////////
/// @enum EWmPowerSaveAction
/// 
/// @brief Enumeration of WiFi Driver Power Save action.
/// 	   Each value represents an action the driver might take when it has an 
/// 	   opportunity to  sleep. The Traffic Manager uses this value to configure
/// 	   PS action. 
enum EWmPowerSaveAction
{
	WM_PS_ACTION_MAX_PS = 0,      ///< Maximum power save.
	WM_PS_ACTION_FULL_PERFORMANCE,///< No power Save, the driver is always on.
	WM_PS_ACTION_AUTOMATIC		  ///< Automatic Power Save. The Traffic Manager 
								  ///< is responsible for power save settings and
								  ///< change the PS action mode according to the 
								  ///< device state.
};


///////////////////////////////////////////////////////////////////////////////
/// @enum EWmTriggerPacketsMode
/// 
/// @brief Enumeration of trigger packets modes.
///
enum EWmTriggerPacketsMode
{
    
	WM_TRIGGER_PACKETS_MODE_AUTO = 0, ///< Automatic trigger packets mode. 
									  ///< The Traffic Manager is responsible for 
									  ///< sending uapsd trigger data frames on 
									  ///< behalf of the user. 
	WM_TRIGGER_PACKETS_MODE_MANUAL	  ///< Manual trigger packets mode. 
									  ///< The user is responsible for sending uapsd 
									  ///< trigger data frames.							
								  
};



///////////////////////////////////////////////////////////////////////////////
/// @enum EWmCountry
/// 
/// @brief Enumeration of supported countries for Mac Operation
/// IMPORTANT - if you add elements to this list please do it in alphabetic 
/// 			order.

#define WM_MAX_COUNTRY_NUM  1000 
enum EWmCountry
{
    WM_COUNTRY_ARGENTINA      = 0, 
    WM_COUNTRY_AUSTRALIA	     , 
    WM_COUNTRY_CHINA		     ,  	
    WM_COUNTRY_FRANCE			 , 
    WM_COUNTRY_GERMANY			 ,
    WM_COUNTRY_HONG_KONG		 ,
    WM_COUNTRY_INDIA			 ,
    WM_COUNTRY_ISRAEL			 ,
    WM_COUNTRY_ITALY			 ,
    WM_COUNTRY_JAPAN			 ,
    WM_COUNTRY_TAIWAN			 ,
    WM_COUNTRY_UNITED_KINGDOM	 ,
    WM_COUNTRY_UNITED_STATES	 ,
	WM_COUNTRY_NOT_SUPPORTED  = WM_MAX_COUNTRY_NUM 
};

///////////////////////////////////////////////////////////////////////////////
/// @enum EWmScanChannelsMasks
/// 
/// @brief Enumeration of supported channels for scanning. Should be used for
/// 	   setting scan channel list.	
///
enum EWmScanChannelsMask{

    WM_CHANNEL_1_BIT_MASK	= WM_BIT(0),
    WM_CHANNEL_2_BIT_MASK	= WM_BIT(1),
    WM_CHANNEL_3_BIT_MASK	= WM_BIT(2),
    WM_CHANNEL_4_BIT_MASK	= WM_BIT(3),
    WM_CHANNEL_5_BIT_MASK	= WM_BIT(4),
    WM_CHANNEL_6_BIT_MASK	= WM_BIT(5),
    WM_CHANNEL_7_BIT_MASK	= WM_BIT(6),
    WM_CHANNEL_8_BIT_MASK	= WM_BIT(7),
    WM_CHANNEL_9_BIT_MASK	= WM_BIT(8),
    WM_CHANNEL_10_BIT_MASK	= WM_BIT(9),
    WM_CHANNEL_11_BIT_MASK	= WM_BIT(10),
    WM_CHANNEL_12_BIT_MASK	= WM_BIT(11),
    WM_CHANNEL_13_BIT_MASK	= WM_BIT(12),
    WM_CHANNEL_14_BIT_MASK	= WM_BIT(13),
};


///////////////////////////////////////////////////////////////////////////////
/// @struct Ssid
/// 
/// @brief  Structure that holds network SSID.
/// 
/// 		Stores network SSID.
///         For example if network's SSID is "ssid"
///         Sets Ssid.length  =  4;
/// 			 Ssid.Ssid[0] = 's';
/// 			 Ssid.Ssid[1] = 's';
/// 			 Ssid.Ssid[2] = 'i';
/// 			 Ssid.Ssid[3] = 'd';
/// 

#define    WM_SSID_LEN  32
struct WmSsid
{
  	uint8 Length;             ///< Number of valid characters in Ssid[].
  	uint8 Ssid[WM_SSID_LEN];  ///< SSID of the network.
	uint8 Reserved[3]; 		  ///< Reserved
	mutable char StrSsid[WM_SSID_LEN+1]; ///< String represntation of the
											    ///< Mac Address

	// conveniency ctor
	WmSsid( const char* pSsid = NULL )
	{
		memset(Ssid, 0, WM_SSID_LEN); 
		memset(Reserved, 0, 3); 

		if( pSsid )
		{
			Length = strlen( pSsid );
			if( Length > WM_SSID_LEN )
			{
				Length = WM_SSID_LEN;
			}

			memcpy(Ssid, pSsid, WM_SSID_LEN);
		}
		else
		{
			Length = 0;
		}
	}

    //operator =
	WmSsid& operator=(const WmSsid& ssid)
	{
		if (this == &ssid)
		{
			return *this;
		}

		memset(Ssid, 0, WM_SSID_LEN); 
		memset(Reserved, 0, 3); 
		Length = ssid.Length;
		memcpy(Ssid, ssid.Ssid, ssid.Length);
		return *this;
	}

	//operator ==
	bool operator==(const WmSsid& OtherSsid)
	{
		if(OtherSsid.Length != Length)
		{
			return false;
		}
		for( int i=0; i<Length; ++i )
		{
			if ( Ssid[i] !=  OtherSsid.Ssid[i])
			{
				return false;
			}
		}
		memset(Reserved, 0, 3); 
		return true;
	}

	bool operator!=(const WmSsid& OtherSsid)
	{
		return !(*this == OtherSsid);
	}

//RR TODO check more on this this one generates warnings
//	const char* const WmSsid2Str() const
	const char* WmSsid2Str() const
	{
		memset(StrSsid, 0, WM_SSID_LEN+1);
		snprintf(StrSsid,WM_SSID_LEN,"%s",Ssid);
		return StrSsid;
	}
};

///////////////////////////////////////////////////////////////////////////////
/// @struct WM_MacAddress
/// 
/// @brief  Structure that holds Mac Address, i.e., network's BSSID.
/// 		
/// 

#define    WM_MAC_ADDR_LEN  6 // If changes change MacAdress reserved size.
#define    WM_MAC_ADDR_STR_LEN 18 // Mac Address format aa:bb:cc:dd:ee:ff

struct WmMacAddress
{
  	uint8 Address[WM_MAC_ADDR_LEN]; 	///< Network's MAC address. 
	uint8 Reserved[8];		        	///< Reserved for alignment.
	mutable char StrBssid[WM_MAC_ADDR_STR_LEN]; ///< String represntation of the
											    ///< Mac Address


	// conveniency ctor
	WmMacAddress( const char* pBssid = NULL )
	{
		memset(Address, 0, WM_MAC_ADDR_LEN); 
		memset(Reserved, 0, 8); 

		if( pBssid )
		{
			const char cSep = ':';
			bool IsLegalFormat = true;
			for (int iConunter = 0; iConunter < 6; ++iConunter)
			{
				unsigned int iNumber = 0;
				char ch;

				//Convert letter into lower case.
				ch = tolower (*pBssid++);
		
				if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
				{
					IsLegalFormat = false;
					break;
				}
		
				//Convert into number. 
				//  a. If character is digit then ch - '0'
				//	b. else (ch - 'a' + 10) it is done 
				//	because addition of 10 takes correct value.
				iNumber = isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
				ch = tolower (*pBssid);
		
				if ((iConunter < 5 && ch != cSep) || 
					(iConunter == 5 && ch != '\0' && !isspace (ch)))
				{
					++pBssid;
		
					if ((ch < '0' || ch > '9') && (ch < 'a' || ch > 'f'))
					{
						IsLegalFormat = false;
						break;
					}
		
					iNumber <<= 4;
					iNumber += isdigit (ch) ? (ch - '0') : (ch - 'a' + 10);
					ch = *pBssid;
		
					if (iConunter < 5 && ch != cSep)
					{
						IsLegalFormat = false;
						break;
					}
				}
				// Store result.
				Address[iConunter] = (unsigned char) iNumber;
				++pBssid;
			}

			if(!IsLegalFormat) 
			{
					memset(Address, 0, WM_MAC_ADDR_LEN); 
					memset(Reserved, 0, 8); 
			}
        }
	}


	//opertaor=
	WmMacAddress& operator=(const WmMacAddress& bssid)
	{
		if (this == &bssid)
		{
			return *this;
		}
		for (uint8 loop=0; loop<WM_MAC_ADDR_LEN; ++loop)
		{
			Address[loop] = bssid.Address[loop];
		}
        return *this;
	}
	//opertaor==
	bool operator==(const WmMacAddress& OtherBssid)
	{
		for( int i=0; i<WM_MAC_ADDR_LEN; ++i )
		{
			if ( Address[i] !=  OtherBssid.Address[i])
			{
				return false;
			}
		}
		return true;
	}
    bool operator!=(const WmMacAddress& OtherBssid)
	{
		return !(*this == OtherBssid);
	}

	///////////////////////////////////////////////////////////////////////////
	// WmMacAddr2Str return a string representation of the form 
	// 		         aa:bb:cc:dd:ee:ff.	
	// @return Returns a string representation of the mac address of the form 
	// 		   aa:bb:cc:dd:ee:ff.	
	// 		   
	// 
//RR TODO check more on this this one generates warnings
// const char* const WmMacAddr2Str() const
	const char* WmMacAddr2Str() const
	{
		sprintf(StrBssid,"%02X:%02X:%02X:%02X:%02X:%02X", 
					Address[0],
					Address[1],
					Address[2],
					Address[3],
					Address[4],
					Address[5] );

		return StrBssid;
	}
};

///////////////////////////////////////////////////////////////////////////////
/// @struct ScanListInfoStruct
/// 
/// @brief  Structure that holds AP's info.
/// 

struct  WmScanListInfo
{
  	
  	WmSsid         	Ssid;             	    	//AP's SSID.
  	WmMacAddress   	Bssid;  	       	        //AP's BSSID, i.e., its MAC 
												//address
  	uint32     		Rssi;       	            //AP's RSSI (Received Signal 
												//Strength Indicator)
  	uint8       	Channel; 	                //AP's channel
	EWmBand       	Band;	                    //AP's band, as defined by 
												//EBand. 
  	uint8 			AuthType;					//Authorization Type
	uint8			SupportedCiphers;			//Supported pairwise 
												// and group ciphers
	uint8			Capabilities;				//AP capabilities (WPS,PREAUTH)
	EWmPsMode       PsMode;						//Power Save mode.
	uint8  		    PrevConfigured;				//SSID/BSSID connection 
												//parameters already in DB.
    EWmCountry      Country;                    //Country String 
												

	///	@brief  Assignment operator
	WmScanListInfo& operator=(const WmScanListInfo& OtherInfo)
	{
		if (this != &OtherInfo)
		{
			Ssid   		    = OtherInfo.Ssid           ;
			Bssid  			= OtherInfo.Bssid          ;
			Rssi   			= OtherInfo.Rssi           ;
			Channel			= OtherInfo.Channel        ;
			Band   			= OtherInfo.Band           ;
			AuthType		= OtherInfo.AuthType	   ;
			SupportedCiphers = OtherInfo.SupportedCiphers;
			Capabilities	=	OtherInfo.Capabilities;
			PsMode          = OtherInfo.PsMode         ;
			PrevConfigured  = OtherInfo.PrevConfigured ;
            Country         = OtherInfo.Country        ;
		}
		return *this;
	}
												  					
};


///////////////////////////////////////////////////////////////////////////////
/// @struct WmStatistics
/// 
/// @brief  Structure that holds statistics retrieved periodically by the QWM
/// when the driver is in connected state.
/// 
struct WmStatistics
{
  	uint32 FailedCount;				//Number of failed to transmit packets.  
	uint32 RetryCount;				//Number of packets tx successfully 
									//after MORE than one trial.
	uint32 FcsErrorCount;           //FCS error count.
	uint32 TransmittedFrameCount;   //Number of successfully transmitted MSDU.
	uint32 AverageRssi;				//Running avg. RSSI of the beacon of the
									//AP we are associate with.
};




///////////////////////////////////////////////////////////////////////////////
/// FOR DEBUG MANAGER ONLY ////////////////////////////////////////////////////
/// Follows IEEE 802.11 name coneventios. /////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// @enum EWmPsAction
/// 
/// @brief Enumeration of WiFi Driver Power Save action.
/// 	   Each value represents an action the driver might take when it has an 
/// 	   opportunity to  sleep (i.e. there is no downlink traffic buffered 
/// 	   at the AP).
enum EWmPsAction
{
	WM_POWERSAVEACTION_ALWAYSON	= 0, ///<P0 
	WM_POWERSAVEACTION_DOZE,		 ///<P1
	WM_POWERSAVEACTION_DEEPSLEEP	 ///<P2
								
};



///////////////////////////////////////////////////////////////////////////////
/// @enum EWmAc
/// 
/// @brief Access catagories. 
/// 	   WM_AC0 -	Best Effort
/// 	   WM_AC1 - Background
/// 	   WM_AC2 - Video
/// 	   WM_AC3 -	Voice
enum EWmAc
{
	WM_AC0 = 0,
	WM_AC1,
	WM_AC2,
	WM_AC3,
};

#define MAX_TX_BATCH_PKT_SIZE 1560

struct WmTxBatchPacket
{
	uint32 numTransmits;				// Number of times this frame will be transmited

	uint8  phyRate;						// 1, 2, 55, 6, 9, 11, 12, 18, 24, 36, 48, 54.

	uint16 packetSize;					// Length of the 802.11 frame (header + body)

	uint8 txPowerLevel;					// This value will be programmed directly into
										//  the PHY header's TXPWRLVL field.

	uint8 pkt[MAX_TX_BATCH_PKT_SIZE]; 	// 802.11 Frame (MAC Header + Frame Body)
};


#define MAX_RX_PKT_SIZE	4096 

#define WM_RXFLAGS_FCS_ERROR         0x00000010 
#define WM_RXFLAGS_LENGTH_EOF_ERROR  0x00000020 
#define WM_RXFLAGS_PROTOCOL_ERROR    0x00000040 
#define WM_RXFLAGS_DECRYPT_ERROR     0x00000100
#define WM_RXFLAGS_LONG_PREAMBLE	 0x00000200

struct WmRxWifiPacket
{
    uint16	length;			// The length of the packet (which does not include FCS),
							//  specified in  bytes.

	uint8	rssiChannelA;  
	uint8	rssiChannelB;

    uint32	tsfTimestamp;	// This field contains the lower 32 bits
							//  of the TSF timer at the time when the packet
							//  was received. This is a microsecond timer. 

	uint32	flags;

	uint8	phyDataRate;	// Valid values to be used for this field for 802.11a are:
							//  6, 9, 12, 18, 24, 36, 48, 54.  
							//
							// Valid values to be used for this field for 802.11g are:
							//  1, 2, 55, 6, 9,  11, 12, 18, 24, 36, 48, 54.  
							//
							// Valid values to be used for this field for 802.11b are:
							//  1, 2, 55, 11

    

    uint8	channel;		// channel number on which this packet was received.  
							// Valid channel numbers are:
							//  1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 34, 36,
							// 38, 40, 42, 44, 46, 48, 52 56, 60, 64.


	uint8	reserved[2];

	uint8	packet[MAX_RX_PKT_SIZE];

}; 

// TX Power Level Control - correspond to constants in WifiApi.h
// IMPORTANT: if these are changed in WifiApi.h we must update them here too
#define WM_POWERTABLE_MAX_FREQ_NUM         48
#define WM_POWERTABLE_MAX_RATE_NUM         23

//
//      Power Level Control - Frequency and Rate Index Table
//
//      unit8 pt[POWERTABLE_MAX_FREQ_NUM][POWERTABLE_MAX_RATE_NUM]
// 
//      Frequency Index Table:
//      Index    Freq            Index    Freq            Index    Freq   
//       0       2.412G           16      5.080G           32      5.560G 
//       1       2.417G           17      5.170G           33      5.580G 
//       2       2.422G           18      5.180G           34      5.600G 
//       3       2.427G           19      5.190G           35      5.620G 
//       4       2.432G           20      5.200G           36      5.640G 
//       5       2.437G           21      5.210G           37      5.660G 
//       6       2.442G           22      5.220G           38      5.680G 
//       7       2.447G           23      5.230G           39      5.700G 
//       8       2.452G           24      5.240G           40      5.745G 
//       9       2.457G           25      5.260G           41      5.765G 
//       10      2.462G           26      5.280G           42      5.785G 
//       11      2.467G           27      5.300G           43      5.805G 
//       12      2.472G           28      5.320G           44      4.920G 
//       13      2.484G           29      5.500G           45      4.940G 
//       14      5.040G           30      5.520G           46      4.960G 
//       15      5.060G           31      5.540G           47      4.980G 
//     
//      Rate Index Table:
//      Index
//       0      1 Mb With Long Preamble           
//       1      2 Mb With Long Preamble          
//       2      2 Mb With Short Preamble         
//       3      5.5 Mb With Long Preamble         
//       4      5.5 Mb With Short Preamble       
//       5      11 Mb With Long Preamble         
//       6      11 Mb With Short Preamble        
//       7      6 Mb                
//       8      9 Mb                
//       9      12 Mb               
//       10     18 Mb               
//       11     24 Mb               
//       12     36 Mb               
//       13     48 Mb               
//       14     54 Mb          
//
//       15	MSC0
//       16	MSC1
//       17	MSC2
//       18	MSC3
//       19	MSC4
//       20	MSC5
//       21	MSC6
//       22	MSC7

struct WmPowerLevelTable
{
    uint8 pt[WM_POWERTABLE_MAX_FREQ_NUM][WM_POWERTABLE_MAX_RATE_NUM];
}; 


enum EWmMibPhyParameterType
{
	WM_PHYPARAM_CHIPSET_ALIAS    = 1,
	WM_PHYPARAM_DEFAULTREGISTERS = 2,
	WM_PHYPARAM_REGISTERSBYBAND  = 3,
	WM_PHYPARAM_REGISTERSBYCHANL = 4,
	WM_PHYPARAM_REGISTERSBYCHANL24 = 5
};


struct WmMibPhyParameter
{
	uint32	paramType;		/// one of the WIFI_PHYPARAM_* values
	uint32	dataSize;		/// data buffer size, bytes
	uint8	*data;			/// pointer to data buffer
};

#define WIFI_SCANCHANNELLIST_MAXCHANNELS	128
struct WmScanChanFreqList
{
	uint32	nChanFreq;
	uint16	chanFreq[WIFI_SCANCHANNELLIST_MAXCHANNELS];
};

enum EWmWPSConnectType
{
	WM_PBC		= 1,
	WM_PIN_CODE = 2
};

#define PIN_CODE_LENGTH 8

#define WM_DEF_SCAN_DWELL_TIME         20
#define WM_MIN_SCAN_DWELL_TIME         20
#define WM_MAX_SCAN_DWELL_TIME         500

#define WM_DEF_ROAMING_TRIGGER_RSSI    25

#define WM_DEF_SCAN_DELTA_SNR          20

#define WM_DEF_BKG_SCAN_INTERVAL       5000


#define WM_DEF_SCAN_HOME_TIME          20
#define WM_MIN_SCAN_HOME_TIME          10
#define WM_MAX_SCAN_HOME_TIME          500

#define WM_DEF_SCAN_NUM_PROBES         5
#define WM_MIN_SCAN_NUM_PROBES         1
#define WM_MAX_SCAN_NUM_PROBES         6

#define WM_DEFAULT_MIN_POWERMGMT_LEVEL         0
#define WM_DEFAULT_MAX_POWERMGMT_LEVEL         100
#define WM_MAX_POWERMGMT_LEVEL         		   1000 //maximum that supported

typedef char WmWPSPinCode[PIN_CODE_LENGTH+1];


//TM Definitions (From WifiAoi.h)

struct WmMibTMdata //was dwMibTM_Cli_t // Note the different name
{
	uint8	type;
	uint32	RxAccRssiA;
	uint32 	RxAccRssiB;
	uint32	TxNAtmp;
	uint32 	TxFail;
	uint32	NumberOfPAckets;
	uint32	RxFCSErr;
	uint32	TxAccOK;
	uint16	RateHistogram[12];
};

 // end TM definitions


#endif	//WM_TYPES_H


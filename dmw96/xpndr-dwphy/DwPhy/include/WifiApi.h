// -*- mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: t; -*-
// vim: set tabstop=4 shiftwidth=4 softtabstop=4 noexpandtab:
////////////////////////////////////////////////////////////////////////////////
///
///	@file		WifiApi.h	
///	@brief		Contains the definitions and structures needed to communicate
///				with the Wifi module.
///
///	@version	$Revision$
///	@internal
///	@author		$Author$
///	@date		$Date$
///	@attention
///
///			Copyright (C) 2001-2008 DSP GROUP, INC.   All Rights Reserved
////////////////////////////////////////////////////////////////////////////////

#ifndef _WIFI_API_H
#define _WIFI_API_H

/// platform specific typedefs.
typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef signed int			int32;

#ifndef WIN32
typedef unsigned long long	uint64;
#endif

#ifndef TRUE 
    #define TRUE 1
    #define FALSE 0
#endif

#if defined(__cplusplus)
#define WIFIAPI extern "C"
#else 
#define WIFIAPI
#endif

#define WIFIAPI_CFG_MAXSCANCACHESIZE 64

///
/// The QoS API provides access to the WiFi QoS services and allows
/// applications to setup and teardown QoS streams and to obtain status and
/// statistics of QoS streams. 
///

/// Valid command types for the WifiQosAPI() call and callbacks
enum {
	WIFI_QOS_API_COMMAND_CREATE = 0,
	WIFI_QOS_API_COMMAND_DELETE,
	WIFI_QOS_API_COMMAND_QUERY,
	WIFI_QOS_API_COMMAND_ENUMERATE_STA_STREAMS,

	WIFI_QOS_API_COMMAND_RESERVED = 0x80
};

typedef enum 
{
	WIFI_SUCCESS = 0, 
	WIFI_FAIL, 
	WIFI_DEVICE_NOT_FOUND, 
	WIFI_INVALID_PARAMS,
	
	/// QAP did not respond to the request within the specified timeout
	WIFI_QOS_TIMEOUT,

	/// Stream does not exist either because the caller deleted the stream, it
	/// was deleted by the QAP or the handle is invalid   
	WIFI_QOS_STREAM_DOES_NOT_EXIST,

	/// QSTA unable to support the request due to resource issues 
	WIFI_QOS_OUT_OF_RESOURCES,

	/// The request to create the stream is in progress (but the stream does
	/// not yet exist).  
	WIFI_QOS_STREAM_CREATE_PENDING,

	/// Attempted to create a stream with a signature identical to an existing
	/// stream
	WIFI_QOS_STREAM_ALREADY_EXISTS,

	/// Attempted to create a stream when the maximum number of streams
	/// supported by the QSTA are already in existence 
	WIFI_QOS_EXCEEDED_MAX_NUM_STREAMS,

	/// Unable to create a stream because scanning is in progress   
	WIFI_QOS_SCAN_IN_PROGESS,

	/// Unable to create a stream because QSTA is not associated with a QAP
	WIFI_QOS_STA_NOT_ASSOCIATED,

	/// The QAP refused this request because it could not provide the
	//requested QoS, admission control didn't permit the request, etc.   
	WIFI_QOS_AP_REFUSED_STREAM_CREATION,

	/// Either the caller does not have WMM-SA enabled or the QAP does not
	/// support WMM-SA 
	WIFI_QOS_NOT_ENABLED,
	
	/// A traffic stream operation is already pending and must
	/// complete before another operation may be started.
	WIFI_QOS_TS_PENDING,

	//
	// MIB
	//
	
	WIFI_MIB_WRITE_NOT_ALLOWED, 
	WIFI_MIB_READ_NOT_ALLOWED,
	WIFI_MIB_UNKNOWN_MIBOID, 
	WIFI_MIB_ADOPT_WRITE_WHILE_NOT_STOPPED,
	WIFI_MIB_UNKNOWN_OPERATION, 
	WIFI_MIB_OUT_OF_RANGE,

	//
	// 802.11 Tx/Rx
	//
	
	WIFI_80211_OUT_OF_TX_BUFFERS,
	WIFI_80211_DEVICE_NOT_READY,

	WIFI_RESERVED		
}
dwWifiStatus_t;

typedef struct {
#define WIFI_ADDTS_TIMEOUT_MIN 500
#define WIFI_ADDTS_TIMEOUT_MAX 1000
	// Time in milliseconds before the ADDTS.Request will timeout if
	// no response is received from the AP.
	uint32 timeout;

	uint8 dialogToken;
	
	// TSINFO (add all TSINFO fields)
#define WIFI_ADDTS_TRAFFICTYPE_UNSPECIFIED 0
#define WIFI_ADDTS_TRAFFICTYPE_PERIODIC 1
	uint8 trafficType;
	uint8 tsid;

#define	WIFI_ADDTS_DIR_UPLINK 0
#define WIFI_ADDTS_DIR_DOWNLINK 1
#define WIFI_ADDTS_DIR_DIRECTLINK 2
#define WIFI_ADDTS_DIR_BIDIRECTIONALLINK 3
	uint8 direction;

#define WIFI_ADDTS_ACCESSPOLICY_RESERVED 0
#define WIFI_ADDTS_ACCESSPOLICY_EDCA 1
#define WIFI_ADDTS_ACCESSPOLICY_HCCA 2
#define WIFI_ADDTS_ACCESSPOLICY_HEMM 3
	uint8 accessPolicy;

	uint8 aggregation;
	uint8 powersave;
	uint8 priority;

#define WIFI_ADDTS_ACKPOLICY_NORMAL_ACK 0
#define WIFI_ADDTS_ACKPOLICY_NO_ACK 1
#define WIFI_ADDTS_ACKPOLICY_NO_EXPLICIT_ACK 2
#define WIFI_ADDTS_ACKPOLICY_BLOCK_ACK 3
	uint8 ackPolicy;

	uint8 schedule;
	uint8 reserved[2];

#define WIFI_ADDTS_FIXED_MSDU_SIZE 0x8000
	uint16 nominalMsduSize;
	uint16 maxMsduSize;
	uint32 minServiceInterval;
	uint32 maxServiceInterval;
	uint32 inactivityInterval;
	uint32 suspensionInterval;
	uint32 serviceStartTime;
	uint32 minDataRate;
	uint32 meanDataRate;
	uint32 peakDataRate;
	uint32 maxBurstSize;
	uint32 delayBound;
	uint32 minPhyRate;
	uint16 sba_integer;
	uint16 sba_fraction;

// HCCA only - value of zero for address/port pair indicates unused
#define WIFI_ADDTS_MAX_TCLAS 5
	struct dw_wifi_tclas
	{
		uint16 ipv4_protoType;
		uint16 ipv4_destIpPort;
		uint32 ipv4_destIpAddress;
	} tclas[WIFI_ADDTS_MAX_TCLAS];
	int numValidTclas;
} WIFI_ADDTS;

typedef struct {
	uint8 addr[6];
	uint8 tsid;
	uint8 direction;
	uint8 accessPolicy;
} WIFI_DELTS;

typedef struct {
	uint8 addr[6];
	uint8 tsid;
	uint8 direction;
	uint8 accessPolicy;
	
	uint32 lastTx;
	uint32 lastRx;
	uint32 nTxOkFrames;
	uint32 nTxFailFrames;
	uint32 nTxBytes;
	uint32 nTxRetries;
	
	uint32 nRxFrames;
	uint32 nRxBytes;
} WIFI_QUERYTS;

typedef struct {
	uint8 addr[6];

#define WIFI_MAX_STREAMS_PER_STA (8/*EDCA*/ + 16/*HCCA*/)
	struct {
		uint8 tsid;
		uint8 direction;
		uint8 accessPolicy;
	} ts[WIFI_MAX_STREAMS_PER_STA];
} WIFI_TSLIST;

/// Structure passed to the WifiQosAPI() call that defines what the user wants
/// the WifiQosAPI() call to do.
typedef struct {
	uint32 command;           
	uint32 status;
	union {
		WIFI_ADDTS add;
		WIFI_DELTS del;
		WIFI_QUERYTS query;
		WIFI_TSLIST list;
	} u;
} WIFI_STREAM;


/// \brief  The WifiQosAPI function call provides the interface to all stream
/// creation, deletion and management capabilities.  
///
/// \param[in] deviceSelector  selects the device to communicate with.  
/// \param[in,out] stream  Structure to pass stream  parameter info in and out
/// for a given stream 
///
/// \return WIFI_QOS_API_STREAM_*
WIFIAPI uint32 WifiQosAPI(void* deviceSelector, WIFI_STREAM* stream);


///
/// The MIB API provides applications with the ability to read and write MIB
/// objects, both standard WiFi MIB objects and non-standard MIB objects which
/// extend the capabilities of the MIB.  The MIB objects are made up of four
/// different types:  R/W, RO, WO, and Adopt. 
///
/// R/W type objects can be read anytime and written anytime-the write
/// instantly affects the system.  RO type objects can be read anytime.  WO
/// objects can be written anytime-the write instantly affects the system.
/// Adopt objects can be read anytime but can only be written when the adaptor
/// is in a stopped state.  The write only takes effect on the system after a
/// MAC SW or HW reset has occurred.  To set Adopt MIB objects the user issues
/// a WIFI_MIB_API_COMMAND_STOP command, then sets the appropriate Adopt MIB
/// objects and then issues a WIFI_MIB_API_COMMAND_START command. 
///


/// 48 bit address: most significant octet @ location address[0]
#define MAC_ADDR_LEN    6
typedef struct macaddrstruct
{
    uint8 address[MAC_ADDR_LEN];
} 
MAC_ADDRESS;


/// 802.11 supported bit rates 
#define MAX_RATES       12
typedef struct supportedRatesStruct
{
    uint32 rate[MAX_RATES];
} 
SUPPORTED_RATES;


#define COUNTRY_LEN     3
typedef struct CountryStringStruct
{
    char CountryString[COUNTRY_LEN];
    uint8 pad[1];
} 
COUNTRY_STRING;


/// Encryption type for key storage
enum
{
	KEY_TYPE_INVALID	= 0,
	KEY_TYPE_WEP,
	KEY_TYPE_TKIP,
	KEY_TYPE_RSVD,
	KEY_TYPE_AES,
	KEY_TYPE_WEP_104
};


#define WIFI_API_MAX_KEY_LEN 32
typedef struct encryptionKeyStruct
{
	// fits a 256-bit key
    uint8 key[ WIFI_API_MAX_KEY_LEN ];  

	// length of the key in bytes (5 , 13, 16 or 32.  
    uint16  size;                 

	// INVALID, WEP, TKIP, AES, PMK 
    uint16  type;                 
}
ENCRYPTION_KEY;


#define DSPG_802_11_LENGTH_SSID         32
/// Structure to store the network SSID
typedef struct ssidStruct
{
	/// Number of valid characters in ssid[]
    uint8 length;

	/// SSID of the network
	uint8 ssid[DSPG_802_11_LENGTH_SSID];

	/// Byte padding
    uint8 reserved[3];
} SSID;


typedef struct dwMibTxBatch_s
{
	/// Number of times this frame will be transmited
	uint32	numTransmits;

	// OR phyRate with this flag to use long preambles for 2Mbs, 5.5Mbs and
	// 11Mbs.
#define WIFI_BATCHTX_RATE_USE_LONG_PREAMBLE 0x80
	// WIFI_BATCHTX_RATE_MCS_IDX is mutually exclusive with
	// WIFI_BATCHTX_RATE_USE_LONG_PREAMBLE.
#define WIFI_BATCHTX_RATE_MCS_IDX 0x40

	/// 1, 2, 55, 6, 9, 11, 12, 18, 24, 36, 48, 54.
	/// if WIFI_BATCHTX_RATE_MCS_IDX is valid values are 0,1,2,3,4,5,6,7
	uint8	phyRate;

	// This value will be programmed directly into the PHY header's TXPWRLVL
	// field.
	uint8	powerLevel;

	/// Length of the 802.11 frame (header + body)
	uint16	pktLen;

	// frag2size - phy/cmd - CRC
#define MIBOID_TXBATCHMODE_MAXPKTSIZE	(1580 - 16 - 4)
	/// 802.11 Frame (MAC Header + Frame Body)
	uint8	pkt[MIBOID_TXBATCHMODE_MAXPKTSIZE];
} 
dwMibTxBatch_t;

typedef struct dwScanChanFreqList_s
{
#define WIFI_SCANCHANNELLIST_MAXCHANNELS	128
	uint32	nChanFreq;
	uint16	chanFreq[WIFI_SCANCHANNELLIST_MAXCHANNELS];
}
dwScanChanFreqList_t;

#define WIFI_MAX_RATE 24 // maximum supported rates
#define WIFI_MAX_VAR_IE_LEN	1024 // bytes for all variable IEs

#define VERSION_LEN 128
typedef struct driverVersionStruct
{
    // Array containing the driver version information
    char driverVersionString[VERSION_LEN];
} 
DRIVER_VERSION;

enum
{
	WIFI_MIB_API_OPERATION_READ		= 1,
	WIFI_MIB_API_OPERATION_WRITE	= 2,
	WIFI_MIB_API_OPERATION_RESERVED	= 0x80
};

enum
{
	MIBOID_TYPE_UINT32,				// 4
	MIBOID_TYPE_UINT64,				// 8
	MIBOID_TYPE_SSID,				// sizeof(SSID)
	MIBOID_TYPE_MACADDR,			// sizeof(MAC_ADDRESS)
	MIBOID_TYPE_SUPPORTEDRATES,		// sizeof(SUPPORTED_RATES)
	MIBOID_TYPE_COUNTRYSTRING,		// sizeof(COUNTRY_STRING)
	MIBOID_TYPE_ENCRYPTIONKEY,		// sizeof(ENCRYPTION_KEY)
	MIBOID_TYPE_TXBATCH,			// sizeof(dwMibTxBatch_t)
	MIBOID_TYPE_IE,					// WIFI_MAX_VAR_IE_LEN 
	MIBOID_TYPE_SCANCHANFREQLIST,	// sizeof(dwScanChanFreqList_t)
    MIBOID_TYPE_POWERTABLE,         // 
	MIBOID_TYPE_PHYPARAMETER,		// sizeof( dwMibPhyParameter_t )
	MIBOID_TYPE_HWMODES,			// sizeof( dwMibHwModes_t )
    MIBOID_TYPE_DRIVER_VERSION,     // sizeof(DRIVER_VERSION)
	
	// Always keep last!
	MIBOID_TYPE_INVALID
};

		
#define MIBOID_ATTRIB_NONE			0
#define MIBOID_ATTRIB_READ			0x0001	
#define MIBOID_ATTRIB_WRITE			0x0002
#define MIBOID_ATTRIB_ADOPT			0x0004
#define MIBOID_ATTRIB_MINMAX		0x0008
#define MIBOID_ATTRIB_INDEX			0x0010
#define MIBOID_ATTRIB_CLEARONREAD	0x0020

// TODO jyoung Update STA/AP only miboids with these attributes 
#define MIBOID_ATTRIB_STAONLY		0x0040	
#define MIBOID_ATTRIB_APONLY		0x0080


typedef struct mibObjectContainer_t
{
	/// ID of the MIB Object 
	uint16 mibObjectID;

	/// Operation to be carried out on the MIB Object specified by #mibObjectID
	uint8	operation;

	/// Status of the #operation carried out on the MIB Object specified by
	/// #mibObjectID
	uint8	status;

	/// Pointer to the MIB Object data 
	void*	mibObject;
} 
mibObjectContainer_t;


/// \brief  Provides the ability to read and write multiple MIB Objects.
///
/// \param[in] deviceSelector selects the device to communicate with.
/// \param[in,out] miboids array of MIB object containers.
/// \param[in] numMiboids number of MIB Object containers in the miboids array.
WIFIAPI uint32 WifiMibAPI(	void* deviceSelector, 
							mibObjectContainer_t *miboids, 
							uint32 numMiboids);


// ============================================================================

/// @brief Power Management Mode
/// Configure the 802.11/WMM power management mode.  
#define MIBOID_POWERMGMT_MODE							0X0001
#define MIBOID_POWERMGMT_MODE_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_MODE_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_POWERMGMT_MODE_MIN						WIFI_POWERSAVEMODE_OFF
#define MIBOID_POWERMGMT_MODE_MAX						WIFI_POWERSAVEMODE_ON

enum 
{
	WIFI_POWERSAVEMODE_OFF	= 0,
	WIFI_POWERSAVEMODE_ON,
};

// ============================================================================

/// @brief Power Management Action
/// Configure the action for the driver to take when it has an oppurtunity to
/// sleep (i.e. there is no downlink traffic buffered at the AP)
#define MIBOID_POWERMGMT_ACTION							0x0002	
#define MIBOID_POWERMGMT_ACTION_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_ACTION_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_MINMAX | \
														MIBOID_ATTRIB_WRITE)
#define MIBOID_POWERMGMT_ACTION_MIN						WIFI_POWERSAVEACTION_ALWAYSON	
#define MIBOID_POWERMGMT_ACTION_MAX						WIFI_POWERSAVEACTION_DEEPSLEEP

enum 
{
	WIFI_POWERSAVEACTION_ALWAYSON	= 0,
	WIFI_POWERSAVEACTION_DOZE,
	WIFI_POWERSAVEACTION_DEEPSLEEP
};

// ============================================================================

/// @brief U-APSD Configuration 
#define MIBOID_POWERMGMT_UAPSDCONFIG					0x0010
#define MIBOID_POWERMGMT_UAPSDCONFIG_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_UAPSDCONFIG_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT)

#define WIFI_UAPSD_AC0_TRIGGER							0x0001
#define WIFI_UAPSD_AC0_DELIVERY							0x0002
#define WIFI_UAPSD_AC1_TRIGGER							0x0004
#define WIFI_UAPSD_AC1_DELIVERY							0x0008
#define WIFI_UAPSD_AC2_TRIGGER							0x0010
#define WIFI_UAPSD_AC2_DELIVERY							0x0020
#define WIFI_UAPSD_AC3_TRIGGER							0x0040
#define WIFI_UAPSD_AC3_DELIVERY							0x0080

#define WIFI_UAPSD_LOW_LATENCY							0x0100


// ============================================================================

/// @brief U-APSD max service period length
#define MIBOID_POWERMGMT_UAPSDMAXSPLEN					0x005E
#define MIBOID_POWERMGMT_UAPSDMAXSPLEN_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_UAPSDMAXSPLEN_ATTRIB			(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT)

#define MIBOID_POWERMGMT_UAPSDMAXSPLEN_MIN				WIFI_UAPSD_MAXSPLEN_ALL
#define MIBOID_POWERMGMT_UAPSDMAXSPLEN_MAX				WIFI_UAPSD_MAXSPLEN_6

enum
{
	WIFI_UAPSD_MAXSPLEN_ALL = 0,
	WIFI_UAPSD_MAXSPLEN_2,
	WIFI_UAPSD_MAXSPLEN_4,
	WIFI_UAPSD_MAXSPLEN_6,
};

// ============================================================================

/// @brief Controls how aggressive the STA is when trying to sleep
/// when idle. The higher the level, the more aggressive the STA will
/// be about trying to save power.
///
/// The STA will try to sleep through multiple DTIM beacons when the
/// level is set above 100. If the user does not want to miss DTIM
/// beacons, then the level should be kept at or below 100. If the
/// level is set too high, applications and protocols that rely on
/// multicast traffic may not operate correctly, e.g. Address
/// Resolution Protocol (ARP).
#define MIBOID_POWERMGMT_LEVEL			0x006F
#define MIBOID_POWERMGMT_LEVEL_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_LEVEL_ATTRIB	(MIBOID_ATTRIB_READ | \
										 MIBOID_ATTRIB_WRITE)

#define MIBOID_POWERMGMT_LEVEL_MIN		0
#define MIBOID_POWERMGMT_LEVEL_MAX		1000

// ============================================================================

///	@brief Authenticaton Response Timeout
/// Number of TUs that a responding STA should wait for the next frame in the
/// authentication sequence.
#define MIBOID_AUTHRESPONSETIMEOUT						0X0003
#define MIBOID_AUTHRESPONSETIMEOUT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_AUTHRESPONSETIMEOUT_MIN					500
#define MIBOID_AUTHRESPONSETIMEOUT_MAX					2000
#define MIBOID_AUTHRESPONSETIMEOUT_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_MINMAX | \
														MIBOID_ATTRIB_WRITE)	

// ============================================================================

/// @brief Association Response Timeout
/// Number of TUs that a responding STA should wait for the next frame in the
/// association sequence.
#define MIBOID_ASSOCRESPONSETIMEOUT						0X0004
#define MIBOID_ASSOCRESPONSETIMEOUT_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_ASSOCRESPONSETIMEOUT_MIN					500
#define MIBOID_ASSOCRESPONSETIMEOUT_MAX					2000
#define MIBOID_ASSOCRESPONSETIMEOUT_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_MINMAX| \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT)

// ============================================================================

/// @brief Traffic stream Timeout (not supported)
#define MIBOID_STREAMTIMEOUT						0X006A
#define MIBOID_STREAMTIMEOUT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_STREAMTIMEOUT_MIN					1000
#define MIBOID_STREAMTIMEOUT_MAX					300000 // 5 minutes
#define MIBOID_STREAMTIMEOUT_ATTRIB					(MIBOID_ATTRIB_READ | \
													 MIBOID_ATTRIB_MINMAX| \
													 MIBOID_ATTRIB_WRITE | \
													 MIBOID_ATTRIB_ADOPT)

// ============================================================================

/// @brief PHY Mode
///
/// Specifies the mode of operation (a or b/g) in which the AP
/// or STA operates.  The bits are defined as follows:
#define MIBOID_PHYMODE									0X0005
#define MIBOID_PHYMODE_TYPE								MIBOID_TYPE_UINT32
#define MIBOID_PHYMODE_ATTRIB							(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_PHYMODE_MIN								PHY_MODE_A_ENABLED	
#define MIBOID_PHYMODE_MAX								PHY_MODE_AG_ENABLED	

enum 
{
	PHY_MODE_A_ENABLED		= 1,
	PHY_MODE_G_ENABLED		= 4,
	PHY_MODE_AG_ENABLED		= 5
};

// ============================================================================

/// @brief BSS Type
/// When configuring Adopt Mib Objects, this MIB Object should be configured
/// first, as it will affect the defaults and the behavior of other adopt MIB
/// objects.
#define MIBOID_BSSTYPE									0X0006
#define MIBOID_BSSTYPE_TYPE								MIBOID_TYPE_UINT32
#define MIBOID_BSSTYPE_ATTRIB							(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT | \
														MIBOID_ATTRIB_MINMAX)

#define MIBOID_BSSTYPE_MIN								DesiredBSSTypeIsIBSS
#define MIBOID_BSSTYPE_MAX								DesiredBSSTypeIs80211SimpleTxMode

enum 
{
	DesiredBSSTypeIsIBSS				= 0,	/// Adhoc mode
	DesiredBSSTypeIsESS					= 1,	/// Infrastructure mode
	DesiredBSSTypeIsAP					= 2,	/// Access Point
	DesiredBSSTypeIsWDS					= 3,	/// WDS
	DesiredBSSTypeIsPromiscuous			= 4,	/// Promiscuous mode

	// The following mode is for Bypass mode only!
	DesiredBSSTypeIs80211SimpleTxMode	= 5,		/// TPROC180 / TPROC185.
};

// ============================================================================

/// @brief Connection State of the device
#define MIBOID_CONNECTIONSTATE							0X0007
#define MIBOID_CONNECTIONSTATE_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_CONNECTIONSTATE_ATTRIB					(MIBOID_ATTRIB_READ)
#define MIBOID_CONNECTIONSTATE_MIN						WIFI_MIB_API_CONNECTION_STATE_STOPPED
#define MIBOID_CONNECTIONSTATE_MAX						WIFI_MIB_API_CONNECTION_STATE_CONNECTED_AND_SCANNING

enum 
{
	/// The device is in a stopped state.  Writes to "adopt" MIB Objects take
	/// effect immediately when the MAC is in this state.
    WIFI_MIB_API_CONNECTION_STATE_STOPPED	= 0,

	/// The device is scanning but the device has not been told to start. 
    WIFI_MIB_API_CONNECTION_STATE_SCANNING	= 1,

	/// The device has been started. Initialization, scanning, authenticating,
	/// and associating are all part of the startup. If a reset command is
	/// sent, the connection state will be this state. 
    WIFI_MIB_API_CONNECTION_STATE_STARTED	= 2,

	/// When a device has successfully authenticated and associated however the
	/// 802.1x handshake may not be completed.
    WIFI_MIB_API_CONNECTION_STATE_CONNECTED	= 3,

	/// The device is in a resetting state due to an internal reason or due to
	/// a reset command.
    WIFI_MIB_API_CONNECTION_STATE_RESETTING	= 4,

	/// The MAC has been started, and a scan is in progress.  When the scan has
	/// completed, it will return to the WIFI_MIB_API_CONNECTION_STATE_STARTED
	/// state.
    WIFI_MIB_API_CONNECTION_STATE_STARTED_AND_SCANNING  = 5,

	/// The MAC is associated and a scan is in progress.  When the scan has
	/// completed, it will return to the
	/// WIFI_MIB_API_CONNECTION_STATE_CONNECTED state.
    WIFI_MIB_API_CONNECTION_STATE_CONNECTED_AND_SCANNING= 6
};

// ============================================================================

/// @brief Dynamic Frequency Selection
/// Virtualization of the MAC's DFS (Dynamic Frequency Selection) register
/// Set to '1' when the phy detecs DFS, otherwise the value is set to 0
#define MIBOID_DFS										0x0008
#define MIBOID_DFS_TYPE									MIBOID_TYPE_UINT32	
#define MIBOID_DFS_ATTRIB								(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_MINMAX | \
														MIBOID_ATTRIB_CLEARONREAD)
#define MIBOID_DFS_MIN									FALSE
#define MIBOID_DFS_MAX									TRUE

// ============================================================================

/// @Spectrum Management Enabled (not supported)
#define MIBOID_SPECTRUM_MANAGEMENT_ENABLED				0X0009
#define MIBOID_SPECTRUM_MANAGEMENT_ENABLED_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_SPECTRUM_MANAGEMENT_ENABLED_ATTRIB		(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_MINMAX | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT)	
#define MIBOID_SPECTRUM_MANAGEMENT_ENABLED_MIN			FALSE
#define MIBOID_SPECTRUM_MANAGEMENT_ENABLED_MAX			TRUE

// ============================================================================

/// @brief Short Retry Limit (not supported)
#define MIBOID_SHORTRETRYLIMIT							0X000B
#define MIBOID_SHORTRETRYLIMIT_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SHORTRETRYLIMIT_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX| \
														MIBOID_ATTRIB_ADOPT)	
#define MIBOID_SHORTRETRYLIMIT_MIN						1
#define MIBOID_SHORTRETRYLIMIT_MAX						255

// ============================================================================

/// @brief Max TX Rate (not supported)
#define MIBOID_MAXTXRATE								0X000C
#define MIBOID_MAXTXRATE_TYPE							MIBOID_TYPE_UINT32
#define MIBOID_MAXTXRATE_ATTRIB							(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_MAXTXRATE_MIN							1
#define MIBOID_MAXTXRATE_MAX							54

// ============================================================================

/// @brief Fallback enabled
/// Modifies MAXTXRATE as either a set rate or limit
#define MIBOID_FALLBACKENABLE							0X000D 
#define MIBOID_FALLBACKENABLE_TYPE						MIBOID_TYPE_UINT32 
#define MIBOID_FALLBACKENABLE_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_ADOPT | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_FALLBACKENABLE_MIN						FALSE	
#define MIBOID_FALLBACKENABLE_MAX						TRUE

// ============================================================================

/// @brief Phy Diversity Support (see MIBOID_DYNAMIC_DIVERSITY)
#define MIBOID_DIVERSITYSUPPORT							0X000E
#define MIBOID_DIVERSITYSUPPORT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_DIVERSITYSUPPORT_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX)

#define MIBOID_DIVERSITYSUPPORT_MIN						WIFI_DIVERSITY_MRC
#define MIBOID_DIVERSITYSUPPORT_MAX						WIFI_DIVERSITY_ANTENNA_B	

enum 
{  
	WIFI_DIVERSITY_MRC			= 0,
	WIFI_DIVERSITY_ANTENNA_A	= 1,
	WIFI_DIVERSITY_ANTENNA_B	= 2
};

// ============================================================================

/// @brief Limit the receive signal detect sensitivity.
/// 
/// The sensitivity level is specified as a dBm value plus
/// MIBOID_RXSENSITIVITY_SCALE. The dBm value is currently quantized and
/// range-limited to values from -97 to -52 in steps of 3 dB.
/// 
/// e.g. 
/// 	-97 dBm => 3
///     3 = -97 + MIBOID_RXSENSITIVITY_SCALE (100)
///     
/// 	-52 dBm => 48
/// 	48 = -52 + MIBOID_RXSENSITIVITY_SCALE (100)
#define MIBOID_RXSENSITIVITY						0X0069
#define MIBOID_RXSENSITIVITY_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_RXSENSITIVITY_ATTRIB					(MIBOID_ATTRIB_READ | \
													 MIBOID_ATTRIB_WRITE | \
													 MIBOID_ATTRIB_MINMAX| \
													 MIBOID_ATTRIB_ADOPT)

#define MIBOID_RXSENSITIVITY_SCALE					100
#define MIBOID_RXSENSITIVITY_DEFAULT				0
#define MIBOID_RXSENSITIVITY_MIN					0
#define MIBOID_RXSENSITIVITY_MAX					(MIBOID_RXSENSITIVITY_SCALE - 1)

// ============================================================================

/// @brief 
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE                  0x006B
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_TYPE             MIBOID_TYPE_UINT32
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_ATTRIB           (MIBOID_ATTRIB_READ | \
																	 MIBOID_ATTRIB_WRITE | \
																	 MIBOID_ATTRIB_MINMAX| \
																	 MIBOID_ATTRIB_ADOPT)

#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_MIN               MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_OFF
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_MAX               MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_AUTO

#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_OFF               0
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_AUTO              1
#define MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_DEFAULT           MIBOID_DYNAMIC_INTERFERENCE_COUNTERMEASURE_OFF

// ============================================================================

/// @brief Scanning mode
#define MIBOID_SCANNINGMODE								0X000F
#define MIBOID_SCANNINGMODE_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANNINGMODE_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX)

#define MIBOID_SCANNINGMODE_MIN							SCANNINGMODE_AUTO
#define MIBOID_SCANNINGMODE_MAX							SCANNINGMODE_ACTIVE

enum
{
	/// The driver will determine the best mode for scanning based on all
	/// available information.  The driver can use the Country String,
	/// Regulatory Domain, or Regulatory Class information MIB objects locally,
	/// and will use Country string, Regulatory Domain, or regulatory Class
	/// information (or whatever else it can use) from Beacons and Probe
	/// Responses. 
	SCANNINGMODE_AUTO		= 0,

	/// Forces the driver to use passive scanning only. 
	SCANNINGMODE_PASSIVE	= 1,

	/// Forces the driver to use active scanning only.   
	SCANNINGMODE_ACTIVE		= 2,

	/// This scanning mode is for internal use only.
	SCANNINGMODE_BACKGROUND = 3,
};

// ============================================================================

/// @brief	List of channel frequencies used during scanning.
///
///			The driver uses this list and the current regulatory information to
///			select which channels are scanned. 
///			
///			Channels that are invalid, duplicates or not supported in the
///			current regulatory domain will be ignored by the driver.
#define MIBOID_SCANCHANFREQLIST								0x0064
#define MIBOID_SCANCHANFREQLIST_TYPE						MIBOID_TYPE_SCANCHANFREQLIST	
#define MIBOID_SCANCHANFREQLIST_ATTRIB						(MIBOID_ATTRIB_READ | \
															MIBOID_ATTRIB_WRITE)

// ============================================================================

/// @brief	Maximum time (in milliseconds) the driver will stay on each channel
/// during a scan.
///			
#define MIBOID_SCANMAXDWELLTIME								0x0065
#define MIBOID_SCANMAXDWELLTIME_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANMAXDWELLTIME_ATTRIB						(MIBOID_ATTRIB_READ | \
															MIBOID_ATTRIB_WRITE | \
															MIBOID_ATTRIB_MINMAX)
#define MIBOID_SCANMAXDWELLTIME_MIN							20	
#define MIBOID_SCANMAXDWELLTIME_MAX							500

// ============================================================================

///	@brief Any scan result than has not been seen in
///	MIBOID_SCANCACHETIMEOUT milliseconds is flushed from the scan
///	cache prior to starting a new scan.
///
/// Setting this to zero effectivly means the driver will flush the
/// entire cache everytime a new scan is started.
#define MIBOID_SCANCACHETIMEOUT								0x0068
#define MIBOID_SCANCACHETIMEOUT_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANCACHETIMEOUT_ATTRIB						(MIBOID_ATTRIB_READ | \
															MIBOID_ATTRIB_WRITE | \
															MIBOID_ATTRIB_MINMAX)
#define MIBOID_SCANCACHETIMEOUT_MIN							0	
#define MIBOID_SCANCACHETIMEOUT_MAX							300000

// ============================================================================

/// @brief Current operating Channel
/// Sets the channel on which to operate.  For BSS Type DesiredBSSTypeIsESS,
/// the channel can not be set, but reading this MIB object indicates the
/// channel on which the intrastructure STA is operating.
#define MIBOID_CURRENTCHANNEL						0X0013
#define MIBOID_CURRENTCHANNEL_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_CURRENTCHANNEL_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT)

// ============================================================================

/// @brief Current operating frequency
/// Sets the frequency on which to operate.  For BSS Type DesiredBSSTypeIsESS,
/// the frequency can not be set, but reading this MIBOID indicates the
/// frequency on which the intrastructure STA is operating.
#define MIBOID_CURRENTCHANNELFREQUENCY				0X0014
#define MIBOID_CURRENTCHANNELFREQUENCY_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_CURRENTCHANNELFREQUENCY_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT )

// ============================================================================

/// @brief Desired SSID 
/// For STA use: Associate to an AP with this SSID. 
/// For AP use: Use this SSID.
#define MIBOID_DESIREDSSID							0X0015
#define MIBOID_DESIREDSSID_TYPE						MIBOID_TYPE_SSID
#define MIBOID_DESIREDSSID_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT)

// ============================================================================

/// @brief Desired BSSID
/// For STA use: Associate to an AP with this BSSID. 
/// For AP use: Illegal
#define MIBOID_DESIREDBSSID							0X0016
#define MIBOID_DESIREDBSSID_TYPE					MIBOID_TYPE_MACADDR
#define MIBOID_DESIREDBSSID_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT | \
													MIBOID_ATTRIB_STAONLY)	

// ============================================================================

/// @brief SSID for scanning
#define MIBOID_SSID_FOR_SCANNING					0X0017
#define MIBOID_SSID_FOR_SCANNING_TYPE				MIBOID_TYPE_SSID
#define MIBOID_SSID_FOR_SCANNING_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE)

// ============================================================================

/// @brief BSSID for scanning
#define MIBOID_BSSID_FOR_SCANNING					0X0018
#define MIBOID_BSSID_FOR_SCANNING_TYPE				MIBOID_TYPE_MACADDR
#define MIBOID_BSSID_FOR_SCANNING_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_STAONLY)	

// ============================================================================

/// @brief Associated BSSID
/// If the STA is associated, this is the BSSID of the AP.
/// TODO Is this a STA only miboid?
#define MIBOID_ASSOCIATEDBSSID						0X0019
#define MIBOID_ASSOCIATEDBSSID_TYPE					MIBOID_TYPE_MACADDR
#define MIBOID_ASSOCIATEDBSSID_ATTRIB				(MIBOID_ATTRIB_READ)	

// ============================================================================

/// @brief Device MAC Address
/// The devices 48-bit MAC Address
#define MIBOID_MACADDRESS							0X001A
#define MIBOID_MACADDRESS_TYPE						MIBOID_TYPE_MACADDR
#define MIBOID_MACADDRESS_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT )

// ============================================================================

/// @brief Operational Rate Set
/// 
/// Set of rates the driver will use during association. This MIBOID is a
/// bitmap with each bit representing an operational rate (see
/// WIFI_RATE_Mbs_*). The caller should read the default operational rates and
/// disable any undesired rate(s).
/// 
/// The STA driver may not be able to establish an association with an AP if a
/// basic rate is disabled that the AP requires.
///
/// NOTE: This is initally a subset of the rates listed in MIBOID_SUPPORTEDRATESET. 
#define MIBOID_OPERATIONALRATESET					0X001B
#define MIBOID_OPERATIONALRATESET_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_OPERATIONALRATESET_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT)

#define	WIFI_RATE_Mbs_1		0x00000001	
#define	WIFI_RATE_Mbs_2		0x00000006	// long and short preamble
#define	WIFI_RATE_Mbs_5p5	0x00000018	// long and short preamble
#define	WIFI_RATE_Mbs_11	0x00000060	// long and short preamble
#define	WIFI_RATE_Mbs_6		0x00000080	
#define	WIFI_RATE_Mbs_9		0x00000100
#define	WIFI_RATE_Mbs_12	0x00000200
#define	WIFI_RATE_Mbs_18	0x00000400
#define	WIFI_RATE_Mbs_24	0x00000800
#define	WIFI_RATE_Mbs_36	0x00001000
#define	WIFI_RATE_Mbs_48	0x00002000
#define	WIFI_RATE_Mbs_54	0x00004000

#define	WIFI_RATE_MCS_IDX_00 0x00008000
#define	WIFI_RATE_MCS_IDX_01 0x00010000
#define	WIFI_RATE_MCS_IDX_02 0x00020000
#define	WIFI_RATE_MCS_IDX_03 0x00040000
#define	WIFI_RATE_MCS_IDX_04 0x00080000
#define	WIFI_RATE_MCS_IDX_05 0x00100000
#define	WIFI_RATE_MCS_IDX_06 0x00200000
#define	WIFI_RATE_MCS_IDX_07 0x00400000

// ============================================================================

/// @brief Supported Rate Set
/// Set of rates that the AP/STA supports. The SUPPORTED_RATES structure
/// contains an array of octects representing all of the devices supported
/// rates. 
///
/// Each octect is encoded according to 7.3.2.2 of the IEEE 802.11
/// specification. If the number of supported rates is less than MAX_RATES, the
/// array of octects will be 'null' terminated with an octect value of zero. 
#define MIBOID_SUPPORTEDRATESET						0X001C
#define MIBOID_SUPPORTEDRATESET_ATTRIB				MIBOID_TYPE_SUPPORTEDRATES
#define MIBOID_SUPPORTEDRATESET_TYPE				(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Country String in which this Wifi MAC will operate. (not supported)
///
/// For each country string, the appropriate set of supported rates, scanning
/// mode, channels, and power level are selected.  
///
/// TODO jyoung What is the requirement for WorldMode?
///
/// If worldMode is enabled with MIBOID_BSSTYPE set to DesiredBSSTypeIsESS then
/// setting the Country String is only a guideline as the STA will use passive
/// scanning on all channels in order to determine the operating environment by
/// either getting  the country string, regulatory domain, regulatory class or
/// transmit power constraints from the Beacon.  
///
/// If wordMode is disabled with the BSS Type set to DesiredBSSTypeIsESS then
/// the STA will be restricted to using only the channels available for that
/// country, but it will attempt to use active scanning if the country allows
/// it.
#define MIBOID_COUNTRYSTRING						0X001D
#define MIBOID_COUNTRYSTRING_TYPE					MIBOID_TYPE_COUNTRYSTRING
#define MIBOID_COUNTRYSTRING_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT )

// See ISO/IEC 3166-1
#define MIBOID_COUNTRYSTRING_ARGENTINA				"AR "
#define MIBOID_COUNTRYSTRING_AUSTRALIA				"AU "
#define MIBOID_COUNTRYSTRING_CHINA					"CN "
#define MIBOID_COUNTRYSTRING_FRANCE					"FR "
#define MIBOID_COUNTRYSTRING_GERMANY				"GR "
#define MIBOID_COUNTRYSTRING_HONG_KONG				"HK "
#define MIBOID_COUNTRYSTRING_INDIA					"IN "
#define MIBOID_COUNTRYSTRING_ISRAEL					"IL "
#define MIBOID_COUNTRYSTRING_ITALY					"IT" 
#define MIBOID_COUNTRYSTRING_JAPAN					"JP "
#define MIBOID_COUNTRYSTRING_TAIWAN					"TW "
#define MIBOID_COUNTRYSTRING_UNITED_KINGDOM			"GB "
#define MIBOID_COUNTRYSTRING_UNITED_STATES			"US "
// #define MIBOID_COUNTRYSTRING_SOUTH_KOREA			"KR "

// ============================================================================

//
// MAC EDCA Table
//
// The STA driver will try to use parameters as a baseline configuration during
// association. The actual EDCA parameters used for a successfull association
// may differ per the BSSID's requirements. 
// 

#define MIBOID_EDCATABLEINDEX						0X001E
#define MIBOID_EDCATABLEINDEX_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_EDCATABLEINDEX_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)
#define MIBOID_EDCATABLEINDEX_MIN					1			
#define MIBOID_EDCATABLEINDEX_MAX					4

// ============================================================================

/// @brief CWmin for STA
#define MIBOID_EDCATABLECWMIN						0X001F
#define MIBOID_EDCATABLECWMIN_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_EDCATABLECWMIN_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT | \
													MIBOID_ATTRIB_INDEX | \
													MIBOID_ATTRIB_MINMAX)

#define MIBOID_EDCATABLECWMIN_MIN					1
#define MIBOID_EDCATABLECWMIN_MAX					255

// ============================================================================

/// @brief CWmax for STA
#define MIBOID_EDCATABLECWMAX						0X0020
#define MIBOID_EDCATABLECWMAX_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_EDCATABLECWMAX_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT | \
													MIBOID_ATTRIB_INDEX | \
													MIBOID_ATTRIB_MINMAX)

#define MIBOID_EDCATABLECWMAX_MIN					3
#define MIBOID_EDCATABLECWMAX_MAX					65535

// ============================================================================

/// @brief Aifsn for STA
#define MIBOID_EDCATABLEAIFSN						0X0021
#define MIBOID_EDCATABLEAIFSN_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_EDCATABLEAIFSN_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT | \
													MIBOID_ATTRIB_INDEX | \
													MIBOID_ATTRIB_MINMAX)
#define MIBOID_EDCATABLEAIFSN_MIN					1
#define MIBOID_EDCATABLEAIFSN_MAX					15

// ============================================================================

/// @brief Access Control Mandatory for STA 
#define MIBOID_EDCATABLEMANDATORY					0X0022
#define MIBOID_EDCATABLEMANDATORY_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_EDCATABLEMANDATORY_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT | \
													MIBOID_ATTRIB_INDEX | \
													MIBOID_ATTRIB_MINMAX)
#define MIBOID_EDCATABLEMANDATORY_MIN				FALSE	
#define MIBOID_EDCATABLEMANDATORY_MAX				TRUE

// ============================================================================

//
// MAC Counters
//

/// @brief Failed Packet Count
/// Maximum transmit attempts tried
#define MIBOID_FAILEDCOUNT							0X0023
#define MIBOID_FAILEDCOUNT_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_FAILEDCOUNT_ATTRIB					(MIBOID_ATTRIB_READ)	

// ============================================================================

/// @brief Packet Retry Count
/// Lifetime retry count since driver initialized
#define MIBOID_RETRYCOUNT							0X0024
#define MIBOID_RETRYCOUNT_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_RETRYCOUNT_ATTRIB					(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Multiple Retry Count 
//#define MIBOID_MULTIPLERETRYCOUNT				0X0025
#define MIBOID_OUTOFBUFFERCOUNT						0X0025
#define MIBOID_OUTOFBUFFERCOUNT_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_OUTOFBUFFERCOUNT_ATTRIB				(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Duplicated Frame Count 
#define MIBOID_FRAMEDUPLICATECOUNT					0X0026
#define MIBOID_FRAMEDUPLICATECOUNT_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_FRAMEDUPLICATECOUNT_ATTRIB			(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Frame Checksum Count
/// Lifetime FCS error count since driver initialized
#define MIBOID_FCSERRORCOUNT						0X0027
#define MIBOID_FCSERRORCOUNT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_FCSERRORCOUNT_ATTRIB					(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Received Fragment Count
/// Incremented whenever FCS clean (fragment) packet is received
#define MIBOID_RECEIVEDFRAGCOUNT					0x0011
#define MIBOID_RECEIVEDFRAGCOUNT_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_RECEIVEDFRAGCOUNT_ATTRIB				(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Transmitted frame count
/// Lifetime frame count - retries, fragments, etc.
#define MIBOID_TRANSMITTEDFRAMECOUNT				0X0028
#define MIBOID_TRANSMITTEDFRAMECOUNT_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_TRANSMITTEDFRAMECOUNT_ATTRIB			(MIBOID_ATTRIB_READ)

// ============================================================================

//
// MAC QoS Counters Table
//

/// @brief MAC QoS Counters Table index
#define MIBOID_QOSCOUNTERSINDEX						0X002C
#define MIBOID_QOSCOUNTERSINDEX_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_QOSCOUNTERSINDEX_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)
#define MIBOID_QOSCOUNTERSINDEX_MIN					1
#define MIBOID_QOSCOUNTERSINDEX_MAX					16	

// ============================================================================

/// @brief Failed Packet count per TID. 
/// Incremented when the driver cannot transmit a packet and has exhausted all
/// retry attempts.
#define MIBOID_QOSFAILEDCOUNT						0X002D
#define MIBOID_QOSFAILEDCOUNT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_QOSFAILEDCOUNT_ATTRIB				(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Retry Packet count per TID
/// Incremented each time the MAC restransmits a packet because no ACK was
/// received. If the MAC retransmits a packet 14 times before finally failing
/// this counter will be incremented by 14.
#define MIBOID_QOSRETRYCOUNT						0X002E
#define MIBOID_QOSRETRYCOUNT_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_QOSRETRYCOUNT_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Out of buffer count per TID
/// Incremented when the driver cannot allocate a buffer to transmit a packet.
#define MIBOID_QOSOUTOFBUFFERCOUNT					0X002F
#define MIBOID_QOSOUTOFBUFFERCOUNT_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_QOSOUTOFBUFFERCOUNT_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Duplicated frame count per TID
#define MIBOID_QOSFRAMEDUPLICATECOUNT				0X0030
#define MIBOID_QOSFRAMEDUPLICATECOUNT_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_QOSFRAMEDUPLICATECOUNT_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Transmitted frame count per TID
#define MIBOID_QOSTRANSMITTEDFRAMECOUNT				0X0031
#define MIBOID_QOSTRANSMITTEDFRAMECOUNT_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_QOSTRANSMITTEDFRAMECOUNT_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Discarded frame count per TID
#define MIBOID_QOSDISCARDEDFRAMECOUNT				0X0032
#define MIBOID_QOSDISCARDEDFRAMECOUNT_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_QOSDISCARDEDFRAMECOUNT_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Received MPDU count per TID
/// Incremented whenever a 802.11 frame is successfully received.
#define MIBOID_QOSMPDUSRECEIVEDCOUNT				0X0033
#define MIBOID_QOSMPDUSRECEIVEDCOUNT_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_QOSMPDUSRECEIVEDCOUNT_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Received retries count per TID
#define MIBOID_QOSRETRIESRECEIVEDCOUNT				0X0034
#define MIBOID_QOSRETRIESRECEIVEDCOUNT_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_QOSRETRIESRECEIVEDCOUNT_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Average RSSI of received beacon frames for the current BSS.
// 
// The Kona driver uses a formula to derive dBm from RSSI.
//
// Subtract 100 from RSSI to derive dBm.
//
// See www.wildpackets.com/elements/whitepapers/Converting_Signal_Strength.pdf
// for description of how other vendors handle conversion of signal
// level and rssi to dBm.
#define MIBOID_AVERAGERSSI							0X0035
#define MIBOID_AVERAGERSSI_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_AVERAGERSSI_ATTRIB					(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Window size used to calcuate MIBOID_AVERAGERSSI. Every X frames the
/// average RSSI is recalculated where X is the average rssi window size.
#define MIBOID_AVERAGERSSIWINDOWSIZE				0X0036
#define MIBOID_AVERAGERSSIWINDOWSIZE_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_AVERAGERSSIWINDOWSIZE_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE)
#define MIBOID_AVERAGERSSIWINDOWSIZE_MIN			1	
#define MIBOID_AVERAGERSSIWINDOWSIZE_MAX			20

// ============================================================================

/// @brief Average RSSI of received data frames for the current BSS.  
/// (see MIBOID_AVERAGERSSI for RSSI-to-dBm formula)
#define MIBOID_AVERAGERSSIDATA						0X0037
#define MIBOID_AVERAGERSSIDATA_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_AVERAGERSSIDATA_ATTRIB				(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Window size used to calcuate MIBOID_AVERAGERSSIDATA. Every X frames
/// the average RSSI is recalculated where X is the average rssi data window
/// size.
#define MIBOID_AVERAGERSSIDATAWINDOWSIZE			0X0038
#define MIBOID_AVERAGERSSIDATAWINDOWSIZE_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_AVERAGERSSIDATAWINDOWSIZE_ATTRIB		(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE)
#define MIBOID_AVERAGERSSIDATAWINDOWSIZE_MIN		1	
#define MIBOID_AVERAGERSSIDATAWINDOWSIZE_MAX		100	
// ============================================================================

//
// MAC Group Address Table (not supported)
//

#define MIBOID_GROUPADDRESSESINDEX					0X0039
#define MIBOID_GROUPADDRESSESINDEX_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_GROUPADDRESSESINDEX_ATTRIB			(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)
#define MIBOID_GROUPADDRESSESINDEX_MIN				1
#define MIBOID_GROUPADDRESSESINDEX_MAX				8	

// ============================================================================

#define MIBOID_GROUPADDRESS							0X003A
#define MIBOID_GROUPADDRESS_TYPE					MIBOID_TYPE_MACADDR
#define MIBOID_GROUPADDRESS_ATTRIB					(MIBOID_ATTRIB_READ | \
													MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_INDEX)

// ============================================================================

#define MIBOID_GROUPADDRESSESSTATUS						0X003B
#define MIBOID_GROUPADDRESSESSTATUS_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_GROUPADDRESSESSTATUS_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Current Regulatory Domain (not supported)
#define MIBOID_CURRENTREGDOMAIN						0X003C
#define MIBOID_CURRENTREGDOMAIN_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_CURRENTREGDOMAIN_ATTRIB				(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief Max TX Power STA is capable of transmitting in the current channel (in dbm) (not supported)
#define MIBOID_MAXTXPOWER								0X003D
#define MIBOID_MAXTXPOWER_TYPE							MIBOID_TYPE_UINT32
#define MIBOID_MAXTXPOWER_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE)
#define MIBOID_MAXTXPOWER_DEFAULT                       19
 
// ============================================================================

//
// DSPG Scan List Table
//

/// @brief Scan List Table Size
/// Specifies the number of valid entries in the scan list table, i.e., how
/// many APs were detected during the scan.
#define MIBOID_SCANLISTSIZE								0X003E
#define MIBOID_SCANLISTSIZE_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTSIZE_ATTRIB						(MIBOID_ATTRIB_READ)

// ============================================================================

///  @brief Index for the Scan list Table
///  This MIB Object is used as an array index into the scan list table for the
///  following Mib Objects:  MIBOID_SCANLISTAUTHENTICATIONALGORITHM
///  MIBOID_SCANLISTSSID                                             
#define MIBOID_SCANLISTINDEX							0X003F
#define MIBOID_SCANLISTINDEX_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTINDEX_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_SCANLISTINDEX_MIN						1
#define MIBOID_SCANLISTINDEX_MAX						WIFIAPI_CFG_MAXSCANCACHESIZE	

// ============================================================================

/// @brief Detected AP's BSSID
#define MIBOID_SCANLISTBSSID							0X0041
#define MIBOID_SCANLISTBSSID_TYPE						MIBOID_TYPE_MACADDR
#define MIBOID_SCANLISTBSSID_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Detected AP's Capabiltities
#define MIBOID_SCANLISTCAPABILITIES						0X0042
#define MIBOID_SCANLISTCAPABILITIES_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTCAPABILITIES_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

#define WIFI_MIB_API_CAPABILITIES_INFRASTRUCTURE_MODE	0x0001
#define WIFI_MIB_API_CAPABILITIES_ADHOC_MODE			0x0002
#define WIFI_MIB_API_CAPABILITIES_CF_POLLABLE			0x0004
#define WIFI_MIB_API_CAPABILITIES_CF_POLL_REQUEST		0x0008
#define WIFI_MIB_API_CAPABILITIES_PRIVACY				0x0010
#define WIFI_MIB_API_CAPABILITIES_SHORT_PREAMBLE		0x0020
#define WIFI_MIB_API_CAPABILITIES_PBCC					0x0040
#define WIFI_MIB_API_CAPABILITIES_CHANNEL_AGILITY		0x0080
#define WIFI_MIB_API_CAPABILITIES_SPECTRUM_MANAGEMENT	0x0100
#define WIFI_MIB_API_CAPABILITIES_QOS					0x0200
#define WIFI_MIB_API_CAPABILITIES_SHORT_SLOT_TIME		0x0400
#define WIFI_MIB_API_CAPABILITIES_APSD					0x0800
#define WIFI_MIB_API_CAPABILITIES_RESERVED				0x1000
#define WIFI_MIB_API_CAPABILITIES_DSSS_OFDM				0x2000
#define WIFI_MIB_API_CAPABILITIES_DELAYED_BLOCK_ACK		0x4000
#define WIFI_MIB_API_CAPABILITIES_IMMEDIATE_BLOCK_ACK	0x8000

// ============================================================================

/// @brief Detected AP's operating channel
#define MIBOID_SCANLISTCHANNEL							0X0043
#define MIBOID_SCANLISTCHANNEL_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTCHANNEL_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Snapshot of RSSIs of the detected AP during the scan
/// (see MIBOID_AVERAGERSSI for RSSI-to-dBm formula)
#define MIBOID_SCANLISTRSSI								0X0045
#define MIBOID_SCANLISTRSSI_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTRSSI_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

// @brief Detected AP's band
#define MIBOID_SCANLISTBAND								0X0046
#define MIBOID_SCANLISTBAND_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTBAND_ATTRIB						(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)
#define WIFI_MIB_API_SCANLIST_BAND_5GHZ					1
#define WIFI_MIB_API_SCANLIST_BAND_24GHZ				2

// ============================================================================

/// @brief Detected AP's beacon interval of the AP (in TUs)
#define MIBOID_SCANLISTBEACONINTERVAL					0X0047
#define MIBOID_SCANLISTBEACONINTERVAL_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTBEACONINTERVAL_ATTRIB			(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Timestamp of the beacon that was used to gather information of the AP 
#define MIBOID_SCANLISTBEACONTIMESTAMP					0X0048
#define MIBOID_SCANLISTBEACONTIMESTAMP_TYPE				MIBOID_TYPE_UINT32		
#define MIBOID_SCANLISTBEACONTIMESTAMP_ATTRIB			(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Number of bytes in the MIBOID_SCANLISTINFORMATIONELEMENTS that can
/// be parsed for Information Elements.
#define MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS		0X0049
#define MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS_TYPE	MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS_ATTRIB	(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Information Element Buffer.
///
/// This buffer contains all of the Information Elements (recognized or not by
/// the driver) from the AP's Beacon Frame. The AP's SSID, Supported Rates, RSN
/// and WMM information elements (if present in the beacon) can be parsed from
/// this buffer. 
///
/// The size of this buffer can be determined by reading
/// MIBOID_SCANLISTSIZEOFINFORMATIONELEMENTS.
#define MIBOID_SCANLISTINFORMATIONELEMENTS				0X004A
#define MIBOID_SCANLISTINFORMATIONELEMENTS_TYPE			MIBOID_TYPE_IE	
#define MIBOID_SCANLISTINFORMATIONELEMENTS_ATTRIB		(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

/// @brief Detected APs' phy mode
/// Specifies the mode of operation (a, b, and g) of the AP found.  The bits
/// are defined as follows:
///
/// Bit 0:    0-disables 802.11a operation, 1-indicates 802.11a operation
/// Bit 1:    0-disables 802.11b operation, 1-indicates 802.11b operation.
/// Bit 2:    0-disables 802.11g operation, 1-indicates 802.11g operation.
#define MIBOID_SCANLISTPHYMODE							0X004B
#define MIBOID_SCANLISTPHYMODE_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_SCANLISTPHYMODE_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_INDEX)

// ============================================================================

//
// Command Table
//

/// @brief DSPG driver command
///
/// Writing this MIB has no effect if the MIBOID_BSSTYPE is
/// DesiredBSSTypeIsIBSS, DesiredBSSTypeIsWDS or
/// DesiredBSSTypeIsPromiscuous.
#define MIBOID_DSPGCOMMAND								0X004C
#define MIBOID_DSPGCOMMAND_TYPE							MIBOID_TYPE_UINT32
#define MIBOID_DSPGCOMMAND_ATTRIB						(MIBOID_ATTRIB_WRITE | \
														MIBOID_ATTRIB_MINMAX)
#define MIBOID_DSPGCOMMAND_MIN							WIFI_MIB_API_COMMAND_STOP	
#define MIBOID_DSPGCOMMAND_MAX							WIFI_MIB_API_COMMAND_REASSOC

enum 
{
	// Disconnect from the AP (as a station) or shutdown the BSS (as
	// an AP) and turn off TX/RX path (all).
	WIFI_MIB_API_COMMAND_STOP	= 0,
	
	// Connect to a BSS (as a station) or start a new BSS (as an AP)
	// and turn on TX/RX path (all).
	WIFI_MIB_API_COMMAND_START	= 1,
	
	// This command is not supported at this time
	WIFI_MIB_API_COMMAND_RESET  = 2,

	// Scan for available BSS (as a station). This does nothing if the
	// device is configured for DesiredBSSTypeIsAP or
	// DesiredBSSTypeIs80211SimpleTxMode.
	WIFI_MIB_API_COMMAND_SCAN	= 3,

	// As a station, try to reassociate with the current BSS. This is ignored
	// if the device is an AP, or if the station is not already associated.
	WIFI_MIB_API_COMMAND_REASSOC = 4,
};

// ============================================================================

/// @brief Force the STA to retreive buffered traffic from the AP
/// during powersave.
///
/// When the STA is in powersave, downlink traffic is buffered at the
/// AP until it can be retrieved implicitly by sending normal traffic
/// (i.e. U-APSD) or explicitly by the driver (i.e. Legacy-PS and/or U-APSD).
///
/// The value written to MIBOID_POWERMGMT_AUTO_TRIGGER is the maximum time in
/// milliseconds that the station will try allow between trigger frames. If
/// there is insufficient data traffic the station may send explicit trigger
/// frames to satisfy this requirement.
///
/// This feature can be disabled by writing a zero value to MIBOID_POWERMGMT_AUTO_TRIGGER.
///
/// While it is safe to write this MIBOID at any time, it will have no
/// effect unless:
/// - MIBOID_BSSTYPE is DesiredBSSTypeIsESS
/// - MIBOID_POWERMGMT_MODE is WIFI_POWERSAVEMODE_ON
/// - MIBOID_CONNECTIONSTATE is WIFI_MIB_API_CONNECTION_STATE_CONNECTED
#define MIBOID_POWERMGMT_AUTO_TRIGGER							0X004D
#define MIBOID_POWERMGMT_AUTO_TRIGGER_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_POWERMGMT_AUTO_TRIGGER_ATTRIB					(MIBOID_ATTRIB_WRITE | \
														 MIBOID_ATTRIB_READ | \
														 MIBOID_ATTRIB_MINMAX)

#define MIBOID_POWERMGMT_AUTO_TRIGGER_MIN 0
#define MIBOID_POWERMGMT_AUTO_TRIGGER_MAX 1000

// ============================================================================

//
// Debug MIBOIDS
//

/// @brief MAC Register Access Address
/// Specifies the physical address of a MAC register.  
///
/// Baseband/PHY:	0 <= addr < 0x179
/// MAC:			(see MemMap.h and wifiRegisterMap.h for base physical
///					address and offsets)  
#define MIBOID_MAC_REG_ACCESS_ADDR						0X005A
#define MIBOID_MAC_REG_ACCESS_ADDR_TYPE					MIBOID_TYPE_UINT32
#define MIBOID_MAC_REG_ACCESS_ADDR_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE)
// ============================================================================

/// @brief MAC Register Access Value 
/// Value of specified MAC register for reading or writing. 
#define MIBOID_MAC_REG_ACCESS_DWORD						0X005B
#define MIBOID_MAC_REG_ACCESS_DWORD_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_MAC_REG_ACCESS_DWORD_ATTRIB				(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE)
// ============================================================================

/// @brief Debug printouts for modules at runtime
#define MIBOID_DBGMODULEFLAGS							0x005C
#define MIBOID_DBGMODULEFLAGS_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_DBGMODULEFLAGS_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE)
// ============================================================================

/// @brief Filter debug prints at runtime
#define MIBOID_DBGFILTERFLAGS							0x005D	
#define MIBOID_DBGFILTERFLAGS_TYPE						MIBOID_TYPE_UINT32
#define MIBOID_DBGFILTERFLAGS_ATTRIB					(MIBOID_ATTRIB_READ | \
														MIBOID_ATTRIB_WRITE)
// ============================================================================

/// @brief Transmit a batch of TX packets.
/// 
/// This miboid requires that the BSSTYPE be set to DesiredBSSTypeIs80211SimpleTxMode.
///
/// When this MIBOID is set the driver will begin transmitting a batch of tx packet(s)
/// described by the dwMibTxBatch_t structure. 
///
/// Batch TX will continue until all of the packets are transmitted or MIBOID_DBGBATCHTX
/// is set again with numTransmits equal to zero.
#define MIBOID_DBGBATCHTX								0x005F
#define MIBOID_DBGBATCHTX_TYPE							MIBOID_TYPE_TXBATCH
#define MIBOID_DBGBATCHTX_ATTRIB						(MIBOID_ATTRIB_WRITE | MIBOID_ATTRIB_READ)

// ============================================================================

//
// RSNA Statistics
//

/// @brief CCMP Decrypt error counters
#define MIBOID_RSNASTATS_CCMPDECRYPTERRORS				0x0060
#define MIBOID_RSNASTATS_CCMPDECRYPTERRORS_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_RSNASTATS_CCMPDECRYPTERRORS_ATTRIB		(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief TKIP MIC failure error counters
#define MIBOID_RSNASTATS_TKIPLOCALMICFAILURES			0x0061
#define MIBOID_RSNASTATS_TKIPLOCALMICFAILURES_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_RSNASTATS_TKIPLOCALMICFAILURES_ATTRIB	(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief TKIP replay error counters
#define MIBOID_RSNASTATS_TKIPREPLAYS				0x0062
#define MIBOID_RSNASTATS_TKIPREPLAYS_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_RSNASTATS_TKIPREPLAYS_ATTRIB			(MIBOID_ATTRIB_READ)

// ============================================================================

/// @brief TX Power Table
#define MIBOID_POWERTABLE				            0x0063
#define MIBOID_POWERTABLE_TYPE			            MIBOID_TYPE_POWERTABLE
#define MIBOID_POWERTABLE_ATTRIB			        (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE)

// ============================================================================

/// @brief	PHY Parameter
#define MIBOID_PHYPARAMETER							0x0066
#define MIBOID_PHYPARAMETER_TYPE					MIBOID_TYPE_PHYPARAMETER
#define MIBOID_PHYPARAMETER_ATTRIB					(MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_ADOPT)

enum
{
	WIFI_PHYPARAM_CHIPSET_ALIAS    = 1,
	WIFI_PHYPARAM_DEFAULTREGISTERS = 2,
	WIFI_PHYPARAM_REGISTERSBYBAND  = 3,
	WIFI_PHYPARAM_REGISTERSBYCHANL = 4,
	WIFI_PHYPARAM_REGISTERSBYCHANL24 = 5
};

// ============================================================================

/// @brief	
#define MIBOID_HWMODES								0X0067
#define MIBOID_HWMODES_TYPE							MIBOID_TYPE_HWMODES
#define MIBOID_HWMODES_ATTRIB						MIBOID_ATTRIB_READ

#define WIFI_HWMODES_MAXCHANNELS					128
#define WIFI_HWMODES_MAXMODES						3	/// i.e. A, B, G

enum
{
	WIFI_HWMODES_A,
	WIFI_HWMODES_B,
	WIFI_HWMODES_G,
};

// ============================================================================

/// @brief Tx Fallbzck Floor
#define MIBOID_FALLBACKFLOOR				        0x006C
#define MIBOID_FALLBACKFLOOR_TYPE			        MIBOID_TYPE_UINT32
#define MIBOID_FALLBACKFLOOR_ATTRIB			        (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)

#define MIBOID_FALLBACKFLOOR_MIN					FALSE
#define MIBOID_FALLBACKFLOOR_MAX					TRUE
// ============================================================================

/// @brief Tx Fallback Floor DBM Threshold
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD			0x006D
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_ATTRIB	(MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)

#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_SCALE     100
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_DEFAULT   25  ///-75dbm
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_MIN		0
#define MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_MAX		( MIBOID_FALLBACKFLOOR_DBMTHRESHOLD_SCALE -1 )

// ============================================================================

/// @brief Tx Fallback Floor DBM Margin
#define MIBOID_FALLBACKFLOOR_DBMMARGIN				0x006E
#define MIBOID_FALLBACKFLOOR_DBMMARGIN_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_FALLBACKFLOOR_DBMMARGIN_ATTRIB		(MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
													MIBOID_ATTRIB_MINMAX)

#define MIBOID_FALLBACKFLOOR_DBMMARGIN_DEFAULT      5 
#define MIBOID_FALLBACKFLOOR_DBMMARGIN_MIN			0
#define MIBOID_FALLBACKFLOOR_DBMMARGIN_MAX			15

// ============================================================================

/// @brief Min TX Power STA is capable of transmitting in the current channel (in dbm) (not supported)
/// 
#define MIBOID_MINTXPOWER                           0X0070
#define MIBOID_MINTXPOWER_TYPE                      MIBOID_TYPE_UINT32
#define MIBOID_MINTXPOWER_ATTRIB                    (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE)

#define MIBOID_MINTXPOWER_DEFAULT                   5 
// ============================================================================

/// @brief Max TX Power allowed in the current channel (in dbm)
/// 
#define MIBOID_MAXTXPOWER_ALLOWED                   0X0071
#define MIBOID_MAXTXPOWER_ALLOWED_TYPE              MIBOID_TYPE_UINT32
#define MIBOID_MAXTXPOWER_ALLOWED_ATTRIB            (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE)

#define MIBOID_MAXTXPOWER_ALLOWED_DEFAULT           19 
// ============================================================================

/// @brief Power Constraint in the current channel (in dbm) 
/// 
#define MIBOID_POWERCONSTRAINT                      0X0072
#define MIBOID_POWERCONSTRAINT_TYPE                 MIBOID_TYPE_UINT32
#define MIBOID_POWERCONSTRAINT_ATTRIB               (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE)
 
#define MIBOID_POWERCONSTRAINT_DEFAULT              0
// ============================================================================

/// @brief Number of Probe for scanning
///  
#define MIBOID_NUMPROBE_FOR_SCANNING                0X0073
#define MIBOID_NUMPROBE_FOR_SCANNING_TYPE           MIBOID_TYPE_UINT32
#define MIBOID_NUMPROBE_FOR_SCANNING_ATTRIB         (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_NUMPROBE_FOR_SCANNING_DEFAULT        2
#define MIBOID_NUMPROBE_FOR_SCANNING_MIN            1
#define MIBOID_NUMPROBE_FOR_SCANNING_MAX            6

// ============================================================================

/// @brief	Home dwell time (in milliseconds) the driver will stay on the home channel 
/// as the driver switches between different channels during a background scanning.
///			
#define MIBOID_SCANHOMEDWELLTIME                    0x0074
#define MIBOID_SCANHOMEDWELLTIME_TYPE               MIBOID_TYPE_UINT32
#define MIBOID_SCANHOMEDWELLTIME_ATTRIB             (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_SCANHOMEDWELLTIME_MIN                10	
#define MIBOID_SCANHOMEDWELLTIME_MAX                200

// ============================================================================

/// @brief Number of Internal Scan.	 
/// 
///			
#define MIBOID_NUMOFINTERNALSCAN                    0x0075
#define MIBOID_NUMOFINTERNALSCAN_TYPE               MIBOID_TYPE_UINT32
#define MIBOID_NUMOFINTERNALSCAN_ATTRIB             (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_NUMOFINTERNALSCAN_MIN                1	
#define MIBOID_NUMOFINTERNALSCAN_MAX                10

// ============================================================================

/// @brief Current PHY mode of the AP to which our STA is connected.	 
///			
#define MIBOID_CURRENTPHYMODE                        0x0076
#define MIBOID_CURRENTPHYMODE_TYPE                   MIBOID_TYPE_UINT32
#define MIBOID_CURRENTPHYMODE_ATTRIB                 MIBOID_ATTRIB_READ

enum
{
	WIFI_PHY_MODE_A = 0,	// 802.11a
	WIFI_PHY_MODE_B = 1,	// 802.11b only
	WIFI_PHY_MODE_G = 2,	// 802.11g only 
	WIFI_PHY_MODE_BG = 3,	// 802.11b/g 
	WIFI_PHY_MODE_INVALID
};

// ============================================================================

/// @brief WMM QOS selection	 
/// 
///			
#define MIBOID_ENABLEQOS                            0x0077
#define MIBOID_ENABLEQOS_TYPE                       MIBOID_TYPE_UINT32
#define MIBOID_ENABLEQOS_ATTRIB                     (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_ADOPT | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_ENABLEQOS_MIN                        FALSE	
#define MIBOID_ENABLEQOS_MAX                        TRUE

// ============================================================================

/// @brief Keep alive time (second).	 
/// 
///			
#define MIBOID_KEEPALIVETIME                        0x0078
#define MIBOID_KEEPALIVETIME_TYPE                   MIBOID_TYPE_UINT32
#define MIBOID_KEEPALIVETIME_ATTRIB                 (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_KEEPALIVETIME_MIN                    10	
#define MIBOID_KEEPALIVETIME_MAX                    100
#define MIBOID_KEEPALIVETIME_DEFAULT                30
// ============================================================================

/// @brief Number of missed beacons before bss timeout.	 
/// 
///			
#define MIBOID_BSSTIMEOUT_BEACONS                   0x0079
#define MIBOID_BSSTIMEOUT_BEACONS_TYPE              MIBOID_TYPE_UINT32
#define MIBOID_BSSTIMEOUT_BEACONS_ATTRIB            (MIBOID_ATTRIB_READ | \
                                                    MIBOID_ATTRIB_WRITE | \
                                                    MIBOID_ATTRIB_MINMAX)

#define MIBOID_BSSTIMEOUT_BEACONS_MIN               1	
#define MIBOID_BSSTIMEOUT_BEACONS_MAX               200
#define MIBOID_BSSTIMEOUT_BEACONS_DEFAULT           80

// ============================================================================

/// @brief Returns the instantaneous PHY rate that the driver is using for
///        trasmitted packets.
///
///
#define MIBOID_CURRENTPHYRATE                        0x007b
#define MIBOID_CURRENTPHYRATE_TYPE                   MIBOID_TYPE_UINT32
#define MIBOID_CURRENTPHYRATE_ATTRIB                 MIBOID_ATTRIB_READ



// ============================================================================

/// @brief Returns the instantaneous PHY rate that the driver is using for
///        trasmitted packets.
///
///
#define MIBOID_FORCE_CTS_TO_SELF                    0x007c
#define MIBOID_FORCE_CTS_TO_SELF_TYPE          MIBOID_TYPE_UINT32
#define MIBOID_FORCE_CTS_TO_SELF_ATTRIB        (MIBOID_ATTRIB_READ | \
                                                MIBOID_ATTRIB_WRITE | \
                                                 MIBOID_ATTRIB_MINMAX)

#define MIBOID_FORCE_CTS_TO_SELF_MIN           0
#define MIBOID_FORCE_CTS_TO_SELF_MAX           1
#define MIBOID_FORCE_CTS_TO_SELF_DEFAULT       0

// ============================================================================

/// @brief Returns the most recent reason for disconnecting
#define MIBOID_DISCONNECT_REASON                    0x007d
#define MIBOID_DISCONNECT_REASON_TYPE			MIBOID_TYPE_UINT32
#define MIBOID_DISCONNECT_REASON_ATTRIB		(MIBOID_ATTRIB_READ)

enum {
	WIFI_DISCONNECT_REASON_UNSPECIFIED = 0,
	WIFI_DISCONNECT_REASON_BSS_TIMEOUT = 1,
	WIFI_DISCONNECT_REASON_BSS_PARAM_CHANGED = 2,
	WIFI_DISCONNECT_REASON_DEAUTH_BY_AP = 3,
	WIFI_DISCONNECT_REASON_DEAUTH_BY_STA = 4,
};

// ============================================================================

/// @brief UP remap Configuration 
#define MIBOID_UPREMAPPING                              0x007e
#define MIBOID_UPREMAPPING_TYPE                         MIBOID_TYPE_UINT32
#define MIBOID_UPREMAPPING_ATTRIB                       (MIBOID_ATTRIB_READ | \
                                                        MIBOID_ATTRIB_WRITE)

#define WIFI_UPREMAPPING_0_MASK                         0x0000000F
#define WIFI_UPREMAPPING_1_MASK                         0x000000F0
#define WIFI_UPREMAPPING_2_MASK                         0x00000F00
#define WIFI_UPREMAPPING_3_MASK                         0x0000F000
#define WIFI_UPREMAPPING_4_MASK                         0x000F0000
#define WIFI_UPREMAPPING_5_MASK                         0x00F00000
#define WIFI_UPREMAPPING_6_MASK                         0x0F000000
#define WIFI_UPREMAPPING_7_MASK                         0xF0000000

#define MIBOID_UPREMAPPING_DEFAULT                      0x76543210

// ============================================================================
/// @brief enable software based auto antenna diversity switching. 
///
/// Enabling this MIBOID will override the antenna selected by MIBOID_DIVERSITYSUPPORT
#define MIBOID_DYNAMIC_DIVERSITY					0x007f
#define MIBOID_DYNAMIC_DIVERSITY_TYPE				MIBOID_TYPE_UINT32
#define MIBOID_DYNAMIC_DIVERSITY_ATTRIB			(MIBOID_ATTRIB_READ | \
												 MIBOID_ATTRIB_WRITE | \
												 MIBOID_ATTRIB_MINMAX)

enum {
	WIFI_DYNAMIC_DIVERSITY_OFF = 0,
	WIFI_DYNAMIC_DIVERSITY_ON = 1
};

#define MIBOID_DYNAMIC_DIVERSITY_MIN WIFI_DYNAMIC_DIVERSITY_OFF
#define MIBOID_DYNAMIC_DIVERSITY_MAX WIFI_DYNAMIC_DIVERSITY_ON
#define MIBOID_DYNAMIC_DIVERSITY_DEFAULT WIFI_DYNAMIC_DIVERSITY_OFF

// ============================================================================

/// @brief select the prefered antenna to use for auto diversity switching
#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT			0x0080
#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT_ATTRIB	(MIBOID_ATTRIB_READ | \
													 MIBOID_ATTRIB_WRITE | \
													 MIBOID_ATTRIB_MINMAX)

enum {
	WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_ANY = 0,
	WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_A = 1,
	WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_B = 2
};

#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT_MIN WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_ANY
#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT_MAX WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_B
#define MIBOID_DYNAMIC_DIVERSITY_PREFERRED_ANT_DEFAULT WIFI_DYNAMIC_DIVERSITY_PREFERRED_ANT_ANY

// ============================================================================

/// @brief When the AP's signal level drops below this threshold, the driver
/// will disable auto-diversity and enable MRC.
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD				0x0081
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD_ATTRIB		(MIBOID_ATTRIB_READ | \
													 MIBOID_ATTRIB_WRITE | \
													 MIBOID_ATTRIB_MINMAX)
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD_MIN 0
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD_MAX 100
#define MIBOID_DYNAMIC_DIVERSITY_THRESHOLD_DEFAULT 80

// ============================================================================

/// @brief When throughput (TX or RX) exceeds this value the station will use
/// the preferred antenna
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD				0x0082
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD_TYPE		MIBOID_TYPE_UINT32
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD_ATTRIB		(MIBOID_ATTRIB_READ | \
																	 MIBOID_ATTRIB_WRITE | \
																	 MIBOID_ATTRIB_MINMAX)
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD_MIN 0
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD_MAX 0xffffffff
#define MIBOID_DYNAMIC_DIVERSITY_ACTIVE_CALL_THRESHOLD_DEFAULT 10240

/// @brief This object provides the driver version information
/// 
#define MIBOID_DRIVER_VERSION						0X0083
#define MIBOID_DRIVER_VERSION_TYPE					MIBOID_TYPE_DRIVER_VERSION
#define MIBOID_DRIVER_VERSION_ATTRIB				(MIBOID_ATTRIB_READ)

// ============================================================================
// when new MIB objects are added, they take the value defined by
// MIBOID_NEXT_AVAILABLE_ID, and MIBOID_NEXT_AVAILABLE_ID is incremented by 1.
#define MIBOID_NEXT_AVAILABLE_ID						0x0084

typedef struct TxWifiPacketStruct
{
	/// When sending multiple packets, this pointer points to the next packet
	/// in a chain of packets.
	struct TxWifiPacketStruct* nextDesc;

	/// the length of the packet (which does not include FCS), specified in
	/// bytes.
	uint16 length;

	/// points to the 802.11 packet.
	uint8* packet;

	uint32 flags;

	/// WIFI_PACKET_API_80211_TXFLAGS_USE_TSF_TIMESTAMP - Set this bit to
	/// include a TSF timestamp in the packet.  The HW will place the TSF
	/// timestamp at the offset specified in the tsfTimestampOffset field.
#define WIFI_PACKET_API_80211_TXFLAGS_USE_TSF_TIMESTAMP		0x00000001
#define WIFI_PACKET_API_80211_TXFLAGS_USE_AES_ENCRYPTION	0x00000004
#define WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT		0x00000008
#define WIFI_PACKET_API_80211_TXFLAGS_USE_LONG_PREAMBLE		0x00000010	
#define WIFI_PACKET_API_80211_TXFLAGS_USE_MCS_INDEX			0x00000020

#define WIFI_PACKET_API_80211_TXFLAGS_USE_FRAG1_FOR_WLAN	0x00000040
#define WIFI_PACKET_API_80211_TXFLAGS_USE_TX_PAD			0x00000080
#define WIFI_PACKET_API_80211_TXFLAGS_FALLBACK_EVERY_RETRY			0x00000100
#define WIFI_PACKET_API_80211_TXFLAGS_FALLBACK_EVERY_OTHER_RETRY	0x00000200

	/// Specifies the offset (in bytes) of where the HW places the TSF
	/// timestamp in the packet.  The offset starts at the beginning of the
	/// wifi packet, e.g. if you wanted to place the offset just after the qos
	/// control field in a Qos Data Packet, set this field to 26.  The offset
	/// must be greater than zero, but less than 1492.
	uint16 tsfTimestampOffset;

    /// number of retries attempted.
	uint8  retries;

	/// Specifies the phy rate for the packet or MCS index.
	///
	/// If WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT is set, phyDataRate
	/// is interpreated as an MCS index.
	///
	/// If fallback is enabled, the packet will go out at phyDataRate, but may
	/// fallback to as low as minPhyDataRate.  Valid values to be used for this
	/// field are:
	///
	/// 802.11a: 6, 9, 12, 18, 24, 36, 48, 54.
	/// 802.11g: 1, 2, 55, 6, 9, 11, 12, 18, 24, 36, 48, 54.
	/// 802.11b: 1, 2, 55, 11
	/// 802.11n (with _MCS_INDEX flag set): 0, 1, 2, 3, 4, 5, 6, 7
	uint8  phyDataRate;

	/// If WIFI_PACKET_API_80211_TXFLAGS_USE_PWRLVL_DIRECT is set, the PHY
	/// Header's TXPWRLVL field will be programmed with powerLevel. Otherwise
	/// TXPWRLVL is set from MIBOID_POWERTABLE indexed by the channel and
	/// phyRate.
	uint8  powerLevel;	

	// deprecated field
	uint32 wifiQosAPIHandle;

	/// [OUT] If none of these bits are set, the packet was transmitted
	/// successfully.  Otherwise, one or more of the following bit definitions
	/// will be set: 
	uint32 txStatusFlags;

/// if any of the fields in the structure are set incorrectly, this bit will be
/// set.
#define WIFI_PACKET_API_80211_TXSTATUSFLAGS_INCORRECT_SETTINGS 0x00000002
#define WIFI_PACKET_API_80211_TXSTATUSFLAGS_OUT_OF_PACKETS     0x00000008
} 
TX_WIFI_PACKET_DESC;

#define WIFI_RXPACKETSIZE	4096
typedef struct RxWifiPacketStruct
{
	/// the length of the packet (which does not include FCS), specified in
	/// bytes.
    uint16	length;

	/// (see MIBOID_AVERAGERSSI for RSSI-to-dBm formula)
	uint8	rssiChannelA;
	uint8	rssiChannelB;

	/// This field contains the lower 32 bits of the TSF timer at the time when
	/// the packet was received. This is a microsecond timer. 
    uint32	tsfTimestamp;

    uint32	flags;
#define WIFI_PACKET_API_80211_RXFLAGS_FCS_ERROR         0x00000010 
#define WIFI_PACKET_API_80211_RXFLAGS_LENGTH_EOF_ERROR  0x00000020 
#define WIFI_PACKET_API_80211_RXFLAGS_PROTOCOL_ERROR    0x00000040 
#define WIFI_PACKET_API_80211_RXFLAGS_DECRYPT_ERROR     0x00000100
#define WIFI_PACKET_API_80211_RXFLAGS_LONG_PREAMBLE		0x00000200
#define WIFI_PACKET_API_80211_RXFLAGS_MCS_INDEX			0x00000400
	
	/// Valid values (without MCS index flag): 1, 2, 55, 6, 9, 11, 12, 18, 24,
	/// 36, 48, 54.
	///
	/// When WIFI_PACKET_API_80211_RXFLAGS_MCS_INDEX is set valid values are
	/// 0,1,2,3,4,5,6,7
    uint8	phyDataRate;

	/// channel number on which this packet was received.  
	/// Valid channel numbers are: 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 34, 36,
	/// 38, 40, 42, 44, 46, 48, 52 56, 60, 64.
    uint8	channel;

	uint8	reserved[2];

	uint8	packet[WIFI_RXPACKETSIZE];
} 
RX_WIFI_PACKET_DESC;


/// @brief Transmit a chain of packets. 
///
/// The driver will attempt to transmit each packet in the chain. If the driver
/// cannot transmit the "Nth" packet it will continue onto the "Nth + 1" packet
/// until no more packets are left in the chain.
///
/// The user can submit a batch of packets for multiple AC's and allow the
/// driver to queue up the maximum number of allowable packets per AC.
///
/// If the wifiQosAPIHandle is specified the driver will queue the packet onto
/// the corresponding HCCA stream queue.
///
/// Else If the packet is a QoS-Data frame, the driver will queue the packet
/// onto the corresponding EDCA queue specified by the TID in the QoS Control
/// field.
///
/// Otherwise the packet is queued as a DCF packet on the AC0 queue.
/// 
/// @param[in] device identify wireless ethernet device
/// @param[in] wifiPacket chain of 1 or more packets to transmit
/// @return dwWifiStatus_t
/// @retval WIFI_SUCCESS
/// @retval WIFI_FAIL
/// @retval WIFI_DEVICE_NOT_FOUND
/// @retval WIFI_INVALID_PARAMS
/// @retval WIFI_80211_OUT_OF_TX_BUFFERS
WIFIAPI uint32 Wifi80211APISendPkt(void* device, TX_WIFI_PACKET_DESC* wifiPacket);

/// @brief Poll the driver to retrive aupdate to 'numRxDesc' packets. These
/// packets will be copied into the rxDesc array and numRxDesc will be updated
/// to indicate the number of successfully retrieved packets.
/// 
/// @param[in] device identify wireless ethernet device
/// @param[io] rxDesc array of RX_WIFI_PACKET_DESC with attached buffers.
/// @param[io] numRxDesc specifices size of rxDesc array on function entry.
///
/// @return dwWifiStatus_t
/// @retval WIFI_SUCCESS
/// @retval WIFI_FAIL
/// @retval WIFI_DEVICE_NOT_FOUND
/// @retval WIFI_INVALID_PARAMS
WIFIAPI uint32 Wifi80211APIReceivePkt(void* device, RX_WIFI_PACKET_DESC rxDesc[], uint8* numRxDesc );


/// WAM COMMAND 
enum 
{
	WIFI_CMD_MIB	= 0x80,	
	WIFI_CMD_QOS,	
	WIFI_CMD_80211_TX,
	WIFI_CMD_80211_RX
};

// This value is incremented whenever updates to the WifiAPI break backwards
// compatibility.
#define WIFIAPIVERSION	0x0001

/// WAM HEADER (version2)
typedef struct wamHdr2_s
{
	/// WifiAPI version
	uint16	version;

	/// WAM command 
	uint8	cmd;	

	/// Number of API objects. This must be 1 for QOS and 802.11 API calls. 
	uint8	num;	

	/// Status of the marshalled WifiAPI fuction call.
	uint16	status;

	uint16	reserved;

	/// Pointer to the API object (Array of API object when used with
	/// WIFI_CMD_MIB only!)
	void*	data;
}
wamHdr2_t;

/// Event types
enum
{
	/// Station Associated
	WIFI_EVENT_STAASSOC	= 0,

	/// Station Disassociated (see reason for 802.11 reason code)
	WIFI_EVENT_STADISASSOC,

	/// Station Scan Completed
	WIFI_EVENT_STASCANCOMPLETE,

	/// The channel changed (see channel for the current channel)
	WIFI_EVENT_CHANNELUPDATE,
	
	WIFI_EVENT_TS_ACTIVE,
	WIFI_EVENT_TS_INACTIVE,

	WIFI_EVENT_BYPASS_TX_BATCH_DONE,
};

/// Event structure for the "Wi-Fi Event Queue"
typedef struct wifiEvent_s
{
	uint16	eventID;
	uint16	reserved;

	union {
		uint32 reason;
		uint32 channel;
		struct {
			uint8 addr[6];
			uint8 tsid;
			uint8 direction;
		} ts;
	};
}
wifiEvent_t;

// TX Power Level Control
#define POWERTABLE_MAX_FREQ_NUM         48
#define POWERTABLE_MAX_RATE_NUM         23

typedef struct powerTableStruct
{
    uint8 pt[POWERTABLE_MAX_FREQ_NUM][POWERTABLE_MAX_RATE_NUM];
} 
dwMibPowerTable_t;
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
//       15		MCS index 1
//       16		MCS index 2
//       17		MCS index 3
//       18		MCS index 4
//       19		MCS index 5
//       20		MCS index 6
//       21		MCS index 7
//       22		MCS index 8
//

typedef struct dwMibPhyParameter_s
{
	uint32	paramType;		/// one of the WIFI_PHYPARAM_* values
	uint32	dataSize;		/// data buffer size, bytes
	uint8	*data;			/// pointer to data buffer
}
dwMibPhyParameter_t;

typedef struct dwMibHwModes_s
{
	uint32	numModes;

	struct
	{
		uint32	phyMode;
		uint32	numChan;
		struct {
			uint16 channel;
			uint16 frequency;
		} chanFreq[WIFI_HWMODES_MAXCHANNELS];
		uint32	numRate;
		uint16	rate[MAX_RATES];	// KHz
	}
	mode[WIFI_HWMODES_MAXMODES];
}
dwMibHwModes_t;

enum
{
#ifdef SIOCIWFIRSTPRIV
	DSPG_PRIV_SET_IE = SIOCIWFIRSTPRIV,
#else
	DSPG_PRIV_SET_IE,
#endif
	DSPG_PRIV_GET_RESERVED,
	DSPG_PRIV_GET_STA,
	DSPG_PRIV_SET_STA,
	DSPG_PRIV_SET_PARAM,
	DSPG_PRIV_GET_PARAM,
};

enum
{
	DSPG_PRIV_SET_IE_BEACON,
	DSPG_PRIV_SET_IE_PROBE_RES,
};

enum
{
	DSPG_PRIV_STA_DEAUTH = 1,
	DSPG_PRIV_STA_DISASSOC,
	DSPG_PRIV_STA_REMOVE,
	DSPG_PRIV_STA_FLAGS,
	DSPG_PRIV_STA_SEQNUM,
};

#endif // _WIFI_API_H

// vim: set comments=b\:///,\:// :

#ifndef ANDROID
#include "WmUtils.h"
#include "VwalWrapper/WmVwalWrapper.h"
#include "Vwal/include/WifiApi.h"
#include "Interfaces/NonCom/SysSettings/ISysSettings.h"
#include "infra/include/InfraMacros.h"
#include "infra/include/InfraLog.h"
#include "infra/include/InfraUtils.h"
#else
#include "WmUtils.h"
#include "VwalWrapper/WmVwalWrapper.h"
#include "xpndr-syssettings/include/ISysSettings.h"
#include "cutils/log.h"

#define LOG_TAG "WmUtils"
#endif

#define TABLE_FILENAME "/etc/wifimgr/wmconfig.sqlite"
//////////////////////////////////////////////////////////////////////////////
// WmUtils supports utils eg. SysSetting access.
// 
// 

uint32 WmUtils::GetStrParam( const char* Key, const char* Group, char** Val )
{
	ISysSettings* pSettings = SYSSETTINGS_Init( TABLE_FILENAME );	


	uint32 sys_res = pSettings->SysSettingsGetStringValue(Key,
														  Group,
														  (const char**)Val);

	SYSSETTINGS_Terminate();

	return sys_res;
}

uint32 WmUtils::SetStrParam(const  char* Key,const  char* Group, const char* Val )
{
	ISysSettings* pSettings = SYSSETTINGS_Init( TABLE_FILENAME );	

	uint32 sys_res =  pSettings->SysSettingsSetStringValue(Key,
														   Group,
														   Val);
	SYSSETTINGS_Terminate();

	return sys_res;
}

uint32 WmUtils::SetIntParam( const char* Key, const char* Group, sint32 Val )
{
	ISysSettings* pSettings = SYSSETTINGS_Init( TABLE_FILENAME );	


	uint32 sys_res = pSettings->SysSettingsSetIntegerValue(Key,
														   Group,
														   Val);

	SYSSETTINGS_Terminate();


	return sys_res;
}

uint32 WmUtils::GetIntParam( const char* Key,const char* Group, sint32& Val )
{
	ISysSettings* pSettings = SYSSETTINGS_Init( TABLE_FILENAME );	


	uint32 sys_res = pSettings->SysSettingsGetIntegerValue(Key,
														   Group,
														   Val);

	SYSSETTINGS_Terminate();

	return sys_res;
}

void WmUtils::InitIntParamFromWmDb(const char* ParamKey, sint32& Value)
{
    
	sint32 tempVal;

	uint32 Status = GetIntParam( ParamKey, "WifiMgr", tempVal );

	if( Status == RC_OK ) 
	{
		//Found previous value of country code in DB. Set m_CountryCode
		//to that value.
		Value = tempVal;
	}
	else 
	{
		//set the defualt val should have been sent with Value.
		StoreIntParamInWmDb(ParamKey, Value);
	}


}

void WmUtils::StoreIntParamInWmDb(const char* ParamKey, sint32 Value)
{
	WmUtils::SetIntParam( ParamKey, "WifiMgr", Value );

}
#ifndef ANDROID
////////////////////////////////////////////////////////////////////////////////
/// @brief PrintScanList
///        Debug function prints information stored in m_DummyDb
void WmUtils::PrintScanList(OUT WmScanListInfo* pScanListInfoArray, 
							 IN  uint8           Size              )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmUtils::PrintScanList ---->" ) );

	if(!pScanListInfoArray) {
		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
					 "CWmUtils::PrintScanList NULL Pointer" ) );
		return;
	}

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "===================  Scan List (%d found)===================",(int)Size ) );
	WmScanListInfo* pListInfo;

	for(int i=0;i<Size;i++) {

		pListInfo = &(pScanListInfoArray[i]);

		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "Ssid= %s", 
					  pListInfo->Ssid.Ssid) );
	
		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "Rssi=%d", 
					  pListInfo->Rssi  ) );
	
		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "IsConfigured=%d", 
					  pListInfo->PrevConfigured  ) );
	
			
		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, 
					  "Bssid= %X:%X:%X:%X:%X:%X", 
				   
				  pListInfo->Bssid.Address[0],
				  pListInfo->Bssid.Address[1],
				  pListInfo->Bssid.Address[2],
				  pListInfo->Bssid.Address[3],
				  pListInfo->Bssid.Address[4],
				  pListInfo->Bssid.Address[5]  ) );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_OPEN)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_OPEN") );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_WEP)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_WEP") );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_WPA_PSK)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_WPA_PSK") );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_WPA_EAP)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_WPA_EAP") );


		if( (pListInfo->AuthType) & WM_AUTHTYPE_WPA2_PSK)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_WPA2_PSK") );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_WPA2_EAP)
				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_WPA2_EAP") );

		if( (pListInfo->AuthType) & WM_AUTHTYPE_IEEE8021X)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "AuthType=WM_AUTHTYPE_IEE80211X") );


		if( (pListInfo->Capabilities ) & WM_CAPABILITIES_PREAUTHENTICATION)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "Capabilities=WM_CAPABILITIES_PREAUTHENTICATION") );

		if( (pListInfo->Capabilities ) & WM_CAPABILITIES_WPS)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "Capabilities=WM_CAPABILITIES_WPS") );

		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "SupportedCiphers=%d",pListInfo->SupportedCiphers) );

		if(pListInfo->PsMode == WM_PS_MODE_LEGACY)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "PsMode=WM_PS_LEGACY") );

		if(pListInfo->PsMode == WM_PS_MODE_UAPSD)
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "PsMode=WM_PS_UAPSD") );

		if(pListInfo->Country != WM_COUNTRY_NOT_SUPPORTED) {
			char country[COUNTRY_LEN];
			CWmCountryUtils::EWmCountryToStr(pListInfo->Country,country);
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE, "CountryString= %s", 
						  country) );
		}

		INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "++++++++++++++++++++++++++++" ) );

	}

    INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "====================================================" ) );
	
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				 "CWmUtils::PrintScanList <----" ) );

}
#endif



/* IEEE 802.11i */
#define PMKID_LEN 16
#define PMK_LEN 32

#define WPA_SELECTOR_LEN 4
#define WPA_VERSION 1
#define RSN_SELECTOR_LEN 4
#define RSN_VERSION 1

#define WPA_IE_HEADER_LENGTH 8
#define RSN_IE_HEADER_LENGTH 4

#define RSN_SELECTOR(a, b, c, d) \
	((((uint32) (a)) << 24) | (((uint32) (b)) << 16) | (((uint32) (c)) << 8) | \
	 (uint32) (d))

#define WPA_AUTH_KEY_MGMT_NONE RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define WPA_AUTH_KEY_MGMT_UNSPEC_802_1X RSN_SELECTOR(0x00, 0x50, 0xf2, 1)
#define WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define WPA_CIPHER_SUITE_NONE RSN_SELECTOR(0x00, 0x50, 0xf2, 0)
#define WPA_CIPHER_SUITE_WEP40 RSN_SELECTOR(0x00, 0x50, 0xf2, 1)
#define WPA_CIPHER_SUITE_TKIP RSN_SELECTOR(0x00, 0x50, 0xf2, 2)
#define WPA_CIPHER_SUITE_CCMP RSN_SELECTOR(0x00, 0x50, 0xf2, 4)
#define WPA_CIPHER_SUITE_WEP104 RSN_SELECTOR(0x00, 0x50, 0xf2, 5)


#define RSN_AUTH_KEY_MGMT_UNSPEC_802_1X RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define RSN_AUTH_KEY_MGMT_FT_802_1X RSN_SELECTOR(0x00, 0x0f, 0xac, 3)
#define RSN_AUTH_KEY_MGMT_FT_PSK RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_AUTH_KEY_MGMT_802_1X_SHA256 RSN_SELECTOR(0x00, 0x0f, 0xac, 5)
#define RSN_AUTH_KEY_MGMT_PSK_SHA256 RSN_SELECTOR(0x00, 0x0f, 0xac, 6)

#define RSN_CIPHER_SUITE_NONE RSN_SELECTOR(0x00, 0x0f, 0xac, 0)
#define RSN_CIPHER_SUITE_WEP40 RSN_SELECTOR(0x00, 0x0f, 0xac, 1)
#define RSN_CIPHER_SUITE_TKIP RSN_SELECTOR(0x00, 0x0f, 0xac, 2)
#define RSN_CIPHER_SUITE_CCMP RSN_SELECTOR(0x00, 0x0f, 0xac, 4)
#define RSN_CIPHER_SUITE_WEP104 RSN_SELECTOR(0x00, 0x0f, 0xac, 5)

#define WPA_OUI_TYPE RSN_SELECTOR(0x00, 0x50, 0xf2, 1)

#define RSN_SELECTOR_PUT(a, val) WM_BUF_PUT_BE32((uint8 *) (a), (val))
#define RSN_SELECTOR_GET(a) WM_BUF_GET_BE32((const uint8 *) (a))

struct WmWpaIEData
{
	int proto;
	int pairwise_cipher;
	int group_cipher;
	int key_mgmt;
	int capabilities;
//RR	size_t num_pmkid;
//RR	const uint8 *pmkid;
//RR	int mgmt_group_cipher;
};

enum EWpaCipher
{
	WPA_CIPHER_NONE				= WM_BIT(0),
	WPA_CIPHER_WEP40			= WM_BIT(1),
	WPA_CIPHER_WEP104			= WM_BIT(2),
	WPA_CIPHER_TKIP				= WM_BIT(3),
	WPA_CIPHER_CCMP				= WM_BIT(4),

};

enum EWpaKeyMgmt
{
	WPA_KEY_MGMT_IEEE8021X			= WM_BIT(0),
	WPA_KEY_MGMT_PSK				= WM_BIT(1),
	WPA_KEY_MGMT_NONE				= WM_BIT(2),
	WPA_KEY_MGMT_IEEE8021X_NO_WPA	= WM_BIT(3),
	WPA_KEY_MGMT_WPA_NONE			= WM_BIT(4),
	WPA_KEY_MGMT_FT_IEEE8021X		= WM_BIT(5),
	WPA_KEY_MGMT_FT_PSK				= WM_BIT(6),

};

enum EWpaProto
{
	WPA_PROTO_WPA			= WM_BIT(0),
	WPA_PROTO_RSN			= WM_BIT(1),

};


enum EIECodes
{
	IE_SSID    = 0  ,
    IE_COUNTRY = 7  ,
	IE_RSN     = 48 ,
	IE_VENDOR  = 221	// WPA uses this IE ID 
};
#ifndef ANDROID

class WmWpaParser {

	static int RsnSelectorToBitfield(const uint8 *s)
	{
		if (RSN_SELECTOR_GET(s) == RSN_CIPHER_SUITE_NONE)
			return WPA_CIPHER_NONE;
		if (RSN_SELECTOR_GET(s) == RSN_CIPHER_SUITE_WEP40)
			return WPA_CIPHER_WEP40;
		if (RSN_SELECTOR_GET(s) == RSN_CIPHER_SUITE_TKIP)
			return WPA_CIPHER_TKIP;
		if (RSN_SELECTOR_GET(s) == RSN_CIPHER_SUITE_CCMP)
			return WPA_CIPHER_CCMP;
		if (RSN_SELECTOR_GET(s) == RSN_CIPHER_SUITE_WEP104)
			return WPA_CIPHER_WEP104;
		return 0;
	}
	
	
	static int RsnKeyMgmtToBitfield(const uint8 *s)
	{
		if (RSN_SELECTOR_GET(s) == RSN_AUTH_KEY_MGMT_UNSPEC_802_1X)
			return WPA_KEY_MGMT_IEEE8021X;
		if (RSN_SELECTOR_GET(s) == RSN_AUTH_KEY_MGMT_PSK_OVER_802_1X)
			return WPA_KEY_MGMT_PSK;
		if (RSN_SELECTOR_GET(s) == RSN_AUTH_KEY_MGMT_FT_802_1X)
			return WPA_KEY_MGMT_FT_IEEE8021X;
		if (RSN_SELECTOR_GET(s) == RSN_AUTH_KEY_MGMT_FT_PSK)
			return WPA_KEY_MGMT_FT_PSK;
		return 0;
	}
	
	static int WpaSelectorToBitfield(const uint8 *s)
	{
		if (RSN_SELECTOR_GET(s) == WPA_CIPHER_SUITE_NONE)
			return WPA_CIPHER_NONE;
		if (RSN_SELECTOR_GET(s) == WPA_CIPHER_SUITE_WEP40)
			return WPA_CIPHER_WEP40;
		if (RSN_SELECTOR_GET(s) == WPA_CIPHER_SUITE_TKIP)
			return WPA_CIPHER_TKIP;
		if (RSN_SELECTOR_GET(s) == WPA_CIPHER_SUITE_CCMP)
			return WPA_CIPHER_CCMP;
		if (RSN_SELECTOR_GET(s) == WPA_CIPHER_SUITE_WEP104)
			return WPA_CIPHER_WEP104;
		return 0;
	}
	
	
	static int WpaKeyMgmtToBitfield(const uint8 *s)
	{
		if (RSN_SELECTOR_GET(s) == WPA_AUTH_KEY_MGMT_UNSPEC_802_1X)
			return WPA_KEY_MGMT_IEEE8021X;
		if (RSN_SELECTOR_GET(s) == WPA_AUTH_KEY_MGMT_PSK_OVER_802_1X)
			return WPA_KEY_MGMT_PSK;
		if (RSN_SELECTOR_GET(s) == WPA_AUTH_KEY_MGMT_NONE)
			return WPA_KEY_MGMT_WPA_NONE;
		return 0;
	}
	
public:
	
	static int ParseWpaIE(const uint8 *wpa_ie, 
						size_t wpa_ie_len,
						WmWpaIEData& data)
	{
		const uint8 *pos;
		int left;
		int i, count;
	
		data.proto = WPA_PROTO_WPA;
		data.pairwise_cipher = WPA_CIPHER_TKIP;
		data.group_cipher = WPA_CIPHER_TKIP;
		data.key_mgmt = WPA_KEY_MGMT_IEEE8021X;
		data.capabilities = 0;
	//RR	data.pmkid = NULL;
	//RR	data.num_pmkid = 0;
	//RR	data.mgmt_group_cipher = 0;
	
	
		if (wpa_ie_len < WPA_IE_HEADER_LENGTH) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						  " ie len too short %lu",(unsigned long) wpa_ie_len) );

			return -1;
		}
	
	
		if (wpa_ie[0] != IE_VENDOR ||
			wpa_ie[1] != wpa_ie_len   ||
			RSN_SELECTOR_GET(&(wpa_ie[2])) != WPA_OUI_TYPE ||
			WM_BUF_GET_LE16(&(wpa_ie[6])) != WPA_VERSION) {

			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						   " malformed ie or unknown version") );

			return -1;
		}
	
		pos = (const uint8*)&(wpa_ie[WPA_IE_HEADER_LENGTH]);
		left = wpa_ie_len - WPA_IE_HEADER_LENGTH + 2;
	
	
		if (left >= WPA_SELECTOR_LEN) {
			data.group_cipher = WpaSelectorToBitfield(pos);
			pos += WPA_SELECTOR_LEN;
			left -= WPA_SELECTOR_LEN;
		} else if (left > 0) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						  " ie length mismatch, %u too much",left) );

			return -1;
		}
	
		if (left >= 2) {
			data.pairwise_cipher = 0;
			count = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
			if (count == 0 || left < count * WPA_SELECTOR_LEN) {
				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "WmWpaParser::ParseWpaIE"
							  " ie count botch (pairwise),"
							  "count %u left %u",count,left) );

				return -1;
			}
			for (i = 0; i < count; i++) {
				data.pairwise_cipher |= WpaSelectorToBitfield(pos);
				pos += WPA_SELECTOR_LEN;
				left -= WPA_SELECTOR_LEN;
			}
		} else if (left == 1) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						  " ie too short (for key mgmt)") );
			return -1;
		}
	
		if (left >= 2) {
			data.key_mgmt = 0;
			count = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
			if (count == 0 || left < count * WPA_SELECTOR_LEN) {
				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "WmWpaParser::ParseWpaIE"
							  " ie count botch (key_mgmt),"
							  "count %u left %u",count,left) );

				return -1;
			}
			for (i = 0; i < count; i++) {
				data.key_mgmt |= WpaKeyMgmtToBitfield(pos);
				pos += WPA_SELECTOR_LEN;
				left -= WPA_SELECTOR_LEN;
			}
		} else if (left == 1) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						  " ie too short (for capabilities)") );
			return -1;
		}
	
		if (left >= 2) {
			data.capabilities = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
		}
	
		if (left > 0) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseWpaIE"
						  " ie has %u trailing bytes - ignored",left) );
		}
	
		return 0;
	}
	
	
	
	
	static int ParseRsnIE(const uint8 *rsn_ie, 
						  size_t rsn_ie_len,
						  WmWpaIEData& data)
	{
		const uint8 *pos;
		int left;
		int i, count;
	
		data.proto = WPA_PROTO_RSN;
		data.pairwise_cipher = WPA_CIPHER_CCMP;
		data.group_cipher = WPA_CIPHER_CCMP;
		data.key_mgmt = WPA_KEY_MGMT_IEEE8021X;
		data.capabilities = 0;
	//RR	data.pmkid = NULL;
	//RR	data.num_pmkid = 0;
	//RR	data.mgmt_group_cipher = 0;
	
	
	
		if (rsn_ie_len < RSN_IE_HEADER_LENGTH) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  " ie len too short %lu",(unsigned long) rsn_ie_len) );

			return -1;
		}
/*
		for(int ii=0;ii<(int)rsn_ie_len;ii++) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  " rns_ie[%d]=%u\n",ii,(unsigned int)(rsn_ie[ii])) );
		}
*/
	
		if (rsn_ie[0] != IE_RSN ||
			rsn_ie[1] != rsn_ie_len   ||
			WM_BUF_GET_LE16(&(rsn_ie[2])) != RSN_VERSION) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  " malformed ie or unknown version") );
			return -2;
		}
	
	
		pos = (const uint8*)&(rsn_ie[RSN_IE_HEADER_LENGTH]);
		left = rsn_ie_len - RSN_IE_HEADER_LENGTH + 2;
	
		if (left >= RSN_SELECTOR_LEN) {
			data.group_cipher = RsnSelectorToBitfield(pos);
			pos += RSN_SELECTOR_LEN;
			left -= RSN_SELECTOR_LEN;
		} else if (left > 0) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  "  ie length mismatch, %u too much",left) );
			return -3;
		}
	
		if (left >= 2) {
			data.pairwise_cipher = 0;
			count = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
			if (count == 0 || left < count * RSN_SELECTOR_LEN) {

				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "WmWpaParser::ParseRsnIE"
							  "ie count botch (pairwise), "
							  "count %u left %u",count,left) );
				return -4;
			}
			for (i = 0; i < count; i++) {
				data.pairwise_cipher |= RsnSelectorToBitfield(pos);
				pos += RSN_SELECTOR_LEN;
				left -= RSN_SELECTOR_LEN;
			}
		} else if (left == 1) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  " ie too short (for key mgmt)") );
			return -5;
		}
	
		if (left >= 2) {
			data.key_mgmt = 0;
			count = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
			if (count == 0 || left < count * RSN_SELECTOR_LEN) {
				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "WmWpaParser::ParseRsnIE"
							  "ie count botch (key_mgmt), "
							  "count %u left %u",count,left) );
				return -6;
			}
			for (i = 0; i < count; i++) {
				data.key_mgmt |= RsnKeyMgmtToBitfield(pos);
				pos += RSN_SELECTOR_LEN;
				left -= RSN_SELECTOR_LEN;
			}
		} else if (left == 1) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  " ie too short (for capabilities)") );

			return -7;
		}
	
		if (left >= 2) {
			data.capabilities = WM_BUF_GET_LE16(pos);
			pos += 2;
			left -= 2;
		}
	
	//RR	if (left >= 2) {
	//RR		data.num_pmkid = WM_BUF_GET_LE16(pos);
	//RR		pos += 2;
	//RR		left -= 2;
	//RR		if (left < (int) data.num_pmkid * PMKID_LEN) {
	//RR//RR			wpa_printf(MSG_DEBUG, "%s: PMKID underflow "
	//RR//RR				   "(num_pmkid=%lu left=%d)",
	//RR//RR				   __func__, (unsigned long) data->num_pmkid,
	//RR//RR				   left);
	//RR			data.num_pmkid = 0;
	//RR			return -9;
	//RR		} else {
	
	//RR			data.pmkid = pos;
	//RR			pos += data.num_pmkid * PMKID_LEN;
	//RR			left -=data.num_pmkid * PMKID_LEN;
	//RR		}
	//RR	}
	
	
		if (left > 0) {
			INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
						  "WmWpaParser::ParseRsnIE"
						  "  ie has %u trailing bytes - ignored",left) );
		}
	
		return 0;
	
	}

	static uint8 GetCiphersMask(WmWpaIEData& data)
	{
		uint8 ciphersmask = 0;

		if (data.pairwise_cipher & WPA_CIPHER_NONE)
			ciphersmask |= WM_CIPHERS_PAIRWISE_NONE;
		if (data.pairwise_cipher & WPA_CIPHER_TKIP)
			ciphersmask |= WM_CIPHERS_PAIRWISE_TKIP;
		if (data.pairwise_cipher & WPA_CIPHER_CCMP)
			ciphersmask |= WM_CIPHERS_PAIRWISE_CCMP;

		if (data.group_cipher & WPA_CIPHER_WEP40)
			ciphersmask |= WM_CIPHERS_GROUP_WEP40;
		if (data.group_cipher & WPA_CIPHER_TKIP)
			ciphersmask |= WM_CIPHERS_GROUP_TKIP;
		if (data.group_cipher & WPA_CIPHER_CCMP)
			ciphersmask |= WM_CIPHERS_GROUP_CCMP;
		if (data.group_cipher & WPA_CIPHER_WEP104)
			ciphersmask |= WM_CIPHERS_GROUP_WEP104;

		return ciphersmask;

	}

	static uint8 GetAuthType(WmWpaIEData& data)
	{
		uint8 authtypemask = 0;

		if(data.proto == WPA_PROTO_RSN)
		{
			if(data.key_mgmt & WPA_KEY_MGMT_IEEE8021X ) {
				authtypemask |= WM_AUTHTYPE_WPA2_EAP;
			}
			if(data.key_mgmt & WPA_KEY_MGMT_PSK ) {
				authtypemask |= WM_AUTHTYPE_WPA2_PSK;
			}
		}
		else if(data.proto == WPA_PROTO_WPA) {

			if(data.key_mgmt & WPA_KEY_MGMT_IEEE8021X ) {
				authtypemask |= WM_AUTHTYPE_WPA_EAP;
			}
			if(data.key_mgmt & WPA_KEY_MGMT_PSK ) {
				authtypemask |= WM_AUTHTYPE_WPA_PSK;
			}
			if(data.key_mgmt & WPA_KEY_MGMT_WPA_NONE ) {
				authtypemask |= WM_AUTHTYPE_IEEE8021X; //???????
			}
			
		}
		else {
			//error
		}

		return authtypemask;
	}

};




///////////////////////////////////////////////////////////////////////////
//@brief    ParseInformationElement
// 			Parse Information element and save the results in the goven
// 			WmScanListInfo.
// 			IMPORTANT:
// 			Based on 7.3.2.25 WPA-Specification. Parts are taken from
// 			appendix C of the WPA-Specification 


void CWmIEParser::ParseInformationElement(uint8           ie[], 
											uint32          ieSize, 
											 WmScanListInfo& info  )
{

	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmIEParser::ParseInformationElement ---->" ) );

	uint32 ieofs = 0;

	// walk through the list of IE and print the elements that we are
	// interested in.
    info.Country = WM_COUNTRY_NOT_SUPPORTED;
	while ( ieofs < ieSize ) {
		uint8 id  = ie[ieofs];
		uint8 len = ie[ieofs+1];

        switch ( id ) {

            case IE_SSID:
				memset(info.Ssid.Ssid, 0, WM_SSID_LEN); 
				info.Ssid.Length = len;
				memcpy( info.Ssid.Ssid, &ie[ieofs+2] , len );
                break;

			case IE_RSN:
				{
					WmWpaIEData data;
					int retRes = WmWpaParser::ParseRsnIE(&ie[ieofs],len,data);
					if(retRes == 0) {
						info.SupportedCiphers =  WmWpaParser::GetCiphersMask(data);
						info.AuthType = WmWpaParser::GetAuthType(data);

						INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
									  "CWmIEParser::ParseInformationElement"
									  " Capabilities %d",(int)(data.capabilities) ));

						if(data.capabilities & WM_CAPABILITIES_PREAUTHENTICATION) {
							info.Capabilities |= WM_CAPABILITIES_PREAUTHENTICATION;
						}
					}
					else {
						INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
									  "CWmIEParser::ParseInformationElement"
									  " Error while parsing RSN IE" ) );
					}
					//RR info.gwtEncAlgoMask |= WM_WPA2;
				}
				break;

            case IE_COUNTRY:
                char country[COUNTRY_LEN];
                memcpy(country,&ie[ieofs+2] , COUNTRY_LEN );
                info.Country = CWmCountryUtils::StrToEWmCountry(country);
                break;


			// Vendor Specific IE 
			case IE_VENDOR:
				{
					uint8* oui = &ie[ ieofs + 2];
					uint8 oui_type = ie[ ieofs + 5 ];

					// Wi-Fi Alliance OUI == 00:50:F2
					if (	( 0x00 == oui[0] ) && 
							( 0x50 == oui[1] ) && 
							( 0xF2 == oui[2] )	) 
					{

						// WPA Information Element is OUI Type == 1
						if ( 1 == oui_type ) 
						{
							WmWpaIEData data;
							int retRes = WmWpaParser::ParseWpaIE(&ie[ieofs],len,data);
							if(retRes == 0) {
								info.SupportedCiphers =  WmWpaParser::GetCiphersMask(data);
								info.AuthType = WmWpaParser::GetAuthType(data);
							}
							else {
								INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
											  "CWmIEParser::ParseInformationElement"
											  " Error while parsing WPA IE" ) );
							}

							//RR info.EncAlgoMask |= WM_WPA;

						// WMM Information/Parameter Element is OUI Type == 2
						// See WMM Specification 1.1 for more details about 
						// those fields
						}else if ( 2 == oui_type ) {
							INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "CWmIEParser::ParseInformationElement "
							  "WMM Information/Parameter Element found\n" ) );

							uint8 qos_info = ie[ ieofs + 8 ];
							info.PsMode = WM_PS_MODE_LEGACY;
							
							if( (qos_info & WM_UAPSD_BIT_MASK) )
							{
								info.PsMode = WM_PS_MODE_UAPSD;
								INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
											  "CWmIEParser::ParseInformationElement "
											  "info.PsMode=WM_PS_UAPSD") );
							}

						}else if ( 4 == oui_type ) {
							INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
										  "CWmIEParser::ParseInformationElement "
										  "WPS Information/Parameter Element found\n" ) );

							info.Capabilities |= WM_CAPABILITIES_WPS;
							//RR TODO Add pbc and pin code parsing
						}

						else {
							INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "CWmIEParser::ParseInformationElement "
							  "unknown Wi-Fi Alliance information element\n") );
						}
					} 
					else {
						INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
									  "CWmIEParser::ParseInformationElement"
									  " Vendor specific IE w/unknown OUI\n") );
					}
				}
				break;

			default:
				INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
							  "CWmIEParser::ParseInformationElement "
							  "unknown IE id=%d len=%d\n", id, len ) );
				break;
		}

		ieofs += len + 2;
	}
	INFRA_DLOG( ( LM_WIFIMGRLIB, LP_TRACE,
				  "CWmIEParser::ParseInformationElement <----" ) );

}
#endif

EWmCountry CWmCountryUtils::StrToEWmCountry(char* strCountry )
{

    LOGD(
				"CWmCountryUtils::StrToEWmCountry ---->" );

    
    EWmCountry Country;

    if (strncmp( "AR ", strCountry, COUNTRY_LEN) == 0 )
    {
        Country =  WM_COUNTRY_ARGENTINA;        
    }
    else if (strncmp( "AU ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_AUSTRALIA;        
    }
    else if (strncmp( "CN ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_CHINA;        
    }
    else if (strncmp( "FR ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_FRANCE;        
    }
    else if (strncmp( "GR ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_GERMANY;        
    }
    else if (strncmp( "HK ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_HONG_KONG;        
    }
    else if (strncmp( "IN ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_INDIA;        
    }
    else if (strncmp( "IL ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_ISRAEL;        
    }
    else if (strncmp( "IT ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_ITALY;        
    }
    else if (strncmp( "JP ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_JAPAN;        
    }
    else if (strncmp( "TW ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_TAIWAN;        
    }
    else if (strncmp( "GB ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_UNITED_KINGDOM;        
    }
    else if (strncmp( "US ", strCountry, COUNTRY_LEN) == 0 ) 
    {
        Country =  WM_COUNTRY_UNITED_STATES;        
    }
    else 
    {
        Country =  WM_COUNTRY_NOT_SUPPORTED;
    }

    LOGD(
				"CWmCountryUtils::StrToEWmCountry (country=%d)<----", Country );

    return Country;
}

EWmStatus CWmCountryUtils::EWmCountryToStr(EWmCountry Country, char* strCountry)
{

    LOGD(
				"CWmCountryUtils::EWmCountryToStr ---->" );

    EWmStatus result = WM_OK;
    switch(Country)
	{
    case WM_COUNTRY_ARGENTINA:
        strcpy(strCountry,"AR ");
        break;
    case WM_COUNTRY_AUSTRALIA:
        strcpy(strCountry,"AU ");
        break;
    case WM_COUNTRY_CHINA:
        strcpy(strCountry,"CN ");
        break;
    case WM_COUNTRY_FRANCE:
        strcpy(strCountry,"FR ");
        break;
    case WM_COUNTRY_GERMANY:
        strcpy(strCountry,"GR ");
        break;
    case WM_COUNTRY_HONG_KONG:
        strcpy(strCountry,"HK ");
        break;
    case WM_COUNTRY_INDIA:
        strcpy(strCountry,"IN ");
        break;
    case WM_COUNTRY_ISRAEL:
        strcpy(strCountry,"IL ");
        break;
    case WM_COUNTRY_ITALY:
        strcpy(strCountry,"IT ");
        break;
    case WM_COUNTRY_JAPAN:
        strcpy(strCountry,"JP ");
        break;
    case WM_COUNTRY_TAIWAN:
        strcpy(strCountry,"TW ");
        break;
    case WM_COUNTRY_UNITED_KINGDOM:
        strcpy(strCountry,"GB ");
        break;
    case WM_COUNTRY_UNITED_STATES:
        strcpy(strCountry,"US ");
        break;	
     default: result = WM_COUNTRY_ERROR; break;

    }

    LOGD(
				"CWmCountryUtils::EWmCountryToStr <----" );
    return result;
}



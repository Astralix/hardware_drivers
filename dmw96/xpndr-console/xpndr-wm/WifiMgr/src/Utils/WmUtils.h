#ifndef WM_UTILS_H
#define	WM_UTILS_H

#include "TypeDefs.h"

#ifndef ANDROID
#include "WifiMgr/include/WifiMgr.h"
#else
#include "WmTypes.h"
#endif
/* Macros for handling unaligned memory accesses */

#define WM_BUF_GET_BE16(a) ((uint16) (((a)[0] << 8) | (a)[1]))
#define WM_BUF_PUT_BE16(a, val)			\
	do {					\
		(a)[0] = ((uint16) (val)) >> 8;	\
		(a)[1] = ((uint16) (val)) & 0xff;	\
	} while (0)

#define WM_BUF_GET_LE16(a) ((uint16) (((a)[1] << 8) | (a)[0]))
#define WM_BUF_PUT_LE16(a, val)			\
	do {					\
		(a)[1] = ((uint16) (val)) >> 8;	\
		(a)[0] = ((uint16) (val)) & 0xff;	\
	} while (0)

#define WM_BUF_GET_BE24(a) ((((uint32) (a)[0]) << 16) | (((uint32) (a)[1]) << 8) | \
			 ((uint32) (a)[2]))
#define WM_BUF_PUT_BE24(a, val)					\
	do {							\
		(a)[0] = (uint8) ((((uint32) (val)) >> 16) & 0xff);	\
		(a)[1] = (uint8) ((((uint32) (val)) >> 8) & 0xff);	\
		(a)[2] = (uint8) (((uint32) (val)) & 0xff);		\
	} while (0)

#define WM_BUF_GET_BE32(a) ((((uint32) (a)[0]) << 24) | (((uint32) (a)[1]) << 16) | \
			 (((uint32) (a)[2]) << 8) | ((uint32) (a)[3]))
#define WM_BUF_PUT_BE32(a, val)					\
	do {							\
		(a)[0] = (uint8) ((((uint32) (val)) >> 24) & 0xff);	\
		(a)[1] = (uint8) ((((uint32) (val)) >> 16) & 0xff);	\
		(a)[2] = (uint8) ((((uint32) (val)) >> 8) & 0xff);	\
		(a)[3] = (uint8) (((uint32) (val)) & 0xff);		\
	} while (0)

#define WM_BUF_GET_LE32(a) ((((uint32) (a)[3]) << 24) | (((uint32) (a)[2]) << 16) | \
			 (((uint32) (a)[1]) << 8) | ((uint32) (a)[0]))
#define WM_BUF_PUT_LE32(a, val)					\
	do {							\
		(a)[3] = (uint8) ((((uint32) (val)) >> 24) & 0xff);	\
		(a)[2] = (uint8) ((((uint32) (val)) >> 16) & 0xff);	\
		(a)[1] = (uint8) ((((uint32) (val)) >> 8) & 0xff);	\
		(a)[0] = (uint8) (((uint32) (val)) & 0xff);		\
	} while (0)

#define WM_BUF_GET_BE64(a) ((((uint64) (a)[0]) << 56) | (((uint64) (a)[1]) << 48) | \
			 (((uint64) (a)[2]) << 40) | (((uint64) (a)[3]) << 32) | \
			 (((uint64) (a)[4]) << 24) | (((uint64) (a)[5]) << 16) | \
			 (((uint64) (a)[6]) << 8) | ((uint64) (a)[7]))
#define WM_BUF_PUT_BE64(a, val)				\
	do {						\
		(a)[0] = (uint8) (((uint64) (val)) >> 56);	\
		(a)[1] = (uint8) (((uint64) (val)) >> 48);	\
		(a)[2] = (uint8) (((uint64) (val)) >> 40);	\
		(a)[3] = (uint8) (((uint64) (val)) >> 32);	\
		(a)[4] = (uint8) (((uint64) (val)) >> 24);	\
		(a)[5] = (uint8) (((uint64) (val)) >> 16);	\
		(a)[6] = (uint8) (((uint64) (val)) >> 8);	\
		(a)[7] = (uint8) (((uint64) (val)) & 0xff);	\
	} while (0)

#define WM_BUF_GET_LE64(a) ((((uint64) (a)[7]) << 56) | (((uint64) (a)[6]) << 48) | \
			 (((uint64) (a)[5]) << 40) | (((uint64) (a)[4]) << 32) | \
			 (((uint64) (a)[3]) << 24) | (((uint64) (a)[2]) << 16) | \
			 (((uint64) (a)[1]) << 8) | ((uint64) (a)[0]))


//////////////////////////////////////////////////////////////////////////////
// WmUtils supports utils eg. SysSetting access.
// 
// 
class WmUtils
{
public:
	static uint32 SetStrParam(const char* Key, 
							  const char* Group,
							  const char* Val);


	static uint32 GetStrParam(const char* Key, 
							  const char* Group,
							  char** Val);

	static uint32 SetIntParam(const char* Key, 
							  const char* Group,
							  sint32 Val);


	static uint32 GetIntParam(const char* Key, 
							  const char* Group,
							  sint32& Val);


	//initialize int parameter from wifi manager db (Group=WifiMgr).
	//If the value is not in the db store Value in the Db
	static void InitIntParamFromWmDb(const char* ParamKey, 
									 sint32& Value);

	static void StoreIntParamInWmDb(const char* ParamKey, 
									sint32 Value);
#ifndef ANDROID
	////////////////////////////////////////////////////////////////////////////////
	/// @brief PrintScanList
	///        Debug function prints information stored in m_DummyDb
	static void PrintScanList(OUT WmScanListInfo* pScanListInfoArray, 
							  IN  uint8           Size              );
#endif

}; 


//////////////////////////////////////////////////////////////////////////////
// WmIEParser supports utils eg. SysSetting access.
// 
// 
#ifndef ANDROID
class CWmIEParser
{
public:

///////////////////////////////////////////////////////////////////////////
//@brief    ParseInformationElement
// 			Parse Information element and save the results in the goven
// 			WmScanListInfo.
// 			IMPORTANT:
// 			Based on 7.3.2.25 WPA-Specification. Parts are taken from
// 			appendix C of the WPA-Specification 

	static void ParseInformationElement(uint8		ie[], 
										uint32      ieSize, 
										WmScanListInfo& info  );
private:



};
#endif

class CWmCountryUtils
{
public:

	static EWmCountry StrToEWmCountry(char* strCountry );

	static EWmStatus EWmCountryToStr(EWmCountry Country, char* strCountry);
};

#endif // WM_UTILS_H

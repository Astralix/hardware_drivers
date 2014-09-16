///
/// @file    	ISysSettings.h
///
/// @brief	ISysSettings interfaces' declaration
///
///		An interface for handling system settings, stored into SQLite
/// 		database. System settings are represented as pairs (key, value).
/// 		Key is always a string, that contains group name and parameter 
/// 		name, delimited with space. Value could be one of following 
/// 		types : (integer | string | binary).
///
/// @internal
///
/// @author  	Volodymyr Volkov
///
/// @date    	14/03/2008
///
/// @version 	1.0
///
///		Copyright (C) 2001-2007 DSP GROUP, INC.   All Rights Reserved
///

#ifndef __ISYSSETTINGS_H_INCL__
#define __ISYSSETTINGS_H_INCL__

///////////////////////////////////////////////////////////////////////////////
// Includes
//#include "TypeDefs.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
///
/// @brief IN, OUT and INOUT macros
///
#ifdef ANDROID
#ifndef IN
	#define IN
#endif // IN

#ifndef OUT
	#define OUT
#endif // OUT

#ifndef INOUT
	#define INOUT
#endif // INOUT

#define RC_OK                                   0
#define RC_FAIL                                 1
#endif //ANDROID
///////////////////////////////////////////////////////////////////////////////
// Typedefs
#ifdef ANDROID
typedef unsigned char			uint8;
typedef unsigned short			uint16;
typedef unsigned long			uint32;
typedef unsigned long long		uint64;
typedef unsigned int			uint;
typedef uint16				wchar;


typedef signed char			sint8;
typedef signed short			sint16;
typedef signed long			sint32;
typedef signed long long		sint64;
typedef signed int			sint;

typedef signed char			int8;
typedef signed short			int16;
typedef signed long			int32;
typedef signed long long		int64;
#endif //ANDROID

///////////////////////////////////////////////////////////////////////////////
// Forward declarations

///////////////////////////////////////////////////////////////////////////////
// General types

///////////////////////////////////////////////////////////////////////////////
/// @class  ISysSettings
///
/// @brief  TBD 
///
///  		TBD
///

class ISysSettings
{
public:
	/////////////////////////////////////////////////////////////////////////
	/// Data access functions

	/// @brief      Clean
	virtual uint32 SysSettingsDeleteAll( void ) = 0;


	/// @brief      Use a key&group to get a string value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	/// @param	pValue		Returned string value
	/// 					Note: pValue Valid only untill the next call!
	virtual uint32 SysSettingsGetStringValue(	
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT const char**	ppValue ) = 0;

	/// @brief      Use a key&group to set a string value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsSetStringValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN const char*		pValue ) = 0;

	/// @brief      Use a key&group to get an integer value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsGetIntegerValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT sint32&		Value ) = 0;

	/// @brief      Use a key&group to set an integer value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsSetIntegerValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN sint32		Value ) = 0;

	/// @brief      Use a key&group to get a binary value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	/// @param	Length		Length of returned binary data in bytes
	/// @param	pValue		Returned binary data in bytes
	/// 					Note: pValue Valid only untill the next call!
	virtual uint32 SysSettingsGetBinaryValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT const uint8**	ppValue,
		OUT uint32& 		Length ) = 0;

	/// Use a key&group  to set a binary value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	/// @param	length		Length of provided binary data in bytes
	virtual uint32 SysSettingsSetBinaryValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN const uint8*		pValue,
		IN uint32		Length ) = 0;
#ifdef ANDROID
	virtual ~ISysSettings(){};
#endif //ANDROID


};

/////////////////////////////////////////////////////////////////////////
/// Module management functions

/// @brief 	Initiates connection to the DB, situated by given path
/// @param	strDBName	path to the desired DB file to use (if 
///				NULL is specified 
///				/etc/syssettings/default.sqlite will be
///				used
/// @return			pointer to the instance of the class
ISysSettings* SYSSETTINGS_Init(const char* strDBName);

/// Terminates class functioning and deletes current object
void SYSSETTINGS_Terminate();

///////////////////////////////////////////////////////////////////////////////

#endif // __ISYSSETTINGS_H_INCL__

///////////////////////////////////////////////////////////////////////////////

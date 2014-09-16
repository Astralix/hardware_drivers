#ifndef __CSYSSETTINGS_H_INCL__
#define __CSYSSETTINGS_H_INCL__

///
/// @file    	CSysSettings.h
///
/// @brief		CSysSettings classes' declaration
///
///				TBD
///
/// @internal
///
/// @author  	Volodymyr Volkov
///
/// @date    	14/03/2008
///
/// @version 	1.0
///
///				Copyright (C) 2001-2007 DSP GROUP, INC.   All Rights Reserved
///

///////////////////////////////////////////////////////////////////////////////
// Includes
#include "libsqlitewrapped.h"
#ifndef ANDROID
#include "Interfaces/NonCom/SysSettings/ISysSettings.h"
#else
#include "ISysSettings.h"
#endif

///////////////////////////////////////////////////////////////////////////////
// Defines

#define DEFAULT_DATABASE_FILENAME "/etc/syssettings/syssettings.sqlite"

///////////////////////////////////////////////////////////////////////////////
// Typedefs

///////////////////////////////////////////////////////////////////////////////
// Forward declarations

///////////////////////////////////////////////////////////////////////////////
// General types

///////////////////////////////////////////////////////////////////////////////
/// @class  CSysSettings
///
/// @brief  TBD 
///
///  		TBD
///
class CSysSettings : public ISysSettings
{

public:
///////////////////////////////////////////////////////////////////////////
// @class	CStderrInfraLog
// 
// @brief	This class overrides StderrLog class' error handling using
// 			INFRA_DLOG logging mechanism
	class CStderrInfraLog : public IError
	{
	public:
		void error(Database&,const std::string&);
		void error(Database&,Query&,const std::string&);
	};

private:
	/////////////////////////////////////////////////////////////////////////
	// Constructor
	CSysSettings();
#ifndef ANDROID
public:
#endif
	/////////////////////////////////////////////////////////////////////////
	// Destructor
	virtual ~CSysSettings();

public:
	/////////////////////////////////////////////////////////////////////////
	/// Data access functions

	/// @brief      Clean
	virtual uint32 SysSettingsDeleteAll( void );


	/// @brief      Use a key&group to get a string value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsGetStringValue(	
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT const char**	ppValue );

	/// @brief      Use a key&group to set a string value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsSetStringValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN const char*		pValue );

	/// @brief      Use a key&group to get an integer value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsGetIntegerValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT sint32&			Value );

	/// @brief      Use a key&group to set an integer value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	virtual uint32 SysSettingsSetIntegerValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN sint32			Value );

	/// @brief      Use a key&group to get a binary value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	/// @param	Length		Length of returned binary data in bytes
	virtual uint32 SysSettingsGetBinaryValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		OUT const uint8**	ppValue,
		OUT uint32& 		Length );

	/// @brief		Use a key&group  to set a binary value
	/// @param	pKey		Key Name
	/// @param	pGroup		Group Name (can be NULL)
	/// @param	length		Length of returned binary data in bytes
	virtual uint32 SysSettingsSetBinaryValue(
		IN const char*		pKey,
		IN const char*		pGroup, 
		IN const uint8*		pValue,
		IN uint32			Length );

public: 

	///////////////////////////////////////////////////////////////////////////
	//	Package scope types


public: 
	///////////////////////////////////////////////////////////////////////////
	// Package scope methods

	/// This function defines new name of the DB and initialises it. The 
	/// default DB name is  DATABASE_FILENAME
	uint32 ResetDBName( IN const char* strDBName );

	/// This function returnes an instance of the class
	static CSysSettings* GetInstance();

	static void DestroyInstance();

private:

	///////////////////////////////////////////////////////////////////////////
	// Private types

	enum ESettingType
	{
		INTEGER, STRING, BINARY
	};

private:

	///////////////////////////////////////////////////////////////////////////
	// Private constants


private:

	///////////////////////////////////////////////////////////////////////////
	// Private membres

	/// General function for storing value into DB table
	/// @param	key			sting with "<groupname> <paramname>"
	/// @param	type		type of value to be stored
	/// @param	value		pointer to memory area with value to be stored
	/// @param	length		length of value in bytes
	uint32 SetValue( IN const char*		pKey,
					 IN const char*		pGroup,
					 IN ESettingType    Type,
					 IN void**			ppValue,
					 IN uint32			Length );

	/// General function for retrieving of key's values from the DB
	/// @param	key			sting with "<groupname> <paramname>"
	/// @param	type		type of value that was retrieved
	/// @param	ReqType		requested type of value by the user
	/// @param	value		pointer to memory area with where retrieved value 
	///						was stored
	/// @param	length		length of retrieved value in bytes
	uint32 GetValue( IN const char*		pKey,
					 IN const char*		pGroup,
					 IN ESettingType	RequiredType,
					 OUT void**			ppValue,
					 OUT uint32&		Length );

	/// This function validates that DB exists and has correct schema.
    /// If DB doesnot exist or its schema corrupted creates a new db.
	uint32 ValidateDB();

	/// This function evaluates ID of record by given key and group names
	uint32 GetIDFromKeyGroup( IN const char* pKey,
							  IN const char* pGroup );

	/// This function deletes old buffer and allocates new with given length
	void* AllocateBuffer( uint8* pBuffer, uint32 length );

private:

	//////////////////////////////////////////////////////////////////////////
	// Private member variables

	// singleton instance container
	static CSysSettings* pInstance;

	/// This object handels all DB errors
	CStderrInfraLog m_Log;

	/// This variable handles DB connection
	mutable Database* m_pDB;

	/// This buffer is used for reading binary values
	uint8* m_pBinaryBuffer;

	/// This buffer is used for reading string values
	uint8* m_pStringBuffer;

protected:

	///////////////////////////////////////////////////////////////////////////
	// Protected members

};


///////////////////////////////////////////////////////////////////////////////

#endif // __ISYSSETTINGS_H_INCL__

///////////////////////////////////////////////////////////////////////////////

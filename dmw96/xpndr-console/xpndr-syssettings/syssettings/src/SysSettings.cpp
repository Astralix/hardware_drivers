///
/// @file    	CSysSettings.cpp
///
/// @brief		CCCSysSettings class'  implementation
///
///				This class handles DAL of simple db that is capable to store "key=value" settings.
///
/// @internal
///
/// @author  	Volodymyr Volkov
///
/// @date    	3/17/2008
///
/// @version 	1.0
///
///				Copyright (C) 2007-2008 DSP GROUP, INC.   All Rights Reserved
///

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <cstring>
#include <sstream>
#ifndef ANDROID
#include "infra/include/InfraMacros.h"
#endif


#include "SysSettings.h"
#ifdef ANDROID
#include <unistd.h>
#endif

using namespace std;
///////////////////////////////////////////////////////////////////////////////
// Defines

#define MAX_KEY_LENGTH 100


///////////////////////////////////////////////////////////////////////////////
//  CSysSettings::CStderrInfraLog implementation

CSysSettings* CSysSettings::pInstance = NULL;



///////////////////////////////////////////////////////////////////////////////
// SQLite (wrapper's) error messages handling functions

void CSysSettings::CStderrInfraLog::error(
	Database& db, const string& str )
{
#ifndef ANDROID
	ARGUSED(db);
	ARGUSED(str);
#endif
	//SH_printf("SQLite encountered an error: %s", str.c_str());
/*
	INFRA_DLOG( (LM_APP_FRAMEWORK, LP_ERROR, 
			"SQLite encountered an error: %s", str.c_str() ) );
*/
}

void CSysSettings::CStderrInfraLog::error(
	Database& db, Query& qry, const string& str )
{
#ifndef ANDROID
	ARGUSED(db);
	ARGUSED(qry);
	ARGUSED(str);
#endif
	//SH_printf("SQLite encountered an error: %s\n\tIn query: %s",
	//		   str.c_str(), qry.GetLastQuery().c_str() );
/*
	INFRA_DLOG( (LM_APP_FRAMEWORK, LP_ERROR, 
			"SQLite encountered an error: %s\n\tIn query: %s",
			str.c_str(), qry.GetLastQuery().c_str() ) );
*/
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  CSysSettings implementation

///////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

CSysSettings::CSysSettings() : m_Log(), m_pDB(NULL)
{
	m_pBinaryBuffer = NULL;
	m_pStringBuffer = NULL;
}

CSysSettings::~CSysSettings()
{
	delete m_pDB;
	m_pDB = NULL;

	delete m_pBinaryBuffer;
	m_pBinaryBuffer = NULL;

	delete m_pStringBuffer;
	m_pStringBuffer = NULL;
}

/////////////////////////////////////////////////////////////////////////
/// Module management functions

/// @brief 	Initiates connection to the DB, situated by given path
/// @param	strDBName	path to the desired DB file to use (if 
///				NULL is specified 
///				/etc/syssettings/default.sqlite will be
///				used
/// @return			pointer to the instance of the class
ISysSettings* SYSSETTINGS_Init(const char* strDBName)
{
	CSysSettings* pSysSettings = CSysSettings::GetInstance();

	pSysSettings->ResetDBName(strDBName);

	return (ISysSettings*)pSysSettings;
}

/// Terminates class functioning and deletes current object
void SYSSETTINGS_Terminate()
{
	CSysSettings::DestroyInstance();
}

//////////////////////////////////////////////////////////////////////////////
// Package scope methods implementation

CSysSettings* CSysSettings::GetInstance()
{
	if ( !pInstance )
	{
		pInstance = new CSysSettings();
	}

	return pInstance;
}

void CSysSettings::DestroyInstance()
{
	if( pInstance ) 
	{
		delete pInstance;
	}

	pInstance = NULL;
}

uint32 CSysSettings::GetIDFromKeyGroup(
	IN const char* pKey,
	IN const char* pGroup )
{
	if ( !pKey )
	{
		return RC_FAIL;
	}

	Query query(*m_pDB);
	stringstream ssQuery;
	ssQuery << "select ID from PLAIN_TABLE where GROUP_NAME ";
	if ( pGroup )
	{
		ssQuery << " = '" << pGroup << "'";
	}
	else
	{
		ssQuery << " IS NULL";
	}
	ssQuery << " and  KEY_NAME = '" << pKey << "';";
	return query.get_count(ssQuery.str());
}

uint32 CSysSettings::ResetDBName( IN const char* strDBName )
{
	if ( strDBName == NULL )
	{
		strDBName = DEFAULT_DATABASE_FILENAME;
	}

	if (m_pDB)
	{
		delete m_pDB;
	}

	m_pDB = new Database(strDBName, &m_Log);
#ifndef ANDROID
    INFRA_ASSERT(m_pDB);
#else
    if(!m_pDB)
    {
		return RC_FAIL;
    }
#endif
    if( !ValidateDB() )
	{
		return RC_FAIL;
	}
	
	return RC_OK;
}

uint32 CSysSettings::SetValue(
	IN const char*		pKey,
	IN const char*		pGroup,
	IN ESettingType		Type,
	IN void**			ppValue,
	IN uint32			Length )
{

    uint32 nID = GetIDFromKeyGroup( pKey, pGroup );

	Query query(*m_pDB);
	stringstream ssQuery;

	
	if ( 0 != nID )
	{	
		
		// we have a record with the same Key & Group pair as requested to set
		// -> delete it

		// resolve record type
		ssQuery << "select TYPE from PLAIN_TABLE where ID = " << nID;
		ESettingType oldType = (ESettingType)query.get_count(ssQuery.str());		

		// delete attached value
		ssQuery.str("");
		switch ( oldType )
		{
		case INTEGER:
			ssQuery << "delete from INTEGER_TYPE where ID = " << nID;
			break;
		case STRING:
			ssQuery << "delete from STRING_TYPE where ID = " << nID;
			break;
		case BINARY:
			ssQuery << "delete from BINARY_TYPE where ID = " << nID;
			break;
		}
		query.execute(ssQuery.str());

		// delete the record from palin table
		ssQuery.str("");
		ssQuery << "delete from PLAIN_TABLE where ID = " << nID;
		query.execute(ssQuery.str());
	}
	

	ssQuery.str("");
	if ( pGroup )
	{
		ssQuery << "insert into PLAIN_TABLE " << 
			"(GROUP_NAME, KEY_NAME, TYPE) " <<
			"values ('" << pGroup << "', '" <<
			pKey << "', " << Type << ")";
	}
	else
	{
		ssQuery << "insert into PLAIN_TABLE " << 
			"(KEY_NAME, TYPE) " <<
			"values ('" << pKey << "', " << Type << ")";
	}
	if(!query.execute(ssQuery.str()))
	{
		return RC_FAIL;
	}
	sync();

	nID = GetIDFromKeyGroup( pKey, pGroup );
	
	//for some reason we failed to add the value
	if ( 0 == nID )
	{
		return RC_FAIL;
	}

	ssQuery.str("");
	uint8* buffer = NULL;
	switch ( Type )
	{
	case INTEGER:
		ssQuery.str("");
		ssQuery << "insert into INTEGER_TYPE (ID, VALUE) values (" <<
			nID << ", '" << *(sint32*)*ppValue << "')";
		break;
	case STRING:
		ssQuery << "insert into STRING_TYPE (ID, VALUE) values (" <<
			nID << ", '" << (char*)*ppValue << "')";
		break;
	case BINARY:
		// Allow extra space for encoded binary as per comments in
		// SQLite encode.c See bottom of this file for implementation
		// of SQLite functions use 3 instead of 2 just to be sure ;-)
		buffer = new uint8[ 3 + (257*Length)/254 ];
		sqlite3_encode_binary((const unsigned char*)*ppValue, Length, buffer);

		ssQuery << "insert into BINARY_TYPE (ID, VALUE) values (" <<
			nID << ", '" << (char*)buffer << "')";

		break;
	}

    if(!query.execute(ssQuery.str()))
	{
		ssQuery.str("");
		ssQuery << "delete from PLAIN_TABLE where ID = " << nID;
		query.execute(ssQuery.str());
		sync();
		return RC_FAIL;
	}

	if ( buffer )
	{
		delete[] buffer;
	}

	sync();
	return RC_OK;
}

uint32 CSysSettings::GetValue(
	IN const char*		pKey,
	IN const char*		pGroup,
	IN ESettingType	    RequiredType,
	OUT void**			ppValue,
	OUT uint32&			Length )
{
	uint32 nID = GetIDFromKeyGroup( pKey, pGroup );

	if ( 0 == nID )
	{
		return RC_FAIL;
	}

	Query query(*m_pDB);
	stringstream ssQuery;
	ssQuery << "select TYPE from PLAIN_TABLE where ID = " << nID;
	ESettingType ActualType = (ESettingType)query.get_count(ssQuery.str());
	
	if(RequiredType != ActualType) 
	{
		return RC_FAIL;
	}

	ssQuery.str("");
	const char* strValue;
	switch ( RequiredType )
	{
	case INTEGER:
		ssQuery << "select VALUE from INTEGER_TYPE where ID = " << nID;
		*(sint32*)*ppValue = query.get_count(ssQuery.str());
		Length = sizeof(sint32);
		break;
	case STRING:
		ssQuery << "select VALUE from STRING_TYPE where ID = " << nID;
		strValue = query.get_string(ssQuery.str());
		Length = strlen(strValue);
		*ppValue = AllocateBuffer( m_pStringBuffer, Length + 1 );
		strcpy( (char*)*ppValue, strValue );
		break;
	case BINARY:
		ssQuery << "select VALUE from BINARY_TYPE where ID = " << nID;
		strValue = query.get_string(ssQuery.str());
		*ppValue = AllocateBuffer( m_pBinaryBuffer, strlen(strValue) + 1 );
		Length = sqlite3_decode_binary(
			(const unsigned char*)strValue, 
			(unsigned char*)*ppValue
		);
		break;
	}

	return RC_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  CSysSettings public interface implementation

/// @brief      Clean
uint32 CSysSettings::SysSettingsDeleteAll( void )
{
	Query query(*m_pDB);
	stringstream ssQuery;
	ssQuery << "delete from PLAIN_TABLE";
	query.execute(ssQuery.str());

	ssQuery.str("");
	ssQuery << "delete from INTEGER_TYPE";
	query.execute(ssQuery.str());

	ssQuery.str("");
	ssQuery << "delete from STRING_TYPE";
	query.execute(ssQuery.str());

	ssQuery.str("");
	ssQuery << "delete from BINARY_TYPE";
	query.execute(ssQuery.str());

	sync();
	return RC_OK;
}


/// @brief      Use a key&group to get a string value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
uint32 CSysSettings::SysSettingsGetStringValue(	
	IN const char*		pKey,
	IN const char*		pGroup, 
	OUT const char**	ppValue )
{
	ESettingType Type;
	uint32 Length;
	return GetValue(pKey, pGroup, STRING, (void**)ppValue, Length);
}

/// @brief      Use a key&group to set a string value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
uint32 CSysSettings::SysSettingsSetStringValue(
	IN const char*		pKey,
	IN const char*		pGroup, 
	IN const char*		pValue )
{
	return SetValue(pKey, pGroup, STRING, (void**)&pValue, strlen(pValue)+1 );
}

/// @brief      Use a key&group to get an integer value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
uint32 CSysSettings::SysSettingsGetIntegerValue(
	IN const char*		pKey,
	IN const char*		pGroup, 
	OUT sint32&			Value )
{
	ESettingType Type;
	uint32 Length;
	sint32* pValue = &Value;
	return GetValue(pKey, pGroup, INTEGER, (void**)&pValue, Length);
}

/// @brief      Use a key&group to set an integer value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
uint32 CSysSettings::SysSettingsSetIntegerValue(
	IN const char*		pKey,
	IN const char*		pGroup, 
	IN sint32			Value )
{
	sint32* pValue = &Value;
	return SetValue(pKey, pGroup, INTEGER, (void**)&pValue, sizeof(Value));
}

/// @brief      Use a key&group to get a binary value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
/// @param	Length		Length of returned binary data in bytes
uint32 CSysSettings::SysSettingsGetBinaryValue(
	IN const char*		pKey,
	IN const char*		pGroup, 
	OUT const uint8**	ppValue,
	OUT uint32& 		Length )
{
	ESettingType Type;
	return GetValue(pKey, pGroup, BINARY, (void**)ppValue, Length);
}

/// @brief		Use a key&group  to set a binary value
/// @param	pKey		Key Name
/// @param	pGroup		Group Name (can be NULL)
/// @param	length		Length of returned binary data in bytes
uint32 CSysSettings::SysSettingsSetBinaryValue(
	IN const char*		pKey,
	IN const char*		pGroup, 
	IN const uint8*		pValue,
	IN uint32			Length )
{
	return SetValue(pKey, pGroup, BINARY, (void**)&pValue, Length);
}

uint32 CSysSettings::ValidateDB()
{

	
	bool result = true;
	
	if (!m_pDB->Connected())
	{
		return RC_FAIL;
	}
	
    //Note that changing table's structure may require change when
    //reading data from table. Look for fetch_row and see if data is
    //read in the correct order after change or add new column at the end. 
    const char* strTablesCreateSql[][2] =
    {
        {
        "CREATE TABLE 'PLAIN_TABLE' ("
        "\"ID\" INTEGER PRIMARY KEY AUTOINCREMENT,"
        "\"GROUP_NAME\" TEXT,"
        "\"KEY_NAME\" TEXT,"
        "\"TYPE\" INTEGER"
        ")",
        "PLAIN_TABLE"
        },

        {
        "CREATE TABLE \"INTEGER_TYPE\" ("
        "\"ID\" INTEGER PRIMARY KEY NOT NULL,"
        "\"VALUE\" INTEGER"
        ")",
        "INTEGER_TYPE"
        },
        {
        "CREATE TABLE \"STRING_TYPE\" ("
        "\"ID\" INTEGER PRIMARY KEY NOT NULL,"
        "\"VALUE\" TEXT"
        ")",
        "STRING_TYPE"
        },
        {
        "CREATE TABLE \"BINARY_TYPE\" ("
        "\"ID\" INTEGER PRIMARY KEY NOT NULL,"
        "\"VALUE\" BLOB"
        ")",
        "BINARY_TYPE"
        }
    };
    // Verify db exists and have a proper scheme.
	// The idea is following : there is a special table inside SQLite, called
	// 'sqlite_master' which consists descriptons of all objects created in 
	// the database. Tables are stored among them. Following code circles 
	// through list of nesessary objects and search them in sqlite_master in
	// column called 'sql'. If db does not exist, or corrupted creates a new one.		
    Query query(*m_pDB);
    stringstream QueryStr;
    
    for ( uint32 i = 0; 
          i < sizeof(strTablesCreateSql)/sizeof(strTablesCreateSql[0]);
          ++i )
    {
        QueryStr.str("");
        QueryStr << "select sql from sqlite_master where name = '" <<
        strTablesCreateSql[i][1] << "';";
        const char* strResult = query.get_string(QueryStr.str());
        if ( strcmp(strResult,strTablesCreateSql[i][0])!=0 )
        {
            QueryStr.str(strTablesCreateSql[i][0]);
            if(!query.execute(QueryStr.str()))
            {
                    return RC_FAIL;
            }
        }
    } 
	sync();
    return RC_OK;
}


void* CSysSettings::AllocateBuffer( uint8* pBuffer, uint32 length )
{
	if ( pBuffer )
	{
		delete pBuffer;
	}

	pBuffer = new uint8[length];

	return (void*)pBuffer;
}


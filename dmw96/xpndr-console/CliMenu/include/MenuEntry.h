///////////////////////////////////////////////////////////////////////////////
///
/// @file    	MenuEntry.h
///
/// @brief		Abstraction over menu entry. 
///
///
/// @internal
///
/// @author  	ronyr
///
/// @date    	28/01/2008
///
/// @version 	1.0
///
///
///				Copyright (C) 2001-2008 DSP GROUP, INC.   All Rights Reserved
///	
/// 
#ifndef MENUENTRY_H
#define MENUENTRY_H

#include "TypeDefs.h"
#include <string>
#include <iostream>

using namespace::std;

enum MenuEntryType {

	MENUENTRY_CMD,
	MENUENTRY_SUBMENU
};

#include "Common.h"

///////////////////////////////////////////////////////////////////////////////
/// @class	CMenuEntry
///
/// @brief  Defines interface to menu entry . 
///
///  		Each class that will implement menu entry type should derive from this
/// 		class or from classes that will derive from it 
///
class CMenuEntry {
public:


	//////////////////////////////////////////////////////////////////////////
	///	@brief	CMenuEntry
	///         Contsructor of CMenuEntry class
	/// 		@param[in] name - Name of the entry. It will serve as command for 
	/// 						  selecting this entry. 
	/// 		@param[in] description - Description of the entry. Description will
	/// 								 be displayed when parent sub menu of this entry
	/// 						         will display its entries.
	///         @param[in] shortcut - Shortcut key for selecting this entry
	CMenuEntry(const std::string & name,
			   const std::string & description,
			   uint8 shortcut);

	//////////////////////////////////////////////////////////////////////////
	///	@brief	~CMenuEntry
	///         Distructor of CMenuEntry class
	virtual ~CMenuEntry();

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Execute
	///         Function that should be implemented by derived classes to specify
	/// 		the behaviour of the entry when it is selected
	virtual void Execute() = 0;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	GetEntryType
	///         Function that should be implemented by derived classes
	/// 		Function should return the type of the entry Command/SubMenu
	/// 		@return    MENUENTRY_CMD  - if the class implements command entry.
	/// 				  !MENUENTRY_SUBMENU  - if the class implements sub menu entry.		
	virtual MenuEntryType GetEntryType() const = 0;


	//////////////////////////////////////////////////////////////////////////
	///	@brief	CanHandle
	/// 		Function  returns whether it can handle givven command
	/// 		@param[in] command - Command string 
	/// 		@return    true  - if object can handle the command.
	/// 				  false  - if object cannot handle the command.		
	virtual bool CanHandle(const std::string & command) const;


	//////////////////////////////////////////////////////////////////////////
	///	@brief	CanHandle
	/// 		Function  returns whether it can handle givven shortcut
	/// 		@param[in] shorcut - Command shortcut
	/// 		@return    true  - if object can handle the shortcut.
	/// 				  false  - if object cannot handle the shortcut.		
	virtual bool CanHandle(uint8 shortcut) const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	GetParent
	/// 		Function  returns pointer to entries parent
	// 			@return   pointer to entries parent
	/// 				  NULL  - if entry has no parent
	virtual const CMenuEntry* GetParent() const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Name
	/// 		Function  returns name of the entry
	// 			@return   string containing name of the entry
	virtual const std::string & Name() const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Desc
	/// 		Function  returns description of the entry
	// 			@return   string containing description of the entry
	virtual const std::string & Desc() const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Shortcut
	/// 		Function  returns shortcut for entry selection
	// 			@return   character used for entry selection
	virtual int Shortcut() const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	GetPath
	/// 		Function  returns full path of the menu entry, relative to menu root
	/// 		@param[out] strPath - string that contains full path of the entry
	virtual void GetPath(std::string& strPath) const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	SetParent
	/// 		Function  sets parent to the entry
	/// 		@param[in] parent - pointer to new parent of the menu entry
	virtual void SetParent(CMenuEntry* parent);

protected:

	CMenuEntry* m_parent;

	const std::string m_strName;

	const std::string m_strDescription;

	int m_iShortcut;


};

#endif /*MENUENTRY_H*/


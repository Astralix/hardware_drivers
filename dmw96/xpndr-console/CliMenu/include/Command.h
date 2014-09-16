///////////////////////////////////////////////////////////////////////////////
///
/// @file    	Command.h
///
/// @brief		Abstraction over command entry. 
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
#ifndef COMMAND_H
#define COMMAND_H

#include "MenuEntry.h"

#include <string>

///////////////////////////////////////////////////////////////////////////////
/// @class	CCommand
///
/// @brief  Extends CMenuEntry to provide abstraction over command . 
///
class CCommand : public CMenuEntry {
public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief	CCommand
	///         Contsructor of CCommand class
	/// 		@param[in] name - Name of the entry. It will serve as command for 
	/// 						  selecting this entry. 
	/// 		@param[in] description - Description of the entry. Description will
	/// 								 be displayed when parent sub menu of this entry
	/// 						         will display its entries.
	///         @param[in] shortcut - Shortcut key for selecting this entry
	CCommand(const std::string & name,
			  const std::string & description,
			  uint8 shortcut);

	////////////////////////////////////////////////////////////////////////
	///	@brief	~CCommand
	///         Destructor of ~CCommand class
	virtual ~CCommand();

	//////////////////////////////////////////////////////////////////////////
	///	@brief	GetEntryType
	///         Function tells that the type of the entry is Command
	/// 		@return    MENUENTRY_COMMAND  - tells that class implements 
	/// 										command entry.	
	virtual MenuEntryType GetEntryType() const;

protected:

};

#endif /*COMMAND_H*/

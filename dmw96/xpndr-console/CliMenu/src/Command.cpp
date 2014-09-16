///////////////////////////////////////////////////////////////////////////////
///
/// @file    	Command.cpp
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
#include "Command.h"

CCommand::CCommand(const std::string & name,
		  const std::string & description,
		  uint8 shortcut):CMenuEntry(name,description,shortcut)
{

}

CCommand::~CCommand()
{

}

/**
 * @return if command or submenu.
 */
MenuEntryType CCommand::GetEntryType() const
{

	return MENUENTRY_CMD;
}



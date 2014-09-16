///////////////////////////////////////////////////////////////////////////////
///
/// @file    	ActionCalbackCommand.cpp
///
/// @brief		Abstraction over action callback command entry. 
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
#include "ActionCallbackCommand.h"
#include <iostream>


CActionCallbackCommand::CActionCallbackCommand(
	const std::string & name,
	const std::string & description,
	uint8 shortcut,
	ACTION action) : CCommand(name,description,shortcut),m_action(action)
{

}

CActionCallbackCommand::~CActionCallbackCommand()
{

}

/**
 * Execute the command.
 */
void CActionCallbackCommand::Execute()
{

	if(m_action != NULL) {

		std::string strPath;
		GetPath(strPath);

		cout << endl << strPath << "\\[Start]>" << endl;
		m_action();
		cout << endl << strPath << "\\[Done]>" << endl;

	}

}



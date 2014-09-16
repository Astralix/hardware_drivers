///////////////////////////////////////////////////////////////////////////////
///
/// @file    	ActionCommand.cpp
///
/// @brief		Abstraction over action command entry. 
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
#include "ActionCommand.h"
#include <iostream>


CActionCommand::CActionCommand(const std::string & name,
			  const std::string & description,
			  uint8 shortcut):CCommand(name,description,shortcut)
{

}

CActionCommand::~CActionCommand()
{

}

/**
 * Execute the command.
 */
void CActionCommand::Execute()
{
	std::string strPath;
	GetPath(strPath);

	cout << endl << strPath << "\\[Start]>" << endl;
	Action();
	cout << endl << strPath << "\\[Done]>" << endl;
}


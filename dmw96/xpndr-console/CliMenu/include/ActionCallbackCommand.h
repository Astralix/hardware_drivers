///////////////////////////////////////////////////////////////////////////////
///
/// @file    	ActionCallbackCommand.h
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
#ifndef ACTIONCALLBACKCOMMAND_H
#define ACTIONCALLBACKCOMMAND_H

#include "Command.h"

#include <string>

typedef void (*ACTION)();

class CActionCallbackCommand : public CCommand {
public:

	CActionCallbackCommand(const std::string & name,
				  const std::string & description,
				  uint8 shortcut,
                  ACTION action);

	virtual ~CActionCallbackCommand();

	/**
	 * Execute the command.
	 */
	virtual void Execute();


protected:

	ACTION m_action;

};

#endif /*ACTIONCALLBACKCOMMAND_H*/



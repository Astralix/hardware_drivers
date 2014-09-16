///////////////////////////////////////////////////////////////////////////////
///
/// @file    	ActionCommand.h
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
#ifndef ACTIONCOMMAND_H
#define ACTIONCOMMAND_H

#include "Command.h"

#include <string>

///////////////////////////////////////////////////////////////////////////////
/// @class	CActionCommand
///
/// @brief  Extends CCommand to provide additional abstraction over command . 
///
class CActionCommand : public CCommand {
public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief	CCommand
	///         Contsructor of CActionCommand class
	/// 		@param[in] name - Name of the entry. It will serve
	/// 						  as command for selecting this entry. 
	/// 		@param[in] description - Description of the entry. 
	/// 								 Description will be displayed when
	/// 								parent sub menu of this entry
	/// 						         will display its entries.
	///         @param[in] shortcut - Shortcut key for selecting this entry
	CActionCommand(const std::string & name,
				  const std::string & description,
				  uint8 shortcut);
	////////////////////////////////////////////////////////////////////////
	///	@brief	~CActionCommand
	///         Destructor of ~CActionCommand class
	virtual ~CActionCommand();

public: 

	/// 


	////////////////////////////////////////////////////////////////////////
	///	@brief	Execute
	///         This function is called when command is selected. 
	/// 		Function display some output to user and calls to Action() 
	/// 		function which should be implemented by classes derived from 
	/// 		this class
	virtual void Execute();

public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief Action
	///        Function that should be implemented by derived classes to specify
	/// 	   the behaviour of the command when it is selected
	virtual void Action() {};


protected:

	

};

#endif /*ACTIONCOMMAND_H*/


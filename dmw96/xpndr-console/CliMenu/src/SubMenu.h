///////////////////////////////////////////////////////////////////////////////
///
/// @file    	SubMenu.h
///
/// @brief		This file provides a definition of CSubMenu class
/// 			that represents an abstraction over sub menu entry of CLI Menu
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
#ifndef SUBMENU_H
#define SUBMENU_H

#include <string>
#include <vector>

#include "Common.h"
#include "MenuEntry.h"
#include "ActionCommand.h"


//////////////////////////////////////////////////////////////////////////////
/// @class	CSubMenu
///
/// @brief  This class provides an abstraction over 
/// 		sub menu entry of CLI Menu. This class 
/// 		derives from CMenuEntry class and implements 
/// 		all its pure virtual functions. CSubMenu class
/// 		can be instantiated.
///
class CSubMenu : public CMenuEntry {
public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief	CSubMenu
	///         Constructor of CSubMenu class
	/// 		@param[in] name - Name of the submenu. This will also serve
	/// 						  as a command that user will type to select
	/// 						  this sub menu in a case that input mode
	/// 						 is INPUTMODE_BYCOMMAND or INPUTMODE_BOTH. 
	/// 		@param[in] description - Description string of the sub menu
	/// 		@param[in] shortcut - Shortcut key, that user will have to
	/// 							 type to select this sub menu, in a case
	/// 							 that input mode is INPUTMODE_BYSHORTCUT
	/// 							 or INPUTMODE_BOTH. 
	/// 							 Otherwise this parameter will be ignored.
	/// 		@param[in] inputMode - This parameter controls a way user 
	/// 							   selects commands from the menu. 
	/// 							   Parameter can get following values:
	///                                a. INPUTMODE_BYSHORTCUT - commands are
	/// 														 selected by 
	/// 														typing a shortcut
	/// 														char.
	///                                b.INPUTMODE_BYCOMMAND - commands are 
	/// 													   selected by typing
	/// 													   full command name.
	///                                c.INPUTMODE_BOTH - commands are selected
	/// 												  either by typing a 
	/// 												  shortcut or 
	/// 												  command name.
	/// 		@param[in] displayMode - This parameter controls how the menus 
	/// 								 are displayed. 
	/// 								 Parameter can get following values:
	///                                 a.	DISPLAYMODE_FULL - all sub menus 
	/// 													   entries are 
	/// 													   displayed.
	///                                 b.	DISPLAYMODE_SILENT - sub menus entries
	/// 														 are not displayed.
	/// 														 To display entries
	/// 														user will have 
	/// 													   to type '1'.
	CSubMenu(const std::string & name,
			  const std::string & description,
			  uint8 shortcut,
			  EMenuEntryInputMode inputMode = INPUTMODE_BYSHORTCUT,
			  EMenuDisplayMode displayMode = DISPLAYMODE_FULL);

	////////////////////////////////////////////////////////////////////////
	///	@brief	~CSubMenu
	///         Destructor of CSubMenu class
	virtual ~CSubMenu();

	////////////////////////////////////////////////////////////////////////
	///	@brief	Execute
	///         This function is called when sub menu is selected and 
	/// 		receives the menu context. Function is responsible to
	/// 		display etries belonging to this sub menu, receive and parse user's
	/// 		input and according to the input give up or pass a menu context
	/// 		to other sub menus or commands.
	virtual void Execute();

	//////////////////////////////////////////////////////////////////////////
	///	@brief	GetEntryType
	///         Function tells that the type of the entry is SubMenu
	/// 		@return    MENUENTRY_SUBMENU  - tells that class implements 
	/// 										sub menu entry.	
	virtual MenuEntryType GetEntryType() const;

	//////////////////////////////////////////////////////////////////////////
	///	@brief	AddMenuEntry
	///         Function is called to add entries to this sub menu
	/// 		@param[in] newEntry - Pointer to entry to be added
	/// 		@return - CLIMENU_RES_OK - in a case of success
	/// 				  error code - otherwise
	virtual ECliMenuRes AddMenuEntry(CMenuEntry* pNewEntry);

protected:

	////////////////////////////////////////////////////////////////////////
	///	@brief	GetInputCommand
	///         Function is called to get input from user and try
	/// 		to find an entry that matches the input
	/// 		@param[out] cmd - if there is an entry that matches the input
	/// 						  command, cmd will point on entry pointer,
	/// 						  otherwise it will be NULL.
	/// 		@return - true  - if exit command was entered
	/// 				  false - otherwise
	virtual  bool GetInputCommand(CMenuEntry** cmd) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	CheckForReturnCmd
	///         Function tells whether command is "Exit" or not
	/// 		@param[in] command - command string
    /// 		@return - true  - if command is "Exit" command
	/// 				  false - otherwise
	bool CheckForReturnCmd(const std::string& command) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	CheckForReturnCmd
	///         Function tells whether shortcut is shortcut of  "Exit" command
	/// 		@param[in] shortcutKey - shortkey char 
	/// 		@return - true  - if shortcut belongs to "Exit" command
	/// 				  false - otherwise
	bool CheckForReturnCmd(uint8 shortcutKey) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	CheckForDisplayCmd
	///         Function tells whether command is "Display" or not
	/// 		@param[in] command - command string
	/// 		@return - true  - if command is "Display" command
	/// 				  false - otherwise
	bool CheckForDisplayCmd(const std::string& command) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	CheckForDisplayCmd
	///         Function tells whether shortcut is shortcut of  "Display" command
	/// 		@param[in] shortcutKey - shortkey char 
	/// 		@return - true  - if shortcut belongs to "Display" command
	/// 				  false - otherwise
	bool CheckForDisplayCmd(uint8 shortcutKey) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	ProcessCommand
	///         Function is called to activate selected entry
	/// 		@param[in] cmd - pointer on selected entry
	void ProcessCommand(CMenuEntry* cmd) const;

	////////////////////////////////////////////////////////////////////////
	///	@brief	DisplayMenu
	///         Function is called to display sub menu
	/// 		@param[in] displayMode - This parameter controls how the menus 
	/// 								 are displayed. 
	/// 								 Parameter can get following values:
	///                                 a.	DISPLAYMODE_FULL - all sub menus 
	/// 													   entries are 
	/// 													   displayed.
	///                                 b.	DISPLAYMODE_SILENT - sub menus entries
	/// 														 are not displayed.
	/// 														 To display entries
	/// 														user will have 
	/// 													   to type '1'.
	void DisplayMenu(EMenuDisplayMode displayMode = DISPLAYMODE_FULL) const;

	typedef std::vector< CMenuEntry* > MenuEntriesList;

	MenuEntriesList m_menuEntries;

	EMenuEntryInputMode m_inputMode;

	EMenuDisplayMode m_displayMode;

	CActionCommand* m_pReturnCommand;

};

#endif /*SUBMENU_H*/


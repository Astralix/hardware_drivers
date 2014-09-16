///////////////////////////////////////////////////////////////////////////////
///
/// @file    	CliMenu.h
///
/// @brief		This class provides an implementation for 
/// 			building and operating CLI Menu. 
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
#ifndef CLIMENU_H
#define CLIMENU_H

#include "TypeDefs.h"
#include "Common.h"
#include "Command.h"

//////////////////////////////////////////////////////////////////////////////
/// @class	CCliMenu
///
/// @brief  This class provides an implementation for 
/// 		building and operating CLI Menu. 
///

///Forward declaration of CSubMenu

class CSubMenu;

class CCliMenu {

public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief CCliMenu
	///        Contsructor of CMenuEntry class
	/// 	   @param[in] name - Name of the root menu. This name will be
	/// 	 				  	displayed as a first name of the root path. 
	/// 	   @param[in] description - Description of the root menu. 
	/// 	   @param[in] inputMode -This parameter controls a way user 
	/// 							 selects commands from the menu. 
	/// 							 Parameter can get following values:
	///                              a. INPUTMODE_BYSHORTCUT - commands are
	/// 													 selected by 
	/// 													typing a shortcut
	/// 													char.
	///                              b.INPUTMODE_BYCOMMAND - commands are 
	/// 												   selected by typing
	/// 												   full command name.
	///                              c.INPUTMODE_BOTH - commands are selected
	/// 											    either by typing a 
	/// 												shortcut or 
	/// 												command name.
	/// 	    @param[in] displayMode - This parameter controls how the menus 
	/// 								 are displayed. 
	/// 								 Parameter can get following values:
	///                                a. DISPLAYMODE_FULL - all sub menus 
	/// 												     entries are 
	/// 												     displayed.
	///                                b. DISPLAYMODE_SILENT - sub menus entries
	/// 													  are not displayed.
	/// 													  To display entries
	/// 													  user will have 
	/// 													  to type '1'.

	CCliMenu(const std::string & name,
			 const std::string & description,
			 EMenuEntryInputMode inputMode = INPUTMODE_BYSHORTCUT,
			 EMenuDisplayMode displayMode = DISPLAYMODE_FULL);


	//////////////////////////////////////////////////////////////////////////
	///	@brief	~CCliMenu
	///         Destructor of CCliMenu class
	virtual ~CCliMenu();

public:

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Start
	/// 		Call this function to start menu execution.
	/// 		@return    CLIMENU_RES_OK  - menu was started successfully.
	/// 				   error code - menu cannot start.
	virtual ECliMenuRes Start();

	//////////////////////////////////////////////////////////////////////////
	///	@brief	AddSubMenu
	///         Add new sub menu
	/// 		@param[in] parentMenuHandle - handle of the sub menu that 
	/// 									  will be a parent of the new
	/// 									  inserted sub menu. 
	/// 									  In a case parent menu is a 
	/// 									  root menu, use MENU_ROOT 
	/// 									  value for this argument. 
	/// 		@param[out] pNewMenuHandle - on exit this parameter will 
	/// 									 hold a handle value of the new
	/// 									 inserted sub menu
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
	virtual ECliMenuRes AddSubMenu(MenuHandle parentMenuHandle,
								   MenuHandle* pNewMenuHandle,
								   const std::string & name,
								   const std::string & description,
								   uint8 shortcut);

	//////////////////////////////////////////////////////////////////////////
	///	@brief	AddCommandToSubMenu
	///         Add new command to some sub menu
	/// 		@param[in] parentMenuHandle - handle of the sub menu that 
	/// 									  will be a parent of the new
	/// 									  inserted command. 
	/// 									  In a case parent menu is a 
	/// 									  root menu, use MENU_ROOT 
	/// 									  value for this argument.
	/// 		@param[in] pNewCmd	- pointer to command object. Command object's
	/// 							  class should be some class derived
	/// 							 from CCommand
	/// 		@return    CLIMENU_RES_OK  - menu was started successfully.
	/// 				   error code - menu cannot start.
	virtual ECliMenuRes AddCommandToSubMenu(MenuHandle parentMenuHandle,
											CCommand* pNewCmd);

	//////////////////////////////////////////////////////////////////////////
	///	@brief	SetIOStreams
	///         Redirects i/o streams
	/// 		@param[in] iStream - input stream (e.g. cin, or fstream)
	/// 
	/// 		@param[in] oStream	- output stream (e.g. cout, or fstream)
	/// 
	/// 		@return    CLIMENU_RES_OK  - menu was started successfully.
	/// 				   error code - menu cannot start.

	virtual ECliMenuRes SetIOStreams(std::istream& iStream = std::cin,
									 std::ostream& oStream = std::cout);

	virtual ECliMenuRes ResetIOStreams();
//	virtual CLIMENU_RES GetCurSubMenuHandle(MenuHandle* pCurMenuHandle) const;



protected:

	int m_LastCommandIndex;

	int m_LastMenuIndex;

	int m_CurAllocNumOfCommands;

	int m_CurAllocNumOfMenus;

	MenuHandle m_curMenuHandle;

	CCommand** m_pCommands;

	CSubMenu**	m_pMenus;

	EMenuEntryInputMode m_inputMode;

	EMenuDisplayMode    m_displayMode;

//	std::istream*  m_piStream;

//	std::ostream*  m_poStream;

	streambuf* m_pOutBuf;

	streambuf* m_pInBuf;



protected:

	int IncreaseNumOfCommands();

	int IncreaseNumOfMenus();

	virtual ECliMenuRes AddSubMenu( MenuHandle 	parentMenuHandle,
									CSubMenu*  	pNewSubMenu,
									MenuHandle* pNewMenuHandle );


};

#endif //CLIMENU_H

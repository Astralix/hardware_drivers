///////////////////////////////////////////////////////////////////////////////
///
/// @file    	SubMenu.cpp
///
/// @brief		This file provides an implementation of CSubMenu class
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
/// 

#include "SubMenu.h"
#include "ActionCommand.h"
#include <iostream>
#include <iomanip>
#ifndef ANDROID
#include <libio.h>
#else
#include <cstdio>
#endif //ANDROID

CSubMenu::CSubMenu(const std::string & name,
		  const std::string & description,
		  uint8 shortcut,
		  EMenuEntryInputMode inputMode,
		  EMenuDisplayMode displayMode) : \
						CMenuEntry(name,description,shortcut),
						m_inputMode(inputMode),
						m_displayMode(displayMode)
{

	m_pReturnCommand = new CActionCommand("Exit","Exit from this menu",'0');

	AddMenuEntry(m_pReturnCommand);



}

CSubMenu::~CSubMenu()
{
	m_menuEntries.clear();

	if(m_pReturnCommand != NULL)
	{
		delete m_pReturnCommand;
	}

}


/**
 * Execute the command.
 */
void CSubMenu::Execute()
{

	CMenuEntry* cmd;

	while (1) {

		DisplayMenu(m_displayMode);

		if(GetInputCommand(&cmd)) {

			break;
			
		}

		if(cmd != NULL) {
			
			ProcessCommand(cmd);

		}


	}
}


/**
 * @return if command or submenu.
 */
MenuEntryType CSubMenu::GetEntryType() const
{
	return MENUENTRY_SUBMENU;
}

ECliMenuRes CSubMenu::AddMenuEntry(CMenuEntry* newEntry)
{

	if(newEntry != NULL) {

		newEntry->SetParent(this);

		m_menuEntries.push_back(newEntry);

	}
	
	return CLIMENU_RES_OK;
}




bool CSubMenu::GetInputCommand(CMenuEntry** cmd) const
{

	if(m_displayMode != DISPLAYMODE_SILENT) {
		cout << endl<< "Choose command: " << endl;
	}

	std::string strPath;
	GetPath(strPath);

	cout << strPath << "\\>";

	int iShortcutKey;

	uint8 shortcutKey;

	iShortcutKey = cin.get();

	if(iShortcutKey == EOF)
	{
        return true;
	}

	shortcutKey = (uint8)iShortcutKey;

    if(m_inputMode == INPUTMODE_BOTH || m_inputMode == INPUTMODE_BYCOMMAND)
	{

		std::string command;

		if (cin.peek() != '\n' || m_inputMode == INPUTMODE_BYCOMMAND)
		{
			command += (char)shortcutKey;

			if (cin.peek() != '\n') 
			{
				
				std::string inpStr;

				cin >> inpStr;

                cin.ignore(INT_MAX, '\n');

				command += inpStr;

			}

			if(CheckForReturnCmd(command)) {

				return true;
			}

			if(m_displayMode == DISPLAYMODE_SILENT) {

				if(CheckForDisplayCmd(command)) {

					DisplayMenu(DISPLAYMODE_FULL);

					*cmd = NULL;

					return false;

				}
			}

			MenuEntriesList::const_iterator it = m_menuEntries.begin();

			while ((it != m_menuEntries.end()) && !(*it)->CanHandle(command)) {
					it++;
			}

			if (it != m_menuEntries.end()) {

				//*cmd = dynamic_cast<CMenuEntry*>(*it);
				*cmd = *it;

				return false;
			} 

			else {

				cout << "Command not recognized" << endl;

				*cmd = NULL;

				return false;
			}
		}
	}

	//Shortcut Mode

	cin.ignore(INT_MAX, '\n');

	MenuEntriesList::const_iterator it = m_menuEntries.begin();

	if(CheckForReturnCmd(shortcutKey)) {

		return true;

	}

	if(m_displayMode == DISPLAYMODE_SILENT) {

		if(CheckForDisplayCmd(shortcutKey)) {

			DisplayMenu(DISPLAYMODE_FULL);

			*cmd = NULL;

			return false;

		}

	}

	while ((it != m_menuEntries.end()) && !(*it)->CanHandle(shortcutKey)) {
		it++;
	}

	if (it != m_menuEntries.end()) {

		//*cmd = dynamic_cast<CMenuEntry*>(*it);
		*cmd = *it;

		return false;

	} 

	else {

		cout << "Shortcut not recognized" << endl;

		*cmd = NULL;

		return false;

	}

}

bool CSubMenu::CheckForReturnCmd(const std::string& command) const
{

	if(command == "Exit") {

        return true;
	}

	return false;

}

bool CSubMenu::CheckForReturnCmd(uint8 shortcutKey) const
{

    if(shortcutKey == '0') {

        return true;
	}

	
	return false;
}

bool CSubMenu::CheckForDisplayCmd(const std::string& command) const
{

    if(command == "Display") {

		return true;
	}

	return false;

}

bool CSubMenu::CheckForDisplayCmd(uint8 shortcutKey) const
{

	if(shortcutKey == '1') {

		return true;
	}

	return false;
}


void CSubMenu::ProcessCommand(CMenuEntry* cmd) const
{

	if(cmd != NULL) {

		cmd->Execute();

	}

}

void CSubMenu::DisplayMenu(EMenuDisplayMode displayMode) const
{

	if(displayMode != DISPLAYMODE_SILENT) {

		char entryType;

		cout << endl <<  endl << endl;

		cout<<"-------------------------------------------------------"<<endl;
		cout <<this->Name() << ":\t" << this->Desc() << endl;
		cout<<"-------------------------------------------------------"<<endl<<endl;
		cout<<setw(10)<<left<<"Key"<<setw(20)<<left<<"Command"<<setw(20) \
			<<left<<"(M)enu\\(C)md"<<setw(30)<<left<<"Description"<<endl;

		cout<<"======================================================="<<endl;

		for (MenuEntriesList::const_iterator it = m_menuEntries.begin() ; \
			  it != m_menuEntries.end() ; it++) {

			if((*it)->GetEntryType() == MENUENTRY_CMD) {
				entryType = 'C';
			}
			else {
				entryType = 'M';
			}

			cout << "(" << (char)((*it)->Shortcut()) << ")" 				\
				 <<setw(7)<<left<< "" <<setw(20)<<left<< (*it)->Name() 		\
				 <<setw(6)<<""<<setw(13)<<left<<entryType<<setw(20)<<left 	\
				 << (*it)->Desc() << endl;

		}

		cout << endl;
	}

}


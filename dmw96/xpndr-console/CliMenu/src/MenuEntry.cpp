///////////////////////////////////////////////////////////////////////////////
///
/// @file    	MenuEntry.cpp
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

#include "MenuEntry.h"

CMenuEntry::CMenuEntry(const std::string & name,
					   const std::string & description,
					   uint8 shortcut):m_strName(name),
					   m_strDescription(description),
					   m_iShortcut(shortcut),
					   m_parent(NULL)

									
{

}

CMenuEntry::~CMenuEntry()
{
	
}


/**
 * @return true if this Command class can handle the command.
 */

bool CMenuEntry::CanHandle(const std::string & command) const
{

	return (command == Name());

}


/**
 * @return true if this Command class can handle the shortcut.
 */
bool CMenuEntry::CanHandle(uint8 shortcut) const
{
	return (shortcut == Shortcut());
}



const CMenuEntry* CMenuEntry::GetParent() const
{

	return m_parent;
}


/**
 * @return the Command name.
 */
const std::string& CMenuEntry::Name() const
{

	return m_strName;
}

/**
 * @return a description.
 */
const std::string& CMenuEntry::Desc() const
{

	return m_strDescription;

}

/**
 * @return a shortcut.
 */
int CMenuEntry::Shortcut() const
{

	return m_iShortcut;
}

/**
 * @set parent to this menu.
 */
void CMenuEntry::SetParent(CMenuEntry* parent)
{

	m_parent = parent;

}

void CMenuEntry::GetPath(std::string& strPath) const
{

	strPath = "";

	const CMenuEntry* curParent = GetParent();

	if(curParent) {
		curParent->GetPath(strPath);
	}
	strPath += "\\";
	strPath += m_strName;
}


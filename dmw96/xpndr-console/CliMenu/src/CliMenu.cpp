///////////////////////////////////////////////////////////////////////////////
///
/// @file    	CLIMenu.cpp
///
/// @brief		This class provides an implementation for 
/// 			building and operating CLI Menu. 
///
///
/// @internal
///
/// @author  	ronyr, sharons
///
/// @date    	21/08/2008
///
/// @version 	1.1
///
///
///				Copyright (C) 2001-2008 DSP GROUP, INC.   All Rights Reserved
///	
/// 
#include "SubMenu.h"
#include "CliMenu.h"
#ifndef ANDROID
#include "infra/include/InfraMacros.h"
#endif //ANDROID

#define CLIMENU_INIT_NUM_OF_MENUS 50
#define CLIMENU_INIT_NUM_OF_COMMANDS 100
#define CLIMENU_MENUS_INCREASE_FACTOR 50
#define CLIMENU_COMMANDS_INCREASE_FACTOR 100

CCliMenu::CCliMenu(const std::string & name,
				   const std::string & description,
				   EMenuEntryInputMode inputMode,
				   EMenuDisplayMode displayMode) : \
					m_CurAllocNumOfCommands(CLIMENU_INIT_NUM_OF_COMMANDS),
					m_CurAllocNumOfMenus(CLIMENU_INIT_NUM_OF_MENUS),
					m_inputMode(inputMode),
					m_displayMode(displayMode),
					m_curMenuHandle(MENU_ROOT),
					m_LastCommandIndex(0),
					m_LastMenuIndex(MENU_ROOT)

{
	

	


//		m_piStream = &(std::cin);

//		m_poStream = &(std::cout);

	m_pInBuf = std::cin.rdbuf();

	m_pOutBuf = std::cout.rdbuf();


	m_pCommands = new CCommand *[m_CurAllocNumOfCommands];

	int ind;

	for(ind=0;ind < m_CurAllocNumOfCommands;ind++)
	{
		m_pCommands[ind] = NULL;
	}

	m_pMenus 	= new CSubMenu *[m_CurAllocNumOfMenus];

	for(ind=0;ind < m_CurAllocNumOfMenus;ind++)
	{
		m_pMenus[ind] = NULL;
	}

	m_pMenus[m_LastMenuIndex] =
		new CSubMenu(name,description,'a',m_inputMode,m_displayMode);


}

CCliMenu::~CCliMenu()
{
	int ind;

	if(m_pMenus != NULL)
	{
		for(ind = 0;ind <= m_LastMenuIndex;ind++)
		{
			delete m_pMenus[ind];
		}

		delete []m_pMenus;
	}

	if(m_pCommands != NULL)
	{
		delete []m_pCommands;
	}

	ResetIOStreams();

}

ECliMenuRes  CCliMenu::Start()
{
	if(m_LastMenuIndex > 0)
	{
#ifndef ANDROID
		INFRA_ASSERT(m_pMenus);
		INFRA_ASSERT(m_pMenus[0]);
#else
		if(!m_pMenus || !m_pMenus[0]) 
			return CLIMENU_RES_ARG_ERR;
#endif //ANDROID

		m_pMenus[0]->Execute();
	}

	return CLIMENU_RES_OK;

}

ECliMenuRes  CCliMenu::AddSubMenu(MenuHandle parentMenuHandle,
								  CSubMenu* pNewSubMenu,
								  MenuHandle* pNewMenuHandle)
{
	int res;

	*pNewMenuHandle = 0;

	if(parentMenuHandle > m_LastMenuIndex)
	{
		return CLIMENU_RES_INDEX_ERR;
	}

	if(m_LastMenuIndex + 1 == m_CurAllocNumOfMenus)
	{

		res = IncreaseNumOfMenus();

		if(res < 0)
		{
			return CLIMENU_RES_MEMERR;
		}
	}
#ifndef ANDROID
	INFRA_ASSERT(m_pMenus);
#else
	if(!m_pMenus)
		return CLIMENU_RES_ARG_ERR;
#endif //ANDROID
    
    m_pMenus[m_LastMenuIndex + 1] = pNewSubMenu;
														 
	res = (m_pMenus[parentMenuHandle])->AddMenuEntry(pNewSubMenu);

	if(res < 0)
	{
		return CLIMENU_RES_ADDENTRY_ERR;
	}

	m_LastMenuIndex = m_LastMenuIndex + 1;

	*pNewMenuHandle = m_LastMenuIndex;

	return CLIMENU_RES_OK;

}

ECliMenuRes  CCliMenu::AddSubMenu(MenuHandle parentMenuHandle,
								  MenuHandle* pNewMenuHandle,
								  const std::string & name,
								  const std::string & description,
								  uint8 shortcut)
{
	*pNewMenuHandle = 0;

	if(parentMenuHandle > m_LastMenuIndex)
	{
		return CLIMENU_RES_INDEX_ERR;
	}

	CSubMenu* pNewSubMenu = new CSubMenu(name,
										 description,
										 shortcut,
										 m_inputMode,
										 m_displayMode);

	if(pNewSubMenu == NULL)
	{
		return CLIMENU_RES_ADDENTRY_ERR;
	}

	ECliMenuRes res = AddSubMenu(parentMenuHandle,pNewSubMenu,pNewMenuHandle);

	if(res != CLIMENU_RES_OK)
	{
		delete pNewSubMenu;
	}

	return res;


}

ECliMenuRes  CCliMenu::AddCommandToSubMenu(MenuHandle parentMenuHandle,
										   CCommand* pNewCmd)
{
	int res;

	if(parentMenuHandle > m_LastMenuIndex)
	{
		return CLIMENU_RES_INDEX_ERR;
	}

	if(m_LastCommandIndex + 1 == m_CurAllocNumOfCommands)
	{
		res = IncreaseNumOfCommands();

		if(res < 0)
		{
			return CLIMENU_RES_MEMERR;
		}
	}
#ifndef ANDROID
	INFRA_ASSERT(m_pCommands);
#else
	if(!m_pCommands)
		return CLIMENU_RES_ARG_ERR;
#endif //ANDROID
    
    m_pCommands[m_LastCommandIndex + 1] = pNewCmd;

	res = (m_pMenus[parentMenuHandle])->AddMenuEntry(pNewCmd);

	if(res < 0)
	{
		return CLIMENU_RES_ADDENTRY_ERR;
	}

	m_LastCommandIndex = m_LastCommandIndex + 1;

	return CLIMENU_RES_OK;
}


int CCliMenu::IncreaseNumOfMenus()
{
	int newAllocNumOfMenus = m_CurAllocNumOfMenus + 
							 CLIMENU_MENUS_INCREASE_FACTOR;

	CSubMenu** pNewMenus 	= new CSubMenu *[newAllocNumOfMenus];

	if(pNewMenus == NULL)
	{
		return -1;
	}

	int ind;

	for(ind=0; ind < m_LastMenuIndex;ind++)
	{
		pNewMenus[ind] = m_pMenus[ind];
	}

	for(ind = m_LastMenuIndex; ind < newAllocNumOfMenus;ind++)
	{
		pNewMenus[ind] = NULL;
	}

	m_CurAllocNumOfMenus = newAllocNumOfMenus;

	m_pMenus = pNewMenus;

	return 0;

}

int CCliMenu::IncreaseNumOfCommands()
{
	int newAllocNumOfCommands = m_CurAllocNumOfCommands + 
								CLIMENU_COMMANDS_INCREASE_FACTOR;

	CCommand** pNewCommands 	= new CCommand *[newAllocNumOfCommands];

	if(pNewCommands == NULL)
	{
		return -1;
	}

	int ind;

	for(ind=0; ind < m_LastCommandIndex;ind++)
	{
		pNewCommands[ind] = m_pCommands[ind];
	}
	for(ind=m_LastCommandIndex;ind<newAllocNumOfCommands;ind++)
	{
		pNewCommands[ind] = NULL;
	}

	m_CurAllocNumOfCommands = newAllocNumOfCommands;

	m_pCommands = pNewCommands;

	return 0;

}

ECliMenuRes CCliMenu::SetIOStreams(std::istream& iStream,
								   std::ostream& oStream)
{
    if(iStream == 0 || oStream==0)
	{
		return CLIMENU_RES_ARG_ERR;
	}
		
	streambuf *psbuf;

	
	psbuf = iStream.rdbuf();
	
	std::cin.rdbuf(psbuf);
	
	psbuf = oStream.rdbuf();
	
	std::cout.rdbuf(psbuf);
    
//		m_piStream = iStream;

//		m_poStream = oStream;

	return CLIMENU_RES_OK;
}

ECliMenuRes CCliMenu::ResetIOStreams()
{
#ifndef ANDROID
	INFRA_ASSERT(m_pInBuf);
#else
	if(!m_pInBuf)
		return CLIMENU_RES_ARG_ERR;
#endif //ANDROID
	
#ifndef ANDROID
	INFRA_ASSERT(m_pOutBuf);
#else
	if(!m_pOutBuf)
		return CLIMENU_RES_ARG_ERR;
#endif //ANDROID

    
	std::cin.rdbuf(m_pInBuf);

	std::cout.rdbuf(m_pOutBuf);

	return CLIMENU_RES_OK;
}

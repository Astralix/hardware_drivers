///////////////////////////////////////////////////////////////////////////////
///
/// @file    	Common.h
///
/// @brief		This file provides general definitions used across the module
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
#ifndef COMMON_H
#define COMMON_H


#define MENU_ROOT 0

typedef int MenuHandle;

enum ECliMenuRes
{
	CLIMENU_RES_OK,
	CLIMENU_RES_INDEX_ERR,
	CLIMENU_RES_MEMERR,
    CLIMENU_RES_ADDENTRY_ERR,
	CLIMENU_RES_ARG_ERR


};


enum EMenuEntryInputMode
{

	INPUTMODE_BYSHORTCUT,
	INPUTMODE_BYCOMMAND,
	INPUTMODE_BOTH
};

enum EMenuDisplayMode
{
	DISPLAYMODE_SILENT,
	DISPLAYMODE_FULL
};


#endif //COMMON_H

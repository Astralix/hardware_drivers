///////////////////////////////////////////////////////////////////////////////
///
/// @file    	CliUtils.h
///
/// @brief		Definition of CCliUtils class that
/// 			provides useful tools for dealing with i/o
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
#ifndef CLIUTILS_H
#define CLIUTILS_H

#include "TypeDefs.h"
#include <string>


class CCliUtils {

public:

	static bool GetFromUser_Int(int* piInput,
								const std::string& _strPrompt);

	static bool GetFromUser_HexInt(uint32* piInput,
								   const std::string& _strPrompt);

	static bool GetFromUser_MacAddr(uint8* pMacAddr,
									const std::string& strPrompt);

	static bool GetFromUser_String(std::string& strOut,
								   const std::string& strPrompt);

};

#endif //CLIUTILS_H

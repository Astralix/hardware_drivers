///////////////////////////////////////////////////////////////////////////////
///
/// @file    	CliUtils.h
///
/// @brief		Implementation of CCliUtils class that
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
#include "CliUtils.h"
#include <iostream>

using namespace std;

bool CCliUtils::GetFromUser_Int(int* piInput,const std::string& strPrompt)
{

	int iInp;
	cout<< strPrompt << ">";

		cin>>std::dec>>iInp;

	if(!std::cin)
	{
	  std::cin.clear();
	  std::cin.ignore(INT_MAX,'\n');
	  return false;
	}

	cin.ignore(INT_MAX, '\n');

	*piInput = iInp;

	return true;
}


bool CCliUtils::GetFromUser_HexInt(uint32* piInput,const std::string& strPrompt)
{

	uint32 iInp;
	cout<< strPrompt << ">";

	cin>>std::hex>>iInp;

	if(!std::cin)
	{
	  std::cin.clear();
	  std::cin.ignore(INT_MAX,'\n');
	  return false;
	}

	cin.ignore(INT_MAX, '\n');
	*piInput = iInp;

	return true;
}

bool CCliUtils::GetFromUser_MacAddr(uint8* pMacAddr,
									const std::string& strPrompt)
{

	if(pMacAddr == NULL)
	{
		return false;
	}

	char colon;
	cout<< strPrompt << ">";
	uint byteAddrArr[6];

	for(int byteAddr=0;byteAddr < 6; byteAddr++)
	{
		cin>>std::hex>>byteAddrArr[byteAddr];

		if(!std::cin)
		{
			goto error_handling;
		}

		if(byteAddrArr[byteAddr] > 0xFF) {

			goto error_handling;

		}

		if(byteAddr < 5) {

			if(cin.peek() != ':') {

				goto error_handling;
			}

			cin>>colon;

		}


	}

	cin.ignore(INT_MAX, '\n');

	for(int byteAddr=0;byteAddr < 6; byteAddr++)
	{
		pMacAddr[byteAddr] = byteAddrArr[byteAddr];
	}


	return true;


error_handling:
  std::cin.clear();
  std::cin.ignore(INT_MAX,'\n');
  return false;


}

bool CCliUtils::GetFromUser_String(std::string& strOut,const std::string& strPrompt)
{

	int iInp;
	cout<< strPrompt << ">";

	cin>>strOut;

	if(!std::cin)
	{
	  std::cin.clear();
	  std::cin.ignore(INT_MAX,'\n');
	  return false;
	}

	cin.ignore(INT_MAX, '\n');

	return true;
}


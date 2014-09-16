#include "WifiCLICommand.h"
#include "CliMenu/include/CliUtils.h"
#include <iostream>

CWifiCliCommand::CWifiCliCommand(CWifiTestMgr* pWifiTestMgr,
								 const std::string & name,
								 const std::string & description,
								 uint8 shortcut):CActionCommand(name,
															  description,
															  shortcut),
												m_pWifiTestMgr(pWifiTestMgr)
															  
{

}

CWifiCliCommand::~CWifiCliCommand()
{

}

void CWifiCliCommand::Action()
{
	if(m_pWifiTestMgr == NULL) {

		DisplayErrorMsg("Test manager is not itilialized");

		return;
	}

	WifiAction();
}

void CWifiCliCommand::WifiAction()
{

	//Default implementation does nothing

}

void CWifiCliCommand::DisplayErrorMsg(const std::string& strErr)
{
	cout<<"Result: Error"<<endl;

	cout<<strErr<<endl;
}

void CWifiCliCommand::DisplayErrorMsg(const std::string& strErr,int errCode)
{
	cout<<"Result: Error"<<endl;

	cout<<strErr<<" Error code is:\t"<<errCode<<endl;
}


void CWifiCliCommand::DisplaySuccessMsg(const std::string& strMsg)
{
	cout<<"Result: Success"<<endl;

	cout<<strMsg<<endl;

}

void CWifiCliCommand::DisplayMsg(const std::string& strMsg)
{
	cout<<strMsg<<endl;
}

bool CWifiCliCommand::GetInputFromUser_Int(int* piInput,
										   const std::string& strPromtMsg)
{
	return CCliUtils::GetFromUser_Int(piInput,strPromtMsg);
}

bool CWifiCliCommand::GetInputFromUser_HexInt(uint32* piInput,
											  const std::string& strPromtMsg)
{
	return CCliUtils::GetFromUser_HexInt(piInput,strPromtMsg);
}

bool CWifiCliCommand::GetInputFromUser_MacAddr(uint8* pMacAddr,
											   const std::string& strPromtMsg)
{
	return CCliUtils::GetFromUser_MacAddr(pMacAddr,strPromtMsg);
}

bool CWifiCliCommand::GetInputFromUser_String(std::string& strOut,
											  const std::string& strPromtMsg)
{
	return CCliUtils::GetFromUser_String(strOut,strPromtMsg);
}

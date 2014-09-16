#ifndef WIFICLICOMMAND_H
#define WIFICLICOMMAND_H

#include "WifiMgr/include/WifiTestMgr.h"
#include "CliMenu/include/ActionCommand.h"
#include <string>
using namespace std;


class CWifiCliCommand : public CActionCommand
{
public:

	CWifiCliCommand(CWifiTestMgr* pWifiTestMgr,
					const std::string & name,
				    const std::string & description,
				    uint8 shortcut);

	virtual ~CWifiCliCommand();

	//////////////////////////////////////////////////////////////////////////
	///	@brief	Action
	///         Function that should be implemented by derived classes to specify
	/// 		the behaviour of the command when it is selected
	virtual void Action();

	virtual void WifiAction();

	virtual void DisplayErrorMsg(const std::string& strErr);

	virtual void DisplayErrorMsg(const std::string& strErr,int errCode);

	virtual void DisplaySuccessMsg(const std::string& strMsg);

	virtual void DisplayMsg(const std::string& strMsg);

	virtual bool GetInputFromUser_Int(int* piInput,const std::string& strPromtMsg);

	virtual bool GetInputFromUser_HexInt(uint32* piInput,const std::string& strPromtMsg);

	virtual bool GetInputFromUser_MacAddr(uint8* pMacAddr,const std::string& strPromtMsg);

	virtual bool GetInputFromUser_String(std::string& strOut,const std::string& strPromtMsg);

protected:

	CWifiTestMgr* m_pWifiTestMgr;

};

#endif //WIFICLICOMMAND_H

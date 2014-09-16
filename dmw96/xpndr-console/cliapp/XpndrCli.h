#ifndef XPNDRCLI_H
#define XPNDRCLI_H

#include "WifiMgr/include/WifiTestMgr.h"
#include "CliMenu/include/CliMenu.h"
#include "CliMenu/include/ActionCommand.h"
#include "RxSettings.h"
#include "TxPacketSettings.h"
#include <fstream>
#include <strstream>

#ifdef ANDROID
#include <assert.h>
#ifndef ASSERT
#define ASSERT assert
#endif //ASSERT
#endif //ANDROID

#define MAX_NUMBER_OF_CLI_COMMANDS 150

class CXpndrWifiCli 
{
public:

	//cin input
	CXpndrWifiCli(CWifiTestMgr* 	pWifiTestMgr,
				  bool bIsSilent = false);

	//file input
	CXpndrWifiCli(CWifiTestMgr* 	pWifiTestMgr,
                  char* fileName,
				  bool bIsSilent = false);
	//stream input
	CXpndrWifiCli(CWifiTestMgr*  pWifiTestMgr,
				  char* pArgsArr[],
				  int numOfStreamArgs,
				  bool bIsSilent = false);


	~CXpndrWifiCli();

	int Run();


private:

	void CreateMenu(bool bIsSilent);

	ECliMenuRes BuildMenus();

	int RedirectInputToFile(char* fileName);

	int RedirectInputToArgStream(char* pArgsArr[],
								 int numOfStreamArgs);


	CCliMenu* m_pMenu;

	CWifiTestMgr* 	m_pWifiTestMgr;

	CRxSettings*	m_pRxSettings;

	CTxPacketSettings* m_pTxPacketSettings;

	CActionCommand* m_pCmdArr[MAX_NUMBER_OF_CLI_COMMANDS];

	std::ifstream m_filestr;

	std::strstream  m_argInputStream;

};

#endif //XPNDRCLI_H
